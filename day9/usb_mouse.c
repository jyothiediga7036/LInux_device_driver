// SPDX-License-Identifier: GPL-2.0
/*
 * basic_usb_mouse.c
 *
 * Minimal Linux USB boot-protocol mouse driver.
 *
 * Supports:
 *   - Left mouse button
 *   - Right mouse button
 *   - Middle mouse button
 *   - Relative X movement
 *   - Relative Y movement
 *   - Optional mouse wheel
 *
 * This is a teaching driver and is not a replacement for usbhid.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/input.h>
#include <linux/slab.h>

#define DRIVER_NAME "basic_usb_mouse"

/*
 * Replace these values using the output from:
 *
 *     lsusb
 */
#define MOUSE_VENDOR_ID  0x1234
#define MOUSE_PRODUCT_ID 0x5678

/*
 * USB HID boot-mouse interface:
 *
 * Class    = 0x03: HID
 * Subclass = 0x01: Boot interface
 * Protocol = 0x02: Mouse
 */
#define HID_CLASS          0x03
#define HID_BOOT_SUBCLASS  0x01
#define HID_MOUSE_PROTOCOL 0x02

/*
 * HID class request used to select boot protocol.
 */
#define HID_SET_PROTOCOL   0x0b
#define HID_BOOT_PROTOCOL  0

/*
 * A basic boot-mouse packet generally needs only three or four
 * bytes. Eight bytes gives this teaching driver some extra room.
 */
#define MAX_REPORT_SIZE 8

struct usb_mouse {
    /*
     * Physical USB device.
     */
    struct usb_device *udev;

    /*
     * USB interface bound to this driver.
     */
    struct usb_interface *intf;

    /*
     * Linux input subsystem device.
     */
    struct input_dev *input;

    /*
     * Interrupt transfer request.
     */
    struct urb *urb;

    /*
     * Buffer receiving the mouse report.
     */
    unsigned char *data;
    size_t data_size;

    /*
     * Prevent URB resubmission after disconnect.
     */
    bool disconnected;
};

/*
 * Called whenever one mouse report is received.
 */
static void usb_mouse_irq(struct urb *urb)
{
    struct usb_mouse *mouse = urb->context;
    unsigned char *data = mouse->data;
    int ret;

    /*
     * Check transfer completion status.
     */
    switch (urb->status) {
    case 0:
        /*
         * Successful transfer.
         */
        break;

    case -ECONNRESET:
    case -ENOENT:
    case -ESHUTDOWN:
        /*
         * URB was stopped because the mouse was disconnected
         * or the driver was unbound.
         */
        return;

    default:
        /*
         * Temporary transfer error.
         * Try submitting the URB again.
         */
        goto resubmit;
    }

    /*
     * Basic boot-mouse report:
     *
     * Byte 0: Button bitmap
     * Byte 1: Signed X movement
     * Byte 2: Signed Y movement
     * Byte 3: Optional signed wheel movement
     */
    if (urb->actual_length >= 3) {
        /*
         * Byte 0 button bitmap:
         *
         * Bit 0: Left button
         * Bit 1: Right button
         * Bit 2: Middle button
         */
        input_report_key(
            mouse->input,
            BTN_LEFT,
            !!(data[0] & BIT(0))
        );

        input_report_key(
            mouse->input,
            BTN_RIGHT,
            !!(data[0] & BIT(1))
        );

        input_report_key(
            mouse->input,
            BTN_MIDDLE,
            !!(data[0] & BIT(2))
        );

        /*
         * Relative mouse movement.
         *
         * Cast to s8 because movement may be negative.
         */
        input_report_rel(
            mouse->input,
            REL_X,
            (s8)data[1]
        );

        input_report_rel(
            mouse->input,
            REL_Y,
            (s8)data[2]
        );

        /*
         * Some boot mice provide wheel movement in byte 3.
         */
        if (urb->actual_length >= 4) {
            input_report_rel(
                mouse->input,
                REL_WHEEL,
                (s8)data[3]
            );
        }

        /*
         * Mark the end of this input report.
         */
        input_sync(mouse->input);
    }

resubmit:
    /*
     * Do not restart the transfer after disconnect.
     */
    if (READ_ONCE(mouse->disconnected))
        return;

    /*
     * Submit the same URB again to receive the next report.
     *
     * The completion callback runs in atomic context, so use
     * GFP_ATOMIC.
     */
    ret = usb_submit_urb(urb, GFP_ATOMIC);
    if (ret) {
        dev_err_ratelimited(
            &mouse->intf->dev,
            "URB resubmit failed: %d\n",
            ret
        );
    }
}

/*
 * Called when USB core finds a matching mouse interface.
 */
static int usb_mouse_probe(
    struct usb_interface *intf,
    const struct usb_device_id *id)
{
    struct usb_device *udev;
    struct usb_host_interface *alt;
    struct usb_endpoint_descriptor *ep = NULL;
    struct usb_mouse *mouse;
    unsigned int max_packet;
    int i;
    int ret;

    udev = interface_to_usbdev(intf);
    alt = intf->cur_altsetting;

    /*
     * Search for the interrupt-IN endpoint.
     */
    for (i = 0; i < alt->desc.bNumEndpoints; i++) {
        struct usb_endpoint_descriptor *candidate;

        candidate = &alt->endpoint[i].desc;

        if (usb_endpoint_is_int_in(candidate)) {
            ep = candidate;
            break;
        }
    }

    if (!ep) {
        dev_err(
            &intf->dev,
            "interrupt-IN endpoint not found\n"
        );

        return -ENODEV;
    }

    max_packet = usb_endpoint_maxp(ep);

    /*
     * A basic boot-mouse report needs at least:
     *
     * buttons + X + Y = three bytes
     */
    if (max_packet < 3) {
        dev_err(
            &intf->dev,
            "endpoint packet size %u is too small\n",
            max_packet
        );

        return -ENODEV;
    }

    /*
     * Allocate the driver's private structure.
     */
    mouse = kzalloc(sizeof(*mouse), GFP_KERNEL);
    if (!mouse)
        return -ENOMEM;

    mouse->udev = usb_get_dev(udev);
    mouse->intf = intf;

    mouse->data_size = min_t(
        size_t,
        max_packet,
        MAX_REPORT_SIZE
    );

    /*
     * Allocate the report buffer.
     */
    mouse->data = kzalloc(
        mouse->data_size,
        GFP_KERNEL
    );

    /*
     * Allocate one interrupt URB.
     */
    mouse->urb = usb_alloc_urb(0, GFP_KERNEL);

    /*
     * Allocate a Linux input device.
     */
    mouse->input = input_allocate_device();

    if (!mouse->data ||
        !mouse->urb ||
        !mouse->input) {
        ret = -ENOMEM;
        goto error;
    }

    /*
     * Configure the Linux input device.
     */
    mouse->input->name = "Basic USB Mouse";
    mouse->input->dev.parent = &intf->dev;

    mouse->input->id.bustype = BUS_USB;

    mouse->input->id.vendor =
        le16_to_cpu(udev->descriptor.idVendor);

    mouse->input->id.product =
        le16_to_cpu(udev->descriptor.idProduct);

    mouse->input->id.version =
        le16_to_cpu(udev->descriptor.bcdDevice);

    /*
     * Declare the supported mouse buttons.
     */
    input_set_capability(
        mouse->input,
        EV_KEY,
        BTN_LEFT
    );

    input_set_capability(
        mouse->input,
        EV_KEY,
        BTN_RIGHT
    );

    input_set_capability(
        mouse->input,
        EV_KEY,
        BTN_MIDDLE
    );

    /*
     * Declare relative movement capabilities.
     */
    input_set_capability(
        mouse->input,
        EV_REL,
        REL_X
    );

    input_set_capability(
        mouse->input,
        EV_REL,
        REL_Y
    );

    input_set_capability(
        mouse->input,
        EV_REL,
        REL_WHEEL
    );

    /*
     * Prepare the interrupt URB.
     */
    usb_fill_int_urb(
        mouse->urb,
        udev,
        usb_rcvintpipe(
            udev,
            ep->bEndpointAddress
        ),
        mouse->data,
        mouse->data_size,
        usb_mouse_irq,
        mouse,
        ep->bInterval
    );

    /*
     * Register the input device.
     *
     * After this succeeds, Linux creates an event interface
     * such as:
     *
     *     /dev/input/event5
     */
    ret = input_register_device(mouse->input);
    if (ret)
        goto error;

    /*
     * Save private driver data in the USB interface.
     */
    usb_set_intfdata(intf, mouse);

    /*
     * Request the simple boot-protocol packet format.
     */
    ret = usb_control_msg(
        udev,
        usb_sndctrlpipe(udev, 0),
        HID_SET_PROTOCOL,
        USB_DIR_OUT |
        USB_TYPE_CLASS |
        USB_RECIP_INTERFACE,
        HID_BOOT_PROTOCOL,
        alt->desc.bInterfaceNumber,
        NULL,
        0,
        1000
    );

    if (ret < 0) {
        dev_warn(
            &intf->dev,
            "SET_PROTOCOL failed: %d\n",
            ret
        );
    }

    /*
     * Start receiving mouse packets.
     */
    ret = usb_submit_urb(
        mouse->urb,
        GFP_KERNEL
    );

    if (ret)
        goto error_registered;

    dev_info(
        &intf->dev,
        "mouse connected: VID=%04x PID=%04x "
        "endpoint=0x%02x\n",
        le16_to_cpu(udev->descriptor.idVendor),
        le16_to_cpu(udev->descriptor.idProduct),
        ep->bEndpointAddress
    );

    return 0;

error_registered:
    usb_set_intfdata(intf, NULL);

    /*
     * input_unregister_device() also frees the input device.
     */
    input_unregister_device(mouse->input);
    mouse->input = NULL;

error:
    if (mouse->input)
        input_free_device(mouse->input);

    usb_free_urb(mouse->urb);
    kfree(mouse->data);
    usb_put_dev(mouse->udev);
    kfree(mouse);

    return ret;
}

/*
 * Called when the mouse is removed or the driver is unbound.
 */
static void usb_mouse_disconnect(
    struct usb_interface *intf)
{
    struct usb_mouse *mouse;

    mouse = usb_get_intfdata(intf);

    usb_set_intfdata(intf, NULL);

    if (!mouse)
        return;

    /*
     * Prevent the completion callback from resubmitting
     * the URB.
     */
    WRITE_ONCE(mouse->disconnected, true);

    /*
     * Stop the interrupt transfer and wait for any running
     * completion callback to finish.
     */
    usb_kill_urb(mouse->urb);

    /*
     * Remove /dev/input/eventX and unregister the input device.
     */
    input_unregister_device(mouse->input);

    usb_free_urb(mouse->urb);
    kfree(mouse->data);
    usb_put_dev(mouse->udev);
    kfree(mouse);

    dev_info(
        &intf->dev,
        "mouse disconnected\n"
    );
}

/*
 * Match one particular USB boot-protocol mouse.
 */
static const struct usb_device_id usb_mouse_ids[] = {
    {
        USB_DEVICE_AND_INTERFACE_INFO(
            MOUSE_VENDOR_ID,
            MOUSE_PRODUCT_ID,
            HID_CLASS,
            HID_BOOT_SUBCLASS,
            HID_MOUSE_PROTOCOL
        )
    },
    { }
};

MODULE_DEVICE_TABLE(usb, usb_mouse_ids);

/*
 * Register this driver with the Linux USB core.
 */
static struct usb_driver usb_mouse_driver = {
    .name       = DRIVER_NAME,
    .probe      = usb_mouse_probe,
    .disconnect = usb_mouse_disconnect,
    .id_table   = usb_mouse_ids,
};

module_usb_driver(usb_mouse_driver);

MODULE_AUTHOR("Prashant Soni / TechDhaba");
MODULE_DESCRIPTION("Basic USB boot mouse driver");
MODULE_LICENSE("GPL");

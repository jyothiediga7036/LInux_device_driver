// SPDX-License-Identifier: GPL-2.0
/*
 * usb_char.c - Minimal synchronous USB bulk character driver
 *
 * Features:
 *   - Matches one USB VID/PID
 *   - Detects bulk-IN and bulk-OUT endpoints
 *   - Creates /dev/myusbN
 *   - read()  -> USB bulk-IN transfer
 *   - write() -> USB bulk-OUT transfer
 *   - Handles unplug while the device node is open
 *
 * IMPORTANT:
 *   Replace USB_VENDOR_ID and USB_PRODUCT_ID with your device values
 *   from `lsusb`. The USB device firmware must expose compatible bulk
 *   endpoints and implement the data protocol expected by user space.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/kref.h>

#define DRIVER_NAME            "usb_char"
#define USB_CHAR_MINOR_BASE    192

/* Replace these with values from lsusb. */
#define USB_VENDOR_ID          0x1234
#define USB_PRODUCT_ID         0x5678

#define USB_IO_TIMEOUT_MS      5000
#define USB_MAX_TRANSFER       (16 * 1024)

struct usb_char_device {
    struct usb_device *udev;
    struct usb_interface *interface;

    unsigned char bulk_in_ep;
    unsigned char bulk_out_ep;

    size_t bulk_in_max_packet;
    size_t bulk_out_max_packet;

    /*
     * Serializes read(), write(), and disconnect().
     *
     * usb_bulk_msg() sleeps, so a mutex is appropriate here.
     */
    struct mutex io_mutex;

    /*
     * Keeps the object alive while a user process still has
     * /dev/myusbN open after physical USB disconnection.
     */
    struct kref kref;

    bool disconnected;
};

static struct usb_driver usb_char_driver;

/*
 * Called when the final reference is released.
 */
static void usb_char_delete(struct kref *kref)
{
    struct usb_char_device *dev;

    dev = container_of(kref, struct usb_char_device, kref);

    usb_put_dev(dev->udev);
    kfree(dev);
}

/*
 * Called when a process executes:
 *
 *     open("/dev/myusb0", O_RDWR);
 */
static int usb_char_open(struct inode *inode, struct file *file)
{
    struct usb_interface *interface;
    struct usb_char_device *dev;
    int subminor;
    int ret = 0;

    subminor = iminor(inode);

    /*
     * Find the USB interface associated with this character-device
     * minor number.
     */
    interface = usb_find_interface(&usb_char_driver, subminor);
    if (!interface)
        return -ENODEV;

    dev = usb_get_intfdata(interface);
    if (!dev)
        return -ENODEV;

    /*
     * Keep the object alive until release().
     */
    kref_get(&dev->kref);

    mutex_lock(&dev->io_mutex);

    if (dev->disconnected)
        ret = -ENODEV;
    else
        file->private_data = dev;

    mutex_unlock(&dev->io_mutex);

    if (ret) {
        kref_put(&dev->kref, usb_char_delete);
        return ret;
    }

    /*
     * This character device does not support seeking.
     */
    ret = nonseekable_open(inode, file);
    if (ret) {
        file->private_data = NULL;
        kref_put(&dev->kref, usb_char_delete);
    }

    return ret;
}

/*
 * Called when the application closes the file descriptor.
 */
static int usb_char_release(struct inode *inode, struct file *file)
{
    struct usb_char_device *dev = file->private_data;

    if (!dev)
        return -ENODEV;

    file->private_data = NULL;

    /*
     * Release the reference acquired in open().
     */
    kref_put(&dev->kref, usb_char_delete);

    return 0;
}

/*
 * User-space read:
 *
 *     read(fd, buffer, size);
 *
 * Data flow:
 *
 * USB device
 *      |
 *      | Bulk IN
 *      v
 * usb_bulk_msg()
 *      |
 * copy_to_user()
 *      |
 * application
 */
static ssize_t usb_char_read(struct file *file,
                             char __user *user_buffer,
                             size_t count,
                             loff_t *ppos)
{
    struct usb_char_device *dev = file->private_data;
    unsigned char *buffer;
    size_t transfer_len;
    int actual_length = 0;
    int ret;

    if (!dev)
        return -ENODEV;

    if (count == 0)
        return 0;

    /*
     * Limit each system call to 16 KiB in this teaching example.
     */
    transfer_len = min_t(size_t, count, USB_MAX_TRANSFER);

    buffer = kmalloc(transfer_len, GFP_KERNEL);
    if (!buffer)
        return -ENOMEM;

    mutex_lock(&dev->io_mutex);

    if (dev->disconnected) {
        ret = -ENODEV;
        goto out_unlock;
    }

    /*
     * Synchronous USB bulk-IN transfer.
     *
     * This call sleeps until:
     *   - data arrives,
     *   - an error occurs,
     *   - the device disconnects,
     *   - or the timeout expires.
     */
    ret = usb_bulk_msg(
        dev->udev,
        usb_rcvbulkpipe(dev->udev, dev->bulk_in_ep),
        buffer,
        transfer_len,
        &actual_length,
        USB_IO_TIMEOUT_MS
    );

    if (ret)
        goto out_unlock;

    if (actual_length > 0 &&
        copy_to_user(user_buffer, buffer, actual_length)) {
        ret = -EFAULT;
        goto out_unlock;
    }

    ret = actual_length;

out_unlock:
    mutex_unlock(&dev->io_mutex);
    kfree(buffer);

    return ret;
}

/*
 * User-space write:
 *
 *     write(fd, buffer, size);
 *
 * Data flow:
 *
 * application
 *      |
 * copy_from_user()/memdup_user()
 *      |
 * usb_bulk_msg()
 *      |
 *      | Bulk OUT
 *      v
 * USB device
 */
static ssize_t usb_char_write(struct file *file,
                              const char __user *user_buffer,
                              size_t count,
                              loff_t *ppos)
{
    struct usb_char_device *dev = file->private_data;
    unsigned char *buffer;
    size_t transfer_len;
    int actual_length = 0;
    int ret;

    if (!dev)
        return -ENODEV;

    if (count == 0)
        return 0;

    transfer_len = min_t(size_t, count, USB_MAX_TRANSFER);

    /*
     * Allocate a kernel buffer and copy user data into it.
     */
    buffer = memdup_user(user_buffer, transfer_len);
    if (IS_ERR(buffer))
        return PTR_ERR(buffer);

    mutex_lock(&dev->io_mutex);

    if (dev->disconnected) {
        ret = -ENODEV;
        goto out_unlock;
    }

    /*
     * Synchronous USB bulk-OUT transfer.
     */
    ret = usb_bulk_msg(
        dev->udev,
        usb_sndbulkpipe(dev->udev, dev->bulk_out_ep),
        buffer,
        transfer_len,
        &actual_length,
        USB_IO_TIMEOUT_MS
    );

    if (ret)
        goto out_unlock;

    ret = actual_length;

out_unlock:
    mutex_unlock(&dev->io_mutex);
    kfree(buffer);

    return ret;
}

static const struct file_operations usb_char_fops = {
    .owner   = THIS_MODULE,
    .open    = usb_char_open,
    .release = usb_char_release,
    .read    = usb_char_read,
    .write   = usb_char_write,
};

/*
 * usb_register_dev() uses this structure to create a character
 * device associated with the USB interface.
 *
 * Device names:
 *
 *     /dev/myusb0
 *     /dev/myusb1
 */
static struct usb_class_driver usb_char_class = {
    .name       = "myusb%d",
    .fops       = &usb_char_fops,
    .minor_base = USB_CHAR_MINOR_BASE,
};

/*
 * Called automatically after:
 *
 * 1. The USB device is inserted.
 * 2. USB core enumerates it.
 * 3. VID/PID matches usb_char_id_table.
 */
static int usb_char_probe(struct usb_interface *interface,
                          const struct usb_device_id *id)
{
    struct usb_char_device *dev;
    struct usb_host_interface *alt;
    struct usb_endpoint_descriptor *ep;
    int i;
    int ret;

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    kref_init(&dev->kref);
    mutex_init(&dev->io_mutex);

    /*
     * Obtain a reference to the physical USB device.
     */
    dev->udev = usb_get_dev(interface_to_usbdev(interface));
    dev->interface = interface;

    /*
     * Examine the currently active alternate setting.
     */
    alt = interface->cur_altsetting;

    /*
     * Find the first bulk-IN and first bulk-OUT endpoints.
     */
    for (i = 0; i < alt->desc.bNumEndpoints; i++) {
        ep = &alt->endpoint[i].desc;

        if (!dev->bulk_in_ep &&
            usb_endpoint_is_bulk_in(ep)) {
            dev->bulk_in_ep = ep->bEndpointAddress;
            dev->bulk_in_max_packet = usb_endpoint_maxp(ep);
        }

        if (!dev->bulk_out_ep &&
            usb_endpoint_is_bulk_out(ep)) {
            dev->bulk_out_ep = ep->bEndpointAddress;
            dev->bulk_out_max_packet = usb_endpoint_maxp(ep);
        }
    }

    if (!dev->bulk_in_ep || !dev->bulk_out_ep) {
        dev_err(
            &interface->dev,
            "bulk-IN and bulk-OUT endpoints are required\n"
        );

        ret = -ENODEV;
        goto error_put;
    }

    /*
     * Associate our private structure with this USB interface.
     */
    usb_set_intfdata(interface, dev);

    /*
     * Register a USB character device and create /dev/myusbN.
     */
    ret = usb_register_dev(interface, &usb_char_class);
    if (ret) {
        dev_err(
            &interface->dev,
            "usb_register_dev failed: %d\n",
            ret
        );

        usb_set_intfdata(interface, NULL);
        goto error_put;
    }

    dev_info(
        &interface->dev,
        "connected: VID=%04x PID=%04x, "
        "bulk-in=0x%02x (%zu), "
        "bulk-out=0x%02x (%zu), "
        "node=/dev/myusb%d\n",

        le16_to_cpu(dev->udev->descriptor.idVendor),
        le16_to_cpu(dev->udev->descriptor.idProduct),

        dev->bulk_in_ep,
        dev->bulk_in_max_packet,

        dev->bulk_out_ep,
        dev->bulk_out_max_packet,

        interface->minor - USB_CHAR_MINOR_BASE
    );

    return 0;

error_put:
    kref_put(&dev->kref, usb_char_delete);
    return ret;
}

/*
 * Called automatically when the matching USB interface is removed.
 */
static void usb_char_disconnect(struct usb_interface *interface)
{
    struct usb_char_device *dev;
    int minor;

    dev = usb_get_intfdata(interface);

    /*
     * Prevent future lookups of this private data.
     */
    usb_set_intfdata(interface, NULL);

    /*
     * Save the minor before usb_deregister_dev() changes the
     * interface registration.
     */
    minor = interface->minor;

    /*
     * Remove /dev/myusbN and prevent new opens.
     */
    usb_deregister_dev(interface, &usb_char_class);

    if (!dev)
        return;

    /*
     * Wait for any current synchronous transfer to finish and then
     * mark the device unavailable.
     */
    mutex_lock(&dev->io_mutex);
    dev->disconnected = true;
    mutex_unlock(&dev->io_mutex);

    dev_info(
        &interface->dev,
        "disconnected: /dev/myusb%d removed\n",
        minor - USB_CHAR_MINOR_BASE
    );

    /*
     * Drop the reference created by probe().
     *
     * If an application still has the device open, the structure
     * remains alive until release().
     */
    kref_put(&dev->kref, usb_char_delete);
}

/*
 * USB matching table.
 *
 * The terminating empty entry is required.
 */
static const struct usb_device_id usb_char_id_table[] = {
    {
        USB_DEVICE(
            USB_VENDOR_ID,
            USB_PRODUCT_ID
        )
    },
    { }
};

MODULE_DEVICE_TABLE(usb, usb_char_id_table);

/*
 * Register callbacks with USB core.
 */
static struct usb_driver usb_char_driver = {
    .name       = DRIVER_NAME,
    .probe      = usb_char_probe,
    .disconnect = usb_char_disconnect,
    .id_table   = usb_char_id_table,
};

module_usb_driver(usb_char_driver);

MODULE_AUTHOR("Prashant Soni / TechDhaba");
MODULE_DESCRIPTION(
    "USB bulk character driver with read/write support"
);
MODULE_LICENSE("GPL");

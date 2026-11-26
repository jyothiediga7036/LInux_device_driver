#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

#define USB_VENDOR_ID   0x1234
#define USB_PRODUCT_ID  0x5678

/* File operations (dummy) */
static int usb_open(struct inode *inode, struct file *file)
{
    pr_info("USB Device Opened\n");
    return 0;
}

static int usb_release(struct inode *inode, struct file *file)
{
    pr_info("USB Device Closed\n");
    return 0;
}

static const struct file_operations usb_fops = {
    .owner   = THIS_MODULE,
    .open    = usb_open,
    .release = usb_release,
};

/* USB class driver */
static struct usb_class_driver usb_cd = {
    .name       = "usb/test%d",
    .fops       = &usb_fops,
    .minor_base = 192,
};

/* Probe Function */
static int usb_drv_probe(struct usb_interface *interface,
                         const struct usb_device_id *id)
{
    struct usb_host_interface *iface_desc;
    int ret;

    iface_desc = interface->cur_altsetting;

    pr_info("USB Device Probed\n");
    pr_info("Vendor ID  : %04x\n", id->idVendor);
    pr_info("Product ID : %04x\n", id->idProduct);

    pr_info("Interface Number : %d\n",
            iface_desc->desc.bInterfaceNumber);

    pr_info("Number of Endpoints : %d\n",
            iface_desc->desc.bNumEndpoints);

    pr_info("Interface Class : 0x%02x\n",
            iface_desc->desc.bInterfaceClass);

    ret = usb_register_dev(interface, &usb_cd);
    if (ret) {
        pr_err("Failed to allocate minor number\n");
        return ret;
    }

    pr_info("Minor Number Allocated : %d\n", interface->minor);

    return 0;
}

/* Disconnect Function */
static void usb_drv_disconnect(struct usb_interface *interface)
{
    pr_info("USB Device Disconnected\n");

    usb_deregister_dev(interface, &usb_cd);

    pr_info("Minor Number Released : %d\n", interface->minor);
}

/* Supported USB Devices */
static const struct usb_device_id usb_drv_table[] = {
    {
        USB_DEVICE(USB_VENDOR_ID, USB_PRODUCT_ID)
    },
    { }
};

MODULE_DEVICE_TABLE(usb, usb_drv_table);

/* USB Driver Structure */
static struct usb_driver usb_drv_struct = {
    .name       = "usb_test_driver",
    .probe      = usb_drv_probe,
    .disconnect = usb_drv_disconnect,
    .id_table   = usb_drv_table,
};

/* Module Init */
static int __init usb_test_init(void)
{
    pr_info("Registering USB Driver\n");

    return usb_register(&usb_drv_struct);
}

/* Module Exit */
static void __exit usb_test_exit(void)
{
    pr_info("Deregistering USB Driver\n");

    usb_deregister(&usb_drv_struct);
}

module_init(usb_test_init);
module_exit(usb_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("Simple USB Test Driver");














/*#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/usb.h>

#define USB_VENDOR_ID 0x1234
#define USB_PRODUCT_ID 0x5678

struct usb_class_driver usb_cd;

static int usb_drv_probe(struct usb_interface *interface,cost struct usb_device_id*id)
{
	 struct usb_host_interface *intface_desc;
	struct usb_enpoint_drescriptor *endpoint;
	int ret;
	intface_desc =interface->cur_altsetting;
	pr_info(" usb info %d now probed : %04x :%04)\n",intface_desc->desc.bInterfaceNumber, id->idVendor,id->idProduct);
	pr_info("Id ->bNumEndpoints :%02x\n",intface_desc->descbNumEndpoints);

	pr_info("Id->bIterfaceClass : %02x\n",intface_desc.desc.bInterfaceClass);

	ret= usb_register_dev(interface,&usb_cd);
	if(ret)
	{
		pr_info("not eble to get the minor number\n");
	}
	else
	{
		pr_inf("minor  number =%d\n",interface->minor);
	}	

}

static void usb_drv_disconnect(struct usb_interface *interface)
{
	pr_info("desconnect and release the minor number %d\n",interface->minor);
	usb_deregister_dev(interface,&usb_cd);
}


static struct usb_device_id usb_drv_table[]={
	{
	UAB_DEVICE(USB_VENDOR_ID,USB_PRODUCT_ID)
	},
	{}
};

MODULE_DEVICE_TABLE(usb,usb_drv_table);

static struct usb_driver usb_drv_struct={
	.name="usb_driver",
	.probe=usb_drv_probe,
	.disconnect=usb_drv_disconnec,
	.id_table=usb_drv_table,
};


static int __init usb_test_init(void)
{
	pr_info(" register the usb driver with the usb subsystem\n");
	usb_register(&usb_drv_struct);
}
static void __exit usb_test_exit(void)
{
	pr_info("deregister the usb driver with the usb subsystem\n");
	usb_deregister(&usb_drv_struct);
}

module_init(usb_test_init);
module_exit(usb_test_xit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("JYOTHI");
MODULE_DESCRIPTION("USB test driver");
*/


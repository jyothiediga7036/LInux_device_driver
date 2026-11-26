
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include"common.h"

#define DEVICE_NAME "mycaldev"

static int major;
struct calculator kdata;

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device Opened\n");
    return 0;
}


static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device Closed\n");
    return 0;
}


static ssize_t dev_read(struct file *file,
                        char __user *buf,
                        size_t len,
                        loff_t *offset)
{
    if (copy_to_user(buf, &kdata,sizeof(struct calculator)))
        return -EFAULT;
	
     *offset += sizeof(struct calculator);

    return sizeof(struct calculator);
}


static ssize_t dev_write(struct file *file,
                         const char __user *buf,
                         size_t len,
                         loff_t *offset)
{
   
    if (copy_from_user(&kdata, buf, sizeof(struct calculator)))
        return -EFAULT;
        
    switch(kdata.op)
    {
    	case '+': kdata.result=kdata.a+kdata.b; break;
    	case '-': kdata.result=kdata.a-kdata.b; break;
    	case '*': kdata.result=kdata.a*kdata.b; break;
    	case '/': kdata.result=kdata.a/kdata.b; break;
	default:
            return -EINVAL;
     }
    	
    printk(KERN_INFO "write result =%d\n",kdata.result);

    return sizeof(struct calculator);
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init driver_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);

    if (major < 0)
    {
        printk(KERN_ALERT "Registration Failed\n");
        return major;
    }

    printk(KERN_INFO "Driver Loaded\n");
    printk(KERN_INFO "Major Number = %d\n", major);

    return 0;
}

static void __exit driver_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Driver Unloaded\n");
}

module_init(driver_init);
module_exit(driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jyothi");
MODULE_DESCRIPTION("calculator  Character Device Driver");

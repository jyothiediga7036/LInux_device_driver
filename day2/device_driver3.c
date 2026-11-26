#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024

static int major;
static char device_buffer[BUFFER_SIZE];
static int data_size = 0;

/* Open */
static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device Opened\n");
    return 0;
}

/* Close */
static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device Closed\n");
    return 0;
}

/* Read */
static ssize_t dev_read(struct file *file,
                        char __user *buf,
                        size_t len,
                        loff_t *offset)
{
    int bytes;

    if (*offset >= data_size)
        return 0;

    bytes = min((size_t)(data_size - *offset), len);

    if (copy_to_user(buf, device_buffer + *offset, bytes))
        return -EFAULT;

    *offset += bytes;

    printk(KERN_INFO "Read %d bytes\n", bytes);

    return bytes;
}

/* Write */
static ssize_t dev_write(struct file *file,
                         const char __user *buf,
                         size_t len,
                         loff_t *offset)
{
    int bytes;

    bytes = min((size_t)BUFFER_SIZE, len);

    if (copy_from_user(device_buffer, buf, bytes))
        return -EFAULT;

    data_size = bytes;

    printk(KERN_INFO "Written %d bytes\n", bytes);

    return bytes;
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

/* Module Init */
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
    printk(KERN_INFO "Create Device:\n");
    printk(KERN_INFO "mknod /dev/mychardev c %d 0\n", major);

    return 0;
}

/* Module Exit */
static void __exit driver_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Driver Unloaded\n");
}

module_init(driver_init);
module_exit(driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jyothi);
MODULE_DESCRIPTION("Simple Linux Character Device Driver");

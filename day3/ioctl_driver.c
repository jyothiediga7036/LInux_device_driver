#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/errno.h>

#define DEVICE_NAME "ioctldev"
#define IOCTL_MAGIC 'A'
#define IOCTL_SET_VALUE _IOWR(IOCTL_MAGIC, 1,int)

static int major;
static int kernel_value=0;

static long basic_ioctle(struct file *file,unsigned int cmd, unsigned long arg)
{
	int user_value;
	switch(cmd)
	{
		case IOCTL_SET_VALUE:
					if(copy_from_user(&user_value,(int __user*)arg,sizeof(int)))
					return -EFAULT;
		
					pr_info("kernel : recived %d from user \n",user_value);
					kernel_value=user_value+10;
					
					if(copy_to_user((int __user*)arg,&kernel_value,sizeof(int)))
					return -EFAULT;
		
					break;
		default:
				return -EINVAL;
	}
	return 0;
}
static struct file_operations fops={
	.owner=THIS_MODULE,
	.unlocked_ioctl=basic_ioctle,
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
		

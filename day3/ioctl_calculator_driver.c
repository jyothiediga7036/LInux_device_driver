#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include"common.h"

#define DEVICE_NAME "ioctldev"
#define IOCTL_MAGIC 'A'
#define IOCTL_SET_VALUE _IOWR(IOCTL_MAGIC, 1,int)
#define CALC_ADD  _IOWR('C', 1, struct calculator)
#define CALC_SUB  _IOWR('C', 2, struct calculator)
#define CALC_MUL  _IOWR('C', 3, struct calculator)
#define CALC_DIV  _IOWR('C', 4, struct calculator)


static int major;
static int kernel_value=0;
struct calculator kdata;

static long basic_ioctle(struct file *file,unsigned int cmd, unsigned long arg)
{

	if (copy_from_user(&kdata, (struct kadata __user *)arg,sizeof(kdata)))
    	{
        	return -EFAULT;
    	}

	int user_value;
	switch(cmd)
	{
		case CALC_ADD: kdata.result=kdata.a+kdata.b;break;
		case CALC_SUB: kdata.result=kdata.a-kdata.b;break;
		case CALC_MUL: kdata.result=kdata.a*kdata.b;break;
		case CALC_DIV: kdata.result=kdata.a/kdata.b;break;
		default:
				return -EINVAL;
	}
	if (copy_to_user((struct kdata __user *)arg,&kdata,sizeof(kdata)))
    	{
       		return -EFAULT;
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
		

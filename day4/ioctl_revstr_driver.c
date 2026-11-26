#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/string.h>
#include"struct_string.h"

#define STRING_REV _IOWR('J',1,struct string_data)
#define DEVICE_NAME "mydriver"

static int major;
static struct string_data kdata;

static long my_ioctl(struct file *file,unsigned int cmd, unsigned long arg)
{
	if(copy_from_user(&kdata,(struct string_data __user* )arg, sizeof(kdata)))
	{
		return -EFAULT;
	}
	
	if(cmd!=STRING_REV)
	{
		return -EFAULT;
	}
	
	int i=0;
	int j=strlen(kdata.str)-1;
	for(i=0;i<j;i++,j--)
	{
		char b=kdata.str[i];
		kdata.str[i]=kdata.str[j];
		kdata.str[j]=b;
	}
	if (copy_to_user((struct string_data __user *)arg,&kdata,sizeof(kdata)))
    	{
       		return -EFAULT;
    	}

	return 0;
}

static struct file_operations fops={
	.owner=THIS_MODULE,
	.unlocked_ioctl=my_ioctl,
};


static int __init my_init(void)
{
	major=register_chrdev(0,DEVICE_NAME,&fops);
	if(major<0)
	{
		pr_info("failed\n");
		return major;
	}
	pr_info("ioctl rev driver is loaded\n");
	pr_info("major number is=%d\n",major);
	return 0;
}
static void __exit my_exit(void)
{
	unregister_chrdev(major,DEVICE_NAME);
	pr_info("ioctl rev driver is unloaded\n");
}


module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("simple ioctl revarse function");



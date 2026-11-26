#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/fs.h>
#define DEVICE_NAME "mydevice"
static int major_number;
static int my_open(struct inode *inode,struct file*file)
{
	pr_info("mydevice :Device opend\n");
	return 0;
}
static ssize_t my_read(struct file*file,char __user*buf,size_t count,loff_t *offset)
{
	pr_info("mydevice:read called\n");
	return 0;
}
static ssize_t my_write(struct file*file,const char __user*buf,size_t count,loff_t *offset)
{
	pr_info("mydevice: write called\n");
	return 0;
}
static int my_release(struct inode*inode, struct file*file)
{
	pr_info("mydevice : device closed\n");
	return 0;
}
static const struct file_operations fops=
{
	.owner=THIS_MODULE,
	.open=my_open,
	.read=my_read,
	.write=my_write,
	.release=my_release,
};

static int __init mydevice_init(void)
{
	major_number=register_chrdev(0,DEVICE_NAME,&fops);
	if(major_number<0)
	{
		pr_err("mydevice:failed to register device\n");
		return major_number;
	}
	pr_info("mydevice:Deviceloaded\n");
	pr_info("major number is =%d\n",major_number);
	return 0;
}
static void __exit mydevice_exit(void)
{
	unregister_chrdev(major_number,DEVICE_NAME);
	pr_info("mydevice:Device unloaded\n");
}
module_init(mydevice_init);
module_exit(mydevice_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("basic character device driver");













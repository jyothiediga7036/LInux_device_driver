#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/fs.h>
#define DEVICE_NAME "basic_char"
static int major_number;

static int __init basic_char_init(void)
{
	/*major_number=register_chrdev(0,DEVICE_NAME,NULL);
	if(major_number<0)
	{
		pr_err("filed to register char dev\n");
		return major_number;
	}*/
	pr_info("BASIC char driver loaded\n");
	pr_info("major number allocated:%d\n",major_number);
	return 0;
}
/*static int __init basic_init(void)
{
	printk(KERN_INFO "basic linux device driver init \n");
	return 0;
}*/
static void __exit basic_exit(void)
{
	//unregister_chrdev(major_number,DEVICE_NAME);
	printk(KERN_INFO "basic char  device dribver exit\n");
}
module_init(basic_char_init);
module_exit(basic_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYOTHI");
MODULE_DESCRIPTION("BASIC LINUX KERNEL DRIVER");


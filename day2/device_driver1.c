#include<linux/module.h>
#include<linux/init.h>
#include<linux/kernel.h>
static int __init basic_init(void)
{
	printk(KERN_INFO "basic linux device driver init \n");
	return 0;
}
static void __exit basic_exit(void)
{
	printk(KERN_INFO "basic linux device dribver exit\n");
}
module_init(basic_init);
module_exit(basic_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JYOTHI");
MODULE_DESCRIPTION("BASIC LINUX KERNEL DRIVER");

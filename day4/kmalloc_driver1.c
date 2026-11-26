#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/slab.h>

static char*buffer;
static int __init my_init(void)
{
	buffer=kmalloc(100,GFP_KERNEL);
	pr_info("kmalloc loaded\n");
	if(!buffer)
	{
		pr_err("kmalloc failed\n");
		return -ENOMEM;
	}
	
	buffer[0]='J';
	pr_info("kmalloc sucess  character is %c\n",buffer[0]);
	
	return 0;
}
static void __exit my_exit(void)
{
	kfree(buffer);
	pr_info("kmalloc unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("kmalloc function");


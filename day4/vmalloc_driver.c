#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/slab.h>
#include<linux/vmalloc.h>
static char*buf;

static int __init my_init(void)
{
	buf=vmalloc(100);
	pr_info("vmalloc loaded\n");
	if(!buf)
	{
		pr_err("vmalloc failed\n");
		return -ENOMEM;
	}
	
	buf[0]='V';
	pr_info("vmalloc sucess character is %c\n",buf[0]);
	return 0;
}

static void __exit my_exit(void)
{
	vfree(buf);
	pr_info("vmalloc unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("vmalloc function ");
	

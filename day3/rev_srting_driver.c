#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>


#define DEVICE_NAME "myrevdev"
#define BUFFER_SIZE 1024

static char buffer[BUFFER_SIZE];
static int major_number;
static int data_size = 0;
struct calculator kdata;

static int rev_open(struct inode*inode,struct file*file)
{
	pr_info("rev driver is open \n");
	return 0;
}
static int  rev_release(struct inode*inode,struct file *file)
{
	pr_info("rev driver is closed\n");
	return 0;
}
static ssize_t rev_read(struct file *file,char __user *buf,size_t len,loff_t *offset)
{
    int bytes;

    if (*offset >= data_size)
        return 0;

    bytes = min((size_t)(data_size - *offset), len);

    if (copy_to_user(buf,buffer + *offset, bytes))
        return -EFAULT;

    *offset += bytes;
    printk(KERN_INFO "Read %d bytes\n", bytes);
    

    return bytes;
}
static ssize_t rev_write(struct file *file,
                         const char __user *buf,
                         size_t len,
                         loff_t *offset)
{
    int bytes;

    bytes = min((size_t)BUFFER_SIZE, len);

    if (copy_from_user(buffer, buf, bytes))
        return -EFAULT;

    data_size = bytes;
    int i,j;
    char b;
    j=bytes-1;
    for(i=0;i<j;i++,j--)
    {
    	b=buffer[i];
    	buffer[i]=buffer[j];
    	buffer[j]=b;
    }


    printk(KERN_INFO "Written %d bytes\n", bytes);

    return bytes;
}

static struct file_operations fop={
	.owner=THIS_MODULE,
	.open=rev_open,
	.read=rev_read,
	.write=rev_write,
	.release = rev_release,
};
static int __init my_init(void)
{
	pr_info("my revarse driver is loaded \n");
	major_number=register_chrdev(0,DEVICE_NAME,&fop);
	if(major_number<0)
	{
		pr_info("failed\n");
		return major_number;
	}
	printk(KERN_INFO "Driver Loaded\n");
   	printk(KERN_INFO "Major Number = %d\n", major_number);
	return 0;
}
static void __exit my_exit(void)
{
	unregister_chrdev(major_number, DEVICE_NAME);
	pr_info("my revarse driver is unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("my rev character driver \n");




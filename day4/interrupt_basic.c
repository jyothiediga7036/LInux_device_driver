#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/interrupt.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/errno.h>


#define DEVICE_NAME "irq_demo_driver"
#define IRQ_NUM 1 // ex: keyboard IRQ  on x86

static int irq_counter=0;

// interrupr service routine
// this runs in interrupt context (top half)

static irqreturn_t irq_demo_isr(int irq,void*dev_id)
{
	irq_counter++;
	pr_info("%s: interrupts recevied IRQ=%d count =%d\n",DEVICE_NAME,irq,irq_counter);
	
	return IRQ_HANDLED;
}

static int __init irq_demo_init(void)
{
	int ret;
	pr_info("%s :Initializing \n",DEVICE_NAME);
	
	/*
	request_irq argumwnts:
	irq=irq number
	handler=ISR handler
	flags=IRQF shared allows sharing
	name=visible in /proc/interrupt
	dev_id=unique identifier (must match free_irq)
	*/
	
	ret=request_irq(IRQ_NUM,irq_demo_isr,IRQF_SHARED,
	DEVICE_NAME,(void*)irq_demo_isr);
	if(ret)
	{
		pr_err("%s : Failed to request IRQ %d\n",DEVICE_NAME,IRQ_NUM);
		return ret;
	}
	pr_info("%s: IRQ %d registerd  sucessfully\n",DEVICE_NAME,IRQ_NUM);
	return 0;
}

static void __exit irq_demo_exit(void)
{
	pr_info("%s : cleaning up\n",DEVICE_NAME);
	free_irq(IRQ_NUM,(void*)irq_demo_isr);
	pr_info("%s: IRQ freed\n",DEVICE_NAME);
}
module_init(irq_demo_init);
module_exit(irq_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jyothi");
MODULE_DESCRIPTION("IRQ handler ");
		


	
	
	
	
	
	
	
	

	

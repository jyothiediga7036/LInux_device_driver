#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>

#define GPIO_BASE_PHYS   0xFE200000    // BCM2711 GPIO base
#define GPIO_SIZE        0xB4

#define GPFSEL1_OFFSET   0x04
#define GPSET0_OFFSET    0x1C
#define GPCLR0_OFFSET    0x28

static void __iomem *gpio_base;

static int __init led_init(void)
{
    u32 val;
   g int i;

    pr_info("LED ioremap driver loaded\n");

    gpio_base = ioremap(GPIO_BASE_PHYS, GPIO_SIZE);
    if (!gpio_base) {
        pr_err("ioremap failed\n");
        return -ENOMEM;
    }

    /* Set GPIO17 as OUTPUT */
    val = readl(gpio_base + GPFSEL1_OFFSET);
    val &= ~(7 << 21);      // Clear bits
    val |=  (1 << 21);      // Set output
    writel(val, gpio_base + GPFSEL1_OFFSET);

    /* Blink LED 10 times */
    for (i = 0; i < 10; i++) {
        /* LED ON */
        writel(1 << 17, gpio_base + GPSET0_OFFSET);
        msleep(500);

        /* LED OFF */
        writel(1 << 17, gpio_base + GPCLR0_OFFSET);
        msleep(500);
    }
    return 0;
}

static void __exit led_exit(void)
{
	writel(1<<17,gpio_base+GPCLR0_OFFSET);
	
	if(gpio_base)
		iounmap(gpio_base);
	
	pr_info("LED Ioremap driver unloaded\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("raspberry pi led driver using ioremap only");

  

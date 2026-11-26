#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>

#define GPIO_BASE_PHYS   0xFE200000
#define GPIO_SIZE        0xB4

#define GPFSEL1_OFFSET   0x04
#define GPSET0_OFFSET    0x1C
#define GPCLR0_OFFSET    0x28
#define GPLEV0_OFFSET    0x34

static void __iomem *gpio_base;

static int __init led_init(void)
{
    u32 val;
    u32 status;
    int gpio18_state;

    pr_info("LED driver loaded\n");

    /* Map GPIO registers */
    gpio_base = ioremap(GPIO_BASE_PHYS, GPIO_SIZE);

    if (!gpio_base) {
        pr_err("ioremap failed\n");
        return -ENOMEM;
    }

    /*
     * Configure GPIO17 and GPIO18 as OUTPUT
     *
     * GPIO17 -> GPFSEL1 bits 21-23
     * GPIO18 -> GPFSEL1 bits 24-26
     */

    val = readl(gpio_base + GPFSEL1_OFFSET);

    /* Clear GPIO17 and GPIO18 function bits */
    val &= ~(7 << 21);
    val &= ~(7 << 24);

    /* 001 = OUTPUT */
    val |= (1 << 21);     /* GPIO17 output */
    val |= (1 << 24);     /* GPIO18 output */

    /* Write configuration */
    writel(val, gpio_base + GPFSEL1_OFFSET);


    /* ------------------------------------------------ */
    /* GPIO18 ON                                        */
    /* ------------------------------------------------ */

    writel(1 << 18, gpio_base + GPSET0_OFFSET);

    pr_info("GPIO18 ON\n");

    msleep(100);


    /* Read GPIO18 level */
    status = readl(gpio_base + GPLEV0_OFFSET);

    gpio18_state = (status >> 18) & 1;


    /* If GPIO18 is HIGH, turn GPIO17 ON */
    if (gpio18_state == 1) {

        writel(1 << 17, gpio_base + GPSET0_OFFSET);

        pr_info("GPIO18 HIGH -> GPIO17 ON\n");
    }


    msleep(1000);


    /* ------------------------------------------------ */
    /* GPIO18 OFF                                       */
    /* ------------------------------------------------ */

    writel(1 << 18, gpio_base + GPCLR0_OFFSET);

    pr_info("GPIO18 OFF\n");

    msleep(100);


    /* Read GPIO18 level again */
    status = readl(gpio_base + GPLEV0_OFFSET);

    gpio18_state = (status >> 18) & 1;


    /* If GPIO18 is LOW, turn GPIO17 OFF */
    if (gpio18_state == 0) {

        writel(1 << 17, gpio_base + GPCLR0_OFFSET);

        pr_info("GPIO18 LOW -> GPIO17 OFF\n");
    }

    return 0;
}


static void __exit led_exit(void)
{
    /* Turn both LEDs OFF */

    if (gpio_base) {

        writel(1 << 17, gpio_base + GPCLR0_OFFSET);
        writel(1 << 18, gpio_base + GPCLR0_OFFSET);

        iounmap(gpio_base);
    }

    pr_info("LED driver unloaded\n");
}


module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("GPIO18 controls GPIO17 LED");

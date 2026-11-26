#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define TIMER_INTERVAL_MS 1000   // 1 second

static struct timer_list my_timer;

/* Timer callback function */
static void my_timer_callback(struct timer_list *t)
{
    printk(KERN_INFO "Timer interrupt occurred!\n");

    /* Restart the timer (periodic behavior) */
    mod_timer(&my_timer,
              jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS));
}

/* Module initialization */
static int __init timer_driver_init(void)
{
    printk(KERN_INFO "Timer driver loaded\n");

    /* Initialize timer */
    timer_setup(&my_timer, my_timer_callback, 0);

    /* Start timer */
    mod_timer(&my_timer,
              jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS));

    return 0;
}

/* Module exit */
static void __exit timer_driver_exit(void)
{
    del_timer_sync(&my_timer);

    printk(KERN_INFO "Timer driver unloaded\n");
}

module_init(timer_driver_init);
module_exit(timer_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("Kernel Timer Example");

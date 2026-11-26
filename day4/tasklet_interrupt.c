#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>

#define IRQ_NUMBER 1   /* EXAMPLE ONLY — DO NOT blindly use this */

struct my_ctx {
    struct tasklet_struct tlet;
    unsigned long data;
};

static struct my_ctx ctx;

/* Bottom half */
static void tasklet_fn(struct tasklet_struct *t)
{
    struct my_ctx *c;

    c = container_of(t, struct my_ctx, tlet);

    pr_info("Tasklet executed: data = %lu\n", c->data);
}

/* Top half / ISR */
static irqreturn_t irq_handler(int irq, void *dev_id)
{
    struct my_ctx *c = dev_id;

    c->data++;

    pr_info("ISR executed: IRQ = %d\n", irq);

    tasklet_schedule(&c->tlet);

    return IRQ_HANDLED;
}

/* Module initialization */
static int __init irq_tasklet_init(void)
{
    int ret;

    ctx.data = 0;

    /* Initialize tasklet */
    tasklet_setup(&ctx.tlet, tasklet_fn);

    /*
     * Register interrupt handler.
     *
     * IRQ_NUMBER must be replaced by a REAL IRQ
     * belonging to the device you are driving.
     */
    ret = request_irq(IRQ_NUMBER,
                      irq_handler,
                      0,
                      "my_tasklet_irq",
                      &ctx);

    if (ret) {
        pr_err("request_irq() failed: %d\n", ret);

        tasklet_kill(&ctx.tlet);

        return ret;
    }

    pr_info("IRQ + Tasklet module loaded\n");

    return 0;
}

/* Module cleanup */
static void __exit irq_tasklet_exit(void)
{
    /*
     * Stop/remove IRQ handler first.
     */
    free_irq(IRQ_NUMBER, &ctx);

    /*
     * Wait for tasklet to finish.
     */
    tasklet_kill(&ctx.tlet);

    pr_info("IRQ + Tasklet module unloaded\n");
}

module_init(irq_tasklet_init);
module_exit(irq_tasklet_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("IRQ top half and tasklet bottom half example");

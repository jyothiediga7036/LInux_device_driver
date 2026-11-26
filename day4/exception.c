#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/tracepoint.h>

/*
 * Forward declaration of the tracepoint.
 * Ubuntu headers do not expose trace event headers.
 */
extern struct tracepoint __tracepoint_die;

/*
 * Tracepoint probe
 */
static void my_die_probe(void *ignore, struct pt_regs *regs,const char *str,long err)
{
    pr_alert("DIE tracepoint hit: %s err=%ld\n", str, err);
}

/*
 * Module init
 */
static int __init exception_init(void)
{
    int ret;

    pr_info("Registering die tracepoint probe (Ubuntu-safe)\n");

    ret = tracepoint_probe_register(&__tracepoint_die,
                                    (void *)my_die_probe,
                                    NULL);

    if (ret) {
        pr_err("Failed to register die tracepoint\n");
        return ret;
    }

    /* Safe trigger */
    WARN_ON(1);
    return 0;
}
static void __exit exception_exit(void)
{
	pr_info("unregistering die tracepoint probe\n");
	tracepoint_probe_unregister(&_tracepoint_die,(void*)my_die_prob,NULL);
	
}
module_init(exception_init);
module_exit(exception_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("ubantu 6.8 die tracepoint  example");












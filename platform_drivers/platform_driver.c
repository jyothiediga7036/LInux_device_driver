#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>

static int my_probe(struct platform_device *pdev)
{
    void __iomem *base;
    int irq;

    pr_info("MY DRIVER: probe() called\n");

    /* Get and map the memory resource */
    base = devm_platform_ioremap_resource(pdev, 0);

    if (IS_ERR(base))
        return PTR_ERR(base);

    /* Get the IRQ */
    irq = platform_get_irq(pdev, 0);

    if (irq < 0)
        return irq;

    pr_info("MY DRIVER: Register mapped\n");
    pr_info("MY DRIVER: IRQ = %d\n", irq);

    /* Normally hardware initialization happens here */

    return 0;
}

static int  my_remove(struct platform_device *pdev)
{
    pr_info("MY DRIVER: remove() called\n");
    return 0;

    /* Cleanup hardware/resources here */
}

static const struct of_device_id my_match[] = {
    {
        .compatible = "jyothi,my-device",
    },
    { }
};

MODULE_DEVICE_TABLE(of, my_match);

static struct platform_driver my_driver = {
    .probe  = my_probe,
    .remove = my_remove,

    .driver = {
        .name = "my_platform_driver",
        .of_match_table = my_match,
    },
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("Simple Platform Driver");

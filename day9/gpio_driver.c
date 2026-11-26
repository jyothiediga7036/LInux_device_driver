#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>

struct gpio_led_dev {
    struct gpio_desc *led;
};

static int gpio_led_probe(struct platform_device *pdev)
{
    struct gpio_led_dev *dev;

    pr_info("Probe Called\n");

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(dev->led))
        return PTR_ERR(dev->led);

    pr_info("GPIO acquired successfully\n");

    /* Turn ON LED */
    gpiod_set_value(dev->led, 1);

    platform_set_drvdata(pdev, dev);

    return 0;
}

static int gpio_led_remove(struct platform_device *pdev)
{
    struct gpio_led_dev *dev = platform_get_drvdata(pdev);

    gpiod_set_value(dev->led, 0);

    pr_info("Driver Removed\n");

    return 0;
}

static const struct of_device_id gpio_led_of_match[] = {
    { .compatible = "mycompany,gpio-led" },
    { }
};

MODULE_DEVICE_TABLE(of, gpio_led_of_match);

static struct platform_driver gpio_led_driver = {
    .probe = gpio_led_probe,
    .remove = gpio_led_remove,
    .driver = {
        .name = "gpio_led",
        .of_match_table = gpio_led_of_match,
    },
};

module_platform_driver(gpio_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothi");
MODULE_DESCRIPTION("Simple GPIO Platform Driver");

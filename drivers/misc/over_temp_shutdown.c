#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

static struct gpio_desc *gpio_input;
static struct gpio_desc *gpio_output1;
static struct gpio_desc *gpio_output2;
static int irq_number;

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    gpiod_set_value(gpio_output1, 0);
    gpiod_set_value(gpio_output2, 0);

    printk(KERN_INFO "GPIO Interrupt: Over temp triggered, setting GPIO %d and %d to low to close 310B.\n",
           desc_to_gpio(gpio_output1), desc_to_gpio(gpio_output2));

    return IRQ_HANDLED;
}

static int over_temp_shutdown_probe(struct platform_device *pdev)
{
    int ret;

    gpio_input = devm_gpiod_get_optional(&pdev->dev, "input", GPIOD_IN);
    if (IS_ERR(gpio_input)) {
        printk(KERN_ALERT "Failed to get GPIO input\n");
        return PTR_ERR(gpio_input);
    }

    gpio_output1 = devm_gpiod_get_optional(&pdev->dev, "output1", GPIOD_OUT_HIGH);
    if (IS_ERR(gpio_output1)) {
        printk(KERN_ALERT "Failed to get GPIO output1\n");
        return PTR_ERR(gpio_output1);
    }

    gpio_output2 = devm_gpiod_get_optional(&pdev->dev, "output2", GPIOD_OUT_HIGH);
    if (IS_ERR(gpio_output2)) {
        printk(KERN_ALERT "Failed to get GPIO output2\n");
        return PTR_ERR(gpio_output2);
    }

    if (gpio_input) {
        irq_number = gpiod_to_irq(gpio_input);
        if (irq_number < 0) {
            printk(KERN_ALERT "Failed to get IRQ number for GPIO input\n");
            return irq_number;
        }

        ret = request_irq(irq_number, gpio_irq_handler, IRQF_TRIGGER_RISING, "over_temp_irq_handler", NULL);
        if (ret < 0) {
            printk(KERN_ALERT "Failed to request IRQ %d\n", irq_number);
            return ret;
        }
    }

    return 0;
}

static int over_temp_shutdown_remove(struct platform_device *pdev)
{
    if (gpio_input) {
        free_irq(irq_number, NULL);
    }

    return 0;
}

static const struct of_device_id over_temp_shutdown_of_match[] = {
    { .compatible = "orangepi,over-temp-shutdown", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, over_temp_shutdown_of_match);

static struct platform_driver over_temp_shutdown_driver = {
    .probe = over_temp_shutdown_probe,
    .remove = over_temp_shutdown_remove,
    .driver = {
        .name = "over_temp_shutdown-device",
        .of_match_table = over_temp_shutdown_of_match,
        .owner = THIS_MODULE,
    },
};

module_platform_driver(over_temp_shutdown_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Orange Pi");
MODULE_DESCRIPTION("opiaimax over temp shutdown");

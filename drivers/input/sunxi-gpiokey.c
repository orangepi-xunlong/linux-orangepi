#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/clk.h>
#include <linux/sunxi-gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

static volatile u32 key_val;
static struct gpio_config	power_key_io;
static spinlock_t gk_lock;
static struct input_dev *sunxigk_dev;
static int suspend_flg;

extern int enable_gpio_wakeup_src(int para);

#ifdef CONFIG_PM
static struct dev_pm_domain keyboard_pm_domain;
#endif

static irqreturn_t sunxi_isr_gk(int irq, void *dummy)
{
	unsigned long flags;
	spin_lock_irqsave(&gk_lock, flags);
	if (__gpio_get_value(power_key_io.gpio)) {
		if (suspend_flg) {
			input_report_key(sunxigk_dev, KEY_POWER, 1);
			input_sync(sunxigk_dev);
			suspend_flg = 0;
		}
		input_report_key(sunxigk_dev, KEY_POWER, 0);
		input_sync(sunxigk_dev);
	} else{
		input_report_key(sunxigk_dev, KEY_POWER, 1);
		input_sync(sunxigk_dev);
		suspend_flg = 0;
	}
	spin_unlock_irqrestore(&gk_lock, flags);
	return IRQ_HANDLED;
}

static int sunxi_keyboard_suspend(struct device *dev)
{
	unsigned long flags;
	spin_lock_irqsave(&gk_lock, flags);
	suspend_flg = 1;
	spin_unlock_irqrestore(&gk_lock, flags);
	return 0;
}
static int sunxi_keyboard_resume(struct device *dev)
{

	return 0;
}

static int sunxigk_script_startup(struct device_node *np)
{
	unsigned int gpio = 0;

	gpio = of_get_named_gpio_flags(np, "key_io", 0,
						(enum of_gpio_flags *)&power_key_io);
	if (!gpio_is_valid(gpio)) {
		return -1;
	}

	return 0;
}

static int  sunxigk_probe(struct platform_device *pdev)
{
	int err = 0;
	int irq_number = 0;
	struct device_node *sunxigk_np = NULL;

	sunxigk_np = pdev->dev.of_node;
	if (!sunxigk_np) {
		printk("leds_blink_np is not found");
		goto fail1;
	}

	if (sunxigk_script_startup(sunxigk_np)) {
		err = -EFAULT;
		goto fail1;
	}

	sunxigk_dev = input_allocate_device();
	if (!sunxigk_dev) {
		pr_err("sunxigk: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxigk_dev->name = "sunxi-gpiokey";
	sunxigk_dev->phys = "sunxikbd/input0";
	sunxigk_dev->id.bustype = BUS_HOST;
	sunxigk_dev->id.vendor = 0x0001;
	sunxigk_dev->id.product = 0x0001;
	sunxigk_dev->id.version = 0x0100;

#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	sunxigk_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	printk(KERN_DEBUG "REPORT_REPEAT_KEY_BY_INPUT_CORE is defined, support report repeat key value. \n");
#else
	sunxigk_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif
	set_bit(KEY_POWER, sunxigk_dev->keybit);
	spin_lock_init(&gk_lock);

	irq_number = gpio_to_irq(power_key_io.gpio);

	err = input_register_device(sunxigk_dev);
	if (err)
		goto fail2;
#ifdef CONFIG_PM
	keyboard_pm_domain.ops.suspend = sunxi_keyboard_suspend;
	keyboard_pm_domain.ops.resume = sunxi_keyboard_resume;
	sunxigk_dev->dev.pm_domain = &keyboard_pm_domain;
#endif

	if (devm_request_irq(&sunxigk_dev->dev, irq_number, sunxi_isr_gk,
			       IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND, "gk_EINT", NULL)) {
		err = -EBUSY;
		pr_err(KERN_DEBUG "request irq failure. \n");
		goto fail3;
	}

	enable_gpio_wakeup_src(power_key_io.gpio);
	return 0;

fail3:
	input_unregister_device(sunxigk_dev);
fail2:
	input_free_device(sunxigk_dev);
fail1:
	printk(KERN_DEBUG "sunxikbd_init failed. \n");

	return err;
}

static int sunxigk_remove(struct platform_device *dev)
{
	devm_free_irq(&sunxigk_dev->dev, gpio_to_irq(power_key_io.gpio), NULL);

	input_unregister_device(sunxigk_dev);

	return 0;
}

static const struct of_device_id sunxigk_of_match[] = {
	{.compatible = "allwinner,sunxi-gpio-power-key", .data = NULL},
	{ /* sentinel */ }
};

static struct platform_driver sunxigk_driver = {
	.probe = sunxigk_probe,
	.remove = sunxigk_remove,
	.driver = {
		.name = "sunxi_gpio_power_key",
		.of_match_table = of_match_ptr(sunxigk_of_match),
	},
};

static int __init sunxigk_init(void)
{

	if (platform_driver_register(&sunxigk_driver)) {
		pr_err("gpio user platform_driver_register  fail\n");
		return -1;
	}

	return 0;
}

static void __exit sunxigk_exit(void)
{
	platform_driver_unregister(&sunxigk_driver);
}

module_init(sunxigk_init);
module_exit(sunxigk_exit);

MODULE_AUTHOR("lipeifeng");
MODULE_DESCRIPTION("sunxi gpio power key driver");
MODULE_LICENSE("GPL");



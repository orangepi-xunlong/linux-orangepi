/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/pinctrl/consumer.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <sunxi-gpio.h>

#define DEFAULT_LED_CTRL_GPIO 236 //GPIO PH12
#define DEFAULT_LED_BATTLE_GPIO GPIOL(6)
#define UP_DOWN_TIME  ((50) * (1000))  //1us

#define LED_LEVEL_MAX   16

static struct
{
	int gpio;
	int level;
	int gpio_baffle;
	struct hrtimer hrtimer;
	int virq;
} led_ctrl_data;

#define led_on()    gpio_set_value(led_ctrl_data.gpio, 1)
#define led_off()   gpio_set_value(led_ctrl_data.gpio, 0)

static int interrupt_cnt;
static enum hrtimer_restart led_hrtimer_fn(struct hrtimer *hrtimer)
{
	ktime_t period;

	if (interrupt_cnt > 0) {
		if (interrupt_cnt % 2)
			led_on();
		else
			led_off();

		interrupt_cnt--;
		period = ktime_set(0, UP_DOWN_TIME);
		hrtimer_forward_now(&led_ctrl_data.hrtimer, period);
		return HRTIMER_RESTART;
	} else {
		led_on();
		return HRTIMER_NORESTART;
	}
}

static void led_ctrl_set_level(int level)
{
	led_ctrl_data.level = level;
	if (level == 0) {
		led_off();
		return;
	}
	interrupt_cnt = level * 2 + 1;
	led_on();
	hrtimer_start(&led_ctrl_data.hrtimer, ktime_set(0, 50 * 1000), HRTIMER_MODE_REL);
}

static int led_ctrl_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int led_ctrl_close(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t led_ctrl_read(struct file *filp, char __user *buf,
							size_t count, loff_t *offp)
{
	int ret;
	char buffer[64];

	ret = snprintf(buffer, 64, "%d\n", led_ctrl_data.level);

	return simple_read_from_buffer(buf, count, offp, buffer, ret);

}

static ssize_t led_ctrl_write(struct file *filp, const char __user *buf,
							size_t count, loff_t *offp)
{
	int level;
	char buffer[64];

	if (count >= sizeof(buffer))
		return -1;

	if (copy_from_user(buffer, buf, count))
		return -1;

	buffer[count] = '\0';
	if (kstrtouint(buffer, 10, &level) != 0) {
		printk("kstrtouint error!\n");
		return -1;
	}

	if (level == 0) {
		led_ctrl_set_level(0);
		return count;
	} else if (level > LED_LEVEL_MAX) {
			printk("led leve err:%d use:%d\n", level, LED_LEVEL_MAX);
			level = LED_LEVEL_MAX;
	}

	led_ctrl_set_level(level);
	return count;
}

static struct file_operations led_fops = {
	.owner   = THIS_MODULE,
	.open	 = led_ctrl_open,
	.release = led_ctrl_close,
	.read    = led_ctrl_read,
	.write   = led_ctrl_write,
};

static struct miscdevice led_ctrl_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "led_ctrl",
	.fops	= &led_fops,
};

#if IS_ENABLED(CONFIG_SUNXI_LED_DM)
static irqreturn_t sunxi_gpio_irq_on_handler(int irq, void *dev_id)
{
	int data;
	data = gpio_get_value(led_ctrl_data.gpio_baffle);
	if (data)
		led_off();
	else
		led_on();

	return IRQ_NONE;
}
#endif

static int led_gpio_init(void)
{
	struct device_node *np = NULL;
	struct gpio_config gpio_flags;
	int led_gpio = 0;
#if IS_ENABLED(CONFIG_SUNXI_LED_DM)
	int led_baffle = 0;
#endif
	np = of_find_compatible_node(NULL, NULL, "allwinner,led-level-ctrl");
	led_gpio = of_get_named_gpio_flags(np, "led-gpio", 0,
					(enum of_gpio_flags *)&gpio_flags);
	if (led_gpio > 0) {
			led_ctrl_data.gpio = led_gpio;
	} else {
			led_ctrl_data.gpio = DEFAULT_LED_CTRL_GPIO;
	}
	if (!gpio_is_valid(led_ctrl_data.gpio)) {
		printk("cant find led-gpios \n");
		return -1;
	}
#if IS_ENABLED(CONFIG_SUNXI_LED_DM)
	led_baffle = of_get_named_gpio_flags(np, "led-baffle", 0,
					(enum of_gpio_flags *)&gpio_flags);
	if (led_baffle > 0) {
			led_ctrl_data.gpio_baffle = led_baffle;
	} else {
			led_ctrl_data.gpio_baffle = DEFAULT_LED_BATTLE_GPIO;
	}
	if (!gpio_is_valid(led_ctrl_data.gpio_baffle)) {
		printk("cant find led-baffle\n");
		return -1;
	}
#endif
	gpio_direction_output(led_ctrl_data.gpio, 0);
	led_off();

	return 0;
}

static int led_level_ctrl_probe(struct platform_device *pdev)
{
	int ret;
#if IS_ENABLED(CONFIG_SUNXI_LED_DM)
	struct device *dev = &pdev->dev;
#endif
	ret = led_gpio_init();
	if (ret) {
		printk("gpio request fail \n");
		return -1;
	}

	hrtimer_init(&led_ctrl_data.hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	led_ctrl_data.hrtimer.function = led_hrtimer_fn;

	ret = misc_register(&led_ctrl_dev);
	if (ret != 0) {
		printk("led_level_ctrl init faild \n");
		return -1;
	}
#if IS_ENABLED(CONFIG_SUNXI_LED_DM)
	led_ctrl_data.virq = gpio_to_irq(led_ctrl_data.gpio_baffle);
	ret = devm_request_irq(dev, led_ctrl_data.virq, sunxi_gpio_irq_on_handler, IRQ_TYPE_EDGE_BOTH, "LED_ON", NULL);
	device_init_wakeup(dev, true);
#endif
	return 0;
}

#ifdef CONFIG_PM
static int led_level_suspend(struct device *dev)
{
	led_off();

#if IS_ENABLED(CONFIG_SUNXI_LED_DM)
	if (device_may_wakeup(dev))
		enable_irq_wake(led_ctrl_data.virq);
#endif
	return 0;
}

static int led_level_resume(struct device *dev)
{
#if IS_ENABLED(CONFIG_SUNXI_LED_DM)
	int data;
	data = gpio_get_value(led_ctrl_data.gpio_baffle);
	if (data)
		led_off();
	else
		led_on();

	if (device_may_wakeup(dev))
		disable_irq_wake(led_ctrl_data.virq);
#else
	led_ctrl_set_level(led_ctrl_data.level);
#endif
	return 0;
}
static SIMPLE_DEV_PM_OPS(led_level_pm, led_level_suspend, led_level_resume);
#endif

static int led_level_ctrl_remove(struct platform_device *pdev)
{
	printk("led_level_ctrl: driver exit\n");
	hrtimer_cancel(&led_ctrl_data.hrtimer);

	misc_deregister(&led_ctrl_dev);
	return 0;
}

static const struct of_device_id led_level_ids[] = {
	{ .compatible = "allwinner,led-level-ctrl" },
	{ /* Sentinel */ }
};

static struct platform_driver led_level_driver = {
	.probe	= led_level_ctrl_probe,
	.remove	= led_level_ctrl_remove,
	.driver	= {
			.owner	= THIS_MODULE,
			.name	= "led-level-ctrl",
			.of_match_table	= led_level_ids,
#ifdef CONFIG_PM
			.pm = &led_level_pm,
#endif
			},
};

module_platform_driver(led_level_driver);

MODULE_AUTHOR("K.L");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#include <linux/module.h>
#include <linux/init.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <mach/sys_config.h>
#include <linux/gpio.h>

static const char bt_name[] = "sunxi-bt";
static struct rfkill *sw_rfkill;
static int    bt_used;
static int    bt_rst_n;

extern int sunxi_gpio_req(struct gpio_config *gpio);
#define RF_MSG(...)     do {printk("[rfkill]: "__VA_ARGS__);} while(0)

static int rfkill_set_power(void *data, bool blocked)
{
	RF_MSG("rfkill set power %d\n", blocked);
	if (!blocked) {
		if(bt_rst_n != -1)
			gpio_set_value(bt_rst_n, 1);
	} else {
		if(bt_rst_n != -1)
			gpio_set_value(bt_rst_n, 0);
	}

	msleep(10);
	return 0;
}

static struct rfkill_ops sw_rfkill_ops = {
	.set_block = rfkill_set_power,
};

static int sw_rfkill_probe(struct platform_device *pdev)
{
	int ret = 0;
	sw_rfkill = rfkill_alloc(bt_name, &pdev->dev,
			RFKILL_TYPE_BLUETOOTH, &sw_rfkill_ops, NULL);
	if (unlikely(!sw_rfkill))
		return -ENOMEM;

	rfkill_set_states(sw_rfkill, true, false);

	ret = rfkill_register(sw_rfkill);
	if (unlikely(ret)) {
		rfkill_destroy(sw_rfkill);
	}
	return ret;
}

static int sw_rfkill_remove(struct platform_device *pdev)
{
	if (likely(sw_rfkill)) {
		rfkill_unregister(sw_rfkill);
		rfkill_destroy(sw_rfkill);
	}
	return 0;
}

static struct platform_driver sw_rfkill_driver = {
	.probe = sw_rfkill_probe,
	.remove = sw_rfkill_remove,
	.driver = {
		.name = "sunxi-rfkill",
		.owner = THIS_MODULE,
	},
};

static struct platform_device sw_rfkill_dev = {
	.name = "sunxi-rfkill",
};

static int __init sw_rfkill_init(void)
{
	script_item_value_type_e type;
	script_item_u val;
	struct gpio_config  *gpio_p = NULL;

	type = script_get_item("bt_para", "bt_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		RF_MSG("failed to fetch bt configuration!\n");
		return -1;
	}
	if (!val.val) {
		RF_MSG("init no bt used in configuration\n");
		return 0;
	}
	bt_used = val.val;

	bt_rst_n = -1;
	type = script_get_item("bt_para", "bt_rst_n", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type)
		RF_MSG("mod has no bt_rst_n gpio\n");
	else {
		gpio_p = &val.gpio;
		bt_rst_n = gpio_p->gpio;
		sunxi_gpio_req(gpio_p);
	}

	platform_device_register(&sw_rfkill_dev);
	return platform_driver_register(&sw_rfkill_driver);
}

static void __exit sw_rfkill_exit(void)
{
	if (!bt_used) {
		RF_MSG("exit no bt used in configuration");
		return ;
	}

	platform_driver_unregister(&sw_rfkill_driver);
	platform_device_unregister(&sw_rfkill_dev);
}

late_initcall_sync(sw_rfkill_init);
module_exit(sw_rfkill_exit);

MODULE_DESCRIPTION("sunxi-rfkill driver");
MODULE_AUTHOR("Aaron.magic<mgaic@reuuimllatech.com>");
MODULE_LICENSE(GPL);

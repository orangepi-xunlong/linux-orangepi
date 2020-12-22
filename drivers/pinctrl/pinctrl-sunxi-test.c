/*
 * Allwinner A1X SoCs pinctrl driver.
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/io.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#include <mach/gpio.h>

struct pinctrl *devices_pinctrl[2048];

static int sunxi_devices_pin_request(struct platform_device *pdev)
{
	int              mainkey_count;
	int              mainkey_idx;
	struct pinctrl   *pinctrl;
	
	/* get main key count */
	mainkey_count = script_get_main_key_count();
	pr_warn("mainkey total count : %d\n", mainkey_count);
	for (mainkey_idx = 0; mainkey_idx < mainkey_count; mainkey_idx++) {
		char           *mainkey_name;
		script_item_u  *pin_list;
		int            pin_count;
		char           *device_name;
		script_item_value_type_e type;
		script_item_u   item;
		script_item_u   used;
		
		/* get main key name by index */
		mainkey_name = script_get_main_key_name(mainkey_idx);
		if (!mainkey_name) {
			/* get mainkey name failed */
			pr_warn("get mainkey [%s] name failed\n", mainkey_name);
			continue;
		}
		/* get main-key(device) pin configuration */
		pin_count = script_get_pio_list(mainkey_name, &pin_list);
		pr_warn("mainkey name : %s, pin count : %d\n", mainkey_name, pin_count);
		if (pin_count == 0) {
			/* mainley have no pin configuration */
			continue;
		}
		/* try to get device name */
		type = script_get_item(mainkey_name, "used", &used);
		if ((type != SCIRPT_ITEM_VALUE_TYPE_INT) || (used.val == 0)) {
			/* this device not used */
			continue;
		}
		/* try to get device name */
		type = script_get_item(mainkey_name, "device_name", &item);
		if ((type == SCIRPT_ITEM_VALUE_TYPE_STR) && (item.str)) {
			/* the mainkey have valid device-name,
			 * use the config device_name
			 */
			device_name = item.str;
		} else {
			/* have no device_name config, 
			 * default use mainkey name as device name
			 */
			device_name = mainkey_name;
		}
		/* set device name */
		dev_set_name(&(pdev->dev), device_name);
		
		/* request device pinctrl, set as default state */
		pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
		if (IS_ERR_OR_NULL(pinctrl)) {
			pr_warn("request pinctrl handle for device [%s] failed\n", 
			        dev_name(&pdev->dev));
			return -EINVAL;
		}
		pr_warn("device [%s] pinctrl request succeeded\n", device_name);
		devices_pinctrl[mainkey_idx] = pinctrl;
	}
	return 0;
}

static int sunxi_pin_cfg_check(struct gpio_config *pin_cfg)
{
	int           fail_cnt = 0;
	char          pin_name[SUNXI_PIN_NAME_MAX_LEN];
	unsigned long config;
	
	/* get gpio name */
	sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
	
	/* check function config */
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, pin_name, &config);
	if (pin_cfg->mul_sel != SUNXI_PINCFG_UNPACK_VALUE(config)) {
		fail_cnt++;
		pr_warn("sunxi pin [%s] func config error, [%d != %ld]\n",
		pin_name, pin_cfg->mul_sel, SUNXI_PINCFG_UNPACK_VALUE(config));
	}
	/* check pull config */
	if (pin_cfg->pull != GPIO_PULL_DEFAULT) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0xFFFF);
		pin_config_get(SUNXI_PINCTRL, pin_name, &config);
		if (pin_cfg->pull != SUNXI_PINCFG_UNPACK_VALUE(config)) {
			fail_cnt++;
			pr_warn("sunxi pin [%s] pull config error, [%d != %ld]\n",
			pin_name, pin_cfg->pull, SUNXI_PINCFG_UNPACK_VALUE(config));
		}
	}
	if (pin_cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 0xFFFF);
		pin_config_get(SUNXI_PINCTRL, pin_name, &config);
		if (pin_cfg->pull != SUNXI_PINCFG_UNPACK_VALUE(config)) {
			fail_cnt++;
			pr_warn("sunxi pin [%s] driver level config error, [%d != %ld]\n",
			pin_name, pin_cfg->drv_level, SUNXI_PINCFG_UNPACK_VALUE(config));
		}
	}
	if (pin_cfg->data != GPIO_DATA_DEFAULT) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 0xFFFF);
		pin_config_get(SUNXI_PINCTRL, pin_name, &config);
		if (pin_cfg->data != SUNXI_PINCFG_UNPACK_VALUE(config)) {
			fail_cnt++;
			pr_warn("sunxi pin [%s] data config error, [%d != %ld]\n",
			pin_name, pin_cfg->drv_level, SUNXI_PINCFG_UNPACK_VALUE(config));
		}
	}
	return fail_cnt;
}

static int sunxi_devices_pin_check(struct platform_device *pdev)
{
	int  mainkey_count;
	int  mainkey_idx;
	
	/* get main key count */
	mainkey_count = script_get_main_key_count();
	pr_warn("mainkey total count : %d\n", mainkey_count);
	for (mainkey_idx = 0; mainkey_idx < mainkey_count; mainkey_idx++) {
		char           *mainkey_name;
		script_item_u  *pin_list;
		int             pin_count;
		int 		pin_index;
		char           *device_name;
		script_item_value_type_e type;
		script_item_u   item;
		int             fail_cnt;
		script_item_u   used;
		
		/* get main key name by index */
		mainkey_name = script_get_main_key_name(mainkey_idx);
		if (!mainkey_name) {
			/* get mainkey name failed */
			pr_warn("get mainkey [%s] name failed\n", mainkey_name);
			continue;
		}
		/* get main-key(device) pin configuration */
		pin_count = script_get_pio_list(mainkey_name, &pin_list);
		pr_warn("mainkey name : %s, pin count : %d\n", mainkey_name, pin_count);
		if (pin_count == 0) {
			/* mainley have no pin configuration */
			continue;
		}
		/* try to get device name */
		type = script_get_item(mainkey_name, "used", &used);
		if ((type != SCIRPT_ITEM_VALUE_TYPE_INT) || (used.val == 0)) {
			/* this device not used */
			continue;
		}
		/* try to get device name */
		type = script_get_item(mainkey_name, "device_name", &item);
		if ((type == SCIRPT_ITEM_VALUE_TYPE_STR) && (item.str)) {
			/* the mainkey have valid device-name,
			 * use the config device_name.
			 */
			device_name = item.str;
		} else {
			/* have no device_name config, 
			 * default use mainkey name as device name
			 */
			device_name = mainkey_name;
		}
		fail_cnt = 0;
		for (pin_index = 0; pin_index < pin_count; pin_index++) {
			fail_cnt += sunxi_pin_cfg_check(&(pin_list[pin_index].gpio));
		}
		if (fail_cnt) {
			pr_warn("device [%s] pin check fail cnt %d\n", device_name, fail_cnt);
		} else {
			pr_warn("device [%s] pin check succeeded\n", device_name);
		}
	}
	return 0;
}

static int sunxi_pin_resource_req(struct platform_device *pdev)
{
	script_item_u  *pin_list;
	int            pin_count;
	int            pin_index;
	
	pr_warn("device [%s] pin resource request enter\n", dev_name(&pdev->dev));
	
	/* get pin sys_config info */
	pin_count = script_get_pio_list("lcd0", &pin_list);
	if (pin_count == 0) {
		/* "lcd0" have no pin configuration */
		return -EINVAL;
	}
	/* request pin individually */
	for (pin_index = 0; pin_index < pin_count; pin_index++) {
		struct gpio_config *pin_cfg = &(pin_list[pin_index].gpio);
		char               pin_name[SUNXI_PIN_NAME_MAX_LEN];
		unsigned long      config;
		
		if (IS_AXP_PIN(pin_cfg->gpio)) {
			/* valid pin of axp-pinctrl, 
			 * config pin attributes individually.
			 */
			sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
			pin_config_set(AXP_PINCTRL, pin_name, config);
			if (pin_cfg->data != GPIO_DATA_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg->data);
				pin_config_set(AXP_PINCTRL, pin_name, config);
			}
		} else {
			/* valid pin of sunxi-pinctrl, 
			 * config pin attributes individually.
			 */
			sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
			if (pin_cfg->pull != GPIO_PULL_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pin_cfg->pull);
				pin_config_set(SUNXI_PINCTRL, pin_name, config);
			}
			if (pin_cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, pin_cfg->drv_level);
				pin_config_set(SUNXI_PINCTRL, pin_name, config);
			}
			if (pin_cfg->data != GPIO_DATA_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg->data);
				pin_config_set(SUNXI_PINCTRL, pin_name, config);
			}
		}
	}
	pr_debug("device [%s] pin resource request ok\n", dev_name(&pdev->dev));
	return 0;
}


static irqreturn_t sunxi_gpio_irq_test_handler(int irq, void *dev_id)
{
	pr_warn("sunxi gpio irq coming\n");
	return IRQ_HANDLED;
}

static int sunxi_gpio_eint_test(struct platform_device *pdev)
{
	struct device  *dev   = &pdev->dev;
	int            virq;
	int            ret;
	
	/* map the virq of gpio */
	virq = gpio_to_irq(GPIOA(0));
	if (IS_ERR_VALUE(virq)) {
		pr_warn("map gpio [%d] to virq failed, errno = %d\n", 
		         GPIOA(0), virq);
		return -EINVAL;
	}
	pr_warn("gpio [%d] map to virq [%d] ok\n", GPIOA(0), virq);
	
	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(dev, virq, sunxi_gpio_irq_test_handler, 
			       IRQF_TRIGGER_HIGH, "PA0_EINT", NULL);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("request virq %d failed, errno = %d\n", 
		         virq, ret);
		return -EINVAL;
	}
	/* enbale virq */
	//enable_irq(virq);
	
	return 0;
}

static int sunxi_gpio_test(struct platform_device *pdev)
{
	int ret;
	ret = gpio_request(GPIO_AXP(0), "axp_gpio");
	if (IS_ERR_VALUE(ret)) {
		pr_warn("request axp gpio failed, errno %d\n", ret);
		return -EINVAL;
	}
	ret = gpio_direction_input(GPIO_AXP(0));
	if (IS_ERR_VALUE(ret)) {
		pr_warn("set axp gpio direction input failed, errno %d\n", ret);
		return -EINVAL;
	}
	
	ret = gpio_request(GPIOA(0), "sunxi_gpio");
	if (IS_ERR_VALUE(ret)) {
		pr_warn("request sunxi gpio failed, errno %d\n", ret);
		return -EINVAL;
	}
	ret = gpio_direction_input(GPIOA(0));
	if (IS_ERR_VALUE(ret)) {
		pr_warn("set sunxi gpio direction input failed, errno %d\n", ret);
		return -EINVAL;
	}
	return 0;
}

static int sunxi_pinctrl_test_probe(struct platform_device *pdev)
{
	struct pinctrl       *pinctrl;
	
	pr_warn("device [%s] probe enter\n", dev_name(&pdev->dev));
	
	/* set device name */
	dev_set_name(&(pdev->dev), "i2s1");
	
	/* request device pinctrl, set as default state */
	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR_OR_NULL(pinctrl)) {
		pr_warn("request pinctrl handle for device [%s] failed\n", 
		        dev_name(&pdev->dev));
		return -EINVAL;
	}
	//sunxi_devices_pin_request(pdev);
	
	//sunxi_devices_pin_check(pdev);
	
	pr_warn("sunxi-pinctrl test ok\n");
	return 0;
}

static struct platform_driver sunxi_pinctrl_test_driver = {
	.probe = sunxi_pinctrl_test_probe,
	.driver = {
		.name = "lcd0",
		.owner = THIS_MODULE,
	},
};

static struct platform_device sunxi_pinctrl_test_device = {
	.name = "lcd0",
	.id = PLATFORM_DEVID_NONE, /* this is only one device for pinctrl driver */
};

static int __init sunxi_pinctrl_test_init(void)
{
	int ret;
	ret = platform_device_register(&sunxi_pinctrl_test_device);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("register sunxi pinctrl platform device failed\n");
		return -EINVAL;
	}
	ret = platform_driver_register(&sunxi_pinctrl_test_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("register sunxi pinctrl platform driver failed\n");
		return -EINVAL;
	}
	return 0;
}
late_initcall(sunxi_pinctrl_test_init);

MODULE_AUTHOR("Sunny <sunny@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner A1X pinctrl driver test");
MODULE_LICENSE("GPL");

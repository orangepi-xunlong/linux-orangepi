#include <linux/io.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sys_config.h>
#include <linux/slab.h>
#include <linux/mfd/core.h>
#include <linux/seq_file.h>
#include <linux/i2c.h>
#include "../../../pinctrl/core.h"
#include "../axp-core.h"
#include "../axp-gpio.h"
#include "axp20-gpio.h"

static int axp20_gpio_get_data(struct axp_dev *axp_dev, int gpio)
{
	u8 ret;
	struct axp_regmap *map;

	map = axp_dev->regmap;
	if (NULL == map) {
		pr_err("%s: axp regmap is null\n", __func__);
		return -ENXIO;
	}

	switch (gpio) {
	case 0:
		axp_regmap_read(map, AXP_GPIO_STATE, &ret); ret &= 0x10; break;
	case 1:
		axp_regmap_read(map, AXP_GPIO_STATE, &ret); ret &= 0x20; break;
	case 2:
		axp_regmap_read(map, AXP_GPIO_STATE, &ret); ret &= 0x40; break;
	default:
		pr_err("This IO is not an input\n");
		return -ENXIO;
	}

	return ret;
}

static int axp20_gpio_set_data(struct axp_dev *axp_dev, int gpio, int value)
{
	struct axp_regmap *map;

	if ((0 <= gpio) && (2 >= gpio)) {
		map = axp_dev->regmap;
		if (NULL == map) {
			pr_err("%s: %d axp regmap is null\n",
					__func__, __LINE__);
			return -ENXIO;
		}
	} else {
		return -ENXIO;
	}

	if (value) {
		/* high */
		switch (gpio) {
		case 0:
			return axp_regmap_update_sync(map,
				AXP_GPIO0_CFG, 0x01, 0x07);
		case 1:
			return axp_regmap_update_sync(map,
				AXP_GPIO1_CFG, 0x01, 0x07);
		case 2:
			return -ENXIO;
		default:
			break;
		}
	} else {
		/* low */
		switch (gpio) {
		case 0:
			return axp_regmap_update_sync(map,
				AXP_GPIO0_CFG, 0x0, 0x07);
		case 1:
			return axp_regmap_update_sync(map,
				AXP_GPIO1_CFG, 0x0, 0x07);
		case 2:
			return axp_regmap_update_sync(map,
				AXP_GPIO2_CFG, 0x0, 0x07);
		default:
			break;
		}
	}

	return -ENXIO;
}

static int axp20_pmx_set(struct axp_dev *axp_dev, int gpio, int mux)
{
	struct axp_regmap *map;

	map = axp_dev->regmap;
	if (NULL == map) {
		pr_err("%s: %d axp regmap is null\n", __func__, __LINE__);
		return -ENXIO;
	}

	if (mux == 1) {
		/* output */
		switch (gpio) {
		case 0:
			return axp_regmap_clr_bits_sync(map,
				AXP_GPIO0_CFG, 0x06);
		case 1:
			return axp_regmap_clr_bits_sync(map,
				AXP_GPIO1_CFG, 0x06);
		case 2:
			return axp_regmap_clr_bits_sync(map,
				AXP_GPIO2_CFG, 0x07);
		default:
			return -ENXIO;
		}
	} else if (mux == 0) {
		/* input */
		switch (gpio) {
		case 0:
			axp_regmap_clr_bits_sync(map,
				AXP_GPIO0_CFG, 0x07);
			return axp_regmap_set_bits_sync(map,
				AXP_GPIO0_CFG, 0x02);
		case 1:
			axp_regmap_clr_bits_sync(map,
				AXP_GPIO1_CFG, 0x07);
			return axp_regmap_set_bits_sync(map,
				AXP_GPIO1_CFG, 0x02);
		case 2:
			axp_regmap_clr_bits_sync(map,
				AXP_GPIO2_CFG, 0x07);
			return axp_regmap_set_bits_sync(map,
				AXP_GPIO2_CFG, 0x02);
		default:
			pr_err("This IO can not config as input!");
			return -ENXIO;
		}
	}

	return -ENXIO;
}

static int axp20_pmx_get(struct axp_dev *axp_dev, int gpio)
{
	u8 ret;
	struct axp_regmap *map;

	map = axp_dev->regmap;
	if (NULL == map) {
		pr_err("%s: %d axp regmap is null\n",
				__func__, __LINE__);
		return -ENXIO;
	}

	switch (gpio) {
	case 0:
		axp_regmap_read(map, AXP_GPIO0_CFG, &ret); break;
	case 1:
		axp_regmap_read(map, AXP_GPIO1_CFG, &ret); break;
	case 2:
		axp_regmap_read(map, AXP_GPIO2_CFG, &ret); break;
	default:
		return -ENXIO;
	}

	if (0 == (ret & 0x06))
		return 1;
	else if (0x02 == (ret & 0x07))
		return 0;
	else
		return -ENXIO;
}

static const struct axp_desc_pin axp20_pins[] = {
	AXP_PIN_DESC(AXP_PINCTRL_GPIO(0),
			 AXP_FUNCTION(0x0, "gpio_in"),
			 AXP_FUNCTION(0x1, "gpio_out")),
	AXP_PIN_DESC(AXP_PINCTRL_GPIO(1),
			 AXP_FUNCTION(0x0, "gpio_in"),
			 AXP_FUNCTION(0x1, "gpio_out")),
	AXP_PIN_DESC(AXP_PINCTRL_GPIO(2),
			 AXP_FUNCTION(0x0, "gpio_in"),
			 AXP_FUNCTION(0x1, "gpio_out")),
};

static struct axp_pinctrl_desc axp20_pinctrl_pins_desc = {
	.pins  = axp20_pins,
	.npins = ARRAY_SIZE(axp20_pins),
};

static struct axp_gpio_ops axp20_gpio_ops = {
	.gpio_get_data = axp20_gpio_get_data,
	.gpio_set_data = axp20_gpio_set_data,
	.pmx_set = axp20_pmx_set,
	.pmx_get = axp20_pmx_get,
};

static int axp20_gpio_probe(struct platform_device *pdev)
{

	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct axp_pinctrl *axp20_pin;

	axp20_pin = axp_pinctrl_register(&pdev->dev,
			axp_dev, &axp20_pinctrl_pins_desc, &axp20_gpio_ops);
	if (IS_ERR_OR_NULL(axp20_pin))
		return -1;

	platform_set_drvdata(pdev, axp20_pin);

	return 0;
}

static int axp20_gpio_remove(struct platform_device *pdev)
{
	struct axp_pinctrl *axp20_pin = platform_get_drvdata(pdev);

	return axp_pinctrl_unregister(axp20_pin);
}

static const struct of_device_id axp20_gpio_dt_ids[] = {
	{ .compatible = "axp209-gpio", },
	{},
};
MODULE_DEVICE_TABLE(of, axp20_gpio_dt_ids);

static struct platform_driver axp20_gpio_driver = {
	.driver = {
		.name = "axp20-gpio",
		.of_match_table = axp20_gpio_dt_ids,
	},
	.probe  = axp20_gpio_probe,
	.remove = axp20_gpio_remove,
};

static int __init axp20_gpio_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp20_gpio_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
fs_initcall(axp20_gpio_initcall);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gpio Driver for axp20x");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");

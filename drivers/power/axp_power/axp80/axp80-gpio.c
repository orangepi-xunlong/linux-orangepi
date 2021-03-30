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
#include "../../../pinctrl/core.h"
#include "../axp-core.h"
#include "../axp-gpio.h"
#include "axp80-gpio.h"

static int axp80_gpio_get_data(struct axp_dev *axp_dev, int gpio)
{
	if (0 == gpio) {
		pr_err("This IO is not an input\n");
		return -ENXIO;
	} else {
		return -ENXIO;
	}
}

static int axp80_gpio_set_data(struct axp_dev *axp_dev, int gpio, int value)
{
	struct axp_regmap *map;

	if (0 == gpio) {
		map = axp_dev->regmap;
		if (NULL == map) {
			pr_err("%s: %d axp regmap is null\n",
					__func__, __LINE__);
			return -ENXIO;
		}
	} else {
		return -ENXIO;
	}

	/* high */
	if (value)
		return axp_regmap_update_sync(map, AXP_GPIO0_CFG, 0x03, 0x07);
	else
		return axp_regmap_update_sync(map, AXP_GPIO0_CFG, 0x02, 0x07);
}

static int axp80_pmx_set(struct axp_dev *axp_dev, int gpio, int mux)
{
	struct axp_regmap *map;

	if (0 == gpio) {
		map = axp_dev->regmap;
		if (NULL == map) {
			pr_err("%s: %d axp regmap is null\n",
					__func__, __LINE__);
			return -ENXIO;
		}
	} else {
		return -ENXIO;
	}

	/* output */
	if (mux == 1)
		return axp_regmap_update_sync(map, AXP_GPIO0_CFG, 0x02, 0x07);
	else
		return -ENXIO;
}

static int axp80_pmx_get(struct axp_dev *axp_dev, int gpio)
{
	u8 ret;
	struct axp_regmap *map;

	if (0 == gpio) {
		map = axp_dev->regmap;
		if (NULL == map) {
			pr_err("%s: %d axp regmap is null\n",
					__func__, __LINE__);
			return -ENXIO;
		}
	} else {
		return -ENXIO;
	}

	axp_regmap_read(map, AXP_GPIO0_CFG, &ret);
	if (0x2 == (ret & 0x06))
		return 1;
	else
		return -ENXIO;
}

static const struct axp_desc_pin axp80_pins[] = {
	AXP_PIN_DESC(AXP_PINCTRL_GPIO(0),
			 AXP_FUNCTION(0x0, "gpio_in"),
			 AXP_FUNCTION(0x1, "gpio_out")),
};

static struct axp_pinctrl_desc axp80_pinctrl_pins_desc = {
	.pins  = axp80_pins,
	.npins = ARRAY_SIZE(axp80_pins),
};

static struct axp_gpio_ops axp80_gpio_ops = {
	.gpio_get_data = axp80_gpio_get_data,
	.gpio_set_data = axp80_gpio_set_data,
	.pmx_set = axp80_pmx_set,
	.pmx_get = axp80_pmx_get,
};

static int axp80_gpio_probe(struct platform_device *pdev)
{

	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct axp_pinctrl *axp80_pin;

	axp80_pin = axp_pinctrl_register(&pdev->dev,
			axp_dev, &axp80_pinctrl_pins_desc, &axp80_gpio_ops);
	if (IS_ERR_OR_NULL(axp80_pin))
		return -1;

	platform_set_drvdata(pdev, axp80_pin);

	return 0;
}

static int axp80_gpio_remove(struct platform_device *pdev)
{
	struct axp_pinctrl *axp80_pin = platform_get_drvdata(pdev);

	return axp_pinctrl_unregister(axp80_pin);
}

static const struct of_device_id axp80_gpio_dt_ids[] = {
	{ .compatible = "axp806-gpio", },
	{},
};
MODULE_DEVICE_TABLE(of, axp80_gpio_dt_ids);

static struct platform_driver axp80_gpio_driver = {
	.driver = {
		.name = "axp80-gpio",
		.of_match_table = axp80_gpio_dt_ids,
	},
	.probe  = axp80_gpio_probe,
	.remove = axp80_gpio_remove,
};

static int __init axp80_gpio_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp80_gpio_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
fs_initcall(axp80_gpio_initcall);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gpio Driver for axp80x");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");

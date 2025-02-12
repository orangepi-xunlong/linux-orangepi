// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Pinctrl driver for Ky PMIC
 *
 * Copyright (c) 2023, KY Co., Ltd
 *
 */

#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/gpio/driver.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/mfd/ky/ky_pmic.h>

#include "core.h"
#include "pinctrl-utils.h"
#include "pinmux.h"

SPM8821_PINMUX_DESC;
SPM8821_PINFUNC_DESC;
SPM8821_PIN_CINFIG_DESC;
SPM8821_PINCTRL_MATCH_DATA;

struct ky_pctl {
	struct gpio_chip	chip;
	struct regmap		*regmap;
	struct pinctrl_dev	*pctldev;
	struct device		*dev;
	struct pinctrl_desc pinctrl_desc;
	int funcdesc_nums, confdesc_nums;
	const struct pin_func_desc *func_desc;
	const struct pin_config_desc *config_desc;
	const char *name;
};

static const struct pinctrl_ops ky_gpio_pinctrl_ops = {
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_group,
	.dt_free_map = pinconf_generic_dt_free_map,
};

static int ky_gpio_pinmux_set(struct pinctrl_dev *pctldev,
			      unsigned int function, unsigned int group)
{
	int i, ret;
	struct ky_pctl *pctl = pinctrl_dev_get_drvdata(pctldev);
	const char *funcname = pinmux_generic_get_function_name(pctldev, function);

	/* get the target desc */
	for (i = 0; i < pctl->funcdesc_nums; ++i) {
		if (strcmp(funcname, pctl->func_desc[i].name) == 0 && group ==
				pctl->func_desc[i].pin_id) {
			/* set the first */
			ret = regmap_update_bits(pctl->regmap,
					pctl->func_desc[i].func_reg,
					pctl->func_desc[i].func_mask,
					pctl->func_desc[i].en_val
					<< (ffs(pctl->func_desc[i].func_mask) - 1));
			if (ret) {
				dev_err(pctl->dev, "set PIN%d, function:%s, failed\n", group, funcname);
				return ret;
			}

			/* set the next if it have */
			if (pctl->func_desc[i].ha_sub) {
				ret = regmap_update_bits(pctl->regmap,
					pctl->func_desc[i].sub_reg,
					pctl->func_desc[i].sub_mask,
					pctl->func_desc[i].sube_val
					<< (ffs(pctl->func_desc[i].sub_mask) - 1));
				if (ret) {
					dev_err(pctl->dev, "set PIN%d, function:%s, failed\n", group, funcname);
					return ret;
				}
			}

			break;
		}
	}

	if (i >= pctl->funcdesc_nums) {
		dev_err(pctl->dev, "Unsupported PIN%d, function:%s\n", group, funcname);
		return -EINVAL;
	}

	return 0;
}

static int ky_pmx_gpio_set_direction(struct pinctrl_dev *pctldev,
					 struct pinctrl_gpio_range *range,
					 unsigned int offset, bool input)
{
	int ret;
	struct ky_pctl *pctl = pinctrl_dev_get_drvdata(pctldev);

	if (strcmp(pctl->name, "spm8821") == 0)
		/* when input == true, it means that we should set this pin
		 * as gpioin, so we should pass function(0) to set_mux
		 */
		ret = ky_gpio_pinmux_set(pctldev, !input, offset);
	else
		return -EINVAL;

	return ret;
}

static const struct pinmux_ops ky_gpio_pinmux_ops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.set_mux = ky_gpio_pinmux_set,
	.gpio_set_direction = ky_pmx_gpio_set_direction,
	.strict = true,
};

static int ky_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	int ret;
	unsigned int val;
	struct ky_pctl *pctl = gpiochip_get_data(chip);

	ret = regmap_read(pctl->regmap, pctl->config_desc[offset].input.reg, &val);
	if (ret) {
		dev_err(pctl->dev, "get PIN%d, direction failed\n", offset);
		return ret;
	}

	val = val & pctl->config_desc[offset].input.msk;
	val >>= ffs(pctl->config_desc[offset].input.msk) - 1;

	return val;
}

static int ky_gpio_get_direction(struct gpio_chip *chip,
				     unsigned int offset)
{
	int i, ret;
	unsigned int val, direction = 0;
	struct ky_pctl *pctl = gpiochip_get_data(chip);

	/* read the function set register */
	for (i = 0; i < pctl->funcdesc_nums; ++i) {
		if (offset == pctl->func_desc[i].pin_id) {
			ret = regmap_read(pctl->regmap, pctl->func_desc[i].func_reg, &val);
			if (ret) {
				dev_err(pctl->dev, "get PIN%d, direction failed\n", offset);
				return ret;
			}

			direction = val & pctl->func_desc[i].func_mask;
			direction >>= ffs(pctl->func_desc[i].func_mask) - 1;

			break;
		}
	}

	if (strcmp(pctl->name, "spm8821") == 0)
		return !direction;
	else
		return -EINVAL;
}

static void ky_gpio_set(struct gpio_chip *chip, unsigned int offset,
			    int value)
{
	int ret;
	struct ky_pctl *pctl = gpiochip_get_data(chip);

	ret = regmap_update_bits(pctl->regmap,
			pctl->config_desc[offset].output.reg,
			pctl->config_desc[offset].output.msk,
			value ? pctl->config_desc[offset].output.msk : 0);
	if (ret)
		dev_err(pctl->dev, "set PIN%d, val:%d, failed\n", offset, value);
}

static int ky_gpio_input(struct gpio_chip *chip, unsigned int offset)
{
	/* set the gpio input */
	return pinctrl_gpio_direction_input(chip->base + offset);
}

static int ky_gpio_output(struct gpio_chip *chip, unsigned int offset,
			      int value)
{
	/* set the gpio output */
	return pinctrl_gpio_direction_input(chip->base + offset);
}

static int ky_pin_conf_get(struct pinctrl_dev *pctldev, unsigned int pin,
			       unsigned long *config)
{
	/* Do nothing by now */
	return 0;
}

static int ky_pin_conf_set(struct pinctrl_dev *pctldev, unsigned int pin,
			       unsigned long *configs, unsigned int num_configs)
{
	unsigned int reg, msk, ret;
	struct ky_pctl *pctl = pinctrl_dev_get_drvdata(pctldev);

	while (num_configs) {
		switch (pinconf_to_config_param(*configs)) {
		case PIN_CONFIG_BIAS_DISABLE:
		case PIN_CONFIG_BIAS_PULL_DOWN:
		case PIN_CONFIG_BIAS_PULL_UP:
			reg = pctl->config_desc[pin].pup.reg;
			msk = pctl->config_desc[pin].pup.msk;
			break;
		case PIN_CONFIG_DRIVE_OPEN_DRAIN:
		case PIN_CONFIG_DRIVE_PUSH_PULL:
		case PIN_CONFIG_DRIVE_OPEN_SOURCE:
			reg = pctl->config_desc[pin].od.reg;
			msk = pctl->config_desc[pin].od.msk;
			break;
		case PIN_CONFIG_INPUT_DEBOUNCE:
			reg = pctl->config_desc[pin].deb.reg;
			msk = pctl->config_desc[pin].deb.timemsk;
			break;
		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			reg = pctl->config_desc[pin].deb.reg;
			msk = pctl->config_desc[pin].deb.en.msk;
			break;
		case PIN_CONFIG_OUTPUT:
			reg = pctl->config_desc[pin].output.reg;
			msk = pctl->config_desc[pin].output.msk;
			break;
		default:
			return -ENOTSUPP;
		}

		ret = regmap_update_bits(pctl->regmap, reg, msk,
				pinconf_to_config_argument(*configs)
				<< (ffs(msk) - 1));
		if (ret) {
			dev_err(pctl->dev, "set reg:%x, msk:%x failed\n", reg, msk);
			return -EINVAL;
		}
		++configs;
		--num_configs;
	}

	return 0;
}

static int ky_pconf_group_set(struct pinctrl_dev *pctldev, unsigned group,
				 unsigned long *configs, unsigned num_configs)
{
	return ky_pin_conf_set(pctldev, group, configs, num_configs);
}

static int ky_pconf_group_get(struct pinctrl_dev *pctldev,
				 unsigned group,
				 unsigned long *config)
{
	return ky_pin_conf_get(pctldev, group, config);
}

static const struct pinconf_ops ky_gpio_pinconf_ops = {
	.is_generic = true,
	.pin_config_get = ky_pin_conf_get,
	.pin_config_set = ky_pin_conf_set,
	.pin_config_group_get	= ky_pconf_group_get,
	.pin_config_group_set	= ky_pconf_group_set,
};

static const struct of_device_id ky_pmic_pinctrl_of_match[] = {
	{ .compatible = "pmic,pinctrl,spm8821", .data = (void *)&spm8821_pinctrl_match_data },
	{ },
};
MODULE_DEVICE_TABLE(of, ky_pmic_pinctrl_of_match);

static int ky_pmic_pinctrl_probe(struct platform_device *pdev)
{
	int i, res;
	struct ky_pctl *pctl;
	unsigned int npins;
	const char **pin_names;
	unsigned int *pin_nums;
	struct pinctrl_pin_desc *pins;
	const struct of_device_id *of_id;
	struct ky_pmic *pmic = dev_get_drvdata(pdev->dev.parent);
	struct pinctrl_match_data *match_data;

	of_id = of_match_device(ky_pmic_pinctrl_of_match, &pdev->dev);
	if (!of_id) {
		pr_err("Unable to match OF ID\n");
		return -ENODEV;
	}

	match_data = (struct pinctrl_match_data *)of_id->data;

	pctl = devm_kzalloc(&pdev->dev, sizeof(*pctl), GFP_KERNEL);
	if (!pctl)
		return -ENOMEM;

	pctl->name = match_data->name;
	pctl->dev = &pdev->dev;
	pctl->regmap = pmic->regmap;
	pctl->func_desc = match_data->pinfunc_desc;
	pctl->funcdesc_nums = match_data->nr_pin_fuc_desc;
	pctl->config_desc = match_data->pinconf_desc;
	pctl->confdesc_nums = match_data->nr_pin_conf_desc;
	dev_set_drvdata(&pdev->dev, pctl);

	if (of_property_read_u32(pdev->dev.of_node, "ky,npins", &npins))
		return dev_err_probe(&pdev->dev, -EINVAL,
				     "ky,npins property not found\n");

	pins = devm_kmalloc_array(&pdev->dev, npins, sizeof(pins[0]),
				  GFP_KERNEL);
	pin_names = devm_kmalloc_array(&pdev->dev, npins, sizeof(pin_names[0]),
				       GFP_KERNEL);
	pin_nums = devm_kmalloc_array(&pdev->dev, npins, sizeof(pin_nums[0]),
				      GFP_KERNEL);
	if (!pins || !pin_names || !pin_nums)
		return -ENOMEM;

	for (i = 0; i < npins; i++) {
		pins[i].number = i;
		pins[i].name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "PIN%u", i);
		pins[i].drv_data = pctl;
		pin_names[i] = pins[i].name;
		pin_nums[i] = i;
	}

	pctl->pinctrl_desc.name = dev_name(pctl->dev);
	pctl->pinctrl_desc.pins = pins;
	pctl->pinctrl_desc.npins = npins;
	pctl->pinctrl_desc.pctlops = &ky_gpio_pinctrl_ops;
	pctl->pinctrl_desc.pmxops = &ky_gpio_pinmux_ops;
	pctl->pinctrl_desc.confops = &ky_gpio_pinconf_ops;

	pctl->pctldev =	devm_pinctrl_register(&pdev->dev, &pctl->pinctrl_desc, pctl);
	if (IS_ERR(pctl->pctldev))
		return dev_err_probe(&pdev->dev, PTR_ERR(pctl->pctldev),
				     "Failed to register pinctrl device.\n");

	for (i = 0; i < npins; i++) {
		res = pinctrl_generic_add_group(pctl->pctldev, pins[i].name,
						pin_nums + i, 1, pctl);
		if (res < 0)
			return dev_err_probe(pctl->dev, res,
					     "Failed to register group");
	}

	for (i = 0; i < match_data->nr_pin_mux; ++i) {
		res = pinmux_generic_add_function(pctl->pctldev, match_data->pinmux_funcs[i],
						  pin_names, npins, pctl);
		if (res < 0)
			return dev_err_probe(pctl->dev, res,
					     "Failed to register function.");
	}

	pctl->chip.base			= -1;
	pctl->chip.can_sleep		= true;
	pctl->chip.request		= gpiochip_generic_request;
	pctl->chip.free			= gpiochip_generic_free;
	pctl->chip.parent		= &pdev->dev;
	pctl->chip.label		= dev_name(&pdev->dev);
	pctl->chip.owner		= THIS_MODULE;
	pctl->chip.get			= ky_gpio_get;
	pctl->chip.get_direction	= ky_gpio_get_direction;
	pctl->chip.set			= ky_gpio_set;
	pctl->chip.direction_input	= ky_gpio_input;
	pctl->chip.direction_output	= ky_gpio_output;

	pctl->chip.ngpio = pctl->pinctrl_desc.npins;

	res = devm_gpiochip_add_data(&pdev->dev, &pctl->chip, pctl);
	if (res) {
		dev_err(&pdev->dev, "Failed to register GPIO chip\n");
		return res;
	}

	return 0;
}

static struct platform_driver ky_pmic_pinctrl_driver = {
	.probe = ky_pmic_pinctrl_probe,
	.driver = {
		.name = "ky-pmic-pinctrl",
		.of_match_table = ky_pmic_pinctrl_of_match,
	},
};
module_platform_driver(ky_pmic_pinctrl_driver);

MODULE_LICENSE("GPL v2");

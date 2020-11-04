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
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/sunxi-gpio.h>
#include <asm/uaccess.h>
#include <linux/arisc/arisc.h>

#include "../core.h"
#include "../pinconf.h"
#include "../../gpio/gpiolib.h"
#include "pinctrl-sunxi.h"

static struct irq_chip sunxi_pinctrl_edge_irq_chip;
static struct irq_chip sunxi_pinctrl_level_irq_chip;

static struct sunxi_pinctrl_group *
sunxi_pinctrl_find_group_by_name(struct sunxi_pinctrl *pctl, const char *group)
{
	int i;

	for (i = 0; i < pctl->ngroups; i++) {
		struct sunxi_pinctrl_group *grp = pctl->groups + i;

		if (!strcmp(grp->name, group))
			return grp;
	}

	return NULL;
}

static struct sunxi_pinctrl_function *
sunxi_pinctrl_find_function_by_name(struct sunxi_pinctrl *pctl,
				    const char *name)
{
	struct sunxi_pinctrl_function *func = pctl->functions;
	int i;

	for (i = 0; i < pctl->nfunctions; i++) {
		if (!func[i].name)
			break;

		if (!strcmp(func[i].name, name))
			return func + i;
	}

	return NULL;
}

static struct sunxi_desc_function *
sunxi_pinctrl_desc_find_function_by_mulval(struct sunxi_pinctrl *pctl,
					 const char *pin_name,
					 const u32 mul_sel)
{
	int i;

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;

		if (!strcmp(pin->pin.name, pin_name)) {
			struct sunxi_desc_function *func = pin->functions;

			while (func->name) {
				if (func->muxval == mul_sel)
					return func;

				func++;
			}
		}
	}

	return NULL;
}

static struct sunxi_desc_function *
sunxi_pinctrl_desc_find_function_by_name(struct sunxi_pinctrl *pctl,
					 const char *pin_name,
					 const char *func_name)
{
	int i;

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;

		if (!strcmp(pin->pin.name, pin_name)) {
			struct sunxi_desc_function *func = pin->functions;

			while (func->name) {
				if (!strcmp(func->name, func_name))
					return func;

				func++;
			}
		}
	}

	return NULL;
}

static struct sunxi_desc_function *
sunxi_pinctrl_desc_find_function_by_pin(struct sunxi_pinctrl *pctl,
					const u16 pin_num,
					const char *func_name)
{
	int i;

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;

		if (pin->pin.number == pin_num) {
			struct sunxi_desc_function *func = pin->functions;

			while (func->name) {
				if (!strcmp(func->name, func_name))
					return func;

				func++;
			}
		}
	}

	return NULL;
}

static int sunxi_pctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->ngroups;
}

static const char *sunxi_pctrl_get_group_name(struct pinctrl_dev *pctldev,
					      unsigned group)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->groups[group].name;
}

static int sunxi_pctrl_get_group_pins(struct pinctrl_dev *pctldev,
				      unsigned group,
				      const unsigned **pins,
				      unsigned *num_pins)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	*pins = (unsigned *)&pctl->groups[group].pin;
	*num_pins = 1;

	return 0;
}

static bool sunxi_pctrl_has_bias_prop(struct device_node *node)
{
	return	of_find_property(node, "allwinner,pull", NULL);
}

static bool sunxi_pctrl_has_drive_prop(struct device_node *node)
{
	return of_find_property(node, "allwinner,drive", NULL);
}

static bool sunxi_pctrl_has_data_prop(struct device_node *node)
{
	return of_find_property(node, "allwinner,data", NULL);
}

static int sunxi_pctrl_parse_bias_prop(struct device_node *node)
{
	u32 val;

	/* And fall back to the old binding */
	if (of_property_read_u32(node, "allwinner,pull", &val))
		return -EINVAL;

	switch (val) {
	case SUNXI_PINCTRL_NO_PULL:
		return PIN_CONFIG_BIAS_DISABLE;
	case SUNXI_PINCTRL_PULL_UP:
		return PIN_CONFIG_BIAS_PULL_UP;
	case SUNXI_PINCTRL_PULL_DOWN:
		return PIN_CONFIG_BIAS_PULL_DOWN;
	case SYSCFG_PROP_DEFAULT_VAL:
		return val;
	}

	return -EINVAL;
}

static int sunxi_pctrl_parse_data_prop(struct device_node *node)
{
	u32 val;

	if (of_property_read_u32(node, "allwinner,data", &val))
		return -EINVAL;

	switch (val) {
	case 0:
	case 1:
	case SYSCFG_PROP_DEFAULT_VAL:
		return val;
	}

	return -EINVAL;
}

static int sunxi_pctrl_parse_drive_prop(struct device_node *node)
{
	u32 val;

	/* Try the new style binding */
	if (!of_property_read_u32(node, "drive-strength", &val)) {
		/* We can't go below 10mA ... */
		if (val < 10)
			return -EINVAL;

		/* ... and only up to 40 mA ... */
		if (val > 40)
			val = 40;

		/* by steps of 10 mA */
		return rounddown(val, 10);
	}

	/* And then fall back to the old binding */
	if (of_property_read_u32(node, "allwinner,drive", &val))
		return -EINVAL;
	switch (val) {
	case 0:
	case 1:
	case 2:
	case 3:
		return (val + 1) * 10;
	case SYSCFG_PROP_DEFAULT_VAL:
		return SYSCFG_PROP_DEFAULT_VAL;
	}

	return -EINVAL;

}

static const char *sunxi_pctrl_parse_function_prop(struct device_node *node)
{
	const char *function;
	int ret;

	/* Try the generic binding */
	ret = of_property_read_string(node, "function", &function);
	if (!ret)
		return function;

	/* And fall back to our legacy one */
	ret = of_property_read_string(node, "allwinner,function", &function);
	if (!ret)
		return function;

	return NULL;
}

static int sunxi_pctrl_parse_muxsel_prop(struct device_node *node,
								u32 *muxsel)
{
	int ret;

	ret = of_property_read_u32(node, "muxsel", muxsel);
	if (!ret)
		return ret;

	return of_property_read_u32(node, "allwinner,muxsel", muxsel) ;
}

static const char *sunxi_pctrl_find_pins_prop(struct device_node *node,
					      int *npins)
{
	int count;

	/* Try the generic binding */
	count = of_property_count_strings(node, "pins");
	if (count > 0) {
		*npins = count;
		return "pins";
	}

	/* And fall back to our legacy one */
	count = of_property_count_strings(node, "allwinner,pins");
	if (count > 0) {
		*npins = count;
		return "allwinner,pins";
	}

	return NULL;
}


static unsigned long *sunxi_pctrl_build_pin_config(struct device_node *node,
						   unsigned int *len)
{
	unsigned long *pinconfig;
	unsigned int configlen = 0, idx = 0;
	int ret;

	if (sunxi_pctrl_has_drive_prop(node))
		configlen++;
	if (sunxi_pctrl_has_bias_prop(node))
		configlen++;
	if (sunxi_pctrl_has_data_prop(node))
		configlen++;

	pinconfig = kcalloc(configlen, sizeof(*pinconfig), GFP_KERNEL);
	/*
	 * If we don't have any configuration, bail out
	 */
	if (!configlen)
		return NULL;

	pinconfig = kzalloc(configlen * sizeof(*pinconfig), GFP_KERNEL);
	if (!pinconfig)
		return ERR_PTR(-ENOMEM);

	if (sunxi_pctrl_has_drive_prop(node)) {
		int drive = sunxi_pctrl_parse_drive_prop(node);

		if (drive == -EINVAL) {
			pr_err("pinctrl-%s: parse drive prop failed!\n",
								node->name);
			pr_err("pinctrl-%s: parse drive prop failed!\n", node->name);
			ret = drive;
			goto err_free;
		}

		if (drive != SYSCFG_PROP_DEFAULT_VAL)
			pinconfig[idx++] = pinconf_to_config_packed(
					PIN_CONFIG_DRIVE_STRENGTH, drive);
	}

	if (sunxi_pctrl_has_data_prop(node)) {
		int data = sunxi_pctrl_parse_data_prop(node);

		if (data == -EINVAL) {
			pr_err("pinctrl-%s: parse data prop failed!\n",
								node->name);
			ret = data;
			goto err_free;
		}

		if (data != SYSCFG_PROP_DEFAULT_VAL)
			pinconfig[idx++] =
			pinconf_to_config_packed(PIN_CONFIG_OUTPUT, data);
	}

	if (sunxi_pctrl_has_bias_prop(node)) {
		int pull = sunxi_pctrl_parse_bias_prop(node);

		if (pull == -EINVAL) {
			pr_err("pinctrl-%s: parse bias prop failed!\n",
								node->name);
			ret = pull;
			goto err_free;
		}

		if (pull != SYSCFG_PROP_DEFAULT_VAL)
			pinconfig[idx++] = pinconf_to_config_packed(pull, 0);
	}

	*len = configlen;
	return pinconfig;

err_free:
	kfree(pinconfig);
	return ERR_PTR(ret);
}

static int sunxi_pctrl_dt_node_to_map(struct pinctrl_dev *pctldev,
				      struct device_node *node,
				      struct pinctrl_map **map,
				      unsigned *num_maps)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	unsigned long *pinconfig;
	struct property *prop;
	struct sunxi_desc_function *func;
	const char *expect_func, *pin_prop;
	const char *group;
	int ret, npins, nmaps, configlen = 0, i = 0;
	u32 muxsel;

	*map = NULL;
	*num_maps = 0;

	ret = sunxi_pctrl_parse_muxsel_prop(node, &muxsel);
	if (ret) {
		dev_err(pctl->dev,
			"missing allwinner,muxsel property in node %s\n",
			node->name);
		return -EINVAL;
	}

	expect_func = sunxi_pctrl_parse_function_prop(node);
	if (!expect_func) {
		dev_err(pctl->dev, "missing expect_func property in node %s\n",
			node->name);
		return -EINVAL;
	}


	pin_prop = sunxi_pctrl_find_pins_prop(node, &npins);
	if (!pin_prop) {
		dev_err(pctl->dev, "missing pins property in node %s\n",
			node->name);
		return -EINVAL;
	}

	/*
	 * We have two maps for each pin: one for the function, one
	 * for the configuration (bias, strength, etc).
	 *
	 * We might be slightly overshooting, since we might not have
	 * any configuration.
	 */
	nmaps = npins * 2;
	*map = kmalloc(nmaps * sizeof(struct pinctrl_map), GFP_KERNEL);
	if (!*map)
		return -ENOMEM;

	/*try the allwinner specific devicetree configuration first*/
	pinconfig = sunxi_pctrl_build_pin_config(node, &configlen);
	if (IS_ERR(pinconfig)) {
		ret = PTR_ERR(pinconfig);
		goto err_free_map;
	}
	/*then try the generic devicetree configuration */
	if (!pinconfig) {
		ret = pinconf_generic_parse_dt_config(node, pctldev, &pinconfig,
						&configlen);
		if (ret)
			return ret;
	}

	of_property_for_each_string(node, pin_prop, prop, group) {
		struct sunxi_pinctrl_group *grp =
			sunxi_pinctrl_find_group_by_name(pctl, group);

		if (!grp) {
			dev_err(pctl->dev, "unknown pin %s", group);
			continue;
		}
		func = sunxi_pinctrl_desc_find_function_by_mulval(pctl,
							grp->name,
							muxsel);
		if (!func) {
			dev_err(pctl->dev, "can not get function on pin %s",
				group);
			continue;
		}
		/*ignore io_disabled func name*/
		if (strcmp(func->name, expect_func) &&
			 (strcmp(func->name, "io_disabled")))
			dev_warn(pctl->dev,
			"expect_func as:%s, but muxsel(%d) is func:%s\n",
			expect_func, muxsel, func->name);

		(*map)[i].type = PIN_MAP_TYPE_MUX_GROUP;
		(*map)[i].data.mux.group = group;
		(*map)[i].data.mux.function = func->name;

		i++;

		if (pinconfig) {
			(*map)[i].type = PIN_MAP_TYPE_CONFIGS_GROUP;
			(*map)[i].data.configs.group_or_pin = group;
			(*map)[i].data.configs.configs = pinconfig;
			(*map)[i].data.configs.num_configs = configlen;
			i++;
		}
	}

	*num_maps = i;

	/*
	 * We know have the number of maps we need, we can resize our
	 * map array
	 */
	*map = krealloc(*map, i * sizeof(struct pinctrl_map), GFP_KERNEL);
	if (!map)
		return -ENOMEM;

	return 0;

err_free_map:
	kfree(*map);
	*map = NULL;
	return ret;
}

static void sunxi_pctrl_dt_free_map(struct pinctrl_dev *pctldev,
				    struct pinctrl_map *map,
				    unsigned num_maps)
{
	int i;

	/* pin config is never in the first map */
	for (i = 1; i < num_maps; i++) {
		if (map[i].type != PIN_MAP_TYPE_CONFIGS_GROUP)
			continue;

		/*
		 * All the maps share the same pin config,
		 * free only the first one we find.
		 */
		kfree(map[i].data.configs.configs);
		break;
	}


	kfree(map);
}

static const struct pinctrl_ops sunxi_pctrl_ops = {
	.dt_node_to_map		= sunxi_pctrl_dt_node_to_map,
	.dt_free_map		= sunxi_pctrl_dt_free_map,
	.get_groups_count	= sunxi_pctrl_get_groups_count,
	.get_group_name		= sunxi_pctrl_get_group_name,
	.get_group_pins		= sunxi_pctrl_get_group_pins,
};

static int sunxi_pmx_get_funcs_cnt(struct pinctrl_dev *pctldev)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->nfunctions;
}

static const char *sunxi_pmx_get_func_name(struct pinctrl_dev *pctldev,
					   unsigned function)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->functions[function].name;
}

static int sunxi_pmx_get_func_groups(struct pinctrl_dev *pctldev,
				     unsigned function,
				     const char * const **groups,
				     unsigned * const num_groups)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	*groups = pctl->functions[function].groups;
	*num_groups = pctl->functions[function].ngroups;

	return 0;
}

static void sunxi_pmx_set(struct pinctrl_dev *pctldev,
				 unsigned pin,
				 u8 config)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	unsigned long flags;
	u32 val, mask;

	raw_spin_lock_irqsave(&pctl->lock, flags);

	pin -= pctl->desc->pin_base;
	val = readl(pctl->membase + sunxi_mux_reg(pin));
	mask = MUX_PINS_MASK << sunxi_mux_offset(pin);
	writel((val & ~mask) | config << sunxi_mux_offset(pin),
		pctl->membase + sunxi_mux_reg(pin));

	raw_spin_unlock_irqrestore(&pctl->lock, flags);
}

static int sunxi_pmx_set_mux(struct pinctrl_dev *pctldev,
			     unsigned function,
			     unsigned group)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_pinctrl_group *g = pctl->groups + group;
	struct sunxi_pinctrl_function *func = pctl->functions + function;
	struct sunxi_desc_function *desc =
		sunxi_pinctrl_desc_find_function_by_name(pctl,
							 g->name,
							 func->name);

	if (!desc)
		return -EINVAL;

	sunxi_pmx_set(pctldev, g->pin, desc->muxval);

	return 0;
}

static int
sunxi_pmx_gpio_set_direction(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range,
			unsigned offset,
			bool input)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_desc_function *desc;
	const char *func;

	if (input)
		func = "gpio_in";
	else
		func = "gpio_out";

	desc = sunxi_pinctrl_desc_find_function_by_pin(pctl, offset, func);
	if (!desc)
		return -EINVAL;

	sunxi_pmx_set(pctldev, offset, desc->muxval);

	return 0;
}

static void sunxi_pmx_gpio_disable_free(struct pinctrl_dev *pctldev,
				struct pinctrl_gpio_range *range,
				unsigned offset)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_desc_function *desc;

	desc = sunxi_pinctrl_desc_find_function_by_pin(pctl, offset, "io_disabled");
	if (!desc)
		return;

	sunxi_pmx_set(pctldev, offset, desc->muxval);
}

static const struct pinmux_ops sunxi_pmx_ops = {
	.get_functions_count	= sunxi_pmx_get_funcs_cnt,
	.get_function_name	= sunxi_pmx_get_func_name,
	.get_function_groups	= sunxi_pmx_get_func_groups,
	.set_mux		= sunxi_pmx_set_mux,
	.gpio_set_direction	= sunxi_pmx_gpio_set_direction,
	.gpio_disable_free	= sunxi_pmx_gpio_disable_free,
};

static int sunxi_pconf_reg(unsigned pin, enum pin_config_param param,
			   u32 *offset, u32 *shift, u32 *mask)
{
	switch (param) {
	case PIN_CONFIG_DRIVE_STRENGTH:
	case SUNXI_PINCFG_TYPE_DRV:
		*offset = sunxi_dlevel_reg(pin);
		*shift = sunxi_dlevel_offset(pin);
		*mask = DLEVEL_PINS_MASK;
		break;

	case PIN_CONFIG_BIAS_PULL_UP:
	case PIN_CONFIG_BIAS_PULL_DOWN:
	case PIN_CONFIG_BIAS_DISABLE:
	case SUNXI_PINCFG_TYPE_PUD:
		*offset = sunxi_pull_reg(pin);
		*shift = sunxi_pull_offset(pin);
		*mask = PULL_PINS_MASK;
		break;
	case SUNXI_PINCFG_TYPE_DAT:
	case PIN_CONFIG_OUTPUT:
		*offset = sunxi_data_reg(pin);
		*shift = sunxi_data_offset(pin);
		*mask = DATA_PINS_MASK;
		break;
	case SUNXI_PINCFG_TYPE_FUNC:
	case PIN_CONFIG_INPUT_ENABLE:
		*offset = sunxi_mux_reg(pin);
		*shift = sunxi_mux_offset(pin);
		*mask = MUX_PINS_MASK;
		break;
	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int sunxi_pconf_get(struct pinctrl_dev *pctldev,
			     unsigned pin,
			     unsigned long *config)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	enum pin_config_param param = pinconf_to_config_param(*config);
	unsigned long flags;
	u32 offset, shift, mask, val;
	u16 arg;
	int ret = 0;

	pin -= pctl->desc->pin_base;

	raw_spin_lock_irqsave(&pctl->lock, flags);

	ret = sunxi_pconf_reg(pin, param, &offset, &shift, &mask);
	if (ret < 0) {
		raw_spin_unlock_irqrestore(&pctl->lock, flags);
		return ret;
	}

	val = (readl(pctl->membase + offset) >> shift) & mask;

	switch (param) {
	/* sunxi platform specified config type */
	case SUNXI_PINCFG_TYPE_DRV:
	case SUNXI_PINCFG_TYPE_PUD:
	case SUNXI_PINCFG_TYPE_DAT:
	case SUNXI_PINCFG_TYPE_FUNC:
		arg = val;
		break;
	/* pinctrl generic config type */
	case PIN_CONFIG_DRIVE_STRENGTH:
		arg = (val + 1) * 10;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		if (val != SUNXI_PINCTRL_PULL_UP)
			ret = -EINVAL;
		arg = 1; /* hardware is weak pull-up */
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		if (val != SUNXI_PINCTRL_PULL_DOWN)
			ret = -EINVAL;
		arg = 1; /* hardware is weak pull-down */
		break;

	case PIN_CONFIG_BIAS_DISABLE:
		if (val != SUNXI_PINCTRL_NO_PULL)
			ret = -EINVAL;
		arg = 0;
		break;
	default:
		pr_debug("invalid sunxi pconf type for get\n");
		ret = -EINVAL;
	}

	if (!ret)
		*config = pinconf_to_config_packed(param, arg);

	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return ret;
}

static int sunxi_pconf_set(struct pinctrl_dev *pctldev,
			     unsigned pin,
			     unsigned long *pin_config,
			     unsigned num_configs)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	unsigned long config = (unsigned long)pin_config;
	unsigned long flags;
	u32 offset, shift, mask, reg;
	u32 arg, val;
	int ret;
	enum pin_config_param param;

	pin = pin - pctl->desc->pin_base;

	param = pinconf_to_config_param(config);
	arg = pinconf_to_config_argument(config);
	ret = sunxi_pconf_reg(pin, param, &offset, &shift, &mask);
	if (ret < 0)
		return 0;
	switch (param) {
	case SUNXI_PINCFG_TYPE_DRV:
		val = arg;
		break;
	case SUNXI_PINCFG_TYPE_PUD:
		val = arg;
		break;

	case SUNXI_PINCFG_TYPE_DAT:
		val = arg;
		break;
	case SUNXI_PINCFG_TYPE_FUNC:
		val = arg;
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		if (arg < 10 || arg > 40)
			return -EINVAL;
		/*
		 * We convert from mA to what the register expects:
		 *   0: 10mA
		 *   1: 20mA
		 *   2: 30mA
		 *   3: 40mA
		 */
		val = arg / 10 - 1;
		break;
	case PIN_CONFIG_BIAS_DISABLE:
		val = 0;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		val = 1;
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		val = 2;
		break;
	case PIN_CONFIG_INPUT_ENABLE:
		/*input muxsel is 0, output is 1*/
		val = !arg;
		break;
	case PIN_CONFIG_OUTPUT:
		val = arg;
		break;
	default:
		pr_debug("invalid sunxi pconf type for set\n");
		return -EINVAL;
	}
	raw_spin_lock_irqsave(&pctl->lock, flags);
	reg = readl(pctl->membase + offset);
	reg &= ~(mask << shift);
	writel(reg | val << shift, pctl->membase + offset);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int sunxi_pconf_group_get(struct pinctrl_dev *pctldev,
				 unsigned group,
				 unsigned long *config)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_pinctrl_group *g = &pctl->groups[group];
	/* We only support 1 pin per group. Chain it to the pin callback */
	return sunxi_pconf_get(pctldev, g->pin, config);
}

static int sunxi_pconf_group_set(struct pinctrl_dev *pctldev,
				 unsigned group,
				 unsigned long *configs,
				 unsigned num_configs)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_pinctrl_group *g = &pctl->groups[group];
	unsigned pin = g->pin - pctl->desc->pin_base;
	int i;

	for (i = 0; i < num_configs; i++) {
		enum pin_config_param param;
		unsigned long flags;
		u32 offset, shift, mask, reg;
		u32 arg, val;
		int ret;

		param = pinconf_to_config_param(configs[i]);
		arg = pinconf_to_config_argument(configs[i]);

		ret = sunxi_pconf_reg(pin, param, &offset, &shift, &mask);
		/*In sunxi platform, if the pin property is set 'default' by
		 * sys_config,the param and arg will be zero, and then
		 * sunxi_pconf_reg() will return -ENOTSUPP, we didn't handle
		 * this fault before we abondon the old configurate ways by
		 * sys_config, so return  0 now*/
		if (ret < 0)
			return 0;

		switch (param) {
		case PIN_CONFIG_DRIVE_STRENGTH:
			if (arg < 10 || arg > 40)
				return -EINVAL;
			/*
			 * We convert from mA to what the register expects:
			 *   0: 10mA
			 *   1: 20mA
			 *   2: 30mA
			 *   3: 40mA
			 */
			val = arg / 10 - 1;
			break;
		case PIN_CONFIG_BIAS_DISABLE:
			val = 0;
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			val = 1;
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			val = 2;
			break;
		case PIN_CONFIG_INPUT_ENABLE:
			sunxi_pmx_gpio_set_direction(pctldev, NULL, pin,
								true);
			return 0;
		case PIN_CONFIG_OUTPUT:
			sunxi_pmx_gpio_set_direction(pctldev, NULL, pin,
								false);
			val = arg;
			break;
		default:
			/* sunxi_pconf_reg should catch anything unsupported */
			WARN_ON(1);
			return -ENOTSUPP;
		}
		/* cache the config value */
		g->config = configs[i];
		raw_spin_lock_irqsave(&pctl->lock, flags);
		reg = readl(pctl->membase + offset);
		reg &= ~(mask << shift);
		writel(reg | val << shift, pctl->membase + offset);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);
	} /* for each config */

	return 0;
}

static const struct pinconf_ops sunxi_pconf_ops = {
	.is_generic		= true,
	.pin_config_get	= sunxi_pconf_get,
	.pin_config_set	= sunxi_pconf_set,
	.pin_config_group_get	= sunxi_pconf_group_get,
	.pin_config_group_set	= sunxi_pconf_group_set,
};

static int sunxi_pinctrl_gpio_direction_input(struct gpio_chip *chip,
					unsigned offset)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(chip->parent);
	u32 pin = pctl->desc->pin_base + offset;

	sunxi_pmx_set(pctl->pctl_dev, pin, SUN4I_FUNC_INPUT);

	return 0;
}

static int sunxi_pinctrl_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(chip->parent);
	u32 reg = sunxi_data_reg(offset);
	u8 index = sunxi_data_offset(offset);
	u32 set_mux = pctl->desc->irq_read_needs_mux &&
			test_bit(FLAG_USED_AS_IRQ, &chip->gpiodev->descs[offset].flags);
	u32 val;

	if (set_mux)
		sunxi_pmx_set(pctl->pctl_dev, offset, SUN4I_FUNC_INPUT);

	val = (readl(pctl->membase + reg) >> index) & DATA_PINS_MASK;

	if (set_mux)
		sunxi_pmx_set(pctl->pctl_dev, offset, SUN4I_FUNC_IRQ);

	return val;
}

static void sunxi_pinctrl_gpio_set(struct gpio_chip *chip,
				unsigned offset, int value)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(chip->parent);
	u32 reg = sunxi_data_reg(offset);
	u8 index = sunxi_data_offset(offset);
	unsigned long flags;
	u32 regval;

	raw_spin_lock_irqsave(&pctl->lock, flags);

	regval = readl(pctl->membase + reg);

	if (value)
		regval |= BIT(index);
	else
		regval &= ~(BIT(index));

	writel(regval, pctl->membase + reg);

	raw_spin_unlock_irqrestore(&pctl->lock, flags);
}

static int sunxi_pinctrl_gpio_direction_output(struct gpio_chip *chip,
					unsigned offset, int value)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(chip->parent);
	u32 pin = pctl->desc->pin_base + offset;

	sunxi_pinctrl_gpio_set(chip, offset, value);
	sunxi_pmx_set(pctl->pctl_dev, pin, SUN4I_FUNC_OUTPUT);

	return 0;
}

static int sunxi_pinctrl_gpio_set_debounce(struct gpio_chip *chip,
					unsigned offset, unsigned value)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(chip->parent);
	unsigned pinnum = pctl->desc->pin_base + offset;
	unsigned bank_base;
	struct sunxi_desc_function *desc;
	u32 reg, reg_val;
	unsigned int val_clk_per_scale;
	unsigned int val_clk_select;
	unsigned long flags;

	if (offset >= chip->ngpio)
		return -ENXIO;

	desc = sunxi_pinctrl_desc_find_function_by_pin(pctl, pinnum, "irq");
	if (!desc)
		return -EINVAL;

	bank_base = pctl->desc->irq_bank_base[desc->irqbank];
	reg = sunxi_irq_debounce_reg_from_bank(desc->irqbank, bank_base);

	raw_spin_lock_irqsave(&pctl->lock, flags);
	reg_val = readl(pctl->membase + reg);
	val_clk_select = value & 1;
	val_clk_per_scale = (value >> 4) & 0x07;

	/*set debounce pio interrupt clock select */
	reg_val &= ~(1 << 0);
	reg_val |= val_clk_select;

	/* set debounce clock pre scale */
	reg_val &= ~(7 << 4);
	reg_val |= val_clk_per_scale << 4;
	writel(reg_val, pctl->membase + reg);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int sunxi_pinctrl_gpio_of_xlate(struct gpio_chip *gc,
				const struct of_phandle_args *gpiospec,
				u32 *flags)
{
	struct gpio_config *config;
	int pin, base;

	base = PINS_PER_BANK * gpiospec->args[0];
	pin = base + gpiospec->args[1];
	pin = pin - gc->base;
	if (pin > gc->ngpio)
		return -EINVAL;

	if (flags) {
		config = (struct gpio_config *)flags;
		config->gpio = base + gpiospec->args[1];
		config->mul_sel = gpiospec->args[2];
		config->pull = gpiospec->args[3];
		config->drv_level = gpiospec->args[4];
		config->data = gpiospec->args[5];
	}

	return pin;
}

static int sunxi_pinctrl_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(chip->parent);
	struct sunxi_desc_function *desc;
	unsigned pinnum = pctl->desc->pin_base + offset;
	unsigned irqnum;

	if (offset >= chip->ngpio)
		return -ENXIO;

	desc = sunxi_pinctrl_desc_find_function_by_pin(pctl, pinnum, "irq");
	if (!desc)
		return -EINVAL;

	irqnum = desc->irqbank * IRQ_PER_BANK + desc->irqnum;

	dev_dbg(chip->parent, "%s: request IRQ for GPIO %d, return %d\n",
		chip->label, offset + chip->base, irqnum);

	return irq_find_mapping(pctl->domain, irqnum);
}

static int sunxi_pinctrl_irq_request_resources(struct irq_data *d)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	struct sunxi_desc_function *func;

	func = sunxi_pinctrl_desc_find_function_by_pin(pctl,
					pctl->irq_array[d->hwirq], "irq");
	if (!func)
		return -EINVAL;

	/* Change muxing to INT mode */
	sunxi_pmx_set(pctl->pctl_dev, pctl->irq_array[d->hwirq], func->muxval);

	return 0;
}

static void sunxi_pinctrl_irq_release_resources(struct irq_data *d)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	struct sunxi_desc_function *func;

	func = sunxi_pinctrl_desc_find_function_by_pin(pctl,
					pctl->irq_array[d->hwirq], "io_disabled");
	if (!func)
		return;

	/* Change muxing to io_disabled mode */
	sunxi_pmx_set(pctl->pctl_dev, pctl->irq_array[d->hwirq], func->muxval);
}

static int sunxi_pinctrl_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	unsigned bank_base = pctl->desc->irq_bank_base[d->hwirq/IRQ_PER_BANK];
	u32 reg = sunxi_irq_cfg_reg(d->hwirq, bank_base);
	u8 index = sunxi_irq_cfg_offset(d->hwirq);
	unsigned long flags;
	u32 regval;
	u8 mode;

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		mode = IRQ_EDGE_RISING;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		mode = IRQ_EDGE_FALLING;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		mode = IRQ_EDGE_BOTH;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		mode = IRQ_LEVEL_HIGH;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		mode = IRQ_LEVEL_LOW;
		break;
	default:
		return -EINVAL;
	}

	raw_spin_lock_irqsave(&pctl->lock, flags);

	if (type & IRQ_TYPE_LEVEL_MASK)
		irq_set_chip_handler_name_locked(d, &sunxi_pinctrl_level_irq_chip,
						 handle_fasteoi_irq, NULL);
	else
		irq_set_chip_handler_name_locked(d, &sunxi_pinctrl_edge_irq_chip,
						 handle_edge_irq, NULL);

	regval = readl(pctl->membase + reg);
	regval &= ~(IRQ_CFG_IRQ_MASK << index);
	writel(regval | (mode << index), pctl->membase + reg);

	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static void sunxi_pinctrl_irq_ack(struct irq_data *d)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	unsigned bank_base = pctl->desc->irq_bank_base[d->hwirq/IRQ_PER_BANK];
	u32 status_reg = sunxi_irq_status_reg(d->hwirq, bank_base);
	u8 status_idx = sunxi_irq_status_offset(d->hwirq);

	/* Clear the IRQ */
	writel(1 << status_idx, pctl->membase + status_reg);
}

static void sunxi_pinctrl_irq_mask(struct irq_data *d)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	unsigned bank_base = pctl->desc->irq_bank_base[d->hwirq/IRQ_PER_BANK];
	u32 reg = sunxi_irq_ctrl_reg(d->hwirq, bank_base);
	u8 idx = sunxi_irq_ctrl_offset(d->hwirq);
	unsigned long flags;
	u32 val;

	raw_spin_lock_irqsave(&pctl->lock, flags);

	/* Mask the IRQ */
	val = readl(pctl->membase + reg);
	writel(val & ~(1 << idx), pctl->membase + reg);

	raw_spin_unlock_irqrestore(&pctl->lock, flags);
}

static void sunxi_pinctrl_irq_unmask(struct irq_data *d)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	unsigned bank_base = pctl->desc->irq_bank_base[d->hwirq/IRQ_PER_BANK];
	u32 reg = sunxi_irq_ctrl_reg(d->hwirq, bank_base);
	u8 idx = sunxi_irq_ctrl_offset(d->hwirq);
	unsigned long flags;
	u32 val;

	raw_spin_lock_irqsave(&pctl->lock, flags);

	/* Unmask the IRQ */
	val = readl(pctl->membase + reg);
	writel(val | (1 << idx), pctl->membase + reg);

	raw_spin_unlock_irqrestore(&pctl->lock, flags);
}

static void sunxi_pinctrl_irq_ack_unmask(struct irq_data *d)
{
	sunxi_pinctrl_irq_ack(d);
	sunxi_pinctrl_irq_unmask(d);
}

static int sunxi_pinctrl_irq_set_wake(struct irq_data *d, unsigned int on)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	unsigned long bank = d->hwirq / PINS_PER_BANK;
	unsigned long idx = d->hwirq % PINS_PER_BANK;
	struct irq_data *bank_irq_d = irq_get_irq_data(pctl->irq[bank]);


	if (on) {
		pctl->wake_mask[bank] |= BIT(idx);
		arisc_set_wakeup_source(SET_SEC_WAKEUP_SOURCE(bank_irq_d->hwirq,
							d->hwirq));
	} else {
		pctl->wake_mask[bank] &= ~BIT(idx);
		arisc_clear_wakeup_source(SET_SEC_WAKEUP_SOURCE(bank_irq_d->hwirq,
							d->hwirq));
	}

	return 0;
}

static int sunxi_pinctrl_get_intc_mask(struct sunxi_pinctrl *pctl, u32 *mask)
{
	u8 bank;
	unsigned bank_base;
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	for (bank = 0; bank < pctl->desc->irq_banks; bank++) {
		bank_base = pctl->desc->irq_bank_base[bank];
		mask[bank] = readl(pctl->membase +
				sunxi_irq_ctrl_reg_from_bank(bank, bank_base));
	}

	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;

}

static int sunxi_pinctrl_set_intc_mask(struct sunxi_pinctrl *pctl, u32 *mask)
{
	u8 bank;
	unsigned bank_base;
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	for (bank = 0; bank < pctl->desc->irq_banks; bank++) {
		bank_base = pctl->desc->irq_bank_base[bank];
		writel(mask[bank], pctl->membase +
			sunxi_irq_ctrl_reg_from_bank(bank, bank_base));
	}

	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int sunxi_pinctrl_suspend(struct device *device)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(device);
	const struct sunxi_pinctrl_desc *desc = pctl->desc;
	unsigned long flags;
	u8 bank, i;
	u32 *regs = pctl->regs_backup, *bank_membase;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	for (bank = 0; bank < desc->banks; bank++) {
		bank_membase = pctl->membase + (desc->bank_base[bank] + bank)
								* BANK_MEM_SIZE;
		for (i = 0; i < BANK_MEM_SIZE/4; i++)
			*regs++ = readl(bank_membase + i);
	}

	for (bank = 0; bank < desc->irq_banks; bank++) {
		bank_membase = pctl->membase + IRQ_CFG_REG +
				(desc->irq_bank_base[bank] + bank) * IRQ_MEM_SIZE;
		for (i = 0; i < IRQ_CFG_SIZE/4; i++)
			*regs++ = readl(bank_membase + i);
		*regs++ = readl(pctl->membase +
				sunxi_irq_debounce_reg_from_bank(bank, pctl->desc->irq_bank_base[bank]));
	}
#ifdef CONFIG_SUNXI_PIO_POWER_MODE
	*regs++ = readl(pctl->membase + GPIO_POW_MODE_SEL);
#endif
	raw_spin_unlock_irqrestore(&pctl->lock, flags);


	/* store the pins irq mask */
	sunxi_pinctrl_get_intc_mask(pctl, pctl->cur_mask);
	/* enable the wakeup pins irq */
	sunxi_pinctrl_set_intc_mask(pctl, pctl->wake_mask);

	return 0;
}

static int sunxi_pinctrl_resume(struct device *device)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(device);
	const struct sunxi_pinctrl_desc *desc = pctl->desc;
	unsigned long flags;
	int bank, i;
	u32 *regs = pctl->regs_backup, *bank_membase;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	for (bank = 0; bank < desc->banks; bank++) {
		bank_membase = pctl->membase + (desc->bank_base[bank] + bank)
								* BANK_MEM_SIZE;
		for (i = 0; i < BANK_MEM_SIZE/4; i++)
			writel(*regs++, bank_membase + i);
	}

	for (bank = 0; bank < desc->irq_banks; bank++) {
		bank_membase = pctl->membase + IRQ_CFG_REG +
				(desc->irq_bank_base[bank] + bank) * IRQ_MEM_SIZE;
		for (i = 0; i < IRQ_CFG_SIZE/4; i++)
			writel(*regs++, bank_membase + i);
		writel(*regs++, pctl->membase +
				sunxi_irq_debounce_reg_from_bank(bank,
					pctl->desc->irq_bank_base[bank]));
	}
#ifdef CONFIG_SUNXI_PIO_POWER_MODE
	writel(*regs++, pctl->membase + GPIO_POW_MODE_SEL);
#endif
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	/* restore the pins irq mask */
	sunxi_pinctrl_set_intc_mask(pctl, pctl->cur_mask);

	return 0;
}

const struct dev_pm_ops sunxi_pinctrl_pm_ops = {
	.suspend_noirq = sunxi_pinctrl_suspend,
	.resume_noirq = sunxi_pinctrl_resume,
};

static struct irq_chip sunxi_pinctrl_edge_irq_chip = {
	.name		= "sunxi_pio_edge",
	.irq_ack	= sunxi_pinctrl_irq_ack,
	.irq_mask	= sunxi_pinctrl_irq_mask,
	.irq_unmask	= sunxi_pinctrl_irq_unmask,
	.irq_request_resources = sunxi_pinctrl_irq_request_resources,
	.irq_release_resources = sunxi_pinctrl_irq_release_resources,
	.irq_set_type	= sunxi_pinctrl_irq_set_type,
	.irq_set_wake	= sunxi_pinctrl_irq_set_wake,
};

static struct irq_chip sunxi_pinctrl_level_irq_chip = {
	.name		= "sunxi_pio_level",
	.irq_eoi	= sunxi_pinctrl_irq_ack,
	.irq_mask	= sunxi_pinctrl_irq_mask,
	.irq_unmask	= sunxi_pinctrl_irq_unmask,
	/* Define irq_enable / disable to avoid spurious irqs for drivers
	 * using these to suppress irqs while they clear the irq source */
	.irq_enable	= sunxi_pinctrl_irq_ack_unmask,
	.irq_disable	= sunxi_pinctrl_irq_mask,
	.irq_request_resources = sunxi_pinctrl_irq_request_resources,
	.irq_release_resources = sunxi_pinctrl_irq_release_resources,
	.irq_set_type	= sunxi_pinctrl_irq_set_type,
	.irq_set_wake	= sunxi_pinctrl_irq_set_wake,
	.flags		= IRQCHIP_EOI_THREADED | IRQCHIP_EOI_IF_HANDLED,
};

static int sunxi_pinctrl_irq_of_xlate(struct irq_domain *d,
				      struct device_node *node,
				      const u32 *intspec,
				      unsigned int intsize,
				      unsigned long *out_hwirq,
				      unsigned int *out_type)
{
	struct sunxi_pinctrl *pctl = d->host_data;
	struct sunxi_desc_function *desc;
	int pin, base;

	if (intsize < 3)
		return -EINVAL;

	base = PINS_PER_BANK * intspec[0];
	pin = pctl->desc->pin_base + base + intspec[1];

	desc = sunxi_pinctrl_desc_find_function_by_pin(pctl, pin, "irq");
	if (!desc)
		return -EINVAL;

	*out_hwirq = desc->irqbank * PINS_PER_BANK + desc->irqnum;
	*out_type = intspec[2];

	return 0;
}

static const struct irq_domain_ops sunxi_pinctrl_irq_domain_ops = {
	.xlate		= sunxi_pinctrl_irq_of_xlate,
};

static void sunxi_pinctrl_irq_handler(struct irq_desc *desc)
{
	unsigned int irq = irq_desc_get_irq(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct sunxi_pinctrl *pctl = irq_desc_get_handler_data(desc);
	unsigned long bank, reg, val;
	unsigned int base_bank;

	chained_irq_enter(chip, desc);
	for (bank = 0; bank < pctl->desc->irq_banks; bank++)
		if (irq == pctl->irq[bank])
			break;

	if (bank == pctl->desc->irq_banks)
		return;

	base_bank = pctl->desc->irq_bank_base[bank];
	reg = sunxi_irq_status_reg_from_bank(bank, base_bank);
	val = readl(pctl->membase + reg);
	chained_irq_enter(chip, desc);

	if (val) {
		int irqoffset;

		for_each_set_bit(irqoffset, &val, IRQ_PER_BANK) {
			int pin_irq = irq_find_mapping(pctl->domain,
						       bank * IRQ_PER_BANK + irqoffset);
			generic_handle_irq(pin_irq);
		}
	}
	chained_irq_exit(chip, desc);

}

static int sunxi_pinctrl_add_function(struct sunxi_pinctrl *pctl,
					const char *name)
{
	struct sunxi_pinctrl_function *func = pctl->functions;

	while (func->name) {
		/* function already there */
		if (strcmp(func->name, name) == 0) {
			func->ngroups++;
			return -EEXIST;
		}
		func++;
	}

	func->name = name;
	func->ngroups = 1;
	return 0;
}

static int sunxi_pinctrl_count_function(struct sunxi_pinctrl *pctl,
					struct sunxi_pinctrl_function *func)
{
	int i = 0;
	int nfunctions = 0;
	/* Count functions */
	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_desc_function *func_desc = pin->functions;

		while (func_desc->name) {
			struct sunxi_pinctrl_function *tmp_func = func;

			while (tmp_func->name) {
				if (strcmp(func_desc->name, tmp_func->name) == 0)
					break;
				tmp_func++;
			}
			if (!tmp_func->name) {
				tmp_func->name = func_desc->name;
				nfunctions++;
			}
			func_desc++;
		}
	}
	return nfunctions;
}


static int sunxi_pinctrl_build_state(struct platform_device *pdev)
{
	struct sunxi_pinctrl *pctl = platform_get_drvdata(pdev);
	int i;

	pctl->ngroups = pctl->desc->npins;

	/* Allocate groups */
	pctl->groups = devm_kzalloc(&pdev->dev,
				    pctl->ngroups * sizeof(*pctl->groups),
				    GFP_KERNEL);
	if (!pctl->groups)
		return -ENOMEM;

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_pinctrl_group *group = pctl->groups + i;

		group->name = pin->pin.name;
		group->pin = pin->pin.number;
	}

	/*
	 * We have known all functions of pin,so we should use
	 * pctl->nfunctions instead of pctl->desc->npins to alloc buffer
	 */
	pctl->functions = devm_kzalloc(&pdev->dev,
				pctl->nfunctions * sizeof(*pctl->functions),
				GFP_KERNEL);
	if (!pctl->functions)
		return -ENOMEM;

	/* Count functions associated groups */
	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_desc_function *func = pin->functions;

		while (func->name) {
			/* Create interrupt mapping while we're at it */
			if (!strcmp(func->name, "irq")) {
				int irqnum = func->irqnum + func->irqbank * IRQ_PER_BANK;
				pctl->irq_array[irqnum] = pin->pin.number;
			}

			sunxi_pinctrl_add_function(pctl, func->name);
			func++;
		}
	}

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_desc_function *func = pin->functions;

		while (func->name) {
			struct sunxi_pinctrl_function *func_item;
			const char **func_grp;

			func_item = sunxi_pinctrl_find_function_by_name(pctl,
									func->name);
			if (!func_item)
				return -EINVAL;

			if (!func_item->groups) {
				func_item->groups =
					devm_kzalloc(&pdev->dev,
						     func_item->ngroups * sizeof(*func_item->groups),
						     GFP_KERNEL);
				if (!func_item->groups)
					return -ENOMEM;
			}

			func_grp = func_item->groups;
			while (*func_grp)
				func_grp++;

			*func_grp = pin->pin.name;
			func++;
		}
	}

	return 0;
}

static int sunxi_pinctrl_get_debounce_div(unsigned long clock_rate, int freq)
{
	unsigned int best_diff, best_div;
	int i;

	best_diff = abs(freq - clock_rate);
	best_div = 0;

	for (i = 1; i < 8; i++) {
		int cur_diff = abs(freq - (clock_rate >> i));

		if (cur_diff < best_diff) {
			best_diff = cur_diff;
			best_div = i;
		}
	}

	return best_div;
}

static int sunxi_pinctrl_setup_debounce(struct sunxi_pinctrl *pctl,
					struct device_node *node)
{
	unsigned long hosc_rate, losc_rate;
	struct clk *hosc, *losc;
	u8 div, src;
	int i, ret;

	/* Deal with old DTs that didn't have the oscillators */
	if (of_count_phandle_with_args(node, "clocks", "#clock-cells") != 3)
		return 0;
	/* If we don't have any setup, bail out */
	if (!of_find_property(node, "input-debounce", NULL))
		return 0;

	losc = devm_clk_get(pctl->dev, "losc");
	if (IS_ERR(losc))
		return PTR_ERR(losc);

	hosc = devm_clk_get(pctl->dev, "hosc");
	if (IS_ERR(hosc))
		return PTR_ERR(hosc);

	losc_rate = clk_get_rate(losc);
	hosc_rate = clk_get_rate(hosc);

	for (i = 0; i < pctl->desc->irq_banks; i++) {
		unsigned long debounce_freq;
		u32 debounce;

		ret = of_property_read_u32_index(node, "input-debounce",
						 i, &debounce);
		if (ret)
			return ret;

		if (!debounce)
			continue;

		/*
		 * if debounce > 1, freq = (1000000 + (debounce/2)) / debounce
		 * else freq = (1000000 - (debounce/2)) / debounce
		 */
		debounce_freq = DIV_ROUND_CLOSEST(USEC_PER_SEC, debounce);

		if (debounce_freq > hosc_rate) {
			pr_debug("not support set rate %ld, use max rate%ld\n",
					debounce_freq, hosc_rate);
			src = 1;
			div = 0;
		} else if (debounce_freq > losc_rate) {
			pr_debug("use hosc , set rate %ld\n", debounce_freq);
			src = 1;
			div = sunxi_pinctrl_get_debounce_div(hosc_rate,
								  debounce_freq);
		} else {
			pr_debug("use default, set min rate %ld\n", losc_rate);
			src = 0;
			div = 0;
		}

		writel(src | div << 4,
		       pctl->membase +
		       sunxi_irq_debounce_reg_from_bank(i,
						pctl->desc->irq_bank_base[i]));
	}

	return 0;
}

/* Show sunxi pinctrl state, provide debug information */
void sunxi_pinctrl_state_show(void)
{
	struct pinctrl_dev *pctldev;
	struct sunxi_pinctrl *pctl;
	unsigned long config;
	unsigned i, pin, muxsel;

	pctldev = get_pinctrl_dev_from_devname(SUNXI_PINCTRL);
	if (!pctldev) {
		pr_err("can't get pinctrl dev for %s\n", SUNXI_PINCTRL);
		return;
	}

	pctl = pinctrl_dev_get_drvdata(pctldev);

	pr_info("pio function state: (%d pins registered)\n",
		pctl->desc->npins);
	pr_info("pin num (name): function    owner\n");

	/* The pin number can be retrived from the pin controller descriptor */
	for (i = 0; i < pctl->desc->npins; i++) {
		struct pin_desc *desc;
		struct sunxi_desc_function *func;

		pin = pctldev->desc->pins[i].number;
		desc = pin_desc_get(pctldev, pin);

		/* Get real pin function */
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFFFF);
		sunxi_pconf_get(pctldev, pin, &config);
		muxsel = SUNXI_PINCFG_UNPACK_VALUE(config);

		func = pctl->desc->pins[i].functions;
		while (func->name) {
			if (func->muxval == muxsel)
				break;
			func++;
		}

		pr_info("pin %-3d (%-4s): %-11s %s\n", pin, desc->name,
			 func->name ? func->name : "(UNCLAIMED)",
			 desc->mux_owner ? desc->mux_owner : "(UNCLAIMED)");
	}
}
EXPORT_SYMBOL_GPL(sunxi_pinctrl_state_show);

int sunxi_sel_pio_mode(struct pinctrl *pinctrl, u32 pm_sel)
{
#ifdef CONFIG_SUNXI_PIO_POWER_MODE
	struct pinctrl_setting const *setting;
	struct sunxi_pinctrl *pctl;
	struct pinctrl_dev *pctldev;
	const struct pinctrl_ops *pctlops;
	int ret, i;
	const unsigned *pins = NULL;
	unsigned num_pins = 0;
	const char *gname;
	u32 bank, tmp;
#ifdef CONFIG_SUNXI_PIO_POWER_SEL
	u32 tmp1;
#endif
	unsigned long flags;

	list_for_each_entry(setting, &pinctrl->state->settings, node) {
		if (setting->type != PIN_MAP_TYPE_MUX_GROUP)
			continue;

		pctldev = setting->pctldev;
		pctlops = pctldev->desc->pctlops;

		if (pctlops->get_group_pins)
			ret = pctlops->get_group_pins(pctldev,
						setting->data.mux.group,
						&pins, &num_pins);

		if (ret) {
			gname = pctlops->get_group_name(pctldev,
						setting->data.mux.group);
			dev_err(pctldev->dev,
				 "could not get pins for group %s\n", gname);
			return -EINVAL;
		}

		pctl = pinctrl_dev_get_drvdata(pctldev);
		for (i = 0; i < num_pins; i++) {
			bank = pins[i] / PINS_PER_BANK;
			raw_spin_lock_irqsave(&pctl->lock, flags);
			tmp = readl(pctl->membase + GPIO_POW_MODE_SEL);
			tmp |= (pm_sel << bank);
			writel(tmp, pctl->membase + GPIO_POW_MODE_SEL);
#ifdef CONFIG_SUNXI_PIO_POWER_SEL
			if (bank == 5) {
				tmp1 = readl(pctl->membase + GPIO_POW_VOL_SEL);
				tmp1 &= ~(1>>0);
				tmp1 |= (!pm_sel);
				writel(tmp1, pctl->membase + GPIO_POW_VOL_SEL);
			}
#endif
			raw_spin_unlock_irqrestore(&pctl->lock, flags);
		}
	}
	return 0;
#else
	return -EINVAL;
#endif
}
EXPORT_SYMBOL_GPL(sunxi_sel_pio_mode);

#ifdef CONFIG_DEBUG_FS
#define SUNXI_MAX_NAME_LEN 20
static char sunxi_dbg_pinname[SUNXI_MAX_NAME_LEN];
static char sunxi_dbg_devname[SUNXI_MAX_NAME_LEN] = SUNXI_PINCTRL;
static unsigned long sunxi_dbg_function;
static unsigned long sunxi_dbg_data;
static unsigned long sunxi_dbg_level;
static unsigned long sunxi_dbg_pull;

static int sunxi_pin_configure_show(struct seq_file *s, void *d)
{
	int pin;
	unsigned long config;
	struct pinctrl_dev *pctldev;
	struct sunxi_pinctrl *pctl;

	/* get pinctrl device */
	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev) {
		seq_puts(s, "cannot get pinctrl device from devname\n");
		return -EINVAL;
	}

	/* change pin name to pin index */
	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}
	pctl = pinctrl_dev_get_drvdata(pctldev);

	/*get pin func*/
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] funciton: %lx\n", sunxi_dbg_pinname,
			SUNXI_PINCFG_UNPACK_VALUE(config));

	/*get pin data*/
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] data: %lx\n", sunxi_dbg_pinname,
			SUNXI_PINCFG_UNPACK_VALUE(config));

	/*get pin dlevel*/
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] dlevel: %lx\n", sunxi_dbg_pinname,
			SUNXI_PINCFG_UNPACK_VALUE(config));

	/*get pin pull*/
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] pull: %lx\n", sunxi_dbg_pinname,
			SUNXI_PINCFG_UNPACK_VALUE(config));

	return 0;
}

static ssize_t sunxi_pin_configure_write(struct file *file,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	int pin;
	unsigned int function;
	unsigned int data;
	unsigned int pull;
	unsigned int dlevel;
	unsigned long config;
	struct pinctrl_dev *pctldev;
	struct sunxi_pinctrl *pctl;
	unsigned char buf[SUNXI_MAX_NAME_LEN];

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;
	err = sscanf(buf, "%19s %u %u %u %u", sunxi_dbg_pinname,
			&function, &data, &dlevel, &pull);
	if (err != 6)
		return -EINVAL;

	if (function > 7) {
		pr_err("Input Parameters function error!\n");
		return -EINVAL;
	}
	sunxi_dbg_function = function;

	if (data > 1) {
		pr_err("Input Parameters data error!\n");
		return -EINVAL;
	}
	sunxi_dbg_data = data;

	if (pull > 3) {
		pr_err("Input Parameters pull error!\n");
		return -EINVAL;
	}
	sunxi_dbg_pull = pull;

	if (dlevel > 3) {
		pr_err("Input Parameters dlevel error!\n");
		return -EINVAL;
	}
	sunxi_dbg_level = dlevel;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	pctl = pinctrl_dev_get_drvdata(pctldev);

	/* set function value*/
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, function);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
	pr_debug("pin[%s] set function:     %x;\n",
			sunxi_dbg_pinname, function);

	/* set data value*/
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, data);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
	pr_debug("pin[%s] set data:     %x;\n", sunxi_dbg_pinname, data);

	/* set dlevel value*/
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, dlevel);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
	pr_debug("pin[%s] set dlevel:     %x;\n", sunxi_dbg_pinname, dlevel);

	/* set pull value*/
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pull);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
	pr_debug("pin[%s] set pull:     %x;\n", sunxi_dbg_pinname, pull);

	return count;
}

static int sunxi_pin_show(struct seq_file *s, void *d)
{
	if (strlen(sunxi_dbg_pinname))
		seq_printf(s, "%s\n", sunxi_dbg_pinname);
	else
		seq_puts(s, "No pin name set\n");

	return 0;
}

static ssize_t sunxi_pin_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	unsigned char buf[SUNXI_MAX_NAME_LEN];

	if (count > SUNXI_MAX_NAME_LEN)
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s", sunxi_dbg_pinname);
	if (err != 1)
		return -EINVAL;

	return count;
}

static int sunxi_dev_name_show(struct seq_file *s, void *d)
{
	if (strlen(sunxi_dbg_devname))
		seq_printf(s, "%s\n", sunxi_dbg_devname);
	else
		seq_puts(s, "No dev name set\n");

	return 0;
}

static ssize_t sunxi_dev_name_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	unsigned char buf[SUNXI_MAX_NAME_LEN];

	if (count > SUNXI_MAX_NAME_LEN)
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s", sunxi_dbg_devname);
	if (err != 1)
		return -EINVAL;

	return count;
}


static int sunxi_pin_function_show(struct seq_file *s, void *d)
{
	unsigned long config;
	struct pinctrl_dev *pctldev;
	int pin;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] funciton: %lx\n", sunxi_dbg_pinname,
			SUNXI_PINCFG_UNPACK_VALUE(config));
	return 0;
}

static ssize_t sunxi_pin_function_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int err;
	unsigned long function;
	unsigned long config;
	int pin;
	unsigned char buf[SUNXI_MAX_NAME_LEN];
	struct pinctrl_dev *pctldev;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %lu", sunxi_dbg_pinname, &function);
	if (err != 2)
		return err;

	if (function > 7) {
		pr_debug("Input Parameters function error!\n");
		return -EINVAL;
	}

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	sunxi_dbg_function = function;

	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, function);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);

	return count;
}

static int sunxi_pin_data_show(struct seq_file *s, void *d)
{
	unsigned long config;
	int pin;
	struct pinctrl_dev *pctldev;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] data: %lx\n", sunxi_dbg_pinname,
			SUNXI_PINCFG_UNPACK_VALUE(config));
	return 0;
}

static ssize_t sunxi_pin_data_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int err;
	unsigned long data;
	unsigned long config;
	unsigned char buf[SUNXI_MAX_NAME_LEN];
	int pin;
	struct pinctrl_dev *pctldev;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %lu", sunxi_dbg_pinname, &data);
	if (err != 2)
		return err;

	if (data > 1) {
		pr_debug("Input Parameters data error!\n");
		return -EINVAL;
	}
	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}
	sunxi_dbg_data = data;

	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, data);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);

	return count;
}

static int sunxi_pin_dlevel_show(struct seq_file *s, void *d)
{
	unsigned long config;
	int pin;
	struct pinctrl_dev *pctldev;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;
	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 0XFFFFFF);
	pin_config_get(SUNXI_PINCTRL, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] dlevel: %lx\n", sunxi_dbg_pinname,
			SUNXI_PINCFG_UNPACK_VALUE(config));
	return 0;
}

static ssize_t sunxi_pin_dlevel_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	unsigned long dlevel;
	unsigned long config;
	unsigned char buf[SUNXI_MAX_NAME_LEN];
	int pin;
	struct pinctrl_dev *pctldev;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %lu", sunxi_dbg_pinname, &dlevel);
	if (err != 2)
		return err;

	if (dlevel > 3) {
		pr_debug("Input Parameters dlevel error!\n");
		return -EINVAL;
	}

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	sunxi_dbg_level = dlevel;

	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, dlevel);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);

	return count;
}

static int sunxi_pin_pull_show(struct seq_file *s, void *d)
{
	unsigned long config;
	int pin;
	struct pinctrl_dev *pctldev;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] pull: %lx\n", sunxi_dbg_pinname,
			SUNXI_PINCFG_UNPACK_VALUE(config));
	return 0;
}

static ssize_t sunxi_pin_pull_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int err;
	unsigned long pull;
	unsigned long config;
	unsigned char buf[SUNXI_MAX_NAME_LEN];
	int pin;
	struct pinctrl_dev *pctldev;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %lu", sunxi_dbg_pinname, &pull);
	if (err != 2)
		return err;

	if (pull > 3) {
		pr_debug("Input Parameters pull error!\n");
		return -EINVAL;
	}

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		pr_err("unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pull);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);

	return count;
}

static int sunxi_platform_show(struct seq_file *s, void *d)
{
#if defined(CONFIG_ARCH_SUN8IW10P1)
	seq_puts(s, "SUN8IW10P1\n");
#elif defined(CONFIG_ARCH_SUN8IW11P1)
	seq_puts(s, "SUN8IW11P1\n");
#elif defined(CONFIG_ARCH_SUN8IW7P1)
	seq_puts(s, "SUN8IW7P1\n");
#elif defined(CONFIG_ARCH_SUN50IW1P1)
	seq_puts(s, "SUN50IW1P1\n");
#elif defined(CONFIG_ARCH_SUN50IW2P1)
	seq_puts(s, "SUN50IW2P1\n");
#elif defined(CONFIG_ARCH_SUN50IW3P1)
	seq_puts(s, "SUN50IW3P1\n");
#elif defined(CONFIG_ARCH_SUN50IW6P1)
	seq_puts(s, "SUN50IW6P1\n");
#elif defined(CONFIG_ARCH_SUN3IW1P1)
	seq_puts(s, "SUN3IW1P1\n");
#else
	seq_puts(s, "NOMATCH\n");
#endif
	return 0;
}

static ssize_t sunxi_platform_write(struct file *file,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	return count;
}

static int sunxi_pin_configure_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_configure_show, inode->i_private);
}

static int sunxi_pin_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_show, inode->i_private);
}

static int sunxi_dev_name_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_dev_name_show, inode->i_private);
}

static int sunxi_pin_function_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_function_show, inode->i_private);
}

static int sunxi_pin_data_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_data_show, inode->i_private);
}

static int sunxi_pin_dlevel_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_dlevel_show, inode->i_private);
}

static int sunxi_pin_pull_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_pull_show, inode->i_private);
}

static int sunxi_platform_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_platform_show, inode->i_private);
}

static const struct file_operations sunxi_pin_configure_ops = {
	.open		= sunxi_pin_configure_open,
	.write		= sunxi_pin_configure_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_ops = {
	.open		= sunxi_pin_open,
	.write		= sunxi_pin_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_dev_name_ops = {
	.open		= sunxi_dev_name_open,
	.write		= sunxi_dev_name_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_function_ops = {
	.open		= sunxi_pin_function_open,
	.write		= sunxi_pin_function_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_data_ops = {
	.open		= sunxi_pin_data_open,
	.write		= sunxi_pin_data_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_dlevel_ops = {
	.open		= sunxi_pin_dlevel_open,
	.write		= sunxi_pin_dlevel_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_pull_ops = {
	.open		= sunxi_pin_pull_open,
	.write		= sunxi_pin_pull_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_platform_ops = {
	.open		= sunxi_platform_open,
	.write		= sunxi_platform_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static struct dentry *debugfs_root;

static void sunxi_pinctrl_debugfs(void)
{
	debugfs_root = debugfs_create_dir("sunxi_pinctrl", NULL);
	if (IS_ERR(debugfs_root) || !debugfs_root) {
		pr_debug("failed to create debugfs directory\n");
		debugfs_root = NULL;
		return;
	}

	debugfs_create_file("sunxi_pin_configure",
				(S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_configure_ops);
	debugfs_create_file("sunxi_pin", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_ops);
	debugfs_create_file("function", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_function_ops);
	debugfs_create_file("data", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_data_ops);
	debugfs_create_file("dlevel", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_dlevel_ops);
	debugfs_create_file("pull", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_pull_ops);
	debugfs_create_file("platform", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_platform_ops);
	debugfs_create_file("dev_name", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_dev_name_ops);

}
#endif
int sunxi_pinctrl_init(struct platform_device *pdev,
		       const struct sunxi_pinctrl_desc *desc)
{
	struct device_node *node = pdev->dev.of_node;
	struct pinctrl_desc *pctrl_desc;
	struct pinctrl_pin_desc *pins;
	struct sunxi_pinctrl *pctl;
	struct pinmux_ops *pmxops;
	struct resource *res;
	int i, ret, last_pin;
	struct clk *clk;
	struct sunxi_pinctrl_function *func;

	pctl = devm_kzalloc(&pdev->dev, sizeof(*pctl), GFP_KERNEL);
	if (!pctl)
		return -ENOMEM;
	platform_set_drvdata(pdev, pctl);

	raw_spin_lock_init(&pctl->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pctl->membase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pctl->membase))
		return PTR_ERR(pctl->membase);

	pctl->dev = &pdev->dev;
	pctl->desc = desc;

	pctl->irq_array = devm_kcalloc(&pdev->dev,
				       IRQ_PER_BANK * pctl->desc->irq_banks,
				       sizeof(*pctl->irq_array),
				       GFP_KERNEL);
	if (!pctl->irq_array)
		return -ENOMEM;
	pctl->wake_mask = devm_kcalloc(&pdev->dev,
					pctl->desc->irq_banks,
					sizeof(*pctl->wake_mask),
					GFP_KERNEL);
	if (!pctl->wake_mask)
		return -ENOMEM;
	pctl->cur_mask = devm_kcalloc(&pdev->dev,
					pctl->desc->irq_banks,
					sizeof(*pctl->cur_mask),
					GFP_KERNEL);
	if (!pctl->cur_mask)
		return -ENOMEM;

	/*
	 * alloc a temp buffer for count the pin
	 * functions,and then we shuold free it
	 */
	func = kzalloc(pctl->desc->npins * sizeof(*pctl->functions) * 7,
		       GFP_KERNEL);
	if (!func)
		return -ENOMEM;
	/* count the pin functions for using in sunxi_pinctrl_build_state */
	pctl->nfunctions = sunxi_pinctrl_count_function(pctl, func);

	kfree(func);

	ret = sunxi_pinctrl_build_state(pdev);
	if (ret) {
		dev_err(&pdev->dev, "dt probe failed: %d\n", ret);
		return ret;
	}

	pins = devm_kzalloc(&pdev->dev,
			    pctl->desc->npins * sizeof(*pins),
			    GFP_KERNEL);
	if (!pins)
		return -ENOMEM;

	for (i = 0; i < pctl->desc->npins; i++)
		pins[i] = pctl->desc->pins[i].pin;

	pctrl_desc = devm_kzalloc(&pdev->dev,
				  sizeof(*pctrl_desc),
				  GFP_KERNEL);
	if (!pctrl_desc)
		return -ENOMEM;

	pctrl_desc->name = dev_name(&pdev->dev);
	pctrl_desc->owner = THIS_MODULE;
	pctrl_desc->pins = pins;
	pctrl_desc->npins = pctl->desc->npins;
	pctrl_desc->confops = &sunxi_pconf_ops;
	pctrl_desc->pctlops = &sunxi_pctrl_ops;

	pmxops = devm_kmemdup(&pdev->dev, &sunxi_pmx_ops, sizeof(sunxi_pmx_ops),
			      GFP_KERNEL);
	if (!pmxops)
		return -ENOMEM;

	if (desc->disable_strict_mode)
		pmxops->strict = false;

	pctrl_desc->pmxops = pmxops;

	pctl->pctl_dev = pinctrl_register(pctrl_desc,
					  &pdev->dev, pctl);
	if (IS_ERR(pctl->pctl_dev)) {
		dev_err(&pdev->dev, "couldn't register pinctrl driver\n");
		return PTR_ERR(pctl->pctl_dev);
	}

	pctl->regs_backup = devm_kzalloc(&pdev->dev,
		pctl->desc->banks * BANK_MEM_SIZE +  /* bank regs */
		pctl->desc->irq_banks * IRQ_MEM_SIZE + /* irq regs */
		4, /* power mode reg */
		GFP_KERNEL);

	pctl->chip = devm_kzalloc(&pdev->dev, sizeof(*pctl->chip), GFP_KERNEL);
	if (!pctl->chip) {
		ret = -ENOMEM;
		goto pinctrl_error;
	}

	last_pin = pctl->desc->pins[pctl->desc->npins - 1].pin.number;
	pctl->chip->owner = THIS_MODULE;
	pctl->chip->request = gpiochip_generic_request,
	pctl->chip->free = gpiochip_generic_free,
	pctl->chip->direction_input = sunxi_pinctrl_gpio_direction_input,
	pctl->chip->direction_output = sunxi_pinctrl_gpio_direction_output,
	pctl->chip->get = sunxi_pinctrl_gpio_get,
	pctl->chip->set = sunxi_pinctrl_gpio_set,
	pctl->chip->set_debounce = sunxi_pinctrl_gpio_set_debounce,
	pctl->chip->of_xlate = sunxi_pinctrl_gpio_of_xlate,
	pctl->chip->to_irq = sunxi_pinctrl_gpio_to_irq,
	pctl->chip->of_gpio_n_cells = 6,
	pctl->chip->can_sleep = false,
	pctl->chip->ngpio = round_up(last_pin + 1, PINS_PER_BANK) -
			    pctl->desc->pin_base;
	pctl->chip->label = dev_name(&pdev->dev);
	pctl->chip->parent = &pdev->dev;
	pctl->chip->base = pctl->desc->pin_base;

	ret = gpiochip_add(pctl->chip);
	if (ret)
		goto pinctrl_error;

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;

		ret = gpiochip_add_pin_range(pctl->chip, dev_name(&pdev->dev),
					     pin->pin.number - pctl->desc->pin_base,
					     pin->pin.number, 1);
		if (ret)
			goto gpiochip_error;
	}

	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		goto gpiochip_error;
	}

	ret = clk_prepare_enable(clk);
	if (ret)
		goto gpiochip_error;

	pctl->irq = devm_kcalloc(&pdev->dev,
				 pctl->desc->irq_banks,
				 sizeof(*pctl->irq),
				 GFP_KERNEL);
	if (!pctl->irq) {
		ret = -ENOMEM;
		goto clk_error;
	}

	for (i = 0; i < pctl->desc->irq_banks; i++) {
		pctl->irq[i] = platform_get_irq(pdev, i);
		if (pctl->irq[i] < 0) {
			ret = pctl->irq[i];
			goto clk_error;
		}
	}

	pctl->domain = irq_domain_add_linear(node,
					     pctl->desc->irq_banks * IRQ_PER_BANK,
					     &sunxi_pinctrl_irq_domain_ops,
					     pctl);
	if (!pctl->domain) {
		dev_err(&pdev->dev, "Couldn't register IRQ domain\n");
		ret = -ENOMEM;
		goto clk_error;
	}

	for (i = 0; i < (pctl->desc->irq_banks * IRQ_PER_BANK); i++) {
		int irqno = irq_create_mapping(pctl->domain, i);

		irq_set_chip_and_handler(irqno, &sunxi_pinctrl_edge_irq_chip,
					 handle_edge_irq);
		irq_set_chip_data(irqno, pctl);
	}

	for (i = 0; i < pctl->desc->irq_banks; i++) {
		/* Mask and clear all IRQs before registering a handler */
		unsigned bank_base = pctl->desc->irq_bank_base[i];

		writel(0, pctl->membase +
			sunxi_irq_ctrl_reg_from_bank(i, bank_base));
		writel(0xffffffff, pctl->membase +
			sunxi_irq_status_reg_from_bank(i, bank_base));

		irq_set_chained_handler_and_data(pctl->irq[i],
						 sunxi_pinctrl_irq_handler,
						 pctl);
	}

#ifdef CONFIG_DEBUG_FS
	sunxi_pinctrl_debugfs();
#endif

	sunxi_pinctrl_setup_debounce(pctl, node);

	dev_info(&pdev->dev, "initialized sunXi PIO driver\n");

	return 0;

clk_error:
	clk_disable_unprepare(clk);
gpiochip_error:
	gpiochip_remove(pctl->chip);
pinctrl_error:
	pinctrl_unregister(pctl->pctl_dev);
	return ret;
}

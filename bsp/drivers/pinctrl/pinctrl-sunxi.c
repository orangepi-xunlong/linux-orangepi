/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_clk.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/slab.h>
#include <sunxi-sip.h>
#include <linux/iopoll.h>
#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
#include <linux/irqnr.h>
#include <linux/irq.h>
#include <linux/remoteproc/sunxi_remoteproc.h>
#endif
#if IS_ENABLED(CONFIG_RISCV)
#include <asm/sbi.h>
#endif
#include <dt-bindings/pinctrl/sun4i-a10.h>
#include "core.h"
#include "pinctrl-sunxi.h"

#define SUNXI_PINCTRL_CORE_VERSION	"1.4.11"
#define SUNXI_PINCTRL_I2S0_ROUTE_PAD
/* Indexed by `enum sunxi_pinctrl_hw_type` */
struct sunxi_pinctrl_hw_info sunxi_pinctrl_hw_info[SUNXI_PCTL_HW_TYPE_CNT] = {
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.dlevel_regs_offset	= 0x14,
		.bank_mem_size          = 0x24,
		.pull_regs_offset       = 0x1c,
		.dlevel_pins_per_reg    = 16,
		.dlevel_pins_bits       = 2,
		.dlevel_pins_mask       = 0x3,
		.irq_mux_val         	= 0x6,
		.irq_cfg_reg		= 0x200,
		.irq_ctrl_reg		= 0x210,
		.irq_status_reg		= 0x214,
		.irq_debounce_reg	= 0x218,
		.irq_mem_base		= 0x200,
		.irq_mem_size		= 0x20,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x340,
		.power_mode_ctrl_reg	= 0x344,
		.power_mode_val_reg	= 0x348,
		.pio_pow_ctrl_reg	= 0x350,
		.power_mode_reverse	= false,
	},
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.dlevel_regs_offset	= 0x14,
		.bank_mem_size          = 0x30,
		.pull_regs_offset       = 0x24,
		.dlevel_pins_per_reg    = 8,
		.dlevel_pins_bits       = 4,
		.dlevel_pins_mask       = 0xF,
		.irq_mux_val         	= 0xE,
		.irq_cfg_reg		= 0x200,
		.irq_ctrl_reg		= 0x210,
		.irq_status_reg		= 0x214,
		.irq_debounce_reg	= 0x218,
		.irq_mem_base		= 0x200,
		.irq_mem_size		= 0x20,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x340,
		.power_mode_ctrl_reg	= 0x344,
		.power_mode_val_reg	= 0x348,
		.pio_pow_ctrl_reg	= 0x350,
		.power_mode_reverse	= false,
	},
	{

		.initial_bank_offset	= 0x80,
		.mux_regs_offset	= 0x00,
		.data_regs_offset	= 0x10,
		.dlevel_regs_offset	= 0x20,
		.bank_mem_size          = 0x80,
		.pull_regs_offset       = 0x30,
		.dlevel_pins_per_reg    = 8,
		.dlevel_pins_bits       = 4,
		.dlevel_pins_mask       = 0xF,
		.irq_mux_val         	= 0xE,
		.irq_cfg_reg		= 0xc0,
		.irq_ctrl_reg		= 0xd0,
		.irq_status_reg		= 0xd4,
		.irq_debounce_reg	= 0xd8,
		.irq_mem_base		= 0xc0,
		.irq_mem_size		= 0x80,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x340,
		.power_mode_ctrl_reg	= 0x344,
		.power_mode_val_reg	= 0x348,
		.pio_pow_ctrl_reg	= 0x350,
		.power_mode_reverse	= false,
	},
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.dlevel_regs_offset	= 0x14,
		.bank_mem_size          = 0x30,
		.pull_regs_offset       = 0x24,
		.dlevel_pins_per_reg    = 8,
		.dlevel_pins_bits       = 4,
		.dlevel_pins_mask       = 0xF,
		.irq_mux_val         	= 0xE,
		.irq_cfg_reg		= 0x200,
		.irq_ctrl_reg		= 0x210,
		.irq_status_reg		= 0x214,
		.irq_debounce_reg	= 0x218,
		.irq_mem_base		= 0x200,
		.irq_mem_size		= 0x20,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x380,
		.mode_sel_vccio_bit	= 12,
		.power_mode_ctrl_reg	= 0x384,
		.mode_ctrl_vccio_bit	= 12,
		.power_mode_val_reg	= 0x388,
		.mode_val_vccio_bit	= 16,
		.pio_pow_ctrl_reg	= 0x390,
		.power_mode_reverse	= true,
		.power_mode_detect	= true,
	},
	{
		.initial_bank_offset	= 0x80,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.data_set_regs_offset	= 0x14,
		.data_clr_regs_offset	= 0x18,
		.dlevel_regs_offset	= 0x20,
		.bank_mem_size		= 0x80,
		.pull_regs_offset	= 0x30,
		.dlevel_pins_per_reg	= 8,
		.dlevel_pins_bits	= 4,
		.dlevel_pins_mask	= 0xF,
		.irq_mux_val		= 0xE,
		.irq_cfg_reg		= 0xC0,
		.irq_ctrl_reg		= 0xD0,
		.irq_status_reg		= 0xD4,
		.irq_debounce_reg	= 0xD8,
		.irq_mem_base		= 0xC0,
		.irq_mem_size		= 0x80,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x40,
		.power_mode_ctrl_reg	= 0x48,
		.power_mode_val_reg	= 0x48,
		.pio_pow_ctrl_reg	= 0x70,
		.power_mode_reverse	= false,
		.data_set_mode_select	= false,
	},
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.dlevel_regs_offset	= 0x14,
		.bank_mem_size          = 0x24,
		.pull_regs_offset       = 0x1c,
		.dlevel_pins_per_reg    = 16,
		.dlevel_pins_bits       = 2,
		.dlevel_pins_mask       = 0x3,
		.irq_mux_val         	= 0x6,
		.irq_cfg_reg		= 0x200,
		.irq_ctrl_reg		= 0x210,
		.irq_status_reg		= 0x214,
		.irq_debounce_reg	= 0x218,
		.irq_mem_base		= 0x200,
		.irq_mem_size		= 0x20,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x340,
		.power_mode_ctrl_reg	= 0x344,
		.power_mode_val_reg	= 0x348,
		.pio_pow_ctrl_reg	= 0x350,
		.power_mode_reverse	= false,
		.power_mode_detect	= true,
	},
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.dlevel_regs_offset	= 0x14,
		.bank_mem_size          = 0x30,
		.pull_regs_offset       = 0x24,
		.dlevel_pins_per_reg    = 8,
		.dlevel_pins_bits       = 4,
		.dlevel_pins_mask       = 0xF,
		.irq_mux_val         	= 0xE,
		.irq_cfg_reg		= 0x200,
		.irq_ctrl_reg		= 0x210,
		.irq_status_reg		= 0x214,
		.irq_debounce_reg	= 0x218,
		.irq_mem_base		= 0x200,
		.irq_mem_size		= 0x20,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x340,
		.power_mode_ctrl_reg	= 0x344,
		.power_mode_val_reg	= 0x348,
		.pio_pow_ctrl_reg	= 0x350,
		.power_mode_reverse	= true,
		.power_mode_detect	= true,
	},
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x500,
		.data_reg_irregular	= true,
		.data_mem_size		= 0x10,
		.data_set_regs_offset	= 0x504,
		.data_clr_regs_offset	= 0x508,
		.dlevel_regs_offset	= 0x14,
		.bank_mem_size          = 0x30,
		.pull_regs_offset       = 0x24,
		.dlevel_pins_per_reg    = 8,
		.dlevel_pins_bits       = 4,
		.dlevel_pins_mask       = 0xF,
		.irq_mux_val         	= 0xE,
		.irq_cfg_reg		= 0x200,
		.irq_ctrl_reg		= 0x210,
		.irq_status_reg		= 0x214,
		.irq_debounce_reg	= 0x218,
		.irq_mem_base		= 0x200,
		.irq_mem_size		= 0x20,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x340,
		.power_mode_ctrl_reg	= 0x344,
		.power_mode_val_reg	= 0x348,
		.pio_pow_ctrl_reg	= 0x350,
		.power_mode_reverse	= true,
		.power_mode_detect	= true,
		.data_set_mode_select	= false,
	},
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.data_mem_size		= 0x30,
		.dlevel_regs_offset	= 0x14,
		.bank_mem_size          = 0x30,
		.pull_regs_offset       = 0x24,
		.dlevel_pins_per_reg    = 8,
		.dlevel_pins_bits       = 4,
		.dlevel_pins_mask       = 0xF,
		.irq_mux_val         	= 0xE,
		.irq_cfg_reg		= 0x40,
		.irq_ctrl_reg		= 0x50,
		.irq_status_reg		= 0x54,
		.irq_debounce_reg	= 0x58,
		.irq_mem_base		= 0x40,
		.irq_mem_size		= 0x20,
		.irq_mem_used		= 0x20,
	},
	{
		.initial_bank_offset	= 0x0,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.dlevel_regs_offset	= 0x14,
		.bank_mem_size          = 0x30,
		.pull_regs_offset       = 0x24,
		.dlevel_pins_per_reg    = 8,
		.dlevel_pins_bits       = 4,
		.dlevel_pins_mask       = 0xF,
		.irq_mux_val         	= 0xE,
		.irq_cfg_reg		= 0x200,
		.irq_ctrl_reg		= 0x210,
		.irq_status_reg		= 0x214,
		.irq_debounce_reg	= 0x218,
		.irq_mem_base		= 0x200,
		.irq_mem_size		= 0x20,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x340,
		.power_mode_ctrl_reg	= 0x344,
		.power_mode_val_reg	= 0x348,
		.pio_pow_ctrl_reg	= 0x350,
		.power_mode_reverse	= false,
		.power_mode_detect	= true,
	},
	/* SUNXI_PCTL_HW_TYPE_10 */
	{
		.initial_bank_offset	= 0x80,
		.mux_regs_offset	= 0x0,
		.data_regs_offset	= 0x10,
		.data_set_regs_offset	= 0x14,
		.data_clr_regs_offset	= 0x18,
		.dlevel_regs_offset	= 0x20,
		.bank_mem_size		= 0x80,
		.pull_regs_offset	= 0x30,
		.dlevel_pins_per_reg	= 8,
		.dlevel_pins_bits	= 4,
		.dlevel_pins_mask	= 0xF,
		.irq_mux_val		= 0xE,
		.irq_cfg_reg		= 0xC0,
		.irq_ctrl_reg		= 0xD0,
		.irq_status_reg		= 0xD4,
		.irq_debounce_reg	= 0xD8,
		.irq_mem_base		= 0xC0,
		.irq_mem_size		= 0x80,
		.irq_mem_used		= 0x20,
		.power_mode_sel_reg	= 0x40,
		.power_mode_ctrl_reg	= 0x48,
		.power_mode_val_reg	= 0x48,
		.pio_pow_ctrl_reg	= 0x70,
		.auto_power_detect_mode_reverse = true,
	},
};
EXPORT_SYMBOL_GPL(sunxi_pinctrl_hw_info);

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
				if (!strcmp(func->name, func_name) &&
					(!func->variant ||
					func->variant & pctl->variant))
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
	return of_find_property(node, "bias-pull-up", NULL) ||
		of_find_property(node, "bias-pull-down", NULL) ||
		of_find_property(node, "bias-disable", NULL) ||
		of_find_property(node, "allwinner,pull", NULL);
}

static bool sunxi_pctrl_has_drive_prop(struct device_node *node)
{
	return of_find_property(node, "drive-strength", NULL) ||
		of_find_property(node, "allwinner,drive", NULL);
}

static bool sunxi_pctrl_has_power_source_prop(struct device_node *node)
{
	return of_find_property(node, "power-source", NULL);
}

static int sunxi_pctrl_parse_bias_prop(struct device_node *node)
{
	u32 val;

	/* Try the new style binding */
	if (of_find_property(node, "bias-pull-up", NULL))
		return PIN_CONFIG_BIAS_PULL_UP;

	if (of_find_property(node, "bias-pull-down", NULL))
		return PIN_CONFIG_BIAS_PULL_DOWN;

	if (of_find_property(node, "bias-disable", NULL))
		return PIN_CONFIG_BIAS_DISABLE;

	/* And fall back to the old binding */
	if (of_property_read_u32(node, "allwinner,pull", &val))
		return -EINVAL;

	switch (val) {
	case SUN4I_PINCTRL_NO_PULL:
		return PIN_CONFIG_BIAS_DISABLE;
	case SUN4I_PINCTRL_PULL_UP:
		return PIN_CONFIG_BIAS_PULL_UP;
	case SUN4I_PINCTRL_PULL_DOWN:
		return PIN_CONFIG_BIAS_PULL_DOWN;
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

	return (val + 1) * 10;
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

static int sunxi_pctrl_parse_power_source_prop(struct device_node *node)
{
	u32 val;

	if (!of_property_read_u32(node, "power-source", &val)) {
		if (val == 1800 || val == 3300)
			return val;
	}

	return -EINVAL;
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
	if (sunxi_pctrl_has_power_source_prop(node))
		configlen++;

	/*
	 * If we don't have any configuration, bail out
	 */
	if (!configlen)
		return NULL;

	pinconfig = kcalloc(configlen, sizeof(*pinconfig), GFP_KERNEL);
	if (!pinconfig)
		return ERR_PTR(-ENOMEM);

	if (sunxi_pctrl_has_drive_prop(node)) {
		int drive = sunxi_pctrl_parse_drive_prop(node);
		if (drive < 0) {
			ret = drive;
			goto err_free;
		}

		pinconfig[idx++] = pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH,
							  drive);
	}

	if (sunxi_pctrl_has_bias_prop(node)) {
		int pull = sunxi_pctrl_parse_bias_prop(node);
		int arg = 0;
		if (pull < 0) {
			ret = pull;
			goto err_free;
		}

		if (pull != PIN_CONFIG_BIAS_DISABLE)
			arg = 1; /* hardware uses weak pull resistors */

		pinconfig[idx++] = pinconf_to_config_packed(pull, arg);
	}

	if (sunxi_pctrl_has_power_source_prop(node)) {
		int power = sunxi_pctrl_parse_power_source_prop(node);
		if (power < 0) {
			ret = power;
			goto err_free;
		}

		pinconfig[idx++] = pinconf_to_config_packed(PIN_CONFIG_POWER_SOURCE,
							    power);
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
	const char *function, *pin_prop;
	const char *group;
	int ret, npins, nmaps, configlen = 0, i = 0;

	*map = NULL;
	*num_maps = 0;

	function = sunxi_pctrl_parse_function_prop(node);
	if (!function) {
		sunxi_err(pctl->dev, "missing function property in node %pOFn\n",
			node);
		return -EINVAL;
	}

	pin_prop = sunxi_pctrl_find_pins_prop(node, &npins);
	if (!pin_prop) {
		sunxi_err(pctl->dev, "missing pins property in node %pOFn\n",
			node);
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
	*map = kcalloc(nmaps, sizeof(**map), GFP_KERNEL);
	if (!*map)
		return -ENOMEM;

	pinconfig = sunxi_pctrl_build_pin_config(node, &configlen);
	if (IS_ERR(pinconfig)) {
		ret = PTR_ERR(pinconfig);
		goto err_free_map;
	}

	of_property_for_each_string(node, pin_prop, prop, group) {
		struct sunxi_pinctrl_group *grp =
			sunxi_pinctrl_find_group_by_name(pctl, group);

		if (!grp) {
			sunxi_err(pctl->dev, "unknown pin %s", group);
			continue;
		}

		if (!sunxi_pinctrl_desc_find_function_by_name(pctl,
							      grp->name,
							      function)) {
			sunxi_err(pctl->dev, "unsupported function %s on pin %s",
				function, group);
			continue;
		}

		(*map)[i].type = PIN_MAP_TYPE_MUX_GROUP;
		(*map)[i].data.mux.group = group;
		(*map)[i].data.mux.function = function;

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
	*map = krealloc(*map, i * sizeof(**map), GFP_KERNEL);
	if (!*map)
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

DEFINE_SPINLOCK(sun55iw3_pinctrl_lock);

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
static const u32 sun55iw3_bank_base[] = {
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('E', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
	SUNXI_BANK_OFFSET('H', 'A'),
	SUNXI_BANK_OFFSET('I', 'A'),
	SUNXI_BANK_OFFSET('J', 'A'),
	SUNXI_BANK_OFFSET('K', 'A'),
};
/* Describe which bank use vcc-io */
static const u32 sun55iw3_vccio_banks[] = {
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('H', 'A'),
};

static const u32 sun55iw3_r_bank_base[] = {
	SUNXI_BANK_OFFSET('L', 'L'),
	SUNXI_BANK_OFFSET('M', 'L'),
};

static u32 sun55iw3_vccio_sel_map(bool is_cpus_gpio, int bank)
{
	int i;

	if (is_cpus_gpio)
		return bank;

	for (i = 0; i < ARRAY_SIZE(sun55iw3_vccio_banks); i++) {
		if (sun55iw3_vccio_banks[i] == bank)
			return 12;
	}
	return bank;
}

static u32 sun55iw3_vccio_val_map(bool is_cpus_gpio, int bank)
{
	int i;

	if (is_cpus_gpio)
		return bank;

	for (i = 0; i < ARRAY_SIZE(sun55iw3_vccio_banks); i++) {
		if (sun55iw3_vccio_banks[i] == bank)
			return 16;
	}
	return bank;
}

void sun55iw3_pinctrl_fix_over_voltage(bool is_cpus_gpio)
{
	u32 mod_val, mod_sel, val, bank, i, vccio_sel_bank, vccio_val_bank, reg;
	unsigned long flags;
	int cur_uV, mod_uV, banks;
	void __iomem *membase;
	const u32 *bank_base;
	u32 power_mode_sel_reg, power_mode_val_reg;
	static u32 power_mon_print_once, r_power_mon_print_once;

	if (is_cpus_gpio) {
		membase = ioremap(0x7022000, 0x800);
		banks = ARRAY_SIZE(sun55iw3_r_bank_base);
		bank_base = sun55iw3_r_bank_base;

		power_mode_sel_reg = 0x340;
		power_mode_val_reg = 0x348;
	} else {
		membase = ioremap(0x2000000, 0x800);
		banks = ARRAY_SIZE(sun55iw3_bank_base);
		bank_base = sun55iw3_bank_base;

		power_mode_sel_reg = 0x380;
		power_mode_val_reg = 0x388;
	}


	mod_val = readl(membase + power_mode_val_reg);
	mod_sel = readl(membase + power_mode_sel_reg);

	for (i = 0; i < banks; i++) {
		bank = bank_base[i];

		/* Skip PF */
		if (i == 4)
			continue;

		vccio_sel_bank = sun55iw3_vccio_sel_map(is_cpus_gpio, bank);
		vccio_val_bank = sun55iw3_vccio_val_map(is_cpus_gpio, bank);

		mod_uV = mod_sel & BIT(vccio_sel_bank) ? 3300000 : 1800000;

		cur_uV = mod_val & BIT(vccio_val_bank) ? 1800000 : 3300000;

		if (is_cpus_gpio)
			sunxi_debug(NULL, "bank[%c] power_val=%d, power_sel=%d\n", 'L' + bank, cur_uV, mod_uV);
		else
			sunxi_debug(NULL, "bank[%c] power_val=%d, power_sel=%d\n", 'A' + bank, cur_uV, mod_uV);

		if (cur_uV > mod_uV) {
			if (is_cpus_gpio)
				sunxi_warn(NULL, "bank[%c] over voltage, we will correct it\n", 'L' + bank);
			else
				sunxi_warn(NULL, "bank[%c] over voltage, we will correct it\n", 'A' + bank);
			/* We always set power sel 3.3v */
			val = 1;
			/* Set withstand voltage value */
			spin_lock_irqsave(&sun55iw3_pinctrl_lock, flags);
			reg = readl(membase + power_mode_sel_reg);
			reg &= ~BIT(vccio_sel_bank);
			writel(reg | (val << vccio_sel_bank), membase + power_mode_sel_reg);
			spin_unlock_irqrestore(&sun55iw3_pinctrl_lock, flags);
		} else if (cur_uV < mod_uV) {
			/* We print power mode warning once for every bank */
			if (is_cpus_gpio) {
				if (!(r_power_mon_print_once & BIT(bank))) {
					sunxi_warn(NULL, "bank[%c] power mode not correct, power_val=%d, power_sel=%d\n",
							'L' + bank, cur_uV, mod_uV);
					r_power_mon_print_once |= BIT(bank);
				}
			} else {
				if (!(power_mon_print_once & BIT(bank))) {
					sunxi_warn(NULL, "bank[%c] power mode not correct, power_val=%d, power_sel=%d\n",
							'A' + bank, cur_uV, mod_uV);
					power_mon_print_once |= BIT(bank);
				}
			}
		} else {
			/* Do nothing */
		}
	}

	iounmap(membase);
}
EXPORT_SYMBOL_GPL(sun55iw3_pinctrl_fix_over_voltage);
#endif

static inline u32 sunxi_pintctrl_vccio_ctrl_map(struct sunxi_pinctrl *pctl, u32 bank)
{
	enum sunxi_pinctrl_hw_type hw_type = pctl->desc->hw_type;
	const struct sunxi_pinctrl_desc *desc = pctl->desc;
	u32 mode_ctrl_vccio_bit = sunxi_pinctrl_hw_info[hw_type].mode_ctrl_vccio_bit;
	int i;

	for (i = 0; i < desc->vccio_nbanks; i++) {
		if (desc->vccio_banks[i] == bank) {
			sunxi_debug(NULL, "vccio-ctrl-map: bank[%d] use vccio, map to bank[%d]\n",
				    bank, mode_ctrl_vccio_bit);
			return mode_ctrl_vccio_bit;
		}
	}
	return bank;
}

static inline u32 sunxi_pintctrl_vccio_sel_map(struct sunxi_pinctrl *pctl, u32 bank)
{
	enum sunxi_pinctrl_hw_type hw_type = pctl->desc->hw_type;
	const struct sunxi_pinctrl_desc *desc = pctl->desc;
	u32 mode_sel_vccio_bit = sunxi_pinctrl_hw_info[hw_type].mode_sel_vccio_bit;
	int i;

	for (i = 0; i < desc->vccio_nbanks; i++) {
		if (desc->vccio_banks[i] == bank) {
			sunxi_debug(NULL, "vccio-sel-map: bank[%d] use vccio, map to bank[%d]\n",
				    bank, mode_sel_vccio_bit);
			return mode_sel_vccio_bit;
		}
	}
	return bank;
}

static inline u32 sunxi_pintctrl_vccio_val_map(struct sunxi_pinctrl *pctl, u32 bank)
{
	enum sunxi_pinctrl_hw_type hw_type = pctl->desc->hw_type;
	const struct sunxi_pinctrl_desc *desc = pctl->desc;
	u32 mode_val_vccio_bit = sunxi_pinctrl_hw_info[hw_type].mode_val_vccio_bit;
	int i;

	for (i = 0; i < desc->vccio_nbanks; i++) {
		if (desc->vccio_banks[i] == bank) {
			sunxi_debug(NULL, "vccio-val-map: bank[%d] use vccio, map to bank[%d]\n",
				    bank, mode_val_vccio_bit);
			return mode_val_vccio_bit;
		}
	}
	return bank;
}

static int sunxi_pinctrl_set_io_bias_cfg(struct sunxi_pinctrl *pctl,
					 unsigned pin,
					 struct regulator *supply)
{
	unsigned short bank;
	unsigned long flags, g_flags;
	u32 val, reg, vccio_ctrl_bank, vccio_sel_bank;
	int uV;
	enum sunxi_pinctrl_hw_type hw_type = pctl->desc->hw_type;
	bool power_mode_detect = sunxi_pinctrl_hw_info[hw_type].power_mode_detect;
	u32 power_mode_sel_reg = sunxi_pinctrl_hw_info[hw_type].power_mode_sel_reg;
	u32 power_mode_ctrl_reg = sunxi_pinctrl_hw_info[hw_type].power_mode_ctrl_reg;
	bool power_mode_reverse = sunxi_pinctrl_hw_info[hw_type].power_mode_reverse;

	pin -= pctl->desc->pin_base;
	bank = pin / PINS_PER_BANK;

	vccio_sel_bank = sunxi_pintctrl_vccio_sel_map(pctl, bank);
	vccio_ctrl_bank = sunxi_pintctrl_vccio_ctrl_map(pctl, bank);

	if (!pctl->desc->io_bias_cfg_variant)
		return 0;

	if (power_mode_detect) {
		/*
		 * Read the GPIO_POW_VAL register to set GPIO_POW_MOD_SEL
		 * 0: bias = 3.3V or bias >= 2.5V
		 * 1: bias = 1.8V or bias <= 2.0V
		 */
		reg = readl(pctl->membase + sunxi_pinctrl_hw_info[hw_type].power_mode_val_reg);
		if (reg & BIT(sunxi_pintctrl_vccio_val_map(pctl, bank)))
			uV = 1800000;
		else
			uV = 3300000;
	} else {
		uV = regulator_get_voltage(supply);
		if (uV < 0)
			return uV;
	}

	/* Might be dummy regulator with no voltage set */
	if (uV == 0)
		return 0;

	switch (pctl->desc->io_bias_cfg_variant) {
	case BIAS_VOLTAGE_GRP_CONFIG:
		/*
		 * Configured value must be equal or greater to actual
		 * voltage.
		 */
		if (uV <= 1800000)
			val = 0x0; /* 1.8V */
		else if (uV <= 2500000)
			val = 0x6; /* 2.5V */
		else if (uV <= 2800000)
			val = 0x9; /* 2.8V */
		else if (uV <= 3000000)
			val = 0xA; /* 3.0V */
		else
			val = 0xD; /* 3.3V */

		reg = readl(pctl->membase + sunxi_grp_config_reg(pin));
		reg &= ~IO_BIAS_MASK;
		writel(reg | val, pctl->membase + sunxi_grp_config_reg(pin));
		return 0;
	case BIAS_VOLTAGE_PIO_POW_MODE_SEL:
	case BIAS_VOLTAGE_PIO_POW_MODE_CTL:
		val = uV <= 1800000 ? 1 : 0;

		raw_spin_lock_irqsave(&pctl->lock, flags);
		reg = readl(pctl->membase + power_mode_sel_reg);
		reg &= ~(1 << bank);
		writel(reg | val << bank, pctl->membase + power_mode_sel_reg);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);

		if (pctl->desc->io_bias_cfg_variant ==
		    BIAS_VOLTAGE_PIO_POW_MODE_SEL)
			return 0;

		val = (1800000 < uV && uV <= 2500000) ? 1 : 0;

		raw_spin_lock_irqsave(&pctl->lock, flags);
		reg = readl(pctl->membase + power_mode_ctrl_reg);
		reg &= ~BIT(bank);
		writel(reg | val << bank, pctl->membase + power_mode_ctrl_reg);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);
		return 0;
	case BIAS_VOLTAGE_PIO_POW_MODE_CTL_V2:

		if (power_mode_reverse)
			val = uV <= 1800000 ? 0 : 1;
		else
			val = uV <= 1800000 ? 1 : 0;

		/* Set withstand voltage value */
		spin_lock_irqsave(&sun55iw3_pinctrl_lock, g_flags);
		raw_spin_lock_irqsave(&pctl->lock, flags);
		reg = readl(pctl->membase + power_mode_sel_reg);
		reg &= ~BIT(vccio_sel_bank);
		writel(reg | val << vccio_sel_bank, pctl->membase + power_mode_sel_reg);

		raw_spin_unlock_irqrestore(&pctl->lock, flags);
		spin_unlock_irqrestore(&sun55iw3_pinctrl_lock, g_flags);

		sunxi_debug(NULL, "!pf-withstand: bank[%d-%d]=%duV set modesel[0x%x=0x%x]\n",
			    bank, vccio_sel_bank, uV, power_mode_sel_reg,
			    readl(pctl->membase + power_mode_sel_reg));
		val = 1;

		/* Disable self-adaption */
		raw_spin_lock_irqsave(&pctl->lock, flags);
		reg = readl(pctl->membase + power_mode_ctrl_reg);
		reg &= ~BIT(vccio_ctrl_bank);
		writel(reg | val << vccio_ctrl_bank, pctl->membase + power_mode_ctrl_reg);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);
		sunxi_debug(NULL, "!pf-withstand: bank[%d-%d] set modectrl[0x%x=0x%x]\n",
			    bank, vccio_ctrl_bank, power_mode_ctrl_reg, readl(pctl->membase + power_mode_ctrl_reg));
		return 0;

	default:
		return -EINVAL;
	}
}

static int sunxi_pinctrl_get_debounce_div(struct clk *clk, int freq, int *diff)
{
	unsigned long clock = clk_get_rate(clk);
	unsigned int best_diff, best_div;
	int i;

	best_diff = abs(freq - clock);
	best_div = 0;

	for (i = 1; i < 8; i++) {
		int cur_diff = abs(freq - (clock >> i));

		if (cur_diff < best_diff) {
			best_diff = cur_diff;
			best_div = i;
		}
	}

	*diff = best_diff;
	return best_div;
}

static unsigned int sunxi_pinctrl_get_debounce_param(struct sunxi_pinctrl *pctl, unsigned debounce)
{
	u8 div, src;
	struct clk *hosc, *losc;
	unsigned long debounce_freq;
	unsigned int hosc_diff, losc_diff;
	unsigned int hosc_div, losc_div;

	losc = devm_clk_get(pctl->dev, "losc");
	if (IS_ERR(losc))
		return PTR_ERR(losc);

	hosc = devm_clk_get(pctl->dev, "hosc");
	if (IS_ERR(hosc))
		return PTR_ERR(hosc);

	debounce_freq = DIV_ROUND_CLOSEST(NSEC_PER_SEC, debounce);
	losc_div = sunxi_pinctrl_get_debounce_div(losc,
			debounce_freq,
			&losc_diff);

	hosc_div = sunxi_pinctrl_get_debounce_div(hosc,
			debounce_freq,
			&hosc_diff);

	if (hosc_diff < losc_diff) {
		div = hosc_div;
		src = 1;
	} else {
		div = losc_div;
		src = 0;
	}

	return src | div << 4;
}

static int sunxi_pconf_reg(unsigned pin, enum pin_config_param param,
			   u32 *offset, u32 *shift, u32 *mask,
			   struct sunxi_pinctrl *pctl)
{
	unsigned short bank = pin / PINS_PER_BANK;
	enum sunxi_pinctrl_hw_type hw_type = pctl->desc->hw_type;

	switch ((int)param) {
	case PIN_CONFIG_DRIVE_STRENGTH:
		*offset = sunxi_dlevel_reg(pin, hw_type);
		*shift = sunxi_dlevel_offset(pin, hw_type);
		*mask = sunxi_pinctrl_hw_info[hw_type].dlevel_pins_mask;
		break;

	case PIN_CONFIG_BIAS_PULL_UP:
	case PIN_CONFIG_BIAS_PULL_DOWN:
	case PIN_CONFIG_BIAS_DISABLE:
		*offset = sunxi_pull_reg(pin, hw_type);
		*shift = sunxi_pull_offset(pin);
		*mask = PULL_PINS_MASK;
		break;

	case PIN_CONFIG_POWER_SOURCE:
		/* As SDIO pin, PF needs voltage switching function. */
		if (bank != 5)
			return -ENOTSUPP;

		*offset = sunxi_pinctrl_hw_info[hw_type].pio_pow_ctrl_reg;
		*shift = 0;
		*mask = POWER_SOURCE_MASK;
		break;

	case PIN_CONFIG_INPUT_DEBOUNCE:
		*offset = sunxi_pinctrl_hw_info[hw_type].irq_debounce_reg +
				bank * sunxi_pinctrl_hw_info[hw_type].irq_mem_size;
		*shift = 0;
		*mask = IRQ_DEBOUNCE_MASK;
		break;

#if IS_ENABLED(CONFIG_AW_PINCTRL_DEBUGFS)
	case SUNXI_PINCFG_TYPE_DAT:
		*offset = sunxi_data_reg(pin, hw_type);
		*shift = sunxi_data_offset(pin);
		*mask = DATA_PINS_MASK;
		break;

	case SUNXI_PINCFG_TYPE_FUNC:
		*offset = sunxi_mux_reg(pin, hw_type);
		*shift = sunxi_mux_offset(pin);
		*mask = MUX_PINS_MASK;
		break;
#endif

	default:
		return -ENOTSUPP;
	}

	return 0;
}

static int sunxi_pconf_get(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *config)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	enum pin_config_param param = pinconf_to_config_param(*config);
	u32 offset, shift, mask, val;
	u16 arg;
	int ret;

	pin -= pctl->desc->pin_base;

	ret = sunxi_pconf_reg(pin, param, &offset, &shift, &mask, pctl);
	if (ret < 0)
		return ret;

	val = (readl(pctl->membase + offset) >> shift) & mask;

	switch ((int)param) {
	case PIN_CONFIG_DRIVE_STRENGTH:
		arg = (val + 1) * 10;
		break;

	case PIN_CONFIG_BIAS_PULL_UP:
		if (val != SUN4I_PINCTRL_PULL_UP)
			return -EINVAL;
		arg = 1; /* hardware is weak pull-up */
		break;

	case PIN_CONFIG_BIAS_PULL_DOWN:
		if (val != SUN4I_PINCTRL_PULL_DOWN)
			return -EINVAL;
		arg = 2; /* hardware is weak pull-down */
		break;

	case PIN_CONFIG_BIAS_DISABLE:
		if (val != SUN4I_PINCTRL_NO_PULL)
			return -EINVAL;
		arg = 0;
		break;

	case PIN_CONFIG_POWER_SOURCE:
		arg = val ? 3300 : 1800;
		break;

	case PIN_CONFIG_INPUT_DEBOUNCE:
		arg = val;
		break;

#if IS_ENABLED(CONFIG_AW_PINCTRL_DEBUGFS)
	case SUNXI_PINCFG_TYPE_DAT:
	case SUNXI_PINCFG_TYPE_FUNC:
		arg = val;
		break;
#endif
	default:
		/* sunxi_pconf_reg should catch anything unsupported */
		WARN_ON(1);
		return -ENOTSUPP;
	}

	*config = pinconf_to_config_packed(param, arg);

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

static void sunxi_power_auto_switch_pf(struct sunxi_pinctrl *pctl, unsigned pin, u32 target_mV)
{
	u32 val, reg;
	u32 current_mV;
	unsigned long flags;
	unsigned short bank = pin / PINS_PER_BANK;
	enum sunxi_pinctrl_hw_type hw_type = pctl->desc->hw_type;
	void __iomem *pow_val_addr = pctl->membase + sunxi_pinctrl_hw_info[hw_type].power_mode_val_reg;
	u32 pio_pow_ctrl_reg = sunxi_pinctrl_hw_info[hw_type].pio_pow_ctrl_reg;
	u32 power_mode_val_reg = sunxi_pinctrl_hw_info[hw_type].power_mode_val_reg;
	bool auto_power_detect_mode_reverse = sunxi_pinctrl_hw_info[hw_type].auto_power_detect_mode_reverse;

	current_mV = readl(pctl->membase + pio_pow_ctrl_reg);
	current_mV = current_mV == 0 ? 1800 : 3300;

	sunxi_debug(NULL, "pf-switch to %d from %d\n", target_mV, current_mV);
	if (current_mV < target_mV) { /* Increase the voltage */
		raw_spin_lock_irqsave(&pctl->lock, flags);
		val = 1;
		writel(val, pctl->membase + pio_pow_ctrl_reg);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);

		/* Wait for voltage increasing. Double check! */
		if (auto_power_detect_mode_reverse)
			WARN_ON(readl_relaxed_poll_timeout(pow_val_addr, reg, !(reg & BIT(bank * 2)), 100, 10000));
		else
			WARN_ON(readl_relaxed_poll_timeout(pow_val_addr, reg, reg & BIT(bank * 2), 100, 10000));

		udelay(10);

		sunxi_debug(NULL, "pf-switch-increase: Wait for voltage increase done[0x%x=0x%x] pf[0x%x=0x%x]\n",
				power_mode_val_reg, readl(pctl->membase + power_mode_val_reg),
				pio_pow_ctrl_reg, readl(pctl->membase + pio_pow_ctrl_reg));

		/* Increase the voltage, reg value should be 3.3v */
		if (auto_power_detect_mode_reverse) {
			/* 0 --> 3.3	1 --> 1.8 */
			if (readl(pow_val_addr) & BIT(bank * 2))
				panic("PF voltage switching failed, please be aware!");
		} else {
			/* 0 --> 1.8	1 --> 3.3 */
			if (!(readl(pow_val_addr) & BIT(bank * 2)))
				panic("PF voltage switching failed, please be aware!");
		}
	} else if (current_mV > target_mV) { /* Decrease the voltage */
		raw_spin_lock_irqsave(&pctl->lock, flags);
		/* Decrease the voltage */
		val = 0;
		writel(val, pctl->membase + pio_pow_ctrl_reg);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);

		/* Wait for voltage decreasing. Double check! */
		if (auto_power_detect_mode_reverse)
			WARN_ON(readl_relaxed_poll_timeout(pow_val_addr, reg, reg & BIT(bank * 2), 100, 10000));
		else
			WARN_ON(readl_relaxed_poll_timeout(pow_val_addr, reg, !(reg & BIT(bank * 2)), 100, 10000));

		udelay(10);

		sunxi_debug(NULL, "pf-switch-decrease: Wait for voltage decrease done[0x%x=0x%x] pf[0x%x=0x%x]\n",
				power_mode_val_reg, readl(pctl->membase + power_mode_val_reg),
				pio_pow_ctrl_reg, readl(pctl->membase + pio_pow_ctrl_reg));

		/* Decrease the voltage, reg value should be 1.8v */
		if (auto_power_detect_mode_reverse) {
			/* 0 --> 3.3	1 --> 1.8 */
			if (!(readl(pow_val_addr) & BIT(bank * 2)))
				panic("PF voltage switching failed, please be aware!");
		} else {
			/* 0 --> 1.8	1 --> 3.3 */
			if (readl(pow_val_addr) & BIT(bank * 2))
				panic("PF voltage switching failed, please be aware!");
		}
	}
	sunxi_debug(NULL, "pf-switch to %d from %d ok\n", target_mV, current_mV);
}
static void sunxi_power_switch_pf(struct sunxi_pinctrl *pctl, unsigned pin, u32 target_mV)
{
	u32 val, reg;
	u32 current_mV;
	unsigned long flags, g_flags;
	unsigned short bank = pin / PINS_PER_BANK;
	enum sunxi_pinctrl_hw_type hw_type = pctl->desc->hw_type;
	void __iomem *pow_val_addr = pctl->membase + sunxi_pinctrl_hw_info[hw_type].power_mode_val_reg;
	bool power_mode_reverse = sunxi_pinctrl_hw_info[hw_type].power_mode_reverse;
	u32 power_mode_sel_reg = sunxi_pinctrl_hw_info[hw_type].power_mode_sel_reg;
	u32 power_mode_ctrl_reg = sunxi_pinctrl_hw_info[hw_type].power_mode_ctrl_reg;
	u32 pio_pow_ctrl_reg = sunxi_pinctrl_hw_info[hw_type].pio_pow_ctrl_reg;
	u32 power_mode_val_reg = sunxi_pinctrl_hw_info[hw_type].power_mode_val_reg;

	current_mV = readl(pctl->membase + pio_pow_ctrl_reg);
	current_mV = current_mV == 0 ? 1800 : 3300;

	sunxi_debug(NULL, "pf-switch to %d from %d\n", target_mV, current_mV);
	if (current_mV < target_mV) { /* Increase the voltage */
		if (power_mode_reverse)
			val = 1;
		else
			val = 0;

		spin_lock_irqsave(&sun55iw3_pinctrl_lock, g_flags);
		raw_spin_lock_irqsave(&pctl->lock, flags);
		/* 1.Set withstand voltage value */
		reg = readl(pctl->membase + power_mode_sel_reg);
		reg &= ~(1 << bank);
		writel(reg | val << bank, pctl->membase + power_mode_sel_reg);
		sunxi_debug(NULL, "pf-switch-increase: 1.Set withstand voltage value[0x%x=0x%x]\n",
			    power_mode_sel_reg, readl(pctl->membase + power_mode_sel_reg));
		/* Disable self-adaption */
		val = 1;
		reg = readl(pctl->membase + power_mode_ctrl_reg);
		reg &= ~BIT(bank);
		writel(reg | val << bank, pctl->membase + power_mode_ctrl_reg);
		sunxi_debug(NULL, "pf-switch-increase: 2.Disable self-adaption[0x%x=0x%x]\n",
			    power_mode_ctrl_reg, readl(pctl->membase + power_mode_ctrl_reg));

		/* Increase the voltage */
		val = 1;
		writel(val, pctl->membase + pio_pow_ctrl_reg);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);
		spin_unlock_irqrestore(&sun55iw3_pinctrl_lock, g_flags);

		/* Wait for voltage increasing. Double check! */
		WARN_ON(readl_relaxed_poll_timeout(pow_val_addr, reg, !(reg & BIT(bank)), 100, 10000));
		udelay(10);
		if (readl(pow_val_addr) & BIT(bank))
			panic("PF voltage switching failed, please be aware!");

		sunxi_debug(NULL, "pf-switch-increase: 3.Wait for voltage increase done[0x%x=0x%x]\n",
			    power_mode_val_reg, readl(pctl->membase + power_mode_val_reg));
	} else if (current_mV > target_mV) { /* Decrease the voltage */
		raw_spin_lock_irqsave(&pctl->lock, flags);
		/* Decrease the voltage */
		val = 0;
		writel(val, pctl->membase + pio_pow_ctrl_reg);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);

		/* Wait for voltage decreasing. Double check! */
		WARN_ON(readl_relaxed_poll_timeout(pow_val_addr, reg, reg & BIT(bank), 100, 10000));
		udelay(10);
		if (!(readl(pow_val_addr) & BIT(bank)))
			panic("PF voltage switching failed, please be aware!");

		sunxi_debug(NULL, "pf-switch-decrease: 1.Wait for voltage decrease done[0x%x=0x%x]\n",
			    power_mode_val_reg, readl(pctl->membase + power_mode_val_reg));

		spin_lock_irqsave(&sun55iw3_pinctrl_lock, g_flags);
		raw_spin_lock_irqsave(&pctl->lock, flags);
		if (power_mode_reverse)
			val = 0;
		else
			val = 1;

		/* 2.Set withstand voltage value */
		reg = readl(pctl->membase + power_mode_sel_reg);
		reg &= ~BIT(bank);
		writel(reg | val << bank, pctl->membase + power_mode_sel_reg);

		sunxi_debug(NULL, "pf-switch-decrease: 2.Set withstand voltage value[0x%x=0x%x]\n",
			    power_mode_sel_reg, readl(pctl->membase + power_mode_sel_reg));
		/* 3.Disable self-adaption */
		val = 1;
		reg = readl(pctl->membase + power_mode_ctrl_reg);
		reg &= ~BIT(bank);
		writel(reg | val << bank, pctl->membase + power_mode_ctrl_reg);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);
		spin_unlock_irqrestore(&sun55iw3_pinctrl_lock, g_flags);

		sunxi_debug(NULL, "pf-switch-decrease: 3.Disable self-adaption[0x%x=0x%x]\n",
			    power_mode_ctrl_reg, readl(pctl->membase + power_mode_ctrl_reg));
	}
	sunxi_debug(NULL, "pf-switch to %d from %d ok\n", target_mV, current_mV);
}

static int sunxi_pconf_set(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *configs, unsigned num_configs)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	unsigned short bank_offset;
	struct sunxi_pinctrl_regulator *s_reg;
	int i;

	pin -= pctl->desc->pin_base;
	bank_offset = pin / PINS_PER_BANK;
	s_reg = &pctl->regulators[bank_offset];

	for (i = 0; i < num_configs; i++) {
		enum pin_config_param param;
		unsigned long flags;
		u32 offset, shift, mask, reg;
		u32 arg, val;
		int ret;

		param = pinconf_to_config_param(configs[i]);
		arg = pinconf_to_config_argument(configs[i]);

		ret = sunxi_pconf_reg(pin, param, &offset, &shift, &mask, pctl);
		if (ret < 0)
			return ret;

		switch ((int)param) {
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
			if (arg == 0)
				return -EINVAL;
			val = 1;
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			if (arg == 0)
				return -EINVAL;
			val = 2;
			break;
		case PIN_CONFIG_POWER_SOURCE:
			if (arg != 1800 && arg != 3300)
				return -EINVAL;

			/*
			 * Only PF port as SDIO supports power source setting,
			 * configure pio group withstand voltage mode for PF.
			 */
			if (pctl->desc->auto_power_source_switch)
				sunxi_power_auto_switch_pf(pctl, pin, arg);
			else
				sunxi_power_switch_pf(pctl, pin, arg);

			break;
		case PIN_CONFIG_INPUT_DEBOUNCE:
			if (arg == 0)
				return -EINVAL;
			val = sunxi_pinctrl_get_debounce_param(pctl, arg);
			break;
#if IS_ENABLED(CONFIG_AW_PINCTRL_DEBUGFS)
		case SUNXI_PINCFG_TYPE_DAT:
			val = arg;
			break;
		case SUNXI_PINCFG_TYPE_FUNC:
			val = arg;
			break;
#endif
		default:
			/* sunxi_pconf_reg should catch anything unsupported */
			WARN_ON(1);
			return -ENOTSUPP;
		}

		if (param == PIN_CONFIG_POWER_SOURCE)
			continue;

		raw_spin_lock_irqsave(&pctl->lock, flags);
		reg = readl(pctl->membase + offset);
		reg &= ~(mask << shift);
		writel(reg | val << shift, pctl->membase + offset);
		raw_spin_unlock_irqrestore(&pctl->lock, flags);
	} /* for each config */

	return 0;
}

static int sunxi_pconf_group_set(struct pinctrl_dev *pctldev, unsigned group,
				 unsigned long *configs, unsigned num_configs)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct sunxi_pinctrl_group *g = &pctl->groups[group];

	/* We only support 1 pin per group. Chain it to the pin callback */
	return sunxi_pconf_set(pctldev, g->pin, configs, num_configs);
}

static const struct pinconf_ops sunxi_pconf_ops = {
	.is_generic		= true,
	.pin_config_get		= sunxi_pconf_get,
	.pin_config_set		= sunxi_pconf_set,
	.pin_config_group_get	= sunxi_pconf_group_get,
	.pin_config_group_set	= sunxi_pconf_group_set,
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
	val = readl(pctl->membase + sunxi_mux_reg(pin, pctl->desc->hw_type));
	mask = MUX_PINS_MASK << sunxi_mux_offset(pin);
	writel((val & ~mask) | config << sunxi_mux_offset(pin),
		pctl->membase + sunxi_mux_reg(pin, pctl->desc->hw_type));

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
	__maybe_unused u32 index = 0, regval = 0;

	if (!desc)
		return -EINVAL;

#ifdef SUNXI_PINCTRL_I2S0_ROUTE_PAD
	if (pctl->pad_addr[0].addr && !strncmp(func->name, pctl->pad_addr[0].name, strlen(func->name))) {
		if (g->pin > PL_BASE) {
			sunxi_debug(NULL, "i2s0 change to r_pio!\n");
			writel(regval & BIT(index), pctl->pad_addr[0].addr);
		} else {
			sunxi_debug(NULL, "i2s0 change to pio!\n");
			writel(regval | BIT(index), pctl->pad_addr[0].addr);
		}
	}

	if (pctl->pad_addr[1].addr && !strncmp(func->name, pctl->pad_addr[1].name, strlen(func->name))) {
		if (g->pin > PL_BASE) {
			sunxi_debug(NULL, "dmic change to r_pio!\n");
			writel(regval & BIT(index), pctl->pad_addr[1].addr);
		} else {
			sunxi_debug(NULL, "dmic change to pio!\n");
			writel(regval | BIT(index), pctl->pad_addr[1].addr);
		}
	}
#endif

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

static int sunxi_pmx_request(struct pinctrl_dev *pctldev, unsigned offset)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	unsigned short bank = offset / PINS_PER_BANK;
	unsigned short bank_offset = bank - pctl->desc->pin_base /
					    PINS_PER_BANK;
	struct sunxi_pinctrl_regulator *s_reg = &pctl->regulators[bank_offset];
	struct regulator *reg = s_reg->regulator;
	struct regulator *reg_op = s_reg->regulator_optional;
	char supply[16];
	char supply_name[16];
	int ret;

	if (pctl->desc->auto_power_source_switch) {
		sunxi_info_once(pctl->dev, "Auto power withstand voltage configuration detected, automatically exit!\n");
		return 0;
	}

	if (refcount_read(&s_reg->refcount)) {
		sunxi_debug(pctl->dev, "bank P%c regulator has been opened\n",
			'A' + bank);
		refcount_inc(&s_reg->refcount);
		return 0;
	}

	/*
	 * We should only call regulator_get when a bank is first requested -
	 * If we call regulator_get here every time, the DPM list will be
	 * corrupted. The calling chain:
	 *
	   device.suspend
	   |
	   V
	   pinctrl_select_state(default)
	   |
	   V
	   pinctrl_commit_state
	   |
	   V
	   pinmux_enable_setting
	   |
	   V
	   pin_request
	   |
	   V
	   sunxi_pmx_request
	   |
	   V
	   regulator_get
	   |
	   V
	   device_link_add
	   |
	   V
	   device_reorder_to_tail
	   |
	   V
	   device_pm_move_last
	   |
	   V
	   mv dev & child dev ->power.entry to dpm_list last
	 */

	if (IS_ERR_OR_NULL(reg)) {
		snprintf(supply, sizeof(supply), "vcc-p%c", 'a' + bank);
		reg = regulator_get(pctl->dev, supply);
		if (IS_ERR(reg)) {
			sunxi_err(pctl->dev, "Couldn't get bank P%c regulator\n",
				'A' + bank);
			return PTR_ERR(reg);
		}

		snprintf(supply_name, sizeof(supply_name), "%s-supply", supply);
		if ((of_property_read_bool(pctl->dev->of_node, supply_name))
		     && sunxi_pinctrl_hw_info[pctl->desc->hw_type].power_mode_detect)
			sunxi_warn(pctl->dev,
				   "Dts property 'vcc-p%c=*' takes no effect on gpio's withstand voltage settings",
				   'a' + bank);
	}

	if (pctl->desc->pf_power_source_switch && bank == 5 && IS_ERR_OR_NULL(reg_op)) {
		reg_op = regulator_get(pctl->dev, "vcc-pfo");
		if (IS_ERR(reg_op)) {
			sunxi_err(pctl->dev,
				"Couldn't get bank PF optional regulator\n");
			ret = PTR_ERR(reg_op);
			goto out_reg;
		}
	}

	ret = regulator_enable(reg);
	if (ret) {
		sunxi_err(pctl->dev,
			"Couldn't enable bank P%c regulator\n", 'A' + bank);
		goto out_reg_op;
	}

	if (pctl->desc->pf_power_source_switch && bank == 5) {
		ret = regulator_enable(reg_op);
		if (ret) {
			sunxi_err(pctl->dev,
				"Couldn't enable bank PF optional regulator\n");
			goto out_dis;
		}
	}

	/* Skip bank PF because we don't know which voltage to use now */
	if (!(pctl->desc->pf_power_source_switch && bank == 5) && !(pctl->desc->auto_power_source_switch))
		sunxi_pinctrl_set_io_bias_cfg(pctl, offset, reg);

	s_reg->regulator = reg;
	s_reg->regulator_optional = reg_op;
	refcount_set(&s_reg->refcount, 1);

	return 0;

out_dis:
	regulator_disable(reg);
out_reg_op:
	regulator_put(reg_op);
out_reg:
	regulator_put(reg);

	return ret;
}

static int sunxi_pmx_free(struct pinctrl_dev *pctldev, unsigned offset)
{
	struct sunxi_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	unsigned short bank = offset / PINS_PER_BANK;
	unsigned short bank_offset = bank - pctl->desc->pin_base /
					    PINS_PER_BANK;
	struct sunxi_pinctrl_regulator *s_reg = &pctl->regulators[bank_offset];

	if (pctl->desc->auto_power_source_switch) {
		sunxi_info_once(pctl->dev, "Auto power withstand voltage configuration detected, automatically exit!\n");
		return 0;
	}

	if (!refcount_dec_and_test(&s_reg->refcount))
		return 0;

	if (s_reg->regulator_optional)
		regulator_disable(s_reg->regulator_optional);
	regulator_disable(s_reg->regulator);

	return 0;
}

static const struct pinmux_ops sunxi_pmx_ops = {
	.get_functions_count	= sunxi_pmx_get_funcs_cnt,
	.get_function_name	= sunxi_pmx_get_func_name,
	.get_function_groups	= sunxi_pmx_get_func_groups,
	.set_mux		= sunxi_pmx_set_mux,
	.gpio_set_direction	= sunxi_pmx_gpio_set_direction,
	.request		= sunxi_pmx_request,
	.free			= sunxi_pmx_free,
	.strict			= true,
};

static int sunxi_pinctrl_gpio_direction_input(struct gpio_chip *chip,
					unsigned offset)
{
	return pinctrl_gpio_direction_input(chip->base + offset);
}

static int sunxi_pinctrl_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct sunxi_pinctrl *pctl = gpiochip_get_data(chip);
	u32 reg = sunxi_data_reg(offset, pctl->desc->hw_type);
	u8 index = sunxi_data_offset(offset);
	bool set_mux = pctl->desc->irq_read_needs_mux &&
		gpiochip_line_is_irq(chip, offset);
	u32 pin = offset + chip->base;
	u32 val;

	if (set_mux)
		sunxi_pmx_set(pctl->pctl_dev, pin, SUN4I_FUNC_INPUT);

	val = (readl(pctl->membase + reg) >> index) & DATA_PINS_MASK;

	if (set_mux)
		sunxi_pmx_set(pctl->pctl_dev, pin, sunxi_pinctrl_hw_info[pctl->desc->hw_type].irq_mux_val);

	return !!val;
}

static void sunxi_pinctrl_gpio_set(struct gpio_chip *chip,
				unsigned offset, int value)
{
	struct sunxi_pinctrl *pctl = gpiochip_get_data(chip);
	enum sunxi_pinctrl_hw_type hw_type = pctl->desc->hw_type;
	u32 reg = sunxi_data_reg(offset, pctl->desc->hw_type);
	u8 index = sunxi_data_offset(offset);
	unsigned long flags;
	u32 regval, set_reg, clr_reg, set_regval, clr_regval;

	raw_spin_lock_irqsave(&pctl->lock, flags);

	sunxi_debug(pctl->dev, "data_set_mode_select is %d", sunxi_pinctrl_hw_info[hw_type].data_set_mode_select);
	if (sunxi_pinctrl_hw_info[hw_type].data_set_mode_select) {
		set_reg = sunxi_set_data_reg(offset, pctl->desc->hw_type);
		clr_reg = sunxi_clr_data_reg(offset, pctl->desc->hw_type);
		set_regval = readl(pctl->membase + set_reg);
		clr_regval = readl(pctl->membase + clr_reg);

		if (value) {
			set_regval |= BIT(index);
			writel(set_regval, pctl->membase + set_reg);
		} else {
			clr_regval |= BIT(index);
			writel(clr_regval, pctl->membase + clr_reg);
		}

		sunxi_debug(pctl->dev, "value is %d setreg[0x%x=0x%x] clearreg[0x%x=0x%x]\n",
			 value, set_reg, readl(pctl->membase + set_reg), clr_reg, readl(pctl->membase + clr_reg));
	} else {
		regval = readl(pctl->membase + reg);

		if (value)
			regval |= BIT(index);
		else
			regval &= ~(BIT(index));

		writel(regval, pctl->membase + reg);
	}
	raw_spin_unlock_irqrestore(&pctl->lock, flags);
}

static int sunxi_pinctrl_gpio_get_direction(struct gpio_chip *chip,
					unsigned offset)
{
	struct sunxi_pinctrl *pctl = gpiochip_get_data(chip);
	unsigned long flags;
	u32 val, mask;

	raw_spin_lock_irqsave(&pctl->lock, flags);

	val = readl(pctl->membase + sunxi_mux_reg(offset, pctl->desc->hw_type));
	mask = MUX_PINS_MASK << sunxi_mux_offset(offset);
	val = (val & mask) >> sunxi_mux_offset(offset);

	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return (val == 1) ? GPIO_LINE_DIRECTION_OUT : GPIO_LINE_DIRECTION_IN;
}

static int sunxi_pinctrl_gpio_direction_output(struct gpio_chip *chip,
					unsigned offset, int value)
{
	sunxi_pinctrl_gpio_set(chip, offset, value);
	return pinctrl_gpio_direction_output(chip->base + offset);
}

static int sunxi_pinctrl_gpio_of_xlate(struct gpio_chip *gc,
				const struct of_phandle_args *gpiospec,
				u32 *flags)
{
	int pin, base;

	base = PINS_PER_BANK * gpiospec->args[0];
	pin = base + gpiospec->args[1];

	if (pin > gc->ngpio)
		return -EINVAL;

	if (flags)
		*flags = gpiospec->args[2];

	return pin;
}

static int sunxi_pinctrl_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct sunxi_pinctrl *pctl = gpiochip_get_data(chip);
	struct sunxi_desc_function *desc;
	unsigned pinnum = pctl->desc->pin_base + offset;
	unsigned irqnum;

	if (offset >= chip->ngpio)
		return -ENXIO;

	desc = sunxi_pinctrl_desc_find_function_by_pin(pctl, pinnum, "irq");
	if (!desc)
		return -EINVAL;

	irqnum = desc->irqbank * IRQ_PER_BANK + desc->irqnum;

	sunxi_debug(chip->parent, "%s: request IRQ for GPIO %d, return %d\n",
		chip->label, offset + chip->base, irqnum);

	return irq_find_mapping(pctl->domain, irqnum);
}

static int sunxi_pinctrl_irq_request_resources(struct irq_data *d)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	struct sunxi_desc_function *func;
	int ret;

	func = sunxi_pinctrl_desc_find_function_by_pin(pctl,
					pctl->irq_array[d->hwirq], "irq");
	if (!func)
		return -EINVAL;

	ret = gpiochip_lock_as_irq(pctl->chip,
			pctl->irq_array[d->hwirq] - pctl->desc->pin_base);
	if (ret) {
		sunxi_err(pctl->dev, "unable to lock HW IRQ %lu for IRQ\n",
			irqd_to_hwirq(d));
		return ret;
	}

	/* Change muxing to INT mode */
	sunxi_pmx_set(pctl->pctl_dev, pctl->irq_array[d->hwirq], func->muxval);

	return 0;
}

static void sunxi_pinctrl_irq_release_resources(struct irq_data *d)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);

	gpiochip_unlock_as_irq(pctl->chip,
			      pctl->irq_array[d->hwirq] - pctl->desc->pin_base);
}

static int sunxi_pinctrl_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	u32 reg = sunxi_irq_cfg_reg(pctl->desc, d->hwirq);
	u8 index = sunxi_irq_cfg_offset(d->hwirq);
	unsigned long flags;
	u32 regval;
	u8 mode;

	if (pctl->ignore_irq[d->hwirq / IRQ_PER_BANK]) {
		sunxi_err(pctl->dev,
			"should not set pins irq type for "
			"GPIO hwirq%ld(ignore)\n", d->hwirq);
		return -EINVAL;
	}

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
	u32 status_reg = sunxi_irq_status_reg(pctl->desc, d->hwirq);
	u8 status_idx = sunxi_irq_status_offset(d->hwirq);

	/* Clear the IRQ */
	writel(1 << status_idx, pctl->membase + status_reg);
}

static void sunxi_pinctrl_irq_mask(struct irq_data *d)
{
	struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	u32 reg = sunxi_irq_ctrl_reg(pctl->desc, d->hwirq);
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
	u32 reg = sunxi_irq_ctrl_reg(pctl->desc, d->hwirq);
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
	__attribute__((unused))struct sunxi_pinctrl *pctl = irq_data_get_irq_chip_data(d);
	__attribute__((unused)) unsigned long bank = d->hwirq / IRQ_PER_BANK;
	__attribute__((unused)) struct irq_data *bank_irq_d = irq_get_irq_data(pctl->irq[bank]);
#if IS_ENABLED(CONFIG_ARM) || IS_ENABLED(CONFIG_ARM64)
	invoke_scp_fn_smc(on ? SET_WAKEUP_SRC : CLEAR_WAKEUP_SRC,
			  SET_SEC_WAKEUP_SOURCE(bank_irq_d->hwirq, d->hwirq),
			  0, 0);
#endif
	return 0;
}

static int sunxi_irq_set_affinity(struct irq_data *d,
		const struct cpumask *mask_val, bool force)
{
	unsigned int cpu;

	if (!force)
		cpu = cpumask_any_and(mask_val, cpu_online_mask);
	else
		cpu = cpumask_first(mask_val);

	irq_data_update_effective_affinity(d, cpumask_of(cpu));

	if (d->parent_data)
		return irq_chip_set_affinity_parent(d, mask_val, force);

	return 0;
}

static int sunxi_irq_set_retrigger(struct irq_data *d)
{
	/* if return 0, irq will be retriggered by handle_edge_irq */
	return 0;
}

static struct irq_chip sunxi_pinctrl_edge_irq_chip = {
	.name		= "sunxi_pio_edge",
	.irq_ack	= sunxi_pinctrl_irq_ack,
	.irq_mask	= sunxi_pinctrl_irq_mask,
	.irq_unmask	= sunxi_pinctrl_irq_unmask,
	.irq_request_resources = sunxi_pinctrl_irq_request_resources,
	.irq_release_resources = sunxi_pinctrl_irq_release_resources,
	.irq_set_type	= sunxi_pinctrl_irq_set_type,
	.flags		= IRQCHIP_MASK_ON_SUSPEND,
	.irq_set_wake   = sunxi_pinctrl_irq_set_wake,
	.irq_set_affinity = sunxi_irq_set_affinity,
	.irq_retrigger = sunxi_irq_set_retrigger,
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
	.flags		= IRQCHIP_MASK_ON_SUSPEND | IRQCHIP_EOI_THREADED |
			  IRQCHIP_EOI_IF_HANDLED,
	.irq_set_wake   = sunxi_pinctrl_irq_set_wake,
	.irq_set_affinity = sunxi_irq_set_affinity,
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
	unsigned long bank, reg, val, irq_ctrl_reg, irq_ctrl_val;

	for (bank = 0; bank < pctl->desc->irq_banks; bank++)
		if (irq == pctl->irq[bank])
			break;

	BUG_ON(bank == pctl->desc->irq_banks);
	/*
	 * normally, the ignored gpio_bank should not run here,
	 * but we strictly check it.
	 */
	BUG_ON(pctl->ignore_irq[bank]);

	chained_irq_enter(chip, desc);

	reg = sunxi_irq_status_reg_from_bank(pctl->desc, bank);
	val = readl(pctl->membase + reg);

	irq_ctrl_reg = sunxi_irq_ctrl_reg_from_bank(pctl->desc, bank);
	irq_ctrl_val = readl(pctl->membase + irq_ctrl_reg);
#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	val &= (~(sunxi_rproc_get_gpio_mask_by_hwirq(desc->irq_data.hwirq)));
#endif

	if (val) {
		int irqoffset;

		for_each_set_bit(irqoffset, &val, IRQ_PER_BANK) {
			int pin_irq = irq_find_mapping(pctl->domain,
						       bank * IRQ_PER_BANK + irqoffset);
			if (BIT(irqoffset) & irq_ctrl_val)
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

	pctl->nfunctions++;

	return 0;
}

static int sunxi_pinctrl_build_state(struct platform_device *pdev)
{
	struct sunxi_pinctrl *pctl = platform_get_drvdata(pdev);
	void *ptr;
	int i;

	/*
	 * Allocate groups
	 *
	 * We assume that the number of groups is the number of pins
	 * given in the data array.

	 * This will not always be true, since some pins might not be
	 * available in the current variant, but fortunately for us,
	 * this means that the number of pins is the maximum group
	 * number we will ever see.
	 */
	pctl->groups = devm_kcalloc(&pdev->dev,
				    pctl->desc->npins, sizeof(*pctl->groups),
				    GFP_KERNEL);
	if (!pctl->groups)
		return -ENOMEM;

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_pinctrl_group *group = pctl->groups + pctl->ngroups;

		if (pin->variant && !(pctl->variant & pin->variant))
			continue;

		group->name = pin->pin.name;
		group->pin = pin->pin.number;

		/* And now we count the actual number of pins / groups */
		pctl->ngroups++;
	}

	/*
	* We suppose that the maximum number of functions will be 12 times the current num of pins,
	 * and extra four fixed functions:gpio_in, gpio_out, io_disabled and irq.
	 * We'll reallocate that later.
	 */
	pctl->functions = kcalloc(pctl->ngroups * 12 + 4,
			sizeof(*pctl->functions),
				  GFP_KERNEL);
	if (!pctl->functions)
		return -ENOMEM;

	/* Count functions and their associated groups */
	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_desc_function *func;

		if (pin->variant && !(pctl->variant & pin->variant))
			continue;

		for (func = pin->functions; func->name; func++) {
			if (func->variant && !(pctl->variant & func->variant))
				continue;

			/* Create interrupt mapping while we're at it */
			if (!strcmp(func->name, "irq") && pctl->desc->irq_banks) {
				int irqnum = func->irqnum + func->irqbank * IRQ_PER_BANK;
				pctl->irq_array[irqnum] = pin->pin.number;
			}

			sunxi_pinctrl_add_function(pctl, func->name);
		}
	}

	/* And now allocated and fill the array for real */
	ptr = krealloc(pctl->functions,
		       pctl->nfunctions * sizeof(*pctl->functions),
		       GFP_KERNEL);
	if (!ptr) {
		kfree(pctl->functions);
		pctl->functions = NULL;
		return -ENOMEM;
	}
	pctl->functions = ptr;

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;
		struct sunxi_desc_function *func;

		if (pin->variant && !(pctl->variant & pin->variant))
			continue;

		for (func = pin->functions; func->name; func++) {
			struct sunxi_pinctrl_function *func_item;
			const char **func_grp;

			if (func->variant && !(pctl->variant & func->variant))
				continue;

			func_item = sunxi_pinctrl_find_function_by_name(pctl,
									func->name);
			if (!func_item) {
				kfree(pctl->functions);
				return -EINVAL;
			}

			if (!func_item->groups) {
				func_item->groups =
					devm_kcalloc(&pdev->dev,
						     func_item->ngroups,
						     sizeof(*func_item->groups),
						     GFP_KERNEL);
				if (!func_item->groups) {
					kfree(pctl->functions);
					return -ENOMEM;
				}
			}

			func_grp = func_item->groups;
			while (*func_grp)
				func_grp++;

			*func_grp = pin->pin.name;
		}
	}

	return 0;
}

static int sunxi_pinctrl_setup_debounce(struct sunxi_pinctrl *pctl,
					struct device_node *node)
{
	u8 val;
	int i, ret;

	/* Deal with old DTs that didn't have the oscillators */
	if (of_clk_get_parent_count(node) != 3)
		return 0;

	/* If we don't have any setup, bail out */
	if (!of_find_property(node, "input-debounce", NULL))
		return 0;

	for (i = 0; i < pctl->desc->irq_banks; i++) {
		u32 debounce;

		ret = of_property_read_u32_index(node, "input-debounce",
						 i, &debounce);
		if (ret)
			return ret;

		if (!debounce)
			continue;

		val = sunxi_pinctrl_get_debounce_param(pctl, debounce);

		writel(val,
		       pctl->membase +
		       sunxi_irq_debounce_reg_from_bank(pctl->desc, i));
	}

	return 0;
}

static struct irq_domain *sunxi_pctrl_get_irq_domain(struct device_node *np)
{
	struct device_node *parent;
	struct irq_domain *domain;

	if (!of_find_property(np, "interrupt-parent", NULL))
		return NULL;

	parent = of_irq_find_parent(np);
	if (!parent)
		return ERR_PTR(-ENXIO);

	domain = irq_find_host(parent);
	if (!domain)
		/* domain not registered yet */
		return ERR_PTR(-EPROBE_DEFER);

	return domain;
}

int sunxi_bsp_pinctrl_init_with_variant(struct platform_device *pdev,
				    const struct sunxi_pinctrl_desc *desc,
				    unsigned long variant)
{
	struct device_node *node = pdev->dev.of_node;
	struct pinctrl_desc *pctrl_desc;
	struct pinctrl_pin_desc *pins;
	struct sunxi_pinctrl *pctl;
	struct pinmux_ops *pmxops;
	int i, ret, last_pin, pin_idx, j;
	struct clk *clk;
	uint32_t *ignore_array, array_num;
	__maybe_unused struct resource *res;
#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	struct irq_desc *irq_desc;
	uint32_t banks_mask;
	void __iomem *ctrl_reg, *sta_reg;
#endif
	bool withstand_auto_assert = false;

	pctl = devm_kzalloc(&pdev->dev, sizeof(*pctl), GFP_KERNEL);
	if (!pctl)
		return -ENOMEM;
	platform_set_drvdata(pdev, pctl);

	raw_spin_lock_init(&pctl->lock);

	pctl->membase = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(pctl->membase))
		return PTR_ERR(pctl->membase);

	pctl->dev = &pdev->dev;
	pctl->desc = desc;
	pctl->variant = variant;

	pctl->irq_array = devm_kcalloc(&pdev->dev,
				       IRQ_PER_BANK * pctl->desc->irq_banks,
				       sizeof(*pctl->irq_array),
				       GFP_KERNEL);
	if (!pctl->irq_array)
		return -ENOMEM;

	ret = sunxi_pinctrl_build_state(pdev);
	if (ret) {
		sunxi_err(&pdev->dev, "dt probe failed: %d\n", ret);
		return ret;
	}

	pins = devm_kcalloc(&pdev->dev,
			    pctl->desc->npins, sizeof(*pins),
			    GFP_KERNEL);
	if (!pins)
		return -ENOMEM;

	for (i = 0, pin_idx = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;

		if (pin->variant && !(pctl->variant & pin->variant))
			continue;

		pins[pin_idx++] = pin->pin;
	}

	pctrl_desc = devm_kzalloc(&pdev->dev,
				  sizeof(*pctrl_desc),
				  GFP_KERNEL);
	if (!pctrl_desc)
		return -ENOMEM;

	pctrl_desc->name = dev_name(&pdev->dev);
	pctrl_desc->owner = THIS_MODULE;
	pctrl_desc->pins = pins;
	pctrl_desc->npins = pctl->ngroups;
	pctrl_desc->confops = &sunxi_pconf_ops;
	pctrl_desc->pctlops = &sunxi_pctrl_ops;

	pmxops = devm_kmemdup(&pdev->dev, &sunxi_pmx_ops, sizeof(sunxi_pmx_ops),
			      GFP_KERNEL);
	if (!pmxops)
		return -ENOMEM;

	if (desc->disable_strict_mode)
		pmxops->strict = false;

	pctrl_desc->pmxops = pmxops;

	pctl->pctl_dev = devm_pinctrl_register(&pdev->dev, pctrl_desc, pctl);
	if (IS_ERR(pctl->pctl_dev)) {
		sunxi_err(&pdev->dev, "couldn't register pinctrl driver\n");
		return PTR_ERR(pctl->pctl_dev);
	}

	pctl->chip = devm_kzalloc(&pdev->dev, sizeof(*pctl->chip), GFP_KERNEL);
	if (!pctl->chip)
		return -ENOMEM;

	last_pin = pctl->desc->pins[pctl->desc->npins - 1].pin.number;
	pctl->chip->owner = THIS_MODULE;
	pctl->chip->request = gpiochip_generic_request;
	pctl->chip->free = gpiochip_generic_free;
	pctl->chip->set_config = gpiochip_generic_config;
	pctl->chip->direction_input = sunxi_pinctrl_gpio_direction_input;
	pctl->chip->direction_output = sunxi_pinctrl_gpio_direction_output;
	pctl->chip->get_direction = sunxi_pinctrl_gpio_get_direction;
	pctl->chip->get = sunxi_pinctrl_gpio_get;
	pctl->chip->set = sunxi_pinctrl_gpio_set;
	pctl->chip->of_xlate = sunxi_pinctrl_gpio_of_xlate;
	pctl->chip->to_irq = sunxi_pinctrl_gpio_to_irq;
	pctl->chip->of_gpio_n_cells = 3;
	pctl->chip->can_sleep = false;
	pctl->chip->ngpio = round_up(last_pin, PINS_PER_BANK) -
			    pctl->desc->pin_base;
	pctl->chip->label = dev_name(&pdev->dev);
	pctl->chip->parent = &pdev->dev;
	pctl->chip->base = pctl->desc->pin_base;

	ret = of_clk_get_parent_count(node);
	clk = devm_clk_get(&pdev->dev, ret == 1 ? NULL : "apb");
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		if (ret != -EPROBE_DEFER)
			sunxi_err(&pdev->dev, "Couldn't get clk\n");

		return ret;
	}

	ret = clk_prepare_enable(clk);
	if (ret) {
		sunxi_err(&pdev->dev, "Couldn't enable clk\n");
		return ret;
	}

	pctl->irq = devm_kcalloc(&pdev->dev,
				 pctl->desc->irq_banks,
				 sizeof(*pctl->irq),
				 GFP_KERNEL);
	if (!pctl->irq) {
		ret = -ENOMEM;
		goto clk_error;
	}

	pctl->ignore_irq = devm_kcalloc(&pdev->dev, pctl->desc->irq_banks,
				 sizeof(*pctl->ignore_irq), GFP_KERNEL);
	if (!pctl->ignore_irq) {
		ret = -ENOMEM;
		goto clk_error;
	}
	memset(pctl->ignore_irq, 0, pctl->desc->irq_banks
					* sizeof(*pctl->ignore_irq));

	ret = of_property_count_elems_of_size(node,
					"ignore-interrupts", sizeof(uint32_t));
	if (ret > 0) {
		array_num = ret;
		ignore_array = devm_kcalloc(&pdev->dev, array_num,
						sizeof(uint32_t), GFP_KERNEL);
		if (!ignore_array) {
			ret = -ENOMEM;
			goto clk_error;
		}
		ret = of_property_read_u32_array(node,
						"ignore-interrupts", ignore_array, array_num);
		if (ret) {
			sunxi_err(&pdev->dev, "Couldn't read ignore-interrupts\n");
			ret = -ENODEV;
			goto clk_error;
		}
	} else {
		ignore_array = NULL;
	}

	for (i = 0; i < pctl->desc->irq_banks; i++) {
		pctl->irq[i] = platform_get_irq(pdev, i);
		if (pctl->irq[i] < 0) {
			ret = pctl->irq[i];
			goto clk_error;
		}
		if (!ignore_array)
			continue;
		for (j = 0; j < array_num; j++)
			if (ignore_array[j] == irq_get_irq_data(pctl->irq[i])->hwirq) {
				pctl->ignore_irq[i] = 1;
				sunxi_info(&pdev->dev, "Ignore GPIO %d IRQ\n", ignore_array[j]);
			}
	}
	if (ignore_array)
		devm_kfree(&pdev->dev, ignore_array);
	ignore_array = NULL;

	pctl->parent_domain = sunxi_pctrl_get_irq_domain(node);
	if (IS_ERR(pctl->parent_domain)) {
		ret = PTR_ERR(pctl->parent_domain);
		goto clk_error;
	}

	pctl->domain = irq_domain_create_hierarchy(pctl->parent_domain, 0,
					     pctl->desc->irq_banks * IRQ_PER_BANK,
					     of_node_to_fwnode(node),
					     &sunxi_pinctrl_irq_domain_ops,
					     pctl);
	if (!pctl->domain) {
		sunxi_err(&pdev->dev, "Couldn't register IRQ domain\n");
		ret = -ENOMEM;
		goto clk_error;
	}

	for (i = 0; i < (pctl->desc->irq_banks * IRQ_PER_BANK); i++) {
		int irqno = irq_create_mapping(pctl->domain, i);
		struct irq_data *child_irq_data, *parent_irq_data;
		irq_set_chip_and_handler(irqno, &sunxi_pinctrl_edge_irq_chip,
					 handle_edge_irq);
		irq_set_chip_data(irqno, pctl);
		child_irq_data = irq_get_irq_data(irqno);
		BUG_ON((i / IRQ_PER_BANK) >= pctl->desc->irq_banks);
		parent_irq_data = irq_get_irq_data(pctl->irq[i / IRQ_PER_BANK]);
		child_irq_data->parent_data = parent_irq_data;
	}

	for (i = 0; i < pctl->desc->irq_banks; i++) {
		/* skip ignored gpio_bank */
		if (pctl->ignore_irq[i])
			continue;

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
		irq_desc = irq_to_desc(pctl->irq[i]);
		banks_mask = sunxi_rproc_get_gpio_mask_by_hwirq(irq_desc->irq_data.hwirq);
		ctrl_reg = pctl->membase + sunxi_irq_ctrl_reg_from_bank(pctl->desc, i);
		sta_reg = pctl->membase + sunxi_irq_status_reg_from_bank(pctl->desc, i);

		writel(readl(ctrl_reg) & banks_mask, ctrl_reg);
		writel(0xffffffff & ~banks_mask, sta_reg);

		if (banks_mask != 0xffffffff)
			irq_set_chained_handler_and_data(pctl->irq[i],
					sunxi_pinctrl_irq_handler,
					pctl);
#else
		/* Mask and clear all IRQs before registering a handler */
		writel(0, pctl->membase +
			  sunxi_irq_ctrl_reg_from_bank(pctl->desc, i));
		writel(0xffffffff,
		       pctl->membase +
		       sunxi_irq_status_reg_from_bank(pctl->desc, i));

		irq_set_chained_handler_and_data(pctl->irq[i],
						 sunxi_pinctrl_irq_handler,
						 pctl);
#endif
	}

#ifdef SUNXI_PINCTRL_I2S0_ROUTE_PAD
	/*
	 * This address is used in both the pio and r_pio drivers,
	 * so it is not bound to a specific device for mapping.
	 * Use ioremap instead of devm_ioremap_resource.
	 */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "i2s0");
	if (res) {
		sunxi_info(&pdev->dev, "find i2s0 route contrl\n");
		pctl->pad_addr[0].addr = ioremap(res->start, resource_size(res));
		if (!pctl->pad_addr[0].addr) {
			ret = -ENXIO;
			goto clk_error;
		}

		strncpy(pctl->pad_addr[0].name, "i2s0", 4);
		pctl->pad_addr[0].name[4] = '\0';
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dmic");
	if (res) {
		sunxi_info(&pdev->dev, "find dmic route contrl\n");
		pctl->pad_addr[1].addr = ioremap(res->start, resource_size(res));
		if (!pctl->pad_addr[1].addr) {
			ret = -ENXIO;
			goto clk_error;
		}

		strncpy(pctl->pad_addr[1].name, "dmic", 4);
		pctl->pad_addr[1].name[4] = '\0';
	}
#endif
	sunxi_pinctrl_setup_debounce(pctl, node);

	ret = gpiochip_add_data(pctl->chip, pctl);
	if (ret) {
		sunxi_err(&pdev->dev, "Couldn't add gpiochip\n");
		goto clk_error;
	}

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;

		ret = gpiochip_add_pin_range(pctl->chip, dev_name(&pdev->dev),
					     pin->pin.number - pctl->desc->pin_base,
					     pin->pin.number, 1);
		if (ret)
			goto gpiochip_error;
	}


	if (pctl->desc->auto_power_source_switch || sunxi_pinctrl_hw_info[pctl->desc->hw_type].power_mode_detect)
		withstand_auto_assert = true;

	sunxi_info(&pdev->dev, "pinctrl withstand voltage config mode=%s\n",
		withstand_auto_assert ? (pctl->desc->auto_power_source_switch ? "auto_hard" : "auto_soft") : "manual");

	sunxi_info_once(NULL, "sunxi pinctrl core driver version: %s\n", SUNXI_PINCTRL_CORE_VERSION);

	return 0;

gpiochip_error:
	gpiochip_remove(pctl->chip);
clk_error:
	clk_disable_unprepare(clk);

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_bsp_pinctrl_init_with_variant);
MODULE_LICENSE("GPL");
MODULE_VERSION(SUNXI_PINCTRL_CORE_VERSION);
MODULE_AUTHOR("lvda@allwinnertech.com");

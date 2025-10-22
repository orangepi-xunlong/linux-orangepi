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

#ifndef __PINCTRL_SUNXI_H
#define __PINCTRL_SUNXI_H

#define SUNXI_MODNAME "pin"
#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/pinctrl/pinconf-generic.h>

#define SUNXI_BANK_OFFSET(bank, bankbase)	((bank) - (bankbase))
#define SUNXI_PIN_BASE(bank)			(SUNXI_BANK_OFFSET(bank, 'A') * 32)

#define PA_BASE			SUNXI_PIN_BASE('A')
#define PB_BASE			SUNXI_PIN_BASE('B')
#define PC_BASE			SUNXI_PIN_BASE('C')
#define PD_BASE			SUNXI_PIN_BASE('D')
#define PE_BASE			SUNXI_PIN_BASE('E')
#define PF_BASE			SUNXI_PIN_BASE('F')
#define PG_BASE			SUNXI_PIN_BASE('G')
#define PH_BASE			SUNXI_PIN_BASE('H')
#define PI_BASE			SUNXI_PIN_BASE('I')
#define PJ_BASE			SUNXI_PIN_BASE('J')
#define PK_BASE			SUNXI_PIN_BASE('K')
#define PL_BASE			SUNXI_PIN_BASE('L')
#define PM_BASE			SUNXI_PIN_BASE('M')
#define PN_BASE			SUNXI_PIN_BASE('N')

#define SUNXI_PINCTRL_PIN(bank, pin)		\
	PINCTRL_PIN(P ## bank ## _BASE + (pin), "P" #bank #pin)

#define SUNXI_PIN_NAME_MAX_LEN	5

#define PINS_PER_BANK		32
#define MUX_PINS_PER_REG	8
#define MUX_PINS_BITS		4
#define MUX_PINS_MASK		0x0f
#define DATA_PINS_PER_REG	32
#define DATA_PINS_BITS		1
#define DATA_PINS_MASK		0x01
#define PULL_PINS_PER_REG	16
#define PULL_PINS_BITS		2
#define PULL_PINS_MASK		0x03

#define IRQ_PER_BANK		32

#define IRQ_CFG_IRQ_PER_REG		8
#define IRQ_CFG_IRQ_BITS		4
#define IRQ_CFG_IRQ_MASK		((1 << IRQ_CFG_IRQ_BITS) - 1)
#define IRQ_CTRL_IRQ_PER_REG		32
#define IRQ_CTRL_IRQ_BITS		1
#define IRQ_CTRL_IRQ_MASK		((1 << IRQ_CTRL_IRQ_BITS) - 1)
#define IRQ_STATUS_IRQ_PER_REG		32
#define IRQ_STATUS_IRQ_BITS		1
#define IRQ_STATUS_IRQ_MASK		((1 << IRQ_STATUS_IRQ_BITS) - 1)

#define IRQ_DEBOUNCE_MASK	0x71

#define IRQ_EDGE_RISING		0x00
#define IRQ_EDGE_FALLING	0x01
#define IRQ_LEVEL_HIGH		0x02
#define IRQ_LEVEL_LOW		0x03
#define IRQ_EDGE_BOTH		0x04

#define GRP_CFG_REG		0x300

#define IO_BIAS_MASK		GENMASK(3, 0)

#define SUN4I_FUNC_INPUT	0

#define PINCTRL_SUN5I_A10S	BIT(1)
#define PINCTRL_SUN5I_A13	BIT(2)
#define PINCTRL_SUN5I_GR8	BIT(3)
#define PINCTRL_SUN6I_A31	BIT(4)
#define PINCTRL_SUN6I_A31S	BIT(5)
#define PINCTRL_SUN4I_A10	BIT(6)
#define PINCTRL_SUN7I_A20	BIT(7)
#define PINCTRL_SUN8I_R40	BIT(8)
#define PINCTRL_SUN8I_V3	BIT(9)
#define PINCTRL_SUN8I_V3S	BIT(10)

#define POWER_SOURCE_MASK	0x01

#if IS_ENABLED(CONFIG_AW_PINCTRL_DEBUGFS)
#define SUNXI_PINCFG_TYPE_FUNC 	(PIN_CONFIG_END - 2)
#define SUNXI_PINCFG_TYPE_DAT 	(PIN_CONFIG_END - 1)
#endif

#define SUN55IW3_PK_BASE	0x0500

#ifndef	GPIO_LINE_DIRECTION_OUT
#define	GPIO_LINE_DIRECTION_OUT	0
#endif

#ifndef	GPIO_LINE_DIRECTION_IN
#define	GPIO_LINE_DIRECTION_IN	1
#endif

enum sunxi_desc_bias_voltage {
	BIAS_VOLTAGE_NONE,
	/*
	 * Bias voltage configuration is done through
	 * Pn_GRP_CONFIG registers, as seen on A80 SoC.
	 */
	BIAS_VOLTAGE_GRP_CONFIG,
	/*
	 * Bias voltage is set through PIO_POW_MOD_SEL_REG
	 * register, as seen on H6 SoC, for example.
	 */
	BIAS_VOLTAGE_PIO_POW_MODE_SEL,
	/*
	 * Bias voltage is set through PIO_POW_MOD_SEL_REG
	 * and power_mode_ctrl_reg register, as seen on
	 * A100 SoC, for example.
	 */
	BIAS_VOLTAGE_PIO_POW_MODE_CTL,
	/*
	 * Bias voltage is set through PIO_POW_MOD_SEL_REG
	 * and power_mode_ctrl_reg register, but has different
	 * mapping and control bit, A523 SoC, for example.
	 */
	BIAS_VOLTAGE_PIO_POW_MODE_CTL_V2,
};

enum sunxi_pinctrl_hw_type {
	SUNXI_PCTL_HW_TYPE_0,  /* Older chips */
	SUNXI_PCTL_HW_TYPE_1,  /* Newer chips: sun8iw20, sun20iw1, sun50iw12 */
	SUNXI_PCTL_HW_TYPE_2,  /* Newer chips: sun60iw1 */
	SUNXI_PCTL_HW_TYPE_3,  /* Newer chips: sun55iw3 */
	SUNXI_PCTL_HW_TYPE_4,  /* Newer chips: sun60iw2 */
	SUNXI_PCTL_HW_TYPE_5,  /* Support self-adaption */
	SUNXI_PCTL_HW_TYPE_6,  /* chips: sun55iw3-prcm */
	SUNXI_PCTL_HW_TYPE_7,  /* chips: sun55iw6-prcm */
	SUNXI_PCTL_HW_TYPE_8,  /* chips: sun300iw1-r */
	SUNXI_PCTL_HW_TYPE_9,  /* chips: sun8iw20 */
	SUNXI_PCTL_HW_TYPE_10, /* chips: sun60iw2 B-Version IC */
	/* Add new types here ... */
	SUNXI_PCTL_HW_TYPE_CNT,
};

/* Reference <Port_Controller_Spec: Port Register List> for the information below: */
struct sunxi_pinctrl_hw_info {
	u8 initial_bank_offset;	/* First bank offset to pin base */
	u8 mux_regs_offset;	/* Configure Register's offset */
	u32 data_regs_offset;	/* Data Register's offset */
	u32 data_set_regs_offset;	/* Set Data Register's offset */
	u32 data_clr_regs_offset;	/* Clear data Register's offset */
	u8 dlevel_regs_offset;	/* Multi Driving Register's offset */
	u8 bank_mem_size;  	/* Size of the basic registers (including CFG/DAT/DRV/PUL) of any bank  */
	u8 pull_regs_offset;	/* Pull Register's offset */
	u8 dlevel_pins_per_reg; /* How many pins does a 'Multi-Driving Register' contain? */
	u8 dlevel_pins_bits;	/* How many bits does a 'Multi-Driving Register' use for a pin? */
	u8 dlevel_pins_mask;	/* Bit mask for 'dlevel_pins_bits' */
	u8 irq_mux_val;		/* Mux value for IRQ function */
	u32 irq_cfg_reg;
	u32 irq_ctrl_reg;
	u32 irq_status_reg;
	u32 irq_debounce_reg;
	u32 irq_mem_size;
	u32 irq_mem_base;
	u32 irq_mem_used;
	u32 power_mode_sel_reg;
	u32 mode_sel_vccio_bit;
	u32 power_mode_ctrl_reg;
	u32 mode_ctrl_vccio_bit;
	u32 power_mode_val_reg;  /* GPIO_POW_VAL: the register read the voltage withstand */
	u32 mode_val_vccio_bit;
	u32 pio_pow_ctrl_reg;
	bool power_mode_reverse;  /* true: GPIO_POW_VAL and GPIO_POW_MID_SEL bit reverse, A523 SOC, for example */
	bool auto_power_detect_mode_reverse;  /* true: GPIO_PWR_VAL bit reverse, for example A733-B SoC */
	bool power_mode_detect;  /* true: Config voltage withstand by reading power_mode_val_reg */
	bool data_reg_irregular;
	bool data_set_mode_select; /* true: change the value of pin by data_set and data_cle reg */
	u32 data_mem_size;
};

/* Indexed by `enum sunxi_pinctrl_hw_type` */
extern struct sunxi_pinctrl_hw_info sunxi_pinctrl_hw_info[SUNXI_PCTL_HW_TYPE_CNT];

struct sunxi_desc_function {
	unsigned long	variant;
	const char	*name;
	u8		muxval;
	u8		irqbank;
	u8		irqnum;
};

struct sunxi_desc_pin {
	struct pinctrl_pin_desc		pin;
	unsigned long			variant;
	struct sunxi_desc_function	*functions;
};

struct sunxi_pinctrl_desc {
	const struct sunxi_desc_pin	*pins;
	unsigned int			npins;
	unsigned int			pin_base;
	unsigned int			banks;
	const unsigned int		*bank_base;
	unsigned int			irq_banks;
	const unsigned int		*irq_bank_map;
	const unsigned int		*vccio_banks;
	unsigned int			vccio_nbanks;
	bool				irq_read_needs_mux;
	bool				disable_strict_mode;
	enum sunxi_desc_bias_voltage	io_bias_cfg_variant;
	bool				pf_power_source_switch;
	bool				auto_power_source_switch;
	enum sunxi_pinctrl_hw_type	hw_type;
};

struct sunxi_pinctrl_function {
	const char	*name;
	const char	**groups;
	unsigned	ngroups;
};

struct sunxi_pinctrl_group {
	const char	*name;
	unsigned	pin;
};

struct sunxi_pinctrl_regulator {
	struct regulator	*regulator;
	struct regulator	*regulator_optional;
	refcount_t		refcount;
};

struct pad_route_addr {
	char name[10]; /* The length of the function */
	u32 __iomem	*addr;
};

struct sunxi_pinctrl {
	void __iomem			*membase;
	struct gpio_chip		*chip;
	const struct sunxi_pinctrl_desc	*desc;
	struct device			*dev;
	struct sunxi_pinctrl_regulator	regulators[11];
	struct irq_domain		*domain;
	struct irq_domain		*parent_domain;
	struct sunxi_pinctrl_function	*functions;
	unsigned			nfunctions;
	struct sunxi_pinctrl_group	*groups;
	unsigned			ngroups;
	int				*irq;
	uint32_t			*ignore_irq;
	unsigned			*irq_array;
	raw_spinlock_t			lock;
	struct pinctrl_dev		*pctl_dev;
	unsigned long			variant;
	struct pad_route_addr		pad_addr[2];
};

#define SUNXI_PIN(_pin, ...)					\
	{							\
		.pin = _pin,					\
		.functions = (struct sunxi_desc_function[]){	\
			__VA_ARGS__, { } },			\
	}

#define SUNXI_PIN_VARIANT(_pin, _variant, ...)			\
	{							\
		.pin = _pin,					\
		.variant = _variant,				\
		.functions = (struct sunxi_desc_function[]){	\
			__VA_ARGS__, { } },			\
	}

#define SUNXI_FUNCTION(_val, _name)				\
	{							\
		.name = _name,					\
		.muxval = _val,					\
	}

#define SUNXI_FUNCTION_VARIANT(_val, _name, _variant)		\
	{							\
		.name = _name,					\
		.muxval = _val,					\
		.variant = _variant,				\
	}

#define SUNXI_FUNCTION_IRQ(_val, _irq)				\
	{							\
		.name = "irq",					\
		.muxval = _val,					\
		.irqnum = _irq,					\
	}

#define SUNXI_FUNCTION_IRQ_BANK(_val, _bank, _irq)		\
	{							\
		.name = "irq",					\
		.muxval = _val,					\
		.irqbank = _bank,				\
		.irqnum = _irq,					\
	}

/*
 * The sunXi PIO registers are organized as is:
 * 0x00 - 0x0c	Muxing values.
 *		8 pins per register, each pin having a 4bits value
 * 0x10		Pin values
 *		32 bits per register, each pin corresponding to one bit
 * 0x14 - 0x18	Drive level
 *		16 pins per register, each pin having a 2bits value
 * 0x1c - 0x20	Pull-Up values
 *		16 pins per register, each pin having a 2bits value
 *
 * This is for the first bank. Each bank will have the same layout,
 * with an offset being a multiple of 0x24.
 *
 * The following functions calculate from the pin number the register
 * and the bit offset that we should access.
 */

static inline u32 sunxi_pinctrl_recalc_offset(enum sunxi_pinctrl_hw_type hw_type, u8 bank)
{
#ifdef CONFIG_ARCH_SUN55IW3
	/* In a523 , the offset of PK does not meet the general rules, it starts directly from 0x500 */
	if (bank != 10)
		return bank * sunxi_pinctrl_hw_info[hw_type].bank_mem_size +
			sunxi_pinctrl_hw_info[hw_type].initial_bank_offset;
	else
		return SUN55IW3_PK_BASE;
#else
	return bank * sunxi_pinctrl_hw_info[hw_type].bank_mem_size +
		sunxi_pinctrl_hw_info[hw_type].initial_bank_offset;
#endif
}

static inline u32 sunxi_mux_reg(u16 pin, enum sunxi_pinctrl_hw_type hw_type)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
	offset += sunxi_pinctrl_hw_info[hw_type].mux_regs_offset;
	offset += pin % PINS_PER_BANK / MUX_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_mux_offset(u16 pin)
{
	u32 pin_num = pin % MUX_PINS_PER_REG;
	return pin_num * MUX_PINS_BITS;
}

static inline u32 sunxi_data_reg(u16 pin, enum sunxi_pinctrl_hw_type hw_type)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset;

	if (sunxi_pinctrl_hw_info[hw_type].data_reg_irregular) {
		offset = sunxi_pinctrl_hw_info[hw_type].data_regs_offset +
				pin / PINS_PER_BANK * sunxi_pinctrl_hw_info[hw_type].data_mem_size;
	} else {
		offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
		offset += sunxi_pinctrl_hw_info[hw_type].data_regs_offset;
	}
	offset += pin % PINS_PER_BANK / DATA_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_set_data_reg(u16 pin, enum sunxi_pinctrl_hw_type hw_type)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset;

	if (sunxi_pinctrl_hw_info[hw_type].data_reg_irregular) {
		offset = sunxi_pinctrl_hw_info[hw_type].data_set_regs_offset +
				pin / PINS_PER_BANK * sunxi_pinctrl_hw_info[hw_type].data_mem_size;
	} else {
		offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
		offset += sunxi_pinctrl_hw_info[hw_type].data_set_regs_offset;
	}
	offset += pin % PINS_PER_BANK / DATA_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_clr_data_reg(u16 pin, enum sunxi_pinctrl_hw_type hw_type)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset;

	if (sunxi_pinctrl_hw_info[hw_type].data_reg_irregular) {
		offset = sunxi_pinctrl_hw_info[hw_type].data_clr_regs_offset +
				pin / PINS_PER_BANK * sunxi_pinctrl_hw_info[hw_type].data_mem_size;
	} else {
		offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
		offset += sunxi_pinctrl_hw_info[hw_type].data_clr_regs_offset;
	}
	offset += pin % PINS_PER_BANK / DATA_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_data_offset(u16 pin)
{
	u32 pin_num = pin % DATA_PINS_PER_REG;
	return pin_num * DATA_PINS_BITS;
}

static inline u32 sunxi_dlevel_reg(u16 pin, enum sunxi_pinctrl_hw_type hw_type)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
	offset += sunxi_pinctrl_hw_info[hw_type].dlevel_regs_offset;
	offset += pin % PINS_PER_BANK / sunxi_pinctrl_hw_info[hw_type].dlevel_pins_per_reg * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_dlevel_offset(u16 pin, enum sunxi_pinctrl_hw_type hw_type)
{
	u32 pin_num = pin % sunxi_pinctrl_hw_info[hw_type].dlevel_pins_per_reg;
	return pin_num * sunxi_pinctrl_hw_info[hw_type].dlevel_pins_bits;
}

static inline u32 sunxi_pull_reg(u16 pin, enum sunxi_pinctrl_hw_type hw_type)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset = sunxi_pinctrl_recalc_offset(hw_type, bank);
	offset += sunxi_pinctrl_hw_info[hw_type].pull_regs_offset;
	offset += pin % PINS_PER_BANK / PULL_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_pull_offset(u16 pin)
{
	u32 pin_num = pin % PULL_PINS_PER_REG;
	return pin_num * PULL_PINS_BITS;
}

static inline u32 sunxi_irq_hw_bank_num(const struct sunxi_pinctrl_desc *desc, u8 bank)
{
	BUG_ON(bank >= desc->irq_banks);
	if (!desc->irq_bank_map)
		return bank;
	else
		return desc->irq_bank_map[bank];
}

static inline u32 sunxi_irq_cfg_reg(const struct sunxi_pinctrl_desc *desc,
				    u16 irq)
{
	u8 bank = irq / IRQ_PER_BANK;
	u8 reg = (irq % IRQ_PER_BANK) / IRQ_CFG_IRQ_PER_REG * 0x04;
	u32 hw_type = desc->hw_type;

	return sunxi_pinctrl_hw_info[hw_type].irq_cfg_reg +
		sunxi_irq_hw_bank_num(desc, bank) * sunxi_pinctrl_hw_info[hw_type].irq_mem_size + reg;
}

static inline u32 sunxi_irq_cfg_offset(u16 irq)
{
	u32 irq_num = irq % IRQ_CFG_IRQ_PER_REG;
	return irq_num * IRQ_CFG_IRQ_BITS;
}

static inline u32 sunxi_irq_regs_size_from_bank(const struct sunxi_pinctrl_desc *desc, u8 bank)
{
	u32 hw_type = desc->hw_type;
	return sunxi_pinctrl_hw_info[hw_type].irq_mem_used;
}

static inline u32 sunxi_irq_regs_from_bank(const struct sunxi_pinctrl_desc *desc, u8 bank)
{
	u32 hw_type = desc->hw_type;
	return sunxi_pinctrl_hw_info[hw_type].irq_mem_base +
			sunxi_irq_hw_bank_num(desc, bank) * sunxi_pinctrl_hw_info[hw_type].irq_mem_size;
}

static inline u32 sunxi_irq_ctrl_reg_from_bank(const struct sunxi_pinctrl_desc *desc, u8 bank)
{
	u32 hw_type = desc->hw_type;
	return sunxi_pinctrl_hw_info[hw_type].irq_ctrl_reg +
		sunxi_irq_hw_bank_num(desc, bank) * sunxi_pinctrl_hw_info[hw_type].irq_mem_size;
}

static inline u32 sunxi_irq_ctrl_reg(const struct sunxi_pinctrl_desc *desc,
				     u16 irq)
{
	u8 bank = irq / IRQ_PER_BANK;

	return sunxi_irq_ctrl_reg_from_bank(desc, bank);
}

static inline u32 sunxi_irq_ctrl_offset(u16 irq)
{
	u32 irq_num = irq % IRQ_CTRL_IRQ_PER_REG;
	return irq_num * IRQ_CTRL_IRQ_BITS;
}

static inline u32 sunxi_irq_debounce_reg_from_bank(const struct sunxi_pinctrl_desc *desc, u8 bank)
{
	u32 hw_type = desc->hw_type;
	return sunxi_pinctrl_hw_info[hw_type].irq_debounce_reg +
		sunxi_irq_hw_bank_num(desc, bank) * sunxi_pinctrl_hw_info[hw_type].irq_mem_size;
}

static inline u32 sunxi_irq_status_reg_from_bank(const struct sunxi_pinctrl_desc *desc, u8 bank)
{
	u32 hw_type = desc->hw_type;
	return sunxi_pinctrl_hw_info[hw_type].irq_status_reg +
		sunxi_irq_hw_bank_num(desc, bank) * sunxi_pinctrl_hw_info[hw_type].irq_mem_size;
}

static inline u32 sunxi_irq_status_reg(const struct sunxi_pinctrl_desc *desc,
				       u16 irq)
{
	u8 bank = irq / IRQ_PER_BANK;

	return sunxi_irq_status_reg_from_bank(desc, bank);
}

static inline u32 sunxi_irq_status_offset(u16 irq)
{
	u32 irq_num = irq % IRQ_STATUS_IRQ_PER_REG;
	return irq_num * IRQ_STATUS_IRQ_BITS;
}

static inline u32 sunxi_grp_config_reg(u16 pin)
{
	u8 bank = pin / PINS_PER_BANK;

	return GRP_CFG_REG + bank * 0x4;
}

int sunxi_bsp_pinctrl_init_with_variant(struct platform_device *pdev,
				    const struct sunxi_pinctrl_desc *desc,
				    unsigned long variant);

#define sunxi_bsp_pinctrl_init(_dev, _desc) \
	sunxi_bsp_pinctrl_init_with_variant(_dev, _desc, 0)

#endif /* __PINCTRL_SUNXI_H */

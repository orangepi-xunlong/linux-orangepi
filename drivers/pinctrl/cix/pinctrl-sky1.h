/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Author: Jerry Zhu <Jerry.Zhu@cixtech.com>
 */

#ifndef __DRIVERS_PINCTRL_SKY1_H
#define __DRIVERS_PINCTRL_SKY1_H

#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinmux.h>

/**
 * struct sky1_pin - describes a single SKY1 pin
 * @pin_id: the pin id of this pin
 * @offest: the iomux register offset
 * @configs: the mux and config vaule for pin
 */
struct sky1_pin {
	unsigned int pin_id;
	unsigned int offset;
	unsigned long configs;
};

/**
 * sky1_pin_reg contains 32 bits
 * bit7:bit8 for function select
 * bit0:bit6 for pad configuration
 */
typedef u32 sky1_pin_reg;

/**
 * @dev: a pointer back to containing device
 * @base: the offset to the controller in virtual memory
 */
struct sky1_pinctrl {
	struct device *dev;
	struct pinctrl_dev *pctl;
	void __iomem *base;
	const struct sky1_pinctrl_soc_info *info;
	sky1_pin_reg *pin_regs;
	unsigned int group_index;
	struct mutex mutex;
};

struct sky1_pinctrl_soc_info {
	const struct pinctrl_pin_desc *pins;
	unsigned int npins;
};

#define SKY1_PINCTRL_PIN(pin) PINCTRL_PIN(pin, #pin)

int sky1_base_pinctrl_probe(struct platform_device *pdev,
			const struct sky1_pinctrl_soc_info *info);

#endif /* __DRIVERS_PINCTRL_SKY1_H */

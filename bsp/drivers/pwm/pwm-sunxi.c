/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinnertech pulse-width-modulation controller driver
 *
 * Copyright (C) 2015 AllWinner
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/* #define DEBUG */
#define SUNXI_MODNAME "pwm"
#include <sunxi-log.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/pwm.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>
#include <linux/spinlock.h>
#include "pwm-sunxi.h"

#define PWM_NUM_MAX 16
#define PWM_BIND_NUM 2
#define PWM_PIN_STATE_ACTIVE "active"
#define PWM_PIN_STATE_SLEEP "sleep"
#define SUNXI_PWM_BIND_DEFAULT 255
#define PRESCALE_MAX 256
#define SUNXI_PWM_NORMAL	1
#define SUNXI_PWM_INVERSED	0
#define SUNXI_PWM_SINGLE	1
#define SUNXI_PWM_DUAL		2
#define SUNXI_CLK_100M		100000000
#define SUNXI_CLK_24M		24000000
#define SUNXI_DIV_CLK		1000000000

#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	    (((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	    (((reg) & CLRMASK(width, shift)) | (val << (shift)))

struct sunxi_pwm_config {
	unsigned int dead_time;
	unsigned int bind_pwm;
};

struct group_pwm_config {
	unsigned int group_channel;
	unsigned int group_run_count;
	unsigned int pwm_polarity;
	int pwm_period;
};

struct sunxi_pwm_hw_data {
	u32 pdzcr01_offset;  /* PWM dead zone control register 01 */
	u32 pdzcr23_offset;  /* PWM dead zone control register 23 */
	u32 pdzcr45_offset;  /* PWM dead zone control register 45 */
	u32 per_offset;  /* PWM enable register */
	u32 cer_offset;  /* PWM capture enable register */
	u32 ver_reg_offset;  /* PWM version register */
	u32 pcr_base_offset;  /* PWM control register */
	u32 ppr_base_offset;  /* PWM period register */
	u32 pcntr_base_offset;  /* PWM counter register */
	u32 ccr_base_offset;  /* PWM capture control register */
	u32 crlr_base_offset;  /* PWM capture rise lock register */
	u32 cflr_base_offset;  /* PWM capture fall lock register */
	u32 pwm_reg_uniform_offset; /* multiple registers use uniform offset */
	int pm_regs_num;  /* PWM pm related register length */
	bool clk_gating_separate;  /* PWM clk gating register whether to separate */
	bool has_clock;  /* flags for the existence of clock and reset resources */
	bool has_bus_clock;  /* PWM multiple clocks need to distinguish */
	bool has_hosc_clock;  /* PWM multiple hosc need to distinguish */
	bool config_status;  /* whether to configure pwmchip status in dtsi */
};

struct sunxi_pwm_chip {
	struct sunxi_pwm_hw_data *data;
	u32 *regs_backup;
	u32 *pm_regs_offset;
	int irq;
	int index;
	wait_queue_head_t wait;
	unsigned long cap_time[3];
	struct pwm_chip pwm_chip;
	void __iomem *base;
	struct sunxi_pwm_config *config;
	struct clk *clk;
	struct clk *bclk;
	struct clk *hosc;
	struct reset_control *reset;
	struct regulator *regulator[PWM_NUM_MAX];
	unsigned int group_ch;
	unsigned int group_polarity;
	unsigned int group_period;
#if IS_ENABLED(CONFIG_AW_EPHY)
	unsigned int ephy_channel;
#endif /* CONFIG_AW_EPHY */
	struct pinctrl *pctl;
	unsigned int cells_num;
	bool status;
	bool channel_polarity_flag[PWM_NUM_MAX]; /* init set pwm polarity flag  */
	bool resume_polarity_flag[PWM_NUM_MAX]; /* resmue set pwm polarity flag */
	spinlock_t lock;
};

static struct sunxi_pwm_hw_data sunxi_pwm_v100_data = {
	.pdzcr01_offset = 0x0030,
	.pdzcr23_offset = 0x0034,
	.pdzcr45_offset = 0x0038,
	.per_offset = 0x0040,
	.cer_offset = 0x0044,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0060,
	.ppr_base_offset = 0x0060 + 0x0004,
	.pcntr_base_offset = 0x0060 + 0x0008,
	.ccr_base_offset = 0x0060 + 0x000c,
	.crlr_base_offset = 0x0060 + 0x0010,
	.cflr_base_offset = 0x0060 + 0x0014,
	.clk_gating_separate = 0,
	.pm_regs_num = 4,
	.pwm_reg_uniform_offset = 32,
	.has_clock = true,
	.has_bus_clock = false,
	.has_hosc_clock = false,
	.config_status = false,
};

static struct sunxi_pwm_hw_data sunxi_pwm_v101_data = {
	.pdzcr01_offset = 0x0030,
	.pdzcr23_offset = 0x0034,
	.pdzcr45_offset = 0x0038,
	.per_offset = 0x0040,
	.cer_offset = 0x0044,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0060,
	.ppr_base_offset = 0x0060 + 0x0004,
	.pcntr_base_offset = 0x0060 + 0x0008,
	.ccr_base_offset = 0x0060 + 0x000c,
	.crlr_base_offset = 0x0060 + 0x0010,
	.cflr_base_offset = 0x0060 + 0x0014,
	.clk_gating_separate = 0,
	.pm_regs_num = 4,
	.pwm_reg_uniform_offset = 32,
	.has_clock = false,
	.has_bus_clock = false,
	.has_hosc_clock = false,
	.config_status = false,
};

static struct sunxi_pwm_hw_data sunxi_pwm_v200_data = {
	.pdzcr01_offset = 0x0060,
	.pdzcr23_offset = 0x0064,
	.pdzcr45_offset = 0x0068,
	.per_offset = 0x0080,
	.cer_offset = 0x00c0,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0100,
	.ppr_base_offset = 0x0100 + 0x0004,
	.pcntr_base_offset = 0x0100 + 0x0008,
	.ccr_base_offset = 0x0100 + 0x0010,
	.crlr_base_offset = 0x0100 + 0x0014,
	.cflr_base_offset = 0x0100 + 0x0018,
	.clk_gating_separate = 1,
	.pm_regs_num = 5,
	.pwm_reg_uniform_offset = 32,
	.has_clock = true,
	.has_bus_clock = false,
	.has_hosc_clock = false,
	.config_status = false,
};

static struct sunxi_pwm_hw_data sunxi_pwm_v201_data = {
	.pdzcr01_offset = 0x0060,
	.pdzcr23_offset = 0x0064,
	.pdzcr45_offset = 0x0068,
	.per_offset = 0x0080,
	.cer_offset = 0x00c0,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0100,
	.ppr_base_offset = 0x0100 + 0x0004,
	.pcntr_base_offset = 0x0100 + 0x0008,
	.ccr_base_offset = 0x0100 + 0x0010,
	.crlr_base_offset = 0x0100 + 0x0014,
	.cflr_base_offset = 0x0100 + 0x0018,
	.clk_gating_separate = 1,
	.pm_regs_num = 5,
	.pwm_reg_uniform_offset = 32,
	.has_clock = true,
	.has_bus_clock = false,
	.has_hosc_clock = false,
	.config_status = true,
};

static struct sunxi_pwm_hw_data sunxi_pwm_v202_data = {
	.pdzcr01_offset = 0x0060,
	.pdzcr23_offset = 0x0064,
	.pdzcr45_offset = 0x0068,
	.per_offset = 0x0080,
	.cer_offset = 0x00c0,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0100,
	.ppr_base_offset = 0x0100 + 0x0004,
	.pcntr_base_offset = 0x0100 + 0x0008,
	.ccr_base_offset = 0x0100 + 0x0010,
	.crlr_base_offset = 0x0100 + 0x0014,
	.cflr_base_offset = 0x0100 + 0x0018,
	.clk_gating_separate = 1,
	.pm_regs_num = 5,
	.pwm_reg_uniform_offset = 32,
	.has_clock = true,
	.has_bus_clock = true,
	.has_hosc_clock = false,
	.config_status = true,
};

static struct sunxi_pwm_hw_data sunxi_pwm_v203_data = {
	.pdzcr01_offset = 0x0060,
	.pdzcr23_offset = 0x0064,
	.pdzcr45_offset = 0x0068,
	.per_offset = 0x0080,
	.cer_offset = 0x00c0,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0100,
	.ppr_base_offset = 0x0100 + 0x0004,
	.pcntr_base_offset = 0x0100 + 0x0008,
	.ccr_base_offset = 0x0100 + 0x0010,
	.crlr_base_offset = 0x0100 + 0x0014,
	.cflr_base_offset = 0x0100 + 0x0018,
	.clk_gating_separate = 1,
	.pm_regs_num = 5,
	.pwm_reg_uniform_offset = 64,
	.has_clock = true,
	.has_bus_clock = false,
	.has_hosc_clock = false,
	.config_status = true,
};

static struct sunxi_pwm_hw_data sunxi_pwm_v204_data = {
	.pdzcr01_offset = 0x0060,
	.pdzcr23_offset = 0x0064,
	.pdzcr45_offset = 0x0068,
	.per_offset = 0x0080,
	.cer_offset = 0x00c0,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0100,
	.ppr_base_offset = 0x0100 + 0x0004,
	.pcntr_base_offset = 0x0100 + 0x0008,
	.ccr_base_offset = 0x0100 + 0x0010,
	.crlr_base_offset = 0x0100 + 0x0014,
	.cflr_base_offset = 0x0100 + 0x0018,
	.clk_gating_separate = 1,
	.pm_regs_num = 5,
	.pwm_reg_uniform_offset = 64,
	.has_clock = true,
	.has_bus_clock = true,
	.has_hosc_clock = false,
	.config_status = true,
};

static struct sunxi_pwm_hw_data sunxi_pwm_v205_data = {
	.pdzcr01_offset = 0x0060,
	.pdzcr23_offset = 0x0064,
	.pdzcr45_offset = 0x0068,
	.per_offset = 0x0080,
	.cer_offset = 0x00c0,
	.ver_reg_offset = 0x0050,
	.pcr_base_offset = 0x0100,
	.ppr_base_offset = 0x0100 + 0x0004,
	.pcntr_base_offset = 0x0100 + 0x0008,
	.ccr_base_offset = 0x0100 + 0x0010,
	.crlr_base_offset = 0x0100 + 0x0014,
	.cflr_base_offset = 0x0100 + 0x0018,
	.clk_gating_separate = 1,
	.pm_regs_num = 5,
	.pwm_reg_uniform_offset = 32,
	.has_clock = true,
	.has_bus_clock = false,
	.has_hosc_clock = true,
	.config_status = true,
};

/*
 * The two channel numbers share one register, the relationship
 * is as follows:
 *
 * reg offset  : | PWM_PCCR01 | PWM_PCCR23 | PWM_PCCR45 | ... |
 * channal num : |  0     1   |  2     3   |  4     5   | ... |
 *
 */
static int sunxi_pwm_regs[] = {
	PWM_PCCR01,
	PWM_PCCR23,
	PWM_PCCR45,
	PWM_PCCR67,
	PWM_PCCR89,
	PWM_PCCRAB,
	PWM_PCCRCD,
	PWM_PCCREF
};

static u32 sunxi_pwm_pre_scal[][2] = {
	/* reg_value  clk_pre_div */
	{0, 1},
	{1, 2},
	{2, 4},
	{3, 8},
	{4, 16},
	{5, 32},
	{6, 64},
	{7, 128},
	{8, 256},
};

static inline void sunxi_pwm_save_regs(struct sunxi_pwm_chip *chip)
{
	int i;

	for (i = 0; i < chip->data->pm_regs_num; i++)
		chip->regs_backup[i] = readl(chip->base + chip->pm_regs_offset[i]);
}

static inline void sunxi_pwm_restore_regs(struct sunxi_pwm_chip *chip)
{
	int i;

	for (i = 0; i < chip->data->pm_regs_num; i++)
		writel(chip->regs_backup[i], chip->base + chip->pm_regs_offset[i]);
}

static inline struct sunxi_pwm_chip *to_sunxi_pwm_chip(struct pwm_chip *pwm_chip)
{
	return container_of(pwm_chip, struct sunxi_pwm_chip, pwm_chip);
}

static u32 sunxi_pwm_readl(struct pwm_chip *pwm_chip, u32 offset)
{
	u32 value;
	struct sunxi_pwm_chip *chip;

	chip = to_sunxi_pwm_chip(pwm_chip);

	value = readl(chip->base + offset);
	sunxi_debug(chip->pwm_chip.dev, "%3u bytes fifo\n", value);

	return value;
}

static u32 sunxi_pwm_writel(struct pwm_chip *pwm_chip, u32 offset, u32 value)
{
	struct sunxi_pwm_chip *chip;

	chip = to_sunxi_pwm_chip(pwm_chip);

	writel(value, chip->base + offset);

	return 0;
}

static void sunxi_pwm_set_reg(struct pwm_chip *pwm_chip, u32 reg_offset,
				u32 reg_shift, u32 reg_width, int data)
{
	u32 value;

	value = sunxi_pwm_readl(pwm_chip, reg_offset);
	value = SET_BITS(reg_shift, reg_width, value, data);
	sunxi_pwm_writel(pwm_chip, reg_offset, value);

	return;

}

static int sunxi_pwm_regulator_request(struct sunxi_pwm_chip *chip, struct device *dev, int index)
{
	if (chip->regulator[index])
		return 0;

	chip->regulator[index] = regulator_get(dev, "pwm");
	if (IS_ERR(chip->regulator[index])) {
		sunxi_err(dev, "get supply failed!\n");
		return -EPROBE_DEFER;
	}

	return 0;
}

static int sunxi_pwm_regulator_release(struct sunxi_pwm_chip *chip)
{
	unsigned int pwm_number, i;

	pwm_number = chip->pwm_chip.npwm;
	for (i = 0; i < pwm_number; i++) {
		if (!chip->regulator[i])
			continue;

		regulator_put(chip->regulator[i]);
		chip->regulator[i] = NULL;
	}

	return 0;
}

static int sunxi_pwm_regulator_enable(struct sunxi_pwm_chip *chip, int index)
{
	if (!chip->regulator[index])
		return 0;

	if (regulator_enable(chip->regulator[index])) {
		sunxi_err(chip->pwm_chip.dev, "enable regulator failed!\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_pwm_regulator_disable(struct sunxi_pwm_chip *chip, int index)
{
	if (!chip->regulator[index])
		return 0;

	if (regulator_is_enabled(chip->regulator[index]))
		regulator_disable(chip->regulator[index]);

	return 0;
}
static int sunxi_pwm_pin_set_state(struct device *dev, char *name)
{
	struct pinctrl *pctl;
	struct pinctrl_state *state = NULL;
	int err;

	pctl = devm_pinctrl_get(dev);
	if (IS_ERR(pctl)) {
		sunxi_err(dev, "pinctrl_get failed\n");
		err = PTR_ERR(pctl);
		return err;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		sunxi_err(dev, "pinctrl_lookup_state(%s) failed\n", name);
		err = PTR_ERR(state);
		goto exit;
	}

	err = pinctrl_select_state(pctl, state);
	if (err) {
		sunxi_err(dev, "pinctrl_select_state(%s) failed\n", name);
		goto exit;
	}

exit:
	devm_pinctrl_put(pctl);
	return err;

}

static int sunxi_pwm_get_config(struct platform_device *pdev,
				struct sunxi_pwm_config *config)
{
	int err;
	struct device_node *np;

	np = pdev->dev.of_node;

	err = of_property_read_u32(np, "bind_pwm", &config->bind_pwm);
	if (err < 0) {
		/* if there is no bind pwm,set 255, dual pwm invalid! */
		config->bind_pwm = SUNXI_PWM_BIND_DEFAULT;
		err = 0;
	}

	err = of_property_read_u32(np, "dead_time", &config->dead_time);
	if (err < 0) {
		/* if there is bind pwm, but not set dead time,set bind pwm 255,dual pwm invalid! */
		config->bind_pwm = SUNXI_PWM_BIND_DEFAULT;
		err = 0;
	}

	of_node_put(np);

	return err;
}

static int sunxi_pwm_set_polarity(struct pwm_chip *pwm_chip, struct pwm_device *pwm,
				enum pwm_polarity polarity)
{
	int bind_num, mode_num, i;
	u32 temp[2], index[2] = {0};
	struct sunxi_pwm_chip *chip;
	unsigned int reg_offset[2], reg_shift, reg_width;

	chip = to_sunxi_pwm_chip(pwm_chip);
	index[0] = pwm->hwpwm;
	reg_shift = PWM_ACT_STA_SHIFT;
	reg_width = PWM_ACT_STA_WIDTH;

	bind_num = chip->config[pwm->hwpwm].bind_pwm;
	if (bind_num == SUNXI_PWM_BIND_DEFAULT) {
		mode_num = SUNXI_PWM_SINGLE;
	} else {
		mode_num = SUNXI_PWM_DUAL;
		index[1] = bind_num - pwm_chip->base;
	}

	for (i = 0; i < mode_num; i++) {
		reg_offset[i] = chip->data->pcr_base_offset + index[i] * chip->data->pwm_reg_uniform_offset;
		temp[i] = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
	}

	/*
	 * config current pwm
	 * bind pwm's polarity is reverse compare with the current pwm
	 */

	spin_lock(&chip->lock);

	if (polarity == PWM_POLARITY_NORMAL)
		temp[0] = SET_BITS(reg_shift, reg_width, temp[0], SUNXI_PWM_NORMAL);
	else
		temp[0] = SET_BITS(reg_shift, reg_width, temp[0], SUNXI_PWM_INVERSED);

	if (mode_num == SUNXI_PWM_DUAL) {
		if (polarity == PWM_POLARITY_NORMAL) {
			temp[1] = SET_BITS(reg_shift, reg_width, temp[1], SUNXI_PWM_INVERSED);
		} else {
			temp[1] = SET_BITS(reg_shift, reg_width, temp[1], SUNXI_PWM_NORMAL);
		}
	}

	/* config register at the same time */
	for (i = 0; i < mode_num; i++)
		sunxi_pwm_writel(pwm_chip, reg_offset[i], temp[i]);

	spin_unlock(&chip->lock);

	return 0;
}

static int get_pdzcr_reg_offset(struct sunxi_pwm_chip *chip, u32 sel, u32 *reg_offset)
{
	switch (sel) {
	case 0:
	case 1:
		*reg_offset = chip->data->pdzcr01_offset;
		break;
	case 2:
	case 3:
		*reg_offset = chip->data->pdzcr23_offset;
		break;
	case 4:
	case 5:
		*reg_offset = chip->data->pdzcr45_offset;
		break;
	case 6:
	case 7:
		*reg_offset = PWM_PDZCR67;
		break;
	case 8:
	case 9:
		*reg_offset = PWM_PDZCR89;
		break;
	case 10:
	case 11:
		*reg_offset = PWM_PDZCRAB;
		break;
	case 12:
	case 13:
		*reg_offset = PWM_PDZCRCD;
		break;
	case 14:
	case 15:
		*reg_offset = PWM_PDZCREF;
		break;
	default:
		sunxi_err(NULL, "%s:Not supported!\n", __func__);
		return -EINVAL;
	}
	return 0;
}

/*
 * write when the period_offset is 0; 0: idle; 1: busy
 * pwm_chip: pwm controller
 * period_flag_offset: period flag register
 * period_crtl_offset: period control register
 * val: Write the value of the period control register
 */
static void sunxi_pwm_wait_for_write(struct pwm_chip *pwm_chip, unsigned int period_flag_offset,
		unsigned int period_crtl_offset, unsigned int val)
{
	unsigned int period_ready_val;
	unsigned long timeout;

	timeout = jiffies + PWM_TIMEOUT;
	do {
		period_ready_val = sunxi_pwm_readl(pwm_chip, period_flag_offset);
		period_ready_val &= PWM_PERIOD_READY_MASK;
		if (!period_ready_val)
			sunxi_pwm_writel(pwm_chip, period_crtl_offset, val);

		if (time_after(jiffies, timeout)) {
			sunxi_debug(pwm_chip->dev, "wait up to 50us, force write register\n");
			sunxi_pwm_writel(pwm_chip, period_crtl_offset, val);
			return;
		}
	} while (period_ready_val);

	return;
}

/*
* This function is used to get the clock source
* c == 0: direct output 100M clock,set by pass
* c == -EINVAL: period of input errors
* c > 0: get the clock source
*/
static long sunxi_pwm_config_clk(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device,
					int duty_ns, int period_ns, int mode_num)
{
	struct sunxi_pwm_chip *chip;
	int src_clk_sel, i, bind_num;
	unsigned int index[2] = {0};
	unsigned int reg_bypass_shift, reg_offset[2];
	unsigned long long clk = 0;

	chip = to_sunxi_pwm_chip(pwm_chip);

	bind_num = chip->config[pwm_device->hwpwm].bind_pwm;
	index[0] = pwm_device->hwpwm;
	reg_offset[0] = sunxi_pwm_regs[index[0] >> 0x1];

	if (mode_num == SUNXI_PWM_DUAL) {
		index[1] = bind_num - pwm_chip->base;
		reg_offset[1] = sunxi_pwm_regs[index[1] >> 0x1];
	}

	if (period_ns > 0 && period_ns <= 10) {
		/* if freq lt 100M, then direct output 100M clock,set by pass */
		src_clk_sel = 1;
		for (i = 0; i < mode_num; i++) {
			/* config the two pwm bypass */
			if (!chip->data->clk_gating_separate) {
				if ((index[i] % 2) == 0)
					reg_bypass_shift = PWM_BYPASS_SHIFT;
				else
					reg_bypass_shift = PWM_BYPASS_SHIFT + 1;
				sunxi_pwm_set_reg(pwm_chip, reg_offset[i], reg_bypass_shift, PWM_BYPASS_WIDTH, 1);
			} else {
				reg_bypass_shift = index[i] + PWM_CGR_BYPASS_SHIFT;
				sunxi_pwm_set_reg(pwm_chip, PWM_PCGR, reg_bypass_shift, PWM_BYPASS_WIDTH, 1);
			}
			/* config the two pwm clock source as clk_src1 */
			sunxi_pwm_set_reg(pwm_chip, reg_offset[i], PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, src_clk_sel);
		}

		return 0;
	} else if (period_ns > 10 && period_ns <= 334) {
		/* if freq between 3M~100M, then select 100M as clock */
		clk = SUNXI_CLK_100M;
		src_clk_sel = 1;
	} else if (period_ns > 334) {
		/* if freq < 3M, then select 24M/40M clock */
		clk = chip->data->has_hosc_clock ? \
		clk_get_rate(chip->hosc) \
		: SUNXI_CLK_24M;
		src_clk_sel = 0;
	} else {
		sunxi_err(pwm_chip->dev, "The frequency range should be between 0HZ and 100MHZ\n");
		return -EINVAL;
	}

	for (i = 0; i < mode_num; i++) {
		reg_offset[i] = sunxi_pwm_regs[index[i] >> 0x1];
		sunxi_pwm_set_reg(pwm_chip, reg_offset[i], PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, src_clk_sel);
	}

	return clk;
}

static int sunxi_pwm_config_single(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device,
		int duty_ns, int period_ns)
{
	unsigned int temp;
	unsigned long long c = 0;
	unsigned long entire_cycles = 256, active_cycles = 192;
	unsigned int reg_offset, period_offset;
	unsigned int pre_scal_id = 0, div_m = 0, prescale = 0;
	unsigned int pwm_run_count = 0, value;
	struct sunxi_pwm_chip *chip;
	struct group_pwm_config *pdevice;
	u32 sel = 0;

	chip = to_sunxi_pwm_chip(pwm_chip);
	pdevice = pwm_device->chip_data;

	if (pwm_device->chip_data) {
		pwm_run_count = pdevice->group_run_count;
		chip->group_ch = pdevice->group_channel;
		chip->group_polarity = pdevice->pwm_polarity;
		chip->group_period = pdevice->pwm_period;
	}

	if (chip->group_ch) {
		reg_offset = chip->data->per_offset;
		value = sunxi_pwm_readl(pwm_chip, reg_offset);
		value &= ~((0xf) << 4*(chip->group_ch - 1));
		sunxi_pwm_writel(pwm_chip, reg_offset, value);
	}

	sel = pwm_device->hwpwm;

	/* sel / 2 * 0x04 + 0x20  */
	reg_offset = sunxi_pwm_regs[sel >> 0x1];
	sunxi_pwm_writel(pwm_chip, reg_offset, 0);

	if (chip->group_ch) {
		/* group_mode used the apb1 clk */
		sunxi_pwm_set_reg(pwm_chip, reg_offset, PWM_CLK_SRC_SHIFT, PWM_CLK_SRC_WIDTH, 0);
	} else {
		c = sunxi_pwm_config_clk(pwm_chip, pwm_device, duty_ns, period_ns, SUNXI_PWM_SINGLE);
		if (c <= 0) {
			/*  c = 0 || c == -EINVAL */
			return c;
		}
		c = c * period_ns;
		do_div(c, SUNXI_DIV_CLK);
		entire_cycles = (unsigned long)c;  /* How many clksrc beats in a PWM period */

		/* get entire cycle length */
		for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
			if (entire_cycles <= 65536)
				break;
			for (prescale = 0; prescale < PRESCALE_MAX+1; prescale++) {
				entire_cycles = ((unsigned long)c/sunxi_pwm_pre_scal[pre_scal_id][1])/(prescale + 1);
				if (entire_cycles <= 65536) {
					div_m = sunxi_pwm_pre_scal[pre_scal_id][0];
					break;
				}
			}
		}

		/*
		 * get active cycles/high level time
		 * active_cycles = entire_cycles * duty_ns / period_ns
		 */
		c = (unsigned long long)entire_cycles * duty_ns;
		do_div(c, period_ns);
		active_cycles = c;
		if (entire_cycles == 0)
			entire_cycles++;
	}

	/* config clk div_m */
	temp = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (chip->group_ch)
		temp = SET_BITS(PWM_DIV_M_SHIFT, PWM_DIV_M_WIDTH, temp, 0);
	else
		temp = SET_BITS(PWM_DIV_M_SHIFT, PWM_DIV_M_WIDTH, temp, div_m);
	sunxi_pwm_writel(pwm_chip, reg_offset, temp);

#if IS_ENABLED(CONFIG_AW_EPHY)
	if (chip->ephy_channel > 0) {
		sunxi_pwm_set_reg(pwm_chip, PWM_PCCR45, chip->ephy_channel + 1, 1, 1);
		sunxi_pwm_set_reg(pwm_chip, PWM_PCCR45, chip->ephy_channel - 1, 1, 1);
	}
#endif

	/* config prescal_k */
	reg_offset = chip->data->pcr_base_offset + chip->data->pwm_reg_uniform_offset * sel;
	temp = sunxi_pwm_readl(pwm_chip, reg_offset);

	if (chip->group_ch)
		temp = SET_BITS(PWM_PRESCAL_SHIFT, PWM_PRESCAL_WIDTH, temp, 0xef);
	else
		temp = SET_BITS(PWM_PRESCAL_SHIFT, PWM_PRESCAL_WIDTH, temp, prescale);
	sunxi_pwm_writel(pwm_chip, reg_offset, temp);

	if (chip->group_ch) {
		/* group set enable */
		reg_offset = PWM_PGR0 + 0x04 * (chip->group_ch - 1);
		sunxi_pwm_set_reg(pwm_chip, reg_offset, sel, 1, 1);

		/* pwm pulse mode set */
		reg_offset = chip->data->pcr_base_offset + sel * chip->data->pwm_reg_uniform_offset;
		temp = sunxi_pwm_readl(pwm_chip, reg_offset);
		temp = SET_BITS(PWM_MODE_ACTS_SHIFT, PWM_MODE_ACTS_WIDTH, temp, 0x3); /* pwm pulse mode and active */
		/* pwm output pulse num */
		temp = SET_BITS(PWM_PUL_NUM_SHIFT, PWM_PUL_NUM_WIDTH, temp, pwm_run_count);   /* pwm output pulse num */
		sunxi_pwm_writel(pwm_chip, reg_offset, temp);
	}

	/* config active cycles num and period cycles num */
	reg_offset = chip->data->ppr_base_offset + chip->data->pwm_reg_uniform_offset * sel;
	temp = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (chip->group_ch) {
		temp = SET_BITS(PWM_ACT_CYCLES_SHIFT, PWM_ACT_CYCLES_WIDTH, temp,
				(unsigned int)((chip->group_period * 3) >> 3));
		temp = SET_BITS(PWM_PERIOD_CYCLES_SHIFT, PWM_PERIOD_CYCLES_WIDTH, temp, chip->group_period);
		chip->group_ch = 0;
	} else {
		temp = SET_BITS(PWM_ACT_CYCLES_SHIFT, PWM_ACT_CYCLES_WIDTH, temp, active_cycles);
		temp = SET_BITS(PWM_PERIOD_CYCLES_SHIFT, PWM_PERIOD_CYCLES_WIDTH, temp, (entire_cycles - 1));
	}

	period_offset = chip->data->pcr_base_offset + chip->data->pwm_reg_uniform_offset * sel;
	sunxi_pwm_wait_for_write(pwm_chip, period_offset, reg_offset, temp);
	sunxi_debug(chip->pwm_chip.dev, "active_cycles=%lu entire_cycles=%lu prescale=%u div_m=%u\n",
			active_cycles, entire_cycles, prescale, div_m);

	return 0;
}

static int sunxi_pwm_config_dual(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device,
		int duty_ns, int period_ns, int bind_num)
{
	unsigned int temp;
	unsigned long long c = 0, clk = 0, clk_temp = 0;
	unsigned long reg_dead, entire_cycles = 256, active_cycles = 192;
	unsigned int reg_offset[2];
	unsigned int period_offset;
	unsigned int reg_dz_en_offset[2];
	unsigned int pre_scal_id = 0, div_m = 0, prescale = 0;
	int pwm_index[2] = {0};
	int i = 0;
	int err;
	unsigned int dead_time = 0, duty = 0;
	struct sunxi_pwm_chip *chip;

	chip = to_sunxi_pwm_chip(pwm_chip);

	pwm_index[0] = pwm_device->hwpwm;
	pwm_index[1] = bind_num - pwm_chip->base;

	if (pwm_index[1] < 0) {
		sunxi_err(chip->pwm_chip.dev, "pwm channel invalid\n");
		return -EINVAL;
	}

	dead_time = chip->config[pwm_index[0]].dead_time;
	duty = (unsigned int)duty_ns;
	/* judge if the pwm eanble dead zone */
	err = get_pdzcr_reg_offset(chip, pwm_index[0], &reg_dz_en_offset[0]);
	if (err) {
		sunxi_err(chip->pwm_chip.dev, "get pwm dead zone failed\n");
		return -EINVAL;
	}

	sunxi_pwm_set_reg(pwm_chip, reg_dz_en_offset[0], PWM_DZ_EN_SHIFT, PWM_DZ_EN_WIDTH, 1);
	temp = sunxi_pwm_readl(pwm_chip, reg_dz_en_offset[0]);
	temp &=  (1u << PWM_DZ_EN_SHIFT);
	/* if duty time < dead time or enable pwm dead zone failed, it is wrong */
	if (duty < dead_time || temp == 0) {
		sunxi_err(chip->pwm_chip.dev, "[PWM]duty time or dead zone error.\n");
		return -EINVAL;
	}

	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = sunxi_pwm_regs[pwm_index[i] >> 0x1];
		sunxi_pwm_writel(pwm_chip, reg_offset[i], 0);
	}

	clk = sunxi_pwm_config_clk(pwm_chip, pwm_device, duty_ns, period_ns, SUNXI_PWM_DUAL);
	if (!clk || clk == -EINVAL)
		return clk;
	c = clk;
	c *= period_ns;
	do_div(c, SUNXI_DIV_CLK);
	entire_cycles = (unsigned long)c;

	clk_temp = clk * dead_time;
	do_div(clk_temp, SUNXI_DIV_CLK);
	reg_dead = (unsigned long)clk_temp;
	/* get div_m and prescale,which satisfy: deat_val <= 256, entire <= 65536 */
	for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
		if (entire_cycles <= 65536 && reg_dead <= 256)
			break;

		for (prescale = 0; prescale < PRESCALE_MAX+1; prescale++) {
			entire_cycles = ((unsigned long)c/sunxi_pwm_pre_scal[pre_scal_id][1])/(prescale + 1);
			do_div(clk_temp, sunxi_pwm_pre_scal[pre_scal_id][1] * (prescale + 1));
			reg_dead = (unsigned long)clk_temp;
			if (entire_cycles <= 65536 && reg_dead <= 256) {
				div_m = sunxi_pwm_pre_scal[pre_scal_id][0];
				break;
			}
		}
	}

	c = (unsigned long long)entire_cycles * duty_ns;
	do_div(c,  period_ns);
	active_cycles = c;
	if (entire_cycles == 0)
		entire_cycles++;

	/* config  clk div_m */
	for (i = 0; i < PWM_BIND_NUM; i++)
		sunxi_pwm_set_reg(pwm_chip, reg_offset[i], PWM_DIV_M_SHIFT, PWM_DIV_M_SHIFT, div_m);

	/* config prescal */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = chip->data->pcr_base_offset + chip->data->pwm_reg_uniform_offset * pwm_index[i];
		sunxi_pwm_set_reg(pwm_chip, reg_offset[i], PWM_PRESCAL_SHIFT, PWM_PRESCAL_WIDTH, prescale);
	}

	/* config active cycles and period cycles */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = chip->data->ppr_base_offset + chip->data->pwm_reg_uniform_offset * pwm_index[i];
		temp = sunxi_pwm_readl(pwm_chip, reg_offset[i]);
		temp = SET_BITS(PWM_ACT_CYCLES_SHIFT, PWM_ACT_CYCLES_WIDTH, temp, active_cycles);
		temp = SET_BITS(PWM_PERIOD_CYCLES_SHIFT, PWM_PERIOD_CYCLES_WIDTH, temp, (entire_cycles - 1));

		period_offset = chip->data->pcr_base_offset + chip->data->pwm_reg_uniform_offset * pwm_index[i];
		sunxi_pwm_wait_for_write(pwm_chip, period_offset, reg_offset[i], temp);
	}

	sunxi_debug(chip->pwm_chip.dev, "active_cycles=%lu entire_cycles=%lu prescale=%u div_m=%u\n",
			active_cycles, entire_cycles, prescale, div_m);

	/* config dead zone, one config for two pwm */
	reg_offset[0] = reg_dz_en_offset[0];
	sunxi_pwm_set_reg(pwm_chip, reg_offset[0], PWM_PDZINTV_SHIFT, PWM_PDZINTV_WIDTH, (unsigned int)reg_dead);

	return 0;
}

static int sunxi_pwm_config_channel(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device,
		int duty_ns, int period_ns)
{
	int bind_num, ret;

	struct sunxi_pwm_chip *chip;

	chip = to_sunxi_pwm_chip(pwm_chip);

	bind_num = chip->config[pwm_device->hwpwm].bind_pwm;
	if (bind_num == SUNXI_PWM_BIND_DEFAULT)
		ret = sunxi_pwm_config_single(pwm_chip, pwm_device, duty_ns, period_ns);
	else
		ret = sunxi_pwm_config_dual(pwm_chip, pwm_device, duty_ns, period_ns, bind_num);

	return ret;
}

static int sunxi_pwm_enable_single(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device)
{
	unsigned int value = 0, index = 0;
	unsigned int reg_offset, reg_shift, reg_width, group_reg_offset;
	unsigned int temp;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;
	static unsigned int enable_num;
	unsigned int pwm_start_count, i;
	int pwm_period = 0;
	int err;
	struct sunxi_pwm_chip *chip;
	struct group_pwm_config *pdevice;

	chip = to_sunxi_pwm_chip(pwm_chip);
	pdevice = pwm_device->chip_data;

	index = pwm_device->hwpwm;
	sub_np = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", index);
	if (!sub_np) {
		sunxi_err(chip->pwm_chip.dev, "can't parse \"sunxi-pwms\" property\n");
		return -ENODEV;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (!pwm_pdevice) {
		sunxi_err(chip->pwm_chip.dev, "can't parse pwm device\n");
		return -ENODEV;
	}

	err = sunxi_pwm_regulator_request(chip, &pwm_pdevice->dev, index);
	if (err) {
		sunxi_err(chip->pwm_chip.dev, "request regulator failed!\n");
		return err;
	}

	err = sunxi_pwm_regulator_enable(chip, index);
	if (err) {
		sunxi_err(chip->pwm_chip.dev, "enable regulator failed!\n");
		goto err0;
	}

	err = sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_ACTIVE);
	if (err != 0)
		goto err1;

	if (pwm_device->chip_data) {
		chip->group_ch = pdevice->group_channel;
		pwm_period = pdevice->pwm_period;
	}

	if (chip->group_ch) {
		enable_num++;
		/* config clk gating */
		if (!chip->data->clk_gating_separate) {
			reg_offset = sunxi_pwm_regs[index >> 0x1];
			reg_shift = PWM_CLK_GATING_SHIFT;
			reg_width = PWM_CLK_GATING_WIDTH;
		} else {
			reg_offset = PWM_PCGR;
			reg_shift = index;
			reg_width = 0x1;
		}
		value = sunxi_pwm_readl(pwm_chip, reg_offset);
		value |= ((0xf) << 4*(chip->group_ch - 1));
		/* value = SET_BITS(reg_shift, reg_width, value, 0); */
		sunxi_pwm_writel(pwm_chip, reg_offset, value);
	}

	/* enable pwm controller  pwm can be used */
	if (!chip->group_ch) {
		/* pwm channel enable */
		reg_offset = chip->data->per_offset;
		sunxi_pwm_set_reg(pwm_chip, reg_offset, index, 1, 1);
		/* config clk gating */
		if (!chip->data->clk_gating_separate) {
			reg_offset = sunxi_pwm_regs[index >> 0x1];
			reg_shift = PWM_CLK_GATING_SHIFT;
			reg_width = PWM_CLK_GATING_WIDTH;
		} else {
			reg_offset = PWM_PCGR;
			reg_shift = index;
			reg_width = 0x1;
		}
		sunxi_pwm_set_reg(pwm_chip, reg_offset, reg_shift, reg_width, 1);
	}

	if (chip->group_ch && enable_num == 4) {
		if (chip->group_polarity)
			pwm_start_count = (unsigned int)pwm_period*6/8;
		else
			pwm_start_count = 0;

		for (i = 4 * (chip->group_ch - 1); i < 4 * chip->group_ch; i++) {
			/* start count set */
			reg_offset = chip->data->pcntr_base_offset + chip->data->pwm_reg_uniform_offset * i;

			temp = pwm_start_count << PWM_COUNTER_START_SHIFT;
			sunxi_pwm_writel(pwm_chip, reg_offset, temp);
			if (chip->group_polarity)
				pwm_start_count = pwm_start_count -
					((unsigned int)pwm_period*2/8);
			else
				pwm_start_count = pwm_start_count +
					((unsigned int)pwm_period*2/8);
		}

		/* pwm channel enable */
		reg_offset = chip->data->per_offset;
		value = sunxi_pwm_readl(pwm_chip, reg_offset);
		value |= ((0xf) << 4*(chip->group_ch - 1));
		sunxi_pwm_writel(pwm_chip, reg_offset, value);

		/* pwm group control */
		group_reg_offset = PWM_PGR0 + 0x04 * (chip->group_ch - 1);
		enable_num = 0;
		pwm_start_count = 0;

		/* group enable and start */
		sunxi_pwm_set_reg(pwm_chip, group_reg_offset, PWMG_EN_SHIFT, 1, 1);
		sunxi_pwm_set_reg(pwm_chip, group_reg_offset, PWMG_START_SHIFT, 1, 1);
		chip->group_ch = 0;
	}

	return 0;

err1:
	sunxi_pwm_regulator_disable(chip, index);
err0:
	sunxi_pwm_regulator_release(chip);
	return err;
}

static int sunxi_pwm_enable_dual(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device, int bind_num)
{
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	struct device_node *sub_np[2];
	struct platform_device *pwm_pdevice[2];
	int i, err;
	unsigned int pwm_index[2] = {0};
	struct sunxi_pwm_chip *chip;

	chip = to_sunxi_pwm_chip(pwm_chip);

	pwm_index[0] = pwm_device->hwpwm;
	pwm_index[1] = bind_num - pwm_chip->base;

	/* get current index pwm device */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		sub_np[i] = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", pwm_index[i]);
		if (IS_ERR_OR_NULL(sub_np[i])) {
			sunxi_err(chip->pwm_chip.dev, "can't parse \"sunxi-pwms\" property\n");
			return -ENODEV;
		}
		pwm_pdevice[i] = of_find_device_by_node(sub_np[i]);
		if (IS_ERR_OR_NULL(pwm_pdevice[i])) {
			sunxi_err(chip->pwm_chip.dev, "can't parse pwm device\n");
			return -ENODEV;
		}
	}

	err = sunxi_pwm_regulator_request(chip, &pwm_pdevice[0]->dev, pwm_index[0]);
	if (err) {
		sunxi_err(chip->pwm_chip.dev, "request regulator failed!\n");
		return err;
	}

	err = sunxi_pwm_regulator_enable(chip, pwm_index[0]);
	if (err) {
		sunxi_err(chip->pwm_chip.dev, "enable regulator failed!\n");
		goto err0;
	}

	err = sunxi_pwm_pin_set_state(&pwm_pdevice[0]->dev, PWM_PIN_STATE_ACTIVE);
	if (err)
		goto err1;

	err = sunxi_pwm_regulator_request(chip, &pwm_pdevice[1]->dev, pwm_index[1]);
	if (err) {
		sunxi_err(chip->pwm_chip.dev, "request regulator failed!\n");
		goto err1;
	}

	err = sunxi_pwm_regulator_enable(chip, pwm_index[1]);
	if (err) {
		sunxi_err(chip->pwm_chip.dev, "enable regulator failed!\n");
		goto err2;
	}

	err = sunxi_pwm_pin_set_state(&pwm_pdevice[1]->dev, PWM_PIN_STATE_ACTIVE);
	if (err)
		goto err3;

	/* enable clk gating for pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		if (!chip->data->clk_gating_separate) {
			reg_offset[i] = sunxi_pwm_regs[pwm_index[i] >> 0x1];
			reg_shift[i] = PWM_CLK_GATING_SHIFT;
			reg_width[i] = PWM_CLK_GATING_WIDTH;
		} else {
			reg_offset[i] = PWM_PCGR;
			reg_shift[i] = pwm_index[i];
			reg_width[i] = 0x1;
		}
		sunxi_pwm_set_reg(pwm_chip, reg_offset[i], reg_shift[i], reg_width[i], 1);
	}

	/* enable pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = chip->data->per_offset;
		sunxi_pwm_set_reg(pwm_chip, reg_offset[i], pwm_index[i], 0x1, 1);
	}

	return 0;

err3:
	sunxi_pwm_regulator_disable(chip, pwm_index[1]);
err2:
	sunxi_pwm_regulator_release(chip);
err1:
	sunxi_pwm_regulator_disable(chip, pwm_index[0]);
err0:
	sunxi_pwm_regulator_release(chip);
	return err;
}

static int sunxi_pwm_enable(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device)
{
	int bind_num;
	int ret;
	struct sunxi_pwm_chip *chip;

	chip = to_sunxi_pwm_chip(pwm_chip);

	bind_num = chip->config[pwm_device->hwpwm].bind_pwm;
	if (bind_num == SUNXI_PWM_BIND_DEFAULT)
		ret = sunxi_pwm_enable_single(pwm_chip, pwm_device);
	else
		ret = sunxi_pwm_enable_dual(pwm_chip, pwm_device, bind_num);

	return ret;
}

static void sunxi_pwm_disable_single(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device)
{
	u32 value = 0, index = 0;
	unsigned int reg_offset, reg_shift, reg_width, group_reg_offset;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;

	static int disable_num;
	struct sunxi_pwm_chip *chip;
	struct group_pwm_config *pdevice;

	chip = to_sunxi_pwm_chip(pwm_chip);
	pdevice = pwm_device->chip_data;

	index = pwm_device->hwpwm;

	if (pwm_device->chip_data)
		chip->group_ch = pdevice->group_channel;

	/* disable pwm controller */
	if (chip->group_ch) {
		if (disable_num == 0) {
			reg_offset = chip->data->per_offset;
			reg_width = 0x4;
			value = sunxi_pwm_readl(pwm_chip, reg_offset);
			value &= ~((0xf) << 4*(chip->group_ch - 1));
			sunxi_pwm_writel(pwm_chip, reg_offset, value);
			/* config clk gating */
			if (!chip->data->clk_gating_separate) {
				reg_offset = sunxi_pwm_regs[index >> 0x1];
				reg_shift = PWM_CLK_GATING_SHIFT;
				reg_width = PWM_CLK_GATING_WIDTH;
			} else {
				reg_offset = PWM_PCGR;
				reg_shift = index;
				reg_width = 0x1;
			}
			value = sunxi_pwm_readl(pwm_chip, reg_offset);
			value &= ~((0xf) << 4*(chip->group_ch - 1));
			/* value = SET_BITS(reg_shift, reg_width, value, 0); */
			sunxi_pwm_writel(pwm_chip, reg_offset, value);
		}
	} else {
		reg_offset = chip->data->per_offset;
		sunxi_pwm_set_reg(pwm_chip, reg_offset, index, 0x1, 0);

		/* config clk gating */
		if (!chip->data->clk_gating_separate) {
			reg_offset = sunxi_pwm_regs[index >> 0x1];
			reg_shift = PWM_CLK_GATING_SHIFT;
			reg_width = PWM_CLK_GATING_WIDTH;
		} else {
			reg_offset = PWM_PCGR;
			reg_shift = index;
			reg_width = 0x1;
		}
		sunxi_pwm_set_reg(pwm_chip, reg_offset, reg_shift, reg_width, 0);
	}

	if (chip->group_ch)
		disable_num++;

	sub_np = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", index);
	if (IS_ERR_OR_NULL(sub_np)) {
		sunxi_err(chip->pwm_chip.dev, "can't parse \"sunxi-pwms\" property\n");
		return;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (IS_ERR_OR_NULL(pwm_pdevice)) {
		sunxi_err(chip->pwm_chip.dev, "can't parse pwm device\n");
		return;
	}
	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_SLEEP);

	sunxi_pwm_regulator_disable(chip, index);

	if (chip->group_ch) {
		group_reg_offset = PWM_PGR0 + 0x04 * (chip->group_ch - 1);
		/* group end */
		sunxi_pwm_set_reg(pwm_chip, group_reg_offset, PWMG_START_SHIFT, 1, 0);

		/* group disable */
		sunxi_pwm_set_reg(pwm_chip, group_reg_offset, PWMG_EN_SHIFT, 1, 0);
		chip->group_ch = 0;
	}
}

static void sunxi_pwm_disable_dual(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device, int bind_num)
{
	unsigned int reg_offset[2], pwm_index[2] = {0};
	struct device_node *sub_np[2];
	struct platform_device *pwm_pdevice[2];
	int i = 0;
	int err;
	struct sunxi_pwm_chip *chip;

	chip = to_sunxi_pwm_chip(pwm_chip);

	pwm_index[0] = pwm_device->hwpwm;
	pwm_index[1] = bind_num - pwm_chip->base;

	/* get current index pwm device */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		sub_np[i] = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", pwm_index[i]);
		if (IS_ERR_OR_NULL(sub_np[i])) {
			sunxi_err(chip->pwm_chip.dev, "can't parse \"sunxi-pwms\" property\n");
			return;
		}
		pwm_pdevice[i] = of_find_device_by_node(sub_np[i]);
		if (IS_ERR_OR_NULL(pwm_pdevice[i])) {
			sunxi_err(chip->pwm_chip.dev, "can't parse pwm device\n");
			return;
		}
	}

	/* disable pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = chip->data->per_offset;
		sunxi_pwm_set_reg(pwm_chip, reg_offset[i], pwm_index[i], 0x1, 0);
	}

	/* disable pwm clk gating */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = sunxi_pwm_regs[pwm_index[i] >> 0x1];
		sunxi_pwm_set_reg(pwm_chip, reg_offset[i], PWM_CLK_GATING_SHIFT, 0x1, 0);
	}

	/* disable pwm dead zone,one for the two pwm */
	err = get_pdzcr_reg_offset(chip, pwm_index[0], &reg_offset[0]);
	if (err) {
		sunxi_err(chip->pwm_chip.dev, "get pwm dead zone failed\n");
		return;
	}

	sunxi_pwm_set_reg(pwm_chip, reg_offset[0], PWM_DZ_EN_SHIFT, PWM_DZ_EN_WIDTH, 0);

	/* config pin sleep */
	sunxi_pwm_pin_set_state(&pwm_pdevice[0]->dev, PWM_PIN_STATE_SLEEP);

	sunxi_pwm_regulator_disable(chip, pwm_index[0]);

	sunxi_pwm_pin_set_state(&pwm_pdevice[1]->dev, PWM_PIN_STATE_SLEEP);

	sunxi_pwm_regulator_disable(chip, pwm_index[1]);
}

static void sunxi_pwm_disable(struct pwm_chip *pwm_chip, struct pwm_device *pwm_device)
{
	int bind_num;
	struct sunxi_pwm_chip *chip;

	chip = to_sunxi_pwm_chip(pwm_chip);

	bind_num = chip->config[pwm_device->hwpwm].bind_pwm;
	if (bind_num == SUNXI_PWM_BIND_DEFAULT)
		sunxi_pwm_disable_single(pwm_chip, pwm_device);
	else
		sunxi_pwm_disable_dual(pwm_chip, pwm_device, bind_num);
}

/*
 * default:24MHz
 * max input pwm period:2.7ms
 * min input pwm period:2.7us
 */
static int sunxi_pwm_capture(struct pwm_chip *pwm_chip, struct pwm_device *pwm,
			struct pwm_capture *result, unsigned long timeout)
{
	struct sunxi_pwm_chip *chip = to_sunxi_pwm_chip(pwm_chip);
	unsigned long long pwm_clk = 0, temp_clk;
	struct device_node *sub_np;
	unsigned int reg_offset, pwm_div, reg_val;
	struct platform_device *pwm_pdevice;
	int ret, index = pwm->hwpwm;

	chip->index = 0;
	sub_np = of_parse_phandle(pwm_chip->dev->of_node, "sunxi-pwms", index);
	if (IS_ERR_OR_NULL(sub_np)) {
		sunxi_err(chip->pwm_chip.dev, "can't parse \"pwms\" property\n");
		return -ENODEV;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (IS_ERR_OR_NULL(pwm_pdevice)) {
		sunxi_err(chip->pwm_chip.dev, "can't parse pwm device\n");
		return -ENODEV;
	}

	ret = sunxi_pwm_regulator_request(chip, &pwm_pdevice->dev, index);
	if (ret) {
		sunxi_err(chip->pwm_chip.dev, "request regulator failed!\n");
		return ret;
	}

	ret = sunxi_pwm_regulator_enable(chip, index);
	if (ret) {
		sunxi_err(chip->pwm_chip.dev, "enable regulator failed!\n");
		goto err0;
	}

	ret = sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_ACTIVE);
	if (ret) {
		sunxi_err(chip->pwm_chip.dev, "set pinctrl status failed!\n");
		goto err1;
	}

	/* enable clk for pwm controller */
	sunxi_pwm_set_reg(pwm_chip, PWM_PCGR, index, 1, 1);

	/* Enable capture */
	sunxi_pwm_set_reg(pwm_chip, PWM_CER, index,  0x1, 0x1);

	/* enabled rising edge trigger */
	sunxi_pwm_writel(pwm_chip, PWM_CCR_BASE + index * chip->data->pwm_reg_uniform_offset, PWM_CAPTURE_CRTE |
			PWM_CAPTURE_CRLF);

	/* enable rise&fail interrupt */
	sunxi_pwm_set_reg(pwm_chip, PWM_CIER, (index << 0x1), 0x2, 0x3);

	ret = wait_event_interruptible_timeout(chip->wait,
			(chip->index >= 0x2) ? 1:0, msecs_to_jiffies(timeout));
	if (ret == 0) {
		sunxi_err(chip->pwm_chip.dev, "capture pwm timeout!\n");
		ret = -EINVAL;
		goto err1;
	}

	reg_offset = sunxi_pwm_regs[index >> 0x1];
	reg_val = sunxi_pwm_readl(pwm_chip, reg_offset);
	pwm_div = sunxi_pwm_pre_scal[reg_val & (0x000f)][1];
	if (reg_val & (0x01 << PWM_CLK_SRC_SHIFT))
		pwm_clk = 100;  /* 100M */
	else
		pwm_clk = 24;  /* 24M */

	temp_clk = (chip->cap_time[1] + chip->cap_time[2]) * 1000 * pwm_div;
	do_div(temp_clk, pwm_clk);
	result->period = (unsigned int)temp_clk;
	temp_clk = chip->cap_time[1] * 1000 * pwm_div;
	do_div(temp_clk, pwm_clk);
	result->duty_cycle = (unsigned int)temp_clk;

	/* disable rise&fail interrupt */
	sunxi_pwm_set_reg(pwm_chip, PWM_CIER, (index << 0x1), 0x2, 0x0);
	sunxi_pwm_set_reg(pwm_chip, PWM_PCGR, index, 1, 0);

	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_SLEEP);

	sunxi_pwm_regulator_disable(chip, index);

	return 0;

err1:
	sunxi_pwm_regulator_disable(chip, index);
err0:
	sunxi_pwm_regulator_release(chip);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int sunxi_pwm_apply(struct pwm_chip *pwm_chip,
				struct pwm_device *pwm_device,
				const struct pwm_state *state)
{
	bool enabled;
	int err;
	struct sunxi_pwm_chip *chip;

	enabled = pwm_device->state.enabled;
	if (enabled && !state->enabled)
		return 0;

	chip = to_sunxi_pwm_chip(pwm_chip);

	if (chip->channel_polarity_flag[pwm_device->pwm]) {
		chip->channel_polarity_flag[pwm_device->pwm] = false;
		err = sunxi_pwm_set_polarity(pwm_chip, pwm_device, PWM_POLARITY_NORMAL);
		if (err) {
			return err;
		}
	} else if (chip->resume_polarity_flag[pwm_device->pwm]) {
		chip->resume_polarity_flag[pwm_device->pwm] = false;
		err = sunxi_pwm_set_polarity(pwm_chip, pwm_device, pwm_device->state.polarity);
		if (err) {
			return err;
		}
	} else if (state->polarity != pwm_device->state.polarity) {
		err = sunxi_pwm_set_polarity(pwm_chip, pwm_device, state->polarity);
		if (err) {
			return err;
		}
	}

	err = sunxi_pwm_config_channel(pwm_chip, pwm_device, state->duty_cycle, state->period);
	if (err)
		return err;

	if (!enabled && state->enabled) {
		err = sunxi_pwm_enable(pwm_chip, pwm_device);
		if (err)
			return err;
	}

	return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 25)
static int sunxi_pwm_get_state(struct pwm_chip *pwm_chip,
				struct pwm_device *pwm_device,
				struct pwm_state *state)
#else
static void sunxi_pwm_get_state(struct pwm_chip *pwm_chip,
				struct pwm_device *pwm_device,
				struct pwm_state *state)
#endif
{
	unsigned int reg_offset, prescale;
	u32 val, sel;
	unsigned long period, duty;
	unsigned long long temp, clk_source;
	unsigned int div_m;
	struct sunxi_pwm_chip *chip;

	chip = to_sunxi_pwm_chip(pwm_chip);

	sel = pwm_device->hwpwm;
	reg_offset = chip->data->pcr_base_offset + sel * chip->data->pwm_reg_uniform_offset;

	val = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (val & BIT_MASK(PWM_ACT_STA))
		state->polarity = PWM_POLARITY_NORMAL;
	else
		state->polarity = PWM_POLARITY_INVERSED;

	val = sunxi_pwm_readl(pwm_chip, chip->data->per_offset);
	if (val & BIT_MASK(sel))
		state->enabled = true;
	else
		state->enabled = false;

	val = sunxi_pwm_readl(pwm_chip, reg_offset);
	prescale = (val & PWM_PRESCAL_K) + 1;

	reg_offset = chip->data->ppr_base_offset + chip->data->pwm_reg_uniform_offset * sel;
	val = sunxi_pwm_readl(pwm_chip, reg_offset);
	duty = val & PWM_DUTY;
	period = val & PWM_PERIOD;

	reg_offset = sunxi_pwm_regs[sel >> 0x1];
	val = sunxi_pwm_readl(pwm_chip, reg_offset);
	if (val & BIT_MASK(PWM_CLK_SRC)) {
		clk_source = SUNXI_CLK_100M;
	} else {
		clk_source = chip->data->has_hosc_clock ? \
		clk_get_rate(chip->hosc) \
		: SUNXI_CLK_24M;
	}

	div_m = (0x1 << (val & PWM_CLK_DIV_M));
	do_div(clk_source, div_m);

	temp = period * prescale * SUNXI_DIV_CLK;
	do_div(temp, clk_source);
	state->period = temp;

	temp = duty * prescale * SUNXI_DIV_CLK;
	do_div(temp, clk_source);
	state->duty_cycle = temp;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 25)
	return 0;
#endif
}

static int sunxi_pwm_resource_get(struct platform_device *pdev,
				struct sunxi_pwm_chip *chip,
				struct device_node *np)
{
	struct resource *res;
	int err, i;
	const char *st = NULL;
	struct platform_device *pwm_pdevice;
	struct device_node *sub_np;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		sunxi_err(&pdev->dev, "fail to get pwm IORESOURCE_MEM\n");
		return -EINVAL;
	}

	chip->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(chip->base)) {
		sunxi_err(&pdev->dev, "fail to map pwm IO resource\n");
		return PTR_ERR(chip->base);
	}

	/*
	 * If there are clock resources, has_clock is true, apply for clock resources;
	 * otherwise, has_clock is false, you do not need to apply for clock resources.
	 */
	if (chip->data->has_clock) {
		chip->reset = devm_reset_control_get(&pdev->dev, NULL);
		if (IS_ERR(chip->reset)) {
			sunxi_err(&pdev->dev, "can't get pwm reset clk\n");
			return PTR_ERR(chip->reset);
		}

		chip->clk = devm_clk_get(&pdev->dev, NULL);
		if (!chip->clk) {
			sunxi_err(&pdev->dev, "fail to get pwm clk!\n");
			return -EINVAL;
		}
		if (chip->data->has_bus_clock) {
			chip->bclk = devm_clk_get(&pdev->dev, "clk_bus_pwm");
			if (!chip->bclk) {
				sunxi_err(&pdev->dev, "fail to get pwm clk!\n");
				return -EINVAL;
			}
		}
		if (chip->data->has_hosc_clock) {
			chip->hosc = devm_clk_get(&pdev->dev, "clk_hosc");
			if (!chip->hosc) {
				sunxi_err(&pdev->dev, "fail to get clk_hosc!\n");
				return -EINVAL;
			}
		}
	}

	/* read property pwm-number */
	err = of_property_read_u32(np, "pwm-number", &chip->pwm_chip.npwm);
	if (err) {
		sunxi_err(&pdev->dev, "failed to get pwm number!\n");
		return -EINVAL;
	}

	/* read property pwm-base */
	err = of_property_read_u32(np, "pwm-base", &chip->pwm_chip.base);
	if (err) {
		sunxi_err(&pdev->dev, "failed to get pwm-base!\n");
		return -EINVAL;
	}

	sunxi_debug(&pdev->dev, "base is %d, num is %d\n", chip->pwm_chip.base, chip->pwm_chip.npwm);

#if IS_ENABLED(CONFIG_AW_EPHY)
	/* read property ephy-channel */
	err = of_property_read_u32(np, "ephy-channel", &chip->ephy_channel);
	if (err < 0) {
		sunxi_debug(&pdev->dev, "failed to get ephy-channel: %d, force to -1 !\n", err);
		chip->ephy_channel = -1;
	}
#endif /* CONFIG_AW_EPHY */

	err = of_property_read_u32(np, "#pwm-cells", &chip->cells_num);
	if (err) {
		sunxi_err(&pdev->dev, "failed to get pwm-cells!\n");
		return -EINVAL;
	}

	if (chip->data->config_status) {
		err = of_property_read_string(np, "status", &st);
		if (err) {
			sunxi_err(&pdev->dev, "failed to get status!\n");
			return -EINVAL;
		}
		if (st && (!strcmp(st, "okay") || !strcmp(st, "ok")))
			chip->status = true;
	}

	chip->config = devm_kzalloc(&pdev->dev, sizeof(*chip->config) * chip->pwm_chip.npwm, GFP_KERNEL);
	if (!chip->config)
		return -ENOMEM;

	for (i = 0; i < chip->pwm_chip.npwm; i++) {
		sub_np = of_parse_phandle(np, "sunxi-pwms", i);
		if (!sub_np) {
			sunxi_err(&pdev->dev, "can't parse \"sunxi-pwms\" property\n");
			return -EINVAL;
		}

		pwm_pdevice = of_find_device_by_node(sub_np);
		/* it may be the program is error or the status of pwm%d  is disabled */
		if (!pwm_pdevice) {
			sunxi_debug(&pdev->dev, "fail to find device for pwm%d, continue!\n", i);
			continue;
		}

		err = sunxi_pwm_get_config(pwm_pdevice, &chip->config[i]);
		if (err) {
			sunxi_err(&pdev->dev, "Get config failed,exit!\n");
			return err;
		}
	}

	return 0;
}

static void sunxi_pwm_resource_put(struct sunxi_pwm_chip *chip)
{
}

static int sunxi_pwm_clk_enable(struct sunxi_pwm_chip *chip)
{
	int err;

	if (chip->data->has_clock) {
		/*
		 * In order to ensure the consistent display from uboot to the kernel stage,
		 * there is no need to reset the clock in the kernel stage.
		 */
		err = reset_control_deassert(chip->reset);
		if (err) {
			sunxi_err(chip->pwm_chip.dev, "deassert pwm reset failed\n");
			return err;
		}

		err = clk_prepare_enable(chip->clk);
		if (err) {
			sunxi_err(chip->pwm_chip.dev, "try to enbale pwm clk failed\n");
			reset_control_assert(chip->reset);
			return err;
		}
		if (chip->data->has_bus_clock) {
			err = clk_prepare_enable(chip->bclk);
			if (err) {
				sunxi_err(chip->pwm_chip.dev, "try to enbale pwm bclk failed\n");
				clk_disable_unprepare(chip->clk);
				reset_control_assert(chip->reset);
				return err;
			}
		}
	}

	return 0;
}

static void sunxi_pwm_clk_disable(struct sunxi_pwm_chip *chip)
{
	if (chip->data->has_clock) {
		if (chip->data->has_bus_clock)
			clk_disable_unprepare(chip->bclk);
		clk_disable_unprepare(chip->clk);
		reset_control_assert(chip->reset);
	}
}

static irqreturn_t sunxi_pwm_handler(int irq, void *dev_id)
{
	struct sunxi_pwm_chip *pwm = (struct sunxi_pwm_chip *)dev_id;
	struct pwm_chip *chip = &(pwm->pwm_chip);
	unsigned int device_num;
	unsigned int reg_val = 0;

	reg_val = sunxi_pwm_readl(chip, PWM_CISR);

	/*
	 * 0 , 1 --> 0/2
	 * 2 , 3 --> 2/2
	 * 4 , 5 --> 4/2
	 * 6 , 7 --> 6/2
	 */
	device_num = ((ffs(reg_val) - 1) & (~(0x01)))/2;
	/*
	 * Capture input:
	 *          _______               _______
	 *         |       |             |       |
	 * ________|       |_____________|       |________
	 * index   ^0      ^1            ^2
	 *
	 * Capture start by the first available rising edge.
	 *
	 */
	switch (pwm->index) {
	case 0:
		pwm->cap_time[pwm->index] = sunxi_pwm_readl(chip,
				PWM_CRLR_BASE + device_num * pwm->data->pwm_reg_uniform_offset);
		/* clean capture CRLF and enabled fail interrupt */
		sunxi_pwm_writel(chip,
				PWM_CCR_BASE + device_num * pwm->data->pwm_reg_uniform_offset, PWM_CAPTURE_CLEAR_ALL);
		break;
	case 1:
		pwm->cap_time[pwm->index] = sunxi_pwm_readl(chip,
				PWM_CFLR_BASE + device_num * pwm->data->pwm_reg_uniform_offset);
		/* clean capture CFLF and disabled fail interrupt */
		sunxi_pwm_writel(chip,
				PWM_CCR_BASE + device_num * pwm->data->pwm_reg_uniform_offset, PWM_CAPTURE_EXCLUDE_CFTE_EN_CLEAR);
		break;
	case 2:
		pwm->cap_time[pwm->index] = sunxi_pwm_readl(chip,
				PWM_CRLR_BASE + device_num * pwm->data->pwm_reg_uniform_offset);
		/* clean capture CRLF and disabled rise interrupt */
		sunxi_pwm_writel(chip,
				PWM_CCR_BASE + device_num * pwm->data->pwm_reg_uniform_offset, PWM_CAPTURE_EXCLUDE_CRTE_EN_CLEAR);
		wake_up(&pwm->wait);
		break;
	default:
		break;
	}
	pwm->index++;

	/* Clean capture rise status */
	sunxi_pwm_writel(chip, PWM_CISR, reg_val);

	return IRQ_HANDLED;
}

static int sunxi_pwm_hw_init(struct platform_device *pdev,
			     struct sunxi_pwm_chip *chip)
{
	int err;

	err = sunxi_pwm_clk_enable(chip);
	if (err) {
		sunxi_err(&pdev->dev, "enable pwm clock failed\n");
		return err;
	}

	chip->irq = platform_get_irq(pdev, 0);
	if (chip->irq < 0)
		sunxi_info(&pdev->dev, "get interrupt resource failed and capture mode invalid\n");
	else {
		init_waitqueue_head(&chip->wait);
		err = devm_request_irq(&pdev->dev, chip->irq, sunxi_pwm_handler, IRQF_TRIGGER_NONE, "pwm", chip);
		if (err) {
			sunxi_err(&pdev->dev, "failed to request PWM IRQ\n");
			sunxi_pwm_clk_disable(chip);
			return err;
		}
	}

	return 0;
}

static void sunxi_pwm_hw_exit(struct sunxi_pwm_chip *chip)
{
	sunxi_pwm_clk_disable(chip);
}

static struct pwm_ops sunxi_pwm_ops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	.apply = sunxi_pwm_apply,
	.free = sunxi_pwm_disable,
#else
	.config = sunxi_pwm_config_channel,
	.enable = sunxi_pwm_enable,
	.disable = sunxi_pwm_disable,
	.set_polarity = sunxi_pwm_set_polarity,
#endif

	.capture = sunxi_pwm_capture,
	.get_state = sunxi_pwm_get_state,
	.owner = THIS_MODULE,
};

static const struct of_device_id sunxi_pwm_match[] = {
	{ .compatible = "allwinner,sunxi-pwm", .data = &sunxi_pwm_v200_data},
	{ .compatible = "allwinner,sunxi-s_pwm", .data = &sunxi_pwm_v200_data},
	{ .compatible = "allwinner,sunxi-pwm-v201", .data = &sunxi_pwm_v201_data},
	{ .compatible = "allwinner,sunxi-pwm-v202", .data = &sunxi_pwm_v202_data},
	{ .compatible = "allwinner,sunxi-pwm-v203", .data = &sunxi_pwm_v203_data},
	{ .compatible = "allwinner,sunxi-pwm-v204", .data = &sunxi_pwm_v204_data},
	{ .compatible = "allwinner,sunxi-pwm-v205", .data = &sunxi_pwm_v205_data},
	{ .compatible = "allwinner,sunxi-pwm-v100", .data = &sunxi_pwm_v100_data},
	{ .compatible = "allwinner,sunxi-pwm-v101", .data = &sunxi_pwm_v101_data},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sunxi_pwm_match);

static int sunxi_pwm_fill_hw_data(struct sunxi_pwm_chip *chip)
{
	size_t size;
	const struct of_device_id *of_id;

	/* get hw data from match table */
	of_id = of_match_device(sunxi_pwm_match, chip->pwm_chip.dev);
	if (!of_id) {
		sunxi_err(chip->pwm_chip.dev, "of_match_device() failed\n");
		return -EINVAL;
	}

	chip->data = (struct sunxi_pwm_hw_data *)(of_id->data);

	size = sizeof(u32) * chip->data->pm_regs_num;
	chip->pm_regs_offset = devm_kzalloc(chip->pwm_chip.dev, size, GFP_KERNEL);
	if (!chip->pm_regs_offset)
		return -ENOMEM;

	chip->regs_backup = devm_kzalloc(chip->pwm_chip.dev, size, GFP_KERNEL);
	if (!chip->regs_backup)
		return -ENOMEM;
	/* Configure the registers that need to be saved for wake-up from sleep */
	chip->pm_regs_offset[0] = PWM_PIER;
	chip->pm_regs_offset[1] = PWM_CIER;
	chip->pm_regs_offset[2] = chip->data->per_offset;
	chip->pm_regs_offset[3] = chip->data->cer_offset;
	if (chip->data->clk_gating_separate)
		chip->pm_regs_offset[4] = PWM_PCGR;

	return 0;
}

static int sunxi_pwm_probe(struct platform_device *pdev)
{
	int ret, i;
	struct sunxi_pwm_chip *chip;
	struct device_node *np = pdev->dev.of_node;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	platform_set_drvdata(pdev, chip);
	chip->pwm_chip.dev = &pdev->dev;

	for (i = 0; i < PWM_NUM_MAX; i++)
		chip->channel_polarity_flag[i] = true;

	ret = sunxi_pwm_fill_hw_data(chip);
	if (ret) {
		sunxi_err(&pdev->dev, "unable to get hw_data\n");
		return ret;
	}

	ret = sunxi_pwm_resource_get(pdev, chip, np);
	if (ret) {
		sunxi_err(&pdev->dev, "pwm failed to get resource\n");
		goto err0;
	}

	if (chip->data->config_status && !chip->status) {
		sunxi_debug(&pdev->dev, "the current status of pwmchip is disabled, it should not be loaded\n");
		goto err1;
	}

	ret = sunxi_pwm_hw_init(pdev, chip);
	if (ret) {
		sunxi_err(&pdev->dev, "pwm failed to hw_init");
		goto err1;
	}

	chip->pwm_chip.dev = &pdev->dev;
	chip->pwm_chip.ops = &sunxi_pwm_ops;
	chip->pwm_chip.of_xlate = of_pwm_xlate_with_flags;
	chip->pwm_chip.of_pwm_n_cells = chip->cells_num;
	spin_lock_init(&chip->lock);
	/*
	 * register pwm chip to pwm-core should be the ending of probe
	 * before registering, all pwm controller resources need to be ready
	 * (pwm_request can happen anytime after registration)
	 */
	ret = pwmchip_add(&chip->pwm_chip);
	if (ret < 0) {
		sunxi_err(&pdev->dev, "register pwmchip failed: %d\n", ret);
		goto err2;
	}

	sunxi_info(&pdev->dev, "pwmchip probe success\n");

	return 0;

err2:
	sunxi_pwm_hw_exit(chip);
err1:
	sunxi_pwm_resource_put(chip);
err0:
	return ret;
}

static int sunxi_pwm_remove(struct platform_device *pdev)
{
	struct sunxi_pwm_chip *chip;

	chip = platform_get_drvdata(pdev);

	pwmchip_remove(&chip->pwm_chip);

	sunxi_pwm_hw_exit(chip);

	sunxi_pwm_resource_put(chip);
	sunxi_pwm_regulator_release(chip);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)

static void sunxi_pwm_stop_work(struct sunxi_pwm_chip *chip)
{
	int i;
	bool pwm_state;

	for (i = 0; i < chip->pwm_chip.npwm; i++) {
		pwm_state = chip->pwm_chip.pwms[i].state.enabled;
		pwm_disable(&chip->pwm_chip.pwms[i]);
		chip->pwm_chip.pwms[i].state.enabled = pwm_state;
	}
}

static void sunxi_pwm_start_work(struct sunxi_pwm_chip *chip)
{
	int i;
	struct pwm_state state;
	for (i = 0; i < PWM_NUM_MAX; i++)
		chip->resume_polarity_flag[i] = true;

	for (i = 0; i < chip->pwm_chip.npwm; i++) {
		pwm_get_state(&chip->pwm_chip.pwms[i], &state);
		pwm_apply_state(&chip->pwm_chip.pwms[i], &state);

		if (pwm_is_enabled(&chip->pwm_chip.pwms[i])) {
			chip->pwm_chip.pwms[i].state.enabled = false;
			pwm_enable(&chip->pwm_chip.pwms[i]);
		}
	}
}

static int sunxi_pwm_suspend(struct device *dev)
{
	struct sunxi_pwm_chip *chip = dev_get_drvdata(dev);

	sunxi_pwm_stop_work(chip);

	sunxi_pwm_save_regs(chip);

	sunxi_pwm_clk_disable(chip);

	return 0;
}

static int sunxi_pwm_resume(struct device *dev)
{
	struct sunxi_pwm_chip *chip = dev_get_drvdata(dev);

	sunxi_pwm_clk_enable(chip);

	sunxi_pwm_restore_regs(chip);

	sunxi_pwm_start_work(chip);

	return 0;
}

static const struct dev_pm_ops pwm_pm_ops = {
	.suspend_late = sunxi_pwm_suspend,
	.resume_early = sunxi_pwm_resume,
};
#else
static const struct dev_pm_ops pwm_pm_ops;
#endif

static struct platform_driver sunxi_pwm_driver = {
	.probe = sunxi_pwm_probe,
	.remove = sunxi_pwm_remove,
	.driver = {
		.name = "sunxi_pwm",
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_pwm_match,
		.pm = &pwm_pm_ops,
	 },
};

static int __init pwm_module_init(void)
{
	return platform_driver_register(&sunxi_pwm_driver);
}

static void __exit pwm_module_exit(void)
{
	platform_driver_unregister(&sunxi_pwm_driver);
}

subsys_initcall_sync(pwm_module_init);
module_exit(pwm_module_exit);

MODULE_AUTHOR("lihuaxing");
MODULE_DESCRIPTION("pwm driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pwm");
MODULE_VERSION("1.3.7");

/*
 * drivers/pwm/pwm-sunxi.c
 *
 * Allwinnertech pulse-width-modulation controller driver
 *
 * Copyright (C) 2015 AllWinner
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
//#define DEBUG
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
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
#include "pwm-sunxi-group.h"

#define PWM_DEBUG 0
#define PWM_NUM_MAX 4
#define PWM_BIND_NUM 2
#define PWM_PIN_STATE_ACTIVE "active"
#define PWM_PIN_STATE_SLEEP "sleep"

#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	    (((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	    (((reg) & CLRMASK(width, shift)) | (val << (shift)))

#if PWM_DEBUG
#define pwm_debug(fmt, arg...)	\
	pr_info("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
#define pwm_debug(msg...)
#endif

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
	u32 pdzcr01_offset;	/* PWM dead zone control register 01*/
	u32 pdzcr23_offset;	/* PWM dead zone control register 23*/
	u32 pdzcr45_offset;	/* PWM dead zone control register 45*/
	u32 per_offset;		/* PWM enable register */
	u32 cer_offset;		/* PWM capture enable register */
	u32 ver_reg_offset;	/* PWM version register */
	u32 pcr_base_offset;	/* PWM control register */
	u32 ppr_base_offset;	/* PWM period register */
	u32 pcntr_base_offset;	/* PWM counter register */
	u32 ccr_base_offset;	/* PWM capture control register */
	u32 crlr_base_offset;	/* PWM capture rise lock register */
	u32 cflr_base_offset;	/* PWM capture fall lock register */
	int pm_regs_num;	/* PWM pm related register length */
	bool clk_gating_separate;	/* PWM clk gating register whether to separate */
};

struct sunxi_pwm_chip {
	struct pinctrl *pctl;
	struct pwm_chip chip;
	struct sunxi_pwm_hw_data *data;
	struct sunxi_pwm_config *config;
	struct clk	*pwm_clk;
	struct reset_control	*pwm_rst_clk;
	void __iomem *base;
	unsigned int g_channel;
	unsigned int g_polarity;
	unsigned int start_count;
	unsigned int g_period;
	u32 *regs_backup;
	u32 *pm_regs_offset;
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

static inline struct sunxi_pwm_chip *to_sunxi_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct sunxi_pwm_chip, chip);
}

static inline u32 sunxi_pwm_readl(struct pwm_chip *chip, u32 offset)
{
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);
	u32 value = 0;

	value = readl(pc->base + offset);

	return value;
}

static inline u32 sunxi_pwm_writel(struct pwm_chip *chip, u32 offset, u32 value)
{
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	writel(value, pc->base + offset);

	return 0;
}

static int sunxi_pwm_pin_set_state(struct device *dev, char *name)
{
	struct pinctrl *pctl;
	struct pinctrl_state *state = NULL;
	int ret;

	pctl = devm_pinctrl_get(dev);
	if (IS_ERR(pctl)) {
		dev_err(dev, "pinctrl_get failed\n");
		ret = PTR_ERR(pctl);
		return ret;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		dev_err(dev, "pinctrl_lookup_state(%s) failed\n", name);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret) {
		dev_err(dev, "pinctrl_select_state(%s) failed\n", name);
		goto exit;
	}

exit:
	devm_pinctrl_put(pctl);
	return ret;

}

static int sunxi_pwm_get_config(struct platform_device *pdev,
				struct sunxi_pwm_config *config)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	ret = of_property_read_u32(np, "bind_pwm", &config->bind_pwm);
	if (ret < 0) {
		/*if there is no bind pwm,set 255, dual pwm invalid!*/
		config->bind_pwm = 255;
		ret = 0;
	}

	ret = of_property_read_u32(np, "dead_time", &config->dead_time);
	if (ret < 0) {
		/*if there is  bind pwm, but not set dead time,set bind pwm 255,dual pwm invalid!*/
		config->bind_pwm = 255;
		ret = 0;
	}

	of_node_put(np);

	return ret;
}

static int sunxi_pwm_set_polarity_single(struct pwm_chip *chip,
					struct pwm_device *pwm,
					enum pwm_polarity polarity)
{
	u32 temp;
	unsigned int reg_offset, reg_shift, reg_width;
	u32 sel = 0;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	sel = pwm->pwm - chip->base;
	reg_offset = pc->data->pcr_base_offset + sel * PWM_REG_UNIFORM_OFFSET;
	reg_shift = PWM_ACT_STA_SHIFT;
	reg_width = PWM_ACT_STA_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset);
	if (polarity == PWM_POLARITY_NORMAL) /* set single polarity*/
		temp = SET_BITS(reg_shift, reg_width, temp, 1);
	else
		temp = SET_BITS(reg_shift, reg_width, temp, 0);

	sunxi_pwm_writel(chip, reg_offset, temp);

	return 0;
}

static int sunxi_pwm_set_polarity_dual(struct pwm_chip *chip,
					struct pwm_device *pwm,
					enum pwm_polarity polarity,
					int bind_num)
{
	u32 temp[2];
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	u32 sel[2] = {0};
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	sel[0] = pwm->pwm - chip->base;
	sel[1] = bind_num - chip->base;
	/* config current pwm*/
	reg_offset[0] = pc->data->pcr_base_offset + sel[0] * PWM_REG_UNIFORM_OFFSET;
	reg_shift[0] = PWM_ACT_STA_SHIFT;
	reg_width[0] = PWM_ACT_STA_WIDTH;
	temp[0] = sunxi_pwm_readl(chip, reg_offset[0]);
	if (polarity == PWM_POLARITY_NORMAL)
		temp[0] = SET_BITS(reg_shift[0], 1, temp[0], 1);
	else
		temp[0] = SET_BITS(reg_shift[0], 1, temp[0], 0);
	/* config bind pwm*/
	reg_offset[1] = pc->data->pcr_base_offset + sel[1] * PWM_REG_UNIFORM_OFFSET;
	reg_shift[1] = PWM_ACT_STA_SHIFT;
	reg_width[1] = PWM_ACT_STA_WIDTH;
	temp[1] = sunxi_pwm_readl(chip, reg_offset[1]);

	/*bind pwm's polarity is reverse compare with the  current pwm*/
	if (polarity == PWM_POLARITY_NORMAL)
		temp[1] = SET_BITS(reg_shift[0], 1, temp[1], 1);
	else
		temp[1] = SET_BITS(reg_shift[0], 1, temp[1], 0);
	/*config register at the same time*/
	sunxi_pwm_writel(chip, reg_offset[0], temp[0]);
	sunxi_pwm_writel(chip, reg_offset[1], temp[1]);

	return 0;
}

static int sunxi_pwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm,
				enum pwm_polarity polarity)
{
	int bind_num;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	bind_num = pc->config[pwm->pwm - chip->base].bind_pwm;
	if (bind_num == 255)
		sunxi_pwm_set_polarity_single(chip, pwm, polarity);
	else
		sunxi_pwm_set_polarity_dual(chip, pwm, polarity, bind_num);

	return 0;
}


static u32 get_pccr_reg_offset(u32 sel, u32 *reg_offset)
{
	switch (sel) {
	case 0:
	case 1:
		*reg_offset = PWM_PCCR01;
		break;
	case 2:
	case 3:
		*reg_offset = PWM_PCCR23;
		break;
	case 4:
	case 5:
		*reg_offset = PWM_PCCR45;
		break;
	case 6:
	case 7:
		*reg_offset = PWM_PCCR67;
		break;
	case 8:
	case 9:
		*reg_offset = PWM_PCCR89;
		break;
	case 10:
	case 11:
		*reg_offset = PWM_PCCRAB;
		break;
	case 12:
	case 13:
		*reg_offset = PWM_PCCRCD;
		break;
	case 14:
	case 15:
		*reg_offset = PWM_PCCREF;
		break;
	default:
		pr_err("%s:Not supported!\n", __func__);
		break;
	}
	return 0;
}

static u32 get_pdzcr_reg_offset(struct sunxi_pwm_chip *chip, u32 sel, u32 *reg_offset)
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
		pr_err("%s:Not supported!\n", __func__);
		break;
	}
	return 0;
}

#define PRESCALE_MAX 256

static int sunxi_pwm_config_single(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns)
{
	unsigned int temp;
	unsigned long long c = 0;
	unsigned long entire_cycles = 256, active_cycles = 192;
	unsigned int reg_offset, reg_shift, reg_width;
	unsigned int reg_bypass_shift/*, group_reg_offset*/;
	unsigned int reg_clk_src_shift, reg_clk_src_width;
	unsigned int reg_div_m_shift, reg_div_m_width, value;
	unsigned int pre_scal_id = 0, div_m = 0, prescale = 0;
	unsigned int reg_clk_gating_shift, reg_clk_gating_width;
	u32 sel = 0;
	u32 pre_scal[][2] = {

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

	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);
	unsigned int pwm_run_count = 0;

	if (pwm->chip_data) {
		pwm_run_count = ((struct group_pwm_config *)pwm->chip_data)->group_run_count;
		pc->g_channel = ((struct group_pwm_config *)pwm->chip_data)->group_channel;
		pc->g_polarity = ((struct group_pwm_config *)pwm->chip_data)->pwm_polarity;
		pc->g_period = ((struct group_pwm_config *)
				pwm->chip_data)->pwm_period;
	}

	if (pc->g_channel) {
		reg_offset = pc->data->per_offset;
		value = sunxi_pwm_readl(chip, reg_offset);
		value &= ~((0xf) << 4*(pc->g_channel - 1));
		sunxi_pwm_writel(chip, reg_offset, value);
	}

	sel = pwm->pwm - chip->base;
	get_pccr_reg_offset(sel, &reg_offset);

	/*src clk reg*/
	reg_clk_src_shift = PWM_CLK_SRC_SHIFT;
	reg_clk_src_width = PWM_CLK_SRC_WIDTH;

	if (pc->g_channel) {
		/* group_mode used the apb1 clk*/
		temp = sunxi_pwm_readl(chip, reg_offset);
		temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 0);
		sunxi_pwm_writel(chip, reg_offset, temp);
	} else {
		if (period_ns > 0 && period_ns <= 10) {
			/* if freq lt 100M, then direct output 100M clock,set by pass. */
			c = 100000000;
			/* config  bypass */
			if (!pc->data->clk_gating_separate) {
				if ((sel % 2) == 0)
					reg_bypass_shift = PWM_BYPASS_SHIFT;
				else
					reg_bypass_shift = PWM_BYPASS_SHIFT + 1;
				temp = sunxi_pwm_readl(chip, reg_offset);
				temp = SET_BITS(reg_bypass_shift, PWM_BYPASS_WIDTH, temp, 1); /*sun50iw9 bypass set */
				sunxi_pwm_writel(chip, reg_offset, temp);
			} else {
				reg_bypass_shift = sel;
				temp = sunxi_pwm_readl(chip, PCGR);
				temp = SET_BITS(reg_bypass_shift, PWM_BYPASS_WIDTH, temp, 1); /* bypass set */
				sunxi_pwm_writel(chip, PCGR, temp);
			}

			/*clk_src_reg*/
			temp = sunxi_pwm_readl(chip, reg_offset);
			temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1);/*clock source*/
			sunxi_pwm_writel(chip, reg_offset, temp);

			return 0;
		} else if (period_ns > 10 && period_ns <= 334) {
			/* if freq between 3M~100M, then select 100M as clock */
			c = 100000000;
			/*clk_src_reg*/
			temp = sunxi_pwm_readl(chip, reg_offset);
			temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 1);
			sunxi_pwm_writel(chip, reg_offset, temp);

		} else if (period_ns > 334) {
			/* if freq < 3M, then select 24M clock */
			c = 24000000;
			/*clk_src_reg*/
			temp = sunxi_pwm_readl(chip, reg_offset);
			temp = SET_BITS(reg_clk_src_shift, reg_clk_src_width, temp, 0);
			sunxi_pwm_writel(chip, reg_offset, temp);
		}
		pwm_debug("duty_ns=%d period_ns=%d c =%llu.\n", duty_ns, period_ns, c);

		c = c * period_ns;
		do_div(c, 1000000000);
		entire_cycles = (unsigned long)c;

		for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
			if (entire_cycles <= 65536)
				break;
			for (prescale = 0; prescale < PRESCALE_MAX+1; prescale++) {
				entire_cycles = ((unsigned long)c/pre_scal[pre_scal_id][1])/(prescale + 1);
				if (entire_cycles <= 65536) {
					div_m = pre_scal[pre_scal_id][0];
					break;
				}
			}
		}
		c = (unsigned long long)entire_cycles * duty_ns;
		do_div(c, period_ns);
		active_cycles = c;
		if (entire_cycles == 0)
			entire_cycles++;
	}

	/* config  clk div_m*/
	reg_div_m_shift = PWM_DIV_M_SHIFT;
	reg_div_m_width = PWM_DIV_M_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset);
	if (pc->g_channel)
		temp = SET_BITS(reg_div_m_shift, reg_div_m_width, temp, 0);
	else
		temp = SET_BITS(reg_div_m_shift, reg_div_m_width, temp, div_m);
	sunxi_pwm_writel(chip, reg_offset, temp);

	/* config clk gating */
	if (!pc->data->clk_gating_separate) {
		get_pccr_reg_offset(sel, &reg_offset);
		reg_clk_gating_shift = PWM_CLK_GATING_SHIFT;
		reg_clk_gating_width = PWM_CLK_GATING_WIDTH;
		temp = sunxi_pwm_readl(chip, reg_offset);
		temp = SET_BITS(reg_clk_gating_shift, reg_clk_gating_width, temp, 1);
		sunxi_pwm_writel(chip, reg_offset, temp);
	} else {
		reg_shift = sel;
		value = sunxi_pwm_readl(chip, PCGR);
		value = SET_BITS(reg_shift, 1, value, 1);/* set gating */
		sunxi_pwm_writel(chip, PCGR, value);
	}

#if defined(CONFIG_ARCH_SUN50IW9) && defined(CONFIG_MFD_ACX00)
	/* use pwm5 as phy(ac300,gmac) clk */
	if (sel == 5) {
		reg_shift = sel + 1;
		value = sunxi_pwm_readl(chip, PWM_PCCR45);
		value = SET_BITS(reg_shift, 1, value, 1);
		sunxi_pwm_writel(chip, PWM_PCCR45, value);

		reg_shift = sel - 1;
		value = sunxi_pwm_readl(chip, PWM_PCCR45);
		value = SET_BITS(reg_shift, 1, value, 1);
		sunxi_pwm_writel(chip, PWM_PCCR45, value);
	}
#endif

	/* config prescal */
	reg_offset = pc->data->pcr_base_offset + PWM_REG_UNIFORM_OFFSET * sel;
	reg_shift = PWM_PRESCAL_SHIFT;
	reg_width = PWM_PRESCAL_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset);
	if (pc->g_channel)
		temp = SET_BITS(reg_shift, reg_width, temp, 0xef);
	else
		temp = SET_BITS(reg_shift, reg_width, temp, prescale);
	sunxi_pwm_writel(chip, reg_offset, temp);

	if (pc->g_channel) {
		/* group set */
		reg_offset = PGR0 + 0x04 * (pc->g_channel - 1);
		reg_shift = sel;
		reg_width = 1;
		temp = sunxi_pwm_readl(chip, reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, 1); /* set  group0_cs */
		sunxi_pwm_writel(chip, reg_offset, temp);

		/* pwm pulse mode */
		reg_offset = pc->data->pcr_base_offset + sel * PWM_REG_UNIFORM_OFFSET;
		reg_shift = PWM_MODE_ACTS_SHIFT;
		reg_width = PWM_MODE_ACTS_WIDTH;
		temp = sunxi_pwm_readl(chip, reg_offset);
		temp = SET_BITS(reg_shift, reg_width, temp, 0x3); /* pwm pulse mode and active */
		/* pwm output pulse num */
		reg_shift = PWM_PUL_NUM_SHIFT;
		reg_width = PWM_PUL_NUM_WIDTH;
		temp = SET_BITS(reg_shift, reg_width, temp, pwm_run_count);   /* pwm output pulse num */
		sunxi_pwm_writel(chip, reg_offset, temp);
	}

	/* config active cycles */
	reg_offset = pc->data->ppr_base_offset + PWM_REG_UNIFORM_OFFSET * sel;
	reg_shift = PWM_ACT_CYCLES_SHIFT;
	reg_width = PWM_ACT_CYCLES_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset);
	if (pc->g_channel)
		temp = SET_BITS(reg_shift, reg_width, temp,
				(unsigned int)(pc->g_period*3/8));
	else
		temp = SET_BITS(reg_shift, reg_width, temp, active_cycles);
	sunxi_pwm_writel(chip, reg_offset, temp);

	/* config period cycles */
	reg_offset = pc->data->ppr_base_offset + PWM_REG_UNIFORM_OFFSET * sel;
	reg_shift = PWM_PERIOD_CYCLES_SHIFT;
	reg_width = PWM_PERIOD_CYCLES_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset);
	if (pc->g_channel) {
		temp = SET_BITS(reg_shift, reg_width, temp, pc->g_period);
		pc->g_channel = 0;
	} else
		temp = SET_BITS(reg_shift, reg_width, temp, (entire_cycles - 1));
	sunxi_pwm_writel(chip, reg_offset, temp);

	pwm_debug("active_cycles=%lu entire_cycles=%lu prescale=%u div_m=%u\n",
			active_cycles, entire_cycles, prescale, div_m);
	return 0;
}

static int sunxi_pwm_config_dual(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns, int bind_num)
{
	u32 value[2] = {0};
	unsigned int temp;
	unsigned long long c = 0, clk = 0, clk_temp = 0;
	unsigned long entire_cycles = 256, active_cycles = 192;
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	unsigned int reg_bypass_shift;
	unsigned int reg_dz_en_offset[2], reg_dz_en_shift[2], reg_dz_en_width[2];
	unsigned int pre_scal_id = 0, div_m = 0, prescale = 0;
	int src_clk_sel = 0;
	int i = 0;
	unsigned int dead_time = 0, duty = 0;
	u32 pre_scal[][2] = {

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
	unsigned int pwm_index[2] = {0};
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	pwm_index[0] = pwm->pwm - chip->base;
	pwm_index[1] = bind_num - chip->base;

	/* if duty time < dead time,it is wrong. */
	dead_time = pc->config[pwm_index[0]].dead_time;
	duty = (unsigned int)duty_ns;
	/* judge if the pwm eanble dead zone */
	get_pdzcr_reg_offset(pc, pwm_index[0], &reg_dz_en_offset[0]);
	reg_dz_en_shift[0] = PWM_DZ_EN_SHIFT;
	reg_dz_en_width[0] = PWM_DZ_EN_WIDTH;

	value[0] = sunxi_pwm_readl(chip, reg_dz_en_offset[0]);
	value[0] = SET_BITS(reg_dz_en_shift[0], reg_dz_en_width[0], value[0], 1);
	sunxi_pwm_writel(chip, reg_dz_en_offset[0], value[0]);
	temp = sunxi_pwm_readl(chip, reg_dz_en_offset[0]);
	temp &=  (1u << reg_dz_en_shift[0]);
	if (duty < dead_time || temp == 0) {
		pr_err("[PWM]duty time or dead zone error.\n");
		return -EINVAL;
	}

	for (i = 0; i < PWM_BIND_NUM; i++) {
		if ((i % 2) == 0)
			reg_bypass_shift = 0x5;
		else
			reg_bypass_shift = 0x6;
		get_pccr_reg_offset(pwm_index[i], &reg_offset[i]);
		reg_shift[i] = reg_bypass_shift;
		reg_width[i] = PWM_BYPASS_WIDTH;
	}

	if (period_ns > 0 && period_ns <= 10) {
		/* if freq lt 100M, then direct output 100M clock,set by pass */
		clk = 100000000;
		src_clk_sel = 1;

		/* config the two pwm bypass */
		for (i = 0; i < PWM_BIND_NUM; i++) {
			temp = sunxi_pwm_readl(chip, reg_offset[i]);
			temp = SET_BITS(reg_shift[i], reg_width[i], temp, 1);
			sunxi_pwm_writel(chip, reg_offset[i], temp);

			reg_shift[i] = PWM_CLK_SRC_SHIFT;
			reg_width[i] = PWM_CLK_SRC_WIDTH;
			temp = sunxi_pwm_readl(chip, reg_offset[i]);
			temp = SET_BITS(reg_shift[i], reg_width[i], temp, 1);
			sunxi_pwm_writel(chip, reg_offset[i], temp);
		}

		return 0;
	} else if (period_ns > 10 && period_ns <= 334) {
		clk = 100000000;
		src_clk_sel = 1;
	} else if (period_ns > 334) {
		/* if freq < 3M, then select 24M clock */
		clk = 24000000;
		src_clk_sel = 0;
	}

	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_shift[i] = PWM_CLK_SRC_SHIFT;
		reg_width[i] = PWM_CLK_SRC_WIDTH;

		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, src_clk_sel);
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	c = clk;
	c *= period_ns;
	do_div(c, 1000000000);
	entire_cycles = (unsigned long)c;

	/* get div_m and prescale,which satisfy: deat_val <= 256, entire <= 65536 */
	for (pre_scal_id = 0; pre_scal_id < 9; pre_scal_id++) {
		for (prescale = 0; prescale < PRESCALE_MAX+1; prescale++) {
			entire_cycles = ((unsigned long)c/pre_scal[pre_scal_id][1])/(prescale + 1);
			clk_temp = clk;
			do_div(clk_temp, pre_scal[pre_scal_id][1] * (prescale + 1));
			clk_temp *= dead_time;
			do_div(clk_temp, 1000000000);
			if (entire_cycles <= 65536 && clk_temp <= 256) {
				div_m = pre_scal[pre_scal_id][0];
				break;
			}
		}
		if (entire_cycles <= 65536 && clk_temp <= 256)
				break;
		else {
			pr_err("%s:config dual err.entire_cycles=%lu, dead_zone_val=%llu",
					__func__, entire_cycles, clk_temp);
			return -EINVAL;
		}
	}

	c = (unsigned long long)entire_cycles * duty_ns;
	do_div(c,  period_ns);
	active_cycles = c;
	if (entire_cycles == 0)
		entire_cycles++;

	/* config  clk div_m*/
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_shift[i] = PWM_DIV_M_SHIFT;
		reg_width[i] = PWM_DIV_M_SHIFT;
		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, div_m);
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	/* config prescal */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->pcr_base_offset + PWM_REG_UNIFORM_OFFSET * pwm_index[i];
		reg_shift[i] = PWM_PRESCAL_SHIFT;
		reg_width[i] = PWM_PRESCAL_WIDTH;
		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, prescale);
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	/* config active cycles */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->ppr_base_offset + PWM_REG_UNIFORM_OFFSET * pwm_index[i];
		reg_shift[i] = PWM_ACT_CYCLES_SHIFT;
		reg_width[i] = PWM_ACT_CYCLES_WIDTH;
		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, active_cycles);
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	/* config period cycles */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->ppr_base_offset + PWM_REG_UNIFORM_OFFSET * pwm_index[i];
		reg_shift[i] = PWM_PERIOD_CYCLES_SHIFT;
		reg_width[i] = PWM_PERIOD_CYCLES_WIDTH;
		temp = sunxi_pwm_readl(chip, reg_offset[i]);
		temp = SET_BITS(reg_shift[i], reg_width[i], temp, (entire_cycles - 1));
		sunxi_pwm_writel(chip, reg_offset[i], temp);
	}

	pwm_debug("active_cycles=%lu entire_cycles=%lu prescale=%u div_m=%u\n",
			active_cycles, entire_cycles, prescale, div_m);

	/* config dead zone, one config for two pwm */
	reg_offset[0] = reg_dz_en_offset[0];
	reg_shift[0] = PWM_PDZINTV_SHIFT;
	reg_width[0] = PWM_PDZINTV_WIDTH;
	temp = sunxi_pwm_readl(chip, reg_offset[0]);
	temp = SET_BITS(reg_shift[0], reg_width[0], temp, (unsigned int)clk_temp);
	sunxi_pwm_writel(chip, reg_offset[0], temp);

	return 0;
}

static int sunxi_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns)
{
	int bind_num;

	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	bind_num = pc->config[pwm->pwm - chip->base].bind_pwm;
	if (bind_num == 255)
		sunxi_pwm_config_single(chip, pwm, duty_ns, period_ns);
	else
		sunxi_pwm_config_dual(chip, pwm, duty_ns, period_ns, bind_num);

	return 0;
}

static int sunxi_pwm_enable_single(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int value = 0, index = 0;
	unsigned int reg_offset, reg_shift, reg_width, group_reg_offset;
	unsigned int temp;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;
	static unsigned int enable_num;
	unsigned int pwm_start_count, i;
	int pwm_period = 0;
	int ret;

	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	index = pwm->pwm - chip->base;
	sub_np = of_parse_phandle(chip->dev->of_node, "sunxi-pwms", index);
	if (IS_ERR_OR_NULL(sub_np)) {
		pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
		return -ENODEV;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (IS_ERR_OR_NULL(pwm_pdevice)) {
		pr_err("%s: can't parse pwm device\n", __func__);
		return -ENODEV;
	}
	ret = sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_ACTIVE);
	if (ret != 0)
		return ret;

	if (pwm->chip_data) {
		pc->g_channel = ((struct group_pwm_config *)pwm->chip_data)->group_channel;
		pwm_period = ((struct group_pwm_config *)
				pwm->chip_data)->pwm_period;
	}

	if (pc->g_channel)
		enable_num++;

	/* enable pwm controller  pwm can be used */
	if (!pc->g_channel) {
		reg_offset = pc->data->per_offset;
		reg_shift = index;
		value = sunxi_pwm_readl(chip, reg_offset);
		value = SET_BITS(reg_shift, 1, value, 1);
		sunxi_pwm_writel(chip, reg_offset, value);

		/* config clk gating */
		if (!pc->data->clk_gating_separate) {
			get_pccr_reg_offset(index, &reg_offset);
			reg_shift = PWM_CLK_GATING_SHIFT;
			reg_width = PWM_CLK_GATING_WIDTH;
		} else {
			reg_offset = PCGR;
			reg_shift = index;
			reg_width = 0x1;
		}
		value = sunxi_pwm_readl(chip, reg_offset);
		value = SET_BITS(reg_shift, reg_width, value, 1);
		sunxi_pwm_writel(chip, reg_offset, value);
	}

	if (pc->g_channel && enable_num == 4) {
		if (pc->g_polarity)
			pwm_start_count = (unsigned int)pwm_period*6/8;
		else
			pwm_start_count = 0;

		for (i = 4*(pc->g_channel - 1); i < 4*pc->g_channel; i++) {
			/* start count set */
			reg_offset = pc->data->pcntr_base_offset + PWM_REG_UNIFORM_OFFSET * i;
			reg_shift = PWM_COUNTER_START_SHIFT;
			reg_width = PWM_COUNTER_START_WIDTH;

			temp = pwm_start_count << reg_shift;
			sunxi_pwm_writel(chip, reg_offset, temp);
			if (pc->g_polarity)
				pwm_start_count = pwm_start_count -
					((unsigned int)pwm_period*2/8);
			else
				pwm_start_count = pwm_start_count +
					((unsigned int)pwm_period*2/8);
		}

		reg_offset = pc->data->per_offset;
		reg_shift = index;
		value = sunxi_pwm_readl(chip, reg_offset);
		value |= ((0xf) << 4*(pc->g_channel - 1));
		sunxi_pwm_writel(chip, reg_offset, value);

		group_reg_offset = PGR0 + 0x04 * (pc->g_channel - 1);

		enable_num = 0;
		pwm_start_count = 0;
		/* group en and start */
		reg_shift = PWMG_EN_SHIFT;
		value = sunxi_pwm_readl(chip, group_reg_offset);
		value = SET_BITS(reg_shift, 1, value, 1);/* enable group0 enable */
		sunxi_pwm_writel(chip, group_reg_offset, value);

		reg_shift = PWMG_START_SHIFT;
		value = sunxi_pwm_readl(chip, group_reg_offset);
		value = SET_BITS(reg_shift, 1, value, 1);/* group0 start */
		sunxi_pwm_writel(chip, group_reg_offset, value);

		pc->g_channel = 0;
	}

	return 0;
}

static int sunxi_pwm_enable_dual(struct pwm_chip *chip, struct pwm_device *pwm, int bind_num)
{
	u32 value[2] = {0};
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	struct device_node *sub_np[2];
	struct platform_device *pwm_pdevice[2];
	int i = 0, ret = 0;
	unsigned int pwm_index[2] = {0};
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	pwm_index[0] = pwm->pwm - chip->base;
	pwm_index[1] = bind_num - chip->base;

	/*set current pwm pin state*/
	sub_np[0] = of_parse_phandle(chip->dev->of_node, "sunxi-pwms", pwm_index[0]);
	if (IS_ERR_OR_NULL(sub_np[0])) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			return -ENODEV;
	}
	pwm_pdevice[0] = of_find_device_by_node(sub_np[0]);
	if (IS_ERR_OR_NULL(pwm_pdevice[0])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return -ENODEV;
	}

	/*set bind pwm pin state*/
	sub_np[1] = of_parse_phandle(chip->dev->of_node, "sunxi-pwms", pwm_index[1]);
	if (IS_ERR_OR_NULL(sub_np[1])) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			return -ENODEV;
	}
	pwm_pdevice[1] = of_find_device_by_node(sub_np[1]);
	if (IS_ERR_OR_NULL(pwm_pdevice[1])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return -ENODEV;
	}

	ret = sunxi_pwm_pin_set_state(&pwm_pdevice[0]->dev, PWM_PIN_STATE_ACTIVE);
	if (ret != 0)
		return ret;
	ret = sunxi_pwm_pin_set_state(&pwm_pdevice[1]->dev, PWM_PIN_STATE_ACTIVE);
	if (ret != 0)
		return ret;

	/* enable clk for pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		get_pccr_reg_offset(pwm_index[i], &reg_offset[i]);
		reg_shift[i] = PWM_CLK_GATING_SHIFT;
		reg_width[i] = PWM_CLK_GATING_WIDTH;
		value[i] = sunxi_pwm_readl(chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 1);
		sunxi_pwm_writel(chip, reg_offset[i], value[i]);
	}

	/* enable pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->per_offset;
		reg_shift[i] = pwm_index[i];
		reg_width[i] = 0x1;
		value[i] = sunxi_pwm_readl(chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 1);
		sunxi_pwm_writel(chip, reg_offset[i], value[i]);
	}

	return 0;
}

static int sunxi_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	int bind_num;
	int ret = 0;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	bind_num = pc->config[pwm->pwm - chip->base].bind_pwm;
	if (bind_num == 255)
		ret = sunxi_pwm_enable_single(chip, pwm);
	else
		ret = sunxi_pwm_enable_dual(chip, pwm, bind_num);

	return ret;
}


static void sunxi_pwm_disable_single(struct pwm_chip *chip, struct pwm_device *pwm)
{
	u32 value = 0, index = 0;
	unsigned int reg_offset, reg_shift, reg_width, group_reg_offset;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;

	static int disable_num;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);
	index = pwm->pwm - chip->base;

	if (pwm->chip_data) {
		pc->g_channel = ((struct group_pwm_config *)pwm->chip_data)->group_channel;
	}
	/* disable pwm controller */
	if (pc->g_channel) {
		if (disable_num == 0) {
			reg_offset = pc->data->per_offset;
			reg_width = 0x4;
			value = sunxi_pwm_readl(chip, reg_offset);
			value &= ~((0xf) << 4*(pc->g_channel - 1));
			sunxi_pwm_writel(chip, reg_offset, value);
			/* config clk gating */
			reg_shift &= ~(1);
			if (GET_BITS(reg_shift, 2, value) == 0) {
			    if (!pc->data->clk_gating_separate) {
				    get_pccr_reg_offset(index, &reg_offset);
				    reg_shift = PWM_CLK_GATING_SHIFT;
				    reg_width = PWM_CLK_GATING_WIDTH;
			    } else {
				    reg_offset = PCGR;
				    reg_shift = index;
				    reg_width = 0x1;
			    }
			    value = sunxi_pwm_readl(chip, reg_offset);
			    value &= ~((0xf) << 4*(pc->g_channel - 1));
			    //	value = SET_BITS(reg_shift, reg_width, value, 0);
			    sunxi_pwm_writel(chip, reg_offset, value);
		       }
		}
	} else {
		reg_offset = pc->data->per_offset;
		reg_shift = index;
		reg_width = 0x1;
		value = sunxi_pwm_readl(chip, reg_offset);
		value = SET_BITS(reg_shift, reg_width, value, 0);
		sunxi_pwm_writel(chip, reg_offset, value);

		/* config clk gating */
		reg_shift &= ~(1);
		if (GET_BITS(reg_shift, 2, value) == 0) {
		    if (!pc->data->clk_gating_separate) {
			    get_pccr_reg_offset(index, &reg_offset);
			    reg_shift = PWM_CLK_GATING_SHIFT;
			    reg_width = PWM_CLK_GATING_WIDTH;
		    } else {
			    reg_offset = PCGR;
			    reg_shift = index;
			    reg_width = 0x1;
		    }
		    value = sunxi_pwm_readl(chip, reg_offset);
		    value = SET_BITS(reg_shift, reg_width, value, 0);
		    sunxi_pwm_writel(chip, reg_offset, value);
		}
	}

	if (pc->g_channel)
		disable_num++;

	sub_np = of_parse_phandle(chip->dev->of_node, "sunxi-pwms", index);
	if (IS_ERR_OR_NULL(sub_np)) {
		pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
		return;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (IS_ERR_OR_NULL(pwm_pdevice)) {
		pr_err("%s: can't parse pwm device\n", __func__);
		return;
	}
	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_SLEEP);

	if (pc->g_channel) {
		group_reg_offset = PGR0 + 0x04 * (pc->g_channel - 1);
		/* group end */
		reg_shift = PWMG_START_SHIFT;
		value = sunxi_pwm_readl(chip, group_reg_offset);
		value = SET_BITS(reg_shift, 1, value, 0);/* group end */
		sunxi_pwm_writel(chip, group_reg_offset, value);

		/* group disable */
		reg_shift = PWMG_EN_SHIFT;
		value = sunxi_pwm_readl(chip, group_reg_offset);
		value = SET_BITS(reg_shift, 1, value, 0);/* group disable */
		sunxi_pwm_writel(chip, group_reg_offset, value);

		pc->g_channel = 0;
	}
}

static void sunxi_pwm_disable_dual(struct pwm_chip *chip, struct pwm_device *pwm, int bind_num)
{
	u32 value[2] = {0};
	unsigned int reg_offset[2], reg_shift[2], reg_width[2];
	struct device_node *sub_np[2];
	struct platform_device *pwm_pdevice[2];
	int i = 0;
	unsigned int pwm_index[2] = {0};
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	pwm_index[0] = pwm->pwm - chip->base;
	pwm_index[1] = bind_num - chip->base;

	/* get current index pwm device */
	sub_np[0] = of_parse_phandle(chip->dev->of_node, "sunxi-pwms", pwm_index[0]);
	if (IS_ERR_OR_NULL(sub_np[0])) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			return;
	}
	pwm_pdevice[0] = of_find_device_by_node(sub_np[0]);
	if (IS_ERR_OR_NULL(pwm_pdevice[0])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return;
	}
	/* get bind pwm device */
	sub_np[1] = of_parse_phandle(chip->dev->of_node, "sunxi-pwms", pwm_index[1]);
	if (IS_ERR_OR_NULL(sub_np[1])) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			return;
	}
	pwm_pdevice[1] = of_find_device_by_node(sub_np[1]);
	if (IS_ERR_OR_NULL(pwm_pdevice[1])) {
			pr_err("%s: can't parse pwm device\n", __func__);
			return;
	}

	/* disable pwm controller */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		reg_offset[i] = pc->data->per_offset;
		reg_shift[i] = pwm_index[i];
		reg_width[i] = 0x1;
		value[i] = sunxi_pwm_readl(chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 0);
		sunxi_pwm_writel(chip, reg_offset[i], value[i]);
	}

	/* disable pwm clk gating */
	for (i = 0; i < PWM_BIND_NUM; i++) {
		get_pccr_reg_offset(pwm_index[i], &reg_offset[i]);
		reg_shift[i] = PWM_CLK_GATING_SHIFT;
		reg_width[i] = 0x1;
		value[i] = sunxi_pwm_readl(chip, reg_offset[i]);
		value[i] = SET_BITS(reg_shift[i], reg_width[i], value[i], 0);
		sunxi_pwm_writel(chip, reg_offset[i], value[i]);
	}

	/* disable pwm dead zone,one for the two pwm */
	get_pdzcr_reg_offset(pc, pwm_index[0], &reg_offset[0]);
	reg_shift[0] = PWM_DZ_EN_SHIFT;
	reg_width[0] = PWM_DZ_EN_WIDTH;
	value[0] = sunxi_pwm_readl(chip, reg_offset[0]);
	value[0] = SET_BITS(reg_shift[0], reg_width[0], value[0], 0);
	sunxi_pwm_writel(chip, reg_offset[0], value[0]);

	/* config pin sleep */
	sunxi_pwm_pin_set_state(&pwm_pdevice[0]->dev, PWM_PIN_STATE_SLEEP);
	sunxi_pwm_pin_set_state(&pwm_pdevice[1]->dev, PWM_PIN_STATE_SLEEP);
}

static void sunxi_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	int bind_num;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	bind_num = pc->config[pwm->pwm - chip->base].bind_pwm;
	if (bind_num == 255)
		sunxi_pwm_disable_single(chip, pwm);
	else
		sunxi_pwm_disable_dual(chip, pwm, bind_num);
}

//TODO:  use pwm interrupt
/* Some soc have not interruput soure.
 * So,use CPU polling pwm capture interrupt status
 * default:24MHz
 * max input pwm period:2.7ms
 * min input pwm period:2.7us
 */
static int sunxi_pwm_capture(struct pwm_chip *chip, struct pwm_device *pwm,
		struct pwm_capture *result, unsigned long timeout)
{
	unsigned long long pwm_clk = 0, temp_clk;
	unsigned int pwm_div;
	unsigned int i = 0;
	/* spinlock_t pwm_lock; */
	/* unsigned long flags; */
	int cap_time[3];
	unsigned int value = 0, temp = 0, irq_num = 0;
	unsigned int reg_offset, reg_shift;
	struct device_node *sub_np;
	struct platform_device *pwm_pdevice;
	int index = pwm->pwm - chip->base;
	u32 pre_scal[][2] = {
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
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	sub_np = of_parse_phandle(chip->dev->of_node, "sunxi-pwms", index);
	if (IS_ERR_OR_NULL(sub_np)) {
		pr_err("%s: can't parse \"pwms\" property\n", __func__);
		return -ENODEV;
	}
	pwm_pdevice = of_find_device_by_node(sub_np);
	if (IS_ERR_OR_NULL(pwm_pdevice)) {
		pr_err("%s: can't parse pwm device\n", __func__);
		return -ENODEV;
	}
	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_ACTIVE);

	/* enable clk for pwm controller */
	get_pccr_reg_offset(index, &reg_offset);
	reg_shift = PWM_CLK_GATING_SHIFT;
	value = sunxi_pwm_readl(chip, reg_offset);
	value = SET_BITS(reg_shift, 1, value, 1);
	sunxi_pwm_writel(chip, reg_offset, value);

	get_pccr_reg_offset(index, &reg_offset);
	temp = sunxi_pwm_readl(chip, reg_offset);
	pwm_div = pre_scal[temp & (0x000f)][1];
	if (temp & (0x01 << PWM_CLK_SRC_SHIFT))
		pwm_clk = 100;//100M
	else
		pwm_clk = 24;//24M

	/* spin_lock_init(&pwm_lock); */
	/* spin_lock_irqsave(&pwm_lock, flags); */

	/* enable rise interrupt */
	temp = sunxi_pwm_readl(chip, PWM_CIER);
	temp = SET_BITS(index * 0x2, 0x1, temp, 0x1);
	sunxi_pwm_writel(chip, PWM_CIER, temp);
	/* Enable capture */
	temp = sunxi_pwm_readl(chip, pc->data->cer_offset);
	temp = SET_BITS(index, 0x1, temp, 0x1);
	sunxi_pwm_writel(chip, pc->data->cer_offset, temp);
	/* Clean capture rise status*/
	temp = sunxi_pwm_readl(chip, PWM_CISR);
	temp = SET_BITS(index * 0x2, 0x1, temp, 0x1);
	sunxi_pwm_writel(chip, PWM_CISR, temp);

	sunxi_pwm_writel(chip, pc->data->ccr_base_offset + index * PWM_REG_UNIFORM_OFFSET, 0x6);

	printk("time out is %ld\n", timeout);
	while (--timeout) {
		for (i = 0; i < 65535; i++) {
		/*
		 * Capture input:
		 *          _______               _______
		 *         |       |             |       |
		 * ________|       |_____________|       |________
		 *irq_num ^0      ^1                ^2
		 *
		 * Capture start by the first available rising edge.
		 */
		temp = sunxi_pwm_readl(chip, PWM_CISR);
		if ((temp & (0x1 << (index * 0x2))) &&
			(temp & (0x2 << (index * 0x2)))) {
			pr_err("input signal is constant of greater than Hz\n");
			goto err;
		}
		if (temp & (0x1 << (index * 0x2))) {
			if (irq_num == 1) {
				pr_err("pwm high time too short,can not capture,index:%d\n",
					irq_num);
				goto err;
			}
			cap_time[irq_num] = sunxi_pwm_readl(chip,
						pc->data->crlr_base_offset + index * PWM_REG_UNIFORM_OFFSET);
			irq_num++;

			/* clean irq status*/
			temp = sunxi_pwm_readl(chip, PWM_CISR);
			temp = SET_BITS(index * 0x2, 0x1, temp, 0x1);
			sunxi_pwm_writel(chip, PWM_CISR, temp);
			/* clean capture crlf */
			sunxi_pwm_writel(chip,
					pc->data->ccr_base_offset + index * PWM_REG_UNIFORM_OFFSET, 0x6);

			/* enable fail interrupt */
			temp = sunxi_pwm_readl(chip, PWM_CIER);
			temp = SET_BITS(1 + index * 0x2, 0x1, temp, 0x1);
			sunxi_pwm_writel(chip, PWM_CIER, temp);
		} else if (temp & (0x2 << (index * 0x2))) {
			if (irq_num == 0 || irq_num == 2) {
				pr_err("pwm low time too short,can not capture, index:%d\n",
					irq_num);
				goto err;
			}
			cap_time[irq_num] = sunxi_pwm_readl(chip,
					pc->data->cflr_base_offset + index * PWM_REG_UNIFORM_OFFSET);
			irq_num++;

			/* clean irq status*/
			temp = sunxi_pwm_readl(chip, PWM_CISR);
			temp = SET_BITS(1 + index * 0x2, 0x1, temp, 0x1);
			sunxi_pwm_writel(chip, PWM_CISR, temp);
			/* clean capture cflf */
			sunxi_pwm_writel(chip,
					pc->data->ccr_base_offset + index * PWM_REG_UNIFORM_OFFSET, 0x2);
		}
		if (irq_num > 2) {
err:
			/* spin_unlock_irqrestore(&pwm_lock, flags); */
			/* disable fail interrupt */
			temp = sunxi_pwm_readl(chip, PWM_CIER);
			temp = SET_BITS(1 + index * 0x2, 0x1, temp, 0x0);
			sunxi_pwm_writel(chip, PWM_CIER, temp);

			/* disable capture */
			temp = sunxi_pwm_readl(chip, pc->data->cer_offset);
			temp = SET_BITS(index, 0x1, temp, 0x0);
			sunxi_pwm_writel(chip, pc->data->cer_offset, temp);
			goto end;
		}
		}
	}
end:
	temp_clk = (cap_time[1] + cap_time[2]) * 1000 * pwm_div;
	do_div(temp_clk, pwm_clk);
	result->period = (unsigned int)temp_clk;
	temp_clk = cap_time[1] * 1000 * pwm_div;
	do_div(temp_clk, pwm_clk);
	result->duty_cycle = (unsigned int)temp_clk;

	reg_shift = index;
	value = sunxi_pwm_readl(chip, pc->data->per_offset);
	/*
	 * 0 , 1 --> 0
	 * 2 , 3 --> 2
	 * 4 , 5 --> 4
	 * 6 , 7 --> 6
	 */
	reg_shift &= ~(1);
	if (GET_BITS(reg_shift, 2, value) == 0) {
		value = sunxi_pwm_readl(chip, pc->data->cer_offset);
		if (GET_BITS(reg_shift, 2, value) == 0) {
			/* disable clk for pwm controller. */
			get_pccr_reg_offset(index, &reg_offset);
			reg_shift = PWM_CLK_GATING_SHIFT;
			value = sunxi_pwm_readl(chip, reg_offset);
			value = SET_BITS(reg_shift, 0x1, value, 0);
			sunxi_pwm_writel(chip, reg_offset, value);
		}
	}
	sunxi_pwm_pin_set_state(&pwm_pdevice->dev, PWM_PIN_STATE_SLEEP);

	if (timeout <= 0) {
		pr_err("%s: pwm capture timeout !\n", __func__);
		return -1;
	}
	return 0;
}

static void sunxi_pwm_get_state(struct pwm_chip *chip,
				struct pwm_device *pwm,
				struct pwm_state *state)
{
	unsigned int reg_offset;
	u32 val, sel;
	struct sunxi_pwm_chip *pc = to_sunxi_pwm_chip(chip);

	sel = pwm->pwm - chip->base;
	reg_offset = pc->data->pcr_base_offset + sel * PWM_REG_UNIFORM_OFFSET;

	val = sunxi_pwm_readl(chip, reg_offset);
	if (val & BIT_MASK(8)) {
		state->polarity = PWM_POLARITY_NORMAL;
	} else {
		state->polarity = PWM_POLARITY_INVERSED;
	}

}

static struct pwm_ops sunxi_pwm_ops = {
	.config = sunxi_pwm_config,
	.enable = sunxi_pwm_enable,
	.disable = sunxi_pwm_disable,
	.set_polarity = sunxi_pwm_set_polarity,
	.capture = sunxi_pwm_capture,
	.get_state = sunxi_pwm_get_state,
	.owner = THIS_MODULE,
};

static const struct of_device_id sunxi_pwm_match[] = {
	{ .compatible = "allwinner,sunxi-pwm",		.data = &sunxi_pwm_v200_data},
	{ .compatible = "allwinner,sunxi-s_pwm",	.data = &sunxi_pwm_v200_data},
	{ .compatible = "allwinner,sunxi-pwm-v100",	.data = &sunxi_pwm_v100_data},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sunxi_pwm_match);

static int sunxi_pwm_fill_hw_data(struct sunxi_pwm_chip *pwm)
{
	size_t size;
	const struct of_device_id *of_id;

	/* get hw data from match table*/
	of_id = of_match_device(sunxi_pwm_match, pwm->chip.dev);
	if (!of_id) {
		dev_err(pwm->chip.dev, "of_match_device() failed\n");
		return -EINVAL;
	}

	pwm->data = (struct sunxi_pwm_hw_data *)(of_id->data);

	size = sizeof(u32) * pwm->data->pm_regs_num;
	pwm->pm_regs_offset = devm_kzalloc(pwm->chip.dev, size, GFP_KERNEL);
	pwm->regs_backup = devm_kzalloc(pwm->chip.dev, size, GFP_KERNEL);

	/* Configure the registers that need to be saved for wake-up from sleep */
	pwm->pm_regs_offset[0] = PWM_PIER;
	pwm->pm_regs_offset[1] = PWM_CIER;
	pwm->pm_regs_offset[2] = pwm->data->per_offset;
	pwm->pm_regs_offset[3] = pwm->data->cer_offset;
	if(pwm->data->clk_gating_separate)
		pwm->pm_regs_offset[4] = PCGR;

	return 0;
}

static int sunxi_pwm_probe(struct platform_device *pdev)
{
	int ret;
	struct sunxi_pwm_chip *pwm;
	struct device_node *np = pdev->dev.of_node;
	int i;
	struct platform_device *pwm_pdevice;
	struct device_node *sub_np;

	pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);

	if (IS_ERR_OR_NULL(pwm))
		return -ENOMEM;

	platform_set_drvdata(pdev, pwm);
	pwm->chip.dev = &pdev->dev;

	ret = sunxi_pwm_fill_hw_data(pwm);
	if (ret) {
		dev_err(&pdev->dev, "unable to get hw_data\n");
		return ret;
	}

	/* io map pwm base */
	pwm->base = (void __iomem *)of_iomap(pdev->dev.of_node, 0);
	if (!pwm->base) {
		dev_err(&pdev->dev, "unable to map pwm registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}

	/* read property pwm-number */
	ret = of_property_read_u32(np, "pwm-number", &pwm->chip.npwm);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get pwm number: %d, force to one!\n", ret);
		/* force to one pwm if read property fail */
		pwm->chip.npwm = 1;
		goto err_iomap;
	}

	/* read property pwm-base */
	ret = of_property_read_u32(np, "pwm-base", &pwm->chip.base);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get pwm-base: %d, force to -1 !\n", ret);
		/* force to one pwm if read property fail */
		pwm->chip.base = -1;
	}

	pwm->chip.ops = &sunxi_pwm_ops;
	pwm->chip.of_xlate = of_pwm_xlate_with_flags;
	pwm->chip.of_pwm_n_cells = 3;

	/* add pwm chip to pwm-core */
	ret = pwmchip_add(&pwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		goto err_add;
	}

	pwm->config = devm_kzalloc(&pdev->dev, sizeof(*pwm->config) * pwm->chip.npwm, GFP_KERNEL);
	if (!pwm->config) {
		dev_err(&pdev->dev, "failed to allocate memory!\n");
		goto err_alloc;
	}

	for (i = 0; i < pwm->chip.npwm; i++) {
		sub_np = of_parse_phandle(np, "sunxi-pwms", i);
		if (IS_ERR_OR_NULL(sub_np)) {
			pr_err("%s: can't parse \"sunxi-pwms\" property\n", __func__);
			return -EINVAL;
		}

		pwm_pdevice = of_find_device_by_node(sub_np);
		/* it may be the program is error or the status of pwm%d  is disabled */
		if (!pwm_pdevice) {
			pr_debug("%s:fail to find device for pwm%d, continue!\n", __func__, i);
			continue;
		}
		ret = sunxi_pwm_get_config(pwm_pdevice, &pwm->config[i]);
		if (ret) {
			pr_err("Get config failed,exit!\n");
			goto err_get_config;
		}
	}

	pwm->pwm_clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(pwm->pwm_clk)) {
		pr_err("%s: can't get pwm clk\n", __func__);
		return -EINVAL;
	}
	pwm->pwm_rst_clk = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR_OR_NULL(pwm->pwm_rst_clk)) {
		pr_err("%s: can't get pwm reset clk\n", __func__);
		return -EINVAL;
	}
	reset_control_deassert(pwm->pwm_rst_clk);
	clk_prepare_enable(pwm->pwm_clk);

	return 0;

err_get_config:
err_alloc:
	pwmchip_remove(&pwm->chip);
err_add:
	iounmap(pwm->base);
err_iomap:
	return ret;
}

static int sunxi_pwm_remove(struct platform_device *pdev)
{
	struct sunxi_pwm_chip *pwm = platform_get_drvdata(pdev);
	clk_disable(pwm->pwm_clk);
	reset_control_assert(pwm->pwm_rst_clk);
	return pwmchip_remove(&pwm->chip);
}

#if IS_ENABLED(CONFIG_PM)

static void sunxi_pwm_stop_work(struct sunxi_pwm_chip *pwm)
{
	int i;
	bool pwm_state;

	for (i = 0; i < pwm->chip.npwm; i++) {
		pwm_state = pwm->chip.pwms[i].state.enabled;
		pwm_disable(&pwm->chip.pwms[i]);
		pwm->chip.pwms[i].state.enabled = pwm_state;
	}
}

static void sunxi_pwm_start_work(struct sunxi_pwm_chip *pwm)
{
	int i;
	struct pwm_state state;

	for (i = 0; i < pwm->chip.npwm; i++) {
		pwm_get_state(&pwm->chip.pwms[i], &state);
		pwm->chip.pwms[i].state.period = 0;
		pwm->chip.pwms[i].state.duty_cycle = 0;
		pwm->chip.pwms[i].state.polarity = PWM_POLARITY_NORMAL;
		pwm_apply_state(&pwm->chip.pwms[i], &state);
		if (pwm_is_enabled(&pwm->chip.pwms[i])) {
			pwm->chip.pwms[i].state.enabled = false;
			pwm_enable(&pwm->chip.pwms[i]);
		}
	}
}

static int sunxi_pwm_suspend(struct device *dev)
{
	struct platform_device *pdev = container_of(dev,
			struct platform_device, dev);
	struct sunxi_pwm_chip *pwm = platform_get_drvdata(pdev);

	sunxi_pwm_stop_work(pwm);

	sunxi_pwm_save_regs(pwm);

	clk_disable_unprepare(pwm->pwm_clk);

	reset_control_assert(pwm->pwm_rst_clk);

	return 0;
}

static int sunxi_pwm_resume(struct device *dev)
{
	struct platform_device *pdev = container_of(dev,
			struct platform_device, dev);
	struct sunxi_pwm_chip *pwm = platform_get_drvdata(pdev);
	int ret = 0;

	ret = reset_control_deassert(pwm->pwm_rst_clk);
	if (ret) {
		pr_err("reset_control_deassert() failed\n");
		return 0;
	}

	ret = clk_prepare_enable(pwm->pwm_clk);
	if (ret) {
		pr_err("clk_prepare_enable() failed\n");
		return 0;
	}

	sunxi_pwm_restore_regs(pwm);

	sunxi_pwm_start_work(pwm);

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
	int ret = 0;

	pr_info("pwm module init!\n");

#if !IS_ENABLED(CONFIG_OF)
	ret = platform_device_register(&sunxi_pwm_device);
#endif
	if (ret == 0) {
		ret = platform_driver_register(&sunxi_pwm_driver);
	}

	return ret;
}

static void __exit pwm_module_exit(void)
{
	pr_info("pwm module exit!\n");

	platform_driver_unregister(&sunxi_pwm_driver);
#if !IS_ENABLED(CONFIG_OF)
	platform_device_unregister(&sunxi_pwm_device);
#endif
}

subsys_initcall_sync(pwm_module_init);
module_exit(pwm_module_exit);

MODULE_AUTHOR("lihuaxing");
MODULE_DESCRIPTION("pwm driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pwm");
MODULE_VERSION("1.0.1");


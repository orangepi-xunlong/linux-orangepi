// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2016 Allwinner.
 * fuzhaoke <fuzhaoke@allwinnertech.com>
 *
 * SUNXI GPADC Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

/* #define DEBUG */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm.h>
#include <linux/pm_wakeirq.h>
#include <linux/pm_runtime.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>
#include <linux/of_device.h>

#if IS_ENABLED(CONFIG_IIO)
#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/regmap.h>
#endif

/* GPADC register offset */
/* Sample Rate config register */
#define GP_SR_REG			(0x00)

/* control register */
#define GP_CTRL_REG			(0x04)

/* compare and select enable register */
#define GP_CS_EN_REG			(0x08)

/* FIFO interrupt config register */
#define GP_FIFO_INTC_REG		(0x0c)

/* FIFO interrupt status register */
#define GP_FIFO_INTS_REG		(0x10)

/* FIFO data register */
#define GP_FIFO_DATA_REG		(0x14)

/* calibration data register */
#define GP_CB_DATA_REG			(0x18)

#define GP_DATAL_INTC_REG		(0x20)
#define GP_DATAH_INTC_REG		(0x24)
#define GP_DATA_INTC_REG		(0x28)
#define GP_DATAL_INTS_REG		(0x30)
#define GP_DATAH_INTS_REG		(0x34)
#define GP_DATA_INTS_REG		(0x38)

/* channel N compare data register */
#define GP_CH0_CMP_DATA_REG		(0x40)
#define GP_CH1_CMP_DATA_REG		(0x44)
#define GP_CH2_CMP_DATA_REG		(0x48)
#define GP_CH3_CMP_DATA_REG		(0x4c)
#define GP_CH4_CMP_DATA_REG		(0x50)
#define GP_CH5_CMP_DATA_REG		(0x54)
#define GP_CH6_CMP_DATA_REG		(0x58)
#define GP_CH7_CMP_DATA_REG		(0x5c)
#define GP_CH8_CMP_DATA_REG		(0x60)
#define GP_CH9_CMP_DATA_REG		(0x64)
#define GP_CHA_CMP_DATA_REG		(0x68)
#define GP_CHB_CMP_DATA_REG		(0x6c)
#define GP_CHC_CMP_DATA_REG		(0x70)
#define GP_CHD_CMP_DATA_REG		(0x74)
#define GP_CHE_CMP_DATA_REG		(0x78)
#define GP_CHF_CMP_DATA_REG		(0x7c)

/* channel N data register */
#define GP_CH0_DATA_REG			(0x80)

/*
 * GP_SR_REG default value: 0x01df_002f 50KHZ
 * sample_rate = clk_in/(n+1) = 24MHZ/(0x1df + 1) = 50KHZ
 */
#define GP_SR_DIV_MASK			(0xffff << 16)
#define GP_SR_DIV_SHIFT			16

/* delay time of the first time */
#define GP_FIRST_CONCERT_DLY		(0xff << 24)

/* After calibration, this bit is automatically cleared to 0 */
#define GP_CALI_EN			BIT(17)

/* GPADC function enable, only when 'single conversion mode' is selected,
 * this bit is automatically cleared to 0
 */
#define GP_ADC_EN			BIT(16)

/*
 * 00:single conversion mode
 * 01:single-cycle conversion mode
 * 10:continuous mode
 * 11:burst mode
 */
#define GP_MODE_MASK			(3 << 18)
#define GP_MODE_SHIFT			(18)

/*
 * GP_FIFO_INTC_REG default value: 0x0000_0f00
 * 0:disable, 1:enable
 */

/* fifo over run irq enable */
#define FIFO_OVER_IRQ_EN		BIT(17)

/* fifo data irq enable */
#define FIFO_DATA_IRQ_EN		BIT(16)

/* write 1 to flush TX FIFO, self clear to 0 */
#define FIFO_FLUSH			BIT(4)

/* 0: no pending irq, 1: over pending, need write 1 to clear flag */

/* fifo over pending flag */
#define FIFO_OVER_PEND			BIT(17)

/* fifo data pending flag */
#define FIFO_DATA_PEND			BIT(16)

/* the data count in fifo */
#define FIFO_CNT			(0x3f << 8)

/* GPADC data in fifo */
#define GP_FIFO_DATA			(0xfff << 0)

/* GPADC calibration data */
#define GP_CB_DATA			(0xfff << 0)

#define GP_CH_CMP_DATA_SHIFT		(0x4)
#define GP_CH_CMP_DATA_MASK		(0xfff)
#define GP_CH_CMP_HIGH_DATA_SHIFT	(16)
#define GP_CH_CMP_SHIFT			16

#define ADDRESS(offset, id, width) ((offset) + ((id) * (width)))
#define KEY_MAX_CNT			13
#define VOL_NUM				KEY_MAX_CNT
#define MAX_INPUT_VOLTAGE		1800
#define DEVIATION			100
#define SUNXIKEY_DOWN			(MAX_INPUT_VOLTAGE - DEVIATION)
#define SUNXIKEY_UP			SUNXIKEY_DOWN
#define MAXIMUM_SCALE			128
#define SCALE_UNIT			(MAX_INPUT_VOLTAGE / MAXIMUM_SCALE)
#define SAMPLING_FREQUENCY		10
#define SUNXI_GPADC_DEV_NAME		"sunxi-gpadc"
#define OSC_24MHZ			(24000000UL)
#define MAX_SR				(1000000UL)
#define MIN_SR				(400UL)
#define DEFAULT_SR			(1000UL)

/* reference voltage 1.8v */
#define VOL_REFER			(1800000UL)

/* 4095 <---> 12bits resolution ratio */
#define GP_RESOLUTION_RATIO		(0xfff)
#define GP_KEY_MAP_MASK			(0xff)

/* The maximum number of channels of a single gpadc chip */
#define CHANNEL_MAX_NUM                 16

/* The maximum number of gpadc chip resources */
#define GPADC_MAX_CHIP			2

#define STR_SIZE			30
enum sunxi_gpadc_mode {
	GP_SINGLE_MODE,
	GP_SINGLE_CYCLE_MODE,
	GP_CONTINUOUS_MODE,
	GP_BURST_MODE,
};

struct sunxi_gpadc_config {
	u32 mode_select;  /* select work mode */
	u32 channel_select;  /* selected channel to use */
	u32 keyadc_select;  /* channel as keyadc */
	u32 data_select;  /* enable data irq channel */
	u32 cld_select;  /* enable compare low data irq channel */
	u32 chd_select;  /* enable compare high data irq channel */
	u32 cmp_lowdata[CHANNEL_MAX_NUM];
	u32 cmp_highdata[CHANNEL_MAX_NUM];
	u32 scan_data;  /* enable channel scan_data */
};

struct sunxi_gpadc_status_reg {
	unsigned char channel;  /* gpadc channel */
	unsigned char val;
};

struct sunxi_gpadc_vol_reg {
	unsigned char channel;  /* gpadc channel */
	unsigned char index;
	unsigned long vol;
};

struct sunxi_gpadc_unit_reg {
	char *pst;
	unsigned long val;
};

/* Registers which needs to be saved and restored before and after sleeping */
static u32 sunxi_gpadc_regs_offset[] = {
	GP_SR_REG,
	GP_CTRL_REG,
	GP_CS_EN_REG,
	GP_FIFO_INTC_REG,
	GP_FIFO_DATA_REG,
	GP_CB_DATA_REG,
	GP_DATAL_INTC_REG,
	GP_DATAH_INTC_REG,
	GP_DATA_INTC_REG,
	GP_CH0_CMP_DATA_REG,
	GP_CH1_CMP_DATA_REG,
	GP_CH2_CMP_DATA_REG,
	GP_CH3_CMP_DATA_REG,
	GP_CH4_CMP_DATA_REG,
	GP_CH5_CMP_DATA_REG,
	GP_CH6_CMP_DATA_REG,
	GP_CH7_CMP_DATA_REG,
	GP_CH8_CMP_DATA_REG,
	GP_CH9_CMP_DATA_REG,
	GP_CHA_CMP_DATA_REG,
	GP_CHB_CMP_DATA_REG,
	GP_CHC_CMP_DATA_REG,
	GP_CHD_CMP_DATA_REG,
	GP_CHE_CMP_DATA_REG,
	GP_CHF_CMP_DATA_REG,
};

struct sunxi_gpadc_hw_data {
	bool has_bus_clk_hosc;
};

struct sunxi_gpadc {
	/* gpadc framework data */
	struct platform_device	*pdev;
	struct device *dev;
	struct device *class_dev;
	struct input_dev *input_gpadc[CHANNEL_MAX_NUM];
	struct sunxi_gpadc_config gpadc_config;
	u32 scankeycodes[CHANNEL_MAX_NUM][KEY_MAX_CNT];

	/* dts data */
	struct clk *bus_clk;
	struct clk *bus_clk_hosc;
	struct reset_control *reset;
	struct resource	*res;
	void __iomem *reg_base;
	int irq_num;
	u32 controller_num;
	u32 wakeup_en;
	u32 channel_num;
	u32 gpadc_sample_rate;
	char key_name[CHANNEL_MAX_NUM][16];
	u32 key_num[CHANNEL_MAX_NUM];
	u8 key_cnt;

	/* other data */
	u32 key_vol[CHANNEL_MAX_NUM][VOL_NUM];
	u8 filter_cnt;
	u8 channel;
	u8 compare_before;
	u8 compare_later;
	u8 key_code;
	const struct sunxi_gpadc_hw_data *data;
	unsigned char keypad_mapindex[CHANNEL_MAX_NUM][MAXIMUM_SCALE];
	u32 regs_backup[ARRAY_SIZE(sunxi_gpadc_regs_offset)];
};

static struct sunxi_gpadc global_gpadc[GPADC_MAX_CHIP];

static u32 sunxi_gpadc_sample_rate_read(void __iomem *reg_base, u32 clk_in)
{
	u32 div, round_clk;

	div = readl(reg_base + GP_SR_REG);
	div = div >> GP_SR_DIV_SHIFT;
	round_clk = clk_in / (div + 1);

	return round_clk;
}

/* clk_in: source clock, round_clk: sample rate */
static void sunxi_gpadc_sample_rate_set(void __iomem *reg_base, u32 clk_in,
		u32 round_clk)
{
	u32 div, reg_val;

	div = clk_in / round_clk - 1;
	reg_val = readl(reg_base + GP_SR_REG);
	reg_val = (reg_val & ~GP_SR_DIV_MASK) | (div << GP_SR_DIV_SHIFT);
	writel(reg_val, reg_base + GP_SR_REG);
}

static void sunxi_gpadc_ch_enable(void __iomem *reg_base, u32 id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	reg_val |= BIT(id);
	writel(reg_val, reg_base + GP_CS_EN_REG);
}

static void sunxi_gpadc_ch_disable(void __iomem *reg_base, u32 id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	reg_val &= ~(BIT(id));
	writel(reg_val, reg_base + GP_CS_EN_REG);
}

static void sunxi_gpadc_ch_cmp_low(void __iomem *reg_base, u32 id,
							u32 low_uv)
{
	u32 reg_val, low, unit;

	/* anolog voltage range 0~1.8v, 12bits resolution ratio, unit = 1.8v / (2^12 - 1) */
	unit = VOL_REFER / GP_RESOLUTION_RATIO;
	low = low_uv / unit;
	if (low > GP_RESOLUTION_RATIO)
		low = GP_RESOLUTION_RATIO;

	reg_val = readl(reg_base + ADDRESS(GP_CH0_CMP_DATA_REG, id, GP_CH_CMP_DATA_SHIFT));
	reg_val &= ~GP_RESOLUTION_RATIO;
	reg_val |= low & GP_RESOLUTION_RATIO;
	writel(reg_val, reg_base + ADDRESS(GP_CH0_CMP_DATA_REG, id, GP_CH_CMP_DATA_SHIFT));
}

static void sunxi_gpadc_ch_cmp_high(void __iomem *reg_base, u32 id, u32 hig_uv)
{
	u32 reg_val, hig, unit;

	/* anolog voltage range 0~1.8v, 12bits resolution ratio, unit = 1.8v / (2^12 - 1) */
	unit = VOL_REFER / GP_RESOLUTION_RATIO;
	hig = hig_uv / unit;
	if (hig > GP_RESOLUTION_RATIO)
		hig = GP_RESOLUTION_RATIO;

	reg_val = readl(reg_base + ADDRESS(GP_CH0_CMP_DATA_REG, id, GP_CH_CMP_DATA_SHIFT));
	reg_val &= ~(GP_RESOLUTION_RATIO << GP_CH_CMP_HIGH_DATA_SHIFT);
	reg_val |= (hig & GP_RESOLUTION_RATIO) << GP_CH_CMP_HIGH_DATA_SHIFT;
	writel(reg_val, reg_base + ADDRESS(GP_CH0_CMP_DATA_REG, id, GP_CH_CMP_DATA_SHIFT));
}

static void sunxi_gpadc_cmp_enable(void __iomem *reg_base, u32 id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	reg_val |= BIT(id + GP_CH_CMP_SHIFT);
	writel(reg_val, reg_base + GP_CS_EN_REG);
}

static void sunxi_gpadc_cmp_disable(void __iomem *reg_base, u32 id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	reg_val &= ~BIT(id + GP_CH_CMP_SHIFT);
	writel(reg_val, reg_base + GP_CS_EN_REG);
}

static void sunxi_gpadc_set_mode(void __iomem *reg_base, enum sunxi_gpadc_mode mode)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val &= ~GP_MODE_MASK;
	reg_val |= (mode << GP_MODE_SHIFT);
	writel(reg_val, reg_base + GP_CTRL_REG);
}

static void sunxi_gpadc_enable(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val |= GP_ADC_EN;
	writel(reg_val, reg_base + GP_CTRL_REG);
}

static void sunxi_gpadc_disable(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val &= ~GP_ADC_EN;
	writel(reg_val, reg_base + GP_CTRL_REG);
}

static void sunxi_gpadc_calibration_enable(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val |= GP_CALI_EN;
	writel(reg_val, reg_base + GP_CTRL_REG);

}

static void sunxi_gpadc_lowirq_control(void __iomem *reg_base, u32 id, bool irq_en)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATAL_INTC_REG);

	if (irq_en)
		reg_val |= BIT(id);
	else
		reg_val &= ~BIT(id);

	writel(reg_val, reg_base + GP_DATAL_INTC_REG);
}

static void sunxi_gpadc_highirq_control(void __iomem *reg_base, u32 id, bool irq_en)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATAH_INTC_REG);

	if (irq_en)
		reg_val |= BIT(id);
	else
		reg_val &= ~BIT(id);

	writel(reg_val, reg_base + GP_DATAH_INTC_REG);
}

static void sunxi_gpadc_datairq_control(void __iomem *reg_base, bool irq_en)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_FIFO_INTC_REG);

	if (irq_en)
		reg_val |= FIFO_DATA_IRQ_EN;
	else
		reg_val &= ~FIFO_DATA_IRQ_EN;

	writel(reg_val, reg_base + GP_FIFO_INTC_REG);
}

static u32 sunxi_gpadc_read_data(void __iomem *reg_base, u32 id)
{
	return readl(reg_base + ADDRESS(GP_CH0_DATA_REG, id, GP_CH_CMP_DATA_SHIFT))
							& GP_CH_CMP_DATA_MASK;
}

static void sunxi_gpadc_key_report(struct sunxi_gpadc *chip, u32 data, u32 id)
{
	/* vin_m: input voltage, the unit is millivolt
	 * vin_u: input voltage, the unit is microvolt
	 */
	u32 vin_m, vin_u;

	/* vin = (Vref / 4095) * data, 4095 <---> 12bits resolution ratio */
	vin_u = ((VOL_REFER / GP_RESOLUTION_RATIO) * data);
	vin_m = vin_u / 1000;

	if (vin_m < SUNXIKEY_DOWN) {
		dev_dbg(chip->dev, "data is %u, vin_m is %u\n", data, vin_m);

		/* MAX compare_before = 128 */
		chip->compare_before = (vin_m / SCALE_UNIT) & GP_KEY_MAP_MASK;
		dev_dbg(chip->dev, "bf=%3d lt=%3d\n", chip->compare_before, chip->compare_later);

		if (chip->compare_before >= chip->compare_later - 1
			&& chip->compare_before <= chip->compare_later + 1)
			chip->key_cnt++;
		else
			chip->key_cnt = 0;

		chip->compare_later = chip->compare_before;

		/* when the voltage for filter_cnt consecutive times remains the same,
		 * the condition of reporting the key is met;
		 * it can avoid the problem of voltage mutation caused by unfiltered hardware
		 */
		if (chip->key_cnt >= chip->filter_cnt) {
			chip->key_code = chip->keypad_mapindex[id][chip->compare_before];

			/* When the number of keys does not exceed the limit, report the keys */
			if (chip->key_code != chip->key_num[id]) {
				input_report_key(chip->input_gpadc[id],
							chip->scankeycodes[id][chip->key_code], 1);
				input_sync(chip->input_gpadc[id]);
			}

			chip->compare_later = 0;
			chip->key_cnt = 0;
		}
	}
}

static void sunxi_gpadc_data_report(struct sunxi_gpadc *chip, u32 channel)
{
	u32 data;

	data = sunxi_gpadc_read_data(chip->reg_base, channel);
	input_event(chip->input_gpadc[channel], EV_MSC, MSC_SCAN, data);
	input_sync(chip->input_gpadc[channel]);
}

static void sunxi_gpadc_low_key_report(struct sunxi_gpadc *chip, u32 channel)
{
	u32 data;

	data = sunxi_gpadc_read_data(chip->reg_base, channel);
	sunxi_gpadc_key_report(chip, data, channel);
	sunxi_gpadc_highirq_control(chip->reg_base, channel, true);
}

static void sunxi_gpadc_high_key_report(struct sunxi_gpadc *chip, u32 channel)
{
	input_report_key(chip->input_gpadc[channel],
				chip->scankeycodes[channel][chip->key_code], 0);
	input_sync(chip->input_gpadc[channel]);
	chip->compare_later = 0;
	chip->key_cnt = 0;
	sunxi_gpadc_highirq_control(chip->reg_base, channel, false);
}

static irqreturn_t sunxi_gpadc_irq_handler(int irqno, void *dev_id)
{
	struct sunxi_gpadc *chip = (struct sunxi_gpadc *)dev_id;
	struct sunxi_gpadc_config *config = &chip->gpadc_config;
	u32 data_irq_status, low_data_irq_status, high_data_irq_status;
	u32 data_irq_en, low_data_irq_en, high_data_irq_en;
	u32 i;

	/* check the data irq coming or not with data irq enable or not */
	data_irq_en = readl(chip->reg_base + GP_DATA_INTC_REG);
	/* check the low data irq coming or not with low data irq enable or not */
	low_data_irq_en = readl(chip->reg_base + GP_DATAL_INTC_REG);
	/* check the high data irq coming or not with high data irq enable or not */
	high_data_irq_en = readl(chip->reg_base + GP_DATAH_INTC_REG);

	/* Read the data interrupt status and clear the interrupt */
	data_irq_status = readl(chip->reg_base + GP_DATA_INTS_REG);
	writel(data_irq_status, chip->reg_base + GP_DATA_INTS_REG);

	/* Read the low data interrupt status and clear the interrupt */
	low_data_irq_status = readl(chip->reg_base + GP_DATAL_INTS_REG);
	writel(low_data_irq_status, chip->reg_base + GP_DATAL_INTS_REG);

	/* Read the high data interrupt status and clear the interrupt */
	high_data_irq_status = readl(chip->reg_base + GP_DATAH_INTS_REG);
	writel(high_data_irq_status, chip->reg_base + GP_DATAH_INTS_REG);

	for (i = 0; i < chip->channel_num; i++) {
		if (config->keyadc_select & BIT(i)) {
			if (low_data_irq_status & low_data_irq_en & BIT(i))
				sunxi_gpadc_low_key_report(chip, i);
			else if (high_data_irq_status & high_data_irq_en & BIT(i))
				sunxi_gpadc_high_key_report(chip, i);
			else
				dev_dbg(chip->dev, "unknown key irq\n");
		} else {
			if (data_irq_status & data_irq_en & BIT(i)) {
				sunxi_gpadc_data_report(chip, i);
				dev_dbg(chip->dev, "channel %d data pend\n", i);
			}

			if (low_data_irq_en & high_data_irq_en & BIT(i)) {
				if (low_data_irq_status & high_data_irq_status & BIT(i)) {
					sunxi_gpadc_data_report(chip, i);
					dev_dbg(chip->dev, "channel %d low and hig pend\n", i);
				} else {
					dev_dbg(chip->dev, "invalid range\n");
				}
			} else {
				if (low_data_irq_status & low_data_irq_en
							& ~high_data_irq_en
							& BIT(i)) {
					sunxi_gpadc_data_report(chip, i);
					dev_dbg(chip->dev, "channel %d low pend\n", i);
				} else if (high_data_irq_status & high_data_irq_en
								& ~low_data_irq_en
								& BIT(i)) {
					sunxi_gpadc_data_report(chip, i);
					dev_dbg(chip->dev, "channel %d hig pend\n", i);
				} else
					dev_dbg(chip->dev, "unknown data irq\n");
			}
		}
	}

	return IRQ_HANDLED;
}

static int sunxi_gpadc_parse_str(const char *buf, size_t size, char *str,
				 struct sunxi_gpadc_unit_reg *temp)
{
	char *ptr = NULL;
	char *pst, *ped;

	if (!buf)
		goto err;

	pst = (char *)buf;
	ped = pst;
	ped = strnchr(pst, size, ',');
	if (!ped)
		goto err;

	*ped = '\0';
	ped++;
	pst = strim(pst);
	ped = strim(ped);

	/* ignore uppercase and lowercase letters, split strings and numbers */
	ptr = strstr(pst, str);
	if (!ptr) {
		ptr = pst;
		while (*ptr != 0) {
			if (*ptr >= 'A' && *ptr <= 'Z')
				*ptr += 32;
			ptr++;
		}
		ptr = pst;
		ptr = strstr(ptr, str);
		if (!ptr)
			goto err;
	}

	temp->val = *(pst + strlen(str)) - 48;
	temp->pst = ped;

	return 0;
err:
	return -EINVAL;
}

static int sunxi_gpadc_parse_status_str(struct sunxi_gpadc *chip,
					const char *buf,
					size_t size,
					struct sunxi_gpadc_status_reg *reg_tmp)
{
	struct sunxi_gpadc_unit_reg tmp1;
	int err;

	err = sunxi_gpadc_parse_str(buf, size, "gpadc", &tmp1);
	if (err) {
		dev_err(chip->dev, "parse str failed\n");
		return err;
	}

	reg_tmp->channel = tmp1.val;
	if (reg_tmp->channel < 0
			|| reg_tmp->channel > (chip->channel_num - 1)) {
		dev_err(chip->dev, "config channel num failed\n");
		return -EINVAL;
	}

	reg_tmp->val = *tmp1.pst - 48;

	return 0;
}

static int sunxi_gpadc_parse_vol_str(struct sunxi_gpadc *chip,
				const char *buf,
				size_t size,
				struct sunxi_gpadc_vol_reg *reg_tmp)
{
	int err;
	struct sunxi_gpadc_unit_reg tmp1, tmp2;

	err = sunxi_gpadc_parse_str(buf, size, "gpadc", &tmp1);
	if (err)
		return err;

	reg_tmp->channel = tmp1.val;
	if (reg_tmp->channel < 0
			|| reg_tmp->channel > (chip->channel_num - 1))
		return -EINVAL;

	err = sunxi_gpadc_parse_str(tmp1.pst, strlen(tmp1.pst), "vol", &tmp2);
	if (err)
		return err;

	reg_tmp->index = tmp2.val;
	if (reg_tmp->index < 0
			|| reg_tmp->index > (chip->key_num[reg_tmp->channel] - 1))
		return -EINVAL;

	err = kstrtoul(tmp2.pst, 10, &reg_tmp->vol);
	if (err)
		return err;

	return 0;
}

static int sunxi_gpadc_parse_unit_str(const char *buf,
					size_t size,
					struct sunxi_gpadc_unit_reg *reg_tmp)
{
	int ret;

	if (!buf)
		return -EINVAL;

	reg_tmp->pst = (char *)buf;
	reg_tmp->pst = strim(reg_tmp->pst);

	ret = kstrtoul(reg_tmp->pst, 10, &reg_tmp->val);
	if (ret)
		return ret;

	return 0;
}

/**
 * cat status
 * View the switch status of each channel of gpadc
 */
static ssize_t
sunxi_gpadc_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);
	int i;
	u32 reg_val;

	reg_val = readl(chip->reg_base + GP_CS_EN_REG);

	for (i = 0; i < chip->channel_num; i++) {
		if (reg_val & (1 << i))
			dev_info(chip->dev, "channel%d: okay\n", i);
		else
			dev_info(chip->dev, "channel%d: disable\n", i);
	}

	return 0;
}

/**
 * echo gpadc0,0 > statue
 * echo gpadcn,status > status
 * n is the number of gpadc channel
 * status: 0 or 1, 0: disable, 1: enable
 */
static ssize_t
sunxi_gpadc_status_store(struct device *dev, struct device_attribute *attr,
								const char *buf,
								size_t count)
{
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);
	struct sunxi_gpadc_status_reg status_para;
	int err;

	err = sunxi_gpadc_parse_status_str(chip, buf, count, &status_para);
	if (err) {
		dev_err(chip->dev, "%s(): %d: err, invalid status para\n", __func__, __LINE__);
		return err;
	}

	if (status_para.val) {
		sunxi_gpadc_ch_enable(chip->reg_base, status_para.channel);
		dev_info(chip->dev, "Enable channel %u\n", status_para.channel);
	} else {
		sunxi_gpadc_ch_disable(chip->reg_base, status_para.channel);
		dev_info(chip->dev, "Disable channel %u\n", status_para.channel);
	}

	return count;
}

/**
 * cat vol
 * view the sampling voltage value and key index mapping of all keys
 */
static ssize_t
sunxi_gpadc_vol_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);
	int i, chan;
	char tmp;

	for (chan = 0; chan < chip->channel_num; chan++) {
		dev_info(chip->dev, "key_cnt = %d\n", chip->key_num[chan]);
		for (i = 0; i < chip->key_num[chan]; i++)
			dev_info(chip->dev, "key%d_vol = %d\n", i, chip->key_vol[chan][i]);

		dev_info(chip->dev, "keypad_mapindex[%d][%d] = {", chan, MAXIMUM_SCALE);
		for (i = 0; i < MAXIMUM_SCALE; i++) {
			if (chip->keypad_mapindex[chan][i] != tmp)
				dev_info(chip->dev, "\n\t");
			dev_info(chip->dev, "%d, ", chip->keypad_mapindex[chan][i]);
			tmp = chip->keypad_mapindex[chan][i];
		}
		dev_dbg(chip->dev, "\n}\n");
	}

	return 0;
}

/**
 * echo vol0,125 > vol
 * set the voltage value corresponding to key0 to 125 mv
 */
static ssize_t
sunxi_gpadc_vol_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned char i, j = 0;
	u32 gap = 0, half_gap, cnt = 0;
	u32 set_vol[VOL_NUM];
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);
	struct sunxi_gpadc_vol_reg vol_para;
	int err;

	err = sunxi_gpadc_parse_vol_str(chip, buf, count, &vol_para);
	if (err) {
		dev_err(chip->dev, "%s(): %d: err, invalid vol para!\n", __func__, __LINE__);
		return err;
	}

	chip->key_vol[vol_para.channel][vol_para.index] = vol_para.vol;

	for (i = chip->key_num[vol_para.channel] - 1; i > 0; i--) {
		gap += chip->key_vol[vol_para.channel][i]
			- chip->key_vol[vol_para.channel][i - 1];
		cnt++;
	}
	half_gap = gap / cnt / 2;
	for (i = 0; i < chip->key_num[vol_para.channel]; i++)
		set_vol[i] = chip->key_vol[vol_para.channel][i] + half_gap;

	for (i = 0; i < MAXIMUM_SCALE; i++) {
		if (i * SCALE_UNIT > set_vol[j])
			j++;
		chip->keypad_mapindex[vol_para.channel][i] = j;
	}

	return count;
}

/**
 * cat sr
 * get the current sampling frequency
 */
static ssize_t
sunxi_gpadc_sr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u32 sr;
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);

	sr = sunxi_gpadc_sample_rate_read(chip->reg_base, OSC_24MHZ);

	return scnprintf(buf, PAGE_SIZE, "gpadc sampling frequency: %u\n", sr);
}

/**
 * echo 5000 > sr
 * set the sampling frequency to 5000
 */
static ssize_t
sunxi_gpadc_sr_store(struct device *dev, struct device_attribute *attr,
							const char *buf,
							size_t count)
{
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);
	struct sunxi_gpadc_unit_reg sr_para;
	int err;

	err = sunxi_gpadc_parse_unit_str(buf, count, &sr_para);
	if (err) {
		dev_err(chip->dev, "%s(): %d: err, invalid sr para!\n", __func__, __LINE__);
		return err;
	}

	if (sr_para.val < MIN_SR || sr_para.val > MAX_SR) {
		dev_err(chip->dev, "%s(): %d: err, sampling frequency should be: [%lu,%lu]\n",
				__func__, __LINE__, MIN_SR, MAX_SR);
		return -EINVAL;
	}

	sunxi_gpadc_sample_rate_set(chip->reg_base, OSC_24MHZ, sr_para.val);

	return count;
}

/**
 * cat filter
 * get the current filter cnt
 */
static ssize_t
sunxi_gpadc_filter_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "filter_cnt = %u\n", chip->filter_cnt);
}

/**
 * echo 6 > filter
 * set the filter cnt to 6
 */
static ssize_t
sunxi_gpadc_filter_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);
	struct sunxi_gpadc_unit_reg filter_para;
	int err;

	err = sunxi_gpadc_parse_unit_str(buf, count, &filter_para);
	if (err) {
		dev_err(chip->dev, "%s(): %d: err, invalid filter para!\n", __func__, __LINE__);
		return err;
	}

	chip->filter_cnt = filter_para.val;

	return count;
}

/**
 * Get the voltage value of the specified channel of the gpadc chip
 *
 * @controller_num: Specified gpadc chip
 * @channel: Channel specified by gpadc chip
 *
 * returns: The voltage value obtained by the specified channel of the gpadc chip
 */
u32 sunxi_gpadc_read_channel_data(u32 controller_num, u8 channel)
{
	u32 reg_val, data, vin_u, vin_m;
	struct sunxi_gpadc *chip = &global_gpadc[controller_num];

	reg_val = readl(chip->reg_base + GP_CS_EN_REG);

	if ((reg_val & BIT(channel)) == 0)
		return VOL_REFER + 1;

	data = sunxi_gpadc_read_data(chip->reg_base, channel);

	vin_u = ((VOL_REFER / GP_RESOLUTION_RATIO) * data);
	vin_m = vin_u / 1000;

	dev_dbg(chip->dev, "vol_data: %d\n", vin_m);

	return vin_m;
}
EXPORT_SYMBOL_GPL(sunxi_gpadc_read_channel_data);

/*
 * cat data, get the voltage of the current channel
 * it should be executed first: echo channel > data, switch to gpadc channel
 */
static ssize_t
sunxi_gpadc_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int data;
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);

	data = sunxi_gpadc_read_channel_data(chip->controller_num, chip->channel);

	return scnprintf(buf, PAGE_SIZE, "voltage data is %u\n", data);
}

/*
 * echo channel_n > data, switch to gpadc channel_n
 * eg: echo 0 > data, switch to channel 0
 */
static ssize_t
sunxi_gpadc_data_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);
	struct sunxi_gpadc_unit_reg channel_para;
	int err;

	err = sunxi_gpadc_parse_unit_str(buf, count, &channel_para);
	if (err) {
		dev_err(chip->dev, "%s(): %d: err, invalid data para!\n", __func__, __LINE__);
		return err;
	}

	chip->channel = channel_para.val;

	return count;
}

static struct device_attribute gpadc_class_attrs[] = {
	__ATTR(status, 0644, sunxi_gpadc_status_show, sunxi_gpadc_status_store),
	__ATTR(vol,    0644, sunxi_gpadc_vol_show,    sunxi_gpadc_vol_store),
	__ATTR(sr,     0644, sunxi_gpadc_sr_show,     sunxi_gpadc_sr_store),
	__ATTR(filter, 0644, sunxi_gpadc_filter_show, sunxi_gpadc_filter_store),
	__ATTR(data,   0644, sunxi_gpadc_data_show,   sunxi_gpadc_data_store),
};

static struct class gpadc_class = {
	.name		= "gpadc",
	.owner		= THIS_MODULE,
};

#if IS_ENABLED(CONFIG_IIO)
struct sunxi_gpadc_iio {
	struct sunxi_gpadc *sunxi_gpadc;
};

static int sunxi_gpadc_read_raw(struct iio_dev *indio_dev,
			struct iio_chan_spec const *chan,
			int *val, int *val2, long mask)
{
	int ret = 0;
	struct sunxi_gpadc_iio *info = iio_priv(indio_dev);
	struct sunxi_gpadc *sunxi_gpadc = info->sunxi_gpadc;
	u32 data, vin_m, vin_u;

	mutex_lock(&indio_dev->mlock);
	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		/*
		 * This is an example,so if there are new requirements,
		 * you can replace it with your interface to upload data.
		 * *val and *val2 are used as needed.
		 * In the shell console you can use the "cat in_voltage0_raw"
		 * command to view the data.
		 */
		data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, chan->channel);

		vin_u = ((VOL_REFER / GP_RESOLUTION_RATIO) * data);
		vin_m = vin_u / 1000;
		*val  = vin_m;

		ret = IIO_VAL_INT;
		break;

	case IIO_CHAN_INFO_SCALE:
		break;

	default:
		ret = -EINVAL;
	}
	mutex_unlock(&indio_dev->mlock);

	return ret;
}

/*
 * If necessary, you can fill other callback functions
 * in this data structure, for example:
 * write_raw/read_event_value/write_event_value and so on...
 */
static const struct iio_info sunxi_gpadc_iio_info = {
	.read_raw = &sunxi_gpadc_read_raw,
};

static void sunxi_gpadc_remove_iio(void *data)
{
	struct iio_dev *indio_dev = data;

	if (!indio_dev)
		dev_err(&indio_dev->dev, "indio_dev is null\n");
	else
		iio_device_unregister(indio_dev);
}

#define ADC_CHANNEL(_index, _id) {	\
	.type = IIO_VOLTAGE,			\
	.indexed = 1,					\
	.channel = _index,				\
	.address = _index,				\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),	\
	.datasheet_name = _id,							\
}

/*
 * According to the GPADC spec,
 * the current GPADC only supports 4 channels.
 */
static const struct iio_chan_spec gpadc_adc_iio_channels[] = {
	ADC_CHANNEL(0, "adc_chan0"),
	ADC_CHANNEL(1, "adc_chan1"),
	ADC_CHANNEL(2, "adc_chan2"),
	ADC_CHANNEL(3, "adc_chan3"),
	/*
	ADC_CHANNEL(4, "adc_chan4"),
	ADC_CHANNEL(5, "adc_chan5"),
	ADC_CHANNEL(6, "adc_chan6"),
	ADC_CHANNEL(7, "adc_chan7"),
	*/
};

/*
 * Register the IIO data structure,
 * the basic data has been initialized in the function: sunxi_gpadc_probe,
 * you can use it directly here: platform_get_drvdata.
 */
static int sunxi_gpadc_iio_init(struct platform_device *pdev)
{
	int ret;
	struct iio_dev *indio_dev;
	struct sunxi_gpadc_iio *info;
	struct sunxi_gpadc *sunxi_gpadc = platform_get_drvdata(pdev);

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*sunxi_gpadc));
	if (!indio_dev)
		return -ENOMEM;

	info = iio_priv(indio_dev);
	info->sunxi_gpadc = sunxi_gpadc;

	indio_dev->dev.parent = &pdev->dev;
	indio_dev->name = pdev->name;
	indio_dev->channels = gpadc_adc_iio_channels;
	indio_dev->num_channels = ARRAY_SIZE(gpadc_adc_iio_channels);
	indio_dev->info = &sunxi_gpadc_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to register iio device\n");
		ret = -EINVAL;
	}
/*
 * Destroy IIO:the kernel will call the callback to execute sunxi_gpadc_remove_iio.
 */
	ret = devm_add_action(&pdev->dev, sunxi_gpadc_remove_iio, indio_dev);

	if (ret) {
		dev_err(&pdev->dev, "unable to add iio cleanup action\n");
		goto err_iio_unregister;
	}

	return 0;

err_iio_unregister:
	iio_device_unregister(indio_dev);

	return ret;
}
#endif  /* IS_ENABLED(CONFIG_IIO) */

static int sunxi_gpadc_dts_parse(struct sunxi_gpadc *chip)
{
	u32 val;
	int i;
	unsigned char name[STR_SIZE];
	struct device_node *np = chip->dev->of_node;
	struct sunxi_gpadc_config *config = &chip->gpadc_config;

	chip->controller_num = of_alias_get_id(np, "gpadc");
	if (chip->controller_num < 0) {
		dev_err(chip->dev, "get alias id failed\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "gpadc_sample_rate", &chip->gpadc_sample_rate)) {
		dev_warn(chip->dev, "warn: sample rate not set\n");
		chip->gpadc_sample_rate = DEFAULT_SR;
	}
	if (chip->gpadc_sample_rate < MIN_SR || chip->gpadc_sample_rate > MAX_SR)
		chip->gpadc_sample_rate = DEFAULT_SR;

	if (of_property_read_u32(np, "channel_num", &chip->channel_num)) {
		dev_err(chip->dev, "get channel num failed\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "channel_select", &config->channel_select)) {
		dev_warn(chip->dev, "warn: channel not select\n");
		config->channel_select = 0;
	}

	config->mode_select = GP_CONTINUOUS_MODE;
	of_property_read_u32(np, "gpadc_mode_select", &config->mode_select);

	if (of_property_read_u32(np, "keyadc-select",
				&config->keyadc_select)) {
		dev_warn(chip->dev, "%s: warn: keyadc not select\n", __func__);
		config->keyadc_select = 0;
	}

	if (of_property_read_u32(np, "channel_data_select", &config->data_select)) {
		dev_dbg(chip->dev, "warn: channel data irq not select\n");
		config->data_select = 0;
	}

	if (of_property_read_u32(np, "channel_cld_select", &config->cld_select)) {
		dev_warn(chip->dev, "warn: channel compare low data irq not select\n");
		config->cld_select = 0;
	}

	if (of_property_read_u32(np, "channel_chd_select", &config->chd_select)) {
		dev_warn(chip->dev, "warn: get channel compare hig data irq not select\n");
		config->chd_select = 0;
	}

	if (of_property_read_u32(np, "scan_data", &config->scan_data)) {
		dev_dbg(chip->dev, "scan data not config\n");
		config->scan_data = 0;
	}

	for (i = 0; i < chip->channel_num; i++) {
		if (BIT(i) & config->cld_select) {
			snprintf(name, sizeof(name), "channel%d_compare_lowdata", i);
			if (of_property_read_u32(np, name, &val)) {
				dev_err(chip->dev, "%s:get %s err!\n", __func__, name);
				val = 0;
			}
			config->cmp_lowdata[i] = val;
		}

		if (BIT(i) & config->chd_select) {
			snprintf(name, sizeof(name), "channel%d_compare_higdata", i);
			if (of_property_read_u32(np, name, &val)) {
				dev_err(chip->dev, "%s:get %s err!\n", __func__, name);
				val = 0;
			}
			config->cmp_highdata[i] = val;
		}
	}

	chip->wakeup_en = 0;
	if (of_property_read_bool(np, "wakeup-source")) {
		device_init_wakeup(chip->dev, true);
		dev_pm_set_wake_irq(chip->dev, chip->irq_num);
		chip->wakeup_en = 1;
	}

	return 0;
}

static int sunxi_gpadc_clk_get(struct sunxi_gpadc *chip)
{
	chip->reset = devm_reset_control_get(chip->dev, NULL);
	if (IS_ERR(chip->reset)) {
		dev_err(chip->dev, "gpadc request GPADC reset failed\n");
		return PTR_ERR(chip->reset);
	}

	chip->bus_clk = devm_clk_get(chip->dev, "bus");
	if (IS_ERR(chip->bus_clk)) {
		dev_err(chip->dev, "gpadc request GPADC clock failed\n");
		return PTR_ERR(chip->bus_clk);
	}

	if (chip->data->has_bus_clk_hosc) {
		chip->bus_clk_hosc = devm_clk_get(chip->dev, "hosc");
		if (IS_ERR(chip->bus_clk_hosc)) {
			dev_err(chip->dev, "gpadc get GPADC 24M clock failed\n");
			return -EINVAL;
		}
	}
	return 0;
}

/* enable clk */
static int sunxi_gpadc_clk_init(struct sunxi_gpadc *chip)
{
	int err;

	err = reset_control_deassert(chip->reset);
	if (err) {
		dev_err(chip->dev, "deassert clk error\n");
		goto err0;
	}

	err = clk_prepare_enable(chip->bus_clk);
	if (err) {
		dev_err(chip->dev, "enable bus_clk failed!\n");
		goto err1;
	}

	if (chip->data->has_bus_clk_hosc) {
		err = clk_prepare_enable(chip->bus_clk_hosc);
		if (err) {
			dev_err(chip->dev, "enable bus_clk_hosc failed!\n");
			goto err1;
		}
	}
	return 0;

err1:
	reset_control_assert(chip->reset);
err0:
	return err;
}

static void sunxi_gpadc_clk_exit(struct sunxi_gpadc *chip)
{
	if (chip->data->has_bus_clk_hosc)
		clk_disable_unprepare(chip->bus_clk_hosc);
	clk_disable_unprepare(chip->bus_clk);
	reset_control_assert(chip->reset);
}

/* get resource */
static int sunxi_gpadc_resource_get(struct sunxi_gpadc *chip)
{
	int err;

	chip->filter_cnt = 5;

	chip->res = platform_get_resource(chip->pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(chip->res)) {
		dev_err(chip->dev, "failed to get IORESOURCE_MEM\n");
		return PTR_ERR(chip->res);
	}

	chip->reg_base = devm_ioremap_resource(chip->dev, chip->res);
	if (IS_ERR(chip->reg_base)) {
		dev_err(chip->dev, "unable to ioremap base_addr\n");
		return PTR_ERR(chip->reg_base);
	}

	err = sunxi_gpadc_dts_parse(chip);
	if (err) {
		dev_err(chip->dev, "failed to parse dts\n");
		return err;
	}

	err = sunxi_gpadc_clk_get(chip);
	if (err) {
		dev_err(chip->dev, "failed to get sunxi_gpadc_clk\n");
		return err;
	}

	chip->irq_num = platform_get_irq(chip->pdev, 0);
	if (chip->irq_num < 0) {
		dev_err(chip->dev, "sunxi_gpadc fail to map irq\n");
		return -EINVAL;
	}

	err = devm_request_irq(chip->dev, chip->irq_num, sunxi_gpadc_irq_handler,
						0, SUNXI_GPADC_DEV_NAME, chip);
	if (err) {
		return err;
	}

	return 0;
}

/* put resource */
static void sunxi_gpadc_resource_put(struct sunxi_gpadc *chip)
{
}

static int sunxi_gpadc_hw_init(struct sunxi_gpadc *chip)
{
	struct sunxi_gpadc_config *config = &chip->gpadc_config;
	int i, err;

	err = sunxi_gpadc_clk_init(chip);
	if (err) {
		dev_err(chip->dev, "gpadc init gpadc clk failed\n");
		return err;
	}

	sunxi_gpadc_sample_rate_set(chip->reg_base, OSC_24MHZ, chip->gpadc_sample_rate);
	for (i = 0; i < chip->channel_num; i++) {
		if (config->channel_select & BIT(i)) {
			/* enable the ith gpadc channel */
			sunxi_gpadc_ch_enable(chip->reg_base, i);

			/* compare data enable */
			if ((config->chd_select | config->cld_select) & BIT(i))
				sunxi_gpadc_cmp_enable(chip->reg_base, i);

			/* compare low data enable */
			if (config->cld_select & BIT(i)) {
				sunxi_gpadc_lowirq_control(chip->reg_base, i, true);
				sunxi_gpadc_ch_cmp_low(chip->reg_base, i,
								config->cmp_lowdata[i]);
			}

			/* compare high data enable */
			if (config->chd_select & BIT(i)) {
				sunxi_gpadc_highirq_control(chip->reg_base, i, true);
				sunxi_gpadc_ch_cmp_high(chip->reg_base, i,
								config->cmp_highdata[i]);
			}
		}
	}

	sunxi_gpadc_calibration_enable(chip->reg_base);
	sunxi_gpadc_set_mode(chip->reg_base, config->mode_select);
	sunxi_gpadc_datairq_control(chip->reg_base, true);
	sunxi_gpadc_enable(chip->reg_base);

	return 0;
}

static void sunxi_gpadc_hw_exit(struct sunxi_gpadc *chip)
{
	struct sunxi_gpadc_config *config = &chip->gpadc_config;
	int i;

	sunxi_gpadc_disable(chip->reg_base);
	sunxi_gpadc_datairq_control(chip->reg_base, false);

	for (i = 0; i < chip->channel_num; i++) {
		if (config->channel_select & BIT(i)) {

			/* compare high data disable */
			if (config->chd_select & BIT(i))
				sunxi_gpadc_highirq_control(chip->reg_base, i, false);

			/* compare low data disable */
			if (config->cld_select & BIT(i))
				sunxi_gpadc_lowirq_control(chip->reg_base, i, false);

			/* compare data disable */
			if ((config->chd_select | config->cld_select) & BIT(i))
				sunxi_gpadc_cmp_disable(chip->reg_base, i);

			/* disable the ith gpadc channel */
			sunxi_gpadc_ch_disable(chip->reg_base, i);
		}
	}

	sunxi_gpadc_clk_exit(chip);
}

static int sunxi_gpadc_key_init(struct sunxi_gpadc *chip)
{
	struct device_node *child, *np = chip->dev->of_node;
	struct sunxi_gpadc_config *config = &chip->gpadc_config;
	unsigned char i, j;
	u32 vol, val;
	u32 set_vol[VOL_NUM];
	int chan = 0;

	for_each_child_of_node(np, child) {
		if (config->keyadc_select & BIT(chan)) {
			j = 0;
			if (of_property_read_u32(child, "key_cnt", &chip->key_num[chan])) {
				dev_err(chip->dev, "%s: get key count failed", __func__);
				return -EINVAL;
			}
			dev_dbg(chip->dev, "%s key number = %d.\n", __func__, chip->key_num[chan]);
			if (chip->key_num[chan] < 1 || chip->key_num[chan] > VOL_NUM) {
				dev_err(chip->dev, "chan%d incorrect key number\n", chan);
				return -EINVAL;
			}
			for (i = 0; i < chip->key_num[chan]; i++) {
				snprintf(chip->key_name[chan], sizeof(chip->key_name[chan]), "key%d_vol", i);
				if (of_property_read_u32(child, chip->key_name[chan], &vol)) {
					dev_err(chip->dev, "%s:get%s err!\n", __func__,
							chip->key_name[chan]);
					return -EINVAL;
				}
				chip->key_vol[chan][i] = vol;

				snprintf(chip->key_name[chan], sizeof(chip->key_name[chan]), "key%d_val", i);
				if (of_property_read_u32(child, chip->key_name[chan], &val)) {
					dev_err(chip->dev, "%s:get%s err!\n", __func__,
							chip->key_name[chan]);
					return -EINVAL;
				}
				chip->scankeycodes[chan][i] = val;
				dev_dbg(chip->dev, "%s: key%d vol= %d code= %d\n", __func__, i,
									chip->key_vol[chan][i],
									chip->scankeycodes[chan][i]);
			}
			chip->key_vol[chan][i] = MAX_INPUT_VOLTAGE;

			for (i = 0; i < chip->key_num[chan]; i++)
				set_vol[i] = chip->key_vol[chan][i]
						+ (chip->key_vol[chan][i+1]
						- chip->key_vol[chan][i]) / 2;

			for (i = 0; i < 128; i++) {
				if (i * SCALE_UNIT > set_vol[j])
					j++;
				chip->keypad_mapindex[chan][i] = j;
			}
		}
		chan++;
	};

	return 0;
}

static void sunxi_gpadc_set_usage(struct sunxi_gpadc *chip, u32 id)
{
	int i;
	struct sunxi_gpadc_config *config = &chip->gpadc_config;

	if (config->keyadc_select & BIT(id)) {
		/* if gapdc is used as keyadc, the key event is set */
		chip->input_gpadc[id]->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

		for (i = 0; i < KEY_MAX_CNT; i++)
			set_bit(chip->scankeycodes[id][i], chip->input_gpadc[id]->keybit);
	} else {
		chip->input_gpadc[id]->evbit[0] = BIT_MASK(EV_MSC);
		chip->input_gpadc[id]->mscbit[0] = BIT_MASK(MSC_SCAN);
		set_bit(EV_MSC, chip->input_gpadc[id]->evbit);
		set_bit(MSC_SCAN, chip->input_gpadc[id]->mscbit);
	}
}

static int sunxi_gpadc_inputdev_do_register(struct sunxi_gpadc *chip, int id)
{
	struct input_dev *input_dev = NULL;
	int err;
	char *name;

	input_dev = devm_input_allocate_device(chip->dev);
	if (!input_dev) {
		dev_err(chip->dev, "input_dev: not enough memory for input device\n");
		return -ENOMEM;
	}

	name = devm_kzalloc(chip->dev, STR_SIZE, GFP_KERNEL);
	snprintf(name, STR_SIZE, "sunxi-gpadc%d/channel%d", chip->controller_num, id);

	input_dev->name = name;
	input_dev->phys = strcat(name, "/input0");

	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;

	chip->input_gpadc[id] = input_dev;
	sunxi_gpadc_set_usage(chip, id);
	err = input_register_device(chip->input_gpadc[id]);
	if (err) {
		dev_err(chip->dev, "input register fail\n");
		return err;
	}

	return 0;
}

static int sunxi_gpadc_inputdev_register(struct sunxi_gpadc *chip)
{
	struct sunxi_gpadc_config *config = &chip->gpadc_config;
	int i, err;

	for (i = 0; i < chip->channel_num; i++) {
		if (config->channel_select & BIT(i)) {
			err = sunxi_gpadc_inputdev_do_register(chip, i);
			if (err) {
				dev_err(chip->dev, "failed to register gpadc channel\n");
				goto err0;
			}
		}
	}

	return 0;
err0:
	while (i--)
		input_unregister_device(chip->input_gpadc[i]);
	return err;
}

static void sunxi_gpadc_inputdev_unregister(struct sunxi_gpadc *chip)
{
	struct sunxi_gpadc_config *config = &chip->gpadc_config;
	int i;

	for (i = 0; i < chip->channel_num; i++) {
		if (config->channel_select & BIT(i))
			input_unregister_device(chip->input_gpadc[i]);
	}
}

static int sunxi_gpadc_sysfs_create(struct sunxi_gpadc *chip)
{
	int i;
	int err;
	char name[STR_SIZE];

	snprintf(name, sizeof(name), "gpadc_chip%u", chip->controller_num);
	chip->class_dev = device_create(&gpadc_class, chip->dev, chip->controller_num, chip, "%s", name);

	if (IS_ERR(chip->class_dev))
		return PTR_ERR(chip->class_dev);

	for (i = 0; i < ARRAY_SIZE(gpadc_class_attrs); i++) {
		err = device_create_file(chip->class_dev, &gpadc_class_attrs[i]);
		if (err) {
			dev_err(chip->dev, "device_create_file() failed\n");
			while (i--)
				device_remove_file(chip->class_dev, &gpadc_class_attrs[i]);

			device_destroy(&gpadc_class, chip->controller_num);
			return err;
		}
	}

	return 0;
}

static void sunxi_gpadc_sysfs_destroy(struct sunxi_gpadc *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(gpadc_class_attrs); i++)
		device_remove_file(chip->class_dev, &gpadc_class_attrs[i]);

	device_destroy(&gpadc_class, chip->controller_num);
}

static int sunxi_gpadc_probe(struct platform_device *pdev)
{
	int err = 0;
	struct sunxi_gpadc *chip;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (IS_ERR_OR_NULL(chip)) {
		err = -ENOMEM;
		goto err0;
	}

	chip->pdev = pdev;
	chip->dev = &pdev->dev;

	chip->data = of_device_get_match_data(&pdev->dev);
	if (!chip->data) {
		dev_err(chip->dev, "failed to get device data\n");
		goto err0;
	}

	err = sunxi_gpadc_resource_get(chip);
	if (err) {
		dev_err(chip->dev, "failed to resource_get\n");
		goto err0;
	}

	global_gpadc[chip->controller_num] = *chip;

	if (chip->gpadc_config.keyadc_select) {
		err = sunxi_gpadc_key_init(chip);
		if (err) {
			dev_err(chip->dev, "failed to key_init\n");
			goto err0;
		}
	}

	err = sunxi_gpadc_hw_init(chip);
	if (err) {
		dev_err(chip->dev, "failed to hw_init\n");
		goto err1;
	}

	err = sunxi_gpadc_inputdev_register(chip);
	if (err) {
		dev_err(chip->dev, "failed to input_register\n");
		goto err2;
	}

	err = sunxi_gpadc_sysfs_create(chip);
	if (err) {
		dev_err(chip->dev, "failed to node_init\n");
		goto err3;
	}

	platform_set_drvdata(pdev, chip);

#if IS_ENABLED(CONFIG_IIO)
	sunxi_gpadc_iio_init(pdev);
#endif

	dev_info(chip->dev, "sunxi_gpadc probe success\n");

	return 0;

err3:
	sunxi_gpadc_inputdev_unregister(chip);
err2:
	sunxi_gpadc_hw_exit(chip);
err1:
	sunxi_gpadc_resource_put(chip);
err0:

	return err;
}

static int sunxi_gpadc_remove(struct platform_device *pdev)
{
	struct sunxi_gpadc *chip = platform_get_drvdata(pdev);

	sunxi_gpadc_sysfs_destroy(chip);
	sunxi_gpadc_inputdev_unregister(chip);
	sunxi_gpadc_hw_exit(chip);
	sunxi_gpadc_resource_put(chip);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static inline void sunxi_gpadc_save_regs(struct sunxi_gpadc *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_gpadc_regs_offset); i++)
		chip->regs_backup[i] = readl(chip->reg_base + sunxi_gpadc_regs_offset[i]);
}

static inline void sunxi_gpadc_restore_regs(struct sunxi_gpadc *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_gpadc_regs_offset); i++)
		writel(chip->regs_backup[i], chip->reg_base + sunxi_gpadc_regs_offset[i]);
}

static int sunxi_gpadc_suspend(struct device *dev)
{
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);

	if (chip->wakeup_en) {
		sunxi_gpadc_save_regs(chip);
	} else {
		disable_irq_nosync(chip->irq_num);
		sunxi_gpadc_save_regs(chip);
		sunxi_gpadc_clk_exit(chip);
	}

	dev_info(chip->dev, "sunxi gpadc suspend\n");
	return 0;
}

static int sunxi_gpadc_resume(struct device *dev)
{
	struct sunxi_gpadc *chip = dev_get_drvdata(dev);

	if (chip->wakeup_en) {
		sunxi_gpadc_restore_regs(chip);
	} else {
		sunxi_gpadc_clk_init(chip);
		sunxi_gpadc_restore_regs(chip);
		enable_irq(chip->irq_num);
	}

	dev_info(chip->dev, "sunxi gpadc resume\n");
	return 0;
}

static const struct dev_pm_ops sunxi_gpadc_dev_pm_ops = {
	.suspend = sunxi_gpadc_suspend,
	.resume = sunxi_gpadc_resume,
};

#define SUNXI_GPADC_DEV_PM_OPS (&sunxi_gpadc_dev_pm_ops)
#else
#define SUNXI_GPADC_DEV_PM_OPS NULL
#endif  /* IS_ENABLED(CONFIG_PM) */

static struct sunxi_gpadc_hw_data sunxi_gpadc_v100_data = {
	.has_bus_clk_hosc = false,
};

static struct sunxi_gpadc_hw_data sunxi_gpadc_v101_data = {
	.has_bus_clk_hosc = true,
};

static const struct of_device_id sunxi_gpadc_of_match[] = {
	/* compatible for old name format */
	{ .compatible = "allwinner,sunxi-gpadc", .data = &sunxi_gpadc_v100_data },
	{ .compatible = "allwinner,sunxi-gpadc-v101", .data = &sunxi_gpadc_v101_data },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_gpadc_of_match);

static struct platform_driver sunxi_gpadc_driver = {
	.probe  = sunxi_gpadc_probe,
	.remove = sunxi_gpadc_remove,
	.driver = {
		.name   = SUNXI_GPADC_DEV_NAME,
		.owner  = THIS_MODULE,
		.pm = SUNXI_GPADC_DEV_PM_OPS,
		.of_match_table = of_match_ptr(sunxi_gpadc_of_match),
	},
};

static int __init sunxi_gpadc_init(void)
{
	int ret;

	/* create dir: /sys/class/gpadc */
	ret = class_register(&gpadc_class);
	if (ret < 0)
		pr_err("%s(): %d: gpadc class register err, ret:%d\n", __func__, __LINE__, ret);
	else
		pr_info("%s(): %d: gpadc class register success\n", __func__, __LINE__);

	ret = platform_driver_register(&sunxi_gpadc_driver);
	if (ret)
		class_unregister(&gpadc_class);

	return ret;
}

static void __exit sunxi_gpadc_exit(void)
{
	platform_driver_unregister(&sunxi_gpadc_driver);
	class_unregister(&gpadc_class);
}

module_init(sunxi_gpadc_init);
module_exit(sunxi_gpadc_exit);

MODULE_AUTHOR("Fuzhaoke <fuzhaoke@allwinnertech.com>");
MODULE_AUTHOR("shaosidi <shaosidi@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi gpadc driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("2.1.0");

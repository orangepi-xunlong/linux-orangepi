/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner devfreq driver.
 *
 * Copyright (C) 2019 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * Supplied sunxi ddr devfreq.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <sunxi-log.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/devfreq-event.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include <linux/version.h>
#include <linux/cpufreq.h>
#include <../drivers/devfreq/governor.h>
//#include "../crashdump/sunxi-crashdump.h"

#if IS_ENABLED(CONFIG_ARCH_SUN65IW1)
#define PMU_PER_REG			0x102C
#define PMU_CFG_REG			0x1030
#else
#define PMU_PER_REG			0x1030
#define PMU_CFG_REG			0x1034
#endif

#define PMU_FC_EN_MASK		(0xff << 24)
#define PMU_IRQ_RATIO_MASK	(0x3 << 18)
#define PMU_IRQ_MODE_MASK	(0x3 << 16)
#define PMU_IRQ_MASK 		BIT(12)
#define PMU_MODE_MASK		(0xf << 8)
#define PMU_CLK_SEL_MASK	BIT(5)
#define PMU_PER1_MASK		0x7

#define PMU_FC_EN_VAL		(0xff << 24)
#define PMU_IRQ_RATIO_VAL	(0x1 << 18)
#define PMU_IRQ_MODE_VAL	(0x1 << 16)
#define PMU_IRQ_VAL 		BIT(12)
#define PMU_MODE_VAL		(0x2 << 8)
#define PMU_CLK_SEL_VAL		(0x0 << 5)
#define PMU_PER1_VAL		0x1

#define PMU_FREQ0_HIGH_REG	0x1040
#define PMU_FREQ0_LOW_REG	0x1044
#define PMU_FREQ1_HIGH_REG	0x1048
#define PMU_FREQ1_LOW_REG	0x104C
#define PMU_FREQ2_HIGH_REG	0x1050
#define PMU_FREQ2_LOW_REG	0x1054
#define PMU_FREQ3_HIGH_REG	0x1058
#define PMU_FREQ3_LOW_REG	0x105C

#define PMU_SOFT_CTRL_REG	0x1060
#define PMU_CLR				BIT(1)
#define PMU_RESETN			BIT(0)

#define PMU_EN_REG			0x1064
#define PMU_EN				BIT(0)

#define PMU_IRQ_EN_REG		0x1068
#define PMU_IRQ_EN			0x3
#define PMU_UP_IRQ_EN		BIT(1)
#define PMU_DN_IRQ_EN		BIT(0)

#define PMU_IRQ_PEND_REG	0x106C
#define PMU_UP_IRQ_PEND		BIT(1)
#define PMU_DN_IRQ_PEND     BIT(0)

#define PMU_REQ_R_REG		0x1088
#define PMU_REQ_W_REG		0x108C
#define PMU_REQ_RW_REG		0x1090

#define MASTER_REG0			0x10000
#define DDR_TYPE_LPDDR4		BIT(5)

#define SUNXI_IAPE	0xC0
#define SUNXI_IAPC	0xC4
#define SUNXI_IAPP	0xC8
#define SUNXI_IADR	0xD4
#define SUNXI_IADW	0xD8

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
#define MC_SRAM_ECC			0x30
#define MC_ECC_INJ_DATA(n)	(0x30 + (n) * 4)
#define MC_ECC_INJ_STA(n)	(0x44 + (n) * 4)
#define MC_ECC_ORI_DATA(n)	(0x80 + (n) * 4)
#define MX_SWCTL			0x10320
#define MX_ECCSTAT			0x10078
#define MX_ECCCLR			0x1007C
#endif

#define SECOND		1000	/* 1ms(const) */
#define DFSO_UPTHRESHOLD	(90)
#define DFSO_DOWNDIFFERENCTIAL	(5)
#define LOWFREQ_UP_COMP			(20)
#define DISP_ON_DOWN_THRESHOLD (50000000UL / SECOND)
#define LOWFREQ_THRESHOLD	(300) /*unit:MBytes*/
#define DEVFREQ_EN BIT(2)
#define DRIVER_NAME	"devfreq Driver"

static inline void clrsetbitsl(volatile void __iomem *addr, u32 mask, u32 set)
{
	writel((readl(addr) & ~mask) | (set & mask), addr);
}

struct freq_t {
	unsigned long freq;
	unsigned long freq_upthreshold;
	unsigned long freq_downthreshold;
};

struct sunxi_dmcfreq_plat_data {
	unsigned int de_nsi_masterid;
	unsigned int dfi_misc_reg;
};

struct sunxi_dmcfreq {
	struct device *dev;
	void __iomem *common_base;
	void __iomem *base;
	void __iomem *con_base;
	struct devfreq *devfreq;
	struct devfreq_simple_ondemand_data ondemand_data;
	struct clk *dmc_clk;
	struct clk *bus_clk;
	struct mutex lock;
	const struct sunxi_dmcfreq_plat_data *plat_data;
	unsigned int dram_type;
	unsigned int dram_div;
	unsigned int down_num;
	unsigned long rw_data;
	unsigned long de_rw_data;
	unsigned int normalvoltage, boostvoltage;
	int irq;
	int inlinecc_irq;
	int sramecc_irq;
	unsigned long rate;
#if IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	struct clk *ddrpll0_clk;
	struct clk *ddrpll1_clk;
	struct clk *ddrpll2_clk;
	unsigned long ddrpll[3];
#endif
	struct freq_t freq[8];
};

static int dbg_enable;
static int pooling_ms = 1;
static int down_threshold = 25 * (SECOND / 1000);
module_param_named(dbg_level, dbg_enable, int, 0644);

#define DBG(args...) \
	do { \
		if (dbg_enable) { \
			sunxi_info(NULL, args); \
		} \
	} while (0)

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
static void sunxi_start_nsi_de_counter(struct sunxi_dmcfreq *dmcfreq) {}
static void sunxi_stop_nsi_de_counter(struct sunxi_dmcfreq *dmcfreq) {}
u32 sunxi_read_nsi_de_counter(struct sunxi_dmcfreq *dmcfreq) { return 0; }
#else
static void sunxi_start_nsi_de_counter(struct sunxi_dmcfreq *dmcfreq)
{
	unsigned int val;
	const struct sunxi_dmcfreq_plat_data *plat_data = dmcfreq->plat_data;
	unsigned int de_id = plat_data->de_nsi_masterid;

	/* Automatically updated every 100ms */
	val = (clk_get_rate(dmcfreq->bus_clk) / SECOND) * pooling_ms;
	writel(val, dmcfreq->base + SUNXI_IAPP + (de_id * 0x200));
	val = readl(dmcfreq->base + SUNXI_IAPE + (de_id * 0x200));
	val |= BIT(0);
	writel(val, dmcfreq->base + SUNXI_IAPE + (de_id * 0x200));
}

static void sunxi_stop_nsi_de_counter(struct sunxi_dmcfreq *dmcfreq)
{
	u32 val;
	const struct sunxi_dmcfreq_plat_data *plat_data = dmcfreq->plat_data;
	unsigned int de_id = plat_data->de_nsi_masterid;

	val = readl(dmcfreq->base + SUNXI_IAPE + (de_id * 0x200));
	val &= ~BIT(0);
	writel(val, dmcfreq->base + SUNXI_IAPE + (de_id * 0x200));

	val = readl(dmcfreq->base + SUNXI_IAPC + (de_id * 0x200));
	val |= BIT(0);
	writel(val, dmcfreq->base + SUNXI_IAPC + (de_id * 0x200));

	udelay(1);

	val = readl(dmcfreq->base + SUNXI_IAPC + (de_id * 0x200));
	val &= ~BIT(0);
	writel(val, dmcfreq->base + SUNXI_IAPC + (de_id * 0x200));
}

u32 sunxi_read_nsi_de_counter(struct sunxi_dmcfreq *dmcfreq)
{
	u32 read_data, write_data;
	const struct sunxi_dmcfreq_plat_data *plat_data = dmcfreq->plat_data;
	unsigned int de_id = plat_data->de_nsi_masterid;

	read_data = readl(dmcfreq->base + SUNXI_IADR + (de_id * 0x200));
	write_data = readl(dmcfreq->base + SUNXI_IADW + (de_id * 0x200));
	return read_data + write_data;
}
#endif

static void sunxi_ddrpmu_init(struct sunxi_dmcfreq *dmcfreq)
{
	clrsetbitsl(dmcfreq->common_base + PMU_CFG_REG,
	(unsigned int)(PMU_FC_EN_MASK | PMU_IRQ_RATIO_MASK	| PMU_IRQ_MODE_MASK	|
	PMU_IRQ_MASK | PMU_MODE_MASK | PMU_CLK_SEL_MASK | PMU_PER1_MASK),
	(unsigned int)(PMU_FC_EN_VAL | PMU_IRQ_RATIO_VAL | PMU_IRQ_MODE_VAL	|
	PMU_IRQ_VAL | PMU_MODE_VAL | PMU_CLK_SEL_VAL | PMU_PER1_VAL));
	clrsetbitsl(dmcfreq->common_base + PMU_SOFT_CTRL_REG, PMU_RESETN, -1U);
}

static void sunxi_ddrpmu_start_hardware_counter(struct sunxi_dmcfreq *dmcfreq)
{
	unsigned int val;
	/* Automatically updated every 1ms */
	val = (200000000UL / SECOND) * pooling_ms;
	writel(val, dmcfreq->common_base + PMU_PER_REG);
	clrsetbitsl(dmcfreq->common_base + PMU_EN_REG, PMU_EN, -1U);
	clrsetbitsl(dmcfreq->common_base + PMU_IRQ_EN_REG, PMU_IRQ_EN, -1U);
}

static void sunxi_ddrpmu_stop_hardware_counter(struct sunxi_dmcfreq *dmcfreq)
{
	clrsetbitsl(dmcfreq->common_base + PMU_IRQ_EN_REG, PMU_IRQ_EN, 0);
	clrsetbitsl(dmcfreq->common_base + PMU_EN_REG, PMU_EN, 0);
	clrsetbitsl(dmcfreq->common_base + PMU_SOFT_CTRL_REG, PMU_CLR, -1U);
	udelay(1);
	clrsetbitsl(dmcfreq->common_base + PMU_SOFT_CTRL_REG, PMU_CLR, 0);
}

static void sunxi_adjust_monitor_threshold(struct sunxi_dmcfreq *dmcfreq)
{
	unsigned long freq = dmcfreq->rate;
	const struct sunxi_dmcfreq_plat_data *plat_data = dmcfreq->plat_data;
	unsigned int i;
	unsigned int dfi_misc;

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	dfi_misc = readl(dmcfreq->con_base + plat_data->dfi_misc_reg);
#else
	dfi_misc = readl(dmcfreq->common_base + plat_data->dfi_misc_reg);
#endif

	dfi_misc = (dfi_misc >> 8) & 0x3;
	for (i = 0; i < 8; i++) {
		if (dmcfreq->freq[i].freq / 1000000 == freq / 1000000) {
			writel(dmcfreq->freq[i].freq_upthreshold,
				dmcfreq->common_base + PMU_FREQ0_HIGH_REG + (dfi_misc * 8));
			writel(dmcfreq->freq[i].freq_downthreshold,
				dmcfreq->common_base + PMU_FREQ0_LOW_REG + (dfi_misc * 8));
		}
	}
}

static irqreturn_t sunximon_thread_isr(int irq, void *data)
{
	struct sunxi_dmcfreq *dmcfreq = data;
	unsigned int irq_pending;
	unsigned int rw_data;

	if (IS_ERR_OR_NULL(dmcfreq->devfreq))
		return IRQ_NONE;

	mutex_lock(&dmcfreq->devfreq->lock);
	clrsetbitsl(dmcfreq->common_base + PMU_EN_REG, PMU_EN, 0);
	irq_pending = readl(dmcfreq->common_base + PMU_IRQ_PEND_REG);
	writel(irq_pending, dmcfreq->common_base + PMU_IRQ_PEND_REG);
	rw_data = readl(dmcfreq->common_base + PMU_REQ_RW_REG);
	if (rw_data <= 100)
		goto handled;

	if (irq_pending & PMU_DN_IRQ_PEND) {
//		dmcfreq->rw_data =
//			dmcfreq->down_num == 0 ? rw_data : (dmcfreq->rw_data + rw_data) >> 1;
		dmcfreq->rw_data += rw_data;
		dmcfreq->de_rw_data += sunxi_read_nsi_de_counter(dmcfreq);
		dmcfreq->down_num++;
		if (dmcfreq->down_num < down_threshold)
			goto handled;
		else {
			dmcfreq->rw_data /= down_threshold;
			dmcfreq->de_rw_data /= down_threshold;
		}
	} else
		if ((dmcfreq->freq[0].freq / 1000000 == dmcfreq->rate / 1000000)
				&& (dmcfreq->freq[0].freq / 1000000 <= LOWFREQ_THRESHOLD))
			dmcfreq->rw_data = (rw_data * 3) >> 1;
		else
			dmcfreq->rw_data = rw_data;
	update_devfreq(dmcfreq->devfreq);
	dmcfreq->down_num = 0;
	dmcfreq->rw_data = 0;
	dmcfreq->de_rw_data = 0;

handled:
	clrsetbitsl(dmcfreq->common_base + PMU_EN_REG, PMU_EN, -1U);
	mutex_unlock(&dmcfreq->devfreq->lock);
	return IRQ_HANDLED;
}

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
static irqreturn_t sunxi_inlinecc_isr(int irq, void *data)
{
	struct sunxi_dmcfreq *dmcfreq = data;
	struct device *dev = dmcfreq->dev;
	unsigned int reg_val = 0, ecc_uncorrected_err, ecc_corrected_err, ecc_corrected_bit;

	writel(0, dmcfreq->common_base + MX_SWCTL);
	reg_val = readl(dmcfreq->common_base + MX_ECCSTAT);
	sunxi_err(dev, "MX_ECCSTAT:0x%x\n", reg_val);

	ecc_uncorrected_err = (reg_val >> 16) & 0xffff;
	ecc_corrected_err	= (reg_val >> 8) & 0x00ff;
	ecc_corrected_bit	= (reg_val >> 0) & 0x007f;

	/* lock	0;unlock 1;*/
	clrsetbitsl(dmcfreq->common_base + MX_ECCCLR, (0x3 << 8), 0);
	clrsetbitsl(dmcfreq->common_base + MX_ECCCLR, 0x1f, -1U);

	writel(1, dmcfreq->common_base + MX_SWCTL);

	if (ecc_uncorrected_err)
		sunxi_err(dev, "inline_ecc_uncorrected_err:%d\n", ecc_uncorrected_err);

	if (ecc_corrected_err) {
		sunxi_err(dev, "inline_ecc_corrected_err:%d\r\n", ecc_corrected_err);
		sunxi_err(dev, "ecc_corrected_bit:%d\n", ecc_corrected_bit);
	}

	writel(0, dmcfreq->common_base + MX_SWCTL);
	clrsetbitsl(dmcfreq->common_base + MX_ECCCLR, (0x3 << 8), -1U);
	writel(1, dmcfreq->common_base + MX_SWCTL);

	return IRQ_HANDLED;
}

static irqreturn_t sunxi_sramecc_isr(int irq, void *data)
{
	struct sunxi_dmcfreq *dmcfreq = data;
	struct device *dev = dmcfreq->dev;
	unsigned int reg_val = 0, i = 0;

	/* close irq enable */
	clrsetbitsl(dmcfreq->common_base + MC_SRAM_ECC, (0x1 << 2), 0);

	for (i = 1; i <= 4; i++) {
		reg_val = readl(dmcfreq->common_base + MC_ECC_INJ_DATA(i));
		sunxi_err(dev, "MC_ECC_INJ_DATA%d:0x%x\n", i, reg_val);
	}

	for (i = 0; i < 16; i++) {
		reg_val = readl(dmcfreq->common_base + MC_ECC_INJ_STA(i));
		sunxi_err(dev, "MC_ECC_INJ_STA%d:0x%x\n", i, reg_val);
	}

	for (i = 1; i <= 4; i++) {
		reg_val = readl(dmcfreq->common_base + MC_ECC_ORI_DATA(i));
		sunxi_err(dev, "MC_ECC_ORI_DATA%d:0x%x\n", i, reg_val);
	}
	/* clear ecc status */
	clrsetbitsl(dmcfreq->common_base + MC_SRAM_ECC, (0x1 << 0), -1U);
	clrsetbitsl(dmcfreq->common_base + MC_SRAM_ECC, (0x1 << 0), 0);
	clrsetbitsl(dmcfreq->common_base + MC_SRAM_ECC, (0x1 << 2), -1U);

	return IRQ_HANDLED;
}
#endif

static int sunxi_dmc_target(struct device *dev,
						unsigned long *freq, u32 flags)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	unsigned long target_rate;
//	unsigned long last_rate_tmp;
	struct dev_pm_opp *opp;
	int rc = 0;
	unsigned int timeout;
//	u64 start_time;
//	u64 end_time;

	DBG("req_freq:%ld, flags:%d\n", *freq, flags);
	if ((dmcfreq->freq[0].freq / 1000000 == *freq / 1000000) &&
		(dmcfreq->de_rw_data >= 100) && (dmcfreq->freq[0].freq / 1000000 <= LOWFREQ_THRESHOLD))
		*freq += 1000000;
	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp))
		return PTR_ERR(opp);

	target_rate = dev_pm_opp_get_freq(opp);
	dmcfreq->rate = clk_get_rate(dmcfreq->dmc_clk);
	DBG("target_rate:%ld, cur_rate:%ld\n", target_rate, dmcfreq->rate);
	if (dmcfreq->rate == target_rate)
		return 0;

//	last_rate_tmp = dmcfreq->rate;
//	start_time = ktime_get();
	/* start frequency scaling */
	mutex_lock(&dmcfreq->lock);
//	sunxi_ddrpmu_stop_hardware_counter(dmcfreq);
//	rc = clk_set_rate(dmcfreq->dmc_clk, target_rate);
	rc = dev_pm_opp_set_opp(dev, opp);
	if (rc)
		goto out;

	timeout = 200;
	usleep_range(100, 300);
	while ((dmcfreq->rate = clk_get_rate(dmcfreq->dmc_clk)) / 1000000 != target_rate / 1000000) {
		if (!timeout) {
			sunxi_err(dev, "change dram clock error!,target_rate:%ld, cur_rate:%ld, req_rate:%ld\n",
					target_rate, dmcfreq->rate, *freq);
			rc = -ETIMEDOUT;
			goto out;
		}

		timeout--;
		cpu_relax();
		msleep(2);
	}

out:
	sunxi_adjust_monitor_threshold(dmcfreq);
//	sunxi_ddrpmu_start_hardware_counter(dmcfreq);
//	end_time = ktime_get();
//	sunxi_get_freq_info(dev, NULL, start_time, end_time, last_rate_tmp, target_rate);
	mutex_unlock(&dmcfreq->lock);
	return rc;
}

static int sunxi_get_event(struct sunxi_dmcfreq *dmcfreq,
				  struct devfreq_event_data *edata)
{
	unsigned long rw_data = dmcfreq->rw_data, ddr_type = dmcfreq->dram_type &= DDR_TYPE_LPDDR4;

	/*
	 * read/write: In byte
	 * Max Utilization
	 *
	 * load = (read + write) / (dram_clk * 2 * 4)
	 */
	if (ddr_type)
		edata->load_count = (unsigned long)rw_data << 6;
	else
		edata->load_count = (unsigned long)rw_data << 5;
	edata->total_count = (clk_get_rate(dmcfreq->dmc_clk) / SECOND) * 8 * pooling_ms;

	DBG("rate:%ldM load:%ld rw:%ldM total:%ldM\n",
				clk_get_rate(dmcfreq->dmc_clk) / 1000000,
				(edata->load_count >> 7) * 100 / (edata->total_count >> 7),
				(edata->load_count * SECOND) / pooling_ms / 1000000,
				(edata->total_count * SECOND) / pooling_ms / 1000000);
	DBG("rw_data:%ld, load_count:%ld, total_count:%ld\n",
		rw_data, edata->load_count, edata->total_count);

	return 0;
}

static int sunxi_get_dev_status(struct device *dev,
				struct devfreq_dev_status *stat)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	int ret = 0;

	struct devfreq_event_data edata;
	ret = sunxi_get_event(dmcfreq, &edata);
	if (ret < 0)
		return ret;
	/* To be used by the sunxi governor */
	stat->private_data = dmcfreq;

	stat->current_frequency = clk_get_rate(dmcfreq->dmc_clk);
	stat->busy_time = edata.load_count;
	stat->total_time = edata.total_count;
	DBG("busy_time:%ld, total_time:%ld, cur_freq:%ld\n",
		stat->busy_time, stat->total_time, stat->current_frequency);

	if (stat->busy_time > stat->total_time)
		stat->busy_time = stat->total_time;

	return ret;
}

static int sunxi_dmcfreq_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);

	*freq = clk_get_rate(dmcfreq->dmc_clk);

	return 0;
}
#if IS_ENABLED(CONFIG_ARCH_SUN65IW1)
static void sunxi_init_freqlist(struct sunxi_dmcfreq *dmcfreq)
{
	struct device *dev = dmcfreq->dev;
	unsigned int dram_div = dmcfreq->dram_div;
	unsigned int upthreshold = dmcfreq->ondemand_data.upthreshold;
	unsigned int downthreshold =
			upthreshold - dmcfreq->ondemand_data.downdifferential;
	unsigned int i;
	unsigned int sdiv;

	sdiv = dram_div >> 24;
	dmcfreq->freq[0].freq = (dmcfreq->ddrpll[(sdiv >> 3) & 0x3] >> 1) / ((sdiv & 0x7) + 1);
	sdiv = dram_div >> 16;
	dmcfreq->freq[1].freq = (dmcfreq->ddrpll[(sdiv >> 3) & 0x3] >> 1) / ((sdiv & 0x7) + 1);
	sdiv = dram_div >> 8;
	dmcfreq->freq[2].freq = (dmcfreq->ddrpll[(sdiv >> 3) & 0x3] >> 1) / ((sdiv & 0x7) + 1);
	sdiv = dram_div >> 0;
	dmcfreq->freq[3].freq = (dmcfreq->ddrpll[(sdiv >> 3) & 0x3] >> 1) / ((sdiv & 0x7) + 1);

	dmcfreq->freq[0].freq_downthreshold = 0;
	dmcfreq->freq[0].freq_upthreshold =
			((dmcfreq->freq[0].freq / (100 * SECOND)) * (upthreshold - LOWFREQ_UP_COMP) * pooling_ms) >> 2;
	if (dmcfreq->freq[0].freq / 1000000 <= LOWFREQ_THRESHOLD) {
		dmcfreq->freq[1].freq_downthreshold = DISP_ON_DOWN_THRESHOLD >> 5;
	} else {
		dmcfreq->freq[1].freq_downthreshold =
			((dmcfreq->freq[1].freq / (100 * SECOND)) * downthreshold * pooling_ms) >> 2;
	}
	dmcfreq->freq[1].freq_upthreshold =
		((dmcfreq->freq[1].freq / (100 * SECOND)) * upthreshold * pooling_ms) >> 2;
	dmcfreq->freq[2].freq_upthreshold =
		((dmcfreq->freq[2].freq / (100 * SECOND)) * upthreshold * pooling_ms) >> 2;
	dmcfreq->freq[2].freq_downthreshold =
		((dmcfreq->freq[2].freq / (100 * SECOND)) * downthreshold * pooling_ms) >> 2;
	dmcfreq->freq[3].freq_upthreshold = 0xffffffff;
	dmcfreq->freq[3].freq_downthreshold =
		((dmcfreq->freq[3].freq / (100 * SECOND)) * downthreshold * pooling_ms) >> 2;
	for (i = 0; i < 4; i++)
		dev_pm_opp_add(dev, dmcfreq->freq[i].freq, dmcfreq->normalvoltage);
	sunxi_info(NULL, "F0:%ld, F1:%ld, F2:%ld, F3:%ld, up:%d, down:%d\n",
		dmcfreq->freq[0].freq, dmcfreq->freq[1].freq, dmcfreq->freq[2].freq,
			dmcfreq->freq[3].freq, upthreshold, downthreshold);
}
#else
static void sunxi_init_freqlist(struct sunxi_dmcfreq *dmcfreq)
{
	struct device *dev = dmcfreq->dev;
	unsigned long freq = dmcfreq->rate;
	unsigned int dram_div = dmcfreq->dram_div;
	unsigned int upthreshold = dmcfreq->ondemand_data.upthreshold;
	unsigned int downthreshold =
			upthreshold - dmcfreq->ondemand_data.downdifferential;
	unsigned int i;

	if ((dram_div & 0x1f) == 0x3) {
		dmcfreq->freq[0].freq = (freq << 2) / (((dram_div >> 24) & 0x1f) + 1);
		dmcfreq->freq[1].freq = (freq << 2) / (((dram_div >> 16) & 0x1f) + 1);
		dmcfreq->freq[2].freq = (freq << 2) / (((dram_div >> 8) & 0x1f) + 1);
		dmcfreq->freq[3].freq = (freq << 2) / ((dram_div & 0x1f) + 1);
		dmcfreq->freq[0].freq_downthreshold = 0;
		dmcfreq->freq[0].freq_upthreshold =
				((dmcfreq->freq[0].freq / (100 * SECOND)) * (upthreshold - LOWFREQ_UP_COMP) * pooling_ms) >> 2;
		if (dmcfreq->freq[0].freq / 1000000 <= LOWFREQ_THRESHOLD) {
			dmcfreq->freq[1].freq_downthreshold = DISP_ON_DOWN_THRESHOLD >> 5;
		} else {
			dmcfreq->freq[1].freq_downthreshold =
				((dmcfreq->freq[1].freq / (100 * SECOND)) * downthreshold * pooling_ms) >> 2;
		}
		dmcfreq->freq[1].freq_upthreshold =
			((dmcfreq->freq[1].freq / (100 * SECOND)) * upthreshold * pooling_ms) >> 2;
		dmcfreq->freq[2].freq_upthreshold =
			((dmcfreq->freq[2].freq / (100 * SECOND)) * upthreshold * pooling_ms) >> 2;
		dmcfreq->freq[2].freq_downthreshold =
			((dmcfreq->freq[2].freq / (100 * SECOND)) * downthreshold * pooling_ms) >> 2;
		dmcfreq->freq[3].freq_upthreshold = 0xffffffff;
		dmcfreq->freq[3].freq_downthreshold =
			((dmcfreq->freq[3].freq / (100 * SECOND)) * downthreshold * pooling_ms) >> 2;
		for (i = 0; i < 4; i++)
			dev_pm_opp_add(dev, dmcfreq->freq[i].freq, dmcfreq->normalvoltage);
		sunxi_info(NULL, "F0:%ld, F1:%ld, F2:%ld, F3:%ld, up:%d, down:%d\n",
			dmcfreq->freq[0].freq, dmcfreq->freq[1].freq, dmcfreq->freq[2].freq,
				dmcfreq->freq[3].freq, upthreshold, downthreshold);
	} else {
		dmcfreq->freq[0].freq = (freq << 2) / (((dram_div >> 24) & 0x1f) + 1);
		dmcfreq->freq[1].freq = (freq << 2) / (((dram_div >> 16) & 0x1f) + 1);
		dmcfreq->freq[2].freq = (freq << 2) / (((dram_div >> 8) & 0x1f) + 1);
		dmcfreq->freq[3].freq = (freq << 2) / ((dram_div & 0x1f) + 1);
		dmcfreq->freq[4].freq = (freq << 2) / 7;
		dmcfreq->freq[5].freq = (freq << 2) / 6;
		dmcfreq->freq[6].freq = (freq << 2) / 5;
		dmcfreq->freq[7].freq = (freq << 2) / 4;
		dmcfreq->freq[0].freq_downthreshold = 0;
		dmcfreq->freq[0].freq_upthreshold =
				((dmcfreq->freq[0].freq / (100 * SECOND)) * (upthreshold - LOWFREQ_UP_COMP) * pooling_ms) >> 2;
		if (dmcfreq->freq[0].freq / 1000000 <= LOWFREQ_THRESHOLD) {
			dmcfreq->freq[1].freq_downthreshold = DISP_ON_DOWN_THRESHOLD >> 5;
		} else {
			dmcfreq->freq[1].freq_downthreshold =
				((dmcfreq->freq[1].freq / (100 * SECOND)) * downthreshold * pooling_ms) >> 2;
		}
		dmcfreq->freq[1].freq_upthreshold =
			((dmcfreq->freq[1].freq / (100 * SECOND)) * upthreshold * pooling_ms) >> 2;
		for (i = 2; i < 7; i++) {
			dmcfreq->freq[i].freq_upthreshold =
				((dmcfreq->freq[i].freq / (100 * SECOND)) * upthreshold * pooling_ms) >> 2;
			dmcfreq->freq[i].freq_downthreshold =
				((dmcfreq->freq[i].freq / (100 * SECOND)) * downthreshold * pooling_ms) >> 2;
		}
		dmcfreq->freq[7].freq_upthreshold = 0xffffffff;
		dmcfreq->freq[7].freq_downthreshold =
			((dmcfreq->freq[7].freq / (100 * SECOND)) * downthreshold * pooling_ms) >> 2;
		for (i = 0; i < 8; i++)
			dev_pm_opp_add(dev, dmcfreq->freq[i].freq, dmcfreq->normalvoltage);
		sunxi_info(NULL, "F0:%ld, F1:%ld, F2:%ld, F3:%ld, F4:%ld, F5:%ld, F6:%ld, F7:%ld, up:%d, down:%d\n",
			dmcfreq->freq[0].freq, dmcfreq->freq[1].freq, dmcfreq->freq[2].freq,
				dmcfreq->freq[3].freq, dmcfreq->freq[4].freq,
					dmcfreq->freq[5].freq, dmcfreq->freq[6].freq, dmcfreq->freq[7].freq,
					upthreshold, downthreshold);
	}
}
#endif

static void sunxi_adjust_freq(struct sunxi_dmcfreq *dmcfreq)
{
	struct device *dev = dmcfreq->dev;
	unsigned int ddr_type = dmcfreq->dram_type & DDR_TYPE_LPDDR4;
	unsigned int i;

	dev_pm_opp_of_remove_table(dev);
	sunxi_init_freqlist(dmcfreq);

	if (ddr_type) {
		for (i = 0; i < 8; i++) {
			dmcfreq->freq[i].freq_upthreshold >>= 1;
			dmcfreq->freq[i].freq_downthreshold >>= 1;
		}
	}
}

static struct devfreq_dev_profile sunxi_dmcfreq_profile = {
	.polling_ms     = 100,
	.target         = sunxi_dmc_target,
	.get_dev_status = sunxi_get_dev_status,
	.get_cur_freq   = sunxi_dmcfreq_get_cur_freq,
};

static void sunxi_actmon_pause(struct sunxi_dmcfreq *dmcfreq)
{
	disable_irq(dmcfreq->irq);
	sunxi_stop_nsi_de_counter(dmcfreq);
	sunxi_ddrpmu_stop_hardware_counter(dmcfreq);
}

static int sunxi_actmon_resume(struct sunxi_dmcfreq *dmcfreq)
{
	sunxi_ddrpmu_start_hardware_counter(dmcfreq);
	sunxi_start_nsi_de_counter(dmcfreq);
	enable_irq(dmcfreq->irq);

	return 0;
}

static int sunxi_actmon_start(struct sunxi_dmcfreq *dmcfreq)
{
	int ret = 0;

	ret = sunxi_actmon_resume(dmcfreq);

	return ret;
}

static void sunxi_actmon_stop(struct sunxi_dmcfreq *dmcfreq)
{
	sunxi_actmon_pause(dmcfreq);
}

static int sunxi_governor_get_target(struct devfreq *devfreq,
				     unsigned long *freq)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(devfreq->dev.parent);
	struct devfreq_dev_status *stat;
	unsigned long long a, b;
	unsigned int dfso_upthreshold = DFSO_UPTHRESHOLD;
	unsigned int dfso_downdifferential = DFSO_DOWNDIFFERENCTIAL;
	struct devfreq_simple_ondemand_data *data = devfreq->data;
	int err;

	err = devfreq_update_stats(devfreq);
	if (err) {
		DBG("devfreq_update_stats err!\n");
		return err;
	}

	stat = &devfreq->last_status;

	if (data) {
		if (data->upthreshold) {
			if (stat->current_frequency / 1000000 == dmcfreq->freq[0].freq / 1000000)
				dfso_upthreshold = data->upthreshold - LOWFREQ_UP_COMP;
			else
				dfso_upthreshold = data->upthreshold;
		}
		if (data->downdifferential)
			dfso_downdifferential = data->downdifferential;
	}
	if (dfso_upthreshold > 100 ||
	    dfso_upthreshold < dfso_downdifferential) {
		DBG("dfso_upthreshold err!\n");
		return -EINVAL;
	}

	/* Assume MAX if it is going to be divided by zero */
	if ((stat->total_time == 0) || (stat->busy_time == 0)) {
		*freq = DEVFREQ_MAX_FREQ;
		DBG("total_time is 0, so freq is max value\n");
		return 0;
	}

	/* Prevent overflow */
	if (stat->busy_time >= (1 << 24) || stat->total_time >= (1 << 24)) {
		stat->busy_time >>= 7;
		stat->total_time >>= 7;
	}

	/* Set MAX if it's busy enough */
	if (stat->busy_time * 100 >
	    stat->total_time * dfso_upthreshold) {
//		*freq = DEVFREQ_MAX_FREQ;
		a = stat->busy_time;
		a *= stat->current_frequency;
		b = div_u64(a, stat->total_time);
		b *= 100;
		b = div_u64(b, (dfso_upthreshold - dfso_downdifferential / 2));
		*freq = (unsigned long) b;
		DBG("governor raise freq:%lld\n", b);
		return 0;
	}

	/* Set MAX if we do not know the initial frequency */
	if (stat->current_frequency == 0) {
		*freq = DEVFREQ_MAX_FREQ;
		DBG("current_frequency is 0, so freq is max value\n");
		return 0;
	}

	/* Keep the current frequency */
	if (stat->busy_time * 100 >
	    stat->total_time * (dfso_upthreshold - dfso_downdifferential)) {
		*freq = stat->current_frequency;
		DBG("load is under the thresh,so freq is no change:%ld\n",
			stat->current_frequency);
		return 0;
	}

	/* Set the desired frequency based on the load */
	a = stat->busy_time;
	a *= stat->current_frequency;
	b = div_u64(a, stat->total_time);
	b *= 100;
	b = div_u64(b, (dfso_upthreshold - dfso_downdifferential / 2));
	*freq = (unsigned long) b;
	DBG("governor down freq:%lld\n", b);

	return 0;
}

static int sunxi_governor_event_handler(struct devfreq *devfreq,
					unsigned int event, void *data)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(devfreq->dev.parent);
	unsigned int *new_delay = data;
	int ret = 0;

	switch (event) {
	case DEVFREQ_GOV_START:
		devfreq_monitor_start(devfreq);
		ret = sunxi_actmon_start(dmcfreq);
		break;

	case DEVFREQ_GOV_STOP:
		sunxi_actmon_stop(dmcfreq);
		devfreq_monitor_stop(devfreq);
		break;

	case DEVFREQ_GOV_UPDATE_INTERVAL:
		/*
		 * ACTMON hardware supports up to 256 milliseconds for the
		 * sampling period.
		 */
		if (*new_delay > 256) {
			ret = -EINVAL;
			break;
		}

		sunxi_actmon_pause(dmcfreq);
		devfreq_update_interval(devfreq, new_delay);
		ret = sunxi_actmon_resume(dmcfreq);
		break;

	case DEVFREQ_GOV_SUSPEND:
		sunxi_actmon_stop(dmcfreq);
		devfreq_monitor_suspend(devfreq);
		break;

	case DEVFREQ_GOV_RESUME:
		devfreq_monitor_resume(devfreq);
		ret = sunxi_actmon_start(dmcfreq);
		break;
	}

	return ret;
}

static struct devfreq_governor sunxi_devfreq_governor = {
	.name = "sunxi_actmon",
	.attrs = DEVFREQ_GOV_ATTR_POLLING_INTERVAL,
	.flags = DEVFREQ_GOV_FLAG_IRQ_DRIVEN,// | DEVFREQ_GOV_FLAG_IMMUTABLE,
	.get_target_freq = sunxi_governor_get_target,
	.event_handler = sunxi_governor_event_handler,
};

static const struct sunxi_dmcfreq_plat_data dmcfreq_sun55iw3_data = {
	.de_nsi_masterid = 13,
	.dfi_misc_reg = 0x101b0,
};

static const struct sunxi_dmcfreq_plat_data dmcfreq_sun55iw6_data = {
	.de_nsi_masterid = 4,
	.dfi_misc_reg = 0x101b0,
};

static const struct sunxi_dmcfreq_plat_data dmcfreq_sun60iw2_data = {
	.de_nsi_masterid = 2,
	.dfi_misc_reg = 0x10510,
};

static const struct sunxi_dmcfreq_plat_data dmcfreq_sun65iw1_data = {
	.de_nsi_masterid = 2,
	.dfi_misc_reg = 0x101b0,
};

static const struct of_device_id sunxi_dmcfreq_match[] = {
	{ .compatible = "allwinner,sun55iw3-dmc", .data = &dmcfreq_sun55iw3_data},
	{ .compatible = "allwinner,sun55iw6-dmc", .data = &dmcfreq_sun55iw6_data},
	{ .compatible = "allwinner,sun60iw2-dmc", .data = &dmcfreq_sun60iw2_data},
	{ .compatible = "allwinner,sun65iw1-dmc", .data = &dmcfreq_sun65iw1_data},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_dmcfreq_match);

static int sunxi_dmcfreq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *dram_np;
	struct sunxi_dmcfreq *dmcfreq;
	const char *rname[] = {"vddcore", NULL};
	unsigned int tpr13;
	unsigned int dram_div;
	int rc = 0;

	dram_np = of_find_node_by_path("/dram");
	if (!dram_np) {
		sunxi_err(&pdev->dev, "failed to find dram node\n");
		return -ENODEV;
	}

	rc = of_property_read_u32(dram_np, "dram_para[30]", &tpr13);
	if (rc) {
		rc = of_property_read_u32(dram_np, "dram_para30", &tpr13);
		if (rc) {
			sunxi_err(&pdev->dev, "failed to find tpr13\n");
			return -ENODEV;
		}
	}
	if ((tpr13 & DEVFREQ_EN) == 0) {
		sunxi_info(&pdev->dev, "disable devfreq\n");
		return -ENODEV;
	}

	rc = of_property_read_u32(dram_np, "dram_para[24]", &dram_div);
	if (rc) {
		rc = of_property_read_u32(dram_np, "dram_para24", &dram_div);
		if (rc) {
			sunxi_err(&pdev->dev, "failed to find dram_div\n");
			return -ENODEV;
		}
	}

	dmcfreq = devm_kzalloc(dev, sizeof(*dmcfreq), GFP_KERNEL);
	if (!dmcfreq)
		return -ENOMEM;

	mutex_init(&dmcfreq->lock);

	dmcfreq->dram_div = dram_div;
	dmcfreq->down_num = 0;
	dmcfreq->rw_data = 0;
	dmcfreq->plat_data = of_device_get_match_data(dev);
	dmcfreq->dmc_clk = devm_clk_get(dev, "dram");
	if (IS_ERR(dmcfreq->dmc_clk)) {
		sunxi_err(&pdev->dev, "devm_clk_get error!\n");
		return PTR_ERR(dmcfreq->dmc_clk);
	}

	dmcfreq->bus_clk = devm_clk_get(dev, "bus");
	if (IS_ERR(dmcfreq->bus_clk)) {
		sunxi_err(&pdev->dev, "devm_clk_get error!\n");
		return PTR_ERR(dmcfreq->bus_clk);
	}

#if IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	dmcfreq->ddrpll0_clk = devm_clk_get(dev, "ddrpll0");
	if (IS_ERR(dmcfreq->ddrpll0_clk)) {
		sunxi_err(&pdev->dev, "ddrpll0 get error!\n");
		return PTR_ERR(dmcfreq->ddrpll0_clk);
	}
	dmcfreq->ddrpll[0] = clk_get_rate(dmcfreq->ddrpll0_clk);

	dmcfreq->ddrpll1_clk = devm_clk_get(dev, "ddrpll1");
	if (IS_ERR(dmcfreq->ddrpll1_clk)) {
		sunxi_err(&pdev->dev, "ddrpll1 get error!\n");
		return PTR_ERR(dmcfreq->ddrpll1_clk);
	}
	dmcfreq->ddrpll[1] = clk_get_rate(dmcfreq->ddrpll1_clk);

	dmcfreq->ddrpll2_clk = devm_clk_get(dev, "ddrpll2");
	if (IS_ERR(dmcfreq->ddrpll2_clk)) {
		sunxi_err(&pdev->dev, "ddrpll2 get error!\n");
		return PTR_ERR(dmcfreq->ddrpll2_clk);
	}
	dmcfreq->ddrpll[2] = clk_get_rate(dmcfreq->ddrpll2_clk);
#endif
	dmcfreq->irq = platform_get_irq(pdev, 0);
	if (dmcfreq->irq < 0) {
		sunxi_err(&pdev->dev, "irq get error!\n");
		return dmcfreq->irq;
	}

	irq_set_status_flags(dmcfreq->irq, IRQ_NOAUTOEN);

	rc = devm_request_threaded_irq(&pdev->dev, dmcfreq->irq, NULL,
					sunximon_thread_isr, IRQF_ONESHOT,
					"sun55iw3-devfreq", dmcfreq);
	if (rc) {
		sunxi_err(&pdev->dev, "Interrupt request failed: %d\n", rc);
		return rc;
	}

	dmcfreq->common_base = devm_of_iomap(&pdev->dev, np, 0, NULL);
	if (IS_ERR(dmcfreq->common_base)) {
		sunxi_err(&pdev->dev, "devm_ioremap_resource error!\n");
		return PTR_ERR(dmcfreq->common_base);
	}

	dmcfreq->base = devm_of_iomap(&pdev->dev, np, 1, NULL);
	if (IS_ERR(dmcfreq->base)) {
		sunxi_err(&pdev->dev, "devm_ioremap_resource error!\n");
		return PTR_ERR(dmcfreq->base);
	}

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	dmcfreq->con_base = devm_of_iomap(&pdev->dev, np, 2, NULL);
	if (IS_ERR(dmcfreq->con_base)) {
		sunxi_err(&pdev->dev, "devm_ioremap_resource error!\n");
		return PTR_ERR(dmcfreq->con_base);
	}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6)
	dmcfreq->inlinecc_irq = platform_get_irq(pdev, 1);
	if (dmcfreq->inlinecc_irq < 0) {
		sunxi_err(&pdev->dev, "inlinecc_irq get error!\n");
		return dmcfreq->inlinecc_irq;
	}

	dmcfreq->sramecc_irq = platform_get_irq(pdev, 2);
	if (dmcfreq->sramecc_irq < 0) {
		sunxi_err(&pdev->dev, "sramecc_irq get error!\n");
		return dmcfreq->sramecc_irq;
	}

	rc = devm_request_threaded_irq(&pdev->dev, dmcfreq->inlinecc_irq, NULL,
					sunxi_inlinecc_isr, IRQF_ONESHOT,
					"sun55iw3-devfreq", dmcfreq);
	if (rc) {
		sunxi_err(&pdev->dev, "Inlinecc interrupt request failed: %d\n", rc);
		return rc;
	}

	rc = devm_request_threaded_irq(&pdev->dev, dmcfreq->sramecc_irq, NULL,
					sunxi_sramecc_isr, IRQF_ONESHOT,
					"sun55iw3-devfreq", dmcfreq);
	if (rc) {
		sunxi_err(&pdev->dev, "Sramecc interrupt request failed: %d\n", rc);
		return rc;
	}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	rc = devm_pm_opp_set_regulators(dev, rname, 1);
#else
	rc = devm_pm_opp_set_regulators(dev, rname);
#endif
	if (rc) {
		sunxi_err(&pdev->dev, "failed to set core OPP regulator\n");
		return rc;
	}

	/*
	 * We add a devfreq driver to our parent since it has a device tree node
	 * with operating points.
	 */
	rc = dev_pm_opp_of_add_table(dev);
	if (rc < 0) {
		sunxi_err(&pdev->dev, "dev_pm_opp_of_add_table error!\n");
		return rc;
	}

	of_property_read_u32(np, "upthreshold",
			     &dmcfreq->ondemand_data.upthreshold);
	of_property_read_u32(np, "downdifferential",
			     &dmcfreq->ondemand_data.downdifferential);
	of_property_read_u32(np, "holdtime",
			     &down_threshold);
	of_property_read_u32(np, "normalvoltage",
			     &dmcfreq->normalvoltage);
	of_property_read_u32(np, "boostvoltage",
			     &dmcfreq->boostvoltage);

	dmcfreq->rate = clk_get_rate(dmcfreq->dmc_clk);
	dmcfreq->dev = dev;

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	dmcfreq->dram_type |= DDR_TYPE_LPDDR4;
#else
	dmcfreq->dram_type = readl(dmcfreq->common_base + MASTER_REG0);
#endif
	sunxi_adjust_freq(dmcfreq);
	sunxi_adjust_monitor_threshold(dmcfreq);
	sunxi_start_nsi_de_counter(dmcfreq);
	platform_set_drvdata(pdev, dmcfreq);

	rc = devfreq_add_governor(&sunxi_devfreq_governor);
	if (rc) {
		sunxi_err(&pdev->dev, "Failed to add governor: %d\n", rc);
		goto remove_table;
	}
	/* Add devfreq device to monitor */
	dmcfreq->devfreq = devm_devfreq_add_device(dev,
						   &sunxi_dmcfreq_profile,
						   DEVFREQ_GOV_PERFORMANCE,
						   &(dmcfreq->ondemand_data));
	if (IS_ERR(dmcfreq->devfreq)) {
		sunxi_err(&pdev->dev, "devm_devfreq_add_device error!\n");
		rc = PTR_ERR(dmcfreq->devfreq);
		goto remove_governor;
	}
	sunxi_ddrpmu_init(dmcfreq);

	return 0;

remove_governor:
	devfreq_remove_governor(&sunxi_devfreq_governor);
remove_table:
	dev_pm_opp_of_remove_table(dev);

	return rc;
}

static int sunxi_dmcfreq_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_dmcfreq *dmcfreq = platform_get_drvdata(pdev);

	if (dmcfreq == NULL)
		return 0;

	dev_pm_opp_of_remove_table(dev);
	devfreq_remove_governor(&sunxi_devfreq_governor);
	return 0;
}

static __maybe_unused int sunxi_dmcfreq_suspend(struct device *dev)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	struct dev_pm_opp *opp;
	int ret = 0;

	opp = devfreq_recommended_opp(dev, &(dmcfreq->freq[3].freq), 0);
	if (IS_ERR(opp)) {
		sunxi_err(dev, "failed to get suspend freq\n");
		return PTR_ERR(opp);
	}

	ret = dev_pm_opp_set_opp(dev, opp);
	if (ret) {
		sunxi_err(dev, "failed to set suspend freq\n");
		return ret;
	}

	if (dmcfreq == NULL)
		return 0;

	ret = devfreq_suspend_device(dmcfreq->devfreq);
	if (ret < 0) {
		sunxi_err(dev, "failed to suspend the devfreq devices\n");
		return ret;
	}

	return 0;
}

static __maybe_unused int sunxi_dmcfreq_resume(struct device *dev)
{
	struct sunxi_dmcfreq *dmcfreq = dev_get_drvdata(dev);
	int ret = 0;

	if (dmcfreq == NULL)
		return 0;

	sunxi_adjust_monitor_threshold(dmcfreq);
	sunxi_ddrpmu_init(dmcfreq);
	ret = devfreq_resume_device(dmcfreq->devfreq);
	if (ret < 0) {
		sunxi_err(dev, "failed to resume the devfreq devices\n");
		return ret;
	}
	return ret;
}

static SIMPLE_DEV_PM_OPS(sunxi_dmcfreq_pm, sunxi_dmcfreq_suspend,
			 sunxi_dmcfreq_resume);

static struct platform_driver sunxi_dmcfreq_driver = {
	.probe  = sunxi_dmcfreq_probe,
	.remove  = sunxi_dmcfreq_remove,
	.driver = {
		.name = "sunxi-dmcfreq",
		.pm = &sunxi_dmcfreq_pm,
		.of_match_table = sunxi_dmcfreq_match,
	},
};

module_platform_driver(sunxi_dmcfreq_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SUNXI dmcfreq driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("2.0.0");

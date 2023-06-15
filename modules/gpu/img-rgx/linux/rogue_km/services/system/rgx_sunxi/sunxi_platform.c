/*
 * Copyright (C) 2019 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: Albert Yu <yuxyun@allwinnertech.com>
 */

#include <asm/io.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include "platform.h"
#include <linux/clk-provider.h>
#include <linux/pm_runtime.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>

#ifdef CONFIG_DEVFREQ_THERMAL
#include <linux/devfreq_cooling.h>
#endif
#include <linux/thermal.h>
#include "rgxdevice.h"

#if defined(CONFIG_ARCH_SUN50IW10)
#define PPU_REG_BASE		(0x07001000)
#define GPU_SYS_REG_BASE		(0x01880000)
#define GPU_RSTN_REG		(0x0)
#define GPU_CLK_GATE_REG		(0x04)
#define GPU_POWEROFF_GATING_REG	(0x08)
#define GPU_PSWON_REG		(0x0c)
#define GPU_PSWOFF_REG		(0x10)
#define GPU_PD_STAT_REG		(0x20)
#define PPU_POWER_CTRL_REG	(0x24)
#define PPU_IRQ_MASK_REG		(0x2C)
#define GPU_PD_STAT_MASK		(0x03)
#define GPU_PD_STAT_ALLOFF	(0x02)
#define GPU_PD_STAT_3D_MODE	(0x00)
#define POWER_CTRL_MODE_MASK	(0x03)
#define POWER_CTRL_MODE_SW	(0x01)
#define POWER_CTRL_MODE_HW	(0x02)
#define POWER_IRQ_MASK_REQ	(0x01)
#define PPU_REG_SIZE		(0x34)

#define GPU_PD_STAT_MASK		(0x03)
#define GPU_PD_STAT_3D		(0x00)
#define GPU_PD_STAT_HOST		(0x01)
#define GPU_PD_STAT_ALLOFF	(0x02)


#define GPU_CLK_CTRL_REG		(GPU_SYS_REG_BASE + 0x20)
#define GPU_DFS_MAX		(16)
#define GPU_DFS_MIN		(13)
#endif /* defined(CONFIG_ARCH_SUN50IW10) */

struct sunxi_platform *sunxi_data;

static inline int get_clks_wrap(struct device *dev)
{
#if defined(CONFIG_OF)
	sunxi_data->clks.pll = of_clk_get(dev->of_node, 0);
	if (IS_ERR_OR_NULL(sunxi_data->clks.pll)) {
		dev_err(dev, "failed to get GPU pll clock");
		return -1;
	}

	sunxi_data->clks.core = of_clk_get(dev->of_node, 1);
	if (IS_ERR_OR_NULL(sunxi_data->clks.core)) {
		dev_err(dev, "failed to get GPU core clock");
		return -1;
	}
#endif /* defined(CONFIG_OF) */

	return 0;
}

static inline long ppu_mode_switch(int mode, bool irq_enable)
{
	long val;
	/* Mask the power_request irq */
	val = readl(sunxi_data->ppu_reg + PPU_IRQ_MASK_REG);
	if (!irq_enable)
		val = val & (~POWER_IRQ_MASK_REQ);
	else
		val = val | POWER_IRQ_MASK_REQ;
	writel(val, sunxi_data->ppu_reg + PPU_IRQ_MASK_REG);
	/* switch power mode control */
	val = readl(sunxi_data->ppu_reg + PPU_POWER_CTRL_REG);
	val = (val & ~POWER_CTRL_MODE_MASK) + mode;
	writel(val, sunxi_data->ppu_reg + PPU_POWER_CTRL_REG);

	return readl(sunxi_data->ppu_reg + PPU_POWER_CTRL_REG);
}

static inline void ppu_power_mode(int mode)
{
	unsigned int val;
	val = readl(sunxi_data->ppu_reg + GPU_PD_STAT_REG);
	val &= (~GPU_PD_STAT_MASK);
	val += mode;
	writel(val, sunxi_data->ppu_reg + GPU_PD_STAT_REG);
}

static inline void switch_interl_dfs(int val)
{
	unsigned int val2;
	writel(val, sunxi_data->gpu_reg);
	val2 = readl(sunxi_data->gpu_reg);
	WARN_ON(val != val2);
}

PVRSRV_ERROR sunxiPrePowerState(IMG_HANDLE hSysData,
					 PVRSRV_DEV_POWER_STATE eNewPowerState,
					 PVRSRV_DEV_POWER_STATE eCurrentPowerState,
					 IMG_BOOL bForced)
{
	struct sunxi_platform *platform = (struct sunxi_platform *)hSysData;

	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_ON && !platform->power_on) {
		pm_runtime_get_sync(platform->dev);
		if (!sunxi_data->soft_mode) {
			ppu_mode_switch(POWER_CTRL_MODE_HW, false);
		}
		platform->power_on = 1;
	}
	return PVRSRV_OK;

}

PVRSRV_ERROR sunxiPostPowerState(IMG_HANDLE hSysData,
					  PVRSRV_DEV_POWER_STATE eNewPowerState,
					  PVRSRV_DEV_POWER_STATE eCurrentPowerState,
					  IMG_BOOL bForced)
{
	struct sunxi_platform *platform = (struct sunxi_platform *)hSysData;

	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF
		&& platform->power_on && platform->power_idle) {
		if (!sunxi_data->soft_mode) {
			ppu_mode_switch(POWER_CTRL_MODE_SW, false);
			ppu_power_mode(GPU_PD_STAT_HOST);
		}
		pm_runtime_put_sync(platform->dev);
		platform->power_on = 0;
	}
	return PVRSRV_OK;
}

void sunxiSetFrequency(IMG_UINT32 ui32Frequency)
{
	int i = 0;
	struct sunxi_clks_table *find = NULL;

	if (NULL == sunxi_data)
		panic("oops");

	if (!sunxi_data->dvfs && sunxi_data->current_clk != &sunxi_data->clk_table[0])
		ui32Frequency = sunxi_data->clk_table[0].true_clk;

	while (i < sunxi_data->table_num) {
		if (ui32Frequency == sunxi_data->clk_table[i].true_clk) {
			find = &sunxi_data->clk_table[i];
			break;
		}
		i++;
	}
	if (find == NULL) {
		dev_err(sunxi_data->dev, "%s:give us illlegal Frequency value =%u!",
			__func__, ui32Frequency);
		return;
	}
	if (find->true_clk == sunxi_data->current_clk->true_clk)
		return;
	if (find->core_clk != sunxi_data->current_clk->core_clk) {
		if (clk_set_rate(sunxi_data->clks.core, find->core_clk) < 0) {
			dev_err(sunxi_data->dev, "%s:clk_set_rate Frequency value =%u err!",
				__func__, ui32Frequency);
			return;
		}
	}
	switch_interl_dfs(find->gpu_dfs);
	sunxi_data->current_clk = find;

}

void sunxiSetVoltage(IMG_UINT32 ui32Volt)
{
	if (NULL == sunxi_data)
		panic("oops");
	if (sunxi_data->regula && sunxi_data->independent_power) {
		if (regulator_set_voltage(sunxi_data->regula, ui32Volt, INT_MAX) != 0) {
			dev_err(sunxi_data->dev, "%s:Failed to set gpu power voltage=%d!", __func__, ui32Volt);
		}
	}
}

static int sunxi_decide_pll(struct sunxi_platform *sunxi)
{
	long pll_rate = 500000000;
#if defined(CONFIG_PM_OPP) && defined(CONFIG_OF)
	const struct property *prop;
	const __be32 *val;
	int i = 0, j = 0, nr = 0;
	unsigned long step = 0;
	unsigned long core_clk[4];
	u32 val2;
	int err;
	sunxi->pll_clk_rate = 500000000;
	err = of_property_read_u32(sunxi->dev->of_node, "pll_rate", &val2);
	if (!err)
		sunxi->pll_clk_rate = val2 * 1000;
	while (i < 4) {
		core_clk[i] = sunxi->pll_clk_rate / (i + 1);
		core_clk[i] /= 1000;
		core_clk[i] *= 1000;
		i++;
	}
	prop = of_find_property(sunxi->dev->of_node, "operating-points", NULL);
	if (!prop)
		return -ENODEV;
	if (!prop->value)
		return -ENODATA;

	nr = prop->length / sizeof(u32);
	if (nr % 2) {
		dev_err(sunxi->dev, "%s: Invalid OPP table\n", __func__);
		return -EINVAL;
	}
	sunxi->clk_table = (struct sunxi_clks_table *)kzalloc(sizeof(struct sunxi_clks_table) * nr/2, GFP_KERNEL);
	if (!sunxi->clk_table) {
		dev_err(sunxi->dev, "failed to get kzalloc sunxi_clks_table\n");
		return -1;

	}
	sunxi->table_num = nr/2;

	i = 0;
	j = 0;
	val = prop->value;
	while (nr) {
		unsigned long freq = be32_to_cpup(val++) * 1000;
		val++;
		sunxi->clk_table[i].true_clk = freq;
		while (freq < (core_clk[j] * GPU_DFS_MIN)/GPU_DFS_MAX && j < 4)
			j++;
		if (j >= 4) {
			dev_err(sunxi->dev, "%s: give us an OPP table\n", __func__);
		}
		step = core_clk[j]/16;
		sunxi->clk_table[i].gpu_dfs = 1<<(GPU_DFS_MAX-freq/step);
		sunxi->clk_table[i].core_clk =  core_clk[j];
		dev_info(sunxi->dev, "set gpu core rate:%lu freq:%lu dfs:0x%08x\n",
				core_clk[j], freq, sunxi->clk_table[i].gpu_dfs);
		if (freq % step)
			dev_err(sunxi->dev, "%s: give us an error freq:%lu\n", __func__, freq);
		nr -= 2;
		i++;
	}
	pll_rate = sunxi->pll_clk_rate;
#endif /* defined(CONFIG_OF) */

	clk_set_rate(sunxi->clks.pll, pll_rate);
	clk_set_rate(sunxi->clks.core, pll_rate);
	sunxi->current_clk = &sunxi->clk_table[0];

	return 0;
}

static int parse_dts(struct device *dev, struct sunxi_platform *sunxi_data)
{
#ifdef CONFIG_OF
	u32 val;
	int err;
	err = of_property_read_u32(dev->of_node, "gpu_idle", &val);
	if (!err)
		sunxi_data->power_idle = val ? true : false;
	err = of_property_read_u32(dev->of_node, "independent_power", &val);
	if (!err)
		sunxi_data->independent_power = val ? true : false;
	err = of_property_read_u32(dev->of_node, "dvfs_status", &val);
	if (!err)
		sunxi_data->dvfs = val ? true : false;
#endif /* CONFIG_OF */

	return 0;
}

int sunxi_platform_init(struct device *dev)
{
#if defined(CONFIG_OF)
	struct resource *reg_res;
	struct platform_device *pdev = to_platform_device(dev);
#endif /* defined(CONFIG_OF) */
	unsigned int val;

	sunxi_data = (struct sunxi_platform *)kzalloc(sizeof(struct sunxi_platform), GFP_KERNEL);
	if (!sunxi_data) {
		dev_err(dev, "failed to get kzalloc sunxi_platform");
		return -1;
	}

	dev->platform_data = sunxi_data;

#if defined(CONFIG_OF)
	pdev = to_platform_device(dev);

	reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (reg_res == NULL) {
		dev_err(dev, "failed to get register data from device tree");
		return -1;
	}
	sunxi_data->reg_base = reg_res->start;
	sunxi_data->reg_size = reg_res->end - reg_res->start + 1;
	sunxi_data->dev = dev;

	sunxi_data->irq_num = platform_get_irq_byname(pdev, "IRQGPU");
	if (sunxi_data->irq_num < 0) {
		dev_err(dev, "failed to get irq number from device tree");
		return -1;
	}
#endif /* defined(CONFIG_OF) */

	if (get_clks_wrap(dev))
		return -1;

	sunxi_data->power_idle = 1;
	sunxi_data->dvfs = 1;
	sunxi_data->independent_power = 0;
	sunxi_data->soft_mode = 1;

	parse_dts(dev, sunxi_data);

	sunxi_decide_pll(sunxi_data);

	pm_runtime_enable(dev);

	sunxi_data->ppu_reg = ioremap(PPU_REG_BASE, PPU_REG_SIZE);
	sunxi_data->gpu_reg = ioremap(GPU_CLK_CTRL_REG, 4);
	clk_prepare_enable(sunxi_data->clks.pll);
	clk_prepare_enable(sunxi_data->clks.core);
	sunxi_data->power_on = 0;

	val = readl(sunxi_data->ppu_reg + GPU_PD_STAT_REG);
	WARN_ON((val & GPU_PD_STAT_MASK) != GPU_PD_STAT_3D_MODE);
	if (!sunxi_data->soft_mode) {
		val = ppu_mode_switch(POWER_CTRL_MODE_HW, false);
		WARN_ON((val & POWER_CTRL_MODE_MASK) != POWER_CTRL_MODE_HW);
	} else {
		val = ppu_mode_switch(POWER_CTRL_MODE_SW, true);
		WARN_ON((val & POWER_CTRL_MODE_MASK) != POWER_CTRL_MODE_SW);
	}

	return 0;
}

void sunxi_platform_term(void)
{
	pm_runtime_disable(sunxi_data->dev);
	kfree(sunxi_data);

	sunxi_data = NULL;
}

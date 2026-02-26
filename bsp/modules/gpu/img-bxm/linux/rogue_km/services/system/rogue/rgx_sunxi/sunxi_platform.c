/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2023 - 2030 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023-2030 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: zhengwanyu<zhengwanyu@allwinnertech.com>
 */

#include <asm/io.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <linux/clk-provider.h>
#include <linux/pm_runtime.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>

#ifdef CONFIG_DEVFREQ_THERMAL
#include <linux/devfreq_cooling.h>
#endif
#include <linux/thermal.h>

#include "rgxdevice.h"
#include "sunxi_platform.h"
#include <sunxi-sid.h>

#if defined(SUNXI_DVFS_CTRL_ENABLE)
#include "sunxi_dvfs_ctrl.h"
#endif

#define GPU_PARENT_CLK_NAME "clk_parent"
#define GPU_CORE_CLK_NAME   "clk"
#define GPU_CLK_BUS_NAME    "clk_bus"
#define GPU_CLK_800_NAME    "clk_800"
#define GPU_CLK_600_NAME    "clk_600"
#define GPU_CLK_400_NAME    "clk_400"
#define GPU_CLK_300_NAME    "clk_300"
#define GPU_CLK_200_NAME    "clk_200"

#define GPU_RESET_BUS	     "reset_bus"
#define GPU0_RESET_BUS_SMMU  "reset_bus_smmu_gpu0"
#define GPU1_RESET_BUS_SMMU  "reset_bus_smmu_gpu1"

#define PPU_GPU_TOP 0x07066000
#define DEFAULT_GPU_RATE 600000000

struct sunxi_platform *sunxi_data;

#if defined(USE_FPGA)
static void sunxi_ppu_enable_gpu(void)
{
    void __iomem *ioaddr;
    int ret;

    ioaddr = ioremap(PPU_GPU_TOP, 4);
    ret = readl(ioaddr);
    printk("Before write PPU_GPU_TOP value is 0x%08x\n", ret);

    writel(0x100, ioaddr);
    ret = readl(ioaddr);

    printk("PPU_GPU_TOP value is 0x%08x\n", ret);
}
#endif

static int sunxi_get_clks_wrap(struct device *dev)
{
#if defined(CONFIG_OF)
	sunxi_data->clk_parent = of_clk_get_by_name(dev->of_node, GPU_PARENT_CLK_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_parent)) {
		dev_err(dev, "failed to get GPU clk_parent clock");
		return -1;
	}

	sunxi_data->clk_core = of_clk_get_by_name(dev->of_node, GPU_CORE_CLK_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_core)) {
		dev_err(dev, "failed to get GPU clk_core clock");
		return -1;
	}

	sunxi_data->clk_bus = of_clk_get_by_name(dev->of_node, GPU_CLK_BUS_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_core)) {
		dev_err(dev, "failed to get GPU clk_bus clock");
		return -1;
	}

	sunxi_data->clk_800 = of_clk_get_by_name(dev->of_node, GPU_CLK_800_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_800)) {
		dev_err(dev, "failed to get GPU clk_800 clock");
		return -1;
	}

	sunxi_data->clk_600 = of_clk_get_by_name(dev->of_node, GPU_CLK_600_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_600)) {
		dev_err(dev, "failed to get GPU clk_600 clock");
		return -1;
	}

	sunxi_data->clk_400 = of_clk_get_by_name(dev->of_node, GPU_CLK_400_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_400)) {
		dev_err(dev, "failed to get GPU clk_400 clock");
		return -1;
	}

	sunxi_data->clk_300 = of_clk_get_by_name(dev->of_node, GPU_CLK_300_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_300)) {
		dev_err(dev, "failed to get GPU clk_300 clock");
		return -1;
	}

	sunxi_data->clk_200 = of_clk_get_by_name(dev->of_node, GPU_CLK_200_NAME);
	if (IS_ERR_OR_NULL(sunxi_data->clk_200)) {
		dev_err(dev, "failed to get GPU clk_200 clock");
		return -1;
	}

	sunxi_data->rst_bus = devm_reset_control_get(dev, GPU_RESET_BUS);
	if (IS_ERR_OR_NULL(sunxi_data->rst_bus)) {
		dev_err(dev, "failed to get %s clock", GPU_RESET_BUS);
		return -1;
	}
#endif /* defined(CONFIG_OF) */

	return 0;
}

#define GPU_CLK 0x02002b20
static void sunxi_enable_clks_wrap(struct device *dev)
{
	void __iomem *ioaddr;
	unsigned long value;

	if (sunxi_data->clk_parent)
		clk_prepare_enable(sunxi_data->clk_parent);
	else
		dev_err(dev, "%s No clk_parent\n", __func__);

	if (sunxi_data->clk_800)
		clk_prepare_enable(sunxi_data->clk_800);
	else
		dev_err(dev, "%s No clk_800\n", __func__);

	if (sunxi_data->clk_600)
		clk_prepare_enable(sunxi_data->clk_600);
	else
		dev_err(dev, "%s No clk_600\n", __func__);

	if (sunxi_data->clk_400)
		clk_prepare_enable(sunxi_data->clk_400);
	else
		dev_err(dev, "%s No clk_400\n", __func__);

	if (sunxi_data->clk_300)
		clk_prepare_enable(sunxi_data->clk_300);
	else
		dev_err(dev, "%s No clk_300\n", __func__);

	if (sunxi_data->clk_200)
		clk_prepare_enable(sunxi_data->clk_200);
	else
		dev_err(dev, "%s No clk_200\n", __func__);

	if (sunxi_data->rst_bus)
		reset_control_deassert(sunxi_data->rst_bus);
	else
		dev_err(dev, "%s No rst_bus\n", __func__);

	if (sunxi_data->clk_bus)
		clk_prepare_enable(sunxi_data->clk_bus);
	else
		dev_err(dev, "%s No clk_bus\n", __func__);

	if (sunxi_data->clk_core)
		clk_prepare_enable(sunxi_data->clk_core);
	else
		dev_err(dev, "%s No clk_core\n", __func__);

	ioaddr = ioremap(GPU_CLK, 4);
	value = readl(ioaddr);

	writel(value | 1 << 27, ioaddr);
	iounmap(ioaddr);
}

static void sunxi_disable_clks_wrap(struct device *dev)
{
	if (sunxi_data->rst_bus)
		clk_disable_unprepare(sunxi_data->clk_bus);
	else
		dev_err(dev, "%s No rst_bus\n", __func__);

	if (sunxi_data->clk_core)
		clk_disable_unprepare(sunxi_data->clk_core);
	else
		dev_err(dev, "%s No clk_core\n", __func__);

	if (sunxi_data->clk_200)
		clk_disable_unprepare(sunxi_data->clk_200);
	else
		dev_err(dev, "%s No clk_200\n", __func__);

	if (sunxi_data->clk_300)
		clk_disable_unprepare(sunxi_data->clk_300);
	else
		dev_err(dev, "%s No clk_300\n", __func__);

	if (sunxi_data->clk_400)
		clk_disable_unprepare(sunxi_data->clk_400);
	else
		dev_err(dev, "%s No clk_400\n", __func__);

	if (sunxi_data->clk_600)
		clk_disable_unprepare(sunxi_data->clk_600);
	else
		dev_err(dev, "%s No clk_600\n", __func__);

	if (sunxi_data->clk_800)
		clk_disable_unprepare(sunxi_data->clk_800);
	else
		dev_err(dev, "%s No clk_800\n", __func__);

	if (sunxi_data->clk_parent)
		clk_disable_unprepare(sunxi_data->clk_parent);
	else
		dev_err(dev, "%s No clk_parent\n", __func__);

	if (sunxi_data->rst_bus)
		reset_control_assert(sunxi_data->rst_bus);
	else
		dev_err(dev, "%s No clk_bus\n", __func__);
}

void sunxi_set_device_clk_rate(unsigned long rate)
{
	sunxiSetFrequency(rate);
}

IMG_UINT32 sunxi_get_device_clk_rate(IMG_HANDLE hSysData)
{
	return clk_get_rate(sunxi_data->clk_core);
}

PVRSRV_ERROR sunxiPrePowerState(IMG_HANDLE hSysData,
				PVRSRV_DEV_POWER_STATE eNewPowerState,
				PVRSRV_DEV_POWER_STATE eCurrentPowerState,
				PVRSRV_POWER_FLAGS ePwrFlags)
{
	struct sunxi_platform *platform = (struct sunxi_platform *)hSysData;

	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_ON && !platform->power_on) {
		sunxi_enable_clks_wrap(platform->dev);
		pm_runtime_get_sync(platform->dev);
		platform->power_on = 1;
	}
	return PVRSRV_OK;

}

PVRSRV_ERROR sunxiPostPowerState(IMG_HANDLE hSysData,
				 PVRSRV_DEV_POWER_STATE eNewPowerState,
				 PVRSRV_DEV_POWER_STATE eCurrentPowerState,
				 PVRSRV_POWER_FLAGS ePwrFlags)
{
	struct sunxi_platform *platform = (struct sunxi_platform *)hSysData;

	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF
		&& platform->power_on) {
		pm_runtime_put_sync(platform->dev);
		sunxi_disable_clks_wrap(platform->dev);
		platform->power_on = 0;
	}
	return PVRSRV_OK;
}

void sunxiSetFrequency(IMG_UINT32 ui32Frequency)
{
	IMG_UINT32 ui32FrequencyMHZ = ui32Frequency / 1000000;

	if (ui32FrequencyMHZ > 800) {
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_parent) < 0) {
			dev_err(sunxi_data->dev, "%s:clk_set_parent err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_parent, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:clk_set_rate err!", __func__);
		}

		return;
	}

	switch (ui32FrequencyMHZ) {
	case 800:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_800) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 800 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 800 err!", __func__);
		}
		break;
	case 600:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_600) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 600 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 600 err!", __func__);
		}
		break;
	case 400:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_400) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 400 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 400 err!", __func__);
		}
		break;
	case 300:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_300) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 300 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 300 err!", __func__);
		}
		break;
	case 200:
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_200) < 0) {
			dev_err(sunxi_data->dev, "%s:set parent 200 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set rate 200 err!", __func__);
		}
		break;
	default:
		dev_err(sunxi_data->dev, "%s: Unsupport freq:%u\n", __func__, ui32Frequency);
		if (clk_set_parent(sunxi_data->clk_core, sunxi_data->clk_600) < 0) {
			dev_err(sunxi_data->dev, "%s:set default parent 600 err!", __func__);
		}
		if (clk_set_rate(sunxi_data->clk_core, ui32Frequency) < 0) {
			dev_err(sunxi_data->dev, "%s:set default 600 err!", __func__);
		}
		break;
	}
}

void sunxiSetVoltage(IMG_UINT32 ui32Volt)
{
	if (sunxi_data->regula) {
		if (regulator_set_voltage(sunxi_data->regula,
					ui32Volt, INT_MAX) != 0) {
			dev_err(sunxi_data->dev,
				"%s:Failed to set gpu power voltage=%d!",
				__func__, ui32Volt);
		}
		udelay(200);
	}
}

#if defined(SUPPORT_LINUX_DVFS)
#define DVFS_EFUSE_OFF         (0x4c)

static int sunxi_match_vf_table(struct device *dev, u32 combi, u32 *value)
{
	struct device_node *np = NULL;
	int nsels, ret, i;
	u32 tmp;

	np = of_find_node_by_name(NULL, "vf_mapping_table");
	if (!np) {
		dev_err(dev, "Unable to find node 'note'\n");
		return -EINVAL;
	}

	if (!of_get_property(np, "table", &nsels)) {
		dev_err(dev, "can NOT get 'table' property under node of 'vf_mapping_table'\n");
		return -EINVAL;
	}

	nsels /= sizeof(u32);
	if (!nsels) {
		dev_err(dev, "invalid table property size\n");
		return -EINVAL;
	}

	for (i = 0; i < nsels / 2; i++) {
		ret = of_property_read_u32_index(np, "table", i * 2, &tmp);
		if (ret) {
			dev_err(dev, "could not retrieve table property: %d\n", ret);
			return ret;
		}

		if (tmp == combi) {
			ret = of_property_read_u32_index(np, "table", i * 2 + 1, &tmp);
			if (ret) {
				dev_err(dev, "could not retrieve table property: %d\n", ret);
				return ret;
			}

			*value = tmp;
			break;
		} else
			continue;
	}

	if (i == nsels/2) {
		dev_err(dev, "%s %d, could not match vf table, i:%d", __func__, __LINE__, i);
		return -1;
	}

	return 0;
}

static int sunxi_get_opp(struct device *dev, struct sunxi_platform *sunxi_data)
{
	int match = 0;
	unsigned int dvfs_code;
	unsigned int dvfs_value;

	int opp_count = 0;
	int index;
	bool order_reverse = false;
	struct device_node *node;
	struct device_node *opp_node = of_parse_phandle(dev->of_node,
							"operating-points-v2", 0);
	if (!opp_node) {
		dev_err(dev, "of_parse_phandle failed");
		return -1;
	}

	sunxi_get_module_param_from_sid(&dvfs_code, DVFS_EFUSE_OFF, 4);
	dev_info(dev, "get dvfs_code:0x%x\n", dvfs_code);
	dvfs_code = (dvfs_code >> 24) & 0xff;
	match = sunxi_match_vf_table(dev, dvfs_code, &dvfs_value);
	dev_info(dev, "get dvfs_value:0x%04x\n", dvfs_value);

	index = 0;
	for_each_available_child_of_node(opp_node, node) {
		int err;
		u64 opp_freq;
		u32 opp_uvolt;
		char opp_microvolt_name[40] = { 0 };

		err = of_property_read_u64(node, "opp-hz", &opp_freq);
		if (err) {
			dev_err(dev, "Failed to read opp-hz property with error %d\n", err);
			continue;
		}

		if (match < 0)
			sprintf(opp_microvolt_name, "opp-microvolt-vfdefault");
		else
			sprintf(opp_microvolt_name, "opp-microvolt-vf%04x", dvfs_value);
		err = of_property_read_u32(node, opp_microvolt_name, &opp_uvolt);
		if (err) {
			dev_err(dev, "Failed to read %s property with error %d\n", opp_microvolt_name, err);
			continue;
		}

		sunxi_data->asOPPTable[index].ui32Freq = (IMG_UINT32)opp_freq;
		sunxi_data->asOPPTable[index].ui32Volt = (IMG_UINT32)opp_uvolt;
		dev_info(dev, "get opp-hz=%u, %s=%u)\n", sunxi_data->asOPPTable[index].ui32Freq,
							 opp_microvolt_name,
							 sunxi_data->asOPPTable[index].ui32Volt);

		index++;
	}

	opp_count = index;
	dev_info(dev, "opp count:%d\n", opp_count);
	if (opp_count > OPP_MAX_NUM) {
		dev_err(dev, "opp count is too big, has exceed OPP_MAX_NUM:%d\n", OPP_MAX_NUM);
		return -1;
	}

	sunxi_data->ui32OPPTableSize = opp_count;

	if (sunxi_data->asOPPTable[0].ui32Freq > sunxi_data->asOPPTable[1].ui32Freq)
		order_reverse = true;
	sunxi_data->clk_rate = order_reverse ? sunxi_data->asOPPTable[0].ui32Freq
					: sunxi_data->asOPPTable[opp_count - 1].ui32Freq;
	sunxi_data->volt = order_reverse ? sunxi_data->asOPPTable[0].ui32Volt
					: sunxi_data->asOPPTable[opp_count - 1].ui32Volt;
	dev_info(dev, "change default clk_rate to max opp freq:%u volt:%u\n",
			sunxi_data->clk_rate, sunxi_data->volt);


	return 0;
}
#endif

static int sunxi_parse_dts(struct device *dev, struct sunxi_platform *sunxi_data)
{
#ifdef CONFIG_OF
	int ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *reg_res;

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

#if defined(SUNXI_DVFS_CTRL_ENABLE)
	sunxi_data->dvfs_irq_num = platform_get_irq_byname(pdev, "IRQGPUDVFS");
	if (sunxi_data->dvfs_irq_num < 0) {
		dev_err(dev, "failed to get dvfs irq number from device tree");
		return -1;
	}
#endif
	ret = of_property_read_u32(dev->of_node, "clk_rate", &sunxi_data->clk_rate);
	if (ret < 0) {
		dev_info(dev, "warning: default clk_rate is NOT set in DTS, set it to default:%d\n", DEFAULT_GPU_RATE);
		sunxi_data->clk_rate = DEFAULT_GPU_RATE;
	}
	dev_info(dev, "%s clk_rate:%d\n", __func__, sunxi_data->clk_rate);

	ret = sunxi_get_clks_wrap(dev);
	if (ret < 0) {
		dev_err(dev, "sunxi_get_clks_wrap failed");
		return ret;
	}

	sunxi_data->regula = regulator_get_optional(dev, "gpu");
	if (!sunxi_data->regula) {
		dev_err(dev, "regulator_get_optional for gpu-supply failed\n");
	}

	sunxi_data->volt = regulator_get_voltage(sunxi_data->regula);
	dev_info(dev, "succeed to get gpu regulator, default volt value:%u",
				sunxi_data->volt);

#endif /* CONFIG_OF */

#if defined(SUPPORT_LINUX_DVFS)
	sunxi_get_opp(dev, sunxi_data);
#endif
	dev_info(dev, "%s finished\n", __func__);

	return 0;
}

int sunxi_platform_init(struct device *dev)
{
#if defined(SUNXI_DVFS_CTRL_ENABLE)
	struct sunxi_dvfs_init_params dvfs_init_para;
#endif

	dev_info(dev, "%s start\n", __func__);
	sunxi_data = (struct sunxi_platform *)kzalloc(sizeof(struct sunxi_platform), GFP_KERNEL);
	if (!sunxi_data) {
		dev_err(dev, "failed to get kzalloc sunxi_platform");
		return -1;
	}
	dev->platform_data = sunxi_data;

	if (sunxi_parse_dts(dev, sunxi_data) < 0)
		return -1;

	sunxi_enable_clks_wrap(dev);
	sunxi_data->power_on = 1;
#if defined(USE_FPGA)
	sunxi_ppu_enable_gpu();
#endif
	sunxi_set_device_clk_rate(sunxi_data->clk_rate);
	dev_info(dev, "sunxi_set_device_clk_rate:%d\n", sunxi_data->clk_rate);

#if defined(SUNXI_DVFS_CTRL_ENABLE)
	dvfs_init_para.reg_base = sunxi_data->reg_base + SUNXI_DVFS_CTRL_OFFSET;
	dvfs_init_para.clk_rate = sunxi_data->clk_rate;
	dvfs_init_para.irq_no = sunxi_data->dvfs_irq_num;
	if (sunxi_dvfs_ctrl_init(dev, &dvfs_init_para) < 0) {
		dev_err(dev, "sunxi_dvfs_ctrl_init failed\n");
		return -1;
	}
	dev_info(dev, "sunxi_dvfs_ctrl_init");

	if (sunxi_dvfs_ctrl_get_opp_table(&sunxi_data->asOPPTable,
					&sunxi_data->ui32OPPTableSize) < 0) {
		dev_err(dev, "No any valid opp got!\n");
		return -1;
	}
#endif
	if (regulator_enable(sunxi_data->regula) < 0)
		dev_err(dev, "regulator_enable in probe failed\n");
	sunxiSetVoltage(sunxi_data->volt);
	dev_info(dev, "sunxiSetVoltage:%u\n", sunxi_data->volt);

	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);
	dev_info(dev, "%s end\n", __func__);
	return 0;
}

void sunxi_platform_term(void)
{
	sunxi_disable_clks_wrap(sunxi_data->dev);
	pm_runtime_disable(sunxi_data->dev);
	kfree(sunxi_data);

	sunxi_data = NULL;
}

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* #define DEBUG */
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <asm/io.h>
#include <linux/remoteproc.h>
#include <linux/mailbox_client.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/of_reserved_mem.h>

#include "sunxi_rproc_boot.h"
#include "sunxi_rproc_standby.h"
#include <sunxi-log.h>

#ifdef dev_fmt
#undef dev_fmt
#define dev_fmt(fmt) fmt
#endif

/*
 * RISC-V CFG Peripheral Register
 */
#define RV_CFG_VER_REG			(0x0000) /* RV_CFG Version Register */
#define RV_CFG_RF1P_CFG_REG		(0x0010) /* RV_CFG Control Register0 */
#define RV_CFG_TS_TMODE_SEL_REG		(0x0040) /* RV_CFG Test Mode Select Register */
#define RV_CFG_STA_ADD_REG		(0x0204) /* RV_CFG Boot Address Register */
#define RV_CFG_WAKEUP_EN_REG		(0x0220) /* RV_CFG WakeUp Enable Register */
#define RV_CFG_WAKEUP_MASK0_REG		(0x0224) /* RV_CFG WakeUp Mask0 Register */
#define RV_CFG_WAKEUP_MASK1_REG		(0x0228) /* RV_CFG WakeUp Mask1 Register */
#define RV_CFG_WAKEUP_MASK2_REG		(0x022C) /* RV_CFG WakeUp Mask2 Register */
#define RV_CFG_WAKEUP_MASK3_REG		(0x0230) /* RV_CFG WakeUp Mask3 Register */
#define RV_CFG_WAKEUP_MASK4_REG		(0x0234) /* RV_CFG WakeUp Mask4 Register */
#define RV_CFG_WORK_MODE_REG		(0x0248) /* RV_CFG Worke Mode Register */

/*
 * RV_CFG Version Register
 */
#define SMALL_VER_MASK			(0x1f << 0)
#define LARGE_VER_MASK			(0x1f << 16)

/*
 * RV_CFG Test Mode Select Register
 */
#define BIT_TEST_MODE			(1 << 1)

/*
 * RV_CFG WakeUp Enable Register
 */
#define BIT_WAKEUP_EN			(1 << 0)

/*
 * RV_CFG Worke Mode Register
 */
#define BIT_LOCK_STA			(1 << 3)
#define BIT_DEBUG_MODE			(1 << 2)
#define BIT_LOW_POWER_MASK		(0x3)
#define BIT_DEEP_SLEEP_MODE		(0x0)
#define BIT_LIGHT_SLEEP_MODE		(0x1)

#define RPROC_NAME "e907"

extern int simulator_debug;

static int sunxi_rproc_e907_attach_pd(struct device *dev, const char *values_of_power_domain_names[], int count);


#define RV_DTS_PROPERTY_CORE_CLK_FREQ "core-clock-frequency"
#define RV_DTS_PROPERTY_AXI_CLK_FREQ "axi-clock-frequency"

#define RV_CORE_MUX_INPUT_CLK_NAME "input"
#define RV_CORE_DIV_CLK_NAME "core-div"
#define RV_CORE_GATE_CLK_NAME "core-gate"
#define RV_AXI_DIV_CLK_NAME "axi-div"
#define RV_CFG_GATE_CLK_NAME "rv-cfg-gate"

#define RV_CORE_RST_NAME "core-rst"
#define RV_DBG_RST_NAME "dbg-rst"
#define RV_CFG_RST_NAME "rv-cfg-rst"

#define RV_IO_RES_NAME "rv-cfg"

static inline int parse_clk(struct device *dev, struct clk **clk, const char *clk_name)
{
	*clk = devm_clk_get(dev, clk_name);
	if (IS_ERR_OR_NULL(*clk)) {
		dev_err(dev, "get clk '%s' failed, ret: %ld\n", clk_name, PTR_ERR(*clk));
		return -ENXIO;
	}

	return 0;
}

static int parse_clk_resource(struct device *dev, struct sunxi_rproc_e907_cfg *cfg)
{
	struct device_node *np = dev->of_node;
	const char *property_name = NULL, *clk_name = NULL;
	int ret = 0;

	property_name = RV_DTS_PROPERTY_CORE_CLK_FREQ;
	ret = of_property_read_u32(np, property_name, &cfg->core_freq);
	if (ret) {
		dev_err(dev, "parse dts property '%s' failed, ret: %d", property_name, ret);
		return -ENXIO;
	}

	property_name = RV_DTS_PROPERTY_AXI_CLK_FREQ;
	ret = of_property_read_u32(np, property_name, &cfg->axi_freq);
	if (ret) {
		dev_err(dev, "parse dts property '%s' failed, ret: %d", property_name, ret);
		return -ENXIO;
	}

	clk_name = RV_CORE_MUX_INPUT_CLK_NAME;
	ret = parse_clk(dev, &cfg->core_mux_input_clk, clk_name);
	if (ret)
		return ret;

	clk_name = RV_CORE_DIV_CLK_NAME;
	ret = parse_clk(dev, &cfg->core_div_clk, clk_name);
	if (ret)
		return ret;

	clk_name = RV_CORE_GATE_CLK_NAME;
	ret = parse_clk(dev, &cfg->core_gate_clk, clk_name);
	if (ret)
		return ret;

	clk_name = RV_AXI_DIV_CLK_NAME;
	ret = parse_clk(dev, &cfg->axi_div_clk, clk_name);
	if (ret)
		return ret;

	clk_name = RV_CFG_GATE_CLK_NAME;
	ret = parse_clk(dev, &cfg->rv_cfg_gate_clk, clk_name);
	if (ret)
		return ret;

	clk_name = RV_CORE_RST_NAME;
	ret = parse_clk(dev, &cfg->core_rst, clk_name);
	if (ret)
		return ret;

	clk_name = RV_DBG_RST_NAME;
	ret = parse_clk(dev, &cfg->dbg_rst, clk_name);
	if (ret)
		return ret;

	return 0;
}

static inline int set_reset_state(struct device *dev, struct clk *clk, int active)
{
	int ret;

	if (active) {
		if (!__clk_is_enabled(clk))
			return 0;

		clk_disable_unprepare(clk);
	} else {
		if (__clk_is_enabled(clk))
			return 0;

		ret = clk_prepare_enable(clk);
		if (ret) {
			dev_err(dev, "exit reset state failed, name: '%s', ret: %d\n",
					__clk_get_name(clk), ret);
			return ret;
		}
	}

	return 0;
}

static int set_core_reset_state(struct sunxi_rproc_priv *rproc_priv, int active)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;

	return set_reset_state(rproc_priv->dev, cfg->core_rst, active);
}

static int set_dbg_reset_state(struct sunxi_rproc_priv *rproc_priv, int active)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;

	return set_reset_state(rproc_priv->dev, cfg->dbg_rst, active);
}

static int sunxi_rproc_e907_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	struct sunxi_rproc_e907_cfg *cfg;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct resource *res;
	u32 *map_array;
	int ret, i, count;
	const char *values_of_power_domain_names[16];
	const char *rst_name = NULL;
	struct reset_control **rst;

	rproc_priv->dev = dev;

	cfg = devm_kzalloc(dev, sizeof(*cfg), GFP_KERNEL);
	if (!cfg) {
		dev_err(dev, "alloc rproc cfg error\n");
		return -ENOMEM;
	}

	ret = parse_clk_resource(dev, cfg);
	if (ret)
		return ret;

	rst_name = RV_CFG_RST_NAME;
	rst = &cfg->rv_cfg_rst;
	*rst = devm_reset_control_get(dev, rst_name);
	if (IS_ERR_OR_NULL(*rst)) {
		dev_err(dev, "get rst '%s' failed, ret: %ld\n", rst_name, PTR_ERR(*rst));
		return -ENXIO;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, RV_IO_RES_NAME);
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "get io resource '%s' failed, ret: %ld\n",
				RV_IO_RES_NAME, PTR_ERR(res));
		return -ENXIO;
	}

	dev_info(dev, "rv_cfg module base: 0x%08x", res->start);

	cfg->rv_cfg_reg_base = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(cfg->rv_cfg_reg_base)) {
		dev_err(dev, "ioremap resource '%s' failed, ret: %ld\n",
				res->name, PTR_ERR(cfg->rv_cfg_reg_base));
		return -ENXIO;
	}

	count = of_property_count_strings(np, "power-domain-names");
	if (count > 0) {
		ret = of_property_read_string_array(np, "power-domain-names",
											values_of_power_domain_names, count);
		if (ret < 0) {
			dev_err(dev, "fail to get power domain names\n");
			return -ENXIO;
		}

		ret = sunxi_rproc_e907_attach_pd(dev, values_of_power_domain_names, count);
		if (ret) {
			dev_err(dev, "fail to attach pd\n");
			return -ENXIO;
		}

		pm_runtime_enable(dev);
	}

	ret = of_property_count_elems_of_size(np, "memory-mappings", sizeof(u32) * 3);
	if (ret <= 0) {
		dev_err(dev, "fail to get memory-mappings\n");
		ret = -ENXIO;
		goto disadle_pm;
	}
	rproc_priv->mem_maps_cnt = ret;
	rproc_priv->mem_maps = devm_kcalloc(dev, rproc_priv->mem_maps_cnt,
										sizeof(*(rproc_priv->mem_maps)),
										GFP_KERNEL);
	if (!rproc_priv->mem_maps) {
		ret = -ENOMEM;
		goto disadle_pm;
	}

	map_array = devm_kcalloc(dev, rproc_priv->mem_maps_cnt * 3, sizeof(u32), GFP_KERNEL);
	if (!map_array) {
		ret = -ENOMEM;
		goto disadle_pm;
	}

	ret = of_property_read_u32_array(np, "memory-mappings", map_array,
									 rproc_priv->mem_maps_cnt * 3);
	if (ret) {
		dev_err(dev, "fail to read memory-mappings\n");
		ret = -ENXIO;
		goto disadle_pm;
	}

	for (i = 0; i < rproc_priv->mem_maps_cnt; i++) {
		rproc_priv->mem_maps[i].da = map_array[i * 3];
		rproc_priv->mem_maps[i].len = map_array[i * 3 + 1];
		rproc_priv->mem_maps[i].pa = map_array[i * 3 + 2];
		dev_dbg(dev, "memory-mappings[%d]: da: 0x%llx, len: 0x%llx, pa: 0x%llx\n",
				i, rproc_priv->mem_maps[i].da, rproc_priv->mem_maps[i].len,
				rproc_priv->mem_maps[i].pa);
	}

	devm_kfree(dev, map_array);

	rproc_priv->rproc_cfg = cfg;

	return 0;

disadle_pm:
	if (count > 0)
		pm_runtime_disable(dev);

	return ret;
}

static void sunxi_rproc_e907_resource_put(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_disable(dev);
}

static int sunxi_rproc_e907_start(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;
	struct device *dev = rproc_priv->dev;
	unsigned long core_freq, axi_freq;
	int ret;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(dev, "%s,%d rproc does not need to reset clk\n",
				__func__, __LINE__);
		return 0;
	}

	ret = set_core_reset_state(rproc_priv, 1);
	if (ret) {
		dev_err(rproc_priv->dev, "core rst assert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = set_dbg_reset_state(rproc_priv, 1);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst assert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = reset_control_assert(cfg->rv_cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "rv cfg rst assert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = clk_prepare_enable(cfg->rv_cfg_gate_clk);
	if (ret) {
		dev_err(dev, "cfg clk enable err, ret: %d\n", ret);
		return ret;
	}

	ret = clk_set_parent(cfg->core_div_clk, cfg->core_mux_input_clk);
	if (ret) {
		dev_err(dev, "set parent failed, ret: %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(cfg->core_div_clk, cfg->core_freq);
	if (ret) {
		dev_warn(dev, "set core clk rate failed, ret: %d\n", ret);
	}

	ret = clk_set_rate(cfg->axi_div_clk, cfg->axi_freq);
	if (ret) {
		dev_warn(dev, "set axi clk rate failed, ret: %d\n", ret);
	}

	ret = clk_prepare_enable(cfg->core_gate_clk);
	if (ret) {
		dev_err(dev, "core clk enable err, ret: %d\n", ret);
		return ret;
	}

	ret = reset_control_deassert(cfg->rv_cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "rv cfg rst deassert err, ret: %d\n", ret);
		return -ENXIO;
	}

	dev_info(dev, "boot address: 0x%08x", rproc_priv->pc_entry);
	writel(rproc_priv->pc_entry, (cfg->rv_cfg_reg_base + RV_CFG_STA_ADD_REG));

	core_freq = clk_get_rate(cfg->core_div_clk);
	axi_freq = clk_get_rate(cfg->axi_div_clk);

	dev_info(dev, "core freq: %luHz, axi freq: %luHz", core_freq, axi_freq);

	ret = set_dbg_reset_state(rproc_priv, 0);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst dessert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = set_core_reset_state(rproc_priv, 0);
	if (ret) {
		dev_err(rproc_priv->dev, "core rst deassert err, ret: %d\n", ret);
		return -ENXIO;
	}

	return 0;
}

static int sunxi_rproc_e907_stop(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;
	int ret;

	dev_dbg(rproc_priv->dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(rproc_priv->dev, "%s,%d rproc does not need to close clk\n",
				__func__, __LINE__);
		return 0;
	}

	ret = set_core_reset_state(rproc_priv, 1);
	if (ret) {
		dev_err(rproc_priv->dev, "core rst assert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = set_dbg_reset_state(rproc_priv, 1);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst assert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = reset_control_assert(cfg->rv_cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "rv cfg rst assert err, ret: %d\n", ret);
		return -ENXIO;
	}

	clk_disable_unprepare(cfg->core_gate_clk);
	clk_disable_unprepare(cfg->rv_cfg_gate_clk);

	pm_runtime_put_sync(rproc_priv->dev);
	return 0;
}


static int sunxi_rproc_e907_attach(struct sunxi_rproc_priv *rproc_priv)
{
	struct device *dev = rproc_priv->dev;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	pm_runtime_get_sync(dev);
	return 0;
}

static int sunxi_rproc_e907_assert(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;
	int ret;

	ret = set_core_reset_state(rproc_priv, 1);
	if (ret) {
		dev_err(rproc_priv->dev, "core rst assert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = set_dbg_reset_state(rproc_priv, 1);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst assert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = reset_control_assert(cfg->rv_cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "rv cfg rst assert err, ret: %d\n", ret);
		return -ENXIO;
	}

	return ret;
}

static int sunxi_rproc_e907_deassert(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;
	int ret;

	ret = set_core_reset_state(rproc_priv, 0);
	if (ret) {
		dev_err(rproc_priv->dev, "core rst deassert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = set_dbg_reset_state(rproc_priv, 0);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst deassert err, ret: %d\n", ret);
		return -ENXIO;
	}

	ret = reset_control_deassert(cfg->rv_cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "rv cfg rst deassert err, ret: %d\n", ret);
		return -ENXIO;
	}

	return ret;
}

static int sunxi_rproc_e907_reset(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;

	ret = sunxi_rproc_e907_assert(rproc_priv);
	if (ret)
		return -ENXIO;

	ret = sunxi_rproc_e907_deassert(rproc_priv);
	if (ret)
		return -ENXIO;

	return ret;
}

static int sunxi_rproc_e907_enable_sram(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return 0;
}

static int sunxi_rproc_e907_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return 0;
}

static bool sunxi_rproc_e907_is_booted(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e907_cfg *cfg = rproc_priv->rproc_cfg;

	return __clk_is_enabled(cfg->core_gate_clk);
}

static struct sunxi_rproc_ops sunxi_rproc_e907_ops = {
	.resource_get = sunxi_rproc_e907_resource_get,
	.resource_put = sunxi_rproc_e907_resource_put,
	.start = sunxi_rproc_e907_start,
	.stop = sunxi_rproc_e907_stop,
	.attach = sunxi_rproc_e907_attach,
	.reset = sunxi_rproc_e907_reset,
	.set_localram = sunxi_rproc_e907_enable_sram,
	.set_runstall = sunxi_rproc_e907_set_runstall,
	.is_booted = sunxi_rproc_e907_is_booted,
};

static int sunxi_rproc_e907_attach_pd(struct device *dev, const char *values_of_power_domain_names[], int count)
{
	struct device_link *link;
	struct device *pd_dev;
	int i;

	/* Do nothing when in a single power domain */
	if (dev->pm_domain)
		return 0;

	for (i = 0; i < count; i++) {
		pd_dev = dev_pm_domain_attach_by_name(dev, values_of_power_domain_names[i]);
		if (IS_ERR(pd_dev))
			return PTR_ERR(pd_dev);
		/* Do nothing when power domain missing */
		else if (!pd_dev)
			return 0;
		else {
			link = device_link_add(dev, pd_dev,
								   DL_FLAG_STATELESS |
								   DL_FLAG_PM_RUNTIME |
								   DL_FLAG_RPM_ACTIVE);
			if (!link) {
				dev_err(dev, "Failed to add device_link to %s.\n",
						values_of_power_domain_names[i]);
				return -EINVAL;
			}
		}
	}

	return 0;
}

/* xxx_boot_init must run before sunxi_rproc_init */
static int __init sunxi_rproc_e907_boot_init(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_register(RPROC_NAME, &sunxi_rproc_e907_ops, NULL);
	if (ret) {
		sunxi_err(NULL, "rproc("RPROC_NAME") register sunxi rproc ops failed, ret: %d\n", ret);
		return ret;
	}

	return 0;
}
subsys_initcall(sunxi_rproc_e907_boot_init);

static void __exit sunxi_rproc_e907_boot_exit(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_unregister(RPROC_NAME);
	if (ret)
		sunxi_err(NULL, "rproc("RPROC_NAME") unregister sunxi rproc ops failed, ret: %d\n", ret);
}
module_exit(sunxi_rproc_e907_boot_exit)

MODULE_DESCRIPTION("Allwinner sunxi rproc e907 boot driver");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.1");

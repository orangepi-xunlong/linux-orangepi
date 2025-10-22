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

/*
 * E906 CFG Register
 */
#define E906_VER_REG			(0x0000) /* E906 Version Register */
#define E906_RF1P_CFG_REG		(0x0010) /* E906 Control Register0 */
#define E906_TS_TMODE_SEL_REG		(0x0040) /* E906 TEST MODE SELETE Register */
#define E906_STA_ADD_REG		(0x0204) /* E906 STAT Register */
#define E906_WAKEUP_EN_REG		(0x0220) /* E906 WakeUp Enable Register */
#define E906_WAKEUP_MASK0_REG		(0x0224) /* E906 WakeUp Mask0 Register */
#define E906_WAKEUP_MASK1_REG		(0x0228) /* E906 WakeUp Mask1 Register */
#define E906_WAKEUP_MASK2_REG		(0x022C) /* E906 WakeUp Mask2 Register */
#define E906_WAKEUP_MASK3_REG		(0x0230) /* E906 WakeUp Mask3 Register */
#define E906_WAKEUP_MASK4_REG		(0x0234) /* E906 WakeUp Mask4 Register */
#define E906_WORK_MODE_REG		(0x0248) /* E906 Worke Mode Register */

/*
 * E906 Version Register
 */
#define SMALL_VER_MASK			(0x1f << 0)
#define LARGE_VER_MASK			(0x1f << 16)

/*
 * E906 PRID Register
 */
#define BIT_TEST_MODE			(1 << 1)

/*
 * E906 WakeUp Enable Register
 */
#define BIT_WAKEUP_EN			(1 << 0)

/*
 * E906 Worke Mode Register
 */
#define BIT_LOCK_STA			(1 << 3)
#define BIT_DEBUG_MODE			(1 << 2)
#define BIT_LOW_POWER_MASK		(0x3)
#define BIT_DEEP_SLEEP_MODE		(0x0)
#define BIT_LIGHT_SLEEP_MODE		(0x1)

/* E906 standby */
#ifdef CONFIG_AW_REMOTEPROC_E906_STANDBY
/*
 * message handler
 */
#define RPROC_STANDBY_E906_NAME		"e906"
#define STANDBY_MBOX_NAME		"arm-standby"
#define SUSPEND_TIMEOUT			msecs_to_jiffies(1000)
#define RESUME_TIMEOUT			msecs_to_jiffies(300)
#define E906_STANDBY_SUSPEND		(0xf3f30101)
#define E906_STANDBY_RESUME		(0xf3f30201)
#define PM_FINISH_FLAG			(0x1)
#define E906_SUSPEND_FINISH_FIELD	(0x16aa0000)

struct sunxi_standby_mbox {
	struct mbox_chan *chan;
	struct mbox_client client;
};

struct sunxi_e906_standby {
	void __iomem *rec_reg;
	uint32_t running;
	struct sunxi_standby_mbox mb;
	struct completion complete_ack;
	struct completion complete_dead;
	struct delayed_work pm_work;
	struct standby_memory_store memstore[];
};
#endif /* CONFIG_AW_REMOTEPROC_E906_STANDBY */

extern int simulator_debug;

static int sunxi_rproc_e906_assert(struct sunxi_rproc_priv *rproc_priv);
static int sunxi_rproc_e906_deassert(struct sunxi_rproc_priv *rproc_priv);
static int sunxi_rproc_e906_attach_pd(struct device *dev, const char *values_of_power_domain_names[], int count);

static int sunxi_rproc_e906_resource_get(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	struct sunxi_rproc_e906_cfg *e906_cfg;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct resource *res;
	u32 *map_array;
	int ret, i, count;
	const char *values_of_power_domain_names[16];

	rproc_priv->dev = dev;

	e906_cfg = devm_kzalloc(dev, sizeof(*e906_cfg), GFP_KERNEL);
	if (!e906_cfg) {
		dev_err(dev, "alloc e906 cfg error\n");
		return -ENOMEM;
	}

	e906_cfg->pubsram_clk = devm_clk_get(dev, "pubsram");
	if (IS_ERR_OR_NULL(e906_cfg->pubsram_clk)) {
		dev_err(dev, "no find pubsram in dts\n");
		return -ENXIO;
	}

	e906_cfg->mod_clk = devm_clk_get(dev, "mod");
	if (IS_ERR_OR_NULL(e906_cfg->mod_clk)) {
		dev_err(dev, "no find mod in dts\n");
		return -ENXIO;
	}

	e906_cfg->cfg_clk = devm_clk_get(dev, "cfg");
	if (IS_ERR_OR_NULL(e906_cfg->cfg_clk)) {
		dev_err(dev, "no find cfg in dts\n");
		return -ENXIO;
	}

	ret = of_property_match_string(np, "reset-names", "pubsram-rst");
	if (ret >= 0) {
		e906_cfg->pubsram_rst = devm_reset_control_get(dev, "pubsram-rst");
		if (IS_ERR_OR_NULL(e906_cfg->pubsram_rst)) {
			dev_err(dev, "no find pubsram-rst in dts\n");
			return -ENXIO;
		}
	}

	e906_cfg->mod_rst = devm_reset_control_get(dev, "mod-rst");
	if (IS_ERR_OR_NULL(e906_cfg->mod_rst)) {
		dev_err(dev, "no find mod-rst in dts\n");
		return -ENXIO;
	}

	e906_cfg->cfg_rst = devm_reset_control_get(dev, "cfg-rst");
	if (IS_ERR_OR_NULL(e906_cfg->cfg_rst)) {
		dev_err(dev, "no find cfg-rst in dts\n");
		return -ENXIO;
	}

	e906_cfg->dbg_rst = devm_reset_control_get(dev, "dbg-rst");
	if (IS_ERR_OR_NULL(e906_cfg->dbg_rst)) {
		dev_err(dev, "no find dbg-rst in dts\n");
		return -ENXIO;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "e906-cfg");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find e906-cfg in dts\n");
		return -ENXIO;
	}

	e906_cfg->e906_cfg = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(e906_cfg->e906_cfg)) {
		dev_err(dev, "fail to ioremap e906-cfg\n");
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

		ret = sunxi_rproc_e906_attach_pd(dev, values_of_power_domain_names, count);
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

	rproc_priv->rproc_cfg = e906_cfg;

	return 0;

disadle_pm:
	if (count > 0)
		pm_runtime_disable(dev);

	return ret;
}

static void sunxi_rproc_e906_resource_put(struct sunxi_rproc_priv *rproc_priv, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_disable(dev);
}

static int sunxi_rproc_e906_start(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e906_cfg *e906_cfg = rproc_priv->rproc_cfg;
	struct device *dev = rproc_priv->dev;
	int ret;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(dev, "%s,%d e906 does not need to reset clk\n",
				__func__, __LINE__);
		return 0;
	}

	ret = sunxi_rproc_e906_assert(rproc_priv);
	if (ret) {
		dev_err(dev, "rproc assert err\n");
		return ret;
	}

	ret = sunxi_rproc_e906_deassert(rproc_priv);
	if (ret) {
		dev_err(dev, "rproc deassert err\n");
		return ret;
	}

	ret = clk_prepare_enable(e906_cfg->cfg_clk);
	if (ret) {
		dev_err(dev, "cfg clk enable err\n");
		return ret;
	}

	/* set vector */
	writel(rproc_priv->pc_entry, (e906_cfg->e906_cfg + E906_STA_ADD_REG));

	ret = clk_prepare_enable(e906_cfg->mod_clk);
	if (ret) {
		dev_err(dev, "mod clk enable err\n");
		return ret;
	}

	return 0;
}

static int sunxi_rproc_e906_stop(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e906_cfg *e906_cfg = rproc_priv->rproc_cfg;
	int ret;

	dev_dbg(rproc_priv->dev, "%s,%d\n", __func__, __LINE__);

	if (simulator_debug) {
		dev_dbg(rproc_priv->dev, "%s,%d e906 does not need to close clk\n",
				__func__, __LINE__);
		return 0;
	}

	/* reset before turning off the clock for rv core */
	ret = reset_control_assert(e906_cfg->mod_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "mod rst assert err\n");
		return -ENXIO;
	}

	clk_disable_unprepare(e906_cfg->mod_clk);
	clk_disable_unprepare(e906_cfg->cfg_clk);

	ret = reset_control_assert(e906_cfg->cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "cfg rst assert err\n");
		return -ENXIO;
	}

	ret = reset_control_assert(e906_cfg->dbg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst assert err\n");
		return -ENXIO;
	}

	clk_disable_unprepare(e906_cfg->pubsram_clk);

	pm_runtime_put_sync(rproc_priv->dev);

	return 0;
}

static int sunxi_rproc_e906_attach(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e906_cfg *e906_cfg = rproc_priv->rproc_cfg;
	struct device *dev = rproc_priv->dev;
	int ret;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	pm_runtime_get_sync(dev);

	ret = clk_prepare_enable(e906_cfg->pubsram_clk);
	if (ret) {
		dev_err(rproc_priv->dev, "pubsram clk enable err\n");
		return ret;
	}

	ret = sunxi_rproc_e906_deassert(rproc_priv);
	if (ret) {
		dev_err(dev, "rproc deassert err\n");
		return ret;
	}

	ret = clk_prepare_enable(e906_cfg->cfg_clk);
	if (ret) {
		dev_err(dev, "cfg clk enable err\n");
		return ret;
	}

	ret = clk_prepare_enable(e906_cfg->mod_clk);
	if (ret) {
		dev_err(dev, "mod clk enable err\n");
		return ret;
	}

	return 0;
}

static int sunxi_rproc_e906_assert(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e906_cfg *e906_cfg = rproc_priv->rproc_cfg;
	int ret;

	ret = reset_control_assert(e906_cfg->mod_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "mod rst assert err\n");
		return -ENXIO;
	}

	ret = reset_control_assert(e906_cfg->cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "cfg rst assert err\n");
		return -ENXIO;
	}

	ret = reset_control_assert(e906_cfg->dbg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst assert err\n");
		return -ENXIO;
	}

	return ret;
}

static int sunxi_rproc_e906_deassert(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e906_cfg *e906_cfg = rproc_priv->rproc_cfg;
	int ret;

	ret = reset_control_deassert(e906_cfg->mod_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "mod rst de-assert err\n");
		return -ENXIO;
	}

	ret = reset_control_deassert(e906_cfg->cfg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "cfg rst de-assert err\n");
		return -ENXIO;
	}

	ret = reset_control_deassert(e906_cfg->dbg_rst);
	if (ret) {
		dev_err(rproc_priv->dev, "dbg rst de-assert err\n");
		return -ENXIO;
	}

	return ret;
}

static int sunxi_rproc_e906_reset(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;

	ret = sunxi_rproc_e906_assert(rproc_priv);
	if (ret)
		return -ENXIO;

	ret = sunxi_rproc_e906_deassert(rproc_priv);
	if (ret)
		return -ENXIO;

	return ret;
}

static int sunxi_rproc_e906_enable_sram(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	struct sunxi_rproc_e906_cfg *e906_cfg = rproc_priv->rproc_cfg;
	struct device *dev = rproc_priv->dev;
	int ret;

	if (value) {
		ret = of_property_match_string(rproc_priv->dev->of_node, "reset-names", "pubsram-rst");
		if (ret >= 0) {
			ret = reset_control_deassert(e906_cfg->pubsram_rst);
			if (ret) {
				dev_err(rproc_priv->dev, "pubsram reset err\n");
				return ret;
			}
		}

		/* must enable sram clk and power, so that arm can memcpy elf to sram */
		pm_runtime_get_sync(dev);
		ret = clk_prepare_enable(e906_cfg->pubsram_clk);
		if (ret) {
			dev_err(rproc_priv->dev, "pubsram clk enable err\n");
			return ret;
		}
	}

	return 0;
}

static int sunxi_rproc_e906_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	/* e906 do not have runstall reg bit */
	return 0;
}

static bool sunxi_rproc_e906_is_booted(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_e906_cfg *e906_cfg = rproc_priv->rproc_cfg;

	return __clk_is_enabled(e906_cfg->mod_clk);
}

static struct sunxi_rproc_ops sunxi_rproc_e906_ops = {
	.resource_get = sunxi_rproc_e906_resource_get,
	.resource_put = sunxi_rproc_e906_resource_put,
	.start = sunxi_rproc_e906_start,
	.stop = sunxi_rproc_e906_stop,
	.attach = sunxi_rproc_e906_attach,
	.reset = sunxi_rproc_e906_reset,
	.set_localram = sunxi_rproc_e906_enable_sram,
	.set_runstall = sunxi_rproc_e906_set_runstall,
	.is_booted = sunxi_rproc_e906_is_booted,
};

static int sunxi_rproc_e906_attach_pd(struct device *dev, const char *values_of_power_domain_names[], int count)
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

#ifdef CONFIG_AW_REMOTEPROC_E906_STANDBY
static void pm_work_func(struct work_struct *work)
{
	struct delayed_work *pm_work = to_delayed_work(work);
	struct sunxi_e906_standby *e906_standby;
	uint32_t reg_val;

	e906_standby = container_of(pm_work, struct sunxi_e906_standby, pm_work);

	reg_val = readl(e906_standby->rec_reg);
       if (reg_val == (E906_SUSPEND_FINISH_FIELD | PM_FINISH_FLAG)) {
		complete(&e906_standby->complete_dead);
       } else
		schedule_delayed_work(&e906_standby->pm_work, msecs_to_jiffies(50));
}

static void sunxi_rproc_e906_standby_rxcallback(struct mbox_client *cl, void *data)
{
	uint32_t rec_data = *(uint32_t *)data;
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct sunxi_rproc_standby *rproc_standby;
	struct sunxi_e906_standby *e906_standby;

	rproc_standby = sunxi_rproc_get_standby_handler(rproc->priv);
	if (!rproc_standby) {
		dev_err(cl->dev, "rx get standby resource failed\n");
		return;
	}
	e906_standby = rproc_standby->priv;

	switch (rec_data) {
	case E906_STANDBY_SUSPEND:
	case E906_STANDBY_RESUME:
		complete(&e906_standby->complete_ack);
		break;
	default:
		dev_warn(cl->dev, "rx unkown data(0x%08x)\n", rec_data);
		break;
	}
}

static int sunxi_rproc_e906_standby_request_mbox(struct sunxi_rproc_standby *rproc_standby)
{
	struct sunxi_e906_standby *e906_standby = rproc_standby->priv;
	struct device *dev = rproc_standby->dev;

	e906_standby->mb.client.rx_callback = sunxi_rproc_e906_standby_rxcallback;
	e906_standby->mb.client.tx_done = NULL;
	e906_standby->mb.client.tx_block = false;
	e906_standby->mb.client.dev = dev;

	e906_standby->mb.chan = mbox_request_channel_byname(&e906_standby->mb.client,
							STANDBY_MBOX_NAME);
	if (IS_ERR(e906_standby->mb.chan)) {
		if (PTR_ERR(e906_standby->mb.chan) == -EPROBE_DEFER)
		dev_err(dev, "standby get %s mbox failed\n", STANDBY_MBOX_NAME);
		e906_standby->mb.chan = NULL;
		return -EBUSY;
	}

	return 0;
}

static void sunxi_rproc_e906_standby_free_mbox(struct sunxi_rproc_standby *rproc_standby)
{
	struct sunxi_e906_standby *e906_standby = rproc_standby->priv;

	mbox_free_channel(e906_standby->mb.chan);
	e906_standby->mb.chan = NULL;
}

static int sunxi_rproc_e906_standby_init(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev)
{
	struct sunxi_e906_standby *e906_standby;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct of_phandle_iterator it;
	struct reserved_mem *rmem;
	uint32_t rec_reg;
	int ret;
	int index = 0;

	if (!rproc_standby || !rproc_standby->rproc_priv)
		return -EINVAL;

	if (of_property_read_u32(np, "standby-record-reg", &rec_reg)) {
		dev_err(dev, "failed to get standby-record-reg property\n");
		return -ENXIO;
	}

	if (of_get_property(np, "standby-ctrl-en", NULL)) {
		if (of_property_read_u32(np, "standby-ctrl-en", &rproc_standby->ctrl_en)) {
			dev_err(dev, "failed to get standby-ctrl-en property\n");
			return -ENXIO;
		}
	} else
		rproc_standby->ctrl_en = 0;

	if (!rproc_standby->ctrl_en) {
		rproc_standby->dev = dev;
		rproc_standby->priv = NULL;
		dev_warn(dev, "standby ctrl_en is 0\n");
		return 0;
	}

	rproc_standby->dev = dev;
	ret = sunxi_rproc_standby_init_prepare(rproc_standby);
	if (ret) {
		dev_err(dev, "standby prepare failed\n");
		return -EINVAL;
	}

	e906_standby = kzalloc(struct_size(e906_standby, memstore, rproc_standby->num_memstore), GFP_KERNEL);
	if (!e906_standby) {
		dev_err(dev, "alloc e906_standby failed\n");
		return -ENOMEM;
	}

	ret = of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	if (ret) {
		dev_err(dev, "fail to pemory-region iterator init for standby, return %d\n", ret);
		return -ENODEV;
	}

	while (of_phandle_iterator_next(&it) == 0) {
		if (of_get_property(it.node, "sleep-data-loss", NULL)) {
			rmem = of_reserved_mem_lookup(it.node);
			if (!rmem) {
				dev_err(dev, "unable to acquire memory-region for standby init\n");
				ret = -EINVAL;
				goto init_free_priv;
			}

			if (index < rproc_standby->num_memstore) {
				e906_standby->memstore[index].start = rmem->base;
				e906_standby->memstore[index].size = rmem->size;
			}
			index++;
		}
	}

	if (index != rproc_standby->num_memstore) {
		dev_err(dev, "standby num_memstore(%d) invalid, index: %d\n", rproc_standby->num_memstore, index);
		ret = -EINVAL;
		goto init_free_priv;
	}

	e906_standby->rec_reg = ioremap(rec_reg, 4);
	if (IS_ERR_OR_NULL(e906_standby->rec_reg)) {
		dev_err(dev, "fail to ioremap e906_standby rec_reg\n");
		ret = -ENXIO;
		goto init_free_priv;
	}

	if (e906_standby->running != 1)
		e906_standby->running = 0;
	rproc_standby->priv = e906_standby;

	ret = sunxi_rproc_e906_standby_request_mbox(rproc_standby);
	if (ret) {
		dev_err(dev, "request e906 standby mbox failed, return %d\n", ret);
		goto init_rec_unmap;
	}

	init_completion(&e906_standby->complete_ack);
	init_completion(&e906_standby->complete_dead);

	INIT_DELAYED_WORK(&e906_standby->pm_work, pm_work_func);

	dev_info(dev, "e906 standby init end\n");

	return 0;

init_rec_unmap:
	iounmap(e906_standby->rec_reg);

init_free_priv:
	rproc_standby->priv = NULL;
	kfree(e906_standby);
	rproc_standby->ctrl_en = 0;

	return ret;

}

static void sunxi_rproc_e906_standby_exit(struct sunxi_rproc_standby *rproc_standby, struct platform_device *pdev)
{
	struct sunxi_e906_standby *e906_standby;

	if (!rproc_standby || !rproc_standby->priv)
		return;

	e906_standby = rproc_standby->priv;

	if (rproc_standby->ctrl_en) {
		cancel_delayed_work_sync(&e906_standby->pm_work);

		reinit_completion(&e906_standby->complete_dead);
		reinit_completion(&e906_standby->complete_ack);

		sunxi_rproc_e906_standby_free_mbox(rproc_standby);

		iounmap(e906_standby->rec_reg);
	}

	rproc_standby->priv = NULL;
	kfree(e906_standby);
}

static int sunxi_rproc_e906_standby_start(struct sunxi_rproc_standby *rproc_standby)
{
	struct sunxi_e906_standby *e906_standby = rproc_standby->priv;

	e906_standby->running = 1;

	return 0;
}

static int sunxi_rproc_e906_standby_stop(struct sunxi_rproc_standby *rproc_standby)
{
	struct sunxi_e906_standby *e906_standby = rproc_standby->priv;

	e906_standby->running = 0;

	return 0;
}

static int sunxi_rproc_e906_standby_prepare(struct sunxi_rproc_standby *rproc_standby)
{
	int i;
	int ret;
	struct sunxi_e906_standby *e906_standby = rproc_standby->priv;
	struct device *dev = rproc_standby->dev;
	struct standby_memory_store *store;

	if (!e906_standby->running)
		return 0;

	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e906_standby->memstore[i];
		store->iomap = ioremap(store->start, store->size);
		if (!store->iomap) {
			dev_err(dev, "store ioremap failed, index: %d\n", i);
			ret = -ENOMEM;
			goto data_store_free;
		}

		store->buf = vmalloc(store->size);
		if (!store->buf) {
			dev_err(dev, "store vmalloc failed, index: %d\n", i);
			ret = -ENOMEM;
			goto data_store_free;
		}
	}

	return 0;

data_store_free:
	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e906_standby->memstore[i];
		if (store->iomap) {
			iounmap(store->iomap);
			store->iomap = NULL;
		}

		if (store->buf) {
			vfree(store->buf);
			store->buf = NULL;
		}
	}

	return ret;
}

static void sunxi_rproc_e906_standby_complete(struct sunxi_rproc_standby *rproc_standby)
{
	int i;
	struct sunxi_e906_standby *e906_standby = rproc_standby->priv;
	struct standby_memory_store *store;

	if (!e906_standby->running)
		return;

	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e906_standby->memstore[i];
		if (store->iomap) {
			iounmap(store->iomap);
			store->iomap = NULL;
		}

		if (store->buf) {
			vfree(store->buf);
			store->buf = NULL;
		}
	}
}

static int sunxi_rproc_e906_standby_suspend(struct sunxi_rproc_standby *rproc_standby)
{
	int ret;
	int i;
	uint32_t data = E906_STANDBY_SUSPEND;
	struct sunxi_e906_standby *e906_standby = rproc_standby->priv;
	struct sunxi_rproc_e906_cfg *e906_cfg = rproc_standby->rproc_priv->rproc_cfg;
	struct standby_memory_store *store;

	if (!e906_standby->running || !sunxi_rproc_e906_is_booted(rproc_standby->rproc_priv))
		return 0;

	ret = mbox_send_message(e906_standby->mb.chan, &data);
	if (ret < 0) {
		dev_err(rproc_standby->dev, "suspend send mbox msg failed\n");
		return ret;
	}

	ret = wait_for_completion_timeout(&e906_standby->complete_ack, SUSPEND_TIMEOUT);
	if (!ret) {
		dev_err(rproc_standby->dev, "timeout return suspend ack\n");
		return -EBUSY;
	}

	/* check whether e906 completes the suspend process */
	schedule_delayed_work(&e906_standby->pm_work, msecs_to_jiffies(50));
	ret = wait_for_completion_timeout(&e906_standby->complete_dead, SUSPEND_TIMEOUT);
	cancel_delayed_work_sync(&e906_standby->pm_work);
	if (!ret) {
		dev_err(rproc_standby->dev, "timeout return suspend dead\n");
		writel(E906_SUSPEND_FINISH_FIELD & ~(PM_FINISH_FLAG), e906_standby->rec_reg);
		return -EBUSY;
	}
	msleep(1);

	/* stop clk/rst to avoid restarting immediately after power-on */
	clk_disable_unprepare(e906_cfg->mod_clk);
	ret = reset_control_assert(e906_cfg->dbg_rst);
	if (ret) {
		dev_err(rproc_standby->dev, "dbg rst assert err\n");
		return ret;
	}
	ret = reset_control_assert(e906_cfg->mod_rst);
	if (ret) {
		dev_err(rproc_standby->dev, "mod rst assert err\n");
		return ret;
	}
	clk_disable_unprepare(e906_cfg->cfg_clk);
	ret = reset_control_assert(e906_cfg->cfg_rst);
	if (ret) {
		dev_err(rproc_standby->dev, "cfg rst assert err\n");
		return ret;
	}
	/* FIXME: restore if failed
	 * The heartbeat packet is expected to restart the system.
	 */

	/* store data */
	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e906_standby->memstore[i];
		memcpy_fromio(store->buf, store->iomap, store->size);
	}

	return 0;
}

static int sunxi_rproc_e906_standby_resume(struct sunxi_rproc_standby *rproc_standby)
{
	int ret;
	int i;
	uint32_t reg_val;
	uint32_t data = E906_STANDBY_RESUME;
	struct sunxi_e906_standby *e906_standby = rproc_standby->priv;
	struct sunxi_rproc_e906_cfg *e906_cfg = rproc_standby->rproc_priv->rproc_cfg;
	struct standby_memory_store *store;

	if (!e906_standby->running)
		return 0;

	/* restore data */
	for (i = 0; i < rproc_standby->num_memstore; i++) {
		store = &e906_standby->memstore[i];
		memcpy_toio(store->iomap, store->buf, store->size);
	}

	if (!sunxi_rproc_e906_is_booted(rproc_standby->rproc_priv)) {
		ret = reset_control_deassert(e906_cfg->pubsram_rst);
		if (ret) {
			dev_err(rproc_standby->dev, "pubsram reset err\n");
			return ret;
		}

		ret = clk_prepare_enable(e906_cfg->pubsram_clk);
		if (ret) {
			dev_err(rproc_standby->dev, "pubsram clk enable err\n");
			return ret;
		}

		ret = reset_control_deassert(e906_cfg->cfg_rst);
		if (ret) {
			dev_err(rproc_standby->dev, "cfg rst deassert err\n");
			return ret;
		}

		ret = clk_prepare_enable(e906_cfg->cfg_clk);
		if (ret) {
			dev_err(rproc_standby->dev, "cfg clk enable err\n");
			return ret;
		}

		/* set vector */
		writel(rproc_standby->rproc_priv->pc_entry, (e906_cfg->e906_cfg + E906_STA_ADD_REG));

		ret = reset_control_deassert(e906_cfg->mod_rst);
		if (ret) {
			dev_err(rproc_standby->dev, "mod rst deassert err\n");
			return ret;
		}

		ret = reset_control_deassert(e906_cfg->dbg_rst);
		if (ret) {
			dev_err(rproc_standby->dev, "dbg rst deassert err\n");
			return ret;
		}

		ret = clk_prepare_enable(e906_cfg->mod_clk);
		if (ret) {
			dev_err(rproc_standby->dev, "mod clk enable err\n");
			return ret;
		}
	}

	reg_val = readl(e906_standby->rec_reg);
	if ((reg_val & 0xffff0000) == E906_SUSPEND_FINISH_FIELD) {
		/* Note the asynchronous rewrite, the register is only used for one core */
		writel(reg_val & ~(PM_FINISH_FLAG), e906_standby->rec_reg);
	}

	ret = mbox_send_message(e906_standby->mb.chan, &data);
	if (ret < 0) {
		dev_err(rproc_standby->dev, "resume send mbox msg failed\n");
		return ret;
	}

	ret = wait_for_completion_timeout(&e906_standby->complete_ack, SUSPEND_TIMEOUT);
	if (!ret) {
		dev_err(rproc_standby->dev, "timeout return resume ack\n");
		return -EBUSY;
	}

	return 0;
}

static struct sunxi_rproc_standby_ops rproc_e906_standby_ops = {
	.init = sunxi_rproc_e906_standby_init,
	.exit = sunxi_rproc_e906_standby_exit,
	.attach = sunxi_rproc_e906_standby_start,
	.detach = sunxi_rproc_e906_standby_stop,
	.start = sunxi_rproc_e906_standby_start,
	.stop = sunxi_rproc_e906_standby_stop,
	.prepare = sunxi_rproc_e906_standby_prepare,
	.suspend = sunxi_rproc_e906_standby_suspend,
	.resume = sunxi_rproc_e906_standby_resume,
	.complete = sunxi_rproc_e906_standby_complete,
};
#endif /* CONFIG_AW_REMOTEPROC_E906_STANDBY */

/* e906_boot_init must run before sunxi_rproc probe */
static int __init sunxi_rproc_e906_boot_init(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_register("e906", &sunxi_rproc_e906_ops, NULL);
	if (ret) {
		pr_err("e906 register ops failed\n");
		return ret;
	}

#ifdef CONFIG_AW_REMOTEPROC_E906_STANDBY
	ret = sunxi_rproc_standby_ops_register(RPROC_STANDBY_E906_NAME, &rproc_e906_standby_ops, NULL);
	if (ret) {
		pr_err("e906 registers standby ops failed\n");
		return ret;
	}
#endif

	return 0;
}
subsys_initcall(sunxi_rproc_e906_boot_init);

static void __exit sunxi_rproc_e906_boot_exit(void)
{
	int ret;

#ifdef CONFIG_AW_REMOTEPROC_E906_STANDBY
	ret = sunxi_rproc_standby_ops_unregister(RPROC_STANDBY_E906_NAME);
	if (ret)
		pr_err("e906 unregister standby ops failed\n");
#endif

	ret = sunxi_rproc_priv_ops_unregister("e906");
	if (ret)
		pr_err("e906 unregister ops failed\n");
}
module_exit(sunxi_rproc_e906_boot_exit)

MODULE_DESCRIPTION("Allwinner sunxi rproc e906 boot driver");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.6");

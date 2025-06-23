// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.

#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mailbox_client.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/remoteproc.h>
#include <linux/reset.h>
#include <linux/workqueue.h>
#include <linux/pm_wakeirq.h>
#include <linux/pm_runtime.h>

#include "remoteproc_internal.h"
#include "remoteproc_elf_helpers.h"

#ifdef CONFIG_PLAT_BBOX
#include <linux/soc/cix/rdr_pub.h>
#include <mntn_public_interface.h>
#endif

/* DSP register define */
#define SKY1_INFO_HIFI0				0x00
#define SKY1_INFO_HIFI1				0x04
#define SKY1_INFO_HIFI2				0x08

/* info hifi0 */
#define SKY1_INFO_HIFI0_OCD_HALTONRST		BIT(1)
#define SKY1_INFO_HIFI0_CLK_EN			BIT(0)
/* info hifi1 */
#define SKY1_INFO_HIFI1_ALTER_RST_VECTOR	GENMASK(31, 0)
#define SKY1_INFO_HIFI1_SEL_VECTOR		BIT(0)
/* info hifi2 */
#define SKY1_INFO_HIFI2_NMI			BIT(2)
#define SKY1_INFO_HIFI2_RUN_STALL		BIT(1)
#define SKY1_INFO_HIFI2_PWAIT_MODE		BIT(0)

#define CIX_AUD_CLK_NUM				(6)
#define CIX_MEM_REG_NUM				(8)
#define PM_COMP_TIMEOUT				(100)
#define RPROC_READY_WAIT_MAX_CNT		(3000)
#define MBOX_SEND_TIMEOUT			(100)
#define MBOX_MSG_OFFSET				(1)
#define MBOX_MSG_LEN				(2)

/**
 * struct cix_dsp_mem - internal memory structure
 * @cpu_addr: Virtual address of the memory region
 * @bus_addr: Bus address used to access the memory region
 * @size: Size of the memory region
 */
struct cix_dsp_mem {
	void __iomem *cpu_addr;
	phys_addr_t bus_addr;
	size_t size;
};

/**
 * struct cix_mem_region - memory region structure
 * @name: Name of the memory region
 * @da: Device address of the memory region from DSP view
 * @sa: System Bus address used to access the memory region
 * @len: Length of the memory region
 */
struct cix_mem_region {
	const char *name;
	u32 da;
	u64 sa;
	u32 len;
};

static const struct cix_mem_region sky1_dsp_mems[CIX_MEM_REG_NUM] = {
	/* 128K iram */
	{ .name = "iram", .da = 0x20200000, .sa = 0x1C80200000, .len = 0x20000 },
	/* 128K dram0 */
	{ .name = "dram0", .da = 0x20220000, .sa = 0x1C80220000, .len = 0x20000 },
	/* 128K dram1 */
	{ .name = "dram1", .da = 0x20240000, .sa = 0x1C80240000, .len = 0x20000 },
	/* 1M extern sram */
	{ .name = "ext-sram", .da = 0x20400000, .sa = 0x1C80400000, .len = 0x100000 },
	/* 16K vdev0vring0 */
	{ .name = "vdev0vring0", .da = 0x3de00000, .sa = 0xcde00000, .len = 0x4000 },
	/* 16K vdev0vring1 */
	{ .name = "vdev0vring1", .da = 0x3de04000, .sa = 0xcde04000, .len = 0x4000 },
	/* 1M vdev0buffer */
	{ .name = "vdev0buffer", .da = 0x3de08000, .sa = 0xcde08000, .len = 0x100000 },
	/* 16M dsp_reserved */
	{ .name = "dsp_reserved", .da = 0x3e000000, .sa = 0xce000000, .len = 0x1000000 },
};

struct cix_dsp_rproc {
	struct device *dev;
	struct regmap *regmap;
	struct rproc *rproc;
	struct reset_control *dsp_rst;
	struct reset_control *mb0_rst;
	struct reset_control *mb1_rst;
	struct clk *clks[CIX_AUD_CLK_NUM];
	int wdg_irq;
	int num_mem;
	struct cix_dsp_mem *mem;
	struct cix_mem_region *mem_region;
	struct mbox_client cl;
	struct mbox_chan *tx_ch;
	struct mbox_chan *rx_ch;
	struct work_struct rproc_work;
	struct workqueue_struct *workqueue;
	struct completion rsp_comp;
	bool rproc_ready;
	bool is_resume_back;
	bool is_wdg_trigger;
};

enum cix_dsp_mbox_messages {
	MBOX_MSG_REPROC_READY	= 0xFFF0,
	MBOX_MSG_REPROC_CRASH	= 0xFFF1,
	MBOX_MSG_SYSTEM_SUSPEND	= 0xFFF2,
	MBOX_MSG_SUSPEND_ACK	= 0xFFF3,
	MBOX_MSG_SYSTEM_RESUME	= 0xFFF4,
	MBOX_MSG_RESUME_ACK	= 0xFFF5,
	MBOX_MSG_REPROC_STOP	= 0xFFF6,
	MBOX_MSG_REPROC_STOP_ACK= 0xFFF7,
};

#ifdef CONFIG_PLAT_BBOX
enum RDR_AUDIO_MODID {
	RDR_AUDIO_MODID_START = PLAT_BB_MOD_HIFI_START,
	RDR_AUDIO_SOC_WD_TIMEOUT_MODID,
	RDR_AUDIO_MODID_END = PLAT_BB_MOD_HIFI_END,
};

static struct rdr_register_module_result g_current_info;

static struct rdr_exception_info_s g_dsp_einfo[] = {
	{ { 0, 0 }, RDR_AUDIO_SOC_WD_TIMEOUT_MODID, RDR_AUDIO_SOC_WD_TIMEOUT_MODID, RDR_ERR,
	 RDR_REBOOT_NO, RDR_HIFI, RDR_HIFI, RDR_HIFI,
	 (u32)RDR_REENTRANT_DISALLOW, AUDIO_CODEC_EXCEPTION, 0, (u32)RDR_UPLOAD_YES,
	 "audio dsp", "audio dsp watchdog timeout", 0, 0, 0 },
};

struct rdr_dump_mem_region {
	const char *name;
	void *va;
	u32 length;
};
#define RDR_DUMP_MEM_REGION_0_SIZE	5
#define RDR_DUMP_MEM_REGION_1_SIZE	3
static struct rdr_dump_mem_region g_rdr_dump_mem_region_0[RDR_DUMP_MEM_REGION_0_SIZE],
				  g_rdr_dump_mem_region_1[RDR_DUMP_MEM_REGION_1_SIZE];
static unsigned int g_rdr_dump_mem_region_index_0,
		    g_rdr_dump_mem_region_index_1;

#define RDR_DUMP_COMP_TIMEOUT		500
static struct completion g_rdr_dump_comp;

/*
 * Description : Dump function of the AP when an exception occurs
 */
static void cix_dsp_rproc_rdr_dump(u32 modid, u32 etype,
				   u64 coreid, char *log_path,
				   pfn_cb_dump_done pfn_cb)
{
	unsigned int i;
	int ret;

	/* iram, dram0, dram1, ext-sram, dsp_reserved */
	for (i = 0; i < g_rdr_dump_mem_region_index_0; i++) {
		if (g_rdr_dump_mem_region_0[i].va) {
			ret = rdr_savebuf2fs(log_path, g_rdr_dump_mem_region_0[i].name,
					     g_rdr_dump_mem_region_0[i].va,
					     g_rdr_dump_mem_region_0[i].length, 0);
			if (ret < 0) {
				pr_err("rdr_savelogbuf2fs region_0: name = %s, error = %d\n",
				       g_rdr_dump_mem_region_0[i].name, ret);
				return;
			}

			pr_debug("dump to file successed: path = %s, name = %s, len = 0x%x\n",
				 log_path, g_rdr_dump_mem_region_0[i].name, g_rdr_dump_mem_region_0[i].length);
		}
	}

	/* vdev0vring0, vdev0vring1, vdev0buffer */
	for (i = 0; i < g_rdr_dump_mem_region_index_1; i++) {
		if (g_rdr_dump_mem_region_1[i].va) {
			ret = rdr_savebuf2fs(log_path, g_rdr_dump_mem_region_1[i].name,
					     g_rdr_dump_mem_region_1[i].va,
					     g_rdr_dump_mem_region_1[i].length, 0);
			if (ret < 0) {
				pr_err("rdr_savelogbuf2fs region_1: name = %s, error = %d\n",
				       g_rdr_dump_mem_region_1[i].name, ret);
				return;
			}

			pr_debug("dump to file successed: path = %s, name = %s, len = 0x%x\n",
				 log_path, g_rdr_dump_mem_region_1[i].name, g_rdr_dump_mem_region_1[i].length);
		}
	}

	if (pfn_cb)
		pfn_cb(modid, coreid);

	complete(&g_rdr_dump_comp);
}

/*
 * Description : register exception with the rdr
 */
static void cix_dsp_rproc_rdr_register_exception(void)
{
	unsigned int i;
	int ret;

	for (i = 0; i < sizeof(g_dsp_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("register exception:%u", g_dsp_einfo[i].e_exce_type);
		ret = rdr_register_exception(&g_dsp_einfo[i]);
		if (ret == 0) {
			pr_err("rdr_register_exception fail, ret = [%d]\n", ret);
			return;
		}
	}
}

static void cix_dsp_rproc_rdr_unregister_exception(void)
{
	unsigned int i;

	for (i = 0; i < sizeof(g_dsp_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("unregister exception:%u", g_dsp_einfo[i].e_exce_type);
		rdr_unregister_exception(g_dsp_einfo[i].e_modid);
	}
}

/*
 * Description : Register the dump and reset functions to the rdr
 */
static int cix_dsp_rproc_rdr_register_core(void)
{
	struct rdr_module_ops_pub s_dsp_ops;
	struct rdr_register_module_result retinfo;
	u64 coreid = RDR_HIFI;
	int ret;

	s_dsp_ops.ops_dump = cix_dsp_rproc_rdr_dump;
	s_dsp_ops.ops_reset = NULL;

	ret = rdr_register_module_ops(coreid, &s_dsp_ops, &retinfo);
	if (ret < 0) {
		pr_err("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	g_current_info.log_addr = retinfo.log_addr;
	g_current_info.log_len = retinfo.log_len;
	g_current_info.nve = retinfo.nve;
	pr_debug("%s,%d: addr=0x%llx, len=0x%x\n",
		__func__, __LINE__, g_current_info.log_addr, g_current_info.log_len);

	return ret;
}

static void cix_dsp_rproc_rdr_unregister_core(void)
{
	u64 coreid = RDR_HIFI;

	rdr_unregister_module_ops(coreid);
}

static void cix_dsp_rproc_rdr_coredump(struct rproc *rproc)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	struct device *dev = rproc_priv->dev;
	struct rproc_mem_entry *entry, *tmp;

	g_rdr_dump_mem_region_index_1 = 0;

	list_for_each_entry_safe(entry, tmp, &rproc->carveouts, node) {
		g_rdr_dump_mem_region_1[g_rdr_dump_mem_region_index_1].name = entry->name;
		g_rdr_dump_mem_region_1[g_rdr_dump_mem_region_index_1].va = entry->va;
		g_rdr_dump_mem_region_1[g_rdr_dump_mem_region_index_1].length = entry->len;

		dev_dbg(dev, "rdr_mem_region_1: index = %d, name = %s, va = %px, len = %lu\n",
			g_rdr_dump_mem_region_index_1, entry->name, entry->va, entry->len);

		g_rdr_dump_mem_region_index_1++;
	}

	reinit_completion(&g_rdr_dump_comp);

	/* asynchronous api */
	rdr_system_error(RDR_AUDIO_SOC_WD_TIMEOUT_MODID, 0, 0);

	if (!wait_for_completion_timeout(&g_rdr_dump_comp,
					 msecs_to_jiffies(RDR_DUMP_COMP_TIMEOUT)))
		dev_err(dev, "rdr dump file timeout");
}
#endif

static int cix_dsp_rproc_mem_alloc(struct rproc *rproc,
				 struct rproc_mem_entry *mem)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	struct device *dev = rproc_priv->dev;
	void *va;

	dev_dbg(dev, "map memory: %pa+%lx\n", &mem->dma, mem->len);
	va = ioremap_wc(mem->dma, mem->len);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "Unable to map memory region: %pa+%lx\n",
			&mem->dma, mem->len);
		return -ENOMEM;
	}

	/* Update memory entry va */
	mem->va = va;

	return 0;
}

static int cix_dsp_rproc_mem_release(struct rproc *rproc,
				   struct rproc_mem_entry *mem)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;

	dev_dbg(rproc_priv->dev, "unmap memory: %pa\n", &mem->dma);
	iounmap(mem->va);

	return 0;
}

static int cix_dsp_rproc_da_to_sa(struct cix_dsp_rproc *rproc_priv, u64 da, size_t len, u64 *sa)
{
	struct cix_mem_region *mem_reg = rproc_priv->mem_region;
	int i;

	for (i = 0; i < CIX_MEM_REG_NUM; i++) {
		if (da >= mem_reg[i].da
		    && da + len <= mem_reg[i].da + mem_reg[i].len) {
			unsigned int offset = da - mem_reg[i].da;

			*sa = mem_reg[i].sa + offset;
			return 0;
		}
	}

	return -ENOENT;
}

static int cix_dsp_rproc_parse_memory_regions(struct rproc *rproc)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	struct cix_mem_region *mem_reg = rproc_priv->mem_region;
	struct device *dev = rproc_priv->dev;
	struct rproc_mem_entry *mem;
	int i;

	dev_dbg(dev, "%s\n", __func__);

	for (i = 0; i < CIX_MEM_REG_NUM; i++) {
		if (!strcmp(mem_reg[i].name, "vdev0buffer")) {
			mem = rproc_mem_entry_init(dev, NULL,
						   (dma_addr_t)mem_reg[i].sa,
						   mem_reg[i].len, mem_reg[i].da,
						   NULL,
						   NULL,
						   mem_reg[i].name);
		} else if (!strcmp(mem_reg[i].name, "vdev0vring0") ||
			   !strcmp(mem_reg[i].name, "vdev0vring1")) {
			mem = rproc_mem_entry_init(dev, NULL,
						   (dma_addr_t)mem_reg[i].sa,
						   mem_reg[i].len, mem_reg[i].da,
						   cix_dsp_rproc_mem_alloc,
						   cix_dsp_rproc_mem_release,
						   mem_reg[i].name);
		} else {
			continue;
		}

		if (!mem)
			return -ENOMEM;

		dev_dbg(dev, "add carveout %s, base 0x%llx size 0x%x\n",
			mem_reg[i].name, mem_reg[i].sa, mem_reg[i].len);

		rproc_add_carveout(rproc, mem);
	}

	return 0;
}

static const char *cix_dsp_clk_names[CIX_AUD_CLK_NUM] = {
	/* DSP clocks */
	"clk", "bclk", "pbclk",

	/* Peripheral clocks */
	"sramclk", "mb0clk", "mb1clk",
};

static int cix_dsp_rproc_clks_get(struct cix_dsp_rproc *rproc_priv)
{
	int i;

	for (i = 0; i < CIX_AUD_CLK_NUM; i++) {
		rproc_priv->clks[i] = devm_clk_get(rproc_priv->dev,
						   cix_dsp_clk_names[i]);
		if (IS_ERR(rproc_priv->clks[i])) {
			dev_err(rproc_priv->dev, "failed to get clock %s\n",
				cix_dsp_clk_names[i]);
			return PTR_ERR(rproc_priv->clks[i]);
		}
	}

	return 0;
}

static void cix_dsp_rproc_stall(struct cix_dsp_rproc *rproc_priv)
{
	/* stall DSP core */
	regmap_update_bits(rproc_priv->regmap, SKY1_INFO_HIFI2,
			   SKY1_INFO_HIFI2_RUN_STALL, SKY1_INFO_HIFI2_RUN_STALL);
}

static void cix_dsp_rproc_run(struct cix_dsp_rproc *rproc_priv)
{
	/* reset DSP core */
	reset_control_assert(rproc_priv->dsp_rst);

	usleep_range(1, 2);

	/* reset DSP core */
	reset_control_deassert(rproc_priv->dsp_rst);

	/* run DSP core */
	regmap_update_bits(rproc_priv->regmap, SKY1_INFO_HIFI2,
			   SKY1_INFO_HIFI2_RUN_STALL, 0);
}

static void cix_dsp_rproc_peri_reset(struct cix_dsp_rproc *rproc_priv)
{
	/* reset peripheral */
	reset_control_assert(rproc_priv->mb0_rst);
	reset_control_assert(rproc_priv->mb1_rst);

	usleep_range(1, 2);

	/* release peripheral reset */
	reset_control_deassert(rproc_priv->mb0_rst);
	reset_control_deassert(rproc_priv->mb1_rst);
}

static int cix_dsp_rproc_clks_enable(struct cix_dsp_rproc *rproc_priv)
{
	int i, err;

	for (i = 0; i < CIX_AUD_CLK_NUM; i++) {
		err = clk_prepare_enable(rproc_priv->clks[i]);
		if (err) {
			dev_err(rproc_priv->dev, "failed to enable clock %s\n",
				cix_dsp_clk_names[i]);
			goto err_dsp_clks;
		}
	}

	return 0;

err_dsp_clks:
	while (--i >= 0)
		clk_disable_unprepare(rproc_priv->clks[i]);

	return err;
}

static void cix_dsp_rproc_clks_disable(struct cix_dsp_rproc *rproc_priv)
{
	int i;

	for (i = 0; i < CIX_AUD_CLK_NUM; i++)
		clk_disable_unprepare(rproc_priv->clks[i]);
}

static void cix_dsp_rproc_set_vector_base(struct cix_dsp_rproc *rproc_priv,
				    u32 vector_addr)
{
	/* reset DSP core */
	reset_control_assert(rproc_priv->dsp_rst);

	/* keep reset asserted for 10 cycles */
	usleep_range(1, 2);

	/* keep DSP stalled for FW loading */
	cix_dsp_rproc_stall(rproc_priv);

	/* DSP must in reset when select alternative reset vector address */
	regmap_update_bits(rproc_priv->regmap, SKY1_INFO_HIFI1,
			   SKY1_INFO_HIFI1_ALTER_RST_VECTOR, vector_addr);

	regmap_update_bits(rproc_priv->regmap, SKY1_INFO_HIFI1,
			   SKY1_INFO_HIFI1_SEL_VECTOR, SKY1_INFO_HIFI1_SEL_VECTOR);

	/*
	 * The value must be stable for at least 10 clock cycles before and
	 * after the reset signal is released.
	 */
	usleep_range(1, 2);

	/* release DSP core */
	reset_control_deassert(rproc_priv->dsp_rst);
}

static int cix_dsp_rproc_prepare(struct rproc *rproc)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	int err;

	err = cix_dsp_rproc_parse_memory_regions(rproc);
	if (err)
		return err;

	pm_runtime_get_sync(rproc_priv->dev);

	return 0;
}

static int cix_dsp_rproc_unprepare(struct rproc *rproc)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;

	pm_runtime_put_sync(rproc_priv->dev);

	return 0;
}

static int cix_dsp_rproc_start(struct rproc *rproc)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	int i;

	if (!rproc_priv->rx_ch)
		return 0;

	rproc_priv->is_wdg_trigger = false;
	rproc_priv->rproc_ready = false;

	cix_dsp_rproc_run(rproc_priv);

	for (i = 0; i < RPROC_READY_WAIT_MAX_CNT; i++) {
		if (rproc_priv->rproc_ready == true) {
			/* Need DSP clear interrupt firstly when recover */
			enable_irq(rproc_priv->wdg_irq);
			return 0;
		}
		usleep_range(200, 500);
	}

	return -ETIMEDOUT;
}

static void cix_dsp_mbox_dump_regs(struct rproc *rproc)
{
	void __iomem *base;
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	int i;
	u32 val;
#define RCSU_MBOX_BASE		(0x070f0000)
#define RCSU_MBOX_REG_SIZE	(0x10000)
#define MBOX_RGE_DUMP_NUM	(7)
#define MBOX_REG_OFFSET		(0xb4)

	base = ioremap(RCSU_MBOX_BASE, RCSU_MBOX_REG_SIZE);
	for (i = 0; i < MBOX_RGE_DUMP_NUM; i++) {
		val = readl(base + MBOX_REG_OFFSET + 4*i);
		dev_err(rproc_priv->dev, "[0x%x]: 0x%x\n",
			MBOX_REG_OFFSET + 4*i, val);
	}
	iounmap(base);
}

static int cix_dsp_rproc_stop(struct rproc *rproc)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	u32 msg[MBOX_MSG_LEN];
	int ret;

	/*
	 * If dsp wdg triggered to ap, means that dsp crashed,
	 * no ability to ack stop message.
	 */
	if (!rproc_priv->is_wdg_trigger) {
		msg[0] = MBOX_MSG_LEN;
		msg[MBOX_MSG_OFFSET] = MBOX_MSG_REPROC_STOP;
		reinit_completion(&rproc_priv->rsp_comp);
		ret = mbox_send_message(rproc_priv->tx_ch, (void *)&msg);
		if (ret < 0) {
			cix_dsp_mbox_dump_regs(rproc);
			dev_err(rproc_priv->dev, "PM mbox_send_message failed: %d\n", ret);
			return ret;
		}
		if (!wait_for_completion_timeout(&rproc_priv->rsp_comp,
						 msecs_to_jiffies(PM_COMP_TIMEOUT)))
			return -EBUSY;
	}

	cix_dsp_rproc_stall(rproc_priv);

	rproc_priv->rproc_ready = false;

	return 0;
}

static void cix_dsp_rproc_kick(struct rproc *rproc, int vqid)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	int err;
	u32 msg[MBOX_MSG_LEN];

	if (!rproc_priv->tx_ch) {
		dev_err(rproc_priv->dev, "No initialized mbox tx channel\n");
		return;
	}
	msg[0] = MBOX_MSG_LEN;
	msg[MBOX_MSG_OFFSET] = vqid;

	err = mbox_send_message(rproc_priv->tx_ch, (void *)&msg);
	if (err < 0) {
		cix_dsp_mbox_dump_regs(rproc);
		dev_err(rproc_priv->dev, "%s: failed (%d, err:%d)\n",
			__func__, vqid, err);
	}
}

static void *cix_dsp_rproc_da_to_va(struct rproc *rproc, u64 da, size_t len, bool *is_iomem)
{
	struct cix_dsp_rproc *priv = rproc->priv;
	void *va = NULL;
	u64 pa;
	int i;

	if (len == 0)
		return NULL;

	if (cix_dsp_rproc_da_to_sa(priv, da, len, &pa))
		return NULL;

	for (i = 0; i < priv->num_mem; i++) {
		if (pa >= priv->mem[i].bus_addr
		    && pa + len <= priv->mem[i].bus_addr +  priv->mem[i].size) {
			unsigned int offset = pa - priv->mem[i].bus_addr;

			va = priv->mem[i].cpu_addr + offset;
			break;
		}
	}

	dev_dbg(priv->dev, "da = 0x%llx len = 0x%zx va = 0x%px\n",
		da, len, va);

	return va;
}

static int cix_dsp_rproc_elf_load_segments(struct rproc *rproc, const struct firmware *fw)
{
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	struct device *dev = &rproc->dev;
	const void *ehdr, *phdr;
	int i, ret = 0;
	u16 phnum;
	const u8 *elf_data = fw->data;
	u8 class = fw_elf_get_class(fw);
	u32 elf_phdr_get_size = elf_size_of_phdr(class);

	/* set reset vector address */
	cix_dsp_rproc_set_vector_base(rproc_priv, (u32)rproc->bootaddr);

	if (!rproc_priv->is_resume_back) {
		/* clear buffers */
		for (i = 0; i < CIX_MEM_REG_NUM; i++)
			if (rproc_priv->mem[i].cpu_addr)
				memset(rproc_priv->mem[i].cpu_addr, 0, rproc_priv->mem[i].size);
	}
	rproc_priv->is_resume_back = false;

	ehdr = elf_data;
	phnum = elf_hdr_get_e_phnum(class, ehdr);
	phdr = elf_data + elf_hdr_get_e_phoff(class, ehdr);

	/* go through the available ELF segments */
	for (i = 0; i < phnum; i++, phdr += elf_phdr_get_size) {
		u64 da = elf_phdr_get_p_paddr(class, phdr);
		u64 memsz = elf_phdr_get_p_memsz(class, phdr);
		u64 filesz = elf_phdr_get_p_filesz(class, phdr);
		u64 offset = elf_phdr_get_p_offset(class, phdr);
		u32 type = elf_phdr_get_p_type(class, phdr);
		void *ptr;

		if (type != PT_LOAD || !memsz)
			continue;

		dev_dbg(dev, "phdr: type %d da 0x%llx memsz 0x%llx filesz 0x%llx\n",
			type, da, memsz, filesz);

		if (filesz > memsz) {
			dev_err(dev, "bad phdr filesz 0x%llx memsz 0x%llx\n",
				filesz, memsz);
			ret = -EINVAL;
			break;
		}

		if (offset + filesz > fw->size) {
			dev_err(dev, "truncated fw: need 0x%llx avail 0x%zx\n",
				offset + filesz, fw->size);
			ret = -EINVAL;
			break;
		}

		if (!rproc_u64_fit_in_size_t(memsz)) {
			dev_err(dev, "size (%llx) does not fit in size_t type\n",
				memsz);
			ret = -EOVERFLOW;
			break;
		}

		/* grab the kernel address for this device address */
		ptr = rproc_da_to_va(rproc, da, memsz, NULL);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%llx mem 0x%llx\n", da,
				memsz);
			ret = -EINVAL;
			break;
		}

		/* put the segment where the remote processor expects it */
		if (filesz)
			memcpy(ptr, elf_data + offset, filesz);

		/*
		 * Zero out remaining memory for this segment.
		 *
		 * This isn't strictly required since dma_alloc_coherent already
		 * did this for us. albeit harmless, we may consider removing
		 * this.
		 */
		if (memsz > filesz)
			memset(ptr + filesz, 0, memsz - filesz);
	}

	return ret;
}

static const struct rproc_ops cix_dsp_rproc_ops = {
	.prepare	= cix_dsp_rproc_prepare,
	.unprepare	= cix_dsp_rproc_unprepare,
	.start		= cix_dsp_rproc_start,
	.stop		= cix_dsp_rproc_stop,
	.kick		= cix_dsp_rproc_kick,
	.da_to_va	= cix_dsp_rproc_da_to_va,
	.load		= cix_dsp_rproc_elf_load_segments,
#ifdef CONFIG_PLAT_BBOX
	.coredump       = cix_dsp_rproc_rdr_coredump,
#endif
	.parse_fw	= rproc_elf_load_rsc_table,
	.sanity_check	= rproc_elf_sanity_check,
	.get_boot_addr	= rproc_elf_get_boot_addr,
};

static int cix_dsp_rproc_addr_init(struct cix_dsp_rproc *rproc_priv)
{
	struct cix_mem_region *mem_reg = rproc_priv->mem_region;
	struct device *dev = rproc_priv->dev;
	struct cix_dsp_mem *p_mem;
	int i;

	p_mem = devm_kcalloc(dev, CIX_MEM_REG_NUM, sizeof(*p_mem), GFP_KERNEL);
	if (!p_mem)
		return -ENOMEM;

	for (i = 0; i < CIX_MEM_REG_NUM; i++) {
		if (!strcmp(mem_reg[i].name, "vdev0buffer") ||
		    !strcmp(mem_reg[i].name, "vdev0vring0") ||
		    !strcmp(mem_reg[i].name, "vdev0vring1"))
			continue;

		p_mem[i].cpu_addr = devm_ioremap_wc(dev, mem_reg[i].sa, mem_reg[i].len);
		if (!p_mem[i].cpu_addr) {
			dev_err(dev, "failed to remap %#x bytes from %#llx\n",
				mem_reg[i].len, mem_reg[i].sa);
			return -ENOMEM;
		}
		p_mem[i].bus_addr = mem_reg[i].sa;
		p_mem[i].size = mem_reg[i].len;

		dev_dbg(dev, " %s, priv->mem[%d] sys_addr: 0x%llx, cpu_addr: 0x%px, size 0x%lx\n",
			__func__, i, p_mem[i].bus_addr, p_mem[i].cpu_addr, p_mem[i].size);
	}

	rproc_priv->mem = p_mem;
	rproc_priv->num_mem = CIX_MEM_REG_NUM;

#ifdef CONFIG_PLAT_BBOX
	g_rdr_dump_mem_region_index_0 = 0;

	for (i = 0; i < CIX_MEM_REG_NUM; i++) {
		if (p_mem[i].cpu_addr) {
			g_rdr_dump_mem_region_0[g_rdr_dump_mem_region_index_0].name = mem_reg[i].name;
			g_rdr_dump_mem_region_0[g_rdr_dump_mem_region_index_0].va = p_mem[i].cpu_addr;
			g_rdr_dump_mem_region_0[g_rdr_dump_mem_region_index_0].length = p_mem[i].size;

			dev_dbg(dev, "rdr_mem_region_0 index = %d, name = %s, va = %px, len = %lu\n",
			g_rdr_dump_mem_region_index_0, mem_reg[i].name, p_mem[i].cpu_addr, p_mem[i].size);

			g_rdr_dump_mem_region_index_0++;
		}
	}
#endif

	return 0;
}

static irqreturn_t cix_dsp_rproc_wdg(int irq, void *data)
{
	struct platform_device *pdev = data;
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct cix_dsp_rproc *rproc_priv = rproc->priv;

	/* Disable wdg_irq to avoid interrupt storm */
	disable_irq_nosync(rproc_priv->wdg_irq);

	rproc_priv->is_wdg_trigger = true;

	rproc_report_crash(rproc, RPROC_WATCHDOG);

	return IRQ_HANDLED;
}

static void cix_dsp_rproc_rx_callback(struct mbox_client *cl, void *data)
{
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	u32 *message = (u32 *)data;

	dev_dbg(rproc_priv->dev, " mbox rx msg: 0x%x 0x%x\n",
		message[0], message[MBOX_MSG_OFFSET]);

	switch (message[MBOX_MSG_OFFSET]) {
	case MBOX_MSG_REPROC_READY:
		rproc_priv->rproc_ready = true;
		break;
	case MBOX_MSG_SUSPEND_ACK:
	case MBOX_MSG_REPROC_STOP_ACK:
		complete(&rproc_priv->rsp_comp);
		break;
	case MBOX_MSG_REPROC_CRASH:
		rproc_report_crash(rproc, RPROC_WATCHDOG);
		break;
	default:
		queue_work(rproc_priv->workqueue, &rproc_priv->rproc_work);
		break;
	}
}

static int cix_dsp_rproc_request_mbox(struct cix_dsp_rproc *rproc_priv)
{
	struct device *dev = rproc_priv->dev;
	struct mbox_client *cl;
	int ret;

	if (!device_property_present(dev, "mbox-names"))
		return 0;

	cl = &rproc_priv->cl;
	cl->dev = dev;
	cl->tx_block = true;
	cl->tx_tout = MBOX_SEND_TIMEOUT;
	cl->knows_txdone = false;
	cl->rx_callback = cix_dsp_rproc_rx_callback;

	rproc_priv->tx_ch = mbox_request_channel_byname(cl, "tx0");
	if (IS_ERR(rproc_priv->tx_ch)) {
		ret = PTR_ERR(rproc_priv->tx_ch);
		dev_dbg(cl->dev, "failed to request tx mailbox channel: %d\n",
			ret);
		goto err_out;
	}

	rproc_priv->rx_ch = mbox_request_channel_byname(cl, "rx0");
	if (IS_ERR(rproc_priv->rx_ch)) {
		ret = PTR_ERR(rproc_priv->rx_ch);
		dev_dbg(cl->dev, "failed to request rx mailbox channel: %d\n",
			ret);
		goto err_out;
	}

	return 0;

err_out:
	if (!IS_ERR(rproc_priv->tx_ch))
		mbox_free_channel(rproc_priv->tx_ch);
	if (!IS_ERR(rproc_priv->rx_ch))
		mbox_free_channel(rproc_priv->rx_ch);

	return ret;
}

static void cix_dsp_rproc_free_mbox(struct cix_dsp_rproc *rproc_priv)
{
	mbox_free_channel(rproc_priv->tx_ch);
	mbox_free_channel(rproc_priv->rx_ch);
}

static void cix_dsp_rproc_mbox_vq_work(struct work_struct *work)
{
	struct cix_dsp_rproc *rproc_priv = container_of(work, struct cix_dsp_rproc,
							   rproc_work);
	struct rproc *rproc = rproc_priv->rproc;

	mutex_lock(&rproc->lock);

	if (rproc->state != RPROC_RUNNING)
		goto unlock_mutex;

	rproc_vq_interrupt(rproc_priv->rproc, 0);
	rproc_vq_interrupt(rproc_priv->rproc, 1);

unlock_mutex:
	mutex_unlock(&rproc->lock);
}

static int cix_dsp_rproc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cix_dsp_rproc *rproc_priv;
	struct rproc *rproc;
	const char *fw_name;
	int ret, irq;
	const void *dev_data;

	dev_dbg(dev, " %s\n", __func__);

	dev_data = device_get_match_data(dev);
	if (!dev_data)
		return -ENODEV;

	ret = device_property_read_string(dev, "firmware-name", &fw_name);
	if (ret) {
		dev_err(dev, "failed to parse firmware-name property, ret = %d\n",
			ret);
		return ret;
	}

	rproc = rproc_alloc(dev, "cix-dsp-rproc", &cix_dsp_rproc_ops, fw_name,
			    sizeof(*rproc_priv));
	if (!rproc)
		return -ENOMEM;

	rproc_priv = rproc->priv;
	rproc_priv->rproc = rproc;
	rproc_priv->dev = dev;
	rproc_priv->mem_region = (struct cix_mem_region *)dev_data;
	rproc_priv->rproc_ready = false;

	ret = cix_dsp_rproc_clks_get(rproc_priv);
	if (ret) {
		dev_err(dev, "failed to get clocks\n");
		return ret;
	}

	rproc_priv->regmap =
		device_syscon_regmap_lookup_by_property(&pdev->dev,
							"cix,dsp-ctrl");
	if (IS_ERR(rproc_priv->regmap)) {
		dev_err(dev, "failed to find syscon\n");
		return PTR_ERR(rproc_priv->regmap);
	}

	rproc_priv->dsp_rst = devm_reset_control_get(dev, "dsp");
	if (IS_ERR(rproc_priv->dsp_rst))
		return PTR_ERR(rproc_priv->dsp_rst);

	rproc_priv->mb0_rst = devm_reset_control_get(dev, "mb0");
	if (IS_ERR(rproc_priv->mb0_rst))
		return PTR_ERR(rproc_priv->mb0_rst);

	rproc_priv->mb1_rst = devm_reset_control_get(dev, "mb1");
	if (IS_ERR(rproc_priv->mb1_rst))
		return PTR_ERR(rproc_priv->mb1_rst);

	irq = platform_get_irq(pdev, 0);
	if (irq == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (irq > 0) {
		ret = devm_request_irq(dev, irq, cix_dsp_rproc_wdg, 0,
				       dev_name(dev), pdev);
		if (ret) {
			dev_err(dev, "failed to request wdg irq\n");
			goto err_irq_req;
		}

		rproc_priv->wdg_irq = irq;

		disable_irq(irq);

		dev_info(dev, "wdg irq registered\n");
	}

	dev_set_drvdata(dev, rproc);

	rproc_priv->workqueue = create_workqueue(dev_name(dev));
	if (!rproc_priv->workqueue) {
		dev_err(dev, "cannot create workqueue\n");
		ret = -ENOMEM;
		goto err_wkq;
	}

	INIT_WORK(&rproc_priv->rproc_work, cix_dsp_rproc_mbox_vq_work);

	ret = cix_dsp_rproc_addr_init(rproc_priv);
	if (ret) {
		dev_err(dev, "failed on cix_dsp_rproc_addr_init\n");
		goto err_addr;
	}

	init_completion(&rproc_priv->rsp_comp);
	rproc->auto_boot = false;
	ret = rproc_add(rproc);
	if (ret) {
		dev_err(dev, "rproc_add failed\n");
		goto err_rproc_add;
	}

	pm_runtime_enable(dev);

#ifdef CONFIG_PLAT_BBOX
	init_completion(&g_rdr_dump_comp);

	cix_dsp_rproc_rdr_register_exception();

	ret = cix_dsp_rproc_rdr_register_core();
	if (ret) {
		dev_err(dev, "cix_dsp_rproc_rdr_register_core fail, ret = [%d]\n", ret);
		goto err_rproc_add;
	}
#endif

	return 0;

err_rproc_add:
err_addr:
	destroy_workqueue(rproc_priv->workqueue);
err_irq_req:
err_wkq:
	rproc_free(rproc);

	return ret;
}

static int cix_dsp_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct cix_dsp_rproc *rproc_priv = rproc->priv;

#ifdef CONFIG_PLAT_BBOX
	cix_dsp_rproc_rdr_unregister_core();
	cix_dsp_rproc_rdr_unregister_exception();
#endif

	pm_runtime_disable(&pdev->dev);
	rproc_del(rproc);
	destroy_workqueue(rproc_priv->workqueue);
	rproc_free(rproc);

	return 0;
}

static int cix_dsp_rproc_runtime_suspend(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct cix_dsp_rproc *rproc_priv = rproc->priv;

	dev_info(dev, "%s\n", __func__);

	disable_irq(rproc_priv->wdg_irq);

	cix_dsp_rproc_free_mbox(rproc_priv);

	cix_dsp_rproc_clks_disable(rproc_priv);

	return 0;
}

static int cix_dsp_rproc_runtime_resume(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	int ret;

	dev_info(dev, "%s\n", __func__);

	ret = cix_dsp_rproc_clks_enable(rproc_priv);
	if (ret) {
		dev_err(rproc_priv->dev, "failed to enable clocks\n");
		return ret;
	}

	cix_dsp_rproc_peri_reset(rproc_priv);

	ret = cix_dsp_rproc_request_mbox(rproc_priv);
	if (ret) {
		dev_err(dev, "mbox_init failed\n");
		return ret;
	}

	return 0;
}

static int cix_dsp_rproc_suspend(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	u32 msg[MBOX_MSG_LEN];
	int ret;

	dev_info(dev, " %s\n", __func__);

	if (rproc->state == RPROC_RUNNING) {
		reinit_completion(&rproc_priv->rsp_comp);

		msg[0] = MBOX_MSG_LEN;
		msg[MBOX_MSG_OFFSET] = MBOX_MSG_SYSTEM_SUSPEND;
		ret = mbox_send_message(rproc_priv->tx_ch, (void *)&msg);
		if (ret < 0) {
			cix_dsp_mbox_dump_regs(rproc);
			dev_err(dev, "PM mbox_send_message failed: %d\n", ret);
			return ret;
		}

		if (!wait_for_completion_timeout(&rproc_priv->rsp_comp,
						 msecs_to_jiffies(PM_COMP_TIMEOUT)))
			return -EBUSY;
	}

	return pm_runtime_force_suspend(dev);
}

static void cix_dsp_rproc_load_firmware(const struct firmware *fw, void *context)
{
	struct rproc *rproc = context;
	int ret;

	pr_info("%s\n", __func__);
	ret = rproc_load_segments(rproc, fw);
	if (ret)
		goto out;

	ret = rproc->ops->start(rproc);
	if (ret)
		goto out;

	rproc->ops->kick(rproc, 0);

out:
	release_firmware(fw);
}

static int cix_dsp_rproc_resume(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct cix_dsp_rproc *rproc_priv = rproc->priv;
	int ret = 0;

	dev_info(dev, "%s\n", __func__);
	ret = pm_runtime_force_resume(dev);
	if (ret)
		return ret;

	if (rproc->state == RPROC_RUNNING) {
		rproc_priv->is_resume_back = true;

		cix_dsp_rproc_stall(rproc_priv);

		ret = request_firmware_nowait(THIS_MODULE,
					      1,
					      rproc->firmware,
					      dev,
					      GFP_KERNEL,
					      rproc,
					      cix_dsp_rproc_load_firmware);
		if (ret < 0) {
			dev_err(dev, "load firmware failed: %d\n", ret);
			goto err;
		}
	}

	return 0;

err:
	pm_runtime_force_suspend(dev);

	return ret;
}

static const struct dev_pm_ops cix_dsp_rproc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cix_dsp_rproc_suspend, cix_dsp_rproc_resume)
	SET_RUNTIME_PM_OPS(cix_dsp_rproc_runtime_suspend,
			   cix_dsp_rproc_runtime_resume, NULL)
};

static const struct of_device_id cix_dsp_rproc_of_match[] = {
	{ .compatible = "cix,sky1-hifi5", .data = &sky1_dsp_mems, },
	{ },
};
MODULE_DEVICE_TABLE(of, cix_dsp_rproc_of_match);

static const struct acpi_device_id cix_dsp_rproc_acpi_match[] = {
	{ "CIXH6000", (kernel_ulong_t)&sky1_dsp_mems },
	{ },
};
MODULE_DEVICE_TABLE(acpi, cix_dsp_rproc_acpi_match);

static struct platform_driver cix_dsp_rproc_driver = {
	.probe = cix_dsp_rproc_probe,
	.remove = cix_dsp_rproc_remove,
	.driver = {
		.name = "cix-dsp-rproc",
		.of_match_table = cix_dsp_rproc_of_match,
		.acpi_match_table = cix_dsp_rproc_acpi_match,
		.pm = &cix_dsp_rproc_pm_ops,
	},
};
module_platform_driver(cix_dsp_rproc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CIX Audio DSP Remote Processor Control Driver");
MODULE_AUTHOR("Lihua Liu <Lihua.Liu@cixcomputing.com>");

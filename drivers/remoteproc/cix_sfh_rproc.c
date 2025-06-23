// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.

#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mailbox_client.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/remoteproc.h>
#include <linux/workqueue.h>
#include <linux/pm_wakeirq.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/firmware.h>
#ifdef CONFIG_HIBERNATION
#include <linux/soc/cix/cix_hibernate.h>
#endif
#include "remoteproc_internal.h"
#include "remoteproc_elf_helpers.h"

#ifdef CONFIG_PLAT_BBOX
#include <linux/soc/cix/rdr_pub.h>
#include <mntn_public_interface.h>
#endif /* CONFIG_PLAT_BBOX */

/* SFH register define */
#define CIX_SFH_CRU_OFFSET		(0x80000)
#define CIX_SFH_CRU_SIZE		(0x10000)

#define SFH_CLK_EN			(0x000)	/* sf subsystem ip clock enable offset */
#define CLK_STAR_EN			BIT(0)
#define CLK_SRAM_EN			BIT(1)
#define CLK_MBOX0_EN			BIT(11)
#define CLK_MBOX1_EN			BIT(12)
#define CLK_MBOX2_EN			BIT(13)
#define CLK_MBOX3_EN			BIT(14)

#define SFH_SW_RST			(0x010)	/* sf subsystem ip software reset offset */
#define SW_RST_SRAM			BIT(4)

#define SFH_SW_RST_APB			(0x014)
#define SW_RST_APB_MBOX0		BIT(14)
#define SW_RST_APB_MBOX1		BIT(15)
#define SW_RST_APB_MBOX2		BIT(16)
#define SW_RST_APB_MBOX3		BIT(17)

#define HUB2NIC_ADR_REMAP		(0x07c)
#define HUB2NIC_CONFIG_SET		(0x20)	/* default 0x20, make 0xA000_0000 remap to 0x2000_0000*/

#define SFH_RST_REG_BASE		(0x16000000)	/* pmctl s5 base addr */
#define SENSORHUB_RST			(0x308)	/* sensorhub reset offset */
#define SENSORHUB_NOC_RST		(0x30c)	/* sensorhub noc reset offset */
#define SFH_RESET			BIT(0)
#define SFH_NOC_RESET			BIT(0)

#define RCSU_REMAP_BASE			(0x8000000)
#define RCSU_SFH_REMAP			(0x034)
#define RCSU_SFH_CRU			(0x50000000)
#define RCSU_SFH_SRAM			(0x20000000)

#define CIX_MEM_REG_NUM			(2)
#define PM_COMP_TIMEOUT			(100)
#define SFH_COMP_TIMEOUT		(1000)
#define RPROC_READY_WAIT_MAX_CNT	(3000)
#define MBOX_SEND_TIMEROUT		(100000)
#define MBOX_MSG_OFFSET			(1)
#define MBOX_MSG_LEN			(2)

#define CIX_SFH_CLK_NUM		(2)

static const char *cix_sfh_clk_names[CIX_SFH_CLK_NUM] = {
	"sensor_hub_25M",
	"sensor_hub_400M"
};

/**
 * struct cix_sfh_mem - internal memory structure
 * @cpu_addr: Virtual address of the memory region
 * @bus_addr: Bus address used to access the memory region
 * @dev_addr: Device address of the memory region from M33 view
 * @size: Size of the memory region
 */
struct cix_sfh_mem {
	void __iomem *cpu_addr;
	phys_addr_t bus_addr;
	u32 dev_addr;
	size_t size;
};

/**
 * struct cix_mem_region - memory region structure
 * @name: Name of the memory region
 * @sa: System Bus address used to access the memory region
 * @da: Device address of the memory region from M33 view
 * @len: Length of the memory region
 */
struct cix_mem_region {
	const char *name;
	u32 da;
	u64 sa;
	u32 len;
};

#ifdef CONFIG_PLAT_BBOX
enum RDR_SF_MODID{
	RDR_SF_MODID_START = PLAT_BB_MOD_SF_START,
	RDR_SF_SOC_WD_TIMEOUT_MODID,
	RDR_SF_MODID_END = PLAT_BB_MOD_SF_END,
};

static struct rdr_register_module_result g_current_info;

struct rdr_exception_info_s g_sf_einfo[] = {
	{ { 0, 0 }, RDR_SF_SOC_WD_TIMEOUT_MODID, RDR_SF_SOC_WD_TIMEOUT_MODID, RDR_ERR,
	 RDR_REBOOT_NO, RDR_SF, RDR_SF, RDR_SF,
	 (u32)RDR_REENTRANT_DISALLOW, SENSOR_FUSION_EXCEPTION, 0, (u32)RDR_UPLOAD_YES,
	 "sensor fusion", "sensor fusion watchdog timeout", 0, 0, 0 },
};

struct rdr_dump_mem_region {
	const char *name;
	void *va;
	u32 length;
};

#define RDR_DUMP_MEM_REGION_1_SIZE  3
static struct rdr_dump_mem_region g_rdr_dump_mem_region_0, g_rdr_dump_mem_region_1[RDR_DUMP_MEM_REGION_1_SIZE];
static unsigned int g_rdr_dump_mem_region_index_1;

#define RDR_DUMP_COMP_TIMEOUT		500
static struct completion g_rdr_dump_comp;


#endif /* CONFIG_PLAT_BBOX */

static const struct cix_mem_region sky1_sfh_mems[CIX_MEM_REG_NUM] = {
	{ .name = "sram", .da = 0x20000000, .sa = 0x1ca0000000, .len = 0x80000 },  /* 512 KB */
	{ .name = "ddr", .da = 0x64000000, .sa = 0x84000000, .len = 0x1000000 },   /* 16 MB */
};

enum cix_sfh_rst {
	RST_N 	= 0,
	NOC_RST_N = 1,
	RST_NUM	= 2,
};

static const struct regmap_config cix_sfh_regmap_cfg = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.name = "cix_sfh"
};

struct cix_sfh_rproc {
	struct device *dev;
	struct regmap *regmap;
	struct rproc *rproc;
	struct reset_control *rst[RST_NUM];
	struct clk *clks[CIX_SFH_CLK_NUM];
	int wdg_irq;
	int sw_irq;
	int num_mem;
	int offset;
	struct cix_sfh_mem *mem;
	struct cix_mem_region *mem_region;
	struct mbox_client cl;
	struct mbox_chan *tx_ch;
	struct mbox_chan *rx_ch;
	struct work_struct rproc_work;
	struct workqueue_struct *workqueue;
	struct completion rsp_comp;
	bool rproc_ready;
};

enum cix_sfh_mbox_messages {
	MBOX_MSG_REPROC_READY	= 0xFFF0,
	MBOX_MSG_REPROC_CRASH	= 0xFFF1,
	MBOX_MSG_SYSTEM_SUSPEND	= 0xFFF2,
	MBOX_MSG_SUSPEND_ACK	= 0xFFF3,
	MBOX_MSG_SYSTEM_RESUME	= 0xFFF4,
	MBOX_MSG_RESUME_ACK	= 0xFFF5,
	MBOX_MSG_SFH_COMPLETE   = 0xFFF6L,
};

#ifdef CONFIG_HIBERNATION
static struct hibernate_rmem_ops cix_rproc_reserve_ops[2] = {NULL}; //for vring0 and vring1

static void cix_rproc_hibernate_mem_reserve(void)
{
	int i;

	for (i = 0; i < sizeof(cix_rproc_reserve_ops) / sizeof(cix_rproc_reserve_ops[0]); i++) {
		register_reserve_mem_ops(&cix_rproc_reserve_ops[i]);
	}
}

static void cix_rproc_hibernate_mem_release(void)
{
	int i;

	for (i = 0; i < sizeof(cix_rproc_reserve_ops) / sizeof(cix_rproc_reserve_ops[0]); i++) {
		unregister_reserve_mem_ops(&cix_rproc_reserve_ops[i]);
	}
}
#endif

static int cix_rproc_mem_alloc(struct rproc *rproc,
				 struct rproc_mem_entry *mem)
{
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
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

static int cix_rproc_mem_release(struct rproc *rproc,
				   struct rproc_mem_entry *mem)
{
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	struct device *dev = rproc_priv->dev;

	dev_dbg(dev, "unmap memory: %pa\n", &mem->dma);
	iounmap(mem->va);

	return 0;
}

static int cix_rproc_pa_to_da(struct rproc *rproc, phys_addr_t pa, size_t len, u64 *da)
{
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	struct cix_mem_region *mem_reg = rproc_priv->mem_region;
	int i;

	/* parse address translation table */
	for (i = 0; i < CIX_MEM_REG_NUM; i++) {
		if (pa >= mem_reg[i].sa
		    && pa + len <= mem_reg[i].sa + mem_reg[i].len) {
			unsigned int offset = pa - mem_reg[i].sa;

			*da = mem_reg[i].da + offset;
			return 0;
		}
	}

	return -ENOENT;
}

static int cix_rproc_da_to_pa(struct cix_sfh_rproc *rproc_priv, u64 da, size_t len, u64 *pa)
{
	struct cix_mem_region *mem_reg = rproc_priv->mem_region;
	int i;

	/* parse address translation table */
	for (i = 0; i < CIX_MEM_REG_NUM; i++) {
		if (da >= mem_reg[i].da
		    && da + len <= mem_reg[i].da + mem_reg[i].len) {
			unsigned int offset = da - mem_reg[i].da;

			*pa = mem_reg[i].sa + offset;
			return 0;
		}
	}

	return -ENOENT;
}

static int cix_rproc_parse_memory_regions(struct rproc *rproc)
{
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	struct device *dev = rproc_priv->dev;
	struct device_node *np = dev->of_node;
	struct of_phandle_iterator it;
	struct rproc_mem_entry *mem;
	struct reserved_mem *rmem;
	u64 da;
	int index = 0;

	/* Register associated reserved memory regions */
	of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	while (of_phandle_iterator_next(&it) == 0) {
		rmem = of_reserved_mem_lookup(it.node);
		if (!rmem) {
			dev_err(dev, "unable to acquire memory-region\n");
			return -EINVAL;
		}

		if (!strcmp(it.node->name, "ram0"))
			continue;

		if (cix_rproc_pa_to_da(rproc, rmem->base, rmem->size, &da) < 0) {
			dev_err(dev, "memory region not valid %pa\n",
				&rmem->base);
			return -EINVAL;
		}

		/*  No need to map vdev buffer */
		if (strcmp(it.node->name, "vdev0buffer")) {
			/* Register memory region */
			mem = rproc_mem_entry_init(dev, NULL,
						   (dma_addr_t)rmem->base,
						   rmem->size, da,
						   cix_rproc_mem_alloc,
						   cix_rproc_mem_release,
						   it.node->name);

		} else {
			/* Register reserved memory for vdev buffer alloc */
			mem = rproc_of_resm_mem_entry_init(dev, index,
							   rmem->size,
							   rmem->base,
							   it.node->name);
		}

		if (!mem)
			return -ENOMEM;

		dev_dbg(dev, "add carveout %s, base 0x%llx, size 0x%llx\n",
			it.node->name, rmem->base, rmem->size);

		rproc_add_carveout(rproc, mem);
		index++;
	}

	return 0;
}

static int cix_sfh_get_clks(struct cix_sfh_rproc *rproc_priv)
{
	int i;

	for (i = 0; i < CIX_SFH_CLK_NUM; i++) {
		rproc_priv->clks[i] = devm_clk_get_optional(rproc_priv->dev,
					cix_sfh_clk_names[i]);
		if (IS_ERR(rproc_priv->clks[i])) {
			dev_err(rproc_priv->dev, "failed to get clock %s\n",
					cix_sfh_clk_names[i]);
			return PTR_ERR(rproc_priv->clks[i]);
		}
	}

	return 0;
}

static int cix_sfh_clks_enable(struct cix_sfh_rproc *rproc_priv)
{
	int i, err;

	for (i = 0; i < CIX_SFH_CLK_NUM; i++) {
		err = clk_prepare_enable(rproc_priv->clks[i]);
		if (err) {
			dev_err(rproc_priv->dev, "failed to enable clock %s\n",
				cix_sfh_clk_names[i]);
			goto err_sfh_clks;
		}
	}

	return 0;

err_sfh_clks:
	while (--i >= 0)
		clk_disable_unprepare(rproc_priv->clks[i]);

	return err;
}

static void cix_sfh_clks_disable(struct cix_sfh_rproc *rproc_priv)
{
	int i;

	for (i = 0; i < CIX_SFH_CLK_NUM; i++)
		clk_disable_unprepare(rproc_priv->clks[i]);
}

static void cix_rcsu_remap_sfh(void)
{
	void __iomem *base;

	base = ioremap(RCSU_REMAP_BASE, CIX_SFH_CRU_SIZE);
	writel(RCSU_SFH_CRU, base + RCSU_SFH_REMAP);
	iounmap(base);
}

static void cix_sfh_mbox_clock_enable(struct cix_sfh_rproc *rproc_priv)
{
	regmap_update_bits(rproc_priv->regmap, rproc_priv->offset + SFH_CLK_EN,
				(CLK_MBOX0_EN | CLK_MBOX1_EN | CLK_MBOX2_EN | CLK_MBOX3_EN),
				(CLK_MBOX0_EN | CLK_MBOX1_EN | CLK_MBOX2_EN | CLK_MBOX3_EN));
}

static void cix_sfh_mbox_clock_disable(struct cix_sfh_rproc *rproc_priv)
{
	regmap_clear_bits(rproc_priv->regmap, rproc_priv->offset + SFH_CLK_EN,
				(CLK_MBOX0_EN | CLK_MBOX1_EN | CLK_MBOX2_EN | CLK_MBOX3_EN));
}

static void cix_sfh_mbox_reset(struct cix_sfh_rproc *rproc_priv)
{
        regmap_update_bits(rproc_priv->regmap, rproc_priv->offset + SFH_SW_RST_APB,
                                (SW_RST_APB_MBOX0 | SW_RST_APB_MBOX1 | SW_RST_APB_MBOX2 | SW_RST_APB_MBOX3), 0);
}

static void cix_sfh_mbox_release_reset(struct cix_sfh_rproc *rproc_priv)
{
	regmap_update_bits(rproc_priv->regmap, rproc_priv->offset + SFH_SW_RST_APB,
				(SW_RST_APB_MBOX0 | SW_RST_APB_MBOX1 | SW_RST_APB_MBOX2 | SW_RST_APB_MBOX3),
				(SW_RST_APB_MBOX0 | SW_RST_APB_MBOX1 | SW_RST_APB_MBOX2 | SW_RST_APB_MBOX3));
}

static void sfh_hub2nic_remap(struct cix_sfh_rproc *rproc_priv)
{
	regmap_write(rproc_priv->regmap, rproc_priv->offset + HUB2NIC_ADR_REMAP,
				HUB2NIC_CONFIG_SET);
}

static void sfh_sram_rst(struct cix_sfh_rproc *rproc_priv)
{
	regmap_update_bits(rproc_priv->regmap, rproc_priv->offset + SFH_SW_RST,
				SW_RST_SRAM, SW_RST_SRAM);
}

static void cix_sfh_enable_clock(struct cix_sfh_rproc *rproc_priv)
{
	regmap_update_bits(rproc_priv->regmap, rproc_priv->offset + SFH_CLK_EN,
				CLK_STAR_EN | CLK_SRAM_EN, CLK_STAR_EN | CLK_SRAM_EN);
}

static void cix_sfh_disable_clock(struct cix_sfh_rproc *rproc_priv)
{
	regmap_clear_bits(rproc_priv->regmap, rproc_priv->offset + SFH_CLK_EN, CLK_STAR_EN | CLK_SRAM_EN);
}

/*
 * The M33 cores have a local reset that affects only the CPU, and a
 * generic module reset that powers on the device and allows the sram
 * memories to be accessed. This function is
 * used to release the global reset on M33 core to allow loading into the
 * RAMs. The .prepare() ops is invoked by remoteproc core before any
 * firmware loading, and is followed by the .start() ops after loading to
 * actually let the M33 cores run.
 */
static int cix_sfh_rproc_prepare(struct rproc *rproc)
{
	int err;

	err = cix_rproc_parse_memory_regions(rproc);
	if (err)
		return err;

	return 0;
}

static int cix_sfh_rproc_unprepare(struct rproc *rproc)
{
	// TBD, Does this global reset only impact M33?
	// If not, we couldn't do reset operation here.
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	reset_control_deassert(rproc_priv->rst[RST_N]);

	return 0;
}

static int cix_sfh_rproc_start(struct rproc *rproc)
{
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	int i;

	rproc_priv->rproc_ready = false;
	/* kick-off the M33 core to run */
	/* reset reset_n */
	reset_control_assert(rproc_priv->rst[RST_N]);
	/* release reset_n */
	reset_control_deassert(rproc_priv->rst[RST_N]);

	if (!rproc_priv->rx_ch)
		return 0;

	for (i = 0; i < RPROC_READY_WAIT_MAX_CNT; i++) {
		if (rproc_priv->rproc_ready == true)
			return 0;
		usleep_range(200, 500);
	}

	return -ETIMEDOUT;
}

static int cix_sfh_rproc_stop(struct rproc *rproc)
{
	struct cix_sfh_rproc *rproc_priv = rproc->priv;

	flush_workqueue(rproc_priv->workqueue);
	disable_irq(rproc_priv->wdg_irq);
	/*
	 * sensorHub is on S5 always-On domain , no need
	 * to power-off, just disable the clock.
	 */
	regmap_update_bits(rproc_priv->regmap, rproc_priv->offset + SFH_CLK_EN, CLK_STAR_EN | CLK_SRAM_EN, 0);
	rproc_priv->rproc_ready = false;

	return 0;
}

static void cix_sfh_mbox_dump_regs(struct cix_sfh_rproc *rproc_priv)
{
	void __iomem *base;
	int i;
	u32 val;
#define RCSU_MBOX_BASE		(0x080a0000)
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

static void cix_sfh_rproc_kick(struct rproc *rproc, int vqid)
{
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	int err;
	u32 msg[MBOX_MSG_LEN];

	if (!rproc_priv->tx_ch) {
		dev_err(rproc_priv->dev, "No initialized mbox tx channel\n");
		return;
	}

	/*
	 * Send the index of the triggered virtqueue as the mbox payload.
	 * Let remote processor know which virtqueue is used.
	 */
	msg[0] = MBOX_MSG_LEN;
	msg[MBOX_MSG_OFFSET] = vqid;

	err = mbox_send_message(rproc_priv->tx_ch, (void *)&msg);
	if (err < 0) {
		cix_sfh_mbox_dump_regs(rproc_priv);
		dev_err(rproc_priv->dev, "%s: failed (%d, err:%d)\n",
			__func__, vqid, err);
	}
}

static void *cix_sfh_rproc_da_to_va(struct rproc *rproc, u64 da, size_t len, bool *is_iomem)
{
	struct cix_sfh_rproc *priv = rproc->priv;
	void *va = NULL;
	u64 pa;
	int i;

	if (len == 0)
		return NULL;

	if (cix_rproc_da_to_pa(priv, da, len, &pa))
		return NULL;

	for (i = 0; i < priv->num_mem; i++) {
		if (pa >= priv->mem[i].bus_addr
		    && pa + len <= priv->mem[i].bus_addr +  priv->mem[i].size) {
			unsigned int offset = pa - priv->mem[i].bus_addr;

			va = priv->mem[i].cpu_addr + offset;
			break;
		}
	}

	dev_dbg(priv->dev, "da = 0x%llx len = 0x%zx va = 0x%p\n",
		da, len, va);

	return va;
}

#ifdef CONFIG_PLAT_BBOX
/*
 * Description : register exceptionwith to rdr
 */
static void rdr_sky1_sf_register_exception(void)
{
	unsigned int i;
	u32 ret;

	pr_debug("%s enter\n", __func__);
	for (i = 0; i < sizeof(g_sf_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("register exception:%u", g_sf_einfo[i].e_exce_type);
		ret = rdr_register_exception(&g_sf_einfo[i]);
		if (ret == 0){
			pr_err("rdr_register_exception fail, ret = [%u]\n", ret);
			return;
		}
	}
	pr_debug("%s end\n", __func__);
}

static void rdr_sky1_sf_unregister_exception(void)
{
	unsigned int i;

	for (i = 0; i < sizeof(g_sf_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("unregister exception:%u", g_sf_einfo[i].e_exce_type);
		rdr_unregister_exception(g_sf_einfo[i].e_modid);
	}
}

static void rdr_sky1_sf_dump(u32 modid, u32 etype,
				u64 coreid, char *log_path,
				pfn_cb_dump_done pfn_cb)
{
	unsigned int i;
	int ret;

	/* sram */
	if (g_rdr_dump_mem_region_0.va) {
		ret = rdr_savebuf2fs(log_path, g_rdr_dump_mem_region_0.name,
							g_rdr_dump_mem_region_0.va, g_rdr_dump_mem_region_0.length, 0);
		if (ret < 0) {
			pr_err("rdr_savelogbuf2fs region_0: name = %s, error = %d\n",
					g_rdr_dump_mem_region_0.name, ret);
			return;
		}

		pr_debug("dump to file successed: path = %s, name = %s, len = 0x%x\n",
				 log_path, g_rdr_dump_mem_region_0.name, g_rdr_dump_mem_region_0.length);
	}

	/* vdev0vring0, vdev0vring1, vdev0buffer*/
	for (i = 0; i < RDR_DUMP_MEM_REGION_1_SIZE; i++) {
		if (g_rdr_dump_mem_region_1[i].va) {
			ret = rdr_savebuf2fs(log_path, g_rdr_dump_mem_region_1[i].name,
					     g_rdr_dump_mem_region_1[i].va,
					     g_rdr_dump_mem_region_1[i].length, 0);
			if (ret < 0) {
				pr_err("rdr_savelogbuf2fs region_0: name = %s, error = %d\n",
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
 * Description : Register the dump and reset functions to the rdr
 */
static int rdr_sky1_sf_register_core(void)
{
	struct rdr_module_ops_pub s_sf_ops;
	struct rdr_register_module_result retinfo;
	int ret;
	u64 coreid = RDR_SF;

	s_sf_ops.ops_dump = rdr_sky1_sf_dump;
	s_sf_ops.ops_reset = NULL;

	ret = rdr_register_module_ops(coreid, &s_sf_ops, &retinfo);
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

static void rdr_sky1_sf_unregister_core(void)
{
	rdr_unregister_module_ops(RDR_SF);
}

static void cix_sfh_rproc_rdr_coredump(struct rproc *rproc)
{
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
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
	rdr_system_error(RDR_SF_SOC_WD_TIMEOUT_MODID, 0, 0);

	if (!wait_for_completion_timeout(&g_rdr_dump_comp,
					 msecs_to_jiffies(RDR_DUMP_COMP_TIMEOUT)))
		dev_err(dev, "rdr dump file timeout");
}
#endif /* CONFIG_PLAT_BBOX */

static const struct rproc_ops cix_sfh_rproc_ops = {
	.prepare	= cix_sfh_rproc_prepare,
	.unprepare	= cix_sfh_rproc_unprepare,
	.start		= cix_sfh_rproc_start,
	.stop		= cix_sfh_rproc_stop,
	.kick		= cix_sfh_rproc_kick,
	.da_to_va	= cix_sfh_rproc_da_to_va,
	.load		= rproc_elf_load_segments,
#ifdef CONFIG_PLAT_BBOX
	.coredump	= cix_sfh_rproc_rdr_coredump,
#endif
	.parse_fw	= rproc_elf_load_rsc_table,
	.sanity_check	= rproc_elf_sanity_check,
	.get_boot_addr	= rproc_elf_get_boot_addr,
};

static int cix_sfh_rproc_addr_init(struct cix_sfh_rproc *rproc_priv)
{
	struct device *dev = rproc_priv->dev;
	struct device_node *np = dev->of_node;
	struct cix_mem_region *mem_reg = rproc_priv->mem_region;
	struct cix_sfh_mem *p_mem;
	int err, nph, i;
	int r_num = 0;

	nph = of_count_phandle_with_args(np, "memory-region", NULL);
	if (nph <= 0)
		return 0;
	p_mem = devm_kcalloc(dev, nph, sizeof(*p_mem), GFP_KERNEL);
	if (!p_mem)
		return -ENOMEM;

	for (i = 0; i < nph; i++) {
		struct device_node *node;
		struct resource res;

		node = of_parse_phandle(np, "memory-region", i);
		err = of_address_to_resource(node, 0, &res);
		if (err) {
			dev_err(dev, "unable to find resolved memory region\n");
			return err;
		}

		of_node_put(node);

		p_mem[i].cpu_addr = devm_ioremap_wc(rproc_priv->dev, res.start,
							   resource_size(&res));
		if (!p_mem[i].cpu_addr) {
			dev_err(dev, "failed to remap %pr\n", &res);
			return -ENOMEM;
		}
		p_mem[i].bus_addr = res.start;
		p_mem[i].size = resource_size(&res);
#ifdef CONFIG_HIBERNATION
		if (!strncmp(res.name, "vdev0vring0", 11) ||
		    !strncmp(res.name, "vdev0vring1", 11)) {
			cix_rproc_reserve_ops[r_num].name = (char *)res.name;
			cix_rproc_reserve_ops[r_num].paddr = p_mem[i].bus_addr;
			cix_rproc_reserve_ops[r_num].size = p_mem[i].size;
			r_num++;
		}
#endif
		dev_dbg(dev, " %s, p_mem[%d].bus_addr: 0x%llx, size 0x%lx\n",
			__func__, i, p_mem[i].bus_addr, p_mem[i].size);
	}

	rproc_priv->mem = p_mem;
	rproc_priv->num_mem = nph;

#ifdef CONFIG_PLAT_BBOX
	if (p_mem[nph-1].cpu_addr) {
		g_rdr_dump_mem_region_0.name = mem_reg[0].name;
		g_rdr_dump_mem_region_0.va = p_mem[nph-1].cpu_addr;
		g_rdr_dump_mem_region_0.length = p_mem[nph-1].size;

		dev_dbg(dev, "rdr_mem_region_0 name = %s, va = %px, len = %lu\n",
			mem_reg[0].name, p_mem[nph-1].cpu_addr, p_mem[nph-1].size);
	}
#endif

	return 0;
}

static irqreturn_t cix_rproc_wdg(int irq, void *data)
{
	struct platform_device *pdev = data;
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct cix_sfh_rproc *rproc_priv = rproc->priv;

	/* Disable wdg_irq to avoid interrupt storm */
	disable_irq_nosync(rproc_priv->wdg_irq);

	rproc_report_crash(rproc, RPROC_WATCHDOG);

	return IRQ_HANDLED;
}

static irqreturn_t cix_rproc_sw_irq(int irq, void *data)
{
	struct platform_device *pdev = data;
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct cix_sfh_rproc *rproc_priv = rproc->priv;

	/* Disable sw_irq to avoid interrupt storm */
	disable_irq_nosync(rproc_priv->sw_irq);

	return IRQ_HANDLED;
}

static void cix_sfh_rproc_rx_callback(struct mbox_client *cl, void *data)
{
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	u32 *message = (u32 *)data;

	dev_dbg(rproc_priv->dev, " mbox rx msg: 0x%x 0x%x\n",
				message[0], message[MBOX_MSG_OFFSET]);

	switch (message[MBOX_MSG_OFFSET]) {
	case MBOX_MSG_REPROC_READY:
		rproc_priv->rproc_ready = true;
		break;
	case MBOX_MSG_SUSPEND_ACK:
	case MBOX_MSG_RESUME_ACK:
	case MBOX_MSG_SFH_COMPLETE:
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

static int cix_sendor_rproc_request_mbox(struct cix_sfh_rproc *rproc_priv)
{
	struct device *dev = rproc_priv->dev;
	struct mbox_client *cl;
	int ret;

	if (!of_get_property(dev->of_node, "mbox-names", NULL))
		return 0;

	cl = &rproc_priv->cl;
	cl->dev = dev;
	cl->tx_block = true;
	cl->tx_tout = MBOX_SEND_TIMEROUT;
	cl->knows_txdone = false;
	cl->rx_callback = cix_sfh_rproc_rx_callback;

	rproc_priv->tx_ch = mbox_request_channel_byname(cl, "tx1");
	if (IS_ERR(rproc_priv->tx_ch)) {
		ret = PTR_ERR(rproc_priv->tx_ch);
		dev_dbg(cl->dev, "failed to request tx mailbox channel: %d\n",
			ret);
		goto err_out;
	}

	rproc_priv->rx_ch = mbox_request_channel_byname(cl, "rx1");
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

static void cix_rproc_free_mbox(struct rproc *rproc)
{
	struct cix_sfh_rproc *priv = rproc->priv;

	mbox_free_channel(priv->tx_ch);
	mbox_free_channel(priv->rx_ch);
}

static void cix_sfh_mbox_vq_work(struct work_struct *work)
{
	struct cix_sfh_rproc *rproc_priv = container_of(work, struct cix_sfh_rproc,
							   rproc_work);
	rproc_vq_interrupt(rproc_priv->rproc, 0);
	rproc_vq_interrupt(rproc_priv->rproc, 1);
}

static void cix_sfh_rproc_load_firmware(const struct firmware *fw, void *context)
{
	struct rproc *rproc = context;

	rproc_boot(rproc);

	rproc->ops->kick(rproc, 0);

	release_firmware(fw);
}

static int cix_sfh_rproc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct cix_sfh_rproc *rproc_priv;
	struct rproc *rproc;
	const char *fw_name;
	int ret, irq;
	const void *dev_data;

	dev_dbg(dev, " %s\n", __func__);

	dev_data = of_device_get_match_data(dev);
	if (!dev_data)
		return -ENODEV;

	ret = rproc_of_parse_firmware(dev, 0, &fw_name);
	if (ret) {
		dev_err(dev, "failed to parse firmware-name property, ret = %d\n",
			ret);
		return ret;
	}

	rproc = rproc_alloc(dev, "cix-sfh-rproc", &cix_sfh_rproc_ops, fw_name,
			    sizeof(*rproc_priv));
	if (!rproc)
		return -ENOMEM;

	rproc_priv = rproc->priv;
	rproc_priv->rproc = rproc;
	rproc_priv->dev = dev;
	rproc_priv->mem_region = (struct cix_mem_region *)dev_data;
	rproc_priv->rproc_ready = false;

	ret = cix_sfh_get_clks(rproc_priv);
	if (ret) {
		dev_err(dev, "failed to get clocks\n");
		return ret;
	}

	rproc_priv->regmap = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "cix,sfh-ctrl");
	rproc_priv->offset = 0;
	if (IS_ERR(rproc_priv->regmap)) {
		dev_err(dev, "failed to find syscon\n");
		return PTR_ERR(rproc_priv->regmap);
	}

	rproc_priv->rst[RST_N] = devm_reset_control_get(dev, "reset");
	if (IS_ERR(rproc_priv->rst[RST_N]))
		return PTR_ERR(rproc_priv->rst[RST_N]);

	rproc_priv->rst[NOC_RST_N] = devm_reset_control_get(dev, "noc_reset");
	if (IS_ERR(rproc_priv->rst[NOC_RST_N]))
		return PTR_ERR(rproc_priv->rst[NOC_RST_N]);

	irq = platform_get_irq_byname(pdev, "wdt");
	if (irq == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (irq > 0) {
		ret = devm_request_irq(dev, irq, cix_rproc_wdg, 0,
				       dev_name(dev), pdev);
		if (ret) {
			dev_err(dev, "failed to request wdg irq\n");
			goto err_irq_req;
		}

		rproc_priv->wdg_irq = irq;

		if (of_property_read_bool(np, "wakeup-source")) {
			device_init_wakeup(dev, true);
			dev_pm_set_wake_irq(dev, irq);
		}

		dev_info(dev, "wdg irq registered\n");
	}

	irq = platform_get_irq_byname(pdev, "sw_intr");
	if (irq == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (irq > 0) {
		ret = devm_request_irq(dev, irq, cix_rproc_sw_irq,
				0, dev_name(dev), pdev);
		if (ret) {
			dev_err(dev, "failed to request sw irq\n");
			goto err_irq_req;
		}

		rproc_priv->sw_irq = irq;
	}

	dev_set_drvdata(dev, rproc);

	rproc_priv->workqueue = create_workqueue(dev_name(dev));
	if (!rproc_priv->workqueue) {
		dev_err(dev, "cannot create workqueue\n");
		ret = -ENOMEM;
		goto err_wkq;
	}

	INIT_WORK(&rproc_priv->rproc_work, cix_sfh_mbox_vq_work);

	ret = cix_sfh_rproc_addr_init(rproc_priv);
	if (ret) {
		dev_err(dev, "failed on cix_sfh_rproc_addr_init\n");
		goto err_addr;
	}

	cix_rcsu_remap_sfh();

	/* before loading fw image to sram, we need to release global noc reset */
	reset_control_assert(rproc_priv->rst[NOC_RST_N]);
	reset_control_deassert(rproc_priv->rst[NOC_RST_N]);

	/* enable sensor hub 24M clk and 400M clk in s5 */
	cix_sfh_clks_enable(rproc_priv);

	/* enable star and sram clock */
	cix_sfh_enable_clock(rproc_priv);

	sfh_hub2nic_remap(rproc_priv);
	sfh_sram_rst(rproc_priv);

	cix_sfh_mbox_clock_enable(rproc_priv);
	cix_sfh_mbox_reset(rproc_priv);
	cix_sfh_mbox_release_reset(rproc_priv);

	ret = cix_sendor_rproc_request_mbox(rproc_priv);
	if (ret) {
		dev_err(dev, "mbox_init failed\n");
		goto err_mbox_init;
	}

	init_completion(&rproc_priv->rsp_comp);
	rproc->auto_boot = of_property_read_bool(np, "cix,auto-boot");
	ret = rproc_add(rproc);
	if (ret) {
		dev_err(dev, "rproc_add failed\n");
		goto err_rproc_add;
	}

#ifdef CONFIG_PLAT_BBOX
	init_completion(&g_rdr_dump_comp);

	rdr_sky1_sf_register_exception();

	ret = rdr_sky1_sf_register_core();
	if (ret) {
		dev_err(dev, "rdr_sky1_sf_register_core fail, ret = [%d]\n", ret);
		goto err_rproc_add;
	}
#endif

	reinit_completion(&rproc_priv->rsp_comp);
        if (!wait_for_completion_timeout(&rproc_priv->rsp_comp,
                                           msecs_to_jiffies(SFH_COMP_TIMEOUT)))
                         return -EBUSY;

#ifdef CONFIG_HIBERNATION
	/*Add reserve memory to the save and restore queue of the STD process,
	this ensures that the data in the vring buffer used by the rpmsg driver is not lost.*/
	cix_rproc_hibernate_mem_reserve();
#endif

	dev_info(dev,"%s success\n",__func__);
	return 0;

err_rproc_add:
err_mbox_init:
err_addr:
	destroy_workqueue(rproc_priv->workqueue);
err_irq_req:
err_wkq:
	rproc_free(rproc);

	return ret;
}

static int cix_sfh_suspend(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	u32 msg[MBOX_MSG_LEN];
	int ret;

	msg[0] = MBOX_MSG_LEN;
	msg[MBOX_MSG_OFFSET] = MBOX_MSG_SYSTEM_SUSPEND;

	dev_info(dev, " %s\n", __func__);
	if (rproc->state == RPROC_RUNNING) {
		reinit_completion(&rproc_priv->rsp_comp);

		ret = mbox_send_message(rproc_priv->tx_ch, (void *)&msg);
		if (ret < 0) {
			dev_err(dev, "%s mbox_send_message failed: %d\n", __func__, ret);
			cix_sfh_mbox_dump_regs(rproc_priv);
			return ret;
		}

		if (!wait_for_completion_timeout(&rproc_priv->rsp_comp,
						 msecs_to_jiffies(PM_COMP_TIMEOUT)))
			return -EBUSY;
	}

	return 0;
}

static int cix_sfh_resume(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	u32 msg[MBOX_MSG_LEN];
	int ret;

	// set rcsu register to access sensor fusion when ap resume.
	cix_rcsu_remap_sfh();

	msg[0] = MBOX_MSG_LEN;
	msg[MBOX_MSG_OFFSET] = MBOX_MSG_SYSTEM_RESUME;

	dev_info(dev, " %s\n", __func__);
	if (rproc->state == RPROC_RUNNING) {
		reinit_completion(&rproc_priv->rsp_comp);

		ret = mbox_send_message(rproc_priv->tx_ch, (void *)&msg);
		if (ret < 0) {
			dev_err(dev, "%s mbox_send_message failed: %d\n", __func__, ret);
			cix_sfh_mbox_dump_regs(rproc_priv);
			return ret;
		}

		if (!wait_for_completion_timeout(&rproc_priv->rsp_comp,
						 msecs_to_jiffies(PM_COMP_TIMEOUT)))
			return -EBUSY;
	}

	return 0;
}

static int cix_sfh_restore(struct device *dev)
{
	int ret = 0;
	struct rproc *rproc = dev_get_drvdata(dev);
	struct cix_sfh_rproc *rproc_priv = rproc->priv;

 	cix_rcsu_remap_sfh();
	reset_control_assert(rproc_priv->rst[NOC_RST_N]);
	reset_control_deassert(rproc_priv->rst[NOC_RST_N]);

	ret = cix_sfh_clks_enable(rproc_priv);
	if (ret) {
		dev_err(dev, "cix_sfh_clks_enable failed\n");
		goto out;
	}

	cix_sfh_enable_clock(rproc_priv);

	sfh_hub2nic_remap(rproc_priv);
	sfh_sram_rst(rproc_priv);

	cix_sfh_mbox_clock_enable(rproc_priv);
	cix_sfh_mbox_reset(rproc_priv);
	cix_sfh_mbox_release_reset(rproc_priv);

	ret = cix_sendor_rproc_request_mbox(rproc_priv);
	if (ret) {
		dev_err(dev, "cix_sendor_rproc_request_mbox failed\n");
		goto out;
	}

	reinit_completion(&rproc_priv->rsp_comp);
	ret = request_firmware_nowait(THIS_MODULE,
					  1,
					  rproc->firmware,
					  dev,
					  GFP_KERNEL,
					  rproc,
					  cix_sfh_rproc_load_firmware);
	if (ret < 0) {
		dev_err(dev, "%s load firmware failed: %d\n", __func__, ret);
		goto out;
	}

	reinit_completion(&rproc_priv->rsp_comp);
	if (!wait_for_completion_timeout(&rproc_priv->rsp_comp,
		msecs_to_jiffies(SFH_COMP_TIMEOUT))) {
		dev_err(dev, "%s wait for completion timeout. \n", __func__);
		return -EBUSY;
	}

out:
	return ret;
}

static int cix_sfh_freeze(struct device *dev)
{
	int ret = 0;
	struct rproc *rproc = dev_get_drvdata(dev);
	struct cix_sfh_rproc *rproc_priv = rproc->priv;

	ret = rproc_shutdown(rproc);
	if (ret) {
		dev_err(dev, "%s rproc_shutdown failed\n", __func__);
		goto out;
	}

	cix_rproc_free_mbox(rproc);
	cix_sfh_mbox_reset(rproc_priv);
	cix_sfh_mbox_clock_disable(rproc_priv);
	cix_sfh_disable_clock(rproc_priv);
	cix_sfh_clks_disable(rproc_priv);

out:
	return ret;
}

static const struct dev_pm_ops cix_sfh_rproc_pm_ops = {
	.suspend = cix_sfh_suspend,
	.resume = cix_sfh_resume,
	.restore = cix_sfh_restore,
	.freeze = cix_sfh_freeze,
	.thaw = cix_sfh_restore,
};

static const struct of_device_id cix_sfh_rproc_of_match[] = {
	{ .compatible = "cix,sky1-sfh", .data = &sky1_sfh_mems, },
	{ },
};
MODULE_DEVICE_TABLE(of, cix_sfh_rproc_of_match);

static int cix_sfh_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct cix_sfh_rproc *rproc_priv = rproc->priv;
	struct device *dev = &pdev->dev;

#ifdef CONFIG_PLAT_BBOX
	rdr_sky1_sf_unregister_core();
	rdr_sky1_sf_unregister_exception();
#endif

#ifdef CONFIG_HIBERNATION
	cix_rproc_hibernate_mem_release();
#endif

	rproc_del(rproc);
	cix_rproc_free_mbox(rproc);
	cix_sfh_mbox_reset(rproc_priv);
	cix_sfh_mbox_clock_disable(rproc_priv);
	cix_sfh_disable_clock(rproc_priv);
	cix_sfh_clks_disable(rproc_priv);
	reset_control_assert(rproc_priv->rst[RST_N]);
	reset_control_assert(rproc_priv->rst[NOC_RST_N]);
	destroy_workqueue(rproc_priv->workqueue);

	if (device_may_wakeup(dev)) {
		dev_pm_clear_wake_irq(dev);
		device_init_wakeup(dev, false);
	}
	rproc_free(rproc);

	return 0;
}

static struct platform_driver cix_sfh_rproc_driver = {
	.probe = cix_sfh_rproc_probe,
	.remove = cix_sfh_rproc_remove,
	.driver = {
		.name = "cix-sfh-rproc",
		.of_match_table = cix_sfh_rproc_of_match,
		.pm = &cix_sfh_rproc_pm_ops,
	},
};
module_platform_driver(cix_sfh_rproc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CIX Sensor Fusion Hub Remote Processor Control Driver");
MODULE_AUTHOR("Lihua Liu <Lihua.Liu@cixcomputing.com>");

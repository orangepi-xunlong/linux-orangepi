/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*******************************************************************************
 * Copyright (C) 2016-2018, Allwinner Technology CO., LTD.
 * Author: zhuxianbin <zhuxianbin@allwinnertech.com>
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 ******************************************************************************/
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/iommu.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/sizes.h>
#include <linux/device.h>
#include <asm/cacheflush.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>
#include <linux/of_fdt.h>
#include <linux/libfdt.h>
#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <trace/hooks/iommu.h>
#endif

#include <sunxi-iommu.h>
#include "sunxi-iommu.h"

/*
 * Register of IOMMU device
 */
#define IOMMU_VERSION_REG			0x0000
#define IOMMU_RESET_REG				0x0010
#define IOMMU_ENABLE_REG			0x0020
#define IOMMU_BYPASS_REG			0x0030
#define IOMMU_AUTO_GATING_REG			0x0040
#define IOMMU_WBUF_CTRL_REG			0x0044
#define IOMMU_OOO_CTRL_REG			0x0048
#define IOMMU_4KB_BDY_PRT_CTRL_REG		0x004C
#define IOMMU_TTB_REG				0x0050
#define IOMMU_TLB_ENABLE_REG			0x0060
#define IOMMU_TLB_PREFETCH_REG			0x0070
#define IOMMU_TLB_FLUSH_ENABLE_REG		0x0080
#define IOMMU_TLB_IVLD_MODE_SEL_REG		0x0084
#define IOMMU_TLB_IVLD_START_ADDR_REG		0x0088
#define IOMMU_TLB_IVLD_END_ADDR_REG		0x008C
#define IOMMU_TLB_IVLD_ADDR_REG			0x0090
#define IOMMU_TLB_IVLD_ADDR_MASK_REG		0x0094
#define IOMMU_TLB_IVLD_ENABLE_REG		0x0098
#define IOMMU_PC_IVLD_MODE_SEL_REG		0x009C
#define IOMMU_PC_IVLD_ADDR_REG			0x00A0
#define IOMMU_PC_IVLD_START_ADDR_REG		0x00A4
#define IOMMU_PC_IVLD_ENABLE_REG		0x00A8
#define IOMMU_PC_IVLD_END_ADDR_REG		0x00Ac
#define IOMMU_DM_AUT_CTRL_REG0			0x00B0
#define IOMMU_DM_AUT_CTRL_REG1			0x00B4
#define IOMMU_DM_AUT_CTRL_REG2			0x00B8
#define IOMMU_DM_AUT_CTRL_REG3			0x00BC
#define IOMMU_DM_AUT_CTRL_REG4			0x00C0
#define IOMMU_DM_AUT_CTRL_REG5			0x00C4
#define IOMMU_DM_AUT_CTRL_REG6			0x00C8
#define IOMMU_DM_AUT_CTRL_REG7			0x00CC
#define IOMMU_DM_AUT_OVWT_REG			0x00D0
#define IOMMU_INT_ENABLE_REG			0x0100
#define IOMMU_INT_CLR_REG			0x0104
#define IOMMU_INT_STA_REG			0x0108
#define IOMMU_INT_ERR_ADDR_REG0			0x0110

#define IOMMU_INT_ERR_ADDR_REG1			0x0114
#define IOMMU_INT_ERR_ADDR_REG2			0x0118

#define IOMMU_INT_ERR_ADDR_REG3			0x011C
#define IOMMU_INT_ERR_ADDR_REG4			0x0120
#define IOMMU_INT_ERR_ADDR_REG5			0x0124

#define IOMMU_INT_ERR_ADDR_REG6			0x0128
#define IOMMU_INT_ERR_ADDR_REG7			0x0130
#define IOMMU_INT_ERR_ADDR_REG8			0x0134

#define IOMMU_INT_ERR_DATA_REG0			0x0150
#define IOMMU_INT_ERR_DATA_REG1			0x0154
#define IOMMU_INT_ERR_DATA_REG2			0x0158
#define IOMMU_INT_ERR_DATA_REG3			0x015C
#define IOMMU_INT_ERR_DATA_REG4			0x0160
#define IOMMU_INT_ERR_DATA_REG5			0x0164

#define IOMMU_INT_ERR_DATA_REG6			0x0168
#define IOMMU_INT_ERR_DATA_REG7			0x0170
#define IOMMU_INT_ERR_DATA_REG8			0x0174

#define IOMMU_L1PG_INT_REG			0x0180
#define IOMMU_L2PG_INT_REG			0x0184
#define IOMMU_VA_REG				0x0190
#define IOMMU_VA_DATA_REG			0x0194
#define IOMMU_VA_CONFIG_REG			0x0198
#define IOMMU_PMU_ENABLE_REG			0x0200
#define IOMMU_PMU_CLR_REG			0x0210
#define IOMMU_PMU_ACCESS_LOW_REG0		0x0230
#define IOMMU_PMU_ACCESS_HIGH_REG0		0x0234
#define IOMMU_PMU_HIT_LOW_REG0			0x0238
#define IOMMU_PMU_HIT_HIGH_REG0			0x023C
#define IOMMU_PMU_ACCESS_LOW_REG1		0x0240
#define IOMMU_PMU_ACCESS_HIGH_REG1		0x0244
#define IOMMU_PMU_HIT_LOW_REG1			0x0248
#define IOMMU_PMU_HIT_HIGH_REG1			0x024C
#define IOMMU_PMU_ACCESS_LOW_REG2		0x0250
#define IOMMU_PMU_ACCESS_HIGH_REG2		0x0254
#define IOMMU_PMU_HIT_LOW_REG2			0x0258
#define IOMMU_PMU_HIT_HIGH_REG2			0x025C
#define IOMMU_PMU_ACCESS_LOW_REG3		0x0260
#define IOMMU_PMU_ACCESS_HIGH_REG3		0x0264
#define IOMMU_PMU_HIT_LOW_REG3			0x0268
#define IOMMU_PMU_HIT_HIGH_REG3			0x026C
#define IOMMU_PMU_ACCESS_LOW_REG4		0x0270
#define IOMMU_PMU_ACCESS_HIGH_REG4		0x0274
#define IOMMU_PMU_HIT_LOW_REG4			0x0278
#define IOMMU_PMU_HIT_HIGH_REG4			0x027C
#define IOMMU_PMU_ACCESS_LOW_REG5		0x0280
#define IOMMU_PMU_ACCESS_HIGH_REG5		0x0284
#define IOMMU_PMU_HIT_LOW_REG5			0x0288
#define IOMMU_PMU_HIT_HIGH_REG5			0x028C

#define IOMMU_PMU_ACCESS_LOW_REG6		0x0290
#define IOMMU_PMU_ACCESS_HIGH_REG6		0x0294
#define IOMMU_PMU_HIT_LOW_REG6			0x0298
#define IOMMU_PMU_HIT_HIGH_REG6			0x029C
#define IOMMU_PMU_ACCESS_LOW_REG7		0x02D0
#define IOMMU_PMU_ACCESS_HIGH_REG7		0x02D4
#define IOMMU_PMU_HIT_LOW_REG7			0x02D8
#define IOMMU_PMU_HIT_HIGH_REG7			0x02DC
#define IOMMU_PMU_ACCESS_LOW_REG8		0x02E0
#define IOMMU_PMU_ACCESS_HIGH_REG8		0x02E4
#define IOMMU_PMU_HIT_LOW_REG8			0x02E8
#define IOMMU_PMU_HIT_HIGH_REG8			0x02EC

#define IOMMU_PMU_TL_LOW_REG0			0x0300
#define IOMMU_PMU_TL_HIGH_REG0			0x0304
#define IOMMU_PMU_ML_REG0			0x0308

#define IOMMU_PMU_TL_LOW_REG1			0x0310
#define IOMMU_PMU_TL_HIGH_REG1			0x0314
#define IOMMU_PMU_ML_REG1			0x0318

#define IOMMU_PMU_TL_LOW_REG2			0x0320
#define IOMMU_PMU_TL_HIGH_REG2			0x0324
#define IOMMU_PMU_ML_REG2			0x0328

#define IOMMU_PMU_TL_LOW_REG3			0x0330
#define IOMMU_PMU_TL_HIGH_REG3			0x0334
#define IOMMU_PMU_ML_REG3			0x0338

#define IOMMU_PMU_TL_LOW_REG4			0x0340
#define IOMMU_PMU_TL_HIGH_REG4			0x0344
#define IOMMU_PMU_ML_REG4			0x0348

#define IOMMU_PMU_TL_LOW_REG5			0x0350
#define IOMMU_PMU_TL_HIGH_REG5			0x0354
#define IOMMU_PMU_ML_REG5			0x0358

#define IOMMU_PMU_TL_LOW_REG6			0x0360
#define IOMMU_PMU_TL_HIGH_REG6			0x0364
#define IOMMU_PMU_ML_REG6			0x0368

#define IOMMU_RESET_SHIFT   31
#define IOMMU_RESET_MASK (1 << IOMMU_RESET_SHIFT)
#define IOMMU_RESET_SET (0 << 31)
#define IOMMU_RESET_RELEASE (1 << 31)

/*
 * IOMMU enable register field
 */
#define IOMMU_ENABLE	0x1

/*
 * IOMMU interrupt id mask
 */
#define MICRO_TLB0_INVALID_INTER_MASK   0x1
#define MICRO_TLB1_INVALID_INTER_MASK   0x2
#define MICRO_TLB2_INVALID_INTER_MASK   0x4
#define MICRO_TLB3_INVALID_INTER_MASK   0x8
#define MICRO_TLB4_INVALID_INTER_MASK   0x10
#define MICRO_TLB5_INVALID_INTER_MASK   0x20
#define MICRO_TLB6_INVALID_INTER_MASK   0x40

#define L1_PAGETABLE_INVALID_INTER_MASK   0x10000
#define L2_PAGETABLE_INVALID_INTER_MASK   0x20000

/**
 * sun8iw15p1
 *	DE :		masterID 0
 *	E_EDMA:		masterID 1
 *	E_FE:		masterID 2
 *	VE:		masterID 3
 *	CSI:		masterID 4
 *	G2D:		masterID 5
 *	E_BE:		masterID 6
 *
 * sun50iw9p1:
 *	DE :		masterID 0
 *	DI:			masterID 1
 *	VE_R:		masterID 2
 *	VE:			masterID 3
 *	CSI0:		masterID 4
 *	CSI1:		masterID 5
 *	G2D:		masterID 6
 * sun8iw19p1:
 *	DE :>--->-------masterID 0
 *	EISE:		masterID 1
 *	AI:		masterID 2
 *	VE:>---->-------masterID 3
 *	CSI:	>-->----masterID 4
 *	ISP:>-->------	masterID 5
 *	G2D:>--->-------masterID 6
 * sun8iw21:
 *	VE :			masterID 0
 *	CSI:			masterID 1
 *	DE:				masterID 2
 *	G2D:			masterID 3
 *	ISP:			masterID 4
 *	RISCV:			masterID 5
 *	NPU:			masterID 6
 */
#define DEFAULT_BYPASS_VALUE     0x7f
static const u32 master_id_bitmap[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40};

#define sunxi_wait_when(COND, MS) ({ \
	unsigned long timeout__ = jiffies + msecs_to_jiffies(MS) + 1;	\
	int ret__ = 0;							\
	while ((COND)) {						\
		if (time_after(jiffies, timeout__)) {			\
			ret__ = (!COND) ? 0 : -ETIMEDOUT;		\
			break;						\
		}							\
		udelay(1);					\
	}								\
	ret__;								\
})

/*
 * The format of device tree, and client device how to use it.
 *
 * /{
 *	....
 *	smmu: iommu@xxxxx {
 *		compatible = "allwinner,iommu";
 *		reg = <xxx xxx xxx xxx>;
 *		interrupts = <GIC_SPI xxx IRQ_TYPE_LEVEL_HIGH>;
 *		interrupt-names = "iommu-irq";
 *		clocks = <&iommu_clk>;
 *		clock-name = "iommu-clk";
 *		#iommu-cells = <1>;
 *		status = "enabled";
 *	};
 *
 *	de@xxxxx {
 *		.....
 *		iommus = <&smmu ID>;
 *	};
 *
 * }
 *
 * Here, ID number is 0 ~ 5, every client device have a unique id.
 * Every id represent a micro TLB, also represent a master device.
 *
 */

enum sunxi_iommu_version {
	IOMMU_VERSION_V10 = 0x10,
	IOMMU_VERSION_V11,
	IOMMU_VERSION_V12,
	IOMMU_VERSION_V13,
	IOMMU_VERSION_V14,
};

struct sunxi_iommu_plat_data {
	u32 version;
	u32 tlb_prefetch;
	u32 tlb_invalid_mode;
	u32 ptw_invalid_mode;
	const char *master[8];
};

struct sunxi_iommu_dev {
	struct iommu_device iommu;
	struct device *dev;
	void __iomem *base;
	struct clk *clk;
	int irq;
	u32 bypass;
	spinlock_t iommu_lock;
	struct list_head rsv_list;
	const struct sunxi_iommu_plat_data *plat_data;
};

struct sunxi_iommu_domain {
	unsigned int *pgtable;		/* first page directory, size is 16KB */
	u32 *sg_buffer;
	struct spinlock  dt_lock;	/* lock for modifying page table @ pgtable */
	struct dma_iommu_mapping *mapping;
	struct iommu_domain domain;
	/* struct iova_domain iovad; */
	/* list of master device, it represent a micro TLB */
	struct list_head mdevs;
	spinlock_t lock;
};

/*
 * sunxi master device which use iommu.
 */
struct sunxi_mdev {
	struct list_head node;	/* for sunxi_iommu mdevs list */
	struct device *dev;	/* the master device */
	unsigned int tlbid;	/* micro TLB id, distinguish device by it */
	bool flag;
};

struct sunxi_iommu_owner {
	unsigned int tlbid;
	bool flag;
	struct sunxi_iommu_dev *data;
	struct device *dev;
	struct dma_iommu_mapping *mapping;
};

#define _max(x, y) (((u64)(x) > (u64)(y)) ? (x) : (y))

static struct kmem_cache *iopte_cache;
static struct sunxi_iommu_dev *global_iommu_dev;
static struct sunxi_iommu_domain *global_sunxi_iommu_domain;
struct iommu_domain *global_iommu_domain;
static struct iommu_group *global_group;
static bool iommu_hw_init_flag;
static struct device *dma_dev;
EXPORT_SYMBOL_GPL(global_iommu_domain);

static sunxi_iommu_fault_cb sunxi_iommu_fault_notify_cbs[7];

#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
static struct sunxi_iommu_iova_info *info_head, *info_tail;
static struct sunxi_iommu_iova_info *info_head_free, *info_tail_free;
static struct dentry *iommu_debug_dir;

static int iova_show(struct seq_file *s, void *data)
{
	struct sunxi_iommu_iova_info *info_p;

	seq_puts(s, "device              iova_start     iova_end      size(byte)    timestamp(s)\n");
	for (info_p = info_head; info_p != NULL; info_p = info_p->next) {
		seq_printf(s, "%-16s    %8lx       %8lx      %-8d      %-lu\n", info_p->dev_name,
			info_p->iova, info_p->iova + info_p->size, info_p->size, info_p->timestamp);
	}

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(iova);

static int iova_show_on_irq(void)
{
	struct sunxi_iommu_iova_info *info_p;

	printk("device              iova_start     iova_end      size(byte)    timestamp(s)\n");
	for (info_p = info_head; info_p != NULL; info_p = info_p->next) {
		printk("%-16s    %8lx       %8lx      %-8d      %-lu\n", info_p->dev_name,
			info_p->iova, info_p->iova + info_p->size, info_p->size, info_p->timestamp);
	}
	printk("\n");

	printk("iova that has been freed===================================================\n");
	printk("device              iova_start     iova_end      size(byte)    timestamp(s)\n");
	for (info_p = info_head_free; info_p != NULL; info_p = info_p->next) {
		printk("%-16s    %8lx       %8lx      %-8d      %-lu\n", info_p->dev_name,
			info_p->iova, info_p->iova + info_p->size, info_p->size, info_p->timestamp);
	}
	printk("\n");

	return 0;
}

static void sunxi_iommu_alloc_iova(void *p, struct device *dev,
				struct iova_domain *domain,
				dma_addr_t iova, size_t size)
{
	struct sunxi_iommu_iova_info *info_p;

	if (info_head == NULL) {
		info_head = info_p = kmalloc(sizeof(*info_head), GFP_KERNEL);
		info_tail = info_head;
	} else {
		info_p = kmalloc(sizeof(*info_head), GFP_KERNEL);
		info_tail->next = info_p;
		info_tail = info_p;
	}
	strcpy(info_p->dev_name, dev_name(dev));
	info_p->iova = iova;
	info_p->size = size;
	info_p->timestamp = ktime_get_seconds();
}

static void sunxi_iommu_free_iova(void *p, struct iova_domain *domain,
				dma_addr_t iova, size_t size)
{
	struct sunxi_iommu_iova_info *info_p, *info_prev, *info_p_free;

	info_p = info_prev = info_head;
	WARN_ON(info_p == NULL);
	if (!info_p)
		return;
	for (; info_p != NULL; info_p = info_p->next) {
		if (info_p->iova == iova && info_p->size == size)
			break;
		info_prev = info_p;
	}

	if (info_head_free == NULL) {
		info_head_free = kmalloc(sizeof(*info_head_free), GFP_KERNEL);
		info_p_free = info_tail_free = info_head_free;
	} else {
		info_p_free = kmalloc(sizeof(*info_head_free), GFP_KERNEL);
		info_tail_free->next = info_p_free;
		info_tail_free = info_p_free;
	}
	strcpy(info_p_free->dev_name, info_p->dev_name);
	info_p_free->iova = info_p->iova;
	info_p_free->size = info_p->size;
	info_p_free->timestamp = info_p->timestamp;

	if (info_p == info_head)  {             /* free head */
		info_head = info_p->next;
	} else if (info_p == info_tail) {       /* free tail */
		info_tail = info_prev;
		info_tail->next = NULL;
	} else                                  /* free middle */
		info_prev->next = info_p->next;

	kfree(info_p);
	info_p = NULL;
}
#endif

void sunxi_iommu_register_fault_cb(sunxi_iommu_fault_cb cb, unsigned int master_id)
{
	sunxi_iommu_fault_notify_cbs[master_id] = cb;
}
EXPORT_SYMBOL_GPL(sunxi_iommu_register_fault_cb);

static inline u32 sunxi_iommu_read(struct sunxi_iommu_dev *iommu,
				   u32 offset)
{
	return readl(iommu->base + offset);
}

static inline void sunxi_iommu_write(struct sunxi_iommu_dev *iommu,
				     u32 offset, u32 value)
{
	writel(value, iommu->base + offset);
}

void sunxi_reset_device_iommu(unsigned int master_id)
{
	unsigned int regval;
	struct sunxi_iommu_dev *iommu = global_iommu_dev;

	regval = sunxi_iommu_read(iommu, IOMMU_RESET_REG);
	sunxi_iommu_write(iommu, IOMMU_RESET_REG, regval & (~(1 << master_id)));
	regval = sunxi_iommu_read(iommu, IOMMU_RESET_REG);
	if (!(regval & ((1 << master_id)))) {
		sunxi_iommu_write(iommu, IOMMU_RESET_REG, regval | ((1 << master_id)));
	}
}
EXPORT_SYMBOL(sunxi_reset_device_iommu);

void sunxi_enable_device_iommu(unsigned int master_id, bool flag)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	if (flag)
		iommu->bypass &= ~(master_id_bitmap[master_id]);
	else
		iommu->bypass |= master_id_bitmap[master_id];
	sunxi_iommu_write(iommu, IOMMU_BYPASS_REG, iommu->bypass);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
}
EXPORT_SYMBOL(sunxi_enable_device_iommu);

static int sunxi_tlb_flush(struct sunxi_iommu_dev *iommu)
{
	int ret;

	/* enable the maximum number(7) of master to fit all platform */
	sunxi_iommu_write(iommu, IOMMU_TLB_FLUSH_ENABLE_REG, 0x0003007f);
	ret = sunxi_wait_when(
		(sunxi_iommu_read(iommu, IOMMU_TLB_FLUSH_ENABLE_REG)), 2);
	if (ret)
		dev_err(iommu->dev, "Enable flush all request timed out\n");

	return ret;
}

static int sunxi_iommu_hw_init(struct iommu_domain *input_domain)
{
	int ret = 0;
	int iommu_enable = 0;
	phys_addr_t dte_addr;
	unsigned long mflag;
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(input_domain, struct sunxi_iommu_domain, domain);

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	dte_addr = __pa(sunxi_domain->pgtable);
	sunxi_iommu_write(iommu, IOMMU_TTB_REG, dte_addr);

	/*
	 * set preftech functions, including:
	 * master prefetching and only prefetch valid page to TLB/PTW
	 */
	sunxi_iommu_write(iommu, IOMMU_TLB_PREFETCH_REG, plat_data->tlb_prefetch);
	/* new TLB invalid function: use range(start, end) to invalid TLB, started at version V12 */
	if (plat_data->version >= IOMMU_VERSION_V12)
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_MODE_SEL_REG, plat_data->tlb_invalid_mode);
	/* new PTW invalid function: use range(start, end) to invalid PTW, started at version V14 */
	if (plat_data->version >= IOMMU_VERSION_V14)
		sunxi_iommu_write(iommu, IOMMU_PC_IVLD_MODE_SEL_REG, plat_data->ptw_invalid_mode);

	/* disable interrupt of prefetch */
	sunxi_iommu_write(iommu, IOMMU_INT_ENABLE_REG, 0x3003f);
	sunxi_iommu_write(iommu, IOMMU_BYPASS_REG, iommu->bypass);

	ret = sunxi_tlb_flush(iommu);
	if (ret) {
		dev_err(iommu->dev, "Enable flush all request timed out\n");
		goto out;
	}
	sunxi_iommu_write(iommu, IOMMU_AUTO_GATING_REG, 0x1);
	sunxi_iommu_write(iommu, IOMMU_ENABLE_REG, IOMMU_ENABLE);
	iommu_enable = sunxi_iommu_read(iommu, IOMMU_ENABLE_REG);
	if (iommu_enable != 0x1) {
		iommu_enable = sunxi_iommu_read(iommu, IOMMU_ENABLE_REG);
		if (iommu_enable != 0x1) {
			dev_err(iommu->dev, "iommu enable failed! No iommu in bitfile!\n");
			ret = -ENODEV;
			goto out;
		}
	}
	iommu_hw_init_flag = true;

out:
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return ret;
}

static int sunxi_tlb_invalid(dma_addr_t iova, dma_addr_t iova_mask)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	dma_addr_t iova_end = iova_mask;
	int ret = 0;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	/* new TLB invalid function: use range(start, end) to invalid TLB page */
	if (plat_data->version >= IOMMU_VERSION_V12) {
		pr_debug("iommu: TLB invalid:0x%x-0x%x\n", (unsigned int)iova,
			(unsigned int)iova_end);
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_START_ADDR_REG, iova);
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_END_ADDR_REG, iova_end);
	} else {
		/* old TLB invalid function: only invalid 4K at one time */
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_REG, iova);
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_MASK_REG, iova_mask);
	}
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x1);

	ret = sunxi_wait_when(
		(sunxi_iommu_read(iommu, IOMMU_TLB_IVLD_ENABLE_REG)&0x1), 2);
	if (ret) {
		dev_err(iommu->dev, "TLB cache invalid timed out\n");
	}
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return ret;
}

static int sunxi_ptw_cache_invalid(dma_addr_t iova_start, dma_addr_t iova_end)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	int ret = 0;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	/* new PTW invalid function: use range(start, end) to invalid PTW page */
	if (plat_data->version >= IOMMU_VERSION_V14) {
		pr_debug("iommu: PTW invalid:0x%x-0x%x\n", (unsigned int)iova_start,
			 (unsigned int)iova_end);
		WARN_ON(iova_end == 0);
		sunxi_iommu_write(iommu, IOMMU_PC_IVLD_START_ADDR_REG, iova_start);
		sunxi_iommu_write(iommu, IOMMU_PC_IVLD_END_ADDR_REG, iova_end);
	} else {
		/* old ptw invalid function: only invalid 1M at one time */
		pr_debug("iommu: PTW invalid:0x%x\n", (unsigned int)iova_start);
		sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ADDR_REG, iova_start);
	}
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x1);

	ret = sunxi_wait_when(
		(sunxi_iommu_read(iommu, IOMMU_PC_IVLD_ENABLE_REG)&0x1), 2);
	if (ret) {
		dev_err(iommu->dev, "PTW cache invalid timed out\n");
		goto out;
	}

out:
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return ret;
}

static void sunxi_zap_tlb(unsigned long iova, size_t size)
{
	const struct sunxi_iommu_plat_data *plat_data = global_iommu_dev->plat_data;

	if (plat_data->version <= IOMMU_VERSION_V11) {
		sunxi_tlb_invalid(iova, (u32)IOMMU_PT_MASK);
		sunxi_tlb_invalid(iova + SPAGE_SIZE, (u32)IOMMU_PT_MASK);
		sunxi_tlb_invalid(iova + size, (u32)IOMMU_PT_MASK);
		sunxi_tlb_invalid(iova + size + SPAGE_SIZE, (u32)IOMMU_PT_MASK);
		sunxi_ptw_cache_invalid(iova, 0);
		sunxi_ptw_cache_invalid(iova + SPD_SIZE, 0);
		sunxi_ptw_cache_invalid(iova + size, 0);
		sunxi_ptw_cache_invalid(iova + size + SPD_SIZE, 0);
	} else if (plat_data->version <= IOMMU_VERSION_V13) {
		sunxi_tlb_invalid(iova, iova + 2 * SPAGE_SIZE);
		sunxi_tlb_invalid(iova + size - SPAGE_SIZE, iova + size + 8 * SPAGE_SIZE);
		sunxi_ptw_cache_invalid(iova, 0);
		sunxi_ptw_cache_invalid(iova + size, 0);

		sunxi_ptw_cache_invalid(iova + SPD_SIZE, 0);
		sunxi_ptw_cache_invalid(iova + size + SPD_SIZE, 0);
		sunxi_ptw_cache_invalid(iova + size + 2 * SPD_SIZE, 0);
	} else {
		sunxi_tlb_invalid(iova, iova + 2 * SPAGE_SIZE);
		sunxi_tlb_invalid(iova + size - SPAGE_SIZE, iova + size + 8 * SPAGE_SIZE);
		sunxi_ptw_cache_invalid(iova, iova + SPD_SIZE);
		sunxi_ptw_cache_invalid(iova + size - SPD_SIZE, iova + size);
	}

	return;
}

static int sunxi_iommu_map(struct iommu_domain *domain, unsigned long iova,
			   phys_addr_t paddr, size_t size, int prot, gfp_t gfp)
{
	struct sunxi_iommu_domain *sunxi_domain;
	size_t iova_start, iova_end, paddr_start, s_iova_start;
	int ret;
	unsigned long flags;

	sunxi_domain = container_of(domain, struct sunxi_iommu_domain, domain);
	WARN_ON(sunxi_domain->pgtable == NULL);
	iova_start = iova & IOMMU_PT_MASK;
	paddr_start = paddr & IOMMU_PT_MASK;
	iova_end = SPAGE_ALIGN(iova + size);
	s_iova_start = iova_start;

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	ret = sunxi_pgtable_prepare_l1_tables(sunxi_domain->pgtable, iova_start,
					      iova_end, prot);
	if (ret) {
		spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);
		return -ENOMEM;
	}

	iova_start = s_iova_start;
	sunxi_pgtable_prepare_l2_tables(sunxi_domain->pgtable,
					iova_start, iova_end, paddr, prot);

	/*
	 * ioctl sync map have no iova argument in 5.10
	 * and force flush all, this may cause performance
	 * problem, we flush here with correct iova and size
	 * to minimalize flush range
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0) && LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	sunxi_zap_tlb(iova, size);
#endif
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return 0;
}

static size_t sunxi_iommu_unmap(struct iommu_domain *domain, unsigned long iova,
				size_t size, struct iommu_iotlb_gather *gather)
{
	struct sunxi_iommu_domain *sunxi_domain;
	const struct sunxi_iommu_plat_data *plat_data;
	size_t iova_start, iova_end;
	u32 iova_tail_size;
	unsigned long flags;

	sunxi_domain = container_of(domain, struct sunxi_iommu_domain, domain);
	plat_data = global_iommu_dev->plat_data;
	WARN_ON(sunxi_domain->pgtable == NULL);
	iova_start = iova & IOMMU_PT_MASK;
	iova_end = SPAGE_ALIGN(iova + size);

	if (gather->start > iova_start)
		gather->start = iova_start;
	if (gather->end < iova_end)
		gather->end = iova_end;

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	/* Invalid TLB and PTW */
	if (plat_data->version >= IOMMU_VERSION_V12)
		sunxi_tlb_invalid(iova_start, iova_end);
	if (plat_data->version >= IOMMU_VERSION_V14)
		sunxi_ptw_cache_invalid(iova_start, iova_end);

	for (; iova_start < iova_end; ) {
		iova_tail_size = sunxi_pgtable_delete_l2_tables(
			 sunxi_domain->pgtable, iova_start, iova_end);

		if (plat_data->version < IOMMU_VERSION_V14)
			sunxi_ptw_cache_invalid(iova_start, 0);
		iova_start += iova_tail_size;
	}
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return size;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static void sunxi_iommu_iotlb_sync_map(struct iommu_domain *domain,
				unsigned long iova, size_t size)
{
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(domain, struct sunxi_iommu_domain, domain);
	unsigned long flags;

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	sunxi_zap_tlb(iova, size);
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return;
}
#endif

static void sunxi_iommu_iotlb_sync(struct iommu_domain *domain,
			    struct iommu_iotlb_gather *iotlb_gather)
{
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(domain, struct sunxi_iommu_domain, domain);
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	unsigned long flags;

	if (plat_data->version >= IOMMU_VERSION_V14)
		return ;

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	sunxi_zap_tlb(iotlb_gather->start, iotlb_gather->end - iotlb_gather->start);
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return;
}

static phys_addr_t sunxi_iommu_iova_to_phys(struct iommu_domain *domain,
					    dma_addr_t iova)
{
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(domain, struct sunxi_iommu_domain, domain);
	phys_addr_t ret = 0;
	unsigned long flags;


	WARN_ON(sunxi_domain->pgtable == NULL);
	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	ret = sunxi_pgtable_iova_to_phys(sunxi_domain->pgtable, iova);
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return ret;
}

static struct iommu_domain *sunxi_iommu_domain_alloc(unsigned type)
{
	struct sunxi_iommu_domain *sunxi_domain;

	if (type != IOMMU_DOMAIN_DMA && type != IOMMU_DOMAIN_UNMANAGED)
		return NULL;

	/* we just use one domain */
	if (global_sunxi_iommu_domain)
		return &global_sunxi_iommu_domain->domain;

	sunxi_domain = kzalloc(sizeof(*sunxi_domain), GFP_KERNEL);

	if (!sunxi_domain)
		return NULL;

	sunxi_domain->pgtable = sunxi_pgtable_alloc();
	if (!sunxi_domain->pgtable) {
		pr_err("sunxi domain get pgtable failed\n");
		goto err_page;
	}

	sunxi_domain->sg_buffer = (unsigned int *)__get_free_pages(
				GFP_KERNEL, get_order(MAX_SG_TABLE_SIZE));
	if (!sunxi_domain->sg_buffer) {
		pr_err("sunxi domain get sg_buffer failed\n");
		goto err_sg_buffer;
	}

#ifndef COOKIE_HANDLE_BY_CORE
	if (type == IOMMU_DOMAIN_DMA &&
				iommu_get_dma_cookie(&sunxi_domain->domain)) {
		pr_err("sunxi domain get dma cookie failed\n");
		goto err_dma_cookie;
	}
#endif

	sunxi_domain->domain.geometry.aperture_start = 0;
	sunxi_domain->domain.geometry.aperture_end	 = (1ULL << 32)-1;
	sunxi_domain->domain.geometry.force_aperture = true;
	spin_lock_init(&sunxi_domain->dt_lock);
	global_sunxi_iommu_domain = sunxi_domain;
	global_iommu_domain = &sunxi_domain->domain;

	if (!iommu_hw_init_flag) {
		if (sunxi_iommu_hw_init(&sunxi_domain->domain))
			pr_err("sunxi iommu hardware init failed\n");
	}

	return &sunxi_domain->domain;

#ifndef COOKIE_HANDLE_BY_CORE
err_dma_cookie:
#endif
err_sg_buffer:
	sunxi_pgtable_free(sunxi_domain->pgtable);
	sunxi_domain->pgtable = NULL;
err_page:
	kfree(sunxi_domain);

	return NULL;
}

static void sunxi_iommu_domain_free(struct iommu_domain *domain)
{
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(domain, struct sunxi_iommu_domain, domain);
	unsigned long flags;

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	sunxi_pgtable_clear(sunxi_domain->pgtable);
	sunxi_tlb_flush(global_iommu_dev);
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);
	sunxi_pgtable_free(sunxi_domain->pgtable);
	sunxi_domain->pgtable = NULL;
	free_pages((unsigned long)sunxi_domain->sg_buffer,
						get_order(MAX_SG_TABLE_SIZE));
	sunxi_domain->sg_buffer = NULL;
#ifndef COOKIE_HANDLE_BY_CORE
	iommu_put_dma_cookie(domain);
#endif
	kfree(sunxi_domain);
}

static int sunxi_iommu_attach_dev(struct iommu_domain *domain,
				  struct device *dev)
{
	return 0;
}

#ifndef DETACH_OP_DEPRECATED
static void sunxi_iommu_detach_dev(struct iommu_domain *domain,
				   struct device *dev)
{
		return;
}
#endif

static void sunxi_iommu_probe_device_finalize(struct device *dev)
{
	struct sunxi_iommu_owner *owner = dev_iommu_priv_get(dev);

#if defined(CONFIG_ARM)
	WARN(*dev->dma_mask == 0, "0 dma mask will fail iommu setup\n");
	iommu_setup_dma_ops(dev, 0, *dev->dma_mask);
#endif
	sunxi_enable_device_iommu(owner->tlbid, owner->flag);
}

static struct iommu_device *sunxi_iommu_probe_device(struct device *dev)
{
	struct sunxi_iommu_owner *owner = dev_iommu_priv_get(dev);

	if (!owner) /* Not a iommu client device */
		return ERR_PTR(-ENODEV);

	return &owner->data->iommu;
}

static void sunxi_iommu_release_device(struct device *dev)
{
	struct sunxi_iommu_owner *owner = dev_iommu_priv_get(dev);

	if (!owner)
		return;

	sunxi_enable_device_iommu(owner->tlbid, false);
	dev->iommu_group = NULL;
	devm_kfree(dev, dev->dma_parms);
	dev->dma_parms = NULL;
	kfree(owner);
	owner = NULL;
	dev_iommu_priv_set(dev, NULL);
}

/* set dma params for master devices */
#ifndef GROUP_NOTIFIER_DEPRECATED
static int sunxi_iommu_set_dma_parms(struct notifier_block *nb,
			      unsigned long action, void *data)
{
	struct device *dev = data;

	if (action != IOMMU_GROUP_NOTIFY_BIND_DRIVER)
		return 0;

	dev->dma_parms = devm_kzalloc(dev, sizeof(*dev->dma_parms), GFP_KERNEL);
	if (!dev->dma_parms)
		return -ENOMEM;
	dma_set_max_seg_size(dev, DMA_BIT_MASK(32));

	return 0;
}
#endif

static struct iommu_group *sunxi_iommu_device_group(struct device *dev)
{
	struct iommu_group *group;
	struct notifier_block *nb;

	if (!global_group) {
		nb = kzalloc(sizeof(*nb), GFP_KERNEL);
		if (!nb)
			return ERR_PTR(-ENOMEM);

		global_group = iommu_group_alloc();
		if (IS_ERR(global_group)) {
			pr_err("sunxi iommu alloc group failed\n");
			goto err_group_alloc;
		}

#ifndef GROUP_NOTIFIER_DEPRECATED
		nb->notifier_call = sunxi_iommu_set_dma_parms;
		if (iommu_group_register_notifier(global_group, nb)) {
			pr_err("sunxi iommu group register notifier failed!\n");
			goto err_notifier;
		}
#endif

	}
	group = global_group;

	return group;

#ifndef GROUP_NOTIFIER_DEPRECATED
err_notifier:
#endif
err_group_alloc:
	kfree(nb);

	return ERR_PTR(-EBUSY);
}

static int sunxi_iommu_of_xlate(struct device *dev,
				struct of_phandle_args *args)
{
	struct sunxi_iommu_owner *owner = dev_iommu_priv_get(dev);
	struct platform_device *sysmmu = of_find_device_by_node(args->np);
	struct sunxi_iommu_dev *data;

	if (!sysmmu)
		return -ENODEV;

	data = platform_get_drvdata(sysmmu);
	if (data == NULL)
		return -ENODEV;

	if (!owner) {
		owner = kzalloc(sizeof(*owner), GFP_KERNEL);
		if (!owner)
			return -ENOMEM;
		owner->tlbid = args->args[0];
		owner->flag = args->args[1];
		owner->data = data;
		owner->dev = dev;
		dev_iommu_priv_set(dev, owner);
	}

	return 0;
}

void sunxi_set_debug_mode(void)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;

	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x80000000);
}

void sunxi_set_prefetch_mode(void)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;

	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x00000000);
}

int sunxi_iova_test_write(dma_addr_t iova, u32 val)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	int retval;

	sunxi_iommu_write(iommu, IOMMU_VA_REG, iova);
	sunxi_iommu_write(iommu, IOMMU_VA_DATA_REG, val);
	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x80000100);
	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x80000101);
	retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_VA_CONFIG_REG) & 0x1), 1);
	if (retval)
		dev_err(iommu->dev,
			"write VA address request timed out\n");
	return retval;
}

unsigned long sunxi_iova_test_read(dma_addr_t iova)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	unsigned long retval;

	sunxi_iommu_write(iommu, IOMMU_VA_REG, iova);
	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x80000000);
	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x80000001);
	retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_VA_CONFIG_REG) & 0x1), 1);
	if (retval) {
		dev_err(iommu->dev,
			"read VA address request timed out\n");
		retval = 0;
		goto out;
	}
	retval = sunxi_iommu_read(iommu,
			IOMMU_VA_DATA_REG);

out:
	return retval;
}

static int sunxi_iova_invalid_helper(unsigned long iova)
{
	struct sunxi_iommu_domain *sunxi_domain = global_sunxi_iommu_domain;

	return sunxi_pgtable_invalid_helper(sunxi_domain->pgtable, iova);
}

static irqreturn_t sunxi_iommu_irq(int irq, void *dev_id)
{

	u32 inter_status_reg = 0;
	u32 addr_reg = 0;
	u32	int_masterid_bitmap = 0;
	u32	data_reg = 0;
	u32	l1_pgint_reg = 0;
	u32	l2_pgint_reg = 0;
	u32	master_id = 0;
	unsigned long mflag;
	struct sunxi_iommu_dev *iommu = dev_id;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	inter_status_reg = sunxi_iommu_read(iommu, IOMMU_INT_STA_REG) & 0x3ffff;
	l1_pgint_reg = sunxi_iommu_read(iommu, IOMMU_L1PG_INT_REG);
	l2_pgint_reg = sunxi_iommu_read(iommu, IOMMU_L2PG_INT_REG);
	int_masterid_bitmap = inter_status_reg | l1_pgint_reg | l2_pgint_reg;

	if (inter_status_reg & MICRO_TLB0_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[0]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG0);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG0);
	} else if (inter_status_reg & MICRO_TLB1_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[1]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG1);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG1);
	} else if (inter_status_reg & MICRO_TLB2_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[2]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG2);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG2);
	} else if (inter_status_reg & MICRO_TLB3_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[3]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG3);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG3);
	} else if (inter_status_reg & MICRO_TLB4_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[4]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG4);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG4);
	} else if (inter_status_reg & MICRO_TLB5_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[5]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG5);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG5);
	} else if (inter_status_reg & MICRO_TLB6_INVALID_INTER_MASK) {
		pr_err("%s Invalid Authority\n", plat_data->master[6]);
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG6);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG6);
	} else if (inter_status_reg & L1_PAGETABLE_INVALID_INTER_MASK) {
		/* It's OK to prefetch an invalid page, no need to print msg for debug. */
		if (!(int_masterid_bitmap & (1U << 31)))
			pr_err("L1 PageTable Invalid\n");
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG7);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG7);
	} else if (inter_status_reg & L2_PAGETABLE_INVALID_INTER_MASK) {
		if (!(int_masterid_bitmap & (1U << 31)))
			pr_err("L2 PageTable Invalid\n");
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG8);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG8);
	} else
		pr_err("sunxi iommu int error!!!\n");

	if (!(int_masterid_bitmap & (1U << 31))) {
		sunxi_iova_invalid_helper(addr_reg);
		int_masterid_bitmap &= 0xffff;
		master_id = __ffs(int_masterid_bitmap);
		pr_err("Bug is in %s module, invalid address: 0x%x, data:0x%x, id:0x%x\n",
			plat_data->master[master_id], addr_reg, data_reg,
				int_masterid_bitmap);

#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
		iova_show_on_irq();
#endif
		/* master debug callback */
		if (sunxi_iommu_fault_notify_cbs[master_id])
			sunxi_iommu_fault_notify_cbs[master_id]();
	}

	/* invalid TLB */
	if (plat_data->version <= IOMMU_VERSION_V11) {
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_REG, addr_reg);
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_MASK_REG, (u32)IOMMU_PT_MASK);
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x1);
		while (sunxi_iommu_read(iommu, IOMMU_TLB_IVLD_ENABLE_REG) & 0x1)
			;
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_REG, addr_reg + 0x2000);
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_MASK_REG, (u32)IOMMU_PT_MASK);
	} else {
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_START_ADDR_REG, addr_reg);
		sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_END_ADDR_REG, addr_reg + 4 * SPAGE_SIZE);
	}
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x1);
	while (sunxi_iommu_read(iommu, IOMMU_TLB_IVLD_ENABLE_REG) & 0x1)
		;

	/* invalid PTW */
	if (plat_data->version <= IOMMU_VERSION_V13) {
		sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ADDR_REG, addr_reg);
		sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x1);
		while (sunxi_iommu_read(iommu, IOMMU_PC_IVLD_ENABLE_REG) & 0x1)
			;
		sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ADDR_REG, addr_reg + 0x200000);
	} else {
		sunxi_iommu_write(iommu, IOMMU_PC_IVLD_START_ADDR_REG, addr_reg);
		sunxi_iommu_write(iommu, IOMMU_PC_IVLD_END_ADDR_REG, addr_reg + 2 * SPD_SIZE);
	}
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x1);
	while (sunxi_iommu_read(iommu, IOMMU_PC_IVLD_ENABLE_REG) & 0x1)
		;

	sunxi_iommu_write(iommu, IOMMU_INT_CLR_REG, inter_status_reg);
	inter_status_reg |= (l1_pgint_reg | l2_pgint_reg);
	inter_status_reg &= 0xffff;
	sunxi_iommu_write(iommu, IOMMU_RESET_REG, ~inter_status_reg);
	sunxi_iommu_write(iommu, IOMMU_RESET_REG, 0xffffffff);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return IRQ_HANDLED;
}

static ssize_t sunxi_iommu_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	u32 data;

	spin_lock(&iommu->iommu_lock);
	data = sunxi_iommu_read(iommu, IOMMU_PMU_ENABLE_REG);
	spin_unlock(&iommu->iommu_lock);

	return scnprintf(buf, PAGE_SIZE,
		"enable = %d\n", data & 0x1 ? 1 : 0);
}

static ssize_t sunxi_iommu_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	unsigned long val;
	u32 data;
	int retval;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val) {
		spin_lock(&iommu->iommu_lock);
		data = sunxi_iommu_read(iommu, IOMMU_PMU_ENABLE_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_ENABLE_REG, data | 0x1);
		data = sunxi_iommu_read(iommu, IOMMU_PMU_CLR_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_CLR_REG, data | 0x1);
		retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_PMU_CLR_REG) & 0x1), 1);
		if (retval)
			dev_err(iommu->dev, "Clear PMU Count timed out\n");
		spin_unlock(&iommu->iommu_lock);
	} else {
		spin_lock(&iommu->iommu_lock);
		data = sunxi_iommu_read(iommu, IOMMU_PMU_CLR_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_CLR_REG, data | 0x1);
		retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_PMU_CLR_REG) & 0x1), 1);
		if (retval)
			dev_err(iommu->dev, "Clear PMU Count timed out\n");
		data = sunxi_iommu_read(iommu, IOMMU_PMU_ENABLE_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_ENABLE_REG, data & ~0x1);
		spin_unlock(&iommu->iommu_lock);
	}

	return count;
}

static ssize_t sunxi_iommu_profilling_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	struct {
		u64 macrotlb_access_count;
		u64 macrotlb_hit_count;
		u64 ptwcache_access_count;
		u64 ptwcache_hit_count;
		struct {
			u64 access_count;
			u64 hit_count;
			u64 latency;
			u32 max_latency;
		} micro_tlb[7];
	} *iommu_profile;
	iommu_profile = kmalloc(sizeof(*iommu_profile), GFP_KERNEL);
	if (!iommu_profile)
		goto err;
	spin_lock(&iommu->iommu_lock);

	iommu_profile->micro_tlb[0].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG0) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG0);
	iommu_profile->micro_tlb[0].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG0) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG0);

	iommu_profile->micro_tlb[1].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG1) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG1);
	iommu_profile->micro_tlb[1].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG1) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG1);

	iommu_profile->micro_tlb[2].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG2) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG2);
	iommu_profile->micro_tlb[2].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG2) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG2);

	iommu_profile->micro_tlb[3].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG3) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG3);
	iommu_profile->micro_tlb[3].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG3) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG3);

	iommu_profile->micro_tlb[4].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG4) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG4);
	iommu_profile->micro_tlb[4].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG4) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG4);

	iommu_profile->micro_tlb[5].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG5) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG5);
	iommu_profile->micro_tlb[5].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG5) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG5);

	iommu_profile->micro_tlb[6].access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG6) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG6);
	iommu_profile->micro_tlb[6].hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG6) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG6);

	iommu_profile->macrotlb_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG7) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG7);
	iommu_profile->macrotlb_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG7) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG7);

	iommu_profile->ptwcache_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG8) &
		       0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG8);
	iommu_profile->ptwcache_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG8) & 0x7ff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG8);

	iommu_profile->micro_tlb[0].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG0) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG0);
	iommu_profile->micro_tlb[1].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG1) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG1);
	iommu_profile->micro_tlb[2].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG2) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG2);
	iommu_profile->micro_tlb[3].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG3) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG3);
	iommu_profile->micro_tlb[4].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG4) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG4);
	iommu_profile->micro_tlb[5].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG5) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG5);

	iommu_profile->micro_tlb[6].latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG6) &
		       0x3ffff)
		 << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG6);

	iommu_profile->micro_tlb[0].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG0);
	iommu_profile->micro_tlb[1].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG1);
	iommu_profile->micro_tlb[2].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG2);
	iommu_profile->micro_tlb[3].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG3);
	iommu_profile->micro_tlb[4].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG4);
	iommu_profile->micro_tlb[5].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG5);
	iommu_profile->micro_tlb[6].max_latency =
		sunxi_iommu_read(iommu, IOMMU_PMU_ML_REG6);

	spin_unlock(&iommu->iommu_lock);
err:
	kfree(iommu_profile);

	return scnprintf(
		buf, PAGE_SIZE,
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"macrotlb_access_count = 0x%llx\n"
		"macrotlb_hit_count = 0x%llx\n"
		"ptwcache_access_count = 0x%llx\n"
		"ptwcache_hit_count = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_total_latency = 0x%llx\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n"
		"%s_max_latency = 0x%x\n",
		plat_data->master[0], iommu_profile->micro_tlb[0].access_count,
		plat_data->master[0], iommu_profile->micro_tlb[0].hit_count,
		plat_data->master[1], iommu_profile->micro_tlb[1].access_count,
		plat_data->master[1], iommu_profile->micro_tlb[1].hit_count,
		plat_data->master[2], iommu_profile->micro_tlb[2].access_count,
		plat_data->master[2], iommu_profile->micro_tlb[2].hit_count,
		plat_data->master[3], iommu_profile->micro_tlb[3].access_count,
		plat_data->master[3], iommu_profile->micro_tlb[3].hit_count,
		plat_data->master[4], iommu_profile->micro_tlb[4].access_count,
		plat_data->master[4], iommu_profile->micro_tlb[4].hit_count,
		plat_data->master[5], iommu_profile->micro_tlb[5].access_count,
		plat_data->master[5], iommu_profile->micro_tlb[5].hit_count,
		plat_data->master[6], iommu_profile->micro_tlb[6].access_count,
		plat_data->master[6], iommu_profile->micro_tlb[6].hit_count,
		iommu_profile->macrotlb_access_count,
		iommu_profile->macrotlb_hit_count,
		iommu_profile->ptwcache_access_count,
		iommu_profile->ptwcache_hit_count, plat_data->master[0],
		iommu_profile->micro_tlb[0].latency, plat_data->master[1],
		iommu_profile->micro_tlb[1].latency, plat_data->master[2],
		iommu_profile->micro_tlb[2].latency, plat_data->master[3],
		iommu_profile->micro_tlb[3].latency, plat_data->master[4],
		iommu_profile->micro_tlb[4].latency, plat_data->master[5],
		iommu_profile->micro_tlb[5].latency, plat_data->master[6],
		iommu_profile->micro_tlb[6].latency, plat_data->master[0],
		iommu_profile->micro_tlb[0].max_latency, plat_data->master[1],
		iommu_profile->micro_tlb[1].max_latency, plat_data->master[2],
		iommu_profile->micro_tlb[2].max_latency, plat_data->master[3],
		iommu_profile->micro_tlb[3].max_latency, plat_data->master[4],
		iommu_profile->micro_tlb[4].max_latency, plat_data->master[5],
		iommu_profile->micro_tlb[5].max_latency, plat_data->master[6],
		iommu_profile->micro_tlb[6].max_latency);
}

ssize_t sunxi_iommu_dump_pgtable(char *buf, size_t buf_len,
					       bool for_sysfs_show)
{
	/* walk and dump */
	struct sunxi_iommu_domain *sunxi_domain = container_of(
		global_iommu_domain, struct sunxi_iommu_domain, domain);
	ssize_t len = 0;

	len = sunxi_iommu_dump_rsv_list(&global_iommu_dev->rsv_list, len, buf,
					buf_len, for_sysfs_show);
	len = sunxi_pgtable_dump(sunxi_domain->pgtable, len, buf, buf_len,
				 for_sysfs_show);
	return len;
}
EXPORT_SYMBOL_GPL(sunxi_iommu_dump_pgtable);

static ssize_t sunxi_iommu_map_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	return sunxi_iommu_dump_pgtable(buf, PAGE_SIZE, true);
}

static struct device_attribute sunxi_iommu_enable_attr =
	__ATTR(enable, 0644, sunxi_iommu_enable_show,
	sunxi_iommu_enable_store);
static struct device_attribute sunxi_iommu_profilling_attr =
	__ATTR(profilling, 0444, sunxi_iommu_profilling_show, NULL);
static struct device_attribute sunxi_iommu_map_attr =
	__ATTR(page_debug, 0444, sunxi_iommu_map_show, NULL);

static void sunxi_iommu_sysfs_create(struct platform_device *_pdev,
				struct sunxi_iommu_dev *sunxi_iommu)
{
	device_create_file(&_pdev->dev, &sunxi_iommu_enable_attr);
	device_create_file(&_pdev->dev, &sunxi_iommu_profilling_attr);
	device_create_file(&_pdev->dev, &sunxi_iommu_map_attr);
#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
	iommu_debug_dir = debugfs_create_dir("iommu_sunxi", NULL);
	if (!iommu_debug_dir) {
		pr_err("%s: create iommu debug dir failed!\n", __func__);
		return;
	}
	debugfs_create_file("iova", 0444, iommu_debug_dir, sunxi_iommu, &iova_fops);
#endif
}

static void sunxi_iommu_sysfs_remove(struct platform_device *_pdev)
{
	device_remove_file(&_pdev->dev, &sunxi_iommu_enable_attr);
	device_remove_file(&_pdev->dev, &sunxi_iommu_profilling_attr);
	device_remove_file(&_pdev->dev, &sunxi_iommu_map_attr);
#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
	debugfs_remove(iommu_debug_dir);
#endif
}

static int __init_reserve_mem(struct sunxi_iommu_dev *dev)
{
	return bus_for_each_dev(&platform_bus_type, NULL, &dev->rsv_list,
			sunxi_iommu_check_cmd);
}

static void sunxi_iommu_get_resv_regions(struct device *dev,
					 struct list_head *head)
{
	struct iommu_resv_region *entry;
	struct iommu_resv_region *region;

	if (list_empty(&global_iommu_dev->rsv_list))
		__init_reserve_mem(global_iommu_dev);

	if (list_empty(&global_iommu_dev->rsv_list))
		return;
	list_for_each_entry (entry, &global_iommu_dev->rsv_list, list) {
		region = iommu_alloc_resv_region(entry->start, entry->length,
						 entry->prot, entry->type
#ifdef RESV_REGION_NEED_GFP_FLAG
						 ,
						 GFP_KERNEL
#endif
		);
		list_add_tail(&region->list, head);
	}
}

#ifdef SEPERATE_DOMAIN_API
static const struct iommu_domain_ops sunxi_iommu_domain_ops = {
	.attach_dev	= sunxi_iommu_attach_dev,
#ifndef DETACH_OP_DEPRECATED
	.detach_dev	= sunxi_iommu_detach_dev,
#endif
	.map		= sunxi_iommu_map,
	.unmap		= sunxi_iommu_unmap,
	.iotlb_sync_map = sunxi_iommu_iotlb_sync_map,
	.iova_to_phys	= sunxi_iommu_iova_to_phys,
	.iotlb_sync	= sunxi_iommu_iotlb_sync,
	.free		= sunxi_iommu_domain_free,
};
static const struct iommu_ops sunxi_iommu_ops = {
	.pgsize_bitmap =
		SZ_4K | SZ_16K | SZ_64K | SZ_256K | SZ_1M | SZ_4M | SZ_16M,
	.domain_alloc	    = sunxi_iommu_domain_alloc,
	.probe_device	    = sunxi_iommu_probe_device,
	.probe_finalize = sunxi_iommu_probe_device_finalize,
	.release_device	    = sunxi_iommu_release_device,
	.device_group	    = sunxi_iommu_device_group,
	.of_xlate	    = sunxi_iommu_of_xlate,
	.default_domain_ops = &sunxi_iommu_domain_ops,
	.owner		    = THIS_MODULE,
	.get_resv_regions = sunxi_iommu_get_resv_regions,
};

#else
static const struct iommu_ops sunxi_iommu_ops = {
	.pgsize_bitmap = SZ_4K | SZ_16K | SZ_64K | SZ_256K | SZ_1M | SZ_4M | SZ_16M,
	.map  = sunxi_iommu_map,
	.unmap = sunxi_iommu_unmap,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	.iotlb_sync_map = sunxi_iommu_iotlb_sync_map,
#endif
	.iotlb_sync = sunxi_iommu_iotlb_sync,
	.domain_alloc = sunxi_iommu_domain_alloc,
	.domain_free = sunxi_iommu_domain_free,
	.attach_dev = sunxi_iommu_attach_dev,
	.detach_dev = sunxi_iommu_detach_dev,
	.get_resv_regions = sunxi_iommu_get_resv_regions,
	.probe_device = sunxi_iommu_probe_device,
	.probe_finalize = sunxi_iommu_probe_device_finalize,
	.release_device = sunxi_iommu_release_device,
	.device_group	= sunxi_iommu_device_group,
	.of_xlate = sunxi_iommu_of_xlate,
	.iova_to_phys = sunxi_iommu_iova_to_phys,
	.owner = THIS_MODULE,
};
#endif

static int sunxi_iommu_probe(struct platform_device *pdev)
{
	int ret, irq;
	struct device *dev = &pdev->dev;
	struct sunxi_iommu_dev *sunxi_iommu;
	struct resource *res;

	iopte_cache = sunxi_pgtable_alloc_pte_cache();
	if (!iopte_cache) {
		pr_err("%s: Failed to create sunx-iopte-cache.\n", __func__);
		return -ENOMEM;
	}
	if (!iopte_cache) {
		pr_err("%s: Failed to create sunx-iopte-cache.\n", __func__);
		return -ENOMEM;
	}

	sunxi_iommu = devm_kzalloc(dev, sizeof(*sunxi_iommu), GFP_KERNEL);
	if (!sunxi_iommu)
		return	-ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_dbg(dev, "Unable to find resource region\n");
		ret = -ENOENT;
		goto err_res;
	}

	sunxi_iommu->base = devm_ioremap_resource(&pdev->dev, res);
	if (!sunxi_iommu->base) {
		dev_dbg(dev, "Unable to map IOMEM @ PA:%#x\n",
				(unsigned int)res->start);
		ret = -ENOENT;
		goto err_res;
	}

	sunxi_iommu->bypass = DEFAULT_BYPASS_VALUE;

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_dbg(dev, "Unable to find IRQ resource\n");
		ret = -ENOENT;
		goto err_irq;
	}
	pr_info("sunxi iommu: irq = %d\n", irq);

	ret = devm_request_irq(dev, irq, sunxi_iommu_irq, 0,
			dev_name(dev), (void *)sunxi_iommu);
	if (ret < 0) {
		dev_dbg(dev, "Unabled to register interrupt handler\n");
		goto err_irq;
	}

	sunxi_iommu->irq = irq;

	sunxi_iommu->clk = of_clk_get_by_name(dev->of_node, "iommu");
	if (IS_ERR(sunxi_iommu->clk)) {
		sunxi_iommu->clk = NULL;
		dev_dbg(dev, "Unable to find clock\n");
		ret = -ENOENT;
		goto err_clk;
	}
	clk_prepare_enable(sunxi_iommu->clk);

	platform_set_drvdata(pdev, sunxi_iommu);
	sunxi_iommu->dev = dev;
	spin_lock_init(&sunxi_iommu->iommu_lock);
	global_iommu_dev = sunxi_iommu;
	sunxi_iommu->plat_data = of_device_get_match_data(dev);

	if (sunxi_iommu->plat_data->version !=
			sunxi_iommu_read(sunxi_iommu, IOMMU_VERSION_REG)) {
		dev_err(dev, "iommu version mismatch, please check and reconfigure\n");
		goto err_clk;
	}

	sunxi_iommu_sysfs_create(pdev, sunxi_iommu);
	ret = iommu_device_sysfs_add(&sunxi_iommu->iommu, dev, NULL,
				     dev_name(dev));
	if (ret) {
		dev_err(dev, "Failed to register iommu in sysfs\n");
		goto err_clk;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = iommu_device_register(&sunxi_iommu->iommu, &sunxi_iommu_ops, dev);
#else
	iommu_device_set_ops(&sunxi_iommu->iommu, &sunxi_iommu_ops);
	iommu_device_set_fwnode(&sunxi_iommu->iommu, dev->fwnode);
	ret = iommu_device_register(&sunxi_iommu->iommu);
#endif
	if (ret) {
		dev_err(dev, "Failed to register iommu\n");
		goto err_clk;
	}

#ifndef BUS_SET_OP_DEPRECATED
	bus_set_iommu(&platform_bus_type, &sunxi_iommu_ops);
#endif

	INIT_LIST_HEAD(&sunxi_iommu->rsv_list);

	if (!dma_dev) {
		dma_dev = &pdev->dev;
		sunxi_pgtable_set_dma_dev(dma_dev);
	}

#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = register_trace_android_vh_iommu_iovad_alloc_iova(sunxi_iommu_alloc_iova, NULL) ?:
		register_trace_android_vh_iommu_iovad_free_iova(sunxi_iommu_free_iova, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	ret = register_trace_android_vh_iommu_alloc_iova(sunxi_iommu_alloc_iova, NULL) ?:
		register_trace_android_vh_iommu_free_iova(sunxi_iommu_free_iova, NULL);
#endif
	if (ret)
		pr_err("%s: register android vendor hook failed!\n", __func__);
#endif

	return 0;

err_clk:
	devm_free_irq(dev, irq, sunxi_iommu);
err_irq:
	devm_iounmap(dev, sunxi_iommu->base);
err_res:
	sunxi_pgtable_free_pte_cache(iopte_cache);
	dev_err(dev, "Failed to initialize\n");

	return ret;
}

static int sunxi_iommu_remove(struct platform_device *pdev)
{
	struct sunxi_iommu_dev *sunxi_iommu = platform_get_drvdata(pdev);
	struct iommu_resv_region *entry, *next;

	sunxi_pgtable_free_pte_cache(iopte_cache);
	if (!list_empty(&sunxi_iommu->rsv_list)) {
		list_for_each_entry_safe (entry, next, &sunxi_iommu->rsv_list,
					  list)
			kfree(entry);
	}
#ifndef BUS_SET_OP_DEPRECATED
	bus_set_iommu(&platform_bus_type, NULL);
#endif
	devm_free_irq(sunxi_iommu->dev, sunxi_iommu->irq, sunxi_iommu);
	devm_iounmap(sunxi_iommu->dev, sunxi_iommu->base);
	sunxi_iommu_sysfs_remove(pdev);
	iommu_device_sysfs_remove(&sunxi_iommu->iommu);
	iommu_device_unregister(&sunxi_iommu->iommu);
	global_iommu_dev = NULL;

	return 0;
}

static int sunxi_iommu_suspend(struct device *dev)
{
	clk_disable_unprepare(global_iommu_dev->clk);

	return 0;
}

static int sunxi_iommu_resume(struct device *dev)
{
	int err;

	clk_prepare_enable(global_iommu_dev->clk);

	if (unlikely(!global_sunxi_iommu_domain))
		return 0;

	err = sunxi_iommu_hw_init(&global_sunxi_iommu_domain->domain);

	return err;
}

static const struct dev_pm_ops sunxi_iommu_pm_ops = {
	.suspend	= sunxi_iommu_suspend,
	.resume		= sunxi_iommu_resume,
};

static const struct sunxi_iommu_plat_data iommu_v10_sun50iw6_data = {
	.version = 0x10,
	.tlb_prefetch = 0x7f,
	.tlb_invalid_mode = 0x0,
	.ptw_invalid_mode = 0x0,
	.master = {"DE", "VE_R", "DI", "VE", "CSI", "VP9"},
};

static const struct sunxi_iommu_plat_data iommu_v11_sun8iw15_data = {
	.version = 0x11,
	.tlb_prefetch = 0x5f,
	.tlb_invalid_mode = 0x0,
	.ptw_invalid_mode = 0x0,
	.master = {"DE", "E_EDMA", "E_FE", "VE", "CSI",
			"G2D", "E_BE", "DEBUG_MODE"},
};

static const struct sunxi_iommu_plat_data iommu_v12_sun8iw19_data = {
	.version = 0x12,
	.tlb_prefetch = 0x0,
	.tlb_invalid_mode = 0x1,
	.ptw_invalid_mode = 0x0,
	.master = {"DE", "EISE", "AI", "VE", "CSI",
			"ISP", "G2D", "DEBUG_MODE"},
};

static const struct sunxi_iommu_plat_data iommu_v12_sun50iw9_data = {
	.version = 0x12,
	/* disable preftech for performance issue */
	.tlb_prefetch = 0x0,
	.tlb_invalid_mode = 0x1,
	.ptw_invalid_mode = 0x0,
	.master = {"DE", "DI", "VE_R", "VE", "CSI0",
			"CSI1", "G2D", "DEBUG_MODE"},
};

static const struct sunxi_iommu_plat_data iommu_v13_sun50iw10_data = {
	.version = 0x13,
	/* disable preftech for performance issue */
	.tlb_prefetch = 0x0,
	.tlb_invalid_mode = 0x1,
	.ptw_invalid_mode = 0x0,
	.master = {"DE0", "DE1", "VE", "CSI", "ISP",
			"G2D", "EINK", "DEBUG_MODE"},
};

static const struct sunxi_iommu_plat_data iommu_v14_sun50iw12_data = {
	.version = 0x14,
	.tlb_prefetch = 0x3007f,
	.tlb_invalid_mode = 0x1,
	.ptw_invalid_mode = 0x1,
	.master = {"VE", "VE_R", "TVD_MBUS", "TVD_AXI", "TVCAP",
			"AV1", "TVFE", "DEBUG_MODE"},
};

static const struct sunxi_iommu_plat_data iommu_v14_sun50iw15_data = {
	.version = 0x14,
	.tlb_prefetch = 0x3007f,
	.tlb_invalid_mode = 0x1,
	.ptw_invalid_mode = 0x1,
	.master = {"VE", "VE_R", "TVD_MBUS", "TVD_AXI", "TVCAP",
			"KSC", "TVFE", "DEBUG_MODE"},
};

static const struct sunxi_iommu_plat_data iommu_v14_sun8iw20_data = {
	.version = 0x14,
	.tlb_prefetch = 0x30016, /* disable preftech on G2D/VE for better performance */
	.tlb_invalid_mode = 0x1,
	.ptw_invalid_mode = 0x1,
	.master = {"VE", "CSI", "DE", "G2D", "DI", "DEBUG_MODE"},
};

static const struct sunxi_iommu_plat_data iommu_v14_sun8iw21_data = {
	.version = 0x14,
	.tlb_prefetch = 0x30077,
	.tlb_invalid_mode = 0x1,
	.ptw_invalid_mode = 0x1,
	.master = {"VE", "CSI", "DE", "G2D", "ISP", "RISCV", "NPU", "DEBUG_MODE"},
};

static const struct sunxi_iommu_plat_data iommu_v15_sun55iw3_data = {
	.version = 0x15,
	/* disable preftech to test display rcq bug */
	.tlb_prefetch = 0x30000,
	.tlb_invalid_mode = 0x1,
	.ptw_invalid_mode = 0x1,
	.master = {"ISP", "CSI", "VE0", "VE1", "G2D", "DE",
			"DI", "DEBUG_MODE"},
};

static const struct of_device_id sunxi_iommu_dt_ids[] = {
	{ .compatible = "allwinner,iommu-v10-sun50iw6", .data = &iommu_v10_sun50iw6_data},
	{ .compatible = "allwinner,iommu-v11-sun8iw15", .data = &iommu_v11_sun8iw15_data},
	{ .compatible = "allwinner,iommu-v12-sun8iw19", .data = &iommu_v12_sun8iw19_data},
	{ .compatible = "allwinner,iommu-v12-sun50iw9", .data = &iommu_v12_sun50iw9_data},
	{ .compatible = "allwinner,iommu-v13-sun50iw10", .data = &iommu_v13_sun50iw10_data},
	{ .compatible = "allwinner,iommu-v14-sun50iw12", .data = &iommu_v14_sun50iw12_data},
	{ .compatible = "allwinner,iommu-v14-sun50iw15", .data = &iommu_v14_sun50iw15_data},
	{ .compatible = "allwinner,iommu-v14-sun8iw20", .data = &iommu_v14_sun8iw20_data},
	{ .compatible = "allwinner,iommu-v14-sun8iw21", .data = &iommu_v14_sun8iw21_data},
	{ .compatible = "allwinner,iommu-v14-sun20iw1", .data = &iommu_v14_sun8iw20_data},
	{ .compatible = "allwinner,iommu-v15-sun55iw3", .data = &iommu_v15_sun55iw3_data},
	{ /* sentinel */ },
};

static struct platform_driver sunxi_iommu_driver = {
	.probe		= sunxi_iommu_probe,
	.remove		= sunxi_iommu_remove,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "sunxi-iommu",
		.pm 		= &sunxi_iommu_pm_ops,
		.of_match_table = sunxi_iommu_dt_ids,
	}
};

static int __init sunxi_iommu_init(void)
{
	return platform_driver_register(&sunxi_iommu_driver);
}

static void __exit sunxi_iommu_exit(void)
{
	return platform_driver_unregister(&sunxi_iommu_driver);
}

subsys_initcall(sunxi_iommu_init);
module_exit(sunxi_iommu_exit);

MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.5.1");
MODULE_AUTHOR("huangshuosheng<huangshuosheng@allwinnertech.com>");
MODULE_AUTHOR("ouayngkun<ouyangkun@allwinnertech.com>");

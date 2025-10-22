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
#include <linux/reset.h>
#include <linux/pm_domain.h>

#include "sunxi-iommu.h"
#include "sunxi-iommu-pgtable.h"

/*
 * Register of IOMMU device
 */
#define IOMMU_VERSION_REG 0x0000
#define IOMMU_RESET_REG 0x0010
#define IOMMU_ENABLE_REG 0x0014
#define IOMMU_AUTO_GATING_REG 0x001c
#define IOMMU_AUTO_GATING_DEFAULT_VAL 0x7fff0003
#define IOMMU_TTB_REG 0x0020
#define IOMMU_TLB_ENABLE_REG 0x0028
#define IOMMU_TLB_PREFETCH_REG 0x002c
#define IOMMU_TLB_FLUSH_ENABLE_REG 0x0030
#define IOMMU_TLB_IVLD_MODE_SEL_REG 0x0040
#define IOMMU_TLB_IVLD_START_ADDR_REG 0x0044
#define IOMMU_TLB_IVLD_END_ADDR_REG 0x0048
#define IOMMU_TLB_IVLD_ADDR_REG 0x004c
#define IOMMU_TLB_IVLD_ADDR_MASK_REG 0x0050
#define IOMMU_TLB_IVLD_ENABLE_REG 0x0054
#define IOMMU_TLB_IVLD_ADDR_ALIGN_SHIFT (12)
#define IOMMU_TLB_IVLD_ADDR_OFFSET (0)
#define IOMMU_PC_IVLD_MODE_SEL_REG 0x0060
#define IOMMU_PC_IVLD_ADDR_REG 0x0064
#define IOMMU_PC_IVLD_START_ADDR_REG 0x0068
#define IOMMU_PC_IVLD_ENABLE_REG 0x006c
#define IOMMU_PC_IVLD_END_ADDR_REG 0x0070
#define IOMMU_PC_IVLD_ADDR_ALIGN_SHIFT (22)
#define IOMMU_PC_IVLD_ADDR_OFFSET (0)
#define IOMMU_INT_ENABLE_REG 0x080
#define IOMMU_INT_CLR_REG 0x0084
#define IOMMU_INT_STA_REG 0x0088

#define IOMMU_INT_ERR_LOW_ADDR7_REG 0x008C
#define IOMMU_INT_ERR_HIGH_ADDR7_REG 0x0090

#define IOMMU_INT_ERR_LOW_ADDR8_REG 0x0094
#define IOMMU_INT_ERR_HIGH_ADDR8_REG 0x0098

#define IOMMU_INT_ERR_DATA7_REG 0x009C
#define IOMMU_INT_ERR_DATA8_REG 0x00a0

#define IOMMU_L1PG_INT_REG 0x00b0
#define IOMMU_L2PG_INT_REG 0x00b4
#define IOMMU_LOW_VA_REG 0x00c0
#define IOMMU_HIGH_VA_REG 0x00c4
#define IOMMU_VA_DATA_REG 0x00c8
#define IOMMU_VA_CONFIG_REG 0x00cc
#define IOMMU_PMU_ENABLE_REG 0x00e0
#define IOMMU_PMU_CLR_REG 0x00e4
#define IOMMU_PVT_HANG_EN 0x00e8
#define IOMMU_PVT_HANG_PGTB 0x00ec
#define IOMMU_PMU_ACCESS_LOW7_REG 0x00f0
#define IOMMU_PMU_ACCESS_HIGH7_REG 0x00f4
#define IOMMU_PMU_HIT_LOW7_REG 0x00f8
#define IOMMU_PMU_HIT_HIGH7_REG 0x00fc
#define IOMMU_PMU_ACCESS_LOW8_REG 0x0100
#define IOMMU_PMU_ACCESS_HIGH8_REG 0x0104
#define IOMMU_PMU_HIT_LOW8_REG 0x0108
#define IOMMU_PMU_HIT_HIGH8_REG 0x010c

/* per-master micro TLB related */
#define IOMMU_MIC_BASE_OFFSET 0x400
#define IOMMU_MIC_MAX_MASTER 6
#define IOMMU_MIC_AUTO_GATING_REG(master_id) \
	(0x200 * master_id + 0x0000 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_BYP_REG(master_id) \
	(0x200 * master_id + 0x0004 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_WBUF_CTRL_REG(master_id) \
	(0x200 * master_id + 0x0008 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_OOO_REG(master_id) \
	(0x200 * master_id + 0x000c + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_4KB_BDY_PRT_REG(master_id) \
	(0x200 * master_id + 0x0010 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_TLB_ENABLE_REG(master_id) \
	(0x200 * master_id + 0x0014 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_TLB_FLUSH_ENABLE_REG(master_id) \
	(0x200 * master_id + 0x0018 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_DM_AUT_CTRL_REG(master_id) \
	(0x200 * master_id + 0x0020 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_DM_AUT_OVWT_REG(master_id) \
	(0x200 * master_id + 0x0024 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_INT_ENABLE_REG(master_id) \
	(0x200 * master_id + 0x0030 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_INT_CLR_REG(master_id) \
	(0x200 * master_id + 0x0034 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_INT_STA_REG(master_id) \
	(0x200 * master_id + 0x0038 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_INT_ERR_LOW_ADDR_REG(master_id) \
	(0x200 * master_id + 0x003c + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_INT_ERR_HIGH_ADDR_REG(master_id) \
	(0x200 * master_id + 0x0040 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_INT_ERR_DATA_REG(master_id) \
	(0x200 * master_id + 0x0044 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PMU_ENABLE_REG(master_id) \
	(0x200 * master_id + 0x0050 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PMU_CLR_REG(master_id) \
	(0x200 * master_id + 0x0054 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PMU_ACCESS_LOW_REG(master_id) \
	(0x200 * master_id + 0x0060 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PMU_ACCESS_HIGH_REG(master_id) \
	(0x200 * master_id + 0x0064 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PMU_HIT_LOW_REG(master_id) \
	(0x200 * master_id + 0x0068 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PMU_HIT_HIGH_REG(master_id) \
	(0x200 * master_id + 0x006c + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PMU_TL_LOW_REG(master_id) \
	(0x200 * master_id + 0x0080 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PMU_TL_HIGH_REG(master_id) \
	(0x200 * master_id + 0x0084 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PMU_ML_REG(master_id) \
	(0x200 * master_id + 0x0088 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PVT_HANG_EN_REG(master_id) \
	(0x200 * master_id + 0x0090 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PVT_HANG_ADDR_REG(master_id) \
	(0x200 * master_id + 0x0094 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_PVT_HANG_AUTH_REG(master_id) \
	(0x200 * master_id + 0x0098 + IOMMU_MIC_BASE_OFFSET)
#define IOMMU_MIC_GEN_WLAST_EN_REG(master_id) \
	(0x200 * master_id + 0x009c + IOMMU_MIC_BASE_OFFSET)

#define IOMMU_PER_SET_ADDR_SIZE 0x10000
#define IOMMU_HW_SET_COUNT 2
/*
 * IOMMU enable register field
 */
#define IOMMU_ENABLE 0x1

#define IOMMU_INT_L1PG_CLR_EN_MASK 0x1
#define IOMMU_INT_L2PG_CLR_EN_MASK 0x2
#define IOMMU_INT_L1PG_STA_MASK (1 << 16)
#define IOMMU_INT_L2PG_STA_MASK (1 << 17)

#define IOMMU_DUMP(addr)                                            \
	do {                                                        \
		pr_err("%llx: %x\n",                                \
		       (uint64_t)global_iommu_dev->base + (addr),   \
		       sunxi_iommu_read(global_iommu_dev, (addr))); \
	} while (0)

#define IOMMU_DUMP_BOTH(addr)                               \
	do {                                                \
		IOMMU_DUMP(addr);                           \
		IOMMU_DUMP(addr + IOMMU_PER_SET_ADDR_SIZE); \
	} while (0)

#define DEFAULT_BYPASS_VALUE 0x3ff
static const u32 master_id_bitmap[] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40 };

#define sunxi_wait_when(COND, MS)                                             \
	({                                                                    \
		unsigned long timeout__ = jiffies + msecs_to_jiffies(MS) + 1; \
		int ret__ = 0;                                                \
		while ((COND)) {                                              \
			if (time_after(jiffies, timeout__)) {                 \
				ret__ = (!COND) ? 0 : -ETIMEDOUT;             \
				break;                                        \
			}                                                     \
			udelay(1);                                            \
		}                                                             \
		ret__;                                                        \
	})

/* for reg that should be same at every instance */
#define WRITE_EQUAL_REG(iommu, offset_in_instance, val)                      \
	do {                                                                 \
		sunxi_iommu_write(iommu, offset_in_instance, val);           \
		sunxi_iommu_write(                                           \
			iommu, offset_in_instance + IOMMU_PER_SET_ADDR_SIZE, \
			val);                                                \
	} while (0)

#define READ_EQUAL_REG(iommu, offset_in_instance)                             \
	({                                                                    \
		int ret[2] = { 0xA55A3CC3, 0xA55A3CC3 }; /*poison*/           \
		ret[0] = sunxi_iommu_read(iommu, offset_in_instance);         \
		ret[1] = sunxi_iommu_read(                                    \
			iommu, offset_in_instance + IOMMU_PER_SET_ADDR_SIZE); \
		/* TBD: equal assertion for rets */                           \
		ret[0];                                                       \
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
	char *master[IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT];
};

struct sunxi_iommu_domain {
	unsigned int *pgtable; /* first page directory, size is 16KB */
	u32 *sg_buffer;
	struct spinlock dt_lock; /* lock for modifying page table @ pgtable */
	struct dma_iommu_mapping *mapping;
	struct iommu_domain domain;
	//struct iova_domain iovad;
	/* list of master device, it represent a micro TLB */
	struct list_head mdevs;
	spinlock_t lock;
};

struct sunxi_iommu_master_dev {
	struct device *dev;
	u32 id;
};

struct sunxi_iommu_dev {
	struct iommu_device iommu;
	struct device *dev;
	void __iomem *base;
	struct clk **clk;
	int irq[IOMMU_HW_SET_COUNT];
	u32 bypass;
	spinlock_t iommu_lock;
	struct iommu_domain *domain;
	struct list_head rsv_list;
	struct sunxi_iommu_plat_data *plat_data;
	struct reset_control **rst;
	u32 skip_mask;
	struct sunxi_iommu_master_dev *master;
};
static struct class *distribute_master_cs;

/*
 * sunxi master device which use iommu.
 */
struct sunxi_mdev {
	struct list_head node; /* for sunxi_iommu mdevs list */
	struct device *dev; /* the master device */
	unsigned int tlbid; /* micro TLB id, distinguish device by it */
	bool flag;
};

struct sunxi_iommu_owner {
	unsigned int tlbid;
	bool flag;
	struct sunxi_iommu_dev *data;
	struct device *dev;
	struct dma_iommu_mapping *mapping;
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
//iommu domain have seperate ops
#define SEPERATE_DOMAIN_API
//dma-iommu is enclosed into iommu-core
#define DMA_IOMMU_IN_IOMMU
//not used anywhere since refactoring
#define GROUP_NOTIFIER_DEPRECATED
//iommu now have correct probe order
//no more need bus set op as workaround
#define BUS_SET_OP_DEPRECATED
//dma cookie handled by iommu core, not driver
#define COOKIE_HANDLE_BY_CORE
#endif

#ifdef DMA_IOMMU_IN_IOMMU
#include <linux/iommu.h>
/*
 * by design iommu driver should be part of iommu
 * and get to it by ../../dma-iommu.h
 * sunxi bsp have seperate root, use different path
 * to reach dma-iommu.h
 */
#include <../drivers/iommu/dma-iommu.h>
#else
#include <linux/dma-iommu.h>
#endif

#define _max(x, y) (((u64)(x) > (u64)(y)) ? (x) : (y))

static struct kmem_cache *iopte_cache;
static struct sunxi_iommu_dev *global_iommu_dev;
static struct iommu_group *global_group;
static bool iommu_hw_init_flag;
static struct device *dma_dev;
struct iommu_domain *global_iommu_domain;
EXPORT_SYMBOL_GPL(global_iommu_domain);

typedef void (*sunxi_iommu_fault_cb)(void);
static sunxi_iommu_fault_cb
	sunxi_iommu_fault_notify_cbs[IOMMU_HW_SET_COUNT * IOMMU_MIC_MAX_MASTER];

void sunxi_iommu_register_fault_cb(sunxi_iommu_fault_cb cb,
				   unsigned int master_id)
{
	sunxi_iommu_fault_notify_cbs[master_id] = cb;
}
EXPORT_SYMBOL_GPL(sunxi_iommu_register_fault_cb);

static inline u32 sunxi_iommu_read(struct sunxi_iommu_dev *iommu, u32 offset)
{
	if (unlikely((offset & (IOMMU_PER_SET_ADDR_SIZE - 1)) >
		     IOMMU_MIC_BASE_OFFSET)) {
		int master_id = 0;
		u32 read;
		u32 test_offset = offset;
		if (test_offset > IOMMU_PER_SET_ADDR_SIZE) {
			test_offset -= IOMMU_PER_SET_ADDR_SIZE;
			master_id += 6;
		}
		test_offset -= IOMMU_MIC_BASE_OFFSET;
		master_id += test_offset / 0x200;
		if (iommu->skip_mask & (1 << master_id))
			return 0;

		if (iommu->master[master_id].dev)
			WARN_ON(pm_runtime_get_if_in_use(
					iommu->master[master_id].dev) <= 0);
		read = readl(iommu->base + offset);
		if (iommu->master[master_id].dev)
			pm_runtime_put(iommu->master[master_id].dev);

		return read;
	}
	return readl(iommu->base + offset);
}

static inline void sunxi_iommu_write(struct sunxi_iommu_dev *iommu, u32 offset,
				     u32 value)
{
	if (unlikely((offset & (IOMMU_PER_SET_ADDR_SIZE - 1)) >
		     IOMMU_MIC_BASE_OFFSET)) {
		int master_id = 0;
		u32 test_offset = offset;
		if (test_offset > IOMMU_PER_SET_ADDR_SIZE) {
			test_offset -= IOMMU_PER_SET_ADDR_SIZE;
			master_id += 6;
		}
		test_offset -= IOMMU_MIC_BASE_OFFSET;
		master_id += test_offset / 0x200;
		if (iommu->skip_mask & (1 << master_id))
			return;

		if (iommu->master[master_id].dev)
			WARN_ON(pm_runtime_get_if_in_use(
					iommu->master[master_id].dev) <= 0);

		writel(value, iommu->base + offset);
		if (iommu->master[master_id].dev)
			pm_runtime_put(iommu->master[master_id].dev);

		return;
	}
	writel(value, iommu->base + offset);
}

static void sunxi_iommu_distribute_mater_get(int mask)
{
	int i;
	for (i = 0; i < IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT; i++) {
		if (!(mask & (1 << i)))
			continue;
		if (global_iommu_dev->master[i].dev)
			pm_runtime_get_sync(global_iommu_dev->master[i].dev);
	}
}
static void sunxi_iommu_distribute_mater_put(int mask)
{
	int i;
	for (i = 0; i < IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT; i++) {
		if (!(mask & (1 << i)))
			continue;
		if (global_iommu_dev->master[i].dev)
			pm_runtime_put(global_iommu_dev->master[i].dev);
	}
}

__maybe_unused static void __dump_master_reg(int masterid)
{
	unsigned int instance_offset =
		(masterid / IOMMU_MIC_MAX_MASTER) * IOMMU_PER_SET_ADDR_SIZE;
	masterid = masterid % IOMMU_MIC_MAX_MASTER;
	print_hex_dump(KERN_ERR, "dump master", DUMP_PREFIX_OFFSET, 16, 4,
		       global_iommu_dev->base + instance_offset +
			       IOMMU_MIC_BASE_OFFSET + masterid * 0x200,
		       0xA0, false);
}

void sunxi_iommu_dump_mastger_reg(int masterid)
{
	__dump_master_reg(masterid);
}

__maybe_unused static void __dump_reg(int instance_idx)
{
	print_hex_dump(KERN_ERR, "dump reg", DUMP_PREFIX_OFFSET, 16, 4,
		       global_iommu_dev->base +
			       IOMMU_PER_SET_ADDR_SIZE * instance_idx,
		       0x120, false);
}

void sunxi_reset_device_iommu(unsigned int master_id)
{
	unsigned int regval;
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	/* we got serval instance at this iobase, get instance idx and in instance offset */
	unsigned int insatnce_offset =
		(master_id / IOMMU_MIC_MAX_MASTER) * IOMMU_PER_SET_ADDR_SIZE;
	master_id = master_id % IOMMU_MIC_MAX_MASTER;

	regval = sunxi_iommu_read(iommu, IOMMU_RESET_REG + insatnce_offset);
	sunxi_iommu_write(iommu, IOMMU_RESET_REG + insatnce_offset,
			  regval & (~(1 << master_id)));
	regval = sunxi_iommu_read(iommu, IOMMU_RESET_REG + insatnce_offset);
	if (!(regval & ((1 << master_id)))) {
		sunxi_iommu_write(iommu, IOMMU_RESET_REG + insatnce_offset,
				  regval | ((1 << master_id)));
	}
}
EXPORT_SYMBOL(sunxi_reset_device_iommu);

static void __sunxi_enable_device_iommu_unlocked(unsigned int master_id, bool flag)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	int insatnce_offset;
	if (flag)
		iommu->bypass &= ~(1 << master_id);
	else
		iommu->bypass |= (1 << master_id);

	/* bypass is global, perform per instance fix up after bypass updated */
	/* we got serval instance at this iobase, get instance idx and in instance offset */
	insatnce_offset =
		(master_id / IOMMU_MIC_MAX_MASTER) * IOMMU_PER_SET_ADDR_SIZE;
	master_id = master_id % IOMMU_MIC_MAX_MASTER;

	sunxi_iommu_write(iommu, insatnce_offset + IOMMU_MIC_BYP_REG(master_id),
			  flag == 0);
}

void sunxi_enable_device_iommu(unsigned int master_id, bool flag)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	unsigned long mflag;

	sunxi_iommu_distribute_mater_get(1 << master_id);
	spin_lock_irqsave(&iommu->iommu_lock, mflag);

	__sunxi_enable_device_iommu_unlocked(master_id, flag);

	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
	sunxi_iommu_distribute_mater_put(1 << master_id);
}
EXPORT_SYMBOL(sunxi_enable_device_iommu);

static int sunxi_tlb_flush(struct sunxi_iommu_dev *iommu)
{
	int ret = 0;
	int i, j;
	u32 target_reg_offset;
	/* flush all possible master */
	for (i = 0; i < IOMMU_HW_SET_COUNT; i++) {
		j = 0;
		target_reg_offset = IOMMU_PER_SET_ADDR_SIZE * i +
				    IOMMU_TLB_FLUSH_ENABLE_REG;
		sunxi_iommu_write(iommu, target_reg_offset, 0x00000003);

		for (; j < IOMMU_MIC_MAX_MASTER; j++) {
			target_reg_offset = IOMMU_PER_SET_ADDR_SIZE * i +
					    IOMMU_MIC_TLB_FLUSH_ENABLE_REG(j);
			sunxi_iommu_write(iommu, target_reg_offset, 0x00000001);
		}
	}
	udelay(10);//wait for hardware done
	for (i = 0; i < IOMMU_HW_SET_COUNT; i++) {
		j = 0;
		target_reg_offset = IOMMU_PER_SET_ADDR_SIZE * i +
				    IOMMU_TLB_FLUSH_ENABLE_REG;
		sunxi_iommu_write(iommu, target_reg_offset, 0x00000000);
	}

	return ret;
}

static void __sunxi_iommu_enable_interrupt_unlocked(int enable)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	int i;

	if (enable)
		WRITE_EQUAL_REG(iommu, IOMMU_INT_ENABLE_REG,
				IOMMU_INT_L1PG_CLR_EN_MASK |
					IOMMU_INT_L2PG_CLR_EN_MASK);
	else
		WRITE_EQUAL_REG(iommu, IOMMU_INT_ENABLE_REG, 0x0);

	/* we write nothing else but 0/1 into register */
	enable = !!enable;
	for (i = 0; i < IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT; i++) {
		int masterid = i;
		unsigned int instance_offset =
			(masterid / IOMMU_MIC_MAX_MASTER) *
			IOMMU_PER_SET_ADDR_SIZE;
		masterid = masterid % IOMMU_MIC_MAX_MASTER;
		sunxi_iommu_write(iommu,
				  instance_offset +
					  IOMMU_MIC_INT_ENABLE_REG(masterid),
				  enable);
	}
}
void sunxi_iommu_enable_interrupt(int enable)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	unsigned long mflag;

	sunxi_iommu_distribute_mater_get(0xffffffff);
	spin_lock_irqsave(&iommu->iommu_lock, mflag);

	__sunxi_iommu_enable_interrupt_unlocked(enable);

	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
	sunxi_iommu_distribute_mater_put(0xffffffff);
}
EXPORT_SYMBOL(sunxi_iommu_enable_interrupt);

void __sunxi_iommu_prevent_hang_enable_unlocked(int enable)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	int i;
	/* we write nothing else but 0/1 into register */
	enable = enable ? 1 : 0;

	WRITE_EQUAL_REG(iommu, IOMMU_PVT_HANG_EN, enable);

	/*
	 * macro on with micro off is not an option for
	 * the iommu implement, so en/disable all master at once
	 */
	for (i = 0; i < IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT; i++) {
		int masterid = i;
		unsigned int instance_offset =
			(masterid / IOMMU_MIC_MAX_MASTER) *
			IOMMU_PER_SET_ADDR_SIZE;
		masterid = masterid % IOMMU_MIC_MAX_MASTER;
		sunxi_iommu_write(iommu,
				  instance_offset +
					  IOMMU_MIC_PVT_HANG_EN_REG(masterid),
				  enable);
		sunxi_iommu_write(iommu,
				  instance_offset +
					  IOMMU_MIC_PVT_HANG_AUTH_REG(masterid),
				  enable);
	}
#if IS_ENABLED(DEBUG)
	__dump_reg(0);
	__dump_reg(1);
#endif
}
void sunxi_iommu_prevent_hang_enable(int enable)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	unsigned long mflag;

	sunxi_iommu_distribute_mater_get(0xffffffff);
	spin_lock_irqsave(&iommu->iommu_lock, mflag);

	__sunxi_iommu_prevent_hang_enable_unlocked(enable);

	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
	sunxi_iommu_distribute_mater_put(0xffffffff);
}
EXPORT_SYMBOL(sunxi_iommu_prevent_hang_enable);

static int sunxi_iommu_hw_init(struct iommu_domain *input_domain)
{
	int ret = 0;
	int iommu_enable = 0;
	phys_addr_t dte_addr;
	unsigned long mflag;
	int i;
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(input_domain, struct sunxi_iommu_domain, domain);

	sunxi_iommu_distribute_mater_get(0xffffffff);
	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	dte_addr = __pa(sunxi_domain->pgtable);
	WRITE_EQUAL_REG(iommu, IOMMU_TTB_REG, dte_addr >> 14);

	/*
	 * set preftech functions, including:
	 * master prefetching and only prefetch valid page to TLB/PTW
	 */
	WRITE_EQUAL_REG(iommu, IOMMU_TLB_PREFETCH_REG, plat_data->tlb_prefetch);
	/* new TLB invalid function: use range(start, end) to invalid TLB, started at version V12 */
	if (plat_data->version >= IOMMU_VERSION_V12)
		WRITE_EQUAL_REG(iommu, IOMMU_TLB_IVLD_MODE_SEL_REG,
				plat_data->tlb_invalid_mode);
	/* new PTW invalid function: use range(start, end) to invalid PTW, started at version V14 */
	if (plat_data->version >= IOMMU_VERSION_V14)
		WRITE_EQUAL_REG(iommu, IOMMU_PC_IVLD_MODE_SEL_REG,
				plat_data->ptw_invalid_mode);

	__sunxi_iommu_enable_interrupt_unlocked(1);

	for (i = 0; i < IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT; i++) {
		if ((iommu->bypass >> i) & 0x1)
			__sunxi_enable_device_iommu_unlocked(i, 0);
		else
			__sunxi_enable_device_iommu_unlocked(i, 1);
	}

	ret = sunxi_tlb_flush(iommu);
	if (ret) {
		dev_err(iommu->dev, "Enable flush all request timed out\n");
		goto out;
	}
	WRITE_EQUAL_REG(iommu, IOMMU_AUTO_GATING_REG, IOMMU_AUTO_GATING_DEFAULT_VAL);
	WRITE_EQUAL_REG(iommu, IOMMU_ENABLE_REG, IOMMU_ENABLE);
	iommu_enable = READ_EQUAL_REG(iommu, IOMMU_ENABLE_REG);
	if (iommu_enable != 0x1) {
		iommu_enable = READ_EQUAL_REG(iommu, IOMMU_ENABLE_REG);
		if (iommu_enable != 0x1) {
			dev_err(iommu->dev,
				"iommu enable failed! No iommu in bitfile!\n");
			ret = -ENODEV;
			goto out;
		}
	}
	iommu_hw_init_flag = true;

out:
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
	sunxi_iommu_distribute_mater_put(0xffffffff);

	return ret;
}

static int sunxi_tlb_invalid_locked(dma_addr_t iova, dma_addr_t iova_mask)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	dma_addr_t iova_end = iova_mask;
	int ret = 0;

	/* new TLB invalid function: use range(start, end) to invalid TLB page */
	if (plat_data->version >= IOMMU_VERSION_V12) {
		pr_debug("iommu: TLB invalid:0x%pad-0x%pad\n", &iova,
			 &iova_end);
		WRITE_EQUAL_REG(iommu, IOMMU_TLB_IVLD_START_ADDR_REG,
				(iova >> IOMMU_TLB_IVLD_ADDR_ALIGN_SHIFT)
					<< IOMMU_TLB_IVLD_ADDR_OFFSET);
		WRITE_EQUAL_REG(iommu, IOMMU_TLB_IVLD_END_ADDR_REG,
				(iova_end >> IOMMU_TLB_IVLD_ADDR_ALIGN_SHIFT)
					<< IOMMU_TLB_IVLD_ADDR_OFFSET);
	} else {
		/* old TLB invalid function: only invalid 4K at one time */
		WRITE_EQUAL_REG(iommu, IOMMU_TLB_IVLD_ADDR_REG, iova);
		WRITE_EQUAL_REG(iommu, IOMMU_TLB_IVLD_ADDR_MASK_REG, iova_mask);
	}
	WRITE_EQUAL_REG(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x1);

	udelay(10); //wait for hardware done
	WRITE_EQUAL_REG(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x0);

	return ret;
}

static int sunxi_tlb_invalid(dma_addr_t iova, dma_addr_t iova_mask)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	int ret = 0;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	ret = sunxi_tlb_invalid_locked(iova, iova_mask);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return ret;
}

static int sunxi_ptw_cache_invalid_locked(dma_addr_t iova_start,
					  dma_addr_t iova_end)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	int ret = 0;

	/* new PTW invalid function: use range(start, end) to invalid PTW page */
	if (plat_data->version >= IOMMU_VERSION_V14) {
		pr_debug("iommu: PTW invalid:0x%pad-0x%pad\n", &iova_start,
			 &iova_end);
		WARN_ON(iova_end == 0);
		WRITE_EQUAL_REG(iommu, IOMMU_PC_IVLD_START_ADDR_REG,
				(iova_start >> IOMMU_PC_IVLD_ADDR_ALIGN_SHIFT)
					<< IOMMU_PC_IVLD_ADDR_OFFSET);
		WRITE_EQUAL_REG(iommu, IOMMU_PC_IVLD_END_ADDR_REG,
				(iova_end >> IOMMU_PC_IVLD_ADDR_ALIGN_SHIFT)
					<< IOMMU_PC_IVLD_ADDR_OFFSET);
	} else {
		/* old ptw invalid function: only invalid 1M at one time */
		pr_debug("iommu: PTW invalid:0x%x\n", (unsigned int)iova_start);
		WRITE_EQUAL_REG(iommu, IOMMU_PC_IVLD_ADDR_REG, iova_start);
	}
	WRITE_EQUAL_REG(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x1);

	udelay(10); //wait for hardware done
	WRITE_EQUAL_REG(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x0);
	return ret;
}

static int sunxi_ptw_cache_invalid(dma_addr_t iova_start, dma_addr_t iova_end)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	int ret = 0;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	sunxi_ptw_cache_invalid_locked(iova_start, iova_end);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);

	return ret;
}

void sunxi_zap_tlb(unsigned long iova, size_t size)
{
	const struct sunxi_iommu_plat_data *plat_data =
		global_iommu_dev->plat_data;

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
		sunxi_tlb_invalid(iova + size - SPAGE_SIZE,
				  iova + size + 8 * SPAGE_SIZE);
		sunxi_ptw_cache_invalid(iova, 0);
		sunxi_ptw_cache_invalid(iova + size, 0);

		sunxi_ptw_cache_invalid(iova + SPD_SIZE, 0);
		sunxi_ptw_cache_invalid(iova + size + SPD_SIZE, 0);
		sunxi_ptw_cache_invalid(iova + size + 2 * SPD_SIZE, 0);
	} else {
		sunxi_tlb_invalid(iova, iova + 2 * SPAGE_SIZE);
		sunxi_tlb_invalid(iova + size - SPAGE_SIZE,
				  iova + size + 8 * SPAGE_SIZE);
		sunxi_ptw_cache_invalid(iova, iova + SPD_SIZE);
		sunxi_ptw_cache_invalid(iova + size - SPD_SIZE, iova + size);
	}

	return;
}

static int sunxi_iommu_map(struct iommu_domain *domain, unsigned long iova,
			   phys_addr_t paddr, size_t size, int prot, gfp_t gfp)
{
	struct sunxi_iommu_domain *sunxi_domain;
	size_t iova_start, iova_end, s_iova_start;
	int ret;
	unsigned long flags;

	sunxi_domain = container_of(domain, struct sunxi_iommu_domain, domain);
	WARN_ON(sunxi_domain->pgtable == NULL);
	iova_start = iova & IOMMU_PT_MASK;
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

	for (; iova_start < iova_end;) {
		iova_tail_size = sunxi_pgtable_delete_l2_tables(
			 sunxi_domain->pgtable, iova_start, iova_end);

		if (plat_data->version < IOMMU_VERSION_V14)
			sunxi_ptw_cache_invalid(iova_start, 0);
		iova_start += iova_tail_size;
	}
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return size;
}

void sunxi_iommu_iotlb_sync_map(struct iommu_domain *domain, unsigned long iova,
				size_t size)
{
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(domain, struct sunxi_iommu_domain, domain);
	unsigned long flags;

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	sunxi_zap_tlb(iova, size);
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);

	return;
}

void sunxi_iommu_iotlb_sync(struct iommu_domain *domain,
			    struct iommu_iotlb_gather *iotlb_gather)
{
	struct sunxi_iommu_domain *sunxi_domain =
		container_of(domain, struct sunxi_iommu_domain, domain);
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	unsigned long flags;

	if (plat_data->version >= IOMMU_VERSION_V14)
		return;

	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	sunxi_zap_tlb(iotlb_gather->start,
		      iotlb_gather->end - iotlb_gather->start);
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
	if (global_iommu_domain)
		return global_iommu_domain;

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
	sunxi_domain->domain.geometry.aperture_end = (1ULL << 34) - 1;
	sunxi_domain->domain.geometry.force_aperture = true;
	spin_lock_init(&sunxi_domain->dt_lock);
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

	sunxi_iommu_distribute_mater_get(0xffffffff);
	spin_lock_irqsave(&sunxi_domain->dt_lock, flags);
	sunxi_pgtable_clear(sunxi_domain->pgtable);
	sunxi_tlb_flush(global_iommu_dev);
	spin_unlock_irqrestore(&sunxi_domain->dt_lock, flags);
	sunxi_iommu_distribute_mater_put(0xffffffff);
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
int sunxi_iommu_set_dma_parms(struct notifier_block *nb, unsigned long action,
			      void *data)
{
	struct device *dev = data;

#ifndef GROUP_NOTIFIER_DEPRECATED
	if (action != IOMMU_GROUP_NOTIFY_BIND_DRIVER)
		return 0;
#endif

	dev->dma_parms = devm_kzalloc(dev, sizeof(*dev->dma_parms), GFP_KERNEL);
	if (!dev->dma_parms)
		return -ENOMEM;
	dma_set_max_seg_size(dev, DMA_BIT_MASK(32));

	return 0;
}

struct iommu_group *sunxi_iommu_device_group(struct device *dev)
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

	WRITE_EQUAL_REG(iommu, IOMMU_VA_CONFIG_REG, 0x80000000);
}
EXPORT_SYMBOL(sunxi_set_debug_mode);

void sunxi_set_prefetch_mode(void)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;

	WRITE_EQUAL_REG(iommu, IOMMU_VA_CONFIG_REG, 0x00000000);
}
EXPORT_SYMBOL(sunxi_set_prefetch_mode);

int sunxi_iova_test_write(dma_addr_t iova, u32 val)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	int retval;

	WRITE_EQUAL_REG(iommu, IOMMU_LOW_VA_REG, iova & 0xFFFFFFFF);
	WRITE_EQUAL_REG(iommu, IOMMU_HIGH_VA_REG, (iova >> 32) & 0xFFFFFFFF);
	WRITE_EQUAL_REG(iommu, IOMMU_VA_DATA_REG, val);
	WRITE_EQUAL_REG(iommu, IOMMU_VA_CONFIG_REG, 0x80000100);
	WRITE_EQUAL_REG(iommu, IOMMU_VA_CONFIG_REG, 0x80000101);
	retval = sunxi_wait_when(
		((sunxi_iommu_read(iommu, IOMMU_VA_CONFIG_REG) & 0x1) ||
		 (sunxi_iommu_read(iommu, IOMMU_PER_SET_ADDR_SIZE +
						  IOMMU_VA_CONFIG_REG) &
		  0x1)),
		1);
	if (retval)
		dev_err(iommu->dev, "write VA address request timed out\n");
	return retval;
}
EXPORT_SYMBOL(sunxi_iova_test_write);

unsigned long sunxi_iova_test_read(dma_addr_t iova)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	unsigned long retval;

	WRITE_EQUAL_REG(iommu, IOMMU_LOW_VA_REG, iova & 0xFFFFFFFF);
	WRITE_EQUAL_REG(iommu, IOMMU_HIGH_VA_REG, (iova >> 32) & 0xFFFFFFFF);
	WRITE_EQUAL_REG(iommu, IOMMU_VA_CONFIG_REG, 0x80000000);
	WRITE_EQUAL_REG(iommu, IOMMU_VA_CONFIG_REG, 0x80000001);
	retval = sunxi_wait_when(
		((sunxi_iommu_read(iommu, IOMMU_VA_CONFIG_REG) & 0x1) ||
		 (sunxi_iommu_read(iommu, IOMMU_PER_SET_ADDR_SIZE +
						  IOMMU_VA_CONFIG_REG) &
		  0x1)),
		1);
	if (retval) {
		dev_err(iommu->dev, "read VA address request timed out\n");
		retval = 0;
		goto out;
	}
	retval = READ_EQUAL_REG(iommu, IOMMU_VA_DATA_REG);

out:
	return retval;
}
EXPORT_SYMBOL(sunxi_iova_test_read);

int sunxi_iommu_get_idx_by_name(const char *name)
{
	int i;
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	for (i = 0; i < IOMMU_HW_SET_COUNT * IOMMU_MIC_MAX_MASTER; i++) {
		if (strlen(name) == strlen(plat_data->master[i]) &&
		    strcasecmp(name, plat_data->master[i])) {
			return i;
		}
	}
	WARN(1, "%s not a valid iommu master", name);
	return -1;
}
EXPORT_SYMBOL(sunxi_iommu_get_idx_by_name);

static int sunxi_iova_invalid_helper(unsigned long iova)
{
	struct sunxi_iommu_domain *sunxi_domain = container_of(
		global_iommu_domain, struct sunxi_iommu_domain, domain);
	return sunxi_pgtable_invalid_helper(sunxi_domain->pgtable, iova);
}

static void __dump_int_from_one_instance(struct sunxi_iommu_dev *iommu,
					 int index)
{
	u32 inter_status_reg = 0;
	dma_addr_t addr_reg = 0;
	u32 int_masterid_bitmap = 0;
	u32 data_reg = 0;
	u32 l1_pgint_reg = 0;
	u32 l2_pgint_reg = 0;
	u32 master_id = 0;
	unsigned long mflag;
	const struct sunxi_iommu_plat_data *plat_data = iommu->plat_data;
	u32 offset = index * IOMMU_PER_SET_ADDR_SIZE;
	u32 int_clear_mask;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	inter_status_reg = sunxi_iommu_read(iommu, offset + IOMMU_INT_STA_REG) &
			   0x3ffff;
	l1_pgint_reg = sunxi_iommu_read(iommu, offset + IOMMU_L1PG_INT_REG);
	l2_pgint_reg = sunxi_iommu_read(iommu, offset + IOMMU_L2PG_INT_REG);
	int_masterid_bitmap = inter_status_reg | l1_pgint_reg | l2_pgint_reg;

	if (inter_status_reg & (1 << 0)) {
		pr_err("%s Invalid Authority\n",
		       plat_data->master[index * IOMMU_MIC_MAX_MASTER + 0]);
		addr_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_LOW_ADDR_REG(0));
		addr_reg |= (u64)sunxi_iommu_read(
				    iommu,
				    offset + IOMMU_MIC_INT_ERR_HIGH_ADDR_REG(0))
			    << 32;
		data_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_DATA_REG(0));
	} else if (inter_status_reg & (1 << 1)) {
		pr_err("%s Invalid Authority\n",
		       plat_data->master[index * IOMMU_MIC_MAX_MASTER + 1]);
		addr_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_LOW_ADDR_REG(1));
		addr_reg |= (u64)sunxi_iommu_read(
				    iommu,
				    offset + IOMMU_MIC_INT_ERR_HIGH_ADDR_REG(1))
			    << 32;
		data_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_DATA_REG(1));
	} else if (inter_status_reg & (1 << 2)) {
		pr_err("%s Invalid Authority\n",
		       plat_data->master[index * IOMMU_MIC_MAX_MASTER + 2]);
		addr_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_LOW_ADDR_REG(2));
		addr_reg |= (u64)sunxi_iommu_read(
				    iommu,
				    offset + IOMMU_MIC_INT_ERR_HIGH_ADDR_REG(2))
			    << 32;
		data_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_DATA_REG(2));
	} else if (inter_status_reg & (1 << 3)) {
		pr_err("%s Invalid Authority\n",
		       plat_data->master[index * IOMMU_MIC_MAX_MASTER + 3]);
		addr_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_LOW_ADDR_REG(3));
		addr_reg |= (u64)sunxi_iommu_read(
				    iommu,
				    offset + IOMMU_MIC_INT_ERR_HIGH_ADDR_REG(3))
			    << 32;
		data_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_DATA_REG(3));
	} else if (inter_status_reg & (1 << 4)) {
		pr_err("%s Invalid Authority\n",
		       plat_data->master[index * IOMMU_MIC_MAX_MASTER + 4]);
		addr_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_LOW_ADDR_REG(4));
		addr_reg |= (u64)sunxi_iommu_read(
				    iommu,
				    offset + IOMMU_MIC_INT_ERR_HIGH_ADDR_REG(4))
			    << 32;
		data_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_DATA_REG(4));
	} else if (inter_status_reg & (1 << 5)) {
		pr_err("%s Invalid Authority\n",
		       plat_data->master[index * IOMMU_MIC_MAX_MASTER + 5]);
		addr_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_LOW_ADDR_REG(5));
		addr_reg |= (u64)sunxi_iommu_read(
				    iommu,
				    offset + IOMMU_MIC_INT_ERR_HIGH_ADDR_REG(5))
			    << 32;
		data_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_DATA_REG(5));
	} else if (inter_status_reg & (1 << 6)) {
		pr_err("%s Invalid Authority\n",
		       plat_data->master[index * IOMMU_MIC_MAX_MASTER + 6]);
		addr_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_LOW_ADDR_REG(6));
		addr_reg |= (u64)sunxi_iommu_read(
				    iommu,
				    offset + IOMMU_MIC_INT_ERR_HIGH_ADDR_REG(6))
			    << 32;
		data_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_MIC_INT_ERR_DATA_REG(6));
	} else if (inter_status_reg & IOMMU_INT_L1PG_STA_MASK) {
		/*It's OK to prefetch an invalid page, no need to print msg for debug.*/
		if (!(int_masterid_bitmap & (1U << 31)))
			pr_err("L1 PageTable Invalid\n");
		addr_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_INT_ERR_LOW_ADDR7_REG);
		addr_reg |=
			(u64)sunxi_iommu_read(
				iommu, offset + IOMMU_INT_ERR_HIGH_ADDR7_REG)
			<< 32;
		data_reg = sunxi_iommu_read(iommu,
					    offset + IOMMU_INT_ERR_DATA7_REG);
	} else if (inter_status_reg & IOMMU_INT_L2PG_STA_MASK) {
		if (!(int_masterid_bitmap & (1U << 31)))
			pr_err("L2 PageTable Invalid\n");
		addr_reg = sunxi_iommu_read(
			iommu, offset + IOMMU_INT_ERR_LOW_ADDR8_REG);
		addr_reg |=
			(u64)sunxi_iommu_read(
				iommu, offset + IOMMU_INT_ERR_HIGH_ADDR8_REG)
			<< 32;
		data_reg = sunxi_iommu_read(iommu,
					    offset + IOMMU_INT_ERR_DATA8_REG);
	} else
		pr_err("sunxi iommu int error!!!\n");

	if (!(int_masterid_bitmap & (1U << 31))) {
		sunxi_iova_invalid_helper(addr_reg);
		int_masterid_bitmap &= 0xffff;
		master_id = __ffs(int_masterid_bitmap);
		pr_err("Bug is in %s module, invalid address: 0x%llx, data:0x%x, id:0x%x\n",
		       plat_data->master[index * IOMMU_MIC_MAX_MASTER +
					 master_id],
		       addr_reg, data_reg, int_masterid_bitmap);

#if IS_ENABLED(DEBUG)
		__dump_master_reg(index * IOMMU_MIC_MAX_MASTER + master_id);
		__dump_reg(index);
#endif
#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
		iova_show_on_irq();
#endif
		/* master debug callback */
		if (sunxi_iommu_fault_notify_cbs[index * IOMMU_MIC_MAX_MASTER +
						 master_id])
			sunxi_iommu_fault_notify_cbs
				[index * IOMMU_MIC_MAX_MASTER + master_id]();
	}

	/* invalid TLB */
	sunxi_tlb_invalid_locked(addr_reg, addr_reg + 4 * SPAGE_SIZE);

	/* invalid PTW */
	sunxi_ptw_cache_invalid_locked(addr_reg, addr_reg + 2 * SPD_SIZE);

	int_clear_mask = l1_pgint_reg ? IOMMU_INT_L1PG_CLR_EN_MASK : 0;
	int_clear_mask |= l2_pgint_reg ? IOMMU_INT_L2PG_CLR_EN_MASK : 0;
	sunxi_iommu_write(iommu, offset + IOMMU_INT_CLR_REG, int_clear_mask);
	if (!(int_masterid_bitmap & (1U << 31)))
		sunxi_iommu_write(iommu, offset + IOMMU_MIC_INT_CLR_REG(master_id), 1);
	inter_status_reg |= (l1_pgint_reg | l2_pgint_reg);
	inter_status_reg &= 0xffff;
	sunxi_iommu_write(iommu, offset + IOMMU_RESET_REG, ~inter_status_reg);
	sunxi_iommu_write(iommu, offset + IOMMU_RESET_REG, 0xffffffff);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
}

static irqreturn_t sunxi_iommu_irq(int irq, void *dev_id)
{
	struct sunxi_iommu_dev *iommu = dev_id;

	__dump_int_from_one_instance(iommu, irq == iommu->irq[1]);

	return IRQ_HANDLED;
}

static ssize_t sunxi_iommu_enable_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	u32 data;

	spin_lock(&iommu->iommu_lock);
	data = READ_EQUAL_REG(iommu, IOMMU_PMU_ENABLE_REG);
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
	int i;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val) {
		spin_lock(&iommu->iommu_lock);
		data = READ_EQUAL_REG(iommu, IOMMU_PMU_ENABLE_REG);
		WRITE_EQUAL_REG(iommu, IOMMU_PMU_ENABLE_REG, data | 0x1);
		data = READ_EQUAL_REG(iommu, IOMMU_PMU_CLR_REG);
		WRITE_EQUAL_REG(iommu, IOMMU_PMU_CLR_REG, data | 0x1);
		retval = sunxi_wait_when(
			((sunxi_iommu_read(iommu, IOMMU_PMU_CLR_REG) & 0x1) ||
			 (sunxi_iommu_read(iommu, IOMMU_PER_SET_ADDR_SIZE +
							IOMMU_PMU_CLR_REG) &
			  0x1)),
			1);
		if (retval)
			dev_err(iommu->dev, "Clear PMU Count timed out\n");
		spin_unlock(&iommu->iommu_lock);
	} else {
		spin_lock(&iommu->iommu_lock);
		data = READ_EQUAL_REG(iommu, IOMMU_PMU_CLR_REG);
		WRITE_EQUAL_REG(iommu, IOMMU_PMU_CLR_REG, data | 0x1);
		retval = sunxi_wait_when(
			((sunxi_iommu_read(iommu, IOMMU_PMU_CLR_REG) & 0x1) ||
			 (sunxi_iommu_read(iommu, IOMMU_PER_SET_ADDR_SIZE +
							IOMMU_PMU_CLR_REG) &
			  0x1)),
			1);
		if (retval)
			dev_err(iommu->dev, "Clear PMU Count timed out\n");
		data = READ_EQUAL_REG(iommu, IOMMU_PMU_ENABLE_REG);
		WRITE_EQUAL_REG(iommu, IOMMU_PMU_ENABLE_REG, data & ~0x1);
		spin_unlock(&iommu->iommu_lock);
	}

	sunxi_iommu_distribute_mater_get(0xffffffff);
	for (i = 0; i < IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT; i++) {
		int masterid = i;
		unsigned int instance_offset =
			(masterid / IOMMU_MIC_MAX_MASTER) *
			IOMMU_PER_SET_ADDR_SIZE;
		masterid = masterid % IOMMU_MIC_MAX_MASTER;
		sunxi_iommu_write(iommu,
				  instance_offset +
					  IOMMU_MIC_PMU_ENABLE_REG(masterid),
				  !!val);
		sunxi_iommu_write(iommu,
				  instance_offset +
					  IOMMU_MIC_PMU_CLR_REG(masterid),
				  1);
	}
	for (i = 0; i < IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT; i++) {
		int masterid = i;
		unsigned int instance_offset =
			(masterid / IOMMU_MIC_MAX_MASTER) *
			IOMMU_PER_SET_ADDR_SIZE;
		masterid = masterid % IOMMU_MIC_MAX_MASTER;
		sunxi_iommu_write(iommu,
				  instance_offset +
					  IOMMU_MIC_PMU_CLR_REG(masterid),
				  0);
	}
	sunxi_iommu_distribute_mater_put(0xffffffff);

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
		} micro_tlb[IOMMU_MIC_MAX_MASTER];
	} *iommu_profile;

	int i, j;
	int out_len = 0;

	iommu_profile = kmalloc(sizeof(*iommu_profile) * IOMMU_HW_SET_COUNT,
				GFP_KERNEL);
	if (!iommu_profile)
		goto err;

	sunxi_iommu_distribute_mater_get(0xffffffff);
	spin_lock(&iommu->iommu_lock);

	for (i = 0; i < IOMMU_HW_SET_COUNT; i++) {
		j = 0;
		iommu_profile[i].macrotlb_access_count =
			((u64)(sunxi_iommu_read(
				       iommu,
				       i * IOMMU_PER_SET_ADDR_SIZE +
					       IOMMU_PMU_ACCESS_HIGH7_REG) &
			       0x7ff)
			 << 32) |
			sunxi_iommu_read(iommu,
					 i * IOMMU_PER_SET_ADDR_SIZE +
						 IOMMU_PMU_ACCESS_LOW7_REG);
		iommu_profile[i].macrotlb_hit_count =
			((u64)(sunxi_iommu_read(
				       iommu, i * IOMMU_PER_SET_ADDR_SIZE +
						      IOMMU_PMU_HIT_HIGH7_REG) &
			       0x7ff)
			 << 32) |
			sunxi_iommu_read(iommu, i * IOMMU_PER_SET_ADDR_SIZE +
							IOMMU_PMU_HIT_LOW7_REG);

		iommu_profile[i].ptwcache_access_count =
			((u64)(sunxi_iommu_read(
				       iommu,
				       i * IOMMU_PER_SET_ADDR_SIZE +
					       IOMMU_PMU_ACCESS_HIGH8_REG) &
			       0x7ff)
			 << 32) |
			sunxi_iommu_read(iommu,
					 i * IOMMU_PER_SET_ADDR_SIZE +
						 IOMMU_PMU_ACCESS_LOW8_REG);
		iommu_profile[i].ptwcache_hit_count =
			((u64)(sunxi_iommu_read(
				       iommu, i * IOMMU_PER_SET_ADDR_SIZE +
						      IOMMU_PMU_HIT_HIGH8_REG) &
			       0x7ff)
			 << 32) |
			sunxi_iommu_read(iommu, i * IOMMU_PER_SET_ADDR_SIZE +
							IOMMU_PMU_HIT_LOW8_REG);
		for (; j < IOMMU_MIC_MAX_MASTER; j++) {
			iommu_profile[i].micro_tlb[j].access_count =
				((u64)(sunxi_iommu_read(
					       iommu,
					       i * IOMMU_PER_SET_ADDR_SIZE +
						       IOMMU_MIC_PMU_ACCESS_HIGH_REG(
							       0)) &
				       0x7ff)
				 << 32) |
				sunxi_iommu_read(
					iommu,
					i * IOMMU_PER_SET_ADDR_SIZE +
						IOMMU_MIC_PMU_ACCESS_LOW_REG(
							0));
			iommu_profile[i].micro_tlb[j].hit_count =
				((u64)(sunxi_iommu_read(
					       iommu,
					       i * IOMMU_PER_SET_ADDR_SIZE +
						       IOMMU_MIC_PMU_HIT_HIGH_REG(
							       0)) &
				       0x7ff)
				 << 32) |
				sunxi_iommu_read(
					iommu,
					i * IOMMU_PER_SET_ADDR_SIZE +
						IOMMU_MIC_PMU_HIT_LOW_REG(0));
		}
	}

	spin_unlock(&iommu->iommu_lock);
	sunxi_iommu_distribute_mater_put(0xffffffff);
	out_len = 0;
	for (i = 0; i < IOMMU_HW_SET_COUNT; i++) {
		j = 0;
		out_len += sysfs_emit_at(
			buf, out_len,
			"iommu%d macrotlb_access_count = 0x%llx\n", i,
			iommu_profile[i].macrotlb_access_count);
		out_len +=
			sysfs_emit_at(buf, out_len,
				      "iommu%d macrotlb_hit_count = 0x%llx\n",
				      i, iommu_profile[i].macrotlb_hit_count);
		out_len += sysfs_emit_at(
			buf, out_len,
			"iommu%d ptwcache_access_count = 0x%llx\n", i,
			iommu_profile[i].ptwcache_access_count);
		out_len +=
			sysfs_emit_at(buf, out_len,
				      "iommu%d ptwcache_hit_count = 0x%llx\n",
				      i, iommu_profile[i].ptwcache_hit_count);
		for (; j < IOMMU_MIC_MAX_MASTER; j++) {
			out_len += sysfs_emit_at(
				buf, out_len,
				"%s_access_count = 0x%llx\n",
				plat_data->master[i * IOMMU_MIC_MAX_MASTER + j],
				iommu_profile[i].micro_tlb[j].access_count);
			out_len += sysfs_emit_at(
				buf, out_len, "%s_hit_count = 0x%llx\n",
				plat_data->master[i * IOMMU_MIC_MAX_MASTER + j],
				iommu_profile[i].micro_tlb[j].hit_count);
			out_len += sysfs_emit_at(
				buf, out_len,
				"%s_total_latency = 0x%llx\n",
				plat_data->master[i * IOMMU_MIC_MAX_MASTER + j],
				iommu_profile[i].micro_tlb[j].latency);
			out_len += sysfs_emit_at(
				buf, out_len, "%s_max_latency = 0x%x\n",
				plat_data->master[i * IOMMU_MIC_MAX_MASTER + j],
				iommu_profile[i].micro_tlb[j].max_latency);
		}
	}

err:
	if (iommu_profile)
		kfree(iommu_profile);

	return out_len;
}

ssize_t sunxi_iommu_dump_pgtable(char *buf, size_t buf_len, bool for_sysfs_show)
{
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
	__ATTR(enable, 0644, sunxi_iommu_enable_show, sunxi_iommu_enable_store);
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
	sunxi_iommu_init_debugfs(sunxi_iommu);
#endif
}

static void sunxi_iommu_sysfs_remove(struct platform_device *_pdev)
{
	device_remove_file(&_pdev->dev, &sunxi_iommu_enable_attr);
	device_remove_file(&_pdev->dev, &sunxi_iommu_profilling_attr);
	device_remove_file(&_pdev->dev, &sunxi_iommu_map_attr);
#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
	sunxi_iommu_release_debugfs();
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
	.attach_dev = sunxi_iommu_attach_dev,
#ifndef DETACH_OP_DEPRECATED
	.detach_dev	= sunxi_iommu_detach_dev,
#endif
	.map = sunxi_iommu_map,
	.unmap = sunxi_iommu_unmap,
	.iotlb_sync_map = sunxi_iommu_iotlb_sync_map,
	.iova_to_phys = sunxi_iommu_iova_to_phys,
	.iotlb_sync = sunxi_iommu_iotlb_sync,
	.free = sunxi_iommu_domain_free,
};
static const struct iommu_ops sunxi_iommu_ops = {
	.pgsize_bitmap = SZ_4K | SZ_16K | SZ_64K | SZ_256K | SZ_1M | SZ_4M |
			 SZ_16M,
	.domain_alloc = sunxi_iommu_domain_alloc,
	.probe_device = sunxi_iommu_probe_device,
	.probe_finalize = sunxi_iommu_probe_device_finalize,
	.release_device = sunxi_iommu_release_device,
	.device_group = sunxi_iommu_device_group,
	.of_xlate = sunxi_iommu_of_xlate,
	.default_domain_ops = &sunxi_iommu_domain_ops,
	.owner = THIS_MODULE,
	.get_resv_regions = sunxi_iommu_get_resv_regions,
};

#else
static const struct iommu_ops sunxi_iommu_ops = {
	.pgsize_bitmap = SZ_4K | SZ_16K | SZ_64K | SZ_256K | SZ_1M | SZ_4M |
			 SZ_16M,
	.map = sunxi_iommu_map,
	.unmap = sunxi_iommu_unmap,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	.iotlb_sync_map = sunxi_iommu_iotlb_sync_map,
#endif
	.iotlb_sync = sunxi_iommu_iotlb_sync,
	.domain_alloc = sunxi_iommu_domain_alloc,
	.domain_free = sunxi_iommu_domain_free,
	.attach_dev = sunxi_iommu_attach_dev,
	.detach_dev = sunxi_iommu_detach_dev,
	.probe_device = sunxi_iommu_probe_device,
	.probe_finalize = sunxi_iommu_probe_device_finalize,
	.release_device = sunxi_iommu_release_device,
	.device_group = sunxi_iommu_device_group,
	.of_xlate = sunxi_iommu_of_xlate,
	.iova_to_phys = sunxi_iommu_iova_to_phys,
	.owner = THIS_MODULE,
	.get_resv_regions = sunxi_iommu_get_resv_regions,
};
#endif

static int __init_plat_data_from_ofnode(struct sunxi_iommu_plat_data *data,
					struct device_node *node)
{
	int count;
	const char *tmp;
	int i;

	memset(data, 0, sizeof(*data));
	of_property_read_u32(node, "version", &data->version);
	of_property_read_u32(node, "ptw_invalid_mode", &data->ptw_invalid_mode);
	of_property_read_u32(node, "tlb_invalid_mode", &data->tlb_invalid_mode);
	count = of_property_read_string_array(node, "masters", NULL,
					      ARRAY_SIZE(data->master));
	if (count <= 0)
		return -ENODATA;
	for (i = 0; i < IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT && i < count;
	     i++) {
		data->master[i] = kmalloc(16, GFP_KERNEL);
		if (!data->master[i])
			return -ENOMEM;
		of_property_read_string_index(node, "masters", i, &tmp);
		strncpy(data->master[i], tmp, 16);
	}
	return 0;
}

void sunxi_iommu_master_ready(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret;
	struct of_phandle_args args;
	struct sunxi_iommu_dev *iommu = global_iommu_dev;
	int masterid;
	unsigned int instance_offset;

	if (!global_iommu_dev) {
		return;
	}

	ret = of_parse_phandle_with_args(np, "iommus", "#iommu-cells", 0,
					 &args);
	if (ret) {
		return;
	}

	if (args.args_count != 2) {
		return;
	}
	masterid = args.args[0];
	global_iommu_dev->skip_mask &= ~(1 << masterid);

	/* continue config for this master */
	if ((iommu->bypass >> masterid) & 0x1)
		__sunxi_enable_device_iommu_unlocked(masterid, 0);
	else
		__sunxi_enable_device_iommu_unlocked(masterid, 1);


	instance_offset =
		(masterid / IOMMU_MIC_MAX_MASTER) *
		IOMMU_PER_SET_ADDR_SIZE;
	masterid = masterid % IOMMU_MIC_MAX_MASTER;
	/* if common INT enabled, enable master interupt as well */
	sunxi_iommu_write(iommu,
			  instance_offset + IOMMU_MIC_INT_ENABLE_REG(masterid),
			  !!READ_EQUAL_REG(iommu, IOMMU_INT_ENABLE_REG));

	/* if prevent enabled, enable master prevent as well */
	sunxi_iommu_write(iommu,
			  instance_offset + IOMMU_MIC_PVT_HANG_EN_REG(masterid),
			  !!READ_EQUAL_REG(iommu, IOMMU_PVT_HANG_EN));

}
EXPORT_SYMBOL(sunxi_iommu_master_ready);

static int
sunxi_iommu_prepare_masater(struct sunxi_iommu_master_dev *master_dev,
			    struct device_node *child, int id,
			    struct device *dev)
{
	int ret;
	master_dev->dev = device_create(distribute_master_cs, dev,
					MKDEV(0, 0), NULL, "%s",
					child->name);
	if (IS_ERR(master_dev->dev)) {
		return PTR_ERR(master_dev->dev);
	}
	master_dev->dev->of_node = child;
	ret = dev_pm_domain_attach(master_dev->dev, 0);
	if (ret) {
		dev_err(master_dev->dev, "attach fail:%d\n", ret);
		goto err_dev;
	}
	ret = pm_runtime_set_active(master_dev->dev);
	if (ret) {
		dev_err(master_dev->dev, "active fail:%d\n", ret);
		goto err_att;
	}
	pm_runtime_use_autosuspend(master_dev->dev);
	pm_runtime_set_autosuspend_delay(master_dev->dev, 5000);
	pm_runtime_enable(master_dev->dev);
	return ret;
err_att:
	dev_pm_domain_detach(master_dev->dev, 0);
err_dev:
	device_destroy(distribute_master_cs, MKDEV(0, 0));
	return ret;
}

static int
sunxi_iommu_probe_distribute_masters(struct device *dev,
				     struct sunxi_iommu_dev *sunxi_iommu)
{
	struct device_node *np = dev->of_node;
	struct device_node *child = NULL;
	int ret;
	static int32_t pd_configurated_mask = -1;

	sunxi_iommu->bypass = DEFAULT_BYPASS_VALUE;

	sunxi_iommu->master = devm_kcalloc(
		dev, IOMMU_HW_SET_COUNT * IOMMU_MIC_MAX_MASTER,
		sizeof(struct sunxi_iommu_master_dev), GFP_KERNEL | __GFP_ZERO);
	if (!sunxi_iommu->master)
		return -ENOMEM;

	if (!distribute_master_cs) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
		distribute_master_cs = class_create(THIS_MODULE, "iommu_master");
#else
		distribute_master_cs = class_create("iommu_master");
#endif
		if (IS_ERR(distribute_master_cs)) {
			pr_err("device class file already in use\n");
			return -ENOMEM;
		}
	}

	for_each_available_child_of_node(np, child) {
		uint32_t id;
		struct sunxi_iommu_master_dev *master_dev;
		if (!of_property_read_bool(child, "iommu-master"))
			continue;
		if (of_property_read_u32(child, "id", &id))
			BUG();
		if (of_property_read_bool(child, "skip")) {
			sunxi_iommu->skip_mask |= 1 << id;
			continue;
		}

		master_dev = &sunxi_iommu->master[id];
		master_dev->id = id;

		if (of_property_read_bool(child, "power-domains")) {
			struct of_phandle_args pd_args;
			struct device_node *pd_np = NULL;

			ret = of_parse_phandle_with_args(child, "power-domains",
							 "#power-domain-cells",
							 0, &pd_args);
			BUG_ON(ret < 0);

			if (pd_configurated_mask == -1) {
				pd_configurated_mask = 0;
				for_each_available_child_of_node(pd_args.np,
								 pd_np) {
					u32 pd_reg;
					if (!of_property_read_u32(pd_np, "reg",
								  &pd_reg)) {
						pd_configurated_mask |=
							1 << pd_reg;
					}
				}
			}

			if (pd_configurated_mask & (1 << pd_args.args[0])) {
				ret = sunxi_iommu_prepare_masater(
					master_dev, child, id, dev);
				if (ret)
					return ret;
			}
		}
		sunxi_iommu->skip_mask &= ~(1 << id);
	}

	return 0;
}

static int sunxi_iommu_probe(struct platform_device *pdev)
{
	int ret, irq, irq_got;
	struct device *dev = &pdev->dev;
	struct sunxi_iommu_dev *sunxi_iommu;
	struct resource *res;
	struct property *prop;
	struct clk **pclk;
	const char *name;
	int clk_count;
	int i;

	sunxi_iommu = devm_kzalloc(dev, sizeof(*sunxi_iommu), GFP_KERNEL);
	if (!sunxi_iommu)
		return -ENOMEM;

	ret = sunxi_iommu_probe_distribute_masters(dev, sunxi_iommu);
	if (ret) {
		dev_err(dev, "master probe failed with %d\n", ret);
		return ret;
	}

	iopte_cache = sunxi_pgtable_alloc_pte_cache();
	if (!iopte_cache) {
		pr_err("%s: Failed to create sunx-iopte-cache.\n", __func__);
		return -ENOMEM;
	}

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

	for (irq_got = 0; irq_got < IOMMU_HW_SET_COUNT; irq_got++) {
		irq = platform_get_irq(pdev, irq_got);
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

		sunxi_iommu->irq[irq_got] = irq;
	}

	clk_count = of_count_phandle_with_args(dev->of_node, "resets", "#reset-cells");
	if (clk_count > 0) {
		sunxi_iommu->rst = devm_kcalloc(dev, clk_count, sizeof(void *),
						GFP_KERNEL | __GFP_ZERO);
		if (!sunxi_iommu->rst)
			goto err_clk;
		for (i = 0; i < clk_count; i++) {
			sunxi_iommu->rst[i] =
				devm_reset_control_get_by_index(dev, i);
			if (IS_ERR_OR_NULL(sunxi_iommu->rst[i])) {
				dev_err(dev, "unable to get reset[%d]", i);
				goto err_clk;
			}
			if (reset_control_deassert(sunxi_iommu->rst[i])) {
				dev_err(dev, "Couldn't reset control deassert\n");
				goto err_clk;
			}
		}
	}

	clk_count = of_property_count_strings(dev->of_node, "clock-names");
	if (clk_count <= 0) {
		dev_err(dev, "no clocks found\n");
		goto err_clk;
	}
	sunxi_iommu->clk = kzalloc(
		sizeof(void *) * (clk_count + 1 /*sentinel*/), GFP_KERNEL);
	pclk = sunxi_iommu->clk;
	of_property_for_each_string(dev->of_node, "clock-names", prop, name) {
		*pclk = devm_clk_get(dev, name);
		if (IS_ERR(*pclk)) {
			sunxi_iommu->clk = NULL;
			dev_dbg(dev, "Unable to find clock %s\n", name);
			ret = -ENOENT;
			goto err_clk;
		}
		pclk++;
	}
	pclk = sunxi_iommu->clk;
	while (*pclk) {
		clk_prepare_enable(*pclk);
		pclk++;
	}

	platform_set_drvdata(pdev, sunxi_iommu);
	sunxi_iommu->dev = dev;
	spin_lock_init(&sunxi_iommu->iommu_lock);
	global_iommu_dev = sunxi_iommu;
	sunxi_iommu->plat_data =
		kmalloc(sizeof(*sunxi_iommu->plat_data), GFP_KERNEL);

	if (!sunxi_iommu->plat_data) {
		dev_dbg(dev, "no mem for plat data\n");
		ret = -ENOMEM;
		goto err_plat;
	}

	ret = __init_plat_data_from_ofnode(sunxi_iommu->plat_data,
					   dev->of_node);
	if (ret) {
		goto err_plat;
	}

	if (sunxi_iommu->plat_data->version !=
	    sunxi_iommu_read(sunxi_iommu, IOMMU_VERSION_REG)) {
		dev_err(dev,
			"iommu version mismatch, please check and reconfigure\n");
		goto err_plat;
	}

	sunxi_iommu_sysfs_create(pdev, sunxi_iommu);
	ret = iommu_device_sysfs_add(&sunxi_iommu->iommu, dev, NULL,
				     dev_name(dev));
	if (ret) {
		dev_err(dev, "Failed to register iommu in sysfs\n");
		goto err_plat;
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
		goto err_plat;
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
	sunxi_iommu_register_vendorhook();
#endif

	return 0;

err_plat:
	if (sunxi_iommu->plat_data) {
		for (i = 0; i < IOMMU_MIC_MAX_MASTER * IOMMU_HW_SET_COUNT;
		     i++) {
			if (sunxi_iommu->plat_data->master[i])
				kfree(sunxi_iommu->plat_data->master[i]);
		}
		kfree(sunxi_iommu->plat_data);
	}
err_clk:
	for (; irq_got > 0; irq_got--) {
		devm_free_irq(dev, sunxi_iommu->irq[irq_got - 1], sunxi_iommu);
	}
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
	int i;

	sunxi_pgtable_free_pte_cache(iopte_cache);
	if (!list_empty(&sunxi_iommu->rsv_list)) {
		list_for_each_entry_safe (entry, next, &sunxi_iommu->rsv_list,
					  list)
			kfree(entry);
	}
#ifndef BUS_SET_OP_DEPRECATED
	bus_set_iommu(&platform_bus_type, NULL);
#endif
	for (i = 0; i < IOMMU_HW_SET_COUNT; i++) {
		devm_free_irq(sunxi_iommu->dev, sunxi_iommu->irq[i],
			      sunxi_iommu);
	}
	devm_iounmap(sunxi_iommu->dev, sunxi_iommu->base);
	sunxi_iommu_sysfs_remove(pdev);
	iommu_device_sysfs_remove(&sunxi_iommu->iommu);
	iommu_device_unregister(&sunxi_iommu->iommu);
	global_iommu_dev = NULL;

	return 0;
}

static int sunxi_iommu_suspend(struct device *dev)
{
	struct clk **pclk = global_iommu_dev->clk;
	while (*pclk) {
		clk_disable_unprepare(*pclk);
		pclk++;
	}

	return 0;
}

static int sunxi_iommu_resume(struct device *dev)
{
	int err;
	struct clk **pclk = global_iommu_dev->clk;
	while (*pclk) {
		clk_prepare_enable(*pclk);
		pclk++;
	}

	if (unlikely(!global_iommu_domain))
		return 0;

	err = sunxi_iommu_hw_init(global_iommu_domain);

	return err;
}

const struct dev_pm_ops sunxi_iommu_pm_ops = {
	.suspend = sunxi_iommu_suspend,
	.resume = sunxi_iommu_resume,
};

static const struct of_device_id sunxi_iommu_dt_ids[] = {
	{ .compatible = "allwinner,iommu-v20" },
	{ /* sentinel */ },
};

static struct platform_driver
	sunxi_iommu_driver = { .probe = sunxi_iommu_probe,
			       .remove = sunxi_iommu_remove,
			       .driver = {
				       .owner = THIS_MODULE,
				       .name = "sunxi-iommu-v2",
				       .pm = &sunxi_iommu_pm_ops,
				       .of_match_table = sunxi_iommu_dt_ids,
			       } };

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
MODULE_VERSION("1.3.4");
MODULE_AUTHOR("ouayngkun<ouyangkun@allwinnertech.com>");

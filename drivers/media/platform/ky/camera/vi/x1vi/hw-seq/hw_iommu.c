// SPDX-License-Identifier: GPL-2.0
/*
 * isp_iommu.c - Driver for ISP IOMMU
 *
 * Copyright (C) 2023 KY Micro Limited
 */
//#define DEBUG

#ifdef CONFIG_KY_X1_VI_IOMMU
//#include <soc/spm/plat.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/printk.h>
#include "hw_reg.h"
#include "hw_reg_iommu.h"
#include "hw_iommu.h"
#undef CAM_MODULE_TAG
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>

static inline uint32_t iommu_reg_read(struct isp_iommu_device *mmu_dev, uint32_t reg)
{
	return read32(mmu_dev->regs_base + reg);
}

static inline void iommu_reg_write(struct isp_iommu_device *mmu_dev,
				   uint32_t reg, uint32_t val)
{
	write32(mmu_dev->regs_base + reg, val);
}

static inline void iommu_reg_write_mask(struct isp_iommu_device *mmu_dev,
					uint32_t reg, uint32_t val, uint32_t mask)
{
	uint32_t v;

	v = iommu_reg_read(mmu_dev, reg);
	v = (v & ~mask) | (val & mask);
	iommu_reg_write(mmu_dev, reg, v);
}

static inline void iommu_reg_set_bit(struct isp_iommu_device *mmu_dev,
				     uint32_t reg, uint32_t val)
{
	iommu_reg_write_mask(mmu_dev, reg, val, val);
}

static inline void iommu_reg_clr_bit(struct isp_iommu_device *mmu_dev,
				     uint32_t reg, uint32_t val)
{
	iommu_reg_write_mask(mmu_dev, reg, 0, val);
}

static void iommu_enable_tbu(struct isp_iommu_device *mmu_dev, int tbu)
{
	iommu_reg_set_bit(mmu_dev, REG_IOMMU_TCR0(tbu), 0x1);
}

static void iommu_disable_tbu(struct isp_iommu_device *mmu_dev, int tbu)
{
	iommu_reg_clr_bit(mmu_dev, REG_IOMMU_TCR0(tbu), 0x1);
}

static void iommu_set_tbu_ttaddr(struct isp_iommu_device *mmu_dev, int tbu,
				 uint64_t addr)
{
	iommu_reg_write(mmu_dev, REG_IOMMU_TTBL(tbu), addr & 0xffffffff);
	iommu_reg_write(mmu_dev, REG_IOMMU_TTBH(tbu), (addr >> 32) & 0x1);
}

static void iommu_set_tbu_ttsize(struct isp_iommu_device *mmu_dev, int tbu, int size)
{
	iommu_reg_write_mask(mmu_dev, REG_IOMMU_TCR0(tbu),
			     ((size - 1) & 0x1fff) << 16, 0x1fff << 16);
}

static void __maybe_unused iommu_set_tbu_qos(struct isp_iommu_device *mmu_dev, int tbu,
					     int qos)
{
	iommu_reg_write_mask(mmu_dev, REG_IOMMU_TCR0(tbu), (qos & 0xf) << 4, 0xf << 4);
}

/**
 * iommu_update_trans_table - TBU translation table update
 *
 *   this bit will be cleared to 0 after TLB preload.
 *   only work for full frame tbu.
 */
void iommu_update_trans_table(struct isp_iommu_device *mmu_dev, int tbu)
{
	iommu_reg_set_bit(mmu_dev, REG_IOMMU_TCR0(tbu), 0x1 << 2);
}

static void iommu_enable_irqs(struct isp_iommu_device *mmu_dev)
{
	iommu_reg_write_mask(mmu_dev, REG_IOMMU_GIRQ_ENA, 0xffffffff, 0xffffffff);
}

static inline uint32_t iommu_bva_low(struct isp_iommu_device *mmu_dev)
{
	return iommu_reg_read(mmu_dev, REG_IOMMU_BVAL);
}

static int tid_to_tbu(struct isp_iommu_device *mmu_dev, uint32_t tid)
{
	int i;

	for (i = 0; i < ISP_IOMMU_CH_NUM; ++i)
		if (mmu_dev->ch_matrix[i] == tid)
			return i;

	return -1;
}

static int isp_iommu_acquire_channel(struct isp_iommu_device *mmu_dev, uint32_t tid)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_debug("no such channel %x to acquire\n", tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	if (test_bit(tbu, &mmu_dev->ch_map)) {
		spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);
		pr_err("channel %x not free\n", tid);
		return -EBUSY;
	}
	set_bit(tbu, &mmu_dev->ch_map);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

static int isp_iommu_release_channel(struct isp_iommu_device *mmu_dev, uint32_t tid)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("no such channel %x to release\n", tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	clear_bit(tbu, &mmu_dev->ch_map);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

static int isp_iommu_enable_channel(struct isp_iommu_device *mmu_dev, uint32_t tid)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("no such channel %x to enable\n", tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	//if (!test_bit(tbu, &mmu_dev->ch_map)) {
	//      spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);
	//      return -EPERM;
	//}

	iommu_enable_tbu(mmu_dev, tbu);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

static int isp_iommu_disable_channel(struct isp_iommu_device *mmu_dev, uint32_t tid)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("no such channel %x to disable\n", tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	//if (!test_bit(tbu, &mmu_dev->ch_map)) {
	//      spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);
	//      return -EPERM;
	//}

	iommu_disable_tbu(mmu_dev, tbu);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

static int isp_iommu_config_channel(struct isp_iommu_device *mmu_dev,
				    uint32_t tid, uint64_t ttAddr, uint32_t ttSize)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("no such channel %x to configure\n", tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	//if (!test_bit(tbu, &mmu_dev->ch_map)) {
	//	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);
	//	return -EPERM;
	//}

	//iommu_set_tbu_qos(mmu_dev, tbu, 4);
	iommu_set_tbu_ttaddr(mmu_dev, tbu, ttAddr);
	iommu_set_tbu_ttsize(mmu_dev, tbu, ttSize);
	// iommu_update_trans_table(mmu_dev, tbu);
	iommu_enable_irqs(mmu_dev);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

static const uint64_t IOMMU_VADDR_BASE = 0x80000000;
static uint64_t isp_iommu_get_sva(struct isp_iommu_device *mmu_dev,
				  uint32_t tid, uint32_t offset)
{
	int tbu;
	uint64_t svAddr;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("no such channel %x to get sva\n", tid);
		return -ENODEV;
	}

	svAddr = iommu_bva_low(mmu_dev) + 0x2000000 * (uint64_t) tbu + (offset & 0xfff);
	return svAddr;
}

static unsigned int isp_iommu_irq_status(struct isp_iommu_device *mmu_dev)
{
	unsigned int status = 0;
	status = iommu_reg_read(mmu_dev, REG_IOMMU_GIRQ_STAT);
	if (status)
		iommu_reg_write(mmu_dev, REG_IOMMU_GIRQ_STAT, status);
	return status;
}

static int isp_iommu_dump_regs(struct isp_iommu_device *mmu_dev, uint32_t ch_id)
{
	int ret = 0;
	unsigned int status = 0, tlb_size = 0;
	uint64_t addr1 = 0, addr2 = 0, addr3 = 0;

	pr_info("**************start dump isp iommu ch%d regs:\n", ch_id);
	addr1 = iommu_reg_read(mmu_dev, REG_IOMMU_LVAL);
	status = iommu_reg_read(mmu_dev, REG_IOMMU_LVAH);
	if (status)
		addr1 = addr1 | (1ULL << 32);

	addr2 = iommu_reg_read(mmu_dev, REG_IOMMU_LPAL);
	status = iommu_reg_read(mmu_dev, REG_IOMMU_LPAH);
	if (status)
		addr2 |= (1ULL << 32);

	addr3 = iommu_reg_read(mmu_dev, REG_IOMMU_TVAL);
	status = iommu_reg_read(mmu_dev, REG_IOMMU_TVAH);
	if (status)
		addr3 = addr3 | (1ULL << 32);
	pr_info("isp mmu: last virtual addr=0x%llx,last phy addr=0x%llx, timeout addr=0x%llx\n", addr1, addr2, addr3);

	addr1 = iommu_reg_read(mmu_dev, REG_IOMMU_TTBL(ch_id));
	status = iommu_reg_read(mmu_dev, REG_IOMMU_TTBH(ch_id));
	if (status)
		addr1 |= (1ULL << 32);
	status = iommu_reg_read(mmu_dev, REG_IOMMU_TCR0(ch_id));
	tlb_size = (status & 0x1fff0000) >> 16;
	pr_info("isp mmu ch%d: tlb addr=0x%llx,tcr0=0x%x, tlb size=%d\n", ch_id, addr1, status, tlb_size);

	return ret;
}

static void isp_iommu_set_timeout_default_addr(struct isp_iommu_device *mmu_dev,
					       uint64_t timeout_default_addr)
{
	unsigned int high = 0, low = 0;

	low = timeout_default_addr & 0xffffffffULL;
	high = (timeout_default_addr >> 32) & 0xffffffffULL;
	iommu_reg_write(mmu_dev, REG_IOMMU_TIMEOUT_ADDR_LOW, low);
	iommu_reg_write(mmu_dev, REG_IOMMU_TIMEOUT_ADDR_HIGH, high);
}

static struct isp_iommu_ops mmu_ops = {
	.acquire_channel = isp_iommu_acquire_channel,
	.release_channel = isp_iommu_release_channel,
	.enable_channel = isp_iommu_enable_channel,
	.disable_channel = isp_iommu_disable_channel,
	.config_channel = isp_iommu_config_channel,
	.get_sva = isp_iommu_get_sva,
	.irq_status = isp_iommu_irq_status,
	.dump_channel_regs = isp_iommu_dump_regs,
	.set_timeout_default_addr = isp_iommu_set_timeout_default_addr,
};

static const uint32_t iommu_ch_dmac_mapping[ISP_IOMMU_CH_NUM] = {
	MMU_TID(1, 0, 0), 	// fmt0_y 		aout0 	TBU0
	MMU_TID(1, 0, 1), 	// fmt0_uv		aout0	TBU1
	MMU_TID(0, 0, 0), 	// 				ain0	TBU2
	MMU_TID(0, 1, 0), 	// 				ain1	TBU3
	MMU_TID(1, 12, 0),	// rawdump0		aout12	TBU4
	MMU_TID(1, 13, 0),	// rawdump1 	aout13	TBU5
	MMU_TID(1, 6, 0),	// dwt0_l1_y	aout6	TBU6
	MMU_TID(1, 6, 1),	// dwt0_l1_uv	aout6	TBU7
	MMU_TID(1, 7, 0),	// dwt0_l2_y	aout7	TBU8
	MMU_TID(1, 7, 1),	// dwt0_l2_uv	aout7	TBU9
	MMU_TID(1, 8, 0),	// dwt0_l3_y	aout8	TBU10
	MMU_TID(1, 8, 1),	// dwt0_l3_uv	aout8	TBU11
	MMU_TID(1, 11, 0),	// dwt0_l4_y	aout11	TBU12
	MMU_TID(1, 11, 1),	// dwt0_l4_uv	aout11	TBU13
	MMU_TID(1, 2, 0),	// dwt1_l1_y	aout2	TBU14
	MMU_TID(1, 2, 1),	// dwt1_l1_uv	aout2	TBU15
	MMU_TID(1, 3, 0),	// dwt1_l2_y	aout3	TBU16
	MMU_TID(1, 3, 1),	// dwt1_l2_uv	aout3	TBU17
	MMU_TID(1, 4, 0),	// dwt1_l3_y	aout4	TBU18
	MMU_TID(1, 4, 1),	// dwt1_l3_uv	aout4	TBU19
	MMU_TID(1, 5, 0),	// dwt1_l4_y	aout5	TBU20
	MMU_TID(1, 5, 1),	// dwt1_l4_uv	aout5	TBU21
	MMU_TID(1, 1, 0),	// fmt1_y		aout1	TBU22
	MMU_TID(1, 1, 1),	// fmt1_uv		aout1	TBU23
};

struct isp_iommu_device *isp_iommu_create(struct device *dev, unsigned long regs_base)
{
	struct isp_iommu_device *mmu_dev = NULL;

	mmu_dev = devm_kzalloc(dev, sizeof(struct isp_iommu_device), GFP_KERNEL);
	if (!mmu_dev)
		return NULL;

	mmu_dev->regs_base = regs_base;
	mmu_dev->ops = &mmu_ops;
	memcpy(mmu_dev->ch_matrix, iommu_ch_dmac_mapping, sizeof(iommu_ch_dmac_mapping));

	spin_lock_init(&mmu_dev->ops_lock);
	mmu_dev->dev = dev;

	pr_debug("%s X\n", __func__);

	return mmu_dev;
}

void isp_iommu_unregister(struct isp_iommu_device *mmu_dev)
{
	struct device *dev = mmu_dev->dev;
	devm_kfree(dev, mmu_dev);

	pr_debug("%s X\n", __func__);
}

#endif

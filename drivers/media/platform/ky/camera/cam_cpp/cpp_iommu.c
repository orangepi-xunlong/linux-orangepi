// SPDX-License-Identifier: GPL-2.0
/*
 * cpp_iommu.c - Driver for CPP IOMMU
 *
 * Copyright (C) 2023 KY Micro Limited
 */
//#define DEBUG

#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/printk.h>
#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include "cam_dbg.h"
#include "x1_cpp.h"
#include "cpp_dmabuf.h"
#include "regs-cpp-iommu.h"
#include "cpp_iommu.h"

#undef CAM_MODULE_TAG
#define CAM_MODULE_TAG CAM_MDL_CPP

static inline uint32_t iommu_reg_read(struct cpp_iommu_device *mmu_dev, uint32_t reg)
{
	return ioread32(mmu_dev->regs_base + reg);
}

static inline void iommu_reg_write(struct cpp_iommu_device *mmu_dev,
				   uint32_t reg, uint32_t val)
{
	iowrite32(val, mmu_dev->regs_base + reg);
}

static inline void iommu_reg_write_mask(struct cpp_iommu_device *mmu_dev,
					uint32_t reg, uint32_t val, uint32_t mask)
{
	uint32_t v;

	v = iommu_reg_read(mmu_dev, reg);
	v = (v & ~mask) | (val & mask);
	iommu_reg_write(mmu_dev, reg, v);
}

static inline void iommu_reg_set_bit(struct cpp_iommu_device *mmu_dev,
				     uint32_t reg, uint32_t val)
{
	iommu_reg_write_mask(mmu_dev, reg, val, val);
}

static inline void iommu_reg_clr_bit(struct cpp_iommu_device *mmu_dev,
				     uint32_t reg, uint32_t val)
{
	iommu_reg_write_mask(mmu_dev, reg, 0, val);
}

static void iommu_enable_tbu(struct cpp_iommu_device *mmu_dev, int tbu)
{
	iommu_reg_set_bit(mmu_dev, REG_IOMMU_TCR0(tbu), 0x1);
}

static void iommu_disable_tbu(struct cpp_iommu_device *mmu_dev, int tbu)
{
	iommu_reg_clr_bit(mmu_dev, REG_IOMMU_TCR0(tbu), 0x1);
}

static void iommu_set_tbu_ttaddr(struct cpp_iommu_device *mmu_dev, int tbu,
				 uint64_t addr)
{
	iommu_reg_write(mmu_dev, REG_IOMMU_TTBL(tbu), addr & 0xffffffff);
	iommu_reg_write(mmu_dev, REG_IOMMU_TTBH(tbu), (addr >> 32) & 0x1);
}

static void iommu_set_tbu_ttsize(struct cpp_iommu_device *mmu_dev, int tbu, int size)
{
	iommu_reg_write_mask(mmu_dev, REG_IOMMU_TCR0(tbu),
			     (size & 0x1fff) << 16, 0x1fff << 16);
}

static void __maybe_unused iommu_set_tbu_qos(struct cpp_iommu_device *mmu_dev,
					     int tbu, int qos)
{
	iommu_reg_write_mask(mmu_dev, REG_IOMMU_TCR0(tbu), (qos & 0xf) << 4, 0xf << 4);
}

/**
 * iommu_update_trans_table - TBU translation table update
 *
 *   this bit will be cleared to 0 after TLB preload.
 *   only work for full frame tbu.
 */
static void iommu_update_trans_table(struct cpp_iommu_device *mmu_dev, int tbu)
{
	iommu_reg_set_bit(mmu_dev, REG_IOMMU_TCR0(tbu), 0x1 << 2);
}

static void iommu_enable_irqs(struct cpp_iommu_device *mmu_dev)
{
	iommu_reg_write_mask(mmu_dev, REG_IOMMU_GIRQ_ENA, 0x1ffff, 0x1ffff);
}

static inline uint32_t iommu_bva_low(struct cpp_iommu_device *mmu_dev)
{
	return iommu_reg_read(mmu_dev, REG_IOMMU_BVAL);
}

/*
 * curY_L0-----TBU0
 * curY_L1-----TBU1
 * curY_L2-----TBU2
 * curY_L3-----TBU3
 * preY_L0-----TBU4
 * preY_L1-----TBU5
 * preY_L2-----TBU6
 * preY_L3-----TBU7
 * wbY_L0------TBU8
 * wbY_L1------TBU9
 * wbY_L2------TBU10
 * wbY_L3------TBU11
 * curUV_L0----TBU12
 * curUV_L1----TBU13
 * curUV_L2----TBU14
 * curUV_L3----TBU15
 * preUV_L0----TBU16
 * preUV_L1----TBU17
 * preUV_L2----TBU18
 * preUV_L3----TBU19
 * wbUV_L0-----TBU20
 * wbUV_L1-----TBU21
 * wbUV_L2-----TBU22
 * wbUV_L3-----TBU23
 * preK_L0-----TBU24
 * preK_L1-----TBU25
 * preK_L2-----TBU26
 * preK_L3-----TBU27
 * wbK_L0------TBU28
 * wbK_L1------TBU29
 * wbK_L2------TBU30
 * wbK_L3------TBU31
 */
#define MMU_TBU(tbu_id) ((tbu_id) << 16)
static const uint32_t iommu_ch_tid_map[] = {
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_Y_L4) | MMU_TBU(0),
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_C_L4) | MMU_TBU(1),
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_Y_L3) | MMU_TBU(2),
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_C_L3) | MMU_TBU(3),
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_Y_L2) | MMU_TBU(4),
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_C_L2) | MMU_TBU(5),
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_Y_L1) | MMU_TBU(6),
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_C_L1) | MMU_TBU(7),
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_Y_L0) | MMU_TBU(8),	/* if fbc enc mode is on, the tbu is invalid */
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_DWT_C_L0) | MMU_TBU(9),	/* if fbc enc mode is on, the tbu works in full frame mode, need to set tbu_update 1 */
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_FBC_HEADER) | MMU_TBU(9),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_Y_L4) | MMU_TBU(10),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_C_L4) | MMU_TBU(11),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_Y_L3) | MMU_TBU(12),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_C_L3) | MMU_TBU(13),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_Y_L2) | MMU_TBU(14),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_C_L2) | MMU_TBU(15),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_Y_L0) | MMU_TBU(16),	/* if fbc dec mode is on, the tbu is invalid */
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_C_L0) | MMU_TBU(17),	/* if fbc dec mode is on, the tbu works in full frame mode, need to set tbu_update 1 */
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_FBC_HEADER) | MMU_TBU(17),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_Y_L1) | MMU_TBU(18),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_DWT_C_L1) | MMU_TBU(19),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_Y_L4) | MMU_TBU(20),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_C_L4) | MMU_TBU(21),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_Y_L3) | MMU_TBU(22),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_C_L3) | MMU_TBU(23),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_Y_L2) | MMU_TBU(24),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_C_L2) | MMU_TBU(25),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_Y_L1) | MMU_TBU(26),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_C_L1) | MMU_TBU(27),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_Y_L0) | MMU_TBU(28),
	MMU_TID(MAC_DMA_PORT_R0, MAC_DMA_CHNL_DWT_C_L0) | MMU_TBU(29),
	MMU_TID(MAC_DMA_PORT_W0, MAC_DMA_CHNL_KGAIN_L0) | MMU_TBU(30),
	MMU_TID(MAC_DMA_PORT_R1, MAC_DMA_CHNL_KGAIN_L0) | MMU_TBU(31),
};

static int tid_to_tbu(struct cpp_iommu_device *mmu_dev, uint32_t tid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(iommu_ch_tid_map); ++i)
		if ((iommu_ch_tid_map[i] & 0xffff) == tid)
			return iommu_ch_tid_map[i] >> 16;

	return -1;
}

/*
 * cpp iommu api
 **/
enum cpp_iommu_buf_state {
	CPP_IOMMU_BUFF_EXIST,
	CPP_IOMMU_BUFF_NOT_EXIST,
};

struct cam_dmabuf_info {
	int fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attach;
	struct sg_table *table;
	enum dma_data_direction dir;
	int ref_count;
	dma_addr_t paddr;
	struct list_head list;
	size_t len;
	size_t phys_len;
};

static enum cpp_iommu_buf_state
cpp_iommu_check_fd_in_list(struct cpp_iommu_device *mmu_dev, int fd,
			   dma_addr_t *paddr_ptr)
{
	struct cam_dmabuf_info *mapping;

	list_for_each_entry(mapping, &mmu_dev->iommu_buf_list, list) {
		if (mapping->fd == fd) {
			*paddr_ptr = mapping->paddr;
			// *len_ptr = mapping->len;
			mapping->ref_count++;
			return CPP_IOMMU_BUFF_EXIST;
		}
	}

	return CPP_IOMMU_BUFF_NOT_EXIST;
}

static int
cpp_iommu_map_buffer_and_add_to_list(struct cpp_iommu_device *mmu_dev, int fd,
				     enum dma_data_direction dma_dir, bool sync,
				     dma_addr_t *paddr_ptr)
{
	struct dma_buf *dbuf;
	struct dma_buf_attachment *dba;
	struct device *dev = &mmu_dev->cpp_dev->pdev->dev;
	struct sg_table *sgt;
	dma_addr_t paddr;
	struct cam_dmabuf_info *mapping_info;
	int rc = 0;

	dbuf = dma_buf_get(fd);
	if (IS_ERR(dbuf)) {
		pr_err("invalid dmabuf fd %d, %ld\n", fd, PTR_ERR(dbuf));
		return PTR_ERR(dbuf);
	}

	dba = dma_buf_attach(dbuf, dev);
	if (IS_ERR(dba)) {
		pr_err("failed to attach dmabuf, %ld\n", PTR_ERR(dba));
		dma_buf_put(dbuf);
		return PTR_ERR(dba);
	}
	// FIXME
	// 'struct dma_buf_attachment' has no member named 'dma_map_attrs'
	//if (!sync)
	//      dba->dma_map_attrs |= DMA_ATTR_SKIP_CPU_SYNC;

	/* get the associated scatterlist for this buffer */
	sgt = dma_buf_map_attachment_unlocked(dba, dma_dir);
	if (IS_ERR(sgt)) {
		pr_err("Error getting dmabuf scatterlist, %ld\n", PTR_ERR(sgt));
		dma_buf_detach(dbuf, dba);
		dma_buf_put(dbuf);
		return PTR_ERR(sgt);
	}
	paddr = sg_dma_address(sgt->sgl);

	/* fill up mapping_info */
	mapping_info = kzalloc(sizeof(*mapping_info), GFP_KERNEL);
	if (!mapping_info) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	mapping_info->fd = fd;
	mapping_info->buf = dbuf;
	mapping_info->attach = dba;
	mapping_info->table = sgt;
	mapping_info->paddr = paddr;
	mapping_info->len = dbuf->size;
	mapping_info->dir = dma_dir;
	mapping_info->ref_count = 1;

	if (!mapping_info->paddr || !mapping_info->len) {
		pr_err("Error dynamic dma mapping\n");
		kfree(mapping_info);
		mapping_info = NULL;
		rc = -ENOSPC;
		goto err_alloc;
	}

	*paddr_ptr = mapping_info->paddr;
	/* add to the list */
	list_add(&mapping_info->list, &mmu_dev->iommu_buf_list);

	pr_debug("fd=%d, dmabuf=%p, paddr=0x%llx, len=%lu\n", fd, dbuf, paddr,
		 dbuf->size);

	return 0;

err_alloc:
	dma_buf_unmap_attachment_unlocked(dba, sgt, dma_dir);
	dma_buf_detach(dbuf, dba);
	dma_buf_put(dbuf);

	return rc;
}

int cpp_iommu_map_dmabuf(struct cpp_iommu_device *mmu_dev, int fd,
			 uint32_t map_flags, dma_addr_t *paddr_ptr)
{
	enum cpp_iommu_buf_state buf_state;
	enum dma_data_direction dma_dir;
	bool sync;
	int rc = 0;

	if (fd < 0) {
		cam_err("%s: invalid fd=%d", __func__, fd);
		return -EINVAL;
	}

	if (map_flags & IOMMU_MAP_FLAG_READ_ONLY)
		dma_dir = DMA_TO_DEVICE;
	else if (map_flags & IOMMU_MAP_FLAG_WRITE_ONLY)
		dma_dir = DMA_FROM_DEVICE;
	else if (map_flags & IOMMU_MAP_FLAG_READ_WRITE)
		dma_dir = DMA_BIDIRECTIONAL;
	else {
		cam_err("%s: map dmabuf without direction", __func__);
		return -EINVAL;
	}
	sync = !(map_flags & IOMMU_MAP_FLAG_NOSYNC);

	mutex_lock(&mmu_dev->list_lock);
	if (mmu_dev->state != CPP_IOMMU_ATTACHED) {
		cam_err("attach mmu before map dma buffer");
		rc = -EINVAL;
		goto map_dmabuf_end;
	}

	buf_state = cpp_iommu_check_fd_in_list(mmu_dev, fd, paddr_ptr);
	if (buf_state == CPP_IOMMU_BUFF_EXIST) {
		pr_debug("fd=%d already in list", fd);
		rc = 0;
		goto map_dmabuf_end;
	}

	rc = cpp_iommu_map_buffer_and_add_to_list(mmu_dev, fd, dma_dir, sync,
						  paddr_ptr);
	if (rc < 0)
		cam_err("mapping or add list fail, fd=%d, rc=%d", fd, rc);

map_dmabuf_end:
	mutex_unlock(&mmu_dev->list_lock);
	return rc;
}

EXPORT_SYMBOL(cpp_iommu_map_dmabuf);

static struct cam_dmabuf_info *cpp_iommu_find_mapping_by_fd(struct cpp_iommu_device
							    *mmu_dev, int fd)
{
	struct cam_dmabuf_info *mapping;

	list_for_each_entry(mapping, &mmu_dev->iommu_buf_list, list) {
		if (mapping->fd == fd)
			return mapping;
	}

	return NULL;
}

static int cpp_iommu_unmap_buffer_and_remove_from_list(struct cam_dmabuf_info
						       *mapping_info)
{
	if (!mapping_info->buf || !mapping_info->table || !mapping_info->attach) {
		cam_err("%s: invalid params fd=%d, buf=%pK, table=%pK, attach=%pk",
			__func__, mapping_info->fd, mapping_info->buf,
			mapping_info->table, mapping_info->attach);
		return -EINVAL;
	}

	dma_buf_unmap_attachment_unlocked(mapping_info->attach, mapping_info->table,
				 mapping_info->dir);
	dma_buf_detach(mapping_info->buf, mapping_info->attach);
	dma_buf_put(mapping_info->buf);

	mapping_info->buf = NULL;

	list_del_init(&mapping_info->list);

	kfree(mapping_info);
	return 0;
}

int cpp_iommu_unmap_dmabuf(struct cpp_iommu_device *mmu_dev, int fd)
{
	struct cam_dmabuf_info *mapping_info;
	int rc = 0;

	if (fd < 0) {
		cam_err("%s: invalid fd=%d", __func__, fd);
		return -EINVAL;
	}

	mutex_lock(&mmu_dev->list_lock);
	if (mmu_dev->state != CPP_IOMMU_ATTACHED) {
		cam_err("attach mmu before unmap dma buffer");
		rc = -EINVAL;
		goto unmap_dmabuf_end;
	}

	mapping_info = cpp_iommu_find_mapping_by_fd(mmu_dev, fd);
	if (!mapping_info) {
		cam_err("%s: fd=%d mapping not found", __func__, fd);
		rc = -EINVAL;
		goto unmap_dmabuf_end;
	}

	mapping_info->ref_count--;
	if (mapping_info->ref_count > 0) {
		rc = 0;
		goto unmap_dmabuf_end;
	}

	rc = cpp_iommu_unmap_buffer_and_remove_from_list(mapping_info);
	if (rc < 0)
		cam_err("unmapping or remove list fail, fd=%d, rc=%d", fd, rc);

unmap_dmabuf_end:
	mutex_unlock(&mmu_dev->list_lock);
	return rc;
}

EXPORT_SYMBOL(cpp_iommu_unmap_dmabuf);

static int cpp_iommu_acquire_channel(struct cpp_iommu_device *mmu_dev, uint32_t tid)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_debug("%s: no such channel %x to acquire\n", __func__, tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	if (test_bit(tbu, &mmu_dev->ch_map)) {
		spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);
		pr_err("%s: channel %x not free\n", __func__, tid);
		return -EBUSY;
	}
	set_bit(tbu, &mmu_dev->ch_map);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

static int cpp_iommu_release_channel(struct cpp_iommu_device *mmu_dev, uint32_t tid)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("%s: no such channel %x to release\n", __func__, tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	clear_bit(tbu, &mmu_dev->ch_map);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

static int cpp_iommu_enable_channel(struct cpp_iommu_device *mmu_dev, uint32_t tid)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("%s: no such channel %x to enable\n", __func__, tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	if (!test_bit(tbu, &mmu_dev->ch_map)) {
		spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);
		return -EPERM;
	}

	iommu_enable_tbu(mmu_dev, tbu);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

static int cpp_iommu_disable_channel(struct cpp_iommu_device *mmu_dev, uint32_t tid)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("%s: no such channel %x to disable\n", __func__, tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	if (!test_bit(tbu, &mmu_dev->ch_map)) {
		spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);
		return -EPERM;
	}

	iommu_disable_tbu(mmu_dev, tbu);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

/**
 * cpp_iommu_config_channel - copy tt into iommu rsvd contig memory
 *
 * @mmu_dev:
 * @tid:
 * @ttAddr:
 * @ttSize:
 *
 * Return: 0 on success, error code otherwise.
 */
static int cpp_iommu_config_channel(struct cpp_iommu_device *mmu_dev,
				    uint32_t tid, uint32_t *ttAddr, uint32_t ttSize)
{
	int tbu;
	unsigned long flags;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("%s: no such channel %x to configure\n", __func__, tid);
		return -ENODEV;
	}

	spin_lock_irqsave(&mmu_dev->ops_lock, flags);
	if (!test_bit(tbu, &mmu_dev->ch_map)) {
		spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);
		return -EPERM;
	}

	if (ttAddr && ttSize) {
		memcpy(mmu_dev->info[tbu].ttVirt, ttAddr, ttSize * sizeof(uint32_t));
		mmu_dev->info[tbu].ttSize = ttSize;
	}

	/* iommu_set_tbu_qos(mmu_dev, tbu, 4); */
	iommu_set_tbu_ttaddr(mmu_dev, tbu, mmu_dev->info[tbu].ttPhys);
	iommu_set_tbu_ttsize(mmu_dev, tbu, mmu_dev->info[tbu].ttSize);
	iommu_update_trans_table(mmu_dev, tbu);
	iommu_enable_irqs(mmu_dev);
	spin_unlock_irqrestore(&mmu_dev->ops_lock, flags);

	return 0;
}

static const uint64_t IOMMU_VADDR_BASE = 0x80000000;
static uint64_t cpp_iommu_get_iova(struct cpp_iommu_device *mmu_dev,
				   uint32_t tid, uint32_t offset)
{
	int tbu;
	uint64_t iovAddr;

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("%s: no such channel %x to get sva\n", __func__, tid);
		return -ENODEV;
	}

	iovAddr = iommu_bva_low(mmu_dev) + 0x2000000ULL * tbu + (offset & 0xfff);

	return iovAddr;
}

static int cpp_iommu_setup_timeout_address(struct cpp_iommu_device *mmu_dev)
{
	iommu_reg_write(mmu_dev, REG_IOMMU_TOAL,
			mmu_dev->to_dma_addr & 0xffffffff);
	iommu_reg_write(mmu_dev, REG_IOMMU_TOAH,
			(mmu_dev->to_dma_addr >> 32) & 0x1);

	return 0;
}

static int __iommu_fill_ttb_by_sg(struct sg_table *sgt, uint32_t offset,
				  uint32_t length, uint32_t *tt_base)
{
	size_t temp_size = 0, temp_offset, temp_length;
	dma_addr_t start_addr, end_addr, dmad = 0;
	struct scatterlist *sg;
	int i, tt_size = 0;

	sg = sgt->sgl;
	for (i = 0; i < sgt->nents; ++i, sg = sg_next(sg)) {
		pr_debug("sg%d: addr 0x%llx, size 0x%x", i, sg_phys(sg),
			 sg_dma_len(sg));
		temp_size += sg_dma_len(sg);
		if (temp_size <= offset)
			continue;

		if (offset > temp_size - sg_dma_len(sg))
			temp_offset = offset - temp_size + sg_dma_len(sg);
		else
			temp_offset = 0;

		start_addr = ((phys_cpu2cam(sg_phys(sg)) + temp_offset) >> 12) << 12;

		temp_length = temp_size - offset;
		if (temp_length >= length)
			temp_offset = sg_dma_len(sg) - temp_length + length;
		else
			temp_offset = sg_dma_len(sg);

		end_addr = ((phys_cpu2cam(sg_phys(sg)) + temp_offset + 0xfff) >> 12) << 12;

		for (dmad = start_addr; dmad < end_addr; dmad += 0x1000)
			tt_base[tt_size++] = (dmad >> 12) & 0x3fffff;

		if (temp_length >= length)
			break;
	}

	if (dmad) {		/* extend trans table */
		tt_base[tt_size++] = (dmad >> 12) & 0x3fffff;
		tt_base[tt_size++] = (dmad >> 12) & 0x3fffff;
		tt_base[tt_size++] = (dmad >> 12) & 0x3fffff;
	}

	return tt_size;
}

/**
 * cpp_iommu_setup_sglist - setup sglist for tbu 
 *
 * @mmu_dev:
 * @tid:
 * @fd: mapped fd
 * @offset: planar data offset in dma buffer
 * @length: planar length
 *
 * Return: 0 on success, error code otherwise.
 */
static int cpp_iommu_setup_sglist(struct cpp_iommu_device *mmu_dev,
				  uint32_t tid, int fd, uint32_t offset,
				  uint32_t length)
{
	struct cam_dmabuf_info *mapping_info;
	int tbu;
	uint32_t *tt_base;	/* translation table cpu base */

	tbu = tid_to_tbu(mmu_dev, tid);
	if (tbu < 0) {
		pr_err("%s: invalid tid 0x%x\n", __func__, tid);
		return -ENODEV;
	}

	mutex_lock(&mmu_dev->list_lock);
	mapping_info = cpp_iommu_find_mapping_by_fd(mmu_dev, fd);
	if (!mapping_info) {
		mutex_unlock(&mmu_dev->list_lock);
		cam_err("%s: tid=0x%x, fd=%d mapping not found", __func__, tid, fd);
		return -EINVAL;
	}

	tt_base = (uint32_t *) mmu_dev->info[tbu].ttVirt;
	mmu_dev->info[tbu].ttSize =
	    __iommu_fill_ttb_by_sg(mapping_info->table, offset, length, tt_base);

	mutex_unlock(&mmu_dev->list_lock);
	return 0;
}

__maybe_unused static int cpp_iommu_dump_register(struct cpp_iommu_device *mmu_dev)
{
	int tbu;
	struct cam_dmabuf_info *mapping, *_mapping;

	mutex_lock(&mmu_dev->list_lock);
	if (!list_empty(&mmu_dev->iommu_buf_list)) {
		cam_err("%s: iommu buffer liset not empty==>", __func__);
		list_for_each_entry_safe(mapping, _mapping,
					 &mmu_dev->iommu_buf_list, list) {
			cam_err("fd=%d, dmabuf=%p, length=%ld, direction=%d",
				mapping->fd, mapping->buf, mapping->len, mapping->dir);
			cpp_iommu_unmap_buffer_and_remove_from_list(mapping);
		}
	}
	mutex_unlock(&mmu_dev->list_lock);

	for (tbu = 0; tbu < 32; ++tbu) {
		cam_dbg("TBU%d: ttAddr = 0x%llx, ttSize = 0x%zx\n", tbu,
			mmu_dev->info[tbu].ttPhys, mmu_dev->info[tbu].ttSize);
		cam_dbg("REG_IOMMU_TTBL%d 0x%x=0x%08x\n", tbu,
			REG_IOMMU_TTBL(tbu),
			iommu_reg_read(mmu_dev, REG_IOMMU_TTBL(tbu)));
		cam_dbg("REG_IOMMU_TTBH%d 0x%x=0x%08x\n", tbu,
			REG_IOMMU_TTBH(tbu),
			iommu_reg_read(mmu_dev, REG_IOMMU_TTBH(tbu)));
		cam_dbg("REG_IOMMU_TCR0%d 0x%x=0x%08x\n", tbu,
			REG_IOMMU_TCR0(tbu),
			iommu_reg_read(mmu_dev, REG_IOMMU_TCR0(tbu)));
		cam_dbg("REG_IOMMU_TCR1%d 0x%x=0x%08x\n", tbu,
			REG_IOMMU_TCR1(tbu),
			iommu_reg_read(mmu_dev, REG_IOMMU_TCR1(tbu)));
		cam_dbg("REG_IOMMU_STAT%d 0x%x=0x%08x\n", tbu,
			REG_IOMMU_STAT(tbu),
			iommu_reg_read(mmu_dev, REG_IOMMU_STAT(tbu)));
	}

	return 0;
}

static int cpp_iommu_dump_dmabuf(struct cpp_iommu_device *mmu_dev)
{
	struct cam_dmabuf_info *mapping, *_mapping;

	mutex_lock(&mmu_dev->list_lock);
	if (!list_empty(&mmu_dev->iommu_buf_list)) {
		cam_err("%s: iommu buffer liset not empty==>", __func__);
		list_for_each_entry_safe (mapping, _mapping,
					  &mmu_dev->iommu_buf_list, list) {
			cam_err("fd=%d, dmabuf=%p, length=%zd, direction=%d",
				mapping->fd, mapping->buf, mapping->len,
				mapping->dir);
			cpp_iommu_unmap_buffer_and_remove_from_list(mapping);
		}
	}
	mutex_unlock(&mmu_dev->list_lock);

	return 0;
}
static struct cpp_iommu_ops mmu_ops = {
	.acquire_channel = cpp_iommu_acquire_channel,
	.release_channel = cpp_iommu_release_channel,
	.enable_channel = cpp_iommu_enable_channel,
	.disable_channel = cpp_iommu_disable_channel,
	.config_channel = cpp_iommu_config_channel,
	.setup_timeout_address = cpp_iommu_setup_timeout_address,
	.get_iova = cpp_iommu_get_iova,
	.setup_sglist = cpp_iommu_setup_sglist,
	.dump_status = cpp_iommu_dump_dmabuf,
};

int cpp_iommu_register(struct cpp_device *cpp_dev)
{
	struct cpp_iommu_device *mmu_dev;
	size_t size, offset;
	int i;

	mmu_dev = devm_kzalloc(&(cpp_dev->pdev->dev),
			       sizeof(struct cpp_iommu_device), GFP_KERNEL);
	if (!mmu_dev)
		return -ENOMEM;

	size = IOMMU_TRANS_TAB_MAX_NUM * sizeof(uint32_t) * CPP_IOMMU_CH_NUM;
	mmu_dev->rsvd_cpu_addr =
	    dmam_alloc_coherent(&cpp_dev->pdev->dev, size, &mmu_dev->rsvd_dma_addr,
				GFP_KERNEL);
	if (!mmu_dev->rsvd_cpu_addr) {
		pr_err("%s: alloc reserved memory failed\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < CPP_IOMMU_CH_NUM; ++i) {
		offset = IOMMU_TRANS_TAB_MAX_NUM * sizeof(uint32_t) * i;
		mmu_dev->info[i].ttPhys = mmu_dev->rsvd_dma_addr + offset;
		mmu_dev->info[i].ttVirt = mmu_dev->rsvd_cpu_addr + offset;
	}

	mmu_dev->to_cpu_addr = dmam_alloc_coherent(
		&cpp_dev->pdev->dev, 0x1000, &mmu_dev->to_dma_addr, GFP_KERNEL);
	if (!mmu_dev->to_cpu_addr) {
		pr_err("%s: alloc timeout memory failed\n", __func__);
		return -ENOMEM;
	} else {
		pr_debug("%s: timeout memory %pad\n", __func__,
			&mmu_dev->to_dma_addr);
	}

	mmu_dev->regs_base = cpp_dev->regs_base;
	mmu_dev->ops = &mmu_ops;

	INIT_LIST_HEAD(&mmu_dev->iommu_buf_list);
	mutex_init(&mmu_dev->list_lock);
	spin_lock_init(&mmu_dev->ops_lock);
	mmu_dev->cpp_dev = cpp_dev;
	cpp_dev->mmu_dev = mmu_dev;

	pr_debug("%s X\n", __func__);

	return 0;
}

EXPORT_SYMBOL(cpp_iommu_register);

void cpp_iommu_unregister(struct cpp_device *cpp_dev)
{
	struct cpp_iommu_device *mmu_dev = cpp_dev->mmu_dev;
	size_t size;

	mutex_destroy(&mmu_dev->list_lock);
	size = IOMMU_TRANS_TAB_MAX_NUM * sizeof(uint32_t) * CPP_IOMMU_CH_NUM;
	dmam_free_coherent(&cpp_dev->pdev->dev, size, mmu_dev->rsvd_cpu_addr,
			   mmu_dev->rsvd_dma_addr);
	dmam_free_coherent(&cpp_dev->pdev->dev, size, mmu_dev->to_cpu_addr,
			   mmu_dev->to_dma_addr);
	devm_kfree(&(cpp_dev->pdev->dev), cpp_dev->mmu_dev);
	cpp_dev->mmu_dev = NULL;

	pr_debug("%s X\n", __func__);
}

MODULE_IMPORT_NS(DMA_BUF);
EXPORT_SYMBOL(cpp_iommu_unregister);

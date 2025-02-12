/* SPDX-License-Identifier: GPL-2.0 */
/*
 * cpp_iommu.h - Driver for CPP IOMMU
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __CPP_IOMMU_H__
#define __CPP_IOMMU_H__

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>

#define CPP_IOMMU_TBU_NUM	(32)
#define CPP_IOMMU_CH_NUM	(CPP_IOMMU_TBU_NUM)
#define IOMMU_TRANS_TAB_MAX_NUM (8192)
#define MMU_TID(port_id, chnl_id)	((port_id << 8) | chnl_id)

#define IOMMU_MAP_FLAG_READ_ONLY	(1 << 0)
#define IOMMU_MAP_FLAG_WRITE_ONLY	(1 << 1)
#define IOMMU_MAP_FLAG_READ_WRITE	(1 << 2)
#define IOMMU_MAP_FLAG_NOSYNC	(1 << 3)

enum cpp_iommu_state {
	CPP_IOMMU_DETACHED,
	CPP_IOMMU_ATTACHED,
};

struct iommu_ch_info {
	void *ttVirt;
	uint64_t ttPhys;
	size_t ttSize;
};

struct cpp_iommu_device {
	struct cpp_device *cpp_dev;
	void __iomem *regs_base;
	unsigned long ch_map;
	struct iommu_ch_info info[CPP_IOMMU_CH_NUM];
	spinlock_t ops_lock;
	dma_addr_t rsvd_dma_addr;
	void *rsvd_cpu_addr;
	dma_addr_t to_dma_addr;
	void *to_cpu_addr;
	struct mutex list_lock;
	struct list_head iommu_buf_list;
	enum cpp_iommu_state state;

	struct cpp_iommu_ops *ops;
};

struct cpp_iommu_ops {
	int (*acquire_channel)(struct cpp_iommu_device *mmu_dev, uint32_t tid);
	int (*release_channel)(struct cpp_iommu_device *mmu_dev, uint32_t tid);
	int (*enable_channel)(struct cpp_iommu_device *mmu_dev, uint32_t tid);
	int (*disable_channel)(struct cpp_iommu_device *mmu_dev, uint32_t tid);
	int (*config_channel)(struct cpp_iommu_device *mmu_dev, uint32_t tid,
			      uint32_t *ttAddr, uint32_t ttSize);
	uint64_t(*get_iova) (struct cpp_iommu_device *mmu_dev, uint32_t tid,
			     uint32_t offset);
	int (*setup_timeout_address)(struct cpp_iommu_device *mmu_dev);
	int (*setup_sglist)(struct cpp_iommu_device *mmu_dev, uint32_t tid,
			    int fd, uint32_t offset, uint32_t length);
	int (*dump_status)(struct cpp_iommu_device *mmu_dev);
};

int cpp_iommu_register(struct cpp_device *cpp_dev);
void cpp_iommu_unregister(struct cpp_device *cpp_dev);
int cpp_iommu_map_dmabuf(struct cpp_iommu_device *mmu_dev, int fd,
			 uint32_t map_flags, dma_addr_t *paddr_ptr);
int cpp_iommu_unmap_dmabuf(struct cpp_iommu_device *mmu_dev, int fd);
#endif /* ifndef __CPP_IOMMU_H__ */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * isp_iommu.h - Driver for ISP IOMMU
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __ISP_IOMMU_H__
#define __ISP_IOMMU_H__

#include <linux/types.h>
#include <linux/spinlock.h>
#include "hw_reg_iommu.h"

#define ISP_IOMMU_TBU_NUM (24)
#define ISP_IOMMU_CH_NUM (ISP_IOMMU_TBU_NUM)
#define IOMMU_TRANS_TAB_MAX_NUM (8192)
#define MMU_TID(direction, port_id, plane_id) (((direction) << 8) | ((port_id) << 4) | (plane_id))

#define isp_mmu_call(mmu_dev, f, args...)		\
	({						\
		struct isp_iommu_device *__mmu_dev = (mmu_dev);		\
		int __result;						\
		if (!__mmu_dev)						\
			__result = -ENODEV;				\
		else if (!(__mmu_dev->ops && __mmu_dev->ops->f))	\
			__result = -ENOIOCTLCMD;	\
		else					\
			__result = __mmu_dev->ops->f(__mmu_dev, ##args);	\
		__result;				\
	})

struct iommu_ch_info {
	uint32_t tid;
	uint32_t ttSize;
	uint64_t ttAddr;
};

struct isp_iommu_device {
	struct device *dev;
	unsigned long regs_base;
	unsigned long ch_map;
	uint32_t ch_matrix[ISP_IOMMU_CH_NUM];
	struct iommu_ch_info info[ISP_IOMMU_CH_NUM];
	spinlock_t ops_lock;

	struct isp_iommu_ops *ops;
};

struct isp_iommu_ops {
	int (*acquire_channel)(struct isp_iommu_device *mmu_dev, uint32_t tid);
	int (*release_channel)(struct isp_iommu_device *mmu_dev, uint32_t tid);
	int (*enable_channel)(struct isp_iommu_device *mmu_dev, uint32_t tid);
	int (*disable_channel)(struct isp_iommu_device *mmu_dev, uint32_t tid);
	int (*config_channel)(struct isp_iommu_device *mmu_dev, uint32_t tid,
			      uint64_t ttAddr, uint32_t ttSize);
	uint64_t (*get_sva)(struct isp_iommu_device *mmu_dev, uint32_t tid,
			    uint32_t offset);
	unsigned int (*irq_status)(struct isp_iommu_device *mmu_dev);
	int (*dump_channel_regs)(struct isp_iommu_device *mmu_dev, uint32_t tid);
	void (*set_timeout_default_addr)(struct isp_iommu_device *mmu_dev, uint64_t timeout_default_addr);
};

struct isp_iommu_device *isp_iommu_create(struct device *dev, unsigned long regs_base);
void isp_iommu_unregister(struct isp_iommu_device *mmu_dev);
#endif /* ifndef __ISP_IOMMU_H__ */

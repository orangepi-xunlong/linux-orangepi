/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
 *
 * Author: huangshuosheng <huangshuosheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_SUNXI_IOMMU_H
#define __LINUX_SUNXI_IOMMU_H
#include <linux/iommu.h>
#include <linux/iova.h>

typedef void (*sunxi_iommu_fault_cb)(void);
extern void sunxi_iommu_register_fault_cb(sunxi_iommu_fault_cb cb, unsigned int master_id);
extern void sunxi_enable_device_iommu(unsigned int master_id, bool flag);
extern void sunxi_reset_device_iommu(unsigned int master_id);
extern struct iommu_domain *global_iommu_domain;
ssize_t sunxi_iommu_dump_pgtable(char *buf, size_t buf_len,
					       bool for_sysfs_show);
void sunxi_iommu_prevent_hang_enable(int enable);
void sunxi_iommu_enable_interrupt(int enable);
#if IS_ENABLED(CONFIG_AW_IOMMU_DISTRIBUTE)
void sunxi_iommu_master_ready(struct device *dev);
#else
static inline void sunxi_iommu_master_ready(struct device *dev) { ; }
#endif

enum iommu_dma_cookie_type {
	IOMMU_DMA_IOVA_COOKIE,
	IOMMU_DMA_MSI_COOKIE,
};

struct iommu_dma_cookie {
	enum iommu_dma_cookie_type	type;
	union {
		/* Full allocator for IOMMU_DMA_IOVA_COOKIE */
		struct iova_domain	iovad;
		/* Trivial linear page allocator for IOMMU_DMA_MSI_COOKIE */
		dma_addr_t		msi_iova;
	};
	struct list_head		msi_page_list;

	/* Domain for flush queue callback; NULL if flush queue not in use */
	struct iommu_domain		*fq_domain;
};

#endif  /* __LINUX_SUNXI_IOMMU_H */

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi iommu: main structures
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Written by Hiroshi DOYU <Hiroshi.DOYU@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include "sunxi-iommu-pgtable.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
//iommu core dont call detach callback any more
#define DETACH_OP_DEPRECATED
#endif
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
//iommu resv region allocation require gfp flags
#define RESV_REGION_NEED_GFP_FLAG
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

#define MAX_SG_SIZE (128 << 20)
#define MAX_SG_TABLE_SIZE ((MAX_SG_SIZE / SPAGE_SIZE) * sizeof(u32))
#define DUMP_REGION_MAP 0
#define DUMP_REGION_RESERVE 1
struct dump_region {
	u32 access_mask;
	size_t size;
	u32 type;
	dma_addr_t phys, iova;
};
struct sunxi_iommu_dev;


int sunxi_iova_test_write(dma_addr_t iova, u32 val);
unsigned long sunxi_iova_test_read(dma_addr_t iova);
void sunxi_set_debug_mode(void);
void sunxi_set_prefetch_mode(void);
int sunxi_iommu_check_cmd(struct device *dev, void *data);
u32 sunxi_iommu_dump_rsv_list(struct list_head *rsv_list, ssize_t len,
			      char *buf, size_t buf_len, bool for_sysfs_show);
int iova_show_on_irq(void);
void sunxi_iommu_register_vendorhook(void);
void sunxi_iommu_init_debugfs(struct sunxi_iommu_dev *sunxi_iommu);
void sunxi_iommu_release_debugfs(void);

#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
struct sunxi_iommu_iova_info {
	char dev_name[32];
	dma_addr_t iova;
	size_t size;
	time64_t timestamp;
	struct sunxi_iommu_iova_info *next;
};

#endif

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * verisilicon iommu: simple virtual address space management
 *
 * Copyright (c) 2025, Collabora
 *
 * Written by Benjamin Gaignard <benjamin.gaignard@collabora.com>
 */

#ifndef _VSI_IOMMU_H_
#define _VSI_IOMMU_H_

struct iommu_domain;

#if IS_ENABLED(CONFIG_VSI_IOMMU)
void vsi_iommu_restore_ctx(struct iommu_domain *domain);
#else
static inline void vsi_iommu_restore_ctx(struct iommu_domain *domain) {}
#endif

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * hw_reg_iommu.h
 *
 * register for isp iommu
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __REGS_ISP_IOMMU_H__
#define __REGS_ISP_IOMMU_H__

/* TBU(n) registers */
#define REG_IOMMU_TTBL(n)		(0x12240 + 0x20 * (n))
#define REG_IOMMU_TTBH(n)		(0x12244 + 0x20 * (n))
#define REG_IOMMU_TCR0(n)		(0x12248 + 0x20 * (n))
#define REG_IOMMU_TCR1(n)		(0x1224c + 0x20 * (n))
#define REG_IOMMU_STAT(n)		(0x12250 + 0x20 * (n))

/* TOP registers */
#define REG_IOMMU_BVAL		(0x12200)
#define REG_IOMMU_BVAH		(0x12204)
#define REG_IOMMU_TVAL		(0x12208)
#define REG_IOMMU_TVAH		(0x1220c)
#define REG_IOMMU_GIRQ_STAT	(0x12210)
#define REG_IOMMU_GIRQ_ENA	(0x12214)
#define REG_IOMMU_TIMEOUT	(0x12218)
#define REG_IOMMU_ERR_CLR	(0x1221c)
#define REG_IOMMU_LVAL		(0x12220)
#define REG_IOMMU_LVAH		(0x12224)
#define REG_IOMMU_LPAL		(0x12228)
#define REG_IOMMU_LPAH		(0x1222c)
#define REG_IOMMU_TIMEOUT_ADDR_LOW	(0x12234)
#define REG_IOMMU_TIMEOUT_ADDR_HIGH	(0x12238)
#define REG_IOMMU_VER		(0x1223c)

#define MMU_RD_TIMEOUT		(1 << 16)
#define MMU_WR_TIMEOUT		(1 << 17)

#endif /* ifndef __REGS_ISP_IOMMU_H__ */

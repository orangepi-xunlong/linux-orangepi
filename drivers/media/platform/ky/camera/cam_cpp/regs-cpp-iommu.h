/* SPDX-License-Identifier: GPL-2.0 */
/*
 * regs-cpp-iommu.h
 *
 * register for cpp iommu
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __REGS_CPP_IOMMU_H__
#define __REGS_CPP_IOMMU_H__

/* TBU(n) registers */
#define REG_IOMMU_TTBL(n)		(0x1040 + 0x20 * (n))
#define REG_IOMMU_TTBH(n)		(0x1044 + 0x20 * (n))
#define REG_IOMMU_TCR0(n)		(0x1048 + 0x20 * (n))
#define REG_IOMMU_TCR1(n)		(0x104c + 0x20 * (n))
#define REG_IOMMU_STAT(n)		(0x1050 + 0x20 * (n))

/* TOP registers */
#define REG_IOMMU_BVAL		(0x1000)
#define REG_IOMMU_BVAH		(0x1004)
#define REG_IOMMU_TVAL		(0x1008)
#define REG_IOMMU_TVAH		(0x100c)
#define REG_IOMMU_GIRQ_STAT	(0x1010)
#define REG_IOMMU_GIRQ_ENA	(0x1014)
#define REG_IOMMU_TIMEOUT	(0x1018)
#define REG_IOMMU_ERR_CLR	(0x101c)
#define REG_IOMMU_LVAL		(0x1020)
#define REG_IOMMU_LVAH		(0x1024)
#define REG_IOMMU_LPAL		(0x1028)
#define REG_IOMMU_LPAH		(0x102c)
#define REG_IOMMU_TOAL		(0x1034)
#define REG_IOMMU_TOAH		(0x1038)
#define REG_IOMMU_VER		(0x103c)

#endif /* ifndef __REGS_CPP_IOMMU_H__ */

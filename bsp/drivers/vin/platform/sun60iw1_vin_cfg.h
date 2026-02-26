/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
 /*
  * Hawkview ISP - sun60iw1_vin_cfg.h module
  *
  * Copyright (c) 2022 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
  *
  * Authors:  Cao Fuming <caofuming@allwinnertech.com>
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation.
  */

#ifndef _SUN60IW1_VIN_CFG_H_
#define _SUN60IW1_VIN_CFG_H_

#define CSI_CCU_REGS_BASE			0x05800000
#define CSI_TOP_REGS_BASE			0x05800800

#define CSI0_REGS_BASE				0x05820000
#define CSI1_REGS_BASE				0x05821000
#define CSI2_REGS_BASE				0x05822000
#define CSI3_REGS_BASE				0x05823000

#define ISP_REGS_BASE				0x05900000

#define VIPP0_REGS_BASE				0x05910000
#define VIPP1_REGS_BASE				0x05910400
#define VIPP2_REGS_BASE				0x05910800
#define VIPP3_REGS_BASE				0x05910C00
#define VIPP4_REGS_BASE				0x05911000
#define VIPP5_REGS_BASE				0x05911400
#define VIPP6_REGS_BASE				0x05911800
#define VIPP7_REGS_BASE				0x05911c00
#define VIPP8_REGS_BASE				0x05912000
#define VIPP9_REGS_BASE				0x05912400
#define VIPP10_REGS_BASE			0x05912800
#define VIPP11_REGS_BASE			0x05912c00

#define CSI_DMA0_REG_BASE			0x05830000
#define CSI_DMA1_REG_BASE			0x05831000
#define CSI_DMA2_REG_BASE			0x05832000
#define CSI_DMA3_REG_BASE			0x05833000
#define CSI_DMA4_REG_BASE			0x05834000
#define CSI_DMA5_REG_BASE			0x05835000
#define CSI_DMA6_REG_BASE			0x05836000
#define CSI_DMA7_REG_BASE			0x05837000
#define CSI_DMA8_REG_BASE			0x05838000
#define CSI_DMA9_REG_BASE			0x05839000
#define CSI_DMA10_REG_BASE			0x0583a000
#define CSI_DMA11_REG_BASE			0x0583b000

#define GPIO_REGS_VBASE				0x02000000

#define VIN_MAX_DEV			24
#define VIN_MAX_CSI			4
#define VIN_MAX_TDM			1
#define VIN_MAX_CCI			4
#define VIN_MAX_MIPI		3
#define VIN_MAX_ISP			5
#define VIN_MAX_SCALER		24

#define MAX_CH_NUM			4

#define ISP_IOMMU_MASTER	0
#define CSI_DMA0_IOMMU_MASTER	1
#define CSI_DMA1_IOMMU_MASTER	2

#endif /* _SUN60IW1_VIN_CFG_H_ */

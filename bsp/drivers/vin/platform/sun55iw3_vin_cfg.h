/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
 /*
  * Hawkview ISP - sun55iw3_vin_cfg.h module
  *
  * Copyright (c) 2022 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
  *
  * Authors:  Cao Fuming <caofuming@allwinnertech.com>
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation.
  */

#ifndef _SUN55IW3_VIN_CFG_H_
#define _SUN55IW3_VIN_CFG_H_

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

#define CSI_DMA0_REG_BASE			0x05830000
#define CSI_DMA1_REG_BASE			0x05831000
#define CSI_DMA2_REG_BASE			0x05832000
#define CSI_DMA3_REG_BASE			0x05833000
#define CSI_DMA4_REG_BASE			0x05834000
#define CSI_DMA5_REG_BASE			0x05835000

#define GPIO_REGS_VBASE				0x02000000

#define VIN_MAX_DEV			18
#define VIN_MAX_CSI			4
#define VIN_MAX_TDM			1
#define VIN_MAX_CCI			4
#define VIN_MAX_MIPI		4
#define VIN_MAX_ISP			7
#define VIN_MAX_SCALER		18
#define VIN_MAX_PINCTRL		3

#define MAX_CH_NUM			4

#define CSI_IOMMU_MASTER	1
#define ISP_IOMMU_MASTER	0

#endif /* _SUN55IW3_VIN_CFG_H_ */

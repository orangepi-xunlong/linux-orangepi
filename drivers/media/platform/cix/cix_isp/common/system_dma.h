/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SYSTEM_DMA_H__
#define __SYSTEM_DMA_H__

u32 CDMA_Read_Int32(u32 offset);
void CDMA_Write_Int32(u32 offset, u32 value);

u32 VDMA_Read_Int32(u32 baseaddr, u32 offset);
void VDMA_Write_Int32(u32 baseaddr, u32 offset, u32 value);

s32 xdma(u32 unRmtAddr, u32 unLocalAddr, u32 unBytes, u32 bLocal2Rmt);

#endif

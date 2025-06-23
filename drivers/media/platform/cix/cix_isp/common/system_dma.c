// SPDX-License-Identifier: GPL-2.0
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

#include "armcb_camera_io_drv.h"
#include "armcb_register.h"
#include "system_logger.h"
#include "types_utils.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_COMMON
#endif

#ifdef CONFIG_ARENA_FPGA_PLATFORM

static DEFINE_SPINLOCK(xdma_lock);

u32 CDMA_Read_Int32(u32 offset)
{
	void __iomem *virt_addr = NULL;
	u32 reg_val = 0;

	virt_addr = armcb_ispmem_get_cdma_base();
	reg_val = readl(virt_addr + offset);

	return reg_val;
}

void CDMA_Write_Int32(u32 offset, u32 value)
{
	void __iomem *virt_addr = NULL;

	virt_addr = armcb_ispmem_get_cdma_base();
	writel(value, virt_addr + offset);
}

s32 cdma_en(u32 src_addr, u32 dst_addr, u32 bytes)
{
	const u32 dma_max_bytes =
		(0x04000000 - CACHELINE_SIZE); // 0x04000000: Xilinx IP limited.
	u32 temp = 0;
	u32 left = 0;
	u32 src_in_7020 = 1, dst_in_7020 = 0, src_in_440, dst_in_440, addr_ok;
	s32 res = 0;

	LOG(LOG_DEBUG, "src_addr 0x%x, dst_addr=0x%x, bytes=0x%x", src_addr,
		dst_addr, bytes);
	left = bytes;

	while (left > 0) {
		if (left > dma_max_bytes)
			bytes = dma_max_bytes;
		else
			bytes = left;

		// Wait DMA to be idle.
		temp = CDMA_Read_Int32(0x04);
		while (!(temp & 0x2)) { // Bit12: IOC, Bit1: Idle.
			temp = CDMA_Read_Int32(0x04);
		}

		// Address validity check.
		// (src_addr+bytes)/(dst_addr+bytes) can't cross the end address of xc7z020.
		//
		src_in_7020 = (src_addr < MEM_END_7020) &&
				  ((src_addr + bytes - 1) <= MEM_END_7020);
		dst_in_7020 = (dst_addr < MEM_END_7020) &&
				  ((dst_addr + bytes - 1) <= MEM_END_7020);
		src_in_440 =
			(src_addr >= MEM_START_440) &&
			((src_addr + bytes - 1) <=
			 MEM_END_440); // Assumed MEM_START_440 >= MEM_END_7020.
		dst_in_440 = (dst_addr >= MEM_START_440) &&
				 ((dst_addr + bytes - 1) <= MEM_END_440);

		addr_ok = (src_in_7020 && dst_in_7020) ||
			  (src_in_7020 && dst_in_440) ||
			  (src_in_440 && dst_in_440) ||
			  (src_in_440 && dst_in_7020);

		if (!addr_ok) {
			LOG(LOG_INFO,
				"DMA address invalid! src= 0x%x, dest= 0x%x, size 0x%x",
				src_addr, dst_addr, bytes);
			res = -1;
			goto RES_EXIT;
		}

		// Set source address.
		CDMA_Write_Int32(0x18, src_addr);
		CDMA_Write_Int32(0x1c,
				 0x0); // High 32-bit if address bit >32bit.

		// Set destination address.
		CDMA_Write_Int32(0x20, dst_addr);
		CDMA_Write_Int32(0x24,
				 0x0); // High 32-bit if address bit >32bit.

		// Set BTT and start the tx.
		CDMA_Write_Int32(0x28, bytes);

		// Wait DMA done.
		temp = CDMA_Read_Int32(0x04);
		temp = CDMA_Read_Int32(0x04);
		while (!(temp & 0x002)) { // Bit12: IOC, Bit1: Idle.
			temp = CDMA_Read_Int32(0x04);
		}

		// For next loop.
		src_addr += bytes;
		dst_addr += bytes;
		left -= bytes;
		res += bytes;
	}

RES_EXIT:

	return res;
}

//---------------------------------------------------------------------------------------------------------
// Function name: xdma()
// Description:   Do data transfer between XCVU440 and XC7Z020 on Arena.
// Parameters:
//    u32 rmt_addr:   Address in XCVU440 domain for DMA.
//    u32 local_addr: Address in XC7Z020 domain for DMA.
//    u32 bytes:      Bytes to transfer.
//    u32 is_local_to_rmt:  0=Xfer data from XCVU440 to XC7Z020(local).
//                     Others = Xfer data from XC7Z020(local) to XCVU440.
// Notes:
//   1. Memory accessible for dma_x:
//      --------------------------------------+-----------------+-----------------------------------------
//                                            +                 + Page Address and offset for CPU/XC7Z020
//      Address on XCVU440                    + Address of ISP  +-----------------------------------------
//                                            +                 +  Page        + Offset in Page
//      --------------------------------------+-----------------+-----------------------------------------
//      0x1_C000_0000    Reserved(1GB)             N/A               7             0x4000_0000~0x7FFF_FFFF
//      0x1_8000_0000    Reserved(1GB)             N/A               6             0x4000_0000~0x7FFF_FFFF
//      0x1_4000_0000    Reserved(1GB)             N/A               5             0x4000_0000~0x7FFF_FFFF
//      0x1_0000_0000    Reserved(1GB)             N/A               4             0x4000_0000~0x7FFF_FFFF
//      0x0_C000_0000    RAW dump(1GB)             0xC000_0000       3             0x4000_0000~0x7FFF_FFFF
//      0x0_8000_0000    RAW dump(1GB)             0x8000_0000       2             0x4000_0000~0x7FFF_FFFF
//      0x0_4800_0000    ISP buffer(Max 896MB)     0x4800_0000       1             0x4800_0000~0x7FFF_FFFF
//      0x0_4000_0000    Dispplay Buffer(128MB)     0x4000_0000       1             0x4000_0000~0x47FF_FFFF
//      0x0_0000_0000    Reserved(1GB)            0x0000_0000       0             0x4000_0000~0x7FFF_FFFF
//
//      Access to 0x1_0000_0000 and above N/A so far.
//
//----------------------------------------------------------------------------------------------------------
s32 xdma(u32 rmt_addr, u32 local_addr, u32 bytes, u32 is_local_to_rmt)
{
	u32 page, page_bak, temp;
	s32 ret = 0;

	spin_lock(&xdma_lock);

	if (rmt_addr >= 0xC0000000) {
		page = 3;
		rmt_addr = rmt_addr - 0xC0000000 + 0x40000000;
	} else if (rmt_addr >= 0x80000000) {
		page = 2;
		rmt_addr = rmt_addr - 0x80000000 + 0x40000000;
	} else if (rmt_addr >= 0x40000000) {
		page = 1;
		rmt_addr = rmt_addr - 0x40000000 + 0x40000000;
	} else if (rmt_addr >= 0x00000000) {
		page = 0;
		rmt_addr = rmt_addr - 0x00000000 + 0x40000000;
	}
	page = (page << 16);

	// Set Page address.
	temp = armcb_apb2_read_reg(0x14);
	page_bak = temp & (0x07 << 16);
	LOG(LOG_DEBUG, "temp(%x) page_bak(%x) page(%x)", temp, page_bak, page);

	if (page != page_bak) {
		temp &= (~(0x07 << 16));
		temp |= page;

		armcb_apb2_write_reg(0x14, temp);
	}

	if (is_local_to_rmt) {
		ret = cdma_en(local_addr, rmt_addr, bytes);
	} else {
		ret = cdma_en(rmt_addr, local_addr,
			      bytes); // cdma_en(src, dest, size)
	}

	// Resume last page.
	if (page != page_bak) {
		temp = armcb_apb2_read_reg(0x14);
		temp &= (~(0x07 << 16));
		temp |= page_bak;
		armcb_apb2_write_reg(0x14, temp);
	}
	spin_unlock(&xdma_lock);
	return ret;
}
#endif

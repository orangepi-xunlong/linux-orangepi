
/*
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
************************************************************************************************************************
*                                                      eNand
*                                     Nand flash driver format module define
*
*                             Copyright(C), 2008-2009, SoftWinners
*Microelectronic Co., Ltd.
*											       All
*Rights Reserved
*
* File Name : nand_format.h
*
* Author : Kevin.z
*
* Version : v0.1
*
* Date : 2008.03.25
*
* Description : This file define the function interface and some data structure
*export
*               for the format module.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Kevin.z         2008.03.25      0.1          build the file
*
************************************************************************************************************************
*/
#ifndef __SPINAND_INTERFACE_FOR_COMMON_H__
#define __SPINAND_INTERFACE_FOR_COMMON_H__

#include "../phy/phy.h"

extern struct _nand_info *SpiNandHwInit(void);
extern __s32 SpiNandHwExit(void);
extern __s32 SpiNandHwSuperStandby(void);
extern __s32 SpiNandHwSuperResume(void);
extern __s32 SpiNandHwNormalStandby(void);
extern __s32 SpiNandHwNormalResume(void);
extern __s32 SpiNandHwShutDown(void);
extern int NAND_WaitDmaFinish(__u32 tx_flag, __u32 rx_flag);
int Nand_Dma_End(__u32 rw, __u32 addr, __u32 length);
extern __s32 spinand_super_block_read(__u32 nDieNum, __u32 nBlkNum, __u32 nPage,
				      __u64 SectBitmap, void *pBuf,
				      void *pSpare);
extern __s32 spinand_super_block_write(__u32 nDieNum, __u32 nBlkNum,
				       __u32 nPage, __u64 SectBitmap,
				       void *pBuf, void *pSpare);
extern __s32 spinand_super_block_erase(__u32 nDieNum, __u32 nBlkNum);
extern __s32 spinand_super_badblock_check(__u32 nDieNum, __u32 nBlkNum);
extern __s32 spinand_super_badblock_mark(__u32 nDieNum, __u32 nBlkNum);

extern int spinand_physic_erase_block(unsigned int chip, unsigned int block);
extern int spinand_physic_read_page(unsigned int chip, unsigned int block,
				    unsigned int page, unsigned int bitmap,
				    unsigned char *mbuf, unsigned char *sbuf);
extern int spinand_physic_write_page(unsigned int chip, unsigned int block,
				     unsigned int page, unsigned int bitmap,
				     unsigned char *mbuf, unsigned char *sbuf);
extern int spinand_physic_bad_block_check(unsigned int chip,
					  unsigned int block);
extern int spinand_physic_bad_block_mark(unsigned int chip, unsigned int block);
extern int spinand_physic_block_copy(unsigned int chip_s, unsigned int block_s,
				     unsigned int chip_d, unsigned int block_d);

extern __u32 SPINAND_GetLsbblksize(void);
extern __u32 SPINAND_GetLsbPages(void);
extern __u32 SPINAND_UsedLsbPages(void);
extern u32 SPINAND_GetPageNo(u32 lsb_page_no);
extern __u32 SPINAND_GetPageSize(void);
extern __u32 SPINAND_GetPhyblksize(void);
extern __u32 SPINAND_GetPageCntPerBlk(void);
extern __u32 SPINAND_GetBlkCntPerChip(void);
extern __u32 SPINAND_GetChipCnt(void);
extern __u32 spinand_get_twoplane_flag(void);

extern int SPINAND_UpdatePhyArch(void);
extern int spinand_uboot_erase_all_chip(unsigned int force_flag);
extern int spinand_erase_chip(unsigned int chip, unsigned int start_block,
			      unsigned int end_block, unsigned int force_flag);
extern int spinand_dragonborad_test_one(unsigned char *buf, unsigned char *oob,
					unsigned int blk_num);
extern int spinand_physic_info_get_one_copy(unsigned int start_block,
					    unsigned int pages_offset,
					    unsigned int *block_per_copy,
					    unsigned int *buf);
extern int spinand_add_len_to_uboot_tail(unsigned int uboot_size);
extern int spinand_get_param_for_uboottail(void *nand_param);
extern int spinand_get_param(void *nand_param);

extern int spinand_write_boot0_one(unsigned char *buf, unsigned int len,
				   unsigned int counter);
extern int spinand_read_boot0_one(unsigned char *buf, unsigned int len,
				  unsigned int counter);
extern int spinand_write_uboot_one(unsigned char *buf, unsigned int len,
				   struct _boot_info *info_buf,
				   unsigned int info_len, unsigned int counter);
extern int spinand_read_uboot_one(unsigned char *buf, unsigned int len,
				  unsigned int counter);
extern __s32 SPINAND_SetPhyArch_V3(struct _boot_info *ram_arch, void *phy_arch);
extern int SPINAND_UpdatePhyArch(void);
extern __s32 spinand_super_page_read(__u32 nDieNum, __u32 nBlkNum, __u32 nPage,
				     __u64 SectBitmap, void *pBuf,
				     void *pSpare);
extern __s32 spinand_super_page_write(__u32 nDieNum, __u32 nBlkNum, __u32 nPage,
				      __u64 SectBitmap, void *pBuf,
				      void *pSpare);

#endif // ifndef __SPINAND_INTERFACE_FOR_COMMON_H__

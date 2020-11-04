
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


#define __NAND_COMMON_INTERFACE_H__

#ifndef _NAND_COMMON_INTERFACE_H
#define _NAND_COMMON_INTERFACE_H

#include "../nand_osal.h"
#include "../phy/phy.h"
#include "../physic_v2/controller/nand_controller.h"
#include "../physic_v2/nand_type_rawnand.h"
#include "../physic_v2/rawnand_drv_cfg.h"
#include "uboot_head.h"

//=============================================================================
#undef GLOBAL
#ifdef _NPHYI_COMMON_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif
GLOBAL struct _nand_info *NandHwInit(void);
GLOBAL int NandHwExit(void);
GLOBAL int NandHwSuperStandby(void);
GLOBAL int NandHwSuperResume(void);
GLOBAL int NandHwNormalStandby(void);
GLOBAL int NandHwNormalResume(void);
GLOBAL int NandHwShutDown(void);
GLOBAL __s32 PHY_Scan_DelayMode(__u32 clk);
GLOBAL void *NAND_GetIOBaseAddr(u32 no);

GLOBAL int nand_physic_bad_block_mark(unsigned int chip, unsigned int block);
GLOBAL int nand_physic_bad_block_check(unsigned int chip, unsigned int block);
GLOBAL int nand_physic_write_page(unsigned int chip, unsigned int block,
				  unsigned int page, unsigned int bitmap,
				  unsigned char *mbuf, unsigned char *sbuf);
GLOBAL int nand_physic_erase_block(unsigned int chip, unsigned int block);
GLOBAL int nand_physic_read_page(unsigned int chip, unsigned int block,
				 unsigned int page, unsigned int bitmap,
				 unsigned char *mbuf, unsigned char *sbuf);
GLOBAL int nand_physic_block_copy(unsigned int chip_s, unsigned int block_s,
				  unsigned int chip_d, unsigned int block_d);

GLOBAL int PHY_VirtualPageRead(unsigned int nDieNum, unsigned int nBlkNum,
			       unsigned int nPage, uint64 SectBitmap,
			       void *pBuf, void *pSpare);
GLOBAL int PHY_VirtualPageWrite(unsigned int nDieNum, unsigned int nBlkNum,
				unsigned int nPage, uint64 SectBitmap,
				void *pBuf, void *pSpare);
GLOBAL int PHY_VirtualBlockErase(unsigned int nDieNum, unsigned int nBlkNum);
GLOBAL int PHY_VirtualBadBlockCheck(unsigned int nDieNum, unsigned int nBlkNum);
GLOBAL int PHY_VirtualBadBlockMark(unsigned int nDieNum, unsigned int nBlkNum);

GLOBAL __u32 NAND_GetLsbblksize(void);
GLOBAL __u32 NAND_GetLsbPages(void);
GLOBAL __u32 NAND_GetPhyblksize(void);
GLOBAL __u32 NAND_UsedLsbPages(void);
GLOBAL u32 NAND_GetPageNo(u32 lsb_page_no);
GLOBAL __u32 NAND_GetPageCntPerBlk(void);
GLOBAL __u32 NAND_GetPageSize(void);

//=============================================================================
#undef GLOBAL
#ifdef _UBOOTT_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int nand_write_nboot_data(unsigned char *buf, unsigned int len);
GLOBAL int nand_read_nboot_data(unsigned char *buf, unsigned int len);
GLOBAL int nand_write_uboot_data(unsigned char *buf, unsigned int len);
GLOBAL int nand_read_uboot_data(unsigned char *buf, unsigned int len);
GLOBAL int nand_check_nboot(unsigned char *buf, unsigned int len);
GLOBAL int nand_check_uboot(unsigned char *buf, unsigned int len);
GLOBAL int nand_get_param(void *nand_param);
GLOBAL int NAND_UpdatePhyArch(void);
GLOBAL int nand_uboot_erase_all_chip(unsigned int force_flag);
GLOBAL int nand_erase_chip(unsigned int chip, unsigned int start_block,
			   unsigned int end_block, unsigned int force_flag);
GLOBAL int nand_dragonborad_test_one(unsigned char *buf, unsigned char *oob,
				     unsigned int blk_num);
GLOBAL void print_uboot_info(struct _uboot_info *uboot);
GLOBAL int uboot_info_init(struct _uboot_info *uboot);

GLOBAL int nand_get_uboot_total_len(void);
GLOBAL int nand_get_nboot_total_len(void);
GLOBAL int init_phyinfo_buf(void);
GLOBAL int get_uboot_offset(void *buf);
GLOBAL int is_uboot_magic(void *buf);
GLOBAL int check_magic_physic_info(unsigned int *mem_base, char *magic);
GLOBAL int check_sum_physic_info(unsigned int *mem_base, unsigned int size);
GLOBAL int physic_info_get_offset(unsigned int *pages_offset);
GLOBAL int physic_info_get_one_copy(unsigned int start_block,
				    unsigned int pages_offset,
				    unsigned int *block_per_copy,
				    unsigned int *buf);
GLOBAL int clean_physic_info(void);
GLOBAL int physic_info_read(void);
GLOBAL int physic_info_add_to_uboot_tail(unsigned int *buf_dst,
					 unsigned int uboot_size);
GLOBAL int is_phyinfo_empty(struct _boot_info *info);
GLOBAL int add_len_to_uboot_tail(unsigned int uboot_size);
GLOBAL int print_physic_info(struct _boot_info *info);
GLOBAL unsigned int cal_sum_physic_info(struct _boot_info *mem_base,
					unsigned int size);
GLOBAL int change_uboot_start_block(struct _boot_info *info,
				    unsigned int start_block);
GLOBAL int set_uboot_start_and_end_block(void);
GLOBAL int get_uboot_start_block(void);
GLOBAL int get_uboot_next_block(void);
GLOBAL int nand_get_param_for_uboottail(void *nand_param);
GLOBAL int get_physic_block_reserved(void);
GLOBAL int rawnand_erase_chip(unsigned int chip, unsigned int start_block,
			      unsigned int end_block, unsigned int force_flag);
GLOBAL int rawnand_erase_chip(unsigned int chip, unsigned int start_block,
			      unsigned int end_block, unsigned int force_flag);
GLOBAL int spinand_erase_chip(unsigned int chip, unsigned int start_block,
			      unsigned int end_block, unsigned int force_flag);
GLOBAL void rawnand_erase_special_block(void);
GLOBAL void spinand_erase_special_block(void);
GLOBAL int is_physic_info_enable_crc(void);
//=============================================================================
#undef GLOBAL
#ifdef _SECURE_STORAGE_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif
GLOBAL int nand_secure_storage_block;
GLOBAL int nand_secure_storage_block_bak;

GLOBAL int nand_secure_storage_read(int item, unsigned char *buf,
				    unsigned int len);
GLOBAL int nand_secure_storage_write(int item, unsigned char *buf,
				     unsigned int len);

GLOBAL int nand_secure_storage_init(int flag);
GLOBAL int nand_secure_storage_first_build(unsigned int start_block);
GLOBAL int get_nand_secure_storage_max_item(void);
GLOBAL int is_nand_secure_storage_build(void);

GLOBAL int nand_secure_storage_write_init(unsigned int block);
GLOBAL unsigned int nand_secure_check_sum(unsigned char *mbuf,
					  unsigned int len);
GLOBAL int nand_secure_storage_read_one(unsigned int block, int item,
					unsigned char *mbuf, unsigned int len);
GLOBAL int nand_secure_storage_check(void);
GLOBAL int nand_secure_storage_update(void);
GLOBAL int nand_secure_storage_repair(int flag);

#endif /*__NAND_PHYSIC_INCLUDE_H__*/

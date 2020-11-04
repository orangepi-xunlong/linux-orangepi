
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

#ifndef UBOOT_HEAD
#define UBOOT_HEAD

#define TOC_MAIN_INFO_MAGIC 0x89119800
#define PHY_INFO_SIZE 0x8000
#define PHY_INFO_MAGIC 0xaa55a5a5
#define UBOOT_STAMP_VALUE 0xAE15BC34

#define TOC_LEN_OFFSET_BY_INT 9
#define TOC_MAGIC_OFFSET_BY_INT 4

#define NAND_BOOT0_BLK_START 0
#define NAND_BOOT0_BLK_CNT 2
#define NAND_UBOOT_BLK_START (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT)

#define NAND_UBOOT_BLK_CNT 6
#define NAND_BOOT0_PAGE_CNT_PER_COPY 64
#define NAND_BOOT0_PAGE_CNT_PER_COPY_2 128
#define NAND_BOOT0_PAGE_CNT_PER_COPY_4 256 // A50 boot0 size >= 128K
#define BOOT0_MAX_COPY_CNT (8)

#define UBOOT_SCAN_START_BLOCK 4

#define UBOOT_START_BLOCK_BIGNAND 4
#define UBOOT_START_BLOCK_SMALLNAND 8
#define UBOOT_MAX_BLOCK_NUM 50

#define PHYSIC_RECV_BLOCK 6

struct _uboot_info {
	unsigned int sys_mode;
	unsigned int use_lsb_page;
	unsigned int copys;

	unsigned int uboot_len;
	unsigned int total_len;
	unsigned int uboot_pages;
	unsigned int total_pages;

	unsigned int blocks_per_total;
	unsigned int page_offset_for_nand_info;
	unsigned int byte_offset_for_nand_info;
	unsigned char uboot_block[120];

	unsigned int nboot_copys;
	unsigned int nboot_len;
	unsigned int nboot_data_per_page; // 43

	unsigned int nouse[64 - 43];
};

#endif

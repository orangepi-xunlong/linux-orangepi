/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef  _SUNXI_NAND_H
#define  _SUNXI_NAND_H
#include <linux/types.h>

#define NOSPACE 0xFFFFFFFF
#define MAX_BLOCKS (16384)
#define BITS_MAX_BLOCKS (MAX_BLOCKS >> 3)


struct erase_status {
	/*whole chip don't care secure storage and phy storage area*/
	int erase_whole_chip;
	/*start_block=chip*block_per_chip*/
	int start_block;
	/*end_block=chip*block_per_chip*/
	int end_block;
	/*when erase chip, maybe use nftl_erase_zone when the chip is not blank,
	 * when erase chip use physic erase, the flag phy_erase is true*/
	int phy_erase;
#define SINGLE_ERASE (1)
#define MULTI_ERASE  (2)
	int erase_method;

};

/*
 *enum size_type {
 *        BYTE,
 *        SECTOR,
 *        PAGE,
 *        BLOCK
 *};
 */

struct secure_status {
#define SECURE_STORAGE_FIRST_BUILD (1)
	/*sector storage state*/
	int state;
};

struct boot_info_status {
#define BOOT_INFO_OK (1)
#define BOOT_INFO_NOEXIST (2)
	int state;
};

struct kernel_status {
#define KERNEL_IN_BOOT (0)
#define KERNEL_IN_PRODUCT (1)
	int state;
	struct secure_status nand_secstate;
	struct boot_info_status nand_bootinfo_state;
	struct erase_status nand_erase_state;
};
extern struct uboot_status ubootsta;

struct _nand_partition_page {
	unsigned short Page_NO;
	unsigned short Block_NO;
};

struct _physic_par {
	struct _nand_partition_page phy_page;
	unsigned short page_bitmap;
	unsigned char *main_data_addr;
	unsigned char *spare_data_addr;
};

struct _nand_super_block {
	unsigned short Block_NO;
	unsigned short Chip_NO;
};

#endif


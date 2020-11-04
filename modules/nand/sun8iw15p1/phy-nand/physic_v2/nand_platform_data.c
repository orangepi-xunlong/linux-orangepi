
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


#define _NPD_C_

#include "nand_physic_inc.h"

struct _nand_lib_cfg *g_phy_cfg;

struct _nand_storage_info *g_nsi;
struct _nand_storage_info g_nsi_data = {0};

struct _nand_super_storage_info *g_nssi;
struct _nand_super_storage_info g_nssi_data = {0};

struct _nand_controller_info *g_nctri;
struct _nand_controller_info g_nctri_data[2] = {0};

struct __RAWNandStorageInfo_t *g_nand_storage_info;
struct __RAWNandStorageInfo_t g_nand_storage_info_data = {0};

int (*nand_physic_special_init)(void) = NULL;
int (*nand_physic_special_exit)(void) = NULL;

struct _nand_chip_info nci_data[MAX_CHIP_PER_CHANNEL * 2] = {0};
struct _nand_super_chip_info nsci_data[MAX_CHIP_PER_CHANNEL * 2] = {0};

struct _nand_info aw_nand_info = {0};

struct _uboot_info uboot_info = {0};

struct _boot_info *phyinfo_buf;

int nand_physic_temp1;
int nand_physic_temp2;
int nand_physic_temp3;
int nand_physic_temp4;
int nand_physic_temp5;
int nand_physic_temp6;
int nand_physic_temp7;
int nand_physic_temp8;

struct _nand_temp_buf ntf = {0};

struct _nand_permanent_data nand_permanent_data = {
    MAGIC_DATA_FOR_PERMANENT_DATA, 0,
};

struct _nand_physic_function nand_physic_function[] = {
    {
	// driver number:0
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m0_special_init, m0_special_exit, m0_is_lsb_page,
    },
    {
	// driver number:1 hynix16nm 8G
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m8_write_boot0_one,

	m1_special_init, m1_special_exit, m1_is_lsb_page,
    },
    {
	// driver number:2  hynix20nm
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m2_write_boot0_one,

	m2_special_init, m2_special_exit, m2_is_lsb_page,
    },
    {
	// driver number:3 hynix26nm
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m3_write_boot0_one,

	m3_special_init, m3_special_exit, m3_is_lsb_page,
    },
    {
	// driver number:4 lsb page type0x40
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m4_special_init, m4_special_exit, m4_0x40_is_lsb_page,
    },

    {
	// driver number:5 sansumg retry
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m5_special_init, m5_special_exit, m5_is_lsb_page,
    },
    {
	// driver number:6 toshiba
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m6_special_init, m6_special_exit, m6_is_lsb_page,
    },
    {
	// driver number:7 sandisk 19nm
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m7_special_init, m7_special_exit, m7_is_lsb_page,
    },
    {
	// driver number:8    hynix 16nm 4G
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m8_write_boot0_one,

	m1_special_init, m1_special_exit, m1_is_lsb_page,
    }, // driver number:9   sandisk A19 4G/8G
    {
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m9_special_init, m9_special_exit, m7_is_lsb_page,
    },
    {
	// driver number:10 micron lsb page type 0x41
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m4_special_init, m4_special_exit, m4_0x41_is_lsb_page,
    },
    {
	// driver number:11 micron lsb page type 0x42
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m4_special_init, m4_special_exit, m4_0x42_is_lsb_page,
    },
    {
	// driver number:12 micron/intel without lsb page type
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m4_special_init, m4_special_exit, m0_is_lsb_page,
    },
    {
	// driver number:13 sandisk 24nm
	m0_erase_block, m0_read_page, m0_write_page, m0_bad_block_check,
	m0_bad_block_mark,

	m0_erase_super_block, m0_read_super_page, m0_write_super_page,
	m0_super_bad_block_check, m0_super_bad_block_mark,

	m8_read_boot0_page, m8_write_boot0_page,

	m8_read_boot0_one, m0_write_boot0_one,

	m7_special_init, m7_special_exit, m0_is_lsb_page,
    },
};

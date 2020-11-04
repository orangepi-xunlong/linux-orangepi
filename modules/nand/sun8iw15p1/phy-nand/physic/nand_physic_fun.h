
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

#ifndef __NAND_PHYSIC_FUN_H__
#define __NAND_PHYSIC_FUN_H__

extern __s32 m0_spi_nand_reset(__u32 spi_no, __u32 chip);
extern __s32 m0_spi_nand_read_status(__u32 spi_no, __u32 chip, __u8 status,
				     __u32 mode);
extern __s32 m0_spi_nand_setstatus(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m0_spi_nand_getblocklock(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m0_spi_nand_setblocklock(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m0_spi_nand_getotp(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m0_spi_nand_setotp(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m0_spi_nand_getoutdriver(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m0_spi_nand_setoutdriver(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m0_erase_single_block(struct boot_physical_param *eraseop);
extern __s32 m0_write_single_page(struct boot_physical_param *writeop);
extern __s32 m0_read_single_page(struct boot_physical_param *readop,
				 __u32 spare_only_flag);

extern __s32 m1_spi_nand_reset(__u32 spi_no, __u32 chip);
extern __s32 m1_spi_nand_read_status(__u32 spi_no, __u32 chip, __u8 status,
				     __u32 mode);
extern __s32 m1_spi_nand_setstatus(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m1_spi_nand_getblocklock(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m1_spi_nand_setblocklock(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m1_spi_nand_getotp(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m1_spi_nand_setotp(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m1_spi_nand_getoutdriver(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m1_spi_nand_setoutdriver(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m1_erase_single_block(struct boot_physical_param *eraseop);
extern __s32 m1_write_single_page(struct boot_physical_param *writeop);
extern __s32 m1_read_single_page(struct boot_physical_param *readop,
				 __u32 spare_only_flag);

extern __s32 m2_spi_nand_reset(__u32 spi_no, __u32 chip);
extern __s32 m2_spi_nand_read_status(__u32 spi_no, __u32 chip, __u8 status,
				     __u32 mode);
extern __s32 m2_spi_nand_setstatus(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m2_spi_nand_getblocklock(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m2_spi_nand_setblocklock(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m2_spi_nand_getotp(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m2_spi_nand_setotp(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m2_spi_nand_getoutdriver(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m2_spi_nand_setoutdriver(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m2_erase_single_block(struct boot_physical_param *eraseop);
extern __s32 m2_write_single_page(struct boot_physical_param *writeop);
extern __s32 m2_read_single_page(struct boot_physical_param *readop,
				 __u32 spare_only_flag);

extern __s32 m3_spi_nand_reset(__u32 spi_no, __u32 chip);
extern __s32 m3_spi_nand_read_status(__u32 spi_no, __u32 chip, __u8 status,
				     __u32 mode);
extern __s32 m3_spi_nand_setstatus(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m3_spi_nand_getblocklock(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m3_spi_nand_setblocklock(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m3_spi_nand_getotp(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m3_spi_nand_setotp(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m3_spi_nand_getoutdriver(__u32 spi_no, __u32 chip, __u8 *reg);
extern __s32 m3_spi_nand_setoutdriver(__u32 spi_no, __u32 chip, __u8 reg);
extern __s32 m3_erase_single_block(struct boot_physical_param *eraseop);
extern __s32 m3_write_single_page(struct boot_physical_param *writeop);
extern __s32 m3_read_single_page(struct boot_physical_param *readop,
				 __u32 spare_only_flag);

#endif

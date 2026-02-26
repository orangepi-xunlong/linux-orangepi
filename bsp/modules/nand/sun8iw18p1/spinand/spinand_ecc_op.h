/*
 * spinand_ecc_op.h for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __NAND_ECC_OP_H
#define __NAND_ECC_OP_H
#if 0//#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
#include <mach/sunxi_spinand.h>
#else
#include "sunxi_spinand.h"
#endif
/**
 * name format:
 * 1. EccBitCount
 * 2. LimitValue
 * 3. ErrValue
 *
 * [example]
 * BIT3: 3 bit ecc status
 * LIMIT2: value 2 (010b) up to limit
 * ERR7: value 7 (111b) up to error
 */
#define ECC_TYPE_BITMAP (0x0000FFFF)
#define HAS_EXT_ECC_STATUS (1 << 16)
#define HAS_EXT_ECC_SE01 (1 << 17)

enum ecc_type {
	ECC_TYPE_ERR = 0,
	BIT3_LIMIT3_TO_6_ERR7,
	BIT2_LIMIT1_ERR2,
	BIT2_LIMIT1_ERR2_LIMIT3,
	BIT4_LIMIT3_TO_4_ERR15,
	BIT3_LIMIT3_TO_4_ERR7,
	BIT3_LIMIT5_ERR2,
	BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
	BIT4_LIMIT5_TO_7_ERR8,
	BIT2_LIMIT3_ERR2,
	BIT3_LIMIT3_LIMIT_5_ERR2,
	BIT4_LIMIT5_TO_8_ERR15,
	BIT3_LIMIT2_TO_6_ERR7,
};
extern __s32 spinand_check_ecc(enum ecc_type type, __u8 status);

/**
 * name format:
 * 1. SIZE: size in bytes of each spare area
 * 2. OFF: offset in bytes without ecc protected
 * 3. LEN: ecc protected length in bytes
 *
 * [example] - MX35LF1GE4AB
 * SIZE16: each size of spare area is 16 bytes
 * OFF4: 4 bytes length without ecc protected
 * LEN12: 12 bytes under ecc protected
 */
enum ecc_protected_type {
	ECC_PROTECTED_TYPE = 0,
	SIZE16_OFF0_LEN16, /* all spare data are under ecc protection */
	SIZE16_OFF4_LEN12,
	SIZE16_OFF4_LEN4_OFF8,
	SIZE16_OFF4_LEN8_OFF4, /*compatible with GD5F1GQ4UBYIG@R6*/
	SIZE64_OFF32_LEN32,
	SIZE64_OFF8_LEN40_OFF16,
	SIZE256_OFF64_LEN64,
	SIZE16_OFF8_LEN16,
};
extern __s32 spinand_copy_to_spare(enum ecc_protected_type type, __u8 *to,
		__u8 *from, __u32 cnt);
extern __s32 spinand_copy_from_spare(enum ecc_protected_type type, __u8 *to,
		__u8 *from, __u32 cnt);

extern __u32 spinand_get_spare_size(__u32 type);

#endif

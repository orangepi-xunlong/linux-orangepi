// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

//#define pr_fmt(fmt) "sunxi-spinand-phy: " fmt
#define SUNXI_MODNAME "sunxi-spinand-phy"
#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/aw-spinand.h>
#include <linux/of.h>

#include "physic.h"

#define KB (1024)
#define MB (KB * 1024)
#define to_kb(size) (size / KB)
#define to_mb(size) (size / MB)


/* manufacture num */
#define MICRON_MANUFACTURE	0x2c
#define GD_MANUFACTURE		0xc8
#define ATO_MANUFACTURE		0x9b
#define WINBOND_MANUFACTURE	0xef
#define MXIC_MANUFACTURE	0xc2
#define TOSHIBA_MANUFACTURE	0x98
#define ETRON_MANUFACTURE	0xd5
#define XTXTECH_MANUFACTURE	0x0b
#define DSTECH_MANUFACTURE	0xe5
#define FORESEE_MANUFACTURE	0xcd
#define ZETTA_MANUFACTURE	0xba
#define FM_MANUFACTURE		0xa1
#define HEYANGTEK_MANUFACTURE	0xc9

struct spinand_manufacture m;

struct aw_spinand_phy_info gigadevice[] = {
	{
		.Model		= "GD5F1GQ4UCYIG",
		.NandID		= {0xc8, 0xb1, 0x48, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ | SPINAND_ONEDUMMY_AFTER_RANDOMREAD,
		.MaxEraseTimes  = 50000,
		.EccType	= BIT3_LIMIT2_TO_6_ERR7,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "GD5F1GQ4UBYIG",
		.NandID		= {0xc8, 0xd1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccFlag	= HAS_EXT_ECC_SE01,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF4_LEN8_OFF4,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		/* GD5F2GQ4UB9IG did not check yet */
		.Model		= "GD5F2GQ4UB9IG",
		.NandID		= {0xc8, 0xd2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccFlag	= HAS_EXT_ECC_SE01,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF4_LEN12,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model    = "GD5F1GQ5UEYIG",
		.NandID    = {0xc8, 0x51, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie  = 1024,
		.OobSizePerPage = 64,
		.OperationOpt  = SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccFlag  = HAS_EXT_ECC_SE01,
		.EccType  = BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF4_LEN12,
		.BadBlockFlag  = BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "F50L1G41LB(2M)",
		.NandID		= {0xc8, 0x01, 0x7f, 0x7f, 0x7f, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ | SPINAND_QUAD_NO_NEED_ENABLE,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag	= BAD_BLK_FLAG_FIRST_2_PAGE,
	},
	{
		.Model		= "GD5F2GM7UEYI",
		.NandID		= {0xc8, 0x92, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccFlag	= HAS_EXT_ECC_SE01,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "GD5F2GQ5UEYIGR",
		.NandID		= {0xc8, 0x52, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccFlag	= HAS_EXT_ECC_SE01,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF4_LEN12,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
};

struct aw_spinand_phy_info micron[] = {
	{
		.Model		= "MT29F1G01ABAGDWB",
		.NandID		= {0x2c, 0x14, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ | SPINAND_QUAD_NO_NEED_ENABLE,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT3_LIMIT5_ERR2,
		.EccProtectedType = SIZE16_OFF32_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "MT29F2G01ABAGDWB",
		.NandID		= {0x2c, 0x24, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ | SPINAND_QUAD_NO_NEED_ENABLE |
			SPINAND_TWO_PLANE_SELECT,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT3_LIMIT5_ERR2,
		.EccProtectedType = SIZE16_OFF32_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
};

struct aw_spinand_phy_info xtx[] = {
	{
		/* XTX26G02A */
		.Model		= "XTX26G02A",
		.NandID		= {0x0B, 0xE2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.ecc_status_shift = ECC_STATUS_SHIFT_2,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF8_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		/* XTX26G02A */
		.Model		= "XTX26G01A",
		.NandID		= {0x0B, 0xE1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.ecc_status_shift = ECC_STATUS_SHIFT_2,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF8_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		/* XT26G01C */
		.Model		= "XT26G01C",
		.NandID		= {0x0B, 0x11, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.ecc_status_shift = ECC_STATUS_SHIFT_4,
		.EccType	= BIT4_LIMIT5_TO_8_ERR9_TO_15,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		/* XT26G02C */
		.Model		= "XT26G02CWSIG",
		.NandID		= {0x0B, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_DUAL_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.ecc_status_shift = ECC_STATUS_SHIFT_2,
		.EccType	= BIT4_LIMIT5_TO_7_ERR8_LIMIT_12,
		.EccProtectedType = SIZE16_OFF8_LEN16,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		/* XT26G04C */
		.Model		= "XT26G04CWSIG",
		.NandID		= {0x0B, 0x13, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 8,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.ecc_status_shift = ECC_STATUS_SHIFT_4,
		.EccType	= BIT4_LIMIT5_TO_8_ERR9_TO_15,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag	=  BAD_BLK_FLAG_FRIST_1_PAGE,
	},

};

struct aw_spinand_phy_info fm[] = {
	{
		/* only rw stress test */
		.Model		= "FM25S01",
		.NandID		= {0xa1, 0xa1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ | SPINAND_QUAD_NO_NEED_ENABLE,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag = BAD_BLK_FLAG_FIRST_2_PAGE,
	},
	{
		.Model		= "FM25S01A",
		.NandID		= {0xa1, 0xe4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag = BAD_BLK_FLAG_FIRST_2_PAGE,
	},
};

struct aw_spinand_phy_info etron[] = {

};

struct aw_spinand_phy_info toshiba[] = {

};

struct aw_spinand_phy_info ato[] = {

};

struct aw_spinand_phy_info mxic[] = {
	{
		.Model		= "MX35LF1GE4AB",
		.NandID		= {0xc2, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 65000,
		.EccFlag	= HAS_EXT_ECC_STATUS,
		.EccType	= BIT4_LIMIT3_TO_4_ERR15,
		/**
		 * MX35LF1GE4AB should use SIZE16_OFF4_LEN12, however, in order
		 * to compatibility with versions already sent to customers,
		 * which do not use general physical layout, we used
		 * SIZE16_OFF4_LEN4_OFF8 instead.
		 */
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FIRST_2_PAGE,
	},
	{
		.Model		= "MX35LF2GE4AD",
		.NandID		= {0xc2, 0x26, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 65000,
		.EccFlag	= HAS_EXT_ECC_STATUS,
		.EccType	= BIT4_LIMIT5_TO_8_ERR9_TO_15,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FIRST_2_PAGE,
	},
};

struct aw_spinand_phy_info winbond[] = {
	{
		.Model		= "W25N01GVZEIG",
		.NandID		= {0xef, 0xaa, 0x21, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "W25N02KV",
		.NandID		= {0xef, 0xaa, 0x22, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT2_ERR2_LIMIT3,
		.EccProtectedType = SIZE16_OFF4_LEN12,
		.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "W25M02GV",
		.NandID 	= {0xef, 0xab, 0x21, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip	= 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk	= 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_DUAL_READ |
			SPINAND_QUAD_PROGRAM,
		.MaxEraseTimes	= 65000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN12,
		.BadBlockFlag	= BAD_BLK_FLAG_FRIST_1_PAGE,
	},
};

struct aw_spinand_phy_info dosilicon[] = {
	{
		.Model		= "DS35X1GAXXX",
		.NandID		= {0xe5, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 65000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FIRST_2_PAGE,
	},
};

struct aw_spinand_phy_info foresee[] = {
	{
		.Model		= "FS35ND01G-S1F1QWFI000",
		.NandID		= {0xcd, 0xb1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccType	= BIT3_LIMIT3_TO_4_ERR7,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "FS35ND01G-S1Y2QWFI000",
		.NandID		= {0xcd, 0xea, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE,
	},
	{
		.Model		= "FS35SQA001G",
		.NandID		= {0xcd, 0x71, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF0_LEN16,
		.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE,
	},

};

struct aw_spinand_phy_info zetta[] = {
	{
		.Model		= "ZD35Q1GAIB",
		.NandID		= {0xba, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 4,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 1024,
		.OobSizePerPage = 64,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM |
			SPINAND_DUAL_READ,
		.MaxEraseTimes  = 50000,
		.EccType	= BIT2_LIMIT1_ERR2,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FIRST_2_PAGE,
	},
};

struct aw_spinand_phy_info heyangtek[] = {
	{
		.Model		= "HYF4GQ4UDACAE",
		.NandID		= {0xc9, 0xd4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		.DieCntPerChip  = 1,
		.SectCntPerPage = 8,
		.PageCntPerBlk  = 64,
		.BlkCntPerDie	= 2048,
		.OobSizePerPage = 256,
		.OperationOpt	= SPINAND_QUAD_READ | SPINAND_QUAD_PROGRAM | SPINAND_DUAL_READ,
		.MaxEraseTimes  = 100000,
		.EccType	= BIT2_LIMIT1_ERR2_LIMIT3,
		.ecc_status_shift = ECC_STATUS_SHIFT_4,
		.EccProtectedType = SIZE16_OFF4_LEN4_OFF8,
		.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE,
	},
};

static const char *aw_spinand_info_model(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->Model;
}

static void aw_spinand_info_nandid(struct aw_spinand_chip *chip,
		unsigned char *id, int cnt)
{
	int i;
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	cnt = min(cnt, MAX_ID_LEN);
	for (i = 0; i < cnt; i++)
		id[i] = pinfo->NandID[i];
}

static unsigned int aw_spinand_info_sector_size(struct aw_spinand_chip *chip)
{
	return 1 << SECTOR_SHIFT;
}

static unsigned int aw_spinand_info_phy_page_size(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->SectCntPerPage * aw_spinand_info_sector_size(chip);
}

static unsigned int aw_spinand_info_page_size(struct aw_spinand_chip *chip)
{
#if IS_ENABLED(CONFIG_AW_SPINAND_SIMULATE_MULTIPLANE)
	return aw_spinand_info_phy_page_size(chip) * 2;
#else
	return aw_spinand_info_phy_page_size(chip);
#endif
}

static unsigned int aw_spinand_info_phy_block_size(
		struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->PageCntPerBlk * aw_spinand_info_phy_page_size(chip);
}

static unsigned int aw_spinand_info_block_size(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->PageCntPerBlk * aw_spinand_info_page_size(chip);
}

static unsigned int aw_spinand_info_phy_oob_size(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->OobSizePerPage;
}

static unsigned int aw_spinand_info_oob_size(struct aw_spinand_chip *chip)
{
#if IS_ENABLED(CONFIG_AW_SPINAND_SIMULATE_MULTIPLANE)
	return aw_spinand_info_phy_oob_size(chip) * 2;
#else
	return aw_spinand_info_phy_oob_size(chip);
#endif
}

static unsigned int aw_spinand_info_die_cnt(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->DieCntPerChip;
}

static unsigned int aw_spinand_info_total_size(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->DieCntPerChip * pinfo->BlkCntPerDie *
		aw_spinand_info_phy_block_size(chip);
}

static int aw_spinand_info_operation_opt(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->OperationOpt;
}

static int aw_spinand_info_max_erase_times(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;

	return pinfo->MaxEraseTimes;
}

struct spinand_manufacture {
	unsigned char id;
	const char *name;
	struct aw_spinand_phy_info *info;
	unsigned int cnt;
};

#define SPINAND_FACTORY_INFO(_id, _name, _info)			\
	{							\
		.id = _id,					\
		.name = _name,					\
		.info = _info,					\
		.cnt = ARRAY_SIZE(_info),			\
	}
static struct spinand_manufacture spinand_factory[] = {
	SPINAND_FACTORY_INFO(MICRON_MANUFACTURE, "Micron", micron),
	SPINAND_FACTORY_INFO(GD_MANUFACTURE, "GD", gigadevice),
	SPINAND_FACTORY_INFO(ATO_MANUFACTURE, "ATO", ato),
	SPINAND_FACTORY_INFO(WINBOND_MANUFACTURE, "Winbond", winbond),
	SPINAND_FACTORY_INFO(MXIC_MANUFACTURE, "Mxic", mxic),
	SPINAND_FACTORY_INFO(TOSHIBA_MANUFACTURE, "Toshiba", toshiba),
	SPINAND_FACTORY_INFO(ETRON_MANUFACTURE, "Etron", etron),
	SPINAND_FACTORY_INFO(XTXTECH_MANUFACTURE, "XTX", xtx),
	SPINAND_FACTORY_INFO(DSTECH_MANUFACTURE, "Dosilicon", dosilicon),
	SPINAND_FACTORY_INFO(FORESEE_MANUFACTURE, "Foresee", foresee),
	SPINAND_FACTORY_INFO(ZETTA_MANUFACTURE, "Zetta", zetta),
	SPINAND_FACTORY_INFO(FM_MANUFACTURE, "FM", fm),
	SPINAND_FACTORY_INFO(HEYANGTEK_MANUFACTURE, "HeYangTek", heyangtek),
};


static int spinand_get_chip_munufacture(struct aw_spinand_chip *chip, const char **m)
{
	struct aw_spinand_phy_info *info = chip->info->phy_info;

	switch (info->NandID[0]) {
	case MICRON_MANUFACTURE:
		*m = "Micron";
	break;
	case GD_MANUFACTURE:
		*m = "GD";
	break;
	case ATO_MANUFACTURE:
		*m = "ATO";
	break;
	case WINBOND_MANUFACTURE:
		*m = "Winbond";
	break;
	case MXIC_MANUFACTURE:
		*m = "Mxic";
	break;
	case TOSHIBA_MANUFACTURE:
		*m = "Toshiba";
	break;
	case ETRON_MANUFACTURE:
		*m = "Etron";
	break;
	case XTXTECH_MANUFACTURE:
		*m = "XTX";
	break;
	case DSTECH_MANUFACTURE:
		*m = "Dosilicon";
	break;
	case FORESEE_MANUFACTURE:
		*m = "Foresee";
	break;
	case ZETTA_MANUFACTURE:
		*m = "Zetta";
	break;
	case HEYANGTEK_MANUFACTURE:
		*m = "HeYangTek";
	break;
	default:
		*m = NULL;
	break;
	}

	if (*m == NULL)
		return false;
	else
		return true;

}
static const char *aw_spinand_info_manufacture(struct aw_spinand_chip *chip)
{
	int i, j;
	struct spinand_manufacture *m;
	struct aw_spinand_phy_info *pinfo;
	const char *m_name = NULL;
	int ret = 0;

	for (i = 0; i < ARRAY_SIZE(spinand_factory); i++) {
		m = &spinand_factory[i];
		pinfo = chip->info->phy_info;
		for (j = 0; j < m->cnt; j++)
			if (pinfo == &m->info[j])
				return m->name;
	}

	/*for compatible fdt support spi-nand*/
	ret = spinand_get_chip_munufacture(chip, &m_name);
	if (ret < 0)
		return NULL;
	else
		return m_name;
}

static struct spinand_manufacture *spinand_detect_munufacture(unsigned char id)
{
	int index;
	struct spinand_manufacture *m;

	for (index = 0; index < ARRAY_SIZE(spinand_factory); index++) {
		m = &spinand_factory[index];
		if (m->id == id) {
			sunxi_info(NULL, "detect munufacture from id table: %s\n", m->name);
			return m;
		}
	}

	sunxi_err(NULL, "not detect any munufacture from id table\n");
	return NULL;
}

static struct aw_spinand_phy_info *spinand_match_id(
		struct spinand_manufacture *m,
		unsigned char *id)
{
	int i, j, match_max = 1, match_index = 0;
	struct aw_spinand_phy_info *pinfo;

	for (i = 0; i < m->cnt; i++) {
		int match = 1;

		pinfo = &m->info[i];
		for (j = 1; j < MAX_ID_LEN; j++) {
			/* 0xFF matching all ID value */
			if (pinfo->NandID[j] != id[j] &&
					pinfo->NandID[j] != 0xFF)
				break;

			if (pinfo->NandID[j] != 0xFF)
				match++;
		}

		if (match > match_max) {
			match_max = match;
			match_index = i;
		}
	}

	if (match_max > 1)
		return &m->info[match_index];
	return NULL;
}

struct aw_spinand_phy_info *spinand_get_phy_info_from_fdt(struct aw_spinand_chip *chip)
{
	static struct aw_spinand_phy_info info;
	static int had_get;
	int ret = 0;
	const char *bad_blk_mark_pos = NULL;
	const char *quad_read_not_need_enable = NULL;
	const char *read_seq_need_onedummy = NULL;
	int len = 0;
	u32 id = 0xffffffff;
	struct device_node *node = chip->spi->dev.of_node;
	u32 rx_bus_width = 0;
	u32 tx_bus_width = 0;


	if (had_get == true)
		return &info;

#define BAD_BLK_MARK_POS1 "first_1_page"
#define BAD_BLK_MARK_POS2 "first_2_page"
#define BAD_BLK_MARK_POS3 "last_1_page"
#define BAD_BLK_MARK_POS4 "last_2_page"
	memset(&info, 0, sizeof(struct aw_spinand_phy_info));

	ret = of_property_read_string(node, "model", &(info.Model));
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand Model from fdt fail\n");
		goto err;
	}

	ret = of_property_read_u32(node, "id-0", &id);
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand id Low 4 Byte from fdt fail\n");
		goto err;
	}
	len = sizeof(id);
	memmove(info.NandID, &id, min(MAX_ID_LEN, len));

	id = 0xffffffff;
	ret = of_property_read_u32(node, "id-1", &id);
	if (ret < 0) {
		sunxi_info(NULL, "can't get spi-nand id high 4 Byte from fdt, may be not need\n");
	}
	memmove(info.NandID + min(MAX_ID_LEN, len), &id, max(MAX_ID_LEN, len) - min(MAX_ID_LEN, len));

	ret = of_property_read_u32(node, "die_cnt_per_chip", &(info.DieCntPerChip));
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand DieCntPerChip from fdt fail\n");
		goto err;
	}

	ret = of_property_read_u32(node, "blk_cnt_per_die", &(info.BlkCntPerDie));
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand BlkCntPerDie from fdt fail\n");
		goto err;
	}

	ret = of_property_read_u32(node, "page_cnt_per_blk", &(info.PageCntPerBlk));
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand PageCntPerBlk from fdt fail\n");
		goto err;
	}

	ret = of_property_read_u32(node, "sect_cnt_per_page", &(info.SectCntPerPage));
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand SectCntPerPage from fdt fail\n");
		goto err;
	}

	ret = of_property_read_u32(node, "oob_size_per_page", &(info.OobSizePerPage));
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand OobSizePerPage from fdt fail\n");
		goto err;
	}

	ret = of_property_read_string(node, "bad_block_mark_pos", &bad_blk_mark_pos);
	if (ret < 0 || NULL == bad_blk_mark_pos) {
		sunxi_err(NULL, "get spi-nand BadBlockFlag from fdt fail\n");
		goto err;
	} else {
		if (!memcmp(bad_blk_mark_pos, BAD_BLK_MARK_POS1, strlen(BAD_BLK_MARK_POS1)))
			info.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE;
		else if (!memcmp(bad_blk_mark_pos, BAD_BLK_MARK_POS2, strlen(BAD_BLK_MARK_POS2)))
			info.BadBlockFlag = BAD_BLK_FLAG_FRIST_1_PAGE;
		else if (!memcmp(bad_blk_mark_pos, BAD_BLK_MARK_POS3, strlen(BAD_BLK_MARK_POS3)))
			info.BadBlockFlag = BAD_BLK_FLAG_LAST_1_PAGE;
		else if (!memcmp(bad_blk_mark_pos, BAD_BLK_MARK_POS4, strlen(BAD_BLK_MARK_POS4)))
			info.BadBlockFlag = BAD_BLK_FLAG_LAST_2_PAGE;
		else {
			sunxi_err(NULL, "get spi-nand BadBlockFlag pattern is not right\n");
			goto err;
		}
	}

	ret = of_property_read_s32(node, "max_erase_times", &(info.MaxEraseTimes));
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand MaxEraseTimes from fdt fail\n");
		goto err;
	}

	ret = of_property_read_u32(node, "ecc_type", &(info.EccType));
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand EccFlag from fdt fail\n");
		goto err;
	}

	ret = of_property_read_u32(node, "ecc_protected_type", &(info.EccProtectedType));
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand ecc_protected_type from fdt fail\n");
		goto err;
	}

	ret = of_property_read_u32(node, "spi-rx-bus-width", &rx_bus_width);
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand spi-rx-bus-width from fdt fail\n");
		goto err;
	} else {
		switch (rx_bus_width) {
		case SPI_NBITS_DUAL:
			info.OperationOpt |= SPINAND_DUAL_READ;
		break;
		case SPI_NBITS_QUAD:
			info.OperationOpt |= SPINAND_QUAD_READ;
		break;
		default:
			info.OperationOpt |= 0;
		break;
		}
	}

	ret = of_property_read_u32(node, "spi-tx-bus-width", &tx_bus_width);
	if (ret < 0) {
		sunxi_err(NULL, "get spi-nand spi-tx-bus-width from fdt fail\n");
		goto err;
	} else {
		switch (tx_bus_width) {
		case SPI_NBITS_QUAD:
			info.OperationOpt |= SPINAND_QUAD_PROGRAM;
		break;
		default:
			info.OperationOpt |= 0;
		break;
		}
	}

	ret = of_property_read_string(node, "read_from_cache_x4_not_need_enable",
			&quad_read_not_need_enable);
	sunxi_info(NULL, "%d quad_read_not_need_enable:%s\n", __LINE__, quad_read_not_need_enable);
	if (ret < 0 || NULL == quad_read_not_need_enable) {
		sunxi_info(NULL, "can't get spi-nand read_from_cache_x4_need_enable or it is null,"
				" maybe not need enable quad read before read from cache x4\n");
	} else {
		if (!memcmp(quad_read_not_need_enable, "yes", strlen("yes")))
			info.OperationOpt |= SPINAND_QUAD_NO_NEED_ENABLE;
	}

	ret = of_property_read_string(node, "read_from_cache_need_onedummy",
			&read_seq_need_onedummy);
	if (ret < 0 || NULL == read_seq_need_onedummy) {
		sunxi_info(NULL, "can't get spi-nand read_from_cache_need_onedummy or it is null,"
				" maybe read from cache sequence not need one dummy in second Byte\n");
	} else {
		if (!memcmp(read_seq_need_onedummy, "yes", strlen("yes")))
			info.OperationOpt |= SPINAND_ONEDUMMY_AFTER_RANDOMREAD;
	}


	ret = of_property_read_s32(node, "ecc_flag", &(info.EccFlag));
	if (ret < 0) {
		sunxi_err(NULL, "can't get spi-nand EccFlag from fdt,"
				" maybe(default) use 0FH + C0H to get feature,wich obtain ecc status\n");
	}

	ret = of_property_read_u32(node, "ecc_status_shift", &(info.ecc_status_shift));
	if (ret < 0) {
		sunxi_info(NULL, "can't get spi-nand ecc_status_shift from fdt,"
				" use default ecc_status_shift_4 to get ecc status in C0H\n");
	}

	sunxi_debug(NULL, "get spinand phy info from fdt\n");
	sunxi_debug(NULL, "Model:%s\n", info.Model);
	sunxi_debug(NULL, "ID:%02x %02x %02x %02x %02x %02x %02x %02x\n",
			info.NandID[0], info.NandID[1], info.NandID[2], info.NandID[3],
			info.NandID[4], info.NandID[5], info.NandID[6], info.NandID[7]);
	sunxi_debug(NULL, "DieCntPerChip:%d\n", info.DieCntPerChip);
	sunxi_debug(NULL, "BlkCntPerDie:%d\n", info.BlkCntPerDie);
	sunxi_debug(NULL, "PageCntPerBlk:%d\n", info.PageCntPerBlk);
	sunxi_debug(NULL, "SectCntPerPage:%d\n", info.SectCntPerPage);
	sunxi_debug(NULL, "OobSizePerPage:%d\n", info.OobSizePerPage);
	sunxi_debug(NULL, "BadBlockFlag:%d\n", info.BadBlockFlag);
	sunxi_debug(NULL, "OperationOpt:0x%x\n", info.OperationOpt);
	sunxi_debug(NULL, "MaxEraseTimes:%d\n", info.MaxEraseTimes);
	sunxi_debug(NULL, "EccFlag:%x\n", info.EccFlag);
	sunxi_debug(NULL, "ecc_status_shift:%x\n", info.ecc_status_shift);
	sunxi_debug(NULL, "EccType:%x\n", info.EccType);
	sunxi_debug(NULL, "EccProtectedType:%x\n", info.EccProtectedType);

	had_get = true;

	return &info;
err:
	had_get = false;
	return NULL;
}

static struct aw_spinand_info aw_spinand_info = {
	.model = aw_spinand_info_model,
	.manufacture = aw_spinand_info_manufacture,
	.nandid = aw_spinand_info_nandid,
	.die_cnt = aw_spinand_info_die_cnt,
	.oob_size = aw_spinand_info_oob_size,
	.page_size = aw_spinand_info_page_size,
	.block_size = aw_spinand_info_block_size,
	.phy_oob_size = aw_spinand_info_phy_oob_size,
	.phy_page_size = aw_spinand_info_phy_page_size,
	.phy_block_size = aw_spinand_info_phy_block_size,
	.sector_size = aw_spinand_info_sector_size,
	.total_size = aw_spinand_info_total_size,
	.operation_opt = aw_spinand_info_operation_opt,
	.max_erase_times = aw_spinand_info_max_erase_times,
};

static struct spinand_manufacture *spinand_detect_munufacture_from_fdt(struct aw_spinand_chip *chip, unsigned char id)
{
	struct aw_spinand_phy_info *info = NULL;
	struct spinand_manufacture *pm = &m;
	int ret = 0;

	info = spinand_get_phy_info_from_fdt(chip);
	if (info == NULL) {
		sunxi_err(NULL, "get phy info from fdt fail\n");
		goto err;
	}

	if (id == info->NandID[0]) {
		pm->id = info->NandID[0];
		pm->info = info;
		chip->info = &aw_spinand_info;
		chip->info->phy_info = info;
		ret = spinand_get_chip_munufacture(chip, &(pm->name));
		if (ret < 0)
			goto err;
		else
			sunxi_info(NULL, "detect munufacture from fdt: %s \n", pm->name);
	} else {
		goto err;
	}

	return pm;
err:
	sunxi_info(NULL, "not detect munufacture from fdt\n");
	return NULL;
}

static struct aw_spinand_phy_info *spinand_match_id_from_fdt(struct aw_spinand_chip *chip,
		struct spinand_manufacture *m,
		unsigned char *id)
{
	struct aw_spinand_phy_info *info = NULL;
	int i = 0;

	info = spinand_get_phy_info_from_fdt(chip);
	if (info == NULL) {
		sunxi_err(NULL, "get phy info from fdt fail\n");
		goto err;
	}

	for (i = 0; i < MAX_ID_LEN; i++) {
		/*0xff match all id value*/
		if (id[i] != info->NandID[i] && info->NandID[i] != 0xff)
			goto err;
	}

	return info;

err:
	return NULL;
}
static int aw_spinand_info_init(struct aw_spinand_chip *chip,
		struct aw_spinand_phy_info *pinfo)
{
	chip->info = &aw_spinand_info;
	chip->info->phy_info = pinfo;

	sunxi_info(NULL, "========== arch info ==========\n");
	sunxi_info(NULL, "Model:               %s\n", pinfo->Model);
	sunxi_info(NULL, "Munufacture:         %s\n", aw_spinand_info_manufacture(chip));
	sunxi_info(NULL, "DieCntPerChip:       %u\n", pinfo->DieCntPerChip);
	sunxi_info(NULL, "BlkCntPerDie:        %u\n", pinfo->BlkCntPerDie);
	sunxi_info(NULL, "PageCntPerBlk:       %u\n", pinfo->PageCntPerBlk);
	sunxi_info(NULL, "SectCntPerPage:      %u\n", pinfo->SectCntPerPage);
	sunxi_info(NULL, "OobSizePerPage:      %u\n", pinfo->OobSizePerPage);
	sunxi_info(NULL, "BadBlockFlag:        0x%x\n", pinfo->BadBlockFlag);
	sunxi_info(NULL, "OperationOpt:        0x%x\n", pinfo->OperationOpt);
	sunxi_info(NULL, "MaxEraseTimes:       %d\n", pinfo->MaxEraseTimes);
	sunxi_info(NULL, "EccFlag:             0x%x\n", pinfo->EccFlag);
	sunxi_info(NULL, "EccType:             %d\n", pinfo->EccType);
	sunxi_info(NULL, "EccProtectedType:    %d\n", pinfo->EccProtectedType);
	sunxi_info(NULL, "========================================\n");
	sunxi_info(NULL, "\n");
	sunxi_info(NULL, "========== physical info ==========\n");
	sunxi_info(NULL, "TotalSize:    %u M\n", to_mb(aw_spinand_info_total_size(chip)));
	sunxi_info(NULL, "SectorSize:   %u B\n", aw_spinand_info_sector_size(chip));
	sunxi_info(NULL, "PageSize:     %u K\n", to_kb(aw_spinand_info_phy_page_size(chip)));
	sunxi_info(NULL, "BlockSize:    %u K\n", to_kb(aw_spinand_info_phy_block_size(chip)));
	sunxi_info(NULL, "OOBSize:      %u B\n", aw_spinand_info_phy_oob_size(chip));
	sunxi_info(NULL, "========================================\n");
	sunxi_info(NULL, "\n");
	sunxi_info(NULL, "========== logical info ==========\n");
	sunxi_info(NULL, "TotalSize:    %u M\n", to_mb(aw_spinand_info_total_size(chip)));
	sunxi_info(NULL, "SectorSize:   %u B\n", aw_spinand_info_sector_size(chip));
	sunxi_info(NULL, "PageSize:     %u K\n", to_kb(aw_spinand_info_page_size(chip)));
	sunxi_info(NULL, "BlockSize:    %u K\n", to_kb(aw_spinand_info_block_size(chip)));
	sunxi_info(NULL, "OOBSize:      %u B\n", aw_spinand_info_oob_size(chip));
	sunxi_info(NULL, "========================================\n");

	return 0;
}

int aw_spinand_chip_detect(struct aw_spinand_chip *chip)
{
	struct aw_spinand_phy_info *pinfo;
	struct spinand_manufacture *m;
	unsigned char id[MAX_ID_LEN] = {0xFF};
	struct aw_spinand_chip_ops *ops = chip->ops;
	int ret, dummy = 1;

retry:
	/*first read with dummy/address@0x00*/
	ret = ops->read_id(chip, id, MAX_ID_LEN, dummy);
	if (ret) {
		sunxi_err(NULL, "read id failed : %d\n", ret);
		return ret;
	}

	m = spinand_detect_munufacture(id[0]);
	if (!m)
		goto detect_from_fdt;

	pinfo = spinand_match_id(m, id);
	if (pinfo)
		goto detect;

detect_from_fdt:
		m = spinand_detect_munufacture_from_fdt(chip, id[0]);
		if (!m)
			goto not_detect;

		pinfo = spinand_match_id_from_fdt(chip, m, id);
		if (pinfo)
			goto detect;

not_detect:
	/* retry without dummy/address@0x00 */
	if (dummy) {
		dummy = 0;
		goto retry;
	}
	sunxi_info(NULL, "not match spinand: %x %x\n",
			*(__u32 *)id,
			*((__u32 *)id + 1));
	return -ENODEV;
detect:
	sunxi_info(NULL, "detect spinand id: %x %x\n",
			*((__u32 *)pinfo->NandID),
			*((__u32 *)pinfo->NandID + 1));
	return aw_spinand_info_init(chip, pinfo);
}

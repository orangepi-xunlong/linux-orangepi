/*
 * (C) Copyright 2007-2011
 * Allwinnertech Technology Co., Ltd. <www.allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "nand_scan.h"
#include "nand_physic_fun.h"
#include "nand_type_spinand.h"

struct spi_nand_function spi_nand_function0 = {
	m0_spi_nand_reset,	m0_spi_nand_read_status,
	m0_spi_nand_setstatus,    m0_spi_nand_getblocklock,
	m0_spi_nand_setblocklock, m0_spi_nand_getotp,
	m0_spi_nand_setotp,       m0_spi_nand_getoutdriver,
	m0_spi_nand_setoutdriver, m0_erase_single_block,
	m0_write_single_page,     m0_read_single_page,
};

struct spi_nand_function spi_nand_function1 = {
	m1_spi_nand_reset,	m1_spi_nand_read_status,
	m1_spi_nand_setstatus,    m1_spi_nand_getblocklock,
	m1_spi_nand_setblocklock, m1_spi_nand_getotp,
	m1_spi_nand_setotp,       m1_spi_nand_getoutdriver,
	m1_spi_nand_setoutdriver, m1_erase_single_block,
	m1_write_single_page,     m1_read_single_page,
};

struct spi_nand_function spi_nand_function2 = {
	m2_spi_nand_reset,	m2_spi_nand_read_status,
	m2_spi_nand_setstatus,    m2_spi_nand_getblocklock,
	m2_spi_nand_setblocklock, m2_spi_nand_getotp,
	m2_spi_nand_setotp,       m2_spi_nand_getoutdriver,
	m2_spi_nand_setoutdriver, m2_erase_single_block,
	m2_write_single_page,     m2_read_single_page,
};

struct spi_nand_function spi_nand_function3 = {
	m3_spi_nand_reset,	m3_spi_nand_read_status,
	m3_spi_nand_setstatus,    m3_spi_nand_getblocklock,
	m3_spi_nand_setblocklock, m3_spi_nand_getotp,
	m3_spi_nand_setotp,       m3_spi_nand_getoutdriver,
	m3_spi_nand_setoutdriver, m3_erase_single_block,
	m3_write_single_page,     m3_read_single_page,
};

//==============================================================================
// define the physical architecture parameter for all kinds of nand flash
//==============================================================================

//==============================================================================
//============================ GIGADEVICE & MIRA NAND FLASH
//==============================
//==============================================================================
struct __NandPhyInfoPar_t GigaDeviceNandTbl[] = {
    //                   NAND_CHIP_ID                 DieCnt SecCnt  PagCnt
    //                   BlkCnt    OpOpt     Freq     mode   pagewithbadflag
    //                   function		  offset	maxerasetime
    //                   maxecc	ecclimit	idnumber
    //-------------------------------------------------------------------------
    // no support because of no testing
    //{ {0xc8, 0xf1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,
    //1024,   0x007d,     100,    0, 	   0,             &spi_nand_function0 ,
    //1,		 50000,     	8,			8,
    //0x000000},//GD5F1GQ4UAYIG
    //{ {0xc8, 0xf2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,
    //2048,   0x047d,     12,    0, 	   0,             &spi_nand_function0 , 1,
    //50000,     	8,			8,
    //0x000001},//GD5F2GQ4UAYIG
    //{ {0xc8, 0xf4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,
    //4096,   0x007d,     100,    0, 	   0,             &spi_nand_function0 ,
    //1,		 50000,     	8,			8,
    //0x000002},//GD5F4GQ4UAYIG
    //{ {0xc8, 0xb4, 0x68, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     8,      64,
    //2048,   0x017d,     100,    0, 	   0,             &spi_nand_function2 ,
    //1,		 50000,     	8,			5,
    //0x000003},//GD5F4GQ4UCYIXX
    //{ {0xc8, 0x20, 0x7f, 0x7f, 0x7f, 0xff, 0xff, 0xff }, 1,     4,      64,
    //512,   0x007d,     100,    0, 	   1,             &spi_nand_function3 , 1,
    //50000,     	1,			1,
    //0x000004},//PSU12S20BN

    //------------------------------------------------------------------------------------------------------------------------
	{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	 0,
	 0,
	 0,
	 0,
	 0x0000,
	 0,
	 0,
	 0,
	 0,
	 0}, // NULL
};

//==============================================================================
//============================ ATO NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t AtoNandTbl[] = {
    //                   NAND_CHIP_ID                 DieCnt SecCnt  PagCnt
    //                   BlkCnt    OpOpt   Freq     mode   pagewithbadflag
    //                   function		     offset
    //                   maxerasetime	maxecc	ecclimit	idnumber
    //-------------------------------------------------------------------------
    // no support because of no testing
    //{ {0x9b, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,
    //1024,   0x007c,   100,    0,	   0,           &spi_nand_function0, 1,
    //100000,     		0x010000 },//ATO25D1GA

    //-------------------------------------------------------------------------
	{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	0,
	0,
	0,
	0,
	0x0000,
	0,
	0,
	0,
	0,
	0}, // NULL
};

//==============================================================================
//============================ Micron NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t MicronNandTbl[] = {
    //                   NAND_CHIP_ID                 DieCnt SecCnt  PagCnt
    //                   BlkCnt    OpOpt   Freq     mode   pagewithbadflag
    //                   function		     offset
    //                   maxerasetime	maxecc	ecclimit	idnumber
    //-------------------------------------------------------------------------
    // no support because of no testing
    //{ {0x2c, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,
    //1024,   0x00fd,   40,    0,	   0,           &spi_nand_function1, 1,
    //50000,     	4,			1,     	0x020000
    //},//MT29F1G01AAADD
    // { {0x2c, 0x22, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,
    // 2048,   0x00fd,   40,    0,	   0,           &spi_nand_function1, 1,
    // 50000,     	4,			1,     	0x020001
    // },//MT29F2G01AAAED
    //{ {0x2c, 0x32, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,
    //4096,   0x00fd,   40,    0,	   0,           &spi_nand_function1, 1,
    //50000,	4,			1,     	0x020002
    //},//MT29F4G01AAADD

    //-------------------------------------------------------------------------
	{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	0,
	0,
	0,
	0,
	0x0000,
	0,
	0,
	0,
	0,
	0}, // NULL
};

//=============================================================================
//============================ Mxic NAND FLASH ==============================
//=============================================================================
struct __NandPhyInfoPar_t MxicNandTbl[] = {
    //                   NAND_CHIP_ID                 DieCnt SecCnt  PagCnt
    //                   BlkCnt    OpOpt   Freq     mode   pagewithbadflag
    //                   function		     offset
    //                   maxerasetime	maxecc	ecclimit	idnumber
    //-------------------------------------------------------------------------
	{{0xc2, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	1,
	4,
	64,
	1024,
	0x067c,
	75,
	0,
	1,
	&spi_nand_function0,
	1,
	65000,
	4,
	1,
	0x030000}, // MX35LF1GE4AB
	// no support because of no testing
	{{0xc2, 0x22, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	1,
	4,
	64,
	2048,
	0x06fc,
	75,
	0,
	1,
	&spi_nand_function1,
	1,
	65000,
	4,
	1,
	0x030001}, // MX35LF2GE4AB

	//---------------------------------------------------------------------
	{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	0,
	0,
	0,
	0,
	0x0000,
	0,
	0,
	0,
	0,
	0}, // NULL
};
//=============================================================================
//=========================== Winbond NAND FLASH ==============================
//=============================================================================
struct __NandPhyInfoPar_t WinbondNandTbl[] = {
    //                   NAND_CHIP_ID                 DieCnt SecCnt  PagCnt
    //                   BlkCnt    OpOpt   Freq     mode   pagewithbadflag
    //                   function		     offset
    //                   maxerasetime	maxecc	ecclimit	idnumber
    //-------------------------------------------------------------------------
    //{ {0xef, 0xaa, 0x21, 0xff, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,
    //1024,   0x007d,   75,    0,	   0,             &spi_nand_function0, 1,
    //65000,     	4,			1,     	0x040000
    //},//W25N01GVSF1G

    //-------------------------------------------------------------------------
	{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	0,
	0,
	0,
	0,
	0x0000,
	0,
	0,
	0,
	0,
	0}, // NULL
};

//=============================================================================
//============================ DEFAULT NAND FLASH =============================
//=============================================================================
struct __NandPhyInfoPar_t DefaultNandTbl[] = {
    //                    NAND_CHIP_ID                DieCnt SecCnt  PagCnt
    //                    BlkCnt    OpOpt   Freq   mode   pagewithbadflag
    //                    function		     offset
    //                    maxerasetime	maxecc	ecclimit	idnumber
    //-------------------------------------------------------------------------
	{{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	0,
	0,
	0,
	0,
	0x0000,
	0,
	0,
	0,
	NULL,
	0,
	0,
	0x000000}, // default
};

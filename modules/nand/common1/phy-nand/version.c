/* SPDX-License-Identifier: GPL-2.0 */
/*
 ************************************************************************************************************************
 *                                                      eNand
 *                                           Nand flash driver scan module
 *
 *                             Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
 *                                                  All Rights Reserved
 *
 * File Name : version.c
 *
 * Author :
 *
 * Version : v0.1
 *
 * Date : 2013-11-20
 *
 * Description :
 *
 * Others : None at present.
 *
 *
 *
 ************************************************************************************************************************
 */
#define _NVER_C_

#include "version.h"
#include "../nfd/nand_osal_for_linux.h"
#include "nand.h"
#include "nand_boot.h"
#include "nand_errno.h"
#include "nand_nftl.h"
#include "nand_physic_interface.h"
#include "rawnand/rawnand_chip.h"
#include "rawnand/controller/ndfc_base.h"
#include "rawnand/controller/ndfc_ops.h"
#include "rawnand/rawnand.h"
#include "rawnand/rawnand_base.h"
#include "rawnand/rawnand_debug.h"


/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
 *****************************************************************************/
__u32 nand_get_nand_version(void)
{
	__u32 nand_version;

	nand_version = 0;
	nand_version |= 0xff;
	nand_version |= 0x00 << 8;
	nand_version |= NAND_VERSION_0 << 16;
	nand_version |= NAND_VERSION_1 << 24;

	return nand_version;
}

int nand_get_version(__u8 *nand_version)
{
	__u32 version;
	version = nand_get_nand_version();

	nand_version[0] = (u8)version;
	nand_version[1] = (u8)(version >> 8);
	nand_version[2] = (u8)(version >> 16);
	nand_version[3] = (u8)(version >> 24);

	return version;
}


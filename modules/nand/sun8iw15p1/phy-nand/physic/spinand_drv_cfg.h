
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
*                                     Nand flash driver module config define
*
*                             Copyright(C), 2006-2008, SoftWinners
*Microelectronic Co., Ltd.
*											       All
*Rights Reserved
*
* File Name : nand_drv_cfg.h
*
* Author : Kevin.z
*
* Version : v0.1
*
* Date : 2008.03.19
*
* Description : This file define the module config for nand flash driver.
*               if need support some module /
*               if need support some operation type /
*               config limit for some parameter. ex.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Kevin.z         2008.03.19      0.1          build the file
*
************************************************************************************************************************
*/
#ifndef __SPINAND_DRV_CFG_H
#define __SPINAND_DRV_CFG_H

#include "../nand_osal.h"

//==============================================================================
//  define the value of some variable for
//==============================================================================

// don't care this version information
//#define  NAND_DRV_PHYSIC_LAYER_VERSION_0	0x2
//#define  NAND_DRV_PHYSIC_LAYER_VERSION_1	0x21
//#define  NAND_DRV_PHYSIC_LAYER_DATE			0x20141212
//#define  NAND_DRV_PHYSIC_LAYER_TIME			0x1122

// don't care this version information
//#define  NAND_VERSION_0                 0x02
//#define  NAND_VERSION_1                 0x21
//#define  NAND_DRV_DATE                  0x20141212
//#define  NAND_PART_TABLE_MAGIC          0x0055ff00
//#define MAX_NFC_CH                          (1) //2

// define the max value of the count of chip select
#define MAX_CHIP_SELECT_CNT (1)
#define NAND_MAX_PART_CNT (20)

// define the number of the chip select which connecting the boot chip
#define BOOT_CHIP_SELECT_NUM (0)

//==============================================================================
//  define some sitch to decide if need support some operation
//==============================================================================

// define the switch that if need support multi-plane program
#define CFG_SUPPORT_MULTI_PLANE_PROGRAM (1) // 1

// define the switch that if need support multi-plane read
#define CFG_SUPPORT_MULTI_PLANE_READ (1) // Unused

// define the switch that if need support external inter-leave
#define CFG_SUPPORT_EXT_INTERLEAVE (1) // 0

// define the switch that if need support dual program
#define CFG_SUPPORT_DUAL_PROGRAM (0)

// define the switch that if need support dual read
#define CFG_SUPPORT_DUAL_READ (1)

// define the switch that if need support quad program
#define CFG_SUPPORT_QUAD_PROGRAM (1)

// define the switch that if need support quad read
#define CFG_SUPPORT_QUAD_READ (1)

// define the switch that if need support doing page copyback by send command
#define CFG_SUPPORT_PAGE_COPYBACK (1) // Unused

// define the switch that if need support wear-levelling
#define CFG_SUPPORT_WEAR_LEVELLING (0) // Unused

// define the switch that if need support read-reclaim
#define CFG_SUPPORT_READ_RECLAIM (1)

// define if need check the page program status after page program immediately
#define CFG_SUPPORT_CHECK_WRITE_SYNCH (1)

// define if need support align bank when allocating the log page
#define CFG_SUPPORT_ALIGN_NAND_BNK (1)

// define if need support Randomizer
//#define CFG_SUPPORT_RANDOM                      (0)

// define if need support ReadRetry
//#define CFG_SUPPORT_READ_RETRY                  (0)

#define SUPPORT_DMA_IRQ (0)
#define SUPPORT_RB_IRQ (0)

//==============================================================================
//  define some pr__s32 switch
//==============================================================================
// define if need pr__s32 the nand hardware scan module debug message
#define SCAN_DBG_MESSAGE_ON (1)

// define if need pr__s32 the nand hardware scan module error message
#define SCAN_ERR_MESSAGE_ON (1)

// define if need pr__s32 the nand disk format module debug message
#ifndef __OS_NAND_DBG__
#define FORMAT_DBG_MESSAGE_ON (0)
#else
#define FORMAT_DBG_MESSAGE_ON (1)
#endif

// define if need pr__s32 the nand disk format module error message
#define FORMAT_ERR_MESSAGE_ON (1)

// define if need pr__s32 the mapping manage module debug message
#define MAPPING_DBG_MESSAGE_ON (0)

// define if need pr__s32 the mapping manage module error message
#define MAPPING_ERR_MESSAGE_ON (1)

// define if need pr__s32 the logic control layer debug message
#define LOGICCTL_DBG_MESSAGE_ON (0)

// define if need pr__s32 the logic control layer error message
#define LOGICCTL_ERR_MESSAGE_ON (1)

#define FORMAT_WRITE_SPARE_DEBUG_ON (0)
#define FORMAT_READ_SPARE_DEBUG_ON (0)

#define PHY_PAGE_READ_ECC_ERR_DEBUG_ON (0)

#define SUPPORT_READ_CHECK_SPARE (0)

#define SUPPORT_SCAN_EDO_FOR_SDR_NAND (0) // 0

#define SUPPORT_UPDATE_WITH_OLD_PHYSIC_ARCH (1)

#define SUPPORT_UPDATE_EXTERNAL_ACCESS_FREQ (1)

//#define GOOD_DDR_EDO_DELAY_CHAIN_TH				(15)

#if PHY_DBG_MESSAGE_ON
#define PHY_DBG(...) PRINT_DBG(__VA_ARGS__)
#else
#define PHY_DBG(...)
#endif

#if PHY_ERR_MESSAGE_ON
#define PHY_ERR(...) PRINT(__VA_ARGS__)
#else
#define PHY_ERR(...)
#endif

#if SCAN_DBG_MESSAGE_ON
#define SCAN_DBG(...) PRINT_DBG(__VA_ARGS__)
#else
#define SCAN_DBG(...)
#endif

#if SCAN_ERR_MESSAGE_ON
#define SCAN_ERR(...) PRINT(__VA_ARGS__)
#else
#define SCAN_ERR(...)
#endif

#if FORMAT_DBG_MESSAGE_ON
#define FORMAT_DBG(...) PRINT_DBG(__VA_ARGS__)
#else
#define FORMAT_DBG(...)
#endif

#if FORMAT_ERR_MESSAGE_ON
#define FORMAT_ERR(...) PRINT(__VA_ARGS__)
#else
#define FORMAT_ERR(...)
#endif

#if MAPPING_DBG_MESSAGE_ON
#define MAPPING_DBG(...) PRINT_DBG(__VA_ARGS__)
#else
#define MAPPING_DBG(...)
#endif

#if MAPPING_ERR_MESSAGE_ON
#define MAPPING_ERR(...) PRINT(__VA_ARGS__)
#else
#define MAPPING_ERR(...)
#endif

#if LOGICCTL_DBG_MESSAGE_ON
#define LOGICCTL_DBG(...) PRINT_DBG(__VA_ARGS__)
#else
#define LOGICCTL_DBG(...)
#endif

#if LOGICCTL_ERR_MESSAGE_ON
#define LOGICCTL_ERR(...) PRINT(__VA_ARGS__)
#else
#define LOGICCTL_ERR(...)
#endif

#define DBUG_INF(...) // PRINT(__VA_ARGS__)
#define DBUG_MSG(...) // PRINT(__VA_ARGS__)

#endif // ifndef __NAND_DRV_CFG_H

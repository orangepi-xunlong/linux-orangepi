
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

#ifndef __NAND_SCAN_H__
#define __NAND_SCAN_H__

#include "../phy/phy.h"
#include "nand_physic.h"
#include "nand_physic_interface_spinand.h"
#include "nand_type_spinand.h"

//==============================================================================
//  define nand flash manufacture ID number
//==============================================================================

#define MICRON_NAND 0x2c  // Micron nand flash manufacture number
#define GD_NAND 0xc8      // GD and MIRA nand flash manufacture number
#define ATO_NAND 0x9b     // ATO nand flash manufacture number
#define WINBOND_NAND 0xef // winbond nand flash manufacture number
#define MXIC_NAND 0xc2    // mxic nand flash manufacture number

//==============================================================================
//  define the function __s32erface for nand storage scan module
//==============================================================================

/*
 ******************************************************************************
 *                           ANALYZE NAND FLASH STORAGE SYSTEM
 *
 *Description: Analyze nand flash storage system, generate the nand flash
 *physical
 *             architecture parameter and connect information.
 *
 *Arguments  : none
 *
 *Return     : analyze result;
 *               = 0     analyze successful;
 *               < 0     analyze failed, can't recognize or some other error.
 ******************************************************************************
 */
__s32 SCN_AnalyzeNandSystem(void);

__u32 NAND_GetValidBlkRatio(void);
__s32 NAND_SetValidBlkRatio(__u32 ValidBlkRatio);
__u32 NAND_GetFrequencePar(void);
__s32 NAND_SetFrequencePar(__u32 FrequencePar);
__u32 NAND_GetNandVersion(void);
//__s32 NAND_GetParam(boot_spinand_para_t * nand_param);

extern __u32 NAND_GetPlaneCnt(void);

#endif // ifndef __NAND_SCAN_H__

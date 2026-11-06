/** @file */
/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 ******************************************************************************/

#ifndef _TABLEUPD_H2C_8852B_H_
#define _TABLEUPD_H2C_8852B_H_

#include "../../type.h"
#include "../fwcmd.h"
#if MAC_AX_8852B_SUPPORT
/*--------------------Define MACRO--------------------------------------*/
#define CCTL_NTX_PATH_EN_8852B		3
#define CCTL_PATH_MAP_B			1
#define CCTL_PATH_MAP_C			2
#define CCTL_PATH_MAP_D			3
#define CCTRL_NC			1
#define CCTRL_NR			1
#define CCTRL_CB			1
#define CCTRL_NOMINAL_PKT_PADDING_0	0
#define CCTRL_NOMINAL_PKT_PADDING_8	1
#define CCTRL_NOMINAL_PKT_PADDING_16	2

/*--------------------Define Enum---------------------------------------*/

/*--------------------Define Struct-------------------------------------*/

/**
 * @addtogroup Role
 * @{
 * @addtogroup init
 * @{
 */

/**
 * @brief mac_init_cctl_info_8852b
 *
 * @param *adapter
 * @param *macid
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_init_cctl_info_8852b(struct mac_ax_adapter *adapter, u8 macid);
/**
 * @}
 * @}
 */
#endif /* MAC_AX_8852B_SUPPORT  */
#endif

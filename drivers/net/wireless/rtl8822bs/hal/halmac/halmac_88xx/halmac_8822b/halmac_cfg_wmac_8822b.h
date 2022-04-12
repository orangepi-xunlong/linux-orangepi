/******************************************************************************
 *
 * Copyright(c) 2017 Realtek Corporation. All rights reserved.
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

#ifndef _HALMAC_CFG_WMAC_8822B_H_
#define _HALMAC_CFG_WMAC_8822B_H_

#include "../../halmac_api.h"

#if HALMAC_8822B_SUPPORT

HALMAC_RET_STATUS
halmac_cfg_drv_info_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DRV_INFO halmac_drv_info
);

#endif/* HALMAC_8822B_SUPPORT */

#endif/* _HALMAC_CFG_WMAC_8822B_H_ */

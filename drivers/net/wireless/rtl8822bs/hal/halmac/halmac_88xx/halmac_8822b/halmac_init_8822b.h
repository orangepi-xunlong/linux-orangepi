/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation. All rights reserved.
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

#ifndef _HALMAC_INIT_8822B_H_
#define _HALMAC_INIT_8822B_H_

#include "../../halmac_api.h"

#if HALMAC_8822B_SUPPORT

HALMAC_RET_STATUS
halmac_mount_api_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_init_trx_cfg_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode
);

HALMAC_RET_STATUS
halmac_init_protocol_cfg_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_init_h2c_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

#endif /* HALMAC_8822B_SUPPORT */

#endif/* _HALMAC_INIT_8822B_H_ */

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

#ifndef _HALMAC_FW_88XX_H_
#define _HALMAC_FW_88XX_H_

#include "../halmac_api.h"

#if HALMAC_88XX_SUPPORT

#endif /* HALMAC_88XX_SUPPORT */

HALMAC_RET_STATUS
halmac_download_firmware_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHamacl_fw,
	IN u32 halmac_fw_size
);

HALMAC_RET_STATUS
halmac_free_download_firmware_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DLFW_MEM dlfw_mem,
	IN u8 *pHamacl_fw,
	IN u32 halmac_fw_size
);

HALMAC_RET_STATUS
halmac_get_fw_version_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT PHALMAC_FW_VERSION pFw_version
);

HALMAC_RET_STATUS
halmac_check_fw_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u8 *fw_status
);

HALMAC_RET_STATUS
halmac_dump_fw_dmem_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT u8 *dmem,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_cfg_max_dl_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 size
);

#endif/* _HALMAC_FW_88XX_H_ */

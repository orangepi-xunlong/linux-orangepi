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

#include "halmac_cfg_wmac_8822b.h"
#include "halmac_8822b_cfg.h"

#if HALMAC_8822B_SUPPORT

/**
 * halmac_cfg_drv_info_88xx() - config driver info
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_drv_info : driver information selection
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 * Need to build halmac_cfg_drv_info_8821C, halmac_cfg_drv_info_8822B, halmac_cfg_drv_info_88OO
 * Because 88OO has no need to patch Rx packet counter. Soar 20161110
 */
HALMAC_RET_STATUS
halmac_cfg_drv_info_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DRV_INFO halmac_drv_info
)
{
	u8 drv_info_size = 0;
	u8 phy_status_en = 0;
	u8 sniffer_en = 0;
	u8 plcp_hdr_en = 0;
	u8 value8;
	u32 value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_drv_info_8822b ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_drv_info = %d\n", halmac_drv_info);

	switch (halmac_drv_info) {
	case HALMAC_DRV_INFO_NONE:
		drv_info_size = 0;
		phy_status_en = 0;
		sniffer_en = 0;
		plcp_hdr_en = 0;
		break;
	case HALMAC_DRV_INFO_PHY_STATUS:
		drv_info_size = 4;
		phy_status_en = 1;
		sniffer_en = 0;
		plcp_hdr_en = 0;
		break;
	case HALMAC_DRV_INFO_PHY_SNIFFER:
		drv_info_size = 5; /* phy status 4byte, sniffer info 1byte */
		phy_status_en = 1;
		sniffer_en = 1;
		plcp_hdr_en = 0;
		break;
	case HALMAC_DRV_INFO_PHY_PLCP:
		drv_info_size = 6; /* phy status 4byte, plcp header 2byte */
		phy_status_en = 1;
		sniffer_en = 0;
		plcp_hdr_en = 1;
		break;
	default:
		status = HALMAC_RET_SW_CASE_NOT_SUPPORT;
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_cfg_drv_info_8822b error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode != HALMAC_RX_FIFO_EXPANDING_MODE_DISABLE)
		drv_info_size = HALMAC_RX_DESC_DUMMY_SIZE_MAX_8822B >> 3;

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RX_DRVINFO_SZ, drv_info_size);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_TRXFF_BNDY + 1);
	value8 &= 0xF0;
	/* value8 |= (drv_info_size + (pHalmac_adapter->hw_config_info.rxdesc_size >> 3) + 1); */
	value8 |= 0xF; /* For rxdesc len = 0 issue. set to correct value after finding root cause */
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TRXFF_BNDY + 1, value8);

	pHalmac_adapter->drv_info_size = drv_info_size;

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_RCR);
	value32 = (value32 & (~BIT_APP_PHYSTS));
	if (phy_status_en == 1)
		value32 = value32 | BIT_APP_PHYSTS;
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RCR, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_WMAC_OPTION_FUNCTION + 4);
	value32 = (value32 & (~(BIT(8) | BIT(9))));
	if (sniffer_en == 1)
		value32 = value32 | BIT(9);
	if (plcp_hdr_en == 1)
		value32 = value32 | BIT(8);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WMAC_OPTION_FUNCTION + 4, value32);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_drv_info_8822b <==========\n");

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_8822B_SUPPORT */

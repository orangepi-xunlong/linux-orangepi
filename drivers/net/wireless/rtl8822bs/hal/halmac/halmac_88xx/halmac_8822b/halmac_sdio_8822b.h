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

#ifndef _HALMAC_API_8822B_SDIO_H_
#define _HALMAC_API_8822B_SDIO_H_

#include "../../halmac_api.h"
#include "halmac_8822b_cfg.h"

#if HALMAC_8822B_SUPPORT

HALMAC_RET_STATUS
halmac_mac_power_switch_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_MAC_POWER halmac_power
);

HALMAC_RET_STATUS
halmac_tx_allowed_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
);

HALMAC_RET_STATUS
halmac_phy_cfg_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_INTF_PHY_PLATFORM platform
);

HALMAC_RET_STATUS
halmac_pcie_switch_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PCIE_CFG	pcie_cfg
);

HALMAC_RET_STATUS
halmac_interface_integration_tuning_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_get_sdio_tx_addr_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size,
	OUT u32 *pcmd53_addr
);

u8
halmac_reg_read_8_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset
);

HALMAC_RET_STATUS
halmac_reg_write_8_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u8 halmac_data
);

u16
halmac_reg_read_16_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset
);

HALMAC_RET_STATUS
halmac_reg_write_16_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u16 halmac_data
);

u32
halmac_reg_read_32_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset
);

HALMAC_RET_STATUS
halmac_reg_write_32_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u32 halmac_data
);

#endif /* HALMAC_8822B_SUPPORT*/

#endif/* _HALMAC_API_8822B_SDIO_H_ */

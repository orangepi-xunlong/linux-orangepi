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

#include "halmac_cfg_wmac_88xx.h"
#include "halmac_88xx_cfg.h"
#include "halmac_efuse_88xx.h"

#if HALMAC_88XX_SUPPORT

#define EFUSE_PCB_INFO_OFFSET	0xCA

static HALMAC_RET_STATUS
board_rf_fine_tune_88xx(
	IN PHALMAC_ADAPTER adapter
);

/**
 * halmac_cfg_mac_addr_88xx() - config mac address
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_port : 0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * @pHal_address : mac address
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_mac_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN PHALMAC_WLAN_ADDR pHal_address
)
{
	u16 mac_address_H;
	u32 mac_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_mac_addr_88xx ==========>\n");

	if (halmac_port >= HALMAC_PORTIDMAX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]port index >= 5\n");
		return HALMAC_RET_PORT_NOT_SUPPORT;
	}

	mac_address_L = pHal_address->Address_L_H.Address_Low;
	mac_address_H = pHal_address->Address_L_H.Address_High;

	mac_address_L = rtk_le32_to_cpu(mac_address_L);
	mac_address_H = rtk_le16_to_cpu(mac_address_H);

	pHalmac_adapter->pHal_mac_addr[halmac_port].Address_L_H.Address_Low = mac_address_L;
	pHalmac_adapter->pHal_mac_addr[halmac_port].Address_L_H.Address_High = mac_address_H;

	switch (halmac_port) {
	case HALMAC_PORTID0:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID + 4, mac_address_H);
		break;

	case HALMAC_PORTID1:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID1, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID1 + 4, mac_address_H);
		break;

	case HALMAC_PORTID2:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID2, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID2 + 4, mac_address_H);
		break;

	case HALMAC_PORTID3:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID3, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID3 + 4, mac_address_H);
		break;

	case HALMAC_PORTID4:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID4, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID4 + 4, mac_address_H);
		break;

	default:
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_mac_addr_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_bssid_88xx() - config BSSID
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_port : 0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * @pHal_address : bssid
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_bssid_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN PHALMAC_WLAN_ADDR pHal_address
)
{
	u16 bssid_address_H;
	u32 bssid_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_bssid_88xx ==========>\n");

	if (halmac_port >= HALMAC_PORTIDMAX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]port index > 5\n");
		return HALMAC_RET_PORT_NOT_SUPPORT;
	}

	bssid_address_L = pHal_address->Address_L_H.Address_Low;
	bssid_address_H = pHal_address->Address_L_H.Address_High;

	bssid_address_L = rtk_le32_to_cpu(bssid_address_L);
	bssid_address_H = rtk_le16_to_cpu(bssid_address_H);

	pHalmac_adapter->pHal_bss_addr[halmac_port].Address_L_H.Address_Low = bssid_address_L;
	pHalmac_adapter->pHal_bss_addr[halmac_port].Address_L_H.Address_High = bssid_address_H;

	switch (halmac_port) {
	case HALMAC_PORTID0:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID + 4, bssid_address_H);
		break;

	case HALMAC_PORTID1:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID1, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID1 + 4, bssid_address_H);
		break;

	case HALMAC_PORTID2:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID2, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID2 + 4, bssid_address_H);
		break;

	case HALMAC_PORTID3:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID3, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID3 + 4, bssid_address_H);
		break;

	case HALMAC_PORTID4:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID4, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID4 + 4, bssid_address_H);
		break;

	default:
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_bssid_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_transmitter_addr_88xx() - config transmitter address
 * @pHalmac_adapter
 * @halmac_port :  0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * @pHal_address
 * Author : Alan
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_cfg_transmitter_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN PHALMAC_WLAN_ADDR pHal_address
)
{
	u16 mac_address_H;
	u32 mac_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_transmitter_addr_88xx ==========>\n");

	if (halmac_port >= HALMAC_PORTIDMAX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]port index > 5\n");
		return HALMAC_RET_PORT_NOT_SUPPORT;
	}

	mac_address_L = pHal_address->Address_L_H.Address_Low;
	mac_address_H = pHal_address->Address_L_H.Address_High;

	mac_address_L = rtk_le32_to_cpu(mac_address_L);
	mac_address_H = rtk_le16_to_cpu(mac_address_H);

	pHalmac_adapter->pHal_tx_addr[halmac_port].Address_L_H.Address_Low = mac_address_L;
	pHalmac_adapter->pHal_tx_addr[halmac_port].Address_L_H.Address_High = mac_address_H;

	switch (halmac_port) {
	case HALMAC_PORTID0:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TRANSMIT_ADDRSS_0, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TRANSMIT_ADDRSS_0 + 4, mac_address_H);
		break;

	case HALMAC_PORTID1:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TRANSMIT_ADDRSS_1, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TRANSMIT_ADDRSS_1 + 4, mac_address_H);
		break;

	case HALMAC_PORTID2:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TRANSMIT_ADDRSS_2, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TRANSMIT_ADDRSS_2 + 4, mac_address_H);
		break;

	case HALMAC_PORTID3:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TRANSMIT_ADDRSS_3, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TRANSMIT_ADDRSS_3 + 4, mac_address_H);
		break;

	case HALMAC_PORTID4:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TRANSMIT_ADDRSS_4, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TRANSMIT_ADDRSS_4 + 4, mac_address_H);
		break;

	default:
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_transmitter_addr_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_net_type_88xx() - config network type
 * @pHalmac_adapter
 * @halmac_port :  0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * @pHal_address : mac address
 * Author : Alan
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_cfg_net_type_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN HALMAC_NETWORK_TYPE_SELECT net_type
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u8	networktype = 0;
	u8	net_type_temp = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_net_type_88xx ==========>\n");

	if (net_type == HALMAC_NETWORK_AP) {
		if (halmac_port >= HALMAC_PORTID1) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]AP port index > 1\n");
			return HALMAC_RET_PORT_NOT_SUPPORT;
		}
	}

	switch (halmac_port) {
	case HALMAC_PORTID0:
		/* reg 0x100[17:16]*/
		net_type_temp = net_type;
		networktype = ((HALMAC_REG_READ_8(pHalmac_adapter, REG_CR + 2) & 0xFC) | net_type_temp);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 2, networktype);
		break;
	case HALMAC_PORTID1:
		/* reg 0x100[19:18]*/
		net_type_temp = (net_type << 2);
		networktype = ((HALMAC_REG_READ_8(pHalmac_adapter, REG_CR + 2) & 0xF3) | net_type_temp);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 2, networktype);
		break;
	case HALMAC_PORTID2:
		/* reg 0x1100[1:0]*/
		net_type_temp = net_type;
		networktype = ((HALMAC_REG_READ_8(pHalmac_adapter, REG_CR_EXT) & 0xFC) | net_type_temp);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR_EXT, networktype);
		break;
	case HALMAC_PORTID3:
		/* reg 0x1100[3:2]*/
		net_type_temp = (net_type << 2);
		networktype = ((HALMAC_REG_READ_8(pHalmac_adapter, REG_CR_EXT) & 0xF3) | net_type_temp);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR_EXT, networktype);
		break;
	case HALMAC_PORTID4:
		/* reg 0x1100[5:4]*/
		net_type_temp = (net_type << 4);
		networktype = ((HALMAC_REG_READ_8(pHalmac_adapter, REG_CR_EXT) & 0xCF) | net_type_temp);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR_EXT, networktype);
		break;
	default:
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_net_type_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_tsf_rst_88xx() - tsf reset
 * @pHalmac_adapter
 * @halmac_port :  0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * Author : Alan
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_cfg_tsf_rst_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_tsf_rst_88xx ==========>\n");

	switch (halmac_port) {
	case HALMAC_PORTID0:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_DUAL_TSF_RST, HALMAC_REG_READ_8(pHalmac_adapter, REG_DUAL_TSF_RST) | BIT_TSFTR_RST);
		break;
	case HALMAC_PORTID1:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_DUAL_TSF_RST, HALMAC_REG_READ_8(pHalmac_adapter, REG_DUAL_TSF_RST) | BIT_TSFTR_CLI0_RST);
		break;
	case HALMAC_PORTID2:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_DUAL_TSF_RST, HALMAC_REG_READ_8(pHalmac_adapter, REG_DUAL_TSF_RST) | BIT_TSFTR_CLI1_RST);
		break;
	case HALMAC_PORTID3:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_DUAL_TSF_RST, HALMAC_REG_READ_8(pHalmac_adapter, REG_DUAL_TSF_RST) | BIT_TSFTR_CLI2_RST);
		break;
	case HALMAC_PORTID4:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_DUAL_TSF_RST, HALMAC_REG_READ_8(pHalmac_adapter, REG_DUAL_TSF_RST) | BIT_TSFTR_CLI3_RST);
		break;
	default:
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_tsf_rst_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_bcn_space_88xx() - config beacon space
 * @pHalmac_adapter
 * @halmac_port :  0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * @bcn_space : beacon space
 * Author : Alan
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_cfg_bcn_space_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN u32 bcn_space
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u16	bcn_space_real = 0;
	u16	bcn_space_temp = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_bcn_space_88xx ==========>\n");

	bcn_space_real = ((u16)bcn_space);

	switch (halmac_port) {
	case HALMAC_PORTID0:
		/*reg 0x554[15:0]*/
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MBSSID_BCN_SPACE, bcn_space_real);
		break;

	case HALMAC_PORTID1:
		/*reg 0x554[27:16]*/
		bcn_space_temp = ((HALMAC_REG_READ_16(pHalmac_adapter, REG_MBSSID_BCN_SPACE + 2) & 0xF000) | bcn_space_real);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MBSSID_BCN_SPACE + 2, bcn_space_temp);
		break;

	case HALMAC_PORTID2:
		/*reg 0x5B8[11:0]*/
		bcn_space_temp = ((HALMAC_REG_READ_16(pHalmac_adapter, REG_MBSSID_BCN_SPACE2) & 0xF000) | bcn_space_real);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MBSSID_BCN_SPACE2, bcn_space_temp);
		break;

	case HALMAC_PORTID3:
		/*reg 0x5B8[27:16]*/
		bcn_space_temp = ((HALMAC_REG_READ_16(pHalmac_adapter, REG_MBSSID_BCN_SPACE2 + 2) & 0xF000) | bcn_space_real);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MBSSID_BCN_SPACE2 + 2, bcn_space_temp);
		break;

	case HALMAC_PORTID4:
		/*reg 0x5BC[11:0]*/
		bcn_space_temp = ((HALMAC_REG_READ_16(pHalmac_adapter, REG_MBSSID_BCN_SPACE3) & 0xF000) | bcn_space_real);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MBSSID_BCN_SPACE3, bcn_space_temp);
		break;

	default:
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_bcn_space_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_rw_bcn_ctrl_88xx() - r/w beacon control
 * @pHalmac_adapter
 * @halmac_port :  0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * @write_en : 1->write beacon function 0->read beacon function
 * @pBcn_ctrl : beacon control info
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_rw_bcn_ctrl_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN u8 write_en,
	INOUT PHALMAC_BCN_CTRL pBcn_ctrl
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u8	BCN_CTRL_Value = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_rw_bcn_ctrl_88xx ==========>\n");

	if (write_en) {
		if (pBcn_ctrl->dis_rx_bssid_fit == _TRUE)
			BCN_CTRL_Value = (BCN_CTRL_Value | BIT_DIS_RX_BSSID_FIT);

		if (pBcn_ctrl->en_txbcn_rpt == _TRUE)
			BCN_CTRL_Value = (BCN_CTRL_Value | BIT_P0_EN_TXBCN_RPT);

		if (pBcn_ctrl->dis_tsf_udt == _TRUE)
			BCN_CTRL_Value = (BCN_CTRL_Value | BIT_DIS_TSF_UDT);

		if (pBcn_ctrl->en_bcn == _TRUE)
			BCN_CTRL_Value = (BCN_CTRL_Value | BIT_EN_BCN_FUNCTION);

		if (pBcn_ctrl->en_rxbcn_rpt == _TRUE)
			BCN_CTRL_Value = (BCN_CTRL_Value | BIT_P0_EN_RXBCN_RPT);

		if (pBcn_ctrl->en_p2p_ctwin == _TRUE)
			BCN_CTRL_Value = (BCN_CTRL_Value | BIT_EN_P2P_CTWINDOW);

		if (pBcn_ctrl->en_p2p_bcn_area == _TRUE)
			BCN_CTRL_Value = (BCN_CTRL_Value | BIT_EN_P2P_BCNQ_AREA);

		switch (halmac_port) {
		case HALMAC_PORTID0:
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, BCN_CTRL_Value);
			break;

		case HALMAC_PORTID1:
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL_CLINT0, BCN_CTRL_Value);
			break;

		case HALMAC_PORTID2:
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL_CLINT1, BCN_CTRL_Value);
			break;

		case HALMAC_PORTID3:
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL_CLINT2, BCN_CTRL_Value);
			break;

		case HALMAC_PORTID4:
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL_CLINT3, BCN_CTRL_Value);
			break;

		default:
			break;
		}

	} else {
		switch (halmac_port) {
		case HALMAC_PORTID0:
			BCN_CTRL_Value = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL);
			break;

		case HALMAC_PORTID1:
			BCN_CTRL_Value = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL_CLINT0);
			break;

		case HALMAC_PORTID2:
			BCN_CTRL_Value = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL_CLINT1);
			break;

		case HALMAC_PORTID3:
			BCN_CTRL_Value = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL_CLINT2);
			break;

		case HALMAC_PORTID4:
			BCN_CTRL_Value = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL_CLINT3);
			break;

		default:
			break;
		}

		if (BCN_CTRL_Value & BIT_EN_P2P_BCNQ_AREA)
			pBcn_ctrl->en_p2p_bcn_area = _TRUE;
		else
			pBcn_ctrl->en_p2p_bcn_area = _FALSE;

		if (BCN_CTRL_Value & BIT_EN_P2P_CTWINDOW)
			pBcn_ctrl->en_p2p_ctwin = _TRUE;
		else
			pBcn_ctrl->en_p2p_ctwin = _FALSE;

		if (BCN_CTRL_Value & BIT_P0_EN_RXBCN_RPT)
			pBcn_ctrl->en_rxbcn_rpt = _TRUE;
		else
			pBcn_ctrl->en_rxbcn_rpt = _FALSE;

		if (BCN_CTRL_Value & BIT_EN_BCN_FUNCTION)
			pBcn_ctrl->en_bcn = _TRUE;
		else
			pBcn_ctrl->en_bcn = _FALSE;

		if (BCN_CTRL_Value & BIT_DIS_TSF_UDT)
			pBcn_ctrl->dis_tsf_udt = _TRUE;
		else
			pBcn_ctrl->dis_tsf_udt = _FALSE;

		if (BCN_CTRL_Value & BIT_P0_EN_TXBCN_RPT)
			pBcn_ctrl->en_txbcn_rpt = _TRUE;
		else
			pBcn_ctrl->en_txbcn_rpt = _FALSE;

		if (BCN_CTRL_Value & BIT_DIS_RX_BSSID_FIT)
			pBcn_ctrl->dis_rx_bssid_fit = _TRUE;
		else
			pBcn_ctrl->dis_rx_bssid_fit = _FALSE;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_rw_bcn_ctrl_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_multicast_addr_88xx() - config multicast address
 * @pHalmac_adapter : the adapter of halmac
 * @pHal_address : multicast address
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_multicast_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_WLAN_ADDR pHal_address
)
{
	u16 address_H;
	u32 address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_multicast_addr_88xx ==========>\n");

	address_L = pHal_address->Address_L_H.Address_Low;
	address_H = pHal_address->Address_L_H.Address_High;

	address_L = rtk_le32_to_cpu(address_L);
	address_H = rtk_le16_to_cpu(address_H);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MAR, address_L);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MAR + 4, address_H);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_multicast_addr_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_operation_mode_88xx() - config operation mode
 * @pHalmac_adapter : the adapter of halmac
 * @wireless_mode : 802.11 standard(b/g/n/ac¡K)
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_operation_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_WIRELESS_MODE wireless_mode
)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_ch_bw_88xx() - config channel & bandwidth
 * @pHalmac_adapter : the adapter of halmac
 * @channel : WLAN channel, support 2.4G & 5G
 * @pri_ch_idx : primary channel index, idx1, idx2, idx3, idx4
 * @bw : band width, 20, 40, 80, 160, 5 ,10
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_ch_bw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 channel,
	IN HALMAC_PRI_CH_IDX pri_ch_idx,
	IN HALMAC_BW bw
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_ch_bw_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]ch = %d, idx=%d, bw=%d\n", channel, pri_ch_idx, bw);

	halmac_cfg_pri_ch_idx_88xx(pHalmac_adapter,  pri_ch_idx);

	halmac_cfg_bw_88xx(pHalmac_adapter, bw);

	halmac_cfg_ch_88xx(pHalmac_adapter,  channel);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_ch_bw_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_cfg_ch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 channel
)
{
	u8 value8;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_ch_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]ch = %d\n", channel);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CCK_CHECK);
	value8 = value8 & (~(BIT(7)));

	if (channel > 35)
		value8 = value8 | BIT(7);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CCK_CHECK, value8);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_ch_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_cfg_pri_ch_idx_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PRI_CH_IDX pri_ch_idx
)
{
	u8 txsc_40 = 0, txsc_20 = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_pri_ch_idx_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]idx=%d\n",  pri_ch_idx);

	txsc_20 = pri_ch_idx;
	if ((txsc_20 == HALMAC_CH_IDX_1) || (txsc_20 == HALMAC_CH_IDX_3))
		txsc_40 = 9;
	else
		txsc_40 = 10;

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_DATA_SC, BIT_TXSC_20M(txsc_20) | BIT_TXSC_40M(txsc_40));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_pri_ch_idx_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_bw_88xx() - config bandwidth
 * @pHalmac_adapter : the adapter of halmac
 * @bw : band width, 20, 40, 80, 160, 5 ,10
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_bw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_BW bw
)
{
	u32 value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_bw_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]bw=%d\n", bw);

	/* RF Mode */
	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_WMAC_TRXPTCL_CTL);
	value32 = value32 & (~(BIT(7) | BIT(8)));

	switch (bw) {
	case HALMAC_BW_80:
		value32 = value32 | BIT(8);
		break;
	case HALMAC_BW_40:
		value32 = value32 | BIT(7);
		break;
	case HALMAC_BW_20:
	case HALMAC_BW_10:
	case HALMAC_BW_5:
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_cfg_bw_88xx switch case not support\n");
		break;
	}
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WMAC_TRXPTCL_CTL, value32);

	/* MAC CLK */
	/* TODO:Move to change mac clk api later... */
	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_AFE_CTRL1);
	value32 = (value32 & (~(BIT(20) | BIT(21)))) | (HALMAC_MAC_CLOCK_HW_DEF_80M << BIT_SHIFT_MAC_CLK_SEL);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_AFE_CTRL1, value32);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_USTIME_TSF, HALMAC_MAC_CLOCK_88XX);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_USTIME_EDCA, HALMAC_MAC_CLOCK_88XX);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_bw_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_enable_bb_rf_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
)
{
	u8 value8;
	u32 value32;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (enable == 1) {
		status = board_rf_fine_tune_88xx(pHalmac_adapter);
		value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_FUNC_EN);
		value8 = value8 | BIT(0) | BIT(1);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_FUNC_EN, value8);

		value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_RF_CTRL);
		value8 = value8 | BIT(0) | BIT(1) | BIT(2);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RF_CTRL, value8);

		value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_WLRF1);
		value32 = value32 | BIT(24) | BIT(25) | BIT(26);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WLRF1, value32);
	} else {
		value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_FUNC_EN);
		value8 = value8 & (~(BIT(0) | BIT(1)));
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_FUNC_EN, value8);

		value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_RF_CTRL);
		value8 = value8 & (~(BIT(0) | BIT(1) | BIT(2)));
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RF_CTRL, value8);

		value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_WLRF1);
		value32 = value32 & (~(BIT(24) | BIT(25) | BIT(26)));
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WLRF1, value32);
	}

	return status;
}

static HALMAC_RET_STATUS
board_rf_fine_tune_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 *map = NULL;
	u32 size = pHalmac_adapter->hw_config_info.eeprom_size;
	PHALMAC_API pHalmac_api;
	VOID *drv_adapter = NULL;

	drv_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822B) {
		if (!pHalmac_adapter->hal_efuse_map_valid || !pHalmac_adapter->pHalEfuse_map) {
			PLATFORM_MSG_PRINT(drv_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]efuse map invalid!!\n\n");
			return HALMAC_RET_EFUSE_R_FAIL;
		}

		map = (u8 *)PLATFORM_RTL_MALLOC(drv_adapter, size);
		if (!map) {
			PLATFORM_MSG_PRINT(drv_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]malloc map\n");
			return HALMAC_RET_MALLOC_FAIL;
		}

		PLATFORM_RTL_MEMSET(drv_adapter, map, 0xFF, size);

		if (halmac_eeprom_parser_88xx(pHalmac_adapter, pHalmac_adapter->pHalEfuse_map, map) !=
		    HALMAC_RET_SUCCESS) {
			PLATFORM_RTL_FREE(drv_adapter, map, size);
			return HALMAC_RET_EEPROM_PARSING_FAIL;
		}

		/* Fine-tune XTAL voltage for 2L PCB board */
		if (*(map + EFUSE_PCB_INFO_OFFSET) == 0x0C)
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_AFE_CTRL1 + 1,
					   HALMAC_REG_READ_8(pHalmac_adapter, REG_AFE_CTRL1 + 1) | BIT(1));

		PLATFORM_RTL_FREE(drv_adapter, map, size);
	}

	return HALMAC_RET_SUCCESS;
}

VOID
halmac_config_ampdu_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_AMPDU_CONFIG pAmpdu_config
)
{
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_PROT_MODE_CTRL + 2, pAmpdu_config->max_agg_num);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_PROT_MODE_CTRL + 3, pAmpdu_config->max_agg_num);
}

/**
 * halmac_cfg_la_mode_88xx() - config la mode
 * @pHalmac_adapter : the adapter of halmac
 * @la_mode :
 *	disable : no TXFF space reserved for LA debug
 *	partial : partial TXFF space is reserved for LA debug
 *	full : all TXFF space is reserved for LA debug
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_la_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_LA_MODE la_mode
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (pHalmac_adapter->api_registry.la_mode_en == 0)
		return HALMAC_RET_NOT_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_la_mode_88xx ==========>la_mode = %d\n", la_mode);

	pHalmac_adapter->txff_allocation.la_mode = la_mode;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_la_mode_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_rx_fifo_expanding_mode_88xx() - rx fifo expanding
 * @pHalmac_adapter : the adapter of halmac
 * @la_mode :
 *	disable : normal mode
 *	1 block : Rx FIFO + 1 FIFO block; Tx fifo - 1 FIFO block
 *	2 block : Rx FIFO + 2 FIFO block; Tx fifo - 2 FIFO block
 *	3 block : Rx FIFO + 3 FIFO block; Tx fifo - 3 FIFO block
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_rx_fifo_expanding_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_RX_FIFO_EXPANDING_MODE rx_fifo_expanding_mode
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (pHalmac_adapter->api_registry.rx_expand_mode_en == 0)
		return HALMAC_RET_NOT_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_rx_fifo_expanding_mode_88xx ==========>rx_fifo_expanding_mode = %d\n", rx_fifo_expanding_mode);

	pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode = rx_fifo_expanding_mode;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_rx_fifo_expanding_mode_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_config_security_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_SECURITY_SETTING pSec_setting
)
{
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;
	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_config_security_88xx ==========>\n");

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_CR, (u16)(HALMAC_REG_READ_16(pHalmac_adapter, REG_CR) | BIT_MAC_SEC_EN));

	if (pSec_setting->compare_keyid == 1) {
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG + 1, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG + 1) | BIT(0));
		pHalmac_adapter->hw_config_info.security_check_keyid = 1;
	} else {
		pHalmac_adapter->hw_config_info.security_check_keyid = 0;
	}

	/* BC/MC use default key(cam entry 0~3, kei id = 0 -> entry0, kei id = 1 -> entry1... ) */
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG) | BIT(6) | BIT(7));

	if (pSec_setting->tx_encryption == 1)
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG) | BIT(2));
	else
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG) & ~(BIT(2)));

	if (pSec_setting->rx_decryption == 1)
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG) | BIT(3));
	else
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG) & ~(BIT(3)));

	if (pSec_setting->bip_enable == 1) {
		if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822B)
			return HALMAC_RET_BIP_NO_SUPPORT;
#if HALMAC_8821C_SUPPORT
		if (pSec_setting->tx_encryption == 1)
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_WSEC_OPTION + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_WSEC_OPTION + 2) | (BIT(3) | BIT(5)));
		else
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_WSEC_OPTION + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_WSEC_OPTION + 2) & ~(BIT(3) | BIT(5)));

		if (pSec_setting->rx_decryption == 1)
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_WSEC_OPTION + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_WSEC_OPTION + 2) | (BIT(4) | BIT(6)));
		else
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_WSEC_OPTION + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_WSEC_OPTION + 2) & ~(BIT(4) | BIT(6)));
#endif
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_config_security_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

u8
halmac_get_used_cam_entry_num_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HAL_SECURITY_TYPE sec_type
)
{
	u8 entry_num;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_get_used_cam_entry_num_88xx ==========>\n");

	switch (sec_type) {
	case HAL_SECURITY_TYPE_WEP40:
	case HAL_SECURITY_TYPE_WEP104:
	case HAL_SECURITY_TYPE_TKIP:
	case HAL_SECURITY_TYPE_AES128:
	case HAL_SECURITY_TYPE_GCMP128:
	case HAL_SECURITY_TYPE_GCMSMS4:
	case HAL_SECURITY_TYPE_BIP:
		entry_num = 1;
		break;
	case HAL_SECURITY_TYPE_WAPI:
	case HAL_SECURITY_TYPE_AES256:
	case HAL_SECURITY_TYPE_GCMP256:
		entry_num = 2;
		break;
	default:
		entry_num = 0;
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_get_used_cam_entry_num_88xx <==========\n");

	return entry_num;
}

HALMAC_RET_STATUS
halmac_write_cam_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u32 entry_index,
	IN PHALMAC_CAM_ENTRY_INFO pCam_entry_info
)
{
	u32 i;
	u32 command = 0x80010000;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;
	PHALMAC_CAM_ENTRY_FORMAT pCam_entry_format = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_write_cam_88xx ==========>\n");

	if (entry_index >= pHalmac_adapter->hw_config_info.cam_entry_num)
		return HALMAC_RET_ENTRY_INDEX_ERROR;

	if (pCam_entry_info->key_id > 3)
		return HALMAC_RET_FAIL;

	pCam_entry_format = (PHALMAC_CAM_ENTRY_FORMAT)PLATFORM_RTL_MALLOC(pDriver_adapter, sizeof(*pCam_entry_format));
	if (pCam_entry_format == NULL)
		return HALMAC_RET_NULL_POINTER;
	PLATFORM_RTL_MEMSET(pDriver_adapter, pCam_entry_format, 0x00, sizeof(*pCam_entry_format));

	if (pHalmac_adapter->hw_config_info.security_check_keyid == 1)
		pCam_entry_format->key_id = pCam_entry_info->key_id;
	pCam_entry_format->valid = pCam_entry_info->valid;
	PLATFORM_RTL_MEMCPY(pDriver_adapter, pCam_entry_format->mac_address, pCam_entry_info->mac_address, 6);
	PLATFORM_RTL_MEMCPY(pDriver_adapter, pCam_entry_format->key, pCam_entry_info->key, 16);

	switch (pCam_entry_info->security_type) {
	case HAL_SECURITY_TYPE_NONE:
		pCam_entry_format->type = 0;
		break;
	case HAL_SECURITY_TYPE_WEP40:
		pCam_entry_format->type = 1;
		break;
	case HAL_SECURITY_TYPE_WEP104:
		pCam_entry_format->type = 5;
		break;
	case HAL_SECURITY_TYPE_TKIP:
		pCam_entry_format->type = 2;
		break;
	case HAL_SECURITY_TYPE_AES128:
		pCam_entry_format->type = 4;
		break;
	case HAL_SECURITY_TYPE_WAPI:
		pCam_entry_format->type = 6;
		break;
	case HAL_SECURITY_TYPE_AES256:
		pCam_entry_format->type = 4;
		pCam_entry_format->ext_sectype = 1;
		break;
	case HAL_SECURITY_TYPE_GCMP128:
		pCam_entry_format->type = 7;
		break;
	case HAL_SECURITY_TYPE_GCMP256:
	case HAL_SECURITY_TYPE_GCMSMS4:
		pCam_entry_format->type = 7;
		pCam_entry_format->ext_sectype = 1;
		break;
	case HAL_SECURITY_TYPE_BIP:
		pCam_entry_format->type = (pCam_entry_info->unicast == 1) ? 4 : 0;
		pCam_entry_format->mgnt = 1;
		pCam_entry_format->grp = (pCam_entry_info->unicast == 1) ? 0 : 1;
		break;
	default:
		PLATFORM_RTL_FREE(pDriver_adapter, pCam_entry_format, sizeof(*pCam_entry_format));
		return HALMAC_RET_FAIL;
	}


	for (i = 0; i < 8; i++) {
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMWRITE, *((u32 *)pCam_entry_format + i));
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMCMD, command | ((entry_index << 3) + i));
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]1 - CAM entry format : %X\n", *((u32 *)pCam_entry_format + i));
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]1 - REG_CAMCMD : %X\n", command | ((entry_index << 3) + i));
	}

	if (HAL_SECURITY_TYPE_WAPI == pCam_entry_info->security_type || HAL_SECURITY_TYPE_AES256 == pCam_entry_info->security_type ||
			HAL_SECURITY_TYPE_GCMP256 == pCam_entry_info->security_type || HAL_SECURITY_TYPE_GCMSMS4 == pCam_entry_info->security_type) {
		pCam_entry_format->mic = 1;
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pCam_entry_format->key, pCam_entry_info->key_ext, 16);

		for (i = 0; i < 8; i++) {
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMWRITE, *((u32 *)pCam_entry_format + i));
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMCMD, command | (((entry_index + 1) << 3) + i));
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]2 - CAM entry format : %X\n", *((u32 *)pCam_entry_format + i));
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]2 - REG_CAMCMD : %X\n", command | (((entry_index + 1) << 3) + i));
		}
	}

	PLATFORM_RTL_FREE(pDriver_adapter, pCam_entry_format, sizeof(*pCam_entry_format));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_write_cam_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_read_cam_entry_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u32 entry_index,
	OUT PHALMAC_CAM_ENTRY_FORMAT pContent
)
{
	u32 i;
	u32 command = 0x80000000;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_read_cam_entry_88xx ==========>\n");

	if (entry_index >= pHalmac_adapter->hw_config_info.cam_entry_num)
		return HALMAC_RET_ENTRY_INDEX_ERROR;

	for (i = 0; i < 8; i++) {
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMCMD, command | ((entry_index << 3) + i));
		*((u32 *)pContent + i) = HALMAC_REG_READ_32(pHalmac_adapter, REG_CAMREAD);
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_read_cam_entry_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_clear_cam_entry_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 entry_index
)
{
	u32 i;
	u32 command = 0x80010000;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	PHALMAC_CAM_ENTRY_FORMAT pCam_entry_format;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_clear_security_cam_88xx ==========>\n");

	if (entry_index >= pHalmac_adapter->hw_config_info.cam_entry_num)
		return HALMAC_RET_ENTRY_INDEX_ERROR;

	pCam_entry_format = (PHALMAC_CAM_ENTRY_FORMAT)PLATFORM_RTL_MALLOC(pDriver_adapter, sizeof(*pCam_entry_format));
	if (pCam_entry_format == NULL)
		return HALMAC_RET_NULL_POINTER;
	PLATFORM_RTL_MEMSET(pDriver_adapter, pCam_entry_format, 0x00, sizeof(*pCam_entry_format));

	for (i = 0; i < 8; i++) {
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMWRITE, *((u32 *)pCam_entry_format + i));
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMCMD, command | ((entry_index << 3) + i));
	}

	PLATFORM_RTL_FREE(pDriver_adapter, pCam_entry_format, sizeof(*pCam_entry_format));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_clear_security_cam_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

VOID
halmac_rx_shift_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
)
{
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (enable == 1)
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_PQ_MAP, HALMAC_REG_READ_8(pHalmac_adapter, REG_TXDMA_PQ_MAP) | BIT(1));
	else
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_PQ_MAP, HALMAC_REG_READ_8(pHalmac_adapter, REG_TXDMA_PQ_MAP) & ~(BIT(1)));
}

/**
 * halmac_cfg_edca_para_88xx() - config edca parameter
 * @pHalmac_adapter : the adapter of halmac
 * @acq_id : VO/VI/BE/BK
 * @pEdca_para : aifs, cw, txop limit
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_edca_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_ACQ_ID acq_id,
	IN PHALMAC_EDCA_PARA pEdca_para
)
{
	u32 offset, value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_edca_88xx ==========>\n");

	switch (acq_id) {
	case HALMAC_ACQ_ID_VO:
		offset = REG_EDCA_VO_PARAM;
		break;
	case HALMAC_ACQ_ID_VI:
		offset = REG_EDCA_VI_PARAM;
		break;
	case HALMAC_ACQ_ID_BE:
		offset = REG_EDCA_BE_PARAM;
		break;
	case HALMAC_ACQ_ID_BK:
		offset = REG_EDCA_BK_PARAM;
		break;
	default:
		return HALMAC_RET_SWITCH_CASE_ERROR;
	}

	value32 = (pEdca_para->aifs & 0xFF) | ((pEdca_para->cw & 0xFF) << 8) | ((pEdca_para->txop_limit & 0x7FF) << 16);

	HALMAC_REG_WRITE_32(pHalmac_adapter, offset, value32);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_edca_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

VOID
halmac_rx_clk_gate_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
)
{
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (enable == _TRUE)
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RCR + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_RCR + 2) & ~(BIT(3)));
	else
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RCR + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_RCR + 2) | BIT(3));
}

HALMAC_RET_STATUS
halmac_rx_cut_amsdu_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CUT_AMSDU_CFG pCut_amsdu_cfg
)
{
	return HALMAC_RET_NOT_SUPPORT;
}

/**
 * halmac_get_mac_addr_88xx() - get mac address
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_port : 0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * @pHal_address : mac address
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_mac_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	OUT PHALMAC_WLAN_ADDR pHal_address
)
{
	u16 mac_address_H;
	u32 mac_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_get_mac_addr_88xx ==========>\n");

	if (halmac_port >= HALMAC_PORTIDMAX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]port index >= 5\n");
		return HALMAC_RET_PORT_NOT_SUPPORT;
	}

	switch (halmac_port) {
	case HALMAC_PORTID0:
		mac_address_L = HALMAC_REG_READ_32(pHalmac_adapter, REG_MACID);
		mac_address_H = HALMAC_REG_READ_16(pHalmac_adapter, REG_MACID + 4);
		break;
	case HALMAC_PORTID1:
		mac_address_L = HALMAC_REG_READ_32(pHalmac_adapter, REG_MACID1);
		mac_address_H = HALMAC_REG_READ_16(pHalmac_adapter, REG_MACID1 + 4);
		break;
	case HALMAC_PORTID2:
		mac_address_L = HALMAC_REG_READ_32(pHalmac_adapter, REG_MACID2);
		mac_address_H = HALMAC_REG_READ_16(pHalmac_adapter, REG_MACID2 + 4);
		break;
	case HALMAC_PORTID3:
		mac_address_L = HALMAC_REG_READ_32(pHalmac_adapter, REG_MACID3);
		mac_address_H = HALMAC_REG_READ_16(pHalmac_adapter, REG_MACID3 + 4);
		break;
	case HALMAC_PORTID4:
		mac_address_L = HALMAC_REG_READ_32(pHalmac_adapter, REG_MACID4);
		mac_address_H = HALMAC_REG_READ_16(pHalmac_adapter, REG_MACID4 + 4);
		break;
	default:
		return HALMAC_RET_PORT_NOT_SUPPORT;
	}

	mac_address_L = rtk_le32_to_cpu(mac_address_L);
	mac_address_H = rtk_le16_to_cpu(mac_address_H);

	pHal_address->Address_L_H.Address_Low = mac_address_L;
	pHal_address->Address_L_H.Address_High = mac_address_H;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_get_mac_addr_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_88XX_SUPPORT */

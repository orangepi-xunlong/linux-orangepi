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

#include "halmac_sdio_8822b.h"
#include "halmac_pwr_seq_8822b.h"
#include "../halmac_init_88xx.h"
#include "../halmac_common_88xx.h"
#include "../halmac_sdio_88xx.h"

#if HALMAC_8822B_SUPPORT

static HALMAC_RET_STATUS
halmac_check_oqt_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 tx_agg_num,
	IN u8 *pHalmac_buf,
	IN u8 macid_counter
);

static HALMAC_RET_STATUS
halmac_update_oqt_free_space_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_update_sdio_free_page_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

/**
 * halmac_mac_power_switch_8822b_sdio() - switch mac power
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_power : power state
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mac_power_switch_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_MAC_POWER	halmac_power
)
{
	u8 interface_mask;
	u8 value8;
	u8 rpwm;
	u32 imr_backup;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "[TRACE]halmac_mac_power_switch_88xx_sdio==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "[TRACE]halmac_power = %x ==========>\n", halmac_power);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "[TRACE]8822B pwr seq ver = %s\n", HALMAC_8822B_PWR_SEQ_VER);

	interface_mask = HALMAC_PWR_INTF_SDIO_MSK;

	pHalmac_adapter->rpwm_record = HALMAC_REG_READ_8(pHalmac_adapter, REG_SDIO_HRPWM1);

	/* Check FW still exist or not */
	if (HALMAC_REG_READ_16(pHalmac_adapter, REG_MCUFW_CTRL) == 0xC078) {
		/* Leave 32K */
		rpwm = (u8)((pHalmac_adapter->rpwm_record ^ BIT(7)) & 0x80);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SDIO_HRPWM1, rpwm);
	}

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR);
	if (value8 == 0xEA)
		pHalmac_adapter->halmac_state.mac_power = HALMAC_MAC_POWER_OFF;
	else
		pHalmac_adapter->halmac_state.mac_power = HALMAC_MAC_POWER_ON;

	/*Check if power switch is needed*/
	if (halmac_power == HALMAC_MAC_POWER_ON && pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_ON) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_WARN, "[WARN]halmac_mac_power_switch power state unchange!\n");
		return HALMAC_RET_PWR_UNCHANGE;
	}

	imr_backup = HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_HIMR);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SDIO_HIMR, 0);

	if (halmac_power == HALMAC_MAC_POWER_OFF) {
		pHalmac_adapter->pwr_off_flow_flag = 1;
		if (halmac_pwr_seq_parser_88xx(pHalmac_adapter, HALMAC_PWR_CUT_ALL_MSK, HALMAC_PWR_FAB_TSMC_MSK,
			    interface_mask, halmac_8822b_card_disable_flow) != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_ERR, "[ERR]Handle power off cmd error\n");
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SDIO_HIMR, imr_backup);
			return HALMAC_RET_POWER_OFF_FAIL;
		}

		pHalmac_adapter->halmac_state.mac_power = HALMAC_MAC_POWER_OFF;
		pHalmac_adapter->halmac_state.ps_state = HALMAC_PS_STATE_UNDEFINE;
		pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_NONE;
		pHalmac_adapter->pwr_off_flow_flag = 0;
		halmac_init_adapter_dynamic_para_88xx(pHalmac_adapter);
	} else {
		if (halmac_pwr_seq_parser_88xx(pHalmac_adapter, HALMAC_PWR_CUT_ALL_MSK, HALMAC_PWR_FAB_TSMC_MSK,
			    interface_mask, halmac_8822b_card_enable_flow) != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_ERR, "[ERR]Handle power on cmd error\n");
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SDIO_HIMR, imr_backup);
			return HALMAC_RET_POWER_ON_FAIL;
		}

		pHalmac_adapter->halmac_state.mac_power = HALMAC_MAC_POWER_ON;
		pHalmac_adapter->halmac_state.ps_state = HALMAC_PS_STATE_ACT;
	}

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SDIO_HIMR, imr_backup);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "[TRACE]halmac_mac_power_switch_88xx_sdio <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_tx_allowed_sdio_88xx() - check tx status
 * @pHalmac_adapter : the adapter of halmac
 * @pHalmac_buf : tx packet, include txdesc
 * @halmac_size : tx packet size, include txdesc
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_tx_allowed_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
)
{
	u8 *pCurr_packet;
	u16 *pCurr_free_space;
	u32 i, counter;
	u32 tx_agg_num, packet_size = 0, macid_map_size;
	u32 tx_required_page_num, total_required_page_num = 0;
	/*tx descriptor DMA_TXAGG_NUM (8bits), support max 0xFF packets AGG*/
	u8 qsel_first, qsel_now;
	u8 macid, qsel_err_flag = 0, macid_counter = 0;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	VOID *pDriver_adapter = NULL;
	HALMAC_DMA_MAPPING dma_mapping;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	macid_map_size = pHalmac_adapter->sdio_free_space.macid_map_size;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_tx_allowed_sdio_88xx ==========>\n");

	if (NULL == pHalmac_adapter->sdio_free_space.pMacid_map) {
			PLATFORM_MSG_PRINT(pHalmac_adapter->pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]halmac allocate Macid_map Fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
	}

	PLATFORM_RTL_MEMSET(pDriver_adapter, pHalmac_adapter->sdio_free_space.pMacid_map, 0x00, macid_map_size);

	tx_agg_num = GET_TX_DESC_DMA_TXAGG_NUM(pHalmac_buf);
	pCurr_packet = pHalmac_buf;

	tx_agg_num = (tx_agg_num == 0) ? 1 : tx_agg_num;

	qsel_first = (u8)GET_TX_DESC_QSEL(pCurr_packet);
	switch ((HALMAC_QUEUE_SELECT)qsel_first) {
	case HALMAC_QUEUE_SELECT_VO:
	case HALMAC_QUEUE_SELECT_VO_V2:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VO];
		break;
	case HALMAC_QUEUE_SELECT_VI:
	case HALMAC_QUEUE_SELECT_VI_V2:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VI];
		break;
	case HALMAC_QUEUE_SELECT_BE:
	case HALMAC_QUEUE_SELECT_BE_V2:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BE];
		break;
	case HALMAC_QUEUE_SELECT_BK:
	case HALMAC_QUEUE_SELECT_BK_V2:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BK];
		break;
	case HALMAC_QUEUE_SELECT_MGNT:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_MG];
		break;
	case HALMAC_QUEUE_SELECT_HIGH:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI];
		break;
	case HALMAC_QUEUE_SELECT_BCN:
	case HALMAC_QUEUE_SELECT_CMD:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_WARN, "QSEL = %d. BCN/CMD always return HALMAC_RET_SUCCESS\n", qsel_first);
		return HALMAC_RET_SUCCESS;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]Qsel is out of range\n");
		return HALMAC_RET_QSEL_INCORRECT;
	}

	switch (dma_mapping) {
	case HALMAC_DMA_MAPPING_HIGH:
		pCurr_free_space = &pHalmac_adapter->sdio_free_space.high_queue_number;
		break;
	case HALMAC_DMA_MAPPING_NORMAL:
		pCurr_free_space = &pHalmac_adapter->sdio_free_space.normal_queue_number;
		break;
	case HALMAC_DMA_MAPPING_LOW:
		pCurr_free_space = &pHalmac_adapter->sdio_free_space.low_queue_number;
		break;
	case HALMAC_DMA_MAPPING_EXTRA:
		pCurr_free_space = &pHalmac_adapter->sdio_free_space.extra_queue_number;
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]DmaMapping is out of range\n");
		return HALMAC_RET_DMA_MAP_INCORRECT;
	}

	for (i = 0; i < tx_agg_num; i++) {
		/*MACID parser*/
		macid = (u8)GET_TX_DESC_MACID(pCurr_packet);
		qsel_now = (u8)GET_TX_DESC_QSEL(pCurr_packet);
		/*QSEL parser*/
		if (qsel_first == qsel_now) {
			if (*(pHalmac_adapter->sdio_free_space.pMacid_map + macid) == 0) {
				*(pHalmac_adapter->sdio_free_space.pMacid_map + macid) = 1;
				macid_counter++;
			}
		} else {
			switch ((HALMAC_QUEUE_SELECT)qsel_now) {
			case HALMAC_QUEUE_SELECT_VO:
				if ((HALMAC_QUEUE_SELECT)qsel_first != HALMAC_QUEUE_SELECT_VO_V2)
					qsel_err_flag = 1;
				break;
			case HALMAC_QUEUE_SELECT_VO_V2:
				if ((HALMAC_QUEUE_SELECT)qsel_first != HALMAC_QUEUE_SELECT_VO)
					qsel_err_flag = 1;
				break;
			case HALMAC_QUEUE_SELECT_VI:
				if ((HALMAC_QUEUE_SELECT)qsel_first != HALMAC_QUEUE_SELECT_VI_V2)
					qsel_err_flag = 1;
				break;
			case HALMAC_QUEUE_SELECT_VI_V2:
				if ((HALMAC_QUEUE_SELECT)qsel_first != HALMAC_QUEUE_SELECT_VI)
					qsel_err_flag = 1;
				break;
			case HALMAC_QUEUE_SELECT_BE:
				if ((HALMAC_QUEUE_SELECT)qsel_first != HALMAC_QUEUE_SELECT_BE_V2)
					qsel_err_flag = 1;
				break;
			case HALMAC_QUEUE_SELECT_BE_V2:
				if ((HALMAC_QUEUE_SELECT)qsel_first != HALMAC_QUEUE_SELECT_BE)
					qsel_err_flag = 1;
				break;
			case HALMAC_QUEUE_SELECT_BK:
				if ((HALMAC_QUEUE_SELECT)qsel_first != HALMAC_QUEUE_SELECT_BK_V2)
					qsel_err_flag = 1;
				break;
			case HALMAC_QUEUE_SELECT_BK_V2:
				if ((HALMAC_QUEUE_SELECT)qsel_first != HALMAC_QUEUE_SELECT_BK)
					qsel_err_flag = 1;
				break;
			case HALMAC_QUEUE_SELECT_MGNT:
			case HALMAC_QUEUE_SELECT_HIGH:
			case HALMAC_QUEUE_SELECT_BCN:
			case HALMAC_QUEUE_SELECT_CMD:
				qsel_err_flag = 1;
				break;
			default:
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "Qsel is out of range: %d\n", qsel_first);
				return HALMAC_RET_QSEL_INCORRECT;
			}
			if (qsel_err_flag == 1) {
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "Multi-Qsel in a bus agg is not allowed, qsel = %d, %d\n", qsel_first, qsel_now);
				return HALMAC_RET_QSEL_INCORRECT;
			}

			if (*(pHalmac_adapter->sdio_free_space.pMacid_map + macid + HALMAC_MACID_MAX_88XX) == 0) {
				*(pHalmac_adapter->sdio_free_space.pMacid_map + macid + HALMAC_MACID_MAX_88XX) = 1;
				macid_counter++;
			}
		}
		/*Page number parser*/
		packet_size = GET_TX_DESC_TXPKTSIZE(pCurr_packet) + GET_TX_DESC_OFFSET(pCurr_packet);
		tx_required_page_num = (packet_size >> pHalmac_adapter->hw_config_info.page_size_2_power) + ((packet_size & (pHalmac_adapter->hw_config_info.page_size - 1)) ? 1 : 0);
		total_required_page_num += tx_required_page_num;

		pCurr_packet += HALMAC_ALIGN(GET_TX_DESC_TXPKTSIZE(pCurr_packet) + (GET_TX_DESC_PKT_OFFSET(pCurr_packet) << 3) + HALMAC_TX_DESC_SIZE_88XX, 8);
	}

	counter = 10;
	do {
		if ((u32)(*pCurr_free_space + pHalmac_adapter->sdio_free_space.public_queue_number) > total_required_page_num) {
			status = halmac_check_oqt_8822b(pHalmac_adapter, tx_agg_num, pHalmac_buf, macid_counter);
			if (status != HALMAC_RET_SUCCESS) {
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_WARN, "[WARN]oqt buffer full!!\n");
				return status;
			}

			if (*pCurr_free_space >= total_required_page_num) {
				*pCurr_free_space -= (u16)total_required_page_num;
			} else {
				pHalmac_adapter->sdio_free_space.public_queue_number -= (u16)(total_required_page_num - *pCurr_free_space);
				*pCurr_free_space = 0;
			}

			break;
		}

		halmac_update_sdio_free_page_8822b(pHalmac_adapter);

		counter--;
		if (counter == 0)
			return HALMAC_RET_FREE_SPACE_NOT_ENOUGH;
	} while (1);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_tx_allowed_sdio_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_check_oqt_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 tx_agg_num,
	IN u8 *pHalmac_buf,
	IN u8 macid_counter
)
{
	u32 counter = 10;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	/*S0, S1 are not allowed to use, 0x4E4[0] should be 0. Soar 20160323*/
	/*no need to check non_ac_oqt_number. HI and MGQ blocked will cause protocal issue before H_OQT being full*/
	switch ((HALMAC_QUEUE_SELECT)GET_TX_DESC_QSEL(pHalmac_buf)) {
	case HALMAC_QUEUE_SELECT_VO:
	case HALMAC_QUEUE_SELECT_VO_V2:
	case HALMAC_QUEUE_SELECT_VI:
	case HALMAC_QUEUE_SELECT_VI_V2:
	case HALMAC_QUEUE_SELECT_BE:
	case HALMAC_QUEUE_SELECT_BE_V2:
	case HALMAC_QUEUE_SELECT_BK:
	case HALMAC_QUEUE_SELECT_BK_V2:
		if ((macid_counter > HALMAC_ACQ_NUM_MAX_88XX) && (tx_agg_num > HALMAC_OQT_ENTRY_AC_8822B))
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_WARN, "tx_agg_num %d > HALMAC_OQT_ENTRY_AC_88XX, macid_counter %d > HALMAC_ACQ_NUM_MAX_88XX\n", tx_agg_num, macid_counter);
		counter = 10;
		do {
			if (pHalmac_adapter->sdio_free_space.ac_empty >= macid_counter) {
				pHalmac_adapter->sdio_free_space.ac_empty -= macid_counter;
				break;
			}

			if (pHalmac_adapter->sdio_free_space.ac_oqt_number >= tx_agg_num) {
				pHalmac_adapter->sdio_free_space.ac_empty = 0;
				pHalmac_adapter->sdio_free_space.ac_oqt_number -= (u8)tx_agg_num;
				break;
			}

			halmac_update_oqt_free_space_8822b(pHalmac_adapter);

			counter--;
			if (counter == 0)
				return HALMAC_RET_OQT_NOT_ENOUGH;
		} while (1);
		break;
	case HALMAC_QUEUE_SELECT_MGNT:
	case HALMAC_QUEUE_SELECT_HIGH:
		if (tx_agg_num > HALMAC_OQT_ENTRY_NOAC_8822B)
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_WARN, "tx_agg_num %d > HALMAC_OQT_ENTRY_NOAC_88XX\n", tx_agg_num);
		counter = 10;
		do {
			if (pHalmac_adapter->sdio_free_space.non_ac_oqt_number >= tx_agg_num) {
				pHalmac_adapter->sdio_free_space.non_ac_oqt_number -= (u8)tx_agg_num;
				break;
			}

			halmac_update_oqt_free_space_8822b(pHalmac_adapter);

			counter--;
			if (counter == 0)
				return HALMAC_RET_OQT_NOT_ENOUGH;
		} while (1);
		break;
	default:
		break;
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_update_oqt_free_space_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	PHALMAC_SDIO_FREE_SPACE pSdio_free_space;
	u8 value;
	u32 oqt_free_page;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_update_oqt_free_space_88xx ==========>\n");

	pSdio_free_space = &pHalmac_adapter->sdio_free_space;

	oqt_free_page = HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_OQT_FREE_TXPG_V1);
	pSdio_free_space->ac_oqt_number = (u8)BIT_GET_AC_OQT_FREEPG_V1(oqt_free_page);
	pSdio_free_space->non_ac_oqt_number = (u8)BIT_GET_NOAC_OQT_FREEPG_V1(oqt_free_page);
	pSdio_free_space->ac_empty = 0;
	if (pSdio_free_space->ac_oqt_number == HALMAC_OQT_ENTRY_AC_8822B) {
		value = HALMAC_REG_READ_8(pHalmac_adapter, REG_TXPKT_EMPTY);
		while (value > 0) {
			value = value & (value - 1);
			pSdio_free_space->ac_empty++;
		};
	} else {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]pSdio_free_space->ac_oqt_number %d != %d\n", pSdio_free_space->ac_oqt_number, HALMAC_OQT_ENTRY_AC_8822B);
	}
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_update_oqt_free_space_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_update_sdio_free_page_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u32 free_page = 0, free_page2 = 0, free_page3 = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	PHALMAC_SDIO_FREE_SPACE pSdio_free_space;
	u8 data[12] = {0};

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_update_sdio_free_page_88xx ==========>\n");

	pSdio_free_space = &pHalmac_adapter->sdio_free_space;

	HALMAC_REG_SDIO_CMD53_READ_N(pHalmac_adapter, REG_SDIO_FREE_TXPG, 12, data);

	free_page = rtk_le32_to_cpu(*(u32 *)(data + 0));
	free_page2 = rtk_le32_to_cpu(*(u32 *)(data + 4));
	free_page3 = rtk_le32_to_cpu(*(u32 *)(data + 8));

	pSdio_free_space->high_queue_number = (u16)BIT_GET_HIQ_FREEPG_V1(free_page);
	pSdio_free_space->normal_queue_number = (u16)BIT_GET_MID_FREEPG_V1(free_page);
	pSdio_free_space->low_queue_number = (u16)BIT_GET_LOW_FREEPG_V1(free_page2);
	pSdio_free_space->public_queue_number = (u16)BIT_GET_PUB_FREEPG_V1(free_page2);
	pSdio_free_space->extra_queue_number = (u16)BIT_GET_EXQ_FREEPG_V1(free_page3);
	pSdio_free_space->ac_oqt_number = (u8)BIT_GET_AC_OQT_FREEPG_V1(free_page3);
	pSdio_free_space->non_ac_oqt_number = (u8)BIT_GET_NOAC_OQT_FREEPG_V1(free_page3);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_update_sdio_free_page_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_phy_cfg_8822b_sdio() - phy config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_phy_cfg_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_INTF_PHY_PLATFORM platform
)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pcie_switch_8821c() - pcie gen1/gen2 switch
 * @pHalmac_adapter : the adapter of halmac
 * @pcie_cfg : gen1/gen2 selection
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pcie_switch_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PCIE_CFG	pcie_cfg
)
{
	return HALMAC_RET_NOT_SUPPORT;
}

/**
 * halmac_interface_integration_tuning_8822b_sdio() - sdio interface fine tuning
 * @pHalmac_adapter : the adapter of halmac
 * Author : Ivan
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_interface_integration_tuning_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_sdio_tx_addr_sdio_88xx() - get CMD53 addr for the TX packet
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_buf : tx packet, include txdesc
 * @halmac_size : tx packet size
 * @pcmd53_addr : cmd53 addr value
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_sdio_tx_addr_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size,
	OUT u32 *pcmd53_addr
)
{
	u32 four_byte_len;
	VOID *pDriver_adapter = NULL;
	HALMAC_QUEUE_SELECT queue_sel;
	HALMAC_DMA_MAPPING dma_mapping;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_get_sdio_tx_addr_88xx ==========>\n");

	if (halmac_buf == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_buf is NULL!!\n");
		return HALMAC_RET_DATA_BUF_NULL;
	}

	if (halmac_size == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_size is 0!!\n");
		return HALMAC_RET_DATA_SIZE_INCORRECT;
	}

	queue_sel = (HALMAC_QUEUE_SELECT)GET_TX_DESC_QSEL(halmac_buf);

	switch (queue_sel) {
	case HALMAC_QUEUE_SELECT_VO:
	case HALMAC_QUEUE_SELECT_VO_V2:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VO];
		break;
	case HALMAC_QUEUE_SELECT_VI:
	case HALMAC_QUEUE_SELECT_VI_V2:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VI];
		break;
	case HALMAC_QUEUE_SELECT_BE:
	case HALMAC_QUEUE_SELECT_BE_V2:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BE];
		break;
	case HALMAC_QUEUE_SELECT_BK:
	case HALMAC_QUEUE_SELECT_BK_V2:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BK];
		break;
	case HALMAC_QUEUE_SELECT_MGNT:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_MG];
		break;
	case HALMAC_QUEUE_SELECT_HIGH:
	case HALMAC_QUEUE_SELECT_BCN:
	case HALMAC_QUEUE_SELECT_CMD:
		dma_mapping = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI];
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Qsel is out of range\n");
		return HALMAC_RET_QSEL_INCORRECT;
	}

	four_byte_len = (halmac_size >> 2) + ((halmac_size & (4 - 1)) ? 1 : 0);

	switch (dma_mapping) {
	case HALMAC_DMA_MAPPING_HIGH:
		*pcmd53_addr = HALMAC_SDIO_CMD_ADDR_TXFF_HIGH;
		break;
	case HALMAC_DMA_MAPPING_NORMAL:
		*pcmd53_addr = HALMAC_SDIO_CMD_ADDR_TXFF_NORMAL;
		break;
	case HALMAC_DMA_MAPPING_LOW:
		*pcmd53_addr = HALMAC_SDIO_CMD_ADDR_TXFF_LOW;
		break;
	case HALMAC_DMA_MAPPING_EXTRA:
		*pcmd53_addr = HALMAC_SDIO_CMD_ADDR_TXFF_EXTRA;
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]DmaMapping is out of range\n");
		return HALMAC_RET_DMA_MAP_INCORRECT;
	}

	*pcmd53_addr = (*pcmd53_addr << 13) | (four_byte_len & HALMAC_SDIO_4BYTE_LEN_MASK);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_get_sdio_tx_addr_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_reg_read_8_sdio_88xx() - read 1byte register
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : register offset
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
u8
halmac_reg_read_8_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset
)
{
	u8 value8;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (0 == (halmac_offset & 0xFFFF0000)) {
		value8 = (u8)halmac_read_indirect_sdio_88xx(pHalmac_adapter, (u16)halmac_offset, HALMAC_IO_BYTE);
	} else {
		status = halmac_convert_to_sdio_bus_offset_88xx(pHalmac_adapter, &halmac_offset);

		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_reg_read_8_sdio_8822b error = %x\n", status);
			return status;
		}

		value8 = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, halmac_offset);
	}

	return value8;
}

/**
 * halmac_reg_write_8_sdio_88xx() - write 1byte register
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : register offset
 * @halmac_data : register value
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_reg_write_8_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u8 halmac_data
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (0 == (halmac_offset & 0xFFFF0000))
		halmac_offset |= WLAN_IOREG_OFFSET;

	status = halmac_convert_to_sdio_bus_offset_88xx(pHalmac_adapter, &halmac_offset);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_reg_write_8_sdio_8822b error = %x\n", status);
		return status;
	}

	PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset, halmac_data);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_reg_read_16_sdio_88xx() - read 2byte register
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : register offset
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
u16
halmac_reg_read_16_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	union {
		u16	word;
		u8	byte[2];
	} value16 = { 0x0000 };

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (0 == (halmac_offset & 0xFFFF0000)) {
		value16.word = (u16)halmac_read_indirect_sdio_88xx(pHalmac_adapter, (u16)halmac_offset, HALMAC_IO_WORD);
	} else {
		status = halmac_convert_to_sdio_bus_offset_88xx(pHalmac_adapter, &halmac_offset);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_reg_read_16_sdio_8822b error = %x\n", status);
			return status;
		}

		if ((pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF) || ((halmac_offset & (2 - 1)) != 0) ||
			(pHalmac_adapter->sdio_cmd53_4byte == HALMAC_SDIO_CMD53_4BYTE_MODE_RW) || (pHalmac_adapter->sdio_cmd53_4byte == HALMAC_SDIO_CMD53_4BYTE_MODE_R)) {
			value16.byte[0] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, halmac_offset);
			value16.byte[1] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, halmac_offset + 1);
			value16.word = rtk_le16_to_cpu(value16.word);
		} else {
			value16.word = PLATFORM_SDIO_CMD53_READ_16(pDriver_adapter, halmac_offset);
		}
	}

	return value16.word;
}

/**
 * halmac_reg_write_16_sdio_88xx() - write 2byte register
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : register offset
 * @halmac_data : register value
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_reg_write_16_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u16 halmac_data
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (0 == (halmac_offset & 0xFFFF0000))
		halmac_offset |= WLAN_IOREG_OFFSET;

	status = halmac_convert_to_sdio_bus_offset_88xx(pHalmac_adapter, &halmac_offset);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_reg_write_16_sdio_8822b error = %x\n", status);
		return status;
	}

	if ((pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF) || ((halmac_offset & (2 - 1)) != 0) ||
		(pHalmac_adapter->sdio_cmd53_4byte == HALMAC_SDIO_CMD53_4BYTE_MODE_RW) || (pHalmac_adapter->sdio_cmd53_4byte == HALMAC_SDIO_CMD53_4BYTE_MODE_W)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]use cmd52, offset = %x\n", halmac_offset);
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset, (u8)(halmac_data & 0xFF));
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset + 1, (u8)((halmac_data & 0xFF00) >> 8));
	} else {
		PLATFORM_SDIO_CMD53_WRITE_16(pDriver_adapter, halmac_offset, halmac_data);
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_reg_read_32_sdio_88xx() - read 4byte register
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : register offset
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
u32
halmac_reg_read_32_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	union {
		u32	dword;
		u8	byte[4];
	} value32 = { 0x00000000 };

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (0 == (halmac_offset & 0xFFFF0000)) {
		value32.dword = halmac_read_indirect_sdio_88xx(pHalmac_adapter, (u16)halmac_offset, HALMAC_IO_DWORD);
	} else {
		status = halmac_convert_to_sdio_bus_offset_88xx(pHalmac_adapter, &halmac_offset);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_reg_read_32_sdio_8822b error = %x\n", status);
			return status;
		}
		if (pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF || (halmac_offset & (4 - 1)) != 0) {
			value32.byte[0] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, halmac_offset);
			value32.byte[1] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, halmac_offset + 1);
			value32.byte[2] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, halmac_offset + 2);
			value32.byte[3] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, halmac_offset + 3);
			value32.dword = rtk_le32_to_cpu(value32.dword);
		} else {
			value32.dword = PLATFORM_SDIO_CMD53_READ_32(pDriver_adapter, halmac_offset);
		}
	}

	return value32.dword;
}

/**
 * halmac_reg_write_32_sdio_88xx() - write 4byte register
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : register offset
 * @halmac_data : register value
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_reg_write_32_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u32 halmac_data
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if ((halmac_offset & 0xFFFF0000) == 0)
		halmac_offset |= WLAN_IOREG_OFFSET;

	status = halmac_convert_to_sdio_bus_offset_88xx(pHalmac_adapter, &halmac_offset);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_reg_write_32_sdio_8822b error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF || (halmac_offset & (4 - 1)) !=  0) {
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset, (u8)(halmac_data & 0xFF));
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset + 1, (u8)((halmac_data & 0xFF00) >> 8));
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset + 2, (u8)((halmac_data & 0xFF0000) >> 16));
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset + 3, (u8)((halmac_data & 0xFF000000) >> 24));
	} else {
		PLATFORM_SDIO_CMD53_WRITE_32(pDriver_adapter, halmac_offset, halmac_data);
	}

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_8822B_SUPPORT*/

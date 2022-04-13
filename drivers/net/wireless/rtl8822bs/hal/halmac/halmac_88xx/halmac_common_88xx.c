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

#include "halmac_common_88xx.h"
#include "halmac_88xx_cfg.h"
#include "halmac_init_88xx.h"
#include "halmac_cfg_wmac_88xx.h"
#include "halmac_efuse_88xx.h"
#include "halmac_bb_rf_88xx.h"
#include "halmac_usb_88xx.h"
#include "halmac_sdio_88xx.h"
#include "halmac_pcie_88xx.h"
#include "halmac_mimo_88xx.h"

#if HALMAC_88XX_SUPPORT

static HALMAC_RET_STATUS
halmac_parse_c2h_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size
);

static HALMAC_RET_STATUS
halmac_parse_c2h_debug_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_scan_status_rpt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_psd_data_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_cfg_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_update_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_update_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_run_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_channel_switch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_iqk_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_power_tracking_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
);

static HALMAC_CFG_PARA_CMD_CONSTRUCT_STATE
halmac_query_cfg_para_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_send_h2c_phy_parameter_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PHY_PARAMETER_INFO para_info,
	IN u8 full_fifo
);

static HALMAC_RET_STATUS
halmac_transition_cfg_para_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_CFG_PARA_CMD_CONSTRUCT_STATE dest_state
);

static HALMAC_RET_STATUS
halmac_enqueue_para_buff_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PHY_PARAMETER_INFO para_info,
	IN u8 *pCurr_buff_wptr,
	OUT u8 *pEnd_cmd
);

static HALMAC_RET_STATUS
halmac_gen_cfg_para_h2c_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pH2c_buff
);

static HALMAC_RET_STATUS
halmac_send_h2c_update_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PACKET_ID pkt_id,
	IN u8 *pkt,
	IN u32 pkt_size
);

static HALMAC_RET_STATUS
halmac_send_h2c_update_bcn_parse_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_BCN_IE_INFO pBcn_ie_info
);

static HALMAC_RET_STATUS
halmac_send_h2c_run_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type
);

static HALMAC_RET_STATUS
halmac_send_bt_coex_cmd_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pBt_buf,
	IN u32 bt_size,
	IN u8 ack
);

static HALMAC_RET_STATUS
halmac_func_send_original_h2c_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *original_h2c,
	IN u16 *seq,
	IN u8 ack
);

static HALMAC_RET_STATUS
halmac_buffer_read_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u32 size,
	IN HAL_FIFO_SEL halmac_fifo_sel,
	OUT u8 *pFifo_map
);

static HALMAC_SCAN_CMD_CONSTRUCT_STATE
halmac_query_scan_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_transition_scan_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_SCAN_CMD_CONSTRUCT_STATE dest_state
);

static HALMAC_RET_STATUS
halmac_func_ctrl_ch_switch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_SWITCH_OPTION pCs_option
);

static HALMAC_RET_STATUS
halmac_func_send_general_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_GENERAL_INFO pGeneral_info
);

static HALMAC_RET_STATUS
halmac_func_send_phydm_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_GENERAL_INFO pGeneral_info
);

static HALMAC_RET_STATUS
halmac_func_p2pps_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_P2PPS	pP2PPS
);

static HALMAC_RET_STATUS
halmac_query_cfg_para_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

static HALMAC_RET_STATUS
halmac_query_channel_switch_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

static HALMAC_RET_STATUS
halmac_query_update_packet_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

static HALMAC_RET_STATUS
halmac_pwr_sub_seq_parer_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 cut,
	IN u8 fab,
	IN u8 intf,
	IN PHALMAC_WLAN_PWR_CFG pPwr_sub_seq_cfg
);

/**
 * halmac_ofld_func_cfg_88xx() - config offload function
 * @pHalmac_adapter : the adapter of halmac
 * @pOfld_func_info : offload function information
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_ofld_func_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_OFLD_FUNC_INFO pOfld_func_info
)
{
	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO && pOfld_func_info->rsvd_pg_drv_buf_max_sz > HALMAC_SDIO_TX_PKT_MAX_SIZE_88XX)
		return HALMAC_RET_FAIL;

	pHalmac_adapter->ofld_func_info.halmac_malloc_max_sz = pOfld_func_info->halmac_malloc_max_sz;
	pHalmac_adapter->ofld_func_info.rsvd_pg_drv_buf_max_sz = pOfld_func_info->rsvd_pg_drv_buf_max_sz;

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_dl_drv_rsvd_page_88xx() - download packet to rsvd page
 * @pHalmac_adapter : the adapter of halmac
 * @pg_offset : page offset of driver's rsvd page
 * @halmac_buf : data to be downloaded, tx_desc is not included
 * @halmac_size : data size to be downloaded
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dl_drv_rsvd_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 pg_offset,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS ret_status;
	u16 drv_pg_bndy = 0;
	u32 dl_pg_num = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_dl_drv_rsvd_page_88xx ==========>\n");

	/*check boundary and size valid*/
	dl_pg_num = halmac_size / pHalmac_adapter->hw_config_info.page_size + ((halmac_size & (pHalmac_adapter->hw_config_info.page_size - 1)) ? 1 : 0);
	if (pg_offset + dl_pg_num > pHalmac_adapter->txff_allocation.rsvd_drv_pg_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR] driver download offset or size error ==========>\n");
		return HALMAC_RET_DRV_DL_ERR;
	}

	drv_pg_bndy = pHalmac_adapter->txff_allocation.rsvd_drv_pg_bndy + pg_offset;

	ret_status = halmac_download_rsvd_page_88xx(pHalmac_adapter, drv_pg_bndy, pHalmac_buf, halmac_size);

	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_download_rsvd_page_88xx Fail = %x!!\n", ret_status);
		return ret_status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_dl_drv_rsvd_page_88xx < ==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_download_rsvd_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 pg_addr,
	IN u8 *pHal_buf,
	IN u32 size
)
{
	u8 restore[2];
	u8 value8;
	u32 counter;
	HALMAC_RSVD_PG_STATE *pRsvd_pg_state = &pHalmac_adapter->halmac_state.rsvd_pg_state;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (size == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Rsvd page packet size is zero!!\n");
		return HALMAC_RET_ZERO_LEN_RSVD_PACKET;
	}

	if (*pRsvd_pg_state == HALMAC_RSVD_PG_STATE_BUSY)
		return HALMAC_RET_BUSY_STATE;

	*pRsvd_pg_state = HALMAC_RSVD_PG_STATE_BUSY;

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pg_addr & BIT_MASK_BCN_HEAD_1_V1) | BIT(15));

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR + 1);
	restore[0] = value8;
	value8 = (u8)(value8 | BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, value8);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2);
	restore[1] = value8;
	value8 = (u8)(value8 & ~(BIT(6)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, value8);

	if (PLATFORM_SEND_RSVD_PAGE(pDriver_adapter, pHal_buf, size) == _FALSE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]PLATFORM_SEND_RSVD_PAGE 1 error!!\n");
		status = HALMAC_RET_DL_RSVD_PAGE_FAIL;
		goto DL_RSVD_PG_END;
	}

	/* Check Bcn_Valid_Bit */
	counter = 1000;
	while (!(HALMAC_REG_READ_8(pHalmac_adapter, REG_FIFOPAGE_CTRL_2 + 1) & BIT(7))) {
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 10);
		counter--;
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Polling Bcn_Valid_Fail error!!\n");
			status = HALMAC_RET_POLLING_BCN_VALID_FAIL;
			break;
		}
	}
DL_RSVD_PG_END:
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2,
					(u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCN_HEAD_1_V1) | BIT(15));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, restore[1]);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, restore[0]);

	*pRsvd_pg_state = HALMAC_RSVD_PG_STATE_IDLE;

	return status;
}

HALMAC_RET_STATUS
halmac_get_hw_value_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_HW_ID hw_id,
	OUT VOID *pvalue
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_get_hw_value_88xx ==========>\n");

	switch (hw_id) {
	case HALMAC_HW_RQPN_MAPPING:
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_vo = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VO];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_vi = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VI];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_be = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BE];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_bk = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BK];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_mg = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_MG];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_hi = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI];
		break;
	case HALMAC_HW_EFUSE_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.efuse_size;
		break;
	case HALMAC_HW_EEPROM_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.eeprom_size;
		break;
	case HALMAC_HW_BT_BANK_EFUSE_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.bt_efuse_size;
		break;
	case HALMAC_HW_BT_BANK1_EFUSE_SIZE:
	case HALMAC_HW_BT_BANK2_EFUSE_SIZE:
		*(u32 *)pvalue = 0;
		break;
	case HALMAC_HW_TXFIFO_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.tx_fifo_size;
		break;
	case HALMAC_HW_RXFIFO_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.rx_fifo_size;
		break;
	case HALMAC_HW_RSVD_PG_BNDY:
		*(u16 *)pvalue = pHalmac_adapter->txff_allocation.rsvd_drv_pg_bndy;
		break;
	case HALMAC_HW_CAM_ENTRY_NUM:
		*(u8 *)pvalue = pHalmac_adapter->hw_config_info.cam_entry_num;
		break;
	case HALMAC_HW_WLAN_EFUSE_AVAILABLE_SIZE: /*Remove later*/
		status = halmac_dump_logical_efuse_map_88xx(pHalmac_adapter, HALMAC_EFUSE_R_DRV);
		if (status != HALMAC_RET_SUCCESS)
			return status;
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX - pHalmac_adapter->efuse_end;
		break;
	case HALMAC_HW_IC_VERSION:
		*(u8 *)pvalue = pHalmac_adapter->chip_version;
		break;
	case HALMAC_HW_PAGE_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.page_size;
		break;
	case HALMAC_HW_TX_AGG_ALIGN_SIZE:
		*(u16 *)pvalue = pHalmac_adapter->hw_config_info.tx_align_size;
		break;
	case HALMAC_HW_RX_AGG_ALIGN_SIZE:
		*(u8 *)pvalue = 8;
		break;
	case HALMAC_HW_DRV_INFO_SIZE:
		*(u8 *)pvalue = pHalmac_adapter->drv_info_size;
		break;
	case HALMAC_HW_TXFF_ALLOCATION:
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pvalue, &pHalmac_adapter->txff_allocation, sizeof(HALMAC_TXFF_ALLOCATION));
		break;
	case HALMAC_HW_RSVD_EFUSE_SIZE:
		*(u32 *)pvalue = HALMAC_PROTECTED_EFUSE_SIZE_88XX;
		break;
	case HALMAC_HW_FW_HDR_SIZE:
		*(u32 *)pvalue = HALMAC_FWHDR_SIZE_88XX;
		break;
	case HALMAC_HW_TX_DESC_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.txdesc_size;
		break;
	case HALMAC_HW_RX_DESC_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.rxdesc_size;
		break;
	case HALMAC_HW_FW_MAX_SIZE:
		*(u32 *)pvalue = HALMAC_FW_SIZE_MAX_88XX;
		break;
	case HALMAC_HW_ORI_H2C_SIZE:
		*(u32 *)pvalue = HALMAC_H2C_CMD_ORIGINAL_SIZE_88XX;
		break;
	case HALMAC_HW_RSVD_DRV_PGNUM:
		*(u16 *)pvalue = pHalmac_adapter->txff_allocation.rsvd_drv_pg_num;
		break;
	case HALMAC_HW_TX_PAGE_SIZE:
		*(u16 *)pvalue = HALMAC_TX_PAGE_SIZE_88XX;
		break;
	case HALMAC_HW_USB_TXAGG_DESC_NUM:
		*(u8 *)pvalue = pHalmac_adapter->hw_config_info.usb_txagg_num;
		break;
	case HALMAC_HW_AC_OQT_SIZE:
		*(u8 *)pvalue = pHalmac_adapter->hw_config_info.ac_oqt_size;
		break;
	case HALMAC_HW_NON_AC_OQT_SIZE:
		*(u8 *)pvalue = pHalmac_adapter->hw_config_info.non_ac_oqt_size;
		break;
	case HALMAC_HW_AC_QUEUE_NUM:
		*(u8 *)pvalue = pHalmac_adapter->hw_config_info.ac_queue_num;
		break;
	default:
		return HALMAC_RET_PARA_NOT_SUPPORT;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_get_hw_value_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_set_hw_value_88xx() -set hw config value
 * @pHalmac_adapter : the adapter of halmac
 * @hw_id : hw id for driver to config
 * @pvalue : hw value, reference table to get data type
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_set_hw_value_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_HW_ID hw_id,
	IN VOID *pvalue
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_set_hw_value_88xx ==========>\n");

	if (pvalue == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_set_hw_value_88xx (NULL == pvalue)\n");
		return HALMAC_RET_NULL_POINTER;
	}

	switch (hw_id) {
	case HALMAC_HW_USB_MODE:
		status = halmac_set_usb_mode_88xx(pHalmac_adapter, *(HALMAC_USB_MODE *)pvalue);
		if (status != HALMAC_RET_SUCCESS)
			return status;
		break;
	case HALMAC_HW_SEQ_EN:
		/*if (_TRUE == hw_seq_en) {
		} else {
		}*/
		break;
	case HALMAC_HW_BANDWIDTH:
		halmac_cfg_bw_88xx(pHalmac_adapter, *(HALMAC_BW *)pvalue);
		break;
	case HALMAC_HW_CHANNEL:
		halmac_cfg_ch_88xx(pHalmac_adapter, *(u8 *)pvalue);
		break;
	case HALMAC_HW_PRI_CHANNEL_IDX:
		halmac_cfg_pri_ch_idx_88xx(pHalmac_adapter, *(HALMAC_PRI_CH_IDX *)pvalue);
		break;
	case HALMAC_HW_EN_BB_RF:
		halmac_enable_bb_rf_88xx(pHalmac_adapter, *(u8 *)pvalue);
		break;
	case HALMAC_HW_SDIO_TX_PAGE_THRESHOLD:
		halmac_config_sdio_tx_page_threshold_88xx(pHalmac_adapter, (PHALMAC_TX_PAGE_THRESHOLD_INFO)pvalue);
		break;
	case HALMAC_HW_AMPDU_CONFIG:
		halmac_config_ampdu_88xx(pHalmac_adapter, (PHALMAC_AMPDU_CONFIG)pvalue);
		break;
	case HALMAC_HW_RX_SHIFT:
		halmac_rx_shift_88xx(pHalmac_adapter, *(u8 *)pvalue);
		break;
	case HALMAC_HW_TXDESC_CHECKSUM:
		halmac_tx_desc_checksum_88xx(pHalmac_adapter, *(u8 *)pvalue);
		break;
	case HALMAC_HW_RX_CLK_GATE:
		halmac_rx_clk_gate_88xx(pHalmac_adapter, *(u8 *)pvalue);
		break;
	default:
		return HALMAC_RET_PARA_NOT_SUPPORT;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_set_hw_value_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_set_fw_offload_h2c_header_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u8 *pHal_h2c_hdr,
	IN PHALMAC_H2C_HEADER_INFO pH2c_header_info,
	OUT u16 *pSeq_num
)
{
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_set_fw_offload_h2c_header_88xx!!\n");

	FW_OFFLOAD_H2C_SET_TOTAL_LEN(pHal_h2c_hdr, 8 + pH2c_header_info->content_size);
	FW_OFFLOAD_H2C_SET_SUB_CMD_ID(pHal_h2c_hdr, pH2c_header_info->sub_cmd_id);

	FW_OFFLOAD_H2C_SET_CATEGORY(pHal_h2c_hdr, 0x01);
	FW_OFFLOAD_H2C_SET_CMD_ID(pHal_h2c_hdr, 0xFF);

	PLATFORM_MUTEX_LOCK(pDriver_adapter, &pHalmac_adapter->h2c_seq_mutex);
	FW_OFFLOAD_H2C_SET_SEQ_NUM(pHal_h2c_hdr, pHalmac_adapter->h2c_packet_seq);
	*pSeq_num = pHalmac_adapter->h2c_packet_seq;
	pHalmac_adapter->h2c_packet_seq++;
	PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &pHalmac_adapter->h2c_seq_mutex);

	if (pH2c_header_info->ack == _TRUE)
		FW_OFFLOAD_H2C_SET_ACK(pHal_h2c_hdr, _TRUE);

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_send_h2c_pkt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHal_h2c_cmd,
	IN u32 size,
	IN u8 ack
)
{
	u32 counter = 100;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	while (pHalmac_adapter->h2c_buf_free_space <= HALMAC_H2C_CMD_SIZE_88XX) {
		halmac_get_h2c_buff_free_space_88xx(pHalmac_adapter);
		counter--;
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]h2c free space is not enough!!\n");
			return HALMAC_RET_H2C_SPACE_FULL;
		}
	}

	/* Send TxDesc + H2C_CMD */
	counter = 100;
	do {
		if (PLATFORM_SEND_H2C_PKT(pDriver_adapter, pHal_h2c_cmd, size) == _TRUE)
			break;

		counter--;
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Send H2C_CMD pkt error!!\n");
			return HALMAC_RET_SEND_H2C_FAIL;
		}
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 5);

	} while (1);

	pHalmac_adapter->h2c_buf_free_space -= HALMAC_H2C_CMD_SIZE_88XX;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]H2C free space : %d\n", pHalmac_adapter->h2c_buf_free_space);

	return status;
}

HALMAC_RET_STATUS
halmac_get_h2c_buff_free_space_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u32 hw_wptr, fw_rptr;
	PHALMAC_API pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	hw_wptr = HALMAC_REG_READ_32(pHalmac_adapter, REG_H2C_PKT_WRITEADDR) & BIT_MASK_H2C_WR_ADDR;
	fw_rptr = HALMAC_REG_READ_32(pHalmac_adapter, REG_H2C_PKT_READADDR) & BIT_MASK_H2C_READ_ADDR;

	if (hw_wptr >= fw_rptr)
		pHalmac_adapter->h2c_buf_free_space = pHalmac_adapter->h2c_buff_size - (hw_wptr - fw_rptr);
	else
		pHalmac_adapter->h2c_buf_free_space = fw_rptr - hw_wptr;

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_c2h_info_88xx() - process halmac C2H packet
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_buf : RX Packet pointer
 * @halmac_size : RX Packet size
 * Author : KaiYuan Chang/Ivan Lin
 *
 * Used to process c2h packet info from RX path. After receiving the packet,
 * user need to call this api and pass the packet pointer.
 *
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_c2h_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;


	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	/* Check if it is C2H packet */
	if (GET_RX_DESC_C2H(halmac_buf) == _TRUE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]C2H packet, start parsing!\n");

		status = halmac_parse_c2h_packet_88xx(pHalmac_adapter, halmac_buf, halmac_size);

		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_parse_c2h_packet_88xx error = %x\n", status);
			return status;
		}
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_parse_c2h_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size
)
{
	u8 c2h_cmd, c2h_sub_cmd_id;
	u8 *pC2h_buf = halmac_buf + pHalmac_adapter->hw_config_info.rxdesc_size;
	u32 c2h_size = halmac_size - pHalmac_adapter->hw_config_info.rxdesc_size;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_parse_c2h_packet_88xx!!\n"); */

	c2h_cmd = (u8)C2H_HDR_GET_CMD_ID(pC2h_buf);

	/* FW offload C2H cmd is 0xFF */
	if (c2h_cmd != 0xFF) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]C2H_PKT not for FwOffloadC2HFormat!!\n");
		return HALMAC_RET_C2H_NOT_HANDLED;
	}

	/* Get C2H sub cmd ID */
	c2h_sub_cmd_id = (u8)C2H_HDR_GET_C2H_SUB_CMD_ID(pC2h_buf);

	switch (c2h_sub_cmd_id) {
	case C2H_SUB_CMD_ID_C2H_DBG:
		status = halmac_parse_c2h_debug_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
		break;
	case C2H_SUB_CMD_ID_H2C_ACK_HDR:
		status = halmac_parse_h2c_ack_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
		break;
	case C2H_SUB_CMD_ID_BT_COEX_INFO:
		status = HALMAC_RET_C2H_NOT_HANDLED;
		break;
	case C2H_SUB_CMD_ID_SCAN_STATUS_RPT:
		status = halmac_parse_scan_status_rpt_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
		break;
	case C2H_SUB_CMD_ID_PSD_DATA:
		status = halmac_parse_psd_data_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
		break;

	case C2H_SUB_CMD_ID_EFUSE_DATA:
		status = halmac_parse_efuse_data_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_WARN, "[ERR]c2h_sub_cmd_id switch case out of boundary!!\n");
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_WARN, "[ERR]c2h pkt : %.8X %.8X!!\n", *(u32 *)pC2h_buf, *(u32 *)(pC2h_buf + 4));
		status = HALMAC_RET_C2H_NOT_HANDLED;
		break;
	}

	return status;
}

static HALMAC_RET_STATUS
halmac_parse_c2h_debug_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	VOID *pDriver_adapter = NULL;
	u8 i;
	u8 next_msg_offset = 0;
	u8 curr_msg_offset = 0;
	u8 message_length = 0;
	char *pC2h_buf_local = (char *)NULL;
	u8 dbg_content_length = 0;
	u8 dbg_seq_num = 0;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	/* total length of C2H in header byte 3 */
	dbg_content_length = (u8)C2H_HDR_GET_LEN((u8 *)pC2h_buf);

	if (dbg_content_length > C2H_DBG_CONTENT_MAX_LENGTH) {
		PLATFORM_MSG_PRINT(pDriver_adapter,  HALMAC_MSG_H2C,  HALMAC_DBG_ERR,  "[ERR]C2H size > DBG max length!\n");

		return HALMAC_RET_C2H_NOT_HANDLED;
	}

	for (i = 0; i < dbg_content_length; i++) {
		/* find the start of the 2nd message in aggregrative c2h */
		if (*(pC2h_buf + C2H_DBG_HEADER_LENGTH + i) == '\n') {
			if ((*(pC2h_buf + C2H_DBG_HEADER_LENGTH + i + 1) == '\0') || (*(pC2h_buf + C2H_DBG_HEADER_LENGTH + i + 1) == 0xff)) {
				next_msg_offset = C2H_DBG_HEADER_LENGTH + i + 1;
				goto _ENDFOUND;
			}
		}
	}

_ENDFOUND:

	message_length = next_msg_offset - C2H_DBG_HEADER_LENGTH;
	pC2h_buf_local = (char *)PLATFORM_RTL_MALLOC(pDriver_adapter, message_length);
	if (pC2h_buf_local == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac allocate dbg message buffer Fail!!\n");
		return HALMAC_RET_MALLOC_FAIL;
	}

	/* Copy message from content[0] */
	PLATFORM_RTL_MEMCPY(pDriver_adapter, pC2h_buf_local, pC2h_buf + C2H_DBG_HEADER_LENGTH, message_length);

	/* C2H content[0] = sequence number, message start from content[1] */
	dbg_seq_num = (u8)(*(pC2h_buf_local));
	*(pC2h_buf_local + message_length - 1) = '\0';
	PLATFORM_MSG_PRINT(pDriver_adapter,  HALMAC_MSG_H2C,  HALMAC_DBG_ALWAYS,  "[RTKFW, SEQ=%d]: %s\n",  dbg_seq_num,  (char *)(pC2h_buf_local + 1));
	PLATFORM_RTL_FREE(pDriver_adapter, pC2h_buf_local, message_length);


	/* next_msg_offset = index of the header[0] of next message */
	while (*(pC2h_buf + next_msg_offset) != '\0') {
		curr_msg_offset = next_msg_offset;

		/* next_msg_offset = start of header of next C2H */
		message_length = (u8)(*(pC2h_buf + curr_msg_offset + 3)) - 1;
		next_msg_offset += C2H_DBG_HEADER_LENGTH + message_length;

		pC2h_buf_local = (char *)PLATFORM_RTL_MALLOC(pDriver_adapter, message_length);
		if (pC2h_buf_local == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac allocate dbg message buffer Fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
		}

		/* Copy message from content[0] */
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pC2h_buf_local, pC2h_buf + curr_msg_offset + C2H_DBG_HEADER_LENGTH, message_length);
		*(pC2h_buf_local + message_length - 1) = '\0';
		dbg_seq_num = (u8)(*(pC2h_buf_local));
		PLATFORM_MSG_PRINT(pDriver_adapter,  HALMAC_MSG_H2C,  HALMAC_DBG_ALWAYS,  "[RTKFW, SEQ=%d]: %s\n",  dbg_seq_num,	(char *)(pC2h_buf_local + 1));
		PLATFORM_RTL_FREE(pDriver_adapter, pC2h_buf_local, message_length);
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 h2c_cmd_id, h2c_sub_cmd_id;
	u8 h2c_return_code;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;


	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Ack for C2H!!\n");

	h2c_return_code = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(pC2h_buf);
	if (HALMAC_H2C_RETURN_SUCCESS != (HALMAC_H2C_RETURN_CODE)h2c_return_code)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]C2H_PKT Status Error!! Status = %d\n", h2c_return_code);

	h2c_cmd_id = (u8)H2C_ACK_HDR_GET_H2C_CMD_ID(pC2h_buf);

	if (h2c_cmd_id != 0xFF) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]original h2c ack is not handled!!\n");
		status = HALMAC_RET_C2H_NOT_HANDLED;
	} else {
		h2c_sub_cmd_id = (u8)H2C_ACK_HDR_GET_H2C_SUB_CMD_ID(pC2h_buf);

		switch (h2c_sub_cmd_id) {
		case H2C_SUB_CMD_ID_DUMP_PHYSICAL_EFUSE_ACK:
			status = halmac_parse_h2c_ack_phy_efuse_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
			break;
		case H2C_SUB_CMD_ID_CFG_PARAMETER_ACK:
			status = halmac_parse_h2c_ack_cfg_para_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
			break;
		case H2C_SUB_CMD_ID_UPDATE_PACKET_ACK:
			status = halmac_parse_h2c_ack_update_packet_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
			break;
		case H2C_SUB_CMD_ID_UPDATE_DATAPACK_ACK:
			status = halmac_parse_h2c_ack_update_datapack_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
			break;
		case H2C_SUB_CMD_ID_RUN_DATAPACK_ACK:
			status = halmac_parse_h2c_ack_run_datapack_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
			break;
		case H2C_SUB_CMD_ID_CHANNEL_SWITCH_ACK:
			status = halmac_parse_h2c_ack_channel_switch_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
			break;
		case H2C_SUB_CMD_ID_IQK_ACK:
			status = halmac_parse_h2c_ack_iqk_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
			break;
		case H2C_SUB_CMD_ID_POWER_TRACKING_ACK:
			status = halmac_parse_h2c_ack_power_tracking_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
			break;
		case H2C_SUB_CMD_ID_PSD_ACK:
			break;
		case H2C_SUB_CMD_ID_FW_SNDING_ACK:
			status = halmac_parse_h2c_ack_fw_snding_88xx(pHalmac_adapter, pC2h_buf, c2h_size);
			break;
		default:
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_WARN, "[ERR]h2c_sub_cmd_id switch case out of boundary!!\n");
			status = HALMAC_RET_C2H_NOT_HANDLED;
			break;
		}
	}

	return status;
}

static HALMAC_RET_STATUS
halmac_parse_scan_status_rpt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 h2c_return_code;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status;

	h2c_return_code = (u8)SCAN_STATUS_RPT_GET_H2C_RETURN_CODE(pC2h_buf);
	process_status = (HALMAC_H2C_RETURN_SUCCESS == (HALMAC_H2C_RETURN_CODE)h2c_return_code) ? HALMAC_CMD_PROCESS_DONE : HALMAC_CMD_PROCESS_ERROR;

	PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_CHANNEL_SWITCH, process_status, NULL, 0);

	pHalmac_adapter->halmac_state.scan_state_set.process_status = process_status;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]scan status : %X\n", process_status);

	return HALMAC_RET_SUCCESS;
}


static HALMAC_RET_STATUS
halmac_parse_psd_data_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 segment_id = 0, segment_size = 0, h2c_seq = 0;
	u16 total_size;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status;
	PHALMAC_PSD_STATE_SET pPsd_set = &pHalmac_adapter->halmac_state.psd_set;

	h2c_seq = (u8)PSD_DATA_GET_H2C_SEQ(pC2h_buf);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Seq num : h2c -> %d c2h -> %d\n", pPsd_set->seq_num, h2c_seq);
	if (h2c_seq != pPsd_set->seq_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Seq num mismactch : h2c -> %d c2h -> %d\n", pPsd_set->seq_num, h2c_seq);
		return HALMAC_RET_SUCCESS;
	}

	if (pPsd_set->process_status != HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not in HALMAC_CMD_PROCESS_SENDING\n");
		return HALMAC_RET_SUCCESS;
	}

	total_size = (u16)PSD_DATA_GET_TOTAL_SIZE(pC2h_buf);
	segment_id = (u8)PSD_DATA_GET_SEGMENT_ID(pC2h_buf);
	segment_size = (u8)PSD_DATA_GET_SEGMENT_SIZE(pC2h_buf);
	pPsd_set->data_size = total_size;

	if (pPsd_set->pData == NULL)
		pPsd_set->pData = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, pPsd_set->data_size);

	if (segment_id == 0)
		pPsd_set->segment_size = segment_size;

	PLATFORM_RTL_MEMCPY(pDriver_adapter, pPsd_set->pData + segment_id * pPsd_set->segment_size, pC2h_buf + HALMAC_C2H_DATA_OFFSET_88XX, segment_size);

	if (PSD_DATA_GET_END_SEGMENT(pC2h_buf) == _FALSE)
		return HALMAC_RET_SUCCESS;

	process_status = HALMAC_CMD_PROCESS_DONE;
	pPsd_set->process_status = process_status;

	PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_PSD, process_status, pPsd_set->pData, pPsd_set->data_size);

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_cfg_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 h2c_seq = 0;
	u8 h2c_return_code;
	u32 offset_accu = 0, value_accu = 0;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status = HALMAC_CMD_PROCESS_UNDEFINE;

	h2c_seq = (u8)H2C_ACK_HDR_GET_H2C_SEQ(pC2h_buf);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Seq num : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.cfg_para_state_set.seq_num, h2c_seq);
	if (h2c_seq != pHalmac_adapter->halmac_state.cfg_para_state_set.seq_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Seq num mismactch : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.cfg_para_state_set.seq_num, h2c_seq);
		return HALMAC_RET_SUCCESS;
	}

	if (pHalmac_adapter->halmac_state.cfg_para_state_set.process_status != HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not in HALMAC_CMD_PROCESS_SENDING\n");
		return HALMAC_RET_SUCCESS;
	}

	h2c_return_code = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(pC2h_buf);
	pHalmac_adapter->halmac_state.cfg_para_state_set.fw_return_code = h2c_return_code;
	offset_accu = CFG_PARAMETER_ACK_GET_OFFSET_ACCUMULATION(pC2h_buf);
	value_accu = CFG_PARAMETER_ACK_GET_VALUE_ACCUMULATION(pC2h_buf);

	if ((offset_accu != pHalmac_adapter->config_para_info.offset_accumulation) || (value_accu != pHalmac_adapter->config_para_info.value_accumulation)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR][C2H]offset_accu : %x, value_accu : %x!!\n", offset_accu, value_accu);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR][Adapter]offset_accu : %x, value_accu : %x!!\n", pHalmac_adapter->config_para_info.offset_accumulation, pHalmac_adapter->config_para_info.value_accumulation);
		process_status = HALMAC_CMD_PROCESS_ERROR;
	}

	if (((HALMAC_H2C_RETURN_CODE)h2c_return_code == HALMAC_H2C_RETURN_SUCCESS) && (process_status != HALMAC_CMD_PROCESS_ERROR)) {
		process_status = HALMAC_CMD_PROCESS_DONE;
		pHalmac_adapter->halmac_state.cfg_para_state_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_CFG_PARA, process_status, NULL, 0);
	} else {
		process_status = HALMAC_CMD_PROCESS_ERROR;
		pHalmac_adapter->halmac_state.cfg_para_state_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_CFG_PARA, process_status, &pHalmac_adapter->halmac_state.cfg_para_state_set.fw_return_code, 1);
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_update_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 h2c_seq = 0;
	u8 h2c_return_code;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status;

	h2c_seq = (u8)H2C_ACK_HDR_GET_H2C_SEQ(pC2h_buf);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Seq num : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.update_packet_set.seq_num, h2c_seq);
	if (h2c_seq != pHalmac_adapter->halmac_state.update_packet_set.seq_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Seq num mismactch : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.update_packet_set.seq_num, h2c_seq);
		return HALMAC_RET_SUCCESS;
	}

	if (pHalmac_adapter->halmac_state.update_packet_set.process_status != HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not in HALMAC_CMD_PROCESS_SENDING\n");
		return HALMAC_RET_SUCCESS;
	}

	h2c_return_code = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(pC2h_buf);
	pHalmac_adapter->halmac_state.update_packet_set.fw_return_code = h2c_return_code;

	if (HALMAC_H2C_RETURN_SUCCESS == (HALMAC_H2C_RETURN_CODE)h2c_return_code) {
		process_status = HALMAC_CMD_PROCESS_DONE;
		pHalmac_adapter->halmac_state.update_packet_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_UPDATE_PACKET, process_status, NULL, 0);
	} else {
		process_status = HALMAC_CMD_PROCESS_ERROR;
		pHalmac_adapter->halmac_state.update_packet_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_UPDATE_PACKET, process_status, &pHalmac_adapter->halmac_state.update_packet_set.fw_return_code, 1);
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_update_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status = HALMAC_CMD_PROCESS_UNDEFINE;

	PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_UPDATE_DATAPACK, process_status, NULL, 0);

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_run_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status = HALMAC_CMD_PROCESS_UNDEFINE;

	PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_RUN_DATAPACK, process_status, NULL, 0);

	return HALMAC_RET_SUCCESS;
}


static HALMAC_RET_STATUS
halmac_parse_h2c_ack_channel_switch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 h2c_seq = 0;
	u8 h2c_return_code;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status;

	h2c_seq = (u8)H2C_ACK_HDR_GET_H2C_SEQ(pC2h_buf);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Seq num : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.scan_state_set.seq_num, h2c_seq);
	if (h2c_seq != pHalmac_adapter->halmac_state.scan_state_set.seq_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Seq num mismactch : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.scan_state_set.seq_num, h2c_seq);
		return HALMAC_RET_SUCCESS;
	}

	if (pHalmac_adapter->halmac_state.scan_state_set.process_status != HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not in HALMAC_CMD_PROCESS_SENDING\n");
		return HALMAC_RET_SUCCESS;
	}

	h2c_return_code = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(pC2h_buf);
	pHalmac_adapter->halmac_state.scan_state_set.fw_return_code = h2c_return_code;

	if ((HALMAC_H2C_RETURN_CODE)h2c_return_code == HALMAC_H2C_RETURN_SUCCESS) {
		process_status = HALMAC_CMD_PROCESS_RCVD;
		pHalmac_adapter->halmac_state.scan_state_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_CHANNEL_SWITCH, process_status, NULL, 0);
	} else {
		process_status = HALMAC_CMD_PROCESS_ERROR;
		pHalmac_adapter->halmac_state.scan_state_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_CHANNEL_SWITCH, process_status, &pHalmac_adapter->halmac_state.scan_state_set.fw_return_code, 1);
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_iqk_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 h2c_seq = 0;
	u8 h2c_return_code;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status;

	h2c_seq = (u8)H2C_ACK_HDR_GET_H2C_SEQ(pC2h_buf);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Seq num : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.iqk_set.seq_num, h2c_seq);
	if (h2c_seq != pHalmac_adapter->halmac_state.iqk_set.seq_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Seq num mismactch : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.iqk_set.seq_num, h2c_seq);
		return HALMAC_RET_SUCCESS;
	}

	if (pHalmac_adapter->halmac_state.iqk_set.process_status != HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not in HALMAC_CMD_PROCESS_SENDING\n");
		return HALMAC_RET_SUCCESS;
	}

	h2c_return_code = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(pC2h_buf);
	pHalmac_adapter->halmac_state.iqk_set.fw_return_code = h2c_return_code;

	if ((HALMAC_H2C_RETURN_CODE)h2c_return_code == HALMAC_H2C_RETURN_SUCCESS) {
		process_status = HALMAC_CMD_PROCESS_DONE;
		pHalmac_adapter->halmac_state.iqk_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_IQK, process_status, NULL, 0);
	} else {
		process_status = HALMAC_CMD_PROCESS_ERROR;
		pHalmac_adapter->halmac_state.iqk_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_IQK, process_status, &pHalmac_adapter->halmac_state.iqk_set.fw_return_code, 1);
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_parse_h2c_ack_power_tracking_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 h2c_seq = 0;
	u8 h2c_return_code;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status;

	h2c_seq = (u8)H2C_ACK_HDR_GET_H2C_SEQ(pC2h_buf);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Seq num : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.power_tracking_set.seq_num, h2c_seq);
	if (h2c_seq != pHalmac_adapter->halmac_state.power_tracking_set.seq_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Seq num mismactch : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.power_tracking_set.seq_num, h2c_seq);
		return HALMAC_RET_SUCCESS;
	}

	if (pHalmac_adapter->halmac_state.power_tracking_set.process_status != HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not in HALMAC_CMD_PROCESS_SENDING\n");
		return HALMAC_RET_SUCCESS;
	}

	h2c_return_code = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(pC2h_buf);
	pHalmac_adapter->halmac_state.power_tracking_set.fw_return_code = h2c_return_code;

	if ((HALMAC_H2C_RETURN_CODE)h2c_return_code == HALMAC_H2C_RETURN_SUCCESS) {
		process_status = HALMAC_CMD_PROCESS_DONE;
		pHalmac_adapter->halmac_state.power_tracking_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_POWER_TRACKING, process_status, NULL, 0);
	} else {
		process_status = HALMAC_CMD_PROCESS_ERROR;
		pHalmac_adapter->halmac_state.power_tracking_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_POWER_TRACKING, process_status, &pHalmac_adapter->halmac_state.power_tracking_set.fw_return_code, 1);
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_debug_88xx() - dump information for debugging
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_debug_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 temp8 = 0;
	u32 i = 0, temp32 = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug_88xx ==========>\n");

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		/* Dump CCCR, it needs new platform api */

		/*Dump SDIO Local Register, use CMD52*/
		for (i = 0x10250000; i < 0x102500ff; i++) {
			temp8 = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, i);
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: sdio[%x]=%x\n", i, temp8);
		}

		/*Dump MAC Register*/
		for (i = 0x0000; i < 0x17ff; i++) {
			temp8 = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, i);
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp8);
		}

		/*Check RX Fifo status*/
		i = REG_RXFF_PTR_V1;
		temp8 = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp8);
		i = REG_RXFF_WTR_V1;
		temp8 = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp8);
		i = REG_RXFF_PTR_V1;
		temp8 = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp8);
		i = REG_RXFF_WTR_V1;
		temp8 = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp8);
	} else {
		/*Dump MAC Register*/
		for (i = 0x0000; i < 0x17fc; i += 4) {
			temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp32);
		}

		/*Check RX Fifo status*/
		i = REG_RXFF_PTR_V1;
		temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp32);
		i = REG_RXFF_WTR_V1;
		temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp32);
		i = REG_RXFF_PTR_V1;
		temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp32);
		i = REG_RXFF_WTR_V1;
		temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug: mac[%x]=%x\n", i, temp32);
	}

	/*	TODO: Add check register code, including MAC CLK, CPU CLK */

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_debug_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_parameter_88xx() - config parameter by FW
 * @pHalmac_adapter : the adapter of halmac
 * @para_info : cmd id, content
 * @full_fifo : parameter information
 *
 * If msk_en = _TRUE, the format of array is {reg_info, mask, value}.
 * If msk_en =_FAUSE, the format of array is {reg_info, value}
 * The format of reg_info is
 * reg_info[31]=rf_reg, 0: MAC_BB reg, 1: RF reg
 * reg_info[27:24]=rf_path, 0: path_A, 1: path_B
 * if rf_reg=0(MAC_BB reg), rf_path is meaningless.
 * ref_info[15:0]=offset
 *
 * Example: msk_en = _FALSE
 * {0x8100000a, 0x00001122}
 * =>Set RF register, path_B, offset 0xA to 0x00001122
 * {0x00000824, 0x11224433}
 * =>Set MAC_BB register, offset 0x800 to 0x11224433
 *
 * Note : full fifo mode only for init flow
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_parameter_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PHY_PARAMETER_INFO para_info,
	IN u8 full_fifo
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.cfg_para_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_parameter_88xx ==========>\n"); */

	if (pHalmac_adapter->halmac_state.dlfw_state == HALMAC_DLFW_NONE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_cfg_parameter_88xx Fail due to DLFW NONE!!\n");
		return HALMAC_RET_DLFW_FAIL;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Wait event(cfg para)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if ((halmac_query_cfg_para_curr_state_88xx(pHalmac_adapter) != HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE) &&
	    (halmac_query_cfg_para_curr_state_88xx(pHalmac_adapter) != HALMAC_CFG_PARA_CMD_CONSTRUCT_CONSTRUCTING)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Not idle state(cfg para)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	*pProcess_status = HALMAC_CMD_PROCESS_IDLE;

	ret_status = halmac_send_h2c_phy_parameter_88xx(pHalmac_adapter, para_info, full_fifo);

	if ((ret_status != HALMAC_RET_SUCCESS) && (ret_status != HALMAC_RET_PARA_SENDING)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_phy_parameter_88xx Fail!! = %x\n", ret_status);
		return ret_status;
	}

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_parameter_88xx <==========\n"); */

	return ret_status;
}

static HALMAC_CFG_PARA_CMD_CONSTRUCT_STATE
halmac_query_cfg_para_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	return pHalmac_adapter->halmac_state.cfg_para_state_set.cfg_para_cmd_construct_state;
}

static HALMAC_RET_STATUS
halmac_send_h2c_phy_parameter_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PHY_PARAMETER_INFO para_info,
	IN u8 full_fifo
)
{
	u8 drv_trigger_send = _FALSE;
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 rsvd_pg_addr;
	u16 h2c_seq_mum = 0;
	u32 info_size = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	PHALMAC_CONFIG_PARA_INFO pConfig_para_info;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;
	pConfig_para_info = &pHalmac_adapter->config_para_info;

	if (pConfig_para_info->pCfg_para_buf == NULL) {
		if (full_fifo == _TRUE)
			pConfig_para_info->para_buf_size = pHalmac_adapter->ofld_func_info.halmac_malloc_max_sz;
		else
			pConfig_para_info->para_buf_size = HALMAC_CFG_PARA_RSVDPG_SZ_88XX;

		if (pConfig_para_info->para_buf_size > pHalmac_adapter->ofld_func_info.rsvd_pg_drv_buf_max_sz)
			pConfig_para_info->para_buf_size = pHalmac_adapter->ofld_func_info.rsvd_pg_drv_buf_max_sz;

		pConfig_para_info->pCfg_para_buf = halmac_adaptive_malloc_88xx(pHalmac_adapter,
													pConfig_para_info->para_buf_size, &pConfig_para_info->para_buf_size);
		if (pConfig_para_info->pCfg_para_buf != NULL) {
			PLATFORM_RTL_MEMSET(pDriver_adapter, pConfig_para_info->pCfg_para_buf, 0x00, pConfig_para_info->para_buf_size);
			pConfig_para_info->full_fifo_mode = full_fifo;
			pConfig_para_info->pPara_buf_w = pConfig_para_info->pCfg_para_buf;
			pConfig_para_info->para_num = 0;
			pConfig_para_info->avai_para_buf_size = pConfig_para_info->para_buf_size;
			pConfig_para_info->value_accumulation = 0;
			pConfig_para_info->offset_accumulation = 0;
		} else {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Allocate pCfg_para_buf fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
		}
	}

	if (halmac_transition_cfg_para_state_88xx(pHalmac_adapter, HALMAC_CFG_PARA_CMD_CONSTRUCT_CONSTRUCTING) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	halmac_enqueue_para_buff_88xx(pHalmac_adapter, para_info, pConfig_para_info->pPara_buf_w, &drv_trigger_send);

	if (para_info->cmd_id != HALMAC_PARAMETER_CMD_END) {
		pConfig_para_info->para_num++;
		pConfig_para_info->pPara_buf_w += HALMAC_FW_OFFLOAD_CMD_SIZE_88XX;
		pConfig_para_info->avai_para_buf_size = pConfig_para_info->avai_para_buf_size - HALMAC_FW_OFFLOAD_CMD_SIZE_88XX;
	}

	if (((pConfig_para_info->avai_para_buf_size - pHalmac_adapter->hw_config_info.txdesc_size) > HALMAC_FW_OFFLOAD_CMD_SIZE_88XX) &&
	    (drv_trigger_send == _FALSE)) {
		return HALMAC_RET_SUCCESS;
	}

	if (pConfig_para_info->para_num == 0) {
		PLATFORM_RTL_FREE(pDriver_adapter, pConfig_para_info->pCfg_para_buf, pConfig_para_info->para_buf_size);
		pConfig_para_info->pCfg_para_buf = NULL;
		pConfig_para_info->pPara_buf_w = NULL;
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]no cfg parameter element!!\n");

		pHalmac_adapter->halmac_state.cfg_para_state_set.process_status = HALMAC_CMD_PROCESS_DONE;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_CFG_PARA, HALMAC_CMD_PROCESS_DONE, NULL, 0);

		if (halmac_transition_cfg_para_state_88xx(pHalmac_adapter, HALMAC_CFG_PARA_CMD_CONSTRUCT_H2C_SENT) != HALMAC_RET_SUCCESS)
			return HALMAC_RET_ERROR_STATE;

		if (halmac_transition_cfg_para_state_88xx(pHalmac_adapter, HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
			return HALMAC_RET_ERROR_STATE;

		return HALMAC_RET_SUCCESS;
	}

	if (halmac_transition_cfg_para_state_88xx(pHalmac_adapter, HALMAC_CFG_PARA_CMD_CONSTRUCT_H2C_SENT) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	pHalmac_adapter->halmac_state.cfg_para_state_set.process_status = HALMAC_CMD_PROCESS_SENDING;

	if (pConfig_para_info->full_fifo_mode == _TRUE)
		rsvd_pg_addr = 0;
	else
		rsvd_pg_addr = pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy;

	info_size = pConfig_para_info->para_num * HALMAC_FW_OFFLOAD_CMD_SIZE_88XX;

	status = halmac_download_rsvd_page_88xx(pHalmac_adapter, rsvd_pg_addr, (u8 *)pConfig_para_info->pCfg_para_buf, info_size);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_download_rsvd_page_88xx Fail!!\n");
	} else {
		halmac_gen_cfg_para_h2c_88xx(pHalmac_adapter, pH2c_buff);

		h2c_header_info.sub_cmd_id = SUB_CMD_ID_CFG_PARAMETER;
		h2c_header_info.content_size = 4;
		h2c_header_info.ack = _TRUE;
		halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

		pHalmac_adapter->halmac_state.cfg_para_state_set.seq_num = h2c_seq_mum;

		status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail!!\n");
			halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_CFG_PARA);
		}
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]config parameter time = %d\n", HALMAC_REG_READ_32(pHalmac_adapter, REG_FW_DBG6));
	}

	PLATFORM_RTL_FREE(pDriver_adapter, pConfig_para_info->pCfg_para_buf, pConfig_para_info->para_buf_size);
	pConfig_para_info->pCfg_para_buf = NULL;
	pConfig_para_info->pPara_buf_w = NULL;

	if (halmac_transition_cfg_para_state_88xx(pHalmac_adapter, HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	if (drv_trigger_send == _FALSE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Buffer full trigger sending H2C!!\n");
		return HALMAC_RET_PARA_SENDING;
	}

	return status;
}

static HALMAC_RET_STATUS
halmac_transition_cfg_para_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_CFG_PARA_CMD_CONSTRUCT_STATE dest_state
)
{
	PHALMAC_CFG_PARA_STATE_SET pCfg_para = &pHalmac_adapter->halmac_state.cfg_para_state_set;

	if ((pCfg_para->cfg_para_cmd_construct_state != HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE) &&
	    (pCfg_para->cfg_para_cmd_construct_state != HALMAC_CFG_PARA_CMD_CONSTRUCT_CONSTRUCTING) &&
	    (pCfg_para->cfg_para_cmd_construct_state != HALMAC_CFG_PARA_CMD_CONSTRUCT_H2C_SENT))
		return HALMAC_RET_ERROR_STATE;

	if (dest_state == HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE) {
		if (pCfg_para->cfg_para_cmd_construct_state == HALMAC_CFG_PARA_CMD_CONSTRUCT_CONSTRUCTING)
			return HALMAC_RET_ERROR_STATE;
	} else if (dest_state == HALMAC_CFG_PARA_CMD_CONSTRUCT_CONSTRUCTING) {
		if (pCfg_para->cfg_para_cmd_construct_state == HALMAC_CFG_PARA_CMD_CONSTRUCT_H2C_SENT)
			return HALMAC_RET_ERROR_STATE;
	} else if (dest_state == HALMAC_CFG_PARA_CMD_CONSTRUCT_H2C_SENT) {
		if ((pCfg_para->cfg_para_cmd_construct_state == HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE)
		    || (pCfg_para->cfg_para_cmd_construct_state == HALMAC_CFG_PARA_CMD_CONSTRUCT_H2C_SENT))
			return HALMAC_RET_ERROR_STATE;
	}

	pCfg_para->cfg_para_cmd_construct_state = dest_state;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_enqueue_para_buff_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PHY_PARAMETER_INFO para_info,
	IN u8 *pCurr_buff_wptr,
	OUT u8 *pEnd_cmd
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_CONFIG_PARA_INFO pConfig_para_info = &pHalmac_adapter->config_para_info;

	*pEnd_cmd = _FALSE;

	PHY_PARAMETER_INFO_SET_LENGTH(pCurr_buff_wptr, HALMAC_FW_OFFLOAD_CMD_SIZE_88XX);
	PHY_PARAMETER_INFO_SET_IO_CMD(pCurr_buff_wptr, para_info->cmd_id);

	switch (para_info->cmd_id) {
	case HALMAC_PARAMETER_CMD_BB_W8:
	case HALMAC_PARAMETER_CMD_BB_W16:
	case HALMAC_PARAMETER_CMD_BB_W32:
	case HALMAC_PARAMETER_CMD_MAC_W8:
	case HALMAC_PARAMETER_CMD_MAC_W16:
	case HALMAC_PARAMETER_CMD_MAC_W32:
		PHY_PARAMETER_INFO_SET_IO_ADDR(pCurr_buff_wptr, para_info->content.MAC_REG_W.offset);
		PHY_PARAMETER_INFO_SET_DATA(pCurr_buff_wptr, para_info->content.MAC_REG_W.value);
		PHY_PARAMETER_INFO_SET_MASK(pCurr_buff_wptr, para_info->content.MAC_REG_W.msk);
		PHY_PARAMETER_INFO_SET_MSK_EN(pCurr_buff_wptr, para_info->content.MAC_REG_W.msk_en);
		pConfig_para_info->value_accumulation += para_info->content.MAC_REG_W.value;
		pConfig_para_info->offset_accumulation += para_info->content.MAC_REG_W.offset;
		break;
	case HALMAC_PARAMETER_CMD_RF_W:
		PHY_PARAMETER_INFO_SET_RF_ADDR(pCurr_buff_wptr, para_info->content.RF_REG_W.offset); /*In rf register, the address is only 1 byte*/
		PHY_PARAMETER_INFO_SET_RF_PATH(pCurr_buff_wptr, para_info->content.RF_REG_W.rf_path);
		PHY_PARAMETER_INFO_SET_DATA(pCurr_buff_wptr, para_info->content.RF_REG_W.value);
		PHY_PARAMETER_INFO_SET_MASK(pCurr_buff_wptr, para_info->content.RF_REG_W.msk);
		PHY_PARAMETER_INFO_SET_MSK_EN(pCurr_buff_wptr, para_info->content.RF_REG_W.msk_en);
		pConfig_para_info->value_accumulation += para_info->content.RF_REG_W.value;
		pConfig_para_info->offset_accumulation += (para_info->content.RF_REG_W.offset + (para_info->content.RF_REG_W.rf_path << 8));
		break;
	case HALMAC_PARAMETER_CMD_DELAY_US:
	case HALMAC_PARAMETER_CMD_DELAY_MS:
		PHY_PARAMETER_INFO_SET_DELAY_VALUE(pCurr_buff_wptr, para_info->content.DELAY_TIME.delay_time);
		break;
	case HALMAC_PARAMETER_CMD_END:
		*pEnd_cmd = _TRUE;
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_phy_parameter_88xx illegal cmd_id!!\n");
		break;
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_gen_cfg_para_h2c_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pH2c_buff
)
{
	PHALMAC_CONFIG_PARA_INFO pConfig_para_info = &pHalmac_adapter->config_para_info;

	CFG_PARAMETER_SET_NUM(pH2c_buff, pConfig_para_info->para_num);

	if (pConfig_para_info->full_fifo_mode == _TRUE) {
		CFG_PARAMETER_SET_INIT_CASE(pH2c_buff, 0x1);
		CFG_PARAMETER_SET_PHY_PARAMETER_LOC(pH2c_buff, 0);
	} else {
		CFG_PARAMETER_SET_INIT_CASE(pH2c_buff, 0x0);
		CFG_PARAMETER_SET_PHY_PARAMETER_LOC(pH2c_buff, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy - pHalmac_adapter->txff_allocation.rsvd_pg_bndy);
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_update_packet_88xx() - send specific packet to FW
 * @pHalmac_adapter : the adapter of halmac
 * @pkt_id : packet id, to know the purpose of this packet
 * @pkt : packet
 * @pkt_size : packet size
 *
 * Note : TX_DESC is not included in the pkt
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_update_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PACKET_ID	pkt_id,
	IN u8 *pkt,
	IN u32 pkt_size
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.update_packet_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	if (pkt_size > HALMAC_UPDATE_PKT_RSVDPG_SZ_88XX)
		return HALMAC_RET_RSVD_PG_OVERFLOW_FAIL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_update_packet_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Wait event(update_packet)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	status = halmac_send_h2c_update_packet_88xx(pHalmac_adapter, pkt_id, pkt, pkt_size);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_update_packet_88xx packet = %x,  fail = %x!!\n", pkt_id, status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_update_packet_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_send_h2c_update_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PACKET_ID	pkt_id,
	IN u8 *pkt,
	IN u32 pkt_size
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	ret_status = halmac_download_rsvd_page_88xx(pHalmac_adapter, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy, pkt, pkt_size);

	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_download_rsvd_page_88xx Fail = %x!!\n", ret_status);
		return ret_status;
	}

	UPDATE_PACKET_SET_SIZE(pH2c_buff, pkt_size + pHalmac_adapter->hw_config_info.txdesc_size);
	UPDATE_PACKET_SET_PACKET_ID(pH2c_buff, pkt_id);
	UPDATE_PACKET_SET_PACKET_LOC(pH2c_buff, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy - pHalmac_adapter->txff_allocation.rsvd_pg_bndy);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_UPDATE_PACKET;
	h2c_header_info.content_size = 8;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);
	pHalmac_adapter->halmac_state.update_packet_set.seq_num = h2c_seq_mum;

	ret_status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_update_packet_88xx Fail = %x!!\n", ret_status);
		halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_UPDATE_PACKET);
		return ret_status;
	}

	return ret_status;
}

HALMAC_RET_STATUS
halmac_bcn_ie_filter_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_BCN_IE_INFO pBcn_ie_info
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_bcn_ie_filter_88xx ==========>\n");

	status = halmac_send_h2c_update_bcn_parse_info_88xx(pHalmac_adapter, pBcn_ie_info);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_update_bcn_parse_info_88xx fail = %x\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_bcn_ie_filter_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_send_h2c_update_bcn_parse_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_BCN_IE_INFO pBcn_ie_info
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_h2c_update_bcn_parse_info_88xx!!\n");

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	UPDATE_BEACON_PARSING_INFO_SET_FUNC_EN(pH2c_buff, pBcn_ie_info->func_en);
	UPDATE_BEACON_PARSING_INFO_SET_SIZE_TH(pH2c_buff, pBcn_ie_info->size_th);
	UPDATE_BEACON_PARSING_INFO_SET_TIMEOUT(pH2c_buff, pBcn_ie_info->timeout);

	UPDATE_BEACON_PARSING_INFO_SET_IE_ID_BMP_0(pH2c_buff, (u32)(pBcn_ie_info->ie_bmp[0]));
	UPDATE_BEACON_PARSING_INFO_SET_IE_ID_BMP_1(pH2c_buff, (u32)(pBcn_ie_info->ie_bmp[1]));
	UPDATE_BEACON_PARSING_INFO_SET_IE_ID_BMP_2(pH2c_buff, (u32)(pBcn_ie_info->ie_bmp[2]));
	UPDATE_BEACON_PARSING_INFO_SET_IE_ID_BMP_3(pH2c_buff, (u32)(pBcn_ie_info->ie_bmp[3]));
	UPDATE_BEACON_PARSING_INFO_SET_IE_ID_BMP_4(pH2c_buff, (u32)(pBcn_ie_info->ie_bmp[4]));

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_UPDATE_BEACON_PARSING_INFO;
	h2c_header_info.content_size = 24;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail =%x !!\n", status);
		return status;
	}

	return status;
}

HALMAC_RET_STATUS
halmac_update_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type,
	IN PHALMAC_PHY_PARAMETER_INFO para_info
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_update_datapack_88xx ==========>\n");

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_update_datapack_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_run_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_run_datapack_88xx ==========>\n");

	ret_status = halmac_send_h2c_run_datapack_88xx(pHalmac_adapter, halmac_data_type);

	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_run_datapack_88xx Fail, datatype = %x, status = %x!!\n", halmac_data_type, ret_status);
		return ret_status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_update_datapack_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_send_h2c_run_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_h2c_run_datapack_88xx!!\n");

	RUN_DATAPACK_SET_DATAPACK_ID(pH2c_buff, halmac_data_type);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_RUN_DATAPACK;
	h2c_header_info.content_size = 4;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);
		return status;
	}

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_send_bt_coex_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pBt_buf,
	IN u32 bt_size,
	IN u8 ack
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_bt_coex_88xx ==========>\n");

	ret_status = halmac_send_bt_coex_cmd_88xx(pHalmac_adapter, pBt_buf, bt_size, ack);

	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_bt_coex_cmd_88xx Fail = %x!!\n", ret_status);
		return ret_status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_bt_coex_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_send_bt_coex_cmd_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pBt_buf,
	IN u32 bt_size,
	IN u8 ack
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_bt_coex_cmd_88xx!!\n");

	PLATFORM_RTL_MEMCPY(pDriver_adapter, pH2c_buff + 8, pBt_buf, bt_size);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_BT_COEX;
	h2c_header_info.content_size = (u16)bt_size;
	h2c_header_info.ack = ack;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, ack);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);
		return status;
	}

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_send_original_h2c_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *original_h2c,
	IN u16 *seq,
	IN u8 ack
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_original_h2c_88xx ==========>\n");

	status = halmac_func_send_original_h2c_88xx(pHalmac_adapter, original_h2c, seq, ack);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_original_h2c FAIL = %x!!\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_original_h2c_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_func_send_original_h2c_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *original_h2c,
	IN u16 *seq,
	IN u8 ack
)
{
	u8 H2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u8 *pH2c_header, *pH2c_cmd;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_original_h2c ==========>\n");

	pH2c_header = H2c_buff;
	pH2c_cmd = pH2c_header + HALMAC_H2C_CMD_HDR_SIZE_88XX;
	PLATFORM_RTL_MEMCPY(pDriver_adapter, pH2c_cmd, original_h2c, 8); /* Original H2C 8 byte */

	halmac_set_h2c_header_88xx(pHalmac_adapter, pH2c_header, seq, ack);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, H2c_buff, HALMAC_H2C_CMD_SIZE_88XX, ack);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_original_h2c Fail = %x!!\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_original_h2c <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_fill_txdesc_check_sum_88xx() -  fill in tx desc check sum
 * @pHalmac_adapter : the adapter of halmac
 * @pCur_desc : tx desc packet
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_fill_txdesc_check_sum_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT u8 *pCur_desc
)
{
	u16 chk_result = 0;
	u16 *pData = (u16 *)NULL;
	u32 i;
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (pCur_desc == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_fill_txdesc_check_sum_88xx NULL PTR");
		return HALMAC_RET_NULL_POINTER;
	}

	if (pHalmac_adapter->tx_desc_checksum != _TRUE)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]checksum_en = %d", pHalmac_adapter->tx_desc_checksum);

	SET_TX_DESC_TXDESC_CHECKSUM(pCur_desc, 0x0000);

	pData = (u16 *)(pCur_desc);

	/* HW clculates only 32byte */
	for (i = 0; i < 8; i++)
		chk_result ^= (*(pData + 2 * i) ^ *(pData + (2 * i + 1)));

	/* *(pData + 2 * i) & *(pData + (2 * i + 1) have endain issue*/
	/* Process eniadn issue after checksum calculation */
	chk_result = rtk_le16_to_cpu(chk_result);

	SET_TX_DESC_TXDESC_CHECKSUM(pCur_desc, chk_result);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_dump_fifo_88xx() - dump fifo data
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_fifo_sel : FIFO selection
 * @halmac_start_addr : start address of selected FIFO
 * @halmac_fifo_dump_size : dump size of selected FIFO
 * @pFifo_map : FIFO data
 *
 * Note : before dump fifo, user need to call halmac_get_fifo_size to
 * get fifo size. Then input this size to halmac_dump_fifo.
 *
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dump_fifo_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HAL_FIFO_SEL halmac_fifo_sel,
	IN u32 halmac_start_addr,
	IN u32 halmac_fifo_dump_size,
	OUT u8 *pFifo_map
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	u8 reg_backup;
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_fifo_88xx ==========>\n");

	if (halmac_fifo_sel == HAL_FIFO_SEL_TX && (halmac_start_addr + halmac_fifo_dump_size) > pHalmac_adapter->hw_config_info.tx_fifo_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]TX fifo dump size is too large\n");
		return HALMAC_RET_DUMP_FIFOSIZE_INCORRECT;
	}

	if (halmac_fifo_sel == HAL_FIFO_SEL_RX && (halmac_start_addr + halmac_fifo_dump_size) > pHalmac_adapter->hw_config_info.rx_fifo_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]RX fifo dump size is too large\n");
		return HALMAC_RET_DUMP_FIFOSIZE_INCORRECT;
	}

	if ((halmac_fifo_dump_size & (4 - 1)) != 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_fifo_dump_size shall 4byte align\n");
		return HALMAC_RET_DUMP_FIFOSIZE_INCORRECT;
	}

	if (pFifo_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]pFifo_map address is NULL\n");
		return HALMAC_RET_NULL_POINTER;
	}

	reg_backup = HALMAC_REG_READ_8(pHalmac_adapter, REG_RCR + 2);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RCR + 2, reg_backup | BIT(3));

	status = halmac_buffer_read_88xx(pHalmac_adapter, halmac_start_addr, halmac_fifo_dump_size, halmac_fifo_sel, pFifo_map);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RCR + 2, reg_backup);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_buffer_read_88xx error = %x\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_fifo_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_buffer_read_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u32 size,
	IN HAL_FIFO_SEL halmac_fifo_sel,
	OUT u8 *pFifo_map
)
{
	u32 start_page, value_read;
	u32 i, counter = 0, residue;
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (halmac_fifo_sel == HAL_FIFO_SEL_RSVD_PAGE)
		offset = offset + (pHalmac_adapter->txff_allocation.rsvd_pg_bndy << HALMAC_TX_PAGE_SIZE_2_POWER_88XX);

	start_page = offset >> 12;
	residue = offset & (4096 - 1);

	if ((halmac_fifo_sel == HAL_FIFO_SEL_TX) || (halmac_fifo_sel == HAL_FIFO_SEL_RSVD_PAGE))
		start_page += 0x780;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_RX)
		start_page += 0x700;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_REPORT)
		start_page += 0x660;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_LLT)
		start_page += 0x650;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_RXBUF_FW)
		start_page += 0x680;
	else
		return HALMAC_RET_NOT_SUPPORT;

	value_read = HALMAC_REG_READ_16(pHalmac_adapter, REG_PKTBUF_DBG_CTRL);

	do {
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_PKTBUF_DBG_CTRL, (u16)(start_page | (value_read & 0xF000)));

		for (i = 0x8000 + residue; i <= 0x8FFF; i += 4) {
			*(u32 *)(pFifo_map + counter) = HALMAC_REG_READ_32(pHalmac_adapter, i);
			*(u32 *)(pFifo_map + counter) = rtk_le32_to_cpu(*(u32 *)(pFifo_map + counter));
			counter += 4;
			if (size == counter)
				goto HALMAC_BUF_READ_OK;
		}

		residue = 0;
		start_page++;
	} while (1);

HALMAC_BUF_READ_OK:
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_PKTBUF_DBG_CTRL, (u16)value_read);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_fifo_size_88xx() - get fifo size
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_fifo_sel : FIFO selection
 * Author : Ivan Lin/KaiYuan Chang
 * Return : u32
 * More details of status code can be found in prototype document
 */
u32
halmac_get_fifo_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HAL_FIFO_SEL halmac_fifo_sel
)
{
	u32 fifo_size = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fifo_sel == HAL_FIFO_SEL_TX)
		fifo_size = pHalmac_adapter->hw_config_info.tx_fifo_size;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_RX)
		fifo_size = pHalmac_adapter->hw_config_info.rx_fifo_size;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_RSVD_PAGE)
		fifo_size = ((pHalmac_adapter->hw_config_info.tx_fifo_size >> HALMAC_TX_PAGE_SIZE_2_POWER_88XX)
						- pHalmac_adapter->txff_allocation.rsvd_pg_bndy) << HALMAC_TX_PAGE_SIZE_2_POWER_88XX;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_REPORT)
		fifo_size = 65536;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_LLT)
		fifo_size = 65536;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_RXBUF_FW)
		fifo_size = HALMAC_RX_BUF_FW_88XX;

	return fifo_size;
}

HALMAC_RET_STATUS
halmac_set_h2c_header_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u8 *pHal_h2c_hdr,
	IN u16 *seq,
	IN u8 ack
)
{
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_set_h2c_header_88xx!!\n");

	H2C_CMD_HEADER_SET_CATEGORY(pHal_h2c_hdr, 0x00);
	H2C_CMD_HEADER_SET_TOTAL_LEN(pHal_h2c_hdr, 16);

	PLATFORM_MUTEX_LOCK(pDriver_adapter, &pHalmac_adapter->h2c_seq_mutex);
	H2C_CMD_HEADER_SET_SEQ_NUM(pHal_h2c_hdr, pHalmac_adapter->h2c_packet_seq);
	*seq = pHalmac_adapter->h2c_packet_seq;
	pHalmac_adapter->h2c_packet_seq++;
	PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &pHalmac_adapter->h2c_seq_mutex);

	if (ack == _TRUE)
		H2C_CMD_HEADER_SET_ACK(pHal_h2c_hdr, _TRUE);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_add_ch_info_88xx() -add channel information
 * @pHalmac_adapter : the adapter of halmac
 * @pCh_info : channel information
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_add_ch_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_INFO pCh_info
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_CS_INFO pCh_sw_info;
	HALMAC_SCAN_CMD_CONSTRUCT_STATE state_scan;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pCh_sw_info = &pHalmac_adapter->ch_sw_info;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_add_ch_info_88xx ==========>\n");

	if (pHalmac_adapter->halmac_state.dlfw_state != HALMAC_GEN_INFO_SENT) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_add_ch_info_88xx: gen_info is not send to FW!!!!\n");
		return HALMAC_RET_GEN_INFO_NOT_SENT;
	}

	state_scan = halmac_query_scan_curr_state_88xx(pHalmac_adapter);
	if ((state_scan != HALMAC_SCAN_CMD_CONSTRUCT_BUFFER_CLEARED) && (state_scan != HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Scan machine fail(add ch info)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (pCh_sw_info->ch_info_buf == NULL) {
		pCh_sw_info->ch_info_buf = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, HALMAC_SCAN_INFO_RSVDPG_SZ_88XX);
		if (pCh_sw_info->ch_info_buf == NULL)
			return HALMAC_RET_NULL_POINTER;
		pCh_sw_info->ch_info_buf_w = pCh_sw_info->ch_info_buf;
		pCh_sw_info->buf_size = HALMAC_SCAN_INFO_RSVDPG_SZ_88XX;
		pCh_sw_info->avai_buf_size = HALMAC_SCAN_INFO_RSVDPG_SZ_88XX;
		pCh_sw_info->total_size = 0;
		pCh_sw_info->extra_info_en = 0;
		pCh_sw_info->ch_num = 0;
	}

	if (pCh_sw_info->extra_info_en == 1) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_add_ch_info_88xx: construct sequence wrong!!\n");
		return HALMAC_RET_CH_SW_SEQ_WRONG;
	}

	if (pCh_sw_info->avai_buf_size < 4) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_add_ch_info_88xx: no available buffer!!\n");
		return HALMAC_RET_CH_SW_NO_BUF;
	}

	if (halmac_transition_scan_state_88xx(pHalmac_adapter, HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	CHANNEL_INFO_SET_CHANNEL(pCh_sw_info->ch_info_buf_w, pCh_info->channel);
	CHANNEL_INFO_SET_PRI_CH_IDX(pCh_sw_info->ch_info_buf_w, pCh_info->pri_ch_idx);
	CHANNEL_INFO_SET_BANDWIDTH(pCh_sw_info->ch_info_buf_w, pCh_info->bw);
	CHANNEL_INFO_SET_TIMEOUT(pCh_sw_info->ch_info_buf_w, pCh_info->timeout);
	CHANNEL_INFO_SET_ACTION_ID(pCh_sw_info->ch_info_buf_w, pCh_info->action_id);
	CHANNEL_INFO_SET_CH_EXTRA_INFO(pCh_sw_info->ch_info_buf_w, pCh_info->extra_info);

	pCh_sw_info->avai_buf_size = pCh_sw_info->avai_buf_size - 4;
	pCh_sw_info->total_size = pCh_sw_info->total_size + 4;
	pCh_sw_info->ch_num++;
	pCh_sw_info->extra_info_en = pCh_info->extra_info;
	pCh_sw_info->ch_info_buf_w = pCh_sw_info->ch_info_buf_w + 4;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_add_ch_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_SCAN_CMD_CONSTRUCT_STATE
halmac_query_scan_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	return pHalmac_adapter->halmac_state.scan_state_set.scan_cmd_construct_state;
}

static HALMAC_RET_STATUS
halmac_transition_scan_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_SCAN_CMD_CONSTRUCT_STATE dest_state
)
{
	PHALMAC_SCAN_STATE_SET pScan = &pHalmac_adapter->halmac_state.scan_state_set;

	if (pScan->scan_cmd_construct_state > HALMAC_SCAN_CMD_CONSTRUCT_H2C_SENT)
		return HALMAC_RET_ERROR_STATE;

	if (dest_state == HALMAC_SCAN_CMD_CONSTRUCT_IDLE) {
		if ((pScan->scan_cmd_construct_state == HALMAC_SCAN_CMD_CONSTRUCT_BUFFER_CLEARED) ||
		    (pScan->scan_cmd_construct_state == HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING))
			return HALMAC_RET_ERROR_STATE;
	} else if (dest_state == HALMAC_SCAN_CMD_CONSTRUCT_BUFFER_CLEARED) {
		if (pScan->scan_cmd_construct_state == HALMAC_SCAN_CMD_CONSTRUCT_H2C_SENT)
			return HALMAC_RET_ERROR_STATE;
	} else if (dest_state == HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) {
		if ((pScan->scan_cmd_construct_state == HALMAC_SCAN_CMD_CONSTRUCT_IDLE) ||
		    (pScan->scan_cmd_construct_state == HALMAC_SCAN_CMD_CONSTRUCT_H2C_SENT))
			return HALMAC_RET_ERROR_STATE;
	} else if (dest_state == HALMAC_SCAN_CMD_CONSTRUCT_H2C_SENT) {
		if ((pScan->scan_cmd_construct_state != HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) &&
		    (pScan->scan_cmd_construct_state != HALMAC_SCAN_CMD_CONSTRUCT_BUFFER_CLEARED))
			return HALMAC_RET_ERROR_STATE;
	}

	pScan->scan_cmd_construct_state = dest_state;

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_add_extra_ch_info_88xx() -add extra channel information
 * @pHalmac_adapter : the adapter of halmac
 * @pCh_extra_info : extra channel information
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_add_extra_ch_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_EXTRA_INFO pCh_extra_info
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_CS_INFO pCh_sw_info;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pCh_sw_info = &pHalmac_adapter->ch_sw_info;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_add_extra_ch_info_88xx ==========>\n");

	if (pCh_sw_info->ch_info_buf == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_add_extra_ch_info_88xx: NULL==pCh_sw_info->ch_info_buf!!\n");
		return HALMAC_RET_CH_SW_SEQ_WRONG;
	}

	if (pCh_sw_info->extra_info_en == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_add_extra_ch_info_88xx: construct sequence wrong!!\n");
		return HALMAC_RET_CH_SW_SEQ_WRONG;
	}

	if (pCh_sw_info->avai_buf_size < (u32)(pCh_extra_info->extra_info_size + 2)) {/* 2:ch_extra_info_id, ch_extra_info, ch_extra_info_size are totally 2Byte */
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_add_extra_ch_info_88xx: no available buffer!!\n");
		return HALMAC_RET_CH_SW_NO_BUF;
	}

	if (halmac_query_scan_curr_state_88xx(pHalmac_adapter) != HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Scan machine fail(add extra ch info)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (halmac_transition_scan_state_88xx(pHalmac_adapter, HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	CH_EXTRA_INFO_SET_CH_EXTRA_INFO_ID(pCh_sw_info->ch_info_buf_w, pCh_extra_info->extra_action_id);
	CH_EXTRA_INFO_SET_CH_EXTRA_INFO(pCh_sw_info->ch_info_buf_w, pCh_extra_info->extra_info);
	CH_EXTRA_INFO_SET_CH_EXTRA_INFO_SIZE(pCh_sw_info->ch_info_buf_w, pCh_extra_info->extra_info_size);
	PLATFORM_RTL_MEMCPY(pDriver_adapter, pCh_sw_info->ch_info_buf_w + 2, pCh_extra_info->extra_info_data, pCh_extra_info->extra_info_size);

	pCh_sw_info->avai_buf_size = pCh_sw_info->avai_buf_size - (2 + pCh_extra_info->extra_info_size);
	pCh_sw_info->total_size = pCh_sw_info->total_size + (2 + pCh_extra_info->extra_info_size);
	pCh_sw_info->extra_info_en = pCh_extra_info->extra_info;
	pCh_sw_info->ch_info_buf_w = pCh_sw_info->ch_info_buf_w + (2 + pCh_extra_info->extra_info_size);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_add_extra_ch_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_ctrl_ch_switch_88xx() -send channel switch cmd
 * @pHalmac_adapter : the adapter of halmac
 * @pCs_option : channel switch config
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_ctrl_ch_switch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_SWITCH_OPTION	pCs_option
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_SCAN_CMD_CONSTRUCT_STATE state_scan;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.scan_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_ctrl_ch_switch_88xx  pCs_option->switch_en = %d==========>\n", pCs_option->switch_en);

	if (pCs_option->switch_en == _FALSE)
		*pProcess_status = HALMAC_CMD_PROCESS_IDLE;

	if ((*pProcess_status == HALMAC_CMD_PROCESS_SENDING) || (*pProcess_status == HALMAC_CMD_PROCESS_RCVD)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Wait event(ctrl ch switch)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	state_scan = halmac_query_scan_curr_state_88xx(pHalmac_adapter);
	if (pCs_option->switch_en == _TRUE) {
		if (state_scan != HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_ctrl_ch_switch_88xx(on)  invalid in state %x\n", state_scan);
			return HALMAC_RET_ERROR_STATE;
		}
	} else {
		if (state_scan != HALMAC_SCAN_CMD_CONSTRUCT_BUFFER_CLEARED) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_ctrl_ch_switch_88xx(off)  invalid in state %x\n", state_scan);
			return HALMAC_RET_ERROR_STATE;
		}
	}

	status = halmac_func_ctrl_ch_switch_88xx(pHalmac_adapter, pCs_option);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_ctrl_ch_switch FAIL = %x!!\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_ctrl_ch_switch_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_func_ctrl_ch_switch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_SWITCH_OPTION pCs_option
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.scan_state_set.process_status;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_ctrl_ch_switch!!\n");

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (halmac_transition_scan_state_88xx(pHalmac_adapter, HALMAC_SCAN_CMD_CONSTRUCT_H2C_SENT) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	if (pCs_option->switch_en != 0) {
		status = halmac_download_rsvd_page_88xx(pHalmac_adapter, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy,
						pHalmac_adapter->ch_sw_info.ch_info_buf, pHalmac_adapter->ch_sw_info.total_size);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_download_rsvd_page_88xx Fail = %x!!\n", status);
			return status;
		}
	}

	CHANNEL_SWITCH_SET_SWITCH_START(pH2c_buff, pCs_option->switch_en);
	CHANNEL_SWITCH_SET_CHANNEL_NUM(pH2c_buff, pHalmac_adapter->ch_sw_info.ch_num);
	CHANNEL_SWITCH_SET_CHANNEL_INFO_LOC(pH2c_buff, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy - pHalmac_adapter->txff_allocation.rsvd_pg_bndy);
	CHANNEL_SWITCH_SET_DEST_CH_EN(pH2c_buff, pCs_option->dest_ch_en);
	CHANNEL_SWITCH_SET_DEST_CH(pH2c_buff, pCs_option->dest_ch);
	CHANNEL_SWITCH_SET_PRI_CH_IDX(pH2c_buff, pCs_option->dest_pri_ch_idx);
	CHANNEL_SWITCH_SET_ABSOLUTE_TIME(pH2c_buff, pCs_option->absolute_time_en);
	CHANNEL_SWITCH_SET_TSF_LOW(pH2c_buff, pCs_option->tsf_low);
	CHANNEL_SWITCH_SET_PERIODIC_OPTION(pH2c_buff, pCs_option->periodic_option);
	CHANNEL_SWITCH_SET_NORMAL_CYCLE(pH2c_buff, pCs_option->normal_cycle);
	CHANNEL_SWITCH_SET_NORMAL_PERIOD(pH2c_buff, pCs_option->normal_period);
	CHANNEL_SWITCH_SET_SLOW_PERIOD(pH2c_buff, pCs_option->phase_2_period);
	CHANNEL_SWITCH_SET_NORMAL_PERIOD_SEL(pH2c_buff, pCs_option->normal_period_sel);
	CHANNEL_SWITCH_SET_SLOW_PERIOD_SEL(pH2c_buff, pCs_option->phase_2_period_sel);
	CHANNEL_SWITCH_SET_CHANNEL_INFO_SIZE(pH2c_buff, pHalmac_adapter->ch_sw_info.total_size);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_CHANNEL_SWITCH;
	h2c_header_info.content_size = 20;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);
	pHalmac_adapter->halmac_state.scan_state_set.seq_num = h2c_seq_mum;

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);
		halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_CHANNEL_SWITCH);
	}
	PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter->ch_sw_info.ch_info_buf, pHalmac_adapter->ch_sw_info.buf_size);
	pHalmac_adapter->ch_sw_info.ch_info_buf = NULL;
	pHalmac_adapter->ch_sw_info.ch_info_buf_w = NULL;
	pHalmac_adapter->ch_sw_info.extra_info_en = 0;
	pHalmac_adapter->ch_sw_info.buf_size = 0;
	pHalmac_adapter->ch_sw_info.avai_buf_size = 0;
	pHalmac_adapter->ch_sw_info.total_size = 0;
	pHalmac_adapter->ch_sw_info.ch_num = 0;

	if (halmac_transition_scan_state_88xx(pHalmac_adapter, HALMAC_SCAN_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	return status;
}

/**
 * halmac_clear_ch_info_88xx() -clear channel information
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_clear_ch_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_clear_ch_info_88xx ==========>\n");

	if (halmac_query_scan_curr_state_88xx(pHalmac_adapter) == HALMAC_SCAN_CMD_CONSTRUCT_H2C_SENT) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Scan machine fail(clear ch info)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (halmac_transition_scan_state_88xx(pHalmac_adapter, HALMAC_SCAN_CMD_CONSTRUCT_BUFFER_CLEARED) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter->ch_sw_info.ch_info_buf, pHalmac_adapter->ch_sw_info.buf_size);
	pHalmac_adapter->ch_sw_info.ch_info_buf = NULL;
	pHalmac_adapter->ch_sw_info.ch_info_buf_w = NULL;
	pHalmac_adapter->ch_sw_info.extra_info_en = 0;
	pHalmac_adapter->ch_sw_info.buf_size = 0;
	pHalmac_adapter->ch_sw_info.avai_buf_size = 0;
	pHalmac_adapter->ch_sw_info.total_size = 0;
	pHalmac_adapter->ch_sw_info.ch_num = 0;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_clear_ch_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_send_general_info_88xx() -send general information to FW
 * @pHalmac_adapter : the adapter of halmac
 * @pGeneral_info : general information
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_send_general_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_GENERAL_INFO pGeneral_info
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_general_info_88xx ==========>\n");

	if (pHalmac_adapter->halmac_state.dlfw_state == HALMAC_DLFW_NONE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_general_info_88xx Fail due to DLFW NONE!!\n");
		return HALMAC_RET_NO_DLFW;
	}

	status = halmac_func_send_general_info_88xx(pHalmac_adapter, pGeneral_info);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_send_general_info error = %x\n", status);
		return status;
	}

	status = halmac_func_send_phydm_info_88xx(pHalmac_adapter, pGeneral_info);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_send_phydm_info error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->halmac_state.dlfw_state == HALMAC_DLFW_DONE)
		pHalmac_adapter->halmac_state.dlfw_state = HALMAC_GEN_INFO_SENT;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_general_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_func_send_general_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_GENERAL_INFO pGeneral_info
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_general_info!!\n");

	GENERAL_INFO_SET_FW_TX_BOUNDARY(pH2c_buff, pHalmac_adapter->txff_allocation.rsvd_fw_txbuff_pg_bndy - pHalmac_adapter->txff_allocation.rsvd_pg_bndy);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_GENERAL_INFO;
	h2c_header_info.content_size = 4;
	h2c_header_info.ack = _FALSE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);

	return status;
}

static HALMAC_RET_STATUS
halmac_func_send_phydm_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_GENERAL_INFO pGeneral_info
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_send_phydm_info!!\n");

	PHYDM_INFO_SET_REF_TYPE(pH2c_buff, pGeneral_info->rfe_type);
	PHYDM_INFO_SET_RF_TYPE(pH2c_buff, pGeneral_info->rf_type);
	PHYDM_INFO_SET_CUT_VER(pH2c_buff, pHalmac_adapter->chip_version);
	PHYDM_INFO_SET_RX_ANT_STATUS(pH2c_buff, pGeneral_info->rx_ant_status);
	PHYDM_INFO_SET_TX_ANT_STATUS(pH2c_buff, pGeneral_info->tx_ant_status);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_PHYDM_INFO;
	h2c_header_info.content_size = 8;
	h2c_header_info.ack = _FALSE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);

	return status;
}

/**
 * halmac_chk_txdesc_88xx() -check if the tx packet format is incorrect
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_buf : tx Packet buffer, tx desc is included
 * @halmac_size : tx packet size
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_chk_txdesc_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
)
{
	u32 mac_clk = 0;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_chk_txdesc_88xx ==========>\n");

	if (GET_TX_DESC_BMC(pHalmac_buf) == _TRUE)
		if (GET_TX_DESC_AGG_EN(pHalmac_buf) == _TRUE)
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]TxDesc: Agg should not be set when BMC\n");

	if (halmac_size < (GET_TX_DESC_TXPKTSIZE(pHalmac_buf) + GET_TX_DESC_OFFSET(pHalmac_buf)))
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]TxDesc: PktSize too small\n");

	if (GET_TX_DESC_AMSDU_PAD_EN(pHalmac_buf) != 0)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]TxDesc: Do not set AMSDU_PAD_EN\n");

	switch (BIT_GET_MAC_CLK_SEL(HALMAC_REG_READ_32(pHalmac_adapter, REG_AFE_CTRL1))) {
	case 0x0:
		mac_clk = 80;
		break;
	case 0x1:
		mac_clk = 40;
		break;
	case 0x2:
		mac_clk = 20;
		break;
	case 0x3:
		mac_clk = 10;
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ALWAYS, "MAC clock : 0x%XM\n", mac_clk);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ALWAYS, "TX mac agg enable : 0x%X\n", GET_TX_DESC_AGG_EN(pHalmac_buf));
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ALWAYS, "TX mac agg num : 0x%X\n", GET_TX_DESC_MAX_AGG_NUM(pHalmac_buf));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_chk_txdesc_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_version() - get HALMAC version
 * @version : return version of major, prototype and minor information
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_version_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_VER pVersion
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_get_chip_version_88xx ==========>\n");
	pVersion->major_ver = (u8)HALMAC_MAJOR_VER_88XX;
	pVersion->prototype_ver = (u8)HALMAC_PROTOTYPE_VER_88XX;
	pVersion->minor_ver = (u8)HALMAC_MINOR_VER_88XX;
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_get_chip_version_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_p2pps_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_P2PPS	pP2PPS
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 6)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	status = halmac_func_p2pps_88xx(pHalmac_adapter, pP2PPS);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_p2pps FAIL = %x!!\n", status);
		return status;
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_func_p2pps_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_P2PPS	pP2PPS
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_p2pps !!\n");

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	P2PPS_SET_OFFLOAD_EN(pH2c_buff, pP2PPS->offload_en);
	P2PPS_SET_ROLE(pH2c_buff, pP2PPS->role);
	P2PPS_SET_CTWINDOW_EN(pH2c_buff, pP2PPS->ctwindow_en);
	P2PPS_SET_NOA_EN(pH2c_buff, pP2PPS->noa_en);
	P2PPS_SET_NOA_SEL(pH2c_buff, pP2PPS->noa_sel);
	P2PPS_SET_ALLSTASLEEP(pH2c_buff, pP2PPS->all_sta_sleep);
	P2PPS_SET_DISCOVERY(pH2c_buff, pP2PPS->discovery);
	P2PPS_SET_DISABLE_CLOSERF(pH2c_buff, pP2PPS->disable_close_rf);
	P2PPS_SET_P2P_PORT_ID(pH2c_buff, pP2PPS->p2p_port_id);
	P2PPS_SET_P2P_GROUP(pH2c_buff, pP2PPS->p2p_group);
	P2PPS_SET_P2P_MACID(pH2c_buff, pP2PPS->p2p_macid);

	P2PPS_SET_CTWINDOW_LENGTH(pH2c_buff, pP2PPS->ctwindow_length);

	P2PPS_SET_NOA_DURATION_PARA(pH2c_buff, pP2PPS->noa_duration_para);
	P2PPS_SET_NOA_INTERVAL_PARA(pH2c_buff, pP2PPS->noa_interval_para);
	P2PPS_SET_NOA_START_TIME_PARA(pH2c_buff, pP2PPS->noa_start_time_para);
	P2PPS_SET_NOA_COUNT_PARA(pH2c_buff, pP2PPS->noa_count_para);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_P2PPS;
	h2c_header_info.content_size = 24;
	h2c_header_info.ack = _FALSE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _FALSE);

	if (status != HALMAC_RET_SUCCESS)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_p2pps_88xx Fail = %x!!\n", status);

	return status;
}

/**
 * halmac_query_status_88xx() -query the offload feature status
 * @pHalmac_adapter : the adapter of halmac
 * @feature_id : feature_id
 * @pProcess_status : feature_status
 * @data : data buffer
 * @size : data size
 *
 * Note :
 * If user wants to know the data size, user can allocate zero
 * size buffer first. If this size less than the data size, halmac
 * will return  HALMAC_RET_BUFFER_TOO_SMALL. User need to
 * re-allocate data buffer with correct data size.
 *
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_query_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_FEATURE_ID feature_id,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_query_status_88xx ==========>\n"); */

	if (pProcess_status == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]null pointer!!\n");
		return HALMAC_RET_NULL_POINTER;
	}

	switch (feature_id) {
	case HALMAC_FEATURE_CFG_PARA:
		status = halmac_query_cfg_para_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE:
		status = halmac_query_dump_physical_efuse_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_DUMP_LOGICAL_EFUSE:
		status = halmac_query_dump_logical_efuse_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_CHANNEL_SWITCH:
		status = halmac_query_channel_switch_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_UPDATE_PACKET:
		status = halmac_query_update_packet_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_IQK:
		status = halmac_query_iqk_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_POWER_TRACKING:
		status = halmac_query_power_tracking_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_PSD:
		status = halmac_query_psd_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_FW_SNDING:
		status = halmac_query_fw_snding_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_query_status_88xx invalid feature id %d\n", feature_id);
		return HALMAC_RET_INVALID_FEATURE_ID;
	}

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_query_status_88xx <==========\n"); */

	return status;
}

static HALMAC_RET_STATUS
halmac_query_cfg_para_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	PHALMAC_CFG_PARA_STATE_SET pCfg_para_state_set = &pHalmac_adapter->halmac_state.cfg_para_state_set;

	*pProcess_status = pCfg_para_state_set->process_status;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_query_channel_switch_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	PHALMAC_SCAN_STATE_SET pScan_state_set = &pHalmac_adapter->halmac_state.scan_state_set;

	*pProcess_status = pScan_state_set->process_status;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_query_update_packet_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	PHALMAC_UPDATE_PACKET_STATE_SET pUpdate_packet_set = &pHalmac_adapter->halmac_state.update_packet_set;

	*pProcess_status = pUpdate_packet_set->process_status;

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_drv_rsvd_pg_num_88xx() -config reserved page number for driver
 * @pHalmac_adapter : the adapter of halmac
 * @pg_num : page number
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_drv_rsvd_pg_num_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DRV_RSVD_PG_NUM pg_num
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (pHalmac_adapter->api_registry.cfg_drv_rsvd_pg_en == 0)
		return HALMAC_RET_NOT_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_drv_rsvd_pg_num_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]pg_num = %d\n", pg_num);

	switch (pg_num) {
	case HALMAC_RSVD_PG_NUM8:
		pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = 8;
		break;
	case HALMAC_RSVD_PG_NUM16:
		pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = 16;
		break;
	case HALMAC_RSVD_PG_NUM24:
		pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = 24;
		break;
	case HALMAC_RSVD_PG_NUM32:
		pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = 32;
		break;
	case HALMAC_RSVD_PG_NUM64:
		pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = 64;
		break;
	case HALMAC_RSVD_PG_NUM128:
		pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = 128;
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_drv_rsvd_pg_num_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * (debug API)halmac_h2c_lb_88xx() - send h2c loopback packet
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_h2c_lb_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_h2c_lb_88xx ==========>\n");

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_h2c_lb_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_pwr_seq_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 cut,
	IN u8 fab,
	IN u8 intf,
	IN PHALMAC_WLAN_PWR_CFG *ppPwr_seq_cfg
)
{
	u32 seq_idx = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	PHALMAC_WLAN_PWR_CFG pSeq_cmd;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	do {
		pSeq_cmd = ppPwr_seq_cfg[seq_idx];

		if (pSeq_cmd == NULL)
			break;

		status = halmac_pwr_sub_seq_parer_88xx(pHalmac_adapter, cut, fab, intf, pSeq_cmd);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]pwr sub seq parser fail, status = 0x%X!\n", status);
			return status;
		}

		seq_idx++;
	} while (1);

	return status;
}

static HALMAC_RET_STATUS
halmac_pwr_sub_seq_parer_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 cut,
	IN u8 fab,
	IN u8 intf,
	IN PHALMAC_WLAN_PWR_CFG pPwr_sub_seq_cfg
)
{
	u8 value, flag;
	u8 polling_bit;
	u32 offset;
	u32 polling_count;
	static u32 poll_to_static;
	VOID *pDriver_adapter = NULL;
	PHALMAC_WLAN_PWR_CFG pSub_seq_cmd;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	pSub_seq_cmd = pPwr_sub_seq_cfg;

	do {
		if ((pSub_seq_cmd->interface_msk & intf) && (pSub_seq_cmd->fab_msk & fab) && (pSub_seq_cmd->cut_msk & cut)) {
			switch (pSub_seq_cmd->cmd) {
			case HALMAC_PWR_CMD_WRITE:
				if (pSub_seq_cmd->base == HALMAC_PWR_BASEADDR_SDIO)
					offset = pSub_seq_cmd->offset | SDIO_LOCAL_OFFSET;
				else
					offset = pSub_seq_cmd->offset;

				value = HALMAC_REG_READ_8(pHalmac_adapter, offset);
				value = (u8)(value & (u8)(~(pSub_seq_cmd->msk)));
				value = (u8)(value | (u8)(pSub_seq_cmd->value & pSub_seq_cmd->msk));

				HALMAC_REG_WRITE_8(pHalmac_adapter, offset, value);
				break;
			case HALMAC_PWR_CMD_POLLING:
				polling_bit = 0;
				polling_count = HALMAC_POLLING_READY_TIMEOUT_COUNT;
				flag = 0;

				if (pSub_seq_cmd->base == HALMAC_PWR_BASEADDR_SDIO)
					offset = pSub_seq_cmd->offset | SDIO_LOCAL_OFFSET;
				else
					offset = pSub_seq_cmd->offset;

				do {
					polling_count--;
					value = HALMAC_REG_READ_8(pHalmac_adapter, offset);
					value = (u8)(value & pSub_seq_cmd->msk);

					if (value == (pSub_seq_cmd->value & pSub_seq_cmd->msk)) {
						polling_bit = 1;
					} else {
						if (polling_count == 0) {
							if (HALMAC_INTERFACE_PCIE == pHalmac_adapter->halmac_interface && 0 == flag) {
								/* For PCIE + USB package poll power bit timeout issue */
								poll_to_static++;
								PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_WARN, "[WARN]PCIE polling timeout : %d!!\n", poll_to_static);
								HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_PW_CTRL, HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_PW_CTRL) | BIT(3));
								HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_PW_CTRL, HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_PW_CTRL) & ~BIT(3));
								polling_bit = 0;
								polling_count = HALMAC_POLLING_READY_TIMEOUT_COUNT;
								flag = 1;
							} else {
								PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_ERR, "[ERR]Pwr cmd polling timeout!!\n");
								PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_ERR, "[ERR]Pwr cmd offset : %X!!\n", pSub_seq_cmd->offset);
								PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_ERR, "[ERR]Pwr cmd value : %X!!\n", pSub_seq_cmd->value);
								PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_ERR, "[ERR]Pwr cmd msk : %X!!\n", pSub_seq_cmd->msk);
								PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_ERR, "[ERR]Read offset = %X value = %X!!\n", offset, value);
								return HALMAC_RET_PWRSEQ_POLLING_FAIL;
							}
						} else {
							PLATFORM_RTL_DELAY_US(pDriver_adapter, 50);
						}
					}
				} while (!polling_bit);
				break;
			case HALMAC_PWR_CMD_DELAY:
				if (pSub_seq_cmd->value == HALMAC_PWRSEQ_DELAY_US)
					PLATFORM_RTL_DELAY_US(pDriver_adapter, pSub_seq_cmd->offset);
				else
					PLATFORM_RTL_DELAY_US(pDriver_adapter, 1000 * pSub_seq_cmd->offset);
				break;
			case HALMAC_PWR_CMD_READ:
				break;
			case HALMAC_PWR_CMD_END:
				return HALMAC_RET_SUCCESS;
			default:
				return HALMAC_RET_PWRSEQ_CMD_INCORRECT;
			}
		}
		pSub_seq_cmd++;
	} while (1);


	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_parse_intf_phy_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_INTF_PHY_PARA pIntf_phy_para,
	IN HALMAC_INTF_PHY_PLATFORM platform,
	IN HAL_INTF_PHY intf_phy
)
{
	u16 value;
	u16 curr_cut;
	u16 offset;
	u16 ip_sel;
	PHALMAC_INTF_PHY_PARA pCurr_phy_para;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;
	u8 result = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (pHalmac_adapter->chip_version) {
	case HALMAC_CHIP_VER_A_CUT:
		curr_cut = (u16)HALMAC_INTF_PHY_CUT_A;
		break;
	case HALMAC_CHIP_VER_B_CUT:
		curr_cut = (u16)HALMAC_INTF_PHY_CUT_B;
		break;
	case HALMAC_CHIP_VER_C_CUT:
		curr_cut = (u16)HALMAC_INTF_PHY_CUT_C;
		break;
	case HALMAC_CHIP_VER_D_CUT:
		curr_cut = (u16)HALMAC_INTF_PHY_CUT_D;
		break;
	case HALMAC_CHIP_VER_E_CUT:
		curr_cut = (u16)HALMAC_INTF_PHY_CUT_E;
		break;
	case HALMAC_CHIP_VER_F_CUT:
		curr_cut = (u16)HALMAC_INTF_PHY_CUT_F;
		break;
	case HALMAC_CHIP_VER_TEST:
		curr_cut = (u16)HALMAC_INTF_PHY_CUT_TESTCHIP;
		break;
	default:
		return HALMAC_RET_FAIL;
	}

	pCurr_phy_para = pIntf_phy_para;

	do {
		if ((pCurr_phy_para->cut & curr_cut) && (pCurr_phy_para->plaform & (u16)platform)) {
			offset =  pCurr_phy_para->offset;
			value = pCurr_phy_para->value;
			ip_sel = pCurr_phy_para->ip_sel;

			if (offset == 0xFFFF)
				break;

			if (ip_sel == HALMAC_IP_SEL_MAC) {
				HALMAC_REG_WRITE_8(pHalmac_adapter, (u32)offset, (u8)value);
			} else if (intf_phy == HAL_INTF_PHY_USB2) {
				result = halmac_usbphy_write_88xx(pHalmac_adapter, (u8)offset, value, HAL_INTF_PHY_USB2);

				if (result != HALMAC_RET_SUCCESS)
					PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_USB, HALMAC_DBG_ERR, "[ERR]Write USB2PHY fail!\n");

			} else if (intf_phy == HAL_INTF_PHY_USB3) {
				result = halmac_usbphy_write_88xx(pHalmac_adapter, (u8)offset, value, HAL_INTF_PHY_USB3);

				if (result != HALMAC_RET_SUCCESS)
					PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_USB, HALMAC_DBG_ERR, "[ERR]Write USB3PHY fail!\n");

			} else if (intf_phy == HAL_INTF_PHY_PCIE_GEN1) {
				if (ip_sel == HALMAC_IP_SEL_INTF_PHY)
					result = halmac_mdio_write_88xx(pHalmac_adapter, (u8)offset, value, HAL_INTF_PHY_PCIE_GEN1);
				else
					result = halmac_dbi_write8_88xx(pHalmac_adapter, offset, (u8)value);

				if (result != HALMAC_RET_SUCCESS)
					PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_MDIO, HALMAC_DBG_ERR, "[ERR]MDIO write GEN1 fail!\n");

			} else if (intf_phy == HAL_INTF_PHY_PCIE_GEN2) {
				if (ip_sel == HALMAC_IP_SEL_INTF_PHY)
					result = halmac_mdio_write_88xx(pHalmac_adapter, (u8)offset, value, HAL_INTF_PHY_PCIE_GEN2);
				else
					result = halmac_dbi_write8_88xx(pHalmac_adapter, offset, (u8)value);

				if (result != HALMAC_RET_SUCCESS)
					PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_MDIO, HALMAC_DBG_ERR, "[ERR]MDIO write GEN2 fail!\n");
			} else {
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]Parse intf phy cfg error!\n");
			}
		}
		pCurr_phy_para++;
	} while (1);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_txfifo_is_empty_88xx() -check if txfifo is empty
 * @pHalmac_adapter : the adapter of halmac
 * @chk_num : check number
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_txfifo_is_empty_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 chk_num
)
{
	u32 counter;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_txfifo_is_empty_88xx ==========>\n");

	counter = (chk_num <= 10) ? 10 : chk_num;
	do {
		if (HALMAC_REG_READ_8(pHalmac_adapter, REG_TXPKT_EMPTY) != 0xFF)
			return HALMAC_RET_TXFIFO_NO_EMPTY;

		if ((HALMAC_REG_READ_8(pHalmac_adapter, REG_TXPKT_EMPTY + 1) & 0x06) != 0x06)
			return HALMAC_RET_TXFIFO_NO_EMPTY;
		counter--;

	} while (counter != 0);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_txfifo_is_empty_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * (internal use)
 * halmac_adaptive_malloc_88xx() - adapt malloc size
 * @pHalmac_adapter : the adapter of halmac
 * @size : expected malloc size
 * @pNew_size : real malloc size
 * Author : Ivan Lin
 * Return : address pointer
 */
u8*
halmac_adaptive_malloc_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 size,
	OUT u32 *pNew_size
)
{
	VOID *pDriver_adapter = NULL;
	u8 retry_num;
	u8 *pMalloc_buf = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	for (retry_num = 0; retry_num < 5; retry_num++) {
		pMalloc_buf = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, size);

		if (pMalloc_buf != NULL) {
			*pNew_size = size;
			return pMalloc_buf;
		}

		size = size >> 1;

		if (size == 0)
			break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_adaptive_malloc fail!!\n");

	return NULL;
}

/**
 * (internal use)
 * halmac_ltecoex_reg_read_88xx() - read ltecoex register
 * @pHalmac_adapter : the adapter of halmac
 * @offset : offset
 * @pValue : value
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_ltecoex_reg_read_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 offset,
	OUT u32 *pValue
)
{
	u32 counter;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	counter = 10000;
	while ((HALMAC_REG_READ_8(pHalmac_adapter, REG_WL2LTECOEX_INDIRECT_ACCESS_CTRL_V1 + 3) & BIT(5)) == 0) {
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Check ltecoex ready bit fail(R)\n");
			return HALMAC_RET_LTECOEX_READY_FAIL;
		}
		counter--;
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 50);
	}

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WL2LTECOEX_INDIRECT_ACCESS_CTRL_V1, 0x800F0000 | offset);
	*pValue = HALMAC_REG_READ_32(pHalmac_adapter, REG_WL2LTECOEX_INDIRECT_ACCESS_READ_DATA_V1);

	return HALMAC_RET_SUCCESS;
}

/**
 * (internal use)
 * halmac_ltecoex_reg_write_88xx() - write ltecoex register
 * @pHalmac_adapter : the adapter of halmac
 * @offset : offset
 * @value : value
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_ltecoex_reg_write_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 offset,
	IN u32 value
)
{
	u32 counter;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	counter = 10000;
	while ((HALMAC_REG_READ_8(pHalmac_adapter, REG_WL2LTECOEX_INDIRECT_ACCESS_CTRL_V1 + 3) & BIT(5)) == 0) {
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Check ltecoex ready bit fail(W)\n");
			return HALMAC_RET_LTECOEX_READY_FAIL;
		}
		counter--;
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 50);
	}

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WL2LTECOEX_INDIRECT_ACCESS_WRITE_DATA_V1, value);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WL2LTECOEX_INDIRECT_ACCESS_CTRL_V1, 0xC00F0000 | offset);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_download_flash_88xx() -download firmware to flash
 * @pHalmac_adapter : the adapter of halmac
 * @pHalmac_fw : pointer to fw
 * @halmac_fw_size : fw size
 * @rom_address : flash start address where fw should be download
 * Author : Pablo Chiu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_download_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u8 *pHalmac_fw,
	IN u32 halmac_fw_size,
	IN u32 rom_address
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	u8 value8;
	u8 restore[3];
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = {0};
	u16 h2c_seq_mum = 0;
	u32 send_pkt_size, mem_offset;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_download_flash_88xx ==========>\n");

	pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_NONE;

	value8 = HALMAC_DMA_MAPPING_HIGH << 6;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_PQ_MAP + 1, value8);

	/* DLFW only use HIQ, map HIQ to hi priority */
	pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI] = HALMAC_DMA_MAPPING_HIGH;
	value8 = BIT_HCI_TXDMA_EN | BIT_TXDMA_EN;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2CQ_CSR, BIT(31));

	/* Config hi priority queue and public priority queue page number (only for DLFW) */
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_1, 0x200);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RQPN_CTRL_2, HALMAC_REG_READ_32(pHalmac_adapter, REG_RQPN_CTRL_2) | BIT(31));

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_FREE_TXPG);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SDIO_TX_CTRL, 0x00000000);
	}

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR + 1);
	restore[0] = value8;
	value8 = (u8)(value8 | BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, value8);
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL);
	restore[1] = value8;
	value8 = (u8)((value8 & (~(BIT(3)))) | BIT(4));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, value8);
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2);
	restore[2] = value8;
	value8 = (u8)(value8 & ~(BIT(6)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, value8);

	/* Download FW to Flash flow */
	mem_offset = 0;

	while (halmac_fw_size != 0) {
		if (halmac_fw_size >= (HALMAC_EXTRA_INFO_BUFF_SIZE_88XX - 48))
			send_pkt_size = HALMAC_EXTRA_INFO_BUFF_SIZE_88XX - 48;
		else
			send_pkt_size = halmac_fw_size;

		status = halmac_download_rsvd_page_88xx(pHalmac_adapter, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy,
															pHalmac_fw + mem_offset, send_pkt_size);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_download_rsvd_page_88xx Fail = %x!!\n", status);
			goto DLFW_FAIL;
		} else {
			/* Construct H2C Content */
			DOWNLOAD_FLASH_SET_SPI_CMD(pH2c_buff, 0x02);
			DOWNLOAD_FLASH_SET_LOCATION(pH2c_buff, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy - pHalmac_adapter->txff_allocation.rsvd_pg_bndy);
			DOWNLOAD_FLASH_SET_SIZE(pH2c_buff, send_pkt_size);
			DOWNLOAD_FLASH_SET_START_ADDR(pH2c_buff, rom_address);

			/* Fill in H2C Header */
			h2c_header_info.sub_cmd_id = SUB_CMD_ID_DOWNLOAD_FLASH;
			h2c_header_info.content_size = 20;
			h2c_header_info.ack = _TRUE;
			halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

			/* Send H2C Cmd Packet */
			status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);
			if (status != HALMAC_RET_SUCCESS) {
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail!!\n");
				goto DLFW_FAIL;
			}

			value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I);
			value8 |= BIT(0);
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUTST_I, value8);
		}

		rom_address += send_pkt_size;
		mem_offset += send_pkt_size;
		halmac_fw_size -= send_pkt_size;

		while (((HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I)) & BIT(0)) != 0)
			PLATFORM_RTL_DELAY_US(pDriver_adapter, 1000);

		if (((HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I)) & BIT(0)) != 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]download flash fail!!\n");
			goto DLFW_FAIL;
		}
	}

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, restore[2]);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, restore[1]);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, restore[0]);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_download_flash_88xx <==========\n");
	return HALMAC_RET_SUCCESS;

DLFW_FAIL:

	return HALMAC_RET_DLFW_FAIL;
}

/**
 * halmac_read_flash_88xx() -read data from flash
 * @pHalmac_adapter : the adapter of halmac
 * @addr : flash start address where fw should be read
 * Author : Pablo Chiu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_read_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	u32 addr
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	u8 value8;
	u8 restore[3];
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = {0};
	u16 h2c_seq_mum = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	pDriver_adapter =  pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_download_flash_88xx ==========>\n");

	pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_NONE;

	value8 = HALMAC_DMA_MAPPING_HIGH << 6;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_PQ_MAP + 1, value8);

	/* DLFW only use HIQ, map HIQ to hi priority */
	pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI] = HALMAC_DMA_MAPPING_HIGH;
	value8 = BIT_HCI_TXDMA_EN | BIT_TXDMA_EN;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2CQ_CSR, BIT(31));

	/* Config hi priority queue and public priority queue page number (only for DLFW) */
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_1, 0x200);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RQPN_CTRL_2, HALMAC_REG_READ_32(pHalmac_adapter, REG_RQPN_CTRL_2) | BIT(31));

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_FREE_TXPG);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SDIO_TX_CTRL, 0x00000000);
	}

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR + 1);
	restore[0] = value8;
	value8 = (u8)(value8 | BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, value8);
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL);
	restore[1] = value8;
	value8 = (u8)((value8 & (~(BIT(3)))) | BIT(4));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, value8);
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2);
	restore[2] = value8;
	value8 = (u8)(value8 & ~(BIT(6)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, value8);

	/* Set beacon header to  0 */
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, 0x8000);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I);
	value8 |= BIT(0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUTST_I, value8);

	/* Construct H2C Content */
	DOWNLOAD_FLASH_SET_SPI_CMD(pH2c_buff, 0x03);
	DOWNLOAD_FLASH_SET_LOCATION(pH2c_buff, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy - pHalmac_adapter->txff_allocation.rsvd_pg_bndy);
	DOWNLOAD_FLASH_SET_SIZE(pH2c_buff, 4096);
	DOWNLOAD_FLASH_SET_START_ADDR(pH2c_buff, addr);

	/* Fill in H2C Header */
	h2c_header_info.sub_cmd_id = SUB_CMD_ID_DOWNLOAD_FLASH;
	h2c_header_info.content_size = 16;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	/* Send H2C Cmd Packet */
	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail!!\n");
		goto DLFW_FAIL;
	}

	while (((HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I)) & BIT(0)) != 0)
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 1000);

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, restore[2]);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, restore[1]);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, restore[0]);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_download_flash_88xx <==========\n");
	return HALMAC_RET_SUCCESS;

DLFW_FAIL:

	return HALMAC_RET_FAIL;
}

/**
 * halmac_erase_flash_88xx() -erase flash data
 * @pHalmac_adapter : the adapter of halmac
 * @erase_cmd : erase command
 * @addr : flash start address where fw should be erased
 * Author : Pablo Chiu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_erase_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	u8 erase_cmd,
	u32 addr
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	PHALMAC_API pHalmac_api;
	u8 value8;
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = {0};
	u16 h2c_seq_mum = 0;
	u32 timeout;

	/* Construct H2C Content */
	DOWNLOAD_FLASH_SET_SPI_CMD(pH2c_buff, erase_cmd);
	DOWNLOAD_FLASH_SET_LOCATION(pH2c_buff, 0);
	DOWNLOAD_FLASH_SET_START_ADDR(pH2c_buff, addr);
	DOWNLOAD_FLASH_SET_SIZE(pH2c_buff, 0);

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I);
	value8 |= BIT(0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUTST_I, value8);

	/* Fill in H2C Header */
	h2c_header_info.sub_cmd_id = SUB_CMD_ID_DOWNLOAD_FLASH;
	h2c_header_info.content_size = 16;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	/* Send H2C Cmd Packet */
	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail!!\n");

	timeout = 5000;
	while ((((HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I)) & BIT(0)) != 0) && (timeout != 0)) {
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 1000);
		timeout--;
	}

	if (timeout == 0)
		return HALMAC_RET_FAIL;
	else
		return HALMAC_RET_SUCCESS;
}

/**
 * halmac_check_flash_88xx() -check flash data
 * @pHalmac_adapter : the adapter of halmac
 * @pHalmac_fw : pointer to fw
 * @halmac_fw_size : fw size
 * @addr : flash start address where fw should be checked
 * Author : Pablo Chiu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_check_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u8 *pHalmac_fw,
	IN u32 halmac_fw_size,
	IN u32 addr
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u8	value8;
	u16 value16, residue;
	u32 send_pkt_size, start_page, counter;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	pDriver_adapter =  pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	while (halmac_fw_size != 0) {
		start_page = ((pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy << 7) >> 12) + 0x780;
		residue = (pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy << 7) & (4096 - 1);

		if (halmac_fw_size >= HALMAC_EXTRA_INFO_BUFF_SIZE_88XX)
			send_pkt_size = HALMAC_EXTRA_INFO_BUFF_SIZE_88XX;
		else
			send_pkt_size = halmac_fw_size;

		halmac_read_flash_88xx(pHalmac_adapter, addr);

		value16 = HALMAC_REG_READ_16(pHalmac_adapter, REG_PKTBUF_DBG_CTRL);
		counter = 0;
		while (counter < send_pkt_size) {
			HALMAC_REG_WRITE_16(pHalmac_adapter, REG_PKTBUF_DBG_CTRL, (u16)(start_page | (value16 & 0xF000)));
			for (value16 = 0x8000 + residue; value16 <= 0x8FFF; value16++) {
				value8 = HALMAC_REG_READ_8(pHalmac_adapter, value16);

				if (*pHalmac_fw != value8) {
					PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]check flash fail!!\n");
					goto DLFW_FAIL;
				}
				pHalmac_fw++;

				counter++;
				if (counter == send_pkt_size)
					break;
			}
			residue = 0;
			start_page++;
		}
		addr += send_pkt_size;
		halmac_fw_size -= send_pkt_size;
	}

	return HALMAC_RET_SUCCESS;

DLFW_FAIL:

	return HALMAC_RET_FAIL;
}

#endif /* HALMAC_88XX_SUPPORT */

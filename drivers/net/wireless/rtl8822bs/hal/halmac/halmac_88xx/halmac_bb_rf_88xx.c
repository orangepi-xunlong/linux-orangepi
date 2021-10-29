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

#include "halmac_bb_rf_88xx.h"
#include "halmac_88xx_cfg.h"
#include "halmac_common_88xx.h"
#include "halmac_init_88xx.h"

#if HALMAC_88XX_SUPPORT

/**
 * halmac_start_iqk_88xx() -trigger FW IQK
 * @pHalmac_adapter : the adapter of halmac
 * @pIqk_para : IQK parameter
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_start_iqk_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_IQK_PARA pIqk_para
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_num = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.iqk_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_start_iqk_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Wait event(iqk)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	IQK_SET_CLEAR(pH2c_buff, pIqk_para->clear);
	IQK_SET_SEGMENT_IQK(pH2c_buff, pIqk_para->segment_iqk);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_IQK;
	h2c_header_info.content_size = 1;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_num);

	pHalmac_adapter->halmac_state.iqk_set.seq_num = h2c_seq_num;

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);
		halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_IQK);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_start_iqk_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_ctrl_pwr_tracking_88xx() -trigger FW power tracking
 * @pHalmac_adapter : the adapter of halmac
 * @pPwr_tracking_opt : power tracking option
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_ctrl_pwr_tracking_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PWR_TRACKING_OPTION pPwr_tracking_opt
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.power_tracking_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_start_iqk_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Wait event(pwr tracking)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	POWER_TRACKING_SET_TYPE(pH2c_buff, pPwr_tracking_opt->type);
	POWER_TRACKING_SET_BBSWING_INDEX(pH2c_buff, pPwr_tracking_opt->bbswing_index);
	POWER_TRACKING_SET_ENABLE_A(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_A].enable);
	POWER_TRACKING_SET_TX_PWR_INDEX_A(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_A].tx_pwr_index);
	POWER_TRACKING_SET_PWR_TRACKING_OFFSET_VALUE_A(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_A].pwr_tracking_offset_value);
	POWER_TRACKING_SET_TSSI_VALUE_A(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_A].tssi_value);
	POWER_TRACKING_SET_ENABLE_B(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_B].enable);
	POWER_TRACKING_SET_TX_PWR_INDEX_B(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_B].tx_pwr_index);
	POWER_TRACKING_SET_PWR_TRACKING_OFFSET_VALUE_B(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_B].pwr_tracking_offset_value);
	POWER_TRACKING_SET_TSSI_VALUE_B(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_B].tssi_value);
	POWER_TRACKING_SET_ENABLE_C(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_C].enable);
	POWER_TRACKING_SET_TX_PWR_INDEX_C(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_C].tx_pwr_index);
	POWER_TRACKING_SET_PWR_TRACKING_OFFSET_VALUE_C(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_C].pwr_tracking_offset_value);
	POWER_TRACKING_SET_TSSI_VALUE_C(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_C].tssi_value);
	POWER_TRACKING_SET_ENABLE_D(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_D].enable);
	POWER_TRACKING_SET_TX_PWR_INDEX_D(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_D].tx_pwr_index);
	POWER_TRACKING_SET_PWR_TRACKING_OFFSET_VALUE_D(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_D].pwr_tracking_offset_value);
	POWER_TRACKING_SET_TSSI_VALUE_D(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_D].tssi_value);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_POWER_TRACKING;
	h2c_header_info.content_size = 20;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	pHalmac_adapter->halmac_state.power_tracking_set.seq_num = h2c_seq_mum;

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);
		halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_POWER_TRACKING);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_start_iqk_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_query_iqk_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	PHALMAC_IQK_STATE_SET pIqk_set = &pHalmac_adapter->halmac_state.iqk_set;

	*pProcess_status = pIqk_set->process_status;

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_query_power_tracking_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	PHALMAC_POWER_TRACKING_STATE_SET pPower_tracking_state_set = &pHalmac_adapter->halmac_state.power_tracking_set;

	*pProcess_status = pPower_tracking_state_set->process_status;

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_query_psd_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_PSD_STATE_SET pPsd_set = &pHalmac_adapter->halmac_state.psd_set;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	*pProcess_status = pPsd_set->process_status;

	if (data == NULL)
		return HALMAC_RET_NULL_POINTER;

	if (size == NULL)
		return HALMAC_RET_NULL_POINTER;

	if (*pProcess_status == HALMAC_CMD_PROCESS_DONE) {
		if (*size < pPsd_set->data_size) {
			*size = pPsd_set->data_size;
			return HALMAC_RET_BUFFER_TOO_SMALL;
		}

		*size = pPsd_set->data_size;
		PLATFORM_RTL_MEMCPY(pDriver_adapter, data, pPsd_set->pData, *size);
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_psd_88xx() - trigger fw psd
 * @pHalmac_adapter : the adapter of halmac
 * @start_psd : start PSD
 * @end_psd : end PSD
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_psd_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 start_psd,
	IN u16 end_psd
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.psd_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_psd_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Wait event(psd)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (pHalmac_adapter->halmac_state.psd_set.pData != NULL) {
		PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter->halmac_state.psd_set.pData, pHalmac_adapter->halmac_state.psd_set.data_size);
		pHalmac_adapter->halmac_state.psd_set.pData = (u8 *)NULL;
	}

	pHalmac_adapter->halmac_state.psd_set.data_size = 0;
	pHalmac_adapter->halmac_state.psd_set.segment_size = 0;

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	PSD_SET_START_PSD(pH2c_buff, start_psd);
	PSD_SET_END_PSD(pH2c_buff, end_psd);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_PSD;
	h2c_header_info.content_size = 4;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);
		halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_PSD);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_psd_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_88XX_SUPPORT */

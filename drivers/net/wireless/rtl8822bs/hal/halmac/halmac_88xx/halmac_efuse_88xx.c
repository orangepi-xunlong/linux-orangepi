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

#include "halmac_efuse_88xx.h"
#include "halmac_88xx_cfg.h"
#include "halmac_common_88xx.h"
#include "halmac_init_88xx.h"

#if HALMAC_88XX_SUPPORT

static HALMAC_EFUSE_CMD_CONSTRUCT_STATE
halmac_query_efuse_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_dump_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_READ_CFG cfg
);

static HALMAC_RET_STATUS
halmac_read_hw_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u32 size,
	OUT u8 *pEfuse_map
);

static HALMAC_RET_STATUS
halmac_eeprom_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pPhysical_efuse_map,
	OUT u8 *pLogical_efuse_map
);

static HALMAC_RET_STATUS
halmac_read_logical_efuse_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pMap
);

static HALMAC_RET_STATUS
halmac_func_pg_efuse_by_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	IN HALMAC_EFUSE_READ_CFG cfg
);

static HALMAC_RET_STATUS
halmac_dump_efuse_fw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_dump_efuse_drv_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_func_write_logical_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u8 value
);

static HALMAC_RET_STATUS
halmac_update_eeprom_mask_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	OUT u8 *pEeprom_mask_updated
);

static HALMAC_RET_STATUS
halmac_check_efuse_enough_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	IN u8 *pEeprom_mask_updated
);

static HALMAC_RET_STATUS
halmac_program_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	IN u8 *pEeprom_mask_updated
);

/**
 * halmac_dump_efuse_map_88xx() - dump "physical" efuse map
 * @pHalmac_adapter : the adapter of halmac
 * @cfg : dump efuse method
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dump_efuse_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_READ_CFG cfg
)
{
	u8 *pEfuse_map = NULL;
	u32 efuse_size = pHalmac_adapter->hw_config_info.efuse_size;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (cfg == HALMAC_EFUSE_R_FW) {
		if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
			return HALMAC_RET_NO_DLFW;
	}

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_efuse_map_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]cfg=%d\n", cfg);

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Wait event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_WARN, "[WARN]Dump efuse in suspend mode\n");

	*pProcess_status = HALMAC_CMD_PROCESS_IDLE;
	pHalmac_adapter->event_trigger.physical_efuse_map = 1;

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_dump_efuse_88xx(pHalmac_adapter, cfg);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_read_efuse error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->hal_efuse_map_valid == _TRUE) {
		*pProcess_status = HALMAC_CMD_PROCESS_DONE;

		pEfuse_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, efuse_size);
		if (pEfuse_map == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local efuse map Fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
		}
		PLATFORM_RTL_MEMSET(pDriver_adapter, pEfuse_map, 0xFF, efuse_size);
		PLATFORM_MUTEX_LOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pEfuse_map, pHalmac_adapter->pHalEfuse_map, efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX);
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pEfuse_map + efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX + HALMAC_RESERVED_CS_EFUSE_SIZE_88XX,
			pHalmac_adapter->pHalEfuse_map + efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX + HALMAC_RESERVED_CS_EFUSE_SIZE_88XX,
			HALMAC_PROTECTED_EFUSE_SIZE_88XX - HALMAC_RESERVED_EFUSE_SIZE_88XX - HALMAC_RESERVED_CS_EFUSE_SIZE_88XX);
		PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);

		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE, *pProcess_status, pEfuse_map, efuse_size);
		pHalmac_adapter->event_trigger.physical_efuse_map = 0;

		PLATFORM_RTL_FREE(pDriver_adapter, pEfuse_map, efuse_size);
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_efuse_map_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_dump_efuse_map_bt_88xx() - dump "BT physical" efuse map
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_efuse_bank : bt efuse bank
 * @bt_efuse_map_size : bt efuse map size. get from halmac_get_efuse_size API
 * @pBT_efuse_map : bt efuse map
 * Author : Soar / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dump_efuse_map_bt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_BANK halmac_efuse_bank,
	IN u32 bt_efuse_map_size,
	OUT u8 *pBT_efuse_map
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_efuse_map_bt_88xx ==========>\n");

	if (pHalmac_adapter->hw_config_info.bt_efuse_size != bt_efuse_map_size)
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;

	if ((halmac_efuse_bank >= HALMAC_EFUSE_BANK_MAX) || (halmac_efuse_bank == HALMAC_EFUSE_BANK_WIFI)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]Undefined BT bank\n");
		return HALMAC_RET_EFUSE_BANK_INCORRECT;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Wait event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, halmac_efuse_bank);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_read_hw_efuse_88xx(pHalmac_adapter, 0, bt_efuse_map_size, pBT_efuse_map);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_read_hw_efuse_88xx error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_efuse_map_bt_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_write_efuse_bt_88xx() - write "BT physical" efuse offset
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : offset
 * @halmac_value : Write value
 * @pBT_efuse_map : bt efuse map
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_write_efuse_bt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u8 halmac_value,
	IN HALMAC_EFUSE_BANK halmac_efuse_bank
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;


	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_write_efuse_bt_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]offset : %X value : %X Bank : %X\n", halmac_offset, halmac_value, halmac_efuse_bank);

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (halmac_offset >= pHalmac_adapter->hw_config_info.efuse_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]Offset is too large\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if ((halmac_efuse_bank > HALMAC_EFUSE_BANK_MAX) || (halmac_efuse_bank == HALMAC_EFUSE_BANK_WIFI)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]Undefined BT bank\n");
		return HALMAC_RET_EFUSE_BANK_INCORRECT;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, halmac_efuse_bank);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_func_write_efuse_88xx(pHalmac_adapter, halmac_offset, halmac_value);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_write_efuse error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_write_efuse_bt_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_read_efuse_bt_88xx() - read "BT physical" efuse offset
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : offset
 * @pValue : 1 byte efuse value
 * @HALMAC_EFUSE_BANK : efuse bank
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_read_efuse_bt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	OUT u8 *pValue,
	IN HALMAC_EFUSE_BANK halmac_efuse_bank
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_read_efuse_bt_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (halmac_offset >= pHalmac_adapter->hw_config_info.efuse_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]Offset is too large\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if ((halmac_efuse_bank > HALMAC_EFUSE_BANK_MAX) || (halmac_efuse_bank == HALMAC_EFUSE_BANK_WIFI)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]Undefined BT bank\n");
		return HALMAC_RET_EFUSE_BANK_INCORRECT;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, halmac_efuse_bank);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_func_read_efuse_88xx(pHalmac_adapter, halmac_offset, 1, pValue);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_read_efuse error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_read_efuse_bt_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_efuse_auto_check_88xx() - check efuse after writing it
 * @pHalmac_adapter : the adapter of halmac
 * @enable : 1, enable efuse auto check. others, disable
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_efuse_auto_check_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_efuse_auto_check_88xx ==========> function enable = %d\n", enable);

	pHalmac_adapter->efuse_auto_check_en = enable;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_efuse_auto_check_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_efuse_available_size_88xx() - get efuse available size
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_size : physical efuse available size
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_efuse_available_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u32 *halmac_size
)
{
	HALMAC_RET_STATUS status;
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_get_efuse_available_size_88xx ==========>\n");

	status = halmac_dump_logical_efuse_map_88xx(pHalmac_adapter, HALMAC_EFUSE_R_DRV);

	if (status != HALMAC_RET_SUCCESS)
		return status;

	*halmac_size = pHalmac_adapter->hw_config_info.efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX - pHalmac_adapter->efuse_end;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_get_efuse_available_size_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_efuse_size_88xx() - get "physical" efuse size
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_size : physical efuse size
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_efuse_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u32 *halmac_size
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_get_efuse_size_88xx ==========>\n");

	*halmac_size = pHalmac_adapter->hw_config_info.efuse_size;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_get_efuse_size_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_logical_efuse_size_88xx() - get "logical" efuse size
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_size : logical efuse size
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_logical_efuse_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u32 *halmac_size
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_get_logical_efuse_size_88xx ==========>\n");

	*halmac_size = pHalmac_adapter->hw_config_info.eeprom_size;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_get_logical_efuse_size_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_dump_logical_efuse_map_88xx() - dump "logical" efuse map
 * @pHalmac_adapter : the adapter of halmac
 * @cfg : dump efuse method
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dump_logical_efuse_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_READ_CFG cfg
)
{
	u8 *pEeprom_map = NULL;
	u32 eeprom_size = pHalmac_adapter->hw_config_info.eeprom_size;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (cfg == HALMAC_EFUSE_R_FW) {
		if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
			return HALMAC_RET_NO_DLFW;
	}

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_logical_efuse_map_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]cfg = %d\n", cfg);

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_WARN, "[WARN]Dump logical efuse in suspend mode\n");

	*pProcess_status = HALMAC_CMD_PROCESS_IDLE;
	pHalmac_adapter->event_trigger.logical_efuse_map = 1;

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_dump_efuse_88xx(pHalmac_adapter, cfg);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_eeprom_parser_88xx error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->hal_efuse_map_valid == _TRUE) {
		*pProcess_status = HALMAC_CMD_PROCESS_DONE;

		pEeprom_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, eeprom_size);
		if (pEeprom_map == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local eeprom map Fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
		}
		PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_map, 0xFF, eeprom_size);

		if (halmac_eeprom_parser_88xx(pHalmac_adapter, pHalmac_adapter->pHalEfuse_map, pEeprom_map) != HALMAC_RET_SUCCESS) {
			PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
			return HALMAC_RET_EEPROM_PARSING_FAIL;
		}

		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_DUMP_LOGICAL_EFUSE, *pProcess_status, pEeprom_map, eeprom_size);
		pHalmac_adapter->event_trigger.logical_efuse_map = 0;

		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_logical_efuse_map_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_read_logical_efuse_88xx() - read logical efuse map 1 byte
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : offset
 * @pValue : 1 byte efuse value
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_read_logical_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	OUT u8 *pValue
)
{
	u8 *pEeprom_map = NULL;
	u32 eeprom_size = pHalmac_adapter->hw_config_info.eeprom_size;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_read_logical_efuse_88xx ==========>\n");

	if (halmac_offset >= eeprom_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]Offset is too large\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}
	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	pEeprom_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, eeprom_size);
	if (pEeprom_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local eeprom map Fail!!\n");
		return HALMAC_RET_MALLOC_FAIL;
	}
	PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_map, 0xFF, eeprom_size);

	status = halmac_read_logical_efuse_map_88xx(pHalmac_adapter, pEeprom_map);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_read_logical_efuse_map error = %x\n", status);
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
		return status;
	}

	*pValue = *(pEeprom_map + halmac_offset);

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS) {
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
		return HALMAC_RET_ERROR_STATE;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_read_logical_efuse_88xx <==========\n");

	PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_write_logical_efuse_88xx() - write "logical" efuse offset
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : offset
 * @halmac_value : value
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_write_logical_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u8 halmac_value
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_write_logical_efuse_88xx ==========>\n");

	if (halmac_offset >= pHalmac_adapter->hw_config_info.eeprom_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]Offset is too large\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_func_write_logical_efuse_88xx(pHalmac_adapter, halmac_offset, halmac_value);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_write_logical_efuse error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_write_logical_efuse_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pg_efuse_by_map_88xx() - pg logical efuse by map
 * @pHalmac_adapter : the adapter of halmac
 * @pPg_efuse_info : efuse map information
 * @cfg : dump efuse method
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pg_efuse_by_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	IN HALMAC_EFUSE_READ_CFG cfg
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_pg_efuse_by_map_88xx ==========>\n");

	if (pPg_efuse_info->efuse_map_size != pHalmac_adapter->hw_config_info.eeprom_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_map_size is incorrect, should be %d bytes\n", pHalmac_adapter->hw_config_info.eeprom_size);
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if ((pPg_efuse_info->efuse_map_size & 0xF) > 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_map_size should be multiple of 16\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (pPg_efuse_info->efuse_mask_size != pPg_efuse_info->efuse_map_size >> 4) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_mask_size is incorrect, should be %d bytes\n", pPg_efuse_info->efuse_map_size >> 4);
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (pPg_efuse_info->pEfuse_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_map is NULL\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (pPg_efuse_info->pEfuse_mask == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_mask is NULL\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_func_pg_efuse_by_map_88xx(pHalmac_adapter, pPg_efuse_info, cfg);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_pg_efuse_by_map error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_pg_efuse_by_map_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_mask_logical_efuse_88xx() - mask logical efuse
 * @pHalmac_adapter : the adapter of halmac
 * @pEfuse_info : efuse map information
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mask_logical_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT PHALMAC_PG_EFUSE_INFO pEfuse_info
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_mask_logical_efuse_88xx ==========>\n");

	if (pEfuse_info->efuse_map_size != pHalmac_adapter->hw_config_info.eeprom_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_map_size is incorrect, should be %d bytes\n", pHalmac_adapter->hw_config_info.eeprom_size);
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if ((pEfuse_info->efuse_map_size & 0xF) > 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_map_size should be multiple of 16\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (pEfuse_info->efuse_mask_size != pEfuse_info->efuse_map_size >> 4) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_mask_size is incorrect, should be %d bytes\n", pEfuse_info->efuse_map_size >> 4);
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (pEfuse_info->pEfuse_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_map is NULL\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (pEfuse_info->pEfuse_mask == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]efuse_mask is NULL\n");
		return HALMAC_RET_NULL_POINTER;
	}

	halmac_mask_eeprom_88xx(pHalmac_adapter, pEfuse_info);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "[TRACE]halmac_mask_logical_efuse_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_EFUSE_CMD_CONSTRUCT_STATE
halmac_query_efuse_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	return pHalmac_adapter->halmac_state.efuse_state_set.efuse_cmd_construct_state;
}

HALMAC_RET_STATUS
halmac_func_switch_efuse_bank_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_BANK efuse_bank
)
{
	u8 reg_value;
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_BUSY) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	reg_value = HALMAC_REG_READ_8(pHalmac_adapter, REG_LDO_EFUSE_CTRL + 1);

	if (efuse_bank == (reg_value & (BIT(0) | BIT(1))))
		return HALMAC_RET_SUCCESS;

	reg_value &= ~(BIT(0) | BIT(1));
	reg_value |= efuse_bank;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_LDO_EFUSE_CTRL + 1, reg_value);

	if ((HALMAC_REG_READ_8(pHalmac_adapter, REG_LDO_EFUSE_CTRL + 1) & (BIT(0) | BIT(1))) != efuse_bank)
		return HALMAC_RET_SWITCH_EFUSE_BANK_FAIL;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_dump_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_READ_CFG cfg
)
{
	u32 chk_h2c_init;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_H2C_SENT) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	if (cfg == HALMAC_EFUSE_R_AUTO) {
		chk_h2c_init = HALMAC_REG_READ_32(pHalmac_adapter, REG_H2C_PKT_READADDR);
		if (HALMAC_DLFW_NONE == pHalmac_adapter->halmac_state.dlfw_state || 0 == chk_h2c_init)
			status = halmac_dump_efuse_drv_88xx(pHalmac_adapter);
		else
			status = halmac_dump_efuse_fw_88xx(pHalmac_adapter);
	} else if (cfg == HALMAC_EFUSE_R_FW) {
		status = halmac_dump_efuse_fw_88xx(pHalmac_adapter);
	} else {
		status = halmac_dump_efuse_drv_88xx(pHalmac_adapter);
	}

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_read_efuse error = %x\n", status);
		return status;
	}

	return status;
}

HALMAC_RET_STATUS
halmac_transition_efuse_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_CMD_CONSTRUCT_STATE dest_state
)
{
	PHALMAC_EFUSE_STATE_SET pEfuse_state = &pHalmac_adapter->halmac_state.efuse_state_set;

	if ((pEfuse_state->efuse_cmd_construct_state != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE)
	    && (pEfuse_state->efuse_cmd_construct_state != HALMAC_EFUSE_CMD_CONSTRUCT_BUSY)
	    && (pEfuse_state->efuse_cmd_construct_state != HALMAC_EFUSE_CMD_CONSTRUCT_H2C_SENT))
		return HALMAC_RET_ERROR_STATE;

	if (pEfuse_state->efuse_cmd_construct_state == dest_state)
		return HALMAC_RET_ERROR_STATE;

	if (dest_state == HALMAC_EFUSE_CMD_CONSTRUCT_BUSY) {
		if (pEfuse_state->efuse_cmd_construct_state == HALMAC_EFUSE_CMD_CONSTRUCT_H2C_SENT)
			return HALMAC_RET_ERROR_STATE;
	} else if (dest_state == HALMAC_EFUSE_CMD_CONSTRUCT_H2C_SENT) {
		if (pEfuse_state->efuse_cmd_construct_state == HALMAC_EFUSE_CMD_CONSTRUCT_IDLE)
			return HALMAC_RET_ERROR_STATE;
	}

	pEfuse_state->efuse_cmd_construct_state = dest_state;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_read_hw_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u32 size,
	OUT u8 *pEfuse_map
)
{
	u8 value8;
	u32 value32;
	u32 address;
	u32 tmp32, counter;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	/* Read efuse no need 2.5V LDO */
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_LDO_EFUSE_CTRL + 3);
	if (value8 & BIT(7))
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_LDO_EFUSE_CTRL + 3, (u8)(value8 & ~(BIT(7))));

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_EFUSE_CTRL);

	for (address = offset; address < offset + size; address++) {
		value32 = value32 & ~((BIT_MASK_EF_DATA) | (BIT_MASK_EF_ADDR << BIT_SHIFT_EF_ADDR));
		value32 = value32 | ((address & BIT_MASK_EF_ADDR) << BIT_SHIFT_EF_ADDR);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_EFUSE_CTRL, value32 & (~BIT_EF_FLAG));

		counter = 1000000;
		do {
			PLATFORM_RTL_DELAY_US(pDriver_adapter, 1);
			tmp32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_EFUSE_CTRL);
			counter--;
			if (counter == 0) {
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]HALMAC_RET_EFUSE_R_FAIL\n");
				return HALMAC_RET_EFUSE_R_FAIL;
			}
		} while ((tmp32 & BIT_EF_FLAG) == 0);

		*(pEfuse_map + address - offset) = (u8)(tmp32 & BIT_MASK_EF_DATA);
	}

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_func_write_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u8 value
)
{
	const u8 wite_protect_code = 0x69;
	u32 value32, tmp32, counter;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u8 value_read = 0;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MUTEX_LOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);
	pHalmac_adapter->hal_efuse_map_valid = _FALSE;
	PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_PMC_DBG_CTRL2 + 3, wite_protect_code);

	/* Enable 2.5V LDO */
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_LDO_EFUSE_CTRL + 3, (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_LDO_EFUSE_CTRL + 3) | BIT(7)));

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_EFUSE_CTRL);
	value32 = value32 & ~((BIT_MASK_EF_DATA) | (BIT_MASK_EF_ADDR << BIT_SHIFT_EF_ADDR));
	value32 = value32 | ((offset & BIT_MASK_EF_ADDR) << BIT_SHIFT_EF_ADDR) | (value & BIT_MASK_EF_DATA);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_EFUSE_CTRL, value32 | BIT_EF_FLAG);

	counter = 1000000;
	do {
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 1);
		tmp32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_EFUSE_CTRL);
		counter--;
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_write_efuse Fail !!\n");
			return HALMAC_RET_EFUSE_W_FAIL;
		}
	} while (BIT_EF_FLAG == (tmp32 & BIT_EF_FLAG));

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_PMC_DBG_CTRL2 + 3, 0x00);

	/* Disable 2.5V LDO */
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_LDO_EFUSE_CTRL + 3, (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_LDO_EFUSE_CTRL + 3) & ~(BIT(7))));

	if (pHalmac_adapter->efuse_auto_check_en == 1) {
		if (halmac_read_hw_efuse_88xx(pHalmac_adapter, offset, 1, &value_read) != HALMAC_RET_SUCCESS)
			return HALMAC_RET_EFUSE_R_FAIL;
		if (value_read != value) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_write_efuse Fail: result 0x%X != write value 0x%X !!\n", value_read, value);
			return HALMAC_RET_EFUSE_W_FAIL;
		}
	}
	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_eeprom_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pPhysical_efuse_map,
	OUT u8 *pLogical_efuse_map
)
{
	u8 j;
	u8 value8;
	u8 block_index;
	u8 valid_word_enable, word_enable;
	u8 efuse_read_header, efuse_read_header2 = 0;
	u32 eeprom_index;
	u32 efuse_index = 0;
	u32 eeprom_size = pHalmac_adapter->hw_config_info.eeprom_size;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_RTL_MEMSET(pDriver_adapter, pLogical_efuse_map, 0xFF, eeprom_size);

	do {
		value8 = *(pPhysical_efuse_map + efuse_index);
		efuse_read_header = value8;

		if ((efuse_read_header & 0x1f) == 0x0f) {
			efuse_index++;
			value8 = *(pPhysical_efuse_map + efuse_index);
			efuse_read_header2 = value8;
			block_index = ((efuse_read_header2 & 0xF0) >> 1) | ((efuse_read_header >> 5) & 0x07);
			word_enable = efuse_read_header2 & 0x0F;
		} else {
			block_index = (efuse_read_header & 0xF0) >> 4;
			word_enable = efuse_read_header & 0x0F;
		}

		if (efuse_read_header == 0xff)
			break;

		efuse_index++;

		if (efuse_index >= pHalmac_adapter->hw_config_info.efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX - 1)
			return HALMAC_RET_EEPROM_PARSING_FAIL;

		for (j = 0; j < 4; j++) {
			valid_word_enable = (u8)((~(word_enable >> j)) & BIT(0));
			if (valid_word_enable == 1) {
				eeprom_index = (block_index << 3) + (j << 1);

				if ((eeprom_index + 1) > eeprom_size) {
					PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]EEPROM addr exceeds eeprom_size:0x%X, at eFuse 0x%X\n", eeprom_size, efuse_index - 1);
					if ((efuse_read_header & 0x1f) == 0x0f)
						PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]EEPROM header: 0x%X, 0x%X,\n", efuse_read_header, efuse_read_header2);
					else
						PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]EEPROM header: 0x%X,\n", efuse_read_header);

					return HALMAC_RET_EEPROM_PARSING_FAIL;
				}

				value8 = *(pPhysical_efuse_map + efuse_index);
				*(pLogical_efuse_map + eeprom_index) = value8;

				eeprom_index++;
				efuse_index++;

				if (efuse_index > pHalmac_adapter->hw_config_info.efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX - 1)
					return HALMAC_RET_EEPROM_PARSING_FAIL;

				value8 = *(pPhysical_efuse_map + efuse_index);
				*(pLogical_efuse_map + eeprom_index) = value8;

				efuse_index++;

				if (efuse_index > pHalmac_adapter->hw_config_info.efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX)
					return HALMAC_RET_EEPROM_PARSING_FAIL;
			}
		}
	} while (1);

	pHalmac_adapter->efuse_end = efuse_index;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_read_logical_efuse_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pMap
)
{
	u8 *pEfuse_map = NULL;
	u32 efuse_size;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	efuse_size = pHalmac_adapter->hw_config_info.efuse_size;

	if (pHalmac_adapter->hal_efuse_map_valid == _FALSE) {
		pEfuse_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, efuse_size);
		if (pEfuse_map == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local efuse map Fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
		}

		status = halmac_func_read_efuse_88xx(pHalmac_adapter, 0, efuse_size, pEfuse_map);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_read_efuse error = %x\n", status);
			PLATFORM_RTL_FREE(pDriver_adapter, pEfuse_map, efuse_size);
			return status;
		}

		if (pHalmac_adapter->pHalEfuse_map == NULL) {
			pHalmac_adapter->pHalEfuse_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, efuse_size);
			if (pHalmac_adapter->pHalEfuse_map == NULL) {
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate efuse map Fail!!\n");
				PLATFORM_RTL_FREE(pDriver_adapter, pEfuse_map, efuse_size);
				return HALMAC_RET_MALLOC_FAIL;
			}
		}

		PLATFORM_MUTEX_LOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pHalmac_adapter->pHalEfuse_map, pEfuse_map, efuse_size);
		pHalmac_adapter->hal_efuse_map_valid = _TRUE;
		PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);

		PLATFORM_RTL_FREE(pDriver_adapter, pEfuse_map, efuse_size);
	}

	if (halmac_eeprom_parser_88xx(pHalmac_adapter, pHalmac_adapter->pHalEfuse_map, pMap) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_EEPROM_PARSING_FAIL;

	return status;
}

static HALMAC_RET_STATUS
halmac_func_pg_efuse_by_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	IN HALMAC_EFUSE_READ_CFG cfg
)
{
	u8 *pEeprom_mask_updated = NULL;
	u32 eeprom_mask_size = pHalmac_adapter->hw_config_info.eeprom_size >> 4;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pEeprom_mask_updated = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, eeprom_mask_size);
	if (pEeprom_mask_updated == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local eeprom map Fail!!\n");
		return HALMAC_RET_MALLOC_FAIL;
	}
	PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_mask_updated, 0x00, eeprom_mask_size);

	status = halmac_update_eeprom_mask_88xx(pHalmac_adapter, pPg_efuse_info, pEeprom_mask_updated);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_update_eeprom_mask_88xx error = %x\n", status);
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_mask_updated, eeprom_mask_size);
		return status;
	}

	status = halmac_check_efuse_enough_88xx(pHalmac_adapter, pPg_efuse_info, pEeprom_mask_updated);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_check_efuse_enough_88xx error = %x\n", status);
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_mask_updated, eeprom_mask_size);
		return status;
	}

	status = halmac_program_efuse_88xx(pHalmac_adapter, pPg_efuse_info, pEeprom_mask_updated);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_program_efuse_88xx error = %x\n", status);
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_mask_updated, eeprom_mask_size);
		return status;
	}

	PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_mask_updated, eeprom_mask_size);

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_dump_efuse_drv_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 *pEfuse_map = NULL;
	u32 efuse_size;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	efuse_size = pHalmac_adapter->hw_config_info.efuse_size;

	if (pHalmac_adapter->pHalEfuse_map == NULL) {
		pHalmac_adapter->pHalEfuse_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, efuse_size);
		if (pHalmac_adapter->pHalEfuse_map == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate efuse map Fail!!\n");
			halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE);
			return HALMAC_RET_MALLOC_FAIL;
		}
	}

	if (pHalmac_adapter->hal_efuse_map_valid == _FALSE) {
		pEfuse_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, efuse_size);
		if (pEfuse_map == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local efuse map Fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
		}

		if (halmac_read_hw_efuse_88xx(pHalmac_adapter, 0, efuse_size, pEfuse_map) != HALMAC_RET_SUCCESS) {
			PLATFORM_RTL_FREE(pDriver_adapter, pEfuse_map, efuse_size);
			return HALMAC_RET_EFUSE_R_FAIL;
		}

		PLATFORM_MUTEX_LOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pHalmac_adapter->pHalEfuse_map, pEfuse_map, efuse_size);
		pHalmac_adapter->hal_efuse_map_valid = _TRUE;
		PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);

		PLATFORM_RTL_FREE(pDriver_adapter, pEfuse_map, efuse_size);
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_dump_efuse_fw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_DUMP_PHYSICAL_EFUSE;
	h2c_header_info.content_size = 0;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);
	pHalmac_adapter->halmac_state.efuse_state_set.seq_num = h2c_seq_mum;

	if (pHalmac_adapter->pHalEfuse_map == NULL) {
		pHalmac_adapter->pHalEfuse_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, pHalmac_adapter->hw_config_info.efuse_size);
		if (pHalmac_adapter->pHalEfuse_map == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac allocate efuse map Fail!!\n");
			halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE);
			return HALMAC_RET_MALLOC_FAIL;
		}
	}

	if (pHalmac_adapter->hal_efuse_map_valid == _FALSE) {
		status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_read_efuse_fw Fail = %x!!\n", status);
			halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE);
			return status;
		}
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_func_write_logical_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u8 value
)
{
	u8 pg_efuse_byte1, pg_efuse_byte2;
	u8 pg_block, pg_block_index;
	u8 pg_efuse_header, pg_efuse_header2;
	u8 *pEeprom_map = NULL;
	u32 eeprom_size = pHalmac_adapter->hw_config_info.eeprom_size;
	u32 efuse_end, pg_efuse_num;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	pEeprom_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, eeprom_size);
	if (pEeprom_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local eeprom map Fail!!\n");
		return HALMAC_RET_MALLOC_FAIL;
	}
	PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_map, 0xFF, eeprom_size);

	status = halmac_read_logical_efuse_map_88xx(pHalmac_adapter, pEeprom_map);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_read_logical_efuse_map_88xx error = %x\n", status);
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
		return status;
	}

	if (*(pEeprom_map + offset) != value) {
		efuse_end = pHalmac_adapter->efuse_end;
		pg_block = (u8)(offset >> 3);
		pg_block_index = (u8)((offset & (8 - 1)) >> 1);

		if (offset > 0x7f) {
			pg_efuse_header = (((pg_block & 0x07) << 5) & 0xE0) | 0x0F;
			pg_efuse_header2 = (u8)(((pg_block & 0x78) << 1) + ((0x1 << pg_block_index) ^ 0x0F));
		} else {
			pg_efuse_header = (u8)((pg_block << 4) + ((0x01 << pg_block_index) ^ 0x0F));
		}

		if ((offset & 1) == 0) {
			pg_efuse_byte1 = value;
			pg_efuse_byte2 = *(pEeprom_map + offset + 1);
		} else {
			pg_efuse_byte1 = *(pEeprom_map + offset - 1);
			pg_efuse_byte2 = value;
		}

		if (offset > 0x7f) {
			pg_efuse_num = 4;
			if (pHalmac_adapter->hw_config_info.efuse_size <= (pg_efuse_num + HALMAC_PROTECTED_EFUSE_SIZE_88XX + pHalmac_adapter->efuse_end)) {
				PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
				return HALMAC_RET_EFUSE_NOT_ENOUGH;
			}
			halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end, pg_efuse_header);
			halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end + 1, pg_efuse_header2);
			halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end + 2, pg_efuse_byte1);
			status = halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end + 3, pg_efuse_byte2);
		} else {
			pg_efuse_num = 3;
			if (pHalmac_adapter->hw_config_info.efuse_size <= (pg_efuse_num + HALMAC_PROTECTED_EFUSE_SIZE_88XX + pHalmac_adapter->efuse_end)) {
				PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
				return HALMAC_RET_EFUSE_NOT_ENOUGH;
			}
			halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end, pg_efuse_header);
			halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end + 1, pg_efuse_byte1);
			status = halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end + 2, pg_efuse_byte2);
		}

		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac_write_logical_efuse error = %x\n", status);
			PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
			return status;
		}
	}

	PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_func_read_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u32 size,
	OUT u8 *pEfuse_map
)
{
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (pEfuse_map == NULL) {
		PLATFORM_MSG_PRINT(pHalmac_adapter->pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]Malloc for dump efuse map error\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (pHalmac_adapter->hal_efuse_map_valid == _TRUE) {
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pEfuse_map, pHalmac_adapter->pHalEfuse_map + offset, size);
	} else {
		if (halmac_read_hw_efuse_88xx(pHalmac_adapter, offset, size, pEfuse_map) != HALMAC_RET_SUCCESS)
			return HALMAC_RET_EFUSE_R_FAIL;
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_update_eeprom_mask_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT PHALMAC_PG_EFUSE_INFO	pPg_efuse_info,
	OUT u8 *pEeprom_mask_updated
)
{
	u8 *pEeprom_map = NULL;
	u32 eeprom_size = pHalmac_adapter->hw_config_info.eeprom_size;
	u8 *pEeprom_map_pg, *pEeprom_mask;
	u16 i, j;
	u16 map_byte_offset, mask_byte_offset;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	pEeprom_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, eeprom_size);
	if (pEeprom_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local eeprom map Fail!!\n");
		return HALMAC_RET_MALLOC_FAIL;
	}
	PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_map, 0xFF, eeprom_size);

	PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_mask_updated, 0x00, pPg_efuse_info->efuse_mask_size);

	status = halmac_read_logical_efuse_map_88xx(pHalmac_adapter, pEeprom_map);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
		return status;
	}

	pEeprom_map_pg = pPg_efuse_info->pEfuse_map;
	pEeprom_mask = pPg_efuse_info->pEfuse_mask;


	for (i = 0; i < pPg_efuse_info->efuse_mask_size; i++)
		*(pEeprom_mask_updated + i) = *(pEeprom_mask + i);

	for (i = 0; i < pPg_efuse_info->efuse_map_size; i = i + 16) {
		for (j = 0; j < 16; j = j + 2) {
			map_byte_offset = i + j;
			mask_byte_offset = i >> 4;
			if (*(pEeprom_map_pg + map_byte_offset) == *(pEeprom_map + map_byte_offset)) {
				if (*(pEeprom_map_pg + map_byte_offset + 1) == *(pEeprom_map + map_byte_offset + 1)) {
					switch (j) {
					case 0:
						*(pEeprom_mask_updated + mask_byte_offset) = *(pEeprom_mask_updated + mask_byte_offset) & (BIT(4) ^ 0xFF);
						break;
					case 2:
						*(pEeprom_mask_updated + mask_byte_offset) = *(pEeprom_mask_updated + mask_byte_offset) & (BIT(5) ^ 0xFF);
						break;
					case 4:
						*(pEeprom_mask_updated + mask_byte_offset) = *(pEeprom_mask_updated + mask_byte_offset) & (BIT(6) ^ 0xFF);
						break;
					case 6:
						*(pEeprom_mask_updated + mask_byte_offset) = *(pEeprom_mask_updated + mask_byte_offset) & (BIT(7) ^ 0xFF);
						break;
					case 8:
						*(pEeprom_mask_updated + mask_byte_offset) = *(pEeprom_mask_updated + mask_byte_offset) & (BIT(0) ^ 0xFF);
						break;
					case 10:
						*(pEeprom_mask_updated + mask_byte_offset) = *(pEeprom_mask_updated + mask_byte_offset) & (BIT(1) ^ 0xFF);
						break;
					case 12:
						*(pEeprom_mask_updated + mask_byte_offset) = *(pEeprom_mask_updated + mask_byte_offset) & (BIT(2) ^ 0xFF);
						break;
					case 14:
						*(pEeprom_mask_updated + mask_byte_offset) = *(pEeprom_mask_updated + mask_byte_offset) & (BIT(3) ^ 0xFF);
						break;
					default:
						break;
					}
				}
			}
		}
	}

	PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);

	return status;
}

static HALMAC_RET_STATUS
halmac_check_efuse_enough_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	IN u8 *pEeprom_mask_updated
)
{
	u8 pre_word_enb, word_enb;
	u8 pg_efuse_header, pg_efuse_header2;
	u8 pg_block;
	u16 i, j;
	u32 efuse_end;
	u32 tmp_eeprom_offset, pg_efuse_num = 0;

	efuse_end = pHalmac_adapter->efuse_end;

	for (i = 0; i < pPg_efuse_info->efuse_map_size; i = i + 8) {
		tmp_eeprom_offset = i;

		if ((tmp_eeprom_offset & 7) > 0) {
			pre_word_enb = (*(pEeprom_mask_updated + (i >> 4)) & 0x0F);
			word_enb = pre_word_enb ^ 0x0F;
		} else {
			pre_word_enb = (*(pEeprom_mask_updated + (i >> 4)) >> 4);
			word_enb = pre_word_enb ^ 0x0F;
		}

		pg_block = (u8)(tmp_eeprom_offset >> 3);

		if (pre_word_enb > 0) {
			if (tmp_eeprom_offset > 0x7f) {
				pg_efuse_header = (((pg_block & 0x07) << 5) & 0xE0) | 0x0F;
				pg_efuse_header2 = (u8)(((pg_block & 0x78) << 1) + word_enb);
			} else {
				pg_efuse_header = (u8)((pg_block << 4) + word_enb);
			}

			if (tmp_eeprom_offset > 0x7f) {
				pg_efuse_num++;
				pg_efuse_num++;
				efuse_end = efuse_end + 2;
				for (j = 0; j < 4; j++) {
					if (((pre_word_enb >> j) & 0x1) > 0) {
						pg_efuse_num++;
						pg_efuse_num++;
						efuse_end = efuse_end + 2;
					}
				}
			} else {
				pg_efuse_num++;
				efuse_end = efuse_end + 1;
				for (j = 0; j < 4; j++) {
					if (((pre_word_enb >> j) & 0x1) > 0) {
						pg_efuse_num++;
						pg_efuse_num++;
						efuse_end = efuse_end + 2;
					}
				}
			}
		}
	}

	if (pHalmac_adapter->hw_config_info.efuse_size <= (pg_efuse_num + HALMAC_PROTECTED_EFUSE_SIZE_88XX + pHalmac_adapter->efuse_end))
		return HALMAC_RET_EFUSE_NOT_ENOUGH;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_program_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	IN u8 *pEeprom_mask_updated
)
{
	u8 pre_word_enb, word_enb;
	u8 pg_efuse_header, pg_efuse_header2;
	u8 pg_block;
	u16 i, j;
	u32 efuse_end;
	u32 tmp_eeprom_offset;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	efuse_end = pHalmac_adapter->efuse_end;

	for (i = 0; i < pPg_efuse_info->efuse_map_size; i = i + 8) {
		tmp_eeprom_offset = i;

		if (((tmp_eeprom_offset >> 3) & 1) > 0) {
			pre_word_enb = (*(pEeprom_mask_updated + (i >> 4)) & 0x0F);
			word_enb = pre_word_enb ^ 0x0F;
		} else {
			pre_word_enb = (*(pEeprom_mask_updated + (i >> 4)) >> 4);
			word_enb = pre_word_enb ^ 0x0F;
		}

		pg_block = (u8)(tmp_eeprom_offset >> 3);

		if (pre_word_enb > 0) {
			if (tmp_eeprom_offset > 0x7f) {
				pg_efuse_header = (((pg_block & 0x07) << 5) & 0xE0) | 0x0F;
				pg_efuse_header2 = (u8)(((pg_block & 0x78) << 1) + word_enb);
			} else {
				pg_efuse_header = (u8)((pg_block << 4) + word_enb);
			}

			if (tmp_eeprom_offset > 0x7f) {
				halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end, pg_efuse_header);
				status = halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end + 1, pg_efuse_header2);
				efuse_end = efuse_end + 2;
				for (j = 0; j < 4; j++) {
					if (((pre_word_enb >> j) & 0x1) > 0) {
						halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end, *(pPg_efuse_info->pEfuse_map + tmp_eeprom_offset + (j << 1)));
						status = halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end + 1, *(pPg_efuse_info->pEfuse_map + tmp_eeprom_offset + (j << 1) + 1));
						efuse_end = efuse_end + 2;
					}
				}
			} else {
				status = halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end, pg_efuse_header);
				efuse_end = efuse_end + 1;
				for (j = 0; j < 4; j++) {
					if (((pre_word_enb >> j) & 0x1) > 0) {
						halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end, *(pPg_efuse_info->pEfuse_map + tmp_eeprom_offset + (j << 1)));
						status = halmac_func_write_efuse_88xx(pHalmac_adapter, efuse_end + 1, *(pPg_efuse_info->pEfuse_map + tmp_eeprom_offset + (j << 1) + 1));
						efuse_end = efuse_end + 2;
					}
				}
			}
		}
	}

	return status;
}

VOID
halmac_mask_eeprom_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT PHALMAC_PG_EFUSE_INFO	pEfuse_info
)
{
	u8 pre_word_enb;
	u8 *pEeprom_mask_updated;
	u16 i, j;
	u32 tmp_eeprom_offset;

	pEeprom_mask_updated = pEfuse_info->pEfuse_mask;

	for (i = 0; i < pEfuse_info->efuse_map_size; i = i + 8) {
		tmp_eeprom_offset = i;

		if (((tmp_eeprom_offset >> 3) & 1) > 0)
			pre_word_enb = (*(pEeprom_mask_updated + (i >> 4)) & 0x0F);
		else
			pre_word_enb = (*(pEeprom_mask_updated + (i >> 4)) >> 4);

		for (j = 0; j < 4; j++) {
			if (((pre_word_enb >> j) & 0x1) == 0) {
				*(pEfuse_info->pEfuse_map + tmp_eeprom_offset + (j << 1)) = 0xFF;
				*(pEfuse_info->pEfuse_map + tmp_eeprom_offset + (j << 1) + 1) = 0xFF;
			}
		}
	}
}

HALMAC_RET_STATUS
halmac_parse_efuse_data_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 segment_id = 0, segment_size = 0, h2c_seq = 0;
	u8 *pEeprom_map = NULL;
	u32 eeprom_size = pHalmac_adapter->hw_config_info.eeprom_size;
	u8 h2c_return_code = 0;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status;

	h2c_seq = (u8)EFUSE_DATA_GET_H2C_SEQ(pC2h_buf);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Seq num : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.efuse_state_set.seq_num, h2c_seq);
	if (h2c_seq != pHalmac_adapter->halmac_state.efuse_state_set.seq_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Seq num mismactch : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.efuse_state_set.seq_num, h2c_seq);
		return HALMAC_RET_SUCCESS;
	}

	if (pHalmac_adapter->halmac_state.efuse_state_set.process_status != HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not in HALMAC_CMD_PROCESS_SENDING\n");
		return HALMAC_RET_SUCCESS;
	}

	segment_id = (u8)EFUSE_DATA_GET_SEGMENT_ID(pC2h_buf);
	segment_size = (u8)EFUSE_DATA_GET_SEGMENT_SIZE(pC2h_buf);
	if (segment_id == 0)
		pHalmac_adapter->efuse_segment_size = segment_size;

	pEeprom_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, eeprom_size);
	if (pEeprom_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local eeprom map Fail!!\n");
		return HALMAC_RET_MALLOC_FAIL;
	}
	PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_map, 0xFF, eeprom_size);

	PLATFORM_MUTEX_LOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);
	PLATFORM_RTL_MEMCPY(pDriver_adapter, pHalmac_adapter->pHalEfuse_map + segment_id * pHalmac_adapter->efuse_segment_size,
		pC2h_buf + HALMAC_C2H_DATA_OFFSET_88XX, segment_size);
	PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);

	if (EFUSE_DATA_GET_END_SEGMENT(pC2h_buf) == _FALSE) {
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
		return HALMAC_RET_SUCCESS;
	}

	h2c_return_code = pHalmac_adapter->halmac_state.efuse_state_set.fw_return_code;

	if (HALMAC_H2C_RETURN_SUCCESS == (HALMAC_H2C_RETURN_CODE)h2c_return_code) {
		process_status = HALMAC_CMD_PROCESS_DONE;
		pHalmac_adapter->halmac_state.efuse_state_set.process_status = process_status;

		PLATFORM_MUTEX_LOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);
		pHalmac_adapter->hal_efuse_map_valid = _TRUE;
		PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);

		if (pHalmac_adapter->event_trigger.physical_efuse_map == 1) {
			PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE, process_status, pHalmac_adapter->pHalEfuse_map, pHalmac_adapter->hw_config_info.efuse_size);
			pHalmac_adapter->event_trigger.physical_efuse_map = 0;
		}

		if (pHalmac_adapter->event_trigger.logical_efuse_map == 1) {
			if (halmac_eeprom_parser_88xx(pHalmac_adapter, pHalmac_adapter->pHalEfuse_map, pEeprom_map) != HALMAC_RET_SUCCESS) {
				PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
				return HALMAC_RET_EEPROM_PARSING_FAIL;
			}
			PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_DUMP_LOGICAL_EFUSE, process_status, pEeprom_map, eeprom_size);
			pHalmac_adapter->event_trigger.logical_efuse_map = 0;
		}
	} else {
		process_status = HALMAC_CMD_PROCESS_ERROR;
		pHalmac_adapter->halmac_state.efuse_state_set.process_status = process_status;

		if (pHalmac_adapter->event_trigger.physical_efuse_map == 1) {
			PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE, process_status, &pHalmac_adapter->halmac_state.efuse_state_set.fw_return_code, 1);
			pHalmac_adapter->event_trigger.physical_efuse_map = 0;
		}

		if (pHalmac_adapter->event_trigger.logical_efuse_map == 1) {
			if (halmac_eeprom_parser_88xx(pHalmac_adapter, pHalmac_adapter->pHalEfuse_map, pEeprom_map) != HALMAC_RET_SUCCESS) {
				PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
				return HALMAC_RET_EEPROM_PARSING_FAIL;
			}
			PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_DUMP_LOGICAL_EFUSE, process_status, &pHalmac_adapter->halmac_state.efuse_state_set.fw_return_code, 1);
			pHalmac_adapter->event_trigger.logical_efuse_map = 0;
		}
	}

	PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_query_dump_physical_efuse_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	u8 *pEfuse_map = NULL;
	u32 efuse_size = pHalmac_adapter->hw_config_info.efuse_size;
	VOID *pDriver_adapter = NULL;
	PHALMAC_EFUSE_STATE_SET pEfuse_state_set = &pHalmac_adapter->halmac_state.efuse_state_set;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	*pProcess_status = pEfuse_state_set->process_status;

	if (data == NULL)
		return HALMAC_RET_NULL_POINTER;

	if (size == NULL)
		return HALMAC_RET_NULL_POINTER;

	if (*pProcess_status == HALMAC_CMD_PROCESS_DONE) {
		if (*size < efuse_size) {
			*size = efuse_size;
			return HALMAC_RET_BUFFER_TOO_SMALL;
		}

		*size = efuse_size;

		pEfuse_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, efuse_size);
		if (pEfuse_map == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local efuse map Fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
		}
		PLATFORM_RTL_MEMSET(pDriver_adapter, pEfuse_map, 0xFF, efuse_size);
		PLATFORM_MUTEX_LOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pEfuse_map, pHalmac_adapter->pHalEfuse_map, efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX);
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pEfuse_map + efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX + HALMAC_RESERVED_CS_EFUSE_SIZE_88XX,
			pHalmac_adapter->pHalEfuse_map + efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX + HALMAC_RESERVED_CS_EFUSE_SIZE_88XX,
			HALMAC_PROTECTED_EFUSE_SIZE_88XX - HALMAC_RESERVED_EFUSE_SIZE_88XX - HALMAC_RESERVED_CS_EFUSE_SIZE_88XX);
		PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &pHalmac_adapter->EfuseMutex);

		PLATFORM_RTL_MEMCPY(pDriver_adapter, data, pEfuse_map, *size);

		PLATFORM_RTL_FREE(pDriver_adapter, pEfuse_map, efuse_size);
	}

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_query_dump_logical_efuse_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	u8 *pEeprom_map = NULL;
	u32 eeprom_size = pHalmac_adapter->hw_config_info.eeprom_size;
	VOID *pDriver_adapter = NULL;
	PHALMAC_EFUSE_STATE_SET pEfuse_state_set = &pHalmac_adapter->halmac_state.efuse_state_set;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	*pProcess_status = pEfuse_state_set->process_status;

	if (data == NULL)
		return HALMAC_RET_NULL_POINTER;

	if (size == NULL)
		return HALMAC_RET_NULL_POINTER;

	if (*pProcess_status == HALMAC_CMD_PROCESS_DONE) {
		if (*size < eeprom_size) {
			*size = eeprom_size;
			return HALMAC_RET_BUFFER_TOO_SMALL;
		}

		*size = eeprom_size;

		pEeprom_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, eeprom_size);
		if (pEeprom_map == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "[ERR]halmac allocate local eeprom map Fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
		}
		PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_map, 0xFF, eeprom_size);

		if (halmac_eeprom_parser_88xx(pHalmac_adapter, pHalmac_adapter->pHalEfuse_map, pEeprom_map) != HALMAC_RET_SUCCESS) {
			PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
			return HALMAC_RET_EEPROM_PARSING_FAIL;
		}

		PLATFORM_RTL_MEMCPY(pDriver_adapter, data, pEeprom_map, *size);

		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
	}

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_parse_h2c_ack_phy_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 h2c_seq = 0;
	u8 h2c_return_code;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	h2c_seq = (u8)H2C_ACK_HDR_GET_H2C_SEQ(pC2h_buf);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Seq num : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.efuse_state_set.seq_num, h2c_seq);
	if (h2c_seq != pHalmac_adapter->halmac_state.efuse_state_set.seq_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Seq num mismactch : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.efuse_state_set.seq_num, h2c_seq);
		return HALMAC_RET_SUCCESS;
	}

	if (pHalmac_adapter->halmac_state.efuse_state_set.process_status != HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not in HALMAC_CMD_PROCESS_SENDING\n");
		return HALMAC_RET_SUCCESS;
	}

	h2c_return_code = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(pC2h_buf);
	pHalmac_adapter->halmac_state.efuse_state_set.fw_return_code = h2c_return_code;

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_88XX_SUPPORT */

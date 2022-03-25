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

#include "halmac_common_8822b.h"
#include "../halmac_common_88xx.h"

#if HALMAC_8822B_SUPPORT

/**
 * halmac_get_hw_value_8822b() -get hw config value
 * @pHalmac_adapter : the adapter of halmac
 * @hw_id : hw id for driver to query
 * @pvalue : hw value, reference table to get data type
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_hw_value_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_HW_ID hw_id,
	OUT VOID *pvalue
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_get_hw_value_8822b ==========>\n");

	if (pvalue == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_get_hw_value_8822b (NULL ==pvalue)\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (halmac_get_hw_value_88xx(pHalmac_adapter, hw_id, pvalue) != HALMAC_RET_SUCCESS) {
		/*switch (hw_id) {
		default:
			return HALMAC_RET_PARA_NOT_SUPPORT;
		}*/
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_get_hw_value_8822b <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_set_hw_value_8822b() -set hw config value
 * @pHalmac_adapter : the adapter of halmac
 * @hw_id : hw id for driver to config
 * @pvalue : hw value, reference table to get data type
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_set_hw_value_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_HW_ID hw_id,
	IN VOID *pvalue
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_set_hw_value_8822b ==========>\n");

	if (pvalue == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_set_hw_value_8822b (NULL == pvalue)\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (halmac_set_hw_value_88xx(pHalmac_adapter, hw_id, pvalue) != HALMAC_RET_SUCCESS) {
		switch (hw_id) {
		case HALMAC_HW_SDIO_TX_FORMAT:
			break;
		default:
			return HALMAC_RET_PARA_NOT_SUPPORT;
		}
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_set_hw_value_8822b <==========\n");

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_8822B_SUPPORT */

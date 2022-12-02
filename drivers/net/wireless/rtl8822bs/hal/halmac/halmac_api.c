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

#include "halmac_type.h"
#include "halmac_api.h"

#if (HALMAC_PLATFORM_WINDOWS)

#if HALMAC_8822B_SUPPORT
#include "halmac_88xx/halmac_init_win8822b.h"
#endif

#if HALMAC_8821C_SUPPORT
#include "halmac_88xx/halmac_init_win8821c.h"
#endif

#if HALMAC_8814B_SUPPORT
#include "halmac_88xx_v1/halmac_init_win8814b_v1.h"
#endif

#if HALMAC_8822C_SUPPORT
#include "halmac_88xx/halmac_init_win8822c.h"
#endif

#else

#if HALMAC_88XX_SUPPORT
#include "halmac_88xx/halmac_init_88xx.h"
#endif
#if HALMAC_88XX_V1_SUPPORT
#include "halmac_88xx_v1/halmac_init_88xx_v1.h"
#endif

#endif

static HALMAC_RET_STATUS
halmac_check_platform_api(
	IN VOID *pDriver_adapter,
	IN HALMAC_INTERFACE halmac_interface,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api
);

static HALMAC_RET_STATUS
halmac_get_chip_info(
	IN VOID	*pDriver_adapter,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api,
	IN HALMAC_INTERFACE	halmac_interface,
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static u8
platform_reg_read_8_sdio(
	IN VOID	*pDriver_adapter,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api,
	IN u32 offset
);

static HALMAC_RET_STATUS
plarform_reg_write_8_sdio(
	IN VOID	*pDriver_adapter,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api,
	IN u32 offset,
	IN u8 data
);

static u8
platform_reg_read_indirect_cmd52_sdio(
	IN VOID	*pDriver_adapter,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api,
	IN u32 offset
);

static HALMAC_RET_STATUS
halmac_convert_to_sdio_bus_offset(
	INOUT u32 *halmac_offset
);

/**
 * halmac_init_adapter() - init halmac_adapter
 * @pDriver_adapter : the adapter of caller
 * @pHalmac_platform_api : the platform APIs which is used in halmac APIs
 * @halmac_interface : bus interface
 * @ppHalmac_adapter : the adapter of halmac
 * @ppHalmac_api : the function pointer of APIs, caller shall call APIs by function pointer
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_adapter(
	IN VOID	*pDriver_adapter,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api,
	IN HALMAC_INTERFACE	halmac_interface,
	OUT	PHALMAC_ADAPTER *ppHalmac_adapter,
	OUT	PHALMAC_API *ppHalmac_api
)
{
	PHALMAC_ADAPTER pHalmac_adapter = (PHALMAC_ADAPTER)NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	u8 *pBuf = NULL;

	union {
		u32	i;
		u8	x[4];
	} ENDIAN_CHECK = { 0x01000000 };

	status = halmac_check_platform_api(pDriver_adapter, halmac_interface, pHalmac_platform_api);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, HALMAC_SVN_VER "\n");
	pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_MAJOR_VER = %x\n", HALMAC_MAJOR_VER);
	pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_PROTOTYPE_VER = %x\n", HALMAC_PROTOTYPE_VER);
	pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_MINOR_VER = %x\n", HALMAC_MINOR_VER);
	pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_PATCH_VER = %x\n", HALMAC_PATCH_VER);

	pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_adapter_88xx ==========>\n");

	if (ENDIAN_CHECK.x[0] == HALMAC_SYSTEM_ENDIAN) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Endian setting Err!!\n");
		return HALMAC_RET_ENDIAN_ERR;
	}

	pBuf = (u8 *)pHalmac_platform_api->RTL_MALLOC(pDriver_adapter, sizeof(HALMAC_ADAPTER));

	if (pBuf == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Malloc HAL Adapter Err!!\n");
		return HALMAC_RET_MALLOC_FAIL;
	}
	pHalmac_platform_api->RTL_MEMSET(pDriver_adapter, pBuf, 0x00, sizeof(HALMAC_ADAPTER));
	pHalmac_adapter = (PHALMAC_ADAPTER)pBuf;

	/* return halmac adapter address to caller */
	*ppHalmac_adapter = pHalmac_adapter;

	/* Record caller info */
	pHalmac_adapter->pHalmac_platform_api = pHalmac_platform_api;
	pHalmac_adapter->pDriver_adapter = pDriver_adapter;
	halmac_interface = (halmac_interface == HALMAC_INTERFACE_AXI) ? HALMAC_INTERFACE_PCIE : halmac_interface;
	pHalmac_adapter->halmac_interface = halmac_interface;

	if (halmac_get_chip_info(pDriver_adapter, pHalmac_platform_api, halmac_interface, pHalmac_adapter) != HALMAC_RET_SUCCESS) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]HALMAC_RET_CHIP_NOT_SUPPORT\n");
		return HALMAC_RET_CHIP_NOT_SUPPORT;
	}

	PLATFORM_MUTEX_INIT(pDriver_adapter, &pHalmac_adapter->EfuseMutex);
	PLATFORM_MUTEX_INIT(pDriver_adapter, &pHalmac_adapter->h2c_seq_mutex);
	PLATFORM_MUTEX_INIT(pDriver_adapter, &(pHalmac_adapter->sdio_indirect_mutex));

#if HALMAC_PLATFORM_WINDOWS == 0

#if HALMAC_88XX_SUPPORT
	if (HALMAC_CHIP_ID_8822B == pHalmac_adapter->chip_id || HALMAC_CHIP_ID_8821C == pHalmac_adapter->chip_id || HALMAC_CHIP_ID_8822C == pHalmac_adapter->chip_id) {
		halmac_init_adapter_para_88xx(pHalmac_adapter);
		status = halmac_mount_api_88xx(pHalmac_adapter);
	}
#endif

#if HALMAC_88XX_V1_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8814B) {
		halmac_init_adapter_para_88xx_v1(pHalmac_adapter);
		status = halmac_mount_api_88xx_v1(pHalmac_adapter);
	}
#endif

#else

#if HALMAC_8822B_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822B) {
		halmac_init_adapter_para_win8822b(pHalmac_adapter);
		status = halmac_mount_api_win8822b(pHalmac_adapter);
	}
#endif

#if HALMAC_8821C_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8821C) {
		halmac_init_adapter_para_win8821c(pHalmac_adapter);
		status = halmac_mount_api_win8821c(pHalmac_adapter);
	}
#endif

#if HALMAC_8814B_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8814B) {
		halmac_init_adapter_para_win8814b_v1(pHalmac_adapter);
		status = halmac_mount_api_win8814b_v1(pHalmac_adapter);
	}
#endif

#if HALMAC_8822C_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822C) {
		halmac_init_adapter_para_win8822c(pHalmac_adapter);
		status = halmac_mount_api_win8822c(pHalmac_adapter);
	}
#endif

#endif

	/* Return halmac API function pointer */
	*ppHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MUTEX_DEINIT(pDriver_adapter, &(pHalmac_adapter->sdio_indirect_mutex));
		PLATFORM_MUTEX_DEINIT(pDriver_adapter, &(pHalmac_adapter->EfuseMutex));
		PLATFORM_MUTEX_DEINIT(pDriver_adapter, &(pHalmac_adapter->h2c_seq_mutex));
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_adapter_88xx <==========\n");

	return status;
}

/**
 * halmac_halt_api() - stop halmac_api action
 * @pHalmac_adapter : the adapter of halmac
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_halt_api(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_halt_api ==========>\n");

	pHalmac_adapter->halmac_state.api_state = HALMAC_API_STATE_HALT;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_halt_api ==========>\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_deinit_adapter() - deinit halmac adapter
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_deinit_adapter(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_deinit_adapter_88xx ==========>\n");

	PLATFORM_MUTEX_DEINIT(pDriver_adapter, &pHalmac_adapter->EfuseMutex);
	PLATFORM_MUTEX_DEINIT(pDriver_adapter, &pHalmac_adapter->h2c_seq_mutex);
	PLATFORM_MUTEX_DEINIT(pDriver_adapter, &pHalmac_adapter->sdio_indirect_mutex);

	if (pHalmac_adapter->pHalEfuse_map != NULL) {
		PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter->pHalEfuse_map, pHalmac_adapter->hw_config_info.efuse_size);
		pHalmac_adapter->pHalEfuse_map = (u8 *)NULL;
	}

	if (NULL != pHalmac_adapter->sdio_free_space.pMacid_map) {
		PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter->sdio_free_space.pMacid_map, pHalmac_adapter->sdio_free_space.macid_map_size);
		pHalmac_adapter->sdio_free_space.pMacid_map = (u8 *)NULL;
	}

	if (pHalmac_adapter->halmac_state.psd_set.pData != NULL) {
		PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter->halmac_state.psd_set.pData, pHalmac_adapter->halmac_state.psd_set.data_size);
		pHalmac_adapter->halmac_state.psd_set.pData = (u8 *)NULL;
	}

	if (pHalmac_adapter->pHalmac_api != NULL) {
		PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter->pHalmac_api, sizeof(HALMAC_API));
		pHalmac_adapter->pHalmac_api = NULL;
	}

	pHalmac_adapter->pHalAdapter_backup = NULL;
	PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter, sizeof(HALMAC_ADAPTER));

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_check_platform_api(
	IN VOID *pDriver_adapter,
	IN HALMAC_INTERFACE	halmac_interface,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api
)
{
	if (pHalmac_platform_api == NULL)
		return HALMAC_RET_PLATFORM_API_NULL;

	if (pHalmac_platform_api->MSG_PRINT == NULL)
		return HALMAC_RET_PLATFORM_API_NULL;

	if (halmac_interface == HALMAC_INTERFACE_SDIO) {
		if (pHalmac_platform_api->SDIO_CMD52_READ == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD52_READ)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->SDIO_CMD53_READ_8 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD53_READ_8)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->SDIO_CMD53_READ_16 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD53_READ_16)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->SDIO_CMD53_READ_32 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD53_READ_32)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->SDIO_CMD53_READ_N == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD53_READ_N)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->SDIO_CMD52_WRITE == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD52_WRITE)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->SDIO_CMD53_WRITE_8 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD53_WRITE_8)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->SDIO_CMD53_WRITE_16 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD53_WRITE_16)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->SDIO_CMD53_WRITE_32 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD53_WRITE_32)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->SDIO_CMD52_CIA_READ == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->SDIO_CMD52_CIA_READ)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
	}

	if ((halmac_interface == HALMAC_INTERFACE_USB) || (halmac_interface == HALMAC_INTERFACE_PCIE)) {
		if (pHalmac_platform_api->REG_READ_8 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->REG_READ_8)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->REG_READ_16 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->REG_READ_16)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->REG_READ_32 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->REG_READ_32)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->REG_WRITE_8 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->REG_WRITE_8)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->REG_WRITE_16 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->REG_WRITE_16)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
		if (pHalmac_platform_api->REG_WRITE_32 == NULL) {
			pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->REG_WRITE_32)\n");
			return HALMAC_RET_PLATFORM_API_NULL;
		}
	}

	if (pHalmac_platform_api->RTL_FREE == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->RTL_FREE)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}

	if (pHalmac_platform_api->RTL_MALLOC == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->RTL_MALLOC)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}
	if (pHalmac_platform_api->RTL_MEMCPY == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->RTL_MEMCPY)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}
	if (pHalmac_platform_api->RTL_MEMSET == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->RTL_MEMSET)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}
	if (pHalmac_platform_api->RTL_DELAY_US == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->RTL_DELAY_US)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}

	if (pHalmac_platform_api->MUTEX_INIT == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->MUTEX_INIT)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}
	if (pHalmac_platform_api->MUTEX_DEINIT == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->MUTEX_DEINIT)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}
	if (pHalmac_platform_api->MUTEX_LOCK == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->MUTEX_LOCK)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}
	if (pHalmac_platform_api->MUTEX_UNLOCK == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->MUTEX_UNLOCK)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}
	if (pHalmac_platform_api->EVENT_INDICATION == NULL) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "(NULL==pHalmac_platform_api->EVENT_INDICATION)\n");
		return HALMAC_RET_PLATFORM_API_NULL;
	}

	pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_check_platform_api ==========>\n");

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
halmac_get_version(
	OUT	HALMAC_VER *version
)
{
	version->major_ver = (u8)HALMAC_MAJOR_VER;
	version->prototype_ver = (u8)HALMAC_PROTOTYPE_VER;
	version->minor_ver = (u8)HALMAC_MINOR_VER;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_get_chip_info(
	IN VOID	*pDriver_adapter,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api,
	IN HALMAC_INTERFACE	halmac_interface,
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 chip_id, chip_version;
	u32 polling_count;

	/* Get Chip_id and Chip_version */
	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		plarform_reg_write_8_sdio(pDriver_adapter, pHalmac_platform_api, REG_SDIO_HSUS_CTRL, platform_reg_read_8_sdio(pDriver_adapter, pHalmac_platform_api, REG_SDIO_HSUS_CTRL) & ~(BIT(0)));

		polling_count = 10000;
		while (!(platform_reg_read_8_sdio(pDriver_adapter, pHalmac_platform_api, REG_SDIO_HSUS_CTRL) & 0x02)) {
			polling_count--;
			if (polling_count == 0)
				return HALMAC_RET_SDIO_LEAVE_SUSPEND_FAIL;
		}

		chip_id = platform_reg_read_indirect_cmd52_sdio(pDriver_adapter, pHalmac_platform_api, REG_SYS_CFG2);
		chip_version =  platform_reg_read_indirect_cmd52_sdio(pDriver_adapter, pHalmac_platform_api, REG_SYS_CFG1 + 1) >> 4;
	} else {
		chip_id = pHalmac_platform_api->REG_READ_8(pDriver_adapter, REG_SYS_CFG2);
		chip_version = pHalmac_platform_api->REG_READ_8(pDriver_adapter, REG_SYS_CFG1 + 1) >> 4;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Chip id : 0x%X\n", chip_id);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Chip version : 0x%X\n", chip_version);

	pHalmac_adapter->chip_version = (HALMAC_CHIP_VER)chip_version;

	if (chip_id == HALMAC_CHIP_ID_HW_DEF_8822B) {
		pHalmac_adapter->chip_id = HALMAC_CHIP_ID_8822B;
	} else if (chip_id == HALMAC_CHIP_ID_HW_DEF_8821C) {
		pHalmac_adapter->chip_id = HALMAC_CHIP_ID_8821C;
	} else if (chip_id == HALMAC_CHIP_ID_HW_DEF_8814B) {
		pHalmac_adapter->chip_id = HALMAC_CHIP_ID_8814B;
	} else if (chip_id == HALMAC_CHIP_ID_HW_DEF_8197F) {
		pHalmac_adapter->chip_id = HALMAC_CHIP_ID_8197F;
	} else if (chip_id == HALMAC_CHIP_ID_HW_DEF_8822C) {
		pHalmac_adapter->chip_id = HALMAC_CHIP_ID_8822C;
	} else {
		pHalmac_adapter->chip_id = HALMAC_CHIP_ID_UNDEFINE;
		return HALMAC_RET_CHIP_NOT_SUPPORT;
	}

	return HALMAC_RET_SUCCESS;
}

static u8
platform_reg_read_8_sdio(
	IN VOID	*pDriver_adapter,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api,
	IN u32 offset
)
{
	u8 value8;
	u32 halmac_offset = offset;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (0 == (halmac_offset & 0xFFFF0000))
		halmac_offset |= WLAN_IOREG_OFFSET;

	status = halmac_convert_to_sdio_bus_offset(&halmac_offset);
	if (status != HALMAC_RET_SUCCESS) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]platform_reg_read_8_sdio error = %x\n", status);
		return status;
	}

	value8 = pHalmac_platform_api->SDIO_CMD52_READ(pDriver_adapter, halmac_offset);

	return value8;
}

static HALMAC_RET_STATUS
plarform_reg_write_8_sdio(
	IN VOID	*pDriver_adapter,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api,
	IN u32 offset,
	IN u8 data
)
{
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	u32 halmac_offset = offset;

	if (0 == (halmac_offset & 0xFFFF0000))
		halmac_offset |= WLAN_IOREG_OFFSET;

	status = halmac_convert_to_sdio_bus_offset(&halmac_offset);

	if (status != HALMAC_RET_SUCCESS) {
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_reg_write_8_sdio_88xx error = %x\n", status);
		return status;
	}
	pHalmac_platform_api->SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset, data);

	return HALMAC_RET_SUCCESS;
}

static u8
platform_reg_read_indirect_cmd52_sdio(
	IN VOID	*pDriver_adapter,
	IN PHALMAC_PLATFORM_API pHalmac_platform_api,
	IN u32 offset
)
{
	u8 value8;
	u8 rtemp = 0xFF;
	u32 counter = 50;

	pHalmac_platform_api->SDIO_CMD52_WRITE(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | (REG_SDIO_INDIRECT_REG_CFG & HALMAC_SDIO_LOCAL_MSK), (u8)offset);
	pHalmac_platform_api->SDIO_CMD52_WRITE(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_CFG + 1) & HALMAC_SDIO_LOCAL_MSK), (u8)(offset >> 8));
	pHalmac_platform_api->SDIO_CMD52_WRITE(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_CFG + 2) & HALMAC_SDIO_LOCAL_MSK), (u8)BIT(3));

	do {
		rtemp = pHalmac_platform_api->SDIO_CMD52_READ(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_CFG + 2) & HALMAC_SDIO_LOCAL_MSK));
		counter--;
	} while (((rtemp & BIT(4)) == 0) && (counter > 0));

	if (((rtemp & BIT(4)) == 0) && (counter == 0))
		pHalmac_platform_api->MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "platform_reg_read_indirect_cmd52_sdio fail, offset = 0x%x\n", offset);

	value8 = pHalmac_platform_api->SDIO_CMD52_READ(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | (REG_SDIO_INDIRECT_REG_DATA & HALMAC_SDIO_LOCAL_MSK));

	return value8;
}

/*Note: copy from halmac_convert_to_sdio_bus_offset_88xx*/
static HALMAC_RET_STATUS
halmac_convert_to_sdio_bus_offset(
	INOUT u32 *halmac_offset
)
{
	switch ((*halmac_offset) & 0xFFFF0000) {
	case WLAN_IOREG_OFFSET:
		*halmac_offset = (HALMAC_SDIO_CMD_ADDR_MAC_REG << 13) | (*halmac_offset & HALMAC_WLAN_MAC_REG_MSK);
		break;
	case SDIO_LOCAL_OFFSET:
		*halmac_offset = (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | (*halmac_offset & HALMAC_SDIO_LOCAL_MSK);
		break;
	default:
		*halmac_offset = 0xFFFFFFFF;
		return HALMAC_RET_CONVERT_SDIO_OFFSET_FAIL;
	}

	return HALMAC_RET_SUCCESS;
}


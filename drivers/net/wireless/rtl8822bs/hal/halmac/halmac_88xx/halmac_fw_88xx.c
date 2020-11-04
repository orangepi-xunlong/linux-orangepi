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

#include "halmac_fw_88xx.h"
#include "halmac_88xx_cfg.h"
#include "halmac_common_88xx.h"
#include "halmac_init_88xx.h"

#if HALMAC_88XX_SUPPORT

#define DLFW_RESTORE_REG_NUM			8
#define ID_INFORM_DLEMEM_RDY			0x80
#define ILLEGAL_KEY_GROUP				0xFAAAAA00

#define FW_STATUS_CHK_FATAL		(BIT(1) | BIT(20))
#define FW_STATUS_CHK_ERR		(BIT(4) | BIT(5) | BIT(6) | BIT(7) | BIT(8) | BIT(9) | \
								BIT(12) | BIT(14) | BIT(15) | BIT(16) | BIT(17) | BIT(18) | BIT(19) | \
								BIT(21) |BIT(22) |BIT(25))
#define FW_STATUS_CHK_WARN		~(FW_STATUS_CHK_FATAL | FW_STATUS_CHK_ERR)

static HALMAC_RET_STATUS
halmac_update_fw_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHamacl_fw,
	IN u32 halmac_fw_size
);

static HALMAC_RET_STATUS
halmac_dlfw_to_mem_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pRam_code,
	IN u32 src,
	IN u32 dest,
	IN u32 code_size
);

static VOID
halmac_restore_mac_register_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_RESTORE_INFO pRestore_info,
	IN u32 restore_num
);

static HALMAC_RET_STATUS
halmac_dlfw_end_flow_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_free_dl_fw_end_flow_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_send_fwpkt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 pg_addr,
	IN u8 *pRam_code,
	IN u32 code_size
);

static HALMAC_RET_STATUS
halmac_iddma_dlfw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 source,
	IN u32 dest,
	IN u32 length,
	IN u8 first
);

static HALMAC_RET_STATUS
halmac_iddma_en_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 source,
	IN u32 dest,
	IN u32 ctrl
);

static HALMAC_RET_STATUS
halmac_check_fw_chksum_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 memory_address
);

/**
 * halmac_download_firmware_88xx() - download Firmware
 * @pHalmac_adapter : the adapter of halmac
 * @pHamacl_fw : firmware bin
 * @halmac_fw_size : firmware size
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_download_firmware_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHamacl_fw,
	IN u32 halmac_fw_size
)
{
	u8 value8;
	u8 *pFile_ptr;
	u16 value16;
	u32 restore_index = 0;
	u32 lte_coex_backup = 0;
	u16 halmac_h2c_ver = 0, fw_h2c_ver = 0;
	u32 iram_pkt_size, dmem_pkt_size, eram_pkt_size = 0;
	HALMAC_RET_STATUS status;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RESTORE_INFO restore_info[DLFW_RESTORE_REG_NUM];

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_download_firmware_88xx ==========>\n");

	if (halmac_fw_size > HALMAC_FW_SIZE_MAX_88XX || halmac_fw_size < HALMAC_FWHDR_SIZE_88XX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]FW size error!\n");
		return HALMAC_RET_FW_SIZE_ERR;
	}

	fw_h2c_ver = rtk_le16_to_cpu(*((u16 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_H2C_FORMAT_VER_88XX)));
	halmac_h2c_ver = H2C_FORMAT_VERSION;
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac h2c/c2h format = %x, fw h2c/c2h format = %x!!\n", halmac_h2c_ver, fw_h2c_ver);
	if (fw_h2c_ver != halmac_h2c_ver)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]H2C/C2H version between HALMAC and FW is compatible!!\n");

	pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_NONE;

	if (halmac_ltecoex_reg_read_88xx(pHalmac_adapter, 0x38, &lte_coex_backup) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_LTECOEX_READY_FAIL;

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1);
	value8 = (u8)(value8 & ~(BIT(2)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1, value8); /* Disable CPU reset */

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_RSV_CTRL + 1);
	value8 = (u8)(value8 & ~(BIT(0)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RSV_CTRL + 1, value8);

	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_TXDMA_PQ_MAP + 1;
	restore_info[restore_index].value = HALMAC_REG_READ_8(pHalmac_adapter, REG_TXDMA_PQ_MAP + 1);
	restore_index++;
	value8 = HALMAC_DMA_MAPPING_HIGH << 6;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_PQ_MAP + 1, value8);  /* set HIQ to hi priority */

	/* DLFW only use HIQ, map HIQ to hi priority */
	pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI] = HALMAC_DMA_MAPPING_HIGH;
	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_CR;
	restore_info[restore_index].value = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR);
	restore_index++;
	restore_info[restore_index].length = 4;
	restore_info[restore_index].mac_register = REG_H2CQ_CSR;
	restore_info[restore_index].value = BIT(31);
	restore_index++;
	value8 = BIT_HCI_TXDMA_EN | BIT_TXDMA_EN;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2CQ_CSR, BIT(31));

	/* Config hi priority queue and public priority queue page number (only for DLFW) */
	restore_info[restore_index].length = 2;
	restore_info[restore_index].mac_register = REG_FIFOPAGE_INFO_1;
	restore_info[restore_index].value = HALMAC_REG_READ_16(pHalmac_adapter, REG_FIFOPAGE_INFO_1);
	restore_index++;
	restore_info[restore_index].length = 4;
	restore_info[restore_index].mac_register = REG_RQPN_CTRL_2;
	restore_info[restore_index].value = HALMAC_REG_READ_32(pHalmac_adapter, REG_RQPN_CTRL_2) | BIT(31);
	restore_index++;
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_1, 0x200);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RQPN_CTRL_2, restore_info[restore_index - 1].value);

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO)
		HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_FREE_TXPG);

	halmac_update_fw_info_88xx(pHalmac_adapter, pHamacl_fw, halmac_fw_size);

	dmem_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_DMEM_SIZE_88XX));
	iram_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_IRAM_SIZE_88XX));
	if (0 != ((*(pHamacl_fw + HALMAC_FWHDR_OFFSET_MEM_USAGE_88XX)) & BIT(4)))
		eram_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_ERAM_SIZE_88XX));

	dmem_pkt_size = rtk_le32_to_cpu(dmem_pkt_size);
	iram_pkt_size = rtk_le32_to_cpu(iram_pkt_size);
	eram_pkt_size = rtk_le32_to_cpu(eram_pkt_size);

	dmem_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;
	iram_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;
	if (eram_pkt_size != 0)
		eram_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;

	if (halmac_fw_size != (HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size + iram_pkt_size + eram_pkt_size)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]FW size mismatch the real fw size!\n");
		status = HALMAC_RET_FW_SIZE_ERR;
		goto DLFW_FAIL;
	}

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR + 1);
	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_CR + 1;
	restore_info[restore_index].value = value8;
	restore_index++;
	value8 = (u8)(value8 | BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, value8); /* Enable SW TX beacon */

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL);
	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_BCN_CTRL;
	restore_info[restore_index].value = value8;
	restore_index++;
	value8 = (u8)((value8 & (~BIT(3))) | BIT(4));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, value8); /* Disable beacon related functions */

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2);
	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_FWHW_TXQ_CTRL + 2;
	restore_info[restore_index].value = value8;
	restore_index++;
	value8 = (u8)(value8 & ~(BIT(6)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, value8); /* Disable ptcl tx bcnq */

	value16 = (u16)(HALMAC_REG_READ_16(pHalmac_adapter, REG_MCUFW_CTRL) & 0x3800);
	value16 |= BIT(0);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MCUFW_CTRL, value16); /* MCU/FW setting */

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CPU_DMEM_CON + 2);
	value8 &= ~(BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CPU_DMEM_CON + 2, value8);
	value8 |= BIT(0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CPU_DMEM_CON + 2, value8);

	pFile_ptr = pHamacl_fw + HALMAC_FWHDR_SIZE_88XX;
	status = halmac_dlfw_to_mem_88xx(pHalmac_adapter, pFile_ptr, 0,
		    rtk_le32_to_cpu(*((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_DMEM_ADDR_88XX))) & ~(BIT(31)), dmem_pkt_size);
	if (status != HALMAC_RET_SUCCESS)
		goto DLFW_END;

	pFile_ptr = pHamacl_fw + HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size;
	status = halmac_dlfw_to_mem_88xx(pHalmac_adapter, pFile_ptr, 0,
		    rtk_le32_to_cpu(*((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_IRAM_ADDR_88XX))) & ~(BIT(31)), iram_pkt_size);
	if (status != HALMAC_RET_SUCCESS)
		goto DLFW_END;

	if (eram_pkt_size != 0) {
		pFile_ptr = pHamacl_fw + HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size + iram_pkt_size;
		status = halmac_dlfw_to_mem_88xx(pHalmac_adapter, pFile_ptr, 0,
			    rtk_le32_to_cpu(*((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_EMEM_ADDR_88XX))) & ~(BIT(31)), eram_pkt_size);
		if (status != HALMAC_RET_SUCCESS)
			goto DLFW_END;
	}

	halmac_init_offload_feature_state_machine_88xx(pHalmac_adapter);
DLFW_END:

	halmac_restore_mac_register_88xx(pHalmac_adapter, restore_info, DLFW_RESTORE_REG_NUM);

	if (status == HALMAC_RET_SUCCESS) {
		status = halmac_dlfw_end_flow_88xx(pHalmac_adapter);
		if (status != HALMAC_RET_SUCCESS)
			goto DLFW_FAIL;
	} else {
		goto DLFW_FAIL;
	}

	if (halmac_ltecoex_reg_write_88xx(pHalmac_adapter, 0x38, lte_coex_backup) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_LTECOEX_READY_FAIL;

	pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_DONE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_download_firmware_88xx <==========\n");

	return HALMAC_RET_SUCCESS;

DLFW_FAIL:

	/* Disable FWDL_EN */
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUFW_CTRL, (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUFW_CTRL) & ~(BIT(0))));

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1);
	value8 = (u8)(value8 | BIT(2));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1, value8);

	if (halmac_ltecoex_reg_write_88xx(pHalmac_adapter, 0x38, lte_coex_backup) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_LTECOEX_READY_FAIL;

	return status;
}

/**
 * halmac_free_download_firmware_88xx() - download specific memory firmware
 * @pHalmac_adapter
 * @dlfw_mem : memory selection
 * @pHamacl_fw : firmware bin
 * @halmac_fw_size : firmware size
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_free_download_firmware_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DLFW_MEM dlfw_mem,
	IN u8 *pHamacl_fw,
	IN u32 halmac_fw_size
)
{
	u8 tx_pause_backup;
	u8 *pFile_ptr;
	u16 dl_addr;
	u32 max_dlfw_sz_backup;
	u32 iram_pkt_size, dmem_pkt_size, eram_pkt_size = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_DLFW_FAIL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_free_download_firmware_88xx ==========>\n");

	if (halmac_fw_size > HALMAC_FW_SIZE_MAX_88XX || halmac_fw_size < HALMAC_FWHDR_SIZE_88XX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]FW size error!\n");
		return HALMAC_RET_FW_SIZE_ERR;
	}

	dmem_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_DMEM_SIZE_88XX));
	iram_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_IRAM_SIZE_88XX));
	if (0 != ((*(pHamacl_fw + HALMAC_FWHDR_OFFSET_MEM_USAGE_88XX)) & BIT(4)))
		eram_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_ERAM_SIZE_88XX));

	dmem_pkt_size = rtk_le32_to_cpu(dmem_pkt_size);
	iram_pkt_size = rtk_le32_to_cpu(iram_pkt_size);
	eram_pkt_size = rtk_le32_to_cpu(eram_pkt_size);

	dmem_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;
	iram_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;
	if (eram_pkt_size != 0)
		eram_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;
	else
		return HALMAC_RET_SUCCESS;

	if (halmac_fw_size != (HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size + iram_pkt_size + eram_pkt_size)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]FW size mismatch the real fw size!\n");
		return HALMAC_RET_DLFW_FAIL;
	}

	max_dlfw_sz_backup = pHalmac_adapter->max_download_size;
	if (dlfw_mem == HALMAC_DLFW_MEM_EMEM) {
		dl_addr = 0;
	} else {
		dl_addr = pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy;
		pHalmac_adapter->max_download_size = (max_dlfw_sz_backup > HALMAC_DLFW_WITH_RSVDPG_SZ_88XX) ?
											HALMAC_DLFW_WITH_RSVDPG_SZ_88XX : max_dlfw_sz_backup;
	}

	tx_pause_backup = HALMAC_REG_READ_8(pHalmac_adapter, REG_TXPAUSE);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXPAUSE, tx_pause_backup | BIT(7));

	if (eram_pkt_size != 0) {
		pFile_ptr = pHamacl_fw + HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size + iram_pkt_size;
		status = halmac_dlfw_to_mem_88xx(pHalmac_adapter, pFile_ptr, dl_addr << 7,
					rtk_le32_to_cpu(*((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_EMEM_ADDR_88XX))) & ~(BIT(31)), eram_pkt_size);
		if (status != HALMAC_RET_SUCCESS)
			goto DL_FREE_FW_END;
	}

	status = halmac_free_dl_fw_end_flow_88xx(pHalmac_adapter);

DL_FREE_FW_END:
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXPAUSE, tx_pause_backup);
	pHalmac_adapter->max_download_size = max_dlfw_sz_backup;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_free_download_firmware_88xx <==========\n");

	return status;
}

/**
 * halmac_get_fw_version_88xx() - get FW version
 * @pHalmac_adapter : the adapter of halmac
 * @pFw_version : fw version info
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_fw_version_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	OUT PHALMAC_FW_VERSION	pFw_version
)
{
	PHALMAC_FW_VERSION pFw_info = &pHalmac_adapter->fw_version;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (pFw_version == NULL)
		return HALMAC_RET_NULL_POINTER;

	if (pHalmac_adapter->halmac_state.dlfw_state == HALMAC_DLFW_NONE)
		return HALMAC_RET_NO_DLFW;

	pFw_version->version = pFw_info->version;
	pFw_version->sub_version = pFw_info->sub_version;
	pFw_version->sub_index = pFw_info->sub_index;
	pFw_version->h2c_version = pFw_info->h2c_version;
	pFw_version->build_time.month = pFw_info->build_time.month;
	pFw_version->build_time.date = pFw_info->build_time.date;
	pFw_version->build_time.hour = pFw_info->build_time.hour;
	pFw_version->build_time.min = pFw_info->build_time.min;
	pFw_version->build_time.year = pFw_info->build_time.year;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_update_fw_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHamacl_fw,
	IN u32 halmac_fw_size
)
{
	PHALMAC_FW_VERSION pFw_info = &pHalmac_adapter->fw_version;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	pFw_info->version = rtk_le16_to_cpu(*((u16 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_VERSION_88XX)));
	pFw_info->sub_version = *(pHamacl_fw + HALMAC_FWHDR_OFFSET_SUBVERSION_88XX);
	pFw_info->sub_index = *(pHamacl_fw + HALMAC_FWHDR_OFFSET_SUBINDEX_88XX);
	pFw_info->h2c_version = rtk_le16_to_cpu(*((u16 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_H2C_FORMAT_VER_88XX)));
	pFw_info->build_time.month = *(pHamacl_fw + HALMAC_FWHDR_OFFSET_MONTH_88XX);
	pFw_info->build_time.date = *(pHamacl_fw + HALMAC_FWHDR_OFFSET_DATE_88XX);
	pFw_info->build_time.hour = *(pHamacl_fw + HALMAC_FWHDR_OFFSET_HOUR_88XX);
	pFw_info->build_time.min = *(pHamacl_fw + HALMAC_FWHDR_OFFSET_MIN_88XX);
	pFw_info->build_time.year = rtk_le16_to_cpu(*((u16 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_YEAR_88XX)));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "[TRACE]FW version : %X\n", pFw_info->version);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "[TRACE]FW sub version : %X\n", pFw_info->sub_version);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "[TRACE]FW sub index : %X\n", pFw_info->sub_index);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "[TRACE]FW build time : %d/%d/%d %d:%d\n",
																			pFw_info->build_time.year, pFw_info->build_time.month,
																			pFw_info->build_time.date, pFw_info->build_time.hour,
																			pFw_info->build_time.min);

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_dlfw_to_mem_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pRam_code,
	IN u32 src,
	IN u32 dest,
	IN u32 code_size
)
{
	u8 *pCode_ptr;
	u8 first_part;
	u32 mem_offset;
	u32 pkt_size_tmp, send_pkt_size;
	HALMAC_RET_STATUS status;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	pCode_ptr = pRam_code;
	mem_offset = 0;
	first_part = 1;
	pkt_size_tmp = code_size;

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_DDMA_CH0CTRL, HALMAC_REG_READ_32(pHalmac_adapter, REG_DDMA_CH0CTRL) | BIT_DDMACH0_RESET_CHKSUM_STS);

	while (pkt_size_tmp != 0) {
		if (pkt_size_tmp >= pHalmac_adapter->max_download_size)
			send_pkt_size = pHalmac_adapter->max_download_size;
		else
			send_pkt_size = pkt_size_tmp;

		status = halmac_send_fwpkt_88xx(pHalmac_adapter, (u16)(src >> 7), pCode_ptr + mem_offset, send_pkt_size);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_send_fwpkt_88xx fail!!");
			return status;
		}

		status = halmac_iddma_dlfw_88xx(pHalmac_adapter, HALMAC_OCPBASE_TXBUF_88XX + src + pHalmac_adapter->hw_config_info.txdesc_size,
			    dest + mem_offset, send_pkt_size, first_part);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_iddma_dlfw_88xx fail!!");
			return status;
		}

		first_part = 0;
		mem_offset += send_pkt_size;
		pkt_size_tmp -= send_pkt_size;
	}

	status = halmac_check_fw_chksum_88xx(pHalmac_adapter, dest);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pHalmac_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_check_fw_chksum_88xx fail!!");
		return status;
	}

	return HALMAC_RET_SUCCESS;
}

static VOID
halmac_restore_mac_register_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_RESTORE_INFO pRestore_info,
	IN u32 restore_num
)
{
	u8 value_length;
	u32 i;
	u32 mac_register;
	u32 mac_value;
	PHALMAC_API pHalmac_api;
	PHALMAC_RESTORE_INFO pCurr_restore_info = pRestore_info;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	for (i = 0; i < restore_num; i++) {
		mac_register = pCurr_restore_info->mac_register;
		mac_value = pCurr_restore_info->value;
		value_length = pCurr_restore_info->length;

		if (value_length == 1)
			HALMAC_REG_WRITE_8(pHalmac_adapter, mac_register, (u8)mac_value);
		else if (value_length == 2)
			HALMAC_REG_WRITE_16(pHalmac_adapter, mac_register, (u16)mac_value);
		else if (value_length == 4)
			HALMAC_REG_WRITE_32(pHalmac_adapter, mac_register, mac_value);

		pCurr_restore_info++;
	}
}

static HALMAC_RET_STATUS
halmac_dlfw_end_flow_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 value8;
	u32 counter;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	PHALMAC_API pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TXDMA_STATUS, BIT(2));

	/* Check IMEM & DMEM checksum is OK or not */
	if (0x50 == (HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUFW_CTRL) & 0x50))
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MCUFW_CTRL, (u16)(HALMAC_REG_READ_16(pHalmac_adapter, REG_MCUFW_CTRL) | BIT_FW_DW_RDY));
	else
		return HALMAC_RET_IDMEM_CHKSUM_FAIL;

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUFW_CTRL, (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUFW_CTRL) & ~(BIT(0))));

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_RSV_CTRL + 1);
	value8 = (u8)(value8 | BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RSV_CTRL + 1, value8);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1);
	value8 = (u8)(value8 | BIT(2));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1, value8); /* Release MCU reset */
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Download Finish, Reset CPU\n");

	counter = 10000;
	while (HALMAC_REG_READ_16(pHalmac_adapter, REG_MCUFW_CTRL) != 0xC078) {
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Check 0x80 = 0xC078 fail\n");
			if (ILLEGAL_KEY_GROUP == (HALMAC_REG_READ_32(pHalmac_adapter, REG_FW_DBG7) & 0xFFFFFF00)) {
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Key fail\n");
				return HALMAC_RET_ILLEGAL_KEY_FAIL;
			}
			return HALMAC_RET_FW_READY_CHK_FAIL;
		}
		counter--;
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 50);
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Check 0x80 = 0xC078 counter = %d\n", counter);

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_free_dl_fw_end_flow_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u32 counter;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	PHALMAC_API pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	counter = 100;
	while (HALMAC_REG_READ_8(pHalmac_adapter, REG_HMETFR + 3) != 0) {
		counter--;
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]0x1CF != 0\n");
			return HALMAC_RET_DLFW_FAIL;
		}
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 50);
	}

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_HMETFR + 3, ID_INFORM_DLEMEM_RDY);

	counter = 10000;
	while (HALMAC_REG_READ_8(pHalmac_adapter, REG_MCU_TST_CFG) != ID_INFORM_DLEMEM_RDY) {
		counter--;
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]0x1AF != 0x80\n");
			return HALMAC_RET_DLFW_FAIL;
		}
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 50);
	}

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCU_TST_CFG, 0);

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_send_fwpkt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 pg_addr,
	IN u8 *pRam_code,
	IN u32 code_size
)
{
	HALMAC_RET_STATUS status;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	status = halmac_download_rsvd_page_88xx(pHalmac_adapter, pg_addr, pRam_code, code_size);
	if (status != HALMAC_RET_SUCCESS)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR]PLATFORM_SEND_RSVD_PAGE 0 error!!\n");

	return status;
}

static HALMAC_RET_STATUS
halmac_iddma_dlfw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 source,
	IN u32 dest,
	IN u32 length,
	IN u8 first
)
{
	u8 value8;
	u8 retry_cnt = 3;
	u32 counter;
	u32 ch0_control = (u32)(BIT_DDMACH0_CHKSUM_EN | BIT_DDMACH0_OWN);
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	counter = HALMC_DDMA_POLLING_COUNT;
	while (HALMAC_REG_READ_32(pHalmac_adapter, REG_DDMA_CH0CTRL) & BIT_DDMACH0_OWN) {
		counter--;
		if (counter == 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR]halmac_iddma_dlfw_88xx error-1!!\n");
			return HALMAC_RET_DDMA_FAIL;
		}
	}

	ch0_control |= (length & BIT_MASK_DDMACH0_DLEN);
	if (first == 0)
		ch0_control |= BIT_DDMACH0_CHKSUM_CONT;

	while (halmac_iddma_en_88xx(pHalmac_adapter, source, dest, ch0_control) != HALMAC_RET_SUCCESS) {
		value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1) & BIT(2);
		if ((retry_cnt != 0) && (value8 == 0)) {
			value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CPU_DMEM_CON + 2);
			value8 &= ~(BIT(0));
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CPU_DMEM_CON + 2, value8);
			value8 |= BIT(0);
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CPU_DMEM_CON + 2, value8);
			retry_cnt--;
		} else {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR]halmac_iddma_dlfw_88xx error-2!!\n");
			return HALMAC_RET_DDMA_FAIL;
		}
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_iddma_en_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 source,
	IN u32 dest,
	IN u32 ctrl
)
{
	u32 counter = HALMC_DDMA_POLLING_COUNT;
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_DDMA_CH0SA, source);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_DDMA_CH0DA, dest);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_DDMA_CH0CTRL, ctrl);

	while (HALMAC_REG_READ_32(pHalmac_adapter, REG_DDMA_CH0CTRL) & BIT_DDMACH0_OWN) {
		counter--;
		if (counter == 0)
			return HALMAC_RET_DDMA_FAIL;
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_check_fw_chksum_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 memory_address
)
{
	u8 mcu_fw_ctrl;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	mcu_fw_ctrl = HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUFW_CTRL);

	if (HALMAC_REG_READ_32(pHalmac_adapter, REG_DDMA_CH0CTRL) & BIT_DDMACH0_CHKSUM_STS) {
		if (memory_address < HALMAC_OCPBASE_DMEM_88XX) {
			mcu_fw_ctrl |= BIT_IMEM_DW_OK;
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUFW_CTRL, (u8)(mcu_fw_ctrl & ~(BIT_IMEM_CHKSUM_OK)));
		} else {
			mcu_fw_ctrl |= BIT_DMEM_DW_OK;
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUFW_CTRL, (u8)(mcu_fw_ctrl & ~(BIT_DMEM_CHKSUM_OK)));
		}

		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR]halmac_check_fw_chksum_88xx error!!\n");

		return HALMAC_RET_FW_CHECKSUM_FAIL;
	}

	if (memory_address < HALMAC_OCPBASE_DMEM_88XX) {
		mcu_fw_ctrl |= BIT_IMEM_DW_OK;
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUFW_CTRL, (u8)(mcu_fw_ctrl | BIT_IMEM_CHKSUM_OK));
	} else {
		mcu_fw_ctrl |= BIT_DMEM_DW_OK;
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUFW_CTRL, (u8)(mcu_fw_ctrl | BIT_DMEM_CHKSUM_OK));
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_check_fw_status_88xx() -check fw status
 * @pHalmac_adapter : the adapter of halmac
 * @fw_status : fw status
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_check_fw_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u8 *fw_status
)
{
	u32 cnt;
	u32 value32, value32_backup;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "[TRACE]halmac_check_fw_status_88xx ==========>\n");

	*fw_status = _TRUE;

	value32 = PLATFORM_REG_READ_32(pDriver_adapter, REG_FW_DBG6);

	if (value32 != 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR]halmac_check_fw_status REG_FW_DBG6 !=0\n");
		if ((value32 & FW_STATUS_CHK_WARN) != 0)
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_WARN, "[WARN]fw status(warn) : %X\n", value32);

		if ((value32 & FW_STATUS_CHK_ERR) != 0)
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR]fw status(err) : %X\n", value32);

		if ((value32 & FW_STATUS_CHK_FATAL) != 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR]fw status(fatal) : %X\n", value32);
			*fw_status = _FALSE;
			return status;
		}
	}

	value32_backup = PLATFORM_REG_READ_32(pDriver_adapter, REG_FW_DBG7);
	cnt = 10;
	while (PLATFORM_REG_READ_32(pDriver_adapter, REG_FW_DBG7) == value32_backup) {
		cnt--;
		if (cnt == 0)
			break;
	}

	if (cnt == 0) {
		cnt = 200;
		while (PLATFORM_REG_READ_32(pDriver_adapter, REG_FW_DBG7) == value32_backup) {
			cnt--;
			if (cnt == 0) {
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR] Poll FW PC fail\n");
				*fw_status = _FALSE;
				return status;
			}
			PLATFORM_RTL_DELAY_US(pDriver_adapter, 50);
		}
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "[TRACE]halmac_check_fw_status_88xx <==========\n");

	return status;
}

HALMAC_RET_STATUS
halmac_dump_fw_dmem_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT u8 *dmem,
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

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_fw_dmem_88xx ==========>\n");


	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_dump_fw_dmem_88xx <==========\n");

	return status;
}

/**
 * halmac_cfg_max_dl_size_88xx() - config max download FW size
 * @pHalmac_adapter : the adapter of halmac
 * @size : max download fw size
 *
 * Halmac uses this setting to set max packet size for
 * download FW.
 * If user has not called this API, halmac use default
 * setting for download FW
 * Note1 : size need multiple of 2
 * Note2 : max size is 31K
 *
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_max_dl_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 size
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_max_dl_size_88xx ==========>\n");

	if (size > HALMAC_FW_CFG_MAX_DL_SIZE_MAX_88XX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR]size > HALMAC_FW_CFG_MAX_DL_SIZE_MAX!\n");
		return HALMAC_RET_CFG_DLFW_SIZE_FAIL;
	}

	if (0 != (size & (2 - 1))) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "[ERR]size is not multiple of 2!\n");
		return HALMAC_RET_CFG_DLFW_SIZE_FAIL;
	}

	pHalmac_adapter->max_download_size = size;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "[TRACE]Cfg max size is : %X\n", size);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_max_dl_size_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_88XX_SUPPORT */

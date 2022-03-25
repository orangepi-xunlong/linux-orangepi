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

#include "halmac_sdio_88xx.h"
#include "halmac_88xx_cfg.h"

#if HALMAC_88XX_SUPPORT

/*only for halmac_read_indirect_sdio_88xx !!, Soar 20171206*/
static u8
halmac_read_indirect_cmd52_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 halmac_offset
);
 
/*only for halmac_read_indirect_sdio_88xx !!, Soar 20171206*/
static u32
halmac_read_indirect_cmd53_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 halmac_offset
);

/**
 * halmac_init_sdio_cfg_88xx() - init SDIO
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_sdio_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u32 value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (pHalmac_adapter->halmac_interface != HALMAC_INTERFACE_SDIO)
		return HALMAC_RET_WRONG_INTF;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_sdio_cfg_88xx ==========>\n");

	HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_FREE_TXPG);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_TX_CTRL);
	value32 &= 0x0000FFFF;
	value32 &= ~(BIT_CMD_ERR_STOP_INT_EN | BIT_EN_MASK_TIMER | BIT_EN_RXDMA_MASK_INT);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SDIO_TX_CTRL, value32);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_sdio_cfg_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_deinit_sdio_cfg_88xx() - deinit SDIO
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_deinit_sdio_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (pHalmac_adapter->halmac_interface != HALMAC_INTERFACE_SDIO)
		return HALMAC_RET_WRONG_INTF;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_deinit_sdio_cfg_88xx ==========>\n");

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_deinit_sdio_cfg_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_rx_aggregation_88xx_sdio() - config rx aggregation
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_rx_agg_mode
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_rx_aggregation_88xx_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_RXAGG_CFG phalmac_rxagg_cfg
)
{
	u8 value8;
	u8 size = 0, timeout = 0, agg_enable = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_rx_aggregation_88xx_sdio ==========>\n");

	agg_enable = HALMAC_REG_READ_8(pHalmac_adapter, REG_TXDMA_PQ_MAP);

	switch (phalmac_rxagg_cfg->mode) {
	case HALMAC_RX_AGG_MODE_NONE:
		agg_enable &= ~(BIT_RXDMA_AGG_EN);
		break;
	case HALMAC_RX_AGG_MODE_DMA:
	case HALMAC_RX_AGG_MODE_USB:
		agg_enable |= BIT_RXDMA_AGG_EN;
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_cfg_rx_aggregation_88xx_usb switch case not support\n");
		agg_enable &= ~BIT_RXDMA_AGG_EN;
		break;
	}

	if (phalmac_rxagg_cfg->threshold.drv_define == _FALSE) {
		size = 0xFF;
		timeout = 0x01;
	} else {
		size = phalmac_rxagg_cfg->threshold.size;
		timeout = phalmac_rxagg_cfg->threshold.timeout;
	}

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_PQ_MAP, agg_enable);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RXDMA_AGG_PG_TH, (u16)(size | (timeout << BIT_SHIFT_DMA_AGG_TO_V1)));

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_RXDMA_MODE);
	if (0 != (agg_enable & BIT_RXDMA_AGG_EN))
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RXDMA_MODE, value8 | BIT_DMA_MODE);
	else
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RXDMA_MODE, value8 & ~(BIT_DMA_MODE));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_rx_aggregation_88xx_sdio <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_reg_read_nbyte_sdio_88xx() - read n byte register
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : register offset
 * @halmac_size : register value size
 * @halmac_data : register value
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_reg_read_nbyte_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u32 halmac_size,
	OUT u8 *halmac_data
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (0 == (halmac_offset & 0xFFFF0000)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_offset error = 0x%x\n", halmac_offset);
		return HALMAC_RET_FAIL;
	}

	status = halmac_convert_to_sdio_bus_offset_88xx(pHalmac_adapter, &halmac_offset);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_reg_read_nbyte_sdio_88xx error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_state error = 0x%x\n", pHalmac_adapter->halmac_state.mac_power);
		return HALMAC_RET_FAIL;
	}

	PLATFORM_SDIO_CMD53_READ_N(pDriver_adapter, halmac_offset, halmac_size, halmac_data);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_tx_agg_align_sdio_88xx() -config sdio bus tx agg alignment
 * @pHalmac_adapter : the adapter of halmac
 * @enable : function enable(1)/disable(0)
 * @align_size : sdio bus tx agg alignment size (2^n, n = 3~11)
 * Author : Soar Tu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_tx_agg_align_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable,
	IN u16 align_size
)
{
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;
	u8 i, align_size_ok = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;


	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_tx_agg_align_sdio_88xx ==========>\n");

	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822B)
		return HALMAC_RET_NOT_SUPPORT;

	if ((align_size & 0xF000) != 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Align size is out of range\n");
		return HALMAC_RET_FAIL;
	}

	for (i = 3; i <= 11; i++) {
		if (align_size == 1 << i) {
			align_size_ok = 1;
			break;
		}
	}
	if (align_size_ok == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Align size is not 2^3 ~ 2^11\n");
		return HALMAC_RET_FAIL;
	}

	/*Keep sdio tx agg alignment size for driver query*/
	pHalmac_adapter->hw_config_info.tx_align_size = align_size;

	if (enable)
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RQPN_CTRL_2, 0x8000 | align_size);
	else
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RQPN_CTRL_2, align_size);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_tx_agg_align_sdio_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_reg_read_indirect_32_sdio_88xx() - read MAC reg by SDIO reg
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : register offset
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
u32
halmac_reg_read_indirect_32_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset
)
{
	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	return halmac_read_indirect_sdio_88xx(pHalmac_adapter, (u16)halmac_offset, HALMAC_IO_DWORD);
}

/**
 * halmac_set_bulkout_num_sdio_88xx() - inform bulk-out num
 * @pHalmac_adapter : the adapter of halmac
 * @bulkout_num : usb bulk-out number
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_set_bulkout_num_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 bulkout_num
)
{
	return HALMAC_RET_NOT_SUPPORT;
}

/**
 * halmac_get_usb_bulkout_id_sdio_88xx() - get bulk out id for the TX packet
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_buf : tx packet, include txdesc
 * @halmac_size : tx packet size
 * @bulkout_id : usb bulk-out id
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_usb_bulkout_id_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size,
	OUT u8 *bulkout_id
)
{
	return HALMAC_RET_NOT_SUPPORT;
}

/**
 * halmac_sdio_cmd53_4byte_88xx() - cmd53 only for 4byte len register IO
 * @pHalmac_adapter : the adapter of halmac
 * @enable : 1->CMD53 only use in 4byte reg, 0 : No limitation
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_sdio_cmd53_4byte_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_SDIO_CMD53_4BYTE_MODE cmd53_4byte_mode
)
{
	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (pHalmac_adapter->halmac_interface != HALMAC_INTERFACE_SDIO)
		return HALMAC_RET_WRONG_INTF;

	if (pHalmac_adapter->api_registry.sdio_cmd53_4byte_en == 0)
		return HALMAC_RET_NOT_SUPPORT;

	pHalmac_adapter->sdio_cmd53_4byte = cmd53_4byte_mode;

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_sdio_hw_info_88xx() - info sdio hw info
 * @pHalmac_adapter : the adapter of halmac
 * @HALMAC_SDIO_CMD53_4BYTE_MODE :
 * clock_speed : sdio bus clock. Unit -> MHz
 * spec_ver : sdio spec version
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_sdio_hw_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_SDIO_HW_INFO pSdio_hw_info
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_sdio_hw_info_88xx ==========>\n");

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (pHalmac_adapter->halmac_interface != HALMAC_INTERFACE_SDIO)
		return HALMAC_RET_WRONG_INTF;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]SDIO hw clock : %d, spec : %d\n", pSdio_hw_info->clock_speed, pSdio_hw_info->spec_ver);

	if (pSdio_hw_info->clock_speed > HALMAC_SDIO_CLOCK_SPEED_MAX_88XX)
		return HALMAC_RET_SDIO_CLOCK_ERR;

	if (pSdio_hw_info->clock_speed > HALMAC_SDIO_CLK_THRESHOLD_88XX)
		pHalmac_adapter->sdio_hw_info.io_hi_speed_flag = 1;

	pHalmac_adapter->sdio_hw_info.io_indir_flag = pSdio_hw_info->io_indir_flag;
	if (pSdio_hw_info->clock_speed > HALMAC_SDIO_CLK_THRESHOLD_88XX && pHalmac_adapter->sdio_hw_info.io_indir_flag == 0)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN,"[WARN]SDIO clock:%d, indir access is better\n", pSdio_hw_info->clock_speed);

	pHalmac_adapter->sdio_hw_info.clock_speed = pSdio_hw_info->clock_speed;
	pHalmac_adapter->sdio_hw_info.spec_ver = pSdio_hw_info->spec_ver;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_sdio_hw_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

VOID
halmac_config_sdio_tx_page_threshold_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_TX_PAGE_THRESHOLD_INFO pThreshold_info
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u32 threshold = pThreshold_info->threshold;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_config_sdio_tx_page_threshold_88xx ==========>\n");

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]queue %d, threshold 0x%X\n", pThreshold_info->dma_queue_sel, threshold);

	if (pThreshold_info->enable == 1) {
		threshold = BIT(31) | threshold;
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]enable\n");
	} else {
		threshold = ~(BIT(31)) & threshold;
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]disable\n");
	}

	switch (pThreshold_info->dma_queue_sel) {
	case HALMAC_MAP2_HQ:
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TQPNT1, threshold);
		break;
	case HALMAC_MAP2_NQ:
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TQPNT2, threshold);
		break;
	case HALMAC_MAP2_LQ:
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TQPNT3, threshold);
		break;
	case HALMAC_MAP2_EXQ:
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TQPNT4, threshold);
		break;
	default:
		break;
	}
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_config_sdio_tx_page_threshold_88xx <==========\n");
}

/*Note: copy to halmac_convert_to_sdio_bus_offset*/
HALMAC_RET_STATUS
halmac_convert_to_sdio_bus_offset_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT u32 *halmac_offset
)
{
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	switch ((*halmac_offset) & 0xFFFF0000) {
	case WLAN_IOREG_OFFSET:
		*halmac_offset = (HALMAC_SDIO_CMD_ADDR_MAC_REG << 13) | (*halmac_offset & HALMAC_WLAN_MAC_REG_MSK);
		break;
	case SDIO_LOCAL_OFFSET:
		*halmac_offset = (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | (*halmac_offset & HALMAC_SDIO_LOCAL_MSK);
		break;
	default:
		*halmac_offset = 0xFFFFFFFF;
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Unknown base address!!\n");
		return HALMAC_RET_CONVERT_SDIO_OFFSET_FAIL;
	}

	return HALMAC_RET_SUCCESS;
}

/*only for halmac_read_indirect_sdio_88xx !!, Soar 20171206*/
static u8
halmac_read_indirect_cmd52_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 halmac_offset
)
{
	u8 value8;
	u8 rtemp = 0xFF;
	u32 counter = 50;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | (REG_SDIO_INDIRECT_REG_CFG & HALMAC_SDIO_LOCAL_MSK), (u8)halmac_offset);
	PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_CFG + 1) & HALMAC_SDIO_LOCAL_MSK), (u8)(halmac_offset >> 8));
	PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_CFG + 2) & HALMAC_SDIO_LOCAL_MSK), (u8)BIT(3));

	do {
		rtemp = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_CFG + 2) & HALMAC_SDIO_LOCAL_MSK));
		counter--;
	} while (((rtemp & BIT(4)) == 0) && (counter > 0));

	if (((rtemp & BIT(4)) == 0) && (counter == 0))
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "halmac_read_indirect_cmd52_sdio_88xx fail, offset = 0x%x\n", halmac_offset);

	value8 = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | (REG_SDIO_INDIRECT_REG_DATA & HALMAC_SDIO_LOCAL_MSK));

	return value8;
}

/*only for halmac_read_indirect_sdio_88xx !!, Soar 20171206*/
static u32
halmac_read_indirect_cmd53_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 halmac_offset
)
{
	u8 rtemp = 0xFF;
	u32 counter = 50;
	VOID *pDriver_adapter = NULL;

	union {
		u32	dword;
		u8	byte[4];
	} value32 = { 0x00000000 };

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_SDIO_CMD53_WRITE_32(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | (REG_SDIO_INDIRECT_REG_CFG & HALMAC_SDIO_LOCAL_MSK), (u32)halmac_offset | BIT(19) | BIT(17));

	do {
		rtemp = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_CFG + 2) & HALMAC_SDIO_LOCAL_MSK));
		counter--;
	} while (((rtemp & BIT(4)) == 0) && (counter > 0));

	if (((rtemp & BIT(4)) == 0) && (counter == 0))
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "halmac_read_indirect_cmd53_sdio_88xx fail, offset = 0x%x\n", halmac_offset);

	value32.dword = PLATFORM_SDIO_CMD53_READ_32(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | (REG_SDIO_INDIRECT_REG_DATA & HALMAC_SDIO_LOCAL_MSK));

	return value32.dword;
}

u32
halmac_read_indirect_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 halmac_offset,
	IN HALMAC_IO_SIZE size
)
{
	VOID *pDriver_adapter = NULL;

	union {
		u32	dword;
		u8	byte[4];
	} value32 = { 0x00000000 };

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MUTEX_LOCK(pDriver_adapter, &(pHalmac_adapter->sdio_indirect_mutex));

	switch (size) {
	case HALMAC_IO_BYTE:
		if (pHalmac_adapter->pwr_off_flow_flag == 1 ||
		    pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF) {
			value32.byte[0] = halmac_read_indirect_cmd52_sdio_88xx(pHalmac_adapter, halmac_offset);
			value32.dword = rtk_le32_to_cpu(value32.dword);
		} else {
			value32.dword = halmac_read_indirect_cmd53_sdio_88xx(pHalmac_adapter, halmac_offset);
		}
		break;
	case HALMAC_IO_WORD:
		if (HALMAC_MAC_POWER_OFF == pHalmac_adapter->halmac_state.mac_power) {
			if (0 != (halmac_offset & (2 - 1))) {
				value32.byte[0] = halmac_read_indirect_cmd52_sdio_88xx(pHalmac_adapter, halmac_offset);
				value32.byte[1] = halmac_read_indirect_cmd52_sdio_88xx(pHalmac_adapter, halmac_offset + 1);
			} else {
				value32.byte[0] = halmac_read_indirect_cmd52_sdio_88xx(pHalmac_adapter, halmac_offset);
				value32.byte[1] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_DATA + 1) & HALMAC_SDIO_LOCAL_MSK));
			}
			value32.dword = rtk_le32_to_cpu(value32.dword);
		} else {
			if (0 != (halmac_offset & (2 - 1))) {
				value32.byte[0] = (u8)halmac_read_indirect_cmd53_sdio_88xx(pHalmac_adapter, halmac_offset);
				value32.byte[1] = (u8)halmac_read_indirect_cmd53_sdio_88xx(pHalmac_adapter, halmac_offset + 1);
				value32.dword = rtk_le32_to_cpu(value32.dword);
			} else {
				value32.dword = halmac_read_indirect_cmd53_sdio_88xx(pHalmac_adapter, halmac_offset);
			}
		}
		break;
	case HALMAC_IO_DWORD:
		if (HALMAC_MAC_POWER_OFF == pHalmac_adapter->halmac_state.mac_power) {
			if (0 != (halmac_offset & (4 - 1))) {
				value32.byte[0] = halmac_read_indirect_cmd52_sdio_88xx(pHalmac_adapter, halmac_offset);
				value32.byte[1] = halmac_read_indirect_cmd52_sdio_88xx(pHalmac_adapter, halmac_offset + 1);
				value32.byte[2] = halmac_read_indirect_cmd52_sdio_88xx(pHalmac_adapter, halmac_offset + 2);
				value32.byte[3] = halmac_read_indirect_cmd52_sdio_88xx(pHalmac_adapter, halmac_offset + 3);
			} else {
				value32.byte[0] = halmac_read_indirect_cmd52_sdio_88xx(pHalmac_adapter, halmac_offset);
				value32.byte[1] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_DATA + 1) & HALMAC_SDIO_LOCAL_MSK));
				value32.byte[2] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_DATA + 2) & HALMAC_SDIO_LOCAL_MSK));
				value32.byte[3] = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, (HALMAC_SDIO_CMD_ADDR_SDIO_REG << 13) | ((REG_SDIO_INDIRECT_REG_DATA + 3) & HALMAC_SDIO_LOCAL_MSK));
			}
			value32.dword = rtk_le32_to_cpu(value32.dword);
		} else {
			if (0 != (halmac_offset & (4 - 1))) {
				value32.byte[0] = (u8)halmac_read_indirect_cmd53_sdio_88xx(pHalmac_adapter, halmac_offset);
				value32.byte[1] = (u8)halmac_read_indirect_cmd53_sdio_88xx(pHalmac_adapter, halmac_offset + 1);
				value32.byte[2] = (u8)halmac_read_indirect_cmd53_sdio_88xx(pHalmac_adapter, halmac_offset + 2);
				value32.byte[3] = (u8)halmac_read_indirect_cmd53_sdio_88xx(pHalmac_adapter, halmac_offset + 3);
				value32.dword = rtk_le32_to_cpu(value32.dword);
			} else {
				value32.dword = halmac_read_indirect_cmd53_sdio_88xx(pHalmac_adapter, halmac_offset);
			}
		}
		break;
	default:
		break;
	}

	PLATFORM_MUTEX_UNLOCK(pDriver_adapter, &(pHalmac_adapter->sdio_indirect_mutex));

	return value32.dword;	
}

#endif /* HALMAC_88XX_SUPPORT */

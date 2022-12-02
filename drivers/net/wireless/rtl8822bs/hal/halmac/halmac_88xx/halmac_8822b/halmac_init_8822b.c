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

#include "halmac_init_8822b.h"
#include "halmac_8822b_cfg.h"
#include "halmac_pcie_8822b.h"
#include "halmac_sdio_8822b.h"
#include "halmac_usb_8822b.h"
#include "halmac_gpio_8822b.h"
#include "halmac_common_8822b.h"
#include "halmac_cfg_wmac_8822b.h"
#include "../halmac_common_88xx.h"
#include "../halmac_init_88xx.h"

#if HALMAC_8822B_SUPPORT

#if HALMAC_PLATFORM_WINDOWS
/*SDIO RQPN Mapping for Windows, extra queue is not implemented in Driver code*/
HALMAC_RQPN HALMAC_RQPN_SDIO_8822B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
};
#else
/*SDIO RQPN Mapping*/
HALMAC_RQPN HALMAC_RQPN_SDIO_8822B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
};
#endif

/*PCIE RQPN Mapping*/
HALMAC_RQPN HALMAC_RQPN_PCIE_8822B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
};

/*USB 2 Bulkout RQPN Mapping*/
HALMAC_RQPN HALMAC_RQPN_2BULKOUT_8822B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
};

/*USB 3 Bulkout RQPN Mapping*/
HALMAC_RQPN HALMAC_RQPN_3BULKOUT_8822B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ, HALMAC_MAP2_HQ, HALMAC_MAP2_HQ},
};

/*USB 4 Bulkout RQPN Mapping*/
HALMAC_RQPN HALMAC_RQPN_4BULKOUT_8822B[] = {
	/* { mode, vo_map, vi_map, be_map, bk_map, mg_map, hi_map } */
	{HALMAC_TRX_MODE_NORMAL, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_TRXSHARE, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_WMM, HALMAC_MAP2_HQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_NQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_P2P, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_LOOPBACK, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, HALMAC_MAP2_NQ, HALMAC_MAP2_NQ, HALMAC_MAP2_LQ, HALMAC_MAP2_LQ, HALMAC_MAP2_EXQ, HALMAC_MAP2_HQ},
};

#if HALMAC_PLATFORM_WINDOWS
/*SDIO Page Number*/
HALMAC_PG_NUM HALMAC_PG_NUM_SDIO_8822B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 64, 64, 64, 0, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 32, 32, 32, 0, 1},
	{HALMAC_TRX_MODE_WMM, 64, 64, 64, 0, 1},
	{HALMAC_TRX_MODE_P2P, 64, 64, 64, 0, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 64, 64, 64, 0, 640},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 64, 64, 64, 0, 640},
};
#else
/*SDIO Page Number*/
HALMAC_PG_NUM HALMAC_PG_NUM_SDIO_8822B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 32, 32, 32, 32, 1},
	{HALMAC_TRX_MODE_WMM, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_P2P, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 64, 64, 64, 64, 640},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 64, 64, 64, 64, 640},
};
#endif

/*PCIE Page Number*/
HALMAC_PG_NUM HALMAC_PG_NUM_PCIE_8822B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_WMM, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_P2P, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 64, 64, 64, 64, 640},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 64, 64, 64, 64, 640},
};

/*USB 2 Bulkout Page Number*/
HALMAC_PG_NUM HALMAC_PG_NUM_2BULKOUT_8822B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 64, 64, 0, 0, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 64, 64, 0, 0, 1},
	{HALMAC_TRX_MODE_WMM, 64, 64, 0, 0, 1},
	{HALMAC_TRX_MODE_P2P, 64, 64, 0, 0, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 64, 64, 0, 0, 1024},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 64, 64, 0, 0, 1024},
};

/*USB 3 Bulkout Page Number*/
HALMAC_PG_NUM HALMAC_PG_NUM_3BULKOUT_8822B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 64, 64, 64, 0, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 64, 64, 64, 0, 1},
	{HALMAC_TRX_MODE_WMM, 64, 64, 64, 0, 1},
	{HALMAC_TRX_MODE_P2P, 64, 64, 64, 0, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 64, 64, 64, 0, 1024},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 64, 64, 64, 0, 1024},
};

/*USB 4 Bulkout Page Number*/
HALMAC_PG_NUM HALMAC_PG_NUM_4BULKOUT_8822B[] = {
	/* { mode, hq_num, nq_num, lq_num, exq_num, gap_num} */
	{HALMAC_TRX_MODE_NORMAL, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_TRXSHARE, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_WMM, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_P2P, 64, 64, 64, 64, 1},
	{HALMAC_TRX_MODE_LOOPBACK, 64, 64, 64, 64, 640},
	{HALMAC_TRX_MODE_DELAY_LOOPBACK, 64, 64, 64, 64, 640},
};


static HALMAC_RET_STATUS
halmac_txdma_queue_mapping_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode
);

static HALMAC_RET_STATUS
halmac_priority_queue_config_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode
);

HALMAC_RET_STATUS
halmac_mount_api_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	pHalmac_adapter->chip_id = HALMAC_CHIP_ID_8822B;
	pHalmac_adapter->hw_config_info.efuse_size = HALMAC_EFUSE_SIZE_8822B;
	pHalmac_adapter->hw_config_info.eeprom_size = HALMAC_EEPROM_SIZE_8822B;
	pHalmac_adapter->hw_config_info.bt_efuse_size = HALMAC_BT_EFUSE_SIZE_8822B;
	pHalmac_adapter->hw_config_info.cam_entry_num = HALMAC_SECURITY_CAM_ENTRY_NUM_8822B;
	pHalmac_adapter->hw_config_info.txdesc_size = HALMAC_TX_DESC_SIZE_8822B;
	pHalmac_adapter->hw_config_info.rxdesc_size = HALMAC_RX_DESC_SIZE_8822B;
	pHalmac_adapter->hw_config_info.tx_fifo_size = HALMAC_TX_FIFO_SIZE_8822B;
	pHalmac_adapter->hw_config_info.rx_fifo_size = HALMAC_RX_FIFO_SIZE_8822B;
	pHalmac_adapter->hw_config_info.page_size = HALMAC_TX_PAGE_SIZE_8822B;
	pHalmac_adapter->hw_config_info.tx_align_size = HALMAC_TX_ALIGN_SIZE_8822B;
	pHalmac_adapter->hw_config_info.page_size_2_power = HALMAC_TX_PAGE_SIZE_2_POWER_8822B;
	pHalmac_adapter->hw_config_info.ac_oqt_size = HALMAC_OQT_ENTRY_AC_8822B;
	pHalmac_adapter->hw_config_info.non_ac_oqt_size = HALMAC_OQT_ENTRY_NOAC_8822B;
	pHalmac_adapter->hw_config_info.usb_txagg_num = HALMAC_BLK_DESC_NUM_8822B;
	pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = HALMAC_RSVD_DRV_PGNUM_8822B;

	pHalmac_api->halmac_init_trx_cfg = halmac_init_trx_cfg_8822b;
	pHalmac_api->halmac_init_protocol_cfg = halmac_init_protocol_cfg_8822b;
	pHalmac_api->halmac_init_h2c = halmac_init_h2c_8822b;
	pHalmac_api->halmac_pinmux_get_func = halmac_pinmux_get_func_8822b;
	pHalmac_api->halmac_pinmux_set_func = halmac_pinmux_set_func_8822b;
	pHalmac_api->halmac_pinmux_free_func = halmac_pinmux_free_func_8822b;
	pHalmac_api->halmac_get_hw_value = halmac_get_hw_value_8822b;
	pHalmac_api->halmac_set_hw_value = halmac_set_hw_value_8822b;
	pHalmac_api->halmac_cfg_drv_info = halmac_cfg_drv_info_8822b;

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		pHalmac_api->halmac_mac_power_switch = halmac_mac_power_switch_8822b_sdio;
		pHalmac_api->halmac_phy_cfg = halmac_phy_cfg_8822b_sdio;
		pHalmac_api->halmac_pcie_switch = halmac_pcie_switch_8822b_sdio;
		pHalmac_api->halmac_interface_integration_tuning = halmac_interface_integration_tuning_8822b_sdio;
		pHalmac_api->halmac_tx_allowed_sdio = halmac_tx_allowed_8822b_sdio;
		pHalmac_api->halmac_get_sdio_tx_addr = halmac_get_sdio_tx_addr_8822b_sdio;
 		pHalmac_api->halmac_reg_read_8 = halmac_reg_read_8_sdio_8822b;
		pHalmac_api->halmac_reg_write_8 = halmac_reg_write_8_sdio_8822b;
		pHalmac_api->halmac_reg_read_16 = halmac_reg_read_16_sdio_8822b;
		pHalmac_api->halmac_reg_write_16 = halmac_reg_write_16_sdio_8822b;
		pHalmac_api->halmac_reg_read_32 = halmac_reg_read_32_sdio_8822b;
		pHalmac_api->halmac_reg_write_32 = halmac_reg_write_32_sdio_8822b;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_USB) {
		pHalmac_api->halmac_mac_power_switch = halmac_mac_power_switch_8822b_usb;
		pHalmac_api->halmac_phy_cfg = halmac_phy_cfg_8822b_usb;
		pHalmac_api->halmac_pcie_switch = halmac_pcie_switch_8822b_usb;
		pHalmac_api->halmac_interface_integration_tuning = halmac_interface_integration_tuning_8822b_usb;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_PCIE) {
		pHalmac_api->halmac_mac_power_switch = halmac_mac_power_switch_8822b_pcie;
		pHalmac_api->halmac_phy_cfg = halmac_phy_cfg_8822b_pcie;
		pHalmac_api->halmac_pcie_switch = halmac_pcie_switch_8822b_pcie;
		pHalmac_api->halmac_interface_integration_tuning = halmac_interface_integration_tuning_8822b_pcie;
	} else {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Undefined IC\n");
		return HALMAC_RET_CHIP_NOT_SUPPORT;
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_trx_cfg_8822b() - config trx dma register
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_trx_mode : trx mode selection
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_trx_cfg_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode
)
{
	u8 value8;
	u32 value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;
	pHalmac_adapter->trx_mode = halmac_trx_mode;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_trx_cfg ==========>halmac_trx_mode = %d\n", halmac_trx_mode);

	status = halmac_txdma_queue_mapping_8822b(pHalmac_adapter, halmac_trx_mode);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_txdma_queue_mapping fail!\n");
		return status;
	}

	value8 = 0;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);
	value8 = HALMAC_CR_TRX_ENABLE_8822B;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2CQ_CSR, BIT(31));

	status = halmac_priority_queue_config_8822b(pHalmac_adapter, halmac_trx_mode);
	if (pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode != HALMAC_RX_FIFO_EXPANDING_MODE_DISABLE)
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RX_DRVINFO_SZ, HALMAC_RX_DESC_DUMMY_SIZE_MAX_8822B >> 3);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_txdma_queue_mapping fail!\n");
		return status;
	}

	/* Config H2C packet buffer */
	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_H2C_HEAD);
	value32 = (value32 & 0xFFFC0000) | (pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy << HALMAC_TX_PAGE_SIZE_2_POWER_8822B);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2C_HEAD, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_H2C_READ_ADDR);
	value32 = (value32 & 0xFFFC0000) | (pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy << HALMAC_TX_PAGE_SIZE_2_POWER_8822B);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2C_READ_ADDR, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_H2C_TAIL);
	value32 = (value32 & 0xFFFC0000) | ((pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy << HALMAC_TX_PAGE_SIZE_2_POWER_8822B) + (HALMAC_RSVD_H2C_QUEUE_PGNUM_8822B << HALMAC_TX_PAGE_SIZE_2_POWER_8822B));
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2C_TAIL, value32);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_H2C_INFO);
	value8 = (u8)((value8 & 0xFC) | 0x01);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_H2C_INFO, value8);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_H2C_INFO);
	value8 = (u8)((value8 & 0xFB) | 0x04);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_H2C_INFO, value8);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_TXDMA_OFFSET_CHK + 1);
	value8 = (u8)((value8 & 0x7f) | 0x80);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_OFFSET_CHK + 1, value8);

	pHalmac_adapter->h2c_buff_size = HALMAC_RSVD_H2C_QUEUE_PGNUM_8822B << HALMAC_TX_PAGE_SIZE_2_POWER_8822B;
	halmac_get_h2c_buff_free_space_88xx(pHalmac_adapter);

	if (pHalmac_adapter->h2c_buff_size != pHalmac_adapter->h2c_buf_free_space) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]get h2c free space error!\n");
		return HALMAC_RET_GET_H2C_SPACE_ERR;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_trx_cfg <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_txdma_queue_mapping_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode
)
{
	u16 value16;
	VOID *pDriver_adapter = NULL;
	PHALMAC_RQPN pCurr_rqpn_Sel = NULL;
	HALMAC_RET_STATUS status;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		pCurr_rqpn_Sel = HALMAC_RQPN_SDIO_8822B;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_PCIE) {
		pCurr_rqpn_Sel = HALMAC_RQPN_PCIE_8822B;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_USB) {
		if (pHalmac_adapter->halmac_bulkout_num == 2) {
			pCurr_rqpn_Sel = HALMAC_RQPN_2BULKOUT_8822B;
		} else if (pHalmac_adapter->halmac_bulkout_num == 3) {
			pCurr_rqpn_Sel = HALMAC_RQPN_3BULKOUT_8822B;
		} else if (pHalmac_adapter->halmac_bulkout_num == 4) {
			pCurr_rqpn_Sel = HALMAC_RQPN_4BULKOUT_8822B;
		} else {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]interface not support\n");
			return HALMAC_RET_NOT_SUPPORT;
		}
	} else {
		return HALMAC_RET_NOT_SUPPORT;
	}

	status = halmac_rqpn_parser_88xx(pHalmac_adapter, halmac_trx_mode, pCurr_rqpn_Sel);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	value16 = 0;
	value16 |= BIT_TXDMA_HIQ_MAP(pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI]);
	value16 |= BIT_TXDMA_MGQ_MAP(pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_MG]);
	value16 |= BIT_TXDMA_BKQ_MAP(pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BK]);
	value16 |= BIT_TXDMA_BEQ_MAP(pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BE]);
	value16 |= BIT_TXDMA_VIQ_MAP(pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VI]);
	value16 |= BIT_TXDMA_VOQ_MAP(pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VO]);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXDMA_PQ_MAP, value16);

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_priority_queue_config_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode
)
{
	u8 transfer_mode = 0;
	u8 value8;
	u32 counter;
	HALMAC_RET_STATUS status;
	PHALMAC_PG_NUM pCurr_pg_num = NULL;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	pHalmac_adapter->hw_config_info.tx_fifo_size = HALMAC_TX_FIFO_SIZE_8822B;
	pHalmac_adapter->hw_config_info.rx_fifo_size = HALMAC_RX_FIFO_SIZE_8822B;

	if (pHalmac_adapter->txff_allocation.la_mode == HALMAC_LA_MODE_DISABLE) {
		if (pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode == HALMAC_RX_FIFO_EXPANDING_MODE_DISABLE) {
			pHalmac_adapter->txff_allocation.tx_fifo_pg_num = HALMAC_TX_FIFO_SIZE_8822B >> HALMAC_TX_PAGE_SIZE_2_POWER_8822B;
		} else if (pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode == HALMAC_RX_FIFO_EXPANDING_MODE_1_BLOCK) {
			pHalmac_adapter->txff_allocation.tx_fifo_pg_num = HALMAC_TX_FIFO_SIZE_RX_FIFO_EXPANDING_1_BLOCK_8822B >> HALMAC_TX_PAGE_SIZE_2_POWER_8822B;
			pHalmac_adapter->hw_config_info.tx_fifo_size = HALMAC_TX_FIFO_SIZE_RX_FIFO_EXPANDING_1_BLOCK_8822B;
			if (HALMAC_RX_FIFO_SIZE_RX_FIFO_EXPANDING_1_BLOCK_8822B <= HALMAC_RX_FIFO_SIZE_RX_FIFO_EXPANDING_1_BLOCK_MAX_8822B)
				pHalmac_adapter->hw_config_info.rx_fifo_size = HALMAC_RX_FIFO_SIZE_RX_FIFO_EXPANDING_1_BLOCK_8822B;
			else
				pHalmac_adapter->hw_config_info.rx_fifo_size = HALMAC_RX_FIFO_SIZE_RX_FIFO_EXPANDING_1_BLOCK_MAX_8822B;
		} else {
			pHalmac_adapter->txff_allocation.tx_fifo_pg_num = HALMAC_TX_FIFO_SIZE_8822B >> HALMAC_TX_PAGE_SIZE_2_POWER_8822B;
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]rx_fifo_expanding_mode = %d not support\n", pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode);
		}
	} else {
		pHalmac_adapter->txff_allocation.tx_fifo_pg_num = HALMAC_TX_FIFO_SIZE_LA_8822B >> HALMAC_TX_PAGE_SIZE_2_POWER_8822B;
	}
	pHalmac_adapter->txff_allocation.rsvd_pg_num = (pHalmac_adapter->txff_allocation.rsvd_drv_pg_num +
							HALMAC_RSVD_H2C_EXTRAINFO_PGNUM_8822B +
							HALMAC_RSVD_H2C_STATICINFO_PGNUM_8822B +
							HALMAC_RSVD_H2C_QUEUE_PGNUM_8822B +
							HALMAC_RSVD_CPU_INSTRUCTION_PGNUM_8822B +
							HALMAC_RSVD_FW_TXBUFF_PGNUM_8822B);

	if (halmac_trx_mode == HALMAC_TRX_MODE_DELAY_LOOPBACK)
		pHalmac_adapter->txff_allocation.rsvd_pg_num += HALMAC_RSVD_DLLB_PGNUM_8822B;

	if (pHalmac_adapter->txff_allocation.rsvd_pg_num > pHalmac_adapter->txff_allocation.tx_fifo_pg_num)
		return HALMAC_RET_CFG_TXFIFO_PAGE_FAIL;

	pHalmac_adapter->txff_allocation.ac_q_pg_num = pHalmac_adapter->txff_allocation.tx_fifo_pg_num - pHalmac_adapter->txff_allocation.rsvd_pg_num;
	pHalmac_adapter->txff_allocation.rsvd_pg_bndy = pHalmac_adapter->txff_allocation.tx_fifo_pg_num - pHalmac_adapter->txff_allocation.rsvd_pg_num;
	pHalmac_adapter->txff_allocation.rsvd_fw_txbuff_pg_bndy = pHalmac_adapter->txff_allocation.tx_fifo_pg_num - HALMAC_RSVD_FW_TXBUFF_PGNUM_8822B;
	pHalmac_adapter->txff_allocation.rsvd_cpu_instr_pg_bndy = pHalmac_adapter->txff_allocation.rsvd_fw_txbuff_pg_bndy - HALMAC_RSVD_CPU_INSTRUCTION_PGNUM_8822B;
	pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy = pHalmac_adapter->txff_allocation.rsvd_cpu_instr_pg_bndy - HALMAC_RSVD_H2C_QUEUE_PGNUM_8822B;
	pHalmac_adapter->txff_allocation.rsvd_h2c_static_info_pg_bndy = pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy - HALMAC_RSVD_H2C_STATICINFO_PGNUM_8822B;
	pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy = pHalmac_adapter->txff_allocation.rsvd_h2c_static_info_pg_bndy - HALMAC_RSVD_H2C_EXTRAINFO_PGNUM_8822B;
	pHalmac_adapter->txff_allocation.rsvd_drv_pg_bndy = pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy - pHalmac_adapter->txff_allocation.rsvd_drv_pg_num;

	if (pHalmac_adapter->txff_allocation.rsvd_pg_bndy != pHalmac_adapter->txff_allocation.rsvd_drv_pg_bndy)
		return HALMAC_RET_CFG_TXFIFO_PAGE_FAIL;

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		pCurr_pg_num = HALMAC_PG_NUM_SDIO_8822B;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_PCIE) {
		pCurr_pg_num = HALMAC_PG_NUM_PCIE_8822B;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_USB) {
		if (pHalmac_adapter->halmac_bulkout_num == 2) {
			pCurr_pg_num = HALMAC_PG_NUM_2BULKOUT_8822B;
		} else if (pHalmac_adapter->halmac_bulkout_num == 3) {
			pCurr_pg_num = HALMAC_PG_NUM_3BULKOUT_8822B;
		} else if (pHalmac_adapter->halmac_bulkout_num == 4) {
			pCurr_pg_num = HALMAC_PG_NUM_4BULKOUT_8822B;
		} else {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]interface not support\n");
			return HALMAC_RET_NOT_SUPPORT;
		}
	} else {
		return HALMAC_RET_NOT_SUPPORT;
	}

	status = halmac_pg_num_parser_88xx(pHalmac_adapter, halmac_trx_mode, pCurr_pg_num);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_1, pHalmac_adapter->txff_allocation.high_queue_pg_num);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_2, pHalmac_adapter->txff_allocation.low_queue_pg_num);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_3, pHalmac_adapter->txff_allocation.normal_queue_pg_num);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_4, pHalmac_adapter->txff_allocation.extra_queue_pg_num);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_5, pHalmac_adapter->txff_allocation.pub_queue_pg_num);

	pHalmac_adapter->sdio_free_space.high_queue_number = pHalmac_adapter->txff_allocation.high_queue_pg_num;
	pHalmac_adapter->sdio_free_space.normal_queue_number = pHalmac_adapter->txff_allocation.normal_queue_pg_num;
	pHalmac_adapter->sdio_free_space.low_queue_number = pHalmac_adapter->txff_allocation.low_queue_pg_num;
	pHalmac_adapter->sdio_free_space.public_queue_number = pHalmac_adapter->txff_allocation.pub_queue_pg_num;
	pHalmac_adapter->sdio_free_space.extra_queue_number = pHalmac_adapter->txff_allocation.extra_queue_pg_num;

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RQPN_CTRL_2, HALMAC_REG_READ_32(pHalmac_adapter, REG_RQPN_CTRL_2) | BIT(31));

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2) | BIT(4));
	/*20170411 Soar*/
	/*SDIO sometimes use two CMD52 to do HALMAC_REG_WRITE_16 and may cause a mismatch between HW status and Reg value.*/
	/*A patch is to write high byte first, suggested by Argis*/
	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCNQ_BDNY_V1 + 1, (u8)((pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCNQ_PGBNDY_V1) >> 8));
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCNQ_BDNY_V1, (u8)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCNQ_PGBNDY_V1));
	} else {
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BCNQ_BDNY_V1, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCNQ_PGBNDY_V1));
	}


	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2 + 2, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));
	/*20170411 Soar*/
	/*SDIO sometimes use two CMD52 to do HALMAC_REG_WRITE_16 and may cause a mismatch between HW status and Reg value.*/
	/*A patch is to write high byte first, suggested by Argis*/
	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCNQ1_BDNY_V1 + 1, (u8)((pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCNQ_PGBNDY_V1) >> 8));
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCNQ1_BDNY_V1, (u8)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCNQ_PGBNDY_V1));
	} else {
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BCNQ1_BDNY_V1, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCNQ1_PGBNDY_V1));
	}

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RXFF_BNDY, pHalmac_adapter->hw_config_info.rx_fifo_size - HALMAC_C2H_PKT_BUF_8822B - 1);

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_USB) {
		value8 = (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_AUTO_LLT_V1) & ~(BIT_MASK_BLK_DESC_NUM << BIT_SHIFT_BLK_DESC_NUM));
		value8 = (u8)(value8 | (HALMAC_BLK_DESC_NUM_8822B << BIT_SHIFT_BLK_DESC_NUM));
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_AUTO_LLT_V1, value8);

		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_AUTO_LLT_V1 + 3, HALMAC_BLK_DESC_NUM_8822B);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_OFFSET_CHK + 1, HALMAC_REG_READ_8(pHalmac_adapter, REG_TXDMA_OFFSET_CHK + 1) | BIT(1));
	}

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_AUTO_LLT_V1, (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_AUTO_LLT_V1) | BIT_AUTO_INIT_LLT_V1));
	counter = 1000;
	while (HALMAC_REG_READ_8(pHalmac_adapter, REG_AUTO_LLT_V1) & BIT_AUTO_INIT_LLT_V1) {
		counter--;
		if (counter == 0)
			return HALMAC_RET_INIT_LLT_FAIL;
	}

	if (halmac_trx_mode == HALMAC_TRX_MODE_DELAY_LOOPBACK) {
		transfer_mode = HALMAC_TRNSFER_LOOPBACK_DELAY;
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_WMAC_LBK_BUF_HD_V1, (u16)pHalmac_adapter->txff_allocation.rsvd_pg_bndy);
	} else if (halmac_trx_mode == HALMAC_TRX_MODE_LOOPBACK) {
		transfer_mode = HALMAC_TRNSFER_LOOPBACK_DIRECT;
	} else {
		transfer_mode = HALMAC_TRNSFER_NORMAL;
	}

	pHalmac_adapter->hw_config_info.trx_mode = transfer_mode;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 3, (u8)transfer_mode);

	return HALMAC_RET_SUCCESS;
}


/**
 * halmac_init_protocol_cfg_8822b() - config protocol register
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_protocol_cfg_8822b(
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

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_protocol_cfg_8822b ==========>\n");

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_AMPDU_MAX_TIME_V1, HALMAC_AMPDU_MAX_TIME_8822B);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TX_HANG_CTRL, BIT_EN_EOF_V1);

	value32 = HALMAC_PROT_RTS_LEN_TH_8822B | (HALMAC_PROT_RTS_TX_TIME_TH_8822B << 8) |
					(HALMAC_PROT_MAX_AGG_PKT_LIMIT_8822B << 16) | (HALMAC_PROT_RTS_MAX_AGG_PKT_LIMIT_8822B << 24);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_PROT_MODE_CTRL, value32);

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BAR_MODE_CTRL + 2, HALMAC_BAR_RETRY_LIMIT_8822B | HALMAC_RA_TRY_RATE_AGG_LIMIT_8822B << 8);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FAST_EDCA_VOVI_SETTING, HALMAC_FAST_EDCA_VO_TH_8822B);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FAST_EDCA_VOVI_SETTING + 2, HALMAC_FAST_EDCA_VI_TH_8822B);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FAST_EDCA_BEBK_SETTING, HALMAC_FAST_EDCA_BE_TH_8822B);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FAST_EDCA_BEBK_SETTING + 2, HALMAC_FAST_EDCA_BK_TH_8822B);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_protocol_cfg_8822b <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_h2c_8822b() - config h2c packet buffer
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_h2c_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 value8;
	u32 value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	value8 = 0;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);
	value8 = HALMAC_CR_TRX_ENABLE_8822B;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_H2C_HEAD);
	value32 = (value32 & 0xFFFC0000) | (pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy << HALMAC_TX_PAGE_SIZE_2_POWER_8822B);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2C_HEAD, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_H2C_READ_ADDR);
	value32 = (value32 & 0xFFFC0000) | (pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy << HALMAC_TX_PAGE_SIZE_2_POWER_8822B);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2C_READ_ADDR, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_H2C_TAIL);
	value32 = (value32 & 0xFFFC0000) | ((pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy << HALMAC_TX_PAGE_SIZE_2_POWER_8822B) + (HALMAC_RSVD_H2C_QUEUE_PGNUM_8822B << HALMAC_TX_PAGE_SIZE_2_POWER_8822B));
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2C_TAIL, value32);
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_H2C_INFO);
	value8 = (u8)((value8 & 0xFC) | 0x01);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_H2C_INFO, value8);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_H2C_INFO);
	value8 = (u8)((value8 & 0xFB) | 0x04);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_H2C_INFO, value8);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_TXDMA_OFFSET_CHK + 1);
	value8 = (u8)((value8 & 0x7f) | 0x80);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_OFFSET_CHK + 1, value8);

	pHalmac_adapter->h2c_buff_size = HALMAC_RSVD_H2C_QUEUE_PGNUM_8822B << HALMAC_TX_PAGE_SIZE_2_POWER_8822B;
	halmac_get_h2c_buff_free_space_88xx(pHalmac_adapter);

	if (pHalmac_adapter->h2c_buff_size != pHalmac_adapter->h2c_buf_free_space) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]get h2c free space error!\n");
		return HALMAC_RET_GET_H2C_SPACE_ERR;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]h2c free space : %d\n", pHalmac_adapter->h2c_buf_free_space);

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_8822B_SUPPORT */

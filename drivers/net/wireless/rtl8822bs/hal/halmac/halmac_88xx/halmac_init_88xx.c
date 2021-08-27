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

#include "halmac_init_88xx.h"
#include "halmac_88xx_cfg.h"
#include "halmac_fw_88xx.h"
#include "halmac_common_88xx.h"
#include "halmac_cfg_wmac_88xx.h"
#include "halmac_efuse_88xx.h"
#include "halmac_mimo_88xx.h"
#include "halmac_bb_rf_88xx.h"
#include "halmac_sdio_88xx.h"
#include "halmac_usb_88xx.h"
#include "halmac_pcie_88xx.h"
#include "halmac_gpio_88xx.h"

#if HALMAC_8822B_SUPPORT
#include "halmac_8822b/halmac_init_8822b.h"
#endif

#if HALMAC_8821C_SUPPORT
#include "halmac_8821c/halmac_init_8821c.h"
#endif

#if HALMAC_8822C_SUPPORT
#include "halmac_8822c/halmac_init_8822c.h"
#endif

#if HALMAC_PLATFORM_TESTPROGRAM
#include "halmisc_api_88xx.h"
#endif

#if HALMAC_88XX_SUPPORT

static VOID
halmac_init_state_machine_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_verify_io_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);


static HALMAC_RET_STATUS
halmac_verify_send_rsvd_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

VOID
halmac_init_adapter_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	pHalmac_adapter->api_registry.rx_expand_mode_en = 1;
	pHalmac_adapter->api_registry.la_mode_en = 1;
	pHalmac_adapter->api_registry.cfg_drv_rsvd_pg_en = 1;
	pHalmac_adapter->api_registry.sdio_cmd53_4byte_en = 1;

	pHalmac_adapter->pHalAdapter_backup = pHalmac_adapter;
	pHalmac_adapter->pHalEfuse_map = (u8 *)NULL;
	pHalmac_adapter->hal_efuse_map_valid = _FALSE;
	pHalmac_adapter->efuse_end = 0;
	pHalmac_adapter->pHal_mac_addr[0].Address_L_H.Address_Low = 0;
	pHalmac_adapter->pHal_mac_addr[0].Address_L_H.Address_High = 0;
	pHalmac_adapter->pHal_mac_addr[1].Address_L_H.Address_Low = 0;
	pHalmac_adapter->pHal_mac_addr[1].Address_L_H.Address_High = 0;
	pHalmac_adapter->pHal_bss_addr[0].Address_L_H.Address_Low = 0;
	pHalmac_adapter->pHal_bss_addr[0].Address_L_H.Address_High = 0;
	pHalmac_adapter->pHal_bss_addr[1].Address_L_H.Address_Low = 0;
	pHalmac_adapter->pHal_bss_addr[1].Address_L_H.Address_High = 0;

	pHalmac_adapter->low_clk = _FALSE;
	pHalmac_adapter->max_download_size = HALMAC_FW_MAX_DL_SIZE_88XX;
	pHalmac_adapter->ofld_func_info.halmac_malloc_max_sz = HALMAC_OFLD_FUNC_MALLOC_MAX_SIZE_88XX;
	pHalmac_adapter->ofld_func_info.rsvd_pg_drv_buf_max_sz = HALMAC_OFLD_FUNC_RSVD_PG_DRV_BUF_MAX_SIZE_88XX;

	pHalmac_adapter->config_para_info.pCfg_para_buf = NULL;
	pHalmac_adapter->config_para_info.pPara_buf_w = NULL;
	pHalmac_adapter->config_para_info.para_num = 0;
	pHalmac_adapter->config_para_info.full_fifo_mode = _FALSE;
	pHalmac_adapter->config_para_info.para_buf_size = 0;
	pHalmac_adapter->config_para_info.avai_para_buf_size = 0;
	pHalmac_adapter->config_para_info.offset_accumulation = 0;
	pHalmac_adapter->config_para_info.value_accumulation = 0;
	pHalmac_adapter->config_para_info.datapack_segment = 0;

	pHalmac_adapter->ch_sw_info.ch_info_buf = NULL;
	pHalmac_adapter->ch_sw_info.ch_info_buf_w = NULL;
	pHalmac_adapter->ch_sw_info.extra_info_en = 0;
	pHalmac_adapter->ch_sw_info.buf_size = 0;
	pHalmac_adapter->ch_sw_info.avai_buf_size = 0;
	pHalmac_adapter->ch_sw_info.total_size = 0;
	pHalmac_adapter->ch_sw_info.ch_num = 0;

	pHalmac_adapter->drv_info_size = 0;
	pHalmac_adapter->tx_desc_transfer = _FALSE;

	pHalmac_adapter->txff_allocation.tx_fifo_pg_num = 0;
	pHalmac_adapter->txff_allocation.ac_q_pg_num = 0;
	pHalmac_adapter->txff_allocation.rsvd_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_drv_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_cpu_instr_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_fw_txbuff_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.pub_queue_pg_num = 0;
	pHalmac_adapter->txff_allocation.high_queue_pg_num = 0;
	pHalmac_adapter->txff_allocation.low_queue_pg_num = 0;
	pHalmac_adapter->txff_allocation.normal_queue_pg_num = 0;
	pHalmac_adapter->txff_allocation.extra_queue_pg_num = 0;

	pHalmac_adapter->txff_allocation.la_mode = HALMAC_LA_MODE_DISABLE;
	pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode = HALMAC_RX_FIFO_EXPANDING_MODE_DISABLE;

	pHalmac_adapter->pwr_off_flow_flag = 0;

	pHalmac_adapter->hw_config_info.security_check_keyid = 0;
	pHalmac_adapter->hw_config_info.ac_queue_num = 8;

	pHalmac_adapter->sdio_cmd53_4byte = HALMAC_SDIO_CMD53_4BYTE_MODE_DISABLE;
	pHalmac_adapter->sdio_hw_info.io_hi_speed_flag = 0;
	pHalmac_adapter->sdio_hw_info.io_indir_flag = 0;
	pHalmac_adapter->sdio_hw_info.spec_ver = HALMAC_SDIO_SPEC_VER_2_00;
	pHalmac_adapter->sdio_hw_info.clock_speed = 50;

	pHalmac_adapter->pinmux_info.wl_led = 0;
	pHalmac_adapter->pinmux_info.sdio_int = 0;
	pHalmac_adapter->pinmux_info.sw_io_0 = 0;
	pHalmac_adapter->pinmux_info.sw_io_1 = 0;
	pHalmac_adapter->pinmux_info.sw_io_2 = 0;
	pHalmac_adapter->pinmux_info.sw_io_3 = 0;
	pHalmac_adapter->pinmux_info.sw_io_4 = 0;
	pHalmac_adapter->pinmux_info.sw_io_5 = 0;
	pHalmac_adapter->pinmux_info.sw_io_6 = 0;
	pHalmac_adapter->pinmux_info.sw_io_7 = 0;
	pHalmac_adapter->pinmux_info.sw_io_8 = 0;
	pHalmac_adapter->pinmux_info.sw_io_9 = 0;
	pHalmac_adapter->pinmux_info.sw_io_10 = 0;
	pHalmac_adapter->pinmux_info.sw_io_11 = 0;
	pHalmac_adapter->pinmux_info.sw_io_12 = 0;
	pHalmac_adapter->pinmux_info.sw_io_13 = 0;
	pHalmac_adapter->pinmux_info.sw_io_14 = 0;
	pHalmac_adapter->pinmux_info.sw_io_15 = 0;

	pHalmac_adapter->sdio_free_space.pMacid_map = (u8 *)NULL;
	pHalmac_adapter->sdio_free_space.macid_map_size = HALMAC_MACID_MAX_88XX << 1;

	if (HALMAC_INTERFACE_SDIO == pHalmac_adapter->halmac_interface) {
		if (NULL == pHalmac_adapter->sdio_free_space.pMacid_map) {
			pHalmac_adapter->sdio_free_space.pMacid_map = (u8 *)PLATFORM_RTL_MALLOC(pHalmac_adapter->pDriver_adapter, pHalmac_adapter->sdio_free_space.macid_map_size);
			if (NULL == pHalmac_adapter->sdio_free_space.pMacid_map)
				PLATFORM_MSG_PRINT(pHalmac_adapter->pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]halmac allocate Macid_map Fail!!\n");
		}
	}

	halmac_init_adapter_dynamic_para_88xx(pHalmac_adapter);
	halmac_init_state_machine_88xx(pHalmac_adapter);
}

VOID
halmac_init_adapter_dynamic_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	pHalmac_adapter->h2c_packet_seq = 0;
	pHalmac_adapter->h2c_buf_free_space = 0;
}

HALMAC_RET_STATUS
halmac_mount_api_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	PHALMAC_API pHalmac_api = (PHALMAC_API)NULL;

	pHalmac_adapter->pHalmac_api = (PHALMAC_API)PLATFORM_RTL_MALLOC(pDriver_adapter, sizeof(HALMAC_API));
	if (pHalmac_adapter->pHalmac_api == NULL)
		return HALMAC_RET_MALLOC_FAIL;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, HALMAC_SVN_VER_88XX"\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_MAJOR_VER_88XX = %x\n", HALMAC_MAJOR_VER_88XX);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_PROTOTYPE_88XX = %x\n", HALMAC_PROTOTYPE_VER_88XX);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_MINOR_VER_88XX = %x\n", HALMAC_MINOR_VER_88XX);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_PATCH_VER_88XX = %x\n", HALMAC_PATCH_VER_88XX);

	/* Mount function pointer */
	pHalmac_api->halmac_register_api = halmac_register_api_88xx;
	pHalmac_api->halmac_download_firmware = halmac_download_firmware_88xx;
	pHalmac_api->halmac_free_download_firmware = halmac_free_download_firmware_88xx;
	pHalmac_api->halmac_get_fw_version = halmac_get_fw_version_88xx;
	pHalmac_api->halmac_cfg_mac_addr = halmac_cfg_mac_addr_88xx;
	pHalmac_api->halmac_cfg_bssid = halmac_cfg_bssid_88xx;
	pHalmac_api->halmac_cfg_transmitter_addr = halmac_cfg_transmitter_addr_88xx;
	pHalmac_api->halmac_cfg_net_type = halmac_cfg_net_type_88xx;
	pHalmac_api->halmac_cfg_tsf_rst = halmac_cfg_tsf_rst_88xx;
	pHalmac_api->halmac_cfg_bcn_space = halmac_cfg_bcn_space_88xx;
	pHalmac_api->halmac_rw_bcn_ctrl = halmac_rw_bcn_ctrl_88xx;
	pHalmac_api->halmac_cfg_multicast_addr = halmac_cfg_multicast_addr_88xx;
	pHalmac_api->halmac_pre_init_system_cfg = halmac_pre_init_system_cfg_88xx;
	pHalmac_api->halmac_init_system_cfg = halmac_init_system_cfg_88xx;
	pHalmac_api->halmac_init_edca_cfg = halmac_init_edca_cfg_88xx;
	pHalmac_api->halmac_cfg_operation_mode = halmac_cfg_operation_mode_88xx;
	pHalmac_api->halmac_cfg_ch_bw = halmac_cfg_ch_bw_88xx;
	pHalmac_api->halmac_cfg_bw = halmac_cfg_bw_88xx;
	pHalmac_api->halmac_init_wmac_cfg = halmac_init_wmac_cfg_88xx;
	pHalmac_api->halmac_init_mac_cfg = halmac_init_mac_cfg_88xx;
	pHalmac_api->halmac_dump_efuse_map = halmac_dump_efuse_map_88xx;
	pHalmac_api->halmac_dump_efuse_map_bt = halmac_dump_efuse_map_bt_88xx;
	pHalmac_api->halmac_write_efuse_bt = halmac_write_efuse_bt_88xx;
	pHalmac_api->halmac_read_efuse_bt = halmac_read_efuse_bt_88xx;
	pHalmac_api->halmac_cfg_efuse_auto_check = halmac_cfg_efuse_auto_check_88xx;
	pHalmac_api->halmac_dump_logical_efuse_map = halmac_dump_logical_efuse_map_88xx;
	pHalmac_api->halmac_pg_efuse_by_map = halmac_pg_efuse_by_map_88xx;
	pHalmac_api->halmac_mask_logical_efuse = halmac_mask_logical_efuse_88xx;
	pHalmac_api->halmac_get_efuse_size = halmac_get_efuse_size_88xx;
	pHalmac_api->halmac_get_efuse_available_size = halmac_get_efuse_available_size_88xx;
	pHalmac_api->halmac_get_c2h_info = halmac_get_c2h_info_88xx;

	pHalmac_api->halmac_get_logical_efuse_size = halmac_get_logical_efuse_size_88xx;

	pHalmac_api->halmac_write_logical_efuse = halmac_write_logical_efuse_88xx;
	pHalmac_api->halmac_read_logical_efuse = halmac_read_logical_efuse_88xx;

	pHalmac_api->halmac_ofld_func_cfg = halmac_ofld_func_cfg_88xx;
	pHalmac_api->halmac_h2c_lb = halmac_h2c_lb_88xx;
	pHalmac_api->halmac_debug = halmac_debug_88xx;
	pHalmac_api->halmac_cfg_parameter = halmac_cfg_parameter_88xx;
	pHalmac_api->halmac_update_datapack = halmac_update_datapack_88xx;
	pHalmac_api->halmac_run_datapack = halmac_run_datapack_88xx;
	pHalmac_api->halmac_send_bt_coex = halmac_send_bt_coex_88xx;
	pHalmac_api->halmac_verify_platform_api = halmac_verify_platform_api_88xx;
	pHalmac_api->halmac_update_packet = halmac_update_packet_88xx;
	pHalmac_api->halmac_bcn_ie_filter = halmac_bcn_ie_filter_88xx;
	pHalmac_api->halmac_cfg_txbf = halmac_cfg_txbf_88xx;
	pHalmac_api->halmac_cfg_mumimo = halmac_cfg_mumimo_88xx;
	pHalmac_api->halmac_cfg_sounding = halmac_cfg_sounding_88xx;
	pHalmac_api->halmac_del_sounding = halmac_del_sounding_88xx;
	pHalmac_api->halmac_su_bfer_entry_init = halmac_su_bfer_entry_init_88xx;
	pHalmac_api->halmac_su_bfee_entry_init = halmac_su_bfee_entry_init_88xx;
	pHalmac_api->halmac_mu_bfer_entry_init = halmac_mu_bfer_entry_init_88xx;
	pHalmac_api->halmac_mu_bfee_entry_init = halmac_mu_bfee_entry_init_88xx;
	pHalmac_api->halmac_su_bfer_entry_del = halmac_su_bfer_entry_del_88xx;
	pHalmac_api->halmac_su_bfee_entry_del = halmac_su_bfee_entry_del_88xx;
	pHalmac_api->halmac_mu_bfer_entry_del = halmac_mu_bfer_entry_del_88xx;
	pHalmac_api->halmac_mu_bfee_entry_del = halmac_mu_bfee_entry_del_88xx;

	pHalmac_api->halmac_add_ch_info = halmac_add_ch_info_88xx;
	pHalmac_api->halmac_add_extra_ch_info = halmac_add_extra_ch_info_88xx;
	pHalmac_api->halmac_ctrl_ch_switch = halmac_ctrl_ch_switch_88xx;
	pHalmac_api->halmac_p2pps = halmac_p2pps_88xx;
	pHalmac_api->halmac_clear_ch_info = halmac_clear_ch_info_88xx;
	pHalmac_api->halmac_send_general_info = halmac_send_general_info_88xx;

	pHalmac_api->halmac_start_iqk = halmac_start_iqk_88xx;
	pHalmac_api->halmac_ctrl_pwr_tracking = halmac_ctrl_pwr_tracking_88xx;
	pHalmac_api->halmac_psd = halmac_psd_88xx;
	pHalmac_api->halmac_cfg_la_mode = halmac_cfg_la_mode_88xx;
	pHalmac_api->halmac_cfg_rx_fifo_expanding_mode = halmac_cfg_rx_fifo_expanding_mode_88xx;

	pHalmac_api->halmac_config_security = halmac_config_security_88xx;
	pHalmac_api->halmac_get_used_cam_entry_num = halmac_get_used_cam_entry_num_88xx;
	pHalmac_api->halmac_read_cam_entry = halmac_read_cam_entry_88xx;
	pHalmac_api->halmac_write_cam = halmac_write_cam_88xx;
	pHalmac_api->halmac_clear_cam_entry = halmac_clear_cam_entry_88xx;

	pHalmac_api->halmac_cfg_drv_rsvd_pg_num = halmac_cfg_drv_rsvd_pg_num_88xx;
	pHalmac_api->halmac_get_chip_version = halmac_get_version_88xx;

	pHalmac_api->halmac_query_status = halmac_query_status_88xx;
	pHalmac_api->halmac_reset_feature = halmac_reset_feature_88xx;
	pHalmac_api->halmac_check_fw_status = halmac_check_fw_status_88xx;
	pHalmac_api->halmac_dump_fw_dmem = halmac_dump_fw_dmem_88xx;
	pHalmac_api->halmac_cfg_max_dl_size = halmac_cfg_max_dl_size_88xx;

	pHalmac_api->halmac_dump_fifo = halmac_dump_fifo_88xx;
	pHalmac_api->halmac_get_fifo_size = halmac_get_fifo_size_88xx;

	pHalmac_api->halmac_chk_txdesc = halmac_chk_txdesc_88xx;
	pHalmac_api->halmac_dl_drv_rsvd_page = halmac_dl_drv_rsvd_page_88xx;
	pHalmac_api->halmac_cfg_csi_rate = halmac_cfg_csi_rate_88xx;

	pHalmac_api->halmac_fill_txdesc_checksum = halmac_fill_txdesc_check_sum_88xx;

	pHalmac_api->halmac_sdio_cmd53_4byte = halmac_sdio_cmd53_4byte_88xx;
	pHalmac_api->halmac_sdio_hw_info = halmac_sdio_hw_info_88xx;

	pHalmac_api->halmac_init_sdio_cfg = halmac_init_sdio_cfg_88xx;
	pHalmac_api->halmac_init_usb_cfg = halmac_init_usb_cfg_88xx;
	pHalmac_api->halmac_init_pcie_cfg = halmac_init_pcie_cfg_88xx;
	pHalmac_api->halmac_deinit_sdio_cfg = halmac_deinit_sdio_cfg_88xx;
	pHalmac_api->halmac_deinit_usb_cfg = halmac_deinit_usb_cfg_88xx;
	pHalmac_api->halmac_deinit_pcie_cfg = halmac_deinit_pcie_cfg_88xx;
	pHalmac_api->halmac_txfifo_is_empty = halmac_txfifo_is_empty_88xx;
	pHalmac_api->halmac_download_flash = halmac_download_flash_88xx;
	pHalmac_api->halmac_read_flash = halmac_read_flash_88xx;
	pHalmac_api->halmac_erase_flash = halmac_erase_flash_88xx;
	pHalmac_api->halmac_check_flash = halmac_check_flash_88xx;
	pHalmac_api->halmac_cfg_edca_para = halmac_cfg_edca_para_88xx;
	pHalmac_api->halmac_pinmux_wl_led_mode = halmac_pinmux_wl_led_mode_88xx;
	pHalmac_api->halmac_pinmux_wl_led_sw_ctrl = halmac_pinmux_wl_led_sw_ctrl_88xx;
	pHalmac_api->halmac_pinmux_sdio_int_polarity = halmac_pinmux_sdio_int_polarity_88xx;
	pHalmac_api->halmac_pinmux_gpio_mode = halmac_pinmux_gpio_mode_88xx;
	pHalmac_api->halmac_pinmux_gpio_output = halmac_pinmux_gpio_output_88xx;
	pHalmac_api->halmac_pinmux_pin_status = halmac_pinmux_pin_status_88xx;

	pHalmac_api->halmac_rx_cut_amsdu_cfg = halmac_rx_cut_amsdu_cfg_88xx;
	pHalmac_api->halmac_fw_snding = halmac_fw_snding_88xx;
	pHalmac_api->halmac_get_mac_addr = halmac_get_mac_addr_88xx;

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		pHalmac_api->halmac_cfg_rx_aggregation = halmac_cfg_rx_aggregation_88xx_sdio;
		pHalmac_api->halmac_init_interface_cfg = halmac_init_sdio_cfg_88xx;
		pHalmac_api->halmac_deinit_interface_cfg = halmac_deinit_sdio_cfg_88xx;
		pHalmac_api->halmac_cfg_tx_agg_align = halmac_cfg_tx_agg_align_sdio_88xx;
		pHalmac_api->halmac_set_bulkout_num = halmac_set_bulkout_num_sdio_88xx;
		pHalmac_api->halmac_get_usb_bulkout_id = halmac_get_usb_bulkout_id_sdio_88xx;
		pHalmac_api->halmac_reg_read_indirect_32 = halmac_reg_read_indirect_32_sdio_88xx;
		pHalmac_api->halmac_reg_sdio_cmd53_read_n = halmac_reg_read_nbyte_sdio_88xx;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_USB) {
		pHalmac_api->halmac_cfg_rx_aggregation = halmac_cfg_rx_aggregation_88xx_usb;
		pHalmac_api->halmac_init_interface_cfg = halmac_init_usb_cfg_88xx;
		pHalmac_api->halmac_deinit_interface_cfg = halmac_deinit_usb_cfg_88xx;
		pHalmac_api->halmac_cfg_tx_agg_align = halmac_cfg_tx_agg_align_usb_88xx;
		pHalmac_api->halmac_tx_allowed_sdio = halmac_tx_allowed_usb_88xx;
		pHalmac_api->halmac_set_bulkout_num = halmac_set_bulkout_num_usb_88xx;
		pHalmac_api->halmac_get_sdio_tx_addr = halmac_get_sdio_tx_addr_usb_88xx;
		pHalmac_api->halmac_get_usb_bulkout_id = halmac_get_usb_bulkout_id_usb_88xx;
		pHalmac_api->halmac_reg_read_8 = halmac_reg_read_8_usb_88xx;
		pHalmac_api->halmac_reg_write_8 = halmac_reg_write_8_usb_88xx;
		pHalmac_api->halmac_reg_read_16 = halmac_reg_read_16_usb_88xx;
		pHalmac_api->halmac_reg_write_16 = halmac_reg_write_16_usb_88xx;
		pHalmac_api->halmac_reg_read_32 = halmac_reg_read_32_usb_88xx;
		pHalmac_api->halmac_reg_write_32 = halmac_reg_write_32_usb_88xx;
		pHalmac_api->halmac_reg_read_indirect_32 = halmac_reg_read_indirect_32_usb_88xx;
		pHalmac_api->halmac_reg_sdio_cmd53_read_n = halmac_reg_read_nbyte_usb_88xx;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_PCIE) {
		pHalmac_api->halmac_cfg_rx_aggregation = halmac_cfg_rx_aggregation_88xx_pcie;
		pHalmac_api->halmac_init_interface_cfg = halmac_init_pcie_cfg_88xx;
		pHalmac_api->halmac_deinit_interface_cfg = halmac_deinit_pcie_cfg_88xx;
		pHalmac_api->halmac_cfg_tx_agg_align = halmac_cfg_tx_agg_align_pcie_88xx;
		pHalmac_api->halmac_tx_allowed_sdio = halmac_tx_allowed_pcie_88xx;
		pHalmac_api->halmac_set_bulkout_num = halmac_set_bulkout_num_pcie_88xx;
		pHalmac_api->halmac_get_sdio_tx_addr = halmac_get_sdio_tx_addr_pcie_88xx;
		pHalmac_api->halmac_get_usb_bulkout_id = halmac_get_usb_bulkout_id_pcie_88xx;
		pHalmac_api->halmac_reg_read_8 = halmac_reg_read_8_pcie_88xx;
		pHalmac_api->halmac_reg_write_8 = halmac_reg_write_8_pcie_88xx;
		pHalmac_api->halmac_reg_read_16 = halmac_reg_read_16_pcie_88xx;
		pHalmac_api->halmac_reg_write_16 = halmac_reg_write_16_pcie_88xx;
		pHalmac_api->halmac_reg_read_32 = halmac_reg_read_32_pcie_88xx;
		pHalmac_api->halmac_reg_write_32 = halmac_reg_write_32_pcie_88xx;
		pHalmac_api->halmac_reg_read_indirect_32 = halmac_reg_read_indirect_32_pcie_88xx;
		pHalmac_api->halmac_reg_sdio_cmd53_read_n = halmac_reg_read_nbyte_pcie_88xx;
	} else {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Set halmac io function Error!!\n");
	}

	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822B) {
#if HALMAC_8822B_SUPPORT
		halmac_mount_api_8822b(pHalmac_adapter);
#endif
	} else if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8821C) {
#if HALMAC_8821C_SUPPORT
		halmac_mount_api_8821c(pHalmac_adapter);
#endif
	} else if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822C) {
#if HALMAC_8822C_SUPPORT
		halmac_mount_api_8822c(pHalmac_adapter);
#endif
	} else {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Chip ID undefine!!\n");
		return HALMAC_RET_CHIP_NOT_SUPPORT;
	}

#if HALMAC_PLATFORM_TESTPROGRAM
	halmac_mount_misc_api_88xx(pHalmac_adapter);
#endif

	return HALMAC_RET_SUCCESS;
}

static VOID
halmac_init_state_machine_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	PHALMAC_STATE pState = &pHalmac_adapter->halmac_state;

	halmac_init_offload_feature_state_machine_88xx(pHalmac_adapter);

	pState->api_state = HALMAC_API_STATE_INIT;

	pState->dlfw_state = HALMAC_DLFW_NONE;
	pState->mac_power = HALMAC_MAC_POWER_OFF;
	pState->ps_state = HALMAC_PS_STATE_UNDEFINE;
	pState->gpio_cfg_state = HALMAC_GPIO_CFG_STATE_IDLE;
	pState->rsvd_pg_state = HALMAC_RSVD_PG_STATE_IDLE;
}

VOID
halmac_init_offload_feature_state_machine_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	PHALMAC_STATE pState = &pHalmac_adapter->halmac_state;

	pState->efuse_state_set.efuse_cmd_construct_state = HALMAC_EFUSE_CMD_CONSTRUCT_IDLE;
	pState->efuse_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
	pState->efuse_state_set.seq_num = pHalmac_adapter->h2c_packet_seq;

	pState->cfg_para_state_set.cfg_para_cmd_construct_state = HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE;
	pState->cfg_para_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
	pState->cfg_para_state_set.seq_num = pHalmac_adapter->h2c_packet_seq;

	pState->scan_state_set.scan_cmd_construct_state = HALMAC_SCAN_CMD_CONSTRUCT_IDLE;
	pState->scan_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
	pState->scan_state_set.seq_num = pHalmac_adapter->h2c_packet_seq;

	pState->update_packet_set.process_status = HALMAC_CMD_PROCESS_IDLE;
	pState->update_packet_set.seq_num = pHalmac_adapter->h2c_packet_seq;

	pState->iqk_set.process_status = HALMAC_CMD_PROCESS_IDLE;
	pState->iqk_set.seq_num = pHalmac_adapter->h2c_packet_seq;

	pState->power_tracking_set.process_status = HALMAC_CMD_PROCESS_IDLE;
	pState->power_tracking_set.seq_num = pHalmac_adapter->h2c_packet_seq;

	pState->psd_set.process_status = HALMAC_CMD_PROCESS_IDLE;
	pState->psd_set.seq_num = pHalmac_adapter->h2c_packet_seq;
	pState->psd_set.data_size = 0;
	pState->psd_set.segment_size = 0;
	pState->psd_set.pData = NULL;

	pState->fw_snding_set.fw_snding_cmd_construct_state = HALMAC_FW_SNDING_CMD_CONSTRUCT_IDLE;
	pState->fw_snding_set.process_status = HALMAC_CMD_PROCESS_IDLE;
	pState->fw_snding_set.seq_num = pHalmac_adapter->h2c_packet_seq;
}

/**
 * halmac_register_api_88xx() - register feature list
 * @pHalmac_adapter
 * @pApi_registry : feature list, 1->enable 0->disable
 * Author : Ivan Lin
 *
 * Default is enable all api registry
 *
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_register_api_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_API_REGISTRY pApi_registry
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (pApi_registry == NULL)
		return HALMAC_RET_NULL_POINTER;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_register_api_88xx ==========>\n");

	PLATFORM_RTL_MEMCPY(pDriver_adapter, &pHalmac_adapter->api_registry, pApi_registry, sizeof(*pApi_registry));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]rx_expand : %d\n", pHalmac_adapter->api_registry.rx_expand_mode_en);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]la_mode : %d\n", pHalmac_adapter->api_registry.la_mode_en);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]cfg_drv_rsvd_pg : %d\n", pHalmac_adapter->api_registry.cfg_drv_rsvd_pg_en);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]sdio_cmd53_4byte : %d\n", pHalmac_adapter->api_registry.sdio_cmd53_4byte_en);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_register_api_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pre_init_system_cfg_88xx() - pre-init system config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pre_init_system_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u32 value32, counter;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u8 enable_bb;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_pre_init_system_cfg ==========>\n");

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RSV_CTRL, 0);

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SDIO_HSUS_CTRL, HALMAC_REG_READ_8(pHalmac_adapter, REG_SDIO_HSUS_CTRL) & ~(BIT(0)));
		counter = 10000;
		while (!(HALMAC_REG_READ_8(pHalmac_adapter, REG_SDIO_HSUS_CTRL) & 0x02)) {
			counter--;
			if (counter == 0)
				return HALMAC_RET_SDIO_LEAVE_SUSPEND_FAIL;
		}

		if (pHalmac_adapter->sdio_hw_info.spec_ver == HALMAC_SDIO_SPEC_VER_3_00)
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 2) | BIT(2));
		else
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 2) & ~(BIT(2)));
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_USB) {
		if (HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_CFG2 + 3) == 0x20)	 /* usb3.0 */
			HALMAC_REG_WRITE_8(pHalmac_adapter, 0xFE5B, HALMAC_REG_READ_8(pHalmac_adapter, 0xFE5B) | BIT(4));
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_PCIE) {
		/* For PCIE power on fail issue */
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 1, HALMAC_REG_READ_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 1) | BIT(0));
	}

	/* Config PIN Mux */
	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_PAD_CTRL1);
	value32 = value32 & (~(BIT(28) | BIT(29)));
	value32 = value32 | BIT(28) | BIT(29);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_PAD_CTRL1, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_LED_CFG);
	value32 = value32 & (~(BIT(25) | BIT(26)));
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_LED_CFG, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_GPIO_MUXCFG);
	value32 = value32 & (~(BIT(2)));
	value32 = value32 | BIT(2);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_GPIO_MUXCFG, value32);

	enable_bb = _FALSE;
	halmac_set_hw_value_88xx(pHalmac_adapter, HALMAC_HW_EN_BB_RF, &enable_bb);


	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_pre_init_system_cfg <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_system_cfg_88xx() -  init system config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_system_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u32 temp = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_system_cfg ==========>\n");

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1, HALMAC_FUNCTION_ENABLE_88XX);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SYS_SDIO_CTRL, (u32)(HALMAC_REG_READ_32(pHalmac_adapter, REG_SYS_SDIO_CTRL) | BIT_LTE_MUX_CTRL_PATH));
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CPU_DMEM_CON, (u32)(HALMAC_REG_READ_32(pHalmac_adapter, REG_CPU_DMEM_CON) | BIT_WL_PLATFORM_RST));

	/*disable boot-from-flash for driver's DL FW*/
	temp = HALMAC_REG_READ_32(pHalmac_adapter, REG_MCUFW_CTRL);
	if (temp & BIT_BOOT_FSPI_EN) {
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MCUFW_CTRL, temp & (~BIT_BOOT_FSPI_EN));
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_GPIO_MUXCFG, HALMAC_REG_READ_32(pHalmac_adapter, REG_GPIO_MUXCFG) & (~BIT_FSPI_EN));
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_system_cfg <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_edca_cfg_88xx() - init EDCA config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_edca_cfg_88xx(
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

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_edca_cfg_88xx ==========>\n");

	/* Clear TX pause */
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXPAUSE, 0x0000);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SLOT, HALMAC_SLOT_TIME_88XX);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_PIFS, HALMAC_PIFS_TIME_88XX);
	value32 = HALMAC_SIFS_CCK_CTX_88XX | (HALMAC_SIFS_OFDM_CTX_88XX << BIT_SHIFT_SIFS_OFDM_CTX) |
		  (HALMAC_SIFS_CCK_TRX_88XX << BIT_SHIFT_SIFS_CCK_TRX) | (HALMAC_SIFS_OFDM_TRX_88XX << BIT_SHIFT_SIFS_OFDM_TRX);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SIFS, value32);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_EDCA_VO_PARAM, HALMAC_REG_READ_32(pHalmac_adapter, REG_EDCA_VO_PARAM) & 0xFFFF);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_EDCA_VO_PARAM + 2, HALMAC_VO_TXOP_LIMIT_88XX);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_EDCA_VI_PARAM + 2, HALMAC_VI_TXOP_LIMIT_88XX);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RD_NAV_NXT, HALMAC_RDG_NAV_88XX | (HALMAC_TXOP_NAV_88XX << 16));
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RXTSF_OFFSET_CCK, HALMAC_CCK_RX_TSF_88XX | (HALMAC_OFDM_RX_TSF_88XX) << 8);

	/* Set beacon cotnrol - enable TSF and other related functions */
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL) | BIT_EN_BCN_FUNCTION));

	/* Set send beacon related registers */
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TBTT_PROHIBIT, HALMAC_TBTT_PROHIBIT_88XX | (HALMAC_TBTT_HOLD_TIME_88XX << BIT_SHIFT_TBTT_HOLD_TIME_AP));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_DRVERLYINT, HALMAC_DRIVER_EARLY_INT_88XX);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCNDMATIM, HALMAC_BEACON_DMA_TIM_88XX);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_edca_cfg_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_wmac_cfg_88xx() - init wmac config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_wmac_cfg_88xx(
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

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_wmac_cfg_88xx ==========>\n");

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RXFLTMAP0, HALMAC_RX_FILTER0_88XX);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RXFLTMAP2, HALMAC_RX_FILTER_88XX);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RCR, HALMAC_RCR_CONFIG_88XX);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RX_PKT_LIMIT, HALMAC_RXPKT_MAX_SIZE_BASE512);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TCR + 2, 0x30);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TCR + 1, 0x30);

#if HALMAC_8821C_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8821C)
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_ACKTO_CCK, HALMAC_ACK_TO_CCK_88XX);
#endif
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WMAC_OPTION_FUNCTION + 8, 0x30810041);

	value32 = (pHalmac_adapter->hw_config_info.trx_mode == HALMAC_TRNSFER_NORMAL) ? 0x50802098 : 0x50802080;
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WMAC_OPTION_FUNCTION + 4, value32);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_wmac_cfg_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_mac_cfg_88xx() - config page1~page7 register
 * @pHalmac_adapter : the adapter of halmac
 * @mode : trx mode
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_mac_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE mode
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_mac_cfg_88xx ==========>mode = %d\n", mode);

	status = pHalmac_api->halmac_init_trx_cfg(pHalmac_adapter, mode);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_init_trx_cfg errorr = %x\n", status);
		return status;
	}

	status = pHalmac_api->halmac_init_protocol_cfg(pHalmac_adapter);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_init_protocol_cfg_88xx error = %x\n", status);
		return status;
	}

	status = halmac_init_edca_cfg_88xx(pHalmac_adapter);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_init_edca_cfg_88xx error = %x\n", status);
		return status;
	}

	status = halmac_init_wmac_cfg_88xx(pHalmac_adapter);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_init_wmac_cfg_88xx error = %x\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_init_mac_cfg_88xx <==========\n");

	return status;
}

/**
 * halmac_reset_feature_88xx() -reset async api cmd status
 * @pHalmac_adapter : the adapter of halmac
 * @feature_id : feature_id
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS.
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_reset_feature_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_FEATURE_ID feature_id
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_STATE pState = &pHalmac_adapter->halmac_state;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_reset_feature_88xx ==========>\n");

	switch (feature_id) {
	case HALMAC_FEATURE_CFG_PARA:
		pState->cfg_para_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->cfg_para_state_set.cfg_para_cmd_construct_state = HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE;
		break;
	case HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE:
	case HALMAC_FEATURE_DUMP_LOGICAL_EFUSE:
		pState->efuse_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->efuse_state_set.efuse_cmd_construct_state = HALMAC_EFUSE_CMD_CONSTRUCT_IDLE;
		break;
	case HALMAC_FEATURE_CHANNEL_SWITCH:
		pState->scan_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->scan_state_set.scan_cmd_construct_state = HALMAC_SCAN_CMD_CONSTRUCT_IDLE;
		break;
	case HALMAC_FEATURE_UPDATE_PACKET:
		pState->update_packet_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		break;
	case HALMAC_FEATURE_IQK:
		pState->iqk_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		break;
	case HALMAC_FEATURE_POWER_TRACKING:
		pState->power_tracking_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		break;
	case HALMAC_FEATURE_PSD:
		pState->psd_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		break;
	case HALMAC_FEATURE_FW_SNDING:
		pState->fw_snding_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->fw_snding_set.fw_snding_cmd_construct_state = HALMAC_FW_SNDING_CMD_CONSTRUCT_IDLE;
		break;
	case HALMAC_FEATURE_ALL:
		pState->cfg_para_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->cfg_para_state_set.cfg_para_cmd_construct_state = HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE;
		pState->efuse_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->efuse_state_set.efuse_cmd_construct_state = HALMAC_EFUSE_CMD_CONSTRUCT_IDLE;
		pState->scan_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->scan_state_set.scan_cmd_construct_state = HALMAC_SCAN_CMD_CONSTRUCT_IDLE;
		pState->update_packet_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->iqk_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->power_tracking_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->psd_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->fw_snding_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->fw_snding_set.fw_snding_cmd_construct_state = HALMAC_FW_SNDING_CMD_CONSTRUCT_IDLE;
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_reset_feature_88xx invalid feature id %d\n", feature_id);
		return HALMAC_RET_INVALID_FEATURE_ID;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_reset_feature_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * (debug API)halmac_verify_platform_api_88xx() - verify platform api
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_verify_platform_api_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_verify_platform_api_88xx ==========>\n");

	ret_status = halmac_verify_io_88xx(pHalmac_adapter);

	if (ret_status != HALMAC_RET_SUCCESS)
		return ret_status;

	if (pHalmac_adapter->txff_allocation.la_mode != HALMAC_LA_MODE_FULL)
		ret_status = halmac_verify_send_rsvd_page_88xx(pHalmac_adapter);

	if (ret_status != HALMAC_RET_SUCCESS)
		return ret_status;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_verify_platform_api_88xx <==========\n");

	return ret_status;
}

VOID
halmac_tx_desc_checksum_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_tx_desc_checksum_88xx ==========>halmac_tx_desc_checksum_en = %d\n", enable);

	pHalmac_adapter->tx_desc_checksum = enable;
	if (enable == _TRUE)
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXDMA_OFFSET_CHK, (u16)(HALMAC_REG_READ_16(pHalmac_adapter, REG_TXDMA_OFFSET_CHK) | BIT_SDIO_TXDESC_CHKSUM_EN));
	else
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXDMA_OFFSET_CHK, (u16)(HALMAC_REG_READ_16(pHalmac_adapter, REG_TXDMA_OFFSET_CHK) & ~BIT_SDIO_TXDESC_CHKSUM_EN));
}

static HALMAC_RET_STATUS
halmac_verify_io_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 value8, wvalue8;
	u32 value32, value32_2, wvalue32;
	u32 halmac_offset;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		halmac_offset = REG_PAGE5_DUMMY;
		if (0 == (halmac_offset & 0xFFFF0000))
			halmac_offset |= WLAN_IOREG_OFFSET;

		ret_status = halmac_convert_to_sdio_bus_offset_88xx(pHalmac_adapter, &halmac_offset);

		/* Verify CMD52 R/W */
		wvalue8 = 0xab;
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset, wvalue8);

		value8 = PLATFORM_SDIO_CMD52_READ(pDriver_adapter, halmac_offset);

		if (value8 != wvalue8) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]cmd52 r/w fail write = %X read = %X\n", wvalue8, value8);
			ret_status = HALMAC_RET_PLATFORM_API_INCORRECT;
		} else {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]cmd52 r/w ok\n");
		}

		/* Verify CMD53 R/W */
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset, 0xaa);
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset + 1, 0xbb);
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset + 2, 0xcc);
		PLATFORM_SDIO_CMD52_WRITE(pDriver_adapter, halmac_offset + 3, 0xdd);

		value32 = PLATFORM_SDIO_CMD53_READ_32(pDriver_adapter, halmac_offset);

		if (value32 != 0xddccbbaa) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]cmd53 r fail : read = %X\n");
			ret_status = HALMAC_RET_PLATFORM_API_INCORRECT;
		} else {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]cmd53 r ok\n");
		}

		wvalue32 = 0x11223344;
		PLATFORM_SDIO_CMD53_WRITE_32(pDriver_adapter, halmac_offset, wvalue32);

		value32 = PLATFORM_SDIO_CMD53_READ_32(pDriver_adapter, halmac_offset);

		if (value32 != wvalue32) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]cmd53 w fail\n");
			ret_status = HALMAC_RET_PLATFORM_API_INCORRECT;
		} else {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]cmd53 w ok\n");
		}

		value32 = PLATFORM_SDIO_CMD53_READ_32(pDriver_adapter, halmac_offset + 2); /* value32 should be 0x33441122 */

		wvalue32 = 0x11225566;
		PLATFORM_SDIO_CMD53_WRITE_32(pDriver_adapter, halmac_offset, wvalue32);

		value32_2 = PLATFORM_SDIO_CMD53_READ_32(pDriver_adapter, halmac_offset + 2); /* value32 should be 0x55661122 */
		if (value32_2 == value32) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]cmd52 is used for HAL_SDIO_CMD53_READ_32\n");
			ret_status = HALMAC_RET_PLATFORM_API_INCORRECT;
		} else {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]cmd53 is correctly used\n");
		}
	} else {
		wvalue32 = 0x77665511;
		PLATFORM_REG_WRITE_32(pDriver_adapter, REG_PAGE5_DUMMY, wvalue32);

		value32 = PLATFORM_REG_READ_32(pDriver_adapter, REG_PAGE5_DUMMY);
		if (value32 != wvalue32) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]reg rw\n");
			ret_status = HALMAC_RET_PLATFORM_API_INCORRECT;
		} else {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]reg rw ok\n");
		}
	}

	return ret_status;
}

static HALMAC_RET_STATUS
halmac_verify_send_rsvd_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 *rsvd_buf = NULL;
	u8 *rsvd_page = NULL;
	u32 i;
	u32 h2c_pkt_verify_size = 64, h2c_pkt_verify_payload = 0xab;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	rsvd_buf = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, h2c_pkt_verify_size);

	if (rsvd_buf == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]rsvd buffer malloc fail!!\n");
		return HALMAC_RET_MALLOC_FAIL;
	}

	PLATFORM_RTL_MEMSET(pDriver_adapter, rsvd_buf, (u8)h2c_pkt_verify_payload, h2c_pkt_verify_size);

	ret_status = halmac_download_rsvd_page_88xx(pHalmac_adapter, pHalmac_adapter->txff_allocation.rsvd_pg_bndy,
														rsvd_buf, h2c_pkt_verify_size);
	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_RTL_FREE(pDriver_adapter, rsvd_buf, h2c_pkt_verify_size);
		return ret_status;
	}

	rsvd_page = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, h2c_pkt_verify_size + pHalmac_adapter->hw_config_info.txdesc_size);

	if (rsvd_page == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]rsvd page malloc fail!!\n");
		PLATFORM_RTL_FREE(pDriver_adapter, rsvd_buf, h2c_pkt_verify_size);
		return HALMAC_RET_MALLOC_FAIL;
	}

	PLATFORM_RTL_MEMSET(pDriver_adapter, rsvd_page, 0x00, h2c_pkt_verify_size + pHalmac_adapter->hw_config_info.txdesc_size);

	ret_status = halmac_dump_fifo_88xx(pHalmac_adapter, HAL_FIFO_SEL_RSVD_PAGE, 0, h2c_pkt_verify_size + pHalmac_adapter->hw_config_info.txdesc_size, rsvd_page);

	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_RTL_FREE(pDriver_adapter, rsvd_buf, h2c_pkt_verify_size);
		PLATFORM_RTL_FREE(pDriver_adapter, rsvd_page, h2c_pkt_verify_size + pHalmac_adapter->hw_config_info.txdesc_size);
		return ret_status;
	}

	for (i = 0; i < h2c_pkt_verify_size; i++) {
		if (*(rsvd_buf + i) != *(rsvd_page + (i + pHalmac_adapter->hw_config_info.txdesc_size))) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]Compare RSVD page Fail\n");
			ret_status = HALMAC_RET_PLATFORM_API_INCORRECT;
		}
	}

	PLATFORM_RTL_FREE(pDriver_adapter, rsvd_buf, h2c_pkt_verify_size);
	PLATFORM_RTL_FREE(pDriver_adapter, rsvd_page, h2c_pkt_verify_size + pHalmac_adapter->hw_config_info.txdesc_size);

	return ret_status;
}

HALMAC_RET_STATUS
halmac_pg_num_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode,
	IN PHALMAC_PG_NUM pPg_num_table
)
{
	u8 search_flag;
	u16 HPQ_num = 0, LPQ_Nnum = 0, NPQ_num = 0, GAPQ_num = 0;
	u16 EXPQ_num = 0, PUBQ_num = 0;
	u32 i = 0;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	search_flag = 0;
	for (i = 0; i < HALMAC_TRX_MODE_MAX; i++) {
		if (halmac_trx_mode == pPg_num_table[i].mode) {
			HPQ_num = pPg_num_table[i].hq_num;
			LPQ_Nnum = pPg_num_table[i].lq_num;
			NPQ_num = pPg_num_table[i].nq_num;
			EXPQ_num = pPg_num_table[i].exq_num;
			GAPQ_num = pPg_num_table[i].gap_num;
			PUBQ_num = pHalmac_adapter->txff_allocation.ac_q_pg_num - HPQ_num - LPQ_Nnum - NPQ_num - EXPQ_num - GAPQ_num;
			search_flag = 1;
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_pg_num_parser_88xx done\n");
			break;
		}
	}

	if (search_flag == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]HALMAC_RET_TRX_MODE_NOT_SUPPORT 1 switch case not support\n");
		return HALMAC_RET_TRX_MODE_NOT_SUPPORT;
	}

	if (pHalmac_adapter->txff_allocation.ac_q_pg_num < HPQ_num + LPQ_Nnum + NPQ_num + EXPQ_num + GAPQ_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]acqnum = %d\n", pHalmac_adapter->txff_allocation.ac_q_pg_num);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]HPQ_num = %d\n", HPQ_num);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]LPQ_num = %d\n", LPQ_Nnum);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]NPQ_num = %d\n", NPQ_num);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]EPQ_num = %d\n", EXPQ_num);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]GAPQ_num = %d\n", GAPQ_num);
		return HALMAC_RET_CFG_TXFIFO_PAGE_FAIL;
	}

	pHalmac_adapter->txff_allocation.high_queue_pg_num = HPQ_num;
	pHalmac_adapter->txff_allocation.low_queue_pg_num = LPQ_Nnum;
	pHalmac_adapter->txff_allocation.normal_queue_pg_num = NPQ_num;
	pHalmac_adapter->txff_allocation.extra_queue_pg_num = EXPQ_num;
	pHalmac_adapter->txff_allocation.pub_queue_pg_num = PUBQ_num;

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_rqpn_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode,
	IN PHALMAC_RQPN pRqpn_table
)
{
	u8 search_flag;
	u32 i;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	search_flag = 0;
	for (i = 0; i < HALMAC_TRX_MODE_MAX; i++) {
		if (halmac_trx_mode == pRqpn_table[i].mode) {
			pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VO] = pRqpn_table[i].dma_map_vo;
			pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VI] = pRqpn_table[i].dma_map_vi;
			pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BE] = pRqpn_table[i].dma_map_be;
			pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BK] = pRqpn_table[i].dma_map_bk;
			pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_MG] = pRqpn_table[i].dma_map_mg;
			pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI] = pRqpn_table[i].dma_map_hi;
			search_flag = 1;
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_rqpn_parser_88xx done\n");
			break;
		}
	}

	if (search_flag == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]HALMAC_RET_TRX_MODE_NOT_SUPPORT 1 switch case not support\n");
		return HALMAC_RET_TRX_MODE_NOT_SUPPORT;
	}

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_88XX_SUPPORT */

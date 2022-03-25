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

#ifndef _HALMAC_COMMON_88XX_H_
#define _HALMAC_COMMON_88XX_H_

#include "../halmac_api.h"
#include "../halmac_pwr_seq_cmd.h"
#include "../halmac_gpio_cmd.h"

#if HALMAC_88XX_SUPPORT

HALMAC_RET_STATUS
halmac_ofld_func_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_OFLD_FUNC_INFO pOfld_func_info
);

HALMAC_RET_STATUS
halmac_dl_drv_rsvd_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 pg_offset,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
);

HALMAC_RET_STATUS
halmac_download_rsvd_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 pg_addr,
	IN u8 *pHal_buf,
	IN u32 size
);

HALMAC_RET_STATUS
halmac_get_hw_value_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_HW_ID hw_id,
	OUT VOID *pvalue
);

HALMAC_RET_STATUS
halmac_set_hw_value_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_HW_ID hw_id,
	IN VOID *pvalue
);

HALMAC_RET_STATUS
halmac_set_fw_offload_h2c_header_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u8 *pHal_h2c_hdr,
	IN PHALMAC_H2C_HEADER_INFO pH2c_header_info,
	OUT u16 *pSeq_num
);

HALMAC_RET_STATUS
halmac_send_h2c_pkt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHal_buff,
	IN u32 size,
	IN u8 ack
);

HALMAC_RET_STATUS
halmac_get_h2c_buff_free_space_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_get_c2h_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size
);

HALMAC_RET_STATUS
halmac_debug_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_cfg_parameter_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PHY_PARAMETER_INFO para_info,
	IN u8 full_fifo
);

HALMAC_RET_STATUS
halmac_update_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PACKET_ID pkt_id,
	IN u8 *pkt,
	IN u32 pkt_size
);

HALMAC_RET_STATUS
halmac_bcn_ie_filter_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_BCN_IE_INFO pBcn_ie_info
);

HALMAC_RET_STATUS
halmac_update_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type,
	IN PHALMAC_PHY_PARAMETER_INFO para_info
);

HALMAC_RET_STATUS
halmac_run_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type
);

HALMAC_RET_STATUS
halmac_send_bt_coex_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pBt_buf,
	IN u32 bt_size,
	IN u8 ack
);

HALMAC_RET_STATUS
halmac_send_original_h2c_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *original_h2c,
	IN u16 *seq,
	IN u8 ack
);

HALMAC_RET_STATUS
halmac_fill_txdesc_check_sum_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *cur_desc
);

HALMAC_RET_STATUS
halmac_dump_fifo_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HAL_FIFO_SEL halmac_fifo_sel,
	IN u32 halmac_start_addr,
	IN u32 halmac_fifo_dump_size,
	OUT u8 *pFifo_map
);

u32
halmac_get_fifo_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HAL_FIFO_SEL halmac_fifo_sel
);

HALMAC_RET_STATUS
halmac_set_h2c_header_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u8 *pHal_h2c_hdr,
	IN u16 *seq,
	IN u8 ack
);

HALMAC_RET_STATUS
halmac_add_ch_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_INFO pCh_info
);

HALMAC_RET_STATUS
halmac_add_extra_ch_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_EXTRA_INFO pCh_extra_info
);

HALMAC_RET_STATUS
halmac_ctrl_ch_switch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_SWITCH_OPTION pCs_option
);

HALMAC_RET_STATUS
halmac_clear_ch_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_send_general_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_GENERAL_INFO pGeneral_info
);

HALMAC_RET_STATUS
halmac_chk_txdesc_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
);

HALMAC_RET_STATUS
halmac_get_version_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_VER pVersion
);

HALMAC_RET_STATUS
halmac_p2pps_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_P2PPS    pP2PPS
);

HALMAC_RET_STATUS
halmac_query_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_FEATURE_ID feature_id,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_cfg_drv_rsvd_pg_num_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DRV_RSVD_PG_NUM pg_num
);

HALMAC_RET_STATUS
halmac_h2c_lb_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_pwr_seq_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 cut,
	IN u8 fab,
	IN u8 intf,
	IN PHALMAC_WLAN_PWR_CFG *ppPwr_seq_cfg

);

HALMAC_RET_STATUS
halmac_parse_intf_phy_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_INTF_PHY_PARA pIntf_phy_para,
	IN HALMAC_INTF_PHY_PLATFORM platform,
	IN HAL_INTF_PHY intf_phy
);

HALMAC_RET_STATUS
halmac_txfifo_is_empty_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 chk_num
);

u8*
halmac_adaptive_malloc_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 size,
	OUT u32 *pNew_size
);

HALMAC_RET_STATUS
halmac_ltecoex_reg_read_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 offset,
	OUT u32 *pValue
);

HALMAC_RET_STATUS
halmac_ltecoex_reg_write_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 offset,
	IN u32 value
);

HALMAC_RET_STATUS
halmac_download_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u8 *pHalmac_fw,
	IN u32 halmac_fw_size,
	IN u32 rom_address
);

HALMAC_RET_STATUS
halmac_read_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	u32 addr
);

HALMAC_RET_STATUS
halmac_erase_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	u8 erase_cmd,
	u32 addr
);

HALMAC_RET_STATUS
halmac_check_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u8 *pHalmac_fw,
	IN u32 halmac_fw_size,
	IN u32 addr
);

#endif/* HALMAC_88XX_SUPPORT */

#endif/* _HALMAC_COMMON_88XX_H_ */

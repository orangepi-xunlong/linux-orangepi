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

#ifndef _HALMAC_CFG_WMAC_88XX_H_
#define _HALMAC_CFG_WMAC_88XX_H_

#include "../halmac_api.h"

#if HALMAC_88XX_SUPPORT

HALMAC_RET_STATUS
halmac_cfg_mac_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN PHALMAC_WLAN_ADDR pHal_address
);

HALMAC_RET_STATUS
halmac_cfg_bssid_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN PHALMAC_WLAN_ADDR pHal_address
);

HALMAC_RET_STATUS
halmac_cfg_transmitter_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN PHALMAC_WLAN_ADDR pHal_address
);

HALMAC_RET_STATUS
halmac_cfg_net_type_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN HALMAC_NETWORK_TYPE_SELECT net_type
);

HALMAC_RET_STATUS
halmac_cfg_tsf_rst_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port
);

HALMAC_RET_STATUS
halmac_cfg_bcn_space_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN u32 bcn_space
);

HALMAC_RET_STATUS
halmac_rw_bcn_ctrl_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN u8 write_en,
	INOUT PHALMAC_BCN_CTRL pBcn_ctrl
);

HALMAC_RET_STATUS
halmac_cfg_multicast_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_WLAN_ADDR pHal_address
);

HALMAC_RET_STATUS
halmac_cfg_operation_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_WIRELESS_MODE wireless_mode
);

HALMAC_RET_STATUS
halmac_cfg_ch_bw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 channel,
	IN HALMAC_PRI_CH_IDX pri_ch_idx,
	IN HALMAC_BW bw
);

HALMAC_RET_STATUS
halmac_cfg_ch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 channel
);

HALMAC_RET_STATUS
halmac_cfg_pri_ch_idx_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PRI_CH_IDX pri_ch_idx
);

HALMAC_RET_STATUS
halmac_cfg_bw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_BW bw
);

VOID
halmac_enable_bb_rf_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
);

VOID
halmac_config_ampdu_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_AMPDU_CONFIG pAmpdu_config
);

HALMAC_RET_STATUS
halmac_cfg_la_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_LA_MODE la_mode
);

HALMAC_RET_STATUS
halmac_cfg_rx_fifo_expanding_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_RX_FIFO_EXPANDING_MODE rx_fifo_expanding_mode
);

HALMAC_RET_STATUS
halmac_config_security_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_SECURITY_SETTING pSec_setting
);

u8
halmac_get_used_cam_entry_num_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HAL_SECURITY_TYPE sec_type
);

HALMAC_RET_STATUS
halmac_write_cam_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u32 entry_index,
	IN PHALMAC_CAM_ENTRY_INFO pCam_entry_info
);

HALMAC_RET_STATUS
halmac_read_cam_entry_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u32 entry_index,
	OUT PHALMAC_CAM_ENTRY_FORMAT pContent
);

HALMAC_RET_STATUS
halmac_clear_cam_entry_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 entry_index
);

VOID
halmac_rx_shift_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
);

HALMAC_RET_STATUS
halmac_cfg_edca_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_ACQ_ID acq_id,
	IN PHALMAC_EDCA_PARA pEdca_para
);

VOID
halmac_rx_clk_gate_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
);

HALMAC_RET_STATUS
halmac_rx_cut_amsdu_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CUT_AMSDU_CFG pCut_amsdu_cfg
);

HALMAC_RET_STATUS
halmac_get_mac_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	OUT PHALMAC_WLAN_ADDR pHal_address
);

#endif/* HALMAC_88XX_SUPPORT */

#endif/* _HALMAC_CFG_WMAC_88XX_H_ */

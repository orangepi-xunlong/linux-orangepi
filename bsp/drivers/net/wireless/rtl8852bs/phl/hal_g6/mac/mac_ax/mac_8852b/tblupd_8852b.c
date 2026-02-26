/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation. All rights reserved.
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

#include "tblupd_8852b.h"

#if MAC_AX_8852B_SUPPORT
u32 mac_init_cctl_info_8852b(struct mac_ax_adapter *adapter, u8 macid)
{
	struct mac_ax_ops *mops = adapter_to_mac_ops(adapter);
	struct rtw_hal_mac_ax_cctl_info info = {0x0};
	struct rtw_hal_mac_ax_cctl_info mask;

	// dword0
	info.datarate = MAC_AX_OFDM6;
	// dword1
	info.data_rty_lowest_rate = MAC_AX_OFDM6;
	info.rtsrate = MAC_AX_OFDM48;
	info.rts_rty_lowest_rate = MAC_AX_OFDM6;
	// dword5
	info.resp_ref_rate = MAC_AX_OFDM54;
	info.ntx_path_en = CCTL_NTX_PATH_EN_8852B;
	info.path_map_b = CCTL_PATH_MAP_B;
	info.path_map_c = CCTL_PATH_MAP_C;
	info.path_map_d = CCTL_PATH_MAP_D;
	// dword6
	info.nominal_pkt_padding = CCTRL_NOMINAL_PKT_PADDING_16;
	info.nominal_pkt_padding40 = CCTRL_NOMINAL_PKT_PADDING_16;
	info.nominal_pkt_padding80 = CCTRL_NOMINAL_PKT_PADDING_16;
	// dword7
	info.nc = CCTRL_NC;
	info.nr = CCTRL_NR;
	info.cb = CCTRL_CB;
	info.csi_para_en = 0x1;
	info.csi_fix_rate = MAC_AX_OFDM54;

	PLTFM_MEMSET(&mask, 0xFF, sizeof(struct rtw_hal_mac_ax_cctl_info));

	return mops->upd_cctl_info(adapter, &info, &mask, macid, 1);
}
#endif

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

#include "phy_misc.h"

u32 mac_fast_ch_sw(struct mac_ax_adapter *adapter, struct mac_ax_fast_ch_sw_param *fast_ch_sw_param)
{
	u8 *buf;
	u32 ret = MACSUCCESS;
	struct fwcmd_fcs *pkt;
	struct h2c_info h2c_info = {0};

	buf = (u8 *)PLTFM_MALLOC(sizeof(struct fwcmd_fcs));
	if (!buf) {
		PLTFM_MSG_ERR("[HM][H2C][FCS] ret = %d\n", MACNOBUF);
		return MACNOBUF;
	}
	pkt = (struct fwcmd_fcs *)buf;
	pkt->dword0 = cpu_to_le32(SET_WORD(fast_ch_sw_param->ap_port_id,
					   FWCMD_H2C_FCS_AP_PORT_ID) |
				  SET_WORD(fast_ch_sw_param->ch_idx,
					   FWCMD_H2C_FCS_CH_IDX) |
				  SET_WORD(fast_ch_sw_param->thermal_idx,
					   FWCMD_H2C_FCS_THERMAL_IDX) |
				  SET_WORD(fast_ch_sw_param->pause_rel_mode,
					   FWCMD_H2C_FCS_PAUSE_REL_MODE) |
				  SET_WORD(fast_ch_sw_param->con_sta_num,
					   FWCMD_H2C_FCS_CON_STA_NUM) |
				  (fast_ch_sw_param->band ? FWCMD_H2C_FCS_BAND : 0) |
				  SET_WORD(fast_ch_sw_param->bandwidth,
					   FWCMD_H2C_FCS_BANDWIDTH) |
				  SET_WORD(fast_ch_sw_param->ch_band, FWCMD_H2C_FCS_CH_BAND));
	pkt->dword1 = cpu_to_le32(SET_WORD(fast_ch_sw_param->pri_ch,
					   FWCMD_H2C_FCS_PRI_CH) |
				  SET_WORD(fast_ch_sw_param->central_ch,
					   FWCMD_H2C_FCS_CENTRAL_CH));
	pkt->dword2 = cpu_to_le32(fast_ch_sw_param->rel_pause_tsfl);
	pkt->dword3 = cpu_to_le32(fast_ch_sw_param->rel_pause_tsfh);
	pkt->dword4 = cpu_to_le32(fast_ch_sw_param->rel_pause_delay_time);
	pkt->dword5 = cpu_to_le32(SET_WORD(fast_ch_sw_param->csa_pkt_id[0],
					   FWCMD_H2C_FCS_CSA_PKT_ID0) |
				  SET_WORD(fast_ch_sw_param->csa_pkt_id[1],
					   FWCMD_H2C_FCS_CSA_PKT_ID1) |
				  SET_WORD(fast_ch_sw_param->csa_pkt_id[2],
					   FWCMD_H2C_FCS_CSA_PKT_ID2) |
				  SET_WORD(fast_ch_sw_param->csa_pkt_id[3],
					   FWCMD_H2C_FCS_CSA_PKT_ID3));
	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_fcs);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_FCS;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_FCS;
	h2c_info.rec_ack = 1;
	h2c_info.done_ack = 1;

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)buf);
	PLTFM_FREE(buf, sizeof(struct fwcmd_fcs));

	if (!ret)
		adapter->fast_ch_sw_info.busy = 1;
	PLTFM_MSG_TRACE("[HM][H2C][FCS] ret = %d\n", ret);
	return ret;
}

u32 mac_fast_ch_sw_done(struct mac_ax_adapter *adapter)
{
	if (adapter->fast_ch_sw_info.busy)
		return MACPROCBUSY;
	else
		return MACSUCCESS;
}

u32 mac_get_fast_ch_sw_rpt(struct mac_ax_adapter *adapter, u32 *fast_ch_sw_status_code)
{
	*fast_ch_sw_status_code = adapter->fast_ch_sw_info.status;
	return MACSUCCESS;
}
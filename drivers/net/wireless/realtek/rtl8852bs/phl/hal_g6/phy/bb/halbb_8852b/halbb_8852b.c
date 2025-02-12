/******************************************************************************
 *
 * Copyright(c) 2007 - 2020  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/
#include "../halbb_precomp.h"

#ifdef BB_8852B_SUPPORT

bool halbb_chk_pkg_valid_8852b(struct bb_info *bb, u8 bb_ver, u8 rf_ver)
{
	bool valid = true;

#if 0
	if (bb_ver >= X && rf_ver >= Y)
		valid = true;
	else if (bb_ver < X && rf_ver < Y)
		valid = true;
	else
		valid = false;
#endif

	if (!valid) {
		/*halbb_set_reg(bb, 0x1c3c, (BIT(0) | BIT(1)), 0x0);*/
		BB_WARNING("[%s] Pkg_ver{bb, rf}={%d, %d} disable all BB block\n",
			 __func__, bb_ver, rf_ver);
	}

	return valid;
}

bool halbb_chk_tx_idle_8852b(struct bb_info *bb, enum phl_phy_idx phy_idx)
{
	u8 tx_state = 0;
	bool idle = false;
	u32 dbg_port = 0x30002;

	BB_DBG(bb, DBG_PHY_CONFIG, "<====== %s ======>\n", __func__);

	if (phy_idx == HW_PHY_1)
		return false;

	if (halbb_bb_dbg_port_racing(bb, DBGPORT_PRI_3)) {
		halbb_dbg_port_sel(bb, (u16)(dbg_port & 0xffff),
				   (u8)((dbg_port & 0xff0000) >> 16), 0x0, 0x1);
		BB_DBG(bb, DBG_PHY_CONFIG,
		       "*Set dbg_port=(0x%x)\n", dbg_port);
	} else {
		dbg_port = halbb_get_bb_dbg_port_idx(bb);
		BB_DBG(bb, DBG_PHY_CONFIG,
		       "[Set dbg_port fail!] Curr-DbgPort=0x%x\n", dbg_port);

		return false;
	}

	/* Release DBG port */
	halbb_release_bb_dbg_port(bb);

	tx_state = (u8)(halbb_get_bb_dbg_port_val(bb) & 0x3f);

	if (tx_state == 0)
		idle = true;
	else
		idle = false;

	return idle;
}

void halbb_stop_pmac_tx_8852b(struct bb_info *bb,
			      struct halbb_pmac_info *tx_info,
			      enum phl_phy_idx phy_idx)
{
	if (tx_info->is_cck) { // CCK
		if (tx_info->mode == CONT_TX) {
			halbb_set_reg(bb, 0x2300, BIT(26), 1);
			halbb_set_reg(bb, 0x2338, BIT(17), 0);
			halbb_set_reg(bb, 0x2300, BIT(28), 0);
			halbb_set_reg(bb, 0x2300, BIT(26), 0);
		} else if (tx_info->mode == PKTS_TX) {
			halbb_set_reg_cmn(bb, 0x9c4, BIT(4), 0, phy_idx);
		}
	} else { // OFDM
		if (tx_info->mode == CONT_TX)
			halbb_set_reg_cmn(bb, 0x9c4, BIT(0), 0, phy_idx);
		else if (tx_info->mode == PKTS_TX)
			halbb_set_reg_cmn(bb, 0x9c4, BIT(4), 0, phy_idx);
	}
}


void halbb_start_pmac_tx_8852b(struct bb_info *bb,
			       struct halbb_pmac_info *tx_info,
			       enum halbb_pmac_mode mode, u32 pkt_cnt,u16 period,
			       enum phl_phy_idx phy_idx)
{
	if (mode == CONT_TX) {
		if (tx_info->is_cck) {
			halbb_set_reg(bb, 0x2338, BIT(17), 1);
			halbb_set_reg(bb, 0x2300, BIT(28), 0);
		} else {
			halbb_set_reg_cmn(bb, 0x9c4, BIT(0), 1, phy_idx);
		}
	} else if (mode == PKTS_TX) {
		/*Tx_N_PACKET_EN */
		halbb_set_reg_cmn(bb, 0x9c4, BIT(4), 1, phy_idx);
		/*Tx_N_PERIOD */
		halbb_set_reg_cmn(bb, 0x9c4, 0xffffff00, period, phy_idx);
		/*Tx_N_PACKET */
		halbb_set_reg_cmn(bb, 0x9c8, 0xffffffff, pkt_cnt, phy_idx);
	} else if (mode == CCK_CARRIER_SIPPRESSION_TX) {
		if (tx_info->is_cck) {
			/*Carrier Suppress Tx*/
			halbb_set_reg(bb, 0x2338, BIT(18), 1);
			/*Disable scrambler at payload part*/
			halbb_set_reg(bb, 0x2304, BIT(26), 1);
		} else {
			return;
		}
		/*Tx_N_PACKET_EN */
		halbb_set_reg_cmn(bb, 0x9c4, BIT(4), 1, phy_idx);
		/*Tx_N_PERIOD */
		halbb_set_reg_cmn(bb, 0x9c4, 0xffffff00, period, phy_idx);
		/*Tx_N_PACKET */
		halbb_set_reg_cmn(bb, 0x9c8, 0xffffffff, pkt_cnt, phy_idx);
	}
	/*Tx_EN */
	halbb_set_reg_cmn(bb, 0x9c0, BIT(0), 1, phy_idx);
	halbb_set_reg_cmn(bb, 0x9c0, BIT(0), 0, phy_idx);
}


void halbb_set_pmac_tx_8852b(struct bb_info *bb, struct halbb_pmac_info *tx_info,
			     enum phl_phy_idx phy_idx)
{
	BB_DBG(bb, DBG_PHY_CONFIG, "<====== %s ======>\n", __func__);

	if (!tx_info->en_pmac_tx) {
		halbb_stop_pmac_tx_8852b(bb, tx_info, phy_idx);
		/* PD hit enable */
		halbb_set_reg_cmn(bb, 0xc3c, BIT(9), 0, phy_idx);
		halbb_set_reg(bb, 0x2344, BIT(31), 0);
		return;
	}
	/*Turn on PMAC */
	/* Tx */
	halbb_set_reg_cmn(bb, 0x0980, BIT(0), 1, phy_idx);
	/* Rx */
	halbb_set_reg_cmn(bb, 0x0980, BIT(16), 1, phy_idx);
	halbb_set_reg_cmn(bb, 0x0988, 0x3f, 0x3f, phy_idx);

	/* PD hit enable */
	halbb_set_reg(bb, 0x704, BIT(1), 0);
	halbb_set_reg_cmn(bb, 0xc3c, BIT(9), 1, phy_idx);
	halbb_set_reg(bb, 0x2344, BIT(31), 1);
	halbb_set_reg(bb, 0x704, BIT(1), 1);

	halbb_start_pmac_tx_8852b(bb, tx_info, tx_info->mode, tx_info->tx_cnt,
		       tx_info->period, phy_idx);
}

void halbb_set_tmac_tx_8852b(struct bb_info *bb, enum phl_phy_idx phy_idx)
{
	/* To do: 0x0d80[16] [25] / 0x0d88[5:0] Should be set to default value in parameter package*/
	/* Turn on TMAC */
	halbb_set_reg_cmn(bb, 0x0980, BIT(0), 0, phy_idx);
	halbb_set_reg_cmn(bb, 0x0980, BIT(16), 0, phy_idx);
	halbb_set_reg_cmn(bb, 0x0988, 0xfff, 0, phy_idx);
	halbb_set_reg_cmn(bb, 0x0994, 0xf0, 0, phy_idx);
	
	// PDP bypass from TMAC
	halbb_set_reg_cmn(bb, 0x09a4, BIT(10), 0, phy_idx);
	// TMAC Tx path
	halbb_set_reg_cmn(bb, 0x09a4, 0x1c, 0, phy_idx);
	// TMAC Tx power
	halbb_set_reg_cmn(bb, 0x09a4, BIT(16), 0, phy_idx);
}

void halbb_ic_hw_setting_init_8852b(struct bb_info *bb)
{
	BB_DBG(bb, DBG_PHY_CONFIG, "<====== %s ======>\n", __func__);

	if(phl_is_mp_mode(bb->phl_com)) {
		// r_en_sound_wo_ndp
		halbb_set_reg(bb, 0xd7c, BIT(1), 1);
		halbb_set_reg(bb, 0x2d7c, BIT(1), 1);
	} else {
		// r_en_sound_wo_ndp
		halbb_set_reg(bb, 0xd7c, BIT(1), 0);
		halbb_set_reg(bb, 0x2d7c, BIT(1), 0);
	}
}

void halbb_ic_hw_setting_8852b(struct bb_info *bb)
{
	bool btg_en;
	struct bb_link_info *link = &bb->bb_link_i;
	struct rtw_phl_stainfo_t *sta = NULL;
	struct rtw_rssi_info *sta_rssi = NULL;
	u16 rssi_a = 0;
	u16 rssi_b = 0;
	u16 rssi_path_diff = 0;
	u32 id = bb->phl_com->id.id & 0xFFFF;
	
	BB_DBG(bb, DBG_PHY_CONFIG, "<====== %s ======>\n", __func__);

	if (!link->is_linked)
		return;
	
	if (!link->is_one_entry_only)
		return;

	sta = bb->phl_sta_info[link->one_entry_macid];
	sta_rssi = &sta->hal_sta->rssi_stat;
	rssi_a = sta_rssi->rssi_ma_path[0];
	rssi_b = sta_rssi->rssi_ma_path[1];
	rssi_path_diff = DIFF_2(rssi_a, rssi_b) >> 5;

	btg_en = (bb->hal_com->band[0].cur_chandef.band == BAND_ON_24G) &&
		((bb->rx_path == RF_PATH_B) || (bb->rx_path == RF_PATH_AB)) ? true : false;
	
	if (btg_en && bb->bb_link_i.is_linked && (bb->bb_ch_i.rssi_min < (75 << 1))) {// if rssi < -35 dbm
		halbb_set_reg(bb, 0x4aa4, BIT(18), 0x0);
		BB_DBG(bb, DBG_PHY_CONFIG, "[BT] BTG enable, Is linked\n");
		BB_DBG(bb, DBG_PHY_CONFIG, "[BT][RSSI] rssi_min=0x%x\n", bb->bb_ch_i.rssi_min >> 1);
	} else {
		halbb_set_reg(bb, 0x4aa4, BIT(18), 0x1);
		BB_DBG(bb, DBG_PHY_CONFIG, "[BT] btg_en=%d, is_linked=%d\n", btg_en, bb->bb_link_i.is_linked);
		BB_DBG(bb, DBG_PHY_CONFIG, "[BT][RSSI] rssi_min=0x%x\n", bb->bb_ch_i.rssi_min >> 1);
	}

	// for wrc
	if ((bb->hal_com->band[0].cur_chandef.band == BAND_ON_5G) && (id == 0x112))
		halbb_set_reg(bb, 0x4408, BIT(25), 0x1);
	else
		halbb_set_reg(bb, 0x4408, BIT(25), 0x0);

	if (rssi_path_diff > 15) {
		halbb_set_reg(bb, 0x4964, BIT(31), 0);
		BB_DBG(bb, DBG_PHY_CONFIG,
			"rssi(a,b)=(%d,%d),rssi_diff=(%d),ant_wgt_normalize_mode_OFF\n",
			rssi_a, rssi_b, rssi_path_diff);
	} else {
		halbb_set_reg(bb, 0x4964, BIT(31), 1);
		BB_DBG(bb, DBG_PHY_CONFIG,
			"rssi(a,b)=(%d,%d),rssi_diff=(%d),ant_wgt_normalize_mode_ON\n",
			rssi_a, rssi_b, rssi_path_diff);
	}
	
}

bool halbb_set_pd_lower_bound_8852b(struct bb_info *bb, u8 bound,
				      enum channel_width bw,
				      enum phl_phy_idx phy_idx)
{
	/* 
	Range of bound value: 
	BW20: 95~33
	BW40: 92~30
	BW80: 89~27
	*/
	u8 bw_attenuation = 0;
	u8 subband_filter_atteniation = 7;
	u8 bound_idx = 0;
	bool rpt = true;

	BB_DBG(bb, DBG_PHY_CONFIG, "<====== %s ======>\n", __func__);

	if (bound == 0) {
		halbb_set_reg_cmn(bb, 0x4860, BIT(30), 0, phy_idx);
		BB_DBG(bb, DBG_PHY_CONFIG,
		       "[PD Bound] Set Boundary to default!\n");
		return true;
	}

	bb->bb_cmn_backup_i.cur_pd_lower_bound = bound;

	if ((bw == CHANNEL_WIDTH_20) || (bw == CHANNEL_WIDTH_10) || (bw == CHANNEL_WIDTH_5)) {
		bw_attenuation = 0;
	} else if (bw == CHANNEL_WIDTH_40) {
		bw_attenuation = 3;
	} else if (bw == CHANNEL_WIDTH_80) {
		bw_attenuation = 6;
	} else {
		BB_DBG(bb, DBG_PHY_CONFIG,
		       "[PD Bound] Only support BW20/40/80 !\n");
		return false;
	}

	bound += (bw_attenuation + subband_filter_atteniation);
	// If Boundary dbm is odd, set it to even number
	bound = bound % 2 ? bound + 1 : bound;

	if (bound < 40) {
		BB_DBG(bb, DBG_PHY_CONFIG,
		       "[PD Bound] Threshold too high, set to highest level!\n");
		bound = 40;
		rpt = false;
	}

	if (bound > 102) {
		BB_DBG(bb, DBG_PHY_CONFIG,
		       "[PD Bound] Threshold too low, disable PD lower bound function!\n");
		halbb_set_reg_cmn(bb, 0x4860, BIT(30), 0, phy_idx);
		return true;
	}

	bound_idx =  (102 - bound) >> 1;

	halbb_set_reg_cmn(bb, 0x4860, 0x7c0, bound_idx, phy_idx);
	halbb_set_reg_cmn(bb, 0x4860, BIT(30), 1, phy_idx);

	BB_DBG(bb, DBG_PHY_CONFIG, "[PD Bound] Set Boundary Success!\n");

	return rpt;
}

bool halbb_set_pd_lower_bound_cck_8852b(struct bb_info *bb, u8 bound,
				      enum channel_width bw,
				      enum phl_phy_idx phy_idx)
{
	u8 bw_attenuation = 0;
	u8 subband_filter_atteniation = 5;
	s8 bound_tmp = 0;

	BB_DBG(bb, DBG_PHY_CONFIG, "<====== %s ======>\n", __func__);

	if (bb->hal_com->cv < CBV) {
		BB_DBG(bb, DBG_PHY_CONFIG, "[PD Bound] CCK pd_lower_bound CV not support!\n");
		return false;
	}
	
	if (bound == 0) {
		halbb_set_reg_cmn(bb, 0x23b0, BIT(23), 0, phy_idx);
		BB_DBG(bb, DBG_PHY_CONFIG,
			"[PD Bound] Set Boundary to default!\n");
		return true;
	}

	if ((bw == CHANNEL_WIDTH_20) || (bw == CHANNEL_WIDTH_10) || (bw == CHANNEL_WIDTH_5)) {
		bw_attenuation = 0;
	}
	else if (bw == CHANNEL_WIDTH_40) {
		bw_attenuation = 3;
	}
	else if (bw == CHANNEL_WIDTH_80) {
		bw_attenuation = 6;
	}
	else {
		BB_DBG(bb, DBG_PHY_CONFIG,
			"[PD Bound] Only support BW20/40/80 !\n");
		return true;
	}

	bound += (bw_attenuation + subband_filter_atteniation);
	bound_tmp = (-1) * MIN_2(bound, 128);

	halbb_set_reg_cmn(bb, 0x23b0, 0xff000000, bound_tmp, phy_idx);
	halbb_set_reg_cmn(bb, 0x23b4, 0xff000000, 0x7f, phy_idx);
	halbb_set_reg_cmn(bb, 0x23b0, BIT(23), 1, phy_idx);

	BB_DBG(bb, DBG_PHY_CONFIG, "[PD Bound] Set CCK Boundary Success!\n");

	return true;
}

u8 halbb_querry_pd_lower_bound_8852b(struct bb_info *bb, bool get_en_info, enum phl_phy_idx phy_idx)
{
	u8 tmp;

	BB_DBG(bb, DBG_PHY_CONFIG, "<====== %s ======>\n", __func__);

	if (get_en_info)
		tmp = (u8)halbb_get_reg_cmn(bb, 0x4860, BIT(30), phy_idx);
	else
		tmp = bb->bb_cmn_backup_i.cur_pd_lower_bound;

	return tmp;
}

void halbb_pop_en_8852b(struct bb_info *bb, bool en, enum phl_phy_idx phy_idx)
{
	BB_DBG(bb, DBG_PHY_CONFIG, "<====== %s ======>\n", __func__);

	if (en) {
		halbb_set_reg_cmn(bb, 0x47D4, BIT(8), 1, phy_idx);

	} else {
		halbb_set_reg_cmn(bb, 0x47D4, BIT(8), 0, phy_idx);
	}
}

bool halbb_querry_pop_en_8852b(struct bb_info *bb, enum phl_phy_idx phy_idx)
{
	bool en;

	BB_DBG(bb, DBG_PHY_CONFIG, "<====== %s ======>\n", __func__);

	en = (bool)halbb_get_reg_cmn(bb, 0x47D4, BIT(8), phy_idx);

	return en;
}

void halbb_dyn_mu_bypass_vht_sigb_8852b(struct bb_info *bb)
{
	struct bb_cmn_rpt_info	*cmn_rpt = &bb->bb_cmn_rpt_i;
	struct bb_pkt_cnt_mu_info *pkt_cnt = &cmn_rpt->bb_pkt_cnt_mu_i;
	u8 offset = 0;
	u32 vht_mu_pkt_th = 10;
	u32 vht_mu_pkt = 0;

	for (offset = 0; offset < VHT_RATE_NUM; offset++) {
		vht_mu_pkt += pkt_cnt->pkt_cnt_vht[offset];
	}
	if (vht_mu_pkt > vht_mu_pkt_th) {
		halbb_set_reg(bb, 0x4414, BIT(5), 1); // loop filter bandwidth selection slicer for 1st step to 2nd step
		halbb_set_reg(bb, 0x4400, 0x07000000, 0x3); // channel tracking coefficients selection slicer for 1st step to 2nd step
		halbb_set_reg(bb, 0x4404, 0x000f8000, 0x1f); // data tracking loop filter bandwidth selection for 2nd step
		halbb_set_reg(bb, 0x4404, 0x0000001f, 0x4); // channel tracking coefficients selection slicer for 2nd step to 3rd step
		halbb_set_reg(bb, 0x43f8, 0x00fc0000, 0x0); // channel tracking coefficients for 2nd step
		halbb_set_reg(bb, 0x440c, 0x000f8000, 0x1f); // pilot tracking loop filter bandwidth selection for 2nd step
		halbb_set_reg(bb, 0x43f8, BIT(30), 0); // enable noise variance tracking
	} else {
		halbb_set_reg(bb, 0x4414, BIT(5), 0); // loop filter bandwidth selection slicer for 1st step to 2nd step
		halbb_set_reg(bb, 0x4400, 0x07000000, 0x4); // channel tracking coefficients selection slicer for 1st step to 2nd step
		halbb_set_reg(bb, 0x4404, 0x000f8000, 0x1); // data tracking loop filter bandwidth selection for 2nd step
		halbb_set_reg(bb, 0x4404, 0x0000001f, 0x8); // channel tracking coefficients selection slicer for 2nd step to 3rd step
		halbb_set_reg(bb, 0x43f8, 0x00fc0000, 0x4); // channel tracking coefficients for 2nd step
		halbb_set_reg(bb, 0x440c, 0x000f8000, 0x1); // pilot tracking loop filter bandwidth selection for 2nd step
		halbb_set_reg(bb, 0x43fc, BIT(30), 1); // enable noise variance tracking
	}
}


#endif

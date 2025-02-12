/******************************************************************************
 *
 * Copyright(c) 2007 - 2020  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHPATHABILITY or
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
#include "halbb_precomp.h"

#ifdef HALBB_PATH_DIV_SUPPORT

bool halbb_pathdiv_abort(struct bb_info *bb)
{
	if (!(bb->support_ability & BB_PATH_DIV)) {
		BB_DBG(bb, DBG_PATH_DIV, "Not support path diversity\n");
		return true;
	}

	if (bb->pause_ability & BB_PATH_DIV) {
		BB_DBG(bb, DBG_PATH_DIV, "Return path diversity pause\n");
		return true;
	}

	return false;
}

#ifdef HALBB_COMPILE_AX_SERIOUS
void halbb_pathdiv_reset_stat(struct bb_info *bb)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct bb_pathdiv_rssi_info *bb_rssi_i = &bb_path_div->bb_rssi_i;

	halbb_mem_set(bb, bb_rssi_i, 0, sizeof(struct bb_pathdiv_rssi_info));
}
#endif

void halbb_pathdiv_reset(struct bb_info *bb)
{
	BB_DBG(bb, DBG_PATH_DIV, "[%s]\n", __func__);

	#ifdef HALBB_COMPILE_AX_SERIOUS
	/* Reset stat */
	halbb_pathdiv_reset_stat(bb);
	#endif
}

void halbb_pathdiv_reg_init(struct bb_info *bb)
{
#ifdef BB_8852A_CAV_SUPPORT
#endif
#ifdef HALBB_COMPILE_AP_SERIES
#endif
}

void halbb_pathdiv_init(struct bb_info *bb)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	//struct bb_pathdiv_rssi_info *rssi = &bb_path_div->bb_rssi_i;
	u32 i = 0, val = 0;

	if (!(bb->support_ability & BB_PATH_DIV))
		return;

	if (bb->phl_com->phy_cap[0].tx_path_num < 2){
		BB_DBG(bb, DBG_PATH_DIV, "Tx path num < 2, Not support path diversity\n");
		bb->support_ability &= ~BB_PATH_DIV;
		return;
	}

	BB_DBG(bb, DBG_PATH_DIV, "%s ======>\n", __func__);

	if (bb->bb_80211spec == BB_AX_IC) {
		bb_path_div->path_sel_1ss = BB_PATH_AUTO;
	} else {
		if (bb_path_div->bb_path_en_i.max_tx_path_en) {
			bb_path_div->bb_path_en_i.path_div_en = BB_PATH_DIV_DISABLE;
			bb_path_div->path_sel_1ss = halbb_gen_mask_from_0(bb->num_rf_path);
		} else {
			if (bb_path_div->bb_path_en_i.path_div_en == BB_PATH_DIV_ENABLE)
				bb_path_div->path_sel_1ss = BB_PATH_AUTO;
			else
				bb_path_div->path_sel_1ss = bb_path_div->bb_path_en_i.bb_path_1sts;
		}
	}

	bb_path_div->use_path_a_as_default_ant = 1;
	bb_path_div->num_tx_path = bb->num_rf_path - 1;
	bb_path_div->default_path = BB_PATH_A;

	bb_path_div->rssi_decision_method = RSSI_LINEAR_AVG;
	bb_path_div->path_rssi_gap = PATH_DIV_RSSI_GAP; /*2dB, u(8,1) RSSI*/

	for (i = 0; i < PHL_MAX_STA_NUM; i++) {
		/*BB_PATH_AB is a invalid value used for init state*/
		bb_path_div->fix_path_en[i] = false;
		bb_path_div->fix_path_sel[i] = BB_PATH_A;
		if ((bb->ic_type == BB_RTL8852C) || (bb->ic_sub_type == BB_IC_SUB_TYPE_8852B_8852BP))
			bb_path_div->path_sel[i] = BB_PATH_B; /*LNV 6G conductive*/
		else
			bb_path_div->path_sel[i] = BB_PATH_NON;
	}

	halbb_pathdiv_reset(bb);

	if (bb->bb_80211spec == BB_AX_IC) {
		halbb_pause_func(bb, F_PATH_DIV, HALBB_PAUSE_NO_SET, HALBB_PAUSE_LV_1, 1, &val, bb->bb_phy_idx);
	} else {
		if (bb_path_div->bb_path_en_i.path_div_en == BB_PATH_DIV_DISABLE)
			halbb_pause_func(bb, F_PATH_DIV, HALBB_PAUSE_NO_SET, HALBB_PAUSE_LV_1, 1, &val, bb->bb_phy_idx);
	}


	BB_DBG(bb, DBG_INIT, "Init path_diversity");
}

void halbb_set_cctrl_tbl(struct bb_info *bb, u16 macid, u16 cfg)
{
	struct hal_txmap_cfg txmap_cfg;

	halbb_mem_set(bb, &txmap_cfg, 0, sizeof(struct hal_txmap_cfg));

	txmap_cfg.macid = (u8)macid;
	txmap_cfg.n_tx_en = cfg & 0x0f;
	txmap_cfg.map_a = ((cfg>>4) & 0x03);
	txmap_cfg.map_b = ((cfg>>6) & 0x03);
	txmap_cfg.map_c = ((cfg>>8) & 0x03);
	txmap_cfg.map_d = ((cfg>>10) & 0x03);

	if (rtw_hal_mac_tx_path_map_cfg(bb->hal_com, &txmap_cfg))
		BB_DBG(bb, DBG_PATH_DIV, "halbb_set_cctrl_tbl failed\n");
	else
		BB_DBG(bb, DBG_PATH_DIV, "halbb_set_cctrl_tbl success\n");
}

void halbb_set_tx_path_by_cmac_tbl(struct bb_info *bb, u16 macid, enum bb_path tx_path_sel_1ss)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	enum bb_path tx_path_sel = tx_path_sel_1ss;
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	u16 cfg = 0;

	/*Adv-ctrl mode*/
	if (bb_path_div->fix_path_en[macid]) {
		tx_path_sel = bb_path_div->fix_path_sel[macid];
		BB_DBG(bb, DBG_PATH_DIV, "Fix TX path= %s\n",
		       (tx_path_sel == BB_PATH_A) ? "A" : "B");
	} else {
		if (bb_path_div->path_sel_1ss == BB_PATH_AUTO)
			tx_path_sel = bb_path_div->path_sel[macid];
		else
			tx_path_sel= bb_path_div->path_sel_1ss;
	}

	BB_DBG(bb, DBG_PATH_DIV, "STA[%d] : path_sel= [%s]\n", macid,
	       (tx_path_sel == BB_PATH_A) ? "A" : "B");
	/*BB_PATH != RF_PATH*/
	tx_path_sel = (enum bb_path)((tx_path_sel == BB_PATH_A) ? RF_PATH_A : RF_PATH_B);
	cfg = halbb_cfg_cmac_tx_ant(bb, (enum rf_path)tx_path_sel);

	halbb_set_cctrl_tbl(bb, macid, cfg);

}

void halbb_set_tx_path_by_reg(struct bb_info *bb, u16 macid, enum bb_path tx_path_sel_1ss)
{

	/* [3] macid_norm_en[0] ||
	|| [4] macid_resp_en[0] ||
	|| [16:13] path_en[3:0] ||
	|| [18:17] map_a[1:0]   ||
	|| [20:19] map_b[1:0]   ||
	|| [22:21] map_c[1:0]   ||
	|| [24:23] map_d[1:0]   ||
	|| [28] hang_proof_en[0]*/

	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct rtw_hal_com_t *hal_com = bb->hal_com;
	enum bb_path tx_path_sel = tx_path_sel_1ss;
	enum bb_path max_tx_path = halbb_gen_mask_from_0(bb->num_rf_path);
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	//band = (bb->bb_phy_idx == HW_PHY_0) ? HW_BAND_0 : HW_BAND_1;
	enum phl_band_idx band = HW_BAND_0;
	u8 i = 0;
	u32 user_path_addr, val = 0x0;
	u32 offset = 4, user_base_addr = 0x0;

	if (bb->bb_80211spec == BB_AX_IC)
		user_base_addr = 0xDC00;
	else if (bb->ic_type == BB_RTL8922A)
		user_base_addr = 0xE500;
	else
		user_base_addr = 0xC000;

	/*Adv-ctrl mode*/
	if (bb_path_div->fix_path_en[macid]) {
		tx_path_sel = bb_path_div->fix_path_sel[macid];
		BB_DBG(bb, DBG_PATH_DIV, "Fix TX path= %s\n",
		       (tx_path_sel == BB_PATH_A) ? "A" : "B");
	} else {
		if (bb_path_div->path_sel_1ss == BB_PATH_AUTO)
			tx_path_sel = bb_path_div->path_sel[macid];
		else
			tx_path_sel= bb_path_div->path_sel_1ss;
	}

	if (tx_path_sel == BB_PATH_NON) {
		BB_DBG(bb, DBG_PATH_DIV, "STA[%d] : invalid tx path\n", macid);
		return;
	}

	BB_DBG(bb, DBG_PATH_DIV, "STA[%d] : path_sel= [%s]\n", macid,
	       (tx_path_sel == BB_PATH_AB) ? "AB" :
	       (tx_path_sel == BB_PATH_A ? "A" : "B"));

	user_path_addr = user_base_addr + offset * macid;

	val &= ~0x1E000; /*[16:13] path_en[3:0]*/
	val |= (tx_path_sel << 13);

	if (tx_path_sel == max_tx_path)
		val = 0;
	else
		val |= (BIT(3) | BIT(4) | BIT(28));

	BB_DBG(bb, DBG_PATH_DIV, "0x%x = 0x%x\n", user_path_addr, val);

	rtw_hal_mac_set_pwr_reg(hal_com, (u8)band, user_path_addr, val);
}

void halbb_set_tx_path(struct bb_info *bb, u16 macid, enum bb_path tx_path_sel_1ss)
{
	switch (bb->ic_type) {

	case BB_RTL8852A:
	case BB_RTL8852B:

	case BB_RTL8851B:
		halbb_set_tx_path_by_cmac_tbl(bb, macid, tx_path_sel_1ss);
		break;

	default:
		halbb_set_tx_path_by_reg(bb, macid, tx_path_sel_1ss);
		break;
	}
}

void halbb_set_pathdiv_pause_val(struct bb_info *bb, u32 *val_buf, u8 val_len)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct bb_link_info *bb_link = &bb->bb_link_i;
	struct rtw_phl_stainfo_t *sta;
	u16 i = 0, sta_cnt = 0;
	u8 pause_result = 0;
	u32 val = 0;

	if (val_len != 1) {
		BB_DBG(bb, DBG_PATH_DIV, "[Error][PathDiv]Need val_len=1\n");
		return;
	}
	BB_DBG(bb, DBG_PATH_DIV, "[%s] len=%d, val[0]=0x%x\n", __func__, val_len, val_buf[0]);

	halbb_ctrl_tx_path_div(bb, val_buf[0]);

	if (bb_path_div->path_sel_1ss != BB_PATH_AUTO) {
		for (i = 0; i < PHL_MAX_STA_NUM; i++) {
			if (!bb->sta_exist[i])
				continue;

			sta = bb->phl_sta_info[i];

			if (!is_sta_active(sta))
				continue;

			sta_cnt ++;

			bb_path_div->path_sel[i] = bb_path_div->path_sel_1ss;
			halbb_set_tx_path(bb, i, bb_path_div->path_sel_1ss);

			if (sta_cnt == bb_link->num_linked_client)
				break;
		}
	} else {
		BB_DBG(bb, DBG_PATH_DIV, "[%s] Pause AUTO is invalid!\n", __func__);
	}
}

void halbb_update_tx_path_div(struct bb_info *bb, struct rtw_phl_stainfo_t *sta)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	u32 val = 0;

	if ((bb->ic_type != BB_RTL8852C) && (bb->ic_type != BB_RTL8852B)) {
		BB_DBG(bb, DBG_PATH_DIV, "[%s], Early return due to wrong ic type (not 52c or 52b)\n",  __func__);
		return;
	}

	if (bb->ic_type == BB_RTL8852B){
		if (bb->ic_sub_type != BB_IC_SUB_TYPE_8852B_8852BP) {
			BB_DBG(bb, DBG_PATH_DIV, "[%s], Early return due to wrong sub-ic type (not 52bp)\n",  __func__);
			return;
		}
	}

	if (bb->bb_80211spec == BB_AX_IC)
		halbb_pause_func(bb, F_PATH_DIV, HALBB_RESUME_NO_RECOVERY, HALBB_PAUSE_LV_1, 1, &val, bb->bb_phy_idx);

	BB_DBG(bb, DBG_PATH_DIV, "[%s], macid = %d, band = %d\n",  __func__, sta->macid, sta->chandef.band);
	if (sta->chandef.band == BAND_ON_6G) {
		halbb_ctrl_tx_path_div(bb, BB_PATH_AUTO);
		bb_path_div->path_sel[sta->macid] = BB_PATH_B;
		halbb_set_tx_path(bb, (u8)sta->macid, BB_PATH_B);
	} else {
		halbb_ctrl_tx_path_div(bb, BB_PATH_AB);
		halbb_set_tx_path(bb, (u8)sta->macid, BB_PATH_AB);
	}
}

#ifdef HALBB_COMPILE_AX_SERIOUS
void halbb_path_diversity_ax(struct bb_info *bb)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct bb_link_info *bb_link = &bb->bb_link_i;
	struct bb_pathdiv_rssi_info *rssi_stat = &bb_path_div->bb_rssi_i;
	struct rtw_phl_stainfo_t *sta;
	enum bb_path path = BB_PATH_A;
	u8 rssi_a = 0, rssi_b = 0;
	u8 mod_rssi_a = 0, mod_rssi_b = 0;
	u16 i = 0, sta_cnt = 0;
	u8 macid = 0, macid_cnt = 0;
	u64 macid_is_linked = 0, macid_mask = 0, macid_diff = 0;

	BB_DBG(bb, DBG_PATH_DIV, "%s ======>\n", __func__);

	for (i = 0; i < PHL_MAX_STA_NUM; i++) {
		if (!bb->sta_exist[i])
			continue;

		sta = bb->phl_sta_info[i];

		if (!is_sta_active(sta))
			continue;

		sta_cnt ++;

		macid = (u8)sta->macid;
		macid_mask = (u64)BIT(macid);
		bb_path_div->macid_is_linked |= macid_mask;
		macid_is_linked |= macid_mask;

		/* 2 Caculate RSSI per path */
		if (bb_path_div->rssi_decision_method == RSSI_LINEAR_AVG) {
			/* Modify db to linear (*10)*/
			rssi_stat->path_a_rssi_sum[macid] = HALBB_DIV_U64(rssi_stat->path_a_rssi_sum[macid], 10);
			rssi_stat->path_b_rssi_sum[macid] = HALBB_DIV_U64(rssi_stat->path_b_rssi_sum[macid], 10);

			if (rssi_stat->path_a_rssi_sum[macid] == 0)
				rssi_a = 0;
			else
				rssi_a = (u8)(halbb_convert_to_db(HALBB_DIV_U64(rssi_stat->path_a_rssi_sum[macid],
								 rssi_stat->path_a_pkt_cnt[macid])) << 1);
			if (rssi_stat->path_b_rssi_sum[macid] == 0)
				rssi_b = 0;
			else
				rssi_b = (u8)(halbb_convert_to_db(HALBB_DIV_U64(rssi_stat->path_b_rssi_sum[macid],
								 rssi_stat->path_b_pkt_cnt[macid])) << 1);
		} else {
			rssi_a = (u8)HALBB_DIV_U64(rssi_stat->path_a_rssi_sum[macid],
					       rssi_stat->path_a_pkt_cnt[macid]);
			rssi_b = (u8)HALBB_DIV_U64(rssi_stat->path_b_rssi_sum[macid],
					       rssi_stat->path_b_pkt_cnt[macid]);
		}

		/* 3 Add RSSI GAP per path to prevent damping*/
		if (bb_path_div->path_sel[macid] == BB_PATH_A) {
			mod_rssi_a = rssi_a + bb_path_div->path_rssi_gap;
			mod_rssi_b = rssi_b;
		} else if (bb_path_div->path_sel[macid] == BB_PATH_B){
			mod_rssi_a = rssi_a;
			mod_rssi_b = rssi_b + bb_path_div->path_rssi_gap;
		} else {
			mod_rssi_a = rssi_a;
			mod_rssi_b = rssi_b;
		}

		if (mod_rssi_a == mod_rssi_b)
			path = bb_path_div->path_sel[macid];
		else
			path = (mod_rssi_a > mod_rssi_b) ? BB_PATH_A : BB_PATH_B;

		BB_DBG(bb, DBG_PATH_DIV,
		       "STA[%d] : PathA sum=%lld, cnt=%d, avg_rssi=%d\n",
		       macid, rssi_stat->path_a_rssi_sum[macid],
		       rssi_stat->path_a_pkt_cnt[macid], rssi_a >> 1);
		BB_DBG(bb, DBG_PATH_DIV,
		       "STA[%d] : PathB sum=%lld, cnt=%d, avg_rssi=%d\n",
		       macid, rssi_stat->path_b_rssi_sum[macid],
		       rssi_stat->path_b_pkt_cnt[macid], rssi_b >> 1);
		BB_DBG(bb, DBG_PATH_DIV,
		       "path_rssi_gap=%d dB\n", bb_path_div->path_rssi_gap >> 1);

		if (!bb_path_div->fix_path_en[macid]) {
			if (bb_path_div->path_sel[macid] != path) {
				bb_path_div->path_sel[macid] = path;
				/* Update Tx path */
				halbb_set_tx_path(bb, macid, path);
				BB_DBG(bb, DBG_PATH_DIV, "Switch TX path= %s\n",
				       (path == BB_PATH_A) ? "A" : "B");
			} else {
				BB_DBG(bb, DBG_PATH_DIV, "Stay in TX path = %s\n",
				       (path == BB_PATH_A) ? "A" : "B");
			}
		} else {
			BB_DBG(bb, DBG_PATH_DIV, "Fix TX path= %s\n",
			       (bb_path_div->fix_path_sel[macid] == BB_PATH_A) ? "A" : "B");
		}

		rssi_stat->path_a_pkt_cnt[macid] = 0;
		rssi_stat->path_a_rssi_sum[macid] = 0;
		rssi_stat->path_b_pkt_cnt[macid] = 0;
		rssi_stat->path_b_rssi_sum[macid] = 0;

		if (sta_cnt == bb_link->num_linked_client)
			break;
	}

	macid_diff = bb_path_div->macid_is_linked ^ macid_is_linked;

	if (macid_diff)
		bb_path_div->macid_is_linked &= ~macid_diff;

	while (macid_diff) {
		if (macid_diff & 0x1) {
			if ((bb->ic_type == BB_RTL8852C) || (bb->ic_sub_type == BB_IC_SUB_TYPE_8852B_8852BP)) {
				bb_path_div->path_sel[macid_cnt] = BB_PATH_B;
				halbb_set_tx_path(bb, macid_cnt, BB_PATH_B);
			} else {
				bb_path_div->path_sel[macid_cnt] = BB_PATH_A;
				halbb_set_tx_path(bb, macid_cnt, BB_PATH_A);
			}
			BB_DBG(bb, DBG_PATH_DIV, "STA[%d] is disconnect\n",
			       macid_cnt);
		}
		macid_cnt++;
		macid_diff >>= 1;
	}

	BB_DBG(bb, DBG_PATH_DIV, "[%s] end\n\n", __func__);
}

void halbb_pathdiv_phy_sts_ax(struct bb_info *bb, struct physts_rxd *desc)
{
	struct bb_physts_rslt_hdr_info	*psts_h = &bb->bb_physts_i.bb_physts_rslt_hdr_i;
	struct bb_cmn_rpt_info	*cmn_rpt = &bb->bb_cmn_rpt_i;
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct bb_pathdiv_rssi_info *rssi = &bb_path_div->bb_rssi_i;
	struct dev_cap_t *dev = &bb->phl_com->dev_cap;
	struct rtw_phl_stainfo_t *sta;
	u16 macid = 0, bb_macid = 0;
	u64 max_value = 0xffffffffffffffff;

	if (cmn_rpt->is_cck_rate)
		return;

	if (desc->macid_su >= PHL_MAX_STA_NUM) {
		BB_WARNING("[%s] macid_su=%d\n", __func__, desc->macid_su);
		return;
	}

	bb_macid = bb->phl2bb_macid_table[desc->macid_su];

	if (bb_macid >= PHL_MAX_STA_NUM) {
		BB_WARNING("[%s] bb_macid=%d\n", __func__, bb_macid);
		return;
	}

	sta = bb->phl_sta_info[bb_macid];

	if (!is_sta_active(sta))
		return;

	if (sta->macid >= PHL_MAX_STA_NUM)
		return;

	if (!sta->hal_sta)
		return;

	if ((dev->rfe_type >= 50) && (bb_macid == 0)) /* No need to cnt AP Rx boardcast pkt*/
		return;

	macid = desc->macid_su;

	if (bb_path_div->rssi_decision_method == RSSI_LINEAR_AVG) {
		if (halbb_db_2_linear(psts_h->rssi[0] >> 1) <= (max_value - rssi->path_a_rssi_sum[macid])) {
			rssi->path_a_rssi_sum[macid] += halbb_db_2_linear(psts_h->rssi[0] >> 1);
			rssi->path_a_pkt_cnt[macid]++;
		}
		if (halbb_db_2_linear(psts_h->rssi[1] >> 1) <= (max_value - rssi->path_b_rssi_sum[macid])) {
			rssi->path_b_rssi_sum[macid] += halbb_db_2_linear(psts_h->rssi[1] >> 1);
			rssi->path_b_pkt_cnt[macid]++;
		}
	} else {
		rssi->path_a_rssi_sum[macid] += psts_h->rssi[0];
		rssi->path_a_pkt_cnt[macid]++;

		rssi->path_b_rssi_sum[macid] += psts_h->rssi[1];
		rssi->path_b_pkt_cnt[macid]++;
	}
}
#endif

#ifdef HALBB_COMPILE_BE_SERIES
void halbb_pathdiv_phy_sts_be(struct bb_info *bb, struct physts_rxd *desc)
{
	struct bb_physts_rslt_hdr_info	*psts_h = &bb->bb_physts_i.bb_physts_rslt_hdr_i;
	struct bb_cmn_rpt_info	*cmn_rpt = &bb->bb_cmn_rpt_i;
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	//struct bb_pathdiv_rssi_info *rssi = &bb_path_div->bb_rssi_i;
	struct dev_cap_t *dev = &bb->phl_com->dev_cap;
	struct rtw_phl_stainfo_t *sta;
	u16 macid = 0, bb_macid = 0;
	u64 max_value = 0xffffffffffffffff;

	if (cmn_rpt->is_cck_rate)
		return;

	if (desc->macid_su >= PHL_MAX_STA_NUM) {
		BB_WARNING("[%s] macid_su=%d\n", __func__, desc->macid_su);
		return;
	}

	bb_macid = bb->phl2bb_macid_table[desc->macid_su];

	if (bb_macid >= PHL_MAX_STA_NUM) {
		BB_WARNING("[%s] bb_macid=%d\n", __func__, bb_macid);
		return;
	}

	sta = bb->phl_sta_info[bb_macid];

	if (!is_sta_active(sta))
		return;

	if (sta->macid >= PHL_MAX_STA_NUM)
		return;

	if (!sta->hal_sta)
		return;

	if ((dev->rfe_type >= 50) && (bb_macid == 0)) /* No need to cnt AP Rx boardcast pkt*/
		return;

	macid = desc->macid_su;

#if 0
	if (bb_path_div->rssi_decision_method == RSSI_LINEAR_AVG) {
		if (halbb_db_2_linear(psts_h->rssi[0] >> 1) <= (max_value - rssi->path_a_rssi_sum[macid])) {
			rssi->path_a_rssi_sum[macid] += halbb_db_2_linear(psts_h->rssi[0] >> 1);
			rssi->path_a_pkt_cnt[macid]++;
		}
		if (halbb_db_2_linear(psts_h->rssi[1] >> 1) <= (max_value - rssi->path_b_rssi_sum[macid])) {
			rssi->path_b_rssi_sum[macid] += halbb_db_2_linear(psts_h->rssi[1] >> 1);
			rssi->path_b_pkt_cnt[macid]++;
		}
		if (halbb_db_2_linear(psts_h->rssi[2] >> 1) <= (max_value - rssi->path_c_rssi_sum[macid])) {
			rssi->path_c_rssi_sum[macid] += halbb_db_2_linear(psts_h->rssi[2] >> 1);
			rssi->path_c_pkt_cnt[macid]++;
		}
		if (halbb_db_2_linear(psts_h->rssi[3] >> 1) <= (max_value - rssi->path_d_rssi_sum[macid])) {
			rssi->path_d_rssi_sum[macid] += halbb_db_2_linear(psts_h->rssi[3] >> 1);
			rssi->path_d_pkt_cnt[macid]++;
		}
	} else {
		rssi->path_a_rssi_sum[macid] += psts_h->rssi[0];
		rssi->path_a_pkt_cnt[macid]++;

		rssi->path_b_rssi_sum[macid] += psts_h->rssi[1];
		rssi->path_b_pkt_cnt[macid]++;

		rssi->path_c_rssi_sum[macid] += psts_h->rssi[2];
		rssi->path_c_pkt_cnt[macid]++;

		rssi->path_d_rssi_sum[macid] += psts_h->rssi[3];
		rssi->path_d_pkt_cnt[macid]++;
	}
#endif
}

void halbb_find_default_path(struct bb_info *bb, u8 macid)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	//struct bb_pathdiv_rssi_info *rssi = &bb_path_div->bb_rssi_i;
	struct rtw_phl_stainfo_t *sta;
	struct rtw_rssi_info *sta_rssi = NULL;
	u8 i = 0, min_rssi_path = 0, second_last_rssi_path= 0;
	u8 rssi_a = 0, rssi_b = 0, rssi_c = 0, rssi_d = 0, rssi_other_avg = 0;
	u8 mod_rssi_a = 0, mod_rssi_b = 0;
	u8 rssi_tmp[4] = {0};

	bb_path_div->path_mask = 0;
	sta = bb->phl_sta_info[macid];
	sta_rssi = &sta->hal_sta->rssi_stat;

	/* Default path Selection By RSSI */

	rssi_a = (sta_rssi->rssi_ma_path[0] >> 5) & 0x7f;
	rssi_b = (sta_rssi->rssi_ma_path[1] >> 5) & 0x7f;
	rssi_c = (sta_rssi->rssi_ma_path[2] >> 5) & 0x7f;
	rssi_d = (sta_rssi->rssi_ma_path[3] >> 5) & 0x7f;

	BB_DBG(bb, DBG_PATH_DIV,
	       "STA[%d] : Path {A, B, C, D} avg_rssi = {%d, %d, %d, %d}\n",
	       macid, rssi_a, rssi_b, rssi_c, rssi_d);

#if 0
	if (bb_path_div->rssi_decision_method == RSSI_LINEAR_AVG) {
		/* Modify db to linear (*10)*/
		rssi->path_a_rssi_sum[macid] = HALBB_DIV_U64(rssi->path_a_rssi_sum[macid], 10);
		rssi->path_b_rssi_sum[macid] = HALBB_DIV_U64(rssi->path_b_rssi_sum[macid], 10);

		if (rssi->path_a_rssi_sum[macid] == 0)
			rssi_a = 0;
		else
			rssi_a = (u8)(halbb_convert_to_db(HALBB_DIV_U64(rssi->path_a_rssi_sum[macid],
							  rssi->path_a_pkt_cnt[macid])) << 1);
		if (rssi->path_b_rssi_sum[macid] == 0)
			rssi_b = 0;
		else
			rssi_b = (u8)(halbb_convert_to_db(HALBB_DIV_U64(rssi->path_b_rssi_sum[macid],
							  rssi->path_b_pkt_cnt[macid])) << 1);
		if (rssi->path_c_rssi_sum[macid] == 0)
			rssi_c = 0;
		else
			rssi_c = (u8)(halbb_convert_to_db(HALBB_DIV_U64(rssi->path_c_rssi_sum[macid],
							  rssi->path_c_pkt_cnt[macid])) << 1);
		if (rssi->path_d_rssi_sum[macid] == 0)
			rssi_d = 0;
		else
			rssi_d = (u8)(halbb_convert_to_db(HALBB_DIV_U64(rssi->path_d_rssi_sum[macid],
							  rssi->path_d_pkt_cnt[macid])) << 1);
	} else {
		rssi_a = (u8)HALBB_DIV_U64(rssi->path_a_rssi_sum[macid],
				           rssi->path_a_pkt_cnt[macid]);
		rssi_b = (u8)HALBB_DIV_U64(rssi->path_b_rssi_sum[macid],
				           rssi->path_b_pkt_cnt[macid]);
		rssi_c = (u8)HALBB_DIV_U64(rssi->path_c_rssi_sum[macid],
				           rssi->path_c_pkt_cnt[macid]);
		rssi_d = (u8)HALBB_DIV_U64(rssi->path_d_rssi_sum[macid],
				           rssi->path_d_pkt_cnt[macid]);
	}

	BB_DBG(bb, DBG_PATH_DIV,
	       "STA[%d] : PathA sum=%lld, cnt=%d, avg_rssi=%d\n",
	       macid, rssi->path_a_rssi_sum[macid],
	       rssi->path_a_pkt_cnt[macid], rssi_a >> 1);
	BB_DBG(bb, DBG_PATH_DIV,
	       "STA[%d] : PathB sum=%lld, cnt=%d, avg_rssi=%d\n",
	       macid, rssi->path_b_rssi_sum[macid],
	       rssi->path_b_pkt_cnt[macid], rssi_b >> 1);
	BB_DBG(bb, DBG_PATH_DIV,
	       "STA[%d] : PathC sum=%lld, cnt=%d, avg_rssi=%d\n",
	       macid, rssi->path_c_rssi_sum[macid],
	       rssi->path_c_pkt_cnt[macid], rssi_c >> 1);
	BB_DBG(bb, DBG_PATH_DIV,
	       "STA[%d] : PathD sum=%lld, cnt=%d, avg_rssi=%d\n",
	       macid, rssi->path_d_rssi_sum[macid],
	       rssi->path_d_pkt_cnt[macid], rssi_d >> 1);
#endif


	if (bb->num_rf_path == 4)
		rssi_other_avg = (rssi_b + rssi_c + rssi_d) / 3;
	else if (bb->num_rf_path == 3)
		rssi_other_avg = (rssi_b + rssi_c) / 2;
	else
		rssi_other_avg = rssi_b;

	if (bb_path_div->use_path_a_as_default_ant == 1) {
		if (rssi_a > rssi_other_avg + bb_path_div->path_rssi_gap) {
			bb_path_div->is_path_a_exist = true;
			bb_path_div->default_path = BB_PATH_A;
			min_rssi_path = 1; // Exclude path A
			second_last_rssi_path = 1; // Exclude path A
		} else {
			bb_path_div->is_path_a_exist = false;
			if ((rssi_b >= rssi_c) && (rssi_b >= rssi_d))
				bb_path_div->default_path = BB_PATH_B;
			else if (rssi_c >= rssi_d)
				bb_path_div->default_path = BB_PATH_C;
			else
				bb_path_div->default_path = BB_PATH_D;
		}
	} else {
		if (rssi_a >= rssi_b && rssi_a >= rssi_c && rssi_a >= rssi_d)
			bb_path_div->default_path = BB_PATH_A;
		else if ((rssi_b >= rssi_c) && (rssi_b >= rssi_d))
			bb_path_div->default_path = BB_PATH_B;
		else if (rssi_c >= rssi_d)
			bb_path_div->default_path = BB_PATH_C;
		else
			bb_path_div->default_path = BB_PATH_D;
	}

	/* For 1T2R case */
	if (bb_path_div->num_tx_path == 1 && bb->num_rf_path == 2) {
		if (bb_path_div->path_sel[macid] == BB_PATH_A) {
			mod_rssi_a = rssi_a + bb_path_div->path_rssi_gap;
			mod_rssi_b = rssi_b;
		} else if (bb_path_div->path_sel[macid] == BB_PATH_B){
			mod_rssi_a = rssi_a;
			mod_rssi_b = rssi_b + bb_path_div->path_rssi_gap;
		} else {
			mod_rssi_a = rssi_a;
			mod_rssi_b = rssi_b;
		}

		if (mod_rssi_a == mod_rssi_b)
			bb_path_div->default_path = bb_path_div->path_sel[macid];
		else
			bb_path_div->default_path = (mod_rssi_a > mod_rssi_b) ? BB_PATH_A : BB_PATH_B;
	}

	rssi_tmp[0] = rssi_a;
	rssi_tmp[1] = rssi_b;
	rssi_tmp[2] = rssi_c;
	rssi_tmp[3] = rssi_d;

	for (i = 1; i < HALBB_MAX_PATH; i++) {
		if (rssi_tmp[i] < rssi_tmp[min_rssi_path])
			min_rssi_path = i;
	}

	bb_path_div->path_mask |= BIT(min_rssi_path);

	if (bb_path_div->num_tx_path == 2) {
		for (i = 1; i < HALBB_MAX_PATH; i++) {
			if (rssi_tmp[i] < rssi_tmp[second_last_rssi_path] &&
			    i != min_rssi_path)
				second_last_rssi_path = i;
		}

		bb_path_div->path_mask |= BIT(second_last_rssi_path);
	}

	BB_DBG(bb, DBG_PATH_DIV, "default_path = 0x%x, path_mask = 0x%x\n",
	       bb_path_div->default_path, bb_path_div->path_mask);
}

void halbb_path_sel_update(struct bb_info *bb, u8 macid)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	//struct bb_pathdiv_rssi_info *rssi = &bb_path_div->bb_rssi_i;
	enum bb_path path = bb_path_div->path_sel[macid];
	enum bb_path max_tx_path = halbb_gen_mask_from_0(bb->num_rf_path);
	u8 i = 0, max_rssi = 0, max_rssi_path = 0;

	bb_path_div->path_sel[macid] = max_tx_path;

	if (bb_path_div->num_tx_path > 1) { /* @3 or 2 TX mode */
		bb_path_div->path_sel[macid] &= ~(bb_path_div->path_mask);
		BB_DBG(bb, DBG_PATH_DIV, "STA[%d] : Choose path_sel path = 0x%x\n",
		       macid, bb_path_div->path_sel[macid]);
	} else { /* @1 TX mode */
		bb_path_div->path_sel[macid] = bb_path_div->default_path;
		BB_DBG(bb, DBG_PATH_DIV, "STA[%d] : Choose default path = 0x%x\n",
		       macid, bb_path_div->default_path);
	}
}

void halbb_path_diversity_be(struct bb_info *bb)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;
	struct bb_link_info *bb_link = &bb->bb_link_i;
	//struct bb_pathdiv_rssi_info *rssi = &bb_path_div->bb_rssi_i;
	struct rtw_phl_stainfo_t *sta;
	enum bb_path max_tx_path = halbb_gen_mask_from_0(bb->num_rf_path);
	u8 i = 0, sta_cnt = 0;
	u8 macid = 0, macid_cnt = 0;
	u64 macid_is_linked = 0, macid_mask = 0, macid_diff = 0;

	BB_DBG(bb, DBG_PATH_DIV, "%s ======>\n", __func__);

	for (i = 0; i < PHL_MAX_STA_NUM; i++) {
		if (!bb->sta_exist[i])
			continue;

		sta = bb->phl_sta_info[i];

		if (!is_sta_active(sta))
			continue;

		sta_cnt ++;

		macid = (u8)sta->macid;
		macid_mask = (u64)BIT(macid);
		bb_path_div->macid_is_linked |= macid_mask;
		macid_is_linked |= macid_mask;

		bb_path_div->pre_tx_path = bb_path_div->path_sel[macid];

		halbb_find_default_path(bb, macid);
		halbb_path_sel_update(bb, macid);

		if (!bb_path_div->fix_path_en[macid]) {
			if (bb_path_div->path_sel[macid] != bb_path_div->pre_tx_path) {
				bb_path_div->pre_tx_path = bb_path_div->path_sel[macid];
				/* Update Tx path */
				halbb_set_tx_path(bb, macid, bb_path_div->path_sel[macid]);
				BB_DBG(bb, DBG_PATH_DIV, "STA[%d] : Switch TX path= [0x%x]\n",
				       macid, bb_path_div->path_sel[macid]);
			} else {
				BB_DBG(bb, DBG_PATH_DIV, "STA[%d] : Stay in TX path = [0x%x]\n",
				       macid, bb_path_div->pre_tx_path);
			}
		} else {
			BB_DBG(bb, DBG_PATH_DIV, "Fix TX path= [0x%x]\n",
			       bb_path_div->fix_path_sel[macid]);
		}
		#if 0
		rssi->path_a_pkt_cnt[macid] = 0;
		rssi->path_a_rssi_sum[macid] = 0;
		rssi->path_b_pkt_cnt[macid] = 0;
		rssi->path_b_rssi_sum[macid] = 0;
		rssi->path_c_pkt_cnt[macid] = 0;
		rssi->path_c_rssi_sum[macid] = 0;
		rssi->path_d_pkt_cnt[macid] = 0;
		rssi->path_d_rssi_sum[macid] = 0;
		#endif

		if (sta_cnt == bb_link->num_linked_client)
			break;
	}

	macid_diff = bb_path_div->macid_is_linked ^ macid_is_linked;

	if (macid_diff)
		bb_path_div->macid_is_linked &= ~macid_diff;

	while (macid_diff) {
		if (macid_diff & 0x1) {
			bb_path_div->path_sel[macid_cnt] = max_tx_path;
			halbb_set_tx_path(bb, macid_cnt, max_tx_path);
			BB_DBG(bb, DBG_PATH_DIV, "STA[%d] is disconnect\n",
			       macid_cnt);
		}
		macid_cnt++;
		macid_diff >>= 1;
	}
}
#endif

void halbb_pathdiv_phy_sts(struct bb_info *bb, struct physts_rxd *desc)
{
	switch (bb->bb_80211spec) {

	#ifdef HALBB_COMPILE_AX_SERIOUS
	case BB_AX_IC:
		halbb_pathdiv_phy_sts_ax(bb, desc);
		break;
	#endif

	#ifdef HALBB_COMPILE_BE_SERIES
	case BB_BE_IC:
		halbb_pathdiv_phy_sts_be(bb, desc);
		break;
	#endif
	default:
		break;
	}
}

void halbb_path_diversity(struct bb_info *bb)
{
	if (halbb_pathdiv_abort(bb)) {
		return;
	}

	switch (bb->bb_80211spec) {

	#ifdef HALBB_COMPILE_AX_SERIOUS
	case BB_AX_IC:
		halbb_path_diversity_ax(bb);
		break;
	#endif

	#ifdef HALBB_COMPILE_BE_SERIES
	case BB_BE_IC:
		halbb_path_diversity_be(bb);
		break;
	#endif
	default:
		break;
	}
}

void halbb_pathdiv_dbg(struct bb_info *bb, char input[][16], u32 *_used,
			      char *output, u32 *_out_len)
{
	struct bb_pathdiv_info *bb_path_div = &bb->bb_path_div_i;

	char help[] = "-h";
	u8 macid = 0;
	u32 var[10] = {0};
	u32 used = *_used;
	u32 out_len = *_out_len;

	if ((_os_strcmp(input[1], help) == 0)) {
		BB_DBG_CNSL(out_len, used, output + used, out_len - used,
			    "Fix CMAC TX path Mode: {1} {en} {macid} {path(1/2)}\n");
		BB_DBG_CNSL(out_len, used, output + used, out_len - used,
			    "RSSI Gap dbg mode: {2} {path rssi gap(1:0.5dB)}\n");
		BB_DBG_CNSL(out_len, used, output + used, out_len - used,
			    "RSSI decision method: {3} {LINEAR_AVG(0), DB_AVG(1)}\n");
	} else {
		HALBB_SCAN(input[1], DCMD_DECIMAL, &var[0]);

		if (var[0] == 1) {
			HALBB_SCAN(input[2], DCMD_DECIMAL, &var[1]);
			HALBB_SCAN(input[3], DCMD_DECIMAL, &var[2]);
			HALBB_SCAN(input[4], DCMD_DECIMAL, &var[3]);
			bb_path_div->fix_path_en[macid] = (u8)var[1];
			macid = (u8)var[2];
			bb_path_div->fix_path_sel[macid] = (enum bb_path)var[3];
			halbb_set_tx_path_by_cmac_tbl(bb, macid, (enum bb_path)var[3]);
			BB_DBG_CNSL(out_len, used, output + used, out_len - used,
				    "Fix STA[%d] path= %s\n", macid,
				    (bb_path_div->fix_path_sel[macid] == BB_PATH_A) ? "A" : "B");
		} else if (var[0] == 2) {
			HALBB_SCAN(input[2], DCMD_DECIMAL, &var[1]);
			bb_path_div->path_rssi_gap = (u8)var[1];
			BB_DBG_CNSL(out_len, used, output + used, out_len - used,
				    "path rssi gap = %d dB\n", var[1] >> 1);
		} else if (var[0] == 3) {
			HALBB_SCAN(input[2], DCMD_DECIMAL, &var[1]);
			bb_path_div->rssi_decision_method = (u8)var[1];
			BB_DBG_CNSL(out_len, used, output + used, out_len - used,
				    "rssi_decision_method = %s\n",
				    (bb_path_div->rssi_decision_method == RSSI_LINEAR_AVG) ? "Linear" : "dB");
		}
	}

	*_used = used;
	*_out_len = out_len;
}

void halbb_cr_cfg_pathdiv_init(struct bb_info *bb)
{
	//struct bb_pathdiv_cr_info *cr = &bb->bb_ant_div_i.bb_antdiv_cr_i;

	switch (bb->cr_type) {

	#ifdef BB_8852A_CAV_SUPPORT
	case BB_52AA:
		break;

	#endif
	#ifdef HALBB_COMPILE_AP_SERIES
	case BB_AP:
		break;

	#endif
	#ifdef HALBB_COMPILE_CLIENT_SERIES
	case BB_CLIENT:
		break;
	#endif

	default:
		break;
	}

}
#endif

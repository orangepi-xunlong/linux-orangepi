/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
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
 *****************************************************************************/
#define _HAL_TEST_MP_TX_C_
#include "../../hal_headers.h"
#include "../../../test/mp/phl_test_mp_def.h"
#include "../../../test/mp/phl_test_mp_watchdog.h"

#ifdef CONFIG_HAL_TEST_MP

void
_mp_get_plcp_common_info(struct mp_context *mp,
                            struct mp_tx_arg *arg,
                            struct halbb_plcp_info *plcp_tx_struct)
{
	/*unsigned long offset1 = (unsigned long)(&(((struct mp_tx_arg *)0)->dbw));
	unsigned long offset2 = (unsigned long)(&(((struct mp_tx_arg *)0)->tb_rsvd));*/
	u8 i = 0;

	PHL_INFO("%s=============================\n",__FUNCTION__);
	PHL_INFO("%s arg->dbw = %d\n",__FUNCTION__, arg->dbw);
	PHL_INFO("%s arg->source_gen_mode = %d\n",__FUNCTION__, arg->source_gen_mode);
	PHL_INFO("%s arg->locked_clk = %d\n",__FUNCTION__, arg->locked_clk);
	PHL_INFO("%s arg->dyn_bw = %d\n",__FUNCTION__, arg->dyn_bw);
	PHL_INFO("%s arg->ndp_en = %d\n",__FUNCTION__, arg->ndp_en);
	PHL_INFO("%s arg->long_preamble_en = %d\n",__FUNCTION__, arg->long_preamble_en);
	PHL_INFO("%s arg->stbc = %d\n",__FUNCTION__, arg->stbc);
	PHL_INFO("%s arg->gi = %d\n",__FUNCTION__, arg->gi);
	PHL_INFO("%s arg->tb_l_len = %d\n",__FUNCTION__, arg->tb_l_len);
	PHL_INFO("%s arg->tb_ru_tot_sts_max = %d\n",__FUNCTION__, arg->tb_ru_tot_sts_max);
	PHL_INFO("%s arg->vht_txop_not_allowed = %d\n",__FUNCTION__, arg->vht_txop_not_allowed);
	PHL_INFO("%s arg->tb_disam = %d\n",__FUNCTION__, arg->tb_disam);
	PHL_INFO("%s arg->doppler = %d\n",__FUNCTION__, arg->doppler);
	PHL_INFO("%s arg->he_ltf_type = %d\n",__FUNCTION__, arg->he_ltf_type);
	PHL_INFO("%s arg->ht_l_len = %d\n",__FUNCTION__, arg->ht_l_len);
	PHL_INFO("%s arg->preamble_puncture = %d\n",__FUNCTION__, arg->preamble_puncture);
	PHL_INFO("%s arg->he_mcs_sigb = %d\n",__FUNCTION__, arg->he_mcs_sigb);
	PHL_INFO("%s arg->he_dcm_sigb = %d\n",__FUNCTION__, arg->he_dcm_sigb);
	PHL_INFO("%s arg->he_sigb_compress_en = %d\n",__FUNCTION__, arg->he_sigb_compress_en);
	PHL_INFO("%s arg->max_tx_time_0p4us = %d\n",__FUNCTION__, arg->max_tx_time_0p4us);
	PHL_INFO("%s arg->ul_flag = %d\n",__FUNCTION__, arg->ul_flag);
	PHL_INFO("%s arg->tb_ldpc_extra = %d\n",__FUNCTION__, arg->tb_ldpc_extra);
	PHL_INFO("%s arg->bss_color = %d\n",__FUNCTION__, arg->bss_color);
	PHL_INFO("%s arg->sr = %d\n",__FUNCTION__, arg->sr);
	PHL_INFO("%s arg->beamchange_en = %d\n",__FUNCTION__, arg->beamchange_en);
	PHL_INFO("%s arg->he_er_u106ru_en = %d\n",__FUNCTION__, arg->he_er_u106ru_en);
	PHL_INFO("%s arg->ul_srp1 = %d\n",__FUNCTION__, arg->ul_srp1);
	PHL_INFO("%s arg->ul_srp2 = %d\n",__FUNCTION__, arg->ul_srp2);
	PHL_INFO("%s arg->ul_srp3 = %d\n",__FUNCTION__, arg->ul_srp3);
	PHL_INFO("%s arg->ul_srp4 = %d\n",__FUNCTION__, arg->ul_srp4);
	PHL_INFO("%s arg->mode = %d\n",__FUNCTION__, arg->mode);
	PHL_INFO("%s arg->group_id = %d\n",__FUNCTION__, arg->group_id);
	PHL_INFO("%s arg->ppdu_type = %d\n",__FUNCTION__, arg->ppdu_type);
	PHL_INFO("%s arg->txop = %d\n",__FUNCTION__, arg->txop);
	PHL_INFO("%s arg->tb_strt_sts = %d\n",__FUNCTION__, arg->tb_strt_sts);
	PHL_INFO("%s arg->tb_pre_fec_padding_factor = %d\n",__FUNCTION__, arg->tb_pre_fec_padding_factor);
	PHL_INFO("%s arg->cbw = %d\n",__FUNCTION__, arg->cbw);
	PHL_INFO("%s arg->txsc = %d\n",__FUNCTION__, arg->txsc);
	PHL_INFO("%s arg->tb_mumimo_mode_en = %d\n",__FUNCTION__, arg->tb_mumimo_mode_en);
	PHL_INFO("%s arg->nominal_t_pe = %d\n",__FUNCTION__, arg->nominal_t_pe);
	PHL_INFO("%s arg->ness = %d\n",__FUNCTION__, arg->ness);
	PHL_INFO("%s arg->n_user = %d\n",__FUNCTION__, arg->n_user);
	PHL_INFO("%s arg->tb_rsvd = %d\n",__FUNCTION__, arg->tb_rsvd);
	PHL_INFO("%s=============================\n",__FUNCTION__);
	plcp_tx_struct->dbw = (u8)arg->dbw;
	plcp_tx_struct->source_gen_mode = (u8)arg->source_gen_mode;
	plcp_tx_struct->locked_clk = (u8)arg->locked_clk;
	plcp_tx_struct->dyn_bw = (u8)arg->dyn_bw;
	plcp_tx_struct->ndp_en = (u8)arg->ndp_en;
	plcp_tx_struct->long_preamble_en = (u8)arg->long_preamble_en;
	plcp_tx_struct->stbc = (u8)arg->stbc;
	plcp_tx_struct->gi = (u8)arg->gi;
	plcp_tx_struct->tb_l_len = (u16)arg->tb_l_len;
	plcp_tx_struct->tb_ru_tot_sts_max = (u8)arg->tb_ru_tot_sts_max;
	plcp_tx_struct->vht_txop_not_allowed = (u8)arg->vht_txop_not_allowed;
	plcp_tx_struct->tb_disam = (u8)arg->tb_disam;
	plcp_tx_struct->doppler = (u8)arg->doppler;
	plcp_tx_struct->he_ltf_type = (u8)arg->he_ltf_type;
	plcp_tx_struct->ht_l_len = (u16)arg->ht_l_len;
	plcp_tx_struct->preamble_puncture = (u8)arg->preamble_puncture;
	plcp_tx_struct->he_mcs_sigb = (u8)arg->he_mcs_sigb;
	plcp_tx_struct->he_dcm_sigb = (u8)arg->he_dcm_sigb;
	plcp_tx_struct->he_sigb_compress_en = (u8)arg->he_sigb_compress_en;
	plcp_tx_struct->max_tx_time_0p4us = (u8)arg->max_tx_time_0p4us;
	plcp_tx_struct->ul_flag = (u8)arg->ul_flag;
	plcp_tx_struct->tb_ldpc_extra = (u8)arg->tb_ldpc_extra;
	plcp_tx_struct->bss_color = (u8)arg->bss_color;
	plcp_tx_struct->sr = (u8)arg->sr;
	plcp_tx_struct->beamchange_en = (u8)arg->beamchange_en;
	plcp_tx_struct->he_er_u106ru_en = (u8)arg->he_er_u106ru_en;
	plcp_tx_struct->ul_srp1 = (u8)arg->ul_srp1;
	plcp_tx_struct->ul_srp2 = (u8)arg->ul_srp2;
	plcp_tx_struct->ul_srp3 = (u8)arg->ul_srp3;
	plcp_tx_struct->ul_srp4 = (u8)arg->ul_srp4;
	plcp_tx_struct->mode = (u8)arg->mode;
	plcp_tx_struct->group_id = (u8)arg->group_id;
	plcp_tx_struct->ppdu_type = (u8)arg->ppdu_type;
	plcp_tx_struct->txop = (u8)arg->txop;
	plcp_tx_struct->tb_strt_sts = (u8)arg->tb_strt_sts;
	plcp_tx_struct->tb_pre_fec_padding_factor = (u8)arg->tb_pre_fec_padding_factor;
	plcp_tx_struct->cbw = (u8)arg->cbw;
	plcp_tx_struct->txsc = (u8)arg->txsc;
	plcp_tx_struct->tb_mumimo_mode_en = (u8)arg->tb_mumimo_mode_en;
	plcp_tx_struct->nominal_t_pe = (u8)arg->nominal_t_pe;
	plcp_tx_struct->ness = (u8)arg->ness;
	plcp_tx_struct->n_user = (u8)arg->n_user;
	plcp_tx_struct->tb_rsvd = (u16)arg->tb_rsvd;
	plcp_tx_struct->txsb = (u8)arg->txsb;
	plcp_tx_struct->punc_pattern = (u8)arg->puncture;
	plcp_tx_struct->eht_mcs_sig = (u8)arg->eht_mcs_sig;

#if 0
	//copy common info
	_os_mem_cpy(mp->phl_com->drv_priv,(void*)(plcp_tx_struct),(void*)((unsigned long)arg+offset1),(offset2-offset1));
#endif

	//copy user info

	plcp_tx_struct->usr[0].mcs = (u8)mp->usr[0].mcs;
	plcp_tx_struct->usr[0].mpdu_len = (u16)mp->usr[0].mpdu_len;
	plcp_tx_struct->usr[0].n_mpdu = (u16)mp->usr[0].n_mpdu;
	plcp_tx_struct->usr[0].fec = (u8)mp->usr[0].fec;
	plcp_tx_struct->usr[0].dcm = (u8)mp->usr[0].dcm;
	plcp_tx_struct->usr[0].aid = (u16)mp->usr[0].aid;
	plcp_tx_struct->usr[0].scrambler_seed = (u8)mp->usr[0].scrambler_seed;
	plcp_tx_struct->usr[0].random_init_seed = (u8)mp->usr[0].random_init_seed;
	plcp_tx_struct->usr[0].apep = (u8)mp->usr[0].apep;
	plcp_tx_struct->usr[0].ru_alloc = (u8)mp->usr[0].ru_alloc;
	plcp_tx_struct->usr[0].nss = (u8)mp->usr[0].nss;
	plcp_tx_struct->usr[0].txbf = (u8)mp->usr[0].txbf;
	plcp_tx_struct->usr[0].pwr_boost_db = (u8)mp->usr[0].pwr_boost_db;
	plcp_tx_struct->usr[0].ru_size = (u8)mp->usr[0].ru_size;
	plcp_tx_struct->usr[0].ru_idx = (u8)mp->usr[0].ru_idx;


	for(i = 0; i < 4; i++) {
		PHL_INFO("%s=============================\n",__FUNCTION__);
		PHL_INFO("%s plcp_usr_idx = %d\n",__FUNCTION__, i);
		PHL_INFO("%s mcs = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].mcs);
		PHL_INFO("%s mpdu_len = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].mpdu_len);
		PHL_INFO("%s n_mpdu = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].n_mpdu);
		PHL_INFO("%s fec = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].fec);
		PHL_INFO("%s dcm = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].dcm);
		PHL_INFO("%s aid = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].aid);
		PHL_INFO("%s scrambler_seed = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].scrambler_seed);
		PHL_INFO("%s random_init_seed = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].random_init_seed);
		PHL_INFO("%s apep = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].apep);
		PHL_INFO("%s ru_alloc = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].ru_alloc);
		PHL_INFO("%s nss = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].nss);
		PHL_INFO("%s txbf = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].txbf);
		PHL_INFO("%s pwr_boost_db = %d\n",__FUNCTION__, plcp_tx_struct->usr[i].pwr_boost_db);
		PHL_INFO("%s=============================\n",__FUNCTION__);
	}
}

void
_mp_get_plcp_user_info(struct mp_context *mp,
                        struct usr_plcp_gen_in *plcp_user_struct)
{
	u8 i = 0;
	for (i = 0; i < 4; i++) {
		plcp_user_struct[i].mcs = (u8)mp->usr[i].mcs;
		plcp_user_struct[i].mpdu_len = (u16)mp->usr[i].mpdu_len;
		plcp_user_struct[i].n_mpdu = (u16)mp->usr[i].n_mpdu;
		plcp_user_struct[i].fec = (u8)mp->usr[i].fec;
		plcp_user_struct[i].dcm = (u8)mp->usr[i].dcm;
		plcp_user_struct[i].aid = (u16)mp->usr[i].aid;
		plcp_user_struct[i].scrambler_seed = (u8)mp->usr[i].scrambler_seed;
		plcp_user_struct[i].random_init_seed = (u8)mp->usr[i].random_init_seed;
		plcp_user_struct[i].apep = (u32)mp->usr[i].apep;
		plcp_user_struct[i].ru_alloc = (u8)mp->usr[i].ru_alloc;
		plcp_user_struct[i].nss = (u8)mp->usr[i].nss;
		plcp_user_struct[i].txbf = (u8)mp->usr[i].txbf;
		plcp_user_struct[i].pwr_boost_db = (u8)mp->usr[i].pwr_boost_db;
		plcp_user_struct[i].ru_size = (u8)mp->usr[i].ru_size;
		plcp_user_struct[i].ru_idx = (u8)mp->usr[i].ru_idx;
	}
}

enum rtw_hal_status rtw_hal_mp_tx_plcp_gen(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_info_t *hal_info = (struct hal_info_t *)mp->hal;
	struct halbb_plcp_info plcp_tx_struct = {0};
	struct usr_plcp_gen_in plcp_user_struct[4] = {0};

	_mp_get_plcp_common_info(mp, arg, &plcp_tx_struct);
	_mp_get_plcp_user_info(mp, plcp_user_struct);

	hal_status = rtw_hal_bb_set_plcp_tx(hal_info->hal_com,
										&plcp_tx_struct,
										plcp_user_struct,
										mp->cur_phy,
										&arg->plcp_sts);
	PHL_INFO("%s: phy index: %d\n", __FUNCTION__, mp->cur_phy);
	PHL_INFO("%s: status = %d\n", __FUNCTION__, hal_status);

	return hal_status;
}


enum rtw_hal_status rtw_hal_mp_tx_pmac_packet(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_info_t *hal_info = (struct hal_info_t *)mp->hal;

	PHL_INFO("%s: phy index: %d\n", __FUNCTION__, mp->cur_phy);
	PHL_INFO("%s: start tx: %d\n", __FUNCTION__, arg->start_tx);
	PHL_INFO("%s: is cck: %d\n", __FUNCTION__, arg->is_cck);
	PHL_INFO("%s: tx count: %d\n", __FUNCTION__, arg->tx_cnt);
	PHL_INFO("%s: period: %d\n", __FUNCTION__, arg->period);
	PHL_INFO("%s: tx time: %d\n", __FUNCTION__, arg->tx_time);

	if (arg->start_tx) {
		/* start mp watchdog timer */
		rtw_phl_mp_watchdog_start(mp);
	} else {
		/* stop mp watchdog timer */
		rtw_phl_mp_watchdog_stop(mp);
	}

	hal_status = rtw_hal_bb_set_pmac_packet_tx(hal_info->hal_com,
						   arg->start_tx,
						   arg->is_cck,
						   arg->tx_cnt,
						   arg->period,
						   arg->tx_time,
						   arg->cck_lbk_en,
						   mp->cur_phy);
	PHL_INFO("%s: status = %d\n", __FUNCTION__, hal_status);

	return hal_status;
}

enum rtw_hal_status rtw_hal_mp_tx_pmac_continuous(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_info_t *hal_info = (struct hal_info_t *)mp->hal;

	PHL_INFO("%s: phy index: %d\n", __FUNCTION__, mp->cur_phy);
	PHL_INFO("%s: start tx: %d\n", __FUNCTION__, arg->start_tx);
	PHL_INFO("%s: is cck: %d\n", __FUNCTION__, arg->is_cck);

	hal_status = rtw_hal_bb_set_pmac_cont_tx(hal_info->hal_com,
						 arg->start_tx,
						 arg->is_cck,
						 mp->cur_phy);
	PHL_INFO("%s: status = %d\n", __FUNCTION__, hal_status);

	return hal_status;
}

enum rtw_hal_status rtw_hal_mp_tx_pmac_fw_trigger(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_info_t *hal_info = (struct hal_info_t *)mp->hal;

	PHL_INFO("%s: phy index: %d\n", __FUNCTION__, mp->cur_phy);
	PHL_INFO("%s: start tx: %d\n", __FUNCTION__, arg->start_tx);
	PHL_INFO("%s: is cck: %d\n", __FUNCTION__, arg->is_cck);
	PHL_INFO("%s: tx count: %d\n", __FUNCTION__, arg->tx_cnt);
	PHL_INFO("%s: tx duty: %d\n", __FUNCTION__, arg->tx_time);

	hal_status = rtw_hal_bb_set_pmac_fw_trigger_tx(hal_info->hal_com,
						   arg->start_tx,
						   arg->is_cck,
						   arg->tx_cnt,
						   (u8)arg->tx_time,
						   mp->cur_phy);
	PHL_INFO("%s: status = %d\n", __FUNCTION__, hal_status);

	return hal_status;
}



enum rtw_hal_status rtw_hal_mp_tx_single_tone(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	PHL_INFO("%s: start tx: %d\n", __FUNCTION__, arg->start_tx);
	PHL_INFO("%s: rf path: %d\n", __FUNCTION__, arg->tx_path);

	hal_status = rtw_hal_rf_set_singletone_tx(mp->hal,
						  arg->start_tx,
						  arg->tx_path);
	PHL_INFO("%s: status = %d\n", __FUNCTION__, hal_status);

	return hal_status;
}

enum rtw_hal_status rtw_hal_mp_tx_carrier_suppression(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_info_t *hal_info = (struct hal_info_t *)mp->hal;

	PHL_INFO("%s\n", __FUNCTION__);

	hal_status = rtw_hal_bb_set_pmac_carrier_suppression_tx
					(hal_info->hal_com,
					arg->start_tx,
					arg->is_cck,
					mp->cur_phy);
	PHL_INFO("%s: status = %d\n", __FUNCTION__, hal_status);

	return hal_status;
}

enum rtw_hal_status rtw_hal_mp_tx_phy_ok_cnt(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	PHL_INFO("%s\n", __FUNCTION__);
	/*
	hal_status = rtw_hal_bb_get_tx_ok(phl_info->hal,
					mp->cur_phy, &arg->tx_ok);
	*/
	hal_status = RTW_HAL_STATUS_SUCCESS;
	PHL_INFO("%s: status = %d\n", __FUNCTION__, hal_status);
	PHL_INFO("%s: phy ok cnt = %d\n", __FUNCTION__, arg->tx_ok);

	return hal_status;
}

enum rtw_hal_status rtw_hal_mp_set_dpd_bypass(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_info_t *hal_info = (struct hal_info_t *)mp->hal;

	PHL_INFO("%s: phy index: %d\n", __FUNCTION__, mp->cur_phy);
	PHL_INFO("%s: dpd_bypass: %d\n", __FUNCTION__, arg->dpd_bypass);

	hal_status = rtw_hal_bb_set_dpd_bypass(hal_info->hal_com,
						   arg->dpd_bypass,
						   mp->cur_phy);

	PHL_INFO("%s: status = %d\n", __FUNCTION__, hal_status);

	return hal_status;
}

void rtw_hal_mp_check_tx_idle(struct mp_context *mp, struct mp_tx_arg *arg)
{
	arg->tx_state = rtw_hal_bb_check_tx_idle(mp->hal, mp->cur_phy);
}


enum rtw_hal_status rtw_hal_mp_bb_loop_bck(struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	u8 tx_path;
	u8 rx_path;

	PHL_INFO("%s\n", __FUNCTION__);

	if (arg->tx_path == RF_PATH_A) {
		tx_path = RF_PATH_A;
		rx_path = RF_PATH_B;
	} else {
		tx_path = RF_PATH_B;
		rx_path = RF_PATH_A;
	}

	hal_status = rtw_hal_bb_loop_bck_en(mp->hal, arg->enable, arg->is_dgt,
										tx_path, rx_path, arg->dbw, mp->cur_phy, arg->is_cck);

	return hal_status;
}

enum rtw_hal_status
rtw_hal_mp_cfg_tx_by_bt_link(struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	PHL_INFO("%s with bt_link(%d)\n", __FUNCTION__, arg->is_bt_link);

	rtw_hal_rf_cfg_tx_by_bt_link(mp->hal, arg->is_bt_link);
	hal_status = rtw_hal_bb_cfg_tx_by_bt_link(mp->hal, arg->is_bt_link);

	if (hal_status != RTW_HAL_STATUS_SUCCESS) {
		PHL_ERR("%s: bb cfg error\n", __FUNCTION__);
		goto exit;
	}
exit:
	return hal_status;
}

#endif /* CONFIG_HAL_TEST_MP */

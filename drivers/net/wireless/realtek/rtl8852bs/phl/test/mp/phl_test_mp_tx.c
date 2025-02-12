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
#define _PHL_TEST_MP_TX_C_
#include "../../phl_headers.h"
#include "phl_test_mp_def.h"
#include "../../hal_g6/test/mp/hal_test_mp_api.h"
#include "../../phl_api.h"

#ifdef CONFIG_PHL_TEST_MP
static enum rtw_phl_status
phl_mp_get_plcp_usr_info(struct mp_context *mp, struct mp_tx_arg *arg)
{
	struct rtw_phl_com_t *phl_com = mp->phl_com;
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	int plcp_usr_idx = arg->plcp_usr_idx;

	if(arg->nss > phl_com->phy_cap[mp->cur_phy].txss){
		hal_status = RTW_HAL_STATUS_FAILURE;
		PHL_INFO("%s Invalid NSS %d\n",__FUNCTION__, arg->nss);
	}
	else{
		PHL_INFO("%s arg->plcp_usr_idx = %d\n",__FUNCTION__, arg->plcp_usr_idx);
		PHL_INFO("%s arg->mcs = %d\n",__FUNCTION__, arg->mcs);
		PHL_INFO("%s arg->mpdu_len = %d\n",__FUNCTION__, arg->mpdu_len);
		PHL_INFO("%s arg->n_mpdu = %d\n",__FUNCTION__, arg->n_mpdu);
		PHL_INFO("%s arg->fec = %d\n",__FUNCTION__, arg->fec);
		PHL_INFO("%s arg->dcm = %d\n",__FUNCTION__, arg->dcm);
		PHL_INFO("%s arg->aid = %d\n",__FUNCTION__, arg->aid);
		PHL_INFO("%s arg->scrambler_seed = %d\n",__FUNCTION__, arg->scrambler_seed);
		PHL_INFO("%s arg->random_init_seed = %d\n",__FUNCTION__, arg->random_init_seed);
		PHL_INFO("%s arg->apep = %d\n",__FUNCTION__, arg->apep);
		PHL_INFO("%s arg->ru_alloc = %d\n",__FUNCTION__, arg->ru_alloc);
		PHL_INFO("%s arg->nss = %d\n",__FUNCTION__, arg->nss);
		PHL_INFO("%s arg->txbf = %d\n",__FUNCTION__, arg->txbf);
		PHL_INFO("%s arg->pwr_boost_db = %d\n",__FUNCTION__, arg->pwr_boost_db);

		/* _os_mem_cpy(mp->phl_com->drv_priv,(void*)(&(mp->usr[plcp_usr_idx])),(void*)((unsigned long)arg+offset1),(offset2-offset1)); */
		mp->usr[plcp_usr_idx].mcs = arg->mcs;
		mp->usr[plcp_usr_idx].mpdu_len = arg->mpdu_len;
		mp->usr[plcp_usr_idx].n_mpdu = arg->n_mpdu;
		mp->usr[plcp_usr_idx].fec = arg->fec;
		mp->usr[plcp_usr_idx].dcm = arg->dcm;
		mp->usr[plcp_usr_idx].aid = arg->aid;
#ifdef RTW_WKARD_AP_MP
		/* in AP mode, Random Seed is not good for MP, so this is a workaround */
		mp->usr[plcp_usr_idx].scrambler_seed = 0x4b;
		mp->usr[plcp_usr_idx].random_init_seed = 0x81;
#else
		mp->usr[plcp_usr_idx].scrambler_seed = arg->scrambler_seed;
		mp->usr[plcp_usr_idx].random_init_seed = arg->random_init_seed;
#endif
		mp->usr[plcp_usr_idx].apep = arg->apep;
		mp->usr[plcp_usr_idx].ru_alloc = arg->ru_alloc;
		mp->usr[plcp_usr_idx].nss = arg->nss;
		mp->usr[plcp_usr_idx].txbf = arg->txbf;
		mp->usr[plcp_usr_idx].pwr_boost_db = arg->pwr_boost_db;

		PHL_INFO("%s Copy to user\n",__FUNCTION__);
		PHL_INFO("%s plcp_usr_idx = %d\n",__FUNCTION__, plcp_usr_idx);
		PHL_INFO("%s mcs = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].mcs);
		PHL_INFO("%s mpdu_len = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].mpdu_len);
		PHL_INFO("%s n_mpdu = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].n_mpdu);
		PHL_INFO("%s fec = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].fec);
		PHL_INFO("%s dcm = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].dcm);
		PHL_INFO("%s aid = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].aid);
		PHL_INFO("%s scrambler_seed = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].scrambler_seed);
		PHL_INFO("%s random_init_seed = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].random_init_seed);
		PHL_INFO("%s apep = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].apep);
		PHL_INFO("%s ru_alloc = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].ru_alloc);
		PHL_INFO("%s nss = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].nss);
		PHL_INFO("%s txbf = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].txbf);
		PHL_INFO("%s pwr_boost_db = %d\n",__FUNCTION__, mp->usr[plcp_usr_idx].pwr_boost_db);
		hal_status = RTW_HAL_STATUS_SUCCESS;
	}

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;
	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_set_param(struct mp_context *mp,
                                                struct mp_tx_arg *arg_param,
                                                struct rtw_trx_test_param *param)
{
	enum rtw_phl_status psts = RTW_PHL_STATUS_SUCCESS;
	struct rtw_wifi_role_t *wrole = NULL;
	struct rtw_wifi_role_link_t *rlink = NULL;
	struct rtw_phl_stainfo_t *sta = NULL;

	wrole = phl_mr_get_role_by_bandidx(mp->phl, (u8)rtw_hal_phy_idx_to_hw_band(arg_param->phy_idx));
	if (wrole == NULL) {
		PHL_ERR("%s: wrole is null with phy_idx(%d)!!",__func__, arg_param->phy_idx);
		psts = RTW_PHL_STATUS_FAILURE;
		return psts;
	}
	rlink = get_rlink(wrole, RTW_RLINK_PRIMARY);
	sta =rtw_phl_get_stainfo_self(mp->phl, rlink);

	param->tx_cap.macid= sta->macid;
	param->tx_cap.band= rlink->hw_band;

	param->tx_cap.f_rate = (u16)arg_param->data_rate;
	param->tx_cap.f_gi_ltf = (u8)arg_param->gi;
	param->tx_cap.f_stbc = (u8)arg_param->stbc;
	param->tx_cap.f_ldpc = (u8)arg_param->fec;
	param->tx_cap.f_bw = (u8)arg_param ->dbw;
	param->tx_cap.f_dcm = (u8)arg_param ->dcm;

	PHL_INFO("%s: stbc = %d, gi = %d, data rate = %X, coding = %d\n", __func__,
		param->tx_cap.f_stbc, param->tx_cap.f_gi_ltf,
		param->tx_cap.f_rate, param->tx_cap.f_ldpc);

	PHL_INFO("%s: macid(%d) hw_band(%d) ui_phy(%d)\n", __func__,
		param->tx_cap.macid, param->tx_cap.band, arg_param->phy_idx);

	return psts;
}

static enum rtw_phl_status phl_mp_tx_tmac(struct mp_context *mp,
							 struct mp_tx_arg *sw_tx_param)
{
	struct rtw_trx_test_param test_param = {0};

	rtw_phl_trx_default_param(mp->phl, &test_param);

	phl_mp_tx_set_param(mp, sw_tx_param, &test_param);

	rtw_phl_trx_testsuite(mp->phl, &test_param);
	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_plcp_gen(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	
	hal_status = rtw_hal_mp_tx_plcp_gen(mp, arg);

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_packet(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;

	if (arg->tx_method == MP_PMACT_TX) {
		PHL_INFO("%s: CMD = MP_PMACT_TX\n", __func__);
		hal_status = rtw_hal_mp_tx_pmac_packet(mp, arg);
		phl_status = RTW_PHL_STATUS_SUCCESS;
	} else if (arg->tx_method == MP_TMACT_TX) {
		PHL_INFO("%s: CMD = MP_TMACT_TX\n", __func__);
		phl_status = phl_mp_tx_tmac(mp, arg);
		hal_status = RTW_HAL_STATUS_SUCCESS;
	}
	else if(arg->tx_method == MP_SW_TX) {
		/* Remove this part after revise the dll command */
		PHL_INFO("%s: CMD = MP_SW_TX\n", __func__);
		phl_status = phl_mp_tx_tmac(mp, arg);
		hal_status = RTW_HAL_STATUS_SUCCESS;
	}
	else if(arg->tx_method == MP_FW_PMAC_TX){
		PHL_INFO("%s: CMD = MP_FW_PMAC_TX\n", __func__);
		hal_status = rtw_hal_mp_tx_pmac_fw_trigger(mp, arg);
		phl_status = RTW_PHL_STATUS_SUCCESS;
	}

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return phl_status;
}

static enum rtw_phl_status phl_mp_tx_continuous_packet(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	if (arg->tx_method == MP_PMACT_TX) {
		PHL_INFO("%s: CMD = MP_PMACT_TX\n", __func__);
		hal_status = rtw_hal_mp_tx_pmac_continuous(mp, arg);
	} else if (arg->tx_method == MP_TMACT_TX) {
		PHL_INFO("%s: CMD = MP_TMACT_TX\n", __func__);
		/* Call hal API */
	}

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_single_tone(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	hal_status = rtw_hal_mp_tx_single_tone(mp, arg);

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_carrier_suppression(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	hal_status = rtw_hal_mp_tx_carrier_suppression(mp, arg);

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_phy_ok(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	hal_status = rtw_hal_mp_tx_phy_ok_cnt(mp, arg);

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_tb_test(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	u8 ui_bssid[6];

	ui_bssid[0] = arg->bSS_id_addr0;
	ui_bssid[1] = arg->bSS_id_addr1;
	ui_bssid[2] = arg->bSS_id_addr2;
	ui_bssid[3] = arg->bSS_id_addr3;
	ui_bssid[4] = arg->bSS_id_addr4;
	ui_bssid[5] = arg->bSS_id_addr5;

	PHL_INFO("Bssid = %x - %x - %x - %x - %x - %x \n",ui_bssid[0],ui_bssid[1],ui_bssid[2],ui_bssid[3],ui_bssid[4],ui_bssid[5]);
	PHL_INFO("Bss aid = %x \n",arg->aid);
	PHL_INFO("Bss color = %x \n",arg->bss_color);

	rtw_phl_test_txtb_cfg(mp->phl_com, &arg->is_link_mode, sizeof(arg->is_link_mode), ui_bssid, (u8)arg->aid, (u8)arg->bss_color);

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_dpd_bypass(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	hal_status = rtw_hal_mp_set_dpd_bypass(mp, arg);

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_check_tx_idle(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_SUCCESS;

	rtw_hal_mp_check_tx_idle(mp, arg);

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = hal_status;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_bb_loop_bck(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	hal_status = rtw_hal_mp_bb_loop_bck(mp, arg);

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = RTW_HAL_STATUS_SUCCESS;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_tx_set_para_with_bt_link(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	hal_status = rtw_hal_mp_cfg_tx_by_bt_link(mp, arg);

	/* Record the result */
	arg->cmd_ok = true;
	arg->status = RTW_HAL_STATUS_SUCCESS;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_sw_tx_start(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;
	struct phl_info_t *phl_info = (struct phl_info_t *)mp->phl;
	struct rtw_phl_handler *tx_handler = &(phl_info->sw_tx_handler);
	struct phl_trx_test *trx_test = (struct phl_trx_test *)phl_info->trx_test;
	struct rtw_trx_test_param *test_param = &trx_test->test_param;

	if (!arg->start_tx) {
		_os_mem_set(phl_to_drvpriv(phl_info), test_param, 0, sizeof(test_param));
		test_param->mode = TEST_MODE_PHL_TX_AMPDU_TEST;
		test_param->ap_mode = 0;
		test_param->pkt_type = TEST_PKT_TYPE_UNI;
		test_param->tx_req_num = (u32)arg->tx_cnt;
		test_param->rx_req_num = 1;
		test_param->cur_addr[0] = 0x00;
		test_param->cur_addr[1] = 0xe0;
		test_param->cur_addr[2] = 0x4c;
		test_param->cur_addr[3] = 0xaa;
		test_param->cur_addr[4] = 0xbb;
		test_param->cur_addr[5] = 0xcc;

		test_param->sta_addr[0] = arg->mac_addr_0;
		test_param->sta_addr[1] = arg->mac_addr_1;
		test_param->sta_addr[2] = arg->mac_addr_2;
		test_param->sta_addr[3] = arg->mac_addr_3;
		test_param->sta_addr[4] = arg->mac_addr_4;
		test_param->sta_addr[5] = arg->mac_addr_5;
		test_param->bssid[0] = arg->mac_addr_0;
		test_param->bssid[1] = arg->mac_addr_1;
		test_param->bssid[2] = arg->mac_addr_2;
		test_param->bssid[3] = arg->mac_addr_3;
		test_param->bssid[4] = arg->mac_addr_4;
		test_param->bssid[5] = arg->mac_addr_5;
		test_param->tx_cap.bc = 0;
		test_param->qos = 1;
		test_param->tx_cap.macid= 0x00;
		test_param->tx_cap.tid = 0x03;
		test_param->tx_cap.dma_ch= 0x01;
		test_param->tx_cap.band= arg->phy_idx;
		test_param->tx_cap.userate_sel = 0x1;
		test_param->tx_cap.f_gi_ltf = 0;
		test_param->tx_cap.cts2self = 1;
		test_param->tx_cap.f_rate = (u16)arg->data_rate;
		test_param->tx_cap.f_gi_ltf = (u8)arg->gi;
		test_param->tx_cap.f_stbc = (u8)arg->stbc;
		test_param->tx_cap.f_ldpc = (u8)arg->fec;
		test_param->tx_cap.f_bw = (u8)arg->dbw;
		test_param->tx_cap.f_dcm = (u8)arg->dcm;

		test_param->tx_cap.ampdu_en = 1;
		test_param->tx_cap.max_agg_num = arg->ampdu_num;
		test_param->tx_payload_size = arg->sw_tx_payload_size;
		test_param->sw_tx_seq = 1;

		tx_handler->type = RTW_PHL_HANDLER_PRIO_HIGH; /* tasklet */
		tx_handler->callback = phl_test_sw_tx_cb;
		tx_handler->context = phl_info;
		tx_handler->drv_priv = mp->phl_com->drv_priv;

		phl_status = phl_schedule_handler(mp->phl_com, tx_handler);
		PHL_INFO("%s: phl_schedule_handler status = %d\n", __func__, phl_status);
	} else {
		PHL_INFO("%s: tx ongoing...\n", __func__);
	}
	/* Record the result */
	arg->cmd_ok = true;
	arg->start_tx = true;
	arg->status = RTW_HAL_STATUS_SUCCESS;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}

static enum rtw_phl_status phl_mp_sw_tx_stop(
	struct mp_context *mp, struct mp_tx_arg *arg)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)mp->phl;
	struct phl_trx_test *trx_test = (struct phl_trx_test *)phl_info->trx_test;
	struct rtw_trx_test_param *test_param = &trx_test->test_param;

	if (arg->start_tx)
		test_param->mode = TEST_MODE_PHL_TX_RING_TEST;
	else
		PHL_INFO("%s: tx did not start...\n", __func__);

	/* Record the result */
	arg->cmd_ok = true;
	arg->start_tx = false;
	arg->status = RTW_HAL_STATUS_SUCCESS;

	/* Transfer to report */
	mp->rpt = arg;
	mp->rpt_len = sizeof(struct mp_tx_arg);
	mp->buf = NULL;
	mp->buf_len = 0;

	return RTW_PHL_STATUS_SUCCESS;
}



enum rtw_phl_status mp_tx(struct mp_context *mp, struct mp_tx_arg *arg)
{
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;

	switch(arg->cmd) {
	case MP_TX_CONFIG_PLCP_PATTERN:
		PHL_INFO("%s: CMD = MP_TX_SET_PLCP_PATTERN\n", __func__);
		break;
	case MP_TX_CONFIG_PLCP_USER_INFO:
		PHL_INFO("%s: CMD = MP_TX_CONFIG_PLCP_USER_INFO\n", __func__);
		phl_status = phl_mp_get_plcp_usr_info(mp,arg);
		break;
	case MP_TX_CONFIG_PLCP_COMMON_INFO:
		PHL_INFO("%s: CMD = MP_TX_CONFIG_PLCP_COMMON_INFO\n", __func__);
		phl_status = phl_mp_tx_plcp_gen(mp, arg);
		break;
	case MP_TX_PACKETS:
		PHL_INFO("%s: CMD = MP_PACKETS_TX\n", __func__);
		phl_status = phl_mp_tx_packet(mp, arg);
		break;
	case MP_TX_CONTINUOUS:
		PHL_INFO("%s: CMD = MP_TX_CONTINUOUS\n", __func__);
		phl_status = phl_mp_tx_continuous_packet(mp, arg);
		break;
	case MP_TX_SINGLE_TONE:
		PHL_INFO("%s: CMD = MP_TX_SINGLE_TONE\n", __func__);
		phl_status = phl_mp_tx_single_tone(mp, arg);
		break;
	case MP_TX_CCK_Carrier_Suppression:
		PHL_INFO("%s: CMD = MP_TX_CCK_Carrier_Suppression\n", __func__);
		phl_status = phl_mp_tx_carrier_suppression(mp, arg);
		break;
	case MP_TX_CMD_PHY_OK:
		PHL_INFO("%s: CMD = MP_TX_CMD_PHY_OK\n", __FUNCTION__);
		phl_status = phl_mp_tx_phy_ok(mp, arg);
		break;
	case MP_TX_NONE:
		PHL_INFO("%s: CMD = MP_TX_NONE\n", __func__);
		break;
	case MP_TX_TB_TEST:
		PHL_INFO("%s: CMD = MP_TX_TB_TEST\n", __FUNCTION__);
		phl_status = phl_mp_tx_tb_test(mp, arg);
		break;
	case MP_TX_DPD_BYPASS:
		PHL_INFO("%s: CMD = MP_TX_DPD_BYPASS\n", __FUNCTION__);
		phl_status = phl_mp_tx_dpd_bypass(mp, arg);
		break;
	case MP_TX_CHECK_TX_IDLE:
		PHL_INFO("%s: CMD = MP_TX_CHECK_TX_IDLE\n", __FUNCTION__);
		phl_status = phl_mp_tx_check_tx_idle(mp, arg);
		break;
	case MP_TX_CMD_BB_LOOPBCK:
		PHL_INFO("%s: CMD = MP_CONFIG_CMD_BB_LOOPBCK\n", __FUNCTION__);
		phl_status = phl_mp_tx_bb_loop_bck(mp, arg);
		break;
	case MP_TX_SET_PARA_BY_BT_LINK:
		PHL_INFO("%s: CMD = MP_TX_SET_PARA_BY_BT_LINK\n", __FUNCTION__);
		phl_status = phl_mp_tx_set_para_with_bt_link(mp, arg);
		break;
	case MP_TX_CMD_SW_TX_START:
		PHL_INFO("%s: CMD = MP_TX_CMD_SW_TX_START\n", __FUNCTION__);
		phl_status = phl_mp_sw_tx_start(mp, arg);
		break;
	case MP_TX_CMD_SW_TX_STOP:
		PHL_INFO("%s: CMD = MP_TX_CMD_SW_TX_STOP\n", __FUNCTION__);
		phl_status = phl_mp_sw_tx_stop(mp, arg);
		break;
	default:
		PHL_INFO("%s: CMD = Unknown\n", __func__);
		break;
	}

	return phl_status;
}
#endif /* CONFIG_PHL_TEST_MP */

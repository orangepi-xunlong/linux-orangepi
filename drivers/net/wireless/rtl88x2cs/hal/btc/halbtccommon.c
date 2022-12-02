/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation.
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

#include "mp_precomp.h"

#if (BT_SUPPORT == 1 && COEX_SUPPORT == 1)

static u8 *trace_buf = &gl_btc_trace_buf[0];
static const u32 coex_ver_date = 20200103;
static const u32 coex_ver = 0x17;
static const u32 wl_fw_desired_ver = 0x70011;

static u8
rtw_btc_rssi_state(struct btc_coexist *btc, u8 pre_state,
		   u8 rssi, u8 rssi_thresh)
{
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8	next_state, tol = chip_para->rssi_tolerance;

	if (pre_state == BTC_RSSI_STATE_LOW ||
	    pre_state == BTC_RSSI_STATE_STAY_LOW) {
		if (rssi >= (rssi_thresh + tol))
			next_state = BTC_RSSI_STATE_HIGH;
		else
			next_state = BTC_RSSI_STATE_STAY_LOW;
	} else {
		if (rssi < rssi_thresh)
			next_state = BTC_RSSI_STATE_LOW;
		else
			next_state = BTC_RSSI_STATE_STAY_HIGH;
	}

	return next_state;
}

static void
rtw_btc_limited_tx(struct btc_coexist *btc, boolean force_exec,
		   boolean tx_limit_en,  boolean ampdu_limit_en)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	const struct btc_chip_para *chip_para = btc->chip_para;
	boolean wl_b_mode = FALSE;

	if (!chip_para->scbd_support)
		return;

	/* Force Max Tx retry limit = 8 */
	if (!force_exec && tx_limit_en == coex_sta->wl_tx_limit_en &&
	    ampdu_limit_en == coex_sta->wl_ampdu_limit_en)
		return;

	/* backup MAC reg */
	if (!coex_sta->wl_tx_limit_en) {
		coex_sta->wl_arfb1 = btc->btc_read_4byte(btc, REG_DARFRC);
		coex_sta->wl_arfb2 = btc->btc_read_4byte(btc, REG_DARFRCH);

		coex_sta->wl_txlimit = btc->btc_read_2byte(btc,
							   REG_RETRY_LIMIT);
	}

	if (!coex_sta->wl_ampdu_limit_en)
		coex_sta->wl_ampdulen =
			btc->btc_read_1byte(btc, REG_AMPDU_MAX_TIME_V1);

	coex_sta->wl_tx_limit_en = tx_limit_en;
	coex_sta->wl_ampdu_limit_en = ampdu_limit_en;

	if (tx_limit_en) {
		/* Set BT polluted packet on for Tx rate adaptive
		 * Set queue life time to avoid can't reach tx retry limit
		 * if tx is always break by GNT_BT.
		 */
		btc->btc_write_1byte_bitmask(btc, REG_TX_HANG_CTRL,
					     BIT_EN_GNT_BT_AWAKE, 0x1);

		/* queue life time can't on if 2-port */
		if (link_info_ext->num_of_active_port <= 1)
			btc->btc_write_1byte_bitmask(btc, REG_LIFETIME_EN, 0xf,
						     0xf);

		/* Max Tx retry limit = 8*/
		btc->btc_write_2byte(btc, REG_RETRY_LIMIT, 0x0808);

		btc->btc_get(btc, BTC_GET_BL_WIFI_UNDER_B_MODE, &wl_b_mode);

		/* Auto rate fallback step within 8 retry*/
		if (wl_b_mode) {
			btc->btc_write_4byte(btc, REG_DARFRC, 0x1000000);
			btc->btc_write_4byte(btc, REG_DARFRCH, 0x1010101);
		} else {
			btc->btc_write_4byte(btc, REG_DARFRC, 0x1000000);
			btc->btc_write_4byte(btc, REG_DARFRCH, 0x4030201);
		}
	} else {
		/* Set BT polluted packet on for Tx rate adaptive not
		 *including Tx retry break by PTA, 0x45c[19] =1
		 */
		btc->btc_write_1byte_bitmask(btc, REG_TX_HANG_CTRL,
					     BIT_EN_GNT_BT_AWAKE, 0x0);

		/* Set queue life time to avoid can't reach tx retry limit
		 * if tx is always break by GNT_BT.
		 */
		btc->btc_write_1byte_bitmask(btc, REG_LIFETIME_EN, 0xf, 0x0);

		/* Recovery Max Tx retry limit*/
		btc->btc_write_2byte(btc, REG_RETRY_LIMIT,
				     coex_sta->wl_txlimit);
		btc->btc_write_4byte(btc, REG_DARFRC, coex_sta->wl_arfb1);
		btc->btc_write_4byte(btc, REG_DARFRCH, coex_sta->wl_arfb2);
	}

	if (ampdu_limit_en)
		btc->btc_write_1byte(btc, REG_AMPDU_MAX_TIME_V1, 0x20);
	else
		btc->btc_write_1byte(btc, REG_AMPDU_MAX_TIME_V1,
				     coex_sta->wl_ampdulen);
}

static void
rtw_btc_low_penalty_ra(struct btc_coexist *btc, boolean force_exec,
		       boolean low_penalty_ra, u8 thres)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;

	if (!force_exec) {
		if (low_penalty_ra == coex_dm->cur_low_penalty_ra &&
		    thres == coex_sta->wl_ra_thres)
			return;
	}

	if (low_penalty_ra)
		btc->btc_phydm_modify_RA_PCR_threshold(btc, 0, thres);
	else
		btc->btc_phydm_modify_RA_PCR_threshold(btc, 0, 0);

	coex_dm->cur_low_penalty_ra = low_penalty_ra;
	coex_sta->wl_ra_thres = thres;
}

static void
rtw_btc_limited_wl(struct btc_coexist *btc)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;

	if (link_info_ext->is_all_under_5g ||
	    link_info_ext->num_of_active_port == 0 ||
	    coex_dm->bt_status == BTC_BTSTATUS_NCON_IDLE) {
		rtw_btc_low_penalty_ra(btc, NM_EXCU, FALSE, 0);
		rtw_btc_limited_tx(btc, NM_EXCU, FALSE, FALSE);
	} else if (link_info_ext->num_of_active_port > 1) {
		rtw_btc_low_penalty_ra(btc, NM_EXCU, TRUE, 30);
		rtw_btc_limited_tx(btc, NM_EXCU, TRUE, TRUE);
	} else {
		if (link_info_ext->is_p2p_connected)
			rtw_btc_low_penalty_ra(btc, NM_EXCU, TRUE, 30);
		else
			rtw_btc_low_penalty_ra(btc, NM_EXCU, TRUE, 15);

		if (coex_sta->bt_hid_exist || coex_sta->bt_hid_pair_num > 0 ||
		    coex_sta->bt_hfp_exist)
			rtw_btc_limited_tx(btc, NM_EXCU, TRUE, TRUE);
		else
			rtw_btc_limited_tx(btc, NM_EXCU, TRUE, FALSE);
	}
}

static void
rtw_btc_mailbox_operation(struct btc_coexist *btc, u8 h2c_id, u8 h2c_len,
			  u8 *h2c_para)
{
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8	buf[6] = {0};

	if (chip_para->mailbox_support) {
		btc->btc_fill_h2c(btc, h2c_id, h2c_len, h2c_para);
		return;
	}

	switch (h2c_id) {
	case 0x61:
		buf[0] = 3;
		buf[1] = 0x1;	/* polling enable, 1=enable, 0=disable */
		buf[2] = 0x2;	/* polling time in seconds */
		buf[3] = 0x1;	/* auto report enable, 1=enable, 0=disable */

		btc->btc_set(btc, BTC_SET_ACT_CTRL_BT_INFO, (void *)&buf[0]);
		break;
	case 0x62:
		buf[0] = 4;
		buf[1] = 0x3;		/* OP_Code */
		buf[2] = 0x2;		/* OP_Code_Length */
		buf[3] = (h2c_para[0] != 0) ? 0x1 : 0x0; /* OP_Code_Content */
		buf[4] = h2c_para[0];/* pwr_level */

		btc->btc_set(btc, BTC_SET_ACT_CTRL_BT_COEX, (void *)&buf[0]);
		break;
	case 0x63:
		buf[0] = 3;
		buf[1] = 0x1;		/* OP_Code */
		buf[2] = 0x1;		/* OP_Code_Length */
		buf[3] = (h2c_para[0] == 0x1) ? 0x1 : 0x0; /* OP_Code_Content */

		btc->btc_set(btc, BTC_SET_ACT_CTRL_BT_COEX, (void *)&buf[0]);
		break;
	case 0x66:
		buf[0] = 5;
		buf[1] = 0x5;		/* OP_Code */
		buf[2] = 0x3;		/* OP_Code_Length */
		buf[3] = h2c_para[0];	/* OP_Code_Content */
		buf[4] = h2c_para[1];
		buf[5] = h2c_para[2];

		btc->btc_set(btc, BTC_SET_ACT_CTRL_BT_COEX, (void *)&buf[0]);
		break;
	}
}

static boolean
rtw_btc_freerun_check(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	u8 bt_rssi;

	if (coex_sta->force_freerun)
		return TRUE;

	if (coex_sta->force_tdd)
		return FALSE;

	if (coex_sta->bt_disabled)
		return FALSE;

	if (btc->board_info.btdm_ant_num == 1 ||
	    btc->board_info.ant_distance <= 5 || !coex_sta->wl_gl_busy)
		return FALSE;

	if (btc->board_info.ant_distance >= 40 ||
	    coex_sta->bt_hid_pair_num >= 2)
		return TRUE;

	/* ant_distance = 5 ~ 40  */
	if (BTC_RSSI_HIGH(coex_dm->wl_rssi_state[1]) &&
	    BTC_RSSI_HIGH(coex_dm->bt_rssi_state[0]))
		return TRUE;

	if (link_info_ext->traffic_dir == BTC_WIFI_TRAFFIC_TX)
		bt_rssi = coex_dm->bt_rssi_state[0];
	else
		bt_rssi = coex_dm->bt_rssi_state[1];

	if (BTC_RSSI_HIGH(coex_dm->wl_rssi_state[3]) &&
	    BTC_RSSI_HIGH(bt_rssi) &&
	    coex_sta->cnt_wl[BTC_CNT_WL_SCANAP] <= 5)
		return TRUE;

	return FALSE;
}

static void
rtw_btc_wl_leakap(struct btc_coexist *btc, boolean enable)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 h2c_para[2] = {0xc, 0};

	if (coex_sta->wl_leak_ap == enable)
		return;

	if (enable) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], turn on Leak-AP Rx Protection!!\n");

		h2c_para[1] = 0x0;
	} else {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], turn off Leak-AP Rx Protection!!\n");

		h2c_para[1] = 0x1;
	}

	BTC_TRACE(trace_buf);
	btc->btc_fill_h2c(btc, 0x69, 2, h2c_para);
	coex_sta->wl_leak_ap = enable;
	coex_sta->cnt_wl[BTC_CNT_WL_LEAKAP_NORX] = 0;
}

static void
rtw_btc_wl_ccklock_action(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 h2c_parameter[2] = {0}, ap_leak_rx_cnt = 0;
	boolean wifi_busy = FALSE;

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	if (!coex_sta->wl_gl_busy ||
	    coex_sta->wl_iot_peer == BTC_IOT_PEER_CISCO) {
		coex_sta->cnt_wl[BTC_CNT_WL_LEAKAP_NORX] = 0;
		return;
	}

	ap_leak_rx_cnt = coex_sta->wl_fw_dbg_info[7];
	/* Get realtime wifi_busy status  */
	btc->btc_get(btc, BTC_GET_BL_WIFI_BUSY, &wifi_busy);

	if (coex_sta->wl_leak_ap && coex_sta->wl_force_lps_ctrl &&
	    !coex_sta->wl_cck_lock_ever) {
		if (ap_leak_rx_cnt <= 5 && wifi_busy)
			coex_sta->cnt_wl[BTC_CNT_WL_LEAKAP_NORX]++;
		else
			coex_sta->cnt_wl[BTC_CNT_WL_LEAKAP_NORX] = 0;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Leak-AP Rx extend cnt = %d!!\n",
			    coex_sta->cnt_wl[BTC_CNT_WL_LEAKAP_NORX]);
		BTC_TRACE(trace_buf);

		/* If 7-streak ap_leak_rx_cnt <= 5, turn off leak-AP for TP*/
		if (coex_sta->cnt_wl[BTC_CNT_WL_LEAKAP_NORX] >= 7)
			rtw_btc_wl_leakap(btc, FALSE);
	} else if (!coex_sta->wl_leak_ap && coex_sta->wl_cck_lock) {
		rtw_btc_wl_leakap(btc, TRUE);
	}
}

static void
rtw_btc_wl_ccklock_detect(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	boolean is_cck_lock_rate = FALSE;

	if (coex_dm->bt_status == BTC_BTSTATUS_INQ_PAGE ||
	    coex_sta->bt_setup_link)
		return;

	if (coex_sta->wl_rx_rate <= BTC_CCK_2 ||
	    coex_sta->wl_rts_rx_rate <= BTC_CCK_2)
		is_cck_lock_rate = TRUE;

	if (link_info_ext->is_connected && coex_sta->wl_gl_busy &&
	    BTC_RSSI_HIGH(coex_dm->wl_rssi_state[3]) &&
	    (coex_dm->bt_status == BTC_BTSTATUS_ACL_BUSY ||
	     coex_dm->bt_status == BTC_BTSTATUS_ACL_SCO_BUSY ||
	     coex_dm->bt_status == BTC_BTSTATUS_SCO_BUSY)) {
		if (is_cck_lock_rate) {
			coex_sta->wl_cck_lock = TRUE;

			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], cck locking...\n");
			BTC_TRACE(trace_buf);
		} else {
			coex_sta->wl_cck_lock = FALSE;

			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], cck unlock...\n");
			BTC_TRACE(trace_buf);
		}
	} else {
		coex_sta->wl_cck_lock = FALSE;
	}

	/* CCK lock identification */
	if (coex_sta->wl_cck_lock && !coex_sta->wl_cck_lock_pre)
		btc->btc_set_timer(btc, BTC_TIMER_WL_CCKLOCK, 3);

	coex_sta->wl_cck_lock_pre = coex_sta->wl_cck_lock;
}

static void
rtw_btc_set_extend_btautoslot(struct btc_coexist *btc, u8 thres)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 h2c_para[2] = {0x9, 0x32};

	if (coex_sta->bt_ext_autoslot_thres == thres)
		return;

	h2c_para[1] = thres; /* thres must be 50 ~ 80*/

	coex_sta->bt_ext_autoslot_thres = h2c_para[1];

	btc->btc_fill_h2c(btc, 0x69, 2, h2c_para);
}

static void
rtw_btc_set_tdma_timer_base(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u16 tbtt_interval = 100;
	u8 h2c_para[2] = {0xb, 0x1};

	btc->btc_get(btc, BTC_GET_U2_BEACON_PERIOD, &tbtt_interval);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], tbtt_interval = %d\n", tbtt_interval);
	BTC_TRACE(trace_buf);

	/* Add for JIRA coex-256 */
	if (type == 3 && tbtt_interval >= 100) { /* 50ms-slot  */
		if (coex_sta->tdma_timer_base == 3)
			return;

		h2c_para[1] = (tbtt_interval / 50) - 1;
		h2c_para[1] = h2c_para[1] | 0xc0; /* 50ms-slot */
		coex_sta->tdma_timer_base = 3;
	} else if (tbtt_interval < 80 && tbtt_interval > 0) {
		if (coex_sta->tdma_timer_base == 2)
			return;
		h2c_para[1] = (100 / tbtt_interval);

		if (100 % tbtt_interval != 0)
			h2c_para[1] = h2c_para[1] + 1;

		h2c_para[1] = h2c_para[1] & 0x3f;
		coex_sta->tdma_timer_base = 2;
	} else if (tbtt_interval >= 180) {
		if (coex_sta->tdma_timer_base == 1)
			return;
		h2c_para[1] = (tbtt_interval / 100);

		if (tbtt_interval % 100 <= 80)
			h2c_para[1] = h2c_para[1] - 1;

		h2c_para[1] = h2c_para[1] & 0x3f;
		h2c_para[1] = h2c_para[1] | 0x80;
		coex_sta->tdma_timer_base = 1;
	} else {
		if (coex_sta->tdma_timer_base == 0)
			return;
		h2c_para[1] = 0x1;
		coex_sta->tdma_timer_base = 0;
	}

	btc->btc_fill_h2c(btc, 0x69, 2, h2c_para);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): h2c_0x69 = 0x%x\n", __func__, h2c_para[1]);
	BTC_TRACE(trace_buf);

	/* no 5ms_wl_slot_extend for 4-slot mode  */
	if (coex_sta->tdma_timer_base == 3)
		rtw_btc_wl_ccklock_action(btc);
}

static void
rtw_btc_set_wl_pri_mask(struct btc_coexist *btc, u8 bitmap, u8 data)
{
	u32 addr;

	addr = REG_BT_COEX_TABLE_H + (bitmap / 8);
	bitmap = bitmap % 8;

	btc->btc_write_1byte_bitmask(btc, addr, BIT(bitmap), data);
}

static void
rtw_btc_set_bt_golden_rx_range(struct btc_coexist *btc, boolean force_exec,
			       u8 profile_id, u8 shift_level)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u16 para;

	if (profile_id > 3)
		return;

	if (!force_exec &&
	    shift_level == coex_sta->bt_golden_rx_shift[profile_id])
		return;

	coex_sta->bt_golden_rx_shift[profile_id] = shift_level;

	para = (profile_id << 8) | ((0x100 - shift_level) & 0xff);

	btc->btc_set(btc, BTC_SET_BL_BT_GOLDEN_RX_RANGE, &para);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): para = 0x%04x\n", __func__, para);
	BTC_TRACE(trace_buf);
}

static void
rtw_btc_query_bt_info(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 h2c_parameter[1] = {0x1};

	if (coex_sta->bt_disabled)
		return;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_mailbox_operation(btc, 0x61, 1, h2c_parameter);
}

static void
rtw_btc_gnt_debug(struct btc_coexist *btc, boolean isenable)
{
	if (!isenable)
		btc->btc_write_1byte_bitmask(btc, 0x73, 0x8, 0x0);
	else
		btc->chip_para->chip_setup(btc, BTC_CSETUP_GNT_DEBUG);
}

static void
rtw_btc_gnt_workaround(struct btc_coexist *btc, boolean force_exec, u8 mode)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (!force_exec) {
		if (coex_sta->gnt_workaround_state == coex_sta->wl_coex_mode)
			return;
	}

	coex_sta->gnt_workaround_state = coex_sta->wl_coex_mode;

	btc->chip_para->chip_setup(btc, BTC_CSETUP_GNT_FIX);
}

static void
rtw_btc_monitor_bt_enable(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *chip_para = btc->chip_para;
	boolean bt_disabled = FALSE;
	u16 scbd;

	if (chip_para->scbd_support) {
		btc->btc_read_scbd(btc, &scbd);
		bt_disabled = (scbd & BTC_SCBD_BT_ONOFF) ? FALSE : TRUE;
	} else {
		if (coex_sta->cnt_bt[BTC_CNT_BT_DISABLE] >= 2)
			bt_disabled = TRUE;
	}

	btc->btc_set(btc, BTC_SET_BL_BT_DISABLE, &bt_disabled);

	if (coex_sta->bt_disabled != bt_disabled) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BT is from %s to %s!!\n",
			    (coex_sta->bt_disabled ? "disabled" : "enabled"),
			    (bt_disabled ? "disabled" : "enabled"));
		BTC_TRACE(trace_buf);
		coex_sta->bt_disabled = bt_disabled;

		coex_sta->bt_supported_feature = 0;
		coex_sta->bt_supported_version = 0;
		coex_sta->bt_ble_scan_type = 0;
		coex_sta->bt_ble_scan_para[0] = 0;
		coex_sta->bt_ble_scan_para[1] = 0;
		coex_sta->bt_ble_scan_para[2] = 0;
		coex_sta->bt_reg_vendor_ac = 0xffff;
		coex_sta->bt_reg_vendor_ae = 0xffff;
		coex_sta->bt_a2dp_vendor_id = 0;
		coex_sta->bt_a2dp_device_name = 0;
		coex_sta->bt_iqk_state = 0;
		coex_dm->cur_bt_lna_lvl = 0;
		btc->bt_info.bt_get_fw_ver = 0;

		/*for win10 BT disable->enable trigger wifi scan issue   */
		if (!coex_sta->bt_disabled) {
			coex_sta->bt_reenable = TRUE;
			btc->btc_set_timer(btc, BTC_TIMER_BT_REENABLE, 15);
		} else {
			coex_sta->bt_reenable = FALSE;
		}
	}
}

static void
rtw_btc_update_bt_sut_info(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u32 val = 0;

	if (coex_sta->bt_profile_num == 0) {
		/* clear golden rx range if no PAN exist */
		if (coex_sta->bt_golden_rx_shift[3] != 0)
			rtw_btc_set_bt_golden_rx_range(btc, FC_EXCU, 3, 0);
		return;
	}

	if (coex_sta->bt_a2dp_exist)
		rtw_btc_set_bt_golden_rx_range(btc, FC_EXCU, 2, 0);
	else
		coex_sta->bt_sut_pwr_lvl[2] = 0xff;

	if (coex_sta->bt_hfp_exist)
		rtw_btc_set_bt_golden_rx_range(btc, FC_EXCU, 0, 0);
	else
		coex_sta->bt_sut_pwr_lvl[0] = 0xff;

	if (coex_sta->bt_hid_exist)
		rtw_btc_set_bt_golden_rx_range(btc, FC_EXCU, 1, 0);
	else
		coex_sta->bt_sut_pwr_lvl[1] = 0xff;

	if (coex_sta->bt_pan_exist) {
		rtw_btc_set_bt_golden_rx_range(btc, FC_EXCU, 3,
					       coex_sta->bt_golden_rx_shift[3]);
	} else {
		coex_sta->bt_golden_rx_shift[3] = 0;
		coex_sta->bt_sut_pwr_lvl[3] = 0xff;
	}
}

static void
rtw_btc_update_wl_link_info(struct btc_coexist *btc, u8 reason)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_wifi_link_info_ext *linfo_ext = &btc->wifi_link_info_ext;
	struct btc_wifi_link_info linfo;
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8 wifi_central_chnl = 0, num_of_wifi_link = 0, i, rssi_state;
	u32 wifi_link_status = 0, wifi_bw;
	s32 wl_rssi;
	boolean isunder5G = FALSE, ismcc25g = FALSE, is_p2p_connected = FALSE,
		plus_bt = FALSE;

	btc->btc_get(btc, BTC_GET_BL_WIFI_SCAN, &linfo_ext->is_scan);
	btc->btc_get(btc, BTC_GET_BL_WIFI_LINK, &linfo_ext->is_link);
	btc->btc_get(btc, BTC_GET_BL_WIFI_ROAM, &linfo_ext->is_roam);
	btc->btc_get(btc, BTC_GET_BL_WIFI_LW_PWR_STATE, &linfo_ext->is_32k);
	btc->btc_get(btc, BTC_GET_BL_WIFI_4_WAY_PROGRESS, &linfo_ext->is_4way);
	btc->btc_get(btc, BTC_GET_BL_WIFI_CONNECTED, &linfo_ext->is_connected);
	btc->btc_get(btc, BTC_GET_U4_WIFI_TRAFFIC_DIR, &linfo_ext->traffic_dir);
	btc->btc_get(btc, BTC_GET_U4_WIFI_BW, &linfo_ext->wifi_bw);
	btc->btc_get(btc, BTC_GET_U4_WIFI_LINK_STATUS, &wifi_link_status);
	linfo_ext->port_connect_status = wifi_link_status & 0xffff;

	btc->btc_get(btc, BTC_GET_BL_WIFI_LINK_INFO, &linfo);
	btc->wifi_link_info = linfo;

	btc->btc_get(btc, BTC_GET_U1_WIFI_CENTRAL_CHNL, &wifi_central_chnl);
	coex_sta->wl_center_ch = wifi_central_chnl;

	btc->btc_get(btc, BTC_GET_S4_WIFI_RSSI, &wl_rssi);
	for (i = 0; i < 4; i++) {
		rssi_state = coex_dm->wl_rssi_state[i];
		rssi_state = rtw_btc_rssi_state(btc, rssi_state,
						(u8)(wl_rssi & 0xff),
						chip_para->wl_rssi_step[i]);
		coex_dm->wl_rssi_state[i] = rssi_state;
	}

	if (coex_sta->wl_linkscan_proc || coex_sta->wl_hi_pri_task1 ||
	    coex_sta->wl_hi_pri_task2 || coex_sta->wl_gl_busy)
		btc->btc_write_scbd(btc, BTC_SCBD_SCAN, TRUE);
	else
		btc->btc_write_scbd(btc, BTC_SCBD_SCAN, FALSE);

	/* Check scan/connect/special-pkt action first  */
	switch (reason) {
	case BTC_RSN_5GSCANSTART:
	case BTC_RSN_5GSWITCHBAND:
	case BTC_RSN_5GCONSTART:

		isunder5G = TRUE;
		break;
	case BTC_RSN_2GSCANSTART:
	case BTC_RSN_2GSWITCHBAND:
	case BTC_RSN_2GCONSTART:

		isunder5G = FALSE;
		break;
	case BTC_RSN_2GCONFINISH:
	case BTC_RSN_5GCONFINISH:
	case BTC_RSN_2GMEDIA:
	case BTC_RSN_5GMEDIA:
	case BTC_RSN_BTINFO:
	case BTC_RSN_PERIODICAL:
	case BTC_RSN_TIMERUP:
	case BTC_RSN_WLSTATUS:
	case BTC_RSN_2GSPECIALPKT:
	case BTC_RSN_5GSPECIALPKT:
	default:
		switch (linfo.link_mode) {
		case BTC_LINK_5G_MCC_GO_STA:
		case BTC_LINK_5G_MCC_GC_STA:
		case BTC_LINK_5G_SCC_GO_STA:
		case BTC_LINK_5G_SCC_GC_STA:

			isunder5G = TRUE;
			break;
		case BTC_LINK_2G_MCC_GO_STA:
		case BTC_LINK_2G_MCC_GC_STA:
		case BTC_LINK_2G_SCC_GO_STA:
		case BTC_LINK_2G_SCC_GC_STA:

			isunder5G = FALSE;
			break;
		case BTC_LINK_25G_MCC_GO_STA:
		case BTC_LINK_25G_MCC_GC_STA:

			isunder5G = FALSE;
			ismcc25g = TRUE;
			break;
		case BTC_LINK_ONLY_STA:
			if (linfo.sta_center_channel > 14)
				isunder5G = TRUE;
			else
				isunder5G = FALSE;
			break;
		case BTC_LINK_ONLY_GO:
		case BTC_LINK_ONLY_GC:
		case BTC_LINK_ONLY_AP:
		default:
			if (linfo.p2p_center_channel > 14)
				isunder5G = TRUE;
			else
				isunder5G = FALSE;
			break;
		}
		break;
	}

	linfo_ext->is_all_under_5g = isunder5G;
	linfo_ext->is_mcc_25g = ismcc25g;

	if (wifi_link_status & WIFI_STA_CONNECTED)
		num_of_wifi_link++;

	if (wifi_link_status & WIFI_AP_CONNECTED)
		num_of_wifi_link++;

	if (wifi_link_status & WIFI_P2P_GO_CONNECTED) {
		if (!(wifi_link_status & WIFI_AP_CONNECTED))
			num_of_wifi_link++;
		is_p2p_connected = TRUE;
	}

	if (wifi_link_status & WIFI_P2P_GC_CONNECTED) {
		num_of_wifi_link++;
		is_p2p_connected = TRUE;
	}

	linfo_ext->num_of_active_port = num_of_wifi_link;
	linfo_ext->is_p2p_connected = is_p2p_connected;

	if (linfo.link_mode == BTC_LINK_ONLY_GO && linfo.bhotspot)
		linfo_ext->is_ap_mode = TRUE;
	else
		linfo_ext->is_ap_mode = FALSE;

	if (linfo_ext->is_p2p_connected && coex_sta->bt_link_exist)
		plus_bt = TRUE;

	btc->btc_set(btc, BTC_SET_BL_MIRACAST_PLUS_BT, &plus_bt);

	if (linfo_ext->is_scan || linfo_ext->is_link ||
	    linfo_ext->is_roam || linfo_ext->is_4way ||
	    reason == BTC_RSN_2GSCANSTART ||
	    reason == BTC_RSN_2GSWITCHBAND ||
	    reason == BTC_RSN_2GCONSTART ||
	    reason == BTC_RSN_2GSPECIALPKT)
		coex_sta->wl_linkscan_proc = TRUE;
	else
		coex_sta->wl_linkscan_proc = FALSE;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], scan = %d, link = %d, roam = %d 4way = %d!!!\n",
		    linfo_ext->is_scan, linfo_ext->is_link,
		    linfo_ext->is_roam,
		    linfo_ext->is_4way);
	BTC_TRACE(trace_buf);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], wifi_link_info: link_mode=%d, STA_Ch=%d, P2P_Ch=%d, AnyClient_Join_Go=%d !\n",
		    linfo.link_mode,
		    linfo.sta_center_channel,
		    linfo.p2p_center_channel,
		    linfo.bany_client_join_go);
	BTC_TRACE(trace_buf);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], wifi_link_info: center_ch=%d, is_all_under_5g=%d, is_mcc_25g=%d!\n",
		    coex_sta->wl_center_ch,
		    linfo_ext->is_all_under_5g,
		    linfo_ext->is_mcc_25g);
	BTC_TRACE(trace_buf);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], wifi_link_info: port_connect_status=0x%x, active_port_cnt=%d, P2P_Connect=%d!\n",
		    linfo_ext->port_connect_status,
		    linfo_ext->num_of_active_port,
		    linfo_ext->is_p2p_connected);
	BTC_TRACE(trace_buf);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], Update reason = %s\n",
		    run_reason_string[reason]);
	BTC_TRACE(trace_buf);

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	/* coex-276  P2P-Go beacon request can't release issue
	 * Only PCIe/USB can set 0x454[6] = 1 to solve this issue,
	 * WL SDIO/USB interface need driver support.
	 */
#ifdef PLATFORM_WINDOWS
	if (btc->chip_interface != BTC_INTF_SDIO)
		btc->btc_write_1byte_bitmask(btc, REG_CCK_CHECK,
					     BIT_EN_BCN_PKT_REL, 0x1);
	else
		btc->btc_write_1byte_bitmask(btc, REG_CCK_CHECK,
					     BIT_EN_BCN_PKT_REL, 0x0);
#endif
}

static void
rtw_btc_update_bt_link_info(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *chip_para = btc->chip_para;
	boolean bt_busy = FALSE, increase_scan_dev_num = FALSE,
		scan_type_change = FALSE;
	u8 i, scan_type, rssi_state;

	/* update wl/bt rssi by btinfo */
	for (i = 0; i < 4; i++) {
		rssi_state = coex_dm->bt_rssi_state[i];
		rssi_state = rtw_btc_rssi_state(btc, rssi_state,
						coex_sta->bt_rssi,
						chip_para->bt_rssi_step[i]);
		coex_dm->bt_rssi_state[i] = rssi_state;
	}

	if (coex_sta->bt_ble_scan_en) {
		scan_type = btc->btc_get_ble_scan_type_from_bt(btc);

		if (scan_type != coex_sta->bt_ble_scan_type)
			scan_type_change = TRUE;

		coex_sta->bt_ble_scan_type = scan_type;
	}

	if (scan_type_change) {
		u32 *p = NULL;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BTinfo HiByte1[5] check, query BLE Scan type!!\n");
		BTC_TRACE(trace_buf);

		if ((coex_sta->bt_ble_scan_type & 0x1) == 0x1) {
			coex_sta->bt_init_scan = TRUE;
			p = &coex_sta->bt_ble_scan_para[0];
			*p = btc->btc_get_ble_scan_para_from_bt(btc, 0x1);
		} else {
			coex_sta->bt_init_scan = FALSE;
		}

		if ((coex_sta->bt_ble_scan_type & 0x2) == 0x2) {
			p = &coex_sta->bt_ble_scan_para[1];
			*p = btc->btc_get_ble_scan_para_from_bt(btc, 0x2);
		}

		if ((coex_sta->bt_ble_scan_type & 0x4) == 0x4) {
			p = &coex_sta->bt_ble_scan_para[2];
			*p = btc->btc_get_ble_scan_para_from_bt(btc, 0x4);
		}
	}

	coex_sta->bt_profile_num = 0;

	/* set link exist status */
	if (!(coex_sta->bt_info_lb2 & BTC_INFO_CONNECTION)) {
		coex_sta->bt_link_exist = FALSE;
		coex_sta->bt_pan_exist = FALSE;
		coex_sta->bt_a2dp_exist = FALSE;
		coex_sta->bt_hid_exist = FALSE;
		coex_sta->bt_hfp_exist = FALSE;
		coex_sta->bt_msft_mr_exist = FALSE;
	} else { /* connection exists */
		coex_sta->bt_link_exist = TRUE;
		if (coex_sta->bt_info_lb2 & BTC_INFO_FTP) {
			coex_sta->bt_pan_exist = TRUE;
			coex_sta->bt_profile_num++;
		} else {
			coex_sta->bt_pan_exist = FALSE;
		}

		if (coex_sta->bt_info_lb2 & BTC_INFO_A2DP) {
			coex_sta->bt_a2dp_exist = TRUE;
			coex_sta->bt_profile_num++;
		} else {
			coex_sta->bt_a2dp_exist = FALSE;
		}

		if (coex_sta->bt_info_lb2 & BTC_INFO_HID) {
			coex_sta->bt_hid_exist = TRUE;
			coex_sta->bt_profile_num++;
		} else {
			coex_sta->bt_hid_exist = FALSE;
		}

		if (coex_sta->bt_info_lb2 & BTC_INFO_SCO_ESCO) {
			coex_sta->bt_hfp_exist = TRUE;
			coex_sta->bt_profile_num++;
		} else {
			coex_sta->bt_hfp_exist = FALSE;
		}
	}

	if (coex_sta->bt_info_lb2 & BTC_INFO_INQ_PAGE) {
		coex_dm->bt_status = BTC_BTSTATUS_INQ_PAGE;
	} else if (!(coex_sta->bt_info_lb2 & BTC_INFO_CONNECTION)) {
		coex_dm->bt_status = BTC_BTSTATUS_NCON_IDLE;
		coex_sta->bt_multi_link_remain = FALSE;
	} else if (coex_sta->bt_info_lb2 == BTC_INFO_CONNECTION) {
		if (coex_sta->bt_msft_mr_exist)
			coex_dm->bt_status = BTC_BTSTATUS_ACL_BUSY;
		else
			coex_dm->bt_status = BTC_BTSTATUS_CON_IDLE;
	} else if ((coex_sta->bt_info_lb2 & BTC_INFO_SCO_ESCO) ||
		   (coex_sta->bt_info_lb2 & BTC_INFO_SCO_BUSY)) {
		if (coex_sta->bt_info_lb2 & BTC_INFO_ACL_BUSY)
			coex_dm->bt_status = BTC_BTSTATUS_ACL_SCO_BUSY;
		else
			coex_dm->bt_status = BTC_BTSTATUS_SCO_BUSY;
	} else if (coex_sta->bt_info_lb2 & BTC_INFO_ACL_BUSY) {
		coex_dm->bt_status = BTC_BTSTATUS_ACL_BUSY;
	} else {
		coex_dm->bt_status = BTC_BTSTATUS_MAX;
	}

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s(), %s!!!\n",
		    __func__, bt_status_string[coex_dm->bt_status]);
	BTC_TRACE(trace_buf);

	if (coex_dm->bt_status == BTC_BTSTATUS_ACL_BUSY ||
	    coex_dm->bt_status == BTC_BTSTATUS_SCO_BUSY ||
	    coex_dm->bt_status == BTC_BTSTATUS_ACL_SCO_BUSY) {
		bt_busy = TRUE;
		increase_scan_dev_num = TRUE;
	} else {
		bt_busy = FALSE;
		increase_scan_dev_num = FALSE;
	}

	btc->btc_set(btc, BTC_SET_BL_BT_TRAFFIC_BUSY, &bt_busy);
	btc->btc_set(btc, BTC_SET_BL_INC_SCAN_DEV_NUM, &increase_scan_dev_num);

	if (coex_sta->bt_profile_num != coex_sta->bt_profile_num_pre) {
		rtw_btc_update_bt_sut_info(btc);
		coex_sta->bt_profile_num_pre = coex_sta->bt_profile_num;

		if (!coex_sta->bt_a2dp_exist) {
			coex_sta->bt_a2dp_vendor_id = 0;
			coex_sta->bt_a2dp_device_name = 0;
			coex_sta->bt_a2dp_flush_time = 0;
		}
	}

	coex_sta->cnt_bt[BTC_CNT_BT_INFOUPDATE]++;
}

static void
rtw_btc_update_wl_ch_info(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *chip_para = btc->chip_para;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	struct btc_wifi_link_info *link_info = &btc->wifi_link_info;
	u8 h2c_para[3] = {0}, i, wl_center_ch = 0;

	if (btc->manual_control)
		return;

	if (btc->stop_coex_dm || btc->wl_rf_state_off) {
		wl_center_ch = 0;
	} else if (type != BTC_MEDIA_DISCONNECT ||
		   (type == BTC_MEDIA_DISCONNECT &&
		    link_info_ext->num_of_active_port > 0)) {
		if (link_info_ext->num_of_active_port == 1) {
			if (link_info_ext->is_p2p_connected)
				wl_center_ch = link_info->p2p_center_channel;
			else
				wl_center_ch = link_info->sta_center_channel;
		} else { /* port > 2 */
			if (link_info->p2p_center_channel > 14 &&
			    link_info->sta_center_channel > 14)
				wl_center_ch = link_info->p2p_center_channel;
			else if (link_info->p2p_center_channel <= 14)
				wl_center_ch = link_info->p2p_center_channel;
			else if (link_info->sta_center_channel <= 14)
				wl_center_ch = link_info->sta_center_channel;
		}
	}

	if (wl_center_ch == 0 ||
	    (btc->board_info.btdm_ant_num == 1 && wl_center_ch <= 14)) {
		h2c_para[0] = 0;
		h2c_para[1] = 0;
		h2c_para[2] = 0;
	} else if (wl_center_ch <= 14) {
		h2c_para[0] = 0x1;
		h2c_para[1] = wl_center_ch;

		if (link_info_ext->wifi_bw == BTC_WIFI_BW_HT40)
			h2c_para[2] = chip_para->bt_afh_span_bw40;
		else
			h2c_para[2] = chip_para->bt_afh_span_bw20;
	} else if (chip_para->afh_5g_num > 1) { /* for 5G  */
		for (i = 0; i < chip_para->afh_5g_num; i++) {
			if (wl_center_ch == chip_para->afh_5g[i].wl_5g_ch) {
				h2c_para[0] = 0x3;
				h2c_para[1] = chip_para->afh_5g[i].bt_skip_ch;
				h2c_para[2] = chip_para->afh_5g[i].bt_skip_span;
				break;
			}
		}
	}

	coex_dm->wl_chnl_info[0] = h2c_para[0];
	coex_dm->wl_chnl_info[1] = h2c_para[1];
	coex_dm->wl_chnl_info[2] = h2c_para[2];
	rtw_btc_mailbox_operation(btc, 0x66, 3, h2c_para);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s: para[0:2] = 0x%x 0x%x 0x%x\n",
		    __func__, h2c_para[0], h2c_para[1], h2c_para[2]);
	BTC_TRACE(trace_buf);
}

static void
rtw_btc_set_wl_tx_power(struct btc_coexist *btc,
			boolean force_exec, u8 wl_pwr_dec_lvl)
{
	const struct btc_chip_para *chip_para = btc->chip_para;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;

	if (!force_exec && wl_pwr_dec_lvl == coex_dm->cur_wl_pwr_lvl)
		return;

	coex_dm->cur_wl_pwr_lvl = wl_pwr_dec_lvl;

	chip_para->chip_setup(btc, BTC_CSETUP_WL_TX_POWER);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s(): level = %d\n",
		    __func__, wl_pwr_dec_lvl);
	BTC_TRACE(trace_buf);
}

static void
rtw_btc_set_bt_tx_power(struct btc_coexist *btc,
			boolean force_exec, u8 bt_pwr_dec_lvl)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	u8 h2c_para[1] = {0};

	if (!force_exec && bt_pwr_dec_lvl == coex_dm->cur_bt_pwr_lvl)
		return;

	h2c_para[0] = (0x100 - bt_pwr_dec_lvl) & 0xff;

	rtw_btc_mailbox_operation(btc, 0x62, 1, h2c_para);

	coex_dm->cur_bt_pwr_lvl = bt_pwr_dec_lvl;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(), bt_tx_power = 0x%x, level = %d\n",
		    __func__, h2c_para[0], bt_pwr_dec_lvl);
	BTC_TRACE(trace_buf);
}

static void
rtw_btc_set_wl_rx_gain(struct btc_coexist *btc, boolean force_exec,
		       boolean low_gain_en)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *chip_para = btc->chip_para;

	if (!force_exec && low_gain_en == coex_dm->cur_wl_rx_low_gain_en)
		return;

	coex_dm->cur_wl_rx_low_gain_en = low_gain_en;

	if (low_gain_en)
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], Hi-L Rx!\n");
	else
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], Nm-L Rx!\n");

	BTC_TRACE(trace_buf);

	chip_para->chip_setup(btc, BTC_CSETUP_WL_RX_GAIN);
}

static void
rtw_btc_set_bt_rx_gain(struct btc_coexist *btc, boolean force_exec, u8 lna_lvl)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;

	if (!force_exec && lna_lvl == coex_dm->cur_bt_lna_lvl)
		return;

	if (lna_lvl < 7) {
		btc->btc_set(btc, BTC_SET_BL_BT_LNA_CONSTRAIN_LEVEL, &lna_lvl);
		/* use scoreboard[4] to notify BT Rx gain table change */
		btc->btc_write_scbd(btc, BTC_SCBD_RXGAIN, TRUE);
	} else {
		btc->btc_write_scbd(btc, BTC_SCBD_RXGAIN, FALSE);
	}

	coex_dm->cur_bt_lna_lvl = lna_lvl;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): bt_rx_LNA_level = %d\n",
		    __func__, lna_lvl);
	BTC_TRACE(trace_buf);
}

static void
rtw_btc_set_rf_para(struct btc_coexist *btc, boolean force_exec,
		    struct btc_rf_para para)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 tmp = 0;

	if (coex_sta->coex_freerun) {
		if (coex_sta->cnt_wl[BTC_CNT_WL_SCANAP] <= 5)
			tmp = 3;
	}

	rtw_btc_set_wl_tx_power(btc, force_exec, para.wl_pwr_dec_lvl);
	rtw_btc_set_bt_tx_power(btc, force_exec, para.bt_pwr_dec_lvl + tmp);
	rtw_btc_set_wl_rx_gain(btc, force_exec, para.wl_low_gain_en);
	rtw_btc_set_bt_rx_gain(btc, force_exec, para.bt_lna_lvl);
}

static void
rtw_btc_coex_ctrl_owner(struct btc_coexist *btc, boolean wifi_control)
{
	u8 val;

	val = (wifi_control) ? 1 : 0; /* 0x70[26] */
	btc->btc_write_1byte_bitmask(btc, REG_SYS_SDIO_CTRL3, BIT(2), val);

	if (!wifi_control)
		btc->chip_para->chip_setup(btc, BTC_CSETUP_WLAN_ACT_IPS);
}

static void
rtw_btc_set_gnt_bt(struct btc_coexist *btc, u8 state)
{
	btc->btc_write_linderct(btc, REG_LTE_IDR_COEX_CTRL, 0xc000, state);
	btc->btc_write_linderct(btc, REG_LTE_IDR_COEX_CTRL, 0x0c00, state);
}

static void
rtw_btc_set_gnt_wl(struct btc_coexist *btc, u8 state)
{
	btc->btc_write_linderct(btc, REG_LTE_IDR_COEX_CTRL, 0x3000, state);
	btc->btc_write_linderct(btc, REG_LTE_IDR_COEX_CTRL, 0x0300, state);
}

#ifdef PLATFORM_WINDOWS
static void
rtw_btc_mimo_ps(struct btc_coexist *btc, boolean force_exec,
		u8 state)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (!force_exec && state == coex_sta->wl_mimo_ps)
		return;

	coex_sta->wl_mimo_ps = state;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): state = %d\n", __func__, state);
	BTC_TRACE(trace_buf);

	btc->btc_set(btc, BTC_SET_MIMO_PS_MODE, &state);
}
#endif

static void
rtw_btc_wltoggle_tableA(IN struct btc_coexist *btc,
			IN boolean force_exec,  IN u32 table_case)
{
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8 h2c_para[6] = {0};
	u32 table_wl = 0x5a5a5a5a;

	h2c_para[0] = 0xd; /* op_code, 0xd= wlan slot toggle-A*/
	h2c_para[1] = 0x1; /* no definition */

	if (btc->board_info.btdm_ant_num == 1) {
		if (table_case < chip_para->table_sant_num)
			table_wl = chip_para->table_sant[table_case].wl;
	} else {
		if (table_case < chip_para->table_nsant_num)
			table_wl = chip_para->table_nsant[table_case].wl;
	}

	/* tell WL FW WL slot toggle table-A*/
	h2c_para[2] = (u8)(table_wl & 0xff);
	h2c_para[3] = (u8)((table_wl & 0xff00) >> 8);
	h2c_para[4] = (u8)((table_wl & 0xff0000) >> 16);
	h2c_para[5] = (u8)((table_wl & 0xff000000) >> 24);

	btc->btc_fill_h2c(btc, 0x69, 6, h2c_para);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): H2C = [%02x %02x %02x %02x %02x %02x]\n",
		    __func__, h2c_para[0], h2c_para[1], h2c_para[2],
		    h2c_para[3], h2c_para[4], h2c_para[5]);
	BTC_TRACE(trace_buf);
}

static void
rtw_btc_wltoggle_tableB(IN struct btc_coexist *btc, IN boolean force_exec,
			IN u8 interval, IN u32 table)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8	cur_h2c_para[6] = {0};
	u8	i, match_cnt = 0;

	cur_h2c_para[0] = 0x7;	/* op_code, 0x7= wlan slot toggle-B*/
	cur_h2c_para[1] = interval;
	cur_h2c_para[2] = (u8)(table & 0xff);
	cur_h2c_para[3] = (u8)((table & 0xff00) >> 8);
	cur_h2c_para[4] = (u8)((table & 0xff0000) >> 16);
	cur_h2c_para[5] = (u8)((table & 0xff000000) >> 24);

	if (ARRAY_SIZE(coex_sta->wl_toggle_para) != 6)
		return;

	coex_sta->wl_toggle_interval = interval;

	for (i = 0; i <= 5; i++)
		coex_sta->wl_toggle_para[i] = cur_h2c_para[i];

	btc->btc_fill_h2c(btc, 0x69, 6, cur_h2c_para);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): H2C = [%02x %02x %02x %02x %02x %02x]\n",
		    __func__, cur_h2c_para[0], cur_h2c_para[1], cur_h2c_para[2],
		    cur_h2c_para[3], cur_h2c_para[4], cur_h2c_para[5]);
	BTC_TRACE(trace_buf);
}

static void
rtw_btc_set_table(struct btc_coexist *btc, boolean force_exec, u32 val0x6c0,
		  u32 val0x6c4)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;

	/* If last tdma is wl slot toggle, force write table*/
	if (!force_exec && coex_sta->coex_run_reason != BTC_RSN_LPS) {
		if (val0x6c0 == btc->btc_read_4byte(btc, REG_BT_COEX_TABLE0) &&
		    val0x6c4 == btc->btc_read_4byte(btc, REG_BT_COEX_TABLE1))
			return;
	}

	btc->btc_write_4byte(btc, REG_BT_COEX_TABLE0, val0x6c0);
	btc->btc_write_4byte(btc, REG_BT_COEX_TABLE1, val0x6c4);
	btc->btc_write_4byte(btc, REG_BT_COEX_BRK_TABLE, 0xf0ffffff);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): 0x6c0 = %x, 0x6c4 = %x\n",
		    __func__, val0x6c0, val0x6c4);
	BTC_TRACE(trace_buf);
}

static void
rtw_btc_table(struct btc_coexist *btc, boolean force_exec, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8 h2c_para[6] = {0};
	u32 table_wl = 0x0;

	coex_sta->coex_table_type = type;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], ***** Coex_Table - %d *****\n", type);
	BTC_TRACE(trace_buf);

	h2c_para[0] = 0xd;
	h2c_para[1] = 0x1;

	if (btc->board_info.btdm_ant_num == 1) {
		if (type < chip_para->table_sant_num)
			rtw_btc_set_table(btc, force_exec,
					  chip_para->table_sant[type].bt,
					  chip_para->table_sant[type].wl);
	} else {
		type = type - 100;
		if (type < chip_para->table_nsant_num)
			rtw_btc_set_table(btc, force_exec,
					  chip_para->table_nsant[type].bt,
					  chip_para->table_nsant[type].wl);
	}

	if (coex_sta->wl_slot_toggle_change)
		rtw_btc_wltoggle_tableA(btc, FC_EXCU, type);
}

static void
rtw_btc_ignore_wlan_act(struct btc_coexist *btc, boolean force_exec,
			boolean enable)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	u8 h2c_para[1] = {0};

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	if (!force_exec && enable == coex_dm->cur_ignore_wlan_act)
		return;

	if (enable)
		h2c_para[0] = 0x1; /* function enable */

	rtw_btc_mailbox_operation(btc, 0x63, 1, h2c_para);

	coex_dm->cur_ignore_wlan_act = enable;
}

static void
rtw_btc_lps_rpwm(struct btc_coexist *btc, boolean force_exec, u8 lps_val,
		 u8 rpwm_val)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;

	if (!force_exec) {
		if (lps_val == coex_dm->cur_lps &&
		    rpwm_val == coex_dm->cur_rpwm)
			return;
	}

	btc->btc_set(btc, BTC_SET_U1_LPS_VAL, &lps_val);
	btc->btc_set(btc, BTC_SET_U1_RPWM_VAL, &rpwm_val);

	coex_dm->cur_lps = lps_val;
	coex_dm->cur_rpwm = rpwm_val;
}

static void
rtw_btc_power_save_state(struct btc_coexist *btc, u8 ps_type, u8 lps_val,
			 u8 rpwm_val)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	boolean low_pwr_dis = FALSE;
	u8 lps_mode = 0x0;
	u8 h2c_para[5] = {0, 0, 0, 0, 0};

	btc->btc_get(btc, BTC_GET_U1_LPS_MODE, &lps_mode);

	switch (ps_type) {
	case BTC_PS_WIFI_NATIVE:
		/* recover to original 32k low power setting */
		coex_sta->wl_force_lps_ctrl = FALSE;
		btc->btc_set(btc, BTC_SET_ACT_PRE_NORMAL_LPS, NULL);

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BTC_PS_WIFI_NATIVE\n", __func__);
		break;
	case BTC_PS_LPS_ON:
		coex_sta->wl_force_lps_ctrl = TRUE;
		/*set tdma off if LPS off  */
		if (!lps_mode)
			btc->btc_fill_h2c(btc, 0x60, 5, h2c_para);
		rtw_btc_lps_rpwm(btc, NM_EXCU, lps_val, rpwm_val);
		/* when coex force to enter LPS, do not enter 32k low power. */
		low_pwr_dis = TRUE;
		btc->btc_set(btc, BTC_SET_ACT_DISABLE_LOW_POWER, &low_pwr_dis);
		/* power save must executed before psTdma. */
		btc->btc_set(btc, BTC_SET_ACT_ENTER_LPS, NULL);

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BTC_PS_LPS_ON\n", __func__);
		break;
	case BTC_PS_LPS_OFF:
		coex_sta->wl_force_lps_ctrl = TRUE;
		/*set tdma off if LPS on  */
		if (lps_mode)
			btc->btc_fill_h2c(btc, 0x60, 5, h2c_para);
		if (btc->btc_set(btc, BTC_SET_ACT_LEAVE_LPS, NULL))
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], %s(): BTC_PS_LPS_OFF\n",
				    __func__);
		else
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], %s(): BTC_PS_LPS_OFF Fail!!\n",
				    __func__);
		break;
	default:
		break;
	}

	BTC_TRACE(trace_buf);
}

static void
rtw_btc_set_tdma(struct btc_coexist *btc, u8 byte1, u8 byte2, u8 byte3,
		 u8 byte4, u8 byte5)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_wifi_link_info_ext *linfo_ext = &btc->wifi_link_info_ext;
	u8 ps_type = BTC_PS_WIFI_NATIVE,
	   real_byte1 = byte1, real_byte5 = byte5;

	if (linfo_ext->is_ap_mode && (byte1 & BIT(4) && !(byte1 & BIT(5)))) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): AP mode\n", __func__);
		BTC_TRACE(trace_buf);

		real_byte1 &= ~BIT(4);
		real_byte1 |= BIT(5);

		real_byte5 |= BIT(5);
		real_byte5 &= ~BIT(6);

		ps_type = BTC_PS_WIFI_NATIVE;
		rtw_btc_power_save_state(btc, ps_type, 0x0, 0x0);
	} else if (byte1 & BIT(4) && !(byte1 & BIT(5))) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): Force LPS (byte1 = 0x%x)\n",
			    __func__, byte1);
		BTC_TRACE(trace_buf);

		if (btc->chip_para->pstdma_type == BTC_PSTDMA_FORCE_LPSOFF)
			ps_type = BTC_PS_LPS_OFF;
		else
			ps_type = BTC_PS_LPS_ON;
		rtw_btc_power_save_state(btc, ps_type, 0x50, 0x4);
	} else {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): native power save (byte1 = 0x%x)\n",
			    __func__, byte1);
		BTC_TRACE(trace_buf);

		ps_type = BTC_PS_WIFI_NATIVE;
		rtw_btc_power_save_state(btc, ps_type, 0x0, 0x0);
	}

	coex_dm->ps_tdma_para[0] = real_byte1;
	coex_dm->ps_tdma_para[1] = byte2;
	coex_dm->ps_tdma_para[2] = byte3;
	coex_dm->ps_tdma_para[3] = byte4;
	coex_dm->ps_tdma_para[4] = real_byte5;

	btc->btc_fill_h2c(btc, 0x60, 5, coex_dm->ps_tdma_para);

	/* Always forec excute rtw_btc_set_table To avoid
	 * coex table error if wl slot toggle mode on ->off
	 * ex: 5508031054 next state -> rtw_btc_table + 5108031054
	 * rtw_btc_table may be changed by 5508031054
	 */
	if (real_byte1 & BIT(2)) {
		coex_sta->wl_slot_toggle = TRUE;
		coex_sta->wl_slot_toggle_change = FALSE;
	} else {
		coex_sta->wl_slot_toggle_change = coex_sta->wl_slot_toggle;
		coex_sta->wl_slot_toggle = FALSE;
	}

	if (ps_type == BTC_PS_WIFI_NATIVE)
		btc->btc_set(btc, BTC_SET_ACT_POST_NORMAL_LPS, NULL);
}

static
void rtw_btc_tdma(struct btc_coexist *btc, boolean force_exec, u32 tcase)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8 type;
	boolean turn_on;

	btc->btc_set_atomic(btc, &coex_dm->setting_tdma, TRUE);
	/* tcase: bit0~7 --> tdma case index
	 *        bit8   --> for 4-slot (50ms) mode
	 */

	if (tcase & TDMA_4SLOT)/* 4-slot (50ms) mode */
		rtw_btc_set_tdma_timer_base(btc, 3);
	else
		rtw_btc_set_tdma_timer_base(btc, 0);

	type = (u8)(tcase & 0xff);
	turn_on = (type == 0 || type == 100) ? FALSE : TRUE;

	/* To avoid TDMA H2C fail before Last LPS enter  */
	if (!force_exec && coex_sta->coex_run_reason != BTC_RSN_LPS) {
		if (turn_on == coex_dm->cur_ps_tdma_on &&
		    type == coex_dm->cur_ps_tdma) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], Skip TDMA because no change TDMA(%s, %d)\n",
				    (coex_dm->cur_ps_tdma_on ? "on" : "off"),
				    coex_dm->cur_ps_tdma);
			BTC_TRACE(trace_buf);

			btc->btc_set_atomic(btc, &coex_dm->setting_tdma, FALSE);
			return;
		}
	}

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], ***** TDMA - %d *****\n", type);
	BTC_TRACE(trace_buf);

	/* TRUE -> Page scan > ACL */
	if (!turn_on ||
	    (coex_sta->bt_a2dp_exist && coex_sta->bt_inq_page_remain))
		btc->btc_write_scbd(btc, BTC_SCBD_TDMA, FALSE);
	else
		btc->btc_write_scbd(btc, BTC_SCBD_TDMA, TRUE);

	if (btc->board_info.btdm_ant_num == 1) {
		if (type < chip_para->tdma_sant_num)
			rtw_btc_set_tdma(btc,
					 chip_para->tdma_sant[type].para[0],
					 chip_para->tdma_sant[type].para[1],
					 chip_para->tdma_sant[type].para[2],
					 chip_para->tdma_sant[type].para[3],
					 chip_para->tdma_sant[type].para[4]);
	} else {
		type = type - 100;
		if (type < chip_para->tdma_nsant_num)
			rtw_btc_set_tdma(btc,
					 chip_para->tdma_nsant[type].para[0],
					 chip_para->tdma_nsant[type].para[1],
					 chip_para->tdma_nsant[type].para[2],
					 chip_para->tdma_nsant[type].para[3],
					 chip_para->tdma_nsant[type].para[4]);
	}

	coex_dm->cur_ps_tdma_on = turn_on;
	coex_dm->cur_ps_tdma = type;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "change TDMA(%s, %d)\n",
		    (coex_dm->cur_ps_tdma_on ? "on" : "off"),
		    coex_dm->cur_ps_tdma);
	BTC_TRACE(trace_buf);

	btc->btc_set_atomic(btc, &coex_dm->setting_tdma, FALSE);
}

static
void rtw_btc_set_ant_switch(struct btc_coexist *btc, boolean force_exec,
			    u8 ctrl_type, u8 pos_type)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;

	if (!force_exec) {
		if (((ctrl_type << 8) + pos_type) == coex_dm->cur_switch_status)
			return;
	}

	coex_dm->cur_switch_status = (ctrl_type << 8) + pos_type;

	btc->chip_para->chip_setup(btc, BTC_CSETUP_ANT_SWITCH);
}

static
void rtw_btc_set_ant_path(struct btc_coexist *btc, boolean force_exec,
			  u8 phase)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_rfe_type *rfe_type = &btc->rfe_type;
	u8 ctrl_type = BTC_SWITCH_CTRL_MAX,
	   pos_type = BTC_SWITCH_TO_MAX, cnt = 0;
	u16 scbd = 0;
	boolean is_btk, is_wlk;

	if (!force_exec && coex_dm->cur_ant_pos_type == phase)
		return;

	coex_dm->cur_ant_pos_type = phase;

	/* To avoid switch coex_ctrl_owner during BT IQK */
	if (rfe_type->wlg_at_btg && btc->chip_para->scbd_support &&
	    coex_sta->bt_iqk_state != 0xff) {

		/* BT RFK  */
		is_btk = ((btc->btc_read_scbd(btc, &scbd) & BIT(5)) == BIT(5));

		/* WL RFK */
		is_wlk = ((btc->btc_read_1byte(btc, 0x49c) & BIT(0)) == BIT(0));

		while (++cnt < 12 && (is_btk || is_wlk)) {
			delay_ms(50);
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], Ant Setup Delay by IQK\n, wlk=%d, btk=%d, cnt=%d\n",
				    is_wlk, is_btk, cnt);
			BTC_TRACE(trace_buf);
		}
		/*  wait timeout */
		if (cnt >= 12)
			coex_sta->bt_iqk_state = 0xff;
	}

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex],  coex_sta->bt_disabled = 0x%x\n",
		    coex_sta->bt_disabled);
	BTC_TRACE(trace_buf);

	switch (phase) {
	case BTC_ANT_POWERON:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_COEX_POWERON\n", __func__);
		BTC_TRACE(trace_buf);

		/* set Path control owner to BT at power-on step */
		if (coex_sta->bt_disabled)
			rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);
		else
			rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_BT);

		/*Caution: Don't indirect access while power on phase   */

		ctrl_type = BTC_SWITCH_CTRL_BY_BBSW;
		pos_type = BTC_SWITCH_TO_BT;
		break;
	case BTC_ANT_INIT:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_COEX_INIT\n", __func__);
		BTC_TRACE(trace_buf);

		if (coex_sta->bt_disabled) {
			/* set GNT_BT to SW low */
			rtw_btc_set_gnt_bt(btc, BTC_GNT_SW_LOW);
			/* set GNT_WL to SW high */
			rtw_btc_set_gnt_wl(btc, BTC_GNT_SW_HIGH);
		} else {
			/* set GNT_BT to SW high */
			rtw_btc_set_gnt_bt(btc, BTC_GNT_SW_HIGH);
			/* set GNT_WL to SW low */
			rtw_btc_set_gnt_wl(btc, BTC_GNT_SW_LOW);
		}

		/* set Path control owner to WL at initial step */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		ctrl_type = BTC_SWITCH_CTRL_BY_BBSW;
		pos_type = BTC_SWITCH_TO_BT;
		break;
	case BTC_ANT_WONLY:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_WLANONLY_INIT\n", __func__);
		BTC_TRACE(trace_buf);

		/* set GNT_BT to SW Low */
		rtw_btc_set_gnt_bt(btc, BTC_GNT_SW_LOW);
		/* Set GNT_WL to SW high */
		rtw_btc_set_gnt_wl(btc, BTC_GNT_SW_HIGH);
		/* set Path control owner to WL at initial step */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		ctrl_type = BTC_SWITCH_CTRL_BY_BBSW;
		pos_type = BTC_SWITCH_TO_WLG;
		break;
	case BTC_ANT_WOFF:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_WLAN_OFF\n", __func__);
		BTC_TRACE(trace_buf);

		/* set Path control owner to BT */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_BT);

		ctrl_type = BTC_SWITCH_CTRL_BY_BT;
		pos_type = BTC_SWITCH_TO_NOCARE;
		break;
	case BTC_ANT_2G:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_2G_RUNTIME\n", __func__);
		BTC_TRACE(trace_buf);

		/* set GNT_BT to PTA */
		rtw_btc_set_gnt_bt(btc, BTC_GNT_HW_PTA);
		/* Set GNT_WL to PTA */
		rtw_btc_set_gnt_wl(btc, BTC_GNT_HW_PTA);

		/* set Path control owner to WL at runtime step */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		ctrl_type = BTC_SWITCH_CTRL_BY_PTA;
		pos_type = BTC_SWITCH_TO_NOCARE;
		break;
	case BTC_ANT_5G:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_5G_RUNTIME\n", __func__);
		BTC_TRACE(trace_buf);

		/* set GNT_BT to SW PTA */
		rtw_btc_set_gnt_bt(btc, BTC_GNT_HW_PTA);
		/* Set GNT_WL to SW Hi */
		rtw_btc_set_gnt_wl(btc, BTC_GNT_SW_HIGH);

		/* set Path control owner to WL at runtime step */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		ctrl_type = BTC_SWITCH_CTRL_BY_BBSW;
		pos_type = BTC_SWITCH_TO_WLA;
		break;
	case BTC_ANT_2G_FREERUN:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_2G_FREERUN\n", __func__);
		BTC_TRACE(trace_buf);

		/* set GNT_BT to SW PTA */
		rtw_btc_set_gnt_bt(btc, BTC_GNT_HW_PTA);

		/* Set GNT_WL to SW Hi */
		rtw_btc_set_gnt_wl(btc, BTC_GNT_SW_HIGH);

		/* set Path control owner to WL at runtime step */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		ctrl_type = BTC_SWITCH_CTRL_BY_BBSW;
		pos_type = BTC_SWITCH_TO_WLG_BT;
		break;
	case BTC_ANT_2G_WLBT:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_2G_WLBT\n", __func__);
		BTC_TRACE(trace_buf);

		/* set GNT_BT to HW PTA */
		rtw_btc_set_gnt_bt(btc, BTC_GNT_HW_PTA);
		/* Set GNT_WL to HW PTA */
		rtw_btc_set_gnt_wl(btc, BTC_GNT_HW_PTA);
		/* set Path control owner to WL at runtime step */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		ctrl_type = BTC_SWITCH_CTRL_BY_BBSW;
		pos_type = BTC_SWITCH_TO_WLG_BT;
		break;
	case BTC_ANT_2G_WL:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_2G_WL\n", __func__);
		BTC_TRACE(trace_buf);

		/* set GNT_BT to PTA */
		rtw_btc_set_gnt_bt(btc, BTC_GNT_HW_PTA);
		/* Set GNT_WL to PTA */
		rtw_btc_set_gnt_wl(btc, BTC_GNT_HW_PTA);
		/* set Path control owner to WL at runtime step */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		ctrl_type = BTC_SWITCH_CTRL_BY_BBSW;
		pos_type = BTC_SWITCH_TO_WLG;
		break;
	case BTC_ANT_2G_BT:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_2G_WL\n", __func__);
		BTC_TRACE(trace_buf);

		/* set GNT_BT to PTA */
		rtw_btc_set_gnt_bt(btc, BTC_GNT_HW_PTA);
		/* Set GNT_WL to PTA */
		rtw_btc_set_gnt_wl(btc, BTC_GNT_HW_PTA);
		/* set Path control owner to WL at runtime step */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		ctrl_type = BTC_SWITCH_CTRL_BY_BBSW;
		pos_type = BTC_SWITCH_TO_BT;
		break;
	case BTC_ANT_BTMP:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_BTMP\n", __func__);
		BTC_TRACE(trace_buf);

		/* set GNT_BT to SW Hi */
		rtw_btc_set_gnt_bt(btc, BTC_GNT_SW_HIGH);
		/* Set GNT_WL to SW Lo */
		rtw_btc_set_gnt_wl(btc, BTC_GNT_SW_LOW);
		/* set Path control owner to WL */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		btc->stop_coex_dm = TRUE;

		ctrl_type = BTC_SWITCH_CTRL_BY_BBSW;
		pos_type = BTC_SWITCH_TO_BT;
		break;
	case BTC_ANT_MCC:
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s() - PHASE_MCC\n", __func__);
		BTC_TRACE(trace_buf);

		/* set GNT_BT to PTA */
		rtw_btc_set_gnt_bt(btc, BTC_GNT_HW_PTA);
		/* Set GNT_WL to PTA */
		rtw_btc_set_gnt_wl(btc, BTC_GNT_HW_PTA);
		/* set Path control owner to WL at runtime step */
		rtw_btc_coex_ctrl_owner(btc, BTC_OWNER_WL);

		ctrl_type = BTC_SWITCH_CTRL_BY_FW;
		pos_type = BTC_SWITCH_TO_NOCARE;
		break;
	}

	if (ctrl_type < BTC_SWITCH_CTRL_MAX && pos_type < BTC_SWITCH_TO_MAX &&
	    rfe_type->ant_switch_exist)
		rtw_btc_set_ant_switch(btc, force_exec, ctrl_type, pos_type);
}

static u8 rtw_btc_algorithm(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 algorithm = BTC_COEX_NOPROFILE;
	u8 profile_map = 0;

	if (coex_sta->bt_hfp_exist)
		profile_map = profile_map | BTC_BTPROFILE_HFP;

	if (coex_sta->bt_hid_exist)
		profile_map = profile_map | BTC_BTPROFILE_HID;

	if (coex_sta->bt_a2dp_exist)
		profile_map = profile_map | BTC_BTPROFILE_A2DP;

	if (coex_sta->bt_pan_exist)
		profile_map = profile_map | BTC_BTPROFILE_PAN;

	switch (profile_map) {
	case BTC_BTPROFILE_NONE:
		algorithm = BTC_COEX_NOPROFILE;
		break;
	case BTC_BTPROFILE_HFP:
		algorithm = BTC_COEX_HFP;
		break;
	case BTC_BTPROFILE_HID:
		algorithm = BTC_COEX_HID;
		break;
	case (BTC_BTPROFILE_HID | BTC_BTPROFILE_HFP):
		algorithm = BTC_COEX_HID;
		break;
	case BTC_BTPROFILE_A2DP:
		/* OPP may disappear during CPT_for_WiFi test */
		if (coex_sta->bt_multi_link && coex_sta->bt_hid_pair_num > 0)
			algorithm = BTC_COEX_A2DP_HID;
		else if (coex_sta->bt_multi_link)
			algorithm = BTC_COEX_A2DP_PAN;
		else
			algorithm = BTC_COEX_A2DP;
		break;
	case (BTC_BTPROFILE_A2DP | BTC_BTPROFILE_HFP):
		algorithm = BTC_COEX_A2DP_HID;
		break;
	case (BTC_BTPROFILE_A2DP | BTC_BTPROFILE_HID):
		algorithm = BTC_COEX_A2DP_HID;
		break;
	case (BTC_BTPROFILE_A2DP | BTC_BTPROFILE_HID | BTC_BTPROFILE_HFP):
		algorithm = BTC_COEX_A2DP_HID;
		break;
	case BTC_BTPROFILE_PAN:
		algorithm = BTC_COEX_PAN;
		break;
	case (BTC_BTPROFILE_PAN | BTC_BTPROFILE_HFP):
		algorithm = BTC_COEX_PAN_HID;
		break;
	case (BTC_BTPROFILE_PAN | BTC_BTPROFILE_HID):
		algorithm = BTC_COEX_PAN_HID;
		break;
	case (BTC_BTPROFILE_PAN | BTC_BTPROFILE_HID | BTC_BTPROFILE_HFP):
		algorithm = BTC_COEX_PAN_HID;
		break;
	case (BTC_BTPROFILE_PAN | BTC_BTPROFILE_A2DP):
		algorithm = BTC_COEX_A2DP_PAN;
		break;
	case (BTC_BTPROFILE_PAN | BTC_BTPROFILE_A2DP | BTC_BTPROFILE_HFP):
		algorithm = BTC_COEX_A2DP_PAN_HID;
		break;
	case (BTC_BTPROFILE_PAN | BTC_BTPROFILE_A2DP | BTC_BTPROFILE_HID):
		algorithm = BTC_COEX_A2DP_PAN_HID;
		break;
	case BTC_BTPROFILE_MAX:
		algorithm = BTC_COEX_A2DP_PAN_HID;
		break;
	}

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], BT Profile = %s => Algorithm = %s\n",
		    bt_profile_string[profile_map],
		    coex_algo_string[algorithm]);
	BTC_TRACE(trace_buf);

	return algorithm;
}

static void rtw_btc_action_coex_all_off(struct btc_coexist *btc)
{
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	/* To avoid  rtw_btc_set_ant_path here */
	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		table_case = 2;
		tdma_case = 0;
	} else { /* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_freerun(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *cpara = btc->chip_para;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	u8 level = 0, i;
	boolean bt_afh_loss = TRUE;

	if (btc->board_info.btdm_ant_num != 2)
		return;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	coex_sta->coex_freerun = TRUE;

	for (i = 0; i <= 8; i++) {
		if (coex_sta->bt_afh_map[i] != 0xff) {
			bt_afh_loss = FALSE;
			break;
		}
	}

	if (bt_afh_loss)
		rtw_btc_update_wl_ch_info(btc, BTC_MEDIA_CONNECT);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G_FREERUN);

	btc->btc_write_scbd(btc, BTC_SCBD_FIX2M, FALSE);

	/* decrease more BT Tx power for clear case */
	if (BTC_RSSI_HIGH(coex_dm->wl_rssi_state[0]))
		level = 2;
	else if (BTC_RSSI_HIGH(coex_dm->wl_rssi_state[1]))
		level = 3;
	else if (BTC_RSSI_HIGH(coex_dm->wl_rssi_state[2]))
		level = 4;
	else
		level = 5;

	if (level > cpara->wl_rf_para_tx_num - 1)
		level = cpara->wl_rf_para_tx_num - 1;

	if (coex_sta->wl_coex_mode != BTC_WLINK_2G1PORT)
		rtw_btc_set_rf_para(btc, NM_EXCU, cpara->wl_rf_para_rx[0]);
	else if (link_info_ext->traffic_dir == BTC_WIFI_TRAFFIC_TX)
		rtw_btc_set_rf_para(btc, NM_EXCU, cpara->wl_rf_para_tx[level]);
	else
		rtw_btc_set_rf_para(btc, NM_EXCU, cpara->wl_rf_para_rx[level]);

	rtw_btc_table(btc, NM_EXCU, 100);
	rtw_btc_tdma(btc, NM_EXCU, 100);
}

static void rtw_btc_action_rf4ce(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_rf_para(btc, NM_EXCU, chip_para->wl_rf_para_rx[0]);
	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);

	switch (coex_sta->ext_chip_mode) {
	case 0:
		table_case = 112;
		tdma_case = 115;

		if (coex_sta->bt_slave)
			rtw_btc_set_extend_btautoslot(btc, 0x3c);
		else
			rtw_btc_set_extend_btautoslot(btc, 0x32);

		rtw_btc_table(btc, NM_EXCU, table_case);
		rtw_btc_tdma(btc, NM_EXCU, tdma_case);
		break;
	case 1:
		table_case = 112;
		tdma_case = 121;

		rtw_btc_table(btc, NM_EXCU, table_case);
		rtw_btc_tdma(btc, NM_EXCU, tdma_case);
		break;
	}
}

static void rtw_btc_action_ext_chip(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (btc->board_info.ext_chip_id == BTC_EXT_CHIP_RF4CE)
		rtw_btc_action_rf4ce(btc);
}

u8 rtw_btc_action_rf4ce_new_tdma(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	switch (type) {
	case 0: /*BT idle*/
		if (coex_sta->ext_chip_mode == BTC_EXTMODE_VOICE)
			tdma_case = 121;
		else
			tdma_case = 117;
		break;
	case 1: /*BT relink*/
		if (coex_sta->ext_chip_mode == BTC_EXTMODE_VOICE)
			tdma_case = 121;
		else
			tdma_case = 117;
		break;
	case 2: /*WIFI linkscan*/
		if (coex_sta->ext_chip_mode == BTC_EXTMODE_VOICE) {
			tdma_case = 125;
		} else{
			if (coex_sta->bt_slave)
				rtw_btc_set_extend_btautoslot(btc, 0x3c);
			else
				rtw_btc_set_extend_btautoslot(btc, 0x32);

			tdma_case = 124;
		}
		break;
	case 3: /*WIFI only*/
		if (coex_sta->ext_chip_mode == BTC_EXTMODE_VOICE) {
			tdma_case = 121;
		} else{
			if (coex_sta->bt_slave)
				rtw_btc_set_extend_btautoslot(btc, 0x3c);
			else
				rtw_btc_set_extend_btautoslot(btc, 0x32);

			tdma_case = 115;
		}
		break;
	default:
		tdma_case = 0;
		break;
	}
	return tdma_case;
}

u8 rtw_btc_ext_chip_new_tdma(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 tdma_case = 0;

	if (btc->board_info.ext_chip_id == BTC_EXT_CHIP_RF4CE)
		tdma_case = rtw_btc_action_rf4ce_new_tdma(btc, type);

	return tdma_case;
}

static void rtw_btc_action_bt_whql_test(struct btc_coexist *btc)
{
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		table_case = 2;
		tdma_case = 0;
	} else { /* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_bt_relink(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;
	u32 slot_type = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (coex_sta->wl_gl_busy) {
			table_case = 26;

			if (coex_sta->bt_hid_exist &&
			    coex_sta->bt_profile_num == 1) {
				slot_type = TDMA_4SLOT;
				tdma_case = 20;
			} else {
				tdma_case = 20;
			}
		} else {
			table_case = 1;
			tdma_case = 0;
		}
	} else { /* Non-Shared-Ant */
		if (coex_sta->wl_gl_busy)
			table_case = 115;
		else
			table_case = 100;
		tdma_case = 100;

		if (coex_sta->wl_gl_busy &&
		    btc->board_info.ext_chip_id != BTC_EXT_CHIP_NONE)
			tdma_case = rtw_btc_ext_chip_new_tdma(btc, 1);
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case | slot_type);
}

static void rtw_btc_action_bt_idle(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_rfe_type *rfe_type = &btc->rfe_type;
	struct btc_wifi_link_info *link_info = &btc->wifi_link_info;
	u8 table_case = 0xff, tdma_case = 0xff;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (rfe_type->ant_switch_with_bt &&
	    coex_dm->bt_status == BTC_BTSTATUS_NCON_IDLE) {
		if (btc->board_info.btdm_ant_num == 1 &&
		    BTC_RSSI_HIGH(coex_dm->wl_rssi_state[3]) &&
		    coex_sta->wl_gl_busy) {
			table_case = 0;
			tdma_case = 0;
		} else if (btc->board_info.btdm_ant_num == 2) {
			table_case = 100;
			tdma_case = 100;
		}

		if (table_case != 0xff && tdma_case != 0xff) {
			rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G_FREERUN);
			goto exit;
		}
	}

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);

#ifndef PLATFORM_WINDOWS
	if (coex_sta->wl_noisy_level > 0) {
		if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
			table_case = 1;
			tdma_case = 0;
		} else { /* Non-Shared-Ant */
			table_case = 123;
			tdma_case = 0;
		}
		goto exit;
	}
#endif

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (!coex_sta->wl_gl_busy) {
			table_case = 10;
			tdma_case = 3;
		} else if (coex_sta->bt_mesh) {
			table_case = 26;
			tdma_case = 7;
		} else if (coex_dm->bt_status == BTC_BTSTATUS_NCON_IDLE) {
			table_case = 11;

			if (coex_sta->bt_ctr_ok &&
			    (coex_sta->lo_pri_rx + coex_sta->lo_pri_tx > 250))
				tdma_case = 17;
			else
				tdma_case = 7;
		} else {
			table_case = 12;
			tdma_case = 7;
		}
	} else { /* Non-Shared-Ant */
		if (!coex_sta->wl_gl_busy) {
			table_case = 112;
			tdma_case = 104;
		} else if ((coex_sta->bt_ble_scan_type & 0x2) &&
			    coex_dm->bt_status == BTC_BTSTATUS_NCON_IDLE) {
			table_case = 114;
			tdma_case = 103;
		} else {
			table_case = 112;
			tdma_case = 103;
		}
		if (coex_sta->wl_gl_busy &&
		    btc->board_info.ext_chip_id != BTC_EXT_CHIP_NONE)
			tdma_case = rtw_btc_ext_chip_new_tdma(btc, 0);
	}

exit:
	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_bt_inquiry(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	boolean wl_hi_pri = FALSE;
	u8 table_case, tdma_case;
	u32 slot_type = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (coex_sta->wl_linkscan_proc || coex_sta->wl_hi_pri_task1 ||
	    coex_sta->wl_hi_pri_task2)
		wl_hi_pri = TRUE;

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (wl_hi_pri) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], bt inq/page +  wifi hi-pri task\n");

			table_case = 15;

			if (coex_sta->bt_profile_num > 0)
				tdma_case = 10;
			else if (coex_sta->wl_hi_pri_task1)
				tdma_case = 6;
			else if (!coex_sta->bt_page)
				tdma_case = 8;
			else
				tdma_case = 9;
		} else if (coex_sta->wl_gl_busy) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], bt inq/page +  wifi busy\n");
#if 0
			table_case = 15;
			tdma_case = 20;
#else
			if (coex_sta->bt_profile_num == 0) {
				table_case = 12;
				tdma_case = 18;
			} else if (coex_sta->bt_profile_num == 1 &&
				   !coex_sta->bt_a2dp_exist) {
				slot_type = TDMA_4SLOT;
				table_case = 12;
				tdma_case = 20;
			} else {
				slot_type = TDMA_4SLOT;
				table_case = 12;
				tdma_case = 26;
			}
#endif
		} else if (link_info_ext->is_connected) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], bt inq/page +  wifi connected\n");

			table_case = 9;
			tdma_case = 27;
		} else {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], bt inq/page +  wifi not-connected\n");

			table_case = 1;
			tdma_case = 0;
		}
	} else { /* Non_Shared-Ant */
		if (wl_hi_pri) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], bt inq/page +  wifi hi-pri task\n");

			table_case = 114;

			if (coex_sta->bt_profile_num > 0)
				tdma_case = 110;
			else if (coex_sta->wl_hi_pri_task1)
				tdma_case = 106;
			else if (!coex_sta->bt_page)
				tdma_case = 108;
			else
				tdma_case = 109;
		} else if (coex_sta->wl_gl_busy) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], bt inq/page +  wifi busy\n");

			table_case = 114;
			tdma_case = 121;
		} else if (link_info_ext->is_connected) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], bt inq/page +  wifi connected\n");

			table_case = 101;
			tdma_case = 100;
		} else {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], bt inq/page +  wifi not-connected\n");

			table_case = 101;
			tdma_case = 100;
		}
	}

	BTC_TRACE(trace_buf);

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case | slot_type);
}

static void rtw_btc_action_bt_hfp(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
#ifdef PLATFORM_WINDOWS
		if (coex_sta->wl_cck_lock_ever) {
			coex_sta->wl_coex_mode = BTC_WLINK_2GFREE;
			table_case = 33;
			tdma_case = 0;
		} else
#endif
		if (coex_sta->bt_multi_link) {
			table_case = 10;
			tdma_case = 17;
		} else {
			table_case = 10;
			tdma_case = 5;
		}
	} else { /* Non-Shared-Ant */
		if (coex_sta->bt_multi_link) {
			table_case = 112;
			tdma_case = 117;
		} else {
			table_case = 105;
			tdma_case = 100;
		}
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_bt_hid(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;
	boolean is_toggle_table = FALSE, is_bt_ctr_hi = FALSE;
	u32 slot_type = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (coex_sta->bt_ctr_ok &&
	    (coex_sta->lo_pri_rx + coex_sta->lo_pri_tx > 360))
		is_bt_ctr_hi = TRUE;

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
#ifdef PLATFORM_WINDOWS
		if (coex_sta->wl_cck_lock_ever) {
			coex_sta->wl_coex_mode = BTC_WLINK_2GFREE;
			table_case = 33;
			tdma_case = 0;
		} else
#endif
		if (coex_sta->bt_ble_exist) { /* RCU */
			table_case = 26;
			tdma_case = 2;
		} else { /* Legacy HID  */
			if (coex_sta->bt_profile_num == 1 &&
			    (coex_sta->bt_multi_link ||
			     is_bt_ctr_hi ||
			     coex_sta->bt_slave ||
			     coex_sta->bt_multi_link_remain)) {
				slot_type = TDMA_4SLOT;

				if (coex_sta->wl_gl_busy &&
				    (coex_sta->wl_rx_rate <= 3 ||
				    coex_sta->wl_rts_rx_rate <= 3))
					table_case = 13;
				else
					table_case = 12;

				tdma_case = 26;
			} else if (coex_sta->bt_a2dp_active) {
				table_case = 9;
				tdma_case = 18;
			} else if (coex_sta->bt_418_hid_exist &&
				   coex_sta->wl_gl_busy) {
				slot_type = TDMA_4SLOT;
				table_case = 32;
				tdma_case = 27;
			} else if (coex_sta->bt_ble_hid_exist &&
				   coex_sta->wl_gl_busy) {
				table_case = 32;
				tdma_case = 9;
			} else {
				table_case = 9;
				tdma_case = 9;
			}
		}
	} else { /* Non-Shared-Ant */
		if (coex_sta->bt_ble_exist) { /* BLE */
			table_case = 110;
			tdma_case = 105;
		} else if (coex_sta->bt_a2dp_active) {
			table_case = 113;
			tdma_case = 118;
		} else {
			table_case = 113;
			tdma_case = 104;
		}
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	if (is_toggle_table) {
		rtw_btc_wltoggle_tableA(btc, FC_EXCU, table_case);
		rtw_btc_wltoggle_tableB(btc, NM_EXCU, 1, 0x5a5a5aaa);
	}

	rtw_btc_tdma(btc, NM_EXCU, tdma_case | slot_type);
}

static void rtw_btc_action_bt_a2dp(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	u8 table_case, tdma_case;
	u32 slot_type = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	slot_type = TDMA_4SLOT;

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (coex_sta->wl_gl_busy && coex_sta->wl_noisy_level == 0)
			table_case = 12;
		else
			table_case = 9;

		if (coex_sta->wl_connecting || !coex_sta->wl_gl_busy)
			tdma_case = 14;
		else
			tdma_case = 13;
	} else { /* Non-Shared-Ant */
		table_case = 121;
		tdma_case = 113;
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case | slot_type);
}

static void rtw_btc_action_bt_a2dpsink(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_wifi_link_info_ext *linfo_ext = &btc->wifi_link_info_ext;
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (linfo_ext->is_ap_mode) {
			table_case = 2;
			tdma_case = 0;
		} else if (coex_sta->wl_gl_busy) {
			table_case = 28;
			tdma_case = 20;
		} else {
			table_case = 28;
			tdma_case = 26;
		}
	} else { /* Non-Shared-Ant */
		if (linfo_ext->is_ap_mode) {
			table_case = 100;
			tdma_case = 100;
		} else {
			table_case = 119;
			tdma_case = 120;
		}
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_bt_pan(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (coex_sta->wl_gl_busy && coex_sta->wl_noisy_level == 0)
			table_case = 14;
		else
			table_case = 10;

		if (coex_sta->wl_gl_busy)
			tdma_case = 17;
		else
			tdma_case = 20;
	} else { /* Non-Shared-Ant */
		table_case = 112;

		if (coex_sta->wl_gl_busy)
			tdma_case = 117;
		else
			tdma_case = 119;
	}

	if (coex_sta->bt_slave && coex_sta->wl_gl_busy)
		rtw_btc_set_bt_golden_rx_range(btc, NM_EXCU, 3, 20);
	else
		rtw_btc_set_bt_golden_rx_range(btc, NM_EXCU, 3, 0);

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_bt_a2dp_hid(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	u8 table_case, tdma_case;
	boolean is_toggle_table = FALSE;
	u32 slot_type = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (coex_sta->wl_iot_peer != BTC_IOT_PEER_CISCO)
		slot_type = TDMA_4SLOT;

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (coex_sta->bt_ble_exist)
			table_case = 26; /* for RCU */
		else
			table_case = 9;

		if (coex_sta->wl_connecting || !coex_sta->wl_gl_busy) {
			tdma_case = 14;
		} else if (coex_sta->bt_418_hid_exist ||
			   coex_sta->bt_ble_hid_exist) {
			is_toggle_table = TRUE;
			tdma_case = 23;
		} else {
			tdma_case = 13;
		}
	} else { /* Non-Shared-Ant */
		if (coex_sta->bt_ble_exist)
			table_case = 110;
		else
			table_case = 121;

		tdma_case = 113;
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	if (is_toggle_table) {
		rtw_btc_wltoggle_tableA(btc, FC_EXCU, table_case);
		rtw_btc_wltoggle_tableB(btc, NM_EXCU, 1, 0x5a5a5aaa);
	}

	rtw_btc_tdma(btc, NM_EXCU, tdma_case | slot_type);
}

static void rtw_btc_action_bt_a2dp_pan(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8 table_case, tdma_case;
	boolean wl_cpt_test = FALSE, bt_cpt_test = FALSE;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	if (btc->board_info.customer_id == RT_CID_LENOVO_CHINA &&
	    coex_sta->cnt_wl[BTC_CNT_WL_SCANAP] <= 10 &&
	    coex_sta->wl_iot_peer == BTC_IOT_PEER_ATHEROS) {
		if (BTC_RSSI_LOW(coex_dm->wl_rssi_state[2]))
			wl_cpt_test = TRUE;
		else
			bt_cpt_test = TRUE;
	}

	if (wl_cpt_test)
		rtw_btc_set_rf_para(btc, NM_EXCU, chip_para->wl_rf_para_rx[1]);
	else
		rtw_btc_set_rf_para(btc, NM_EXCU, chip_para->wl_rf_para_rx[0]);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (wl_cpt_test) {
			if (coex_sta->wl_gl_busy) {
				table_case = 20;
				tdma_case = 17;
			} else {
				table_case = 10;
				tdma_case = 15;
			}
		} else if (bt_cpt_test) {
			table_case = 26;
			tdma_case = 26;
		} else {
			if (coex_sta->wl_gl_busy &&
			    coex_sta->wl_noisy_level == 0)
				table_case = 14;
			else
				table_case = 10;

			if (coex_sta->wl_gl_busy)
				tdma_case = 15;
			else
				tdma_case = 20;
		}
	} else { /* Non-Shared-Ant */
		table_case = 112;

		if (coex_sta->wl_gl_busy)
			tdma_case = 115;
		else
			tdma_case = 120;
	}

	if (coex_sta->bt_slave)
		rtw_btc_set_extend_btautoslot(btc, 0x3c);
	else
		rtw_btc_set_extend_btautoslot(btc, 0x32);

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_bt_pan_hid(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		table_case = 9;

		if (coex_sta->wl_gl_busy)
			tdma_case = 18;
		else
			tdma_case = 19;
	} else { /* Non-Shared-Ant */
		table_case = 113;

		if (coex_sta->wl_gl_busy)
			tdma_case = 117;
		else
			tdma_case = 119;
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_bt_a2dp_pan_hid(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		table_case = 10;

		if (coex_sta->wl_gl_busy)
			tdma_case = 15;
		else
			tdma_case = 20;
	} else { /* Non-Shared-Ant */
		table_case = 113;

		if (coex_sta->wl_gl_busy)
			tdma_case = 115;
		else
			tdma_case = 120;
	}

	if (coex_sta->bt_slave)
		rtw_btc_set_extend_btautoslot(btc, 0x3c);
	else
		rtw_btc_set_extend_btautoslot(btc, 0x32);

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_wl_off(struct btc_coexist *btc)
{
	rtw_btc_tdma(btc, FC_EXCU, 0);
	rtw_btc_ignore_wlan_act(btc, FC_EXCU, TRUE);
	rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_WOFF);

	btc->stop_coex_dm = TRUE;
	btc->wl_rf_state_off = TRUE;

	/* must place in the last step */
	rtw_btc_update_wl_ch_info(btc, BTC_MEDIA_DISCONNECT);
	btc->btc_write_scbd(btc, BTC_SCBD_ALL, FALSE);
}

static void rtw_btc_action_wl_under5g(struct btc_coexist *btc)
{
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_5G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	btc->btc_write_scbd(btc, BTC_SCBD_FIX2M, FALSE);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		table_case = 0;
		tdma_case = 0;
	} else { /* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_wl_only(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		table_case = 2;
		tdma_case = 0;
	} else { /* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
		if (coex_sta->wl_gl_busy &&
		    btc->board_info.ext_chip_id != BTC_EXT_CHIP_NONE)
			tdma_case = rtw_btc_ext_chip_new_tdma(btc, 3);
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_wl_native_lps(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	u8 table_case, tdma_case;

	if (link_info_ext->is_all_under_5g)
		return;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		table_case = 28; /*0x6c0 for A2DP, 0x6c4 for non-A2DP*/
		tdma_case = 0;
	} else { /* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_wl_linkscan(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;
	u32 slot_type = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (coex_sta->bt_a2dp_exist) {
			slot_type = TDMA_4SLOT;
			table_case = 9;
			tdma_case = 11;
		} else {
			table_case = 9;
			tdma_case = 7;
		}
	} else { /* Non-Shared-Ant */
		if (coex_sta->bt_a2dp_exist) {
			slot_type = TDMA_4SLOT;
			table_case = 112;
			tdma_case = 111;
		} else {
			table_case = 112;
			tdma_case = 107;
		}
		if (coex_sta->wl_gl_busy &&
	  	  btc->board_info.ext_chip_id != BTC_EXT_CHIP_NONE)
			tdma_case = rtw_btc_ext_chip_new_tdma(btc, 2);
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case | slot_type);
}

static void rtw_btc_action_wl_not_connected(struct btc_coexist *btc)
{
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	/* CCK Rx, Tx response, Tx beacon = low pri */
	if (link_info_ext->num_of_active_port == 0)
		rtw_btc_set_wl_pri_mask(btc, BTC_WLPRI_RX_CCK, 0);

	coex_sta->wl_cck_lock_ever = FALSE;
	coex_sta->wl_cck_lock = FALSE;
	coex_sta->cnt_wl[BTC_CNT_WL_2G_TDDTRY] = FALSE;
	coex_sta->cnt_wl[BTC_CNT_WL_2G_FDDSTAY] = FALSE;

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		table_case = 1;
		tdma_case = 0;
	} else { /* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_wl_connected(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 algorithm;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	/*Leap-AP protection will reopen when connecting AP*/
	rtw_btc_wl_leakap(btc, TRUE);

	if ((btc->board_info.btdm_ant_num == 2) &&
	    (btc->board_info.ext_chip_id != BTC_EXT_CHIP_NONE)) {
		rtw_btc_action_ext_chip(btc);
		return;
	}

	algorithm = rtw_btc_algorithm(btc);

	switch (algorithm) {
	case BTC_COEX_HFP:
		rtw_btc_action_bt_hfp(btc);
		break;
	case BTC_COEX_HID:
		if (rtw_btc_freerun_check(btc))
			rtw_btc_action_freerun(btc);
		else
			rtw_btc_action_bt_hid(btc);
		break;
	case BTC_COEX_A2DP:
		if (rtw_btc_freerun_check(btc))
			rtw_btc_action_freerun(btc);
		else if (coex_sta->bt_a2dp_sink)
			rtw_btc_action_bt_a2dpsink(btc);
		else
			rtw_btc_action_bt_a2dp(btc);
		break;
	case BTC_COEX_PAN:
		rtw_btc_action_bt_pan(btc);
		break;
	case BTC_COEX_A2DP_HID:
		if (rtw_btc_freerun_check(btc))
			rtw_btc_action_freerun(btc);
		else
			rtw_btc_action_bt_a2dp_hid(btc);
		break;
	case BTC_COEX_A2DP_PAN:
		rtw_btc_action_bt_a2dp_pan(btc);
		break;
	case BTC_COEX_PAN_HID:
		rtw_btc_action_bt_pan_hid(btc);
		break;
	case BTC_COEX_A2DP_PAN_HID:
		rtw_btc_action_bt_a2dp_pan_hid(btc);
		break;
	default:
	case BTC_COEX_NOPROFILE:
		rtw_btc_action_bt_idle(btc);
		break;
	}
}

static void rtw_btc_action_wl_mcc25g(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case, tdma_case;

	rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_MCC);
	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);
	btc->btc_write_scbd(btc, BTC_SCBD_FIX2M, FALSE);

	if (btc->board_info.btdm_ant_num == 1) { /* Shared-Ant */
		if (coex_sta->bt_setup_link) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], %s(): BT Relink\n", __func__);

			table_case = 24;
			tdma_case = 0;
		} else if (coex_sta->bt_inq_page) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], %s(): BT Inq-Pag\n", __func__);

			table_case = 23;
			tdma_case = 0;
		} else {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], %s(): BT on\n", __func__);

			if (coex_sta->wl_gl_busy) {
				if (coex_sta->wl_rx_rate <= 3 ||
				    coex_sta->wl_rts_rx_rate <= 3)
					table_case = 31;
				else if (coex_sta->bt_418_hid_exist ||
					 coex_sta->bt_ble_hid_exist)
					table_case = 25;
				else
					table_case = 23;
			} else {
				table_case = 23;
			}

			tdma_case = 0;
		}
	} else { /* Non-Shared-Ant */
		if (coex_sta->bt_setup_link) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], %s(): BT Relink\n", __func__);

			table_case = 100;
			tdma_case = 100;
		} else if (coex_sta->bt_inq_page) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], %s(): BT Inq-Pag\n", __func__);

			table_case = 118;
			tdma_case = 100;
		} else {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], %s(): BT on!!\n", __func__);

			table_case = 118;
			tdma_case = 100;
		}
	}

	BTC_TRACE(trace_buf);

	rtw_btc_table(btc, NM_EXCU, table_case);
	rtw_btc_tdma(btc, NM_EXCU, tdma_case);
}

static void rtw_btc_action_wl_scc2g(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case = 0xff, tdma_case = 0xff;
	boolean is_toggle_table = FALSE;
	u32 slot_type = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	if (coex_sta->bt_profile_num == 1) {
		if (coex_sta->bt_hid_exist || coex_sta->bt_hfp_exist) {
			if (coex_sta->bt_a2dp_active) {
				table_case = 9;
				tdma_case = 21;
			} else if (coex_sta->bt_418_hid_exist) {
				table_case = 10;
				tdma_case = 24;
				is_toggle_table = TRUE;
				slot_type = TDMA_4SLOT;
			} else {
				table_case = 2;
				tdma_case = 0;
			}
		} else if (coex_sta->bt_a2dp_exist) {
			table_case = 10;
			tdma_case = 22;
			slot_type = TDMA_4SLOT;
		} else { /* PAN or OPP */
			table_case = 10;
			tdma_case = 21;
		}
	} else {
		if ((coex_sta->bt_hid_exist || coex_sta->bt_hfp_exist) &&
		    coex_sta->bt_a2dp_exist) {
			table_case = 9;
			tdma_case = 22;

			slot_type = TDMA_4SLOT;
			if (coex_sta->bt_418_hid_exist)
				is_toggle_table = TRUE;
		} else if (coex_sta->bt_pan_exist && coex_sta->bt_a2dp_exist) {
			table_case = 10;
			tdma_case = 22;
			slot_type = TDMA_4SLOT;
		} else { /* hid + pan */
			table_case = 9;
			tdma_case = 21;
		}
	}

	rtw_btc_table(btc, NM_EXCU, table_case);
	if (is_toggle_table) {
		rtw_btc_wltoggle_tableA(btc, FC_EXCU, table_case);
		rtw_btc_wltoggle_tableB(btc, NM_EXCU, 1, 0x5a5a5aaa);
	}

	rtw_btc_tdma(btc, NM_EXCU, tdma_case | slot_type);
}

static void rtw_btc_action_wl_p2p2g(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_rfe_type *rfe_type = &btc->rfe_type;
	struct btc_wifi_link_info *link_info = &btc->wifi_link_info;
	u8 table_case = 0xff, tdma_case = 0xff, ant_phase;

	if (rfe_type->ant_switch_with_bt)
		ant_phase = BTC_ANT_2G_FREERUN;
	else
		ant_phase = BTC_ANT_2G;

	rtw_btc_set_rf_para(btc, NM_EXCU, btc->chip_para->wl_rf_para_rx[0]);
	btc->btc_write_scbd(btc, BTC_SCBD_FIX2M, FALSE);

	if (coex_sta->bt_disabled) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BT Disable!!\n", __func__);
		BTC_TRACE(trace_buf);
		rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);

		table_case = 0;
		tdma_case = 0;
	} else if (btc->board_info.btdm_ant_num == 2) { /* Non-Shared-Ant */
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): Non_Shared_Ant!!\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_action_freerun(btc);
		return;
	} else if (coex_sta->bt_setup_link) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BT Relink!!\n", __func__);

		rtw_btc_set_ant_path(btc, NM_EXCU, ant_phase);

		table_case = 1;
		tdma_case = 0;
	} else if (coex_sta->bt_inq_page) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BT Inq-Page!!\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_set_ant_path(btc, NM_EXCU, ant_phase);

		table_case = 15;
		tdma_case = 2;
	} else if (coex_sta->bt_profile_num == 0) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BT idle!!\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_set_ant_path(btc, NM_EXCU, ant_phase);

		if (btc->chip_interface == BTC_INTF_PCI &&
		    (link_info->link_mode == BTC_LINK_ONLY_GO ||
		    link_info->link_mode == BTC_LINK_ONLY_GC) &&
		    coex_sta->wl_gl_busy)
			table_case = 3;
		else
			table_case = 1;

		tdma_case = 0;
	} else if (coex_sta->wl_linkscan_proc) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL scan!!\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_action_wl_linkscan(btc);
	} else {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BT busy!!\n", __func__);
		BTC_TRACE(trace_buf);

		switch (link_info->link_mode) {
		case BTC_LINK_2G_SCC_GC_STA:
		case BTC_LINK_2G_SCC_GO_STA:
			rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
			rtw_btc_action_wl_scc2g(btc);
			break;
		case BTC_LINK_ONLY_GO:
		case BTC_LINK_ONLY_GC:
			rtw_btc_set_ant_path(btc, NM_EXCU, BTC_ANT_2G);
#ifdef PLATFORM_WINDOWS
			if (btc->chip_interface == BTC_INTF_PCI &&
			    coex_sta->bt_a2dp_exist && !coex_sta->bt_multi_link)
				table_case = 3;
			else
#endif
				table_case = 2;

			tdma_case = 0;
			break;
		default:
			rtw_btc_set_ant_path(btc, NM_EXCU, ant_phase);
			table_case = 2;
			tdma_case = 0;
			break;
		}
	}

	if (table_case != 0xff && tdma_case != 0xff) {
		rtw_btc_table(btc, NM_EXCU, table_case);
		rtw_btc_tdma(btc, NM_EXCU, tdma_case);
	}
}

static void rtw_btc_run_coex(struct btc_coexist *btc, u8 reason)
{
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	struct btc_wifi_link_info *link_info = &btc->wifi_link_info;
	const struct btc_chip_para *chip_para = btc->chip_para;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): reason = %d\n", __func__, reason);
	BTC_TRACE(trace_buf);

	coex_sta->coex_run_reason = reason;

	/* update wifi_link_info_ext variable */
	rtw_btc_update_wl_link_info(btc, reason);

	rtw_btc_monitor_bt_enable(btc);

	if (btc->manual_control) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], return for Manual CTRL!!\n");
		BTC_TRACE(trace_buf);
		return;
	}

	if (btc->stop_coex_dm) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], return for Stop Coex DM!!\n");
		BTC_TRACE(trace_buf);
		return;
	}

	if (coex_sta->wl_under_ips) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], return for wifi is under IPS!!\n");
		BTC_TRACE(trace_buf);
		return;
	}

	if (coex_sta->wl_under_lps && link_info_ext->is_32k) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], return for wifi is under LPS-32K!!\n");
		BTC_TRACE(trace_buf);
		return;
	}

	if (coex_sta->coex_freeze && reason == BTC_RSN_BTINFO &&
	    !coex_sta->bt_setup_link) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], return for coex_freeze!!\n");
		BTC_TRACE(trace_buf);
		return;
	}

	coex_sta->cnt_wl[BTC_CNT_WL_COEXRUN]++;
	coex_sta->coex_freerun = FALSE;

	/* Pure-5G Coex Process */
	if (link_info_ext->is_all_under_5g) {
		coex_sta->wl_coex_mode = BTC_WLINK_5G;
		rtw_btc_action_wl_under5g(btc);
		goto exit;
	}

	if (link_info_ext->is_mcc_25g) {
		coex_sta->wl_coex_mode = BTC_WLINK_25GMPORT;
		rtw_btc_action_wl_mcc25g(btc);
		goto exit;
	}

	/* if multi-port, P2P-GO, P2P-GC  */
	if (link_info_ext->num_of_active_port > 1 ||
	    (link_info->link_mode == BTC_LINK_ONLY_GO &&
	     !link_info_ext->is_ap_mode) ||
	     link_info->link_mode == BTC_LINK_ONLY_GC) {
		if (link_info->link_mode == BTC_LINK_ONLY_GO)
			coex_sta->wl_coex_mode = BTC_WLINK_2GGO;
		else if (link_info->link_mode == BTC_LINK_ONLY_GC)
			coex_sta->wl_coex_mode = BTC_WLINK_2GGC;
		else
			coex_sta->wl_coex_mode = BTC_WLINK_2GMPORT;
		rtw_btc_action_wl_p2p2g(btc);
		goto exit;
	}

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], WiFi is single-port 2G!!\n");
	BTC_TRACE(trace_buf);

	coex_sta->wl_coex_mode = BTC_WLINK_2G1PORT;

	/*For airpods 2 + HID glitch issue*/
	if (coex_sta->bt_a2dp_vendor_id == 0x4c && coex_sta->bt_multi_link)
		btc->btc_write_scbd(btc, BTC_SCBD_FIX2M, TRUE);
	else
		btc->btc_write_scbd(btc, BTC_SCBD_FIX2M, FALSE);

	if (coex_sta->bt_disabled) {
		if (!link_info_ext->is_connected)
			rtw_btc_action_wl_not_connected(btc);
		else
			rtw_btc_action_wl_only(btc);
		goto exit;
	}

	if (coex_sta->wl_under_lps && !coex_sta->wl_force_lps_ctrl) {
		rtw_btc_action_wl_native_lps(btc);
		goto exit;
	}

	if (coex_sta->bt_whck_test) {
		rtw_btc_action_bt_whql_test(btc);
		goto exit;
	}

	if (coex_sta->bt_setup_link) {
		rtw_btc_action_bt_relink(btc);
		goto exit;
	}

	if (coex_sta->bt_inq_page) {
		rtw_btc_action_bt_inquiry(btc);
		goto exit;
	}

	if ((coex_dm->bt_status == BTC_BTSTATUS_NCON_IDLE ||
	     coex_dm->bt_status == BTC_BTSTATUS_CON_IDLE) &&
	     link_info_ext->is_connected) {
		rtw_btc_action_bt_idle(btc);
		goto exit;
	}

	if (coex_sta->wl_linkscan_proc && !coex_sta->coex_freerun) {
		rtw_btc_action_wl_linkscan(btc);
		goto exit;
	}

	if (link_info_ext->is_connected) {
		rtw_btc_action_wl_connected(btc);
		goto exit;
	} else {
		rtw_btc_action_wl_not_connected(btc);
		goto exit;
	}

exit:
#ifdef PLATFORM_WINDOWS
	/* 0:original, 1:1R */
	if (coex_sta->wl_coex_mode == BTC_WLINK_2GFREE &&
	    chip_para->rx_path_num >= 2)
		rtw_btc_mimo_ps(btc, FC_EXCU, 1);
	else
		rtw_btc_mimo_ps(btc, FC_EXCU, 0);
#endif

	rtw_btc_gnt_workaround(btc, NM_EXCU, coex_sta->wl_coex_mode);
	rtw_btc_limited_wl(btc);
}

static void rtw_btc_init_coex_var(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *chip_para = btc->chip_para;
	u8 i;

	/* Reset Coex variable */
	btc->btc_set(btc, BTC_SET_RESET_COEX_VAR, NULL);

	/* Init Coex variables that are not zero */
	for (i = 0; i < ARRAY_SIZE(coex_dm->bt_rssi_state); i++)
		coex_dm->bt_rssi_state[i] = BTC_RSSI_STATE_LOW;

	for (i = 0; i < ARRAY_SIZE(coex_dm->wl_rssi_state); i++)
		coex_dm->wl_rssi_state[i] = BTC_RSSI_STATE_LOW;

	for (i = 0; i < ARRAY_SIZE(coex_sta->bt_sut_pwr_lvl); i++)
		coex_sta->bt_sut_pwr_lvl[i] = 0xff;

	coex_sta->bt_reg_vendor_ac = 0xffff;
	coex_sta->bt_reg_vendor_ae = 0xffff;

	coex_sta->gnt_workaround_state = BTC_WLINK_MAX;
	btc->bt_info.bt_get_fw_ver = 0;
}

static void
rtw_btc_init_hw_config(struct btc_coexist *btc, boolean wifi_only)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 table_case = 1, tdma_case = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	/* init coex_dm, coex_sta variable to sync with chip status */
	rtw_btc_init_coex_var(btc);

	/* 0xf0[15:12] --> chip kt info */
	coex_sta->kt_ver = (btc->btc_read_1byte(btc, 0xf1) & 0xf0) >> 4;

	rtw_btc_monitor_bt_enable(btc);

	/* TBTT enable */
	btc->btc_write_1byte_bitmask(btc, REG_BCN_CTRL, BIT_EN_BCN_FUNCTION,
				     0x1);

	/* Setup RF front end type */
	btc->chip_para->chip_setup(btc, BTC_CSETUP_RFE_TYPE);

	/* Init coex relared register  */
	btc->chip_para->chip_setup(btc, BTC_CSETUP_INIT_HW);

	/* set Tx response = Hi-Pri (ex: Transmitting ACK,BA,CTS) */
	rtw_btc_set_wl_pri_mask(btc, BTC_WLPRI_TX_RSP, 1);

	/* set Tx beacon = Hi-Pri  */
	rtw_btc_set_wl_pri_mask(btc, BTC_WLPRI_TX_BEACON, 1);

	/* set Tx beacon queue = Hi-Pri  */
	rtw_btc_set_wl_pri_mask(btc, BTC_WLPRI_TX_BEACONQ, 1);

	/* Antenna config */
	if (btc->wl_rf_state_off) {
		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_WOFF);
		btc->btc_write_scbd(btc, BTC_SCBD_ALL, FALSE);
		btc->stop_coex_dm = TRUE;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): RF Off\n", __func__);
		BTC_TRACE(trace_buf);
	} else if (wifi_only) {
		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_WONLY);
		btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE | BTC_SCBD_ON, TRUE);
		btc->stop_coex_dm = TRUE;
	} else {
		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_INIT);
		btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE | BTC_SCBD_ON, TRUE);
		btc->stop_coex_dm = FALSE;
		coex_sta->coex_freeze = TRUE;
	}

	/* PTA parameter */
	rtw_btc_table(btc, FC_EXCU, table_case);
	rtw_btc_tdma(btc, FC_EXCU, tdma_case);

	rtw_btc_query_bt_info(btc);
}

void rtw_btc_ex_power_on_setting(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_board_info *board_info = &btc->board_info;
	u8 table_case = 1;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	btc->stop_coex_dm = TRUE;
	btc->wl_rf_state_off = FALSE;

	/* enable BB, REG_SYS_FUNC_EN to write reg correctly. */
	btc->btc_write_1byte_bitmask(btc, REG_SYS_FUNC_EN,
				     BIT_FEN_BB_GLB_RST | BIT_FEN_BB_RSTB, 0x3);

	rtw_btc_monitor_bt_enable(btc);

	/* Setup RF front end type */
	btc->chip_para->chip_setup(btc, BTC_CSETUP_RFE_TYPE);

	/* Set Antenna Path to BT side */
	rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_POWERON);

	rtw_btc_table(btc, FC_EXCU, table_case);

	/* SD1 Chunchu red x issue */
	btc->btc_write_1byte(btc, 0xff1a, 0x0);

	rtw_btc_gnt_debug(btc, TRUE);

	board_info->btdm_ant_pos = BTC_ANTENNA_AT_MAIN_PORT;
}

void rtw_btc_ex_pre_load_firmware(struct btc_coexist *btc) {}

void rtw_btc_ex_init_hw_config(struct btc_coexist *btc, boolean wifi_only)
{
	rtw_btc_init_hw_config(btc, wifi_only);
}

void rtw_btc_ex_init_coex_dm(struct btc_coexist *btc)
{
}

void rtw_btc_ex_display_simple_coex_info(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *chip_para = btc->chip_para;
	struct btc_rfe_type *rfe_type = &btc->rfe_type;
	struct btc_board_info *board_info = &btc->board_info;

	u8 *cli_buf = btc->cli_buf;
	u32 bt_patch_ver = 0, bt_coex_ver = 0, val = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n _____[BT Coexist info]____");
	CL_PRINTF(cli_buf);

	if (btc->manual_control) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n __[Under Manual Control]_");
		CL_PRINTF(cli_buf);
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n _________________________");
		CL_PRINTF(cli_buf);
	}

	if (btc->stop_coex_dm) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ____[Coex is STOPPED]____");
		CL_PRINTF(cli_buf);
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n _________________________");
		CL_PRINTF(cli_buf);
	}

	if (!coex_sta->bt_disabled &&
	    (coex_sta->bt_supported_version == 0 ||
	     coex_sta->bt_supported_version == 0xffff) &&
	     coex_sta->cnt_wl[BTC_CNT_WL_COEXINFO2] % 3 == 0) {
		btc->btc_get(btc, BTC_GET_U4_SUPPORTED_FEATURE,
			     &coex_sta->bt_supported_feature);

		btc->btc_get(btc, BTC_GET_U4_SUPPORTED_VERSION,
			     &coex_sta->bt_supported_version);

		val = btc->btc_get_bt_reg(btc, 3, 0xac);
		coex_sta->bt_reg_vendor_ac = (u16)(val & 0xffff);

		val = btc->btc_get_bt_reg(btc, 3, 0xae);
		coex_sta->bt_reg_vendor_ae = (u16)(val & 0xffff);

		btc->btc_get(btc, BTC_GET_U4_BT_PATCH_VER, &bt_patch_ver);
		btc->bt_info.bt_get_fw_ver = bt_patch_ver;
	}

	/* BT coex. info. */
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d/ %d/ %s / %d",
		   "Ant PG Num/ Mech/ Pos/ RFE", board_info->pg_ant_num,
		   board_info->btdm_ant_num,
		   (board_info->btdm_ant_pos ==
		    BTC_ANTENNA_AT_MAIN_PORT ? "Main" : "Aux"),
		   rfe_type->rfe_module_type);
	CL_PRINTF(cli_buf);

	bt_coex_ver = ((coex_sta->bt_supported_version & 0xff00) >> 8);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d_%02x/ %d_%02x/ 0x%02x/ 0x%02x (%s)",
		   "Ver Coex/ Para/ BT_Dez/ BT_Rpt",
		   coex_ver_date, coex_ver, chip_para->para_ver_date,
		   chip_para->para_ver, chip_para->bt_desired_ver, bt_coex_ver,
		   (bt_coex_ver == 0xff ? "Unknown" :
		   (coex_sta->bt_disabled ? "BT-disable" :
		   (bt_coex_ver >= chip_para->bt_desired_ver ?
		    "Match" : "Mis-Match"))));
	CL_PRINTF(cli_buf);

	/* BT Status */
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s", "BT status",
		   ((coex_sta->bt_disabled) ? ("disabled") :
		   ((coex_sta->bt_inq_page) ? ("inquiry/page") :
		   ((coex_dm->bt_status == BTC_BTSTATUS_NCON_IDLE) ?
		    "non-connected idle" :
		    ((coex_dm->bt_status == BTC_BTSTATUS_CON_IDLE) ?
		    "connected-idle" : "busy")))));
	CL_PRINTF(cli_buf);

	/* HW Settings */
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d",
		   "0x770(Hi-pri rx/tx)", coex_sta->hi_pri_rx,
		   coex_sta->hi_pri_tx);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d %s",
		   "0x774(Lo-pri rx/tx)", coex_sta->lo_pri_rx,
		   coex_sta->lo_pri_tx, (coex_sta->bt_slave ?
		   "(Slave!!)" : ""));
	CL_PRINTF(cli_buf);

	coex_sta->cnt_wl[BTC_CNT_WL_COEXINFO2]++;
}

void rtw_btc_ex_display_coex_info(struct btc_coexist *btc)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	const struct btc_chip_para *chip_para = btc->chip_para;
	struct btc_rfe_type *rfe_type = &btc->rfe_type;
	struct btc_board_info *board_info = &btc->board_info;

	u8 *cli_buf = btc->cli_buf, i, ps_tdma_case = 0;
	u16 scbd;
	u32 phy_ver = 0, fw_ver = 0,
	    bt_coex_ver = 0, val = 0,
	    fa_ofdm, fa_cck, cca_ofdm, cca_cck,
	    ok_11b, ok_11g, ok_11n, ok_11vht,
	    err_11b, err_11g, err_11n, err_11vht;
	boolean is_bt_reply = FALSE;
	u8 * const p = &coex_sta->bt_afh_map[0];

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n ============[BT Coexist info %s]============",
		   chip_para->chip_name);
	CL_PRINTF(cli_buf);

	if (btc->manual_control) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ============[Under Manual Control]============");
		CL_PRINTF(cli_buf);
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ==========================================");
		CL_PRINTF(cli_buf);
	} else if (btc->stop_coex_dm) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ============[Coex is STOPPED]============");
		CL_PRINTF(cli_buf);
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ==========================================");
		CL_PRINTF(cli_buf);
	} else if (coex_sta->coex_freeze) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ============[coex_freeze]============");
		CL_PRINTF(cli_buf);
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ==========================================");
		CL_PRINTF(cli_buf);
	}

	if (!coex_sta->bt_disabled &&
	     coex_sta->cnt_wl[BTC_CNT_WL_COEXINFO1] % 3 == 0) {
		if (coex_sta->bt_supported_version == 0 ||
		    coex_sta->bt_supported_version == 0xffff) {
			btc->btc_get(btc, BTC_GET_U4_SUPPORTED_VERSION,
				     &coex_sta->bt_supported_version);

			if (coex_sta->bt_supported_version > 0 &&
			    coex_sta->bt_supported_version < 0xffff)
				is_bt_reply = TRUE;
		} else {
			is_bt_reply = TRUE;
		}

		if (coex_dm->bt_status != BTC_BTSTATUS_NCON_IDLE) {
			btc->btc_get_bt_afh_map_from_bt(btc, 0, p);
			val = btc->btc_get_bt_reg(btc, 1, 0xa);
			coex_sta->bt_reg_modem_a = (u16)((val & 0x1c0) >> 6);
			val = btc->btc_get_bt_reg(btc, 0, 0x2);
			coex_sta->bt_reg_rf_2 = (u16)val;
		}
	}

	if (is_bt_reply) {
		if (coex_sta->bt_supported_feature == 0) {
			btc->btc_get(btc, BTC_GET_U4_SUPPORTED_FEATURE,
				     &coex_sta->bt_supported_feature);

			if (coex_sta->bt_supported_feature & BIT(11))
				coex_sta->bt_slave_latency = TRUE;
			else
				coex_sta->bt_slave_latency = FALSE;
		}

		if (coex_sta->bt_reg_vendor_ac == 0xffff) {
			val = btc->btc_get_bt_reg(btc, 3, 0xac);
			coex_sta->bt_reg_vendor_ac = (u16)(val & 0xffff);
		}

		if (coex_sta->bt_reg_vendor_ae == 0xffff) {
			val = btc->btc_get_bt_reg(btc, 3, 0xae);
			coex_sta->bt_reg_vendor_ae = (u16)(val & 0xffff);
		}

		if (btc->bt_info.bt_get_fw_ver == 0)
			btc->btc_get(btc, BTC_GET_U4_BT_PATCH_VER,
				     &btc->bt_info.bt_get_fw_ver);

		if (coex_sta->bt_a2dp_exist &&
		    coex_sta->bt_a2dp_vendor_id == 0 &&
		    coex_sta->bt_a2dp_device_name == 0) {
			btc->btc_get(btc, BTC_GET_U4_BT_DEVICE_INFO, &val);
			coex_sta->bt_a2dp_vendor_id = (u8)(val & 0xff);
			coex_sta->bt_a2dp_device_name = (val & 0xffffff00) >> 8;
		}

		if (coex_sta->bt_a2dp_exist &&
		    coex_sta->bt_a2dp_flush_time == 0) {
			btc->btc_get(btc, BTC_GET_U4_BT_A2DP_FLUSH_VAL, &val);
			coex_sta->bt_a2dp_flush_time = val;
		}
	}

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %s/ %s / %d/ %d",
		   "Ant PG Num/ Mech/ Pos/ RFE/ Dist", board_info->pg_ant_num,
		   (board_info->btdm_ant_num == 1 ? "Shared" : "Non-Shared"),
		   (board_info->btdm_ant_pos == BTC_ANTENNA_AT_MAIN_PORT ?
		   "Main" : "Aux"), rfe_type->rfe_module_type,
		   board_info->ant_distance);
	CL_PRINTF(cli_buf);

	btc->btc_get(btc, BTC_GET_U4_WIFI_FW_VER, &fw_ver);
	btc->btc_get(btc, BTC_GET_U4_WIFI_PHY_VER, &phy_ver);
	bt_coex_ver = ((coex_sta->bt_supported_version & 0xff00) >> 8);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d_%02x/ %d_%02x/ 0x%02x/ 0x%02x (%s)",
		   "Ver Coex/ Para/ BT_Dez/ BT_Rpt",
		   coex_ver_date, coex_ver, chip_para->para_ver_date,
		   chip_para->para_ver, chip_para->bt_desired_ver, bt_coex_ver,
		   (bt_coex_ver == 0xff ? "Unknown" :
		   (coex_sta->bt_disabled ? "BT-disable" :
		   (bt_coex_ver >= chip_para->bt_desired_ver ?
		    "Match" : "Mis-Match"))));
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x(%s)/ 0x%08x/ v%d/ %c",
		   "W_FW/ B_FW/ Phy/ Kt", fw_ver,
		   (fw_ver >= wl_fw_desired_ver ? "Match" : "Mis-Match"),
		   btc->bt_info.bt_get_fw_ver, phy_ver, coex_sta->kt_ver + 65);
	CL_PRINTF(cli_buf);

	/* wifi status */
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
		   "============[Wifi Status]============");
	CL_PRINTF(cli_buf);
	btc->btc_disp_dbg_msg(btc, BTC_DBG_DISP_WIFI_STATUS);

	/*EXT CHIP status*/
	if (btc->board_info.ext_chip_id != BTC_EXT_CHIP_NONE) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
		   "============[EXT CHIP Status]============");
		CL_PRINTF(cli_buf);
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s/ %s",
		   "EXT CHIP ID/EXT CHIP mode",
		   ((btc->board_info.ext_chip_id ==
		   BTC_EXT_CHIP_RF4CE) ? "RF4CE" : "unknown"),
		   ((coex_sta->ext_chip_mode ==
		   BTC_EXTMODE_VOICE) ? "VOICE" : "NORMAL"));
		CL_PRINTF(cli_buf);
	}

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
		   "============[BT Status]============");
	CL_PRINTF(cli_buf);
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s/ %ddBm/ %d/ %d",
		   "BT status/ rssi/ retryCnt/ popCnt",
		   ((coex_sta->bt_disabled) ? ("disabled") :
		    ((coex_sta->bt_inq_page) ? ("inquiry-page") :
		    ((coex_dm->bt_status == BTC_BTSTATUS_NCON_IDLE) ?
		    "non-connecte-idle" : ((coex_dm->bt_status ==
		    BTC_BTSTATUS_CON_IDLE) ? "connected-idle" : "busy")))),
		    coex_sta->bt_rssi - 100, coex_sta->cnt_bt[BTC_CNT_BT_RETRY],
		    coex_sta->cnt_bt[BTC_CNT_BT_POPEVENT]);
	CL_PRINTF(cli_buf);

	if (coex_sta->bt_profile_num != 0) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = %s%s%s%s%s%s (multilink = %d)",
			   "Profiles", ((coex_sta->bt_a2dp_exist) ?
			   ((coex_sta->bt_a2dp_sink) ? "A2DP sink," :
			    "A2DP,") : ""),
			   ((coex_sta->bt_hfp_exist) ? "HFP," : ""),
			   ((coex_sta->bt_hid_exist) ?
			   ((coex_sta->bt_ble_exist) ? "HID(RCU)" :
			   ((coex_sta->bt_hid_slot >= 2) ? "HID(4/18)," :
			   (coex_sta->bt_ble_hid_exist ? "HID(BLE)" :
			   "HID(2/18),"))) : ""), ((coex_sta->bt_pan_exist) ?
			   ((coex_sta->bt_opp_exist) ? "OPP," : "PAN,") :
			   ""), ((coex_sta->bt_ble_voice) ? "Voice," : ""),
			   ((coex_sta->bt_msft_mr_exist) ? "MR" : ""),
			   coex_sta->bt_multi_link);
		CL_PRINTF(cli_buf);

		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = %d/ %d/ %d/ %d",
			   "SUT Power[3:0]",
			   coex_sta->bt_sut_pwr_lvl[3],
			   coex_sta->bt_sut_pwr_lvl[2],
			   coex_sta->bt_sut_pwr_lvl[1],
			   coex_sta->bt_sut_pwr_lvl[0]);

		CL_PRINTF(cli_buf);
	} else {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s",
			   "Profiles",
			   (coex_sta->bt_msft_mr_exist) ? "MR" : "None");

	CL_PRINTF(cli_buf);
	}

	/* for 8822b, Scoreboard[10]: 0: CQDDR off, 1: CQDDR on
	 * for 8822c, Scoreboard[10]: 0: CQDDR on, 1:CQDDR fix 2M
	 */

	if (coex_sta->bt_a2dp_exist) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = %s/ %d/ 0x%x/ 0x%x/ %d",
			   "CQDDR/Bitpool/V_ID/D_name/Flush",
			   (chip_para->new_scbd10_def ?
			   ((coex_sta->bt_fix_2M) ? "fix_2M" : "CQDDR_On") :
			   ((coex_sta->bt_fix_2M) ? "CQDDR_On" : "CQDDR_Off")),
			   coex_sta->bt_a2dp_bitpool,
			   coex_sta->bt_a2dp_vendor_id,
			   coex_sta->bt_a2dp_device_name,
			   coex_sta->bt_a2dp_flush_time);

		CL_PRINTF(cli_buf);
	}

	if (coex_sta->bt_hid_exist) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d",
			   "HID PairNum", coex_sta->bt_hid_pair_num);
		CL_PRINTF(cli_buf);
	}

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s/ %d/ %s/ 0x%x",
		   "Role/RoleSwCnt/IgnWla/Feature",
		   ((coex_sta->bt_slave) ? "Slave" : "Master"),
		   coex_sta->cnt_bt[BTC_CNT_BT_ROLESWITCH],
		   ((coex_dm->cur_ignore_wlan_act) ? "Yes" : "No"),
		   coex_sta->bt_supported_feature);
	CL_PRINTF(cli_buf);

	if (coex_sta->bt_ble_scan_en) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = 0x%x/ 0x%x/ 0x%x/ 0x%x",
			   "BLEScan Type/TV/Init/Ble",
			   coex_sta->bt_ble_scan_type,
			   (coex_sta->bt_ble_scan_type & 0x1 ?
				    coex_sta->bt_ble_scan_para[0] : 0x0),
			   (coex_sta->bt_ble_scan_type & 0x2 ?
				    coex_sta->bt_ble_scan_para[1] : 0x0),
			   (coex_sta->bt_ble_scan_type & 0x4 ?
				    coex_sta->bt_ble_scan_para[2] : 0x0));
		CL_PRINTF(cli_buf);
	}

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d/ %d/ %d/ %d/ %d/ %d/ %d %s",
		   "Init/ReLink/IgnWl/Pag/Inq/iqkO/iqkX",
		   coex_sta->cnt_bt[BTC_CNT_BT_REINIT],
		   coex_sta->cnt_bt[BTC_CNT_BT_SETUPLINK],
		   coex_sta->cnt_bt[BTC_CNT_BT_IGNWLANACT],
		   coex_sta->cnt_bt[BTC_CNT_BT_PAGE],
		   coex_sta->cnt_bt[BTC_CNT_BT_INQ],
		   coex_sta->cnt_bt[BTC_CNT_BT_IQK],
		   coex_sta->cnt_bt[BTC_CNT_BT_IQKFAIL],
		   (coex_sta->bt_setup_link ? "(Relink!!)" : ""));
	CL_PRINTF(cli_buf);

	if (coex_sta->bt_reg_vendor_ae == 0xffff ||
	    coex_sta->bt_reg_vendor_ac == 0xffff)
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = x/ x/ 0x%04x",
			   "0xae[4]/0xac[1:0]/ScBd(B->W)",
			   btc->btc_read_scbd(btc, &scbd));
	else
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = 0x%x/ 0x%x/ 0x%x/ 0x%x/ 0x%04x/ %s",
			   "ae/ac/m_a[8:6]/rf_2/ScBd(B->W)/path",
			   coex_sta->bt_reg_vendor_ae,
			   coex_sta->bt_reg_vendor_ac,
			   coex_sta->bt_reg_modem_a,
			   coex_sta->bt_reg_rf_2,
			   btc->btc_read_scbd(btc, &scbd),
			   ((coex_sta->bt_reg_vendor_ae & BIT(4)) ? "S1" : "S0"
			   ));
	CL_PRINTF(cli_buf);

	if (coex_dm->bt_status != BTC_BTSTATUS_NCON_IDLE) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x",
			   "AFH MAP", coex_sta->bt_afh_map[0],
			   coex_sta->bt_afh_map[1], coex_sta->bt_afh_map[2],
			   coex_sta->bt_afh_map[3], coex_sta->bt_afh_map[4],
			   coex_sta->bt_afh_map[5], coex_sta->bt_afh_map[6],
			   coex_sta->bt_afh_map[7], coex_sta->bt_afh_map[8],
			   coex_sta->bt_afh_map[9]);
		CL_PRINTF(cli_buf);
	}

	for (i = 0; i < BTC_BTINFO_SRC_BT_IQK; i++) {
		if (coex_sta->cnt_bt_info_c2h[i]) {
			CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
				   "\r\n %-35s = %02x %02x %02x %02x %02x %02x %02x (%d)",
				   glbt_info_src[i],
				   coex_sta->bt_info_c2h[i][0],
				   coex_sta->bt_info_c2h[i][1],
				   coex_sta->bt_info_c2h[i][2],
				   coex_sta->bt_info_c2h[i][3],
				   coex_sta->bt_info_c2h[i][4],
				   coex_sta->bt_info_c2h[i][5],
				   coex_sta->bt_info_c2h[i][6],
				   coex_sta->cnt_bt_info_c2h[i]);
			CL_PRINTF(cli_buf);
		}
	}

	if (btc->manual_control) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
			   "============[mechanisms] (under Manual)============");
		CL_PRINTF(cli_buf);

		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = %02x %02x %02x %02x %02x",
			   "TDMA_Now",
			   coex_dm->fw_tdma_para[0], coex_dm->fw_tdma_para[1],
			   coex_dm->fw_tdma_para[2], coex_dm->fw_tdma_para[3],
			   coex_dm->fw_tdma_para[4]);
		CL_PRINTF(cli_buf);
	} else {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
			   "============[Mechanisms]============");
		CL_PRINTF(cli_buf);

		ps_tdma_case = coex_dm->cur_ps_tdma;
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = %02x %02x %02x %02x %02x (case-%d, TDMA-%s, Ext-%d, Tog-%d)",
			   "TDMA",
			   coex_dm->ps_tdma_para[0], coex_dm->ps_tdma_para[1],
			   coex_dm->ps_tdma_para[2], coex_dm->ps_tdma_para[3],
			   coex_dm->ps_tdma_para[4], ps_tdma_case,
			   (coex_dm->cur_ps_tdma_on ? "On" : "Off"),
			   coex_sta->bt_ext_autoslot_thres,
			   coex_sta->wl_toggle_interval);
		CL_PRINTF(cli_buf);
	}

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s/ %s/ %d",
		   "Coex_Mode/Free_Run/Timer_base",
		   coex_mode_string[coex_sta->wl_coex_mode],
		   ((coex_sta->coex_freerun) ? "Yes" : "No"),
		   coex_sta->tdma_timer_base);

	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d/ 0x%x/ 0x%x/ 0x%x",
		   "Table/0x6c0/0x6c4/0x6c8", coex_sta->coex_table_type,
		   btc->btc_read_4byte(btc, REG_BT_COEX_TABLE0),
		   btc->btc_read_4byte(btc, REG_BT_COEX_TABLE1),
		   btc->btc_read_4byte(btc, REG_BT_COEX_BRK_TABLE));
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x/ 0x%x/ 0x%04x/ %d/ %s",
		   "0x778/0x6cc/ScBd(W->B)/RunCnt/Rsn",
		   btc->btc_read_1byte(btc, REG_BT_STAT_CTRL),
		   btc->btc_read_4byte(btc, REG_BT_COEX_TABLE_H),
		   coex_sta->score_board_WB,
		   coex_sta->cnt_wl[BTC_CNT_WL_COEXRUN],
		   run_reason_string[coex_sta->coex_run_reason]);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %02x %02x %02x (RF-Ch = %d)", "AFH Map to BT",
		   coex_dm->wl_chnl_info[0], coex_dm->wl_chnl_info[1],
		   coex_dm->wl_chnl_info[2], coex_sta->wl_center_ch);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s/ %s/ %s/ %d",
		   "AntDiv/BtCtrlLPS/LPRA/g_busy",
		   ((board_info->ant_div_cfg) ? "On" : "Off"),
		   ((coex_sta->wl_force_lps_ctrl) ? "On" : "Off"),
		   ((coex_dm->cur_low_penalty_ra) ? "On" : "Off"),
		   coex_sta->wl_gl_busy);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %d/ %d/ %d",
		   "Null All/Retry/Ack/BT_Empty/BT_Late",
		   coex_sta->wl_fw_dbg_info[1], coex_sta->wl_fw_dbg_info[2],
		   coex_sta->wl_fw_dbg_info[3], coex_sta->wl_fw_dbg_info[4],
		   coex_sta->wl_fw_dbg_info[5]);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %s/ %d",
		   "Cnt TDMA_Togg/LkRx/LKAP_On/fw",
		   coex_sta->wl_fw_dbg_info[6],
		   coex_sta->wl_fw_dbg_info[7],
		   ((coex_sta->wl_leak_ap) ? "Yes" : "No"),
		   coex_sta->cnt_wl[BTC_CNT_WL_FW_NOTIFY]);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %s/ %d",
		   "WL_TxPw/BT_TxPw/WL_Rx/BT_LNA_Lvl",
		   coex_dm->cur_wl_pwr_lvl, coex_dm->cur_bt_pwr_lvl,
		   ((coex_dm->cur_wl_rx_low_gain_en) ? "On" : "Off"),
		   coex_dm->cur_bt_lna_lvl);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %s",
		   "MIMO_PS On/Recover/BlackAP",
		   coex_sta->cnt_wl[BTC_CNT_WL_2G_FDDSTAY],
		   coex_sta->cnt_wl[BTC_CNT_WL_2G_TDDTRY],
		   ((coex_sta->wl_blacklist_ap) ? "Yes": "No"));
	CL_PRINTF(cli_buf);

	/* Hw setting		 */
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
		   "============[Hw setting]============");
	CL_PRINTF(cli_buf);

	btc->chip_para->chip_setup(btc, BTC_CSETUP_COEXINFO_HW);

	fa_ofdm = btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_FA_OFDM);
	fa_cck = btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_FA_CCK);
	cca_ofdm = btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CCA_OFDM);
	cca_cck = btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CCA_CCK);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d/ %d/ %d/ %d",
		   "CCK-CCA/CCK-FA/OFDM-CCA/OFDM-FA", cca_cck, fa_cck, cca_ofdm,
		   fa_ofdm);
	CL_PRINTF(cli_buf);

	ok_11b =
	   btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CRC32_OK_CCK);
	ok_11g =
	   btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CRC32_OK_LEGACY);
	ok_11n =
	   btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CRC32_OK_HT);
	ok_11vht =
	   btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CRC32_OK_VHT);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %d/ %d",
		   "CRC_OK CCK/11g/11n/11ac", ok_11b, ok_11g, ok_11n, ok_11vht);
	CL_PRINTF(cli_buf);

	err_11b =
	   btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CRC32_ERROR_CCK);
	err_11g =
	   btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CRC32_ERROR_LEGACY);
	err_11n =
	    btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CRC32_ERROR_HT);
	err_11vht =
	    btc->btc_phydm_query_PHY_counter(btc, PHYDM_INFO_CRC32_ERROR_VHT);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %d/ %d",
		   "CRC_Err CCK/11g/11n/11ac",
		   err_11b, err_11g, err_11n, err_11vht);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d/ %d/ %s-%d/ %d (Tx macid: %d)",
		   "Rate RxD/RxRTS/TxD/TxRetry_ratio",
		   coex_sta->wl_rx_rate, coex_sta->wl_rts_rx_rate,
		   (coex_sta->wl_tx_rate & 0x80 ? "SGI" : "LGI"),
		   coex_sta->wl_tx_rate & 0x7f,
		   coex_sta->wl_tx_retry_ratio,
		   coex_sta->wl_tx_macid);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s/ %s/ %s/ %d",
		   "HiPr/ Locking/ Locked/ Noisy",
		   (coex_sta->wl_hi_pri_task1 ? "Yes" : "No"),
		   (coex_sta->wl_cck_lock ? "Yes" : "No"),
		   (coex_sta->wl_cck_lock_ever ? "Yes" : "No"),
		   coex_sta->wl_noisy_level);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d",
		   "0x770(Hi-pri rx/tx)", coex_sta->hi_pri_rx,
		   coex_sta->hi_pri_tx);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d %s",
		   "0x774(Lo-pri rx/tx)", coex_sta->lo_pri_rx,
		   coex_sta->lo_pri_tx, (coex_sta->bt_slave ?
		   "(Slave!!)" : ""));
	CL_PRINTF(cli_buf);

	btc->btc_disp_dbg_msg(btc, BTC_DBG_DISP_COEX_STATISTICS);

	coex_sta->cnt_wl[BTC_CNT_WL_COEXINFO1]++;

	if (coex_sta->cnt_wl[BTC_CNT_WL_COEXINFO1] % 5 == 0)
		coex_sta->cnt_bt[BTC_CNT_BT_POPEVENT] = 0;
}

void rtw_btc_ex_ips_notify(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	if (type == BTC_IPS_ENTER) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], IPS ENTER notify\n");
		BTC_TRACE(trace_buf);
		coex_sta->wl_under_ips = TRUE;

		/* Write WL "Active" in Score-board for LPS off */
		btc->btc_write_scbd(btc, BTC_SCBD_ALL, FALSE);

		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_WOFF);
		rtw_btc_action_coex_all_off(btc);
	} else if (type == BTC_IPS_LEAVE) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], IPS LEAVE notify\n");
		BTC_TRACE(trace_buf);
		btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE | BTC_SCBD_ON, TRUE);

		/*leave IPS : run ini hw config (exclude wifi only)*/
		rtw_btc_init_hw_config(btc, FALSE);

		coex_sta->wl_under_ips = FALSE;
	}
}

void rtw_btc_ex_lps_notify(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	if (type == BTC_LPS_ENABLE) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], LPS ENABLE notify\n");
		BTC_TRACE(trace_buf);
		coex_sta->wl_under_lps = TRUE;

		if (coex_sta->wl_force_lps_ctrl) { /* LPS No-32K */
			/* Write WL "Active" in Score-board for PS-TDMA */
			btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE, TRUE);
		} else {
			/* Write WL "Non-Active" in Score-board for Native-PS */
			btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE, FALSE);
			btc->btc_write_scbd(btc, BTC_SCBD_WLBUSY, FALSE);

			rtw_btc_run_coex(btc, BTC_RSN_LPS);
		}
	} else if (type == BTC_LPS_DISABLE) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], LPS DISABLE notify\n");
		BTC_TRACE(trace_buf);
		coex_sta->wl_under_lps = FALSE;

		/* Write WL "Active" in Score-board for LPS off */
		btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE, TRUE);

		if (!coex_sta->wl_force_lps_ctrl)
			rtw_btc_query_bt_info(btc);

		rtw_btc_run_coex(btc, BTC_RSN_LPS);
	}
}

void rtw_btc_ex_scan_notify(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	coex_sta->coex_freeze = FALSE;

	btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE | BTC_SCBD_ON, TRUE);

	if (type == BTC_SCAN_START_5G) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], SCAN START notify (5G)\n");
		BTC_TRACE(trace_buf);

		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_5G);
		rtw_btc_run_coex(btc, BTC_RSN_5GSCANSTART);
	} else if (type == BTC_SCAN_START_2G || type == BTC_SCAN_START) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], SCAN START notify (2G)\n");
		BTC_TRACE(trace_buf);

		coex_sta->wl_hi_pri_task2 = TRUE;

		/* Force antenna setup for no scan result issue */
		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_2G);
		rtw_btc_run_coex(btc, BTC_RSN_2GSCANSTART);
	} else {
		btc->btc_get(btc, BTC_GET_U1_AP_NUM,
			     &coex_sta->cnt_wl[BTC_CNT_WL_SCANAP]);

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], SCAN FINISH notify (Scan-AP = %d)\n",
			    coex_sta->cnt_wl[BTC_CNT_WL_SCANAP]);
		BTC_TRACE(trace_buf);

		coex_sta->wl_hi_pri_task2 = FALSE;

		rtw_btc_run_coex(btc, BTC_RSN_SCANFINISH);
	}
}

void rtw_btc_ex_scan_notify_without_bt(struct btc_coexist *btc, u8 type)
{
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	struct btc_rfe_type *rfe_type = &btc->rfe_type;
	u8 ctrl_type = BTC_SWITCH_CTRL_BY_BBSW, pos_type = BTC_SWITCH_TO_WLG;

	if (!rfe_type->ant_switch_exist)
		return;

	if (type == BTC_SCAN_START && link_info_ext->is_all_under_5g)
		pos_type = BTC_SWITCH_TO_WLA;

	rtw_btc_set_ant_switch(btc, FC_EXCU, ctrl_type, pos_type);
}

void rtw_btc_ex_switchband_notify(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	if (type == BTC_SWITCH_TO_5G) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): TO_5G\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_run_coex(btc, BTC_RSN_5GSWITCHBAND);
	} else if (type == BTC_SWITCH_TO_24G_NOFORSCAN) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): TO_24G_NOFORSCAN\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_run_coex(btc, BTC_RSN_2GSWITCHBAND);
	} else {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): TO_2G\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_ex_scan_notify(btc, BTC_SCAN_START_2G);
	}
}

void rtw_btc_ex_switchband_notify_without_bt(struct btc_coexist *btc, u8 type)
{
	struct btc_rfe_type *rfe_type = &btc->rfe_type;
	u8 ctrl_type = BTC_SWITCH_CTRL_BY_BBSW, pos_type = BTC_SWITCH_TO_WLG;

	if (!rfe_type->ant_switch_exist)
		return;

	if (type == BTC_SWITCH_TO_5G) {
		pos_type = BTC_SWITCH_TO_WLA;
	} else if (type == BTC_SWITCH_TO_24G_NOFORSCAN) {
		pos_type = BTC_SWITCH_TO_WLG;
	} else {
		rtw_btc_ex_scan_notify_without_bt(btc, BTC_SCAN_START_2G);
		return;
	}

	rtw_btc_set_ant_switch(btc, FC_EXCU, ctrl_type, pos_type);
}

void rtw_btc_ex_connect_notify(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE | BTC_SCBD_ON, TRUE);

	if (type == BTC_ASSOCIATE_5G_START) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): 5G start\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_5G);

		rtw_btc_run_coex(btc, BTC_RSN_5GCONSTART);
	} else if (type == BTC_ASSOCIATE_5G_FINISH) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): 5G finish\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_5G);

		rtw_btc_run_coex(btc, BTC_RSN_5GCONFINISH);
	} else if (type == BTC_ASSOCIATE_START) {
		coex_sta->wl_hi_pri_task1 = TRUE;
		coex_sta->cnt_wl[BTC_CNT_WL_ARP] = 0;
		coex_sta->wl_connecting = TRUE;
		btc->btc_set_timer(btc, BTC_TIMER_WL_CONNPKT, 2);

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): 2G start\n", __func__);
		BTC_TRACE(trace_buf);

		/* Force antenna setup for no scan result issue */
		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_2G);

		rtw_btc_run_coex(btc, BTC_RSN_2GCONSTART);

		/* To keep TDMA case during connect process,
		 * to avoid changed by Btinfo and run_coex
		 */
		coex_sta->coex_freeze = TRUE;
		btc->btc_set_timer(btc, BTC_TIMER_WL_COEXFREEZE, 5);
	} else {
		coex_sta->wl_hi_pri_task1 = FALSE;
		coex_sta->coex_freeze = FALSE;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): 2G finish\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_run_coex(btc, BTC_RSN_2GCONFINISH);
	}
}

void rtw_btc_ex_media_status_notify(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	boolean wl_b_mode = FALSE;
	u8 i;

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	btc->btc_get(btc, BTC_GET_BL_WIFI_BSSID, btc->wifi_bssid);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): BSSID = %02x %02X %02X %02x %02X %02X\n",
		     __func__, btc->wifi_bssid[0],
		     btc->wifi_bssid[1], btc->wifi_bssid[2],
		     btc->wifi_bssid[3], btc->wifi_bssid[4],
		     btc->wifi_bssid[5]);
	BTC_TRACE(trace_buf);

	/* check if black-list ap */
	for (i = 0; i <= 5; i++) {
		if (btc->wifi_bssid[i] != btc->wifi_black_bssid[i])
			break;
	}

	if (i <= 5)
		coex_sta->wl_blacklist_ap = FALSE;
	else
		coex_sta->wl_blacklist_ap = TRUE;

	if (type == BTC_MEDIA_CONNECT_5G) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): 5G\n", __func__);
		BTC_TRACE(trace_buf);

		btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE, TRUE);

		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_5G);

		rtw_btc_run_coex(btc, BTC_RSN_5GMEDIA);
	} else if (type == BTC_MEDIA_CONNECT) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): 2G\n", __func__);
		BTC_TRACE(trace_buf);

		btc->btc_write_scbd(btc, BTC_SCBD_ACTIVE, TRUE);

		/* Force antenna setup for no scan result issue */
		rtw_btc_set_ant_path(btc, FC_EXCU, BTC_ANT_2G);

		btc->btc_get(btc, BTC_GET_BL_WIFI_UNDER_B_MODE, &wl_b_mode);

		/* Set CCK Tx/Rx high Pri except 11b mode */
		if (wl_b_mode)/* CCK Rx */
			rtw_btc_set_wl_pri_mask(btc, BTC_WLPRI_RX_CCK, 0);
		else /* CCK Rx */
			rtw_btc_set_wl_pri_mask(btc, BTC_WLPRI_RX_CCK, 1);

		rtw_btc_run_coex(btc, BTC_RSN_2GMEDIA);
	} else {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): disconnect!!\n", __func__);
		BTC_TRACE(trace_buf);
		coex_sta->cnt_wl[BTC_CNT_WL_ARP] = 0;

		rtw_btc_run_coex(btc, BTC_RSN_MEDIADISCON);
	}

	btc->btc_get(btc, BTC_GET_U1_IOT_PEER, &coex_sta->wl_iot_peer);
	rtw_btc_update_wl_ch_info(btc, type);
}

void rtw_btc_ex_specific_packet_notify(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	boolean under_4way = FALSE;

	if (btc->manual_control || btc->stop_coex_dm)
		return;

	if (type & BTC_5G_BAND) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): 5G\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_run_coex(btc, BTC_RSN_5GSPECIALPKT);
		return;
	}

	btc->btc_get(btc, BTC_GET_BL_WIFI_4_WAY_PROGRESS, &under_4way);

	if (under_4way) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): under_4way!!\n", __func__);
		BTC_TRACE(trace_buf);

		coex_sta->wl_hi_pri_task1 = TRUE;
		btc->btc_set_timer(btc, BTC_TIMER_WL_SPECPKT, 2);
	} else if (type == BTC_PACKET_ARP) {
		coex_sta->cnt_wl[BTC_CNT_WL_ARP]++;

		if (coex_sta->wl_hi_pri_task1) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], %s(): ARP cnt = %d\n",
				    __func__, coex_sta->cnt_wl[BTC_CNT_WL_ARP]);
			BTC_TRACE(trace_buf);
		}
	} else {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): DHCP or EAPOL Type = %d\n",
			    __func__, type);
		BTC_TRACE(trace_buf);

		coex_sta->wl_hi_pri_task1 = TRUE;
		btc->btc_set_timer(btc, BTC_TIMER_WL_SPECPKT, 2);
	}

	if (coex_sta->wl_hi_pri_task1)
		rtw_btc_run_coex(btc, BTC_RSN_2GSPECIALPKT);
}

void rtw_btc_ex_bt_info_notify(struct btc_coexist *btc, u8 *tmp_buf, u8 length)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_coex_dm *coex_dm = &btc->coex_dm;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	u8 i, rsp_source = 0, type;

	rsp_source = tmp_buf[0] & 0xf;
	if (rsp_source >= BTC_BTINFO_SRC_MAX)
		return;

	coex_sta->cnt_bt_info_c2h[rsp_source]++;

	/* bt_iqk_state-> 1: start, 0: ok, 2:fail  */
	if (rsp_source == BTC_BTINFO_SRC_BT_IQK) {
		coex_sta->bt_iqk_state = tmp_buf[1];
		if (coex_sta->bt_iqk_state == 0x0)
			coex_sta->cnt_bt[BTC_CNT_BT_IQK]++;
		else if (coex_sta->bt_iqk_state == 0x2)
			coex_sta->cnt_bt[BTC_CNT_BT_IQKFAIL]++;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BT IQK by bt_info, data0 = 0x%02x\n",
			    tmp_buf[1]);
		BTC_TRACE(trace_buf);
		return;
	}

	if (rsp_source == BTC_BTINFO_SRC_BT_SCBD) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BT Scoreboard change notify by WL FW c2h, 0xaa = 0x%02x, 0xab = 0x%02x\n",
			    tmp_buf[1], tmp_buf[2]);
		BTC_TRACE(trace_buf);
		rtw_btc_monitor_bt_enable(btc);

		if (coex_sta->bt_disabled != coex_sta->bt_disabled_pre) {
			coex_sta->bt_disabled_pre = coex_sta->bt_disabled;
			rtw_btc_run_coex(btc, BTC_RSN_BTINFO);
		}
		return;
	}

	if (rsp_source == BTC_BTINFO_SRC_H2C60) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], H2C 0x60 content replied by WL FW: H2C_0x60 = [%02x %02x %02x %02x %02x]\n",
			    tmp_buf[1], tmp_buf[2], tmp_buf[3], tmp_buf[4],
			    tmp_buf[5]);
		BTC_TRACE(trace_buf);

		for (i = 1; i <= 5; i++)
			coex_dm->fw_tdma_para[i - 1] = tmp_buf[i];
		return;
	}

	if (rsp_source == BTC_BTINFO_SRC_WL_FW) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], bt_info reply by WL FW\n");
		BTC_TRACE(trace_buf);
		rtw_btc_update_bt_link_info(btc);
		/* rtw_btc_run_coex(btc, BTC_RSN_BTINFO); */
		return;
	}

	if (rsp_source == BTC_BTINFO_SRC_BT_RSP ||
	    rsp_source == BTC_BTINFO_SRC_BT_ACT) {
		if (coex_sta->bt_disabled) {
			coex_sta->bt_disabled = FALSE;
			coex_sta->bt_reenable = TRUE;
			btc->btc_set_timer(btc, BTC_TIMER_BT_REENABLE, 15);

			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], BT enable detected by bt_info\n");
			BTC_TRACE(trace_buf);
		}
	}

	if (length != 7) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Bt_info length = %d invalid!!\n",
			    length);
		BTC_TRACE(trace_buf);
		return;
	}

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], Bt_info[%d], len=%d, data=[%02x %02x %02x %02x %02x %02x]\n",
		    tmp_buf[0], length, tmp_buf[1], tmp_buf[2], tmp_buf[3],
		    tmp_buf[4], tmp_buf[5], tmp_buf[6]);
	BTC_TRACE(trace_buf);

	for (i = 0; i < 7; i++)
		coex_sta->bt_info_c2h[rsp_source][i] = tmp_buf[i];

	if (coex_sta->bt_info_c2h[rsp_source][1] == coex_sta->bt_info_lb2 &&
	    coex_sta->bt_info_c2h[rsp_source][2] == coex_sta->bt_info_lb3 &&
	    coex_sta->bt_info_c2h[rsp_source][3] == coex_sta->bt_info_hb0 &&
	    coex_sta->bt_info_c2h[rsp_source][4] == coex_sta->bt_info_hb1 &&
	    coex_sta->bt_info_c2h[rsp_source][5] == coex_sta->bt_info_hb2 &&
	    coex_sta->bt_info_c2h[rsp_source][6] == coex_sta->bt_info_hb3) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Return because Btinfo duplicate!!\n");
		BTC_TRACE(trace_buf);
		return;
	}

	coex_sta->bt_info_lb2 = coex_sta->bt_info_c2h[rsp_source][1];
	coex_sta->bt_info_lb3 = coex_sta->bt_info_c2h[rsp_source][2];
	coex_sta->bt_info_hb0 = coex_sta->bt_info_c2h[rsp_source][3];
	coex_sta->bt_info_hb1 = coex_sta->bt_info_c2h[rsp_source][4];
	coex_sta->bt_info_hb2 = coex_sta->bt_info_c2h[rsp_source][5];
	coex_sta->bt_info_hb3 = coex_sta->bt_info_c2h[rsp_source][6];

	/* ========== BT info Low-Byte2 ========== */
	/* if 0xff, it means BT is under WHCK test */
	coex_sta->bt_whck_test = (coex_sta->bt_info_lb2 == 0xff);
	coex_sta->bt_inq_page = ((coex_sta->bt_info_lb2 & BIT(2)) == BIT(2));

	if (coex_sta->bt_inq_page_pre != coex_sta->bt_inq_page) {
		coex_sta->bt_inq_page_pre = coex_sta->bt_inq_page;
		coex_sta->bt_inq_page_remain = TRUE;

		if (!coex_sta->bt_inq_page)
			btc->btc_set_timer(btc, BTC_TIMER_BT_INQPAGE, 2);
	}
	coex_sta->bt_acl_busy = ((coex_sta->bt_info_lb2 & BIT(3)) == BIT(3));

	/* ==========  BT info Low-Byte3 ========== */
	coex_sta->cnt_bt[BTC_CNT_BT_RETRY] = coex_sta->bt_info_lb3 & 0xf;

	if (coex_sta->cnt_bt[BTC_CNT_BT_RETRY] >= 1)
		coex_sta->cnt_bt[BTC_CNT_BT_POPEVENT]++;

	coex_sta->bt_fix_2M = ((coex_sta->bt_info_lb3 & BIT(4)) == BIT(4));

	coex_sta->bt_inq = ((coex_sta->bt_info_lb3 & BIT(5)) == BIT(5));

	coex_sta->bt_mesh = ((coex_sta->bt_info_lb3 & BIT(6)) == BIT(6));

	if (coex_sta->bt_inq)
		coex_sta->cnt_bt[BTC_CNT_BT_INQ]++;

	coex_sta->bt_page = ((coex_sta->bt_info_lb3 & BIT(7)) == BIT(7));

	if (coex_sta->bt_page)
		coex_sta->cnt_bt[BTC_CNT_BT_PAGE]++;

	/* ==========  BT info High-Byte0 ========== */
	/* unit: %, value-100 to translate to unit: dBm */
	if (btc->chip_para->bt_rssi_type == BTC_BTRSSI_RATIO) {
		coex_sta->bt_rssi = coex_sta->bt_info_hb0 * 2 + 10;
	} else { /* coex_sta->bt_info_hb0 is just dbm */
		if (coex_sta->bt_info_hb0 <= 127)
			coex_sta->bt_rssi = 100;
		else if (256 - coex_sta->bt_info_hb0 <= 100)
			coex_sta->bt_rssi = 100 - (256 - coex_sta->bt_info_hb0);
		else
			coex_sta->bt_rssi = 0;
	}

	/* ==========  BT info High-Byte1 ========== */
	coex_sta->bt_ble_exist = ((coex_sta->bt_info_hb1 & BIT(0)) == BIT(0));

	if (coex_sta->bt_info_hb1 & BIT(1))
		coex_sta->cnt_bt[BTC_CNT_BT_REINIT]++;

	if ((coex_sta->bt_info_hb1 & BIT(2)) ||
	    (coex_sta->bt_page && coex_sta->wl_pnp_wakeup)) {
		coex_sta->cnt_bt[BTC_CNT_BT_SETUPLINK]++;
		coex_sta->bt_setup_link = TRUE;

		if (coex_sta->bt_reenable)
			btc->btc_set_timer(btc, BTC_TIMER_BT_RELINK, 6);
		else
			btc->btc_set_timer(btc, BTC_TIMER_BT_RELINK, 1);
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Re-Link start in BT info!!\n");
		BTC_TRACE(trace_buf);
	}

	if (coex_sta->bt_info_hb1 & BIT(3))
		coex_sta->cnt_bt[BTC_CNT_BT_IGNWLANACT]++;

	coex_sta->bt_ble_voice = ((coex_sta->bt_info_hb1 & BIT(4)) == BIT(4));
	coex_sta->bt_ble_scan_en = ((coex_sta->bt_info_hb1 & BIT(5)) == BIT(5));

	if (coex_sta->bt_info_hb1 & BIT(6))
		coex_sta->cnt_bt[BTC_CNT_BT_ROLESWITCH]++;

	coex_sta->bt_multi_link = ((coex_sta->bt_info_hb1 & BIT(7)) == BIT(7));

	/* for multi_link = 0 but bt pkt remain exist ->
	 * Use PS-TDMA to protect WL RX
	 */
	if (!coex_sta->bt_multi_link && coex_sta->bt_multi_link_pre) {
		coex_sta->bt_multi_link_remain = TRUE;
		btc->btc_set_timer(btc, BTC_TIMER_BT_MULTILINK, 3);
	}

	coex_sta->bt_multi_link_pre = coex_sta->bt_multi_link;

	/* Here we need to resend some wifi info to BT */
	/* because bt is reset and loss of the info. */
	/*  Re-Init */
	if ((coex_sta->bt_info_hb1 & BIT(1))) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BT Re-init, send wifi BW & Chnl to BT!!\n");
		BTC_TRACE(trace_buf);
		if (link_info_ext->is_connected)
			type = BTC_MEDIA_CONNECT;
		else
			type = BTC_MEDIA_DISCONNECT;
		rtw_btc_update_wl_ch_info(btc, type);
	}

	/* If Ignore_WLanAct && not SetUp_Link */
	if ((coex_sta->bt_info_hb1 & BIT(3)) &&
	    (!(coex_sta->bt_info_hb1 & BIT(2)))) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BT ext info bit3 check, set BT NOT to ignore Wlan active!!\n");
		BTC_TRACE(trace_buf);
		rtw_btc_ignore_wlan_act(btc, FC_EXCU, FALSE);
	}

	/* ==========  BT info High-Byte2 ========== */
	coex_sta->bt_opp_exist = ((coex_sta->bt_info_hb2 & BIT(0)) == BIT(0));

	if (coex_sta->bt_info_hb2 & BIT(1))
		coex_sta->cnt_bt[BTC_CNT_BT_AFHUPDATE]++;

	coex_sta->bt_a2dp_active = ((coex_sta->bt_info_hb2 & BIT(2)) == BIT(2));
	coex_sta->bt_slave = ((coex_sta->bt_info_hb2 & BIT(3)) == BIT(3));
	coex_sta->bt_hid_slot = (coex_sta->bt_info_hb2 & 0x30) >> 4;
	coex_sta->bt_hid_pair_num = (coex_sta->bt_info_hb2 & 0xc0) >> 6;

	if (coex_sta->bt_hid_pair_num > 0 && coex_sta->bt_hid_slot >= 2) {
		coex_sta->bt_418_hid_exist = TRUE;
	} else if (coex_sta->bt_hid_slot == 1 && coex_sta->bt_ctr_ok &&
		   (coex_sta->hi_pri_rx + 100 < coex_sta->hi_pri_tx) &&
		   coex_sta->hi_pri_rx < 100) {
		coex_sta->bt_ble_hid_exist = TRUE;
	} else if (coex_sta->bt_hid_pair_num == 0 ||
		   coex_sta->bt_hid_slot == 1) {
		coex_sta->bt_418_hid_exist = FALSE;
		coex_sta->bt_ble_hid_exist = FALSE;
	}

	/* ==========  BT info High-Byte3 ========== */
	if ((coex_sta->bt_info_lb2 & 0x49) == 0x49)
		coex_sta->bt_a2dp_bitpool = (coex_sta->bt_info_hb3 & 0x7f);
	else
		coex_sta->bt_a2dp_bitpool = 0;

	coex_sta->bt_a2dp_sink = ((coex_sta->bt_info_hb3 & BIT(7)) == BIT(7));

	rtw_btc_update_bt_link_info(btc);
	rtw_btc_run_coex(btc, BTC_RSN_BTINFO);
}

void rtw_btc_ex_wl_fwdbginfo_notify(struct btc_coexist *btc, u8 *tmp_buf,
				    u8 length)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	u8 i = 0, val = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], WiFi Fw Dbg info = %d %d %d %d %d %d %d %d (len = %d)\n",
		    tmp_buf[0], tmp_buf[1], tmp_buf[2], tmp_buf[3], tmp_buf[4],
		    tmp_buf[5], tmp_buf[6], tmp_buf[7], length);
	BTC_TRACE(trace_buf);

	if (tmp_buf[0] != 0x8)
		return;

	for (i = 1; i <= 7; i++) {
		val = coex_sta->wl_fw_dbg_info_pre[i];
		if (tmp_buf[i] >= val)
			coex_sta->wl_fw_dbg_info[i] = tmp_buf[i] - val;
		else
			coex_sta->wl_fw_dbg_info[i] = 255 - val + tmp_buf[i];

		coex_sta->wl_fw_dbg_info_pre[i] = tmp_buf[i];
	}

	/* wl_fwdbginfo_notify is auto send by WL FW if TDMA slot toggle = 20
	 * coex_sta->wl_fw_dbg_info[6] = TDMA slot toggle
	 * For debug, TDMA slot toggle should be calculated by 2-second
	 */
	coex_sta->cnt_wl[BTC_CNT_WL_FW_NOTIFY]++;
	rtw_btc_wl_ccklock_action(btc);
}

void rtw_btc_ex_rx_rate_change_notify(struct btc_coexist *btc,
				      BOOLEAN is_data_frame, u8 btc_rate_id)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (is_data_frame)
		coex_sta->wl_rx_rate = btc_rate_id;

	else
		coex_sta->wl_rts_rx_rate = btc_rate_id;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): rate id = %d, RTS_Rate = %d\n", __func__,
		    coex_sta->wl_rx_rate, coex_sta->wl_rts_rx_rate);
	BTC_TRACE(trace_buf);

	rtw_btc_wl_ccklock_detect(btc);
}

void rtw_btc_ex_tx_rate_change_notify(struct btc_coexist *btc, u8 tx_rate,
				      u8 tx_retry_ratio, u8 macid)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], %s(): Tx_Rate = %d, Tx_Retry_Ratio = %d, macid =%d\n",
		    __func__, tx_rate, tx_retry_ratio, macid);
	BTC_TRACE(trace_buf);

	coex_sta->wl_tx_rate = tx_rate;
	coex_sta->wl_tx_retry_ratio = tx_retry_ratio;
	coex_sta->wl_tx_macid = macid;
}

void rtw_btc_ex_rf_status_notify(struct btc_coexist *btc, u8 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;

	if (type == BTC_RF_ON) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): RF is turned ON!!\n", __func__);
		BTC_TRACE(trace_buf);
		btc->stop_coex_dm = FALSE;
		btc->wl_rf_state_off = FALSE;

	} else if (type == BTC_RF_OFF) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): RF is turned Off!!\n", __func__);
		BTC_TRACE(trace_buf);

		rtw_btc_action_wl_off(btc);
	}
}

void rtw_btc_ex_halt_notify(struct btc_coexist *btc)
{
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_action_wl_off(btc);
}

void rtw_btc_ex_pnp_notify(struct btc_coexist *btc, u8 pnp_state)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	struct btc_wifi_link_info_ext *link_info_ext = &btc->wifi_link_info_ext;
	u8 phase;

	if (pnp_state == BTC_WIFI_PNP_SLEEP ||
	    pnp_state == BTC_WIFI_PNP_SLEEP_KEEP_ANT) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): Sleep\n", __func__);
		BTC_TRACE(trace_buf);

		btc->btc_write_scbd(btc, BTC_SCBD_ALL, FALSE);

		if (pnp_state == BTC_WIFI_PNP_SLEEP_KEEP_ANT) {
			if (link_info_ext->is_all_under_5g)
				phase = BTC_ANT_5G;
			else
				phase = BTC_ANT_2G;
		} else {
			phase = BTC_ANT_WOFF;
		}
		rtw_btc_set_ant_path(btc, FC_EXCU, phase);

		btc->stop_coex_dm = TRUE;
	} else {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): Wake up\n", __func__);
		BTC_TRACE(trace_buf);
		coex_sta->wl_pnp_wakeup = TRUE;
		btc->btc_set_timer(btc, BTC_TIMER_WL_PNPWAKEUP, 3);

		/*WoWLAN*/
		if (coex_sta->wl_pnp_state_pre == BTC_WIFI_PNP_SLEEP_KEEP_ANT ||
		    pnp_state == BTC_WIFI_PNP_WOWLAN) {
			btc->stop_coex_dm = FALSE;
			rtw_btc_run_coex(btc, BTC_RSN_PNP);
		}
	}

	coex_sta->wl_pnp_state_pre = pnp_state;
}

void rtw_btc_ex_coex_dm_reset(struct btc_coexist *btc)
{
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], %s()\n", __func__);
	BTC_TRACE(trace_buf);

	rtw_btc_init_hw_config(btc, FALSE);
}

void rtw_btc_ex_periodical(struct btc_coexist *btc)
{
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], ============== Periodical ==============\n");
	BTC_TRACE(trace_buf);
}

void rtw_btc_ex_timerup_notify(struct btc_coexist *btc, u32 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	boolean is_change = FALSE;

	if (type & BIT(BTC_TIMER_WL_STAYBUSY)) {
		if (!coex_sta->wl_busy_pre) {
			coex_sta->wl_gl_busy = FALSE;
			is_change = TRUE;
			rtw_btc_update_wl_ch_info(btc, BTC_MEDIA_DISCONNECT);
			btc->btc_write_scbd(btc, BTC_SCBD_WLBUSY, FALSE);
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL busy -> idle!!\n", __func__);
			BTC_TRACE(trace_buf);
		}
	}

	 /*avoid no connect finish notify */
	if (type & BIT(BTC_TIMER_WL_COEXFREEZE)) {
		coex_sta->coex_freeze = FALSE;
		coex_sta->wl_hi_pri_task1 = FALSE;
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): Coex is de-freeze!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_TIMER_WL_SPECPKT)) {
		if (!coex_sta->coex_freeze) {
			coex_sta->wl_hi_pri_task1 = FALSE;
			is_change = TRUE;
		}
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL SPECPKT finish!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	/*for A2DP glitch during connecting AP*/
	if (type & BIT(BTC_TIMER_WL_CONNPKT)) {
		coex_sta->wl_connecting = FALSE;
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL connecting stop!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_TIMER_WL_PNPWAKEUP)) {
		coex_sta->wl_pnp_wakeup = FALSE;
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL pnp wakeup stop!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_TIMER_WL_CCKLOCK)) {
		if (coex_sta->wl_cck_lock_pre) {
			coex_sta->wl_cck_lock_ever = TRUE;
			is_change = TRUE;
		}
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL CCK Lock Detect!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_TIMER_BT_RELINK)) {
		coex_sta->bt_setup_link = FALSE;
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): Re-Link stop!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_TIMER_BT_REENABLE)) {
		coex_sta->bt_reenable = FALSE;
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BT renable finish!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_TIMER_BT_MULTILINK)) {
		coex_sta->bt_multi_link_remain = FALSE;
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BT multilink disappear !!\n",
			    __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_TIMER_BT_INQPAGE)) {
		coex_sta->bt_inq_page_remain = FALSE;
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): BT inq_page disappear !!\n",
			    __func__);
		BTC_TRACE(trace_buf);
	}

	if (is_change)
		rtw_btc_run_coex(btc, BTC_RSN_TIMERUP);
}

void rtw_btc_ex_wl_status_change_notify(struct btc_coexist *btc, u32 type)
{
	struct btc_coex_sta *coex_sta = &btc->coex_sta;
	boolean is_change = FALSE;

	if (type & BIT(BTC_WLSTATUS_CHANGE_TOIDLE)) { /* if busy->idle */
		coex_sta->wl_busy_pre = FALSE;
		btc->btc_set_timer(btc, BTC_TIMER_WL_STAYBUSY, 6);
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL busy -> idle!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_WLSTATUS_CHANGE_TOBUSY)) { /* if idle->busy */
		coex_sta->wl_gl_busy = TRUE;
		coex_sta->wl_busy_pre = TRUE;
		is_change = TRUE;
		rtw_btc_update_wl_ch_info(btc, BTC_MEDIA_CONNECT);
#if 0
		btc->btc_write_scbd(btc, BTC_SCBD_WLBUSY, TRUE);
#endif
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL idle -> busy!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_WLSTATUS_CHANGE_RSSI)) { /* if RSSI change */
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL RSSI change!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_WLSTATUS_CHANGE_LINKINFO)) { /* if linkinfo change */
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL LinkInfo change!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_WLSTATUS_CHANGE_DIR)) { /*if WL UL-DL change*/
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s(): WL UL-DL change!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_WLSTATUS_CHANGE_NOISY)) { /*if noisy level change*/
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s():Noisy Level change!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_WLSTATUS_CHANGE_BTCNT)) { /*if BT counter change*/
		is_change = TRUE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s():BT counter change!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (type & BIT(BTC_WLSTATUS_CHANGE_LOCKTRY)) { /*if WL CCK lock try*/
		is_change = TRUE;
		coex_sta->wl_cck_lock_ever = FALSE;
		coex_sta->wl_cck_lock = FALSE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], %s():WL CCK lock try!!\n", __func__);
		BTC_TRACE(trace_buf);
	}

	if (is_change)
		rtw_btc_run_coex(btc, BTC_RSN_WLSTATUS);
}
#endif
 /* #if (BT_SUPPORT == 1 && COEX_SUPPORT == 1) */

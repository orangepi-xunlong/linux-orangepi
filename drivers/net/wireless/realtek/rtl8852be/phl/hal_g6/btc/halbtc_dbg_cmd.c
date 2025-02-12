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
#define _HAL_BTC_DBG_CMD_C_
#include "../hal_headers_le.h"
#include "hal_btc.h"
#include "halbtc_fw.h"
#include "halbtc_fwdef.h"
#include "halbtc_action.h"
#include "halbtc_def.h"

#ifdef CONFIG_BTCOEX

#define	CLI_PRT(fmt, ...)\
	do {							\
		if (*used < out_len)			\
			*used += _os_snprintf(output + *used,	\
					      out_len - *used, fmt,\
					      ##__VA_ARGS__);\
	} while (0)

struct halbtc_cmd_info {
	char name[16];
	u8 id;
};

enum HALBTC_CMD_ID {
	HALBTC_HELP,
	HALBTC_SHOW,
	HALBTC_READ_BT,
	HALBTC_WRITE_BT,
	HALBTC_SET_COEX,
	HALBTC_UPDATE_POLICY,
	HALBTC_TDMA,
	HALBTC_SLOT,
	HALBTC_SIG_GDBG_EN,
	HALBTC_SGPIO_MAP,
	HALBTC_TRACE_STEP,
	HALBTC_WL_TX_POWER,
	HALBTC_WL_RX_LNA,
	HALBTC_BT_AFH_MAP,
	HALBTC_BT_TX_POWER,
	HALBTC_BT_RX_LNA,
	HALBTC_BT_IGNO_WLAN,
	HALBTC_SET_GNT_WL,
	HALBTC_SET_GNT_BT,
	HALBTC_SET_BT_PSD,
	HALBTC_GET_WL_NHM_DBM,
	HALBTC_DBG,
#ifdef BTC_AISO_SUPPORT
	HALBTC_AISO,
#endif

#ifdef BTC_FDDT_TRAIN_SUPPORT
	HALBTC_FT_SET,
	HALBTC_FT_SHOW,
	HALBTC_FT_CTRL
#endif
};

enum btc_fddt_cell_type {
	BTC_FDDT_CELL_RANGE_UL = BIT(TRAFFIC_UL),
	BTC_FDDT_CELL_RANGE_DL = BIT(TRAFFIC_DL),
	BTC_FDDT_CELL_STATE_UL = 0x80 + BIT(TRAFFIC_UL),
	BTC_FDDT_CELL_STATE_DL = 0x80 + BIT(TRAFFIC_DL)
};

struct halbtc_cmd_info halbtc_cmd_i[] = {
	{"-h", HALBTC_HELP},
	{"show", HALBTC_SHOW},
	{"rb", HALBTC_READ_BT},
	{"wb", HALBTC_WRITE_BT},
	{"mode", HALBTC_SET_COEX},
	{"update", HALBTC_UPDATE_POLICY},
	{"tdma", HALBTC_TDMA},
	{"slot", HALBTC_SLOT},
	{"sig", HALBTC_SIG_GDBG_EN},
	{"gpio", HALBTC_SGPIO_MAP},
	{"wpwr", HALBTC_WL_TX_POWER},
	{"wlna", HALBTC_WL_RX_LNA},
	{"afh", HALBTC_BT_AFH_MAP},
	{"bpwr", HALBTC_BT_TX_POWER},
	{"blna", HALBTC_BT_RX_LNA},
	{"igwl", HALBTC_BT_IGNO_WLAN},
	{"gwl", HALBTC_SET_GNT_WL},
	{"gbt", HALBTC_SET_GNT_BT},
	{"bpsd", HALBTC_SET_BT_PSD},
	{"wnhm", HALBTC_GET_WL_NHM_DBM},
	{"dbg", HALBTC_DBG},
#ifdef BTC_AISO_SUPPORT
	{"aiso", HALBTC_AISO},
#endif

#ifdef BTC_FDDT_TRAIN_SUPPORT
	{"ftset", HALBTC_FT_SET},
	{"ftshow", HALBTC_FT_SHOW},
	{"ftctrl", HALBTC_FT_CTRL},
#endif
};

#define _limit_val(val, max, min) \
	val = (val > max ? max: (val < min ? min : val))

static void _cmd_rb(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
		    u32 input_num, char *output, u32 out_len)
{
	u32 i = 0, type = 0, addr = 0;

	if (input_num >= 3) {
		_os_sscanf(input[1], "%d", &type);
		_os_sscanf(input[2], "%x", &addr);
	}

	if (type > 4 || input_num < 3) {
		CLI_PRT(" rb <type --- 0:rf, 1:modem, 2:bluewize, 3:vendor,"
			" 4:LE> <addr:16bits> \n");
		return;
	}

	btc->dbg.rb_done = false;
	btc->dbg.rb_val = 0xffffffff;
	_read_bt_reg(btc, (u8)(type), (u16)addr);
	for (i = 0; i < 50; i++) {
		if (!btc->dbg.rb_done)
			hal_mdelay(btc->hal, 10);
		else
			break;
	}

	if (!btc->dbg.rb_done) {
		CLI_PRT(" timeout !!\n");
	} else {
		CLI_PRT(" rb %d(0x%x), val=0x%x\n",
			type, addr, btc->dbg.rb_val);
	}
}

static void _show_cx_info(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			   u32 input_num, char *output, u32 out_len)
{
	struct btc_dm *dm = &btc->dm;
	struct btc_bt_info *bt = &btc->cx.bt;
	struct btc_wl_info *wl = &btc->cx.wl;
	struct rtw_phl_com_t *p = btc->phl;
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	struct btc_module *md = &btc->mdinfo;
	u32 ver_main = 0, ver_sub = 0, ver_hotfix = 0, id_branch = 0;
	u32 name_branch = BTC_BRANCH_FORMAL;

	if (!(dm->coex_info_map & BTC_COEX_INFO_CX))
		return;

	dm->cnt_notify[BTC_NCNT_SHOW_COEX_INFO]++;

	CLI_PRT("\n\r========== [BTC COEX INFO (%s%s%s%s%s)] ==========",
		id_to_str(BTC_STR_CHIPID, (u32)btc->hal->chip_id),
		(btc->hal->chip_id == CHIP_WIFI6_8852B &&
		md->ant.num == 1 ? "-VS" : ""),
		(p->hci_type & RTW_HCI_PCIE ?  " PCIe" : ""),
		(p->hci_type & RTW_HCI_USB ? " USB" : ""),
		(p->hci_type & RTW_HCI_SDIO ? " SDIO" : ""));

	ver_main = (coex_ver & bMASKB3) >> 24;
	ver_sub = (coex_ver & bMASKB2) >> 16;
	ver_hotfix = (coex_ver & bMASKB1) >> 8;
	id_branch = coex_ver & bMASKB0;
	CLI_PRT("\n\r %-15s : Coex:%d.%d.%d(branch:%s_%d), ", "[coex_version]",
		ver_main, ver_sub, ver_hotfix,
		id_to_str(BTC_STR_BRANCH, name_branch), id_branch);

	ver_main = (wl->ver_info.fw_coex & bMASKB3) >> 24;
	ver_sub = (wl->ver_info.fw_coex & bMASKB2) >> 16;
	ver_hotfix = (wl->ver_info.fw_coex & bMASKB1) >> 8;
	id_branch = wl->ver_info.fw_coex & bMASKB0;

	CLI_PRT("WL_FW_coex:%d.%d.%d(branch:%d)",
		ver_main, ver_sub, ver_hotfix, id_branch);

	ver_main = (btc->chip->wlcx_desired & bMASKB3) >> 24;
	ver_sub = (btc->chip->wlcx_desired & bMASKB2) >> 16;
	ver_hotfix = (btc->chip->wlcx_desired & bMASKB1) >> 8;

	CLI_PRT("(%s, desired:%d.%d.%d), ",
		(wl->fw_ver_mismatch ? "Mis-Match" : "Match"),
		 ver_main, ver_sub, ver_hotfix);

	CLI_PRT("BT_FW_coex:%d(%s, desired:%d)",
		bt->ver_info.fw_coex,
		(bt->fw_ver_mismatch ? "Mis-Match" : "Match"),
		btc->chip->btcx_desired);

	CLI_PRT("\n\r %-15s : WL_FW:%d.%d.%d.%d, BT_FW:0x%x(%s)",
		"[sub_module]", (wl->ver_info.fw & bMASKB3) >> 24,
		(wl->ver_info.fw & bMASKB2) >> 16,
		(wl->ver_info.fw & bMASKB1) >> 8,
		(wl->ver_info.fw & bMASKB0), bt->ver_info.fw,
		(bt->run_patch_code ? "patch" : "ROM"));

	CLI_PRT("\n\r %-15s : kt_ver:%x,"
		" rfe_type:0x%x(ant_cnt:%d/ext_sw:%d/ant_div:%d), hw_id:0x%x,"
		 " 3rd_coex:%d,"
		 " Endian:%s", "[hw_info]",
		md->kt_ver, md->rfe_type, md->ant.num,
		md->switch_type, md->ant.diversity,
		((btc->phl->id.id & 0xff00) >> 8), btc->cx.other.type,
		BTC_PLATFORM_BIG_ENDIAN ? "Big" : "Little");

	if (md->ant.type == BTC_ANT_SHARED)
		return;

	if (bt_psd_dm->aiso_db_cnt > 0)
		CLI_PRT(" ,ant_iso:%ddB(cal)", md->ant.isolation);
	else
		CLI_PRT(" ,ant_iso:%ddB(def)", md->ant.isolation);
}

static void _show_wl_role_info(struct btc_t *btc, u32 *used,
				char input[][MAX_ARGV], u32 input_num,
				char *output, u32 out_len)
{
	struct btc_wl_link_info *plink = NULL;
	struct btc_wl_info *wl = &btc->cx.wl;
	struct btc_wl_role_info *wl_rinfo = &wl->role_info;
	struct btc_wl_dbcc_info *wd = &wl->dbcc_info;
	struct btc_dm *dm = &btc->dm;
	u8 i;

	if (wl_rinfo->dbcc_en) {
		CLI_PRT("\n\r %-15s : 2G_PHY:%s, chg_cnt:%d, all_2G_cnt:%d",
			"[dbcc_info]",
			(wl_rinfo->dbcc_2g_phy == HW_PHY_MAX ? "None" :
			(wl_rinfo->dbcc_2g_phy == HW_PHY_0 ? "PHY0" : "PHY1")),
			btc->cx.cnt_wl[BTC_WCNT_DBCC_CHG],
			btc->cx.cnt_wl[BTC_WCNT_DBCC_ALL_2G]);

		for (i = HW_PHY_0; i < HW_PHY_MAX; i++) {
			CLI_PRT(", PHY-%d(op:%s/scan:%s/real:%s/role:0x%x)", i,
				id_to_str(BTC_STR_BAND, (u32)wd->op_band[i]),
				id_to_str(BTC_STR_BAND, (u32)wd->scan_band[i]),
				id_to_str(BTC_STR_BAND, (u32)wd->real_band[i]),
				wd->role[i]);
		}
	}

	for (i = 0; i < BTC_WL_MAX_ROLE_NUMBER; i++) {
		plink = &btc->cx.wl.link_info[i];

		if (!plink->active)
			continue;

		CLI_PRT("\n\r [band%d-port%d]   : %s(rID:%d/mID:%d), %s-%s",
			plink->phy, plink->pid,
			id_to_str(BTC_STR_ROLE, (u32)plink->role),
			i, plink->mac_id,
			id_to_str(BTC_STR_WLMODE, (u32)plink->mode),
			id_to_str(BTC_STR_MSTATE, (u32)plink->connected));

		if (plink->connected == MLME_NO_LINK)
			continue;

		if (plink->role == PHL_RTYPE_AP ||
		    plink->role == PHL_RTYPE_P2P_GO)
			CLI_PRT("(clients:%d/Lps:%d/Tdma:%d)",
				plink->client_cnt-1,
				plink->client_ps,
				dm ->client_ps_tdma_on);
		else if (plink->role == PHL_RTYPE_STATION ||
			 plink->role == PHL_RTYPE_P2P_GC)
			CLI_PRT("(bID:0x%02x%02x%02x%02x%02x%02x)",
				plink->mac_addr[0], plink->mac_addr[1],
				plink->mac_addr[2], plink->mac_addr[3],
				plink->mac_addr[4], plink->mac_addr[5]);

		CLI_PRT(", rssi:-%ddBm(%dp), rate(tx:%s/rx:%s),"
			" ch(ct:%d/pr:%d/bw:%s)",
			110-plink->rssi, plink->rssi,
			id_to_str(BTC_STR_RATE, (u32)plink->tx_rate),
			id_to_str(BTC_STR_RATE, (u32)plink->rx_rate),
			plink->chdef.center_ch, plink->chdef.chan,
			id_to_str(BTC_STR_WLBW, (u32)plink->chdef.bw));

		if (plink->role == PHL_RTYPE_AP ||
		    plink->role == PHL_RTYPE_P2P_GO ||
		    plink->role == PHL_RTYPE_P2P_GC)
			CLI_PRT(", noa(%s):%d.%03dms",
				(plink->noa ? "on" : "off"),
				plink->noa_duration/1000,
				plink->noa_duration%1000);
	}
}

static void _show_wl_info(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			  u32 input_num, char *output, u32 out_len)
{
	struct btc_cx *cx = &btc->cx;
	struct rtw_phl_com_t *p = btc->phl;
	struct btc_wl_info *wl = &cx->wl;
	struct btc_wl_role_info *wl_rinfo = &wl->role_info;
	struct btc_wl_smap *wl_smap = &wl->status.map;
	struct btc_module *md = &btc->mdinfo;
	struct btc_traffic *wl_tra = &btc->cx.wl.traffic;
	struct btc_chip_ops *ops = btc->chip->ops;
	char *s_tx = "", *s_rx = "", *s_1ant = "";
	u8 pos = BTC_ANT_DIV_MAIN;

	if (!(btc->dm.coex_info_map & BTC_COEX_INFO_WL))
		return;

	CLI_PRT("\n\r========== [WL Status%s] ==========",
		(p->drv_mode == RTW_DRV_MODE_MP ? "(MP-mode)" : ""));

	CLI_PRT("\n\r %-15s : link_mode:%s(%s%s/%s(0x%x)), mrole:%s,"
		" dbcc_en:%d, ", "[status]",
		id_to_str(BTC_STR_WLLINK, (u32)wl_rinfo->link_mode),
		(wl_smap->busy == 1 ? "busy" : "idle"),
		(wl->busy_to_idle == 1? "->idle" : ""),
		(wl_smap->traffic_dir & BIT(TRAFFIC_UL) ? "UL" : "DL"),
		wl_smap->traffic_dir,
		id_to_str(BTC_STR_MROLE, wl_rinfo->mrole_type),
		wl->role_info.dbcc_en);

#ifdef BTC_CONFIG_FW_IO_OFLD_SUPPORT
	CLI_PRT("IO-offload:%d, ", hal_btc_check_io_ofld(btc));
#else
	CLI_PRT("IO-offload:0, ");
#endif

	CLI_PRT("rf_off:%d, LPS:%s, nhm:%d(max:%d/min:%d), cn:%d",
		wl_smap->rf_off,
		(wl_smap->lps == BTC_LPS_OFF? "0":
		(wl_smap->lps == BTC_LPS_RF_OFF? "1(RF-Off)":"1(RF-on)")),
		wl->nhm.pwr, wl->nhm.pwr_max, wl->nhm.pwr_min, wl->cn_report);

	CLI_PRT("\n\r %-15s : scan:%d(band:%s/phy_map:0x%x/offload:%d),"
		" connecting:%d, roam:%d, _4way:%d, dbccing:%d, cx_state:%s",
		"[scan]", wl_smap->scan,
		id_to_str(BTC_STR_BAND, (u32)wl->scan_info.band[HW_PHY_0]),
		wl->scan_info.phy_map, btc->hal->scanofld_en,
		wl_smap->connecting, wl_smap->roaming, wl_smap->_4way,
		wl_smap->dbccing, id_to_str(BTC_STR_CXSTATE, cx->state_map));

	if (ops && ops->get_reg_status)
		ops->get_reg_status(btc, BTC_CSTATUS_TXDIV_POS, (void*)&pos);
	s_tx = pos ? "/pos:aux" : "/pos:main";

	if (ops && ops->get_reg_status)
		ops->get_reg_status(btc, BTC_CSTATUS_RXDIV_POS, (void*)&pos);
	s_rx = pos ? "/pos:aux" : "/pos:main";

	s_1ant = md->ant.single_pos? "(s1)" : "(s0)";

	CLI_PRT("\n\r %-15s : tx[num:%d/ss:%d%s%s], rx[num:%d/ss:%d%s%s]",
		"[ant_stream]",
		p->phy_cap[0].tx_path_num, p->phy_cap[0].txss,
		(p->phy_cap[0].txss != 1? "" : s_1ant), s_tx,
		p->phy_cap[0].rx_path_num, p->phy_cap[0].rxss,
		(p->phy_cap[0].rxss != 1? "" : s_1ant), s_rx);

	if (wl->role_info.dbcc_en)
		CLI_PRT(", tx1[num:%d/ss:%d], rx1[num:%d/ss:%d]",
			p->phy_cap[1].tx_path_num, p->phy_cap[1].txss,
			p->phy_cap[1].rx_path_num, p->phy_cap[1].rxss);

	CLI_PRT("\n\r %-15s : tx[tp:%dMbps/lvl:%d],"
		" rx[tp:%dMbps/lvl:%d/err_ratio:%dp(all-pkts:%d)/"
		"evm(1ss:%d/2ss_max:%d/2ss_min:%d)]", "[trx_stat]",
		wl_tra->tx_tp, wl_tra->tx_lvl, wl_tra->rx_tp, wl_tra->rx_lvl,
		wl->rx_err_ratio_2s, cx->cnt_wl[BTC_WCNT_RX_LAST],
		wl->evm_1ss_rpt, wl->evm_2ss_max_rpt, wl->evm_2ss_min_rpt);

	_show_wl_role_info(btc, used, input, input_num, output, out_len);
}

static void _show_dm_step(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			  u32 input_num, char *output, u32 out_len)
{
	struct btc_dm *dm = &btc->dm;
	u32 n_begin = 0, n_end = 0, i = 0, cnt = 0;
	u32 step_cnt = dm->dm_step.cnt;

	if (step_cnt == 0)
		return;

	if (step_cnt <= BTC_DM_MAXSTEP)
		n_begin = 1;
	else
		n_begin = step_cnt - BTC_DM_MAXSTEP + 1;

	n_end = step_cnt;

	if (n_begin > n_end)
		return;

	for (i = n_begin; i <= n_end; i++) {
		if (cnt % 6 == 0)
			CLI_PRT("\n\r %-15s : ", "[dm_steps]");

		CLI_PRT("-> %s", dm->dm_step.step[(i-1) % BTC_DM_MAXSTEP]);
		cnt++;
	}
}

static void _show_bt_profile_info(struct btc_t *btc, u32 *used,
				  char input[][MAX_ARGV], u32 input_num,
				  char *output, u32 out_len)
{
	struct btc_bt_link_info *bt_linfo = &btc->cx.bt.link_info;
	struct btc_bt_hfp_desc *hfp = &bt_linfo->hfp_desc;
	struct btc_bt_hid_desc *hid = &bt_linfo->hid_desc;
	struct btc_bt_a2dp_desc *a2dp = &bt_linfo->a2dp_desc;
	struct btc_bt_pan_desc *pan = &bt_linfo->pan_desc;
	u8 i;

	if (hfp->exist) {
#if 0
		CLI_PRT("\n\r %-15s : type:%s", "[HFP]");
#endif
	}

	if (hid->exist) {
		CLI_PRT("\n\r %-15s : pair-cnt:%d", "[HID]", hid->cnt);

		for (i = 0; i < hid->cnt; i++) {
			if (!hid->links[i].update)
				continue;

			CLI_PRT(", Dev-0x%x[v-id:0x%x/%s/%s/slot:%d-%d]",
				hid->links[i].h, hid->links[i].vendor_id,
				(hid->links[i].type? "BLE" : "EDR"),
				(hid->links[i].role? "Master" : "Slave"),
				hid->links[i].window, hid->links[i].interval);
		}
	}

	if (a2dp->exist) {
		CLI_PRT("\n\r %-15s : pair-cnt:%d, vendor-id:0x%x,"
			" flush-time:%d", "[A2DP]",
			a2dp->cnt, a2dp->vendor_id, a2dp->flush_time);

		for (i = 0; i < a2dp->cnt; i++) {
			if (!a2dp->links[i].update)
				continue;

			CLI_PRT(", Dev-0x%x[%s/%s]",
				a2dp->links[i].h,
				(a2dp->links[i].type? "BLE" : "EDR"),
				(a2dp->links[i].role? "Master" : "Slave"));
		}
	}

	if (pan->exist) {
		CLI_PRT("\n\r %-15s : pair-cnt:%d", "[PAN]", pan->cnt);

		for (i = 0; i < pan->cnt; i++) {
			if (!pan->links[i].update)
				continue;

			CLI_PRT(", Dev-0x%x[%s/%s/%s]",
				pan->links[i].h,
				(pan->links[i].type? "BLE" : "EDR"),
				(pan->links[i].role? "Master" : "Slave"),
				(pan->links[i].direct? "Tx" : "Rx"));
		}
	}
}

static void _show_bt_info(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			  u32 input_num, char *output, u32 out_len)
{
	struct btc_cx *cx = &btc->cx;
	struct btc_bt_info *bt = &cx->bt;
	struct btc_wl_info *wl = &cx->wl;
	struct btc_module *module = &btc->mdinfo;
	struct btc_bt_link_info *bt_linfo = &bt->link_info;
	u8 *afh = bt_linfo->afh_map;
	u8 *afh_le = bt_linfo->afh_map_le;
	u32 rpt_map = btc->fwinfo.rpt_en_map;

	if (!(btc->dm.coex_info_map & BTC_COEX_INFO_BT))
		return;

	CLI_PRT("%s", "\n\r========== [BT Status] ==========");

	CLI_PRT("\n\r %-15s : enable:%d, btg:%d%s, connect:%d, ",
		"[status]", bt->enable.now, bt->btg_type,
		(bt->enable.now && (bt->btg_type != module->bt_pos)?
		"(efuse-mismatch!!)" : ""), bt_linfo->status.map.connect);

	CLI_PRT("rssi:%ddBm(lvl:%d), tx_rate:%dM, ",
		bt_linfo->rssi-100, bt->rssi_level, (bt_linfo->tx_3M? 3 : 2));

	CLI_PRT("igno_wl:%d, mailbox_avl:%d, rfk_state:0x%x",
		bt->igno_wl, bt->mbx_avl, bt->rfk_info.val);

	CLI_PRT("\n\r %-15s : profile:%s%s%s%s%s", "[profile]",
		((bt_linfo->profile_cnt.now == 0) ? "None," : ""),
		(bt_linfo->hfp_desc.exist? "HFP," : ""),
		(bt_linfo->hid_desc.exist? "HID," : ""),
		(bt_linfo->a2dp_desc.exist?
		(bt_linfo->a2dp_desc.sink ? "A2DP_sink," :"A2DP,") : ""),
		(bt_linfo->pan_desc.exist? "PAN," : ""));

	CLI_PRT(" multi-link:%d, role:%s, ble-connect:%d, CQDDR:%d,"
		" A2DP_active:%d, PAN_active:%d",
		bt_linfo->multi_link.now,
		(bt_linfo->slave_role ? "Slave" : "Master"),
		bt_linfo->status.map.ble_connect,
		bt_linfo->cqddr,
		bt_linfo->a2dp_desc.active,
		bt_linfo->pan_desc.active);

	CLI_PRT("\n\r %-15s : wl_ch_map[en:%d/ch:%d/bw:%d]", "[AFH]",
		wl->afh_info.en, wl->afh_info.ch, wl->afh_info.bw);

	CLI_PRT(", Legacy[%02x%02x_%02x%02x_%02x%02x_%02x%02x_%02x%02x]"
		"(cnt:%d/co_ch:%d)",
		afh[0], afh[1], afh[2], afh[3], afh[4],
		afh[5], afh[6], afh[7], afh[8], afh[9],
		cx->cnt_bt[BTC_BCNT_AFH_UPDATE],
		cx->cnt_bt[BTC_BCNT_AFH_CONFLICT]);

	if (bt_linfo->status.map.ble_connect) {
		CLI_PRT(", LE[%02x%02x_%02x_%02x%02x](cnt:%d/co_ch:%d)",
			afh_le[0], afh_le[1], afh_le[2], afh_le[3], afh_le[4],
			cx->cnt_bt[BTC_BCNT_AFH_LE_UPDATE],
			cx->cnt_bt[BTC_BCNT_AFH_LE_CONFLICT]);
	}

	CLI_PRT("\n\r %-15s : retry:%d, relink:%d, rate_chg:%d,"
		" reinit:%d, reenable:%d, ", "[stat_cnt]",
		cx->cnt_bt[BTC_BCNT_RETRY],
		cx->cnt_bt[BTC_BCNT_RELINK],
		cx->cnt_bt[BTC_BCNT_RATECHG],
		cx->cnt_bt[BTC_BCNT_REINIT],
		cx->cnt_bt[BTC_BCNT_REENABLE]);

	CLI_PRT("role-sw:%d, afh-sw:%d, inq:%d, pag:%d, igno_wl:%d",
		cx->cnt_bt[BTC_BCNT_ROLESW],
		cx->cnt_bt[BTC_BCNT_AFH],
		cx->cnt_bt[BTC_BCNT_INQ],
		cx->cnt_bt[BTC_BCNT_PAGE],
		cx->cnt_bt[BTC_BCNT_IGNOWL]);

	_show_bt_profile_info(btc, used, input, input_num, output, out_len);

	CLI_PRT("\n\r %-15s : raw_data[%02x %02x %02x %02x %02x %02x]"
		"(type:%s/cnt:%d/same:%d)", "[bt_info]",
		bt->raw_info[2], bt->raw_info[3],
		bt->raw_info[4], bt->raw_info[5],
		bt->raw_info[6], bt->raw_info[7],
		(bt->raw_info[0] == BTC_BTINFO_AUTO ? "auto" : "reply"),
		cx->cnt_bt[BTC_BCNT_INFOUPDATE],
		cx->cnt_bt[BTC_BCNT_INFOSAME]);

	CLI_PRT("\n\r %-15s : Hi-rx=%d, Hi-tx=%d, Lo-rx=%d, Lo-tx=%d"
		" (polut_wl_tx:%d/rx_scan_LP:%d)", "[trx_req_cnt]",
		cx->cnt_bt[BTC_BCNT_HIPRI_RX],
		cx->cnt_bt[BTC_BCNT_HIPRI_TX],
		cx->cnt_bt[BTC_BCNT_LOPRI_RX],
		cx->cnt_bt[BTC_BCNT_LOPRI_TX],
		cx->cnt_bt[BTC_BCNT_POLUT],
		bt->scan_rx_low_pri);

	if (!bt->scan_info_update) {
		rpt_map |= RPT_EN_BT_SCAN_INFO;
	} else {
		rpt_map &= ~RPT_EN_BT_SCAN_INFO;
		CLI_PRT("(BG:%d-%d", bt->scan_info[CXSCAN_BG].win,
			bt->scan_info[CXSCAN_BG].intvl);
		CLI_PRT("/INIT:%d-%d", bt->scan_info[CXSCAN_INIT].win,
			bt->scan_info[CXSCAN_INIT].intvl);
		CLI_PRT("/LE:%d-%d)", bt->scan_info[CXSCAN_LE].win,
			bt->scan_info[CXSCAN_LE].intvl);
	}

	if (bt_linfo->profile_cnt.now)
		rpt_map |= RPT_EN_BT_AFH_MAP;
	else
		rpt_map &= ~RPT_EN_BT_AFH_MAP;

	if (bt_linfo->status.map.ble_connect)
		rpt_map |= RPT_EN_BT_AFH_MAP_LE;
	else
		rpt_map &= ~RPT_EN_BT_AFH_MAP_LE;

	if (bt_linfo->a2dp_desc.exist &&
	    (bt_linfo->a2dp_desc.flush_time == 0 ||
	     bt_linfo->a2dp_desc.vendor_id == 0 ||
	     bt_linfo->a2dp_desc.play_latency == 1))
		rpt_map |= RPT_EN_BT_DEVICE_INFO;
	else
		rpt_map &= ~RPT_EN_BT_DEVICE_INFO;

	_set_fw_rpt(btc, rpt_map, 2);
}

static void _show_fbtc_tdma(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct btc_rpt_cmn_info *pcinfo = NULL;
	struct fbtc_tdma *t = NULL;

	pcinfo = &pfwinfo->rpt_fbtc_tdma.cinfo;
	if (!pcinfo->valid)
		return;

	t = &pfwinfo->rpt_fbtc_tdma.finfo.tdma;

	CLI_PRT("\n\r %-15s : policy:%s  [type:%s/rx_flow:%d/tx_flow:%d/",
		"[tdma_policy]",
		id_to_str(BTC_STR_POLICY, (u32)btc->policy_type),
		id_to_str(BTC_STR_TDMA, (u32)t->type),
		t->rxflctrl, t->txflctrl);

	CLI_PRT("leak_n:%d,/ext_ctrl:%d/null_role:0x%x/opt:0x%x]",
		t->leak_n, t->ext_ctrl, t->rxflctrl_role, t->option_ctrl);
}

static void _show_dm_info(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			  u32 input_num, char *output, u32 out_len)
{
	struct btc_dm *dm = &btc->dm;
	struct btc_wl_info *wl = &btc->cx.wl;
	struct btc_bt_info *bt = &btc->cx.bt;
	struct btc_bt_psd_dm *bp = &btc->bt_psd_dm;
	struct btc_aiso_val *av = &bp->aiso_val;
	struct btc_ant_info *ant = &btc->mdinfo.ant;
	u8 cnt, aiso_cur, aiso_ori = ant->isolation;

	if (!(dm->coex_info_map & BTC_COEX_INFO_DM))
		return;

	CLI_PRT("\n\r========== [Mechanism Status %s] ==========",
		(btc->ctrl.manual? "(Manual)":"(Auto)"));

	CLI_PRT("\n\r %-15s : type:%s, reason:%s(), action:%s(),"
		" ant_path:%s, run_cnt:%d", "[status]",
		(ant->type == BTC_ANT_SHARED ? "shared" : "non-shared"),
		dm->run_reason, dm->run_action,
		id_to_str(BTC_STR_ANTPATH, dm->set_ant_path & 0xff),
		dm->cnt_dm[BTC_DCNT_RUN]);

	_show_fbtc_tdma(btc, used, input, input_num, output, out_len);

	CLI_PRT("\n\r %-15s : wl_only:%d, bt_only:%d, igno_bt:%d,"
		" free_run:%d, wl_ps_ctrl:%d, ", "[dm_flag]",
		dm->wl_only, dm->bt_only, btc->ctrl.igno_bt,
		dm->freerun, btc->hal->btc_ctrl.lps);

#ifdef BTC_FDDT_TRAIN_SUPPORT
	CLI_PRT("leak_ap:%d, fddt_train:%d", dm->leak_ap, dm->fddt_train);
#else
	CLI_PRT("leak_ap:%d, fddt_train:No-Support", dm->leak_ap);
#endif

	CLI_PRT("\n\r %-15s : wl_tx_limit[en:%d/max_t:%dus/max_retry:%d],"
		" bt_slot_req:[%d/%d], bt_stbc_req:%d", "[dm_drv_ctrl]",
		dm->wl_tx_limit.en,
		dm->wl_tx_limit.tx_time, dm->wl_tx_limit.tx_retry,
		btc->bt_req_len[HW_PHY_0], btc->bt_req_len[HW_PHY_1],
		btc->bt_req_stbc);

	CLI_PRT("\n\r %-15s : wl[rssi_lvl:%d/para:%d/tx_pwr:%d/rx_lvl:%d"
		"(pre_agc:%d/lna2:%d)/"
		"btg_rx:%d/stb_chg:%d] ", "[dm_rf_ctrl]",
		wl->rssi_level, dm->trx_para_level,
		dm->rf_trx_para.wl_tx_power, dm->rf_trx_para.wl_rx_gain,
		dm->wl_pre_agc, dm->wl_lna2,
		dm->wl_btg_rx, dm->wl_stb_chg);

	CLI_PRT("bt[tx_pwr_dec:%d/rx_lna:%d(%s-tbl)]",
		dm->rf_trx_para.bt_tx_power, dm->rf_trx_para.bt_rx_gain,
		(bt->hi_lna_rx? "Hi" : "Ori"));

	if (bp->en || bp->aiso_data_ok || bp->rec_time_out || bp->aiso_db_cnt) {
		cnt = bp->aiso_db_cnt;
		aiso_cur = (cnt > 0 ? bp->aiso_db[(cnt-1) & 0xf] : aiso_ori);
		CLI_PRT("\n\r %-15s : cur_iso:%d dB(cnt:%d), en:%d, run:%d,"
			" timeout:%d, data_ok:%d, psd_cnt:%d, exec_cmd_cnt:%d",
			"[dm_ant_iso]", aiso_cur, cnt, bp->en, bp->rec_start,
			bp->rec_time_out, bp->aiso_data_ok, av->psd_rec_cnt,
			bp->aiso_cmd_cnt);
	}
	_show_dm_step(btc, used, input, input_num, output, out_len);
}

static void _show_mreg(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			u32 input_num, char *output, u32 out_len)
{
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct btc_rpt_cmn_info *pcinfo = NULL;
	struct fbtc_mreg_val *pmreg = NULL;
	struct fbtc_gpio_dbg *gdbg = NULL;
	struct btc_cx *cx = &btc->cx;
	struct btc_dm *dm = &btc->dm;
	struct btc_wl_info *wl = &btc->cx.wl;
	struct btc_bt_info *bt = &btc->cx.bt;
	struct btc_wl_role_info *wl_rinfo = &wl->role_info;
	struct btc_gnt_ctrl *gnt = NULL;
	u8 i = 0, type = 0, cnt = 0;
	u32 val, offset;

	if (!(dm->coex_info_map & BTC_COEX_INFO_MREG))
		return;

	CLI_PRT("\n\r========== [HW Status] ==========");

	CLI_PRT("\n\r %-15s : "
		"WL->BT:0x%08x(cnt:%d), BT->WL:0x%08x(total:%d, bt_update:%d)",
		"[scoreboard]", wl->scbd, cx->cnt_wl[BTC_WCNT_SCBDUPDATE],
		bt->scbd, cx->cnt_bt[BTC_BCNT_SCBDREAD],
		cx->cnt_bt[BTC_BCNT_SCBDUPDATE]);

	/* To avoid I/O if WL LPS or power-off  */
	_read_cx_ctrl(btc, &val);
	dm->pta_owner = val;

	CLI_PRT("\n\r %-15s : pta_owner:%s, pta_req_mac:MAC%d", "[gnt_status]",
		(btc->chip->hw & BTC_FEAT_PTA_ONOFF_CTRL ? "HW" :
		(dm->pta_owner == BTC_CTRL_BY_WL? "WL" : "BT")),
		wl->pta_req_mac);

	gnt = &dm->gnt_val[HW_PHY_0];

	CLI_PRT(", phy-0[gnt_wl:%s-%d/gnt_bt:%s-%d/polut_type:%s]",
		(gnt->gnt_wl_sw_en? "SW" : "HW"), gnt->gnt_wl,
		(gnt->gnt_bt_sw_en? "SW" : "HW"), gnt->gnt_bt,
		id_to_str(BTC_STR_POLUT, wl->bt_polut_type[HW_PHY_0]));

	if (wl_rinfo->dbcc_en) {
		gnt = &dm->gnt_val[HW_PHY_1];
		CLI_PRT(", phy-1[gnt_wl:%s-%d/gnt_bt:%s-%d/polut_type:%s]",
			(gnt->gnt_wl_sw_en? "SW" : "HW"), gnt->gnt_wl,
			(gnt->gnt_bt_sw_en? "SW" : "HW"), gnt->gnt_bt,
			id_to_str(BTC_STR_POLUT, wl->bt_polut_type[HW_PHY_1]));
	}

	pcinfo = &pfwinfo->rpt_fbtc_mregval.cinfo;
	if (!pcinfo->valid)
		return;

	pmreg = &pfwinfo->rpt_fbtc_mregval.finfo;

	for (i = 0; i < pmreg->reg_num; i++) {
		type = (u8)btc->chip->mon_reg[i].type;
		offset = btc->chip->mon_reg[i].offset;
		val = pmreg->mreg_val[i];

		if (cnt % 6 == 0)
			CLI_PRT("\n\r %-15s : %s_0x%x=0x%x", "[reg]",
				id_to_str(BTC_STR_REG, (u32)type), offset, val);
		else
			CLI_PRT(", %s_0x%x=0x%x",
				id_to_str(BTC_STR_REG, (u32)type), offset, val);
		cnt++;
	}

	pcinfo = &pfwinfo->rpt_fbtc_gpio_dbg.cinfo;
	if (!pcinfo->valid)
		return;

	gdbg = &pfwinfo->rpt_fbtc_gpio_dbg.finfo;
	if (!gdbg->en_map)
		return;

	CLI_PRT("\n\r %-15s : enable_map:0x%08x", "[gpio_dbg]", gdbg->en_map);

	for (i = 0; i < BTC_DBG_MAX1; i++) {
		if (!(gdbg->en_map & BIT(i)))
			continue;
		CLI_PRT(", %s->GPIO%d",
			id_to_str(BTC_STR_GDBG, (u32)i), gdbg->gpio_map[i]);
	}
}

static void _show_summary(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			  u32 input_num, char *output, u32 out_len)
{
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct btc_rpt_cmn_info *pcinfo = NULL;
	struct fbtc_rpt_ctrl *prptctrl = NULL;
	struct btc_cx *cx = &btc->cx;
	struct btc_dm *dm = &btc->dm;
	struct btc_wl_info *wl = &cx->wl;
	u32 cnt_sum = 0;
	u32 *cnt = btc->dm.cnt_notify;
	u8 i;

	if (!(dm->coex_info_map & BTC_COEX_INFO_SUMMARY))
		return;

	CLI_PRT("%s", "\n\r========== [Statistics] ==========");

	pcinfo = &pfwinfo->rpt_ctrl.cinfo;
	if (pcinfo->valid && wl->status.map.lps != BTC_LPS_RF_OFF &&
	    !wl->status.map.rf_off) {
		prptctrl = &pfwinfo->rpt_ctrl.finfo;

		CLI_PRT("\n\r %-15s : h2c_cnt=%d(fail:%d, fw_recv:%d),"
			" c2h_cnt=%d(fw_send:%d, len:%d, max:%d), ",
			"[summary]", pfwinfo->cnt_h2c, pfwinfo->cnt_h2c_fail,
			prptctrl->rpt_info.cnt_h2c, pfwinfo->cnt_c2h,
			prptctrl->rpt_info.cnt_c2h, prptctrl->rpt_info.len_c2h,
			RTW_PHL_BTC_FWINFO_BUF);

		CLI_PRT("rpt_cnt=%d(fw_send:%d), rpt_map=0x%x",
			pfwinfo->event[BTF_EVNT_RPT], prptctrl->rpt_info.cnt,
			prptctrl->rpt_info.en);

		if (dm->error & BTC_DMERR_WL_FW_HANG)
			CLI_PRT(" (WL FW Hang!!)");

		CLI_PRT("\n\r %-15s : send_ok:%d, send_fail:%d, recv:%d, ",
			"[mailbox]", prptctrl->bt_mbx_info.cnt_send_ok,
			prptctrl->bt_mbx_info.cnt_send_fail,
			prptctrl->bt_mbx_info.cnt_recv);

		CLI_PRT("A2DP_empty:%d(stop:%d/tx:%d/ack:%d/nack:%d)",
			prptctrl->bt_mbx_info.a2dp.cnt_empty,
			prptctrl->bt_mbx_info.a2dp.cnt_flowctrl,
			prptctrl->bt_mbx_info.a2dp.cnt_tx,
			prptctrl->bt_mbx_info.a2dp.cnt_ack,
			prptctrl->bt_mbx_info.a2dp.cnt_nack);

		CLI_PRT("\n\r %-15s :"
			" wl_rfk[req:%d/go:%d/reject:%d/tout:%d/time = %dms]",
			"[RFK/LPS]", cx->cnt_wl[BTC_WCNT_RFK_REQ],
			cx->cnt_wl[BTC_WCNT_RFK_GO],
			cx->cnt_wl[BTC_WCNT_RFK_REJECT],
			cx->cnt_wl[BTC_WCNT_RFK_TIMEOUT],
			wl->rfk_info.proc_time);

		CLI_PRT(", bt_rfk[req:%d]", prptctrl->bt_cnt[BTC_BCNT_RFK_REQ]);

		CLI_PRT(", AOAC[RF_on:%d/RF_off:%d]",
			prptctrl->rpt_info.cnt_aoac_rf_on,
			prptctrl->rpt_info.cnt_aoac_rf_off);
	} else {
		CLI_PRT("\n\r %-15s : h2c_cnt=%d(fail:%d), c2h_cnt=%d"
			" (lps=%d/rf_off=%d)", "[summary]",
			pfwinfo->cnt_h2c, pfwinfo->cnt_h2c_fail,
			pfwinfo->cnt_c2h,
			wl->status.map.lps, wl->status.map.rf_off);
	}

	for (i = 0; i < BTC_NCNT_MAX; i++)
		cnt_sum += dm->cnt_notify[i];

	CLI_PRT("\n\r %-15s : total=%d, show_coex_info=%d,"
		" power_on=%d, init_coex=%d, ", "[notify_cnt]",
		cnt_sum, cnt[BTC_NCNT_SHOW_COEX_INFO],
		cnt[BTC_NCNT_POWER_ON], cnt[BTC_NCNT_INIT_COEX]);

	CLI_PRT("power_off=%d, radio_state=%d, role_info=%d,"
		" wl_rfk=%d, wl_sta=%d",
		cnt[BTC_NCNT_POWER_OFF], cnt[BTC_NCNT_RADIO_STATE],
		cnt[BTC_NCNT_ROLE_INFO], cnt[BTC_NCNT_WL_RFK],
		cnt[BTC_NCNT_WL_STA]);

	CLI_PRT("\n\r %-15s : scan_start=%d, scan_finish=%d,"
		" switch_band=%d, special_pkt=%d, ", "[notify_cnt]",
		cnt[BTC_NCNT_SCAN_START], cnt[BTC_NCNT_SCAN_FINISH],
		cnt[BTC_NCNT_SWITCH_BAND], cnt[BTC_NCNT_SPECIAL_PACKET]);

	CLI_PRT("timer=%d, customerize=%d, hub_msg=%d, chg_fw=%d",
		cnt[BTC_NCNT_TIMER], cnt[BTC_NCNT_CUSTOMERIZE],
		btc->hubmsg_cnt, cnt[BTC_NCNT_RESUME_DL_FW]);
}

static void _show_fbtc_slots(struct btc_t *btc, u32 *used,
			     char input[][MAX_ARGV], u32 input_num,
			     char *output, u32 out_len)
{
	struct btc_dm *dm = &btc->dm;
	struct fbtc_slot s;
	u8 i = 0;

	for (i = 0; i < CXST_MAX; i++) {
		s = dm->slot_now[i];
		if (i % 5 == 0)
			CLI_PRT("\n\r %-15s : %s[%d/%d/0x%x]", "[slot_list]",
				id_to_str(BTC_STR_SLOT, (u32)i),
				s.dur, s.cxtype, s.cxtbl);
		else
			CLI_PRT(", %s[%d/%d/0x%x]",
				id_to_str(BTC_STR_SLOT, (u32)i),
				s.dur, s.cxtype, s.cxtbl);
	}
}

static void _show_fbtc_cysta(struct btc_t *btc, u32 *used,
			     char input[][MAX_ARGV], u32 input_num,
			     char *output, u32 out_len)
{
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct btc_dm *dm = &btc->dm;
	struct btc_bt_info *bt = &btc->cx.bt;
	struct btc_bt_a2dp_desc *a2dp = &bt->link_info.a2dp_desc;
	struct btc_rpt_cmn_info *pcinfo = NULL;
	struct fbtc_cysta *pcysta = NULL;
	u8 i, cnt = 0, divide_cnt = 12;
	u8 slot_pair;
	u16 cycle, c_begin, c_end, s_id;

	pcinfo = &pfwinfo->rpt_fbtc_cysta.cinfo;
	if (!pcinfo->valid)
		return;

	pcysta = &pfwinfo->rpt_fbtc_cysta.finfo;
	CLI_PRT("\n\r %-15s : cycle:%d", "[slot_stat]", pcysta->cycles);

	for (i = 0; i < CXST_MAX; i++) {
		if (!pcysta->slot_cnt[i])
			continue;
		CLI_PRT(", %s:%d",
			id_to_str(BTC_STR_SLOT, (u32)i), pcysta->slot_cnt[i]);
	}

	if (dm->tdma_now.rxflctrl) {
		CLI_PRT(", leak_rx:%d", pcysta->leak_slot.cnt_rximr);
	}

	if (pcysta->collision_cnt) {
		CLI_PRT(", collision:%d", pcysta->collision_cnt);
	}

	if (pcysta->skip_cnt) {
		CLI_PRT(", skip:%d", pcysta->skip_cnt);
	}

	CLI_PRT("\n\r %-15s : avg_t[wl:%d/bt:%d/lk:%d.%03d]", "[cycle_stat]",
		pcysta->cycle_time.tavg[CXT_WL],
		pcysta->cycle_time.tavg[CXT_BT],
		pcysta->leak_slot.tavg/1000, pcysta->leak_slot.tavg%1000);
	CLI_PRT(", max_t[wl:%d/bt:%d(>%dms:%d)/lk:%d.%03d]",
		pcysta->cycle_time.tmax[CXT_WL],
		pcysta->cycle_time.tmax[CXT_BT],
		dm->bt_slot_flood, dm->cnt_dm[BTC_DCNT_BT_SLOT_FLOOD],
		pcysta->leak_slot.tamx/1000, pcysta->leak_slot.tamx%1000);
	CLI_PRT(", bcn[all:%d/ok:%d/in_bt:%d/in_bt_ok:%d]",
		pcysta->bcn_cnt[CXBCN_ALL], pcysta->bcn_cnt[CXBCN_ALL_OK],
		pcysta->bcn_cnt[CXBCN_BT_SLOT], pcysta->bcn_cnt[CXBCN_BT_OK]);

	if (a2dp->exist) {
		CLI_PRT("\n\r %-15s : a2dp_ept:%d, a2dp_late:%d"
			"(streak 2S:%d/max:%d)", "[a2dp_stat]",
			pcysta->a2dp_ept.cnt, pcysta->a2dp_ept.cnt_timeout,
			a2dp->no_empty_streak_2s, a2dp->no_empty_streak_max);

		CLI_PRT(", avg_t:%d, max_t:%d",
			pcysta->a2dp_ept.tavg, pcysta->a2dp_ept.tmax);
	}

	if (pcysta->cycles <= 1)
		return;

	/* 1 cycle = 1 wl-slot + 1 bt-slot */
	slot_pair = BTC_CYCLE_SLOT_MAX/2;

	if (pcysta->cycles <= slot_pair)
		c_begin = 1;
	else
		c_begin = pcysta->cycles - slot_pair + 1;

	c_end = pcysta->cycles;

	if (a2dp->exist)
		divide_cnt = 2;
	else
		divide_cnt = 6;

	if (c_begin > c_end)
		return;

	for (cycle = c_begin; cycle <= c_end; cycle++) {
		cnt++;
		s_id = ((cycle-1) % slot_pair)*2;

		if (cnt % divide_cnt == 1) {
			if (a2dp->exist)
				CLI_PRT("\n\r %-15s : ", "[slotT_wermtan]");
			else
				CLI_PRT("\n\r %-15s : ", "[slotT_rxerr]");
		}

		CLI_PRT("->b%d", pcysta->slot_step_time[s_id]);

		if (a2dp->exist)
			CLI_PRT("(%d/%d/%d/%dM/%d/%d/%d)",
				pcysta->wl_rx_err_ratio[s_id],
				pcysta->a2dp_trx[s_id].empty_cnt,
				pcysta->a2dp_trx[s_id].retry_cnt,
				(pcysta->a2dp_trx[s_id].tx_rate ? 3:2),
				pcysta->a2dp_trx[s_id].tx_cnt,
				pcysta->a2dp_trx[s_id].ack_cnt,
				pcysta->a2dp_trx[s_id].nack_cnt);
		else
			CLI_PRT("(%d)", pcysta->wl_rx_err_ratio[s_id]);

		CLI_PRT("->w%d", pcysta->slot_step_time[s_id+1]);

		if (a2dp->exist)
			CLI_PRT("(%d/%d/%d/%dM/%d/%d/%d)",
				pcysta->wl_rx_err_ratio[s_id+1],
				pcysta->a2dp_trx[s_id+1].empty_cnt,
				pcysta->a2dp_trx[s_id+1].retry_cnt,
				(pcysta->a2dp_trx[s_id+1].tx_rate ? 3:2),
				pcysta->a2dp_trx[s_id+1].tx_cnt,
				pcysta->a2dp_trx[s_id+1].ack_cnt,
				pcysta->a2dp_trx[s_id+1].nack_cnt);
		else
			CLI_PRT("(%d)", pcysta->wl_rx_err_ratio[s_id+1]);
	}
}

static void _show_fbtc_nullsta(struct btc_t *btc, u32 *used,
				char input[][MAX_ARGV], u32 input_num,
				char *output, u32 out_len)
{
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct btc_rpt_cmn_info *pcinfo = NULL;
	struct fbtc_cynullsta *ns = NULL;
	u8 i = 0;

	if (!btc->dm.tdma_now.rxflctrl)
		return;

	pcinfo = &pfwinfo->rpt_fbtc_nullsta.cinfo;
	if (!pcinfo->valid)
		return;

	ns = &pfwinfo->rpt_fbtc_nullsta.finfo;

	for (i = CXNULL_0; i <= CXNULL_1; i++) {
		CLI_PRT("\n\r %-15s : ", "[null_status]");
		CLI_PRT("null-%d", i);
		CLI_PRT("[Tx:%d/", ns->result[i][CXNULL_TX]);
		CLI_PRT("ok:%d/", ns->result[i][CXNULL_OK]);
		CLI_PRT("late:%d/", ns->result[i][CXNULL_LATE]);
		CLI_PRT("fail:%d/", ns->result[i][CXNULL_FAIL]);
		CLI_PRT("retry:%d/", ns->result[i][CXNULL_RETRY]);
		CLI_PRT("avg_t:%d.%03d/", ns->tavg[i]/1000, ns->tavg[i]%1000);
		CLI_PRT("max_t:%d.%03d]", ns->tmax[i]/1000, ns->tmax[i]%1000);
	}
}

static void _show_error(struct btc_t *btc, u32 *used,
			char input[][MAX_ARGV], u32 input_num,
			char *output, u32 out_len)
{
	struct btc_dm *dm = &btc->dm;
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	u32 fw_except_map = 0;
	u8 fw_except_cnt = 0;
	u8 valid = pfwinfo->rpt_fbtc_cysta.cinfo.valid;

	if (valid) {
		fw_except_map = pfwinfo->rpt_fbtc_cysta.finfo.except_map;
		fw_except_cnt = pfwinfo->rpt_fbtc_cysta.finfo.except_cnt;
	}

	if (!(dm->error || fw_except_map || fw_except_cnt ||
	    pfwinfo->len_mismch || pfwinfo->fver_mismch ||
	    pfwinfo->err[BTFRE_EXCEPTION]))
	    return;

	CLI_PRT("\n\r %-15s : dm_err:0x%x, fw_except:0x%x(cnt:%d), "
		"fw_rpt[len_err:0x%x/ver_err:0x%x/except_cnt:%d]", "[ERROR]",
		dm->error, fw_except_map, fw_except_cnt,
		pfwinfo->len_mismch, pfwinfo->fver_mismch,
		pfwinfo->err[BTFRE_EXCEPTION]);
}

#ifdef BTC_FW_STEP_DBG
static void _show_fbtc_step(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct btc_rpt_cmn_info *pcinfo = NULL;
	struct fbtc_steps *pstep = NULL;
	u8 type, val;
	u16 diff_t;
	u32 i, cnt = 0, n_begin = 0, n_end = 0, array_idx = 0;

	PHL_INFO("[BTC],%s():rpt_en_map=0x%x\n", __func__, pfwinfo->rpt_en_map);

	if ((pfwinfo->rpt_en_map & RPT_EN_FW_STEP_INFO) == 0)
		return;

	pcinfo = &pfwinfo->rpt_fbtc_step.cinfo;
	if (!pcinfo->valid)
		return;

	pstep = &pfwinfo->rpt_fbtc_step.finfo;

	if (pcinfo->req_fver != pstep->fver || !pstep->cnt)
		return;

	if (pstep->cnt <= FCXDEF_STEP)
		n_begin = 1;
	else
		n_begin = pstep->cnt - FCXDEF_STEP + 1;

	n_end = pstep->cnt;

	if (n_begin > n_end)
		return;

	/* restore step info by using ring instead of FIFO */
	for (i = n_begin; i <= n_end; i++) {
		array_idx = (i - 1) % FCXDEF_STEP;
		type = pstep->step[array_idx].type;
		val = pstep->step[array_idx].val;
		diff_t = (pstep->step[array_idx].difft_h8 << 8) +
			  pstep->step[array_idx].difft_l8;

		if (type == CXSTEP_NONE || type >= CXSTEP_MAX)
			continue;

		if (cnt % 10 == 0)
			CLI_PRT("\n\r %-15s : ", "[fw_steps]");

		CLI_PRT("-> %s(%d)",
			(type == CXSTEP_SLOT?
			id_to_str(BTC_STR_SLOT, (u32)val) :
			id_to_str(BTC_STR_EVENT, (u32)val)), diff_t);
		cnt++;
	}
}
#endif

static void _show_fw_dm_msg(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	if (!(btc->dm.coex_info_map & BTC_COEX_INFO_DM))
		return;

	_show_fbtc_slots(btc, used, input, input_num, output, out_len);
	_show_fbtc_cysta(btc, used, input, input_num, output, out_len);
	_show_fbtc_nullsta(btc, used, input, input_num, output, out_len);
#ifdef BTC_FW_STEP_DBG
	_show_fbtc_step(btc, used, input, input_num, output, out_len);
#endif
}

static void _cmd_show(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
		      u32 input_num, char *output, u32 out_len)
{
	u32 show_map = 0xff;

	if (input_num > 1)
		_os_sscanf(input[1], "%x", &show_map);

	btc->dm.coex_info_map = show_map & 0xff;

	_show_cx_info(btc, used, input, input_num, output, out_len);
	_show_wl_info(btc, used, input, input_num, output, out_len);
	_show_bt_info(btc, used, input, input_num, output, out_len);
	_show_dm_info(btc, used, input, input_num, output, out_len);
	_show_fw_dm_msg(btc, used, input, input_num, output, out_len);
	_show_error(btc, used, input, input_num, output, out_len);
	_show_mreg(btc, used, input, input_num, output, out_len);
	_show_summary(btc, used, input, input_num, output, out_len);
}

static void _cmd_dbg(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
		     u32 input_num, char *output, u32 out_len)
{
	struct btc_dm *dm = &btc->dm;
	struct btc_wl_info *wl = &btc->cx.wl;
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	u8 buf[9] = {0};
	u8 valid = pfwinfo->rpt_fbtc_cysta.cinfo.valid;
	u32 len = 0, n;
	u32 val = 0, type = 0, i, j;
	u32 fw_err_map = 0;

	/* input[0]->dbg, input[1]->type, input[2]->para1...
	 * buf[0] = type, buf[1] = para1, buf[2] = para2
	 */

	if (input_num <= 1) {
		CLI_PRT(" dbg <type> <para1:8bits> <para2:8bits>... \n");
		CLI_PRT(" dbg 0: H2C-C2H loopback <para1~8: loopback data>\n");
		CLI_PRT(" dbg 1: H2C-mailbox <para1~8: mailbox data>\n");
		CLI_PRT(" dbg 2: bt slot-request as NOA <para1: slot(hex)>\n");
		CLI_PRT(" dbg 3: enable fw_step debug <para1:8bits>\n");
		CLI_PRT(" dbg 4: bt flood-threshold setup <para1:8bits>\n");
		CLI_PRT(" dbg 5: dm error map decode\n");
		CLI_PRT(" dbg 6: fw exception map decode\n");
		CLI_PRT(" dbg 7: fw report length mismatch map decode\n");
		CLI_PRT(" dbg 8: fw report version mismatch map decode\n");
		CLI_PRT(" dbg 9: fddt no-run reason map decode\n");
		CLI_PRT(" dbg 10: MAC crc error report <para1: enable>\n");
		CLI_PRT(" dbg 11: BB btg_rx_ctrl <para1: enable>\n");
		CLI_PRT(" dbg 12: BB pre_agc_ctrl <para1: enable>\n");
		return;
	} else {
		/* dbg cmd type */
		_os_sscanf(input[1], "%d", &type);

		/* error map debug command */
		if ((type < 5 || type > 9) && input_num == 2) {
			CLI_PRT(" para1 not found!!\n");
			return;
		}
	}

	/* dbg cmd para1~para8(max) */
	for (n = 1; n <= input_num-1; n++) {
		_os_sscanf(input[n], "%x", &val);
		len++;
		if(len > sizeof(buf)) {
			CLI_PRT(" para length is too long!!\n");
			return;
		}
		buf[len-1] = (u8)(val & 0xff);
	}

	switch (type) {
	case 0: /* H2C-C2H loopback */
		if (len <= 1) {
			CLI_PRT(" no para found!!\n");
			return;
		}
		_send_fw_cmd(btc, SET_H2C_TEST, buf, (u16)len);
		btc->dbg.rb_done = false;
		for (i = 0; i < 50; i++) {
			if (!btc->dbg.rb_done) {
				hal_mdelay(btc->hal, 10);
				continue;
			}

			CLI_PRT(" H2C-C2H loopback data: [");
			for (j = 0; j < len - 1; j++) {
				CLI_PRT(" 0x%x", btc->dbg.lb_val[j]);
			}
			CLI_PRT("]\n");
			return;
		}
		CLI_PRT(" H2C-C2H loopback timeout!!\n");
		break;
	case 1: /* H2C-mailbox, To DO: FW send mailbox*/
		if (len != 9) {
			CLI_PRT(" para count must be 8!!\n");
			return;
		}
		_send_fw_cmd(btc, SET_H2C_TEST, buf, (u16)len);

		if (buf[1] != 0x33)
			return;

		/* mailbox 0x33 loopback process */
		btc->dbg.rb_done = false;
		for (i = 0; i < 50; i++) {
			if (!btc->dbg.rb_done) {
				hal_mdelay(btc->hal, 10);
				continue;
			}

			CLI_PRT(" Mailbox loopback data: [");
			for (j = 0; j < len - 1; j++) {
				CLI_PRT(" 0x%x", btc->dbg.lb_val[j]);
			}
			CLI_PRT("]\n");
			return;
		}
		CLI_PRT(" Mailbox loopback timeout !!\n");
		break;
	case 2: /* bt slot request */
		btc->bt_req_len[wl->pta_req_mac] = (u32)buf[1];
		_send_phl_evnt(btc, (u8*)&btc->bt_req_len[wl->pta_req_mac], 4,
			       BTC_HMSG_SET_BT_REQ_SLOT);
		CLI_PRT(" bt-slot-req: mac%d=%d\n", wl->pta_req_mac, buf[1]);
		break;
	case 3: /* fw_step debug */
#ifdef BTC_FW_STEP_DBG
		if (buf[1] > 0)
			_set_fw_rpt(btc, RPT_EN_FW_STEP_INFO, 1);
		else
			_set_fw_rpt(btc, RPT_EN_FW_STEP_INFO, 0);
#endif
		break;
	case 4: /* bt slot flood-threshold setup */
		dm->bt_slot_flood = (u16)buf[1];
		CLI_PRT(" bt-slot-flood= %dms!!\n", dm->bt_slot_flood);
		break;
	case 5: /* dm error map */
		if (!dm->error) {
			CLI_PRT(" no DM Error!!\n");
			return;
		}

		CLI_PRT(" DM Error includes:\n");
		for (i = 0; i < BTC_DMERR_MAX; i++) {
			if (!(btc->dm.error & BIT(i)))
				continue;
			CLI_PRT(" %s\n", id_to_str(BTC_STR_DMERROR, BIT(i)));
		}
		break;
	case 6: /* fw exception map */
		if (!valid) {
			CLI_PRT(" no FW exception report!!\n");
			return;
		}

		fw_err_map = pfwinfo->rpt_fbtc_cysta.finfo.except_map;

		if (!fw_err_map) {
			CLI_PRT(" no FW exception!\n");
			return;
		}

		CLI_PRT(" FW exception includes:\n");
		for (i = 0; i < BTC_FWERR_MAX; i++) {
			if (!(fw_err_map & BIT(i)))
				continue;
			CLI_PRT(" %s\n", id_to_str(BTC_STR_FWERROR, BIT(i)));

			if (i == 15)
				CLI_PRT("  --> H2C-Error cmd:SET_%s\n",
					id_to_str(BTC_STR_H2CERROR,
						  (fw_err_map & bMASKB3) >>24));
		}
		break;
	case 7: /* fw report length mismatch */
		if (!pfwinfo->len_mismch) {
			CLI_PRT(" no fw report length mismatch!!\n");
			return;
		}

		CLI_PRT(" fw report length mismatch includes:\n");
		for (i = 0; i < BTC_RPT_TYPE_MAX; i++) {
			if (!(pfwinfo->len_mismch & BIT(i)))
				continue;
			CLI_PRT(" %s\n", id_to_str(BTC_STR_RPTMATCH, i));
		}
		break;
	case 8: /* fw report version mismatch */
		if (!pfwinfo->fver_mismch) {
			CLI_PRT(" no fw report version mismatch!!\n");
			return;
		}

		CLI_PRT(" fw report verison mismatch includes:\n");
		for (i = 0; i < BTC_RPT_TYPE_MAX; i++) {
			if (!(pfwinfo->fver_mismch & BIT(i)))
				continue;
			CLI_PRT(" %s\n", id_to_str(BTC_STR_RPTMATCH, i));
		}
		break;
	case 9: /* fddt no-run reason */
		if (!dm->fddt_info.nrsn_map) {
			CLI_PRT(" no fddt no-run reason!!\n");
			return;
		}

		CLI_PRT(" fddt no-run reason includes:\n");
		for (i = 0; i < BTC_NFRSN_MAX; i++) {
			if (!(dm->fddt_info.nrsn_map & BIT(i)))
				continue;
			CLI_PRT(" %s\n", id_to_str(BTC_STR_FDDT_NORUN, i));
		}
		break;
	case 10:
		btc->dm.rx_err_rpt_en = (buf[1] == 0 ? 0 : 1);
		CLI_PRT("wl_rx_err_ratio enable = %d\n", buf[1]);
		break;
	case 11:
		dm->wl_btg_rx = (u32)buf[1];
		rtw_hal_bb_ctrl_btg(btc->hal, (bool)(!!dm->wl_btg_rx));
		break;
	case 12:
		dm->wl_pre_agc = buf[1];
		rtw_hal_bb_ctrl_btc_preagc(btc->hal, (bool)(!!dm->wl_pre_agc));
		break;
	}
}

static void _cmd_wb(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
		    u32 input_num, char *output, u32 out_len)
{
	u32 type = 0, addr = 0, val = 0;

	if (input_num >= 4) {
		_os_sscanf(input[1], "%d", &type);
		_os_sscanf(input[2], "%x", &addr);
		_os_sscanf(input[3], "%x", &val);
	}

	if (type > 4 || input_num < 4) {
		CLI_PRT(" wb <type --- 0:rf, 1:modem, 2:bluewize, 3:vendor,"
			" 4:LE> <addr:16bits> <val> \n");
		return;
	}

	_write_bt_reg(btc, (u8)type, (u16)addr, val);

	CLI_PRT(" wb type=%d, addr=0x%x, val=0x%x !! \n", type, addr, val);
}

static void _cmd_set_coex(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			  u32 input_num, char *output, u32 out_len)
{
	u32 mode = 0;

	if (input_num < 2) {
		CLI_PRT(" mode <val--- 0:original, 1:freeze coex,"
			" 2:always-freerun, 3:always-WLonly,"
			" 4:always-BTonly> \n");
		return;
	}

	_os_sscanf(input[1], "%d", &mode);

	switch(mode) {
	case 0: /* original */
		btc->ctrl.manual = 0;
		btc->ctrl.always_freerun = 0;
		btc->dm.wl_only = 0;
		btc->dm.bt_only = 0;
		CLI_PRT(" recovery original coex mechanism !! \n");
		break;
	case 1: /* freeze */
		btc->ctrl.manual = 1;
		CLI_PRT(" freeze coex mechanism !! \n");
		break;
	case 2: /* fix freerun */
		btc->ctrl.always_freerun = 1;
		CLI_PRT(" freerun coex mechanism !! \n");
		break;
	case 3: /* fix wl only */
		btc->dm.wl_only = 1;
		CLI_PRT(" always WL-only coex mechanism!! \n");
		break;
	case 4: /* fix bt only */
		btc->dm.bt_only = 1;
		CLI_PRT(" always BT-only coex mechanism!! \n");
		break;
	}

	_set_init_info(btc);
	_run_coex(btc, __func__);
}

static void _cmd_upd_policy(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	_update_poicy(btc, FC_EXEC, (u16)BTC_CXP_MANUAL<<8, __func__);

	CLI_PRT(" Update tdma/slot policy !! \n");
}

static void _cmd_tdma(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
		      u32 input_num, char *output, u32 out_len)
{
	u32 para1 = 0, para2 = 0;
	u8 buf[8] = {0};

	if (input_num >= 3) {
		_os_sscanf(input[1], "%x", &para1);
		_os_sscanf(input[2], "%x", &para2);

		buf[0] = (u8)((para1 & bMASKB3) >> 24);
		buf[1] = (u8)((para1 & bMASKB2) >> 16);
		buf[2] = (u8)((para1 & bMASKB1) >> 8);
		buf[3] = (u8)(para1 & bMASKB0);

		buf[4] = (u8)((para2 & bMASKB3) >> 24);
		buf[5] = (u8)((para2 & bMASKB2) >> 16);
		buf[6] = (u8)((para2 & bMASKB1) >> 8);
		buf[7] = (u8)(para2 & bMASKB0);

		if (buf[0] < CXTDMA_MAX) {
			_tdma_cpy(&btc->dm.tdma, buf);
			CLI_PRT("tdma-para1: type=%s,"
				" rxflctrl=0x%x, txflctrl=%d, rsvd=%d\n",
				id_to_str(BTC_STR_TDMA, (u32)buf[0]),
				buf[1], buf[2], buf[3]);

			CLI_PRT("tdma-para2: leak_n=%d,"
				" ext_ctrl=%x, rxflctrl_role=0x%x,"
				" option=0x%x\n",
				buf[4], buf[5], buf[6], buf[7]);
			return;
		}
	}

	CLI_PRT("tdma type error!! (>= %d)\n", CXTDMA_MAX);
	CLI_PRT(" tdma <para1:32bits> <para2:32bits>\n");
	CLI_PRT(" <para1>\n");
	CLI_PRT(" bit[31:24]= type 0:off, 1:fix, 2:auto, 3:auto2\n");
	CLI_PRT(" bit[23:16]= rx_flow_ctrl 0:off, 1:null, 2:Qos-null, 3:cts\n");
	CLI_PRT(" bit[15:8] = tx_flow_ctrl 0:off, 1:on\n");
	CLI_PRT(" bit[7:0]  = resevred\n");
	CLI_PRT(" <para2>\n");
	CLI_PRT(" bit[31:24]= leak_n enter leak slot every n*w2b-slot\n");
	CLI_PRT(" bit[23:16]= ext_ctrl 0:off, 1:bcn-early 2:ext\n");
	CLI_PRT(" bit[15:8] = rxflctrl_role\n");
	CLI_PRT(" bit[7:0]  = option\n");
}

static void _cmd_slot(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
		      u32 input_num, char *output, u32 out_len)
{
	u32 sid = 0, dur = 0, cx_tbl = 0, cx_type = 0, i;
	u8 buf[9] = {0};

	if (input_num >= 5) {
		_os_sscanf(input[1], "%d", &sid);
		_os_sscanf(input[2], "%d", &dur);
		_os_sscanf(input[3], "%d", &cx_type);
		_os_sscanf(input[4], "%x", &cx_tbl);

		buf[0] = (u8) sid;
		buf[1] = (u8)(dur & bMASKB0);
		buf[2] = (u8)((dur & bMASKB1) >> 8);

		buf[3] = (u8)(cx_type & bMASKB0);
		buf[4] = (u8)((cx_type & bMASKB1) >> 8);

		buf[5] = (u8)(cx_tbl & bMASKB0);
		buf[6] = (u8)((cx_tbl & bMASKB1) >> 8);
		buf[7] = (u8)((cx_tbl & bMASKB2) >> 16);
		buf[8] = (u8)((cx_tbl & bMASKB3) >> 24);

		if (buf[0] < CXST_MAX) {
			_slot_cpy(&btc->dm.slot[buf[0]], &buf[1]);
			CLI_PRT(" set slot para: slot:%s,"
				" duration:%d, type=%s, table:%08x!\n",
				id_to_str(BTC_STR_SLOT, sid), dur,
				(cx_type? "iso" : "mix"), cx_tbl);
			return;
		}
	}

	CLI_PRT("slot id error!! (>= %d)\n", CXST_MAX);
	CLI_PRT(" slot <slot_id:8bits> <slot_duration:16bits>"
		" <type:16bits> <coex_table:32bits>\n");

	CLI_PRT("      <slot_id> \n");

	for (i = 0; i < CXST_MAX; i++) {
		if ((i+1) % 5 == 1)
			CLI_PRT("		  %d: %s",
				i, id_to_str(BTC_STR_SLOT, i));
		else
			CLI_PRT(", %d: %s", i, id_to_str(BTC_STR_SLOT, i));

		if (((i+1) % 5 == 0) || (i == CXST_MAX - 1))
			CLI_PRT("\n");
	}

	CLI_PRT("      <slot_duration> unit: ms\n");
	CLI_PRT("      <type> BT packet at WL slot 0: allowed, 1: isolated\n");
	CLI_PRT("      <coex_table> 32bits coex table\n");
}


static void _cmd_sig_gdbg_en(struct btc_t *btc, u32 *used,
			     char input[][MAX_ARGV], u32 input_num,
			     char *output, u32 out_len)
{
	u32 map = 0, i;

	if (input_num >= 2) {
		_os_sscanf(input[1], "%x", &map);

		hal_btc_fw_set_gpio_dbg(btc, CXDGPIO_EN_MAP, map);

		CLI_PRT(" signal to gpio debug map = 0x%08x!!\n", map);
		return;
	}

	CLI_PRT(" sig <enable_map:hex/32bits> ");
	CLI_PRT("(Default Signal->GPIO: No-Assign->0, GNT_WL->2, GNT_BT->3, ");
	CLI_PRT("WL_CCA->1, WL_ERR->6, WL_OK->7)\n");

	for (i = 0; i < BTC_DBG_MAX1; i++) {
		if ((i+1) % 8 == 1)
			CLI_PRT("      %d: %s", i, id_to_str(BTC_STR_GDBG, i));
		else
			CLI_PRT(", %d: %s", i, id_to_str(BTC_STR_GDBG, i));

		if (((i+1) % 8 == 0) || (i == BTC_DBG_MAX1 - 1))
			CLI_PRT("\n");
	}
}

static void _cmd_sgpio_map(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			   u32 input_num, char *output, u32 out_len)
{
	u32 sig = 0, gpio = 0;

	if (input_num >= 3) {
		_os_sscanf(input[1], "%d", &sig);
		_os_sscanf(input[2], "%d", &gpio);
	}

	if ((sig >= BTC_DBG_MAX1 || gpio > 15) || input_num < 3) {
		CLI_PRT(" gpio <sig:0~31> <gpio:0~15>\n");
		return;
	}

	hal_btc_fw_set_gpio_dbg(btc, CXDGPIO_MUX_MAP, ((gpio << 8) + sig));

	CLI_PRT(" signal-%s -> gpio-%d\n", id_to_str(BTC_STR_GDBG, sig), gpio);
}

static void _cmd_wl_tx_power(struct btc_t *btc, u32 *used,
			     char input[][MAX_ARGV], u32 input_num,
			     char *output, u32 out_len)
{
	u32 pwr = 0;
	u8 is_negative = 0;

	if (input_num >= 2) {
		_os_sscanf(input[1], "%d", &pwr);
		if (pwr & BIT(31)) {
			pwr = ~pwr + 1;
			is_negative = 1;
		}
		pwr = pwr & bMASKB0;
	}

	if ((pwr != 255 && pwr > 20) || input_num < 2) {
		CLI_PRT(" wpwr <wl_tx_power(dBm): -20dbm ~ +20dBm,"
			" 255-> original tx power\n");
		return;
	}

	if (is_negative)
		pwr |= BIT(7);

	_set_wl_tx_power(btc, pwr); /* pwr --> 1's complement */

	if (pwr == 0xff)
		CLI_PRT(" set wl tx power level to original!!\n");
	else if (is_negative)
		CLI_PRT(" set wl tx power level = -%d dBm!!\n", pwr & 0x7f);
	else
		CLI_PRT(" set wl tx power level = +%d dBm!!\n", pwr & 0x7f);
}

static void _cmd_wl_rx_lna(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			   u32 input_num, char *output, u32 out_len)
{
	u32 lna = 0;

	if (input_num < 2) {
		CLI_PRT(" wlna <wl_rx_level: 0~7> \n");
		return;
	}

	_os_sscanf(input[1], "%d", &lna);

	_set_wl_rx_gain(btc, lna);
	CLI_PRT(" set wl rx level = %d!!\n", lna);
}

static void _cmd_bt_afh_map(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	struct btc_wl_afh_info *wl_afh = &btc->cx.wl.afh_info;
	u32 en = 0, ch = 0, bw = 0;
	u8 buf[3] = {0};

	if (input_num < 4) {
		CLI_PRT(" afh <en: 0/1> <ch: 0~255> <bw: 0~255>\n");
		return;
	}

	_os_sscanf(input[1], "%d", &en);
	_os_sscanf(input[2], "%d", &ch);
	_os_sscanf(input[3], "%d", &bw);

	buf[0] = (u8)(en & 0xff);
	buf[1] = (u8)(ch & 0xff);
	buf[2] = (u8)(bw & 0xff);

	if (_send_fw_cmd(btc, SET_BT_WL_CH_INFO, buf, 3)) {
		wl_afh->en = buf[0];
		wl_afh->ch = buf[1];
		wl_afh->bw = buf[2];
		CLI_PRT(" set bt afh map: en=%d, ch=%d, map=%d\n", en, ch, bw);
	} else {
		CLI_PRT(" set bt afh map fail!!");
	}
}

static void _cmd_bt_tx_power(struct btc_t *btc, u32 *used,
			     char input[][MAX_ARGV], u32 input_num,
			     char *output, u32 out_len)
{
	s32 pwr = 0;
	u32 pwr2;

	if (input_num < 2) {
		CLI_PRT(" bpwr <decrease power: 0~255 > \n");
		return;
	}

	_os_sscanf(input[1], "%d", &pwr);

	pwr2 = pwr & bMASKB0;

	_set_bt_tx_power(btc, pwr2);
	CLI_PRT(" decrease bt tx power level = %d!!\n", pwr2);
}

static void _cmd_bt_rx_lna(struct btc_t *btc, u32 *used, char input[][MAX_ARGV],
			   u32 input_num, char *output, u32 out_len)
{
	struct btc_wl_info *wl = &btc->cx.wl;
	struct btc_bt_info *bt = &btc->cx.bt;
	u32 lna = 0;
	u8 buf;

	if (input_num >= 2)
		_os_sscanf(input[1], "%d", &lna);

	if (input_num < 2 || lna > 7) {
		CLI_PRT(" blna <lna_constrain: 0~7> \n");
		return;
	}

	buf = (u8)(lna & bMASKB0);

	if (_send_fw_cmd(btc, SET_BT_LNA_CONSTRAIN, &buf, 1)) {
		CLI_PRT(" set bt rx lna constrain level fail!!\n");
		return;
	}

	bt->rf_para.rx_gain_freerun = lna;
	btc->dm.rf_trx_para.bt_rx_gain = lna;

	_os_delay_us(btc->hal, BTC_SCBD_REWRITE_DELAY*2);

	if (buf == BTC_BT_RX_NORMAL_LVL)
		_write_scbd(btc, BTC_WSCB_RXGAIN, false);
	else
		_write_scbd(btc, BTC_WSCB_RXGAIN, true);

	rtw_hal_mac_set_scoreboard(btc->hal, &wl->scbd);

	CLI_PRT(" set bt rx lna constrain level = %d!!\n", buf);
}

static void _cmd_bt_igno_wl(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	u32 igno = 0;

	if (input_num < 2) {
		CLI_PRT(" igwl <0: don't ignore wlan, 1: ignore wlan > \n");
		return;
	}

	_os_sscanf(input[1], "%d", &igno);

	igno = igno & BIT(0);

	_set_bt_ignore_wl_act(btc, (u8)igno);
	CLI_PRT(" set bt ignore wlan = %d!!\n", igno);
}

static void _cmd_set_gnt_wl(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	u32 gwl = 0, phy_map = BTC_PHY_ALL, val = 0;

	if (input_num != 2 && input_num != 3) {
		CLI_PRT(" gwl <0:SW_0, 1:SW_1, 2:HW_PTA>"
			" <b0->0:PHY-0/1:PHY-1, b1->0:bt0/1:bt1, none:All> \n");
		return;
	}

	_os_sscanf(input[1], "%d", &gwl);

	btc->dm.bt_select = BTC_ALL_BT;
	if (input_num == 3) {
		_os_sscanf(input[2], "%d", &val);
		phy_map = ((val & BIT(0))? BTC_PHY_1 : BTC_PHY_0);
		btc->dm.bt_select = ((val & BIT(1))? BTC_BT_1 : BTC_BT_0);
	}

	switch(gwl) {
	case 0:
		_set_gnt(btc, (u8)phy_map, BTC_GNT_SW_LO, BTC_GNT_SET_SKIP,
			 BTC_WLACT_SW_LO);
		CLI_PRT(" set gnt_wl = SW-0");
		break;
	case 1:
		_set_gnt(btc, (u8)phy_map, BTC_GNT_SW_HI, BTC_GNT_SET_SKIP,
			 BTC_WLACT_SW_HI);
		CLI_PRT(" set gnt_wl = SW-1");
		break;
	case 2:
		_set_gnt(btc, (u8)phy_map, BTC_GNT_HW, BTC_GNT_SET_SKIP,
			 BTC_WLACT_HW);
		CLI_PRT(" set gnt_wl = HW-PTA ctrl");
		break;
	}

	CLI_PRT(" (phy_map=0x%x, bt_sel=0x%x)\n", phy_map, btc->dm.bt_select+1);
}

static void _cmd_set_gnt_bt(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	u32 gbt = 0, phy_map = BTC_PHY_ALL, val = 0;

	if (input_num != 2 && input_num != 3) {
		CLI_PRT(" gbt <0:SW_0, 1:SW_1, 2:HW_PTA>"
			" <b0->0:PHY-0/1:PHY-1, b1->0:bt0/1:bt1, none:All> \n");
		return;
	}

	_os_sscanf(input[1], "%d", &gbt);

	btc->dm.bt_select = BTC_ALL_BT;
	if (input_num == 3) {
		_os_sscanf(input[2], "%d", &val);
		phy_map = ((val & BIT(0))? BTC_PHY_1 : BTC_PHY_0);
		btc->dm.bt_select = ((val & BIT(1))? BTC_BT_1 : BTC_BT_0);
	}

	switch(gbt) {
	case 0:
		_set_gnt(btc, (u8)phy_map, BTC_GNT_SET_SKIP, BTC_GNT_SW_LO,
			 BTC_WLACT_SW_HI);
		CLI_PRT(" set gnt_bt = SW-0");
		break;
	case 1:
		_set_gnt(btc, (u8)phy_map, BTC_GNT_SET_SKIP, BTC_GNT_SW_HI,
			 BTC_WLACT_SW_LO);
		CLI_PRT(" set gnt_bt = SW-1");
		break;
	case 2:
		_set_gnt(btc, (u8)phy_map, BTC_GNT_SET_SKIP, BTC_GNT_HW,
			 BTC_WLACT_HW);
		CLI_PRT(" set gnt_bt = HW-PTA ctrl");
		break;
	}

	CLI_PRT(" (phy_map=0x%x, bt_sel=0x%x)\n", phy_map, btc->dm.bt_select+1);
}

static void _cmd_set_bt_psd(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	u32 idx = 0, type = 0;

	if (input_num < 3) {
		CLI_PRT(" bpsd <start_idex, dec-8bit>"
			"<report_type, dec-8bit> \n");
		return;
	}

	_os_sscanf(input[1], "%d", &idx);
	_os_sscanf(input[2], "%d", &type);

	_bt_psd_setup(btc, (u8)idx, (u8)type);
}

s8 _get_bw_att_db(struct btc_t *btc)
{
	struct btc_wl_info *wl = &btc->cx.wl;
	s8 bw_att_db = 13;

	if (wl->afh_info.bw >= CHANNEL_WIDTH_5)
		return bw_att_db;

	if (wl->afh_info.bw != CHANNEL_WIDTH_80_80)
		bw_att_db += 3 * (s8)(wl->afh_info.bw - CHANNEL_WIDTH_20);
	else
		bw_att_db += 3 * 3;

	return bw_att_db;
}

void _get_wl_nhm_dbm(struct btc_t *btc)
{
	struct btc_wl_nhm *wl_nhm = &btc->cx.wl.nhm;
	s8 bw_att_db = 0, nhm_pwr_dbm = 0, pwr = 0, cur_pwr = 0;
	u16 save_index = 0;
	bool new_data_flag = false;
	struct watchdog_nhm_report nhm_rpt = {0};

	bw_att_db = _get_bw_att_db(btc);

	rtw_hal_bb_nhm_mntr_result(btc->hal, &nhm_rpt, HW_PHY_0);

	if (!nhm_rpt.ccx_rpt_result)
		return;

	nhm_pwr_dbm = nhm_rpt.nhm_pwr_dbm;

	wl_nhm->instant_wl_nhm_dbm = nhm_pwr_dbm;

	if (wl_nhm->start_flag == false) {
		wl_nhm->start_flag = true;
		pwr = nhm_pwr_dbm - bw_att_db;
		new_data_flag = true;
	} else {
		pwr = wl_nhm->pwr;
		if (wl_nhm->last_ccx_rpt_stamp != nhm_rpt.ccx_rpt_stamp) {
			new_data_flag = true;
			wl_nhm->current_status = 1; // new data
		} else {
			wl_nhm->current_status = 2; //duplicated data
		}
	}

	if (!new_data_flag)
		return;

	wl_nhm->last_ccx_rpt_stamp = nhm_rpt.ccx_rpt_stamp;
	cur_pwr = nhm_pwr_dbm - bw_att_db;
	wl_nhm->instant_wl_nhm_per_mhz = cur_pwr;

	/*  reset max/min by 100 record time */
	if (wl_nhm->valid_record_times % 100 == 0) {
		wl_nhm->pwr_max = -127;
		wl_nhm->pwr_min = 0;
	}

	wl_nhm->valid_record_times += 1;

	if (wl_nhm->valid_record_times == 0) {
		wl_nhm->valid_record_times = 16;
	}

	wl_nhm->ratio = nhm_rpt.nhm_ratio;

	if (cur_pwr < pwr) {
		pwr = cur_pwr;
		wl_nhm->refresh = true;
	} else {
		pwr = (pwr >> 1) + (cur_pwr >> 1);
		wl_nhm->refresh = false;
	}

	save_index = ((wl_nhm->valid_record_times + 16 - 1) % 16) & 0xf;

	wl_nhm->pwr = pwr;
	wl_nhm->record_pwr[save_index] = pwr;
	wl_nhm->record_ratio[save_index] = nhm_rpt.nhm_ratio;

	if (wl_nhm->pwr > wl_nhm->pwr_max)
		wl_nhm->pwr_max = wl_nhm->pwr;

	if (wl_nhm->pwr < wl_nhm->pwr_min)
		wl_nhm->pwr_min = wl_nhm->pwr;
}

static void _cmd_get_wl_nhm(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	struct btc_wl_info *wl = &btc->cx.wl;
	struct btc_wl_nhm *wl_nhm = &wl->nhm;
	u16 start_index = 0, stop_index = 0, i;
	u32 show_record_num = 16;

	if (input_num >= 3) {
		CLI_PRT(" wnhm <last_record_num, dec,1-16, default:16> \n");
		return;
	}

	_get_wl_nhm_dbm(btc);

	_os_sscanf(input[1], "%d", &show_record_num);

	if (show_record_num >= 16)
		show_record_num = 16;
	else if (show_record_num == 0)
		show_record_num = 1;

	if (wl_nhm->valid_record_times == 0) {
		CLI_PRT("wl nhm not ready\n");
		return;
	}

	if (wl_nhm->current_status == 0) {
		CLI_PRT("wl nhm failed this time\n");
	}

	CLI_PRT("nhm_psd = %d dBm/MHz, nhm_ratio = %d, status = %d, "
		"valid_record_times = %d\n",
		wl_nhm->pwr, wl_nhm->ratio,
		wl_nhm->current_status, wl_nhm->valid_record_times);

	stop_index = wl_nhm->valid_record_times;
	if (wl_nhm->valid_record_times > (u16)show_record_num)
		start_index = wl_nhm->valid_record_times - (u16)show_record_num;

	for (i = start_index; i < stop_index; i++) {
		if (i == start_index) {
			CLI_PRT("record_pwr(old->new)= %4d",
				wl_nhm->record_pwr[(i + 16) & 0xF]);
		} else if (i == (stop_index - 1)) {
			CLI_PRT(", %4d dBm/MHz\n",
				wl_nhm->record_pwr[(i + 16) & 0xF]);
		} else {
			CLI_PRT(", %4d", wl_nhm->record_pwr[(i + 16) & 0xF]);
		}
	}

	for (i = start_index; i < stop_index; i++) {
		if (i == start_index) {
			CLI_PRT("record_ratio(old->new)= %4d",
				wl_nhm->record_ratio[(i + 16) & 0xF]);
		} else if (i == (stop_index - 1)) {
			CLI_PRT(", %4d percent\n",
				wl_nhm->record_ratio[(i + 16) & 0xF]);
		} else {
			CLI_PRT(", %4d", wl_nhm->record_ratio[(i + 16) & 0xF]);
		}
	}
}

void _get_wl_cn_report(struct btc_t *btc)
{
	struct btc_wl_info *wl = &btc->cx.wl;
	wl->cn_report = rtw_hal_ex_cn_report(btc->hal);
}


void _get_wl_evm_report(struct btc_t *btc)
{
	struct btc_wl_info *wl = &btc->cx.wl;
	wl->evm_1ss_rpt = rtw_hal_ex_evm_1ss_report(btc->hal);
	wl->evm_2ss_max_rpt = rtw_hal_ex_evm_max_report(btc->hal);
	wl->evm_2ss_min_rpt = rtw_hal_ex_evm_min_report(btc->hal);
}

#ifdef BTC_FDDT_TRAIN_SUPPORT
static void _cmd_fddt_cell(struct btc_t *btc, u32 *used,
			   char input[][MAX_ARGV], u32 type,
			   char *output, u32 out_len)
{
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct btc_rpt_cmn_info *pcinfo = NULL;
	struct fbtc_fddt_sta *pfddt = NULL;
	u8 i, j, r_lvl, m_lvl = BTC_BT_RSSI_THMAX;
	s8 rssi_th, rssi_th2, para1, para2, para3, para4;
	struct btc_fddt_train_info *train_now = &btc->dm.fddt_info.train_now;
	struct btc_fddt_cell cell[5][5];
	struct fbtc_fddt_cell_status cell_c2h[5][5];

	if (type == BTC_FDDT_CELL_STATE_UL || type == BTC_FDDT_CELL_STATE_DL) {
		pcinfo = &pfwinfo->rpt_fbtc_fddt.cinfo;
		if (!pcinfo->valid)
			return;

		pfddt = &pfwinfo->rpt_fbtc_fddt.finfo;
	}

	CLI_PRT("\n============ ");

	switch (type) {
	case BTC_FDDT_CELL_RANGE_UL:
		CLI_PRT("Cell-Range-UL (WL_Tx_min/WL_Tx_max/BT_Tx_min/BT_Rx)");
		hal_mem_cpy(btc->hal, cell, train_now->cell_ul, sizeof(cell));
		break;
	case BTC_FDDT_CELL_STATE_UL:
		CLI_PRT("Cell-State-UL (WL_Tx/BT_Tx_Dec/BT_Rx/Cell_Status)");
		hal_mem_cpy(btc->hal, cell_c2h, pfddt->fddt_cells[0],
			    sizeof(cell_c2h));
		break;
	case BTC_FDDT_CELL_STATE_DL:
		CLI_PRT("Cell-State-DL (WL_Tx/BT_Tx_Dec/BT_Rx/Cell_Status)");
		hal_mem_cpy(btc->hal, cell_c2h, pfddt->fddt_cells[1],
			    sizeof(cell_c2h));
		break;
	default:
		CLI_PRT("Cell-Range-DL (WL_Tx_min/WL_Tx_max/BT_Tx_min/BT_Rx)");
		hal_mem_cpy(btc->hal, cell, train_now->cell_dl, sizeof(cell));
		break;
	}

	CLI_PRT(" ============\nWL_RSSI		 > %3d    ",
		btc->chip->wl_rssi_thres[0] - 110);

	for (i = 0; i < m_lvl; i++) {
		rssi_th = btc->chip->wl_rssi_thres[i] - 110;
		if (i == m_lvl - 1) {
			CLI_PRT("   < %3d   \n", rssi_th);
		} else {
			rssi_th2 = btc->chip->wl_rssi_thres[i+1] - 110;
			CLI_PRT(" %3d ~ %3d  ", rssi_th, rssi_th2);
		}
	}

	CLI_PRT("BT_RSSI > %d:    ", btc->chip->bt_rssi_thres[0] - 100);

	for (i = 0; i <= m_lvl; i++) {
		if (type == BTC_FDDT_CELL_STATE_UL ||
		    type == BTC_FDDT_CELL_STATE_DL) {
			para1 = cell_c2h[m_lvl][m_lvl-i].wl_tx_pwr;
			para2 = cell_c2h[m_lvl][m_lvl-i].bt_tx_pwr;
			para3 = cell_c2h[m_lvl][m_lvl-i].bt_rx_gain;
			para4 = cell_c2h[m_lvl][m_lvl-i].state_phase;
		} else {
			para1 = cell[m_lvl][m_lvl-i].wl_pwr_min;
			para2 = cell[m_lvl][m_lvl-i].wl_pwr_max;
			para3 = cell[m_lvl][m_lvl-i].bt_pwr_dec_max;
			para4 = cell[m_lvl][m_lvl-i].bt_rx_gain;
		}

		CLI_PRT(" %2d/%2d/%2d/%2x", para1, para2, para3, para4);
	}

	CLI_PRT("\n");

	for (j = 0; j < m_lvl; j++) {
		rssi_th = btc->chip->bt_rssi_thres[j] - 100;
		r_lvl = m_lvl - j - 1;

		if (j == m_lvl - 1) {
			CLI_PRT("BT_RSSI < %d:    ", rssi_th);
		} else {
			rssi_th2 = btc->chip->bt_rssi_thres[j+1] - 100;
			CLI_PRT("BT_RSSI %d ~ %d:", rssi_th, rssi_th2);
		}

		for (i = 0; i <= 4; i++) {
			if (type == BTC_FDDT_CELL_STATE_UL ||
			    type == BTC_FDDT_CELL_STATE_DL) {
				para1 = cell_c2h[r_lvl][m_lvl-i].wl_tx_pwr;
				para2 = cell_c2h[r_lvl][m_lvl-i].bt_tx_pwr;
				para3 = cell_c2h[r_lvl][m_lvl-i].bt_rx_gain;
				para4 = cell_c2h[r_lvl][m_lvl-i].state_phase;
			} else {
				para1 = cell[r_lvl][m_lvl-i].wl_pwr_min;
				para2 = cell[r_lvl][m_lvl-i].wl_pwr_max;
				para3 = cell[r_lvl][m_lvl-i].bt_pwr_dec_max;
				para4 = cell[r_lvl][m_lvl-i].bt_rx_gain;
			}

			CLI_PRT(" %2d/%2d/%2d/%2x", para1, para2, para3, para4);
		}

		CLI_PRT("\n");
	}
}

static void _show_fddt_cycle(struct btc_t *btc, u32 *used,
			     char input[][MAX_ARGV], u32 input_num,
			     char *output, u32 out_len)
{
	struct btc_wl_info *wl = &btc->cx.wl;
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct btc_rpt_cmn_info *pcinfo = NULL;
	struct fbtc_fddt_sta *pfddt = NULL;
	struct fbtc_fddt_cycle_info c;
	u8 cnt = 0;
	u16 cycle, c_begin, c_end, s_id, cycle_total;

	pcinfo = &pfwinfo->rpt_fbtc_fddt.cinfo;
	if (!pcinfo->valid)
		return;

	pfddt = &pfwinfo->rpt_fbtc_fddt.finfo;
	cycle_total = (pfddt->cycles_h8 << 8) + pfddt->cycles_l8;

	if (cycle_total <= 1)
		return;

	if (cycle_total <= BTC_CYCLE_SLOT_MAX)
		c_begin = 1;
	else
		c_begin = cycle_total - BTC_CYCLE_SLOT_MAX + 1;

	c_end = cycle_total;

	if (c_begin > c_end)
		return;

	CLI_PRT("\n============ Cycle ([RSSI]-[Train]"
		 "-[TRx_Para]-[mW_KPI]-[mB_KPI])============\n");

	for (cycle = c_begin; cycle <= c_end; cycle++) {
		cnt++;
		s_id = (cycle-1) % BTC_CYCLE_SLOT_MAX;
		c = pfddt->fddt_trx[s_id];

		CLI_PRT("No.%d", cycle);

		CLI_PRT("([W%d/B%d/CN%d]-[Cy%d/St%d->S%d/P%d/R0x%x]-"
			"[WT%d/WR%d/BT%d/BR%d]-[TP:%d]-[ne:%d])-->",
			c.rssi & 0xf, c.rssi & 0xf0) >> 4, c.cn,
			c.phase_cycle, c.train_step,
			(c.train_status & 0xf0) >> 4, c.train_status & 0xf,
			c.train_result,
			c.tx_power, wl->rf_para.rx_gain_freerun,
			c.bt_tx_power, c.bt_rx_gain,
			(c.tp_h8 << 8) + c.tp_l8,
			c.no_empty_cnt);

		if (cnt % 2 == 0)
			CLI_PRT("\n");
	}
}

static void _cmd_fddt_show(struct btc_t *btc, u32 *used,
			   char input[][MAX_ARGV], u32 input_num,
			   char *output, u32 out_len)
{
	struct btc_dm *dm = &btc->dm;
	struct btc_cx *cx = &btc->cx;
	struct btc_wl_info *wl = &cx->wl;
	struct btc_bt_info *bt = &cx->bt;
	struct btc_module *module = &btc->mdinfo;
	struct btc_wl_role_info *wl_rinfo = &wl->role_info;
	struct btc_bt_link_info *bt_linfo = &bt->link_info;
	struct btc_wl_link_info *plink = &wl->link_info[0];
	struct btc_wl_smap *wl_smap = &wl->status.map;
	struct fbtc_rpt_ctrl *finfo = &btc->fwinfo.rpt_ctrl.finfo;
	struct btc_rpt_ctrl_a2dp_empty *a2dp = &finfo->bt_mbx_info.a2dp;
	struct btc_fddt_info *fdinfo = &dm->fddt_info;
	struct btc_fddt_train_info *train_now = &fdinfo->train_now;
	struct btc_rpt_ctrl_a2dp_empty *a2dp_last = &fdinfo->bt_stat.a2dp_last;
	u32 ratio_ack = 0, ratio_nack = 0, show_map = BTC_FDDT_INFO_MAP_DEF;
	u32 n;

	if (input_num > 1)
		_os_sscanf(input[1], "%x", &show_map);

	if (show_map & BIT(0)) {
		CLI_PRT("============ %s()  ============\n", __func__);

		CLI_PRT("enable=%d, count=%d, type=%s, state=%s, result=%d,"
			" rsn_map=0x%x, tdma_policy=%s, aiso_sort_avg=%d\n",
			dm->fddt_train, dm->cnt_dm[BTC_DCNT_FDDT_TRIG],
			id_to_str(BTC_STR_FDDT_TYPE, (u32)fdinfo->type),
			id_to_str(BTC_STR_FDDT_STATE, (u32)fdinfo->state),
			fdinfo->result, fdinfo->nrsn_map,
			id_to_str(BTC_STR_POLICY, (u32)btc->policy_type),
			module->ant.isolation);

		CLI_PRT("[Time_Ctrl] m_cycle=%d, w_cycle=%d, k_cycle=%d\n",
			train_now->t_ctrl.m_cycle,
			train_now->t_ctrl.w_cycle,
			train_now->t_ctrl.k_cycle);

		CLI_PRT("[Pass_Ctrl] map=0x%x, no_empty_cnt<%d, tp_ratio=%d, "
			"kpibtr_ratio=%d\n",
			train_now->f_chk.check_map,
			train_now->f_chk.bt_no_empty_cnt,
			train_now->f_chk.wl_tp_ratio,
			train_now->f_chk.wl_kpibtr_ratio);

		CLI_PRT("[Break_Ctrl] map=0x%x, no_empty_cnt>=%d, tp_ratio=%d,",
			train_now->b_chk.check_map,
			train_now->b_chk.bt_no_empty_cnt,
			train_now->b_chk.wl_tp_ratio);

		CLI_PRT(" tp_low=%d, cn=%d, cell_chg=%d, cn_limit=%d, "
			"nhm_limit=%d\n",
			train_now->b_chk.wl_tp_low_bound,
			train_now->b_chk.cn,
			train_now->b_chk.cell_chg,
			train_now->b_chk.cn_limit,
			train_now->b_chk.nhm_limit);
	}

	if (show_map & BIT(1))
		_cmd_fddt_cell(btc, used, input, wl_smap->traffic_dir,
			       output, out_len);

	if (show_map & BIT(2)) {
		CLI_PRT("\n============ WL ============\n");

		CLI_PRT("link_mode=%s, rssi=-%ddBm(%d, level=%d), busy=%d,"
			" nhm=%d, cn=%d\n",
			id_to_str(BTC_STR_WLLINK, (u32)wl_rinfo->link_mode),
			110-plink->rssi, plink->rssi, wl->rssi_level,
			wl_smap->busy, wl->nhm.pwr, wl->cn_report);

		CLI_PRT("evm(1ss:%d, 2ss_max:%d, 2ss_min:%d)\n",
			wl->evm_1ss_rpt,
			wl->evm_2ss_max_rpt, wl->evm_2ss_min_rpt);

		CLI_PRT("dir=%s, tx[tp:%d Mbps/byte:%d/rate:%s/busy_level:%d],"
			" rx[tp:%d Mbps/byte:%d/rate:%s/busy_level:%d]\n",
			(wl_smap->traffic_dir & BIT(TRAFFIC_UL) ? "UL" : "DL"),
			wl->traffic.tx_tp, (u32)wl->traffic.tx_byte,
			id_to_str(BTC_STR_RATE, (u32)plink->tx_rate),
			wl->traffic.tx_lvl,
			wl->traffic.rx_tp, (u32)wl->traffic.rx_byte,
			id_to_str(BTC_STR_RATE, (u32)plink->rx_rate),
			wl->traffic.rx_lvl);
	}

	if (show_map & BIT(3)) {
		CLI_PRT("\n============ BT ============\n");

		CLI_PRT("state=0x%x, rssi=%ddBm(%d, level=%d), role:%s,"
			" tx_rate:%dM, rate_chg:%d\n",
			bt->raw_info[2], bt_linfo->rssi-100,
			bt_linfo->rssi, bt->rssi_level,
			(bt_linfo->slave_role ? "Slave" : "Master"),
			(bt_linfo->tx_3M? 3 : 2),
			cx->cnt_bt[BTC_BCNT_RATECHG]);

		if (a2dp->cnt_tx != 0) {
			ratio_ack = a2dp->cnt_ack * 100 / a2dp->cnt_tx;
			ratio_nack = a2dp->cnt_nack * 100 / a2dp->cnt_tx;
		}

		CLI_PRT("Retry:%d, A2DP_empty:%d"
			"[stop:%d/tx:%d/ack:%d(%d)/nack:%d(%d)]\n",
			cx->cnt_bt[BTC_BCNT_RETRY],
			a2dp->cnt_empty, a2dp->cnt_flowctrl, a2dp->cnt_tx,
			a2dp->cnt_ack, ratio_ack, a2dp->cnt_nack, ratio_nack);

		n = a2dp->cnt_tx - a2dp_last->cnt_tx;

		if (n > 0) {
			ratio_ack = (a2dp->cnt_ack - a2dp_last->cnt_ack) * 100;
			ratio_ack /= n;
			ratio_nack = (a2dp->cnt_nack - a2dp_last->cnt_nack)*100;
			ratio_nack /= n;
		}

		n = cx->cnt_bt[BTC_BCNT_RETRY] - fdinfo->bt_stat.retry_last;

		CLI_PRT("Retry_diff:%d, A2DP_empty_diff:%d"
			"[stop:%d/tx:%d/ack:%d(%d)/nack:%d(%d)]\n",
			n, a2dp->cnt_empty - a2dp_last->cnt_empty,
			a2dp->cnt_flowctrl - a2dp_last->cnt_flowctrl,
			a2dp->cnt_tx - a2dp_last->cnt_tx,
			a2dp->cnt_ack - a2dp_last->cnt_ack, ratio_ack,
			a2dp->cnt_nack - a2dp_last->cnt_nack, ratio_nack);

		n = sizeof(struct btc_rpt_ctrl_a2dp_empty);
		fdinfo->bt_stat.retry_last = cx->cnt_bt[BTC_BCNT_RETRY];
		hal_mem_cpy(btc->hal, a2dp_last, a2dp, n);
	}

	if (show_map & BIT(4))
		_show_fddt_cycle(btc, used, input, input_num, output, out_len);

	n = wl_smap->traffic_dir | 0x80;
	if (show_map & BIT(5))
		_cmd_fddt_cell(btc, used, input, n, output, out_len);
}

static void _cmd_fddt_set(struct btc_t *btc, u32 *used,
			  char input[][MAX_ARGV], u32 input_num,
			  char *output, u32 out_len)
{
	u32 para = 0;

	if (input_num > 1)
		_os_sscanf(input[1], "%d", &para);

	if (input_num <= 1 || para >= BTC_FDDT_MAX) {
		CLI_PRT(" ftset <val--- 0:force-stop, 1:auto,"
			" 2:fix-tdd-phase, 3:fix-fdd-phase> \n");
		return;
	}

	btc->dm.fddt_info.type = (u8) para;

	_run_coex(btc, __func__);
	CLI_PRT(" FDD-Train para = %d!!\n", para);
}

static void _cmd_fddt_ctrl(struct btc_t *btc, u32 *used,
			   char input[][MAX_ARGV], u32 input_num,
			   char *output, u32 out_len)
{
	struct btc_dm *dm = &btc->dm;
	struct btc_module *module = &btc->mdinfo;
	struct rtw_hal_com_t *h = btc->hal;
	struct btc_fddt_train_info *t = &dm->fddt_info.train;
	u32 type = 0, para = 0, para1 = 0;
	u8 buf[7] = {0};

	_os_sscanf(input[1], "%d", &type);

	if (input_num <= 1 || type > 4) {
		CLI_PRT(" ftctrl <val--- 0:time_cycle, 1:break_check,"
			" 2:fail_check, 3:cell_para, 4:cell_update>\n");
		return;
	} else if (input_num <= 3) {
		switch (type) {
		case 0:
			CLI_PRT(" time_cycle: ftctrl 0 "
				"<1:m_cycle, 2:w_cycle, 3:k_cycle>\n");
			return;
		case 1:
			CLI_PRT(" break_check: ftctrl 1 "
				"<1:bt_no_ept_cnt, 2:wl_tp_ratio,"
				" 3:wl_tp_low_bound, 4:cn, 5:cell_chg,"
				" 6: cn_limit, 7: nhm_limit> \n");
			return;
		case 2:
			CLI_PRT(" fail_check: ftctrl 2 "
				"<1:bt_no_ept_cnt, 2:wl_tp_ratio,"
				" 3:wl_kpibtr_ratio> \n");
			return;
		case 3:
			CLI_PRT(" cell_para: ftctrl 3 "
				"<cell_index:24bits> <cell_para:32bits> \n");
			CLI_PRT(" <cell_index>\n");
			CLI_PRT("   bit[23:16]= wl_dir 0:UL, 1:DL\n");
			CLI_PRT("   bit[15:8] = bt_rssi 0~4 \n");
			CLI_PRT("   bit[7:0]  = wl_rssi 0~4 \n");
			CLI_PRT(" <cell_para>\n");
			CLI_PRT("   bit[31:24]= wl_pwr_max 0~0xf \n");
			CLI_PRT("   bit[23:16]= wl_pwr_min 0~0xf \n");
			CLI_PRT("   bit[15:8] = bt_pwr_dec_max 0~0x28 \n");
			CLI_PRT("   bit[7:0]  = bt_rx_gain 0x7~0x4 \n");
			return;
		case 4:
			CLI_PRT(" cell_update: ftctrl 4 "
				 "<0:reset to default, 1:update by abt_iso>\n");
			return;
		}
	}

	if (type != 3) {
		_os_sscanf(input[2], "%d", &para);
		_os_sscanf(input[3], "%d", &para1);
	} else {
		_os_sscanf(input[2], "%x", &para);
		_os_sscanf(input[3], "%x", &para1);

		/*buf[0] buf[1] buf[2] : UL/DL, bt_rssi->0~4, wl_rssi->0~4*/
		buf[0] = (u8)((para & bMASKB2) >> 16);
		buf[1] = (u8)((para & bMASKB1) >> 8);
		buf[2] = (u8)(para & bMASKB0);

		/*buf[3] buf[4] buf[5] buf[6] :
		 * wl_pwr_max wl_pwr_min bt_pwr_dec_max bt_rx_gain
		 */
		buf[3] = (u8)((para1 & bMASKB3) >> 24);
		buf[4] = (u8)((para1 & bMASKB2) >> 16);
		buf[5] = (u8)((para1 & bMASKB1) >> 8);
		buf[6] = (u8)(para1 & bMASKB0);
	}

	switch (type) {
	case 0:
		CLI_PRT(" FDDT Time_cycle setup\n");
		if (para == 1) {
			t->t_ctrl.m_cycle = (u8) para1;
			CLI_PRT(" m_cycle=%d\n", para1);
		} else if (para == 2) {
			t->t_ctrl.w_cycle = (u8) para1;
			CLI_PRT(" w_cycle=%d\n", para1);
		} else {
			t->t_ctrl.k_cycle = (u8) para1;
			CLI_PRT(" k_cycle=%d\n", para1);
		}
		break;
	case 1:
		CLI_PRT(" FDDT Break_check setup\n");
		if (para == 1) {
			t->b_chk.bt_no_empty_cnt = (u8) para1;
			CLI_PRT(" bt_no_ept_cnt=%d\n", para1);
		} else if (para == 2) {
			t->b_chk.wl_tp_ratio = (u8) para1;
			CLI_PRT(" wl_tp_ratio=%d\n", para1);
		} else if (para == 3) {
			t->b_chk.wl_tp_low_bound = (u8) para1;
			CLI_PRT(" wl_tp_low=%d\n", para1);
		} else if (para == 4) {
			t->b_chk.cn = (u8) para1;
			CLI_PRT(" cn=%d\n", para1);
		} else if (para == 5) {
			t->b_chk.cell_chg = (u8) para1;
			CLI_PRT(" cell_chg_cnt=%d\n", para1);
		} else if (para == 6) {
			t->b_chk.cn_limit = (u8) para1;
			CLI_PRT(" cn_limit=%d\n", para1);
		} else {
			t->b_chk.nhm_limit = (s8) para1;
			CLI_PRT(" nhm_limit=%d\n", para1);
	 }
		break;
	case 2:
		CLI_PRT(" FDDT Fail_check setup\n");
		if (para == 1) {
			t->f_chk.bt_no_empty_cnt = (u8) para1;
			CLI_PRT(" bt_no_ept_cnt=%d\n", para1);
		} else if (para == 2) {
			t->f_chk.wl_tp_ratio = (u8) para1;
			CLI_PRT(" wl_tp_ratio=%d\n", para1);
		} else {
			t->f_chk.wl_kpibtr_ratio = (u8) para1;
			CLI_PRT(" wl_kpibtr_ratio=%d\n", para1);
		}
		break;
	case 3:
		if (!buf[0]) {
			t->cell_ul[buf[1]][buf[2]].wl_pwr_min = (u8) buf[3];
			t->cell_ul[buf[1]][buf[2]].wl_pwr_max = (u8) buf[4];
			t->cell_ul[buf[1]][buf[2]].bt_pwr_dec_max = (u8) buf[5];
			t->cell_ul[buf[1]][buf[2]].bt_rx_gain = (u8) buf[6];
			CLI_PRT(" FDDT Cell_para "
				"[UL:%d] [bt:%d]/[wl:%d] = %d/%d/%d/%d!!\n",
				buf[0], buf[1], buf[2], buf[3], buf[4],
				buf[5], buf[6]);
		} else {
			t->cell_dl[buf[1]][buf[2]].wl_pwr_min = (u8) buf[3];
			t->cell_dl[buf[1]][buf[2]].wl_pwr_max = (u8) buf[4];
			t->cell_dl[buf[1]][buf[2]].bt_pwr_dec_max = (u8) buf[5];
			t->cell_dl[buf[1]][buf[2]].bt_rx_gain = (u8) buf[6];
			CLI_PRT(" FDDT Cell_para "
				"[DL:%d] [bt:%d]/[wl:%d] = %d/%d/%d/%d!!\n",
				buf[0], buf[1], buf[2], buf[3], buf[4],
				buf[5], buf[6]);
		}
		break;
	case 4:
		if (para1 == 1) {
			_set_fddt_cell_by_antiso(btc);
			CLI_PRT(" FDDT Cell update by ant_iso = %d!\n",
				module->ant.isolation);
		} else {
			hal_mem_cpy(h, t->cell_ul, cell_ul_def,
				    sizeof(cell_ul_def));
			hal_mem_cpy(h, t->cell_dl, cell_dl_def,
				    sizeof(cell_dl_def));
			CLI_PRT(" FDDT Cell reset to default!\n");
		}
		break;
	default:
		break;
	}

	_run_coex(btc, __func__);
}
#endif

#ifdef BTC_AISO_SUPPORT
static void _bt_psd_enable(struct btc_t *btc, bool enable)
{
	u8 buf[8] = {0};

	buf[0] = 1;
	buf[1] = 0x22;
	buf[2] = 0x2;

	if (enable)
		buf[3] |= BIT(0);
#if 0
	if (fix_gain) {
		buf[3] |= BIT(1);
		buf[4] = (gain_idx & 0x3F);
	}
#endif

	_send_fw_cmd(btc, SET_H2C_TEST, buf, 5);
}

void _bt_psd_setup(struct btc_t *btc, u8 start_idx, u8 auto_rpt_type)
{
	u8 buf[2] = {0};

	PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_,
		  "[BTC], %s(): set bt psd start_idx = %d, rpt_type = %d\n",
		  __func__, start_idx, auto_rpt_type);

	buf[0] = start_idx;
	buf[1] = (auto_rpt_type & 0x3) | ((BT_PSD_SRC_RAW & 0x3) << 2);

	_send_fw_cmd(btc, SET_BT_PSD_REPORT, buf, 2);
}

static bool _bt_psd_rpt_ctrl(struct btc_t *btc, bool en_psd)
{
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	u8 cur_wl_ch = btc->cx.wl.afh_info.ch, start_index;
	bool result = false;

	if (en_psd && (cur_wl_ch >= 1 && cur_wl_ch <= 14) &&
	    (!bt_psd_dm->aiso_data_ok && !bt_psd_dm->en)) {

		if (cur_wl_ch < 14)
			start_index = 3 + 5 * (cur_wl_ch - 1);
		else
			start_index = 75;

		bt_psd_dm->en = true;
		bt_psd_dm->rec_time_out = false;
		result = true;

		/* set query start index to BT */
		_bt_psd_setup(btc, start_index, BT_PSD_RPT_TYPE_CAL);

		_bt_psd_enable(btc, true);

		PHL_INFO("[BTC], %s(): enable=%d: cur_wl_ch=%d, start_idx=%d\n",
			 __func__, en_psd, cur_wl_ch, start_index);
	} else if (!en_psd && bt_psd_dm->en) { /* disable */
		_bt_psd_enable(btc, false);
		bt_psd_dm->en = false;
		result = true;

		PHL_INFO("[BTC], %s(): disable!!\n", __func__);
	}

	return result;
}

static s8 _bt_psd_s16du8r(s16 sum, u8 num)
{
	s16 ans_int;
	s16 ans_fra;

	if (num == 0) {
		PHL_INFO("[BTC], %s() Divider is zero\n", __func__);
		return 1;
	}

	ans_int = (s16)(sum / num);
	ans_fra = sum - ans_int * num;

	if (ans_fra >= 0) {
		if (ans_fra * 2 >= (s16)num)
			ans_int++;
	} else {
		if (-ans_fra * 2 >= (s16)num)
			ans_int--;
	}
	return (s8)ans_int;
}

static s16 _bt_psd_s32du16r(s32 sum, u16 num)
{
	s32 ans_int;
	s32 ans_fra;

	if (num == 0) {
		PHL_INFO("[BTC], %s() Divider is zero\n", __func__);
		return 1;
	}

	ans_int = (s32)(sum / num);
	ans_fra = sum - ans_int * num;

	if (ans_fra >= 0) {
		if (ans_fra * 2 >= (s32)num)
			ans_int++;
	} else {
		if (-ans_fra * 2 >= (s32)num)
			ans_int--;
	}
	return (s16)ans_int;
}

static s8 _bt_psd_db_sum(s8 db_a, s8 db_b)
{
	s8 db_major;
	s8 db_minor;
	s8 db_sum;

	if (db_a >= db_b) {
		db_major = db_a;
		db_minor = db_b;
	} else {
		db_major = db_b;
		db_minor = db_a;
	}

	db_sum = db_major - db_minor; /* diff db */
	if (db_sum <= 1) /* 0~1 */
		db_sum = db_major + 3;
	else if (db_major <= 3) /* 2~3 */
		db_sum = db_major + 2;
	else if (db_major <= 9) /* 4~9 */
		db_sum = db_major + 1;
	else /* >=10 */
		db_sum = db_major;

	return db_sum;
}

static u16 _bt_psd_get_tx_rate(struct btc_t *btc)
{
	struct btc_wl_link_info *plink = NULL;
	u8 i;
	bool is_found = false;

	for (i = 0; i < BTC_WL_MAX_ROLE_NUMBER; i++) {
		plink = &btc->cx.wl.link_info[i];
		if (plink->active && plink->connected != MLME_NO_LINK) {
			is_found = true;
			break;
		}
	}

	if (!is_found)
		PHL_INFO("[BTC], %s() wl no valid port!!!\n", __func__);

	return (plink->tx_rate);
}

u8 _bt_psd_get_tx_ss_num(struct btc_t *btc)
{
	struct btc_wl_link_info *plink = NULL;
	u8 ss = 0, i;
	u16 tx_rate = RTW_DATA_RATE_MAX;
	bool is_found = false;

	for (i = 0; i < BTC_WL_MAX_ROLE_NUMBER; i++) {
		plink = &btc->cx.wl.link_info[i];
		if (plink->role == PHL_RTYPE_STATION &&
		    plink->active && plink->connected != MLME_NO_LINK) {
			is_found = true;
			tx_rate = plink->tx_rate;
			break;
		}
	}

	if (is_found && tx_rate != RTW_DATA_RATE_MAX) {
		if (tx_rate <= RTW_DATA_RATE_OFDM54)
			ss = 1;
		else if (tx_rate <= RTW_DATA_RATE_MCS31) /* bit[4:3] */
			ss = ((tx_rate >> 3) & 0x3) + 1;
		else /* bit[5:4] */
			ss = ((tx_rate >> 4) & 0x3) + 1;
	}

	return ss;
}

s8 _bt_psd_get_tx_pwr_dbm(struct btc_t *btc, u8 rf_path)
{
	s16 wl_txp_dbm_tssi;
	u32 reg_add;
	u16 reg_val;

	if (rf_path == RF_PATH_A)
		reg_add = 0x1C44;
	else
		reg_add = 0x3C44;

	reg_val = (u16)(rtw_hal_bb_read_cr(btc->hal, reg_add, 0x1FF) & 0x1FF);

	wl_txp_dbm_tssi = 16 + (((s16)reg_val - 300) / 8);

	return ((s8)wl_txp_dbm_tssi);
}

static s8 _bt_psd_get_tx_pwr(struct btc_t *btc)
{
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	s8 wl_tx_power_dbm = 0, tx_power_a, tx_power_b;

	bt_psd_dm->wl_tx_ss = _bt_psd_get_tx_ss_num(btc);
	bt_psd_dm->wl_tx_rate = _bt_psd_get_tx_rate(btc);

	tx_power_a = _bt_psd_get_tx_pwr_dbm(btc, RF_PATH_A);
	tx_power_b = _bt_psd_get_tx_pwr_dbm(btc, RF_PATH_B);

	if (bt_psd_dm->wl_tx_ss < 2)
		wl_tx_power_dbm = tx_power_a;
	else
		wl_tx_power_dbm = _bt_psd_db_sum(tx_power_a, tx_power_b);

	wl_tx_power_dbm -= _get_bw_att_db(btc); /* Tx power per MHz */

	return (wl_tx_power_dbm);
}

static void _bt_psd_aiso_var_reset(struct btc_t *btc)
{
	struct rtw_hal_com_t *h = (struct rtw_hal_com_t *)btc->hal;
	struct btc_aiso_val *av = &btc->bt_psd_dm.aiso_val;

	PHL_INFO("[BTC], %s():\n", __func__);
	hal_mem_set(h, av, 0, sizeof(struct btc_aiso_val));

	hal_mem_set(h, av->psd_max, 0x80, sizeof(av->psd_max));
	hal_mem_set(h, av->psd_min, 0x7F, sizeof(av->psd_min));

	hal_mem_set(h, av->txbusy_psd_max, 0x80, sizeof(av->txbusy_psd_max));
	hal_mem_set(h, av->txbusy_psd_min, 0x7F, sizeof(av->txbusy_psd_min));
	hal_mem_set(h, av->txbusy_psd_avg, 0x80, sizeof(av->txbusy_psd_avg));

	hal_mem_set(h, av->txidle_psd_max, 0x80, sizeof(av->txidle_psd_max));
	hal_mem_set(h, av->txidle_psd_min, 0x7F, sizeof(av->txidle_psd_min));
	hal_mem_set(h, av->txidle_psd_avg, 0x80, sizeof(av->txidle_psd_avg));

	hal_mem_set(h, av->aiso_md, -128, sizeof(av->aiso_md));
}

static void _bt_psd_aiso_db_update(struct btc_t *btc)
{
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	struct btc_aiso_val *av = &bt_psd_dm->aiso_val;
	u8 aiso_method = bt_psd_dm->aiso_method;
	u8 aiso_method_final = bt_psd_dm->aiso_method_final;
	u8 idx_offset = 0, i, cnt;
	s8 rx_psd_final = 0;
	s16 aiso_db_final = -128, sum;

	for (i = 0; i < 10; i++) {
		cnt = av->txidle_psd_cnt[i];
		sum = av->txidle_psd_sum[i];

		if (cnt == 0) {
			av->txidle_psd_avg[i] = -128;
			continue;
		}

		av->txidle_psd_avg[i] = _bt_psd_s16du8r(sum, cnt);
	}

	if (aiso_method != BTC_AISO_M01_AVG && aiso_method != BTC_AISO_M0 &&
	    aiso_method != BTC_AISO_M1 && aiso_method != BTC_AISO_M_ALL)
		aiso_db_final = -128;

	_limit_val(aiso_db_final, 127, -128);

	if (aiso_method_final < BTC_AISO_M_MAX) {
		aiso_db_final = av->aiso_md[aiso_method_final];
		rx_psd_final = av->rx_psd[aiso_method_final];
	}

	PHL_INFO("[BTC], %s(): Goal %d tms m:%d aiso = %d (%d-%d) dB\n",
		 __func__, av->psd_rec_cnt, aiso_method_final,
		 aiso_db_final, av->wl_air_psd_avg, rx_psd_final);

	/* Update ant iso to array: 1st_data -> index = 0  */
	bt_psd_dm->aiso_db[bt_psd_dm->aiso_db_cnt & 0xF] = (s8)aiso_db_final;
	bt_psd_dm->aiso_db_cnt++;

	if (bt_psd_dm->aiso_db_cnt < 16)
		idx_offset = 0;
	else
		idx_offset = bt_psd_dm->aiso_db_cnt - 16;

	for (i = idx_offset; i < bt_psd_dm->aiso_db_cnt; i++) {
		PHL_INFO("[BTC], %s(): %d times ant_iso = %d\n",
			 __func__, i+1, bt_psd_dm->aiso_db[i & 0xF]);
	}
}

static s16 _bt_psd_aiso_db_calc(struct btc_t *btc, s16 rx_psd_sum, u8 cnt)
{
	struct btc_aiso_val *av = &btc->bt_psd_dm.aiso_val;
	s32 res_part = 0;
	s16 aiso_db = -128;

	if (av->psd_rec_cnt == 0)
		return aiso_db;

	res_part = av->wl_air_psd_sum - av->wl_air_psd_avg * av->psd_rec_cnt;
	res_part = (res_part * cnt) / av->psd_rec_cnt;
	res_part = rx_psd_sum - res_part;

	_limit_val(res_part, 32767, -32768);

	aiso_db = av->wl_air_psd_avg - _bt_psd_s16du8r((s16)res_part, cnt);

	return aiso_db;
}

static void _bt_psd_group_calc(struct btc_t *btc, u8 md)
{
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	struct btc_aiso_val *av = &bt_psd_dm->aiso_val;
	s8 psd_max = 0, psd_min = 0;
	s8 *psd_avg = NULL;
	s16 rx_psd_sum = 0, psd_sum = 0, res_part = 0, val = 0;
	u8 cnt = 0, i, psd_cnt = 0;

	/* Excute for the following cases:
	 * 1. aiso_method = BTC_AISO_M_ALL
	 * 2. aiso_method = BTC_AISO_M4 && md = BTC_AISO_M1
	 * 3. aiso_method = BTC_AISO_M6 && md = BTC_AISO_M5
	 * 4. aiso_method = md
	 */
	if (bt_psd_dm->aiso_method != BTC_AISO_M_ALL &&
	    (!(bt_psd_dm->aiso_method == BTC_AISO_M4 && md == BTC_AISO_M1)) &&
	    (!(bt_psd_dm->aiso_method == BTC_AISO_M6 && md == BTC_AISO_M5)) &&
	    bt_psd_dm->aiso_method != md)
		return;

	for (i = 0; i < 10; i++) {
		psd_max = av->txbusy_psd_max[i];
		psd_min = av->txbusy_psd_min[i];
		psd_cnt = av->txbusy_psd_cnt[i];
		psd_sum = av->txbusy_psd_sum[i];
		psd_avg = &av->txbusy_psd_avg[i];

		/* Skip if the freq-point not reach the threshold */
		if (md <= BTC_AISO_M4 && psd_cnt < BT_PSD_TX_BUSY_CNT_MIN)
			continue;

		switch(md) {
		case BTC_AISO_M0: /* m0: busy avg */
			*psd_avg = _bt_psd_s16du8r(psd_sum, psd_cnt);
			rx_psd_sum += *psd_avg;
			cnt++;
			break;
		case BTC_AISO_M1: /* m1: busy max */
			rx_psd_sum += psd_max;
			cnt++;
			break;
		case BTC_AISO_M2: /* m2: busy remove max and min */
			val = psd_sum - psd_max - psd_min;
			*psd_avg = _bt_psd_s16du8r(val, psd_cnt - 2);
			rx_psd_sum += *psd_avg;
			cnt++;
			break;
		case BTC_AISO_M3: /* m3: busy remove max */
			val = psd_sum - psd_max;
			*psd_avg = _bt_psd_s16du8r(val, psd_cnt - 1);
			rx_psd_sum += *psd_avg;
			cnt++;
			break;
		case BTC_AISO_M4: /* m4:filter m1 */
			if (psd_max >= av->rx_psd[BTC_AISO_M1])
				rx_psd_sum += psd_max;
			cnt++;
			break;
		case BTC_AISO_M5: /* m5: hold max value */
			rx_psd_sum += psd_max;
			cnt++;
			break;
		case BTC_AISO_M6: /* m6: filter m5 */
			if (psd_max >= av->rx_psd[BTC_AISO_M5])
				rx_psd_sum += psd_max;
			cnt++;
			break;
		case BTC_AISO_M01_AVG: /* avg (m0 + m1) */
			res_part = av->aiso_md[BTC_AISO_M0] +
				   av->aiso_md[BTC_AISO_M1];
			rx_psd_sum = av->rx_psd[BTC_AISO_M0] +
				     av->rx_psd[BTC_AISO_M1];

			cnt = 2;
			break;
		case BTC_AISO_M12_AVG: /* avg (m1 + m2) */
			res_part = av->aiso_md[BTC_AISO_M1] +
				   av->aiso_md[BTC_AISO_M2];
			rx_psd_sum = av->rx_psd[BTC_AISO_M1] +
				     av->rx_psd[BTC_AISO_M2];
			cnt = 2;
			break;
		case BTC_AISO_M13_AVG: /* avg (m1 + m3) */
			res_part = av->aiso_md[BTC_AISO_M1] +
				   av->aiso_md[BTC_AISO_M3];
			rx_psd_sum = av->rx_psd[BTC_AISO_M1] +
				     av->rx_psd[BTC_AISO_M3];
			cnt = 2;
			break;
		case BTC_AISO_M46_AVG: /* avg (m4 + m6) */
			res_part = av->aiso_md[BTC_AISO_M4] +
				   av->aiso_md[BTC_AISO_M6];
			rx_psd_sum = av->rx_psd[BTC_AISO_M4] +
				     av->rx_psd[BTC_AISO_M6];
			cnt = 2;
			break;
		}
	}

	if (cnt == 0) {
		PHL_INFO("[BTC], %s(): m:%d return by cnt = 0\n", __func__, md);
		return;
	}

	av->rx_psd[md] = _bt_psd_s16du8r(rx_psd_sum, cnt);

	if (md <= BTC_AISO_M6)
		av->aiso_md[md] = _bt_psd_aiso_db_calc(btc, rx_psd_sum, cnt);
	else
		av->aiso_md[md] = _bt_psd_s16du8r(res_part, cnt);
}

static void _bt_psd_rec_refresh(struct btc_t *btc, bool is_2nd_half)
{
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	struct btc_aiso_val *av = &bt_psd_dm->aiso_val;
	u8 i = 0, rec_idx = 0;
	u8 *half_data = NULL;
	u8 wl_tx_busy_status = bt_psd_dm->raw_info[2];
	s8 bt_psd_dbm_offset = 0; /* if lna-constrain 5 = 28*/
	s8 cur_psd_rpt_dbm;

	if (is_2nd_half)
		half_data = &av->last_2nd_half[0];
	else
		half_data = &av->last_1st_half[0];

	hal_mem_cpy(btc->hal, half_data, &bt_psd_dm->raw_info[2], 5);

	for (i = 0; i < 5; i++) { /* psd_data_0 ~ psd_data_4 */
		rec_idx = (is_2nd_half ? i + 5 : i);

		cur_psd_rpt_dbm = bt_psd_dm->raw_info[i + 3];
		cur_psd_rpt_dbm += bt_psd_dbm_offset;

		if (cur_psd_rpt_dbm > av->psd_max[rec_idx])
			av->psd_max[rec_idx] = cur_psd_rpt_dbm;

		if (cur_psd_rpt_dbm < av->psd_min[rec_idx])
			av->psd_min[rec_idx] = cur_psd_rpt_dbm;

		if (wl_tx_busy_status & BIT(i)) {
			if (cur_psd_rpt_dbm > av->txbusy_psd_max[rec_idx])
				av->txbusy_psd_max[rec_idx] = cur_psd_rpt_dbm;

			if (cur_psd_rpt_dbm < av->txbusy_psd_min[rec_idx])
				av->txbusy_psd_min[rec_idx] = cur_psd_rpt_dbm;

			/* avoid overflow */
			if (av->txbusy_psd_cnt[rec_idx] < 200) {
				av->txbusy_psd_sum[rec_idx] += cur_psd_rpt_dbm;
				av->txbusy_psd_cnt[rec_idx]++;
			}
		} else {
			if (cur_psd_rpt_dbm > av->txidle_psd_max[rec_idx])
				av->txidle_psd_max[rec_idx] = cur_psd_rpt_dbm;

			if (cur_psd_rpt_dbm < av->txidle_psd_min[rec_idx])
				av->txidle_psd_min[rec_idx] = cur_psd_rpt_dbm;

			/* avoid overflow */
			if (av->txidle_psd_cnt[rec_idx] < 200) {
				av->txidle_psd_sum[rec_idx] += cur_psd_rpt_dbm;
				av->txidle_psd_cnt[rec_idx]++;
			}
		}
	}
}

static bool _bt_psd_ready_check(struct btc_t *btc)
{
	struct btc_aiso_val *av = &btc->bt_psd_dm.aiso_val;

	u8 half1_ch = 0, half2_ch = 0, i = 0;

	/* calculate 1st_half_data_enough_flag */
	for (i = 0; i < 5; i++)
		if (av->txbusy_psd_cnt[i] >= BT_PSD_TX_BUSY_CNT_MIN)
			half1_ch++;

	/* calculate 2nd_half_data_enough_flag */
	for (i = 5; i < 10; i++)
		if (av->txbusy_psd_cnt[i] >= BT_PSD_TX_BUSY_CNT_MIN)
			half2_ch++;

	if (av->psd_rec_cnt % BT_PSD_PRINT_PERIOD == 1)
		PHL_INFO("[BTC], %s(): TxBusy_PSD_CNT[ch]"
			 " = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
			 av->txbusy_psd_cnt[0], av->txbusy_psd_cnt[1],
			 av->txbusy_psd_cnt[2], av->txbusy_psd_cnt[3],
			 av->txbusy_psd_cnt[4], av->txbusy_psd_cnt[5],
			 av->txbusy_psd_cnt[6], av->txbusy_psd_cnt[7],
			 av->txbusy_psd_cnt[8], av->txbusy_psd_cnt[9]);

	if (half1_ch >= BT_PSD_VALID_CH_MIN && half2_ch >= BT_PSD_VALID_CH_MIN)
		return true;
	else
		return false;
}

/* post-process bt psd data and change flag to stop it */
static void _bt_psd_process(struct btc_t *btc, bool is_reset)
{
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	struct btc_aiso_val *av = &bt_psd_dm->aiso_val;

	u8 i = 0, start_index, cur_wl_ch = btc->cx.wl.afh_info.ch;
	u8 seq_idx = (bt_psd_dm->raw_info[1] >> 4) & 0xF;
	u8 idx_1st_half = 0, idx_2nd_half = 0;
	u16 cnt = 0;

	/* bt_psd_dm->raw_info format:
	 * (buf + 0) : malbox id
	 * (buf + 1) :  [1:0] : data_src
	 *		[3:2] : auto_rpt_type
	 *		[7:4] : seq_idx
	 * (buf + 2) : 	[4:0] : wl_s0_txbusy_mask
	 *		[7:5] : reserved for wl_rxbusy
	 * (buf + 3) : psd_data_0
	 * (buf + 4) : psd_data_1
	 * (buf + 5) : psd_data_2
	 * (buf + 6) : psd_data_3
	 * (buf + 7) : psd_data_4
	 */

	if (is_reset)
		_bt_psd_aiso_var_reset(btc);

	/* cur_wl_ch decide the psd-start index
	 * ex: cur_wl_ch = 1 --> start index = 3 (2405MHz)
	 * idx_1st_half = 3 ~ 7 (2405MHz~2409MHz)
	 * idx_2nd_half = 13 ~ 17 (2415MHz~2419MHz)
	 */
	if (cur_wl_ch < 14)
		start_index = 3 + 5 * (cur_wl_ch - 1);
	else
		start_index = 75;

	idx_1st_half = start_index & 0xF;
	idx_2nd_half = (start_index + 10) & 0xF;

	if (seq_idx != idx_1st_half && seq_idx != idx_2nd_half) {
		PHL_INFO("[BTC], %s(): wrong psd report index\n", __func__);
		return;
	} else {
		/* refreh record  if data is NOT duplicate */
		av->psd_rec_cnt++;
		cnt = av->psd_rec_cnt;

		PHL_INFO("[BTC], %s(): psd_rec_cnt = %d\n", __func__, cnt);

		if (seq_idx == idx_1st_half)
			_bt_psd_rec_refresh(btc, false);
		else if (seq_idx == idx_2nd_half)
			_bt_psd_rec_refresh(btc, true);
	}

	/* record wl tx psd */
	av->wl_air_psd_sum += _bt_psd_get_tx_pwr(btc); /* dbm per MHz */

	av->wl_air_psd_avg = (s8)_bt_psd_s32du16r(av->wl_air_psd_sum, cnt);

	/* Check if data ready */
	if (!_bt_psd_ready_check(btc)) {
		if (cnt >= BT_PSD_WAIT_CNT) { /* timeout */
			PHL_INFO("[BTC], %s(): wait rec timeout!\n", __func__);

			bt_psd_dm->aiso_data_ok = false;
			bt_psd_dm->rec_start = false;
			bt_psd_dm->rec_time_out = true;
			_bt_psd_rpt_ctrl(btc, false);
		}

		return;
	}

	/* calculate antenna isolation if data ready */
	for (i = 0; i < BTC_AISO_M_MAX; i++) {

		/* Must execute the other methos first for M4/M6 */
		if (i == BTC_AISO_M4)
			_bt_psd_group_calc(btc, BTC_AISO_M1);
		else if (i == BTC_AISO_M6)
			_bt_psd_group_calc(btc, BTC_AISO_M5);

		_bt_psd_group_calc(btc, i);

		PHL_INFO("[BTC], %s(): m%d = %d (%d-(%d)) dB\n", __func__,
			 i, av->aiso_md[i], av->wl_air_psd_avg, av->rx_psd[i]);
	}

	bt_psd_dm->aiso_data_ok = true;
	bt_psd_dm->rec_start = false;

	_bt_psd_aiso_db_update(btc);

	if (bt_psd_dm->aiso_db_cnt < bt_psd_dm->aiso_cmd_cnt) {
		PHL_INFO("[BTC], %s(): aiso_db_cnt = %d, aiso_cmd_cnt = %d\n",
			 __func__, bt_psd_dm->aiso_db_cnt,
			 bt_psd_dm->aiso_cmd_cnt);

		bt_psd_dm->aiso_data_ok = false;
		bt_psd_dm->rec_start = false;
		bt_psd_dm->rec_time_out = false;
		return;
	}

	_bt_psd_rpt_ctrl(btc, false);

	PHL_INFO("[BTC], %s(): Good-Ending!!\n", __func__);
}

/* post-process and change flag to stop it */
void _bt_psd_update(struct btc_t *btc, u8 *buf, u32 len)
{
	struct rtw_hal_com_t *h = btc->hal;
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	u8 cur_wl_ch = btc->cx.wl.afh_info.ch;
	bool reset_data_flag = false;

	if (!bt_psd_dm->en) {
		/* PHL_INFO("[BTC], %s(): drop by en = false\n", __func__); */
		return;
	}

	if (!btc->dm.freerun) {
		/* return; */
	}

	PHL_INFO("[BTC], %s(len=%d): seq_idx:0x%x/type:0x%x/src:0x%x/tx:0x%x"
		 " [%x %x %x %x %x]\n", __func__, len,
		 *(buf + 1) >> 4, (*(buf + 1) >> 2) & 0x3,
		 *(buf + 1) & 0x3, *(buf + 2) & 0x1F,
		 *(buf + 3), *(buf + 4), *(buf + 5), *(buf + 6), *(buf + 7));

	hal_mem_cpy(h, bt_psd_dm->raw_info, buf, BTC_BTINFO_MAX);

	/* before start */
	if (!bt_psd_dm->rec_start && !bt_psd_dm->aiso_data_ok) {
		bt_psd_dm->rec_start = true;
		reset_data_flag = true;
	} else if (bt_psd_dm->wl_ch_last != 0xFF &&
		   (cur_wl_ch != bt_psd_dm->wl_ch_last)) {
		reset_data_flag = true; /* rec_start & Not-Ready -> change ch */
	}

	if (bt_psd_dm->rec_start) {
		_bt_psd_process(btc, reset_data_flag);
		bt_psd_dm->wl_ch_last = cur_wl_ch;
	}
}

static void _bt_psd_show_record(struct btc_t *btc, u32 *used,
				char input[][MAX_ARGV], u32 input_num,
				char *output, u32 out_len)
{
	struct btc_aiso_val *av = &btc->bt_psd_dm.aiso_val;

	CLI_PRT("============ %s()  ============\n", __func__);

	CLI_PRT(" PSD_MAX = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d dBm\n",
		av->psd_max[0], av->psd_max[1], av->psd_max[2], av->psd_max[3],
		av->psd_max[4], av->psd_max[5], av->psd_max[6], av->psd_max[7],
		av->psd_max[8], av->psd_max[9]);

	CLI_PRT(" PSD_MIN = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d dBm\n",
		av->psd_min[0], av->psd_min[1], av->psd_min[2], av->psd_min[3],
		av->psd_min[4], av->psd_min[5], av->psd_min[6], av->psd_min[7],
		av->psd_min[8], av->psd_min[9]);

	CLI_PRT(" TxBusy_PSD_MAX = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d dBm\n",
		av->txbusy_psd_max[0], av->txbusy_psd_max[1],
		av->txbusy_psd_max[2], av->txbusy_psd_max[3],
		av->txbusy_psd_max[4], av->txbusy_psd_max[5],
		av->txbusy_psd_max[6], av->txbusy_psd_max[7],
		av->txbusy_psd_max[8], av->txbusy_psd_max[9]);

	CLI_PRT(" TxBusy_PSD_MIN = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d dBm\n",
		av->txbusy_psd_min[0], av->txbusy_psd_min[1],
		av->txbusy_psd_min[2], av->txbusy_psd_min[3],
		av->txbusy_psd_min[4], av->txbusy_psd_min[5],
		av->txbusy_psd_min[6], av->txbusy_psd_min[7],
		av->txbusy_psd_min[8], av->txbusy_psd_min[9]);

	CLI_PRT(" TxBusy_PSD_AVG = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d dBm\n",
		av->txbusy_psd_avg[0], av->txbusy_psd_avg[1],
		av->txbusy_psd_avg[2], av->txbusy_psd_avg[3],
		av->txbusy_psd_avg[4], av->txbusy_psd_avg[5],
		av->txbusy_psd_avg[6], av->txbusy_psd_avg[7],
		av->txbusy_psd_avg[8], av->txbusy_psd_avg[9]);

	CLI_PRT(" TxBusy_PSD_CNT = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		av->txbusy_psd_cnt[0], av->txbusy_psd_cnt[1],
		av->txbusy_psd_cnt[2], av->txbusy_psd_cnt[3],
		av->txbusy_psd_cnt[4], av->txbusy_psd_cnt[5],
		av->txbusy_psd_cnt[6], av->txbusy_psd_cnt[7],
		av->txbusy_psd_cnt[8], av->txbusy_psd_cnt[9]);

	CLI_PRT(" TxIdle_PSD_MAX = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d dBm\n",
		av->txidle_psd_max[0], av->txidle_psd_max[1],
		av->txidle_psd_max[2], av->txidle_psd_max[3],
		av->txidle_psd_max[4], av->txidle_psd_max[5],
		av->txidle_psd_max[6], av->txidle_psd_max[7],
		av->txidle_psd_max[8], av->txidle_psd_max[9]);

	CLI_PRT(" TxIdle_PSD_MIN = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d dBm\n",
		av->txidle_psd_min[0], av->txidle_psd_min[1],
		av->txidle_psd_min[2], av->txidle_psd_min[3],
		av->txidle_psd_min[4], av->txidle_psd_min[5],
		av->txidle_psd_min[6], av->txidle_psd_min[7],
		av->txidle_psd_min[8], av->txidle_psd_min[9]);

	CLI_PRT(" TxIdle_PSD_AVG = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d dBm\n",
		av->txidle_psd_avg[0], av->txidle_psd_avg[1],
		av->txidle_psd_avg[2], av->txidle_psd_avg[3],
		av->txidle_psd_avg[4], av->txidle_psd_avg[5],
		av->txidle_psd_avg[6], av->txidle_psd_avg[7],
		av->txidle_psd_avg[8], av->txidle_psd_avg[9]);

	CLI_PRT(" TxIdle_PSD_CNT = %d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		av->txidle_psd_cnt[0], av->txidle_psd_cnt[1],
		av->txidle_psd_cnt[2], av->txidle_psd_cnt[3],
		av->txidle_psd_cnt[4], av->txidle_psd_cnt[5],
		av->txidle_psd_cnt[6], av->txidle_psd_cnt[7],
		av->txidle_psd_cnt[8], av->txidle_psd_cnt[9]);
}

static void _bt_psd_aiso_sort(struct btc_t *btc, u32 *used,
			      char *output, u32 out_len)
{
	struct btc_bt_psd_dm *bp = &btc->bt_psd_dm;
	struct btc_module *module = &btc->mdinfo;
	u8 i, rec_idx, cnt = bp->aiso_db_cnt;
	u8 max_aiso = 0, mid_cnt = 0, th = 3, min_lim = 0, max_lim = 0,
	   avg_sort_cnt = 0;
	u32 aiso_sort_tol = 0;

	if (cnt > 16)
		return;

	mid_cnt = cnt % 2 ? cnt : cnt + 1;

	hal_mem_cpy(btc->hal, bp->aiso_sort_db, bp->aiso_db, 16);

	//sort raw data of antenna-isolation results
	for (i = 0; i < cnt-1; i++) {
		for (rec_idx = 0; rec_idx <(u8)(cnt-1-i); rec_idx++) {
			if (bp->aiso_sort_db[rec_idx] <=
			    bp->aiso_sort_db[rec_idx+1])
			    continue;

			max_aiso = bp->aiso_sort_db[rec_idx];
			bp->aiso_sort_db[rec_idx] = bp->aiso_sort_db[rec_idx+1];
			bp->aiso_sort_db[rec_idx+1] = max_aiso;
		}
	}

	min_lim = (bp->aiso_sort_db[(mid_cnt/2)-1]) - th;
	max_lim = (bp->aiso_sort_db[(mid_cnt/2)-1]) + th;
	// list the sorting data of antenna-isolation results
	for (rec_idx = 0; rec_idx < cnt; rec_idx++) {
		CLI_PRT(" #%3d: ant_iso sort result = %3d dB\n",
			rec_idx+1, bp->aiso_sort_db[rec_idx]);
	}

	// list mid data of sorting antenna-isolation results
	CLI_PRT(" #%3d: ant_iso sort mid result = %3d dB\n",
		(mid_cnt/2), bp->aiso_sort_db[(mid_cnt/2)-1]);

	// calculate average of sorting antenna-isolation results
	for (rec_idx = 0; rec_idx < cnt; rec_idx++) {
		if (bp->aiso_sort_db[rec_idx] >= min_lim &&
		    bp->aiso_sort_db[rec_idx] <= max_lim) {
			avg_sort_cnt ++;
			aiso_sort_tol += bp->aiso_sort_db[rec_idx];
		}
	}

	//list average result
	if (avg_sort_cnt != 0)
		bp->aiso_sort_avg = aiso_sort_tol / avg_sort_cnt;
	module->ant.isolation = bp->aiso_sort_avg;
	CLI_PRT(" ant_iso sort avg result = %3d dB, avg_count = %3d\n",
		bp->aiso_sort_avg, avg_sort_cnt);
}

static void _cmd_get_tx_pwr(struct btc_t *btc, u32 *used,
			    char input[][MAX_ARGV], u32 input_num,
			    char *output, u32 out_len)
{
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	s8 txp_dbm = 127, txp_dbm_mhz = 127;

	txp_dbm_mhz = _bt_psd_get_tx_pwr(btc);
	txp_dbm = txp_dbm_mhz + _get_bw_att_db(btc);

	CLI_PRT("tx_rate=%s, ss=%d, txp=%d dBm, txp_MHz=%d dBm/MHz\n",
		id_to_str(BTC_STR_RATE, (u32)bt_psd_dm->wl_tx_rate),
		bt_psd_dm->wl_tx_ss, txp_dbm, txp_dbm_mhz);
}

static void _cmd_trig_aiso(struct btc_t *btc, u32 *used,
			   char input[][MAX_ARGV], u32 input_num,
			   char *output, u32 out_len)
{
	struct rtw_hal_com_t *h = btc->hal;
	struct btc_bt_psd_dm *bp = &btc->bt_psd_dm;
	u32 enable = 0, exec_cnt = 1;

	if (input_num >= 3)
		_os_sscanf(input[2], "%d", &enable);

	if (input_num > 3)
		_os_sscanf(input[3], "%d", &exec_cnt);

	if (input_num < 3 || exec_cnt == 0 || exec_cnt > 16) {
		CLI_PRT(" aiso 1 <0:force stop, 1:trig start>\n");
		CLI_PRT(" aiso 1 1 <execute count:1~16>\n");
		return;
	}

	if (enable && bp->en) {
		CLI_PRT("ant isolation calculation is already running!!\n");
		return;
	}

	if (enable) {
		bp->aiso_data_ok = false;
		bp->rec_start = false;
		bp->aiso_cmd_cnt = (u8)exec_cnt;

		if (bp->aiso_cmd_cnt > 1) {
			bp->aiso_db_cnt = 0;
			hal_mem_set(h, bp->aiso_db, 0, sizeof(bp->aiso_db));
		}

		_bt_psd_rpt_ctrl(btc, true);

		CLI_PRT("trig ant isolation calculation (exec_cnt=%d)!!\n",
			bp->aiso_cmd_cnt);

	} else {
		_bt_psd_rpt_ctrl(btc, false);
		CLI_PRT("stop ant isolation calculation!!!!\n");
	}

	_run_coex(btc, __func__);
}

static void _cmd_get_aiso(struct btc_t *btc, u32 *used,
			  char input[][MAX_ARGV], u32 input_num,
			  char *output, u32 out_len)
{
	struct btc_bt_psd_dm *bt_psd_dm = &btc->bt_psd_dm;
	struct btc_aiso_val *av = &bt_psd_dm->aiso_val;
	u8 idx_offset, i, rec_idx, md, cnt;
	u32 debug_level = 0;

	if (input_num < 3) {
		CLI_PRT(" aiso 0 <0:current result, 1:history result>\n");
		return;
	}

	_os_sscanf(input[2], "%d", &debug_level);

	if (!bt_psd_dm->aiso_data_ok) {
		if (bt_psd_dm->rec_time_out)
			CLI_PRT(" bt psd rpt record timeout!!\n");
		else if (!bt_psd_dm->en)
			CLI_PRT(" not start to enable bt psd rpt!!\n");
		else
			CLI_PRT(" ant isolation is under calculation!!\n");

		return;
	}

	cnt = bt_psd_dm->aiso_db_cnt;

	if (cnt == 0) {
		CLI_PRT(" no ant_iso result!!\n");
		return;
	}

	/* List the late antenna-isolation result */
	if (debug_level == 0) {
		rec_idx = (cnt - 1) & 0xF;
		CLI_PRT(" current ant_iso result = %3d dB\n",
			bt_psd_dm->aiso_db[rec_idx]);
		return;
	}

	/* List the latest 16 antenna-isolation results */
	idx_offset = (cnt < 16 ? 0 : cnt - 16);

	for (i = idx_offset; i < cnt; i++) {
		rec_idx = i & 0xF;
		CLI_PRT(" #%3d: ant_iso result = %3d dB\n",
			i+1, bt_psd_dm->aiso_db[rec_idx]);
	}

	_bt_psd_aiso_sort(btc, used, output, out_len);

	md = bt_psd_dm->aiso_method_final;
	CLI_PRT(" psd_rpt: rec_cnt=%d, method=%d, ant_iso_db=%d (%d-(%d))dB\n",
		av->psd_rec_cnt, md, av->aiso_md[md],
		av->wl_air_psd_avg, av->rx_psd[md]);

	for (i = 0; i < BTC_AISO_M_MAX; i++) {

		if (i != BTC_AISO_M4 && i != BTC_AISO_M5 &&
		    i != BTC_AISO_M6 && i != BTC_AISO_M46_AVG)
			continue;

		CLI_PRT(" ant_iso_md (m:%d) = %d (%d - (%d))dB\n", i,
			av->aiso_md[i], av->wl_air_psd_avg, av->rx_psd[i]);
	}

	_bt_psd_show_record(btc, used, input, input_num, output, out_len);
}

static void _cmd_ant_iso(struct btc_t *btc, u32 *used,
			 char input[][MAX_ARGV], u32 input_num,
			 char *output, u32 out_len)
{
	u32 cmd_set = 0;

	if (input_num < 2)
		goto help;

	_os_sscanf(input[1], "%d", &cmd_set);

	switch (cmd_set) {
	case 0:
		_cmd_get_aiso(btc, used, input, input_num, output, out_len);
		break;
	case 1:
		_cmd_trig_aiso(btc, used, input, input_num, output, out_len);
		break;
	case 2:
		_cmd_get_tx_pwr(btc, used, input, input_num, output, out_len);
		break;
	default:
		goto help;
	}

	return;
help:
	CLI_PRT("aiso <0:show result 1:trig 2:show wl tx power>\n");
}
#else
void _bt_psd_setup(struct btc_t *btc, u8 start_idx, u8 rpt_type)
{
	u8 buf[2] = {0};

	PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_, "[BTC], %s(): set bt psd\n",
		  __func__);

	buf[0] = start_idx;
	buf[1] = rpt_type;
	_send_fw_cmd(btc, SET_BT_PSD_REPORT, buf, 2);
}

void _bt_psd_update(struct btc_t *btc, u8 *buf, u32 len)
{
	PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_, "[BTC], %s():\n", __func__);
}
#endif

void halbtc_cmd_parser(struct btc_t *btc, char input[][MAX_ARGV],
			u32 input_num, char *output, u32 out_len)
{
	u32 offset = 0;
	u32 *used = &offset;
	u8 id = 0;
	u32 i;
	u32 array_size = sizeof(halbtc_cmd_i) / sizeof(struct halbtc_cmd_info);

	CLI_PRT("\n");

	/* Parsing Cmd ID */
	if (input_num) {
		/* Avoid input number and debug cmd mismatch */
		input_num--;
		for (i = 0; i < array_size; i++) {
			if (_os_strcmp(halbtc_cmd_i[i].name, input[0]) == 0) {
				id = halbtc_cmd_i[i].id;
				break;
			}
		}
	}

	switch (id) {
	case HALBTC_DBG:
		_cmd_dbg(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_SHOW:
		_cmd_show(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_WRITE_BT:
		_cmd_wb(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_READ_BT:
		_cmd_rb(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_SET_COEX:
		_cmd_set_coex(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_UPDATE_POLICY:
		_cmd_upd_policy(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_TDMA:
		_cmd_tdma(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_SLOT:
		_cmd_slot(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_SIG_GDBG_EN:
		_cmd_sig_gdbg_en(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_SGPIO_MAP:
		_cmd_sgpio_map(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_WL_TX_POWER:
		_cmd_wl_tx_power(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_WL_RX_LNA:
		_cmd_wl_rx_lna(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_BT_AFH_MAP:
		_cmd_bt_afh_map(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_BT_TX_POWER:
		_cmd_bt_tx_power(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_BT_RX_LNA:
		_cmd_bt_rx_lna(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_BT_IGNO_WLAN:
		_cmd_bt_igno_wl(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_SET_GNT_WL:
		_cmd_set_gnt_wl(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_SET_GNT_BT:
		_cmd_set_gnt_bt(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_SET_BT_PSD:
		_cmd_set_bt_psd(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_GET_WL_NHM_DBM:
		_cmd_get_wl_nhm(btc, used, input, input_num, output, out_len);
		break;
#ifdef BTC_FDDT_TRAIN_SUPPORT
	case HALBTC_FT_CTRL:
		_cmd_fddt_ctrl(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_FT_SET:
		_cmd_fddt_set(btc, used, input, input_num, output, out_len);
		break;
	case HALBTC_FT_SHOW:
		_cmd_fddt_show(btc, used, input, input_num, output, out_len);
		break;
#endif
#ifdef BTC_AISO_SUPPORT
	case HALBTC_AISO:
		_cmd_ant_iso(btc, used, input, input_num, output, out_len);
		break;
#endif
	default:
		CLI_PRT("BTC cmd ==>\n");

		for (i = 0; i < array_size - 1; i++)
			CLI_PRT(" %s\n", halbtc_cmd_i[i + 1].name);
		break;
	}
}

s32 halbtc_cmd(struct btc_t *btc, char *input, char *output, u32 out_len)
{
	char *token;
	u32 argc = 0;
	char argv[MAX_ARGC][MAX_ARGV];

	do {
		token = _os_strsep(&input, ", ");
		if (token) {
			if (_os_strlen((u8 *)token) <= MAX_ARGV)
				_os_strcpy(argv[argc], token);

			argc++;
		} else {
			break;
		}
	} while (argc < MAX_ARGC);
#if 0
	if (argc == 1)
		argv[0][_os_strlen((u8 *)argv[0]) - 1] = '\0';
#endif

	halbtc_cmd_parser(btc, argv, argc, output, out_len);
	return 0;
}

#endif


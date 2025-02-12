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
#define _HAL_BTC_FW_C_
#include "../hal_headers_le.h"
#include "hal_btc.h"
#include "halbtc_fw.h"
#include "halbtc_def.h"

#ifdef CONFIG_BTCOEX

const struct btc_byte_map_desc bmap_h2c[CXDRVINFO_MAX] = {
	{ 24,  0,  0}, /* CXDRVINFO_INIT -> struct btc_init_info */
	{ 76,  0,  6}, /* CXDRVINFO_ROLE -> struct btc_wl_role_info */
	{  4,  0,  0}, /* CXDRVINFO_CTRL -> struct btc_ctrl */
	{ 12,  2,  3}, /* CXDRVINFO_TRX -> struct btc_trx_info */
	{  1,  0,  0}, /* CXDRVINFO_TXPWR -> u8 */
	{216,  0,  0}, /* CXDRVINFO_FDDT -> struct btc_fddt_train_info */
};

const struct btc_byte_map_desc bmap_c2h[BTC_RPT_TYPE_MAX] = {
	{ 12, 12, 11}, /* BTC_RPT_TYPE_CTRL -> struct fbtc_rpt_ctrl */
	{ 12,  0,  0}, /* BTC_RPT_TYPE_TDMA -> struct fbtc_1tdma */
	{  4, 72,  1}, /* BTC_RPT_TYPE_SLOT -> struct fbtc_slots */
	{436, 82,  2}, /* BTC_RPT_TYPE_CYSTA -> struct fbtc_cysta */
#ifdef BTC_FW_STEP_DBG
	{204,  0,  1}, /* BTC_RPT_TYPE_STEP -> struct fbtc_steps */
#else
	{  4,  0,  1}, /* BTC_RPT_TYPE_STEP -> struct fbtc_steps */
#endif
	{  4,  0, 14}, /* BTC_RPT_TYPE_NULLSTA -> struct fbtc_cynullsta */
	{780,  0,  0}, /* BTC_RPT_TYPE_FDDT -> struct fbtc_fddt_sta */
	{  4,  0, 20}, /* BTC_RPT_TYPE_MREG -> struct fbtc_mreg_val */
	{ 36,  0,  2}, /* BTC_RPT_TYPE_GPIO_DBG -> struct fbtc_gpio_dbg */
	{  4,  0,  3}, /* BTC_RPT_TYPE_BT_VER -> struct fbtc_btver */
	{  4,  6,  0}, /* BTC_RPT_TYPE_BT_SCAN -> struct fbtc_btscan */
	{ 24,  0,  0}, /* BTC_RPT_TYPE_BT_AFH -> struct fbtc_btafh */
	{  2,  1,  2}, /* BTC_RPT_TYPE_BT_DEVICE -> struct fbtc_btdevinfo */
	{  0,  0,  0}, /* BTC_RPT_TYPE_TEST */
};

#if BTC_PLATFORM_BIG_ENDIAN
void _endian_translate(struct btc_t *btc, u8 dir, u8* data)
{
	struct btc_dm *dm = &btc->dm;
	u8 type;
	u16 offset, len, i;
	const struct btc_byte_map_desc *bmap = NULL;

	if (!data)
		return;

	if (dir == BTC_FW_H2C) {
		if (data[0] >= CXDRVINFO_MAX)
			return;

		type = data[0];
		len = data[2];
		offset = bmap[type].map_u8 + 2;
		bmap = bmap_h2c;
	} else {
		if (data[0] >= BTC_RPT_TYPE_MAX)
			return;

		type = data[0];
		len = (data[2] << 8) + data[1];
		offset = bmap[type].map_u8 + 3;
		bmap = bmap_c2h;
	}

	if (len != (bmap[type].map_u8 +
		    bmap[type].map_u16 * 2 +
		    bmap[type].map_u32 * 4)) {

		if (dir == BTC_FW_H2C)
			dm->error |= BTC_DMERR_H2C_BMAP_MISMATCH;
		else
			dm->error |= BTC_DMERR_C2H_BMAP_MISMATCH;
		return;

	}

	if (!bmap[type].map_u16 && !bmap[type].map_u32) /* only u8 */
		return;

	if (bmap[type].map_u16 != 0) /* u16 byte-swap */
		for (i = 0; i < bmap[type].map_u16; i++, offset += 2)
			btc_swap2(data[offset], data[offset + 1]);

	if (bmap[type].map_u32 != 0)  /* u32 byte-swap */
		for (i = 0; i < bmap[type].map_u32; i++, offset += 4)
			btc_swap4(data[offset], data[offset + 1],
				  data[offset + 2], data[offset + 3]);
}
#endif

void _chk_btc_err(struct btc_t *btc, u8 type, u32 val)
{
	struct btc_cx *cx = &btc->cx;
	struct btc_dm *dm = &btc->dm;
	struct btc_wl_info *wl = &cx->wl;
	struct btc_bt_info *bt = &cx->bt;
	struct btc_bt_a2dp_desc *a2dp = &bt->link_info.a2dp_desc;
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct fbtc_cysta *pcysta = &pfwinfo->rpt_fbtc_cysta.finfo;
	u32 *presult = &pfwinfo->rpt_fbtc_nullsta.finfo.result[CXNULL_1][0];
	u32 empty_streak = 0, empty_streak_max = 0;
	u32 slot_pair, c_begin, c_end, cycle, s_id;

	if (wl->status.map.lps == BTC_LPS_RF_OFF ||
	    wl->status.map.rf_off)
		return;

	switch (type) {
	case BTC_DCNT_WL_FW_VER_MATCH:
		if ((wl->ver_info.fw_coex & 0xffffff00) !=
		     btc->chip->wlcx_desired) {
			wl->fw_ver_mismatch = 1;
			dm->error |= BTC_DMERR_WL_VER_MISMATCH;
		} else {
			wl->fw_ver_mismatch = 0;
			dm->error &= ~BTC_DMERR_WL_VER_MISMATCH;
		}
		break;
	case BTC_DCNT_NULL_TX_FAIL:
		if (presult[CXNULL_LATE] * BTC_NULLTX_FAIL_TH >
		    presult[CXNULL_OK])
			dm->cnt_dm[BTC_DCNT_NULL_TX_FAIL]++;
		else
			dm->cnt_dm[BTC_DCNT_NULL_TX_FAIL] = 0;

		if (dm->cnt_dm[BTC_DCNT_NULL_TX_FAIL] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_NULL1_TX_LATE;
		else
			dm->error &= ~BTC_DMERR_NULL1_TX_LATE;
		break;
	case BTC_DCNT_RPT_HANG:
		if (dm->cnt_dm[BTC_DCNT_RPT] == val && pfwinfo->rpt_en_map)
			dm->cnt_dm[BTC_DCNT_RPT_HANG]++;
		else
			dm->cnt_dm[BTC_DCNT_RPT_HANG] = 0;

		if (dm->cnt_dm[BTC_DCNT_RPT_HANG] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_WL_FW_HANG;
		else
			dm->error &= ~BTC_DMERR_WL_FW_HANG;

		dm->cnt_dm[BTC_DCNT_RPT] = val;
		break;
	case BTC_DCNT_BT_SLOT_FLOOD:
		if (!pfwinfo->rpt_fbtc_cysta.cinfo.valid ||
		    dm->cnt_dm[BTC_DCNT_CYCLE] == val)
		    return;

		if (val < BTC_CYCLE_SLOT_MAX || /* exclude old array data */
		    (dm->tdma_now.type != CXTDMA_AUTO &&
		     dm->tdma_now.type != CXTDMA_AUTO2 &&
		     dm->tdma_now.ext_ctrl != CXECTL_EXT)) {
			dm->error &= ~BTC_DMERR_BT_SLOT_FLOOD;
			dm->cnt_dm[BTC_DCNT_BT_SLOT_FLOOD] = 0;
			a2dp->no_empty_streak_2s = 0;
			a2dp->no_empty_streak_max = 0;
			return;
		}

		/* 1 cycle = 1 wl-slot + 1 bt-slot */
		slot_pair = BTC_CYCLE_SLOT_MAX/2;

		/* dm->cnt_dm[BTC_DCNT_CYCLE] --> last cycles */
		if (val - dm->cnt_dm[BTC_DCNT_CYCLE] > slot_pair)
			c_begin = val - slot_pair + 1;
		else
			c_begin = dm->cnt_dm[BTC_DCNT_CYCLE] + 1;

		c_end = val;

		for (cycle = c_begin; cycle <= c_end; cycle++) {
			s_id = ((cycle-1) % slot_pair)*2;

			if (pcysta->slot_step_time[s_id] >= dm->bt_slot_flood)
				dm->cnt_dm[BTC_DCNT_BT_SLOT_FLOOD]++;

			/* calculate no-A2DP-empty streak count */
			if (pcysta->a2dp_trx[s_id].empty_cnt == 0 &&
			    pcysta->a2dp_trx[s_id+1].empty_cnt == 0) {
				empty_streak++;
				if (empty_streak > empty_streak_max)
					empty_streak_max = empty_streak;
			} else {
				empty_streak = 0;
			}
		}

		a2dp->no_empty_streak_2s = empty_streak_max;

		if (a2dp->no_empty_streak_2s > a2dp->no_empty_streak_max)
			a2dp->no_empty_streak_max = a2dp->no_empty_streak_2s;

		if (dm->cnt_dm[BTC_DCNT_BT_SLOT_FLOOD])
			dm->error |= BTC_DMERR_BT_SLOT_FLOOD;
		break;
	case BTC_DCNT_CYCLE_HANG:
		if (dm->cnt_dm[BTC_DCNT_CYCLE] == val &&
		    (dm->tdma_now.type != CXTDMA_OFF ||
		     dm->tdma_now.ext_ctrl == CXECTL_EXT))
			dm->cnt_dm[BTC_DCNT_CYCLE_HANG]++;
		else
			dm->cnt_dm[BTC_DCNT_CYCLE_HANG] = 0;

		if (dm->cnt_dm[BTC_DCNT_CYCLE_HANG])
			dm->error |= BTC_DMERR_CYCLE_HANG;
		else
			dm->error &= ~BTC_DMERR_CYCLE_HANG;

		dm->cnt_dm[BTC_DCNT_CYCLE] = val;
		break;
	case BTC_DCNT_W1_HANG:
		if (dm->cnt_dm[BTC_DCNT_W1] == val &&
		    dm->tdma_now.type != CXTDMA_OFF)
			dm->cnt_dm[BTC_DCNT_W1_HANG]++;
		else
			dm->cnt_dm[BTC_DCNT_W1_HANG] = 0;

		if (dm->cnt_dm[BTC_DCNT_W1_HANG])
			dm->error |= BTC_DMERR_W1_HANG;
		else
			dm->error &= ~BTC_DMERR_W1_HANG;

		dm->cnt_dm[BTC_DCNT_W1] = val;
		break;
	case BTC_DCNT_B1_HANG:
		if (dm->cnt_dm[BTC_DCNT_B1] == val &&
		    dm->tdma_now.type != CXTDMA_OFF)
			dm->cnt_dm[BTC_DCNT_B1_HANG]++;
		else
			dm->cnt_dm[BTC_DCNT_B1_HANG] = 0;

		if (dm->cnt_dm[BTC_DCNT_B1_HANG] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_B1_HANG;
		else
			dm->error &= ~BTC_DMERR_B1_HANG;

		dm->cnt_dm[BTC_DCNT_B1] = val;
		break;
	case BTC_DCNT_E2G_HANG:
		if (dm->cnt_dm[BTC_DCNT_E2G] == val &&
		    dm->tdma_now.ext_ctrl == CXECTL_EXT)
			dm->cnt_dm[BTC_DCNT_E2G_HANG]++;
		else
			dm->cnt_dm[BTC_DCNT_E2G_HANG] = 0;

		if (dm->cnt_dm[BTC_DCNT_E2G_HANG] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_E2G_HANG;
		else
			dm->error &= ~BTC_DMERR_E2G_HANG;

		dm->cnt_dm[BTC_DCNT_E2G] = val;
		break;
	case BTC_DCNT_TDMA_NONSYNC:
		if (val != 0) /* if tdma not sync between drv/fw  */
			dm->cnt_dm[BTC_DCNT_TDMA_NONSYNC]++;
		else
			dm->cnt_dm[BTC_DCNT_TDMA_NONSYNC] = 0;

		if (dm->cnt_dm[BTC_DCNT_TDMA_NONSYNC] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_TDMA_NO_SYNC;
		else
			dm->error &= ~BTC_DMERR_TDMA_NO_SYNC;
		break;
	case BTC_DCNT_SLOT_NONSYNC:
		if (val != 0) /* if slot not sync between drv/fw  */
			dm->cnt_dm[BTC_DCNT_SLOT_NONSYNC]++;
		else
			dm->cnt_dm[BTC_DCNT_SLOT_NONSYNC] = 0;

		if (dm->cnt_dm[BTC_DCNT_SLOT_NONSYNC] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_SLOT_NO_SYNC;
		else
			dm->error &= ~BTC_DMERR_SLOT_NO_SYNC;
		break;
	case BTC_DCNT_BTCNT_HANG:
		val = cx->cnt_bt[BTC_BCNT_HIPRI_RX] +
		      cx->cnt_bt[BTC_BCNT_HIPRI_TX] +
		      cx->cnt_bt[BTC_BCNT_LOPRI_RX] +
		      cx->cnt_bt[BTC_BCNT_LOPRI_TX];

		if (val == 0)
			dm->cnt_dm[BTC_DCNT_BTCNT_HANG]++;
		else
			dm->cnt_dm[BTC_DCNT_BTCNT_HANG] = 0;
		break;
	case BTC_DCNT_BTTX_HANG:
		val = cx->cnt_bt[BTC_BCNT_LOPRI_TX];

		if (val == 0 && bt->link_info.slave_role)
			dm->cnt_dm[BTC_DCNT_BTTX_HANG]++;
		else
			dm->cnt_dm[BTC_DCNT_BTTX_HANG] = 0;

		if (dm->cnt_dm[BTC_DCNT_BTTX_HANG] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_BTTX_HANG;
		else
			dm->error &= ~BTC_DMERR_BTTX_HANG;
		break;
	case BTC_DCNT_WL_SLOT_DRIFT:
		if (val > dm->slot_now[CXST_W1].dur)
			val = val - dm->slot_now[CXST_W1].dur;
		else
			val = dm->slot_now[CXST_W1].dur - val;

		if (val >= BTC_CHK_WLSLOT_DRIFT_MAX)
			dm->cnt_dm[BTC_DCNT_WL_SLOT_DRIFT]++;
		else
			dm->cnt_dm[BTC_DCNT_WL_SLOT_DRIFT] = 0;

		if (dm->cnt_dm[BTC_DCNT_WL_SLOT_DRIFT] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_WL_SLOT_DRIFT;
		else
			dm->error &= ~BTC_DMERR_WL_SLOT_DRIFT;
		break;
	case BTC_DCNT_BT_SLOT_DRIFT:
		if (!btc->bt_req_len[wl->pta_req_mac])
			return;

		if (val > btc->bt_req_len[wl->pta_req_mac])
			val = val - btc->bt_req_len[wl->pta_req_mac];
		else
			val = btc->bt_req_len[wl->pta_req_mac] - val;

		if (val >= BTC_CHK_BTSLOT_DRIFT_MAX)
			dm->cnt_dm[BTC_DCNT_BT_SLOT_DRIFT]++;
		else
			dm->cnt_dm[BTC_DCNT_BT_SLOT_DRIFT] = 0;

		if (dm->cnt_dm[BTC_DCNT_BT_SLOT_DRIFT] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_BT_SLOT_DRIFT;
		else
			dm->error &= ~BTC_DMERR_BT_SLOT_DRIFT;
		break;
	case BTC_DCNT_WL_STA_NTFY:
		val = dm->cnt_notify[BTC_NCNT_WL_STA] -
		      dm->cnt_notify[BTC_NCNT_WL_STA_LAST];

		dm->cnt_notify[BTC_NCNT_WL_STA_LAST] =
				dm->cnt_notify[BTC_NCNT_WL_STA];

		if (val == 0 && wl->role_info.link_mode != BTC_WLINK_NOLINK)
			dm->cnt_dm[BTC_DCNT_WL_STA_NTFY]++;
		else
			dm->cnt_dm[BTC_DCNT_WL_STA_NTFY] = 0;

		if (dm->cnt_dm[BTC_DCNT_WL_STA_NTFY] >= BTC_CHK_ERR_TH)
			dm->error |= BTC_DMERR_WL_NO_STA_NTFY;
		else
			dm->error &= ~BTC_DMERR_WL_NO_STA_NTFY;
		break;
	}
}

static void _update_bt_report(struct btc_t *btc, u8 rpt_type, u8* pfinfo)
{
	struct rtw_hal_com_t *h = btc->hal;
	struct btc_cx *cx = &btc->cx;
	struct btc_dm *dm = &btc->dm;
	struct btc_wl_info *wl = &cx->wl;
	struct btc_bt_info *bt = &cx->bt;
	struct btc_bt_link_info *b = &bt->link_info;
	struct btc_bt_a2dp_desc *a2dp = &b->a2dp_desc;

	struct fbtc_btver* pver = (struct fbtc_btver*) pfinfo;
	struct fbtc_btscan* pscan = (struct fbtc_btscan*) pfinfo;
	struct fbtc_btafh* pafh = (struct fbtc_btafh*) pfinfo;
	struct fbtc_btdevinfo* pdev = (struct fbtc_btdevinfo*) pfinfo;
	u8 i, j, xmap, scan_update;
	u32 cnt = 0, cnt_le = 0;

	switch (rpt_type) {
	case BTC_RPT_TYPE_BT_VER:
		bt->ver_info.fw = pver->fw_ver;
		bt->ver_info.fw_coex = (pver->coex_ver & bMASKB0);
		bt->feature = pver->feature;

		if (bt->ver_info.fw_coex != btc->chip->btcx_desired) {
			bt->fw_ver_mismatch = 1;
			dm->error |= BTC_DMERR_BT_VER_MISMATCH;
		} else {
			bt->fw_ver_mismatch = 0;
			dm->error &= ~BTC_DMERR_BT_VER_MISMATCH;
		}
		break;
	case BTC_RPT_TYPE_BT_SCAN:
		scan_update = 1;
		for(i = CXSCAN_BG; i < CXSCAN_MAX1; i++) {
			hal_mem_cpy(h, &bt->scan_info[i], &pscan->para[i], 4);
			if ((pscan->type & BIT(i)) &&
			     pscan->para[i].win == 0 &&
			     pscan->para[i].intvl == 0)
			     scan_update = 0;
		}

		if (scan_update)
			bt->scan_info_update = 1;
		break;
	case BTC_RPT_TYPE_BT_AFH:
		if (pafh->map_type & BTC_RPT_BT_AFH_LEGACY) {
			hal_mem_cpy(h, &b->afh_map[0], pafh->afh_l, 4);
			hal_mem_cpy(h, &b->afh_map[4], pafh->afh_m, 4);
			hal_mem_cpy(h, &b->afh_map[8], pafh->afh_h, 2);
			cx->cnt_bt[BTC_BCNT_AFH_UPDATE]++;
			cx->cnt_bt[BTC_BCNT_AFH_CONFLICT] = 0;
			dm->error &= ~BTC_DMERR_AFH_CONFLICT;

			if (b->status.map.connect && wl->afh_info.en) {
				for(i = 0; i < 12; i++) {
					/* conflict channel map*/
					xmap = b->afh_map[i] & wl->ch_map[i];
					if (!xmap)
						continue;

					for (j = 0; j < 8; j++) {
						if (!(xmap & BIT(j)))
					    		continue;
						cnt++;
					}
				}

				cx->cnt_bt[BTC_BCNT_AFH_CONFLICT] = cnt;
				if (cnt)
					dm->error |= BTC_DMERR_AFH_CONFLICT;
			}
		}

		if (pafh->map_type & BTC_RPT_BT_AFH_LE) {
			hal_mem_cpy(h, &b->afh_map_le[0], pafh->afh_le_a, 4);
			hal_mem_cpy(h, &b->afh_map_le[4], pafh->afh_le_b, 1);
			cx->cnt_bt[BTC_BCNT_AFH_LE_UPDATE]++;
			cx->cnt_bt[BTC_BCNT_AFH_LE_CONFLICT] = 0;
			dm->error &= ~BTC_DMERR_AFH_LE_CONFLICT;

			if (b->status.map.ble_connect && wl->afh_info.en) {
				for(i = 0; i < 5; i++) {
					/* conflict channel map*/
					xmap = b->afh_map_le[i] &
					       wl->ch_map_le[i];
					if (!xmap)
						continue;

					for (j = 0; j < 8; j++) {
						if (!(xmap & BIT(j)))
					    		continue;
						cnt_le++;
					}
				}

				cx->cnt_bt[BTC_BCNT_AFH_LE_CONFLICT] = cnt_le;
				if (cnt_le)
					dm->error |= BTC_DMERR_AFH_LE_CONFLICT;
			}
		}

		break;
	case BTC_RPT_TYPE_BT_DEVICE:
		a2dp->device_name = pdev->dev_name;
		a2dp->vendor_id = pdev->vendor_id;
		a2dp->flush_time = pdev->flush_time;
		break;
	default:
		break;
	}
}

static u32 _chk_btc_report(struct btc_t *btc, u8 *prptbuf, u32 index)
{
	struct btc_dm *dm = &btc->dm;
	struct btf_fwinfo *pfwinfo = &btc->fwinfo;
	struct btc_wl_role_info *wl_rinfo = &btc->cx.wl.role_info;
	struct rtw_hal_com_t *hal = btc->hal;
	struct btc_rpt_cmn_info *pcinfo = NULL;
	struct btc_cx *cx = &btc->cx;
	struct btc_wl_info *wl = &cx->wl;
	struct btc_bt_info *bt = &cx->bt;
	struct fbtc_rpt_ctrl *prpt = NULL;
	struct fbtc_cysta *pcysta = NULL;
	struct fbtc_cynullsta *pcynull = NULL;
	struct fbtc_1tdma *ptdma = NULL;
	struct fbtc_slot_u16 *pslot = NULL;
	struct btc_chip_ops *ops = btc->chip->ops;
	u8 rpt_type = 0;
	u8 *rpt_content = NULL;
	u8 *pfinfo = NULL;
	u32 rpt_len = 0, i = 0;
	u32 val1, val2;

	if (!prptbuf) {
		pfwinfo->err[BTFRE_INVALID_INPUT]++;
		return 0;
	}

#if BTC_PLATFORM_BIG_ENDIAN
	_endian_translate(btc, BTC_FW_C2H, prptbuf);
#endif

	rpt_type = prptbuf[index];
	rpt_len = (prptbuf[index+2] << 8) + prptbuf[index+1];
	rpt_content = &prptbuf[index+3];

	switch (rpt_type) {
	case BTC_RPT_TYPE_CTRL:
		pcinfo = &pfwinfo->rpt_ctrl.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_ctrl.finfo);
		pcinfo->req_len = sizeof(struct fbtc_rpt_ctrl);
		pcinfo->req_fver = FCX_VER_BTCRPT;
		break;
	case BTC_RPT_TYPE_TDMA:
		pcinfo = &pfwinfo->rpt_fbtc_tdma.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_tdma.finfo);
		pcinfo->req_len = sizeof(struct fbtc_1tdma);
		pcinfo->req_fver = FCX_VER_TDMA;
		break;
	case BTC_RPT_TYPE_SLOT:
		pcinfo = &pfwinfo->rpt_fbtc_slots.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_slots.finfo);
		pcinfo->req_len = sizeof(struct fbtc_slots);
		pcinfo->req_fver = FCX_VER_SLOT;
		break;
	case BTC_RPT_TYPE_CYSTA:
		pcinfo = &pfwinfo->rpt_fbtc_cysta.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_cysta.finfo);
		pcinfo->req_len = sizeof(struct fbtc_cysta);
		pcinfo->req_fver = FCX_VER_CYSTA;
		break;
#ifdef BTC_FDDT_TRAIN_SUPPORT
	case BTC_RPT_TYPE_FDDT:
		pcinfo = &pfwinfo->rpt_fbtc_fddt.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_fddt.finfo);
		pcinfo->req_len = sizeof(struct fbtc_fddt_sta);
		pcinfo->req_fver = FCX_VER_FDDT;
		break;
#endif
#ifdef BTC_FW_STEP_DBG
	case BTC_RPT_TYPE_STEP:
		pcinfo = &pfwinfo->rpt_fbtc_step.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_step.finfo);
		pcinfo->req_len = sizeof(struct fbtc_steps);
		pcinfo->req_fver = FCX_VER_STEP;
		break;
#endif
	case BTC_RPT_TYPE_NULLSTA:
		pcinfo = &pfwinfo->rpt_fbtc_nullsta.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_nullsta.finfo);
		pcinfo->req_len = sizeof(struct fbtc_cynullsta);
		pcinfo->req_fver = FCX_VER_NULLSTA;
		break;
	case BTC_RPT_TYPE_MREG:
		pcinfo = &pfwinfo->rpt_fbtc_mregval.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_mregval.finfo);
		pcinfo->req_len = sizeof(struct fbtc_mreg_val);
		pcinfo->req_fver = FCX_VER_MREG;
		break;
	case BTC_RPT_TYPE_GPIO_DBG:
		pcinfo = &pfwinfo->rpt_fbtc_gpio_dbg.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_gpio_dbg.finfo);
		pcinfo->req_len = sizeof(struct fbtc_gpio_dbg);
		pcinfo->req_fver = FCX_VER_GPIODBG;
		break;
	case BTC_RPT_TYPE_BT_VER:
		pcinfo = &pfwinfo->rpt_fbtc_btver.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_btver.finfo);
		pcinfo->req_len = sizeof(struct fbtc_btver);
		pcinfo->req_fver = FCX_VER_BTVER;
		break;
	case BTC_RPT_TYPE_BT_SCAN:
		pcinfo = &pfwinfo->rpt_fbtc_btscan.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_btscan.finfo);
		pcinfo->req_len = sizeof(struct fbtc_btscan);
		pcinfo->req_fver = FCX_VER_BTSCAN;
		break;
	case BTC_RPT_TYPE_BT_AFH:
		pcinfo = &pfwinfo->rpt_fbtc_btafh.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_btafh.finfo);
		pcinfo->req_len = sizeof(struct fbtc_btafh);
		pcinfo->req_fver = FCX_VER_BTAFH;
		break;
	case BTC_RPT_TYPE_BT_DEVICE:
		pcinfo = &pfwinfo->rpt_fbtc_btdev.cinfo;
		pfinfo = (u8 *)(&pfwinfo->rpt_fbtc_btdev.finfo);
		pcinfo->req_len = sizeof(struct fbtc_btdevinfo);
		pcinfo->req_fver = FCX_VER_BTDEVINFO;
		break;
	default:
		pfwinfo->err[BTFRE_UNDEF_TYPE]++;
		return 0;
	}

	pcinfo->rsp_fver = *rpt_content;
	pcinfo->rx_len = rpt_len;
	pcinfo->rx_cnt++;

	if (rpt_len != pcinfo->req_len) {
		if (rpt_type < BTC_RPT_TYPE_MAX)
			pfwinfo->len_mismch |= BIT(rpt_type);
		else
			pfwinfo->len_mismch |= BIT31;

		pcinfo->valid = 0;
		return 0;
	} else if (pcinfo->req_fver != pcinfo->rsp_fver) {
		if (rpt_type < BTC_RPT_TYPE_MAX)
			pfwinfo->fver_mismch |= BIT(rpt_type);
		else
			pfwinfo->fver_mismch |= BIT31;
		pcinfo->valid = 0;
		return 0;
	} else if (!pfinfo || !rpt_content || !pcinfo->req_len) {
		pfwinfo->err[BTFRE_EXCEPTION]++;
		pcinfo->valid = 0;
		return 0;
	}

	hal_mem_cpy(hal, (void *)pfinfo, (void *)rpt_content, pcinfo->req_len);
	pcinfo->valid = 1;

	switch (rpt_type) {
	case BTC_RPT_TYPE_CTRL:
		prpt = &pfwinfo->rpt_ctrl.finfo;
		pfwinfo->rpt_en_map = prpt->rpt_info.en;
		wl->ver_info.fw_coex = prpt->rpt_info.cx_ver;
		wl->ver_info.fw = prpt->rpt_info.fw_ver;

		val1 = sizeof(struct btc_gnt_ctrl);
		for (i = HW_PHY_0; i < HW_PHY_MAX; i++)
			hal_mem_cpy(hal, (void *)&dm->gnt_val[i],
				    (void *)&prpt->gnt_val[i][0], val1);

		cx->cnt_bt[BTC_BCNT_HIPRI_TX] = prpt->bt_cnt[BTC_BCNT_HI_TX];
		cx->cnt_bt[BTC_BCNT_HIPRI_RX] = prpt->bt_cnt[BTC_BCNT_HI_RX];
		cx->cnt_bt[BTC_BCNT_LOPRI_TX] = prpt->bt_cnt[BTC_BCNT_LO_TX];
		cx->cnt_bt[BTC_BCNT_LOPRI_RX] = prpt->bt_cnt[BTC_BCNT_LO_RX];
		cx->cnt_bt[BTC_BCNT_POLUT] = prpt->bt_cnt[BTC_BCNT_POLLUTED];

		val1 = pfwinfo->event[BTF_EVNT_RPT];
		_chk_btc_err(btc, BTC_DCNT_BTCNT_HANG, 0);
		_chk_btc_err(btc, BTC_DCNT_RPT_HANG, val1);
		_chk_btc_err(btc, BTC_DCNT_WL_FW_VER_MATCH, 0);
		_chk_btc_err(btc, BTC_DCNT_BTTX_HANG, 0);
		_chk_btc_err(btc, BTC_DCNT_WL_STA_NTFY, 0);
		break;
	case BTC_RPT_TYPE_TDMA:
		ptdma = &pfwinfo->rpt_fbtc_tdma.finfo;
		_chk_btc_err(btc, BTC_DCNT_TDMA_NONSYNC,
			     _tdma_cmp(&dm->tdma_now, &ptdma->tdma));

		break;
	case BTC_RPT_TYPE_SLOT:
		pslot = pfwinfo->rpt_fbtc_slots.finfo.slot;
		for (i = 0; i < CXST_MAX; i++) {
			val2 = (pslot[i].cxtbl_h16 << 16) + pslot[i].cxtbl_l16;
			if (dm->slot_now[i].dur != pslot[i].dur ||
			    dm->slot_now[i].cxtype != pslot[i].cxtype ||
			    dm->slot_now[i].cxtbl != val2) {
				val1 = 1;
			} else {
				val1 = 0;
			}

			_chk_btc_err(btc, BTC_DCNT_SLOT_NONSYNC, val1);

			if (dm->error & BTC_DMERR_SLOT_NO_SYNC)
				break; /* exit if 1 slot no-sync error occur*/
		}
		break;
	case BTC_RPT_TYPE_CYSTA:
		if (dm->fddt_train == BTC_FDDT_ENABLE)
			break;

		pcysta = &pfwinfo->rpt_fbtc_cysta.finfo;

		if (dm->tdma_now.type != CXTDMA_OFF) {
			/* Check diff time between real WL slot and W1 slot */
			_chk_btc_err(btc, BTC_DCNT_WL_SLOT_DRIFT,
				     pcysta->cycle_time.tavg[CXT_WL]);

			/* Check Leak-AP */
			val1 = pcysta->leak_slot.cnt_rximr * BTC_LEAK_AP_TH;
			val2 = pcysta->slot_cnt[CXST_LK];
			if (val2 >= (BTC_CYSTA_CHK_PERIOD * 2) &&
			    dm->tdma_now.rxflctrl && val1 > val2)
				dm->leak_ap = 1;
		} else if (dm->tdma_now.ext_ctrl == CXECTL_EXT &&
			   wl_rinfo->link_mode != BTC_WLINK_2G_GC) {
			/* Check diff between real BT slot and EBT/E5G slot */
			_chk_btc_err(btc, BTC_DCNT_BT_SLOT_DRIFT,
				     pcysta->cycle_time.tavg[CXT_BT]);

			/* Check bt slot length for P2P mode*/
			val1 = pcysta->a2dp_ept.cnt_timeout * BTC_SLOT_REQ_TH;
			val2 = pcysta->a2dp_ept.cnt;
			if (pcysta->cycles >= BTC_CYSTA_CHK_PERIOD &&
			    val1 > val2)
				dm->slot_req_more = 1;
			else if (bt->link_info.status.map.connect == 0)
				dm->slot_req_more = 0;
		}

		_chk_btc_err(btc, BTC_DCNT_E2G_HANG,pcysta->slot_cnt[CXST_E2G]);
		_chk_btc_err(btc, BTC_DCNT_W1_HANG, pcysta->slot_cnt[CXST_W1]);
		_chk_btc_err(btc, BTC_DCNT_B1_HANG, pcysta->slot_cnt[CXST_B1]);

		/* "BT_SLOT_FLOOD" error-check MUST before "CYCLE_HANG" */
		_chk_btc_err(btc, BTC_DCNT_BT_SLOT_FLOOD, (u32)pcysta->cycles);
		_chk_btc_err(btc, BTC_DCNT_CYCLE_HANG, (u32)pcysta->cycles);
		break;
	case BTC_RPT_TYPE_NULLSTA:
		pcynull = &pfwinfo->rpt_fbtc_nullsta.finfo;
		if (dm->fddt_train == BTC_FDDT_ENABLE ||
		    pcynull->result[CXNULL_1][CXNULL_TX] <
		    BTC_NULLTX_CHK_PERIOD)
			break;

		_chk_btc_err(btc, BTC_DCNT_NULL_TX_FAIL, 0);
		break;
	case BTC_RPT_TYPE_MREG:
		if (!ops || !ops->get_reg_status)
			break;
		ops->get_reg_status(btc, BTC_CSTATUS_BB_GNT_MUX_MON, (void*)&i);
		if (dm->wl_btg_rx == BTC_BB_GNT_FWCTRL)
			dm->wl_btg_rx_rb = BTC_BB_GNT_FWCTRL;
		else
			dm->wl_btg_rx_rb = i;

		ops->get_reg_status(btc, BTC_CSTATUS_BB_PRE_AGC_MON, (void*)&i);
		if (dm->wl_pre_agc == BTC_BB_PRE_AGC_FWCTRL)
			dm->wl_pre_agc_rb = BTC_BB_PRE_AGC_FWCTRL;
		else
			dm->wl_pre_agc_rb = (u8)i;
		break;
	case BTC_RPT_TYPE_BT_VER:
	case BTC_RPT_TYPE_BT_SCAN:
	case BTC_RPT_TYPE_BT_AFH:
	case BTC_RPT_TYPE_BT_DEVICE:
		_update_bt_report(btc, rpt_type, pfinfo);
		break;
	}

	return (rpt_len + BTC_RPT_HDR_SIZE);
}

static void _append_tdma(struct btc_t *btc, bool force_exec)
{
	struct btc_dm *dm = &btc->dm;
	struct btf_tlv *tlv = NULL;

	if (btc->policy_len >= BTC_POLICY_MAXLEN) {
		PHL_INFO("[BTC], %s(): buff overflow!\n", __func__);
		return;
	}

	if (!force_exec && !_tdma_cmp(&dm->tdma, &dm->tdma_now)) {
		PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_,
			  "[BTC], %s(): tdma no change!\n", __func__);
		return;
	}

	tlv = (struct btf_tlv *)&btc->policy[btc->policy_len];
	tlv->type = CXPOLICY_TDMA;
	tlv->ver = FCX_VER_TDMA;
	tlv->len = sizeof(struct fbtc_tdma);

	_tdma_cpy(&tlv->val[0], &dm->tdma); /* put tdma-descriptor to h2c-buf */
	btc->policy_len += (3 + tlv->len); /* update total length */

	PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_,
		 "[BTC], %s: len=%d, type:%d, rxfctrl=%d, txfctrl=%d, rsvd=%d, "
		 "leak_n=%d, ext_ctrl=%d, rxflctrl_role=0x%x, opt_ctrl=0x%x\n",
		 __func__, btc->policy_len,
		 dm->tdma.type, dm->tdma.rxflctrl, dm->tdma.txflctrl,
		 dm->tdma.rsvd, dm->tdma.leak_n, dm->tdma.ext_ctrl,
		 dm->tdma.rxflctrl_role, dm->tdma.option_ctrl);
}

static void _append_slot(struct btc_t *btc, bool force_exec)
{
	struct btc_dm *dm = &btc->dm;
	struct btf_tlv *tlv = NULL;
	u8 i, cnt = 0;
	u16 len;

	for (i = 0; i < CXST_MAX; i++) {
		if (!force_exec && !_slot_cmp(&dm->slot[i], &dm->slot_now[i]))
			continue;

		if (btc->policy_len >= BTC_POLICY_MAXLEN) {
			PHL_INFO("[BTC], %s(): buff overflow!\n", __func__);
			break;
		}

		len = btc->policy_len;

		if (cnt == 0) {
			tlv = (struct btf_tlv *)&btc->policy[len];
			tlv->type = CXPOLICY_SLOT;
			tlv->ver = FCX_VER_SLOT;
			tlv->len = sizeof(struct fbtc_slot) + 1; /*u8 + 1-slot*/
			len += 3;
		}

		btc->policy[len] = i; /* slot-id */

		/* put tdma-descriptor to h2c-buf */
		btc->policy[len+1] = (u8)(dm->slot[i].dur & bMASKB0);
		btc->policy[len+2] = (u8)((dm->slot[i].dur & bMASKB1) >> 8);

		btc->policy[len+3] = (u8)(dm->slot[i].cxtype & bMASKB0);
		btc->policy[len+4] = (u8)((dm->slot[i].cxtype & bMASKB1) >> 8);

		btc->policy[len+5] = (u8)(dm->slot[i].cxtbl & bMASKB0);
		btc->policy[len+6] = (u8)((dm->slot[i].cxtbl & bMASKB1) >> 8);
		btc->policy[len+7] = (u8)((dm->slot[i].cxtbl & bMASKB2) >> 16);
		btc->policy[len+8] = (u8)((dm->slot[i].cxtbl & bMASKB3) >> 24);

		len += tlv->len;

		PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_,
			  "[BTC], %s: policy_len=%d, slot-%d:"
			  " dur=%d, type=%d, table=0x%08x\n",
			  __func__, btc->policy_len, i, dm->slot[i].dur,
			  dm->slot[i].cxtype, dm->slot[i].cxtbl);
		cnt++;
		btc->policy_len = len; /* update total length */
	}

	if (cnt > 0)
		PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_,
			 "[BTC], %s: slot update (cnt=%d, len=%d)!!\n",
			 __func__, cnt, btc->policy_len);
}

/*
 * extern functions
 */

void hal_btc_fw_chk_struct(struct btc_t *btc)
{
	struct btc_dm *dm = &btc->dm;
	const struct btc_byte_map_desc *bmap = NULL;
	u8 i = 0;
	u16 sz = 0, map_sz = 0;

	/* cehck h2c structure */
	bmap = bmap_h2c;
	for (i = 0; i < CXDRVINFO_MAX; i++) {

		switch(i) {
		case CXDRVINFO_INIT:
			sz = sizeof(struct btc_init_info);
			break;
		case CXDRVINFO_ROLE:
			sz = sizeof(struct btc_wl_role_info);
			break;
		case CXDRVINFO_CTRL:
			sz = sizeof(struct btc_ctrl);
			break;
		case CXDRVINFO_TRX:
			sz = sizeof(struct btc_trx_info);
			break;
		case CXDRVINFO_TXPWR:
			sz = sizeof(u8);
			break;
		case CXDRVINFO_FDDT:
			sz = sizeof(struct btc_fddt_train_info);
			break;
		}

		map_sz = bmap[i].map_u8 + bmap[i].map_u16 * 2 +
			 bmap[i].map_u32 * 4;

		if (sz % 4 != 0 && i != CXDRVINFO_TXPWR)
			dm->error |= BTC_DMERR_H2C_STRUCT_INVALID;

		if (sz != map_sz)
			dm->error |= BTC_DMERR_H2C_BMAP_MISMATCH;
	}

	/* cehck c2h structure */
	bmap = bmap_c2h;
	for (i = 0; i < BTC_RPT_TYPE_MAX; i++) {

		switch(i) {
		case BTC_RPT_TYPE_CTRL:
			sz = sizeof(struct fbtc_rpt_ctrl);
			break;
		case BTC_RPT_TYPE_TDMA:
			sz = sizeof(struct fbtc_1tdma);
			break;
		case BTC_RPT_TYPE_SLOT:
			sz = sizeof(struct fbtc_slots);
			break;
		case BTC_RPT_TYPE_CYSTA:
			sz = sizeof(struct fbtc_cysta);
			break;
		case BTC_RPT_TYPE_STEP:
			sz = sizeof(struct fbtc_steps);
			break;
		case BTC_RPT_TYPE_NULLSTA:
			sz = sizeof(struct fbtc_cynullsta);
			break;
		case BTC_RPT_TYPE_FDDT:
			sz = sizeof(struct fbtc_fddt_sta);
			break;
		case BTC_RPT_TYPE_MREG:
			sz = sizeof(struct fbtc_mreg_val);
			break;
		case BTC_RPT_TYPE_GPIO_DBG:
			sz = sizeof(struct fbtc_gpio_dbg);
			break;
		case BTC_RPT_TYPE_BT_VER:
			sz = sizeof(struct fbtc_btver);
			break;
		case BTC_RPT_TYPE_BT_SCAN:
			sz = sizeof(struct fbtc_btscan);
			break;
		case BTC_RPT_TYPE_BT_AFH:
			sz = sizeof(struct fbtc_btafh);
			break;
		case BTC_RPT_TYPE_BT_DEVICE:
			sz = sizeof(struct fbtc_btdevinfo);
			break;
		case BTC_RPT_TYPE_TEST:
			sz = 0;
			break;
		}

		map_sz = bmap[i].map_u8 + bmap[i].map_u16 * 2 +
			 bmap[i].map_u32 * 4;

		if (sz % 4 != 0)
			dm->error |= BTC_DMERR_C2H_STRUCT_INVALID;

		if (sz != map_sz)
			dm->error |= BTC_DMERR_C2H_BMAP_MISMATCH;
	}
}

void hal_btc_fw_set_rpt(struct btc_t *btc, u32 rpt_map, u32 rpt_state)
{
	struct btf_fwinfo* fwinfo = &btc->fwinfo;
	struct btc_wl_smap *wl_smap = &btc->cx.wl.status.map;
	u8 data[7] = {0};
	u16 len = 0;
	u32 val = 0;

	if (rpt_state > 2)
		return;

	/* Skip enable report if enter IPS/LPS/RF-Off  */
	if ((wl_smap->rf_off || wl_smap->lps != BTC_LPS_OFF) &&
	     rpt_state != 0)
	     return;

	if (rpt_state == 2) /* for multi-bit operation */
		val = rpt_map;
	else if (rpt_state == 1)
		val = fwinfo->rpt_en_map | rpt_map;
	else if (rpt_state == 0)
		val = fwinfo->rpt_en_map & (~rpt_map);

	if ((val == fwinfo->rpt_en_map) && (!(rpt_map & RPT_EN_MREG)))
		return;

	data[0] = SET_REPORT_EN;  /* type */
	data[1] = FCX_VER_BTCRPT; /* ver */
	data[2] = 4;  /*  length */

	/* report-enable data */
	data[3] = (u8)(val & bMASKB0);
	data[4] = (u8)((val & bMASKB1) >> 8);
	data[5] = (u8)((val & bMASKB2) >> 16);
	data[6] = (u8)((val & bMASKB3) >> 24);

	len = data[2] + 3;
	if (_send_fw_cmd(btc, SET_REPORT_EN, data, len))
		fwinfo->rpt_en_map = val;
}

void hal_btc_fw_set_slots(struct btc_t *btc)
{
	struct rtw_hal_com_t *h = btc->hal;
	struct btc_dm *dm = &btc->dm;
	struct btf_tlv *tlv = NULL;
	u8 *ptr = NULL;
	u8 i;
	u16 len = 0, n = 0;

	len = sizeof(btc->dm.slot) + sizeof(*tlv) - 1;
	tlv = hal_mem_alloc(h, len);
	if (!tlv)
		return;

	tlv->type = SET_SLOT_TABLE;
	tlv->ver = FCX_VER_SLOT;
	tlv->len = ARRAY_SIZE(dm->slot); /* total slot count: CXST_MAX*/
	ptr = &tlv->val[0];

	for (i = 0; i < tlv->len; i++) {
		n = i * sizeof(struct fbtc_slot);
		ptr[n+0] = (u8)(dm->slot[i].dur & bMASKB0);
		ptr[n+1] = (u8)((dm->slot[i].dur & bMASKB1) >> 8);

		ptr[n+2] = (u8)(dm->slot[i].cxtype & bMASKB0);
		ptr[n+3] = (u8)((dm->slot[i].cxtype & bMASKB1) >> 8);

		ptr[n+4] = (u8)(dm->slot[i].cxtbl & bMASKB0);
		ptr[n+5] = (u8)((dm->slot[i].cxtbl & bMASKB1) >> 8);
		ptr[n+6] = (u8)((dm->slot[i].cxtbl & bMASKB2) >> 16);
		ptr[n+7] = (u8)((dm->slot[i].cxtbl & bMASKB3) >> 24);
	}

	_send_fw_cmd(btc, SET_SLOT_TABLE, (u8*)tlv, len);
	hal_mem_free(h, (void*)tlv, len);
}

/* set RPT_EN_MREG = 0 to stop 2s monitor timer in WL FW,
 * before SET_MREG_TABLE, and set RPT_EN_MREG = 1 after
 * SET_MREG_TABLE
 */
void hal_btc_fw_set_monreg(struct btc_t *btc)
{
	struct rtw_hal_com_t *h = btc->hal;
	struct btf_tlv *tlv = NULL;
	u8 *ptr = NULL;
	u8 n, ulen, i;
	u16 len = 0;

	if (btc->chip->mon_reg_num > CXMREG_MAX) {
		PHL_TRACE(COMP_PHL_BTC, _PHL_ERR_,
			  "[BTC], mon reg count %d > %d\n",
			  btc->chip->mon_reg_num , CXMREG_MAX);
		return;
	}

	ulen = sizeof(struct fbtc_mreg);
	len = (ulen * btc->chip->mon_reg_num) + sizeof(*tlv) - 1;
	tlv = hal_mem_alloc(h, len);
	if (!tlv)
		return;

	tlv->type = RPT_EN_MREG;
	tlv->ver = FCX_VER_MREG;
	tlv->len = btc->chip->mon_reg_num; /*total moniter register count */
	ptr = &tlv->val[0];

	for (i = 0; i < tlv->len; i++) {
		n = i * ulen;
		ptr[n+0] = (u8)(btc->chip->mon_reg[i].type & bMASKB0);
		ptr[n+1] = (u8)((btc->chip->mon_reg[i].type & bMASKB1) >> 8);

		ptr[n+2] = (u8)(btc->chip->mon_reg[i].bytes & bMASKB0);
		ptr[n+3] = (u8)((btc->chip->mon_reg[i].bytes & bMASKB1) >> 8);

		ptr[n+4] = (u8)(btc->chip->mon_reg[i].offset & bMASKB0);
		ptr[n+5] = (u8)((btc->chip->mon_reg[i].offset & bMASKB1) >> 8);
		ptr[n+6] = (u8)((btc->chip->mon_reg[i].offset & bMASKB2) >> 16);
		ptr[n+7] = (u8)((btc->chip->mon_reg[i].offset & bMASKB3) >> 24);
	}

	_send_fw_cmd(btc, SET_MREG_TABLE, (u8 *)tlv, len);
	hal_mem_free(h, (void *)tlv, len);
	_set_fw_rpt(btc, RPT_EN_MREG, 1);
}

bool hal_btc_fw_set_policy(struct btc_t *btc, bool force_exec, u16 policy_type,
		   const char* action)
{
	struct btc_dm *dm = &btc->dm;
	u32 len = _os_strlen((u8 *)action);
	u8 *btc_ctrl_lps = &btc->hal->btc_ctrl.lps;

	_act_cpy(dm->run_action, (char*)action, len);
	_update_dm_step(btc, action);
	_update_dm_step(btc, id_to_str(BTC_STR_POLICY, (u32)policy_type));

	btc->policy_len = 0; /* clear length before append */
	btc->policy_type = policy_type;
	_append_tdma(btc, force_exec);
	_append_slot(btc, force_exec);

	if (btc->policy_len == 0) {
		return false;
	} else if (btc->policy_len >= BTC_POLICY_MAXLEN) {
		dm->error |= BTC_DMERR_H2C_BUF_OVER;
		return false;
	}

	PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_,
		  "[BTC], %s(): action=%s -> policy type/len: 0x%04x/%d\n",
		  __func__, action, policy_type, btc->policy_len);

	*btc_ctrl_lps = (dm->tdma.rxflctrl == CXFLC_NULLP? 1 : 0);

	if (*btc_ctrl_lps == 1)
		hal_btc_notify_ps_tdma(btc, *btc_ctrl_lps);

	if (_send_fw_cmd(btc, SET_CX_POLICY, btc->policy, btc->policy_len)) {
		_tdma_cpy(&dm->tdma_now, &dm->tdma);
		_slots_cpy(dm->slot_now, dm->slot);
	}

	if (*btc_ctrl_lps == 0)
		hal_btc_notify_ps_tdma(btc, *btc_ctrl_lps);

	return true;
}

void hal_btc_fw_set_gpio_dbg(struct btc_t *btc, u8 type, u32 val)
{
	u8 data[7] = {0};
	u16 len = 0;

	if (type >= CXDGPIO_MAX)
		return;

	PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_, "[BTC], %s()\n", __func__);

	data[0] = type;  /* type */
	data[1] = FCX_VER_GPIODBG; /* fver */

	switch(type) {
	case CXDGPIO_EN_MAP:
		data[2] = 4; /* data length */
		data[3] = (u8)(val & bMASKB0);
		data[4] = (u8)((val & bMASKB1) >> 8);
		data[5] = (u8)((val & bMASKB2) >> 16);
		data[6] = (u8)((val & bMASKB3) >> 24);
		break;
	case CXDGPIO_MUX_MAP:
		data[2] = 2; /* data length */
		data[3] = (u8)(val & bMASKB0);
		data[4] = (u8)((val & bMASKB1) >> 8);
		break;
	default:
		return;
	}

	len = data[2] + 3;

	_send_fw_cmd(btc, SET_GPIO_DBG, data, len);
}

void hal_btc_fw_set_drv_info(struct btc_t *btc, u8 type)
{
	struct rtw_hal_com_t *h = btc->hal;
	struct btc_wl_info *wl = &btc->cx.wl;
	struct btc_dm *dm = &btc->dm;
	struct btc_rf_trx_para rf_para = dm->rf_trx_para;
	u8 buf[256] = {0};
	u8 sz = 0, n = 0;

	if (type >= CXDRVINFO_MAX)
		return;

	switch (type) {
	case CXDRVINFO_INIT:
		n = sizeof(dm->init_info);
		sz = n + 3;

		if (sz > sizeof(buf))
			return;

		buf[0] = CXDRVINFO_INIT;
		buf[1] = FCX_VER_INIT;
		buf[2] = n;
		hal_mem_cpy(h, (void *)&buf[3], &dm->init_info, n);
		break;
	case CXDRVINFO_ROLE:
		n = sizeof(wl->role_info);
		sz = n + 3;

		if (sz > sizeof(buf))
			return;

		buf[0] = CXDRVINFO_ROLE;
		buf[1] = FCX_VER_ROLE;
		buf[2] = n;
		hal_mem_cpy(h, (void *)&buf[3], &wl->role_info, n);
		break;
	case CXDRVINFO_CTRL:
		n = sizeof(btc->ctrl);
		sz = n + 3;

		if (sz > sizeof(buf))
			return;

		buf[0] = CXDRVINFO_CTRL;
		buf[1] = FCX_VER_CTRL;
		buf[2] = n;
		hal_mem_cpy(h, (void *)&buf[3], &btc->ctrl, n);
		break;
	case CXDRVINFO_TRX:
		n = sizeof(dm->trx_info);
		sz = n + 3;

		if (sz > sizeof(buf))
			return;

		buf[0] = CXDRVINFO_TRX;
		buf[1] = FCX_VER_TRX;
		buf[2] = n;

		dm->trx_info.tx_power = rf_para.wl_tx_power & 0xff;
		dm->trx_info.rx_gain = rf_para.wl_rx_gain & 0xff;
		dm->trx_info.bt_tx_power = rf_para.bt_tx_power & 0xff;
		dm->trx_info.bt_rx_gain = rf_para.bt_rx_gain & 0xff;
		dm->trx_info.cn = wl->cn_report;
		dm->trx_info.nhm = wl->nhm.pwr;

		hal_mem_cpy(h, (void *)&buf[3], &dm->trx_info, n);
		break;
	case CXDRVINFO_TXPWR:
		n = 1;
		sz = n + 3;

		if (sz > sizeof(buf))
			return;

		buf[0] = CXDRVINFO_TXPWR;
		buf[1] = FCX_VER_TRX;
		buf[2] = n;
		buf[3] = (u8)(dm->rf_trx_para.wl_tx_power & 0xff);
		break;
	case CXDRVINFO_FDDT:
		n = sizeof(dm->fddt_info.train);
		sz = n + 3;

		if (sz > sizeof(buf))
			return;

		buf[0] = CXDRVINFO_FDDT;
		buf[1] = FCX_VER_FDDT;
		buf[2] = n;
		hal_mem_cpy(h, (void *)&buf[3], &dm->fddt_info.train, n);
		break;
	default:
		return;
	}

#if BTC_PLATFORM_BIG_ENDIAN
	_endian_translate(btc, BTC_FW_H2C, buf);
#endif
	_send_fw_cmd(btc, SET_DRV_INFO, buf, sz);
}

void hal_btc_fw_parse_rpt(struct btc_t *btc, u8 *data, u32 len)
{
	u32 index = 0, rpt_len = 0;

	if (!len || !data)
		return;

	while (1) {
		if (index+2 >= RTW_PHL_BTC_FWINFO_BUF)
			break;
		/* At least 3 bytes: type(1) & len(2) */
		rpt_len = (data[index+2] << 8) + data[index+1];
		if ((index + rpt_len + BTC_RPT_HDR_SIZE) > len)
			break;

		rpt_len = _chk_btc_report(btc, data, index);
		if (!rpt_len)
			break;
		index += rpt_len;
	}
}

#endif

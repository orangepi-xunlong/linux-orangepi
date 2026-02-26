/******************************************************************************
 *
 * Copyright(c) 2016 - 2019 Realtek Corporation.
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
#define _RTL8852B_BTC_C_

#include "../../hal_headers_le.h"
#include "../hal_btc.h"

#ifdef CONFIG_BTCOEX
#if defined (CONFIG_RTL8852B) || defined (CONFIG_RTL8852BP) || defined (CONFIG_RTL8851B)

#include "btc_8852b.h"

/* WL rssi threshold in % (dbm = % - 110)
 * array size limit by BTC_WL_RSSI_THMAX
 * BT rssi threshold in % (dbm = % - 100)
 * array size limit by BTC_BT_RSSI_THMAX
 */

static const u8 btc_8852b_wl_rssi_thres[BTC_WL_RSSI_THMAX] = {70, 60, 50, 40};
static const u8 btc_8852b_bt_rssi_thres[BTC_BT_RSSI_THMAX] = {50, 40, 30, 20};

static const struct btc_rf_cfg btc_8852b_rf_0[] = {
	{0xef, 0x1000}, {0x33, 0x0},  {0x3f, 0x15}, {0x33, 0x1},  {0x3f, 0x17},
	{0x33, 0x2},    {0x3f, 0x15}, {0x33, 0x3},  {0x3f, 0x17}, {0xef, 0x0}};
static const struct btc_rf_cfg btc_8852b_rf_1[] = {
	{0xef, 0x1000}, {0x33, 0x0},  {0x3f, 0x15}, {0x33, 0x1},  {0x3f, 0x5},
	{0x33, 0x2},    {0x3f, 0x15}, {0x33, 0x3},  {0x3f, 0x5},  {0xef, 0x0}};
static const struct btc_rf_cfg btc_8852bp_rf_0[] = {
	{0xef, 0x1000}, {0x33, 0x0},  {0x3f, 0x15}, {0x33, 0x1},  {0x3f, 0x17},
	{0x33, 0x2}, 	{0x3f, 0x15}, {0x33, 0x3},  {0x3f, 0x17}, {0xef, 0x0},
	{0xef, 0x4000}, {0x33, 0x7},  {0x3e, 0x0},  {0x3f, 0x700},{0x33, 0x6},
	{0x3e, 0x0},    {0x3f, 0x700},{0x33, 0xf},  {0x3e, 0x0},  {0x3f, 0x700},
	{0x33, 0xe},    {0x3e, 0x0},  {0x3f, 0x700},{0x33, 0x17}, {0x3e, 0x0},
	{0x3f, 0x700}, 	{0x33, 0x16}, {0x3e, 0x0},  {0x3f, 0x700},{0xef, 0x0},
	{0xee, 0x10}, 	{0x33, 0x0},  {0x3f, 0x3},  {0x33, 0x1},  {0x3f, 0x3},
	{0x33, 0x2}, 	{0x3f, 0x3},  {0xee, 0x0}};
static const struct btc_rf_cfg btc_8852bp_rf_1[] = {
	{0xef, 0x1000}, {0x33, 0x0},  {0x3f, 0x15}, {0x33, 0x1},    {0x3f, 0x5},
	{0x33, 0x2}, 	{0x3f, 0x15}, {0x33, 0x3},  {0x3f, 0x5},    {0xef, 0x0},
	{0xef, 0x4000}, {0x33, 0x7},  {0x3e, 0x0},  {0x3f, 0xa0700},{0x33, 0x6},
	{0x3e, 0x0}, {0x3f, 0xa0700}, {0x33, 0xf},  {0x3e, 0x0},{0x3f, 0xa0700},
	{0x33, 0xe},    {0x3e, 0x0},  {0x3f, 0xa0700},{0x33, 0x17}, {0x3e, 0x0},
	{0x3f, 0xa0700},{0x33, 0x16}, {0x3e, 0x0},  {0x3f, 0xa0700},{0xef, 0x0},
	{0xee, 0x10}, 	{0x33, 0x0},  {0x3f, 0x2},  {0x33, 0x1},    {0x3f, 0x2},
	{0x33, 0x2}, 	{0x3f, 0x2},  {0xee, 0x0}};
static const struct btc_rf_cfg btc_8851b_rf_0[] = {{0x2, 0x0}};
static const struct btc_rf_cfg btc_8851b_rf_1[] = {{0x2, 0x1}};

static struct btc_chip_ops btc_8852b_ops = {
	_8852b_rfe_type,
	_8852b_init_cfg,
	_8852b_wl_tx_power,
	_8852b_wl_rx_gain,
	_8852b_wl_btg_standby,
	_8852b_wl_req_mac,
	_8852b_get_reg_status,
	_8852b_bt_rssi
};

/* Set  WL/BT periodical moniter reg, Max size: CXMREG_MAX*/
/*
 * =========== CO-Rx AGC related monitor ===========
 * Co-Rx condition : WL RSSI < -35dBm (BB defined)
 * If WL RSSI < -35dBm --> 0x4aa4[18] = 0;
 * 0x4aa4[18] = 0: Not to adjust WL Rx AGC by GNT_BT_Rx.
 *		    To avoid WL Rx gain change.
 * 0x4aa4[18] = 1: When WL RSSI too strong, out of Co-Rx range
 *		    give up the Co-Rx WL enhancement.
 * 0x476c[31:24] = 0x20 (dBm); LNA6 op1dB
 * 0x4778[7:0]= 0x30 (dBm); TIA index0 LNA6 op1dB
 */
static struct fbtc_mreg btc_8852b_mon_reg[] = {
	{REG_MAC, 4, 0xda24},
	{REG_MAC, 4, 0xda28},
	{REG_MAC, 4, 0xda2c},
	{REG_MAC, 4, 0xda30},
	{REG_MAC, 4, 0xda4c},
	{REG_MAC, 4, 0xda10},
	{REG_MAC, 4, 0xda20},
	{REG_MAC, 4, 0xda34},
	{REG_MAC, 4, 0xcef4},
	{REG_MAC, 4, 0x8424},
	{REG_MAC, 4, 0xd200},
	{REG_MAC, 4, 0xd220},
	{REG_BB, 4, 0x980},
	{REG_BB, 4, 0x4aa4},
	{REG_BB, 4, 0x4778},
	{REG_BB, 4, 0x476c},
	/*{REG_RF, 2, 0x2},*/ /* for RF, bytes->path, 1:A, 2:B... */
	/*{REG_RF, 2, 0x18},*/
	/*{REG_BT_RF, 2, 0x9},*/
	/*{REG_BT_MODEM, 2, 0xa} */
};
static struct fbtc_mreg btc_8851b_mon_reg[] = {
	{REG_MAC, 4, 0xda24},
	{REG_MAC, 4, 0xda28},
	{REG_MAC, 4, 0xda2c},
	{REG_MAC, 4, 0xda30},
	{REG_MAC, 4, 0xda4c},
	{REG_MAC, 4, 0xda10},
	{REG_MAC, 4, 0xda20},
	{REG_MAC, 4, 0xda34},
	{REG_MAC, 4, 0xcef4},
	{REG_MAC, 4, 0x8424},
	{REG_MAC, 4, 0xd200},
	{REG_MAC, 4, 0xd220},
	{REG_BB, 4, 0x980},
	{REG_BB, 4, 0x4738}, /* path0 reg*/
	{REG_BB, 4, 0x4688}, /* path0 reg*/
	{REG_BB, 4, 0x4694}, /* path0 reg*/
	/*{REG_RF, 2, 0x2},*/ /* for RF, bytes->path, 1:A, 2:B... */
	/*{REG_RF, 2, 0x18},*/
	/*{REG_BT_RF, 2, 0x9},*/
	/*{REG_BT_MODEM, 2, 0xa} */
};


/* wl_tx_power: 255->original or BTC_WL_DEF_TX_PWR
 *              else-> bit7:signed bit, ex: 13:13dBm, 0x85:-5dBm
 * wl_rx_gain: 0->original, 1-> for Free-run, 2-> for BTG co-rx
 * bt_tx_power: decrease power, 0->original, 5 -> decreas 5dB.
 * bt_rx_gain: BT LNA constrain Level, 7->original
 */

struct btc_rf_trx_para btc_8852b_rf_ul[] = {
	{15, 0, 0, 7}, /* 0 -> original */
	{15, 2, 0, 7}, /* 1 -> for BT-connected ACI issue && BTG co-rx */
	{15, 0, 0, 7}, /* 2 ->reserved for shared-antenna */
	{15, 0, 0, 7}, /* 3- >reserved for shared-antenna */
	{15, 0, 0, 7}, /* 4 ->reserved for shared-antenna */
	{15, 1, 0, 7}, /* the below id is for non-shared-antenna free-run */
	{ 6, 1, 0, 7},
	{13, 1, 0, 7},
	{13, 1, 0, 7}
};

struct btc_rf_trx_para btc_8852b_rf_dl[] = {
	{15, 0, 0, 7}, /* 0 -> original */
	{15, 2, 0, 7}, /* 1 -> reserved for shared-antenna */
	{15, 0, 0, 7}, /* 2 ->reserved for shared-antenna */
	{15, 0, 0, 7}, /* 3- >reserved for shared-antenna */
	{15, 0, 0, 7}, /* 4 ->reserved for shared-antenna */
	{15, 1, 0, 7}, /* the below id is for non-shared-antenna free-run */
	{15, 1, 0, 7},
	{15, 1, 0, 7},
	{15, 1, 0, 7}
};

const struct btc_chip chip_8852bp = {
	0x8852B, /* chip id */
	0x0, /* chip HW feature/parameter, refer to enum btc_chip_feature */
	0x7, /* desired bt_ver */
	0x07080000, /* desired wl_fw btc ver  */
	0x1, /* scoreboard version */
	0x1, /* mailbox version*/
	BTC_COEX_RTK_MODE, /* pta_mode */
	BTC_COEX_INNER, /* pta_direction */
	6, /* afh_guard_ch */
	btc_8852b_wl_rssi_thres, /* wl rssi threshold level */
	btc_8852b_bt_rssi_thres, /* bt rssi threshold level */
	(u8)2, /* rssi tolerance */
	&btc_8852b_ops, /* chip-dependent function */
	ARRAY_SIZE(btc_8852b_mon_reg),
	btc_8852b_mon_reg, /* wl moniter register */
	ARRAY_SIZE(btc_8852b_rf_ul),
	btc_8852b_rf_ul,
	ARRAY_SIZE(btc_8852b_rf_dl),
	btc_8852b_rf_dl
};

const struct btc_chip chip_8851b = {
	0x8851B, /* chip id */
	0x0, /* chip HW feature/parameter, refer to enum btc_chip_feature */
	0x7, /* desired bt_ver */
	0x07080000, /* desired wl_fw btc ver  */
	0x1, /* scoreboard version */
	0x1, /* mailbox version*/
	BTC_COEX_RTK_MODE, /* pta_mode */
	BTC_COEX_INNER, /* pta_direction */
	6, /* afh_guard_ch */
	btc_8852b_wl_rssi_thres, /* wl rssi threshold level */
	btc_8852b_bt_rssi_thres, /* bt rssi threshold level */
	(u8)2, /* rssi tolerance */
	&btc_8852b_ops, /* chip-dependent function */
	ARRAY_SIZE(btc_8851b_mon_reg),
	btc_8851b_mon_reg, /* wl moniter register */
	ARRAY_SIZE(btc_8852b_rf_ul),
	btc_8852b_rf_ul,
	ARRAY_SIZE(btc_8852b_rf_dl),
	btc_8852b_rf_dl
};

const struct btc_chip chip_8852b = {
	0x8852B, /* chip id */
	0x0, /* chip HW feature/parameter, refer to enum btc_chip_feature */
	0x7, /* desired bt_ver */
	0x07080000, /* desired wl_fw btc ver  */
	0x1, /* scoreboard version */
	0x1, /* mailbox version*/
	BTC_COEX_RTK_MODE, /* pta_mode */
	BTC_COEX_INNER, /* pta_direction */
	6, /* afh_guard_ch */
	btc_8852b_wl_rssi_thres, /* wl rssi threshold level */
	btc_8852b_bt_rssi_thres, /* bt rssi threshold level */
	(u8)2, /* rssi tolerance */
	&btc_8852b_ops, /* chip-dependent function */
	ARRAY_SIZE(btc_8852b_mon_reg),
	btc_8852b_mon_reg, /* wl moniter register */
	ARRAY_SIZE(btc_8852b_rf_ul),
	btc_8852b_rf_ul,
	ARRAY_SIZE(btc_8852b_rf_dl),
	btc_8852b_rf_dl
};

void _8852b_rfe_type(struct btc_t *btc)
{
	struct rtw_phl_com_t *p = btc->phl;
	struct rtw_hal_com_t *h = btc->hal;
	struct btc_module *module = &btc->mdinfo;

	PHL_TRACE(COMP_PHL_BTC, _PHL_DEBUG_, "[BTC], %s !! \n", __FUNCTION__);

	/* get from final capability of device  */
	module->rfe_type = p->dev_cap.rfe_type;
	module->kt_ver = h->cv;
	module->kt_ver_adie = h->acv;
	module->bt_solo = 0;
	module->bt_pos = BTC_BT_BTG;
	module->switch_type = BTC_SWITCH_INTERNAL;
	module->wa_type = 0;

	module->ant.type = BTC_ANT_SHARED;
	module->ant.num = 2;
	module->ant.isolation = 10;
	module->ant.diversity = 0;
	/* WL 1-stream+1-Ant is located at 0:s0(path-A) or 1:s1(path-B) */
	module->ant.single_pos = RF_PATH_A;
	module->ant.btg_pos = RF_PATH_B;

	if (module->rfe_type == 0) {
		btc->dm.error |= BTC_DMERR_RFE_TYPE0;
		return;
	}

	switch (btc->hal->chip_id) {
	case CHIP_WIFI6_8851B: /* 51B */
		/* rfe_type 3*n+1: 1-Ant(shared),
		 *          3*n+2: 2-Ant+Div(non-shared),
		 *          3*n+3: 2-Ant+no-Div(non-shared)
		 */
		module->ant.num = (module->rfe_type % 3 == 1)? 1 : 2;
		/* WL-1ss at S0, btg at s0 (On 1 WL RF) */
		module->ant.single_pos = RF_PATH_A;
		module->ant.btg_pos = RF_PATH_A;

		if (module->ant.num == 1) {
			module->ant.type = BTC_ANT_SHARED;
			module->bt_pos = BTC_BT_BTG;
			module->wa_type = 1;
		} else { /* ant.num == 2 */
			module->ant.type = BTC_ANT_DEDICATED;
			module->bt_pos = BTC_BT_ALONE;
			module->switch_type = BTC_SWITCH_EXTERNAL;
			if (module->rfe_type % 3 == 2)
				module->ant.diversity = 1;
		}
		break;
	default: /* 52B, 52BP */
		/*rfe_type odd: 2-Ant(shared), even: 3-Ant(non-shared)*/
		module->ant.num = (module->rfe_type % 2)? 2 : 3;

		if (module->ant.num == 2) {
			module->ant.type = BTC_ANT_SHARED;
			module->bt_pos = BTC_BT_BTG;
			if (p->phy_cap[0].txss == 1 &&
			    p->phy_cap[0].rxss == 1) { /* 52B-VS */
				module->ant.num = 1;
				/* WL-1ss at S1, btg at s1*/
				module->ant.single_pos = RF_PATH_B;
				module->ant.btg_pos = RF_PATH_B;
			}

			/* Todo: setup if enable MIMO-PS (to 1T1R)*/
		} else { /* ant.num == 3 */
			module->ant.type = BTC_ANT_DEDICATED;
			module->bt_pos = BTC_BT_ALONE;
		}
		break;
	}
}

void _8852b_get_reg_status(struct btc_t *btc, u8 type, void *status)
{
	struct btc_module *md = &btc->mdinfo;
	u32 val;
	u8 *pos = (u8*)status;

	switch (type) {
	case BTC_CSTATUS_TXDIV_POS:
		if (btc->hal->chip_id != CHIP_WIFI6_8851B)
			return;

		if (md->switch_type == BTC_SWITCH_INTERNAL) {
			*pos = BTC_ANT_DIV_MAIN;
			break;
		}

		val = hal_read32(btc->hal, R_BTC_BB_ANT_DIV_CTRL) & 0xf000;
		*pos = (val == 0x1000)? BTC_ANT_DIV_AUX : BTC_ANT_DIV_MAIN;
		break;
	case BTC_CSTATUS_RXDIV_POS:
		if (btc->hal->chip_id != CHIP_WIFI6_8851B)
			return;

		if (md->switch_type == BTC_SWITCH_INTERNAL) {
			*pos = BTC_ANT_DIV_MAIN;
			break;
		}

		val = hal_read32(btc->hal, R_BTC_BB_ANT_DIV_CTRL) & 0xf0;
		*pos = (val == 0x10)? BTC_ANT_DIV_AUX : BTC_ANT_DIV_MAIN;
		break;
	}
}

void _8852b_wl_tx_power(struct btc_t *btc, u32 level)
{
	/*
	 * =========== All-Time WL Tx power control ==========
	 * (ex: all-time fix WL Tx 10dBm , don't care GNT _BT and GNT _LTE)
	 * Turn off per-packet power control
	 * 0xD220[1] = 0, 0xD220[2] = 0;
	 *
	 * disable using related power table
	 * 0xd208[20] = 0, 0xd208[21] = 0, 0xd21c[17] = 0, 0xd20c[29] = 0;
	 *
	 * enable force tx power mode and value
	 * 0xD200[9] = 1;
	 * 0xD200[8:0] = 0x28; S(9,2): 1step= 0.25dB, i.e. 40*0.25 = 10 dBm
	 * =========== per-packet Tx power control ===========
	 * (ex: GNT_BT = 1 -> 5dBm,  GNT _BT = 0 -> 10dBm)
	 * Turn on per-packet power control
	 * 0xD220[1] = 1, 0xD220[2] = 0;
	 * 0xD220[11:3] = 0x14; S(9,2): 1step = 0.25dB, i.e. 20*0.25 = 5 dBm
	 *
	 * disable using related power table
	 * 0xd208[20] = 0, 0xd208[21] = 0, 0xd21c[17] = 0, 0xd20c[29] = 0;
	 *
	 * enable force tx power mode and value
	 * 0xD200[9] = 1;
	 * 0xD200[8:0] = 0x28; S(9,2): 1 step = 0.25dB, i.e. 40*0.25 = 10 dBm
	 * ===================================================================
	 * level define:
	 *    if level = 255 -> back to original (btc don't control)
	 *    else in dBm --> bit7->signed bit, ex: 0xa-> +10dBm, 0x85-> -5dBm
	 * pwr_val define:
	 *     bit15~0  --> All-time (GNT_BT = 0) Tx power control
	 *     bit31~16 --> Per-Packet (GNT_BT = 1) Tx power control
	 */
	u32 pwr_val = bMASKDW, phi_idx = HW_PHY_0;
	bool en = false;

	if ((level & 0x7f) < BTC_WL_DEF_TX_PWR) { /* back to original */
		pwr_val = (level & 0x3f) << 2; /* to fit s(9,2) format */

		if (level & BIT(7)) /* negative value */
			pwr_val |= BIT(8); /* to fit s(9,2) format */
		pwr_val |= bMASKHW;
		en = true;
	}

	if (btc->hal->dbcc_en)
		phi_idx = btc->cx.wl.pta_req_mac;

	rtw_hal_rf_wlan_tx_power_control(btc->hal, phi_idx, ALL_TIME_CTRL,
					 pwr_val, en);
}

void _8852b_wl_rx_gain(struct btc_t *btc, u32 level)
{
	/* To improve BT ACI in co-rx
	 * level=0 Default: TIA 1/0= (LNA2,TIAN6) = (7,1)/(5,1) = 21dB/12dB
	 * level=1 Fix LNA2=5: TIA 1/0= (LNA2,TIAN6) = (5,0)/(5,1) = 18dB/12dB
	 */
	u32 mask = bMASKRF, n = 0, i = 0, val = 0;
	u32 type = (btc->mdinfo.ant.btg_pos << 8) | RTW_MAC_RF_CMD_OFLD;
	const struct btc_rf_cfg *rf = NULL;
	bool en;

	switch (level) {
	case 0: /* original */
	default:
		btc->dm.wl_lna2 = 0;
		break;
	case 1: /* for FDD free-run */
		btc->dm.wl_lna2 = 0;
		break;
	case 2: /* for BTG Co-Rx*/
		btc->dm.wl_lna2 = 1;
		break;
	}

	switch(btc->hal->chip_id) {
	case CHIP_WIFI6_8852B:
	default:
		mask = bMASKRF;
		if (btc->dm.wl_lna2 == 0) {
			rf = btc_8852b_rf_0;
			n = sizeof(btc_8852b_rf_0)/sizeof(btc_8852b_rf_0[0]);
		} else {
			rf = btc_8852b_rf_1;
			n = sizeof(btc_8852b_rf_1)/sizeof(btc_8852b_rf_1[0]);
		}
		break;
	case CHIP_WIFI6_8852BP:
		mask = bMASKRF;
		if (btc->dm.wl_lna2 == 0) {
			rf = btc_8852bp_rf_0;
			n = sizeof(btc_8852bp_rf_0)/sizeof(btc_8852bp_rf_0[0]);
		} else {
			rf = btc_8852bp_rf_1;
			n = sizeof(btc_8852bp_rf_1)/sizeof(btc_8852bp_rf_1[0]);
		}
		break;;
	case CHIP_WIFI6_8851B:
		mask = 0x700; /* bit8~10 */
		if (btc->dm.wl_lna2 == 0) {
			rf = btc_8851b_rf_0;
			n = sizeof(btc_8851b_rf_0)/sizeof(btc_8851b_rf_0[0]);
		} else {
			rf = btc_8851b_rf_1;
			n = sizeof(btc_8851b_rf_1)/sizeof(btc_8851b_rf_1[0]);
		}
		break;
	}

	while (1) {
		en = (i == n-1) ? true : false;

		val = rf->val;
		/* bit[10] = 1 if non-shared-ant for 8851b */
		if (btc->hal->chip_id == CHIP_WIFI6_8851B &&
		    btc->mdinfo.ant.type == BTC_ANT_DEDICATED)
			val |= 0x4;

		_btc_io_w(btc, type, rf->addr, mask, val, en);
		i++;
		if (i > n-1)
			break;
		rf++;
	}
}

u8 _8852b_bt_rssi(struct btc_t *btc, u8 val)
{
	val = (val <= 127? 100 : (val >= 156? val - 156 : 0));
	val = val + 6; /* compensate offset */

	if (val > 100)
		val = 100;

	return (val);
}

void _8852b_wl_trx_mask(struct btc_t *btc, u32 type, u8 group, u32 val)
{
	if (btc->hal->chip_id == CHIP_WIFI6_8851B) {
		if (group > BTC_BT_SS_GROUP)
			group--; /* Tx-group=1, Rx-group=2 */

		if (btc->mdinfo.ant.type == BTC_ANT_SHARED) /* 1-Ant */
			group += 3;
	}

	_btc_io_w(btc, type, R_BTC_RF_LUT_WA, bMASKRF, group, false);
	_btc_io_w(btc, type, R_BTC_RF_LUT_WD0, bMASKRF, val, false);
}

void _8852b_wl_btg_standby(struct btc_t *btc, u32 state)
{
	u32 data0, data1;
	u32 type = (btc->mdinfo.ant.btg_pos << 8) | RTW_MAC_RF_CMD_OFLD;

	switch(btc->hal->chip_id) {
	default:
	case CHIP_WIFI6_8852B:
		data1 = 0x31;
		data0 = (state == 1) ? 0x179 : 0x20;
		break;
	case CHIP_WIFI6_8852BP:
		data1 = 0x620;
		data0 = (state == 1) ? 0x179c : 0x208;
		break;
	case CHIP_WIFI6_8851B:
		data1 = 0x110;
		data0 = (state == 1) ? 0x179c : 0x208;
		break;
	}

	/* set WL standby = Rx for GNT_BT_Tx = 1->0 settle issue */
	_btc_io_w(btc, type, R_BTC_RF_LUT_EN, bMASKRF, BIT(19), false);
	_btc_io_w(btc, type, R_BTC_RF_LUT_WA, bMASKRF, 0x1, false);
	_btc_io_w(btc, type, R_BTC_RF_LUT_WD1, bMASKRF, data1, false);
	_btc_io_w(btc, type, R_BTC_RF_LUT_WD0, bMASKRF, data0, false);
	_btc_io_w(btc, type, R_BTC_RF_LUT_EN, bMASKRF, 0x0, true);
}

void _8852b_wl_req_mac(struct btc_t *btc, u8 mac_id)
{
	u32 val1;

	val1 = _read_cx_reg(btc, R_BTC_CFG);

	if (mac_id == HW_PHY_0)
		val1 = val1 & (~B_BTC_WL_SRC);
	else
		val1 = val1 | B_BTC_WL_SRC;

	_write_cx_reg(btc, R_BTC_CFG, val1);
}

void _8852b_wl_pri(struct btc_t *btc, u8 map, bool state)
{
	u32 reg = 0, bitmap = 0;

	switch (map) {
	case BTC_PRI_MASK_TX_RESP:
	default:
		reg = R_BTC_BT_COEX_MSK_TABLE;
		bitmap = B_BTC_PRI_MASK_TX_RESP_V1;
		break;
	case BTC_PRI_MASK_BEACON:
		reg = R_BTC_WL_PRI_MSK;
		bitmap = B_BTC_PTA_WL_PRI_MASK_BCNQ;
		break;
	case BTC_PRI_MASK_RX_CCK:
		reg = R_BTC_BT_COEX_MSK_TABLE;
		bitmap = B_BTC_PRI_MASK_RXCCK_V1;
		break;
	}

	_btc_io_w(btc, RTW_MAC_MAC_CMD_OFLD, reg, bitmap, state, false);
}

void _8852b_init_cfg(struct btc_t *btc)
{
	struct rtw_hal_com_t *h = btc->hal;
	struct btc_ant_info *ant = &btc->mdinfo.ant;
	u32 path, type, path_min, path_max;

	PHL_INFO("[BTC], %s !! \n", __FUNCTION__);

	/* PTA init  */
	rtw_hal_mac_coex_init(h, btc->chip->pta_mode, btc->chip->pta_direction);

	/* set WL Tx response = Hi-Pri */
	_8852b_wl_pri(btc, BTC_PRI_MASK_TX_RESP, true);

	/* set WL Tx beacon = Hi-Pri */
	_8852b_wl_pri(btc, BTC_PRI_MASK_BEACON, true);

	/* for 1-Ant && 1-ss case: only 1-path */
	if (ant->num == 1) {
		path_min = ant->single_pos;
		path_max = path_min;
	} else {
		path_min = RF_PATH_A;
		path_max = RF_PATH_B;
	}

	path = path_min;

	while (1) {
		type = (path << 8) | RTW_MAC_RF_CMD_OFLD;
		/* set rf gnt-debug off */
		_btc_io_w(btc, type, R_BTC_RF_BTG_CTRL, bMASKRF, 0x0, false);

		/* set DEBUG_LUT_RFMODE_MASK = 1 to start trx-mask-setup */
		_btc_io_w(btc, type, R_BTC_RF_LUT_EN, bMASKRF, BIT(17), false);

		/* if GNT_WL=0 && BT=SS_group --> WL Tx/Rx = THRU  */
		_8852b_wl_trx_mask(btc, type, BTC_BT_SS_GROUP, 0x5ff);

		/* if GNT_WL=0 && BT=Rx_group --> WL-Rx = THRU + WL-Tx = MASK */
		_8852b_wl_trx_mask(btc, type, BTC_BT_RX_GROUP, 0x5df);

		/* if GNT_WL = 0 && BT = Tx_group -->
		 * Shared-Ant && BTG-path:WL mask(0x55f), others:WL THRU(0x5ff)
		 */
		if (ant->type == BTC_ANT_SHARED && ant->btg_pos == path)
			_8852b_wl_trx_mask(btc, type, BTC_BT_TX_GROUP, 0x55f);
		else
			_8852b_wl_trx_mask(btc, type, BTC_BT_TX_GROUP, 0x5ff);

		/* set DEBUG_LUT_RFMODE_MASK = 0 to stop trx-mask-setup */
		_btc_io_w(btc, type, R_BTC_RF_LUT_EN, bMASKRF, 0, false);

		path++;
		if (path > path_max)
			break;
	}

	/* set PTA break table */
	_btc_io_w(btc, RTW_MAC_MAC_CMD_OFLD, R_BTC_BREAK_TABLE, bMASKDW,
		  0xf0ffffff, false);

	/* enable BT counter 0xda40[16,2] = 2b'11 */
	_btc_io_w(btc, RTW_MAC_MAC_CMD_OFLD, R_BTC_CSR_MODE, 0x10004,
		  0x4001, true);
}

#endif /* CONFIG_RTL8852B */
#endif



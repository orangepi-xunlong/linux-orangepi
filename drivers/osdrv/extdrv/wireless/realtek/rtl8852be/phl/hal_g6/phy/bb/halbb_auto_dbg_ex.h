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
#ifndef __HALBB_AUTO_DBG_EX_H__
#define __HALBB_AUTO_DBG_EX_H__
#ifdef HALBB_AUTO_DBG_SUPPORT

#include "halbb_statistics.h"

/*@--------------------------[Define] ---------------------------------------*/

/*@--------------------------[Enum]------------------------------------------*/
enum bb_auto_dbg_t {
	AUTO_DBG_CHECK_HANG	= BIT(0),
	AUTO_DBG_CHECK_TX	= BIT(1),
	AUTO_DBG_STORE_PMAC	= BIT(2),
	AUTO_DBG_PHY_UTILITY	= BIT(3)
};

/*@--------------------------[Structure]-------------------------------------*/
struct bb_bkp_phy_utility_info {
	u16 rx_utility;
	u16 avg_phy_rate;
	u16 rx_rate_plurality;
	u16 tx_rate;
	struct bb_physts_avg_info bb_physts_avg_i;
};

struct bb_bkp_pmac_info {
	struct bb_tx_cnt_info		bb_tx_cnt_i;
	struct bb_cca_info		bb_cca_i;
	struct bb_crc_info		bb_crc_i;
	struct bb_crc2_info		bb_crc2_i;
	struct bb_fa_info		bb_fa_i;
	struct bb_usr_set_info		bb_usr_set_i;
};

struct bb_info;
/*@--------------------------[Prptotype]-------------------------------------*/
void halbb_query_hang_info(struct bb_info *bb, struct bb_stat_hang_info *rpt);
void halbb_query_pmac_info(struct bb_info *bb, struct bb_bkp_pmac_info *rpt);
void halbb_query_phy_utility_info(struct bb_info *bb, struct bb_bkp_phy_utility_info *rpt);
void halbb_auto_debug_en(struct bb_info *bb, enum bb_auto_dbg_t dbg_type, bool en);
#endif
#endif


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
#ifndef __HALBB_AUTO_DBG_H__
#define __HALBB_AUTO_DBG_H__
#ifdef HALBB_AUTO_DBG_SUPPORT
/*@--------------------------[Define] ---------------------------------------*/

/*@--------------------------[Enum]------------------------------------------*/

/*@--------------------------[Structure]-------------------------------------*/

struct bb_chk_hang_info {
	u32 table_size;
	u32 *dbg_port_table;
	u32 *dbg_port_val;
};

struct bb_auto_dbg_info {
	u32 auto_dbg_type; /*enum bb_auto_dbg_t*/
	struct bb_chk_hang_info bb_chk_hang_i;
	struct bb_bkp_pmac_info bb_bkp_pmac_i;
	struct bb_bkp_phy_utility_info bb_bkp_phy_utility_i;
};

struct bb_info;
/*@--------------------------[Prptotype]-------------------------------------*/
void halbb_auto_debug_watchdog(struct bb_info *bb);
void halbb_auto_debug_init(struct bb_info *bb);
void halbb_auto_debug_dbg(struct bb_info *bb, char input[][16], 
			  u32 *_used, char *output, u32 *_out_len);

#endif
#endif


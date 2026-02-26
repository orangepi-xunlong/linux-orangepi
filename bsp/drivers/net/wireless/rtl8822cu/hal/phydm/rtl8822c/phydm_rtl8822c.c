/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
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
#include "mp_precomp.h"
#include "../phydm_precomp.h"

#if (RTL8822C_SUPPORT)
void phydm_dynamic_switch_htstf_agc_8822c(struct dm_struct *dm)
{
	u16 ndp_valid_cnt = 0;
	u16 ndp_valid_cnt_diff = 0;

	if (dm->bhtstfdisabled)
		return;

	/*set debug port to 0x51f*/
	if (phydm_set_bb_dbg_port(dm, DBGPORT_PRI_1, 0x51f)) {
		ndp_valid_cnt = (u16)(phydm_get_bb_dbg_port_val(dm) & 0xff);
		phydm_release_bb_dbg_port(dm);

		ndp_valid_cnt_diff = DIFF_2(dm->ndp_cnt_pre, ndp_valid_cnt);
		dm->ndp_cnt_pre = ndp_valid_cnt;

		if (ndp_valid_cnt_diff)
			dm->is_beamformed = true;
		else
			dm->is_beamformed = false;

		if (dm->total_tp == 0 || dm->is_beamformed) {
			odm_set_bb_reg(dm, R_0x8a0, BIT(2), 0x1);
			dm->no_ndp_cnts = 0;
		} else {
			if (dm->no_ndp_cnts == 3)
				odm_set_bb_reg(dm, R_0x8a0, BIT(2), 0x0);
			else if (dm->no_ndp_cnts < 3)
				dm->no_ndp_cnts++;
		}
	}
}

void phydm_hwsetting_8822c(struct dm_struct *dm)
{
	phydm_dynamic_switch_htstf_agc_8822c(dm);
}
#endif

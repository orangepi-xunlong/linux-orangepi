/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2021, Realtek Semiconductor Corp. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   * Neither the name of the Realtek nor the names of its contributors may
 *     be used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "halbb_precomp.h"

#ifdef HALBB_SR_SUPPORT

void halbb_spatial_reuse_set_pwr_ref(struct bb_info *bb, u32 val)
{
	struct rtw_hal_com_t *hal_com = bb->hal_com;
	struct bb_spatial_reuse_info *bb_sr = &bb->bb_sr_i;
	struct bb_spatial_reuse_cr_info *cr = &bb_sr->bb_spatial_reuse_cr_i;
	
	BB_DBG(bb, DBG_PWR_CTRL, "[%s] set pwr_ref to {%d}dBm", __func__,
		  (val & 0x7f));

	bb_sr->txpwr_ref = (val & 0x7f);
	rtw_hal_mac_write_msk_pwr_reg(hal_com, HW_PHY_0, cr->r_sr_txpwr_ref,
								  cr->r_sr_txpwr_ref_m, bb_sr->txpwr_ref);
}

u8 halbb_spatial_reuse_get_pwr_ref(struct bb_info *bb)
{
	struct bb_spatial_reuse_info *bb_sr = &bb->bb_sr_i;

	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);

	return (bb_sr->txpwr_ref);
}

bool halbb_spatial_reuse_abort(struct bb_info *bb)
{
	if (!(bb->bb_sr_i.ax_sr_info.sr_en)){
		BB_DBG(bb, DBG_PWR_CTRL, "SR disable\n");
		return true;
	}

	return false;
}

void halbb_spatial_reuse_update(struct bb_info *bb)
{
	struct bb_spatial_reuse_info *bb_sr = &bb->bb_sr_i;
	struct rtw_mac_ax_sr_info *sr_info = &bb_sr->ax_sr_info;
		
	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);
	
	rtw_hal_mac_sr_update(bb->hal_com, (void *)sr_info, (u8)HW_PHY_0);
}

void halbb_spatial_reuse(struct bb_info *bb)
{
	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);
	
	if (halbb_spatial_reuse_abort(bb))
		return;
	if (bb->bb_sr_i.need_update) {
		halbb_spatial_reuse_update(bb);
		bb->bb_sr_i.need_update = false;
	}
}

void halbb_spatial_reuse_init(struct bb_info *bb)
{
	struct bb_spatial_reuse_info *bb_sr = &bb->bb_sr_i;
	struct rtw_mac_ax_sr_info *sr_info = &bb_sr->ax_sr_info;
	
	if(phl_is_mp_mode(bb->phl_com))
		return;
	
	BB_DBG(bb, DBG_INIT, "[%s]", __func__);

	sr_info->sr_en = 0;
	sr_info->sr_field_v15_allowed = 0;
	sr_info->srg_obss_pd_min = ((SRG_OBSS_PD_MIN + 110) << 2);
	sr_info->srg_obss_pd_max = ((SRG_OBSS_PD_MAX + 110) << 2);
	sr_info->non_srg_obss_pd_min = ((NON_SRG_OBSS_PD_MIN + 110) << 2);
	sr_info->non_srg_obss_pd_max = ((NON_SRG_OBSS_PD_MAX + 110) << 2);
	sr_info->srg_bsscolor_bitmap_0 = 0;
	sr_info->srg_bsscolor_bitmap_1 = 0;
	sr_info->srg_partbsid_bitmap_0 = 0;
	sr_info->srg_partbsid_bitmap_1 = 0;

	bb_sr->need_update = true;

	halbb_spatial_reuse_set_pwr_ref(bb, SR_TXPWR_REF);
	halbb_spatial_reuse_update(bb);
}

void halbb_spatial_reuse_en(struct bb_info *bb, bool sr_en)
{
	struct bb_spatial_reuse_info *bb_sr = &bb->bb_sr_i;
	struct rtw_mac_ax_sr_info *sr_info = &bb_sr->ax_sr_info;

	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);

	sr_info->sr_en = (sr_en)? 1 : 0;
	halbb_spatial_reuse_update(bb);
}

bool halbb_spatial_reuse_is_en(struct bb_info *bb)
{
	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);

	return (bb->bb_sr_i.ax_sr_info.sr_en)? true : false;
}

void halbb_cr_cfg_spatial_reuse_init(struct bb_info *bb)
{
	struct bb_spatial_reuse_cr_info *cr = &bb->bb_sr_i.bb_spatial_reuse_cr_i;
	
	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);

	if (bb->bb_80211spec == BB_AX_IC){
		cr->r_sr_txpwr_ref = 0xD23C;
		cr->r_sr_txpwr_ref_m = 0x7F;
	}
	else {
		cr->r_sr_txpwr_ref = 0x11AD4;
		cr->r_sr_txpwr_ref_m = 0x7F;
	}
}

void halbb_spatial_reuse_dbg(struct bb_info *bb, char input[][16], u32 *_used,
		   char *output, u32 *_out_len)
{
	struct bb_spatial_reuse_info *bb_sr = &bb->bb_sr_i;
	struct rtw_mac_ax_sr_info *sr_info = &bb_sr->ax_sr_info;
	
	u32 val[10] = {0};
	s8 val_s8 = 0;
	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 	"[SR CTRL]\n");

	if (_os_strcmp(input[1], "-h") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"show\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"sr_en {0/1: disable/enable}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"non_srg_obss_pd {0/1: min/max} {val dBm}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"pwr_ref {val dBm}\n");
		return;
	}
	if (_os_strcmp(input[1], "show") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    	"================\n\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"%-22s {%s}\n", "[sr_en]", (sr_info->sr_en)? "true" : "false");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"%-22s {%d dBm}\n", "[non_srg_obss_pd_min]",
					((sr_info->non_srg_obss_pd_min >> 2) - 110));
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"%-22s {%d dBm}\n", "[non_srg_obss_pd_max]",
					((sr_info->non_srg_obss_pd_max >> 2) - 110));
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"%-22s {%d dBm}\n", "[pwr_ref]",
					halbb_spatial_reuse_get_pwr_ref(bb));
	}
	else if (_os_strcmp(input[1], "sr_en") == 0) {
		HALBB_SCAN(input[2], DCMD_HEX, &val[0]);
		if (val[0] == 0)
			halbb_spatial_reuse_en(bb, false);
		else
			halbb_spatial_reuse_en(bb, true);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			   		"sr_en = %s\n", (val[0])? "true" : "false");
	}
	else if (_os_strcmp(input[1], "non_srg_obss_pd") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[1]);
		val_s8 = (s8)val[1];
		if (val[0] == 0)
			sr_info->non_srg_obss_pd_min = (((val_s8 + 110) << 2) & 0xff);
		else if(val[0] == 1)
			sr_info->non_srg_obss_pd_max = (((val_s8 + 110) << 2) & 0xff);
		else{
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    		"Set Error\n");
			return;
		}
		halbb_spatial_reuse_update(bb);
	}
	else if (_os_strcmp(input[1], "pwr_ref") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		halbb_spatial_reuse_set_pwr_ref(bb, val[0]);
	}
	else
	{
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    	"Set Error\n");
		return;
	}
}

#endif


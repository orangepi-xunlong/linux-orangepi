/******************************************************************************
 *
 * Copyright(c) 2019 - 2021 Realtek Corporation.
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
#ifndef __HALBB_BB_WRAPPER_H__
#define __HALBB_BB_WRAPPER_H__


struct bb_info;
/*@--------------------------[Prptotype]-------------------------------------*/
//void halbb_tmac_force_tx_pwr(struct bb_info *bb, s8 pw_val, u8 n_path, u8 dbw_idx, enum phl_phy_idx phy_idx);
void halbb_bb_wrap_tpu_set_all(struct bb_info* bb, enum phl_phy_idx phy_idx);
void halbb_bb_wrap_init(struct bb_info *bb_0, enum phl_phy_idx phy_idx);
void halbb_bb_wrap_dbg_be(struct bb_info *bb, char input[][16], u32 *_used,
			  char *output, u32 *_out_len);
void halbb_bb_wrap_be_set_pwr_ref_all(struct bb_info *bb, enum phl_phy_idx phy_idx);
//void halbb_bb_wrap_set_tx_src(struct bb_info *bb, u8 option, s8 pw_val, u8 n_path, u8 dbw_idx, enum phl_phy_idx phy_idx);
#endif

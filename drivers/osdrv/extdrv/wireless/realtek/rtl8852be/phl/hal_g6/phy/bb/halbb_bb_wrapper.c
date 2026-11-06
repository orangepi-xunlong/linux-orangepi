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
#ifdef HALBB_BB_WRAP_SUPPORT

#define BE_TPU_CR_FILL_IN

#ifdef BE_TPU_CR_FILL_IN
void halbb_bb_wrap_be_set_pow_by_rate_all(struct bb_info* bb, enum phl_phy_idx phy_idx)
{
	struct bb_tpu_be_info *tpu = &bb->hal_com->band[phy_idx].bb_tpu_all_i.bb_tpu_be_i;
	struct bb_tpu_pwr_by_rate_info_be *by_rate = &tpu->rtw_tpu_pwr_by_rate_be_i;

	s8 *tmp, *tmp_1;
	u32 bw_idx, ss_idx, mcs_idx, cr;
	u32 ss_ofst = 0, bw_ofst[TPU_SIZE_PWR_TAB_DBW] = {0, 0x58, 0xAC, 0x100, 0x154};
	u32 base_cck = 0, base_ofdm = 0, base_eht = 0, base_mcs = 0, base_dcm = 0;
	u32	base_dlru = 0, base_dlru_dcm = 0;	// base addr
	
	/* base CR addr setting */
	switch (bb->ic_type) {
		
	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		ss_ofst = 36;
		base_cck = 0x11C00;
		base_ofdm = 0x11C04;
		base_eht = 0x11C0C;
		base_mcs = 0x11C10;
		base_dcm = 0x11C1E;
		base_dlru = 0x11C22;
		base_dlru_dcm = 0x11C30;
		break;
	#endif
	
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		ss_ofst = 36;
		base_cck = 0x11E00;
		base_ofdm = 0x11E04;
		base_eht = 0x11E0C;
		base_mcs = 0x11E10;
		base_dcm = 0x11E1E;
		base_dlru = 0x11E22;
		base_dlru_dcm = 0x11E30;
		break;
	#endif	

	default:
		break;
	}

	if (base_cck == 0){
		BB_WARNING("[%s]\n", "IC_TYPE Selected Wrong");
		return;
	}

	/* fill in TPU */
	for (bw_idx = 0; bw_idx < TPU_SIZE_PWR_TAB_DBW; bw_idx++) {
		/* CCK */
		if (bw_idx < 2) {
			cr = base_cck + bw_ofst[bw_idx];
			tmp = &by_rate->pwr_by_rate_cck[bw_idx][0]; 
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));
		} 
		/* OFDM(Legacy) */
		cr = base_ofdm + bw_ofst[bw_idx];
		tmp = &by_rate->pwr_by_rate_ofdm[bw_idx][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, (cr + 4), BYTE_2_DWORD(tmp[7],tmp[6],tmp[5],tmp[4]));
		/* EHT */
		for (ss_idx = 0; ss_idx < HALBB_MAX_PATH; ss_idx++) {
			cr = base_eht + bw_ofst[bw_idx] + ss_ofst*ss_idx;
			tmp = &by_rate->pwr_by_rate_eht[bw_idx][0];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));
		
			if (bb->ic_type == BB_RTL8922A || bb->ic_type == BB_RLE1115)
				break;
		}
		/* MCS(non-Lagecy) + DCM  + dlru */
		for (ss_idx = 0; ss_idx < HALBB_MAX_PATH; ss_idx++) {
			/* MCS(non-Lagecy) 0~13 + DCM 0~1*/
			for (mcs_idx = 0; mcs_idx <= 13; mcs_idx += 4) {
				cr = base_mcs + bw_ofst[bw_idx] + ss_ofst*ss_idx + mcs_idx;
				tmp = &by_rate->pwr_by_rate_mcs[bw_idx][ss_idx][mcs_idx];
				rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));
			}
			/* DCM 3~4 + dlru 0~1 */
			mcs_idx = 16;	
			cr = base_mcs + bw_ofst[bw_idx] + ss_ofst*ss_idx + mcs_idx;
			tmp = &by_rate->pwr_by_rate_mcs[bw_idx][ss_idx][mcs_idx];
			tmp_1 = &by_rate->pwr_by_rate_dlru[bw_idx][ss_idx][0];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));

			/* dlru 2~13 */
			for (mcs_idx = 2; mcs_idx <= 13; mcs_idx += 4) {
				cr = base_dlru + bw_ofst[bw_idx] + ss_ofst*ss_idx + mcs_idx;
				tmp = &by_rate->pwr_by_rate_dlru[bw_idx][ss_idx][mcs_idx];
				rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));
			}
			/* dlru_DCM */
			mcs_idx = 14;
			cr = base_dlru_dcm + bw_ofst[bw_idx] + ss_ofst*ss_idx;
			tmp = &by_rate->pwr_by_rate_dlru[bw_idx][ss_idx][mcs_idx];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));
		}
		if (bb->ic_type == BB_RLE1115)
				break;		
	}
}

void halbb_bb_wrap_be_set_pwr_limit_all(struct bb_info* bb, enum phl_phy_idx phy_idx)
{
	struct bb_tpu_be_info *tpu = &bb->hal_com->band[phy_idx].bb_tpu_all_i.bb_tpu_be_i;
	struct bb_tpu_pwr_lmt_info_be *by_lmt = &tpu->rtw_tpu_pwr_lmt_be_i;

	s8 *tmp, *tmp_1;
	u32 ru_idx, ss_idx, ss_ofst = 0, cr;
	u32 base_cck = 0, base_lgcy_non_dup = 0, base_BW20 = 0, base_BW40 = 0;
	u32 base_BW80 = 0, base_BW160 = 0, base_BW320 = 0;
	u32 base_BW40_0p5 = 0, base_BW40_2p5 = 0, base_BW40_4p5 = 0, base_BW40_6p5 = 0;
	
	/* base CR addr setting */
	switch (bb->ic_type) {
		
	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		ss_ofst = 0x4C;
		base_cck = 0x11C58;
		base_lgcy_non_dup = 0x11C5C;
		base_BW20 = 0x11C5E;
		base_BW40 = 0x11C7E;
		base_BW80 = 0x11C8E;
		base_BW160 = 0x11C96;
		base_BW320 = 0x11C9A;
		base_BW40_0p5 = 0x11C9C;
		base_BW40_2p5 = 0x11C9E;
		base_BW40_4p5 = 0x11CA0;
		base_BW40_6p5 = 0x11CA2;
		break;
	#endif
	
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		ss_ofst = 0x4C;
		base_cck = 0x11FAC;
		base_lgcy_non_dup = 0x11FB0;
		base_BW20 = 0x11FB2;
		base_BW40 = 0x11FD2;
		base_BW80 = 0x11FE2;
		base_BW160 = 0x11FEA;
		base_BW320 = 0x11FEE;
		base_BW40_0p5 = 0x11FF0;
		base_BW40_2p5 = 0x11FF2;
		base_BW40_4p5 = 0x11FF4;
		base_BW40_6p5 = 0x11FF6;
		break;
	#endif	

	default:
		break;
	}

	if (base_cck == 0) {
		BB_WARNING("[%s]\n", "IC_TYPE Selected Wrong");
		return;
	}

	for (ss_idx = 0; ss_idx < HALBB_MAX_PATH; ss_idx++) {
		/* CCK */
		cr = base_cck + ss_ofst*ss_idx;
		tmp = &by_lmt->pwr_lmt_cck[ss_idx][0][0];
		tmp_1 = &by_lmt->pwr_lmt_cck[ss_idx][1][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));

		/* lgcy_non_dup + BW20 0 */
		ru_idx = 0;
		cr = base_lgcy_non_dup + ss_ofst*ss_idx;
		tmp = &by_lmt->pwr_lmt_lgcy_non_dup[ss_idx][0];
		tmp_1 = &by_lmt->pwr_lmt_20m[ss_idx][ru_idx][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));
		
		/* BW20 1~14 */
		for (ru_idx = 1; ru_idx <= 14; ru_idx += 2) {
			cr = base_BW20 + ss_ofst*ss_idx + 2*ru_idx;
			tmp = &by_lmt->pwr_lmt_20m[ss_idx][ru_idx][0];
			tmp_1 = &by_lmt->pwr_lmt_20m[ss_idx][ru_idx+1][0];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));
		}
		
		/* BW20 15 + BW40 0 */
		ru_idx = 15;
		cr = base_BW20 + ss_ofst*ss_idx + 2*ru_idx;
		tmp = &by_lmt->pwr_lmt_20m[ss_idx][ru_idx][0];
		tmp_1 = &by_lmt->pwr_lmt_40m[ss_idx][0][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));

		/* BW40 1~6 */
		for (ru_idx = 1; ru_idx <= 6; ru_idx += 2) {
			cr = base_BW40 + ss_ofst*ss_idx + 2*ru_idx;
			tmp = &by_lmt->pwr_lmt_40m[ss_idx][ru_idx][0];
			tmp_1 = &by_lmt->pwr_lmt_40m[ss_idx][ru_idx+1][0];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));
		}	
		
		/* BW40 7 + BW80 0*/
		ru_idx = 7;
		cr = base_BW40 + ss_ofst*ss_idx + 2*ru_idx;
		tmp = &by_lmt->pwr_lmt_40m[ss_idx][ru_idx][0];
		tmp_1 = &by_lmt->pwr_lmt_80m[ss_idx][0][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));	
		
		/* BW80 1~2 */
		ru_idx = 1;
		cr = base_BW80 + ss_ofst*ss_idx + 2*ru_idx;
		tmp = &by_lmt->pwr_lmt_80m[ss_idx][ru_idx][0];
		tmp_1 = &by_lmt->pwr_lmt_80m[ss_idx][ru_idx+1][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));

		/* BW80 3 + BW160 0 */
		ru_idx = 3;
		cr = base_BW80 + ss_ofst*ss_idx + 2*ru_idx;
		tmp = &by_lmt->pwr_lmt_80m[ss_idx][ru_idx][0];
		tmp_1 = &by_lmt->pwr_lmt_160m[ss_idx][0][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));

		/* BW160 1 + BW320 0 */
		ru_idx = 1;
		cr = base_BW160 + ss_ofst*ss_idx + 2*ru_idx;
		tmp = &by_lmt->pwr_lmt_160m[ss_idx][ru_idx][0];
		tmp_1 = &by_lmt->pwr_lmt_320m[ss_idx][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));

		/* BW40 0p5 + 2p5 */
		cr = base_BW40_0p5 + ss_ofst*ss_idx;
		tmp = &by_lmt->pwr_lmt_40m_0p5[ss_idx][0];
		tmp_1 = &by_lmt->pwr_lmt_40m_2p5[ss_idx][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));

		/* BW40 4p5 + 6p5 */
		cr = base_BW40_4p5 + ss_ofst*ss_idx;
		tmp = &by_lmt->pwr_lmt_40m_4p5[ss_idx][0];
		tmp_1 = &by_lmt->pwr_lmt_40m_6p5[ss_idx][0];
		rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp_1[1],tmp_1[0],tmp[1],tmp[0]));
	}
}

void halbb_bb_wrap_be_set_pwr_limit_rua_all(struct bb_info* bb, enum phl_phy_idx phy_idx){

	struct bb_tpu_be_info *tpu = &bb->hal_com->band[phy_idx].bb_tpu_all_i.bb_tpu_be_i;
	struct bb_tpu_pwr_lmt_ru_info_be *by_lmtru = &tpu->rtw_tpu_pwr_lmt_ru_be_i;

	s8 *tmp;
	u32 ru_idx, ss_idx, ss_ofst = 0, cr;
	u32 base_ru26 = 0, base_ru52 = 0, base_ru106 = 0, base_ru52_26 = 0, base_ru106_26 = 0;
	
	/* base CR addr setting */
	switch (bb->ic_type) {
		
	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		ss_ofst = 0x50;
		base_ru26 = 0x11CF4;
		base_ru52 = 0x11D04;
		base_ru106 = 0x11D14;
		base_ru52_26 = 0x11D24;
		base_ru106_26 = 0x11D34;
		break;
	#endif
	
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		ss_ofst = 0x50;
		base_ru26 = 0x12048;
		base_ru52 = 0x12058;
		base_ru106 = 0x12068;
		base_ru52_26 = 0x12078;
		base_ru106_26 = 0x12088;
		break;
	#endif	

	default:
		break;
	}

	if (base_ru26 == 0) {
		BB_WARNING("[%s]\n", "IC_TYPE Selected Wrong");
		return;
	}

	for (ss_idx = 0; ss_idx < HALBB_MAX_PATH; ss_idx++) {
		for (ru_idx = 0; ru_idx < 16; ru_idx += 4) {
			/* ru26 0~15 */
			cr = base_ru26 + ss_ofst*ss_idx + ru_idx;
			tmp = &by_lmtru->pwr_lmt_ru_be[ss_idx][0][ru_idx];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));

			/* ru52 0~15 */
			cr = base_ru52 + ss_ofst*ss_idx + ru_idx;
			tmp = &by_lmtru->pwr_lmt_ru_be[ss_idx][1][ru_idx];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));

			/* ru106 0~15 */
			cr = base_ru106 + ss_ofst*ss_idx + ru_idx;
			tmp = &by_lmtru->pwr_lmt_ru_be[ss_idx][2][ru_idx];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));

			/* ru52_26 0~15 */
			cr = base_ru52_26 + ss_ofst*ss_idx + ru_idx;
			tmp = &by_lmtru->pwr_lmt_ru52_26_be[ss_idx][ru_idx];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));

			/* ru106_26 0~15 */
			cr = base_ru106_26 + ss_ofst*ss_idx + ru_idx;
			tmp = &by_lmtru->pwr_lmt_ru106_26_be[ss_idx][ru_idx];
			rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BYTE_2_DWORD(tmp[3],tmp[2],tmp[1],tmp[0]));
		}
	}
}

void halbb_bb_wrap_be_set_pwr_ofst_mode_all(struct bb_info *bb, enum phl_phy_idx phy_idx)
{
	struct bb_tpu_be_info *tpu = &bb->hal_com->band[phy_idx].bb_tpu_all_i.bb_tpu_be_i;
	
	u32 cr, base_pwr_ofst_mode;
	s8 *tmp = &tpu->pwr_ofst_mode[0];
	
	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);

	if (bb->ic_type == BB_RLE1115 || bb->ic_type == BB_RTL8922A)
		base_pwr_ofst_mode = 0x11A30;
	else 
		return;
	
	cr = base_pwr_ofst_mode;
	rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, NIBBLE_2_DWORD(tmp[7],tmp[6],tmp[5],tmp[4],tmp[3],tmp[2],tmp[1],tmp[0]));

}

void halbb_bb_wrap_be_set_pwr_ofst_bw_all(struct bb_info *bb, enum phl_phy_idx phy_idx)
{
	struct bb_tpu_be_info *tpu = &bb->hal_com->band[phy_idx].bb_tpu_all_i.bb_tpu_be_i;

	u32 cr, base_pwr_ofst_bw;
	s8 *tmp = &tpu->pwr_ofst_bw[0];

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);
	
	if (bb->ic_type == BB_RLE1115 || bb->ic_type == BB_RTL8922A)
		base_pwr_ofst_bw = 0x11A34;
	else 
		return;
	
	cr = base_pwr_ofst_bw;
	rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, NIBBLE_2_DWORD(tmp[7],tmp[6],tmp[5],tmp[4],tmp[3],tmp[2],tmp[1],tmp[0]));
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, (u8)phy_idx, cr + 0x4, 0xFF, BYTE_2_DWORD(0, 0, 0, NIBBLE_2_BYTE(tmp[9], tmp[8])));
}

void halbb_bb_wrap_be_set_limit_en_all(struct bb_info *bb, enum phl_phy_idx phy_idx)
{
	struct bb_tpu_be_info *tpu = &bb->hal_com->band[phy_idx].bb_tpu_all_i.bb_tpu_be_i;
	u32 cr, base_pwr_limit_en;

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);

	if (bb->ic_type == BB_RLE1115 || bb->ic_type == BB_RTL8922A)
		base_pwr_limit_en = 0x11A58;
	else 
		return;
	
	cr = base_pwr_limit_en;
	if (tpu->pwr_lmt_en) {
		rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BIT(12), 0x1);
		rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BIT(17), 0x1);
	}
	else {
		rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BIT(12), 0x0);
		rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, (u8)phy_idx, cr, BIT(17), 0x0);
	}

}

void halbb_bb_wrap_be_set_cck_dup_path(struct bb_info* bb, enum phl_phy_idx phy_idx)
{
	struct bb_tpu_be_info *tpu = &bb->hal_com->band[phy_idx].bb_tpu_all_i.bb_tpu_be_i;

	u32 cr, base_cck_dup_path = 0, val = 0;

	switch (bb->ic_type) {
		
	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		base_cck_dup_path = 0x11CF0;
		val = (tpu->pwr_cck_dup_patha_h_pathb_l_2tx << 16) | (tpu->pwr_cck_dup_patha_l_pathb_h_2tx);
		break;
	#endif
	
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		base_cck_dup_path = 0x12044;
		val = (tpu->pwr_cck_dup_patha_h_pathb_l_2tx << 8) | (tpu->pwr_cck_dup_patha_l_pathb_h_2tx);
		break;
	#endif	

	default:
		break;
	}

	if (base_cck_dup_path == 0) {
		BB_WARNING("[%s]\n", "IC_TYPE Selected Wrong");
		return;
	}

	cr = base_cck_dup_path;
	rtw_hal_mac_set_pwr_reg(bb->hal_com, (u8)phy_idx, cr, val);	
}

void halbb_bb_wrap_be_pwr_and_cca_th_by_macid_init(struct bb_info* bb, enum phl_phy_idx phy_idx)
{
	u32 macid_idx, cr, base_macid_lmt = 0, max_macid = 32;

	switch (bb->ic_type) {
		
	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		base_macid_lmt = 0xE900;
		break;
	#endif
	
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		base_macid_lmt = 0xED00;
		break;
	#endif	
	
	default:
		break;
	}

	if (base_macid_lmt == 0) {
		BB_WARNING("[%s]\n", "IC_TYPE Selected Wrong");
		return;
	}

	for (macid_idx = 0; macid_idx < 4 * max_macid; macid_idx += 4) {
		cr = base_macid_lmt + macid_idx;
		rtw_hal_mac_write_bb_wrapper(bb->hal_com, cr, 0x03007F7F);
	}
}

void halbb_bb_wrap_be_tx_path_by_macid_init(struct bb_info* bb)
{
	u32 macid_idx, cr, max_macid = 0, base_tx_path_macid = 0, val = 0;
	
	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);

	switch (bb->ic_type) {
		
	#if defined(BB_1115_SUPPORT) || defined(BB_8922A_SUPPORT)
	case BB_RLE1115:
	case BB_RTL8922A:
		base_tx_path_macid = 0xE500;
		max_macid = 32;
		val  = 0x03C86000;
		break;
	#endif
	default:
		break;
	}
		
	if (val == 0) {
		BB_WARNING("[%s]\n", "IC_TYPE Selected Wrong");
		return;
	}

	for (macid_idx = 0; macid_idx < max_macid; macid_idx++) {
		cr = base_tx_path_macid + macid_idx*4;
		BB_DBG(bb, DBG_INIT, "0x%x = 0x%x\n", cr, val);
		rtw_hal_mac_write_bb_wrapper(bb->hal_com, cr, val);
	}
}

void halbb_bb_wrap_be_set_pwr_ref_all(struct bb_info *bb, enum phl_phy_idx phy_idx)
{
	struct bb_tpu_be_info *tpu = &bb->hal_com->band[phy_idx].bb_tpu_all_i.bb_tpu_be_i;
	u32 cr, base_pwr_ref;

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);

	if (bb->ic_type == BB_RLE1115 || bb->ic_type == BB_RTL8922A)
		base_pwr_ref = 0x11A20;
	else 
		return;
	
	cr = base_pwr_ref;
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, cr, 0x3FE, tpu->ref_pow_ofdm & 0x1ff);
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, cr, 0x7FC00, tpu->ref_pow_cck & 0x1ff);
}

void halbb_bb_wrap_be_set_pwr_cusofst_all(struct bb_info *bb, enum phl_phy_idx phy_idx)
{
	struct bb_tpu_be_info *tpu = &bb->hal_com->band[phy_idx].bb_tpu_all_i.bb_tpu_be_i;
	
	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);
	if (!(bb->ic_type == BB_RLE1115 || bb->ic_type == BB_RTL8922A))
		return;

	//0x11A20[27:19] bylim, 0x11A24[8:0] bylimbf, 0x11A2C[8:0] byrate, 0x11A44[17:9] byrulim, 0x11AE8[27:24] sw
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A20, 0xFF80000, (tpu->pwr_cusofst_bylim & 0x1ff));
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A24, 0x1FF, (tpu->pwr_cusofst_bylimbf & 0x1ff));
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A2C, 0x1FF, (tpu->pwr_cusofst_byrate & 0x1ff));
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A44, 0x3FE00, (tpu->pwr_cusofst_byrulim & 0x1ff));
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11AE8, 0xF000000, (tpu->pwr_cusofst_sw & 0xf));
}

#endif

void halbb_tmac_force_tx_pwr(struct bb_info *bb, s8 pw_val, u8 n_path, u8 dbw_idx, enum phl_phy_idx phy_idx)

{
	switch (bb->ic_type) {

	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		halbb_tmac_force_tx_pwr_1115(bb, pw_val, n_path, dbw_idx, phy_idx);
		break;
	#endif
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		halbb_tmac_force_tx_pwr_8922a(bb, pw_val, n_path, dbw_idx, phy_idx);
		break;
	#endif
	default:
		break;
	}
}

void halbb_bb_wrap_set_tx_src(struct bb_info *bb, u8 option, s8 pw_val,
			      u8 n_path, u8 dbw_idx, enum phl_phy_idx phy_idx)
{
	u32 base_txpwr_dbm = 0, mask_txpwr_dbm = 0, mask_txpwr_mac = 0;

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);

	/*tx-info control by BB CR: option=0->TMAC force txpwr, option=1->PMAC force txpwr, option=2->Default txpwr*/
	pw_val = pw_val & 0xFF;

	switch (bb->ic_type) {
		
	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		base_txpwr_dbm = 0x4FC0;
		mask_txpwr_dbm = 0x3FE00;
		mask_txpwr_mac = 0x01F80000;
		break;
	#endif
	
	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		base_txpwr_dbm = 0x6908;
		mask_txpwr_dbm = 0x1FF;
		mask_txpwr_mac = 0x03F00000;
		break;
	#endif	

	default:
		break;
	}

	if (base_txpwr_dbm == 0){
		BB_WARNING("[%s]\n", "IC_TYPE Selected Wrong");
		return;
	}

	if (option == 0) {
		if (bb->bb_80211spec == BB_AX_IC){
			BB_WARNING("Command not support for non-BE chip!!\n");
			return;
		} else{
			halbb_set_reg_cmn(bb, 0x9a4, BIT(10), 0, phy_idx);
			halbb_tmac_force_tx_pwr(bb, pw_val, n_path, dbw_idx, phy_idx);
		}
	}
	if (option == 1) {
		if (bb->bb_80211spec == BB_AX_IC) {
			halbb_set_reg_cmn(bb, 0x9a4, BIT(16), 1, phy_idx);
			halbb_set_reg_cmn(bb, 0x4594, 0x7FC00000, (s16)(pw_val << 2), phy_idx);
		} else {
			halbb_set_reg_cmn(bb, 0x9a4, BIT(10), 1, phy_idx);
			halbb_set_reg_cmn(bb, base_txpwr_dbm, mask_txpwr_dbm, (s16)(pw_val << 2), phy_idx);
		}
	}
	if (option == 2) {
		if (bb->bb_80211spec == BB_AX_IC){
			halbb_set_reg_cmn(bb, 0x9a4, BIT(16), 0, HW_PHY_0);
			halbb_set_reg_cmn(bb, 0x9a4, BIT(16), 0, HW_PHY_1);
		}
		else {
			struct rtw_hal_com_t *hal_com = bb->hal_com;
			halbb_set_reg_cmn(bb, 0x9a4, BIT(10), 0, HW_PHY_0);
			halbb_set_reg_cmn(bb, 0x9a4, BIT(10), 0, HW_PHY_1);
			rtw_hal_mac_write_msk_pwr_reg(hal_com, HW_PHY_0, 0x11964, 0x00000060, 0);
			rtw_hal_mac_write_msk_pwr_reg(hal_com, HW_PHY_0, 0x11908, 0x7FC00000, 0);
			rtw_hal_mac_write_msk_pwr_reg(hal_com, HW_PHY_0, 0x11924, mask_txpwr_mac, 0);
			rtw_hal_mac_write_msk_pwr_reg(hal_com, HW_PHY_1, 0x11964, 0x00000060, 0);
			rtw_hal_mac_write_msk_pwr_reg(hal_com, HW_PHY_1, 0x11908, 0x7FC00000, 0);
			rtw_hal_mac_write_msk_pwr_reg(hal_com, HW_PHY_1, 0x11924, mask_txpwr_mac, 0);
		}
	}
}

void halbb_bb_wrap_set_pow_by_rate_all(struct bb_info *bb, enum phl_phy_idx phy_idx) {

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);
	
	if (bb->bb_80211spec == BB_AX_IC) {
		rtw_hal_mac_write_pwr_by_rate_reg(bb->hal_com, (enum phl_band_idx)phy_idx);
	} else {
		halbb_bb_wrap_be_set_pow_by_rate_all(bb, (enum phl_band_idx)phy_idx);
	}
}

void halbb_bb_wrap_set_pwr_limit_rua_all(struct bb_info *bb, enum phl_phy_idx phy_idx) {

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);
	
	if (bb->bb_80211spec == BB_AX_IC) {
		rtw_hal_mac_write_pwr_limit_rua_reg(bb->hal_com, (enum phl_band_idx)phy_idx);
	} else {
		halbb_bb_wrap_be_set_pwr_limit_rua_all(bb, (enum phl_band_idx)phy_idx);
	}
}

void halbb_bb_wrap_set_pwr_limit_all(struct bb_info *bb, enum phl_phy_idx phy_idx) {

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);
	
	if (bb->bb_80211spec == BB_AX_IC) {
		rtw_hal_mac_write_pwr_limit_reg(bb->hal_com, (enum phl_band_idx)phy_idx);
	} else {
		halbb_bb_wrap_be_set_pwr_limit_all(bb, (enum phl_band_idx)phy_idx);
	}
}

void halbb_bb_wrap_set_pwr_ofst_mode_all(struct bb_info *bb, enum phl_phy_idx phy_idx) {

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);
	
	if (bb->bb_80211spec == BB_AX_IC) {
		rtw_hal_mac_write_pwr_ofst_mode(bb->hal_com, (enum phl_band_idx)phy_idx);
	} else {
		halbb_bb_wrap_be_set_pwr_ofst_mode_all(bb, phy_idx);
	}
}

void halbb_bb_wrap_set_pwr_ofst_bw_all(struct bb_info *bb, enum phl_phy_idx phy_idx) {

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);
	
	if (bb->bb_80211spec == BB_AX_IC) {
		rtw_hal_mac_write_pwr_ofst_bw(bb->hal_com, (enum phl_band_idx)phy_idx);
	} else {
		halbb_bb_wrap_be_set_pwr_ofst_bw_all(bb, phy_idx);
	}
}

void halbb_bb_wrap_set_pwr_limit_en(struct bb_info *bb, enum phl_phy_idx phy_idx) {

	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);
	
	if (bb->bb_80211spec == BB_AX_IC) {
		rtw_hal_mac_write_pwr_limit_en(bb->hal_com, (enum phl_band_idx)phy_idx);
	} else {
		halbb_bb_wrap_be_set_limit_en_all(bb, phy_idx);
	}
}

#ifdef BB_1115_SUPPORT
void halbb_bb_wrap_stl_ul_pwr_ctrl_init(struct bb_info* bb){

	struct rtw_hal_com_t *hal_com = bb->hal_com;

	//MAC0
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11A1C, 0xb0aa8000);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11A24, 0x17fbfc00);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11A60, 0x00180000);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11A68, 0x00000040);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11A78, 0x00fffe7e);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11A80, 0x007fe03f);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11A84, 0x0201FE00);
	//MAC1
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11A1C, 0xb0aa8000);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11A24, 0x17fbfc00);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11A60, 0x00180000);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11A68, 0x00000040);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11A78, 0x00fffe7e);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11A80, 0x007fe03f);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11A84, 0x0201FE00);
}

void halbb_bb_wrap_force_txpwr_dbm_init(struct bb_info* bb)
{
	struct rtw_hal_com_t *hal_com = bb->hal_com;

	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11964, 0x00000060); //bb_wrapper force txpwr dbm/mac en = 1
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11908, 0x2000000); //powrcom_txpwr_dbm_val for 2dBm per path on total DBW
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11964, 0x00000060); //bb_wrapper force txpwr dbm/mac en = 1
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11908, 0x2000000); //powrcom_txpwr_dbm_val for 2dBm per path on total DBW
}
#endif

#ifdef BB_8922A_SUPPORT
void halbb_bb_wrap_8922A_Acut_ul_power_cal_fix_error_n_fine_tune(struct bb_info* bb)
{
	struct rtw_hal_com_t* hal_com = bb->hal_com;

	// fix CR setting error: r_STA_pwr_ctrl_RSSI_Target_lim_max; 11a84[17:10] = 0x7f
	// fix CR setting error: r_STA_pwr_ctrl_RSSI_Target_lim_min; 11a84[25:18] = 0x80
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11A84, 0x0201FE00);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11A84, 0x0201FE00);

	// fix CR setting error: r_PWR_bb_min_dbm should follow IEEE Std.; 0x11a78[15:7] = 0x1D8
	// fine tune: r_PWR_error_toler 0x11a78[23:16] = 0xff
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_0, 0x11A78, 0x00ffec7e);
	rtw_hal_mac_set_pwr_reg(hal_com, HW_PHY_1, 0x11A78, 0x00ffec7e);
	
}
#endif

void halbb_bb_wrap_tpu_set_all(struct bb_info* bb, enum phl_phy_idx phy_idx)
{
	BB_DBG(bb, DBG_PWR_CTRL, "[%s] %d", __func__, phy_idx);

	#ifdef BB_1115_SUPPORT
	if (bb->ic_type == BB_RLE1115) {
		halbb_bb_wrap_force_txpwr_dbm_init(bb);
		halbb_bb_wrap_stl_ul_pwr_ctrl_init(bb);
		return;
	}
	#endif
		
	halbb_bb_wrap_set_pow_by_rate_all(bb, phy_idx);
	halbb_bb_wrap_set_pwr_limit_all(bb, phy_idx);
	halbb_bb_wrap_set_pwr_limit_rua_all(bb, phy_idx);

	halbb_bb_wrap_set_pwr_ofst_mode_all(bb, phy_idx);
	halbb_bb_wrap_set_pwr_ofst_bw_all(bb, phy_idx);

	/*BE only APIS*/
	if (bb->bb_80211spec == BB_BE_IC) {
		halbb_bb_wrap_be_set_cck_dup_path(bb, phy_idx);
		halbb_bb_wrap_be_set_pwr_cusofst_all(bb, phy_idx);
	}
}

void  halbb_bb_wrap_be_listen_path_en_init(struct bb_info* bb)
{
	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);
		
	if (bb->ic_type != BB_RTL8922A)
		return;	

	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, HW_PHY_1, 0x11988, 0xF0000000, 0x2);
}

void halbb_bb_wrap_be_ftm_init(struct bb_info* bb, enum phl_phy_idx phy_idx)
{
	u32 cr = 0, val = 0, val2 = 0, mask = 0;
	
	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);
	
	switch (bb->ic_type) {

	#ifdef BB_8922A_SUPPORT
		case BB_RTL8922A:
			cr = 0x11B00;
			val = 0xE4E431;
			val2 = 0;
			mask = 0x7;
			break;
	#endif
		default:
			break;
	}
	
	if (cr == 0){
		BB_WARNING("[FTM] without initialization");
		return;
	}
	
	rtw_hal_mac_set_pwr_reg(bb->hal_com, phy_idx, cr, val);
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, (cr + 4), mask, val2);
}

void halbb_bb_wrap_be_force_cr_init(struct bb_info* bb, enum phl_phy_idx phy_idx)
{
	BB_DBG(bb, DBG_PWR_CTRL, "[%s]", __func__);

	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A28, 0x40, 0);
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A40, 0x20000000, 0);
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A44, 0x10000000, 0);
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A44, 0x40000, 0);
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A48, 0x200, 0);
	rtw_hal_mac_write_msk_pwr_reg(bb->hal_com, phy_idx, 0x11A54, 0x38000000, 0);
}


void halbb_bb_wrap_init(struct bb_info *bb_0, enum phl_phy_idx phy_idx) {

	struct bb_info *bb = bb_0;

	if (bb->bb_80211spec != BB_BE_IC)
		return;

#ifdef HALBB_DBCC_SUPPORT
	HALBB_GET_PHY_PTR(bb_0, bb, phy_idx);
#endif
	BB_DBG(bb, DBG_COMMON_FLOW, "[%s] phy_idx=%d\n", __func__, phy_idx);

	halbb_bb_wrap_be_pwr_and_cca_th_by_macid_init(bb, phy_idx);
	halbb_bb_wrap_be_tx_path_by_macid_init(bb);
	halbb_bb_wrap_be_listen_path_en_init(bb);
	halbb_bb_wrap_be_force_cr_init(bb, phy_idx);
	halbb_bb_wrap_be_ftm_init(bb, phy_idx);
	halbb_bb_wrap_tpu_set_all(bb, phy_idx); /*Init only for debug*/
	
	#ifdef BB_8922A_SUPPORT
	if (bb->ic_type == BB_RTL8922A && bb->hal_com->cv == CAV) {
		halbb_bb_wrap_8922A_Acut_ul_power_cal_fix_error_n_fine_tune(bb); /*fix 8922A_Acut UL OFDMA setting error & param fine tune*/
	}
	#endif
}

void halbb_bb_wrap_dbg_be(struct bb_info *bb, char input[][16], u32 *_used,
			  char *output, u32 *_out_len)
{
	struct bb_tpu_be_info *tpu = NULL;
	struct bb_tpu_pwr_by_rate_info_be *by_rate = NULL;
	struct bb_tpu_pwr_lmt_info_be *lmt = NULL;
	struct bb_tpu_pwr_lmt_ru_info_be *lmt_ru = NULL;
	u16 size_tmp = 0;
	u32 val[10] = {0};
	u16 i = 0, j = 0, k = 0;
	u8 rate_idx = 0, path = 0, dbw_idx = 0, tbl_idx = 0;
	u32 result = 0;
	u32 val32 = 0;
	s8 val_s8 = 0;
	s8 *tmp_s8 = NULL;
#if 0
	u32 addr;
	enum phl_band_idx band;
	bool rpt_tmp;
	u32 offset;
	u32 base;
#endif

	if (bb->bb_80211spec != BB_BE_IC) {
		BB_WARNING("[%s]\n", __func__);
		return;
	}

	tpu = &bb->hal_com->band[bb->bb_phy_idx].bb_tpu_all_i.bb_tpu_be_i;
	by_rate = &tpu->rtw_tpu_pwr_by_rate_be_i;
	lmt = &tpu->rtw_tpu_pwr_lmt_be_i;
	lmt_ru = &tpu->rtw_tpu_pwr_lmt_ru_be_i;

	BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "[BB-Wrapper CTRL]\n");

	if (_os_strcmp(input[1], "-h") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "tpu\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "dbg_en {en}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "show\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "set all {s(7,1) dB}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "set cck {dbw} {idx} {s(7,1) dB}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "set ofdm {dbw} {idx} {s(7,1) dB}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "set mcs {dbw} {path:0~3} {idx} {s(7,1) dB}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "set eht {dbw} {idx} {s(7,1) dB}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "set dlru {dbw} {path:0~3} {idx} {s(7,1) dB}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "cck_dup {s(7,1) dBm for cck_dup_l_h} {s(7,1) dBm for cck_dup_h_l}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "cusofst {bylim, bylimbf, byrate, byrulim, sw} {non-sw: s(9,2), sw:u(4.0) dB}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "ofst bw {all:255, 0~9: 20, 40, 80, 160, 320, dlru_20, dlru_40, dlru_80, dlru_160, dlru_320} {0:+, 1:-} {s(4,1) dB}\n");	
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "ofst mode {all:255 0~7: CCK, Legacy, HT, VHT, HE, EHT, DLRU_HE, DLRU_EHT} {0:+, 1:-} {s(4,1) dB}\n");				
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "ref {cck, ofdm} {s(9,2) dB}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "ref ofst {s(8,3) dB}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "cw {0:rf_0db_cw(39), 1:tssi_16dBm_cw(300)} {val}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "lmt en {en}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "lmt {all, ru_all} {s(7,1) dB}\n");
		//BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		//	 "tb_ofst {s(5,0) dB}\n");
		//BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		//	 "tx_shap {ch} {shap_idx} {is_ofdm}\n");
		//BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		//	 "tpu 0\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "tx_src {0:tmac_frc_txpwr, 1:pmac_frc_txpwr, 2:Default} {pw_val(dB) per path} {#total path: 1, 2, 3, 4} {DBW_idx 0:20M, 1:40M, 2:80M, 3:160M, 4:320M} {phy_idx: 0, 1}\n");
		//BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		//	 "frc_pmac {pw_val(dbm)} {phy_idx}\n");

		return;
	}
	if (_os_strcmp(input[1], "tpu") == 0) {
		halbb_set_tx_pow_ref(bb, bb->bb_phy_idx);
		halbb_bb_wrap_tpu_set_all(bb, bb->bb_phy_idx);

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "Set all TPU component\n");
	} else if (_os_strcmp(input[1], "dbg_en") == 0) {
		HALBB_SCAN(input[2], DCMD_HEX, &val[0]);
		rtw_hal_mac_set_tpu_mode(bb->hal_com, (enum rtw_tpu_op_mode)val[0], bb->bb_phy_idx);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "dbg_en=%d, Locking driver set TPU = %d\n", val[0], tpu->normal_mode_lock_en);
	} else if (_os_strcmp(input[1], "tx_src") == 0) {
		HALBB_SCAN(input[2], DCMD_HEX, &val[0]); /*option*/
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[1]); /*pw S(9,2)*/
		HALBB_SCAN(input[4], DCMD_DECIMAL, &val[2]); /*#path: 1, 2, 3, 4*/
		HALBB_SCAN(input[5], DCMD_DECIMAL, &val[3]); /*DBW_idx 0:20M, 1:40M, 2:80M, 3:160M, 4:320M*/
		HALBB_SCAN(input[6], DCMD_DECIMAL, &val[4]); /*phy_idx*/
	
		if ((u8)val[0] == 0) {
			if ((val[4] != 0 && val[4] != 1) || ((u8)val[2] <= 0 || (u8)val[2] > HAL_MAX_PATH) || ((u8)val[3] >= 5)){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"Set Err\n");
				return;
			}
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"Total path = %d\n", val[2]);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"Force TMAC tx_pwr for phy_idx[%d] = %d dBm per path on DBW %d MHz\n", 
				(enum phl_phy_idx)val[4], val[1], 
				(((u8)val[3]==0)?20:(((u8)val[3]==1)?40:(((u8)val[3]==2)?80:(((u8)val[3]==3)?160:(((u8)val[3]==4)?320:0))))));
		} else if ((u8)val[0] == 1) {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"Force PMAC tx_pwr for phy_idx[%d] = %d dBm\n", (enum phl_phy_idx)val[4], val[1]);
		} else if ((u8)val[0] == 2) {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"Default Txpwr for both phy_idx\n");
		} else {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "Set Err\n");
			return;
		}
		halbb_bb_wrap_set_tx_src(bb, (u8)val[0], (s8)val[1], (u8)val[2], (u8)val[3], (enum phl_phy_idx)val[4]);
#if 1
	} else if (_os_strcmp(input[1], "show") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[PW Ref]\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "%-10s {%d}\n", "[base_cw_0db]", tpu->base_cw_0db);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "%-10s {%s dB}\n", "[path_B_ofst]",
			    halbb_print_sign_frac_digit2(bb, tpu->ofst_int, 8, 3));
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "%-10s {(%d.%d) dBm} pw_cw=0x%03x\n", "[CCK]",
			    (tpu->ref_pow_cck >> 2) & (s16)0x00ff, (tpu->ref_pow_cck & 0x3) * 25, 
			    tpu->ref_pow_cck_cw);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "%-10s {(%d.%d) dBm} pw_cw=0x%03x\n", "[OFDM]",
			    (tpu->ref_pow_ofdm >> 2) & (s16)0x00ff, (tpu->ref_pow_ofdm & 0x3) * 25,
			    tpu->ref_pow_ofdm_cw);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[PW Offset] (s41)\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "%-20s {%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n",
			     "[B/G/N/AC/AX/BE/DLRU_AX/DLRU_BE]",
			    tpu->pwr_ofst_mode[0], tpu->pwr_ofst_mode[1],
			    tpu->pwr_ofst_mode[2], tpu->pwr_ofst_mode[3],
			    tpu->pwr_ofst_mode[4], tpu->pwr_ofst_mode[5],
			    tpu->pwr_ofst_mode[6], tpu->pwr_ofst_mode[7]);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "%-20s {%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n",
			     "[20/40/80/160/320/dlru_20/dlru_40/dlru_80/dlru_160/dlru_320]",
			    tpu->pwr_ofst_bw[0], tpu->pwr_ofst_bw[1],
			    tpu->pwr_ofst_bw[2], tpu->pwr_ofst_bw[3],
			    tpu->pwr_ofst_bw[4], tpu->pwr_ofst_bw[5],
			    tpu->pwr_ofst_bw[6], tpu->pwr_ofst_bw[7],
			    tpu->pwr_ofst_bw[8], tpu->pwr_ofst_bw[9]);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[PW Cusofst] (s92)\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "%-20s {%02d, %02d, %02d, %02d, %02d}\n","[bylim/bylimbf/byrate/byrulim/sw]",
			    tpu->pwr_cusofst_bylim, tpu->pwr_cusofst_bylimbf,
			    tpu->pwr_cusofst_byrate, tpu->pwr_cusofst_byrulim,tpu->pwr_cusofst_sw);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Pwr cck_dup] (s71)\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "%-20s {%02d, %02d}\n",
			     "[patha_h_pathb_l/patha_l_pathb_h]",
			    tpu->pwr_cck_dup_patha_h_pathb_l_2tx, tpu->pwr_cck_dup_patha_l_pathb_h_2tx);

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Pwr By Rate] (s71)\n");
		for (i = 0; i < TPU_SIZE_PWR_TAB_DBW_CCK; i++) { //1: only for rle1115. for1115 or above, 1->TPU_SIZE_PWR_TAB_DBW_CCK
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, "DBW_IDX(0:20,1:40): [%d]\n", i);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    	    "%-10s {%02d, %02d, %02d, %02d}\n", "[CCK]",
			    	    by_rate->pwr_by_rate_cck[i][0], by_rate->pwr_by_rate_cck[i][1],
			    	    by_rate->pwr_by_rate_cck[i][2], by_rate->pwr_by_rate_cck[i][3]);
			if (bb->ic_type == BB_RLE1115)
				break;
		}
	
		for (i = 0; i < TPU_SIZE_PWR_TAB_DBW; i++) {  //1: only for rle1115. for1115 or above, 1->TPU_SIZE_PWR_TAB_DBW
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, "DBW_IDX(0:20,1:40,2:80,3:160,4:320): [%d]\n", i);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "%-10s {%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n","[Lgcy]",
				    by_rate->pwr_by_rate_ofdm[i][0], by_rate->pwr_by_rate_ofdm[i][1],
				    by_rate->pwr_by_rate_ofdm[i][2], by_rate->pwr_by_rate_ofdm[i][3],
				    by_rate->pwr_by_rate_ofdm[i][4], by_rate->pwr_by_rate_ofdm[i][5],
				    by_rate->pwr_by_rate_ofdm[i][6], by_rate->pwr_by_rate_ofdm[i][7]);
			if (bb->ic_type == BB_RLE1115)
				break;
		}
		
		for (i = 0; i < TPU_SIZE_PWR_TAB_DBW; i++) { //1: only for rle1115. for1115 or above, 1->TPU_SIZE_PWR_TAB_DBW
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, "DBW_IDX(0:20,1:40,2:80,3:160,4:320): [%d]\n", i);
			for (j = 0; j < HALBB_MAX_PATH; j++) {
				if (j >= bb->num_rf_path)
					break;

				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, 
				"[%d]%-7s {%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n", j, "[OFDM]",
					    by_rate->pwr_by_rate_mcs[i][j][0], by_rate->pwr_by_rate_mcs[i][j][1],
					    by_rate->pwr_by_rate_mcs[i][j][2], by_rate->pwr_by_rate_mcs[i][j][3],
					    by_rate->pwr_by_rate_mcs[i][j][4], by_rate->pwr_by_rate_mcs[i][j][5],
					    by_rate->pwr_by_rate_mcs[i][j][6], by_rate->pwr_by_rate_mcs[i][j][7],
					    by_rate->pwr_by_rate_mcs[i][j][8], by_rate->pwr_by_rate_mcs[i][j][9],
					    by_rate->pwr_by_rate_mcs[i][j][10], by_rate->pwr_by_rate_mcs[i][j][11],
					    by_rate->pwr_by_rate_mcs[i][j][12], by_rate->pwr_by_rate_mcs[i][j][13]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "[%d]%-7s {%02d, %02d, %02d, %02d}\n", j, "[OFDM-DCM]",
					    by_rate->pwr_by_rate_mcs[i][j][14], by_rate->pwr_by_rate_mcs[i][j][15],
					    by_rate->pwr_by_rate_mcs[i][j][16], by_rate->pwr_by_rate_mcs[i][j][17]);
			}
			if (bb->ic_type == BB_RLE1115)
				break;
		}

		for (i = 0; i < TPU_SIZE_PWR_TAB_DBW; i++) {  //1: only for rle1115. for1115 or above, 1->TPU_SIZE_PWR_TAB_DBW
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, "DBW_IDX(0:20,1:40,2:80,3:160,4:320): [%d]\n", i);
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,"%-10s {%02d, %02d, %02d, %02d}\n","[EHT]",
				    by_rate->pwr_by_rate_eht[i][0], by_rate->pwr_by_rate_eht[i][1],
				    by_rate->pwr_by_rate_eht[i][2], by_rate->pwr_by_rate_eht[i][3]);
			if (bb->ic_type == BB_RLE1115)
				break;
		}

		for (i = 0; i < TPU_SIZE_PWR_TAB_DBW; i++) { //1: only for rle1115. for1115 or above, 1->TPU_SIZE_PWR_TAB_DBW

			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, "DBW_IDX(0:20,1:40,2:80,3:160,4:320): [%d]\n", i);
			for (j = 0; j < HALBB_MAX_PATH; j++) {
				if (j >= bb->num_rf_path)
					break;

				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "[%d]%-7s {%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n",
					     j, "[DLRU]",
					    by_rate->pwr_by_rate_dlru[i][j][0], by_rate->pwr_by_rate_dlru[i][j][1],
					    by_rate->pwr_by_rate_dlru[i][j][2], by_rate->pwr_by_rate_dlru[i][j][3],
					    by_rate->pwr_by_rate_dlru[i][j][4], by_rate->pwr_by_rate_dlru[i][j][5],
					    by_rate->pwr_by_rate_dlru[i][j][6], by_rate->pwr_by_rate_dlru[i][j][7],
					    by_rate->pwr_by_rate_dlru[i][j][8], by_rate->pwr_by_rate_dlru[i][j][9],
					    by_rate->pwr_by_rate_dlru[i][j][10], by_rate->pwr_by_rate_dlru[i][j][11],
					    by_rate->pwr_by_rate_dlru[i][j][12], by_rate->pwr_by_rate_dlru[i][j][13]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "[%d]%-7s {%02d, %02d, %02d, %02d}\n", j, "[DLRU-DCM]",
					    by_rate->pwr_by_rate_dlru[i][j][14], by_rate->pwr_by_rate_dlru[i][j][15],
					    by_rate->pwr_by_rate_dlru[i][j][16], by_rate->pwr_by_rate_dlru[i][j][17]);
				if (bb->ic_type == BB_RLE1115)
					break;
			}
		}

		for (j = 0; j < TPU_SIZE_BF; j++) {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");

			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    	"[Pwr Lmt][%sBF]\n", (j == 0) ? "non-" : "");

			for (i = 0; i < HALBB_MAX_PATH; i++) {

				if (i >= bb->num_rf_path)
					break;

				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d}\n", "[CCK-20M]", i,
					    lmt->pwr_lmt_cck[i][0][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d}\n", "[CCK-40M]", i, 
					    lmt->pwr_lmt_cck[i][1][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d}\n", "[Lgcy_non_dup]", i, 
					    lmt->pwr_lmt_lgcy_non_dup[i][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n",
					    "[OFDM-20M]",i, 
					    lmt->pwr_lmt_20m[i][0][j], lmt->pwr_lmt_20m[i][1][j],
					    lmt->pwr_lmt_20m[i][2][j], lmt->pwr_lmt_20m[i][3][j],
					    lmt->pwr_lmt_20m[i][4][j], lmt->pwr_lmt_20m[i][5][j],
					    lmt->pwr_lmt_20m[i][6][j], lmt->pwr_lmt_20m[i][7][j],
					    lmt->pwr_lmt_20m[i][8][j], lmt->pwr_lmt_20m[i][9][j],
					    lmt->pwr_lmt_20m[i][10][j], lmt->pwr_lmt_20m[i][11][j],
					    lmt->pwr_lmt_20m[i][12][j], lmt->pwr_lmt_20m[i][13][j],
					    lmt->pwr_lmt_20m[i][14][j], lmt->pwr_lmt_20m[i][15][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n", "[OFDM-40M]", i, 
					    lmt->pwr_lmt_40m[i][0][j], lmt->pwr_lmt_40m[i][1][j],
					    lmt->pwr_lmt_40m[i][2][j], lmt->pwr_lmt_40m[i][3][j],
					    lmt->pwr_lmt_40m[i][4][j], lmt->pwr_lmt_40m[i][5][j],
					    lmt->pwr_lmt_40m[i][6][j], lmt->pwr_lmt_40m[i][7][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d, %02d, %02d, %02d}\n", "[OFDM-80M]", i, 
					    lmt->pwr_lmt_80m[i][0][j], lmt->pwr_lmt_80m[i][1][j],
					    lmt->pwr_lmt_80m[i][1][j], lmt->pwr_lmt_80m[i][3][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s[%d]{%02d, %02d}\n", "[OFDM-160M]", i, 
					    lmt->pwr_lmt_160m[i][0][j], lmt->pwr_lmt_160m[i][1][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s[%d]{%02d}\n", "[OFDM-320M]", i, 
					    lmt->pwr_lmt_320m[i][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d}\n", "[40m_0p5]", i, 
					    lmt->pwr_lmt_40m_0p5[i][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d}\n", "[40m_2p5]", i, 
					    lmt->pwr_lmt_40m_2p5[i][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d}\n", "[40m_4p5]", i, 
					    lmt->pwr_lmt_40m_4p5[i][j]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d}\n", "[40m_6p5]", i, 
					    lmt->pwr_lmt_40m_6p5[i][j]);
			}
		}
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Pwr Lmt RUA]\n");

		for (j = 0; j < TPU_SIZE_RUA; j++) {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 	    "[RU-%3d]\n", (j == 0) ? 26 : ((j == 1) ? 52 : 106));

			#if 1
			for (i = 0; i < HALBB_MAX_PATH; i++) {
				if (i >= bb->num_rf_path)
					break;
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s [%d]{%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n",
					    "[OFDM-20M]", i,
					    lmt_ru->pwr_lmt_ru_be[i][j][0], lmt_ru->pwr_lmt_ru_be[i][j][1],
					    lmt_ru->pwr_lmt_ru_be[i][j][2], lmt_ru->pwr_lmt_ru_be[i][j][3],
					    lmt_ru->pwr_lmt_ru_be[i][j][4], lmt_ru->pwr_lmt_ru_be[i][j][5],
					    lmt_ru->pwr_lmt_ru_be[i][j][6], lmt_ru->pwr_lmt_ru_be[i][j][7], 
					    lmt_ru->pwr_lmt_ru_be[i][j][8], lmt_ru->pwr_lmt_ru_be[i][j][9],
					    lmt_ru->pwr_lmt_ru_be[i][j][10], lmt_ru->pwr_lmt_ru_be[i][j][11],
					    lmt_ru->pwr_lmt_ru_be[i][j][12], lmt_ru->pwr_lmt_ru_be[i][j][13],
					    lmt_ru->pwr_lmt_ru_be[i][j][14], lmt_ru->pwr_lmt_ru_be[i][j][15]);
			}
			#endif
		}

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Pwr Lmt MRU]\n");

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"[RU-52_26]\n");

		#if 1
		for (i = 0; i < HALBB_MAX_PATH; i++) {
			if (i >= bb->num_rf_path)
				break;
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "%-10s [%d]{%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n",
				    "[OFDM-20M]", i,
				    lmt_ru->pwr_lmt_ru52_26_be[i][0], lmt_ru->pwr_lmt_ru52_26_be[i][1],
				    lmt_ru->pwr_lmt_ru52_26_be[i][2], lmt_ru->pwr_lmt_ru52_26_be[i][3],
				    lmt_ru->pwr_lmt_ru52_26_be[i][4], lmt_ru->pwr_lmt_ru52_26_be[i][5],
				    lmt_ru->pwr_lmt_ru52_26_be[i][6], lmt_ru->pwr_lmt_ru52_26_be[i][7], 
				    lmt_ru->pwr_lmt_ru52_26_be[i][8], lmt_ru->pwr_lmt_ru52_26_be[i][9],
				    lmt_ru->pwr_lmt_ru52_26_be[i][10], lmt_ru->pwr_lmt_ru52_26_be[i][11],
				    lmt_ru->pwr_lmt_ru52_26_be[i][12], lmt_ru->pwr_lmt_ru52_26_be[i][13],
				    lmt_ru->pwr_lmt_ru52_26_be[i][14], lmt_ru->pwr_lmt_ru52_26_be[i][15]);
		}
		#endif

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Pwr Lmt MRU]\n");

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"[RU-106_26]\n");

		#if 1
		for (i = 0; i < HALBB_MAX_PATH; i++) {
			if (i >= bb->num_rf_path)
				break;
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "%-10s [%d]{%02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d, %02d}\n",
				    "[OFDM-20M]", i,
				    lmt_ru->pwr_lmt_ru106_26_be[i][0], lmt_ru->pwr_lmt_ru106_26_be[i][1],
				    lmt_ru->pwr_lmt_ru106_26_be[i][2], lmt_ru->pwr_lmt_ru106_26_be[i][3],
				    lmt_ru->pwr_lmt_ru106_26_be[i][4], lmt_ru->pwr_lmt_ru106_26_be[i][5],
				    lmt_ru->pwr_lmt_ru106_26_be[i][6], lmt_ru->pwr_lmt_ru106_26_be[i][7], 
				    lmt_ru->pwr_lmt_ru106_26_be[i][8], lmt_ru->pwr_lmt_ru106_26_be[i][9],
				    lmt_ru->pwr_lmt_ru106_26_be[i][10], lmt_ru->pwr_lmt_ru106_26_be[i][11],
				    lmt_ru->pwr_lmt_ru106_26_be[i][12], lmt_ru->pwr_lmt_ru106_26_be[i][13],
				    lmt_ru->pwr_lmt_ru106_26_be[i][14], lmt_ru->pwr_lmt_ru106_26_be[i][15]);
		}
		#endif

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "================\n\n");
	} else if (_os_strcmp(input[1], "cw") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[1]);
		if (val[0] == 0) {
			tpu->base_cw_0db = (u8)val[1];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "rf_cw_0dbm=%d\n", tpu->base_cw_0db);
		} else if (val[0] == 1) {
			tpu->tssi_16dBm_cw = (u16)val[1];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "tssi_16dBm_cw=%d\n", tpu->tssi_16dBm_cw);
		} else {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "Set Err\n");
			return;
		}
		halbb_set_tx_pow_ref(bb, bb->bb_phy_idx);
	} else if (_os_strcmp(input[1], "ofst") == 0) {
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[4], DCMD_DECIMAL, &val[1]);
		HALBB_SCAN(input[5], DCMD_DECIMAL, &val[2]);

		val[2] = val[2] << 1;
		if (val[1] == 0) {
			if (val[2] > 7) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "[Set Err] max = +3.5 dB\n");
				return;
			}

			val_s8 = (s8)val[2];
		} else {
			if (val[2] > 8) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "[Set Err] max = -4 dB\n");
				return;
			}
			val_s8 = (s8)val[2] * -1;
		}

		if (_os_strcmp(input[2], "bw") == 0) {
			if (val[0] == 255) {
				for (i = 0; i < TPU_SIZE_BW_BE; i++)
					tpu->pwr_ofst_bw[i] = val_s8 & 0xf;

			} else if (val[0] >= TPU_SIZE_BW_BE) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "Set Err\n");
				return;
			} else {
				tpu->pwr_ofst_bw[val[0]] = val_s8 & 0xf;
			}
			halbb_bb_wrap_set_pwr_ofst_bw_all(bb, (enum phl_band_idx)bb->bb_phy_idx);
		} else if (_os_strcmp(input[2], "mode") == 0) {
			if (val[0] == 255) {
				for (i = 0; i < TPU_SIZE_MODE_BE; i++)
					tpu->pwr_ofst_mode[i] = val_s8 & 0xf;

			} else if (val[0] >= TPU_SIZE_MODE_BE) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "Set Err\n");
				return;
			} else {
				tpu->pwr_ofst_mode[val[0]] = val_s8 & 0xf;
			}
			halbb_bb_wrap_set_pwr_ofst_mode_all(bb, (enum phl_band_idx)bb->bb_phy_idx);
		} else {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "Set Err\n");
			return;
		}

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[%s]pw ofst[%d]=(%s%d.%d)dB\n",
			    (_os_strcmp(input[2], "bw") == 0) ? "BW" : "MODE",
			    val[0], (val[1] == 0) ? "+" : "-",
			    val[2] >> 1, (val[2] & 0x1) * 5);
	} else if (_os_strcmp(input[1], "ref") == 0) {
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);
		if (_os_strcmp(input[2], "ofst") == 0) {
			tpu->ofst_int = ((s8)val[0] << 3) & 0xff;
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "ref_ofst=(%s)dB\n",
				 halbb_print_sign_frac_digit2(bb, tpu->ofst_int, 8, 3));
		} else {
			if (_os_strcmp(input[2], "ofdm") == 0) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "%-10s ref_pw={%d dBm} cw=0x%09x\n", "[OFDM]",
					    val[0], tpu->ref_pow_ofdm_cw);
				tpu->ref_pow_ofdm = ((s16)val[0] << 2) & 0x1ff;
			} else if (_os_strcmp(input[2], "cck") == 0) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,   //wrong show
					    "%-10s ref_pw={%d dBm} cw=0x%09x\n", "[CCK]",
					    val[0], tpu->ref_pow_cck_cw);
				tpu->ref_pow_cck = ((s16)val[0] << 2) & 0x1ff; 
			} else {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "Set Err\n");
				return;
			}
			//halbb_print_sign_frac_digit2(bb, val[0], 32, 2);
			//BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used, "ref_pw = (%s)dBm\n", bb->dbg_buf);
		}
		halbb_set_tx_pow_ref(bb, bb->bb_phy_idx);
	} else if (_os_strcmp(input[1], "set") == 0) {
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[4], DCMD_DECIMAL, &val[1]);
		HALBB_SCAN(input[5], DCMD_DECIMAL, &val[2]);
		HALBB_SCAN(input[6], DCMD_DECIMAL, &val[3]);
		if (_os_strcmp(input[2], "cck") == 0) {
			dbw_idx = (u8)val[0];
			rate_idx = (u8)val[1];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "[TX Pw] CCK[%d][%d] = %d dBm\n", dbw_idx, rate_idx, val[2]);
			if (dbw_idx + 1 > TPU_SIZE_PWR_TAB_DBW_CCK){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "dbw_idx is out of range!!\n");
				return;
			}
			if (rate_idx + 1 > TPU_SIZE_PWR_TAB_CCK){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "rate_idx is out of range!!\n");
				return;
			}

			//*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_cck + dbw_idx ) + rate_idx) = (s8)val[2];
			tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_cck[dbw_idx][rate_idx] = ((s8)val[2] << 1) & 0x7f;
			
		} else if (_os_strcmp(input[2], "ofdm") == 0) {
			dbw_idx = (u8)val[0];
			rate_idx = (u8)val[1];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "[TX Pw] OFDM[%d][%d] = %d dBm\n", dbw_idx, rate_idx, val[2]);
			if (dbw_idx + 1 > TPU_SIZE_PWR_TAB_DBW){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "dbw_idx is out of range!!\n");
				return;
			}
			if (rate_idx + 1 > TPU_SIZE_PWR_TAB_OFDM){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "rate_idx is out of range!!\n");
				return;
			}
			//*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_ofdm + dbw_idx ) + rate_idx) = (s8)val[2];
			tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_ofdm[dbw_idx][rate_idx] = ((s8)val[2] << 1) & 0x7f;
		} else if (_os_strcmp(input[2], "mcs") == 0) {
			dbw_idx = (u8)val[0];
			path = (u8)val[1];
			rate_idx = (u8)val[2];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "[TX Pw] DBW[%d] Path[%d] MCS[%d] = %d dBm\n", dbw_idx, path, rate_idx, val[3]);
			if (dbw_idx + 1 > TPU_SIZE_PWR_TAB_DBW){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "dbw_idx is out of range!!\n");
				return;
			}
			if (path + 1 > HALBB_MAX_PATH){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "path_idx is out of range!!\n");
				return;
			}
			if (rate_idx + 1 > TPU_SIZE_PWR_TAB_BE){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "rate_idx is out of range!!\n");
				return;
			}
			//*(*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_mcs + dbw_idx) + path) + rate_idx) = (s8)val[3];
			tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_mcs[dbw_idx][path][rate_idx] = ((s8)val[3] << 1) & 0x7f;
			
		} else if (_os_strcmp(input[2], "eht") == 0) {
			dbw_idx = (u8)val[0];
			rate_idx = (u8)val[1];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "[TX Pw] EHT[%d][%d] = %d mdBm\n", dbw_idx, rate_idx, val[2]);
			if (dbw_idx + 1 > TPU_SIZE_PWR_TAB_DBW){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "dbw_idx is out of range!!\n");
				return;
			}
			if (rate_idx + 1 > TPU_SIZE_PWR_TAB_EHT_BE){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "rate_idx is out of range!!\n");
				return;
			}
			//*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_eht + dbw_idx) + rate_idx) = (s8)val[2];
			tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_eht[dbw_idx][rate_idx] = ((s8)val[2] << 1) & 0x7f;
			
		} else if (_os_strcmp(input[2], "dlru") == 0) {
			dbw_idx = (u8)val[0];
			path = (u8)val[1];
			rate_idx = (u8)val[2];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "[TX Pw] DBW[%d] Path[%d] MCS[%d] = %d dBm\n", dbw_idx, path, rate_idx, val[3]);
			if (dbw_idx + 1 > TPU_SIZE_PWR_TAB_DBW){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "dbw_idx is out of range!!\n");
				return;
			}
			if (path + 1 > HALBB_MAX_PATH){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "path_idx is out of range!!\n");
				return;
			}
			if (rate_idx + 1 > TPU_SIZE_PWR_TAB_BE){
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 	  "rate_idx is out of range!!\n");
				return;
			}
			//*(*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_dlru + dbw_idx) + path) + rate_idx) = (s8)val[3];
			tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_dlru[dbw_idx][path][rate_idx] = ((s8)val[3] << 1) & 0x7f;
		} else if (_os_strcmp(input[2], "all") == 0) {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "[TX Pw] All rate = %d dBm\n", val[0]);
			for (i = 0; i < TPU_SIZE_PWR_TAB_DBW_CCK; i++){
				for (j = 0; j < TPU_SIZE_PWR_TAB_CCK; j++)
					//*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_cck + i) + j) = (s8)val[0];
					tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_cck[i][j] = ((s8)val[0] << 1) & 0x7f;
			}
			for (i = 0; i < TPU_SIZE_PWR_TAB_DBW; i++){
				for (j = 0; j < TPU_SIZE_PWR_TAB_OFDM; j++)
					//*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_ofdm + i) + j) = (s8)val[0];
					tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_ofdm[i][j] = ((s8)val[0] << 1) & 0x7f;
				for (j = 0; j < TPU_SIZE_PWR_TAB_EHT_BE; j++)
					//*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_eht + i) + j) = (s8)val[0];
					tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_eht[i][j] = ((s8)val[0] << 1) & 0x7f;
				for (j = 0; j < HALBB_MAX_PATH; j++) {
					for (k = 0; k < TPU_SIZE_PWR_TAB_BE; k++){
						//*(*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_mcs + i) + j) + k) = (s8)val[0];
						tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_mcs[i][j][k]= ((s8)val[0] << 1) & 0x7f;
						//*(*(*(tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_dlru + i) + j) + k) = (s8)val[0];
						tpu->rtw_tpu_pwr_by_rate_be_i.pwr_by_rate_dlru[i][j][k] = ((s8)val[0] << 1) & 0x7f;
					}
				}
			}
			
		} else {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "Set Err\n");
			return;
		}
		halbb_bb_wrap_set_pow_by_rate_all(bb, (enum phl_band_idx)bb->bb_phy_idx);
	
	} else if (_os_strcmp(input[1], "cck_dup") == 0) { 
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[1]);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"[TX Pw] cck_dup_patha_l_pathb_h_2tx = %d dBm\n", val[0]);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"[TX Pw] cck_dup_patha_h_pathb_l_2tx = %d dBm\n", val[1]);
		tpu->pwr_cck_dup_patha_l_pathb_h_2tx = ((s8)val[0] << 1) & 0x7f; 
		tpu->pwr_cck_dup_patha_h_pathb_l_2tx = ((s8)val[1] << 1) & 0x7f;
		halbb_bb_wrap_be_set_cck_dup_path(bb, (enum phl_band_idx)bb->bb_phy_idx);
	} else if (_os_strcmp(input[1], "cusofst") == 0) { 
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);
		if (_os_strcmp(input[2], "sw") == 0){
			tpu->pwr_cusofst_sw = (u8)val[0];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"[Cusofst] sw = %d dBm\n", val[0]);
		}
		else if (_os_strcmp(input[2], "bylim") == 0){
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"[Cusofst] bylim = %d dBm\n", val[0]);
			tpu->pwr_cusofst_bylim = ((s16)val[0] << 2) & 0x1ff;
		}
		else if (_os_strcmp(input[2], "bylimbf") == 0){
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"[Cusofst] bylimbf = %d dBm\n", val[0]);
			tpu->pwr_cusofst_bylimbf = ((s16)val[0] << 2) & 0x1ff;
		}
		else if (_os_strcmp(input[2], "byrate") == 0){
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"[Cusofst] byrate = %d dBm\n", val[0]);
			tpu->pwr_cusofst_byrate = ((s16)val[0] << 2) & 0x1ff;
		}
		else if (_os_strcmp(input[2], "byrulim") == 0){
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				"[Cusofst] byrulim = %d dBm\n", val[0]);
			tpu->pwr_cusofst_byrulim = ((s16)val[0] << 2) & 0x1ff;
		} else {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "Set Err\n");
			return;
		}
		halbb_bb_wrap_be_set_pwr_cusofst_all(bb, (enum phl_band_idx)bb->bb_phy_idx);
	
	} else if (_os_strcmp(input[1], "lmt") == 0) {
		if (_os_strcmp(input[2], "en") == 0) {
			HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);

			tpu->pwr_lmt_en = (bool)val[0];

			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "pwr_lmt_en = %d\n", tpu->pwr_lmt_en);
			
			halbb_bb_wrap_set_pwr_limit_en(bb, bb->bb_phy_idx);
		} else if (_os_strcmp(input[2], "all") == 0) {
			HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);

			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "Set all Pwr Lmt = %d dBm\n", val[0]);
			size_tmp = sizeof(struct bb_tpu_pwr_lmt_info_be) / sizeof(s8);
			tmp_s8 = &lmt->pwr_lmt_cck[0][0][0];

			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "pwr_lmt_size = %d\n", size_tmp);

			for (i = 0; i < size_tmp; i++) {
				*tmp_s8 = ((s8)val[0] << 1) & 0x7f;
				tmp_s8++;
			}
			halbb_bb_wrap_set_pwr_limit_all(bb, (enum phl_band_idx)bb->bb_phy_idx);
		} else if (_os_strcmp(input[2], "ru_all") == 0) {
			HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);

			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "Set all RUA Pwr Lmt = %d dBm\n", val[0]);

			size_tmp = sizeof(struct bb_tpu_pwr_lmt_ru_info_be) / sizeof(s8);
			tmp_s8 = &lmt_ru->pwr_lmt_ru_be[0][0][0];

			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				 "pwr_lmt_size_ru = %d\n", size_tmp);

			for (i = 0; i < size_tmp; i++) {
				*tmp_s8 = ((s8)val[0] << 1) & 0x7f;
				tmp_s8++;
			}
			halbb_bb_wrap_set_pwr_limit_rua_all(bb, (enum phl_band_idx)bb->bb_phy_idx);
		} else {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "Set Err\n");
			return;
		}
	#if 0
	} else if (_os_strcmp(input[1], "tb_ofst") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);

		rpt_tmp = halbb_set_pwr_ul_tb_ofst(bb, (s16)val[0], bb->bb_phy_idx);
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[ULTB Ofst]Set succcess=%d,  en = %d, pw_ofst=%d\n",
			    rpt_tmp, val[0], (s16)val[1]);
	} else if (_os_strcmp(input[1], "tx_shap") == 0) {
		HALBB_SCAN(input[2], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[1]);
		HALBB_SCAN(input[4], DCMD_DECIMAL, &val[2]);

		tpu->tx_ptrn_shap_idx = (u8)val[1];
		halbb_set_tx_pow_pattern_shap(bb, (u8)val[0], (bool)val[2], bb->bb_phy_idx);

		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "[Tx Shap] ch=%d, shap_idx=%d\n", val[0], tpu->tx_ptrn_shap_idx);
	}
	#endif

#endif
	} else {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			    "Set Err\n");
	}
}

#endif

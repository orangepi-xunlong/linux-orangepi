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
#include "halbb_precomp.h"
#ifdef BB_8922A_SUPPORT
	#include "halbb_8922a_2/halbb_hwimg_raw_data_8922a_2.h"
#endif

bool halbb_sel_headline(struct bb_info *bb, u32 *array, u32 array_len,
			u8 *headline_size, u8 *headline_idx)
{
	bool case_match = false;
	u32 cut_drv = (u32)bb->hal_com->cv;
	u32 rfe_drv = (u32)bb->phl_com->dev_cap.rfe_type;
	u32 cut_para = 0, rfe_para = 0;
	u32 compare_target = 0;
	u32 cut_max = 0;
	u32 i = 0;

	*headline_idx = 0;
	*headline_size = 0;

	if (bb->bb_dbg_i.cr_dbg_mode_en) {
		rfe_drv = bb->bb_dbg_i.rfe_type_curr_dbg;
		cut_drv = bb->bb_dbg_i.cut_curr_dbg;
	}

	BB_DBG(bb, DBG_INIT, "[%s] {RFE, Cart}={%d, %d}, dbg_en=%d\n",
	       __func__, rfe_drv, cut_drv, bb->bb_dbg_i.cr_dbg_mode_en);

	while ((i + 1) < array_len) {
		if ((array[i] >> 28) != 0xf) {
			*headline_size = (u8)i;
			break;
		}
		BB_DBG(bb, DBG_INIT, "array[%02d]=0x%08x, array[%02d]=0x%08x\n",
		       i, array[i], i+1, array[i+1]);
		i += 2;
	}

	BB_DBG(bb, DBG_INIT, "headline_size=%d\n", i);

	if (i == 0)
		return true;

	/*case_idx:1 {RFE:Match, CUT:Match}*/
	compare_target = ((rfe_drv & 0xff) << 16) | (cut_drv & 0xff);
	BB_DBG(bb, DBG_INIT, "[1] CHK {RFE:Match, CUT:Match}\n");
	for (i = 0; i < *headline_size; i += 2) {
		if ((array[i] & 0x0fffffff) == compare_target) {
			*headline_idx = (u8)(i >> 1);
			return true;
		}
	}
	BB_DBG(bb, DBG_INIT, "\t fail\n");

#if 0
	/*case_idx:2 {RFE:Match, CUT:Dont care}*/
	compare_target = ((rfe_drv & 0xff) << 16) | (BB_DONT_CARE & 0xff);
	BB_DBG(bb, DBG_INIT, "[2] CHK {RFE:Match, CUT:Dont_Care}\n");
	for (i = 0; i < *headline_size; i += 2) {
		if ((array[i] & 0x0fffffff) == compare_target) {
			*headline_idx = (u8)(i >> 1);
			return true;
		}
	}
	BB_DBG(bb, DBG_INIT, "\t fail\n");
#endif

	/*case_idx:3 {RFE:Match, CUT:Max_in_table}*/
	BB_DBG(bb, DBG_INIT, "[3] CHK {RFE:Match, CUT:Max_in_Table}\n");
	for (i = 0; i < *headline_size; i += 2) {
		rfe_para = (array[i] & 0x00ff0000) >> 16; 
		cut_para = array[i] & 0x0ff;
		if (rfe_para == rfe_drv) {
			if (cut_para >= cut_max) {
				cut_max = cut_para;
				*headline_idx = (u8)(i >> 1);
				BB_DBG(bb, DBG_INIT, "cut_max:%d\n", cut_max);
				case_match = true;
			}
		}
	}
	if (case_match) {
		return true;
	}
	BB_DBG(bb, DBG_INIT, "\t fail\n");

#if 0
	/*case_idx:4 {RFE:Dont Care, CUT:Max_in_table}*/
	BB_DBG(bb, DBG_INIT, "[4] CHK {RFE:Dont_Care, CUT:Max_in_Table}\n");
	for (i = 0; i < *headline_size; i += 2) {
		rfe_para = (array[i] & 0x00ff0000) >> 16; 
		cut_para = array[i] & 0x0ff;
		if (rfe_para == BB_DONT_CARE) {
			if (cut_para >= cut_max &&
			    cut_para <= cut_drv) {
				cut_max = cut_para;
				*headline_idx = (u8)(i >> 1);
				BB_DBG(bb, DBG_INIT, "cut_max:%d\n", cut_max);
				case_match = true;
				
			}
		}
	}
	if (case_match) {
		return true;
	}
	BB_DBG(bb, DBG_INIT, "\t fail\n");
#endif

#if 0
	/*case_idx:5 {RFE:Not_Match, CUT:Not_Match}*/
	BB_DBG(bb, DBG_INIT, "[5] CHK {RFE:Not_Match, CUT:Not_Match}\n");
	BB_DBG(bb, DBG_INIT, "\t all fail\n");
#endif
	BB_WARNING("[%s] Parameter package init Fail {RFE, Cart}={%d, %d}\n",
		   __func__, rfe_drv, cut_drv);

	return false;
}

void halbb_cfg_bb_rpl_ofst(struct bb_info *bb, enum bb_band_t band, u8 path, u32 addr, u32 data)
{
	struct bb_gain_info *gain = &bb->bb_gain_i;
	u8 i = 0;
	u8 bw = (u8)(addr & 0xf0) >> 4;
	u8 rxsc_start = (u8)(addr & 0xf);
	u8 rxsc = 0;
	s8 ofst = 0;

	if (bw == (u8)CHANNEL_WIDTH_20) {
		gain->rpl_ofst_20[band][path] = (s8)data;
		BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
		       band, path, (20 << bw), rxsc, gain->rpl_ofst_20[band][path]);
	} else if (bw == (u8)CHANNEL_WIDTH_40){
		if (rxsc_start == BB_RXSC_START_IDX_FULL) {
			gain->rpl_ofst_40[band][path][0] = (s8)data;
			BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
			       band, path, (20 << bw), rxsc,
			       gain->rpl_ofst_40[band][path][0]);
		} else if (rxsc_start == BB_RXSC_START_IDX_20) {
			for (i = 0; i < 2; i++) {
				rxsc = BB_RXSC_START_IDX_20 + i;
				ofst = (s8)((data >> (8 * i)) & 0xff);
				gain->rpl_ofst_40[band][path][rxsc] = ofst;
				BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
				       band, path, (20 << bw), rxsc,
				       gain->rpl_ofst_40[band][path][rxsc]);
			}
		}

	} else if (bw == (u8)CHANNEL_WIDTH_80){
		if (rxsc_start == BB_RXSC_START_IDX_FULL) {
			gain->rpl_ofst_80[band][path][0] = (s8)data;
			BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
			       band, path, (20 << bw), rxsc,
			       gain->rpl_ofst_80[band][path][0]);
		} else if (rxsc_start == BB_RXSC_START_IDX_20) {
			for (i = 0; i < 4; i++) {
				rxsc = BB_RXSC_START_IDX_20 + i;
				ofst = (s8)((data >> (8 * i)) & 0xff);
				gain->rpl_ofst_80[band][path][rxsc] = ofst;
				BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
				       band, path, (20 << bw), rxsc,
				       gain->rpl_ofst_80[band][path][rxsc]);
			}
		} else if (rxsc_start == BB_RXSC_START_IDX_40) {
			for (i = 0; i < 2; i++) {
				rxsc = BB_RXSC_START_IDX_40 + i;
				ofst = (s8)((data >> (8 * i)) & 0xff);
				gain->rpl_ofst_80[band][path][rxsc] = ofst;
				BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
				       band, path, (20 << bw), rxsc,
				       gain->rpl_ofst_80[band][path][rxsc]);
			}
		}
	} else if (bw == (u8)CHANNEL_WIDTH_160) {
		if (rxsc_start == BB_RXSC_START_IDX_FULL) {
			gain->rpl_ofst_160[band][path][0] = (s8)data;
			BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
			       band, path, (20 << bw), rxsc,
			       gain->rpl_ofst_160[band][path][0]);
		} else if (rxsc_start == BB_RXSC_START_IDX_20) {
			for (i = 0; i < 4; i++) {
				rxsc = BB_RXSC_START_IDX_20 + i;
				ofst = (s8)((data >> (8 * i)) & 0xff);
				gain->rpl_ofst_160[band][path][rxsc] = ofst;
				BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
				       band, path, (20 << bw), rxsc,
				       gain->rpl_ofst_160[band][path][rxsc]);
			}
		} else if (rxsc_start == BB_RXSC_START_IDX_20_1) {
			for (i = 0; i < 4; i++) {
				rxsc = BB_RXSC_START_IDX_20_1 + i;
				ofst = (s8)((data >> (8 * i)) & 0xff);
				gain->rpl_ofst_160[band][path][rxsc] = ofst;
				BB_DBG(bb, DBG_INIT, "-------------------------------------------------------------------\n");
				BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
				       band, path, (20 << bw), rxsc,
				       gain->rpl_ofst_160[band][path][rxsc]);
				BB_DBG(bb, DBG_INIT, "Data=0x%x\n", data);
				BB_DBG(bb, DBG_INIT, "-------------------------------------------------------------------\n");
			}
		} else if (rxsc_start == BB_RXSC_START_IDX_40) {
			for (i = 0; i < 4; i++) {
				rxsc = BB_RXSC_START_IDX_40 + i;
				ofst = (s8)((data >> (8 * i)) & 0xff);
				gain->rpl_ofst_160[band][path][rxsc] = ofst;
				BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
				       band, path, (20 << bw), rxsc,
				       gain->rpl_ofst_160[band][path][rxsc]);
			}
		} else if (rxsc_start == BB_RXSC_START_IDX_80) {
			for (i = 0; i < 2; i++) {
				rxsc = BB_RXSC_START_IDX_80 + i;
				ofst = (s8)((data >> (8 * i)) & 0xff);
				gain->rpl_ofst_160[band][path][rxsc] = ofst;
				BB_DBG(bb, DBG_INIT, "RPL[Band:%d][path=%d][%dM][rxsc=%d]=%d\n",
				       band, path, (20 << bw), rxsc,
				       gain->rpl_ofst_160[band][path][rxsc]);
			}
		}
	}
}

void halbb_flag_2_default(bool *is_matched, bool *find_target)
{
	*is_matched = true;
	*find_target = false;
}

#ifdef HALBB_COMPILE_BE_SERIES

void halbb_set_lna_tia_gain_bbcr_gt2(struct bb_info *bb, u8 fc_ch, enum band_type band, enum channel_width bw, enum rf_path path)
{
	struct bb_hw_cfg_info *hw_cfg = &bb->bb_hw_cfg_i;
	struct bb_hw_cfg_cr_info *cr = &bb->bb_hw_cfg_i.bb_hw_cfg_cr_i;
	struct bb_gain_gen2_info *gain = &bb->bb_gain_gen2_i;
	enum bb_band_gt2_t band_gt2;
	enum bb_tab_idx_gt2_t tab_idx = BB_GT2_TAB_2G;
	enum bb_bw_gt2_t bw_gt2 = 0;
	u8 lna_idx = 0, tia_idx = 0;
	s8 val = 0;
	u8 i = 0;

	if (path >= RF_PATH_C) {
		BB_WARNING("Rx Gain offset only supports 2 paths temporarily!!\n");
		return;
	}

	bw_gt2 = (bw <= CHANNEL_WIDTH_40) ? BB_GT2_BW_20_40 : BB_GT2_BW_80_160_320;

	if (band == BAND_ON_24G) {
		band_gt2 = BB_GT2_BAND_2G;
		tab_idx = BB_GT2_TAB_2G;

		if (!hw_cfg->gain_table_init_ready_2g_a && (path == RF_PATH_A))
			hw_cfg->gain_table_init_ready_2g_a = true;
		else if (!hw_cfg->gain_table_init_ready_2g_b && (path == RF_PATH_B))
			hw_cfg->gain_table_init_ready_2g_b = true;

		/*only init 2G table for 1 time*/
		if (hw_cfg->gain_table_init_ready_2g_a && hw_cfg->gain_table_init_ready_2g_b)
			return;

	} else {
		band_gt2 = halbb_get_band_gen2(bb, fc_ch, band);
		tab_idx = BB_GT2_TAB_5G_6G;

		hw_cfg->curr_5g_6g_cfg_bw_gt2 = bw_gt2;
		hw_cfg->curr_5g_6g_cfg_band_gt2 = band_gt2;
	}

	BB_DBG(bb, DBG_PHY_CONFIG, "[%s] fc_ch=%d, band=%d, bw_tab_idx=%d, path=%d\n", 
		__func__, fc_ch, band_gt2, bw_gt2, path);

	/*LNA*/
	for (lna_idx = 0; lna_idx < BB_GT2_LNA_NUM; lna_idx++) {
		val = gain->lna_gain[band_gt2][bw_gt2][path][lna_idx];
		
		halbb_set_reg_cmn(bb, cr->lna_gain_cr[tab_idx][path][lna_idx], cr->lna_gain_cr_m[tab_idx][lna_idx], val, bb->bb_phy_idx);
	}
	/*TIA*/
	for (tia_idx = 0; tia_idx < BB_GT2_TIA_NUM; tia_idx++) {
		val = gain->tia_gain[band_gt2][bw_gt2][path][tia_idx];
		halbb_set_reg_cmn(bb, cr->tia_gain_cr[tab_idx][path][tia_idx], cr->tia_gain_cr_m[tab_idx][tia_idx], val, bb->bb_phy_idx);
	}
#if 0
	/*LNA op1db*/
	for (i = 0; i < BB_GT2_LNA_NUM; i++) {
		val = gain->lna_op1db[band_gt2][bw_gt2][path][i];
		halbb_set_reg_cmn(bb, cr->lna_op1db_cr[tab_idx][path][i], cr->lna_op1db_cr_m[tab_idx][i], val, bb->bb_phy_idx);
	}
	/*LNA op1db*/
	for (i = 0; i < BB_GT2_TIA_LNA_OP1DB_NUM; i++) {
		val = gain->tia_lna_op1db[band_gt2][bw_gt2][path][i];
		halbb_set_reg_cmn(bb, cr->tia_lna_op1db_cr[tab_idx][path][i], cr->tia_lna_op1db_cr_m[tab_idx][i], val, bb->bb_phy_idx);
	}
#endif
}

void halbb_fill_in_gain_table_gt2(struct bb_info *bb, u32 addr, u32 data)
{
	struct bb_gain_info *gain = &bb->bb_gain_i;
	struct bb_gain_gen2_info *gain_2 = &bb->bb_gain_gen2_i;
	enum bb_func_type_gt2_t func_type_gt2 = (enum bb_func_type_gt2_t)((addr & 0xff000000) >> 24);
	enum bb_band_gt2_t band_gt2 = (enum bb_band_gt2_t)((addr & 0xff0000) >> 16);
	enum bb_bw_gt2_t bw_gt2 = (enum bb_bw_gt2_t)((addr & 0xf000) >> 12);
	u8 path = (u8)((addr & 0xf00) >> 8);
	u8 type = (u8)(addr & 0xff);
	u8 i = 0;

	if (band_gt2 >= BB_GT2_BAND_NUM) {
		BB_WARNING("[%s] band_gt2=%d\n", __func__, band_gt2);
		return;
	}

	if (bw_gt2 >= BB_GT2_BW_NUM) {
		BB_WARNING("[%s] bw_gt2=%d\n", __func__, bw_gt2);
		return;
	}

	if (path >= BB_GT2_PATH_NUM) {
		BB_WARNING("[%s] path_gt2=%d\n", __func__, path);
		return;
	}

	if (func_type_gt2 == BB_GT2_FUNC_GAIN_ERROR) {

		if (type == 0) {
			for (i = 0; i < 4; i++)
				gain_2->lna_gain[band_gt2][bw_gt2][path][i] = (data >> (8 * i)) & 0xff;
		} else if (type == 1) {
			for (i = 0; i < 3; i++)
				gain_2->lna_gain[band_gt2][bw_gt2][path][4 + i] = (data >> (8 * i)) & 0xff;
		} else if (type == 2) {
			for (i = 0; i < 2; i++)
				gain_2->tia_gain[band_gt2][bw_gt2][path][i] = (data >> (8 * i)) & 0xff;
		}
	} 
#if 0
	else if (BB_GT2_FUNC_RPL == 1) {
		halbb_cfg_bb_rpl_ofst(bb, band_gt2, path, addr, data);
	} else if (func_type_gt2 == BB_GT2_FUNC_BYPASS) {

		if (type == 0) {
			for (i = 0; i < 4; i++)
				gain->lna_gain_bypass[band_gt2][path][i] = (data >> (8 * i)) & 0xff;
		} else if (type == 1) {
			for (i = 0; i < 3; i++)
				gain->lna_gain_bypass[band_gt2][path][4 + i] = (data >> (8 * i)) & 0xff;
		}

	}
#endif

	else if (func_type_gt2 == BB_GT2_FUNC_OP1DB) {

		if (type == 0) {
			for (i = 0; i < 4; i++)
				gain_2->lna_op1db[band_gt2][bw_gt2][path][i] = (data >> (8 * i)) & 0xff;
		} else if (type == 1) {
			for (i = 0; i < 3; i++)
				gain_2->lna_op1db[band_gt2][bw_gt2][path][4 + i] = (data >> (8 * i)) & 0xff;
		} else if (type == 2) {
			for (i = 0; i < 4; i++)
				gain_2->tia_lna_op1db[band_gt2][bw_gt2][path][i] = (data >> (8 * i)) & 0xff;
		} else if (type == 3) {
			for (i = 0; i < 4; i++)
				gain_2->tia_lna_op1db[band_gt2][bw_gt2][path][4 + i] = (data >> (8 * i)) & 0xff;
		}
	}
#if 0
	else if (func_type_gt2 == BB_GT2_FUNC_WBADC) {

		if (type == 0)
			gain->wb_gidx_elna[band_gt2][path] = data;
		else if (type == 1)
			for (i = 0; i < 8; i++)
				gain->wb_gidx_lna_tia[band_gt2][path][i] = (data >> (4 * i)) & 0x7;
		else if (type == 2)
			for (i = 0; i < 8; i++)
				gain->wb_gidx_lna_tia[band_gt2][path][i + 8] = (data >> (4 * i)) & 0x7;
		else if (type == 3)
			gain->gs_idx[band_gt2][path][0] = data;
		else if (type == 4)
			gain->gs_idx[band_gt2][path][1] = data;
		else if (type == 5)
			for (i = 0; i < 2; i++)
				gain->g_elna[band_gt2][path][i] = (data >> (8 * i)) & 0xff;
	} else {
		BB_WARNING("[%s] cfg_type=%d\n", __func__, func_type_gt2);
	}
#endif

}

bool halbb_cfg_bb_gain_gt2(struct bb_info *bb, bool is_form_folder,
				  u32 folder_len, u32 *folder_array)
{
	bool is_matched, find_target;
	u32 cfg_target = 0, cfg_para = 0;
	u32 i = 0;
	u32 array_len = 0;
	u32 *array = NULL;
	u32 v1 = 0, v2 = 0;
	u8 h_size = 0;
	u8 h_idx = 0;

	BB_DBG(bb, DBG_INIT, "===> %s\n", __func__);

	if (bb->bb_80211spec != BB_BE_IC) {
		BB_WARNING("[%s]\n", __func__);
		return false;
	}

	if (is_form_folder) {
		array_len = folder_len;
		array = folder_array;
#ifdef BB_8922A_SUPPORT
	} else if (bb->ic_type == BB_RTL8922A) {
		array_len = sizeof(array_mp_8922a_2_phy_reg_gain) / sizeof(u32);
		array = (u32 *)array_mp_8922a_2_phy_reg_gain;
#endif
	} else {
		BB_WARNING("[%s] Not Support IC\n", __func__);
		return false;
	}

	BB_DBG(bb, DBG_INIT, "GAIN_TABLE_form_folder=%d, len=%d\n",
	       is_form_folder, array_len);

	if (!halbb_sel_headline(bb, array, array_len, &h_size, &h_idx)) {
		BB_WARNING("[%s]Invalid BB CR Pkg\n", __func__);
		return false;
	}
	BB_DBG(bb, DBG_INIT, "h_size = %d, h_idx = %d\n", h_size, h_idx);

	if (h_size != 0) {
		cfg_target = array[h_idx << 1] & 0x0fffffff;
	}

	i += h_size;

	BB_DBG(bb, DBG_INIT, "cfg_target = 0x%x\n", cfg_target);
	BB_DBG(bb, DBG_INIT, "array[i] = 0x%x, array[i+1] = 0x%x\n", array[i], array[i + 1]);

	halbb_flag_2_default(&is_matched, &find_target);
	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];
		i += 2;

		switch (v1 >> 28) {
		case BB_IF:
		case BB_ELSE_IF:
			cfg_para = v1 & 0x0fffffff;
			BB_DBG(bb, DBG_INIT, "*if (rfe=%d, cart=%d)\n",
			       (cfg_para & 0xff0000) >> 16, cfg_para & 0xff);
			break;
		case BB_ELSE:
			BB_DBG(bb, DBG_INIT, "*else\n");
			is_matched = false;
			if (!find_target) {
				BB_WARNING("Init BBCR Fail in Reg 0x%x\n", array[i]);
				return false;
			}
			break;
		case BB_END:
			BB_DBG(bb, DBG_INIT, "*endif\n");
			halbb_flag_2_default(&is_matched, &find_target);
			break;
		case BB_CHK:
			/*Check this para meets driver's requirement or not*/
			if (find_target) {
				BB_DBG(bb, DBG_INIT, "\t skip\n");
				is_matched = false;
				break;
			}

			if (cfg_para == cfg_target) {
				is_matched = true;
				find_target = true;
			} else {
				is_matched = false;
				find_target = false;
			}
			BB_DBG(bb, DBG_INIT, "\t match=%d\n", is_matched);
			break;
		default:
			if (is_matched)
				halbb_fill_in_gain_table_gt2(bb, v1, v2);
			break;
		}
	}

	BB_DBG(bb, DBG_INIT, "BBCR gain Init Success\n\n");
	return true;
}

void halbb_cfg_bb_phy(struct bb_info *bb, u32 addr, u32 data,
			    enum phl_phy_idx phy_idx)
{
#ifdef HALBB_DBCC_SUPPORT
	u32 ofst = 0;
#endif

	if (addr == 0xfe) {
		halbb_delay_ms(bb, 50);
		BB_DBG(bb, DBG_INIT, "Delay 50 ms\n");
	} else if (addr == 0xfd) {
		halbb_delay_ms(bb, 5);
		BB_DBG(bb, DBG_INIT, "Delay 5 ms\n");
	} else if (addr == 0xfc) {
		halbb_delay_ms(bb, 1);
		BB_DBG(bb, DBG_INIT, "Delay 1 ms\n");
	} else if (addr == 0xfb) {
		halbb_delay_us(bb, 50);
		BB_DBG(bb, DBG_INIT, "Delay 50 us\n");
	} else if (addr == 0xfa) {
		halbb_delay_us(bb, 5);
		BB_DBG(bb, DBG_INIT, "Delay 5 us\n");
	} else if (addr == 0xf9) {
		halbb_delay_us(bb, 1);
		BB_DBG(bb, DBG_INIT, "Delay 1 us\n");
	} else {
		#if 0
		#ifdef HALBB_DBCC_SUPPORT
		if ((bb->hal_com->dbcc_en || bb->bb_dbg_i.cr_dbg_mode_en) &&
		    phy_idx == HW_PHY_1) {
			ofst = halbb_phy0_to_phy1_ofst(bb, addr, phy_idx);
			if (ofst == 0)
				return;
			addr += ofst;
		} else {
			phy_idx = HW_PHY_0;
		}
		if (phy_idx == HW_PHY_1)
			BB_DBG(bb, DBG_DBCC, "[REG][%d]0x%04X = 0x%08X\n", phy_idx, addr, data);
		#else
		BB_DBG(bb, DBG_INIT, "[REG]0x%04X = 0x%08X\n", addr, data);
		#endif

		halbb_set_reg(bb, addr, MASKDWORD, data);
		#else
		halbb_set_reg_cmn(bb, addr, MASKDWORD, data, phy_idx);
		#endif
	}
}

bool halbb_cfg_bbcr_be(struct bb_info *bb, bool is_form_folder,
			  u32 folder_len, u32 *folder_array,
			  enum phl_phy_idx phy_idx)
{
	bool is_matched, find_target;
	u32 cfg_target = 0, cfg_para = 0;
	u32 i = 0;
	u32 array_len = 0;
	u32 *array = NULL;
	u32 v1 = 0, v2 = 0;
	u8 h_size = 0;
	u8 h_idx = 0;
	bool ret = false;

	BB_DBG(bb, DBG_INIT, "===> %s\n", __func__);

	if (is_form_folder) {
		array_len = folder_len;
		array = folder_array;
#ifdef BB_8922A_SUPPORT
	} else if (bb->ic_type == BB_RTL8922A) {
		array_len = sizeof(array_mp_8922a_2_phy_reg) / sizeof(u32);
		array = (u32 *)array_mp_8922a_2_phy_reg;
#endif
	} else {
		BB_WARNING("[%s] Not Support IC\n", __func__);
		return false;
	}

	BB_DBG(bb, DBG_INIT, "form_folder=%d, len=%d, dbcc_en=%d, phy_idx=%d\n",
	       is_form_folder, array_len, bb->hal_com->dbcc_en, phy_idx);

	if (!halbb_sel_headline(bb, array, array_len, &h_size, &h_idx)) {
		BB_WARNING("[%s]Invalid BB CR Pkg\n", __func__);
		return false;
	}
	BB_DBG(bb, DBG_INIT, "h_size = %d, h_idx = %d\n", h_size, h_idx);

	if (h_size != 0) {
		cfg_target = array[h_idx << 1] & 0x0fffffff;
	}

	i += h_size;

	BB_DBG(bb, DBG_INIT, "cfg_target = 0x%x\n", cfg_target);
	BB_DBG(bb, DBG_INIT, "array[i] = 0x%x, array[i+1] = 0x%x\n", array[i], array[i + 1]);

	halbb_flag_2_default(&is_matched, &find_target);
	#ifdef HALBB_FW_OFLD_SUPPORT
	if (halbb_check_fw_ofld(bb))
		BB_WARNING("Becareful it is fwofld mode in BB init !!");
	#endif
	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];
		i += 2;

		switch (v1 >> 28) {
		case BB_IF:
		case BB_ELSE_IF:
			cfg_para = v1 & 0x0fffffff;
			BB_DBG(bb, DBG_INIT, "*if (rfe=%d, cart=%d)\n",
			       (cfg_para & 0xff0000) >> 16, cfg_para & 0xff);
			break;
		case BB_ELSE:
			BB_DBG(bb, DBG_INIT, "*else\n");
			is_matched = false;
			if (!find_target) {
				BB_WARNING("Init BBCR Fail in Reg 0x%x\n", array[i]);
				return false;
			}
			break;
		case BB_END:
			BB_DBG(bb, DBG_INIT, "*endif\n");
			halbb_flag_2_default(&is_matched, &find_target);
			break;
		case BB_CHK:
			/*Check this para meets driver's requirement or not*/
			if (find_target) {
				BB_DBG(bb, DBG_INIT, "\t skip\n");
				is_matched = false;
				break;
			}

			if (cfg_para == cfg_target) {
				is_matched = true;
				find_target = true;
			} else {
				is_matched = false;
				find_target = false;
			}
			BB_DBG(bb, DBG_INIT, "\t match=%d\n", is_matched);
			break;
		default:
			if (is_matched) {
				//#ifdef HALBB_FW_OFLD_SUPPORT
				//ret = halbb_fwcfg_bb_phy_8922a(bb, v1, v2, phy_idx);
				//#else
				halbb_cfg_bb_phy(bb, v1, v2, phy_idx);
				//#endif
			}
			break;
		}
	}
	BB_DBG(bb, DBG_INIT, "BBCR Init Success\n\n");
	#ifdef HALBB_FW_OFLD_SUPPORT
	return ret;
	#else
	return true;
	#endif
}

#endif

bool halbb_init_cr_default(struct bb_info *bb, bool is_form_folder, u32 folder_len,
		    u32 *folder_array, enum phl_phy_idx phy_idx)
{
	bool result = true;

	if (!bb->bb_cmn_info_init_ready) {
		BB_WARNING("bb_cmn_info_init_ready = false");
		return false;
	}

	if (is_form_folder) {
		if (!folder_array) {
			BB_WARNING("[%s] folder_array=NULL\n", __func__);
			return false;
		}

		if (folder_len == 0) {
			BB_WARNING("[%s] folder_len=0\n", __func__);
			return false;
		}
	}

#ifdef HALBB_DBCC_SUPPORT
	if (phy_idx == HW_PHY_1 && !bb->hal_com->dbcc_en) {
		BB_WARNING("[%s]\n",__func__);
		if (!bb->bb_dbg_i.cr_dbg_mode_en)
			return false;
	}
#endif

	BB_DBG(bb, DBG_INIT, "[%s] ic=%d\n", __func__, bb->hal_com->chip_id);

	switch (bb->ic_type) {

	#ifdef BB_8852A_2_SUPPORT
	case BB_RTL8852A:
		result = halbb_cfg_bbcr_ax_8852a_2(bb, is_form_folder, folder_len,
						   folder_array, phy_idx);
		halbb_tpu_mac_cr_init(bb, phy_idx);
		break;
	#endif

	#ifdef BB_8852B_SUPPORT
	case BB_RTL8852B:
		result = halbb_cfg_bbcr_ax_8852b(bb, is_form_folder, folder_len,
						   folder_array, phy_idx);
		halbb_tpu_mac_cr_init(bb, phy_idx);
		break;
	#endif

	#ifdef BB_8852C_SUPPORT
	case BB_RTL8852C:
		result = halbb_cfg_bbcr_ax_8852c(bb, is_form_folder, folder_len,
						   folder_array, phy_idx);
		halbb_tpu_mac_cr_init(bb, phy_idx);
		halbb_tssi_ctrl_mac_cr_init(bb, phy_idx);
		break;
	#endif

	#ifdef BB_8192XB_SUPPORT
	case BB_RTL8192XB:
		result = halbb_cfg_bbcr_ax_8192xb(bb, is_form_folder, folder_len,
						   folder_array, phy_idx);
		halbb_tpu_mac_cr_init(bb, phy_idx);
		halbb_tssi_ctrl_mac_cr_init(bb, phy_idx);
		break;
	#endif

	#ifdef BB_8851B_SUPPORT
	case BB_RTL8851B:
		result = halbb_cfg_bbcr_ax_8851b(bb, is_form_folder, folder_len,
						 folder_array, phy_idx);
		halbb_tpu_mac_cr_init(bb, phy_idx);
		break;
	#endif

	#ifdef BB_1115_SUPPORT
	case BB_RLE1115:
		result = halbb_cfg_bbcr_ax_1115(bb, is_form_folder, folder_len,
						 folder_array, phy_idx);
		break;
	#endif

	#if 0//def BB_8922A_SUPPORT
	case BB_RTL8922A:
		result = halbb_cfg_bbcr_be(bb, is_form_folder, folder_len,
						 folder_array, phy_idx);
		break;
	#endif

	default:
	#ifdef HALBB_COMPILE_BE_SERIES
		if (bb->bb_80211spec == BB_BE_IC)
			result = halbb_cfg_bbcr_be(bb, is_form_folder, folder_len,
						   folder_array, phy_idx);
		else
	#endif
			BB_WARNING("[%s] ic=%d\n", __func__, bb->hal_com->chip_id);

		break;
	}

	BB_DBG(bb, DBG_INIT, "BB_CR_init_success = %d\n", result);
	return result;
}

bool halbb_init_gain_table(struct bb_info *bb, bool is_form_folder, u32 folder_len,
			   u32 *folder_array, enum phl_phy_idx phy_idx)
{
	bool result = true;

	if (!bb->bb_cmn_info_init_ready) {
		BB_WARNING("bb_cmn_info_init_ready = false");
		return false;
	}

	if (is_form_folder) {
		if (!folder_array) {
			BB_WARNING("[%s] folder_array=NULL\n", __func__);
			return false;
		}

		if (folder_len == 0) {
			BB_WARNING("[%s] folder_len=0\n", __func__);
			return false;
		}
	}

#ifdef HALBB_DBCC_SUPPORT
	if (phy_idx == HW_PHY_1 && !bb->hal_com->dbcc_en) {
		BB_WARNING("[%s]\n",__func__);
		if (!bb->bb_dbg_i.cr_dbg_mode_en)
			return false;
	}
#endif

	BB_DBG(bb, DBG_INIT, "[%s] ic=%d\n", __func__, bb->hal_com->chip_id);

	switch (bb->ic_type) {

	#ifdef BB_8852A_2_SUPPORT
	case BB_RTL8852A:
		result &= halbb_cfg_bb_gain_ax_8852a_2(bb, is_form_folder,
						       folder_len, folder_array);
		break;
	#endif

	#ifdef BB_8852B_SUPPORT
	case BB_RTL8852B:
		result &= halbb_cfg_bb_gain_ax_8852b(bb, is_form_folder,
						     folder_len, folder_array);
		break;
	#endif

	#ifdef BB_8852C_SUPPORT
	case BB_RTL8852C:
		result &= halbb_cfg_bb_gain_ax_8852c(bb, is_form_folder,
						       folder_len, folder_array);	
		#ifdef HALBB_FW_OFLD_SUPPORT
		if (halbb_check_fw_ofld(bb)) {
			halbb_fwofld_set_gain_cr_init_8852c(bb);
		} else {
			halbb_set_gain_cr_init_8852c(bb);
		}
		#else
		halbb_set_gain_cr_init_8852c(bb);
		#endif
		break;
	#endif

	#ifdef BB_8192XB_SUPPORT
	case BB_RTL8192XB:
		result &= halbb_cfg_bb_gain_ax_8192xb(bb, is_form_folder,
						       folder_len, folder_array);
		break;
	#endif

	#ifdef BB_8851B_SUPPORT
	case BB_RTL8851B:
		result &= halbb_cfg_bb_gain_ax_8851b(bb, is_form_folder,
						     folder_len, folder_array);
		break;
	#endif
	#if 0//def BB_8922A_SUPPORT
	case BB_RTL8922A:
		result &= halbb_cfg_bb_gain_gt2(bb, is_form_folder,
						folder_len, folder_array);
		break;
	#endif

	default:
		#ifdef HALBB_COMPILE_BE_SERIES
		if (bb->bb_80211spec == BB_BE_IC) {
			result &= halbb_cfg_bb_gain_gt2(bb, is_form_folder,
							folder_len, folder_array);
		} else	
		#endif
		{
			BB_WARNING("[%s] ic=%d\n", __func__, bb->hal_com->chip_id);
		}
		break;
	}

	BB_DBG(bb, DBG_INIT, "BB_Gain_table_init_success = %d\n", result);
	return result;
}


void halbb_get_efuse_ofst_init(struct bb_info *bb)
{
	switch (bb->ic_type) {

	#ifdef BB_8852A_2_SUPPORT
	case BB_RTL8852A:
		halbb_get_efuse_ofst_init_8852a_2(bb);
		break;
	#endif
	#ifdef BB_8852B_SUPPORT
	case BB_RTL8852B:
		halbb_get_efuse_ofst_init_8852b(bb);
		break;
	#endif

	#ifdef BB_8852C_SUPPORT
	case BB_RTL8852C:
		halbb_get_efuse_ofst_init_8852c(bb);
		break;
	#endif

	#ifdef BB_8192XB_SUPPORT
	case BB_RTL8192XB:
		halbb_get_efuse_ofst_init_8192xb(bb);
		break;
	#endif

	#ifdef BB_8851B_SUPPORT
	case BB_RTL8851B:
		halbb_get_efuse_ofst_init_8851b(bb);
		break;
	#endif

	#ifdef BB_8922A_SUPPORT
	case BB_RTL8922A:
		halbb_get_efuse_ofst_init_8922a(bb);
		break;
	#endif

	default:
		break;
	}
}

bool halbb_init_reg(struct bb_info *bb)
{
	struct rtw_para_info_t *reg = NULL;
	bool rpt_0 = true, rpt_1 = true, rpt_gain = true;

	#ifdef HALBB_FW_OFLD_SUPPORT
	halbb_fwofld_bitmap_en(bb, true, FW_OFLD_PHY_0_CR_INIT);
	#endif

	BB_DBG(bb, DBG_INIT, "[%s] dbcc_en=%d\n", __func__, bb->hal_com->dbcc_en);

	reg = &bb->phl_com->phy_sw_cap[HW_PHY_0].bb_phy_reg_info;
	rpt_0 = halbb_init_cr_default(bb, reg->para_src, reg->para_data_len, reg->para_data, HW_PHY_0);

	#ifdef HALBB_FW_OFLD_SUPPORT
	halbb_fwofld_bitmap_en(bb, false, FW_OFLD_PHY_0_CR_INIT);
	#endif

	if (bb->hal_com->dbcc_en || bb->bb_cmn_hooker->ic_dual_phy_support) {
		reg = &bb->phl_com->phy_sw_cap[HW_PHY_1].bb_phy_reg_info;
		rpt_1 = halbb_init_cr_default(bb, reg->para_src, reg->para_data_len, reg->para_data, HW_PHY_1);
	}

	reg = &bb->phl_com->phy_sw_cap[HW_PHY_0].bb_phy_reg_gain_info;
	rpt_gain = halbb_init_gain_table(bb, reg->para_src, reg->para_data_len, reg->para_data, HW_PHY_0);

	halbb_get_efuse_ofst_init(bb);

	BB_DBG(bb, DBG_INIT, "phy0/1/gain success: {%d, %d, %d}\n", rpt_0, rpt_1, rpt_gain);

	if (rpt_0 && rpt_1 && rpt_gain)
		return true;
	else
		return false;
}

bool halbb_init_bb_cr_per_phy(struct bb_info *bb, enum phl_phy_idx phy_idx)
{
	struct rtw_para_info_t *reg = NULL;
	bool rpt = true;

	BB_DBG(bb, DBG_INIT, "[%s] phy_idx=%d\n", __func__, phy_idx);

	if (phy_idx == HW_PHY_0) {
		reg = &bb->phl_com->phy_sw_cap[HW_PHY_0].bb_phy_reg_info;
		rpt = halbb_init_cr_default(bb, reg->para_src, reg->para_data_len, reg->para_data, HW_PHY_0);
		BB_DBG(bb, DBG_INIT, "phy0 success: %d\n", rpt);
	} else if (phy_idx == HW_PHY_1 && bb->hal_com->dbcc_en) {
		reg = &bb->phl_com->phy_sw_cap[HW_PHY_1].bb_phy_reg_info;

		#ifdef HALBB_FW_OFLD_SUPPORT
		halbb_fwofld_bitmap_en(bb, true, FW_OFLD_PHY_1_CR_INIT);
		#endif

		rpt = halbb_init_cr_default(bb, reg->para_src, reg->para_data_len, reg->para_data, HW_PHY_1);

		#ifdef HALBB_FW_OFLD_SUPPORT
		halbb_fwofld_bitmap_en(bb, false, FW_OFLD_PHY_1_CR_INIT);
		#endif

		BB_DBG(bb, DBG_INIT, "phy1 success: %d\n", rpt);
		#ifdef HALBB_DBCC_SUPPORT
		if (bb->bb_phy_hooker)
			halbb_mem_cpy(bb, &bb->bb_phy_hooker->bb_gain_i, &bb->bb_gain_i, sizeof(struct bb_gain_info));
		#endif
	} else {
		rpt = false;
	}
	return rpt;
}

void halbb_rx_gain_table_dbg(struct bb_info *bb, char input[][16], 
			     u32 *_used, char *output, u32 *_out_len)
{
	struct bb_hw_cfg_info *hw_cfg = &bb->bb_hw_cfg_i;
	struct bb_hw_cfg_cr_info *cr = &bb->bb_hw_cfg_i.bb_hw_cfg_cr_i;
	struct bb_gain_gen2_info *gain_gt2 = &bb->bb_gain_gen2_i;
	struct bb_gain_info *gain = &bb->bb_gain_i;
	u32 val[10] = {0};
	u32 val_32 = 0;
	u8 i = 0, j = 0, k = 0;
	u8 bw = 0;
	char *text_val = NULL;

	if (_os_strcmp(input[1], "-h") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "{init}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "{dump_reg}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "{show_gt2}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "{show}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "set {lna, tia} band path idx val\n");
		return;
	}

	if (_os_strcmp(input[1], "init") == 0) {
		halbb_hw_cfg_init(bb);
	} else if (_os_strcmp(input[1], "dump_reg") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			"Current_5g_6g_cfg bw=%d, band=%d\n",
			hw_cfg->curr_5g_6g_cfg_bw_gt2, hw_cfg->curr_5g_6g_cfg_band_gt2);

		for (i = 0; i < BB_GT2_TAB_NUM; i++) {
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
				    "[%d G]\n", (i == BB_GT2_TAB_2G) ? 2 : 5);

			for (j = 0; j < BB_GT2_PATH_NUM; j++) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					    "    [path = %d]\n", j);
				
				for (k = 0; k < BB_GT2_LNA_NUM; k++) {
					val_32 = halbb_get_reg_cmn(bb, cr->lna_gain_cr[i][j][k], cr->lna_gain_cr_m[i][k], bb->bb_phy_idx);
					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						    "        [lna = %d] Reg0x%x[0x%08x] = 0x%02x\n",
						    k, cr->lna_gain_cr[i][j][k], cr->lna_gain_cr_m[i][k], val_32);
				}
				for (k = 0; k < BB_GT2_TIA_NUM; k++) {
					val_32 = halbb_get_reg_cmn(bb, cr->tia_gain_cr[i][j][k], cr->tia_gain_cr_m[i][k], bb->bb_phy_idx);
					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						    "        [tia = %d] Reg0x%x[0x%08x] = 0x%02x\n",
						    k, cr->tia_gain_cr[i][j][k], cr->tia_gain_cr_m[i][k], val_32);
				}
				for (k = 0; k < BB_GT2_LNA_NUM; k++) {
					val_32 = halbb_get_reg_cmn(bb, cr->lna_op1db_cr[i][j][k], cr->lna_op1db_cr_m[i][k], bb->bb_phy_idx);
					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						    "        [lna_op1db = %d] Reg0x%x[0x%08x] = 0x%02x\n",
						    k, cr->lna_op1db_cr[i][j][k], cr->lna_op1db_cr_m[i][k], val_32);
				}
				for (k = 0; k < BB_GT2_TIA_LNA_OP1DB_NUM; k++) {
					val_32 = halbb_get_reg_cmn(bb, cr->tia_lna_op1db_cr[i][j][k], cr->tia_lna_op1db_cr_m[i][k], bb->bb_phy_idx);
					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						    "        [tia_lna_op1db = %d] Reg0x%x[0x%08x] = 0x%02x\n",
						    k, cr->tia_lna_op1db_cr[i][j][k], cr->tia_lna_op1db_cr_m[i][k], val_32);
				}
			}
		}
	} else if (_os_strcmp(input[1], "show_gt2") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			"Current_5g_6g_cfg bw=%d, band=%d\n",
			hw_cfg->curr_5g_6g_cfg_bw_gt2, hw_cfg->curr_5g_6g_cfg_band_gt2);

		for (i = 0; i < BB_GT2_BAND_NUM; i++) {
			if (i == hw_cfg->curr_5g_6g_cfg_band_gt2)
				text_val = "V";
			else
				text_val = " ";

			if (i == BB_GT2_BAND_2G) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"(V)======[%02d: 2G]===\n", i);
			} else if (i < BB_GT2_BAND_6G_L0) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"(%s)===[%02d: 5G-%d]===\n", text_val, i, i - BB_GT2_BAND_5G_L);
			} else {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"(%s)===[%02d: 6G-%d]===\n", text_val, i, i - BB_GT2_BAND_6G_L0);
			}

			for (bw = 0; bw < BB_GT2_BW_NUM; bw++) {
				if ((i == hw_cfg->curr_5g_6g_cfg_band_gt2 &&
				    bw == hw_cfg->curr_5g_6g_cfg_bw_gt2) || i == 0)
					text_val = "V";
				else
					text_val = " ";

				if (bw == BB_GT2_BW_20_40) {
					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						    "  (%s)[BW 20/40]\n", text_val);
				} else {
					if (i == BB_GT2_BAND_2G)
						break;

					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						    "  (%s)[BW 80/160/320]\n", text_val);
				}

				for (j = 0; j < BB_GT2_PATH_NUM; j++) {
					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						"      [Path=%d]\n",j);
					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						"        LNA_gain =      {%03d, %03d, %03d, %03d, %03d, %03d, %03d}\n",
						gain_gt2->lna_gain[i][bw][j][0],
						gain_gt2->lna_gain[i][bw][j][1],
						gain_gt2->lna_gain[i][bw][j][2],
						gain_gt2->lna_gain[i][bw][j][3],
						gain_gt2->lna_gain[i][bw][j][4],
						gain_gt2->lna_gain[i][bw][j][5],
						gain_gt2->lna_gain[i][bw][j][6]);
					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						"        TIA_gain =      {%03d, %03d}\n",
						gain_gt2->tia_gain[i][bw][j][0],
						gain_gt2->tia_gain[i][bw][j][1]);

					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						"        LNA_OP1dB =     {%03d, %03d, %03d, %03d, %03d, %03d, %03d}\n",
						gain_gt2->lna_op1db[i][bw][j][0],
						gain_gt2->lna_op1db[i][bw][j][1],
						gain_gt2->lna_op1db[i][bw][j][2],
						gain_gt2->lna_op1db[i][bw][j][3],
						gain_gt2->lna_op1db[i][bw][j][4],
						gain_gt2->lna_op1db[i][bw][j][5],
						gain_gt2->lna_op1db[i][bw][j][6]);
					BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
						"        TIA_LNA_OP1dB = {%03d, %03d, %03d, %03d, %03d, %03d, %03d, %03d}\n",
						gain_gt2->tia_lna_op1db[i][bw][j][0],
						gain_gt2->tia_lna_op1db[i][bw][j][1],
						gain_gt2->tia_lna_op1db[i][bw][j][2],
						gain_gt2->tia_lna_op1db[i][bw][j][3],
						gain_gt2->tia_lna_op1db[i][bw][j][4],
						gain_gt2->tia_lna_op1db[i][bw][j][5],
						gain_gt2->tia_lna_op1db[i][bw][j][6],
						gain_gt2->tia_lna_op1db[i][bw][j][7]);
				}
			}
		}
	} else if (_os_strcmp(input[1], "show") == 0) {
		for (i = 0; i < BB_GAIN_BAND_NUM; i++) {
			if (i == 0) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"===[2G]===\n");
			} else if (i < 4) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"===[5G-%s]===\n", (i == 1) ? ("Low") : ((i == 2) ? "Mid" : "High"));
			} else {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"===[6G-%s]===\n", (i == 4) ? ("Low") : ((i == 5) ? "Mid" : ((i == 6) ? "High" : "Ultra-High")));
			}
			for (j = 0; j < HALBB_MAX_PATH; j++) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"LNA_gain[Path=%d] = {%d, %d, %d, %d, %d, %d, %d}\n",
					j,
					gain->lna_gain[i][j][0],
					gain->lna_gain[i][j][1],
					gain->lna_gain[i][j][2],
					gain->lna_gain[i][j][3],
					gain->lna_gain[i][j][4],
					gain->lna_gain[i][j][5],
					gain->lna_gain[i][j][6]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"TIA_gain[Path=%d] = {%d, %d}\n",
					j,
					gain->tia_gain[i][j][0],
					gain->tia_gain[i][j][1]);
			}
		} 
	} else if (_os_strcmp(input[1], "set") == 0) {
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[4], DCMD_DECIMAL, &val[1]);
		HALBB_SCAN(input[5], DCMD_DECIMAL, &val[2]);
		HALBB_SCAN(input[6], DCMD_DECIMAL, &val[3]);
		
		if (_os_strcmp(input[2], "lna") == 0) {
			if (val[0] >= BB_GAIN_BAND_NUM ||
			    val[1] >= HALBB_MAX_PATH ||
			    val[2] >= IC_LNA_NUM) {
			    BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 	"Set Err\n");
			    return;
			}
			gain->lna_gain[val[0]][val[1]][val[2]] = (s8)val[3];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		 		"Set lna_gain[%d][%d][%d] = %d\n",
		 		val[0], val[1], val[2], val[3]);
			
		} else if (_os_strcmp(input[2], "tia") == 0) {
			if (val[0] >= BB_GAIN_BAND_NUM ||
			    val[1] >= HALBB_MAX_PATH ||
			    val[2] >= IC_TIA_NUM) {
			    BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 	"Set Err\n");
			    return;
			}
			gain->tia_gain[val[0]][val[1]][val[2]] = (s8)val[3];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		 		"Set tia_gain[%d][%d][%d] = %d\n",
		 		val[0], val[1], val[2], val[3]);
		}
		//halbb_set_gain_error(bb, bb->hal_com->band[bb->bb_phy_idx].cur_chandef.center_ch);
	} else {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		 	"Set Err\n");
	}
}

void halbb_rx_op1db_table_dbg(struct bb_info *bb, char input[][16],
			      u32 *_used, char *output, u32 *_out_len)
{
	struct bb_gain_info *gain = &bb->bb_gain_i;
	u32 val[10] = {0};
	u8 i = 0, j = 0;

	if (_os_strcmp(input[1], "-h") == 0) {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "{show}\n");
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 "set {lna_op1db, tia_lna_op1db} band path idx val\n");
		return;
	}

	if (_os_strcmp(input[1], "show") == 0) {
		for (i = 0; i < BB_GAIN_BAND_NUM; i++) {
			if (i == 0) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"===[2G]===\n");
			} else if (i < 4) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"===[5G-%s]===\n", (i == 1) ? ("Low") : ((i == 2) ? "Mid" : "High"));
			} else {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 		"===[6G-%s]===\n", (i == 4) ? ("Low") : ((i == 5) ? "Mid" : ((i == 6) ? "High" : "Ultra-High")));
			}
			for (j = 0; j < HALBB_MAX_PATH; j++) {
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"LNA_op1db[Path=%d] = {%d, %d, %d, %d, %d, %d, %d}\n",
					j,
					gain->lna_op1db[i][j][0],
					gain->lna_op1db[i][j][1],
					gain->lna_op1db[i][j][2],
					gain->lna_op1db[i][j][3],
					gain->lna_op1db[i][j][4],
					gain->lna_op1db[i][j][5],
					gain->lna_op1db[i][j][6]);
				BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
					"TIA_LNA_op1db[Path=%d] = {%d, %d, %d, %d, %d, %d, %d, %d}\n",
					j,
					gain->tia_lna_op1db[i][j][0],
					gain->tia_lna_op1db[i][j][1],
					gain->tia_lna_op1db[i][j][2],
					gain->tia_lna_op1db[i][j][3],
					gain->tia_lna_op1db[i][j][4],
					gain->tia_lna_op1db[i][j][5],
					gain->tia_lna_op1db[i][j][6],
					gain->tia_lna_op1db[i][j][7]);
			}
		}
	} else if (_os_strcmp(input[1], "set") == 0) {
		HALBB_SCAN(input[3], DCMD_DECIMAL, &val[0]);
		HALBB_SCAN(input[4], DCMD_DECIMAL, &val[1]);
		HALBB_SCAN(input[5], DCMD_DECIMAL, &val[2]);
		HALBB_SCAN(input[6], DCMD_DECIMAL, &val[3]);

		if (_os_strcmp(input[2], "lna_op1db") == 0) {
			if (val[0] >= BB_GAIN_BAND_NUM ||
			    val[1] >= HALBB_MAX_PATH ||
			    val[2] >= IC_LNA_OP1DB_NUM) {
			    BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 	"Set Err\n");
			    return;
			}
			gain->lna_op1db[val[0]][val[1]][val[2]] = (s8)val[3];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		 		"Set lna_op1db[%d][%d][%d] = %d\n",
		 		val[0], val[1], val[2], val[3]);
		} else if (_os_strcmp(input[2], "tia_lna_op1db") == 0) {
			if (val[0] >= BB_GAIN_BAND_NUM ||
			    val[1] >= HALBB_MAX_PATH ||
			    val[2] >= IC_TIA_LNA_OP1DB_NUM) {
			    BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
			 	"Set Err\n");
			    return;
			}
			gain->tia_lna_op1db[val[0]][val[1]][val[2]] = (s8)val[3];
			BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		 		"Set tia_lna_op1db[%d][%d][%d] = %d\n",
		 		val[0], val[1], val[2], val[3]);
		}
		//halbb_set_gain_error(bb, bb->hal_com->band[bb->bb_phy_idx].cur_chandef.center_ch);
	} else {
		BB_DBG_CNSL(*_out_len, *_used, output + *_used, *_out_len - *_used,
		 	"Set Err\n");
	}
}

void halbb_hw_cfg_init(struct bb_info *bb)
{
	struct bb_hw_cfg_info *hw_cfg = &bb->bb_hw_cfg_i;

	hw_cfg->gain_table_init_ready_2g_a = false;
	hw_cfg->gain_table_init_ready_2g_b = false;
}

void halbb_cr_cfg_hw_cfg_init(struct bb_info *bb)
{
	struct bb_hw_cfg_cr_info *cr = &bb->bb_hw_cfg_i.bb_hw_cfg_cr_i;

	if (bb->bb_80211spec != BB_BE_IC)
		return;

	switch (bb->cr_type) {

	#ifdef HALBB_COMPILE_BE1_SERIES
	case BB_BE1:
	/* === [LNA/TIA] =====================================================*/
		//2G
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][0] = PATH0_R_G_G_LNA0_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][1] = PATH0_R_G_G_LNA1_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][2] = PATH0_R_G_G_LNA2_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][3] = PATH0_R_G_G_LNA3_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][4] = PATH0_R_G_G_LNA4_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][5] = PATH0_R_G_G_LNA5_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][6] = PATH0_R_G_G_LNA6_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][0] = PATH1_R_G_G_LNA0_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][1] = PATH1_R_G_G_LNA1_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][2] = PATH1_R_G_G_LNA2_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][3] = PATH1_R_G_G_LNA3_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][4] = PATH1_R_G_G_LNA4_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][5] = PATH1_R_G_G_LNA5_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][6] = PATH1_R_G_G_LNA6_BE1;

		cr->lna_gain_cr_m[BB_GT2_TAB_2G][0] = PATH0_R_G_G_LNA0_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_2G][1] = PATH0_R_G_G_LNA1_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_2G][2] = PATH0_R_G_G_LNA2_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_2G][3] = PATH0_R_G_G_LNA3_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_2G][4] = PATH0_R_G_G_LNA4_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_2G][5] = PATH0_R_G_G_LNA5_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_2G][6] = PATH0_R_G_G_LNA6_BE1_M;

		cr->tia_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][0] = PATH0_R_G_G_TIA0_BE1;
		cr->tia_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][1] = PATH0_R_G_G_TIA1_BE1;
		cr->tia_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][0] = PATH1_R_G_G_TIA0_BE1;
		cr->tia_gain_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][1] = PATH1_R_G_G_TIA1_BE1;

		cr->tia_gain_cr_m[BB_GT2_TAB_2G][0] = PATH0_R_G_G_TIA0_BE1_M;
		cr->tia_gain_cr_m[BB_GT2_TAB_2G][1] = PATH0_R_G_G_TIA1_BE1_M;

		//5G
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][0] = PATH0_R_A_G_LNA0_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][1] = PATH0_R_A_G_LNA1_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][2] = PATH0_R_A_G_LNA2_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][3] = PATH0_R_A_G_LNA3_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][4] = PATH0_R_A_G_LNA4_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][5] = PATH0_R_A_G_LNA5_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][6] = PATH0_R_A_G_LNA6_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][0] = PATH1_R_A_G_LNA0_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][1] = PATH1_R_A_G_LNA1_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][2] = PATH1_R_A_G_LNA2_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][3] = PATH1_R_A_G_LNA3_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][4] = PATH1_R_A_G_LNA4_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][5] = PATH1_R_A_G_LNA5_BE1;
		cr->lna_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][6] = PATH1_R_A_G_LNA6_BE1;

		cr->lna_gain_cr_m[BB_GT2_TAB_5G_6G][0] = PATH0_R_A_G_LNA0_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_5G_6G][1] = PATH0_R_A_G_LNA1_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_5G_6G][2] = PATH0_R_A_G_LNA2_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_5G_6G][3] = PATH0_R_A_G_LNA3_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_5G_6G][4] = PATH0_R_A_G_LNA4_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_5G_6G][5] = PATH0_R_A_G_LNA5_BE1_M;
		cr->lna_gain_cr_m[BB_GT2_TAB_5G_6G][6] = PATH0_R_A_G_LNA6_BE1_M;

		cr->tia_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][0] = PATH0_R_A_G_TIA0_BE1;
		cr->tia_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][1] = PATH0_R_A_G_TIA1_BE1;
		cr->tia_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][0] = PATH1_R_A_G_TIA0_BE1;
		cr->tia_gain_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][1] = PATH1_R_A_G_TIA1_BE1;

		cr->tia_gain_cr_m[BB_GT2_TAB_5G_6G][0] = PATH0_R_A_G_TIA0_BE1_M;
		cr->tia_gain_cr_m[BB_GT2_TAB_5G_6G][1] = PATH0_R_A_G_TIA1_BE1_M;

	/* === [OP1dB & tia_lna_op1db] =======================================*/
		//2G
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][0] = PATH0_R_A_LNA0_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][1] = PATH0_R_A_LNA1_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][2] = PATH0_R_A_LNA2_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][3] = PATH0_R_A_LNA3_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][4] = PATH0_R_A_LNA4_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][5] = PATH0_R_A_LNA5_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][6] = PATH0_R_A_LNA6_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][0] = PATH1_R_A_LNA0_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][1] = PATH1_R_A_LNA1_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][2] = PATH1_R_A_LNA2_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][3] = PATH1_R_A_LNA3_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][4] = PATH1_R_A_LNA4_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][5] = PATH1_R_A_LNA5_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][6] = PATH1_R_A_LNA6_OP1DB_BE1;

		cr->lna_op1db_cr_m[BB_GT2_TAB_2G][0] = PATH0_R_A_LNA0_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_2G][1] = PATH0_R_A_LNA1_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_2G][2] = PATH0_R_A_LNA2_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_2G][3] = PATH0_R_A_LNA3_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_2G][4] = PATH0_R_A_LNA4_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_2G][5] = PATH0_R_A_LNA5_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_2G][6] = PATH0_R_A_LNA6_OP1DB_BE1_M;

		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][0] = PATH0_R_A_TIA0_LNA0_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][1] = PATH0_R_A_TIA0_LNA1_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][2] = PATH0_R_A_TIA0_LNA2_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][3] = PATH0_R_A_TIA0_LNA3_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][4] = PATH0_R_A_TIA0_LNA4_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][5] = PATH0_R_A_TIA0_LNA5_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][6] = PATH0_R_A_TIA0_LNA6_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_A][7] = PATH0_R_A_TIA1_LNA6_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][0] = PATH1_R_A_TIA0_LNA0_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][1] = PATH1_R_A_TIA0_LNA1_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][2] = PATH1_R_A_TIA0_LNA2_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][3] = PATH1_R_A_TIA0_LNA3_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][4] = PATH1_R_A_TIA0_LNA4_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][5] = PATH1_R_A_TIA0_LNA5_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][6] = PATH1_R_A_TIA0_LNA6_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_2G][BB_GT2_PATH_B][7] = PATH1_R_A_TIA1_LNA6_OP1DB_BE1;

		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_2G][0] = PATH0_R_A_TIA0_LNA0_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_2G][1] = PATH0_R_A_TIA0_LNA1_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_2G][2] = PATH0_R_A_TIA0_LNA2_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_2G][3] = PATH0_R_A_TIA0_LNA3_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_2G][4] = PATH0_R_A_TIA0_LNA4_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_2G][5] = PATH0_R_A_TIA0_LNA5_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_2G][6] = PATH0_R_A_TIA0_LNA6_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_2G][7] = PATH0_R_A_TIA1_LNA6_OP1DB_BE1_M;

		//5G
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][0] = PATH0_R_G_LNA0_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][1] = PATH0_R_G_LNA1_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][2] = PATH0_R_G_LNA2_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][3] = PATH0_R_G_LNA3_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][4] = PATH0_R_G_LNA4_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][5] = PATH0_R_G_LNA5_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][6] = PATH0_R_G_LNA6_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][0] = PATH1_R_G_LNA0_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][1] = PATH1_R_G_LNA1_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][2] = PATH1_R_G_LNA2_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][3] = PATH1_R_G_LNA3_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][4] = PATH1_R_G_LNA4_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][5] = PATH1_R_G_LNA5_OP1DB_BE1;
		cr->lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][6] = PATH1_R_G_LNA6_OP1DB_BE1;

		cr->lna_op1db_cr_m[BB_GT2_TAB_5G_6G][0] = PATH0_R_G_LNA0_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_5G_6G][1] = PATH0_R_G_LNA1_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_5G_6G][2] = PATH0_R_G_LNA2_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_5G_6G][3] = PATH0_R_G_LNA3_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_5G_6G][4] = PATH0_R_G_LNA4_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_5G_6G][5] = PATH0_R_G_LNA5_OP1DB_BE1_M;
		cr->lna_op1db_cr_m[BB_GT2_TAB_5G_6G][6] = PATH0_R_G_LNA6_OP1DB_BE1_M;

		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][0] = PATH0_R_G_TIA0_LNA0_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][1] = PATH0_R_G_TIA0_LNA1_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][2] = PATH0_R_G_TIA0_LNA2_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][3] = PATH0_R_G_TIA0_LNA3_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][4] = PATH0_R_G_TIA0_LNA4_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][5] = PATH0_R_G_TIA0_LNA5_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][6] = PATH0_R_G_TIA0_LNA6_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_A][7] = PATH0_R_G_TIA1_LNA6_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][0] = PATH1_R_G_TIA0_LNA0_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][1] = PATH1_R_G_TIA0_LNA1_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][2] = PATH1_R_G_TIA0_LNA2_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][3] = PATH1_R_G_TIA0_LNA3_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][4] = PATH1_R_G_TIA0_LNA4_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][5] = PATH1_R_G_TIA0_LNA5_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][6] = PATH1_R_G_TIA0_LNA6_OP1DB_BE1;
		cr->tia_lna_op1db_cr[BB_GT2_TAB_5G_6G][BB_GT2_PATH_B][7] = PATH1_R_G_TIA1_LNA6_OP1DB_BE1;

		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_5G_6G][0] = PATH0_R_G_TIA0_LNA0_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_5G_6G][1] = PATH0_R_G_TIA0_LNA1_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_5G_6G][2] = PATH0_R_G_TIA0_LNA2_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_5G_6G][3] = PATH0_R_G_TIA0_LNA3_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_5G_6G][4] = PATH0_R_G_TIA0_LNA4_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_5G_6G][5] = PATH0_R_G_TIA0_LNA5_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_5G_6G][6] = PATH0_R_G_TIA0_LNA6_OP1DB_BE1_M;
		cr->tia_lna_op1db_cr_m[BB_GT2_TAB_5G_6G][7] = PATH0_R_G_TIA1_LNA6_OP1DB_BE1_M;

		break;
	#endif
	default:
		BB_WARNING("[%s] BBCR Hook FAIL!\n", __func__);
		if (bb->bb_dbg_i.cr_fake_init_hook_en) {
			BB_TRACE("[%s] BBCR fake init\n", __func__);
			halbb_cr_hook_fake_init(bb, (u32 *)cr, (sizeof(struct bb_cfo_trk_cr_info) >> 2));
		}
		break;
	}

	if (bb->bb_dbg_i.cr_init_hook_recorder_en) {
		BB_TRACE("[%s] BBCR Hook dump\n", __func__);
		halbb_cr_hook_init_dump(bb, (u32 *)cr, (sizeof(struct bb_cfo_trk_cr_info) >> 2));
	}
}

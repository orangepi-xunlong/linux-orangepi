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
#ifndef __HALBB_HW_CFG_H__
#define __HALBB_HW_CFG_H__

/*@--------------------------[Define] ---------------------------------------*/

#define BB_DONT_CARE	0xff

#define BB_IF		0x8
#define BB_ELSE_IF	0x9
#define BB_ELSE		0xa
#define BB_END		0xb
#define BB_CHK		0x4

#define BB_RXSC_NUM_40		9 /*SC:0,1~8*/
#define BB_RXSC_NUM_80		13 /*SC:0,1~8,9~12*/
#define BB_RXSC_NUM_160		15 /*SC:0,1~8,9~12,13~14*/
#define BB_RXSC_START_IDX_FULL	0
#define BB_RXSC_START_IDX_20	1
#define BB_RXSC_START_IDX_20_1	5
#define BB_RXSC_START_IDX_40	9
#define BB_RXSC_START_IDX_80	13

#define BB_BAND_NUM_MAX		12 /*2G:1, 5G:3, 6G:8*/
#define BB_HIDE_EFUSE_SIZE	55

#define BB_GT2_TIA_LNA_OP1DB_NUM	8 // [TIA0 & LNA0~6] + [TIA1_LNA6]
#define BB_GT2_LNA_NUM			7
#define BB_GT2_TIA_NUM			2

/*@--------------------------[Enum]------------------------------------------*/
enum bb_band_t {
	BB_BAND_2G	= 0,
	BB_BAND_5G_L	= 1,
	BB_BAND_5G_M	= 2,
	BB_BAND_5G_H	= 3,
	BB_BAND_6G_L	= 4,
	BB_BAND_6G_M	= 5,
	BB_BAND_6G_H	= 6,
	BB_BAND_6G_UH	= 7,
	BB_GAIN_BAND_NUM	= 8
};

enum bb_func_type_gt2_t { /*GT2: Gain Table Gen2*/
	BB_GT2_FUNC_GAIN_ERROR	= 0,
	BB_GT2_FUNC_RPL		= 1,
	BB_GT2_FUNC_BYPASS	= 2,
	BB_GT2_FUNC_OP1DB	= 3,
	BB_GT2_FUNC_WBADC	= 4
};

enum bb_tab_idx_gt2_t { /*GT2: Gain Table Gen2*/
	BB_GT2_TAB_2G		= 0,
	BB_GT2_TAB_5G_6G	= 1,
	BB_GT2_TAB_NUM		= 2
};

enum bb_band_gt2_t { /*GT2: Gain Table Gen2*/
	BB_GT2_BAND_2G		= 0,
	BB_GT2_BAND_5G_L	= 1,
	BB_GT2_BAND_5G_M	= 2,
	BB_GT2_BAND_5G_H	= 3,
	BB_GT2_BAND_6G_L0	= 4,
	BB_GT2_BAND_6G_L1	= 5,
	BB_GT2_BAND_6G_M0	= 6,
	BB_GT2_BAND_6G_M1	= 7,
	BB_GT2_BAND_6G_H0	= 8,
	BB_GT2_BAND_6G_H1	= 9,
	BB_GT2_BAND_6G_UH0	= 10,
	BB_GT2_BAND_6G_UH1	= 11,
	BB_GT2_BAND_NUM		= 12
};

enum bb_bw_gt2_t { /*GT2: Gain Table Gen2*/
	BB_GT2_BW_20_40		= 0,
	BB_GT2_BW_80_160_320	= 1,
	BB_GT2_BW_NUM		= 2
};

enum bb_path_gt2_t { /*GT2: Gain Table Gen2*/
	BB_GT2_PATH_A	= 0,
	BB_GT2_PATH_B	= 1,
	BB_GT2_PATH_NUM	= 2
};

/*@--------------------------[Structure]-------------------------------------*/
struct bb_gain_gen2_info {
	s8 lna_gain[BB_GT2_BAND_NUM][BB_GT2_BW_NUM][BB_GT2_PATH_NUM][BB_GT2_LNA_NUM];
	s8 tia_gain[BB_GT2_BAND_NUM][BB_GT2_BW_NUM][BB_GT2_PATH_NUM][BB_GT2_TIA_NUM];
	s8 lna_op1db[BB_GT2_BAND_NUM][BB_GT2_BW_NUM][BB_GT2_PATH_NUM][BB_GT2_LNA_NUM];
	s8 tia_lna_op1db[BB_GT2_BAND_NUM][BB_GT2_BW_NUM][BB_GT2_PATH_NUM][BB_GT2_TIA_LNA_OP1DB_NUM];

};

struct bb_hw_cfg_cr_info {
	u32 lna_gain_cr[BB_GT2_TAB_NUM][BB_GT2_PATH_NUM][BB_GT2_LNA_NUM];
	u32 lna_gain_cr_m[BB_GT2_TAB_NUM][BB_GT2_LNA_NUM];
	u32 tia_gain_cr[BB_GT2_TAB_NUM][BB_GT2_PATH_NUM][BB_GT2_TIA_NUM];
	u32 tia_gain_cr_m[BB_GT2_TAB_NUM][BB_GT2_TIA_NUM];
	u32 lna_op1db_cr[BB_GT2_TAB_NUM][BB_GT2_PATH_NUM][BB_GT2_LNA_NUM];
	u32 lna_op1db_cr_m[BB_GT2_TAB_NUM][BB_GT2_LNA_NUM];
	u32 tia_lna_op1db_cr[BB_GT2_TAB_NUM][BB_GT2_PATH_NUM][BB_GT2_TIA_LNA_OP1DB_NUM];
	u32 tia_lna_op1db_cr_m[BB_GT2_TAB_NUM][BB_GT2_TIA_LNA_OP1DB_NUM];

};

struct bb_hw_cfg_info {
	struct	bb_hw_cfg_cr_info	bb_hw_cfg_cr_i;
	bool gain_table_init_ready_2g_a;
	bool gain_table_init_ready_2g_b;
	enum bb_bw_gt2_t	curr_5g_6g_cfg_bw_gt2;
	enum bb_band_gt2_t	curr_5g_6g_cfg_band_gt2;
};

/*@--------------------------[Prptotype]-------------------------------------*/
struct bb_info;
void halbb_set_lna_tia_gain_bbcr_gt2(struct bb_info *bb, u8 central_ch, enum band_type band_type, enum channel_width bw, enum rf_path path);
bool halbb_sel_headline(struct bb_info *bb, u32 *array, u32 array_len,
			u8 *headline_size, u8 *headline_idx);
void halbb_cfg_bb_rpl_ofst(struct bb_info *bb, enum bb_band_t band, u8 path, u32 addr, u32 data);
bool halbb_init_cr_default(struct bb_info *bb, bool is_form_folder, u32 folder_len,
			   u32 *folder_array, enum phl_phy_idx phy_idx);
bool halbb_init_gain_table(struct bb_info *bb, bool is_form_folder, u32 folder_len,
				 u32 *folder_array, enum phl_phy_idx phy_idx);
void halbb_rx_gain_table_dbg(struct bb_info *bb, char input[][16], 
			     u32 *_used, char *output, u32 *_out_len);
void halbb_rx_op1db_table_dbg(struct bb_info *bb, char input[][16], 
			      u32 *_used, char *output, u32 *_out_len);
void halbb_hw_cfg_init(struct bb_info *bb);
void halbb_cr_cfg_hw_cfg_init(struct bb_info *bb);

#endif

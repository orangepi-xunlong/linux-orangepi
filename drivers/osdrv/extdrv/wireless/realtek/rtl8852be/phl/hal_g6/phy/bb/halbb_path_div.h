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
#ifndef __HALBB_PATH_DIV_H__
#define __HALBB_PATH_DIV_H__

#define PATH_DIV_VERSION "1.0"
/*@--------------------------[Define] ---------------------------------------*/
#define	PATH_DIV_RSSI_GAP	4 /*@ 2dB*/

/*@--------------------------[Structure]-------------------------------------*/
#ifdef HALBB_COMPILE_AX_SERIOUS
struct bb_pathdiv_rssi_info { /*all in U(8,1)*/
	/*acc value*/
	u64 path_a_rssi_sum[PHL_MAX_STA_NUM];
	u64 path_b_rssi_sum[PHL_MAX_STA_NUM];
	u16 path_a_pkt_cnt[PHL_MAX_STA_NUM];
	u16 path_b_pkt_cnt[PHL_MAX_STA_NUM];
};
#endif
//struct bb_pathdiv_cr_info {
//};

enum bb_rssi_method_t {
	RSSI_LINEAR_AVG	= 0,
	RSSI_DB_AVG	= 1
};

enum bb_tx_sts_src_idx_t {
	TX_STS_0 = 0,
	TX_STS_1 = 1,
	TX_STS_2 = 2,
	TX_STS_3 = 3,
};

struct bb_path_map_info {
	enum bb_tx_sts_src_idx_t bb_path_map_1sts[HALBB_MAX_PATH];
	enum bb_tx_sts_src_idx_t bb_path_map_2sts[HALBB_MAX_PATH];
	enum bb_tx_sts_src_idx_t bb_path_map_3sts[HALBB_MAX_PATH];
	enum bb_tx_sts_src_idx_t bb_path_map_4sts[HALBB_MAX_PATH];
};

struct bb_pathdiv_info {
	//struct bb_pathdiv_cr_info bb_pathdiv_cr_i;
	/* For RSSI */
#ifdef HALBB_COMPILE_AX_SERIOUS
	struct bb_pathdiv_rssi_info bb_rssi_i;
#endif
	bool fix_path_en[PHL_MAX_STA_NUM]; /*@ debug mode*/
	enum bb_path path_sel_1ss;
	enum bb_path path_sel[PHL_MAX_STA_NUM];
	enum bb_path fix_path_sel[PHL_MAX_STA_NUM];/*@ debug mode*/
	enum bb_rssi_method_t rssi_decision_method;
	u8 path_rssi_gap;
	u64 macid_is_linked;
	u32 rvrt_val; /*all rvrt_val for pause API must set to u32*/

	//boolean fix_path_bfer;
	//boolean is_become_linked;
	//boolean is_u3_mode;
	//u8 dtp_period;
	u8 num_tx_path;
	u8 default_path;
	//u8 num_candidate;
	//u8 ant_candidate_1;
	//u8 ant_candidate_2;
	//u8 ant_candidate_3;
	//u8 phydm_dtp_state;
	//u8 dtp_check_patha_counter;
	//u8	search_space_2[NUM_CHOOSE2_FROM4];
	//u8	search_space_3[NUM_CHOOSE3_FROM4];
	u8 pre_tx_path;
	u8 use_path_a_as_default_ant;
	u8 path_mask;
	bool is_path_a_exist;

	bool tx_path_pmac_ctrl_en;
	struct bb_tx_path_en_info bb_path_en_i;
	struct bb_path_map_info bb_path_map_i;
};

struct bb_info;
/*@--------------------------[Prptotype]-------------------------------------*/
void halbb_cr_cfg_pathdiv_init(struct bb_info *bb);
void halbb_pathdiv_reg_init(struct bb_info *bb);
void halbb_pathdiv_init(struct bb_info *bb);
void halbb_pathdiv_reset(struct bb_info *bb);
void halbb_pathdiv_reset_stat(struct bb_info *bb);
void halbb_set_cctrl_tbl(struct bb_info *bb, u16 macid, u16 cfg);
void halbb_set_tx_path_by_cmac_tbl(struct bb_info *bb, u16 macid, enum bb_path tx_path_sel_1ss);
void halbb_update_tx_path_div(struct bb_info *bb, struct rtw_phl_stainfo_t *sta);
void halbb_path_diversity(struct bb_info *bb);
void halbb_pathdiv_phy_sts(struct bb_info *bb, struct physts_rxd *desc);
void halbb_pathdiv_dbg(struct bb_info *bb, char input[][16], u32 *_used,
			      char *output, u32 *_out_len);
void halbb_set_pathdiv_pause_val(struct bb_info *bb, u32 *val_buf, u8 val_len);
#endif


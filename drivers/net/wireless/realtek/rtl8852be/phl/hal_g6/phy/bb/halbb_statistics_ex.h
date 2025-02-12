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
#ifndef __HALBB_STATISTICS_EX_H__
#define __HALBB_STATISTICS_EX_H__

/*@--------------------------[Define] ---------------------------------------*/

/*@--------------------------[Enum]------------------------------------------*/
enum stat_type_sel {
	STATE_PROBE_RESP	= 1,
	STATE_BEACON		= 2,
	STATE_ACTION		= 3,
	STATE_BFRP		= 4,
	STATE_NDPA		= 5,
	STATE_BA		= 6,
	STATE_RTS		= 7,
	STATE_CTS		= 8,
	STATE_ACK		= 9,
	STATE_DATA		= 10,
	STATE_NULL		= 11,
	STATE_QOS		= 12,
};

enum stat_mac_type {
	TYPE_PROBE_RESP		= 0x05,
	TYPE_BEACON		= 0x08,
	TYPE_ACTION		= 0x0d,
	TYPE_BFRP		= 0x14,
	TYPE_NDPA		= 0x15,
	TYPE_BA			= 0x19,
	TYPE_RTS		= 0x1b,
	TYPE_CTS		= 0x1c,
	TYPE_ACK		= 0x1d,
	TYPE_DATA		= 0x20,
	TYPE_NULL		= 0x24,
	TYPE_QOS		= 0x28,
};

/*@--------------------------[Structure]-------------------------------------*/
#if 1

struct bb_stat_hang_info {
	u16		consecutive_no_tx_cnt;
	u16		consecutive_no_rx_cnt;
	bool		hang_occur;
};

struct bb_usr_set_info {
	u16		ofdm2_rate_idx;
	u16		ht2_rate_idx;
	u16		vht2_rate_idx;
	u16		he2_rate_idx;
	u16		eht2_rate_idx;
	enum stat_mac_type stat_mac_type_i;
	enum stat_type_sel stat_type_sel_i;
};

struct bb_cca_info {
	u32		cnt_ofdm_cca;
	u32		cnt_cck_cca;
	u32		cnt_cca_all;
	u32		cnt_cck_spoofing;
	u32		cnt_ofdm_spoofing;
	u32		cnt_cca_spoofing_all;
	u32		pop_cnt;
};

struct bb_crc_info {
	u32		cnt_ampdu_miss;
	u32		cnt_ampdu_crc_error;
	u32		cnt_ampdu_crc_ok;
	u32		cnt_cck_crc32_error;
	u32		cnt_cck_crc32_ok;
	u32		cnt_ofdm_crc32_error;
	u32		cnt_ofdm_crc32_ok;
	u32		cnt_ht_crc32_error;
	u32		cnt_ht_crc32_ok;
	u32		cnt_vht_crc32_error;
	u32		cnt_vht_crc32_ok;
	u32		cnt_he_crc32_ok;
	u32		cnt_he_crc32_error;
	u32		cnt_eht_crc32_ok;
	u32		cnt_eht_crc32_error;
	u32		cnt_crc32_error_all;
	u32		cnt_crc32_ok_all;
};

struct bb_crc2_info {
	u32		cnt_ofdm2_crc32_error;
	u32		cnt_ofdm2_crc32_ok;
	u8		ofdm2_pcr;
	u32		cnt_ht2_crc32_error;
	u32		cnt_ht2_crc32_ok;
	u8		ht2_pcr;
	u32		cnt_vht2_crc32_error;
	u32		cnt_vht2_crc32_ok;
	u8		vht2_pcr;
	u32		cnt_he2_crc32_error;
	u32		cnt_he2_crc32_ok;
	u8		he2_pcr;
	u32		cnt_eht2_crc32_ok;
	u32		cnt_eht2_crc32_error;
	u8		eht2_pcr;
	u32		cnt_ofdm3_crc32_error;
	u32		cnt_ofdm3_crc32_ok;
};

struct bb_cck_fa_info {
	u32		sfd_gg_cnt;
	u32		sig_gg_cnt;
	u32		cnt_cck_crc_16;
};

struct bb_legacy_fa_info {
	u32		cnt_lsig_brk_s_th;
	u32		cnt_lsig_brk_l_th;
	u32		cnt_parity_fail;
	u32		cnt_rate_illegal;	
	u32		cnt_sb_search_fail;
};

struct bb_ht_fa_info {
	u32		cnt_crc8_fail;
	u32		cnt_crc8_fail_s_th;
	u32		cnt_crc8_fail_l_th;
	u32		cnt_mcs_fail;
};

struct bb_vht_fa_info {
	u32		cnt_crc8_fail_vhta;
	/*u32		cnt_crc8_fail_vhtb; removed at RXD*/
	u32		cnt_mcs_fail_vht;
};

struct bb_he_fa_info {
	u32		cnt_crc4_fail_hea_su;
	u32		cnt_crc4_fail_hea_ersu;
	u32		cnt_crc4_fail_hea_mu;
	u32		cnt_crc4_fail_heb_ch1_mu;
	u32		cnt_crc4_fail_heb_ch2_mu;
	u32		cnt_mcs_fail_he_bcc;
	u32		cnt_mcs_fail_he;
	u32		cnt_mcs_fail_he_dcm;
};

struct bb_fa_info {
	u32		cnt_total_brk;
	u32		cnt_cck_fail;
	u32		cnt_ofdm_fail;
	u32		cnt_fail_all;
	struct bb_cck_fa_info		bb_cck_fa_i;
	struct bb_legacy_fa_info	bb_legacy_fa_i;
	struct bb_ht_fa_info		bb_ht_fa_i;
	struct bb_vht_fa_info		bb_vht_fa_i;
	struct bb_he_fa_info		bb_he_fa_i;
};

struct bb_tx_cnt_info {
	u32		cck_mac_txen;
	u32		cck_phy_txon;
	u32		ofdm_mac_txen;
	u32		ofdm_phy_txon;
};
#endif

struct bb_info;
/*@--------------------------[Prptotype]-------------------------------------*/
void halbb_pmac_statistics_ex(struct bb_info *bb_0, bool en, enum phl_phy_idx phy_idx);
#endif


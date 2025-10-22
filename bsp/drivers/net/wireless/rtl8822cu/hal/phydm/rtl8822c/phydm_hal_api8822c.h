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
#ifndef __INC_PHYDM_API_H_8822C__
#define __INC_PHYDM_API_H_8822C__

#if (RTL8822C_SUPPORT)
/*2020.02.25: Add dis low rate DPD in channel API with iot table for LG TV*/
#define PHY_CONFIG_VERSION_8822C "1.8.8"
/*#define CONFIG_TXAGC_DEBUG_8822C*/

#define INVALID_RF_DATA 0xffffffff
#define INVALID_TXAGC_DATA 0xff
#define L_BND_DEFAULT_8822C 0xd

#define config_phydm_read_rf_check_8822c(data) ((data) != INVALID_RF_DATA)
#define config_phydm_read_txagc_check_8822c(data) ((data) != INVALID_TXAGC_DATA)

enum agc_tab_sel_8822c {
	OFDM_2G_BW40_8822C		= 0,
	OFDM_5G_LOW_BAND_8822C		= 1,
	OFDM_5G_MID_BAND_8822C		= 2,
	OFDM_5G_HIGH_BAND_8822C		= 3,
	CCK_BW40_8822C			= 4,
	CCK_BW20_8822C			= 5,
	OFDM_2G_BW20_8822C		= 6
};

struct txagc_table_8822c {
	u8 ref_pow_cck[2];
	u8 ref_pow_ofdm[2];
	s8 diff_t[NUM_RATE_AC_2SS]; /*by rate differential table*/
};

struct tx_path_en_8822c {
	u8 tx_path_en_ofdm_1sts;
	u8 tx_path_en_ofdm_2sts;
	u8 tx_path_en_cck;
	boolean is_path_ctrl_by_bb_reg;
	boolean stop_path_div;
};

struct rx_path_en_8822c {
	u8 rx_path_en_ofdm;
	u8 rx_path_en_cck;
};

boolean phydm_chk_pkg_set_valid_8822c(struct dm_struct *dm,
				      u8 ver_bb, u8 ver_rf);

u32 config_phydm_read_rf_reg_8822c(struct dm_struct *dm, enum rf_path path,
				   u32 reg_addr, u32 bit_mask);

boolean config_phydm_write_rf_reg_8822c(struct dm_struct *dm, enum rf_path path,
					u32 reg_addr, u32 bit_mask, u32 data);

boolean phydm_write_txagc_1byte_8822c(struct dm_struct *dm, u32 pw_idx,
				      u8 hw_rate);

boolean config_phydm_write_txagc_ref_8822c(struct dm_struct *dm, u8 power_index,
					   enum rf_path path,
					   enum PDM_RATE_TYPE rate_type);

boolean config_phydm_write_txagc_diff_8822c(struct dm_struct *dm,
					    s8 power_index1, s8 power_index2,
					    s8 power_index3, s8 power_index4,
					    u8 hw_rate);

#ifdef CONFIG_TXAGC_DEBUG_8822C
void phydm_txagc_tab_buff_show_8822c(struct dm_struct *dm);
#endif

s8 config_phydm_read_txagc_diff_8822c(struct dm_struct *dm, u8 hw_rate);

u8 config_phydm_read_txagc_8822c(struct dm_struct *dm, enum rf_path path,
				 u8 hw_rate, enum PDM_RATE_TYPE rate_type);

void phydm_get_tx_path_en_setting_8822c(struct dm_struct *dm,
					struct tx_path_en_8822c *path);

void phydm_get_rx_path_en_setting_8822c(struct dm_struct *dm,
					struct rx_path_en_8822c *path);

void phydm_config_tx_path_8822c(struct dm_struct *dm, enum bb_path tx_path_2ss,
				enum bb_path tx_path_sel_1ss,
				enum bb_path tx_path_sel_cck);

boolean config_phydm_trx_mode_8822c(struct dm_struct *dm,
				    enum bb_path tx_path_en,
				    enum bb_path rx_path,
				    enum bb_path tx_path_sel_1ss);

boolean config_phydm_switch_band_8822c(struct dm_struct *dm, u8 central_ch);

boolean config_phydm_switch_channel_8822c(struct dm_struct *dm, u8 central_ch);

boolean config_phydm_switch_bandwidth_8822c(struct dm_struct *dm, u8 pri_ch,
					    enum channel_width bw);

boolean config_phydm_switch_channel_bw_8822c(struct dm_struct *dm,
					     u8 central_ch, u8 primary_ch_idx,
					     enum channel_width bandwidth);

void phydm_i_only_setting_8822c(struct dm_struct *dm, boolean en_i_only,
				boolean en_before_cca);

boolean phydm_1rcca_setting_8822c(struct dm_struct *dm, boolean en_1rcca);

void phydm_invld_pkt_setting_8822c(struct dm_struct *dm, boolean en_invld_pkt);

void phydm_cck_gi_bound_8822c(struct dm_struct *dm);

void phydm_ch_smooth_setting_8822c(struct dm_struct *dm, boolean en_ch_smooth);

u16 phydm_get_dis_dpd_by_rate_8822c(struct dm_struct *dm);

void phydm_set_auto_nbi_8822c(struct dm_struct *dm, boolean en_auto_nbi);

boolean config_phydm_parameter_init_8822c(struct dm_struct *dm,
					  enum odm_parameter_init type);

boolean phydm_chk_bb_state_idle_8822c(struct dm_struct *dm);

u16 phydm_get_gpio_setting_by_rfe_ctrl_8822c(struct dm_struct *dm);

#if CONFIG_POWERSAVING
boolean phydm_8822c_lps(struct dm_struct *dm, boolean enable_lps);
#endif /* #if CONFIG_POWERSAVING */

void config_phydm_set_txagc_to_hw_8822c(struct dm_struct *dm);

boolean config_phydm_write_txagc_8822c(struct dm_struct *dm, u32 power_index,
				       enum rf_path path, u8 hw_rate);

void phydm_set_txagc_by_table_8822c(struct dm_struct *dm,
				    struct txagc_table_8822c *tab);

void phydm_get_txagc_ref_and_diff_8822c(struct dm_struct *dm,
					u8 txagc_buff[2][NUM_RATE_AC_2SS],
					u16 length,
					struct txagc_table_8822c *tab);
#endif /* RTL8822C_SUPPORT */
#endif /*  __INC_PHYDM_API_H_8822C__ */

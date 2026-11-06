/******************************************************************************
 *
 * Copyright(c) 2007 - 2022 Realtek Corporation.
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
#ifndef _HAL_DFS_H_
#define _HAL_DFS_H_

#ifdef CONFIG_DFS_MASTER
struct rtw_dfs_t {
	enum phydm_dfs_region_domain region_domain;
	bool enable; /* set by core layer to enable/disable radar detection */
	bool under_cac; /* set by core layer to indicate CAC status */

	/*
	* set by core to specify detect range
	* sp_detect_range_hi = 0 means no specified range, whole range allowed
	* by HAL will enable radar detection
	*/
	u32 sp_detect_range_hi;
	u32 sp_detect_range_lo;

	bool radar_detect_enabled; /* if radar detection is enabled */
	bool cac_tx_paused; /* if tx paused by CAC */
	bool pending_domain_change; /* if there is domain change under process */

	bool is_radar_detectd; /* if radar is detected */
};

struct hal_com_data;

bool hal_is_radar_detect_enabled(struct hal_com_data *hal_data, u8 band_idx);

bool hal_is_under_cac(struct hal_com_data *hal_data, u8 band_idx);

bool hal_is_cac_tx_paused(struct hal_com_data *hal_data, u8 band_idx);

struct dfs_rd_ch_switch_ctx {
	bool rd_enabled;
	bool should_rd_en_on_new_ch;
	bool under_cac;
	bool cac_tx_paused;
};

void hal_dfs_rd_setting_before_ch_switch(struct hal_com_data *hal_data, u8 band_idx
	, enum band_type band, u8 ch, enum channel_width bw, enum chan_offset offset, struct dfs_rd_ch_switch_ctx *ctx);

void hal_dfs_rd_setting_after_ch_switch(struct hal_com_data *hal_data, u8 band_idx
	, enum band_type band, u8 ch, enum channel_width bw, enum chan_offset offset, struct dfs_rd_ch_switch_ctx *ctx);

int
rtw_hal_dfs_change_domain(struct hal_com_data *hal_data, enum phl_band_idx hw_band
	, enum phydm_dfs_region_domain domain);

int
rtw_hal_dfs_rd_enable_all_range(struct hal_com_data *hal_data, enum phl_band_idx hw_band);

int
rtw_hal_dfs_rd_enable_with_sp_chbw(struct hal_com_data *hal_data, enum phl_band_idx hw_band
	, bool cac, u8 sp_ch, enum channel_width sp_bw, enum chan_offset sp_offset);

int
rtw_hal_dfs_rd_enable_with_sp_freq_range(struct hal_com_data *hal_data, enum phl_band_idx hw_band
	, bool cac, u32 sp_freq_hi, u32 sp_freq_lo);

int
rtw_hal_dfs_rd_set_cac_status(struct hal_com_data *hal_data, enum phl_band_idx hw_band, bool cac);

int
rtw_hal_dfs_rd_disable(struct hal_com_data *hal_data, enum phl_band_idx hw_band);
#endif

#endif /* _HAL_DFS_H_ */


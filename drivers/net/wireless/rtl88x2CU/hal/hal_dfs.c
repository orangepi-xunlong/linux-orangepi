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
#define _HAL_DFS_C_

#include <drv_types.h>
#include <hal_data.h>

/* be careful about the sychronization with phydm */
#define PHYDM_DFS_DOMAIN_NUM PHYDM_DFS_DOMAIN_ETSI + 1
#define PHYDM_DFS_DOMAIN_IS_UNKNOWN(domain) ((domain) == PHYDM_DFS_DOMAIN_UNKNOWN || (domain) >= PHYDM_DFS_DOMAIN_NUM)

#ifdef CONFIG_DFS_MASTER
struct dfs_rd_ctl_param {
	/*
	* DFS region domain set from core
	* >=PHYDM_DFS_DOMAIN_NUM => not set (keep original)
	* < PHYDM_DFS_DOMAIN_NUM  => change domain (if needed) and ignore other parameters
	*/
	enum phydm_dfs_region_domain domain;

	/*
	* enable=true,  cac=1     => enable radar detect and is under CAC
	* enable=true,  cac=0     => enalbe radar detect, in-service monitoring
	* enable=true,  cac<0     => enable radar detect w/o changing CAC status
	* enable=false, cac=any => disable radar detect
	*/
	bool enable;

	/*
	* CAC status set from core
	* < 0: not set (keep original)
	*    0: not under CAC
	*    1: under CAC
	*/
	s8 cac;

	/*
	* configuration for specific radar detect range (5G band is implicit), the value of sp_ch:
	* < 0: not set (keep original)
	*    0: all detectable range
	* > 0: specific detect range (by ch, bw, offset)
	*/
	s16 sp_ch;
	enum channel_width sp_bw;
	enum chan_offset sp_offset;

	/*
	* configuration for specific radar detect range in freqency, valid when sp_ch < 0
	* 0: not set (keep original)
	*/
	u32 sp_freq_hi;
	u32 sp_freq_lo;
};

static bool hal_bchbw_in_radar_domain(enum band_type band, u8 ch
	, enum channel_width bw, enum chan_offset offset)
{
	return band == BAND_ON_5G
		&& ((ch >= 52 && ch <= 64) || (ch >= 100 && ch <= 144))
		;
}

static bool hal_radar_detect_range_specified(struct rtw_dfs_t *dfs_info)
{
	return dfs_info->sp_detect_range_hi != 0;
}

static bool hal_overlap_radar_detect_range(struct rtw_dfs_t *dfs_info
	, enum band_type band, u8 ch, enum channel_width bw, enum chan_offset offset)
{
	bool ret = false;
	u32 hi = 0, lo = 0;
	int i;

	if (!rtw_bchbw_to_freq_range(band, ch, bw, offset, &hi, &lo)) {
		rtw_warn_on(1);
		goto exit;
	}

	if (rtw_is_range_overlap(hi, lo, dfs_info->sp_detect_range_hi, dfs_info->sp_detect_range_lo))
		ret = true;

exit:
	return ret;
}

static bool hal_should_radar_detect_enable_by_ch(struct hal_com_data *hal_data, u8 band_idx,
	enum band_type band, u8 channel, enum channel_width bwmode, enum chan_offset offset)
{
	struct rtw_dfs_t *dfs_info = &hal_data->dfs_info;

	if (dfs_info->enable && !PHYDM_DFS_DOMAIN_IS_UNKNOWN(dfs_info->region_domain)
		&& hal_bchbw_in_radar_domain(band, channel, bwmode, offset)
	) {
		if (!hal_radar_detect_range_specified(dfs_info)
			|| hal_overlap_radar_detect_range(dfs_info, band, channel, bwmode, offset))
			return true;
	}

	return false;
}

static void hal_dfs_handle_pending_domain_change(struct hal_com_data *hal_data, u8 band_idx
	, enum band_type band, u8 ch, enum channel_width bw, enum chan_offset offset)
{
	struct rtw_dfs_t *dfs_info = &hal_data->dfs_info;

	if (dfs_info->pending_domain_change) {
		if (dfs_info->radar_detect_enabled)
			phydm_radar_detect_disable(&hal_data->odmpriv);
		odm_cmn_info_init(&hal_data->odmpriv, ODM_CMNINFO_DFS_REGION_DOMAIN
			, hal_data->dfs_info.region_domain);
		if (dfs_info->radar_detect_enabled)
			phydm_radar_detect_enable(&hal_data->odmpriv);

		dfs_info->pending_domain_change = false;
	}
}

static int
hal_radar_detect_switch(struct hal_com_data *hal_data, u8 band_idx, bool enable)
{
	struct rtw_dfs_t *dfs_info = &hal_data->dfs_info;

	if (enable)
		phydm_radar_detect_enable(&hal_data->odmpriv);
	else
		phydm_radar_detect_disable(&hal_data->odmpriv);

	dfs_info->radar_detect_enabled = enable;
	return _SUCCESS;
}

bool hal_is_radar_detect_enabled(struct hal_com_data *hal_data, u8 band_idx)
{
	return hal_data->dfs_info.radar_detect_enabled;
}

static void hal_set_under_cac(struct hal_com_data *hal_data, u8 band_idx, bool under)
{
	hal_data->dfs_info.under_cac = under;
}

bool hal_is_under_cac(struct hal_com_data *hal_data, u8 band_idx)
{
	return hal_data->dfs_info.under_cac;
}

static int
hal_cac_tx_pause_switch(struct hal_com_data *hal_data, u8 band_idx, bool enable)
{
	struct rtw_dfs_t *dfs_info = &hal_data->dfs_info;

	if (rtw_hal_tx_pause(hal_data->adapter, PAUSE_RSON_DFS_CAC, enable) == _SUCCESS) {
		dfs_info->cac_tx_paused = enable;
		return _SUCCESS;
	}
	return _FAIL;
}

bool hal_is_cac_tx_paused(struct hal_com_data *hal_data, u8 band_idx)
{
	return hal_data->dfs_info.cac_tx_paused;
}

void hal_dfs_rd_setting_before_ch_switch(struct hal_com_data *hal_data, u8 band_idx
	, enum band_type band, u8 ch, enum channel_width bw, enum chan_offset offset, struct dfs_rd_ch_switch_ctx *ctx)
{
	ctx->should_rd_en_on_new_ch = hal_should_radar_detect_enable_by_ch(hal_data
		, band_idx, band, ch, bw, offset);
	ctx->under_cac = hal_is_under_cac(hal_data, band_idx);
	ctx->cac_tx_paused = hal_is_cac_tx_paused(hal_data, band_idx);

	ctx->rd_enabled = hal_is_radar_detect_enabled(hal_data, band_idx);

	if (!ctx->should_rd_en_on_new_ch && ctx->rd_enabled) {
		/* turn off radar detect before channel setting (ex: leaving detection range) */
		int rst = hal_radar_detect_switch(hal_data, band_idx, false);

		if (rst == _SUCCESS)
			RTW_INFO("[DFS] new ch=%d,%u,%d,%d disable radar detect\n", band, ch, bw, offset);
		else
			RTW_ERR("[DFS] new ch=%d,%u,%d,%d disable radar detect failed\n", band, ch, bw, offset);

		ctx->rd_enabled = hal_is_radar_detect_enabled(hal_data, band_idx);
	}

	if (ctx->should_rd_en_on_new_ch && ctx->under_cac && !ctx->cac_tx_paused) {
		/* turn on CAC tx pause before channel setting (ex: entering detection range) */
		int rst = hal_cac_tx_pause_switch(hal_data, band_idx, true);

		if (rst == _SUCCESS)
			RTW_INFO("[DFS] new ch=%d,%u,%d,%d enable CAC tx pause\n", band, ch, bw, offset);
		else
			RTW_ERR("[DFS] new ch=%d,%u,%d,%d enable CAC tx pause failed\n", band, ch, bw, offset);
	}
}

void hal_dfs_rd_setting_after_ch_switch(struct hal_com_data *hal_data, u8 band_idx
	, enum band_type band, u8 ch, enum channel_width bw, enum chan_offset offset, struct dfs_rd_ch_switch_ctx *ctx)
{
	if (ctx->should_rd_en_on_new_ch && !ctx->rd_enabled) {
		/* turn on radar detect after channel setting (ex: entered radar detect range) */
		int rst = hal_radar_detect_switch(hal_data, band_idx, true);

		if (rst == _SUCCESS)
			RTW_INFO("[DFS] new ch=%d,%u,%d,%d enable radar detect\n", band, ch, bw, offset);
		else
			RTW_ERR("[DFS] new ch=%d,%u,%d,%d enable radar detect failed\n", band, ch, bw, offset);
	}

	if ((!ctx->should_rd_en_on_new_ch || !ctx->under_cac) && ctx->cac_tx_paused) {
		/* turn off CAC tx pause after channel setting (ex: leaved detection range) */
		int rst = hal_cac_tx_pause_switch(hal_data, band_idx, false);

		if (rst == _SUCCESS)
			RTW_INFO("[DFS] new ch=%d,%u,%d,%d disable CAC tx pause\n", band, ch, bw, offset);
		else
			RTW_ERR("[DFS] new ch=%d,%u,%d,%d disable CAC tx pause failed\n", band, ch, bw, offset);
	}
}

static int
hal_radar_detect_confs_apply(struct hal_com_data *hal_data, u8 band_idx)
{
	struct rtw_dfs_t *dfs_info = &hal_data->dfs_info;
	int rd_rst = _SUCCESS;
	int cac_rst = _SUCCESS;
	u8 band, ch, bw, offset;
	bool should_rd_enable;
	bool under_cac;
	bool cac_tx_paused;

	ch = rtw_get_oper_ch(hal_data->adapter);
	bw = rtw_get_oper_bw(hal_data->adapter);
	offset = rtw_get_oper_choffset(hal_data->adapter);
	band = rtw_is_2g_ch(ch) ? BAND_ON_24G : BAND_ON_5G;

	hal_dfs_handle_pending_domain_change(hal_data, band_idx
		, band, ch, bw, offset);

	should_rd_enable = hal_should_radar_detect_enable_by_ch(hal_data, band_idx
		, band, ch, bw, offset);
	under_cac = hal_is_under_cac(hal_data, band_idx);
	cac_tx_paused = hal_is_cac_tx_paused(hal_data, band_idx);

	if (!dfs_info->radar_detect_enabled) {
		if (should_rd_enable) {
			rd_rst = hal_radar_detect_switch(hal_data, band_idx, true);
			if (rd_rst == _SUCCESS)
				RTW_INFO("[DFS] ch=%d,%u,%d,%d enable radar detect\n", band, ch, bw, offset);
			else
				RTW_ERR("[DFS] ch=%d,%u,%d,%d enable radar detect failed\n", band, ch, bw, offset);
		}

	} else if (dfs_info->radar_detect_enabled) {
		if (!should_rd_enable) {
			rd_rst = hal_radar_detect_switch(hal_data, band_idx, false);
			if (rd_rst == _SUCCESS)
				RTW_INFO("[DFS] ch=%d,%u,%d,%d disable radar detect\n", band, ch, bw, offset);
			else
				RTW_ERR("[DFS] ch=%d,%u,%d,%d disable radar detect failed\n", band, ch, bw, offset);
		}
	}

	if (!cac_tx_paused) {
		if (under_cac && dfs_info->radar_detect_enabled) {
			cac_rst = hal_cac_tx_pause_switch(hal_data, band_idx, true);
			if (cac_rst ==  _SUCCESS)
				RTW_INFO("[DFS] ch=%d,%u,%d,%d enable CAC tx pause\n", band, ch, bw, offset);
			else
				RTW_INFO("[DFS] ch=%d,%u,%d,%d enable CAC tx pause failed\n", band, ch, bw, offset);
		}

	} else 	if (cac_tx_paused) {
		if (!under_cac) {
			/*
			* Release CAC tx pause only when not under CAC
			* Keep CAC tx pause when under CAC and radar detect is turned off by
			* specifying new detect range which doesn't overlap current channel setting
			* (ex: operating channel switching to new DFS channel)
			*/
			cac_rst = hal_cac_tx_pause_switch(hal_data, band_idx, false);
			if (cac_rst ==  _SUCCESS)
				RTW_INFO("[DFS] ch=%d,%u,%d,%d disable CAC tx pause\n", band, ch, bw, offset);
			else
				RTW_INFO("[DFS] ch=%d,%u,%d,%d disable CAC tx pause failed\n", band, ch, bw, offset);
		}
	}

	if (rd_rst == _SUCCESS && cac_rst == _SUCCESS)
		return _SUCCESS;
	return _FAIL;
}

static int
hal_dfs_rd_ctl_hdl(struct hal_com_data *hal_data, struct dfs_rd_ctl_param *rd_ctl_param)
{
	struct rtw_dfs_t *dfs_info = &hal_data->dfs_info;
	u8 band_idx = HW_BAND_0; /* multi band/phy capable? */

	if (rd_ctl_param->domain < PHYDM_DFS_DOMAIN_NUM) {
		if (rd_ctl_param->domain != dfs_info->region_domain) {
			RTW_INFO("%s set domain to %d\n", __func__, rd_ctl_param->domain);
			dfs_info->region_domain = rd_ctl_param->domain;
			dfs_info->pending_domain_change = true;
		}
		goto apply;
	}

	if (rd_ctl_param->enable) {
		if (!dfs_info->enable) {
			RTW_INFO("%s enable\n", __func__);
			dfs_info->enable = true;
		}
		if (rd_ctl_param->cac == 1) {
			if (!hal_is_under_cac(hal_data, band_idx)) {
				RTW_INFO("%s under CAC\n", __func__);
				hal_set_under_cac(hal_data, band_idx, true);
			}
		} else if (rd_ctl_param->cac == 0) {
			if (hal_is_under_cac(hal_data, band_idx)) {
				RTW_INFO("%s CAC done\n", __func__);
				hal_set_under_cac(hal_data, band_idx, false);
			}
		}

	} else {
		if (dfs_info->enable) {
			RTW_INFO("%s disable\n", __func__);
			dfs_info->enable = false;
		}
		hal_set_under_cac(hal_data, band_idx, false);
	}

	if (rd_ctl_param->sp_ch > 0) {
		u32 hi, lo;

		if (rtw_bchbw_to_freq_range(BAND_ON_5G
				, rd_ctl_param->sp_ch, rd_ctl_param->sp_bw, rd_ctl_param->sp_offset
				, &hi, &lo)
		) {
			if (dfs_info->sp_detect_range_hi != hi || dfs_info->sp_detect_range_lo != lo) {
				RTW_INFO("%s sp_ch:%u,%d,%d is set\n", __func__
					, rd_ctl_param->sp_ch, rd_ctl_param->sp_bw, rd_ctl_param->sp_offset);
				dfs_info->sp_detect_range_hi = hi;
				dfs_info->sp_detect_range_lo = lo;
			}
		} else {
			RTW_WARN("%s sp_ch:%u,%d,%d to freq range fail, all range applied\n", __func__
				, rd_ctl_param->sp_ch, rd_ctl_param->sp_bw, rd_ctl_param->sp_offset);
			dfs_info->sp_detect_range_hi = 0;
		}
	} else if (rd_ctl_param->sp_ch == 0) {
		if (dfs_info->sp_detect_range_hi != 0) {
			RTW_INFO("%s all range applied\n", __func__);
			dfs_info->sp_detect_range_hi = 0;
		}
	} else if (rd_ctl_param->sp_freq_hi) {
		if (rd_ctl_param->sp_freq_hi <= rd_ctl_param->sp_freq_lo) {
			RTW_WARN("%s sp_freq_hi:%u <= sp_freq_lo:%u, all range applied\n", __func__
				, rd_ctl_param->sp_freq_hi, rd_ctl_param->sp_freq_lo);
		} else {
			if (dfs_info->sp_detect_range_hi != rd_ctl_param->sp_freq_hi
				|| dfs_info->sp_detect_range_lo != rd_ctl_param->sp_freq_lo
			) {
				RTW_INFO("%s sp_freq %u to %u is set\n", __func__
					, rd_ctl_param->sp_freq_lo, rd_ctl_param->sp_freq_hi);
				dfs_info->sp_detect_range_hi = rd_ctl_param->sp_freq_hi;
				dfs_info->sp_detect_range_lo = rd_ctl_param->sp_freq_lo;
			}
		}
	}

apply:
	/* apply new configs on cur channel */
	return hal_radar_detect_confs_apply(hal_data, band_idx);
}

static int
_rtw_hal_dfs_rd_ctl(struct hal_com_data *hal_data, enum phl_band_idx hw_band
	, enum phydm_dfs_region_domain domain, bool enable, s8 cac, s16 sp_ch, enum channel_width sp_bw, enum chan_offset sp_offset
	, u32 sp_freq_hi, u32 sp_freq_lo)
{
	int ret;
	struct dfs_rd_ctl_param param;

	param.domain = domain;
	param.enable = enable;
	param.cac = cac;
	param.sp_ch = sp_ch;
	param.sp_bw = sp_bw;
	param.sp_offset = sp_offset;
	param.sp_freq_hi = sp_freq_hi;
	param.sp_freq_lo = sp_freq_lo;

	ret = hal_dfs_rd_ctl_hdl(hal_data, &param);

	return ret;
}

int
rtw_hal_dfs_change_domain(struct hal_com_data *hal_data, enum phl_band_idx hw_band
	, enum phydm_dfs_region_domain domain)
{
	if (domain >= PHYDM_DFS_DOMAIN_NUM) {
		RTW_WARN("%s(), invalid domain:%d\n", __func__, domain);
		return _FAIL;
	}

	return _rtw_hal_dfs_rd_ctl(hal_data, hw_band
		, domain /* change domain, other parameters will be ignored */
		, false, 0, -1, 0, 0, 0, 0);
}

int
rtw_hal_dfs_rd_enable_all_range(struct hal_com_data *hal_data, enum phl_band_idx hw_band)
{
	return _rtw_hal_dfs_rd_ctl(hal_data, hw_band
		, PHYDM_DFS_DOMAIN_NUM
		, true, -1 /* enable radar detect w/o changing CAC status */
		, 0, 0, 0, 0, 0);
}

int
rtw_hal_dfs_rd_enable_with_sp_chbw(struct hal_com_data *hal_data, enum phl_band_idx hw_band
	, bool cac, u8 sp_ch, enum channel_width sp_bw, enum chan_offset sp_offset)
{
	return _rtw_hal_dfs_rd_ctl(hal_data, hw_band
		, PHYDM_DFS_DOMAIN_NUM
		, true, cac ? 1 : 0, sp_ch, sp_bw, sp_offset, 0, 0);
}

int
rtw_hal_dfs_rd_enable_with_sp_freq_range(struct hal_com_data *hal_data, enum phl_band_idx hw_band
	, bool cac, u32 sp_freq_hi, u32 sp_freq_lo)
{
	return _rtw_hal_dfs_rd_ctl(hal_data, hw_band
		, PHYDM_DFS_DOMAIN_NUM
		, true, cac ? 1 : 0, -1, 0, 0, sp_freq_hi, sp_freq_lo);
}

int
rtw_hal_dfs_rd_set_cac_status(struct hal_com_data *hal_data, enum phl_band_idx hw_band, bool cac)
{
	return _rtw_hal_dfs_rd_ctl(hal_data, hw_band
		, PHYDM_DFS_DOMAIN_NUM
		, true /* CAC status only valid when radar detect enable */
		, cac ? 1 : 0, -1, 0, 0, 0, 0);
}

int
rtw_hal_dfs_rd_disable(struct hal_com_data *hal_data, enum phl_band_idx hw_band)
{
	return _rtw_hal_dfs_rd_ctl(hal_data, hw_band
		, PHYDM_DFS_DOMAIN_NUM
		, false, 0, -1, 0, 0, 0, 0);
}
#endif

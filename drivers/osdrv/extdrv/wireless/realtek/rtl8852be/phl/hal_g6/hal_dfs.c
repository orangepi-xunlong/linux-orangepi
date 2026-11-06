/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
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
#include "hal_headers.h"

#ifdef CONFIG_PHL_DFS

enum rtw_hal_status
rtw_hal_radar_detect_cfg(void *hal, u8 band_idx, bool dfs_enable)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	PHL_INFO("====>%s dfs_en:%d ============\n", __func__, dfs_enable);

	rtw_hal_bb_dfs_rpt_cfg(hal_info->hal_com, rtw_hal_hw_band_to_phy_idx(band_idx), dfs_enable);

	return RTW_HAL_STATUS_SUCCESS;
}

bool rtw_hal_is_radar_detect_enabled(struct rtw_hal_com_t *hal_com, enum phl_phy_idx phy_idx)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal_com->hal_priv;

	return rtw_phl_is_radar_detect_enabled(hal_info->phl_com, rtw_hal_phy_idx_to_hw_band(phy_idx));
}

bool rtw_hal_is_under_cac(struct rtw_hal_com_t *hal_com, enum phl_phy_idx phy_idx)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal_com->hal_priv;

	return rtw_phl_is_under_cac(hal_info->phl_com, rtw_hal_phy_idx_to_hw_band(phy_idx));
}
#endif


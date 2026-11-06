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
#define _HAL_API_C_
#include "hal_headers.h"

enum rtw_hal_status rtw_hal_get_tsf(void *hal, enum phl_band_idx band,
					u8 port, u32 *tsf_h, u32 *tsf_l)
{
	return rtw_hal_mac_get_tsf(hal, band, port, tsf_h, tsf_l);
}

enum rtw_hal_status
rtw_hal_tsf_sync(void *hal, u8 port_sync_from,
					u8 port_sync_to, enum phl_band_idx band,
					s32 sync_offset_tu, enum hal_tsf_sync_act act)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	return rtw_hal_mac_tsf_sync(hal_info, port_sync_from,
						port_sync_to, band, sync_offset_tu, act);
}

enum rtw_hal_status
rtw_hal_config_rts_th(void *hal, u8 band_idx, u16 rts_time_th, u16 rts_len_th)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	return rtw_hal_mac_set_hw_rts_th(hal_info, band_idx, rts_time_th, rts_len_th);
}

u32 rtw_hal_get_btc_req_slot(void *hal, enum phl_band_idx hw_band)
{
	return rtw_hal_btc_req_bt_slot_t(hal, hw_band);
}

/**
 * This function is used to pause/unpause multiple macid
 * @hal: see hal_info_t
 * @macid_arr: macid array to be pause/unpause
 * 1 means to be set (pause/unpause)
 * 0 means don't care
 * @macid_arr_sz: size of macid array
 * @pause: all macid of this array to be paused/unpaused
 *  1=paused,0=unpaused
 */
enum rtw_hal_status
rtw_hal_set_macid_grp_pause(void *hal, u32 *macid_arr, u8 arr_size, bool pause)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;

	return rtw_hal_mac_set_macid_grp_pause(hal_com, macid_arr, arr_size, pause);
}

/**
 * This function is used to pause/unpause single macid
 * @hal: see hal_info_t
 * @macid: macid be pause/unpause
 * @pause: all macid of this array to be paused/unpaused
 *  1=paused,0=unpaused
 */
enum rtw_hal_status
rtw_hal_set_macid_pause(void *hal, u16 macid, bool pause)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;

	return rtw_hal_mac_set_macid_pause(hal_com, macid, pause);
}

/**
 * This function is used to pause/unpause AC queue of a single macid
 * @hal: see hal_info_t
 * @macid: macid be pause/unpause
 * @pause: paused/unpaused (1=paused,0=unpaused)
 */
enum rtw_hal_status rtw_hal_set_macid_pause_ac(void *hal, u16 macid, bool pause)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;

	return rtw_hal_mac_set_macid_pause_sleep(hal_com, macid, pause, false);
}

enum rtw_hal_status
rtw_hal_set_macid_pkt_drop(void *hal, u16 macid, u8 sel, u8 band, u8 port,
                           u8 mbssid)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;

	return rtw_hal_mac_set_macid_pkt_drop(hal_com, macid, sel, band, port,
	                                      mbssid);
}

enum rtw_hal_status rtw_hal_set_dfs_tb_ctrl(void *hal, u8 set)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	return rtw_hal_mac_set_dfs_tb_ctrl(hal_info, set);
}

u32 rtw_hal_get_phy_stat_info(void *hal, enum phl_band_idx hw_band,
			      enum phl_stat_info_query phy_stat)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;
	struct rtw_hal_stat_info *stat_info = &hal_com->band[hw_band].stat_info;

	switch (phy_stat) {
	case STAT_INFO_FA_ALL:
		return stat_info->cnt_fail_all;
	case STAT_INFO_CCA_ALL:
		return stat_info->cnt_cca_all;
	default:
		return 0;
	}
}

enum rtw_hal_status rtw_hal_get_rx_cnt_by_idx(void *hal,
					      enum phl_band_idx hw_band,
					      enum phl_rxcnt_idx idx,
					      u16 *rx_cnt)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	return rtw_hal_mac_get_rx_cnt_by_idx(hal_info, hw_band, idx, rx_cnt);
}

enum rtw_hal_status rtw_hal_set_reset_rx_cnt(void *hal,
					     enum phl_band_idx hw_band)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	return rtw_hal_mac_set_reset_rx_cnt(hal_info, hw_band);
}

enum rtw_hal_status
rtw_hal_set_dctrl_tbl_seq(void *hal,
		struct rtw_phl_stainfo_t *sta, u32 seq)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;
	enum rtw_hal_status sts = RTW_HAL_STATUS_FAILURE;
	struct mac_ax_dctl_info dctrl = {0}, dctrl_mask = {0};

	if (NULL == sta)
		goto out;

	dctrl.seq0 = seq;
	dctrl.seq1 = seq;
	dctrl.seq2 = seq;
	dctrl.seq3 = seq;

	dctrl_mask.seq0 = 0x0FFF;
	dctrl_mask.seq1 = 0x0FFF;
	dctrl_mask.seq2 = 0x0FFF;
	dctrl_mask.seq3 = 0x0FFF;

	sts = rtw_hal_dmc_tbl_cfg(hal_info, &dctrl ,&dctrl_mask, sta->macid);

out:
	return sts;
}

#ifdef CONFIG_PHL_DRV_HAS_NVM
enum rtw_hal_status
rtw_hal_extract_efuse_info(void *hal, u8 *efuse_map,
                           enum rtw_efuse_info info_type,
                           void *value,
                           u8 size, u8 map_valid)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;
	enum rtw_phl_status hal_status = RTW_HAL_STATUS_FAILURE;

	if (info_type <= EFUSE_INFO_MAC_MAX)
		hal_status = rtw_hal_mac_get_efuse_info(hal_info->hal_com,
							efuse_map,
							info_type,
							value,
							size,
							1);
	else if (info_type <= EFUSE_INFO_BB_MAX)
		hal_status = rtw_hal_bb_get_efuse_info(hal_info->hal_com,
		                                       efuse_map,
		                                       info_type,
		                                       value,
		                                       size,
		                                       1);
	else if (info_type <= EFUSE_INFO_RF_MAX)
		hal_status = rtw_hal_rf_get_efuse_info(hal_info->hal_com,
		                                       efuse_map,
		                                       info_type,
		                                       value,
		                                       size,
		                                       1);
	else if (info_type <= EFUSE_INFO_BTCOEX_MAX)
		hal_status = rtw_hal_btc_get_efuse_info(hal_info->hal_com,
							efuse_map,
							info_type,
							value,
							size,
							1);
	return hal_status;
}

enum rtw_hal_status
rtw_hal_get_efuse_size(void *hal, u32 *log_efuse_size, u32 *limit_efuse_size,
                       u32 *mask_size, u32 *limit_mask_size)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_SUCCESS;
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	if (log_efuse_size)
		hal_status = rtw_hal_mac_get_log_efuse_size(hal_info->hal_com,
		                                            log_efuse_size,
		                                            false);
	if (limit_efuse_size && (hal_status == RTW_HAL_STATUS_SUCCESS))
		hal_status = rtw_hal_mac_get_log_efuse_size(hal_info->hal_com,
		                                            limit_efuse_size,
		                                            true);
	if (mask_size && (hal_status == RTW_HAL_STATUS_SUCCESS))
		hal_status = rtw_hal_mac_get_efuse_mask_size(hal_info->hal_com,
		                                             mask_size,
		                                             false);
	if (limit_mask_size  && (hal_status == RTW_HAL_STATUS_SUCCESS))
		hal_status = rtw_hal_mac_get_efuse_mask_size(hal_info->hal_com,
		                                             limit_mask_size,
		                                             true);
	return hal_status;
}

enum rtw_hal_status
rtw_hal_nvm_apply_dev_cap(void *hal, struct rtw_phl_com_t *phl_com)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;
	enum rtw_hal_status hal_sts;

	hal_com->dev_hw_cap.pkg_type = phl_com->dev_sw_cap.pkg_type;
	hal_com->dev_hw_cap.rfe_type = phl_com->dev_sw_cap.rfe_type;
	hal_com->dev_hw_cap.xcap = phl_com->dev_sw_cap.xcap;
	hal_com->dev_hw_cap.domain = phl_com->dev_sw_cap.domain;

	hal_sts = rtw_hal_mac_set_xcap(hal_com, 0, hal_com->dev_hw_cap.xcap);

	if (hal_sts != RTW_HAL_STATUS_SUCCESS)
		return RTW_HAL_STATUS_FAILURE;

	hal_sts = rtw_hal_mac_set_xcap(hal_com, 1, hal_com->dev_hw_cap.xcap);

	#ifdef CONFIG_RTW_MULTI_DEV_MULTI_BAND
	if (hal_sts == RTW_HAL_STATUS_SUCCESS) {
		struct hal_ops_t *hal_ops = hal_get_ops(hal_info);

		if (hal_ops->cfg_share_xstal != NULL) {
			#ifdef CONFIG_SHARE_XSTAL
			hal_sts = hal_ops->cfg_share_xstal(hal_info, phl_com,
							    true);
			#else
			hal_sts = hal_ops->cfg_share_xstal(hal_info, phl_com,
							    false);
			#endif /* CONFIG_SHARE_XSTAL */
		}
	}
	#endif /* CONFIG_RTW_MULTI_DEV_MULTI_BAND */

	return hal_sts;
}

enum rtw_hal_status
rtw_hal_flash_get_info(struct rtw_hal_com_t *hal_com,
		       enum rtw_efuse_info info_type,
		       void *value,
		       u8 size)
{
	u32 ret = _os_nvm_get_info(hal_com->drv_priv, info_type, value, size);

	return (ret == _SUCCESS) ? RTW_HAL_STATUS_SUCCESS : RTW_HAL_STATUS_FAILURE;
}

#endif /* CONFIG_PHL_DRV_HAS_NVM */

enum rtw_hal_status
rtw_hal_set_fw_ul_fixinfo(void *hal,
			  struct rtw_phl_ax_ul_fixinfo *ul_fixinfo)
{
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;

	hstatus = rtw_hal_mac_set_fw_ul_fixinfo(hal, ul_fixinfo);

	return hstatus;
}

u16 rtw_hal_get_ampdu_num(void *hal, u8 band)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	return rtw_hal_mac_query_ampdu_num(hal_info, band);
}

enum rtw_chip_id rtw_hal_get_chip_id(void *hal)
{
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	return hal_get_chip_id(hal_info->hal_com);
}

enum rtw_hal_status rtw_hal_set_append_fcs(void *hal, u8 enable)
{
	return rtw_hal_mac_set_append_fcs((struct hal_info_t *)hal, enable);
}

enum rtw_hal_status
rtw_hal_pwr_switch_mac(void *hal, bool on)
{
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;
	hstatus = rtw_hal_mac_pwr_switch(hal, on);
	return hstatus;
}

enum rtw_hal_status rtw_hal_get_freerun_counter(void *hal, u8 band, u32 *freerun_h, u32 *freerun_l)
{
	enum rtw_hal_status status = RTW_HAL_STATUS_FAILURE;
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	status = rtw_hal_mac_get_freerun_cnt(hal_info->hal_com, band, freerun_h, freerun_l);

	if (status != RTW_HAL_STATUS_SUCCESS) {
		PHL_TRACE(COMP_PHL_MCC, _PHL_ERR_, "rtw_hal_get_freerun_counter(): get failed\n");
	}

	return status;
}

enum rtw_hal_status rtw_hal_reset_freerun_counter(void *hal, u8 band)
{
	enum rtw_hal_status status = RTW_HAL_STATUS_FAILURE;
	struct hal_info_t *hal_info = (struct hal_info_t *)hal;

	status = rtw_hal_mac_reset_freerun_cnt(hal_info->hal_com, band);

	if (status != RTW_HAL_STATUS_SUCCESS) {
		PHL_TRACE(COMP_PHL_MCC, _PHL_ERR_, "rtw_hal_reset_freerun_counter(): get failed\n");
	}

	return status;
}

u8 rtw_hal_query_group_cntry_num(void *hal, struct rtw_regu_policy *policy,
	u8 group_id)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal || !policy)
		return 0;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return 0;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_query_group_cntry_num)
		return regu_ops->hal_query_group_cntry_num(policy, group_id);

	return 0;
}

void rtw_hal_fill_group_cntry_list(void *hal,
	struct rtw_regu_policy *policy, char *list,
	u32 group_size, u8 group_id)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal || !policy)
		return;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_fill_group_cntry_list)
		regu_ops->hal_fill_group_cntry_list(policy, list,
		group_size, group_id);
}

u8 rtw_hal_get_cntry_idx(void *hal, char *cntry)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return INVALID_CNTRY_IDX;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return INVALID_CNTRY_IDX;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_cntry_idx)
		return regu_ops->hal_get_cntry_idx(cntry);

	return INVALID_CNTRY_IDX;
}

u8 rtw_hal_get_cntry_tbl_size(void *hal)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return 0;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return 0;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_cntry_tbl_size)
		return regu_ops->hal_get_cntry_tbl_size();

	return 0;
}

void rtw_hal_qry_cntry_chnlplan(
	void *hal, struct rtw_regulation_country_chplan *chplan,
	char *country)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_qry_cntry_chnlplan)
		regu_ops->hal_qry_cntry_chnlplan(chplan, country);
}

void rtw_hal_get_chplan_update_info(
	void *hal, u8 group, u8 did,
	void *info, enum band_type band)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_chplan_update_info)
		regu_ops->hal_get_chplan_update_info(group, did, info, band);
}

u8 rtw_hal_get_chnlplan_ver(void *hal)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return INVALID_VER;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return INVALID_VER;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_chnlplan_ver)
		return regu_ops->hal_get_chnlplan_ver();

	return INVALID_VER;
}

u8 rtw_hal_get_country_ver(void *hal)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return INVALID_VER;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return INVALID_VER;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_country_ver)
		return regu_ops->hal_get_country_ver();

	return INVALID_VER;
}

void rtw_hal_get_chdef_6g(void * hal, u8 ch_idx,
	struct chdef_6ghz *chdef)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_chdef_6g)
		regu_ops->hal_get_chdef_6g(ch_idx, chdef);
}

u8 rtw_hal_get_domain_regulation(void *hal,
	u8 domain, u8 band)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return REGULATION_MAX;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return REGULATION_MAX;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_domain_regulation)
		return regu_ops->hal_get_domain_regulation(domain, band);

	return REGULATION_MAX;
}

u8 rtw_hal_get_domain_idx(void *hal,
	u8 domain, bool is_6g)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return INVALID_DOMAIN_INDEX;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return INVALID_DOMAIN_INDEX;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_domain_idx)
		return regu_ops->hal_get_domain_idx(domain, is_6g);

	return INVALID_DOMAIN_INDEX;
}

u8 rtw_hal_get_cat6g_by_country(
	void *hal, char * country)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return 0;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return 0;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_cat6g_by_country)
		return regu_ops->hal_get_cat6g_by_country(country);

	return 0;
}

void rtw_hal_get_6g_regulatory_info(void *hal, u8 domain,
	u8 *dm_code, u8 *regulation, u8 *ch_idx)
{
	struct hal_info_t *hal_info = NULL;
	struct hal_ops_t *hal_ops = NULL;
	struct hal_regu_ops *regu_ops = NULL;

	if (!hal)
		return;

	hal_info = (struct hal_info_t *)hal;
	hal_ops = hal_get_ops(hal_info);
	if (!hal_ops)
		return;

	regu_ops = hal_get_regu_ops(hal_ops);
	if (regu_ops && regu_ops->hal_get_6g_regulatory_info)
		regu_ops->hal_get_6g_regulatory_info(domain,
			dm_code, regulation, ch_idx);
}

enum rtw_hal_status
rtw_hal_set_bcn_early_rpt(void *hal, u8 band, u8 port, u8 en)
{
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;

	hstatus = rtw_hal_mac_cfg_bcn_early_rpt(hal, band, port, en);

	return hstatus;
}

#ifdef CONFIG_PHL_CSUM_OFFLOAD_RX
enum rtw_hal_status rtw_hal_chk_rx_tcpip_chksum_ofd(void *h,
						    struct rtw_r_meta_data *mdata,
                                                    u8 status)
{
	struct hal_info_t *hal = (struct hal_info_t *)h;

	return rtw_hal_mac_chk_rx_tcpip_chksum_ofd(hal, mdata, status);
}
#endif /* CONFIG_PHL_CSUM_OFFLOAD_RX */

/******************************************************************************
 *
 * Copyright(c) 2007 - 2023 Realtek Corporation.
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
#define _RTW_CFG_C_

#include <drv_types.h>

#ifdef CONFIG_AP_MODE
uint rtw_max_ap_assoc_sta = CONFIG_RTW_MAX_AP_ASSOC_STA;
module_param(rtw_max_ap_assoc_sta, uint, 0644);
MODULE_PARM_DESC(rtw_max_ap_assoc_sta, "the maximum number of associated STAs of AP mode, 0: not specified");
#endif /* CONFIG_AP_MODE */

#ifdef CONFIG_REGD_SRC_FROM_OS
static uint rtw_regd_src = CONFIG_RTW_REGD_SRC;
module_param(rtw_regd_src, uint, 0644);
MODULE_PARM_DESC(rtw_regd_src, "The default regd source selection, 0:RTK_PRIV, 1:OS");
#endif

uint rtw_init_regd_always_apply = CONFIG_RTW_INIT_REGD_ALWAYS_APPLY;
module_param(rtw_init_regd_always_apply, uint, 0644);
MODULE_PARM_DESC(rtw_init_regd_always_apply, "Whether INIT regd request is always applied"
	" (being included when taking intersection together with higher priority requests)"
	" when regd source is RTK_PRIV");

uint rtw_user_regd_always_apply = CONFIG_RTW_USER_REGD_ALWAYS_APPLY;
module_param(rtw_user_regd_always_apply, uint, 0644);
MODULE_PARM_DESC(rtw_user_regd_always_apply, "Whether USER regd request is always applied"
	" (being included when taking intersection together with higher priority requests)"
	" when regd source is RTK_PRIV");

char *rtw_country_code = CONFIG_RTW_COUNTRY_CODE;
module_param(rtw_country_code, charp, 0644);
MODULE_PARM_DESC(rtw_country_code, "The default country code (in alpha2)");

uint rtw_channel_plan = CONFIG_RTW_CHPLAN;
module_param(rtw_channel_plan, uint, 0644);
MODULE_PARM_DESC(rtw_channel_plan, "The default chplan ID when rtw_alpha2 is not specified or valid");

static uint rtw_excl_chs[MAX_CHANNEL_NUM_2G_5G] = CONFIG_RTW_EXCL_CHS;
static int rtw_excl_chs_num = 0;
module_param_array(rtw_excl_chs, uint, &rtw_excl_chs_num, 0644);
MODULE_PARM_DESC(rtw_excl_chs, "Exclusive channel list of 2G and 5G band");

#if CONFIG_IEEE80211_BAND_6GHZ
uint rtw_channel_plan_6g = CONFIG_RTW_CHPLAN_6G;
module_param(rtw_channel_plan_6g, uint, 0644);
MODULE_PARM_DESC(rtw_channel_plan_6g, "The default chplan_6g ID when rtw_alpha2 is not specified or valid");

static uint rtw_excl_chs_6g[MAX_CHANNEL_NUM_6G] = CONFIG_RTW_EXCL_CHS_6G;
static int rtw_excl_chs_6g_num = 0;
module_param_array(rtw_excl_chs_6g, uint, &rtw_excl_chs_6g_num, 0644);
MODULE_PARM_DESC(rtw_excl_chs_6g, "Exclusive channel list of 6G band");
#endif /* CONFIG_IEEE80211_BAND_6GHZ */

char *rtw_dis_ch_flags = CONFIG_RTW_DIS_CH_FLAGS;
module_param(rtw_dis_ch_flags, charp, 0644);
MODULE_PARM_DESC(rtw_dis_ch_flags, "The flags with which channel is to be disabled");

static uint rtw_bcn_hint_valid_ms = CONFIG_RTW_BCN_HINT_VALID_MS;
module_param(rtw_bcn_hint_valid_ms, uint, 0644);
MODULE_PARM_DESC(rtw_bcn_hint_valid_ms, "The length of time beacon hint continue");

#ifdef CONFIG_80211D
static uint rtw_country_ie_slave_en_mode = CONFIG_RTW_COUNTRY_IE_SLAVE_EN_MODE;
module_param(rtw_country_ie_slave_en_mode, uint, 0644);
MODULE_PARM_DESC(rtw_country_ie_slave_en_mode, "802.11d country IE slave enable mode:"
	" 0: disable, 1: enable, 2: enable when INIT/USER set world wide mode");

static uint rtw_country_ie_slave_flags = CONFIG_RTW_COUNTRY_IE_SLAVE_FLAGS;
module_param(rtw_country_ie_slave_flags, uint, 0644);
MODULE_PARM_DESC(rtw_country_ie_slave_flags, "802.11d country IE slave flags:"
	" BIT0: take intersection when having multiple received IEs, otherwise choose effected one from received IEs"
	", BIT1: consider all environment BSSs, otherwise associated BSSs only");

static uint rtw_country_ie_slave_en_role = CONFIG_RTW_COUNTRY_IE_SLAVE_EN_ROLE;
module_param(rtw_country_ie_slave_en_role, uint, 0644);
MODULE_PARM_DESC(rtw_country_ie_slave_en_role, "802.11d country IE slave enable role: BIT0:pure STA mode, BIT1:P2P group client");

static uint rtw_country_ie_slave_en_ifbmp = CONFIG_RTW_COUNTRY_IE_SLAVE_EN_IFBMP;
module_param(rtw_country_ie_slave_en_ifbmp, uint, 0644);
MODULE_PARM_DESC(rtw_country_ie_slave_en_ifbmp, "802.11d country IE slave enable iface bitmap");

static uint rtw_country_ie_slave_scan_int_ms = CONFIG_RTW_COUNTRY_IE_SLAVE_SCAN_INT_MS;
module_param(rtw_country_ie_slave_scan_int_ms, uint, 0644);
MODULE_PARM_DESC(rtw_country_ie_slave_scan_int_ms, "802.11d country IE slave auto scan interval in ms to find environment BSSs."
	" 0: no environment BSS auto scan triggered by driver self");
#endif

uint rtw_edcca_mode_sel = CONFIG_RTW_EDCCA_MODE_SEL;
module_param(rtw_edcca_mode_sel, uint, 0644);
MODULE_PARM_DESC(rtw_edcca_mode_sel, "0:NORMAL, 1:CS, 2:ADPT, 3:CBP, 0xFF:auto");

uint rtw_adaptivity_en = CONFIG_RTW_ADAPTIVITY_EN;
module_param(rtw_adaptivity_en, uint, 0644);
MODULE_PARM_DESC(rtw_adaptivity_en, "0:disable, 1:enable, 2:auto");

uint rtw_adaptivity_mode = CONFIG_RTW_ADAPTIVITY_MODE;
module_param(rtw_adaptivity_mode, uint, 0644);
MODULE_PARM_DESC(rtw_adaptivity_mode, "0:normal, 1:carrier sense");

int rtw_adaptivity_th_l2h_ini = CONFIG_RTW_ADAPTIVITY_TH_L2H_INI;
module_param(rtw_adaptivity_th_l2h_ini, int, 0644);
MODULE_PARM_DESC(rtw_adaptivity_th_l2h_ini, "th_l2h_ini for Adaptivity");

int rtw_adaptivity_th_edcca_hl_diff = CONFIG_RTW_ADAPTIVITY_TH_EDCCA_HL_DIFF;
module_param(rtw_adaptivity_th_edcca_hl_diff, int, 0644);
MODULE_PARM_DESC(rtw_adaptivity_th_edcca_hl_diff, "th_edcca_hl_diff for Adaptivity");

#ifdef CONFIG_DFS_MASTER
uint rtw_dfs_region_domain = CONFIG_RTW_DFS_REGION_DOMAIN;
module_param(rtw_dfs_region_domain, uint, 0644);
MODULE_PARM_DESC(rtw_dfs_region_domain, "0:NONE, 1:FCC, 2:MKK, 3:ETSI, 4:KCC");
#endif

static inline void rtw_regsty_load_chplan(struct registry_priv *regsty)
{
	u16 chplan = RTW_CHPLAN_UNSPECIFIED;
	u16 chplan_6g = RTW_CHPLAN_6G_UNSPECIFIED;

	chplan = rtw_channel_plan;
#if CONFIG_IEEE80211_BAND_6GHZ
	chplan_6g = rtw_channel_plan_6g;
#endif

	rtw_chplan_ioctl_input_mapping(&chplan, &chplan_6g);

	regsty->channel_plan = chplan;
#if CONFIG_IEEE80211_BAND_6GHZ
	regsty->channel_plan_6g = chplan_6g;
#endif
}

static inline void rtw_regsty_load_alpha2(struct registry_priv *regsty)
{
	if (!rtw_country_code || strlen(rtw_country_code) != 2
		|| (!IS_ALPHA2_WORLDWIDE(rtw_country_code)
			&& (is_alpha(rtw_country_code[0]) == _FALSE
				|| is_alpha(rtw_country_code[1]) == _FALSE)
			)
	) {
		if (rtw_country_code && rtw_country_code[0] != '\0')
			RTW_ERR("%s discard rtw_country_code not in alpha2 or \"%s\"\n", __func__, WORLDWIDE_ALPHA2);
		SET_UNSPEC_ALPHA2(regsty->alpha2);
	} else
		_rtw_memcpy(regsty->alpha2, rtw_country_code, 2);
}

static void rtw_regsty_load_addl_ch_disable_conf(struct registry_priv *regsty)
{
	int i;
	int ch_num = 0;

	if (rtw_dis_ch_flags && strlen(rtw_dis_ch_flags)) {
		char *buf = rtw_malloc(strlen(rtw_dis_ch_flags) + 1);

		if (buf) {
			char *c;
			enum rtw_ch_type ch_type;

			_rtw_memcpy(buf, rtw_dis_ch_flags, strlen(rtw_dis_ch_flags) + 1);
			for (c = strsep(&buf, ","); c; c = strsep(&buf, ",")) {
				ch_type = get_ch_type_from_str(c, strlen(c));
				if (ch_type != RTW_CHT_NUM)
					regsty->dis_ch_flags |= BIT(ch_type);
			}
			rtw_mfree(buf, strlen(rtw_dis_ch_flags) + 1);
		} else
			RTW_WARN("%s rtw_malloc(strlen(rtw_dis_ch_flags) fail\n", __func__);
	}

	for (i = 0; i < MAX_CHANNEL_NUM_2G_5G; i++)
		if (((u8)rtw_excl_chs[i]) != 0)
			regsty->excl_chs[ch_num++] = (u8)rtw_excl_chs[i];

	if (ch_num < MAX_CHANNEL_NUM_2G_5G)
		regsty->excl_chs[ch_num] = 0;

#if CONFIG_IEEE80211_BAND_6GHZ
	ch_num = 0;
	for (i = 0; i < MAX_CHANNEL_NUM_6G; i++)
		if (((u8)rtw_excl_chs_6g[i]) != 0)
			regsty->excl_chs_6g[ch_num++] = (u8)rtw_excl_chs_6g[i];

	if (ch_num < MAX_CHANNEL_NUM_6G)
		regsty->excl_chs_6g[ch_num] = 0;
#endif
}

#ifdef CONFIG_80211D
static inline void rtw_regsty_load_country_ie_slave_settings(struct registry_priv *regsty)
{
	regsty->country_ie_slave_en_mode = rtw_country_ie_slave_en_mode;
	regsty->country_ie_slave_flags = rtw_country_ie_slave_flags;
	regsty->country_ie_slave_en_role = rtw_country_ie_slave_en_role;
	regsty->country_ie_slave_en_ifbmp = rtw_country_ie_slave_en_ifbmp;
	regsty->country_ie_slave_scan_int_ms = rtw_country_ie_slave_scan_int_ms;
}
#endif

static void rtw_regsty_load_edcca_mode_settings(struct registry_priv *regsty)
{
	regsty->edcca_mode_sel = (u8)rtw_edcca_mode_sel;
	if (regsty->edcca_mode_sel < RTW_EDCCA_MODE_NUM || regsty->edcca_mode_sel == RTW_EDCCA_AUTO) {
		if (regsty->edcca_mode_sel == RTW_EDCCA_NORM) {
			/* consider old interfaces */
			if (rtw_adaptivity_en == RTW_ADAPTIVITY_EN_ENABLE) {
				if (rtw_adaptivity_mode == RTW_ADAPTIVITY_MODE_NORMAL)
					regsty->edcca_mode_sel = RTW_EDCCA_ADAPT;
				else if (rtw_adaptivity_mode == RTW_ADAPTIVITY_MODE_CARRIER_SENSE)
					regsty->edcca_mode_sel = RTW_EDCCA_CS;
			} else if (rtw_adaptivity_en == RTW_ADAPTIVITY_EN_AUTO)
				regsty->edcca_mode_sel = RTW_EDCCA_AUTO;
		}
	} else {
		RTW_WARN("%s invalid rtw_edcca_mode_sel(%u), set to %s\n", __func__
			, regsty->edcca_mode_sel, rtw_edcca_mode_str(RTW_EDCCA_NORM));
		regsty->edcca_mode_sel = RTW_EDCCA_NORM;
	}

	regsty->adaptivity_th_l2h_ini = (s8)rtw_adaptivity_th_l2h_ini;
	regsty->adaptivity_th_edcca_hl_diff = (s8)rtw_adaptivity_th_edcca_hl_diff;
}

#ifdef CONFIG_DFS_MASTER
static void rtw_regsty_load_dfs_region_domain_settings(struct registry_priv *regsty)
{
	regsty->dfs_region_domain = (u8)rtw_dfs_region_domain;
	if (regsty->dfs_region_domain >= RTW_DFS_REGD_NUM) {
		RTW_WARN("%s invalid DFS region domain(%u), set to %s\n", __func__
			, regsty->dfs_region_domain, rtw_dfs_regd_str(RTW_DFS_REGD_NONE));
		regsty->dfs_region_domain = RTW_DFS_REGD_NONE;
		return;
	}
	#ifdef CONFIG_REGD_SRC_FROM_OS
	if (rtw_regd_src == REGD_SRC_OS && regsty->dfs_region_domain != RTW_DFS_REGD_NONE) {
		RTW_WARN("%s force disable radar detection capability when regd_src is OS\n", __func__);
		regsty->dfs_region_domain = RTW_DFS_REGD_NONE;
	}
	#endif
}
#endif

uint rtw_load_registry(_adapter *adapter)
{
	uint status = _SUCCESS;
	struct registry_priv  *registry_par = &adapter->registrypriv;

#ifdef CONFIG_AP_MODE
	registry_par->max_ap_assoc_sta = (u8)rtw_max_ap_assoc_sta;
#endif /* CONFIG_AP_MODE */

	rtw_regsty_load_edcca_mode_settings(registry_par);

#ifdef CONFIG_REGD_SRC_FROM_OS
	if (regd_src_is_valid(rtw_regd_src))
		registry_par->regd_src = (u8)rtw_regd_src;
	else {
		RTW_WARN("%s invalid rtw_regd_src(%u), use REGD_SRC_RTK_PRIV instead\n", __func__, rtw_regd_src);
		registry_par->regd_src = REGD_SRC_RTK_PRIV;
	}
#endif
	registry_par->init_regd_always_apply = !!rtw_init_regd_always_apply;
	registry_par->user_regd_always_apply = !!rtw_user_regd_always_apply;
	rtw_regsty_load_alpha2(registry_par);
	rtw_regsty_load_chplan(registry_par);
	rtw_regsty_load_addl_ch_disable_conf(registry_par);
	registry_par->bcn_hint_valid_ms = rtw_bcn_hint_valid_ms;
#ifdef CONFIG_80211D
	rtw_regsty_load_country_ie_slave_settings(registry_par);
#endif

#ifdef CONFIG_DFS_MASTER
	rtw_regsty_load_dfs_region_domain_settings(registry_par);
#endif

	return status;
}

static void rtw_cfg_edcca_mode_msg(void *sel, _adapter *adapter)
{
	struct registry_priv *regsty = &adapter->registrypriv;

	RTW_PRINT_SEL(sel, "RTW_EDCCA_");
	if (regsty->edcca_mode_sel == RTW_EDCCA_AUTO)
		_RTW_PRINT_SEL(sel, "AUTO\n");
	else
		_RTW_PRINT_SEL(sel, "%s\n", rtw_edcca_mode_str(regsty->edcca_mode_sel));
}

void rtw_cfg_adaptivity_config_msg(void *sel, _adapter *adapter)
{
	rtw_odm_adaptivity_ver_msg(sel, adapter);
	rtw_cfg_edcca_mode_msg(sel, adapter);
}

bool rtw_cfg_adaptivity_needed(_adapter *adapter)
{
	struct registry_priv *regsty = &adapter->registrypriv;

	return regsty->edcca_mode_sel != RTW_EDCCA_NORM;
}


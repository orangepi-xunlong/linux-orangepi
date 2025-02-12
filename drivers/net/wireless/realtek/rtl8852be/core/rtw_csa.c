/******************************************************************************
 *
 * Copyright(c) 2019 - 2021 Realtek Corporation.
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
#define _RTW_CSA_C_
#include <drv_types.h>

#if CONFIG_DFS
#ifdef CONFIG_ECSA_PHL
void reset_all_ecsa_param(struct _ADAPTER *a)
{
	struct dvobj_priv *d = adapter_to_dvobj(a);
	struct _ADAPTER *iface;
	u8 i;

	for (i = 0; i < d->iface_nums; i++) {
		iface = d->padapters[i];
		if (iface->ecsa_info.core_ecsa_type != ECSA_CORE_TYPE_NONE)
			reset_ecsa_param(iface);
	}
}

void reset_ecsa_param(struct _ADAPTER *a)
{
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	struct rtw_phl_ecsa_param *ecsa_param = &(ecsa_info->phl_ecsa_param);

	SET_ECSA_STATE(a, ECSA_ST_NONE);
	ecsa_info->core_ecsa_type = ECSA_CORE_TYPE_NONE;
	ecsa_info->ecsa_allow_case = 0xff;
	ecsa_info->ecsa_delay_time = 0;
	ecsa_info->channel_width = 255;
	ecsa_info->bss_param = NULL;
	ecsa_info->trigger_iface = NULL;
	ecsa_info->execute_iface = NULL;
	_rtw_memset(ecsa_param, 0, sizeof(struct rtw_phl_ecsa_param));
}

bool rtw_is_ecsa_enabled(struct _ADAPTER *a)
{
	return a->registrypriv.en_ecsa;
}

bool rtw_mr_is_ecsa_running(struct _ADAPTER *a)
{
	struct dvobj_priv *d = adapter_to_dvobj(a);
	struct _ADAPTER *iface;
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	u8 i;

	for (i = 0; i < d->iface_nums; i++) {
		iface = d->padapters[i];
		if (!iface)
			continue;
		if (!CHK_ECSA_STATE(iface, ECSA_ST_NONE))
			return _TRUE;
	}
	return _FALSE;
}

bool rtw_hal_is_csa_support(struct _ADAPTER *a)
{
	/* AX series are all supported */
	return true;
}

static void rtw_ecsa_update_sta_cur_network(struct _ADAPTER_LINK *alink, u8 *pframe, u32 packet_len)
{
	WLAN_BSSID_EX *cur_network = &(alink->mlmepriv.cur_network.network);

	cur_network->IELength = packet_len - sizeof(struct rtw_ieee80211_hdr_3addr);
	_rtw_memcpy(cur_network->IEs, (pframe + sizeof(struct rtw_ieee80211_hdr_3addr)), cur_network->IELength);
}

static void rtw_ecsa_parsing_bcn_ie(struct _ADAPTER_LINK *alink)
{
	struct _ADAPTER *a = alink->adapter;
	WLAN_BSSID_EX *cur_network = &(alink->mlmepriv.cur_network.network);
	u8 *bcn_ie = cur_network->IEs + _BEACON_IE_OFFSET_;
	u32 bcn_len = cur_network->IELength - _BEACON_IE_OFFSET_;

	rtw_ie_handler(a, alink, bcn_ie, bcn_len);
}

/* Reference from rtw_joinbss_update_stainfo() and update_sta_info() */
static void rtw_ecsa_sta_update_stainfo(struct _ADAPTER_LINK *alink)
{
	struct _ADAPTER *a = alink->adapter;
	struct sta_info *psta = NULL;
	struct link_mlme_ext_priv *pmlmeext = &(alink->mlmeextpriv);
	struct link_mlme_ext_info *pmlmeinfo = &(pmlmeext->mlmext_info);
	struct link_mlme_priv *pmlmepriv = &(alink->mlmepriv);
	struct wlan_network *cur_network = &(alink->mlmepriv.cur_network);

	psta = rtw_get_stainfo(&a->stapriv, get_link_bssid(&alink->mlmepriv));
	if (!psta) {
		RTW_ERR("CSA : %s sta_info is null\n", __func__);
		return;
	}

	update_wireless_mode(a, alink);
	psta->phl_sta->wmode = pmlmeext->cur_wireless_mode;

	/* update supported rate */
	psta->bssratelen = rtw_get_rateset_len(cur_network->network.SupportedRates);
	_rtw_memcpy(psta->bssrateset, cur_network->network.SupportedRates, psta->bssratelen);

#ifdef CONFIG_80211N_HT
	if (pmlmeinfo->HT_caps_enable) {
		_rtw_memcpy(psta->htpriv.ht_cap.supp_mcs_set,
				pmlmeinfo->HT_caps.u.HT_cap_element.MCS_rate, 16);

		psta->htpriv.ht_option = true;
		if (support_short_GI(a, alink, &(pmlmeinfo->HT_caps), CHANNEL_WIDTH_20))
			psta->htpriv.sgi_20m = true;
		else
			psta->htpriv.sgi_20m = false;

		if (support_short_GI(a, alink, &(pmlmeinfo->HT_caps), CHANNEL_WIDTH_40))
			psta->htpriv.sgi_40m = true;
		else
			psta->htpriv.sgi_40m = false;
	} else {
		_rtw_memset(&(psta->htpriv), 0, sizeof(struct ht_priv));
	}
	psta->htpriv.ch_offset = pmlmeext->chandef.offset;
#endif

#ifdef CONFIG_80211AC_VHT
	if (pmlmeinfo->VHT_enable) {
		_rtw_memcpy(&psta->vhtpriv, &pmlmepriv->vhtpriv, sizeof(struct vht_priv));

		psta->vhtpriv.vht_option = true;
		#ifdef CONFIG_BEAMFORMING
		psta->vhtpriv.beamform_cap = pmlmepriv->vhtpriv.beamform_cap;
		#endif
	} else {
		_rtw_memset(&(psta->vhtpriv), 0, sizeof(struct vht_priv));
	}
#endif

#ifdef CONFIG_80211AX_HE
	if (pmlmeinfo->HE_enable)
		_rtw_memcpy(&psta->hepriv, &pmlmepriv->hepriv, sizeof(struct he_priv));
	else
		_rtw_memset(&(psta->hepriv), 0, sizeof(struct he_priv));
#endif

#if CONFIG_IEEE80211_BAND_6GHZ
	if (psta->phl_sta->chandef.band == BAND_ON_6G) {
		psta->ampdu_priv.ampdu_enable = pmlmepriv->ampdu_priv.ampdu_enable;
		/* update mpdu_min_spacing in HE_6g_bandcap_handler */
		psta->ampdu_priv.rx_ampdu_min_spacing = psta->hepriv.mpdu_min_spacing;
	}
#endif
}

void rtw_ecsa_update_sta_mlme(struct _ADAPTER_LINK *alink, u8 *pframe, u32 packet_len)
{
	struct link_mlme_priv *pmlmepriv = &(alink->mlmepriv);
	struct link_mlme_ext_info *pmlmeinfo = &(alink->mlmeextpriv.mlmext_info);
	struct beacon_keys *cur_beacon = &(pmlmepriv->cur_beacon_keys);

#ifdef CONFIG_80211N_HT
	if (cur_beacon->proto_cap & PROTO_CAP_11N) {
		pmlmepriv->htpriv.ht_option = _TRUE;
		pmlmeinfo->HT_enable = _TRUE;
	} else {
		pmlmepriv->htpriv.ht_option = _FALSE;
		pmlmeinfo->HT_enable = _FALSE;
	}
#endif

#ifdef CONFIG_80211AC_VHT
	if (cur_beacon->proto_cap & PROTO_CAP_11AC) {
		pmlmepriv->vhtpriv.vht_option = _TRUE;
		pmlmeinfo->VHT_enable = _TRUE;
	} else {
		pmlmepriv->vhtpriv.vht_option = _FALSE;
		pmlmeinfo->VHT_enable = _FALSE;
	}
#endif

	rtw_ecsa_update_sta_cur_network(alink, pframe, packet_len);

	/* parse new beacon ie from current network to update protocol information of link_mlme_ext_info */
	rtw_ecsa_parsing_bcn_ie(alink);

	/* update sta_info from protocol information */
	rtw_ecsa_sta_update_stainfo(alink);
}

void rtw_build_csa_ie(struct _ADAPTER *a, struct rtw_phl_ecsa_param *ecsa_param)
{
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	WLAN_BSSID_EX *pnetwork = &(alink->mlmeextpriv.mlmext_info.network);
	u8 csa_data[CSA_IE_LEN] = {0};

	/*
	* [0] : Channel Switch Mode
	* [1] : New Channel Number
	* [2] : Channel Switch Count
	*/
	csa_data[0] = ecsa_param->mode;
	csa_data[1] = ecsa_param->new_chan_def.chan;
	csa_data[2] = ecsa_param->count;
	rtw_add_bcn_ie(a, pnetwork, WLAN_EID_CHANNEL_SWITCH, csa_data, CSA_IE_LEN);
	RTW_INFO("CSA : build channel switch announcement IE by driver\n");
	RTW_INFO("CSA : mode = %u, ch = %u, switch count = %u\n",
			csa_data[0], csa_data[1], csa_data[2]);
}

void rtw_build_ecsa_ie(struct _ADAPTER *a, struct rtw_phl_ecsa_param *ecsa_param)
{
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	WLAN_BSSID_EX *pnetwork = &(alink->mlmeextpriv.mlmext_info.network);
	u8 ecsa_data[ECSA_IE_LEN] = {0};

	/*
	* [0] : Channel Switch Mode
	* [1] : New Operating Class
	* [2] : New Channel Number
	* [3] : Channel Switch Count
	*/
	ecsa_data[0] = ecsa_param->mode;
	ecsa_data[1] = ecsa_param->op_class;
	ecsa_data[2] = ecsa_param->new_chan_def.chan;
	ecsa_data[3] = ecsa_param->count;
	rtw_add_bcn_ie(a, pnetwork, WLAN_EID_ECSA, ecsa_data, ECSA_IE_LEN);
	RTW_INFO("CSA : build extended channel switch announcement IE by driver\n");
	RTW_INFO("CSA : mode = %u, op_class = %u, ch = %u, switch count = %u\n",
			ecsa_data[0], ecsa_data[1], ecsa_data[2], ecsa_data[3]);
}

void rtw_build_sec_offset_ie(struct _ADAPTER *a, u8 seconday_offset)
{
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	WLAN_BSSID_EX *pnetwork = &(alink->mlmeextpriv.mlmext_info.network);

	rtw_add_bcn_ie(a, pnetwork, WLAN_EID_SECONDARY_CHANNEL_OFFSET, &seconday_offset, 1);
	RTW_INFO("CSA : build secondary channel offset IE by driver, sec_offset = %u\n", seconday_offset);
}

void rtw_build_wide_bw_cs_ie(struct _ADAPTER *a, struct rtw_chan_def new_chandef)
{
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	WLAN_BSSID_EX *pnetwork = &(alink->mlmeextpriv.mlmext_info.network);
	u8 csw_data[CS_WR_DATA_LEN] = {0};
	u8 ch_width, seg_0;

	switch (new_chandef.bw) {
	case CHANNEL_WIDTH_40:
		ch_width = 0;
		break;
	case CHANNEL_WIDTH_80:
		ch_width = 1;
		break;
	default:
		ch_width = 1;
		break;
	}

	seg_0 = rtw_phl_get_center_ch(&new_chandef);

	/*
	* subfields of Wide Bandwidth Channel Switch subelement
	* [1] : Length
	* [2] : New Channel Width
	* [3] : New Channel Center Frequency Segment 0
	* [4] : New Channel Center Frequency Segment 1
	*/
	csw_data[0] = WLAN_EID_VHT_WIDE_BW_CHSWITCH;
	csw_data[1] = 3;
	csw_data[2] = ch_width;
	csw_data[3] = seg_0;
	csw_data[4] = 0;
	rtw_add_bcn_ie(a, pnetwork, WLAN_EID_CHANNEL_SWITCH_WRAPPER, csw_data, CS_WR_DATA_LEN);
	RTW_INFO("CSA : build channel switch wrapper IE by driver\n");
	RTW_INFO("CSA : channel width = %u, segment_0 = %u, segment_1 = %u\n",
				csw_data[2], csw_data[3], csw_data[4]);
}

void rtw_core_build_ecsa_beacon(struct _ADAPTER *a,
			struct rtw_phl_ecsa_param *ecsa_param,
			u8 is_vht)
{
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	WLAN_BSSID_EX *pnetwork = &(alink->mlmeextpriv.mlmext_info.network);
	struct rtw_chan_def new_chandef = ecsa_param->new_chan_def;
	struct ieee80211_info_element *ie;
	u8 *ies;
	uint ies_len;
	bool cs_add = false, ecs_add = false, sco_add = false, csw_add = false;

	ies = pnetwork->IEs + _BEACON_IE_OFFSET_;
	ies_len = pnetwork->IELength - _BEACON_IE_OFFSET_;

	/* Check the missing IE of beacon, then build it by driver */
	for_each_ie(ie, ies, ies_len) {
		switch (ie->id) {
		case WLAN_EID_CHANNEL_SWITCH:
			#ifdef DBG_CSA
			RTW_INFO("CSA : find CSA IE\n");
			#endif
			cs_add = true;
			break;

		case WLAN_EID_ECSA:
			#ifdef DBG_CSA
			RTW_INFO("CSA : find ECSA IE\n");
			#endif
			ecs_add = true;
			break;

		case WLAN_EID_SECONDARY_CHANNEL_OFFSET:
			#ifdef DBG_CSA
			RTW_INFO("CSA : find SEC_CH_OFFSET IE\n");
			#endif
			sco_add = true;
			break;

		case WLAN_EID_CHANNEL_SWITCH_WRAPPER:
			#ifdef DBG_CSA
			RTW_INFO("CSA : find CH_SW_WRAPPER IE\n");
			#endif
			csw_add = true;
			break;

		default:
			break;
		}
	}

	/* Build Channel Switch Announcement element */
	if (!cs_add)
		rtw_build_csa_ie(a, ecsa_param);

	/* Build Extended Channel Switch Announcement element */
	if (!ecs_add && rtw_is_ecsa_enabled(a))
		rtw_build_ecsa_ie(a, ecsa_param);

	/* Build Secondary Channel Offset element */
	if (!sco_add && new_chandef.offset != CHAN_OFFSET_NO_EXT)
		rtw_build_sec_offset_ie(a, new_chandef.offset);

	/* Build Channel Switch Wrapper element which only include Wide Bandwidth Channel Switch subelement */
	if (!csw_add && is_vht && (new_chandef.bw >= CHANNEL_WIDTH_40 && new_chandef.bw <= CHANNEL_WIDTH_80_80))
		rtw_build_wide_bw_cs_ie(a, new_chandef);
}

void rtw_cfg80211_build_csa_beacon(struct _ADAPTER *a,
	struct cfg80211_csa_settings *params, struct rtw_chan_def csa_chdef)
{
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	WLAN_BSSID_EX *pnetwork = &(alink->mlmeextpriv.mlmext_info.network);
	struct ieee80211_info_element *ie;
	u8 *ies;
	uint ies_len;

	if (params->beacon_csa.tail) {
		ies = (u8 *)params->beacon_csa.tail;
		ies_len = params->beacon_csa.tail_len;
		RTW_INFO("CSA : Parsing beacon content from cfg80211_csa_settings\n");

		for_each_ie(ie, ies, ies_len) {
			#ifdef DBG_CSA
			RTW_INFO("CSA : for each IE, element id = %u, len = %u\n", ie->id, ie->len);
			#endif
			switch (ie->id) {
			case WLAN_EID_CHANNEL_SWITCH:
				RTW_INFO("CSA : add channel switch announcement IE to beacon\n");
				RTW_INFO("CSA : mode = %u, ch = %u, switch count = %u\n",
						ie->data[0], ie->data[1], ie->data[2]);
				rtw_add_bcn_ie(a, pnetwork, ie->id, ie->data, ie->len);
				break;

			case WLAN_EID_ECSA:
				RTW_INFO("CSA : add extended channel switch announcement IE to beacon\n");
				RTW_INFO("CSA : mode = %u, op_class = %u, ch = %u, switch count = %u\n",
						ie->data[0], ie->data[1], ie->data[2], ie->data[3]);
				rtw_add_bcn_ie(a, pnetwork, ie->id, ie->data, ie->len);
				break;

			/* Secondary channel offset element is not necessary for channel switching to BW_20M */
			case WLAN_EID_SECONDARY_CHANNEL_OFFSET:
				RTW_INFO("CSA : add secondary channel offset IE to beacon, sec_offset = %u\n",
						ie->data[0]);
				rtw_add_bcn_ie(a, pnetwork, ie->id, ie->data, ie->len);
				break;

			case WLAN_EID_CHANNEL_SWITCH_WRAPPER:
				RTW_INFO("CSA : add channel switch wrapper IE to beacon\n");
				RTW_INFO("CSA : channel width = %u, segment_0 = %u, segment_1 = %u\n",
						ie->data[2], ie->data[3], ie->data[4]);
				rtw_add_bcn_ie(a, pnetwork, ie->id, ie->data, ie->len);
				break;

			default:
				break;
			}
		}
	}
}

static void rtw_ecsa_update_sta_chan_info(struct _ADAPTER *a,
			struct _ADAPTER_LINK *alink,
			struct rtw_chan_def new_chan_def)
{
	struct dvobj_priv *d = adapter_to_dvobj(a);
	struct rtw_chset *chset = dvobj_to_chset(d);
	u8 new_ch = new_chan_def.chan;
	u8 new_bw = (u8)new_chan_def.bw;
	u8 new_offset = (u8)new_chan_def.offset;
	bool is_chctx_add = _FALSE;
	struct rtw_mr_chctx_info mr_cc_info = {0};
	struct link_mlme_ext_priv *pmlmeext = &alink->mlmeextpriv;
	struct link_mlme_priv *pmlmepriv = &alink->mlmepriv;
	u32 csa_wait_bcn_ms;

	pmlmeext->chandef.band= new_chan_def.band;
	pmlmeext->chandef.chan= new_ch;
	pmlmeext->chandef.bw = new_bw;
	pmlmeext->chandef.offset = new_offset;
	pmlmepriv->cur_network.network.Configuration.DSConfig = new_ch;

	/* update wifi role link chandef */
	rtw_hw_update_chan_def(a, alink);
	/* update chanctx */
	rtw_phl_chanctx_del(d->phl, a->phl_role, alink->wrlink, NULL);
	is_chctx_add = rtw_phl_chanctx_add(d->phl, a->phl_role,
						alink->wrlink,
						&new_chan_def, &mr_cc_info);
	if (is_chctx_add == _FALSE)
		RTW_ERR("CSA : "FUNC_ADPT_FMT" chan_ctx add fail!", FUNC_ADPT_ARG(a));

	set_fwstate(&a->mlmepriv, WIFI_CSA_UPDATE_BEACON);

	if (rtw_chset_is_dfs_chbw(chset, new_ch, new_bw, new_offset))
		csa_wait_bcn_ms = CAC_TIME_MS + 10000;
	else
		csa_wait_bcn_ms = WAIT_BCN_TIMES * 2 * 1000;

	RTW_INFO("CSA : set csa_wait_bcn_timer to %u ms\n", csa_wait_bcn_ms);
	_set_timer(&a->mlmeextpriv.csa_wait_bcn_timer, csa_wait_bcn_ms);

	#if CONFIG_DFS && CONFIG_IEEE80211_BAND_5GHZ
	rtw_dfs_rd_en_dec_on_mlme_act(a, alink, MLME_OPCH_SWITCH, 0);
	#endif
}

void rtw_ap_update_beacon_by_role(void *priv, struct rtw_wifi_role_t *role , struct rtw_wifi_role_link_t *rlink)
{
	/* TODO */
}
static void rtw_ecsa_update_ap_chan_info(struct _ADAPTER *a, struct rtw_chan_def new_chan_def)
{
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	struct createbss_parm *parm;

	ecsa_info->bss_param = (struct createbss_parm *)rtw_zmalloc(sizeof(struct createbss_parm));
	if (ecsa_info->bss_param) {
		parm = ecsa_info->bss_param;
		parm->adhoc = 0;
		parm->ifbmp = BIT(a->iface_id);
		parm->excl_ifbmp = 0;
		parm->req_band = new_chan_def.band;
		parm->req_ch = new_chan_def.chan;
		parm->req_bw = new_chan_def.bw;
		parm->req_offset = new_chan_def.offset;
		parm->ifbmp_ch_changed = 0;
		parm->ch_to_set = 0;
		parm->bw_to_set = 0;
		parm->offset_to_set = 0;
		parm->is_change_chbw = _TRUE;

		rtw_core_ap_prepare(a, parm);
	} else {
		RTW_ERR("CSA : can't allocate memory for bss_param\n");
	}
}

/**
 * rtw_ecsa_update_sta_ap_chan_info - update STA role and AP role chandef
 * Flow :
 * del AP role chandef -> del STA role chandef -> add STA role chandef -> add AP role chandef
 */
static void rtw_ecsa_update_sta_ap_chan_info(struct _ADAPTER *ap_iface,
			struct _ADAPTER_LINK *ap_iface_link,
			struct rtw_chan_def ap_new_chan_def)
{
	struct dvobj_priv *d = adapter_to_dvobj(ap_iface);
	struct _ADAPTER *sta_iface = ap_iface->ecsa_info.trigger_iface;
	struct _ADAPTER_LINK *sta_iface_link = GET_PRIMARY_LINK(sta_iface);
	struct rtw_chan_def sta_new_chan_def = sta_iface->ecsa_info.phl_ecsa_param.new_chan_def;

	rtw_phl_chanctx_del(d->phl, ap_iface->phl_role, ap_iface_link->wrlink, NULL);
	ap_iface_link->is_ap_chan_ctx_added = false;

	rtw_ecsa_update_sta_chan_info(sta_iface, sta_iface_link, sta_new_chan_def);

	rtw_ecsa_update_ap_chan_info(ap_iface, ap_new_chan_def);
}

void rtw_ecsa_update_probe_resp(struct xmit_frame *xframe)
{
	struct _ADAPTER *a = xframe->padapter;
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	struct rtw_phl_ecsa_param *ecsa_param = &(ecsa_info->phl_ecsa_param);
	struct pkt_attrib *pattrib = &xframe->attrib;
	u8 hdr_len = sizeof(struct rtw_ieee80211_hdr_3addr);
	u8 *ies;
	sint ies_len;
	u8 *csa_ie, *ecsa_ie;
	sint csa_ie_len, ecsa_ie_len;

	ies = xframe->buf_addr + TXDESC_OFFSET + hdr_len + _BEACON_IE_OFFSET_;
	ies_len = pattrib->pktlen - hdr_len - _BEACON_IE_OFFSET_;

	csa_ie = rtw_get_ie(ies, WLAN_EID_CHANNEL_SWITCH, &csa_ie_len, ies_len);

	if (csa_ie == NULL)
		return;

	csa_ie[2 + CSA_SWITCH_COUNT] = ecsa_param->count;

	#ifdef DBG_CSA
	RTW_INFO("CSA : update csa count of probe response = %u\n",
		csa_ie[2 + CSA_SWITCH_COUNT]);
	#endif

	if (rtw_is_ecsa_enabled(a)) {
		ecsa_ie = rtw_get_ie(ies, WLAN_EID_ECSA, &ecsa_ie_len, ies_len);
		if (ecsa_ie == NULL)
			return;

		/*
		* [0] : Channel Switch Mode
		* [1] : New Operating Class
		* [2] : New Channel Number
		* [3] : Channel Switch Count
		*/
		ecsa_ie[2 + 3] = ecsa_param->count;

		#ifdef DBG_CSA
		RTW_INFO("CSA : update ecsa count of probe response = %u\n",
			ecsa_ie[2 + 3]);
		#endif
	}
}

static void rtw_check_phl_ecsa_request(struct _ADAPTER *a)
{
	struct dvobj_priv *d = adapter_to_dvobj(a);
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	u8 is_vht = alink->mlmepriv.vhtpriv.vht_option;
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	struct rtw_phl_ecsa_param *ecsa_param = &(ecsa_info->phl_ecsa_param);
	struct rtw_phl_ecsa_param *phl_ecsa_param;

	/* ECSA request comes from PHL MR module, so we need to fill ecsa_param */
	if (CHK_ECSA_STATE(a, ECSA_ST_SW_START) &&
	   ecsa_info->core_ecsa_type == ECSA_CORE_TYPE_NONE) {
		/* Get pointer of ecsa_parameter from PHL, then copy value to ecsa_parameter of core */
		rtw_phl_ecsa_get_param(d->phl, &phl_ecsa_param);
		_rtw_memcpy(ecsa_param, phl_ecsa_param, sizeof(struct rtw_phl_ecsa_param));
		rtw_core_build_ecsa_beacon(a, ecsa_param, is_vht);
	}
}

static bool _chk_csa_groupd(struct rtw_chan_def *chandef_a,
	struct rtw_chan_def *chandef_b)
{
	if (chandef_a->band != chandef_b->band)
		return _FALSE;

	if ((chandef_a->bw == CHANNEL_WIDTH_40 || chandef_a->bw == CHANNEL_WIDTH_80) &&
	   (chandef_b->bw == CHANNEL_WIDTH_40 || chandef_b->bw == CHANNEL_WIDTH_80)) {
		if (chandef_a->offset != chandef_b->offset)
			return _FALSE;
	}
	return _TRUE;
}

void rtw_ecsa_update_beacon(void *priv, struct rtw_wifi_role_t *role , struct rtw_wifi_role_link_t *rlink)
{
#ifdef CONFIG_AP_MODE
	struct dvobj_priv *d = (struct dvobj_priv *)priv;
	struct _ADAPTER *a = d->padapters[role->id];
	struct _ADAPTER_LINK *alink = GET_LINK(a, rlink->id);
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	struct rtw_phl_ecsa_param *ecsa_param = &(ecsa_info->phl_ecsa_param);
	WLAN_BSSID_EX *pnetwork = &(alink->mlmeextpriv.mlmext_info.network);

	rtw_check_phl_ecsa_request(a);

	/* update CSA and ECSA IE at the same time */
	_update_beacon(a, alink, WLAN_EID_CHANNEL_SWITCH, NULL, _TRUE, 0, "update CSA count");

	if (ecsa_param->count == 0) {
		rtw_remove_bcn_ie(a, pnetwork, WLAN_EID_SECONDARY_CHANNEL_OFFSET);
		rtw_remove_bcn_ie(a, pnetwork, WLAN_EID_CHANNEL_SWITCH_WRAPPER);
	}
#endif
}

/* PHL MR module check core layer whether AP mode can switch channel now */
bool rtw_ap_check_ecsa_allow(
	void *priv,
	struct rtw_wifi_role_t *role,
	struct rtw_chan_def sta_chdef,
	enum phl_ecsa_start_reason reason,
#ifdef CONFIG_ECSA_EXTEND_OPTION
	u32 *extend_option,
#endif
	u32 *delay_start_ms
)
{
	struct dvobj_priv *d = (struct dvobj_priv *)priv;
	struct _ADAPTER *a = d->padapters[role->id];
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	struct rtw_chan_def ap_chdef = alink->wrlink->chandef;
	struct rf_ctl_t *rfctl = dvobj_to_rfctl(d);
	RT_CHANNEL_INFO *chinfo = rtw_chset_get_chinfo_by_bch(&rfctl->chset, sta_chdef.band, sta_chdef.chan, 0);
	bool ecsa_allow = _TRUE;
	u8 i;

#ifdef CONFIG_MCC_MODE
	if (rtw_hw_mcc_chk_inprogress(a, alink)) {
		RTW_INFO("CSA : "FUNC_ADPT_FMT" : Don't switch channel if MCC enabled\n",
			FUNC_ADPT_ARG(a));
		ecsa_allow = _FALSE;
		goto exit;
	}
#endif

	if (!(ecsa_info->ecsa_allow_case & BIT(reason))) {
		RTW_INFO("CSA : "FUNC_ADPT_FMT" : Case %u not support!\n", FUNC_ADPT_ARG(a), reason);
		ecsa_allow = _FALSE;
		goto exit;
	}

	/* Check DFS channel */
	if (!chinfo || (chinfo->flags & (RTW_CHF_NO_IR | RTW_CHF_DFS))) {
		RTW_ERR("CSA : "FUNC_ADPT_FMT" : DFS channel (%u) not support!\n",
				FUNC_ADPT_ARG(a), sta_chdef.chan);
		ecsa_allow = _FALSE;
		goto exit;
	}

	if (ecsa_info->ecsa_delay_time != 0)
		*delay_start_ms = ecsa_info->ecsa_delay_time;
	else if (reason <= ECSA_START_MCC_5G_TO_24G)
		*delay_start_ms = MCC_ECSA_DELAY_START_TIME;
	RTW_INFO("CSA : "FUNC_ADPT_FMT" ECSA will delay %u ms to start\n", FUNC_ADPT_ARG(a), *delay_start_ms);

	SET_ECSA_STATE(a, ECSA_ST_SW_START);

	/*
	* AP mode use the its bandwidth/offset to switch channel if its bandwidth/offset can group with STA mode.
	* Otherwise, AP mode use chandef of STA mode.
	*/
	if (_chk_csa_groupd(&ap_chdef, &sta_chdef)) {
		#ifdef CONFIG_ECSA_EXTEND_OPTION
		*extend_option = ECSA_EX_OPTION_USE_AP_CHANDEF;
		#endif
		RTW_INFO("CSA : AP mode use its bandwidth/offset to switch channel\n");
	} else {
		RTW_INFO("CSA : bandwidth/offset of AP mode can't group with STA mode, so use bandwidth/offset of STA mode\n");
	}

	RTW_INFO("CSA : Switch AP to STA's channel. AP:%u,%u,%u  STA:%u,%u,%u\n",
		ap_chdef.chan, ap_chdef.bw, ap_chdef.offset,
		sta_chdef.chan, sta_chdef.bw, sta_chdef.offset);
exit:
	return ecsa_allow;
}

void rtw_ecsa_mr_update_chan_info_by_role(
	void *priv,
	struct rtw_wifi_role_t *role,
	struct rtw_wifi_role_link_t *rlink,
	struct rtw_chan_def new_chan_def)
{
	struct dvobj_priv *d = (struct dvobj_priv *)priv;
	struct _ADAPTER *a = d->padapters[role->id];
	struct _ADAPTER_LINK *alink = GET_LINK(a, rlink->id);
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);

	RTW_INFO("CSA : "FUNC_ADPT_FMT", new ch/bw/offset = %u,%u,%u\n",
		FUNC_ADPT_ARG(a), new_chan_def.chan, new_chan_def.bw,
		new_chan_def.offset);

	if (ecsa_info->core_ecsa_type == ECSA_CORE_TYPE_SCC_STA_AP &&
	   (role->type == PHL_RTYPE_AP || role->type == PHL_RTYPE_P2P_GO)) {
		/* multi-role STA + AP in SCC */
		rtw_ecsa_update_sta_ap_chan_info(a, alink, new_chan_def);
	} else {
		 /* single-role or multi-role in MCC or DBCC */
		if (role->type == PHL_RTYPE_STATION || role->type == PHL_RTYPE_P2P_GC)
			rtw_ecsa_update_sta_chan_info(a, alink, new_chan_def);
		else if (role->type == PHL_RTYPE_AP ||role->type == PHL_RTYPE_P2P_GO)
			rtw_ecsa_update_ap_chan_info(a, new_chan_def);
	}
}

bool rtw_ecsa_check_tx_resume_allow(void *priv, struct rtw_wifi_role_t *role)
{
	/* TODO */
	/* Is DFS slave still monitoring channel ?
	If Yes, return False to PHL; If no, return True to PHL */
	struct dvobj_priv *d = (struct dvobj_priv *)priv;
	struct _ADAPTER *a = d->padapters[role->id];

	RTW_INFO("CSA : "FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(a));
	return 1;
}

void rtw_ecsa_complete(void *priv, struct rtw_wifi_role_t *role)
{
	struct dvobj_priv *d = (struct dvobj_priv *)priv;
	struct _ADAPTER *a = d->padapters[role->id];
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(a);
	struct rf_ctl_t *rfctl = dvobj_to_rfctl(d);
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	struct createbss_parm *parm;
	u8 ht_option = 0;

	RTW_INFO("CSA : "FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(a));
	rtw_phl_mr_dump_cur_chandef(d->phl, role);

	 /* single-rol or multi-role in MCC or DBCC */
	if (role->type == PHL_RTYPE_STATION || role->type == PHL_RTYPE_P2P_GC) {
		/*
		* TODO
		* STA mode need to update RA if it receive CHANNEL_SWITCH_WRAPPER IE
		* STA mode update its RA at rtw_check_bcn_info() now
		*/
		rtw_rfctl_update_op_mode(rfctl, 0, 0, 0);

		clr_fwstate(&(a->mlmepriv), WIFI_CSA_SKIP_CHECK_BEACON);
	} else if (role->type == PHL_RTYPE_AP ||role->type == PHL_RTYPE_P2P_GO) {
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
		#ifdef CONFIG_80211N_HT
		ht_option = alink->mlmepriv.htpriv.ht_option;
		#endif
		rtw_cfg80211_ch_switch_notify(a,
			GET_PRIMARY_LINK(a),
			&alink->mlmeextpriv.chandef,
			ht_option, 0);
		#endif

		parm = ecsa_info->bss_param;
		rtw_rfctl_update_op_mode(adapter_to_rfctl(a),
			parm->ifbmp, 1, parm->excl_ifbmp);
		rtw_core_ap_start(a, parm);
		rtw_ap_update_clients_rainfo(a, PHL_CMD_NO_WAIT);
		rtw_mfree((u8 *)parm, sizeof(struct createbss_parm));
	}

	rtw_mi_os_xmit_schedule(a);
	reset_all_ecsa_param(a);
}

static void rtw_ap_send_csa_action_frame(struct _ADAPTER *a, struct _ADAPTER_LINK *alink)
{
	_list	*phead, *plist;
	struct sta_info *psta = NULL;
	struct sta_priv *pstapriv = &(a->stapriv);
	u8 bc_addr[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	rtw_stapriv_asoc_list_lock(pstapriv);
	phead = &pstapriv->asoc_list;
	plist = get_next(phead);

	/* for each sta in asoc_queue */
	while ((rtw_end_of_queue_search(phead, plist)) == _FALSE) {
		psta = LIST_CONTAINOR(plist, struct sta_info, asoc_list);
		plist = get_next(plist);

		issue_action_spct_ch_switch(a, alink, psta->phl_sta->mac_addr);
	}
	rtw_stapriv_asoc_list_unlock(pstapriv);

	issue_action_spct_ch_switch(a, alink, bc_addr);
}

/* Get ch/bw/offset of CSA from adapter, and check these parameters is valid or not */
static bool rtw_sta_get_ecsa_setting(struct _ADAPTER *a, s16 *req_ch, u8 *req_bw, u8 *req_offset)
{
	struct rtw_chset *chset = adapter_to_chset(a);
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	struct rtw_phl_ecsa_param *ecsa_param = &(ecsa_info->phl_ecsa_param);
	u8 ifbmp_m = rtw_mi_get_ap_mesh_ifbmp(a);
	u8 band = ecsa_param->new_chan_def.band;
	u8 csa_chan = ecsa_param->new_chan_def.chan;
	u8 csa_offset = ecsa_param->new_chan_def.offset;
	bool valid = _TRUE;

	*req_ch = REQ_CH_NONE;
	*req_bw = CHANNEL_WIDTH_20;
	*req_offset = CHAN_OFFSET_NO_EXT;

	if (rtw_chset_search_ch(chset, csa_chan) >= 0
		&& !rtw_chset_is_ch_non_ocp(chset, csa_chan)
	) {
		/* CSA channel available and valid */
		*req_ch = csa_chan;
		RTW_INFO("CSA : "FUNC_ADPT_FMT" valid CSA ch%u\n", FUNC_ADPT_ARG(a), csa_chan);
	} else if (ifbmp_m) {
		/* no available or valid CSA channel, having AP/MESH ifaces */
		*req_ch = REQ_CH_NONE;
		valid = _FALSE;
		RTW_INFO("CSA : "FUNC_ADPT_FMT" ch sel by AP/MESH ifaces\n", FUNC_ADPT_ARG(a));
		goto exit;
	} else {
		/* no available or valid CSA channel and no AP/MESH ifaces */
		/* TODO : DFS slave may need to switch channel as soon as possible before disconnect */
		#if 0
		if (!is_supported_24g(adapter_to_regsty(a)->band_type))
			*req_ch = 36;
		else
			*req_ch = 1;
		#endif
		valid = _FALSE;
		RTW_INFO("CSA : "FUNC_ADPT_FMT" switch to ch %d, then disconnect with AP\n",
			FUNC_ADPT_ARG(a), *req_ch);
		goto exit;
	}

	if (ecsa_param->op_class != 0) {
		/* Get chan_def by operating class */
		rtw_phl_get_chandef_from_operating_class(*req_ch,
			ecsa_param->op_class, &ecsa_param->new_chan_def);

		*req_bw = ecsa_param->new_chan_def.bw;
		*req_offset = ecsa_param->new_chan_def.offset;

		/* Get correct offset and check ch/bw/offset is valid or not */
		if (*req_offset == CHAN_OFFSET_NO_DEF) {
			if (!rtw_get_offset_by_chbw(*req_ch, *req_bw, req_offset)) {
				*req_bw = CHANNEL_WIDTH_20;
				*req_offset = CHAN_OFFSET_NO_EXT;
			}
			ecsa_param->new_chan_def.bw = *req_bw;
			ecsa_param->new_chan_def.offset = *req_offset;
		}
	} else {
		/* Transform channel_width to bandwidth 20/40/80M */
		switch (ecsa_info->channel_width) {
		case CH_WIDTH_80_160M:
			*req_bw = CHANNEL_WIDTH_80;
			*req_offset = csa_offset;
			break;
		case CH_WIDTH_20_40M:
			/*
			* We don't know the actual offset of channel 5 to 9
			* if offset is CHAN_OFFSET_NO_EXT and bandwidth is 40MHz,
			* so force its bandwidth to 20MHz
			*/
			if ((band == BAND_ON_24G && *req_ch >= 5 && *req_ch <=9) &&
			   csa_offset == CHAN_OFFSET_NO_EXT)
				*req_bw = CHANNEL_WIDTH_20;
			else
				*req_bw = CHANNEL_WIDTH_40;
			*req_offset = csa_offset;
			break;
		default:
			*req_bw = CHANNEL_WIDTH_20;
			*req_offset = CHAN_OFFSET_NO_EXT;
			break;
		}

		/* Get correct offset and check ch/bw/offset is valid or not */
		if (!rtw_get_offset_by_chbw(*req_ch, *req_bw, req_offset)) {
			*req_bw = CHANNEL_WIDTH_20;
			*req_offset = CHAN_OFFSET_NO_EXT;
		}

		/* Update result to ecsa_param */
		ecsa_param->new_chan_def.chan = *req_ch;
		ecsa_param->new_chan_def.bw = *req_bw;
		ecsa_param->new_chan_def.offset = *req_offset;
	}
exit:
	return valid;
}

static void rtw_sta_ecsa_invalid_hdl(struct _ADAPTER *a, s16 req_ch, u8 req_bw, u8 req_offset)
{
	struct dvobj_priv *d = adapter_to_dvobj(a);
	struct rf_ctl_t *rfctl = dvobj_to_rfctl(d);
	struct mlme_ext_priv *pmlmeext = &a->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &pmlmeext->mlmext_info;
	u8 ifbmp_s = rtw_mi_get_ld_sta_ifbmp(a);
	struct rtw_chan_def mr_chdef = {0};

	if (!ifbmp_s)
		return;

	set_fwstate(&a->mlmepriv,  WIFI_OP_CH_SWITCHING);
	issue_deauth(a, get_bssid(&a->mlmepriv), WLAN_REASON_DEAUTH_LEAVING);

	/* Decide whether enable DFS slave radar detection or not */
	#if CONFIG_DFS && CONFIG_IEEE80211_BAND_5GHZ
	rtw_dfs_rd_en_dec_on_mlme_act(a, GET_PRIMARY_LINK(a), MLME_OPCH_SWITCH, ifbmp_s);
	#endif

	/* TODO : DFS slave may need to switch channel as soon as possible before disconnect */

	/* This context can't I/O, so use RTW_CMDF_DIRECTLY */
	rtw_disassoc_cmd(a, 0, RTW_CMDF_DIRECTLY);
	rtw_indicate_disconnect(a, 0, _FALSE);
	#ifndef CONFIG_STA_CMD_DISPR
	rtw_free_assoc_resources(a, _TRUE);
	#endif
	rtw_free_network_queue(a, _TRUE);
	rtw_free_mld_network_queue(a, _TRUE);
	RTW_INFO("CSA : "FUNC_ADPT_FMT" disconnect with AP\n", FUNC_ADPT_ARG(a));

	pmlmeinfo->disconnect_occurred_time = rtw_systime_to_ms(rtw_get_current_time());
	pmlmeinfo->disconnect_code = DISCONNECTION_BY_DRIVER_DUE_TO_RECEIVE_INVALID_CSA;
	pmlmeinfo->wifi_reason_code = WLAN_REASON_DEAUTH_LEAVING;

	rtw_mi_os_xmit_schedule(a);
}

static enum ecsa_mr_case rtw_ecsa_mr_check(struct _ADAPTER *execute_iface)
{
	struct dvobj_priv *d = adapter_to_dvobj(execute_iface);
	struct rtw_wifi_role_t *role = execute_iface->phl_role;
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(execute_iface);
	struct rtw_wifi_role_link_t *rlink = alink->wrlink;
	struct core_ecsa_info *ecsa_info = &(execute_iface->ecsa_info);
	struct rtw_phl_ecsa_param *ecsa_param = &(ecsa_info->phl_ecsa_param);
	struct rtw_chan_def n_chdef;
	struct mi_state mstate = {0};
	struct rtw_mr_chctx_info mr_cc_info = {0};

	n_chdef = ecsa_param->new_chan_def;
	rtw_mi_status(execute_iface, &mstate);

	/* Check whether channel, bandwidth, offset can group under multi-role */
	if ((MSTATE_STA_LD_NUM(&mstate) +
	   MSTATE_AP_NUM(&mstate) +
	   MSTATE_MESH_NUM(&mstate) >= 2) &&
	   !rtw_phl_chanctx_chk(GET_PHL_INFO(d), role, rlink, &n_chdef, &mr_cc_info)) {
		#ifdef CONFIG_DBCC_SUPPORT
		if (mr_cc_info.sugg_opmode == MR_OP_DBCC) {
			rtw_phl_mr_trig_dbcc_enable(GET_PHL_INFO(d));
		} else
		#endif
		{
			/* chandef of multi-role can't group */
			RTW_INFO("CSA : "FUNC_ADPT_FMT ": "
				"chandef can't group, use SCC mode to switch channel! "
				"ld_sta_num:%u, ap_num:%u, mesh_num:%u\n",
				FUNC_ADPT_ARG(execute_iface), MSTATE_STA_LD_NUM(&mstate),
				MSTATE_AP_NUM(&mstate), MSTATE_MESH_NUM(&mstate));

			/* Support ECSA of one STA + one AP/GO */
			if (MSTATE_STA_LD_NUM(&mstate) == 1 &&
			   (MSTATE_AP_NUM(&mstate) + MSTATE_MESH_NUM(&mstate) == 1))
				return ECSA_MR_SCC_STA_AP;
			else
				return ECSA_MR_UNSUPPORTED;
		}
	}

	return ECSA_SINGLE_ROLE;
}

static struct _ADAPTER *rtw_scc_sta_ap_init_ap_role(struct _ADAPTER *sta_iface)
{
	struct dvobj_priv *d = adapter_to_dvobj(sta_iface);
	struct core_ecsa_info *sta_ecsa_info = &(sta_iface->ecsa_info);
	struct rtw_phl_ecsa_param *sta_ecsa_param = &(sta_ecsa_info->phl_ecsa_param);
	struct core_ecsa_info *ap_ecsa_info;
	struct rtw_phl_ecsa_param *ap_ecsa_param;
	struct _ADAPTER *ap_iface = NULL;
	struct _ADAPTER_LINK *alink;
	struct link_mlme_ext_priv *lmlmeext;
	struct rtw_chan_def *ap_new_chan_def;
	u8 is_vht;
	u8 i;

	sta_ecsa_info->trigger_iface = sta_iface;
	for (i = 0; i < d->iface_nums; i++) {
		ap_iface = d->padapters[i];

		if (!ap_iface || ap_iface == sta_iface || !rtw_is_adapter_up(ap_iface))
			continue;

		if ((MLME_IS_AP(ap_iface) || MLME_IS_MESH(ap_iface))
			&& check_fwstate(&ap_iface->mlmepriv, WIFI_ASOC_STATE)) {

			sta_ecsa_info->execute_iface = ap_iface;
			alink = GET_PRIMARY_LINK(ap_iface);
			lmlmeext = &(alink->mlmeextpriv);
			ap_ecsa_info = &(ap_iface->ecsa_info);
			ap_ecsa_param = &(ap_ecsa_info->phl_ecsa_param);

			SET_ECSA_STATE(ap_iface, ECSA_ST_SW_START);
			ap_ecsa_info->core_ecsa_type = ECSA_CORE_TYPE_SCC_STA_AP;
			ap_ecsa_info->trigger_iface = sta_iface;
			ap_ecsa_info->execute_iface = ap_iface;
			_rtw_memcpy(ap_ecsa_param, sta_ecsa_param, sizeof(struct rtw_phl_ecsa_param));

			ap_new_chan_def = &(ap_ecsa_param->new_chan_def);
			ap_new_chan_def->bw = lmlmeext->chandef.bw;
			ap_new_chan_def->offset = lmlmeext->chandef.offset;

			/* adjust bw according registery and hw cap*/
			rtw_adjust_bchbw(ap_iface, ap_new_chan_def->band,
						ap_new_chan_def->chan,
						(u8 *)&ap_new_chan_def->bw,
						(u8 *)&ap_new_chan_def->offset);

			/* adjust bw and offset according chanctx to avoid wrong offset */
			rtw_phl_adjust_chandef(d->phl, alink->wrlink, ap_new_chan_def);

			is_vht = alink->mlmepriv.vhtpriv.vht_option;
			rtw_core_build_ecsa_beacon(ap_iface, ap_ecsa_param, is_vht);
			break;
		}
	}

	return ap_iface;
}

void rtw_fill_phl_ecsa_type(struct core_ecsa_info *ecsa_info)
{
	struct rtw_phl_ecsa_param *ecsa_param = &(ecsa_info->phl_ecsa_param);

	switch (ecsa_info->core_ecsa_type) {
	case ECSA_CORE_TYPE_NONE:
		ecsa_param->ecsa_type = ECSA_TYPE_NONE;
		break;
	case ECSA_CORE_TYPE_AP:
		ecsa_param->ecsa_type = ECSA_TYPE_AP;
		break;
	case ECSA_CORE_TYPE_STA:
		ecsa_param->ecsa_type = ECSA_TYPE_STA;
		break;
	case ECSA_CORE_TYPE_SCC_STA_AP:
		ecsa_param->ecsa_type = ECSA_TYPE_AP;
		break;
	}
}

/**
 * @trigger_iface : interface which trigger core layer ECSA flow
 * @execute_iface : interface which actually execute PHL ECSA FG
 */
bool rtw_trigger_phl_ecsa_start(struct _ADAPTER *trigger_iface,
	enum csa_trigger_type trigger_type)
{
	struct dvobj_priv *d = adapter_to_dvobj(trigger_iface);
	struct _ADAPTER *execute_iface = trigger_iface;
	struct rtw_wifi_role_t *role = execute_iface->phl_role;
	/* ToDo CONFIG_RTW_MLD: [currently primary link only] */
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(execute_iface);
	struct rtw_wifi_role_link_t *rlink = alink->wrlink;
	struct rtw_chan_def c_chdef = alink->mlmeextpriv.chandef;
	struct core_ecsa_info *ecsa_info = &(execute_iface->ecsa_info);
	struct rtw_phl_ecsa_param *ecsa_param = &(ecsa_info->phl_ecsa_param);
	struct rtw_chan_def n_chdef;
	enum ecsa_mr_case mr_case;
	s16 req_ch;
	u8 req_bw, req_offset;
	u8 is_vht = alink->mlmepriv.vhtpriv.vht_option;

	switch (trigger_type) {
	case CSA_AP_CORE_SWITCH_CH:
		ecsa_info->core_ecsa_type = ECSA_CORE_TYPE_AP;

		/* Build CSA beacon by self */
		rtw_core_build_ecsa_beacon(trigger_iface, ecsa_param, is_vht);
		break;

	case CSA_AP_CFG80211_SWITCH_CH:
		ecsa_info->core_ecsa_type = ECSA_CORE_TYPE_AP;

		/* Build CSA beacon by self */
		rtw_core_build_ecsa_beacon(trigger_iface, ecsa_param, is_vht);
		break;

	case CSA_STA_RX_CSA_IE:
		if (!rtw_sta_get_ecsa_setting(execute_iface, &req_ch, &req_bw, &req_offset)) {
			/* we should handle error case by core layer self */
			rtw_sta_ecsa_invalid_hdl(execute_iface, req_ch, req_bw, req_offset);
			goto err_hdl;
		}

		mr_case = rtw_ecsa_mr_check(execute_iface);
		switch (mr_case) {
		case ECSA_SINGLE_ROLE:
			ecsa_info->core_ecsa_type = ECSA_CORE_TYPE_STA;
			break;

		case ECSA_MR_SCC_STA_AP:
			ecsa_info->core_ecsa_type = ECSA_CORE_TYPE_SCC_STA_AP;

			/* Use AP interface to execute ECSA FG */
			execute_iface = rtw_scc_sta_ap_init_ap_role(trigger_iface);
			if (execute_iface == NULL)
				goto err_hdl;
			break;

		case ECSA_MR_UNSUPPORTED:
			RTW_ERR("CSA : only support ECSA of one STA + one AP/GO for now\n");
			goto err_hdl;
			break;
		}

		set_fwstate(&execute_iface->mlmepriv, WIFI_CSA_SKIP_CHECK_BEACON);
		break;

	case CSA_STA_DISCONNECT_ON_DFS:
		ecsa_info->core_ecsa_type = ECSA_CORE_TYPE_AP;

		/* Build CSA beacon by self */
		rtw_core_build_ecsa_beacon(trigger_iface, ecsa_param, is_vht);

		/* Send CSA action frame to clients */
		rtw_ap_send_csa_action_frame(trigger_iface, alink);
		break;

	default:
		goto err_hdl;
	}

	n_chdef = ecsa_param->new_chan_def;
	RTW_INFO("CSA : "FUNC_ADPT_FMT" core_ecsa_type = %u\n",
		FUNC_ADPT_ARG(trigger_iface), ecsa_info->core_ecsa_type);
	RTW_INFO("CSA : Trigger interface("ADPT_FMT")"
			"current:%u,%u,%u,%u ==> new:%u,%u,%u,%u\n",
		ADPT_ARG(trigger_iface),
		c_chdef.band, c_chdef.chan, c_chdef.bw, c_chdef.offset,
		n_chdef.band, n_chdef.chan, n_chdef.bw, n_chdef.offset);

	/**
	  * Update all parameters to parameters of interface which execute ECSA FG
	  * if execute_iface doesn't equal to trigger_iface.
	  */
	if (execute_iface != trigger_iface) {
		role = execute_iface->phl_role;
		alink = GET_PRIMARY_LINK(execute_iface);
		rlink = alink->wrlink;
		ecsa_info = &(execute_iface->ecsa_info);
		ecsa_param = &(ecsa_info->phl_ecsa_param);

		c_chdef = alink->mlmeextpriv.chandef;
		n_chdef = ecsa_param->new_chan_def;
		RTW_INFO("CSA : Execute interface("ADPT_FMT")"
				"current:%u,%u,%u,%u ==> new:%u,%u,%u,%u\n",
			ADPT_ARG(execute_iface),
			c_chdef.band, c_chdef.chan, c_chdef.bw, c_chdef.offset,
			n_chdef.band, n_chdef.chan, n_chdef.bw, n_chdef.offset);
	}

	/* Only execute interface needs to fill PHL ECSA type */
	rtw_fill_phl_ecsa_type(ecsa_info);
	if (rtw_phl_ecsa_start(GET_PHL_INFO(d), role, rlink, ecsa_param) != RTW_PHL_STATUS_SUCCESS) {
		RTW_ERR("CSA : "FUNC_ADPT_FMT" Start PHL ECSA fail\n", FUNC_ADPT_ARG(execute_iface));
		goto err_hdl;
	}

	return true;

err_hdl:
	reset_all_ecsa_param(execute_iface);
	return false;
}

/*
 * @csa_ch_width gets from Channel Switch Wrapper IE
 * 0 for 40 MHz
 * 1 for 80 MHz, 160 MHz or 80+80 MHz
 * 255 for initial value, defined by driver
 *
 * @ecsa_op_class gets from ECSA IE
 */
bool rtw_hal_trigger_csa_start(struct _ADAPTER *a, enum csa_trigger_type trigger_type,
	u8 csa_mode, u8 ecsa_op_class, u8 csa_switch_cnt,
	u8 csa_band, u8 csa_ch, u8 csa_bw, u8 csa_offset,
	u8 csa_ch_width, u8 csa_ch_freq_seg0, u8 csa_ch_freq_seg1)
{
	struct core_ecsa_info *ecsa_info = &(a->ecsa_info);
	struct rtw_phl_ecsa_param *ecsa_param = &(ecsa_info->phl_ecsa_param);

	RTW_INFO("CSA : "FUNC_ADPT_FMT" csa trigger type = %u\n", FUNC_ADPT_ARG(a), trigger_type);
	SET_ECSA_STATE(a, ECSA_ST_SW_START);
	ecsa_param->mode = csa_mode;
	ecsa_param->op_class = ecsa_op_class;
	ecsa_param->count = csa_switch_cnt;
	ecsa_param->new_chan_def.band = csa_band;
	ecsa_param->new_chan_def.chan = csa_ch;
	ecsa_param->new_chan_def.bw = csa_bw;
	ecsa_param->new_chan_def.offset = csa_offset;
	/* The channel width defined in 802.11 spec */
	ecsa_info->channel_width = csa_ch_width;
	ecsa_param->new_chan_def.center_freq1 = csa_ch_freq_seg0;
	ecsa_param->new_chan_def.center_freq2 = csa_ch_freq_seg1;
	ecsa_param->flag = 0;
	ecsa_param->delay_start_ms = 0;

	return rtw_trigger_phl_ecsa_start(a, trigger_type);
}

bool rtw_hal_dfs_trigger_csa(struct _ADAPTER *a,
	enum csa_trigger_type trigger_type,
	u8 band_idx, u8 u_ch, u8 u_bw, u8 u_offset)
{
	struct rf_ctl_t *rfctl = adapter_to_rfctl(a);
	enum band_type band;
	u8 ch, bw, offset;
	bool ch_avail, ret;
	struct rtw_chan_def new_chandef = {0};
	u8 csa_mode, ecsa_op_class, csa_switch_cnt;
	u8 csa_ch_width = 0, csa_ch_freq_seg0 = 0, csa_ch_freq_seg1 = 0;

	#ifndef PRIVATE_R
	/* select new chan_def */
	ch_avail = rtw_rfctl_choose_bchbw(rfctl, BAND_MAX, 0, u_bw, 0,
				BAND_ON_5G, u_ch, u_offset,
				&band, &ch, &bw, &offset,
				false, false, __func__);
	if (!ch_avail)
		return false;
	#else
	/* only select CH36 or CH149 */
	band = BAND_ON_5G;
	ch = rfctl->p2p_park_ch;
	bw = CHANNEL_WIDTH_20;
	offset = CHAN_OFFSET_NO_EXT;
	#endif

	RTW_INFO(FUNC_HWBAND_FMT" CSA to %s-%u,%u,%u\n",
		FUNC_HWBAND_ARG(band_idx), band_str(band), ch, bw, offset);

	csa_mode = 1;
	new_chandef.band = band;
	new_chandef.chan = ch;
	new_chandef.bw = bw;
	new_chandef.offset = offset;
	ecsa_op_class = rtw_phl_get_operating_class(new_chandef);
	csa_switch_cnt = 2;

	ret = rtw_hal_trigger_csa_start(a, CSA_STA_DISCONNECT_ON_DFS,
		csa_mode, ecsa_op_class, csa_switch_cnt,
		band, ch, bw, offset,
		csa_ch_width, csa_ch_freq_seg0, csa_ch_freq_seg1);

	return ret;
}
#endif /* CONFIG_ECSA_PHL */
#endif /* CONFIG_DFS */

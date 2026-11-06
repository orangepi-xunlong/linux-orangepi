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

#ifndef __RTW_TWT_H_
#define __RTW_TWT_H_

#ifdef CONFIG_80211AX_HE
#ifdef CONFIG_TWT

#define TWT_REQ_SUPP(twt_cap) (twt_cap & RTW_PHL_TWT_REQ_SUP)
#define TWT_RSP_SUPP(twt_cap) (twt_cap & RTW_PHL_TWT_RSP_SUP)

#define TWT_NEGO_TYPE(twt_ele) (twt_ele)->twt_ctrl.nego_type
#define TWT_NEGO_TYPE_I(twt_ele) (((twt_ele)->twt_ctrl.nego_type == RTW_PHL_INDIV_TWT) \
				  || ((twt_ele)->twt_ctrl.nego_type == RTW_PHL_WAKE_TBTT_INR))
#define TWT_NEGO_TYPE_B(twt_ele) (((twt_ele)->twt_ctrl.nego_type == RTW_PHL_BCAST_TWT) \
				  || ((twt_ele)->twt_ctrl.nego_type == RTW_PHL_MANAGE_BCAST_TWT))
#define TWT_REQER_I(twt_ele) (twt_ele)->info.i_twt_para_set.req_type.twt_request
#define TWT_REQER_B(twt_ele, bidx) (twt_ele)->info.b_twt_para_set[bidx].req_type.twt_request
#define TWT_SETUP_CMD_I(twt_ele) (twt_ele)->info.i_twt_para_set.req_type.twt_setup_cmd
#define TWT_SETUP_CMD_B(twt_ele, bidx) (twt_ele)->info.b_twt_para_set[bidx].req_type.twt_setup_cmd

enum rtw_core_twt_cmd_id {
	CORE_TWT_CMD_ISETUP,
	CORE_TWT_CMD_BSETUP,
	CORE_TWT_CMD_ITEARDOWN,
	CORE_TWT_CMD_BTEARDOWN,
	CORE_TWT_CMD_SET_INFO,
	CORE_TWT_CMD_ON_ISETUP,
	CORE_TWT_CMD_ON_BSETUP,
	CORE_TWT_CMD_ON_TEARDOWN
};

struct rtw_core_twt_cmd_obj {
	_adapter *padapter;
	struct _ADAPTER_LINK *padapter_link;
	struct sta_info *psta;
	enum rtw_core_twt_cmd_id cmd_id;
	void *pparm;
	u32 parm_sz;
	bool pparm_need_free;
};

struct rtw_core_twt_set_info {
	bool suspend;
	u32 dur;
};

u8 rtw_core_twt_init(_adapter *padapter);
u8 rtw_core_twt_reset(_adapter *padapter, struct _ADAPTER_LINK *padapter_link);
u8 rtw_core_twt_hdl(_adapter *padapter, u8 *pbuf);
u8 rtw_core_twt_cmd(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
		    struct sta_info *psta, enum rtw_core_twt_cmd_id cmd_id,
		    void *pparm, u32 pparm_sz, bool parm_need_free);
u8 rtw_core_twt_set_ctrl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			 u32 ndp_pag_ind, u32 rsp_pm_mode, u32 nego_type,
			 u32 twt_info_dis, u32 wk_dur_unit);
u8 rtw_core_twt_set_ireq(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			 u32 request, u32 setup_cmd, u32 trigger, u32 implicit,
			 u32 flow_type, u32 wk_intvl_exp, u32 protection);
u8 rtw_core_twt_set_isetup(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			   struct sta_info *psta, u32 target_wk_time,
			   u32 nom_min_wk_dur, u32 wk_int_mantissa, u32 twt_chnl);
u8 rtw_core_twt_set_iteardown(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			      struct sta_info *psta, u32 nego_type,
			      u32 flow_id, u32 trdwn_all);
u8 rtw_core_twt_set_breq(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			 u32 request, u32 setup_cmd, u32 trigger, u32 lst_bcparm_set,
			 u32 flow_type, u32 btwt_rcmd, u32 wk_intvl_exp);
u8 rtw_core_twt_set_bsetup(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			   struct sta_info *psta, u32 target_wk_time,
			   u32 nom_min_wk_dur, u32 wk_int_mantissa, u32 btwt_id,
			   u32 btwt_prstnc);
u8 rtw_core_twt_set_bteardown(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			      struct sta_info *psta, u32 nego_type,
			      u32 btwt_id, u32 trdwn_all);
u8 rtw_core_twt_set_info(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			 struct sta_info *psta, bool suspend, u32 dur);
u8 rtw_issue_twt_setup(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
		       u8 *da, struct rtw_phl_twt_setup_info *twt_setup_info);
u8 rtw_issue_twt_teardown(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			  u8 *da, struct rtw_phl_twt_flow_field *twt_flow_info,
			  int try_cnt, int wait_ms);
u8 rtw_issue_twt_teardown_cmn(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			      u8 *da, struct rtw_phl_twt_element *twt_ele,
			      int try_cnt, int wait_ms, bool individual);
u8 rtw_issue_twt_info(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
		       u8 *da, struct rtw_phl_twt_info_f *twt_info);
u8 rtw_core_twt_on_action(_adapter *padapter, u8 *raddr, u8 *pframe_body, u32 frame_len);

#endif /*CONFIG_TWT */
#endif /* CONFIG_80211AX_HE */

#endif /* __RTW_TWT_H_ */

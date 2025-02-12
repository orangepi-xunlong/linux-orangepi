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
#define _RTW_TWT_C_

#include <drv_types.h>

#ifdef CONFIG_80211AX_HE
#ifdef CONFIG_TWT

#if 0
void _twt_debug_print_info(_adapter *padapter, struct rtw_phl_twt_element *twt_element, const char* func_name)
{
	struct rtw_phl_indiv_twt_para_set *twt_para = &twt_element->info.i_twt_para_set;

	RTW_INFO("[TWT] %s - nego_type             = %d\n", func_name, TWT_NEGO_TYPE(twt_element));
	if (TWT_NEGO_TYPE(twt_element) == RTW_PHL_INDIV_TWT ||
		TWT_NEGO_TYPE(twt_element) == RTW_PHL_WAKE_TBTT_INR) {
		RTW_INFO("[TWT] %s - request               = %d\n", func_name, twt_para->req_type.twt_request);
		RTW_INFO("[TWT] %s - twt_setup_cmd         = %d\n", func_name, TWT_I_SETUP_CMD(twt_element));

		RTW_INFO("[TWT] %s - twt_protection        = %d\n", func_name, twt_para->req_type.twt_protection);
		RTW_INFO("[TWT] %s - trigger               = %d\n", func_name, twt_para->req_type.trigger);
		RTW_INFO("[TWT] %s - implicit              = %d\n", func_name, twt_para->req_type.implicit);
		RTW_INFO("[TWT] %s - flow_type             = %d\n", func_name, twt_para->req_type.flow_type);
		RTW_INFO("[TWT] %s - twt_flow_id           = %d\n", func_name, TWT_I_FLOW_ID(twt_element));
		RTW_INFO("[TWT] %s - twt_wake_int_exp      = %d\n", func_name, twt_para->req_type.twt_wake_int_exp);
		RTW_INFO("[TWT] %s - target_wake_t_h       = %u\n", func_name, twt_para->target_wake_t_h);
		RTW_INFO("[TWT] %s - target_wake_t_l       = %u\n", func_name, twt_para->target_wake_t_l);
		RTW_INFO("[TWT] %s - nom_min_twt_wake_dur  = %d\n", func_name, twt_para->nom_min_twt_wake_dur);
		RTW_INFO("[TWT] %s - twt_wake_int_mantissa = %d\n", func_name, twt_para->twt_wake_int_mantissa);
	}
}
#endif

static void _twt_teardown_done(void *priv, struct rtw_phl_stainfo_t *sta,
			       enum rtw_phl_status sts)
{
	if (RTW_PHL_STATUS_SUCCESS == sts || RTW_PHL_STATUS_CMD_SUCCESS == sts )
		RTW_INFO("[TWT] %s: success\n", __func__);
	else
		RTW_INFO("[TWT] %s: fail (%d)\n", __func__, sts);
}

static void _twt_accept_done(void *priv, struct rtw_phl_stainfo_t *sta,
			     struct rtw_phl_twt_setup_info *setup_info,
			     enum rtw_phl_status sts)
{
	struct dvobj_priv * dvobj = priv;
	_adapter *padapter = dvobj_get_primary_adapter(dvobj);
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	struct rtw_phl_twt_element *twt_ele = &setup_info->twt_element;

	if (RTW_PHL_STATUS_SUCCESS == sts || RTW_PHL_STATUS_CMD_SUCCESS == sts ) {
		RTW_INFO("[TWT] %s: success\n", __func__);
		return;
	}

	rtw_issue_twt_teardown_cmn(padapter, padapter_link, sta->mac_addr,
				   twt_ele, 3, 500, TWT_NEGO_TYPE_I(twt_ele));
}

static void _rtw_core_twt_ele_init(struct rtw_phl_twt_element *twt_ele)
{
	_rtw_memset(twt_ele, 0x0, sizeof(struct rtw_phl_twt_element));
}

u8 rtw_core_twt_init(_adapter *padapter)
{
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct he_priv *phepriv = &pmlmepriv->dev_hepriv;

	_rtw_core_twt_ele_init(&phepriv->twt_ele);

	return _SUCCESS;
}

u8 rtw_core_twt_reset(_adapter *padapter, struct _ADAPTER_LINK *padapter_link)
{
	struct he_priv *phepriv = &padapter_link->mlmepriv.hepriv;

	_rtw_core_twt_ele_init(&phepriv->twt_ele);

	return _SUCCESS;
}

u8 rtw_core_twt_isetup_hdl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			   struct sta_info *psta, struct rtw_phl_twt_element *twt_ele)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_phl_indiv_twt_para_set *isetup = &twt_ele->info.i_twt_para_set;
	struct rtw_phl_twt_get_twt_i get_twt_i = {0};
	struct rtw_phl_stainfo_t *phl_sta = psta->phl_sta;
	struct rtw_phl_twt_setup_info *nego_info = &phl_sta->wrole->twt_setup_info;

	if (isetup->req_type.twt_setup_cmd == RTW_PHL_SUGGEST_TWT) {
		get_twt_i.wrole = padapter->phl_role;
		get_twt_i.id = IGNORE_CFG_ID;
		get_twt_i.offset = 3 * 100;
		get_twt_i.tsf_h = &isetup->target_wake_t_h;
		get_twt_i.tsf_l = &isetup->target_wake_t_l;
		if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_get_target_wake_time(GET_PHL_INFO(dvobj),
			&get_twt_i, PHL_CMD_DIRECTLY, 0)) {
			RTW_ERR("[TWT] %s: rtw_phl_twt_get_target_wake_time fail!\n", __func__);
			goto exit;
		}
	} else {
		isetup->target_wake_t_h = 0;
		isetup->target_wake_t_l = 0;
	}

	if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_get_new_flow_id(GET_PHL_INFO(dvobj),
					phl_sta, &isetup->req_type.twt_flow_id)) {
		RTW_ERR("[TWT] %s: rtw_phl_twt_get_new_flow_id fail!\n", __func__);
		goto exit;
	}

	_rtw_memcpy(&nego_info->twt_element, twt_ele, sizeof(struct rtw_phl_twt_element));
	nego_info->dialog_token++;

	RTW_INFO("[TWT] %s: twt_flow_id:%d, dialog_token:%d\n", __func__,
		 isetup->req_type.twt_flow_id, nego_info->dialog_token);

	rtw_issue_twt_setup(padapter, padapter_link, phl_sta->mac_addr, nego_info);

	return _SUCCESS;

exit:
	rtw_core_twt_reset(padapter, padapter_link);

	return _FAIL;
}

u8 rtw_core_twt_iteardown_hdl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			      struct sta_info *psta,
			      struct rtw_phl_twt_sta_teardown_i *twt_trdwn_cfg)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_phl_stainfo_t *phl_sta = psta->phl_sta;
	u8 ret = _SUCCESS;

	if (_SUCCESS != rtw_issue_twt_teardown(padapter, padapter_link, phl_sta->mac_addr,
		&twt_trdwn_cfg->twt_flow, 3, 500)) {
		RTW_ERR("[TWT] %s: rtw_issue_twt_teardown fail!\n", __func__);
		ret = _FAIL;
	}

	if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_teardown_for_sta_mode(GET_PHL_INFO(dvobj),
		twt_trdwn_cfg)) {
		RTW_ERR("[TWT] %s: rtw_phl_twt_teardown_for_sta_mode fail!\n", __func__);
		ret = _FAIL;
	}

	rtw_core_twt_reset(padapter, padapter_link);
	return ret;
}

u8 rtw_core_twt_bsetup_hdl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			   struct sta_info *psta, struct rtw_phl_twt_element *twt_ele)
{
	struct rtw_phl_bcast_twt_para_set *bsetup = &twt_ele->info.b_twt_para_set[0];
	struct rtw_phl_stainfo_t *phl_sta = psta->phl_sta;
	struct rtw_phl_twt_setup_info *nego_info = &phl_sta->wrole->twt_setup_info;

	_rtw_memcpy(&nego_info->twt_element, twt_ele, sizeof(struct rtw_phl_twt_element));
	nego_info->dialog_token++;

	RTW_INFO("[TWT] %s: btwt_id %d, dialog_token %d\n", __func__,
		 bsetup->btwt_i.btwt_id, nego_info->dialog_token);

	rtw_issue_twt_setup(padapter, padapter_link, phl_sta->mac_addr, nego_info);

	return _SUCCESS;
}

u8 rtw_core_twt_bteardown_hdl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			      struct sta_info *psta,
			      struct rtw_phl_twt_sta_teardown_i *twt_trdwn_cfg)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_phl_stainfo_t *phl_sta = psta->phl_sta;
	u8 ret = _SUCCESS;

	if (_SUCCESS != rtw_issue_twt_teardown(padapter, padapter_link, phl_sta->mac_addr,
		&twt_trdwn_cfg->twt_flow, 3, 500)) {
		RTW_ERR("[TWT] %s: rtw_issue_twt_teardown fail!\n", __func__);
		ret = _FAIL;
	}

	if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_teardown_for_sta_mode(GET_PHL_INFO(dvobj),
		twt_trdwn_cfg)) {
		RTW_ERR("[TWT] %s: rtw_phl_twt_teardown_for_sta_mode fail!\n", __func__);
		ret = _FAIL;
	}

	rtw_core_twt_reset(padapter, padapter_link);
	return ret;
}

u8 rtw_core_twt_set_info_hdl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			     struct sta_info *psta,
			     struct rtw_core_twt_set_info *twt_info)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_phl_stainfo_t *phl_sta = psta->phl_sta;
	struct rtw_phl_twt_info_f_hdr_i info_hdr = {0};
	struct rtw_phl_twt_info_f *info_f = &info_hdr.info_f;
	u8 used_map = 0, i = 0, id = 0;

	rtw_phl_twt_get_all_id(GET_PHL_INFO(dvobj), phl_sta, RTW_PHL_INDIV_TWT, &used_map);
	while ((used_map >> i) > 0) {
		if ((used_map >> i) & BIT0) {
			id = i;
			break;
		}
		i++;
	}

	info_f->twt_flow_id = id;

	if (twt_info->suspend == _FALSE) {
		struct rtw_phl_twt_get_twt_i get_twt_i = {0};
		u8 cfg_id = 0;
		u32 tsf_h = 0, tsf_l = 0;

		if (!rtw_phl_twt_get_cfg_id(GET_PHL_INFO(dvobj), phl_sta,
			RTW_PHL_INDIV_TWT, info_hdr.info_f.twt_flow_id, &cfg_id)) {
			RTW_ERR("[TWT] %s: fail to get cfg_id by macid %d and twt_flow_id %d!\n",
				__func__, phl_sta->macid, info_hdr.info_f.twt_flow_id);
			return _FAIL;
		}
		get_twt_i.offset = twt_info->dur;
		get_twt_i.wrole = padapter->phl_role;
		get_twt_i.id = cfg_id;
		get_twt_i.tsf_h = &tsf_h;
		get_twt_i.tsf_l = &tsf_l;
		if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_get_target_wake_time(GET_PHL_INFO(dvobj),
			&get_twt_i, PHL_CMD_DIRECTLY, 0)) {
			RTW_ERR("[TWT] %s: rtw_phl_twt_get_target_wake_time fail!\n", __func__);
			return _FAIL;
		}
		info_f->next_twt = tsf_h;
		info_f->next_twt = (info_f->next_twt << 32);
		info_f->next_twt |= tsf_l;
		info_f->next_twt_size = 3;
	}

	info_hdr.sta = phl_sta;

	RTW_INFO("[TWT] %s: twt_flow_id %d, rsp_req %d, next_twt_req %d, next_twt_size %d, all_twt %d, next_twt 0x%08X %08X\n",
		__func__, info_f->twt_flow_id, info_f->rsp_req, info_f->next_twt_req,
		info_f->next_twt_size, info_f->all_twt, (u32)(info_f->next_twt >> 32),
		(u32)(info_f->next_twt));

	if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_info_f_hrl(GET_PHL_INFO(dvobj), &info_hdr)) {
		RTW_ERR("[TWT] %s: rtw_phl_twt_info_f_hrl fail!\n", __func__);
		return _FAIL;
	}

	return rtw_issue_twt_info(padapter, padapter_link, phl_sta->mac_addr, info_f);
}

u8 rtw_core_twt_on_isetup_hdl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			     struct sta_info *psta, struct rtw_phl_twt_setup_info *twt_setup_info)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_phl_stainfo_t *phl_sta = psta->phl_sta;
	struct rtw_phl_twt_element *twt_ele = &twt_setup_info->twt_element;
	struct rtw_phl_twt_setup_info *nego_info = &phl_sta->wrole->twt_setup_info;
	struct rtw_phl_twt_sta_accept_i acpt_i = {0};

	if (TWT_REQER_I(twt_ele)) {
		RTW_ERR("[TWT] %s: not support to be TWT responder in STA mode!\n", __func__);
		goto twt_setup_fail;
	}

	if (!twt_setup_info->dialog_token) {
		RTW_ERR("[TWT] %s: invalid dialog token 0!\n", __func__);
		goto twt_setup_fail;
	}

	if ((twt_setup_info->dialog_token != nego_info->dialog_token)) {
		RTW_ERR("[TWT] %s: dialog token mismatch (tx %d, rx %d)!\n", __func__,
			nego_info->dialog_token, twt_setup_info->dialog_token);
		goto twt_setup_fail;
	}

	if (TWT_SETUP_CMD_I(twt_ele) != RTW_PHL_ACCEPT_TWT) {
		RTW_ERR("[TWT] %s: can't support setup cmd %d currently!\n", __func__,
			TWT_SETUP_CMD_I(twt_ele));
		goto twt_setup_fail;
	}

	_rtw_memcpy(&acpt_i.setup_info, twt_setup_info, sizeof(struct rtw_phl_twt_setup_info));
	acpt_i.sta = phl_sta;
	acpt_i.accept_done = _twt_accept_done;

	if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_accept_for_sta_mode(GET_PHL_INFO(dvobj),
		&acpt_i)) {
		RTW_ERR("[TWT] %s: rtw_phl_twt_accept_for_sta_mode fail!\n", __func__);
		goto twt_setup_fail;
	}

	return _SUCCESS;

twt_setup_fail:
	rtw_issue_twt_teardown_cmn(padapter, padapter_link, phl_sta->mac_addr,
				   twt_ele, 3, 500, TWT_NEGO_TYPE_I(twt_ele));

	return _FAIL;
}

u8 rtw_core_twt_on_bsetup_hdl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			     struct sta_info *psta, struct rtw_phl_twt_setup_info *twt_setup_info)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_phl_stainfo_t *phl_sta = psta->phl_sta;
	struct rtw_phl_twt_element *twt_ele = &twt_setup_info->twt_element;
	struct rtw_phl_twt_setup_info *nego_info = &phl_sta->wrole->twt_setup_info;
	struct rtw_phl_bcast_twt_para_set *para_src = NULL, *para_dst = NULL;
	struct rtw_phl_twt_sta_accept_i acpt_i = {0};
	u8 idx = 0, acpt_num = 0;

	if (!twt_setup_info->dialog_token) {
		RTW_ERR("[TWT] %s: invalid dialog token 0!\n", __func__);
		goto twt_setup_fail;
	}

	if ((twt_setup_info->dialog_token != nego_info->dialog_token)) {
		RTW_ERR("[TWT] %s: dialog token mismatch (tx %d, rx %d)!\n", __func__,
			nego_info->dialog_token, twt_setup_info->dialog_token);
		goto twt_setup_fail;
	}

	for (idx = 0; idx < twt_ele->num_btwt_para; idx++) {
		if (TWT_REQER_B(twt_ele, idx))
			continue;

		if (TWT_SETUP_CMD_B(twt_ele, idx) != RTW_PHL_ACCEPT_TWT)
			continue;

		para_src = &twt_ele->info.b_twt_para_set[idx];
		para_dst = &acpt_i.setup_info.twt_element.info.b_twt_para_set[acpt_num];
		_rtw_memcpy(para_dst, para_src, sizeof(struct rtw_phl_bcast_twt_para_set));
		acpt_num++;
	}

	if (!acpt_num) {
		RTW_ERR("[TWT] %s: no btwt para set is accepted!\n", __func__);
		goto twt_setup_fail;
	}

	_rtw_memcpy(&acpt_i.setup_info.twt_element.twt_ctrl, &twt_ele->twt_ctrl,
		    sizeof(struct rtw_phl_twt_control));
	acpt_i.setup_info.dialog_token = twt_setup_info->dialog_token;
	acpt_i.sta = phl_sta;
	acpt_i.accept_done = _twt_accept_done;

	if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_accept_for_sta_mode(GET_PHL_INFO(dvobj),
		&acpt_i)) {
		RTW_ERR("[TWT] %s: rtw_phl_twt_accept_for_sta_mode fail!\n", __func__);
		goto twt_setup_fail;
	}

	return _SUCCESS;

twt_setup_fail:
	rtw_issue_twt_teardown_cmn(padapter, padapter_link, phl_sta->mac_addr,
				   twt_ele, 3, 500, TWT_NEGO_TYPE_I(twt_ele));

	return _FAIL;
}

u8 rtw_core_twt_on_teardown_hdl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			     struct sta_info *psta, struct rtw_phl_twt_flow_field *twt_flow_info)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_phl_twt_sta_teardown_i teardown_cfg = {0};

	teardown_cfg.sta = psta->phl_sta;
	_rtw_memcpy(&teardown_cfg.twt_flow, &twt_flow_info, sizeof(struct rtw_phl_twt_flow_field));
	teardown_cfg.teardown_done = _twt_teardown_done;

	if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_teardown_for_sta_mode(GET_PHL_INFO(dvobj),
		&teardown_cfg)) {
		RTW_ERR("[TWT] %s: rtw_phl_twt_teardown_for_sta_mode fail!\n", __func__);
		return _FAIL;
	}

	return _SUCCESS;
}

u8 rtw_core_twt_hdl(_adapter *padapter, u8 *pbuf)
{
	struct rtw_core_twt_cmd_obj *ptwtcmd = NULL;
	u8 ret = _FAIL;

	if (!pbuf)
		return _FALSE;

	ptwtcmd = (struct rtw_core_twt_cmd_obj*)pbuf;

	switch (ptwtcmd->cmd_id) {
	case CORE_TWT_CMD_ISETUP:
		ret = rtw_core_twt_isetup_hdl(ptwtcmd->padapter, ptwtcmd->padapter_link,
			ptwtcmd->psta, (struct rtw_phl_twt_element*)ptwtcmd->pparm);
		break;
	case CORE_TWT_CMD_BSETUP:
		ret = rtw_core_twt_bsetup_hdl(ptwtcmd->padapter, ptwtcmd->padapter_link,
			ptwtcmd->psta, (struct rtw_phl_twt_element*)ptwtcmd->pparm);
		break;
	case CORE_TWT_CMD_ITEARDOWN:
		ret = rtw_core_twt_iteardown_hdl(ptwtcmd->padapter, ptwtcmd->padapter_link,
			ptwtcmd->psta, (struct rtw_phl_twt_sta_teardown_i*)ptwtcmd->pparm);
		break;
	case CORE_TWT_CMD_BTEARDOWN:
		ret = rtw_core_twt_bteardown_hdl(ptwtcmd->padapter, ptwtcmd->padapter_link,
			ptwtcmd->psta, (struct rtw_phl_twt_sta_teardown_i*)ptwtcmd->pparm);
		break;
	case CORE_TWT_CMD_SET_INFO:
		ret = rtw_core_twt_set_info_hdl(ptwtcmd->padapter, ptwtcmd->padapter_link,
			ptwtcmd->psta, (struct rtw_core_twt_set_info*)ptwtcmd->pparm);
		break;
	case CORE_TWT_CMD_ON_ISETUP:
		ret = rtw_core_twt_on_isetup_hdl(ptwtcmd->padapter, ptwtcmd->padapter_link,
			ptwtcmd->psta, (struct rtw_phl_twt_setup_info*)ptwtcmd->pparm);
		break;
	case CORE_TWT_CMD_ON_BSETUP:
		ret = rtw_core_twt_on_bsetup_hdl(ptwtcmd->padapter, ptwtcmd->padapter_link,
			ptwtcmd->psta, (struct rtw_phl_twt_setup_info*)ptwtcmd->pparm);
		break;
	case CORE_TWT_CMD_ON_TEARDOWN:
		ret = rtw_core_twt_on_teardown_hdl(ptwtcmd->padapter, ptwtcmd->padapter_link,
			ptwtcmd->psta, (struct rtw_phl_twt_flow_field*)ptwtcmd->pparm);
		break;
	}

	if (ptwtcmd->pparm_need_free == _TRUE)
		rtw_mfree(ptwtcmd->pparm, ptwtcmd->parm_sz);

	return ret;
}

u8 rtw_core_twt_cmd(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
		    struct sta_info *psta, enum rtw_core_twt_cmd_id cmd_id,
		    void *pparm, u32 pparm_sz, bool parm_need_free)
{
	struct	cmd_obj	*pcmdobj;
	struct	rtw_core_twt_cmd_obj *ptwtcmd;
	struct	mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct	cmd_priv *pcmdpriv = &adapter_to_dvobj(padapter)->cmdpriv;
	u8 res = _SUCCESS;

	pcmdobj = (struct cmd_obj *)rtw_zmalloc(sizeof(struct cmd_obj));
	if (pcmdobj == NULL) {
		res = _FAIL;
		goto exit;
	}
	pcmdobj->padapter = padapter;

	ptwtcmd = (struct rtw_core_twt_cmd_obj*)rtw_zmalloc(sizeof(struct rtw_core_twt_cmd_obj));
	if (ptwtcmd == NULL) {
		rtw_mfree((u8 *)pcmdobj, sizeof(struct cmd_obj));
		res = _FAIL;
		goto exit;
	}
	_rtw_memset(ptwtcmd, 0x0, sizeof(struct	rtw_core_twt_cmd_obj));

	ptwtcmd->padapter = padapter;
	ptwtcmd->padapter_link = padapter_link;
	ptwtcmd->psta = psta;
	ptwtcmd->cmd_id = cmd_id;
	ptwtcmd->pparm = pparm;
	ptwtcmd->parm_sz = pparm_sz;
	ptwtcmd->pparm_need_free = parm_need_free;

	init_h2fwcmd_w_parm_no_rsp(pcmdobj, ptwtcmd, CMD_TWT);
	res = rtw_enqueue_cmd(pcmdpriv, pcmdobj);

exit:
	return res;
}

u8 rtw_core_twt_set_ctrl(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			 u32 ndp_pag_ind, u32 rsp_pm_mode, u32 nego_type,
			 u32 twt_info_dis, u32 wk_dur_unit)
{
	struct rtw_phl_twt_element *twt_ele = &padapter_link->mlmepriv.hepriv.twt_ele;

	twt_ele->twt_ctrl.ndp_paging_indic = ndp_pag_ind;
	twt_ele->twt_ctrl.responder_pm_mode = rsp_pm_mode;
	twt_ele->twt_ctrl.nego_type = nego_type;
	twt_ele->twt_ctrl.twt_info_frame_disable = twt_info_dis;
	twt_ele->twt_ctrl.wake_dur_unit = wk_dur_unit;

	RTW_INFO("[TWT] %s: ndp_pag_ind %d, rsp_pm_md %d, ng_type %d, twt_info_dis %d, wk_dur_unt %d\n",
		 __func__, ndp_pag_ind, rsp_pm_mode, nego_type, twt_info_dis, wk_dur_unit);

	return _SUCCESS;
}

u8 rtw_core_twt_set_ireq(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			 u32 request, u32 setup_cmd, u32 trigger, u32 implicit,
			 u32 flow_type, u32 wk_intvl_exp, u32 protection)
{
	struct rtw_phl_twt_element *twt_ele = &padapter_link->mlmepriv.hepriv.twt_ele;
	struct rtw_phl_req_type_indiv *ireq = &twt_ele->info.i_twt_para_set.req_type;

	ireq->twt_request = request;
	ireq->twt_setup_cmd = setup_cmd;
	ireq->trigger = trigger;
	ireq->implicit = implicit;
	ireq->flow_type = flow_type;
	ireq->twt_wake_int_exp = wk_intvl_exp;
	ireq->twt_protection = protection;

	RTW_INFO("[TWT] %s: req %d, stp_cmd %d, trig %d, implct %d, flow_type %d, wk_intvl_exp %d protct %d\n",
		 __func__, request, setup_cmd, trigger, implicit, flow_type, wk_intvl_exp,
		 protection);

	return _SUCCESS;
}

u8 rtw_core_twt_set_isetup(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			   struct sta_info *psta, u32 target_wk_time,
			   u32 nom_min_wk_dur, u32 wk_int_mantissa, u32 twt_chnl)
{
	struct rtw_phl_twt_element *twt_ele = &padapter_link->mlmepriv.hepriv.twt_ele;
	struct rtw_phl_indiv_twt_para_set *isetup = &twt_ele->info.i_twt_para_set;

	isetup->nom_min_twt_wake_dur = nom_min_wk_dur;
	isetup->twt_wake_int_mantissa = wk_int_mantissa;
	isetup->twt_channel = twt_chnl;

	RTW_INFO("[TWT] %s: trgt_wk_t %d, nom_min_twt_wk_dur %d, twt_wk_int_mantissa %d, twt_chnl %d\n",
		 __func__, target_wk_time, nom_min_wk_dur, wk_int_mantissa, twt_chnl);

	return rtw_core_twt_cmd(padapter, padapter_link, psta, CORE_TWT_CMD_ISETUP,
				(void*)twt_ele, sizeof(struct rtw_phl_twt_element),
				_FALSE);
}

u8 rtw_core_twt_set_iteardown(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			      struct sta_info *psta, u32 nego_type,
			      u32 flow_id, u32 trdwn_all)
{
	struct rtw_phl_twt_sta_teardown_i *twt_trdwn_cfg = NULL;

	twt_trdwn_cfg = (struct rtw_phl_twt_sta_teardown_i*)rtw_zmalloc(
		sizeof(struct rtw_phl_twt_sta_teardown_i));
	if (twt_trdwn_cfg == NULL)
		return _FAIL;

	twt_trdwn_cfg->twt_flow.nego_type = nego_type;
	twt_trdwn_cfg->twt_flow.info.twt_flow01.twt_flow_id = flow_id;
	twt_trdwn_cfg->twt_flow.info.twt_flow01.teardown_all = trdwn_all;
	twt_trdwn_cfg->sta = psta->phl_sta;
	twt_trdwn_cfg->teardown_done = _twt_teardown_done;

	RTW_INFO("[TWT] %s: nego_type %d, flow_id %d, trdwn_all %d\n", __func__,
		 nego_type, flow_id, trdwn_all);

	return rtw_core_twt_cmd(padapter, padapter_link, psta, CORE_TWT_CMD_ITEARDOWN,
				(void*)twt_trdwn_cfg, sizeof(struct rtw_phl_twt_sta_teardown_i),
				_TRUE);
}

u8 rtw_core_twt_set_breq(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			 u32 request, u32 setup_cmd, u32 trigger, u32 lst_bcparm_set,
			 u32 flow_type, u32 btwt_rcmd, u32 wk_intvl_exp)
{
	struct rtw_phl_twt_element *twt_ele = &padapter_link->mlmepriv.hepriv.twt_ele;
	struct rtw_phl_btwt_req_type *breq = &twt_ele->info.b_twt_para_set[0].req_type;

	breq->twt_request = request;
	breq->twt_setup_cmd = setup_cmd;
	breq->trigger = trigger;
	breq->lst_bc_para_set = lst_bcparm_set;
	breq->flow_type = flow_type;
	breq->btwt_rcmd = btwt_rcmd;
	breq->twt_wake_int_exp = wk_intvl_exp;

	RTW_INFO("[TWT] %s: req %d, stp_cmd %d, trig %d, lst_bcparm_set %d, flow_type %d, btwt_rcmd %d, wk_intvl_exp %d\n",
		 __func__, request, setup_cmd, trigger, lst_bcparm_set, flow_type, btwt_rcmd, wk_intvl_exp);

	return _SUCCESS;
}

u8 rtw_core_twt_set_bsetup(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			   struct sta_info *psta, u32 target_wk_time,
			   u32 nom_min_wk_dur, u32 wk_int_mantissa, u32 btwt_id,
			   u32 btwt_prstnc)
{
	struct rtw_phl_twt_element *twt_ele = &padapter_link->mlmepriv.hepriv.twt_ele;
	struct rtw_phl_bcast_twt_para_set *bsetup = &twt_ele->info.b_twt_para_set[0];

	twt_ele->num_btwt_para = 1;
	bsetup->target_wake_t_l = target_wk_time;
	bsetup->nom_min_twt_wake_dur = nom_min_wk_dur;
	bsetup->twt_wake_int_mantissa = wk_int_mantissa;
	bsetup->btwt_i.btwt_id = btwt_id;
	bsetup->btwt_i.btwt_prstnc = btwt_prstnc;

	RTW_INFO("[TWT] %s: trgt_wk_t %d, nom_min_twt_wk_dur %d, twt_wk_int_mantissa %d, btwt_id %d btwt_prstnc %d\n",
		 __func__, target_wk_time, nom_min_wk_dur, wk_int_mantissa, btwt_id, btwt_prstnc);

	return rtw_core_twt_cmd(padapter, padapter_link, psta, CORE_TWT_CMD_BSETUP,
				(void*)twt_ele, sizeof(struct rtw_phl_twt_element),
				_FALSE);
}

u8 rtw_core_twt_set_bteardown(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			      struct sta_info *psta, u32 nego_type,
			      u32 btwt_id, u32 trdwn_all)
{
	struct rtw_phl_twt_sta_teardown_i *twt_trdwn_cfg = NULL;

	twt_trdwn_cfg = (struct rtw_phl_twt_sta_teardown_i*)rtw_zmalloc(
		sizeof(struct rtw_phl_twt_sta_teardown_i));
	if (twt_trdwn_cfg == NULL)
		return _FAIL;

	twt_trdwn_cfg->twt_flow.nego_type = nego_type;
	twt_trdwn_cfg->twt_flow.info.twt_flow3.bcast_twt_id = btwt_id;
	twt_trdwn_cfg->twt_flow.info.twt_flow3.teardown_all = trdwn_all;
	twt_trdwn_cfg->sta = psta->phl_sta;
	twt_trdwn_cfg->teardown_done = _twt_teardown_done;

	RTW_INFO("[TWT] %s: nego_type %d, btwt_id %d, trdwn_all %d\n", __func__,
		 nego_type, btwt_id, trdwn_all);

	return rtw_core_twt_cmd(padapter, padapter_link, psta, CORE_TWT_CMD_BTEARDOWN,
				(void*)twt_trdwn_cfg, sizeof(struct rtw_phl_twt_sta_teardown_i),
				_TRUE);
}

u8 rtw_core_twt_set_info(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			 struct sta_info *psta, bool suspend, u32 dur)
{
	struct rtw_core_twt_set_info *twt_info = NULL;

	twt_info = (struct rtw_core_twt_set_info*)rtw_zmalloc(
		sizeof(struct rtw_core_twt_set_info));
	if (twt_info == NULL)
		return _FAIL;

	twt_info->suspend = suspend;
	twt_info->dur = dur;

	RTW_INFO("[TWT] %s: suspend %d, dur %d\n", __func__, suspend, dur);

	return rtw_core_twt_cmd(padapter, padapter_link, psta, CORE_TWT_CMD_SET_INFO,
				(void*)twt_info, sizeof(struct rtw_core_twt_set_info),
				_TRUE);
}

u8 rtw_issue_twt_setup(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
		       u8 *da, struct rtw_phl_twt_setup_info *twt_setup_info)
{
	enum rtw_phl_status status = RTW_PHL_STATUS_SUCCESS;
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	u8					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	u8					*mac, *bssid;
	struct link_mlme_ext_priv	*pmlmeext = &(padapter_link->mlmeextpriv);
	struct link_mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*cur_network = &(pmlmeinfo->network);
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
	u8 len = 0;
	u8 category = RTW_WLAN_CATEGORY_UNP_S1G;
	u8 action = RTW_WLAN_ACTION_UNP_S1G_TWT_SETUP;
	u8 twt_element_id = WLAN_EID_TWT;

	if (da == NULL)
		return _FAIL;

	if (alink_is_tx_blocked_by_ch_waiting(padapter_link))
		return _FAIL;

	RTW_INFO("[TWT] %s: issue TWT Setup action frame to "MAC_FMT"\n",
	      __func__, MAC_ARG(da));

	pmgntframe = alloc_mgtxmitframe(pxmitpriv);
	if (pmgntframe == NULL) {
		RTW_INFO("[TWT] %s: alloc_mgtxmitframe fail\n", __func__);
		return _FAIL;
	}

	/* update attribute */
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, padapter_link, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	mac = adapter_mac_addr(padapter);
	bssid = cur_network->MacAddress;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_rtw_memcpy(pwlanhdr->addr1, da, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, bssid, ETH_ALEN);

	SetSeqNum(pwlanhdr, padapter->mlmeextpriv.mgnt_seq);
	padapter->mlmeextpriv.mgnt_seq++;
	set_frame_sub_type(fctrl, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(twt_setup_info->dialog_token), &(pattrib->pktlen));

	/* fill TWT element id */
	pframe = rtw_set_fixed_ie(pframe, 1, &(twt_element_id), &(pattrib->pktlen));

	/* fill TWT element body */
	status = rtw_phl_twt_fill_twt_element(&twt_setup_info->twt_element, pframe + 1, &len);

	/* fill TWT element length */
	*pframe = len;

	pframe += len + 1;
	pattrib->pktlen += len + 1;

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

	return _SUCCESS;
}

static u8 _issue_twt_teardown(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			      u8 *da, struct rtw_phl_twt_flow_field *twt_flow_info,
			      int wait_ms)
{
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	u8					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	u8					*mac, *bssid;
	struct link_mlme_ext_priv	*pmlmeext = &(padapter_link->mlmeextpriv);
	struct link_mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*cur_network = &(pmlmeinfo->network);
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
	u16 len = 0;
	u8 category = RTW_WLAN_CATEGORY_UNP_S1G;
	u8 action = RTW_WLAN_ACTION_UNP_S1G_TWT_TEARDOWN;
	enum rtw_phl_status status = RTW_PHL_STATUS_SUCCESS;
	u8 ret = _FAIL;

	pmgntframe = alloc_mgtxmitframe(pxmitpriv);
	if (pmgntframe == NULL) {
		RTW_INFO("[TWT] %s - alloc_mgtxmitframe fail\n", __func__);
		return _FAIL;
	}

	/* update attribute */
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, padapter_link, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	mac = adapter_mac_addr(padapter);
	bssid = cur_network->MacAddress;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_rtw_memcpy(pwlanhdr->addr1, da, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, bssid, ETH_ALEN);

	SetSeqNum(pwlanhdr, padapter->mlmeextpriv.mgnt_seq);
	padapter->mlmeextpriv.mgnt_seq++;
	set_frame_sub_type(fctrl, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));

	/* fill TWT flow body */
	status = rtw_phl_twt_fill_flow_field(twt_flow_info, pframe, &len);

	pframe += len;
	pattrib->pktlen += len;

	pattrib->last_txcmdsz = pattrib->pktlen;

	if (wait_ms) {
		ret = dump_mgntframe_and_wait_ack_timeout(padapter, pmgntframe, wait_ms);
	} else {
		dump_mgntframe(padapter, pmgntframe);
		ret = _SUCCESS;
	}

	return ret;
}

u8 rtw_issue_twt_teardown(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			  u8 *da, struct rtw_phl_twt_flow_field *twt_flow_info,
			  int try_cnt, int wait_ms)
{
	systime start = rtw_get_current_time();
	int i = 0;
	u8 ret = _FAIL;

	if (da == NULL)
		return _FAIL;

	if (alink_is_tx_blocked_by_ch_waiting(padapter_link))
		return _FAIL;

	RTW_INFO("[TWT] %s: issue TWT teardown frame to "MAC_FMT"\n",
		 __func__, MAC_ARG(da));

	do {
		ret = _issue_twt_teardown(padapter, padapter_link, da, twt_flow_info,
					  wait_ms);

		i++;

		if (RTW_CANNOT_RUN(adapter_to_dvobj(padapter)))
			break;

		if (i < try_cnt && wait_ms > 0 && ret == _FAIL)
			rtw_msleep_os(10);

	} while ((i < try_cnt) && ((ret == _FAIL) || (wait_ms == 0)));

	if (ret != _FAIL) {
		ret = _SUCCESS;
#if 0
#ifndef DBG_XMIT_ACK
		goto exit;
#endif
#endif
	}

	if (try_cnt && wait_ms) {
		RTW_INFO("[TWT] "FUNC_ADPT_FMT" to "MAC_FMT", %s, %d/%d in %u ms\n",
			 FUNC_ADPT_ARG(padapter), MAC_ARG(da), ret == _SUCCESS ?
			 ", acked" : ", not acked", i, try_cnt,
			 rtw_get_passing_time_ms(start));
	}
exit:
	return ret;
}

u8 rtw_issue_twt_teardown_cmn(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			      u8 *da, struct rtw_phl_twt_element *twt_ele,
			      int try_cnt, int wait_ms, bool individual)
{
	struct rtw_phl_twt_flow_field trdwn_info = {0};

	trdwn_info.nego_type = twt_ele->twt_ctrl.nego_type;

	if (individual) {
		trdwn_info.info.twt_flow01.twt_flow_id =
			twt_ele->info.i_twt_para_set.req_type.twt_flow_id;
	} else {
		trdwn_info.info.twt_flow3.bcast_twt_id = 0;
		trdwn_info.info.twt_flow3.teardown_all = 1;
	}

	return rtw_issue_twt_teardown(padapter, padapter_link, da, &trdwn_info,
				      try_cnt, wait_ms);
}

u8 rtw_issue_twt_info(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
		       u8 *da, struct rtw_phl_twt_info_f *twt_info)
{
	enum rtw_phl_status status = RTW_PHL_STATUS_SUCCESS;
	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	u8					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	u8					*mac, *bssid;
	struct link_mlme_ext_priv	*pmlmeext = &(padapter_link->mlmeextpriv);
	struct link_mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*cur_network = &(pmlmeinfo->network);
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
	u16 len = 0;
	u8 category = RTW_WLAN_CATEGORY_UNP_S1G;
	u8 action = RTW_WLAN_ACTION_UNP_S1G_TWT_INFO;

	if (da == NULL)
		return _FAIL;

	if (alink_is_tx_blocked_by_ch_waiting(padapter_link))
		return _FAIL;

	RTW_INFO("[TWT] %s: issue TWT Info action frame to "MAC_FMT"\n",
	      __func__, MAC_ARG(da));

	pmgntframe = alloc_mgtxmitframe(pxmitpriv);
	if (pmgntframe == NULL) {
		RTW_INFO("[TWT] %s: alloc_mgtxmitframe fail\n", __func__);
		return _FAIL;
	}

	/* update attribute */
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, padapter_link, pattrib);

	_rtw_memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	mac = adapter_mac_addr(padapter);
	bssid = cur_network->MacAddress;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_rtw_memcpy(pwlanhdr->addr1, da, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, bssid, ETH_ALEN);

	SetSeqNum(pwlanhdr, padapter->mlmeextpriv.mgnt_seq);
	padapter->mlmeextpriv.mgnt_seq++;
	set_frame_sub_type(fctrl, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));

	/* fill TWT Info body */
	status = rtw_phl_twt_fill_info_field(twt_info, pframe, &len);

	pframe += len;
	pattrib->pktlen += len;

	pattrib->last_txcmdsz = pattrib->pktlen;

	dump_mgntframe(padapter, pmgntframe);

	return _SUCCESS;
}

static u8 _twt_on_setup_sta(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			    u8 *raddr, u8 *ptwt_ie, u32 frame_len)
{
	struct sta_info *psta;
	struct rtw_phl_twt_setup_info *twt_setup_info = NULL;
	struct rtw_phl_twt_element *twt_ele = NULL;
	u16 len = frame_len;

	RTW_INFO("[TWT] %s: Recv TWT setup frame: mac_addr = " MAC_FMT ", diag_token = %d, len = %d\n",
		 __func__, MAC_ARG(raddr), ptwt_ie[2], len);

	psta = rtw_get_stainfo(&padapter->stapriv, raddr);
	if (psta == NULL || psta->phl_sta == NULL) {
		RTW_ERR("[TWT] %s: STA not exist!\n", __func__);
		goto twt_setup_fail;
	}

	twt_setup_info = (struct rtw_phl_twt_setup_info*)rtw_zmalloc(
		sizeof(struct rtw_phl_twt_setup_info));
	if (twt_setup_info == NULL) {
		RTW_ERR("[TWT] %s: malloc fail!\n", __func__);
		goto twt_setup_fail;
	}

	twt_ele = &twt_setup_info->twt_element;

	/* parse TWT setup frame content */
	if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_parse_setup_info(ptwt_ie, len,
		twt_setup_info)) {
		RTW_ERR("[TWT] %s: rtw_phl_parse_twt_setup_info fail!\n", __func__);
		goto twt_setup_fail;
	}

	/* handle itwt case and btwt case seperately */
	if (_SUCCESS != rtw_core_twt_cmd(padapter, padapter_link, psta,
			(TWT_NEGO_TYPE_I(twt_ele)) ? CORE_TWT_CMD_ON_ISETUP :
			CORE_TWT_CMD_ON_BSETUP, (void*)twt_setup_info,
			sizeof(struct rtw_phl_twt_setup_info), _TRUE)) {
			RTW_ERR("[TWT] %s: rtw_core_twt_cmd fail!\n", __func__);
			goto twt_setup_fail;
		}

	return _SUCCESS;

twt_setup_fail:
	rtw_issue_twt_teardown_cmn(padapter, padapter_link, psta->phl_sta->mac_addr,
				   twt_ele, 0, 0, TWT_NEGO_TYPE_I(twt_ele));

	return _FAIL;
}

static u8 _twt_on_teardown_sta(_adapter *padapter, struct _ADAPTER_LINK *padapter_link,
			       u8 *raddr, u8 *pframe, u32 frame_len)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_phl_twt_flow_field *twt_flow_info = NULL;
	struct sta_info *psta;
	u16 len = frame_len;

	RTW_INFO("[TWT] %s: Recv TWT flow frame, mac_addr = " MAC_FMT "\n",
		 __func__, MAC_ARG(raddr));

	psta = rtw_get_stainfo(&padapter->stapriv, raddr);
	if (psta == NULL || psta->phl_sta == NULL) {
		RTW_INFO("[TWT] %s: STA not exist!\n", __func__);
		return _FAIL;
	}

	twt_flow_info = (struct rtw_phl_twt_flow_field*)rtw_zmalloc(
		sizeof(struct rtw_phl_twt_flow_field));
	if (twt_flow_info == NULL) {
		RTW_ERR("[TWT] %s: malloc fail!\n", __func__);
		return _FAIL;
	}

	if (RTW_PHL_STATUS_SUCCESS != rtw_phl_twt_parse_flow_field(pframe + 2, len,
		twt_flow_info)) {
		RTW_ERR("[TWT] %s: rtw_phl_parse_twt_flow_field fail!\n", __func__);
		return _FAIL;
	}

	if (_SUCCESS != rtw_core_twt_cmd(padapter, padapter_link, psta,
			CORE_TWT_CMD_ON_TEARDOWN, (void*)twt_flow_info,
			sizeof(struct rtw_phl_twt_flow_field), _TRUE)) {
		RTW_ERR("[TWT] %s: rtw_core_twt_cmd fail!\n", __func__);
			return _FAIL;
	}

	return _SUCCESS;
}

u8 rtw_core_twt_on_action(_adapter *padapter, u8 *raddr, u8 *pframe_body, u32 frame_len)
{
	struct _ADAPTER_LINK *padapter_link = GET_PRIMARY_LINK(padapter);
	u8 ret = _FAIL;
	u8 action_code = 0;

	action_code = pframe_body[1];

	/* check dev cap support for TWT */
	if ((!TWT_REQ_SUPP(padapter_link->wrlink->protocol_cap.twt)) &&
	    (!TWT_RSP_SUPP(padapter_link->wrlink->protocol_cap.twt))) {
		RTW_INFO("[TWT] %s: Dev cap does not support TWT\n", __func__);
		return _SUCCESS;
	}

	/* only handle twt action frame if in wifi test mode or TWT is enabled currently */
	if (!padapter->registrypriv.wifi_spec && !padapter->registrypriv.twt_en) {
		RTW_INFO("[TWT] %s: Only handle action code %d in wifi test mode\n",
			 __func__, action_code);
		return _SUCCESS;
	}

	/* limit to use in STA mode associated to 11AX AP currently */
	if (!MLME_IS_STA(padapter)) {
		RTW_INFO("[TWT] %s: Only support to use in STA mode\n", __func__);
		return _SUCCESS;
	}
	if (!MLME_IS_ASOC(padapter)) {
		RTW_INFO("[TWT] %s: Only support to use when assoc. to AP\n", __func__);
		return _SUCCESS;
	}
	if (padapter_link->mlmepriv.hepriv.he_option == _FALSE) {
		RTW_INFO("[TWT] %s: Only support to use in 802.11AX mode\n", __func__);
		return _SUCCESS;
	}

	switch (action_code) {
		case RTW_WLAN_ACTION_UNP_S1G_TWT_SETUP:
			ret = _twt_on_setup_sta(padapter, padapter_link, raddr,
						pframe_body, frame_len);
			break;
		case RTW_WLAN_ACTION_UNP_S1G_TWT_TEARDOWN:
			ret = _twt_on_teardown_sta(padapter, padapter_link, raddr,
						   pframe_body, frame_len);
			break;
		case RTW_WLAN_ACTION_UNP_S1G_TWT_INFO:
			RTW_INFO("[TWT] %s: Not support TWT information action frame\n", __func__);
			ret = _SUCCESS;
			break;
		default:
			break;
	}

	return ret;
}

#endif /* CONFIG_TWT */
#endif /* CONFIG_80211AX_HE */

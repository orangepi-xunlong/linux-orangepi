/******************************************************************************
 *
 * Copyright(c) 2023 Realtek Corporation.
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
#define _PHL_CUSTOMIZE_FEATURE_ANTENNA_C_
#include "../phl_headers.h"

#ifdef CONFIG_PHL_CUSTOM_FEATURE_ANTENNA
#include "phl_custom_antenna.h"


void _phl_custom_ant_cmd_complete(void *priv, struct phl_msg *msg)
{
	struct rtw_custom_decrpt *c_cmd = NULL;

	if ((msg->inbuf != NULL) && (msg->inlen !=0)) {
		c_cmd = (struct rtw_custom_decrpt *)msg->inbuf;
		PHL_INFO("[PHL][CUSTOM][ANT]---> %s, op code %d, free size %d \n", __func__, c_cmd->evt_id, msg->inlen);
		_os_mem_free(priv, msg->inbuf, msg->inlen);
	}
}



enum rtw_phl_status
phl_custom_ant_cmd_enqueue(struct phl_info_t *phl_info,
			   enum phl_band_idx band_idx,
			   enum phl_msg_evt_id evt_id,
			   u8 *cmd_buf,
			   u32 cmd_len)
{
	enum rtw_phl_status psts = RTW_PHL_STATUS_FAILURE;
	struct phl_msg msg = {0};
	struct phl_msg_attribute attr = {0};

	SET_MSG_MDL_ID_FIELD(msg.msg_id, PHL_MDL_CUSTOM);
	SET_MSG_EVT_ID_FIELD(msg.msg_id, evt_id);

	msg.inbuf = (u8 *)cmd_buf;
	msg.inlen = cmd_len;
	msg.band_idx = band_idx;
	attr.completion.completion = _phl_custom_ant_cmd_complete;
	attr.completion.priv = phl_info;


	psts = phl_disp_eng_send_msg(phl_info, &msg, &attr, NULL);

	if (psts != RTW_PHL_STATUS_SUCCESS) {
		PHL_ERR("%s send msg failed\n", __func__);
	}

	return psts;
}



void rtw_phl_custom_antenna_frn_rst(struct phl_info_t *phl_info, enum phl_band_idx band_idx)
{
	void *drv = phl_to_drvpriv(phl_info);
	u8 *cmd_buf = NULL;
	u32 cmd_sz = sizeof(struct rtw_custom_decrpt);
	struct rtw_custom_decrpt *c_cmd = NULL;

	PHL_INFO("[PHL][CUSTOM][ANT] ---> %s\n", __func__);

	cmd_buf = _os_mem_alloc(drv, cmd_sz);
	PHL_INFO("[PHL][CUSTOM][ANT] allocate mem size %d \n", cmd_sz);

	if (cmd_buf != NULL) {
		c_cmd = ( struct rtw_custom_decrpt *)cmd_buf;
		c_cmd->customer_id = CUS_ID_ANT_TOOL;
		c_cmd->evt_id = (u8)RTW_PHL_CUSTOM_ANT_OP_FRN_CNT_RST;
		c_cmd->type = CUSTOM_CORE;
		c_cmd->len = 0;

		PHL_INFO("[PHL][CUSTOM][ANT] send cmd MSG_EVT_ANT_TOOL_OP_HDLR\n");
		if (RTW_PHL_STATUS_SUCCESS != phl_custom_ant_cmd_enqueue(
					phl_info,
					band_idx,
					MSG_EVT_ANT_TOOL_OP_HDLR,
					cmd_buf,
					cmd_sz)) {
			PHL_INFO("[PHL][CUSTOM][ANT] send cmd fail, free cmd buf memory \n");
			_os_mem_free(drv, cmd_buf, cmd_sz);
		}
	}
	PHL_INFO("[PHL][CUSTOM][ANT] <--- %s\n", __func__);
}

void rtw_phl_custom_antenna_fix_div_ant(struct phl_info_t *phl_info, enum phl_band_idx band_idx, u8 *ant_idx, u8 para_len)
{
	void *drv = phl_to_drvpriv(phl_info);
	u8 *cmd_buf = NULL;
	u32 cmd_sz = (para_len + sizeof(struct rtw_custom_decrpt));
	struct rtw_custom_decrpt *c_cmd = NULL;

	PHL_INFO("[PHL][CUSTOM][ANT] ---> %s\n", __func__);

	cmd_buf = _os_mem_alloc(drv, cmd_sz);
	PHL_INFO("[PHL][CUSTOM][ANT] allocate mem size %d \n", cmd_sz);

	if (cmd_buf != NULL) {
		c_cmd = ( struct rtw_custom_decrpt *)cmd_buf;
		c_cmd->customer_id = CUS_ID_ANT_TOOL;
		c_cmd->evt_id = (u8)RTW_PHL_CUSTOM_ANT_OP_FIX_DIV_ANT;
		c_cmd->type = CUSTOM_CORE;
		c_cmd->len = para_len;

		if ((NULL != ant_idx) && (para_len > 0))
			_os_mem_cpy(drv, (void *)(c_cmd + 1), ant_idx, para_len);

		PHL_INFO("[PHL][CUSTOM][ANT] send cmd MSG_EVT_ANT_TOOL_OP_HDLR\n");
		if (RTW_PHL_STATUS_SUCCESS != phl_custom_ant_cmd_enqueue(
					phl_info,
					band_idx,
					MSG_EVT_ANT_TOOL_OP_HDLR,
					cmd_buf,
					cmd_sz)) {
			PHL_INFO("[PHL][CUSTOM][ANT] send cmd fail, free cmd buf memory \n");
			_os_mem_free(drv, cmd_buf, cmd_sz);
		}
	}
	PHL_INFO("[PHL][CUSTOM][ANT] <--- %s\n", __func__);
}

void rtw_phl_custom_antenna_example_cmd(struct phl_info_t *phl_info, enum phl_band_idx band_idx, u8 *para, u8 para_len)
{
	void *drv = phl_to_drvpriv(phl_info);
	u8 *cmd_buf = NULL;
	u32 cmd_sz = (para_len + sizeof(struct rtw_custom_decrpt));
	struct rtw_custom_decrpt *c_cmd = NULL;

	PHL_INFO("[PHL][CUSTOM][ANT] ---> %s\n", __func__);

	cmd_buf = _os_mem_alloc(drv, cmd_sz);
	PHL_INFO("[PHL][CUSTOM][ANT] allocate mem size %d \n", cmd_sz);

	if (cmd_buf != NULL) {

		c_cmd = ( struct rtw_custom_decrpt *)cmd_buf;
		c_cmd->customer_id = CUS_ID_ANT_TOOL;
		c_cmd->evt_id = (u8)RTW_PHL_CUSTOM_ANT_OP_EXAMPLE;
		c_cmd->type = CUSTOM_CORE;
		c_cmd->len = para_len;
		/* memory copy para to cmd buf, after rtw_custom_decrpt */
		if ((NULL != para) && (para_len > 0))
			_os_mem_cpy(drv, (void *)(c_cmd + 1), para, para_len);

		PHL_INFO("[PHL][CUSTOM][ANT] send cmd RTW_PHL_CUSTOM_ANT_OP_MIN\n");
		if (RTW_PHL_STATUS_SUCCESS != phl_custom_ant_cmd_enqueue(
					phl_info,
					band_idx,
					MSG_EVT_ANT_TOOL_OP_HDLR,
					cmd_buf,
					cmd_sz)) {
			PHL_INFO("[PHL][CUSTOM][ANT] send cmd fail, free cmd buf memory \n");
			_os_mem_free(drv, cmd_buf, cmd_sz);
		}
	}
	PHL_INFO("[PHL][CUSTOM][ANT] <--- %s\n", __func__);
}


enum phl_mdl_ret_code
phl_custom_antenna_cmd_evt_post_hdlr(void* dispr,
				     void* custom_ctx,
				     struct phl_msg* msg)
{
	struct rtw_custom_decrpt *cmd = NULL;
	enum phl_mdl_ret_code ret = MDL_RET_SUCCESS;
	struct phl_info_t *phl = phl_custom_get_phl_info(custom_ctx);
	enum rtw_hal_status hstate = RTW_HAL_STATUS_SUCCESS;

	PHL_INFO("[PHL][CUSTOM][ANT] ---> %s\n", __func__);

	if (msg->inlen < sizeof(struct rtw_custom_decrpt))
		return MDL_RET_FAIL;


	cmd = (struct rtw_custom_decrpt *)(msg->inbuf);
	PHL_INFO("[PHL][CUSTOM][ANT] op_code %d\n", cmd->evt_id);

	switch (cmd->evt_id) {
	case RTW_PHL_CUSTOM_ANT_OP_EXAMPLE:
	{
		u8 need_len = sizeof(u32);
		u32 exp_para = 0;
		PHL_INFO("[PHL][CUSTOM][ANT] example : cmd->len = %d\n", cmd->len);
		if ((cmd->len >= need_len) &&
			(msg->inlen >= (cmd->len + sizeof(struct rtw_custom_decrpt)))) {
			exp_para = *((u32 *)(cmd + 1));
			PHL_INFO("[PHL][CUSTOM][ANT] example : exp_para = %d\n", exp_para);
		}
	}
	break;
	case RTW_PHL_CUSTOM_ANT_OP_FRN_CNT_RST:
	{
		hstate = rtw_hal_reset_freerun_counter(phl->hal,  msg->band_idx);
		if (hstate != RTW_HAL_STATUS_SUCCESS)
			ret = MDL_RET_FAIL;
	}
	break;
	case RTW_PHL_CUSTOM_ANT_OP_FIX_DIV_ANT:
	{
		u8 need_len = sizeof(u8);
		u8 ant_idx = 0;
		PHL_INFO("[PHL][CUSTOM][ANT] fix_div_ant : cmd->len = %d\n", cmd->len);
		if ((cmd->len >= need_len) &&
			(msg->inlen >= (cmd->len + sizeof(struct rtw_custom_decrpt)))) {
			ant_idx = *((u8 *)(cmd + 1));
			PHL_INFO("[PHL][CUSTOM][ANT] FIX_DIV_ANT : ant_idx = %d\n", ant_idx);
			hstate = rtw_hal_antdiv_fix_ant(phl->hal, ant_idx);
			if (hstate != RTW_HAL_STATUS_SUCCESS)
				ret = MDL_RET_FAIL;
		}
	}
	break;
	default:
		ret = MDL_RET_SUCCESS;
	break;
	}


	PHL_INFO("[PHL][CUSTOM][ANT] <--- %s\n", __func__);
	return ret;
}

enum phl_mdl_ret_code
phl_custom_antenna_cmd_evt_pre_hdlr(void* dispr,
				    void* custom_ctx,
				    struct phl_msg* msg)
{
	enum phl_mdl_ret_code ret = MDL_RET_SUCCESS;
	struct rtw_custom_decrpt *cmd = NULL;

	if (msg->inlen < sizeof(struct rtw_custom_decrpt))
		return MDL_RET_FAIL;

	cmd = (struct rtw_custom_decrpt *)(msg->inbuf);

	switch (cmd->evt_id) {
	case RTW_PHL_CUSTOM_ANT_OP_EXAMPLE:
	case RTW_PHL_CUSTOM_ANT_OP_FRN_CNT_RST:
	case RTW_PHL_CUSTOM_ANT_OP_FIX_DIV_ANT:
		ret = MDL_RET_SUCCESS;
	break;
	default:
		ret = MDL_RET_SUCCESS;
	break;
	}

	return ret;
}


enum phl_mdl_ret_code
phl_custom_antenna_cmd_hdlr(void* dispr,
			    void* custom_ctx,
			    struct phl_msg* msg)
{
	enum phl_mdl_ret_code ret = MDL_RET_SUCCESS;

	switch (MSG_EVT_ID_FIELD(msg->msg_id)) {
	case MSG_EVT_ANT_TOOL_OP_HDLR:
		if (IS_MSG_IN_PRE_PHASE(msg->msg_id))
			ret = phl_custom_antenna_cmd_evt_pre_hdlr(dispr, custom_ctx, msg);
		else
			ret = phl_custom_antenna_cmd_evt_post_hdlr(dispr, custom_ctx, msg);
	break;
	default:
		ret = MDL_RET_SUCCESS;
	break;
	}

	return ret;
}

#endif

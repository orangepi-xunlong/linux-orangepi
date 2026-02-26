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
#define _PHL_TEST_FPGA_C_
#include "../../phl_headers.h"
#include "phl_test_fpga_def.h"
#include "phl_test_fpga_api.h"

#ifdef CONFIG_PHL_TEST_FPGA
void fpga_notification_complete(void* priv, struct phl_msg* msg)
{
	struct fpga_context *fpga_ctx = (struct fpga_context *)priv;

	if(msg->inbuf){
		PHL_INFO("%s: Free info buf\n", __FUNCTION__);
		_os_kmem_free(fpga_ctx->phl_com->drv_priv, msg->inbuf, msg->inlen);
	}
}


void fpga_cmd_done_notification(struct fpga_context *fpga_ctx, enum fpga_class fpga_class,
							u8 fpga_cmd_id)
{
	struct phl_msg msg = {0};
	struct phl_msg_attribute attr;
	u8 *info = NULL;

	info = _os_kmem_alloc(fpga_ctx->phl_com->drv_priv, 2);

	if(info == NULL){
		PHL_ERR("%s: Allocate msg hub buffer fail!\n", __FUNCTION__);
		return;
	}

	info[0] = fpga_class;
	info[1] = fpga_cmd_id;

	SET_MSG_MDL_ID_FIELD(msg.msg_id, PHL_FUNC_MDL_TEST_MODULE);
	SET_MSG_EVT_ID_FIELD(msg.msg_id, MSG_EVT_FPGA_CMD_DONE);

	attr.completion.completion =fpga_notification_complete;
	attr.completion.priv = fpga_ctx;

	msg.inbuf = info;
	msg.inlen = 2;

	if (phl_msg_hub_send(fpga_ctx->phl,
			&attr, &msg) != RTW_PHL_STATUS_SUCCESS) {
		PHL_ERR("%s: send msg_hub failed\n", __func__);
		_os_kmem_free(fpga_ctx->phl_com->drv_priv, info, 2);
	}
}

/*
 * @enum phl_msg_evt_id id: Assign different types of FPGA related msg event
 *	to pass buffer to another layer for further process
 */
void fpga_buf_notification(struct fpga_context *fpga_ctx, void *buf, u32 buf_len,
			 enum phl_msg_evt_id id)
{
	struct phl_msg msg = {0};
	struct phl_msg_attribute attr;
	u8 *info = NULL;

	info = _os_kmem_alloc(fpga_ctx->phl_com->drv_priv, buf_len);

	if(info == NULL){
		PHL_ERR("%s: Allocate msg hub buffer fail!\n", __FUNCTION__);
		return;
	}

	_os_mem_cpy(fpga_ctx->phl_com->drv_priv, info, buf, buf_len);

	SET_MSG_MDL_ID_FIELD(msg.msg_id, PHL_FUNC_MDL_TEST_MODULE);
	SET_MSG_EVT_ID_FIELD(msg.msg_id, id);

	attr.completion.completion = fpga_notification_complete;
	attr.completion.priv = fpga_ctx;

	msg.inbuf = info;
	msg.inlen = buf_len;

	if (phl_msg_hub_send(fpga_ctx->phl, &attr, &msg) != RTW_PHL_STATUS_SUCCESS) {
		PHL_ERR("%s: send msg_hub failed\n", __func__);
		_os_kmem_free(fpga_ctx->phl_com->drv_priv, info, buf_len);
	}
}


bool fpga_get_rpt_check(struct fpga_context *fpga_ctx, void *rpt_buf)
{
	bool ret = true;
#if 0
	struct fpga_arg_hdr *rpt_hdr = (struct fpga_arg_hdr *)fpga_ctx->rpt;
	struct fpga_arg_hdr *rpt_buf_hdr = (struct fpga_arg_hdr *)rpt_buf;


	/* Do not check report buffer now */
	if((rpt_hdr->fpga_class != rpt_buf_hdr->fpga_class) ||
		(rpt_hdr->cmd != rpt_buf_hdr->cmd)) {
		PHL_WARN("%s: Report buffer not match!\n", __FUNCTION__);
		rpt_buf_hdr->cmd_ok = true;
		rpt_buf_hdr->status = RTW_PHL_STATUS_FAILURE;
		ret = false;
	}
#endif
	return ret;
}

u8 fpga_get_class_from_buf(struct fpga_context *fpga_ctx)
{
	u8 *buf_tmp = NULL;
	u8 fpga_class = FPGA_CLASS_MAX;
	if(fpga_ctx && fpga_ctx->buf) {
		buf_tmp	= (u8 *)fpga_ctx->buf;
		fpga_class = buf_tmp[0];
	}
	return fpga_class;
}

enum rtw_phl_status
_fpga_mlo_replace_tx(struct fpga_context *fpga_ctx,
                     struct rtw_xmit_req *tx_req)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)fpga_ctx->phl;
	struct rtw_t_meta_data *mdata = &tx_req->mdata;
	struct rtw_phl_stainfo_t *sta = NULL;
	struct rtw_phl_mld_t *mld = NULL;
	struct rtw_wifi_role_t *role = NULL;
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;
	u8 idx = 0, rlink_id = 0xFF;

	if (tx_req->role_id >= MAX_WIFI_ROLE_NUMBER) {
		PHL_WARN("%s: Tx req with unknown role!\n", __func__);
		goto exit;
	}

	if (mdata->type != RTW_PHL_PKT_TYPE_DATA) {
		/* Fixed link only for data packet */
		phl_status = RTW_PHL_STATUS_SUCCESS;
		goto exit;
	}

	rlink_id = fpga_ctx->mlo_ctx->fixed_link[tx_req->role_id];
	if (rlink_id == 0xFF) {
		/* No fixed link */
		phl_status = RTW_PHL_STATUS_SUCCESS;
		goto exit;
	}

	role = phl_get_wrole_by_ridx(fpga_ctx->phl,
	                             tx_req->role_id);
	if (role == NULL) {
		PHL_WARN("%s: Target role is NULL!\n", __func__);
		goto exit;
	}

	/* If ra is not broadcast address, use it to get MLD */
	if (!is_broadcast_mac_addr(mdata->ra))
		mld = rtw_phl_get_mld_by_addr(phl_info,
		                              role,
		                              mdata->ra);

	/*
	 * If ra is broadcast address or the MLD can not be get from ra, use
	 * self address to get MLD.
	 */
	if (mld == NULL)
		mld = rtw_phl_get_mld_self(phl_info,
		                           role);

	for (idx = 0; idx < mld->sta_num; idx++) {
		sta = mld->phl_sta[idx];
		if (sta->rlink->id == rlink_id) {
			mdata->macid = sta->macid;
			PHL_INFO("%s Assigned fixed link macid %d\n", __func__, sta->macid);
			/* shall update mlo table later */
			mdata->sw_mld_en = true;
			phl_status = RTW_PHL_STATUS_SUCCESS;
			break;
		}
	}
	if (phl_status != RTW_PHL_STATUS_SUCCESS)
		PHL_WARN("%s: Target role link not found (%d)!\n", __func__, rlink_id);

exit:
	return phl_status;
}


u8 fpga_bp_handler(void *priv, struct test_bp_info* bp_info)
{
	struct fpga_context *fpga_ctx = (struct fpga_context *)priv;
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;

	PHL_DBG("%s: bp_info->type = %x\n", __FUNCTION__, bp_info->type);

	switch(bp_info->type){
		case BP_INFO_TYPE_FPGA_CMD_EVENT:
			if(fpga_ctx->status == FPGA_STATUS_WAIT_CMD) {
				fpga_ctx->status = FPGA_STATUS_CMD_EVENT;
				_os_sema_up(fpga_ctx->phl_com->drv_priv,&(fpga_ctx->fpga_cmd_sema));
				phl_status = RTW_PHL_STATUS_SUCCESS;
			}
			break;
		case BP_INFO_TYPE_CORE_CB:
			if (bp_info->core_cb) {
				PHL_PRINT("%s: BP_INFO_TYPE_CORE_CB case\n", __FUNCTION__);
				bp_info->core_cb(fpga_ctx->phl_com->drv_priv, bp_info->ptr, bp_info->len);
				phl_status = RTW_PHL_STATUS_SUCCESS;
			}
			break;
		case BP_INFO_TYPE_MLO_ON_SAME_BAND:
			/* For 8852A simulation only, remove later */
			PHL_INFO("%s: BP_INFO_TYPE_MLO_ON_SAME_BAND\n", __func__);
			if (bp_info->ptr)
				*(u8 *)(bp_info->ptr) = HW_BAND_0;
			break;
		case BP_INFO_TYPE_MLO_REPLACE_TX:
			PHL_DBG("%s: BP_INFO_TYPE_MLO_REPLACE_TX\n", __func__);
			if (bp_info->ptr) {
				struct rtw_xmit_req *tx_req = (struct rtw_xmit_req *)bp_info->ptr;
				phl_status = _fpga_mlo_replace_tx(fpga_ctx, tx_req);
			}
			break;
		case BP_INFO_TYPE_MAX:
			PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "fpga_bp_handler(): Unsupported case:%d, please check it\n",
					bp_info->type);
			break;
		default:
			PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "fpga_bp_handler(): Unrecognize case:%d, please check it\n",
					bp_info->type);
			break;
	}

	return phl_status;
}

u8 fpga_get_fail_rsn(void *priv,char* rsn, u32 max_len)
{
	//struct fpga_context *fpga_ctx = (struct fpga_context *)priv;
	return true;
}

u8 fpga_is_test_end(void *priv)
{
	struct fpga_context *fpga_ctx = (struct fpga_context *)priv;

	return fpga_ctx->is_fpga_test_end;
}

u8 fpga_is_test_pass(void *priv)
{
	//struct fpga_context *fpga_ctx = (struct fpga_context *)priv;
	return true;
}

u8 fpga_start(void *priv)
{
	struct fpga_context *fpga_ctx = (struct fpga_context *)priv;
	struct rtw_phl_com_t* phl_com = fpga_ctx->phl_com;

	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;
	u8 fpga_class = FPGA_CLASS_MAX;
	FUNCIN();
	while(!fpga_is_test_end(fpga_ctx)){
		_os_sema_down(phl_com->drv_priv,&(fpga_ctx->fpga_cmd_sema));
		if(fpga_ctx->status == FPGA_STATUS_CMD_EVENT){
			fpga_ctx->status = FPGA_STATUS_RUN_CMD;
			fpga_class = fpga_get_class_from_buf(fpga_ctx);

			/* Clear report buffer before executing next command */
			if(fpga_ctx->rpt != NULL) {
				PHL_INFO("%s: Report not empty, cleanup!\n", __FUNCTION__);
				_os_mem_free(phl_com->drv_priv, fpga_ctx->rpt, fpga_ctx->rpt_len);
				fpga_ctx->rpt = NULL;
				fpga_ctx->rpt_len = 0;
			}

			switch(fpga_class){
				case FPGA_CLASS_CONFIG:
					PHL_INFO("%s: class = FPGA_CLASS_CONFIG\n", __FUNCTION__);
					phl_status = fpga_config(fpga_ctx, (struct fpga_config_arg *)fpga_ctx->buf);
					break;
				default:
					PHL_WARN("%s: Unknown fpga class! (%d)\n", __FUNCTION__, fpga_class);
					break;
			}

			if(fpga_ctx->rpt != NULL) {
				struct fpga_arg_hdr *hdr = (struct fpga_arg_hdr *)fpga_ctx->rpt;
				fpga_cmd_done_notification(fpga_ctx, hdr->fpga_class, hdr->cmd);
				PHL_INFO("%s: Indication class(%d) cmd(%d)\n",
						 __FUNCTION__, hdr->fpga_class, hdr->cmd);
			}

			/* Clear command buffer after executing the command */
			if(fpga_ctx->buf != NULL) {
				PHL_INFO("%s: Command buf not empty, cleanup!\n", __FUNCTION__);
				_os_mem_free(phl_com->drv_priv, fpga_ctx->buf, fpga_ctx->buf_len);
				fpga_ctx->buf = NULL;
				fpga_ctx->buf_len = 0;
			}
			fpga_ctx->status = FPGA_STATUS_WAIT_CMD;
		}
	}

	FUNCOUT();
	return (u8)phl_status;
}

void fpga_change_mode(struct fpga_context *fpga_ctx,
                      enum rtw_drv_mode driver_mode)
{
	struct phl_info_t *phl_info = fpga_ctx->phl;

	PHL_INFO("%s Change to %x\n", __FUNCTION__, driver_mode);

	phl_info->phl_com->drv_mode = driver_mode;
}

enum rtw_phl_status phl_test_fpga_alloc(struct phl_info_t *phl_info, void *hal, void **fpga)
{
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;
	struct rtw_phl_com_t *phl_com = phl_info->phl_com;
	struct fpga_context *fpga_ctx = NULL;
	u8 idx = 0;

	fpga_ctx = _os_mem_alloc(phl_com->drv_priv, sizeof(struct fpga_context));

	if (fpga_ctx == NULL) {
		PHL_ERR("alloc fpga_context failed\n");
		phl_status = RTW_PHL_STATUS_RESOURCE;
		goto exit;
	}

	fpga_ctx->mlo_ctx = _os_mem_alloc(phl_com->drv_priv, sizeof(struct mlo_test_context));

	if (fpga_ctx->mlo_ctx == NULL) {
		PHL_ERR("alloc mlo_ctx failed\n");
		_os_mem_free(phl_com->drv_priv, fpga_ctx, sizeof(struct fpga_context));
		phl_status = RTW_PHL_STATUS_RESOURCE;
		goto exit;
	}

	for (idx = 0; idx < MAX_WIFI_ROLE_NUMBER; idx ++) {
		/* 0xFF means no fixed link */
		fpga_ctx->mlo_ctx->fixed_link[idx] = 0xFF;
	}

	_os_sema_init(phl_com->drv_priv,&(fpga_ctx->fpga_cmd_sema), 0);
	fpga_ctx->cur_phy = HW_PHY_0;
	fpga_ctx->phl = phl_info;
	fpga_ctx->phl_com = phl_com;
	fpga_ctx->hal = hal;
	fpga_ctx->status = FPGA_STATUS_INIT;
	*fpga = fpga_ctx;
	phl_status = RTW_PHL_STATUS_SUCCESS;

exit:
	return phl_status;
}

void phl_test_fpga_free(void **fpga)
{
	struct fpga_context *fpga_ctx = NULL;

	if(*fpga == NULL)
		return;

	fpga_ctx = (struct fpga_context *)(*fpga);
	_os_sema_free(fpga_ctx->phl_com->drv_priv, &(fpga_ctx->fpga_cmd_sema));
	_os_mem_free(fpga_ctx->phl_com->drv_priv, fpga_ctx->mlo_ctx, sizeof(struct mlo_test_context));
	_os_mem_free(fpga_ctx->phl_com->drv_priv, fpga_ctx, sizeof(struct fpga_context));
	fpga_ctx = NULL;
	*fpga = NULL;
}

void phl_test_fpga_init(void *fpga)
{
	struct fpga_context *fpga_ctx = NULL;
	struct test_obj_ctrl_interface *pctrl = NULL;

	if(fpga == NULL)
		return;

	PHL_INFO("%s: Start to init FPGA\n", __FUNCTION__);

	fpga_ctx = (struct fpga_context *)fpga;
	pctrl = &(fpga_ctx->fpga_test_ctrl);
	fpga_ctx->max_para = 30;

	fpga_ctx->status = FPGA_STATUS_WAIT_CMD;
	fpga_ctx->is_fpga_test_end = false;
	pctrl->bp_handler = fpga_bp_handler;
	pctrl->get_fail_rsn = fpga_get_fail_rsn;
	pctrl->is_test_end = fpga_is_test_end;
	pctrl->is_test_pass = fpga_is_test_pass;
	pctrl->start_test = fpga_start;
	rtw_phl_test_add_new_test_obj(fpga_ctx->phl_com,
	                              "fpga_test",
	                              fpga_ctx,
	                              TEST_LVL_LOW,
	                              pctrl,
	                              -1,
	                              TEST_SUB_MODULE_FPGA,
	                              INTGR_TEST_MODE);
}

void phl_test_fpga_deinit(void *fpga)
{
	struct fpga_context *fpga_ctx = NULL;
	if(fpga == NULL)
		return;

	PHL_INFO("%s: Start to deinit fpga test module\n", __FUNCTION__);

	fpga_ctx = (struct fpga_context *)fpga;

	if(fpga_ctx->status < FPGA_STATUS_WAIT_CMD)
		return;

	fpga_ctx->is_fpga_test_end = true;
	_os_sema_up(fpga_ctx->phl_com->drv_priv,&(fpga_ctx->fpga_cmd_sema));
	fpga_ctx->status = FPGA_STATUS_INIT;
}

void phl_test_fpga_start(void *fpga, u8 tm_mode)
{
	struct fpga_context *fpga_ctx = NULL;

	if(fpga == NULL)
		return;

	fpga_ctx = (struct fpga_context *)fpga;

	fpga_change_mode(fpga_ctx, RTW_DRV_MODE_FPGA_SMDL_TEST);
}

void phl_test_fpga_stop(void *fpga, u8 tm_mode)
{
	struct fpga_context *fpga_ctx = NULL;
	if(fpga == NULL)
		return;

	fpga_ctx = (struct fpga_context *)fpga;

	if(fpga_ctx->status < FPGA_STATUS_WAIT_CMD)
		return;

	fpga_change_mode(fpga_ctx, RTW_DRV_MODE_NORMAL);
}

void phl_test_fpga_cmd_process(void *fpga, void *buf, u32 buf_len, u8 submdid)
{
	struct fpga_context *fpga_ctx = NULL;
	struct rtw_phl_com_t *phl_com = NULL;
	struct test_bp_info bp_info;
	FUNCIN();

	if(fpga == NULL)
		return;

	fpga_ctx = (struct fpga_context *)fpga;
	phl_com = fpga_ctx->phl_com;

	if((buf == NULL) || (buf_len > fpga_ctx->max_para)) {
		PHL_ERR("%s: Invalid buffer content!\n", __func__);
		return;
	}


	if(fpga_ctx->status == FPGA_STATUS_WAIT_CMD) {
		fpga_ctx->buf_len = buf_len;
		fpga_ctx->buf = _os_mem_alloc(phl_com->drv_priv, buf_len);
		_os_mem_cpy(phl_com->drv_priv, fpga_ctx->buf, buf, buf_len);
		_os_mem_set(phl_com->drv_priv, &bp_info, 0, sizeof(struct test_bp_info));
		bp_info.type = BP_INFO_TYPE_FPGA_CMD_EVENT;
		rtw_phl_test_setup_bp(phl_com, &bp_info, submdid);
	}
	else {
		PHL_WARN("%s: Previous command is still running!\n", __FUNCTION__);
	}

	FUNCOUT();
}

void phl_test_fpga_get_rpt(void *fpga, void *buf, u32 buf_len)
{
	struct fpga_context *fpga_ctx = NULL;
	FUNCIN();

	if(fpga == NULL) {
		PHL_WARN("%s: fpga is NULL!\n", __FUNCTION__);
		goto exit;
	}

	fpga_ctx = (struct fpga_context *)fpga;

	if(fpga_ctx->status != FPGA_STATUS_WAIT_CMD) {
		PHL_WARN("%s: command is running!\n", __FUNCTION__);
		goto exit;
	}

	if(fpga_ctx->rpt == NULL) {
		PHL_DBG("%s: fpga_ctx->rpt  is NULL!\n", __FUNCTION__);
		goto exit;
	}

	if(buf_len < fpga_ctx->rpt_len) {
		PHL_WARN("%s: buffer not enough!\n", __FUNCTION__);
		goto exit;
	}

	if(fpga_get_rpt_check(fpga_ctx, buf) == true) {
		_os_mem_cpy(fpga_ctx->phl_com->drv_priv, buf, fpga_ctx->rpt, fpga_ctx->rpt_len);
		_os_mem_free(fpga_ctx->phl_com->drv_priv, fpga_ctx->rpt, fpga_ctx->rpt_len);
		fpga_ctx->rpt = NULL;
		fpga_ctx->rpt_len = 0;
	}

exit:
	FUNCOUT();
}
#endif /* CONFIG_PHL_TEST_FPGA */

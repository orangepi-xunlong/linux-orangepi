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
#define _PHL_TEST_DBCC_C_
#include "../../../phl_headers.h"
#include "../phl_test_verify_def.h"
#include "../phl_test_verify_api.h"
#include "phl_test_dbcc_def.h"

#ifdef CONFIG_PHL_TEST_VERIFY
static u8 dbcc_get_class_from_buf(struct verify_context *ctx)
{
	u8 *buf_tmp = NULL;
	u8 dbcc_class = DBCC_CLASS_MAX;

	if (ctx && ctx->buf &&
		(ctx->buf_len > VERIFY_CMD_HDR_SIZE)) {
		buf_tmp	= (u8 *)VERIFY_GET_SUBUF((u8 *)ctx->buf);
		dbcc_class = buf_tmp[0];
	}
	return dbcc_class;
}

#ifdef CONFIG_DBCC_SUPPORT
static enum rtw_phl_status
phl_dbcc_ext_role_alloc(struct verify_context *ctx, struct dbcc_config_arg *arg)
{
	enum rtw_phl_status psts = RTW_PHL_STATUS_SUCCESS;
	struct phl_info_t *phl = (struct phl_info_t *)ctx->phl;
	struct rtw_wifi_role_t* def_role = NULL;
	struct ext_role_t *ext_wrole = &ctx->ext_wrole;
	u8 mac_addr[6] = {0};
	u8 *link_mac_addr[RTW_RLINK_MAX] = {0};

	/* Get the default port wrole */
	def_role = phl_mr_get_role_by_bandidx(phl, arg->cur_phy);
	if (def_role == NULL) {
		PHL_ERR("%s: wrole is null with phy_idx(%d)!!",__func__, arg->cur_phy);
		psts = RTW_PHL_STATUS_FAILURE;
		return psts;
	}

	/* Create ext_wrole for band 1*/
	_os_mem_cpy(phl_to_drvpriv(phl), mac_addr, def_role->mac_addr, MAC_ALEN);
	mac_addr[0] |= BIT1;
	link_mac_addr[0] = mac_addr;
	ext_wrole->role_idx = rtw_phl_wifi_role_alloc(phl,
	                                              mac_addr,
	                                              link_mac_addr,
	                                              PHL_RTYPE_STATION,
	                                              UNSPECIFIED_ROLE_ID,
	                                              &(ctx->ext_wrole.wrole),
	                                              DEV_TYPE_LEGACY,
	                                              false);

	if (ext_wrole->role_idx == INVALID_WIFI_ROLE_IDX) {
		psts = RTW_PHL_STATUS_FAILURE;
		PHL_ERR("%s: allocate ext wrole fail!!\n", __func__);
	}

	PHL_INFO("%s: ext_wrole->role_idx(%d)", __func__, ext_wrole->role_idx);

	return psts;
}

static void
phl_dbcc_ext_role_free(struct verify_context *ctx)
{
	struct phl_info_t *phl = (struct phl_info_t *)ctx->phl;
	struct ext_role_t *ext_wrole = &ctx->ext_wrole;

	if (ext_wrole->wrole != NULL)
		rtw_phl_wifi_role_free(phl, ext_wrole->role_idx);
	else
		PHL_WARN("%s: ext_wrole is null", __func__);

	return;
}

static enum rtw_phl_status
phl_dbcc_test(struct verify_context *ctx, struct dbcc_config_arg *arg)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)ctx->phl;
	enum rtw_phl_status psts = RTW_PHL_STATUS_SUCCESS;
	enum rtw_hal_status hsts = RTW_HAL_STATUS_FAILURE;
	bool dbcc_en = (bool)arg->dbcc_en;


	PHL_INFO("[DBCC] %s: %s\n", __func__, (dbcc_en) ? "EN" : "DIS");


	hsts = rtw_hal_dbcc_pre_cfg(phl_info->hal, phl_info->phl_com, dbcc_en);
	if (hsts != RTW_HAL_STATUS_SUCCESS) {
		PHL_INFO("[DBCC] %s: pre_cfg fail\n", __func__ );
		psts = RTW_PHL_STATUS_FAILURE;
		goto exit;
	}

	hsts = rtw_hal_dbcc_cfg(phl_info->hal, phl_info->phl_com, dbcc_en);
	if (hsts != RTW_HAL_STATUS_SUCCESS) {
		PHL_INFO("[DBCC] %s: cfg fail\n", __func__ );
		psts = RTW_PHL_STATUS_FAILURE;
		goto exit;
	}

	if (dbcc_en == true) {
		hsts = rtw_hal_dbcc_reset_hal(phl_info->hal);\
		if (hsts != RTW_HAL_STATUS_SUCCESS) {
			PHL_INFO("[DBCC] %s: reset fail\n", __func__ );
			psts = RTW_PHL_STATUS_FAILURE;
			goto exit;
		}

		phl_dbcc_ext_role_alloc(ctx, arg);
	} else {
		phl_dbcc_ext_role_free(ctx);
	}
exit:
	return psts;
}
#endif /* CONFIG_DBCC_SUPPORT */

static enum rtw_phl_status
dbcc_config(struct verify_context *ctx, struct dbcc_config_arg *arg)
{
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;

	if (arg == NULL) {
		return phl_status;
	}

	PHL_INFO("%s: en %u id %u\n", __FUNCTION__, arg->dbcc_en, arg->macid);
#ifdef CONFIG_DBCC_SUPPORT
	phl_status = phl_dbcc_test(ctx, arg);
#else
	PHL_ERR("%s: %s No Support DBCC\n", __FUNCTION__,
			ctx->phl_com->hal_spec.ic_name);
	phl_status = RTW_PHL_STATUS_SUCCESS;
#endif
	return phl_status;
}

enum rtw_phl_status rtw_test_dbcc_cmd_process(void *priv)
{
	struct verify_context *ctx = NULL;
	struct rtw_phl_com_t *phl_com = NULL;
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;
	u8 dbcc_class = DBCC_CLASS_MAX;

	FUNCIN();

	if (priv == NULL)
		return phl_status;

	ctx = (struct verify_context *)priv;
	phl_com = ctx->phl_com;

	if ((ctx->buf_len < VERIFY_CMD_HDR_SIZE)) {
		PHL_ERR("%s: Invalid buffer content!\n", __FUNCTION__);
		return phl_status;
	}

	dbcc_class = dbcc_get_class_from_buf(ctx);
	switch (dbcc_class) {
	case DBCC_CLASS_CONFIG:
		PHL_INFO("%s: class = DBCC_CLASS_CONFIG\n", __FUNCTION__);
		phl_status = dbcc_config(ctx, (struct dbcc_config_arg *)VERIFY_GET_SUBUF((u8 *)ctx->buf));
		break;
	default:
		PHL_WARN("%s: Unknown DBCC_CLASS! (%d)\n", __FUNCTION__, dbcc_class);
		break;
	}

	FUNCOUT();

	return phl_status;
}
#else
enum rtw_phl_status rtw_test_dbcc_cmd_process(void *priv)
{
	return RTW_PHL_STATUS_SUCCESS;
}
#endif /* CONFIG_PHL_TEST_VERIFY */

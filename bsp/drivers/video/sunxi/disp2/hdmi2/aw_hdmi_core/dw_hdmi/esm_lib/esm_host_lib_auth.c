/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "include/ESMHost.h"

ESM_STATUS ESM_Kill(esm_instance_t *esm)
{
	ESM_STATUS err;

	if (esm == NULL) {
		hdmi_err("%s: ESM is NULL!\n", __func__);
		return ESM_HL_NO_INSTANCE;
	}

	ESM_FlushExceptions(esm);

	esm->esm_exception = 0;
	esm->esm_sync_lost = 0;
	esm->esm_auth_pass = 0;
	esm->esm_auth_fail = 0;

	err = esm_hostlib_mb_cmd(esm, ESM_CMD_SYSTEM_ON_EXIT_REQ, 0, 0,
					ESM_CMD_SYSTEM_ON_EXIT_RESP, 1,
					&esm->status, CMD_DEFAULT_TIMEOUT);
	/* This command is designed to cause a timeout */
	if (err != ESM_HL_COMMAND_TIMEOUT) {
		hdmi_err("%s: MB Failed %d\n", __func__, err);
		return ESM_HL_MB_FAILED;
	}

	return ESM_HL_SUCCESS;
}

ESM_STATUS ESM_Reset(esm_instance_t *esm)
{
	/* ESM_STATUS err; */

	if (esm == NULL) {
		hdmi_err("%s: ESM is NULL!\n", __func__);
		return ESM_HL_NO_INSTANCE;
	}

	ESM_FlushExceptions(esm);

	esm->esm_exception = 0;
	esm->esm_sync_lost = 0;
	esm->esm_auth_pass = 0;
	esm->esm_auth_fail = 0;

	if (esm_hostlib_mb_cmd(esm, ESM_CMD_SYSTEM_RESET_REQ, 0, 0,
		ESM_CMD_SYSTEM_RESET_RESP, 1, &esm->status, CMD_DEFAULT_TIMEOUT) != ESM_HL_SUCCESS) {
		hdmi_err("%s: MB Failed\n", __func__);
		return ESM_HL_MB_FAILED;
	}

	if (esm->status != ESM_SUCCESS) {
		hdmi_err("%s: Failed status %d\n", __func__, esm->status);
		return ESM_HL_FAILED;
	}

	return ESM_HL_SUCCESS;
}

ESM_STATUS ESM_EnableLowValueContent(esm_instance_t *esm)
{
	ESM_STATUS err;
	uint32_t req = 0;
	uint32_t resp = 0;

	if (esm == NULL) {
		hdmi_err("%s: ESM is NULL!\n", __func__);
		return ESM_HL_NO_INSTANCE;
	}

	/* esm->fw_type = ESM_HOST_LIB_TX; */
	/* switch (esm->fw_type) {
	case ESM_HOST_LIB_TX:
		req =  ESM_HDCP_HDMI_TX_CMD_ENABLE_LOW_VALUE_CONTENT_REQ;
		resp = ESM_HDCP_HDMI_TX_CMD_ENABLE_LOW_VALUE_CONTENT_RESP;
		break;

	default:
		return ESM_HL_INVALID_COMMAND;
	} */
	req =  ESM_HDCP_HDMI_TX_CMD_ENABLE_LOW_VALUE_CONTENT_REQ;
	resp = ESM_HDCP_HDMI_TX_CMD_ENABLE_LOW_VALUE_CONTENT_RESP;
	err = esm_hostlib_mb_cmd(esm, req, 0, 0, resp, 0, &esm->status,
						CMD_DEFAULT_TIMEOUT);
	if (err != ESM_HL_SUCCESS) {
		hdmi_err("%s: MB Failed %d\n", __func__, err);
		return ESM_HL_MB_FAILED;
	}

	if (esm->status != ESM_SUCCESS) {
		hdmi_err("%s: Faild status %d\n", __func__, esm->status);
		return ESM_HL_FAILED;
	}

	return ESM_HL_SUCCESS;
}


ESM_STATUS ESM_Authenticate(esm_instance_t *esm, uint32_t Cmd,
	uint32_t StreamID, uint32_t ContentType)
{
	uint32_t req = 0;
	uint32_t resp = 0;
	uint32_t req_param;

	LOG_TRACE();

	if (esm == NULL) {
		hdmi_err("%s: ESM is NULL!\n", __func__);
		return ESM_HL_NO_INSTANCE;
	}

	if (ContentType > 2) {
		hdmi_err("%s: content type(%d) > 2\n", __func__, ContentType);
		return ESM_HL_INVALID_PARAMETERS;
	}

	ESM_FlushExceptions(esm);

	esm->esm_exception = 0;
	esm->esm_sync_lost = 0;
	esm->esm_auth_pass = 0;
	esm->esm_auth_fail = 0;
	esm->fw_type = 2;

	if (Cmd) {
		req = ESM_HDCP_HDMI_TX_CMD_AKE_START_REQ;
		resp = ESM_HDCP_HDMI_TX_CMD_AKE_START_RESP;

		req_param = ContentType;

		if (esm_hostlib_mb_cmd(esm, req, 1, &req_param, resp, 1,
			&esm->status, CMD_DEFAULT_TIMEOUT) != ESM_HL_SUCCESS) {
			hdmi_err("%s: [ESM_HOST_LIB_TX] MB Failed\n", __func__);
			return ESM_HL_MB_FAILED;
		}
	} else {
		if (esm_hostlib_mb_cmd(esm, ESM_HDCP_HDMI_TX_CMD_AKE_STOP_REQ, 0, 0,
				ESM_HDCP_HDMI_TX_CMD_AKE_STOP_RESP, 1,
				&esm->status, CMD_DEFAULT_TIMEOUT) != ESM_HL_SUCCESS) {
			hdmi_err("%s: MB Failed\n", __func__);
			return ESM_HL_MB_FAILED;
		}
	}

	if (esm->status != ESM_SUCCESS) {
		hdmi_err("%s: Failed status %d\n", __func__, esm->status);
		return ESM_HL_FAILED;
	}
	return ESM_HL_SUCCESS;
}

ESM_STATUS ESM_SetCapability(esm_instance_t *esm)
{
	ESM_STATUS err = -1;

	LOG_TRACE();
	if (esm == NULL) {
		hdmi_err("%s: ESM is NULL!\n", __func__);
		return ESM_HL_NO_INSTANCE;
	}

	if (esm_hostlib_mb_cmd(esm, ESM_HDCP_HDMI_TX_CMD_AKE_SET_CAPABILITY_REQ,
				0, 0, ESM_HDCP_HDMI_TX_CMD_AKE_SET_CAPABILITY_RESP, 1,
				&esm->status, CMD_DEFAULT_TIMEOUT) != ESM_HL_SUCCESS) {
		hdmi_err("%s: MB Failed %d\n", __func__, err);
		return ESM_HL_MB_FAILED;
	}

	if (esm->status != ESM_SUCCESS) {
		hdmi_err("%s: Failed status %d\n", __func__, esm->status);
		return ESM_HL_FAILED;
	}

	return ESM_HL_SUCCESS;
}

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

/* Enables or disables logging. */
ESM_STATUS ESM_LogControl(esm_instance_t *esm, uint32_t Cmd, uint32_t Size)
{
	ESM_STATUS err;
	uint32_t req_param = Cmd;

	LOG_TRACE();
	if (esm == 0)
		return ESM_HL_NO_INSTANCE;

	if (Cmd) {
		req_param = Size;

		err = esm_hostlib_mb_cmd(esm, ESM_CMD_SYSTEM_ENABLE_LOGGING_REQ, 1,
				&req_param, ESM_CMD_SYSTEM_ENABLE_LOGGING_RESP, 1,
				&esm->status, CMD_DEFAULT_TIMEOUT);

		if (err != ESM_HL_SUCCESS) {
			pr_info("MB Failed %d", err);
			return ESM_HL_MB_FAILED;
		}
	} else {
		err = esm_hostlib_mb_cmd(esm, ESM_CMD_SYSTEM_DISABLE_LOGGING_REQ, 0, 0,
				ESM_CMD_SYSTEM_DISABLE_LOGGING_RESP, 1, &esm->status, CMD_DEFAULT_TIMEOUT);

		if (err != ESM_HL_SUCCESS) {
			pr_info("MB Failed %d", err);
			return ESM_HL_MB_FAILED;
		}
	}

	if (esm->status != ESM_SUCCESS) {
		pr_info("status %d", esm->status);
		return ESM_HL_FAILED;
	}

	return ESM_HL_SUCCESS;
}


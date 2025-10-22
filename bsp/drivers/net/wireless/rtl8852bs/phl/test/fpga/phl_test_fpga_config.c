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
#define _PHL_TEST_FPGA_CONFIG_C_
#include "../../phl_headers.h"
#include "phl_test_fpga_def.h"
#include "../../hal_g6/test/fpga/hal_test_fpga_api.h"

#ifdef CONFIG_PHL_TEST_FPGA

enum rtw_phl_status
_fpga_config_pkt_tx(struct fpga_context *fpga,
                    struct fpga_config_arg *arg)
{
	struct rtw_trx_test_param test_param = {0};

	rtw_phl_trx_default_param(fpga->phl, &test_param);

	test_param.tx_req_num = arg->param1;
	/* Fill test_param.tx_cap with input paramenter */
	rtw_phl_trx_testsuite(fpga->phl, &test_param);
	return RTW_PHL_STATUS_SUCCESS;
}

enum rtw_phl_status
_fpga_config_loopback(struct fpga_context *fpga,
                     struct fpga_config_arg *arg)
{
	if (arg->param1 != 0) {
		fpga_change_mode(fpga, RTW_DRV_MODE_FPGA_SMDL_LOOPBACK);
		rtw_phl_reset(fpga->phl);
	} else {
		fpga_change_mode(fpga, RTW_DRV_MODE_FPGA_SMDL_TEST);
		rtw_phl_reset(fpga->phl);
	}
	return RTW_PHL_STATUS_SUCCESS;
}

enum rtw_phl_status
_fpga_config_normal(struct fpga_context *fpga,
                     struct fpga_config_arg *arg)
{
	if (arg->param1 != 0) {
		fpga_change_mode(fpga, RTW_DRV_MODE_NORMAL);
		rtw_phl_reset(fpga->phl);
	} else {
		fpga_change_mode(fpga, RTW_DRV_MODE_FPGA_SMDL_TEST);
		rtw_phl_reset(fpga->phl);
	}
	return RTW_PHL_STATUS_SUCCESS;
}

enum rtw_phl_status fpga_config(struct fpga_context *fpga,struct fpga_config_arg *arg)
{
	enum rtw_phl_status phl_status = RTW_PHL_STATUS_FAILURE;
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_SUCCESS;

	switch(arg->cmd){
		case FPGA_CONFIG_CMD_START_DUT:
			PHL_INFO("%s: CMD = FPGA_CONFIG_CMD_START_DUT\n", __func__);

			arg->cmd_ok = true;
			arg->status = RTW_PHL_STATUS_SUCCESS;
			/* Transfer to report */
			fpga->rpt = arg;
			fpga->rpt_len = sizeof(struct fpga_config_arg);
			fpga->buf = NULL;
			fpga->buf_len = 0;
			break;
		case FPGA_CONFIG_CMD_MAC_CRC_OK:
			PHL_INFO("%s: CMD = FPGA_CONFIG_CMD_MAC_CRC_OK\n", __func__);

			hal_status = rtw_hal_fpga_rx_mac_crc_ok(fpga,
			                                        arg->param1,
			                                        &arg->param2);

			arg->cmd_ok = true;
			arg->status = hal_status;
			/* Transfer to report */
			fpga->rpt = arg;
			fpga->rpt_len = sizeof(struct fpga_config_arg);
			fpga->buf = NULL;
			fpga->buf_len = 0;
			break;
		case FPGA_CONFIG_CMD_MAC_CRC_ERR:
			PHL_INFO("%s: CMD = FPGA_CONFIG_CMD_MAC_CRC_ERR\n", __func__);

			hal_status = rtw_hal_fpga_rx_mac_crc_err(fpga,
			                                         arg->param1,
			                                         &arg->param2);

			arg->cmd_ok = true;
			arg->status = hal_status;
			/* Transfer to report */
			fpga->rpt = arg;
			fpga->rpt_len = sizeof(struct fpga_config_arg);
			fpga->buf = NULL;
			fpga->buf_len = 0;
			break;
		case FPGA_CONFIG_CMD_RESET_MAC_RX_CNT:
			PHL_INFO("%s: CMD = FPGA_CONFIG_CMD_RESET_MAC_RX_CNT\n", __func__);

			hal_status = rtw_hal_fpga_reset_mac_cnt(fpga,
			                                        arg->param1);
			arg->cmd_ok = true;
			arg->status = hal_status;
			/* Transfer to report */
			fpga->rpt = arg;
			fpga->rpt_len = sizeof(struct fpga_config_arg);
			fpga->buf = NULL;
			fpga->buf_len = 0;
			break;
		case FPGA_CONFIG_CMD_PKT_TX:
			PHL_INFO("%s: CMD = FPGA_CONFIG_CMD_PKT_TX\n", __func__);

			_fpga_config_pkt_tx(fpga, arg);

			arg->cmd_ok = true;
			arg->status = hal_status;
			/* Transfer to report */
			fpga->rpt = arg;
			fpga->rpt_len = sizeof(struct fpga_config_arg);
			fpga->buf = NULL;
			fpga->buf_len = 0;
			break;
		case FPGA_CONFIG_CMD_LOOPBACK:
			PHL_INFO("%s: CMD = FPGA_CONFIG_CMD_LOOPBACK\n", __func__);
			_fpga_config_loopback(fpga, arg);

			arg->cmd_ok = true;
			arg->status = hal_status;
			/* Transfer to report */
			fpga->rpt = arg;
			fpga->rpt_len = sizeof(struct fpga_config_arg);
			fpga->buf = NULL;
			fpga->buf_len = 0;
			break;
		case FPGA_CONFIG_CMD_NORMAL:
			PHL_INFO("%s: CMD = FPGA_CONFIG_CMD_NORMAL\n", __func__);
			_fpga_config_normal(fpga, arg);

			arg->cmd_ok = true;
			arg->status = hal_status;
			/* Transfer to report */
			fpga->rpt = arg;
			fpga->rpt_len = sizeof(struct fpga_config_arg);
			fpga->buf = NULL;
			fpga->buf_len = 0;
			break;
		case FPGA_CONFIG_CMD_FIXED_LINK_TX:
			{
				u8 role_id = 0xff;
				u8 rlink_id = 0xff;
				PHL_INFO("%s: CMD = FPGA_CONFIG_CMD_FIXED_LINK_TX\n", __func__);

				/*
				 * param1:
				 * [0~7] = link id (0xff = No fixed)
				 * [8~15] = role id
				 */
				role_id = (u8)((arg->param1 >> 8) & 0xff);
				rlink_id = (u8)(arg->param1 & 0xff);
				if (role_id >= MAX_WIFI_ROLE_NUMBER) {
					hal_status = RTW_HAL_STATUS_FAILURE;
				} else {
					if (rlink_id != 0xff &&
					    rlink_id >= RTW_RLINK_MAX) {
						hal_status = RTW_HAL_STATUS_FAILURE;
					} else {
						fpga->mlo_ctx->fixed_link[role_id] = rlink_id;
					}
				}

				arg->cmd_ok = true;
				arg->status = hal_status;
				/* Transfer to report */
				fpga->rpt = arg;
				fpga->rpt_len = sizeof(struct fpga_config_arg);
				fpga->buf = NULL;
				fpga->buf_len = 0;
			}
			break;
		default:
			PHL_WARN("%s: CMD NOT RECOGNIZED\n", __func__);
			break;
	}

	return phl_status;
}
#endif /* CONFIG_PHL_TEST_FPGA */

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
#ifndef _PHL_TEST_FPGA_DEF_H_
#define _PHL_TEST_FPGA_DEF_H_

#ifdef CONFIG_PHL_TEST_FPGA
enum fpga_cmd_status {
	FPGA_STATUS_NOT_INIT = 0,
	FPGA_STATUS_INIT = 1,
	FPGA_STATUS_WAIT_CMD = 2,
	FPGA_STATUS_CMD_EVENT = 3,
	FPGA_STATUS_RUN_CMD = 4,
};


/* fpga command class */
enum fpga_class {
	FPGA_CLASS_CONFIG = 0,
	FPGA_CLASS_MAX,
};

/* fpga config command */
enum fpga_config_cmd {
	FPGA_CONFIG_CMD_START_DUT,
	FPGA_CONFIG_CMD_MAC_CRC_OK,
	FPGA_CONFIG_CMD_MAC_CRC_ERR,
	FPGA_CONFIG_CMD_RESET_MAC_RX_CNT,
	FPGA_CONFIG_CMD_PKT_TX,
	FPGA_CONFIG_CMD_LOOPBACK,
	FPGA_CONFIG_CMD_NORMAL,
	FPGA_CONFIG_CMD_FIXED_LINK_TX,
	FPGA_CONFIG_CMD_MAX,
};

struct fpga_arg_hdr {
	u8 fpga_class;
	u8 cmd;
	u8 cmd_ok;
	u8 status;
};

struct fpga_config_arg {
	u8 fpga_class;
	u8 cmd;
	u8 cmd_ok;
	u8 status;
	u32 param1;
	u32 param2;
	u32 param3;
	u32 param4;
};

struct mlo_test_context {
	/* Fixed linked settings, 0xff: Auto */
	u8 fixed_link[MAX_WIFI_ROLE_NUMBER];
};

struct fpga_context {
	u8 status;
	u8 cur_phy;
	_os_sema fpga_cmd_sema;
	void *buf;
	u32 buf_len;
	void *rpt;
	u32 rpt_len;
	u8 is_fpga_test_end;
	struct test_obj_ctrl_interface fpga_test_ctrl;
	struct phl_info_t *phl;
	struct rtw_phl_com_t *phl_com;
	void *hal;
	u32 max_para;
	struct mlo_test_context *mlo_ctx;
};

void fpga_change_mode(struct fpga_context *fpga_ctx,
                      enum rtw_drv_mode driver_mode);


#endif /* CONFIG_PHL_TEST_FPGA */

#endif /* _PHL_TEST_FPGA_DEF_H_ */



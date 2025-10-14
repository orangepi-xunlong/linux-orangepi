/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright 2025 Cix Technology Group Co., Ltd.*/

#ifndef __CIX_AP2SE_IPC_H__
#define __CIX_AP2SE_IPC_H__

#include <linux/types.h>

#define CIX_MBOX_MSG_LEN (32)
#define MBOX_HEADER_NUM (2)
#define MBOX_HEADER_SIZE (sizeof(uint32_t) * MBOX_HEADER_NUM)

enum {
	FFA_SRC_SE_CRASH = 0x40000001,
	FFA_SRC_PM_CRASH = 0x40000002,
	FFA_SRC_TEST_CRASH = 0x40000003,
	FFA_CMDID_PLAT_WAKEUP_SRC = 0x50000001,

	FFA_GET_DDR_IRQ_DIS = 0x82000011,
	FFA_SET_EXCEPTION_ADDR = 0x82000015,
	FFA_REQ_WAKEUP_SOURCE = 0x82000016,
	FFA_REQ_AP_HARDLOCK = 0x82000017,
	FFA_CLK_AUTO_GATING_ENABLE = 0x82000018,
	FFA_CLK_AUTO_GATING_DISABLE = 0x82000019,
};

struct mbox_msg_t {
	uint32_t size : 7;
	uint32_t type : 3;
	uint32_t reserve1 : 22;
	uint32_t cmd_id;
	uint32_t data[CIX_MBOX_MSG_LEN - MBOX_HEADER_NUM];
};

typedef void (*ipc_rx_callback_t)(char *inbuf, size_t len);

int cix_ap2se_register_rx_cbk(uint32_t cmd_id, ipc_rx_callback_t cbk);
int cix_ap2se_ipc_send(uint32_t cmd_id, char *data, size_t len, bool need_reply);

#endif

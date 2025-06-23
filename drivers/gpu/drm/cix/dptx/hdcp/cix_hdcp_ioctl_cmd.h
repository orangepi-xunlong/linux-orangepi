// SPDX-License-Identifier: GPL-2.0
/*
 * display port hdcp driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef CIX_HDCP_IOCTL_CMD_H_
#define CIX_HDCP_IOCTL_CMD_H_

#include <linux/types.h>
#include <asm/ioctl.h>

//------------------------------------------------------------------------------
//  Aux transaction callback
//------------------------------------------------------------------------------
typedef struct dptx_aux_trxn_s dptx_aux_trxn_t;	//  Aux Transaction forward reference
typedef void (*dptx_aux_cb_t)(dptx_aux_trxn_t* trxn);	//  callback

//------------------------------------------------------------------------------
//  aux transmitter status
//------------------------------------------------------------------------------
typedef enum {
	dptx_aux_status_ok,		// status is good, operation success
	dptx_aux_status_error,		// general error condition
	dptx_aux_status_ct		// status count (must be last)
} dptx_aux_status_t;

//------------------------------------------------------------------------------
//  DisplayPort transmitter aux transaction structure.
//  This structure is linked to the dptx_aux_trxn_t typedef.
//------------------------------------------------------------------------------
struct dptx_aux_trxn_s {
	uint32_t cmd;			//  aux-bus command
	uint32_t addr;			//  native: aux-bus address / i2c: i2c-address
	uint32_t ct;			//  data count
	uint8_t *data;
	void *user_data;		//  handle data
	dptx_aux_cb_t cb;
	dptx_aux_status_t status;	//  transaction status
};

#define CIX_HDCP_IOCTL_BASE		'H'
#define CIX_HDCP_IO(nr)			_IO(CIX_HDCP_IOCTL_BASE,nr)
#define CIX_HDCP_IOR(nr,type)		_IOR(CIX_HDCP_IOCTL_BASE,nr,type)
#define CIX_HDCP_IOW(nr,type)		_IOW(CIX_HDCP_IOCTL_BASE,nr,type)
#define CIX_HDCP_IOWR(nr,type)		_IOWR(CIX_HDCP_IOCTL_BASE,nr,type)

#define HDCP2_IOCTL_RXSTATE		CIX_HDCP_IOR(0x00, uint)
#define HDCP2_IOCTL_TIMER_START		CIX_HDCP_IOW(0x01, uint)
#define HDCP2_IOCTL_TIMER_STOP		CIX_HDCP_IO(0x02)
#define HDCP2_IOCTL_DPCD_ACCESS		CIX_HDCP_IOWR(0x03, dptx_aux_trxn_t)

#endif // CIX_HDCP_IOCTL_CMD_H_

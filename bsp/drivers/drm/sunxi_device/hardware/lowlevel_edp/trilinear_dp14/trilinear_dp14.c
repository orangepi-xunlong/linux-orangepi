/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * api for inno edp tx based on edp_1.3 hardware operation
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "../../../sunxi_edp.h"
#include "trilinear_dp14.h"
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#define LS_PER_TU	64

struct edp_aux_request {
	bool address_only;
	int cmd_code;
	int address;
	int data_len;
	int data[16];
};

struct edp_audio_mn_aud {
	int link_bw;
	int data_rate;
	int maud;
	int naud;
};

#define RET_OP(ret)	(0 - ret)

static const char * const aux_fail_reason_str[] = {
	[RET_OP(RET_AUX_NACK)]			= "AUX_NACK",
	[RET_OP(RET_AUX_TIMEOUT)]		= "AUX_TIMEOUT",
	[RET_OP(RET_AUX_DEFER)]			= "AUX_DEFER",
	[RET_OP(RET_AUX_NO_STOP)]		= "AUX_NO_STOP",
	[RET_OP(RET_AUX_RPLY_ERR)]		= "AUX_RPLY_ERR",
	[RET_OP(RET_I2C_AUX_NACK)]		= "I2C_AUX_NACK",
	[RET_OP(RET_I2C_AUX_DEFER)]		= "I2C_AUX_DEFER",
};


#define EDP_AUD_DEFINE(bw, rate, m, n) \
	.link_bw = bw, \
	.data_rate = rate, \
	.maud = m, \
	.naud = n,

struct edp_audio_mn_aud aud_tbl[] = {
	{ EDP_AUD_DEFINE(162, 32000, 1024, 10125) },
	{ EDP_AUD_DEFINE(162, 44100, 784, 5625) },
	{ EDP_AUD_DEFINE(162, 48000, 512, 3375) },
	{ EDP_AUD_DEFINE(162, 64000, 2048, 10125) },
	{ EDP_AUD_DEFINE(162, 88200, 1568, 5625) },
	{ EDP_AUD_DEFINE(162, 90600, 1024, 3375) },
	{ EDP_AUD_DEFINE(162, 128000, 4096, 10125) },
	{ EDP_AUD_DEFINE(162, 176400, 3136, 5625) },
	{ EDP_AUD_DEFINE(162, 192000, 2048, 3375) },

	{ EDP_AUD_DEFINE(270, 32000, 1024, 16875) },
	{ EDP_AUD_DEFINE(270, 44100, 784, 9375) },
	{ EDP_AUD_DEFINE(270, 48000, 512, 5625) },
	{ EDP_AUD_DEFINE(270, 64000, 2048, 16875) },
	{ EDP_AUD_DEFINE(270, 88200, 1568, 9375) },
	{ EDP_AUD_DEFINE(270, 90600, 1024, 5625) },
	{ EDP_AUD_DEFINE(270, 128000, 4096, 16875) },
	{ EDP_AUD_DEFINE(270, 176400, 3136, 9375) },
	{ EDP_AUD_DEFINE(270, 192000, 2048, 5625) },

	{ EDP_AUD_DEFINE(540, 32000, 1024, 33750) },
	{ EDP_AUD_DEFINE(540, 44100, 784, 18750) },
	{ EDP_AUD_DEFINE(540, 48000, 512, 11250) },
	{ EDP_AUD_DEFINE(540, 64000, 2048, 33750) },
	{ EDP_AUD_DEFINE(540, 88200, 1568, 18750) },
	{ EDP_AUD_DEFINE(540, 90600, 1024, 11250) },
	{ EDP_AUD_DEFINE(540, 128000, 4096, 33750) },
	{ EDP_AUD_DEFINE(540, 176400, 3136, 18750) },
	{ EDP_AUD_DEFINE(540, 192000, 2048, 11250) },

	{ EDP_AUD_DEFINE(810, 32000, 1024, 50625) },
	{ EDP_AUD_DEFINE(810, 44100, 784, 28125) },
	{ EDP_AUD_DEFINE(810, 48000, 512, 16875) },
	{ EDP_AUD_DEFINE(810, 64000, 2048, 50625) },
	{ EDP_AUD_DEFINE(810, 88200, 1568, 28125) },
	{ EDP_AUD_DEFINE(810, 90600, 1024, 16875) },
	{ EDP_AUD_DEFINE(810, 128000, 4096, 50625) },
	{ EDP_AUD_DEFINE(810, 176400, 3136, 28125) },
	{ EDP_AUD_DEFINE(810, 192000, 2048, 16875) },
};

u32 TR_READ(struct sunxi_edp_hw_desc *edp_hw, u32 addr)
{
	return readl(edp_hw->reg_base + addr);
}

void TR_WRITE(struct sunxi_edp_hw_desc *edp_hw, u32 addr, u32 val)
{
	writel(val, edp_hw->reg_base + addr);
}

u32 TR_TOP_READ(struct sunxi_edp_hw_desc *edp_hw, u32 addr)
{
	return readl(edp_hw->top_base + addr);
}

void TR_TOP_WRITE(struct sunxi_edp_hw_desc *edp_hw, u32 addr, u32 val)
{
	writel(val, edp_hw->top_base + addr);
}

__maybe_unused static void TR_CLR_BITS(struct sunxi_edp_hw_desc *edp_hw, u32 reg, u32 shift, u32 width)
{
	u32 reg_val;

	reg_val = TR_READ(edp_hw, reg);
	reg_val = CLR_BITS(shift, width, reg_val);
	TR_WRITE(edp_hw, reg, reg_val);
}

static void TR_SET_BITS(struct sunxi_edp_hw_desc *edp_hw, u32 reg, u32 shift, u32 width, u32 val_mask)
{
	u32 reg_val;

	reg_val = TR_READ(edp_hw, reg);
	reg_val = SET_BITS(shift, width, reg_val, val_mask);
	TR_WRITE(edp_hw, reg, reg_val);
}

static u32 TR_GET_BITS(struct sunxi_edp_hw_desc *edp_hw, u32 reg, u32 shift, u32 width)
{
	u32 reg_val;

	reg_val = TR_READ(edp_hw, reg);
	reg_val = GET_BITS(shift, width, reg_val);

	return reg_val;
}

/* must set to disable  before link training
 * and set to enable after link training */
void trilinear_scrambling_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	TR_SET_BITS(edp_hw, TR_SCRAMBLING_DISABLE, 0, 1, enable ? 0x0 : 0x1);
}

void trilinear_mst_init(struct sunxi_edp_hw_desc *edp_hw, u32 mst_cnt)
{
	if (mst_cnt == 2)
		TR_SET_BITS(edp_hw, TR_INPUT_SOURCE_ENABLE, 0, 3, 0x3);
	else if (mst_cnt == 3)
		TR_SET_BITS(edp_hw, TR_INPUT_SOURCE_ENABLE, 0, 3, 0x7);
	else
		TR_SET_BITS(edp_hw, TR_INPUT_SOURCE_ENABLE, 0, 3, 0x1);
}

void trilinear_sst_init(struct sunxi_edp_hw_desc *edp_hw)
{
	// set mst source channel, only use channel 0
	TR_SET_BITS(edp_hw, TR_INPUT_SOURCE_ENABLE, 0, 3, 0x1);

//	TR_SET_BITS(edp_hw, TR_SOFT_RESET, 1, 1, 0x1);
	TR_SET_BITS(edp_hw, TR_TRANSMITTER_OUTPUT_EN, 0, 1, 0x1);
}

void trilinear_fec_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{

	TR_SET_BITS(edp_hw, TR_FEC_ENABLE, 0, 1, enable ? 0x1 : 0x0);
}


/* should use in HBR2 and intval should use the value read from DPCD_024A/024B */
void edp_hbr2_scrambler_config(struct sunxi_edp_hw_desc *edp_hw, u32 intval)
{
	TR_SET_BITS(edp_hw, TR_HBR2_SCRAMBLER_RESET, 0, 16, intval);
}

s32 trilinear_lane_remap_config(struct sunxi_edp_hw_desc *edp_hw, u32 lane_id, u32 remap_id)
{
	// put remap config into phy driver
	return RET_OK;
	if (lane_id < 0 || lane_id > 3) {
		EDP_WRN("lane remap: unsupport lane number!\n");
		return RET_FAIL;
	}

	if (remap_id < 0 || remap_id > 3) {
		EDP_WRN("lane remap: unsupport remap number!\n");
		return RET_FAIL;
	}

	switch (lane_id) {
	case 0:
		TR_SET_BITS(edp_hw, TR_LANE_REMAP_CONTROL, 0, 2, remap_id);
		break;
	case 1:
		TR_SET_BITS(edp_hw, TR_LANE_REMAP_CONTROL, 2, 2, remap_id);
		break;
	case 2:
		TR_SET_BITS(edp_hw, TR_LANE_REMAP_CONTROL, 4, 2, remap_id);
		break;
	case 3:
		TR_SET_BITS(edp_hw, TR_LANE_REMAP_CONTROL, 6, 2, remap_id);
		break;
	}

	/* enable lane remap in controller */
	TR_SET_BITS(edp_hw, TR_LANE_REMAP_CONTROL, 8, 1, 1);

	return RET_OK;
}

/* some markid should always invert because base-board lane has been inverted */
s32 trilinear_lane_invert_config(struct sunxi_edp_hw_desc *edp_hw, u32 lane_id, bool invert)
{
	// put invert config into phy driver
	return RET_OK;
	if (lane_id < 0 || lane_id > 4) {
		EDP_WRN("lane invert: unsupport lane number!\n");
		return RET_FAIL;
	}

	switch (lane_id) {
	case 0:
		TR_SET_BITS(edp_hw, TR_LANE_REMAP_CONTROL, 16, 1, invert ? 1 : 0);
		break;
	case 1:
		TR_SET_BITS(edp_hw, TR_LANE_REMAP_CONTROL, 17, 1, invert ? 1 : 0);
		break;
	case 2:
		TR_SET_BITS(edp_hw, TR_LANE_REMAP_CONTROL, 18, 1, invert ? 1 : 0);
		break;
	case 3:
		TR_SET_BITS(edp_hw, TR_LANE_REMAP_CONTROL, 19, 1, invert ? 1 : 0);
		break;
	}

	return RET_OK;
}

/*0:edp_mode   1:dp_mode*/
/* difference between dp and edp mode, is the lenth of aux sync header
 * dp mode lenth: 16
 * edp mode lenth: 8
 */
void trilinear_aux_init(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	u32 mode = edp_core->controller_mode;
	// FIXME: 200 refer to raw code from SHC
	u32 src_apb_clk = 200;

	// FIXME: pass from dts
	mode = 1;
	if (mode == AUX_EDP_MODE)
		TR_SET_BITS(edp_hw, TR_CAPBILITY_CONFIG, 1, 1, 0x1);
	else
		TR_SET_BITS(edp_hw, TR_CAPBILITY_CONFIG, 1, 1, 0x0);

	TR_SET_BITS(edp_hw, TR_AUX_CLOCK_DIVIDER, 0, 32, src_apb_clk);

	/* init aux reply timeout interval as 440ms */
	TR_SET_BITS(edp_hw, TR_AUX_REPLY_TIMEOUT_INTV, 0, 32, 440);
}

static bool edp_aux_ready(struct sunxi_edp_hw_desc *edp_hw)
{
	return TR_READ(edp_hw, TR_AUX_STATUS) & AUX_REQUEST_IN_PROGRESS ? false : true;
}

static bool edp_aux_reply_timeout(struct sunxi_edp_hw_desc *edp_hw)
{
	return TR_READ(edp_hw, TR_INTERRUPT_STATE) & REPLY_TIMEOUT ? true : false;
}

static bool edp_aux_reply_error(struct sunxi_edp_hw_desc *edp_hw)
{
	return TR_READ(edp_hw, TR_AUX_STATUS) & AUX_REPLY_ERROR ? true : false;
}

static bool edp_aux_reply_received(struct sunxi_edp_hw_desc *edp_hw)
{
	return (TR_READ(edp_hw, TR_AUX_STATUS) & AUX_REPLY_RECEIVED) ? true : false;
}

static int edp_aux_reply_code(struct sunxi_edp_hw_desc *edp_hw)
{
	return TR_READ(edp_hw, TR_AUX_REPLY_CODE);
}

int edp_wait_reply(struct sunxi_edp_hw_desc *edp_hw)
{
	unsigned int count = 0;

	while (1) {
		// reply timeout
		if (count > 5000) {
			EDP_ERR("edp wait aux retry timeout!\n");
			return RET_AUX_TIMEOUT;
		} else if (edp_aux_reply_timeout(edp_hw)) {
			EDP_ERR("edp wait aux reply timeout!\n");
			return RET_AUX_TIMEOUT;
		} else if (edp_aux_reply_error(edp_hw)) {
			EDP_ERR("edp aux reply error!\n");
			return RET_AUX_RPLY_ERR;
		} else if (edp_aux_reply_received(edp_hw))
			return RET_OK;

		count++;
	}

	return RET_OK;
}

int edp_send_aux_request(struct sunxi_edp_hw_desc *edp_hw,
			 struct edp_aux_request *request)
{
	unsigned int count = 0;
	int i = 0;
	int cmd;

	while (!edp_aux_ready(edp_hw)) {
		if (count > 5000) {
			EDP_ERR("last aux reply request timeout, aux request fail now!\n");
			return RET_FAIL;
		}
		count++;
	}

	TR_WRITE(edp_hw, TR_AUX_ADDRESS, request->address);

	if (request->cmd_code == TR_DP_CMD_REQUEST_WRITE ||
	    request->cmd_code == TR_DP_CMD_REQUEST_I2C_WRITE ||
	    request->cmd_code == TR_DP_CMD_REQUEST_I2C_WRITE_MOT) {
		for (i = 0; i < request->data_len; i++)
			TR_WRITE(edp_hw, TR_AUX_WRITE_FIFO, request->data[i]);
	}

	cmd = request->cmd_code;

	if (request->address_only)
		cmd |= TR_DP_CMD_REQUEST_ADDRESS_ONLY;

	cmd |= request->data_len - 1;

	EDP_LOW_DBG("[%s] aux_cmd: 0x%x addr:0x%x\n", __func__, cmd, request->address);
	TR_WRITE(edp_hw, TR_AUX_COMMAND, cmd);

	return RET_OK;
}

int edp_aux_read(struct sunxi_edp_hw_desc *edp_hw,
		 u32 addr, u32 len, char *buf)
{
	struct edp_aux_request request;
	int ret = 0, i = 0;
	int reply_code = 0;

	mutex_lock(&edp_hw->aux_lock);

	memset(&request, 0, sizeof(struct edp_aux_request));
	request.cmd_code = TR_DP_CMD_REQUEST_READ;
	request.address_only = false;
	request.address = addr;
	request.data_len = 16;

	edp_send_aux_request(edp_hw, &request);

	ret = edp_wait_reply(edp_hw);
	if (ret != RET_OK) {
		EDP_LOW_DBG("[%s] wait reply fail\n", __func__);
		mutex_unlock(&edp_hw->aux_lock);
		return ret;
	}

	reply_code = edp_aux_reply_code(edp_hw);
	EDP_LOW_DBG("[%s] reply_code:0x%x\n", __func__, reply_code);
	switch (reply_code) {
	case AUX_ACK:
		for (i = 0; i < request.data_len; i++) {
			/* add limit to avoid memory exceed */
			if (i <= len) {
				buf[i] = TR_READ(edp_hw, TR_AUX_REPLY_DATA_FIFO);
				EDP_LOW_DBG("[%s] result: buf[%d] = 0x%x\n", __func__, i, buf[i]);
			}
		}
		ret = RET_OK;
		goto OUT;
	case NATIVE_AUX_NACK:
		ret = RET_AUX_NACK;
		goto OUT;
	case NATIVE_AUX_DEFER:
		ret = RET_AUX_DEFER;
		goto OUT;
	}
OUT:
	mutex_unlock(&edp_hw->aux_lock);
	return ret;
}

int edp_aux_write(struct sunxi_edp_hw_desc *edp_hw,
		  u32 addr, u32 len, char *buf)
{
	struct edp_aux_request request;
	int ret = 0, i = 0;
	int reply_code;

	mutex_lock(&edp_hw->aux_lock);

	memset(&request, 0, sizeof(struct edp_aux_request));
	request.cmd_code = TR_DP_CMD_REQUEST_WRITE;
	request.address_only = false;
	request.address = addr;
	request.data_len = len;

	for (i = 0; i < request.data_len; i++) {
		request.data[i] = buf[i];
		EDP_LOW_DBG("[%s] want_write: buf[%d] = 0x%x\n", __func__, i, buf[i]);
	}

	edp_send_aux_request(edp_hw, &request);

	ret = edp_wait_reply(edp_hw);
	if (ret != RET_OK) {
		EDP_LOW_DBG("[%s] wait reply fail\n", __func__);
		mutex_unlock(&edp_hw->aux_lock);
		return ret;
	}

	reply_code = edp_aux_reply_code(edp_hw);
	EDP_LOW_DBG("[%s] reply_code:0x%x\n", __func__, reply_code);
	switch (reply_code) {
	case AUX_ACK:
		ret = RET_OK;
		goto OUT;
	case NATIVE_AUX_NACK:
		ret = RET_AUX_NACK;
		goto OUT;
	case NATIVE_AUX_DEFER:
		ret = RET_AUX_DEFER;
		goto OUT;
	}
OUT:
	mutex_unlock(&edp_hw->aux_lock);
	return ret;
}

int edp_aux_i2c_read(struct sunxi_edp_hw_desc *edp_hw,
		     u32 i2c_addr, u32 addr, u32 len, char *buf, bool mot)
{
	struct edp_aux_request request;
	int ret = 0, i = 0;
	int reply_code;

	mutex_lock(&edp_hw->aux_lock);

	memset(&request, 0, sizeof(struct edp_aux_request));
	request.address = i2c_addr;
	request.data_len = 16;
	if (mot)
		request.cmd_code = TR_DP_CMD_REQUEST_I2C_READ_MOT;
	else
		request.cmd_code = TR_DP_CMD_REQUEST_I2C_READ;

	edp_send_aux_request(edp_hw, &request);
	ret = edp_wait_reply(edp_hw);
	if (ret != RET_OK) {
		EDP_LOW_DBG("[%s] wait reply fail\n", __func__);
		mutex_unlock(&edp_hw->aux_lock);
		return ret;
	}

	reply_code = edp_aux_reply_code(edp_hw);
	EDP_LOW_DBG("[%s] reply_code:0x%x\n", __func__, reply_code);
	switch (reply_code) {
	case AUX_ACK:
		for (i = 0; i < request.data_len; i++) {
			if (i <= len) {
				buf[i] = TR_READ(edp_hw, TR_AUX_REPLY_DATA_FIFO);
				EDP_LOW_DBG("[%s] result: buf[%d] = 0x%x\n", __func__, i, buf[i]);
			}
		}
		ret = RET_OK;
		break;
	case I2C_AUX_NACK:
	case NATIVE_AUX_NACK:
		ret = RET_I2C_AUX_NACK;
		goto OUT;
	case I2C_AUX_DEFER:
	case NATIVE_AUX_DEFER:
		ret = RET_I2C_AUX_DEFER;
		goto OUT;
	}


OUT:
	mutex_unlock(&edp_hw->aux_lock);
	return ret;
}

int edp_aux_i2c_write(struct sunxi_edp_hw_desc *edp_hw,
		      u32 i2c_addr, u32 addr, u32 len, char *buf, bool mot)
{
	struct edp_aux_request request;
	int ret = 0, i = 0;
	int reply_code;

	mutex_lock(&edp_hw->aux_lock);

	memset(&request, 0, sizeof(struct edp_aux_request));
	request.cmd_code = TR_DP_CMD_REQUEST_I2C_WRITE_MOT;
	request.address = i2c_addr;
	request.data_len = len;

	if (!mot) {
		/* if is the last once transmit, use I2C_WRITE but not
		 * I2C_WRITE_MOT*/
		request.cmd_code = TR_DP_CMD_REQUEST_I2C_WRITE;
	}

	for (i = 0; i < request.data_len; i++) {
		request.data[i] = buf[i];
		EDP_LOW_DBG("[%s] want_write: buf[%d] = 0x%x\n", __func__, i, buf[i]);
	}

	edp_send_aux_request(edp_hw, &request);

	ret = edp_wait_reply(edp_hw);
	if (ret != RET_OK) {
		EDP_LOW_DBG("[%s] wait reply fail\n", __func__);
		mutex_unlock(&edp_hw->aux_lock);
		return ret;
	}

	reply_code = edp_aux_reply_code(edp_hw);
	switch (reply_code) {
	case AUX_ACK:
		ret = RET_OK;
		goto OUT;
	case I2C_AUX_NACK:
	case NATIVE_AUX_NACK:
		ret = RET_I2C_AUX_NACK;
		goto OUT;
	case I2C_AUX_DEFER:
	case NATIVE_AUX_DEFER:
		ret = RET_I2C_AUX_DEFER;
		goto OUT;
	}

OUT:
	mutex_unlock(&edp_hw->aux_lock);
	return ret;
}

s32 trilinear_aux_read(struct sunxi_edp_hw_desc *edp_hw,
		     s32 addr, s32 len, char *buf)
{
	int ret = 0, j = 0;
	int len_rest;
	int block = 16;
	u32 ext = (len % block) ? 1 : 0;
	u32 retry_cnt = 0;

	len_rest = len;
	for (j = 0; j < (len / block + ext); j++) {
		retry_cnt = 0;
		while (retry_cnt < 7) {
			ret = edp_aux_read(edp_hw, addr + (j * block), (len_rest > block) ? block : len_rest,
					       buf + (j * block));

			/*
			 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
			 * AUX_TIMEOUT, AUX_NO_STOP
			 */
			if ((ret != RET_AUX_NACK) &&
			    (ret != RET_AUX_TIMEOUT) &&
			    (ret != RET_AUX_RPLY_ERR) &&
			    (ret != RET_AUX_DEFER) &&
			    (ret != RET_I2C_AUX_NACK) &&
			    (ret != RET_I2C_AUX_DEFER) &&
			    (ret != RET_AUX_NO_STOP))
				break;
			/* at least 400us between two request is request in dp cts */
			usleep_range(500, 550);
			retry_cnt++;
			EDP_LOW_DBG("%s fail(%s), addr:0x%x len:16, retry (%d) times now\n",
				     __func__, aux_fail_reason_str[RET_OP(ret)], (addr + j * block), retry_cnt);
		}

		if (ret != RET_OK) {
			EDP_ERR("%s retry 7 timies but still %s, request(addr:0x%x len:16)\n",
			    __func__, aux_fail_reason_str[RET_OP(ret)], (addr + j * block));

			return ret;
		}
		len_rest -= block;
	}

	return ret;
}

s32 trilinear_aux_write(struct sunxi_edp_hw_desc *edp_hw,
			s32 addr, s32 len, char *buf)
{
	int ret = 0, j = 0;
	int len_rest;
	int block = 16;
	u32 ext = (len % block) ? 1 : 0;
	u32 retry_cnt = 0;

	len_rest = len;
	for (j = 0; j < (len / block + ext); j++) {
		retry_cnt = 0;
		while (retry_cnt < 7) {
			ret = edp_aux_write(edp_hw, addr + (j * block), (len_rest > block) ? block : len_rest,
					       buf + (j * block));

			/*
			 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
			 * AUX_TIMEOUT, AUX_NO_STOP
			 */
			if ((ret != RET_AUX_NACK) &&
			    (ret != RET_AUX_TIMEOUT) &&
			    (ret != RET_AUX_RPLY_ERR) &&
			    (ret != RET_AUX_DEFER) &&
			    (ret != RET_I2C_AUX_NACK) &&
			    (ret != RET_I2C_AUX_DEFER) &&
			    (ret != RET_AUX_NO_STOP))
				break;
			/* at least 400us between two request is request in dp cts */
			usleep_range(500, 550);
			retry_cnt++;
			EDP_LOW_DBG("%s fail(%s), addr:0x%x len:16, retry (%d) times now\n",
				     __func__, aux_fail_reason_str[RET_OP(ret)], (addr + j * block), retry_cnt);
		}

		if (ret != RET_OK) {
			EDP_ERR("%s retry 7 timies but still %s, request(addr:0x%x len:16)\n",
			    __func__, aux_fail_reason_str[RET_OP(ret)], (addr + j * block));

			return ret;
		}
		len_rest -= block;
	}

	return ret;
}

s32 trilinear_aux_i2c_read(struct sunxi_edp_hw_desc *edp_hw,
			   s32 i2c_addr, s32 addr, s32 len, char *buf)
{
	struct edp_aux_request request;
	int block = 16;
	int ret = 0, j = 0;
	u32 ext = (len % block) ? 1 : 0;
	u32 retry_cnt = 0;
	int len_rest;
	bool mot;

	len_rest = len;
	mutex_lock(&edp_hw->aux_lock);
	memset(&request, 0, sizeof(struct edp_aux_request));
	request.cmd_code = TR_DP_CMD_REQUEST_I2C_WRITE_MOT;
	request.address = i2c_addr;
	//request.address_only = true;
	request.data_len = 1;
	request.data[0] = addr;
	request.data[1] = 0;
	while (retry_cnt < 7) {
		edp_send_aux_request(edp_hw, &request);

		ret = edp_wait_reply(edp_hw);
		if (ret == RET_OK)
			break;

		retry_cnt++;
		EDP_LOW_DBG("i2c read fail in write-i2c-addr(err:%s), addr:0x%x len:16 retry (%d) times now\n",
			    aux_fail_reason_str[RET_OP(ret)], (addr + j * block), retry_cnt);
		/* at least 400us between two request is request in dp cts */
		usleep_range(500, 550);
	}
	mutex_unlock(&edp_hw->aux_lock);
	if (ret != RET_OK)
		return ret;


	for (j = 0; j < (len / block + ext); j++) {
		retry_cnt = 0;
		while (retry_cnt < 7) {
			if ((j == ((len / block) - 1)) || (j == ((len / block))))
				mot = false;
			else
				mot = true;
			ret = edp_aux_i2c_read(edp_hw, i2c_addr, addr, (len_rest > block) ? block : len_rest,
					       buf + (j * block), mot);

			/*
			 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
			 * AUX_TIMEOUT, AUX_NO_STOP
			 */
			if ((ret != RET_AUX_NACK) &&
			    (ret != RET_AUX_TIMEOUT) &&
			    (ret != RET_AUX_RPLY_ERR) &&
			    (ret != RET_AUX_DEFER) &&
			    (ret != RET_I2C_AUX_NACK) &&
			    (ret != RET_I2C_AUX_DEFER) &&
			    (ret != RET_AUX_NO_STOP))
				break;
			/* at least 400us between two request is request in dp cts */
			usleep_range(500, 550);
			retry_cnt++;
			EDP_LOW_DBG("%s fail(%s), addr:0x%x len:16, retry (%d) times now\n",
				     __func__, aux_fail_reason_str[RET_OP(ret)], (addr + j * block), retry_cnt);
		}

		if (ret != RET_OK) {
			EDP_ERR("%s retry 7 timies but still %s, request(i2c_addr:0x%x addr:0x%x len:16)\n",
			    __func__, aux_fail_reason_str[RET_OP(ret)], i2c_addr, (addr + j * block));

			return ret;
		}
		len_rest -= block;
	}

	return ret;
}

s32 trilinear_aux_i2c_write(struct sunxi_edp_hw_desc *edp_hw,
			    s32 i2c_addr, s32 addr, s32 len, char *buf)
{
	struct edp_aux_request request;
	int block = 16;
	int ret = 0, j = 0;
	u32 ext = (len % block) ? 1 : 0;
	u32 retry_cnt = 0;
	int len_rest;
	bool mot;

	mutex_lock(&edp_hw->aux_lock);
	memset(&request, 0, sizeof(struct edp_aux_request));

	len_rest = len;
	request.cmd_code = TR_DP_CMD_REQUEST_I2C_WRITE_MOT;
	request.address = i2c_addr;
	//request.address_only = true;
	request.data_len = 1;
	request.data[0] = addr;
	request.data[1] = 0;

	while (retry_cnt < 7) {
		edp_send_aux_request(edp_hw, &request);

		ret = edp_wait_reply(edp_hw);
		if (ret == RET_OK)
			break;

		retry_cnt++;
		EDP_LOW_DBG("i2c write fail in write-i2c-addr(err:%s), addr:0x%x len:16 retry (%d) times now\n",
			    aux_fail_reason_str[RET_OP(ret)], (addr + j * block), retry_cnt);
		/* at least 400us between two request is request in dp cts */
		usleep_range(500, 550);
	}
	mutex_unlock(&edp_hw->aux_lock);
	if (ret != RET_OK)
		return ret;

	for (j = 0; j < (len / block + ext); j++) {
		retry_cnt = 0;
		while (retry_cnt < 7) {
			if (len <= 16)
				mot = true;
			else if ((16 * j) < (len - 16))
				mot = true;
			else {
				/* if is the last once transmit, use I2C_WRITE but not
				 * I2C_WRITE_MOT*/
				mot = false;
			}

			ret = edp_aux_i2c_write(edp_hw, i2c_addr, addr, (len_rest > block) ? block : len_rest,
				  buf + (j * block), mot);

			/*
			 * for CTS 4.2.1.1, 4.2.2.5, add retry when AUX_NACK, AUX_DEFER,
			 * AUX_TIMEOUT, AUX_NO_STOP
			 */
			if ((ret != RET_AUX_NACK) &&
			    (ret != RET_AUX_TIMEOUT) &&
			    (ret != RET_AUX_RPLY_ERR) &&
			    (ret != RET_AUX_DEFER) &&
			    (ret != RET_I2C_AUX_NACK) &&
			    (ret != RET_I2C_AUX_DEFER) &&
			    (ret != RET_AUX_NO_STOP))
				break;
			/* at least 400us between two request is request in dp cts */
			usleep_range(500, 550);
			retry_cnt++;
			EDP_LOW_DBG("%s fail(%s), addr:0x%x len:16, retry (%d) times now\n",
				     __func__, aux_fail_reason_str[RET_OP(ret)], (addr + j * block), retry_cnt);
		}

		if (ret != RET_OK) {
			EDP_ERR("%s retry 7 timies but still %s, request(i2c_addr:0x%x addr:0x%x len:16)\n",
			    __func__, aux_fail_reason_str[RET_OP(ret)], i2c_addr, (addr + j * block));

			return ret;
		}
		len_rest -= block;
	}

	return ret;
}

s32 trilinear_read_edid_block(struct sunxi_edp_hw_desc *edp_hw,
			      u8 *raw_edid, unsigned int block_id, size_t len)
{
	unsigned int addr = block_id * EDID_LENGTH;

	trilinear_aux_i2c_read(edp_hw, EDID_ADDR, addr, len, (char *)(raw_edid));

	return RET_OK;
}


void trilinear_set_lane_rate(struct sunxi_edp_hw_desc *edp_hw, u64 bit_rate)
{
	switch (bit_rate) {
	case BIT_RATE_1G62:
		TR_SET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8, 0x6);
		break;
	case BIT_RATE_2G16:
		TR_SET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8, 0x8);
		break;
	case BIT_RATE_2G43:
		TR_SET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8, 0x9);
		break;
	case BIT_RATE_2G7:
		TR_SET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8, 0xA);
		break;
	case BIT_RATE_3G24:
		TR_SET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8, 0xC);
		break;
	case BIT_RATE_4G32:
		TR_SET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8, 0x10);
		break;
	case BIT_RATE_5G4:
		TR_SET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8, 0x14);
		break;
	case BIT_RATE_8G1:
		TR_SET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8, 0x1E);
		break;
	default:
		/* if not configure, set 2.7G as default */
		TR_SET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8, 0xA);
		break;
	}
}

void trilinear_set_lane_cnt(struct sunxi_edp_hw_desc *edp_hw, u32 lane_cnt)
{
	if ((lane_cnt < 0) || (lane_cnt > 4)) {
		EDP_WRN("unsupport lane number!\n");
	}

	switch (lane_cnt) {
	case 0:
	case 3:
		EDP_WRN("edp lane number can not be configed to 0/3!\n");
		break;
	case 1:
		TR_SET_BITS(edp_hw, TR_LANE_COUNT_SET, 0, 5, 0x1);
		break;
	case 2:
		TR_SET_BITS(edp_hw, TR_LANE_COUNT_SET, 0, 5, 0x2);
		break;
	case 4:
		TR_SET_BITS(edp_hw, TR_LANE_COUNT_SET, 0, 5, 0x4);
		break;
	}
}

void edp_set_secondary_data_window(struct sunxi_edp_hw_desc *edp_hw, u64 bit_rate, u32 pixel_clk)
{
	u32 window_size = 0;
	u32 link_rate;
	u32 hstart;

	link_rate = bit_rate / 10000 / 2;

	hstart = TR_READ(edp_hw, TR_SRC0_MAIN_STREAM_HSTART);

	// determine the window in link clocks
	window_size = (hstart * link_rate) / pixel_clk;

	TR_WRITE(edp_hw, TR_SRC0_SECOND_DATA_WINDOW, window_size);
}

void edp_video_stream_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_SET_BITS(edp_hw, TR_VIDEO_STREAM_ENABLE, 0, 1, 0x1);
	TR_SET_BITS(edp_hw, TR_SECOND_STREAM_ENABLE, 0, 1, 0x1);
}

void edp_video_stream_disable(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_SET_BITS(edp_hw, TR_VIDEO_STREAM_ENABLE, 0, 1, 0x0);
//	TR_SET_BITS(edp_hw, TR_SECOND_STREAM_ENABLE, 0, 1, 0x0);
}

void trilinear_set_qual_pattern(struct sunxi_edp_hw_desc *edp_hw, u32 pattern, u32 lane_cnt)
{
	switch (lane_cnt) {
	case 1:
		TR_SET_BITS(edp_hw, TR_LINK_QUAL_PATTERN_SET, 0, 3, pattern);
		break;
	case 2:
		TR_SET_BITS(edp_hw, TR_LINK_QUAL_PATTERN_SET, 0, 3, pattern);
		TR_SET_BITS(edp_hw, TR_LINK_QUAL_PATTERN_SET, 3, 3, pattern);
		break;
	case 4:
		TR_SET_BITS(edp_hw, TR_LINK_QUAL_PATTERN_SET, 0, 3, pattern);
		TR_SET_BITS(edp_hw, TR_LINK_QUAL_PATTERN_SET, 8, 3, pattern);
		TR_SET_BITS(edp_hw, TR_LINK_QUAL_PATTERN_SET, 16, 3, pattern);
		TR_SET_BITS(edp_hw, TR_LINK_QUAL_PATTERN_SET, 24, 3, pattern);
		break;
	}
}

static void trilinear_timer_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	if (enable)
		TR_SET_BITS(edp_hw, TR_GP_HOST_TIMER, 31, 1, 0x1);
	else
		TR_SET_BITS(edp_hw, TR_GP_HOST_TIMER, 31, 1, 0x0);
}

void trilinear_timer_init(struct sunxi_edp_hw_desc *edp_hw)
{
	trilinear_timer_enable(edp_hw, false);
}

void trilinear_hpd_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_SET_BITS(edp_hw, TR_INTERRUPT_MASK, 0, 7, 0);
}

void trilinear_hpd_disable(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_SET_BITS(edp_hw, TR_INTERRUPT_MASK, 0, 7, 0x7ff);
}

bool trilinear_get_hotplug_change(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val = 0;

	reg_val = TR_READ(edp_hw, TR_INTERRUPT_STATE);
	if (reg_val & HPD_EVENT)
		return true;
	else
		return false;

	return false;
}

s32 trilinear_get_hotplug_state(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val = 0;

	reg_val = TR_READ(edp_hw, TR_HPD_INPUT_STATE);
	if (reg_val == 0x1)
		return 1;
	else
		return 0;

	return 0;
}

void trilinear_irq_handler(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	u32 reg_val = 0;

	// clear all interrupt state at the end of irq handler
	reg_val = TR_READ(edp_hw, TR_INTERRUPT_CAUSE);
}

void trilinear_set_training_pattern(struct sunxi_edp_hw_desc *edp_hw,
				    u32 pattern)
{
	TR_SET_BITS(edp_hw, TR_TRAINING_PATTERN_SET, 0, 3, pattern);
}

void trilinear_dsc_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	if (enable)
		TR_SET_BITS(edp_hw, TR_DSC_COMPRESSION_EN, 0, 1, 0x1);
	else
		TR_SET_BITS(edp_hw, TR_DSC_COMPRESSION_EN, 0, 1, 0x0);
}

static void trilinear_audio_set_interface(struct sunxi_edp_hw_desc *edp_hw, int interface)
{
	switch (interface) {
	case HDMI_SPDIF:
		TR_SET_BITS(edp_hw, TR_SEC0_AUDIO_INPUT_SEL, 0, 2, 0x3);
		break;
	default:
	case HDMI_I2S:
		TR_SET_BITS(edp_hw, TR_SEC0_AUDIO_INPUT_SEL, 0, 2, 0x0);
		break;
	}
}

static void trilinear_audio_set_channel_cnt(struct sunxi_edp_hw_desc *edp_hw, int chn_cnt)
{
	switch (chn_cnt) {
	case 2:
		//TR_SET_BITS(edp_hw, TR_SEC0_CHANNEL_CNT, 0, 32, 0x2);
		/* fix some displayport panel no audio ouput issue */
		TR_SET_BITS(edp_hw, TR_SEC0_CHANNEL_CNT, 0, 32, 0x3);
		break;
	case 5:
		TR_SET_BITS(edp_hw, TR_SEC0_CHANNEL_CNT, 0, 32, 0x6);
		break;
	default:
	case 8:
		TR_SET_BITS(edp_hw, TR_SEC0_CHANNEL_CNT, 0, 32, 0x8);
		break;
	}

}

static void trilinear_audio_set_mn_aud(struct sunxi_edp_hw_desc *edp_hw,
				       int link_bw, int audio_data_rate)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(aud_tbl); i++) {
		if ((aud_tbl[i].link_bw == link_bw) &&
		    (aud_tbl[i].data_rate == audio_data_rate)) {
			TR_SET_BITS(edp_hw, TR_SEC0_MAUD, 0, 32, aud_tbl[i].maud);
			TR_SET_BITS(edp_hw, TR_SEC0_NAUD, 0, 32, aud_tbl[i].naud);
			break;
		}
	}
}

static void trilinear_audio_set_data_rate(struct sunxi_edp_hw_desc *edp_hw, int audio_data_rate)
{
	int link_bw = 0;

	link_bw = TR_READ(edp_hw, TR_LINK_BW_SET) * 27;

	/* set audio clock mode to synchronous mode */
	TR_SET_BITS(edp_hw, TR_SEC0_AUDIO_CLOCK_MODE, 0, 32, 0x1);

	/* set MAUD NAUD */
	trilinear_audio_set_mn_aud(edp_hw, link_bw, audio_data_rate);
}

s32 trilinear_audio_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_SET_BITS(edp_hw, TR_SEC0_AUDIO_ENABLE, 0, 1, 0x1);

	return RET_OK;
}

s32 trilinear_audio_disable(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_SET_BITS(edp_hw, TR_SEC0_AUDIO_ENABLE, 1, 1, 0x1);

	return RET_OK;
}

s32 trilinear_audio_mute(struct sunxi_edp_hw_desc *edp_hw,
			 bool enable, int direction)
{
	TR_SET_BITS(edp_hw, TR_SEC0_AUDIO_ENABLE, 1, 1, enable ? 0x1 : 0x0);

	return RET_OK;
}

s32 trilinear_audio_config(struct sunxi_edp_hw_desc *edp_hw, int interface,
		      int chn_cnt, int data_width, int data_rate)
{
	trilinear_audio_set_interface(edp_hw, interface);
	trilinear_audio_set_channel_cnt(edp_hw, chn_cnt);
	trilinear_audio_set_data_rate(edp_hw, data_rate);
//	trilinear_audio_set_data_width(edp_hw, data_width);
	return RET_OK;
}

bool trilinear_audio_is_enabled(struct sunxi_edp_hw_desc *edp_hw)
{
	int reg_val = 0;

	reg_val = TR_GET_BITS(edp_hw, TR_SEC0_AUDIO_ENABLE, 0, 1);

	return reg_val ? true : false;
}

u32 trilinear_get_audio_if(struct sunxi_edp_hw_desc *edp_hw)
{
	int reg_val = 0;

	reg_val = TR_GET_BITS(edp_hw, TR_SEC0_AUDIO_INPUT_SEL, 0, 2);

	switch (reg_val) {
	case 0x3:
		return 0;
	default:
	case 0x0:
		return 1;
	}

	return 1;
}

bool trilinear_audio_is_muted(struct sunxi_edp_hw_desc *edp_hw)
{
	int reg_val = 0;

	reg_val = TR_GET_BITS(edp_hw, TR_SEC0_AUDIO_ENABLE, 1, 1);

	return reg_val ? true : false;
}

u32 trilinear_get_audio_max_channel(struct sunxi_edp_hw_desc *edp_hw)
{
	return 8;
}

u32 trilinear_get_audio_chn_cnt(struct sunxi_edp_hw_desc *edp_hw)
{
	int reg_val = 0;

	reg_val = TR_GET_BITS(edp_hw, TR_SEC0_CHANNEL_CNT, 0, 32);

	switch (reg_val) {
	case 2:
		return 2;
	case 6:
		return 5;
	case 8:
		return 8;
	}

	return 0;
}

u32 trilinear_get_audio_date_width(struct sunxi_edp_hw_desc *edp_hw)
{
	//TODO
	return 0;
}


void edp_controller_soft_reset(struct sunxi_edp_hw_desc *edp_hw)
{
//	TR_SET_BITS(edp_hw, TR_SOFT_RESET, 4, 1, 0x1);
//	TR_SET_BITS(edp_hw, TR_SOFT_RESET, 0, 1, 0x1);
//	TR_CLR_BITS(edp_hw, TR_TRANSMITTER_OUTPUT_EN, 0, 1);
}

s32 trilinear_irq_enable(struct sunxi_edp_hw_desc *edp_hw, u32 irq_id)
{
	return 0;
}

s32 trilinear_irq_disable(struct sunxi_edp_hw_desc *edp_hw, u32 irq_id)
{
	return 0;
}

s32 trilinear_transfer_unit_config(struct sunxi_edp_hw_desc *edp_hw,
			     u32 bpp, u32 lane_cnt, u64 bit_rate, u32 pixel_clk)
{
	u32 bw = 0;
	u32 tu_size = 0;
	u32 data_per_tu = 0;

	bw = lane_cnt * bit_rate / 10000;
	bw = bw / 1000;
//	bw = TR_READ(edp_hw, TR_LANE_COUNT_SET) * TR_READ(edp_hw, TR_LINK_BW_SET) * 27;

	if (bw != 0) {
		data_per_tu = (pixel_clk * 8 * bpp) / bw;
		tu_size = ((data_per_tu % 1000) * 16) / 1000;
		tu_size = (tu_size << 8) + (data_per_tu / 1000);
		/* set tu size to fixed 64 */
		tu_size = (tu_size << 16) | LS_PER_TU;
		TR_SET_BITS(edp_hw, TR_SRC0_TU_CONFIG, 0, 32, tu_size);
	}

	return RET_OK;
}

s32 trilinear_init_early(struct sunxi_edp_hw_desc *edp_hw)
{
	mutex_init(&edp_hw->aux_lock);

	return RET_OK;
}

void trilinear_top_init(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_TOP_WRITE(edp_hw, 0x0, 0x1);
	TR_TOP_WRITE(edp_hw, 0x4, 0x1);
}

s32 trilinear_controller_init(struct sunxi_edp_hw_desc *edp_hw,
			      struct edp_tx_core *edp_core)
{
	s32 ret = 0;

	trilinear_top_init(edp_hw);
	edp_controller_soft_reset(edp_hw);
	trilinear_timer_init(edp_hw);
	trilinear_hpd_enable(edp_hw);
	trilinear_aux_init(edp_hw, edp_core);
	trilinear_sst_init(edp_hw);

	usleep_range(500, 1000);

	return ret;
}

s32 trilinear_enable(struct sunxi_edp_hw_desc *edp_hw,
		     struct edp_tx_core *edp_core)
{
	return RET_OK;
}

s32 trilinear_disable(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	edp_video_stream_disable(edp_hw);

	return 0;
}

u64 trilinear_get_max_rate(struct sunxi_edp_hw_desc *edp_hw)
{
	return BIT_RATE_5G4;
}

u32 trilinear_get_max_lane(struct sunxi_edp_hw_desc *edp_hw)
{
	return 4;
}

void trilinear_set_video_timestamp(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	/* set MVID */
	// not sure divide 1000 or 100000, spec seems conflict
	TR_SET_BITS(edp_hw, TR_SRC0_MVID, 0, 24, edp_core->timings.pixel_clk / 1000);

	/* set NVID */
	TR_SET_BITS(edp_hw, TR_SRC0_NVID, 0, 24, edp_core->lane_para.bit_rate / 10000);
}

s32 trilinear_set_misc(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	u32 colordepth = edp_core->lane_para.colordepth;
	u32 color_fmt = edp_core->lane_para.color_fmt;
	u32 colormetry = edp_core->lane_para.colormetry;
	u32 sync_clock = edp_core->sync_clock;
	u32 reg_val = 0;
	u32 format_val = 0, depth_val = 0;
	u32 metry_val = 0, sync_val = 0;

	/* 0:RGB  1:YUV444  2:YUV422  3:YUV420 */
	if (color_fmt == 0) {
		format_val = (0x0 << 1);
	} else if (color_fmt == 1) {
		format_val = (0x2 << 1);
	} else if (color_fmt == 2) {
		format_val = (0x1 << 1);
	} else if (color_fmt == 3) {
		/* enable misc0/misc1 override and override color_space as yuv420 */
		TR_SET_BITS(edp_hw, TR_SRC0_COLORMETRY, 3, 4, 0xc0);
	} else {
		EDP_ERR("color format is not support!");
		return RET_FAIL;
	}

	if (colordepth == 6) {
		depth_val = (0x0 << 5);
	} else if (colordepth == 8) {
		depth_val = (0x1 << 5);
	} else if (colordepth == 10) {
		depth_val = (0x2 << 5);
	} else if (colordepth == 12) {
		depth_val = (0x3 << 5);
	} else if (colordepth == 16) {
		depth_val = (0x4 << 5);
	} else {
		EDP_ERR("color depth is not support!");
		return RET_FAIL;
	}

	/* 0:BT601   1:BT709 */
	if (colormetry == 0) {
		metry_val = (0x0 << 4);
	} else if (colormetry == 1) {
		metry_val = (0x1 << 4);
	} else {
		EDP_ERR("colormetry is not support!");
		return RET_FAIL;
	}

	/* sync clock. 0:async  1:sync */
	if (sync_clock == 0)
		sync_val = (0x0 << 0);
	else
		sync_val = (0x1 << 0);

	reg_val = format_val| depth_val | metry_val | sync_val;

	/* set misc0 */
	TR_SET_BITS(edp_hw, TR_SRC0_MAIN_STREAM_MISC0, 0, 8, reg_val);

	/* set misc1 */
	// TODO: add it when VSC/stereo/interlace is need

	return RET_OK;
}

s32 trilinear_set_pixel_mode(struct sunxi_edp_hw_desc *edp_hw, u32 pixel_cnt)
{
	u32 pixel_count = pixel_cnt;

	/* set default pixel mode to 1 pixel */
	if (pixel_count == 0)
		pixel_count = 1;

	if (pixel_count != 1 && pixel_count != 2 && pixel_count != 4) {
		EDP_ERR("pixel mode:%d not support!\n", pixel_count);
		return RET_FAIL;
	}

	TR_SET_BITS(edp_hw, TR_SRC0_USER_PIXEL_CNT, 0, 3, pixel_count);

	return RET_OK;
}

u32 trilinear_get_pixel_mode(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;

	reg_val = TR_GET_BITS(edp_hw, TR_SRC0_USER_PIXEL_CNT, 0, 31);

	return reg_val;
}

s32 trilinear_set_video_data_count(struct sunxi_edp_hw_desc *edp_hw, u32 hres, u32 bpp, u32 lane_cnt)
{
	u32 symbol_cnt = 0;
	u32 data_cnt = 0;

	symbol_cnt = ((hres * bpp) + 7) / 8;
	data_cnt = (symbol_cnt + lane_cnt - 1) / lane_cnt;

	TR_SET_BITS(edp_hw, TR_SRC0_USER_DATA_CNT, 0, 18, data_cnt);

	return RET_OK;
}

s32 trilinear_set_video_interlace(struct sunxi_edp_hw_desc *edp_hw, bool interlace)
{
	TR_SET_BITS(edp_hw, TR_SRC0_MAIN_STREAM_INTERLACE, 0, 1,
		    interlace ? 0x1 : 0x0);

	return RET_OK;
}

s32 trilinear_set_video_format(struct sunxi_edp_hw_desc *edp_hw, struct edp_tx_core *edp_core)
{
	s32 ret = 0;
	u32 lane_cnt = edp_core->lane_para.lane_cnt;
	u32 bpp = edp_core->lane_para.bpp;
	u32 hres = edp_core->timings.x_res;

	trilinear_set_video_timestamp(edp_hw, edp_core);

	ret = trilinear_set_misc(edp_hw, edp_core);
	if (ret)
		return ret;

	ret = trilinear_set_video_data_count(edp_hw, hres, bpp, lane_cnt);
	if (ret)
		return ret;

	ret = trilinear_set_video_interlace(edp_hw, edp_core->interlace);
	if (ret)
		return ret;

	return RET_OK;
}

s32 trilinear_set_video_timings(struct sunxi_edp_hw_desc *edp_hw,
				struct disp_video_timings *timings)
{
	/* set horizon timings */
	TR_WRITE(edp_hw, TR_SRC0_MAIN_STREAM_HTOTAL, timings->hor_total_time);
	TR_WRITE(edp_hw, TR_SRC0_MAIN_STREAM_HSW, timings->hor_sync_time);
	TR_WRITE(edp_hw, TR_SRC0_MAIN_STREAM_HRES, timings->x_res);
	TR_WRITE(edp_hw, TR_SRC0_MAIN_STREAM_HSTART, timings->hor_sync_time + timings->hor_back_porch);

	/* set vertical timings */
	TR_WRITE(edp_hw, TR_SRC0_MAIN_STREAM_VTOTAL, timings->ver_total_time);
	TR_WRITE(edp_hw, TR_SRC0_MAIN_STREAM_VSW, timings->ver_sync_time);
	TR_WRITE(edp_hw, TR_SRC0_MAIN_STREAM_VRES, timings->y_res);
	TR_WRITE(edp_hw, TR_SRC0_MAIN_STREAM_VSTART, timings->ver_sync_time + timings->ver_back_porch);

	/* set timings poloarity*/
	TR_SET_BITS(edp_hw, TR_SRC0_MAIN_STREAM_POLAR, 0, 1, timings->hor_sync_polarity);
	TR_SET_BITS(edp_hw, TR_SRC0_MAIN_STREAM_POLAR, 1, 1, timings->ver_sync_polarity);

	return RET_OK;
}

s32 trilinear_set_transfer_config(struct sunxi_edp_hw_desc *edp_hw,
				  struct edp_tx_core *edp_core)
{
	struct disp_video_timings *tmgs = &edp_core->timings;
	u32 pixel_clk = tmgs->pixel_clk;
	u64 bit_rate = edp_core->lane_para.bit_rate;
	u32 lane_cnt = edp_core->lane_para.lane_cnt;
	u32 bpp = edp_core->lane_para.bpp;
	s32 ret;

	edp_set_secondary_data_window(edp_hw, bit_rate, pixel_clk / 1000);

	ret = trilinear_transfer_unit_config(edp_hw, bpp, lane_cnt, bit_rate, pixel_clk / 1000);
	if (ret)
		return ret;

	return RET_OK;
}

void trilinear_crc_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	TR_SET_BITS(edp_hw, TR_SRC0_EDP_CRC_ENABLE, 0, 1, enable ? 0x1 : 0x0);
}

void trilinear_psr_vsc_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	TR_SET_BITS(edp_hw, TR_SRC0_PSR_3D_ENABLE, 0, 1, enable ? 0x1 : 0x0);
}

void trilinear_psr_capture_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	TR_SET_BITS(edp_hw, TR_SRC0_PSR_STATE, 0, 1, enable ? 0x1 : 0x0);
	TR_SET_BITS(edp_hw, TR_SRC0_PSR_STATE, 1, 1, enable ? 0x1 : 0x0);
}

void trilinear_psr2_capture_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	TR_SET_BITS(edp_hw, TR_SRC0_PSR_STATE, 0, 1, enable ? 0x1 : 0x0);
	TR_SET_BITS(edp_hw, TR_SRC0_PSR_STATE, 1, 1, enable ? 0x1 : 0x0);
	TR_SET_BITS(edp_hw, TR_SRC0_PSR_STATE, 2, 1, enable ? 0x1 : 0x0);
}

s32 trilinear_psr_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	/* not sure if VSC is need for PSR1 */
	//trilinear_psr_vsc_enable(edp_hw, enable);
	trilinear_crc_enable(edp_hw, enable);
	trilinear_psr_capture_enable(edp_hw, enable);
	return RET_OK;
}

bool trilinear_psr_is_enabled(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;

	reg_val = TR_GET_BITS(edp_hw, TR_SRC0_PSR_STATE, 0, 1);
	reg_val1 = TR_GET_BITS(edp_hw, TR_SRC0_PSR_STATE, 1, 1);

	if (reg_val && reg_val1)
		return true;
	else
		return false;
}

bool trilinear_psr2_is_enabled(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;
	u32 reg_val2 = 0;

	reg_val = TR_GET_BITS(edp_hw, TR_SRC0_PSR_STATE, 0, 1);
	reg_val1 = TR_GET_BITS(edp_hw, TR_SRC0_PSR_STATE, 1, 1);
	reg_val2 = TR_GET_BITS(edp_hw, TR_SRC0_PSR_STATE, 2, 1);

	if (reg_val && reg_val1 && reg_val2)
		return true;
	else
		return false;
}

void trilinear_psr2_set_area(struct sunxi_edp_hw_desc *edp_hw,
			     u32 top, u32 bot, u32 left, u32 width)
{
	TR_SET_BITS(edp_hw, TR_PSR2_UPDATE_TOP, 0, 16, top);
	TR_SET_BITS(edp_hw, TR_PSR2_UPDATE_BOTTOM, 0, 16, bot);
	TR_SET_BITS(edp_hw, TR_PSR2_UPDATE_LEFT, 0, 16, left);
	TR_SET_BITS(edp_hw, TR_PSR2_UPDATE_WIDTH, 0, 16, width);
}

void trilinear_psr2_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	trilinear_psr_vsc_enable(edp_hw, enable);
	trilinear_crc_enable(edp_hw, enable);
	trilinear_psr2_capture_enable(edp_hw, enable);
}

s32 trilinear_assr_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	TR_SET_BITS(edp_hw, TR_CAPBILITY_CONFIG, 0, 1, enable ? 1 : 0);

	return RET_OK;
}

void edp_set_80bit_pattern(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_WRITE(edp_hw, TR_80BIT_PATTERN_0_31, 0x3E0F83E0);
	TR_WRITE(edp_hw, TR_80BIT_PATTERN_32_63, 0x0F83E0F8);
	TR_WRITE(edp_hw, TR_80BIT_PATTERN_64_79, 0xF83E);
}

s32 trilinear_set_pattern(struct sunxi_edp_hw_desc *edp_hw, u32 pattern, u32 lane_cnt)
{
	switch (pattern) {
	case D10_2:
		trilinear_set_qual_pattern(edp_hw, 0x1, lane_cnt);
		break;
	case SYMBOL_MEASURE_PATTERN:
		trilinear_set_qual_pattern(edp_hw, 0x2, lane_cnt);
		break;
	case PRBS7:
		trilinear_set_qual_pattern(edp_hw, 0x3, lane_cnt);
		break;
	case PATTERN_80BIT:
		trilinear_set_qual_pattern(edp_hw, 0x4, lane_cnt);
		edp_set_80bit_pattern(edp_hw);
		break;
	case HBR2_EYE:
		trilinear_set_qual_pattern(edp_hw, 0x5, lane_cnt);
		break;
	case CP2520_PATTERN2:
		trilinear_set_qual_pattern(edp_hw, 0x6, lane_cnt);
		break;
	case CP2520_PATTERN3:
		trilinear_set_qual_pattern(edp_hw, 0x7, lane_cnt);
		break;
	default:
		trilinear_set_training_pattern(edp_hw, pattern);
		break;
	}
	return RET_OK;
}

s32 trilinear_get_color_fmt(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 color_depth;
	u32 format;

	color_depth = TR_GET_BITS(edp_hw, TR_SRC0_MAIN_STREAM_MISC0, 5, 3);
	format = TR_GET_BITS(edp_hw, TR_SRC0_MAIN_STREAM_MISC0, 1, 2);

	switch (format) {
	/* RGB */
	case 0x0:
		if (color_depth == 0x0)
			return RGB_6BIT;
		else if (color_depth == 0x1)
			return RGB_8BIT;
		else if (color_depth == 0x2)
			return RGB_10BIT;
		else if (color_depth == 0x3)
			return RGB_12BIT;
		else if (color_depth == 0x4)
			return RGB_16BIT;
		else
			return RET_FAIL;
		break;
	/* YUV422 */
	case 0x1:
		if (color_depth == 0x0)
			return YCBCR422_6BIT;
		else if (color_depth == 0x1)
			return YCBCR422_8BIT;
		else if (color_depth == 0x2)
			return YCBCR422_10BIT;
		else if (color_depth == 0x3)
			return YCBCR422_12BIT;
		else if (color_depth == 0x4)
			return YCBCR422_16BIT;
		else
			return RET_FAIL;
		break;
	/* YUV444 */
	case 0x2:
		if (color_depth == 0x0)
			return YCBCR444_6BIT;
		else if (color_depth == 0x1)
			return YCBCR444_8BIT;
		else if (color_depth == 0x2)
			return YCBCR444_10BIT;
		else if (color_depth == 0x3)
			return YCBCR444_12BIT;
		else if (color_depth == 0x4)
			return YCBCR444_16BIT;
		else
			return RET_FAIL;
		break;
	}

	return RET_FAIL;
}


u32 trilinear_get_pixclk(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 pixclk;
	u32 reg_val;

	reg_val = TR_GET_BITS(edp_hw, TR_SRC0_MVID, 0, 24);
	pixclk = reg_val * 1000;

	return pixclk;
}


u32 trilinear_get_pattern(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 reg_val;
	u32 pattern_lane;

	pattern_lane = TR_GET_BITS(edp_hw, TR_LINK_QUAL_PATTERN_SET, 24, 3);
	//whatever 1/2/4 lane pattern are set, lane0 is always need, so we
	//just read lane0's configuration
	if (pattern_lane != 0) {
		reg_val = TR_GET_BITS(edp_hw, TR_LINK_QUAL_PATTERN_SET, 0, 3);
		switch (reg_val) {
		case 0x1:
			return D10_2;
		case 0x2:
			return SYMBOL_MEASURE_PATTERN;
		case 0x3:
			return PRBS7;
		case 0x4:
			return PATTERN_80BIT;
		case 0x5:
			return HBR2_EYE;
		case 0x6:
			return CP2520_PATTERN2;
		case 0x7:
			return CP2520_PATTERN3;
		}
	}

	/* no test pattern set, now check if training pattern is set */
	reg_val = TR_GET_BITS(edp_hw, TR_TRAINING_PATTERN_SET, 0, 3);
	switch (reg_val) {
	case 0x1:
		return TPS1;
	case 0x2:
		return TPS2;
	case 0x3:
		return TPS3;
	case 0x4:
		return TPS4;
	default:
		return PATTERN_NONE;
	}

	return PATTERN_NONE;
}

s32 trilinear_get_lane_para(struct sunxi_edp_hw_desc *edp_hw, struct edp_lane_para *tmp_lane_para)
{
	u32 reg_val;

	/* bit rate */
	reg_val = TR_GET_BITS(edp_hw, TR_LINK_BW_SET, 0, 8);
	switch (reg_val) {
	case 0x6:
		tmp_lane_para->bit_rate = BIT_RATE_1G62;
		break;
	case 0x8:
		tmp_lane_para->bit_rate = BIT_RATE_2G16;
		break;
	case 0x9:
		tmp_lane_para->bit_rate = BIT_RATE_2G43;
		break;
	case 0xA:
		tmp_lane_para->bit_rate = BIT_RATE_2G7;
		break;
	case 0xC:
		tmp_lane_para->bit_rate = BIT_RATE_3G24;
		break;
	case 0x10:
		tmp_lane_para->bit_rate = BIT_RATE_4G32;
		break;
	case 0x14:
		tmp_lane_para->bit_rate = BIT_RATE_5G4;
		break;
	case 0x1E:
		tmp_lane_para->bit_rate = BIT_RATE_8G1;
		break;
	}

	/* lane count */
	reg_val = TR_GET_BITS(edp_hw, TR_LANE_COUNT_SET, 0, 8);
	if (reg_val == 1)
		tmp_lane_para->lane_cnt = 1;
	else if (reg_val == 2)
		tmp_lane_para->lane_cnt = 2;
	else if (reg_val == 4)
		tmp_lane_para->lane_cnt = 4;
	else
		tmp_lane_para->lane_cnt = 0;

	return RET_OK;
}

u32 trilinear_get_tu_size(struct sunxi_edp_hw_desc *edp_hw)
{
	return LS_PER_TU;
}

u32 trilinear_get_tu_valid_symbol(struct sunxi_edp_hw_desc *edp_hw)
{
	u32 symbol_frac = 0;
	u32 symbol = 0;
	u32 count;

	symbol_frac = TR_GET_BITS(edp_hw, TR_SRC0_TU_CONFIG, 24, 4);
	symbol = TR_GET_BITS(edp_hw, TR_SRC0_TU_CONFIG, 16, 8);
	count = (symbol * 10) + symbol_frac;

	return count;
}

s32 trilinear_link_soft_reset(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_SET_BITS(edp_hw, TR_SOFT_RESET, 4, 1, 0x1);
	msleep(5);
	return RET_OK;
}

s32 trilinear_video_soft_reset(struct sunxi_edp_hw_desc *edp_hw)
{
	TR_SET_BITS(edp_hw, TR_SOFT_RESET, 0, 1, 0x1);
	msleep(5);
	return RET_OK;
}

s32 trilinear_link_start(struct sunxi_edp_hw_desc *edp_hw)
{
	edp_video_stream_enable(edp_hw);
	return RET_OK;
}

s32 trilinear_link_stop(struct sunxi_edp_hw_desc *edp_hw)
{
	edp_video_stream_disable(edp_hw);

	return RET_OK;
}

s32 trilinear_query_transfer_unit(struct sunxi_edp_hw_desc *edp_hw,
				  struct edp_tx_core *edp_core,
				struct disp_video_timings *tmgs)
{
	u32 pack_data_rate;
	u32 valid_symbol;
	u32 pixel_clk;
	u32 bandwidth;
	u32 pre_div = 1000;
	u64 bit_rate = edp_core->lane_para.bit_rate;
	u32 lane_cnt =  edp_core->lane_para.lane_cnt;
	//fixme
	//fix me if color_fmt and colordepth can be configured
	u32 bpp = edp_core->lane_para.bpp;

	/*
	 * avg valid syobol per TU: pack_data_rate / bandwidth * LS_PER_TU
	 * pack_data_rate = (bpp / 8bit) * pix_clk / lane_cnt (1 symbol is 8 bit)
	 */

	pixel_clk = tmgs->pixel_clk;
	pixel_clk = pixel_clk / 1000;

	bandwidth = bit_rate / 10000000;

	if ((bit_rate == 0) || (lane_cnt == 0) || (pixel_clk == 0)) {
		EDP_ERR("edp param is zero, mode not support! pixclk:%d, bitrate:%lld lane_cnt:%d\n",
			pixel_clk * 1000, bit_rate, lane_cnt);
		return RET_FAIL;
	}

	pack_data_rate = (bpp * pixel_clk / 8) / lane_cnt;
	valid_symbol = LS_PER_TU * (pack_data_rate / bandwidth);

	if (valid_symbol > (64 * pre_div)) {
		EDP_ERR("out of valid symbol limit(lane:%d bit_rate:%lld pixel_clk:%d symbol_limit:62 symbol_now:%d\n",
				lane_cnt, bit_rate, pixel_clk, valid_symbol / 1000);
		EDP_ERR("check if lane_cnt or lane_rate can be enlarged!\n");

		return RET_FAIL;
	}

	return RET_OK;
}


void trilinear_set_lane_sw_pre(struct sunxi_edp_hw_desc *edp_hw, u32 lane_id, u32 sw, u32 pre, u32 param_type)
{
}

bool trilinear_support_tps3(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_fast_train(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool trilinear_support_audio(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_psr(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_psr2(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_ssc(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_assr(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_mst(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool trilinear_support_fec(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_lane_remap(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_lane_invert(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_hdcp1x(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_hardware_hdcp1x(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool trilinear_support_hdcp2x(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_hardware_hdcp2x(struct sunxi_edp_hw_desc *edp_hw)
{
	return false;
}

bool trilinear_support_enhance_frame(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_support_muti_pixel_mode(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}

bool trilinear_need_reset_before_enable(struct sunxi_edp_hw_desc *edp_hw)
{
	return true;
}


static struct sunxi_edp_hw_video_ops trilinear_dp14_video_ops = {
	.assr_enable = trilinear_assr_enable,
	.psr_enable = trilinear_psr_enable,
	.psr_is_enabled = trilinear_psr_is_enabled,
	.get_tu_size = trilinear_get_tu_size,
	.get_pixel_clk = trilinear_get_pixclk,
	.get_pixel_mode = trilinear_get_pixel_mode,
	.get_color_format = trilinear_get_color_fmt,
	.get_lane_para = trilinear_get_lane_para,
	.get_tu_valid_symbol = trilinear_get_tu_valid_symbol,
	.get_hotplug_change = trilinear_get_hotplug_change,
	.get_hotplug_state = trilinear_get_hotplug_state,
	.irq_handle = trilinear_irq_handler,
	.get_pattern = trilinear_get_pattern,
	.set_pattern = trilinear_set_pattern,
	.aux_read = trilinear_aux_read,
	.aux_write = trilinear_aux_write,
	.aux_i2c_read = trilinear_aux_i2c_read,
	.aux_i2c_write = trilinear_aux_i2c_write,
	.read_edid_block = trilinear_read_edid_block,
	.irq_enable = trilinear_irq_enable,
	.irq_disable = trilinear_irq_disable,
	.link_soft_reset = trilinear_link_soft_reset,
	.video_soft_reset = trilinear_video_soft_reset,
	.main_link_start = trilinear_link_start,
	.main_link_stop = trilinear_link_stop,
	.lane_remap = trilinear_lane_remap_config,
	.lane_invert = trilinear_lane_invert_config,
	.scrambling_enable = trilinear_scrambling_enable,
	.fec_enable = trilinear_fec_enable,
	.set_lane_sw_pre = trilinear_set_lane_sw_pre,
	.set_lane_cnt = trilinear_set_lane_cnt,
	.set_lane_rate = trilinear_set_lane_rate,
	.set_pixel_mode = trilinear_set_pixel_mode,
	.init_early = trilinear_init_early,
	.init = trilinear_controller_init,
	.enable = trilinear_enable,
	.disable = trilinear_disable,
	.set_video_timings = trilinear_set_video_timings,
	.set_video_format = trilinear_set_video_format,
	.config_tu = trilinear_set_transfer_config,
	.config_mst = trilinear_mst_init,
	.query_tu_capability = trilinear_query_transfer_unit,
	.support_max_rate = trilinear_get_max_rate,
	.support_max_lane = trilinear_get_max_lane,
	.support_tps3 = trilinear_support_tps3,
	.support_fast_training = trilinear_support_fast_train,
	.support_psr = trilinear_support_psr,
	.support_psr2 = trilinear_support_psr2,
	.support_ssc = trilinear_support_ssc,
	.support_mst = trilinear_support_mst,
	.support_fec = trilinear_support_fec,
	.support_assr = trilinear_support_assr,
	.support_lane_remap = trilinear_support_lane_remap,
	.support_lane_invert = trilinear_support_lane_invert,
	.support_enhance_frame = trilinear_support_enhance_frame,
	.support_muti_pixel_mode = trilinear_support_muti_pixel_mode,
	.need_reset_before_enable = trilinear_need_reset_before_enable,
};

struct sunxi_edp_hw_video_ops *sunxi_edp_get_hw_video_ops(void)
{
	return &trilinear_dp14_video_ops;
}

static struct sunxi_edp_hw_audio_ops trilinear_dp14_audio_ops = {
	.support_audio = trilinear_support_audio,
	.audio_enable = trilinear_audio_enable,
	.audio_disable = trilinear_audio_disable,
	.audio_config = trilinear_audio_config,
	.audio_mute = trilinear_audio_mute,
	.audio_is_enabled = trilinear_audio_is_enabled,
	.get_audio_if = trilinear_get_audio_if,
	.audio_is_muted = trilinear_audio_is_muted,
	.get_audio_data_width = trilinear_get_audio_date_width,
	.get_audio_chn_cnt = trilinear_get_audio_chn_cnt,
	.get_audio_max_channel = trilinear_get_audio_max_channel,
};

struct sunxi_edp_hw_audio_ops *sunxi_edp_get_hw_audio_ops(void)
{
	return &trilinear_dp14_audio_ops;
}

void trilinear_hdcp_set_mode(struct sunxi_edp_hw_desc *edp_hw,
			     enum dp_hdcp_mode mode)
{
	if (mode == HDCP14_MODE)
		TR_SET_BITS(edp_hw, TR_HDCP_MODE, 0, 2, 0x1);
	else if (mode == HDCP23_MODE)
		TR_SET_BITS(edp_hw, TR_HDCP_MODE, 0, 2, 0x2);
	else
		TR_SET_BITS(edp_hw, TR_HDCP_MODE, 0, 2, 0x0);
}

void trilinear_hdcp_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	if (enable)
		TR_SET_BITS(edp_hw, TR_HDCP_ENABLE, 0, 1, 0x1);
	else
		TR_SET_BITS(edp_hw, TR_HDCP_ENABLE, 0, 1, 0x0);
}

u64 trilinear_hdcp1_get_aksv(struct sunxi_edp_hw_desc *edp_hw)
{
	u64 aksv = 0;

	aksv |= ((u64)TR_READ(edp_hw, TR_HDCP_AKSV_63_32)) << 32;
	aksv |= (u64)TR_READ(edp_hw, TR_HDCP_AKSV_31_0);

	return aksv;
}

u64 trilinear_hdcp1_get_an(struct sunxi_edp_hw_desc *edp_hw)
{
	u64 an = 0;
	u32 an_l32 = 0;
	u32 an_h32 = 0;

	TR_SET_BITS(edp_hw, TR_HDCP_RNG_CIPGER_AN, 0, 1, 0x1);
	an_l32 = TR_READ(edp_hw, TR_HDCP_RNG_AN_31_0);
	an_h32 = TR_READ(edp_hw, TR_HDCP_RNG_AN_63_32);

	an |= ((u64)(an_h32)) << 32;
	an |= (u64)(an_l32);

	//FIXME: not sure if need when we use hardware km calculator
	// TR_WRITE(edp_hw, TR_HDCP_AN_31_0, an_l32);
	// TR_WRITE(edp_hw, TR_HDCP_AN_63_32, an_h32);

	return an;
}

void trilinear_hdcp_encrypt_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	if (enable) {
		TR_SET_BITS(edp_hw, TR_HDCP_STREAM_CIPGER_EN, 0, 1, 0x1);
		//TR_SET_BITS(edp_hw, TR_FORCE_SCRAMBLER_RESET, 0, 1, 0x1);
	} else
		TR_SET_BITS(edp_hw, TR_HDCP_STREAM_CIPGER_EN, 0, 1, 0x0);
}

void trilinear_hdcp1_write_bksv(struct sunxi_edp_hw_desc *edp_hw, u64 bksv)
{
	/*
	 * do nothing, because in trilinear, km would be caculated
	 * when bkasv is writen to register to trigger it.
	 * */
}

u64 trilinear_hdcp1_cal_km(struct sunxi_edp_hw_desc *edp_hw, u64 bksv)
{
	TR_SET_BITS(edp_hw, TR_HDCP_CIPHER_CONTROL, 0, 1, 0x1);
	TR_WRITE(edp_hw, TR_HDCP_BKSV_31_0, (u32)(bksv & 0xffffffff));
	TR_WRITE(edp_hw, TR_HDCP_BKSV_31_0, (u32)(bksv >> 32));

	// FIXME:it seems km and r0 are caculate internal,
	// we can't fetch km's value

	return 1442;
}

u32 trilinear_hdcp1_cal_r0(struct sunxi_edp_hw_desc *edp_hw, u64 an, u64 km)
{
	u32 timeout = 50000;
	u32 reg_val = 0;
	u32 r0_ready;
	u32 r0 = 0;

	while (timeout > 0) {
		reg_val = TR_READ(edp_hw, TR_HDCP_R0_STATUS);
		r0_ready = reg_val & BIT(16);

		if (r0_ready) {
			r0 = reg_val & 0xffff;
			break;
		} else {
			timeout--;
			continue;
		}
	}

	if (!r0)
		EDP_ERR("[HDCP1x] r0 calculate timeout!\n");

	return r0;
}

u64 trilinear_hdcp1_get_m0(struct sunxi_edp_hw_desc *edp_hw)
{
	u64 m0 = 0;

	m0 |= (u64)(TR_READ(edp_hw, TR_HDCP_M0_31_0));
	m0 |= (u64)(TR_READ(edp_hw, TR_HDCP_M0_63_32)) << 32;

	return m0;
}

struct sunxi_edp_hw_hdcp_ops trilinear_dp14_dpcd_ops = {
	.support_hdcp1x = trilinear_support_hdcp1x,
	.support_hdcp2x = trilinear_support_hdcp2x,
	.support_hw_hdcp1x = trilinear_support_hardware_hdcp1x,
	.support_hw_hdcp2x = trilinear_support_hardware_hdcp2x,
	.hdcp1_get_an = trilinear_hdcp1_get_an,
	.hdcp1_get_aksv = trilinear_hdcp1_get_aksv,
	.hdcp1_write_bksv = trilinear_hdcp1_write_bksv,
	.hdcp1_cal_km = trilinear_hdcp1_cal_km,
	.hdcp1_cal_r0 = trilinear_hdcp1_cal_r0,
	.hdcp1_get_m0 = trilinear_hdcp1_get_m0,
	.hdcp1_encrypt_enable = trilinear_hdcp_encrypt_enable,
	.hdcp_enable = trilinear_hdcp_enable,
	.hdcp_set_mode = trilinear_hdcp_set_mode,
};

struct sunxi_edp_hw_hdcp_ops *sunxi_dp_get_hw_hdcp_ops(void)
{
	return &trilinear_dp14_dpcd_ops;
}

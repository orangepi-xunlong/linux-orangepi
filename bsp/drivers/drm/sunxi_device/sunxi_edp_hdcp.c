/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * core function of edp driver
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "sunxi_edp.h"
#include "sha1.h"
#include "sha256.h"
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/slab.h>

#define HDCP1X_REAUTH_CNT 3


static inline struct sunxi_dp_hdcp *
sunxi_hdcp1_info_to_sunxi_hdcp(struct sunxi_dptx_hdcp1_info *info)
{
	return container_of(info, struct sunxi_dp_hdcp, hdcp1_info);
}

static u64 dptx_hdcp1_hw_get_an(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return 0;

	if (ops->hdcp1_get_an)
		return ops->hdcp1_get_an(edp_hw);
	else
		return 0;
}

static u64 dptx_hdcp1_hw_get_aksv(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return 0;

	if (ops->hdcp1_get_aksv)
		return ops->hdcp1_get_aksv(edp_hw);
	else
		return 0;
}

static void dptx_hdcp1_hw_write_bksv(struct sunxi_edp_hw_desc *edp_hw, u64 bksv)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return;

	if (ops->hdcp1_write_bksv)
		return ops->hdcp1_write_bksv(edp_hw, bksv);
}

static u64 dptx_hdcp1_hw_calculate_km(struct sunxi_edp_hw_desc *edp_hw, u64 bksv)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return 0;

	if (ops->hdcp1_cal_km)
		return ops->hdcp1_cal_km(edp_hw, bksv);
	else
		return 0;
}

static u32 dptx_hdcp1_hw_calculate_r0(struct sunxi_edp_hw_desc *edp_hw, u64 an, u64 km)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return 0;

	if (ops->hdcp1_cal_r0)
		return ops->hdcp1_cal_r0(edp_hw, an, km);
	else
		return 0;
}

static u64 dptx_hdcp1_hw_get_m0(struct sunxi_edp_hw_desc *edp_hw)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return 0;

	if (ops->hdcp1_get_m0)
		return ops->hdcp1_get_m0(edp_hw);
	else
		return 0;
}

static void dptx_hdcp1_hw_encrypt_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return;

	if (ops->hdcp1_encrypt_enable)
		ops->hdcp1_encrypt_enable(edp_hw, enable);
}

static void dptx_hdcp_hw_enable(struct sunxi_edp_hw_desc *edp_hw, bool enable)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return;

	if (ops->hdcp_enable)
		ops->hdcp_enable(edp_hw, enable);
}

void dptx_hdcp_hw_set_mode(struct sunxi_edp_hw_desc *edp_hw, enum dp_hdcp_mode mode)
{
	struct sunxi_edp_hw_hdcp_ops *ops = edp_hw->hdcp_ops;

	if (ops == NULL)
		return;

	if (ops->hdcp_set_mode)
		ops->hdcp_set_mode(edp_hw, mode);
}



static bool hdcp1_status_success(struct sunxi_dptx_hdcp1_info *info)
{
	return (info->status == DP_HDCP_STATUS_SUCCESS) ? true : false;
}

static bool hdcp1_status_fail(struct sunxi_dptx_hdcp1_info *info)
{
	return (info->status == DP_HDCP_STATUS_FAIL) ? true : false;
}

/* reserve delay for some unpreditable case in future */
static void sunxi_dp_hdcp1_set_state(struct sunxi_dptx_hdcp1_info *info,
				     enum sunxi_dp_hdcp1_state state, u32 delay_ms)
{
	if (delay_ms)
		msleep(delay_ms);
	info->state = state;
}

static void sunxi_dp_hdcp1_set_status(struct sunxi_dptx_hdcp1_info *info,
				       enum sunxi_dp_hdcp_status status, u32 delay_ms)
{
	if (delay_ms)
		msleep(delay_ms);
	info->status = status;
}

static s32 sunxi_dp_hdcp1_read_parse_bcaps(struct sunxi_dptx_hdcp1_info *info)
{
	char bcap_buf[16];
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	memset(bcap_buf, 0, sizeof(bcap_buf));

	if (edp_hw_aux_read(hdcp->edp_hw, HDCP1X_DPCD_BCAPS,
			      HDCP1X_DPCD_BCAPS_BYTES, &bcap_buf[0]) < 0)
		return RET_FAIL;

	info->bcaps = bcap_buf[0];
	EDP_HDCP_DBG("[HDCP1x] bcaps:0x%x\n", bcap_buf[0]);

	if (info->bcaps & HDCP1X_CAPABLE)
		info->hdcp1_capable = true;

	if (info->bcaps & HDCP1X_REPEATER)
		info->rx_is_repeater = true;

	return RET_OK;
}

static s32 sunxi_dp_hdcp1_write_an_aksv(struct sunxi_dptx_hdcp1_info *info)
{
	char an_buf[16];
	char aksv_buf[16];
	s32 ret = RET_FAIL;
	u64 aksv = 0, an = 0;
	u32 i = 0;
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	memset(an_buf, 0, sizeof(an_buf));
	memset(aksv_buf, 0, sizeof(aksv_buf));

	an = dptx_hdcp1_hw_get_an(hdcp->edp_hw);
	aksv = dptx_hdcp1_hw_get_aksv(hdcp->edp_hw);

	/* LSB first, DPCD value size is 8bit per reg */
	for (i = 0; i < HDCP1X_DPCD_AN_BYTES; i++)
		an_buf[i] = (an >> (i * 8)) & 0xff;
	ret = edp_hw_aux_write(hdcp->edp_hw, HDCP1X_DPCD_AN,
				 HDCP1X_DPCD_AN_BYTES, &an_buf[0]);
	if (ret != RET_OK)
		return ret;

	for (i = 0; i < HDCP1X_DPCD_AKSV_BYTES; i++)
		aksv_buf[i] = (aksv >> (i * 8)) & 0xff;
	ret = edp_hw_aux_write(hdcp->edp_hw, HDCP1X_DPCD_AKSV,
				 HDCP1X_DPCD_AKSV_BYTES, &aksv_buf[0]);
	if (ret != RET_OK)
		return ret;

	/* just for debug*/
	info->an = an;
	info->aksv = aksv;

	return RET_OK;
}

static s32 sunxi_dp_hdcp1_read_bksv(struct sunxi_dptx_hdcp1_info *info)
{
	char bksv_buf[16];
	u32 i = 0;
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	memset(bksv_buf, 0, sizeof(bksv_buf));

	if (edp_hw_aux_read(hdcp->edp_hw, HDCP1X_DPCD_BKSV,
			      HDCP1X_DPCD_BKSV_BYTES, &bksv_buf[0]) < 0)
		return RET_FAIL;

	// FIXME: not sure LSB/MSB for bksv
	// can print it after read, ensure high 24 bit is 0
	for (i = 0; i < HDCP1X_DPCD_BKSV_BYTES; i++) {
		EDP_HDCP_DBG("[HDCP1x] bksv: data[%d]:0x%x\n", i, bksv_buf[i]);
		info->bksv |= ((u64)(bksv_buf[i]) << (i * 8));
	}

	return RET_OK;
}

static bool sunxi_dp_hdcp1_validate_srm(u64 bksv)
{
	//TODO: maybe some controller IP has its own srm version
	// return edp_hal_hdcp1_validate_srm();
	return true;
}

static bool sunxi_dp_hdcp1_validate_ksv_checksum(u64 bksv)
{
	int i, ones = 0;
	u64 val = 1;

	// test for bits above the KSV
	if (bksv & 0xffffff0000000000U)
		return false;

	// loop through the bits of the ksv
	// test each bit, calculate the number of one's.
	for (i = 0; i < 40; i++) {
		if (bksv & (val << i))
			ones += 1;
	}

	// return status of the ksv
	return (ones == 20) ? true : false;
}

static s32 sunxi_dp_hdcp1_validate_bksv(u64 bksv)
{
	bool pass;

	pass = sunxi_dp_hdcp1_validate_ksv_checksum(bksv);
	if (!pass)
		return RET_FAIL;

	pass = sunxi_dp_hdcp1_validate_srm(bksv);
	if (!pass)
		return RET_FAIL;

	return RET_OK;
}

static s32 sunxi_dp_hdcp1_set_ainfo(struct sunxi_dptx_hdcp1_info *info)
{
	char ainfo_buf[16];
	s32 ret = RET_FAIL;
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	memset(ainfo_buf, 0, sizeof(ainfo_buf));

	ainfo_buf[0] |= HDCP1X_REAUTH_ENABLE_IRQ_HPD;
	ret = edp_hw_aux_write(hdcp->edp_hw, HDCP1X_DPCD_KSV_AINFO,
				 HDCP1X_DPCD_KSV_AINFO_BYTES, &ainfo_buf[0]);
	if (ret != RET_OK)
		return ret;

	return RET_OK;
}

static void sunxi_dp_hdcp1_cal_km(struct sunxi_dptx_hdcp1_info *info)
{
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	dptx_hdcp1_hw_write_bksv(hdcp->edp_hw, info->bksv);
	info->km = dptx_hdcp1_hw_calculate_km(hdcp->edp_hw, info->bksv);
}

static void sunxi_dp_hdcp1_cal_r0(struct sunxi_dptx_hdcp1_info *info)
{
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	info->r0 = dptx_hdcp1_hw_calculate_r0(hdcp->edp_hw, info->an, info->km);
}

static s32 hdcp1_read_parse_r0_prime(struct sunxi_dptx_hdcp1_info *info)
{
	char bstatus_buf[16];
	char r0_buf[16];
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	memset(bstatus_buf, 0, sizeof(bstatus_buf));
	memset(r0_buf, 0, sizeof(r0_buf));

	if (edp_hw_aux_read(hdcp->edp_hw, HDCP1X_DPCD_BSTATUS,
			      HDCP1X_DPCD_BSTATUS_BYTES, &bstatus_buf[0]) < 0)
		return RET_FAIL;

	if (bstatus_buf[0] & HDCP1X_R0_PRIME_AVAILABLE) {
		info->r0_prime_ready = true;

		if (edp_hw_aux_read(hdcp->edp_hw, HDCP1X_DPCD_R0_PRIME,
				      HDCP1X_DPCD_R0_PRIME_BYTES, &r0_buf[0]) < 0)
			return RET_FAIL;

		// FIXME: not sure LSB/MSB for r0 prime
		info->r0_prime |= (u32)(r0_buf[0]);
		info->r0_prime |= (u32)(r0_buf[1]) << 8;
	}

	EDP_HDCP_DBG("[HDCP1X] r0'= 0x%x\n", info->r0_prime);

	return RET_OK;
}

static s32 sunxi_dp_hdcp_read_parse_binfo_ksvs_ready(struct sunxi_dptx_hdcp1_info *info)
{
	char bstatus_buf[16];
	char binfo_buf[16];
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	memset(bstatus_buf, 0, sizeof(bstatus_buf));
	memset(binfo_buf, 0, sizeof(binfo_buf));

	if (edp_hw_aux_read(hdcp->edp_hw, HDCP1X_DPCD_BSTATUS,
			      HDCP1X_DPCD_BSTATUS_BYTES, &bstatus_buf[0]) < 0)
		return RET_FAIL;

	if (bstatus_buf[0] & HDCP1X_KSV_LIST_READY) {
		info->ksv_list_ready = true;
		if (edp_hw_aux_read(hdcp->edp_hw, HDCP1X_DPCD_BINFO,
				      HDCP1X_DPCD_BINFO_BYTES, &binfo_buf[0]) < 0)
			return RET_FAIL;

		info->repeater_dev_cnt = binfo_buf[0] & HDCP1X_REPEATER_DEVICE_CNT_MASK;
		info->device_exceeded = (binfo_buf[0] & HDCP1X_REPEATER_DEVICE_EXCEED) ? true : false;
		info->repeater_dev_dep = binfo_buf[1] & HDCP1X_REPEATER_DEVICE_DEP_MASK;
		info->cascade_exceeded = binfo_buf[1] & HDCP1X_REPEATER_CASCADE_EXCEED;
		info->binfo[0] = binfo_buf[0];
		info->binfo[1] = binfo_buf[1] << 8;
	}

	return RET_OK;
}

static bool hdcp1_validate_r0_r0_prime(struct sunxi_dptx_hdcp1_info *info)
{
	return (info->r0 == info->r0_prime) ? true : false;
}

static s32 sunxi_dp_hdcp1_read_ksv_list(struct sunxi_dptx_hdcp1_info *info)
{
	char ksv_list_buf[16];
	u32 read_cnt = 0;
	u32 i = 0, j = 0;
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	// note:
	// HDCP1_KSV_LIST_BYTES_PER_DEV = 5
	// HDCP1_DEV_CNT_PER_KSV_LIST_FIFO = 3
	// HDCP1X_DPCD_KSV_FIFO_BYTES = 15

	/* each dpcd read ksv list fifo contains 3 device's info */
	read_cnt = info->repeater_dev_cnt / HDCP1_DEV_CNT_PER_KSV_LIST_FIFO;
	read_cnt += (info->repeater_dev_cnt % HDCP1_DEV_CNT_PER_KSV_LIST_FIFO) ? 1 : 0;

	for (i = 0; i < read_cnt; i++) {
		memset(ksv_list_buf, 0, sizeof(ksv_list_buf));
		if (edp_hw_aux_read(hdcp->edp_hw, HDCP1X_DPCD_KSV_FIFO,
			HDCP1X_DPCD_KSV_FIFO_BYTES, &ksv_list_buf[0]) < 0) {
			return RET_FAIL;
		}

		for (j = 0; j < HDCP1_DEV_CNT_PER_KSV_LIST_FIFO; j++) {
			memcpy(info->ksv_list + (info->ksv_cnt * HDCP1_KSV_LIST_BYTES_PER_DEV),
			       ksv_list_buf + (j * HDCP1_KSV_LIST_BYTES_PER_DEV),
			       HDCP1_KSV_LIST_BYTES_PER_DEV);
			info->ksv_cnt++;

			/* break if device cnt reach */
			if (info->ksv_cnt == info->repeater_dev_cnt)
				break;
		}
	}

	return RET_OK;
}

static void sunxi_dp_hdcp1_get_m0(struct sunxi_dptx_hdcp1_info *info)
{
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	u64 m0 = dptx_hdcp1_hw_get_m0(hdcp->edp_hw);
	u32 i = 0;

	for (i = 0; i < 8; i++)
		info->m0[i] = (u8)((m0 >> (8 * i)) & 0xff);
}

static s32 sunxi_dp_hdcp1_read_v_prime(struct sunxi_dptx_hdcp1_info *info)
{
	char vprime_buf[HDCP1X_DPCD_V_PRIME_BYTES];
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	memset(vprime_buf, 0, sizeof(vprime_buf));

	if (edp_hw_aux_read(hdcp->edp_hw, HDCP1X_DPCD_V_PRIME,
			      HDCP1X_DPCD_V_PRIME_BYTES, &vprime_buf[0]) < 0)
		return RET_FAIL;

	memcpy(info->v_prime, vprime_buf, HDCP1X_DPCD_V_PRIME_BYTES);

	return RET_OK;
}

static void sunxi_dp_hdcp1_calculate_v(struct sunxi_dptx_hdcp1_info *info)
{
	SHA1Context sha1_ctx;
	u8 sha1_output[HDCP1X_DPCD_V_PRIME_BYTES];

	if ((info->repeater_dev_cnt == 0) || (info->ksv_cnt == 0))
		return;

	// note:
	// HDCP1_KSV_LIST_BYTES_PER_DEV = 5

	SHA1Reset(&sha1_ctx);
	SHA1Input(&sha1_ctx, info->ksv_list, info->ksv_cnt * HDCP1_KSV_LIST_BYTES_PER_DEV);
#if 1
	SHA1Input(&sha1_ctx, info->binfo, HDCP1X_DPCD_BINFO_BYTES);
	SHA1Input(&sha1_ctx, info->m0, HDCP1X_M0_BYTES);
#else
	// FIXME: consider move m0 append to binfo
	//SHA1Input(&sha1_ctx, info->binfo, (HDCP1X_M0_BYTES + HDCP1X_DPCD_BINFO_BYTES));
#endif
	SHA1Result(&sha1_ctx, sha1_output);

	memcpy(info->v, sha1_output, HDCP1X_DPCD_V_PRIME_BYTES);
}


static bool sunxi_dp_hdcp1_v_validate(struct sunxi_dptx_hdcp1_info *info)
{
	u8 zero_char[HDCP1X_DPCD_V_PRIME_BYTES] = {0};

	if ((info->repeater_dev_cnt == 0) || (info->ksv_cnt == 0))
		return false;

	if (memcmp(info->v, zero_char, HDCP1X_DPCD_V_PRIME_BYTES) == 0)
		return false;

	if (memcmp(info->v_prime, zero_char, HDCP1X_DPCD_V_PRIME_BYTES) == 0)
		return false;

	if (memcmp(info->v, info->v_prime, HDCP1X_DPCD_V_PRIME_BYTES) == 0)
		return true;
	else
		return false;
}

s32 sunxi_dp_hdcp_irq_handler(struct sunxi_dp_hdcp *hdcp)
{
	struct sunxi_dptx_hdcp1_info *info = &hdcp->hdcp1_info;

	if (hdcp->hdcp1_capable) {
		hdcp1_read_parse_r0_prime(info);
		sunxi_dp_hdcp_read_parse_binfo_ksvs_ready(info);

		if (info->r0_prime_ready == true || info->ksv_list_ready == true)
			wake_up(&hdcp->auth_queue);

//TODO
//		if (ksv_list_available)
//			wake_up(&hdcp->auth_queue);

//TODO
//		if (link_integrity)
//			stop_and_re_auth();;
	}

	if (hdcp->hdcp2_capable) {
	}

	return RET_OK;
}

static void sunxi_dp_hdcp1_encrypt_enable(struct sunxi_dptx_hdcp1_info *info, bool enable)
{
	struct sunxi_dp_hdcp *hdcp = sunxi_hdcp1_info_to_sunxi_hdcp(info);

	dptx_hdcp1_hw_encrypt_enable(hdcp->edp_hw, enable);
}

s32 sunxi_dp_hdcp1_auth(struct sunxi_dp_hdcp *hdcp)
{
	struct sunxi_dptx_hdcp1_info *info = &hdcp->hdcp1_info;
	s32 ret = RET_OK;
	long timeout = 0;

	while (!hdcp1_status_success(info) && !hdcp1_status_fail(info)) {
		switch (info->state) {
		case HDCP1_DP_STATE_NONE:
			EDP_ERR("hdcp1 for dp not enable yet!\n");
			return RET_OK;
		case HDCP1_DP_STATE_START:
			sunxi_dp_hdcp1_set_state(info, HDCP1_A0_DETERMINE_RX_HDCP_CAPABLE, 0);
			break;
		case HDCP1_A0_DETERMINE_RX_HDCP_CAPABLE:
			ret = sunxi_dp_hdcp1_read_parse_bcaps(info);
			if (ret != RET_OK)
				sunxi_dp_hdcp1_set_state(info, HDCP1_READ_BCAPS_FAIL, 0);

			if (info->hdcp1_capable)
				sunxi_dp_hdcp1_set_state(info, HDCP1_A1_EXCHANGE_KSVS_WRITE_AN_AKSV, 0);
			else
				sunxi_dp_hdcp1_set_state(info, HDCP1_A0_HDCP_NOT_SUPPORT, 0);
			break;
		case HDCP1_A1_EXCHANGE_KSVS_WRITE_AN_AKSV:
			ret = sunxi_dp_hdcp1_write_an_aksv(info);
			if (ret != RET_OK)
				sunxi_dp_hdcp1_set_state(info, HDCP1_A1_WRITE_AN_AKSV_FAIL, 0);
			else
				sunxi_dp_hdcp1_set_state(info, HDCP1_A1_EXCHANGE_KSVS_READ_BKSV, 0);
			break;
		case HDCP1_A1_EXCHANGE_KSVS_READ_BKSV:
			ret = sunxi_dp_hdcp1_read_bksv(info);
			if (ret != RET_OK)
				sunxi_dp_hdcp1_set_state(info, HDCP1_A1_READ_BKSV_FAIL, 0);
			else
				sunxi_dp_hdcp1_set_state(info, HDCP1_A1_EXCHANGE_KSVS_VALIDATE_BKSV, 0);
			break;
		case HDCP1_A1_EXCHANGE_KSVS_VALIDATE_BKSV:
			ret = sunxi_dp_hdcp1_validate_bksv(info->bksv);
			if (ret != RET_OK)
				sunxi_dp_hdcp1_set_state(info, HDCP1_A1_VALIDATE_BKSV_FAIL, 0);
			else {
				if (info->rx_is_repeater) {
					if (sunxi_dp_hdcp1_set_ainfo(info) != RET_OK)
						sunxi_dp_hdcp1_set_state(info, HDCP1_A1_WRITE_AINFO_FAIL, 0);
				}
				sunxi_dp_hdcp1_set_state(info, HDCP1_A1_EXCHANGE_KSVS_CAL_KM, 0);
			}
			break;
		case HDCP1_A1_EXCHANGE_KSVS_CAL_KM:
			sunxi_dp_hdcp1_cal_km(info);
			sunxi_dp_hdcp1_set_state(info, HDCP1_A2_COMPUTATIONS, 0);
			break;
		case HDCP1_A2_COMPUTATIONS:
			sunxi_dp_hdcp1_cal_r0(info);
			sunxi_dp_hdcp1_set_state(info, HDCP1_A2_A3_WAIT_FOR_R0_PRIME, 0);
			break;
		case HDCP1_A2_A3_WAIT_FOR_R0_PRIME:
			/* refer from HDCP1X spec, we should not read r0' within 100ms*/
			timeout = wait_event_timeout(hdcp->auth_queue, info->r0_prime_ready == true,
						     msecs_to_jiffies(100));
			sunxi_dp_hdcp1_set_state(info, HDCP1_A2_A3_READ_R0_PRIME, 0);
			break;
		case HDCP1_A2_A3_READ_R0_PRIME:
			/*
			 * usually r0' has been read after CP_IRQ income
			 * but some system not support CP_IRQ, so read r0'
			 * manaualy after 100ms timeout
			 */
			if (!info->r0_prime_ready || !info->r0_prime) {
				if (hdcp1_read_parse_r0_prime(info) != RET_OK)
					sunxi_dp_hdcp1_set_state(info, HDCP1_A2_A3_READ_R0_PRIME_FAIL, 0);
			}

			if (!info->r0_prime_ready || !info->r0_prime)
				sunxi_dp_hdcp1_set_state(info, HDCP1_A2_A3_READ_R0_PRIME_FAIL, 0);
			else
				sunxi_dp_hdcp1_set_state(info, HDCP1_A3_VALIDATE_RX, 0);
			break;
		case HDCP1_A3_VALIDATE_RX:
			if (!hdcp1_validate_r0_r0_prime(info)) {
				if (info->r0_prime_retry >= 3)
					sunxi_dp_hdcp1_set_state(info, HDCP1_A3_VALIDATE_RX_FAIL, 0);
				else {
					/* clear r0' and try to re-read it 2 more times*/
					info->r0_prime = 0;
					sunxi_dp_hdcp1_set_state(info, HDCP1_A2_A3_READ_R0_PRIME, 0);
					info->r0_prime_retry++;
				}
			} else {
				sunxi_dp_hdcp1_set_state(info, HDCP1_A5_TEST_FOR_REPEATER, 0);
			}
			break;
		case HDCP1_A5_TEST_FOR_REPEATER:
			if (info->rx_is_repeater)
				sunxi_dp_hdcp1_set_state(info, HDCP1_A6_WAIT_FOR_READY, 0);
			else
				sunxi_dp_hdcp1_set_state(info, HDCP1_A4_AUTHENTICATED, 0);
			break;
		case HDCP1_A6_WAIT_FOR_READY:
			timeout = wait_event_timeout(hdcp->auth_queue, info->ksv_list_ready == true,
						     msecs_to_jiffies(5000));
			/*
			 * usually bstatus READY BIT has been read after CP_IRQ income but some
			 * system not support CP_IRQ, so read bstatus manaualy after 5s timeout
			 */
			if (!info->ksv_list_ready) {
				if (sunxi_dp_hdcp_read_parse_binfo_ksvs_ready(info) != RET_OK)
					sunxi_dp_hdcp1_set_state(info, HDCP1_A6_READ_BSTATUS_BINFO_FAIL, 0);
			}

			if (!info->ksv_list_ready || info->device_exceeded || info->cascade_exceeded)
				sunxi_dp_hdcp1_set_state(info, HDCP1_A6_WAIT_FOR_READY_FAIL, 0);
			else if (info->ksv_list_ready && (info->device_exceeded || info->cascade_exceeded))
				sunxi_dp_hdcp1_set_state(info, HDCP1_A6_REPEATER_DEV_DEP_EXCEED_FAIL, 0);
			else
				sunxi_dp_hdcp1_set_state(info, HDCP1_A7_READ_KSV_LIST, 0);
			break;
		case HDCP1_A7_READ_KSV_LIST:
			ret = sunxi_dp_hdcp1_read_ksv_list(info);
			if (ret != RET_OK)
				sunxi_dp_hdcp1_set_state(info, HDCP1_A7_READ_KSV_LIST_FAIL, 0);
			else
				sunxi_dp_hdcp1_set_state(info, HDCP1_A7_CALCULATE_V, 0);
			break;
		case HDCP1_A7_CALCULATE_V:
			sunxi_dp_hdcp1_get_m0(info);
			ret = sunxi_dp_hdcp1_read_v_prime(info);
			if (ret != RET_OK)
				sunxi_dp_hdcp1_set_state(info, HDCP1_A7_READ_V_PRIME_FAIL, 0);
			else {
				sunxi_dp_hdcp1_calculate_v(info);
				sunxi_dp_hdcp1_set_state(info, HDCP1_A7_VERIFY_V_V_PRIME, 0);
			}
			break;
		case HDCP1_A7_VERIFY_V_V_PRIME:
			if (!sunxi_dp_hdcp1_v_validate(info)) {
				if (info->v_read_retry >= 3)
					sunxi_dp_hdcp1_set_state(info, HDCP1_A7_VERIFY_V_V_PRIME_FAIL, 0);
				else {
					sunxi_dp_hdcp1_set_state(info, HDCP1_A7_READ_KSV_LIST, 0);
					info->v_read_retry++;
				}
			} else
				sunxi_dp_hdcp1_set_state(info, HDCP1_A4_AUTHENTICATED, 0);
			break;
		case HDCP1_A4_AUTHENTICATED:
			sunxi_dp_hdcp1_encrypt_enable(info, true);
			sunxi_dp_hdcp1_set_state(info, HDCP1_DP_STATE_END, 0);
			break;
		case HDCP1_DP_STATE_END:
			sunxi_dp_hdcp1_set_status(info, DP_HDCP_STATUS_SUCCESS, 0);
			//sunxi_dp_hdcp1_clear_r0_prime_ready(info);
			//sunxi_dp_hdcp1_clear_ksv_list_ready(info);
			ret = RET_OK;
			break;
		case HDCP1_READ_BCAPS_FAIL:
		case HDCP1_A0_HDCP_NOT_SUPPORT:
		case HDCP1_A1_WRITE_AN_AKSV_FAIL:
		case HDCP1_A1_READ_BKSV_FAIL:
		case HDCP1_A1_VALIDATE_BKSV_FAIL:
		case HDCP1_A1_WRITE_AINFO_FAIL:
		case HDCP1_A2_A3_READ_R0_PRIME_FAIL:
		case HDCP1_A3_VALIDATE_RX_FAIL:
		case HDCP1_A6_READ_BSTATUS_BINFO_FAIL:
		case HDCP1_A6_WAIT_FOR_READY_FAIL:
		case HDCP1_A6_REPEATER_DEV_DEP_EXCEED_FAIL:
		case HDCP1_A7_READ_KSV_LIST_FAIL:
		case HDCP1_A7_READ_V_PRIME_FAIL:
		case HDCP1_A7_VERIFY_V_V_PRIME_FAIL:
		default:
			//EDP_ERR("dp hdcp1 authentication fail, reason:%s\n", hdcp1_state_to_string(info->state));
			//EDP_HDCP_DBG("%s\n", sunxi_dp_hdcp1_dump());
			sunxi_dp_hdcp1_set_status(info, DP_HDCP_STATUS_FAIL, 0);
			//sunxi_dp_hdcp1_clear_r0_prime_ready(info);
			//sunxi_dp_hdcp1_clear_ksv_list_ready(info);
			ret = RET_FAIL;
			break;
		}
	}

	return ret;
}

void sunxi_dp_hdcp1_clear_info(struct sunxi_dptx_hdcp1_info *info)
{
	memset(info, 0, sizeof(struct sunxi_dptx_hdcp1_info));
}


s32 sunxi_dp_hdcp1_disable(struct sunxi_dp_hdcp *hdcp)
{
	struct sunxi_dptx_hdcp1_info *info = &hdcp->hdcp1_info;

	sunxi_dp_hdcp1_encrypt_enable(info, false);
	sunxi_dp_hdcp1_clear_info(info);
	dptx_hdcp_hw_enable(hdcp->edp_hw, false);
	dptx_hdcp_hw_set_mode(hdcp->edp_hw, HDCP_NONE_MODE);

	return RET_OK;
}

s32 sunxi_dp_hdcp1_enable(struct sunxi_dp_hdcp *hdcp)
{
	struct sunxi_dptx_hdcp1_info *info = &hdcp->hdcp1_info;
	u32 i = 0;
	s32 ret = 0;

	// FIXME: not sure if need: clear all exit info,
	// begin authencation at the original procedure

	dptx_hdcp_hw_set_mode(hdcp->edp_hw, HDCP14_MODE);
	dptx_hdcp_hw_enable(hdcp->edp_hw, true);
	sunxi_dp_hdcp1_set_state(info, HDCP1_DP_STATE_START, 0);

	for (i = 0; i < HDCP1X_REAUTH_CNT; i++) {
		sunxi_dp_hdcp1_clear_info(info);
		ret = sunxi_dp_hdcp1_auth(hdcp);
		if (ret == RET_OK) {
			//sunxi_dp_hdcp1_clear_info(info);
			return ret;
		}
	}

	EDP_ERR("hdcp1 retry 3 times but still fail, authentication fail!\n");
	sunxi_dp_hdcp1_disable(hdcp);

	return RET_FAIL;
}

bool dprx_hdcp1_capable(struct sunxi_dp_hdcp *hdcp)
{
	char bcap_buf[16];

	memset(bcap_buf, 0, sizeof(bcap_buf));

	if (edp_hw_aux_read(hdcp->edp_hw, HDCP1X_DPCD_BCAPS, HDCP1X_DPCD_BCAPS_BYTES, &bcap_buf[0]) < 0)
		return false;

	if (bcap_buf[0] & HDCP1X_CAPABLE) {
		hdcp->hdcp1_capable = true;
		return true;
	}

	hdcp->hdcp1_capable = false;
	return false;
}

s32 sunxi_dp_hdcp_init(struct sunxi_dp_hdcp *hdcp,
				struct sunxi_edp_hw_desc *edp_hw)
{
	/* deliver edp_hw to hdcp, use for some lowlevel operation */
	hdcp->edp_hw = edp_hw;
	mutex_init(&hdcp->auth_lock);
	init_waitqueue_head(&hdcp->auth_queue);

	return RET_OK;
}

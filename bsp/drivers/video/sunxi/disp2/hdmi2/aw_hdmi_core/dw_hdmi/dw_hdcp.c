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
#include <sunxi-log.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#include "dw_mc.h"
#include "dw_fc.h"
#include "dw_i2cm.h"
#include "dw_access.h"
#include "dw_hdcp22_tx.h"
#include "dw_hdcp.h"

static dw_hdmi_dev_t   *hdmi_dev;
static dw_hdcp_param_t    *phdcp;
static dw_video_param_t   *pvideo;

bool hdcp14_enable;
bool hdcp22_enable;

static u32 hdcp14_auth_enable;
static u32 hdcp14_auth_complete;
static u32 hdcp14_encryption;

static u8 hdcp_status;
static u32 hdcp_engaged_count;
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
bool esm_enable_fail;
#endif

#define DW_HDCP_KSV_HEADER		10
#define DW_HDCP_KSV_SHAMAX		20

#define DW_HDCP_OESS_SIZE             (0x40)

/* KSV value size */
#define HDCP14_KSV_LEN             (5)

#define HDCP14_ADDR_JUMP           (4)

/* HDCP Interrupt fields */
#define INT_KSV_ACCESS            (A_APIINTSTAT_KSVACCESSINT_MASK)
#define INT_KSV_SHA1              (A_APIINTSTAT_KSVSHA1CALCINT_MASK)
#define INT_KSV_SHA1_DONE         (A_APIINTSTAT_KSVSHA1CALCDONEINT_MASK)
#define INT_HDCP_FAIL             (A_APIINTSTAT_HDCP_FAILED_MASK)
#define INT_HDCP_ENGAGED          (A_APIINTSTAT_HDCP_ENGAGED_MASK)

static int _dw_hdcp_ddc_read_version(dw_hdmi_dev_t *dev, u8 *data)
{
	if (dw_i2cm_ddc_read(dev, 0x3A, 0, 0, 0x50, 1, data)) {
		hdmi_err("%s: DDC Read hdcp2Version failed!\n", __func__);
		return -1;
	}
	return 0;
}

/**
 * @desc: config hdcp14 work in hdmi mode or dvi mode
 * @bit: 1 - hdmi mode
 *       0 - dvi mode
*/
static void _dw_hdcp14_tmds_mode(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_HDMIDVI_MASK, bit);
}

static void _dw_hdcp14_feature11_enable(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_EN11FEATURE_MASK, bit);
}

static void _dw_hdcp14_rxdetect_enable(dw_hdmi_dev_t *dev, u8 enable)
{
	LOG_TRACE1(enable);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_RXDETECT_MASK, enable);
}

/**
 * @desc: config hdcp14 ri check mode
 * @dev: dw hdmi devices
 * @bit: 1 - 2s mode
 *       0 - 128 frame mode
*/
static void _dw_hdcp14_set_ri_mode(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_SYNCRICHECK_MASK, bit);
}

static void _dw_hdcp14_set_bypass_encryption(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_BYPENCRYPTION_MASK, bit);
}

static void _dw_hdcp14_set_i2c_fast_mode(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_I2CFASTMODE_MASK, bit);
}

static void _dw_hdcp14_set_enhance_link_verification(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_ELVENA_MASK, bit);
}

/**
 * @desc: Software reset signal, active by writing a zero
 *	      and auto cleared to 1 in the following cycle
 */
static void _dw_hdcp14_sw_reset(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	dev_write_mask(dev, A_HDCPCFG1, A_HDCPCFG1_SWRESET_MASK, 0);
}

static void _dw_hdcp14_disable_encrypt(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_HDCPCFG1, A_HDCPCFG1_ENCRYPTIONDISABLE_MASK, bit);
}

static u8 _dw_hdcp14_get_encryption(dw_hdmi_dev_t *dev)
{
	return dev_read_mask(dev, A_HDCPCFG1, A_HDCPCFG1_ENCRYPTIONDISABLE_MASK);
}

static void _dw_hdcp14_interrupt_clear(dw_hdmi_dev_t *dev, u8 value)
{
	dev_write(dev, (A_APIINTCLR), value);
}

static u8 _dw_hdcp14_get_interrupt_state(dw_hdmi_dev_t *dev)
{
	return dev_read(dev, A_APIINTSTAT);
}

static void _dw_hdcp14_set_interrupt_mask(dw_hdmi_dev_t *dev, u8 value)
{
	dev_write(dev, (A_APIINTMSK), value);
}

static u8 _dw_hdcp14_get_interrupt_mask(dw_hdmi_dev_t *dev)
{
	return dev_read(dev, A_APIINTMSK);
}

static void _dw_hdcp14_set_oess_size(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write(dev, (A_OESSWCFG), value);
}

static void _dw_hdcp14_set_access_ksv_memory(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE();
	dev_write_mask(dev, A_KSVMEMCTRL, A_KSVMEMCTRL_KSVMEMREQUEST_MASK, bit);
}

static u8 _dw_hdcp14_get_access_ksv_memory_granted(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	return (u8)((dev_read(dev, A_KSVMEMCTRL)
		& A_KSVMEMCTRL_KSVMEMACCESS_MASK) >> 1);
}

static void _dw_hdcp14_update_ksv_list_state(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_KSVMEMCTRL, A_KSVMEMCTRL_SHA1FAIL_MASK, bit);
	dev_write_mask(dev, A_KSVMEMCTRL, A_KSVMEMCTRL_KSVCTRLUPD_MASK, 1);
	dev_write_mask(dev, A_KSVMEMCTRL, A_KSVMEMCTRL_KSVCTRLUPD_MASK, 0);
}

static u16 _dw_hdcp14_get_bstatus(dw_hdmi_dev_t *dev)
{
	u16 bstatus = 0;

	bstatus	= dev_read(dev, HDCP_BSTATUS);
	bstatus	|= dev_read(dev, HDCP_BSTATUS + HDCP14_ADDR_JUMP) << 8;
	return bstatus;
}

static void _dw_hdcp_set_hsync_polarity(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_VIDPOLCFG, A_VIDPOLCFG_HSYNCPOL_MASK, bit);
}

static void _dw_hdcp_set_vsync_polarity(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_VIDPOLCFG, A_VIDPOLCFG_VSYNCPOL_MASK, bit);
}

static void _dw_hdcp_enable_data_polarity(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, A_VIDPOLCFG, A_VIDPOLCFG_DATAENPOL_MASK, bit);
}

static u8 _dw_hdcp14_get_bskv(dw_hdmi_dev_t *dev, u8 index)
{
	return (u8)dev_read_mask(dev, (HDCPREG_BKSV0 + (index * HDCP14_ADDR_JUMP)),
		HDCPREG_BKSV0_HDCPREG_BKSV0_MASK);
}

static void _dw_hdcp14_sha_reset(dw_hdmi_dev_t *dev, sha_t *sha)
{
	size_t i = 0;

	sha->mIndex = 0;
	sha->mComputed = false;
	sha->mCorrupted = false;
	for (i = 0; i < sizeof(sha->mLength); i++)
		sha->mLength[i] = 0;

	sha->mDigest[0] = 0x67452301;
	sha->mDigest[1] = 0xEFCDAB89;
	sha->mDigest[2] = 0x98BADCFE;
	sha->mDigest[3] = 0x10325476;
	sha->mDigest[4] = 0xC3D2E1F0;
}

static void _dw_hdcp14_sha_process_block(dw_hdmi_dev_t *dev, sha_t *sha)
{
#define shaCircularShift(bits, word) \
		((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))

	const unsigned K[] = {	/* constants defined in SHA-1 */
		0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
	};
	unsigned W[80];		/* word sequence */
	unsigned A, B, C, D, E;	/* word buffers */
	unsigned temp = 0;
	int t = 0;

	/* Initialize the first 16 words in the array W */
	for (t = 0; t < 80; t++) {
		if (t < 16) {
			W[t] = ((unsigned)sha->mBlock[t * 4 + 0]) << 24;
			W[t] |= ((unsigned)sha->mBlock[t * 4 + 1]) << 16;
			W[t] |= ((unsigned)sha->mBlock[t * 4 + 2]) << 8;
			W[t] |= ((unsigned)sha->mBlock[t * 4 + 3]) << 0;
		} else {
			W[t] =
			    shaCircularShift(1,
					     W[t - 3] ^ W[t - 8] ^ W[t -
							14] ^ W[t - 16]);
		}
	}

	A = sha->mDigest[0];
	B = sha->mDigest[1];
	C = sha->mDigest[2];
	D = sha->mDigest[3];
	E = sha->mDigest[4];

	for (t = 0; t < 80; t++) {
		temp = shaCircularShift(5, A);
		if (t < 20)
			temp += ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		else if (t < 40)
			temp += (B ^ C ^ D) + E + W[t] + K[1];
		else if (t < 60)
			temp += ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		else
			temp += (B ^ C ^ D) + E + W[t] + K[3];

		E = D;
		D = C;
		C = shaCircularShift(30, B);
		B = A;
		A = (temp & 0xFFFFFFFF);
	}

	sha->mDigest[0] = (sha->mDigest[0] + A) & 0xFFFFFFFF;
	sha->mDigest[1] = (sha->mDigest[1] + B) & 0xFFFFFFFF;
	sha->mDigest[2] = (sha->mDigest[2] + C) & 0xFFFFFFFF;
	sha->mDigest[3] = (sha->mDigest[3] + D) & 0xFFFFFFFF;
	sha->mDigest[4] = (sha->mDigest[4] + E) & 0xFFFFFFFF;

	sha->mIndex = 0;
}

static void _dw_hdcp14_sha_input(dw_hdmi_dev_t *dev, sha_t *sha,
		const u8 *data, size_t size)
{
	int i = 0;
	unsigned j = 0;
	int rc = true;

	if (data == NULL || size == 0) {
		hdmi_err("%s: invalid input data\n", __func__);
		return;
	}
	if (sha->mComputed == true || sha->mCorrupted == true) {
		sha->mCorrupted = true;
		return;
	}
	while (size-- && sha->mCorrupted == false) {
		sha->mBlock[sha->mIndex++] = *data;

		for (i = 0; i < 8; i++) {
			rc = true;
			for (j = 0; j < sizeof(sha->mLength); j++) {
				sha->mLength[j]++;
				if (sha->mLength[j] != 0) {
					rc = false;
					break;
				}
			}
			sha->mCorrupted = (sha->mCorrupted == true
					   || rc == true) ? true : false;
		}
		/* if corrupted then message is too long */
		if (sha->mIndex == 64)
			_dw_hdcp14_sha_process_block(dev, sha);

		data++;
	}
}

static void _dw_hdcp14_sha_pad_message(dw_hdmi_dev_t *dev, sha_t *sha)
{
	/*
	 *  Check to see if the current message block is too small to hold
	 *  the initial padding bits and length.  If so, we will pad the
	 *  block, process it, and then continue padding into a second
	 *  block.
	 */
	if (sha->mIndex > 55) {
		sha->mBlock[sha->mIndex++] = 0x80;
		while (sha->mIndex < 64)
			sha->mBlock[sha->mIndex++] = 0;

		_dw_hdcp14_sha_process_block(dev, sha);
		while (sha->mIndex < 56)
			sha->mBlock[sha->mIndex++] = 0;

	} else {
		sha->mBlock[sha->mIndex++] = 0x80;
		while (sha->mIndex < 56)
			sha->mBlock[sha->mIndex++] = 0;
	}

	/* Store the message length as the last 8 octets */
	sha->mBlock[56] = sha->mLength[7];
	sha->mBlock[57] = sha->mLength[6];
	sha->mBlock[58] = sha->mLength[5];
	sha->mBlock[59] = sha->mLength[4];
	sha->mBlock[60] = sha->mLength[3];
	sha->mBlock[61] = sha->mLength[2];
	sha->mBlock[62] = sha->mLength[1];
	sha->mBlock[63] = sha->mLength[0];

	_dw_hdcp14_sha_process_block(dev, sha);
}

static int _dw_hdcp14_sha_result(dw_hdmi_dev_t *dev, sha_t *sha)
{
	if (sha->mCorrupted == true)
		return false;

	if (sha->mComputed == false) {
		_dw_hdcp14_sha_pad_message(dev, sha);
		sha->mComputed = true;
	}
	return true;
}

static int _dw_hdcp14_verify_ksv(dw_hdmi_dev_t *dev, const u8 *data, size_t size)
{
	size_t i = 0;
	sha_t sha;

	LOG_TRACE1((int)size);

	if (data == NULL || size < (DW_HDCP_KSV_HEADER + DW_HDCP_KSV_SHAMAX)) {
		hdmi_err("%s: invalid input data\n", __func__);
		return false;
	}
	_dw_hdcp14_sha_reset(dev, &sha);
	_dw_hdcp14_sha_input(dev, &sha, data, size - DW_HDCP_KSV_SHAMAX);

	if (_dw_hdcp14_sha_result(dev, &sha) == false) {
		hdmi_err("%s: cannot process SHA digest\n", __func__);
		return false;
	}

	for (i = 0; i < DW_HDCP_KSV_SHAMAX; i++) {
		if (data[size - DW_HDCP_KSV_SHAMAX + i] !=
				(u8) (sha.mDigest[i / 4] >> ((i % 4) * 8))) {
			hdmi_err("%s: SHA digest does not match\n", __func__);
			return false;
		}
	}
	return true;
}

/* SHA-1 calculation by Software */
static u8 _dw_hdcp14_read_ksv_list(dw_hdmi_dev_t *dev, int *param)
{
	int timeout = 1000;
	u16 bstatus = 0;
	u16 deviceCount = 0;
	int valid = HDCP_IDLE;
	int size = 0;
	int i = 0;

	u8 *hdcp_ksv_list_buffer = dev->hdcp.mKsvListBuffer;

	/* 1 - Wait for an interrupt to be triggered
		(a_apiintstat.KSVSha1calcint) */
	/* This is called from the INT_KSV_SHA1 irq
		so nothing is required for this step */

	/* 2 - Request access to KSV memory through
		setting a_ksvmemctrl.KSVMEMrequest to 1'b1 and */
	/* pool a_ksvmemctrl.KSVMEMaccess until
		this value is 1'b1 (access granted). */
	_dw_hdcp14_set_access_ksv_memory(dev, true);
	while (_dw_hdcp14_get_access_ksv_memory_granted(dev) == 0 && timeout--)
		asm volatile ("nop");

	if (_dw_hdcp14_get_access_ksv_memory_granted(dev) == 0) {
		_dw_hdcp14_set_access_ksv_memory(dev, false);
		hdmi_err("%s: KSV List memory access denied\n", __func__);
		*param = 0;
		return HDCP_KSV_LIST_ERR_MEM_ACCESS;
	}

	/* 3 - Read VH', M0, Bstatus, and the KSV FIFO.
	The data is stored in the revocation memory, as */
	/* provided in the "Address Mapping for Maximum Memory Allocation"
	table in the databook. */
	bstatus = _dw_hdcp14_get_bstatus(dev);
	deviceCount = bstatus & BSTATUS_DEVICE_COUNT_MASK;

	if (deviceCount > dev->hdcp.maxDevices) {
		*param = 0;
		hdmi_err("%s: depth exceeds KSV List memory\n", __func__);
		return HDCP_KSV_LIST_ERR_DEPTH_EXCEEDED;
	}

	size = deviceCount * HDCP14_KSV_LEN + DW_HDCP_KSV_HEADER + DW_HDCP_KSV_SHAMAX;

	for (i = 0; i < size; i++) {
		if (i < DW_HDCP_KSV_HEADER) { /* BSTATUS & M0 */
			hdcp_ksv_list_buffer[(deviceCount * HDCP14_KSV_LEN) + i] =
			(u8)dev_read(dev, HDCP_BSTATUS + (i * HDCP14_ADDR_JUMP));
		} else if (i < (DW_HDCP_KSV_HEADER + (deviceCount * HDCP14_KSV_LEN))) { /* KSV list */
			hdcp_ksv_list_buffer[i - DW_HDCP_KSV_HEADER] =
			(u8)dev_read(dev, HDCP_BSTATUS + (i * HDCP14_ADDR_JUMP));
		} else { /* SHA */
			hdcp_ksv_list_buffer[i] = (u8)dev_read(dev,
				HDCP_BSTATUS + (i * HDCP14_ADDR_JUMP));
		}
	}

	/* 4 - Calculate the SHA-1 checksum (VH) over M0,
		Bstatus, and the KSV FIFO. */
	if (_dw_hdcp14_verify_ksv(dev, hdcp_ksv_list_buffer, size) == true) {
		valid = HDCP_KSV_LIST_READY;
		HDCP_INF("HDCP_KSV_LIST_READY\n");
	} else {
		valid = HDCP_ERR_KSV_LIST_NOT_VALID;
		hdmi_err("%s: HDCP_ERR_KSV_LIST_NOT_VALID\n", __func__);
	}

	/* 5 - If the calculated VH equals the VH',
	set a_ksvmemctrl.SHA1fail to 0 and set */
	/* a_ksvmemctrl.KSVCTRLupd to 1.
	If the calculated VH is different from VH' then set */
	/* a_ksvmemctrl.SHA1fail to 1 and set a_ksvmemctrl.KSVCTRLupd to 1,
	forcing the controller */
	/* to re-authenticate from the beginning. */
	_dw_hdcp14_set_access_ksv_memory(dev, 0);
	_dw_hdcp14_update_ksv_list_state(dev, (valid == HDCP_KSV_LIST_READY) ? 0 : 1);

	return valid;
}

/* do nor encry until stabilizing successful authentication */
static void _dw_hdcp14_check_engaged(dw_hdmi_dev_t *dev)
{
	if ((hdcp_status == HDCP_ENGAGED) && (hdcp_engaged_count >= 20)) {
		_dw_hdcp14_disable_encrypt(dev, false);
		hdcp_engaged_count = 0;
		hdcp14_encryption  = 1;
		hdmi_inf("hdcp start encryption\n");
	}
}

u8 _dw_hdcp14_status_handler(dw_hdmi_dev_t *dev, int *param, u32 irq_stat)
{
	u8 interrupt_status = 0;
	int valid = HDCP_IDLE;

	interrupt_status = irq_stat;

	if (interrupt_status != 0)
		HDCP_INF("hdcp get interrupt state: 0x%x\n", interrupt_status);

	if (interrupt_status == 0) {
		if (hdcp_engaged_count && (hdcp_status == HDCP_ENGAGED)) {
			hdcp_engaged_count++;
			_dw_hdcp14_check_engaged(dev);
		}

		return hdcp_status;
	}

	if ((interrupt_status & A_APIINTSTAT_KEEPOUTERRORINT_MASK) != 0) {
		hdmi_err("%s: hdcp status: keep out\n", __func__);
		hdcp_status = HDCP_FAILED;

		hdcp_engaged_count = 0;
		return HDCP_FAILED;
	}

	if ((interrupt_status & A_APIINTSTAT_LOSTARBITRATION_MASK) != 0) {
		hdmi_err("%s: hdcp status: lost arbitration\n", __func__);
		hdcp_status = HDCP_FAILED;

		hdcp_engaged_count = 0;
		return HDCP_FAILED;
	}

	if ((interrupt_status & A_APIINTSTAT_I2CNACK_MASK) != 0) {
		hdmi_err("%s: hdcp status: i2c nack\n", __func__);
		hdcp_status = HDCP_FAILED;

		hdcp_engaged_count = 0;
		return HDCP_FAILED;
	}

	if (interrupt_status & INT_KSV_SHA1) {
		hdmi_inf("hdcp status: ksv sha1\n");
		return _dw_hdcp14_read_ksv_list(dev, param);
	}

	if ((interrupt_status & INT_HDCP_FAIL) != 0) {
		*param = 0;
		hdmi_err("%s: hdcp status: hdcp failed\n", __func__);
		_dw_hdcp14_disable_encrypt(dev, true);
		hdcp_status = HDCP_FAILED;

		hdcp_engaged_count = 0;
		return HDCP_FAILED;
	}

	if ((interrupt_status & INT_HDCP_ENGAGED) != 0) {
		*param = 1;
		hdmi_inf("hdcp status: engaged\n");

		hdcp_status = HDCP_ENGAGED;
		hdcp_engaged_count = 1;

		return HDCP_ENGAGED;
	}

	return valid;
}

static int _dw_hdcp14_status_check_and_handle(dw_hdmi_dev_t *dev)
{
	u8 hdcp14_status = 0;
	int param;
	u8 ret = 0;

	if (!hdcp14_auth_enable) {
		hdmi_wrn("hdcp14 auth not enable!\n");
		return -1;
	}

	if (!hdcp14_auth_complete) {
		hdmi_wrn("hdcp14 auth not complete!\n");
		return -1;
	}

	hdcp14_status = _dw_hdcp14_get_interrupt_state(dev);
	_dw_hdcp14_interrupt_clear(dev, hdcp14_status);

	ret = _dw_hdcp14_status_handler(dev, &param, (u32)hdcp14_status);
	if ((ret != HDCP_ERR_KSV_LIST_NOT_VALID) && (ret != HDCP_FAILED))
		return 0;
	else
		return -1;
}

void dw_hdcp_set_avmute_state(dw_hdmi_dev_t *dev, int enable)
{
	LOG_TRACE1(enable);
	dev_write_mask(dev, A_HDCPCFG0, A_HDCPCFG0_AVMUTE_MASK, enable);
}

u8 dw_hdcp_get_avmute_state(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, A_HDCPCFG0, A_HDCPCFG0_AVMUTE_MASK);
}

/* chose hdcp22_ovr_val to designed which hdcp version to be configured */
static void _dw_hdcp_select_hdcp14_path(dw_hdmi_dev_t *dev)
{
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HDCP22_OVR_VAL_MASK, 0);
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HDCP22_OVR_EN_MASK, 1);
}

static void _dw_hdcp14_start_auth(struct work_struct *work)
{
	dw_tmds_mode_t mode;
	u8 hsPol;
	u8 vsPol;
	u8 hdcp_mask = 0;

	if (pvideo == NULL) {
		hdmi_err("%s: pointer pvideo is null!!!\n", __func__);
		return;
	}

	if (hdcp14_auth_complete) {
		hdmi_wrn("hdcp14 has been auth!\n");
		return;
	}

	mode  = pvideo->mHdmi;
	hsPol = pvideo->mDtd.mHSyncPolarity;
	vsPol = pvideo->mDtd.mVSyncPolarity;

	mutex_lock(&hdmi_dev->i2c_lock);
	msleep(20);

	/* disable encrypt */
	_dw_hdcp14_disable_encrypt(hdmi_dev, true);

	/* main control disable hdcp clock */
	dw_mc_hdcp_clock_disable(hdmi_dev, true);

	/* enable hdcp keepout */
	dw_fc_video_set_hdcp_keepout(hdmi_dev, true);

	/* config hdcp work mode */
	_dw_hdcp14_tmds_mode(hdmi_dev, (mode == DW_TMDS_MODE_HDMI) ? true : false);

	/* config Hsync and Vsync polarity and enable */
	_dw_hdcp_set_hsync_polarity(hdmi_dev, (hsPol > 0) ? true : false);
	_dw_hdcp_set_vsync_polarity(hdmi_dev, (vsPol > 0) ? true : false);
	_dw_hdcp_enable_data_polarity(hdmi_dev, true);

	/* bypass hdcp_block = 0 */
	dev_write_mask(hdmi_dev, 0x1000C, 1 << 5, 0x0);

	/* config use hdcp14 path */
	_dw_hdcp_select_hdcp14_path(hdmi_dev);

	/* config hdcp14 feature11 */
	_dw_hdcp14_feature11_enable(hdmi_dev, (phdcp->mEnable11Feature > 0) ? 1 : 0);

	/* config hdcp14 ri check mode */
	_dw_hdcp14_set_ri_mode(hdmi_dev, (phdcp->mRiCheck > 0) ? 1 : 0);

	/* config hdcp14 i2c mode */
	_dw_hdcp14_set_i2c_fast_mode(hdmi_dev, (phdcp->mI2cFastMode > 0) ? 1 : 0);

	_dw_hdcp14_set_enhance_link_verification(hdmi_dev,
				(phdcp->mEnhancedLinkVerification > 0) ? 1 : 0);

	/* fixed */
	dw_hdcp_set_avmute_state(hdmi_dev, false);

	/* config send color when sending unencrtypted video data */
	dev_write_mask(hdmi_dev, A_VIDPOLCFG, A_VIDPOLCFG_UNENCRYPTCONF_MASK, 0x2);

	/* set enable encoding packet header */
	dev_write_mask(hdmi_dev, A_HDCPCFG1, A_HDCPCFG1_PH2UPSHFTENC_MASK, 0x1);

	/* config encryption oess size */
	_dw_hdcp14_set_oess_size(hdmi_dev, DW_HDCP_OESS_SIZE);

	/* config use hdcp14 encryption path */
	_dw_hdcp14_set_bypass_encryption(hdmi_dev, false);

	/* _dw_hdcp14_disable_encrypt(hdmi_dev, false); */

	/* software reset hdcp14 engine */
	_dw_hdcp14_sw_reset(hdmi_dev);

	/* enable hdcp14 rx detect for start auth */
	_dw_hdcp14_rxdetect_enable(hdmi_dev, true);

	hdcp_mask  = A_APIINTCLR_KSVACCESSINT_MASK;
	hdcp_mask |= A_APIINTCLR_KSVSHA1CALCINT_MASK;
	hdcp_mask |= A_APIINTCLR_KEEPOUTERRORINT_MASK;
	hdcp_mask |= A_APIINTCLR_LOSTARBITRATION_MASK;
	hdcp_mask |= A_APIINTCLR_I2CNACK_MASK;
	hdcp_mask |= A_APIINTCLR_HDCP_FAILED_MASK;
	hdcp_mask |= A_APIINTCLR_HDCP_ENGAGED_MASK;
	_dw_hdcp14_interrupt_clear(hdmi_dev, hdcp_mask);

	hdcp_mask  = A_APIINTMSK_KSVACCESSINT_MASK;
	hdcp_mask |= A_APIINTMSK_KSVSHA1CALCINT_MASK;
	hdcp_mask |= A_APIINTMSK_KEEPOUTERRORINT_MASK;
	hdcp_mask |= A_APIINTMSK_LOSTARBITRATION_MASK;
	hdcp_mask |= A_APIINTMSK_I2CNACK_MASK;
	hdcp_mask |= A_APIINTMSK_SPARE_MASK;
	hdcp_mask |= A_APIINTMSK_HDCP_FAILED_MASK;
	hdcp_mask |= A_APIINTMSK_HDCP_ENGAGED_MASK;
	_dw_hdcp14_set_interrupt_mask(hdmi_dev,
		((~hdcp_mask) & _dw_hdcp14_get_interrupt_mask(hdmi_dev)));

	/* main control enable hdcp clock */
	dw_mc_hdcp_clock_disable(hdmi_dev, false);

	hdcp14_auth_enable = 1;
	hdcp14_auth_complete = 1;

	mutex_unlock(&hdmi_dev->i2c_lock);
}

static void _dw_hdcp14_config(dw_hdmi_dev_t *dev, dw_hdcp_param_t *hdcp)
{
	LOG_TRACE();
	hdcp_status = HDCP_FAILED;

	_dw_hdcp14_start_auth(NULL);

	hdcp14_enable = true;
	hdcp22_enable = false;
}

static void _dw_hdcp14_disconfig(dw_hdmi_dev_t *dev)
{
	/* 1. disable encryption */
	_dw_hdcp14_disable_encrypt(dev, true);
	dw_mc_hdcp_clock_disable(dev, true);

	hdcp_status   = HDCP_FAILED;
	hdcp14_enable = false;
	hdcp14_auth_enable   = false;
	hdcp14_auth_complete = false;
	hdcp14_encryption    = false;
	HDCP_INF("hdcp14 disconfig done!\n");
}

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
/* chose which way to enable hdcp22
* @enable: 0-chose to enable hdcp22 by dwc_hdmi inner signal ist_hdcp_capable
*              1-chose to enable hdcp22 by hdcp22_ovr_val bit
*/
static void _dw_hdcp_select_hdcp22_path(dw_hdmi_dev_t *dev, u8 enable)
{
	dev_write_mask(dev, HDCP22REG_CTRL,
		HDCP22REG_CTRL_HDCP22_OVR_VAL_MASK, enable);
	dev_write_mask(dev, HDCP22REG_CTRL,
		HDCP22REG_CTRL_HDCP22_OVR_EN_MASK, enable);
}

void _dw_hdcp22_ovr_enable_avmute(dw_hdmi_dev_t *dev, u8 val, u8 enable)
{
	dev_write_mask(dev, HDCP22REG_CTRL1,
		HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_VAL_MASK, val);
	dev_write_mask(dev, HDCP22REG_CTRL1,
		HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_EN_MASK, enable);
}

/* chose what place hdcp22 hpd come from and config hdcp22 hpd enable or disable
* @val: 0-hdcp22 hpd come from phy: phy_stat0.HPD
*         1-hdcp22 hpd come from hpd_ovr_val
* @enable:hpd_ovr_val
*/
static void _dw_hdcp22_ovr_enable_hpd(dw_hdmi_dev_t *dev, u8 val, u8 enable)
{
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HPD_OVR_VAL_MASK, val);
	dev_write_mask(dev, (HDCP22REG_CTRL),
		HDCP22REG_CTRL_HPD_OVR_EN_MASK, enable);
}
#endif

/* To judge if sink support hdcp2.2
* return: 1: sink support hdcp2.2
*         0: sink do not support hdcp2.2
*        -1: ddc reading fail
*/
int dw_hdcp22_get_sink_support(dw_hdmi_dev_t *dev)
{
	u8 data = 0;
	if (_dw_hdcp_ddc_read_version(dev, &data) == 0) {
		if ((data & 0x4) == 0x4)
			return 1;
		else
			return 0;
	} else {
		hdmi_err("%s: hdcp ddc read sink version failed\n", __func__);
		return -1;
	}
	return -1;
}

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
/* configure hdcp2.2 and enable hdcp2.2 encrypt */
static void _dw_hdcp22_config(dw_hdmi_dev_t *dev, dw_hdcp_param_t *hdcp, dw_video_param_t *video)
{
	dw_tmds_mode_t mode = video->mHdmi;
	u8 hsPol = pvideo->mDtd.mHSyncPolarity;
	u8 vsPol = pvideo->mDtd.mVSyncPolarity;

	LOG_TRACE();
	/* 1 - set main controller hdcp clock disable */
	dw_hdcp22_data_enable(0);

	/* 2 - set hdcp keepout */
	dw_fc_video_set_hdcp_keepout(hdmi_dev, true);

	/* 3 - Select DVI or HDMI mode */
	_dw_hdcp14_tmds_mode(hdmi_dev, (mode == DW_TMDS_MODE_HDMI) ? 1 : 0);

	/* 4 - Set the Data enable, Hsync, and VSync polarity */
	_dw_hdcp_set_hsync_polarity(hdmi_dev, (hsPol > 0) ? 1 : 0);
	_dw_hdcp_set_vsync_polarity(hdmi_dev, (vsPol > 0) ? 1 : 0);
	_dw_hdcp_enable_data_polarity(hdmi_dev, 1);
	/* hdcp22_switch_lck(dev, 0); */
	dev_write_mask(dev, 0x1000C, 1 << 5, 0x1);
	_dw_hdcp_select_hdcp22_path(dev, 1);
	_dw_hdcp22_ovr_enable_hpd(dev, 1, 1);
	_dw_hdcp22_ovr_enable_avmute(dev, 0, 0);
	dev_write_mask(dev, 0x1000C, 1 << 4, 0x1);
	/* mask the interrupt of hdcp22 event */
	dev_write_mask(dev, HDCP22REG_MASK, 0xff, 0);
	/* hdcp22_switch_lck(dev, 1); */

	if (dw_esm_open() < 0) {
		esm_enable_fail = true;
		return;
	}
	dw_hdcp22_esm_start_auth();
	hdcp14_enable = false;
	hdcp22_enable = true;
}

static void _dw_hdcp22_disconfig(dw_hdmi_dev_t *dev)
{
	dw_mc_hdcp_clock_disable(dev, 1);/* 0:enable   1:disable */

	/* hdcp22_switch_lck(dev, 0); */
	dev_write_mask(dev, 0x1000C, 1 << 5, 0x0);
	_dw_hdcp_select_hdcp22_path(dev, 0);
	_dw_hdcp22_ovr_enable_hpd(dev, 0, 1);
	dev_write_mask(dev, 0x1000C, 1 << 4, 0x0);
	/* hdcp22_switch_lck(dev, 1); */
	dw_esm_disable();

	hdcp22_enable = false;
	esm_enable_fail = false;
	HDCP_INF("hdcp22 disconfig done!\n");
}

void dw_hdcp22_data_enable(u8 enable)
{
	dw_mc_hdcp_clock_disable(hdmi_dev, enable ? 0 : 1);
	_dw_hdcp22_ovr_enable_avmute(hdmi_dev, enable ? 0 : 1, 1);
}
#endif

void dw_hdcp_main_config(dw_hdmi_dev_t *dev, dw_hdcp_param_t *hdcp, dw_video_param_t *video)
{
	int hdcp_type = 0;
	hdmi_dev = dev;
	phdcp = hdcp;
	pvideo = video;

	LOG_TRACE();

	if ((hdmi_dev == NULL) || (phdcp == NULL) || (pvideo == NULL)) {
		hdmi_err("%s: param is null!!!\n", __func__);
		return;
	}

	if ((dev->snps_hdmi_ctrl.hdcp_on == 0) || (hdcp->use_hdcp == 0)) {
		hdmi_err("%s: hdcp is not active when hdcp_on(%d), use_hdcp(%d)\n",
			__func__, dev->snps_hdmi_ctrl.hdcp_on, hdcp->use_hdcp);
		return;
	}

	if (dev->snps_hdmi_ctrl.hdcp_enable) {
		hdmi_wrn("hdcp has been enable.\n");
		return;
	}

	/* Before configure HDCP we should configure the internal parameters */
	hdcp->maxDevices = 128;
	hdcp->mI2cFastMode = 0;
	memcpy(&dev->hdcp, hdcp, sizeof(dw_hdcp_param_t));

	if (hdcp->hdcp_debug == DW_HDCP_ENABLE_FORCE_14) {
		hdmi_inf("dw force enable hdcp1.4\n");

		_dw_hdcp14_config(dev, hdcp);
		dev->snps_hdmi_ctrl.hdcp_type = DW_HDCP_TYPE_HDCP14;
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	} else if (hdcp->hdcp_debug == DW_HDCP_ENABLE_FORCE_22) {
		hdmi_inf("dw force enable hdcp2.2\n");

		hdcp_type = dw_hdcp22_get_sink_support(dev);
		if (hdcp_type == 1) {
			_dw_hdcp22_config(dev, hdcp, video);
			dev->snps_hdmi_ctrl.hdcp_type = DW_HDCP_TYPE_HDCP22;
		} else {
			hdmi_err("%s: rx not support hdcp2.2 when force enable hdcp2.2!!!\n",
					__func__);
		}
#endif
	} else {
		/* normal flow */
		if (!hdcp->use_hdcp22) {
			_dw_hdcp14_config(dev, hdcp);
			dev->snps_hdmi_ctrl.hdcp_type = DW_HDCP_TYPE_HDCP14;
			hdmi_inf("tx not use hdcp2.2, only use hdcp1.4.\n");
		} else {
			hdcp_type = dw_hdcp22_get_sink_support(dev);
			if (hdcp_type == 1) {
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
				hdmi_inf("tx and rx both support hdcp2.2.\n");
				_dw_hdcp22_config(dev, hdcp, video);
#endif
				dev->snps_hdmi_ctrl.hdcp_type = DW_HDCP_TYPE_HDCP22;
			} else {
				hdmi_inf("rx only support hdcp1.4\n");
				_dw_hdcp14_config(dev, hdcp);
				dev->snps_hdmi_ctrl.hdcp_type = DW_HDCP_TYPE_HDCP14;
			}
		}
	}

	dev->snps_hdmi_ctrl.hdcp_enable = 1;
}

void dw_hdcp_main_disconfig(dw_hdmi_dev_t *dev)
{
	if (dev->snps_hdmi_ctrl.hdcp_enable == 0) {
		hdmi_wrn("hdcp has been disable\n");
		return;
	}

	/* dw_fc_video_set_hdcp_keepout(dev, false); */

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (hdcp22_enable)
		_dw_hdcp22_disconfig(dev);
#endif

	if (hdcp14_enable)
		_dw_hdcp14_disconfig(dev);

	dw_mc_hdcp_clock_disable(dev, true);

	dev->snps_hdmi_ctrl.hdcp_enable = 0;
}

void dw_hdcp_main_close(dw_hdmi_dev_t *dev)
{
	dev->snps_hdmi_ctrl.hdcp_enable = 0;
	dev->snps_hdmi_ctrl.hdcp_type = DW_HDCP_TYPE_NULL;

	_dw_hdcp14_disconfig(dev);
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (dev->snps_hdmi_ctrl.use_hdcp22) {
		_dw_hdcp22_disconfig(dev);
		dw_esm_close();
	}
#endif
}

int dw_hdcp_get_state(dw_hdmi_dev_t *dev)
{
	if (hdcp14_enable)
		return _dw_hdcp14_status_check_and_handle(dev);
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	else if (hdcp22_enable)
		return dw_hdcp22_status_check_and_handle();
	else if (esm_enable_fail)
		/* dw_esm_open fail */
		return -1;
#endif
	else
		/* wait for hdcp22_enable or hdcp14_enable to be set */
		return 1;
}

void dw_hdcp_config_init(dw_hdmi_dev_t *dev)
{
	/* init hdcp14 oess windows size is DW_HDCP_OESS_SIZE */
	_dw_hdcp14_set_oess_size(dev, DW_HDCP_OESS_SIZE);

	/* frame composer keepout hdcp */
	dw_fc_video_set_hdcp_keepout(dev, true);

	/* init not bypass encryption path */
	_dw_hdcp14_set_bypass_encryption(dev, false);

	/* main control disable hdcp clock */
	dw_mc_hdcp_clock_disable(dev, true);

	/* disable hdcp14 rx detect */
	_dw_hdcp14_rxdetect_enable(dev, false);

	/* disable hdcp14 encrypt */
	_dw_hdcp14_disable_encrypt(dev, true);

	/* disable hdcp data polarity */
	_dw_hdcp_enable_data_polarity(dev, 0);
}

void dw_hdcp_initial(dw_hdmi_dev_t *dev, dw_hdcp_param_t *hdcp)
{
	/* config use standard mode */
	ctrl_i2cm_ddc_fast_mode(dev, 0);

	/* config i2c speed */
	dw_i2cm_ddc_clk_config(dev, I2C_SFR_CLK,
		I2C_MIN_SS_SCL_LOW_TIME, I2C_MIN_SS_SCL_HIGH_TIME,
		I2C_MIN_FS_SCL_LOW_TIME, I2C_MIN_FS_SCL_HIGH_TIME);

	/* enable hdcp14 bypass encryption */
	_dw_hdcp14_set_bypass_encryption(dev, false);

	/* disable mc hdcp clock */
//	dw_mc_hdcp_clock_disable(dev, false);

	/* disable hdcp14 encryption */
	_dw_hdcp14_disable_encrypt(dev, true);

	/* reset hdcp14 status flag */
	hdcp14_auth_enable   = 0;
	hdcp14_auth_complete = 0;
	hdcp14_encryption    = 0;

	/* hdcp22 esm initial */
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (hdcp->use_hdcp22) {
		if (dw_esm_tx_initial(hdcp->esm_hpi_base,
			(u32)hdcp->esm_firm_phy_addr, hdcp->esm_firm_vir_addr, hdcp->esm_firm_size,
			(u32)hdcp->esm_data_phy_addr, hdcp->esm_data_vir_addr, hdcp->esm_data_size))
		hdmi_err("%s: esm tx initial failed\n", __func__);
	}
#endif
}

void dw_hdcp_exit(void)
{
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	dw_esm_tx_exit();
#endif
}

ssize_t dw_hdcp_config_dump(dw_hdmi_dev_t *dev, char *buf)
{
	ssize_t n = 0;
	dw_hdmi_ctrl_t *ctrl = &dev->snps_hdmi_ctrl;

	n += sprintf(buf + n, "\n[dw hdcp]:\n");

	if (!ctrl->use_hdcp)
		n += sprintf(buf + n, " - not config use hdcp!\n");

	if (!ctrl->use_hdcp22)
		n += sprintf(buf + n, " - not config use hdcp22!\n");

	if (!ctrl->hdcp_enable) {
		n += sprintf(buf + n, "hdcp is disable.\n");
		return n;
	}

	if (ctrl->hdcp_on)
		n += sprintf(buf + n, " - hdmi mode: %s\n",
			ctrl->hdmi_on == DW_TMDS_MODE_DVI ? "dvi" : "hdmi");

	if (hdcp14_enable) {
		n += sprintf(buf + n, " - [hdcp14]\n");
		n += sprintf(buf + n, "    - auth state: %s\n",\
			hdcp14_auth_complete ? "complete" : "uncomplete");
		n += sprintf(buf + n, "    - sw encrty: %s\n",
			hdcp14_encryption ? "on" : "off");
		n += sprintf(buf + n, "    - hw encrty: %s\n",
			_dw_hdcp14_get_encryption(dev) == 1 ? "off" : "on");
		n += sprintf(buf + n, "    - bksv: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			_dw_hdcp14_get_bskv(dev, 0), _dw_hdcp14_get_bskv(dev, 1),
			_dw_hdcp14_get_bskv(dev, 2), _dw_hdcp14_get_bskv(dev, 3),
			_dw_hdcp14_get_bskv(dev, 4));
	}
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (hdcp22_enable) {
		n += sprintf(buf + n, " - [hdcp22]\n");
		n += dw_hdcp22_esm_dump(buf + n);
	}
#endif
	return n;
}

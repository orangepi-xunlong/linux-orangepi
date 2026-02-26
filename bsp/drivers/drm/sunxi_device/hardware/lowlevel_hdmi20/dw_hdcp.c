/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/

#include <linux/delay.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0)
#include <drm/drm_hdcp.h>
#else
#include <drm/display/drm_hdcp.h>
#endif

#include "dw_dev.h"
#include "dw_mc.h"
#include "dw_fc.h"
#include "dw_i2cm.h"
#include "dw_hdcp22.h"
#include "dw_hdcp.h"

static struct dw_hdcp_s  *hdcp;

/** Number of supported devices
 * (depending on instantiated KSV MEM RAM - Revocation Memory to support
 * HDCP repeaters)
 */
#define DW_HDCP14_MAX_DEVICES		(128)
#define DW_HDCP14_KSV_HEADER		(10)
#define DW_HDCP14_KSV_SHAMAX		(20)
#define DW_HDCP14_KSV_LEN			(5)
#define DW_HDCP14_OESS_SIZE			(0x40)

enum _dw_hdcp1x_state_e {
	DW_HDCP1X_IDLE = 0,
	DW_HDCP1X_KSV_READY,
	DW_HDCP1X_KSV_ERR_INVALID,
	DW_HDCP1X_KSV_ERR_DEPTH_EXCEEDED,
	DW_HDCP1X_KSV_ERR_MEM_ACCESS,
	DW_HDCP1X_ENGAGED,
	DW_HDCP1X_FAILED
};

struct _dw_hdcp14_sha_s{
	u8 mLength[8];
	u8 mBlock[64];
	int mIndex;
	bool mComputed;
	bool mCorrupted;
	unsigned mDigest[5];
} ;

static inline int _dw_hdcp_get_enable_type(void)
{
	return hdcp->enable_type;
}

static inline void _dw_hdcp1x_set_rxdetect(u8 bit)
{
	dw_write_mask(A_HDCPCFG0, A_HDCPCFG0_RXDETECT_MASK, bit);
}

static inline void _dw_hdcp1x_set_encrypt(u8 bit)
{
	dw_write_mask(A_HDCPCFG1, A_HDCPCFG1_ENCRYPTIONDISABLE_MASK, !bit);
}

static inline void _dw_hdcp1x_int_clear(u8 value)
{
	dw_write(A_APIINTCLR, value);
}

static inline void _dw_hdcp1x_int_mask(u8 value)
{
	dw_write(A_APIINTMSK, value);
}

static inline void _dw_hdcp1x_set_oess_size(u8 value)
{
	dw_write(A_OESSWCFG, value);
}

static inline void _dw_hdcp1x_set_data_polarity(u8 enable)
{
	dw_write_mask(A_VIDPOLCFG, A_VIDPOLCFG_DATAENPOL_MASK, enable);
}

static inline u8 _dw_hdcp1x_sha_get_memory_granted(void)
{
	return (u8)dw_read_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_KSVMEMACCESS_MASK);
}

void dw_hdcp1x_set_avmute(int enable)
{
	dw_write_mask(A_HDCPCFG0, A_HDCPCFG0_AVMUTE_MASK, enable);
}

int dw_hdcp_set_enable_type(int type)
{
	if (type == DW_HDCP_TYPE_NULL)
		hdcp->enable_type = 0x0;
	else
		hdcp->enable_type = BIT(type);

	hdmi_inf("dw hdcp set type: %d\n", hdcp->enable_type);
	return hdcp->enable_type;
}

void dw_hdcp_sync_tmds_mode(void)
{
	dw_tmds_mode_t mode = dw_fc_video_get_tmds_mode();

	dw_write_mask(A_HDCPCFG0, A_HDCPCFG0_HDMIDVI_MASK,
		mode == DW_TMDS_MODE_HDMI ? 0x1 : 0x0);
}

void dw_hdcp_sync_data_polarity(void)
{
	u8 hsPol = dw_fc_video_get_hsync_polarity();
	u8 vsPol = dw_fc_video_get_vsync_polarity();

	dw_write_mask(A_VIDPOLCFG, A_VIDPOLCFG_VSYNCPOL_MASK, vsPol ? 0x1 : 0x0);
	dw_write_mask(A_VIDPOLCFG, A_VIDPOLCFG_HSYNCPOL_MASK, hsPol ? 0x1 : 0x0);
	_dw_hdcp1x_set_data_polarity(DW_HDMI_ENABLE);
}

static void _dw_hdcp1x_sha_reset(struct _dw_hdcp14_sha_s *sha)
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

static void _dw_hdcp1x_sha_process_block(struct _dw_hdcp14_sha_s *sha)
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

static void _dw_hdcp1x_sha_input(struct _dw_hdcp14_sha_s *sha, const u8 *data, size_t size)
{
	int i = 0;
	unsigned j = 0;
	bool rc = true;

	if (data == 0 || size == 0) {
		hdmi_err("invalid input data\n");
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
			_dw_hdcp1x_sha_process_block(sha);

		data++;
	}
}

static void _dw_hdcp1x_sha_pad_message(struct _dw_hdcp14_sha_s *sha)
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

		_dw_hdcp1x_sha_process_block(sha);
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

	_dw_hdcp1x_sha_process_block(sha);
}

static int _dw_hdcp1x_sha_result(struct _dw_hdcp14_sha_s *sha)
{
	if (sha->mCorrupted == true)
		return false;

	if (sha->mComputed == false) {
		_dw_hdcp1x_sha_pad_message(sha);
		sha->mComputed = true;
	}
	return true;
}

static int _dw_hdcp1x_sha_verify_ksv(const u8 *data, size_t size)
{
	size_t i = 0;
	struct _dw_hdcp14_sha_s sha;

	if (data == 0 || size < (DW_HDCP14_KSV_HEADER + DW_HDCP14_KSV_SHAMAX)) {
		hdmi_err("invalid input data\n");
		return false;
	}
	_dw_hdcp1x_sha_reset(&sha);
	_dw_hdcp1x_sha_input(&sha, data, size - DW_HDCP14_KSV_SHAMAX);

	if (_dw_hdcp1x_sha_result(&sha) == false) {
		hdmi_err("cannot process SHA digest\n");
		return false;
	}

	for (i = 0; i < DW_HDCP14_KSV_SHAMAX; i++) {
		if (data[size - DW_HDCP14_KSV_SHAMAX + i] !=
				(u8) (sha.mDigest[i / 4] >> ((i % 4) * 8))) {
			hdmi_err("SHA digest does not match\n");
			return false;
		}
	}
	return true;
}

/* SHA-1 calculation by Software */
static u8 _dw_hdcp1x_sha_calculation(int *param)
{
	int timeout = 1000;
	u16 bstatus = 0;
	u16 deviceCount = 0;
	int valid = DW_HDCP1X_IDLE;
	int size = 0;
	int i = 0;

	u8 *hdcp_ksv_list_buffer = NULL;

	/* 1 - Wait for an interrupt to be triggered
		(a_apiintstat.KSVSha1calcint) */
	/* This is called from the INT_KSV_SHA1 irq
		so nothing is required for this step */

	/* 2 - Request access to KSV memory through
		setting a_ksvmemctrl.KSVMEMrequest to 1'b1 and */
	/* pool a_ksvmemctrl.KSVMEMaccess until
		this value is 1'b1 (access granted). */
	dw_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_KSVMEMREQUEST_MASK, 0x1);
	while (_dw_hdcp1x_sha_get_memory_granted() == 0 && timeout--)
		asm volatile ("nop");

	if (_dw_hdcp1x_sha_get_memory_granted() == 0) {
		dw_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_KSVMEMREQUEST_MASK, 0x0);
		hdcp_log("KSV List memory access denied");
		*param = 0;
		return DW_HDCP1X_KSV_ERR_MEM_ACCESS;
	}

	/* 3 - Read VH', M0, Bstatus, and the KSV FIFO.
	The data is stored in the revocation memory, as */
	/* provided in the "Address Mapping for Maximum Memory Allocation"
	table in the databook. */
	bstatus	 = dw_read(HDCP_BSTATUS);
	bstatus |= dw_read(HDCP_BSTATUS1) << 8;

	deviceCount = DRM_HDCP_NUM_DOWNSTREAM(bstatus);
	if (deviceCount > DW_HDCP14_MAX_DEVICES) {
		*param = 0;
		hdcp_log("depth exceeds KSV List memory");
		return DW_HDCP1X_KSV_ERR_DEPTH_EXCEEDED;
	}

	size = deviceCount * DW_HDCP14_KSV_LEN + DW_HDCP14_KSV_HEADER + DW_HDCP14_KSV_SHAMAX;

	for (i = 0; i < size; i++) {
		if (i < DW_HDCP14_KSV_HEADER) { /* BSTATUS & M0 */
			hdcp_ksv_list_buffer[(deviceCount * DW_HDCP14_KSV_LEN) + i] =
			(u8)dw_read(HDCP_BSTATUS + i);
		} else if (i < (DW_HDCP14_KSV_HEADER + (deviceCount * DW_HDCP14_KSV_LEN))) { /* KSV list */
			hdcp_ksv_list_buffer[i - DW_HDCP14_KSV_HEADER] =
			(u8)dw_read(HDCP_BSTATUS + i);
		} else { /* SHA */
			hdcp_ksv_list_buffer[i] = (u8)dw_read(HDCP_BSTATUS + i);
		}
	}

	/* 4 - Calculate the SHA-1 checksum (VH) over M0,
		Bstatus, and the KSV FIFO. */
	if (_dw_hdcp1x_sha_verify_ksv(hdcp_ksv_list_buffer, size) == true) {
		valid = DW_HDCP1X_KSV_READY;
		hdcp_log("dw hdcp1x verify ksv list ready\n");
	} else {
		valid = DW_HDCP1X_KSV_ERR_INVALID;
		hdcp_log("dw hdcp1x verify ksv not valid\n");
	}

	/* 5 - If the calculated VH equals the VH',
	set a_ksvmemctrl.SHA1fail to 0 and set */
	/* a_ksvmemctrl.KSVCTRLupd to 1.
	If the calculated VH is different from VH' then set */
	/* a_ksvmemctrl.SHA1fail to 1 and set a_ksvmemctrl.KSVCTRLupd to 1,
	forcing the controller */
	/* to re-authenticate from the beginning. */
	dw_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_KSVMEMREQUEST_MASK, 0x0);
	dw_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_SHA1FAIL_MASK,
			(valid == DW_HDCP1X_KSV_READY) ? 0 : 1);
	dw_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_KSVCTRLUPD_MASK, 1);
	dw_write_mask(A_KSVMEMCTRL, A_KSVMEMCTRL_KSVCTRLUPD_MASK, 0);

	return valid;
}

u8 _dw_hdcp1x_status_handler(int *param, u32 irq_stat)
{
	int valid = DW_HDCP1X_IDLE;

	if (irq_stat != 0)
		hdcp_log("hdcp get interrupt state: 0x%x\n", irq_stat);

	if (irq_stat == 0) {
		if (hdcp->hdcp1x_encry_delay && (hdcp->hdcp1x_auth_state == DW_HDCP1X_ENGAGED)) {
			hdcp->hdcp1x_encry_delay++;
			if (hdcp->hdcp1x_encry_delay >= 20) {
				_dw_hdcp1x_set_encrypt(DW_HDMI_ENABLE);
				hdcp->hdcp1x_encry_delay = 0;
				hdcp->hdcp1x_encrying    = DW_HDMI_ENABLE;
				hdmi_inf("dw hdcp1x start encryption\n");
			}
		}

		return hdcp->hdcp1x_auth_state;
	}

	if ((irq_stat & A_APIINTSTAT_KEEPOUTERRORINT_MASK) != 0) {
		hdcp->hdcp1x_auth_state = DW_HDCP1X_FAILED;
		hdcp->hdcp1x_encry_delay = 0;
		return DW_HDCP1X_FAILED;
	}

	if ((irq_stat & A_APIINTSTAT_LOSTARBITRATION_MASK) != 0) {
		hdcp->hdcp1x_auth_state = DW_HDCP1X_FAILED;
		hdcp->hdcp1x_encry_delay = 0;
		return DW_HDCP1X_FAILED;
	}

	if ((irq_stat & A_APIINTSTAT_I2CNACK_MASK) != 0) {
		hdcp->hdcp1x_auth_state = DW_HDCP1X_FAILED;
		hdcp->hdcp1x_encry_delay = 0;
		return DW_HDCP1X_FAILED;
	}

	if (irq_stat & A_APIINTSTAT_KSVSHA1CALCINT_MASK)
		return _dw_hdcp1x_sha_calculation(param);

	if ((irq_stat & A_APIINTSTAT_HDCP_FAILED_MASK) != 0) {
		*param = 0;
		_dw_hdcp1x_set_encrypt(false);
		hdcp->hdcp1x_auth_state = DW_HDCP1X_FAILED;
		hdcp->hdcp1x_encry_delay = 0;
		return DW_HDCP1X_FAILED;
	}

	if ((irq_stat & A_APIINTSTAT_HDCP_ENGAGED_MASK) != 0) {
		*param = 1;
		hdcp->hdcp1x_auth_state = DW_HDCP1X_ENGAGED;
		hdcp->hdcp1x_encry_delay = 1;
		return DW_HDCP1X_ENGAGED;
	}

	return valid;
}

static int _dw_hdcp1x_get_encrypt_state(void)
{
	u8 hdcp14_status = 0;
	int param;
	u8 ret = 0;

	if (!hdcp->hdcp1x_auth_done) {
		hdmi_wrn("hdcp14 auth not enable!\n");
		return DW_HDCP_DISABLE;
	}

	hdcp14_status = dw_read(A_APIINTSTAT);
	_dw_hdcp1x_int_clear(hdcp14_status);

	ret = _dw_hdcp1x_status_handler(&param, (u32)hdcp14_status);
	if ((ret != DW_HDCP1X_KSV_ERR_INVALID) && (ret != DW_HDCP1X_FAILED))
		return DW_HDCP_SUCCESS;

	return DW_HDCP_FAILED;
}

int dw_hdcp1x_enable(void)
{
	u8 hdcp_mask = 0;

	if (hdcp->hdcp1x_auth_done) {
		hdmi_inf("dw hdcp1x has been auth done!\n");
		return 0;
	}

	/* 1. disable hdcp config for hdcp1x enable flow */
	_dw_hdcp1x_set_encrypt(DW_HDMI_DISABLE);
	_dw_hdcp1x_set_rxdetect(DW_HDMI_DISABLE);
	_dw_hdcp1x_set_data_polarity(DW_HDMI_DISABLE);

	/* 2. set hdcp1x data path for hdcp1x enable flow */
	dw_hdcp_sync_tmds_mode();
	dw_hdcp_sync_data_polarity();
	dw_fc_video_set_hdcp_keepout(DW_HDMI_ENABLE);
	dw_hdcp2x_ovr_set_path(DW_HDMI_DISABLE, DW_HDMI_ENABLE);

	/* 3. set hdcp1x basic config for hdcp1x enable flow */
	dw_write_mask(A_HDCPCFG0, A_HDCPCFG0_EN11FEATURE_MASK, DW_HDMI_DISABLE);
	dw_write_mask(A_HDCPCFG0, A_HDCPCFG0_SYNCRICHECK_MASK, DW_HDMI_DISABLE);
	dw_write_mask(A_HDCPCFG0, A_HDCPCFG0_I2CFASTMODE_MASK, DW_HDMI_DISABLE);
	dw_write_mask(A_HDCPCFG0, A_HDCPCFG0_ELVENA_MASK, DW_HDMI_DISABLE);
	dw_write_mask(A_VIDPOLCFG, A_VIDPOLCFG_UNENCRYPTCONF_MASK, 0x2);
	dw_write_mask(A_HDCPCFG1, A_HDCPCFG1_PH2UPSHFTENC_MASK, DW_HDMI_ENABLE);

	_dw_hdcp1x_set_oess_size(DW_HDCP14_OESS_SIZE);
	dw_hdcp1x_set_avmute(DW_HDMI_DISABLE);

	/* 4. set hdcp1x hardware reset for hdcp1x enable flow */
	dw_write_mask(A_HDCPCFG1, A_HDCPCFG1_SWRESET_MASK, DW_HDMI_DISABLE);

	/* 5. mask and clear hdcp1x interrupt state */
	hdcp_mask  = A_APIINTCLR_KSVACCESSINT_MASK;
	hdcp_mask |= A_APIINTCLR_KSVSHA1CALCINT_MASK;
	hdcp_mask |= A_APIINTCLR_KEEPOUTERRORINT_MASK;
	hdcp_mask |= A_APIINTCLR_LOSTARBITRATION_MASK;
	hdcp_mask |= A_APIINTCLR_I2CNACK_MASK;
	hdcp_mask |= A_APIINTCLR_HDCP_FAILED_MASK;
	hdcp_mask |= A_APIINTCLR_HDCP_ENGAGED_MASK;
	_dw_hdcp1x_int_clear(hdcp_mask);
	_dw_hdcp1x_int_mask(hdcp_mask);

	/* 6. enable and start auth for hdcp1x enable flow */
	_dw_hdcp1x_set_rxdetect(DW_HDMI_ENABLE);
	dw_mc_set_clk(DW_MC_CLK_HDCP, DW_HDMI_ENABLE);

	hdcp->hdcp1x_auth_done  = DW_HDMI_ENABLE;
	hdcp->hdcp1x_auth_state = DW_HDCP1X_FAILED;
	dw_hdcp_set_enable_type(DW_HDCP_TYPE_HDCP14);
	return 0;
}

int dw_hdcp1x_disable(void)
{
	_dw_hdcp1x_set_encrypt(DW_HDMI_DISABLE);

	dw_hdcp_set_enable_type(DW_HDCP_TYPE_NULL);

	hdcp->hdcp1x_auth_state = DW_HDCP1X_FAILED;
	hdcp->hdcp1x_auth_done  = DW_HDMI_DISABLE;
	hdcp->hdcp1x_encrying   = DW_HDMI_DISABLE;
	hdcp_log("hdcp14 disconfig done!\n");
	return 0;
}

int dw_hdcp_get_state(void)
{
	int ret = DW_HDCP_DISABLE;

	if (_dw_hdcp_get_enable_type() & BIT(DW_HDCP_TYPE_HDCP22))
		ret = dw_hdcp2x_get_encrypt_state();
	else if (_dw_hdcp_get_enable_type() & BIT(DW_HDCP_TYPE_HDCP14))
		ret = _dw_hdcp1x_get_encrypt_state();

	return ret;
}

void dw_hdcp_initial(void)
{
	struct dw_hdmi_dev_s  *hdmi = dw_get_hdmi();

	hdcp = &hdmi->hdcp_dev;

	dw_i2cm_re_init();

	/* enable hdcp14 bypass encryption */
	dw_write_mask(A_HDCPCFG0, A_HDCPCFG0_BYPENCRYPTION_MASK, DW_HDMI_DISABLE);

	/* disable hdcp14 encryption */
	_dw_hdcp1x_set_encrypt(DW_HDMI_DISABLE);

	/* reset hdcp14 status flag */
	hdcp->hdcp1x_auth_done = DW_HDMI_DISABLE;
	hdcp->hdcp1x_encrying  = DW_HDMI_DISABLE;
}

void dw_hdcp_exit(void)
{
	dw_hdcp2x_exit();
}

ssize_t dw_hdcp_dump(char *buf)
{
	ssize_t n = 0;

	if (_dw_hdcp_get_enable_type() & BIT(DW_HDCP_TYPE_HDCP14)) {
		n += sprintf(buf + n, "\n[dw hdcp1x]\n");
		n += sprintf(buf + n, "|  name  | mode  | data path | config | auth | sw encry | hw encry |            bksv              |\n");
		n += sprintf(buf + n, "|--------+-------+-----------+--------+------+----------+----------+------------------------------|\n");
		n += sprintf(buf + n, "| state  | %-4s  |  %-6s   |  %-4s  | %-4s |   %-3s    |   %-3s    | 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x |\n",
			dw_read_mask(A_HDCPCFG0, A_HDCPCFG0_HDMIDVI_MASK) ? "hdmi" : "dvi",
			dw_hdcp2x_get_path() ? "hdcp2x" : "hdcp1x",
			hdcp->hdcp1x_auth_done ? "yes" : "not",
			hdcp->hdcp1x_auth_state == DW_HDCP1X_ENGAGED ? "pass" : "fail",
			hdcp->hdcp1x_encrying ? "on" : "off",
			dw_read_mask(A_HDCPCFG1, A_HDCPCFG1_ENCRYPTIONDISABLE_MASK) ? "off" : "on",
			dw_read_mask(HDCPREG_BKSV0, HDCPREG_BKSV0_HDCPREG_BKSV0_MASK),
			dw_read_mask(HDCPREG_BKSV1, HDCPREG_BKSV1_HDCPREG_BKSV1_MASK),
			dw_read_mask(HDCPREG_BKSV2, HDCPREG_BKSV2_HDCPREG_BKSV2_MASK),
			dw_read_mask(HDCPREG_BKSV3, HDCPREG_BKSV3_HDCPREG_BKSV3_MASK),
			dw_read_mask(HDCPREG_BKSV4, HDCPREG_BKSV4_HDCPREG_BKSV4_MASK));
	} else if (_dw_hdcp_get_enable_type() & BIT(DW_HDCP_TYPE_HDCP22)) {
		n += dw_hdcp2x_dump(buf + n);
	} else {
		n += sprintf(buf + n, "\n[dw hdcp]\n");
		n += sprintf(buf + n, "|  name  | mode  |  path  |\n");
		n += sprintf(buf + n, "|--------+-------+--------|\n");
		n += sprintf(buf + n, "| state  | %-5s | %-6s |\n",
			dw_read_mask(A_HDCPCFG0, A_HDCPCFG0_HDMIDVI_MASK) ? "hdmi" : "dvi",
			dw_hdcp2x_get_path() ? "hdcp2x" : "hdcp1x");
	}
	return n;
}

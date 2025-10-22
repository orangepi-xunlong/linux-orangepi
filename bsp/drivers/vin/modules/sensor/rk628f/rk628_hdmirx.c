// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Rockchip Electronics Co. Ltd.
 *
 * Author: Shunqing Chen <csq@rock-chips.com>
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/soc/rockchip/rk_vendor_storage.h>
#include <linux/slab.h>

#include "../sensor_helper.h"
#include "rk628.h"
#include "rk628_combrxphy.h"
#include "rk628_cru.h"
#include "rk628_hdmirx.h"

#define SENSOR_NAME "rk628f_mipi"
#define INIT_FIFO_STATE			64

struct rk628_audiostate {
	u32 hdmirx_aud_clkrate;
	u32 fs_audio;
	u32 ctsn_flag;
	u32 fifo_flag;
	int init_state;
	int pre_state;
	bool fifo_int;
	bool audio_enable;
};

struct rk628_audioinfo {
	struct delayed_work delayed_work_audio_rate_change;
	struct delayed_work delayed_work_audio;
	struct mutex *confctl_mutex;
	struct rk628 *rk628;
	struct rk628_audiostate audio_state;
	bool i2s_enabled_default;
	bool i2s_enabled;
	int debug;
	bool fifo_ints_en;
	bool ctsn_ints_en;
	bool audio_present;
	struct device *dev;
};

struct hdmirx_tmdsclk_cnt {
	u32 tmds_cnt;
	u8  cnt;
};

#define HDMIRX_GET_TMDSCLK_TIME		21

static int supported_fs[] = {
	32000,
	44100,
	48000,
	88200,
	96000,
	176400,
	192000,
	768000,
	-1
};

static int hdcp_load_keys_cb(struct rk628 *rk628, struct rk628_hdcp *hdcp)
{
	int size = 0;
	u8 hdcp_vendor_data[320];

	hdcp->keys = kmalloc(HDCP_KEY_SIZE, GFP_KERNEL);
	if (!hdcp->keys)
		return -ENOMEM;

	hdcp->seeds = kmalloc(HDCP_KEY_SEED_SIZE, GFP_KERNEL);
	if (!hdcp->seeds) {
		kfree(hdcp->keys);
		hdcp->keys = NULL;
		return -ENOMEM;
	}

	// size = rk_vendor_read(HDMIRX_HDCP1X_ID, hdcp_vendor_data, 314);
	dev_err(rk628->dev, "HDCP: implicit declaration of function 'rk_vendor_read'\n");
	if (size < (HDCP_KEY_SIZE + HDCP_KEY_SEED_SIZE)) {
		dev_dbg(rk628->dev, "HDCP: read size %d\n", size);
		kfree(hdcp->keys);
		hdcp->keys = NULL;
		kfree(hdcp->seeds);
		hdcp->seeds = NULL;
		return -EINVAL;
	}
	memcpy(hdcp->keys, hdcp_vendor_data, HDCP_KEY_SIZE);
	memcpy(hdcp->seeds, hdcp_vendor_data + HDCP_KEY_SIZE,
	       HDCP_KEY_SEED_SIZE);

	return 0;
}

static int rk628_hdmi_hdcp_load_key(struct rk628 *rk628, struct rk628_hdcp *hdcp)
{
	int i;
	int ret;
	struct hdcp_keys *hdcp_keys;
	u32 seeds = 0;

	if (!hdcp->keys) {
		ret = hdcp_load_keys_cb(rk628, hdcp);
		if (ret) {
			dev_err(rk628->dev, "HDCP: load key failed\n");
			return ret;
		}
	}
	hdcp_keys = hdcp->keys;

	rk628_i2c_update_bits(rk628, HDMI_RX_HDCP_CTRL,
			HDCP_ENABLE_MASK |
			HDCP_ENC_EN_MASK,
			HDCP_ENABLE(0) |
			HDCP_ENC_EN(0));
	rk628_i2c_update_bits(rk628, GRF_SYSTEM_CON0,
			SW_ADAPTER_I2CSLADR_MASK |
			SW_EFUSE_HDCP_EN_MASK,
			SW_ADAPTER_I2CSLADR(0) |
			SW_EFUSE_HDCP_EN(1));
	/* The useful data in ksv should be 5 byte */
	for (i = 0; i < KSV_LEN; i++)
		rk628_i2c_write(rk628, HDCP_KEY_KSV0 + i * 4,
					hdcp_keys->KSV[i]);

	for (i = 0; i < HDCP_PRIVATE_KEY_SIZE; i++)
		rk628_i2c_write(rk628, HDCP_KEY_DPK0 + i * 4,
				hdcp_keys->devicekey[i]);

	rk628_i2c_update_bits(rk628, GRF_SYSTEM_CON0,
			SW_ADAPTER_I2CSLADR_MASK |
			SW_EFUSE_HDCP_EN_MASK,
			SW_ADAPTER_I2CSLADR(0) |
			SW_EFUSE_HDCP_EN(0));
	rk628_i2c_update_bits(rk628, HDMI_RX_HDCP_CTRL,
			HDCP_ENABLE_MASK |
			HDCP_ENC_EN_MASK,
			HDCP_ENABLE(1) |
			HDCP_ENC_EN(1));

	/* Enable decryption logic */
	if (hdcp->seeds) {
		seeds = (hdcp->seeds[0] & 0xff) << 8;
		seeds |= (hdcp->seeds[1] & 0xff);
	}
	if (seeds) {
		rk628_i2c_update_bits(rk628, HDMI_RX_HDCP_CTRL,
				   KEY_DECRIPT_ENABLE_MASK,
				   KEY_DECRIPT_ENABLE(1));
		rk628_i2c_write(rk628, HDMI_RX_HDCP_SEED, seeds);
	} else {
		rk628_i2c_update_bits(rk628, HDMI_RX_HDCP_CTRL,
				   KEY_DECRIPT_ENABLE_MASK,
				   KEY_DECRIPT_ENABLE(0));
	}

	return 0;
}

void rk628_hdmirx_set_hdcp(struct rk628 *rk628, struct rk628_hdcp *hdcp, bool en)
{
	sensor_dbg("%s: %sable\n", __func__, en ? "en" : "dis");

	if (en) {
		rk628_hdmi_hdcp_load_key(rk628, hdcp);
	} else {
		rk628_i2c_update_bits(rk628, HDMI_RX_HDCP_CTRL,
				      HDCP_ENABLE_MASK | HDCP_ENC_EN_MASK,
				      HDCP_ENABLE(0) | HDCP_ENC_EN(0));
	}
}
EXPORT_SYMBOL(rk628_hdmirx_set_hdcp);

void rk628_hdmirx_controller_setup(struct rk628 *rk628)
{
	rk628_i2c_write(rk628, HDMI_RX_HDMI20_CONTROL, 0x10000011);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_MODE_RECOVER, 0x00000021);
	rk628_i2c_write(rk628, HDMI_RX_PDEC_CTRL, 0xbfff8011);
	rk628_i2c_write(rk628, HDMI_RX_PDEC_ASP_CTRL, 0x00000040);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_RESMPL_CTRL, 0x00000000);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_SYNC_CTRL, 0x00000014);
	rk628_i2c_write(rk628, HDMI_RX_PDEC_ERR_FILTER, 0x00000008);
	rk628_i2c_write(rk628, HDMI_RX_SCDC_I2CCONFIG, 0x01000000);
	rk628_i2c_write(rk628, HDMI_RX_SCDC_CONFIG, 0x00000001);
	rk628_i2c_write(rk628, HDMI_RX_SCDC_WRDATA0, 0xabcdef01);
	rk628_i2c_write(rk628, HDMI_RX_CHLOCK_CONFIG, 0x0030c15c);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_ERROR_PROTECT, 0x000d0c98);
	rk628_i2c_write(rk628, HDMI_RX_MD_HCTRL1, 0x00000010);
	rk628_i2c_write(rk628, HDMI_RX_MD_HCTRL2, 0x0000173a);
	rk628_i2c_write(rk628, HDMI_RX_MD_VCTRL, 0x00000002);
	rk628_i2c_write(rk628, HDMI_RX_MD_VTH, 0x0000073a);
	rk628_i2c_write(rk628, HDMI_RX_MD_IL_POL, 0x00000004);
	rk628_i2c_write(rk628, HDMI_RX_PDEC_ACRM_CTRL, 0x00000000);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_DCM_CTRL, 0x00040414);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_CKM_EVLTM, 0x00103e70);
	rk628_i2c_write(rk628, HDMI_RX_HDMI_CKM_F, 0x0c1c0b54);
	rk628_i2c_update_bits(rk628, HDMI_RX_HDMI_TIMER_CTRL, VIDEO_PERIOD_MASK, VIDEO_PERIOD(1));

	rk628_i2c_update_bits(rk628, HDMI_RX_HDCP_SETTINGS,
			      HDMI_RESERVED_MASK |
			      FAST_I2C_MASK |
			      ONE_DOT_ONE_MASK |
			      FAST_REAUTH_MASK,
			      HDMI_RESERVED(1) |
			      FAST_I2C(0) |
			      ONE_DOT_ONE(0) |
			      FAST_REAUTH(0));
}
EXPORT_SYMBOL(rk628_hdmirx_controller_setup);

int rk628_hdmirx_get_hdcp_enc_status(struct rk628 *rk628)
{
	u32 val;

	rk628_i2c_read(rk628, HDMI_RX_HDCP_STS, &val);
	val &= HDCP_ENC_STATE;

	return val ? 1 : 0;
}
EXPORT_SYMBOL(rk628_hdmirx_get_hdcp_enc_status);

static bool is_validfs(int fs)
{
	int i = 0;
	int fs_t;

	fs_t = supported_fs[i++];
	while (fs_t > 0) {
		if (fs == fs_t)
			return true;
		fs_t = supported_fs[i++];
	};
	return false;
}

static int rk628_hdmirx_audio_find_closest_fs(struct rk628_audioinfo *aif, int fs)
{
	int i = 0;
	int fs_t;
	int difference;

	fs_t = supported_fs[i++];
	while (fs_t > 0) {
		difference = abs(fs - fs_t);
		if (difference <= 2000) {
			if (fs != fs_t)
				dev_dbg(aif->dev, "%s fix fs from %u to %u", __func__, fs, fs_t);
			return fs_t;
		}
		fs_t = supported_fs[i++];
	};
	return fs_t;
}

static void rk628_hdmirx_audio_fifo_init(struct rk628_audioinfo *aif)
{

	dev_dbg(aif->dev, "%s initial fifo\n", __func__);
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_ICLR, 0x1f);
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_CTRL, 0x10001);
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_CTRL, 0x10000);
	aif->audio_state.pre_state = aif->audio_state.init_state = INIT_FIFO_STATE*4;
}

static void rk628_hdmirx_audio_fifo_initd(struct rk628_audioinfo *aif)
{

	dev_dbg(aif->dev, "%s double initial fifo\n", __func__);
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_ICLR, 0x1f);
	rk628_i2c_update_bits(aif->rk628, HDMI_RX_AUD_FIFO_TH,
			   AFIF_TH_START_MASK,
			   AFIF_TH_START(192));
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_CTRL, 0x10001);
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_CTRL, 0x10000);
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_CTRL, 0x10001);
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_CTRL, 0x10000);
	rk628_i2c_update_bits(aif->rk628, HDMI_RX_AUD_FIFO_TH,
			   AFIF_TH_START_MASK,
			   AFIF_TH_START(INIT_FIFO_STATE));
	aif->audio_state.pre_state = aif->audio_state.init_state = INIT_FIFO_STATE*4;
}

static u32 _rk628_hdmirx_audio_fs(struct rk628_audioinfo *aif)
{
	u64 tmdsclk = 0;
	u32 clkrate = 0, cts_decoded = 0, n_decoded = 0, fs_audio = 0;

	/* fout=128*fs=ftmds*N/CTS */
	rk628_i2c_read(aif->rk628, HDMI_RX_HDMI_CKM_RESULT, &clkrate);
	clkrate = clkrate & 0xffff;
	/* tmdsclk = (clkrate/1000) * 49500000 */
	tmdsclk = clkrate * (49500000 / 1000);
	rk628_i2c_read(aif->rk628, HDMI_RX_PDEC_ACR_CTS, &cts_decoded);
	rk628_i2c_read(aif->rk628, HDMI_RX_PDEC_ACR_N, &n_decoded);
	if (cts_decoded != 0) {
		fs_audio = div_u64((tmdsclk * n_decoded), cts_decoded);
		fs_audio /= 128;
		fs_audio = rk628_hdmirx_audio_find_closest_fs(aif, fs_audio);
	}
	dev_dbg(aif->dev,
		"%s: clkrate:%u tmdsclk:%llu, n_decoded:%u, cts_decoded:%u, fs_audio:%u\n",
		__func__, clkrate, tmdsclk, n_decoded, cts_decoded, fs_audio);
	if (!is_validfs(fs_audio))
		fs_audio = 0;
	return fs_audio;
}

static void rk628_hdmirx_audio_clk_set_rate(struct rk628_audioinfo *aif, u32 rate)
{

	dev_dbg(aif->dev, "%s: %u to %u\n",
		 __func__, aif->audio_state.hdmirx_aud_clkrate, rate);
	rk628_clk_set_rate(aif->rk628, CGU_CLK_HDMIRX_AUD, rate);
	aif->audio_state.hdmirx_aud_clkrate = rate;
}

static void rk628_hdmirx_audio_clk_inc_rate(struct rk628_audioinfo *aif, int dis)
{
	u32 hdmirx_aud_clkrate = aif->audio_state.hdmirx_aud_clkrate + dis;

	dev_dbg(aif->dev, "%s: %u to %u\n",
		 __func__, aif->audio_state.hdmirx_aud_clkrate, hdmirx_aud_clkrate);
	rk628_clk_set_rate(aif->rk628, CGU_CLK_HDMIRX_AUD, hdmirx_aud_clkrate);
	aif->audio_state.hdmirx_aud_clkrate = hdmirx_aud_clkrate;
}

static void rk628_hdmirx_audio_set_fs(struct rk628_audioinfo *aif, u32 fs_audio)
{
	u32 hdmirx_aud_clkrate_t = fs_audio*128;

	dev_dbg(aif->dev, "%s: %u to %u with fs %u\n", __func__,
		 aif->audio_state.hdmirx_aud_clkrate, hdmirx_aud_clkrate_t,
		 fs_audio);
	rk628_clk_set_rate(aif->rk628, CGU_CLK_HDMIRX_AUD, hdmirx_aud_clkrate_t);
	aif->audio_state.hdmirx_aud_clkrate = hdmirx_aud_clkrate_t;
	aif->audio_state.fs_audio = fs_audio;
}

static void rk628_hdmirx_audio_enable(struct rk628_audioinfo *aif)
{
	u32 fifo_ints;

	rk628_i2c_read(aif->rk628, HDMI_RX_AUD_FIFO_ISTS, &fifo_ints);
	dev_dbg(aif->dev, "%s fifo ints %#x\n", __func__, fifo_ints);
	if ((fifo_ints & 0x18) == 0x18)
		rk628_hdmirx_audio_fifo_initd(aif);
	else if (fifo_ints & 0x18)
		rk628_hdmirx_audio_fifo_init(aif);
	rk628_i2c_update_bits(aif->rk628, HDMI_RX_DMI_DISABLE_IF,
			      AUD_ENABLE_MASK, AUD_ENABLE(1));
	aif->audio_state.audio_enable = true;
	aif->fifo_ints_en = true;
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_IEN_SET,
			AFIF_OVERFL_ISTS | AFIF_UNDERFL_ISTS);
}

static const char *audio_fifo_err(u32 fifo_status)
{
	switch (fifo_status & (AFIF_UNDERFL_ISTS | AFIF_OVERFL_ISTS)) {
	case AFIF_UNDERFL_ISTS:
		return "underflow";
	case AFIF_OVERFL_ISTS:
		return "overflow";
	case AFIF_UNDERFL_ISTS | AFIF_OVERFL_ISTS:
		return "underflow and overflow";
	}
	return "underflow or overflow";
}

static void rk628_csi_delayed_work_audio_v2(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct rk628_audioinfo *aif = container_of(dwork, struct rk628_audioinfo,
						   delayed_work_audio);
	struct rk628_audiostate *audio_state = &aif->audio_state;
	struct rk628 *rk628 = aif->rk628;
	u32 fs_audio, sample_flat;
	int init_state, pre_state, fifo_status, fifo_ints;
	unsigned long delay = 500;

	fs_audio = _rk628_hdmirx_audio_fs(aif);
	/* read fifo init status */
	rk628_i2c_read(rk628, HDMI_RX_AUD_FIFO_ISTS, &fifo_ints);
	dev_dbg(rk628->dev, "%s: HDMI_RX_AUD_FIFO_ISTS:%#x\r\n", __func__, fifo_ints);

	if (fifo_ints & (AFIF_UNDERFL_ISTS | AFIF_OVERFL_ISTS)) {
		dev_warn(rk628->dev, "%s: audio %s %#x, with fs %svalid %d\n",
			 __func__, audio_fifo_err(fifo_ints), fifo_ints,
			 is_validfs(fs_audio) ? "" : "in", fs_audio);
		if (is_validfs(fs_audio))
			rk628_hdmirx_audio_set_fs(aif, fs_audio);
		rk628_hdmirx_audio_fifo_init(aif);
		audio_state->pre_state = 0;
		goto exit;
	}

	/* read fifo fill status */
	init_state = audio_state->init_state;
	pre_state = audio_state->pre_state;
	rk628_i2c_read(rk628, HDMI_RX_AUD_FIFO_FILLSTS1, &fifo_status);
	dev_dbg(rk628->dev,
		"%s: HDMI_RX_AUD_FIFO_FILLSTS1:%#x, single offset:%d, total offset:%d\n",
		__func__, fifo_status, fifo_status - pre_state, fifo_status - init_state);
	if (!is_validfs(fs_audio)) {
		dev_dbg(rk628->dev, "%s: no supported fs(%u), fifo_status %d\n",
			__func__, fs_audio, fifo_status);
		delay = 1000;
	} else if (abs(fs_audio - audio_state->fs_audio) > 1000) {
		dev_info(rk628->dev, "%s: restart audio fs(%d -> %d)\n",
			 __func__, audio_state->fs_audio, fs_audio);
		rk628_hdmirx_audio_set_fs(aif, fs_audio);
		rk628_hdmirx_audio_fifo_init(aif);
		audio_state->pre_state = 0;
		goto exit;
	}
	if (fifo_status != 0) {
		if (!aif->audio_present) {
			dev_info(rk628->dev, "audio on");
			aif->audio_present = true;
		}
		if (fifo_status - init_state > 16 && fifo_status - pre_state > 0)
			rk628_hdmirx_audio_clk_inc_rate(aif, 10);
		else if (fifo_status - init_state < -16 && fifo_status - pre_state < 0)
			rk628_hdmirx_audio_clk_inc_rate(aif, -10);
	} else {
		if (aif->audio_present) {
			dev_info(rk628->dev, "audio off");
			aif->audio_present = false;
		}
	}
	audio_state->pre_state = fifo_status;

	rk628_i2c_read(rk628, HDMI_RX_AUD_SPARE, &sample_flat);
	sample_flat = sample_flat & AUDS_MAS_SAMPLE_FLAT;
	if (!sample_flat)
		rk628_i2c_update_bits(rk628, GRF_SYSTEM_CON0, SW_I2S_DATA_OEN_MASK, SW_I2S_DATA_OEN(0));
	else
		rk628_i2c_update_bits(rk628, GRF_SYSTEM_CON0, SW_I2S_DATA_OEN_MASK, SW_I2S_DATA_OEN(1));

exit:
	schedule_delayed_work(&aif->delayed_work_audio, msecs_to_jiffies(delay));
}

static void rk628_csi_delayed_work_audio(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct rk628_audioinfo *aif = container_of(dwork, struct rk628_audioinfo,
						   delayed_work_audio);
	struct rk628_audiostate *audio_state = &aif->audio_state;
	u32 fs_audio;
	int cur_state, init_state, pre_state;

	init_state = audio_state->init_state;
	pre_state = audio_state->pre_state;
	fs_audio = _rk628_hdmirx_audio_fs(aif);
	if (!is_validfs(fs_audio)) {
		dev_dbg(aif->dev, "%s: no supported fs(%u)\n", __func__, fs_audio);
		goto exit;
	}
	if (!audio_state->audio_enable) {
		rk628_hdmirx_audio_set_fs(aif, fs_audio);
		rk628_hdmirx_audio_enable(aif);
		goto exit;
	}
	if (abs(fs_audio - audio_state->fs_audio) > 1000)
		rk628_hdmirx_audio_set_fs(aif, fs_audio);
	rk628_i2c_read(aif->rk628, HDMI_RX_AUD_FIFO_FILLSTS1, &cur_state);
	dev_dbg(aif->dev, "%s: HDMI_RX_AUD_FIFO_FILLSTS1:%#x, single offset:%d, total offset:%d\n",
		 __func__, cur_state, cur_state - pre_state, cur_state - init_state);
	if (cur_state != 0)
		aif->audio_present = true;
	else
		aif->audio_present = false;

	if ((cur_state - init_state) > 16 && (cur_state - pre_state) > 0)
		rk628_hdmirx_audio_clk_inc_rate(aif, 10);
	else if ((cur_state != 0) && (cur_state - init_state) < -16 && (cur_state - pre_state) < 0)
		rk628_hdmirx_audio_clk_inc_rate(aif, -10);
	audio_state->pre_state = cur_state;
exit:
	schedule_delayed_work(&aif->delayed_work_audio, msecs_to_jiffies(1000));

}

static void rk628_csi_delayed_work_audio_rate_change(struct work_struct *work)
{
	u32 fifo_fillsts;
	u32 fs_audio;
	struct delayed_work *dwork = to_delayed_work(work);
	struct rk628_audioinfo *aif = container_of(dwork, struct rk628_audioinfo,
						   delayed_work_audio_rate_change);

	mutex_lock(aif->confctl_mutex);
	fs_audio = _rk628_hdmirx_audio_fs(aif);
	dev_dbg(aif->dev, "%s get audio fs %u\n", __func__, fs_audio);
	if (aif->audio_state.ctsn_flag == (ACR_N_CHG_ICLR | ACR_CTS_CHG_ICLR)) {
		aif->audio_state.ctsn_flag = 0;
		if (is_validfs(fs_audio)) {
			rk628_hdmirx_audio_set_fs(aif, fs_audio);
			/* We start audio work after recieveing cts n interrupt */
			rk628_hdmirx_audio_enable(aif);
		} else {
			dev_dbg(aif->dev, "%s invalid fs when ctsn updating\n", __func__);
		}
		schedule_delayed_work(&aif->delayed_work_audio, msecs_to_jiffies(1000));
	}
	if (aif->audio_state.fifo_int) {
		aif->audio_state.fifo_int = false;
		if (is_validfs(fs_audio))
			rk628_hdmirx_audio_set_fs(aif, fs_audio);
		rk628_i2c_read(aif->rk628, HDMI_RX_AUD_FIFO_FILLSTS1, &fifo_fillsts);
		if (!fifo_fillsts) {
			dev_dbg(aif->dev, "%s underflow after overflow\n", __func__);
			rk628_hdmirx_audio_fifo_initd(aif);
		} else {
			dev_dbg(aif->dev, "%s overflow after underflow\n", __func__);
			rk628_hdmirx_audio_fifo_initd(aif);
		}
	}
	mutex_unlock(aif->confctl_mutex);
}

HAUDINFO rk628_hdmirx_audioinfo_alloc(struct device *dev,
				      struct mutex *confctl_mutex,
				      struct rk628 *rk628,
				      bool en)
{
	struct rk628_audioinfo *aif;

	aif = devm_kzalloc(dev, sizeof(*aif), GFP_KERNEL);
	if (!aif)
		return NULL;
	if (rk628->version >= RK628F_VERSION) {
		INIT_DELAYED_WORK(&aif->delayed_work_audio, rk628_csi_delayed_work_audio_v2);
	} else {
		INIT_DELAYED_WORK(&aif->delayed_work_audio, rk628_csi_delayed_work_audio);
		INIT_DELAYED_WORK(&aif->delayed_work_audio_rate_change,
				   rk628_csi_delayed_work_audio_rate_change);
	}
	aif->confctl_mutex = confctl_mutex;
	aif->rk628 = rk628;
	aif->i2s_enabled_default = en;
	aif->dev = dev;
	return aif;
}
EXPORT_SYMBOL(rk628_hdmirx_audioinfo_alloc);

void rk628_hdmirx_audio_cancel_work_audio(HAUDINFO info, bool sync)
{
	struct rk628_audioinfo *aif = (struct rk628_audioinfo *)info;

	if (sync)
		cancel_delayed_work_sync(&aif->delayed_work_audio);
	else
		cancel_delayed_work(&aif->delayed_work_audio);
}
EXPORT_SYMBOL(rk628_hdmirx_audio_cancel_work_audio);

void rk628_hdmirx_audio_cancel_work_rate_change(HAUDINFO info, bool sync)
{
	struct rk628_audioinfo *aif = (struct rk628_audioinfo *)info;

	if (sync)
		cancel_delayed_work_sync(&aif->delayed_work_audio_rate_change);
	else
		cancel_delayed_work(&aif->delayed_work_audio_rate_change);
}
EXPORT_SYMBOL(rk628_hdmirx_audio_cancel_work_rate_change);

void rk628_hdmirx_audio_destroy(HAUDINFO info)
{
	struct rk628_audioinfo *aif = (struct rk628_audioinfo *)info;
	struct rk628 *rk628;

	if (!aif)
		return;
	rk628 = aif->rk628;
	rk628_hdmirx_audio_cancel_work_audio(aif, true);
	if (rk628->version < RK628F_VERSION)
		rk628_hdmirx_audio_cancel_work_rate_change(aif, true);
	aif->confctl_mutex = NULL;
	aif->rk628 = NULL;
}
EXPORT_SYMBOL(rk628_hdmirx_audio_destroy);

bool rk628_hdmirx_audio_present(HAUDINFO info)
{
	struct rk628_audioinfo *aif = (struct rk628_audioinfo *)info;

	if (!aif)
		return false;
	return aif->audio_present;
}
EXPORT_SYMBOL(rk628_hdmirx_audio_present);

int rk628_hdmirx_audio_fs(HAUDINFO info)
{
	struct rk628_audioinfo *aif = (struct rk628_audioinfo *)info;

	if (!aif)
		return 0;
	return aif->audio_state.fs_audio;
}
EXPORT_SYMBOL(rk628_hdmirx_audio_fs);

void rk628_hdmirx_audio_i2s_ctrl(HAUDINFO info, bool enable)
{
	struct rk628_audioinfo *aif = (struct rk628_audioinfo *)info;

	if (enable == aif->i2s_enabled)
		return;
	if (enable) {
		rk628_i2c_write(aif->rk628, HDMI_RX_AUD_SAO_CTRL,
				I2S_LPCM_BPCUV(0) |
				I2S_32_16(1));
	} else {
		rk628_i2c_write(aif->rk628, HDMI_RX_AUD_SAO_CTRL,
				I2S_LPCM_BPCUV(0) |
				I2S_32_16(1) |
				I2S_ENABLE_BITS(0x3f));
	}
	aif->i2s_enabled = enable;
}
EXPORT_SYMBOL(rk628_hdmirx_audio_i2s_ctrl);

void rk628_hdmirx_audio_setup(HAUDINFO info)
{
	struct rk628_audioinfo *aif = (struct rk628_audioinfo *)info;
	struct rk628 *rk628 = aif->rk628;
	u32 audio_pll_n, audio_pll_cts;

	dev_dbg(aif->dev, "%s: setup audio\n", __func__);
	audio_pll_n = 5644;
	audio_pll_cts = 148500;
	aif->audio_state.ctsn_flag = 0;
	aif->audio_state.fs_audio = 0;
	aif->audio_state.pre_state = 0;
	aif->audio_state.init_state = INIT_FIFO_STATE*4;
	aif->audio_state.fifo_int = false;
	aif->audio_state.audio_enable = false;
	aif->fifo_ints_en = false;
	aif->ctsn_ints_en = false;
	aif->i2s_enabled = false;

	if (rk628->version >= RK628F_VERSION)
		rk628_i2c_write(rk628, CRU_MODE_CON00, HIWORD_UPDATE(1, 4, 4));

	rk628_hdmirx_audio_clk_set_rate(aif, 5644800);
	/* manual aud CTS */
	rk628_i2c_write(aif->rk628, HDMI_RX_AUDPLL_GEN_CTS, audio_pll_cts);
	/* manual aud N */
	rk628_i2c_write(aif->rk628, HDMI_RX_AUDPLL_GEN_N, audio_pll_n);

	/* aud CTS N en manual */
	rk628_i2c_update_bits(aif->rk628, HDMI_RX_AUD_CLK_CTRL,
			CTS_N_REF_MASK, CTS_N_REF(1));
	/* aud pll ctrl */
	rk628_i2c_update_bits(aif->rk628, HDMI_RX_AUD_PLL_CTRL,
			PLL_LOCK_TOGGLE_DIV_MASK, PLL_LOCK_TOGGLE_DIV(0));
	rk628_i2c_update_bits(aif->rk628, HDMI_RX_AUD_FIFO_TH,
		AFIF_TH_START_MASK |
		AFIF_TH_MAX_MASK |
		AFIF_TH_MIN_MASK,
		AFIF_TH_START(64) |
		AFIF_TH_MAX(8) |
		AFIF_TH_MIN(8));

	/* AUTO_VMUTE */
	rk628_i2c_update_bits(aif->rk628, HDMI_RX_AUD_FIFO_CTRL,
			AFIF_SUBPACKET_DESEL_MASK |
			AFIF_SUBPACKETS_MASK,
			AFIF_SUBPACKET_DESEL(0) |
			AFIF_SUBPACKETS(1));
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_SAO_CTRL,
			I2S_LPCM_BPCUV(0) |
			I2S_32_16(1)|
			(aif->i2s_enabled_default ? 0 : I2S_ENABLE_BITS(0x3f)));
	aif->i2s_enabled = aif->i2s_enabled_default;
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_MUTE_CTRL,
			APPLY_INT_MUTE(0)	|
			APORT_SHDW_CTRL(3)	|
			AUTO_ACLK_MUTE(2)	|
			AUD_MUTE_SPEED(1)	|
			AUD_AVMUTE_EN(1)	|
			AUD_MUTE_SEL(0)		|
			AUD_MUTE_MODE(1));

	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_PAO_CTRL,
			PAO_RATE(0));
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_CHEXTR_CTRL,
			AUD_LAYOUT_CTRL(1));
	if (rk628->version >= RK628F_VERSION) {
		rk628_i2c_update_bits(aif->rk628, HDMI_RX_DMI_DISABLE_IF,
				AUD_ENABLE_MASK, AUD_ENABLE(1));
		schedule_delayed_work(&aif->delayed_work_audio, msecs_to_jiffies(1000));
	} else {
		aif->ctsn_ints_en = true;
		rk628_i2c_write(aif->rk628, HDMI_RX_PDEC_IEN_SET,
				ACR_N_CHG_ICLR | ACR_CTS_CHG_ICLR);
		/* audio detect */
		rk628_i2c_write(aif->rk628, HDMI_RX_PDEC_AUDIODET_CTRL, AUDIODET_THRESHOLD(0));
	}
}
EXPORT_SYMBOL(rk628_hdmirx_audio_setup);

bool rk628_audio_fifoints_enabled(HAUDINFO info)
{
	return ((struct rk628_audioinfo *)info)->fifo_ints_en;
}
EXPORT_SYMBOL(rk628_audio_fifoints_enabled);

bool rk628_audio_ctsnints_enabled(HAUDINFO info)
{
	return ((struct rk628_audioinfo *)info)->ctsn_ints_en;
}
EXPORT_SYMBOL(rk628_audio_ctsnints_enabled);

void rk628_csi_isr_ctsn(HAUDINFO info, u32 pdec_ints)
{
	struct rk628_audioinfo *aif = (struct rk628_audioinfo *)info;
	u32 ctsn_mask = ACR_N_CHG_ICLR | ACR_CTS_CHG_ICLR;

	dev_dbg(aif->dev, "%s: pdec_ints:%#x\n", __func__, pdec_ints);
	/* cts & n both need update but maybe come diff int */
	if (pdec_ints & ACR_N_CHG_ICLR)
		aif->audio_state.ctsn_flag |= ACR_N_CHG_ICLR;
	if (pdec_ints & ACR_CTS_CHG_ICLR)
		aif->audio_state.ctsn_flag |= ACR_CTS_CHG_ICLR;
	if (aif->audio_state.ctsn_flag == ctsn_mask) {
		dev_dbg(aif->dev, "%s: ctsn updated, disable ctsn int\n", __func__);
		rk628_i2c_write(aif->rk628, HDMI_RX_PDEC_IEN_CLR, ctsn_mask);
		aif->ctsn_ints_en = false;
		schedule_delayed_work(&aif->delayed_work_audio_rate_change, 0);
	}
	rk628_i2c_write(aif->rk628, HDMI_RX_PDEC_ICLR, pdec_ints & ctsn_mask);
}
EXPORT_SYMBOL(rk628_csi_isr_ctsn);

void rk628_csi_isr_fifoints(HAUDINFO info, u32 fifo_ints)
{
	struct rk628_audioinfo *aif = (struct rk628_audioinfo *)info;
	u32 fifo_mask = AFIF_OVERFL_ISTS | AFIF_UNDERFL_ISTS;

	dev_dbg(aif->dev, "%s: fifo_ints:%#x\n", __func__, fifo_ints);
	/* cts & n both need update but maybe come diff int */
	if (fifo_ints & AFIF_OVERFL_ISTS) {
		dev_dbg(aif->dev, "%s: Audio FIFO overflow\n", __func__);
		aif->audio_state.fifo_flag |= AFIF_OVERFL_ISTS;
	}
	if (fifo_ints & AFIF_UNDERFL_ISTS) {
		dev_dbg(aif->dev, "%s: Audio FIFO underflow\n", __func__);
		aif->audio_state.fifo_flag |= AFIF_UNDERFL_ISTS;
	}
	if (aif->audio_state.fifo_flag == fifo_mask) {
		aif->audio_state.fifo_int = true;
		aif->audio_state.fifo_flag = 0;
		schedule_delayed_work(&aif->delayed_work_audio_rate_change, 0);
	}
	rk628_i2c_write(aif->rk628, HDMI_RX_AUD_FIFO_ICLR, fifo_ints & fifo_mask);
}
EXPORT_SYMBOL(rk628_csi_isr_fifoints);

int rk628_is_avi_ready(struct rk628 *rk628, bool avi_rcv_rdy)
{
	u8 i;
	u32 val, avi_pb = 0;
	u8 cnt = 0, max_cnt = 2;
	u32 hdcp_ctrl_val = 0;

	if (rk628->version >= RK628F_VERSION)
		return 1;

	rk628_i2c_read(rk628, HDMI_RX_HDCP_CTRL, &val);
	if ((val & HDCP_ENABLE_MASK))
		max_cnt = 5;

	for (i = 0; i < 100; i++) {
		rk628_i2c_read(rk628, HDMI_RX_PDEC_AVI_PB, &val);
		dev_info(rk628->dev, "%s PDEC_AVI_PB:%#x, avi_rcv_rdy:%d\n",
			 __func__, val, avi_rcv_rdy);
		if (i > 30 && !(hdcp_ctrl_val & 0x400)) {
			rk628_i2c_read(rk628, HDMI_RX_HDCP_CTRL, &hdcp_ctrl_val);
			/* force hdcp avmute */
			hdcp_ctrl_val |= 0x400;
			rk628_i2c_write(rk628, HDMI_RX_HDCP_CTRL, hdcp_ctrl_val);
		}

		if (val && val == avi_pb && avi_rcv_rdy) {
			if (++cnt >= max_cnt)
				break;
		} else {
			cnt = 0;
			avi_pb = val;
		}
		msleep(30);
	}
	if (cnt < max_cnt)
		return 0;

	return 1;
}
EXPORT_SYMBOL(rk628_is_avi_ready);

static void hdmirxphy_write(struct rk628 *rk628,  u32 offset, u32 val)
{
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_ADDRESS, offset);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_DATAO, val);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_OPERATION, 1);
}

static __maybe_unused u32 hdmirxphy_read(struct rk628 *rk628, u32 offset)
{
	u32 val;

	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_ADDRESS, offset);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_OPERATION, 2);
	rk628_i2c_read(rk628, HDMI_RX_I2CM_PHYG3_DATAI, &val);

	return val;
}

static void rk628_hdmirxphy_enable(struct rk628 *rk628, bool is_hdmi2, bool scramble_en)
{
	hdmirxphy_write(rk628, 0x02, 0x1860);
	hdmirxphy_write(rk628, 0x03, 0x0060);
	if (!is_hdmi2 && scramble_en)
		hdmirxphy_write(rk628, 0x0d, 0x00c0);
	else
		hdmirxphy_write(rk628, 0x0d, 0x0);
	hdmirxphy_write(rk628, 0x27, 0x1c94);
	hdmirxphy_write(rk628, 0x28, 0x3713);
	hdmirxphy_write(rk628, 0x29, 0x24da);
	hdmirxphy_write(rk628, 0x2a, 0x5492);
	hdmirxphy_write(rk628, 0x2b, 0x4b0d);
	hdmirxphy_write(rk628, 0x2d, 0x008c);
	hdmirxphy_write(rk628, 0x2e, 0x0001);

	if (is_hdmi2)
		hdmirxphy_write(rk628, 0x0e, 0x0108);
	else
		hdmirxphy_write(rk628, 0x0e, 0x0008);

}

static void rk628_hdmirxphy_set_clrdpt(struct rk628 *rk628, bool is_8bit)
{
	if (is_8bit)
		hdmirxphy_write(rk628, 0x03, 0x0000);
	else
		hdmirxphy_write(rk628, 0x03, 0x0060);
}

static int rk628_hdmirx_cec_log_addr(struct cec_adapter *adap, u8 logical_addr)
{
	struct rk628_hdmirx_cec *cec = cec_get_drvdata(adap);
	struct rk628 *rk628 = cec->rk628;

	if (logical_addr == CEC_LOG_ADDR_INVALID)
		cec->addresses = 0;
	else
		cec->addresses |= BIT(logical_addr) | BIT(15);

	rk628_i2c_write(rk628, HDMI_RX_CEC_ADDR_L, cec->addresses & 0xff);
	rk628_i2c_write(rk628, HDMI_RX_CEC_ADDR_H, (cec->addresses >> 8) & 0xff);

	return 0;
}

static int rk628_hdmirx_cec_enable(struct cec_adapter *adap, bool enable)
{
	struct rk628_hdmirx_cec *cec = cec_get_drvdata(adap);
	struct rk628 *rk628 = cec->rk628;

	if (!enable) {
		rk628_i2c_write(rk628, HDMI_RX_AUD_CEC_IEN_CLR, ~0);
		rk628_i2c_update_bits(rk628, HDMI_RX_DMI_DISABLE_IF, CEC_ENABLE_MASK, 0);
	} else {
		unsigned int irqs;

		rk628_hdmirx_cec_log_addr(cec->adap, CEC_LOG_ADDR_INVALID);
		rk628_i2c_update_bits(rk628, HDMI_RX_DMI_DISABLE_IF, CEC_ENABLE_MASK,
				      CEC_ENABLE_MASK);

		rk628_i2c_write(rk628, HDMI_RX_CEC_CTRL, 0);
		rk628_i2c_write(rk628, HDMI_RX_AUD_CEC_ICLR, ~0);
		rk628_i2c_write(rk628, HDMI_RX_CEC_LOCK, 0);

		irqs = ERROR_INIT_ENSET | NACK_ENSET | EOM_ENSET | DONE_ENSET;
		rk628_i2c_write(rk628, HDMI_RX_AUD_CEC_IEN_SET, irqs);
	}

	return 0;
}

static int rk628_hdmirx_cec_transmit(struct cec_adapter *adap, u8 attempts,
				     u32 signal_free_time, struct cec_msg *msg)
{
	struct rk628_hdmirx_cec *cec = cec_get_drvdata(adap);
	struct rk628 *rk628 = cec->rk628;
	int i, msg_len;
	unsigned int ctrl;

	switch (signal_free_time) {
	case CEC_SIGNAL_FREE_TIME_RETRY:
		ctrl = CEC_CTRL_RETRY;
		break;
	case CEC_SIGNAL_FREE_TIME_NEW_INITIATOR:
	default:
		ctrl = CEC_CTRL_NORMAL;
		break;
	case CEC_SIGNAL_FREE_TIME_NEXT_XFER:
		ctrl = CEC_CTRL_IMMED;
		break;
	}

	msg_len = msg->len;
	if (msg->len > 16)
		msg_len = 16;
	if (msg_len <= 0)
		return 0;

	for (i = 0; i < msg_len; i++)
		rk628_i2c_write(rk628, HDMI_RX_CEC_TX_DATA_0 + i * 4, msg->msg[i]);

	rk628_i2c_write(rk628, HDMI_RX_CEC_TX_CNT, msg_len);
	rk628_i2c_write(rk628, HDMI_RX_CEC_CTRL, ctrl | CEC_SEND);

	return 0;
}

static const struct cec_adap_ops rk628_hdmirx_cec_ops = {
	.adap_enable = rk628_hdmirx_cec_enable,
	.adap_log_addr = rk628_hdmirx_cec_log_addr,
	.adap_transmit = rk628_hdmirx_cec_transmit,
};

static void rk628_hdmirx_cec_del(void *data)
{
	struct rk628_hdmirx_cec *cec = data;

	cec_delete_adapter(cec->adap);
}

void rk628_hdmirx_cec_irq(struct rk628 *rk628, struct rk628_hdmirx_cec *cec)
{
	u32 stat, val;

	rk628_i2c_read(rk628, HDMI_RX_AUD_CEC_ISTS, &stat);
	if (stat == 0)
		return;

	rk628_i2c_write(rk628, HDMI_RX_AUD_CEC_ICLR, stat);

	if (stat & ERROR_INIT) {
		cec->tx_status = CEC_TX_STATUS_ERROR;
		cec->tx_done = true;
	} else if (stat & DONE) {
		cec->tx_status = CEC_TX_STATUS_OK;
		cec->tx_done = true;
	} else if (stat & NACK) {
		cec->tx_status = CEC_TX_STATUS_NACK;
		cec->tx_done = true;
	}

	if (stat & EOM) {
		unsigned int len, i;

		rk628_i2c_read(rk628, HDMI_RX_CEC_RX_CNT, &val);
		len = val & 0x1f;
		if (len > sizeof(cec->rx_msg.msg))
			len = sizeof(cec->rx_msg.msg);

		for (i = 0; i < len; i++) {
			rk628_i2c_read(rk628, HDMI_RX_CEC_RX_DATA_0 + i * 4, &val);
			cec->rx_msg.msg[i] = val & 0xff;
		}
		rk628_i2c_write(rk628, HDMI_RX_CEC_LOCK, 0);

		cec->rx_msg.len = len;
		cec->rx_done = true;
	}

	if (cec->tx_done) {
		cec->tx_done = false;
		//cec_transmit_attempt_done(cec->adap, cec->tx_status);
		dev_err(rk628->dev, "CEC: implicit declaration of function 'cec_transmit_attempt_done'\n");
	}
	if (cec->rx_done) {
		cec->rx_done = false;
		//cec_received_msg(cec->adap, &cec->rx_msg);
		dev_err(rk628->dev, "CEC: implicit declaration of function 'cec_received_msg'\n");
	}
}
EXPORT_SYMBOL(rk628_hdmirx_cec_irq);

struct rk628_hdmirx_cec *rk628_hdmirx_cec_register(struct rk628 *rk628)
{
	struct rk628_hdmirx_cec *cec;
	int ret;
	unsigned int irqs;

	if (!rk628)
		return NULL;

	/*
	 * Our device is just a convenience - we want to link to the real
	 * hardware device here, so that userspace can see the association
	 * between the HDMI hardware and its associated CEC chardev.
	 */
	cec = devm_kzalloc(rk628->dev, sizeof(*cec), GFP_KERNEL);
	if (!cec)
		return NULL;

	cec->rk628 = rk628;
	cec->dev = rk628->dev;

	rk628_i2c_write(rk628, HDMI_RX_CEC_MASK, 0);
	rk628_i2c_update_bits(rk628, HDMI_RX_DMI_DISABLE_IF, CEC_ENABLE_MASK, CEC_ENABLE_MASK);

	rk628_i2c_write(rk628, HDMI_RX_CEC_TX_CNT, 0);
	rk628_i2c_write(rk628, HDMI_RX_CEC_RX_CNT, 0);
	/* clk_hdmirx_cec = 32.768k */
	rk628_clk_set_rate(rk628, CGU_CLK_HDMIRX_CEC, 32768);

	/*
	cec->adap = cec_allocate_adapter(&rk628_hdmirx_cec_ops, cec, "rk628-hdmirx",
					 CEC_CAP_LOG_ADDRS | CEC_CAP_TRANSMIT |
					 CEC_CAP_RC | CEC_CAP_PASSTHROUGH,
					 CEC_MAX_LOG_ADDRS);
	*/
	dev_err(rk628->dev, "CEC: implicit declaration of function 'cec_allocate_adapter'\n");

	if (IS_ERR(cec->adap)) {
		dev_err(cec->dev, "cec adap allocate failed!\n");
		return NULL;
	}

	/* override the module pointer */
	cec->adap->owner = THIS_MODULE;

	ret = devm_add_action(cec->dev, rk628_hdmirx_cec_del, cec);
	if (ret) {
		cec_delete_adapter(cec->adap);
		return NULL;
	}

	cec->notify = cec_notifier_cec_adap_register(cec->dev,
						     NULL, cec->adap);
	if (!cec->notify) {
		dev_err(cec->dev, "cec notify register failed!\n");
		return NULL;
	}

	ret = cec_register_adapter(cec->adap, cec->dev);
	if (ret < 0) {
		dev_err(cec->dev, "cec register adapter failed!\n");
		cec_notifier_cec_adap_unregister(cec->notify, cec->adap);
		return NULL;
	}

	/* The TV functionality can only map to physical address 0 */
	cec_s_phys_addr(cec->adap, 0, false);

	rk628_i2c_update_bits(rk628, HDMI_RX_DMI_DISABLE_IF, CEC_ENABLE_MASK, CEC_ENABLE_MASK);
	irqs = ERROR_INIT_ENSET | NACK_ENSET | EOM_ENSET | DONE_ENSET;
	rk628_i2c_write(rk628, HDMI_RX_AUD_CEC_IEN_SET, irqs);
	rk628_i2c_write(rk628, HDMI_RX_AUD_CEC_ICLR, ~0);

	/*
	 * CEC documentation says we must not call cec_delete_adapter
	 * after a successful call to cec_register_adapter().
	 */
	devm_remove_action(cec->dev, rk628_hdmirx_cec_del, cec);

	return cec;
}
EXPORT_SYMBOL(rk628_hdmirx_cec_register);

void rk628_hdmirx_cec_unregister(struct rk628_hdmirx_cec *cec)
{
	if (!cec)
		return;

	cec_notifier_cec_adap_unregister(cec->notify, cec->adap);
	cec_unregister_adapter(cec->adap);
}
EXPORT_SYMBOL(rk628_hdmirx_cec_unregister);

void rk628_hdmirx_cec_hpd(struct rk628_hdmirx_cec *cec, bool en)
{
	if (!cec || !cec->adap)
		return;

	//cec_queue_pin_hpd_event(cec->adap, en, ktime_get());
	printk("CEC: implicit declaration of function 'cec_queue_pin_hpd_event'\n");
}
EXPORT_SYMBOL(rk628_hdmirx_cec_hpd);

void rk628_hdmirx_cec_state_reconfiguration(struct rk628 *rk628,
					    struct rk628_hdmirx_cec *cec)
{
	unsigned int irqs;
	u32 val;

	rk628_i2c_write(rk628, HDMI_RX_CEC_ADDR_L, cec->addresses & 0xff);
	rk628_i2c_write(rk628, HDMI_RX_CEC_ADDR_H, (cec->addresses >> 8) & 0xff);

	rk628_i2c_write(rk628, HDMI_RX_CEC_MASK, 0);
	rk628_i2c_write(rk628, HDMI_RX_CEC_TX_CNT, 0);
	rk628_i2c_write(rk628, HDMI_RX_AUD_CEC_IEN_CLR, ~0);
	rk628_i2c_write(rk628, HDMI_RX_AUD_CEC_ICLR, ~0);
	rk628_i2c_write(rk628, HDMI_RX_CEC_CTRL, 0);
	rk628_i2c_write(rk628, HDMI_RX_CEC_LOCK, 0);

	irqs = ERROR_INIT_ENSET | NACK_ENSET | EOM_ENSET | DONE_ENSET;
	rk628_i2c_read(rk628, HDMI_RX_AUD_CEC_IEN, &val);
	if (!(val & irqs))
		rk628_i2c_write(rk628, HDMI_RX_AUD_CEC_IEN_SET, irqs);

	rk628_i2c_update_bits(rk628, HDMI_RX_DMI_DISABLE_IF, CEC_ENABLE_MASK, CEC_ENABLE(1));
}
EXPORT_SYMBOL(rk628_hdmirx_cec_state_reconfiguration);

void rk628_hdmirx_verisyno_phy_power_on(struct rk628 *rk628)
{
	bool is_hdmi2 = false;
	u32 val;
	int i;
	bool scramble = false;

	/* wait tx to write scdc tmds ratio */
	for (i = 0; i < 50; i++) {
		rk628_i2c_read(rk628, HDMI_RX_SCDC_REGS0, &val);
		if (val & SCDC_TMDSBITCLKRATIO)
			break;
		msleep(20);
	}

	if (val & SCDC_TMDSBITCLKRATIO)
		is_hdmi2 = true;

	rk628_i2c_read(rk628, HDMI_RX_HDMI20_STATUS, &val);
	scramble = (val & SCRAMBDET_MASK) ? true : false;

	dev_info(rk628->dev, "%s: %s, %s\n", __func__, is_hdmi2 ? "hdmi2.0" : "hdmi1.4",
		 scramble ? "Scramble" : "Descramble");
	/* power down phy */
	rk628_i2c_write(rk628, GRF_SW_HDMIRXPHY_CRTL, 0x17);
	usleep_range(20, 30);
	rk628_i2c_write(rk628, GRF_SW_HDMIRXPHY_CRTL, 0x15);
	/* init phy i2c */
	rk628_i2c_write(rk628, HDMI_RX_SNPS_PHYG3_CTRL, 0);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_SS_CNTS, 0x018c01d2);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_FS_HCNT, 0x003c0081);
	rk628_i2c_write(rk628, HDMI_RX_I2CM_PHYG3_MODE, 1);
	rk628_i2c_write(rk628, GRF_SW_HDMIRXPHY_CRTL, 0x11);
	/* enable rx phy */
	rk628_hdmirxphy_enable(rk628, is_hdmi2, scramble);
	rk628_i2c_write(rk628, GRF_SW_HDMIRXPHY_CRTL, 0x14);
	msleep(20);
}
EXPORT_SYMBOL(rk628_hdmirx_verisyno_phy_power_on);

void rk628_hdmirx_phy_prepclk_cfg(struct rk628 *rk628)
{
	u32 format;
	bool is_clrdpt_8bit = false;

	usleep_range(20 * 1000, 30 * 1000);
	rk628_i2c_read(rk628, HDMI_RX_PDEC_AVI_PB, &format);
	format = (format & VIDEO_FORMAT_MASK) >> 5;
	dev_info(rk628->dev, "%s: format = %d from AVI\n", __func__, format);

	/* yuv420 should set phy color depth 8bit */
	if (format == 3)
		is_clrdpt_8bit = true;

	rk628_i2c_read(rk628, HDMI_RX_PDEC_GCP_AVMUTE, &format);
	format = (format & PKTDEC_GCP_CD_MASK) >> 4;
	dev_info(rk628->dev, "%s: format = %d from GCP\n", __func__, format);

	/* 10bit color depth should set phy color depth 8bit */
	if (format == 5)
		is_clrdpt_8bit = true;

	rk628_hdmirxphy_set_clrdpt(rk628, is_clrdpt_8bit);
}
EXPORT_SYMBOL(rk628_hdmirx_phy_prepclk_cfg);

static const char * const bus_format_str[] = {
	"RGB",
	"YUV422",
	"YUV444",
	"YUV420",
	"UNKNOWN",
};

u8 rk628_hdmirx_get_format(struct rk628 *rk628)
{
	u32 val;
	u8 video_fmt;

	rk628_i2c_read(rk628, HDMI_RX_PDEC_AVI_PB, &val);
	video_fmt = (val & VIDEO_FORMAT_MASK) >> 5;
	if (video_fmt > BUS_FMT_UNKNOWN)
		video_fmt = BUS_FMT_UNKNOWN;
	dev_info(rk628->dev, "%s: format = %s\n", __func__, bus_format_str[video_fmt]);

	/*
	 * set avmute value to black
	 * RGB:    R: CH2[15:0], G:CH0_1[31:16], B: CH0_1[15:0]
	 * YUV:    Cr:CH2[15:0], Y:CH0_1[31:16], Cb:CH0_1[15:0]
	 */
	if (video_fmt == BUS_FMT_RGB) {
		rk628_i2c_write(rk628, HDMI_VM_CFG_CH0_1, 0x0);
		rk628_i2c_write(rk628, HDMI_VM_CFG_CH2, 0x0);
	} else {
		rk628_i2c_write(rk628, HDMI_VM_CFG_CH0_1, 0x00008000);
		rk628_i2c_write(rk628, HDMI_VM_CFG_CH2, 0x8000);
	}

	return video_fmt;
}
EXPORT_SYMBOL(rk628_hdmirx_get_format);

void rk628_set_bg_enable(struct rk628 *rk628, bool en)
{
	if (en) {
		if (rk628->tx_mode)
			rk628_i2c_write(rk628, GRF_BG_CTRL,
				BG_R_OR_V(0) | BG_B_OR_U(0) | BG_G_OR_Y(0) | BG_ENABLE(1));
		else
			rk628_i2c_write(rk628, GRF_BG_CTRL,
				BG_R_OR_V(512) | BG_B_OR_U(512) | BG_G_OR_Y(64) | BG_ENABLE(1));
		return;
	}
	rk628_i2c_write(rk628, GRF_BG_CTRL, BG_ENABLE(0));
}
EXPORT_SYMBOL(rk628_set_bg_enable);

u32 rk628_hdmirx_get_tmdsclk_cnt(struct rk628 *rk628)
{
	int i, j;
	u32 val, tmdsclk_cnt = 0;
	struct hdmirx_tmdsclk_cnt tmdsclk[HDMIRX_GET_TMDSCLK_TIME] = {0};

	for (i = 0; i < HDMIRX_GET_TMDSCLK_TIME; i++) {
		rk628_i2c_read(rk628, HDMI_RX_HDMI_CKM_RESULT, &val);
		tmdsclk_cnt = val & 0xffff;
		for (j = 0; j < HDMIRX_GET_TMDSCLK_TIME; j++) {
			if (tmdsclk_cnt == tmdsclk[j].tmds_cnt || !tmdsclk[j].tmds_cnt) {
				tmdsclk[j].tmds_cnt = tmdsclk_cnt;
				tmdsclk[j].cnt++;
				break;
			}
		}
	}

	for (i = 0; i < HDMIRX_GET_TMDSCLK_TIME; i++) {
		if (!tmdsclk[i].tmds_cnt)
			return tmdsclk_cnt;

		dev_info(rk628->dev, "tmdsclk_cnt: %d, cnt: %d\n",
			 tmdsclk[i].tmds_cnt, tmdsclk[i].cnt);
		if (!i)
			tmdsclk_cnt = tmdsclk[i].tmds_cnt;
		else if (tmdsclk[i].cnt > tmdsclk[i - 1].cnt)
			tmdsclk_cnt = tmdsclk[i].tmds_cnt;
	}

	return tmdsclk_cnt;
}
EXPORT_SYMBOL(rk628_hdmirx_get_tmdsclk_cnt);

static int rk628_hdmirx_read_timing(struct rk628 *rk628,
				    struct v4l2_dv_timings *timings)
{
	struct v4l2_bt_timings *bt = &timings->bt;
	u32 hact, vact, htotal, vtotal, fps, status;
	u32 val;
	u32 modetclk_cnt_hs, modetclk_cnt_vs, hs, vs;
	u32 hofs_pix, hbp, hfp, vbp, vfp;
	u32 tmds_clk, tmdsclk_cnt;
	u64 tmp_data;
	u8 video_fmt;

	memset(timings, 0, sizeof(struct v4l2_dv_timings));
	timings->type = V4L2_DV_BT_656_1120;
	rk628_i2c_read(rk628, HDMI_RX_SCDC_REGS1, &val);
	status = val;

	rk628_i2c_read(rk628, HDMI_RX_MD_STS, &val);
	bt->interlaced = val & ILACE_STS ?
		V4L2_DV_INTERLACED : V4L2_DV_PROGRESSIVE;

	rk628_i2c_read(rk628, HDMI_RX_MD_HACT_PX, &val);
	hact = val & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_VAL, &val);
	vact = val & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_HT1, &val);
	htotal = (val >> 16) & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_VTL, &val);
	vtotal = val & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_HT1, &val);
	hofs_pix = val & 0xffff;
	rk628_i2c_read(rk628, HDMI_RX_MD_VOL, &val);
	vbp = (val & 0xffff) + 1;

	tmdsclk_cnt = rk628_hdmirx_get_tmdsclk_cnt(rk628);
	tmp_data = tmdsclk_cnt;
	tmp_data = ((tmp_data * HDMIRX_MODETCLK_HZ) + HDMIRX_MODETCLK_CNT_NUM / 2);
	do_div(tmp_data, HDMIRX_MODETCLK_CNT_NUM);
	tmds_clk = tmp_data;
	if (!htotal || !vtotal || bt->interlaced || vtotal > 3000) {
		dev_err(rk628->dev, "timing err, %s htotal:%d, vtotal:%d\n",
			bt->interlaced ? "interlaced is not supported," : "",
			htotal, vtotal);
		goto TIMING_ERR;
	}
	if (rk628->version >= RK628F_VERSION)
		fps = tmds_clk  / (htotal * vtotal);
	else
		fps = (tmds_clk + (htotal * vtotal) / 2) / (htotal * vtotal);

	rk628_i2c_read(rk628, HDMI_RX_MD_HT0, &val);
	modetclk_cnt_hs = val & 0xffff;
	hs = (tmdsclk_cnt * modetclk_cnt_hs + HDMIRX_MODETCLK_CNT_NUM / 2) /
		HDMIRX_MODETCLK_CNT_NUM;

	rk628_i2c_read(rk628, HDMI_RX_MD_VSC, &val);
	modetclk_cnt_vs = val & 0xffff;
	vs = (tmdsclk_cnt * modetclk_cnt_vs + HDMIRX_MODETCLK_CNT_NUM / 2) /
		HDMIRX_MODETCLK_CNT_NUM;
	vs = (vs + htotal / 2) / htotal;

	if ((hofs_pix < hs) || (htotal < (hact + hofs_pix)) ||
			(vtotal < (vact + vs + vbp)) || !vs) {
		dev_err(rk628->dev, "timing err, total:%dx%d, act:%dx%d, hofs:%d, hs:%d, vs:%d, vbp:%d\n",
			htotal, vtotal, hact, vact, hofs_pix, hs, vs, vbp);
		goto TIMING_ERR;
	}
	hbp = hofs_pix - hs;
	hfp = htotal - hact - hofs_pix;
	vfp = vtotal - vact - vs - vbp;

	video_fmt = rk628_hdmirx_get_format(rk628);
	if (video_fmt == BUS_FMT_YUV420) {
		htotal *= 2;
		hact *= 2;
		hfp *= 2;
		hbp *= 2;
		hs *= 2;
	}

	dev_info(rk628->dev, "cnt_num:%d, tmds_cnt:%d, hs_cnt:%d, vs_cnt:%d, hofs:%d\n",
		 HDMIRX_MODETCLK_CNT_NUM, tmdsclk_cnt, modetclk_cnt_hs, modetclk_cnt_vs, hofs_pix);

	bt->width = hact;
	bt->height = vact;
	bt->hfrontporch = hfp;
	bt->hsync = hs;
	bt->hbackporch = hbp;
	bt->vfrontporch = vfp;
	bt->vsync = vs;
	bt->vbackporch = vbp;
	if (rk628->version >= RK628F_VERSION)
		bt->pixelclock = tmds_clk;
	else
		bt->pixelclock = htotal * vtotal * fps;

	if (bt->interlaced == V4L2_DV_INTERLACED) {
		bt->height *= 2;
		bt->il_vsync = bt->vsync + 1;
		bt->pixelclock /= 2;
	}
	if (video_fmt == BUS_FMT_YUV420)
		bt->pixelclock *= 2;

	if (vact == 1080 && vtotal > 1500)
		goto TIMING_ERR;

	dev_info(rk628->dev, "SCDC_REGS1:%#x, act:%dx%d, total:%dx%d, fps:%d, pixclk:%llu\n",
		 status, hact, vact, htotal, vtotal, fps, bt->pixelclock);

	return 0;

TIMING_ERR:
	return -ENOLCK;
}

bool rk628_hdmirx_tx_5v_power_detect(struct gpio_config *det_gpio)
{
	bool ret;
	int val, i, cnt;

	/* Direct Mode */
	if (!gpio_is_valid(det_gpio->gpio)) {
		sensor_print("%s: det_gpio->gpio is invalid!, direct mode!\n", __func__);
		return true;
	}

	cnt = 0;
	for (i = 0; i < 5; i++) {
		val = gpio_get_value(det_gpio->gpio);
		if (val <= 0)	// hdmirx_det gpio inverted
			cnt++;
		usleep_range(500, 600);
	}

	ret = (cnt >= 3) ? true : false;

	// sensor_dbg("rk628_hdmirx_tx_5v_power_detect return %s\n", ret ? "true" : "false");

	return ret;
}
EXPORT_SYMBOL(rk628_hdmirx_tx_5v_power_detect);

static int rk628_hdmirx_try_to_get_timing(struct rk628 *rk628,
					  struct v4l2_dv_timings *timings)
{
	int ret, i;

	for (i = 0; i < 5; i++) {
		ret = rk628_hdmirx_read_timing(rk628, timings);
		if (!ret)
			return ret;
		msleep(20);
	}

	return ret;
}

int rk628_hdmirx_get_timings(struct rk628 *rk628,
			     struct v4l2_dv_timings *timings)
{
	int i, cnt = 0, ret = 0;
	u32 last_w, last_h;
	u32 val;
	u8 last_fmt;
	struct v4l2_bt_timings *bt = &timings->bt;

	last_w = 0;
	last_h = 0;
	last_fmt = BUS_FMT_RGB;

	for (i = 0; i < HDMIRX_GET_TIMING_CNT; i++) {
		if (!rk628_hdmirx_tx_5v_power_detect(&rk628->hdmirx_det_gpio)) {
			dev_info(rk628->dev, "%s: hdmi plug out!\n", __func__);
			return -EINVAL;
		}

		ret = rk628_hdmirx_try_to_get_timing(rk628, timings);
		if ((last_w == 0) && (last_h == 0)) {
			last_w = bt->width;
			last_h = bt->height;
			last_fmt = rk628_hdmirx_get_format(rk628);
		}

		if (ret && i > 2)
			return -EINVAL;

		if (ret || (last_w != bt->width) || (last_h != bt->height)
		    || (last_fmt != rk628_hdmirx_get_format(rk628)))
			cnt = 0;
		else
			cnt++;

		if (cnt >= 8)
			break;

		last_w = bt->width;
		last_h = bt->height;
		last_fmt = rk628_hdmirx_get_format(rk628);
		usleep_range(10*1000, 10*1100);
	}

	if (cnt < 8) {
		dev_info(rk628->dev, "%s: res not stable!\n", __func__);
		ret = -EINVAL;
	}

	if (rk628->version >= RK628F_VERSION) {
		val = DIV_ROUND_CLOSEST_ULL(1188000000, bt->pixelclock);
		val *= bt->pixelclock;
		if (val > 1188000000) {
			/* set pll rate according hdmirx tmds clk */
			rk628_clk_set_rate(rk628, CGU_CLK_CPLL, val);
			dev_dbg(rk628->dev, "set CPLL to %d\n", val);
			msleep(50);
		}
	}

	return ret;
}
EXPORT_SYMBOL(rk628_hdmirx_get_timings);

u8 rk628_hdmirx_get_range(struct rk628 *rk628)
{
	u32 val;
	u8 color_range;

	rk628_i2c_read(rk628, HDMI_RX_PDEC_AVI_PB, &val);
	color_range = (val & RGB_COLORRANGE_MASK) >> 18;
	if (color_range == 0x1)
		color_range = CSC_LIMIT_RANGE;
	else
		color_range = CSC_FULL_RANGE;

	return color_range;
}
EXPORT_SYMBOL(rk628_hdmirx_get_range);

void rk628_hdmirx_controller_reset(struct rk628 *rk628)
{
	mutex_lock(&rk628->rst_lock);
	rk628_control_assert(rk628, RGU_HDMIRX);
	rk628_control_assert(rk628, RGU_HDMIRX_PON);
	udelay(10);
	rk628_control_deassert(rk628, RGU_HDMIRX);
	rk628_control_deassert(rk628, RGU_HDMIRX_PON);
	udelay(10);
	rk628_i2c_write(rk628, HDMI_RX_DMI_SW_RST, 0x000101ff);
	rk628_i2c_write(rk628, HDMI_RX_DMI_DISABLE_IF, 0x00000000);
	rk628_i2c_write(rk628, HDMI_RX_DMI_DISABLE_IF, 0x0000017f);
	rk628_i2c_write(rk628, HDMI_RX_DMI_DISABLE_IF, 0x0001017f);
	mutex_unlock(&rk628->rst_lock);
}
EXPORT_SYMBOL(rk628_hdmirx_controller_reset);

bool rk628_hdmirx_scdc_ced_err(struct rk628 *rk628)
{
	u32 val, val1;

	if (rk628->version < RK628F_VERSION)
		return false;

	rk628_i2c_read(rk628, HDMI_RX_SCDC_REGS1, &val);
	rk628_i2c_read(rk628, HDMI_RX_SCDC_REGS2, &val1);
	if (((val >> 15) & SCDC_ERRDET_MASK) < SCDC_CED_ERR_CNT &&
	    ((val1 >> 15) & SCDC_ERRDET_MASK) < SCDC_CED_ERR_CNT &&
	    (val1 & SCDC_ERRDET_MASK) < SCDC_CED_ERR_CNT)
		return false;

	dev_info(rk628->dev, "%s: Character Error(0x%x  0x%x)!\n", __func__, val, val1);
	return true;
}
EXPORT_SYMBOL(rk628_hdmirx_scdc_ced_err);

bool rk628_hdmirx_is_signal_change_ists(struct rk628 *rk628)
{
	u32 md_ints, pdec_ints;
	u32 md_mask, pded_madk;

	md_mask = VACT_LIN_ISTS | HACT_PIX_ISTS |
		  HS_CLK_ISTS | DE_ACTIVITY_ISTS |
		  VS_ACT_ISTS | HS_ACT_ISTS | VS_CLK_ISTS;
	rk628_i2c_read(rk628, HDMI_RX_MD_ISTS, &md_ints);
	if (md_ints & md_mask)
		return true;

	pded_madk = AVI_CKS_CHG_ISTS;
	rk628_i2c_read(rk628, HDMI_RX_PDEC_ISTS, &pdec_ints);
	if (pdec_ints & pded_madk)
		return true;

	return false;
}
EXPORT_SYMBOL(rk628_hdmirx_is_signal_change_ists);

static int rk628_hdmirx_phy_reg_show(struct seq_file *s, void *v)
{
	struct rk628 *rk628 = s->private;
	unsigned int i;

	seq_printf(s, "rk628_%s:\n", file_dentry(s->file)->d_iname);

	for (i = 0; i <= 0xb7; i++)
		seq_printf(s, "0x%02x: %08x\n", i, hdmirxphy_read(rk628, i));

	return 0;
}

static ssize_t rk628_hdmirx_phy_reg_write(struct file *file, const char __user *buf,
			       size_t count, loff_t *ppos)
{
	struct rk628 *rk628 = file->f_path.dentry->d_inode->i_private;
	u32 addr;
	u32 val;
	char kbuf[25];
	int ret;

	if (count >= sizeof(kbuf))
		return -ENOSPC;

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;

	kbuf[count] = '\0';

	ret = sscanf(kbuf, "%x%x", &addr, &val);
	if (ret != 2)
		return -EINVAL;

	if (addr > 0xb7)
		return -EINVAL;

	hdmirxphy_write(rk628, addr, val);

	return count;
}

static int rk628_hdmirx_phy_reg_open(struct inode *inode, struct file *file)
{
	struct rk628 *rk628 = inode->i_private;

	return single_open(file, rk628_hdmirx_phy_reg_show, rk628);
}

static const struct file_operations rk628_hdmirx_phy_reg_fops = {
	.owner          = THIS_MODULE,
	.open           = rk628_hdmirx_phy_reg_open,
	.read           = seq_read,
	.write          = rk628_hdmirx_phy_reg_write,
	.llseek         = seq_lseek,
	.release        = single_release,
};
void rk628_hdmirx_phy_debugfs_register_create(struct rk628 *rk628, struct dentry *dir)
{
	if (rk628->version < RK628F_VERSION)
		return;
	if (IS_ERR(dir))
		return;

	debugfs_create_file("hdmirxphy", 0600, dir, rk628, &rk628_hdmirx_phy_reg_fops);
}
EXPORT_SYMBOL(rk628_hdmirx_phy_debugfs_register_create);

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
#include <linux/version.h>
#include <linux/dma-buf.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <video/sunxi_metadata.h>
#include <linux/regulator/consumer.h>

#include "aw_hdmi_core.h"

#define AW_EDID_MAX_SIZE 		(1024)
u8   gEdid_test_buff[AW_EDID_MAX_SIZE];
bool bEdid_test_mode;

struct aw_hdmi_core_s *g_aw_core;
dw_hdmi_dev_t *g_dw_dev;

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
DEFINE_MUTEX(cec_la_lock);
#endif

static struct disp_hdmi_mode aw_hdmi_mode_tbl[] = {
	{DISP_TV_MOD_480I,                HDMI_720x480I60_16x9,      },
	{DISP_TV_MOD_576I,                HDMI_720x576I50_16x9,      },
	{DISP_TV_MOD_480P,                HDMI_720x480P60_16x9,      },
	{DISP_TV_MOD_576P,                HDMI_720x576P50_16x9,      },
	{DISP_TV_MOD_720P_50HZ,           HDMI_1280x720P50_16x9,     },
	{DISP_TV_MOD_720P_60HZ,           HDMI_1280x720P60_16x9,     },
	{DISP_TV_MOD_1080I_50HZ,          HDMI_1920x1080I50_16x9,    },
	{DISP_TV_MOD_1080I_60HZ,          HDMI_1920x1080I60_16x9,    },
	{DISP_TV_MOD_1080P_24HZ,          HDMI_1920x1080P24_16x9,    },
	{DISP_TV_MOD_1080P_50HZ,          HDMI_1920x1080P50_16x9,    },
	{DISP_TV_MOD_1080P_60HZ,          HDMI_1920x1080P60_16x9,    },
	{DISP_TV_MOD_1080P_25HZ,          HDMI_1920x1080P25_16x9,    },
	{DISP_TV_MOD_1080P_30HZ,          HDMI_1920x1080P30_16x9,    },
	{DISP_TV_MOD_1080P_24HZ_3D_FP,    HDMI_1080P_24_3D_FP,       },
	{DISP_TV_MOD_720P_50HZ_3D_FP,     HDMI_720P_50_3D_FP,        },
	{DISP_TV_MOD_720P_60HZ_3D_FP,     HDMI_720P_60_3D_FP,        },
	{DISP_TV_MOD_3840_2160P_30HZ,     HDMI_3840x2160P30_16x9,    },
	{DISP_TV_MOD_3840_2160P_25HZ,     HDMI_3840x2160P25_16x9,    },
	{DISP_TV_MOD_3840_2160P_24HZ,     HDMI_3840x2160P24_16x9,    },
	{DISP_TV_MOD_4096_2160P_24HZ,     HDMI_4096x2160P24_256x135, },
	{DISP_TV_MOD_4096_2160P_25HZ,	  HDMI_4096x2160P25_256x135, },
	{DISP_TV_MOD_4096_2160P_30HZ,	  HDMI_4096x2160P30_256x135, },

	{DISP_TV_MOD_3840_2160P_50HZ,     HDMI_3840x2160P50_16x9,    },
	{DISP_TV_MOD_4096_2160P_50HZ,     HDMI_4096x2160P50_256x135, },
	{DISP_TV_MOD_3840_2160P_60HZ,     HDMI_3840x2160P60_16x9,    },
	{DISP_TV_MOD_4096_2160P_60HZ,     HDMI_4096x2160P60_256x135, },

	{DISP_TV_MOD_2560_1440P_60HZ,     HDMI_2560x1440P60,         },
	{DISP_TV_MOD_1440_2560P_70HZ,     HDMI_1440x2560P70,         },
	{DISP_TV_MOD_1080_1920P_60HZ,     HDMI_1080x1920P60,         },
};

static struct disp_hdmi_mode aw_hdmi_mode_tbl_4_3[] = {
	{DISP_TV_MOD_480I,                HDMI_720x480I60_4x3, },
	{DISP_TV_MOD_576I,                HDMI_720x576I50_4x3, },
	{DISP_TV_MOD_480P,                HDMI_720x480P60_4x3, },
	{DISP_TV_MOD_576P,                HDMI_720x576P50_4x3, },
};

static struct aw_sink_blacklist sink_blacklist[] = {
	{ { {0x36, 0x74}, {0x4d, 0x53, 0x74, 0x61, 0x72, 0x20, 0x44, 0x65, 0x6d, 0x6f, 0x0a, 0x20, 0x20}, 0x38},        /* sink */
	{ {DISP_TV_MOD_3840_2160P_30HZ, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },  /* issue */

	{ { {0x2d, 0xee}, {0x4b, 0x4f, 0x4e, 0x41, 0x4b, 0x20, 0x54, 0x56, 0x0a, 0x20, 0x20, 0x20, 0x20}, 0xa5},
	{ {DISP_TV_MOD_3840_2160P_30HZ, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },

	{ { {0x2d, 0xe1}, {0x54, 0x56, 0x5f, 0x4d, 0x4f, 0x4e, 0x49, 0x54, 0x4f, 0x52, 0x0a, 0x20, 0x20}, 0x28},
	{ {DISP_TV_MOD_1080P_24HZ_3D_FP, 0x00}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },

	{ { {0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0},
	{ {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} } },
};

static int _phy_core_write(u16 addr, u32 data)
{
#if defined(SUNXI_HDMI20_PHY_AW)
	aw_phy_write((u8)addr, data);
#elif defined(SUNXI_HDMI20_PHY_INNO)
	inno_phy_write((u8)addr, (u8)data);
#elif defined(SUNXI_HDMI20_PHY_SNPS)
	snps_phy_write(g_dw_dev, (u8)addr, (u16)data);
#elif defined(SUNXI_HDMI20_PHY_INNO_FPGA)
	inno_phy_fpga_write(addr, (u8)data);
#else
	hdmi_err("%s: not config phy model!!!\n", __func__);
	return -1;
#endif
	return 0;
}

static int _phy_core_read(u16 addr, u32 *value)
{
#if defined(SUNXI_HDMI20_PHY_AW)
	aw_phy_read((u8)addr, value);
#elif defined(SUNXI_HDMI20_PHY_INNO)
	inno_phy_read((u8)addr, (u8 *)value);
#elif defined(SUNXI_HDMI20_PHY_SNPS)
	snps_phy_read(g_dw_dev, (u8)addr, (u16 *)value);
#elif defined(SUNXI_HDMI20_PHY_INNO_FPGA)
	inno_phy_fpga_read(addr, value);
#else
	hdmi_err("%s: not config phy model!!!\n", __func__);
	return -1;
#endif
	return 0;
}

static int _phy_core_config(void)
{
#if defined(SUNXI_HDMI20_PHY_SNPS)
	u16 phy_model = g_aw_core->hdmi_tx_phy;
	return snps_phy_configure(g_dw_dev, phy_model);
#endif

#if defined(SUNXI_HDMI20_PHY_INNO)
	return inno_phy_configure(g_dw_dev);
#endif

#if defined(SUNXI_HDMI20_PHY_AW)
	dw_color_format_t format = g_aw_core->mode.pVideo.mEncodingOut;
	return aw_phy_configure(g_dw_dev, format);
#endif

#if defined(SUNXI_HDMI20_PHY_INNO_FPGA)
	return inno_phy_fpga_configure(g_dw_dev);
#endif

	hdmi_err("%s: not config phy model!!!\n", __func__);
	return 0;
}

static void _phy_core_reset(void)
{
#if defined(SUNXI_HDMI20_PHY_AW)
	aw_phy_reset();
#else
	hdmi_err("%s: not config phy model!!!\n", __func__);
#endif
}

static int _phy_core_resume(void)
{
#if defined(SUNXI_HDMI20_PHY_AW)
	return aw_phy_config_resume();
#else
	hdmi_err("%s: not config phy model!!!\n", __func__);
#endif
	return 0;
}

static void _phy_core_set_power(u8 enable)
{
	return dw_phy_power_enable(g_dw_dev, enable);
}

static u32 _phy_core_get_power(void)
{
	return dw_phy_power_state(g_dw_dev);
}

static u32 _phy_core_get_pll(void)
{
	return dw_phy_pll_lock_state(g_dw_dev);
}

static u32 _phy_core_get_rxsense(void)
{
	return dw_phy_rxsense_state(g_dw_dev);
}

static void _phy_core_set_hpd(u8 enable)
{
	if (enable) {
		ctrl_i2cm_ddc_fast_mode(g_dw_dev, 0);
		dw_phy_enable_hpd_sense(g_dw_dev);
	} else {
		dw_phy_disable_hpd_sense(g_dw_dev);
	}
}

static u8 _phy_core_get_hpd(void)
{
	return dw_phy_hot_plug_state(g_dw_dev);
}

static u32 _audio_core_get_acr_n(void)
{
	return dw_audio_get_clock_n(g_dw_dev);
}

static u32 _audio_core_get_layout(void)
{
	return dw_fc_audio_get_packet_layout(g_dw_dev);
}

static u32 _audio_core_get_sample_freq(void)
{
	return dw_fc_audio_get_sample_freq(g_dw_dev);
}

static u32 _audio_core_get_sample_size(void)
{
	return dw_fc_audio_get_word_length(g_dw_dev);
}

static u32 _audio_core_get_channel_count(void)
{
	return dw_fc_audio_get_channel_count(g_dw_dev);
}

static void _audio_core_print_info(dw_audio_param_t *audio)
{
	AUDIO_INF("Audio interface type = %s\n",
			audio->mInterfaceType == DW_AUDIO_INTERFACE_I2S ? "I2S" :
			audio->mInterfaceType == DW_AUDIO_INTERFACE_SPDIF ? "SPDIF" :
			audio->mInterfaceType == DW_AUDIO_INTERFACE_HBR ? "HBR" :
			audio->mInterfaceType == DW_AUDIO_INTERFACE_GPA ? "GPA" :
			audio->mInterfaceType == DW_AUDIO_INTERFACE_DMA ? "DMA" : "---");

	AUDIO_INF("Audio coding = %s\n", audio->mCodingType == DW_AUD_CODING_PCM ? "PCM" :
		audio->mCodingType == DW_AUD_CODING_AC3 ? "AC3" :
		audio->mCodingType == DW_AUD_CODING_MPEG1 ? "MPEG1" :
		audio->mCodingType == DW_AUD_CODING_MP3 ? "MP3" :
		audio->mCodingType == DW_AUD_CODING_MPEG2 ? "MPEG2" :
		audio->mCodingType == DW_AUD_CODING_AAC ? "AAC" :
		audio->mCodingType == DW_AUD_CODING_DTS ? "DTS" :
		audio->mCodingType == DW_AUD_CODING_ATRAC ? "ATRAC" :
		audio->mCodingType == DW_AUD_CODING_ONE_BIT_AUDIO ? "ONE BIT AUDIO" :
		audio->mCodingType == DW_AUD_CODING_DOLBY_DIGITAL_PLUS ? "DOLBY DIGITAL +" :
		audio->mCodingType == DW_AUD_CODING_DTS_HD ? "DTS HD" :
		audio->mCodingType == DW_AUD_CODING_MAT ? "MAT" : "---");
	AUDIO_INF("Audio frequency = %dHz\n", audio->mSamplingFrequency);
	AUDIO_INF("Audio sample size = %d\n", audio->mSampleSize);
	AUDIO_INF("Audio FS factor = %d\n", audio->mClockFsFactor);
	AUDIO_INF("Audio ChannelAllocationr = %d\n", audio->mChannelAllocation);
	AUDIO_INF("Audio mChannelNum = %d\n", audio->mChannelNum);
}

static int _audio_core_config(void)
{
	int success = true;
	dw_hdmi_ctrl_t *tx_ctrl = &g_dw_dev->snps_hdmi_ctrl;
	u32 tmds_clk = 0;
	dw_tmds_mode_t hdmi_on;
	dw_video_param_t *video = &g_aw_core->mode.pVideo;
	dw_audio_param_t *audio = &g_aw_core->mode.pAudio;

	_audio_core_print_info(audio);

	hdmi_on = video->mHdmi;
	tx_ctrl->hdmi_on  = (hdmi_on == DW_TMDS_MODE_HDMI) ? 1 : 0;
	tx_ctrl->audio_on = (hdmi_on == DW_TMDS_MODE_HDMI) ? 1 : 0;
	tx_ctrl->pixel_clock = dw_vp_get_pixel_clk(g_dw_dev, video);
	tx_ctrl->color_resolution = video->mColorResolution;
	tx_ctrl->pixel_repetition = video->mDtd.mPixelRepetitionInput;

	switch (video->mColorResolution) {
	case DW_COLOR_DEPTH_8:
		tmds_clk = tx_ctrl->pixel_clock;
		break;
	case DW_COLOR_DEPTH_10:
		if (video->mEncodingOut != DW_COLOR_FORMAT_YCC422)
			tmds_clk = tx_ctrl->pixel_clock * 125 / 100;
		else
			tmds_clk = tx_ctrl->pixel_clock;

		break;
	case DW_COLOR_DEPTH_12:
		if (video->mEncodingOut != DW_COLOR_FORMAT_YCC422)
			tmds_clk =  tx_ctrl->pixel_clock * 3 / 2;
		else
			tmds_clk =  tx_ctrl->pixel_clock;

		break;
	default:
		hdmi_err("%s: Do not support mColorResolution %d\n", __func__, video->mColorResolution);
		break;
	}
	tx_ctrl->tmds_clk = tmds_clk;
	/* Audio - Workaround */
	dw_audio_initialize(g_dw_dev);
	success = dw_audio_config(g_dw_dev, audio);
	if (success == false)
		hdmi_err("%s: audio configure failed!\n", __func__);
	dw_mc_audio_sampler_clock_enable(g_dw_dev,
				      g_dw_dev->snps_hdmi_ctrl.audio_on ? 0 : 1);
	dw_fc_audio_force(g_dw_dev, 0);

	return success;
}

void _video_core_set_avmute(u8 enable)
{
	dw_gcp_set_avmute(g_dw_dev, enable);
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	dw_hdcp_set_avmute_state(g_dw_dev, enable);
#endif
}

u32 _video_core_get_avmute(void)
{
	return dw_gcp_get_avmute(g_dw_dev);
}

void _video_core_set_tmds_mode(u8 enable)
{
	return dw_fc_video_set_tmds_mode(g_dw_dev, !enable);
}

u32 _video_core_get_tmds_mode(void)
{
	return dw_fc_video_get_tmds_mode(g_dw_dev);
}

s32 _video_core_get_color_format(void)
{
	return g_aw_core->mode.pVideo.mEncodingIn;
}

u8 _video_core_get_color_range(void)
{
	return g_aw_core->mode.pVideo.mRgbQuantizationRange;
}

u32 _video_core_get_color_metry(void)
{
	return dw_avi_get_colori_metry(g_dw_dev);
}

u32 _video_core_get_color_space(void)
{
	return dw_avi_get_rgb_ycc(g_dw_dev);
}

u32 _video_core_get_color_depth(void)
{
	return dw_vp_get_color_depth(g_dw_dev);
}

u32 _video_core_get_scramble(void)
{
	return dw_fc_video_get_scramble(g_dw_dev);
}

u32 _video_core_get_pixel_repetion(void)
{
	return dw_vp_get_pixel_repet_factor(g_dw_dev);
}

u32 _video_core_get_vic_code(void)
{
	return dw_avi_get_video_code(g_dw_dev);
}

void _video_core_drm_up(dw_fc_drm_pb_t *pb)
{
	dw_drm_packet_up(g_dw_dev, pb);
}

void _video_core_print_info(dw_video_param_t *pVideo)
{
	u32 refresh_rate = dw_dtd_get_rate(&pVideo->mDtd);

	if (pVideo->mCea_code)
		VIDEO_INF("[HDMI2.0]CEA VIC=%d\n", pVideo->mCea_code);
	else
		VIDEO_INF("[HDMI2.0]HDMI VIC=%d\n", pVideo->mHdmi_code);
	if (pVideo->mDtd.mInterlaced)
		VIDEO_INF("%dx%di", pVideo->mDtd.mHActive,
						pVideo->mDtd.mVActive*2);
	else
		VIDEO_INF("%dx%dp", pVideo->mDtd.mHActive,
						pVideo->mDtd.mVActive);
	VIDEO_INF("@%d fps\n", refresh_rate/1000);
	VIDEO_INF("%d:%d, ", pVideo->mDtd.mHImageSize,
					pVideo->mDtd.mVImageSize);
	VIDEO_INF("%d-bpp\n", pVideo->mColorResolution);
	VIDEO_INF("%s\n", dw_vp_get_color_format_string(pVideo->mEncodingIn));
	switch (pVideo->mColorimetry) {
	case 0:
		hdmi_wrn("you haven't set an colorimetry\n");
		break;
	case DW_METRY_ITU601:
		VIDEO_INF("BT601\n");
		break;
	case DW_METRY_ITU709:
		VIDEO_INF("BT709\n");
		break;
	case DW_METRY_EXTENDED:
		if (pVideo->mExtColorimetry == DW_METRY_EXT_BT2020_Y_CB_CR)
			VIDEO_INF("BT2020_Y_CB_CR\n");
		else
			VIDEO_INF("extended color space standard %d undefined\n", pVideo->mExtColorimetry);
		break;
	default:
		VIDEO_INF("color space standard %d undefined\n", pVideo->mColorimetry);
	}
	if (pVideo->pb == NULL)
		return;

	switch (pVideo->pb->eotf) {
	case DW_EOTF_SDR:
		VIDEO_INF("eotf:SDR_LUMINANCE_RANGE\n");
		break;
	case DW_EOTF_HDR:
		VIDEO_INF("eotf:HDR_LUMINANCE_RANGE\n");
		break;
	case DW_EOTF_SMPTE2084:
		VIDEO_INF("eotf:SMPTE_ST_2084\n");
		break;
	case DW_EOTF_HLG:
		VIDEO_INF("eotf:HLG\n");
		break;
	default:
		VIDEO_INF("eotf:undefined\n");
		break;
	}
}

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
static void _hdcp_core_close(void)
{
	dw_hdcp_main_close(g_dw_dev);
}

static void _hdcp_core_configure(void)
{
	dw_hdcp_param_t  *hdcp  = &g_aw_core->mode.pHdcp;
	dw_video_param_t *video = &g_aw_core->mode.pVideo;

	g_dw_dev->snps_hdmi_ctrl.hdcp_on = 1;
	dw_hdcp_main_config(g_dw_dev, hdcp, video);
}

static void _hdcp_core_disconfigure(void)
{
	g_dw_dev->snps_hdmi_ctrl.hdcp_on = 0;
	dw_hdcp_main_disconfig(g_dw_dev);
}

static int _hdcp_core_get_type(void)
{
	return (int)g_dw_dev->snps_hdmi_ctrl.hdcp_type;
}

static int _hdcp_core_get_status(void)
{
	return dw_hdcp_get_state(g_dw_dev);
}

ssize_t _hdcp_core_config_dump(char *buf)
{
	return dw_hdcp_config_dump(g_dw_dev, buf);
}

static u32 _hdcp_core_get_avmute(void)
{
	return dw_hdcp_get_avmute_state(g_dw_dev);
}

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
static u32 _hdcp22_core_get_sink_support(void)
{
	return dw_hdcp22_get_sink_support(g_dw_dev);
}
#endif
#endif /* CONFIG_AW_HDMI2_HDCP_SUNXI */

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)

int _cec_core_enable(void)
{
	g_dw_dev->snps_hdmi_ctrl.cec_on = true;

	dw_mc_cec_clock_enable(g_dw_dev, 0x0);
	udelay(200);
	dw_cec_enable(g_dw_dev);
	return 0;
}

int _cec_core_disable(void)
{
	dw_cec_disable(g_dw_dev);
	udelay(200);
	dw_mc_cec_clock_enable(g_dw_dev, 0x1);

	g_dw_dev->snps_hdmi_ctrl.cec_on = false;
	return 0;
}

static s32 _cec_core_receive(unsigned char *msg, unsigned *size)
{
	return dw_cec_receive_frame(g_dw_dev, msg, size);
}

static s32 _cec_core_send(unsigned char *msg, unsigned size, unsigned frame_type)
{
	u32 dw_frame_type = 0;

	switch (frame_type) {
	case AW_HDMI_CEC_FRAME_TYPE_RETRY:
		dw_frame_type = CEC_CTRL_FRAME_TYP_RETRY;
		break;
	case AW_HDMI_CEC_FRAME_TYPE_NORMAL:
	default:
		dw_frame_type = CEC_CTRL_FRAME_TYP_NORMAL;
		break;
	case AW_HDMI_CEC_FRAME_TYPE_IMMED:
		dw_frame_type = CEC_CTRL_FRAME_TYP_IMMED;
		break;
	}

	return dw_cec_send_frame(g_dw_dev, msg, size, frame_type);
}

static u16 _cec_core_get_physic_addr(void)
{
	u16 addr = 0x1000;
	if (g_aw_core == NULL) {
		hdmi_err("%s: point g_aw_core is null!!!\n", __func__);
		return addr;
	}

	if (g_aw_core->mode.sink_cap && g_aw_core->mode.edid_done) {
		addr = g_aw_core->mode.sink_cap->edid_mHdmivsdb.mPhysicalAddress;
	}

	return addr;
}

static int _cec_core_set_logical_addr(unsigned int addr)
{
	if (g_aw_core == NULL) {
		hdmi_err("%s: param g_aw_core is null!!!\n", __func__);
		return -1;
	}

	mutex_lock(&cec_la_lock);
	dw_cec_set_logical_addr(g_dw_dev, addr);
	mutex_unlock(&cec_la_lock);

	return 0;
}

static u16 _cec_core_get_logical_addr(void)
{
	u16 logical = 0;

	mutex_lock(&cec_la_lock);
	logical = (unsigned char)dw_cec_get_logical_addr(g_dw_dev);
	mutex_unlock(&cec_la_lock);

	return logical;
}

static int _cec_core_interrupt_get_state(void)
{
	u32 dw_state = dw_cec_interrupt_get_state(g_dw_dev);
	u32 state = 0;

	if (dw_state & IH_CEC_STAT0_DONE_MASK)
		state |= AW_HDMI_CEC_STAT_DONE;
	if (dw_state & IH_CEC_STAT0_EOM_MASK)
		state |= AW_HDMI_CEC_STAT_EOM;
	if (dw_state & IH_CEC_STAT0_NACK_MASK)
		state |= AW_HDMI_CEC_STAT_NACK;
	if (dw_state & IH_CEC_STAT0_ARB_LOST_MASK)
		state |= AW_HDMI_CEC_STAT_ARBLOST;
	if (dw_state & IH_CEC_STAT0_ERROR_INITIATOR_MASK)
		state |= AW_HDMI_CEC_STAT_ERROR_INIT;
	if (dw_state & IH_CEC_STAT0_ERROR_FOLLOW_MASK)
		state |= AW_HDMI_CEC_STAT_ERROR_FOLL;
	if (dw_state & IH_CEC_STAT0_WAKEUP_MASK)
		state |= AW_HDMI_CEC_STAT_WAKEUP;

	return state;
}

static void _cec_core_interrupt_clear_state(unsigned state)
{
	u32 dw_state = 0;

	if (state & AW_HDMI_CEC_STAT_DONE)
		dw_state |= IH_CEC_STAT0_DONE_MASK;
	if (state & AW_HDMI_CEC_STAT_EOM)
		dw_state |= IH_CEC_STAT0_EOM_MASK;
	if (state & AW_HDMI_CEC_STAT_NACK)
		dw_state |= IH_CEC_STAT0_NACK_MASK;
	if (state & AW_HDMI_CEC_STAT_ARBLOST)
		dw_state |= IH_CEC_STAT0_ARB_LOST_MASK;
	if (state & AW_HDMI_CEC_STAT_ERROR_INIT)
		dw_state |= IH_CEC_STAT0_ERROR_INITIATOR_MASK;
	if (state & AW_HDMI_CEC_STAT_ERROR_FOLL)
		dw_state |= IH_CEC_STAT0_ERROR_FOLLOW_MASK;
	if (state & AW_HDMI_CEC_STAT_WAKEUP)
		dw_state |= IH_CEC_STAT0_WAKEUP_MASK;

	dw_cec_interrupt_clear_state(g_dw_dev, dw_state);
}

#endif /* CONFIG_AW_HDMI2_CEC_SUNXI */

/* Check if the sink pluged in is in sink blacklist
* @sink_edid: point to the sink's edid block 0
* return: true: indicate the sink is in blacklist
*         false: indicate the sink is NOT in blacklist
*/
int _edid_core_check_in_blacklist(u8 *sink_edid)
{
	u32 i, j;

	for (i = 0; sink_blacklist[i].sink.checksum != 0; i++) {
		for (j = 0; j < 2; j++) {
			if (sink_blacklist[i].sink.mft_id[j] != sink_edid[8 + j])
				break;
		}
		if (j < 2)
			continue;

		for (j = 0; j < 13; j++) {
			if (sink_blacklist[i].sink.stib[j] != sink_edid[95 + j])
				break;
		}
		if (j < 13)
			continue;

		if (sink_blacklist[i].sink.checksum != sink_edid[127])
			return -1;
		else
			return i;
	}

	return -1;
}

static int _edid_sink_supports_vic_code(u32 vic_code)
{
	int i = 0;
	dw_dtd_t dtd;
	sink_edid_t *edid_exten = NULL;
	struct detailed_timing *dt_ext = NULL;
	struct edid *edid_block0 = NULL;
	struct detailed_timing *dt = NULL;

	edid_exten = g_aw_core->mode.sink_cap;
	if (edid_exten == NULL) {
		hdmi_err("%s: param edid_exten is null!!!\n", __func__);
		return false;
	}

	edid_block0 = g_aw_core->mode.edid;
	if (edid_block0 == NULL) {
		hdmi_err("%s: param edid_block0 is null!!!\n", __func__);
		return false;
	}

	dt_ext = edid_exten->detailed_timings;
	if (dt_ext == NULL) {
		hdmi_err("%s: param dt_ext is null!!!\n", __func__);
		return false;
	}

	for (i = 0; (i < edid_exten->edid_mSvdIndex) && (edid_exten->edid_mSvd[i].mCode != 0); i++) {
		if (edid_exten->edid_mSvd[i].mCode == vic_code)
			return true;
	}

	for (i = 0; (i < edid_exten->edid_mHdmivsdb.mHdmiVicCount) &&
			(edid_exten->edid_mHdmivsdb.mHdmiVic[i] != 0); i++) {
		if (edid_exten->edid_mHdmivsdb.mHdmiVic[i] ==
				dw_vp_get_hdmi_vic(vic_code))
			return true;
	}

	if ((vic_code == 0x80 + 19) || (vic_code == 0x80 + 4)
		|| (vic_code == 0x80 + 32)) {
		if (edid_exten->edid_mHdmivsdb.m3dPresent == 1) {
			VIDEO_INF("Support basic 3d video format\n");
			return true;
		} else {
			hdmi_wrn("Do not support any basic 3d video format\n");
			return false;
		}
	}

	if (!dw_dtd_fill(g_dw_dev, &dtd, vic_code, 0)) {
		hdmi_err("%s: Get detailed timing failed for vic_code:%d\n", __func__, vic_code);
		return false;
	}

	if (!edid_block0)
		return false;
	dt = edid_block0->detailed_timings;
	if (dt == NULL) {
		hdmi_err("%s: edid block0 dtd pointer is null!!!\n", __func__);
		return false;
	}

	/* Check EDID Block0 detailed timing block */
	for (i = 0; i < 2; i++) {
		if ((dtd.mPixelClock * (dtd.mPixelRepetitionInput + 1) == dt[i].pixel_clock * 10)
			&& (dtd.mHActive == ((((dt[i].data.pixel_data.hactive_hblank_hi >> 4) & 0x0f) << 8)
			| dt[i].data.pixel_data.hactive_lo))
			&& ((dtd.mVActive / (dtd.mInterlaced + 1)) == ((((dt[i].data.pixel_data.vactive_vblank_hi >> 4) & 0x0f) << 8)
			| dt[i].data.pixel_data.vactive_lo))) {
			hdmi_inf("detailed timing block support\n");
			return true;
		}
	}

	/* Check EDID Extension Block detailed timing block */
	for (i = 0; i < 2; i++) {
		if ((dtd.mPixelClock * (dtd.mPixelRepetitionInput + 1) == dt_ext[i].pixel_clock * 10)
			&& (dtd.mHActive == ((((dt_ext[i].data.pixel_data.hactive_hblank_hi >> 4) & 0x0f) << 8)
			| dt_ext[i].data.pixel_data.hactive_lo))
			&& ((dtd.mVActive / (dtd.mInterlaced + 1)) == ((((dt_ext[i].data.pixel_data.vactive_vblank_hi >> 4) & 0x0f) << 8)
			| dt_ext[i].data.pixel_data.vactive_lo))) {
			hdmi_inf("detailed timing block support\n");
			return true;
		}
	}

	return false;
}

static bool _edid_check_cea_vic_support(u32 cea_vic)
{
	struct hdmi_mode *mode = &g_aw_core->mode;
	bool support = false;
	u32 i;

	if (cea_vic == 0) {
		hdmi_err("%s: invalid cea_vic code:%d\n", __func__, cea_vic);
		goto exit;
	}

	if ((mode == NULL) || (mode->sink_cap == NULL)) {
		hdmi_err("%s: Has NULL struct\n", __func__);
		goto exit;
	}
	for (i = 0; (i < 128) && (mode->sink_cap->edid_mSvd[i].mCode != 0); i++) {
		if (mode->sink_cap->edid_mSvd[i].mCode == cea_vic)
			support = true;
	}

exit:
	return support;
}

static bool _edid_check_hdmi_vic_support(u32 hdmi_vic)
{
	struct hdmi_mode *mode = &g_aw_core->mode;
	bool support = false;
	u32 i;

	if (hdmi_vic == 0) {
		hdmi_err("%s: invalid cea_vic code:%d\n", __func__, hdmi_vic);
		goto exit;
	}

	if ((mode == NULL) || (mode->sink_cap == NULL)) {
		hdmi_err("%s: Has NULL struct\n", __func__);
		goto exit;
	}
	for (i = 0; (i < 4) && (mode->sink_cap->edid_mHdmivsdb.mHdmiVic[i] != 0); i++) {
		if (mode->sink_cap->edid_mHdmivsdb.mHdmiVic[i] == hdmi_vic)
			support = true;
	}

exit:
	return support;
}

static void _edid_set_video_prefered(sink_edid_t *sink_cap, dw_video_param_t *pVideo)
{
	/* Configure using Native SVD or HDMI_VIC */
	int i = 0;
	int hdmi_vic = 0;
	bool cea_vic_support = false;
	bool hdmi_vic_support = false;
	unsigned int vic_index = 0;

	LOG_TRACE();
	if ((sink_cap == NULL) || (pVideo == NULL)) {
		hdmi_err("%s: Have NULL params\n", __func__);
		return;
	}

	if (!g_aw_core->mode.edid_done) {
		pVideo->mCea_code = pVideo->mDtd.mCode;
		return;
	}

	if ((pVideo->mDtd.mCode == 0x80 + 19) ||
		(pVideo->mDtd.mCode == 0x80 + 4) ||
		(pVideo->mDtd.mCode == 0x80 + 32)) {
		pVideo->mCea_code = pVideo->mDtd.mCode - 0x80;
		pVideo->mHdmi_code = 0;
		return;
	}

	if ((pVideo->mDtd.mCode == 0x201)
		|| (pVideo->mDtd.mCode == 0x202)
		|| (pVideo->mDtd.mCode == 0x203)) {
		pVideo->mCea_code = 0;
		pVideo->mHdmi_code = 0;
		return;
	}

	pVideo->mHdmi20 = 0;
	pVideo->mHdmi20 = sink_cap->edid_m20Sink;

	/* parse if this vic in sink support yuv420 */
	for (i = 0; (i < 128) && (sink_cap->edid_mSvd[i].mCode != 0); i++) {
		if (sink_cap->edid_mSvd[i].mCode == pVideo->mDtd.mCode) {
			vic_index = i;
			break;
		}
	}

	dw_vp_set_support_ycc420(&pVideo->mDtd,
					&sink_cap->edid_mSvd[vic_index]);

	if (dw_vp_get_hdmi_vic(pVideo->mDtd.mCode) > 0)
		hdmi_vic_support = _edid_check_hdmi_vic_support(dw_vp_get_hdmi_vic(pVideo->mDtd.mCode));

	if (hdmi_vic_support)
		VIDEO_INF("Sink Support hdmi vic mode:%d\n",
					dw_vp_get_hdmi_vic(pVideo->mDtd.mCode));

	cea_vic_support = _edid_check_cea_vic_support(pVideo->mDtd.mCode);
	if (cea_vic_support)
		VIDEO_INF("Sink Support cea vic mode:%d\n",
						pVideo->mDtd.mCode);
	else
		VIDEO_INF("Sink do NOT Support cea vic mode:%d\n",
						pVideo->mDtd.mCode);

	if (hdmi_vic_support) {
		pVideo->mCea_code = 0;
		hdmi_vic = dw_vp_get_hdmi_vic(pVideo->mDtd.mCode);
		pVideo->mHdmi_code = hdmi_vic;
		g_aw_core->mode.pProduct.mOUI = 0x000c03;
		g_aw_core->mode.pProduct.mVendorPayload[0] = 0x20;
		g_aw_core->mode.pProduct.mVendorPayload[1] = hdmi_vic;
		g_aw_core->mode.pProduct.mVendorPayload[2] = 0;
		g_aw_core->mode.pProduct.mVendorPayload[3] = 0;
		g_aw_core->mode.pProduct.mVendorPayloadLength = 4;
	} else if (cea_vic_support) {
		pVideo->mCea_code = pVideo->mDtd.mCode;
		pVideo->mHdmi_code = 0;
		if (((pVideo->mCea_code == 96)
			|| (pVideo->mCea_code == 97)
			|| (pVideo->mCea_code == 101)
			|| (pVideo->mCea_code == 102))
			&& pVideo->mHdmi20) {
			g_aw_core->mode.pProduct.mOUI = 0xc45dd8;
			g_aw_core->mode.pProduct.mVendorPayload[0] = 0x1;
			g_aw_core->mode.pProduct.mVendorPayload[1] = 0;
			g_aw_core->mode.pProduct.mVendorPayload[2] = 0;
			g_aw_core->mode.pProduct.mVendorPayload[3] = 0;
			g_aw_core->mode.pProduct.mVendorPayloadLength = 4;
		} else {
			g_aw_core->mode.pProduct.mOUI = 0x000c03;
			g_aw_core->mode.pProduct.mVendorPayload[0] = 0x0;
			g_aw_core->mode.pProduct.mVendorPayload[1] = 0;
			g_aw_core->mode.pProduct.mVendorPayload[2] = 0;
			g_aw_core->mode.pProduct.mVendorPayload[3] = 0;
			g_aw_core->mode.pProduct.mVendorPayloadLength = 4;

		}
	} else {
		pVideo->mCea_code = pVideo->mDtd.mCode;
		hdmi_wrn("sink do not support this mode: %d\n",
			pVideo->mDtd.mCode);
	}

	pVideo->scdc_ability = sink_cap->edid_mHdmiForumvsdb.mSCDC_Present;
}

void _edid_core_set_test_mode(bool state)
{
	bEdid_test_mode = state;
}

bool _edid_core_get_test_mode(void)
{
	return bEdid_test_mode;
}

static void _edid_core_set_test_data(const unsigned char *data, unsigned int size)
{
	if (!data) {
		hdmi_err("%s: param data is null!!!\n", __func__);
		return;
	}

	if (size > AW_EDID_MAX_SIZE) {
		hdmi_err("%s: invalid edid size input\n", __func__);
		return;
	}

	memset(gEdid_test_buff, 0, AW_EDID_MAX_SIZE);
	memcpy(gEdid_test_buff, data, size);
	_edid_core_set_test_mode(true);
}

static void _edid_core_edid_read(void)
{
	struct edid  *edid = NULL;
	u8  *edid_ext = NULL;
	sink_edid_t *sink = NULL;
	int edid_ok = 0;
	int edid_tries = 3;
	int edid_ext_cnt = 0;

	LOG_TRACE();
	if (g_aw_core == NULL) {
		hdmi_err("%s: could not get g_aw_core structure!!!\n", __func__);
		return;
	}

	g_aw_core->mode.sink_cap = kzalloc(sizeof(sink_edid_t), GFP_KERNEL);
	if (!g_aw_core->mode.sink_cap) {
		hdmi_err("%s: could not allocated edid sink!\n", __func__);
		return;
	}
	memset(g_aw_core->mode.sink_cap, 0, sizeof(sink_edid_t));

	sink = g_aw_core->mode.sink_cap;
	if (sink == NULL) {
		hdmi_err("%s: sink point is null!!!\n", __func__);
		return;
	}

	g_aw_core->mode.edid = kzalloc(sizeof(struct edid), GFP_KERNEL);
	edid = g_aw_core->mode.edid;
	if (!edid) {
		hdmi_err("%s: kzalloc for edid failed\n", __func__);
		return;
	}

	g_aw_core->blacklist_sink = -1;

	mutex_lock(&g_aw_core->hdmi_tx.i2c_lock);
	memset(sink, 0, sizeof(sink_edid_t));
	dw_edid_reset(g_dw_dev, sink);

	do {
		if (bEdid_test_mode) {
			memcpy(edid, gEdid_test_buff, 128);
		} else {
			edid_ok = dw_edid_read_base_block(g_dw_dev, edid);
			if (edid_ok == -1) /* error case */
				continue;
		}

		g_aw_core->mode.edid = edid;
		if (dw_edid_parser(g_dw_dev, (u8 *)edid, sink) == false) {
			hdmi_err("%s: parse edid failed, error edid block0\n", __func__);
			g_aw_core->mode.edid_done = 0;
			mutex_unlock(&g_aw_core->hdmi_tx.i2c_lock);
			return;
		}

		g_aw_core->blacklist_sink = _edid_core_check_in_blacklist((u8 *)edid);
		if (g_aw_core->blacklist_sink >= 0)
			hdmi_wrn("this sink is in blacklist: %d\n", g_aw_core->blacklist_sink);

		break;
	} while (edid_tries--);

	if (edid_tries <= 0) {
		hdmi_err("%s: could not read devices edid!\n", __func__);
		g_aw_core->mode.edid_done = 0;
		mutex_unlock(&g_aw_core->hdmi_tx.i2c_lock);
		return;
	}

	if (edid->extensions == 0) {
		g_aw_core->mode.edid_done = 1;
	} else {
		int edid_ext_total_cnt = 0;

		/* (1)<----read and parse the 1st extension block data */
		u8 temp_ext[128];

		EDID_INF("read and parse edid extension block 1\n");

		edid_tries = 3;
		do {
			if (bEdid_test_mode) {
				memcpy(temp_ext, gEdid_test_buff + 128, 128);
			} else {
				edid_ok = dw_edid_read_extenal_block(g_dw_dev, 0x1, temp_ext);
				if (edid_ok)
					continue;
			}

			g_aw_core->mode.edid_done = 1;
			if (!dw_edid_parser(g_dw_dev, temp_ext, sink)) {
				hdmi_err("%s: could not parse edid extension block 1\n", __func__);
				g_aw_core->mode.edid_done = 0;
			}
			break;
		} while (edid_tries--);
		/* end of read and parse the 1st extension block data---> */

		/* (2)<---decide total edid extension block count */
		if (sink->hf_eeodb_block_count) {
			edid_ext_total_cnt = sink->hf_eeodb_block_count;
			EDID_INF("has HF-EEODB, hf_eeodb_block_count:%d\n",
						sink->hf_eeodb_block_count);
		} else
			edid_ext_total_cnt = edid->extensions;
		EDID_INF("Sink edid has %d extension block count\n",
						edid_ext_total_cnt);
		/* end of decide total edid extension block count---> */


		/* (3)<---allocate memory to store edid extension block data */
		edid_ext = kzalloc(128 * edid_ext_total_cnt, GFP_KERNEL);
		if (!edid_ext) {
			mutex_unlock(&g_aw_core->hdmi_tx.i2c_lock);
			hdmi_err("%s: kzalloc for edid extension failed\n", __func__);
			return;
		}
		g_aw_core->mode.edid_ext = edid_ext;
		/* end of allocate memory---> */

		/* (4)<---copy the 1st extension block data to g_aw_core->mode.edid_ext */
		memcpy(edid_ext, temp_ext, 128);
		/* end of copy---> */

		/* (5)<---read the other extension blocks(>=2) and parse hf-eeodb */
		edid_ext_cnt = 2;
		while (edid_ext_cnt <= edid_ext_total_cnt) {
			EDID_INF("read edid extension block %d\n", edid_ext_cnt);
			edid_tries = 3;
			do {
				/* (5-1)reading extension block data */
				if (bEdid_test_mode) {
					memcpy(edid_ext + (edid_ext_cnt - 1) * 128,
						gEdid_test_buff + edid_ext_cnt * 128,
						128);
				} else {
					edid_ok = dw_edid_read_extenal_block(g_dw_dev, edid_ext_cnt,
							(edid_ext + (edid_ext_cnt - 1) * 128));
					if (edid_ok == -1)
						continue;
				}

				/* (5-2)<---parse the HF-EEODB */
				if (sink->hf_eeodb_block_count >= edid_ext_cnt) {
					u8 *eeodb, tag, length;

					eeodb = &edid_ext[(edid_ext_cnt - 1) * 0x80 + 4];
					tag = eeodb[0] >> 5 & 0x7;
					length = eeodb[0] & 0x1f;
					if (tag == 0x2) {
						int i, j;

						for (i = 1; i <= length; i++) {
							for (j = 0; j < sink->edid_mSvdIndex; j++) {
								if (sink->edid_mSvd[i].mCode == eeodb[i])
									break;
							}

							if (j == sink->edid_mSvdIndex) {
								sink->edid_mSvd[sink->edid_mSvdIndex].mCode
									= eeodb[i];
								sink->edid_mSvdIndex++;
								EDID_INF("HF-EEODB, block:%d has VIC:%d\n",
										edid_ext_cnt, eeodb[i]);
							}
						}
					} else {
						EDID_INF("block:%d has NOT valid video data block\n",
															edid_ext_cnt);
					}
				}

				break;
			} while (edid_tries--);
			edid_ext_cnt++;
		}
		/* end of read the other extension blocks--> */
	}

	mutex_unlock(&g_aw_core->hdmi_tx.i2c_lock);
}

static void _edid_core_release(void)
{
	if (g_aw_core->mode.sink_cap) {
		kfree(g_aw_core->mode.sink_cap);
		g_aw_core->mode.sink_cap = NULL;
	}

	if (g_aw_core->mode.edid) {
		kfree(g_aw_core->mode.edid);
		g_aw_core->mode.edid = NULL;
	}

	if (g_aw_core->mode.edid_ext) {
		kfree(g_aw_core->mode.edid_ext);
		g_aw_core->mode.edid_ext = NULL;
	}
}

/*
* mainly for correcting hardware config after booting
* reason: as we do not read edid during booting,
* some configs that depended on edid
* can not be sure, so we have to correct them after
* booting and reading edid
*/
static void _edid_core_correct_hardware_config(void)
{
	u8 vsif_data[2];
	int cea_code = 0;
	int hdmi_code = 0;

	if (!g_aw_core->mode.edid_done) {
		hdmi_wrn("%s: edid read not over yet!\n", __func__);
		return;
	}

	/* correct vic setting */
	dw_vsif_get_hdmi_vic(g_dw_dev, vsif_data);
	if ((vsif_data[0]) >> 5 == 0x1) {/* To check if there is sending hdmi_vic */
		if (_edid_check_hdmi_vic_support(vsif_data[1]))
			return;
		cea_code = dw_vp_get_cea_vic(vsif_data[1]);
		if (_edid_check_cea_vic_support(cea_code)) {
			vsif_data[0] = 0x0;
			vsif_data[1] = 0x0;
			dw_vsif_set_hdmi_vic(g_dw_dev, vsif_data);
			dw_avi_set_video_code(g_dw_dev, cea_code);
		}
	} else if ((vsif_data[0] >> 5) == 0x0) {/* To check if there is sending cea_vic */
		cea_code = dw_avi_get_video_code(g_dw_dev);
		hdmi_code = dw_vp_get_hdmi_vic(cea_code);
		if (hdmi_code > 0) {
			vsif_data[0] = 0x1 << 5;
			vsif_data[1] = hdmi_code;
			if (_edid_check_hdmi_vic_support(cea_code)) {
				dw_avi_set_video_code(g_dw_dev, 0x0);
				dw_vsif_set_hdmi_vic(g_dw_dev, vsif_data);
			}
		}
	}
}

static void _edid_core_set_video_prefered(void)
{
	if (g_aw_core->mode.sink_cap == NULL) {
		hdmi_err("%s: sink cap is NULL!\n", __func__);
		return ;
	}

	_edid_set_video_prefered(g_aw_core->mode.sink_cap, &g_aw_core->mode.pVideo);
}
#ifndef SUPPORT_ONLY_HDMI14
static int _aw_core_scdc_read(u8 address, u8 size, u8 *data)
{
	return dw_scdc_read(g_dw_dev, address, size, data);
}

static int _aw_core_scdc_write(u8 address, u8 size, u8 *data)
{
	return dw_scdc_write(g_dw_dev, address, size, data);
}
#endif

static void _aw_core_update_ctrl_param(dw_video_param_t *video, dw_hdcp_param_t *hdcp)
{
	dw_tmds_mode_t hdmi_on = 0;
	dw_hdmi_ctrl_t *tx_ctrl = &g_dw_dev->snps_hdmi_ctrl;

	hdmi_on = video->mHdmi;
	tx_ctrl->hdmi_on = (hdmi_on == DW_TMDS_MODE_HDMI) ? 1 : 0;
	tx_ctrl->hdcp_on = hdcp->hdcp_on;
	tx_ctrl->audio_on = (hdmi_on == DW_TMDS_MODE_HDMI) ? 1 : 0;
	tx_ctrl->use_hdcp = hdcp->use_hdcp;
	tx_ctrl->use_hdcp22 = hdcp->use_hdcp22;
	tx_ctrl->pixel_clock = dw_vp_get_pixel_clk(g_dw_dev, video);
	tx_ctrl->color_resolution = video->mColorResolution;
	tx_ctrl->pixel_repetition = video->mDtd.mPixelRepetitionInput;
}

static int _aw_core_main_standby(void)
{
	dw_phy_standby(g_dw_dev);
	dw_mc_all_clock_standby(g_dw_dev);

	g_dw_dev->snps_hdmi_ctrl.hdmi_on = 1;
	g_dw_dev->snps_hdmi_ctrl.pixel_clock = 0;
	g_dw_dev->snps_hdmi_ctrl.color_resolution = 0;
	g_dw_dev->snps_hdmi_ctrl.pixel_repetition = 0;
	g_dw_dev->snps_hdmi_ctrl.audio_on = 1;

	return true;
}

static int _aw_core_main_close(void)
{
	dw_phy_standby(g_dw_dev);
	dw_mc_all_clock_disable(g_dw_dev);

	g_dw_dev->snps_hdmi_ctrl.hdmi_on = 1;
	g_dw_dev->snps_hdmi_ctrl.pixel_clock = 0;
	g_dw_dev->snps_hdmi_ctrl.color_resolution = 0;
	g_dw_dev->snps_hdmi_ctrl.pixel_repetition = 0;
	g_dw_dev->snps_hdmi_ctrl.audio_on = 1;

	return true;
}

static int _aw_core_main_config(void)
{
	int success = true;
	unsigned int tmds_clk = 0;

	dw_video_param_t   *video   = &g_aw_core->mode.pVideo;
	dw_audio_param_t   *audio   = &g_aw_core->mode.pAudio;
	dw_hdcp_param_t    *hdcp    = &g_aw_core->mode.pHdcp;
	dw_product_param_t *product = &g_aw_core->mode.pProduct;

	LOG_TRACE();

	_video_core_print_info(video);
	_audio_core_print_info(audio);

	_aw_core_update_ctrl_param(video, hdcp);

	switch (video->mColorResolution) {
	case DW_COLOR_DEPTH_8:
		tmds_clk = g_dw_dev->snps_hdmi_ctrl.pixel_clock;
		break;
	case DW_COLOR_DEPTH_10:
		if (video->mEncodingOut != DW_COLOR_FORMAT_YCC422)
			tmds_clk = g_dw_dev->snps_hdmi_ctrl.pixel_clock * 125 / 100;
		else
			tmds_clk = g_dw_dev->snps_hdmi_ctrl.pixel_clock;

		break;
	case DW_COLOR_DEPTH_12:
		if (video->mEncodingOut != DW_COLOR_FORMAT_YCC422)
			tmds_clk = g_dw_dev->snps_hdmi_ctrl.pixel_clock * 3 / 2;
		else
			tmds_clk = g_dw_dev->snps_hdmi_ctrl.pixel_clock;

		break;

	default:
		hdmi_err("%s: invalid color depth %d\n", __func__, video->mColorResolution);
		break;
	}
	g_dw_dev->snps_hdmi_ctrl.tmds_clk = tmds_clk;

	if (video->mEncodingIn == DW_COLOR_FORMAT_YCC420) {
		g_dw_dev->snps_hdmi_ctrl.pixel_clock = g_dw_dev->snps_hdmi_ctrl.pixel_clock / 2;
		g_dw_dev->snps_hdmi_ctrl.tmds_clk /= 2;
	}
	if (video->mEncodingIn == DW_COLOR_FORMAT_YCC422)
		g_dw_dev->snps_hdmi_ctrl.color_resolution = 8;

	_video_core_set_avmute(true);

	dw_phy_standby(g_dw_dev);

	/* Disable interrupts */
	dw_mc_irq_all_mute(g_dw_dev);

	success = dw_video_config(g_dw_dev, video);
	if (success == false)
		hdmi_err("%s: video configure is failed!!!\n", __func__);

	/* Audio - Workaround */
	dw_audio_initialize(g_dw_dev);
	success = dw_audio_config(g_dw_dev, audio);
	if (success == false)
		hdmi_err("%s: audio configure is failed!!!\n", __func__);

	/* Packets */
	success = dw_fc_packet_config(g_dw_dev, video, product);
	if (success == false)
		hdmi_err("%s: packets configure is failed!!!\n", __func__);

	dw_mc_all_clock_enable(g_dw_dev);
	mdelay(10);

#ifndef SUPPORT_ONLY_HDMI14
	if ((g_dw_dev->snps_hdmi_ctrl.tmds_clk  > 340000)
		/* && (video->scdc_ability) */) {
		dw_video_set_scramble(g_dw_dev, 1);
		VIDEO_INF("config scramble enable.\n");
	} else if (video->scdc_ability || dw_fc_video_get_scramble(g_dw_dev)) {
		dw_video_set_scramble(g_dw_dev, 0);
		VIDEO_INF("config scramble disable.\n");
	}
#endif

	success = _phy_core_config();
	if (success == false)
		hdmi_err("%s: phy configure is failed!!!\n", __func__);

	/* Disable blue screen transmission
	after turning on all necessary blocks (e.g. HDCP) */
	dw_fc_force_output(g_dw_dev, false);
	dw_mc_irq_mask_all(g_dw_dev);

	mdelay(100);

	/* enable interrupts */
	dw_mc_irq_all_unmute(g_dw_dev);
	_video_core_set_avmute(false);

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	dw_hdcp_config_init(g_dw_dev);
	if (hdcp->use_hdcp && hdcp->hdcp_on)
		dw_hdcp_main_config(g_dw_dev, hdcp, video);
#endif

	dw_fc_iteration_process(g_dw_dev);

	return success;
}

/**
 * @code: svd <1-107, 0x80+4, 0x80+19, 0x80+32, 0x201>
 * @refresh_rate: [optional] Hz*1000,which is 1000 times than 1 Hz
 */
static u32 _aw_core_svd_user_config(u32 vic_code, u32 refresh_rate)
{
	dw_dtd_t dtd;

	if (g_aw_core == NULL) {
		hdmi_err("%s: hdmi g_aw_core structure is null!!!\n", __func__);
		return -1;
	}

	if (vic_code < 1 || vic_code > 0x300) {
		hdmi_err("%s: vic code is out of range! %d\n", __func__, vic_code);
		return -1;
	}

	if (refresh_rate == 0) {
		dtd.mCode = vic_code;
		/* Ensure Pixel clock is 0 to get the default refresh rate */
		dtd.mPixelClock = 0;
		VIDEO_INF("HDMI_WARN:Code %d with default refresh rate %d\n",
					vic_code, dw_dtd_get_rate(&dtd)/1000);
	} else {
		VIDEO_INF("HDMI_WARN:Code %d with refresh rate %d\n",
				vic_code, dw_dtd_get_rate(&dtd)/1000);
	}

	if (dw_dtd_fill(g_dw_dev, &dtd, vic_code, refresh_rate) == false) {
		hdmi_err("%s: Can not find detailed timing\n", __func__);
		return -1;
	}

	memcpy(&g_aw_core->mode.pVideo.mDtd, &dtd, sizeof(dw_dtd_t));
	g_aw_core->edid_ops.set_prefered_video();

	if ((vic_code == 0x80 + 19) || (vic_code == 0x80 + 4) || (vic_code == 0x80 + 32)) {
		g_aw_core->mode.pVideo.mHdmiVideoFormat = 0x02;
		g_aw_core->mode.pVideo.m3dStructure = 0;
	} else if (g_aw_core->mode.pVideo.mHdmi_code) {
		g_aw_core->mode.pVideo.mHdmiVideoFormat = 0x01;
		g_aw_core->mode.pVideo.m3dStructure = 0;
	} else {
		g_aw_core->mode.pVideo.mHdmiVideoFormat = 0x0;
		g_aw_core->mode.pVideo.m3dStructure = 0;
	}

	return 0;
}

s32 _aw_core_get_current_tv_mode(void)
{
	u32 i;
	s32 tv_mode = 0;
	for (i = 0; i < sizeof(aw_hdmi_mode_tbl)/sizeof(struct disp_hdmi_mode); i++) {
		if (aw_hdmi_mode_tbl[i].hdmi_mode == g_aw_core->mode.pVideo.mDtd.mCode) {
			tv_mode = aw_hdmi_mode_tbl[i].mode;
			break;
		}
	}
	return tv_mode;
}

s32 _aw_core_update_tv_mode_dtd(u32 mode)
{
	u32 vic_code;
	u32 i;
	bool find = false;

	VIDEO_INF("hdmi set display mode: %d\n", mode);

	for (i = 0; i < sizeof(aw_hdmi_mode_tbl)/sizeof(struct disp_hdmi_mode); i++) {
		if (aw_hdmi_mode_tbl[i].mode == (enum disp_tv_mode)mode) {
			vic_code = aw_hdmi_mode_tbl[i].hdmi_mode;
			find = true;
			break;
		}
	}

	if (find) {
		/* user configure detailed timing according to vic */
		if (_aw_core_svd_user_config(vic_code, 0) != 0) {
			hdmi_err("%s: svd user config failed!\n", __func__);
			return -1;
		} else {
			VIDEO_INF("Set hdmi vic code: %d\n", vic_code);
			return 0;
		}
	} else {
		hdmi_err("%s: unsupported video mode %d when set display mode\n", __func__, mode);
		return -1;
	}

}

s32 _aw_core_check_tv_mode(u32 mode)
{
	u32 hdmi_mode;
	u32 i;
	int ret = 0;

	if (g_aw_core->blacklist_sink >= 0) {
		for (i = 0; i < 10; i++) {
			if (sink_blacklist[g_aw_core->blacklist_sink].issue[i].tv_mode == mode) {
				ret = sink_blacklist[g_aw_core->blacklist_sink].issue[i].issue_type << 1;
				EDID_INF("check tv mode %d is in blacklist\n", mode);
				return ret;
			}
		}
	}

	for (i = 0; i < sizeof(aw_hdmi_mode_tbl)/sizeof(struct disp_hdmi_mode); i++) {
		if (aw_hdmi_mode_tbl[i].mode == (enum disp_tv_mode)mode) {
			hdmi_mode = aw_hdmi_mode_tbl[i].hdmi_mode;
			if (_edid_sink_supports_vic_code(hdmi_mode) == true) {
				EDID_INF("check tv mode(%d) is in edid vic code(%d), table1\n", mode, hdmi_mode);
				return 1;
			}
		}
	}

	for (i = 0; i < sizeof(aw_hdmi_mode_tbl_4_3)/
		sizeof(struct disp_hdmi_mode); i++) {
		if (aw_hdmi_mode_tbl_4_3[i].mode == (enum disp_tv_mode)mode) {
			hdmi_mode = aw_hdmi_mode_tbl_4_3[i].hdmi_mode;
			if (_edid_sink_supports_vic_code(hdmi_mode) == true) {
				EDID_INF("check tv mode(%d) is in edid vic code(%d). table2\n", mode, hdmi_mode);
				return 1;
			}
		}
	}

	return 0;
}

int _aw_core_get_blacklist_issue(u32 mode)
{
	u32 i;
	u8 issue = 0;

	if (g_aw_core->blacklist_sink < 0)
		return 0;

	for (i = 0; i < 10; i++) {
		if (sink_blacklist[g_aw_core->blacklist_sink].issue[i].tv_mode == mode)
			issue = sink_blacklist[g_aw_core->blacklist_sink].issue[i].issue_type;
	}

	return issue;
}

s32 _aw_core_smooth_enable(void)
{
	dw_video_param_t *pvideo = &g_aw_core->mode.pVideo;
	u32 update = pvideo->update;

	if (update & DISP_CONFIG_UPDATE_EOTF) {
		if ((pvideo->pb != NULL) && (pvideo->pb->eotf == DW_EOTF_SDR)) {
			dw_drm_packet_clear(pvideo->pb);
			dw_drm_packet_up(g_dw_dev, pvideo->pb);
			msleep(2000);
			dw_drm_packet_disabled(g_dw_dev);
		} else {
			dw_drm_packet_up(g_dw_dev, pvideo->pb);
		}
	}

	if (update & DISP_CONFIG_UPDATE_CS)
		dw_avi_set_colori_metry(g_dw_dev, pvideo->mColorimetry, pvideo->mExtColorimetry);

	if (update & DISP_CONFIG_UPDATE_RANGE)
		dw_avi_set_quantization_range(g_dw_dev, pvideo->mRgbQuantizationRange);

	if (update & DISP_CONFIG_UPDATE_SCAN)
		dw_avi_set_scan_info(g_dw_dev, pvideo->mScanInfo);

	if (update & DISP_CONFIG_UPDATE_RATIO)
		dw_avi_set_aspect_ratio(g_dw_dev, pvideo->mActiveFormatAspectRatio);

	return 0;
}

int _aw_core_init_ops_device(void)
{
	g_aw_core->dev_ops.dev_smooth_enable         = _aw_core_smooth_enable;
	g_aw_core->dev_ops.dev_tv_mode_check         = _aw_core_check_tv_mode;
	g_aw_core->dev_ops.dev_tv_mode_get           = _aw_core_get_current_tv_mode;
	g_aw_core->dev_ops.dev_tv_mode_update_dtd    = _aw_core_update_tv_mode_dtd;
	g_aw_core->dev_ops.dev_config                = _aw_core_main_config;
	g_aw_core->dev_ops.dev_standby               = _aw_core_main_standby;
	g_aw_core->dev_ops.dev_close                 = _aw_core_main_close;
	g_aw_core->dev_ops.dev_get_blacklist_issue   = _aw_core_get_blacklist_issue;

#ifndef SUPPORT_ONLY_HDMI14
	g_aw_core->dev_ops.scdc_write         = _aw_core_scdc_write;
	g_aw_core->dev_ops.scdc_read          = _aw_core_scdc_read;
#endif

	return 0;
}

int _aw_core_init_ops_video(void)
{
	g_aw_core->video_ops.set_drm_up          = _video_core_drm_up;
	g_aw_core->video_ops.get_vic_code        = _video_core_get_vic_code;
	g_aw_core->video_ops.get_avmute          = _video_core_get_avmute;
	g_aw_core->video_ops.set_avmute          = _video_core_set_avmute;
	g_aw_core->video_ops.get_scramble        = _video_core_get_scramble;
	g_aw_core->video_ops.set_tmds_mode       = _video_core_set_tmds_mode;
	g_aw_core->video_ops.get_tmds_mode       = _video_core_get_tmds_mode;
	g_aw_core->video_ops.get_color_depth     = _video_core_get_color_depth;
	g_aw_core->video_ops.get_color_format    = _video_core_get_color_format;
	g_aw_core->video_ops.get_color_metry     = _video_core_get_color_metry;
	g_aw_core->video_ops.get_color_range     = _video_core_get_color_range;
	g_aw_core->video_ops.get_color_space     = _video_core_get_color_space;
	g_aw_core->video_ops.get_pixel_repetion  = _video_core_get_pixel_repetion;
	return 0;
}

int _aw_core_init_ops_audio(void)
{
	g_aw_core->audio_ops.get_acr_n          = _audio_core_get_acr_n;
	g_aw_core->audio_ops.get_layout         = _audio_core_get_layout;
	g_aw_core->audio_ops.get_sample_freq    = _audio_core_get_sample_freq;
	g_aw_core->audio_ops.get_sample_size    = _audio_core_get_sample_size;
	g_aw_core->audio_ops.get_channel_count  = _audio_core_get_channel_count;
	g_aw_core->audio_ops.audio_config       = _audio_core_config;
	return 0;
}

int _aw_core_init_ops_hdcp(void)
{
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	g_aw_core->hdcp_ops.hdcp_close         = _hdcp_core_close;
	g_aw_core->hdcp_ops.hdcp_configure     = _hdcp_core_configure;
	g_aw_core->hdcp_ops.hdcp_disconfigure  = _hdcp_core_disconfigure;
	g_aw_core->hdcp_ops.hdcp_get_type      = _hdcp_core_get_type;
	g_aw_core->hdcp_ops.hdcp_get_status    = _hdcp_core_get_status;
	g_aw_core->hdcp_ops.hdcp_get_avmute    = _hdcp_core_get_avmute;
	g_aw_core->hdcp_ops.hdcp_config_dump   = _hdcp_core_config_dump;
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	g_aw_core->hdcp_ops.hdcp22_get_support = _hdcp22_core_get_sink_support;
#endif
#endif
	return 0;
}

int _aw_core_init_ops_phy(void)
{
	g_aw_core->phy_ops.phy_write         = _phy_core_write;
	g_aw_core->phy_ops.phy_read          = _phy_core_read;
	g_aw_core->phy_ops.phy_get_rxsense   = _phy_core_get_rxsense;
	g_aw_core->phy_ops.phy_get_pll_lock  = _phy_core_get_pll;
	g_aw_core->phy_ops.phy_set_power     = _phy_core_set_power;
	g_aw_core->phy_ops.phy_get_power     = _phy_core_get_power;
	g_aw_core->phy_ops.phy_reset         = _phy_core_reset;
	g_aw_core->phy_ops.phy_resume        = _phy_core_resume;
	g_aw_core->phy_ops.set_hpd           = _phy_core_set_hpd;
	g_aw_core->phy_ops.get_hpd           = _phy_core_get_hpd;

	return 0;
}

int _aw_core_init_ops_edid(void)
{
	g_aw_core->edid_ops.get_test_mode      = _edid_core_get_test_mode;
	g_aw_core->edid_ops.set_test_mode      = _edid_core_set_test_mode;
	g_aw_core->edid_ops.set_test_data      = _edid_core_set_test_data;
	g_aw_core->edid_ops.main_read          = _edid_core_edid_read;
	g_aw_core->edid_ops.main_release       = _edid_core_release;
	g_aw_core->edid_ops.set_prefered_video = _edid_core_set_video_prefered;
	g_aw_core->edid_ops.correct_hw_config  = _edid_core_correct_hardware_config;

	return 0;
}

static int _aw_core_init_ops_cec(void)
{
#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	g_aw_core->cec_ops.enable         = _cec_core_enable;
	g_aw_core->cec_ops.disable        = _cec_core_disable;
	g_aw_core->cec_ops.receive        = _cec_core_receive;
	g_aw_core->cec_ops.send           = _cec_core_send;
	g_aw_core->cec_ops.get_la         = _cec_core_get_logical_addr;
	g_aw_core->cec_ops.set_la         = _cec_core_set_logical_addr;
	g_aw_core->cec_ops.get_pa         = _cec_core_get_physic_addr;
	g_aw_core->cec_ops.get_ir_state   = _cec_core_interrupt_get_state;
	g_aw_core->cec_ops.clear_ir_state = _cec_core_interrupt_clear_state;
#endif
	return 0;
}

static void _aw_hdmi_init_phy(int phy_model)
{
	g_aw_core->hdmi_tx_phy = phy_model;
	g_dw_dev->snps_hdmi_ctrl.phy_access = DW_PHY_ACCESS_I2C;

#if defined(SUNXI_HDMI20_PHY_AW)
	aw_phy_set_reg_base(g_aw_core->reg_base);
#elif defined(SUNXI_HDMI20_PHY_INNO)
	inno_phy_set_reg_base(g_aw_core->reg_base);
#elif defined(SUNXI_HDMI20_PHY_SNPS)
#else
	hdmi_err("%s: not config phy model!!!\n", __func__);
#endif
}

static int _aw_hdmi_init_audio(void)
{
	int ret = 0;
	ret = dw_audio_param_reset(&g_aw_core->mode.pAudio);
	return ret;
}

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
static int _aw_hdmi_init_hdcp(struct hdmi_mode *mode, dw_hdcp_param_t *hdcp)
{
	dw_hdcp_param_t *pHdcp = NULL;

	if (mode == NULL) {
		hdmi_err("%s: param mode is null!!!\n", __func__);
		return -1;
	}

	if (hdcp == NULL) {
		hdmi_err("%s: param hdcp is null!!!\n", __func__);
		return -1;
	}

	pHdcp = &(mode->pHdcp);

	pHdcp->hdcp_on = 0;
	pHdcp->mRiCheck = -1;
	pHdcp->maxDevices = 0;
	pHdcp->mI2cFastMode = -1;
	pHdcp->mEnable11Feature = -1;
	pHdcp->mEnhancedLinkVerification = -1;

	pHdcp->mAksv = NULL;
	pHdcp->mKeys = NULL;
	pHdcp->mSwEncKey = NULL;
	pHdcp->mKsvListBuffer = NULL;
	pHdcp->use_hdcp = hdcp->use_hdcp;

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	pHdcp->use_hdcp22        = hdcp->use_hdcp22;
	pHdcp->esm_hpi_base      = hdcp->esm_hpi_base;
	pHdcp->esm_firm_phy_addr = hdcp->esm_firm_phy_addr;
	pHdcp->esm_firm_vir_addr = hdcp->esm_firm_vir_addr;
	pHdcp->esm_firm_size     = hdcp->esm_firm_size;
	pHdcp->esm_data_size     = hdcp->esm_data_size;
	pHdcp->esm_data_phy_addr = hdcp->esm_data_phy_addr;
	pHdcp->esm_data_vir_addr = hdcp->esm_data_vir_addr;
#endif

	g_dw_dev->snps_hdmi_ctrl.use_hdcp   = hdcp->use_hdcp;
	g_dw_dev->snps_hdmi_ctrl.use_hdcp22 = hdcp->use_hdcp22;

	if (hdcp->use_hdcp)
		dw_hdcp_initial(g_dw_dev, hdcp);

	return 0;
}
#endif

int _aw_hdmi_init_access(void)
{
	dw_access_init(&(g_aw_core->acs_ops));
	return 0;
}

int aw_hdmi_core_init(struct aw_hdmi_core_s *core,
		int phy_model, dw_hdcp_param_t *hdcp)
{
	g_aw_core = core;
	g_dw_dev = &core->hdmi_tx;

	memset(g_dw_dev, 0, sizeof(dw_hdmi_dev_t));

	g_aw_core->mode.sink_cap = kzalloc(sizeof(sink_edid_t), GFP_KERNEL);
	if (!g_aw_core->mode.sink_cap) {
		hdmi_err("%s: could not allocated edid sink!\n", __func__);
		return -1;
	}
	memset(g_aw_core->mode.sink_cap, 0, sizeof(sink_edid_t));

	g_aw_core->blacklist_sink = -1;

	mutex_init(&g_dw_dev->i2c_lock);

	_aw_hdmi_init_access();

	_aw_hdmi_init_phy(phy_model);

	_aw_hdmi_init_audio();

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	_aw_hdmi_init_hdcp(&g_aw_core->mode, hdcp);
#endif

	/* register general function ops */
	if (_aw_core_init_ops_device() != 0)
		hdmi_wrn("aw core general ops init failed!!!\n");

	/* register video function ops */
	if (_aw_core_init_ops_video() != 0)
		hdmi_wrn("aw core video ops init failed!!!\n");

	/* register hdcp function ops */
	if (_aw_core_init_ops_hdcp() != 0)
		hdmi_wrn("aw core hdcp ops init failed!!!\n");

	/* register audio function ops */
	if (_aw_core_init_ops_audio() != 0)
		hdmi_wrn("aw core hdcp ops init failed!!!\n");

	/* register phy function ops */
	if (_aw_core_init_ops_phy() != 0)
		hdmi_wrn("aw core phy ops init failed!!!\n");

	/* register edid function ops */
	if (_aw_core_init_ops_edid() != 0)
		hdmi_wrn("aw core edid ops init failed!!!\n");

	/* register cec function ops */
	if (_aw_core_init_ops_cec() != 0)
		hdmi_wrn("aw core cec ops init failed!!!\n");

	return 0;
}

void aw_hdmi_core_exit(struct aw_hdmi_core_s *g_aw_core)
{
	kfree(g_aw_core->mode.edid);
	kfree(g_aw_core->mode.edid_ext);
	kfree(g_aw_core->mode.sink_cap);
	kfree(g_aw_core);

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	dw_hdcp_exit();
#endif

	g_dw_dev = NULL;
}

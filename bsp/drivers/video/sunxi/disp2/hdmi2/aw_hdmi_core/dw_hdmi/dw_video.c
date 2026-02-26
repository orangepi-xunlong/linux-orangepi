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
#include "dw_access.h"
#include "dw_mc.h"
#include "dw_fc.h"
#include "dw_scdc.h"
#include "dw_video.h"

/**
 * @param params pointer to the video parameters structure
 * @return true if csc is needed
 */
int _dw_vp_check_color_space_conversion(dw_hdmi_dev_t *dev,
		dw_video_param_t *params)
{
	return params->mEncodingIn != params->mEncodingOut;
}

/**
 * @param params pointer to the video parameters structure
 * @return true if color space decimation is needed
 */
int _dw_vp_check_color_space_decimation(dw_hdmi_dev_t *dev,
		dw_video_param_t *params)
{
	return params->mEncodingOut == DW_COLOR_FORMAT_YCC422 &&
		(params->mEncodingIn == DW_COLOR_FORMAT_RGB ||
			params->mEncodingIn == DW_COLOR_FORMAT_YCC444);
}

/**
 * @param params pointer to the video parameters structure
 * @return true if if video is interpolated
 */
int _dw_vp_check_color_space_interpolation(dw_hdmi_dev_t *dev,
		dw_video_param_t *params)
{
	return params->mEncodingIn == DW_COLOR_FORMAT_YCC422 &&
		(params->mEncodingOut == DW_COLOR_FORMAT_RGB
			|| params->mEncodingOut == DW_COLOR_FORMAT_YCC444);
}

void _dw_vp_output_selector(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	if (value == 0) {	/* pixel packing */
		dev_write_mask(dev, VP_CONF, VP_CONF_BYPASS_EN_MASK, 0);
		/* enable pixel packing */
		dev_write_mask(dev, VP_CONF, VP_CONF_PP_EN_MASK, 1);
		dev_write_mask(dev, VP_CONF, VP_CONF_YCC422_EN_MASK, 0);
	} else if (value == 1) {	/* YCC422 */
		dev_write_mask(dev, VP_CONF, VP_CONF_BYPASS_EN_MASK, 0);
		dev_write_mask(dev, VP_CONF, VP_CONF_PP_EN_MASK, 0);
		/* enable YCC422 */
		dev_write_mask(dev, VP_CONF, VP_CONF_YCC422_EN_MASK, 1);
	} else if (value == 2 || value == 3) {	/* bypass */
		/* enable bypass */
		dev_write_mask(dev, VP_CONF, VP_CONF_BYPASS_EN_MASK, 1);
		dev_write_mask(dev, VP_CONF, VP_CONF_PP_EN_MASK, 0);
		dev_write_mask(dev, VP_CONF, VP_CONF_YCC422_EN_MASK, 0);
	} else {
		hdmi_err("%s: wrong output option: %d\n", __func__, value);
		return;
	}

	/* YCC422 stuffing */
	dev_write_mask(dev, VP_STUFF, VP_STUFF_YCC422_STUFFING_MASK, 1);
	/* pixel packing stuffing */
	dev_write_mask(dev, VP_STUFF, VP_STUFF_PP_STUFFING_MASK, 1);

	/* output selector */
	dev_write_mask(dev, VP_CONF, VP_CONF_OUTPUT_SELECTOR_MASK, value);
}

char *dw_vp_get_color_format_string(dw_color_format_t encoding)
{
	switch (encoding) {
	case	DW_COLOR_FORMAT_RGB:
		return "RGB";
	case	DW_COLOR_FORMAT_YCC444:
		return "YCbCr-444";
	case	DW_COLOR_FORMAT_YCC422:
		return "YCbCr-422";
	case	DW_COLOR_FORMAT_YCC420:
		return "YCbCr-420";
	default:
		break;
	}
	return "Undefined";
}

int dw_vp_get_cea_vic(int hdmi_vic)
{
	switch (hdmi_vic) {
	case 1:
		return 95;
		break;
	case 2:
		return 94;
		break;
	case 3:
		return 93;
		break;
	case 4:
		return 98;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

int dw_vp_get_hdmi_vic(int cea_vic)
{
	switch (cea_vic) {
	case 95:
		return 1;
		break;
	case 94:
		return 2;
		break;
	case 93:
		return 3;
		break;
	case 98:
		return 4;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

void dw_vp_set_support_ycc420(dw_dtd_t *paramsDtd, dw_edid_block_svd_t *paramsSvd)
{
	paramsDtd->mLimitedToYcc420 = paramsSvd->mLimitedToYcc420;
	paramsDtd->mYcc420 = paramsSvd->mYcc420;
}

void dw_vp_param_reset(dw_hdmi_dev_t *dev, dw_video_param_t *params)
{
	dw_dtd_t dtd;

	LOG_TRACE();
	dw_dtd_fill(dev, &dtd, 1, 60000);

	params->mHdmi = DW_TMDS_MODE_DVI;
	params->mEncodingOut = DW_COLOR_FORMAT_RGB;
	params->mEncodingIn = DW_COLOR_FORMAT_RGB;
	params->mColorResolution = DW_COLOR_DEPTH_8;
	params->mPixelRepetitionFactor = 0;
	params->mRgbQuantizationRange = 0;
	params->mPixelPackingDefaultPhase = 0;
	params->mColorimetry = 0;
	params->mScanInfo = 0;
	params->mActiveFormatAspectRatio = 8;
	params->mNonUniformScaling = 0;
	params->mExtColorimetry = ~0;
	params->mItContent = 0;
	params->mEndTopBar = ~0;
	params->mStartBottomBar = ~0;
	params->mEndLeftBar = ~0;
	params->mStartRightBar = ~0;
	params->mCscFilter = 0;
	params->mHdmiVideoFormat = 0;
	params->m3dStructure = 0;
	params->m3dExtData = 0;
	params->mHdmiVic = 0;
	params->mHdmi20 = 0;

	memcpy(&params->mDtd, &dtd, sizeof(dw_dtd_t));

/* params->mDtd.mCode = 0; */
/* params->mDtd.mLimitedToYcc420 = 0xFF; */
/* params->mDtd.mYcc420 = 0xFF; */
/* params->mDtd.mPixelRepetitionInput = 0xFF; */
/* params->mDtd.mPixelClock = 0; */
/* params->mDtd.mInterlaced = 0xFF; */
/* params->mDtd.mHActive = 0; */
/* params->mDtd.mHBlanking = 0; */
/* params->mDtd.mHBorder = 0xFFFF; */
/* params->mDtd.mHImageSize = 0; */
/* params->mDtd.mHSyncOffset = 0; */
/* params->mDtd.mHSyncPulseWidth = 0; */
/* params->mDtd.mHSyncPolarity = 0xFF; */
/* params->mDtd.mVActive = 0; */
/* params->mDtd.mVBlanking = 0; */
/* params->mDtd.mVBorder = 0xFFFF; */
/* params->mDtd.mVImageSize = 0; */
/* params->mDtd.mVSyncOffset = 0; */
/* params->mDtd.mVSyncPulseWidth = 0; */
/* params->mDtd.mVSyncPolarity = 0xFF; */
}

/* [KHz] */
u32 dw_vp_get_pixel_clk(dw_hdmi_dev_t *dev, dw_video_param_t *params)
{
	if (params->mHdmiVideoFormat == 2) {
		if (params->m3dStructure == 0)
			return 2 * params->mDtd.mPixelClock;
	}
	return params->mDtd.mPixelClock;
}

/* 0.01 */
unsigned dw_vp_get_ratio_clk(dw_hdmi_dev_t *dev, dw_video_param_t *params)
{
	unsigned ratio = 100;

	if (params->mEncodingOut != DW_COLOR_FORMAT_YCC422) {
		if (params->mColorResolution == 8)
			ratio = 100;
		else if (params->mColorResolution == 10)
			ratio = 125;
		else if (params->mColorResolution == 12)
			ratio = 150;
		else if (params->mColorResolution == 16)
			ratio = 200;
	}
	return ratio * (params->mPixelRepetitionFactor + 1);
}

/**
 * @param params pointer to the video parameters structure
 * @return true if if video has pixel repetition
 */
int dw_vp_check_pixel_repet(dw_hdmi_dev_t *dev, dw_video_param_t *params)
{
	return (params->mPixelRepetitionFactor > 0) ||
		(params->mDtd.mPixelRepetitionInput > 0);
}

void dw_vp_update_csc_coefficients(dw_hdmi_dev_t *dev, dw_video_param_t *params)
{
	u16 i = 0;

	if (!_dw_vp_check_color_space_conversion(dev, params)) {
		for (i = 0; i < 4; i++) {
			params->mCscA[i] = 0;
			params->mCscB[i] = 0;
			params->mCscC[i] = 0;
		}
		params->mCscA[0] = 0x2000;
		params->mCscB[1] = 0x2000;
		params->mCscC[2] = 0x2000;
		params->mCscScale = 1;
	} else if (_dw_vp_check_color_space_conversion(dev, params) &&
		params->mEncodingOut == DW_COLOR_FORMAT_RGB) {
		if (params->mColorimetry == DW_METRY_ITU601) {
			params->mCscA[0] = 0x2000;
			params->mCscA[1] = 0x6926;
			params->mCscA[2] = 0x74fd;
			params->mCscA[3] = 0x010e;

			params->mCscB[0] = 0x2000;
			params->mCscB[1] = 0x2cdd;
			params->mCscB[2] = 0x0000;
			params->mCscB[3] = 0x7e9a;

			params->mCscC[0] = 0x2000;
			params->mCscC[1] = 0x0000;
			params->mCscC[2] = 0x38b4;
			params->mCscC[3] = 0x7e3b;

			params->mCscScale = 1;
		} else if (params->mColorimetry == DW_METRY_ITU709) {
			params->mCscA[0] = 0x2000;
			params->mCscA[1] = 0x7106;
			params->mCscA[2] = 0x7a02;
			params->mCscA[3] = 0x00a7;

			params->mCscB[0] = 0x2000;
			params->mCscB[1] = 0x3264;
			params->mCscB[2] = 0x0000;
			params->mCscB[3] = 0x7e6d;

			params->mCscC[0] = 0x2000;
			params->mCscC[1] = 0x0000;
			params->mCscC[2] = 0x3b61;
			params->mCscC[3] = 0x7e25;

			params->mCscScale = 1;
		}
	} else if (_dw_vp_check_color_space_conversion(dev, params) &&
		params->mEncodingIn == DW_COLOR_FORMAT_RGB) {
		if (params->mColorimetry == DW_METRY_ITU601) {
			params->mCscA[0] = 0x2591;
			params->mCscA[1] = 0x1322;
			params->mCscA[2] = 0x074b;
			params->mCscA[3] = 0x0000;

			params->mCscB[0] = 0x6535;
			params->mCscB[1] = 0x2000;
			params->mCscB[2] = 0x7acc;
			params->mCscB[3] = 0x0200;

			params->mCscC[0] = 0x6acd;
			params->mCscC[1] = 0x7534;
			params->mCscC[2] = 0x2000;
			params->mCscC[3] = 0x0200;

			params->mCscScale = 0;
		} else if (params->mColorimetry == DW_METRY_ITU709) {
			params->mCscA[0] = 0x2dc5;
			params->mCscA[1] = 0x0d9b;
			params->mCscA[2] = 0x049e;
			params->mCscA[3] = 0x0000;

			params->mCscB[0] = 0x62f0;
			params->mCscB[1] = 0x2000;
			params->mCscB[2] = 0x7d11;
			params->mCscB[3] = 0x0200;

			params->mCscC[0] = 0x6756;
			params->mCscC[1] = 0x78ab;
			params->mCscC[2] = 0x2000;
			params->mCscC[3] = 0x0200;

			params->mCscScale = 0;
		}
	}
	/* else use user coefficients */
}

/* be used if the user wants to do pixel repetition using H13TCTRL controller */
void dw_vp_set_pixel_repetition_factor(dw_hdmi_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	/* desired factor */
	dev_write_mask(dev, VP_PR_CD, VP_PR_CD_DESIRED_PR_FACTOR_MASK, value);
	/* enable stuffing */
	dev_write_mask(dev, VP_STUFF, VP_STUFF_PR_STUFFING_MASK, 1);
	/* enable block */
	dev_write_mask(dev, VP_CONF, VP_CONF_PR_EN_MASK, (value > 1) ? 1 : 0);
	/* bypass block */
	dev_write_mask(dev, VP_CONF, VP_CONF_BYPASS_SELECT_MASK,
						(value > 1) ? 0 : 1);
}

u8 dw_vp_get_color_depth(dw_hdmi_dev_t *dev)
{
	u8 depth = 0;

	LOG_TRACE();

	switch (dev_read_mask(dev, VP_PR_CD, VP_PR_CD_COLOR_DEPTH_MASK)) {
	case 0x0:
	case 0x4:
		depth = 8;
		break;
	case 0x5:
		depth = 10;
		break;
	case 0x6:
		depth = 12;
		break;
	case 0x7:
		depth = 16;
		break;
	default:
		depth = 8;
		break;
	}
	return depth;
}

u8 dw_vp_get_pixel_repet_factor(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, VP_PR_CD, VP_PR_CD_DESIRED_PR_FACTOR_MASK);
}

void _dw_video_sampler_config(dw_hdmi_dev_t *dev, u8 map_code)
{
	dev_write_mask(dev, TX_INVID0, TX_INVID0_INTERNAL_DE_GENERATOR_MASK, 0x1);

	dev_write_mask(dev, TX_INVID0, TX_INVID0_VIDEO_MAPPING_MASK, map_code);

	dev_write(dev, (TX_GYDATA0), 0x0);
	dev_write(dev, (TX_GYDATA1), 0x0);
	dev_write_mask(dev, TX_INSTUFFING, TX_INSTUFFING_GYDATA_STUFFING_MASK, 1);


	dev_write(dev, (TX_RCRDATA0), 0x0);
	dev_write(dev, (TX_RCRDATA1), 0x0);
	dev_write_mask(dev, TX_INSTUFFING, TX_INSTUFFING_RCRDATA_STUFFING_MASK, 1);

	dev_write(dev, (TX_BCBDATA0), 0x0);
	dev_write(dev, (TX_BCBDATA1), 0x0);
	dev_write_mask(dev, TX_INSTUFFING, TX_INSTUFFING_BCBDATA_STUFFING_MASK, 1);
}

void _dw_video_csc_config(dw_hdmi_dev_t *dev, dw_video_param_t *video,
		unsigned interpolation, unsigned decimation, unsigned color_depth)
{
	dev_write_mask(dev, CSC_CFG, CSC_CFG_INTMODE_MASK, interpolation);
	dev_write_mask(dev, CSC_CFG, CSC_CFG_DECMODE_MASK, decimation);

	dev_write(dev, CSC_COEF_A1_LSB, (u8)(video->mCscA[0]));
	dev_write_mask(dev, CSC_COEF_A1_MSB,
			CSC_COEF_A1_MSB_CSC_COEF_A1_MSB_MASK, (u8)(video->mCscA[0] >> 8));

	dev_write(dev, CSC_COEF_A2_LSB, (u8)(video->mCscA[1]));
	dev_write_mask(dev, CSC_COEF_A2_MSB,
			CSC_COEF_A2_MSB_CSC_COEF_A2_MSB_MASK, (u8)(video->mCscA[1] >> 8));

	dev_write(dev, CSC_COEF_A3_LSB, (u8)(video->mCscA[2]));
	dev_write_mask(dev, CSC_COEF_A3_MSB,
		CSC_COEF_A3_MSB_CSC_COEF_A3_MSB_MASK, (u8)(video->mCscA[2] >> 8));

	dev_write(dev, CSC_COEF_A4_LSB, (u8)(video->mCscA[3]));
	dev_write_mask(dev, CSC_COEF_A4_MSB,
		CSC_COEF_A4_MSB_CSC_COEF_A4_MSB_MASK, (u8)(video->mCscA[3] >> 8));

	dev_write(dev, CSC_COEF_B1_LSB, (u8)(video->mCscB[0]));
	dev_write_mask(dev, CSC_COEF_B1_MSB,
		CSC_COEF_B1_MSB_CSC_COEF_B1_MSB_MASK, (u8)(video->mCscB[0] >> 8));

	dev_write(dev, CSC_COEF_B2_LSB, (u8)(video->mCscB[1]));
	dev_write_mask(dev, CSC_COEF_B2_MSB,
			CSC_COEF_B2_MSB_CSC_COEF_B2_MSB_MASK, (u8)(video->mCscB[1] >> 8));

	dev_write(dev, CSC_COEF_B3_LSB, (u8)(video->mCscB[2]));
	dev_write_mask(dev, CSC_COEF_B3_MSB,
		CSC_COEF_B3_MSB_CSC_COEF_B3_MSB_MASK, (u8)(video->mCscB[2] >> 8));

	dev_write(dev, CSC_COEF_B4_LSB, (u8)(video->mCscB[3]));
	dev_write_mask(dev, CSC_COEF_B4_MSB,
		CSC_COEF_B4_MSB_CSC_COEF_B4_MSB_MASK, (u8)(video->mCscB[3] >> 8));

	dev_write(dev, CSC_COEF_C1_LSB, (u8) (video->mCscC[0]));
	dev_write_mask(dev, CSC_COEF_C1_MSB,
		CSC_COEF_C1_MSB_CSC_COEF_C1_MSB_MASK, (u8)(video->mCscC[0] >> 8));

	dev_write(dev, CSC_COEF_C2_LSB, (u8) (video->mCscC[1]));
	dev_write_mask(dev, CSC_COEF_C2_MSB,
			CSC_COEF_C2_MSB_CSC_COEF_C2_MSB_MASK, (u8)(video->mCscC[1] >> 8));

	dev_write(dev, CSC_COEF_C3_LSB, (u8) (video->mCscC[2]));
	dev_write_mask(dev, CSC_COEF_C3_MSB,
		CSC_COEF_C3_MSB_CSC_COEF_C3_MSB_MASK, (u8)(video->mCscC[2] >> 8));

	dev_write(dev, CSC_COEF_C4_LSB, (u8) (video->mCscC[3]));
	dev_write_mask(dev, CSC_COEF_C4_MSB,
			CSC_COEF_C4_MSB_CSC_COEF_C4_MSB_MASK, (u8)(video->mCscC[3] >> 8));

	dev_write_mask(dev, CSC_SCALE, CSC_SCALE_CSCSCALE_MASK, video->mCscScale);
	dev_write_mask(dev, CSC_SCALE, CSC_SCALE_CSC_COLOR_DEPTH_MASK, color_depth);

}

/**
 * Set up color space converter to video requirements
 * (if there is any encoding type conversion or csc coefficients)
 * @param baseAddr Base Address of module
 * @param params VideoParams
 * @return true if successful
 */
int _dw_video_color_space_converter(dw_hdmi_dev_t *dev, dw_video_param_t *video)
{
	unsigned interpolation = 0;
	unsigned decimation = 0;
	unsigned color_depth = 0;

	LOG_TRACE();

	if (_dw_vp_check_color_space_interpolation(dev, video)) {
		if (video->mCscFilter > 1) {
			hdmi_err("%s: invalid chroma interpolation filter:%d\n", __func__, video->mCscFilter);
			return false;
		}
		interpolation = 1 + video->mCscFilter;
	} else if (_dw_vp_check_color_space_decimation(dev, video)) {
		if (video->mCscFilter > 2) {
			hdmi_err("%s: invalid chroma decimation filter:%d\n", __func__, video->mCscFilter);
			return false;
		}
		decimation = 1 + video->mCscFilter;
	}

	if ((video->mColorResolution == DW_COLOR_DEPTH_8) ||
		(video->mColorResolution == 0))
		color_depth = 4;
	else if (video->mColorResolution == DW_COLOR_DEPTH_10)
		color_depth = 5;
	else if (video->mColorResolution == DW_COLOR_DEPTH_12)
		color_depth = 6;
	else if (video->mColorResolution == DW_COLOR_DEPTH_16)
		color_depth = 7;
	else {
		hdmi_err("%s: invalid color depth: %d\n",
				__func__, video->mColorResolution);
		return false;
	}

	VIDEO_INF("interpolation:%d  decimation:%d  color_depth:%d\n",
					interpolation, decimation, color_depth);
	_dw_video_csc_config(dev, video, interpolation, decimation, color_depth);

	return true;
}

/**
 * Set up video packetizer which "packetizes" pixel transmission
 * (in deep colour mode, YCC422 mapping and pixel repetition)
 * @param baseAddr Base Address of module
 * @param params VideoParams
 * @return true if successful
 */
int _dw_video_packetizer(dw_hdmi_dev_t *dev, dw_video_param_t *video)
{
	unsigned color_depth = 0;
	unsigned remap_size = 0;
	unsigned output_select = 0;

	LOG_TRACE();
	if ((video->mEncodingOut == DW_COLOR_FORMAT_RGB) || (video->mEncodingOut == DW_COLOR_FORMAT_YCC444) ||
		(video->mEncodingOut == DW_COLOR_FORMAT_YCC420)) {
		if (video->mColorResolution == 0)
			output_select = 3;
		else if (video->mColorResolution == DW_COLOR_DEPTH_8) {
			color_depth = 0;
			output_select = 3;
		} else if (video->mColorResolution == DW_COLOR_DEPTH_10)
			color_depth = 5;
		else if (video->mColorResolution == DW_COLOR_DEPTH_12)
			color_depth = 6;
		else if (video->mColorResolution == DW_COLOR_DEPTH_16)
			color_depth = 7;
		else {
			hdmi_err("%s: invalid color depth: %d format: %d\n",
					__func__, video->mColorResolution, video->mEncodingOut);
			return false;
		}
	} else if (video->mEncodingOut == DW_COLOR_FORMAT_YCC422) {
		if ((video->mColorResolution == DW_COLOR_DEPTH_8) ||
			(video->mColorResolution == 0))
			remap_size = 0;
		else if (video->mColorResolution == DW_COLOR_DEPTH_10)
			remap_size = 1;
		else if (video->mColorResolution == DW_COLOR_DEPTH_12)
			remap_size = 2;
		else {
			hdmi_err("%s: invalid color depth: %d format: %d\n",
					__func__, video->mColorResolution, video->mEncodingOut);
			return false;
		}
		output_select = 1;
	} else {
		hdmi_err("%s: invalid output encoding type: %d\n",
				__func__, video->mEncodingOut);
		return false;
	}

	dw_vp_set_pixel_repetition_factor(dev, video->mPixelRepetitionFactor);
	dev_write_mask(dev, VP_PR_CD, VP_PR_CD_COLOR_DEPTH_MASK, color_depth);

	dev_write_mask(dev, VP_STUFF, VP_STUFF_IDEFAULT_PHASE_MASK,
		video->mPixelPackingDefaultPhase);

	dev_write_mask(dev, VP_REMAP, VP_REMAP_YCC422_SIZE_MASK, remap_size);

	_dw_vp_output_selector(dev, output_select);
	return true;
}

/**
 * Set up video mapping and stuffing
 * @param baseAddr Base Address of module
 * @param params VideoParams
 * @return true if successful
 */
int _dw_video_sampler(dw_hdmi_dev_t *dev, dw_video_param_t *video)
{
	unsigned map_code = 0;

	LOG_TRACE();

	if (video->mEncodingIn == DW_COLOR_FORMAT_RGB) {
		if ((video->mColorResolution == DW_COLOR_DEPTH_8) ||
			(video->mColorResolution == 0))
			map_code = 1;
		else if (video->mColorResolution == DW_COLOR_DEPTH_10)
			map_code = 3;
		else if (video->mColorResolution == DW_COLOR_DEPTH_12)
			map_code = 5;
		else if (video->mColorResolution == DW_COLOR_DEPTH_16)
			map_code = 7;
		else {
			hdmi_err("%s: invalid color depth: %d format: %d\n",
					__func__, video->mColorResolution, video->mEncodingIn);
			return false;
		}
	} else if (video->mEncodingIn == DW_COLOR_FORMAT_YCC422) {
		/* YCC422 mapping is discontinued - only map 1 is supported */
		if (video->mColorResolution == DW_COLOR_DEPTH_8)
			map_code = 22;
		else if (video->mColorResolution == DW_COLOR_DEPTH_10)
			map_code = 20;
		else if ((video->mColorResolution == DW_COLOR_DEPTH_12) ||
			(video->mColorResolution == 0))
			map_code = 18;
		else {
			hdmi_err("%s: invalid color depth: %d format: %d\n",
					__func__, video->mColorResolution, video->mEncodingIn);
			return false;
		}
	} else if (video->mEncodingIn == DW_COLOR_FORMAT_YCC444) {
		if (video->mColorResolution == DW_COLOR_DEPTH_8)
			map_code = 23;
		else if (video->mColorResolution == DW_COLOR_DEPTH_10)
			map_code = 24;
		else if ((video->mColorResolution == DW_COLOR_DEPTH_12) ||
			(video->mColorResolution == 0))
			map_code = 27;
		else {
			hdmi_err("%s: invalid color depth: %d format: %d\n",
					__func__, video->mColorResolution, video->mEncodingIn);
			return false;
		}
	} else if (video->mEncodingIn == DW_COLOR_FORMAT_YCC420) {
		if (video->mColorResolution == DW_COLOR_DEPTH_8)
			map_code = 9;
		else if (video->mColorResolution == DW_COLOR_DEPTH_10)
			map_code = 11;
		else if ((video->mColorResolution == DW_COLOR_DEPTH_12) ||
			(video->mColorResolution == 0))
			map_code = 13;
		else {
			hdmi_err("%s: invalid color depth: %d format: %d\n",
					__func__, video->mColorResolution, video->mEncodingIn);
			return false;
		}
	} else {
		hdmi_err("%s: invalid input encoding type: %d\n",
				__func__, video->mEncodingIn);
		return false;
	}

	_dw_video_sampler_config(dev, map_code);

	return true;
}

int dw_video_config(dw_hdmi_dev_t *dev, dw_video_param_t *video)
{
	LOG_TRACE();

	/* DVI mode does not support pixel repetition */
	if ((video->mHdmi == DW_TMDS_MODE_DVI) &&
		dw_vp_check_pixel_repet(dev, video)) {
		hdmi_err("%s: DVI mode with pixel repetition:video not transmitted\n", __func__);
		return false;
	}

	dw_fc_force_output(dev, 1);

	if (dw_fc_video_config(dev, video) == false)
		return false;
	if (_dw_video_packetizer(dev, video) == false)
		return false;
	if (_dw_video_color_space_converter(dev, video) == false)
		return false;
	if (_dw_video_sampler(dev, video) == false)
		return false;

	return true;
}

#ifndef SUPPORT_ONLY_HDMI14
void dw_video_set_scramble(dw_hdmi_dev_t *dev, u8 enable)
{
	LOG_TRACE1(enable);
	if (enable == 1) {
		dw_scdc_set_scramble(dev, true);

		/* Start/stop HDCP keep-out window generation not needed because it's always on */
		/* TMDS software reset request */
		dw_mc_tmds_clock_reset(dev, true);

		/* Enable/Disable Scrambling */
		dw_fc_video_set_scramble(dev, true);

		dw_scdc_set_tmds_clk_ratio(dev, true);
	} else {
		/* Enable/Disable Scrambling */
		dw_fc_video_set_scramble(dev, false);

		dw_scdc_set_scramble(dev, false);

		/* TMDS software reset request */
		dw_mc_tmds_clock_reset(dev, true);

		/* Write on RX the bit Scrambling_Enable, register 0x20 */
		dw_scdc_set_tmds_clk_ratio(dev, false);
	}
}
#endif  /* SUPPORT_ONLY_HDMI14 */

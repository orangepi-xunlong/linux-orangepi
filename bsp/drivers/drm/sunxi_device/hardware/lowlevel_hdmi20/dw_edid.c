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
#include "dw_dev.h"
#include "dw_edid.h"

#ifdef sink_info_reset
#undef sink_info_reset
#endif
#define sink_info_reset(p_data, size)   \
	do {                                \
		if (IS_ERR_OR_NULL(p_data))     \
			memset(p_data, 0x0, size);  \
	} while (0)

#define TAG_BASE_BLOCK         0x00
#define TAG_CEA_EXT            0x02
#define TAG_VTB_EXT            0x10
#define TAG_DI_EXT             0x40
#define TAG_LS_EXT             0x50
#define TAG_MI_EXT             0x60

#define HDMI_IEEE_CODE         (0x000C03)
#define HDMI_FORUM_IEEE_CODE   (0xC45DD8)

enum cta_edid_tag {
	/* Reserved: 0 */
	TAG_ADB        = 1, /* Audio Data Block */
	TAG_VDB        = 2, /* Video Data Block */
	TAG_VSDB       = 3, /* Vendor-Specific Data Block */
	TAG_SADB       = 4, /* Speaker Allocation Data Block */
	TAG_VESA_DTCDB = 5, /* VESA Display Transfer Characteristic Data Block */
	/* Reserved: 6 */
	TAG_Extend     = 7, /* Use Extended Tag */
};

/* edid data block extern-tag */
enum cta_edid_extend_tag {
	EXT_TAG_Video_Cap_DB             = 0, /* Video Capability Data Block */
	EXT_TAG_Vendor_Specifiv_VDB      = 1, /* Vendor-Specific Video Data Block */
	EXT_TAG_VESA_Display_Device_DB   = 2, /* VESA Display Device Data Block */
	EXT_TAG_VESA_VDB                 = 3, /* Reserved: VESA Video Data Block */
	EXT_TAG_HDMI_VDB                 = 4, /* Reserved: HDMI Video Data Block */
	EXT_TAG_Colorimetry_DB           = 5, /* Colorimetry Data Block */
	EXT_TAG_HDR_SMDB                 = 6, /* HDR Static Metadata Data Block */
	EXT_TAG_HDR_DMDB                 = 7, /* HDR Dynamic Metadata Data Block */
	/* reserved: 8...12 */
	EXT_TAG_Video_Format_Per_DB      = 13, /* Video Format Preference Data Block */
	EXT_TAG_YCbCr420_Video_DB        = 14, /* YCbCr 4:2:0 Video Data Block */
	EXT_TAG_YCbCr420_Cap_Map_DB      = 15, /* YCbCr 4:2:0 Capability Map Data Block */
	EXT_TAG_Misc_Audio_Field_DB      = 16, /* Reserved: CTA Miscellaneous Auido Fields */
	EXT_TAG_Vendor_Specific_ADB      = 17, /* Vendor-Specific Audio Data Block */
	EXT_TAG_HDMI_Audio_DB            = 18, /* HDMI Audio Data Block */
	EXT_TAG_ROOM_Config_DB           = 19, /* Room Configuration Data Block */
	EXT_TAG_Speaker_Location_DB      = 20, /* Speaker Location Data Block */
	/* reserved: 21...31 */
	EXT_TAG_Infoframe_DB             = 32, /* Infoframe Data Block */
	/* reserved: 33 */
	EXT_TAG_DisplayID_TypeVII_VTDB   = 34, /* DisplayID Type-VII Video Timing Data Block */
	EXT_TAG_DisplayID_TypeVIII_VTDB  = 35, /* DisplayID Type-VIII Video Timing Data Block */
	/* reserved: 36...41 */
	EXT_TAG_DisplayID_TypeX_VTDB     = 42, /* DisplayID Type-X Video Timing Data Block */
	/* reserved: 43...119 */
	EXT_TAG_HF_EEODB                 = 120, /* HDMI Forum EDID Extension Override Data Block */
	EXT_TAG_HF_SCDB                  = 121, /* HDMI Forum Sink Capability Data Block */
	/* reserved: 122...127 for HDMI */
	/* reserved: 128...255 */
};

const unsigned DTD_SIZE = 0x12;

static struct sink_info_s *dw_get_sink(void)
{
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();

	if (IS_ERR_OR_NULL(hdmi)) {
		shdmi_err(hdmi);
		return NULL;
	}

	return hdmi->sink_info;
}

static int _dw_edid_update_sink_hdmi20(bool state)
{
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}

	sink->edid_m20Sink = state;
	hdmi->video_dev.mHdmi20 = state;
	return 0;
}

static void _reset_sink_monitor_desc(dw_edid_monitor_descriptior_data_t *data)
{
	sink_info_reset(data, sizeof(dw_edid_monitor_descriptior_data_t));
}

static void _reset_sink_colorimetry(dw_edid_colorimetry_data_t *data)
{
	sink_info_reset(data, sizeof(dw_edid_colorimetry_data_t));
}

static void _reset_sink_hdr_static_metadata(dw_edid_hdr_static_metadata_data_t *data)
{
	sink_info_reset(data, sizeof(dw_edid_hdr_static_metadata_data_t));
}

static void _reset_sink_hdmi_forum(dw_edid_hdmi_forum_vs_data_t *data)
{
	sink_info_reset(data, sizeof(dw_edid_hdmi_forum_vs_data_t));
}

static void _reset_sink_hdmi14(dw_edid_hdmi_vs_data_t *data)
{
	sink_info_reset(data, sizeof(dw_edid_hdmi_vs_data_t));
}

static void _reset_sink_video_capabilit(dw_edid_video_capabilit_data_t *data)
{
	sink_info_reset(data, sizeof(dw_edid_video_capabilit_data_t));
}

static void _reset_sink_short_audio(dw_edid_block_sad_t *data)
{
	sink_info_reset(data, sizeof(dw_edid_block_sad_t));
}

static void _reset_sink_short_video(dw_edid_block_svd_t *data)
{
	sink_info_reset(data, sizeof(dw_edid_block_svd_t));
}

static void _reset_sink_speaker_alloction(dw_edid_speaker_allocation_data_t *data)
{
	sink_info_reset(data, sizeof(dw_edid_speaker_allocation_data_t));
}

static int _parse_hdmi_forum_info(dw_edid_hdmi_forum_vs_data_t *hf, u8 *data)
{
	hf->mVersion             = dw_bit_field(data[4], 0, 8);
	hf->mMaxTmdsCharRate     = dw_bit_field(data[5], 0, 8);
	hf->mSCDC_Present        = dw_bit_field(data[6], 7, 1);
	hf->mRR_Capable          = dw_bit_field(data[6], 6, 1);
	hf->mLTS_340Mcs_scramble = dw_bit_field(data[6], 3, 1);
	hf->mIndependentView     = dw_bit_field(data[6], 2, 1);
	hf->mDualView            = dw_bit_field(data[6], 1, 1);
	hf->m3D_OSD_Disparity    = dw_bit_field(data[6], 0, 1);
	hf->mDC_30bit_420        = dw_bit_field(data[7], 0, 1);
	hf->mDC_36bit_420        = dw_bit_field(data[7], 1, 1);
	hf->mDC_48bit_420        = dw_bit_field(data[7], 2, 1);
	hf->mValid = true;
	return 0;
}

/**
 * Parses the Detailed Timing Descriptor.
 * Encapsulating the parsing process
 * @param dtd pointer to dw_dtd_t strucutute for the information to be save in
 * @param data a pointer to the 18-byte structure to be parsed.
 * @return true if success
 */
int _parse_data_block_detailed_timing(dw_dtd_t *dtd, u8 data[18])
{
	if (IS_ERR_OR_NULL(dtd)) {
		shdmi_err(dtd);
		return false;
	}

	dtd->mCode = -1;
	dtd->mPixelRepetitionInput = 0;
	dtd->mLimitedToYcc420 = 0;
	dtd->mYcc420 = 0;

	dtd->mPixelClock = 1000 * dw_byte_to_word(data[1], data[0]);/* [10000Hz] */
	if (dtd->mPixelClock < 0x01) {	/* 0x0000 is defined as reserved */
		hdmi_err("dw edid parse dtd timing pixel clock[%dKHz] invalid!\n",
				dtd->mPixelClock);
		return false;
	}

	dtd->mHActive         = dw_concat_bits(data[4], 4, 4, data[2], 0, 8);
	dtd->mHBlanking       = dw_concat_bits(data[4], 0, 4, data[3], 0, 8);
	dtd->mHSyncOffset     = dw_concat_bits(data[11], 6, 2, data[8], 0, 8);
	dtd->mHSyncPulseWidth = dw_concat_bits(data[11], 4, 2, data[9], 0, 8);
	dtd->mHImageSize      = dw_concat_bits(data[14], 4, 4, data[12], 0, 8);
	dtd->mHBorder         = data[15];

	dtd->mVActive         = dw_concat_bits(data[7], 4, 4, data[5], 0, 8);
	dtd->mVBlanking       = dw_concat_bits(data[7], 0, 4, data[6], 0, 8);
	dtd->mVSyncOffset     = dw_concat_bits(data[11], 2, 2, data[10], 4, 4);
	dtd->mVSyncPulseWidth = dw_concat_bits(data[11], 0, 2, data[10], 0, 4);
	dtd->mVImageSize      = dw_concat_bits(data[14], 0, 4, data[13], 0, 8);
	dtd->mVBorder         = data[16];

	if (dw_bit_field(data[17], 4, 1) != 1) {/* if not DIGITAL SYNC SIGNAL DEF */
		hdmi_err("invalid dtd byte[17] bit4 parameters\n");
		return false;
	}

	if (dw_bit_field(data[17], 3, 1) != 1) {/* if not DIGITAL SEPATATE SYNC */
		hdmi_err("invalid dtd byte[17] bit3 parameters\n");
		return false;
	}

	/* no stereo viewing support in HDMI */
	dtd->mInterlaced    = (dw_bit_field(data[17], 7, 1) == 1);
	dtd->mVSyncPolarity = (dw_bit_field(data[17], 2, 1) == 1);
	dtd->mHSyncPolarity = (dw_bit_field(data[17], 1, 1) == 1);
	return true;
}

int _parse_data_block_short_audio(dw_edid_block_sad_t *sad, u8 *data)
{
	if (IS_ERR_OR_NULL(sad)) {
		shdmi_err(sad);
		return false;
	}

	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		return false;
	}

	_reset_sink_short_audio(sad);

	sad->mFormat      = dw_bit_field(data[0], 3, 4);
	sad->mMaxChannels = dw_bit_field(data[0], 0, 3) + 1;
	sad->mSampleRates = dw_bit_field(data[1], 0, 7);
	sad->mByte3       = data[2];

	return true;
}

int _parse_data_block_short_video(dw_edid_block_svd_t *svd, u8 data)
{
	if (IS_ERR_OR_NULL(svd)) {
		shdmi_err(svd);
		return false;
	}

	_reset_sink_short_video(svd);

	svd->mYcc420 = 0;
	svd->mLimitedToYcc420 = 0;
	svd->mCode   = dw_bit_field(data, 0, 7);
	svd->mNative = (dw_bit_field(data, 7, 1) == 1) ? true : false;

	hdmi_trace(" - svd code[%03d], native[%d]\n",
			svd->mCode, svd->mNative);
	return true;
}

/**
 * Parse an array of data to fill the dw_edid_hdmi_vs_data_t data strucutre
 * @param *vsdb pointer to the structure to be filled
 * @param *data pointer to the 8-bit data type array to be parsed
 * @return Success, or error code:
 * @return 1 - array pointer invalid
 * @return 2 - Invalid datablock tag
 * @return 3 - Invalid minimum length
 * @return 4 - HDMI IEEE registration identifier not valid
 * @return 5 - Invalid length - latencies are not valid
 * @return 6 - Invalid length - Interlaced latencies are not valid
 */
int _parse_data_block_hdmi14(dw_edid_hdmi_vs_data_t *vsdb, u8 *data)
{
	u8 blockLength = 0;
	unsigned videoInfoStart = 0;
	unsigned hdmi3dStart = 0;
	unsigned hdmiVicLen = 0;
	unsigned hdmi3dLen = 0;
	unsigned spanned3d = 0;
	unsigned i = 0;
	unsigned j = 0;

	u8 len = 0;

	if (IS_ERR_OR_NULL(vsdb)) {
		shdmi_err(vsdb);
		return false;
	}

	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		return false;
	}

	blockLength = dw_bit_field(data[0], 0, 5);
	if (blockLength < 5) {
		hdmi_err("dw edid check hdmi14 vsdb len[%d] invalid\n", len);
		return false;
	}

	_reset_sink_hdmi14(vsdb);

	vsdb->mId = HDMI_IEEE_CODE;
	vsdb->mPhysicalAddress = dw_byte_to_word(data[4], data[5]);

	if (blockLength == 5) {
		hdmi_trace("dw edid hdmi14 vsdb only parse physical address\n");
		return true;
	}

	/* parse extension fields if they exist */
	vsdb->mSupportsAi    = dw_bit_field(data[6], 7, 1) == 1;
	vsdb->mDeepColor48   = dw_bit_field(data[6], 6, 1) == 1;
	vsdb->mDeepColor36   = dw_bit_field(data[6], 5, 1) == 1;
	vsdb->mDeepColor30   = dw_bit_field(data[6], 4, 1) == 1;
	vsdb->mDeepColorY444 = dw_bit_field(data[6], 3, 1) == 1;
	vsdb->mDviDual       = dw_bit_field(data[6], 0, 1) == 1;

	vsdb->mMaxTmdsClk = (blockLength > 6) ? data[7] : 0;
	vsdb->mVideoLatency = 0;
	vsdb->mAudioLatency = 0;
	vsdb->mInterlacedVideoLatency = 0;
	vsdb->mInterlacedAudioLatency = 0;
	if (blockLength > 7) {
		if (dw_bit_field(data[8], 7, 1) == 1) {
			if (blockLength < 10) {
				hdmi_err("Invalid length - latencies are not valid\n");
				return false;
			}
			if (dw_bit_field(data[8], 6, 1) == 1) {
				if (blockLength < 12) {
					hdmi_err("Invalid length - Interlaced latencies are not valid\n");
					return false;
				} else {
					vsdb->mVideoLatency = data[9];
					vsdb->mAudioLatency = data[10];
					vsdb->mInterlacedVideoLatency = data[11];
					vsdb->mInterlacedAudioLatency = data[12];
					videoInfoStart = 13;
				}
			} else {
				vsdb->mVideoLatency = data[9];
				vsdb->mAudioLatency = data[10];
				vsdb->mInterlacedVideoLatency = 0;
				vsdb->mInterlacedAudioLatency = 0;
				videoInfoStart = 11;
			}
		} else { /* no latency data */
			vsdb->mVideoLatency = 0;
			vsdb->mAudioLatency = 0;
			vsdb->mInterlacedVideoLatency = 0;
			vsdb->mInterlacedAudioLatency = 0;
			videoInfoStart = 9;
		}
		vsdb->mContentTypeSupport = dw_bit_field(data[8], 0, 4);
	}
	/* additional video format capabilities are described */
	if (dw_bit_field(data[8], 5, 1) == 1) {
		vsdb->mImageSize = dw_bit_field(data[videoInfoStart], 3, 2);
		hdmiVicLen = dw_bit_field(data[videoInfoStart + 1], 5, 3);
		hdmi3dLen = dw_bit_field(data[videoInfoStart + 1], 0, 5);
		for (i = 0; i < hdmiVicLen; i++)
			vsdb->mHdmiVic[i] = data[videoInfoStart + 2 + i];

		vsdb->mHdmiVicCount = hdmiVicLen;
		if (dw_bit_field(data[videoInfoStart], 7, 1) == 1) {/* 3d present */
			vsdb->m3dPresent = true;
			hdmi3dStart = videoInfoStart + hdmiVicLen + 2;
			/* 3d multi 00 -> both 3d_structure_all
			and 3d_mask_15 are NOT present */
			/* 3d mutli 11 -> reserved */
			if (dw_bit_field(data[videoInfoStart], 5, 2) == 1) {
				/* 3d multi 01 */
				/* 3d_structure_all is present but 3d_mask_15 not present */
				for (j = 0; j < 16; j++) {
					/* j spans 3d structures */
					if (dw_bit_field(data[hdmi3dStart
						+ (j / 8)], (j % 8), 1) == 1) {
						for (i = 0; i < 16; i++)
							vsdb->mVideo3dStruct[i][(j < 8)	? j+8 : j - 8] = 1;
					}
				}
				spanned3d = 2;
				/* hdmi3dStart += 2;
				   hdmi3dLen -= 2; */
			} else if (dw_bit_field(data[videoInfoStart], 5, 2) == 2) {
				/* 3d multi 10 */
				/* 3d_structure_all and 3d_mask_15 are present */
				for (j = 0; j < 16; j++) {
					for (i = 0; i < 16; i++) {
						if (dw_bit_field(data[hdmi3dStart + 2 + (i / 8)], (i % 8), 1) == 1)
							vsdb->mVideo3dStruct[(i < 8) ? i + 8 : i - 8][(j < 8) ? j + 8 : j - 8] = dw_bit_field(data[hdmi3dStart + (j / 8)], (j % 8), 1);
					}
				}
				spanned3d = 4;
			}
			if (hdmi3dLen > spanned3d) {
				hdmi3dStart += spanned3d;
				for (i = 0, j = 0; i < (hdmi3dLen - spanned3d); i++) {
					vsdb->mVideo3dStruct[dw_bit_field(data[hdmi3dStart + i + j], 4, 4)][dw_bit_field(data[hdmi3dStart + i + j], 0, 4)] = 1;
					if (dw_bit_field(data[hdmi3dStart + i + j], 4, 4) > 7) {
						j++;
						vsdb->mDetail3d[dw_bit_field(data[hdmi3dStart + i + j], 4, 4)][dw_bit_field(data[hdmi3dStart + i + j], 4, 4)] = dw_bit_field(data[hdmi3dStart + i + j], 4, 4);
					}
				}
			}
		} else {	/* 3d NOT present */
			vsdb->m3dPresent = false;
		}
	}
	vsdb->mValid = true;

	return true;
}

int _parse_data_block_hdmi_forum(dw_edid_hdmi_forum_vs_data_t *hf, u8 *data)
{
	u8 len = 0;

	if (IS_ERR_OR_NULL(hf)) {
		shdmi_err(hf);
		return false;
	}

	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		return false;
	}

	_reset_sink_hdmi_forum(hf);

	len = dw_bit_field(data[0], 0, 5);
	if (len < 7) {
		hdmi_err("dw edid check hf-vsdb len[%d] invalid!\n", len);
		return false;
	}

	_parse_hdmi_forum_info(hf, data);

	return true;
}

int _parse_data_block_speaker_allocation(dw_edid_speaker_allocation_data_t *sadb, u8 *data)
{
	u8 len = 0;

	if (IS_ERR_OR_NULL(sadb)) {
		shdmi_err(sadb);
		return false;
	}

	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		return false;
	}

	_reset_sink_speaker_alloction(sadb);

	len = dw_bit_field(data[0], 0, 5);
	if (len != 0x03) {
		hdmi_err("dw edid check speaker alloction len[%d] invalid!\n", len);
		return false;
	}

	/* TODO: speaker alloction has 3-bytes. but now only save 1-byte. */
	sadb->mValid = true;
	sadb->mByte1 = data[1];

	return true;
}

int _parse_data_block_video_capability(dw_edid_video_capabilit_data_t *vcdb, u8 *data)
{
	u8 len = 0;

	if (IS_ERR_OR_NULL(vcdb)) {
		shdmi_err(vcdb);
		return false;
	}

	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		return false;
	}

	_reset_sink_video_capabilit(vcdb);

	len = dw_bit_field(data[0], 0, 5);
	if (len != 0x02) {
		hdmi_err("dw edid check video capability len[%d] invalid\n", len);
		return false;
	}

	/* so far VCDB is 2 bytes long */
	vcdb->mValid = true;
	vcdb->mCeScanInfo = dw_bit_field(data[2], 0, 2);
	vcdb->mItScanInfo = dw_bit_field(data[2], 2, 2);
	vcdb->mPreferredTimingScanInfo = dw_bit_field(data[2], 4, 2);
	vcdb->mQuantizationRangeSelectable = (dw_bit_field(data[2], 6, 1) == 1);
	return true;
}

int _parse_data_block_colorimetry(dw_edid_colorimetry_data_t *cdb, u8 *data)
{
	u8 len = 0;

	if (IS_ERR_OR_NULL(cdb)) {
		shdmi_err(cdb);
		return false;
	}

	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		return false;
	}

	_reset_sink_colorimetry(cdb);

	len = dw_bit_field(data[0], 0, 5);
	if (len != 0x03) {
		hdmi_err("dw edid check colorimerty len[%d] invalid\n", len);
		return false;
	}

	cdb->mByte3 = data[2];
	cdb->mByte4 = data[3];
	cdb->mValid = true;
	return true;
}

int _parse_data_block_hdr_static_metadata(dw_edid_hdr_static_metadata_data_t *hsmd, u8 *data)
{
	u8 len = 0;

	if (IS_ERR_OR_NULL(hsmd)) {
		shdmi_err(hsmd);
		return false;
	}

	if (IS_ERR_OR_NULL(data)) {
		shdmi_err(data);
		return false;
	}

	_reset_sink_hdr_static_metadata(hsmd);

	len = dw_bit_field(data[0], 0, 5);
	if (len < 0x03) {
		hdmi_err("dw edid check hdr static metadata len[%d] invalid\n", len);
		return false;
	}

	hsmd->et_n = dw_bit_field(data[2], 0, 5);
	hsmd->sm_n = data[3];
	if (len > 3)
		hsmd->dc_max_lum_data = data[4];
	if (len > 4)
		hsmd->dc_max_fa_lum_data = data[5];
	if (len > 5)
		hsmd->dc_min_lum_data = data[6];
	return true;

}

static void _parse_data_block_ycc420_video(struct sink_info_s *sink,
		u8 Ycc420All, u8 LimitedToYcc420All)
{
	u32 edid_cnt = 0;

	for (edid_cnt = 0; edid_cnt < sink->edid_mSvdIndex; edid_cnt++) {
		switch (sink->edid_mSvd[edid_cnt].mCode) {
		case 96:
		case 97:
		case 101:
		case 102:
		case 106:
		case 107:
			if (Ycc420All == 1) {
				sink->edid_mSvd[edid_cnt].mYcc420 = 1;
				hdmi_trace("dw edid set vic[%d] also support 420\n",
						sink->edid_mSvd[edid_cnt].mCode);
			}
			if (LimitedToYcc420All == 1) {
				sink->edid_mSvd[edid_cnt].mLimitedToYcc420 = 1;
				hdmi_trace("dw edid set vic[%d] only support 420\n",
						sink->edid_mSvd[edid_cnt].mCode);
			}
			break;
		}
	}
}

static int _parse_cta_data_block(u8 *data, struct sink_info_s *sink)
{
	dw_edid_block_sad_t tmpSad;
	dw_edid_block_svd_t tmpSvd;
	u8 tmpYcc420All = 0;
	u8 tmpLimitedYcc420All = 0;
	u32 ieeeId = 0;
	int i = 0, ret = 0;
	int svdNr = 0;
	int icnt = 0;
	u8 tag = dw_bit_field(data[0], 5, 3);
	u8 length = dw_bit_field(data[0], 0, 5);

	tmpSvd.mLimitedToYcc420 = 0;
	tmpSvd.mYcc420 = 0;

	switch (tag) {
	case TAG_ADB:
		hdmi_trace("dw edid parse: audio data block\n");
		for (i = 1; i < (length + 1); i += 3) {
			_parse_data_block_short_audio(&tmpSad, data + i);
			if (sink->edid_mSadIndex < ARRAY_SIZE(sink->edid_mSad))
				sink->edid_mSad[sink->edid_mSadIndex++] = tmpSad;
			else
				hdmi_wrn("edid sad %d not save when buffer full\n", i);
		}
		break;
	case TAG_VDB:
		hdmi_trace("dw edid parse: video data block\n");
		for (i = 1; i < (length + 1); i++) {
			_parse_data_block_short_video(&tmpSvd, data[i]);
			if (sink->edid_mSvdIndex < ARRAY_SIZE(sink->edid_mSvd))
				sink->edid_mSvd[sink->edid_mSvdIndex++] = tmpSvd;
			else
				hdmi_wrn("edid svd %d not save when buffer full\n", i);
		}
		break;
	case TAG_VSDB:
		ieeeId = dw_byte_to_dword(0x00, data[3], data[2], data[1]);
		if (ieeeId == HDMI_IEEE_CODE) {
			hdmi_trace("dw edid parse: hdmi14 vendor specific data block");
			ret = _parse_data_block_hdmi14(&sink->edid_mHdmivsdb, data);
			if (ret != true) {
				hdmi_err("dw edid parse hdmi14 vsdb failed!\n");
				break;
			}
		} else if (ieeeId == HDMI_FORUM_IEEE_CODE) {
			hdmi_trace("dw edid parse: hdmi forum vendor specific data block");
			_dw_edid_update_sink_hdmi20(true);
			ret = _parse_data_block_hdmi_forum(&sink->edid_mHdmiForumvsdb, data);
			if (ret != true) {
				hdmi_err("dw edid parse hf-vsdb failed!\n");
				break;
			}
		} else
			hdmi_wrn("dw edid unsupport parse ieee: 0x%X\n", ieeeId);
		break;
	case TAG_SADB:
		hdmi_trace("dw edid parse: speaker allocation data block\n");
		ret = _parse_data_block_speaker_allocation(&sink->edid_mSpeakerAllocationDataBlock, data);
		if (ret != true)
			hdmi_err("dw edid parse speaker allocation failed\n");
		break;
	case TAG_Extend:{
		switch (data[1]) {
		case EXT_TAG_Video_Cap_DB:
			hdmi_trace("dw edid parse: extend video capability data block\n");
			ret = _parse_data_block_video_capability(&sink->edid_mVideoCapabilityDataBlock, data);
			if (ret != true)
				hdmi_err("dw edid parse vsdb failed\n");
			break;
		case EXT_TAG_HDMI_VDB:
			hdmi_inf("dw edid unsupport parse hdmi video data block\n");
			break;
		case EXT_TAG_Colorimetry_DB:
			hdmi_trace("dw edid parse: colorimetry data block\n");
			ret = _parse_data_block_colorimetry(&sink->edid_mColorimetryDataBlock, data);
			if (ret != true)
				hdmi_err("dw edid pase colorimetry failed\n");
			break;
		case EXT_TAG_HDR_SMDB:
			hdmi_trace("dw edid parse: hdr static metadata data block\n");
			ret = _parse_data_block_hdr_static_metadata(&sink->edid_hdr_static_metadata_data_block, data);
			if (ret != true)
				hdmi_err("dw edid parse hdr static metadata failed\n");
			break;
		case EXT_TAG_HDMI_Audio_DB:
			hdmi_inf("dw edid unsupport parse hdmi audio data block\n");
			break;
		case EXT_TAG_YCbCr420_Video_DB:
			hdmi_trace("dw edid parse: ycc420 video data block\n");
			_dw_edid_update_sink_hdmi20(true);
			tmpLimitedYcc420All = (dw_bit_field(data[0], 0, 5) == 1 ? 1 : 0);
			_parse_data_block_ycc420_video(sink, tmpYcc420All, tmpLimitedYcc420All);
			for (i = 0; i < (dw_bit_field(data[0], 0, 5) - 1); i++) {
				/* * Length includes the tag byte */
				tmpSvd.mCode = data[2 + i];
				tmpSvd.mNative = 0;
				tmpSvd.mLimitedToYcc420 = 1;
				if (sink->edid_mSvdIndex < ARRAY_SIZE(sink->edid_mSvd)) {
					sink->edid_mSvd[sink->edid_mSvdIndex] = tmpSvd;
					sink->edid_mSvdIndex++;
				} else
					hdmi_wrn("dw edid sct not save vic[%02d] when buffer full\n",
							tmpSvd.mCode);
			}
			break;
		case EXT_TAG_YCbCr420_Cap_Map_DB:
			_dw_edid_update_sink_hdmi20(true);
			hdmi_trace("dw edid parse: ycc420 capability map data block\n");
			if (dw_bit_field(data[0], 0, 5) > 1) {
				for (i = 0; i < dw_bit_field(data[0], 0, 5) - 1; i++) {
					for (icnt = 0; icnt < 8; icnt++) {
						/* * Lenght includes the tag byte */
						if ((dw_bit_field(data[i + 2], icnt, 1) & 0x01)) {
							svdNr = icnt + (i * 8);
							tmpSvd.mCode = sink->edid_mSvd[svdNr].mCode;
							tmpSvd.mYcc420 = 1;
							sink->edid_mSvd[svdNr] = tmpSvd;
							hdmi_trace(" - update svd[%02d] vic[%03d] support ycc420\n",
									svdNr, tmpSvd.mCode);
						}
					}
				}
			} else {
				tmpYcc420All = (dw_bit_field(data[0], 0, 5) == 1 ? 1 : 0);
				_parse_data_block_ycc420_video(sink, tmpYcc420All, tmpLimitedYcc420All);
				sink->edid_mYcc420Support = true;
			}
			break;
		case EXT_TAG_HF_SCDB:
			hdmi_trace("dw edid parse: hdmi forum sink capability data block\n");
			_dw_edid_update_sink_hdmi20(true);
			_parse_hdmi_forum_info(&sink->edid_mHdmiForumvsdb, data);
			break;
		default:
			hdmi_inf("dw edid unsupport parse extern data block: %d\n", data[1]);
			break;
		}
		break;
	}
	default:
		hdmi_wrn("dw edid unsupport data block: %d\n", tag);
		break;
	}
	return length + 1;
}

static int _edid_parser_block_base(struct edid *edid, struct sink_info_s *sink)
{
	int i;
	dw_dtd_t tmpDtd;
	struct detailed_timing    *desc = NULL;
	struct detailed_non_pixel *data = NULL;

	if (edid->header[0] != 0) {
		hdmi_err("edid base block is unvalid!!!\n");
		return -1;
	}

	/* base info */
	sink->mfg_week = edid->mfg_week;
	sink->mfg_year = edid->mfg_year;

	hdmi_trace("dw edid parse block0 detailed timing:\n");
	for (i = 0; i < 4; i++) {
		desc = &edid->detailed_timings[i];
		if (desc->pixel_clock == 0) {
			data = &desc->data.other_data;
			switch (data->type) {
			case EDID_DETAIL_MONITOR_NAME:
				memset(sink->prod_name, 0x0, sizeof(sink->prod_name));
				memcpy(sink->prod_name, data->data.str.str,
					ARRAY_SIZE(data->data.str.str));
				break;
			default:
				hdmi_inf("dw edid unsupport parser desc type: %d\n", data->type);
				break;
			}
			continue;
		}

		if (_parse_data_block_detailed_timing(&tmpDtd, (u8 *)desc) != true) {
			hdmi_inf("dw edid base block desc timing %d parser failed\n", i);
			continue;
		}
		hdmi_trace(" - [dtd %d] pixel clock: %dKHz, %dx%d%s\n",
			sink->edid_mDtdIndex, (tmpDtd.mPixelClock / 100),
			tmpDtd.mHActive, tmpDtd.mVActive, tmpDtd.mInterlaced ? "I" : "P");
		sink->edid_mDtd[sink->edid_mDtdIndex] = tmpDtd;
		sink->edid_mDtdIndex++;
	}
	return 0;
}

static int _edid_parser_block_cta_861(u8 *buffer, struct sink_info_s *sink)
{
	int i = 0;
	int c = 0;
	dw_dtd_t tmpDtd;
	u8 offset = buffer[2];

	if (buffer[1] < 0x03) {
		hdmi_err("edid cta 861 block version unsupport!\n");
		return -1;
	}

	/* CTA 861 header block parse */
	sink->edid_mYcc422Support     = dw_bit_field(buffer[3],	4, 1) == 1;
	sink->edid_mYcc444Support     = dw_bit_field(buffer[3],	5, 1) == 1;
	sink->edid_mBasicAudioSupport = dw_bit_field(buffer[3], 6, 1) == 1;
	sink->edid_mUnderscanSupport  = dw_bit_field(buffer[3], 7, 1) == 1;

	/* CTA 861 data block parse */
	if (offset != 4) {
		for (i = 4; i < offset; i += _parse_cta_data_block(buffer + i, sink))
			;
	}

	memcpy(sink->detailed_timings, buffer + 84, sizeof(sink->detailed_timings));

	for (i = offset, c = 0; ((i + DTD_SIZE) < (EDID_BLOCK_SIZE - 1)) && c < 6; i += DTD_SIZE, c++) {
		if (_parse_data_block_detailed_timing(&tmpDtd, buffer + i) == true) {
			if (sink->edid_mDtdIndex < ((sizeof(sink->edid_mDtd) / sizeof(dw_dtd_t)))) {
				sink->edid_mDtd[sink->edid_mDtdIndex] = tmpDtd;
				sink->edid_mDtdIndex++;
				continue;
			}
			hdmi_inf("dw edid parse not save dtd %d\n", c);
		}
	}
	return 0;
}

void dw_edid_reset_sink(void)
{
	struct sink_info_s *sink = dw_get_sink();
	u8 i = 0;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return;
	}

	_dw_edid_update_sink_hdmi20(false);

	for (i = 0; i < sizeof(sink->prod_name); i++)
		sink->prod_name[i] = 0;

	sink->edid_mBasicAudioSupport = false;
	sink->edid_mUnderscanSupport  = false;
	sink->edid_mYcc422Support = false;
	sink->edid_mYcc444Support = false;
	sink->edid_mYcc420Support = false;
	sink->edid_mDtdIndex = 0;
	sink->edid_mSadIndex = 0;
	sink->edid_mSvdIndex = 0;

	_reset_sink_hdmi14(&sink->edid_mHdmivsdb);
	_reset_sink_hdmi_forum(&sink->edid_mHdmiForumvsdb);
	_reset_sink_monitor_desc(&sink->edid_mMonitorRangeLimits);
	_reset_sink_video_capabilit(&sink->edid_mVideoCapabilityDataBlock);
	_reset_sink_colorimetry(&sink->edid_mColorimetryDataBlock);
	_reset_sink_hdr_static_metadata(&sink->edid_hdr_static_metadata_data_block);
	_reset_sink_speaker_alloction(&sink->edid_mSpeakerAllocationDataBlock);
}

int dw_edid_parse_info(u8 *data)
{
	struct sink_info_s *sink = dw_get_sink();
	int ret = -1;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		goto parse_exit;
	}

	switch (data[0]) {
	case TAG_BASE_BLOCK:
		ret = _edid_parser_block_base((struct edid *)data, sink);
		goto parse_exit;
	case TAG_CEA_EXT:
		ret = _edid_parser_block_cta_861(data, sink);
		goto parse_exit;
	case TAG_VTB_EXT:
	case TAG_DI_EXT:
	case TAG_LS_EXT:
	case TAG_MI_EXT:
	default:
		hdmi_inf("edid block header 0x%02x not supported\n", data[0]);
	}

parse_exit:
	return ret;
}

int dw_edid_exit(void)
{
	struct sink_info_s *sink = dw_get_sink();

	shdmi_free_point(sink);
	return 0;
}

int dw_edid_init(void)
{
	struct sink_info_s   *sink = NULL;
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();

	sink = kzalloc(sizeof(struct sink_info_s), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}
	hdmi->sink_info = sink;

	memset(sink, 0, sizeof(struct sink_info_s));
	return 0;
}

int dw_sink_support_hdmi20(void)
{
	struct sink_info_s *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return 0;
	}

	return sink->edid_m20Sink;
}

int dw_sink_support_hdmi_vic(u32 code)
{
	struct sink_info_s *sink = dw_get_sink();
	u32 i;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return 0;
	}

	if (code == 0) {
		hdmi_wrn("dw edid check cea code %d is invalid\n", code);
		return 0;
	}

	for (i = 0; (i < 4) && (sink->edid_mHdmivsdb.mHdmiVic[i] != 0); i++) {
		if (sink->edid_mHdmivsdb.mHdmiVic[i] == code)
			return 1;
	}

	return 0;
}

int dw_sink_support_cea_vic(u32 code)
{
	struct sink_info_s *sink = dw_get_sink();
	u32 i;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return 0;
	}

	if (code == 0) {
		hdmi_wrn("dw edid check cea code %d is invalid\n", code);
		return 0;
	}

	for (i = 0; (i < 128) && (sink->edid_mSvd[i].mCode != 0); i++) {
		if (sink->edid_mSvd[i].mCode == code)
			return 1;
	}

	return 0;
}

bool dw_sink_support_scdc(void)
{
	struct sink_info_s   *sink = dw_get_sink();

	return sink->edid_mHdmiForumvsdb.mSCDC_Present;
}

int dw_sink_support_only_yuv420(u32 vic)
{
	struct sink_info_s   *sink = dw_get_sink();
	int i = 0;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return 0x0;
	}

	if (vic == 0x0)
		return 0x0;

	for (i = 0; i < sink->edid_mSvdIndex; i++) {
		if (sink->edid_mSvd[i].mCode == vic) {
			if (sink->edid_mSvd[i].mLimitedToYcc420) {
				return 0x1;
			}
		}
	}

	return 0x0;
}

int dw_sink_support_yuv420(u32 vic)
{
	struct sink_info_s   *sink = dw_get_sink();
	int i = 0;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}

	if (vic == 0)
		return 0x0;

	for (i = 0; i < sink->edid_mSvdIndex; i++) {
		if (sink->edid_mSvd[i].mCode == vic) {
			if (sink->edid_mSvd[i].mLimitedToYcc420 || sink->edid_mSvd[i].mYcc420) {
				hdmi_trace("this vic %d is support 420. limit - %d, all - %d\n",
					vic, sink->edid_mSvd[i].mLimitedToYcc420, sink->edid_mSvd[i].mYcc420);
				return 0x1;
			}
		}
	}

	return 0x0;
}

int dw_sink_support_yuv422(void)
{
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}

	if (sink->edid_mYcc422Support)
		return 0x1;

	return 0x0;
}

int dw_sink_support_yuv444(void)
{
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}

	if (sink->edid_mYcc444Support)
		return 0x1;

	return 0x0;
}

int dw_sink_support_rgb_dc(u8 bits)
{
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}

	switch (bits) {
	case 1: /* 10-bits */
		return sink->edid_mHdmivsdb.mDeepColor30;
	case 2: /* 12-bits */
		return sink->edid_mHdmivsdb.mDeepColor36;
	case 3: /* 16-bits */
		return sink->edid_mHdmivsdb.mDeepColor48;
	default:
		return 0x0;
	}

	return 0x0;
}

int dw_sink_support_yuv444_dc(u8 bits)
{
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}

	if (sink->edid_mHdmivsdb.mDeepColorY444 == 0)
		return 0x0;

	switch (bits) {
	case 1: /* 10-bits */
		return sink->edid_mHdmivsdb.mDeepColor30;
	case 2: /* 12-bits */
		return sink->edid_mHdmivsdb.mDeepColor36;
	case 3: /* 16-bits */
		return sink->edid_mHdmivsdb.mDeepColor48;
	default:
		return 0x0;
	}

	return 0x0;
}

int dw_sink_support_yuv422_dc(u8 bits)
{
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}

	switch (bits) {
	case 1: /* 10-bits */
		return sink->edid_mHdmivsdb.mDeepColor30;
	case 2: /* 12-bits */
		return sink->edid_mHdmivsdb.mDeepColor36;
	case 3: /* 16-bits */
		return sink->edid_mHdmivsdb.mDeepColor48;
	default:
		return 0x0;
	}

	return 0x0;
}

int dw_sink_support_yuv420_dc(u8 bits)
{
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}

	switch (bits) {
	case 1: /* 10-bits */
		return sink->edid_mHdmiForumvsdb.mDC_30bit_420;
	case 2: /* 12-bits */
		return sink->edid_mHdmiForumvsdb.mDC_36bit_420;
	case 3: /* 16-bits */
		return sink->edid_mHdmiForumvsdb.mDC_48bit_420;
	default:
		return 0;
	}

	return 0;
}

int dw_sink_support_sdr(void)
{
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return 0x0;
	}

	if (sink->edid_hdr_static_metadata_data_block.et_n & BIT(0))
		return 0x1;

	return 0x0;
}

int dw_sink_support_hdr10(void)
{
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return 0x0;
	}

	if (sink->edid_hdr_static_metadata_data_block.et_n & BIT(2))
		return 0x1;

	return 0x0;
}

int dw_sink_support_hlg(void)
{
	struct sink_info_s   *sink = dw_get_sink();

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return 0x0;
	}

	if (sink->edid_hdr_static_metadata_data_block.et_n & BIT(3))
		return 0x1;

	return 0x0;
}

int dw_sink_support_max_tmdsclk(u32 clk)
{
	struct sink_info_s   *sink = dw_get_sink();
	u32 max_rate = 300;

	if (IS_ERR_OR_NULL(sink)) {
		shdmi_err(sink);
		return -1;
	}

	if (dw_sink_support_hdmi20()) {
		if (sink->edid_mHdmiForumvsdb.mMaxTmdsCharRate)
			max_rate = sink->edid_mHdmiForumvsdb.mMaxTmdsCharRate * 5;
		else
			max_rate = 600;
	}

	if (max_rate > (clk / 1000))
		return 0x1;

	hdmi_trace("dw edid check max rate: %d, use rate: %d, check rate: %d\n",
		sink->edid_mHdmiForumvsdb.mMaxTmdsCharRate, max_rate, clk / 1000);
	return 0x0;
}

ssize_t dw_sink_dump(char *buf)
{
	struct sink_info_s   *sink = dw_get_sink();
	ssize_t n = 0;
	int i = 0;

	n += sprintf(buf + n, "[sink]\n");
	n += sprintf(buf + n, " - date    : %d-%02d\n",
			(sink->mfg_year + 1990), (sink->mfg_week));
	n += sprintf(buf + n, " - name    : %s", sink->prod_name);
	n += sprintf(buf + n, " - version : %s\n",
			dw_sink_support_hdmi20() ? "2.0" : "1.4");
	n += sprintf(buf + n, " - dtd timing\n");
	for (i = 0; i < sink->edid_mDtdIndex; i++)
		n += sprintf(buf + n, "    [%d]: %dx%d%s\n", i,
				sink->edid_mDtd[i].mHActive,
				sink->edid_mDtd[i].mVActive,
				sink->edid_mDtd[i].mInterlaced ? "i" : "p");

	n += sprintf(buf + n, " - svd timing\n");
	for (i = 0; i < sink->edid_mSvdIndex; i++)
		n += sprintf(buf + n, "    svd[%d]: vic[%02d], OnlyYcc420[%d], Ycc420[%d]\n",
				i, sink->edid_mSvd[i].mCode,
				sink->edid_mSvd[i].mLimitedToYcc420,
				sink->edid_mSvd[i].mYcc420);

	n += sprintf(buf + n, "[hfvsdb]\n");
	n += sprintf(buf + n, " - scdc    : %s\n",
			sink->edid_mHdmiForumvsdb.mSCDC_Present ? "support" : "un-support");
	n += sprintf(buf + n, " - max rate: %dMhz\n",
			sink->edid_mHdmiForumvsdb.mMaxTmdsCharRate * 5);
	if (sink->edid_mHdmiForumvsdb.mDC_30bit_420)
		n += sprintf(buf + n, " - dc-10bits\n");
	if (sink->edid_mHdmiForumvsdb.mDC_36bit_420)
		n += sprintf(buf + n, " - dc-12bits\n");
	if (sink->edid_mHdmiForumvsdb.mDC_48bit_420)
		n += sprintf(buf + n, " - dc-16bits\n");

	n += sprintf(buf + n, "\n");

	return n;
}

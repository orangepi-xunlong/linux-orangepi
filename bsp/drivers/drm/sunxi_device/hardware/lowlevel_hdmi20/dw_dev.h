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
#ifndef _DW_DEV_H_
#define _DW_DEV_H_

#include <drm/drm_print.h>
#include <drm/drm_edid.h>
#include <linux/i2c.h>

#define DW_HDMI_ENABLE         (1)
#define DW_HDMI_DISABLE        (0)

#define DW_EDID_MAC_HDMI_VIC		16
#define DW_EDID_MAX_HDMI_3DSTRUCT	16
#define DW_EDID_MAX_VIC_WITH_3D		16

enum sunxi_platform_version {
	HDMI_SUN8I_W20_P1 = 0,
	HDMI_SUN50I_W9_P1,
	HDMI_SUN55I_W3_P1,
	HDMI_SUN60I_W2_P1,
};

typedef enum {
	DW_PHY_ACCESS_NULL = 0,
	DW_PHY_ACCESS_I2C  = 1,
	DW_PHY_ACCESS_JTAG = 2
} dw_phy_access_t;

typedef enum {
	DW_AUDIO_INTERFACE_NULL  = -1,
	DW_AUDIO_INTERFACE_I2S   = 0,
	DW_AUDIO_INTERFACE_SPDIF = 1,
	DW_AUDIO_INTERFACE_HBR   = 2,
	DW_AUDIO_INTERFACE_GPA   = 3,
	DW_AUDIO_INTERFACE_DMA   = 4,
	DW_AUDIO_INTERFACE_MAX
} dw_aud_interface_t;

typedef enum {
	DW_AUD_CODING_NULL               = -1,
	DW_AUD_CODING_PCM                = 1,
	DW_AUD_CODING_AC3                = 2,
	DW_AUD_CODING_MPEG1              = 3,
	DW_AUD_CODING_MP3                = 4,
	DW_AUD_CODING_MPEG2              = 5,
	DW_AUD_CODING_AAC                = 6,
	DW_AUD_CODING_DTS                = 7,
	DW_AUD_CODING_ATRAC              = 8,
	DW_AUD_CODING_ONE_BIT_AUDIO      = 9,
	DW_AUD_CODING_DOLBY_DIGITAL_PLUS = 10,
	DW_AUD_CODING_DTS_HD             = 11,
	DW_AUD_CODING_MAT                = 12,
	DW_AUD_CODING_DST                = 13,
	DW_AUD_CODING_WMAPRO             = 14
} dw_aud_coding_t;

typedef enum {
	DW_TMDS_MODE_NULL = -1,
	DW_TMDS_MODE_DVI  = 0,
	DW_TMDS_MODE_HDMI = 1
} dw_tmds_mode_t;

typedef enum {
	DW_HW_COLOR_DEPTH_8    = 4,
	DW_HW_COLOR_DEPTH_10   = 5,
	DW_HW_COLOR_DEPTH_12   = 6,
	DW_HW_COLOR_DEPTH_16   = 7
} dw_hw_color_depth_t;

typedef enum {
	DW_COLOR_DEPTH_NULL = 0,
	DW_COLOR_DEPTH_8    = 8,
	DW_COLOR_DEPTH_10   = 10,
	DW_COLOR_DEPTH_12   = 12,
	DW_COLOR_DEPTH_16   = 16
} dw_color_depth_t;

typedef enum {
	DW_PIXEL_REPETITION_OFF = 0,
	DW_PIXEL_REPETITION_1   = 1,
	DW_PIXEL_REPETITION_2   = 2,
	DW_PIXEL_REPETITION_3   = 3,
	DW_PIXEL_REPETITION_4   = 4,
	DW_PIXEL_REPETITION_5   = 5,
	DW_PIXEL_REPETITION_6   = 6,
	DW_PIXEL_REPETITION_7   = 7,
	DW_PIXEL_REPETITION_8   = 8,
	DW_PIXEL_REPETITION_9   = 9,
	DW_PIXEL_REPETITION_10  = 10
} dw_pixel_repetition_t;

typedef enum {
	DW_PHY_OPMODE_HDMI14           = 1,
	DW_PHY_OPMODE_HDMI_20          = 2,
	DW_PHY_OPMODE_MHL_24           = 3,
	DW_PHY_OPMODE_MHL_PACKEDPIXEL  = 4
} dw_phy_operation_mode_t;

typedef enum {
	DW_COLOR_FORMAT_NULL   = -1,
	DW_COLOR_FORMAT_RGB    = 0,
	DW_COLOR_FORMAT_YCC444 = 1,
	DW_COLOR_FORMAT_YCC422 = 2,
	DW_COLOR_FORMAT_YCC420 = 3
} dw_color_format_t;

typedef enum {
	DW_METRY_NULL     = 0,
	DW_METRY_ITU601   = 1,
	DW_METRY_ITU709   = 2,
	DW_METRY_EXTENDED = 3
} dw_colorimetry_t;

typedef enum {
	DW_METRY_EXT_XV_YCC601         = 0,
	DW_METRY_EXT_XV_YCC709         = 1,
	DW_METRY_EXT_S_YCC601          = 2,
	DW_METRY_EXT_ADOBE_YCC601      = 3,
	DW_METRY_EXT_ADOBE_RGB         = 4,
	DW_METRY_EXT_BT2020_Yc_Cbc_Crc = 5,
	DW_METRY_EXT_BT2020_Y_CB_CR    = 6
} dw_ext_colorimetry_t;

typedef enum {
	DW_EOTF_SDR       = 0,
	DW_EOTF_HDR       = 1,
	DW_EOTF_SMPTE2084 = 2,
	DW_EOTF_HLG       = 3
} dw_eotf_t;

enum dw_range_type_e {
	DW_RGB_RANGE_DEFAULT = 0,
	DW_RGB_RANGE_LIMIT   = 1,
	DW_RGB_RANGE_FULL    = 2
};

enum dw_video_format_e {
	DW_VIDEO_FORMAT_NONE = 0,
	DW_VIDEO_FORMAT_HDMI14_4K = 1,
	DW_VIDEO_FORMAT_3D = 2
};

enum dw_hdcp_type_e {
	DW_HDCP_TYPE_NULL   = -1,
	DW_HDCP_TYPE_HDCP14 = 0,
	DW_HDCP_TYPE_HDCP22 = 1
};

/**
 * @desc: dw hdcp state
 */
enum dw_hdcp_state_e {
	DW_HDCP_DISABLE   = 0,
	DW_HDCP_ING       = 1,
	DW_HDCP_FAILED    = 2,
	DW_HDCP_SUCCESS   = 3,
};

/**
 * @file
 * For detailed handling of this structure,
 * refer to documentation of the functions
 */
typedef struct {
	/* vic code */
	u32 mCode;

	/* * Identifies modes that ONLY can be displayed in YCC 4:2:0 */
	u8 mLimitedToYcc420;

	/* * Identifies modes that can also be displayed in YCC 4:2:0 */
	u8 mYcc420;

	u16 mPixelRepetitionInput;

	/* * In units of 1KHz */
	u32 mPixelClock;

	/* * 1 for interlaced, 0 progressive */
	u8 mInterlaced;

	u16 mHActive;

	u16 mHBlanking;

	u16 mHBorder;

	u16 mHImageSize; /* For picture aspect ratio */

	u16 mHSyncOffset;

	u16 mHSyncPulseWidth;

	/* * 0 for Active low, 1 active high */
	u8 mHSyncPolarity;

	u16 mVActive;

	u16 mVBlanking;

	u16 mVBorder;

	u16 mVImageSize; /* For picture aspect ratio */

	u16 mVSyncOffset;

	u16 mVSyncPulseWidth;

	/* * 0 for Active low, 1 active high */
	u8 mVSyncPolarity;

} dw_dtd_t;

/**
 * For detailed handling of this structure,
 * refer to documentation of the functions
 */
struct dw_audio_s {
	dw_aud_interface_t mInterfaceType;

	dw_aud_coding_t mCodingType;

	u8 mChannelNum;

	u8 mChannelAllocation;

	u8 mSampleSize;

	u32 mSamplingFrequency;

	u8 mLevelShiftValue;

	u8 mDownMixInhibitFlag;

	u8 mIecCopyright;

	u8 mIecCgmsA; /* * IEC CGMS-A */

	u8 mIecPcmMode;	/* * IEC PCM mode */

	u8 mIecCategoryCode; /* * IEC category code */

	u8 mIecSourceNumber; /* * IEC source number */

	u8 mIecClockAccuracy; /* * IEC clock accuracy */

	u16 mClockFsFactor; /** Input audio clock Fs factor used at the audio
						packetizer to calculate the CTS value and ACR packet
						insertion rate */

	u8 mDmaThreshold; /** When the number of samples in the Audio FIFO is lower
						than the threshold, the DMA engine requests a new burst
						request to the AHB master interface */

	u8 mDmaHlock; /* * Master burst lock mechanism */
};

typedef struct dw_fc_drm_pb {
	u8 eotf;
	u8 metadata;
	u16 r_x;
	u16 r_y;
	u16 g_x;
	u16 g_y;
	u16 b_x;
	u16 b_y;
	u16 w_x;
	u16 w_y;
	u16 luma_max;
	u16 luma_min;
	u16 mcll;
	u16 mfll;
} dw_fc_drm_pb_t;

struct dw_video_s {
	u32 update;
	dw_tmds_mode_t mHdmi;
	u8 mCea_code;
	u8 mHdmi_code;
	u8 mHdr;
	dw_fc_drm_pb_t *pb;
	dw_fc_drm_pb_t *dynamic_pb;
	dw_color_format_t mEncodingOut;
	dw_color_format_t mEncodingIn;
	u8 mColorResolution; /* color depth */
	u8 mPixelRepetitionFactor; /* For packetizer pixel repeater */
	dw_dtd_t mDtd;
	u8 mRgbQuantizationRange;
	u8 mPixelPackingDefaultPhase;
	u8 mColorimetry;
	u8 mScanInfo;
	u8 mActiveFormatAspectRatio;
	u8 mNonUniformScaling;
	dw_ext_colorimetry_t mExtColorimetry;
	u8 mColorimetryDataBlock;
	u8 mItContent;
	u16 mEndTopBar;
	u16 mStartBottomBar;
	u16 mEndLeftBar;
	u16 mStartRightBar;
	u16 mCscFilter;
	u16 mCscA[4];
	u16 mCscC[4];
	u16 mCscB[4];
	u16 mCscScale;
	u8 mHdmiVideoFormat;/* 0:There's not 4k*2k or not 3D  1:4k*2k  2:3D */
	u8 m3dStructure; /* packing frame and so on */
	u8 m3dExtData;/* 3d extra structure, if 3d_structure=0x1000,there must be a 3d extra structure */
	u8 mHdmiVic;
	u8 mHdmi20;/* decided by sink */
	u8 scdc_ability;
	u32 rate;
};

struct dw_hdcp_s {
	int enable_type;

	/* hdcp1x value */
	int hdcp1x_auth_done;
	int hdcp1x_auth_state;
	int hdcp1x_encry_delay;
	int hdcp1x_encrying;

	/* hdcp2x value */
	int esm_loading;
	u32 esm_size;
	int esm_open;
	int esm_auth_done;
	int esm_auth_state;
	int esm_cap_done;
	u8 *esm_pairdata;
	struct mutex lock_esm_auth;
	struct mutex lock_esm_handler;
	struct work_struct        hdcp2x_work;
	struct workqueue_struct  *hdcp2x_workqueue;

	unsigned long esm_hpi_addr;
	u32 esm_fw_size;
	dma_addr_t esm_fw_addr;
	unsigned long esm_fw_addr_vir;
	u32 esm_data_size;
	dma_addr_t esm_data_addr;
	unsigned long esm_data_addr_vir;
};

/** For detailed handling of this structure,
refer to documentation of the functions */
struct dw_product_s {
	/* Vendor Name of eight 7-bit ASCII characters */
	u8 mVendorName[8];

	u8 mVendorNameLength;

	/* Product name or description,
	consists of sixteen 7-bit ASCII characters */
	u8 mProductName[16];

	u8 mProductNameLength;

	/* Code that classifies the source device (CEA Table 15) */
	u8 mSourceType;

	/* oui 24 bit IEEE Registration Identifier */
	u32 mOUI;

	u8 mVendorPayload[24];

	u8 mVendorPayloadLength;
};

/**
 * @file
 * Short Video Descriptor.
 * Parse and hold Short Video Descriptors found in Video Data Block in EDID.
 */
/** For detailed handling of this structure,
	refer to documentation of the functions */
typedef struct {
	int	mNative;

	unsigned int mCode;

	unsigned int mLimitedToYcc420;

	unsigned int mYcc420;

} dw_edid_block_svd_t;

/**
 * @file
 * Short Audio Descriptor.
 * Found in Audio Data Block (dw_edid_block_sad_t *sad, CEA Data Block Tage Code 1)
 * Parse and hold information from EDID data structure
 */
/** For detailed handling of this structure, refer to documentation
	of the functions */
typedef struct {
	u8 mFormat;

	u8 mMaxChannels;

	u8 mSampleRates;

	u8 mByte3;
} dw_edid_block_sad_t;

/* For detailed handling of this structure,
	refer to documentation of the functions */
typedef struct {
	u16 mPhysicalAddress; /* physical address for cec */

	int mSupportsAi; /* Support ACP ISRC1 ISRC2 packets */

	int mDeepColor30;

	int mDeepColor36;

	int mDeepColor48;

	int mDeepColorY444;

	int mDviDual; /* Support DVI dual-link operation */

	u16 mMaxTmdsClk;

	u16 mVideoLatency;

	u16 mAudioLatency;

	u16 mInterlacedVideoLatency;

	u16 mInterlacedAudioLatency;

	u32 mId;
	/* Sink Support for some particular content types */
	u8 mContentTypeSupport;

	u8 mImageSize; /* for picture espect ratio */

	int mHdmiVicCount;

	u8 mHdmiVic[DW_EDID_MAC_HDMI_VIC];

	int m3dPresent;
	/* row index is the VIC number */
	int mVideo3dStruct[DW_EDID_MAX_VIC_WITH_3D][DW_EDID_MAX_HDMI_3DSTRUCT];
	/* row index is the VIC number */
	int mDetail3d[DW_EDID_MAX_VIC_WITH_3D][DW_EDID_MAX_HDMI_3DSTRUCT];

	int mValid;

} dw_edid_hdmi_vs_data_t;

/* HDMI 2.0 HF_VSDB */
typedef struct {
	u32 mIeee_Oui;
	u8 mValid;
	u8 mVersion;
	u8 mMaxTmdsCharRate;
	u8 m3D_OSD_Disparity;
	u8 mDualView;
	u8 mIndependentView;
	u8 mLTS_340Mcs_scramble;
	u8 mRR_Capable;
	u8 mSCDC_Present;
	u8 mDC_30bit_420;
	u8 mDC_36bit_420;
	u8 mDC_48bit_420;
} dw_edid_hdmi_forum_vs_data_t;

/**
 * @file
 * Second Monitor Descriptor
 * Parse and hold Monitor Range Limits information read from EDID
 */
typedef struct {
	u8 mMinVerticalRate;
	u8 mMaxVerticalRate;
	u8 mMinHorizontalRate;
	u8 mMaxHorizontalRate;
	u8 mMaxPixelClock;
	int mValid;
} dw_edid_monitor_descriptior_data_t;

/**
 * @file
 * Video Capability Data Block.
 * (dw_edid_video_capabilit_data_t * vcdbCEA Data Block Tag Code 0).
 * Parse and hold information from EDID data structure.
 * For detailed handling of this structure,
 * refer to documentation of the functions
 */

typedef struct {
	int mQuantizationRangeSelectable;
	u8 mPreferredTimingScanInfo;
	u8 mItScanInfo;
	u8 mCeScanInfo;
	int mValid;
} dw_edid_video_capabilit_data_t;

/**
 * @file
 * Colorimetry Data Block class.
 * Holds and parses the Colorimetry data-block information.
 */
typedef struct {
	u8  mByte3;
	u8  mByte4;
	int mValid;
} dw_edid_colorimetry_data_t;

typedef struct {
	u8 et_n;
	u8 sm_n;

	/* Desired Content Max Luminance data */
	u8 dc_max_lum_data;

	/* Desired Content Max Frame-average Luminance data */
	u8 dc_max_fa_lum_data;

	/* Desired Content Min Luminance data */
	u8 dc_min_lum_data;
} dw_edid_hdr_static_metadata_data_t;

/**
 * @file
 * SpeakerAllocation Data Block.
 * Holds and parse the Speaker Allocation data block information.
 * For detailed handling of this structure,
 * refer to documentation of the functions
 */
typedef struct {
	u8 mByte1;
	int mValid;
} dw_edid_speaker_allocation_data_t;

struct sink_info_s{
	/* base block info */
	u8 mfg_week;
	u8 mfg_year;
	char prod_name[30];

	/* Array to hold all the parsed Detailed Timing Descriptors. */
	dw_dtd_t edid_mDtd[32];
	unsigned int edid_mDtdIndex;

	/* array to hold all the parsed Short Video Descriptors. */
	dw_edid_block_svd_t edid_mSvd[128];
	unsigned int edid_mSvdIndex;

	/* array to hold all the parsed Short Audio Descriptors. */
	dw_edid_block_sad_t edid_mSad[128];
	unsigned int edid_mSadIndex;

	int edid_mYcc444Support;
	int edid_mYcc422Support;
	int edid_mYcc420Support;

	int edid_mBasicAudioSupport;
	int edid_mUnderscanSupport;

	/* If Sink is HDMI 2.0 */
	int edid_m20Sink;
	dw_edid_hdmi_vs_data_t             edid_mHdmivsdb;
	dw_edid_hdmi_forum_vs_data_t       edid_mHdmiForumvsdb;
	dw_edid_monitor_descriptior_data_t edid_mMonitorRangeLimits;
	dw_edid_video_capabilit_data_t     edid_mVideoCapabilityDataBlock;
	dw_edid_colorimetry_data_t         edid_mColorimetryDataBlock;
	dw_edid_hdr_static_metadata_data_t edid_hdr_static_metadata_data_block;
	dw_edid_speaker_allocation_data_t  edid_mSpeakerAllocationDataBlock;
	int hf_eeodb_block_count; /* HF-EEODB */

	/* detailed discriptor */
	struct detailed_timing detailed_timings[2];
};

struct dw_i2cm_s {
	int     mode;	/* 0: standard mode, 1: fast mode */
	u8		slave_reg;
	bool	is_regaddr;
	bool	is_segment;
	u16     sfrClock;
	u16     ss_low_ckl;
	u16     ss_high_ckl;
};

struct dw_phy_ops_s {
	int (*phy_init)(void);
	int (*phy_resume)(void);
	int (*phy_reset)(void);
	int (*phy_disconfig)(void);
	int (*phy_config)(void);
	int (*phy_read)(u8 addr, void *data);
	int (*phy_write)(u8 addr, void *data);
	ssize_t (*phy_dump)(char *buf);
};

/**
 * @short Main structures to instantiate the driver
 */
struct dw_hdmi_dev_s {
	struct drm_connector   *connect;
	struct i2c_adapter     *i2c_adap;
	struct device          *dev;

	struct dw_phy_ops_s   *phy_ext;
	struct sink_info_s    *sink_info;

	uintptr_t   addr;
	u32         pixel_clk;
	u32         tmds_clk;
	u8          pixel_repeat;
	u8          color_bits;
	u8          hdmi_on;
	u8          audio_on;
	u8			clock_src; /* 0:phypll, 1:ccmu */
	int   		plat_id;
	int 		sw_init;

	struct mutex  aud_lock_params;

	struct dw_hdcp_s		hdcp_dev;
	struct dw_video_s		video_dev;
	struct dw_audio_s		audio_dev;
	struct dw_product_s		prod_dev;
	struct dw_i2cm_s		i2cm_dev;
};

/*****************************************************
 * sunxi hdmi print macro
 * @desc: support uart print and hdmi log buffer
 *        support keywords sunxi-hdmi
 *****************************************************/
#ifdef hdmi_inf
#undef hdmi_inf
#endif
#define hdmi_inf(fmt, args...)                    \
	do {                                          \
		DRM_INFO("sunxi-hdmi: "fmt, ##args);      \
	} while (0)

#ifdef hdmi_wrn
#undef hdmi_wrn
#endif
#define hdmi_wrn(fmt, args...)                      \
	do {                                            \
		DRM_INFO("sunxi-hdmi: [warn] "fmt, ##args); \
	} while (0)

#ifdef hdmi_err
#undef hdmi_err
#endif
#define hdmi_err(fmt, args...)                       \
	do {                                             \
		DRM_INFO("sunxi-hdmi: [error] "fmt, ##args); \
	} while (0)

#ifdef hdcp_log
#undef hdcp_log
#endif
#define hdcp_log(fmt, args...)                          \
	do {                                                \
		if (dw_hdmi_get_loglevel())  \
			DRM_INFO("sunxi-hdmi: [hdcp] "fmt, ##args); \
	} while (0)

#ifdef hdmi_trace
#undef hdmi_trace
#endif
#define hdmi_trace(fmt, args...)                        \
	do {                                                \
		if (dw_hdmi_get_loglevel()) \
			DRM_INFO("sunxi-hdmi: "fmt, ##args);        \
	} while (0)

#ifdef shdmi_err
#undef shdmi_err
#endif
#define shdmi_err(name)                                     \
	do {                                                    \
		hdmi_err("check point %s is err or null\n", #name); \
		dump_stack();                                       \
	} while (0)

#ifdef shdmi_free_point
#undef shdmi_free_point
#endif
#define shdmi_free_point(x)   \
	do {                      \
		if (x != NULL) {      \
			kfree(x);         \
			x = NULL;         \
		}                     \
	} while (0)

/**
 * @desc: get byte from dw index
 * @data: u32 data
 * @index: need get byte index. 0,1,2,3
 * @return: index to byte
 */
u8 dw_to_byte(u32 data, u8 index);
/**
 * Concatenate two parts of two 8-bit bytes into a new 16-bit word
 * @param bHi first byte
 * @param oHi shift part of first byte (to be place as most significant
 * bits)
 * @param nHi width part of first byte (to be place as most significant
 * bits)
 * @param bLo second byte
 * @param oLo shift part of second byte (to be place as least
 * significant bits)
 * @param nLo width part of second byte (to be place as least
 * significant bits)
 * @returns 16-bit concatenated word as part of the first byte and part of
 * the second byte
 */
u16 dw_concat_bits(u8 bHi, u8 oHi, u8 nHi, u8 bLo, u8 oLo, u8 nLo);

/** Concatenate two full bytes into a new 16-bit word
 * @param hi
 * @param lo
 * @returns hi as most significant bytes concatenated with lo as least
 * significant bits.
 */
u16 dw_byte_to_word(const u8 hi, const u8 lo);

/** Extract the content of a certain part of a byte
 * @param data 8bit byte
 * @param shift shift from the start of the bit (0)
 * @param width width of the desired part starting from shift
 * @returns an 8bit byte holding only the desired part of the passed on
 * data byte
 */
u8 dw_bit_field(const u16 data, u8 shift, u8 width);

/** Concatenate four 8-bit bytes into a new 32-bit word
 * @param b3 assumed as most significant 8-bit byte
 * @param b2
 * @param b1
 * @param b0 assumed as least significant 8bit byte
 * @returns a 2D word, 32bits, composed of the 4 passed on parameters
 */
u32 dw_byte_to_dword(u8 b3, u8 b2, u8 b1, u8 b0);

/**
 * Read the contents of a register
 * @param addr of the register
 * @return 8bit byte containing the contents
 */
u32 dw_read(u32 addr);

/**
 * Read several bits from a register
 * @param addr of the register
 * @param shift of the bit from the beginning
 * @param width or number of bits to read
 * @return the contents of the specified bits
 */
u32 dw_read_mask(u32 addr, u8 mask);

/**
 * Write a byte to a register
 * @param data to be written to the register
 * @param addr of the register
 */
void dw_write(u32 addr, u32 data);

/**
 * Write to several bits in a register
 *
 * @param data to be written to the required part
 * @param addr of the register
 * @param shift of the bits from the beginning
 * @param width or number of bits to written to
 */
void dw_write_mask(u32 addr, u8 mask, u8 data);
/**
 * @desc: dw hdmi get log level
 * @return: log level
 */
u8 dw_hdmi_get_loglevel(void);
/**
 * @desc: dw hdmi set log level
 * @level: log level
 */
void dw_hdmi_set_loglevel(u8 level);
/**
 * @desc: dw hdmi control params reset
 * @return: 0 - success
 */
int dw_hdmi_ctrl_reset(void);
/**
 * @desc: dw hdmi control params update by video params
 * @return: 0 - success
 */
int dw_hdmi_ctrl_update(void);
/**
 * @desc: dw hdmi scdc set scramble state
 * @setup: 1 - enable sink scramble
 *         0 - disable sink scramble
 * @return: 0 - success
 */
int dw_hdmi_scdc_set_scramble(u8 setup);
/**
 * @desc: dw hdmi device init
 * @data: point to hdmi data
 * @return: 0 - success
 */
int dw_hdmi_init(struct dw_hdmi_dev_s *data);
/**
 * @desc: dw hdmi device exit
 * @return: 0 - success
 */
int dw_hdmi_exit(void);
/**
 * @desc: dw hdmi info dump
 * @buf: point to dump buffer
 * @return: dump buffer size
 */
ssize_t dw_hdmi_dump(char *buf);
/**
 * @desc: dw hdmi get
 * @return: point to dw hdmi device
 */
struct dw_hdmi_dev_s *dw_get_hdmi(void);

#endif /* _DW_DEV_H_ */

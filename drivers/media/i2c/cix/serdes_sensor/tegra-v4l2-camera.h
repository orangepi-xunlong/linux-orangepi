/* SPDX-License-Identifier: GPL-2.0 */
 /*
  * Copyright 2025 Cix Technology Group Co., Ltd.
  *
  */
#ifndef __TEGRA_V4L2_CAMERA__
#define __TEGRA_V4L2_CAMERA__

#include <linux/v4l2-controls.h>

#define TEGRA_CAMERA_CID_BASE	(V4L2_CTRL_CLASS_CAMERA | 0x2000)

#define TEGRA_CAMERA_CID_FRAME_LENGTH		(TEGRA_CAMERA_CID_BASE+0)
#define TEGRA_CAMERA_CID_COARSE_TIME		(TEGRA_CAMERA_CID_BASE+1)
#define TEGRA_CAMERA_CID_COARSE_TIME_SHORT	(TEGRA_CAMERA_CID_BASE+2)
#define TEGRA_CAMERA_CID_GROUP_HOLD		(TEGRA_CAMERA_CID_BASE+3)
#define TEGRA_CAMERA_CID_HDR_EN			(TEGRA_CAMERA_CID_BASE+4)
#define TEGRA_CAMERA_CID_EEPROM_DATA		(TEGRA_CAMERA_CID_BASE+5)
#define TEGRA_CAMERA_CID_OTP_DATA		(TEGRA_CAMERA_CID_BASE+6)
#define TEGRA_CAMERA_CID_FUSE_ID		(TEGRA_CAMERA_CID_BASE+7)
#define TEGRA_CAMERA_CID_SENSOR_MODE_ID		(TEGRA_CAMERA_CID_BASE+8)

#define TEGRA_CAMERA_CID_GAIN			(TEGRA_CAMERA_CID_BASE+9)
#define TEGRA_CAMERA_CID_EXPOSURE		(TEGRA_CAMERA_CID_BASE+10)
#define TEGRA_CAMERA_CID_FRAME_RATE		(TEGRA_CAMERA_CID_BASE+11)
#define TEGRA_CAMERA_CID_EXPOSURE_SHORT		(TEGRA_CAMERA_CID_BASE+12)
#define TEGRA_CAMERA_CID_STEREO_EEPROM		(TEGRA_CAMERA_CID_BASE+13)
#define TEGRA_CAMERA_CID_ALTERNATING_EXPOSURE	(TEGRA_CAMERA_CID_BASE+14)

#define TEGRA_CAMERA_CID_SENSOR_CONFIG		(TEGRA_CAMERA_CID_BASE+50)
#define TEGRA_CAMERA_CID_SENSOR_MODE_BLOB	(TEGRA_CAMERA_CID_BASE+51)
#define TEGRA_CAMERA_CID_SENSOR_CONTROL_BLOB	(TEGRA_CAMERA_CID_BASE+52)

#define TEGRA_CAMERA_CID_GAIN_TPG		(TEGRA_CAMERA_CID_BASE+70)
#define TEGRA_CAMERA_CID_GAIN_TPG_EMB_DATA_CFG	(TEGRA_CAMERA_CID_BASE+71)

#define TEGRA_CAMERA_CID_VI_BYPASS_MODE		(TEGRA_CAMERA_CID_BASE+100)
#define TEGRA_CAMERA_CID_OVERRIDE_ENABLE	(TEGRA_CAMERA_CID_BASE+101)
#define TEGRA_CAMERA_CID_VI_HEIGHT_ALIGN	(TEGRA_CAMERA_CID_BASE+102)
#define TEGRA_CAMERA_CID_VI_SIZE_ALIGN		(TEGRA_CAMERA_CID_BASE+103)
#define TEGRA_CAMERA_CID_WRITE_ISPFORMAT	(TEGRA_CAMERA_CID_BASE+104)

#define TEGRA_CAMERA_CID_SENSOR_SIGNAL_PROPERTIES  (TEGRA_CAMERA_CID_BASE+105)
#define TEGRA_CAMERA_CID_SENSOR_IMAGE_PROPERTIES   (TEGRA_CAMERA_CID_BASE+106)
#define TEGRA_CAMERA_CID_SENSOR_CONTROL_PROPERTIES (TEGRA_CAMERA_CID_BASE+107)
#define TEGRA_CAMERA_CID_SENSOR_DV_TIMINGS         (TEGRA_CAMERA_CID_BASE+108)
#define TEGRA_CAMERA_CID_LOW_LATENCY         (TEGRA_CAMERA_CID_BASE+109)
#define TEGRA_CAMERA_CID_VI_PREFERRED_STRIDE (TEGRA_CAMERA_CID_BASE+110)
#define TEGRA_CAMERA_CID_VI_CAPTURE_TIMEOUT (TEGRA_CAMERA_CID_BASE+111)

/**
 * This is temporary with the current v4l2 infrastructure
 * currently discussing with upstream maintainers our proposals and
 * better approaches to resolve this
 */
#define TEGRA_CAMERA_CID_SENSOR_MODES		(TEGRA_CAMERA_CID_BASE + 130)

#define MAX_BUFFER_SIZE			32
#define MAX_CID_CONTROLS		32
#define MAX_NUM_SENSOR_MODES		30
#define OF_MAX_STR_LEN			256
#define OF_SENSORMODE_PREFIX ("mode")

/*
 * Scaling factor for converting a Q10.22 fixed point value
 * back to its original floating point value
 */
#define FIXED_POINT_SCALING_FACTOR (1ULL << 22)

#define TEGRA_CAM_MAX_STRING_CONTROLS 8
#define TEGRA_CAM_STRING_CTRL_EEPROM_INDEX 0
#define TEGRA_CAM_STRING_CTRL_FUSEID_INDEX 1
#define TEGRA_CAM_STRING_CTRL_OTP_INDEX 2

#define TEGRA_CAM_MAX_COMPOUND_CONTROLS 4
#define TEGRA_CAM_COMPOUND_CTRL_EEPROM_INDEX 0
#define TEGRA_CAM_COMPOUND_CTRL_ALT_EXP_INDEX 1

#define	CSI_PHY_MODE_DPHY	0
#define	CSI_PHY_MODE_CPHY	1
#define	SLVS_EC			2

struct unpackedU64 {
	__u32 high;
	__u32 low;
};

union __u64val {
	struct unpackedU64 unpacked;
	__u64 val;
};

struct sensor_signal_properties {
	__u32 readout_orientation;
	__u32 num_lanes;
	__u32 mclk_freq;
	union __u64val pixel_clock;
	__u32 cil_settletime;
	__u32 lane_polarity;
	__u32 discontinuous_clk;
	__u32 dpcm_enable;
	__u32 tegra_sinterface;
	__u32 phy_mode;
	__u32 deskew_initial_enable;
	__u32 deskew_periodic_enable;
	union __u64val serdes_pixel_clock;
	union __u64val mipi_clock;
};

struct sensor_image_properties {
	__u32 width;
	__u32 height;
	__u32 line_length;
	__u32 pixel_format;
	__u32 embedded_metadata_height;
	__u32 reserved[11];
};

struct sensor_dv_timings {
	__u32 hfrontporch;
	__u32 hsync;
	__u32 hbackporch;
	__u32 vfrontporch;
	__u32 vsync;
	__u32 vbackporch;
	__u32 reserved[10];
};

struct sensor_control_properties {
	__u32 gain_factor;
	__u32 framerate_factor;
	__u32 inherent_gain;
	__u32 min_gain_val;
	__u32 max_gain_val;
	__u32 min_hdr_ratio;
	__u32 max_hdr_ratio;
	__u32 min_framerate;
	__u32 max_framerate;
	union __u64val min_exp_time;
	union __u64val max_exp_time;
	__u32 step_gain_val;
	__u32 step_framerate;
	__u32 exposure_factor;
	union __u64val step_exp_time;
	__u32 default_gain;
	__u32 default_framerate;
	union __u64val default_exp_time;
	__u32 is_interlaced;
	__u32 interlace_type;
	__u32 reserved[10];
};

struct sensor_mode_properties {
	struct sensor_signal_properties signal_properties;
	struct sensor_image_properties image_properties;
	struct sensor_control_properties control_properties;
	struct sensor_dv_timings dv_timings;
};

#define SENSOR_SIGNAL_PROPERTIES_CID_SIZE \
	(sizeof(struct sensor_signal_properties) / sizeof(__u32))
#define SENSOR_IMAGE_PROPERTIES_CID_SIZE \
	(sizeof(struct sensor_image_properties) / sizeof(__u32))
#define SENSOR_CONTROL_PROPERTIES_CID_SIZE \
	(sizeof(struct sensor_control_properties) / sizeof(__u32))
#define SENSOR_DV_TIMINGS_CID_SIZE \
	(sizeof(struct sensor_dv_timings) / sizeof(__u32))
#define SENSOR_MODE_PROPERTIES_CID_SIZE \
	(sizeof(struct sensor_mode_properties) / sizeof(__u32))
#define SENSOR_CONFIG_SIZE \
	(sizeof(struct sensor_cfg) / sizeof(__u32))
#define SENSOR_MODE_BLOB_SIZE \
	(sizeof(struct sensor_blob) / sizeof(__u32))
#define SENSOR_CTRL_BLOB_SIZE \
	(sizeof(struct sensor_blob) / sizeof(__u32))

struct alternating_exposure_cfg {
	__u8 enable;
	__s64 exp_time_0;
	__s64 exp_time_1;
	__s64 analog_gain_0;
	__s64 analog_gain_1;
};

#define ALTERNATING_EXPOSURE_CID_SIZE \
	sizeof(struct alternating_exposure_cfg)
#endif /* __TEGRA_V4L2_CAMERA__ */

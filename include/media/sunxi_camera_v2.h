/*
 * include/media/sunxi_camera.h -- Ctrl IDs definitions for sunxi-vin
 *
 * Copyright (C) 2014 Allwinnertech Co., Ltd.
 * Copyright (C) 2015 Yang Feng
 *
 * Author: Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#ifndef _SUNXI_CAMERA_H_
#define _SUNXI_CAMERA_H_

#include <linux/types.h>
#include <linux/videodev2.h>

/*  Flags for 'capability' and 'capturemode' fields */
#define V4L2_MODE_HIGHQUALITY		0x0001
#define V4L2_MODE_VIDEO			0x0002
#define V4L2_MODE_IMAGE			0x0003
#define V4L2_MODE_PREVIEW		0x0004
/*
 *	USER CIDS
 */
struct v4l2_win_coordinate {
	__s32 x1;
	__s32 y1;
	__s32 x2;
	__s32 y2;
};

#define V4L2_FLASH_LED_MODE_AUTO		(V4L2_FLASH_LED_MODE_TORCH + 1)
#define V4L2_FLASH_LED_MODE_RED_EYE		(V4L2_FLASH_LED_MODE_TORCH + 2)

struct v4l2_win_setting {
	struct v4l2_win_coordinate coor;
};

enum v4l2_gain_shift {
	V4L2_GAIN_SHIFT = 0,
	V4L2_SHARP_LEVEL_SHIFT = 8,
	V4L2_SHARP_MIN_SHIFT = 20,
	V4L2_NDF_SHIFT = 26,
};

#define MAX_EXP_FRAMES     5

/*
 * The base for the sunxi-vfe controls.
 * Total of 64 controls is reserved for this driver, add by yangfeng
 */
#define V4L2_CID_USER_SUNXI_CAMERA_BASE		(V4L2_CID_USER_BASE + 0x1050)

#define V4L2_CID_AUTO_FOCUS_INIT	(V4L2_CID_USER_SUNXI_CAMERA_BASE + 2)
#define V4L2_CID_AUTO_FOCUS_RELEASE	(V4L2_CID_USER_SUNXI_CAMERA_BASE + 3)
#define V4L2_CID_GSENSOR_ROTATION	(V4L2_CID_USER_SUNXI_CAMERA_BASE + 4)
#define V4L2_CID_FRAME_RATE             (V4L2_CID_USER_SUNXI_CAMERA_BASE + 5)

enum v4l2_take_picture {
	V4L2_TAKE_PICTURE_STOP = 0,
	V4L2_TAKE_PICTURE_NORM = 1,
	V4L2_TAKE_PICTURE_FAST = 2,
	V4L2_TAKE_PICTURE_FLASH = 3,
	V4L2_TAKE_PICTURE_HDR = 4,
};
struct isp_hdr_setting_t {
	__s32 hdr_en;
	__s32 hdr_mode;
	__s32 frames_count;
	__s32 total_frames;
	__s32 values[MAX_EXP_FRAMES];
};

#define HDR_CTRL_GET    0
#define HDR_CTRL_SET     1
struct isp_hdr_ctrl {
	__s32 flag;
	__s32 count;
	struct isp_hdr_setting_t hdr_t;
};

#define V4L2_CID_TAKE_PICTURE	(V4L2_CID_USER_SUNXI_CAMERA_BASE + 6)

typedef union {
	unsigned int dwval;
	struct {
		unsigned int af_sharp:16;
		unsigned int hdr_cnt:4;
		unsigned int flash_ok:1;
		unsigned int capture_ok:1;
		unsigned int fast_capture_ok:1;
		unsigned int res0:9;
	} bits;
} IMAGE_FLAG_t;

#define  V4L2_CID_HOR_VISUAL_ANGLE	(V4L2_CID_USER_SUNXI_CAMERA_BASE + 7)
#define  V4L2_CID_VER_VISUAL_ANGLE	(V4L2_CID_USER_SUNXI_CAMERA_BASE + 8)
#define  V4L2_CID_FOCUS_LENGTH		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 9)
#define  V4L2_CID_R_GAIN		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 10)
#define  V4L2_CID_GR_GAIN		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 11)
#define  V4L2_CID_GB_GAIN		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 12)
#define  V4L2_CID_B_GAIN		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 13)

enum v4l2_sensor_type {
	V4L2_SENSOR_TYPE_YUV = 0,
	V4L2_SENSOR_TYPE_RAW = 1,
};

#define V4L2_CID_SENSOR_TYPE		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 14)

#define  V4L2_CID_AE_WIN_X1		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 15)
#define  V4L2_CID_AE_WIN_Y1		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 16)
#define  V4L2_CID_AE_WIN_X2		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 17)
#define  V4L2_CID_AE_WIN_Y2		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 18)

#define  V4L2_CID_AF_WIN_X1		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 19)
#define  V4L2_CID_AF_WIN_Y1		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 20)
#define  V4L2_CID_AF_WIN_X2		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 21)
#define  V4L2_CID_AF_WIN_Y2		(V4L2_CID_USER_SUNXI_CAMERA_BASE + 22)

/*
 *	PRIVATE IOCTRLS
 */

struct isp_stat_buf {
	void __user *buf;
	__u32 buf_size;
};
struct isp_exif_attribute {
	struct v4l2_fract exposure_time;
	struct v4l2_fract shutter_speed;
	__u32 fnumber;
	__u32 focal_length;
	__s32 exposure_bias;
	__u32 iso_speed;
	__u32 flash_fire;
	__u32 brightness;
	__s32 reserved[16];
};

#define VIDIOC_ISP_AE_STAT_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct isp_stat_buf)
#define VIDIOC_ISP_HIST_STAT_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct isp_stat_buf)
#define VIDIOC_ISP_AF_STAT_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 3, struct isp_stat_buf)
#define VIDIOC_ISP_EXIF_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 4, struct isp_exif_attribute)
#define VIDIOC_ISP_GAMMA_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 5, struct isp_stat_buf)
#define VIDIOC_HDR_CTRL \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 8, struct isp_hdr_ctrl)

/*
 * Events
 *
 * V4L2_EVENT_VIN_H3A: Histogram and AWB AE AF statistics data ready
 */

#define V4L2_EVENT_VIN_CLASS		(V4L2_EVENT_PRIVATE_START | 0x100)
#define V4L2_EVENT_VIN_H3A		(V4L2_EVENT_VIN_CLASS | 0x1)
#define V4L2_EVENT_VIN_HDR		(V4L2_EVENT_VIN_CLASS | 0x2)

struct vin_isp_h3a_config {

	__u32 buf_size;
	__u16 config_counter;

	/* Private fields */
	__u16 saturation_limit;
	__u16 win_height;
	__u16 win_width;
	__u16 ver_win_count;
	__u16 hor_win_count;
	__u16 ver_win_start;
	__u16 hor_win_start;
	__u16 blk_ver_win_start;
	__u16 blk_win_height;
	__u16 subsample_ver_inc;
	__u16 subsample_hor_inc;
	__u8 alaw_enable;
};

/**
 * struct vin_isp_stat_data - Statistic data sent to or received from user
 * @ts: Timestamp of returned framestats.
 * @buf: Pointer to pass to user.
 * @frame_number: Frame number of requested stats.
 * @cur_frame: Current frame number being processed.
 * @config_counter: Number of the configuration associated with the data.
 */
struct vin_isp_stat_data {
	struct timeval ts;
	void __user *buf;
	__u32 buf_size;
	__u16 frame_number;
	__u16 cur_frame;
	__u16 config_counter;
};

struct vin_isp_stat_event_status {
	__u32 frame_number;
	__u16 config_counter;
	__u8 buf_err;
};

struct vin_isp_hdr_event_data {
	__u32			cmd;
	struct isp_hdr_ctrl	hdr;
};

/*
 * Statistics IOCTLs
 *
 * VIDIOC_VIN_ISP_H3A_CFG: Set AE configuration
 * VIDIOC_VIN_ISP_STAT_REQ: Read statistics (AE/AWB/AF/histogram) data
 * VIDIOC_VIN_ISP_STAT_EN: Enable/disable a statistics module
 */

#define VIDIOC_VIN_ISP_H3A_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 31, struct vin_isp_h3a_config)
#define VIDIOC_VIN_ISP_STAT_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 32, struct vin_isp_stat_data)
#define VIDIOC_VIN_ISP_STAT_EN \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 33, unsigned long)

/*
 * CSI configure
 */

/*
 * if format
 */
enum csi_if {
	CSI_IF_INTLV = 0x00,	/* 1SEG DATA in one channel */
	CSI_IF_SPL = 0x01,	/* 2SEG Y in one ch, UV in second ch */
	CSI_IF_PL = 0x02,	/* 3SEG YUV444 */
	CSI_IF_PL_SPL = 0x03,	/* 3SEG YUV444 to 2SEG YUV422 */

	CSI_IF_CCIR656_1CH = 0x04,	/* 1SEG ccir656 1ch */
	CSI_IF_CCIR656_1CH_SPL = 0x05,	/* 2SEG ccir656 1ch */
	CSI_IF_CCIR656_1CH_PL = 0x06,	/* 3SEG ccir656 1ch */
	CSI_IF_CCIR656_1CH_PL_SPL = 0x07,	/* 3SEG to 2SEG ccir656 1ch */
	CSI_IF_CCIR656_16BIT = 0x08,	/* 16BIT ccir656 1ch */

	CSI_IF_CCIR656_2CH = 0x0c,	/* D7~D0:ccir656 2ch */
	CSI_IF_CCIR656_4CH = 0x0d,	/* D7~D0:ccir656 4ch */

	CSI_IF_MIPI = 0x80,	/* MIPI CSI */
};

/*
 *  data width
 */
enum csi_data_width {
	CSI_8BIT = 0,
	CSI_10BIT = 1,
	CSI_12BIT = 2,
};

/*
 * input data format
 */
enum csi_input_fmt {
	CSI_RAW = 0,		/* raw stream  */
	CSI_BAYER,		/* byer rgb242 */
	CSI_CCIR656,		/* ccir656     */
	CSI_YUV422,		/* yuv422      */
	CSI_YUV422_16 = 4,	/* yuv422 16 bit */
	CSI_YUV444 = 4,		/* yuv444 24 bit */
	CSI_YUV420 = 4,		/* yuv420 */
	CSI_CCIR656_2CH	= 5,	/* ccir656 2 channel */
	CSI_CCIR656_4CH	= 7,	/* ccir656 4 channel */
};

/*
 * output data format
 */
enum csi_output_fmt {
	/* only when input is raw */
	CSI_FIELD_RAW_8 = 0,
	CSI_FIELD_RAW_10 = 1,
	CSI_FIELD_RAW_12 = 2,
	CSI_FIELD_RGB565 = 4,
	CSI_FIELD_RGB888 = 5,
	CSI_FIELD_PRGB888 = 6,
	CSI_FRAME_RAW_8 = 8,
	CSI_FRAME_RAW_10 = 9,
	CSI_FRAME_RAW_12 = 10,
	CSI_FRAME_RGB565 = 12,
	CSI_FRAME_RGB888 = 13,
	CSI_FRAME_PRGB888 = 14,

	/* only when input is bayer */

	/* only when input is yuv422/yuv420 */
	CSI_FIELD_PLANAR_YUV422 = 0, /* parse 1 field (odd or even) */
	CSI_FIELD_PLANAR_YUV420 = 1,
	CSI_FRAME_PLANAR_YUV420 = 2, /* parse 2 fields (odd and even) */
	CSI_FRAME_PLANAR_YUV422 = 3,
	CSI_FIELD_UV_CB_YUV422 = 4,
	CSI_FIELD_UV_CB_YUV420 = 5,
	CSI_FRAME_UV_CB_YUV420 = 6,
	CSI_FRAME_UV_CB_YUV422 = 7,
	CSI_FIELD_MB_YUV422 = 8,
	CSI_FIELD_MB_YUV420 = 9,
	CSI_FRAME_MB_YUV420 = 10,
	CSI_FRAME_MB_YUV422 = 11,
	CSI_FIELD_UV_CB_YUV422_10 = 12,
	CSI_FIELD_UV_CB_YUV420_10 = 13,
};

/*
 * field sequenc or polarity
 */

enum csi_field {
	/* For Embedded Sync timing */
	CSI_FIELD_TF = 0,	/* top filed first */
	CSI_FIELD_BF = 1,	/* bottom field first */

	/* For External Sync timing */
	CSI_FIELD_NEG = 0,	/* field negtive indicates odd field */
	CSI_FIELD_POS = 1,	/* field postive indicates odd field */
};

/*
 * input field selection, only when input is ccir656
 */
enum csi_field_sel {
	CSI_ODD,		/* odd field */
	CSI_EVEN,		/* even field */
	CSI_EITHER,		/* either field */
};

/*
 * input source type
 */
enum csi_src_type {
	CSI_PROGRESSIVE = 0,	/* progressive */
	CSI_INTERLACE = 1,	/* interlace */
};

/*
 * input data sequence
 */
enum csi_input_seq {
	/* only when input is yuv422 */
	CSI_YUYV = 0,
	CSI_YVYU,
	CSI_UYVY,
	CSI_VYUY,

	/* only when input is byer */
	CSI_RGRG = 0,		/* first line sequence is RGRG... */
	CSI_GRGR,		/* first line sequence is GRGR... */
	CSI_BGBG,		/* first line sequence is BGBG... */
	CSI_GBGB,		/* first line sequence is GBGB... */
};

/*
 * input reference signal polarity
 */
enum csi_ref_pol {
	CSI_LOW,		/* active low */
	CSI_HIGH,		/* active high */
};

/*
 * input data valid of the input clock edge type
 */
enum csi_edge_pol {
	CSI_FALLING,		/* active falling */
	CSI_RISING,		/* active rising */
};

/*
 * csi interface configuration
 */
struct csi_if_cfg {
	enum csi_src_type src_type;	/* interlaced or progressive */
	enum csi_data_width data_width;	/* csi data width */
	enum csi_if interface;	/* csi interface */
};

/*
 * csi timing configuration
 */
struct csi_timing_cfg {
	enum csi_field field;	/* top or bottom field first / field polarity */
	enum csi_ref_pol vref;	/* input vref signal polarity */
	enum csi_ref_pol href;	/* input href signal polarity */
	enum csi_edge_pol sample; /* the valid input clock edge */
};

/*
 * csi size and offset configuration
 */
struct csi_size_offset_cfg {
    unsigned int length_h;
    unsigned int length_v;
    unsigned int length_y;
    unsigned int length_c;
    unsigned int start_h;
    unsigned int start_v;
};

/*
 * csi mode configuration
 */
struct csi_fmt_cfg {
	enum csi_input_fmt input_fmt;	/* input data format */
	enum csi_output_fmt output_fmt;	/* output data format */
	enum csi_field_sel field_sel;	/* input field selection */
	enum csi_input_seq input_seq;	/* input data sequence */
	enum csi_data_width data_width;	/* csi data width */
};

/*
 * csi buffer
 */

enum csi_buf_sel {
	CSI_BUF_0_A = 0,	/* FIFO for Y address A */
	CSI_BUF_0_B,		/* FIFO for Y address B */
	CSI_BUF_1_A,		/* FIFO for Cb address A */
	CSI_BUF_1_B,		/* FIFO for Cb address B */
	CSI_BUF_2_A,		/* FIFO for Cr address A */
	CSI_BUF_2_B,		/* FIFO for Cr address B */
};

/*
 * csi buffer configs
 */

struct csi_buf_cfg {
	enum csi_buf_sel buf_sel;
	unsigned long addr;
};

/*
 * csi capture status
 */
struct csi_capture_status {
	_Bool picture_in_progress;
	_Bool video_in_progress;
};

enum csi_cap_mode {
	CSI_SCAP = 1,
	CSI_VCAP,
};

/*
 * csi interrupt
 */
enum csi_int_sel {
	CSI_INT_CAPTURE_DONE = 0X1,
	CSI_INT_FRAME_DONE = 0X2,
	CSI_INT_BUF_0_OVERFLOW = 0X4,
	CSI_INT_BUF_1_OVERFLOW = 0X8,
	CSI_INT_BUF_2_OVERFLOW = 0X10,
	CSI_INT_PROTECTION_ERROR = 0X20,
	CSI_INT_HBLANK_OVERFLOW = 0X40,
	CSI_INT_VSYNC_TRIG = 0X80,
	CSI_INT_ALL = 0XFF,
};

/*
 * csi interrupt status
 */
struct csi_int_status {
	_Bool capture_done;
	_Bool frame_done;
	_Bool buf_0_overflow;
	_Bool buf_1_overflow;
	_Bool buf_2_overflow;
	_Bool protection_error;
	_Bool hblank_overflow;
	_Bool vsync_trig;
};

struct vin_csi_fmt_cfg {
	unsigned long ch;
	struct csi_fmt_cfg fmt;
	struct csi_size_offset_cfg so;
};

struct vin_csi_buf_cfg {
	unsigned long ch;
	unsigned long set;
	struct csi_buf_cfg buf;
};

struct vin_csi_cap_mode {
	unsigned long total_ch;
	unsigned long on;
	enum csi_cap_mode mode;
};

struct vin_csi_cap_status {
	unsigned long ch;
	struct csi_capture_status status;
};

struct vin_csi_int_cfg {
	unsigned long ch;
	unsigned long en;
	enum csi_int_sel sel;
};

/*
 * CSI Bsp IOCTLs
 *
 * VIDIOC_VIN_CSI_ONOFF: Enable/disable CSI module
 * VIDIOC_VIN_CSI_IF_CFG: Configure CSI interface
 * VIDIOC_VIN_CSI_TIMING_CFG: Configure CSI timing
 * VIDIOC_VIN_CSI_FMT_CFG: Configure CSI format
 * VIDIOC_VIN_CSI_BUF_ADDR_CFG: Set/Get CSI buffer address
 * VIDIOC_VIN_CSI_CAP_MODE_CFG: Start/Stop CSI video or still capture mode
 * VIDIOC_VIN_CSI_CAP_STATUS: Get CSI capture status
 * VIDIOC_VIN_CSI_INT_CFG: Enable/Disable CSI interrupts
 */

#define VIDIOC_VIN_CSI_ONOFF \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 41, unsigned long)
#define VIDIOC_VIN_CSI_IF_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 42, struct csi_if_cfg)
#define VIDIOC_VIN_CSI_TIMING_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 43, struct csi_timing_cfg)
#define VIDIOC_VIN_CSI_FMT_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 44, struct vin_csi_fmt_cfg)
#define VIDIOC_VIN_CSI_BUF_ADDR_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 45, struct vin_csi_buf_cfg)
#define VIDIOC_VIN_CSI_CAP_MODE_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 46, struct vin_csi_cap_mode)
#define VIDIOC_VIN_CSI_CAP_STATUS \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 47, struct vin_csi_cap_status)
#define VIDIOC_VIN_CSI_INT_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 48, struct vin_csi_int_cfg)


struct sensor_config {
	int width;
	int height;
	unsigned int hoffset;	/*receive hoffset from sensor output*/
	unsigned int voffset;	/*receive voffset from sensor output*/
	unsigned int hts;	/*h size of timing, unit: pclk      */
	unsigned int vts;	/*v size of timing, unit: line      */
	unsigned int pclk;	/*pixel clock in Hz                 */
	unsigned int bin_factor;/*binning factor                    */
	unsigned int intg_min;	/*integration min, unit: line, Q4   */
	unsigned int intg_max;	/*integration max, unit: line, Q4   */
	unsigned int gain_min;	/*sensor gain min, Q4               */
	unsigned int gain_max;	/*sensor gain max, Q4               */
};

/*
 * Camera Sensor IOCTLs
 */

#define VIDIOC_VIN_SENSOR_CFG_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 60, struct sensor_config)


#endif /*_SUNXI_CAMERA_H_*/


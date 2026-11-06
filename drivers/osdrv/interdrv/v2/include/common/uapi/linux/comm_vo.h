/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/comm_vo.h
 * Description:
 *   The common data type defination for VO module.
 */

#ifndef __COMM_VO_H__
#define __COMM_VO_H__

#include <linux/common.h>
#include <linux/comm_video.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define VO_GAMMA_NODENUM 65
#define MAX_VO_PINS 32

/* VO video output interface type */
typedef enum {
	VO_INTF_BT656 = (0x01L << 7),
	VO_INTF_BT1120 = (0x01L << 8),
	VO_INTF_PARALLEL_RGB = (0x01L << 9),
	VO_INTF_SERIAL_RGB = (0x01L << 10),
	VO_INTF_I80 = (0x01L << 11),
	VO_INTF_HW_MCU = (0x01L << 12),
	VO_INTF_MIPI = (0x01L << 13),
	VO_INTF_LVDS = (0x01L << 14),
	VO_INTF_HDMI = (0x01L << 15),
	VO_INTF_BUTT
} vo_intf_type_e;

/* VO video output sync type */
typedef enum {
	VO_OUTPUT_PAL = 0, /* PAL standard*/
	VO_OUTPUT_NTSC, /* NTSC standard */
	VO_OUTPUT_1080P24, /* 1920 x 1080 at 24 Hz. */
	VO_OUTPUT_1080P25, /* 1920 x 1080 at 25 Hz. */
	VO_OUTPUT_1080P30, /* 1920 x 1080 at 30 Hz. */
	VO_OUTPUT_720P50, /* 1280 x  720 at 50 Hz. */
	VO_OUTPUT_720P60, /* 1280 x  720 at 60 Hz. */
	VO_OUTPUT_1080P50, /* 1920 x 1080 at 50 Hz. */
	VO_OUTPUT_1080P60, /* 1920 x 1080 at 60 Hz. */
	VO_OUTPUT_576P50, /* 720  x  576 at 50 Hz. */
	VO_OUTPUT_480P60, /* 720  x  480 at 60 Hz. */
	VO_OUTPUT_800x600_60, /* VESA 800 x 600 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1024x768_60, /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1280x1024_60, /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1366x768_60, /* VESA 1366 x 768 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1440x900_60, /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT Compliant */
	VO_OUTPUT_1280x800_60, /* 1280*800@60Hz VGA@60Hz*/
	VO_OUTPUT_1600x1200_60, /* VESA 1600 x 1200 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1680x1050_60, /* VESA 1680 x 1050 at 60 Hz (non-interlaced) */
	VO_OUTPUT_1920x1200_60, /* VESA 1920 x 1600 at 60 Hz (non-interlaced) CVT (Reduced Blanking)*/
	VO_OUTPUT_640x480_60, /* VESA 640 x 480 at 60 Hz (non-interlaced) CVT */
	VO_OUTPUT_720x1280_60, /* For MIPI DSI Tx 720 x1280 at 60 Hz */
	VO_OUTPUT_1080x1920_60, /* For MIPI DSI Tx 1080x1920 at 60 Hz */
	VO_OUTPUT_480x800_60, /* For MIPI DSI Tx 480x800 at 60 Hz */
	VO_OUTPUT_1440P60, /* 2560 x 1440 at 60 Hz. */
	VO_OUTPUT_2160P24, /* 3840 x 2160 at 24 Hz. */
	VO_OUTPUT_2160P25, /* 3840 x 2160 at 25 Hz. */
	VO_OUTPUT_2160P30, /* 3840 x 2160 at 30 Hz. */
	VO_OUTPUT_2160P50, /* 3840 x 2160 at 50 Hz. */
	VO_OUTPUT_2160P60, /* 3840 x 2160 at 60 Hz. */
	VO_OUTPUT_4096x2160P24, /* 4096 x 2160 at 24 Hz. */
	VO_OUTPUT_4096x2160P25, /* 4096 x 2160 at 25 Hz. */
	VO_OUTPUT_4096x2160P30, /* 4096 x 2160 at 30 Hz. */
	VO_OUTPUT_4096x2160P50, /* 4096 x 2160 at 50 Hz. */
	VO_OUTPUT_4096x2160P60, /* 4096 x 2160 at 60 Hz. */
	VO_OUTPUT_USER, /* User timing. */
	VO_OUTPUT_BUTT
} vo_intf_sync_e;

/*
 * synm: sync mode(0:timing,as BT.656; 1:signal,as LCD)
 * iop: interlaced or progressive display(0:i; 1:p)
 * frame_rate: frame-rate
 * vact: vertical active area
 * vbb: vertical back blank porch
 * vfb: vertical front blank porch
 * hact: horizontal active area
 * hbb: horizontal back blank porch
 * hfb: horizontal front blank porch
 * hpw: horizontal pulse width
 * vpw: vertical pulse width
 * idv: inverse data valid of output
 * ihs: inverse horizontal synch signal
 * ivs: inverse vertical synch signal
 */
typedef struct {
	bool synm;
	bool iop;

	u16 frame_rate;

	u16 vact;
	u16 vbb;
	u16 vfb;

	u16 hact;
	u16 hbb;
	u16 hfb;

	u16 hpw;
	u16 vpw;

	bool idv;
	bool ihs;
	bool ivs;
} vo_sync_info_s;

typedef enum {
	VO_VIVO_CLK  = 0,
	VO_VIV1_CLK  = 1,
	VO_MIPI1_TXN1 = 2,
	VO_MIPI1_TXP2 = 3,
	VO_MIPI1_TXN2 = 4,
	VO_MIPI1_TXP3 = 5,
	VO_MIPI1_TXN3 = 6,
	VO_MIPI1_TXP4 = 7,
	VO_MIPI1_TXN4 = 8,
	VO_VIVO0_D10 = 9,
	VO_VIVO0_D11 = 10,
	VO_VIVO0_D12 = 11,
	VO_VIVO0_D13 = 12,
	VO_VIVO0_D14 = 13,
	VO_VIVO0_D15 = 14,
	VO_VIVO0_D16 = 15,
	VO_MIPI0_TXP0 = 16,
	VO_MIPI0_TXN0 = 17,
	VO_MIPI0_TXP1 = 18,
	VO_MIPI0_TXN1 = 19,
	VO_MIPI0_TXP2 = 20,
	VO_MIPI0_TXN2 = 21,
	VO_MIPI0_TXP3 = 22,
	VO_MIPI0_TXN3 = 23,
	VO_MIPI0_TXP4 = 24,
	VO_MIPI0_TXN4 = 25,
	VO_MIPI1_TXP0 = 26,
	VO_MIPI1_TXN0 = 27,
	VO_MIPI1_TXP1 = 28,
	VO_PAD_MAX,
} vo_mac_d_sel_e;

typedef enum {
	VO_MUX_BT_VS = 0,
	VO_MUX_BT_HS,
	VO_MUX_BT_HDE,
	VO_MUX_BT_DATA0,
	VO_MUX_BT_DATA1,
	VO_MUX_BT_DATA2,
	VO_MUX_BT_DATA3,
	VO_MUX_BT_DATA4,
	VO_MUX_BT_DATA5,
	VO_MUX_BT_DATA6,
	VO_MUX_BT_DATA7,
	VO_MUX_BT_DATA8,
	VO_MUX_BT_DATA9,
	VO_MUX_BT_DATA10,
	VO_MUX_BT_DATA11,
	VO_MUX_BT_DATA12,
	VO_MUX_BT_DATA13,
	VO_MUX_BT_DATA14,
	VO_MUX_BT_DATA15,
	VO_MUX_TG_HS_TILE = 30,
	VO_MUX_TG_VS_TILE,
	VO_MUX_BT_CLK,
	VO_BT_MUX_MAX,
}vo_mac_bt_mux_e;

typedef enum {
	VO_BT_MODE_656 = 0,
	VO_BT_MODE_1120,
	VO_BT_MODE_601,
	VO_BT_MODE_MAX,
}vo_bt_mode_e;

typedef enum {
	VO_BT_DATA_SEQ0 = 0,
	VO_BT_DATA_SEQ1,
	VO_BT_DATA_SEQ2,
	VO_BT_DATA_SEQ3,
} vo_bt_data_seq_e;

typedef struct {
	vo_mac_d_sel_e sel;
	vo_mac_bt_mux_e mux;
}vo_d_remap_s;

typedef struct {
	unsigned char pin_num;
	bool bt_clk_inv;
	bool bt_vs_inv;
	bool bt_hs_inv;
	vo_bt_data_seq_e data_seq;
	vo_d_remap_s d_pins[MAX_VO_PINS];
} vo_bt_attr_s;

/*
 * bgcolor: Background color of a device, in RGB format.
 * intf_type: Type of a VO interface.
 * intf_sync: Type of a VO interface timing.
 * sync_info: Information about VO interface timings if customed type.
 */
typedef struct {
	u32 bgcolor;
	vo_intf_type_e intf_type;
	vo_intf_sync_e intf_sync;
	vo_sync_info_s sync_info;
} vo_pub_attr_s;

typedef enum {
	VO_LVDS_MODE_JEIDA = 0,
	VO_LVDS_MODE_VESA,
	VO_LVDS_MODE_MAX,
} vo_lvds_mode_e;

typedef enum {
	VO_LVDS_OUT_6BIT = 0,
	VO_LVDS_OUT_8BIT,
	VO_LVDS_OUT_10BIT,
	VO_LVDS_OUT_MAX,
} vo_lvds_out_bit_e;

typedef enum {
	VO_LVDS_LANE_CLK = 0,
	VO_LVDS_LANE_0,
	VO_LVDS_LANE_1,
	VO_LVDS_LANE_2,
	VO_LVDS_LANE_3,
	VO_LVDS_LANE_MAX,
} vo_lvds_lane_e;

/* Define LVDS's config
 *
 * lvds_vesa_mode: true for VESA mode; false for JEIDA mode
 * out_bits: 6/8/10 bit
 * chn_num: output channel num
 * data_big_endian: true for big endian; false for little endian
 * lane_id: lane mapping, -1 no used
 * lane_pn_swap: lane pn-swap if true
 */
typedef struct {
	vo_lvds_mode_e lvds_vesa_mode;
	vo_lvds_out_bit_e out_bits;
	u8 chn_num;
	bool data_big_endian;
	vo_lvds_lane_e lane_id[VO_LVDS_LANE_MAX];
	bool lane_pn_swap[VO_LVDS_LANE_MAX];
} vo_lvds_attr_s;

typedef enum {
	VO_CSC_MATRIX_IDENTITY = 0,

	VO_CSC_MATRIX_601_LIMIT_YUV2RGB,
	VO_CSC_MATRIX_601_FULL_YUV2RGB,

	VO_CSC_MATRIX_709_LIMIT_YUV2RGB,
	VO_CSC_MATRIX_709_FULL_YUV2RGB,

	VO_CSC_MATRIX_601_LIMIT_RGB2YUV,
	VO_CSC_MATRIX_601_FULL_RGB2YUV,

	VO_CSC_MATRIX_709_LIMIT_RGB2YUV,
	VO_CSC_MATRIX_709_FULL_RGB2YUV,

	VO_CSC_MATRIX_BUTT
} vo_csc_matrix_e;

/*
 * csc_matrix: CSC matrix
 */
typedef struct {
	vo_csc_matrix_e csc_matrix;
} vo_csc_s;

typedef struct {
	vo_csc_s hdmi_csc;
} vo_hdmi_param_s;

/*
 * disp_rect: Display resolution
 * img_size: Original ImageSize.
 *              Only useful if vo support scaling, otherwise, it should be the same width disp_rect.
 * frame_rate: frame rate.
 * pixformat: Pixel format of the video layer
 * depth: Depth of layer done queue.
 */
typedef struct {
	rect_s disp_rect;
	size_s img_size;
	u32 frame_rate;
	pixel_format_e pixformat;
	u32 depth;
} vo_video_layer_attr_s;

/*
 * priority: Video out overlay priority.
 * rect: Rectangle of video output channel.
 * depth: Depth of video output channel done queue.
 */
typedef struct {
	u32 priority;
	rect_s rect;
	u32 depth;
} vo_chn_attr_s;

/*
 * aspect_ratio: Aspect Ratio info.
 */
typedef struct {
	aspect_ratio_s aspect_ratio;
} vo_chn_param_s;

typedef enum {
	VO_CHN_ZOOM_IN_RECT = 0, /* Zoom in by rect */
	VO_CHN_ZOOM_IN_RATIO = 1, /* Zoom in by ratio */
	VO_CHN_ZOOM_IN_BUTT,
} vo_chn_zoom_type_e;

typedef struct {
	/* RW; range: [0, 1000]; x_ratio = x * 1000 / W, x means start point to be zoomed, W means channel's width. */
	u32 x_ratio;
	/* RW; range: [0, 1000]; y_ratio = y * 1000 / H, y means start point to be zoomed, H means channel's height. */
	u32 y_ratio;
	/* RW; range: [0, 1000]; width_ratio = w * 1000 / W, w means width to be zoomed, W means channel's width. */
	u32 width_ratio;
	/* RW; range: [0, 1000]; height_ratio = h * 1000 / H, h means height to be zoomed, H means channel's height. */
	u32 height_ratio;
} vo_chn_zoom_ratio_e;

typedef struct {
	vo_chn_zoom_type_e zoom_type; /* RW; choose the type of zoom in */
	union {
		rect_s rect; /* RW; zoom in by rect. AUTO:vo_chn_zoom_type_e:OT_VO_ZOOM_IN_RECT; */
		vo_chn_zoom_ratio_e zoom_ratio; /* RW; zoom in by ratio. AUTO:vo_chn_zoom_type_e:OT_VO_ZOOM_IN_RATIO; */
	};
} vo_chn_zoom_attr_s;

typedef struct {
	bool enable; /* RW; do frame or not */
	border_s border; /* RW; frame's top, bottom, left, right width and color */
} vo_chn_border_attr_s;

typedef enum {
	VO_CHN_MIRROR_NONE = 0,
	VO_CHN_MIRROR_HOR = 1,
	VO_CHN_MIRROR_VER = 2,
	VO_CHN_MIRROR_BOTH = 3,
	VO_CHN_MIRROR_BUTT
} vo_chn_mirror_type_e;

/*
 * chn_buf_used: Channel buffer that been occupied.
 */
typedef struct {
	u32 chn_buf_used;
} vo_query_status_s;

typedef struct {
	size_s target_size; /* RW; WBC zoom target size */
	pixel_format_e pixformat; /* RW; the pixel format of WBC output */
	u32 frame_rate; /* RW; frame rate control */
	dynamic_range_e dynamic_range; /* RW; write back dynamic range type */
	compress_mode_e compress_mode; /* RW; write back compressing mode */
} vo_wbc_attr_s;

typedef enum {
    VO_WBC_MODE_NORM = 0, /* In this mode, wbc will capture frames according to dev frame rate
                                and wbc frame rate */
    VO_WBC_MODE_DROP_REPEAT = 1, /* In this mode, wbc will drop dev repeat frame, and capture the real frame
                                according to video layer's display rate and wbc frame rate */
    VO_WBC_MODE_PROGRESSIVE_TO_INTERLACED = 2, /* In this mode, wbc will drop dev repeat frame which repeats more
                                than 3 times, and change two progressive frames to one interlaced frame */
    VO_WBC_MODE_BUTT,
} vo_wbc_mode_e;

typedef enum {
    VO_WBC_SRC_DEV = 0, /* WBC source is device */
    VO_WBC_SRC_VIDEO = 1, /* WBC source is video layer */
    VO_WBC_SRC_BUTT,
} vo_wbc_src_type_e;

typedef struct {
	vo_wbc_src_type_e src_type; /* RW; WBC source's type */
	u32 src_id; /* RW; WBC source's ID */
} vo_wbc_src_s;

typedef struct {
	vo_dev dev;
	bool enable;
	bool osd_apply;
	u32 value[VO_GAMMA_NODENUM];
} vo_gamma_info_s;

typedef struct {
	vo_gamma_info_s gamma_info;
	u32 guard_magic;
} vo_bin_info_s;

typedef enum {
	VO_PAT_OFF = 0,
	VO_PAT_SNOW,
	VO_PAT_AUTO,
	VO_PAT_RED,
	VO_PAT_GREEN,
	VO_PAT_BLUE,
	VO_PAT_COLORBAR,
	VO_PAT_GRAY_GRAD_H,
	VO_PAT_GRAY_GRAD_V,
	VO_PAT_BLACK,
	VO_PAT_MAX,
} vo_pattern_mode_e;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __COMM_VO_H__ */

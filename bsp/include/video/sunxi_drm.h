// SPDX-License-Identifier: GPL-2.0
/*
 * sunxi_drm.h
 *
 * Copyright (c) 2007-2024 Allwinnertech Co., Ltd.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _SUNXI_DRM_H
#define _SUNXI_DRM_H
#include <uapi/drm/drm.h>

#define DCI_REG_COUNT 66
#define DEBAND_REG_COUNT 22
#define SHARP_DE35x_COUNT 26
#define HDR_REG_COUNT 6
#define SNR_REG_COUNT 11
#define ASU_REG_COUNT 9
#define DLC_USER_PARAM_COUNT (21)
#define DLC_CURVE_CTRL_POINT_COUNT (32)
#define DLC_ALL_COUNT (DLC_USER_PARAM_COUNT + DLC_CURVE_CTRL_POINT_COUNT * 7 + 1)

/* -- dci api -- */
/* dci pqd ioctl para */
typedef struct _dci_module_param_t {
	union {
		int id;
		/* enum pq_cmd */
		int cmd;
	};
	unsigned int value[DCI_REG_COUNT];
} dci_module_param_t;

/* dci commit para */
struct de_dci_commit_para {
	u8 enable;
	u8 demo_en;
	u8 demo_x;
	u8 demo_y;
	u8 demo_w;
	u8 demo_h;
	u32 dirty; /* enum pq_commit_dirty_mask */
};

/* dci blob data */
struct de_dci_para {
	struct de_dci_commit_para commit;
	dci_module_param_t pqd;
	/* enum pq_dirty_type_mask */
	u32 dirty;
};

/* -- dci api end -- */

/* -- dlc api -- */
struct dlc_module_para {
	unsigned int user_value[DLC_USER_PARAM_COUNT];
	unsigned int dynamic_limit[DLC_CURVE_CTRL_POINT_COUNT];
	unsigned int static_curvel[DLC_CURVE_CTRL_POINT_COUNT];
	unsigned int static_curvem[DLC_CURVE_CTRL_POINT_COUNT];
	unsigned int static_curveh[DLC_CURVE_CTRL_POINT_COUNT];

	// feedback data
	unsigned int final_curve[DLC_CURVE_CTRL_POINT_COUNT];
	unsigned int dynamic_curve[DLC_CURVE_CTRL_POINT_COUNT];
	unsigned int histogram[DLC_CURVE_CTRL_POINT_COUNT];
	int apl_show;
};

/* dlc pqd ioctl para */
typedef struct _dlc_module_param_t {
	union {
		int id;
		/* enum pq_cmd */
		int cmd;
	};
	union {
		unsigned int value[DLC_ALL_COUNT];
		struct dlc_module_para param;
	};
} dlc_module_param_t;

/* dlc commit para */
struct de_dlc_commit_para {
	u8 enable;
	u8 demo_en;
	u8 demo_x;
	u8 demo_y;
	u8 demo_w;
	u8 demo_h;
	u32 dirty; /* enum pq_commit_dirty_mask */
};

/* dlc blob data */
struct de_dlc_para {
	struct de_dlc_commit_para commit;
	dlc_module_param_t pqd;
	/* enum pq_dirty_type_mask */
	u32 dirty;
};

/* -- dlc api end -- */

/* -- deband api -- */
/* deband pqd ioctl para */
typedef struct _deband_module_param_t {
	union {
		int id;
		/* enum pq_cmd */
		int cmd;
	};
	unsigned int value[DEBAND_REG_COUNT];
} deband_module_param_t;

/* deband commit para */
struct de_deband_commit_para {
	u8 enable;
	u8 demo_en;
	u8 demo_x;
	u8 demo_y;
	u8 demo_w;
	u8 demo_h;
	u32 dirty; /* enum pq_commit_dirty_mask */
};

/* deband blob data */
struct de_deband_para {
	struct de_deband_commit_para commit;
	deband_module_param_t pqd;
	/* enum pq_dirty_type_mask */
	u32 dirty;
};

/* -- deband api end -- */

/* -- sharp api -- */
/* sharp pqd ioctl para */
typedef struct _sharp_de35x_t{
	union {
		int id;
		/* enum pq_cmd */
		int cmd;
	};
	unsigned int value[SHARP_DE35x_COUNT];
} sharp_de35x_t;

/* sharp commit para */
struct de_sharp_commit_para {
	u8 enable;
	u8 demo_en;
	u8 demo_x;
	u8 demo_y;
	u8 demo_w;
	u8 demo_h;
	u32 lti_level;
	u32 peak_level;
	u32 dirty; /* enum pq_commit_dirty_mask */
};

/* sharp blob data */
struct de_sharp_para {
	struct de_sharp_commit_para commit;
	sharp_de35x_t pqd;
	/* enum pq_dirty_type_mask */
	u32 dirty;
};

/* -- sharp api end -- */

/* -- hdr/gtm/cdc api -- */
/* hdr pqd ioctl para */
typedef struct _hdr_module_param_t {
	union {
		int id;
		/* enum pq_cmd */
		int cmd;
	};
	unsigned int value[HDR_REG_COUNT];
} hdr_module_param_t;

/* hdr commit para */
struct de_hdr_commit_para {
	u32 enable;
//not support demo mode
};

/* hdr blob data */
struct de_cdc_para {
	struct de_hdr_commit_para commit;
	hdr_module_param_t pqd;
	/* enum pq_dirty_type_mask */
	u32 dirty;
};

/* -- hdr/gtm/cdc api end -- */

/* -- snr api -- */
/* snr pqd ioctl para */
typedef struct _snr_module_param_t {
	union {
		int id;
		/* enum pq_cmd */
		int cmd;
	};
	unsigned int value[SNR_REG_COUNT];
} snr_module_param_t;

enum snr_buffer_flags {
	DISP_BF_NORMAL     = 0, /* non-stereo */
	DISP_BF_STEREO_TB  = 1 << 0, /* stereo top-bottom */
	DISP_BF_STEREO_FP  = 1 << 1, /* stereo frame packing */
	DISP_BF_STEREO_SSH = 1 << 2, /* stereo side by side half */
	DISP_BF_STEREO_SSF = 1 << 3, /* stereo side by side full */
	DISP_BF_STEREO_LI  = 1 << 4, /* stereo line interlace */
	/*
	 * 2d plus depth to convert into 3d,
	 * left and right image using the same frame buffer
	 */
	DISP_BF_STEREO_2D_DEPTH  = 1 << 5,
};

/* snr commit para */
struct de_snr_commit_para {
	u32 b_trd_out;
	u8 enable;
	u8 demo_en;
	u8 demo_x;
	u8 demo_y;
	u8 demo_w;
	u8 demo_h;
	unsigned char y_strength;
	unsigned char u_strength;
	unsigned char v_strength;
	unsigned char th_ver_line;
	unsigned char th_hor_line;
	enum snr_buffer_flags   flags;
	u32 dirty; /* enum pq_commit_dirty_mask */
};

/* snr blob data */
struct de_snr_para {
	struct de_snr_commit_para commit;
	snr_module_param_t pqd;
	/* enum pq_dirty_type_mask */
	u32 dirty;
};

/* -- snr api end -- */

/* -- asu api -- */
/* asu pqd ioctl para */
typedef struct _asu_module_param_t {
	union {
		int id;
		/* enum pq_cmd */
		int cmd;
	};
	unsigned int value[ASU_REG_COUNT];
} asu_module_param_t;

/* asu commit para */
struct de_asu_commit_para {
	u32 enable;//not support demo
};

/* asu blob data */
struct de_asu_para {
	struct de_asu_commit_para commit;
	asu_module_param_t pqd;
	/* enum pq_dirty_type_mask */
	u32 dirty;
};

/* -- asu api end -- */

/* --fcm api -- */
typedef struct fcm_hardware_data {
	char name[32];
	u32 lut_id;

	s32 hbh_hue[28];
	s32 sbh_hue[28];
	s32 ybh_hue[28];

	s32 angle_hue[28];
	s32 angle_sat[13];
	s32 angle_lum[13];

	s32 hbh_sat[364];
	s32 sbh_sat[364];
	s32 ybh_sat[364];

	s32 hbh_lum[364];
	s32 sbh_lum[364];
	s32 ybh_lum[364];
} fcm_hardware_data_t;

/* fcm pqd ioctl para */
struct fcm_info {
	union {
		int id;
		/* enum pq_cmd */
		int cmd;
	};
	fcm_hardware_data_t fcm_data;
};

/* fcm commit para */
struct de_fcm_commit_para {
	u8 enable;
	u8 demo_en;
	u8 demo_x;
	u8 demo_y;
	u8 demo_w;
	u8 demo_h;
	u32 dirty; /* enum pq_commit_dirty_mask */
};

/* fcm blob data */
struct de_fcm_para {
	struct de_fcm_commit_para commit;
	struct fcm_info pqd;
	u32 dirty;
};

/* --fcm api end -- */

/* -- csc api -- */
struct matrix4x4 {
	__s64 x00;
	__s64 x01;
	__s64 x02;
	__s64 x03;
	__s64 x10;
	__s64 x11;
	__s64 x12;
	__s64 x13;
	__s64 x20;
	__s64 x21;
	__s64 x22;
	__s64 x23;
	__s64 x30;
	__s64 x31;
	__s64 x32;
	__s64 x33;
};

struct color_enh {
	int contrast;
	int brightness;
	int saturation;
	int hue;
};

struct de_csc_commit_para {
	/* csc not support disable actually  */
	u32 enable;//not support demo
};

enum matrix_type {
	R2Y_BT601_F2F = 0,
	R2Y_BT709_F2F,
	R2Y_YCC,
	R2Y_ENHANCE,
	Y2R_BT601_F2F,
	Y2R_BT709_F2F,
	Y2R_YCC,
	Y2R_ENHANCE,
};

struct matrix_cfg {
	struct matrix4x4 matrix;
	int type; /* enum matrix_type */
};

enum csc_pqd_dirty {
	MATRIX_DIRTY = 1 << 0,
	BCSH_DIRTY = 1 << 1,
};

struct csc_info {
	int cmd;/* enum pq_cmd */
	struct matrix_cfg matrix;
	struct color_enh enhance;
	u32 dirty;/* enum csc_pqd_dirty */
};

struct de_csc_para {
	struct de_csc_commit_para commit;
	struct csc_info pqd;
	u32 dirty;/* enum pq_dirty_type_mask */
};
/* -- csc api end -- */

/* -- gamma api -- */
/* gamma pqd ioctl para */
struct gamma_para {
	union {
		int id;
		/* enum pq_cmd */
		int cmd;
	};
	unsigned int size;
	u32 *lut;
};

struct de_gamma_commit_para {
	u8 enable;
	u8 demo_en;
	u8 demo_x;
	u8 demo_y;
	u8 demo_w;
	u8 demo_h;
	u32 dirty; /* enum pq_commit_dirty_mask */
};

struct de_gamma_para {
	struct gamma_para pqd;
	struct de_gamma_commit_para commit;
	u32 dirty;/* enum pq_dirty_type_mask */
};

/* -- gamma api end -- */

/* -- common PQ api -- */

enum pq_cmd {
	PQ_WRITE_AND_UPDATE = 0,
	PQ_WRITE_WITHOUT_UPDATE = 1,
	PQ_READ = 2,
};

/* ioctl args for pqd */
/* 0 enum sunxi_pq_type */
/* 1 hw_id */
/* 2 ptr: pqd_module para , beginning with int cmd */
/* 3 bytes of [2] */
struct ioctl_pq_data {
	unsigned long data[4];
};

enum sunxi_pq_type {
	PQ_SET_REG =		0x1,
	PQ_GET_REG =		0x2,
	PQ_ENABLE =		0x3,
	PQ_COLOR_MATRIX =	0x4,
	PQ_FCM =		0x5,
	PQ_CDC =		0x6,
	PQ_DCI =		0x7,
	PQ_DEBAND =		0x8,
	PQ_SHARP35X =		0x9,
	PQ_SNR =		0xa,
	PQ_GTM =		0xb,
	PQ_ASU =		0xc,
	PQ_GAMMA =		0xd,
	PQ_DLC =		0xe,
};

enum pq_dirty_mask {
	FCM_DIRTY =	1 << PQ_FCM,
	DCI_DIRTY =	1 << PQ_DCI,
	CSC_DIRTY =	1 << PQ_COLOR_MATRIX,
	CDC_DIRTY =	1 << PQ_CDC,
	SHARP_DIRTY =	1 << PQ_SHARP35X,
	SNR_DIRTY =	1 << PQ_SNR,
	ASU_DIRTY =	1 << PQ_ASU,
	DEBAND_DIRTY =	1 << PQ_DEBAND,
	DLC_DIRTY =	1 << PQ_DLC,
	PQ_ALL_DIRTY =	0xffffffff,
};

enum pq_commit_dirty_mask {
	PQ_ENABLE_DIRTY =	1 << 0,
	PQ_DEMO_DIRTY =		1 << 1,
};

enum pq_dirty_type_mask {
	PQD_DIRTY_MASK =	1 << 0,
	COMMIT_DIRTY_MASK =	1 << 1,
};

struct de_frontend_data {
	struct de_snr_para snr_para;
	struct de_dci_para dci_para;
	struct de_fcm_para fcm_para;
	struct de_cdc_para cdc_para;
	struct de_sharp_para sharp_para;
	struct de_asu_para asu_para;
	struct de_dlc_para dlc_para;
	/* enum pq_dirty_mask */
	u32 dirty;
};

struct de_dither_para {
	u32 enable;
	u32 dirty;
};

struct de_smbl_para {
	u8 enable;
	u8 demo_en;
	u8 demo_x;
	u8 demo_y;
	u8 demo_w;
	u8 demo_h;
	u32 dirty; /* enum pq_commit_dirty_mask */
};

struct de_fmt_para {
	u32 enable;
	u32 dirty;
};

struct de_backend_data {
	struct de_dither_para dither_para;
	struct de_deband_para deband_para;
	struct de_smbl_para smbl_para;
	struct de_fmt_para fmt_para;
	struct de_csc_para csc_para;
	struct de_gamma_para gamma_para;
	/* enum pq_dirty_mask */
	u32 dirty;
};

struct de_color_ctm {
	/*
	 * | R_out |     | C00 C01 C02 |   | R_in |   | C03 * databits |
	 * | G_out | = ( | C10 C11 C12 | x | G_in | + | C13 * databits | ) >> 2^48
	 * | B_out |     | C20 C21 C22 |   | B_in |   | C23 * databits |
	 */
	u64 matrix[12];
};

/* -- common PQ api end -- */


/* -- feature blob -- */

/**
 *  1. channel feature blob data/disp feature blob data filled with several struct de_xxx_feature.
 *  2. channel feature blob data starts with struct de_channel_module_support, and follows with @feature_cnt de_xxx_feature.
 *  3. disp feature blob data starts with struct de_disp_feature, and follows with @feature_cnt de_xxx_feature.
 *  4. struct de_xxx_feature except de_disp_feature and de_channel_module_support should start with type flag, userspace can use this to
 *       parse the following info.
 */

struct de_chn_mod_support {
	union {
		u32 v;
		struct {
			u32 fcm:1;
			u32 dci:1;
			u32 dlc:1;
			u32 gamma:1;
			u32 sharp:1;
			u32 snr:1;
			u32 res:26;
		} module;
	};
};

struct de_channel_feature {
	struct de_chn_mod_support support;
	u8 feature_cnt;
	u8 layer_cnt;
	u8 hw_id;
};

struct de_channel_linebuf_feature {
	u32 scaler_lbuffer_yuv;
	u32 scaler_lbuffer_rgb;
	u32 scaler_lbuffer_yuv_ed;
	u8 afbc_rotate_support;
	u32 limit_afbc_rotate_height;
};

struct de_disp_mod_support {
	union {
		u32 v;
		struct {
			u32 deband:1;
			u32 fmt:1;
			u32 dither:1;
			u32 smbl:1;
			u32 ksc:1;
			u32 crc:1;
			u32 res:26;
		} module;
	};
};

struct de_disp_feature {
	struct de_disp_mod_support support;
	u8 hw_id;
	u8 feature_cnt;
	union {
		u32 v;
		struct {
			u32 share_scaler:1;
			u32 res:31;
		} feat;
	};
};

/* -- feature end -- */
#define DRM_SUNXI_PQ_PROC              0x00
#define DRM_IOCTL_SUNXI_PQ_PROC        DRM_IOWR(DRM_COMMAND_BASE + DRM_SUNXI_PQ_PROC, struct ioctl_pq_data)

#endif /*End of file*/

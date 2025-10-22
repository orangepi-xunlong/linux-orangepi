// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 Rockchip Electronics Co. Ltd.
 *
 */
#include "rk628.h"
#include "rk628_cru.h"
#include "rk628_hdmirx.h"
#include "rk628_post_process.h"
#include <linux/videodev2.h>

#define PQ_CSC_HUE_TABLE_NUM			256
#define PQ_CSC_MODE_COEF_COMMENT_LEN		32
#define PQ_CSC_SIMPLE_MAT_PARAM_FIX_BIT_WIDTH	10
#define PQ_CSC_SIMPLE_MAT_PARAM_FIX_NUM		(1 << PQ_CSC_SIMPLE_MAT_PARAM_FIX_BIT_WIDTH)

#define PQ_CALC_ENHANCE_BIT			6
/* csc convert coef fixed-point num bit width */
#define PQ_CSC_PARAM_FIX_BIT_WIDTH		10
/* csc convert coef half fixed-point num bit width */
#define PQ_CSC_PARAM_HALF_FIX_BIT_WIDTH		(PQ_CSC_PARAM_FIX_BIT_WIDTH - 1)
/* csc convert coef fixed-point num */
#define PQ_CSC_PARAM_FIX_NUM			(1 << PQ_CSC_PARAM_FIX_BIT_WIDTH)
#define PQ_CSC_PARAM_HALF_FIX_NUM		(1 << PQ_CSC_PARAM_HALF_FIX_BIT_WIDTH)
/* csc input param bit width */
#define PQ_CSC_IN_PARAM_NORM_BIT_WIDTH		9
/* csc input param normalization coef */
#define PQ_CSC_IN_PARAM_NORM_COEF		(1 << PQ_CSC_IN_PARAM_NORM_BIT_WIDTH)

/* csc hue table range [0,255] */
#define PQ_CSC_HUE_TABLE_DIV_COEF		2
/* csc brightness offset */
#define PQ_CSC_BRIGHTNESS_OFFSET		256

/* dc coef base bit width */
#define PQ_CSC_DC_COEF_BASE_BIT_WIDTH		10
/* input dc coef offset for 10bit data */
#define PQ_CSC_DC_IN_OFFSET			64
/* input and output dc coef offset for 10bit data u,v */
#define PQ_CSC_DC_IN_OUT_DEFAULT		512
/* r,g,b color temp div coef, range [-128,128] for 10bit data */
#define PQ_CSC_TEMP_OFFSET_DIV_COEF		2

#define	MAX(a, b)				((a) > (b) ? (a) : (b))
#define	MIN(a, b)				((a) < (b) ? (a) : (b))
#define	CLIP(x, min_v, max_v)			MIN(MAX(x, min_v), max_v)

#define V4L2_COLORSPACE_BT709F	0xfe
#define V4L2_COLORSPACE_BT2020F	0xff

enum vop_csc_bit_depth {
	CSC_10BIT_DEPTH,
	CSC_13BIT_DEPTH,
};

enum rk_pq_csc_mode {
	RK_PQ_CSC_YUV2RGB_601 = 0,             /* YCbCr_601 LIMIT-> RGB FULL */
	RK_PQ_CSC_YUV2RGB_709,                 /* YCbCr_709 LIMIT-> RGB FULL */
	RK_PQ_CSC_RGB2YUV_601,                 /* RGB FULL->YCbCr_601 LIMIT */
	RK_PQ_CSC_RGB2YUV_709,                 /* RGB FULL->YCbCr_709 LIMIT */
	RK_PQ_CSC_YUV2YUV_709_601,             /* YCbCr_709 LIMIT->YCbCr_601 LIMIT */
	RK_PQ_CSC_YUV2YUV_601_709,             /* YCbCr_601 LIMIT->YCbCr_709 LIMIT */
	RK_PQ_CSC_YUV2YUV,                     /* YCbCr LIMIT->YCbCr LIMIT */
	RK_PQ_CSC_YUV2RGB_601_FULL,            /* YCbCr_601 FULL-> RGB FULL */
	RK_PQ_CSC_YUV2RGB_709_FULL,            /* YCbCr_709 FULL-> RGB FULL */
	RK_PQ_CSC_RGB2YUV_601_FULL,            /* RGB FULL->YCbCr_601 FULL */
	RK_PQ_CSC_RGB2YUV_709_FULL,            /* RGB FULL->YCbCr_709 FULL */
	RK_PQ_CSC_YUV2YUV_709_601_FULL,        /* YCbCr_709 FULL->YCbCr_601 FULL */
	RK_PQ_CSC_YUV2YUV_601_709_FULL,        /* YCbCr_601 FULL->YCbCr_709 FULL */
	RK_PQ_CSC_YUV2YUV_FULL,                /* YCbCr FULL->YCbCr FULL */
	RK_PQ_CSC_YUV2YUV_LIMIT2FULL,          /* YCbCr  LIMIT->YCbCr  FULL */
	RK_PQ_CSC_YUV2YUV_601_709_LIMIT2FULL,  /* YCbCr 601 LIMIT->YCbCr 709 FULL */
	RK_PQ_CSC_YUV2YUV_709_601_LIMIT2FULL,  /* YCbCr 709 LIMIT->YCbCr 601 FULL */
	RK_PQ_CSC_YUV2YUV_FULL2LIMIT,          /* YCbCr  FULL->YCbCr  LIMIT */
	RK_PQ_CSC_YUV2YUV_601_709_FULL2LIMIT,  /* YCbCr 601 FULL->YCbCr 709 LIMIT */
	RK_PQ_CSC_YUV2YUV_709_601_FULL2LIMIT,  /* YCbCr 709 FULL->YCbCr 601 LIMIT */
	RK_PQ_CSC_YUV2RGBL_601,                /* YCbCr_601 LIMIT-> RGB LIMIT */
	RK_PQ_CSC_YUV2RGBL_709,                /* YCbCr_709 LIMIT-> RGB LIMIT */
	RK_PQ_CSC_RGBL2YUV_601,                /* RGB LIMIT->YCbCr_601 LIMIT */
	RK_PQ_CSC_RGBL2YUV_709,                /* RGB LIMIT->YCbCr_709 LIMIT */
	RK_PQ_CSC_YUV2RGBL_601_FULL,           /* YCbCr_601 FULL-> RGB LIMIT */
	RK_PQ_CSC_YUV2RGBL_709_FULL,           /* YCbCr_709 FULL-> RGB LIMIT */
	RK_PQ_CSC_RGBL2YUV_601_FULL,           /* RGB LIMIT->YCbCr_601 FULL */
	RK_PQ_CSC_RGBL2YUV_709_FULL,           /* RGB LIMIT->YCbCr_709 FULL */
	RK_PQ_CSC_RGB2RGBL,                    /* RGB FULL->RGB LIMIT */
	RK_PQ_CSC_RGBL2RGB,                    /* RGB LIMIT->RGB FULL */
	RK_PQ_CSC_RGBL2RGBL,                   /* RGB LIMIT->RGB LIMIT */
	RK_PQ_CSC_RGB2RGB,                     /* RGB FULL->RGB FULL */
	RK_PQ_CSC_YUV2RGB_2020,                /* YUV 2020 FULL->RGB  2020 FULL */
	RK_PQ_CSC_RGB2YUV2020_LIMIT2FULL,      /* BT2020RGBLIMIT -> BT2020YUVFULL */
	RK_PQ_CSC_RGB2YUV2020_LIMIT,           /* BT2020RGBLIMIT -> BT2020YUVLIMIT */
	RK_PQ_CSC_RGB2YUV2020_FULL2LIMIT,      /* BT2020RGBFULL -> BT2020YUVLIMIT */
	RK_PQ_CSC_RGB2YUV2020_FULL,            /* BT2020RGBFULL -> BT2020YUVFULL */
};

enum color_space_type {
	OPTM_CS_E_UNKNOWN = 0,
	OPTM_CS_E_ITU_R_BT_709 = 1,
	OPTM_CS_E_FCC = 4,
	OPTM_CS_E_ITU_R_BT_470_2_BG = 5,
	OPTM_CS_E_SMPTE_170_M = 6,
	OPTM_CS_E_SMPTE_240_M = 7,
	OPTM_CS_E_XV_YCC_709 = OPTM_CS_E_ITU_R_BT_709,
	OPTM_CS_E_XV_YCC_601 = 8,
	OPTM_CS_E_RGB = 9,
	OPTM_CS_E_XV_YCC_2020 = 10,
	OPTM_CS_E_RGB_2020 = 11,
};

struct rk_pq_csc_coef {
	s32 csc_coef00;
	s32 csc_coef01;
	s32 csc_coef02;
	s32 csc_coef10;
	s32 csc_coef11;
	s32 csc_coef12;
	s32 csc_coef20;
	s32 csc_coef21;
	s32 csc_coef22;
};

struct rk_pq_csc_ventor {
	s32 csc_offset0;
	s32 csc_offset1;
	s32 csc_offset2;
};

struct rk_pq_csc_dc_coef {
	s32 csc_in_dc0;
	s32 csc_in_dc1;
	s32 csc_in_dc2;
	s32 csc_out_dc0;
	s32 csc_out_dc1;
	s32 csc_out_dc2;
};

/* color space param */
struct rk_csc_colorspace_info {
	enum color_space_type input_color_space;
	enum color_space_type output_color_space;
	bool in_full_range;
	bool out_full_range;
};

struct rk_csc_mode_coef {
	enum rk_pq_csc_mode csc_mode;
	char c_csc_comment[PQ_CSC_MODE_COEF_COMMENT_LEN];
	const struct rk_pq_csc_coef *pst_csc_coef;
	const struct rk_pq_csc_dc_coef *pst_csc_dc_coef;
	struct rk_csc_colorspace_info st_csc_color_info;
};

/*
 *CSC matrix
 */
/* xv_ycc BT.601 limit(i.e. SD) -> RGB full */
static const struct rk_pq_csc_coef rk_csc_table_xv_yccsdy_cb_cr_limit_to_rgb_full = {
	1196, 0, 1639,
	1196, -402, -835,
	1196, 2072, 0
};

static const struct rk_pq_csc_dc_coef rk_dc_csc_table_xv_yccsdy_cb_cr_limit_to_rgb_full = {
	-64, -512, -512,
	0, 0, 0
};

/* BT.709 limit(i.e. HD) -> RGB full */
static const struct rk_pq_csc_coef rk_csc_table_hdy_cb_cr_limit_to_rgb_full = {
	1196, 0, 1841,
	1196, -219, -547,
	1196, 2169, 0
};

static const struct rk_pq_csc_dc_coef rk_dc_csc_table_hdy_cb_cr_limit_to_rgb_full = {
	-64, -512, -512,
	0, 0, 0
};

/* RGB full-> YUV601 (i.e. SD) limit */
static const struct rk_pq_csc_coef rk_csc_table_rgb_to_xv_yccsdy_cb_cr = {
	262, 515, 100,
	-151, -297, 448,
	448, -376, -73
};

static const struct rk_pq_csc_dc_coef rk_dc_csc_table_rgb_to_xv_yccsdy_cb_cr = {
	0, 0, 0,
	64, 512, 512
};

/* RGB full-> YUV709 (i.e. SD) limit */
static const struct rk_pq_csc_coef rk_csc_table_rgb_to_hdy_cb_cr = {
	186, 627, 63,
	-103, -346, 448,
	448, -407, -41
};

static const struct rk_pq_csc_dc_coef rk_dc_csc_table_rgb_to_hdy_cb_cr = {
	0, 0, 0,
	64, 512, 512
};

/* BT.709 (i.e. HD) -> to xv_ycc BT.601 (i.e. SD) */
static const struct rk_pq_csc_coef rk_csc_table_hdy_cb_cr_to_xv_yccsdy_cb_cr = {
	1024, 104, 201,
	0, 1014, -113,
	0, -74, 1007
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_hdy_cb_cr_to_xv_yccsdy_cb_cr = {
	-64, -512, -512,
	64, 512, 512
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_hdy_cb_cr_full_to_xv_yccsdy_cb_cr_full = {
	0, -512, -512,
	0, 512, 512
};

/* xv_ycc BT.601 (i.e. SD) -> to BT.709 (i.e. HD) */
static const struct rk_pq_csc_coef rk_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr = {
	1024, -121, -218,
	0, 1043, 117,
	0, 77, 1050
};

static const struct rk_pq_csc_dc_coef rk_dc_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr = {
	-64, -512, -512,
	64, 512, 512
};

/* xv_ycc BT.601 full(i.e. SD) -> RGB full */
static const struct rk_pq_csc_coef rk_csc_table_xv_yccsdy_cb_cr_to_rgb_full = {
	1024, 0, 1436,
	1024, -352, -731,
	1024, 1815, 0
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_xv_yccsdy_cb_cr_to_rgb_full = {
	0, -512, -512,
	0, 0, 0
};

/* BT.709 full(i.e. HD) -> RGB full */
static const struct rk_pq_csc_coef rk_csc_table_hdy_cb_cr_to_rgb_full = {
	1024, 0, 1613,
	1024, -192, -479,
	1024, 1900, 0
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_hdy_cb_cr_to_rgb_full = {
	0, -512, -512,
	0, 0, 0
};

/* RGB full-> YUV601 full(i.e. SD) */
static const struct rk_pq_csc_coef rk_csc_table_rgb_to_xv_yccsdy_cb_cr_full = {
	306, 601, 117,
	-173, -339, 512,
	512, -429, -83
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_rgb_to_xv_yccsdy_cb_cr_full = {
	0, 0, 0,
	0, 512, 512
};

/* RGB full-> YUV709 full (i.e. SD) */
static const struct rk_pq_csc_coef rk_csc_table_rgb_to_hdy_cb_cr_full = {
	218, 732, 74,
	-117, -395, 512,
	512, -465, -47
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_rgb_to_hdy_cb_cr_full = {
	0, 0, 0,
	0, 512, 512
};

/* limit -> full */
static const struct rk_pq_csc_coef rk_csc_table_identity_y_cb_cr_limit_to_y_cb_cr_full = {
	1196, 0, 0,
	0, 1169, 0,
	0, 0, 1169
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_y_cb_cr_limit_to_y_cb_cr_full = {
	-64, -512, -512,
	0, 512, 512
};

/* 601 limit -> 709 full */
static const struct rk_pq_csc_coef rk_csc_table_identity_601_limit_to_709_full = {
	1196, -138, -249,
	0, 1191, 134,
	0, 88, 1199
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_601_limit_to_709_full = {
	-64, -512, -512,
	0, 512, 512
};

/* 709 limit -> 601 full */
static const struct rk_pq_csc_coef rk_csc_table_identity_709_limit_to_601_full = {
	1196, 119, 229,
	0, 1157, -129,
	0, -85, 1150
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_709_limit_to_601_full = {
	-64, -512, -512,
	0, 512, 512
};

/* full ->   limit */
static const struct rk_pq_csc_coef rk_csc_table_identity_y_cb_cr_full_to_y_cb_cr_limit = {
	877, 0, 0,
	0, 897, 0,
	0, 0, 897
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_y_cb_cr_full_to_y_cb_cr_limit = {
	0, -512, -512,
	64, 512, 512
};

/* 601 full ->  709 limit */
static const struct rk_pq_csc_coef rk_csc_table_identity_y_cb_cr_601_full_to_y_cb_cr_709_limit = {
	877, -106, -191,
	0, 914, 103,
	0, 67, 920
};
static const struct rk_pq_csc_dc_coef
rk_dc_csc_table_identity_y_cb_cr_601_full_to_y_cb_cr_709_limit = {
	0, -512, -512,
	64, 512, 512
};

/* 709 full ->  601 limit */
static const struct rk_pq_csc_coef rk_csc_table_identity_y_cb_cr_709_full_to_y_cb_cr_601_limit = {
	877, 91, 176,
	0, 888, -99,
	0, -65, 882
};
static const struct rk_pq_csc_dc_coef
rk_dc_csc_table_identity_y_cb_cr_709_full_to_y_cb_cr_601_limit = {
	0, -512, -512,
	64, 512, 512
};

/* xv_ycc BT.601 limit(i.e. SD) -> RGB limit */
static const struct rk_pq_csc_coef rk_csc_table_xv_yccsdy_cb_cr_limit_to_rgb_limit = {
	1024, 0, 1404,
	1024, -344, -715,
	1024, 1774, 0
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_xv_yccsdy_cb_cr_limit_to_rgb_limit = {
	-64, -512, -512,
	64, 64, 64
};

/* BT.709 limit(i.e. HD) -> RGB limit */
static const struct rk_pq_csc_coef rk_csc_table_hdy_cb_cr_limit_to_rgb_limit = {
	1024, 0, 1577,
	1024, -188, -469,
	1024, 1858, 0
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_hdy_cb_cr_limit_to_rgb_limit = {
	-64, -512, -512,
	64, 64, 64
};

/* RGB limit-> YUV601 (i.e. SD) limit */
static const struct rk_pq_csc_coef rk_csc_table_rgb_limit_to_xv_yccsdy_cb_cr = {
	306, 601, 117,
	-177, -347, 524,
	524, -439, -85
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_rgb_limit_to_xv_yccsdy_cb_cr = {
	-64, -64, -64,
	64, 512, 512
};

/* RGB limit -> YUV709 (i.e. SD) limit */
static const struct rk_pq_csc_coef rk_csc_table_rgb_limit_to_hdy_cb_cr = {
	218, 732, 74,
	-120, -404, 524,
	524, -476, -48
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_rgb_limit_to_hdy_cb_cr = {
	-64, -64, -64,
	64, 512, 512
};

/* xv_ycc BT.601 full(i.e. SD) -> RGB limit */
static const struct rk_pq_csc_coef rk_csc_table_xv_yccsdy_cb_cr_to_rgb_limit = {
	877, 0, 1229,
	877, -302, -626,
	877, 1554, 0
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_xv_yccsdy_cb_cr_to_rgb_limit = {
	0, -512, -512,
	64, 64, 64
};

/* BT.709 full(i.e. HD) -> RGB limit */
static const struct rk_pq_csc_coef rk_csc_table_hdy_cb_cr_to_rgb_limit = {
	877, 0, 1381,
	877, -164, -410,
	877, 1627, 0
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_hdy_cb_cr_to_rgb_limit = {
	0, -512, -512,
	64, 64, 64
};

/* RGB limit-> YUV601 full(i.e. SD) */
static const struct rk_pq_csc_coef rk_csc_table_rgb_limit_to_xv_yccsdy_cb_cr_full = {
	358, 702, 136,
	-202, -396, 598,
	598, -501, -97
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_rgb_limit_to_xv_yccsdy_cb_cr_full = {
	-64, -64, -64,
	0, 512, 512
};

/* RGB limit-> YUV709 full (i.e. SD) */
static const struct rk_pq_csc_coef rk_csc_table_rgb_limit_to_hdy_cb_cr_full = {
	254, 855, 86,
	-137, -461, 598,
	598, -543, -55
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_rgb_limit_to_hdy_cb_cr_full = {
	-64, -64, -64,
	0, 512, 512
};

/* RGB full -> RGB limit */
static const struct rk_pq_csc_coef rk_csc_table_identity_rgb_to_rgb_limit = {
	877, 0, 0,
	0, 877, 0,
	0, 0, 877
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_rgb_to_rgb_limit = {
	0, 0, 0,
	64, 64, 64
};

/* RGB limit -> RGB full */
static const struct rk_pq_csc_coef rk_csc_table_identity_rgb_limit_to_rgb = {
	1196, 0, 0,
	0, 1196, 0,
	0, 0, 1196
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_rgb_limit_to_rgb = {
	-64, -64, -64,
	0, 0, 0
};

/* RGB limit/full -> RGB limit/full */
static const struct rk_pq_csc_coef rk_csc_table_identity_rgb_to_rgb = {
	1024, 0, 0,
	0, 1024, 0,
	0, 0, 1024
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_rgb_to_rgb1 = {
	-64, -64, -64,
	64, 64, 64
};

static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_rgb_to_rgb2 = {
	0, 0, 0,
	0, 0, 0
};

static const struct rk_pq_csc_coef rk_csc_table_identity_yuv_to_rgb_2020 = {
	1024, 0, 1510,
	1024, -169, -585,
	1024, 1927, 0
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_yuv_to_rgb_2020 = {
	0, -512, -512,
	0, 0, 0
};

/* 2020 RGB LIMIT ->YUV LIMIT */
static const struct rk_pq_csc_coef rk_csc_table_identity_rgb_limit_to_yuv_limit_2020 = {
	269, 694, 61,
	-146, -377, 524,
	524, -482, -42
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_rgb_limit_to_yuv_limit_2020 = {
	-64, -64, -64,
	64, 512, 512
};

/* 2020 RGB LIMIT ->YUV FULL */
static const struct rk_pq_csc_coef rk_csc_table_identity_rgb_limit_to_yuv_full_2020 = {
	314, 811, 71,
	-167, -431, 598,
	598, -550, -48
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_rgb_limit_to_yuv_full_2020 = {
	-64, -64, -64,
	0, 512, 512
};

/* 2020 RGB FULL ->YUV LIMIT */
static const struct rk_pq_csc_coef rk_csc_table_identity_rgb_full_to_yuv_limit_2020 = {
	230, 595, 52,
	-125, -323, 448,
	448, -412, -36
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_rgb_full_to_yuv_limit_2020 = {
	0, 0, 0,
	64, 512, 512
};

/* 2020 RGB FULL ->YUV FULL */
static const struct rk_pq_csc_coef rk_csc_table_identity_rgb_full_to_yuv_full_2020 = {
	269, 694, 61,
	-143, -369, 512,
	512, -471, -41
};
static const struct rk_pq_csc_dc_coef rk_dc_csc_table_identity_rgb_full_to_yuv_full_2020 = {
	0, 0, 0,
	0, 512, 512
};

/* identity matrix */
static const struct rk_pq_csc_coef rk_csc_table_identity_y_cb_cr_to_y_cb_cr = {
	1024, 0, 0,
	0, 1024, 0,
	0, 0, 1024
};

/*
 *CSC Param Struct
 */
static const struct rk_csc_mode_coef g_mode_csc_coef[] = {
	{
		RK_PQ_CSC_YUV2RGB_601, "YUV601 L->RGB F",
		&rk_csc_table_xv_yccsdy_cb_cr_limit_to_rgb_full,
		&rk_dc_csc_table_xv_yccsdy_cb_cr_limit_to_rgb_full,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_RGB, false, true
		}
	},
	{
		RK_PQ_CSC_YUV2RGB_709, "YUV709 L->RGB F",
		&rk_csc_table_hdy_cb_cr_limit_to_rgb_full,
		&rk_dc_csc_table_hdy_cb_cr_limit_to_rgb_full,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_RGB, false, true
		}
	},
	{
		RK_PQ_CSC_RGB2YUV_601, "RGB F->YUV601 L",
		&rk_csc_table_rgb_to_xv_yccsdy_cb_cr,
		&rk_dc_csc_table_rgb_to_xv_yccsdy_cb_cr,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_XV_YCC_601, true, false
		}
	},
	{
		RK_PQ_CSC_RGB2YUV_709, "RGB F->YUV709 L",
		&rk_csc_table_rgb_to_hdy_cb_cr,
		&rk_dc_csc_table_rgb_to_hdy_cb_cr,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_ITU_R_BT_709, true, false
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_709_601, "YUV709 L->YUV601 L",
		&rk_csc_table_hdy_cb_cr_to_xv_yccsdy_cb_cr,
		&rk_dc_csc_table_hdy_cb_cr_to_xv_yccsdy_cb_cr,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_XV_YCC_601, false, false
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_601_709, "YUV601 L->YUV709 L",
		&rk_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr,
		&rk_dc_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_ITU_R_BT_709, false, false
		}
	},
	{
		RK_PQ_CSC_YUV2YUV, "YUV L->YUV L",
		&rk_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr,
		&rk_dc_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_ITU_R_BT_709, false, false
		}
	},
	{
		RK_PQ_CSC_YUV2RGB_601_FULL, "YUV601 F->RGB F",
		&rk_csc_table_xv_yccsdy_cb_cr_to_rgb_full,
		&rk_dc_csc_table_xv_yccsdy_cb_cr_to_rgb_full,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_RGB, true, true
		}
	},
		{
		RK_PQ_CSC_YUV2RGB_709_FULL, "YUV709 F->RGB F",
		&rk_csc_table_hdy_cb_cr_to_rgb_full,
		&rk_dc_csc_table_hdy_cb_cr_to_rgb_full,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_RGB, true, true
		}
	},
	{
		RK_PQ_CSC_RGB2YUV_601_FULL, "RGB F->YUV601 F",
		&rk_csc_table_rgb_to_xv_yccsdy_cb_cr_full,
		&rk_dc_csc_table_rgb_to_xv_yccsdy_cb_cr_full,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_XV_YCC_601, true, true
		}
	},
	{
		RK_PQ_CSC_RGB2YUV_709_FULL, "RGB F->YUV709 F",
		&rk_csc_table_rgb_to_hdy_cb_cr_full,
		&rk_dc_csc_table_rgb_to_hdy_cb_cr_full,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_ITU_R_BT_709, true, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_709_601_FULL, "YUV709 F->YUV601 F",
		&rk_csc_table_hdy_cb_cr_to_xv_yccsdy_cb_cr,
		&rk_dc_csc_table_hdy_cb_cr_full_to_xv_yccsdy_cb_cr_full,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_XV_YCC_601, true, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_601_709_FULL, "YUV601 F->YUV709 F",
		&rk_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr,
		&rk_dc_csc_table_hdy_cb_cr_full_to_xv_yccsdy_cb_cr_full,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_ITU_R_BT_709, true, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_FULL, "YUV F->YUV F",
		&rk_csc_table_identity_y_cb_cr_to_y_cb_cr,
		&rk_dc_csc_table_hdy_cb_cr_full_to_xv_yccsdy_cb_cr_full,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_ITU_R_BT_709, true, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_LIMIT2FULL, "YUV L->YUV F",
		&rk_csc_table_identity_y_cb_cr_limit_to_y_cb_cr_full,
		&rk_dc_csc_table_identity_y_cb_cr_limit_to_y_cb_cr_full,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_ITU_R_BT_709, false, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_601_709_LIMIT2FULL, "YUV601 L->YUV709 F",
		&rk_csc_table_identity_601_limit_to_709_full,
		&rk_dc_csc_table_identity_601_limit_to_709_full,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_ITU_R_BT_709, false, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_709_601_LIMIT2FULL, "YUV709 L->YUV601 F",
		&rk_csc_table_identity_709_limit_to_601_full,
		&rk_dc_csc_table_identity_709_limit_to_601_full,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_XV_YCC_601, false, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_FULL2LIMIT, "YUV F->YUV L",
		&rk_csc_table_identity_y_cb_cr_full_to_y_cb_cr_limit,
		&rk_dc_csc_table_identity_y_cb_cr_full_to_y_cb_cr_limit,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_ITU_R_BT_709, true, false
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_601_709_FULL2LIMIT, "YUV601 F->YUV709 L",
		&rk_csc_table_identity_y_cb_cr_601_full_to_y_cb_cr_709_limit,
		&rk_dc_csc_table_identity_y_cb_cr_601_full_to_y_cb_cr_709_limit,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_ITU_R_BT_709, true, false
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_709_601_FULL2LIMIT, "YUV709 F->YUV601 L",
		&rk_csc_table_identity_y_cb_cr_709_full_to_y_cb_cr_601_limit,
		&rk_dc_csc_table_identity_y_cb_cr_709_full_to_y_cb_cr_601_limit,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_XV_YCC_601, true, false
		}
	},
	{
		RK_PQ_CSC_YUV2RGBL_601, "YUV601 L->RGB L",
		&rk_csc_table_xv_yccsdy_cb_cr_limit_to_rgb_limit,
		&rk_dc_csc_table_xv_yccsdy_cb_cr_limit_to_rgb_limit,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_RGB, false, false
		}
	},
	{
		RK_PQ_CSC_YUV2RGBL_709, "YUV709 L->RGB L",
		&rk_csc_table_hdy_cb_cr_limit_to_rgb_limit,
		&rk_dc_csc_table_hdy_cb_cr_limit_to_rgb_limit,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_RGB, false, false
		}
	},
	{
		RK_PQ_CSC_RGBL2YUV_601, "RGB L->YUV601 L",
		&rk_csc_table_rgb_limit_to_xv_yccsdy_cb_cr,
		&rk_dc_csc_table_rgb_limit_to_xv_yccsdy_cb_cr,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_XV_YCC_601, false, false
		}
	},
	{
		RK_PQ_CSC_RGBL2YUV_709, "RGB L->YUV709 L",
		&rk_csc_table_rgb_limit_to_hdy_cb_cr,
		&rk_dc_csc_table_rgb_limit_to_hdy_cb_cr,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_ITU_R_BT_709, false, false
		}
	},
	{
		RK_PQ_CSC_YUV2RGBL_601_FULL, "YUV601 F->RGB L",
		&rk_csc_table_xv_yccsdy_cb_cr_to_rgb_limit,
		&rk_dc_csc_table_xv_yccsdy_cb_cr_to_rgb_limit,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_RGB, true, false
		}
	},
	{
		RK_PQ_CSC_YUV2RGBL_709_FULL, "YUV709 F->RGB L",
		&rk_csc_table_hdy_cb_cr_to_rgb_limit,
		&rk_dc_csc_table_hdy_cb_cr_to_rgb_limit,
		{
			OPTM_CS_E_ITU_R_BT_709, OPTM_CS_E_RGB, true, false
		}
	},
	{
		RK_PQ_CSC_RGBL2YUV_601_FULL, "RGB L->YUV601 F",
		&rk_csc_table_rgb_limit_to_xv_yccsdy_cb_cr_full,
		&rk_dc_csc_table_rgb_limit_to_xv_yccsdy_cb_cr_full,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_XV_YCC_601, false, true
		}
	},
	{
		RK_PQ_CSC_RGBL2YUV_709_FULL, "RGB L->YUV709 F",
		&rk_csc_table_rgb_limit_to_hdy_cb_cr_full,
		&rk_dc_csc_table_rgb_limit_to_hdy_cb_cr_full,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_ITU_R_BT_709, false, true
		}
	},
	{
		RK_PQ_CSC_RGB2RGBL, "RGB F->RGB L",
		&rk_csc_table_identity_rgb_to_rgb_limit,
		&rk_dc_csc_table_identity_rgb_to_rgb_limit,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_RGB, true, false
		}
	},
	{
		RK_PQ_CSC_RGBL2RGB, "RGB L->RGB F",
		&rk_csc_table_identity_rgb_limit_to_rgb,
		&rk_dc_csc_table_identity_rgb_limit_to_rgb,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_RGB, false, true
		}
	},
	{
		RK_PQ_CSC_RGBL2RGBL, "RGB L->RGB L",
		&rk_csc_table_identity_rgb_to_rgb,
		&rk_dc_csc_table_identity_rgb_to_rgb1,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_RGB, false, false
		}
	},
	{
		RK_PQ_CSC_RGB2RGB, "RGB F->RGB F",
		&rk_csc_table_identity_rgb_to_rgb,
		&rk_dc_csc_table_identity_rgb_to_rgb2,
		{
			OPTM_CS_E_RGB, OPTM_CS_E_RGB, true, true
		}
	},
	{
		RK_PQ_CSC_YUV2RGB_2020, "YUV2020 F->RGB2020 F",
		&rk_csc_table_identity_yuv_to_rgb_2020,
		&rk_dc_csc_table_identity_yuv_to_rgb_2020,
		{
			OPTM_CS_E_XV_YCC_2020, OPTM_CS_E_RGB_2020, true, true
		}
	},
	{
		RK_PQ_CSC_RGB2YUV2020_LIMIT2FULL, "RGB2020 L->YUV2020 F",
		&rk_csc_table_identity_rgb_limit_to_yuv_full_2020,
		&rk_dc_csc_table_identity_rgb_limit_to_yuv_full_2020,
		{
			OPTM_CS_E_RGB_2020, OPTM_CS_E_XV_YCC_2020, false, true
		}
	},
	{
		RK_PQ_CSC_RGB2YUV2020_LIMIT, "RGB2020 L->YUV2020 L",
		&rk_csc_table_identity_rgb_limit_to_yuv_limit_2020,
		&rk_dc_csc_table_identity_rgb_limit_to_yuv_limit_2020,
		{
			OPTM_CS_E_RGB_2020, OPTM_CS_E_XV_YCC_2020, false, false
		}
	},
	{
		RK_PQ_CSC_RGB2YUV2020_FULL2LIMIT, "RGB2020 F->YUV2020 L",
		&rk_csc_table_identity_rgb_full_to_yuv_limit_2020,
		&rk_dc_csc_table_identity_rgb_full_to_yuv_limit_2020,
		{
			OPTM_CS_E_RGB_2020, OPTM_CS_E_XV_YCC_2020, true, false
		}
	},
	{
		RK_PQ_CSC_RGB2YUV2020_FULL, "RGB2020 F->YUV2020 F",
		&rk_csc_table_identity_rgb_full_to_yuv_full_2020,
		&rk_dc_csc_table_identity_rgb_full_to_yuv_full_2020,
		{
			OPTM_CS_E_RGB_2020, OPTM_CS_E_XV_YCC_2020, true, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV, "YUV 601 L->YUV 601 L",
		&rk_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr,
		&rk_dc_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_XV_YCC_601, false, false
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_FULL, "YUV 601 F->YUV 601 F",
		&rk_csc_table_identity_y_cb_cr_to_y_cb_cr,
		&rk_dc_csc_table_hdy_cb_cr_full_to_xv_yccsdy_cb_cr_full,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_XV_YCC_601, true, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_LIMIT2FULL, "YUV 601 L->YUV 601 F",
		&rk_csc_table_identity_y_cb_cr_limit_to_y_cb_cr_full,
		&rk_dc_csc_table_identity_y_cb_cr_limit_to_y_cb_cr_full,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_XV_YCC_601,  false, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_FULL2LIMIT, "YUV 601 F->YUV 601 L",
		&rk_csc_table_identity_y_cb_cr_full_to_y_cb_cr_limit,
		&rk_dc_csc_table_identity_y_cb_cr_full_to_y_cb_cr_limit,
		{
			OPTM_CS_E_XV_YCC_601, OPTM_CS_E_XV_YCC_601, true, false
		}
	},
	{
		RK_PQ_CSC_YUV2YUV, "YUV 2020 L->YUV 2020 L",
		&rk_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr,
		&rk_dc_csc_table_xv_yccsdy_cb_cr_to_hdy_cb_cr,
		{
			OPTM_CS_E_XV_YCC_2020, OPTM_CS_E_XV_YCC_2020, false, false
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_FULL, "YUV 2020 F->YUV 2020 F",
		&rk_csc_table_identity_y_cb_cr_to_y_cb_cr,
		&rk_dc_csc_table_hdy_cb_cr_full_to_xv_yccsdy_cb_cr_full,
		{
			OPTM_CS_E_XV_YCC_2020, OPTM_CS_E_XV_YCC_2020, true, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_LIMIT2FULL, "YUV 2020 L->YUV 2020 F",
		&rk_csc_table_identity_y_cb_cr_limit_to_y_cb_cr_full,
		&rk_dc_csc_table_identity_y_cb_cr_limit_to_y_cb_cr_full,
		{
			OPTM_CS_E_XV_YCC_2020, OPTM_CS_E_XV_YCC_2020, false, true
		}
	},
	{
		RK_PQ_CSC_YUV2YUV_FULL2LIMIT, "YUV 2020 F->YUV 2020 L",
		&rk_csc_table_identity_y_cb_cr_full_to_y_cb_cr_limit,
		&rk_dc_csc_table_identity_y_cb_cr_full_to_y_cb_cr_limit,
		{
			OPTM_CS_E_XV_YCC_2020, OPTM_CS_E_XV_YCC_2020, true, false
		}
	},
	{
		RK_PQ_CSC_RGB2RGBL, "RGB 2020 F->RGB 2020 L",
		&rk_csc_table_identity_rgb_to_rgb_limit,
		&rk_dc_csc_table_identity_rgb_to_rgb_limit,
		{
			OPTM_CS_E_RGB_2020, OPTM_CS_E_RGB_2020, true, false
		}
	},
	{
		RK_PQ_CSC_RGBL2RGB, "RGB 2020 L->RGB 2020 F",
		&rk_csc_table_identity_rgb_limit_to_rgb,
		&rk_dc_csc_table_identity_rgb_limit_to_rgb,
		{
			OPTM_CS_E_RGB_2020, OPTM_CS_E_RGB_2020, false, true
		}
	},
	{
		RK_PQ_CSC_RGBL2RGBL, "RGB 2020 L->RGB 2020 L",
		&rk_csc_table_identity_rgb_to_rgb,
		&rk_dc_csc_table_identity_rgb_to_rgb1,
		{
			OPTM_CS_E_RGB_2020, OPTM_CS_E_RGB_2020, false, false
		}
	},
	{
		RK_PQ_CSC_RGB2RGB, "RGB 2020 F->RGB 2020 F",
		&rk_csc_table_identity_rgb_to_rgb,
		&rk_dc_csc_table_identity_rgb_to_rgb2,
		{
			OPTM_CS_E_RGB_2020, OPTM_CS_E_RGB_2020, true, true
		}
	},
};

enum vop_csc_format {
	CSC_BT601L,
	CSC_BT709L,
	CSC_BT601F,
	CSC_BT2020,
	CSC_BT709L_13BIT,
	CSC_BT709F_13BIT,
	CSC_BT2020L_13BIT,
	CSC_BT2020F_13BIT,
	CSC_RGBL2BT709F_13BIT,
	CSC_RGBL2BT2020F_13BIT,
};

enum vop_csc_mode {
	CSC_RGB,
	CSC_YUV,
};

struct csc_mapping {
	enum vop_csc_format csc_format;
	enum color_space_type rgb_color_space;
	enum color_space_type yuv_color_space;
	bool rgb_full_range;
	bool yuv_full_range;
};

static const struct csc_mapping csc_mapping_table[] = {
	{
		CSC_BT601L,
		OPTM_CS_E_RGB,
		OPTM_CS_E_XV_YCC_601,
		true,
		false,
	},
	{
		CSC_BT709L,
		OPTM_CS_E_RGB,
		OPTM_CS_E_XV_YCC_709,
		true,
		false,
	},
	{
		CSC_BT601F,
		OPTM_CS_E_RGB,
		OPTM_CS_E_XV_YCC_601,
		true,
		true,
	},
	{
		CSC_BT2020,
		OPTM_CS_E_RGB_2020,
		OPTM_CS_E_XV_YCC_2020,
		true,
		true,
	},
	{
		CSC_BT709L_13BIT,
		OPTM_CS_E_RGB,
		OPTM_CS_E_XV_YCC_709,
		true,
		false,
	},
	{
		CSC_BT709F_13BIT,
		OPTM_CS_E_RGB,
		OPTM_CS_E_XV_YCC_709,
		true,
		true,
	},
	{
		CSC_RGBL2BT709F_13BIT,
		OPTM_CS_E_RGB,
		OPTM_CS_E_XV_YCC_709,
		false,
		true,
	},
	{
		CSC_BT2020L_13BIT,
		OPTM_CS_E_RGB_2020,
		OPTM_CS_E_XV_YCC_2020,
		true,
		false,
	},
	{
		CSC_BT2020F_13BIT,
		OPTM_CS_E_RGB_2020,
		OPTM_CS_E_XV_YCC_2020,
		true,
		true,
	},
	{
		CSC_RGBL2BT2020F_13BIT,
		OPTM_CS_E_RGB_2020,
		OPTM_CS_E_XV_YCC_2020,
		false,
		true,
	},
};

static bool is_rgb_format(u64 format)
{
	switch (format) {
	case BUS_FMT_YUV420:
	case BUS_FMT_YUV422:
	case BUS_FMT_YUV444:
		return false;
	case BUS_FMT_RGB:
	default:
		return true;
	}
}

struct post_csc_coef {
	s32 csc_coef00;
	s32 csc_coef01;
	s32 csc_coef02;
	s32 csc_coef10;
	s32 csc_coef11;
	s32 csc_coef12;
	s32 csc_coef20;
	s32 csc_coef21;
	s32 csc_coef22;

	s32 csc_dc0;
	s32 csc_dc1;
	s32 csc_dc2;

	u32 range_type;
};

static int csc_get_mode_index(int post_csc_mode, bool is_input_yuv, bool is_output_yuv)
{
	const struct rk_csc_colorspace_info *colorspace_info;
	enum color_space_type input_color_space;
	enum color_space_type output_color_space;
	bool is_input_full_range;
	bool is_output_full_range;
	int i;

	for (i = 0; i < ARRAY_SIZE(csc_mapping_table); i++) {
		if (post_csc_mode == csc_mapping_table[i].csc_format) {
			input_color_space = is_input_yuv ? csc_mapping_table[i].yuv_color_space :
					    csc_mapping_table[i].rgb_color_space;
			is_input_full_range = is_input_yuv ? csc_mapping_table[i].yuv_full_range :
					      csc_mapping_table[i].rgb_full_range;
			output_color_space = is_output_yuv ? csc_mapping_table[i].yuv_color_space :
					     csc_mapping_table[i].rgb_color_space;
			is_output_full_range = is_output_yuv ? csc_mapping_table[i].yuv_full_range :
					       csc_mapping_table[i].rgb_full_range;
			break;
		}
	}
	if (i >= ARRAY_SIZE(csc_mapping_table))
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(g_mode_csc_coef); i++) {
		colorspace_info = &g_mode_csc_coef[i].st_csc_color_info;
		if (colorspace_info->input_color_space == input_color_space &&
		    colorspace_info->output_color_space == output_color_space &&
		    colorspace_info->in_full_range == is_input_full_range &&
		    colorspace_info->out_full_range == is_output_full_range)
			return i;
	}

	return -EINVAL;
}

static void __maybe_unused csc_matrix_multiply(struct rk_pq_csc_coef *dst,
				const struct rk_pq_csc_coef *m0,
				const struct rk_pq_csc_coef *m1)
{
	dst->csc_coef00 = m0->csc_coef00 * m1->csc_coef00 +
			  m0->csc_coef01 * m1->csc_coef10 +
			  m0->csc_coef02 * m1->csc_coef20;

	dst->csc_coef01 = m0->csc_coef00 * m1->csc_coef01 +
			  m0->csc_coef01 * m1->csc_coef11 +
			  m0->csc_coef02 * m1->csc_coef21;

	dst->csc_coef02 = m0->csc_coef00 * m1->csc_coef02 +
			  m0->csc_coef01 * m1->csc_coef12 +
			  m0->csc_coef02 * m1->csc_coef22;

	dst->csc_coef10 = m0->csc_coef10 * m1->csc_coef00 +
			  m0->csc_coef11 * m1->csc_coef10 +
			  m0->csc_coef12 * m1->csc_coef20;

	dst->csc_coef11 = m0->csc_coef10 * m1->csc_coef01 +
			  m0->csc_coef11 * m1->csc_coef11 +
			  m0->csc_coef12 * m1->csc_coef21;

	dst->csc_coef12 = m0->csc_coef10 * m1->csc_coef02 +
			  m0->csc_coef11 * m1->csc_coef12 +
			  m0->csc_coef12 * m1->csc_coef22;

	dst->csc_coef20 = m0->csc_coef20 * m1->csc_coef00 +
			  m0->csc_coef21 * m1->csc_coef10 +
			  m0->csc_coef22 * m1->csc_coef20;

	dst->csc_coef21 = m0->csc_coef20 * m1->csc_coef01 +
			  m0->csc_coef21 * m1->csc_coef11 +
			  m0->csc_coef22 * m1->csc_coef21;

	dst->csc_coef22 = m0->csc_coef20 * m1->csc_coef02 +
			  m0->csc_coef21 * m1->csc_coef12 +
			  m0->csc_coef22 * m1->csc_coef22;
}

static void csc_matrix_ventor_multiply(struct rk_pq_csc_ventor *dst,
				       const struct rk_pq_csc_coef *m0,
				       const struct rk_pq_csc_ventor *v0)
{
	dst->csc_offset0 = m0->csc_coef00 * v0->csc_offset0 +
			   m0->csc_coef01 * v0->csc_offset1 +
			   m0->csc_coef02 * v0->csc_offset2;

	dst->csc_offset1 = m0->csc_coef10 * v0->csc_offset0 +
			   m0->csc_coef11 * v0->csc_offset1 +
			   m0->csc_coef12 * v0->csc_offset2;

	dst->csc_offset2 = m0->csc_coef20 * v0->csc_offset0 +
			   m0->csc_coef21 * v0->csc_offset1 +
			   m0->csc_coef22 * v0->csc_offset2;
}

static inline s32 pq_csc_simple_round(s32 x, s32 n)
{
	s32 value = 0;

	if (n == 0)
		return x;

	value = (abs(x) + (1 << (n - 1))) >> (n);
	return (((x) >= 0) ? value : -value);
}

static void rockchip_swap_color_channel(bool is_input_yuv, bool is_output_yuv,
					struct post_csc_coef *csc_simple_coef,
					struct rk_pq_csc_coef *out_matrix,
					struct rk_pq_csc_ventor *out_dc)
{
	csc_simple_coef->csc_coef00 = out_matrix->csc_coef00;
	csc_simple_coef->csc_coef01 = out_matrix->csc_coef01;
	csc_simple_coef->csc_coef02 = out_matrix->csc_coef02;
	csc_simple_coef->csc_coef10 = out_matrix->csc_coef10;
	csc_simple_coef->csc_coef11 = out_matrix->csc_coef11;
	csc_simple_coef->csc_coef12 = out_matrix->csc_coef12;
	csc_simple_coef->csc_coef20 = out_matrix->csc_coef20;
	csc_simple_coef->csc_coef21 = out_matrix->csc_coef21;
	csc_simple_coef->csc_coef22 = out_matrix->csc_coef22;
	csc_simple_coef->csc_dc0 = out_dc->csc_offset0;
	csc_simple_coef->csc_dc1 = out_dc->csc_offset1;
	csc_simple_coef->csc_dc2 = out_dc->csc_offset2;
}

static int csc_calc_default_output_coef(const struct rk_csc_mode_coef *csc_mode_cfg,
					struct rk_pq_csc_coef *out_matrix,
					struct rk_pq_csc_ventor *out_dc)
{
	const struct rk_pq_csc_coef *csc_coef;
	const struct rk_pq_csc_dc_coef *csc_dc_coef;
	struct rk_pq_csc_ventor dc_in_ventor;
	struct rk_pq_csc_ventor dc_out_ventor;
	struct rk_pq_csc_ventor v;

	csc_coef = csc_mode_cfg->pst_csc_coef;
	csc_dc_coef = csc_mode_cfg->pst_csc_dc_coef;

	out_matrix->csc_coef00 = csc_coef->csc_coef00;
	out_matrix->csc_coef01 = csc_coef->csc_coef01;
	out_matrix->csc_coef02 = csc_coef->csc_coef02;
	out_matrix->csc_coef10 = csc_coef->csc_coef10;
	out_matrix->csc_coef11 = csc_coef->csc_coef11;
	out_matrix->csc_coef12 = csc_coef->csc_coef12;
	out_matrix->csc_coef20 = csc_coef->csc_coef20;
	out_matrix->csc_coef21 = csc_coef->csc_coef21;
	out_matrix->csc_coef22 = csc_coef->csc_coef22;

	dc_in_ventor.csc_offset0 = csc_dc_coef->csc_in_dc0;
	dc_in_ventor.csc_offset1 = csc_dc_coef->csc_in_dc1;
	dc_in_ventor.csc_offset2 = csc_dc_coef->csc_in_dc2;
	dc_out_ventor.csc_offset0 = csc_dc_coef->csc_out_dc0;
	dc_out_ventor.csc_offset1 = csc_dc_coef->csc_out_dc1;
	dc_out_ventor.csc_offset2 = csc_dc_coef->csc_out_dc2;

	csc_matrix_ventor_multiply(&v, csc_coef, &dc_in_ventor);
	out_dc->csc_offset0 = v.csc_offset0 + dc_out_ventor.csc_offset0 *
			      PQ_CSC_SIMPLE_MAT_PARAM_FIX_NUM;
	out_dc->csc_offset1 = v.csc_offset1 + dc_out_ventor.csc_offset1 *
			      PQ_CSC_SIMPLE_MAT_PARAM_FIX_NUM;
	out_dc->csc_offset2 = v.csc_offset2 + dc_out_ventor.csc_offset2 *
			      PQ_CSC_SIMPLE_MAT_PARAM_FIX_NUM;

	return 0;
}

static int vop2_convert_csc_mode(int csc_mode, int bit_depth, int range)
{
	switch (csc_mode) {
	case V4L2_COLORSPACE_SMPTE170M:
	case V4L2_COLORSPACE_470_SYSTEM_M:
	case V4L2_COLORSPACE_470_SYSTEM_BG:
		return CSC_BT601L;
	case V4L2_COLORSPACE_REC709:
	case V4L2_COLORSPACE_SMPTE240M:
	case V4L2_COLORSPACE_DEFAULT:
		if (bit_depth == CSC_13BIT_DEPTH)
			return CSC_BT709L_13BIT;
		else
			return CSC_BT709L;
	case V4L2_COLORSPACE_JPEG:
		return CSC_BT601F;
	case V4L2_COLORSPACE_BT2020:
		if (bit_depth == CSC_13BIT_DEPTH)
			return CSC_BT2020L_13BIT;
		else
			return CSC_BT2020;
	case V4L2_COLORSPACE_BT709F:
		if (bit_depth == CSC_10BIT_DEPTH)
			return CSC_BT601F;
		else if (range == CSC_FULL_RANGE)
			return CSC_BT709F_13BIT;
		else
			return CSC_RGBL2BT709F_13BIT;
	case V4L2_COLORSPACE_BT2020F:
		if (bit_depth == CSC_10BIT_DEPTH)
			return CSC_BT601F;
		else if (range == CSC_FULL_RANGE)
			return CSC_BT2020F_13BIT;
		else
			return CSC_RGBL2BT2020F_13BIT;
	default:
		return CSC_BT709L;
	}
}

static int rockchip_calc_post_csc(struct post_csc_coef *csc_simple_coef,
				  int csc_mode, bool is_input_yuv, bool is_output_yuv)
{
	int ret = 0;
	struct rk_pq_csc_coef out_matrix;
	struct rk_pq_csc_ventor out_dc;
	const struct rk_csc_mode_coef *csc_mode_cfg;
	int bit_num = PQ_CSC_SIMPLE_MAT_PARAM_FIX_BIT_WIDTH;

	ret = csc_get_mode_index(csc_mode, is_input_yuv, is_output_yuv);
	if (ret < 0)
		return ret;

	csc_mode_cfg = &g_mode_csc_coef[ret];


	ret = csc_calc_default_output_coef(csc_mode_cfg, &out_matrix, &out_dc);

	rockchip_swap_color_channel(is_input_yuv, is_output_yuv, csc_simple_coef, &out_matrix,
				    &out_dc);

	csc_simple_coef->csc_dc0 = pq_csc_simple_round(csc_simple_coef->csc_dc0, bit_num);
	csc_simple_coef->csc_dc1 = pq_csc_simple_round(csc_simple_coef->csc_dc1, bit_num);
	csc_simple_coef->csc_dc2 = pq_csc_simple_round(csc_simple_coef->csc_dc2, bit_num);
	csc_simple_coef->range_type = csc_mode_cfg->st_csc_color_info.out_full_range;

	return ret;
}

static void rk628_post_process_csc(struct rk628 *rk628)
{
	enum bus_format in_fmt, out_fmt;
	struct post_csc_coef csc_coef = {};
	bool is_input_yuv, is_output_yuv;
	u32 color_space = V4L2_COLORSPACE_BT709F;
	u32 csc_mode;
	u32 val;
	int range_type;
	int color_range;

	in_fmt = rk628_hdmirx_get_format(rk628);
	out_fmt = rk628->tx_mode ? BUS_FMT_RGB : BUS_FMT_YUV422;

	if (rk628->version == RK628D_VERSION) {
		if (in_fmt == BUS_FMT_RGB)
			rk628_i2c_write(rk628, GRF_CSC_CTRL_CON, SW_R2Y_EN(1));
		else if (out_fmt == BUS_FMT_RGB)
			rk628_i2c_write(rk628, GRF_CSC_CTRL_CON, SW_Y2R_EN(1));
	} else {
		color_range = rk628_hdmirx_get_range(rk628);
		csc_mode = vop2_convert_csc_mode(color_space, CSC_13BIT_DEPTH, color_range);

		is_input_yuv = !is_rgb_format(in_fmt);
		is_output_yuv = !is_rgb_format(out_fmt);
		rockchip_calc_post_csc(&csc_coef, csc_mode, is_input_yuv, is_output_yuv);

		val = ((csc_coef.csc_coef01 & 0xffff) << 16) | (csc_coef.csc_coef00 & 0xffff);
		rk628_i2c_write(rk628, GRF_CSC_MATRIX_COE01_COE00, val);

		val = ((csc_coef.csc_coef10 & 0xffff) << 16) | (csc_coef.csc_coef02 & 0xffff);
		rk628_i2c_write(rk628, GRF_CSC_MATRIX_COE10_COE02, val);

		val = ((csc_coef.csc_coef12 & 0xffff) << 16) | (csc_coef.csc_coef11 & 0xffff);
		rk628_i2c_write(rk628, GRF_CSC_MATRIX_COE12_COE11, val);

		val = ((csc_coef.csc_coef21 & 0xffff) << 16) | (csc_coef.csc_coef20 & 0xffff);
		rk628_i2c_write(rk628, GRF_CSC_MATRIX_COE21_COE20, val);

		rk628_i2c_write(rk628, GRF_CSC_MATRIX_COE22, csc_coef.csc_coef22);

		rk628_i2c_write(rk628, GRF_CSC_MATRIX_OFFSET0, csc_coef.csc_dc0);
		rk628_i2c_write(rk628, GRF_CSC_MATRIX_OFFSET1, csc_coef.csc_dc1);
		rk628_i2c_write(rk628, GRF_CSC_MATRIX_OFFSET2, csc_coef.csc_dc2);

		range_type = csc_coef.range_type ? 0 : 1;
		range_type <<= is_input_yuv ? 0 : 1;
		val = SW_Y2R_MODE(range_type) | SW_FROM_CSC_MATRIX_EN(1);
		rk628_i2c_write(rk628, GRF_CSC_CTRL_CON, val);
	}
}

void rk628_post_process_csc_en(struct rk628 *rk628)
{
	rk628_post_process_csc(rk628);
	rk628_i2c_write(rk628, GRF_SCALER_CON0, SCL_EN(1));
}
EXPORT_SYMBOL(rk628_post_process_csc_en);

void rk628_post_process_csc_dis(struct rk628 *rk628)
{
	rk628_i2c_write(rk628, GRF_SCALER_CON0, SCL_EN(0));
}
EXPORT_SYMBOL(rk628_post_process_csc_dis);

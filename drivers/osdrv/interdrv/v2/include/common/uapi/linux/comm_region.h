/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/comm_region.h
 * Description:
 *   Common region definitions.
 */

#ifndef __COMM_REGION_H__
#define __COMM_REGION_H__

#include <linux/common.h>
#include <linux/comm_video.h>
#include <linux/defines.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define RGN_COLOR_LUT_NUM 2
#define RGN_BATCHHANDLE_MAX 24
#define RGN_INVALID_HANDLE (-1U)
#define RGN_CMPR_MIN_SIZE 128000

typedef unsigned int rgn_handle;
typedef unsigned int rgn_handlegroup;

/* type of video regions */
typedef enum _rgn_type_e {
	OVERLAY_RGN = 0,
	COVER_RGN,
	COVEREX_RGN,
	OVERLAYEX_RGN,
	MOSAIC_RGN,
	RGN_BUTT
} rgn_type_e;

typedef enum _rgn_area_type_e {
	AREA_RECT = 0,
	AREA_QUAD_RANGLE,
	AREA_BUTT
} rgn_area_type_e;

/*
 * RGN_ABS_COOR: Absolute coordinate.
 * RGN_RATIO_COOR: Ratio coordinate.
 */
typedef enum _rgn_coordinate_e {
	RGN_ABS_COOR = 0,
	RGN_RATIO_COOR
} rgn_coordinate_e;

/*
 * bSolid: whether solid or dashed quadrangle
 * thick: Line Width of quadrangle, valid when dashed quadrangle
 * point[4]: points of quadrilateral
 */
typedef struct _rgn_quadrangle_s {
	unsigned char bSolid;
	unsigned int thick;
	point_s point[4];
} rgn_quadrangle_s;

/*
 * cover_type: rect or arbitrary quadrilateral COVER
 * rect: config of rect
 * stQuadRangle: config of arbitrary quadrilateral COVER
 * color: color of region.
 * layer: COVER region layer
 * coordinate: ratio coordiante or abs coordinate
 */
typedef struct _cover_chn_attr_s {
	rgn_area_type_e cover_type;
	union {
		rect_s rect;
		rgn_quadrangle_s stQuadRangle;
	};
	unsigned int color;
	unsigned int layer;
	rgn_coordinate_e coordinate;
} cover_chn_attr_s;

/*
 * cover_type: rect or arbitrary quadrilateral COVER
 * rect: config of rect
 * stQuadRangle: config of arbitrary quadrilateral COVER
 * color: color of region.
 * layer: COVER region layer
 */
typedef struct _coverex_chn_attr_s {
	rgn_area_type_e cover_type;
	union {
		rect_s rect;
		rgn_quadrangle_s stQuadRangle;
	};
	unsigned int color;
	unsigned int layer;
} coverex_chn_attr_s;

typedef enum _mosaic_blk_size_e {
	MOSAIC_BLK_SIZE_8 = 0, /* block size 8*8 of MOSAIC */
	MOSAIC_BLK_SIZE_16, /* block size 16*16 of MOSAIC */
	MOSAIC_BLK_SIZE_BUTT
} mosaic_blk_size_e;

/*
 * rect: config of rect
 * blk_size: block size of MOSAIC
 * layer: MOSAIC region layer range
 */
typedef struct _mosaic_chn_attr_s {
	rect_s rect;
	mosaic_blk_size_e blk_size;
	unsigned int layer;
} mosaic_chn_attr_s;

/*
 * OSD_COMPRESS_MODE_NONE : No compression
 * OSD_COMPRESS_MODE_SW : data from SW compressor
 * OSD_COMPRESS_MODE_HW : data from HW compressor
 */
typedef enum _osd_compress_mode_e {
	OSD_COMPRESS_MODE_NONE = 0,
	OSD_COMPRESS_MODE_SW,
	OSD_COMPRESS_MODE_HW,
	OSD_COMPRESS_MODE_BUTT
} osd_compress_mode_e;

/*
 * osd_compress_mode : Check the VGOP input data from SW/HW compressor.
 * u32EstCompressSize : Estimate bitstream size(SW compressor) for ion_alloc.
 * u32CompressSize : Real bitstream size(HW compressor) for ion_alloc.
 */
typedef struct _osd_compress_info_s {
	osd_compress_mode_e osd_compress_mode;
	unsigned int Est_compressed_size;
	unsigned int compressed_size;
} osd_compress_info_s;

/*
 * bg_color: background color, format depends on "pixel_format"
 * size: region size
 * canvas_num: num of canvas. 2 for double buffer.
 */
typedef struct _overlay_attr_s {
	pixel_format_e pixel_format;
	unsigned int bg_color;
	size_s size;
	unsigned int canvas_num;
	osd_compress_info_s compress_info;
} overlay_attr_s;

/*
 * bg_color: background color, format depends on "pixel_format"
 * size: region size
 * canvas_num: num of canvas. 2 for double buffer.
 */
typedef struct _overlay_ex_attr_s {
	pixel_format_e pixel_format;
	unsigned int bg_color;
	size_s size;
	unsigned int canvas_num;
	osd_compress_info_s compress_info;
} overlay_ex_attr_s;

typedef enum _invert_color_mode_e {
	LESSTHAN_LUM_THRESH = 0, /* the lum of the video is less than the lum threshold which is set by lum_thresh  */
	MORETHAN_LUM_THRESH,     /* the lum of the video is more than the lum threshold which is set by lum_thresh  */
	INVERT_COLOR_BUTT
} invert_color_mode_e;

typedef struct _overlay_invert_color_s {
	size_s inv_col_area;
	unsigned int lum_thresh;
	invert_color_mode_e chg_mod;
	unsigned char inv_col_en;  /* The switch of inverting color. */
} overlay_invert_color_s;

/*
 * point: position of region.
 * layer: region layer.
 */
typedef struct _overlay_chn_attr_s {
	point_s point;
	unsigned int layer;
	overlay_invert_color_s invert_color;
} overlay_chn_attr_s;

/*
 * point: position of region.
 * layer: region layer.
 */
typedef struct _overlayex_chn_attr_s {
	point_s point;
	unsigned int layer;
	overlay_invert_color_s invert_color;
} overlayex_chn_attr_s;

typedef union _rgn_attr_u {
	overlay_attr_s overlay; /* attribute of overlay region */
	overlay_ex_attr_s overlay_ex; /* attribute of overlayex region */
} rgn_attr_u;

/* attribute of a region.
 *
 * type: region type.
 * unattr: region attribute.
 */
typedef struct _rgn_attr_s {
	rgn_type_e type;
	rgn_attr_u unattr;
} rgn_attr_s;

/*
 * overlay_chn: attribute of overlay region
 * cover_chn: attribute of cover region
 * cover_ex_chn: attribute of coverex region
 * overlay_ex_chn: attribute of overlayex region
 * mosaic_chn: attribute of mosic region
 */
typedef union _rgn_chn_attr_u {
	overlay_chn_attr_s overlay_chn;
	cover_chn_attr_s cover_chn;
	coverex_chn_attr_s cover_ex_chn;
	overlayex_chn_attr_s overlay_ex_chn;
	mosaic_chn_attr_s mosaic_chn;
} rgn_chn_attr_u;

/* attribute of a region
 *
 * show: region show or not.
 * type: region type.
 * unchn_attr: region attribute.
 */
typedef struct _rgn_chn_attr_s {
	unsigned char show;
	rgn_type_e type;
	rgn_chn_attr_u unchn_attr;
} rgn_chn_attr_s;


typedef enum _rgn_cmpr_type_e {
	RGN_CMPR_RECT = 0,
	RGN_CMPR_BIT_MAP,
	RGN_CMPR_LINE,
	RGN_CMPR_BUTT
} rgn_cmpr_type_e;

typedef struct _rgn_line_attr_s {
	point_s point_start;
	point_s point_end;
	unsigned int thick;
	unsigned int color;
} rgn_line_attr_s;

typedef struct _rgn_rect_attr_s {
	rect_s rect;
	unsigned int thick;
	unsigned int color;
	unsigned int is_fill;
} rgn_rect_attr_s;

typedef struct _rgn_bitmap_attr_s {
	rect_s rect;
	uint64_t bitmap_paddr;
} rgn_bitmap_attr_s;

typedef struct _rgn_cmpr_obj_attr_s {
	rgn_cmpr_type_e obj_type;
	union {
		rgn_line_attr_s line;
		rgn_rect_attr_s rgn_rect;
		rgn_bitmap_attr_s bitmap;
	};
} rgn_cmpr_obj_attr_s;

typedef struct _rgn_canvas_cmpr_attr_s {
	unsigned int width;
	unsigned int height;
	unsigned int bg_color;
	pixel_format_e pixel_format;
	unsigned int bs_size;
	unsigned int obj_num;
} rgn_canvas_cmpr_attr_s;

typedef struct _rgn_canvas_info_s {
	uint64_t phy_addr;
	unsigned char *virt_addr;
#ifdef __arm__
	__u32 padding; /* padding for keeping same size of this structure */
#endif
	size_s size;
	unsigned int stride;
	pixel_format_e pixel_format;
	unsigned char compressed;
	unsigned int compressed_size;
	osd_compress_mode_e osd_compress_mode;
	rgn_canvas_cmpr_attr_s *pcanvas_cmpr_attr;
	rgn_cmpr_obj_attr_s *pobj_attr;
} rgn_canvas_info_s;

typedef struct _rgn_component_info_s {
	int alen;
	int rlen;
	int glen;
	int blen;
} rgn_component_info_s;

typedef struct _rgn_rgbquad {
	unsigned char argb_alpha;
	unsigned char argb_red;
	unsigned char argb_green;
	unsigned char argb_blue;
} rgn_rgbquad_s;

/* the color format OSD supported */
typedef enum _rgn_color_fmt_e {
	RGN_COLOR_FMT_RGB444 = 0,
	RGN_COLOR_FMT_RGB4444 = 1,
	RGN_COLOR_FMT_RGB555 = 2,
	RGN_COLOR_FMT_RGB565 = 3,
	RGN_COLOR_FMT_RGB1555 = 4,
	RGN_COLOR_FMT_RGB888 = 6,
	RGN_COLOR_FMT_RGB8888 = 7,
	RGN_COLOR_FMT_ARGB4444 = 8,
	RGN_COLOR_FMT_ARGB1555 = 9,
	RGN_COLOR_FMT_ARGB8888 = 10,
	RGN_COLOR_FMT_BUTT
} rgn_color_fmt_e;

typedef struct _rgn_palette {
	rgn_rgbquad_s *ppalette_table;
	unsigned short lut_length;
	rgn_color_fmt_e pixel_format;
} rgn_palette;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /* __COMM_REGION_H__ */

/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File name: include/comm_gdc.h
 * Description:
 *   Common gdc definitions.
 */

#ifndef __COMM_GDC_H__
#define __COMM_GDC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <linux/common.h>
#include <linux/comm_video.h>


#define FISHEYE_MAX_REGION_NUM 4 /*fisheye max region num*/
#define AFFINE_MAX_REGION_NUM 32 /*affine max region num*/


#ifdef __arm__
typedef int GDC_HANDLE; /*gdc handle*/
#else
typedef long long GDC_HANDLE; /*gdc handle*/
#endif

/*
 * img_in: input picture
 * img_out: output picture
 * privatedata[4]: rw; private data of task
 * reserved: rw; debug information,state of current picture
 * name: task name
 */
typedef struct _gdc_task_attr_s {
	video_frame_info_s img_in;
	video_frame_info_s img_out;
	unsigned long long privatedata[4];
	unsigned long long reserved;
	char name[32];
} gdc_task_attr_s;

/* Mount mode of device */
typedef enum _fisheye_mount_mode_e {
	FISHEYE_DESKTOP_MOUNT = 0,
	FISHEYE_CEILING_MOUNT = 1,
	FISHEYE_WALL_MOUNT = 2,
	FISHEYE_MOUNT_MODE_BUTT
} fisheye_mount_mode_e;

/* View mode of client*/
typedef enum _fisheye_view_mode_e {
	FISHEYE_VIEW_360_PANORAMA = 0,
	FISHEYE_VIEW_180_PANORAMA = 1,
	FISHEYE_VIEW_NORMAL = 2,
	FISHEYE_NO_TRANSFORMATION = 3,
	FISHEYE_VIEW_MODE_BUTT
} fisheye_view_mode_e;

/*Fisheye region correction attribute
 *
 * view_mode: gdc view mode
 * in_radius: inner radius of gdc correction region
 * out_radius: out radius of gdc correction region
 * pan: Range: [0, 360]
 * tilt: Range: [0, 360]
 * hor_zoom: Range: [1, 4095]
 * ver_zoom: Range: [1, 4095]
 * stoutrect: out Imge rectangle attribute
 */
typedef struct _fisheye_region_attr_s {
	fisheye_view_mode_e view_mode;
	unsigned int in_radius;
	unsigned int out_radius;
	unsigned int pan;
	unsigned int tilt;
	unsigned int hor_zoom;
	unsigned int ver_zoom;
	rect_s stoutrect;
} fisheye_region_attr_s;

/* fish eye usage mode*/
typedef enum _usage_mode {
	MODE_PANORAMA_360 = 1,
	MODE_PANORAMA_180 = 2,
	MODE_01_1O = 3,
	MODE_02_1O4R = 4,
	MODE_03_4R = 5,
	MODE_04_1P2R = 6,
	MODE_05_1P2R = 7,
	MODE_06_1P = 8,
	MODE_07_2P = 9,
	MODE_STEREO_FIT = 10,
	MODE_MAX
} usage_mode;

/* Fisheye all regions correction attribute
 *
 * enable: RW; Range: [0, 1], whether enable fisheye correction or not
 * enable_bg_color: RW; Range: [0, 1], whether use background color or not
 * bg_color: RW; Range: [0,0xffffff], the background color RGB888
 *
 * hor_offset: RW; Range: [-511, 511], the horizontal offset between image center and physical center of len
 * ver_offset: RW; Range: [-511, 511], the vertical offset between image center and physical center of len
 *
 * trapezoid_coef: RW; Range: [0, 32], strength coefficient of trapezoid correction
 * fan_strength: RW; Range: [-760, 760], strength coefficient of fan correction
 * mount_mode: gdc mount mode
 *
 * use_mode: easy scenario. If this isn't MODE_MAX, then reference this parameter and ignore following region attrs.
 * region_num: RW; Range: [1, FISHEYE_MAX_REGION_NUM], gdc correction region number
 * fisheye_region_attr: RW; attribute of gdc correction region
 * grid_info_attr: grid info attr
 */
typedef struct _fisheye_attr_s {
	unsigned char enable;
	unsigned char enable_bg_color;
	unsigned int bg_color;
	int hor_offset;
	int ver_offset;
	unsigned int trapezoid_coef;
	int fan_strength;
	fisheye_mount_mode_e mount_mode;
	usage_mode use_mode;
	unsigned int region_num;
	fisheye_region_attr_s fisheye_region_attr[FISHEYE_MAX_REGION_NUM];
	grid_info_attr_s grid_info_attr;
} fisheye_attr_s;

/* Warp attribute
 *
 * enable: RW; Range: [0, 1], whether enable fisheye correction or not
 * grid_info_attr: grid info attr
 */
typedef struct _warp_attr_s {
	unsigned char enable;
	grid_info_attr_s grid_info_attr;
} warp_attr_s;

/* Point coordinates*/
typedef struct _point2f_s {
	float x;
	float y;
} point2f_s;

/* Affine all regions correction attribute
 *
 * region_num: Range: [1, AFFINE_MAX_REGION_NUM], gdc region number
 * region_attr: source point of gdc affine region
 * dest_size: dest size of each affine region
 */
typedef struct _affine_attr_s {
	unsigned int region_num;
	point2f_s region_attr[AFFINE_MAX_REGION_NUM][4];
	size_s dest_size;
} affine_attr_s;

/* Buffer Wrap
 *
 * enable: Whether bufwrap is enabled.
 * buf_line: buffer line number.
 *             Support 64, 128, 192, 256.
 * wrap_buffer_size: buffer size.
 */
typedef struct _ldc_buf_wrap_s {
	unsigned char enable;
	unsigned int buf_line;
	unsigned int wrap_buffer_size;
} ldc_buf_wrap_s;

/* Vi gdc attribute
 *
 * chn: vi chnnel
 */
typedef struct _vi_mesh_attr_s {
	vi_chn chn;
} vi_mesh_attr_s;

/* Vpss gdc attribute
 *
 * grp: vpss group
 * chn: vpss chnnel
 */
typedef struct _vpss_mesh_attr_s {
	vpss_grp grp;
	vpss_chn chn;
} vpss_mesh_attr_s;

/* Mesh dump attribute
 *
 * in_file_name: mesh file name
 * mod_id: module id
 * vi_mesh_attr: vi gdc attribute
 * vpss_mesh_attr:  vpss gdc attribute
 */
typedef struct _mesh_dump_attr_s {
	char in_file_name[128];
	mod_id_e mod_id;
	union {
		vi_mesh_attr_s vi_mesh_attr;
		vpss_mesh_attr_s vpss_mesh_attr;
	};
} mesh_dump_attr_s;

/* Gdc identity attribute
 *
 * mod_id: identity name
 * mod_id: module id
 * id: identity id
 * sync_io: gdc job sync
 */
typedef struct _gdc_identity_attr_s {
	char name[32];
	mod_id_e mod_id;
	unsigned int id;
	unsigned char sync_io;
} gdc_identity_attr_s;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __CVI_COMM_GDC_H__ */

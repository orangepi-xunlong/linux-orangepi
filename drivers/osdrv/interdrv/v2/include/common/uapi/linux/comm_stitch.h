/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File name: include/cvi_comm_stitch.h
 * Description:
 *   The common data type defination for Stitch module.
 */

#ifndef __COMM_STITCH_H__
#define __COMM_STITCH_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <linux/common.h>
#include <linux/comm_video.h>
#include <linux/defines.h>

#define STITCH_R_IDX 0
#define STITCH_G_IDX 1
#define STITCH_B_IDX 2

#define STITCH_ALIGN 16
#define STITCH_INVALID_GRP     (-1)

enum stitch_src_id {
	STITCH_SRC_ID_0 = 0,
	STITCH_SRC_ID_1,
	STITCH_SRC_ID_2,
	STITCH_SRC_ID_3,
	STITCH_SRC_ID_SEP,
};

enum stitch_wgt_mode {
	STITCH_WGT_YUV_SHARE = 0,
	STITCH_WGT_UV_SHARE,
	STITCH_WGT_SEP,
};

enum stitch_data_src {
	STITCH_DATA_SRC_DDR = 0,
	STITCH_DATA_SRC_VPSS,
	STITCH_DATA_SRC_SEP,
};

enum stitch_way_num {
	STITCH_2_WAY = 0,
	STITCH_4_WAY,
	STITCH_WAY_SEP,
};

//ovlp_lx[0]: overlap left x position of img12
//ovlp_rx[0]: overlap right x position of img12
//ovlp_lx[1]: overlap left x position of img23
//ovlp_rx[1]: overlap right x position of img23
//ovlp_lx[2]: overlap left x position of img34
//ovlp_rx[2]: overlap right x position of img34
typedef struct _stitch_src_ovlp_attr {
	short ovlp_lx[STITCH_MAX_SRC_NUM -1];
	short ovlp_rx[STITCH_MAX_SRC_NUM -1];
} stitch_src_ovlp_attr;

//bd_lx[0]: left boardWidth of img0
//bd_rx[0]: right boardWidth of img1
typedef struct _stitch_src_bd_attr {
	short bd_lx[STITCH_MAX_SRC_NUM];
	short bd_rx[STITCH_MAX_SRC_NUM];
} stitch_src_bd_attr;

typedef struct _stitch_src_attr {
	stitch_src_ovlp_attr ovlap_attr;
	stitch_src_bd_attr bd_attr;
	size_s size[STITCH_MAX_SRC_NUM];
	pixel_format_e fmt_in;
	enum stitch_way_num way_num;
} stitch_src_attr;

//phy_addr_wgt[0]: phy_addr_wgt12(alpha, beta)
//phy_addr_wgt[1]: phy_addr_wgt23(alpha, beta)
//phy_addr_wgt[2]: phy_addr_wgt34(alpha, beta)
//size_wgt[0]: size of wgt12(alpha, beta)
//size_wgt[1]: size of wgt23(alpha, beta)
//size_wgt[2]: size of wgt34(alpha, beta)
typedef struct _stitch_bld_wgt_attr {
	unsigned long long phy_addr_wgt[STITCH_MAX_SRC_NUM -1][2];
	//void *vir_addr_wgt[STITCH_MAX_SRC_NUM -1][2];
	size_s size_wgt[STITCH_MAX_SRC_NUM -1];
} stitch_bld_wgt_attr;

typedef struct _stitch_chn_attr {
	size_s size;
	pixel_format_e fmt_out;
} stitch_chn_attr;

typedef struct _stitch_op_attr {
	enum stitch_wgt_mode wgt_mode;
	enum stitch_data_src data_src;
} stitch_op_attr;



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /* __CVI_COMM_STITCH_H__ */

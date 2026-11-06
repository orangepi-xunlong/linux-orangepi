/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: ive_ioctl.h
 * Description:
 */

#ifndef __IVE_UAPI_H__
#define __IVE_UAPI_H__

// #include "cvi_comm_ive.h"
// #include "comm_errno.h"
struct ive_ioctl_arg {
	void *buffer;
	unsigned long input_data;
	uint32_t size;
};

struct ive_test_arg {
	ive_image_type_e type;
	char *addr;
#ifdef __arm__
	__u32 padding;
#endif
	uint16_t width;
	uint16_t height;
};

struct ive_query_arg {
	ive_handle ive_handle;
	unsigned char *pbFinish;
#ifdef __arm__
	__u32 padding;
#endif
	unsigned char bBlock;
};

struct ive_ioctl_add_arg {
	ive_handle ive_handle;
	ive_src_image_s src1;
	ive_src_image_s src2;
	ive_dst_image_s dst;
	ive_add_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_and_arg {
	ive_handle ive_handle;
	ive_src_image_s src1;
	ive_src_image_s src2;
	ive_dst_image_s dst;
	unsigned char instant;
};

struct ive_ioctl_xor_arg {
	ive_handle ive_handle;
	ive_src_image_s src1;
	ive_src_image_s src2;
	ive_dst_image_s dst;
	unsigned char instant;
};

struct ive_ioctl_or_arg {
	ive_handle ive_handle;
	ive_src_image_s src1;
	ive_src_image_s src2;
	ive_dst_image_s dst;
	unsigned char instant;
};

struct ive_ioctl_sub_arg {
	ive_handle ive_handle;
	ive_src_image_s src1;
	ive_src_image_s src2;
	ive_dst_image_s dst;
	ive_sub_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_erode_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_erode_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_dilate_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_dilate_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_thresh_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_thresh_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_match_bgmodel_arg {
	ive_handle ive_handle;
	ive_src_image_s cur_img;
	ive_data_s bg_model;
	ive_image_s fg_flag;
	ive_dst_image_s stDiffFg;
	ive_dst_mem_info_s stat_data;
	ive_match_bg_model_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_update_bgmodel_arg {
	ive_handle ive_handle;
	ive_src_image_s cur_img;
	ive_data_s bg_model;
	ive_image_s fg_flag;
	ive_dst_image_s bg_img;
	ive_dst_image_s chg_sta;
	ive_dst_mem_info_s stat_data;
	ive_update_bg_model_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_gmm_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s fg;
	ive_dst_image_s bg;
	ive_mem_info_s model;
	ive_gmm_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_gmm2_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_src_image_s factor;
	ive_dst_image_s fg;
	ive_dst_image_s bg;
	ive_dst_image_s stInfo;
	ive_mem_info_s model;
	ive_gmm2_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_dma_arg {
	ive_handle ive_handle;
	ive_data_s src;
	ive_dst_data_s dst;
	ive_dma_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_bernsen_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_bernsen_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_filter_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_filter_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_sobel_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst_h;
	ive_dst_image_s dst_v;
	ive_sobel_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_maganang_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst_mag;
	ive_dst_image_s dst_ang;
	ive_mag_and_ang_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_csc_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_csc_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_hist_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_mem_info_s dst;
	unsigned char instant;
};

struct ive_ioctl_filter_and_csc_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_filter_and_csc_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_map_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_src_mem_info_s map;
	ive_dst_image_s dst;
	ive_map_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_ncc_arg {
	ive_handle ive_handle;
	ive_src_image_s src1;
	ive_src_image_s src2;
	ive_dst_mem_info_s dst;
	unsigned char instant;
};

struct ive_ioctl_integ_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_mem_info_s dst;
	ive_integ_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_lbp_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_lbp_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_thresh_s16_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_thresh_s16_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_thresh_u16_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_thresh_u16_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_16bit_to_8bit_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_16bit_to_8bit_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_ord_stat_filter_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_ord_stat_filter_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_canny_hys_edge_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_dst_mem_info_s stack;
	ive_canny_hys_edge_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_norm_grad_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst_h;
	ive_dst_image_s dst_v;
	ive_dst_image_s dst_hv;
	ive_norm_grad_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_grad_fg_arg {
	ive_handle ive_handle;
	ive_src_image_s bg_diff_fg;
	ive_src_image_s cur_grad;
	ive_src_image_s bg_grad;
	ive_dst_image_s grad_fg;
	ive_grad_fg_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_sad_arg {
	ive_handle ive_handle;
	ive_src_image_s src1;
	ive_src_image_s src2;
	ive_dst_image_s sad;
	ive_dst_image_s thr;
	ive_sad_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_resize_arg {
	ive_handle ive_handle;
	ive_src_image_s src;
#ifdef __arm__
		__u32 padding1;
#endif
	ive_dst_image_s dst;
#ifdef __arm__
		__u32 padding2;
#endif
	ive_resize_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_ccl_arg{
	ive_handle ive_handle;
	ive_image_s src_dst;
	ive_dst_mem_info_s blob;
	ive_ccl_ctrl_s ccl_ctrl;
	unsigned char instant;
};

struct ive_ioctl_rgbPToYuvToErodeToDilate {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst1;
	ive_dst_image_s dst2;
	ive_filter_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_stcandicorner {
	ive_handle ive_handle;
	ive_src_image_s src;
	ive_dst_image_s dst;
	ive_st_candi_corner_ctrl_s ctrl;
	unsigned char instant;
};

struct ive_ioctl_md {
	ive_handle ive_handle;
	ive_src_image_s src1;
	ive_src_image_s src2;
	ive_dst_image_s dst;
	ive_frame_diff_motion_ctrl_s ctrl;
	unsigned char instant;
};

#define IVE_IOC_MAGIC 'v'
#define IVE_IOC_TEST _IOW(IVE_IOC_MAGIC, 0x00, unsigned long long)
#define IVE_IOC_DMA _IOW(IVE_IOC_MAGIC, 0x01, unsigned long long)
#define IVE_IOC_ADD _IOW(IVE_IOC_MAGIC, 0x02, unsigned long long)
#define IVE_IOC_AND _IOW(IVE_IOC_MAGIC, 0x03, unsigned long long)
#define IVE_IOC_OR _IOW(IVE_IOC_MAGIC, 0x04, unsigned long long)
#define IVE_IOC_SUB _IOW(IVE_IOC_MAGIC, 0x05, unsigned long long)
#define IVE_IOC_XOR _IOW(IVE_IOC_MAGIC, 0x06, unsigned long long)
#define IVE_IOC_THRESH _IOW(IVE_IOC_MAGIC, 0x07, unsigned long long)
#define IVE_IOC_THRESH_S16 _IOW(IVE_IOC_MAGIC, 0x08, unsigned long long)
#define IVE_IOC_THRESH_U16 _IOW(IVE_IOC_MAGIC, 0x09, unsigned long long)
#define IVE_IOC_16BIT_TO_8BIT _IOW(IVE_IOC_MAGIC, 0x0a, unsigned long long)
#define IVE_IOC_CSC _IOW(IVE_IOC_MAGIC, 0x0b, unsigned long long)
#define IVE_IOC_GRADFG _IOW(IVE_IOC_MAGIC, 0x0c, unsigned long long)
#define IVE_IOC_NORMGRAD _IOW(IVE_IOC_MAGIC, 0x0d, unsigned long long)
#define IVE_IOC_FILTER _IOW(IVE_IOC_MAGIC, 0x0e, unsigned long long)
#define IVE_IOC_FILTER_AND_CSC _IOW(IVE_IOC_MAGIC, 0x0f, unsigned long long)
#define IVE_IOC_HIST _IOW(IVE_IOC_MAGIC, 0x10, unsigned long long)
#define IVE_IOC_EQUALIZE_HIST _IOW(IVE_IOC_MAGIC, 0x11, unsigned long long)
#define IVE_IOC_MAP _IOW(IVE_IOC_MAGIC, 0x12, unsigned long long)
#define IVE_IOC_NCC _IOWR(IVE_IOC_MAGIC, 0x13, unsigned long long)
#define IVE_IOC_ORD_STAT_FILTER _IOW(IVE_IOC_MAGIC, 0x14, unsigned long long)
#define IVE_IOC_RESIZE _IOW(IVE_IOC_MAGIC, 0x15, unsigned long long)
#define IVE_IOC_CANNYHYSEDGE _IOW(IVE_IOC_MAGIC, 0x16, unsigned long long)
#define IVE_IOC_CANNYEDGE _IOW(IVE_IOC_MAGIC, 0x17, unsigned long long)
#define IVE_IOC_INTEG _IOW(IVE_IOC_MAGIC, 0x18, unsigned long long)
#define IVE_IOC_LBP _IOW(IVE_IOC_MAGIC, 0x19, unsigned long long)
#define IVE_IOC_MAG_AND_ANG _IOW(IVE_IOC_MAGIC, 0x1a, unsigned long long)
#define IVE_IOC_ST_CANDI_CORNER _IOW(IVE_IOC_MAGIC, 0x1b, unsigned long long)
#define IVE_IOC_ST_CORNER _IOW(IVE_IOC_MAGIC, 0x1c, unsigned long long)
#define IVE_IOC_SOBEL _IOW(IVE_IOC_MAGIC, 0x1d, unsigned long long)
#define IVE_IOC_CCL _IOW(IVE_IOC_MAGIC, 0x1e, unsigned long long)
#define IVE_IOC_DILATE _IOW(IVE_IOC_MAGIC, 0x1f, unsigned long long)
#define IVE_IOC_ERODE _IOW(IVE_IOC_MAGIC, 0x20, unsigned long long)
#define IVE_IOC_MATCH_BGMODEM _IOWR(IVE_IOC_MAGIC, 0x21, unsigned long long)
#define IVE_IOC_UPDATE_BGMODEL _IOWR(IVE_IOC_MAGIC, 0x22, unsigned long long)
#define IVE_IOC_GMM _IOW(IVE_IOC_MAGIC, 0x23, unsigned long long)
#define IVE_IOC_GMM2 _IOW(IVE_IOC_MAGIC, 0x24, unsigned long long)
#define IVE_IOC_LK_OPTICAL_FLOW_PYR                                           \
	_IOW(IVE_IOC_MAGIC, 0x25, unsigned long long)
#define IVE_IOC_SAD _IOW(IVE_IOC_MAGIC, 0x26, unsigned long long)
#define IVE_IOC_BERNSEN _IOW(IVE_IOC_MAGIC, 0x27, unsigned long long)
#define IVE_IOC_IMGIN_To_ODMA _IOW(IVE_IOC_MAGIC, 0x28, unsigned long long)
#define IVE_IOC_RGBP2YUV2ERODE2DILATE                                   \
	_IOW(IVE_IOC_MAGIC, 0x29, unsigned long long)
#define IVE_IOC_MD _IOW(IVE_IOC_MAGIC, 0x2a, unsigned long long)
#define IVE_IOC_CMDQ _IOW(IVE_IOC_MAGIC, 0x2b, unsigned long long)
#define IVE_IOC_RESET _IOW(IVE_IOC_MAGIC, 0xF0, unsigned long long)
#define IVE_IOC_DUMP _IO(IVE_IOC_MAGIC, 0xF1)
#define IVE_IOC_QUERY _IOWR(IVE_IOC_MAGIC, 0xF2, unsigned long long)
#endif /* __CVI_IVE_IOCTL_H__ */

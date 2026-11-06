#ifndef _COMM_IVE_H_
#define _COMM_IVE_H_

typedef void *ive_handle;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "comm_errno.h"

#define SUCCESS             0
#define CVI_SUCCESS_ALL_CHN     1
#define FAILURE             (-1)
#define FAILURE_ILLEGAL_PARAM (-2)
#define TRUE                1
#define FALSE               0

typedef unsigned char u0q8;
typedef unsigned char u1q7;
typedef unsigned char U5Q3;
typedef unsigned short u0q16;
typedef unsigned short u4q12;
typedef unsigned short u6q10;
typedef unsigned short charq8;
typedef unsigned short u9q7;
typedef unsigned short u12q4;
typedef unsigned short u14q2;
typedef unsigned short u5q11;
typedef short s9q7;
typedef short s14q2;
typedef short s1q15;
typedef unsigned int u22q10;
typedef unsigned int u25q7;
typedef unsigned int u21q11;
typedef int s25q7;
typedef int shortq16;
typedef unsigned short charq4f4;

typedef enum _ive_image_type_e {
	IVE_IMAGE_TYPE_U8C1 = 0x0,
	IVE_IMAGE_TYPE_S8C1 = 0x1,

	IVE_IMAGE_TYPE_YUV420SP = 0x2, /*YUV420 SemiPlanar*/
	IVE_IMAGE_TYPE_YUV422SP = 0x3, /*YUV422 SemiPlanar*/
	IVE_IMAGE_TYPE_YUV420P = 0x4, /*YUV420 Planar */
	IVE_IMAGE_TYPE_YUV422P = 0x5, /*YUV422 planar */

	IVE_IMAGE_TYPE_S8C2_PACKAGE = 0x6,
	IVE_IMAGE_TYPE_S8C2_PLANAR = 0x7,

	IVE_IMAGE_TYPE_S16C1 = 0x8,
	IVE_IMAGE_TYPE_U16C1 = 0x9,

	IVE_IMAGE_TYPE_U8C3_PACKAGE = 0xa,
	IVE_IMAGE_TYPE_U8C3_PLANAR = 0xb,

	IVE_IMAGE_TYPE_S32C1 = 0xc,
	IVE_IMAGE_TYPE_U32C1 = 0xd,

	IVE_IMAGE_TYPE_S64C1 = 0xe,
	IVE_IMAGE_TYPE_U64C1 = 0xf,

	IVE_IMAGE_TYPE_BF16C1 = 0x10,
	IVE_IMAGE_TYPE_FP32C1 = 0x11,
	IVE_IMAGE_TYPE_BUTT

} ive_image_type_e;

typedef struct CVI_IMG CVI_IMG_S;

typedef struct _ive_image_s {
	ive_image_type_e type;

	uint64_t phy_addr[3];
	uint64_t vir_addr[3];
	unsigned int stride[3];
	unsigned int width;
	unsigned int height;
	unsigned int reserved;
} ive_image_s;
typedef ive_image_s ive_src_image_s;
typedef ive_image_s ive_dst_image_s;

typedef struct _ive_mem_info_s {
	uint64_t phy_addr;
	uint64_t vir_addr;
	unsigned int size;
} ive_mem_info_s;
typedef ive_mem_info_s ive_src_mem_info_s;
typedef ive_mem_info_s ive_dst_mem_info_s;

typedef struct _ive_data_s {
	uint64_t phy_addr;
	uint64_t vir_addr;
	unsigned int stride;
	unsigned int width;
	unsigned int height;
	unsigned int reserved;
} ive_data_s;
typedef ive_data_s ive_src_data_s;
typedef ive_data_s ive_dst_data_s;

typedef union _ive_8bit_u {
	signed char s8_val;
	unsigned char u8_val;
} ive_8bit_u;

typedef struct _ive_point_u16_s {
	unsigned short x;
	unsigned short y;
} ive_point_u16_s;

typedef struct _ive_point_s16_s {
	unsigned short x;
	unsigned short y;
} ive_point_s16_s;

typedef struct _ive_point_s25q7_s {
	s25q7 x; /*X coordinate*/
	s25q7 y; /*Y coordinate*/
} ive_point_s25q7_s;

typedef struct _ive_rect_u16_s {
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
} ive_rect_u16_s;

typedef struct _ive_look_up_table_s {
	ive_mem_info_s table;
	unsigned short elem_num; /*LUT's elements number*/

	unsigned char tab_in_preci;
	unsigned char tab_out_norm;

	int tab_in_lower; /*LUT's original input lower limit*/
	int table_in_upper; /*LUT's original input upper limit*/
} ive_look_up_table_s;

typedef enum _en_ive_err_code_e {
	ERR_IVE_SYS_TIMEOUT = 0x40, /* IVE process timeout */
	ERR_IVE_QUERY_TIMEOUT = 0x41, /* IVE query timeout */
	ERR_IVE_OPEN_FILE = 0x42, /* IVE open file error */
	ERR_IVE_READ_FILE = 0x43, /* IVE read file error */
	ERR_IVE_WRITE_FILE = 0x44, /* IVE write file error */

	ERR_IVE_BUTT
} en_ive_err_code_e;

typedef enum _en_fd_err_code_e {
	ERR_FD_SYS_TIMEOUT = 0x40, /* FD process timeout */
	ERR_FD_CFG = 0x41, /* FD configuration error */
	ERR_FD_FACE_NUM_OVER = 0x42, /* FD candidate face number over*/
	ERR_FD_OPEN_FILE = 0x43, /* FD open file error */
	ERR_FD_READ_FILE = 0x44, /* FD read file error */
	ERR_FD_WRITE_FILE = 0x45, /* FD write file error */

	ERR_FD_BUTT
} en_fd_err_code_e;

#define ERR_IVE_INVALID_DEVID                                              \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
#define ERR_IVE_INVALID_CHNID                                              \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define ERR_IVE_ILLEGAL_PARAM                                              \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define ERR_IVE_EXIST                                                      \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
#define ERR_IVE_UNEXIST                                                    \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
#define ERR_IVE_NULL_PTR                                                   \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define ERR_IVE_NOT_CONFIG                                                 \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
#define IVE_NOT_SURPPORT                                               \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define ERR_IVE_NOT_PERM                                                   \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define ERR_IVE_NOMEM                                                      \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define ERR_IVE_NOBUF                                                      \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define ERR_IVE_BUF_EMPTY                                                  \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define ERR_IVE_BUF_FULL                                                   \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define ERR_IVE_NOTREADY                                                   \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define ERR_IVE_BADADDR                                                    \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
#define ERR_IVE_BUSY                                                       \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define ERR_IVE_SYS_TIMEOUT                                                \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_SYS_TIMEOUT)
#define ERR_IVE_QUERY_TIMEOUT                                              \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_QUERY_TIMEOUT)
#define ERR_IVE_OPEN_FILE                                                  \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_OPEN_FILE)
#define ERR_IVE_READ_FILE                                                  \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_READ_FILE)
#define ERR_IVE_WRITE_FILE                                                 \
	DEF_ERR(ID_IVE, EN_ERR_LEVEL_ERROR, ERR_IVE_WRITE_FILE)

#define ERR_FD_INVALID_DEVID                                               \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
#define ERR_FD_INVALID_CHNID                                               \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define ERR_FD_ILLEGAL_PARAM                                               \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define ERR_FD_EXIST                                                       \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
#define ERR_FD_UNEXIST                                                     \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
#define ERR_FD_NULL_PTR                                                    \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define ERR_FD_NOT_CONFIG                                                  \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
#define ERR_FD_NOT_SURPPORT                                                \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
#define ERR_FD_NOT_PERM                                                    \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define ERR_FD_NOMEM                                                       \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define ERR_FD_NOBUF                                                       \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
#define ERR_FD_BUF_EMPTY                                                   \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define ERR_FD_BUF_FULL                                                    \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define ERR_FD_NOTREADY                                                    \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define ERR_FD_BADADDR                                                     \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
#define ERR_FD_BUSY DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define ERR_FD_SYS_TIMEOUT                                                 \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_SYS_TIMEOUT)
#define ERR_FD_CFG DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_CFG)
#define ERR_FD_FACE_NUM_OVER                                               \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_FACE_NUM_OVER)
#define ERR_FD_OPEN_FILE                                                   \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_OPEN_FILE)
#define ERR_FD_READ_FILE                                                   \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_READ_FILE)
#define ERR_FD_WRITE_FILE                                                  \
	DEF_ERR(ID_FD, EN_ERR_LEVEL_ERROR, ERR_FD_WRITE_FILE)

#define ERR_ODT_INVALID_CHNID                                              \
	DEF_ERR(ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define ERR_ODT_EXIST                                                      \
	DEF_ERR(ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
#define ERR_ODT_UNEXIST                                                    \
	DEF_ERR(ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
#define ERR_ODT_NOT_PERM                                                   \
	DEF_ERR(ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define ERR_ODT_NOTREADY                                                   \
	DEF_ERR(ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define ERR_ODT_BUSY                                                       \
	DEF_ERR(ID_ODT, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
//==============================================================================
typedef struct _ive_block_ctrl_s {
	float scale_size;
	unsigned int cell_size;
} ive_block_ctrl_s;

typedef struct _ive_blend_ctrl_s {
	unsigned char weight;
} ive_blend_ctrl_s;

typedef enum ive_cc_dir { DIRECTION_4 = 0x0, DIRECTION_8 = 0x1 } ive_cc_dir_e;

typedef struct _ive_cc_ctrl_s {
	ive_cc_dir_e mode;
} ive_cc_ctrl_s;

typedef struct _ive_hog_ctrl_s {
	unsigned char bin_size;
	unsigned int cell_size;
	unsigned short blk_size_in_cell;
	unsigned short blk_step_x;
	unsigned short blk_step_y;
} ive_hog_ctrl_s;

typedef enum _ive_itc_type_e {
	IVE_ITC_SATURATE = 0x0,
	IVE_ITC_NORMALIZE = 0x1,
} ive_itc_type_e;

typedef struct _ive_itc_ctrl_s {
	ive_itc_type_e type;
} ive_itc_ctrl_s;
//==============================================================================
#define IVE_HIST_NUM 256
#define IVE_MAP_NUM 256
#define IVE_MAX_REGION_NUM 254
#define IVE_ST_MAX_CORNER_NUM 500

typedef enum _ive_dma_mode_e {
	IVE_DMA_MODE_DIRECT_COPY = 0x0,
	IVE_DMA_MODE_INTERVAL_COPY = 0x1,
	IVE_DMA_MODE_SET_3BYTE = 0x2,
	IVE_DMA_MODE_SET_8BYTE = 0x3,
	IVE_DMA_MODE_BUTT
} ive_dma_mode_e;

typedef struct _ive_dma_ctrl_s {
	ive_dma_mode_e mode;
	uint64_t val; /*Used in memset mode*/
	/*
	 * Used in interval-copy mode, every row was segmented by
	 * hor_seg_size bytes, restricted in values of 2,3,4,8,16
	 */
	unsigned char hor_seg_size;
	/*
	 * Used in interval-copy mode, the valid bytes copied
	 * in front of every segment in a valid row, w_ch 0<elem_size<hor_seg_size
	 */
	unsigned char elem_size;
	unsigned char ver_seg_rows; /*Used in interval-copy mode, copy one row in every ver_seg_rows*/
} ive_dma_ctrl_s;

typedef struct _ive_filter_ctrl_s {
	signed char mask[25]; /*Template parameter filter coefficient*/
	unsigned char norm; /*Normalization parameter, by right s_ft*/
} ive_filter_ctrl_s;

typedef enum _ive_csc_mode_e {
	IVE_CSC_MODE_VIDEO_BT601_YUV2RGB =
		0x0, /*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
	IVE_CSC_MODE_VIDEO_BT709_YUV2RGB =
		0x1, /*CSC: YUV2RGB, video transfer mode, RGB value range [16, 235]*/
	IVE_CSC_MODE_PIC_BT601_YUV2RGB =
		0x2, /*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/
	IVE_CSC_MODE_PIC_BT709_YUV2RGB =
		0x3, /*CSC: YUV2RGB, picture transfer mode, RGB value range [0, 255]*/

	IVE_CSC_MODE_PIC_BT601_YUV2HSV =
		0x4, /*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/
	IVE_CSC_MODE_PIC_BT709_YUV2HSV =
		0x5, /*CSC: YUV2HSV, picture transfer mode, HSV value range [0, 255]*/

	IVE_CSC_MODE_PIC_BT601_YUV2LAB =
		0x6, /*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/
	IVE_CSC_MODE_PIC_BT709_YUV2LAB =
		0x7, /*CSC: YUV2LAB, picture transfer mode, Lab value range [0, 255]*/

	IVE_CSC_MODE_VIDEO_BT601_RGB2YUV =
		0x8, /*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
	IVE_CSC_MODE_VIDEO_BT709_RGB2YUV =
		0x9, /*CSC: RGB2YUV, video transfer mode, YUV value range [0, 255]*/
	IVE_CSC_MODE_PIC_BT601_RGB2YUV =
		0xa, /*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/
	IVE_CSC_MODE_PIC_BT709_RGB2YUV =
		0xb, /*CSC: RGB2YUV, picture transfer mode, Y:[16, 235],U\V:[16, 240]*/

	IVE_CSC_MODE_PIC_RGB2HSV = 0xb,
	IVE_CSC_MODE_PIC_RGB2GRAY = 0xc,
	IVE_CSC_MODE_BUTT
} ive_csc_mode_e;

typedef struct _ive_csc_ctrl_s {
	ive_csc_mode_e mode; /*Working mode*/
} ive_csc_ctrl_s;

typedef struct _ive_filter_and_csc_ctrl_s {
	ive_csc_mode_e mode; /*CSC working mode*/
	signed char mask[25]; /*Template parameter filter coefficient*/
	unsigned char norm; /*Normalization parameter, by right s_ft*/
} ive_filter_and_csc_ctrl_s;

typedef enum _ive_sobel_out_ctrl_e {
	IVE_SOBEL_OUT_CTRL_BOTH = 0x0, /*Output horizontal and vertical*/
	IVE_SOBEL_OUT_CTRL_HOR = 0x1, /*Output horizontal*/
	IVE_SOBEL_OUT_CTRL_VER = 0x2, /*Output vertical*/
	IVE_SOBEL_OUT_CTRL_BUTT
} ive_sobel_out_ctrl_e;

typedef struct _ive_sobel_ctrl_s {
	ive_sobel_out_ctrl_e out_ctrl; /*Output format*/
	signed char mask[25]; /*Template parameter*/
} ive_sobel_ctrl_s;

typedef enum _ive_mag_and_ang_out_ctrl_e {
	IVE_MAG_AND_ANG_OUT_CTRL_MAG = 0x0, /*Only the magnitude is output.*/
	IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG =
		0x1, /*The magnitude and angle are output.*/
	IVE_MAG_AND_ANG_OUT_CTRL_BUTT
} ive_mag_and_ang_out_ctrl_e;

typedef struct _ive_mag_and_ang_ctrl_s {
	ive_mag_and_ang_out_ctrl_e out_ctrl;
	unsigned short thr;
	signed char mask[25]; /*Template parameter.*/
} ive_mag_and_ang_ctrl_s;

typedef struct _ive_dilate_ctrl_s {
	unsigned char mask[25]; /*The template parameter value must be 0 or 255.*/
} ive_dilate_ctrl_s;
typedef ive_dilate_ctrl_s ive_erode_ctrl_s;

typedef enum _ive_thresh_mode_e {
	/*srcVal <= lowThr, dstVal = minVal; srcVal > lowThr, dstVal = maxVal.*/
	IVE_THRESH_MODE_BINARY = 0x0,
	/*srcVal <= lowThr, dstVal = srcVal; srcVal > lowThr, dstVal = maxVal.*/
	IVE_THRESH_MODE_TRUNC = 0x1,
	/*srcVal <= lowThr, dstVal = minVal; srcVal > lowThr, dstVal = srcVal.*/
	IVE_THRESH_MODE_TO_MINVAL = 0x2,
	/*
	 * srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= _ghThr,
	 * dstVal = midVal; srcVal > _ghThr, dstVal = maxVal.
	 */
	IVE_THRESH_MODE_MIN_MID_MAX = 0x3,
	/*
	 * srcVal <= lowThr, dstVal = srcVal;  lowThr < srcVal <= _ghThr,
	 * dstVal = midVal; srcVal > _ghThr, dstVal = maxVal.
	 */
	IVE_THRESH_MODE_ORI_MID_MAX = 0x4,
	/*
	 * srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= _ghThr,
	 * dstVal = midVal; srcVal > _ghThr, dstVal = srcVal.
	 */
	IVE_THRESH_MODE_MIN_MID_ORI = 0x5,
	/*
	 * srcVal <= lowThr, dstVal = minVal;  lowThr < srcVal <= _ghThr,
	 * dstVal = srcVal; srcVal > _ghThr, dstVal = maxVal.
	 */
	IVE_THRESH_MODE_MIN_ORI_MAX = 0x6,
	/*
	 * srcVal <= lowThr, dstVal = srcVal;  lowThr < srcVal <= _ghThr,
	 * dstVal = midVal; srcVal > _ghThr, dstVal = srcVal.
	 */
	IVE_THRESH_MODE_ORI_MID_ORI = 0x7,

	IVE_THRESH_MODE_BUTT
} ive_thresh_mode_e;

typedef struct _ive_thresh_ctrl_s {
	ive_thresh_mode_e mode;
	unsigned char low_thr; /*user-defined threshold,  0<=low_thr<=255 */
	/*
	 * user-defined threshold, if mode<IVE_THRESH_MODE_MIN_MID_MAX,
	 * high_thr is not used, else 0<=low_thr<=high_thr<=255;
	 */
	unsigned char high_thr;
	unsigned char min_val; /*Minimum value when tri-level thresholding*/
	unsigned char mid_val; /*Middle value when tri-level thresholding, if mode<2, u32MidVal is not used; */
	unsigned char max_val; /*Maxmum value when tri-level thresholding*/
} ive_thresh_ctrl_s;

typedef enum _ive_sub_mode_e {
	IVE_SUB_MODE_ABS = 0x0, /*Absolute value of the difference*/
	IVE_SUB_MODE_SHIFT =
		0x1, /*The output result is obtained by s_fting the result one digit right to reserve the signed bit.*/
	IVE_SUB_MODE_BUTT
} ive_sub_mode_e;

typedef struct _ive_sub_ctrl_s {
	ive_sub_mode_e mode;
} ive_sub_ctrl_s;

typedef enum _ive_integ_outmode_ctrl_e {
	IVE_INTEG_OUT_CTRL_COMBINE = 0x0,
	IVE_INTEG_OUT_CTRL_SUM = 0x1,
	IVE_INTEG_OUT_CTRL_SQSUM = 0x2,
	IVE_INTEG_OUT_CTRL_BUTT
} ive_integ_outmode_ctrl_e;

typedef struct _ive_integ_ctrl_s {
	ive_integ_outmode_ctrl_e out_ctrl;
} ive_integ_ctrl_s;

typedef enum _ive_thresh_s16_mode_e {
	IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX = 0x0,
	IVE_THRESH_S16_MODE_S16_TO_S8_MIN_ORI_MAX = 0x1,
	IVE_THRESH_S16_MODE_S16_TO_U8_MIN_MID_MAX = 0x2,
	IVE_THRESH_S16_MODE_S16_TO_U8_MIN_ORI_MAX = 0x3,

	IVE_THRESH_S16_MODE_BUTT
} ive_thresh_s16_mode_e;

typedef struct _ive_thresh_s16_ctrl_s {
	ive_thresh_s16_mode_e mode;
	short low_thr; /*User-defined threshold*/
	short high_thr; /*User-defined threshold*/
	ive_8bit_u min_val; /*Minimum value when tri-level thresholding*/
	ive_8bit_u mid_val; /*Middle value when tri-level thresholding*/
	ive_8bit_u max_val; /*Maxmum value when tri-level thresholding*/
} ive_thresh_s16_ctrl_s;

typedef enum _ive_thresh_u16_mode_e {
	IVE_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX = 0x0,
	IVE_THRESH_U16_MODE_U16_TO_U8_MIN_ORI_MAX = 0x1,

	IVE_THRESH_U16_MODE_BUTT
} ive_thresh_u16_mode_e;

typedef struct _ive_thresh_u16_ctrl_s {
	ive_thresh_u16_mode_e mode;
	unsigned short low_thr;
	unsigned short high_thr;
	unsigned char min_val;
	unsigned char mid_val;
	unsigned char max_val;
} ive_thresh_u16_ctrl_s;

typedef enum _ive_16bit_to_8bit_mode_e {
	IVE_16BIT_TO_8BIT_MODE_S16_TO_S8 = 0x0,
	IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_ABS = 0x1,
	IVE_16BIT_TO_8BIT_MODE_S16_TO_U8_BIAS = 0x2,
	IVE_16BIT_TO_8BIT_MODE_U16_TO_U8 = 0x3,

	IVE_16BIT_TO_8BIT_MODE_BUTT
} ive_16bit_to_8bit_mode_e;

typedef struct _ive_16bit_to_8bit_ctrl_s {
	ive_16bit_to_8bit_mode_e mode;
	unsigned short denominator;
	unsigned char numerator;
	signed char s8Bias;
} ive_16bit_to_8bit_ctrl_s;

typedef enum _ive_ord_stat_filter_mode_e {
	IVE_ORD_STAT_FILTER_MODE_MEDIAN = 0x0,
	IVE_ORD_STAT_FILTER_MODE_MAX = 0x1,
	IVE_ORD_STAT_FILTER_MODE_MIN = 0x2,

	IVE_ORD_STAT_FILTER_MODE_BUTT
} ive_ord_stat_filter_mode_e;

typedef struct _ive_ord_stat_filter_ctrl_s {
	ive_ord_stat_filter_mode_e mode;

} ive_ord_stat_filter_ctrl_s;

typedef enum _ive_map_mode_e {
	IVE_MAP_MODE_U8 = 0x0,
	IVE_MAP_MODE_S16 = 0x1,
	IVE_MAP_MODE_U16 = 0x2,

	IVE_MAP_MODE_BUTT
} ive_map_mode_e;

typedef struct _ive_map_ctrl_s {
	ive_map_mode_e mode;
} ive_map_ctrl_s;

typedef struct _ive_map_u8bit_lut_mem_s {
	unsigned char map[IVE_MAP_NUM];
} ive_map_u8bit_lut_mem_s;

typedef struct _ive_map_u16bit_lut_mem_s {
	unsigned short map[IVE_MAP_NUM];
} ive_map_u16bit_lut_mem_s;

typedef struct _ive_map_s16bit_lut_mem_s {
	short map[IVE_MAP_NUM];
} ive_map_s16bit_lut_mem_s;

typedef struct _ive_equalize_hist_ctrl_mem_s {
	unsigned int hist[IVE_HIST_NUM];
	unsigned char map[IVE_MAP_NUM];
} ive_equalize_hist_ctrl_mem_s;

typedef struct _ive_equalize_hist_ctrl_s {
	ive_mem_info_s mem;
} ive_equalize_hist_ctrl_s;

typedef struct _ive_add_ctrl_s {
	u0q16 x; /*x of "xA+yB"*/
	u0q16 y; /*y of "xA+yB"*/
} ive_add_ctrl_s;

typedef struct _ive_ncc_dst_mem_s {
	uint64_t numerator;
	uint64_t quad_sum1;
	uint64_t quad_sum2;
	unsigned char reserved[8];
} ive_ncc_dst_mem_s;

typedef struct _ive_region_s {
	unsigned int area; /*Represented by the pixel number*/
	unsigned short left; /*Circumscribed rectangle left border*/
	unsigned short right; /*Circumscribed rectangle right border*/
	unsigned short top; /*Circumscribed rectangle top border*/
	unsigned short bottom; /*Circumscribed rectangle bottom border*/
} ive_region_s;

typedef struct _ive_ccblob_s {
	unsigned short cur_area_thr; /*Threshold of the result regions' area*/
	signed char label_status; /*-1: Labeled failed ; 0: Labeled successfully*/
	unsigned char region_num; /*Number of valid region, non-continuous stored*/
	/*Valid regions with 'area>0' and 'label = ArrayIndex+1'*/
	ive_region_s region[IVE_MAX_REGION_NUM];
} ive_ccblob_s;

typedef enum _ive_ccl_mode_e {
	IVE_CCL_MODE_4C = 0x0, /*4-connected*/
	IVE_CCL_MODE_8C = 0x1, /*8-connected*/

	IVE_CCL_MODE_BUTT
} ive_ccl_mode_e;

typedef struct _ive_ccl_ctrl_s {
	ive_ccl_mode_e mode; /*Mode*/
	unsigned short init_area_thr; /*Init threshold of region area*/
	unsigned short u16Step; /*Increase area step for once*/
} ive_ccl_ctrl_s;

typedef struct _ive_gmm_ctrl_s {
	u22q10 noise_var; /*Initial noise Variance*/
	u22q10 max_var; /*Max  Variance*/
	u22q10 min_var; /*Min  Variance*/
	u0q16 learn_rate; /*Learning rate*/
	u0q16 bg_ratio; /*Background ratio*/
	charq8 var_thr; /*Variance Threshold*/
	u0q16 init_weight; /*Initial Weight*/
	unsigned char model_num; /*Model number: 3 or 5*/
} ive_gmm_ctrl_s;

typedef enum _ive_gmm2_sns_factor_mode_e {
	IVE_GMM2_SNS_FACTOR_MODE_GLB = 0x0, /*Global sensitivity factor mode*/
	IVE_GMM2_SNS_FACTOR_MODE_PIX = 0x1, /*Pixel sensitivity factor mode*/

	IVE_GMM2_SNS_FACTOR_MODE_BUTT
} ive_gmm2_sns_factor_mode_e;

typedef enum _ive_gmm2_life_update_factor_mode_e {
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_GLB =
		0x0, /*Global life update factor mode*/
	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_PIX =
		0x1, /*Pixel life update factor mode*/

	IVE_GMM2_LIFE_UPDATE_FACTOR_MODE_BUTT
} ive_gmm2_life_update_factor_mode_e;

typedef struct _ive_gmm2_ctrl_s {
	ive_gmm2_sns_factor_mode_e sns_factor_mode; /*Sensitivity factor mode*/
	ive_gmm2_life_update_factor_mode_e
	life_update_factor_mode; /*Life update factor mode*/
	unsigned short glb_life_update_factor; /*Global life update factor (default: 4)*/
	unsigned short life_thr; /*Life threshold (default: 5000)*/
	unsigned short freq_init_val; /*Initial frequency (default: 20000)*/
	unsigned short freq_redu_factor; /*Frequency reduction factor (default: 0xFF00)*/
	unsigned short freq_add_factor; /*Frequency adding factor (default: 0xEF)*/
	unsigned short freq_thr; /*Frequency threshold (default: 12000)*/
	unsigned short var_rate; /*Variation update rate (default: 1)*/
	u9q7 max_var; /*Max variation (default: (16 * 16)<<7)*/
	u9q7 min_var; /*Min variation (default: ( 8 *  8)<<7)*/
	unsigned char glb_sns_factor; /*Global sensitivity factor (default: 8)*/
	unsigned char model_num; /*Model number (range: 1~5, default: 3)*/
} ive_gmm2_ctrl_s;

typedef struct _ive_canny_hys_edge_ctrl_s {
	ive_mem_info_s mem;
	unsigned short low_thr;
	unsigned short high_thr;
	signed char mask[25];
} ive_canny_hys_edge_ctrl_s;

typedef struct _ive_canny_stack_size_s {
	unsigned int stack_size; /*Stack size for output*/
	unsigned char reserved[12]; /*For 16 byte align*/
} ive_canny_stack_size_s;

typedef enum _ive_lbp_cmp_mode_e {
	IVE_LBP_CMP_MODE_NORMAL =
		0x0, /* P(x)-P(center)>= thr.s8_val, s(x)=1; else s(x)=0; */
	IVE_LBP_CMP_MODE_ABS =
		0x1, /* Abs(P(x)-P(center))>=thr.u8_val, s(x)=1; else s(x)=0; */

	IVE_LBP_CMP_MODE_BUTT
} ive_lbp_cmp_mode_e;

typedef struct _ive_lbp_ctrl_s {
	ive_lbp_cmp_mode_e mode;
	ive_8bit_u thr;
} ive_lbp_ctrl_s;

typedef enum _ive_norm_grad_out_ctrl_e {
	IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER = 0x0,
	IVE_NORM_GRAD_OUT_CTRL_HOR = 0x1,
	IVE_NORM_GRAD_OUT_CTRL_VER = 0x2,
	IVE_NORM_GRAD_OUT_CTRL_COMBINE = 0x3,

	IVE_NORM_GRAD_OUT_CTRL_BUTT
} ive_norm_grad_out_ctrl_e;

typedef struct _ive_norm_grad_ctrl_s {
	ive_norm_grad_out_ctrl_e out_ctrl;
	signed char mask[25];
	unsigned char norm;
} ive_norm_grad_ctrl_s;

typedef struct _ive_frame_diff_motion_ctrl_s {
	ive_sub_mode_e sub_mode;

	ive_thresh_mode_e thr_mode;
	unsigned char thr_low; /*user-defined threshold,  0<=low_thr<=255 */
	/*
	 * user-defined threshold, if mode<IVE_THRESH_MODE_MIN_MID_MAX,
	 * high_thr is not used, else 0<=low_thr<=high_thr<=255;
	 */
	unsigned char thr_high;
	unsigned char thr_min_val; /*Minimum value when tri-level thresholding*/
	unsigned char thr_mid_val; /*Middle value when tri-level thresholding, if mode<2, u32MidVal is not used; */
	unsigned char thr_max_val; /*Maxmum value when tri-level thresholding*/

	unsigned char erode_mask[25]; /*The template parameter value must be 0 or 255.*/
	unsigned char dilate_mask[25]; /*The template parameter value must be 0 or 255.*/

} ive_frame_diff_motion_ctrl_s;

typedef enum _ive_lk_optical_flow_pyr_out_mode_e {
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_NONE = 0, /*Output none*/
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_STATUS = 1, /*Output status*/
	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_BOTH = 2, /*Output status and err*/

	IVE_LK_OPTICAL_FLOW_PYR_OUT_MODE_BUTT
} ive_lk_optical_flow_pyr_out_mode_e;

typedef struct _ive_lk_optical_flow_pyr_ctrl_s {
	ive_lk_optical_flow_pyr_out_mode_e out_mode;
	unsigned char use_init_flow; /*where to use initial flow*/
	unsigned short pts_num; /*Number of the feature points,<=500*/
	unsigned char max_level; /*0<=max_level<=3*/
	u0q8 min_eig_thr; /*Minimum eigenvalue threshold*/
	unsigned char iter_cnt; /*Maximum iteration times, <=20*/
	u0q8 eps; /*Used for exit criteria: dx^2 + dy^2 < eps */
} ive_lk_optical_flow_pyr_ctrl_s;

typedef struct _ive_st_max_eig_s {
	unsigned short max_eig; /*S_-Tomasi second step output MaxEig*/
	unsigned char reserved[14]; /*For 16 byte align*/
} ive_st_max_eig_s;

typedef struct _ive_st_candi_corner_ctrl_s {
	ive_mem_info_s mem;
	u0q8 quality_level;
} ive_st_candi_corner_ctrl_s;

typedef struct _ive_st_corner_info_s {
	unsigned short corner_num;
	ive_point_u16_s corner[IVE_ST_MAX_CORNER_NUM];
} ive_st_corner_info_s;

typedef struct _ive_st_corner_ctrl_s {
	unsigned short max_corner_num;
	unsigned short min_dist;
} ive_st_corner_ctrl_s;

typedef enum _ive_grad_fg_mode_e {
	IVE_GRAD_FG_MODE_USE_CUR_GRAD = 0x0,
	IVE_GRAD_FG_MODE_FIND_MIN_GRAD = 0x1,

	IVE_GRAD_FG_MODE_BUTT
} ive_grad_fg_mode_e;

typedef struct _ive_grad_fg_ctrl_s {
	ive_grad_fg_mode_e mode; /*Calculation mode*/
	unsigned short edw_factor; /*Edge width adjustment factor (range: 500 to 2000; default: 1000)*/
	unsigned char crl_coe_thr; /*Gradient vector correlation coefficient threshold (ranges: 50 to 100; default: 80)*/
	unsigned char mag_crl_thr; /*Gradient amplitude threshold (range: 0 to 20; default: 4)*/
	unsigned char min_mag_diff; /*Gradient magnitude difference threshold (range: 2 to 8; default: 2)*/
	unsigned char noise_val; /*Gradient amplitude noise threshold (range: 1 to 8; default: 1)*/
	unsigned char edw_dark; /*Black pixels enable flag (range: 0 (no), 1 (yes); default: 1)*/
} ive_grad_fg_ctrl_s;

typedef struct _ive_candi_bg_pix_s {
	charq4f4 mean; /*Candidate background grays value */
	unsigned short start_time; /*Candidate Background start time */
	unsigned short sum_access_time; /*Candidate Background cumulative access time */
	unsigned short short_keep_time; /*Candidate background short hold time*/
	unsigned char chg_cond; /*Time condition for candidate background into the changing state*/
	unsigned char poten_bg_life; /*Potential background cumulative access time */
} ive_candi_bg_pix_s;

typedef struct _ive_work_bg_pix_s {
	charq4f4 mean; /*0# background grays value */
	unsigned short acc_time; /*Background cumulative access time */
	unsigned char pre_gray; /*Gray value of last pixel */
	U5Q3 diff_thr; /*Differential threshold */
	unsigned char acc_flag; /*Background access flag */
	unsigned char bg_gray[3]; /*1# ~ 3# background grays value */
} ive_work_bg_pix_s;

typedef struct _ive_bg_life_s {
	unsigned char work_bg_life[3]; /*1# ~ 3# background vitality */
	unsigned char candi_bg_life; /*Candidate background vitality */
} ive_bg_life_s;

typedef struct _ive_bg_model_pix_s {
	ive_work_bg_pix_s work_bg_pixel; /*Working background */
	ive_candi_bg_pix_s candi_pixel; /*Candidate background */
	ive_bg_life_s bg_life; /*Background vitality */
} ive_bg_model_pix_s;

typedef struct _ive_fg_stat_data_s {
	unsigned int pix_num;
	unsigned int sum_lum;
	unsigned char reserved[8];
} ive_fg_stat_data_s;

typedef struct _ive_bg_stat_data_s {
	unsigned int pix_num;
	unsigned int sum_lum;
	unsigned char reserved[8];
} ive_bg_stat_data_s;

typedef struct _ive_match_bg_model_ctrl_s {
	unsigned int cur_frm_num; /*Current frame timestamp, in frame units */
	unsigned int pre_frm_num; /*Previous frame timestamp, in frame units */
	unsigned short time_thr; /*Potential background replacement time threshold (range: 2 to 100 frames; default: 20) */
	/*
	 * Correlation coefficients between differential threshold and gray value
	 * (range: 0 to 5; default: 0)
	 */
	unsigned char diff_thr_crl_coef;
	unsigned char diff_max_thr; /*Maximum of background differential threshold (range: 3 to 15; default: 6) */
	unsigned char diff_min_thr; /*Minimum of background differential threshold (range: 3 to 15; default: 4) */
	unsigned char diff_thr_inc; /*Dynamic Background differential threshold increment (range: 0 to 6; default: 0) */
	unsigned char fast_learn_rate; /*Quick background learning rate (range: 0 to 4; default: 2) */
	unsigned char det_chg_region; /*Whether to detect change region (range: 0 (no), 1 (yes); default: 0) */
} ive_match_bg_model_ctrl_s;

typedef struct _ive_update_bg_model_ctrl_s {
	unsigned int cur_frm_num; /*Current frame timestamp, in frame units */
	unsigned int pre_chk_time; /*The last time when background status is checked */
	unsigned int frm_chk_period; /*Background status checking period (range: 0 to 2000 frames; default: 50) */

	unsigned int init_min_time; /*Background initialization shortest time (range: 20 to 6000 frames; default: 100)*/
	/*
	 * Steady background integration shortest time
	 * (range: 20 to 6000 frames; default: 200)
	 */
	unsigned int sty_bg_min_blend_time;
	/*
	 * Steady background integration longest time
	 * (range: 20 to 40000 frames; default: 1500)
	 */
	unsigned int sty_bg_max_blend_time;
	/*
	 * Dynamic background integration shortest time
	 * (range: 0 to 6000 frames; default: 0)
	 */
	unsigned int dyn_bg_min_blend_time;
	unsigned int static_det_min_time; /*Still detection shortest time (range: 20 to 6000 frames; default: 80)*/
	unsigned short fg_max_fade_time; /*Foreground disappearing longest time (range: 1 to 255 seconds; default: 15)*/
	unsigned short bg_max_fade_time; /*Background disappearing longest time (range: 1 to 255  seconds ; default: 60)*/

	unsigned char sty_bg_acc_time_rate_thr; /*Steady background access time ratio threshold (range: 10 to 100; default: 80)*/
	unsigned char chg_bg_acc_time_rate_thr; /*Change background access time ratio threshold (range: 10 to 100; default: 60)*/
	unsigned char dyn_bg_acc_time_thr; /*Dynamic background access time ratio threshold (range: 0 to 50; default: 0)*/
	unsigned char dyn_bg_depth; /*Dynamic background depth (range: 0 to 3; default: 3)*/
	/*
	 * Background state time ratio threshold when initializing
	 * (range: 90 to 100; default: 90)
	 */
	unsigned char bg_eff_sta_rate_thr;

	unsigned char acce_bg_learn; /*Whether to accelerate background learning (range: 0 (no), 1 (yes); default: 0)*/
	unsigned char det_chg_region; /*Whether to detect change region (range: 0 (no), 1 (yes); default: 0)*/
} ive_update_bg_model_ctrl_s;

typedef enum _ive_ann_mlp_activ_func_e {
	IVE_ANN_MLP_ACTIV_FUNC_IDENTITY = 0x0,
	IVE_ANN_MLP_ACTIV_FUNC_SIGMOID_SYM = 0x1,
	IVE_ANN_MLP_ACTIV_FUNC_GAUSSIAN = 0x2,

	IVE_ANN_MLP_ACTIV_FUNC_BUTT
} ive_ann_mlp_activ_func_e;

typedef enum _ive_ann_mlp_accurate_e {
	IVE_ANN_MLP_ACCURATE_SRC16_WGT16 =
		0x0, /*input decimals' accurate 16 bit, weight 16bit*/
	IVE_ANN_MLP_ACCURATE_SRC14_WGT20 =
		0x1, /*input decimals' accurate 14 bit, weight 20bit*/

	IVE_ANN_MLP_ACCURATE_BUTT
} ive_ann_mlp_accurate_e;

typedef struct _ive_ann_mlp_model_s {
	ive_ann_mlp_activ_func_e activ_func;
	ive_ann_mlp_accurate_e accurate;
	ive_mem_info_s weight;
	unsigned int total_weight_size;

	unsigned short layer_count[8]; /*8 layers, including input and output layer*/
	unsigned short max_count; /*MaxCount<=1024*/
	unsigned char layer_num; /*2<layerNum<=8*/
	unsigned char reserved;
} ive_ann_mlp_model_s;

typedef enum _ive_svm_type_e {
	IVE_SVM_TYPE_C_SVC = 0x0,
	IVE_SVM_TYPE_NU_SVC = 0x1,

	IVE_SVM_TYPE_BUTT
} ive_svm_type_e;

typedef enum _ive_svm_kernel_type_e {
	IVE_SVM_KERNEL_TYPE_LINEAR = 0x0,
	IVE_SVM_KERNEL_TYPE_POLY = 0x1,
	IVE_SVM_KERNEL_TYPE_RBF = 0x2,
	IVE_SVM_KERNEL_TYPE_SIGMOID = 0x3,

	IVE_SVM_KERNEL_TYPE_BUTT
} ive_svm_kernel_type_e;

typedef struct _ive_svm_model_s {
	ive_svm_type_e type;
	ive_svm_kernel_type_e kernel_type;

	ive_mem_info_s sv; /*SV memory*/
	ive_mem_info_s df; /*Decision functions memory*/
	unsigned int total_df_size; /*All decision functions coef size in byte*/

	unsigned short feature_dim;
	unsigned short sv_total;
	unsigned char class_count;
} ive_svm_model_s;

typedef enum _ive_sad_mode_e {
	IVE_SAD_MODE_MB_4X4 = 0x0, /*4x4*/
	IVE_SAD_MODE_MB_8X8 = 0x1, /*8x8*/
	IVE_SAD_MODE_MB_16X16 = 0x2, /*16x16*/

	IVE_SAD_MODE_BUTT
} ive_sad_mode_e;

typedef enum _ive_sad_out_ctrl_e {
	IVE_SAD_OUT_CTRL_16BIT_BOTH = 0x0, /*Output 16 bit sad and thresh*/
	IVE_SAD_OUT_CTRL_8BIT_BOTH = 0x1, /*Output 8 bit sad and thresh*/
	IVE_SAD_OUT_CTRL_16BIT_SAD = 0x2, /*Output 16 bit sad*/
	IVE_SAD_OUT_CTRL_8BIT_SAD = 0x3, /*Output 8 bit sad*/
	IVE_SAD_OUT_CTRL_THRESH = 0x4, /*Output thresh,16 bits sad */

	IVE_SAD_OUT_CTRL_BUTT
} ive_sad_out_ctrl_e;

typedef struct _ive_sad_ctrl_s {
	ive_sad_mode_e mode;
	ive_sad_out_ctrl_e out_ctrl;
	unsigned short thr; /*srcVal <= thr, dstVal = minVal; srcVal > thr, dstVal = maxVal.*/
	unsigned char min_val; /*Min value*/
	unsigned char max_val; /*Max value*/
} ive_sad_ctrl_s;

typedef enum _ive_resize_mode_e {
	IVE_RESIZE_MODE_LINEAR = 0x0, /*Bilinear interpolation*/
	IVE_RESIZE_MODE_AREA = 0x1, /*Area-based (or super) interpolation*/

	IVE_RESIZE_MODE_BUTT
} ive_resize_mode_e;

typedef struct _ive_resize_ctrl_s {
	ive_resize_mode_e mode;
	ive_mem_info_s mem;
	unsigned short num;
} ive_resize_ctrl_s;

typedef enum _ive_cnn_activ_func_e {
	IVE_CNN_ACTIV_FUNC_NONE = 0x0, /*Do not taking a activation, equivalent f(x)=x*/
	IVE_CNN_ACTIV_FUNC_RELU = 0x1, /*f(x)=max(0, x)*/
	IVE_CNN_ACTIV_FUNC_SIGMOID = 0x2, /*f(x)=1/(1+exp(-x)), not support*/

	IVE_CNN_ACTIV_FUNC_BUTT
} ive_cnn_activ_func_e;

typedef enum _ive_cnn_pooling_e {
	IVE_CNN_POOLING_NONE = 0x0, /*Do not taking a pooling action*/
	IVE_CNN_POOLING_MAX = 0x1, /*Using max value of every pooling area*/
	IVE_CNN_POOLING_AVG = 0x2, /*Using average value of every pooling area*/

	IVE_CNN_POOLING_BUTT
} ive_cnn_pooling_e;

typedef struct _ive_cnn_conv_pooling_s {
	ive_cnn_activ_func_e activ_func; /*Type of activation function*/
	ive_cnn_pooling_e pooling; /*Mode of pooling method*/

	unsigned char feature_map_num; /*Number of feature maps*/
	unsigned char kernel_size; /*Kernel size, only support 3 currently*/
	unsigned char conv_step; /*Convolution step, only support 1 currently*/

	unsigned char pool_size; /*Pooling size, only support 2 currently*/
	unsigned char pool_step; /*Pooling step, only support 2 currently*/
	unsigned char reserved[3];

} ive_cnn_conv_pooling_s;

typedef struct _ive_cnn_full_connect_s {
	unsigned short layer_cnt[8]; /*Neuron number of every fully connected layers*/
	unsigned short max_cnt; /*Max neuron number in all fully connected layers*/
	unsigned char layer_num; /*Number of fully connected layer*/
	unsigned char reserved;
} ive_cnn_full_connect_s;

typedef struct _ive_cnn_model_s {
	ive_cnn_conv_pooling_s conv_pool[8]; /*Conv-ReLU-Pooling layers info*/
	ive_cnn_full_connect_s full_connect; /*Fully connected layers info*/

	ive_mem_info_s
	conv_kernel_bias; /*Conv-ReLU-Pooling layers' kernels and bias*/
	unsigned int conv_kernel_bias_size; /*Size of Conv-ReLU-Pooling layer' kernels and bias*/

	ive_mem_info_s fcl_wgt_bias; /*Fully Connection Layers' weights and bias*/
	unsigned int fcl_wgt_bias_size; /*Size of fully connection layers weights and bias*/

	unsigned int total_mem_size; /*Total memory size of all kernels, weights, bias*/

	ive_image_type_e type; /*Image type used for the CNN model*/
	unsigned int width; /*Image width used for the model*/
	unsigned int height; /*Image height used for the model*/

	unsigned short class_count; /*Number of classes*/
	unsigned char conv_pool_layer_num; /*Number of Conv-ReLU-Pooling layers*/
	unsigned char reserved;
} ive_cnn_model_s;

typedef struct _ive_cnn_ctrl_s {
	ive_mem_info_s mem; /*Assist memory*/
	unsigned int num; /*Input image number*/
} ive_cnn_ctrl_s;

typedef struct _ive_cnn_result_s {
	int class_idx; /*The most possible index of the classification*/
	int confidence; /*The confidence of the classification*/
} ive_cnn_result_s;

typedef enum _ive_bernsen_mode_e {
	IVE_BERNSEN_MODE_NORMAL = 0x0, /*Simple Bernsen thresh*/
	IVE_BERNSEN_MODE_THRESH =
		0x1, /*Thresh based on the global threshold and local Bernsen threshold*/
	IVE_BERNSEN_MODE_PAPER =
		0x2, /*This method is same with original paper*/
	IVE_BERNSEN_MODE_BUTT
} ive_bernsen_mode_e;

typedef struct _ive_bernsen_ctrl_s {
	ive_bernsen_mode_e mode;
	unsigned char win_size; /* 3x3 or 5x5 */
	unsigned char thr;
	unsigned char contrast_threshold; // compare with midgray
} ive_bernsen_ctrl_s;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /*_CVI_COMM_IVE_H*/

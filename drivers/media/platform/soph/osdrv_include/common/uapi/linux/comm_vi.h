/*
 * Copyright (C) Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/comm_vi.h
 * Description:
 *   Common video input definitions.
 */

#ifndef __COMM_VI_H__
#define __COMM_VI_H__

#include <linux/common.h>
#include <linux/comm_video.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


#define VI_MAX_ADCHN_NUM (4UL)

#define VI_COMPMASK_NUM (2UL)
#define VI_PRO_MAX_FRAME_NUM (8UL)
#define VI_SHARPEN_GAIN_NUM 32
#define VI_AUTO_ISO_STRENGTH_NUM 16

#define VI_INVALID_FRMRATE (-1)
#define VI_CHN0 0
#define VI_CHN1 1
#define VI_CHN2 2
#define VI_CHN3 3
#define VI_INVALID_CHN -1

#define VI_MAX_VC_NUM 4

typedef struct _vi_low_delay_info_s {
	unsigned char enable; /* RW; Low delay enable. */
	unsigned int line_cnt; /* RW; Range: [32, 16384]; Low delay shoreline. */
} vi_low_delay_info_s;

/* Information of raw data cmpresss param */
typedef struct _vi_cmp_param_s {
	unsigned char cmp_param[VI_CMP_PARAM_SIZE];
} vi_cmp_param_s;

typedef enum _vi_user_pic_mode_e {
	VI_USERPIC_MODE_PIC = 0, /* YUV picture */
	VI_USERPIC_MODE_BGC, /* Background picture only with a color */
	VI_USERPIC_MODE_BUTT,
} vi_user_pic_mode_e;

typedef struct _vi_user_pic_bgc_s {
	unsigned int bg_color;
} vi_user_pic_bgc_s;

typedef struct _vi_user_pic_attr_s {
	vi_user_pic_mode_e userpic_mode; /* User picture mode */
	union {
		video_frame_info_s user_pic_frm; /* Information about a YUV picture */
		vi_user_pic_bgc_s user_pic_bg; /* Information about a background picture only with a color */
	} user_pic;
} vi_user_pic_attr_s;

/* interface mode of video input */
typedef enum _vi_intf_mode_e {
	VI_MODE_BT656 = 0, /* ITU-R BT.656 YUV4:2:2 */
	VI_MODE_BT601, /* ITU-R BT.601 YUV4:2:2 */
	VI_MODE_DIGITAL_CAMERA, /* digatal camera mode */
	VI_MODE_BT1120_STANDARD, /* BT.1120 progressive mode */
	VI_MODE_BT1120_INTERLEAVED, /* BT.1120 interstage mode */
	VI_MODE_MIPI, /* MIPI RAW mode */
	VI_MODE_MIPI_YUV420_NORMAL, /* MIPI YUV420 normal mode */
	VI_MODE_MIPI_YUV420_LEGACY, /* MIPI YUV420 legacy mode */
	VI_MODE_MIPI_YUV422, /* MIPI YUV422 mode */
	VI_MODE_LVDS, /* LVDS mode */
	VI_MODE_HISPI, /* HiSPi mode */
	VI_MODE_SLVS, /* SLVS mode */

	VI_MODE_BUTT
} vi_intf_mode_e;

/* Input mode */
typedef enum _vi_input_mode_e {
	VI_INPUT_MODE_BT656 = 0, /* ITU-R BT.656 YUV4:2:2 */
	VI_INPUT_MODE_BT601, /* ITU-R BT.601 YUV4:2:2 */
	VI_INPUT_MODE_DIGITAL_CAMERA, /* digatal camera mode */
	VI_INPUT_MODE_INTERLEAVED, /* interstage mode */
	VI_INPUT_MODE_MIPI, /* MIPI mode */
	VI_INPUT_MODE_LVDS, /* LVDS mode */
	VI_INPUT_MODE_HISPI, /* HiSPi mode */
	VI_INPUT_MODE_SLVS, /* SLVS mode */

	VI_INPUT_MODE_BUTT
} vi_input_mode_e;

/* Work mode */
typedef enum _vi_work_mode_e {
	VI_WORK_MODE_1MULTIPLEX = 0, /* 1 Multiplex mode */
	VI_WORK_MODE_2MULTIPLEX, /* 2 Multiplex mode */
	VI_WORK_MODE_3MULTIPLEX, /* 3 Multiplex mode */
	VI_WORK_MODE_4MULTIPLEX, /* 4 Multiplex mode */

	VI_WORK_MODE_BUTT
} vi_work_mode_e;

/* YUV sensor scene(same with isp_yuv_scene_e of vi_drv.h) */
typedef enum _vi_isp_yuv_scene {
	VI_ISP_YUV_SCENE_BYPASS = 0,
	VI_ISP_YUV_SCENE_ONLINE,
	VI_ISP_YUV_SCENE_ISP,
	VI_ISP_YUV_SCENE_MAX,
} vi_isp_yuv_scene_e;

/* whether an input picture is interlaced or progressive */
typedef enum _vi_scan_mode_e {
	VI_SCAN_INTERLACED = 0, /* interlaced mode */
	VI_SCAN_PROGRESSIVE, /* progressive mode */

	VI_SCAN_BUTT
} vi_scan_mode_e;

/* Sequence of YUV data
 *
 * VI_DATA_SEQ_VUVU: The input sequence of the second component(only contains u and v) in BT.1120 mode is VUVU
 * VI_DATA_SEQ_UVUV: The input sequence of the second component(only contains u and v) in BT.1120 mode is UVUV
 */
typedef enum _vi_yuv_data_seq_e {
	VI_DATA_SEQ_VUVU = 0,
	VI_DATA_SEQ_UVUV,

	VI_DATA_SEQ_UYVY, /* The input sequence of YUV is UYVY */
	VI_DATA_SEQ_VYUY, /* The input sequence of YUV is VYUY */
	VI_DATA_SEQ_YUYV, /* The input sequence of YUV is YUYV */
	VI_DATA_SEQ_YVYU, /* The input sequence of YUV is YVYU */

	VI_DATA_SEQ_BUTT
} vi_yuv_data_seq_e;

/* Clock edge mode */
typedef enum _vi_clk_edge_e {
	VI_CLK_EDGE_SINGLE_UP = 0, /* single-edge mode and in rising edge */
	VI_CLK_EDGE_SINGLE_DOWN, /* single-edge mode and in falling edge */

	VI_CLK_EDGE_BUTT
} vi_clk_edge_e;

/* Component mode */
typedef enum _vi_component_mode_e {
	VI_COMPONENT_MODE_SINGLE = 0, /* single component mode */
	VI_COMPONENT_MODE_DOUBLE, /* double component mode */

	VI_COMPONENT_MODE_BUTT
} vi_component_mode_e;

/* Y/C composite or separation mode */
typedef enum _vi_combine_mode_e {
	VI_COMBINE_COMPOSITE = 0, /* Composite mode */
	VI_COMBINE_SEPARATE, /* Separate mode */

	VI_COMBINE_BUTT
} vi_combine_mode_e;

/* Attribute of the vertical synchronization signal */
typedef enum _vi_vsync_e {
	VI_VSYNC_FIELD = 0, /* Field/toggle mode:a signal reversal means a new frame or a field */
	VI_VSYNC_PULSE, /* Pusle/effective mode:a pusle or an effective signal means a new frame or a field */

	VI_VSYNC_BUTT
} vi_vsync_e;

/* Polarity of the vertical synchronization signal
 *
 * VI_VSYNC_NEG_HIGH: if VIU_VSYNC_E = VIU_VSYNC_FIELD,then the v-sync signal of even field is high-level,
 *		      if VIU_VSYNC_E = VIU_VSYNC_PULSE,then the v-sync pulse is positive pulse.
 * VI_VSYNC_NEG_LOW: if VIU_VSYNC_E = VIU_VSYNC_FIELD,then the v-sync signal of even field is low-level,
 *		     if VIU_VSYNC_E = VIU_VSYNC_PULSE,then the v-sync pulse is negative pulse.
 */
typedef enum _vi_vsync_neg_e {
	VI_VSYNC_NEG_HIGH = 0,
	VI_VSYNC_NEG_LOW,
	VI_VSYNC_NEG_BUTT
} vi_vsync_neg_e;

/* Attribute of the horizontal synchronization signal */
typedef enum _vi_hsync_e {
	VI_HSYNC_VALID_SINGNAL = 0, /* the h-sync is valid signal mode */
	VI_HSYNC_PULSE, /* the h-sync is pulse mode, a new pulse means the beginning of a new line */

	VI_HSYNC_BUTT
} vi_hsync_e;

/* Polarity of the horizontal synchronization signal
 *
 * VI_HSYNC_NEG_HIGH: if vi_hsync_e = VI_HSYNC_VALID_SINGNAL,then the valid h-sync signal is high-level;
 *		    if vi_hsync_e = VI_HSYNC_PULSE,then the h-sync pulse is positive pulse.
 * VI_HSYNC_NEG_LOW: if vi_hsync_e = VI_HSYNC_VALID_SINGNAL,then the valid h-sync signal is low-level;
 *		    if vi_hsync_e = VI_HSYNC_PULSE,then the h-sync pulse is negative pulse
 */
typedef enum _vi_hsync_neg_e {
	VI_HSYNC_NEG_HIGH = 0,
	VI_HSYNC_NEG_LOW,
	VI_HSYNC_NEG_BUTT
} vi_hsync_neg_e;

/* Attribute of the valid vertical synchronization signal
 *
 * VI_VSYNC_NORM_PULSE: the v-sync is pusle mode, a pusle means a new frame or field
 * VI_VSYNC_VALID_SIGNAL: the v-sync is effective mode, a effective signal means a new frame or field
 */
typedef enum _vi_vsync_valid_e {
	VI_VSYNC_NORM_PULSE = 0,
	VI_VSYNC_VALID_SIGNAL,

	VI_VSYNC_VALID_BUTT
} vi_vsync_valid_e;

/* Polarity of the valid vertical synchronization signal
 *
 * VI_VSYNC_VALID_NEG_HIGH: if vi_vsync_valid_e = VI_VSYNC_NORM_PULSE,a positive pulse means v-sync pulse;
 *			    if vi_vsync_valid_e = VI_VSYNC_VALID_SIGNAL,the valid v-sync signal is high-level
 * VI_VSYNC_VALID_NEG_LOW: if vi_vsync_valid_e = VI_VSYNC_NORM_PULSE,a negative pulse means v-sync pulse
 *			   if vi_vsync_valid_e = VI_VSYNC_VALID_SIGNAL,the valid v-sync signal is low-level
 */
typedef enum _vi_vsync_valid_neg_e {
	VI_VSYNC_VALID_NEG_HIGH = 0,
	VI_VSYNC_VALID_NEG_LOW,
	VI_VSYNC_VALID_NEG_BUTT
} vi_vsync_valid_neg_e;

typedef enum _vi_state_e {
	VI_RUNNING,
	VI_SUSPEND,
	VI_MAX,
} vi_state_e;

/* Blank information of the input timing
 *
 * vsync_vfb: RW;Vertical front blanking height of one frame or odd-field frame picture
 * vsync_vact: RW;Vertical effetive width of one frame or odd-field frame picture
 * vsync_vbb: RW;Vertical back blanking height of one frame or odd-field frame picture
 * vsync_vbfb: RW;Even-field vertical front blanking height when input mode is interlace
 *		(invalid when progressive input mode)
 * vsync_vbact: RW;Even-field vertical effetive width when input mode is interlace
 *		(invalid when progressive input mode)
 * vsync_vbbb: RW;Even-field vertical back blanking height when input mode is interlace
 *		(invalid when progressive input mode)
 */
typedef struct _vi_timing_blank_s {
	unsigned int hsync_hfb; /* RW;Horizontal front blanking width */
	unsigned int hsync_act; /* RW;Horizontal effetive width */
	unsigned int hsync_hbb; /* RW;Horizontal back blanking width */
	unsigned int vsync_vfb;
	unsigned int vsync_vact;
	unsigned int vsync_vbb;
	unsigned int vsync_vbfb;
	unsigned int vsync_vbact;
	unsigned int vsync_vbbb;
} vi_timing_blank_s;

/* synchronization information about the BT.601 or DC timing */
typedef struct _vi_sync_cfg_s {
	vi_vsync_e vsync;
	vi_vsync_neg_e vsync_neg;
	vi_hsync_e hsync;
	vi_hsync_neg_e hsync_neg;
	vi_vsync_valid_e vsync_valid;
	vi_vsync_valid_neg_e vsync_valid_neg;
	vi_timing_blank_s timing_blank;
} vi_sync_cfg_s;

/* the highest bit of the BT.656 timing reference code */
typedef enum _vi_bt656_fixcode_e {
	VI_BT656_FIXCODE_1 = 0, /* The highest bit of the EAV/SAV data over the BT.656 protocol is always 1. */
	VI_BT656_FIXCODE_0, /* The highest bit of the EAV/SAV data over the BT.656 protocol is always 0. */

	VI_BT656_FIXCODE_BUTT
} vi_bt656_fixcode_e;

/* Polarity of the field indicator bit (F) of the BT.656 timing reference code */
typedef enum _vi_bt656_field_polar_e {
	VI_BT656_FIELD_POLAR_STD = 0, /* the standard BT.656 mode,the first filed F=0,the second filed F=1 */
	VI_BT656_FIELD_POLAR_NSTD, /* the non-standard BT.656 mode,the first filed F=1,the second filed F=0 */

	VI_BT656_FIELD_POLAR_BUTT
} vi_bt656_field_polar_e;

/* synchronization information about the BT.656 */
typedef struct _vi_bt656_sync_cfg_s {
	vi_bt656_fixcode_e fix_code;
	vi_bt656_field_polar_e field_polar;
} vi_bt656_sync_cfg_s;

/* Input data type */
typedef enum _vi_data_type_s {
	VI_DATA_TYPE_YUV = 0,
	VI_DATA_TYPE_RGB,
	VI_DATA_TYPE_YUV_EARLY,

	VI_DATA_TYPE_BUTT
} vi_data_type_s;

/* Attribute of wdr */
typedef struct _vi_wdr_attr_s {
	wdr_mode_e wdr_mode; /* RW; WDR mode.*/
	unsigned int cache_line; /* RW; WDR cache line.*/
} vi_wdr_attr_s;

/* the extended attributes of VI device
 *
 * input_data_type: RW;RGB: CSC-709 or CSC-601, PT YUV444 disable; YUV: default yuv CSC coef PT YUV444 enable.
 */
typedef struct _vi_dev_attr_ex_s {
	vi_input_mode_e input_mode; /* RW;Input mode */
	vi_work_mode_e work_mode; /* RW; Work mode */

	vi_combine_mode_e combine_mode; /* RW;Y/C composite or separation mode */
	vi_component_mode_e component_mode; /* RW;Component mode (single-component or dual-component) */
	vi_clk_edge_e clk_edge; /* RW;Clock edge mode (sampling on the rising or falling edge) */

	unsigned int component_mask[VI_COMPMASK_NUM]; /* RW;Component mask */

	vi_scan_mode_e scan_mode; /* RW;Input scanning mode (progressive or interlaced) */
	int ad_chn_id[VI_MAX_ADCHN_NUM]; /* RW;AD channel ID. Typically, the default value -1 is recommended */

	vi_yuv_data_seq_e data_seq; /* RW;Input data sequence (only the YUV format is supported) */
	vi_sync_cfg_s sync_cfg; /* RW;Sync timing. This member must be configured in BT.601 mode or DC mode */

	vi_bt656_sync_cfg_s bt656_sync_cfg; /* RW;Sync timing. This member must be configured in BT.656 mode */

	vi_data_type_s input_data_type;

	unsigned char data_reverse; /* RW;Data reverse */

	size_s size; /* RW;Input size */

	vi_wdr_attr_s wdr_attr; /* RW;Attribute of WDR */
} vi_dev_attr_ex_s;

/* The attributes of a VI device
 *
 * input_data_type: RW;RGB: CSC-709 or CSC-601, PT YUV444 disable; YUV: default yuv CSC coef PT YUV444 enable.
 */
typedef struct _vi_dev_attr_s {
	vi_intf_mode_e intf_mode; /* RW;Interface mode */
	vi_work_mode_e work_mode; /* RW;Work mode */

	vi_scan_mode_e scan_mode; /* RW;Input scanning mode (progressive or interlaced) */
	int ad_chn_id[VI_MAX_ADCHN_NUM]; /* RW;AD channel ID. Typically, the default value -1 is recommended */

	/* The below members must be configured in BT.601 mode or DC mode and are invalid in other modes */
	vi_yuv_data_seq_e data_seq; /* RW;Input data sequence (only the YUV format is supported) */
	vi_sync_cfg_s sync_cfg; /* RW;Sync timing. This member must be configured in BT.601 mode or DC mode */

	vi_data_type_s input_data_type;

	size_s size; /* RW;Input size */

	vi_wdr_attr_s wdr_attr; /* RW;Attribute of WDR */

	bayer_format_e bayer_format; /* RW;Bayer format of Device */

	unsigned int chn_num; /* R; total chnannels sended from dev */

	unsigned int snr_fps; /* R; snr init fps from isp pub attr */

	vi_isp_yuv_scene_e yuv_scene_mode; /* RW;YUV scene mode */
} vi_dev_attr_s;

/* Information of pipe binded to device */
typedef struct _vi_dev_bind_pipe_s {
	unsigned int num; /* RW;Range [1,VI_MAX_PIPE_NUM] */
	__s32 pipe_id[VI_MAX_PIPE_NUM]; /* RW;Array of pipe ID */
} vi_dev_bind_pipe_s;

/* Source of 3DNR reference frame */
typedef enum _vi_nr_ref_source_e {
	VI_NR_REF_FROM_RFR = 0, /* Reference frame from reconstruction frame */
	VI_NR_REF_FROM_CHN0, /* Reference frame from CHN0's frame */

	VI_NR_REF_FROM_BUTT
} vi_nr_ref_source_e;

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _vi_pipe_bypass_mode_e {
	VI_PIPE_BYPASS_NONE,
	VI_PIPE_BYPASS_FE,
	VI_PIPE_BYPASS_BE,

	VI_PIPE_BYPASS_BUTT
} vi_pipe_bypass_mode_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

/* The attributes of 3DNR */
typedef struct _vi_nr_attr_s {
	pixel_format_e pixel_fmt; /* RW;Pixel format of reference frame */
	data_bitwidth_e bit_width; /* RW;Bit Width of reference frame */
	vi_nr_ref_source_e nr_ref_source; /* RW;Source of 3DNR reference frame */
	compress_mode_e compress_mode; /* RW;Reference frame compress mode */
} vi_nr_attr_s;

/* The attributes of pipe
 *
 * discard_pro_pic: RW;when professional mode snap, whether to discard long exposure picture in the video pipe.
 */
// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef struct _vi_pipe_attr_s {
	vi_pipe_bypass_mode_e pipe_bypass_mode;
	unsigned char yuv_skip; /* RW;YUV skip enable */
	unsigned char isp_bypass; /* RW;ISP bypass enable */
	unsigned int max_width; /* RW;Range[VI_PIPE_MIN_WIDTH,VI_PIPE_MAX_WIDTH];Maximum width */
	unsigned int max_height; /* RW;Range[VI_PIPE_MIN_HEIGHT,VI_PIPE_MAX_HEIGHT];Maximum height */
	pixel_format_e pixel_fmt; /* RW;Pixel format */
	compress_mode_e compress_mode; /* RW;Compress mode.*/
	data_bitwidth_e bit_width; /* RW;Bit width*/
	unsigned char nr_en; /* RW;3DNR enable */
	unsigned char sharpen_en; /* RW;Sharpen enable*/
	frame_rate_ctrl_s frame_rate; /* RW;Frame rate */
	unsigned char discard_pro_pic;
	unsigned char yuv_bypass_path; /* RW;ISP YUV bypass enable */
} vi_pipe_attr_s;
// -------- If you want to change these interfaces, please contact the isp team. --------

/*
 * texture_str: RW; range: [0, 4095]; Format:7.5;Undirectional sharpen strength for texture and detail enhancement.
 * edge_str: RW; range: [0, 4095]; Format:7.5;Directional sharpen strength for edge enhancement.
 * texture_freq: RW; range: [0, 4095]; Format:6.6; Texture frequency adjustment.
 *		   Texture and detail will be finer when it increase.
 * edge_freq: RW; range: [0, 4095]; Format:6.6; Edge frequency adjustment.
 *		Edge will be narrower and thiner when it increase.
 * shoot_sup_str: RW; range: [0, 255]; Format:8.0;overshoot and undershoot suppression strength,
 *		  the amplitude and width of shoot will be decrease when shootSupSt increase.
 */
typedef struct _vi_pipe_sharpen_manual_attr_s {
	unsigned short texture_str[VI_SHARPEN_GAIN_NUM];
	unsigned short edge_str[VI_SHARPEN_GAIN_NUM];
	unsigned short texture_freq;
	unsigned short edge_freq;
	unsigned char over_shoot; /* RW; range: [0, 127];  Format:7.0;over_shootAmt*/
	unsigned char under_shoot; /* RW; range: [0, 127];  Format:7.0;under_shootAmt*/
	unsigned char shoot_sup_str;

} vi_pipe_sharpen_manual_attr_s;

/*
 * texture_str: RW; range: [0, 4095]; Format:7.5;Undirectional sharpen strength for texture and detail enhancement.
 * edge_str:  RW; range: [0, 4095]; Format:7.5;Directional sharpen strength for edge enhancement
 * atexture_freq: RW; range: [0, 4095]; Format:6.6;Texture frequency adjustment.
 *		    Texture and detail will be finer when it increase
 * aedge_freq: RW; range: [0, 4095]; Format:6.6;Edge frequency adjustment.
 *		 Edge will be narrower and thiner when it increase
 * ashoot_sup_str: RW; range: [0, 255]; Format:8.0;overshoot and undershoot suppression strength,
 *		   the amplitude and width of shoot will be decrease when shootSupSt increase
 */
typedef struct _vi_pipe_sharpen_auto_attr_s {
	unsigned short texture_str[VI_SHARPEN_GAIN_NUM][VI_AUTO_ISO_STRENGTH_NUM];
	unsigned short edge_str[VI_SHARPEN_GAIN_NUM][VI_AUTO_ISO_STRENGTH_NUM];
	unsigned short atexture_freq[VI_AUTO_ISO_STRENGTH_NUM];
	unsigned short aedge_freq[VI_AUTO_ISO_STRENGTH_NUM];
	unsigned char aover_shoot[VI_AUTO_ISO_STRENGTH_NUM]; /* RW; range: [0, 127];  Format:7.0;over_shootAmt*/
	unsigned char aunder_shoot[VI_AUTO_ISO_STRENGTH_NUM]; /* RW; range: [0, 127];  Format:7.0;under_shootAmt*/
	unsigned char ashoot_sup_str[VI_AUTO_ISO_STRENGTH_NUM];

} vi_pipe_sharpen_auto_attr_s;

typedef struct _vi_pipe_sharpen_attr_s {
	operation_mode_e op_type;
	unsigned char luma_wgt[VI_SHARPEN_GAIN_NUM]; /* RW; range: [0, 127];  Format:7.0;*/
	vi_pipe_sharpen_manual_attr_s sharpen_manual_attr;
	vi_pipe_sharpen_auto_attr_s sharpen_auto_attr;
} vi_pipe_sharpen_attr_s;

typedef enum _vi_pipe_repeat_mode_s {
	VI_PIPE_REPEAT_NONE = 0,
	VI_PIPE_REPEAT_ONCE = 1,
	VI_PIPE_REPEAT_BUTT
} vi_pipe_repeat_mode_s;

/*
 * IES: RW; Range:[0,255];Format 8.0;the absolute strength of image enhancement for edge
 * IESS:RW; Range:[0,255];Format 8.0;the absolute strength of image enhancement for texture and shadow
 * IEDZ: RW; Range:[0,8192];Format 14.0;the threshold of image enhancement for controlling noise
 */
typedef struct {
	unsigned char IES;
	unsigned char IESS;
	unsigned short IEDZ;
} tv59a_iey;

/*
 * SBF: RW; Range:[0,3];Format 2.0;the band type of spatial filter,
 *	notice: SBF0,SBF1 range is [2,3];SBF2,SBF3 range is [0,3]
 * STR: RW; Range:[0,13];Format 4.0;the relative strength of spatial filter refer to the previous frame
 * STHp: RW; Range:[0,2];Format 2.0;Not recommended for debugging
 * SFT: RW; Range:[0,31];Format 5.0;Not recommended for debugging
 * kPro: RW; Range:[0,7];Format 3.0;notice: the kPro of sfy2 range is [0,7],the kPro of sfy3 range is [0,4]
 * STH: RW; Range:[0,999];Format 10.0;the edge-preserve threshold for spatial filter
 * SBS: RW; Range:[0,9999];Format 14.0;the noise reduction strength of spatial filter for the relative bright pixel
 * SDS: RW; Range:[0,9999];Format 14.0;the noise reduction strength of spatial filter for the relative dark pixel
 */
typedef struct {
	unsigned char SBF : 2;
	unsigned char STR : 4;
	unsigned char STHp : 2;
	unsigned char SFT : 5;
	unsigned char kPro : 3;

	unsigned short STH[3];
	unsigned short SBS[3];
	unsigned short SDS[3];

} tv59a_sfy;

typedef struct {
	unsigned short MATH : 10; /* RW; Range:[0,1023];Format 10.0;the motion detection threshold for temporal filter */
	unsigned short MATE : 4; /* RW; Range:[0,11];Format 4.0;the motion detection index of flat area for temporal filter */
	unsigned short MATW : 2; /* RW; Range:[0,3];Format 2.0;the index of suppressing trailing for temporal filter */
	unsigned char MASW : 4; /* RW; Range:[0,12];Format 4.0;the index of suppressing raindrop noise for temporal filter */
	unsigned char MABW : 3; /* RW; Range:[0,4];Format 3.0;the window of motion detection for temporal filter */
	unsigned char MAXN : 1; /* RW; Range:[0,1];Format 1.0;Not recommended for debugging */

} tv59a_mdy;

typedef struct {
	unsigned char TFR[4]; /* RW; Range:[0,255];Format 8.0;the relative strength of temporal filter for the static area */
	unsigned short TDZ : 14; /* RW; Range:[0,999];Format 10.0;the threshold of dead-area of temporal filter */
	unsigned short TDX : 2; /* RW; Range:[0,2];Format 2.0;Not recommended for debugging */
	unsigned short TFS : 6; /* RW; Range:[0,63];Format 6.0;the absolute strength of temporal filter  */
} tv59a_tfy;

typedef struct {
	unsigned short SFC : 10; /* RW; Range:[0,1023];Format 10.0;the strength of spatial filter for NRC0 */
	unsigned short TFC : 6; /* RW; Range:[0,63];Format 6.0;the strength of temporal filter for NRC0 */
	unsigned short CSFS : 14; /* RW; Range:[0,999];Format 10.0;the strength of spatial filter for NRC1 */
	unsigned short CSFk : 2; /* RW; Range:[0,3];Format 2.0;Not recommended for debugging */
	unsigned short CTFS : 4; /* RW; Range:[0,15];Format 4.0;the strength of temporal filter for NRC1 */
	unsigned short CIIR : 1; /* RW; Range:[0,1];Format 1.0;the mode of spatial filter for NRC1 */
	unsigned short CTFR : 11; /* RW; Range:[0,999];Format 10.0;the relative strength of temporal filter for NRC1 */

} tv_59a_nrc;

/* 3DNR Spatial Filter: sfy0,sfy1,sfy2,sfy3; Temporal Filter:TFy0,TFy1;Chroma Noise Reduction: NRC0,NRC1
 *
 * hdg_type: RW; Range:[0,1];Format 1.0;the type of complexed mixed spatial filter whether is SFi or SFk
 * bri_type: RW; Range:[0,1];Format 1.0;the mode decide sfy3 whether is SFk type or SFi type
 * hdg_mode: RW; Range:[0,3];Format 2.0;the mode decide complexed mixed spatial filter band for flat area
 * k_tab2: RW; Range:[0,1];Format 1.0;the parameter decide sfy2 whether or not based on the image absolute luminance
 * hdg_wnd: RW; Range:[0,1];Format 1.0;the sampling window of complexed mixed spatial filter for noise detection
 * k_tab3: RW; Range:[0,1];Format 1.0;the parameter decide sfy3 whether or not based on the image absolute luminance
 * hdg_sfr: RW; Range:[0,13];Format 4.0;the trend of the noise reduction of complexed mixed spatial filter for flat area
 * n_out: RW; Range:[0,27];Format 5.0;the parameter for output intermediate result of sfy3
 * hdg_ies: RW; Range:[0,255];Format 8.0;the strength of image enhancement for complexed mixed spatial filter
 * ref: RW; Range:[0,1];Format 1.0;Not recommended for debugging
 *
 * SFRi: RW; Range:[0,255];Format 8.0;the relative strength of sfy3 when the filter type is SFi
 * SFRk: RW; Range:[0,255];Format 8.0;the relative strength of sfy3 when the filter type is SFk
 * SBSk2: RW; Range:[0,9999];Format 14.0;the noise reduction strength of sfy2 for the relative bright pixel
 *	  based on the image absolute luminance
 * SBSk3: RW; Range:[0,9999];Format 14.0;the noise reduction strength of sfy3 for the relative bright pixel
 *	  based on the image absolute luminance
 * SDSk2: RW; Range:[0,9999];Format 14.0;the noise reduction strength of sfy2 for the relative dark pixel
 *	  based on the image absolute luminance
 * SDSk3: RW; Range:[0,9999];Format 14.0;the noise reduction strength of sfy3 for the relative dark pixel
 *	  based on the image absolute luminance
 * BriThr: RW; Range:[0,1024];Format 11.0;the threshold decide sfy3 choose the SFi type filter or SFk type filter
 *	  in dark and bright area
 *
 */
typedef struct {
	tv59a_iey iey;
	tv59a_sfy sfy[5];
	tv59a_mdy mdy[2];
	tv59a_tfy tfy[2];

	unsigned short hdg_type : 1;
	unsigned short bri_type : 1;
	unsigned short hdg_mode : 2;
	unsigned short k_tab2 : 1;
	unsigned short hdg_wnd : 1;
	unsigned short k_tab3 : 1;
	unsigned short hdg_sfr : 4;
	unsigned short n_out : 5;
	unsigned char hdg_ies;
	unsigned char ref : 1;

	unsigned char iey_mode : 1;
	unsigned char iey_ex[4];

	unsigned char SFRi[4];
	unsigned char SFRk[4];
	unsigned short SBSk2[32];
	unsigned short SBSk3[32];
	unsigned short SDSk2[32];
	unsigned short SDSk3[32];
	unsigned short BriThr[16];

	tv_59a_nrc nrc;
} vi_pipe_nrx_param_v1_s;

typedef enum _vi_nr_version_e { VI_NR_V1 = 1, VI_NR_V2 = 2, VI_NR_V3 = 3, VI_NR_V4 = 4, VI_NR_BUTT } vi_nr_version_e;

typedef struct _nrx_param_manual_v1_s {
	vi_pipe_nrx_param_v1_s nrx_param_v1;
} nrx_param_manual_v1_s;

typedef struct _nrx_param_auto_v1_s {
	unsigned int u32ParamNum;

	unsigned int *ATTRIBUTE pau32ISO;
	vi_pipe_nrx_param_v1_s *ATTRIBUTE past_nrx_param_v1;
} nrx_param_auto_v1_s;

typedef struct _nrx_param_v1_s {
	operation_mode_e opt_mode; /* RW;Adaptive NR */
	nrx_param_manual_v1_s nrx_manual_v1; /* RW;NRX V1 param for manual */
	nrx_param_auto_v1_s nrx_auto_v1; /* RW;NRX V1 param for auto */
} nrx_param_v1_s;

typedef struct {
	unsigned char ies0, ies1, ies2, ies3;
	unsigned short iedz : 10, _rb_ : 6;
} tv500_vi_iey;

typedef struct {
	unsigned char spn6 : 3, sfr : 5;
	unsigned char sbn6 : 3, pbr6 : 5;
	unsigned short srt0 : 5, srt1 : 5, jmode : 3, deidx : 3;
	unsigned char derate, sfr6[3];

	unsigned char sfs1, sft1, sbr1;
	unsigned char sfs2, sft2, sbr2;
	unsigned char sfs4, sft4, sbr4;

	unsigned short sth1 : 9, sfn1 : 3, nryen : 1, sfn0 : 3;
	unsigned short sth2 : 9, sfn2 : 3, bwsf4 : 1, kmode : 3;
	unsigned short sth3 : 9, sfn3 : 3, trith : 1, _rb0_ : 3;
} tv500_vi_sfy;

typedef struct {
	tv500_vi_iey iey;
	tv500_vi_sfy sfy;
} vi_pipe_nrx_param_v2_s;

typedef struct _nrx_param_manual_v2_s {
	vi_pipe_nrx_param_v2_s nrx_param_v2;
} nrx_param_manual_v2_s;

typedef struct _nrx_param_auto_v2_s {
	unsigned int u32ParamNum;

	unsigned int *ATTRIBUTE pau32ISO;
	vi_pipe_nrx_param_v2_s *ATTRIBUTE past_nrx_param_v2;

} nrx_param_auto_v2_s;

typedef struct _nrx_param_v2_s {
	operation_mode_e opt_mode; /* RW;Adaptive NR */
	nrx_param_manual_v2_s mrx_manual_v2; /* RW;NRX V2 param for manual */
	nrx_param_auto_v2_s mrx_auto_v2; /* RW;NRX V2 param for auto */
} nrx_param_v2_s;

typedef struct _vi_pipe_nrx_param_s {
	vi_nr_version_e nr_version; /* RW;3DNR Version*/
	union {
		nrx_param_v1_s nrx_param_v1; /* RW;3DNR X param version 1 */
		nrx_param_v2_s nrx_param_v2; /* RW;3DNR X param version 2 */
	};
} vi_pipe_nrx_param_s;

/* The attributes of channel */
typedef struct _vi_chn_attr_s {
	size_s size; /* RW;Channel out put size */
	pixel_format_e pixel_format; /* RW;Pixel format */
	dynamic_range_e dynamic_range; /* RW;Dynamic Range */
	video_format_e video_format; /* RW;Video format */
	compress_mode_e compress_mode; /* RW;256B Segment compress or no compress. */
	unsigned char mirror; /* RW;Mirror enable */
	unsigned char flip; /* RW;Flip enable */
	unsigned int depth; /* RW;Range [0,8];Depth */
	frame_rate_ctrl_s frame_rate; /* RW;Frame rate */
	unsigned int bind_vb_pool; /*chn bind vb*/
} vi_chn_attr_s;

/* The status of pipe */
typedef struct _vi_pipe_status_s {
	unsigned char enable; /* RO;Whether this pipe is enabled */
	unsigned int int_cnt; /* RO;The video frame interrupt count */
	unsigned int frame_rate; /* RO;Current frame rate */
	unsigned int lost_frame; /* RO;Lost frame count */
	unsigned int vb_fail; /* RO;Video buffer malloc failure */
	size_s size; /* RO;Current pipe output size */
} vi_pipe_status_s;

/* VS signal output mode */
typedef enum _vi_vs_signal_mode_e {
	VI_VS_SIGNAL_ONCE = 0, /* output one time */
	VI_VS_SIGNAL_FREQ, /* output frequently */

	VI_VS_SIGNAL_MODE_BUTT
} vi_vs_signal_mode_e;

/* The attributes of VS signal */
typedef struct _vi_vs_signal_attr_s {
	vi_vs_signal_mode_e mode; /* RW;output one time, output frequently*/
	unsigned int u32StartTime; /* RW;output start time,unit: sensor pix clk.*/
	unsigned int u32Duration; /* RW;output high duration, unit: sensor pix clk.*/
	unsigned int u32CapFrmIndex; /* RW;VS signal will be output after trigger by which vframe, default is 0. */
	unsigned int u32Interval; /* RW;output frequently interval, unit: frame*/
} vi_vs_signal_attr_s;

typedef enum _vi_ext_chn_source_e {
	VI_EXT_CHN_SOURCE_TAIL,
	VI_EXT_CHN_SOURCE_HEAD,

	VI_EXT_CHN_SOURCE_BUTT
} vi_ext_chn_source_e;

typedef struct _vi_ext_chn_attr_s {
	vi_ext_chn_source_e source;
	__s32 bind_chn; /* RW;Range [VI_CHN0, VI_MAX_PHY_CHN_NUM);The channel num which extend channel will bind to*/
	size_s size; /* RW;Channel out put size */
	pixel_format_e pixel_format; /* RW;Pixel format */
	unsigned int depth; /* RW;Range [0,8];Depth */
	frame_rate_ctrl_s frame_rate; /* RW;Frame rate */
} vi_ext_chn_attr_s;

typedef enum _vi_crop_coordinate_e {
	VI_CROP_RATIO_COOR = 0, /* Ratio coordinate */
	VI_CROP_ABS_COOR, /* Absolute coordinate */
	VI_CROP_BUTT
} vi_crop_coordinate_e;

/* Information of chn crop */
typedef struct _vi_crop_info_s {
	unsigned char enable; /* RW;CROP enable*/
	vi_crop_coordinate_e crop_coordinate; /* RW;Coordinate mode of the crop start point*/
	rect_s crop_rect; /* RW;CROP rectangular*/
} vi_crop_info_s;

/* The attributes of LDC */
typedef struct _vi_ldc_attr_s {
	unsigned char enable; /* RW;Whether LDC is enable */
	ldc_attr_s attr;
} vi_ldc_attr_s;

/* The status of chn */
typedef struct _vi_chn_status_s {
	unsigned char enable; /* RO;Whether this channel is enabled */
	unsigned int frame_rate; /* RO;current frame rate */
	__u64 prev_time; // latest time (us)
	unsigned int frame_num;  //The number of Frame in one second
	unsigned int lost_frame; /* RO;Lost frame count */
	unsigned int vb_fail; /* RO;Video buffer malloc failure */
	unsigned int int_cnt; /* RO;Receive frame int count */
	unsigned int recv_pic; /* RO;Receive frame count */
	unsigned int total_mem_byte; /* RO;VI buffer malloc failure */
	size_s size; /* RO;chn output size */

} vi_chn_status_s;

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef enum _vi_dump_type_e {
	VI_DUMP_TYPE_RAW = 0,
	VI_DUMP_TYPE_YUV = 1,
	VI_DUMP_TYPE_IR = 2,
	VI_DUMP_TYPE_BUTT
} vi_dump_type_e;
// -------- If you want to change these interfaces, please contact the isp team. --------

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef struct _vi_dump_attr_s {
	unsigned char enable; /* RW;Whether dump is enable */
	unsigned int depth; /* RW;Range [0,8];Depth */
	vi_dump_type_e dump_type;
} vi_dump_attr_s;
// -------- If you want to change these interfaces, please contact the isp team. --------

typedef enum _vi_pipe_frame_source_e {
	VI_PIPE_FRAME_SOURCE_DEV = 0, /* RW;Source from dev */
	VI_PIPE_FRAME_SOURCE_USER_FE, /* RW;User send to FE */
	VI_PIPE_FRAME_SOURCE_USER_BE, /* RW;User send to BE */

	VI_PIPE_FRAME_SOURCE_BUTT
} vi_pipe_frame_source_e;

typedef struct _vi_raw_info_s {
	video_frame_info_s video_frame;
	isp_config_info_s isp_info;
} vi_raw_info_s;

/* module params */
typedef struct _vi_mode_param_s {
	int detect_err_frame;
	unsigned int drop_err_frame;
} vi_mode_param_s;

typedef struct _vi_dev_timing_attr_s {
	unsigned char enable; /* RW;Whether enable VI generate timing */
	int frm_rate; /* RW;Generate timing Frame rate*/
} vi_dev_timing_attr_s;

typedef struct _vi_early_interrupt_s {
	unsigned char enable;
	unsigned int line_cnt;
} vi_early_interrupt_s;

/* VI dump register table */
typedef struct _mlsc_gain_lut_s {
	unsigned short *r_gain;
	unsigned short *g_gain;
	unsigned short *b_gain;
} mlsc_gain_lut_s;

typedef struct _vi_dump_register_table_s {
	mlsc_gain_lut_s mlsc_gain_lut;
} vi_dump_register_table_s;

typedef int (*pfn_videv_pm_ops)(void *pv_data);

typedef struct _vi_pm_ops_s {
	pfn_videv_pm_ops sns_suspend;
	pfn_videv_pm_ops sns_resume;
	pfn_videv_pm_ops mipi_suspend;
	pfn_videv_pm_ops mipi_resume;
} vi_pm_ops_s;

typedef struct _vi_smooth_raw_dump_info_s {
	__s32 vi_pipe;
	unsigned char  blk_cnt;	// ring buffer number
	__u64 *phy_addr_list;	// ring buffer addr
	rect_s  crop_rect;
} vi_smooth_raw_dump_info_s;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef__COMM_VI_IN_H__ */

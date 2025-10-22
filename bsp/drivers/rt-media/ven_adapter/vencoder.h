/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : vencoder.h
* Description :
* History :
*   Author  : fangning <fangning@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/

/*
 *this software is based in part on the work
 * of the Independent JPEG Group
 */
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "ve_interface.h"
#include "../memory/mem_interface.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef _VENCODER_H_
#define _VENCODER_H_

#define DATA_TIME_LENGTH 24
#define INFO_LENGTH 64
#define GPS_PROCESS_METHOD_LENGTH 100
#define DESCRIPTOR_INFO 128
#define PROC_BUF_LEN 1024
#define MAX_CHANNEL_NUM 16

#define MAX_FRM_NUM 5
#define MAX_GOP_SIZE 63
#define MAX_OVERLAY_SIZE 64
#define MAX_RC_GOP_SIZE 256

#define VENCODER_TMP_RATIO (14)

#define VENC_BUFFERFLAG_KEYFRAME 0x00000001
#define VENC_BUFFERFLAG_EOS 0x00000002
#define VENC_BUFFERFLAG_THUMB 0x00000004

#define H264_VERSION2_USE64 0
#define SHARP_ROI_MAX_NUM (8)

#define AE_AVG_ROW  (18)
#define AE_AVG_COL  (24)
#define AE_AVG_NUM  (432)
#define AE_HIST_NUM (256)

#define DEFAULT_MOTION_SEARCH_HOR_REGION_NUM (10)
#define DEFAULT_MOTION_SEARCH_VER_REGION_NUM (5)
#define DEFAULT_REGION_D3D_HOR_REGION_NUM    (15)
#define DEFAULT_REGION_D3D_VER_REGION_NUM    (8)

#define DEBUG_SHOW_ENCODE_TIME     1
/** VENC_IN is used to identify inputs to an VENC function.  This designation
    will also be used in the case of a pointer that points to a parameter
    that is used as an output. */
#ifndef VENC_IN
#define VENC_IN
#endif

/** VENC_OUT is used to identify outputs from an VENC function.  This
    designation will also be used in the case of a pointer that points
    to a parameter that is used as an input. */
#ifndef VENC_OUT
#define VENC_OUT
#endif

#define PRINTF_CODE_POS
//logd("func = %s, line = %d", __FUNCTION__, __LINE__);

typedef enum eVencEncppScalerRatio {
	VENC_ISP_SCALER_0       = 0, //not scaler
	VENC_ISP_SCALER_EIGHTH  = 1, //scaler 1/8 write back
	VENC_ISP_SCALER_HALF    = 2, //scaler 1/2 write back
	VENC_ISP_SCALER_QUARTER = 3, //scaler 1/4 write back
} eVencEncppScalerRatio;

typedef struct sWbYuvParam {
	unsigned int bEnableWbYuv;
	unsigned int nWbBufferNum;
	eVencEncppScalerRatio scalerRatio;
} sWbYuvParam;

typedef enum eIspOnlineStatus {
	ISP_ONLINE_STATUS_READY     = 0,
	ISP_ONLINE_STATUS_NOT_READY = 1,
} eIspOnlineStatus;

typedef struct s3DfilterParam {
	unsigned char enable_3d_filter;
	unsigned char adjust_pix_level_enable; // adjustment of coef pix level enable
	unsigned char smooth_filter_enable; //* 3x3 smooth filter enable
	unsigned char max_pix_diff_th; //* range[0~31]: maximum threshold of pixel difference
	unsigned char max_mad_th; //* range[0~63]: maximum threshold of mad
	unsigned char max_mv_th; //* range[0~63]: maximum threshold of motion vector
	unsigned char min_coef; //* range[0~16]: minimum weight of 3d filter
	unsigned char max_coef; //* range[0~16]: maximum weight of 3d filter,
} s3DfilterParam;

typedef struct VencRegionD3DRegion {
	int pix_x_bgn;
	int pix_x_end;
	int pix_y_bgn;
	int pix_y_end;
	int total_num;
	int zero_mv_num;
	int is_possible_static;
	int is_really_static;
	float zero_mv_rate;
} VencRegionD3DRegion;

typedef struct VencRegionD3DResult {
	int total_region_num;
	int static_region_num;
	VencRegionD3DRegion *region;
} VencRegionD3DResult;

typedef struct VencRegionD3DParam {
	int en_region_d3d;
	int dis_default_para;
	int hor_region_num;
	int ver_region_num;
	int hor_expand_num;
	int ver_expand_num;
	float zero_mv_rate_th[3];
	unsigned char chroma_offset;
	unsigned char static_coef[3]; //* fallow by zero_mv_rate
	unsigned char motion_coef[4]; //* fallow by MoveStatus
} VencRegionD3DParam;

typedef struct VencExtremeD3DParam {
	unsigned char  en_extreme_d3d;
	float          zero_mv_ratio_th;
	s3DfilterParam ex_d3d_param;
} VencExtremeD3DParam;

typedef struct s2DfilterParam {
	unsigned char enable_2d_filter;
	unsigned char filter_strength_uv; //* range[0~255], 0 means close 2d filter, advice: 32
	unsigned char filter_strength_y; //* range[0~255], 0 means close 2d filter, advice: 32
	unsigned char filter_th_uv; //* range[0~15], advice: 2
	unsigned char filter_th_y; //* range[0~15], advice: 2
} s2DfilterParam;

typedef struct VencIspMotionParam {
	int dis_default_para;
	int large_mv_th;
} VencIspMotionParam;

#define ISP_TABLE_LEN (22)

typedef struct {
	unsigned char is_overflow;
	unsigned short moving_level_table[ISP_TABLE_LEN * ISP_TABLE_LEN];
} MovingLevelInfo;

typedef struct VencVe2IspParam {
	int d2d_level; //[1,1024], 256 means 1X
	int d3d_level; //[1,1024], 256 means 1X
	MovingLevelInfo mMovingLevelInfo;
} VencVe2IspParam;

typedef struct VencVe2IspD2DLimit {
	int en_d2d_limit;
	int d2d_level[6];
} VencVe2IspD2DLimit;

typedef struct VencTargetBitsClipParam {
	int dis_default_para;
	int mode;
	int en_gop_clip;
	float gop_bit_ratio_th[3];
	float coef_th[5][2];
} VencTargetBitsClipParam;

typedef enum eGdcPerspFunc {
	Gdc_Persp_Only,
	Gdc_Persp_LDC
} eGdcPerspFunc;

typedef enum eGdcLensDistModel {
	Gdc_DistModel_WideAngle,
	Gdc_DistModel_FishEye
} eGdcLensDistModel;

typedef enum eGdcMountType {
	Gdc_Mount_Top,
	Gdc_Mount_Wall,
	Gdc_Mount_Bottom
} eGdcMountType;

typedef enum eGdcWarpType {
	Gdc_Warp_LDC,
	Gdc_Warp_Pano180,
	Gdc_Warp_Pano360,
	Gdc_Warp_Normal,
	Gdc_Warp_Fish2Wide,
	Gdc_Warp_Perspective,
	Gdc_Warp_BirdsEye,
	Gdc_Warp_User
} eGdcWarpType;

typedef enum eRotationType {
	RotAngle_0 = 0x0,
	RotAngle_90,
	RotAngle_180,
	RotAngle_270,
} eRotationType;

typedef enum {
    CAMERA_ADAPTIVE_STATIC = 0,
    CAMERA_FORCE_STATIC = 1,
    CAMERA_FORCE_MOVING = 2,
    CAMERA_STATUS_NUM
} eCameraStatus;

typedef struct {
	unsigned char bGDC_en;
	eGdcWarpType eWarpMode;
	eGdcMountType eMountMode;
	unsigned char bMirror;
	unsigned int calib_widht;
	unsigned int calib_height;
	float fx;
	float fy;
	float cx;
	float cy;
	float fx_scale;
	float fy_scale;
	float cx_scale;
	float cy_scale;
	eGdcLensDistModel eLensDistModel;
	float distCoef_wide_ra[3];
	float distCoef_wide_ta[2];
	float distCoef_fish_k[4];
	int centerOffsetX;
	int centerOffsetY;
	int rotateAngle;
	int radialDistortCoef;
	int trapezoidDistortCoef;
	int fanDistortCoef;
	int pan;
	int tilt;
	int zoomH;
	int zoomV;
	int scale;
	int innerRadius;
	float roll;
	float pitch;
	float yaw;
	eGdcPerspFunc perspFunc;
	float perspectiveProjMat[9];
	int birdsImg_width;
	int birdsImg_height;
	float mountHeight;
	float roiDist_ahead;
	float roiDist_left;
	float roiDist_right;
	float roiDist_bottom;

	int peaking_en;
	int peaking_clamp;
	int peak_m;
	int th_strong_edge;
	int peak_weights_strength;
} sGdcParam;

#define ISP_REG_TBL_LENGTH 33

typedef struct sEncppSharpParamStatic {
	unsigned char ss_shp_ratio;
	unsigned char ls_shp_ratio;
	unsigned short ss_dir_ratio;
	unsigned short ls_dir_ratio;
	unsigned short ss_crc_stren;
	unsigned char ss_crc_min;
	unsigned short wht_sat_ratio;
	unsigned short blk_sat_ratio;
	unsigned char wht_slp_bt;
	unsigned char blk_slp_bt;

	unsigned short sharp_ss_value[ISP_REG_TBL_LENGTH];
	unsigned short sharp_ls_value[ISP_REG_TBL_LENGTH];
	unsigned short sharp_hsv[46];
} sEncppSharpParamStatic;

typedef struct sEncppSharpParamDynamic {
	unsigned short ss_ns_lw;
	unsigned short ss_ns_hi;
	unsigned short ls_ns_lw;
	unsigned short ls_ns_hi;
	unsigned char ss_lw_cor;
	unsigned char ss_hi_cor;
	unsigned char ls_lw_cor;
	unsigned char ls_hi_cor;
	unsigned short ss_blk_stren;
	unsigned short ss_wht_stren;
	unsigned short ls_blk_stren;
	unsigned short ls_wht_stren;
	unsigned char ss_avg_smth;
	unsigned char ss_dir_smth;
	unsigned char dir_smth[4];
	unsigned char hfr_smth_ratio;
	unsigned short hfr_hf_wht_stren;
	unsigned short hfr_hf_blk_stren;
	unsigned short hfr_mf_wht_stren;
	unsigned short hfr_mf_blk_stren;
	unsigned char hfr_hf_cor_ratio;
	unsigned char hfr_mf_cor_ratio;
	unsigned short hfr_hf_mix_ratio;
	unsigned short hfr_mf_mix_ratio;
	unsigned char hfr_hf_mix_min_ratio;
	unsigned char hfr_mf_mix_min_ratio;
	unsigned short hfr_hf_wht_clp;
	unsigned short hfr_hf_blk_clp;
	unsigned short hfr_mf_wht_clp;
	unsigned short hfr_mf_blk_clp;
	unsigned short wht_clp_para;
	unsigned short blk_clp_para;
	unsigned char wht_clp_slp;
	unsigned char blk_clp_slp;
	unsigned char max_clp_ratio;

	unsigned short sharp_edge_lum[ISP_REG_TBL_LENGTH];
} sEncppSharpParamDynamic;

typedef struct sEncppSharpParam {
	sEncppSharpParamDynamic *pDynamicParam;
	sEncppSharpParamStatic *pStaticParam;
} sEncppSharpParam;

typedef struct {
    int win_pix_n;
    char avg[AE_AVG_NUM];
    //int hist[256];
    //int hist1[256];
} sIspAeStatus;

typedef struct {
    eCameraStatus mEnCameraMove;
    int mEnvLv;
    int mAeWeightLum;
    sEncppSharpParam mSharpParam;
    sIspAeStatus mIspAeStatus;
} VencIsp2VeParam;


#define US_PER_MS (1 * 1000)
#define US_PER_S (1 * 1000 * 1000)
#define MS_PER_S (1 * 1000)

typedef enum {
	PSKIP	= 0,
	BSKIP_DIRECT = 0,
	P16x16       = 1,
	P16x8	= 2,
	P8x16	= 3,
	SMB8x8       = 4,
	SMB8x4       = 5,
	SMB4x8       = 6,
	SMB4x4       = 7,
	P8x8	 = 8,
	I4MB	 = 9,
	I16MB	= 10,
	IBLOCK       = 11,
	SI4MB	= 12,
	I8MB	 = 13,
	IPCM	 = 14,
	MAXMODE      = 15
} MB_TYPE;

typedef struct {
	short mv_x;
	short mv_y;
	int mode;
	unsigned char lt_flag;
	unsigned short depth;
} VencMotionVector;

typedef struct {
	MB_TYPE mb_type;
	VencMotionVector mb_mv;
} MbMvList;

typedef struct rational_t {
	unsigned int num;
	unsigned int den;
} rational_t;

typedef struct srational_t {
	int num;
	int den;
} srational_t;

typedef enum ExifMeteringModeType {
	UNKNOWN_AW_EXIF,
	AVERAGE_AW_EXIF,
	CENTER_AW_EXIF,
	SPOT_AW_EXIF,
	MULTISPOT_AW_EXIF,
	PATTERN_AW_EXIF,
	PARTIAL_AW_EXIF,
	OTHER_AW_EXIF = 255,
} ExifMeteringModeType;

typedef enum ExifExposureModeType {
	EXPOSURE_AUTO_AW_EXIF,
	EXPOSURE_MANUAL_AW_EXIF,
	EXPOSURE_AUTO_BRACKET_AW_EXIF,
} ExifExposureModeType;

typedef enum {
	UNKNOWN			     = 0,
	SUNLIGHT		     = 1,
	TUNGSTEN_LAMP		     = 2,
	FILAMENT_LAMP		     = 3,
	FLASH_LAMP		     = 4,
	OVERCAST		     = 9,
	CLOUDY			     = 10,
	SHADOW			     = 11,
	INCANDESCENT_LAMP	    = 12,
	WHITE_DAY_FLUORESCENT_LAMP   = 13,
	COOL_COLOUR_FLUORESCENT_LAMP = 14,
	WHITE_FLUORESCENT_LAMP       = 15,
	STANDARD_LAMP_A		     = 17,
	STANDARD_LAMP_B		     = 18,
	STANDARD_lAMP_C		     = 19,
	D55			     = 20,
	D65			     = 21,
	D75			     = 22,
	D50			     = 23,
	PROJECTION_ROOM_LAMP	 = 24,
	OTHERS			     = 255
} ExifLightSource;

typedef struct EXIFInfo {
	unsigned char CameraMake[INFO_LENGTH];
	unsigned char CameraModel[INFO_LENGTH];
	unsigned char DateTime[DATA_TIME_LENGTH];

	unsigned int ThumbWidth;
	unsigned int ThumbHeight;
	unsigned char *ThumbAddrVir;
	unsigned int ThumbLen;

	int Orientation; //value can be 0,90,180,270 degree
	rational_t ExposureTime; //tag 0x829A
	rational_t FNumber; //tag 0x829D
	short ISOSpeed; //tag 0x8827

	srational_t ShutterSpeedValue; //tag 0x9201
	rational_t Aperture; //tag 0x9202
	//srational_t    BrightnessValue;   //tag 0x9203
	srational_t ExposureBiasValue; //tag 0x9204

	rational_t MaxAperture; //tag 0x9205

	short MeteringMode; //tag 0x9207
	short LightSource; //tag 0x9208
	short FlashUsed; //tag 0x9209
	rational_t FocalLength; //tag 0x920A

	rational_t DigitalZoomRatio; // tag 0xA404
	short WhiteBalance; //tag 0xA403
	short ExposureMode; //tag 0xA402

	// gps info
	int enableGpsInfo;
	double gps_latitude;
	double gps_longitude;
	double gps_altitude;
	long gps_timestamp;
	unsigned char gpsProcessingMethod[GPS_PROCESS_METHOD_LENGTH];

	unsigned char CameraSerialNum[128]; //tag 0xA431 (exif 2.3 version)
	short FocalLengthIn35mmFilm; // tag 0xA405

	unsigned char ImageName[128]; //tag 0x010D
	unsigned char ImageDescription[128]; //tag 0x010E
	short ImageWidth; //tag 0xA002
	short ImageHeight; //tag 0xA003

	int thumb_quality;
} EXIFInfo;

typedef struct VencRect {
	int nLeft;
	int nTop;
	int nWidth;
	int nHeight;
} VencRect;

typedef enum VENC_YUV2YUV {
	VENC_YCCToBT601,
	VENC_BT601ToYCC,
} VENC_YUV2YUV;

typedef enum VENC_CODING_MODE {
	VENC_FRAME_CODING = 0,
	VENC_FIELD_CODING = 1,
} VENC_CODING_MODE;

//* The Amount of Temporal SVC Layers
typedef enum {
	NO_T_SVC  = 0,
	T_LAYER_2 = 2,
	T_LAYER_3 = 3,
	T_LAYER_4 = 4
} T_LAYER;

//* The Multiple of Skip_Frame
typedef enum {
	NO_SKIP = 0,
	SKIP_2  = 2,
	SKIP_4  = 4,
	SKIP_8  = 8
} SKIP_FRAME;

typedef enum VENC_CODEC_TYPE {
	VENC_CODEC_H264,
	VENC_CODEC_JPEG,
	VENC_CODEC_H264_VER2,
	VENC_CODEC_H265,
	VENC_CODEC_VP8,
} VENC_CODEC_TYPE;

typedef enum VENC_PIXEL_FMT {
	VENC_PIXEL_YUV420SP,
	VENC_PIXEL_YVU420SP,
	VENC_PIXEL_YUV420P,
	VENC_PIXEL_YVU420P,
	VENC_PIXEL_YUV422SP,
	VENC_PIXEL_YVU422SP,
	VENC_PIXEL_YUV422P,
	VENC_PIXEL_YVU422P,
	VENC_PIXEL_YUYV422,
	VENC_PIXEL_UYVY422,
	VENC_PIXEL_YVYU422, //10
	VENC_PIXEL_VYUY422,
	VENC_PIXEL_ARGB,
	VENC_PIXEL_RGBA,
	VENC_PIXEL_ABGR,
	VENC_PIXEL_BGRA,
	VENC_PIXEL_TILE_32X32,
	VENC_PIXEL_TILE_128X32,
	VENC_PIXEL_AFBC_AW,
	VENC_PIXEL_LBC_AW, //* for v5v200 and newer ic //19
} VENC_PIXEL_FMT;

typedef struct VencThumbInfo {
	unsigned int nThumbSize;
	unsigned char *pThumbBuf;
	unsigned int bWriteToFile;
	struct file *fp;
	unsigned int nEncodeCnt;
} VencThumbInfo;

typedef struct VencBaseConfig {
	unsigned char bEncH264Nalu;
	unsigned int nInputWidth;
	unsigned int nInputHeight;
	unsigned int nDstWidth;
	unsigned int nDstHeight;
	unsigned int nStride;
	VENC_PIXEL_FMT eInputFormat;
	struct mem_interface *memops;
	struct ve_interface *veOpsS;
	void *pVeOpsSelf;

	unsigned char bOnlyWbFlag;

	//* for v5v200 and newer ic
	unsigned char bLbcLossyComEnFlag1_5x;
	unsigned char bLbcLossyComEnFlag2x;
	unsigned char bLbcLossyComEnFlag2_5x;
	unsigned char bIsVbvNoCache;
	//* end

	unsigned int bOnlineMode; //* 1: online mode,    0: offline mode;
	unsigned int bOnlineChannel; //* 1: online channel, 0: offline channel;
	unsigned int nOnlineShareBufNum; //* share buffer num

	//*for debug
	unsigned int extend_flag; //* flag&0x1: printf reg before interrupt
	//* flag&0x2: printf reg after  interrupt
	eVeLbcMode rec_lbc_mode; //*0: disable, 1:1.5x , 2: 2.0x, 3: 2.5x, 4: no_lossy
	//*for debug(end)

	int channel_id;
} VencBaseConfig;

/**
 * H264 profile types
 */
typedef enum VENC_H264PROFILETYPE {
	VENC_H264ProfileBaseline = 66, /**< Baseline profile */
	VENC_H264ProfileMain     = 77, /**< Main profile */
	VENC_H264ProfileHigh     = 100, /**< High profile */
} VENC_H264PROFILETYPE;

/**
 * H264 level types
 */
typedef enum VENC_H264LEVELTYPE {
	VENC_H264Level1       = 10, /**< Level 1 */
	VENC_H264Level11      = 11, /**< Level 1.1 */
	VENC_H264Level12      = 12, /**< Level 1.2 */
	VENC_H264Level13      = 13, /**< Level 1.3 */
	VENC_H264Level2       = 20, /**< Level 2 */
	VENC_H264Level21      = 21, /**< Level 2.1 */
	VENC_H264Level22      = 22, /**< Level 2.2 */
	VENC_H264Level3       = 30, /**< Level 3 */
	VENC_H264Level31      = 31, /**< Level 3.1 */
	VENC_H264Level32      = 32, /**< Level 3.2 */
	VENC_H264Level4       = 40, /**< Level 4 */
	VENC_H264Level41      = 41, /**< Level 4.1 */
	VENC_H264Level42      = 42, /**< Level 4.2 */
	VENC_H264Level5       = 50, /**< Level 5 */
	VENC_H264Level51      = 51, /**< Level 5.1 */
	VENC_H264Level52      = 52, /**< Level 5.2 */
	VENC_H264LevelDefault = 0
} VENC_H264LEVELTYPE;

typedef struct VencH264ProfileLevel {
	VENC_H264PROFILETYPE nProfile;
	VENC_H264LEVELTYPE nLevel;
} VencH264ProfileLevel;

typedef struct VencQPRange {
	int nMaxqp;
	int nMinqp;
	int nMaxPqp;
	int nMinPqp;
	int nQpInit;
	int bEnMbQpLimit;
} VencQPRange;

typedef struct MotionParam {
	int nMotionDetectEnable;
	int nMotionDetectRatio; /* 0~12, advise set 0 */
	int nStaticDetectRatio; /* 0~12, should be larger than  nMotionDetectRatio, advise set 2 */
	int nMaxNumStaticFrame; /* advise set 4 */
	double nStaticBitsRatio; /* advise set 0.2~0.3 at daytime, set 0.1 at night */
	double nMV64x64Ratio; /* advise set 0.01 */
	short nMVXTh; /* advise set 6 */
	short nMVYTh; /* advise set 6 */
} MotionParam;

typedef struct VencHeaderData {
	unsigned char *pBuffer;
	unsigned int nLength;
} VencHeaderData;

/* support 4 ROI region */
typedef struct VencROIConfig {
	int bEnable;
	int index; /* (0~3) */
	int nQPoffset;
	unsigned char roi_abs_flag;
	VencRect sRect;
} VencROIConfig;

typedef struct VencFixQP {
	int bEnable;
	int nIQp;
	int nPQp;
} VencFixQP;

typedef struct VencCopyROIConfig {
	int bEnable;
	int num; /* (0~16) */
	VencRect sRect[16];
	unsigned char *pRoiYAddrVir;
	unsigned char *pRoiCAddrVir;
	unsigned long pRoiYAddrPhy;
	unsigned long pRoiCAddrPhy;
} VencCopyROIConfig;

typedef enum {
	AW_CBR   = 0,
	AW_VBR   = 1,
	AW_AVBR  = 2,
	AW_QPMAP = 3,
	AW_FIXQP = 4,
} VENC_RC_MODE;

typedef enum VENC_VIDEO_GOP_MODE {
	AW_NORMALP	= 1, //one p ref frame
	AW_ADVANCE_SINGLE = 2,
	AW_DOUBLEP	= 3,
	AW_SPECIAL_DOUBLE = 4,
	AW_SPECIAL_SMARTP = 5, //double p ref frames and use virtual i frame,but virtual i ref virtual i
	AW_SMARTP	 = 6, //double p ref frames and use virtual i frame
} VENC_VIDEO_GOP_MODE;

typedef struct VencInputBuffer {
	unsigned long nID;
	long long nPts;
	unsigned int nFlag;
	unsigned char *pAddrPhyY;
	unsigned char *pAddrPhyC;
	unsigned char *pAddrVirY;
	unsigned char *pAddrVirC;
	int bEnableCorp;
	VencRect sCropInfo;

	int ispPicVar;
	int ispPicVarChroma; //chroma  filter  coef[0-63],  from isp
	int bUseInputBufferRoi;
	VencROIConfig roi_param[8];
	int bAllocMemSelf;
	int nShareBufFd;
	unsigned char bUseCsiColorFormat;
	VENC_PIXEL_FMT eCsiColorFormat;

	int envLV;
	int bNeedFlushCache;
} VencInputBuffer;

typedef struct FrameInfo {
	int CurrQp;
	int avQp;
	int nGopIndex;
	int nFrameIndex;
	int nTotalIndex;
} FrameInfo;

typedef struct VeProcSet {
	unsigned char bProcEnable;
	unsigned int nProcFreq;
	unsigned int nStatisBitRateTime;
	unsigned int nStatisFrRateTime;
} VeProcSet;

typedef struct {
    long long mb_start;
    long long mb_end;
    long long mb_time;
    long long mv_start;
    long long mv_end;
    long long mv_time;
    long long bin_start;
    long long bin_end;
    long long bin_time;
    long long r3d_start;
    long long r3d_end;
    long long r3d_time;
    long long int_start;
    long long int_end;
    long long int_time;
    long long sum_start;
    long long sum_end;
    long long sum_time;
    long long avg_int_time;
} StatisticTime;

typedef struct {
    float nSceneCoef[3];
    float nMoveCoef[5];
} VencIPTargetBitsRatio;

typedef struct VencOutputBuffer {
	int nID;
	long long nPts;
	unsigned int nFlag;
	unsigned int nSize0;
	unsigned int nSize1;
	unsigned char *pData0;
	unsigned char *pData1;

	FrameInfo frame_info;
	unsigned int nSize2;
	unsigned char *pData2;
} VencOutputBuffer;

typedef struct VencAllocateBufferParam {
	unsigned int nSizeY;
	unsigned int nSizeC;
} VencAllocateBufferParam;

#define EXTENDED_SAR 255
typedef struct VencH264AspectRatio {
	unsigned char aspect_ratio_idc;
	unsigned short sar_width;
	unsigned short sar_height;
} VencH264AspectRatio;

typedef struct VencSaveBSFile {
	char filename[256];
	unsigned char save_bsfile_flag;
	unsigned int save_start_time;
	unsigned int save_end_time;
} VencSaveBSFile;

typedef enum VENC_COLOR_SPACE {
	RESERVED0       = 0,
	VENC_BT709      = 1, // include BT709-5, BT1361, IEC61966-2-1 IEC61966-2-4
	RESERVED1       = 2,
	RESERVED2       = 3,
	VENC_BT470      = 4, // include BT470-6_SystemM,
	VENC_BT601      = 5, // include BT470-6_SystemB, BT601-6_625, BT1358_625, BT1700_625
	VENC_SMPTE_170M = 6, // include SMPTE_170M, BT601-6_525, BT1358_525, BT1700_NTSC
	VENC_SMPTE_240M = 7, // include SMPTE_240M
	VENC_YCC	= 8, // include GENERIC_FILM
	VENC_BT2020     = 9, // include BT2020
	VENC_ST_428     = 10, // include SMPTE_428-1
} VENC_COLOR_SPACE;

typedef enum VENC_VIDEO_FORMAT {
	COMPONENT = 0,
	PAL       = 1,
	NTSC      = 2,
	SECAM     = 3,
	MAC       = 4,
	DEFAULT   = 5,
} VENC_VIDEO_FORMAT;

typedef struct VencJpegVideoSignal {
	VENC_COLOR_SPACE src_colour_primaries;
	VENC_COLOR_SPACE dst_colour_primaries;
} VencJpegVideoSignal;

typedef struct VencH264VideoSignal {
	VENC_VIDEO_FORMAT video_format;

	unsigned char full_range_flag;

	VENC_COLOR_SPACE src_colour_primaries;
	VENC_COLOR_SPACE dst_colour_primaries;
} VencH264VideoSignal;

typedef struct VencH264VideoTiming {
	unsigned long num_units_in_tick;
	unsigned long time_scale;
	unsigned int fixed_frame_rate_flag;

} VencH264VideoTiming;

// Add for setting SVC and Skip_Frame
typedef struct VencH264SVCSkip {
	T_LAYER nTemporalSVC;
	SKIP_FRAME nSkipFrame;
	int bEnableLayerRatio;
	unsigned int nLayerRatio[4];
} VencH264SVCSkip;

typedef struct VencCyclicIntraRefresh {
	int bEnable;
	int nBlockNumber;
} VencCyclicIntraRefresh;

typedef struct VencSize {
	int nWidth;
	int nHeight;
} VencSize;

typedef struct VencAdvancedRefParam {
	unsigned char bAdvancedRefEn; //advanced ref frame mode, 0:not use , 1:use
	unsigned int nBase; //base frame num
	unsigned int nEnhance; //enhance frame num
	unsigned char bRefBaseEn; //ctrl base frame ref base frame, 0:enable, 1:disable
} VencAdvancedRefParam;

typedef struct VencGopParam {
	unsigned char bUseGopCtrlEn; //use user set gop mode
	VENC_VIDEO_GOP_MODE eGopMode; //gop mode
	unsigned int nVirtualIFrameInterval;
	unsigned int nSpInterval; //user set special p frame ref interval
	VencAdvancedRefParam sRefParam; //user set advanced ref frame mode
} VencGopParam;

typedef struct VencCheckColorFormat {
	int index;
	VENC_PIXEL_FMT eColorFormat;
} VencCheckColorFormat;

typedef struct VencVP8Param {
	int nFramerate; /* fps*/
	int nBitrate; /* bps*/
	int nMaxKeyInterval;
} VencVP8Param;

typedef enum VENC_SUPERFRAME_MODE {
	VENC_SUPERFRAME_NONE,
	VENC_SUPERFRAME_DISCARD,
	VENC_SUPERFRAME_REENCODE,
} VENC_SUPERFRAME_MODE;

typedef struct VencSuperFrameConfig {
	VENC_SUPERFRAME_MODE    eSuperFrameMode;
	unsigned int            nMaxIFrameBits;
	unsigned int            nMaxPFrameBits;
    int                     nMaxRencodeTimes;
    float                   nMaxP2IFrameBitsRatio;
} VencSuperFrameConfig;

typedef struct VencBitRateRange {
	int bitRateMax;
	int bitRateMin;
} VencBitRateRange;

typedef struct VencRoiBgFrameRate {
	int nSrcFrameRate;
	int nDstFrameRate;
} VencRoiBgFrameRate;

typedef struct VencAlterFrameRateInfo {
	unsigned char bEnable;
	unsigned char bUseUserSetRoiInfo; //0:use csi roi info; 1:use user set roi info
	VencRoiBgFrameRate sRoiBgFrameRate;
	VencROIConfig roi_param[8];
} VencAlterFrameRateInfo;

typedef struct VencH265TranS {
	/*** unsigned char       transquant_bypass_enabled_flag; not support ***/
	//0:disable transform skip; 1:enable transform skip
	unsigned char transform_skip_enabled_flag;
	//chroma_qp= sliece_qp+chroma_qp_offset
	int chroma_qp_offset;
} VencH265TranS;

typedef struct VencH265SaoS {
	//0:disable luma sao filter; 1:enable luma sao filter
	unsigned char slice_sao_luma_flag;
	//0:disable chroma sao filter; 1:enable chroma sao filter
	unsigned char slice_sao_chroma_flag;
} VencH265SaoS;

typedef struct VencH265DblkS {
	//0:enable deblock filter; 1:disable deblock filter
	unsigned char slice_deblocking_filter_disabled_flag;
	char slice_beta_offset_div2; //range: [-6,6]
	char slice_tc_offset_div2; //range: [-6,6]
} VencH265DblkS;

typedef struct VencH265TimingS {
	//0:stream without timing info; 1:stream with timing info
	unsigned char timing_info_present_flag;
	unsigned int num_units_in_tick; //time_scale/frameRate
	unsigned int time_scale; //1second is  average divided by time_scale
	unsigned int num_ticks_poc_diff_one; //num ticks of diff frame
} VencH265TimingS;

typedef enum VENC_OVERLAY_ARGB_TYPE {
	VENC_OVERLAY_ARGB_MIN = -1,
	VENC_OVERLAY_ARGB8888 = 0,
	VENC_OVERLAY_ARGB4444 = 1,
	VENC_OVERLAY_ARGB1555 = 2,
	VENC_OVERLAY_ARGB_MAX = 3,
} VENC_OVERLAY_ARGB_TYPE;

typedef enum VENC_OVERLAY_TYPE {
	NORMAL_OVERLAY       = 0, //normal overlay
	COVER_OVERLAY	= 1, //use the setting yuv to cover region
	LUMA_REVERSE_OVERLAY = 2, //normal overlay and luma reverse
} VENC_OVERLAY_TYPE;

typedef struct VencOverlayCoverYuvS {
	unsigned char use_cover_yuv_flag; //1:use the cover yuv; 0:transform the argb data to yuv ,then cover
	unsigned char cover_y; //the value of cover y
	unsigned char cover_u; //the value of cover u
	unsigned char cover_v; //the value of cover v
} VencOverlayCoverYuvS;

typedef struct VencOverlayHeaderS {
	unsigned short start_mb_x; //horizonal value of  start points divided by 16
	unsigned short end_mb_x; //horizonal value of  end points divided by 16
	unsigned short start_mb_y; //vertical value of  start points divided by 16
	unsigned short end_mb_y; //vertical value of  end points divided by 16
	unsigned char extra_alpha_flag; //0:no use extra_alpha; 1:use extra_alpha
	unsigned char extra_alpha; //use user set extra_alpha, range is [0, 15]
	VencOverlayCoverYuvS cover_yuv; //when use COVER_OVERLAY should set the cover yuv
	VENC_OVERLAY_TYPE overlay_type; //reference define of VENC_OVERLAY_TYPE
	unsigned char *overlay_blk_addr; //the vir addr of overlay block
	unsigned int bitmap_size; //the size of bitmap

	//* for v5v200 and newer ic
	unsigned int bforce_reverse_flag;
	unsigned int reverse_unit_mb_w_minus1;
	unsigned int reverse_unit_mb_h_minus1;
	//* end

} VencOverlayHeaderS;

typedef struct VencOverlayInfoS {
	unsigned char blk_num; //num of overlay region
	VENC_OVERLAY_ARGB_TYPE argb_type; //reference define of VENC_ARGB_TYPE
	VencOverlayHeaderS overlayHeaderList[MAX_OVERLAY_SIZE];

	//* for v5v200 and newer ic
	unsigned int invert_mode;
	unsigned int invert_threshold;
	//* end
	unsigned int is_user_buf_flag; //* 1: should use copy_from_user to copy bitmap data
} VencOverlayInfoS;

typedef struct VencBrightnessS {
	unsigned int dark_th; //dark threshold, default 60, range[0, 255]
	unsigned int bright_th; //bright threshold, default 200, range[0, 255]
} VencBrightnessS;

typedef struct VencEncodeTimeS {
	unsigned int frame_num; //current frame num
	unsigned int curr_enc_time; //current frame encoder time
	unsigned int curr_empty_time; //the time between current frame and last frame
	unsigned int avr_enc_time; //average encoder time
	unsigned int avr_empty_time; //average empty time
	unsigned int max_enc_time;
	unsigned int max_enc_time_frame_num;
	unsigned int max_empty_time;
	unsigned int max_empty_time_frame_num;
} VencEncodeTimeS;

typedef struct VencH264ConstraintFlag {
	unsigned char                 reserve_zero : 2;
	unsigned char                 constraint_5 : 1;
	unsigned char                 constraint_4 : 1;
	unsigned char                 constraint_3 : 1;
	unsigned char                 constraint_2 : 1;
	unsigned char                 constraint_1 : 1;
	unsigned char                 constraint_0 : 1;
} VencH264ConstraintFlag;

typedef struct VeProcEncH265Info {
	float Lambda;
	float LambdaC;
	float LambdaSqrt;

	unsigned int uIntraCoef0;
	unsigned int uIntraCoef1;
	unsigned int uIntraCoef2;
	unsigned int uIntraTh32;
	unsigned int uIntraTh16;
	unsigned int uIntraTh8;

	unsigned int uInterTend;
	unsigned int uSkipTend;
	unsigned int uMergeTend;

	unsigned char fast_intra_en;
	unsigned char adaptitv_tu_split_en;
	unsigned int combine_4x4_md_th;
} VeProcEncH265Info;

typedef struct VeProcEncInfo {
	unsigned int                nChannelNum;

	unsigned char               bEnEncppSharp;

	unsigned int                bOnlineMode;
	unsigned int                bOnlineChannel;
	unsigned int                nOnlineShareBufNum;

	unsigned int                nCsiOverwriteFrmNum;

	int                         nInputFormat;

	int                         nRecLbcMode;

	unsigned int                nInputWidth;
	unsigned int                nInputHeight;
	unsigned int                nDstWidth;
	unsigned int                nDstHeight;
	unsigned int                nStride;

	unsigned int                rot_angle;

	int                         crop_left;
	int                         crop_top;
	int                         crop_width;
	int                         crop_height;

	unsigned int                nProfileIdc;
	unsigned int                nLevelIdc;

	int                         nBitRate;
	int                         nFrameRate;

	VENC_RC_MODE                eRcMode;
	VENC_VIDEO_GOP_MODE         eGopMode;
	int                         nIDRItl;
	int                         nProductMode;
	int                         nColourMode;

	VencFixQP                   fix_qp;

	int                         i_qp_max;
	int                         i_qp_min;
	int                         p_qp_max;
	int                         p_qp_min;
	int                         nInitQp;
	int                         en_mb_qp_limit;

	VencROIConfig               sRoi[8];

	unsigned int                avr_bit_rate;
	unsigned int                real_bit_rate;
	unsigned int                avr_frame_rate;
	unsigned int                real_frame_rate;

	int                         vbv_size;
	int                         UnusedBufferSize;
	int                         nValidFrameNum;
	int                         nValidDataSize;

	int                         eSliceType;
	int                         nCurrQp;
	int                         nFrameIndex;
	int                         nTotalIndex;

	int                         nRealBits;
	int                         nTargetBits;

	StatisticTime               sStaTime;

	/******************Advanced Parameters******************/
	unsigned char               bIntra4x4;
	unsigned char               bIntraInP;
    unsigned char               bSmallSearchRange;

	unsigned char               bD3DInIFrm;
	unsigned char               bTightMbQp;
	unsigned char               bExtremeD3D;

	unsigned char               f2d_en;
	unsigned int                f2d_strength_uv;
	unsigned int                f2d_strength_y;
	unsigned int                f2d_th_uv;
	unsigned int                f2d_th_y;

	unsigned char               f3d_en;
	unsigned int                f3d_max_mv_th;
	unsigned int                f3d_max_mad_th;
	unsigned char               f3d_pix_level_en;
	unsigned int                f3d_max_pix_diff_th;
	unsigned char               f3d_smooth_en;
	unsigned int                f3d_min_coef;
	unsigned int                f3d_max_coef;

	VencRegionD3DParam          sRegionD3DParam;

	unsigned char               mv_tu_split_en;
	unsigned char               mv_amp_en;
	int                         mv_lambda_offset;
	unsigned char               pmv_en;
	unsigned char               cpmv_case5_en;
	unsigned char               cpmv_case4_en;
	unsigned char               cpmv_case3_en;
	unsigned int                cpmv_case3_th;
	unsigned char               cpmv_case2_en;
	unsigned int                cpmv_case2_th;
	unsigned char               cpmv_case1_en;
	unsigned int                cpmv_case1_th;
	unsigned char               mv_denoise_en;
	unsigned int                noise_estimate;

	VencIPTargetBitsRatio       sBitsRatio;
	VencTargetBitsClipParam     sBitsClipParam;

	float                       WeakTextTh;
	unsigned char               MbRcEn;
	int                         EnIFrmMbRcMoveStatus;
	unsigned char               MadEn;
	float                       MadHistogram[12][11];
	unsigned char               ClassifyTh[12];
	unsigned char               MdEn;
	float                       MdHistogram[16][11];
	int                         MdQp[16];
	unsigned int                MdLevel[16];

	unsigned int                uMaxBitRate;
	int                         nQuality;
	int                         nIFrmBitsCoef;
	int                         nPFrmBitsCoef;

	unsigned int                uSceneStatus;
	unsigned int                uMoveStatus;
	unsigned int                uMovingLevel;
	float                       BinImgRatio;

	int                         nEnvLv;
	int                         nAeWeightLum;

	unsigned char               en_camera_move;
	unsigned char               lens_moving;

	VencVe2IspD2DLimit          ve2isp_d2d;
	int                         isp_d2d_level;
	int                         isp_d3d_level;

	int                         bIsOverflow;
	int                         nIspScale;

	int                         nSuperFrameMode;
	int                         nSuperMaxIBytes;
	int                         nSuperMaxPBytes;
	int                         nSuperMaxTimes;
	float                       nSuperP2IBitsRatio;
	int                         nSuperTotalTimes;

	VeProcEncH265Info           sH265Info;
} VeProcEncInfo;

typedef enum VENC_INDEXTYPE {
	VENC_IndexParamBitrate = 0x0,
	/**< reference type: int */
	VENC_IndexParamFramerate,
	/**< reference type: int */
	VENC_IndexParamMaxKeyInterval,
	/**< reference type: int */
	VENC_IndexParamIfilter,
	/**< reference type: int */
	VENC_IndexParamRotation,
	/**< reference type: int */
	VENC_IndexParamSliceHeight,
	/**< reference type: int */
	VENC_IndexParamForceKeyFrame,
	/**< reference type: int (write only)*/
	VENC_IndexParamMotionDetectEnable,
	/**< reference type: MotionParam(write only) */
	VENC_IndexParamMotionDetectStatus,
	/**< reference type: int(read only) */
	VENC_IndexParamRgb2Yuv,
	/**< reference type: VENC_COLOR_SPACE */
	VENC_IndexParamYuv2Yuv,
	/**< reference type: VENC_YUV2YUV */
	VENC_IndexParamROIConfig,
	/**< reference type: VencROIConfig */
	VENC_IndexParamStride,
	/**< reference type: int */
	VENC_IndexParamColorFormat,
	/**< reference type: VENC_PIXEL_FMT */
	VENC_IndexParamSize,
	/**< reference type: VencSize(read only) */
	VENC_IndexParamSetVbvSize,
	/**< reference type: setVbvSize(write only) */
	VENC_IndexParamVbvInfo,
	/**< reference type: getVbvInfo(read only) */
	VENC_IndexParamSuperFrameConfig,
	/**< reference type: VencSuperFrameConfig */
	VENC_IndexParamSetPSkip,
	/**< reference type: unsigned int */
	VENC_IndexParamResetEnc,
	/**< reference type: */
	VENC_IndexParamSaveBSFile,
	/**< reference type: VencSaveBSFile */
	VENC_IndexParamHorizonFlip,
	/**< reference type: unsigned int */

	/* check capabiliy */
	VENC_IndexParamMAXSupportSize,
	/**< reference type: VencSize(read only) */
	VENC_IndexParamCheckColorFormat,
	/**< reference type: VencCheckFormat(read only) */

	/* H264 param */
	VENC_IndexParamH264Param = 0x100,
	/**< reference type: VencH264Param */
	VENC_IndexParamH264SPSPPS,
	/**< reference type: VencHeaderData (read only)*/
	VENC_IndexParamH264QPRange,
	/**< reference type: VencQPRange */
	VENC_IndexParamH264ProfileLevel,
	/**< reference type: VencProfileLevel */
	VENC_IndexParamH264EntropyCodingCABAC,
	/**< reference type: int(0:CAVLC 1:CABAC) */
	VENC_IndexParamH264CyclicIntraRefresh,
	/**< reference type: VencCyclicIntraRefresh */
	VENC_IndexParamH264FixQP,
	/**< reference type: VencFixQP */
	VENC_IndexParamH264SVCSkip,
	/**< reference type: VencH264SVCSkip */
	VENC_IndexParamH264AspectRatio,
	/**< reference type: VencH264AspectRatio */
	VENC_IndexParamFastEnc,
	/**< reference type: int */
	VENC_IndexParamH264VideoSignal,
	/**< reference type: VencH264VideoSignal */
	VENC_IndexParamH264VideoTiming,
	/**< reference type: VencH264VideoTiming */
	VENC_IndexParamChmoraGray,
	/**< reference type: unsigned char */
	VENC_IndexParamIQpOffset,
	/**< reference type: int */
	/* jpeg param */
	VENC_IndexParamJpegQuality = 0x200,
	/**< reference type: int (1~100) */
	VENC_IndexParamJpegExifInfo,
	/**< reference type: EXIFInfo */
	VENC_IndexParamJpegEncMode,
	/**< reference type: 0:jpeg; 1:motion_jepg */
	VENC_IndexParamJpegVideoSignal,
	/**< reference type: VencJpegVideoSignal */

	/* VP8 param */
	VENC_IndexParamVP8Param,
	/* max one frame length */
	VENC_IndexParamSetFrameLenThreshold,
	/**< reference type: int */
	/* decrease the a20 dram bands */
	VENC_IndexParamSetA20LowBands,
	/**< reference type: 0:disable; 1:enable */
	VENC_IndexParamSetBitRateRange,
	/**< reference type: VencBitRateRange */
	VENC_IndexParamLongTermReference,
	/**< reference type: 0:disable; 1:enable, default:enable */

	/* h265 param */
	VENC_IndexParamH265Param = 0x300,
	VENC_IndexParamH265Gop,
	VENC_IndexParamH265ToalFramesNum,
	VENC_IndexParamH26xUpdateLTRef,
	VENC_IndexParamH265Header,
	VENC_IndexParamH265TendRatioCoef,
	VENC_IndexParamH265Trans,
	/**< reference type: VencH265TranS */
	VENC_IndexParamH265Sao,
	/**< reference type: VencH265SaoS */
	VENC_IndexParamH265Dblk,
	/**< reference type: VencH265DblkS */
	VENC_IndexParamH265Timing,
	/**< reference type: VencH265TimingS */
	VENC_IndexParamIntraPeriod,
	VENC_IndexParamMBModeCtrl,
	VENC_IndexParamMBSumInfoOutput,
	VENC_IndexParamMBInfoOutput,
	VENC_IndexParamVUIAspectRatio,
	VENC_IndexParamVUIVideoSignal,
	VENC_IndexParamVUIChromaLoc,
	VENC_IndexParamVUIDisplayWindow,
	VENC_IndexParamVUIBitstreamRestriction,

	VENC_IndexParamAlterFrame = 0x400,
	/**< reference type: unsigned int */
	VENC_IndexParamVirtualIFrame,
	VENC_IndexParamChannelNum,
	VENC_IndexParamProcSet,
	/**< reference type: VencOverlayInfoS */
	VENC_IndexParamSetOverlay,
	/**< reference type: unsigned char */
	VENC_IndexParamAllParams,
	/**< reference type:VencBrightnessS */
	VENC_IndexParamBright,
	/**< reference type:VencSmartFun */
	VENC_IndexParamSmartFuntion,
	/**< reference type: VencHVS */
	VENC_IndexParamHVS,
	/**< reference type: unsigned char */
	VENC_IndexParamSkipTend,
	/**< reference type: unsigned char */
	VENC_IndexParamHighPassFilter,
	/**< reference type: unsigned char */
	VENC_IndexParamPFrameIntraEn,
	/**< reference type: unsigned char */
	VENC_IndexParamEncodeTimeEn,
	/**< reference type: VencEncodeTimeS */
	VENC_IndexParamGetEncodeTime,
	/**< reference type: s2DfilterParam */
	VENC_IndexParam2DFilter,
	/**< reference type: unsigned char */
	VENC_IndexParam3DFilter,
	/**< reference type: s3DfilterParam */
	VENC_IndexParam3DFilterNew,
	/**< reference type: unsigned char */
	VENC_IndexParamIntra4x4En,

	/**< reference type: unsigned int */
	VENC_IndexParamSetNullFrame = 0x500,
	/**< reference type: VencThumbInfo */
	VENC_IndexParamGetThumbYUV,
	/**< reference type: eVencEncppScalerRatio */
	//VENC_IndexParamThumbScaler,
	/**< reference type: unsigned char */
	VENC_IndexParamAdaptiveIntraInP,
	/**< reference type: VencBaseConfig */
	VENC_IndexParamUpdateBaseInfo,

	/**< reference type: unsigned char */
	VENC_IndexParamFillingCbr,

	/**< reference type: unsigned char */
	VENC_IndexParamRoi,

	/**< reference type: unsigned int */
	/* not encode some frame, such as: not encode the begining 10 frames */
	VENC_IndexParamDropFrame,

	/**< reference type: unsigned int */
	/* drop the frame that bitstreamLen exceed vbv-valid-size */
	VENC_IndexParamDropOverflowFrame,

	/**< reference type: unsigned int; 0: day, 1: night*/
	VENC_IndexParamIsNightCaseFlag,

	/**< reference type: unsigned int; 0: normal case, 1: ipc case*/
	VENC_IndexParamProductCase,

	/**< reference type: unsigned int; 0: sp2305 1: c2398*/
	VENC_IndexParamSensorType,

	/**< reference type: VencEnvLvRange */
	VENC_IndexParamSetEnvLvTh,
	/**< reference type: VencVbrParam */
	VENC_IndexParamSetVbrParam,

	/**< reference type: VencIPTargetBitsRatio */
	VENC_IndexParamIPTargetBitsRatio,

	/**< reference type: Set or Get VencMotionSearchParam* */
	VENC_IndexParamMotionSearchParam,

	/**< reference type: Get VencMotionSearchRegin* */
	VENC_IndexParamMotionSearchResult,

	/**< reference type: unsigned int; 0: have enought vbv buf, 1: vbv buf is full*/
	VENC_IndexParamBSbufIsFull,

	/**< reference type: sGdcParam*/
	VENC_IndexParamGdcConfig,

	/**< reference type: sSharpParam*/
	VENC_IndexParamSharpConfig,

	/**< reference type: unsigned int; 0: disbale, 1: enable*/
	VENC_IndexParamEnableWbYuv,

	/**< reference type: unsigned int; 0: disbale, 1: enable*/
	VENC_IndexParamEnableEncppSharp,

	/**< reference type: unsigned int; 0: disbale, 1: enable*/
	VENC_IndexParamEnableCheckOnlineStatus,

	/**< reference type: sIspMotionParam */
	VENC_IndexParamIspMotionParam,

	/**< reference type: VencVe2IspParam */
	VENC_IndexParamVe2IspParam,

	/**< reference type: int */
	VENC_IndexParamEnvLv,

	/**< reference type: int */
	VENC_IndexParamAeWeightLum,

	/**< reference type: eCameraStatus */
	VENC_IndexParamEnCameraMove,

	/**< reference type: VencTargetBitsClipParam */
	VENC_IndexParamTargetBitsClipParam,

	/**< reference type: int */
	VENC_IndexParamEnIFrmMbRcMoveStatus,

	/**< reference type: sIspAeStatus */
	VENC_IndexParamIspAeStatus,

	/* set rec_lbc_mode */
	VENC_IndexParamSetRecRefLbcMode,

	VENC_IndexParamGetVbvShareFd,

	VENC_IndexParamEnableRecRefBufReduceFunc,

	/**< reference type: float [0,100]*/
	VENC_IndexParamWeakTextTh,

	/**< reference type: int [0,1]*/
	VENC_IndexParamEnD3DInIFrm,

	/**< reference type: int [0,1]*/
	VENC_IndexParamEnTightMbQp,

	/**< reference type: VencExtremeD3DParam */
	VENC_IndexParamSetExtremeD3D,

	/**< reference type: Set or Get VencRegionD3DParam* */
	VENC_IndexParamRegionD3DParam,

	/**< reference type: Get VencRegionD3DResult* */
	VENC_IndexParamRegionD3DResult,

	/**< reference type: int */
	VENC_IndexParamChromaQPOffset,

	/**< reference type:  VencH264ConstraintFlag */
	VENC_IndexParamH264ConstraintFlag,

	/**< reference type: VencVe2IspD2DLimit */
	VENC_IndexParamVe2IspD2DLimit
} VENC_INDEXTYPE;
typedef struct VencEnvLvRange {
	int env_lv_high_th;
	int env_lv_low_th;
	int env_lv_coef; //range[0,15]
} VencEnvLvRange;

typedef enum VENC_RESULT_TYPE {
	VENC_RESULT_ERROR	      = -1,
	VENC_RESULT_OK		       = 0,
	VENC_RESULT_NO_FRAME_BUFFER    = 1,
	VENC_RESULT_BITSTREAM_IS_FULL  = 2,
	VENC_RESULT_ILLEGAL_PARAM      = 3,
	VENC_RESULT_NOT_SUPPORT	= 4,
	VENC_RESULT_BITSTREAM_IS_EMPTY = 5,
	VENC_RESULT_NO_MEMORY	  = 6,
	VENC_RESULT_NO_RESOURCE	= 7,
	VENC_RESULT_NULL_PTR	   = 8,
	VENC_RESULT_DROP_FRAME	 = 9,
	VENC_RESULT_CONTINUE	   = 10,

	VENC_RESULT_EFUSE_ERROR = 25,
} VENC_RESULT_TYPE;

typedef enum {
	H265_B_SLICE   = 0x0,
	H265_P_SLICE   = 0x1,
	H265_I_SLICE   = 0x2,
	H265_IDR_SLICE = 0x12
} VENC_H265_CODE_TYPE;

typedef struct JpegEncInfo {
	VencBaseConfig sBaseInfo;
	int bNoUseAddrPhy;
	unsigned char *pAddrPhyY;
	unsigned char *pAddrPhyC;
	unsigned char *pAddrVirY;
	unsigned char *pAddrVirC;
	int bEnableCorp;
	VencRect sCropInfo;
	int quality;
	int nShareBufFd;
} JpegEncInfo;

typedef struct VbvInfo {
	unsigned int vbv_size;
	unsigned int coded_frame_num;
	unsigned int coded_size;
	unsigned int maxFrameLen;
	unsigned char *start_addr;
} VbvInfo;

typedef enum {
	VENC_H265ProfileMain      = 1,
	VENC_H265ProfileMain10    = 2,
	VENC_H265ProfileMainStill = 3
} VENC_H265PROFILETYPE;

typedef enum {
	VENC_H265Level1       = 30, /**< Level 1 */
	VENC_H265Level2       = 60, /**< Level 2 */
	VENC_H265Level21      = 63, /**< Level 2.1 */
	VENC_H265Level3       = 90, /**< Level 3 */
	VENC_H265Level31      = 93, /**< Level 3.1 */
	VENC_H265Level4       = 120, /**< Level 4 */
	VENC_H265Level41      = 123, /**< Level 4.1 */
	VENC_H265Level5       = 150, /**< Level 5 */
	VENC_H265Level51      = 153, /**< Level 5.1 */
	VENC_H265Level52      = 156, /**< Level 5.2 */
	VENC_H265Level6       = 180, /**< Level 6 */
	VENC_H265Level61      = 183, /**< Level 6.1 */
	VENC_H265Level62      = 186, /**< Level 6.2 */
	VENC_H265LevelDefault = 0
} VENC_H265LEVELTYPE;

typedef enum {
	REF_IDC_DISCARD     = 0,
	REF_IDC_CURRENT_USE = 1,
	REF_IDC_FUTURE_USE  = 2,
	REF_IDC_LONG_TERM   = 4,
	REF_IDC_CURRENT_REF = 8,
} ReferenceIdc;

typedef enum {
	VENC_ST_DIS_WDR = 0,
	VENC_ST_EN_WDR  = 1,
	VENC_ST_NONE
} eSensorType;

typedef struct {
	VENC_H265PROFILETYPE nProfile;
	VENC_H265LEVELTYPE nLevel;
} VencH265ProfileLevel;

typedef struct {
	unsigned int uMaxBitRate;
	unsigned int nMovingTh; //range[1,31], 1:all frames are moving,
	//            31:have no moving frame, default: 20
	int nQuality; //range[1,20], 1:worst quality, 20:best quality
	int nIFrmBitsCoef; //range[1, 20], 1:worst quality, 20:best quality
	int nPFrmBitsCoef; //range[1, 50], 1:worst quality, 50:best quality
} VencVbrParam;

typedef struct {
	unsigned char mode_ctrl_en;
	unsigned char *p_map_info;
} VencMBModeCtrl;

typedef struct {
	VENC_RC_MODE eRcMode;
	unsigned char bLowBitrateBeginFlag;
	unsigned char bUseSetMadThrdFlag;
	unsigned char uMadThrdI[12]; //range 0-255
	unsigned char uMadThrdP[12]; //range 0-255
	unsigned char uMadThrdB[12]; //no support
	unsigned int uStatTime; //range [1,10], default:1
	unsigned int uMinIQp;
	int nMaxReEncodeTimes; //default use one time

	VencVbrParam sVbrParam; //valid only at AW_VBR/AW_AVBR
	VencFixQP sFixQp; //valid only at AW_FIXQP
	VencMBModeCtrl sQpMap; //valid only at AW_QPMAP

	unsigned int uRowQpDelta; //no support
	unsigned int uDirectionThrd; //no support
	unsigned int uQpDeltaLevelI; //no support
	unsigned int uQpDeltaLevelP; //no support
	unsigned int uQpDeltaLevelB; //no support
	unsigned int uInputFrmRate; //no support
	unsigned int uOutputFrmRate; //no support
	unsigned int uFluctuateLevel; //no support
	unsigned int uMinIprop; //no support
	unsigned int uMaxIprop; //no support
} VencRcParam;

typedef struct VencH264Param {
	VencH264ProfileLevel sProfileLevel;
	int bEntropyCodingCABAC; /* 0:CAVLC 1:CABAC*/
	VencQPRange sQPRange;
	int nFramerate; /* fps*/
	int nSrcFramerate; /* fps*/
	int nBitrate; /* bps*/
	int nMaxKeyInterval;
	VENC_CODING_MODE nCodingMode;
	VencGopParam sGopParam;
	VencRcParam sRcParam;
	int breduce_refrecmem;
} VencH264Param;

typedef struct {
	int idr_period;
	VencH265ProfileLevel sProfileLevel;
	VencQPRange sQPRange;
	int nFramerate; /* fps*/
	int nSrcFramerate; /* fps*/
	int nBitrate; /* bps*/
	int nIntraPeriod;
	int nGopSize;
	int nQPInit; /* qp of first IDR_frame if use rate control */
	VencRcParam sRcParam;
	VencGopParam sGopParam;
	int breduce_refrecmem;
} VencH265Param;

typedef struct {
	unsigned int slice_type;
	int poc; // dispaly order of the frame within a GOP, ranging from 1 to gop_size
	int qp_offset;
	int tc_offset_div2; // offset of LoopFilterTcOffsetDiv2, ranging from -6 to 6
	int beta_offset_div2; // offset of LoopFilterTcOffsetDiv2, ranging from -6 to 6

	unsigned int num_ref_pics; // number of ref_frames reserved for cur_frame and future frames
	unsigned int num_ref_pics_active; // number of ref_frames is permited to be used in L0 or L1

	int reference_pics[MAX_FRM_NUM - 1]; // = ref_frame_poc - cur_frame_poc
	// = discard_frame_poc - cur_frame_poc, means derlta_poc of ref_frames which are discarded
	int discard_pics[MAX_FRM_NUM - 1];

	unsigned char lt_ref_flag; // 1: enable cur_frame use long term ref_frame
	int lt_ref_poc; // poc of lt_ref_frame of cur_frame

	// 0 means next 4 member parameters are ignored; 1 means next 3 member parameters are need
	// this parameter of the first frame of a GOP must be 0
	unsigned char predict;

	unsigned int delta_rps_idx; // = cur_frame_encoding_idx - predictor_frame_encoding_idx

	int delta_rps; // = predictor_frame_poc - cur_frame_poc

	// num of ref_idcs to encoder for the current frame, the value is equal to
	// the value of num_st_ref_pics of the predictor_frame + 1 + lt_ref_flag
	unsigned int num_ref_idcs;

	// [][0]=(ref_frame_poc or discard_frame_poc) - cur_frame_poc
	// [][1]indicating the ref_pictures reserved in ref_list_buffer:
	// [][1]=0: will not be a ref_picture anymore
	// [][1]=1: is a ref_picture used by cur_picture
	// [][1]=2: is a ref_picture used by future_picture
	// [][1]=3: is a long term ref_picture
	int reference_idcs[MAX_FRM_NUM][2];
} RefPicSet;
typedef struct {
	int gop_size;
	int intra_period;
	int max_num_ref_pics;
	unsigned char num_ref_idx_l0_default_active;
	unsigned char num_ref_idx_l1_default_active;
	RefPicSet ref_str[MAX_GOP_SIZE + 2]; // just when custom_rps_flag is 1, it should be set
	unsigned char use_sps_rps_flag; // if it is 1, rps will not occur in slice_header
	unsigned char use_lt_ref_flag;
	unsigned char custom_rps_flag; // 0: default ref_str will be use; 1: user should set ref_str[]
} VencH265GopStruct;

#define MAX_NUM_MB (65536)
typedef struct {
	unsigned char mb_mad;
	unsigned char mb_qp;
	unsigned int mb_sse;
	double mb_psnr;
} VencMBInfoPara;

typedef struct {
	unsigned int num_mb;
	VencMBInfoPara *p_para;
} VencMBInfo;

typedef struct {
	// all these average value is for mb16x16
	unsigned int avg_mad;
	unsigned int avg_md;
	unsigned int avg_sse;
	unsigned int avg_qp;
	double avg_psnr;
	unsigned char *p_mb_mad_qp_sse;
	unsigned char *p_mb_bin_img;
	unsigned char *p_mb_mv;
} VencMBSumInfo;

typedef struct {
	unsigned char mb_qp; // {5:0}
	unsigned char mb_skip_flag; // {6}
	unsigned char mb_en; // {7}
} VencMBModeCtrlInfo;

typedef struct {
	unsigned char hp_filter_en;
	unsigned int hp_coef_shift; //* range[0 ~ 7],  default: 3
	unsigned int hp_coef_th; //* range[0 ~ 7],  default: 5
	unsigned int hp_contrast_th; //* range[0 ~ 63], default: 0
	unsigned int hp_mad_th; //* range[0 ~ 63], default: 0
} VencHighPassFilter;

typedef struct {
	unsigned char hvs_en;
	unsigned int th_dir;
	unsigned int th_coef_shift;
} VencHVS;

typedef struct {
	unsigned int inter_tend;
	unsigned int skip_tend;
	unsigned int merge_tend;
} VencH265TendRatioCoef;

typedef struct {
	unsigned char smart_fun_en;
	unsigned char img_bin_en;
	unsigned int img_bin_th;
	unsigned int shift_bits;
} VencSmartFun;

typedef struct {
	unsigned int chroma_sample_top;
	unsigned int chroma_sample_bottom;
} VencVUIChromaLoc;

typedef struct {
	unsigned int win_left_offset;
	unsigned int win_right_offset;
	unsigned int win_top_offset;
	unsigned int win_bottom_offset;
} VencVUIDisplayWindow;

typedef struct {
	unsigned char tiles_fixed_structure_flag;
	unsigned char mv_over_pic_boundaries_flag;
	unsigned char restricted_ref_pic_lists_flag;
	unsigned int min_spatial_seg_idc;
	unsigned int max_bytes_per_pic_denom;
	unsigned int max_bits_per_min_cu_denom;
	unsigned int log2_max_mv_len_hor;
	unsigned int log2_max_mv_len_ver;
} VencVUIBitstreamRestriction;

typedef struct {
	int pix_x_bgn;
	int pix_x_end;
	int pix_y_bgn;
	int pix_y_end;
	int total_num;
	int intra_num;
	int large_mv_num;
	int small_mv_num;
	int zero_mv_num;
	int large_sad_num;
	int is_motion;
} VencMotionSearchRegion;

typedef struct {
	int total_region_num;
	int motion_region_num;
	VencMotionSearchRegion *region;
} VencMotionSearchResult;

typedef struct {
	int en_motion_search;
	int dis_default_para;
	int hor_region_num;
	int ver_region_num;
	int large_mv_th;
	float large_mv_ratio_th;    // include intra and large mv
	float non_zero_mv_ratio_th; // include intra, large mv and samll mv
	float large_sad_ratio_th;
} VencMotionSearchParam;

typedef void *VideoEncoder;
int raise(int signum);
//* new api
VideoEncoder *VencCreate(VENC_CODEC_TYPE eCodecType);
void VencDestroy(VideoEncoder *pEncoder);
int VencInit(VideoEncoder *pEncoder, VencBaseConfig *pConfig);
int VencStart(VideoEncoder *pEncoder);
int VencPause(VideoEncoder *pEncoder);
int VencReset(VideoEncoder *pEncoder);

int VencGetParameter(VideoEncoder *pEncoder, VENC_INDEXTYPE indexType, void *paramData);
int VencSetParameter(VideoEncoder *pEncoder, VENC_INDEXTYPE indexType, void *paramData);

int VencGetValidOutputBufNum(VideoEncoder *pEncoder);
int VencDequeueOutputBuf(VideoEncoder *pEncoder, VencOutputBuffer *pBuffer);
int VencQueueOutputBuf(VideoEncoder *pEncoder, VencOutputBuffer *pBuffer);

int VencGetValidInputBufNum(VideoEncoder *pEncoder);
int VencQueueInputBuf(VideoEncoder *pEncoder, VencInputBuffer *inputbuffer);
int VencAllocateInputBuf(VideoEncoder *pEncoder, VencAllocateBufferParam *pBufferParam, VencInputBuffer *dst_inputBuf);
void VencSetDdrMode(VideoEncoder *pEncoder, int nDdrType);
int VencSetFreq(VideoEncoder *pEncoder, int nVeFreq);

typedef enum {
	VencEvent_FrameFormatNotMatch  = 0, // frame format is not match to initial setting.
	VencEvent_UpdateMbModeInfo     = 1,
	VencEvent_UpdateMbStatInfo     = 2,
	VencEvent_UpdateIspToVeParam     = 3,
	VencEvent_UpdateIspMotionParam = 4,
	VencEvent_UpdateVeToIspParam   = 5,
	VencEvent_Max		       = 0x7FFFFFFF
} VencEventType;

typedef struct {
	int nResult;
	VencInputBuffer *pInputBuffer;
	//other informations about this frame encoding can be added below.

} VencCbInputBufferDoneInfo;

typedef struct {
	/** The EventHandler method is used to notify the application when an
		event of interest occurs.  Events are defined in the VencEventType
		enumeration.  Please see that enumeration for details of what will
		be returned for each type of event. Callbacks should not return
		an error to the component, so if an error occurs, the application
		shall handle it internally.  This is a blocking call.

		The application should return from this call within 5 msec to avoid
		blocking the component for an excessively long period of time.

		@param pAppData
			pointer to an application defined value that was provided by user.
		@param eEvent
			Event that the venclib wants to notify the application about.
		*/

	int (*EventHandler)(
		VideoEncoder *pEncoder,
		void *pAppData,
		VencEventType eEvent,
		unsigned int nData1,
		unsigned int nData2,
		void *pEventData);

	/** The EmptyBufferDone method is used to return emptied buffers to the user for reuse.
		This is a blocking call,
		so the application should not attempt to refill the buffers during this
		call, but should queue them and refill them in another thread.  There
		is no error return, so the application shall handle any errors generated
		internally.

		The application should return from this call within 5 msec.

		@param pAppData
			pointer to an application defined value that was provided by user.
		@param pBufferDoneInfo
			provide input buffer encoding information.
		*/
	int (*InputBufferDone)(
		VideoEncoder *pEncoder,
		void *pAppData,
		VencCbInputBufferDoneInfo *pBufferDoneInfo);

	int (*empty_in_buffer_done)(void *venc_comp, void *frame_info);
} VencCbType;

int VencSetCallbacks(VideoEncoder *pEncoder, VencCbType *pCallbacks, void *pAppData1, void *pAppData2);
int encodeOneFrame(VideoEncoder *pEncoder);

void cdc_log_set_level(unsigned int level);
#endif //_VENCODER_H_

#ifdef __cplusplus
}
#endif /* __cplusplus */

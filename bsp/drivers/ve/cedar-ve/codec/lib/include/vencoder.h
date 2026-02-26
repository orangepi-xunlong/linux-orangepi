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

#ifndef _VENCODER_H_
#define _VENCODER_H_

#ifdef CONFIG_AW_VIDEO_KERNEL_ENC
#else
#include <stdio.h>
#endif

#include "UserKernelAdapter.h"
#include "sc_interface.h"
#include "veInterface.h"

#include <vcodec_base.h>
#include <vencoder_base.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VCS_TOPREG_MMAP_MODE    1
#define VCU_ENC_DEC_MODE        1
#define CSIBK_CH_CNT            2
#define CSIBK_CH_HIST_CNT       8
#define CSIBK_SHARE_BUF         0

#define  DATA_TIME_LENGTH           24
#define  INFO_LENGTH                64
#define  GPS_PROCESS_METHOD_LENGTH  100
#define  DESCRIPTOR_INFO            128
#define  MAX_CHANNEL_NUM            16

#define MAX_FRM_NUM                 5
#define MAX_GOP_SIZE                63
#define MAX_OVERLAY_SIZE            64
#define MAX_RC_GOP_SIZE             256
#define HEVC_MAX_ROI_AREA           8

#define VENCODER_TMP_RATIO (14)

#define VENC_BUFFERFLAG_KEYFRAME 0x00000001
#define VENC_BUFFERFLAG_EOS 0x00000002
#define VENC_BUFFERFLAG_THUMB 0x00000004

#define SHARP_ROI_MAX_NUM (8)

#define JPEG_MAX_SEG_LEN (65523)

#define AE_AVG_ROW  (18)
#define AE_AVG_COL  (24)
#define AE_AVG_NUM  (432)
#define AE_HIST_NUM (256)

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

#define PRINTF_CODE_POS logd("func = %s, line = %d", __FUNCTION__, __LINE__);

typedef enum {
	VENC_FLUSH_INPUT_BUFFER,
	VENC_FLUSH_OUTPUT_BUFFER,
	VENC_FLUSH_IN_AND_OUT_BUFFER,
} VencFlushType;

typedef enum eIspOnlineStatus {
	ISP_ONLINE_STATUS_READY     = 0,
	ISP_ONLINE_STATUS_NOT_READY = 1,
} eIspOnlineStatus;

typedef struct VencIspMotionParam {
	int dis_default_para;
	int large_mv_th;
} VencIspMotionParam;

#define ISP_TABLE_LEN (22)

typedef struct {
	unsigned char is_overflow;
	unsigned short moving_level_table[ISP_TABLE_LEN*ISP_TABLE_LEN];
} MovingLevelInfo;

typedef struct {
	int d2d_level; //[1,1024], 256 means 1X
	int d3d_level; //[1,1024], 256 means 1X
	MovingLevelInfo mMovingLevelInfo;
} VencVe2IspParam;

typedef struct {
	int dis_default_para;
	int diff_frames_th;
	int stable_frames_th[2];
	int small_diff_step;
	int small_diff_qp[2];
	int large_diff_qp[2];
	float diff_th[2];
} VencAeDiffParam;

typedef enum eRotationType {
	RotAngle_0 = 0x0,
	RotAngle_90,
	RotAngle_180,
	RotAngle_270,
} eRotationType;

#define ISP_REG_TBL_LENGTH			33

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

typedef struct sEncppSharpParam{
	sEncppSharpParamDynamic mDynamicParam;
	sEncppSharpParamStatic  mStaticParam;
} sEncppSharpParam;

typedef struct {
	int win_pix_n;
	int avg[AE_AVG_NUM];
	int hist[256];
	int hist1[256];
} sIspAeStatus;

typedef struct{
	eCameraStatus mEnCameraMove;
	int mEnvLv;
	int mAeWeightLum;
	sEncppSharpParam mSharpParam;
	sIspAeStatus mIspAeStatus;
} VencIsp2VeParam;

typedef struct {
	unsigned char isp2VeEn;
	unsigned char ve2IspEn;
} VencIspVeLinkParam;

#define US_PER_MS          		(1*1000)
#define US_PER_S          		(1*1000*1000)
#define MS_PER_S          		(1*1000)


typedef enum {
	PSKIP               = 0,
	BSKIP_DIRECT        = 0,
	P16x16              = 1,
	P16x8               = 2,
	P8x16               = 3,
	SMB8x8              = 4,
	SMB8x4              = 5,
	SMB4x8              = 6,
	SMB4x4              = 7,
	P8x8                = 8,
	I4MB                = 9,
	I16MB               = 10,
	IBLOCK              = 11,
	SI4MB               = 12,
	I8MB                = 13,
	IPCM                = 14,
	MAXMODE             = 15
} MB_TYPE;

typedef struct {
	short         mv_x;
	short         mv_y;
	int           mode;
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

typedef enum ExifExposureProgramType {
	EXPOSURE_PROGRAM_MANUAL = 1,
	EXPOSURE_PROGRAM_NORMAL = 2,
	EXPOSURE_PROGRAM_APERTURE_PRIORITY = 3,
	EXPOSURE_PROGRAM_SHUTTER_PRIORITY = 4,
	EXPOSURE_PROGRAM_CREATIVE = 5,
	EXPOSURE_PROGRAM_ACTION = 6,
	EXPOSURE_PROGRAM_PORTRAIT = 7,
	EXPOSURE_PROGRAM_LANDSCAPE = 8,
} ExifExposureProgramType;

typedef enum ExifMeteringModeType {
	METERING_MODE_UNKNOWN = 0,
	METERING_MODE_AVERAGE = 1,
	METERING_MODE_CENTER = 2,
	METERING_MODE_SPOT = 3,
	METERING_MODE_MULTI_SPOT = 4,
	METERING_MODE_MULTI_SEGMENT = 5,
	METERING_MODE_PARTIAL = 6,
	METERING_MODE_OTHERS = 255,
} ExifMeteringModeType;

typedef enum {
	LIGHT_SOURCE_UNKNOWN = 0,
	LIGHT_SOURCE_SUNLIGHT = 1,
	LIGHT_SOURCE_FLUORESCENT_LAMP = 2,
	LIGHT_SOURCE_TUNGSTEN_LAMP = 3,
	LIGHT_SOURCE_FLASH_LAMP = 4,
	LIGHT_SOURCE_OVERCAST = 9,
	LIGHT_SOURCE_CLOUDY = 10,
	LIGHT_SOURCE_SHADOW = 11,
	LIGHT_SOURCE_SUNLIGHT_FLUORESCENT_LAMP = 12,
	LIGHT_SOURCE_WHITE_DAY_FLUORESCENT_LAMP = 13,
	LIGHT_SOURCE_COOL_COLOUR_FLUORESCENT_LAMP = 14,
	LIGHT_SOURCE_WHITE_FLUORESCENT_LAMP = 15,
	LIGHT_SOURCE_STANDARD_LAMP_A = 17,
	LIGHT_SOURCE_STANDARD_LAMP_B = 18,
	LIGHT_SOURCE_STANDARD_lAMP_C = 19,
	LIGHT_SOURCE_D55 = 20,
	LIGHT_SOURCE_D65 = 21,
	LIGHT_SOURCE_D75 = 22,
	LIGHT_SOURCE_D50 = 23,
	LIGHT_SOURCE_PROJECTION_ROOM_LAMP = 24,
	LIGHT_SOURCE_OTHERS = 255
} ExifLightSource;

typedef enum ExifFlashType {
	FLASH_CLOSE = 0,
	FLASH_OPEN = 1,
	FLASH_OPEN_EN_RETURN = 5,
	FLASH_OPEN_DIS_RETURN = 7,
	FLASH_OPEN_FORCE = 9,
	FLASH_OPEN_FORCE_DIS_RETURN = 13,
	FLASH_OPEN_FORCE_EN_RETURN = 15,
	FLASH_CLOSE_FORCE = 16,
	FLASH_CLOSE_AUTO = 24,
	FLASH_OPEN_AUTO = 25,
	FLASH_OPEN_AUTO_DIS_RETURN = 29,
	FLASH_OPEN_AUTO_EN_RETURN = 31,
	FLASH_NO_FUNCTION = 32,
	FLASH_OPEN_RED_EYE = 65,
	FLASH_OPEN_RED_EYE_DIS_RETURN = 69,
	FLASH_OPEN_RED_EYE_EN_RETURN = 71,
	FLASH_OPEN_FORCE_RED_EYE = 73,
	FLASH_OPEN_FORCE_RED_EYE_DIS_RETURN = 77,
	FLASH_OPEN_FORCE_RED_EYE_EN_RETURN = 79,
	FLASH_OPEN_AUTO_RED_EYE = 89,
	FLASH_OPEN_AUTO_RED_EYE_DIS_RETURN = 93,
	FLASH_OPEN_AUTO_RED_EYE_EN_RETURN = 95,
} ExifFlashType;

typedef enum ExifExposureModeType {
	EXPOSURE_MODE_AUTO = 0,
	EXPOSURE_MODE_MANUAL = 1,
	EXPOSURE_MODE_AUTO_BRACKET = 2,
} ExifExposureModeType;

typedef enum ExifContrastType {
	CONTRAST_NORMAL = 0,
	CONTRAST_SOFT = 1,
	CONTRAST_HARD = 2,
} ExifContrastType;

typedef enum ExifSaturationType {
	SATRATION_NORMAL = 0,
	SATRATION_LOW = 1,
	SATRATION_HIGH = 2,
} ExifSaturationType;

typedef enum ExifSharpnessType {
	SHARPNESS_NORMAL = 0,
	SHARPNESS_SOFT = 1,
	SHARPNESS_HARD = 2,
} ExifSharpnessType;

typedef struct EXIFInfo {
	unsigned char  CameraMake[INFO_LENGTH];
	unsigned char  CameraModel[INFO_LENGTH];
	unsigned char  DateTime[DATA_TIME_LENGTH];

	unsigned int   ThumbWidth;
	unsigned int   ThumbHeight;
	unsigned char *ThumbAddrVir[2];
	unsigned int   ThumbLen[2];

	int              Orientation;  //value can be 0,90,180,270 degree
	rational_t       ExposureTime; //tag 0x829A
	rational_t       FNumber; //tag 0x829D
	short           ISOSpeed;//tag 0x8827

	srational_t    ShutterSpeedValue; //tag 0x9201
	rational_t       Aperture; //tag 0x9202
	//srational_t    BrightnessValue;   //tag 0x9203
	srational_t    ExposureBiasValue; //tag 0x9204

	rational_t       MaxAperture; //tag 0x9205

	ExifMeteringModeType  MeteringMode; //tag 0x9207
	ExifLightSource       LightSource; //tag 0x9208
	ExifFlashType         FlashUsed;   //tag 0x9209
	rational_t       FocalLength; //tag 0x920A

	rational_t       DigitalZoomRatio; // tag 0xA404
	ExifContrastType     Contrast;     // tag 0xA408
	ExifSaturationType   Saturation;   // tag 0xA409
	ExifSharpnessType    Sharpness;    // tag 0xA40A
	ExifExposureProgramType ExposureProgram; // tag 0x8822
	short           WhiteBalance; //tag 0xA403
	ExifExposureModeType    ExposureMode; //tag 0xA402

	rational_t      resolution_x;//tag 0x011A
	rational_t      resolution_y;//tag 0x011B

	// gps info
	int            enableGpsInfo;
	double         gps_latitude;
	double           gps_longitude;
	double         gps_altitude;
	long           gps_timestamp;
	unsigned char  gpsProcessingMethod[GPS_PROCESS_METHOD_LENGTH];

	unsigned char  CameraSerialNum[128];     //tag 0xA431 (exif 2.3 version)
	short              FocalLengthIn35mmFilm;     // tag 0xA405

	unsigned char  ImageName[128];             //tag 0x010D
	unsigned char  ImageDescription[128];     //tag 0x010E
	short            ImageWidth;                 //tag 0xA002
	short            ImageHeight;             //tag 0xA003

	int             thumb_quality;
} EXIFInfo;

typedef enum VENC_YUV2YUV {
	VENC_YCCToBT601,
	VENC_BT601ToYCC,
} VENC_YUV2YUV;

//* The Amount of Temporal SVC Layers
typedef enum {
	NO_T_SVC = 0,
	T_LAYER_2 = 2,
	T_LAYER_3 = 3,
	T_LAYER_4 = 4
} T_LAYER;

//* The Multiple of Skip_Frame
typedef enum {
	NO_SKIP = 0,
	SKIP_2 = 2,
	SKIP_4 = 4,
	SKIP_8 = 8
} SKIP_FRAME;

typedef struct VencThumbInfo {
	unsigned int        nThumbSize;
	unsigned char *pThumbBuf;
	unsigned int        bWriteToFile;
	FILE_STRUCT *fp;
	unsigned int        nEncodeCnt;
} VencThumbInfo;

typedef struct VencBaseConfig {
	unsigned char       bEncH264Nalu;
	unsigned int        nInputWidth;
	unsigned int        nInputHeight;
	unsigned int        nDstWidth;
	unsigned int        nDstHeight;
	unsigned int        nStride;
	VENC_PIXEL_FMT      eInputFormat;
	VENC_OUTPUT_FMT     eOutputFormat;
	struct ScMemOpsS *memops;
	VeOpsS *veOpsS;
	void *pVeOpsSelf;

	unsigned char     bOnlyWbFlag;

	//* for v5v200 and newer ic
	unsigned char     bLbcLossyComEnFlag1_5x;
	unsigned char     bLbcLossyComEnFlag2x;
	unsigned char     bLbcLossyComEnFlag2_5x;
	unsigned char     bIsVbvNoCache;
	//* end

	unsigned int      bOnlineMode;    //* 1: online mode,    0: offline mode;
	unsigned int      bOnlineChannel; //* 1: online channel, 0: offline channel;
	unsigned int      nOnlineShareBufNum; //* share buffer num
	unsigned int nChannel;
	unsigned int sensor_id;
	unsigned int bk_id;

	//*for debug
	unsigned int extend_flag;  //* flag&0x1: printf reg before interrupt
							   //* flag&0x2: printf reg after  interrupt
	eVeLbcMode   rec_lbc_mode; //*0: disable, 1:1.5x , 2: 2.0x, 3: 2.5x, 4: no_lossy, 5:1.0x
	//*for debug(end)

	unsigned int bVcuOn;
	unsigned int bVcuAutoMode;
	// unsigned int nFrameNumInGroup;
	int channel_id;
	unsigned int bEnableMultiOnlineSensor;
    unsigned int bEnableImageStitching;
} VencBaseConfig;

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
	unsigned int    nLength;
} VencHeaderData;

typedef struct VencCopyROIConfig {
	int                     bEnable;
	int                     num; /* (0~16) */
	VencRect                sRect[16];
	unsigned char         *pRoiYAddrVir;
	unsigned char         *pRoiCAddrVir;
	unsigned long         pRoiYAddrPhy;
	unsigned long         pRoiCAddrPhy;
	int                   size;
} VencCopyROIConfig;

typedef struct VencInputBuffer {
	unsigned long  nID;
	long long         nPts;
	unsigned int   nFlag;
	unsigned char *pAddrPhyY;
	unsigned char *pAddrPhyC;
	unsigned char *pAddrVirY;
	unsigned char *pAddrVirC;
	int            bEnableCorp;
	VencRect       sCropInfo;

	int            ispPicVar;
	int            ispPicVarChroma;     //chroma  filter  coef[0-63],  from isp
	int			   bUseInputBufferRoi;
	VencROIConfig  roi_param[8];
	int            bAllocMemSelf;
	int            nShareBufFd;
	unsigned char  bUseCsiColorFormat;
	VENC_PIXEL_FMT eCsiColorFormat;

	int             envLV;
	int             bNeedFlushCache;
} VencInputBuffer;

typedef struct FrameInfo {
	int             CurrQp;
	int             avQp;
	int             nGopIndex;
	int             nFrameIndex;
	int             nTotalIndex;
} FrameInfo;

typedef struct VeProcSet {
	unsigned char               bProcEnable;
	unsigned int                nProcFreq;
	unsigned int				nStatisBitRateTime;
	unsigned int				nStatisFrRateTime;
} VeProcSet;

typedef struct {
	long long mb_time;
	long long bin_time;
	long long sei_time;
	long long rst_time;
	long long int_start;
	long long int_end;
	long long last_int_start;
	long long last_int_end;
	long long int_time;
	long long par_start;
	long long par_end;
	long long last_par_start;
	long long last_par_end;
	long long par_time;
	long long sum_time;
	long long avg_par_time;
	long long avg_int_time;
	long long pskip_time_start;
	long long pskip_time_end;
} StatisticTime;

typedef struct {
	float nSceneCoef[3];
	float nMoveCoef[5];
} VencIPTargetBitsRatio;

typedef struct VencOutputBuffer {
	int               nID;
	long long         nPts;
	unsigned int   nFlag;
	unsigned int   nSize0;
	unsigned int   nSize1;
	unsigned char *pData0;
	unsigned char *pData1;

	FrameInfo       frame_info;
	unsigned int   nSize2;
	unsigned char *pData2;

	unsigned int   nExSize0;
	unsigned int   nExSize1;
	unsigned char  *pExData0;
	unsigned char  *pExData1;
	unsigned int   nPackNum;
	VencPackInfo   mPackInfo[MAX_OUTPUT_PACK_NUM];
} VencOutputBuffer;

typedef struct VencAllocateBufferParam {
	unsigned int   nSizeY;
	unsigned int   nSizeC;
} VencAllocateBufferParam;

#define EXTENDED_SAR 255
typedef struct VencH264AspectRatio {
	unsigned char aspect_ratio_idc;
	unsigned short  sar_width;
	unsigned short  sar_height;
} VencH264AspectRatio;

typedef struct VencSaveBSFile {
	char filename[256];
	unsigned char save_bsfile_flag;
	unsigned int save_start_time;
	unsigned int save_end_time;
} VencSaveBSFile;

// Add for setting SVC and Skip_Frame
typedef struct VencH264SVCSkip {
	T_LAYER        nTemporalSVC;
	SKIP_FRAME     nSkipFrame;
	int            bEnableLayerRatio;
	unsigned int   nLayerRatio[4];
} VencH264SVCSkip;

typedef struct VencSize {
	int                     nWidth;
	int                     nHeight;
} VencSize;

typedef struct VencCheckColorFormat {
	int                        index;
	VENC_PIXEL_FMT          eColorFormat;
} VencCheckColorFormat;

typedef struct VencVP8Param {
	int                     nFramerate; /* fps*/
	int                     nBitrate;   /* bps*/
	int                     nMaxKeyInterval;
} VencVP8Param;

typedef struct VencRoiBgFrameRate {
	int            nSrcFrameRate;
	int            nDstFrameRate;
} VencRoiBgFrameRate;

typedef struct VencAlterFrameRateInfo {
	unsigned char       bEnable;
	unsigned char       bUseUserSetRoiInfo;   //0:use csi roi info; 1:use user set roi info
	VencRoiBgFrameRate  sRoiBgFrameRate;
	VencROIConfig       roi_param[8];
} VencAlterFrameRateInfo;

typedef struct VencH265TranS {
	/*** unsigned char       transquant_bypass_enabled_flag; not support ***/
	//0:disable transform skip; 1:enable transform skip
	unsigned char       transform_skip_enabled_flag;
	//chroma_qp= sliece_qp+chroma_qp_offset
	int                chroma_qp_offset;
} VencH265TranS;

typedef struct VencH265SaoS {
	//0:disable luma sao filter; 1:enable luma sao filter
	unsigned char       slice_sao_luma_flag;
	//0:disable chroma sao filter; 1:enable chroma sao filter
	unsigned char       slice_sao_chroma_flag;
} VencH265SaoS;

typedef struct VencH265DblkS {
	//0:enable deblock filter; 1:disable deblock filter
	unsigned char       slice_deblocking_filter_disabled_flag;
	int                 slice_beta_offset_div2; //range: [-6,6]
	int                 slice_tc_offset_div2; //range: [-6,6]
} VencH265DblkS;

typedef struct VencOverlayHeaderS {
	unsigned short      start_mb_x;         //horizonal value of  start points divided by 16
	unsigned short      end_mb_x;           //horizonal value of  end points divided by 16
	unsigned short      start_mb_y;         //vertical value of  start points divided by 16
	unsigned short      end_mb_y;           //vertical value of  end points divided by 16
	unsigned char       extra_alpha_flag;   //0:no use extra_alpha; 1:use extra_alpha
	unsigned char       extra_alpha;        //use user set extra_alpha, range is [0, 15]
	VencOverlayCoverYuvS cover_yuv;         //when use COVER_OVERLAY should set the cover yuv
	VENC_OVERLAY_TYPE   overlay_type;       //reference define of VENC_OVERLAY_TYPE
	unsigned char *overlay_blk_addr;   //the vir addr of overlay block
	unsigned int        bitmap_size;        //the size of bitmap

	//* for v5v200 and newer ic
	unsigned int        bforce_reverse_flag;
	unsigned int        reverse_unit_mb_w_minus1;
	unsigned int        reverse_unit_mb_h_minus1;
	//* end

} VencOverlayHeaderS;

typedef struct VencOverlayInfoS {
	unsigned char               blk_num; //num of overlay region
	VENC_OVERLAY_ARGB_TYPE      argb_type;//reference define of VENC_ARGB_TYPE
	VencOverlayHeaderS          overlayHeaderList[MAX_OVERLAY_SIZE];

	//* for v5v200 and newer ic
	unsigned int                invert_mode;
	unsigned int                invert_threshold;
	//* end

} VencOverlayInfoS;

typedef struct VencBrightnessS {
	unsigned int               dark_th; //dark threshold, default 60, range[0, 255]
	unsigned int               bright_th; //bright threshold, default 200, range[0, 255]
} VencBrightnessS;

typedef struct VencEncodeTimeS {
	unsigned int                  frame_num; //current frame num
	unsigned int                  curr_enc_time; //current frame encoder time
	unsigned int                  curr_empty_time; //the time between current frame and last frame
	unsigned int                  avr_enc_time; //average encoder time
	unsigned int                  avr_empty_time; //average empty time
	unsigned int                  max_enc_time;
	unsigned int                  max_enc_time_frame_num;
	unsigned int                  max_empty_time;
	unsigned int                  max_empty_time_frame_num;
} VencEncodeTimeS;

typedef enum {
	VencInfoType_1Sec = 1<<0,
	VencInfoType_10Sec = 1<<1,
	VencInfoType_1Times = 1<<2,
} VencSeiInfoType;

typedef struct {
	int nSuperFrameMode;
	int nMaxIFrameKB;
	int nMaxPFrameKB;
	int nMaxRencodeTimes;
	int nMaxP2IFrameBitsRatio;
} SeiSuperFrameConfig;

typedef struct {
	int dis_default_para;
	int mode;
	int en_gop_clip;
	int gop_bit_ratio_th[3];
	int coef_th[5][2];
} SeiTargetBitsClipParam;

typedef struct {
	int enable_2d_filter;
	int filter_strength_uv; //* range[0~255], 0 means close 2d filter, advice: 32
	int filter_strength_y;  //* range[0~255], 0 means close 2d filter, advice: 32
	int filter_th_uv;       //* range[0~15], advice: 2
	int filter_th_y;        //* range[0~15], advice: 2
} Sei2DfilterParam;

typedef struct {
	int luma_en;
	int chroma_en;               //* range[0~1]: luma filter enable
	int move_status_en;          //* range[0~1]: chroma filter enable
	int pix_diff_en;             //* range[0~1]: pixel level enable
	int coef_auto_en;            //* range[0~1]: ref coef auot mode enable
	int max_coef;                //* range[0~16]: minimum weight coef of 3d filter
	int min_coef;                //* range[0~16]: maximum weight coef of 3d filter
} Sei3DfilterParam;

typedef struct {
	int region_en;
	int auot_en;
	int region_width;
	int region_height;
	int hor_region_num;
	int ver_region_num;
	int total_region_num;
	int hor_expand_en;
	int ver_expand_en;
	int region_mv_rd_en;
	int region_mv_wr_en;
	int small_mv_levle_max_num[2];
	int region_mv_level_ratio_th[3];
} SeiRegionD3DParam;

typedef struct {
	int               bOnlineEn;
	int               nInputFormat;
	int               bGdcEn;
	int               bSharpEn;
	int               nRefBufFormat;
	int               bRefPageBufEn;
	int               nRcMode;
	int               nProductCase;
	int               nQuality;
	int               nPBitsCoef;
	VencQPRange               mQpRange;
	SeiSuperFrameConfig       mSuperFrame;
	SeiTargetBitsClipParam    mBitsClip;
	Sei2DfilterParam          mD2D;
	Sei3DfilterParam          mD3D;
	SeiRegionD3DParam         mRegionD3D;
} VencSeiInfo1Times;

typedef struct {
	int nMadTh[12];
	int nMad0Hist[12];
} VencSeiInfo10Sec;

typedef struct {
	int nSceneStatus;
	int nMoveStatus;
	int nEnvLv;
	int nIspD2DLevel;
	int nIspD3DLevel;
	int nCameraJudge;
	int bCameraMoving;
	int nMovingMaxQp;
} VencSeiInfo1Sec;

#define SEI_1TIMES_LEN (260)
#define SEI_10SEC_LEN  (92)
#define SEI_1SEC_LEN   (32)
typedef struct {
	VencSeiInfoType   nInfoBitFlags; //VencInfoType_1Sec | VencInfoType_10Sec | VencInfoType_1Times
	char              mInfo1Sec[SEI_1SEC_LEN];
	char              mInfo10Sec[SEI_10SEC_LEN];
	char              mInfo1Times[SEI_1TIMES_LEN];
} VencSeiInfo;

typedef struct VencProductModeInfo {
	eVencProductMode eProductMode;
	unsigned int     nDstWidth;
	unsigned int     nDstHeight;
	int              nBitrate;   // bps
	int              nFrameRate;
} VencProductModeInfo;

typedef struct VeProcEncH265Info {
	float                       Lambda;
	float                       LambdaC;
	float                       LambdaSqrt;

	unsigned int                uIntraCoef0;
	unsigned int                uIntraCoef1;
	unsigned int                uIntraCoef2;
	unsigned int                uIntraTh32;
	unsigned int                uIntraTh16;
	unsigned int                uIntraTh8;

	unsigned int                uInterTend;
	unsigned int                uSkipTend;
	unsigned int                uMergeTend;

	unsigned char               fast_intra_en;
	unsigned char               adaptitv_tu_split_en;
	unsigned int                combine_4x4_md_th;
} VeProcEncH265Info;

typedef struct VencQVbrParam {
	int                  nIPratio;
	unsigned char        uQVBRParamEn;
	int                  nTPUpDateFrame;
	int                  nTPCalFrame;
	int                  nErrorTh[2]; //0: I frames; 1: P frames
	int                  nTargetBitDiffFrame;
	float                fPsnrCoeffTh[4];
	float                fPsnrCalTPAdjustTh[8];
	float                fMoveCalTPJudgeTh;
	float                fMoveCalTPAdjustTh;
} VencQVbrParam;

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

	VencCropCfg                 CropCfg;

	unsigned int                nProfileIdc;
	unsigned int                nLevelIdc;

	int                         nBitRate;
	int                         nFrameRate;
	int                         bPSkipEn;
	int                         nPInsertNum;

	eVencProductMode            nProductMode;
	VENC_RC_MODE				eRcMode;
	VENC_VIDEO_GOP_MODE         eGopMode;
	int                         nIDRItl;
	int                         nIsIpcCase;
	int                         nColourMode;

	VencFixQP                   fix_qp;

	int                         ori_i_qp_max;
	int                         i_qp_max;
	int                         i_qp_min;
	int                         ori_p_qp_max;
	int                         p_qp_max;
	int                         p_qp_min;
	int                         nInitQp;
	int                         en_mb_qp_limit;

	VencROIConfig               sRoi[8];

	unsigned int				avr_bit_rate;
	unsigned int				real_bit_rate;
	unsigned int				avr_frame_rate;
	unsigned int				real_frame_rate;

	int							vbv_size;
	int							UnusedBufferSize;
	int							nValidFrameNum;
	int							nValidDataSize;
	int                         nMaxFrameLen;

	int                         eSliceType;
	int                         nCurrQp;
	int                         nFrameIndex;
	int                         nTotalIndex;

	int                         nRealBits;
	int                         nTargetBits;

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
	unsigned char               cpmv_select;
	unsigned char               sme_leftmv_update;
	unsigned char               mv_denoise_en;
	unsigned int                noise_estimate;

	VencIPTargetBitsRatio       sBitsRatio;
	VencTargetBitsClipParam     sBitsClipParam;
	VencAeDiffParam             sAeDiffParam;

	float                       WeakTextTh;
	unsigned char               MbRcEn;
	int                         EnIFrmMbRcMoveStatus;
	unsigned char               MadEn;
	unsigned char               FrmAvgMad;
	float                       MadHistogram[12][11];
	unsigned char               ClassifyTh[12];
	unsigned char               MdEn;
	unsigned char               FrmAvgMd;
	float                       MdHistogram[16][11];
	int                         MdQp[16];
	unsigned int                MdLevel[16];

	unsigned int				uMaxBitRate;
	int 						nQuality;
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
	int                         lens_moving_num;
	int                         lens_moving_max_qp;

	VencVe2IspD2DLimit          ve2isp_d2d;
	VencRotVe2Isp               rot_ve2isp;
	int                         isp_d2d_level;
	int                         isp_d3d_level;

	int                         bIsOverflow;
	int                         nIspScale;
	int                         nSuperTotalTimes;
	VencSuperFrameConfig        sSuperFrameCfg;
	int                         nSliceQpMaxDelta;
	int                         nSliceQpMinDelta;
	unsigned char               cIsp2VeEn;
	unsigned char               cVe2IspEn;
	unsigned int                uVbrOptEn;
	VencVbrOptParam             sVbrOptInfo;
	Venc3dFilterParam           sE3dFilterInfo;
	sRegionLinkParam            sRegionLinkInfo;
	VeProcEncH265Info           sH265Info;
} VeProcEncInfo;

typedef enum VENC_INDEXTYPE {
	VENC_IndexParamBitrate                = 0x0,
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
	VENC_IndexParamOutputFormat,
	/**< reference type: VENC_OUTPUT_FMT */
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
	VENC_IndexParamH264Param  = 0x100,
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
	VENC_IndexParamH264NalRefIdc,
	/**< reference type: unsigned char */
	VENC_IndexParamChmoraGray,
	/**< reference type: unsigned char */
	VENC_IndexParamIQpOffset,
	/**< reference type: int */
	/* jpeg param */
	VENC_IndexParamJpegQuality            = 0x200,
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
	/**< reference type: VencEncodeTimeS[2] [0]:hw [1]:sw */
	VENC_IndexParamGetEncodeTime,
	/**< reference type: s2DfilterParam */
	VENC_IndexParam2DFilter,
	/**< reference type: Venc3dFilterParam */
	VENC_IndexParam3DFilter,
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

	/**< reference type: Get VencMotionSearchResult* */
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

	/**< reference type: unsigned int; 0: not ready, 1: ready*/
	VENC_IndexParamCheckOnlineStatus,

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

	/**< reference type: VencAeDiffParam */
	VENC_IndexParamAeDiffParam,

	/**< reference type: MoveStatus */
	VENC_IndexParamEnIFrmMbRcMoveStatus,

	/**< reference type: sIspAeStatus */
	VENC_IndexParamIspAeStatus,

	/* set rec_lbc_mode */
	VENC_IndexParamSetRecRefLbcMode,

	/* set RecRef buf reduce function; type: int; 0: disbale, 1: enable*/
	VENC_IndexParamEnableRecRefBufReduceFunc,

	/**< reference type: float [0,100]*/
	VENC_IndexParamWeakTextTh,

	/**< reference type: int [0,1]*/
	VENC_IndexParamEnD3DInIFrm,

	/**< reference type: int [0,1]*/
	VENC_IndexParamEnTightMbQp,

	/**< reference type: int */
	VENC_IndexParamChromaQPOffset,

	/**< reference type:  VencH264ConstraintFlag */
	VENC_IndexParamH264ConstraintFlag,

	/**< reference type: VencVe2IspD2DLimit */
	VENC_IndexParamVe2IspD2DLimit,

	/**< reference type: int [0,1] */
	VENC_IndexParamEnSmallSearchRange,

	/**< reference type: VencForceConfWin */
	VENC_IndexParamForceConfWin,

	/**< reference type: VencRotVe2Isp */
	VENC_IndexParamRotVe2Isp,

	/**< reference type: VencSeiParam */
	VENC_IndexParamSeiParam,

	/**< reference type: VencSeiInfo */
	VENC_IndexParamSeiInfo,

	/**< reference type: VencInsertData */
	VENC_IndexParamInsertData,

	/**< reference type: VENC_BUF_STATUS */
	VENC_IndexParamInsertDataBufStatus,

	/**< reference type: int [0,1] */
	VENC_IndexParamEncAndDecCase, /*encoder and decoder run at the same time. need reset the whole ve*/

	/**< reference type: int [0,1]*/
	VENC_IndexParamVbrOptEnable,

	/**< reference type: VencVbrOptParam*/
	VENC_IndexParamVbrOptParam,

	/**< reference type: int [1,51] */
	VENC_IndexParamLensMovingMaxQp,
	/**< reference type: int [1,51] */
	VENC_IndexParamCheckIsReadyToEncode,
	VENC_IndexParamGetVbvShareFd,

	/**< reference type: sRegionLinkParam*/
	VENC_IndexParamRegionDetectLinkParam,

	/**< reference type: VencCropCfg*/
	VENC_IndexParamCropCfg,

	/**< reference type: VencQVbrParam*/
	VENC_IndexParamQVbrParam,

	/**< reference type: VencIspVeLinkParam*/
	VENC_IndexParamIspVeLinkParam,
} VENC_INDEXTYPE;

typedef struct VencEnvLvRange {
	int env_lv_high_th;
	int env_lv_low_th;
	int env_lv_coef;  //range[0,15]
} VencEnvLvRange;

typedef enum VENC_RESULT_TYPE {
	VENC_RESULT_ERROR              = -1,
	VENC_RESULT_OK                 = 0,
	VENC_RESULT_NO_FRAME_BUFFER    = 1,
	VENC_RESULT_BITSTREAM_IS_FULL  = 2,
	VENC_RESULT_ILLEGAL_PARAM      = 3,
	VENC_RESULT_NOT_SUPPORT        = 4,
	VENC_RESULT_BITSTREAM_IS_EMPTY = 5,
	VENC_RESULT_NO_MEMORY          = 6,
	VENC_RESULT_NO_RESOURCE        = 7,
	VENC_RESULT_NULL_PTR           = 8,
	VENC_RESULT_DROP_FRAME         = 9,
	VENC_RESULT_CONTINUE           = 10,
	VENC_RESULT_USER_DROP_FRAME    = 11, //user command to drop frame, due to VENC_IndexParamDropFrame
	VENC_RESULT_TIMEOUT            = 12,
	VENC_RESULT_EFUSE_ERROR  = 25,
} VENC_RESULT_TYPE;

typedef enum {
	H265_B_SLICE             = 0x0,
	H265_P_SLICE             = 0x1,
	H265_I_SLICE             = 0x2,
	H265_IDR_SLICE           = 0x12
} VENC_H265_CODE_TYPE;

typedef struct JpegEncInfo {
	VencBaseConfig  sBaseInfo;
	int             bNoUseAddrPhy;
	unsigned char *pAddrPhyY;
	unsigned char *pAddrPhyC;
	unsigned char *pAddrVirY;
	unsigned char *pAddrVirC;
	int             bEnableCorp;
	VencRect        sCropInfo;
	int                quality;
	int             nShareBufFd;
} JpegEncInfo;

typedef struct VbvInfo {
	unsigned int vbv_size;
	unsigned int coded_frame_num;
	unsigned int coded_size;
	unsigned int maxFrameLen;
	unsigned char *start_addr;
} VbvInfo;

typedef enum {
	REF_IDC_DISCARD = 0,
	REF_IDC_CURRENT_USE = 1,
	REF_IDC_FUTURE_USE = 2,
	REF_IDC_LONG_TERM = 4,
	REF_IDC_CURRENT_REF = 8,
} ReferenceIdc;

typedef struct {
	long long min_mvy              : 10; // [09:00]
	long long reserve_11_10        : 2;  // [11:10]
	long long min_mvx              : 12; // [23:12]
	long long resi_mad             : 7;  // [30:24]
	long long reserve_31           : 1;  // [31]
	long long max_mvy              : 10; // [41:32]
	long long reserve_43_42        : 2;  // [43:42]
	long long max_mvx              : 12; // [55:44]
	long long min_cost             : 8;  // [63:56]
} VencMvInfoParcel;

typedef struct {
	long long mad                  : 8;  // [07:00] high 6 bits integer + low 2 bits frac
	long long qp                   : 6;  // [13:08]
	long long mb_type              : 2;  // [15:14] 0:skip 1:inter 2:intra
	long long md                   : 8;  // [23:14]
	long long sse                  : 16; // [39:24]
	long long move_status0         : 2;  // [41:40]
	long long move_status1         : 2;  // [43:42]
	long long move_status2         : 2;  // [45:44]
	long long move_status3         : 2;  // [47:46]
	long long mb_bits              : 12; // [59:48]
	long long reserve_63_60        : 4;  // [63:60]
} VencMbInfoParcel;

typedef struct {
	unsigned int    slice_type;
	int             poc; // dispaly order of the frame within a GOP, ranging from 1 to gop_size
	int             qp_offset;
	int             tc_offset_div2; // offset of LoopFilterTcOffsetDiv2, ranging from -6 to 6
	int             beta_offset_div2; // offset of LoopFilterTcOffsetDiv2, ranging from -6 to 6

	unsigned int    num_ref_pics; // number of ref_frames reserved for cur_frame and future frames
	unsigned int    num_ref_pics_active; // number of ref_frames is permited to be used in L0 or L1

	int             reference_pics[MAX_FRM_NUM-1]; // = ref_frame_poc - cur_frame_poc
	// = discard_frame_poc - cur_frame_poc, means derlta_poc of ref_frames which are discarded
	int             discard_pics[MAX_FRM_NUM-1];

	unsigned char   lt_ref_flag; // 1: enable cur_frame use long term ref_frame
	int             lt_ref_poc; // poc of lt_ref_frame of cur_frame

	// 0 means next 4 member parameters are ignored; 1 means next 3 member parameters are need
	// this parameter of the first frame of a GOP must be 0
	unsigned char   predict;

	unsigned int    delta_rps_idx; // = cur_frame_encoding_idx - predictor_frame_encoding_idx

	int             delta_rps; // = predictor_frame_poc - cur_frame_poc

	// num of ref_idcs to encoder for the current frame, the value is equal to
	// the value of num_st_ref_pics of the predictor_frame + 1 + lt_ref_flag
	unsigned int    num_ref_idcs;

	// [][0]=(ref_frame_poc or discard_frame_poc) - cur_frame_poc
	// [][1]indicating the ref_pictures reserved in ref_list_buffer:
	// [][1]=0: will not be a ref_picture anymore
	// [][1]=1: is a ref_picture used by cur_picture
	// [][1]=2: is a ref_picture used by future_picture
	// [][1]=3: is a long term ref_picture
	int             reference_idcs[MAX_FRM_NUM][2];
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
	unsigned char mb_qp         : 6; // {5:0}
	unsigned char mb_skip_flag  : 1; // {6}
	unsigned char mb_en         : 1; // {7}
} VencMBModeCtrlInfo;

typedef struct {
	unsigned char hp_filter_en;
	unsigned int hp_coef_shift; //* range[0 ~ 7],  default: 3
	unsigned int hp_coef_th;    //* range[0 ~ 7],  default: 5
	unsigned int hp_contrast_th;//* range[0 ~ 63], default: 0
	unsigned int hp_mad_th;     //* range[0 ~ 63], default: 0
} VencHighPassFilter;

typedef struct {
	unsigned char hvs_en;
	unsigned int  th_dir;
	unsigned int  th_coef_shift;
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
	unsigned int  min_spatial_seg_idc;
	unsigned int  max_bytes_per_pic_denom;
	unsigned int  max_bits_per_min_cu_denom;
	unsigned int  log2_max_mv_len_hor;
	unsigned int  log2_max_mv_len_ver;
} VencVUIH265BitstreamRestriction;

typedef struct {
	unsigned char mv_over_pic_boundaries_flag;
	unsigned char max_bytes_per_pic_denom;
	unsigned int  max_bits_per_mb_denom;
	unsigned int  log2_max_mv_len_hor;
	unsigned int  log2_max_mv_len_ver;
	unsigned int  max_num_reorder_frames;
	unsigned int  max_dec_frame_buffing;
} VencVUIH264BitstreamRestriction;

typedef struct {
	unsigned char *pAddrPhyY;
	unsigned char *pAddrPhyC0;
	unsigned char *pAddrPhyC1;
	unsigned char *pAddrVirY;
	unsigned char *pAddrVirC;
	unsigned char *pAddrVirC1;
	unsigned int   nWidth;
	unsigned int   nHeight;
	VENC_PIXEL_FMT colorFormat;
} VencEncppBufferInfo;

typedef struct {
	unsigned int      bScaleFlag;
    unsigned int      nThumbScaleFactor; // 0:1/1  1:1/8  2:1/2  3:1/4
	unsigned int      nRotateAngle;//* 0: no rotate; 1: 90; 2: 180; 3: 270;
	unsigned int      bHorizonflipFlag;

	unsigned int      bOverlayerFlag;
	VencOverlayInfoS *pOverlayerInfo;

	unsigned int      bCropFlag;
	VencRect         *pCropInfo;

    unsigned int      bEnableSharpFlag;
    sEncppSharpParam  *pSharpParam;

	unsigned int      bEnableGdcFlag;
	sGdcParam        *pGdcParam;


	//* yuv2yuv, 0: disable; 1: BT601 to YCC; 2: YCC to BT601;
	//*                      3: BT709 to YCC; 4: YCC to BT709;
	unsigned int      nColorSpaceYuv2Yuv;
	//* rgb2yuv, 0: BT601, 1: BT709, 2: YCC
	unsigned int      nColorSpaceRgb2Yuv;

	unsigned char     bLbcLossyComEnFlag1_5x;
	unsigned char     bLbcLossyComEnFlag2x;
	unsigned char     bLbcLossyComEnFlag2_5x;
	VencCopyROIConfig RoiConfig;
	unsigned int extend_flag;  //* flag&0x1: printf reg before interrupt
							   //* flag&0x2: printf reg after  interrupt
} VencEncppFuncParam;

//* new api
VideoEncoder *VencCreate(VENC_CODEC_TYPE eCodecType);
void VencDestroy(VideoEncoder *pEncoder);
int  VencInit(VideoEncoder *pEncoder, VencBaseConfig *pConfig);
int VencStart(VideoEncoder *pEncoder);
int VencPause(VideoEncoder *pEncoder);
int VencReset(VideoEncoder *pEncoder);
int VencFlush(VideoEncoder *pEncoder, VencFlushType eFlushType);

int VencGetParameter(VideoEncoder *pEncoder, VENC_INDEXTYPE indexType, void *paramData);
int VencSetParameter(VideoEncoder *pEncoder, VENC_INDEXTYPE indexType, void *paramData);

int VencGetValidOutputBufNum(VideoEncoder *pEncoder);
int VencGetUnReadOutputBufNum(VideoEncoder *pEncoder);

int VencDequeueOutputBuf(VideoEncoder *pEncoder, VencOutputBuffer *pBuffer);
int VencQueueOutputBuf(VideoEncoder *pEncoder, VencOutputBuffer *pBuffer);

int VencGetValidInputBufNum(VideoEncoder *pEncoder);
int VencQueueInputBuf(VideoEncoder *pEncoder, VencInputBuffer *inputbuffer);
int VencAllocateInputBuf(VideoEncoder *pEncoder, VencAllocateBufferParam *pBufferParam, VencInputBuffer *dst_inputBuf);
void VencGetVeIommuAddr(VideoEncoder *pEncoder, struct user_iommu_param *pIommuBuf);
void VencFreeVeIommuAddr(VideoEncoder *pEncoder, struct user_iommu_param *pIommuBuf);
void VencSetDdrMode(VideoEncoder *pEncoder, int nDdrType);
int VencSetFreq(VideoEncoder *pEncoder, int nVeFreq);

int VencJpegEnc(JpegEncInfo *pJpegInfo, EXIFInfo *pExifInfo,
	void *pOutBuffer, int *pOutBufferSize);

typedef void *VideoEncoderEncpp;

VideoEncoderEncpp *VencEncppCreate(void);

void VencEncppDestroy(VideoEncoderEncpp *pEncpp);

int VencEncppFunction(VideoEncoderEncpp *pEncpp,
							VencEncppBufferInfo *pInBuffer,
							VencEncppBufferInfo *pOutBuffer,
							VencEncppFuncParam *pIspFunction);

typedef enum {
	VencEvent_FrameFormatNotMatch  = 0,  // frame format is not match to initial setting.
	VencEvent_UpdateMbModeInfo     = 1,
	VencEvent_UpdateMbStatInfo     = 2,
	VencEvent_UpdateIspToVeParam   = 3,
	VencEvent_UpdateIspMotionParam = 4,
	VencEvent_UpdateVeToIspParam   = 5,
	VencEvent_Max = 0x7FFFFFFF
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
} VencCbType;

int VencSetCallbacks(VideoEncoder *pEncoder, VencCbType *pCallbacks, void *pAppData);
int encodeOneFrame(VideoEncoder *pEncoder);

void cdc_log_set_level(unsigned int level);
typedef void *VENC_DEVICE_HANDLE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif    //_VENCODER_H_


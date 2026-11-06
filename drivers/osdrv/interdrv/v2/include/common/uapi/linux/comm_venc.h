/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/comm_venc.h
 * Description:
 *   Common video encode definitions.
 */
#ifndef __COMM_VENC_H__
#define __COMM_VENC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#ifndef USE_KERNEL_MODE
#include <pthread.h>
#endif
#include <linux/comm_video.h>
#include <linux/comm_rc.h>
#include <linux/common.h>
#include <linux/comm_vb.h>

#define JPEG_MARKER_ORDER_CNT	16

enum VENC_BIND_MODE_E {
	VENC_BIND_DISABLE  = 0,
	VENC_BIND_VI,
	VENC_BIND_VPSS,
	VENC_BIND_DPU,
	VENC_BIND_STITCH,
};


// TODO: refinememt for hardcode
#define ENUM_ERR_VENC_INVALID_CHNID \
	((int)(0xC0000000L | ((ID_VENC) << 16) | ((4) << 13) | (2)))
#define ENUM_ERR_VENC_VENC_INIT \
	((int)(0xC0000000L | ((ID_VENC) << 16) | ((4) << 13) | (64)))

typedef enum {
	DRV_ERR_VENC_INVALID_CHNID = ENUM_ERR_VENC_INVALID_CHNID,
	DRV_ERR_VENC_ILLEGAL_PARAM,
	DRV_ERR_VENC_EXIST,
	DRV_ERR_VENC_UNEXIST,
	DRV_ERR_VENC_NULL_PTR,
	DRV_ERR_VENC_NOT_CONFIG,
	DRV_ERR_VENC_NOT_SUPPORT,
	DRV_ERR_VENC_NOT_PERM,
	DRV_ERR_VENC_INVALID_PIPEID,
	DRV_ERR_VENC_INVALID_GRPID,
	DRV_ERR_VENC_NOMEM,
	DRV_ERR_VENC_NOBUF,
	DRV_ERR_VENC_BUF_EMPTY,
	DRV_ERR_VENC_BUF_FULL,
	DRV_ERR_VENC_SYS_NOTREADY,
	DRV_ERR_VENC_BADADDR,
	DRV_ERR_VENC_BUSY,
	DRV_ERR_VENC_SIZE_NOT_ENOUGH,
	DRV_ERR_VENC_INVALID_VB,
	DRV_ERR_VENC_GET_STREAM_END,
    ///========//
	DRV_ERR_VENC_INIT = ENUM_ERR_VENC_VENC_INIT,
	DRV_ERR_VENC_FRC_NO_ENC,
	DRV_ERR_VENC_STAT_VFPS_CHANGE,
	DRV_ERR_VENC_EMPTY_STREAM_FRAME,
	DRV_ERR_VENC_EMPTY_PACK,
	DRV_ERR_VENC_JPEG_MARKER_ORDER,
	DRV_ERR_VENC_CU_PREDICTION,
	DRV_ERR_VENC_RC_PARAM,
	DRV_ERR_VENC_H264_ENTROPY,
	DRV_ERR_VENC_H264_TRANS,
	DRV_ERR_VENC_H265_TRANS,
	DRV_ERR_VENC_MUTEX_ERROR,
	DRV_ERR_VENC_INVALILD_RET,
	DRV_ERR_VENC_H264_VUI,
	DRV_ERR_VENC_H265_VUI,
	DRV_ERR_VENC_GOP_ATTR,
	DRV_ERR_VENC_FRAME_PARAM,
	DRV_ERR_VENC_H264_SPLIT,
	DRV_ERR_VENC_H265_SPLIT,
	DRV_ERR_VENC_H264_INTRA_PRED,
	DRV_ERR_VENC_H265_SAO,
	DRV_ERR_VENC_H265_PRED_UNIT,
	DRV_ERR_VENC_SEARCH_WINDOW,
	DRV_ERR_VENC_SET_EXTERN_BUF,
	DRV_ERR_VENC_BUTT
} venc_recode_e_errtype;

/*the nalu type of H264E*/
typedef enum _h264e_nalu_type_e {
	H264E_NALU_BSLICE = 0, /*B SLICE types*/
	H264E_NALU_PSLICE = 1, /*P SLICE types*/
	H264E_NALU_ISLICE = 2, /*I SLICE types*/
	H264E_NALU_IDRSLICE = 5, /*IDR SLICE types*/
	H264E_NALU_SEI = 6, /*SEI types*/
	H264E_NALU_SPS = 7, /*SPS types*/
	H264E_NALU_PPS = 8, /*PPS types*/
	H264E_NALU_BUTT
} h264e_nalu_type_e;

/*the nalu type of H265E*/
typedef enum _h265e_nalu_type_e {
	H265E_NALU_BSLICE = 0, /*B SLICE types*/
	H265E_NALU_PSLICE = 1, /*P SLICE types*/
	H265E_NALU_ISLICE = 2, /*I SLICE types*/
	H265E_NALU_IDRSLICE = 19, /*IDR SLICE types*/
	H265E_NALU_VPS = 32, /*VPS types*/
	H265E_NALU_SPS = 33, /*SPS types*/
	H265E_NALU_PPS = 34, /*PPS types*/
	H265E_NALU_SEI = 39, /*SEI types*/

	H265E_NALU_BUTT
} h265e_nalu_type_e;

/*h265 decoding refresh type*/
typedef enum _H265E_REFERSH_TYPE_E {
	H265E_REFRESH_IDR = 0, /*Instantaneous decoding refresh picture*/
	H265E_REFRESH_CRA = 1, /*Clean random access picture*/
	H265E_REFRESH_BUTT
} h265e_refresh_type_e;

/*the reference type of H264E slice*/
typedef enum _h264e_refslice_type_e {
	H264E_REFSLICE_FOR_1X = 1, /*Reference slice for H264E_REF_MODE_1X*/
	H264E_REFSLICE_FOR_2X = 2, /*Reference slice for H264E_REF_MODE_2X*/
	H264E_REFSLICE_FOR_4X = 5, /*Reference slice for H264E_REF_MODE_4X*/
	H264E_REFSLICE_FOR_BUTT /* slice not for reference*/
} h264e_refslice_type_e;

/* the entropy mode of H264E */
typedef enum _h264e_entropy {
	H264E_ENTROPY_CAVLC = 0,
	H264E_ENTROPY_CABAC,
} h264e_entropy;

/* the profile of H264E */
typedef enum _h264e_profile {
	H264E_PROFILE_BASELINE = 0,
	H264E_PROFILE_MAIN,
	H264E_PROFILE_HIGH,
	H264E_PROFILE_BUTT
} h264e_profile;

/*the pack type of JPEGE*/
typedef enum _jpege_pack_type_e {
	JPEGE_PACK_ECS = 5, /*ECS types*/
	JPEGE_PACK_APP = 6, /*APP types*/
	JPEGE_PACK_VDO = 7, /*VDO types*/
	JPEGE_PACK_PIC = 8, /*PIC types*/
	JPEGE_PACK_DCF = 9, /*DCF types*/
	JPEGE_PACK_DCF_PIC = 10, /*DCF PIC types*/
	JPEGE_PACK_BUTT
} jpege_pack_type_e;

/*the marker type of JPEGE*/
typedef enum _jpege_marker_type_e {
	JPEGE_MARKER_SOI = 1,           /*SOI*/
	JPEGE_MARKER_DQT = 2,           /*DQT*/
	JPEGE_MARKER_DQT_MERGE = 3,     /*DQT containing multiple tables*/
	JPEGE_MARKER_DHT = 4,           /*DHT*/
	JPEGE_MARKER_DHT_MERGE = 5,     /*DHT containing multiple tables*/
	JPEGE_MARKER_DRI = 6,           /*DRI*/
	JPEGE_MARKER_DRI_OPT = 7,       /*DRI inserted only when restart interval is not 0*/
	JPEGE_MARKER_SOF0 = 8,          /*SOF0*/
	JPEGE_MARKER_JFIF = 9,          /*JFIF tags as APP0*/
	JPEGE_MARKER_FRAME_INDEX = 10,  /*frame index as APP9*/
	JPEGE_MARKER_ADOBE = 11,        /*ADOBE tags as APP14*/
	JPEGE_MARKER_USER_DATA = 12,    /*user data as APP15*/
	JPEGE_MARKER_BUTT
} jpege_marker_type_e;

typedef enum _jpege_format_e {
	JPEGE_FORMAT_DEFAULT = 0,       /* SOI, FRAME_INDEX, USER_DATA, DRI_OPT, DQT, DHT, SOF0 */
	JPEGE_FORMAT_TYPE_1 = 1,        /* SOI, JFIF, DQT_MERGE, SOF0, DHT_MERGE, DRI */
	JPEGE_FORMAT_CUSTOM = 0xFF,     /* custom marker order specified by JpegMarkerOrder */
} jpege_format_e;

/*the pack type of PRORES*/
typedef enum _prores_pack_type_e {
	PRORES_PACK_PIC = 1, /*PIC types*/
	PRORES_PACK_BUTT
} prores_pack_type_e;

/*the data type of VENC*/
typedef union _venc_data_type_u {
	h264e_nalu_type_e enH264EType; /* R; H264E NALU types*/
	jpege_pack_type_e enJPEGEType; /* R; JPEGE pack types*/
	h265e_nalu_type_e enH265EType; /* R; H264E NALU types*/
	prores_pack_type_e enPRORESType;
} venc_data_type_u;

/*the pack info of VENC*/
typedef struct _venc_pack_info_s {
	venc_data_type_u u32PackType; /* R; the pack type*/
	unsigned int u32PackOffset;
	unsigned int u32PackLength;
} venc_pack_info_s;

/*Defines a stream packet*/
typedef struct _venc_pack_s {
	uint64_t u64PhyAddr; /* R; the physics address of stream */

	unsigned char ATTRIBUTE *pu8Addr; /* R; the virtual address of stream */
#ifdef __arm__
	__u32 u32AddrPadding;
#endif
	unsigned int ATTRIBUTE u32Len; /* R; the length of stream */

	uint64_t u64PTS; /* R; PTS */
	uint64_t u64DTS; /* R; DTS */
	unsigned char bFrameEnd; /* R; frame end */

	venc_data_type_u DataType; /* R; the type of stream */
	unsigned int u32Offset; /* R; the offset between the Valid data and the start address*/
	unsigned int u32DataNum; /* R; the  stream packets num */
	venc_pack_info_s stPackInfo[8]; /* R; the stream packet Information */
	int releasFrameIdx;
    uint64_t u64CustomMapPhyAddr; /* R; the physics address of custom map addr */
    unsigned int u32AvgCtuQp; /* R; The average value of CTU QPs */
} venc_pack_s;

/*Defines the frame type and reference attributes of the H.264 frame skipping reference streams*/
typedef enum _h264e_ref_type_e {
	BASE_IDRSLICE = 0, /* the Idr frame at Base layer*/
	BASE_PSLICE_REFTOIDR, // the P frame at Base layer,
	// referenced by other frames at Base layer and reference to Idr frame
	BASE_PSLICE_REFBYBASE, /* the P frame at Base layer, referenced by other frames at Base layer */
	BASE_PSLICE_REFBYENHANCE, /* the P frame at Base layer, referenced by other frames at Enhance layer */
	ENHANCE_PSLICE_REFBYENHANCE, /* the P frame at Enhance layer, referenced by other frames at Enhance layer */
	ENHANCE_PSLICE_NOTFORREF, /* the P frame at Enhance layer ,not referenced */
	ENHANCE_PSLICE_BUTT
} h264e_ref_type_e;

typedef enum _h264e_ref_type_e H265E_REF_TYPE_E;

/*Defines the features of an H.264 stream*/
typedef struct _venc_stream_info_h264_s {
	unsigned int u32PicBytesNum; /* R; the coded picture stream byte number */
	unsigned int u32Inter16x16MbNum; /* R; the inter16x16 macroblock num */
	unsigned int u32Inter8x8MbNum; /* R; the inter8x8 macroblock num */
	unsigned int u32Intra16MbNum; /* R; the intra16x16 macroblock num */
	unsigned int u32Intra8MbNum; /* R; the intra8x8 macroblock num */
	unsigned int u32Intra4MbNum; /* R; the inter4x4 macroblock num */

	h264e_ref_type_e enRefType; /* R; Type of encoded frames in advanced frame skipping reference mode */
	unsigned int u32UpdateAttrCnt; // R; Number of times that channel attributes
	// or parameters (including RC parameters) are set
	unsigned int u32StartQp; /* R; the start Qp of encoded frames*/
	unsigned int u32MeanQp; /* R; the mean Qp of encoded frames*/
	unsigned char bPSkip;
} venc_stream_info_h264_s;

/*Defines the features of an H.265 stream*/
typedef struct _venc_stream_info_h265_s {
	unsigned int u32PicBytesNum; /* R; the coded picture stream byte number */
	unsigned int u32Inter64x64CuNum; /* R; the inter64x64 cu num  */
	unsigned int u32Inter32x32CuNum; /* R; the inter32x32 cu num  */
	unsigned int u32Inter16x16CuNum; /* R; the inter16x16 cu num  */
	unsigned int u32Inter8x8CuNum; /* R; the inter8x8   cu num  */
	unsigned int u32Intra32x32CuNum; /* R; the Intra32x32 cu num  */
	unsigned int u32Intra16x16CuNum; /* R; the Intra16x16 cu num  */
	unsigned int u32Intra8x8CuNum; /* R; the Intra8x8   cu num  */
	unsigned int u32Intra4x4CuNum; /* R; the Intra4x4   cu num  */
	H265E_REF_TYPE_E enRefType; /* R; Type of encoded frames in advanced frame skipping reference mode*/

	unsigned int u32UpdateAttrCnt; // R; Number of times that channel
	// attributes/parameters (including RC parameters) are set
	unsigned int u32StartQp; /* R; the start Qp of encoded frames*/
	unsigned int u32MeanQp; /* R; the mean Qp of encoded frames*/
	unsigned char bPSkip;
} venc_stream_info_h265_s;

/* the sse info*/
typedef struct _venc_sse_info_s {
	unsigned char bSSEEn; /* RW; Range:[0,1]; Region SSE enable */
	unsigned int u32SSEVal; /* R; Region SSE value */
} venc_sse_info_s;

/* the advance information of the h264e */
typedef struct _venc_stream_advance_info_h264_s {
	unsigned int u32ResidualBitNum; /* R; the residual num */
	unsigned int u32HeadBitNum; /* R; the head bit num */
	unsigned int u32MadiVal; /* R; the madi value */
	unsigned int u32MadpVal; /* R; the madp value */
	double dPSNRVal; /* R; the PSNR value */
	unsigned int u32MseLcuCnt; /* R; the lcu cnt of the mse */
	unsigned int u32MseSum; /* R; the sum of the mse */
	venc_sse_info_s stSSEInfo[8]; /* R; the information of the sse */
	unsigned int u32QpHstgrm[52]; /* R; the Qp histogram value */
	unsigned int u32MoveScene16x16Num; /* R; the 16x16 cu num of the move scene*/
	unsigned int u32MoveSceneBits; /* R; the stream bit num of the move scene */
} venc_stream_advance_info_h264_s;

/* the advance information of the Jpege */
typedef struct _venc_stream_advance_info_jpeg_s {
	// unsigned int u32Reserved;
} venc_stream_advance_info_jpeg_s;

/* the advance information of the Prores */
typedef struct _venc_stream_advance_info_prores_s {
	// unsigned int u32Reserved;
} venc_stream_advance_info_prores_s;

/* the advance information of the h265e */
typedef struct _venc_stream_advance_info_h265_s {
	unsigned int u32ResidualBitNum; /* R; the residual num */
	unsigned int u32HeadBitNum; /* R; the head bit num */
	unsigned int u32MadiVal; /* R; the madi value */
	unsigned int u32MadpVal; /* R; the madp value */
	double dPSNRVal; /* R; the PSNR value */
	unsigned int u32MseLcuCnt; /* R; the lcu cnt of the mse */
	unsigned int u32MseSum; /* R; the sum of the mse */
	venc_sse_info_s stSSEInfo[8]; /* R; the information of the sse */
	unsigned int u32QpHstgrm[52]; /* R; the Qp histogram value */
	unsigned int u32MoveScene32x32Num; /* R; the 32x32 cu num of the move scene*/
	unsigned int u32MoveSceneBits; /* R; the stream bit num of the move scene */
} venc_stream_advance_info_h265_s;

/*Defines the features of an jpege stream*/
typedef struct _venc_stream_info_prores_s {
	unsigned int u32PicBytesNum;
	unsigned int u32UpdateAttrCnt;
} venc_stream_info_prores_s;

/*Defines the features of an jpege stream*/
typedef struct _venc_stream_info_jpeg_s {
	unsigned int u32PicBytesNum; /* R; the coded picture stream byte number */
	unsigned int u32UpdateAttrCnt; // R; Number of times that channel attributes
	// or parameters (including RC parameters) are set
	unsigned int u32Qfactor; /* R; image quality */
} venc_stream_info_jpeg_s;

/**
 * @brief Define the attributes of encoded bitstream
 *
 */
typedef struct _venc_stream_s {
	venc_pack_s ATTRIBUTE *pstPack;	///< Encoded bitstream packet
#ifdef __arm__
	__u32 u32stPackPadding;
#endif
	unsigned int ATTRIBUTE u32PackCount; ///< Number of bitstream packets
	unsigned int u32Seq;	///< TODO VENC

	union {
		venc_stream_info_h264_s stH264Info;		///< TODO VENC
		venc_stream_info_jpeg_s stJpegInfo;		///< The information of JPEG bitstream
		venc_stream_info_h265_s stH265Info;		///< TODO VENC
		venc_stream_info_prores_s stProresInfo;	///< TODO VENC
	};

	union {
		venc_stream_advance_info_h264_s
		stAdvanceH264Info;	///< TODO VENC
		venc_stream_advance_info_jpeg_s
		stAdvanceJpegInfo;	///< TODO VENC
		venc_stream_advance_info_h265_s
		stAdvanceH265Info;	///< TODO VENC
		venc_stream_advance_info_prores_s
		stAdvanceProresInfo;///< TODO VENC
	};
} venc_stream_s;

typedef struct _venc_stream_info_s {
	H265E_REF_TYPE_E enRefType; /*Type of encoded frames in advanced frame skipping reference mode */

	unsigned int u32PicBytesNum; /* the coded picture stream byte number */
	unsigned int u32PicCnt; /*Number of times that channel attributes or parameters (including RC parameters) are set */
	unsigned int u32StartQp; /*the start Qp of encoded frames*/
	unsigned int u32MeanQp; /*the mean Qp of encoded frames*/
	unsigned char bPSkip;

	unsigned int u32ResidualBitNum; // residual
	unsigned int u32HeadBitNum; // head information
	unsigned int u32MadiVal; // madi
	unsigned int u32MadpVal; // madp
	unsigned int u32MseSum; /* Sum of MSE value */
	unsigned int u32MseLcuCnt; /* Sum of LCU number */
	double dPSNRVal; // PSNR
} venc_stream_info_s;

/*the size of array is 2,that is the maximum*/
typedef struct _venc_mpf_cfg_s {
	unsigned char u8LargeThumbNailNum; /* RW; Range:[0,2]; the large thumbnail pic num of the MPF */
	size_s astLargeThumbNailSize[2]; /* RW; The resolution of large ThumbNail*/
} venc_mpf_cfg_s;

typedef enum _venc_pic_receive_mode_e {
	VENC_PIC_RECEIVE_SINGLE = 0,
	VENC_PIC_RECEIVE_MULTI,
	VENC_PIC_RECEIVE_BUTT
} venc_pic_receive_mode_e;

/**
 * @brief Define the attributes of JPEG Encoder.
 *
 */
typedef struct _venc_attr_jpeg_s {
	unsigned char bSupportDCF;		///< TODO VENC
	venc_mpf_cfg_s stMPFCfg;	///< TODO VENC
	venc_pic_receive_mode_e enReceiveMode;	///< TODO VENC
} venc_attr_jpeg_s;

/*the attribute of h264e*/
typedef struct _venc_attr_h264_s {
	unsigned char bRcnRefShareBuf; /* RW; Range:[0, 1]; Whether to enable the Share Buf of Rcn and Ref .*/
	unsigned char bSingleLumaBuf; /* Use single luma buffer*/
	// reserved
} venc_attr_h264_s;

/*the attribute of h265e*/
typedef struct _venc_attr_h265_s {
	unsigned char bRcnRefShareBuf; /* RW; Range:[0, 1]; Whether to enable the Share Buf of Rcn and Ref .*/
	// reserved
} venc_attr_h265_s;

/*the frame rate of PRORES*/
typedef enum _prores_framerate {
	PRORES_FR_UNKNOWN = 0,
	PRORES_FR_23_976,
	PRORES_FR_24,
	PRORES_FR_25,
	PRORES_FR_29_97,
	PRORES_FR_30,
	PRORES_FR_50,
	PRORES_FR_59_94,
	PRORES_FR_60,
	PRORES_FR_100,
	PRORES_FR_119_88,
	PRORES_FR_120,
	PRORES_FR_BUTT
} prores_framerate;

/*the aspect ratio of PRORES*/
typedef enum _prores_aspect_ratio {
	prores_aspect_ratio_UNKNOWN = 0,
	prores_aspect_ratio_SQUARE,
	prores_aspect_ratio_4_3,
	prores_aspect_ratio_16_9,
	prores_aspect_ratio_BUTT
} prores_aspect_ratio;

/*the attribute of PRORES*/
typedef struct _venc_attr_prores_s {
	char cIdentifier[4];
	prores_framerate enFrameRateCode;
	prores_aspect_ratio enAspectRatio;
} venc_attr_prores_s;

/**
 * @brief   This is a special enumeration type for defining GOP structure presets.
 */
typedef enum _venc_gop_preset_s {
    GOP_PRESET_IDX_CUSTOM_GOP       =  0, /**< User defined GOP structure */
    GOP_PRESET_IDX_ALL_I            =  1, /**< All Intra, gopsize = 1 */
    GOP_PRESET_IDX_IPP              =  2, /**< Consecutive P, cyclic gopsize = 1 */
    GOP_PRESET_IDX_IBBB             =  3, /**< Consecutive B, cyclic gopsize = 1 */
    GOP_PRESET_IDX_IBPBP            =  4, /**< gopsize = 2 */
    GOP_PRESET_IDX_IBBBP            =  5, /**< gopsize = 4 */
    GOP_PRESET_IDX_IPPPP            =  6, /**< Consecutive P, cyclic gopsize = 4 */
    GOP_PRESET_IDX_IBBBB            =  7, /**< Consecutive B, cyclic gopsize = 4 */
    GOP_PRESET_IDX_RA_IB            =  8, /**< Random Access, cyclic gopsize = 8 */
    GOP_PRESET_IDX_IPP_SINGLE       =  9, /**< Consecutive P, cyclic gopsize = 1, with single reference */
} venc_gop_preset_s;

/**
 * @brief   This is a enumeration type for defining encode mode.
 */
typedef enum _venc_encode_mode {
    VENC_MODE_RECOMMEND = 1,
    VENC_MODE_BOOST     = 2,
    VENC_MODE_FAST      = 3,
} venc_encode_mode;

/**
* @brief    This is a data structure for custom GOP parameters of the given picture.
*/
typedef struct _venc_custom_gop_pic_param {
    int s32PicType;      /**< A picture type of Nth picture in the custom GOP */
    int s32PocOffset;    /**< A POC of Nth picture in the custom GOP */
    int s32PicQp;        /**< A quantization parameter of Nth picture in the custom GOP */
    int s32UseMultiRefP; /**< A flag to use multi reference picture for P picture. It is valid only if PIC_TYPE is P */
    int s32RefPocL0;     /**< A POC of reference L0 of Nth picture in the custom GOP */
    int s32RefPocL1;     /**< A POC of reference L1 of Nth picture in the custom GOP */
    int s32TemporalId;   /**< A temporal ID of Nth picture in the custom GOP */
} venc_custom_gop_pic_param;

/**
* @brief    This is a data structure for custom GOP parameters.
*/
typedef struct _venc_custom_gop_param {
    int s32CustomGopSize;                       /**< The size of custom GOP (1 ~ 8) */
    venc_custom_gop_pic_param picParam[8]; /**< Picture parameters of Nth picture in custom GOP */
} venc_custom_gop_param;

/**
* @brief    This is a data structure for custom GOP parameters.
*/
typedef struct _venc_gop_ex_attr_s {
    unsigned int u32GopPreset;		        /**< gop preset for video encoder(h264/h265) */
    venc_custom_gop_param gopParam;     /**< only valid when u32GopPreset is GOP_PRESET_IDX_CUSTOM_GOP */
} venc_gop_ex_attr_s;

/**
 * @brief Define the attributes of the venc.
 *
 */
typedef struct _venc_attr_s {
	payload_type_e enType;		///< the type of payload

	unsigned int u32MaxPicWidth;		///< maximum width of a picture to be encoded
	unsigned int u32MaxPicHeight;	///< maximum height of a picture to be encoded

	uint64_t u64ExternalBufAddr;
	unsigned int u32BufSize;			///< size of encoded bitstream buffer
	unsigned int u32Profile;			///< profie setting for video encoder(h264/h265)
	unsigned char bByFrame;			///< mode of collecting encoded bitstream\n
								///< 1: frame-based\n 0: packet-based
	unsigned int u32PicWidth;		///< width of a picture to be encoded
	unsigned int u32PicHeight;		///< height of a picture to be encoded
	unsigned char bSingleCore;		///< Use single HW core
	unsigned char bEsBufQueueEn;		///< Use es buffer queue
	unsigned char bIsoSendFrmEn;		///< Isolating SendFrame/GetStream pairing
	pixel_format_e enPixelFormat;
	rotation_e enRotation;      ///< rotate angle option, (0 ~ 3)
	mirror_type_e enMirrorDirextion; ///< representing the mirroring direction, (0 ~ 3)
	unsigned int u32CmdQueueDepth;   ///< venc command queue depth (1 ~ 4)(for h264/h265)
	venc_encode_mode enEncMode; ///< venc encode mode (1 ~ 3)
	union {
		venc_attr_h264_s stAttrH264e;	///< TODO VENC
		venc_attr_h265_s stAttrH265e;	///< TODO VENC
		venc_attr_jpeg_s stAttrJpege;	///< the attibute of JPEG encoder
		venc_attr_prores_s stAttrProres;///< TODO VENC
	};
} venc_attr_s;

/* the gop mode */
typedef enum _venc_gop_mode_e {
	VENC_GOPMODE_NORMALP = 0, /* NORMALP */
	VENC_GOPMODE_DUALP = 1, /* DUALP */
	VENC_GOPMODE_SMARTP = 2, /* SMARTP */
	VENC_GOPMODE_ADVSMARTP = 3, /* ADVSMARTP */
	VENC_GOPMODE_BIPREDB = 4, /* BIPREDB */
	VENC_GOPMODE_LOWDELAYB = 5, /* LOWDELAYB */
	VENC_GOPMODE_BUTT,
} venc_gop_mode_e;

/* the attribute of the normalp*/
typedef struct _venc_gop_normalp_s {
	int s32IPQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and I frame */
} venc_gop_normalp_s;

/* the attribute of the dualp*/
typedef struct _venc_gop_dualp_s {
	unsigned int u32SPInterval; /* RW; Range:[0, 1)U(1, U32Gop -1]; Interval of the special P frames */
	int s32SPQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and special P frame */
	int s32IPQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and I frame */
} venc_gop_dualp_s;

/* the attribute of the smartp*/
typedef struct _venc_gop_smartp_s {
	unsigned int u32BgInterval; /* RW; Range:[U32Gop, 65536] ;Interval of the long-term reference frame*/
	int s32BgQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and Bg frame */
	int s32ViQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and virtual I  frame */
} venc_gop_smartp_s;

/* the attribute of the advsmartp*/
typedef struct _venc_gop_advsmartp_s {
	unsigned int u32BgInterval; /* RW; Range:[U32Gop, 65536] ;Interval of the long-term reference frame*/
	int s32BgQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and Bg frame */
	int s32ViQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and virtual I  frame */
} venc_gop_advsmartp_s;

/* the attribute of the bipredb*/
typedef struct _venc_gop_bipredb_s {
	unsigned int u32BFrmNum; /* RW; Range:[1,3]; Number of B frames */
	int s32BQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and B frame */
	int s32IPQpDelta; /* RW; Range:[-10,30]; QP variance between P frame and I frame */
} venc_gop_bipredb_s;

/* the attribute of the gop*/
typedef struct _venc_gop_attr_s {
	venc_gop_mode_e enGopMode; /* RW; Encoding GOP type */
	union {
		venc_gop_normalp_s stNormalP; /*attributes of normal P*/
		venc_gop_dualp_s stDualP; /*attributes of dual   P*/
		venc_gop_smartp_s stSmartP; /*attributes of Smart P*/
		venc_gop_advsmartp_s stAdvSmartP; /*attributes of AdvSmart P*/
		venc_gop_bipredb_s stBipredB; /*attributes of b */
	};

} venc_gop_attr_s;

/**
 * @brief Define the attributes of the venc channel.
 *
 */
typedef struct _venc_chn_attr_s {
	venc_attr_s stVencAttr;		///< The attribute of video encoder
	venc_rc_attr_s stRcAttr;	///< The attribute of bitrate control
	venc_gop_attr_s stGopAttr;	///< for normal/smart gop
	venc_gop_ex_attr_s stGopExAttr; ///< for gop prest setting
} venc_chn_attr_s;

/* the param of receive picture */
typedef struct _venc_recv_pic_param_s {
	int s32RecvPicNum; // RW; Range:[-1,0)U(0 2147483647]; Number of frames
	// received and encoded by the encoding channel
} venc_recv_pic_param_s;

/**
 * @brief Define the current channel status of encoder
 *
 */
typedef struct _venc_chn_status_s {
	unsigned int u32LeftPics;				///< Number of frames left to be encoded TODO VENC
	unsigned int u32LeftStreamBytes;			///< Number of stream bytes left in the stream buffer TODO VENC
	unsigned int u32LeftStreamFrames;		///< Number of encoded frame in the stream buffer TODO VENC
	unsigned int u32CurPacks;				///< Number of packets in current frame
	unsigned int u32LeftRecvPics;			///< Number of frames to be received TODO VENC
	unsigned int u32LeftEncPics;				///< Number of frames to be encoded. TODO VENC
	unsigned char bJpegSnapEnd;				///< if the process of JPEG captureThe is finished. TODO VENC
	venc_stream_info_s stVencStrmInfo;	///< the stream information of encoder TODO VENC
} venc_chn_status_s;

/* the param of the h264e slice split*/
typedef struct _venc_h264_slice_split_s {
	unsigned char bSplitEnable;
	// RW; Range:[0,1]; slice split enable, 1:enable, 0:disable, default value:0
	unsigned int u32MbLineNum;
	// RW; Range:[1,(Picture height + 15)/16]; this value presents the mb line number of one slice
} venc_h264_slice_split_s;

/* the param of the h264e intra pred*/
typedef struct _venc_h264_intra_pred_s {
	unsigned int constrained_intra_pred_flag;
	// RW; Range:[0,1];default: 0, see the H.264 protocol for the meaning
} venc_h264_intra_pred_s;

/* the param of the h264e trans*/
typedef struct _venc_h264_trans_s {
	unsigned int u32IntraTransMode; // RW; Range:[0,2]; Conversion mode for
	// intra-prediction,0: trans4x4, trans8x8; 1: trans4x4, 2: trans8x8
	unsigned int u32InterTransMode; // RW; Range:[0,2]; Conversion mode for
	// inter-prediction,0: trans4x4, trans8x8; 1: trans4x4, 2: trans8x8

	unsigned char bScalingListValid; /* RW; Range:[0,1]; enable Scaling,default: 0  */
	unsigned char InterScalingList8X8[64]; /* RW; Range:[1,255]; A quantization table for 8x8 inter-prediction*/
	unsigned char IntraScalingList8X8[64]; /* RW; Range:[1,255]; A quantization table for 8x8 intra-prediction*/

	int chroma_qp_index_offset; /* RW; Range:[-12,12];default value: 0, see the H.264 protocol for the meaning*/
} venc_h264_trans_s;

/* the param of the h264e entropy*/
typedef struct _venc_h264_entropy_s {
	unsigned int u32EntropyEncModeI; /* RW; Range:[0,1]; Entropy encoding mode for the I frame, 0:cavlc, 1:cabac */
	unsigned int u32EntropyEncModeP; /* RW; Range:[0,1]; Entropy encoding mode for the P frame, 0:cavlc, 1:cabac */
	unsigned int u32EntropyEncModeB; /* RW; Range:[0,1]; Entropy encoding mode for the B frame, 0:cavlc, 1:cabac */
	unsigned int cabac_init_idc; /* RW; Range:[0,2]; see the H.264 protocol for the meaning */
} venc_h264_entropy_s;

/* the config of the h264e poc*/
typedef struct _venc_h264_poc_s {
	unsigned int pic_order_cnt_type; /* RW; Range:[0,2]; see the H.264 protocol for the meaning */

} venc_h264_poc_s;

/* the param of the h264e deblocking*/
typedef struct _venc_h264_dblk_s {
	unsigned int disable_deblocking_filter_idc; /*  RW; Range:[0,2]; see the H.264 protocol for the meaning */
	int slice_alpha_c0_offset_div2; /*  RW; Range:[-6,+6]; see the H.264 protocol for the meaning */
	int slice_beta_offset_div2; /*  RW; Range:[-6,+6]; see the H.264 protocol for the meaning */
} venc_h264_dblk_s;

/* the param of the h264e vui timing info*/
typedef struct _VENC_H264_VUI_TIME_INFO_S {
	unsigned char timing_info_present_flag; /* RW; Range:[0,1]; If 1, timing info belows will be encoded into vui.*/
	unsigned char fixed_frame_rate_flag; /* RW; Range:[0,1]; see the H.264 protocol for the meaning. */
	unsigned int num_units_in_tick; /* RW; Range:(0,4294967295]; see the H.264 protocol for the meaning */
	unsigned int time_scale; /* RW; Range:(0,4294967295]; see the H.264 protocol for the meaning */
} venc_vui_h264_time_info_s;

/* the param of the vui aspct ratio*/
typedef struct _venc_vui_aspect_ratio_s {
	unsigned char aspect_ratio_info_present_flag;
	// RW; Range:[0,1]; If 1, aspectratio info belows will be encoded into vui
	unsigned char aspect_ratio_idc; /* RW; Range:[0,255]; 17~254 is reserved,see the protocol for the meaning.*/
	unsigned char overscan_info_present_flag; // RW; Range:[0,1]; If 1, oversacan info belows will be encoded into vui.
	unsigned char overscan_appropriate_flag; /* RW; Range:[0,1]; see the protocol for the meaning. */
	unsigned short sar_width; /* RW; Range:(0, 65535]; see the protocol for the meaning. */
	unsigned short sar_height; // RW; Range:(0, 65535]; see the protocol for the meaning.
	// notes: sar_width and sar_height  shall  be  relatively prime.
} venc_vui_aspect_ratio_s;

/* the param of the vui video signal*/
typedef struct _venc_vui_video_signal_s {
	unsigned char video_signal_type_present_flag; /* RW; Range:[0,1]; If 1, video singnal info will be encoded into vui. */
	unsigned char video_format; /* RW; H.264e Range:[0,7], H.265e Range:[0,5]; see the protocol for the meaning. */
	unsigned char video_full_range_flag; /* RW; Range: {0,1}; see the protocol for the meaning.*/
	unsigned char colour_description_present_flag; /* RO; Range: {0,1}; see the protocol for the meaning.*/
	unsigned char colour_primaries; /* RO; Range: [0,255]; see the protocol for the meaning.*/
	unsigned char transfer_characteristics; /* RO; Range: [0,255]; see the protocol for the meaning. */
	unsigned char matrix_coefficients; /* RO; Range:[0,255]; see the protocol for themeaning. */
} venc_vui_video_signal_s;

/* the param of the vui video signal*/
typedef struct _venc_vui_bitstream_restric_s {
	unsigned char bitstream_restriction_flag; /* RW; Range: {0,1}; see the protocol for the meaning.*/
} venc_vui_bitstream_restric_s;

/* the param of the h264e vui */
typedef struct _venc_h264_vui_s {
	venc_vui_aspect_ratio_s stVuiAspectRatio;
	venc_vui_h264_time_info_s stVuiTimeInfo;
	venc_vui_video_signal_s stVuiVideoSignal;
	venc_vui_bitstream_restric_s stVuiBitstreamRestric;
} venc_h264_vui_s;

/* the param of the h265e vui timing info*/
typedef struct _venc_vui_h265_time_info_s {
	unsigned int timing_info_present_flag; /* RW; Range:[0,1]; If 1, timing info belows will be encoded into vui.*/
	unsigned int num_units_in_tick; /* RW; Range:[0,4294967295]; see the H.265 protocol for the meaning. */
	unsigned int time_scale; /* RW; Range:(0,4294967295]; see the H.265 protocol for the meaning */
	unsigned int num_ticks_poc_diff_one_minus1; /* RW; Range:(0,4294967294]; see the H.265 protocol for the meaning */
} venc_vui_h265_time_info_s;

/* the param of the h265e vui */
typedef struct _venc_h265_vui_s {
	venc_vui_aspect_ratio_s stVuiAspectRatio;
	venc_vui_h265_time_info_s stVuiTimeInfo;
	venc_vui_video_signal_s stVuiVideoSignal;
	venc_vui_bitstream_restric_s stVuiBitstreamRestric;
} venc_h265_vui_s;

/* the param of the jpege */
typedef struct _venc_jpeg_param_s {
	unsigned int u32Qfactor; /* RW; Range:[1,99]; Qfactor value, 50 = user q-table */
	unsigned char u8YQt[64]; /* RW; Range:[1, 255]; Y quantization table */
	unsigned char u8CbQt[64]; /* RW; Range:[1, 255]; Cb quantization table */
	unsigned char u8CrQt[64]; /* RW; Range:[1, 255]; Cr quantization table */
	unsigned int u32MCUPerECS; // RW; Range:[0, (picwidth + 15) >> 4 x (picheight + 15) >> 4 x 2]; MCU number of one ECS
} venc_jpeg_param_s;

/* the param of the mjpege */
typedef struct _venc_mjpeg_param_s {
	unsigned char u8YQt[64]; /* RW; Range:[1, 255]; Y quantization table */
	unsigned char u8CbQt[64]; /* RW; Range:[1, 255]; Cb quantization table */
	unsigned char u8CrQt[64]; /* RW; Range:[1, 255]; Cr quantization table */
	unsigned int u32MCUPerECS; // RW; Range:[0, (picwidth + 15) >> 4 x (picheight + 15) >> 4 x 2]; MCU number of one ECS
} venc_mjpeg_param_s;

/* the param of the ProRes */
typedef struct _venc_prores_param_s {
	unsigned char u8LumaQt[64]; /* RW; Range:[1, 255]; Luma quantization table */
	unsigned char u8ChromaQt[64]; /* RW; Range:[1, 255]; Chroma quantization table */
	char encoder_identifier[4]; // RW:  identifies the encoder vendor or product that generated the compressed frame
} venc_prores_param_s;

/* the attribute of the roi */
typedef struct _venc_roi_attr_s {
	unsigned int u32Index; /* RW; Range:[0, 7]; Index of an ROI. The system supports indexes ranging from 0 to 7 */
	unsigned char bEnable; /* RW; Range:[0, 1]; Whether to enable this ROI */
	unsigned char bAbsQp; // RW; Range:[0, 1]; QP mode of an ROI. 0: relative QP. 1: absolute QP
	int s32Qp; /* RW; Range:[-51, 51]; QP value,only relative mode can QP value less than 0. */
	rect_s stRect; /* RW;Region of an ROI*/
} venc_roi_attr_s;

/* ROI struct */
typedef struct _venc_roi_attr_ex_s {
	unsigned int u32Index; /* Index of an ROI. The system supports indexes ranging from 0 to 7 */
	unsigned char bEnable[3]; // Subscript of array   0: I Frame; 1: P/B Frame; 2: VI Frame; other params are the same
	unsigned char bAbsQp[3]; // QP mode of an ROI.0: relative QP. 1: absolute QP
	int s32Qp[3]; /* QP value. */
	rect_s stRect[3]; /* Region of an ROI*/
} venc_roi_attr_ex_s;

/* the param of the roibg frame rate */
typedef struct _venc_roibg_frame_rate_s {
	int s32SrcFrmRate; /* RW; Range:[-1,0)U(0 2147483647];Source frame rate of a non-ROI*/
	int s32DstFrmRate; /* RW; Range:[-1, s32SrcFrmRate]; Target frame rate of a non-ROI  */
} venc_roibg_frame_rate_s;

/* the param of the roibg frame rate */
typedef struct _venc_ref_param_s {
	unsigned int u32Base; /* RW; Range:[0,4294967295]; Base layer period*/
	unsigned int u32Enhance; /* RW; Range:[0,255]; Enhance layer period*/
	unsigned char bEnablePred; // RW; Range:[0, 1]; Whether some frames at the base
	// layer are referenced by other frames at the base layer. When bEnablePred is 0,
	// all frames at the base layer reference IDR frames.
} venc_ref_param_s;

/* Jpeg snap mode */
typedef enum _venc_jpeg_encode_mode_e {
	JPEG_ENCODE_ALL = 0, /* Jpeg channel snap all the pictures when started. */
	JPEG_ENCODE_SNAP = 1, /* Jpeg channel snap the flashed pictures when started. */
	JPEG_ENCODE_BUTT,
} venc_jpeg_encode_mode_e;

/* the information of the stream */
typedef struct _venc_stream_buf_info_s {
	uint64_t u64PhyAddr[MAX_TILE_NUM]; /* R; Start physical address for a stream buffer */

	void ATTRIBUTE *pUserAddr[MAX_TILE_NUM]; /* R; Start virtual address for a stream buffer */
	uint64_t ATTRIBUTE u64BufSize[MAX_TILE_NUM]; /* R; Stream buffer size */
} venc_stream_buf_info_s;

/* the param of the h265e slice split */
typedef struct _venc_h265_slice_split_s {
	unsigned char bSplitEnable; // RW; Range:[0,1]; slice split enable, 1:enable,
	// 0:disable, default value:0
	unsigned int u32LcuLineNum; // RW; Range:(Picture height + lcu size minus one)/lcu
	// size;this value presents lcu line number
} venc_h265_slice_split_s;

/* the param of the h265e pu */
typedef struct _venc_h265_pu_s {
	unsigned int constrained_intra_pred_flag; /* RW; Range:[0,1]; see the H.265 protocol for the meaning. */
	unsigned int strong_intra_smoothing_enabled_flag; /* RW; Range:[0,1]; see the H.265 protocol for the meaning. */
} venc_h265_pu_s;

/* the param of the h265e trans */
typedef struct _venc_h265_trans_s {
	int cb_qp_offset; /* RW; Range:[-12,12]; see the H.265 protocol for the meaning. */
	int cr_qp_offset; /* RW; Range:[-12,12]; see the H.265 protocol for the meaning. */

	unsigned char bScalingListEnabled; /* RW; Range:[0,1]; If 1, specifies that a scaling list is used.*/

	unsigned char bScalingListTu4Valid; /* RW; Range:[0,1]; If 1, ScalingList4X4 belows will be encoded.*/
	unsigned char InterScalingList4X4[2][16]; /* RW; Range:[1,255]; Scaling List for inter 4X4 block.*/
	unsigned char IntraScalingList4X4[2][16]; /* RW; Range:[1,255]; Scaling List for intra 4X4 block.*/

	unsigned char bScalingListTu8Valid; /* RW; Range:[0,1]; If 1, ScalingList8X8 belows will be encoded.*/
	unsigned char InterScalingList8X8[2][64]; /* RW; Range:[1,255]; Scaling List for inter 8X8 block.*/
	unsigned char IntraScalingList8X8[2][64]; /* RW; Range:[1,255]; Scaling List for intra 8X8 block.*/

	unsigned char bScalingListTu16Valid; /* RW; Range:[0,1]; If 1, ScalingList16X16 belows will be encoded.*/
	unsigned char InterScalingList16X16[2][64]; /* RW; Range:[1,255]; Scaling List for inter 16X16 block..*/
	unsigned char IntraScalingList16X16[2][64]; /* RW; Range:[1,255]; Scaling List for inter 16X16 block.*/

	unsigned char bScalingListTU32Valid; /* RW; Range:[0,1]; If 1, ScalingList32X32 belows will be encoded.*/
	unsigned char InterScalingList32X32[64]; /* RW; Range:[1,255]; Scaling List for inter 32X32 block..*/
	unsigned char IntraScalingList32X32[64]; /* RW; Range:[1,255]; Scaling List for inter 32X32 block.*/

} venc_h265_trans_s;

/* the param of the h265e entroy */
typedef struct _venc_h265_entropy_s {
	unsigned int cabac_init_flag; /* RW; Range:[0,1]; see the H.265 protocol for the meaning. */
} venc_h265_entropy_s;

/* the param of the h265e deblocking */
typedef struct _venc_h265_dblk_s {
	unsigned int slice_deblocking_filter_disabled_flag; /* RW; Range:[0,1]; see the H.265 protocol for the meaning. */
	int slice_beta_offset_div2; /* RW; Range:[-6,6]; see the H.265 protocol for the meaning. */
	int slice_tc_offset_div2; /* RW; Range:[-6,6]; see the H.265 protocol for the meaning. */
} venc_h265_dblk_s;

/* the param of the h265e sao */
typedef struct _venc_h265_sao_s {
	unsigned int slice_sao_luma_flag; // RW; Range:[0,1]; Indicates whether SAO filtering is
	// performed on the luminance component of the current slice.
	unsigned int slice_sao_chroma_flag; // RW; Range:[0,1]; Indicates whether SAO filtering
	// is performed on the chrominance component of the current slice
} venc_h265_sao_s;

/* venc mode type */
typedef enum _venc_intra_refresh_mode_e {
	INTRA_REFRESH_ROW = 0, /* Line mode */
	INTRA_REFRESH_COLUMN, /* Column mode */
	INTRA_REFRESH_BUTT
} venc_intra_refresh_mode_e;

/* the param of the intra refresh */
typedef struct _venc_intra_refresh_s {
	unsigned char bRefreshEnable; // RW; Range:[0,1]; intra refresh enable, 1:enable,
    // 0:disable, default value:0
	venc_intra_refresh_mode_e
	enIntraRefreshMode; /*RW;Range:INTRA_REFRESH_ROW or INTRA_REFRESH_COLUMN*/
	unsigned int u32RefreshNum; /* RW; Number of rows/column to be refreshed during each I macroblock refresh*/
	unsigned int u32ReqIQp; /* RW; Range:[0,51]; QP value of the I frame*/
} venc_intra_refresh_s;

/* venc mode type */
typedef enum _venc_modtype_e {
	MODTYPE_VENC = 1, /* VENC */
	MODTYPE_H264E, /* H264e */
	MODTYPE_H265E, /* H265e */
	MODTYPE_JPEGE, /* Jpege */
	MODTYPE_RC, /* Rc */
	MODTYPE_BUTT
} venc_modtype_e;

/* the param of the h264e mod */
typedef struct _venc_mod_h264e_s {
	unsigned int u32OneStreamBuffer; /* RW; Range:[0,1]; one stream buffer*/
	unsigned int u32H264eMiniBufMode; /* RW; Range:[0,1]; H264e MiniBufMode*/
	unsigned int u32H264ePowerSaveEn; /* RW; Range:[0,1]; H264e PowerSaveEn*/
	vb_source_e enH264eVBSource; /* RW; Range:VB_SOURCE_PRIVATE,VB_SOURCE_USER; H264e VBSource*/
	unsigned char bQpHstgrmEn; /* RW; Range:[0,1]*/
	unsigned int u32UserDataMaxLen;   /* RW; Range:[0,65536]; maximum number of bytes of a user data segment */
	unsigned char bSingleEsBuf;       /* RW; Range[0,1]; use single output es buffer in n-way encode */
	unsigned int u32SingleEsBufSize;  /* RW; size of single es buffer in n-way encode */
} venc_mod_h264e_s;

/* the param of the h265e mod */
typedef struct _venc_mod_h265e_s {
	unsigned int u32OneStreamBuffer; /* RW; Range:[0,1]; one stream buffer*/
	unsigned int u32H265eMiniBufMode; /* RW; Range:[0,1]; H265e MiniBufMode*/
	unsigned int u32H265ePowerSaveEn; /* RW; Range:[0,2]; H265e PowerSaveEn*/
	vb_source_e enH265eVBSource; /* RW; Range:VB_SOURCE_PRIVATE,VB_SOURCE_USER; H265e VBSource*/
	unsigned char bQpHstgrmEn; /* RW; Range:[0,1]*/
	unsigned int u32UserDataMaxLen;   /* RW; Range:[0,65536]; maximum number of bytes of a user data segment */
	unsigned char bSingleEsBuf;       /* RW; Range[0,1]; use single output es buffer in n-way encode */
	unsigned int u32SingleEsBufSize;  /* RW; size of single es buffer in n-way encode */
	h265e_refresh_type_e enRefreshType; /* RW; Range:H265E_REFRESH_IDR,H265E_REFRESH_CRA; decoding refresh type */
} venc_mod_h265e_s;

/* the param of the jpege mod */
typedef struct _venc_mod_jpege_s {
	unsigned int u32OneStreamBuffer; /* RW; Range:[0,1]; one stream buffer*/
	unsigned int u32JpegeMiniBufMode; /* RW; Range:[0,1]; Jpege MiniBufMode*/
	unsigned int u32JpegClearStreamBuf; /* RW; Range:[0,1]; JpegClearStreamBuf*/
	unsigned char bSingleEsBuf;       /* RW; Range[0,1]; use single output es buffer in n-way encode */
	unsigned int u32SingleEsBufSize;  /* RW; size of single es buffer in n-way encode */
	jpege_format_e enJpegeFormat; /* RW; Range[0,255]; Jpege format with different marker order */
	jpege_marker_type_e JpegMarkerOrder[JPEG_MARKER_ORDER_CNT];  /* RW: array specifying JPEG marker order*/
} venc_mod_jpege_s;

typedef struct _venc_mod_rc_s {
	unsigned int u32ClrStatAfterSetBr;
} venc_mod_rc_s;
/* the param of the venc mod */
typedef struct _venc_mod_venc_s {
	unsigned int u32VencBufferCache; /* RW; Range:[0,1]; VencBufferCache*/
	unsigned int u32FrameBufRecycle; /* RW; Range:[0,1]; FrameBufRecycle*/
} venc_mod_venc_s;

/* the param of the mod */
typedef struct _VENC_MODPARAM_S {
	venc_modtype_e enVencModType; /* RW; VencModType*/
	union {
		venc_mod_venc_s stVencModParam;
		venc_mod_h264e_s stH264eModParam;
		venc_mod_h265e_s stH265eModParam;
		venc_mod_jpege_s stJpegeModParam;
		venc_mod_rc_s stRcModParam;
	};
} venc_param_mod_s;

typedef enum _venc_frame_type_e {
	VENC_FRAME_TYPE_NONE = 1,
	VENC_FRAME_TYPE_IDR,
	VENC_FRAME_TYPE_BUTT
} venc_frame_type_e;

/* the information of the user rc*/
typedef struct _user_rc_info_s {
	unsigned char bQpMapValid; /*RW; Range:[0,1]; Indicates whether the QpMap mode is valid for the current frame*/
	unsigned char bSkipWeightValid; // RW; Range:[0,1]; Indicates whether
	// the Skip Weight mode is valid for the current frame
	unsigned int u32BlkStartQp; /* RW; Range:[0,51];QP value of the first 16 x 16 block in QpMap mode */
	uint64_t u64QpMapPhyAddr; /* RW; Physical address of the QP table in QpMap mode*/
	uint64_t u64SkipWeightPhyAddr; /* RW; Physical address of the SkipWeight table in QpMap mode*/
	venc_frame_type_e enFrameType;
} user_rc_info_s;

/* the information of the user frame*/
typedef struct _user_frame_info_s {
	video_frame_info_s stUserFrame;
	user_rc_info_s stUserRcInfo;
} user_frame_info_s;

/* the config of the sse*/
typedef struct _venc_sse_cfg_s {
	unsigned int u32Index; /* RW; Range:[0, 7]; Index of an SSE. The system supports indexes ranging from 0 to 7 */
	unsigned char bEnable; /* RW; Range:[0, 1]; Whether to enable SSE */
	rect_s stRect; /* RW; */
} venc_sse_cfg_s;

/* the param of the crop */
typedef struct _venc_crop_info_s {
	unsigned char bEnable; /* RW; Range:[0, 1]; Crop region enable */
	rect_s stRect; /* RW;  Crop region, note: s32X must be multi of 16 */
} venc_crop_info_s;

/* the param of the venc frame rate */
typedef struct _venc_frame_rate_s {
	int s32SrcFrmRate; /* RW; Range:[0, 240]; Input frame rate of a channel*/
	int s32DstFrmRate; /* RW; Range:[0, 240]; Output frame rate of a channel*/
} venc_frame_rate_s;

/* the param of the venc encode chnl */
typedef struct _venc_chn_param_s {
	unsigned char bColor2Grey; /* RW; Range:[0, 1]; Whether to enable Color2Grey.*/
	unsigned int u32Priority; /* RW; Range:[0, 1]; The priority of the coding chnl.*/
	unsigned int u32MaxStrmCnt; /* RW: Range:[0,4294967295]; Maximum number of frames in a stream buffer*/
	unsigned int u32PollWakeUpFrmCnt; /* RW: Range:(0,4294967295]; the frame num needed to wake up  obtaining streams */
	venc_crop_info_s stCropCfg;
	venc_frame_rate_s stFrameRate;
} venc_chn_param_s;

/*the ground protect of FOREGROUND*/
typedef struct _venc_foreground_protect_s {
	unsigned char bForegroundCuRcEn;
	unsigned int u32ForegroundDirectionThresh;
	// RW; Range:[0, 16]; The direction for controlling the macroblock-level bit rate
	unsigned int u32ForegroundThreshGain; /*RW; Range:[0, 15]; The gain of the thresh*/
	unsigned int u32ForegroundThreshOffset; /*RW; Range:[0, 255]; The offset of the thresh*/
	unsigned int u32ForegroundThreshP[RC_TEXTURE_THR_SIZE];
	/*RW; Range:[0, 255]; Mad threshold for controlling the foreground macroblock-level bit rate of P frames */
	unsigned int u32ForegroundThreshB[RC_TEXTURE_THR_SIZE]; // RW; Range:[0, 255]; Mad threshold for controlling
	// the foreground macroblock-level bit rate of B frames
} venc_foreground_protect_s;

/* the scene mode of the venc encode chnl */
typedef enum _venc_scene_mode_e {
	SCENE_0 = 0, /*RW;A scene in which the camera does not move or periodically moves continuously*/
	SCENE_1 = 1, /*RW;Motion scene at high bit rate*/
	SCENE_2 = 2, // RW;It has regular continuous motion at medium bit rate and the
	// encoding pressure is relatively large
	SCENE_BUTT
} venc_scene_mode_e;

typedef struct _venc_debreatheffect_s {
	unsigned char bEnable; /*RW; Range:[0,1];default: 0, DeBreathEffect enable */
	int s32Strength0; /*RW; Range:[0,35];The Strength0 of DeBreathEffect.*/
	int s32Strength1; /*RW; Range:[0,35];The Strength1 of DeBreathEffect.*/
} venc_debreatheffect_s;

typedef struct _venc_cu_prediction_s {
	operation_mode_e enPredMode;

	unsigned int u32IntraCost;
	unsigned int u32Intra32Cost;
	unsigned int u32Intra16Cost;
	unsigned int u32Intra8Cost;
	unsigned int u32Intra4Cost;

	unsigned int u32Inter64Cost;
	unsigned int u32Inter32Cost;
	unsigned int u32Inter16Cost;
	unsigned int u32Inter8Cost;
} venc_cu_prediction_s;

typedef struct _venc_skip_bias_s {
	unsigned char bSkipBiasEn;
	unsigned int u32SkipThreshGain;
	unsigned int u32SkipThreshOffset;
	unsigned int u32SkipBackgroundCost;
	unsigned int u32SkipForegroundCost;
} venc_skip_bias_s;

typedef struct _venc_hierarchical_qp_s {
	unsigned char bHierarchicalQpEn;
	int s32HierarchicalQpDelta[4];
	int s32HierarchicalFrameNum[4];
} venc_hierarchical_qp_s;

typedef struct _venc_chn_pool_s {
	vb_pool hPicVbPool; /* RW;  vb pool id for pic buffer */
	vb_pool hPicInfoVbPool; /* RW;  vb pool id for pic info buffer */
} venc_chn_pool_s;

typedef struct _venc_rc_advparam_s {
	unsigned int u32ClearStatAfterSetAttr;
} venc_rc_advparam_s;

typedef struct _venc_frame_param_s {
	unsigned int u32FrameQp;
	unsigned int u32FrameBits;
} venc_frame_param_s;

typedef struct _venc_sb_setting_S {
	unsigned int codec;  // 0x1:h265, 0x2:h264, 0x4:jpeg
	unsigned int sb_mode;
	unsigned int sb_size;
	unsigned int sb_nb;
	unsigned int y_stride;
	unsigned int uv_stride;
	unsigned int src_height;
	//pri sb address
	unsigned int sb_ybase;
	unsigned int sb_uvbase;
	unsigned int src_ybase;  //[out]
	unsigned int src_uvbase; //[out]
	// sec sb address
	unsigned int sb_ybase1;
	unsigned int sb_uvbase1;
	unsigned int src_ybase1;  //[out]
	unsigned int src_uvbase1; //[out]
} venc_sb_setting;

typedef struct _venc_custom_map_s {
    int roiAvgQp;              /**< It sets an average QP of ROI map. */
    int customRoiMapEnable;    /**< It enables ROI map. */
    int customLambdaMapEnable; /**< It enables custom lambda map. */
    int customModeMapEnable;   /**< It enables to force CTU to be encoded with intra or to be skipped.  */
    int customCoefDropEnable;  /**< It enables to force all coefficients to be zero after TQ or not for each CTU (to be dropped).*/
/**
@verbatim
It indicates the start buffer address of custom map.
Each custom CTU map takes 8 bytes and holds mode, coefficient drop flag, QPs, and lambdas like the below illustration.
*/
    uint64_t addrCustomMap;
} venc_custom_map_s;

/*
 * @verbatim
 * Initial encoding information, produced by the encoder. This structure is
 * essential to actually begin encoding, since it contains all of the
 * necessary information to create and register enough framebuffers. */
typedef struct _venc_initial_info_s {
    /* Caller must register at least this many framebuffers for reconstruction, driver use */
    unsigned int min_num_rec_fb;

    /* Caller must register at least this many framebuffers for source(GOP) */
    unsigned int min_num_src_fb;

    /* (option) Caller can alloc extern buf as bitstream buffer */
     unsigned int min_bs_buf_size;
} venc_initial_info_s;

typedef struct _venc_encode_header_s {
	unsigned char  headerRbsp[256]; /* R; the virtual address of stream */
	unsigned int u32Len; /* R; the length of stream */
} venc_encode_header_s;

typedef enum _venc_search_mode_e {
    SEARCH_MODE_AUTO   = 0,
    SEARCH_MODE_MANUAL = 1,
    SEARCH_MODE_BUTT
} venc_search_mode_e;

typedef struct _venc_search_window_s {
    venc_search_mode_e mode;
    unsigned int u32Hor;
    unsigned int u32Ver;
} venc_search_window_s;

typedef struct _venc_extern_buf {
    unsigned int bs_buf_size;
    uint64_t bs_phys_addr;
} venc_extern_buf_s;

typedef struct _venc_phys_buf {
    unsigned int size;
    unsigned long phys_addr;
} venc_phys_buf_s;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __COMM_VENC_H__ */

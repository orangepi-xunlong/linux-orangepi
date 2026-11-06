/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/comm_rc.h
 * Description:
 *   Common rate control definitions.
 */

#ifndef __COMM_RC_H__
#define __COMM_RC_H__

#include <linux/defines.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

typedef unsigned int DRV_FR32;

/* rc mode */
typedef enum _venc_rc_mode_e {
	VENC_RC_MODE_H264CBR = 1,
	VENC_RC_MODE_H264VBR,
	VENC_RC_MODE_H264AVBR,
	VENC_RC_MODE_H264QVBR,
	VENC_RC_MODE_H264FIXQP,
	VENC_RC_MODE_H264QPMAP,
	VENC_RC_MODE_H264UBR,

	VENC_RC_MODE_MJPEGCBR,
	VENC_RC_MODE_MJPEGVBR,
	VENC_RC_MODE_MJPEGFIXQP,

	VENC_RC_MODE_H265CBR,
	VENC_RC_MODE_H265VBR,
	VENC_RC_MODE_H265AVBR,
	VENC_RC_MODE_H265QVBR,
	VENC_RC_MODE_H265FIXQP,
	VENC_RC_MODE_H265QPMAP,
	VENC_RC_MODE_H265UBR,

	VENC_RC_MODE_BUTT,

} venc_rc_mode_e;

/* qpmap mode*/
typedef enum _venc_rc_qpmap_mode_e {
	VENC_RC_QPMAP_MODE_MEANQP = 0,
	VENC_RC_QPMAP_MODE_MINQP,
	VENC_RC_QPMAP_MODE_MAXQP,

	VENC_RC_QPMAP_MODE_BUTT,
} venc_rc_qpmap_mode_e;

/* the attribute of h264e fixqp*/
typedef struct _venc_h264_fixqp_s {
	unsigned int u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32IQp; ///< qp of the I frame, Range:[0, 51]
	unsigned int u32PQp; ///< qp of the P frame, Range:[0, 51]
	unsigned int u32BQp; ///< Not support
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_h264_fixqp_s;

/* the attribute of h264e cbr*/
typedef struct _venc_h264_cbr_s {
	unsigned int u32Gop; /* RW; Range:[1, 65536]; the interval of I Frame. */
	unsigned int u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32BitRate; /* RW; Range:[2, 409600]; average bitrate */
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_h264_cbr_s;

/* the attribute of h264e vbr*/
typedef struct _venc_h264_vbr_s {
	unsigned int u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	unsigned int u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32MaxBitRate; /* RW; Range:[2, 409600];the max bitrate */
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_h264_vbr_s;

/* the attribute of h264e avbr*/
typedef struct _venc_h264_avbr_s {
	unsigned int u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	unsigned int u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32MaxBitRate; /* RW; Range:[2, 409600];the max bitrate */
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_h264_avbr_s;

/* the attribute of h264e qpmap*/
typedef struct _venc_h264_qpmap_s {
	unsigned int u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	unsigned int u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_h264_qpmap_s;

typedef struct _venc_h264_qvbr_s {
	unsigned int u32Gop; /*the interval of ISLICE. */
	unsigned int u32StatTime; /* the rate statistic time, the unit is senconds(s) */
	unsigned int u32SrcFrameRate; /* the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* the target frame rate of the venc channel */
	unsigned int u32TargetBitRate; /* the target bitrate */
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_h264_qvbr_s;

/* the attribute of h264e ubr*/
typedef struct _venc_h264_ubr_s {
	unsigned int u32Gop; /* RW; Range:[1, 65536]; the interval of I Frame. */
	unsigned int u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32BitRate; /* RW; Range:[2, 409600]; average bitrate */
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_h264_ubr_s;

/* the attribute of h265e qpmap*/
typedef struct _venc_h265_qpmap_s {
	unsigned int u32Gop; /* RW; Range:[1, 65536]; the interval of ISLICE. */
	unsigned int u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	venc_rc_qpmap_mode_e enQpMapMode; /* RW;  the QpMap Mode.*/
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_h265_qpmap_s;

typedef struct _venc_h264_cbr_s VENC_H265_CBR_S;
typedef struct _venc_h264_vbr_s VENC_H265_VBR_S;
typedef struct _venc_h264_avbr_s VENC_H265_AVBR_S;
typedef struct _venc_h264_fixqp_s VENC_H265_FIXQP_S;
typedef struct _venc_h264_qvbr_s VENC_H265_QVBR_S;
typedef struct _venc_h264_ubr_s VENC_H265_UBR_S;

/* the attribute of mjpege fixqp*/
typedef struct _venc_mjpeg_fixqp_s {
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32Qfactor; /* RW; Range:[0,99];image quality. */
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_mjpeg_fixqp_s;

/* the attribute of mjpege cbr*/
typedef struct _venc_mjpeg_cbr_s {
	unsigned int u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32BitRate; /* RW; Range:[2, 409600]; average bitrate */
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_mjpeg_cbr_s;

/* the attribute of mjpege vbr*/
typedef struct _venc_mjpeg_vbr_s {
	unsigned int u32StatTime; /* RW; Range:[1, 60]; the rate statistic time, the unit is senconds(s) */
	unsigned int u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
	DRV_FR32 fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
	unsigned int u32MaxBitRate; /* RW; Range:[2, 409600];the max bitrate */
	unsigned char bVariFpsEn; /* RW; Range:[0, 1]; enable variable framerate */
} venc_mjpeg_vbr_s;

/* the attribute of rc*/
typedef struct _venc_rc_attr_s {
	venc_rc_mode_e enRcMode; /* RW; the type of rc*/
	union {
		venc_h264_cbr_s stH264Cbr;
		venc_h264_vbr_s stH264Vbr;
		venc_h264_avbr_s stH264AVbr;
		venc_h264_qvbr_s stH264QVbr;
		venc_h264_fixqp_s stH264FixQp;
		venc_h264_qpmap_s stH264QpMap;
		venc_h264_ubr_s stH264Ubr;

		venc_mjpeg_cbr_s stMjpegCbr;
		venc_mjpeg_vbr_s stMjpegVbr;
		venc_mjpeg_fixqp_s stMjpegFixQp;

		VENC_H265_CBR_S stH265Cbr;
		VENC_H265_VBR_S stH265Vbr;
		VENC_H265_AVBR_S stH265AVbr;
		VENC_H265_QVBR_S stH265QVbr;
		VENC_H265_FIXQP_S stH265FixQp;
		venc_h265_qpmap_s stH265QpMap;
		VENC_H265_UBR_S stH265Ubr;
	};
} venc_rc_attr_s;

/*the super frame mode*/
typedef enum _venc_superfrm_mode_e {
	SUPERFRM_NONE = 0, /* sdk don't care super frame */
	SUPERFRM_DISCARD, /* the super frame is discarded */
	SUPERFRM_REENCODE, /* the super frame is re-encode */
	SUPERFRM_REENCODE_IDR, /* the super frame is re-encode to IDR */
	SUPERFRM_BUTT
} venc_superfrm_mode_e;

/* The param of H264e cbr*/
typedef struct _venc_param_h264_cbr_s {
	unsigned int u32MinIprop; /* RW; Range:[1, 100]; the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* RW; Range:(u32MinIprop, 100]; the max ratio of i frame and p frame */
	unsigned int u32MaxQp; /* RW; Range:(u32MinQp, 51];the max QP value */
	unsigned int u32MinQp; /* RW; Range:[0, 51]; the min QP value */
	unsigned int u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; max qp for i frame */
	unsigned int u32MinIQp; /* RW; Range:[0, 51]; min qp for i frame */
	int s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/
	unsigned char bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
} venc_param_h264_cbr_s;

/* The param of H264e vbr*/
typedef struct _venc_param_h264_vbr_s {
	int s32ChangePos;
	// RW; Range:[50, 100]; Indicates the ratio of the current bit rate to the maximum
	// bit rate when the QP value starts to be adjusted
	unsigned int u32MinIprop; /* RW; Range:[1, 100] ; the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* RW; Range:(u32MinIprop, 100] ; the max ratio of i frame and p frame */
	int s32MaxReEncodeTimes; /* RW; Range:[0, 3]; max number of re-encode times */
	unsigned char bQpMapEn;

	unsigned int u32MaxQp; /* RW; Range:(u32MinQp, 51]; the max P B qp */
	unsigned int u32MinQp; /* RW; Range:[0, 51]; the min P B qp */
	unsigned int u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; the max I qp */
	unsigned int u32MinIQp; /* RW; Range:[0, 51]; the min I qp */
} venc_param_h264_vbr_s;

/* The param of H264e avbr*/
typedef struct _venc_param_h264_avbr_s {
	int s32ChangePos;
	// RW; Range:[50, 100]; Indicates the ratio of the current bit rate to the maximum
	// bit rate when the QP value starts to be adjusted
	unsigned int u32MinIprop; /* RW; Range:[1, 100] ; the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* RW; Range:(u32MinIprop, 100] ; the max ratio of i frame and p frame */
	int s32MaxReEncodeTimes; /* RW; Range:[0, 3]; max number of re-encode times */
	unsigned char bQpMapEn;

	int s32MinStillPercent; /* RW; Range:[5, 100]; the min percent of target bitrate for still scene */
	unsigned int u32MaxStillQP; /* RW; Range:[u32MinIQp, u32MaxIQp]; the max QP value of I frame for still scene*/
	unsigned int u32MinStillPSNR; /* RW; reserved,Invalid member currently */

	unsigned int u32MaxQp; /* RW; Range:(u32MinQp, 51]; the max P B qp */
	unsigned int u32MinQp; /* RW; Range:[0, 51]; the min P B qp */
	unsigned int u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; the max I qp */
	unsigned int u32MinIQp; /* RW; Range:[0, 51]; the min I qp */
	unsigned int u32MinQpDelta;
	// Difference between FrameLevelMinQp & u32MinQp, FrameLevelMinQp = u32MinQp(or u32MinIQp) + MinQpDelta

	unsigned int u32MotionSensitivity; /* RW; Range:[0, 100]; Motion Sensitivity */
	int	s32AvbrFrmLostOpen; /* RW; Range:[0, 1]; Open Frame Lost */
	int s32AvbrFrmGap; /* RW; Range:[0, 100]; Maximim Gap of Frame Lost */
	int s32AvbrPureStillThr;
} venc_param_h264_avbr_s;

typedef struct _venc_param_h264_qvbr_s {
	unsigned int u32MinIprop; /* the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* the max ratio of i frame and p frame */
	int s32MaxReEncodeTimes; /* max number of re-encode times [0, 3]*/
	unsigned char bQpMapEn;

	unsigned int u32MaxQp; /* the max P B qp */
	unsigned int u32MinQp; /* the min P B qp */
	unsigned int u32MaxIQp; /* the max I qp */
	unsigned int u32MinIQp; /* the min I qp */

	int s32BitPercentUL; /*Indicate the ratio of bitrate  upper limit*/
	int s32BitPercentLL; /*Indicate the ratio of bitrate  lower limit*/
	int s32PsnrFluctuateUL; /*Reduce the target bitrate when the value of psnr approch the upper limit*/
	int s32PsnrFluctuateLL; /*Increase the target bitrate when the value of psnr approch the lower limit */
} venc_param_h264_qvbr_s;

/* The param of H264e ubr */
typedef struct _venc_param_h264_ubr_s {
	unsigned int u32MinIprop; /* RW; Range:[1, 100]; the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* RW; Range:(u32MinIprop, 100]; the max ratio of i frame and p frame */
	unsigned int u32MaxQp; /* RW; Range:(u32MinQp, 51];the max QP value */
	unsigned int u32MinQp; /* RW; Range:[0, 51]; the min QP value */
	unsigned int u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; max qp for i frame */
	unsigned int u32MinIQp; /* RW; Range:[0, 51]; min qp for i frame */
	int s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/
	unsigned char bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
} venc_param_h264_ubr_s;

/* The param of mjpege cbr*/
typedef struct _venc_param_mjpeg_cbr_s {
	unsigned int u32MaxQfactor; /* RW; Range:[MinQfactor, 99]; the max Qfactor value*/
	unsigned int u32MinQfactor; /* RW; Range:[1, 99]; the min Qfactor value */
} venc_param_mjpeg_cbr_s;

/* The param of mjpege vbr*/
typedef struct _venc_param_mjpeg_vbr_s {
	int s32ChangePos;
	// RW; Range:[50, 100]; Indicates the ratio of the current bit rate to the maximum
	// bit rate when the Qfactor value starts to be adjusted
	unsigned int u32MaxQfactor; /* RW; Range:[MinQfactor, 99]; max image quailty allowed */
	unsigned int u32MinQfactor; /* RW; Range:[1, 99]; min image quality allowed */
} venc_param_mjpeg_vbr_s;

/* The param of h265e cbr*/
typedef struct _venc_param_h265_cbr_s {
	unsigned int u32MinIprop; /* RW; Range: [1, 100]; the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* RW; Range: (u32MinIprop, 100] ;the max ratio of i frame and p frame */
	unsigned int u32MaxQp; /* RW; Range:(u32MinQp, 51];the max QP value */
	unsigned int u32MinQp; /* RW; Range:[0, 51];the min QP value */
	unsigned int u32MaxIQp; /* RW; Range:(u32MinIQp, 51];max qp for i frame */
	unsigned int u32MinIQp; /* RW; Range:[0, 51];min qp for i frame */
	int s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/
	unsigned char bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
	venc_rc_qpmap_mode_e enQpMapMode; /* RW; Qpmap Mode*/
} venc_param_h265_cbr_s;

/* The param of h265e vbr*/
typedef struct _venc_param_h265_vbr_s {
	int s32ChangePos;
	// RW; Range:[50, 100];Indicates the ratio of the current
	// bit rate to the maximum bit rate when the QP value starts to be adjusted
	unsigned int u32MinIprop; /* RW; [1, 100]the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* RW; (u32MinIprop, 100]the max ratio of i frame and p frame */
	int s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/

	unsigned int u32MaxQp; /* RW; Range:(u32MinQp, 51]; the max P B qp */
	unsigned int u32MinQp; /* RW; Range:[0, 51]; the min P B qp */
	unsigned int u32MaxIQp; /* RW; Range:(u32MinIQp, 51]; the max I qp */
	unsigned int u32MinIQp; /* RW; Range:[0, 51]; the min I qp */

	unsigned char bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
	venc_rc_qpmap_mode_e enQpMapMode; /* RW; Qpmap Mode*/
} venc_param_h265_vbr_s;

/* The param of h265e vbr*/
typedef struct _venc_param_h265_avbr_s {
	int s32ChangePos;
	// RW; Range:[50, 100];Indicates the ratio of the current
	// bit rate to the maximum bit rate when the QP value starts to be adjusted
	unsigned int u32MinIprop; /* RW; [1, 100]the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* RW; (u32MinIprop, 100]the max ratio of i frame and p frame */
	int s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/

	int s32MinStillPercent; /* RW; Range:[5, 100]; the min percent of target bitrate for still scene */
	unsigned int u32MaxStillQP; /* RW; Range:[u32MinIQp, u32MaxIQp]; the max QP value of I frame for still scene*/
	unsigned int u32MinStillPSNR; /* RW; reserved */

	unsigned int u32MaxQp; /* RW; Range:(u32MinQp, 51];the max P B qp */
	unsigned int u32MinQp; /* RW; Range:[0, 51];the min P B qp */
	unsigned int u32MaxIQp; /* RW; Range:(u32MinIQp, 51];the max I qp */
	unsigned int u32MinIQp; /* RW; Range:[0, 51];the min I qp */
	unsigned int u32MinQpDelta;
	// Difference between FrameLevelMinQp & u32MinQp, FrameLevelMinQp = u32MinQp(or u32MinIQp) + MinQpDelta

	unsigned int u32MotionSensitivity; /* RW; Range:[0, 100]; Motion Sensitivity */
	int	s32AvbrFrmLostOpen; /* RW; Range:[0, 1]; Open Frame Lost */
	int s32AvbrFrmGap; /* RW; Range:[0, 100]; Maximim Gap of Frame Lost */
	int s32AvbrPureStillThr;
	unsigned char bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
	venc_rc_qpmap_mode_e enQpMapMode; /* RW; Qpmap Mode*/
} venc_param_h265_avbr_s;

typedef struct _venc_param_h265_qvbr_s {
	unsigned int u32MinIprop; /* the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* the max ratio of i frame and p frame */
	int s32MaxReEncodeTimes; /* max number of re-encode times [0, 3]*/

	unsigned char bQpMapEn;
	venc_rc_qpmap_mode_e enQpMapMode;

	unsigned int u32MaxQp; /* the max P B qp */
	unsigned int u32MinQp; /* the min P B qp */
	unsigned int u32MaxIQp; /* the max I qp */
	unsigned int u32MinIQp; /* the min I qp */

	int s32BitPercentUL; /* Indicate the ratio of bitrate  upper limit*/
	int s32BitPercentLL; /* Indicate the ratio of bitrate  lower limit*/
	int s32PsnrFluctuateUL; /* Reduce the target bitrate when the value of psnr approch the upper limit */
	int s32PsnrFluctuateLL; /* Increase the target bitrate when the value of psnr approch the lower limit */
} venc_param_h265_qvbr_s;

/* The param of h265e ubr*/
typedef struct _venc_param_h265_ubr_s {
	unsigned int u32MinIprop; /* RW; Range: [1, 100]; the min ratio of i frame and p frame */
	unsigned int u32MaxIprop; /* RW; Range: (u32MinIprop, 100]; the max ratio of i frame and p frame */
	unsigned int u32MaxQp; /* RW; Range:(u32MinQp, 51];the max QP value */
	unsigned int u32MinQp; /* RW; Range:[0, 51];the min QP value */
	unsigned int u32MaxIQp; /* RW; Range:(u32MinIQp, 51];max qp for i frame */
	unsigned int u32MinIQp; /* RW; Range:[0, 51];min qp for i frame */
	int s32MaxReEncodeTimes; /* RW; Range:[0, 3]; Range:max number of re-encode times.*/
	unsigned char bQpMapEn; /* RW; Range:[0, 1]; enable qpmap.*/
	venc_rc_qpmap_mode_e enQpMapMode; /* RW; Qpmap Mode*/
} venc_param_h265_ubr_s;

/* The param of rc*/
typedef struct _venc_rc_param_s {
	unsigned int u32ThrdI[RC_TEXTURE_THR_SIZE]; // RW; Range:[0, 255]; Mad threshold for
	// controlling the macroblock-level bit rate of I frames
	unsigned int u32ThrdP[RC_TEXTURE_THR_SIZE]; // RW; Range:[0, 255]; Mad threshold for
	// controlling the macroblock-level bit rate of P frames
	unsigned int u32ThrdB[RC_TEXTURE_THR_SIZE]; // RW; Range:[0, 255]; Mad threshold for
	// controlling the macroblock-level bit rate of B frames
	unsigned int u32DirectionThrd; /*RW; Range:[0, 16]; The direction for controlling the macroblock-level bit rate */
	unsigned int u32RowQpDelta;
	// RW; Range:[0, 10];the start QP value of each macroblock row relative to the start QP value
	int s32FirstFrameStartQp; /* RW; Range:[-1, 51];Start QP value of the first frame*/
	int s32InitialDelay;	// RW; Range:[10, 3000]; Rate control initial delay (ms).
	unsigned int u32ThrdLv; /*RW; Range:[0, 4]; Mad threshold for controlling the macroblock-level bit rate */
	unsigned char bBgEnhanceEn; /* RW; Range:[0, 1];  Enable background enhancement */
	int s32BgDeltaQp; /* RW; Range:[-51, 51]; Backgournd Qp Delta */
	union {
		venc_param_h264_cbr_s stParamH264Cbr;
		venc_param_h264_vbr_s stParamH264Vbr;
		venc_param_h264_avbr_s stParamH264AVbr;
		venc_param_h264_qvbr_s stParamH264QVbr;
		venc_param_h264_ubr_s stParamH264Ubr;
		venc_param_h265_cbr_s stParamH265Cbr;
		venc_param_h265_vbr_s stParamH265Vbr;
		venc_param_h265_avbr_s stParamH265AVbr;
		venc_param_h265_qvbr_s stParamH265QVbr;
		venc_param_h265_ubr_s stParamH265Ubr;
		venc_param_mjpeg_cbr_s stParamMjpegCbr;
		venc_param_mjpeg_vbr_s stParamMjpegVbr;
	};
} venc_rc_param_s;

/* the frame lost mode*/
typedef enum _venc_framelost_mode_e {
	FRMLOST_NORMAL = 0, /*normal mode*/
	FRMLOST_PSKIP, /*pskip*/
	FRMLOST_BUTT,
} venc_framelost_mode_e;

/* The param of the frame lost mode*/
typedef struct _venc_framelost_s {
	unsigned char bFrmLostOpen; // RW; Range:[0,1];Indicates whether to discard frames
	// to ensure stable bit rate when the instant bit rate is exceeded
	unsigned int u32FrmLostBpsThr; /* RW; Range:[64k, 163840k];the instant bit rate threshold */
	venc_framelost_mode_e enFrmLostMode; /* frame lost strategy*/
	unsigned int u32EncFrmGaps; /* RW; Range:[0,65535]; the gap of frame lost*/
} venc_framelost_s;

/* the rc priority*/
typedef enum _venc_rc_priority_e {
	VENC_RC_PRIORITY_BITRATE_FIRST = 1, /* bitrate first */
	VENC_RC_PRIORITY_FRAMEBITS_FIRST, /* framebits first*/
	VENC_RC_PRIORITY_BUTT,
} venc_rc_priority_e;

/* the config of the superframe */
typedef struct _venc_superframe_cfg_s {
	venc_superfrm_mode_e enSuperFrmMode;
	/* RW; Indicates the mode of processing the super frame */
	unsigned int u32SuperIFrmBitsThr; // RW; Range:[0, 33554432];Indicate the threshold
	// of the super I frame for enabling the super frame processing mode
	unsigned int u32SuperPFrmBitsThr; // RW; Range:[0, 33554432];Indicate the threshold
	// of the super P frame for enabling the super frame processing mode
	unsigned int u32SuperBFrmBitsThr; // RW; Range:[0, 33554432];Indicate the threshold
	// of the super B frame for enabling the super frame processing mode
	venc_rc_priority_e enRcPriority; /* RW; Rc Priority */
} venc_superframe_cfg_s;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __COMM_RC_H__ */

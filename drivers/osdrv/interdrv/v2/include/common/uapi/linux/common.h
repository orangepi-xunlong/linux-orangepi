/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/common.h
 * Description: Common video definitions.
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <linux/defines.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef VER_X
#define VER_X 1
#endif

#ifndef VER_Y
#define VER_Y 7
#endif

#ifndef VER_Z
#define VER_Z 0
#endif

#ifndef __FILENAME__
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifdef DEBUG
#define VER_D " Debug"
#else
#define VER_D " Release"
#endif


#define ATTRIBUTE  __attribute__((aligned(ALIGN_NUM)))

//#ifndef ARRAY_SIZE
//#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
//#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define MK_VERSION(x, y, z) STR(x) "." STR(y) "." STR(z)



typedef int ai_chn;
typedef int AO_CHN;
typedef int aenc_chn;
typedef int adec_chn;
typedef int audio_dev;
typedef int vi_dev;
typedef int vi_pipe;
typedef int vi_chn;
typedef int vo_dev;
typedef int vo_layer;
typedef int vo_chn;
typedef int vo_wbc;
typedef int graphic_layer;
typedef int venc_chn;
typedef int vdec_chn;
typedef int isp_dev;
typedef int sensor_id;
typedef int mipi_dev;
typedef int slave_dev;
typedef int vpss_grp;
typedef int vpss_chn;
typedef int dpu_grp;
typedef int dpu_chn;
typedef int stitch_src_idx;
typedef int stitch_grp;

#define INVALID_CHN (-1)
#define INVALID_LAYER (-1)
#define INVALID_DEV (-1)
#define INVALID_HANDLE (-1)
#define INVALID_VALUE (-1)
#define INVALID_TYPE (-1)


#define CCM_MATRIX_SIZE             (9)
#define CCM_MATRIX_NUM              (7)


#define FOREACH_MOD(MOD) {\
	MOD(BASE)   \
	MOD(VB)	    \
	MOD(SYS)    \
	MOD(RGN)    \
	MOD(CHNL)   \
	MOD(VDEC)   \
	MOD(VPSS)   \
	MOD(VENC)   \
	MOD(H264E)  \
	MOD(JPEGE)  \
	MOD(MPEG4E) \
	MOD(H265E)  \
	MOD(JPEGD)  \
	MOD(VO)	    \
	MOD(VI)	    \
	MOD(DIS)    \
	MOD(RC)	    \
	MOD(AIO)    \
	MOD(AI)	    \
	MOD(AO)	    \
	MOD(AENC)   \
	MOD(ADEC)   \
	MOD(AUD)    \
	MOD(VPU)    \
	MOD(ISP)    \
	MOD(IVE)    \
	MOD(USER)   \
	MOD(PROC)   \
	MOD(LOG)    \
	MOD(H264D)  \
	MOD(GDC)    \
	MOD(PHOTO)  \
	MOD(FB)	    \
	MOD(DPU)	\
	MOD(STITCH) \
	MOD(HDMI)   \
	MOD(BUTT)   \
}

#define GENERATE_ENUM(ENUM) ID_ ## ENUM,

typedef enum _mod_id_e FOREACH_MOD(GENERATE_ENUM) mod_id_e;

typedef struct _mmf_chn_s {
	mod_id_e    mod_id;
	int     dev_id;
	int     chn_id;
} mmf_chn_s;


/* We just copy this value of payload type from RTP/RTSP definition */
typedef enum {
	PT_PCMU          = 0,
	PT_1016          = 1,
	PT_G721          = 2,
	PT_GSM           = 3,
	PT_G723          = 4,
	PT_DVI4_8K       = 5,
	PT_DVI4_16K      = 6,
	PT_LPC           = 7,
	PT_PCMA          = 8,
	PT_G722          = 9,
	PT_S16BE_STEREO  = 10,
	PT_S16BE_MONO    = 11,
	PT_QCELP         = 12,
	PT_CN            = 13,
	PT_MPEGAUDIO     = 14,
	PT_G728          = 15,
	PT_DVI4_3        = 16,
	PT_DVI4_4        = 17,
	PT_G729          = 18,
	PT_G711A         = 19,
	PT_G711U         = 20,
	PT_G726          = 21,
	PT_G729A         = 22,
	PT_LPCM          = 23,
	PT_CelB          = 25,
	PT_JPEG          = 26,
	PT_CUSM          = 27,
	PT_NV            = 28,
	PT_PICW          = 29,
	PT_CPV           = 30,
	PT_H261          = 31,
	PT_MPEGVIDEO     = 32,
	PT_MPEG2TS       = 33,
	PT_H263          = 34,
	PT_SPEG          = 35,
	PT_MPEG2VIDEO    = 36,
	PT_AAC           = 37,
	PT_WMA9STD       = 38,
	PT_HEAAC         = 39,
	PT_PCM_VOICE     = 40,
	PT_PCM_AUDIO     = 41,
	PT_MP3           = 43,
	PT_ADPCMA        = 49,
	PT_AEC           = 50,
	PT_X_LD          = 95,
	PT_H264          = 96,
	PT_D_GSM_HR      = 200,
	PT_D_GSM_EFR     = 201,
	PT_D_L8          = 202,
	PT_D_RED         = 203,
	PT_D_VDVI        = 204,
	PT_D_BT656       = 220,
	PT_D_H263_1998   = 221,
	PT_D_MP1S        = 222,
	PT_D_MP2P        = 223,
	PT_D_BMPEG       = 224,
	PT_MP4VIDEO      = 230,
	PT_MP4AUDIO      = 237,
	PT_VC1           = 238,
	PT_JVC_ASF       = 255,
	PT_D_AVI         = 256,
	PT_DIVX3         = 257,
	PT_AVS             = 258,
	PT_REAL8         = 259,
	PT_REAL9         = 260,
	PT_VP6             = 261,
	PT_VP6F             = 262,
	PT_VP6A             = 263,
	PT_SORENSON          = 264,
	PT_H265          = 265,
	PT_VP8             = 266,
	PT_MVC             = 267,
	PT_PNG           = 268,
	/* add by ourselves */
	PT_AMR           = 1001,
	PT_MJPEG         = 1002,
	PT_BUTT
} payload_type_e;

#define VERSION_NAME_MAXLEN 128
typedef struct _mmf_version_s {
	char version[VERSION_NAME_MAXLEN];
} mmf_version_s;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* __COMMON_H__ */

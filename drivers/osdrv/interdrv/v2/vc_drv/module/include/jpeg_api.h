/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: jpeg_api.h
 * Description:
 *   Jpeg Codec interface
 */
#ifndef JPG_API_H
#define JPG_API_H
#include <linux/types.h>
#ifdef __cplusplus
extern "C" {
#endif

#define DRV_JPG_MARKER_ORDER_CNT 16
#define DRV_JPG_MARKER_ORDER_BUF_SIZE (sizeof(int) * DRV_JPG_MARKER_ORDER_CNT)
#define DRV_JPG_DEFAULT_BUFSIZE (512 * 1024)

/* enum define */
typedef enum {
    JPGCOD_UNKNOWN = 0,
    JPGCOD_DEC = 1,
    JPGCOD_ENC = 2
} JPG_JpgCodType;

typedef enum {
    DRV_FORMAT_420 = 0,
    DRV_FORMAT_422 = 1,
    DRV_FORMAT_224 = 2,
    DRV_FORMAT_444 = 3,
    DRV_FORMAT_400 = 4,
    DRV_FORMAT_BUTT
} JPG_FrameFormat;

typedef enum {
    DRV_CBCR_SEPARATED = 0,
    DRV_CBCR_INTERLEAVE,
    DRV_CRCB_INTERLEAVE
} JPG_CbCrInterLeave;

typedef enum {
    DRV_PACKED_FORMAT_NONE,
    DRV_PACKED_FORMAT_422_YUYV,
    DRV_PACKED_FORMAT_422_UYVY,
    DRV_PACKED_FORMAT_422_YVYU,
    DRV_PACKED_FORMAT_422_VYUY,
    DRV_PACKED_FORMAT_444,
    DRV_PACKED_FORMAT_444_RGB
} JPG_PackedFormat;

typedef enum {
    JPEG_MEM_MODULE = 2,
    JPEG_MEM_EXTERNAL = 3,
} JPEG_MEM_TYPE;

#define DRV_JPEG_OP_BASE 0x10000
#define DRV_JPEG_OP_MASK 0xFF0000
#define DRV_JPEG_OP_SHIFT 16

enum JPG_OP_NUM {
    JPGE_OP_NONE = 0,
    JPEG_OP_SET_QUALITY,
    JPEG_OP_GET_QUALITY,
    JPEG_OP_SET_CHN_ATTR,
    JPEG_OP_SET_MCUPerECS,
    JPEG_OP_RESET_CHN,
    JPEG_OP_SET_USER_DATA,
    JPEG_OP_SHOW_CHN_INFO,
    JPEG_OP_START,
    JPEG_OP_SET_SBM_ENABLE ,
    JPEG_OP_WAIT_FRAME_DONE,
    JPEG_OP_SET_QMAP_TABLE,
    JPEG_OP_SET_RC_PARAM,
    JPEG_OP_MAX,
};

typedef enum _DRV_JPEG_OP_ {
    DRV_JPEG_OP_NONE = 0,
    DRV_JPEG_OP_SET_QUALITY =     (JPEG_OP_SET_QUALITY << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_GET_QUALITY =     (JPEG_OP_GET_QUALITY << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_SET_CHN_ATTR =    (JPEG_OP_SET_CHN_ATTR << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_SET_MCUPerECS =   (JPEG_OP_SET_MCUPerECS << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_RESET_CHN =       (JPEG_OP_RESET_CHN << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_SET_USER_DATA =   (JPEG_OP_SET_USER_DATA << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_SHOW_CHN_INFO =   (JPEG_OP_SHOW_CHN_INFO << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_START =           (JPEG_OP_START << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_SET_SBM_ENABLE =  (JPEG_OP_SET_SBM_ENABLE << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_WAIT_FRAME_DONE = (JPEG_OP_WAIT_FRAME_DONE << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_SET_QMAP_TABLE =  (JPEG_OP_SET_QMAP_TABLE << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_SET_RC_PARAM   =  (JPEG_OP_SET_RC_PARAM << DRV_JPEG_OP_SHIFT),
    DRV_JPEG_OP_MAX =             (JPEG_OP_MAX << DRV_JPEG_OP_SHIFT),
} DRV_JPEG_OP;

/* struct define */

/* Frame Buffer */

typedef struct {
    unsigned int size;
    __u64 phys_addr;
    __u64 base;
    __u8 *virt_addr;
} JPG_BUF;

typedef struct {
    JPG_FrameFormat format;
    JPG_PackedFormat packedFormat;
    JPG_CbCrInterLeave chromaInterleave;
    JPG_BUF vbY;
    JPG_BUF vbCb;
    JPG_BUF vbCr;
    int width;
    int height;
    int strideY;
    int strideC;
} DRVFRAMEBUF;

/* encode config param */
typedef struct {
    int picWidth;
    int picHeight;
    int rotAngle;
    int mirDir;
    JPG_FrameFormat sourceFormat;
    int outNum;
    JPG_CbCrInterLeave chromaInterleave;
    int bEnStuffByte;
    int encHeaderMode;
    int RandRotMode;
    JPG_PackedFormat packedFormat;
    int quality;
    int bitrate;
    int framerate;
    int src_type;
    uint64_t external_bs_addr;
    int bitstreamBufSize;
    int singleEsBuffer;
    int jpgMarkerOrder[DRV_JPG_MARKER_ORDER_CNT];
    int qmin;
    int qmax;
} JPG_EncConfigParam;

/* decode config param */
typedef struct {
    uint64_t external_bs_addr;
    int external_bs_size;
    uint64_t external_fb_addr;
    int external_fb_size;
    /* ROI param */
    int roiEnable;
    int roiWidth;
    int roiHeight;
    int roiOffsetX;
    int roiOffsetY;
    int max_frame_width;
    int max_frame_height;
    int min_frame_width;
    int min_frame_height;
    /* Frame Partial Mode (DON'T SUPPORT)*/
    int usePartialMode;
    /* Rotation Angle (0, 90, 180, 270) */
    int rotAngle;
    /* mirror direction (0-no mirror, 1-vertical, 2-horizontal, 3-both) */
    int mirDir;
    /* Scale Mode */
    int iHorScaleMode;
    int iVerScaleMode;
    /* stream data length */
    int iDataLen;
    int dst_type;
    DRVFRAMEBUF dec_buf;
} JPG_DecConfigParam;

/* jpu config param */
typedef struct {
    JPG_JpgCodType type;
    union {
        JPG_EncConfigParam enc;
        JPG_DecConfigParam dec;
    } u;
    int s32ChnNum;
} drv_jpg_config;

typedef struct _jpeg_chn_attr_ {
    unsigned int
        u32SrcFrameRate; /* RW; Range:[1, 240]; the input frame rate of the venc channel */
    unsigned int
        fr32DstFrameRate; /* RW; Range:[0.015625, u32SrcFrmRate]; the target frame rate of the venc channel */
    unsigned int u32BitRate; /* RW; Range:[2, 409600]; average bitrate */
    unsigned int picWidth; ///< width of a picture to be encoded
    unsigned int picHeight; ///< height of a picture to be encoded
} jpeg_chn_attr;

typedef struct _jpeg_user_data_ {
    unsigned char *userData;
    unsigned int len;
} jpeg_user_data;

typedef struct _jpeg_enc_rc_param {
    int chg_pos;
    int max_qfactor;
    int min_qfactor;
} jpeg_enc_rc_param;

/* JPU CODEC HANDLE */
typedef void *drv_jpg_handle;

void drv_jpg_get_version(void);

/* initial jpu core */
drv_jpg_handle jpeg_enc_init(void);
drv_jpg_handle jpeg_dec_init(void);
/* uninitial jpu core */
void jpeg_enc_deinit(drv_jpg_handle handle);
void jpeg_dec_deinit(drv_jpg_handle handle);
int jpeg_enc_open(drv_jpg_handle handle, drv_jpg_config config);
int jpeg_dec_open(drv_jpg_handle handle, drv_jpg_config config);
/* close and free alloced jpu handle */
int jpeg_enc_close(drv_jpg_handle handle);
int jpeg_dec_close(drv_jpg_handle handle);
int jpeg_get_caps(drv_jpg_handle handle);
/* flush data */
int jpeg_flush(drv_jpg_handle handle);
/* send jpu data to decode or encode */
int jpeg_enc_send_frame(drv_jpg_handle jpgHandle, DRVFRAMEBUF *data, int timeout);
/* send jpu data to decode or encode */
int jpeg_dec_send_stream(drv_jpg_handle handle, void *data, int length, int timeout);
int jpeg_enc_get_stream(drv_jpg_handle handle, void *data, unsigned long int *pu64HwTime);
int jpeg_dec_get_frame(drv_jpg_handle handle, void *data, unsigned long int *pu64HwTime);
/* release stream buffer */
int jpeg_enc_release_stream(drv_jpg_handle handle);
int jpeg_dec_release_frame(drv_jpg_handle handle, uint64_t addr);
int jpeg_reset(void);
int jpeg_enc_set_quality(drv_jpg_handle handle, void *data);
int jpeg_enc_get_quality(drv_jpg_handle handle, void *data);
int jpeg_set_chn_attr(drv_jpg_handle handle, void *arg);
int jpeg_enc_set_mcuinfo(drv_jpg_handle handle, void *data);
int jpeg_channel_reset(drv_jpg_handle handle, void *data);
int jpeg_set_user_data(drv_jpg_handle handle, void *data);
int jpeg_show_channel_info(drv_jpg_handle handle, void *data);
int jpeg_enc_get_output_count(drv_jpg_handle handle);
int jpeg_dec_get_output_count(drv_jpg_handle handle);
int jpeg_ioctl(void *handle, int op, void *arg);
int jpeg_dec_set_param(drv_jpg_handle handle, drv_jpg_config config);

#ifdef __cplusplus
}
#endif

#endif /* JPEG_API_H
*/

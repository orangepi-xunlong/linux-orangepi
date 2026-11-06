#ifndef __ENC_CTX_H__
#define __ENC_CTX_H__

#include <linux/comm_venc.h>
#include "jpeg_api.h"
#include "h265_interface.h"


typedef struct _venc_jpeg_ctx {
    drv_jpg_handle *handle;
} venc_jpeg_ctx;

typedef struct _venc_vid_ctx {
    void (*setInitCfgFixQp)
    (InitEncConfig *pInitEncCfg, void *pchnctx);
    void (*setInitCfgCbr)
    (InitEncConfig *pInitEncCfg, void *pchnctx);
    void (*setInitCfgVbr)
    (InitEncConfig *pInitEncCfg, void *pchnctx);
    void (*setInitCfgAVbr)
    (InitEncConfig *pInitEncCfg, void *pchnctx);
    void (*setInitCfgQpMap)
    (InitEncConfig *pInitEncCfg, void *pchnctx);
    void (*setInitCfgUbr)
    (InitEncConfig *pInitEncCfg, void *pchnctx);
    void (*setInitCfgRc)
    (InitEncConfig *pInitEncCfg, void *pchnctx);
    int (*mapNaluType)(venc_pack_s *ppack, int naluType);
    void *pHandle;
    VEncStreamInfo streamInfo;
} venc_vid_ctx;

typedef struct _venc_h264_ctx {
} venc_h264_ctx;

typedef struct _venc_h265_ctx {
} venc_h265_ctx;

typedef struct _venc_enc_ctx_base {
    int x;
    int y;
    int width;
    int height;
    int u32Profile;
    int rcMode;
    unsigned char bVariFpsEn;
    uint64_t u64PTS;
    uint64_t u64EncHwTime;
    int (*init)(void *ctx);
    int (*open)(void *handle, void *pchnctx);
    int (*close)(void *ctx);
    int (*encOnePic)
    (void *ctx, const video_frame_info_s *pstFrame,
     int s32MilliSec);
    int (*getStream)
    (void *ctx, venc_stream_s *pstStream, int s32MIlliSec);
    int (*releaseStream)(void *ctx, venc_stream_s *pstStream);
    int (*ioctl)(void *ctx, int op, void *arg);
} venc_enc_ctx_base;

typedef struct _venc_enc_ctx {
    venc_enc_ctx_base base;
    union {
        venc_jpeg_ctx jpeg;
        struct {
            venc_vid_ctx vid;
            union {
                venc_h264_ctx h264;
                venc_h265_ctx h265;
            };
        };
    } ext;
} venc_enc_ctx;


#define H265_NALU_TYPE_W_RADL 19
#define H265_NALU_TYPE_N_LP 20
#define H264_NALU_TYPE_IDR 5

#ifndef UNREFERENCED_PARAM
#define UNREFERENCED_PARAM(x) ((void)(x))
#endif

#define DEF_IQP 32
#define DEF_PQP 32
#define ENC_TIMEOUT (-2)
#define DEAULT_QP_MAP_BITRATE 6000

#define MAX_VENC_QUEUE_DEPTH 4



int venc_create_enc_ctx(venc_enc_ctx *pEncCtx, void *pchnctx);
#if 0
int jpege_init(void);
int jpege_open(void *handle, void *pchnctx);
int jpege_close(void *ctx);
int jpege_enc_one_pic(void *ctx,
                 const video_frame_info_s *pstFrame,
                 int s32MilliSec);
void setSrcInfo(DRVFRAMEBUF *psi, venc_enc_ctx *pEncCtx,
               const video_frame_info_s *pstFrame);
int jpege_get_stream(void *ctx, venc_stream_s *pstStream,
                int s32MilliSec);
int jpege_release_stream(void *ctx, venc_stream_s *pstStream);
int jpege_ioctl(void *ctx, int op, void *arg);

int vidEnc_init(void);
int vidEnc_open(void *handle, void *pchnctx);
int vidEnc_close(void *ctx);
int vidEnc_enc_one_pic(void *ctx,
                  const video_frame_info_s *pstFrame,
                  int s32MilliSec);
int vidEnc_get_stream(void *ctx, venc_stream_s *pstStream,
                 int s32MilliSec);
int vidEnc_release_stream(void *ctx, venc_stream_s *pstStream);
int vidEnc_ioctl(void *ctx, int op, void *arg);

void h265e_setInitCfgFixQp(InitEncConfig *pInitEncCfg,
                      void *pchnctx);
void h265e_setInitCfgCbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx);
void h265e_setInitCfgVbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx);
void h265e_setInitCfgAVbr(InitEncConfig *pInitEncCfg,
                     void *pchnctx);
void h265e_setInitCfgQpMap(InitEncConfig *pInitEncCfg,
                      void *pchnctx);
void h265e_setInitCfgUbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx);
int h265e_mapNaluType(venc_pack_s *ppack, int naluType);


void set_init_cfg_gop(InitEncConfig *pInitEncCfg,
                 venc_chn_context *pChnHandle);
void h264e_setInitCfgFixQp(InitEncConfig *pInitEncCfg,
                      void *pchnctx);
void h264e_setInitCfgCbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx);
void h264e_setInitCfgVbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx);
void h264e_setInitCfgAVbr(InitEncConfig *pInitEncCfg,
                     void *pchnctx);
void h264e_setInitCfgUbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx);
int h264e_mapNaluType(venc_pack_s *ppack, int naluType);
#endif

#endif

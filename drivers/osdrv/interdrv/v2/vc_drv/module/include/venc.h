#ifndef __VENC_H__
#define __VENC_H__

#include <base_ctx.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/defines.h>
#include <linux/comm_venc.h>
#include "vbq.h"
#include "enc_ctx.h"
#include "jpeg_api.h"
#include "float_point.h"

#define VENC_MEMSET memset

typedef enum _venc_chn_STATE_ {
    venc_chn_STATE_NONE = 0,
    venc_chn_STATE_INIT,
    venc_chn_STATE_START_ENC,
    venc_chn_STATE_STOP_ENC,
} venc_chn_STATE;

typedef struct _venc_frc {
    unsigned char bFrcEnable;
    int srcFrameDur;
    int dstFrameDur;
    int srcTs;
    int dstTs;
} venc_frc;

typedef struct _venc_vfps {
    unsigned char bVfpsEnable;
    int s32NumFrmsInOneSec;
    uint64_t u64prevSec;
    uint64_t u64StatTime;
} venc_vfps;

typedef struct _venc_vb_ctx {
	unsigned char enable_bind_mode;
	unsigned char currBindMode;
	struct task_struct *thread;
	struct vb_jobs_t vb_jobs;
	unsigned char pause;
} venc_vb_ctx;

#define DRV_DEF_VFPFS_STAT_TIME 2
#define MAX_VENC_FRM_NUM 32

typedef struct _venc_chn_vars {
    uint64_t u64TimeOfSendFrame;
    uint64_t u64LastGetStreamTimeStamp;
    uint64_t u64LastSendFrameTimeStamp;
    uint64_t currPTS;
    uint64_t totalTime;
    int frameIdx;
    int s32RecvPicNum;
    int bind_event_fd;
    venc_frc frc;
    venc_vfps vfps;
    venc_stream_s stStream;
    venc_jpeg_param_s stJpegParam;
    venc_mjpeg_param_s stMjpegParam;
    venc_chn_param_s stChnParam;
    venc_chn_status_s chnStatus;
    venc_cu_prediction_s cuPrediction;
    venc_frame_param_s frameParam;
    venc_chn_STATE chnState;
    user_rc_info_s stUserRcInfo;
    venc_superframe_cfg_s stSuperFrmParam;
    struct semaphore sem_send;
    struct semaphore sem_release;
    unsigned char bAttrChange;
    venc_framelost_s frameLost;
    unsigned char bHasVbPool;
    venc_chn_pool_s vbpool;
    vb_blk vbBLK[VB_COMM_POOL_MAX_CNT];
    BufInfo FrmArray[MAX_VENC_FRM_NUM];
    unsigned int FrmNum;
    unsigned int u32SendFrameCnt;
    unsigned int u32GetStreamCnt;
    int s32BindModeGetStreamRet;
    unsigned int u32FirstPixelFormat;
    unsigned char bSendFirstFrm;
    unsigned int u32Stride[3];
    video_frame_info_s stFrameInfo;
    venc_roi_attr_s stRoiAttr[8];
    vcodec_perf_fps_s stFPS;
} venc_chn_vars;

typedef struct _venc_chn_context {
    venc_chn VeChn;
    venc_chn_attr_s *pChnAttr;
    venc_rc_param_s rcParam;
    venc_ref_param_s refParam;
    venc_framelost_s frameLost;
    venc_h264_entropy_s h264Entropy;
    venc_h264_trans_s h264Trans;
    venc_h265_trans_s h265Trans;
    venc_h265_pu_s    h265PredUnit;
    venc_search_window_s stSearchWindow;
    venc_h264_slice_split_s h264Split;
    venc_h265_slice_split_s h265Split;
    union {
        venc_h264_vui_s h264Vui;
        venc_h265_vui_s h265Vui;
    };
    venc_h264_intra_pred_s h264IntraPred;
    venc_enc_ctx encCtx;
    venc_chn_vars *pChnVars;
    venc_vb_ctx *pVbCtx;
    struct mutex chnMutex;
    struct mutex chnShmMutex;
    unsigned char bSbSkipFrm;
    unsigned int jpgFrmSkipCnt;
    video_frame_info_s stVideoFrameInfo;
    unsigned char bChnEnable;
    union {
        venc_h264_dblk_s h264Dblk;
        venc_h265_dblk_s h265Dblk;
    };
    venc_custom_map_s customMap;
    venc_h265_sao_s h265Sao;
} venc_chn_context;

typedef struct _DRV_VENC_MODPARAM_S {
    venc_mod_venc_s stVencModParam;
    venc_mod_h264e_s stH264eModParam;
    venc_mod_h265e_s stH265eModParam;
    venc_mod_jpege_s stJpegeModParam;
    venc_mod_rc_s stRcModParam;
} drv_venc_param_mod_s;

typedef struct _venc_context {
    venc_chn_context * chn_handle[VENC_MAX_CHN_NUM];
    unsigned int chn_status[VENC_MAX_CHN_NUM];
    drv_venc_param_mod_s ModParam;
} venc_context;


#define Q_TABLE_MAX 99
#define Q_TABLE_CUSTOM 50
#define Q_TABLE_MIN 1
#define Q_TABLE_DEFAULT 1 // 0 = backward compatible
#define SRC_FRAMERATE_DEF 30
#define SRC_FRAMERATE_MAX 300
#define SRC_FRAMERATE_MIN 1
#define DEST_FRAMERATE_DEF 30
#define DEST_FRAMERATE_MAX 300
#define DEST_FRAMERATE_MIN 1
#define DRV_VENC_NO_INPUT -10
#define DRV_VENC_INPUT_ERR -11
#define DUMP_YUV "dump_src.yuv"
#define DUMP_BS "dump_bs.bin"
#ifndef SEC_TO_MS
#define SEC_TO_MS 1000
#endif
#define USERDATA_MAX_DEFAULT 1024
#define USERDATA_MAX_LIMIT 65536
#define DEFAULT_NO_INPUTDATA_TIMEOUT_SEC (5)
#define BYPASS_SB_MODE (0)
#define CLK_ENABLE_REG_2_BASE (0x03002008)
#define VC_FAB_REG_9_BASE (0xb030024)

// below should align to cv183x_vcodec.h
#define VPU_MISCDEV_NAME "/dev/vcodec"
#define VCODEC_VENC_SHARE_MEM_SIZE (0x30000) // 192k
#define VCODEC_VDEC_SHARE_MEM_SIZE (0x8000) // 32k
#define VCODEC_SHARE_MEM_SIZE                                                  \
    (VCODEC_VENC_SHARE_MEM_SIZE + VCODEC_VDEC_SHARE_MEM_SIZE)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define FRC_TIME_SCALE 0xFFF0
#if SOFT_FLOAT
#define FLOAT_VAL_FRC_TIME_SCALE (0x477ff000)
#else
#define FLOAT_VAL_FRC_TIME_SCALE (FRC_TIME_SCALE)
#endif
#define FRC_TIME_OVERFLOW_OFFSET 0x40000000


#define SET_DEFAULT_RC_PARAM(RC)                                        \
    do {                                                                \
        (RC)->u32MaxIprop = DRV_H26X_MAX_I_PROP_DEFAULT;               \
        (RC)->u32MinIprop = DRV_H26X_MIN_I_PROP_DEFAULT;               \
        (RC)->u32MaxIQp = DEF_264_MAXIQP;                              \
        (RC)->u32MinIQp = DEF_264_MINIQP;                              \
        (RC)->u32MaxQp = DEF_264_MAXQP;                                \
        (RC)->u32MinQp = DEF_264_MINQP;                                \
    } while (0)

#define SET_COMMON_RC_PARAM(DEST, SRC)                                 \
    do {                                                               \
        (DEST)->u32MaxIprop = (SRC)->u32MaxIprop;                      \
        (DEST)->u32MinIprop = (SRC)->u32MinIprop;                      \
        (DEST)->u32MaxIQp = (SRC)->u32MaxIQp;                          \
        (DEST)->u32MinIQp = (SRC)->u32MinIQp;                          \
        (DEST)->u32MaxQp = (SRC)->u32MaxQp;                            \
        (DEST)->u32MinQp = (SRC)->u32MinQp;                            \
        (DEST)->s32MaxReEncodeTimes = (SRC)->s32MaxReEncodeTimes;      \
    } while (0)

#if 0
#define IF_WANNA_DISABLE_BIND_MODE()                                           \
    ((pVbCtx->currBindMode == 1) &&                                 \
     (pVbCtx->enable_bind_mode == 0))
#define IF_WANNA_ENABLE_BIND_MODE()                                            \
    ((pVbCtx->currBindMode == 0) &&                                \
     (pVbCtx->enable_bind_mode == 1))
#endif




#endif

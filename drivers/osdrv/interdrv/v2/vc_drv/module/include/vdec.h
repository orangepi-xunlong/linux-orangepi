#ifndef __VDEC_H__
#define __VDEC_H__

#include <linux/defines.h>

#define DISPLAY_QUEUE_SIZE 32
#define MAX_VDEC_FRM_NUM 32

#define MAX_DEC_PIC_WIDTH               4096
#define MAX_DEC_PIC_HEIGHT              2304


typedef struct _vdec_dbg_ {
    int startFn;
    int endFn;
    int dbgMask;
    int currMask;
    char dbgDir[DRV_VDEC_STR_LEN];
} vdec_dbg;

extern uint32_t VDEC_LOG_LV;

#define DRV_VDEC_FUNC_COND(FLAG, FUNC)			\
    do {                                    \
        if (VDEC_LOG_LV & (FLAG)) {    \
            FUNC;                            \
        }                                    \
    } while (0)


#define DRV_VDEC_PRNT(msg, ...)	\
            pr_info(msg, ##__VA_ARGS__)

#define DRV_VDEC_ERR(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & DRV_VDEC_MASK_ERR) \
        pr_err("[ERR] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VDEC_WARN(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & DRV_VDEC_MASK_WARN) \
        pr_warn("[WARN] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VDEC_DISP(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & DRV_VDEC_MASK_DISP) \
        pr_notice("[DISP] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VDEC_INFO(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & DRV_VDEC_MASK_INFO) \
        pr_info("[INFO] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VDEC_MEM(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & DRV_VDEC_MASK_MEM) \
        pr_info("[MEM] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VDEC_API(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & DRV_VDEC_MASK_API) \
        pr_info("[API] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VDEC_TRACE(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & DRV_VDEC_MASK_TRACE) \
        pr_debug("[TRACE] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VDEC_PERF(msg, ...)		\
    do { \
        if (VDEC_LOG_LV & DRV_VDEC_MASK_PERF) \
        pr_notice("[PERF] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)

#define DRV_TRACE_VDEC(level, fmt, ...)                                           \
    DRV_TRACE(level, ID_VDEC , "%s:%d:%s(): " fmt, __FILENAME__, __LINE__, __func__, ##__VA_ARGS__)


typedef struct _vdec_chn_context {
    vdec_chn VdChn;
    uint64_t totalDecTime;
    vdec_chn_attr_s ChnAttr;
    vdec_chn_param_s ChnParam;
    vdec_chn_status_s stStatus;
    video_frame_info_s *VideoFrameArray;
    unsigned int VideoFrameArrayNum;
    unsigned char display_queue[DISPLAY_QUEUE_SIZE];
    unsigned int w_idx;
    unsigned int r_idx;
    unsigned int seqNum;
    struct mutex display_queue_lock;
    struct mutex status_lock;
    struct mutex chnShmMutex;
    void *pHandle;
    unsigned char bHasVbPool;
    vdec_chn_pool_s vbPool;
    vb_blk vbBLK[MAX_VDEC_FRM_NUM];
    BufInfo FrmArray[MAX_VDEC_FRM_NUM];
    unsigned int FrmNum;
    unsigned char bStartRecv;
    struct drv_vdec_vb_ctx *pVbCtx;

    uint64_t u64LastSendStreamTimeStamp;
    uint64_t u64LastGetFrameTimeStamp;
    unsigned int u32SendStreamCnt;
    unsigned int u32GetFrameCnt;
    video_frame_info_s stVideoFrameInfo;
    vcodec_perf_fps_s stFPS;
    struct mutex jpdLock;
} vdec_chn_context;

typedef struct _vdec_context {
    vdec_chn_context *chn_handle[VDEC_MAX_CHN_NUM];
    vdec_mod_param_s g_stModParam;
} vdec_context;

#define DRV_VDEC_NO_INPUT -10
#define DRV_VDEC_INPUT_ERR -11

#define VDEC_TIME_BLOCK_MODE (-1)
#define VDEC_RET_TIMEOUT (-2)
#define VDEC_TIME_TRY_MODE (0)
#define VDEC_DEFAULT_MUTEX_MODE VDEC_TIME_BLOCK_MODE

// below should align to cv183x_vcodec.h
#define VPU_MISCDEV_NAME "/dev/vcodec"
#define VCODEC_VENC_SHARE_MEM_SIZE (0x30000) // 192k
#define VCODEC_VDEC_SHARE_MEM_SIZE (0x8000) // 32k
#define VCODEC_SHARE_MEM_SIZE                                                  \
    (VCODEC_VENC_SHARE_MEM_SIZE + VCODEC_VDEC_SHARE_MEM_SIZE)
#ifndef SEC_TO_MS
#define SEC_TO_MS (1000)
#endif
#ifndef ALIGN
#define ALIGN(x, a) (((x) + ((a)-1)) & ~((a)-1))
#endif
#define UNUSED(x) ((void)(x))

#define IF_WANNA_DISABLE_BIND_MODE()                                           \
    ((pVbCtx->currBindMode == 1) &&                                 \
     (pVbCtx->enable_bind_mode == 0))
#define IF_WANNA_ENABLE_BIND_MODE()                                            \
    ((pVbCtx->currBindMode == 0) &&                                \
     (pVbCtx->enable_bind_mode == 1))


#endif

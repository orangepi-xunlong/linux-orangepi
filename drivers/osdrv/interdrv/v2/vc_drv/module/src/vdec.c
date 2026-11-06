#ifdef ENABLE_DEC
#include "drv_vdec.h"
#include <base_ctx.h>
#include <linux/comm_buffer.h>
#include <linux/defines.h>
#include "module_common.h"
#include "vc_drv_proc.h"
#include "jpeg_api.h"
#include "h265_interface.h"
#include <linux/comm_vdec.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <uapi/linux/sched/types.h>
#include "vc_drv.h"

#include "vdec.h"
#include "drv_file.h"


vdec_dbg vdecDbg;
vdec_context *vdec_handle;

uint32_t VDEC_LOG_LV = DRV_VDEC_MASK_ERR;
module_param(VDEC_LOG_LV, int, 0644);
extern wait_queue_head_t tVdecWaitQueue[];

static DEFINE_MUTEX(g_vdec_handle_mutex);

static uint64_t get_current_time(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    struct timespec64 ts;
#else
    struct timespec ts;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ktime_get_ts64(&ts);
#else
    ktime_get_ts(&ts);
#endif
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // in ms
}

static void change_vdec_mask(int frameIdx)
{
    vdec_dbg *pDbg = &vdecDbg;

    pDbg->currMask = pDbg->dbgMask;
    if (pDbg->startFn >= 0) {
        if (frameIdx >= pDbg->startFn && frameIdx <= pDbg->endFn)
            pDbg->currMask = pDbg->dbgMask;
        else
            pDbg->currMask = DRV_VDEC_MASK_ERR;
    }

    DRV_VDEC_TRACE("currMask = 0x%X\n", pDbg->currMask);
}

static int check_vdec_chn_handle(vdec_chn VdChn)
{
    if (vdec_handle == NULL) {
        DRV_VDEC_ERR("Call VDEC Destroy before Create chn:%d, failed\n", VdChn);
        return DRV_ERR_VDEC_UNEXIST;
    }

    if (vdec_handle->chn_handle[VdChn] == NULL) {
        DRV_VDEC_ERR("VDEC Chn #%d haven't created !\n", VdChn);
        return DRV_ERR_VDEC_INVALID_CHNID;
    }

    return 0;
}

static int h26x_decode(void *pHandle, pixel_format_e enPixelFormat,
                   const vdec_stream_s *pstStream,
                   int s32MilliSec)
{
    DecOnePicCfg dopc, *pdopc = &dopc;
    int decStatus = 0;

    pdopc->bsAddr = pstStream->pu8Addr;
    pdopc->bsLen = pstStream->u32Len;
    pdopc->bEndOfStream = pstStream->bEndOfStream;
    pdopc->pts = pstStream->u64PTS;
    pdopc->dts = pstStream->u64DTS;

    switch (enPixelFormat) {
    case PIXEL_FORMAT_NV12:
        pdopc->cbcrInterleave = 1;
        pdopc->nv21 = 0;
        break;
    case PIXEL_FORMAT_NV21:
        pdopc->cbcrInterleave = 1;
        pdopc->nv21 = 1;
        break;
    default:
        pdopc->cbcrInterleave = 0;
        pdopc->nv21 = 0;
        break;
    }

    decStatus = vdec_decode_frame(pHandle, pdopc, s32MilliSec);

    return decStatus;
}

static void set_video_frame_info(video_frame_info_s *pstVideoFrame,
                 DispFrameCfg *pdfc)
{
    video_frame_s *pstVFrame = &pstVideoFrame->video_frame;

    pstVFrame->pixel_format =
        (pdfc->cbcrInterleave) ?
                  ((pdfc->nv21) ? PIXEL_FORMAT_NV21 : PIXEL_FORMAT_NV12) :
                  PIXEL_FORMAT_YUV_PLANAR_420;

    pstVFrame->viraddr[0] = pdfc->addr[0];
    pstVFrame->viraddr[1] = pdfc->addr[1];
    pstVFrame->viraddr[2] = pdfc->addr[2];

    pstVFrame->phyaddr[0] = pdfc->phyAddr[0];
    pstVFrame->phyaddr[1] = pdfc->phyAddr[1];
    pstVFrame->phyaddr[2] = pdfc->phyAddr[2];

    pstVFrame->width = pdfc->width;
    pstVFrame->height = pdfc->height;

    pstVFrame->stride[0] = pdfc->strideY;
    pstVFrame->stride[1] = pdfc->strideC;
    pstVFrame->stride[2] = pdfc->cbcrInterleave ? 0 : pdfc->strideC;
    pstVFrame->length[0] = pdfc->length[0];
    pstVFrame->length[1] = pdfc->length[1];
    pstVFrame->length[2] = pdfc->length[2];

    if (pdfc->bCompressFrame) {
        pstVFrame->compress_mode = COMPRESS_MODE_FRAME;
        pstVFrame->ext_virt_addr = pdfc->addr[3];
        pstVFrame->ext_phy_addr = pdfc->phyAddr[3];
        pstVFrame->ext_length = pdfc->length[3];
    } else {
        pstVFrame->compress_mode = COMPRESS_MODE_NONE;
        pstVFrame->ext_virt_addr = 0;
        pstVFrame->ext_phy_addr = 0;
        pstVFrame->ext_length = 0;
    }

    pstVFrame->offset_top = 0;
    pstVFrame->offset_bottom = 0;
    pstVFrame->offset_left = 0;
    pstVFrame->offset_right = 0;
    pstVFrame->pts = pdfc->pts;
    pstVFrame->dts = pdfc->dts;
    pstVFrame->private_data = (void *)(uintptr_t)pdfc->indexFrameDisplay;
    pstVFrame->endian = pdfc->endian;
    pstVFrame->interl_aced_frame = pdfc->interlacedFrame;
    pstVFrame->pic_type = pdfc->picType;
    pstVFrame->seqenceno = pdfc->seqenceNo;
    pstVFrame->time_ref = pdfc->seqenceNo;
}

static int set_video_chnattr_toproc(vdec_chn VdChn,
                    video_frame_info_s *psFrameInfo,
                    uint64_t u64DecHwTime)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (psFrameInfo == NULL) {
        DRV_VDEC_ERR("psFrameInfo is NULL\n");
        return -2;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(&pChnHandle->stVideoFrameInfo, psFrameInfo,
           sizeof(video_frame_info_s));
    pChnHandle->stFPS.hw_time = u64DecHwTime;

    return s32Ret;
}

static int set_vdec_fps_toproc(vdec_chn VdChn, unsigned char bSendStream)
{
    int s32Ret = 0;
    uint64_t u64CurTime = get_current_time();
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    if (bSendStream) {
        if ((u64CurTime - pChnHandle->u64LastSendStreamTimeStamp) >
            SEC_TO_MS) {
            pChnHandle->stFPS.in_fps =
                (unsigned int)((pChnHandle->u32SendStreamCnt *
                       SEC_TO_MS) /
                      ((unsigned int)(u64CurTime -
                       pChnHandle->u64LastSendStreamTimeStamp)));
            pChnHandle->u64LastSendStreamTimeStamp = u64CurTime;
            pChnHandle->u32SendStreamCnt = 0;
        }
    } else {
        if ((u64CurTime - pChnHandle->u64LastGetFrameTimeStamp) >
            SEC_TO_MS) {
            pChnHandle->stFPS.out_fps =
                (unsigned int)((pChnHandle->u32GetFrameCnt *
                       SEC_TO_MS) /
                      ((unsigned int)(u64CurTime -
                        pChnHandle->u64LastGetFrameTimeStamp)));
            pChnHandle->u64LastGetFrameTimeStamp = u64CurTime;
            pChnHandle->u32GetFrameCnt = 0;
        }
    }

    return s32Ret;
}


static inline int check_timeout_and_busy(int s32Ret, int line)
{
    if (s32Ret == 0)
        return s32Ret;

    if ((s32Ret == ETIMEDOUT) || (s32Ret == EBUSY)) {
        DRV_VDEC_TRACE("mutex timeout and retry\n");
        return  DRV_ERR_VDEC_BUSY;
    }

    DRV_VDEC_ERR("vdec mutex error[%d], line = %d\n", s32Ret, line);
    return DRV_ERR_VDEC_ERR_VDEC_MUTEX;
}


static int vdec_mutex_unlock(struct mutex *__mutex)
{
    mutex_unlock(__mutex);
    return 0;
}

static int vdec_mutex_lock(struct mutex *__restrict __mutex,
                  int s32MilliSec, int *s32CostTime)
{
    int s32RetCostTime;
    int s32RetCheck;
    uint64_t u64StartTime, u64EndTime;

    u64StartTime = get_current_time();
    if (s32MilliSec < 0) {
        //block mode
        s32RetCheck = mutex_lock_interruptible(__mutex);
        u64EndTime = get_current_time();
        s32RetCostTime = u64EndTime - u64StartTime;
    } else if (s32MilliSec == 0) {
        //trylock
        if (mutex_trylock(__mutex)) {
            s32RetCheck = 0;
        } else {
            s32RetCheck = EBUSY;
        }
        s32RetCostTime = 0;
    } else {
        //timelock
        int wait_cnt_ms = 1;
        while (mutex_is_locked(__mutex)) {
            set_current_state(TASK_INTERRUPTIBLE);
            schedule_timeout(usecs_to_jiffies(1000));
            if (wait_cnt_ms >= s32MilliSec) {
                break;
            }
            wait_cnt_ms++;
        }
        if (mutex_trylock(__mutex)) {
            s32RetCheck = 0;
        } else {
            s32RetCheck = EBUSY;
        }
        u64EndTime = get_current_time();
        s32RetCostTime = u64EndTime - u64StartTime;
    }
    //calculate cost lock time

    //check the mutex validity
    if (s32RetCheck != 0) {
        DRV_VDEC_ERR("mutex lock error[%d]\n", s32RetCheck);
    }

    if (s32CostTime != NULL)
        *s32CostTime = s32RetCostTime;
    return s32RetCheck;
}

int vdec_init_handle(void)
{
    int s32Ret = 0;

    if (MUTEX_LOCK(&g_vdec_handle_mutex) != 0) {
        DRV_VDEC_ERR("can not lock g_vdec_handle_mutex\n");
        return -1;
    }

    if (vdec_handle == NULL) {
        vdec_handle = MEM_CALLOC(1, sizeof(vdec_context));

        if (vdec_handle == NULL) {
            MUTEX_UNLOCK(&g_vdec_handle_mutex);
            return DRV_ERR_VDEC_NOMEM;
        }

        memset(vdec_handle, 0x00, sizeof(vdec_context));
        vdec_handle->g_stModParam.enVdecVBSource =
            VB_SOURCE_COMMON; // Default is VB_SOURCE_COMMON
        vdec_handle->g_stModParam.u32MiniBufMode = 0;
        vdec_handle->g_stModParam.u32ParallelMode = 0;
        vdec_handle->g_stModParam.stVideoModParam.u32MaxPicHeight =
            8192;
        vdec_handle->g_stModParam.stVideoModParam.u32MaxPicWidth = 8192;
        vdec_handle->g_stModParam.stVideoModParam.u32MaxSliceNum = 200;
        vdec_handle->g_stModParam.stPictureModParam.u32MaxPicHeight =
            32768;
        vdec_handle->g_stModParam.stPictureModParam.u32MaxPicWidth =
            32768;
    }

    MUTEX_UNLOCK(&g_vdec_handle_mutex);

    return s32Ret;
}


int vdec_drv_init(void)
{
    int s32Ret = 0;

    s32Ret = vdec_init_handle();
    if (s32Ret != 0) {
        DRV_VENC_ERR("venc_context\n");
        s32Ret = DRV_ERR_VDEC_NOMEM;
    }

    vdec_init_handle_pool();
    return s32Ret;
}

void vdec_drv_deinit(void)
{
    if(vdec_handle)
        MEM_FREE(vdec_handle);
    vdec_handle = NULL;
    vdec_deinit_handle_pool();

    return;
}

static void get_debug_config_from_proc(void)
{
    extern proc_debug_config_t tVdecDebugConfig;
    proc_debug_config_t *ptDecProcDebugLevel = &tVdecDebugConfig;
    vdec_dbg *pDbg = &vdecDbg;

    DRV_VDEC_TRACE("\n");

    if (ptDecProcDebugLevel == NULL) {
        DRV_VDEC_ERR("ptDecProcDebugLevel is NULL\n");
        return;
    }

    memset(pDbg, 0, sizeof(vdec_dbg));

    pDbg->dbgMask = ptDecProcDebugLevel->u32DbgMask;
    if (pDbg->dbgMask == DRV_VDEC_NO_INPUT ||
        pDbg->dbgMask == DRV_VDEC_INPUT_ERR)
        pDbg->dbgMask = DRV_VDEC_MASK_ERR;
    else
        pDbg->dbgMask |= DRV_VDEC_MASK_ERR;

    pDbg->currMask = pDbg->dbgMask;
    pDbg->startFn = ptDecProcDebugLevel->u32StartFrmIdx;
    pDbg->endFn = ptDecProcDebugLevel->u32EndFrmIdx;
    strcpy(pDbg->dbgDir, ptDecProcDebugLevel->cDumpPath);

    change_vdec_mask(0);
}

static void jpeg_set_frame_info(video_frame_s *pVideoFrame,
                    DRVFRAMEBUF *pFrameBuf)
{
    int c_h_shift = 0; // chroma height shift

    pVideoFrame->width = pFrameBuf->width;
    pVideoFrame->height = pFrameBuf->height;
    pVideoFrame->stride[0] = pFrameBuf->strideY;
    pVideoFrame->stride[1] = pFrameBuf->strideC;
    pVideoFrame->stride[2] = pFrameBuf->strideC;
    pVideoFrame->offset_top = 0;
    pVideoFrame->offset_bottom = 0;
    pVideoFrame->offset_left = 0;
    pVideoFrame->offset_right = 0;

    switch (pFrameBuf->format) {
    case DRV_FORMAT_400:
        pVideoFrame->pixel_format = PIXEL_FORMAT_YUV_400;
        pVideoFrame->stride[2] = pFrameBuf->strideC;
        break;
    case DRV_FORMAT_422:
        if(pFrameBuf->packedFormat == DRV_PACKED_FORMAT_422_YUYV) {
            pVideoFrame->pixel_format = PIXEL_FORMAT_YUYV;
        } else if(pFrameBuf->packedFormat == DRV_PACKED_FORMAT_422_UYVY) {
            pVideoFrame->pixel_format = PIXEL_FORMAT_UYVY;
        } else if(pFrameBuf->packedFormat == DRV_PACKED_FORMAT_422_YVYU) {
            pVideoFrame->pixel_format = PIXEL_FORMAT_YVYU;
        } else if(pFrameBuf->packedFormat == DRV_PACKED_FORMAT_422_VYUY) {
            pVideoFrame->pixel_format = PIXEL_FORMAT_VYUY;
        } else if (pFrameBuf->packedFormat == DRV_PACKED_FORMAT_NONE){
            if(pFrameBuf->chromaInterleave == DRV_CBCR_INTERLEAVE) {
                pVideoFrame->pixel_format = PIXEL_FORMAT_NV16;
            } else if(pFrameBuf->chromaInterleave == DRV_CRCB_INTERLEAVE) {
                pVideoFrame->pixel_format = PIXEL_FORMAT_NV61;
            } else
                pVideoFrame->pixel_format = PIXEL_FORMAT_YUV_PLANAR_422;
        }
        pVideoFrame->stride[2] = pFrameBuf->strideC;
        break;
    case DRV_FORMAT_444:
        pVideoFrame->pixel_format = PIXEL_FORMAT_YUV_PLANAR_444;
        pVideoFrame->stride[2] = pFrameBuf->strideC;
        break;
    case DRV_FORMAT_420:
    default:
        c_h_shift = 1;
        if (pFrameBuf->chromaInterleave == DRV_CBCR_INTERLEAVE) {
            pVideoFrame->pixel_format = PIXEL_FORMAT_NV12;
            pVideoFrame->stride[2] = 0;
        } else if (pFrameBuf->chromaInterleave ==
               DRV_CRCB_INTERLEAVE) {
            pVideoFrame->pixel_format = PIXEL_FORMAT_NV21;
            pVideoFrame->stride[2] = 0;
        } else { // DRV_CBCR_SEPARATED
            pVideoFrame->pixel_format =
                PIXEL_FORMAT_YUV_PLANAR_420;
            pVideoFrame->stride[2] = pFrameBuf->strideC;
        }
        break;
    }

    pVideoFrame->length[0] = pFrameBuf->vbY.size;
    pVideoFrame->length[1] = pFrameBuf->vbCb.size;
    pVideoFrame->length[2] = pFrameBuf->vbCr.size;
    pVideoFrame->phyaddr[0] = pFrameBuf->vbY.phys_addr;
    pVideoFrame->viraddr[0] = pFrameBuf->vbY.virt_addr;
    pVideoFrame->phyaddr[1] = pFrameBuf->vbCb.phys_addr;
    pVideoFrame->viraddr[1] = pFrameBuf->vbCb.virt_addr;
    pVideoFrame->phyaddr[2] = pFrameBuf->vbCr.phys_addr;
    pVideoFrame->viraddr[2] = pFrameBuf->vbCr.virt_addr;

    DRV_VDEC_INFO(
        "jpeg dec fmt:%d %dx%d, strideY %d, strideC %d, sizeY %d, sizeC %d\n",
        pVideoFrame->pixel_format,
        pVideoFrame->width, pVideoFrame->height,
        pVideoFrame->stride[0], pVideoFrame->stride[1],
        pVideoFrame->length[0], pVideoFrame->length[1]);
}

static int jpeg_decode(vdec_chn_context *pChnHandle, const vdec_stream_s *pstStream,
                   int s32MilliSec, uint64_t *pu64DecHwTime)
{
    int ret = 0;

    /* send jpeg data for decode or encode operator */
    ret = jpeg_dec_send_stream(pChnHandle->pHandle, pstStream->pu8Addr, pstStream->u32Len, s32MilliSec);
    if (ret != 0) {
        if ((ret == VDEC_RET_TIMEOUT) && (s32MilliSec >= 0)) {
            DRV_VDEC_TRACE("jpeg_dec_send_stream ret timeout\n");
            return DRV_ERR_VDEC_BUSY;

        } else {
            DRV_VDEC_ERR("Failed to jpeg_dec_send_stream, ret = %d\n", ret);
            return DRV_ERR_VDEC_ERR_SEND_FAILED;
        }
    }

    return ret;
}


int vdec_get_output_frame_count(vdec_chn VdChn)
{
    vdec_chn_context *pChnHandle = NULL;
    int s32Ret = 0;

    if (vdec_handle == NULL) {
        return -1;
    }

    if (vdec_handle->chn_handle[VdChn] == NULL) {
        return -2;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    if(pChnHandle->ChnAttr.enType == PT_JPEG
        || pChnHandle->ChnAttr.enType == PT_MJPEG)
        s32Ret = pChnHandle->stStatus.u32LeftPics;
    else
        s32Ret = vdec_get_display_frame_count(pChnHandle->pHandle);

    return s32Ret;
}
static int vdec_init(void)
{
    int ret = 0;

    DRV_VDEC_API("\n");

    get_debug_config_from_proc();

    return ret;
}

#define MAX_VDEC_DISPLAYQ_NUM 32

int drv_vdec_create_chn(vdec_chn VdChn, const vdec_chn_attr_s *pstAttr)
{
    vdec_chn_context *pChnHandle = NULL;
    vdec_chn_attr_s *pstChnAttr;
    drv_jpg_config config;
    int s32Ret = 0;

    DRV_VDEC_API("VdChn = %d\n", VdChn);

    vdec_init();

    vdec_handle->chn_handle[VdChn] =
        MEM_CALLOC(1, sizeof(vdec_chn_context));
    if (vdec_handle->chn_handle[VdChn] == NULL) {
        DRV_VDEC_ERR("Allocate chn_handle memory failed !\n");
        return DRV_ERR_VDEC_NOMEM;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    pChnHandle->VdChn = VdChn;
    pstChnAttr = &pChnHandle->ChnAttr;
    memcpy(&pChnHandle->ChnAttr, pstAttr, sizeof(vdec_chn_attr_s));
    pChnHandle->VideoFrameArrayNum = MAX_VDEC_DISPLAYQ_NUM;

    pChnHandle->VideoFrameArray = MEM_CALLOC(pChnHandle->VideoFrameArrayNum,
                         sizeof(video_frame_info_s));

    if (pChnHandle->VideoFrameArray == NULL) {
        DRV_VDEC_ERR("Allocate VideoFrameArray memory failed !\n");
        return DRV_ERR_VDEC_NOMEM;
    }

    DRV_VDEC_TRACE("u32FrameBufCnt = %d\n",
               pChnHandle->ChnAttr.u32FrameBufCnt);
    DRV_VDEC_TRACE("VideoFrameArrayNum = %d\n",
               pChnHandle->VideoFrameArrayNum);

    memset(pChnHandle->display_queue, -1, DISPLAY_QUEUE_SIZE);

    MUTEX_INIT(&pChnHandle->display_queue_lock, 0);
    MUTEX_INIT(&pChnHandle->status_lock, 0);
    MUTEX_INIT(&pChnHandle->chnShmMutex, &ma);

    if (pChnHandle->ChnAttr.enType == PT_JPEG ||
        pChnHandle->ChnAttr.enType == PT_MJPEG) {
        pChnHandle->pHandle = jpeg_dec_init();
        if (pChnHandle->pHandle == NULL) {
            DRV_VDEC_ERR("\nFailed to jpeg_init\n");
            return -1;
        }

        memset(&config, 0, sizeof(drv_jpg_config));
        config.type = JPGCOD_DEC;
        config.u.dec.dec_buf.format = DRV_FORMAT_BUTT;
        config.u.dec.dec_buf.packedFormat = DRV_PACKED_FORMAT_NONE;
        config.u.dec.dec_buf.chromaInterleave = DRV_CBCR_SEPARATED;
        config.u.dec.usePartialMode = 0;
        config.u.dec.iDataLen = pstChnAttr->u32StreamBufSize;
        if(pstChnAttr->stBufferInfo.bitstream_buffer) {
            config.u.dec.external_bs_addr = pstChnAttr->stBufferInfo.bitstream_buffer->phys_addr;
            config.u.dec.external_bs_size = pstChnAttr->stBufferInfo.bitstream_buffer->size;
        }
        if(pstChnAttr->stBufferInfo.frame_buffer) {
            config.u.dec.external_fb_addr = pstChnAttr->stBufferInfo.frame_buffer->phys_addr;
            config.u.dec.external_fb_size = pstChnAttr->stBufferInfo.frame_buffer->size;
        }

        config.s32ChnNum = pChnHandle->VdChn;

        /* Open JPU Devices */
        s32Ret = jpeg_dec_open(pChnHandle->pHandle, config);
        if (s32Ret != 0) {
            DRV_VDEC_ERR("\nFailed to jpeg_dec_open, ret:%d\n", s32Ret);
            return s32Ret;
        }
    } else if (pChnHandle->ChnAttr.enType == PT_H264 ||
           pChnHandle->ChnAttr.enType == PT_H265) {
        InitDecConfig initDecCfg, *pInitDecCfg;

        pInitDecCfg = &initDecCfg;
        memset(pInitDecCfg, 0, sizeof(InitDecConfig));
        pInitDecCfg->codec = (pChnHandle->ChnAttr.enType == PT_H265) ?
                           CODEC_H265 :
                           CODEC_H264;
        pInitDecCfg->api_mode = API_MODE_SDK;

        pInitDecCfg->vbSrcMode =
            vdec_handle->g_stModParam.enVdecVBSource;
        pInitDecCfg->chnNum = VdChn;
        pInitDecCfg->bsBufferSize = pChnHandle->ChnAttr.u32StreamBufSize;
        pInitDecCfg->frameBufferCount = pChnHandle->ChnAttr.u32FrameBufCnt;

        if (pstAttr->enMode == VIDEO_MODE_STREAM)
            pInitDecCfg->BsMode = BS_MODE_INTERRUPT;
        else if (pstAttr->enMode == VIDEO_MODE_FRAME)
            pInitDecCfg->BsMode = BS_MODE_PIC_END;
        else
            return -1;

        if (pstAttr->enCompressMode == COMPRESS_MODE_FRAME)
            pInitDecCfg->wtl_enable = 0;
        else
            pInitDecCfg->wtl_enable = 1;

        pInitDecCfg->cmdQueueDepth = pChnHandle->ChnAttr.u8CommandQueueDepth;
        pInitDecCfg->reorder_enable = pChnHandle->ChnAttr.u8ReorderEnable;

        pInitDecCfg->picWidth = pChnHandle->ChnAttr.u32PicWidth;
        pInitDecCfg->picHeight = pChnHandle->ChnAttr.u32PicHeight;

        pInitDecCfg->bitstream_buffer = pstAttr->stBufferInfo.bitstream_buffer;
        pInitDecCfg->frame_buffer = pstAttr->stBufferInfo.frame_buffer;
        pInitDecCfg->Ytable_buffer = pstAttr->stBufferInfo.Ytable_buffer;
        pInitDecCfg->Ctable_buffer = pstAttr->stBufferInfo.Ctable_buffer;
        pInitDecCfg->numOfDecFbc = pstAttr->stBufferInfo.numOfDecFbc;
        pInitDecCfg->numOfDecwtl = pstAttr->stBufferInfo.numOfDecwtl;

        s32Ret = vdec_open(pInitDecCfg, &pChnHandle->pHandle);
        if (s32Ret != 0) {
            DRV_VDEC_ERR("vdec_open, %d\n", s32Ret);
            return s32Ret;
        }

    }

    return 0;
}

int drv_vdec_destroy_chn(vdec_chn VdChn)
{
    int s32Ret = 0;
    struct drv_vdec_vb_ctx *pVbCtx = NULL;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    if (vdec_handle == NULL || vdec_handle->chn_handle[VdChn] == NULL) {
        DRV_VDEC_INFO("VdChn: %d already destoryed.\n", VdChn);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    pVbCtx = pChnHandle->pVbCtx;

    if (pChnHandle->ChnAttr.enType == PT_H264 ||
        pChnHandle->ChnAttr.enType == PT_H265) {
        if (pChnHandle->pHandle != NULL) {
            s32Ret = vdec_close(pChnHandle->pHandle);
            pChnHandle->pHandle = NULL;
        }
    }else {
        if (pChnHandle->pHandle != NULL) {
            jpeg_dec_close(pChnHandle->pHandle);
            jpeg_dec_deinit(pChnHandle->pHandle);
            pChnHandle->pHandle = NULL;
        }
    }

    MUTEX_DESTROY(&pChnHandle->display_queue_lock);
    MUTEX_DESTROY(&pChnHandle->status_lock);
    MUTEX_DESTROY(&pChnHandle->chnShmMutex);

    if (pChnHandle->VideoFrameArray) {
        MEM_FREE(pChnHandle->VideoFrameArray);
        pChnHandle->VideoFrameArray = NULL;
    }

    if (vdec_handle->chn_handle[VdChn]) {
        MEM_FREE(vdec_handle->chn_handle[VdChn]);
        vdec_handle->chn_handle[VdChn] = NULL;
    }

    {
        vdec_chn i = 0;
        unsigned char bFreeVdecHandle = 1;

        for (i = 0; i < VDEC_MAX_CHN_NUM; i++) {
            if (vdec_handle->chn_handle[i] != NULL) {
                bFreeVdecHandle = 0;
                break;
            }
        }

    }

    return s32Ret;
}

int drv_vdec_set_chn_param(vdec_chn VdChn, const vdec_chn_param_s *pstParam)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(&pChnHandle->ChnParam, pstParam, sizeof(vdec_chn_param_s));
    if ((pChnHandle->ChnAttr.enType == PT_H264) ||  (pChnHandle->ChnAttr.enType == PT_H265)) {
        if (pstParam->enPixelFormat == PIXEL_FORMAT_NV12)
            set_cbcr_format(pChnHandle->pHandle, 1, 0);
        else if (pstParam->enPixelFormat == PIXEL_FORMAT_NV21)
            set_cbcr_format(pChnHandle->pHandle, 1, 1);
        else
            set_cbcr_format(pChnHandle->pHandle, 0, 0);
    } else if ((pChnHandle->ChnAttr.enType == PT_JPEG) ||  (pChnHandle->ChnAttr.enType == PT_MJPEG)) {
        vdec_chn_param_s *pstChnParam = &pChnHandle->ChnParam;
        drv_jpg_config config = {0};

        config.type = JPGCOD_DEC;
        config.u.dec.dec_buf.format = DRV_FORMAT_BUTT;
        if (pstChnParam->enPixelFormat == PIXEL_FORMAT_NV12) {
            config.u.dec.dec_buf.format = DRV_FORMAT_420;
            config.u.dec.dec_buf.chromaInterleave = DRV_CBCR_INTERLEAVE;
        } else if (pstChnParam->enPixelFormat == PIXEL_FORMAT_NV21) {
            config.u.dec.dec_buf.format = DRV_FORMAT_420;
            config.u.dec.dec_buf.chromaInterleave = DRV_CRCB_INTERLEAVE;
        } else if (pstChnParam->enPixelFormat == PIXEL_FORMAT_NV16) {
            config.u.dec.dec_buf.format = DRV_FORMAT_422;
            config.u.dec.dec_buf.chromaInterleave = DRV_CBCR_INTERLEAVE;
        } else if (pstChnParam->enPixelFormat == PIXEL_FORMAT_NV61) {
                config.u.dec.dec_buf.format = DRV_FORMAT_422;
                config.u.dec.dec_buf.chromaInterleave = DRV_CRCB_INTERLEAVE;
        } else {
            config.u.dec.dec_buf.chromaInterleave = DRV_CBCR_SEPARATED;
        }

        config.u.dec.dec_buf.packedFormat = DRV_PACKED_FORMAT_NONE;
        config.u.dec.roiEnable = pstChnParam->stVdecPictureParam.s32ROIEnable;
        config.u.dec.roiWidth = pstChnParam->stVdecPictureParam.s32ROIWidth;
        config.u.dec.roiHeight = pstChnParam->stVdecPictureParam.s32ROIHeight;
        config.u.dec.roiOffsetX = pstChnParam->stVdecPictureParam.s32ROIOffsetX;
        config.u.dec.roiOffsetY = pstChnParam->stVdecPictureParam.s32ROIOffsetY;
        config.u.dec.max_frame_width = pstChnParam->stVdecPictureParam.s32MaxFrameWidth;
        config.u.dec.max_frame_height = pstChnParam->stVdecPictureParam.s32MaxFrameHeight;
        config.u.dec.min_frame_width = pstChnParam->stVdecPictureParam.s32MinFrameWidth;
        config.u.dec.min_frame_height = pstChnParam->stVdecPictureParam.s32MinFrameHeight;
        config.u.dec.usePartialMode = 0;
        config.u.dec.rotAngle = pstChnParam->stVdecPictureParam.s32RotAngle;
        config.u.dec.mirDir = pstChnParam->stVdecPictureParam.s32MirDir;
        config.u.dec.iHorScaleMode = pstChnParam->stVdecPictureParam.u32HDownSampling;
        config.u.dec.iVerScaleMode = pstChnParam->stVdecPictureParam.u32VDownSampling;
        config.u.dec.iDataLen = pChnHandle->ChnAttr.u32StreamBufSize;
        config.s32ChnNum = pChnHandle->VdChn;
        /* Open JPU Devices */
        s32Ret = jpeg_dec_set_param(pChnHandle->pHandle, config);
        if (s32Ret != 0) {
            DRV_VDEC_ERR("\nFailed to jpeg_dec_open, ret:%d\n", s32Ret);
            return s32Ret;
        }
    }

    return s32Ret;
}

int drv_vdec_get_chn_param(vdec_chn VdChn, vdec_chn_param_s *pstParam)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(pstParam, &pChnHandle->ChnParam, sizeof(vdec_chn_param_s));

    return s32Ret;
}

int drv_vdec_send_stream(vdec_chn VdChn, const vdec_stream_s *pstStream,
                int s32MilliSec)
{
    int s32Ret = 0;
    unsigned char bFlushDecodeQ = 0;
    int s32TimeOutMs = s32MilliSec;
    uint64_t u64DecHwTime = 0;
    struct drv_vdec_vb_ctx *pVbCtx = NULL;
    vdec_chn_context *pChnHandle = NULL;
    uint64_t startTime, endTime;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle,chn:%d ret:%d\n", VdChn, s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    pVbCtx = pChnHandle->pVbCtx;

    if (pChnHandle->bStartRecv == 0) {
        return DRV_ERR_VDEC_NOT_PERM;
    }

    if ((pstStream->u32Len == 0) &&
        (pChnHandle->ChnAttr.enType == PT_JPEG ||
         pChnHandle->ChnAttr.enType == PT_MJPEG))
        return 0;

    if ((pstStream->u32Len == 0) && (pstStream->bEndOfStream == 0))
        return 0;

    if ((pstStream->bEndOfStream == 1) &&
        (pChnHandle->ChnAttr.enType != PT_JPEG &&
         pChnHandle->ChnAttr.enType != PT_MJPEG)) {
        bFlushDecodeQ = 1;
    }

    pChnHandle->u32SendStreamCnt++;
    s32Ret = set_vdec_fps_toproc(VdChn, 1);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("(chn %d) set_vdec_fps_toproc fail\n", VdChn);
        return s32Ret;
    }

    get_debug_config_from_proc();
    startTime = get_current_time();

    pChnHandle->stStatus.u32LeftStreamBytes += pstStream->u32Len;
    if (pChnHandle->ChnAttr.enType == PT_JPEG ||
        pChnHandle->ChnAttr.enType == PT_MJPEG) {
        s32Ret = jpeg_decode(pChnHandle, pstStream, s32TimeOutMs, &u64DecHwTime);
        if (s32Ret != 0) {
            if (s32Ret == DRV_ERR_VDEC_BUSY) {
                pChnHandle->stStatus.u32LeftStreamBytes -= pstStream->u32Len;
                DRV_VDEC_TRACE(
                    "jpeg timeout in nonblock mode[%d]\n",
                    s32TimeOutMs);
                return s32Ret;
            }
            DRV_VDEC_ERR("jpeg_decode error\n");
            return s32Ret;
        }

        wake_up(&tVdecWaitQueue[VdChn]);
    } else if ((pChnHandle->ChnAttr.enType == PT_H264) ||
           (pChnHandle->ChnAttr.enType == PT_H265)) {
        s32Ret = h26x_decode(
                pChnHandle->pHandle,
                pChnHandle->ChnParam.enPixelFormat,
                pstStream, s32TimeOutMs);

        if (s32Ret == RETCODE_QUEUEING_FAILURE) {
            pChnHandle->stStatus.u32LeftStreamBytes -= pstStream->u32Len;
            return DRV_ERR_VDEC_BUF_FULL;
        }
        else if(s32Ret == RETCODE_INVALID_PARAM) {
            pChnHandle->stStatus.u32LeftStreamBytes -= pstStream->u32Len;
            return DRV_ERR_VDEC_ILLEGAL_PARAM;
        }

    } else {
        DRV_VDEC_ERR("enType = %d\n", pChnHandle->ChnAttr.enType);
        return DRV_ERR_VDEC_NOT_SUPPORT;
    }

    pChnHandle->stStatus.u32LeftStreamBytes -= pstStream->u32Len;

    endTime = get_current_time();

    pChnHandle->totalDecTime += (endTime - startTime);
    DRV_VDEC_PERF(
        "SendStream timestamp = %llu , dec time = %llu ms, total = %llu ms\n",
        (unsigned long long)(startTime),
        (unsigned long long)(endTime - startTime),
        (unsigned long long)(pChnHandle->totalDecTime));

    if ((pstStream->bEndOfStream == 1) && (s32MilliSec > 0)) {
        DRV_VDEC_TRACE(
            "force flush in EndOfStream in nonblock mode flush dec\n");
        s32TimeOutMs = -1;
    }

    return s32Ret;
}

int drv_vdec_query_status(vdec_chn VdChn, vdec_chn_status_s *pstStatus)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(pstStatus, &pChnHandle->stStatus, sizeof(vdec_chn_status_s));
    if(pChnHandle->ChnAttr.enType == PT_H264
        ||pChnHandle->ChnAttr.enType == PT_H265)
    get_status(pChnHandle->pHandle, pstStatus);
    return s32Ret;
}

int drv_vdec_reset_chn(vdec_chn VdChn)
{
    int s32Ret = 0;
    vdec_chn_attr_s *pstChnAttr;
    unsigned int u32FrameBufCnt;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    pstChnAttr = &pChnHandle->ChnAttr;

    if (pstChnAttr->enType == PT_H264 || pstChnAttr->enType == PT_H265) {
        s32Ret = vdec_reset(pChnHandle->pHandle);
        if (s32Ret < 0) {
            DRV_VDEC_ERR("vdec_reset, %d\n", s32Ret);
            return DRV_ERR_VDEC_ERR_INVALID_RET;
        }

        if (pChnHandle->VideoFrameArray != NULL) {
            u32FrameBufCnt = pChnHandle->ChnAttr.u32FrameBufCnt;
            memset(pChnHandle->VideoFrameArray, 0,
                   sizeof(video_frame_info_s) * u32FrameBufCnt);
        }
    }

    return s32Ret;
}

int drv_vdec_start_recv_stream(vdec_chn VdChn)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    pChnHandle->bStartRecv = 1;
    return s32Ret;
}

int drv_vdec_stop_recv_stream(vdec_chn VdChn)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    pChnHandle->bStartRecv = 0;
    return s32Ret;
}

int drv_vdec_get_frame(vdec_chn VdChn, video_frame_info_s *pstFrameInfo,
              int s32MilliSec)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;
    DRVFRAMEBUF FrameBuf = {0};
    unsigned long int hw_time;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    s32Ret = set_vdec_fps_toproc(VdChn, 0);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("(chn %d) set_vdec_fps_toproc fail\n", VdChn);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    if (pChnHandle->ChnAttr.enType == PT_JPEG
        || pChnHandle->ChnAttr.enType == PT_MJPEG) {
        pChnHandle->u32GetFrameCnt++;
        s32Ret = jpeg_dec_get_frame(pChnHandle->pHandle, (unsigned char *)&FrameBuf, &hw_time);
        if (s32Ret == 0) {
            jpeg_set_frame_info(&pstFrameInfo->video_frame, &FrameBuf);
            s32Ret = set_video_chnattr_toproc(VdChn, pstFrameInfo, hw_time);
            if (s32Ret != 0) {
                DRV_VDEC_ERR("set_video_chnattr_toproc fail");
                return s32Ret;
            }
        }
    }else {
        DispFrameCfg dfc = {0};
        s32Ret = vdec_get_frame(pChnHandle->pHandle, &dfc);
        if (s32Ret >= 0) {
            pChnHandle->u32GetFrameCnt++;
            set_video_frame_info(pstFrameInfo, &dfc);
            s32Ret = set_video_chnattr_toproc(VdChn, pstFrameInfo, dfc.decHwTime);
            if (s32Ret != 0) {
                DRV_VDEC_ERR("set_video_chnattr_toproc fail");
                return s32Ret;
            }
        }

        pChnHandle->stFPS.hw_time = dfc.decHwTime;
    }
    return s32Ret;
}

int drv_vdec_release_frame(vdec_chn VdChn,
                  const video_frame_info_s *pstFrameInfo)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle;
    int fb_idx;
    video_frame_s *pstVFrame;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    pstVFrame = (video_frame_s *)&pstFrameInfo->video_frame;

    if (pChnHandle->ChnAttr.enType == PT_JPEG
        ||pChnHandle->ChnAttr.enType == PT_MJPEG) {
        jpeg_dec_release_frame(pChnHandle->pHandle, pstVFrame->phyaddr[0]);
    }

    if ((pChnHandle->ChnAttr.enType == PT_H264) ||
        (pChnHandle->ChnAttr.enType == PT_H265)) {
        vdec_release_frame(pChnHandle->pHandle, pstVFrame->private_data, pstVFrame->phyaddr[0]);
    }

    DRV_VDEC_INFO("release fb_idx %d\n", fb_idx);
    return 0;
}

int drv_vdec_set_chn_attr(vdec_chn VdChn, const vdec_chn_attr_s *pstAttr)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(&pChnHandle->ChnAttr, pstAttr, sizeof(vdec_chn_attr_s));

    return s32Ret;
}

int drv_vdec_get_chn_attr(vdec_chn VdChn, vdec_chn_attr_s *pstAttr)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(pstAttr, &pChnHandle->ChnAttr, sizeof(vdec_chn_attr_s));

    return s32Ret;
}

int drv_vdec_attach_vbpool(vdec_chn VdChn, const vdec_chn_pool_s *pstPool)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstPool == NULL) {
        return DRV_ERR_VDEC_NULL_PTR;
    }

    if (vdec_handle->g_stModParam.enVdecVBSource != VB_SOURCE_USER) {
        DRV_VDEC_ERR("Not support enVdecVBSource:%d\n",
                 vdec_handle->g_stModParam.enVdecVBSource);
        return DRV_ERR_VDEC_NOT_SUPPORT;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    pChnHandle->vbPool = *pstPool;
    pChnHandle->bHasVbPool = true;
    if (pChnHandle->ChnAttr.enType == PT_H264 || pChnHandle->ChnAttr.enType == PT_H265) {
        VB_INFO vb_info;
        vb_info.frame_buffer_vb_pool = pstPool->hPicVbPool;
        vb_info.vb_mode = vdec_handle->g_stModParam.enVdecVBSource;
        vdec_attach_vb(pChnHandle->pHandle, vb_info);
    }

    return s32Ret;
}

int drv_vdec_detach_vbpool(vdec_chn VdChn)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle = NULL;

    DRV_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (vdec_handle->g_stModParam.enVdecVBSource != VB_SOURCE_USER) {
        DRV_VDEC_ERR("Invalid detachVb in ChnId[%d] VBSource:[%d]\n",
                 VdChn, vdec_handle->g_stModParam.enVdecVBSource);
        return DRV_ERR_VDEC_NOT_SUPPORT;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    if (pChnHandle->bStartRecv != 0) {
        DRV_VDEC_ERR("Cannot detach vdec vb before StopRecvStream\n");
        return DRV_ERR_VDEC_ERR_SEQ_OPER;
    }

    if (pChnHandle->bHasVbPool == false) {
        DRV_VDEC_ERR("ChnId[%d] Null VB\n", VdChn);
        return 0;

    } else {
        pChnHandle->bHasVbPool = false;
    }

    return 0;
}

int drv_vdec_set_mod_param(const vdec_mod_param_s *pstModParam)
{
    int s32Ret = 0;

    if (pstModParam == NULL) {
        return DRV_ERR_VDEC_ILLEGAL_PARAM;
    }

    s32Ret = vdec_mutex_lock(&g_vdec_handle_mutex,
                    VDEC_DEFAULT_MUTEX_MODE, NULL);
    s32Ret = check_timeout_and_busy(s32Ret, __LINE__);
    if (s32Ret != 0) {
        return s32Ret;
    }
    memcpy(&vdec_handle->g_stModParam, pstModParam,
           sizeof(vdec_mod_param_s));
    vdec_mutex_unlock(&g_vdec_handle_mutex);

    return s32Ret;
}

int drv_vdec_get_mod_param(vdec_mod_param_s *pstModParam)
{
    int s32Ret = 0;

    if (pstModParam == NULL) {
        return DRV_ERR_VDEC_ILLEGAL_PARAM;
    }

    s32Ret = vdec_mutex_lock(&g_vdec_handle_mutex,
                    VDEC_DEFAULT_MUTEX_MODE, NULL);
    if (s32Ret != 0) {
        if ((s32Ret == ETIMEDOUT) || (s32Ret == EBUSY)) {
            DRV_VDEC_TRACE("mutex timeout and retry\n");
            return  DRV_ERR_VDEC_BUSY;
        }
        DRV_VDEC_ERR("vdec mutex error[%d]\n", s32Ret);
        return DRV_ERR_VDEC_ERR_VDEC_MUTEX;
    }
    memcpy(pstModParam, &vdec_handle->g_stModParam,
           sizeof(vdec_mod_param_s));
    vdec_mutex_unlock(&g_vdec_handle_mutex);

    return s32Ret;
}

int drv_vdec_frame_buffer_add_user(vdec_chn VdChn, video_frame_info_s *pstFrameInfo)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return frame_bufer_add_user(pChnHandle->pHandle, (uintptr_t)pstFrameInfo->video_frame.private_data);
}


int drv_vdec_set_stride_align(vdec_chn VdChn, unsigned int align)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return set_stride_align(pChnHandle->pHandle, align);
}

int drv_vdec_set_user_pic(vdec_chn VdChn, const video_frame_info_s *usr_pic)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle;
    DispFrameCfg frame_cfg = {0};

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    frame_cfg.width = usr_pic->video_frame.width;
    frame_cfg.height = usr_pic->video_frame.height;
    frame_cfg.strideY = usr_pic->video_frame.stride[0];
    frame_cfg.strideC = usr_pic->video_frame.stride[1];
    frame_cfg.indexFrameDisplay = -1;

    if (usr_pic->video_frame.pixel_format == PIXEL_FORMAT_NV12) {
        frame_cfg.cbcrInterleave = 1;
        frame_cfg.nv21 = 0;
    } else if (usr_pic->video_frame.pixel_format == PIXEL_FORMAT_NV21) {
        frame_cfg.cbcrInterleave = 1;
        frame_cfg.nv21 = 1;
    } else {//PIXEL_FORMAT_YUV_PLANAR_420
        frame_cfg.cbcrInterleave = 0;
        frame_cfg.nv21 = 0;
    }

    frame_cfg.phyAddr[0] = usr_pic->video_frame.phyaddr[0];
    frame_cfg.phyAddr[1] = usr_pic->video_frame.phyaddr[1];
    frame_cfg.phyAddr[2] = usr_pic->video_frame.phyaddr[2];
    frame_cfg.addr[0] = usr_pic->video_frame.viraddr[0];
    frame_cfg.addr[1] = usr_pic->video_frame.viraddr[1];
    frame_cfg.addr[2] = usr_pic->video_frame.viraddr[2];
    frame_cfg.length[0] = usr_pic->video_frame.length[0];
    frame_cfg.length[1] = usr_pic->video_frame.length[1];
    frame_cfg.length[2] = usr_pic->video_frame.length[2];
    frame_cfg.bCompressFrame = 0;

    frame_cfg.endian = usr_pic->video_frame.endian;
    frame_cfg.picType = usr_pic->video_frame.pic_type;
    frame_cfg.seqenceNo = usr_pic->video_frame.seqenceno;
    frame_cfg.interlacedFrame = usr_pic->video_frame.interl_aced_frame;
    frame_cfg.decHwTime = 0;
    return set_user_pic(pChnHandle->pHandle, &frame_cfg);

}

int drv_vdec_enable_user_pic(vdec_chn VdChn, unsigned char instant)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return enable_user_pic(pChnHandle->pHandle, instant);
}

int drv_vdec_disable_user_pic(vdec_chn VdChn)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return disable_user_pic(pChnHandle->pHandle);

}

int drv_vdec_set_display_mode(vdec_chn VdChn, video_display_mode_e display_mode)
{
    int s32Ret = 0;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != 0) {
        DRV_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return set_display_mode(pChnHandle->pHandle, display_mode);
}

#endif

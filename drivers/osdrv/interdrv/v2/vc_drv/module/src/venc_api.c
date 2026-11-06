#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <uapi/linux/sched/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "vpuapi.h"
#include "debug.h"
#include "chagall.h"
#include "h265_interface.h"
#include "linux/mutex.h"
#include "module_common.h"
#include "base_ctx.h"
#include "vb.h"
#include "venc_help.h"
#include "venc_rc.h"
#include "datastructure.h"

extern vb_blk vb_phys_addr2handle(uint64_t u64PhyAddr);
extern wait_queue_head_t tVencWaitQueue[];
static DEFINE_MUTEX(__venc_init_mutex);
extern int32_t base_ion_cache_flush(uint64_t addr_p, void *addr_v, uint32_t u32Len);

#define MAX_SRC_BUFFER_NUM 32
#define MAX_RETRY_TIMES 5

#define VENC_HEADER_BUF_SIZE (1024*1024)
#define VENC_DEFAULT_BISTREAM_SIZE (4*1024*1024)
#define VENC_BUFF_FULL_EXTERN_SIZE (1024*1024)

// frmame delay = preset Gop size - 1
static int presetGopDelay[] = {
    0,  /* Custom GOP, Not used */
    0,  /* All Intra */
    0,  /* IPP Cyclic GOP size 1 */
    0,  /* IBB Cyclic GOP size 1 */
    1,  /* IBP Cyclic GOP size 2 */
    3,  /* IBBBP Cyclic GOP size 4 */
    3,  /* IPPPP Cyclic GOP size 4 */
    3,  /* IBBBB Cyclic GOP size 4 */
    7,  /* IBBBB Cyclic GOP size 8 */
    0,  /* IPP_SINGLE Cyclic GOP size 1 */
};

typedef enum _VencAsyncMode {
    VENC_ASYNC_FALSE    = 0,
    VENC_ASYNC_TRUE     = 1
} VencAsyncMode;

typedef enum _cviVencVpuRcmode {
    VENC_RC_CNM = 0,
    VENC_RC_HOST_CBR = 1,
    VENC_RC_HOST_VBR = 2,
    VENC_RC_HOST_AVBR = 3,
} cviVencVpuRcmode;

typedef struct _drv_enc_extra_buf_info
{
    vpu_buffer_t extra_vb_buffer;
    int bs_size;
    struct list_head list;
} drv_venc_extra_buf_info;

typedef struct _drv_enc_param
{
    // for idr frame
    BOOL idr_request;
    // for enable dir
    BOOL enable_idr;

    // int frameRateInfo;
    // int bitrate;

    // custom map
    BOOL isCustomMapFlag;
    BOOL edge_api_enable;
    WaveCustomMapOpt customMapOpt;

    // user data
    Uint32 userDataBufSize;
    struct list_head userdataList;

    // rotation and mirr
    Uint32 rotationAngle;
    Uint32 mirrorDirection;

    // rcmode
    Uint32 rcMode;
    Uint32 iqp;
    Uint32 pqp;

    // vui rbsp
    vpu_buffer_t vbVuiRbsp;
} drv_enc_param;

typedef struct _DRV_FRAME_ATTR
{
    PhysicalAddress buffer_addr;
    uint64_t pts;
    uint64_t dts;
    int idx;
    PhysicalAddress custom_map_addr;
} DRV_FRAME_ATTR;

typedef struct encoder_handle
{
    EncHandle handle;
    EncOpenParam open_param;
    FrameBuffer *pst_frame_buffer;
    int stride;
    int core_idx;
    int channel_index;
    int min_recon_frame_count;
    int min_src_frame_count;
    int frame_idx;
    int header_encoded;
    PhysicalAddress bitstream_buffer[MAX_SRC_BUFFER_NUM];
    void *thread_handle;
    int stop_thread;
    Queue *free_stream_buffer;
    Queue *stream_packs;
    stPack header_cache_pack;
    int header_cache_ref;
    stChnVencInfo chn_venc_info;
    DRV_FRAME_ATTR input_frame[MAX_SRC_BUFFER_NUM];
    drv_enc_param enc_param;
    int src_end;
    int ouput_end;
    int cmd_queue_full;
    int bframe_delay;
    int first_frame_pts;
    int cmd_queue_depth;
    int is_bind_mode;   // for edge is always 0
    int is_isolate_send;
    struct completion semGetStreamCmd;
    struct completion semEncDoneCmd;
    int virtualIPeriod;

    // roi process
    RoiParam roi_rect[MAX_NUM_ROI];
    int last_frame_qp;
    Queue* customMapBuffer;
    int customMapBufferSize;
    stRcInfo rc_info;
    int enable_ext_rc;

    // buffull process
    drv_venc_extra_buf_info bs_full_info;
    int extra_buf_cnt;
    int extra_free_buf_cnt;
    struct list_head extra_buf_queue;
    struct list_head extra_free_buf_queue;

    // extern buf
    unsigned int bs_buf_size;
    BOOL use_extern_bs_buf;
    PhysicalAddress extern_bs_addr;
} ENCODER_HANDLE;

static int get_vpu_rcmode(int cvi_rcmode)
{
    int host_rcmode = VENC_RC_CNM;
    switch (cvi_rcmode)
    {
        case RC_MODE_CBR:
            host_rcmode = VENC_RC_CNM;
            break;
        case RC_MODE_UBR:
            host_rcmode = VENC_RC_HOST_CBR;
            break;
        case RC_MODE_VBR:
            host_rcmode = VENC_RC_HOST_VBR;
            break;
        case RC_MODE_AVBR:
            host_rcmode = VENC_RC_HOST_AVBR;
            break;
        default:
            VLOG(WARN, "invalid cvi rcmode %d\n", cvi_rcmode);
            break;
    }

    return host_rcmode;
}

static int get_avc_profile(int profile)
{
    int vpu_avc_profile = 0;
    if (profile == 0) {
        vpu_avc_profile = H264_PROFILE_BP;    // baseline profile
    } else if (profile == 1) {
        vpu_avc_profile = H264_PROFILE_MP;    // main profile
    } else if (profile == 2) {
        vpu_avc_profile = H264_PROFILE_HP;    // high profile
    }

    return vpu_avc_profile;
}

Int32 set_open_param(EncOpenParam *pst_open_param, InitEncConfig *pst_init_cfg)
{
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    int frameRateDiv = 0, frameRateRes = 0;
    int i = 0;

    if (pst_init_cfg->codec == CODEC_H264)
        pst_open_param->bitstreamFormat = STD_AVC;
    else
        pst_open_param->bitstreamFormat = STD_HEVC;

    pst_open_param->picWidth = pst_init_cfg->width;
    pst_open_param->picHeight = pst_init_cfg->height;
    pst_open_param->frameRateInfo = pst_init_cfg->framerate;
    pst_open_param->MESearchRange = 3;
    pst_open_param->rcInitDelay = pst_init_cfg->initialDelay;
    pst_open_param->vbvBufferSize = 0;
    pst_open_param->meBlkMode = 0;
    pst_open_param->frameSkipDisable = 1;
    pst_open_param->sliceMode.sliceMode = 0;
    pst_open_param->sliceMode.sliceSizeMode = 1;
    pst_open_param->sliceMode.sliceSize  = 115;
    pst_open_param->intraRefreshNum = 0;
    pst_open_param->rcIntraQp = 30;
    pst_open_param->userQpMax = -1;
    pst_open_param->userGamma = (Uint32)(0.75 * 32768);
    pst_open_param->rcIntervalMode = 1;
    pst_open_param->mbInterval = 0;
    pst_open_param->MEUseZeroPmv = 0;
    pst_open_param->intraCostWeight = 400;
    pst_open_param->bitRate = pst_init_cfg->bitrate * 1024;
    pst_open_param->srcBitDepth = 8;
    pst_open_param->subFrameSyncMode = 1;
    pst_open_param->frameEndian = VPU_FRAME_ENDIAN;
    pst_open_param->streamEndian = VPU_STREAM_ENDIAN;
    pst_open_param->sourceEndian = VPU_SOURCE_ENDIAN;
    pst_open_param->lineBufIntEn = 1;
    pst_open_param->bitstreamBufferSize = pst_init_cfg->bitstreamBufferSize;
    if (pst_open_param->bitstreamBufferSize == 0)
        pst_open_param->bitstreamBufferSize = VENC_DEFAULT_BISTREAM_SIZE; // default: 4MB

    pst_open_param->enablePTS = FALSE;
    pst_open_param->cmdQueueDepth = pst_init_cfg->s32CmdQueueDepth;


    /* for cvi rate control param */
    pst_open_param->hostRcmode = get_vpu_rcmode(pst_init_cfg->rcMode);      /* 0: chipmedia rc, 1:cbr, 2:vbr, 3:avbr */
    pst_open_param->enableHierarchy = 0;   /* enable hierarchy feature */
    pst_open_param->rcIpQpDelta = pst_init_cfg->s32IPQpDelta;
    pst_open_param->rcBgQpDelta = pst_init_cfg->s32BgQpDelta;

    if (pst_open_param->bitstreamFormat == STD_HEVC) {
        pst_open_param->rcLastClip = 3;
        pst_open_param->rcLevelClip = 3;
        pst_open_param->rcPicNormalClip = 10;
    } else {
        pst_open_param->rcLastClip = 10;
        pst_open_param->rcLevelClip = 10;
        pst_open_param->rcPicNormalClip = 10;
    }

    pst_open_param->statTime = pst_init_cfg->statTime;
    pst_open_param->minIprop = 1;
    pst_open_param->maxIprop = 100;

    pst_open_param->changePos = pst_init_cfg->s32ChangePos;

    pst_open_param->minStillPercent = pst_init_cfg->s32MinStillPercent;
    pst_open_param->maxStillQp = pst_init_cfg->u32MaxStillQP;
    pst_open_param->motionSensitivity = pst_init_cfg->u32MotionSensitivity;
    pst_open_param->avbrPureStillThr = pst_init_cfg->s32AvbrPureStillThr;

    pst_open_param->frmLostOpen = 0;
    pst_open_param->frmLostMode = 1; // only support P_SKIP
    pst_open_param->encFrmGaps = 0;
    pst_open_param->frmLostBpsThr = 0;
    pst_open_param->avbrFrmLostOpen = pst_init_cfg->s32AvbrFrmLostOpen;
    pst_open_param->avbrFrmGaps = pst_init_cfg->s32AvbrFrmGap;

    pst_open_param->picMotionLevel = 0;

    /* for wave521 */
    /* hevc: let firmware determines a profile according to internalbitdepth */
    /* avc: profile cannot be set by host application*/
    if (pst_open_param->bitstreamFormat == STD_AVC) {
        param->profile = get_avc_profile(pst_init_cfg->u32Profile);
    } else {
        param->profile = pst_init_cfg->u32Profile;
    }

    param->level = 0;
    param->tier = 0;
    param->internalBitDepth = 8;
    param->losslessEnable = 0;
    param->constIntraPredFlag = 0;
    param->useLongTerm = 0;

    /* for CMD_ENC_SEQ_GOP_PARAM */
    /* todo: PRESET_IDX_CUSTOM_GOP */
    param->gopPresetIdx = pst_init_cfg->u32GopPreset;
    if (pst_init_cfg->u32GopPreset == PRESET_IDX_CUSTOM_GOP) {
        memcpy(&param->gopParam, &pst_init_cfg->gopParam, sizeof(CustomGopParam));
    }

    /* for CMD_ENC_SEQ_INTRA_PARAM */
    param->decodingRefreshType = pst_init_cfg->decodingRefreshType;
    param->intraPeriod = pst_init_cfg->gop;
    param->avcIdrPeriod = pst_init_cfg->gop;
    param->intraQP = 0;

    /* for CMD_ENC_SEQ_CONF_WIN_TOP_BOT/LEFT_RIGHT */
    // param->confWin.top = pst_init_cfg->confWin.top;
    // param->confWin.bottom = pst_init_cfg->confWin.bottom;
    // param->confWin.left = pst_init_cfg->confWin.left;
    // param->confWin.right = pst_init_cfg->confWin.right;

    /* for CMD_ENC_SEQ_INDEPENDENT_SLICE */
    param->independSliceMode = 0;
    param->independSliceModeArg = 0;

    /* for CMD_ENC_SEQ_DEPENDENT_SLICE */
    param->dependSliceMode = 0;
    param->dependSliceModeArg = 0;

    /* for CMD_ENC_SEQ_INTRA_REFRESH_PARAM */
    param->intraRefreshMode = 0;
    param->intraRefreshArg = 0;
    param->useRecommendEncParam = pst_init_cfg->s32EncMode;

    /* for CMD_ENC_PARAM */
    if (param->useRecommendEncParam != 1)  {
        param->scalingListEnable = 0;
        param->tmvpEnable = 1;
        param->wppEnable = 0;
        param->maxNumMerge = 2;
        param->disableDeblk = 0;
        param->lfCrossSliceBoundaryEnable = 1;
        param->betaOffsetDiv2 = 0;
        param->tcOffsetDiv2 = 0;
        param->skipIntraTrans = 1;
        param->saoEnable = 1;
        param->intraNxNEnable = 1;
    }

    /* for CMD_ENC_RC_PARAM */
    pst_open_param->rcEnable = (pst_init_cfg->rcMode == RC_MODE_FIXQP) ? FALSE : TRUE;;
    pst_open_param->vbvBufferSize = 3000;
    param->roiEnable = 1;
    param->bitAllocMode = 0;
    for (i = 0; i < MAX_GOP_NUM; i++)
        param->fixedBitRatio[i] = 1;
    param->cuLevelRCEnable = 1;
    param->hvsQPEnable = 1;
    param->hvsQpScale = 2;

    /* for CMD_ENC_RC_MIN_MAX_QP */
    param->minQpI = 8;
    param->maxQpI = 51;
    param->minQpP = 8;
    param->maxQpP = 51;
    param->minQpB = 8;
    param->maxQpB = 51;
    param->hvsMaxDeltaQp = 10;

    /* for CMD_ENC_CUSTOM_GOP_PARAM */
    param->gopParam.customGopSize    = 0;
    for (i = 0; i < param->gopParam.customGopSize; i++) {
        param->gopParam.picParam[i].picType = PIC_TYPE_I;
        param->gopParam.picParam[i].pocOffset = 1;
        param->gopParam.picParam[i].picQp = 30;
        param->gopParam.picParam[i].refPocL0 = 0;
        param->gopParam.picParam[i].refPocL1 = 0;
        param->gopParam.picParam[i].temporalId  = 0;
    }

    // for VUI / time information.
    param->numTicksPocDiffOne = 0;

    frameRateDiv = (pst_init_cfg->framerate >> 16);
    frameRateRes = pst_init_cfg->framerate & 0xFFFF;

    if (frameRateDiv == 0) {
        param->numUnitsInTick = 1000;
        frameRateRes *= 1000;
    } else {
        param->numUnitsInTick = frameRateDiv;
    }

    if (pst_open_param->bitstreamFormat == STD_AVC) {
        param->timeScale = frameRateRes * 2;
    } else {
        param->timeScale = frameRateRes;
    }

    param->chromaCbQpOffset = 0;
    param->chromaCrQpOffset = 0;
    param->initialRcQp = 30;
    param->nrYEnable = 0;
    param->nrCbEnable = 0;
    param->nrCrEnable = 0;
    param->nrNoiseEstEnable = 0;
    param->useLongTerm = (pst_init_cfg->virtualIPeriod > 0) ? 1 : 0;

    param->monochromeEnable = 0;
    param->strongIntraSmoothEnable = 1;
    param->weightPredEnable = 0;
    param->bgDetectEnable = 0;
    param->bgThrDiff = 8;
    param->bgThrMeanDiff = 1;
    param->bgLambdaQp = 32;
    param->bgDeltaQp = 3;

    param->customLambdaEnable = 0;
    param->customMDEnable = 0;
    param->pu04DeltaRate = 0;
    param->pu08DeltaRate = 0;
    param->pu16DeltaRate = 0;
    param->pu32DeltaRate = 0;
    param->pu04IntraPlanarDeltaRate = 0;
    param->pu04IntraDcDeltaRate = 0;
    param->pu04IntraAngleDeltaRate = 0;
    param->pu08IntraPlanarDeltaRate = 0;
    param->pu08IntraDcDeltaRate = 0;
    param->pu08IntraAngleDeltaRate = 0;
    param->pu16IntraPlanarDeltaRate = 0;
    param->pu16IntraDcDeltaRate = 0;
    param->pu16IntraAngleDeltaRate = 0;
    param->pu32IntraPlanarDeltaRate = 0;
    param->pu32IntraDcDeltaRate = 0;
    param->pu32IntraAngleDeltaRate = 0;
    param->cu08IntraDeltaRate = 0;
    param->cu08InterDeltaRate = 0;
    param->cu08MergeDeltaRate = 0;
    param->cu16IntraDeltaRate = 0;
    param->cu16InterDeltaRate = 0;
    param->cu16MergeDeltaRate = 0;
    param->cu32IntraDeltaRate = 0;
    param->cu32InterDeltaRate = 0;
    param->cu32MergeDeltaRate = 0;
    param->coefClearDisable = 0;

    param->rcWeightParam               = 2;
    param->rcWeightBuf                 = 128;
    param->s2SearchRangeXDiv4          = 32;
    param->s2SearchRangeYDiv4          = 16;

    // for H.264 encoder
    param->avcIdrPeriod =
        ((param->gopPresetIdx == 1) && (pst_open_param->bitstreamFormat == STD_AVC)) ? 1 : param->avcIdrPeriod;
    param->rdoSkip = 1;
    param->lambdaScalingEnable = 1;
    if (pst_open_param->bitstreamFormat == STD_AVC && param->profile == H264_PROFILE_HP)
        param->transform8x8Enable = 1;

    param->avcSliceMode = 0;
    param->avcSliceArg = 0;
    param->intraMbRefreshMode = 0;
    param->intraMbRefreshArg = 1;
    param->mbLevelRcEnable = 1;
    param->entropyCodingMode = 1;
    param->disableDeblk = 0;

    return 0;
}

int set_vb_flag(PhysicalAddress addr)
{
    vb_blk blk;
    struct vb_s *vb;

    blk = vb_phys_addr2handle(addr);
    if (blk == VB_INVALID_HANDLE)
        return 0;

    vb = (struct vb_s *)&blk;
    atomic_fetch_add(1, &vb->usr_cnt);
    atomic_long_fetch_or(BIT(ID_VENC), &vb->mod_ids);

    return 0;
}

int clr_vb_flag(PhysicalAddress addr)
{
    vb_blk blk;
    struct vb_s *vb;

    blk = vb_phys_addr2handle(addr);
    if (blk == VB_INVALID_HANDLE)
        return 0;

    vb = (struct vb_s *)&blk;
    atomic_long_fetch_and(~BIT(ID_VENC), &vb->mod_ids);
    vb_release_block(blk);

    return 0;
}

static int get_frame_idx(void * handle, PhysicalAddress addr)
{
    int idx;
    ENCODER_HANDLE *pst_handle = handle;

    for(idx=0; idx<MAX_SRC_BUFFER_NUM; idx++) {
        if (pst_handle->input_frame[idx].buffer_addr == 0) {
            pst_handle->input_frame[idx].buffer_addr = addr;
            break;
        }

        if (pst_handle->input_frame[idx].buffer_addr == addr)
            break;
    }

    if (idx == MAX_SRC_BUFFER_NUM)
        return -1;

    return idx;
}

static void release_frame_idx(void * handle, int srcIdx)
{
    ENCODER_HANDLE *pst_handle = handle;

    // sanity check
    if (!pst_handle || srcIdx >= MAX_SRC_BUFFER_NUM) {
        return;
    }

    pst_handle->input_frame[srcIdx].buffer_addr = 0;
}

static int  alloc_framebuffer(void * handle)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    drv_enc_param *pst_ext_param = &pst_handle->enc_param;
    Uint32 fbWidth = 0, fbHeight = 0;
    int stride;
    int frame_size;
    int i;
    int ret;
    vpu_buffer_t vb_buffer;
    int map_type = COMPRESSED_FRAME_MAP;

    if (pst_open_param->bitstreamFormat == STD_AVC) {
        fbWidth  = VPU_ALIGN16(pst_open_param->picWidth);
        fbHeight = VPU_ALIGN16(pst_open_param->picHeight);

        if ((pst_ext_param->rotationAngle != 0 || pst_ext_param->mirrorDirection != 0)
            && !(pst_ext_param->rotationAngle == 180 && pst_ext_param->mirrorDirection == MIRDIR_HOR_VER)) {
            fbWidth  = VPU_ALIGN16(pst_open_param->picWidth);
            fbHeight = VPU_ALIGN16(pst_open_param->picHeight);
        }
        if (pst_ext_param->rotationAngle == 90 || pst_ext_param->rotationAngle == 270) {
            fbWidth  = VPU_ALIGN16(pst_open_param->picHeight);
            fbHeight = VPU_ALIGN16(pst_open_param->picWidth);
        }
    } else {
        fbWidth  = VPU_ALIGN8(pst_open_param->picWidth);
        fbHeight = VPU_ALIGN8(pst_open_param->picHeight);

        if ((pst_ext_param->rotationAngle != 0 || pst_ext_param->mirrorDirection != 0)
            && !(pst_ext_param->rotationAngle == 180 && pst_ext_param->mirrorDirection == MIRDIR_HOR_VER)) {
            fbWidth  = VPU_ALIGN32(pst_open_param->picWidth);
            fbHeight = VPU_ALIGN32(pst_open_param->picHeight);
        }
        if (pst_ext_param->rotationAngle == 90 || pst_ext_param->rotationAngle == 270) {
            fbWidth  = VPU_ALIGN32(pst_open_param->picHeight);
            fbHeight = VPU_ALIGN32(pst_open_param->picWidth);
        }
    }

    pst_handle->pst_frame_buffer = (FrameBuffer *)vzalloc(pst_handle->min_recon_frame_count * sizeof(FrameBuffer));
    stride = VPU_GetFrameBufStride(pst_handle->handle, fbWidth, fbHeight,
        FORMAT_420, 0, map_type);
    frame_size = VPU_GetFrameBufSize(pst_handle->handle, pst_handle->core_idx, stride,
        fbHeight, map_type, FORMAT_420, 0, NULL);
    vb_buffer.size = frame_size;

    for (i = 0; i < pst_handle->min_recon_frame_count; i++) {
        ret = vdi_allocate_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
        if(ret != RETCODE_SUCCESS) {
            return ret;
        }
        pst_handle->pst_frame_buffer[i].bufY = vb_buffer.phys_addr;
        pst_handle->pst_frame_buffer[i].bufCb = (PhysicalAddress) - 1;
        pst_handle->pst_frame_buffer[i].bufCr = (PhysicalAddress) - 1;
        pst_handle->pst_frame_buffer[i].updateFbInfo = 1;
        pst_handle->pst_frame_buffer[i].size = vb_buffer.size;
    }

    ret = VPU_EncRegisterFrameBuffer(pst_handle->handle, pst_handle->pst_frame_buffer,
        pst_handle->min_recon_frame_count, stride, fbHeight, map_type);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed VPU_EncRegisterFrameBuffer(ret:%d)\n", ret);
        return ret;
    }

    return RETCODE_SUCCESS;
}

static int alloc_bitstream_buf(void *handle)
{
    ENCODER_HANDLE *pst_handle = handle;
    vpu_buffer_t vb_buffer;
    int i = 0;
    int ret = 0;

    if (pst_handle->use_extern_bs_buf) {
        memset(&vb_buffer, 0, sizeof(vpu_buffer_t));
        vb_buffer.size = VPU_ALIGN4096(pst_handle->open_param.bitstreamBufferSize);
        for (i = 0; i < pst_handle->min_src_frame_count; i++) {
            pst_handle->bitstream_buffer[i] =
                pst_handle->extern_bs_addr + i * VPU_ALIGN4096(pst_handle->open_param.bitstreamBufferSize);

            vb_buffer.phys_addr = pst_handle->bitstream_buffer[i];
            vb_buffer.virt_addr = (unsigned long)phys_to_virt(vb_buffer.phys_addr);
            vb_buffer.base = vb_buffer.virt_addr;

            vdi_insert_extern_memory(pst_handle->core_idx, &vb_buffer, ENC_BS, 0);
            Queue_Enqueue(pst_handle->free_stream_buffer, &pst_handle->bitstream_buffer[i]);
            VLOG(INFO, "bitstream buf index:%d, phys:0x%lx\n", i, pst_handle->bitstream_buffer[i]);
        }
    } else {
        memset(&vb_buffer, 0, sizeof(vpu_buffer_t));
        vb_buffer.size = pst_handle->open_param.bitstreamBufferSize;
        for (i = 0; i < pst_handle->min_src_frame_count; i++) {
            ret = vdi_allocate_dma_memory(pst_handle->core_idx, &vb_buffer, ENC_BS, 0);
            if (ret != RETCODE_SUCCESS) {
                VLOG(ERR, "<%s:%d> Failed to alloc bitstream_buffer\n", __FUNCTION__, __LINE__);
                return -1;
            }
            pst_handle->bitstream_buffer[i] = vb_buffer.phys_addr;
            Queue_Enqueue(pst_handle->free_stream_buffer, &vb_buffer.phys_addr);
        }
    }

    return 0;
}

static int venc_check_idr_period(void *handle)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    int isIframe;

    if (pst_handle->frame_idx == 0) {
        return TRUE;
    } else if (param->intraPeriod == 0) {
        return FALSE;
    }

    isIframe = ((pst_handle->frame_idx % param->intraPeriod) == 0);

    return isIframe;
}

static void venc_picparam_change_ctrl(void *handle, EncParam *encParam, BOOL* p_header_update)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    drv_enc_param *pst_ext_param = &pst_handle->enc_param;
    EncChangeParam  changeParam;
    int enable_option = 0;
    int ret = 0;
    BOOL rateChangeCmd = FALSE;

    osal_memset(&changeParam, 0x00, sizeof(EncChangeParam));

#ifndef ENABLE_HOST_RC
    // W5_ENC_CHANGE_PARAM_RC_TARGET_RATE
    if(pst_open_param->hostRcmode == RC_MODE_AVBR) {  //no need to refresh at idr frame in avbr mode
        if(venc_rc_avbr_pic_ctrl(&pst_handle->rc_info, pst_open_param, pst_handle->frame_idx)) {
            venc_rc_set_param(&pst_handle->rc_info, pst_open_param, E_BITRATE);
            enable_option |= W5_ENC_CHANGE_PARAM_RC_TARGET_RATE;
            changeParam.bitRate = pst_handle->rc_info.targetBitrate;
            rateChangeCmd = TRUE;
        }
    } else {
#endif
        if(encParam->is_idr_frame && pst_open_param->bitRate != venc_rc_get_param(&pst_handle->rc_info, E_BITRATE)) {
            venc_rc_set_param(&pst_handle->rc_info, pst_open_param, E_BITRATE);
            enable_option |= W5_ENC_CHANGE_PARAM_RC_TARGET_RATE;
            changeParam.bitRate = pst_handle->rc_info.targetBitrate;
            rateChangeCmd = TRUE;
        }
#ifndef ENABLE_HOST_RC
    }
#endif

    // framerate change
    if (encParam->is_idr_frame && pst_open_param->frameRateInfo != venc_rc_get_param(&pst_handle->rc_info, E_FRAMERATE)) {
        venc_rc_set_param(&pst_handle->rc_info, pst_open_param, E_FRAMERATE);
        enable_option |= W5_ENC_CHANGE_PARAM_RC_FRAME_RATE;
        changeParam.frameRate = pst_handle->rc_info.framerate;
        rateChangeCmd = TRUE;
        *p_header_update = TRUE;
    }

    if (rateChangeCmd) {
        changeParam.enable_option = enable_option;
        ret = VPU_EncGiveCommand(pst_handle->handle, ENC_SET_PARA_CHANGE, &changeParam);
    }

    if (pst_ext_param->enable_idr == FALSE && pst_handle->frame_idx != 0) {
        encParam->is_idr_frame = FALSE;
    }

    // idr request
    if (pst_ext_param->idr_request == TRUE && encParam->is_idr_frame == FALSE) {
        encParam->is_idr_frame = TRUE;
        pst_ext_param->idr_request = FALSE;

        encParam->forcePicTypeEnable = 1;
        encParam->forcePicType = 3;    // IDR
    }

#ifndef ENABLE_HOST_RC
    if (pst_handle->rc_info.rcEnable && pst_handle->rc_info.rcMode == RC_MODE_AVBR) {
        if (pst_handle->rc_info.avbrChangeBrEn == TRUE) {
            int deltaQp = venc_rc_avbr_get_qpdelta(&pst_handle->rc_info, pst_open_param);
            int maxQp = encParam->is_idr_frame ? param->maxQpI : param->maxQpP;
            int minQp = encParam->is_idr_frame ? param->minQpI : param->minQpP;

            changeParam.enable_option = W5_ENC_CHANGE_PARAM_RC_MIN_MAX_QP;
            changeParam.maxQpI = CLIP3(0, 51, maxQp + deltaQp);
            changeParam.minQpI = CLIP3(0, 51, minQp);
            changeParam.hvsMaxDeltaQp = deltaQp;
            VPU_EncGiveCommand(pst_handle->handle, ENC_SET_PARA_CHANGE, &changeParam);
        }
    }
#endif
}

static int venc_insert_userdata_segment(Queue *psp, Uint8 *pUserData,
                       Uint32 userDataLen)
{
    stPack userdata_pack = {0};
    Uint8 *pst_buffer = NULL;
    uint32_t total_packs = 0;

    total_packs = Queue_Get_Cnt(psp);
    if (total_packs >= MAX_NUM_PACKS) {
        VLOG(ERR, "totalPacks (%d) >= MAX_NUM_PACKS\n", total_packs);
        return FALSE;
    }

    pst_buffer = (Uint8 *)osal_kmalloc(userDataLen);
    if (pst_buffer == NULL) {
        VLOG(ERR, "out of memory\n");
        return FALSE;
    }

    memcpy(pst_buffer, pUserData, userDataLen);
    memset(&userdata_pack, 0, sizeof(stPack));

    userdata_pack.addr = pst_buffer;
    userdata_pack.len = userDataLen;
    userdata_pack.NalType = NAL_SEI;
    userdata_pack.need_free = TRUE;
    userdata_pack.u64PhyAddr = virt_to_phys(pst_buffer);
    vdi_flush_ion_cache(userdata_pack.u64PhyAddr, pst_buffer, userDataLen);

    Queue_Enqueue(psp, &userdata_pack);

    return TRUE;
}

static int venc_insert_userdata(void *handle)
{
    int ret = TRUE;
    ENCODER_HANDLE *pst_handle = handle;
    drv_enc_param *pst_ext_param = &pst_handle->enc_param;
    UserDataList *userdataNode = NULL;
    UserDataList *n;

    list_for_each_entry_safe(userdataNode, n, &pst_ext_param->userdataList, list) {
        if (userdataNode->userDataBuf != NULL && userdataNode->userDataLen != 0) {
            if (!venc_insert_userdata_segment(pst_handle->stream_packs,
                             userdataNode->userDataBuf,
                             userdataNode->userDataLen)) {
                VLOG(ERR, "failed to insert user data\n");
                ret = FALSE;
            }
            osal_free(userdataNode->userDataBuf);
            list_del(&userdataNode->list);
            osal_free(userdataNode);
            return ret;
        }
    }

    return ret;
}

Int32 venc_write_vui_rbsp_data(void *handle,  Uint8 *pVuiRbspBuf, int32_t vui_len)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    drv_enc_param *pst_ext_param = &pst_handle->enc_param;
    int ret = 0;

    if (pst_open_param->encodeVuiRbsp) {
        pst_ext_param->vbVuiRbsp.size = VUI_HRD_RBSP_BUF_SIZE;

        if (vdi_allocate_dma_memory(pst_handle->core_idx, &pst_ext_param->vbVuiRbsp, ENC_ETC, 0) < 0) {
            VLOG(ERR, "fail to allocate VUI rbsp buffer\n" );
            return FALSE;
        }
        pst_open_param->vuiRbspDataAddr = pst_ext_param->vbVuiRbsp.phys_addr;
        ret = vdi_write_memory(pst_handle->core_idx, pst_open_param->vuiRbspDataAddr
            , pVuiRbspBuf, VPU_ALIGN16(vui_len), VDI_128BIT_BIG_ENDIAN);
    }

    return TRUE;
}

static int venc_h264_sps_add_vui(void *handle, H264Vui *pVui)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    char *vui_rbsp_addr = NULL;
    int vui_rbsp_byte_len = 0, vui_rbsp_bit_len = 0;

    venc_help_h264_sps_add_vui(pVui, (void **)&vui_rbsp_addr, &vui_rbsp_byte_len, &vui_rbsp_bit_len);
    if (!vui_rbsp_addr || vui_rbsp_byte_len == 0) {
        return -1;
    }

    pst_open_param->encodeVuiRbsp = 1;
    pst_open_param->vuiRbspDataSize = vui_rbsp_bit_len;
    venc_write_vui_rbsp_data(handle, vui_rbsp_addr, vui_rbsp_byte_len);

    if (vui_rbsp_addr) {
        osal_kfree(vui_rbsp_addr);
    }
    return 0;
}

static int venc_h265_sps_add_vui(void *handle, H265Vui *pVui)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    void *vui_rbsp_addr = NULL;
    int vui_rbsp_byte_len = 0, vui_rbsp_bit_len = 0;

    venc_help_h265_sps_add_vui(pVui, &vui_rbsp_addr, &vui_rbsp_byte_len, &vui_rbsp_bit_len);
    if (!vui_rbsp_addr || vui_rbsp_byte_len == 0) {
        return -1;
    }

    pst_open_param->encodeVuiRbsp = 1;
    pst_open_param->vuiRbspDataSize = vui_rbsp_bit_len;
    venc_write_vui_rbsp_data(handle, vui_rbsp_addr, vui_rbsp_byte_len);

    if (vui_rbsp_addr) {
        osal_kfree(vui_rbsp_addr);
    }
    return 0;
}

static void venc_process_bsbuf_full(void *handle)
{
    ENCODER_HANDLE *pst_handle = handle;
    drv_venc_extra_buf_info *pst_bsfull_info = &pst_handle->bs_full_info;
    ENC_QUERY_WRPTR_SEL enc_wr_ptr_sel = GET_ENC_PIC_DONE_WRPTR;
    PhysicalAddress ptr_read;
    PhysicalAddress ptr_write;
    vpu_buffer_t vb_buffer;
    int cur_encoded_size = 0;
    int ret = 0;
    int extra_bs_size = (pst_handle->open_param.bitstreamBufferSize > VENC_BUFF_FULL_EXTERN_SIZE)
                    ? pst_handle->open_param.bitstreamBufferSize : VENC_BUFF_FULL_EXTERN_SIZE;

    enc_wr_ptr_sel = GET_ENC_BSBUF_FULL_WRPTR;
    VPU_EncGiveCommand(pst_handle->handle, ENC_WRPTR_SEL, &enc_wr_ptr_sel);
    VPU_EncGetBitstreamBuffer(pst_handle->handle, &ptr_read, &ptr_write, &cur_encoded_size);

    VLOG(INFO, "INT_BSBUF_FULL:%d, rd:0x%lx, wr:0x%lx, pre encoded:%d, size:%d\n"
            , ret, ptr_read, ptr_write, pst_bsfull_info->bs_size, cur_encoded_size);

    // alloc extra buf to store and contimue to encode
    // 0. check extra buf if exist
    // 1. check extra buf if enough
    // 2. if enough, do next step 4; if not enough, alloc new buf, then do next step 3
    // 3. copy the partial encoded data to new buf, free old buf
    // 4. read encoded bitstream data to new buf

    // step 0
    if (pst_bsfull_info->bs_size > 0) {
        // step 1
        if (pst_bsfull_info->bs_size + cur_encoded_size < pst_bsfull_info->extra_vb_buffer.size) {
            // step 2: use original alloc buf
            VLOG(INFO, "no need alloc new buf, encoded size:%d, bufsize:%d"
                , pst_bsfull_info->bs_size + cur_encoded_size, pst_bsfull_info->extra_vb_buffer.size);
        } else {
            // step 2: aloc new buf
            vb_buffer.size = pst_bsfull_info->extra_vb_buffer.size + extra_bs_size;
            ret = vdi_allocate_dma_memory(pst_handle->core_idx,
                        &vb_buffer, ENC_BS, 0);
            if (ret < 0) {
                VLOG(ERR, "alloc new buf for bs_buf_full irq fail, size:%d\n"
                    , pst_bsfull_info->extra_vb_buffer.size);
                return;
            }

            // step 3
            osal_memcpy((void *)vb_buffer.virt_addr, (void *)pst_bsfull_info->extra_vb_buffer.virt_addr
                , pst_bsfull_info->extra_vb_buffer.size);
            vdi_flush_ion_cache(vb_buffer.phys_addr, (void *)vb_buffer.virt_addr, vb_buffer.size);

            vdi_free_dma_memory(pst_handle->core_idx, &pst_bsfull_info->extra_vb_buffer, ENC_BS, 0);

            osal_memcpy(&pst_bsfull_info->extra_vb_buffer, &vb_buffer, sizeof(vpu_buffer_t));
        }
    } else {
        pst_bsfull_info->extra_vb_buffer.size
            = pst_handle->open_param.bitstreamBufferSize + extra_bs_size;
        ret = vdi_allocate_dma_memory(pst_handle->core_idx,
                    &pst_bsfull_info->extra_vb_buffer, ENC_BS, 0);
        if (ret < 0) {
            VLOG(ERR, "alloc new buf for bs_buf_full irq fail, size:%d\n"
                , pst_bsfull_info->extra_vb_buffer.size);
            return;
        }
    }

    // step 4
    vdi_read_memory(pst_handle->core_idx, ptr_read
            , (unsigned char *)(pst_bsfull_info->extra_vb_buffer.virt_addr + pst_bsfull_info->bs_size)
            , cur_encoded_size,  VPU_STREAM_ENDIAN);
    pst_bsfull_info->bs_size += cur_encoded_size;
}

static void venc_release_exten_buf(void *handle)
{
    ENCODER_HANDLE *pst_handle = handle;
    drv_venc_extra_buf_info *pst_extra_buf_info = NULL, *temp_extra_buf_info = NULL;
    vpu_buffer_t vb_buffer;

    if (pst_handle->extra_buf_cnt > 0) {
        list_for_each_entry_safe(pst_extra_buf_info, temp_extra_buf_info
                                    , &pst_handle->extra_buf_queue, list) {
            if (pst_extra_buf_info->extra_vb_buffer.phys_addr > 0) {
                vb_buffer.phys_addr = pst_extra_buf_info->extra_vb_buffer.phys_addr;
                vb_buffer.size = pst_extra_buf_info->extra_vb_buffer.size;
                vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
                list_del(&pst_extra_buf_info->list);
                osal_free(pst_extra_buf_info);
                pst_handle->extra_buf_cnt--;
            }
        }
    }

    if (pst_handle->extra_free_buf_cnt > 0) {
        list_for_each_entry_safe(pst_extra_buf_info, temp_extra_buf_info
                                    , &pst_handle->extra_free_buf_queue, list) {
            if (pst_extra_buf_info && pst_extra_buf_info->extra_vb_buffer.phys_addr > 0) {
                vb_buffer.phys_addr = pst_extra_buf_info->extra_vb_buffer.phys_addr;
                vb_buffer.size = pst_extra_buf_info->extra_vb_buffer.size;
                vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, ENC_BS, 0);
                list_del(&pst_extra_buf_info->list);
                osal_free(pst_extra_buf_info);
                pst_handle->extra_free_buf_cnt--;
            }
        }
    }
}

static int venc_process_frame_done(void* handle, int async_mode)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOutputInfo output_info;
    stPack encode_pack = {0};
    vpu_buffer_t vb_buffer;
    drv_venc_extra_buf_info *pst_extra_buf_info = &pst_handle->bs_full_info;
    drv_venc_extra_buf_info *pst_extra_free_buf = NULL, *temp_extra_buf_info = NULL;
    int remain_encoded_size = 0;
    int cyclePerTick = 256;
    int retry_times = 0;
    int ret;
    vpu_buffer_t header_vb_buffer;

    memset(&output_info, 0, sizeof(EncOutputInfo));
    do {
        ret = VPU_EncGetOutputInfo(pst_handle->handle, &output_info);
        if (ret != RETCODE_SUCCESS) {
            VLOG(TRACE, "VPU_EncGetOutputInfo ret:%d, pictype:0x%x, streamSize:%d\n"
                , ret, output_info.picType, output_info.bitstreamSize);
            retry_times++;
            osal_msleep(1);
            continue;
        } else {
            break;
        }
    } while (retry_times <= MAX_RETRY_TIMES);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed VPU_EncGetOutputInfo ret:%d, reason:0x%x\n", ret, output_info.errorReason);
        return -1;
    }

    VLOG(TRACE, "encode type:%d bitstreamSize:%u, releaseFlag:0x%x, recon:%d \n"
        , output_info.picType, output_info.bitstreamSize, output_info.releaseSrcFlag, output_info.reconFrameIndex);

    if (output_info.bitstreamSize > 0) {
        // async_mode = 1 need backup vps/sps/pps
        if (async_mode && output_info.reconFrameIndex == RECON_IDX_FLAG_HEADER_ONLY  && output_info.picType == PIC_TYPE_I) {
            // release older cache header async
            if (pst_handle->header_cache_pack.len > 0 && pst_handle->header_cache_pack.u64PhyAddr) {
                VLOG(INFO, " release older header cache, ref:%d\n", pst_handle->header_cache_ref);
                if (pst_handle->header_cache_ref == 0) {
                    header_vb_buffer.phys_addr = pst_handle->header_cache_pack.u64PhyAddr;
                    header_vb_buffer.size = VENC_HEADER_BUF_SIZE;
                    vdi_free_dma_memory(pst_handle->core_idx, &header_vb_buffer, 0, 0);
                }

                memset(&pst_handle->header_cache_pack, 0, sizeof(stPack));
            }

            pst_handle->header_cache_pack.u64PhyAddr = output_info.bitstreamBuffer;
            pst_handle->header_cache_pack.addr = phys_to_virt(output_info.bitstreamBuffer);
            pst_handle->header_cache_pack.len = output_info.bitstreamSize;
            pst_handle->header_cache_pack.encSrcIdx = RECON_IDX_FLAG_HEADER_ONLY;
            pst_handle->header_cache_pack.NalType =
                (pst_handle->open_param.bitstreamFormat == STD_HEVC) ? NAL_VPS : NAL_SPS;;
            pst_handle->header_cache_pack.need_free = FALSE;
            pst_handle->header_cache_pack.u64PTS = 0;
            pst_handle->header_cache_pack.u64DTS = 0;
            pst_handle->header_cache_pack.bUsed = FALSE;
            vdi_invalidate_ion_cache(pst_handle->header_cache_pack.u64PhyAddr
                        , pst_handle->header_cache_pack.addr
                        , pst_handle->header_cache_pack.len);
            VLOG(INFO, "header done\n");
            return 0;
        }  else if (output_info.picType == PIC_TYPE_I || output_info.picType == PIC_TYPE_IDR) {
            if (pst_handle->header_cache_pack.len > 0) {
                // header and idr need 2 packs, use cache header
                if (Queue_Get_Cnt(pst_handle->stream_packs) < (MAX_NUM_PACKS-1)) {
                    memcpy(&encode_pack, &pst_handle->header_cache_pack, sizeof(stPack));
                    Queue_Enqueue(pst_handle->stream_packs, &encode_pack);
                    pst_handle->header_cache_ref++;
                }
            }
        }

        if (output_info.avgCtuQp > 0) {
            pst_handle->last_frame_qp = output_info.avgCtuQp;
            ret = venc_insert_userdata(pst_handle);
            if (ret == FALSE) {
                VLOG(ERR, "venc_insert_userdata failed %d\n", ret);
            }
        }

        vdi_invalidate_ion_cache(output_info.bitstreamBuffer
                                , phys_to_virt(output_info.bitstreamBuffer)
                                , output_info.bitstreamSize);

        // drop this packet
        if (Queue_Get_Cnt(pst_handle->stream_packs) >= MAX_NUM_PACKS) {
            pst_handle->chn_venc_info.dropCnt++;
            Queue_Enqueue(pst_handle->free_stream_buffer, &output_info.bitstreamBuffer);
            VLOG(ERR, "bitstream queue is full!\n");
            return -1;
        }

        // check extra_free_buf_queue
        if (pst_handle->extra_free_buf_cnt > 0) {
            list_for_each_entry_safe(pst_extra_free_buf, temp_extra_buf_info, &pst_handle->extra_free_buf_queue, list) {
                if (pst_extra_free_buf && pst_extra_free_buf->extra_vb_buffer.phys_addr > 0) {
                    VLOG(INFO, "release extra released buf, phys:0x%lx, size:%d\n"
                            , pst_extra_free_buf->extra_vb_buffer.phys_addr, pst_extra_free_buf->extra_vb_buffer.size);
                    vb_buffer.phys_addr = pst_extra_free_buf->extra_vb_buffer.phys_addr;
                    vb_buffer.size = pst_extra_free_buf->extra_vb_buffer.size;
                    vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, ENC_BS, 0);
                    list_del(&pst_extra_free_buf->list);
                    osal_free(pst_extra_free_buf);
                    pst_handle->extra_free_buf_cnt--;
                }
            }
        }

        if (pst_extra_buf_info->bs_size > 0) {
            VLOG(INFO, "pre encoded size:%d, bs encoded:%d\n", pst_extra_buf_info->bs_size, output_info.bitstreamSize);
            remain_encoded_size = MAX(output_info.bitstreamSize - pst_extra_buf_info->bs_size, 0);
            osal_memcpy((void *)(pst_extra_buf_info->extra_vb_buffer.virt_addr
                            + pst_extra_buf_info->bs_size)
                        , phys_to_virt(output_info.bitstreamBuffer)
                        , remain_encoded_size);
            pst_extra_buf_info->bs_size = output_info.bitstreamSize;

            vdi_flush_ion_cache(pst_extra_buf_info->extra_vb_buffer.phys_addr
                            , phys_to_virt(pst_extra_buf_info->extra_vb_buffer.phys_addr)
                            , pst_extra_buf_info->bs_size);
            encode_pack.u64PhyAddr = pst_extra_buf_info->extra_vb_buffer.phys_addr;
            encode_pack.addr = (void *)pst_extra_buf_info->extra_vb_buffer.virt_addr;
            encode_pack.len = pst_extra_buf_info->bs_size;

            pst_extra_free_buf = osal_calloc(1, sizeof(drv_venc_extra_buf_info));
            osal_memcpy(&pst_extra_free_buf->extra_vb_buffer, &pst_extra_buf_info->extra_vb_buffer, sizeof(vpu_buffer_t));
            list_add_tail(&pst_extra_free_buf->list, &pst_handle->extra_buf_queue);
            pst_handle->extra_buf_cnt += 1;

            pst_extra_buf_info->bs_size = 0;
            Queue_Enqueue(pst_handle->free_stream_buffer, &output_info.bitstreamBuffer);
        } else {
            encode_pack.u64PhyAddr = output_info.bitstreamBuffer;
            encode_pack.addr = phys_to_virt(output_info.bitstreamBuffer);
            encode_pack.len = output_info.bitstreamSize;
        }

        encode_pack.encSrcIdx = output_info.encSrcIdx;
        encode_pack.NalType = output_info.picType;
        encode_pack.need_free = FALSE;
        encode_pack.u64PTS = output_info.pts;
        encode_pack.u64DTS =
                output_info.encPicCnt - 1 - pst_handle->bframe_delay + pst_handle->first_frame_pts;
        encode_pack.u32AvgCtuQp = output_info.avgCtuQp;
        encode_pack.bUsed = FALSE;
        encode_pack.u32EncHwTime =
                (output_info.encEncodeEndTick - output_info.encHostCmdTick)*cyclePerTick/(VPU_STAT_CYCLES_CLK/1000000);
        if (output_info.encSrcIdx >= 0 && output_info.encSrcIdx < MAX_SRC_BUFFER_NUM) {
            encode_pack.u64CustomMapAddr = pst_handle->input_frame[output_info.encSrcIdx].custom_map_addr;
        }
        Queue_Enqueue(pst_handle->stream_packs, &encode_pack);
        Queue_Enqueue(pst_handle->customMapBuffer, &pst_handle->input_frame[output_info.encSrcIdx].custom_map_addr);
    }

    if (output_info.encSrcIdx >= 0 && output_info.encSrcIdx < MAX_SRC_BUFFER_NUM) {
        release_frame_idx(pst_handle, output_info.encSrcIdx);
    }

    if (pst_handle->src_end == 1 && output_info.reconFrameIndex == RECON_IDX_FLAG_ENC_END) {
        pst_handle->ouput_end = 1;
    }

    pst_handle->chn_venc_info.getStreamCnt++;
    if (async_mode) {
        complete(&pst_handle->semEncDoneCmd);
        complete(&pst_handle->semGetStreamCmd);
    }

    wake_up(&tVencWaitQueue[pst_handle->channel_index]);
    return 0;
}

static int thread_wait_interrupt(void *param)
{
    ENCODER_HANDLE *pst_handle = param;
    EncInitialInfo init_info = {0};
    QueueStatusInfo queue_status = {0};
    int ret;
    int retry_times = 0;

    VLOG(INFO, "start\n");

    while (1) {
        if (pst_handle->stop_thread) {
            VPU_EncGiveCommand(pst_handle->handle, ENC_GET_QUEUE_STATUS, &queue_status);
            if (!queue_status.instanceQueueCount && queue_status.reportQueueEmpty)
                break;
        }

        retry_times = 0;
        ret = VPU_WaitInterruptEx(pst_handle->handle, 100);
        if (ret == -1)
            continue;

        if (ret > 0) {
            VPU_ClearInterruptEx(pst_handle->handle, ret);

            if (ret & (1 << INT_WAVE5_ENC_SET_PARAM)) {
                ret = VPU_EncCompleteSeqInit(pst_handle->handle, &init_info);
                if (ret == RETCODE_VPU_RESPONSE_TIMEOUT) {
                    VLOG(ERR, "<%s:%d> Failed to VPU_EncCompleteSeqInit()\n", __FUNCTION__, __LINE__);
                    break;
                }
            }

            if (ret & (1 << INT_WAVE5_ENC_PIC)) {
                venc_process_frame_done(pst_handle, VENC_ASYNC_TRUE);
            }

            if (ret & (1 << INT_WAVE5_BSBUF_FULL)) {
                VLOG(INFO, "INT_BSBUF_FULL 0x%x\n", ret);
                venc_process_bsbuf_full(pst_handle);
                VPU_EncUpdateBitstreamBuffer(pst_handle->handle, 0);
                continue;
            }
        }

        cond_resched();
    }

    venc_release_exten_buf(pst_handle);
    pst_handle->stop_thread = 1;
    pst_handle->thread_handle = NULL;
    return 0;
}

void internal_venc_init(void)
{
    mutex_lock(&__venc_init_mutex);
    if (VPU_GetProductId(0) != PRODUCT_ID_521) {
        mutex_unlock(&__venc_init_mutex);
        VLOG(ERR, "<%s:%d> Failed to VPU_GetProductId()\n", __FUNCTION__, __LINE__);
        return;
    }
    mutex_unlock(&__venc_init_mutex);
}

void *internal_venc_open(InitEncConfig *pInitEncCfg)
{
    BOOL ret;
    ENCODER_HANDLE *pst_handle;
    int fw_size;
    Uint16 *pus_bitCode;
    int reinit_count = 0;

    pus_bitCode = (Uint16 *)bit_code;
    fw_size = ARRAY_SIZE(bit_code);
reinit:
    ret = VPU_InitWithBitcode(0, pus_bitCode, fw_size);
    if ((ret == RETCODE_VPU_RESPONSE_TIMEOUT) && (reinit_count < 3)) {
        reinit_count++;
        goto reinit;
    } else if ((ret != RETCODE_SUCCESS) && (ret != RETCODE_CALLED_BEFORE)) {
        VLOG(ERR, "<%s:%d> Failed to VPU_InitWithBitcode()\n", __func__, __LINE__);
        return NULL;
    }

    pst_handle = vzalloc(sizeof(ENCODER_HANDLE));
    if (pst_handle == NULL)
        return NULL;

    set_open_param(&pst_handle->open_param, pInitEncCfg);
    pst_handle->open_param.coreIdx = pst_handle->core_idx;
    pst_handle->bframe_delay = presetGopDelay[pst_handle->open_param.EncStdParam.waveParam.gopPresetIdx];
    pst_handle->cmd_queue_depth = pInitEncCfg->s32CmdQueueDepth;
    pst_handle->is_isolate_send = pInitEncCfg->bIsoSendFrmEn;
    init_completion(&pst_handle->semGetStreamCmd);
    init_completion(&pst_handle->semEncDoneCmd);
    pst_handle->virtualIPeriod = pInitEncCfg->virtualIPeriod;

    VLOG(INFO, "<%s:%d> cmd_queue_depth:%d, isolate_send:%d, virtualIPeriod:%d\n"
        , __func__, __LINE__, pst_handle->cmd_queue_depth, pst_handle->is_isolate_send
        , pst_handle->virtualIPeriod);
    // for insertUserData
    pst_handle->enc_param.userDataBufSize = pInitEncCfg->userDataMaxLength;
    INIT_LIST_HEAD(&pst_handle->enc_param.userdataList);

    pst_handle->enc_param.enable_idr = TRUE;
    // for rotation and mirror direction
    pst_handle->enc_param.rotationAngle = pInitEncCfg->s32RotationAngle * 90;
    pst_handle->enc_param.mirrorDirection = pInitEncCfg->s32MirrorDirection;

    pst_handle->enc_param.rcMode = pInitEncCfg->rcMode;
    pst_handle->enc_param.iqp = pInitEncCfg->iqp;
    pst_handle->enc_param.pqp = pInitEncCfg->pqp;
    pst_handle->stream_packs = Queue_Create_With_Lock(MAX_NUM_PACKS, sizeof(stPack));

    memset(&pst_handle->bs_full_info, 0, sizeof(drv_venc_extra_buf_info));
    INIT_LIST_HEAD(&pst_handle->extra_buf_queue);
    INIT_LIST_HEAD(&pst_handle->extra_free_buf_queue);
    pst_handle->extra_buf_cnt = 0;

    pst_handle->enable_ext_rc = 1;
    return pst_handle;
}

int internal_venc_close(void *handle)
{
    int i;
    vpu_buffer_t vb_buffer;
    ENCODER_HANDLE *pst_handle = handle;
    drv_enc_param *pEncParam = &pst_handle->enc_param;
    UserDataList *userdataNode = NULL;
    UserDataList *n;
    int int_reason = 0;

    if (pst_handle->thread_handle != NULL) {
        pst_handle->stop_thread = 1;
        osal_thread_join(pst_handle->thread_handle, NULL);
    }

    while (VPU_EncClose(pst_handle->handle) == RETCODE_VPU_STILL_RUNNING) {
        if ((int_reason = VPU_WaitInterruptEx(pst_handle->handle, 1000*1000)) == -1) {
            VLOG(ERR, "NO RESPONSE FROM VPU_EncClose2()\n");
            break;
        }
        else if (int_reason > 0) {
            VPU_ClearInterruptEx(pst_handle->handle, int_reason);
            if (int_reason & (1 << INT_WAVE5_ENC_PIC)) {
                EncOutputInfo   outputInfo;
                VLOG(INFO, "VPU_EncClose() : CLEAR REMAIN INTERRUPT\n");
                VPU_EncGetOutputInfo(pst_handle->handle, &outputInfo);
                continue;
            }
        }
        osal_msleep(10);
    }

    // release cache header
    if (pst_handle->header_cache_pack.len > 0 && pst_handle->header_cache_pack.u64PhyAddr) {
        vb_buffer.phys_addr = pst_handle->header_cache_pack.u64PhyAddr;
        vb_buffer.size = VENC_HEADER_BUF_SIZE;
        vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
        memset(&pst_handle->header_cache_pack, 0, sizeof(stPack));
    }

    for (i = 0; i < pst_handle->min_recon_frame_count; i++) {
        if (pst_handle->pst_frame_buffer[i].size == 0)
            continue;

        vb_buffer.phys_addr = pst_handle->pst_frame_buffer[i].bufY;
        vb_buffer.size = pst_handle->pst_frame_buffer[i].size;
        vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
    }

    if (pst_handle->pst_frame_buffer != NULL) {
        vfree(pst_handle->pst_frame_buffer);
        pst_handle->pst_frame_buffer = NULL;
    }

    if (!pst_handle->use_extern_bs_buf) {
        for (i = 0; i < pst_handle->min_src_frame_count; i++) {
            if (pst_handle->bitstream_buffer[i] == 0)
                continue;

            vb_buffer.phys_addr = pst_handle->bitstream_buffer[i];
            vb_buffer.size = pst_handle->open_param.bitstreamBufferSize;
            vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, ENC_BS, 0);
        }
    } else {
        for (i = 0; i < pst_handle->min_src_frame_count; i++) {
            if (pst_handle->bitstream_buffer[i] == 0)
                continue;

            vb_buffer.phys_addr = pst_handle->bitstream_buffer[i];
            vb_buffer.size = pst_handle->open_param.bitstreamBufferSize;
            vdi_remove_extern_memory(pst_handle->core_idx, &vb_buffer, ENC_BS, 0);
        }
    }

    while(Queue_Get_Cnt(pst_handle->customMapBuffer) > 0) {
        PhysicalAddress *phys_addr = Queue_Dequeue(pst_handle->customMapBuffer);
        vb_buffer.phys_addr = *phys_addr;
        vb_buffer.size = pst_handle->customMapBufferSize;
        vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, ENC_ETC, 0);
    }

    if (pEncParam->vbVuiRbsp.size) {
        vdi_free_dma_memory(pst_handle->core_idx, &pEncParam->vbVuiRbsp, ENC_ETC, 0);
        pEncParam->vbVuiRbsp.size = 0;
        pEncParam->vbVuiRbsp.phys_addr = 0UL;
    }

    Queue_Destroy(pst_handle->stream_packs);
    Queue_Destroy(pst_handle->free_stream_buffer);
    Queue_Destroy(pst_handle->customMapBuffer);
    VPU_DeInit(pst_handle->core_idx);

    list_for_each_entry_safe(userdataNode, n, &pEncParam->userdataList, list) {
        if (userdataNode->userDataBuf != NULL && userdataNode->userDataLen != 0) {
            osal_free(userdataNode->userDataBuf);
            list_del(&userdataNode->list);
            osal_free(userdataNode);
        }
    }
    pEncParam->userDataBufSize = 0;

    vfree(pst_handle);
    return 0;
}

int build_encode_header(void *handle, EncHeaderParam *pst_enc_param, BOOL is_wait, BOOL is_publish)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOutputInfo output_info;
    EncInitialInfo init_info = {0};
    vpu_buffer_t header_vb_buffer;
    int int_reason = 0;
    int ret = 0;
    int retry_times = 0;

    if (pst_handle->open_param.bitstreamFormat == STD_HEVC)
        pst_enc_param->headerType = CODEOPT_ENC_VPS | CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;
    else
        pst_enc_param->headerType = CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;

    do {
        ret = VPU_EncGiveCommand(pst_handle->handle, ENC_PUT_VIDEO_HEADER, pst_enc_param);
    } while (ret == RETCODE_QUEUEING_FAILURE);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed ENC_PUT_VIDEO_HEADER(ret:%d)\n", ret);
        return ret;
    }

    if (is_wait) {
        do {
            if (retry_times >= MAX_RETRY_TIMES) {
                break;
            }
            retry_times++;
            int_reason = VPU_WaitInterruptEx(pst_handle->handle, 100*1000);
            if (int_reason < INTERRUPT_TIMEOUT_VALUE) {
                VLOG(ERR, "<%s:%d> Failed to VPU_WaitInterruptEx int_reason(%d)\n"
                    , __FUNCTION__, __LINE__, int_reason);
                ret = RETCODE_FAILURE;
                break;
            }

            if (int_reason > 0) {
                VPU_ClearInterruptEx(pst_handle->handle, int_reason);

                if (int_reason & (1 << INT_WAVE5_ENC_SET_PARAM)) {
                    ret = VPU_EncCompleteSeqInit(pst_handle->handle, &init_info);
                    continue;
                }

                if (int_reason & (1 << INT_WAVE5_ENC_PIC)) {
                    memset(&output_info, 0, sizeof(EncOutputInfo));
                    ret = VPU_EncGetOutputInfo(pst_handle->handle, &output_info);
                    if (ret != RETCODE_SUCCESS) {
                        VLOG(ERR, "Failed VPU_EncGetOutputInfo ret:%d, reason:0x%x\n", ret, output_info.errorReason);
                        ret = RETCODE_FAILURE;
                        break;
                    }

                    pst_enc_param->size = output_info.bitstreamSize;

                    ret = RETCODE_SUCCESS;
                    if (is_publish) {
                        // release older cache header
                        if (pst_handle->header_cache_pack.len > 0 && pst_handle->header_cache_pack.u64PhyAddr) {
                            header_vb_buffer.phys_addr = pst_handle->header_cache_pack.u64PhyAddr;
                            header_vb_buffer.size = VENC_HEADER_BUF_SIZE;
                            vdi_free_dma_memory(pst_handle->core_idx, &header_vb_buffer, 0, 0);
                            memset(&pst_handle->header_cache_pack, 0, sizeof(stPack));
                        }

                        pst_handle->header_cache_pack.u64PhyAddr = output_info.bitstreamBuffer;
                        pst_handle->header_cache_pack.addr = phys_to_virt(output_info.bitstreamBuffer);
                        pst_handle->header_cache_pack.len = output_info.bitstreamSize;
                        pst_handle->header_cache_pack.encSrcIdx = RECON_IDX_FLAG_HEADER_ONLY;
                        pst_handle->header_cache_pack.NalType =
                            (pst_handle->open_param.bitstreamFormat == STD_HEVC) ? NAL_VPS : NAL_SPS;
                        pst_handle->header_cache_pack.need_free = FALSE;
                        pst_handle->header_cache_pack.u64PTS = 0;
                        pst_handle->header_cache_pack.u64DTS = 0;
                        pst_handle->header_cache_pack.bUsed = FALSE;
                        vdi_invalidate_ion_cache(pst_handle->header_cache_pack.u64PhyAddr
                                    , pst_handle->header_cache_pack.addr
                                    , pst_handle->header_cache_pack.len);
                    }
                }
            }
        } while(int_reason == INTERRUPT_TIMEOUT_VALUE || !(int_reason & (1 << INT_WAVE5_ENC_PIC)) );
    }

    return ret;
}

int venc_get_encode_header(void *handle, void *arg)
{
    int ret;
    ENCODER_HANDLE *pst_handle = handle;
    EncodeHeaderInfo *encHeaderRbsp = (EncodeHeaderInfo *)arg;
    EncHeaderParam encHeaderParam = {0};
    vpu_buffer_t vb_buffer;
    uint8_t *pVirtHeaderBuf = NULL;
    BOOL is_wait_interrupt = 1;
    BOOL is_publish = 0;

    osal_memset(&vb_buffer, 0, sizeof(vpu_buffer_t));
    vb_buffer.size = VENC_HEADER_BUF_SIZE;
    vdi_allocate_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);

    osal_memset(&encHeaderParam, 0x00, sizeof(EncHeaderParam));
    encHeaderParam.buf = vb_buffer.phys_addr;
    encHeaderParam.size = VENC_HEADER_BUF_SIZE;

    ret = build_encode_header(pst_handle, &encHeaderParam, is_wait_interrupt, is_publish);
    if (ret == RETCODE_SUCCESS) {
        pVirtHeaderBuf = phys_to_virt(encHeaderParam.buf);
        vdi_invalidate_ion_cache(encHeaderParam.buf
                                    , pVirtHeaderBuf
                                    , encHeaderParam.size);
        osal_memcpy(encHeaderRbsp->headerRbsp, pVirtHeaderBuf, encHeaderParam.size);
        encHeaderRbsp->u32Len = encHeaderParam.size;
        pst_handle->header_encoded = 1;
    }

    vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
    return ret;
}

static int venc_build_enc_param(ENCODER_HANDLE *pst_handle, EncOnePicCfg *pPicCfg
                                , FrameBuffer *pst_fb, EncParam *pst_enc_param)
{
    PhysicalAddress *addr = NULL;
    PhysicalAddress *customMapAddr = NULL;
    drv_enc_param *pEncParam = &pst_handle->enc_param;
    int src_idx;

    pst_fb->bufY = pPicCfg->phyAddrY;
    pst_fb->bufCb = pPicCfg->phyAddrCb;
    pst_fb->bufCr = pPicCfg->phyAddrCr;
    pst_fb->stride = pPicCfg->stride;
    pst_fb->cbcrInterleave = pPicCfg->cbcrInterleave;
    pst_fb->nv21 = pPicCfg->nv21;

    addr = Queue_Dequeue(pst_handle->free_stream_buffer);
    if (addr == NULL)
        return RETCODE_STREAM_BUF_FULL;

    if (pPicCfg->src_end == 0) {
        if (pPicCfg->src_idx < 0) {
            if (vb_phys_addr2handle(pst_fb->bufY) == VB_INVALID_HANDLE) {
                Queue_Enqueue(pst_handle->free_stream_buffer, addr);
                return RETCODE_INVALID_PARAM;
            }

            src_idx = get_frame_idx(pst_handle, pst_fb->bufY);
            if  (src_idx < 0) {
                Queue_Enqueue(pst_handle->free_stream_buffer, addr);
                VLOG(ERR, "Failed get_frame_idx idx:%d, bufY:0x%lx\n", src_idx, pst_fb->bufY);
                return RETCODE_FAILURE;
            }
        } else {
            src_idx = pPicCfg->src_idx;
        }
    } else {
        pst_handle->src_end = 1;
        src_idx = -1;
    }

    if (src_idx >= 0 && src_idx < MAX_SRC_BUFFER_NUM) {
        pst_handle->input_frame[src_idx].pts = pPicCfg->u64Pts;
        pst_handle->input_frame[src_idx].idx = pst_handle->frame_idx;
        pst_handle->input_frame[src_idx].custom_map_addr = pEncParam->customMapOpt.addrCustomMap;
    }

    pst_enc_param->sourceFrame = pst_fb;
    pst_enc_param->quantParam = 1;
    pst_enc_param->picStreamBufferAddr = *addr;
    pst_enc_param->picStreamBufferSize = pst_handle->open_param.bitstreamBufferSize;
    pst_enc_param->codeOption.implicitHeaderEncode = 1;
    pst_enc_param->srcIdx = src_idx;
    pst_enc_param->srcEndFlag = pPicCfg->src_end;
    pst_enc_param->pts = pPicCfg->u64Pts;
    if (pst_handle->virtualIPeriod > 0) {
        pst_enc_param->useCurSrcAsLongtermPic
            = (pst_handle->frame_idx % pst_handle->virtualIPeriod) == 0 ? 1 : 0;
        pst_enc_param->useLongtermRef
            = (pst_handle->frame_idx % pst_handle->virtualIPeriod) == 0 ? 1 : 0;
    }

    if (pEncParam->rcMode == RC_MODE_FIXQP) {
        pst_enc_param->forcePicQpEnable = 1;
        pst_enc_param->forcePicQpI = pEncParam->iqp;
        pst_enc_param->forcePicQpP = pEncParam->pqp;
        pst_enc_param->forcePicQpB = pEncParam->pqp;
    }

    { // if current frame have roi region
        int i;
        for (i = 0; i < MAX_NUM_ROI; i++) {
            if (pst_handle->roi_rect[i].roi_enable_flag == 1) {
                pEncParam->isCustomMapFlag = TRUE;
                break;
            }
        }
        if (pst_handle->last_frame_qp == 0) {  //for first frame, no last qp
            pst_handle->last_frame_qp = pst_handle->open_param.EncStdParam.waveParam.initialRcQp;
        }
    }
    if (src_idx >= 0 && src_idx < MAX_SRC_BUFFER_NUM) {
        if (pEncParam->isCustomMapFlag == TRUE && pEncParam->edge_api_enable == FALSE) {
            pEncParam->customMapOpt.customRoiMapEnable = true;
            customMapAddr = Queue_Dequeue(pst_handle->customMapBuffer);
            if (customMapAddr == NULL)
                return RETCODE_STREAM_BUF_FULL;

            venc_help_set_mapdata(pst_handle->core_idx, pst_handle->roi_rect, pst_handle->last_frame_qp, pst_handle->open_param.picWidth, pst_handle->open_param.picHeight,
                                    &(pEncParam->customMapOpt), pst_handle->open_param.bitstreamFormat, customMapAddr);
        }
        pst_handle->input_frame[src_idx].pts = pPicCfg->u64Pts;
        pst_handle->input_frame[src_idx].idx = pst_handle->frame_idx;
        pst_handle->input_frame[src_idx].custom_map_addr = pEncParam->customMapOpt.addrCustomMap;
    }

    // update custom map
    if (pEncParam->isCustomMapFlag == TRUE) {
        // enc_param.customMapOpt.
        memcpy(&pst_enc_param->customMapOpt, &pEncParam->customMapOpt, sizeof(WaveCustomMapOpt));
        pEncParam->isCustomMapFlag = FALSE;
    }
    return 0;
}

int internal_venc_enc_one_pic(void *handle, EncOnePicCfg *pPicCfg, int s32MilliSec)
{
    int ret;
    EncHeaderParam encHeaderParam = {0};
    EncParam enc_param = {0};
    FrameBuffer src_buffer = {0};
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    vpu_buffer_t header_vb_buffer;
    QueueStatusInfo queue_status = {0};
    CodecInst *pCodecInst = NULL;
    BOOL is_wait_interrupt = 0;
    BOOL is_publish = 0;
    BOOL is_header_update = FALSE;

    // create venc_wait thread
    // 1. bind_mode is false
    // 2. bind_mode is true and is_isolate_send is true
    if ((!pst_handle->is_bind_mode || (pst_handle->is_bind_mode && pst_handle->is_isolate_send))
        && (pst_handle->thread_handle == NULL || pst_handle->stop_thread == 1)) {
        struct sched_param param = {
            .sched_priority = 95,
        };
        VLOG(INFO, "internal_venc_enc_one_pic create venc wait thread, chn:%d\n", pst_handle->channel_index);

        pst_handle->stop_thread = 0;
        pst_handle->thread_handle = kthread_run(thread_wait_interrupt, pst_handle, "soph_vc_wait%d", pst_handle->channel_index);
        if (IS_ERR(pst_handle->thread_handle)) {
            pst_handle->thread_handle = NULL;
            return RETCODE_FAILURE;
        }
        sched_setscheduler(pst_handle->thread_handle, SCHED_RR, &param);
    }

    // alloc bit stream buf
    if (!pst_handle->bitstream_buffer[0]) {
        ret = alloc_bitstream_buf(pst_handle);
        if (ret) {
            return RETCODE_FAILURE;
        }
    }

    pst_handle->chn_venc_info.sendAllYuvCnt++;
    VPU_EncGiveCommand(pst_handle->handle, ENC_GET_QUEUE_STATUS, &queue_status);
    if (queue_status.instanceQueueFull == TRUE) {
        VLOG(WARN, "vpu queue fail count:%d, queue full:%d\n"
            , queue_status.instanceQueueCount, queue_status.instanceQueueFull);
        return RETCODE_QUEUEING_FAILURE;
    }

    // update enc param
    pCodecInst = pst_handle->handle;
    pCodecInst->CodecInfo->encInfo.openParam.cbcrInterleave =
        pst_handle->open_param.cbcrInterleave;
    pCodecInst->CodecInfo->encInfo.openParam.nv21 = pst_handle->open_param.nv21;

    memset(&enc_param, 0, sizeof(EncParam));
    enc_param.is_idr_frame = venc_check_idr_period(pst_handle);

    // for avbr bitrate estimate
    pst_open_param->picMotionLevel = pPicCfg->picMotionLevel;
    enc_param.picMotionLevel = pPicCfg->picMotionLevel;

    // check param change
    venc_picparam_change_ctrl(pst_handle, &enc_param, &is_header_update);
    if (pst_handle->frame_idx == 0) {
        is_header_update = TRUE;
    }

    VLOG(INFO, "chn:%d frameidx:%d idr:%d header_update:%d\n"
        , pst_handle->channel_index, pst_handle->frame_idx, enc_param.is_idr_frame, is_header_update);
    if (enc_param.is_idr_frame && is_header_update && !pst_handle->header_encoded) {
        osal_memset(&header_vb_buffer, 0, sizeof(vpu_buffer_t));
        header_vb_buffer.size = VENC_HEADER_BUF_SIZE;
        if (vdi_allocate_dma_memory(pst_handle->core_idx, &header_vb_buffer, ENC_BS, 0) < 0) {
            VLOG(ERR, "fail to allocate header buffer\n" );
            return RETCODE_FAILURE;
        }

        if (!pst_handle->thread_handle) {
            is_wait_interrupt = 1;
            is_publish = 1;
        }

        VLOG(INFO, "build header wait:%d publish:%d\n", is_wait_interrupt, is_publish);
        osal_memset(&encHeaderParam, 0x00, sizeof(EncHeaderParam));
        encHeaderParam.buf = header_vb_buffer.phys_addr;
        encHeaderParam.size = VENC_HEADER_BUF_SIZE;
        ret = build_encode_header(pst_handle, &encHeaderParam, is_wait_interrupt, is_publish);
        if (ret != RETCODE_SUCCESS) {
            vdi_free_dma_memory(pst_handle->core_idx, &header_vb_buffer, 0, 0);
            VLOG(ERR, "Failed ENC_PUT_VIDEO_HEADER(ret:%d)\n", ret);
            return ret;
        }
    }

    ret = venc_build_enc_param(pst_handle, pPicCfg, &src_buffer, &enc_param);
    if (ret == RETCODE_STREAM_BUF_FULL) {
        VLOG(WARN, "venc_build_enc_param streambuf not enough, ret:%d, remain packs:%d\n"
            , ret, Queue_Get_Cnt(pst_handle->stream_packs));
        return ret;
    } else if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed venc_build_enc_param(ret:%d)\n", ret);
        return ret;
    }

    ret = VPU_EncStartOneFrame(pst_handle->handle, &enc_param);
    if (ret == RETCODE_QUEUEING_FAILURE) {
        osal_msleep(1);
        ret = VPU_EncStartOneFrame(pst_handle->handle, &enc_param);
    }
    if (ret != RETCODE_SUCCESS) {
        release_frame_idx(pst_handle, enc_param.srcIdx);
        Queue_Enqueue(pst_handle->free_stream_buffer, &enc_param.picStreamBufferAddr);
        if (ret == RETCODE_QUEUEING_FAILURE) {
            VLOG(WARN, "VPU_EncStartOneFrame queue fail (ret:%d), chn:%d\n", ret, pst_handle->channel_index);
        } else {
            VLOG(ERR, "Failed VPU_EncStartOneFrame(ret:%d), chn:%d\n", ret, pst_handle->channel_index);
        }
        return ret;
    }

    pst_handle->chn_venc_info.sendOkYuvCnt++;
    if (pst_handle->frame_idx == 0) {
        pst_handle->first_frame_pts = enc_param.pts;
    }

    pst_handle->frame_idx++;
    if (pst_handle->src_end && pst_handle->ouput_end == 0) {
        VLOG(WARN, "vpu queue fail for wait output end \n");
        return RETCODE_QUEUEING_FAILURE;
    }

    return 0;
}

static int venc_get_encoded_info(void *handle, int s32MilliSec)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncInitialInfo init_info = {0};
    int int_reason = 0;
    int ret = 0;

    do {
        int_reason = VPU_WaitInterruptEx(pst_handle->handle, s32MilliSec*1000);
        if (int_reason == -1) {
            if (s32MilliSec >= 0) {
                VLOG(WARN, "Error : encoder timeout happened in non_block mode \n");
                return -2;
            }
        }

        if (int_reason > 0) {
            VPU_ClearInterruptEx(pst_handle->handle, int_reason);

            if (int_reason & (1 << INT_WAVE5_ENC_SET_PARAM)) {
                ret = VPU_EncCompleteSeqInit(pst_handle->handle, &init_info);
                if (ret == RETCODE_VPU_RESPONSE_TIMEOUT) {
                    VLOG(ERR, "<%s:%d> Failed to VPU_EncCompleteSeqInit()\n", __FUNCTION__, __LINE__);
                    return -1;
                }
                continue;
            }

            if (int_reason & (1 << INT_WAVE5_ENC_PIC)) {
                if (venc_process_frame_done(pst_handle, VENC_ASYNC_FALSE)) {
                    return -1;
                }
                return 0;
            }

            if (ret & (1 << INT_WAVE5_BSBUF_FULL)) {
                VLOG(INFO, "INT_BSBUF_FULL 0x%x\n", ret);
                venc_process_bsbuf_full(pst_handle);
                VPU_EncUpdateBitstreamBuffer(pst_handle->handle, 0);
            }
        }
    } while (int_reason == INTERRUPT_TIMEOUT_VALUE || int_reason == 0 || int_reason == 512);

    VLOG(ERR, "impossible here, chn:%d int reson:%d\n", pst_handle->channel_index, int_reason);
    return -1;
}

int internal_venc_get_stream(void *handle, VEncStreamInfo *pStreamInfo, int s32MilliSec)
{
    ENCODER_HANDLE *pst_handle = handle;
    int ret = RETCODE_SUCCESS;
    unsigned long timeout = 0;

    if (!pst_handle->is_bind_mode || pst_handle->is_isolate_send) {
        if (s32MilliSec > 0) {
            timeout = jiffies + msecs_to_jiffies(s32MilliSec);
        REWAIT:
            ret = wait_for_completion_timeout(&pst_handle->semGetStreamCmd,
                    msecs_to_jiffies(s32MilliSec));
            if (pst_handle->ouput_end == 1)
                return 0;

            if (ret == 0) {
                if (Queue_Get_Cnt(pst_handle->stream_packs) > 0) {
                    pStreamInfo->psp = pst_handle->stream_packs;
                    return 0;
                }
                return -2;
            } else if (Queue_Get_Cnt(pst_handle->stream_packs) == 0) {
                if (time_after(jiffies, timeout)){
                    return -2;
                }
                // wait again
                goto REWAIT;
            }
        }
    } else if (pst_handle->is_bind_mode) {
        ret = venc_get_encoded_info(handle, s32MilliSec);
        if (ret != RETCODE_SUCCESS) {
            VLOG(ERR, "venc_get_encoded_info ret %d\n", ret);
            return ret;
        }
    }
    VLOG(INFO, "chn:%d iso:%d, ret:%d\n"
            , pst_handle->channel_index, pst_handle->is_isolate_send, ret);
    if (Queue_Get_Cnt(pst_handle->stream_packs) == 0) {
        if (pst_handle->ouput_end == 1) {
            return 0;
        }
        return -2;
    }
    pStreamInfo->psp = pst_handle->stream_packs;
    return 0;
}

int internal_venc_release_stream(void *handle, stPack *pstPack, unsigned int packCnt)
{
    int i = 0;
    ENCODER_HANDLE *pst_handle = handle;
    drv_venc_extra_buf_info *extra_buf_info = NULL;
    drv_venc_extra_buf_info *n;
    vpu_buffer_t vb_buffer;
    int extra_buf_flag = 0;

    if (!pstPack || packCnt == 0)
        return 0;

    for (i=0; i < packCnt; i++) {
        if (pstPack[i].NalType == NAL_SEI) {
            osal_kfree(pstPack[i].addr);
        } else if (pstPack[i].NalType == NAL_VPS || pstPack[i].NalType == NAL_SPS) {
            pst_handle->header_cache_ref--;
            // release older header cache
            if ((pstPack[i].u64PhyAddr != pst_handle->header_cache_pack.u64PhyAddr) && (pst_handle->header_cache_ref == 0)) {
                vb_buffer.phys_addr = pstPack[i].u64PhyAddr;
                vb_buffer.size = VENC_HEADER_BUF_SIZE;
                vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
            }
        } else {
            extra_buf_flag = 0;
            if (pst_handle->extra_buf_cnt > 0) {
                // release extra bitstream buf into extra_free_buf_queue
                list_for_each_entry_safe(extra_buf_info, n, &pst_handle->extra_buf_queue, list) {
                    if (extra_buf_info && extra_buf_info->extra_vb_buffer.phys_addr == pstPack[i].u64PhyAddr) {
                        list_del(&extra_buf_info->list);
                        list_add_tail(&extra_buf_info->list, &pst_handle->extra_free_buf_queue);
                        pst_handle->extra_free_buf_cnt++;
                        pst_handle->extra_buf_cnt--;
                        extra_buf_flag = 1;
                    }
                }
            }

            if (0 == extra_buf_flag) {
                Queue_Enqueue(pst_handle->free_stream_buffer, &pstPack[i].u64PhyAddr);
            }
        }
    }

    return 0;
}

int venc_op_set_rc_param(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    RcParam *prcp = (RcParam *)arg;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    int ret = 0;

    pst_open_param->rcIntervalMode = prcp->u32RowQpDelta;
    pst_open_param->rcInitDelay = prcp->s32InitialDelay;

    pst_open_param->maxIprop = prcp->u32MaxIprop;
    pst_open_param->minIprop = prcp->u32MinIprop;
    pst_open_param->avbrFrmLostOpen = prcp->s32AvbrFrmLostOpen;
    pst_open_param->avbrFrmGaps = prcp->s32AvbrFrmGap;
    pst_open_param->maxStillQp = prcp->u32MaxStillQP;
    pst_open_param->motionSensitivity = prcp->u32MotionSensitivity;
    pst_open_param->avbrPureStillThr = prcp->s32AvbrPureStillThr;
    pst_open_param->minStillPercent = prcp->s32MinStillPercent;
    pst_open_param->changePos = prcp->s32ChangePos;

    param->initialRcQp = prcp->firstFrmstartQp;
    param->hvsQpScale =
        (prcp->u32ThrdLv <= 4) ? (int)prcp->u32ThrdLv : 2;

    if (pst_open_param->bitstreamFormat == STD_HEVC) {
        param->cuLevelRCEnable = (prcp->u32RowQpDelta > 0);
    } else {
        param->mbLevelRcEnable = (prcp->u32RowQpDelta > 0);
    }

    param->maxQpI = prcp->u32MaxIQp;
    param->minQpI = prcp->u32MinIQp;
    param->maxQpP = prcp->u32MaxQp;
    param->minQpP = prcp->u32MinQp;
    param->maxQpB = prcp->u32MaxQp;
    param->minQpB = prcp->u32MinQp;

    return ret;
}

int venc_op_start(void *handle, void *arg)
{
    int ret;
    ENCODER_HANDLE *pst_handle = handle;
    EncInitialInfo init_info = {0};
    SecAxiUse secAxiUse = {0};
    int cyclePerTick = 32768;
    VpuAttr product_attr;
    int chn = *(int *)arg;
    int int_reason = 0;

    ret = VPU_GetProductInfo(pst_handle->core_idx, &product_attr);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    ret = VPU_EncOpen(&pst_handle->handle, &pst_handle->open_param);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed to VPU_EncOpen(ret:%d)\n", ret);
        return ret;
    }
    pst_handle->channel_index = chn;

    // rotation and mirror direction
    if (pst_handle->enc_param.rotationAngle) {
        VPU_EncGiveCommand(pst_handle->handle, ENABLE_ROTATION, 0);
        VPU_EncGiveCommand(pst_handle->handle, SET_ROTATION_ANGLE, &pst_handle->enc_param.rotationAngle);

    }

    if (pst_handle->enc_param.mirrorDirection) {
        VPU_EncGiveCommand(pst_handle->handle, ENABLE_MIRRORING, 0);
        VPU_EncGiveCommand(pst_handle->handle, SET_MIRROR_DIRECTION, &pst_handle->enc_param.mirrorDirection);
    }

    // when picWidth less than 4608, support secAXI
    if (pst_handle->open_param.picWidth <= 4608) {
        secAxiUse.u.wave.useEncRdoEnable = TRUE;
        secAxiUse.u.wave.useEncLfEnable  = TRUE;
    } else {
        secAxiUse.u.wave.useEncRdoEnable = FALSE;
        secAxiUse.u.wave.useEncLfEnable  = FALSE;
    }

    VPU_EncGiveCommand(pst_handle->handle, SET_SEC_AXI, &secAxiUse);

    if (product_attr.supportNewTimer == TRUE)
        cyclePerTick = 256;
    VPU_EncGiveCommand(pst_handle->handle, SET_CYCLE_PER_TICK, (void *)&cyclePerTick);

    do {
        ret = VPU_EncIssueSeqInit(pst_handle->handle);
    } while (ret == RETCODE_QUEUEING_FAILURE);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "<%s:%d> Failed to VPU_EncIssueSeqInit() ret(%d)\n"
            , __FUNCTION__, __LINE__, ret);
        return ret;
    }

    do {
        int_reason = VPU_WaitInterruptEx(pst_handle->handle, 1000*1000);
    } while (int_reason == INTERRUPT_TIMEOUT_VALUE);

    if (int_reason < 0) {
        VLOG(ERR, "<%s:%d> Failed to VPU_WaitInterruptEx int_reason(%d)\n"
            , __FUNCTION__, __LINE__, int_reason);
        goto ERR_VPU_ENC_OPEN;
    }

    if (int_reason > 0)
        VPU_ClearInterruptEx(pst_handle->handle, int_reason);

    if (int_reason & (1 << INT_WAVE5_ENC_SET_PARAM)) {
        ret = VPU_EncCompleteSeqInit(pst_handle->handle, &init_info);
        if (ret == RETCODE_VPU_RESPONSE_TIMEOUT) {
            VLOG(ERR, "<%s:%d> Failed to VPU_EncCompleteSeqInit()\n", __FUNCTION__, __LINE__);
            goto ERR_VPU_ENC_OPEN;
        }

        pst_handle->min_recon_frame_count = init_info.minFrameBufferCount;
        pst_handle->min_src_frame_count = init_info.minSrcFrameCount + pst_handle->cmd_queue_depth;

        VLOG(INFO, "gop_preset:%d, cmdqueue:%d, min_recon_cnt:%d, min_src_cnt:%d\n"
            , pst_handle->open_param.EncStdParam.waveParam.gopPresetIdx, pst_handle->cmd_queue_depth
            , pst_handle->min_recon_frame_count, pst_handle->min_src_frame_count);

        ret = alloc_framebuffer(pst_handle);
        if ( ret != RETCODE_SUCCESS) {
            VLOG(ERR, "<%s:%d> Failed to alloc_framebuffer\n", __FUNCTION__, __LINE__);
            goto ERR_VPU_ENC_OPEN;
        }

        // additional is for store header and header_backup
        pst_handle->free_stream_buffer = Queue_Create_With_Lock(pst_handle->min_src_frame_count, sizeof(PhysicalAddress));

        if (pst_handle->enable_ext_rc == 1) {
            // pst_handle->open_param.RcEn = 1;
            venc_rc_open(&pst_handle->rc_info, &pst_handle->open_param);
        }
        return RETCODE_SUCCESS;
    }

ERR_VPU_ENC_OPEN:
    VPU_EncClose(pst_handle->handle);
    return RETCODE_FAILURE;
}

int venc_op_get_fd(void *handle, void *arg)
{
    return 0;
}

int venc_op_set_request_idr(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;

    pst_handle->enc_param.idr_request = TRUE;
    return 0;
}

int venc_op_set_enable_idr(void *handle, void *arg)
{
    BOOL bEnable = *(unsigned char *)arg;
    ENCODER_HANDLE *pst_handle = handle;
    int period;

    if (bEnable == TRUE) {
        if (pst_handle->enc_param.enable_idr == FALSE) {
            // next frame set to idr
            pst_handle->enc_param.enable_idr = TRUE;
            pst_handle->enc_param.idr_request = TRUE;
            // restore old period
            period = -1;
            VPU_EncGiveCommand(pst_handle->handle, ENC_SET_PERIOD, &period);
        }
    } else {
          period = 0;
          pst_handle->enc_param.enable_idr = FALSE;
          pst_handle->enc_param.idr_request = FALSE;
          VPU_EncGiveCommand(pst_handle->handle, ENC_SET_PERIOD, &period);
    }

    return 0;
}

int venc_op_set_chn_attr(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    VidChnAttr *pChnAttr = (VidChnAttr *)arg;
    unsigned int frameRateDiv = 0, frameRateRes = 0;

    pst_open_param->bitRate = pChnAttr->u32BitRate*1024;

    frameRateDiv = pChnAttr->fr32DstFrameRate >> 16;
    frameRateRes = pChnAttr->fr32DstFrameRate & 0xFFFF;

    if (frameRateDiv == 0) {
        pst_open_param->frameRateInfo = frameRateRes;
        param->numUnitsInTick  = 1000;
        frameRateRes *= 1000;
    } else {
        pst_open_param->frameRateInfo = ((frameRateDiv - 1) << 16) + frameRateRes;
        param->numUnitsInTick  = frameRateDiv;
    }

    if (pst_open_param->bitstreamFormat == STD_AVC) {
        param->timeScale = frameRateRes * 2;
    } else {
        param->timeScale = frameRateRes;
    }

    return 0;
}

int venc_op_set_ref(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    unsigned int *tempLayer = (unsigned int *)arg;

    if (*tempLayer == 2) {
        param->gopPresetIdx = 0;
        param->gopParam.customGopSize = 2;

        param->gopParam.picParam[0].picType = PIC_TYPE_P;
        param->gopParam.picParam[0].pocOffset = 1;
        param->gopParam.picParam[0].picQp = 26;
        param->gopParam.picParam[0].refPocL0 = 0;
        param->gopParam.picParam[0].refPocL1 = 0;
        param->gopParam.picParam[0].temporalId  = 1;

        param->gopParam.picParam[1].picType = PIC_TYPE_P;
        param->gopParam.picParam[1].pocOffset = 2;
        param->gopParam.picParam[1].picQp = 28;
        param->gopParam.picParam[1].refPocL0 = 0;
        param->gopParam.picParam[1].refPocL1 = 0;
        param->gopParam.picParam[1].temporalId  = 0;
    } else if (*tempLayer == 3) {
        param->gopPresetIdx = 0;
        param->gopParam.customGopSize = 4;

        param->gopParam.picParam[0].picType = PIC_TYPE_P;
        param->gopParam.picParam[0].pocOffset = 1;
        param->gopParam.picParam[0].picQp = 30;
        param->gopParam.picParam[0].refPocL0 = 0;
        param->gopParam.picParam[0].refPocL1 = 0;
        param->gopParam.picParam[0].temporalId  = 2;

        param->gopParam.picParam[1].picType = PIC_TYPE_P;
        param->gopParam.picParam[1].pocOffset = 2;
        param->gopParam.picParam[1].picQp = 28;
        param->gopParam.picParam[1].refPocL0 = 0;
        param->gopParam.picParam[1].refPocL1 = 0;
        param->gopParam.picParam[1].temporalId  = 1;

        param->gopParam.picParam[2].picType = PIC_TYPE_P;
        param->gopParam.picParam[2].pocOffset = 3;
        param->gopParam.picParam[2].picQp = 30;
        param->gopParam.picParam[2].refPocL0 = 2;
        param->gopParam.picParam[2].refPocL1 = 0;
        param->gopParam.picParam[2].temporalId  = 2;

        param->gopParam.picParam[3].picType = PIC_TYPE_P;
        param->gopParam.picParam[3].pocOffset = 4;
        param->gopParam.picParam[3].picQp = 26;
        param->gopParam.picParam[3].refPocL0 = 0;
        param->gopParam.picParam[3].refPocL1 = 0;
        param->gopParam.picParam[3].temporalId  = 0;
    }

    return 0;
}

int venc_op_set_roi(void *handle, void *arg)
{
    RoiParam *roiParam = (RoiParam *)arg;
    ENCODER_HANDLE *pst_handle = (ENCODER_HANDLE *)handle;
    int idx;
    vpu_buffer_t vb_buffer;
    int index = roiParam->roi_index;
    int picWidth = pst_handle->open_param.picWidth;
    int picHeight = pst_handle->open_param.picHeight;
    int MbWidth  =  VPU_ALIGN16(picWidth) >> 4;
    int MbHeight =  VPU_ALIGN16(picHeight) >> 4;

    int ctuMapWidthCnt  = VPU_ALIGN64(picWidth) >> 6;
    int ctuMapHeightCnt = VPU_ALIGN64(picHeight) >> 6;

    int MB_NUM = MbWidth * MbHeight;
    int CTU_NUM = ctuMapWidthCnt * ctuMapHeightCnt ;


    // when first set roi,you need to alloc memory
    if (pst_handle->customMapBuffer == NULL) {
        pst_handle->customMapBuffer = Queue_Create_With_Lock(pst_handle->min_src_frame_count, sizeof(PhysicalAddress));

        memset(&vb_buffer, 0, sizeof(vpu_buffer_t));
        pst_handle->customMapBufferSize = (pst_handle->open_param.bitstreamFormat == STD_AVC) ? MB_NUM : CTU_NUM * 8;
        vb_buffer.size = pst_handle->customMapBufferSize;

        for (idx = 0; idx < pst_handle->min_src_frame_count; idx++) {
            if (vdi_allocate_dma_memory(pst_handle->core_idx, &vb_buffer, ENC_ETC, 0) < 0) {
                VLOG(ERR,"vdi_allocate_dma_memory vbCustomMap failed\n",__func__,__LINE__);
                return RETCODE_FAILURE;
            }
            Queue_Enqueue(pst_handle->customMapBuffer, &vb_buffer.phys_addr);
        }
    }
    if (index < 0 || index > MAX_NUM_ROI) {
        VLOG(ERR, "<%s:%d> roi index is invalid\n", __FUNCTION__, __LINE__);
        return -1;
    }
    memcpy(&pst_handle->roi_rect[index],roiParam,sizeof(RoiParam));
    return 0;
}

int venc_op_get_roi(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    RoiParam *roiParam = (RoiParam *)arg;

    int index = roiParam->roi_index;
    const RoiParam *pRoiParam = NULL;

    if (index > MAX_NUM_ROI || index < 0) {
        VLOG(ERR, "<%s:%d> roi index is invalid\n", __FUNCTION__, __LINE__);
        return -1;
    }

    pRoiParam = &pst_handle->roi_rect[index];

    roiParam->roi_enable_flag = pRoiParam->roi_enable_flag;
    roiParam->is_absolute_qp = pRoiParam->is_absolute_qp;
    roiParam->roi_qp = pRoiParam->roi_qp;
    roiParam->roi_rect_x = pRoiParam->roi_rect_x;
    roiParam->roi_rect_y = pRoiParam->roi_rect_y;
    roiParam->roi_rect_width = pRoiParam->roi_rect_width;
    roiParam->roi_rect_height = pRoiParam->roi_rect_height;
    return 0;
}

int venc_op_set_framelost(void *handle, void *arg)
{
    return 0;
}

int venc_op_get_vbinfo(void *handle, void *arg)
{
    return 0;
}

int venc_op_set_vbbuffer(void *handle, void *arg)
{
    return 0;
}

int venc_op_encode_userdata(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    UserData *pSrc = (UserData *)arg;
    unsigned int len;
    UserDataList *userdataNode = NULL;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    drv_enc_param *pEncParam = &pst_handle->enc_param;

    if (pSrc == NULL || pSrc->userData == NULL || pSrc->len == 0) {
        VLOG(ERR, "no user data\n");
        return -1;
    }

    userdataNode = (UserDataList *)osal_calloc(1, sizeof(UserDataList));
    if (userdataNode == NULL)
        return -1;

    userdataNode->userDataBuf = (Uint8 *)osal_calloc(1, pEncParam->userDataBufSize);
    if (userdataNode->userDataBuf == NULL) {
        osal_free(userdataNode);
        return -1;
    }

    len = venc_help_sei_encode(pst_open_param->bitstreamFormat, pSrc->userData,
              pSrc->len, userdataNode->userDataBuf, pEncParam->userDataBufSize);

    if (len > pEncParam->userDataBufSize) {
        VLOG(ERR, "encoded user data len %d exceeds buffer size %d\n",
            len, pEncParam->userDataBufSize);
        osal_free(userdataNode);
        osal_free(userdataNode->userDataBuf);
        return -1;
    }

    userdataNode->userDataLen = len;
    list_add_tail(&userdataNode->list, &pEncParam->userdataList);

    return 0;
}

int venc_op_set_pred(void *handle, void *arg)
{
    // wave521 not support
    return 0;
}

int venc_op_set_h264_entropy(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H264Entropy *pSrc = (H264Entropy *)arg;
    int h264EntropyMode = 0;

    if (pSrc == NULL) {
        VLOG(ERR, "no h264 entropy data\n");
        return -1;
    }

    if (pSrc->entropyEncModeI == 0 && pSrc->entropyEncModeP == 0)
        h264EntropyMode = 0;
    else
        h264EntropyMode = 1;

    param->entropyCodingMode = h264EntropyMode;
    return 0;
}

int venc_op_set_h265_predunit(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H265Pu *pSrc = (H265Pu *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no h265 PredUnit data\n");
        return -1;
    }

    param->constIntraPredFlag = pSrc->constrained_intra_pred_flag;
    param->strongIntraSmoothEnable = pSrc->strong_intra_smoothing_enabled_flag;

    return 0;
}

int venc_op_get_h265_predunit(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H265Pu *pDst = (H265Pu *)arg;

    if (pDst == NULL) {
        VLOG(ERR, "no h265 PredUnit data\n");
        return -1;
    }

    pDst->constrained_intra_pred_flag = param->constIntraPredFlag;
    pDst->strong_intra_smoothing_enabled_flag = param->strongIntraSmoothEnable;

    return 0;
}

int venc_op_set_search_window(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    VencSearchWindow *pSrc = (VencSearchWindow *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no search window data\n");
        return -1;
    }

    if (pSrc->u32Ver < 4 || pSrc->u32Ver > 32) {
        VLOG(ERR, "search Ver should be [4,32]\n");
        return -1;
    }

    if (pSrc->u32Hor < 4 || pSrc->u32Hor > 16) {
        VLOG(ERR, "search Hor should be [4,16]\n");
        return -1;
    }

    param->s2SearchRangeXDiv4 = pSrc->u32Ver;
    param->s2SearchRangeYDiv4 = pSrc->u32Hor;

    return 0;
}

int venc_op_get_search_window(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    VencSearchWindow *pDst = (VencSearchWindow *)arg;

    if (pDst == NULL) {
        VLOG(ERR, "no search window data\n");
        return -1;
    }
    //only support manual mode
    pDst->mode = search_mode_manual;
    pDst->u32Ver = param->s2SearchRangeXDiv4;
    pDst->u32Hor = param->s2SearchRangeYDiv4;

    return 0;
}


int venc_op_set_h264_trans(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H264Trans *pSrc = (H264Trans *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no h264 trans data\n");
        return -1;
    }

    param->chromaCbQpOffset = pSrc->chroma_qp_index_offset;
    param->chromaCrQpOffset = pSrc->chroma_qp_index_offset;

    return 0;
}

int venc_op_set_h265_trans(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H265Trans *pSrc = (H265Trans *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no h265 trans data\n");
        return -1;
    }

    param->chromaCbQpOffset = pSrc->cb_qp_offset;
    param->chromaCrQpOffset = pSrc->cr_qp_offset;

    return 0;
}

int venc_op_reg_reconBuf(void *handle, void *arg)
{
    return 0;
}

int venc_op_set_pixelformat(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    InPixelFormat *pInFormat = (InPixelFormat *)arg;
    EncOpenParam *pst_open_param = &pst_handle->open_param;

    pst_open_param->cbcrInterleave = pInFormat->bCbCrInterleave;
    pst_open_param->nv21 = pInFormat->bNV21;

    return 0;
}

int venc_op_show_chninfo(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    struct seq_file *m = (struct seq_file *)arg;

    if (pst_handle) {
        seq_printf(m, "chn num:%d\t sendCnt:%u\t sendOkCnt:%u\t getCnt:%u\t dropCnt:%u\n"
            , pst_handle->channel_index, pst_handle->chn_venc_info.sendAllYuvCnt
            , pst_handle->chn_venc_info.sendOkYuvCnt, pst_handle->chn_venc_info.getStreamCnt
            , pst_handle->chn_venc_info.dropCnt);
    }

    return 0;
}

int venc_op_set_user_rcinfo(void *handle, void *arg)
{
    return 0;
}

int venc_op_set_superframe(void *handle, void *arg)
{
    return 0;
}

int venc_op_set_h264_vui(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    H264Vui *pSrc = (H264Vui *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no h264 vui data\n");
        return -1;
    }

    venc_h264_sps_add_vui(pst_handle, pSrc);
    return 0;
}

int venc_op_set_h265_vui(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    H265Vui *pSrc = (H265Vui *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no h265 vui data\n");
        return -1;
    }

    venc_h265_sps_add_vui(pst_handle, pSrc);
    return 0;
}

int venc_op_set_frame_param(void *handle, void *arg)
{
    return 0;
}

static int venc_op_wait_encode_done(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    int ret = RETCODE_SUCCESS;

    ret = wait_for_completion_timeout(&pst_handle->semEncDoneCmd,
                usecs_to_jiffies(2000 * 1000));
    if (ret == 0) {
        VLOG(ERR, "get stream timeout!\n");
        return -1;
    }

    return 0;
}

int venc_op_set_h264_split(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H264Split *pSrc = (H264Split *)arg;

    param->avcSliceMode = pSrc->bSplitEnable;
    param->avcSliceArg = pSrc->u32MbLineNum * (pst_open_param->picWidth + 15) / 16;
    return 0;
}

int venc_op_set_h265_split(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H265Split *pSrc = (H265Split *)arg;

    param->independSliceMode = pSrc->bSplitEnable;
    param->independSliceModeArg = pSrc->u32LcuLineNum * (pst_open_param->picWidth + 63) / 64;
    return 0;
}

int venc_op_set_h264_dblk(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H264Dblk *pSrc = (H264Dblk *)arg;

    param->disableDeblk = pSrc->disable_deblocking_filter_idc;
    param->lfCrossSliceBoundaryEnable = 1;
    param->betaOffsetDiv2 = pSrc->slice_beta_offset_div2;
    param->tcOffsetDiv2 = pSrc->slice_alpha_c0_offset_div2;

    return 0;
}

int venc_op_set_h265_dblk(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H265Dblk *pSrc = (H265Dblk *)arg;

    param->disableDeblk = pSrc->slice_deblocking_filter_disabled_flag;
    param->lfCrossSliceBoundaryEnable = 1;
    param->betaOffsetDiv2 = pSrc->slice_beta_offset_div2;
    param->tcOffsetDiv2 = pSrc->slice_tc_offset_div2;

    return 0;
}

int venc_op_set_h264_intrapred(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H264IntraPred *pSrc = (H264IntraPred *)arg;

    param->constIntraPredFlag = pSrc->constrained_intra_pred_flag;
    return 0;
}

int venc_op_set_custommap(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    WaveCustomMapOpt *pCustomOpt = (WaveCustomMapOpt *)arg;
    drv_enc_param *pEncParam = &pst_handle->enc_param;

    memcpy(&pEncParam->customMapOpt, pCustomOpt, sizeof(WaveCustomMapOpt));
    pEncParam->isCustomMapFlag = TRUE;
    pEncParam->edge_api_enable = TRUE;      //when use venc_op_set_custommap, must set edge_api_enable is true, and venc_op_set_roi is bypassed
    return 0;
}

int venc_op_get_initialinfo(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    VencIntialInfo *pEncInitialInfo = (VencIntialInfo *)arg;
    Uint32 bufsize = VPU_ALIGN4096(pst_handle->open_param.bitstreamBufferSize);

    pst_handle->bs_buf_size = pst_handle->min_src_frame_count * bufsize;
    pEncInitialInfo->min_num_rec_fb = pst_handle->min_recon_frame_count;
    pEncInitialInfo->min_num_src_fb = pst_handle->min_src_frame_count;
    pEncInitialInfo->min_bs_buf_size = pst_handle->bs_buf_size;

    return 0;
}

int venc_op_set_h265_sao(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    H265Sao *pSrc = (H265Sao *)arg;
    int h265SaoEnable = 0;

    if (pSrc == NULL) {
        VLOG(ERR, "no h265 sao data\n");
        return -1;
    }

    if (pSrc->slice_sao_luma_flag == 0 && pSrc->slice_sao_chroma_flag == 0)
        h265SaoEnable = 0;
    else
        h265SaoEnable = 1;

    param->saoEnable = h265SaoEnable;
    return 0;
}

int venc_op_set_bindmode(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    int *pSrc = (int *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "invalid param\n");
        return -1;
    }

    pst_handle->is_bind_mode = *pSrc;
    VLOG(INFO, "mode:%d\n", pst_handle->is_bind_mode);
    return 0;
}

int venc_op_set_extern_bs_buf(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    VencExternBuf *pst_bs_buf = (VencExternBuf *)arg;

    // sanity check
    if (pst_bs_buf == NULL || pst_bs_buf->bs_buf_size == 0 || pst_bs_buf->bs_phys_addr == 0) {
        VLOG(ERR, "invalid param 1 \n");
        return -1;
    }

    if (pst_bs_buf->bs_buf_size < pst_handle->bs_buf_size) {
        VLOG(ERR, "invalid param 2, input bs size:%d, need bs size:%d\n"
            , pst_bs_buf->bs_buf_size, pst_handle->bs_buf_size);
        return -1;
    }

    pst_handle->extern_bs_addr = pst_bs_buf->bs_phys_addr;
    pst_handle->use_extern_bs_buf = TRUE;

    VLOG(INFO, "bs addr:0x%lx\n", pst_handle->extern_bs_addr);
    return 0;
}

int venc_op_get_bs_packs_num(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    int32_t *packs_cnt = (int32_t *)arg;

    *packs_cnt = Queue_Get_Cnt(pst_handle->stream_packs);

    if (*packs_cnt == 0 && pst_handle->ouput_end == 1) {
        *packs_cnt = -1;    // indicate bitstream end
    }

    return 0;
}

typedef struct _DRV_VENC_IOCTL_OP_ {
    int opNum;
    int (*ioctlFunc)(void *handle, void *arg);
} DRV_VENC_IOCTL_OP;

DRV_VENC_IOCTL_OP IoctlOp[] = {
    { DRV_H26X_OP_NONE, NULL },
    { DRV_H26X_OP_SET_RC_PARAM, venc_op_set_rc_param },
    { DRV_H26X_OP_START, venc_op_start },
    { DRV_H26X_OP_GET_FD, venc_op_get_fd },
    { DRV_H26X_OP_SET_REQUEST_IDR, venc_op_set_request_idr },
    { DRV_H26X_OP_SET_ENABLE_IDR, venc_op_set_enable_idr },
    { DRV_H26X_OP_SET_CHN_ATTR, venc_op_set_chn_attr },
    { DRV_H26X_OP_SET_REF_PARAM, venc_op_set_ref },
    { DRV_H26X_OP_SET_ROI_PARAM, venc_op_set_roi },
    { DRV_H26X_OP_GET_ROI_PARAM, venc_op_get_roi },
    { DRV_H26X_OP_SET_FRAME_LOST_STRATEGY, venc_op_set_framelost },
    { DRV_H26X_OP_GET_VB_INFO, venc_op_get_vbinfo },
    { DRV_H26X_OP_SET_VB_BUFFER, venc_op_set_vbbuffer },
    { DRV_H26X_OP_SET_USER_DATA, venc_op_encode_userdata },
    { DRV_H26X_OP_SET_PREDICT, venc_op_set_pred },
    { DRV_H26X_OP_SET_H264_ENTROPY, venc_op_set_h264_entropy },
    { DRV_H26X_OP_SET_H264_TRANS, venc_op_set_h264_trans },
    { DRV_H26X_OP_SET_H265_TRANS, venc_op_set_h265_trans },
    { DRV_H26X_OP_REG_VB_BUFFER, venc_op_reg_reconBuf },
    { DRV_H26X_OP_SET_IN_PIXEL_FORMAT, venc_op_set_pixelformat },
    { DRV_H26X_OP_GET_CHN_INFO, venc_op_show_chninfo },
    { DRV_H26X_OP_SET_USER_RC_INFO, venc_op_set_user_rcinfo },
    { DRV_H26X_OP_SET_SUPER_FRAME, venc_op_set_superframe },
    { DRV_H26X_OP_SET_H264_VUI, venc_op_set_h264_vui },
    { DRV_H26X_OP_SET_H265_VUI, venc_op_set_h265_vui },
    { DRV_H26X_OP_SET_FRAME_PARAM, venc_op_set_frame_param },
    { DRV_H26X_OP_CALC_FRAME_PARAM, NULL },
    { DRV_H26X_OP_SET_SB_MODE, NULL },
    { DRV_H26X_OP_START_SB_MODE, NULL },
    { DRV_H26X_OP_UPDATE_SB_WPTR, NULL },
    { DRV_H26X_OP_RESET_SB, NULL },
    { DRV_H26X_OP_SB_EN_DUMMY_PUSH, NULL },
    { DRV_H26X_OP_SB_TRIG_DUMMY_PUSH, NULL },
    { DRV_H26X_OP_SB_DIS_DUMMY_PUSH, NULL },
    { DRV_H26X_OP_SB_GET_SKIP_FRM_STATUS, NULL },
    { DRV_H26X_OP_SET_SBM_ENABLE, NULL },
    { DRV_H26X_OP_WAIT_FRAME_DONE, venc_op_wait_encode_done },
    { DRV_H26X_OP_SET_H264_SPLIT, venc_op_set_h264_split },
    { DRV_H26X_OP_SET_H265_SPLIT, venc_op_set_h265_split },
    { DRV_H26X_OP_SET_H264_DBLK, venc_op_set_h264_dblk },
    { DRV_H26X_OP_SET_H265_DBLK, venc_op_set_h265_dblk },
    { DRV_H26X_OP_SET_H264_INTRA_PRED, venc_op_set_h264_intrapred },
    { DRV_H26X_OP_SET_CUSTOM_MAP, venc_op_set_custommap },
    { DRV_H26X_OP_GET_INITIAL_INFO, venc_op_get_initialinfo },
    { DRV_H26X_OP_SET_H265_SAO, venc_op_set_h265_sao},
    { DRV_H26X_OP_GET_ENCODE_HEADER, venc_get_encode_header},
    { DRV_H26X_OP_SET_BIND_MODE, venc_op_set_bindmode},
    { DRV_H26X_OP_SET_H265_PRED_UNIT, venc_op_set_h265_predunit},
    { DRV_H26X_OP_GET_H265_PRED_UNIT, venc_op_get_h265_predunit},
    { DRV_H26X_OP_SET_SEARCH_WINDOW, venc_op_set_search_window},
    { DRV_H26X_OP_GET_SEARCH_WINDOW, venc_op_get_search_window},
    { DRV_H26X_OP_SET_EXTERN_BS_BUF, venc_op_set_extern_bs_buf},
    { DRV_H26X_OP_GET_BS_PACKS_NUM, venc_op_get_bs_packs_num},
};

int internal_venc_ioctl(void *handle, int op, void *arg)
{
    int ret = 0;
    int currOp;

    if (op <= 0 || op >= DRV_H26X_OP_MAX) {
        VLOG(ERR, "op = %d\n", op);
        return -1;
    }

    currOp = (IoctlOp[op].opNum & DRV_H26X_OP_MASK) >> DRV_H26X_OP_SHIFT;
    if (op != currOp) {
        VLOG(ERR, "op = %d\n", op);
        return -1;
    }

    if (IoctlOp[op].ioctlFunc)
        ret = IoctlOp[op].ioctlFunc(handle, arg);

    return ret;
}


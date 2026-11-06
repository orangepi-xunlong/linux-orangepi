#include "drv_venc.h"
#include <linux/comm_venc.h>
#include <linux/defines.h>
#include "venc.h"
#include "bind.h"
#include "module_common.h"
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <uapi/linux/sched/types.h>

#include "vc_drv_proc.h"
#include "venc_rc.h"


venc_context *handle;

extern wait_queue_head_t tVencWaitQueue[];

#ifdef USE_vb_pool
extern int32_t vb_create_pool(struct vb_pool_cfg *config);
extern int32_t vb_get_config(struct vb_cfg *pstVbConfig);
extern vb_blk vb_get_block_with_id(vb_pool poolId, uint32_t blk_size,
                   MOD_ID_E modId);
extern uint64_t vb_handle2PhysAddr(vb_blk blk);
extern vb_blk vb_physAddr2Handle(uint64_t u64PhyAddr);
extern int32_t vb_release_block(vb_blk blk);
#endif

static int _drv_process_result(venc_chn_context *pChnHandle,
                        venc_stream_s *pstStream);

venc_vb_ctx	vencVbCtx[VENC_MAX_CHN_NUM];

static inline unsigned int _drv_get_num_packs(payload_type_e enType)
{
    return (enType == PT_JPEG || enType == PT_MJPEG) ? 2 : MAX_NUM_PACKS;
}

static inline int _drv_check_common_rcparam_helper(
    unsigned int u32MaxIprop, unsigned int u32MinIprop,
    unsigned int u32MaxIQp, unsigned int u32MinIQp,
    unsigned int u32MaxQp, unsigned int u32MinQp)
{
    int s32Ret = 0;

    s32Ret = ((u32MaxIprop >= DRV_H26X_MAX_I_PROP_MIN) &&
        (u32MaxIprop <= DRV_H26X_MAX_I_PROP_MAX)) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32MaxIprop", u32MaxIprop);
        return s32Ret;
    }

    s32Ret = ((u32MinIprop >= DRV_H26X_MIN_I_PROP_MIN) &&
        (u32MinIprop <= DRV_H26X_MAX_I_PROP_MAX)) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32MinIprop", u32MinIprop);
        return s32Ret;
    }

    s32Ret = (u32MaxIQp <= 51) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32MaxIQp", u32MaxIQp);
        return s32Ret;
    }

    s32Ret = (u32MinIQp <= 51) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32MinIQp", u32MinIQp);
        return s32Ret;
    }

    s32Ret = (u32MaxQp <= 51) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32MaxQp", u32MaxQp);
        return s32Ret;
    }

    s32Ret = (u32MinQp <= 51) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32MinQp", u32MinQp);
        return s32Ret;
    }

    return s32Ret;
}

static inline int _drv_check_common_rcparam(
    const venc_rc_param_s * pstRcParam, venc_attr_s *pVencAttr, venc_rc_mode_e enRcMode)
{
    int s32Ret;

    s32Ret = (pstRcParam->u32RowQpDelta <= DRV_H26X_ROW_QP_DELTA_MAX) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32RowQpDelta", pstRcParam->u32RowQpDelta);
        return s32Ret;
    }

    if (pVencAttr->enType != PT_H265 ||
        pstRcParam->s32FirstFrameStartQp != 63) {
        s32Ret = ((pstRcParam->s32FirstFrameStartQp >= 0) &&
            (pstRcParam->s32FirstFrameStartQp <= 51)) ? 0 : DRV_ERR_VENC_RC_PARAM;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "s32FirstFrameStartQp", pstRcParam->s32FirstFrameStartQp);
            return s32Ret;
        }
    }

    s32Ret = (pstRcParam->u32ThrdLv <= DRV_H26X_THRDLV_MAX) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32ThrdLv", pstRcParam->u32ThrdLv);
        return s32Ret;
    }

    s32Ret = ((pstRcParam->s32BgDeltaQp >= DRV_H26X_BG_DELTA_QP_MIN) &&
        (pstRcParam->s32BgDeltaQp <= DRV_H26X_BG_DELTA_QP_MAX)) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "s32BgDeltaQp", pstRcParam->s32BgDeltaQp);
        return s32Ret;
    }

    switch (enRcMode) {
    case VENC_RC_MODE_H264CBR:
        {
            const venc_param_h264_cbr_s *pprc = &pstRcParam->stParamH264Cbr;

            s32Ret = _drv_check_common_rcparam_helper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H264VBR:
        {
            const venc_param_h264_vbr_s *pprc = &pstRcParam->stParamH264Vbr;

            s32Ret = _drv_check_common_rcparam_helper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H264AVBR:
        {
            const venc_param_h264_avbr_s *pprc = &pstRcParam->stParamH264AVbr;

            s32Ret = _drv_check_common_rcparam_helper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H264UBR:
        {
            const venc_param_h264_ubr_s *pprc = &pstRcParam->stParamH264Ubr;

            s32Ret = _drv_check_common_rcparam_helper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H265CBR:
        {
            const venc_param_h265_cbr_s *pprc = &pstRcParam->stParamH265Cbr;

            s32Ret = _drv_check_common_rcparam_helper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H265VBR:
        {
            const venc_param_h265_vbr_s *pprc = &pstRcParam->stParamH265Vbr;

            s32Ret = _drv_check_common_rcparam_helper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H265AVBR:
        {
            const venc_param_h265_avbr_s *pprc = &pstRcParam->stParamH265AVbr;

            s32Ret = _drv_check_common_rcparam_helper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H265UBR:
        {
            const venc_param_h265_ubr_s *pprc = &pstRcParam->stParamH265Ubr;

            s32Ret = _drv_check_common_rcparam_helper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    default:
        break;
    }

    return s32Ret;
}

void *drv_venc_get_share_mem(void)
{
    return NULL;
}

static uint64_t _drv_get_current_time(void)
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


static int _drv_venc_recv_qbuf(mmf_chn_s chn, vb_blk blk)
{
    int ret;
    venc_chn VencChn = chn.chn_id;

    // sanity check
    if (VencChn >= VC_MAX_CHN_NUM) {
        DRV_VENC_ERR("invalid venc chn\n");
        return -1;
    }

    ret = vb_qbuf(chn, CHN_TYPE_IN, &vencVbCtx[VencChn].vb_jobs, blk);

    return ret;
}

static int _drv_wait_encode_done(venc_chn_context *pChnHandle)
{
    int ret = 0;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    if (pEncCtx->base.ioctl) {
        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265 ||
            pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
            ret = pEncCtx->base.ioctl(pEncCtx,
                DRV_H26X_OP_WAIT_FRAME_DONE, NULL);
            if (ret != 0) {
                MUTEX_UNLOCK(&pChnHandle->chnMutex);
                DRV_VENC_ERR("DRV_H26X_OP_WAIT_FRAME_DONE, %d\n", ret);
                return -1;
            }
        } else {
            // for jpeg, always return success
            ret = 0;
        }
    }
    MUTEX_UNLOCK(&pChnHandle->chnMutex);
    if (ret != 0)
        DRV_VENC_ERR("failed to wait done %d", ret);

    return ret;
}

static int _venc_event_handler(void *data)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)data;
    venc_chn VencChn;
    venc_chn_vars *pChnVars = NULL;
    venc_chn_status_s stStat;
    venc_enc_ctx *pEncCtx;
    int ret;
    int s32MilliSec = -1;
    mmf_chn_s chn = { .mod_id = ID_VENC,
                .dev_id = 0,
                .chn_id = 0 };
    vb_blk blk;
    struct vb_s *vb;
    video_frame_info_s stVFrame;
    video_frame_s *pstVFrame = &stVFrame.video_frame;
    int i;
    int vi_cnt = 0;
    unsigned char bIsoSendFrmEn;
#ifdef DUMP_BIND_YUV
    FILE *out_f = fopen("out.265", "wb");
#endif
    int s32SetFrameMilliSec = 20000;
    int s32Ret = 0;
    venc_vb_ctx *pVbCtx = NULL;

    memset(&stVFrame, 0, sizeof(video_frame_info_s));

    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;
    pVbCtx = pChnHandle->pVbCtx;
    VencChn = pChnHandle->VeChn;

    chn.chn_id = VencChn;
    bIsoSendFrmEn = pChnHandle->pChnAttr->stVencAttr.bIsoSendFrmEn;
    DRV_VENC_DBG("venc_event_handler bIsoSendFrmEn:%d\n", bIsoSendFrmEn);
    memset(&stVFrame, 0, sizeof(video_frame_info_s));

    if (SEMA_WAIT(&pChnVars->sem_send) != 0) {
        DRV_VENC_ERR("can not down sem_send\n");
        return -1;
    }
    pstVFrame->width = pChnHandle->pChnAttr->stVencAttr.u32PicWidth;
    pstVFrame->height = pChnHandle->pChnAttr->stVencAttr.u32PicHeight;
    DRV_VENC_DBG("[%d] venc_chn_STATE_START_ENC\n", VencChn);

    while (!kthread_should_stop() && pChnHandle->bChnEnable) {
        DRV_VENC_DBG("[%d]\n", VencChn);
        // if (IF_WANNA_DISABLE_BIND_MODE() ||
        //     (pChnVars->s32RecvPicNum > 0 &&
        //      vi_cnt >= pChnVars->s32RecvPicNum)) {
        // 	pChnHandle->bChnEnable = 0;
        // 	DRV_VENC_SYNC("end\n");
        // 	break;
        // }

        DRV_VENC_DBG("h26x_handle chn:%d wait.\n", VencChn);

        while (((ret = SEMA_TIMEWAIT(&pVbCtx->vb_jobs.sem, usecs_to_jiffies(1000 * 1000))) != 0)) {
            if (pChnHandle->bChnEnable == 0)
                break;

            continue;
        }
        if (pChnHandle->bChnEnable == 0) {
            DRV_VENC_DBG("end\n");
            break;
        }

        if (ret == -1)
            continue;

        if (pVbCtx->pause) {
            DRV_VENC_DBG("pause and skip update.\n");
            continue;
        }

        // get venc input buf.
        DRV_VENC_DBG("h26x_handle wait jobs chn:%d\n", VencChn);
        blk = base_mod_jobs_waitq_pop(&pVbCtx->vb_jobs);
        if (blk == VB_INVALID_HANDLE) {
            DRV_VENC_DBG("No more vb for dequeue.\n");
            continue;
        }
        vb = (struct vb_s *)blk;

        atomic_long_fetch_and(~BIT(ID_VENC), &vb->mod_ids);
        for (i = 0; i < 3; i++) {
            pstVFrame->phyaddr[i] = vb->buf.phy_addr[i];
            pstVFrame->stride[i] = vb->buf.stride[i];
            DRV_VENC_DBG("phy: 0x%llx, stride: 0x%x\n",
                        pstVFrame->phyaddr[i],
                        pstVFrame->stride[i]);
        }
        pstVFrame->pts = vb->buf.pts;
        pstVFrame->pixel_format = vb->buf.pixel_format;
        pstVFrame->private_data = vb;
        DRV_VENC_DBG("h26x_handle chn:%d send.\n", VencChn);
        s32SetFrameMilliSec = 20000;

        DRV_VENC_DBG("[%d]\n", VencChn);
        s32Ret = drv_venc_send_frame(VencChn, &stVFrame,
                        s32SetFrameMilliSec);
        if (s32Ret == DRV_ERR_VENC_INIT ||
            s32Ret == DRV_ERR_VENC_FRC_NO_ENC ||
            s32Ret == DRV_ERR_VENC_BUSY) {
            DRV_VENC_INFO("no encode,continue\n");
            vb_done_handler(chn, CHN_TYPE_IN, &pVbCtx->vb_jobs, blk);
            continue;
        } else if (s32Ret != 0) {
            DRV_VENC_ERR(
                "drv_venc_send_frame, VencChn = %d, s32Ret = %d\n",
                VencChn, s32Ret);
            goto VENC_EVENT_HANDLER_ERR;
        }

        // check bIsoSendFrmEn for sync getstream or async getstream
        if (bIsoSendFrmEn == 0) {
            s32Ret = drv_venc_query_status(VencChn, &stStat);
            if (s32Ret != 0) {
                DRV_VENC_ERR(
                    "drv_venc_query_status, VencChn = %d, s32Ret = %d\n",
                    VencChn, s32Ret);
                goto VENC_EVENT_HANDLER_ERR;
            }
            if (!stStat.u32CurPacks) {
                DRV_VENC_ERR("u32CurPacks = 0\n");
                s32Ret = DRV_ERR_VENC_EMPTY_PACK;
                goto VENC_EVENT_HANDLER_ERR;
            }
            pChnVars->stStream.pstPack = (venc_pack_s *)MEM_MALLOC(
                sizeof(venc_pack_s) * stStat.u32CurPacks);
            if (pChnVars->stStream.pstPack == NULL) {
                DRV_VENC_ERR("malloc memory failed!\n");
                s32Ret = DRV_ERR_VENC_NOMEM;
                goto VENC_EVENT_HANDLER_ERR;
            }

            pChnVars->stStream.u32PackCount = stStat.u32CurPacks;
            s32Ret = pEncCtx->base.getStream(pEncCtx, &pChnVars->stStream,
                            s32MilliSec);
            pChnVars->s32BindModeGetStreamRet = s32Ret;
            if (s32Ret != 0) {
                DRV_VENC_ERR("getStream, VencChn = %d, s32Ret = %d\n",
                        VencChn, s32Ret);
                SEMA_POST(&pChnVars->sem_send);
                goto VENC_EVENT_HANDLER_ERR;
            }

            vb_done_handler(chn, CHN_TYPE_IN, &pVbCtx->vb_jobs, blk);
            s32Ret = _drv_process_result(pChnHandle, &pChnVars->stStream);
            if (s32Ret != 0) {
                DRV_VENC_ERR("(chn %d) _drv_process_result fail\n",
                        VencChn);
                SEMA_POST(&pChnVars->sem_send);
                goto VENC_EVENT_HANDLER_ERR_2;
            }

            SEMA_POST(&pChnVars->sem_send);

            DRV_VENC_INFO("[%d]wait release\n", vi_cnt);
            if (SEMA_WAIT(&pChnVars->sem_release) != 0) {
                DRV_VENC_ERR("can not down sem_release\n");
                goto VENC_EVENT_HANDLER_ERR_2;
            }

            s32Ret = pEncCtx->base.releaseStream(pEncCtx,
                                &pChnVars->stStream);
            if (s32Ret != 0) {
                DRV_VENC_ERR("[Vench %d]releaseStream ,s32Ret = %d\n",
                        VencChn, s32Ret);
                goto VENC_EVENT_HANDLER_ERR_2;
            }

            MEM_FREE(pChnVars->stStream.pstPack);
            pChnVars->stStream.pstPack = NULL;
        } else {
            _drv_wait_encode_done(pChnHandle);
            vb_done_handler(chn, CHN_TYPE_IN, &pVbCtx->vb_jobs, blk);
        }

        vi_cnt++;
        cond_resched();
    }

    DRV_VENC_INFO("------------end\n");
    #ifdef DUMP_BIND_YUV
    fclose(out_f);
    #endif
    return 0;

VENC_EVENT_HANDLER_ERR:
    vb_done_handler(chn, CHN_TYPE_IN, &pVbCtx->vb_jobs, blk);

VENC_EVENT_HANDLER_ERR_2:
    DRV_VENC_INFO("end\n");

#ifdef DUMP_BIND_YUV
    fclose(out_f);
#endif

    return 0;
}

static int _check_venc_chn_handle(venc_chn VeChn)
{
    if (handle == NULL) {
        DRV_VENC_ERR("Call VENC Destroy before Create, failed\n");
        return DRV_ERR_VENC_UNEXIST;
    }

    if (handle->chn_handle[VeChn] == NULL) {
        DRV_VENC_ERR("VENC Chn #%d haven't created !\n", VeChn);
        return DRV_ERR_VENC_INVALID_CHNID;
    }

    return 0;
}

static void _drv_venc_init_mod_param(drv_venc_param_mod_s *pModParam)
{
    if (!pModParam)
        return;

    memset(pModParam, 0, sizeof(*pModParam));
    pModParam->stJpegeModParam.JpegMarkerOrder[0] = JPEGE_MARKER_SOI;
    pModParam->stJpegeModParam.JpegMarkerOrder[1] =
        JPEGE_MARKER_FRAME_INDEX;
    pModParam->stJpegeModParam.JpegMarkerOrder[2] = JPEGE_MARKER_USER_DATA;
    pModParam->stJpegeModParam.JpegMarkerOrder[3] = JPEGE_MARKER_DRI_OPT;
    pModParam->stJpegeModParam.JpegMarkerOrder[4] = JPEGE_MARKER_DQT;
    pModParam->stJpegeModParam.JpegMarkerOrder[5] = JPEGE_MARKER_DHT;
    pModParam->stJpegeModParam.JpegMarkerOrder[6] = JPEGE_MARKER_SOF0;
    pModParam->stJpegeModParam.JpegMarkerOrder[7] = JPEGE_MARKER_BUTT;
    pModParam->stH264eModParam.u32UserDataMaxLen = USERDATA_MAX_DEFAULT;
    pModParam->stH265eModParam.u32UserDataMaxLen = USERDATA_MAX_DEFAULT;
}

static int _drv_venc_init_ctx_handle(void)
{
    venc_context *pVencHandle;

    if (handle == NULL) {
        handle = MEM_CALLOC(1, sizeof(venc_context));
        if (handle == NULL) {
            DRV_VENC_ERR("venc_context create failure\n");
            return DRV_ERR_VENC_NOMEM;
        }

        pVencHandle = (venc_context *)handle;
        _drv_venc_init_mod_param(&pVencHandle->ModParam);
    }

    return 0;
}


int drv_venc_init(void)
{
    int s32Ret = 0;
    base_register_recv_cb(ID_VENC, _drv_venc_recv_qbuf);

    s32Ret = _drv_venc_init_ctx_handle();
    if (s32Ret != 0) {
        DRV_VENC_ERR("venc_context\n");
        s32Ret = DRV_ERR_VENC_NOMEM;
    }
    return s32Ret;
}

void drv_venc_deinit(void)
{
    base_unregister_recv_cb(ID_VENC);
    if(handle)
        MEM_FREE(handle);
    handle = NULL;
    return;
}

static int _drv_init_frc_softfloat(venc_chn_context *pChnHandle,
        unsigned int u32SrcFrameRate, DRV_FR32 fr32DstFrameRate)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;
    int s32Ret = 0;
    int dstFrDenom;
    int dstFrfract;
    float32 srcFrameRate;
    float32 dstFrameRate;
    int srcFrChecker, dstFrChecker;

    dstFrDenom = (fr32DstFrameRate >> 16) & 0xFFFF;
    dstFrfract = (fr32DstFrameRate & 0xFFFF);

    dstFrameRate = INT_TO_FLOAT(dstFrfract);
    if (dstFrDenom != 0)
        dstFrameRate = FLOAT_DIV(dstFrameRate, INT_TO_FLOAT(dstFrDenom));

    dstFrDenom = (u32SrcFrameRate >> 16) & 0xFFFF;
    dstFrfract = (u32SrcFrameRate & 0xFFFF);

    srcFrameRate = INT_TO_FLOAT(dstFrfract);
    if (dstFrDenom != 0)
        srcFrameRate = FLOAT_DIV(srcFrameRate, INT_TO_FLOAT(dstFrDenom));

    srcFrChecker = FLOAT_TO_INT(FLOAT_MUL(srcFrameRate, FLOAT_VAL_10000));
    dstFrChecker = FLOAT_TO_INT(FLOAT_MUL(dstFrameRate, FLOAT_VAL_10000));

    if (srcFrChecker > dstFrChecker) {
        if (FLOAT_EQ(srcFrameRate, FLOAT_VAL_0) ||
            FLOAT_EQ(dstFrameRate, FLOAT_VAL_0)) {
            DRV_VENC_ERR(
                "Dst frame rate(%d), Src frame rate(%d), not supported\n",
                dstFrameRate, srcFrameRate);
            pvf->bFrcEnable = 0;
            s32Ret = DRV_ERR_VENC_NOT_SUPPORT;
            return s32Ret;
        }
        pvf->bFrcEnable = 1;
        pvf->srcFrameDur =
            FLOAT_TO_INT(FLOAT_DIV(FLOAT_VAL_FRC_TIME_SCALE, srcFrameRate));
        pvf->dstFrameDur =
            FLOAT_TO_INT(FLOAT_DIV(FLOAT_VAL_FRC_TIME_SCALE, dstFrameRate));
        pvf->srcTs = pvf->srcFrameDur;
        pvf->dstTs = pvf->dstFrameDur;
    } else if (srcFrChecker == dstFrChecker) {
        pvf->bFrcEnable = 0;
    } else {
        pvf->bFrcEnable = 0;
        DRV_VENC_ERR(
            "Dst frame rate(%d) > Src frame rate(%d), not supported\n",
            dstFrameRate, srcFrameRate);
        s32Ret = DRV_ERR_VENC_NOT_SUPPORT;
        return s32Ret;
    }

    return s32Ret;
}


static int _drv_init_frc(venc_chn_context *pChnHandle,
        unsigned int u32SrcFrameRate, DRV_FR32 fr32DstFrameRate)
{
    int s32Ret = 0;

    s32Ret = _drv_init_frc_softfloat(pChnHandle, u32SrcFrameRate, fr32DstFrameRate);

    return s32Ret;
}


static int _drv_check_framerate(venc_chn_context *pChnHandle,
                  unsigned int *pu32SrcFrameRate,
                  DRV_FR32 *pfr32DstFrameRate,
                  unsigned char bVariFpsEn)
{
    int s32Ret = 0;

    int frameRateDiv, frameRateRes;
    // TODO: use soft-floating to replace it
    int fSrcFrmrate, fDstFrmrate;

    s32Ret = (bVariFpsEn <= DRV_VARI_FPS_EN_MAX) ?
        0 : DRV_ERR_VENC_ILLEGAL_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "bVariFpsEn", bVariFpsEn);
        return s32Ret;
    }

    if (*pfr32DstFrameRate == 0) {
        DRV_VENC_WARN("set fr32DstFrameRate to %d\n",
                  DEST_FRAMERATE_DEF);
        *pfr32DstFrameRate = DEST_FRAMERATE_DEF;
    }

    if (*pu32SrcFrameRate == 0) {
        DRV_VENC_WARN("set u32SrcFrameRate to %d\n",
                  *pfr32DstFrameRate);
        *pu32SrcFrameRate = *pfr32DstFrameRate;
    }

    frameRateDiv = (*pfr32DstFrameRate >> 16);
    frameRateRes = *pfr32DstFrameRate & 0xFFFF;

    if (frameRateDiv == 0)
        fDstFrmrate = frameRateRes;
    else
        fDstFrmrate = frameRateRes / frameRateDiv;

    frameRateDiv = (*pu32SrcFrameRate >> 16);
    frameRateRes = *pu32SrcFrameRate & 0xFFFF;

    if (frameRateDiv == 0)
        fSrcFrmrate = frameRateRes;
    else
        fSrcFrmrate = frameRateRes / frameRateDiv;

    if (fDstFrmrate > DEST_FRAMERATE_MAX ||
        fDstFrmrate < DEST_FRAMERATE_MIN) {
        DRV_VENC_ERR("fr32DstFrameRate = 0x%X, not support\n",
                 *pfr32DstFrameRate);
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    if (fSrcFrmrate > SRC_FRAMERATE_MAX ||
        fSrcFrmrate < SRC_FRAMERATE_MIN) {
        DRV_VENC_ERR("u32SrcFrameRate = %d, not support\n",
                 *pu32SrcFrameRate);
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    if (fDstFrmrate > fSrcFrmrate) {
        *pfr32DstFrameRate = *pu32SrcFrameRate;
        DRV_VENC_WARN("fDstFrmrate > fSrcFrmrate\n");
        DRV_VENC_WARN("=> fr32DstFrameRate = u32SrcFrameRate\n");
    }

    s32Ret = _drv_init_frc(pChnHandle, *pu32SrcFrameRate, *pfr32DstFrameRate);

    return s32Ret;
}

static int _drv_venc_reg_vbbuf(venc_chn VeChn)
{
    int s32Ret = 0;

    venc_context *pVencHandle = handle;
    venc_enc_ctx *pEncCtx = NULL;

    venc_chn_context *pChnHandle = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    if (pVencHandle == NULL) {
        DRV_VENC_ERR(
            "p_venctx_handle NULL (Channel not create yet..)\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_REG_VB_BUFFER,
                         NULL);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_H26X_OP_REG_VB_BUFFER, %d\n", s32Ret);
            return s32Ret;
        }
    }

    return s32Ret;
}

static int _drv_venc_set_pixelformat(venc_chn VeChn,
                    unsigned char bCbCrInterleave,
                    unsigned char bNV21)
{
    int s32Ret = 0;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_context *pChnHandle = NULL;
    InPixelFormat inPixelFormat;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;

    inPixelFormat.bCbCrInterleave = bCbCrInterleave;
    inPixelFormat.bNV21 = bNV21;

    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_IN_PIXEL_FORMAT,
                     (void *)&inPixelFormat);

    return s32Ret;
}

static int _drv_check_rcmode_attr(venc_chn_context *pChnHandle)
{
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    int s32Ret = 0;

    if (pChnAttr->stVencAttr.enType == PT_MJPEG) {
        if (pChnAttr->stRcAttr.enRcMode == VENC_RC_MODE_MJPEGCBR) {
            venc_mjpeg_cbr_s *pstMjpegeCbr =
                &pChnAttr->stRcAttr.stMjpegCbr;

            s32Ret =  _drv_check_framerate(
                pChnHandle, &pstMjpegeCbr->u32SrcFrameRate,
                &pstMjpegeCbr->fr32DstFrameRate,
                pstMjpegeCbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_MJPEGFIXQP) {
            venc_mjpeg_fixqp_s *pstMjpegeFixQp =
                &pChnAttr->stRcAttr.stMjpegFixQp;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstMjpegeFixQp->u32SrcFrameRate,
                &pstMjpegeFixQp->fr32DstFrameRate,
                pstMjpegeFixQp->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
            VENC_RC_MODE_MJPEGVBR) {
            venc_mjpeg_vbr_s *pstMjpegeVbr =
                &pChnAttr->stRcAttr.stMjpegVbr;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstMjpegeVbr->u32SrcFrameRate,
                &pstMjpegeVbr->fr32DstFrameRate,
                pstMjpegeVbr->bVariFpsEn);
        } else {
            s32Ret = DRV_ERR_VENC_NOT_SUPPORT;
            DRV_VENC_ERR("enRcMode = %d, not support\n",
                     pChnAttr->stRcAttr.enRcMode);
        }
    } else if (pChnAttr->stVencAttr.enType == PT_H264) {
        if (pChnAttr->stRcAttr.enRcMode == VENC_RC_MODE_H264CBR) {
            venc_h264_cbr_s *pstH264Cbr =
                &pChnAttr->stRcAttr.stH264Cbr;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH264Cbr->u32SrcFrameRate,
                &pstH264Cbr->fr32DstFrameRate,
                pstH264Cbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H264VBR) {
            venc_h264_vbr_s *pstH264Vbr =
                &pChnAttr->stRcAttr.stH264Vbr;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH264Vbr->u32SrcFrameRate,
                &pstH264Vbr->fr32DstFrameRate,
                pstH264Vbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H264AVBR) {
            venc_h264_avbr_s *pstH264AVbr =
                &pChnAttr->stRcAttr.stH264AVbr;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH264AVbr->u32SrcFrameRate,
                &pstH264AVbr->fr32DstFrameRate,
                pstH264AVbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H264FIXQP) {
            venc_h264_fixqp_s *pstH264FixQp =
                &pChnAttr->stRcAttr.stH264FixQp;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH264FixQp->u32SrcFrameRate,
                &pstH264FixQp->fr32DstFrameRate,
                pstH264FixQp->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H264UBR) {
            venc_h264_ubr_s *pstH264Ubr =
                &pChnAttr->stRcAttr.stH264Ubr;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH264Ubr->u32SrcFrameRate,
                &pstH264Ubr->fr32DstFrameRate,
                pstH264Ubr->bVariFpsEn);
        } else {
            s32Ret = DRV_ERR_VENC_NOT_SUPPORT;
            DRV_VENC_ERR("enRcMode = %d, not support\n",
                     pChnAttr->stRcAttr.enRcMode);
        }
    } else if (pChnAttr->stVencAttr.enType == PT_H265) {
        if (pChnAttr->stRcAttr.enRcMode == VENC_RC_MODE_H265CBR) {
            VENC_H265_CBR_S *pstH265Cbr =
                &pChnAttr->stRcAttr.stH265Cbr;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH265Cbr->u32SrcFrameRate,
                &pstH265Cbr->fr32DstFrameRate,
                pstH265Cbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265VBR) {
            VENC_H265_VBR_S *pstH265Vbr =
                &pChnAttr->stRcAttr.stH265Vbr;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH265Vbr->u32SrcFrameRate,
                &pstH265Vbr->fr32DstFrameRate,
                pstH265Vbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265AVBR) {
            VENC_H265_AVBR_S *pstH265AVbr =
                &pChnAttr->stRcAttr.stH265AVbr;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH265AVbr->u32SrcFrameRate,
                &pstH265AVbr->fr32DstFrameRate,
                pstH265AVbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265FIXQP) {
            VENC_H265_FIXQP_S *pstH265FixQp =
                &pChnAttr->stRcAttr.stH265FixQp;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH265FixQp->u32SrcFrameRate,
                &pstH265FixQp->fr32DstFrameRate,
                pstH265FixQp->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265QPMAP) {
            venc_h265_qpmap_s *pstH265QpMap =
                &pChnAttr->stRcAttr.stH265QpMap;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH265QpMap->u32SrcFrameRate,
                &pstH265QpMap->fr32DstFrameRate,
                pstH265QpMap->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265UBR) {
            VENC_H265_UBR_S *pstH265Ubr =
                &pChnAttr->stRcAttr.stH265Ubr;

            s32Ret = _drv_check_framerate(
                pChnHandle, &pstH265Ubr->u32SrcFrameRate,
                &pstH265Ubr->fr32DstFrameRate,
                pstH265Ubr->bVariFpsEn);
        } else {
            s32Ret = DRV_ERR_VENC_NOT_SUPPORT;
            DRV_VENC_ERR("enRcMode = %d, not support\n",
                     pChnAttr->stRcAttr.enRcMode);
        }
    }

    return s32Ret;
}

static unsigned int _drv_venc_get_gop(const venc_rc_attr_s *pRcAttr)
{
    switch (pRcAttr->enRcMode) {
    case VENC_RC_MODE_H264CBR:
        return pRcAttr->stH264Cbr.u32Gop;
    case VENC_RC_MODE_H264VBR:
        return pRcAttr->stH264Vbr.u32Gop;
    case VENC_RC_MODE_H264AVBR:
        return pRcAttr->stH264AVbr.u32Gop;
    case VENC_RC_MODE_H264QVBR:
        return pRcAttr->stH264QVbr.u32Gop;
    case VENC_RC_MODE_H264FIXQP:
        return pRcAttr->stH264FixQp.u32Gop;
    case VENC_RC_MODE_H264QPMAP:
        return pRcAttr->stH264QpMap.u32Gop;
    case VENC_RC_MODE_H264UBR:
        return pRcAttr->stH264Ubr.u32Gop;
    case VENC_RC_MODE_H265CBR:
        return pRcAttr->stH265Cbr.u32Gop;
    case VENC_RC_MODE_H265VBR:
        return pRcAttr->stH265Vbr.u32Gop;
    case VENC_RC_MODE_H265AVBR:
        return pRcAttr->stH265AVbr.u32Gop;
    case VENC_RC_MODE_H265QVBR:
        return pRcAttr->stH265QVbr.u32Gop;
    case VENC_RC_MODE_H265FIXQP:
        return pRcAttr->stH265FixQp.u32Gop;
    case VENC_RC_MODE_H265QPMAP:
        return pRcAttr->stH265QpMap.u32Gop;
    case VENC_RC_MODE_H265UBR:
        return pRcAttr->stH265Ubr.u32Gop;
    default:
        return 0;
    }
}

static int _drv_check_gop_attr(venc_chn_context *pChnHandle)
{
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *pRcAttr = &pChnAttr->stRcAttr;
    venc_gop_attr_s *pGopAttr = &pChnAttr->stGopAttr;
    unsigned int u32Gop = _drv_venc_get_gop(pRcAttr);
    int s32Ret;

    if (pChnAttr->stVencAttr.enType == PT_JPEG ||
        pChnAttr->stVencAttr.enType == PT_MJPEG) {
        return 0;
    }

    if (u32Gop == 0) {
        DRV_VENC_ERR("enRcMode = %d, not support\n", pRcAttr->enRcMode);
        return DRV_ERR_VENC_RC_PARAM;
    }

    switch (pGopAttr->enGopMode) {
    case VENC_GOPMODE_NORMALP:
        s32Ret = ((pGopAttr->stNormalP.s32IPQpDelta >= DRV_H26X_NORMALP_IP_QP_DELTA_MIN) &&
            (pGopAttr->stNormalP.s32IPQpDelta <= DRV_H26X_NORMALP_IP_QP_DELTA_MAX)) ?
            0 : DRV_ERR_VENC_GOP_ATTR;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "s32IPQpDelta", pGopAttr->stNormalP.s32IPQpDelta);
            return s32Ret;
        }
        break;

    case VENC_GOPMODE_SMARTP:
        s32Ret = ((pGopAttr->stSmartP.u32BgInterval >= u32Gop) &&
            (pGopAttr->stSmartP.u32BgInterval <= DRV_H26X_SMARTP_BG_INTERVAL_MAX)) ?
            0 : DRV_ERR_VENC_GOP_ATTR;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "u32BgInterval", pGopAttr->stNormalP.s32IPQpDelta);
            return s32Ret;
        }
        s32Ret = ((pGopAttr->stSmartP.s32BgQpDelta >= DRV_H26X_SMARTP_BG_QP_DELTA_MIN) &&
            (pGopAttr->stSmartP.s32BgQpDelta <= DRV_H26X_SMARTP_BG_QP_DELTA_MAX)) ?
            0 : DRV_ERR_VENC_GOP_ATTR;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "s32BgQpDelta", pGopAttr->stNormalP.s32IPQpDelta);
            return s32Ret;
        }
        s32Ret = ((pGopAttr->stSmartP.s32ViQpDelta >= DRV_H26X_SMARTP_VI_QP_DELTA_MIN) &&
            (pGopAttr->stSmartP.s32ViQpDelta <= DRV_H26X_SMARTP_VI_QP_DELTA_MAX)) ?
            0 : DRV_ERR_VENC_GOP_ATTR;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "s32ViQpDelta", pGopAttr->stSmartP.s32ViQpDelta);
            return s32Ret;
        }
        if ((pGopAttr->stSmartP.u32BgInterval % u32Gop) != 0) {
            DRV_VENC_ERR(
                "u32BgInterval %d, not a multiple of u32Gop %d\n",
                pGopAttr->stAdvSmartP.u32BgInterval, u32Gop);
            return DRV_ERR_VENC_GOP_ATTR;
        }
        break;

    default:
        DRV_VENC_ERR("enGopMode = %d, not support\n",
                 pGopAttr->enGopMode);
        return DRV_ERR_VENC_GOP_ATTR;
    }

    return 0;
}

static int _drv_check_gop_exattr(venc_chn_context *pChnHandle)
{
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_gop_ex_attr_s *pGopExAttr = &pChnAttr->stGopExAttr;

    if (pChnAttr->stVencAttr.enType == PT_JPEG ||
        pChnAttr->stVencAttr.enType == PT_MJPEG) {
        return 0;
    }

    if (pGopExAttr->u32GopPreset > GOP_PRESET_IDX_IPP_SINGLE) {
        DRV_VENC_ERR("invalid goppreset:%d\n", pGopExAttr->u32GopPreset);
        return DRV_ERR_VENC_GOP_ATTR;
    }

    if (pGopExAttr->u32GopPreset == GOP_PRESET_IDX_CUSTOM_GOP
        && (pGopExAttr->gopParam.s32CustomGopSize < 1 || pGopExAttr->gopParam.s32CustomGopSize > 8)) {
        DRV_VENC_ERR("invalid gopparam size:%d when goppreset is 0\n", pGopExAttr->u32GopPreset);
        return DRV_ERR_VENC_GOP_ATTR;
    }

    return 0;
}

static int _drv_set_default_rcparam(venc_chn_context *pChnHandle)
{
    int s32Ret = 0;
    venc_chn_attr_s *pChnAttr;
    venc_attr_s *pVencAttr;
    venc_rc_attr_s *prcatt;
    venc_rc_param_s *prcparam;

    pChnAttr = pChnHandle->pChnAttr;
    pVencAttr = &pChnAttr->stVencAttr;
    prcatt = &pChnAttr->stRcAttr;
    prcparam = &pChnHandle->rcParam;

    prcparam->u32RowQpDelta = DRV_H26X_ROW_QP_DELTA_DEFAULT;
    prcparam->s32FirstFrameStartQp =
        ((pVencAttr->enType == PT_H265) ? 63 : DEF_IQP);
    prcparam->s32InitialDelay = DRV_INITIAL_DELAY_DEFAULT;
    prcparam->u32ThrdLv = DRV_H26X_THRDLV_DEFAULT;
    prcparam->bBgEnhanceEn = DRV_H26X_BG_ENHANCE_EN_DEFAULT;
    prcparam->s32BgDeltaQp = DRV_H26X_BG_DELTA_QP_DEFAULT;

    if (pVencAttr->enType == PT_H264) {
        if (prcatt->enRcMode == VENC_RC_MODE_H264CBR) {
            venc_param_h264_cbr_s *pstParamH264Cbr =
                &prcparam->stParamH264Cbr;

            SET_DEFAULT_RC_PARAM(pstParamH264Cbr);
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264VBR) {
            venc_param_h264_vbr_s *pstParamH264Vbr =
                &prcparam->stParamH264Vbr;

            SET_DEFAULT_RC_PARAM(pstParamH264Vbr);
            pstParamH264Vbr->s32ChangePos =
                DRV_H26X_CHANGE_POS_DEFAULT;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264AVBR) {
            venc_param_h264_avbr_s *pstParamH264AVbr =
                &prcparam->stParamH264AVbr;

            SET_DEFAULT_RC_PARAM(pstParamH264AVbr);
            pstParamH264AVbr->s32ChangePos =
                DRV_H26X_CHANGE_POS_DEFAULT;
            pstParamH264AVbr->s32MinStillPercent =
                DRV_H26X_MIN_STILL_PERCENT_DEFAULT;
            pstParamH264AVbr->u32MaxStillQP =
                DRV_H26X_MAX_STILL_QP_DEFAULT;
            pstParamH264AVbr->u32MotionSensitivity =
                DRV_H26X_MOTION_SENSITIVITY_DEFAULT;
            pstParamH264AVbr->s32AvbrFrmLostOpen =
                DRV_H26X_AVBR_FRM_LOST_OPEN_DEFAULT;
            pstParamH264AVbr->s32AvbrFrmGap =
                DRV_H26X_AVBR_FRM_GAP_DEFAULT;
            pstParamH264AVbr->s32AvbrPureStillThr =
                DRV_H26X_AVBR_PURE_STILL_THR_DEFAULT;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264UBR) {
            venc_param_h264_ubr_s *pstParamH264Ubr =
                &prcparam->stParamH264Ubr;

            SET_DEFAULT_RC_PARAM(pstParamH264Ubr);
        }
    } else if (pVencAttr->enType == PT_H265) {
        if (prcatt->enRcMode == VENC_RC_MODE_H265CBR) {
            venc_param_h265_cbr_s *pstParamH265Cbr =
                &prcparam->stParamH265Cbr;

            SET_DEFAULT_RC_PARAM(pstParamH265Cbr);
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265VBR) {
            venc_param_h265_vbr_s *pstParamH265Vbr =
                &prcparam->stParamH265Vbr;

            SET_DEFAULT_RC_PARAM(pstParamH265Vbr);
            pstParamH265Vbr->s32ChangePos =
                DRV_H26X_CHANGE_POS_DEFAULT;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265AVBR) {
            venc_param_h265_avbr_s *pstParamH265AVbr =
                &prcparam->stParamH265AVbr;

            SET_DEFAULT_RC_PARAM(pstParamH265AVbr);
            pstParamH265AVbr->s32ChangePos =
                DRV_H26X_CHANGE_POS_DEFAULT;
            pstParamH265AVbr->s32MinStillPercent =
                DRV_H26X_MIN_STILL_PERCENT_DEFAULT;
            pstParamH265AVbr->u32MaxStillQP =
                DRV_H26X_MAX_STILL_QP_DEFAULT;
            pstParamH265AVbr->u32MotionSensitivity =
                DRV_H26X_MOTION_SENSITIVITY_DEFAULT;
            pstParamH265AVbr->s32AvbrFrmLostOpen =
                DRV_H26X_AVBR_FRM_LOST_OPEN_DEFAULT;
            pstParamH265AVbr->s32AvbrFrmGap =
                DRV_H26X_AVBR_FRM_GAP_DEFAULT;
            pstParamH265AVbr->s32AvbrPureStillThr =
                DRV_H26X_AVBR_PURE_STILL_THR_DEFAULT;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265QPMAP) {
            venc_param_h265_cbr_s *pstParamH265Cbr =
                &prcparam->stParamH265Cbr;

            // When using QP Map, we use CBR as basic setting
            SET_DEFAULT_RC_PARAM(pstParamH265Cbr);
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265UBR) {
            venc_param_h265_ubr_s *pstParamH265Ubr =
                &prcparam->stParamH265Ubr;

            SET_DEFAULT_RC_PARAM(pstParamH265Ubr);
        }
    }  else if (pVencAttr->enType == PT_MJPEG) {
        if (prcatt->enRcMode == VENC_RC_MODE_MJPEGCBR) {
            venc_param_mjpeg_cbr_s *pstParamMjpegCbr = &prcparam->stParamMjpegCbr;

            pstParamMjpegCbr->u32MaxQfactor = Q_TABLE_MAX;
            pstParamMjpegCbr->u32MinQfactor = Q_TABLE_MIN;
        } else if (prcatt->enRcMode == VENC_RC_MODE_MJPEGVBR) {
            venc_param_mjpeg_vbr_s *pstParamMjpegVbr = &prcparam->stParamMjpegVbr;

            pstParamMjpegVbr->s32ChangePos = 100;
            pstParamMjpegVbr->u32MaxQfactor = Q_TABLE_MAX;
            pstParamMjpegVbr->u32MinQfactor = Q_TABLE_MIN;
        }
    }

    return s32Ret;
}

static void _drv_init_vfps(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_vfps *pVfps = &pChnVars->vfps;

    memset(pVfps, 0, sizeof(venc_vfps));
    pVfps->u64StatTime = DRV_DEF_VFPFS_STAT_TIME * 1000 * 1000;
}


static int _drv_set_chn_default(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_attr_s *pVencAttr = &pChnAttr->stVencAttr;
    venc_jpeg_param_s *pvjp = &pChnVars->stJpegParam;
    venc_cu_prediction_s *pcup = &pChnVars->cuPrediction;
    venc_superframe_cfg_s *pcsf = &pChnVars->stSuperFrmParam;
    venc_frame_param_s *pfp = &pChnVars->frameParam;
    int s32Ret = 0;
    mmf_chn_s chn = { .mod_id = ID_VENC,
              .dev_id = 0,
              .chn_id = pChnHandle->VeChn };

    if (pVencAttr->enType == PT_H264) {
        pChnHandle->h264Vui.stVuiAspectRatio.aspect_ratio_idc =
            DRV_H26X_ASPECT_RATIO_IDC_DEFAULT;
        pChnHandle->h264Vui.stVuiAspectRatio.sar_width =
            DRV_H26X_SAR_WIDTH_DEFAULT;
        pChnHandle->h264Vui.stVuiAspectRatio.sar_height =
            DRV_H26X_SAR_HEIGHT_DEFAULT;
        pChnHandle->h264Vui.stVuiTimeInfo.num_units_in_tick =
            DRV_H26X_NUM_UNITS_IN_TICK_DEFAULT;
        pChnHandle->h264Vui.stVuiTimeInfo.time_scale =
            DRV_H26X_TIME_SCALE_DEFAULT;
        pChnHandle->h264Vui.stVuiVideoSignal.video_format =
            DRV_H26X_VIDEO_FORMAT_DEFAULT;
        pChnHandle->h264Vui.stVuiVideoSignal.colour_primaries =
            DRV_H26X_COLOUR_PRIMARIES_DEFAULT;
        pChnHandle->h264Vui.stVuiVideoSignal.transfer_characteristics =
            DRV_H26X_TRANSFER_CHARACTERISTICS_DEFAULT;
        pChnHandle->h264Vui.stVuiVideoSignal.matrix_coefficients =
            DRV_H26X_MATRIX_COEFFICIENTS_DEFAULT;
    } else if (pVencAttr->enType == PT_H265) {
        pChnHandle->h265Vui.stVuiAspectRatio.aspect_ratio_idc =
            DRV_H26X_ASPECT_RATIO_IDC_DEFAULT;
        pChnHandle->h265Vui.stVuiAspectRatio.sar_width =
            DRV_H26X_SAR_WIDTH_DEFAULT;
        pChnHandle->h265Vui.stVuiAspectRatio.sar_height =
            DRV_H26X_SAR_HEIGHT_DEFAULT;
        pChnHandle->h265Vui.stVuiTimeInfo.num_units_in_tick =
            DRV_H26X_NUM_UNITS_IN_TICK_DEFAULT;
        pChnHandle->h265Vui.stVuiTimeInfo.time_scale =
            DRV_H26X_TIME_SCALE_DEFAULT;
        pChnHandle->h265Vui.stVuiTimeInfo.num_ticks_poc_diff_one_minus1 =
            DRV_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_DEFAULT;
        pChnHandle->h265Vui.stVuiVideoSignal.video_format =
            DRV_H26X_VIDEO_FORMAT_DEFAULT;
        pChnHandle->h265Vui.stVuiVideoSignal.colour_primaries =
            DRV_H26X_COLOUR_PRIMARIES_DEFAULT;
        pChnHandle->h265Vui.stVuiVideoSignal.transfer_characteristics =
            DRV_H26X_TRANSFER_CHARACTERISTICS_DEFAULT;
        pChnHandle->h265Vui.stVuiVideoSignal.matrix_coefficients =
            DRV_H26X_MATRIX_COEFFICIENTS_DEFAULT;
    }

    if (pVencAttr->enType == PT_H264) {
        if (pVencAttr->u32Profile == H264E_PROFILE_BASELINE) {
            pChnHandle->h264Entropy.u32EntropyEncModeI =
                H264E_ENTROPY_CAVLC;
            pChnHandle->h264Entropy.u32EntropyEncModeP =
                H264E_ENTROPY_CAVLC;
        } else {
            pChnHandle->h264Entropy.u32EntropyEncModeI =
                H264E_ENTROPY_CABAC;
            pChnHandle->h264Entropy.u32EntropyEncModeP =
                H264E_ENTROPY_CABAC;
        }
    }

    pvjp->u32Qfactor = Q_TABLE_DEFAULT;
    pcup->u32IntraCost = DRV_H26X_INTRACOST_DEFAULT;
    pcsf->enSuperFrmMode = DRV_H26X_SUPER_FRM_MODE_DEFAULT;
    pcsf->u32SuperIFrmBitsThr = DRV_H26X_SUPER_I_BITS_THR_DEFAULT;
    pcsf->u32SuperPFrmBitsThr = DRV_H26X_SUPER_P_BITS_THR_DEFAULT;
    pfp->u32FrameQp = DRV_H26X_FRAME_QP_DEFAULT;
    pfp->u32FrameBits = DRV_H26X_FRAME_BITS_DEFAULT;

    s32Ret = _drv_set_default_rcparam(pChnHandle);
    if (s32Ret < 0) {
        DRV_VENC_ERR("drv_set_default_rcparam\n");
        return s32Ret;
    }

    pChnVars->vbpool.hPicInfoVbPool = VB_INVALID_POOLID;
    chn.chn_id = pChnHandle->VeChn;
    base_mod_jobs_init(&vencVbCtx[pChnHandle->VeChn].vb_jobs, 1, 1, 0);
    SEMA_INIT(&pChnVars->sem_send, 0, 0);
    SEMA_INIT(&pChnVars->sem_release, 0, 0);

    _drv_init_vfps(pChnHandle);

    return s32Ret;
}

static int _drv_set_rcparam_to_drv(venc_chn_context *pChnHandle)
{
    int s32Ret = 0;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    venc_rc_param_s *prcparam = &pChnHandle->rcParam;
    venc_attr_s *pVencAttr = &pChnAttr->stVencAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;

    if (pEncCtx->base.ioctl) {
        RcParam rcp, *prcp = &rcp;
        jpeg_enc_rc_param  jrcp, *pjrcp = &jrcp;

        memset(prcp, 0, sizeof(RcParam));
        memset(pjrcp, 0, sizeof(jpeg_enc_rc_param));

        prcp->u32RowQpDelta = prcparam->u32RowQpDelta;
        prcp->firstFrmstartQp = prcparam->s32FirstFrameStartQp;
        prcp->u32ThrdLv = prcparam->u32ThrdLv;
        prcp->s32InitialDelay = prcparam->s32InitialDelay;
        prcp->s32ChangePos = 0;

        if (pVencAttr->enType == PT_H264) {
            if (prcatt->enRcMode == VENC_RC_MODE_H264CBR) {
                venc_param_h264_cbr_s *pstParamH264Cbr =
                    &prcparam->stParamH264Cbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH264Cbr);
            } else if (prcatt->enRcMode == VENC_RC_MODE_H264VBR) {
                venc_param_h264_vbr_s *pstParamH264Vbr =
                    &prcparam->stParamH264Vbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH264Vbr);
                prcp->s32ChangePos =
                    pstParamH264Vbr->s32ChangePos;
            } else if (prcatt->enRcMode == VENC_RC_MODE_H264AVBR) {
                venc_param_h264_avbr_s *pstParamH264AVbr =
                    &prcparam->stParamH264AVbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH264AVbr);
                prcp->s32ChangePos =
                    pstParamH264AVbr->s32ChangePos;
                prcp->s32MinStillPercent =
                    pstParamH264AVbr->s32MinStillPercent;
                prcp->u32MaxStillQP =
                    pstParamH264AVbr->u32MaxStillQP;
                prcp->u32MotionSensitivity =
                    pstParamH264AVbr->u32MotionSensitivity;
                prcp->s32AvbrFrmLostOpen =
                    pstParamH264AVbr->s32AvbrFrmLostOpen;
                prcp->s32AvbrFrmGap =
                    pstParamH264AVbr->s32AvbrFrmGap;
                prcp->s32AvbrPureStillThr =
                    pstParamH264AVbr->s32AvbrPureStillThr;

            } else if (prcatt->enRcMode == VENC_RC_MODE_H264UBR) {
                venc_param_h264_ubr_s *pstParamH264Ubr =
                    &prcparam->stParamH264Ubr;

                SET_COMMON_RC_PARAM(prcp, pstParamH264Ubr);
            }
        } else if (pVencAttr->enType == PT_H265) {
            if (prcatt->enRcMode == VENC_RC_MODE_H265CBR) {
                venc_param_h265_cbr_s *pstParamH265Cbr =
                    &prcparam->stParamH265Cbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH265Cbr);
            } else if (prcatt->enRcMode == VENC_RC_MODE_H265VBR) {
                venc_param_h265_vbr_s *pstParamH265Vbr =
                    &prcparam->stParamH265Vbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH265Vbr);
                prcp->s32ChangePos =
                    pstParamH265Vbr->s32ChangePos;
            } else if (prcatt->enRcMode == VENC_RC_MODE_H265AVBR) {
                venc_param_h265_avbr_s *pstParamH265AVbr =
                    &prcparam->stParamH265AVbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH265AVbr);
                prcp->s32ChangePos =
                    pstParamH265AVbr->s32ChangePos;
                prcp->s32MinStillPercent =
                    pstParamH265AVbr->s32MinStillPercent;
                prcp->u32MaxStillQP =
                    pstParamH265AVbr->u32MaxStillQP;
                prcp->u32MotionSensitivity =
                    pstParamH265AVbr->u32MotionSensitivity;
                prcp->s32AvbrFrmLostOpen =
                    pstParamH265AVbr->s32AvbrFrmLostOpen;
                prcp->s32AvbrFrmGap =
                    pstParamH265AVbr->s32AvbrFrmGap;
                prcp->s32AvbrPureStillThr =
                    pstParamH265AVbr->s32AvbrPureStillThr;
            } else if (prcatt->enRcMode == VENC_RC_MODE_H265QPMAP) {
                venc_param_h265_cbr_s *pstParamH265Cbr =
                    &prcparam->stParamH265Cbr;

                // When using QP Map, we use CBR as basic setting
                SET_COMMON_RC_PARAM(prcp, pstParamH265Cbr);
            } else if (prcatt->enRcMode == VENC_RC_MODE_H265UBR) {
                venc_param_h265_ubr_s *pstParamH265Ubr =
                    &prcparam->stParamH265Ubr;

                SET_COMMON_RC_PARAM(prcp, pstParamH265Ubr);
            }
        } else if (pVencAttr->enType == PT_MJPEG) {
            if (prcatt->enRcMode == VENC_RC_MODE_MJPEGCBR) {
                venc_param_mjpeg_cbr_s *pstParamMjpegCbr =
                    &prcparam->stParamMjpegCbr;
                pjrcp->chg_pos = 100;
                pjrcp->max_qfactor = pstParamMjpegCbr->u32MaxQfactor;
                pjrcp->min_qfactor = pstParamMjpegCbr->u32MinQfactor;
            } else if (prcatt->enRcMode == VENC_RC_MODE_MJPEGVBR) {
                venc_param_mjpeg_vbr_s *pstParamMjpegVbr =
                    &prcparam->stParamMjpegVbr;
                pjrcp->chg_pos = pstParamMjpegVbr->s32ChangePos;
                pjrcp->max_qfactor = pstParamMjpegVbr->u32MaxQfactor;
                pjrcp->min_qfactor = pstParamMjpegVbr->u32MinQfactor;
            }
        }

        if (pVencAttr->enType == PT_H264 || pVencAttr->enType == PT_H265)
            s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_RC_PARAM, (void *)prcp);
        else if (pVencAttr->enType == PT_MJPEG)
            s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_JPEG_OP_SET_RC_PARAM, (void *)pjrcp);

        if (s32Ret != 0) {
            DRV_VENC_ERR("OP_SET_RC_PARAM, %d\n", s32Ret);
            return s32Ret;
        }
    } else {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    return s32Ret;
}

static int _drv_h26x_trans_chn_attr(venc_chn_attr_s *pInChnAttr,
                 VidChnAttr *pOutAttr)
{
    int s32Ret = -1;
    venc_rc_attr_s *pstRcAttr = NULL;

    if ((pInChnAttr == NULL) || (pOutAttr == NULL))
        return s32Ret;

    pstRcAttr = &pInChnAttr->stRcAttr;

    if (pInChnAttr->stVencAttr.enType == PT_H264) {
        if (pstRcAttr->enRcMode == VENC_RC_MODE_H264CBR) {
            pOutAttr->u32BitRate = pstRcAttr->stH264Cbr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH264Cbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH264Cbr.fr32DstFrameRate;
            s32Ret = 0;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H264VBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stH264Vbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH264Vbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH264Vbr.fr32DstFrameRate;
            s32Ret = 0;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H264AVBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stH264AVbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH264AVbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH264AVbr.fr32DstFrameRate;
            s32Ret = 0;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H264UBR) {
            pOutAttr->u32BitRate = pstRcAttr->stH264Ubr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH264Ubr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH264Ubr.fr32DstFrameRate;
            s32Ret = 0;
        }
    } else if (pInChnAttr->stVencAttr.enType == PT_H265) {
        if (pstRcAttr->enRcMode == VENC_RC_MODE_H265CBR) {
            pOutAttr->u32BitRate = pstRcAttr->stH265Cbr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH265Cbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH265Cbr.fr32DstFrameRate;
            s32Ret = 0;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H265VBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stH265Vbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH265Vbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH265Vbr.fr32DstFrameRate;
            s32Ret = 0;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H265AVBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stH265AVbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH265AVbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH265AVbr.fr32DstFrameRate;
            s32Ret = 0;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H265UBR) {
            pOutAttr->u32BitRate = pstRcAttr->stH265Ubr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH265Ubr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH265Ubr.fr32DstFrameRate;
            s32Ret = 0;
        }
    }

    return s32Ret;
}

static int _drv_jpg_trans_chn_attr(venc_chn_attr_s *pInChnAttr,
                jpeg_chn_attr *pOutAttr)
{
    int s32Ret = -1;
    venc_rc_attr_s *pstRcAttr = NULL;
    venc_attr_s *pstVencAttr = NULL;

    if ((pInChnAttr == NULL) || (pOutAttr == NULL))
        return s32Ret;

    pstRcAttr = &(pInChnAttr->stRcAttr);
    pstVencAttr = &(pInChnAttr->stVencAttr);

    if (pInChnAttr->stVencAttr.enType == PT_MJPEG) {
        pOutAttr->picWidth = pstVencAttr->u32PicWidth;
        pOutAttr->picHeight = pstVencAttr->u32PicHeight;
        if (pstRcAttr->enRcMode == VENC_RC_MODE_MJPEGCBR) {
            pOutAttr->u32BitRate = pstRcAttr->stMjpegCbr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stMjpegCbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stMjpegCbr.fr32DstFrameRate;
            s32Ret = 0;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_MJPEGVBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stMjpegVbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stMjpegVbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stMjpegVbr.fr32DstFrameRate;
            s32Ret = 0;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stMjpegFixQp.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stMjpegFixQp.fr32DstFrameRate;
            s32Ret = 0;
        }
    } else if (pInChnAttr->stVencAttr.enType == PT_JPEG) {
        pOutAttr->picWidth = pstVencAttr->u32PicWidth;
        pOutAttr->picHeight = pstVencAttr->u32PicHeight;
        s32Ret = 0;
    }

    return s32Ret;
}

static int
_drv_venc_check_jpege_format(const venc_mod_jpege_s *pJpegeModParam)
{
    int i;
    unsigned int marker_cnt[JPEGE_MARKER_BUTT];
    const jpege_marker_type_e *p = pJpegeModParam->JpegMarkerOrder;

    switch (pJpegeModParam->enJpegeFormat) {
    case JPEGE_FORMAT_DEFAULT:
    case JPEGE_FORMAT_TYPE_1:
        return 0;
    case JPEGE_FORMAT_CUSTOM:
        // proceed to check marker order validity
        break;
    default:
        DRV_VENC_ERR("Unknown JPEG format %d\n",
                 pJpegeModParam->enJpegeFormat);
        return DRV_ERR_VENC_JPEG_MARKER_ORDER;
    }

    memset(marker_cnt, 0, sizeof(marker_cnt));

    if (p[0] != JPEGE_MARKER_SOI) {
        DRV_VENC_ERR("The first jpeg marker must be SOI\n");
        return DRV_ERR_VENC_JPEG_MARKER_ORDER;
    }

    for (i = 0; i < JPEG_MARKER_ORDER_CNT; i++) {
        int type = p[i];
        if (JPEGE_MARKER_SOI <= type && type < JPEGE_MARKER_BUTT)
            marker_cnt[type] += 1;
        else
            break;
    }

    if (marker_cnt[JPEGE_MARKER_SOI] == 0) {
        DRV_VENC_ERR("There must be one SOI\n");
        return DRV_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_SOF0] == 0) {
        DRV_VENC_ERR("There must be one SOF0\n");
        return DRV_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_DQT] == 0 &&
        marker_cnt[JPEGE_MARKER_DQT_MERGE] == 0) {
        DRV_VENC_ERR("There must be one DQT or DQT_MERGE\n");
        return DRV_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_DQT] > 0 &&
        marker_cnt[JPEGE_MARKER_DQT_MERGE] > 0) {
        DRV_VENC_ERR("DQT and DQT_MERGE are mutually exclusive\n");
        return DRV_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_DHT] > 0 &&
        marker_cnt[JPEGE_MARKER_DHT_MERGE] > 0) {
        DRV_VENC_ERR("DHT and DHT_MERGE are mutually exclusive\n");
        return DRV_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_DRI] > 0 &&
        marker_cnt[JPEGE_MARKER_DRI_OPT] > 0) {
        DRV_VENC_ERR("DRI and DRI_OPT are mutually exclusive\n");
        return DRV_ERR_VENC_JPEG_MARKER_ORDER;
    }

    for (i = JPEGE_MARKER_SOI; i < JPEGE_MARKER_BUTT; i++) {
        if (marker_cnt[i] > 1) {
            DRV_VENC_ERR("Repeating marker type %d present\n", i);
            return DRV_ERR_VENC_JPEG_MARKER_ORDER;
        }
    }

    return 0;
}

static void _drv_venc_config_jpege_format(venc_mod_jpege_s *pJpegeModParam)
{
    switch (pJpegeModParam->enJpegeFormat) {
    case JPEGE_FORMAT_DEFAULT:
        pJpegeModParam->JpegMarkerOrder[0] = JPEGE_MARKER_SOI;
        pJpegeModParam->JpegMarkerOrder[1] = JPEGE_MARKER_FRAME_INDEX;
        pJpegeModParam->JpegMarkerOrder[2] = JPEGE_MARKER_USER_DATA;
        pJpegeModParam->JpegMarkerOrder[3] = JPEGE_MARKER_DRI_OPT;
        pJpegeModParam->JpegMarkerOrder[4] = JPEGE_MARKER_DQT;
        pJpegeModParam->JpegMarkerOrder[5] = JPEGE_MARKER_DHT;
        pJpegeModParam->JpegMarkerOrder[6] = JPEGE_MARKER_SOF0;
        pJpegeModParam->JpegMarkerOrder[7] = JPEGE_MARKER_BUTT;
        return;
    case JPEGE_FORMAT_TYPE_1:
        pJpegeModParam->JpegMarkerOrder[0] = JPEGE_MARKER_SOI;
        pJpegeModParam->JpegMarkerOrder[1] = JPEGE_MARKER_JFIF;
        pJpegeModParam->JpegMarkerOrder[2] = JPEGE_MARKER_DQT_MERGE;
        pJpegeModParam->JpegMarkerOrder[3] = JPEGE_MARKER_SOF0;
        pJpegeModParam->JpegMarkerOrder[4] = JPEGE_MARKER_DHT_MERGE;
        pJpegeModParam->JpegMarkerOrder[5] = JPEGE_MARKER_DRI;
        pJpegeModParam->JpegMarkerOrder[6] = JPEGE_MARKER_BUTT;
        return;
    default:
        return;
    }
}

static void _drv_set_fps(venc_chn_context *pChnHandle, int currFps)
{
    DRV_FR32 fr32DstFrameRate = currFps & 0xFFFF;
    venc_chn_attr_s *pChnAttr;
    venc_rc_attr_s *prcatt;

    pChnAttr = pChnHandle->pChnAttr;
    prcatt = &pChnAttr->stRcAttr;

    if (pChnAttr->stVencAttr.enType == PT_H264) {
        if (prcatt->enRcMode == VENC_RC_MODE_H264CBR) {
            prcatt->stH264Cbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264VBR) {
            prcatt->stH264Vbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264AVBR) {
            prcatt->stH264AVbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264FIXQP) {
            prcatt->stH264FixQp.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264UBR) {
            prcatt->stH264Ubr.fr32DstFrameRate = fr32DstFrameRate;
        }
    } else if (pChnAttr->stVencAttr.enType == PT_H265) {
        if (prcatt->enRcMode == VENC_RC_MODE_H265CBR) {
            prcatt->stH265Cbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265VBR) {
            prcatt->stH265Vbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265AVBR) {
            prcatt->stH265AVbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265FIXQP) {
            prcatt->stH265FixQp.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265UBR) {
            prcatt->stH265Ubr.fr32DstFrameRate = fr32DstFrameRate;
        }
    }
}

static int _drv_check_fps(venc_chn_context *pChnHandle,
               const video_frame_info_s *pstFrame)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_vfps *pVfps = &pChnVars->vfps;
    const video_frame_s *pstVFrame = &pstFrame->video_frame;
    int s32Ret = 0;
    int nextFps;

    if (pVfps->u64prevSec == 0) {
        pVfps->u64prevSec = pstVFrame->pts;
    } else {
        if (pstVFrame->pts - pVfps->u64prevSec >=
            pVfps->u64StatTime) {
            nextFps = pVfps->s32NumFrmsInOneSec /
                  DRV_DEF_VFPFS_STAT_TIME;

            _drv_set_fps(pChnHandle, nextFps);

            pVfps->u64prevSec = pstVFrame->pts;
            pVfps->s32NumFrmsInOneSec = 0;

            s32Ret = DRV_ERR_VENC_STAT_VFPS_CHANGE;
        }
    }
    pVfps->s32NumFrmsInOneSec++;

    return s32Ret;
}


static int _drv_set_chn_attr(venc_chn_context *pChnHandle)
{
    VidChnAttr vidChnAttr = { 0 };
    jpeg_chn_attr jpgChnAttr = { 0 };
    int s32Ret = 0;
    venc_chn_attr_s *pChnAttr;
    venc_attr_s *pVencAttr;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;

    pChnAttr = pChnHandle->pChnAttr;
    pChnVars = pChnHandle->pChnVars;
    pVencAttr = &pChnAttr->stVencAttr;
    pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    if (pVencAttr->enType == PT_H265 || pVencAttr->enType == PT_H264) {
        s32Ret = _drv_h26x_trans_chn_attr(pChnHandle->pChnAttr,
                          &vidChnAttr);
        if (s32Ret != 0) {
            DRV_VENC_ERR("h26x trans_chn_attr fail, %d\n", s32Ret);
            goto CHECK_RC_ATTR_RET;
        }

        s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_CHN_ATTR,
                         (void *)&vidChnAttr);
    } else if (pVencAttr->enType == PT_MJPEG ||
           pVencAttr->enType == PT_JPEG) {
        s32Ret = _drv_jpg_trans_chn_attr(pChnHandle->pChnAttr,
                         &jpgChnAttr);
        if (s32Ret != 0) {
            DRV_VENC_ERR("mjpeg trans_chn_attr fail, %d\n", s32Ret);
            goto CHECK_RC_ATTR_RET;
        }

        s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_JPEG_OP_SET_CHN_ATTR,
                         (void *)&jpgChnAttr);
    }

    if (s32Ret != 0) {
        DRV_VENC_ERR(
            "DRV_H26X_OP_SET_CHN_ATTR or DRV_JPEG_OP_SET_CHN_ATTR, %d\n",
            s32Ret);
        goto CHECK_RC_ATTR_RET;
    }

    s32Ret = _drv_check_rcmode_attr(pChnHandle); // RC side framerate control

CHECK_RC_ATTR_RET:
    pChnVars->bAttrChange = 0;
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret != 0) {
        DRV_VENC_ERR("drv_check_rcmode_attr fail, %d\n", s32Ret);
        return s32Ret;
    }

    s32Ret = _drv_check_gop_attr(pChnHandle);
    if (s32Ret != 0) {
        DRV_VENC_ERR("drv_check_gop_attr fail, %d\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

static int _drv_check_frc(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;
    int ifEncode = 1;

    if (pvf->bFrcEnable) {
        if (pvf->srcTs < pvf->dstTs) {
            ifEncode = 0;
        }
    }

    return ifEncode;
}

static int _drv_update_frc_dst(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;

    if (pvf->bFrcEnable) {
        if (pvf->srcTs >= pvf->dstTs) {
            pvf->dstTs += pvf->dstFrameDur;
        }
    }

    return pvf->bFrcEnable;
}

static int _drv_update_frc_src(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;

    if (pvf->bFrcEnable) {
        pvf->srcTs += pvf->srcFrameDur;
    }

    return pvf->bFrcEnable;
}

static void _drv_check_frc_overflow(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;

    if (pvf->srcTs >= FRC_TIME_OVERFLOW_OFFSET &&
        pvf->dstTs >= FRC_TIME_OVERFLOW_OFFSET) {
        pvf->srcTs -= FRC_TIME_OVERFLOW_OFFSET;
        pvf->dstTs -= FRC_TIME_OVERFLOW_OFFSET;
    }
}


static inline int _drv_check_vui_aspectratio(
    const venc_vui_aspect_ratio_s * pstVuiAspectRatio, int s32Err) {
    int s32Ret = 0;

    s32Ret = (pstVuiAspectRatio->aspect_ratio_info_present_flag <= 1) ? 0 : s32Err;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "aspect_ratio_info_present_flag",
            pstVuiAspectRatio->aspect_ratio_info_present_flag);
        return s32Ret;
    }

    if (pstVuiAspectRatio->aspect_ratio_info_present_flag) {
        s32Ret = (pstVuiAspectRatio->sar_width >= DRV_H26X_SAR_WIDTH_MIN) ? 0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n", "sar_width", pstVuiAspectRatio->sar_width);
            return s32Ret;
        }

        s32Ret = (pstVuiAspectRatio->sar_height >= DRV_H26X_SAR_HEIGHT_MIN) ? 0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n", "sar_height", pstVuiAspectRatio->sar_height);
            return s32Ret;
        }
    }

    s32Ret = (pstVuiAspectRatio->overscan_info_present_flag <= 1) ? 0 : s32Err;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "overscan_info_present_flag",
            pstVuiAspectRatio->overscan_info_present_flag);
        return s32Ret;
    }

    if (pstVuiAspectRatio->overscan_info_present_flag) {
        s32Ret = (pstVuiAspectRatio->overscan_appropriate_flag <= 1) ? 0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "overscan_appropriate_flag",
                pstVuiAspectRatio->overscan_appropriate_flag);
            return s32Ret;
        }
    }

    return s32Ret;
}

static inline int _drv_check_vui_h264_timinginfo(
    const venc_vui_h264_time_info_s * pstVuiH264TimingInfo, int s32Err) {
    int s32Ret = 0;

    DRV_VENC_DBG(
        "timing_info_present_flag %u, fixed_frame_rate_flag %u\n",
        pstVuiH264TimingInfo->timing_info_present_flag,
        pstVuiH264TimingInfo->fixed_frame_rate_flag);
    DRV_VENC_DBG("num_units_in_tick %u, time_scale %u\n",
        pstVuiH264TimingInfo->num_units_in_tick,
        pstVuiH264TimingInfo->time_scale);

    if (pstVuiH264TimingInfo->timing_info_present_flag) {
        s32Ret = (pstVuiH264TimingInfo->fixed_frame_rate_flag <= 1) ? 0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "fixed_frame_rate_flag",
                pstVuiH264TimingInfo->fixed_frame_rate_flag);
            return s32Ret;
        }

        s32Ret = ((pstVuiH264TimingInfo->num_units_in_tick >= DRV_H26X_NUM_UNITS_IN_TICK_MIN) &&
            (pstVuiH264TimingInfo->num_units_in_tick <= DRV_H26X_NUM_UNITS_IN_TICK_MAX)) ?
            0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n", "num_units_in_tick", pstVuiH264TimingInfo->num_units_in_tick);
            return s32Ret;
        }

        s32Ret = ((pstVuiH264TimingInfo->time_scale >= DRV_H26X_TIME_SCALE_MIN) &&
            (pstVuiH264TimingInfo->time_scale <= DRV_H26X_TIME_SCALE_MAX)) ? 0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n", "time_scale", pstVuiH264TimingInfo->time_scale);
            return s32Ret;
        }
    }

    return s32Ret;
}

static inline int _drv_check_vui_h265_timinginfo(
    const venc_vui_h265_time_info_s * pstVuiH265TimingInfo, int s32Err) {
    int s32Ret = 0;

    DRV_VENC_DBG(
        "timing_info_present_flag %u, num_units_in_tick %u, time_scale %u\n",
        pstVuiH265TimingInfo->timing_info_present_flag,
        pstVuiH265TimingInfo->num_units_in_tick,
        pstVuiH265TimingInfo->time_scale);
    DRV_VENC_DBG("num_ticks_poc_diff_one_minus1 %u\n",
             pstVuiH265TimingInfo->num_ticks_poc_diff_one_minus1);

    s32Ret = (pstVuiH265TimingInfo->timing_info_present_flag <= 1) ? 0 : s32Err;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "timing_info_present_flag",
            pstVuiH265TimingInfo->timing_info_present_flag);
        return s32Ret;
    }

    if (pstVuiH265TimingInfo->timing_info_present_flag) {
        s32Ret = ((pstVuiH265TimingInfo->num_units_in_tick >= DRV_H26X_NUM_UNITS_IN_TICK_MIN) &&
            (pstVuiH265TimingInfo->num_units_in_tick <= DRV_H26X_NUM_UNITS_IN_TICK_MAX)) ?
            0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n", "num_units_in_tick", pstVuiH265TimingInfo->num_units_in_tick);
            return s32Ret;
        }

        s32Ret = ((pstVuiH265TimingInfo->time_scale >= DRV_H26X_TIME_SCALE_MIN) &&
            (pstVuiH265TimingInfo->time_scale <= DRV_H26X_TIME_SCALE_MAX)) ? 0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n", "time_scale", pstVuiH265TimingInfo->time_scale);
            return s32Ret;
        }

        s32Ret = (pstVuiH265TimingInfo->num_ticks_poc_diff_one_minus1
            <= DRV_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_MAX) ? 0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "num_ticks_poc_diff_one_minus1",
                pstVuiH265TimingInfo->num_ticks_poc_diff_one_minus1);
            return s32Ret;
        }
    }

    return s32Ret;
}

static inline int _drv_check_vui_videosignal(
    const venc_vui_video_signal_s * pstVuiVideoSignal, unsigned char u8VideoFormatMax, int s32Err) {
    int s32Ret = 0;

    DRV_VENC_DBG(
        "video_signal_type_present_flag %u, video_format %d, video_full_range_flag %u\n",
        pstVuiVideoSignal->video_signal_type_present_flag,
        pstVuiVideoSignal->video_format,
        pstVuiVideoSignal->video_full_range_flag);
    DRV_VENC_DBG(
        "colour_description_present_flag %u, colour_primaries %u\n",
        pstVuiVideoSignal->colour_description_present_flag,
        pstVuiVideoSignal->colour_primaries);
    DRV_VENC_DBG(
        "transfer_characteristics %u, matrix_coefficients %u\n",
        pstVuiVideoSignal->transfer_characteristics,
        pstVuiVideoSignal->matrix_coefficients);

    s32Ret = (pstVuiVideoSignal->video_signal_type_present_flag <= 1) ? 0 : s32Err;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "video_signal_type_present_flag",
            pstVuiVideoSignal->video_signal_type_present_flag);
        return s32Ret;
    }

    if (pstVuiVideoSignal->video_signal_type_present_flag) {
        s32Ret = (pstVuiVideoSignal->video_format <= u8VideoFormatMax) ? 0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "video_format", pstVuiVideoSignal->video_format);
            return s32Ret;
        }

        s32Ret = (pstVuiVideoSignal->video_full_range_flag <= 1) ? 0 : s32Err;
        if (s32Ret != 0) {
            DRV_VENC_ERR("error, %s = %d\n",
                "video_full_range_flag", pstVuiVideoSignal->video_full_range_flag);
            return s32Ret;
        }

        if (pstVuiVideoSignal->colour_description_present_flag) {
            s32Ret = (pstVuiVideoSignal->colour_description_present_flag <= 1) ? 0 : s32Err;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "colour_description_present_flag",
                    pstVuiVideoSignal->colour_description_present_flag);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}

static inline int _drv_check_bitstream_restrict(
    const venc_vui_bitstream_restric_s * pstBitstreamRestrict, int s32Err) {
    int s32Ret = 0;

    s32Ret = (pstBitstreamRestrict->bitstream_restriction_flag <= 1) ? 0 : s32Err;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "bitstream_restriction_flag",
            pstBitstreamRestrict->bitstream_restriction_flag);
        return s32Ret;
    }

    return s32Ret;
}

static int _drv_check_h264_vui_param(const venc_h264_vui_s *pstH264Vui)
{
    int s32Ret = 0;

    s32Ret = _drv_check_vui_aspectratio(&pstH264Vui->stVuiAspectRatio,
                   DRV_ERR_VENC_H264_VUI);
    if (s32Ret != 0) {
        return s32Ret;
    }
    s32Ret = _drv_check_vui_h264_timinginfo(&pstH264Vui->stVuiTimeInfo,
                   DRV_ERR_VENC_H264_VUI);
    if (s32Ret != 0) {
        return s32Ret;
    }
    s32Ret = _drv_check_vui_videosignal(&pstH264Vui->stVuiVideoSignal,
                   DRV_H264_VIDEO_FORMAT_MAX,
                   DRV_ERR_VENC_H264_VUI);
    if (s32Ret != 0) {
        return s32Ret;
    }
    s32Ret = _drv_check_bitstream_restrict(&pstH264Vui->stVuiBitstreamRestric,
                    DRV_ERR_VENC_H264_VUI);
    if (s32Ret != 0) {
        return s32Ret;
    }
    return s32Ret;
}

static int _drv_check_h265_vui_param(const venc_h265_vui_s *pstH265Vui)
{
    int s32Ret = 0;

    s32Ret = _drv_check_vui_aspectratio(&pstH265Vui->stVuiAspectRatio,
                   DRV_ERR_VENC_H265_VUI);
    if (s32Ret != 0) {
        return s32Ret;
    }
    s32Ret = _drv_check_vui_h265_timinginfo(&pstH265Vui->stVuiTimeInfo,
                   DRV_ERR_VENC_H265_VUI);
    if (s32Ret != 0) {
        return s32Ret;
    }
    s32Ret = _drv_check_vui_videosignal(&pstH265Vui->stVuiVideoSignal,
                   DRV_H265_VIDEO_FORMAT_MAX,
                   DRV_ERR_VENC_H265_VUI);
    if (s32Ret != 0) {
        return s32Ret;
    }
    s32Ret = _drv_check_bitstream_restrict(&pstH265Vui->stVuiBitstreamRestric,
                    DRV_ERR_VENC_H265_VUI);
    if (s32Ret != 0) {
        return s32Ret;
    }
    return s32Ret;
}

static int _drv_set_h264_vui(venc_chn_context *pChnHandle,
                  venc_h264_vui_s *pstH264Vui)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H264_VUI,
                  (void *)(pstH264Vui));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("failed to set h264 vui %d", ret);

    return ret;
}

static int _drv_set_h265_vui(venc_chn_context *pChnHandle,
                  venc_h265_vui_s *pstH265Vui)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H265_VUI,
                  (void *)(pstH265Vui));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("failed to set h265 vui %d", ret);

    return ret;
}

#ifdef USB_vb_pool
static int _drv_venc_update_vb_conf(venc_chn_context *pChnHandle,
                    VbVencBufConfig *pVbVencCfg,
                    int VbSetFrameBufSize, int VbSetFrameBufCnt)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;

    #ifdef USE_vb_pool
    int j = 0;
    for (j = 0; j < VbSetFrameBufCnt; j++) {
        uint64_t u64PhyAddr;

        pChnVars->vbBLK[j] =
            vb_get_block_with_id(pChnVars->vbpool.hPicVbPool,
                         VbSetFrameBufSize, ID_VENC);
        if (pChnVars->vbBLK[j] == VB_INVALID_HANDLE) {
            //not enough size in VB  or VB  pool not create
            DRV_VENC_ERR(
                "Not enough size in VB  or VB  pool Not create\n");
            return DRV_ERR_VENC_NOMEM;
        }

        u64PhyAddr = vb_handle2PhysAddr(pChnVars->vbBLK[j]);
        pChnVars->FrmArray[j].phyAddr = u64PhyAddr;
        pChnVars->FrmArray[j].size = VbSetFrameBufSize;
        pChnVars->FrmArray[j].virtAddr =
            vb_handle2VirtAddr(pChnVars->vbBLK[j]);
        pVbVencCfg->vbModeAddr[j] = u64PhyAddr;
    }
    #endif

    pChnVars->FrmNum = VbSetFrameBufCnt;
    pVbVencCfg->VbSetFrameBufCnt = VbSetFrameBufCnt;
    pVbVencCfg->VbSetFrameBufSize = VbSetFrameBufSize;

    return 0;
}
#endif

static int _drv_venc_alloc_vbbuf(venc_chn VeChn)
{
    int s32Ret = 0;
    venc_context *pVencHandle = handle;
    venc_chn_context *pChnHandle = NULL;
    venc_chn_attr_s *pChnAttr = NULL;
    venc_attr_s *pVencAttr = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    vb_source_e eVbSource;
    VbVencBufConfig VbVencCfg;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    if (pVencHandle == NULL) {
        DRV_VENC_ERR(
            "p_venctx_handle NULL (Channel not create yet..)\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnAttr = pChnHandle->pChnAttr;
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;
    pVencAttr = &pChnAttr->stVencAttr;

    if (pVencAttr->enType == PT_H264) {
        eVbSource =
            pVencHandle->ModParam.stH264eModParam.enH264eVBSource;
    } else if (pVencAttr->enType == PT_H265) {
        eVbSource =
            pVencHandle->ModParam.stH265eModParam.enH265eVBSource;
    } else {
        eVbSource = VB_SOURCE_COMMON;
    }

    memset(&VbVencCfg, 0, sizeof(VbVencBufConfig));
    VbVencCfg.VbType = VB_SOURCE_COMMON;

    if (eVbSource == VB_SOURCE_PRIVATE) {
        if (pChnVars->bHasVbPool == 0) {
            // struct vb_pool_cfg stVbPoolCfg;

            pr_err("error warning,vb pool has not implemented\n");
            #ifdef USB_vb_pool
            //step1 get vb info from vpu driver: calculate the buffersize and buffer count
            s32Ret = pEncCtx->base.ioctl(pEncCtx,
                             DRV_H26X_OP_GET_VB_INFO,
                             (void *)&VbVencCfg);

            if (s32Ret != 0) {
                DRV_VENC_ERR("DRV_H26X_OP_GET_VB_INFO, %d\n",
                         s32Ret);
                return s32Ret;
            }

            stVbPoolCfg.blk_size = VbVencCfg.VbGetFrameBufSize;
            stVbPoolCfg.blk_cnt = VbVencCfg.VbGetFrameBufCnt;
            stVbPoolCfg.remap_mode = VB_REMAP_MODE_NONE;
            if (vb_create_pool(&stVbPoolCfg) == 0) {
                pChnVars->vbpool.hPicVbPool =
                    stVbPoolCfg.pool_id;
            }
            pChnVars->bHasVbPool = 1;
            s32Ret = _drv_venc_update_vb_conf(
                pChnHandle, &VbVencCfg,
                VbVencCfg.VbGetFrameBufSize,
                VbVencCfg.VbGetFrameBufCnt);
            if (s32Ret != 0) {
                DRV_VENC_ERR("_drv_venc_update_vb_conf, %d\n",
                         s32Ret);
                return s32Ret;
            }
            #endif
            VbVencCfg.VbType = VB_SOURCE_PRIVATE;
        }
    } else if (eVbSource == VB_SOURCE_USER) {
    #ifdef USE_vb_pool
        struct vb_cfg pstVbConfig;
        int s32UserSetupFrmCnt;
        int s32UserSetupFrmSize;
        //step1 get vb info from vpu driver: calculate the buffersize and buffer count
        s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_GET_VB_INFO,
                         (void *)&VbVencCfg);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_H26X_OP_GET_VB_INFO, %d\n", s32Ret);
            return s32Ret;
        }

        //totally setup by user vbpool id, if not been attachVB return failure
        if (pChnVars->bHasVbPool == 0) {
            DRV_VENC_ERR("[error][%s][%d]\n", __func__, __LINE__);
            DRV_VENC_ERR("Not attach vb pool for channel[%d]\n",
                     VeChn);
            return DRV_ERR_VENC_NOT_SUPPORT;
        }
//check the VB config and compare with VPU requirement
        s32Ret = vb_get_config(&pstVbConfig);
        if (s32Ret == 0) {
            if ((int)pstVbConfig
                    .comm_pool[pChnVars->vbpool.hPicVbPool]
                    .blk_size <
                VbVencCfg.VbGetFrameBufSize) {
                DRV_VENC_WARN(
                    "[Warning] create size is smaller than require size\n");
            }
            if ((int)pstVbConfig
                    .comm_pool[pChnVars->vbpool.hPicVbPool]
                    .blk_size < VbVencCfg.VbGetFrameBufCnt) {
                DRV_VENC_WARN(
                    "[Warning] create blk is smaller than require blk\n");
            }
        } else
            DRV_VENC_ERR("Error while DRV_VB_GetConfig\n");

        s32UserSetupFrmSize =
            pstVbConfig.comm_pool[pChnVars->vbpool.hPicVbPool]
                .blk_size;
        //s32UserSetupFrmCnt = pstVbConfig.comm_pool[pChnHandle->vbpool.hPicVbPool].u32BlkCnt;
        if (s32UserSetupFrmSize < VbVencCfg.VbGetFrameBufSize) {
            DRV_VENC_WARN(
                "Buffer size too small for frame buffer : user mode VB pool[%d] < [%d]n",
                pstVbConfig
                    .comm_pool[pChnVars->vbpool.hPicVbPool]
                    .blk_size,
                VbVencCfg.VbGetFrameBufSize);
        }

        s32UserSetupFrmCnt = VbVencCfg.VbGetFrameBufCnt;
        s32UserSetupFrmSize = VbVencCfg.VbGetFrameBufSize;
        s32Ret = _drv_venc_update_vb_conf(pChnHandle, &VbVencCfg,
                          s32UserSetupFrmSize,
                          s32UserSetupFrmCnt);
        if (s32Ret != 0) {
            DRV_VENC_ERR("_drv_venc_update_vb_conf, %d\n", s32Ret);
            return s32Ret;
        }
        #endif
        VbVencCfg.VbType = VB_SOURCE_USER;
    }

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_VB_BUFFER,
                         (void *)&VbVencCfg);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_H26X_OP_SET_VB_BUFFER, %d\n", s32Ret);
            return s32Ret;
        }
    }

    return s32Ret;
}


static int _drv_init_chn_ctx(venc_chn VeChn, const venc_chn_attr_s *pstAttr)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;

    handle->chn_handle[VeChn] = MEM_CALLOC(1, sizeof(venc_chn_context));
    if (handle->chn_handle[VeChn] == NULL) {
        DRV_VENC_ERR("Allocate chn_handle memory failed !\n");
        s32Ret = DRV_ERR_VENC_NOMEM;
        goto ERR_DRV_INIT_CHN_CTX;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnHandle->VeChn = VeChn;

    MUTEX_INIT(&pChnHandle->chnMutex, 0);
    MUTEX_INIT(&pChnHandle->chnShmMutex, &ma);

    pChnHandle->pChnAttr = MEM_MALLOC(sizeof(venc_chn_attr_s));
    if (pChnHandle->pChnAttr == NULL) {
        DRV_VENC_ERR("Allocate pChnAttr memory failed !\n");
        s32Ret = DRV_ERR_VENC_NOMEM;
        goto ERR_DRV_INIT_CHN_CTX_1;
    }

    memcpy(pChnHandle->pChnAttr, pstAttr, sizeof(venc_chn_attr_s));

    pChnHandle->pChnVars = MEM_CALLOC(1, sizeof(venc_chn_vars));
    if (pChnHandle->pChnVars == NULL) {
        DRV_VENC_ERR("Allocate pChnVars memory failed !\n");
        s32Ret = DRV_ERR_VENC_NOMEM;
        goto ERR_DRV_INIT_CHN_CTX_2;
    }

    pChnHandle->pChnVars->chnState = venc_chn_STATE_INIT;

    pChnHandle->pVbCtx = &vencVbCtx[VeChn];

    pEncCtx = &pChnHandle->encCtx;
    if (venc_create_enc_ctx(pEncCtx, pChnHandle) < 0) {
        DRV_VENC_ERR("venc_create_enc_ctx\n");
        s32Ret = DRV_ERR_VENC_NOMEM;
        goto ERR_DRV_INIT_CHN_CTX_3;
    }

    s32Ret = pEncCtx->base.init(pEncCtx);
    if (s32Ret != 0) {
        DRV_VENC_ERR("base init,s32Ret %#x\n", s32Ret);
        goto ERR_DRV_INIT_CHN_CTX_3;
    }

    s32Ret = _drv_check_rcmode_attr(pChnHandle);
    if (s32Ret != 0) {
        DRV_VENC_ERR("drv_check_rcmode_attr\n");
        goto ERR_DRV_INIT_CHN_CTX_3;
    }

    s32Ret = _drv_check_gop_attr(pChnHandle);
    if (s32Ret != 0) {
        DRV_VENC_ERR("drv_check_gop_attr\n");
        goto ERR_DRV_INIT_CHN_CTX_3;
    }

    s32Ret = _drv_check_gop_exattr(pChnHandle);
    if (s32Ret != 0) {
        DRV_VENC_ERR("drv_check_gop_exattr\n");
        goto ERR_DRV_INIT_CHN_CTX_3;
    }

    s32Ret = _drv_set_chn_default(pChnHandle);
    if (s32Ret != 0) {
        DRV_VENC_ERR("drv_set_chn_default,s32Ret %#x\n", s32Ret);
        goto ERR_DRV_INIT_CHN_CTX_3;
    }

    return s32Ret;

ERR_DRV_INIT_CHN_CTX_3:
    if (pChnHandle->pChnVars) {
        MEM_FREE(pChnHandle->pChnVars);
        pChnHandle->pChnVars = NULL;
    }
ERR_DRV_INIT_CHN_CTX_2:
    if (pChnHandle->pChnAttr) {
        MEM_FREE(pChnHandle->pChnAttr);
        pChnHandle->pChnAttr = NULL;
    }
ERR_DRV_INIT_CHN_CTX_1:
    if (handle->chn_handle[VeChn]) {
        MEM_FREE(handle->chn_handle[VeChn]);
        handle->chn_handle[VeChn] = NULL;
    }
ERR_DRV_INIT_CHN_CTX:

    return s32Ret;
}

static int _drv_check_left_stream_frames(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_chn_status_s *pChnStat = &pChnVars->chnStatus;
    int s32Ret = 0;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    if (pChnStat->u32LeftStreamFrames <= 0) {
        DRV_VENC_WARN(
            "u32LeftStreamFrames <= 0, no stream data to get\n");
        s32Ret = DRV_ERR_VENC_EMPTY_STREAM_FRAME;
    }
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return s32Ret;
}

static int _drv_update_streampack_inbindmode(venc_chn_context *pChnHandle,
                      venc_chn_vars *pChnVars,
                      venc_stream_s *pstStream)
{
    int s32Ret = 0;
    unsigned int idx = 0;

    if (pChnVars->s32BindModeGetStreamRet != 0) {
        DRV_VENC_ERR("bind mode get stream error: 0x%X\n",
                 pChnVars->s32BindModeGetStreamRet);
        return pChnVars->s32BindModeGetStreamRet;
    }

    if (!pChnVars->stStream.pstPack) {
        DRV_VENC_ERR("pChnVars->stStream.pstPack is NULL\n");
        return -2;
    }

    s32Ret = _drv_check_left_stream_frames(pChnHandle);
    if (s32Ret != 0) {
        DRV_VENC_WARN("_drv_check_left_stream_frames, s32Ret = 0x%X\n",
                  s32Ret);
        return s32Ret;
    }

    pstStream->u32PackCount = pChnVars->stStream.u32PackCount;
    for (idx = 0; idx < pstStream->u32PackCount; idx++) {
        venc_pack_s *ppack = &pstStream->pstPack[idx];

        memcpy(ppack, &pChnVars->stStream.pstPack[idx],
               sizeof(venc_pack_s));
        ppack->u64PTS = pChnVars->currPTS;
    }

    return s32Ret;
}

static int _drv_venc_sem_timedwait_millsecs(struct semaphore *sem, long msecs)
{
    int ret;
    int s32RetStatus;

    if (msecs < 0)
        ret = SEMA_WAIT(sem);
    else if (msecs == 0)
        ret = SEMA_TRYWAIT(sem);
    else
        ret = SEMA_TIMEWAIT(sem, usecs_to_jiffies(msecs * 1000));

    if (ret == 0) {
        s32RetStatus = 0;
    } else {
        if (ret > 0) {
            s32RetStatus = DRV_ERR_VENC_BUSY;
        } else if (ret == -EINTR) {
            s32RetStatus = -1;
        } else if (ret == -ETIME) {
            s32RetStatus = DRV_ERR_VENC_BUSY;
        } else {
            DRV_VENC_ERR("sem unexpected errno[%d]time[%ld]\n", ret,
                     msecs);
            s32RetStatus = -1;
        }
    }
    return s32RetStatus;
}


int drv_venc_get_left_streamframes(int VeChn)
{
    venc_chn_context *pChnHandle = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_chn_status_s *pChnStat = NULL;

    if (handle == NULL) {
        return -1;
    }

    if (handle->chn_handle[VeChn] == NULL) {
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pChnStat = &pChnVars->chnStatus;

    return pChnStat->u32LeftStreamFrames;
}

static int _drv_set_venc_perfattr_to_proc(venc_chn_context *pChnHandle)
{
    int s32Ret = 0;
    venc_chn_vars *pChnVars = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    uint64_t u64CurTime = _drv_get_current_time();

    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnShmMutex) != 0) {
        DRV_VENC_ERR("can not lock chnShmMutex\n");
        return -1;
    }
    if ((u64CurTime - pChnVars->u64LastGetStreamTimeStamp) > SEC_TO_MS) {
        pChnVars->stFPS.out_fps =
            (unsigned int)((pChnVars->u32GetStreamCnt * SEC_TO_MS) /
                  ((unsigned int)(u64CurTime -
                         pChnVars->u64LastGetStreamTimeStamp)));
        pChnVars->u64LastGetStreamTimeStamp = u64CurTime;
        pChnVars->u32GetStreamCnt = 0;
    }
    pChnVars->stFPS.max_hw_time = MAX(pChnVars->stFPS.max_hw_time, pEncCtx->base.u64EncHwTime);
    pChnVars->stFPS.hw_time = pEncCtx->base.u64EncHwTime;
    MUTEX_UNLOCK(&pChnHandle->chnShmMutex);

    return s32Ret;
}

static int _drv_process_result(venc_chn_context *pChnHandle,
                venc_stream_s *pstStream)
{
    venc_chn_vars *pChnVars = NULL;
    venc_chn_attr_s *pChnAttr;
    venc_attr_s *pVencAttr;
    int s32Ret = 0;

    pChnVars = pChnHandle->pChnVars;
    pChnAttr = pChnHandle->pChnAttr;
    pVencAttr = &pChnAttr->stVencAttr;

    if (pVencAttr->enType == PT_JPEG || pVencAttr->enType == PT_MJPEG)
        pChnVars->u32GetStreamCnt += pstStream->u32PackCount/2;
    else
        pChnVars->u32GetStreamCnt += pstStream->u32PackCount;

    s32Ret = _drv_set_venc_perfattr_to_proc(pChnHandle);
    if (s32Ret != 0) {
        DRV_VENC_ERR("(chn %d) _drv_set_venc_perfattr_to_proc fail\n",
                 pChnHandle->VeChn);
        return s32Ret;
    }

    if (pVencAttr->enType == PT_JPEG || pVencAttr->enType == PT_MJPEG) {
        if (pstStream->u32PackCount > 0) {
            venc_pack_s *ppack = &pstStream->pstPack[pstStream->u32PackCount -1];
            int j;

            for (j = (ppack->u32Len - ppack->u32Offset - 1); j > 0;
                 j--) {
                unsigned char *tmp_ptr =
                    ppack->pu8Addr + ppack->u32Offset + j;
                if (tmp_ptr[0] == 0xd9 && tmp_ptr[-1] == 0xff) {
                    break;
                }
            }
            ppack->u32Len = ppack->u32Offset + j + 1;
        }
    }

    return s32Ret;
}

static void _drv_update_chn_status(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_chn_status_s *pChnStat = &pChnVars->chnStatus;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return;
    }

    pChnStat->stVencStrmInfo.u32MeanQp =
        pEncCtx->ext.vid.streamInfo.u32MeanQp;
    pChnStat->u32LeftStreamFrames--;

    MUTEX_UNLOCK(&pChnHandle->chnMutex);
}


static int _drv_set_venc_chnattr_to_proc(venc_chn VeChn,
                       const video_frame_info_s *pstFrame)
{
    int s32Ret = 0;
    venc_chn_vars *pChnVars = NULL;
    venc_chn_context *pChnHandle = NULL;
    uint64_t u64CurTime = _drv_get_current_time();


    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    if (pstFrame == NULL) {
        DRV_VENC_ERR("pstFrame is NULL\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;

    if (MUTEX_LOCK(&pChnHandle->chnShmMutex) != 0) {
        DRV_VENC_ERR("can not lock chnShmMutex\n");
        return -1;
    }
    memcpy(&pChnVars->stFrameInfo, pstFrame, sizeof(video_frame_info_s));
    if ((u64CurTime - pChnVars->u64LastSendFrameTimeStamp) > SEC_TO_MS) {
        pChnVars->stFPS.in_fps =
            (unsigned int)((pChnVars->u32SendFrameCnt * SEC_TO_MS) /
                  (unsigned int)(u64CurTime -
                        pChnVars->u64LastSendFrameTimeStamp));
        pChnVars->u64LastSendFrameTimeStamp = u64CurTime;
        pChnVars->u32SendFrameCnt = 0;
    }
    MUTEX_UNLOCK(&pChnHandle->chnShmMutex);

    return s32Ret;
}

static int _drv_venc_h265_set_trans(venc_chn_context *pChnHandle,
                   venc_h265_trans_s *pstH265Trans)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H265_TRANS,
                  (void *)(pstH265Trans));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("failed to set h265 trans %d", ret);

    return ret;
}

static int _drv_check_rc_param(venc_chn_context *pChnHandle,
                   const venc_rc_param_s *pstRcParam)
{
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_attr_s *pVencAttr = &pChnAttr->stVencAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    int s32Ret = 0;

    s32Ret = ((pstRcParam->s32InitialDelay >= DRV_INITIAL_DELAY_MIN) &&
        (pstRcParam->s32InitialDelay <= DRV_INITIAL_DELAY_MAX)) ?
        0 : DRV_ERR_VENC_RC_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "s32InitialDelay", pstRcParam->s32InitialDelay);
        return s32Ret;
    }

    if (pVencAttr->enType == PT_H264) {
        if (prcatt->enRcMode == VENC_RC_MODE_H264CBR) {
            s32Ret = _drv_check_common_rcparam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != 0) {
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264VBR) {
            const venc_param_h264_vbr_s *pprc =
                &pstRcParam->stParamH264Vbr;

            s32Ret = _drv_check_common_rcparam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != 0) {
                return s32Ret;
            }
            s32Ret = ((pprc->s32ChangePos >= DRV_H26X_CHANGE_POS_MIN) &&
                (pprc->s32ChangePos <= DRV_H26X_CHANGE_POS_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32ChangePos", pprc->s32ChangePos);
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264AVBR) {
            const venc_param_h264_avbr_s *pprc =
                &pstRcParam->stParamH264AVbr;

            s32Ret = _drv_check_common_rcparam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != 0) {
                return s32Ret;
            }
            s32Ret = ((pprc->s32ChangePos >= DRV_H26X_CHANGE_POS_MIN) &&
                (pprc->s32ChangePos <= DRV_H26X_CHANGE_POS_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32ChangePos", pprc->s32ChangePos);
                return s32Ret;
            }
            s32Ret = ((pprc->s32MinStillPercent >= DRV_H26X_MIN_STILL_PERCENT_MIN) &&
                (pprc->s32MinStillPercent <= DRV_H26X_MIN_STILL_PERCENT_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32MinStillPercent", pprc->s32MinStillPercent);
                return s32Ret;
            }
            s32Ret = ((pprc->u32MaxStillQP >= pprc->u32MinQp) &&
                (pprc->u32MaxStillQP <= pprc->u32MaxQp)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "u32MaxStillQP", pprc->u32MaxStillQP);
                return s32Ret;
            }
            s32Ret = ((pprc->u32MotionSensitivity >= DRV_H26X_MOTION_SENSITIVITY_MIN) &&
                (pprc->u32MotionSensitivity <= DRV_H26X_MOTION_SENSITIVITY_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "u32MotionSensitivity", pprc->u32MotionSensitivity);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrFrmLostOpen >= DRV_H26X_AVBR_FRM_LOST_OPEN_MIN) &&
                (pprc->s32AvbrFrmLostOpen <= DRV_H26X_AVBR_FRM_LOST_OPEN_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32AvbrFrmLostOpen", pprc->s32AvbrFrmLostOpen);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrFrmGap >= DRV_H26X_AVBR_FRM_GAP_MIN) &&
                (pprc->s32AvbrFrmGap <= DRV_H26X_AVBR_FRM_GAP_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32AvbrFrmGap", pprc->s32AvbrFrmGap);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrPureStillThr >= DRV_H26X_AVBR_PURE_STILL_THR_MIN) &&
                (pprc->s32AvbrPureStillThr <= DRV_H26X_AVBR_PURE_STILL_THR_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32AvbrPureStillThr", pprc->s32AvbrPureStillThr);
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264UBR) {
            s32Ret = _drv_check_common_rcparam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != 0) {
                return s32Ret;
            }
        }
    } else if (pVencAttr->enType == PT_H265) {
        if (prcatt->enRcMode == VENC_RC_MODE_H265CBR) {
            s32Ret = _drv_check_common_rcparam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != 0) {
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265VBR) {
            const venc_param_h265_vbr_s *pprc =
                &pstRcParam->stParamH265Vbr;

            s32Ret = _drv_check_common_rcparam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != 0) {
                return s32Ret;
            }
            s32Ret = ((pprc->s32ChangePos >= DRV_H26X_CHANGE_POS_MIN) &&
                (pprc->s32ChangePos <= DRV_H26X_CHANGE_POS_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32ChangePos", pprc->s32ChangePos);
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265AVBR) {
            const venc_param_h265_avbr_s *pprc =
                &pstRcParam->stParamH265AVbr;

            s32Ret = _drv_check_common_rcparam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != 0) {
                return s32Ret;
            }
            s32Ret = ((pprc->s32ChangePos >= DRV_H26X_CHANGE_POS_MIN) &&
                (pprc->s32ChangePos <= DRV_H26X_CHANGE_POS_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32ChangePos", pprc->s32ChangePos);
                return s32Ret;
            }
            s32Ret = ((pprc->s32MinStillPercent >= DRV_H26X_MIN_STILL_PERCENT_MIN) &&
                (pprc->s32MinStillPercent <= DRV_H26X_MIN_STILL_PERCENT_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32MinStillPercent", pprc->s32MinStillPercent);
                return s32Ret;
            }
            s32Ret = ((pprc->u32MaxStillQP >= pprc->u32MinQp) &&
                (pprc->u32MaxStillQP <= pprc->u32MaxQp)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "u32MaxStillQP", pprc->u32MaxStillQP);
                return s32Ret;
            }
            s32Ret = ((pprc->u32MotionSensitivity >= DRV_H26X_MOTION_SENSITIVITY_MIN) &&
                (pprc->u32MotionSensitivity <= DRV_H26X_MOTION_SENSITIVITY_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "u32MotionSensitivity", pprc->u32MotionSensitivity);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrFrmLostOpen >= DRV_H26X_AVBR_FRM_LOST_OPEN_MIN) &&
                (pprc->s32AvbrFrmLostOpen <= DRV_H26X_AVBR_FRM_LOST_OPEN_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32AvbrFrmLostOpen", pprc->s32AvbrFrmLostOpen);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrFrmGap >= DRV_H26X_AVBR_FRM_GAP_MIN) &&
                (pprc->s32AvbrFrmGap <= DRV_H26X_AVBR_FRM_GAP_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32AvbrFrmGap", pprc->s32AvbrFrmGap);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrPureStillThr >= DRV_H26X_AVBR_PURE_STILL_THR_MIN) &&
                (pprc->s32AvbrPureStillThr <= DRV_H26X_AVBR_PURE_STILL_THR_MAX)) ?
                0 : DRV_ERR_VENC_RC_PARAM;
            if (s32Ret != 0) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "s32AvbrPureStillThr", pprc->s32AvbrPureStillThr);
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265UBR) {
            s32Ret = _drv_check_common_rcparam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != 0) {
                return s32Ret;
            }
        }
    } else if (pVencAttr->enType == PT_MJPEG) {
        if (prcatt->enRcMode == VENC_RC_MODE_MJPEGCBR) {
            const venc_param_mjpeg_cbr_s *pstParamJpgCbr =
                &pstRcParam->stParamMjpegCbr;
            if (pstParamJpgCbr->u32MinQfactor <= 0 ||
                pstParamJpgCbr->u32MinQfactor > Q_TABLE_MAX) {
                DRV_VENC_ERR("error, %s = %d\n",
                        "cbr u32MinQfactor", pstParamJpgCbr->u32MinQfactor);
                return DRV_ERR_VENC_RC_PARAM;
            }
            if (pstParamJpgCbr->u32MaxQfactor < pstParamJpgCbr->u32MinQfactor ||
                pstParamJpgCbr->u32MaxQfactor > Q_TABLE_MAX) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "cbr u32MaxQfactor", pstParamJpgCbr->u32MaxQfactor);
                return DRV_ERR_VENC_RC_PARAM;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_MJPEGVBR) {
            const venc_param_mjpeg_vbr_s *pstParamJpgVbr =
                &pstRcParam->stParamMjpegVbr;
            if (pstParamJpgVbr->u32MinQfactor <= 0 ||
                pstParamJpgVbr->u32MinQfactor > Q_TABLE_MAX) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "vbr u32MinQfactor", pstParamJpgVbr->u32MinQfactor);
                return DRV_ERR_VENC_RC_PARAM;
            }

            if (pstParamJpgVbr->u32MaxQfactor < pstParamJpgVbr->u32MinQfactor ||
                pstParamJpgVbr->u32MaxQfactor > Q_TABLE_MAX) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "vbr u32MaxQfactor", pstParamJpgVbr->u32MaxQfactor);
                return DRV_ERR_VENC_RC_PARAM;
            }

            if (pstParamJpgVbr->s32ChangePos < 50 ||
                pstParamJpgVbr->s32ChangePos > 100) {
                DRV_VENC_ERR("error, %s = %d\n",
                    "vbr s32ChangePos", pstParamJpgVbr->s32ChangePos);
                return DRV_ERR_VENC_RC_PARAM;
            }
        }
    }

    return s32Ret;
}


static int _drv_set_cupred_impl(venc_chn_context *pChnHandle)
{
    venc_cu_prediction_s *pcup = &pChnHandle->pChnVars->cuPrediction;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    Pred pred, *pPred = &pred;
    int s32Ret = 0;

    pPred->u32IntraCost = pcup->u32IntraCost;

    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_PREDICT,
                     (void *)pPred);
    if (s32Ret != 0) {
        DRV_VENC_ERR("DRV_H26X_OP_SET_PREDICT, %d\n", s32Ret);
        return DRV_ERR_VENC_CU_PREDICTION;
    }

    return s32Ret;
}


static int _drv_venc_jpeg_insert_userdata(venc_chn_context *pChnHandle,
                     unsigned char *pu8Data, unsigned int u32Len)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    jpeg_user_data stUserData;

    stUserData.userData = pu8Data;
    stUserData.len = u32Len;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, DRV_JPEG_OP_SET_USER_DATA,
                  (void *)(&stUserData));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("jpeg failed to insert user data, %d", ret);
    return ret;
}

static int _drv_venc_h26x_insert_userdata(venc_chn_context *pChnHandle,
                     unsigned char *pu8Data, unsigned int u32Len)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    UserData stUserData;

    stUserData.userData = pu8Data;
    stUserData.len = u32Len;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_USER_DATA,
                  (void *)(&stUserData));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("h26x failed to insert user data, %d", ret);
    return ret;
}

static int _drv_venc_h264_set_entropy(venc_chn_context *pChnHandle,
                     venc_h264_entropy_s *pstH264Entropy)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H264_ENTROPY,
                  (void *)(pstH264Entropy));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("failed to set h264 entropy, %d", ret);

    return ret;
}

static int _drv_venc_h265_set_predunit(venc_chn_context *pChnHandle,
        venc_h265_pu_s *pstH265PredUnit)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H265_PRED_UNIT,
           (void *)(pstH265PredUnit));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("failed to set h265 PredUnit %d", ret);

    return ret;
}

static int _drv_venc_h265_get_predunit(venc_chn_context *pChnHandle,
        venc_h265_pu_s *pstH265PredUnit)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_GET_H265_PRED_UNIT,
           (void *)(pstH265PredUnit));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("failed to get h265 PredUnit %d", ret);

    return ret;
}

static int _drv_venc_h264_set_trans(venc_chn_context *pChnHandle,
                   venc_h264_trans_s *pstH264Trans)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H264_TRANS,
                  (void *)(pstH264Trans));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("failed to set h264 trans %d", ret);

    return ret;
}

static int _drv_venc_set_searchwindow(venc_chn_context *pChnHandle, venc_search_window_s *pstVencSearchWindow)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_SEARCH_WINDOW,
           (void *)(pstVencSearchWindow));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("failed to set search window %d", ret);

    return ret;
}

static int _drv_venc_get_searchwindow(venc_chn_context *pChnHandle, venc_search_window_s *pstVencSearchWindow)
{
    int ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_GET_SEARCH_WINDOW,
           (void *)(pstVencSearchWindow));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != 0)
        DRV_VENC_ERR("failed to get search window %d", ret);

    return ret;
}
/**
 * @brief Create One VENC Channel.
 * @param[in] VeChn VENC Channel Number.
 * @param[in] pstAttr pointer to venc_chn_attr_s.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
int drv_venc_create_chn(venc_chn VeChn, const venc_chn_attr_s *pstAttr)
{

    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;

    s32Ret = _drv_init_chn_ctx(VeChn, pstAttr);
    if (s32Ret != 0) {
        DRV_VENC_ERR("init\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;

    s32Ret = pEncCtx->base.open(handle, pChnHandle);
    if (s32Ret != 0) {
        DRV_VENC_ERR("venc_init_encoder\n");
        return s32Ret;
    }

    pChnHandle->bSbSkipFrm = false;
    handle->chn_status[VeChn] = venc_chn_STATE_INIT;
    return s32Ret;
}


int drv_venc_start_recvframe(venc_chn VeChn,
                const venc_recv_pic_param_s *pstRecvParam)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_vb_ctx *pVbCtx = NULL;
    venc_attr_s *pVencAttr = NULL;
    mmf_chn_s stBindSrc;
    mmf_chn_s chn = {.mod_id = ID_VENC, .dev_id = 0, .chn_id = VeChn};
    int isBindMode = FALSE;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;
    pVbCtx = pChnHandle->pVbCtx;
    pVencAttr = &pChnHandle->pChnAttr->stVencAttr;

    pChnVars->s32RecvPicNum = pstRecvParam->s32RecvPicNum;
    if (pChnVars->chnState == venc_chn_STATE_START_ENC)
        return 0;
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    s32Ret = _drv_set_rcparam_to_drv(pChnHandle);
    if (s32Ret < 0) {
        DRV_VENC_ERR("drv_set_rcparam_to_drv\n");
        MUTEX_UNLOCK(&pChnHandle->chnMutex);
        return s32Ret;
    }

    s32Ret = _drv_set_cupred_impl(pChnHandle);
    if (s32Ret < 0) {
        DRV_VENC_ERR("_drv_set_cupred_impl\n");
        return s32Ret;
    }

    if (pEncCtx->base.ioctl) {
        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265 ||
            pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
            s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_START, (void *)&pChnHandle->VeChn);
            if (s32Ret != 0) {
                DRV_VENC_ERR("DRV_H26X_OP_START, %d\n", s32Ret);
                MUTEX_UNLOCK(&pChnHandle->chnMutex);
                return -1;
            }
        } /*else {
            s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_JPEG_OP_START,
                (void *)&pChnHandle->pChnAttr->stVencAttr.bIsoSendFrmEn);
            if (s32Ret != 0) {
                DRV_VENC_ERR("DRV_JPEG_OP_START, %d\n", s32Ret);
                MUTEX_UNLOCK(&pChnHandle->chnMutex);
                return -1;
            }
        }*/
    }

    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    pChnVars->chnState = venc_chn_STATE_START_ENC;
    handle->chn_status[VeChn] = venc_chn_STATE_START_ENC;

    s32Ret = bind_get_src(&chn, &stBindSrc);
    DRV_VENC_DBG("get bind src, ret:%d\n", s32Ret);
    if (s32Ret == 0) {
        // struct sched_param param = {
        //     .sched_priority = 95,
        // };
        if (pEncCtx->base.ioctl) {
            if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265 ||
                pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
                isBindMode = TRUE;
                s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_BIND_MODE, (void *)&isBindMode);
                if (s32Ret != 0) {
                    DRV_VENC_ERR("DRV_H26X_OP_SET_BIND_MODE, %d\n", s32Ret);
                    return -1;
                }
            }
        }

        pVbCtx->currBindMode = 1;
        pChnHandle->bChnEnable = 1;
        if (!pVbCtx->thread) {
            pVbCtx->thread = kthread_run(_venc_event_handler,
                            (void *)pChnHandle,
                            "soph_vc_bh%d", VeChn);

            if (IS_ERR(pVbCtx->thread)) {
                DRV_VENC_ERR(
                    "failed to create venc binde mode thread for chn %d\n",
                    VeChn);
                return -1;
            }
            // sched_setscheduler(pVbCtx->thread, SCHED_RR, &param);
            SEMA_POST(&pChnVars->sem_send);
        }
    }

    return 0;
}

int drv_venc_stop_recvframe(venc_chn VeChn)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_vb_ctx *pVbCtx = NULL;
    venc_chn_vars *pChnVars = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pVbCtx = pChnHandle->pVbCtx;

    if (pVbCtx->currBindMode == 1) {
        SEMA_POST(&pChnVars->sem_release);
        pChnHandle->bChnEnable = 0;
        SEMA_POST(&pVbCtx->vb_jobs.sem);
        kthread_stop(pVbCtx->thread);
        pVbCtx->thread = NULL;
        pVbCtx->currBindMode = 0;
        SEMA_POST(&pChnVars->sem_send);
        DRV_VENC_INFO("venc_event_handler end\n");
    }

    pChnVars->chnState = venc_chn_STATE_STOP_ENC;
    handle->chn_status[VeChn] = venc_chn_STATE_STOP_ENC;

    return s32Ret;
}

/**
 * @brief Query the current channel status of encoder
 * @param[in] VeChn VENC Channel Number.
 * @param[out] pstStatus Current channel status of encoder
 * @retval 0 Success
 * @retval Non-0 Failure, Please see the error code table
 */
int drv_venc_query_status(venc_chn VeChn, venc_chn_status_s *pstStatus)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_attr_s *pVencAttr = NULL;
    venc_chn_status_s *pChnStat = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_chn_attr_s *pChnAttr = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pChnAttr = pChnHandle->pChnAttr;
    pChnStat = &pChnVars->chnStatus;
    pVencAttr = &pChnAttr->stVencAttr;

    memcpy(pstStatus, pChnStat, sizeof(venc_chn_status_s));
    pstStatus->u32CurPacks = _drv_get_num_packs(pVencAttr->enType);

    return s32Ret;
}



int drv_venc_send_frame_ex(venc_chn VeChn,
                 const user_frame_info_s *pstUserFrame,
                 int s32MilliSec)
{
    const video_frame_info_s *pstFrame = &pstUserFrame->stUserFrame;
    UserRcInfo userRcInfo, *puri = &userRcInfo;
    venc_chn_context *pChnHandle = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    int s32Ret = 0;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;

    memcpy(&pChnVars->stUserRcInfo, &pstUserFrame->stUserRcInfo,
           sizeof(user_rc_info_s));

    puri->bQpMapValid = pChnVars->stUserRcInfo.bQpMapValid;
    puri->u64QpMapPhyAddr = pChnVars->stUserRcInfo.u64QpMapPhyAddr;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_USER_RC_INFO,
                     (void *)puri);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret != 0) {
        DRV_VENC_ERR("DRV_H26X_OP_SET_USER_RC_INFO, %X\n", s32Ret);
        return s32Ret;
    }

    s32Ret = drv_venc_send_frame(VeChn, pstFrame, s32MilliSec);

    return s32Ret;
}


/**
 * @brief Get encoded bitstream
 * @param[in] VeChn VENC Channel Number.
 * @param[out] pstStream pointer to video_frame_info_s.
 * @param[in] S32MilliSec TODO VENC
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
int drv_venc_get_stream(venc_chn VeChn, venc_stream_s *pstStream,
               int S32MilliSec)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    uint64_t getStream_e;
    venc_chn_attr_s *pChnAttr;
    venc_vb_ctx *pVbCtx = NULL;
    venc_attr_s *pVencAttr = NULL;
    int32_t curr_packs_num = 0;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pChnAttr = pChnHandle->pChnAttr;
    pVbCtx = pChnHandle->pVbCtx;
    pVencAttr = &pChnAttr->stVencAttr;

    if (pChnHandle->bSbSkipFrm == true) {
        //DRV_VENC_ERR("SbSkipFrm\n");
        return DRV_ERR_VENC_BUSY;
    }

    if (S32MilliSec == 0 && (pVencAttr->enType == PT_H264 || pVencAttr->enType == PT_H265)) {
        if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
            DRV_VENC_ERR("can not lock chnMutex\n");
            return -1;
        }
        s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_GET_BS_PACKS_NUM,
                        (void *)&curr_packs_num);
        MUTEX_UNLOCK(&pChnHandle->chnMutex);
        if (curr_packs_num == -1) {
            return DRV_ERR_VENC_GET_STREAM_END;
        }

        if (curr_packs_num == 0) {
            return DRV_ERR_VENC_BUSY;
        }
    }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
    pstStream->pstPack = vmalloc(sizeof(venc_pack_s) * _drv_get_num_packs(pChnAttr->stVencAttr.enType));
    if (pstStream->pstPack == NULL)
        return DRV_ERR_VENC_NOMEM;
#endif

    if (!pVencAttr->bIsoSendFrmEn && pVbCtx->currBindMode == 1) {
        DRV_VENC_INFO("wait encode done\n");
        //sem_wait(&pChnVars->sem_send);

        s32Ret = _drv_venc_sem_timedwait_millsecs(&pChnVars->sem_send,
                            S32MilliSec);
        if (s32Ret == -1) {
            DRV_VENC_ERR("sem wait error\n");
            return DRV_ERR_VENC_MUTEX_ERROR;

        } else if (s32Ret == DRV_ERR_VENC_BUSY) {
            DRV_VENC_INFO("sem wait timeout\n");
            return s32Ret;
        }

        s32Ret = _drv_update_streampack_inbindmode(pChnHandle, pChnVars,
                            pstStream);
        if (s32Ret != 0) {
            DRV_VENC_ERR(
                "_drv_update_streampack_inbindmode fail, s32Ret = 0x%X\n",
                s32Ret);
            return s32Ret;
        }

        _drv_update_chn_status(pChnHandle);

        return 0;
    }

    s32Ret = pEncCtx->base.getStream(pEncCtx, pstStream, S32MilliSec);
    if (s32Ret != 0) {
        if ((S32MilliSec >= 0) && (s32Ret == DRV_ERR_VENC_BUSY)) {
            //none block mode: timeout
            DRV_VENC_DBG("Non-blcok mode timeout\n");
        } else {
            //block mode
            if (s32Ret != DRV_ERR_VENC_EMPTY_STREAM_FRAME && s32Ret != DRV_ERR_VENC_GET_STREAM_END)
                DRV_VENC_ERR("getStream, s32Ret = %d\n", s32Ret);
        }

        return s32Ret;
    }

    s32Ret = _drv_process_result(pChnHandle, pstStream);
    if (s32Ret != 0) {
        DRV_VENC_ERR("(chn %d) _drv_process_result fail\n", VeChn);
        return s32Ret;
    }
    getStream_e = _drv_get_current_time();
    pChnVars->totalTime += (getStream_e - pChnVars->u64TimeOfSendFrame);

    _drv_update_chn_status(pChnHandle);

    return s32Ret;
}

int drv_venc_release_stream(venc_chn VeChn, venc_stream_s *pstStream)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_attr_s *pChnAttr;
    venc_chn_vars *pChnVars = NULL;
    venc_vb_ctx *pVbCtx = NULL;
    venc_attr_s *pVencAttr;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pVbCtx = pChnHandle->pVbCtx;
    pChnAttr = pChnHandle->pChnAttr;
    pVencAttr = &pChnAttr->stVencAttr;

    if (!pVencAttr->bIsoSendFrmEn && pVbCtx->currBindMode == 1) {
        DRV_VENC_INFO("[%d]post release\n", pChnVars->frameIdx);
        SEMA_POST(&pChnVars->sem_release);
    } else {
        s32Ret = pEncCtx->base.releaseStream(pEncCtx, pstStream);
        if (s32Ret != 0) {
            DRV_VENC_ERR("releaseStream, s32Ret = %d\n", s32Ret);
            return s32Ret;
        }
    }

    pChnVars->frameIdx++;

    if (pChnHandle->jpgFrmSkipCnt) {
        pChnHandle->jpgFrmSkipCnt--;
    }

    return s32Ret;
}


/**
 * @brief Send encoder module parameters
 * @param[in] venc_param_mod_s.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
int drv_venc_set_modparam(const venc_param_mod_s *pstModParam)
{
    int s32Ret = 0;
    venc_context *pVencHandle;
    unsigned int *pUserDataMaxLen;

    if (pstModParam == NULL) {
        DRV_VENC_ERR("pstModParam NULL !\n");
        return DRV_ERR_VENC_ILLEGAL_PARAM;
    }

    s32Ret = _drv_venc_init_ctx_handle();
    if (s32Ret != 0) {
        DRV_VENC_ERR("drv_venc_set_modparam  init failure\n");
        return s32Ret;
    }

    pVencHandle = (venc_context *)handle;
    if (pVencHandle == NULL) {
        DRV_VENC_ERR(
            "drv_venc_set_modparam venc_context global handle not create\n");
        return DRV_ERR_VENC_NULL_PTR;

    } else {
        if (pstModParam->enVencModType == MODTYPE_H264E) {
            memcpy(&pVencHandle->ModParam.stH264eModParam,
                   &pstModParam->stH264eModParam,
                   sizeof(venc_mod_h264e_s));

            pUserDataMaxLen =
                &(pVencHandle->ModParam.stH264eModParam
                      .u32UserDataMaxLen);
            *pUserDataMaxLen =
                MIN(*pUserDataMaxLen, USERDATA_MAX_LIMIT);
        } else if (pstModParam->enVencModType == MODTYPE_H265E) {
            memcpy(&pVencHandle->ModParam.stH265eModParam,
                   &pstModParam->stH265eModParam,
                   sizeof(venc_mod_h265e_s));

            pUserDataMaxLen =
                &(pVencHandle->ModParam.stH265eModParam
                      .u32UserDataMaxLen);
            *pUserDataMaxLen =
                MIN(*pUserDataMaxLen, USERDATA_MAX_LIMIT);
        } else if (pstModParam->enVencModType == MODTYPE_JPEGE) {
            s32Ret = _drv_venc_check_jpege_format(
                &pstModParam->stJpegeModParam);
            if (s32Ret != 0) {
                DRV_VENC_ERR(
                    "drv_venc_set_modparam  JPEG marker order error\n");
                return s32Ret;
            }

            memcpy(&pVencHandle->ModParam.stJpegeModParam,
                   &pstModParam->stJpegeModParam,
                   sizeof(venc_mod_jpege_s));
            _drv_venc_config_jpege_format(
                &pVencHandle->ModParam.stJpegeModParam);
        }
    }

    return s32Ret;
}

/**
 * @brief Get encoder module parameters
 * @param[in] venc_param_mod_s.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
int drv_venc_get_modparam(venc_param_mod_s *pstModParam)
{
    int s32Ret;
    venc_context *pVencHandle;

    if (pstModParam == NULL) {
        DRV_VENC_ERR("pstModParam NULL !\n");
        return DRV_ERR_VENC_ILLEGAL_PARAM;
    }

    s32Ret = _drv_venc_init_ctx_handle();
    if (s32Ret != 0) {
        DRV_VENC_ERR("drv_venc_get_modparam  init failure\n");
        return s32Ret;
    }

    pVencHandle = (venc_context *)handle;
    if (pVencHandle == NULL) {
        DRV_VENC_ERR(
            "drv_venc_get_modparam venc_context global handle not create\n");
        return DRV_ERR_VENC_NULL_PTR;

    } else {
        if (pstModParam->enVencModType == MODTYPE_H264E) {
            memcpy(&pstModParam->stH264eModParam,
                   &pVencHandle->ModParam.stH264eModParam,
                   sizeof(venc_mod_h264e_s));
        } else if (pstModParam->enVencModType == MODTYPE_H265E) {
            memcpy(&pstModParam->stH265eModParam,
                   &pVencHandle->ModParam.stH265eModParam,
                   sizeof(venc_mod_h265e_s));
        } else if (pstModParam->enVencModType == MODTYPE_JPEGE) {
            memcpy(&pstModParam->stJpegeModParam,
                   &pVencHandle->ModParam.stJpegeModParam,
                   sizeof(venc_mod_jpege_s));
        }
    }

    return 0;
}

/**
 * @brief encoder module attach vb buffer
 * @param[in] VeChn VENC Channel Number.
 * @param[in] pstPool venc_chn_pool_s.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
int drv_venc_attach_vbpool(venc_chn VeChn, const venc_chn_pool_s *pstPool)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_context *p_venctx_handle = handle;
    vb_source_e eVbSource = VB_SOURCE_COMMON;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = p_venctx_handle->chn_handle[VeChn];

    if (pstPool == NULL) {
        DRV_VENC_ERR("pstPool = NULL\n");
        return DRV_ERR_VENC_ILLEGAL_PARAM;
    }

    if (pChnHandle->pChnAttr->stVencAttr.enType == PT_JPEG ||
        pChnHandle->pChnAttr->stVencAttr.enType == PT_MJPEG) {
        DRV_VENC_ERR("Not support Picture type\n");
        return DRV_ERR_VENC_NOT_SUPPORT;

    } else if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264)
        eVbSource = p_venctx_handle->ModParam.stH264eModParam
                    .enH264eVBSource;
    else if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265)
        eVbSource = p_venctx_handle->ModParam.stH265eModParam
                    .enH265eVBSource;

    if (eVbSource != VB_SOURCE_USER) {
        DRV_VENC_ERR("Not support eVbSource:%d\n", eVbSource);
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    pChnHandle->pChnVars->bHasVbPool = 1;
    pChnHandle->pChnVars->vbpool = *pstPool;

    return 0;
}

/**
 * @brief encoder module detach vb buffer
 * @param[in] VeChn VENC Channel Number.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
int drv_venc_detach_vbpool(venc_chn VeChn)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_context *p_venctx_handle = handle;
    venc_chn_vars *pChnVars = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = p_venctx_handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    if (pChnVars->bHasVbPool == 0) {
        DRV_VENC_ERR("VeChn= %d vbpool not been attached\n", VeChn);
        return DRV_ERR_VENC_NOT_SUPPORT;

    } else {
        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265 ||
            (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264)) {


            #ifdef USE_vb_pool
            int i = 0;
            for (i = 0; i < (int)pChnVars->FrmNum; i++) {
                vb_blk blk;

                blk = vb_physAddr2Handle(
                    pChnVars->FrmArray[i].phyAddr);
                if (blk != VB_INVALID_HANDLE)
                    vb_release_block(blk);
            }
            #endif
        } else {
            DRV_VENC_ERR("Not Support Type with bHasVbPool on\n");
            return DRV_ERR_VENC_NOT_SUPPORT;
        }

        pChnVars->bHasVbPool = 0;
    }

    return 0;
}

/**
 * @brief Send one frame to encode
 * @param[in] VeChn VENC Channel Number.
 * @param[in] pstFrame pointer to video_frame_info_s.
 * @param[in] s32MilliSec TODO VENC
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
int drv_venc_send_frame(venc_chn VeChn, const video_frame_info_s *pstFrame,
               int s32MilliSec)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_chn_status_s *pChnStat = NULL;
    int i = 0;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pChnStat = &pChnVars->chnStatus;

    if (pChnVars->chnState != venc_chn_STATE_START_ENC) {
        return DRV_ERR_VENC_INIT;
    }

    if (pChnVars->bSendFirstFrm == false) {
        unsigned char bCbCrInterleave = 0;
        unsigned char bNV21 = 0;

        switch (pstFrame->video_frame.pixel_format) {
        case PIXEL_FORMAT_NV12:
            bCbCrInterleave = true;
            bNV21 = false;
            break;
        case PIXEL_FORMAT_NV16:
        case PIXEL_FORMAT_NV61:
            bCbCrInterleave = true;
            bNV21 = false;
            break;
        case PIXEL_FORMAT_NV21:
            bCbCrInterleave = true;
            bNV21 = true;
            break;
        case PIXEL_FORMAT_YUV_PLANAR_420:
        default:
            bCbCrInterleave = false;
            bNV21 = false;
            break;
        }

        _drv_venc_set_pixelformat(VeChn, bCbCrInterleave, bNV21);

        s32Ret = _drv_venc_alloc_vbbuf(VeChn);
        if (s32Ret != 0) {
            DRV_VENC_ERR("_drv_venc_alloc_vbbuf\n");
            return DRV_ERR_VENC_NOBUF;
        }

        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264 ||
            pChnHandle->pChnAttr->stVencAttr.enType == PT_H265) {
            _drv_venc_reg_vbbuf(VeChn);
        }

        pChnVars->u32FirstPixelFormat =
            pstFrame->video_frame.pixel_format;
        pChnVars->bSendFirstFrm = true;
    } else {
        if (pChnVars->u32FirstPixelFormat !=
            pstFrame->video_frame.pixel_format) {
            if (pChnHandle->pChnAttr->stVencAttr.enType ==
                    PT_H264 ||
                pChnHandle->pChnAttr->stVencAttr.enType ==
                    PT_H265) {
                DRV_VENC_ERR("Input enPixelFormat change\n");
                return DRV_ERR_VENC_ILLEGAL_PARAM;
            }
        }
    }

    for (i = 0; i < 3; i++) {
        pChnVars->u32Stride[i] = pstFrame->video_frame.stride[i];
    }

    pChnVars->u32SendFrameCnt++;

#ifdef CLI_DEBUG_SUPPORT
    CliDumpSrcYuv(VeChn, pstFrame);
#endif

    s32Ret = _drv_set_venc_chnattr_to_proc(VeChn, pstFrame);
    if (s32Ret != 0) {
        DRV_VENC_ERR("(chn %d) _drv_set_venc_chnattr_to_proc fail\n", VeChn);
        return s32Ret;
    }
    pChnVars->u64TimeOfSendFrame = _drv_get_current_time();

    if (pEncCtx->base.bVariFpsEn) {
        s32Ret = _drv_check_fps(pChnHandle, pstFrame);
        if (s32Ret == DRV_ERR_VENC_STAT_VFPS_CHANGE) {
            pChnVars->bAttrChange = 1;
            s32Ret = _drv_set_chn_attr(pChnHandle);
            if (s32Ret != 0) {
                DRV_VENC_ERR("drv_set_chn_attr, %d\n", s32Ret);
                return s32Ret;
            }
        } else if (s32Ret < 0) {
            DRV_VENC_ERR("drv_check_fps, %d\n", s32Ret);
            return s32Ret;
        }

        s32Ret =
            pEncCtx->base.encOnePic(pEncCtx, pstFrame, s32MilliSec);
        if (s32Ret != 0) {
            if (s32Ret == DRV_ERR_VENC_BUSY)
                DRV_VENC_WARN("chn:%d encOnePic Error, VPU is busy\n", VeChn);
            else
                DRV_VENC_ERR("chn:%d encOnePic Error, s32Ret = %d\n", VeChn, s32Ret);
            return s32Ret;
        }
    } else {
        if (pChnVars->bAttrChange == 1) {
            s32Ret = _drv_set_chn_attr(pChnHandle);
            if (s32Ret != 0) {
                DRV_VENC_ERR("drv_set_chn_attr, %d\n", s32Ret);
                return s32Ret;
            }
        }

        s32Ret = _drv_check_frc(pChnHandle);
        if (s32Ret <= 0) {
            _drv_update_frc_src(pChnHandle);
            return DRV_ERR_VENC_FRC_NO_ENC;
        }

        s32Ret =
            pEncCtx->base.encOnePic(pEncCtx, pstFrame, s32MilliSec);
        if (s32Ret != 0) {
            if (s32Ret == DRV_ERR_VENC_BUSY)
                DRV_VENC_WARN("encOnePic Error, VPU is busy\n");
            else
                DRV_VENC_ERR("encOnePic chn %d Error, s32Ret = %d\n",
                         VeChn, s32Ret);

            return s32Ret;
        }

        _drv_update_frc_dst(pChnHandle);
        _drv_update_frc_src(pChnHandle);
        _drv_check_frc_overflow(pChnHandle);
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    pChnStat->u32LeftStreamFrames++;
    if (pChnHandle->pChnAttr->stVencAttr.enType == PT_JPEG ||
            pChnHandle->pChnAttr->stVencAttr.enType == PT_MJPEG) {
        wake_up(&tVencWaitQueue[VeChn]);
    }
    pChnVars->currPTS = pstFrame->video_frame.pts;
    pEncCtx->base.u64PTS = pChnVars->currPTS;
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return s32Ret;
}


/**
 * @brief Destroy One VENC Channel.
 * @param[in] VeChn VENC Channel Number.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
int drv_venc_destroy_chn(venc_chn VeChn)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_vb_ctx *pVbCtx = NULL;
    // mmf_chn_s chn = { .mod_id = ID_VENC,
    //           .dev_id = 0,
    //           .chn_id = 0};

    if (handle == NULL || handle->chn_handle[VeChn] == NULL) {
        DRV_VENC_INFO("VdChn: %d already destoryed.\n", VeChn);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pVbCtx = pChnHandle->pVbCtx;

    base_mod_jobs_exit(&vencVbCtx[VeChn].vb_jobs);

    s32Ret = pEncCtx->base.close(pEncCtx);

    SEMA_DESTROY(&pChnVars->sem_send);
    SEMA_DESTROY(&pChnVars->sem_release);

    if (pChnVars->bHasVbPool == 1 &&
        (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264 ||
        pChnHandle->pChnAttr->stVencAttr.enType == PT_H265)) {
        //unsigned int i = 0;
        vb_source_e eVbSource;

        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
            eVbSource =
                handle->ModParam.stH264eModParam.enH264eVBSource;
        } else if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265) {
            eVbSource =
                handle->ModParam.stH265eModParam.enH265eVBSource;
        } else {
            eVbSource = VB_SOURCE_COMMON;
        }

        #ifdef USE_vb_pool
        for (i = 0; i < pChnVars->FrmNum; i++) {
            vb_blk blk;

            blk = vb_physAddr2Handle(
                pChnVars->FrmArray[i].phyAddr);
            if (blk != VB_INVALID_HANDLE)
                vb_release_block(blk);
        }

        if (eVbSource == VB_SOURCE_PRIVATE) {
            vb_destroy_pool(pChnVars->vbpool.hPicVbPool);
        }
        #endif

        pChnVars->bHasVbPool = 0;
    }

    if (pChnHandle->pChnVars) {
        MEM_FREE(pChnHandle->pChnVars);
        pChnHandle->pChnVars = NULL;
    }

    if (pChnHandle->pChnAttr) {
        MEM_FREE(pChnHandle->pChnAttr);
        pChnHandle->pChnAttr = NULL;
    }

    MUTEX_DESTROY(&pChnHandle->chnMutex);
    MUTEX_DESTROY(&pChnHandle->chnShmMutex);

    if (handle->chn_handle[VeChn]) {
        MEM_FREE(handle->chn_handle[VeChn]);
        handle->chn_handle[VeChn] = NULL;
    }

    handle->chn_status[VeChn] = venc_chn_STATE_NONE;

    {
        venc_chn i = 0;
        unsigned char bFreeVencHandle = 1;

        for (i = 0; i < VENC_MAX_CHN_NUM; i++) {
            if (handle->chn_handle[i] != NULL) {
                bFreeVencHandle = 0;
                break;
            }
        }
    }

    return s32Ret;
}

/**
 * @brief Resets a VENC channel.
 * @param[in] VeChn VENC Channel Number.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
int drv_venc_reset_chn(venc_chn VeChn)
{
    /* from Hisi,
     * You are advised to call HI_MPI_VENC_ResetChn
     * to reset the channel before switching the format of the input image
     * from non-single-component format to single-component format.
     */
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    payload_type_e enType;
    venc_chn_vars *pChnVars = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;

    if (pChnVars->chnState == venc_chn_STATE_START_ENC) {
        DRV_VENC_ERR("Chn %d is not stopped, failure\n", VeChn);
        return DRV_ERR_VENC_BUSY;
    }

    enType = pChnHandle->pChnAttr->stVencAttr.enType;
    if (enType == PT_JPEG) {
        venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
        if (pEncCtx->base.ioctl) {
            s32Ret = pEncCtx->base.ioctl(
                pEncCtx, DRV_JPEG_OP_RESET_CHN, NULL);
            if (s32Ret != 0) {
                DRV_VENC_ERR(
                    "DRV_JPEG_OP_RESET_CHN fail, s32Ret = %d\n",
                    s32Ret);
                return s32Ret;
            }
        }
    }

    // TODO: + Check resolution to decide if release memory / + re-init FW

    return s32Ret;
}

int drv_venc_set_jpeg_param(venc_chn VeChn,
                  const venc_jpeg_param_s *pstJpegParam)
{
    venc_chn_context *pChnHandle = NULL;
    venc_jpeg_param_s *pvjp = NULL;
    int s32Ret = 0;
    venc_mjpeg_fixqp_s *pstMJPEGFixQp;
    venc_rc_attr_s *prcatt;
    venc_chn_vars *pChnVars;
    venc_enc_ctx *pEncCtx;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pvjp = &pChnVars->stJpegParam;

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(
            pEncCtx, DRV_JPEG_OP_SET_MCUPerECS,
            (void *)&pstJpegParam->u32MCUPerECS);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_JPEG_OP_SET_MCUPerECS, %d\n", s32Ret);
            return -1;
        }
    }

    if (pstJpegParam->u32Qfactor > Q_TABLE_MAX) {
        DRV_VENC_ERR("u32Qfactor <= 0 || >= 100, s32Ret = %d\n",
                 s32Ret);
        return DRV_ERR_VENC_ILLEGAL_PARAM;
    }

    prcatt = &pChnHandle->pChnAttr->stRcAttr;
    pstMJPEGFixQp = &prcatt->stMjpegFixQp;

    prcatt->enRcMode = VENC_RC_MODE_MJPEGFIXQP;
    pstMJPEGFixQp->u32Qfactor = pstJpegParam->u32Qfactor;

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(
            pEncCtx, DRV_JPEG_OP_SET_QUALITY,
            (void *)pstJpegParam);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_JPEG_OP_SET_QUALITY, %d\n", s32Ret);
            return -1;
        }
    }

    memcpy(pvjp, pstJpegParam, sizeof(venc_jpeg_param_s));

    return s32Ret;
}

int drv_venc_get_jpeg_param(venc_chn VeChn, venc_jpeg_param_s *pstJpegParam)
{
    venc_chn_context *pChnHandle = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_jpeg_param_s *pvjp = NULL;
    int s32Ret = 0;
    venc_enc_ctx *pEncCtx;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;

    pvjp = &pChnVars->stJpegParam;

     if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(
            pEncCtx, DRV_JPEG_OP_GET_QUALITY,
            (void *)pvjp);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_JPEG_OP_SET_QUALITY, %d\n", s32Ret);
            return -1;
        }
    }

    memcpy(pstJpegParam, &pChnVars->stJpegParam, sizeof(venc_jpeg_param_s));

    return s32Ret;
}

int drv_venc_set_mjpeg_param(venc_chn VeChn,
                  const venc_mjpeg_param_s *pstMJpegParam)
{
    venc_chn_context *pChnHandle = NULL;
    venc_mjpeg_param_s *pvmjpe = NULL;
    int s32Ret = 0;
    venc_mjpeg_fixqp_s *pstMJPEGFixQp;
    venc_jpeg_param_s stJpegParam = {0};
    venc_rc_attr_s *prcatt;
    venc_chn_vars *pChnVars;
    venc_enc_ctx *pEncCtx;
    int i;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pvmjpe = &pChnVars->stMjpegParam;

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(
            pEncCtx, DRV_JPEG_OP_SET_MCUPerECS,
            (void *)&pstMJpegParam->u32MCUPerECS);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_JPEG_OP_SET_MCUPerECS, %d\n", s32Ret);
            return -1;
        }
    }

    prcatt = &pChnHandle->pChnAttr->stRcAttr;

    // only FixQp mod can set quality by user
    if (prcatt->enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
        pstMJPEGFixQp = &prcatt->stMjpegFixQp;

        stJpegParam.u32Qfactor = pstMJPEGFixQp->u32Qfactor;
        stJpegParam.u32MCUPerECS = pstMJpegParam->u32MCUPerECS;
        for (i = 0; i < ARRAY_SIZE(stJpegParam.u8YQt); i++) {
            stJpegParam.u8YQt[i]   = pstMJpegParam->u8YQt[i];
            stJpegParam.u8CbQt[i]  = pstMJpegParam->u8CbQt[i];
            stJpegParam.u8CrQt[i]  = pstMJpegParam->u8CrQt[i];
        }

        if (pEncCtx->base.ioctl) {
            s32Ret = pEncCtx->base.ioctl(
                pEncCtx, DRV_JPEG_OP_SET_QUALITY,
                (void *)&stJpegParam);
            if (s32Ret != 0) {
                DRV_VENC_ERR("DRV_JPEG_OP_SET_QUALITY, %d\n", s32Ret);
                return -1;
            }
        }
    }

    memcpy(pvmjpe, pstMJpegParam, sizeof(venc_mjpeg_param_s));

    return s32Ret;
}

int drv_venc_get_mjpeg_param(venc_chn VeChn, venc_mjpeg_param_s *pstMJpegParam)
{
    venc_chn_context *pChnHandle = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_mjpeg_param_s *pvmjpe = NULL;
    venc_jpeg_param_s stJpegParam = {0};
    int s32Ret = 0;
    venc_enc_ctx *pEncCtx;
    int i;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;

    pvmjpe = &pChnVars->stMjpegParam;

     if (pEncCtx->base.ioctl) {
         s32Ret = pEncCtx->base.ioctl(
             pEncCtx, DRV_JPEG_OP_GET_QUALITY,
             (void *)&stJpegParam);
         if (s32Ret != 0) {
             DRV_VENC_ERR("DRV_JPEG_OP_SET_QUALITY, %d\n", s32Ret);
             return -1;
         }
     }

     for (i = 0; i < ARRAY_SIZE(stJpegParam.u8YQt); i++) {
         pvmjpe->u8YQt[i]   = stJpegParam.u8YQt[i];
         pvmjpe->u8CbQt[i]  = stJpegParam.u8CbQt[i];
         pvmjpe->u8CrQt[i]  = stJpegParam.u8CrQt[i];
     }

    memcpy(pstMJpegParam, &pChnVars->stMjpegParam, sizeof(venc_mjpeg_param_s));

    return s32Ret;
}

int drv_venc_request_idr(venc_chn VeChn, unsigned char bInstant)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    payload_type_e enType;
    venc_enc_ctx *pEncCtx;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    pEncCtx = &pChnHandle->encCtx;

    if (enType == PT_H264 || enType == PT_H265) {
        if (pEncCtx->base.ioctl) {
            if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
                DRV_VENC_ERR("can not lock chnMutex\n");
                return -1;
            }
            s32Ret =
                pEncCtx->base.ioctl(pEncCtx,
                            DRV_H26X_OP_SET_REQUEST_IDR,
                            (void *)&bInstant);
            MUTEX_UNLOCK(&pChnHandle->chnMutex);
            if (s32Ret != 0) {
                DRV_VENC_ERR(
                    "DRV_H26X_OP_SET_REQUEST_IDR, %d\n",
                    s32Ret);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}

int drv_venc_enable_idr(venc_chn VeChn, unsigned char bEnableIDR)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    payload_type_e enType;
    venc_enc_ctx *pEncCtx;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;
    pEncCtx = &pChnHandle->encCtx;

    if (enType == PT_H264 || enType == PT_H265) {
        if (pEncCtx->base.ioctl) {
            if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
                DRV_VENC_ERR("can not lock chnMutex\n");
                return -1;
            }
            s32Ret =
                pEncCtx->base.ioctl(pEncCtx,
                            DRV_H26X_OP_SET_ENABLE_IDR,
                            (void *)&bEnableIDR);
            MUTEX_UNLOCK(&pChnHandle->chnMutex);
            if (s32Ret != 0) {
                DRV_VENC_ERR(
                    "DRV_H26X_OP_SET_REQUEST_IDR, %d\n",
                    s32Ret);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}


int drv_venc_set_chn_attr(venc_chn VeChn, const venc_chn_attr_s *pstChnAttr)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_rc_attr_s *pstRcAttr = NULL;
    venc_attr_s *pstVencAttr = NULL;

    if (pstChnAttr == NULL) {
        DRV_VENC_ERR("pstChnAttr == NULL\n");
        return DRV_ERR_VENC_ILLEGAL_PARAM;
    }

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    if ((pChnHandle->pChnAttr == NULL) || (pChnHandle->pChnVars == NULL)) {
        DRV_VENC_ERR("pChnAttr or pChnVars is NULL\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    if (pstChnAttr->stVencAttr.enType !=
        pChnHandle->pChnAttr->stVencAttr.enType) {
        DRV_VENC_ERR("enType is incorrect\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    pstRcAttr = &(pChnHandle->pChnAttr->stRcAttr);
    pstVencAttr = &(pChnHandle->pChnAttr->stVencAttr);

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    memcpy(pstRcAttr, &pstChnAttr->stRcAttr, sizeof(venc_rc_attr_s));
    memcpy(pstVencAttr, &pstChnAttr->stVencAttr, sizeof(venc_attr_s));
    pChnHandle->pChnVars->bAttrChange = 1;
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return s32Ret;
}

int drv_venc_get_chn_attr(venc_chn VeChn, venc_chn_attr_s *pstChnAttr)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    memcpy(pstChnAttr, pChnHandle->pChnAttr, sizeof(venc_chn_attr_s));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return s32Ret;
}

int drv_venc_get_rc_param(venc_chn VeChn, venc_rc_param_s *pstRcParam)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(pstRcParam, &pChnHandle->rcParam, sizeof(venc_rc_param_s));

    return s32Ret;
}

int drv_venc_set_rc_param(venc_chn VeChn, const venc_rc_param_s *pstRcParam)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_chn_attr_s *pChnAttr;
    venc_attr_s *pVencAttr;
    venc_rc_param_s *prcparam;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnAttr = pChnHandle->pChnAttr;
    pVencAttr = &pChnAttr->stVencAttr;
    prcparam = &pChnHandle->rcParam;

    s32Ret = _drv_check_rc_param(pChnHandle, pstRcParam);
    if (s32Ret != 0) {
        DRV_VENC_ERR("_drv_check_rc_param\n");
        return s32Ret;
    }

    memcpy(prcparam, pstRcParam, sizeof(venc_rc_param_s));

    if (pVencAttr->enType == PT_H264 || pVencAttr->enType == PT_H265) {
        prcparam->s32FirstFrameStartQp =
            (prcparam->s32FirstFrameStartQp < 0 ||
             prcparam->s32FirstFrameStartQp > 51) ?
                      ((pVencAttr->enType == PT_H265) ? 63 :
                                    DEF_IQP) :
                      prcparam->s32FirstFrameStartQp;

        if (pstRcParam->s32FirstFrameStartQp !=
            prcparam->s32FirstFrameStartQp) {
            DRV_VENC_DBG(
                "pstRcParam 1stQp = %d, prcparam 1stQp = %d\n",
                pstRcParam->s32FirstFrameStartQp,
                prcparam->s32FirstFrameStartQp);
        }
    }

    s32Ret = _drv_set_rcparam_to_drv(pChnHandle);
    if (s32Ret < 0) {
        DRV_VENC_ERR("drv_set_rcparam_to_drv\n");
        return s32Ret;
    }

    return s32Ret;
}

int drv_venc_get_ref_param(venc_chn VeChn, venc_ref_param_s *pstRefParam)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(pstRefParam, &pChnHandle->refParam, sizeof(venc_ref_param_s));

    return s32Ret;
}

int drv_venc_set_ref_param(venc_chn VeChn,
                 const venc_ref_param_s *pstRefParam)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(&pChnHandle->refParam, pstRefParam, sizeof(venc_ref_param_s));

    pEncCtx = &pChnHandle->encCtx;

    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (pEncCtx->base.ioctl && (enType == PT_H264 || enType == PT_H265)) {
        unsigned int tempLayer = 1;

        if (pstRefParam->u32Base == 1 && pstRefParam->u32Enhance == 1 &&
            pstRefParam->bEnablePred == 1)
            tempLayer = 2;
        else if (pstRefParam->u32Base == 2 &&
             pstRefParam->u32Enhance == 1 &&
             pstRefParam->bEnablePred == 1)
            tempLayer = 3;

        s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_REF_PARAM,
                         (void *)&tempLayer);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_H26X_OP_SET_REF_PARAM, %d\n", s32Ret);
            return -1;
        }
    }

    return s32Ret;
}

int drv_venc_set_cu_prediction(venc_chn VeChn,
                 const venc_cu_prediction_s *pstCuPrediction)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_cu_prediction_s *pcup;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pcup = &pChnHandle->pChnVars->cuPrediction;

    s32Ret = (pstCuPrediction->u32IntraCost <= DRV_H26X_INTRACOST_MAX) ?
        0 : DRV_ERR_VENC_CU_PREDICTION;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32IntraCost",
            pstCuPrediction->u32IntraCost);
        return s32Ret;
    }

    memcpy(pcup, pstCuPrediction, sizeof(venc_cu_prediction_s));

    s32Ret = _drv_set_cupred_impl(pChnHandle);
    if (s32Ret < 0) {
        DRV_VENC_ERR("_drv_set_cupred_impl\n");
        return s32Ret;
    }

    return s32Ret;
}


int drv_venc_get_cu_prediction(venc_chn VeChn,
                 venc_cu_prediction_s *pstCuPrediction)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_cu_prediction_s *pcup;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pcup = &pChnHandle->pChnVars->cuPrediction;

    memcpy(pstCuPrediction, pcup, sizeof(venc_cu_prediction_s));

    return s32Ret;
}

int drv_venc_get_roi_attr(venc_chn VeChn, unsigned int u32Index,
                venc_roi_attr_s *pstRoiAttr)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    pEncCtx = &pChnHandle->encCtx;

    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (pEncCtx->base.ioctl && (enType == PT_H264 || enType == PT_H265)) {
        RoiParam RoiParam;

        memset(&RoiParam, 0x0, sizeof(RoiParam));
        RoiParam.roi_index = u32Index;
        s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_GET_ROI_PARAM,
                         (void *)&RoiParam);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_H26X_OP_SET_ROI_PARAM, %d\n", s32Ret);
            return -1;
        }
        pstRoiAttr->u32Index = RoiParam.roi_index;
        pstRoiAttr->bEnable = RoiParam.roi_enable_flag;
        pstRoiAttr->bAbsQp = RoiParam.is_absolute_qp;
        pstRoiAttr->s32Qp = RoiParam.roi_qp;
        pstRoiAttr->stRect.x = RoiParam.roi_rect_x;
        pstRoiAttr->stRect.y = RoiParam.roi_rect_y;
        pstRoiAttr->stRect.width = RoiParam.roi_rect_width;
        pstRoiAttr->stRect.height = RoiParam.roi_rect_height;
    }

    return s32Ret;
}

int drv_venc_set_framelost_strategy(venc_chn VeChn,
                      const venc_framelost_s *pstFrmLostParam)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    payload_type_e enType;
    venc_enc_ctx *pEncCtx;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    enType = pChnHandle->pChnAttr->stVencAttr.enType;
    pEncCtx = &pChnHandle->encCtx;

    if (enType == PT_H264 || enType == PT_H265) {
        if (pEncCtx->base.ioctl) {
            FrameLost frameLostSetting;

            if (pstFrmLostParam->bFrmLostOpen) {
                // If frame lost is on, check parameters
                if (pstFrmLostParam->enFrmLostMode !=
                    FRMLOST_PSKIP) {
                    DRV_VENC_ERR(
                        "frame lost mode = %d, unsupport\n",
                        (int)(pstFrmLostParam
                                  ->enFrmLostMode));
                    return DRV_ERR_VENC_NOT_SUPPORT;
                }

                frameLostSetting.frameLostMode =
                    pstFrmLostParam->enFrmLostMode;
                frameLostSetting.u32EncFrmGaps =
                    pstFrmLostParam->u32EncFrmGaps;
                frameLostSetting.bFrameLostOpen =
                    pstFrmLostParam->bFrmLostOpen;
                frameLostSetting.u32FrmLostBpsThr =
                    pstFrmLostParam->u32FrmLostBpsThr;
            } else {
                // set gap and threshold back to 0
                frameLostSetting.frameLostMode =
                    pstFrmLostParam->enFrmLostMode;
                frameLostSetting.u32EncFrmGaps = 0;
                frameLostSetting.bFrameLostOpen = 0;
                frameLostSetting.u32FrmLostBpsThr = 0;
            }
            DRV_VENC_DBG(
                "bFrameLostOpen = %d, frameLostMode = %d, u32EncFrmGaps = %d, u32FrmLostBpsThr = %d\n",
                frameLostSetting.bFrameLostOpen,
                frameLostSetting.frameLostMode,
                frameLostSetting.u32EncFrmGaps,
                frameLostSetting.u32FrmLostBpsThr);
            memcpy(&pChnHandle->pChnVars->frameLost,
                   pstFrmLostParam, sizeof(venc_framelost_s));

            if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
                DRV_VENC_ERR("can not lock chnMutex\n");
                return -1;
            }
            s32Ret = pEncCtx->base.ioctl(
                pEncCtx, DRV_H26X_OP_SET_FRAME_LOST_STRATEGY,
                (void *)(&frameLostSetting));
            MUTEX_UNLOCK(&pChnHandle->chnMutex);
            if (s32Ret != 0) {
                DRV_VENC_ERR(
                    "DRV_H26X_OP_SET_FRAME_LOST_STRATEGY, %d\n",
                    s32Ret);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}

int drv_venc_set_roi_attr(venc_chn VeChn, const venc_roi_attr_s *pstRoiAttr)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    pEncCtx = &pChnHandle->encCtx;

    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (pEncCtx->base.ioctl && (enType == PT_H264 || enType == PT_H265)) {
        RoiParam RoiParam;

        if (pChnHandle->pChnAttr->stRcAttr.enRcMode ==
            VENC_RC_MODE_H265FIXQP) {
            DRV_VENC_ERR(
                "[H265]ROI does not support FIX QP mode.\n");
            return -1;
        }
        memset(&RoiParam, 0x0, sizeof(RoiParam));
        RoiParam.roi_index = pstRoiAttr->u32Index;
        RoiParam.roi_enable_flag = pstRoiAttr->bEnable;
        RoiParam.is_absolute_qp = pstRoiAttr->bAbsQp;
        RoiParam.roi_qp = pstRoiAttr->s32Qp;
        RoiParam.roi_rect_x = pstRoiAttr->stRect.x;
        RoiParam.roi_rect_y = pstRoiAttr->stRect.y;
        RoiParam.roi_rect_width = pstRoiAttr->stRect.width;
        RoiParam.roi_rect_height = pstRoiAttr->stRect.height;

        s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_ROI_PARAM,
                         (void *)&RoiParam);
        if (s32Ret != 0) {
            DRV_VENC_ERR("DRV_H26X_OP_SET_ROI_PARAM, %d\n", s32Ret);
            return -1;
        }

        if (MUTEX_LOCK(&pChnHandle->chnShmMutex) != 0) {
            DRV_VENC_ERR("can not lock chnShmMutex\n");
            return -1;
        }
        memcpy(&pChnHandle->pChnVars->stRoiAttr[pstRoiAttr->u32Index],
               pstRoiAttr, sizeof(venc_roi_attr_s));
        MUTEX_UNLOCK(&pChnHandle->chnShmMutex);
    }

    return s32Ret;
}

int drv_venc_get_framelost_strategy(venc_chn VeChn,
                      venc_framelost_s *pstFrmLostParam)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    if (pChnHandle->pChnVars) {
        memcpy(pstFrmLostParam, &pChnHandle->pChnVars->frameLost,
               sizeof(venc_framelost_s));
    } else {
        s32Ret = DRV_ERR_VENC_NOT_CONFIG;
    }

    return s32Ret;
}

int drv_venc_set_chn_param(venc_chn VeChn,
                 const venc_chn_param_s *pstChnParam)
{
    int s32Ret = 0;
    venc_chn_context *pChnHandle;
    venc_chn_vars *pChnVars;
    rect_s *prect;
    venc_enc_ctx_base *pvecb;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    prect = &pChnVars->stChnParam.stCropCfg.stRect;
    pvecb = &pChnHandle->encCtx.base;

    memcpy(&pChnVars->stChnParam, pstChnParam, sizeof(venc_chn_param_s));
    pvecb->x = prect->x;
    pvecb->y = prect->y;

    if (prect->x & 0xF)
        pvecb->x &= (~0xF);

    if (prect->y & 0xF)
        pvecb->y &= (~0xF);

    pvecb->width = prect->width;
    pvecb->height = prect->height;
    return s32Ret;
}

int drv_venc_get_chn_param(venc_chn VeChn, venc_chn_param_s *pstChnParam)
{
    int s32Ret = 0;
    venc_chn_param_s *pvcp;
    venc_chn_context *pChnHandle;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pvcp = &pChnHandle->pChnVars->stChnParam;

    memcpy(pstChnParam, pvcp, sizeof(venc_chn_param_s));

    return s32Ret;
}

int drv_venc_insert_userdata(venc_chn VeChn, unsigned char *pu8Data, unsigned int u32Len)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pu8Data == NULL || u32Len == 0) {
        s32Ret = -2;
        DRV_VENC_ERR("no user data\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    if (enType == PT_JPEG || enType == PT_MJPEG) {
        s32Ret = _drv_venc_jpeg_insert_userdata(pChnHandle, pu8Data, u32Len);
    } else if (enType == PT_H264 || enType == PT_H265) {
        s32Ret = _drv_venc_h26x_insert_userdata(pChnHandle, pu8Data, u32Len);
    }

    if (s32Ret != 0)
        DRV_VENC_ERR("failed to insert user data %d", s32Ret);
    return s32Ret;
}


int drv_venc_set_h264_entropy(venc_chn VeChn,
                const venc_h264_entropy_s *pstH264EntropyEnc)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_attr_s *pVencAttr;
    venc_enc_ctx *pEncCtx;
    venc_h264_entropy_s h264Entropy;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264EntropyEnc == NULL) {
        DRV_VENC_ERR("no user data\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pVencAttr = &pChnHandle->pChnAttr->stVencAttr;
    pEncCtx = &pChnHandle->encCtx;

    if (pVencAttr->enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    if (pVencAttr->u32Profile == H264E_PROFILE_BASELINE) {
        if (pstH264EntropyEnc->u32EntropyEncModeI !=
                H264E_ENTROPY_CAVLC ||
            pstH264EntropyEnc->u32EntropyEncModeP !=
                H264E_ENTROPY_CAVLC) {
            DRV_VENC_ERR(
                "h264 Baseline Profile only supports CAVLC\n");
            return DRV_ERR_VENC_NOT_SUPPORT;
        }
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    memcpy(&h264Entropy, pstH264EntropyEnc, sizeof(venc_h264_entropy_s));
    s32Ret = _drv_venc_h264_set_entropy(pChnHandle, &h264Entropy);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h264Entropy, &h264Entropy,
               sizeof(venc_h264_entropy_s));
    } else {
        DRV_VENC_ERR("failed to set h264 entropy %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H264_ENTROPY;
    }

    return s32Ret;
}

int drv_venc_get_h264_entropy(venc_chn VeChn,
                venc_h264_entropy_s *pstH264EntropyEnc)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264EntropyEnc == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h264 entropy param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    memcpy(pstH264EntropyEnc, &pChnHandle->h264Entropy,
           sizeof(venc_h264_entropy_s));
    return 0;
}

int drv_venc_set_h264_trans(venc_chn VeChn,
                  const venc_h264_trans_s *pstH264Trans)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h264_trans_s h264Trans;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Trans == NULL) {
        DRV_VENC_ERR("no h264 trans param\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    memcpy(&h264Trans, pstH264Trans, sizeof(venc_h264_trans_s));
    s32Ret = _drv_venc_h264_set_trans(pChnHandle, &h264Trans);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h264Trans, &h264Trans,
               sizeof(venc_h264_trans_s));
    } else {
        DRV_VENC_ERR("failed to set h264 trans %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H264_TRANS;
    }

    return s32Ret;
}

int drv_venc_get_h264_trans(venc_chn VeChn, venc_h264_trans_s *pstH264Trans)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Trans == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h264 trans param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    memcpy(pstH264Trans, &pChnHandle->h264Trans, sizeof(venc_h264_trans_s));
    return 0;
}

int drv_venc_set_h265_predunit(venc_chn VeChn, const venc_h265_pu_s *pstPredUnit)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h265_pu_s h265PredUnit;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstPredUnit == NULL) {
        DRV_VENC_ERR("no h265 PredUnit\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    memcpy(&h265PredUnit, pstPredUnit, sizeof(venc_h265_pu_s));
    s32Ret = _drv_venc_h265_set_predunit(pChnHandle, &h265PredUnit);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h265PredUnit, &h265PredUnit,
               sizeof(venc_h265_pu_s));
    } else {
        DRV_VENC_ERR("failed to set h265 PredUnit %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H265_PRED_UNIT;
    }

    return s32Ret;
}

int drv_venc_get_h265_predunit(venc_chn VeChn, venc_h265_pu_s *pstPredUnit)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstPredUnit == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h265 PredUnit\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    s32Ret = _drv_venc_h265_get_predunit(pChnHandle, &pChnHandle->h265PredUnit);

    if (s32Ret == 0) {
        memcpy(pstPredUnit, &pChnHandle->h265PredUnit, sizeof(venc_h265_pu_s));
    } else {
        DRV_VENC_ERR("failed to get h265 PredUnit %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H265_PRED_UNIT;
    }

    return 0;
}

int drv_venc_set_h265_trans(venc_chn VeChn,
                  const venc_h265_trans_s *pstH265Trans)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h265_trans_s h265Trans;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Trans == NULL) {
        DRV_VENC_ERR("no h265 trans param\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    memcpy(&h265Trans, pstH265Trans, sizeof(venc_h264_trans_s));
    s32Ret = _drv_venc_h265_set_trans(pChnHandle, &h265Trans);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h265Trans, &h265Trans,
               sizeof(venc_h265_trans_s));
    } else {
        DRV_VENC_ERR("failed to set h265 trans %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H265_TRANS;
    }

    return s32Ret;
}

int drv_venc_get_h265_trans(venc_chn VeChn, venc_h265_trans_s *pstH265Trans)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Trans == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h265 trans param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    memcpy(pstH265Trans, &pChnHandle->h265Trans, sizeof(venc_h265_trans_s));
    return 0;
}

int
drv_venc_set_superframe_strategy(venc_chn VeChn,
                   const venc_superframe_cfg_s *pstSuperFrmParam)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_chn_vars *pChnVars = NULL;
    venc_enc_ctx *pEncCtx;
    SuperFrame super, *pSuper = &super;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstSuperFrmParam == NULL) {
        DRV_VENC_ERR("pstSuperFrmParam = NULL\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;
    pSuper->enSuperFrmMode =
        (pstSuperFrmParam->enSuperFrmMode == SUPERFRM_NONE) ?
                  DRV_SUPERFRM_NONE :
                  DRV_SUPERFRM_REENCODE_IDR;
    pSuper->u32SuperIFrmBitsThr = pstSuperFrmParam->u32SuperIFrmBitsThr;
    pSuper->u32SuperPFrmBitsThr = pstSuperFrmParam->u32SuperPFrmBitsThr;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_SUPER_FRAME,
                     (void *)pSuper);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == 0) {
        memcpy(&pChnVars->stSuperFrmParam, pstSuperFrmParam,
               sizeof(venc_superframe_cfg_s));
    }

    return s32Ret;
}

int drv_venc_get_superframe_strategy(venc_chn VeChn,
                       venc_superframe_cfg_s *pstSuperFrmParam)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_chn_vars *pChnVars = NULL;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstSuperFrmParam == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("pstSuperFrmParam = NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;

    memcpy(pstSuperFrmParam, &pChnVars->stSuperFrmParam,
           sizeof(venc_superframe_cfg_s));
    return 0;
}

int drv_venc_calc_frame_param(venc_chn VeChn,
                venc_frame_param_s *pstFrameParam)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    FrameParam frmp, *pfp = &frmp;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstFrameParam == NULL) {
        DRV_VENC_ERR("pstFrameParam is NULL\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_CALC_FRAME_PARAM,
                     (void *)(pfp));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret != 0) {
        DRV_VENC_ERR("failed to calculate FrameParam %d", s32Ret);
        s32Ret = DRV_ERR_VENC_FRAME_PARAM;
    }

    pstFrameParam->u32FrameQp = pfp->u32FrameQp;
    pstFrameParam->u32FrameBits = pfp->u32FrameBits;
    return s32Ret;
}

int drv_venc_set_frame_param(venc_chn VeChn,
                   const venc_frame_param_s *pstFrameParam)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    venc_chn_vars *pChnVars;
    FrameParam frmp, *pfp = &frmp;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstFrameParam == NULL) {
        DRV_VENC_ERR("pstFrameParam is NULL\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    pfp->u32FrameQp = pstFrameParam->u32FrameQp;
    pfp->u32FrameBits = pstFrameParam->u32FrameBits;

    s32Ret = (pfp->u32FrameQp <= DRV_H26X_FRAME_QP_MAX) ?
        0 : DRV_ERR_VENC_ILLEGAL_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32FrameQp",
            pfp->u32FrameQp);
        return s32Ret;
    }
    s32Ret = ((pfp->u32FrameBits >= DRV_H26X_FRAME_BITS_MIN) &&
        (pfp->u32FrameBits <= DRV_H26X_FRAME_BITS_MAX)) ?
        0 : DRV_ERR_VENC_ILLEGAL_PARAM;
    if (s32Ret != 0) {
        DRV_VENC_ERR("error, %s = %d\n",
            "u32FrameBits", pfp->u32FrameBits);
        return s32Ret;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_FRAME_PARAM,
                     (void *)(pfp));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == 0) {
        memcpy(&pChnVars->frameParam, pstFrameParam,
               sizeof(venc_frame_param_s));
    } else {
        DRV_VENC_ERR("failed to set FrameParam %d", s32Ret);
        s32Ret = DRV_ERR_VENC_FRAME_PARAM;
    }

    return s32Ret;
}

int drv_venc_get_frame_param(venc_chn VeChn,
                   venc_frame_param_s *pstFrameParam)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_chn_vars *pChnVars;
    venc_frame_param_s *pfp;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstFrameParam == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pfp = &pChnVars->frameParam;

    memcpy(pstFrameParam, pfp, sizeof(venc_frame_param_s));

    return s32Ret;
}


int drv_venc_set_h264_vui(venc_chn VeChn, const venc_h264_vui_s *pstH264Vui)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h264_vui_s h264Vui;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Vui == NULL) {
        DRV_VENC_ERR("no h264 vui param\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return -1;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    s32Ret = _drv_check_h264_vui_param(pstH264Vui);
    if (s32Ret != 0) {
        DRV_VENC_ERR("invalid h264 vui param, %d\n", s32Ret);
        return s32Ret;
    }

    memcpy(&h264Vui, pstH264Vui, sizeof(venc_h264_vui_s));
    s32Ret = _drv_set_h264_vui(pChnHandle, &h264Vui);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h264Vui, &h264Vui, sizeof(venc_h264_vui_s));
    } else {
        DRV_VENC_ERR("failed to set h264 vui %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H264_VUI;
    }

    return s32Ret;
}

int drv_venc_set_h265_vui(venc_chn VeChn, const venc_h265_vui_s *pstH265Vui)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h265_vui_s h265Vui;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Vui == NULL) {
        DRV_VENC_ERR("no h265 vui param\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return -1;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    s32Ret = _drv_check_h265_vui_param(pstH265Vui);
    if (s32Ret != 0) {
        DRV_VENC_ERR("invalid h265 vui param, %d\n", s32Ret);
        return s32Ret;
    }

    memcpy(&h265Vui, pstH265Vui, sizeof(venc_h265_vui_s));
    s32Ret = _drv_set_h265_vui(pChnHandle, &h265Vui);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h265Vui, &h265Vui, sizeof(venc_h265_vui_s));
    } else {
        DRV_VENC_ERR("failed to set h265 vui %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H265_VUI;
    }

    return s32Ret;
}

int drv_venc_get_h264_vui(venc_chn VeChn, venc_h264_vui_s *pstH264Vui)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Vui == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h264 vui param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return -1;
    }

    memcpy(pstH264Vui, &pChnHandle->h264Vui, sizeof(venc_h264_vui_s));
    return 0;
}

int drv_venc_get_h265_vui(venc_chn VeChn, venc_h265_vui_s *pstH265Vui)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Vui == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h265 vui param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return -1;
    }

    memcpy(pstH265Vui, &pChnHandle->h265Vui, sizeof(venc_h265_vui_s));
    return 0;
}

int drv_venc_set_h264_slicesplit(venc_chn VeChn, const venc_h264_slice_split_s *pstSliceSplit)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h264_slice_split_s h264Split;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstSliceSplit == NULL) {
        DRV_VENC_ERR("no h264 split param\n");
        return -2;
    }

    if (pstSliceSplit->bSplitEnable == 1
            && pstSliceSplit->u32MbLineNum == 0) {
        DRV_VENC_ERR("invalid h264 split param\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return -1;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    memcpy(&h264Split, pstSliceSplit, sizeof(venc_h264_slice_split_s));
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H264_SPLIT,
                     (void *)pstSliceSplit);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h264Split, &h264Split, sizeof(venc_h264_slice_split_s));
    } else {
        DRV_VENC_ERR("failed to set h264 split %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H264_SPLIT;
    }

    return s32Ret;
}

int drv_venc_get_h264_slicesplit(venc_chn VeChn, venc_h264_slice_split_s *pstH264Split)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Split == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h264 split param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return -1;
    }

    memcpy(pstH264Split, &pChnHandle->h264Split, sizeof(venc_h264_slice_split_s));
    return 0;
}

int drv_venc_set_h265_slicesplit(venc_chn VeChn, const venc_h265_slice_split_s *pstSliceSplit)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h265_slice_split_s h265Split;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstSliceSplit == NULL) {
        DRV_VENC_ERR("no h265 split param\n");
        return -2;
    }

    if (pstSliceSplit->bSplitEnable == 1
            && pstSliceSplit->u32LcuLineNum == 0) {
        DRV_VENC_ERR("invalid h265 split param\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return -1;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    memcpy(&h265Split, pstSliceSplit, sizeof(venc_h265_slice_split_s));
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H265_SPLIT,
                     (void *)pstSliceSplit);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h265Split, &h265Split, sizeof(venc_h265_slice_split_s));
    } else {
        DRV_VENC_ERR("failed to set h265 split %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H265_SPLIT;
    }

    return s32Ret;
}

int drv_venc_get_h265_slicesplit(venc_chn VeChn, venc_h265_slice_split_s *pstH265Split)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Split == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h265 split param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return -1;
    }

    memcpy(pstH265Split, &pChnHandle->h265Split, sizeof(venc_h265_slice_split_s));
    return 0;
}

int drv_venc_set_h264_dblk(venc_chn VeChn, const venc_h264_dblk_s *pstH264Dblk)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h264_dblk_s h264Dblk;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Dblk == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("pstH264Dblk is NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return -1;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    memcpy(&h264Dblk, pstH264Dblk, sizeof(venc_h264_dblk_s));
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H264_DBLK, (void *)(pstH264Dblk));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);
    if (s32Ret == 0) {
        memcpy(&pChnHandle->h264Dblk, pstH264Dblk, sizeof(venc_h264_dblk_s));
    } else {
        DRV_VENC_ERR("failed to set h264 dblk %d", s32Ret);
    }

    return s32Ret;
}

int drv_venc_get_h264_dblk(venc_chn VeChn, venc_h264_dblk_s *pstH264Dblk)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Dblk == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("pstH264Dblk is NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(pstH264Dblk, &pChnHandle->h264Dblk, sizeof(venc_h264_dblk_s));
    return 0;
}

int drv_venc_set_h265_dblk(venc_chn VeChn, const venc_h265_dblk_s *pstH265Dblk)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Dblk == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("pstH265Dblk is NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return -1;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H265_DBLK, (void *)(pstH265Dblk));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);
    if (s32Ret == 0) {
        memcpy(&pChnHandle->h265Dblk, pstH265Dblk, sizeof(venc_h265_dblk_s));
    } else {
        DRV_VENC_ERR("failed to set h265 dblk %d", s32Ret);
    }

    return s32Ret;
}

int drv_venc_get_h265_dblk(venc_chn VeChn, venc_h265_dblk_s *pstH265Dblk)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Dblk == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("pstH265Dblk is NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(pstH265Dblk, &pChnHandle->h265Dblk, sizeof(venc_h265_dblk_s));
    return 0;
}

int drv_venc_set_h264_intrapred(venc_chn VeChn, const venc_h264_intra_pred_s *pstH264IntraPred)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h264_intra_pred_s h264IntraPred;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264IntraPred == NULL) {
        DRV_VENC_ERR("no h264 intra pred param\n");
        return -2;
    }

    if (pstH264IntraPred->constrained_intra_pred_flag != 0 &&
            pstH264IntraPred->constrained_intra_pred_flag != 1) {
        DRV_VENC_ERR("intra pred param invalid:%u\n"
                    , pstH264IntraPred->constrained_intra_pred_flag);
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return -1;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    memcpy(&h264IntraPred, pstH264IntraPred, sizeof(venc_h264_intra_pred_s));
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H264_INTRA_PRED,
                     (void *)pstH264IntraPred);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h264IntraPred, &h264IntraPred, sizeof(venc_h264_intra_pred_s));
    } else {
        DRV_VENC_ERR("failed to set intra pred %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H264_INTRA_PRED;
    }

    return s32Ret;
}

int drv_venc_get_h264_intrapred(venc_chn VeChn, venc_h264_intra_pred_s *pstH264IntraPred)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264IntraPred == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h264 intra pred param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        DRV_VENC_ERR("not h264 encode channel\n");
        return -1;
    }

    memcpy(pstH264IntraPred, &pChnHandle->h264IntraPred, sizeof(venc_h264_intra_pred_s));
    return 0;
}

int drv_venc_set_custommap(venc_chn VeChn, const venc_custom_map_s *pstVeCustomMap)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstVeCustomMap == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no custom map param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264 && enType != PT_H265) {
        DRV_VENC_ERR("custom map only support h264/h265\n");
        return -1;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_CUSTOM_MAP,
                     (void *)pstVeCustomMap);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return 0;
}

int drv_venc_get_intialInfo(venc_chn VeChn, venc_initial_info_s *pstVencInitialInfo)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    VencIntialInfo stEncIntialInfo;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstVencInitialInfo == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no enc intial param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264 && enType != PT_H265) {
        DRV_VENC_ERR("get enc intinial info only support h264/h265\n");
        return -1;
    }

    memset(&stEncIntialInfo, 0x0, sizeof(VencIntialInfo));
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_GET_INITIAL_INFO,
                        (void *)&stEncIntialInfo);
    if (s32Ret != 0) {
        DRV_VENC_ERR("DRV_H26X_OP_GET_ROI_PARAM, %d\n", s32Ret);
        return -1;
    }

    memcpy(pstVencInitialInfo, &stEncIntialInfo, sizeof(VencIntialInfo));
    return s32Ret;
}

int drv_venc_set_h265_sao(venc_chn VeChn, const venc_h265_sao_s *pstH265Sao)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_h265_sao_s h265Sao;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Sao == NULL) {
        DRV_VENC_ERR("no h265 sao param\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return -1;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    memcpy(&h265Sao, pstH265Sao, sizeof(venc_h265_sao_s));

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_H265_SAO,
                  (void *)(pstH265Sao));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->h265Sao, &h265Sao, sizeof(venc_h265_sao_s));
    } else {
        DRV_VENC_ERR("failed to set h265 sao %d", s32Ret);
        s32Ret = DRV_ERR_VENC_H265_SAO;
    }

    return s32Ret;
}

int drv_venc_get_h265_sao(venc_chn VeChn, venc_h265_sao_s *pstH265Sao)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Sao == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no h265 sao param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return -1;
    }

    memcpy(pstH265Sao, &pChnHandle->h265Sao, sizeof(venc_h265_sao_s));
    return 0;
}

int drv_venc_get_encode_header(venc_chn VeChn, venc_encode_header_s *pstEncodeHeader)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstEncodeHeader == NULL) {
        DRV_VENC_ERR("invalid input param\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265 && enType != PT_H264) {
        DRV_VENC_ERR("not h26x encode channel\n");
        return -1;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }

    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_GET_ENCODE_HEADER,
                  (void *)(pstEncodeHeader));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == 0) {
        // memcpy(&pChnHandle->h265Sao, &h265Sao, sizeof(venc_h265_sao_s));
    } else {
        DRV_VENC_ERR("failed to get encode header %d", s32Ret);
        // s32Ret = DRV_ERR_VENC_H265_SAO;
    }

    return s32Ret;
}

int drv_venc_set_search_window(venc_chn VeChn, const venc_search_window_s *pstVencSearchWindow)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;
    venc_search_window_s stVencSearchWindow;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstVencSearchWindow == NULL) {
        DRV_VENC_ERR("no search window\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    memcpy(&stVencSearchWindow, pstVencSearchWindow, sizeof(venc_search_window_s));
    s32Ret = _drv_venc_set_searchwindow(pChnHandle, &stVencSearchWindow);

    if (s32Ret == 0) {
        memcpy(&pChnHandle->stSearchWindow, &stVencSearchWindow,
               sizeof(venc_search_window_s));
    } else {
        DRV_VENC_ERR("failed to set search window %d", s32Ret);
        s32Ret = DRV_ERR_VENC_SEARCH_WINDOW;
    }

    return s32Ret;
}

int drv_venc_get_search_window(venc_chn VeChn, venc_search_window_s *pstVencSearchWindow)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    payload_type_e enType;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstVencSearchWindow == NULL) {
        s32Ret = -2;
        DRV_VENC_ERR("no search window\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265 ) {
        DRV_VENC_ERR("not h265 encode channel\n");
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    s32Ret = _drv_venc_get_searchwindow(pChnHandle, &pChnHandle->stSearchWindow);

    if (s32Ret == 0) {
        memcpy(pstVencSearchWindow, &pChnHandle->stSearchWindow, sizeof(venc_search_window_s));
    } else {
        DRV_VENC_ERR("failed to get search window %d", s32Ret);
        s32Ret = DRV_ERR_VENC_SEARCH_WINDOW;
    }

    return s32Ret;
}

int drv_venc_set_extern_buf(venc_chn VeChn, const venc_extern_buf_s *pstExternBuf)
{
    int s32Ret = -1;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;

    s32Ret = _check_venc_chn_handle(VeChn);
    if (s32Ret != 0) {
        DRV_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstExternBuf == NULL) {
        DRV_VENC_ERR("invalid param\n");
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;

    if (!pEncCtx->base.ioctl) {
        DRV_VENC_ERR("base.ioctl is NULL\n");
        return -1;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        DRV_VENC_ERR("can not lock chnMutex\n");
        return -1;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_SET_EXTERN_BS_BUF,
                  (void *)(pstExternBuf));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret) {
        DRV_VENC_ERR("failed to set h265 sao %d", s32Ret);
        s32Ret = DRV_ERR_VENC_SET_EXTERN_BUF;
    }

    return s32Ret;
}

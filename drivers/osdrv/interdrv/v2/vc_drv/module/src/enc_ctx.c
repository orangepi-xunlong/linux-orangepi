#include "venc.h"
#include "enc_ctx.h"
#include "module_common.h"
#include "drv_venc.h"
#include "main_helper.h"
#include "datastructure.h"

#define JPEG_STREAM_CNT (2)

static void _set_src_info(DRVFRAMEBUF *psi, venc_enc_ctx *pEncCtx,
               const video_frame_info_s *pstFrame)
{
    venc_enc_ctx_base *pvecb = &pEncCtx->base;
    const video_frame_s *pstVFrame;

    pstVFrame = &pstFrame->video_frame;

    psi->strideY = pstVFrame->stride[0];
    psi->strideC = pstVFrame->stride[1];
    psi->vbY.phys_addr =
        pstVFrame->phyaddr[0] + psi->strideY * pvecb->y + pvecb->x;

    switch (pstVFrame->pixel_format) {
    case PIXEL_FORMAT_YUV_PLANAR_422:
        psi->format = DRV_FORMAT_422;
        psi->packedFormat = DRV_PACKED_FORMAT_NONE;
        psi->chromaInterleave = DRV_CBCR_SEPARATED;
        psi->vbCb.phys_addr = pstVFrame->phyaddr[1] +
                      psi->strideC * pvecb->y + pvecb->x / 2;
        psi->vbCr.phys_addr = pstVFrame->phyaddr[2] +
                      psi->strideC * pvecb->y + pvecb->x / 2;
        break;
    case PIXEL_FORMAT_NV16:
    case PIXEL_FORMAT_NV61:
        psi->format = DRV_FORMAT_422;
        psi->packedFormat = DRV_PACKED_FORMAT_NONE;
        psi->chromaInterleave = DRV_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = pstVFrame->phyaddr[1] +
                      psi->strideC * pvecb->y + pvecb->x / 2;
        psi->vbCr.phys_addr = pstVFrame->phyaddr[2] +
                      psi->strideC * pvecb->y + pvecb->x / 2;
        break;
    case PIXEL_FORMAT_YUYV:
        psi->format = DRV_FORMAT_422;
        psi->packedFormat = DRV_PACKED_FORMAT_422_YUYV;
        psi->chromaInterleave = DRV_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_UYVY:
        psi->format = DRV_FORMAT_422;
        psi->packedFormat = DRV_PACKED_FORMAT_422_UYVY;
        psi->chromaInterleave = DRV_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_YVYU:
        psi->format = DRV_FORMAT_422;
        psi->packedFormat = DRV_PACKED_FORMAT_422_YVYU;
        psi->chromaInterleave = DRV_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_VYUY:
        psi->format = DRV_FORMAT_422;
        psi->packedFormat = DRV_PACKED_FORMAT_422_VYUY;
        psi->chromaInterleave = DRV_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_NV12:
    case PIXEL_FORMAT_NV21:
        psi->format = DRV_FORMAT_420;
        psi->packedFormat = DRV_PACKED_FORMAT_NONE;
        psi->chromaInterleave =
            (pstVFrame->pixel_format == PIXEL_FORMAT_NV12) ?
                      DRV_CBCR_INTERLEAVE :
                      DRV_CRCB_INTERLEAVE;
        psi->vbCb.phys_addr = pstVFrame->phyaddr[1] +
                      psi->strideC * pvecb->y / 2 + pvecb->x;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_444:
        psi->format = DRV_FORMAT_444;
        psi->packedFormat = DRV_PACKED_FORMAT_NONE;
        psi->chromaInterleave = DRV_CBCR_SEPARATED;
        psi->vbCb.phys_addr = pstVFrame->phyaddr[1] +
                      psi->strideC * pvecb->y + pvecb->x;
        psi->vbCr.phys_addr = pstVFrame->phyaddr[2] +
                      psi->strideC * pvecb->y + pvecb->x;
        break;
    case PIXEL_FORMAT_YUV_400:
        psi->format = DRV_FORMAT_400;
        psi->packedFormat = DRV_PACKED_FORMAT_NONE;
        psi->chromaInterleave = DRV_CBCR_SEPARATED;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
    default:
        psi->format = DRV_FORMAT_420;
        psi->packedFormat = DRV_PACKED_FORMAT_NONE;
        psi->chromaInterleave = DRV_CBCR_SEPARATED;
        psi->vbCb.phys_addr = pstVFrame->phyaddr[1] +
                      psi->strideC * pvecb->y / 2 +
                      pvecb->x / 2;
        psi->vbCr.phys_addr = pstVFrame->phyaddr[2] +
                      psi->strideC * pvecb->y / 2 +
                      pvecb->x / 2;
        break;
    }
    if (pvecb->x || pvecb->y )
    {
        psi->width = pvecb->width;
        psi->height = pvecb->height;
    } else {
        psi->width = pstVFrame->width;
        psi->height = pstVFrame->height;
    }


    psi->vbY.virt_addr = (void *)pstVFrame->viraddr[0] +
                 (psi->vbY.phys_addr - pstVFrame->phyaddr[0]);
    psi->vbCb.virt_addr = (void *)pstVFrame->viraddr[1] +
                  (psi->vbCb.phys_addr - pstVFrame->phyaddr[1]);
    psi->vbCr.virt_addr = (void *)pstVFrame->viraddr[2] +
                  (psi->vbCr.phys_addr - pstVFrame->phyaddr[2]);

}



static int jpege_init(void *ctx)
{
    int status = 0;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    pEncCtx->ext.jpeg.handle = jpeg_enc_init();
    if (pEncCtx->ext.jpeg.handle == NULL) {
        DRV_VENC_ERR("jpeg_init\n");
        return -1;
    }

    return status;
}

static int jpege_close(void *ctx)
{
    int status = 0;
    drv_jpg_handle *pHandle;

    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    pHandle = pEncCtx->ext.jpeg.handle;
    if (pHandle != NULL) {
        jpeg_enc_close(pHandle);
        jpeg_enc_deinit(pHandle);
        pEncCtx->ext.jpeg.handle = NULL;
    }

    return status;
}


static int jpege_open(void *handle, void *pchnctx)
{
    venc_context *pHandle = (venc_context *)handle;
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    int status = 0;
    drv_jpg_config config;
    venc_rc_attr_s *prcatt = NULL;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    memset(&config, 0, sizeof(drv_jpg_config));

    // JPEG marker order
    memcpy(config.u.enc.jpgMarkerOrder,
           pHandle->ModParam.stJpegeModParam.JpegMarkerOrder,
           DRV_JPG_MARKER_ORDER_BUF_SIZE);

    config.type = JPGCOD_ENC;
    config.u.enc.picWidth = pEncCtx->base.width;
    config.u.enc.picHeight = pEncCtx->base.height;
    config.u.enc.chromaInterleave = DRV_CBCR_SEPARATED; // CbCr Separated
    config.u.enc.mirDir = pChnAttr->stVencAttr.enMirrorDirextion;
    config.u.enc.src_type = JPEG_MEM_EXTERNAL;
    config.u.enc.rotAngle = pChnAttr->stVencAttr.enRotation;
    config.u.enc.external_bs_addr = pChnAttr->stVencAttr.u64ExternalBufAddr;
    config.u.enc.singleEsBuffer =
        pHandle->ModParam.stJpegeModParam.bSingleEsBuf;
    if (config.u.enc.singleEsBuffer)
        config.u.enc.bitstreamBufSize =
            pHandle->ModParam.stJpegeModParam.u32SingleEsBufSize;
    else
        config.u.enc.bitstreamBufSize = pChnAttr->stVencAttr.u32BufSize;

    if(!config.u.enc.bitstreamBufSize) {
        if(config.u.enc.external_bs_addr){
            DRV_VENC_ERR("user set external bs buffer, but bs size is 0, check it.\n");
            return DRV_ERR_VENC_ILLEGAL_PARAM;
        } else {
            config.u.enc.bitstreamBufSize = 512 * 1024;
        }
    }

    switch (pChnAttr->stVencAttr.enPixelFormat) {
    case PIXEL_FORMAT_YUV_PLANAR_422:
        config.u.enc.sourceFormat = DRV_FORMAT_422;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_NONE;
        break;
    case PIXEL_FORMAT_YUYV:
        config.u.enc.sourceFormat = DRV_FORMAT_422;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_422_YUYV;
        config.u.enc.chromaInterleave = DRV_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_UYVY:
        config.u.enc.sourceFormat = DRV_FORMAT_422;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_422_UYVY;
        config.u.enc.chromaInterleave = DRV_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_YVYU:
        config.u.enc.sourceFormat = DRV_FORMAT_422;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_422_YVYU;
        config.u.enc.chromaInterleave = DRV_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_VYUY:
        config.u.enc.sourceFormat = DRV_FORMAT_422;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_422_VYUY;
        config.u.enc.chromaInterleave = DRV_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_NV16:
        config.u.enc.sourceFormat = DRV_FORMAT_422;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_NONE;
        config.u.enc.chromaInterleave = DRV_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_NV61:
        config.u.enc.sourceFormat = DRV_FORMAT_422;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_NONE;
        config.u.enc.chromaInterleave = DRV_CRCB_INTERLEAVE;
        break;
    case PIXEL_FORMAT_NV12:
        config.u.enc.sourceFormat = DRV_FORMAT_420;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_NONE;
        config.u.enc.chromaInterleave = DRV_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_NV21:
        config.u.enc.sourceFormat = DRV_FORMAT_420;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_NONE;
        config.u.enc.chromaInterleave = DRV_CRCB_INTERLEAVE;
        break;
    case PIXEL_FORMAT_YUV_400:
        config.u.enc.sourceFormat = DRV_FORMAT_400;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_NONE;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_444:
        config.u.enc.sourceFormat = DRV_FORMAT_444;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_NONE;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
        config.u.enc.sourceFormat = DRV_FORMAT_420;
        config.u.enc.packedFormat = DRV_PACKED_FORMAT_NONE;
        break;
    default:
        DRV_VENC_ERR("jpeg not support fmt:%d\n",pChnAttr->stVencAttr.enPixelFormat);
        return DRV_ERR_VENC_NOT_SUPPORT;
    }

    DRV_VENC_WARN("fmt:%d sfmt:%d pfmt:%d\n", pChnAttr->stVencAttr.enPixelFormat,
        config.u.enc.sourceFormat, config.u.enc.packedFormat);

    if ((config.u.enc.bitstreamBufSize & 0x3FF) != 0) {
        DRV_VENC_WARN("%s bitstreamBufSize (0x%x) must align to 1024\n",
                 __func__, config.u.enc.bitstreamBufSize);
        config.u.enc.bitstreamBufSize -= config.u.enc.bitstreamBufSize&0x3FF;
        //return DRV_ERR_VENC_NOT_SUPPORT;
    }
    prcatt = &pChnAttr->stRcAttr;

    if (prcatt->enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
        venc_mjpeg_fixqp_s *pstMJPEGFixQp = &prcatt->stMjpegFixQp;

        config.u.enc.quality = pstMJPEGFixQp->u32Qfactor;
    } else if (prcatt->enRcMode == VENC_RC_MODE_MJPEGCBR) {
        venc_mjpeg_cbr_s *pstMJPEGCbr = &prcatt->stMjpegCbr;
        int frameRateDiv, frameRateRes;

        config.u.enc.bitrate = pstMJPEGCbr->u32BitRate;
        frameRateDiv = (pstMJPEGCbr->fr32DstFrameRate >> 16);
        frameRateRes = pstMJPEGCbr->fr32DstFrameRate & 0xFFFF;

        if (frameRateDiv == 0) {
            config.u.enc.framerate = frameRateRes;
        } else {
            config.u.enc.framerate =
                ((frameRateDiv - 1) << 16) + frameRateRes;
        }
    } else {
        venc_mjpeg_vbr_s *pstMJPEGVbr = &prcatt->stMjpegVbr;
        int frameRateDiv, frameRateRes;

        config.u.enc.bitrate = pstMJPEGVbr->u32MaxBitRate;
        frameRateDiv = (pstMJPEGVbr->fr32DstFrameRate >> 16);
        frameRateRes = pstMJPEGVbr->fr32DstFrameRate & 0xFFFF;

        if (frameRateDiv == 0) {
            config.u.enc.framerate = frameRateRes;
        } else {
            config.u.enc.framerate =
                ((frameRateDiv - 1) << 16) + frameRateRes;
        }
    }

    config.s32ChnNum = pChnHandle->VeChn;
    status = jpeg_enc_open(pEncCtx->ext.jpeg.handle, config);
    if (status != 0) {
        DRV_VENC_ERR("%s jpeg_enc_open failed !\n", __func__);
        jpeg_enc_close(pEncCtx->ext.jpeg.handle);
        return DRV_ERR_VENC_NULL_PTR;
    }

    return status;
}

static int jpege_enc_one_pic(void *ctx,
                 const video_frame_info_s *pstFrame,
                 int s32MIlliSec)
{
    drv_jpg_handle *pHandle;
    DRVFRAMEBUF srcInfo, *psi = &srcInfo;
    int status = 0;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    pHandle = pEncCtx->ext.jpeg.handle;

    _set_src_info(psi, pEncCtx, pstFrame);

    status = jpeg_enc_send_frame(pHandle, &srcInfo, s32MIlliSec);
    if (status == ENC_TIMEOUT) {
        //jpeg_enc_one_pic TimeOut..dont close
        //otherwise parallel / multiple jpg encode will failure
        //retry, workaround for https://jira.sophgo.com/browse/SE9SW-1454
        status = jpeg_enc_send_frame(pHandle, &srcInfo, s32MIlliSec);
        if (status == ENC_TIMEOUT)
            return DRV_ERR_VENC_BUSY;
    }

    if (status != 0) {
        DRV_VENC_ERR("Failed to jpeg_enc_send_frame, ret = %x\n", status);
        jpeg_enc_close(pHandle);
        return DRV_ERR_VENC_BUSY;
    }

    return status;
}

static int jpege_get_stream(void *ctx, venc_stream_s *pstStream,
                int s32MilliSec)
{
    JPG_BUF stream_buffer[2] = { 0 };
    int status = 0;
    drv_jpg_handle *pHandle;
    venc_pack_s *ppack;
    int i;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    UNREFERENCED_PARAM(s32MilliSec);

    pHandle = pEncCtx->ext.jpeg.handle;

    status = jpeg_enc_get_stream(
        pHandle, (void *)stream_buffer, (unsigned long int *)&pEncCtx->base.u64EncHwTime);
    if (status != 0) {
        DRV_VENC_ERR("Failed to jpeg_enc_get_stream, ret = %x\n", status);
        return DRV_ERR_VENC_BUSY;
    }

    pstStream->u32PackCount = JPEG_STREAM_CNT;
    for(i = 0; i < pstStream->u32PackCount; i++) {
        ppack = &pstStream->pstPack[i];

        memset(ppack, 0, sizeof(venc_pack_s));

        ppack->pu8Addr = (unsigned char *)stream_buffer[i].virt_addr;
        ppack->u64PhyAddr = stream_buffer[i].phys_addr;
        ppack->u32Len = stream_buffer[i].size;
        ppack->u64PTS = pEncCtx->base.u64PTS;
    }

    return status;
}

static int jpege_release_stream(void *ctx, venc_stream_s *pstStream)
{
    int status = 0;
    drv_jpg_handle *pHandle;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    UNREFERENCED_PARAM(pstStream);

    pHandle = pEncCtx->ext.jpeg.handle;
    status = jpeg_enc_release_stream(pHandle);

    if (status != 0) {
        DRV_VENC_ERR("Failed to jpeg_enc_release_stream, ret = %x\n",
                 status);
        jpeg_enc_close(pHandle);
        return DRV_ERR_VENC_BUSY;
    }

    return 0;
}

static int jpege_ioctl(void *ctx, int op, void *arg)
{
    int status = 0;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    int currOp;

    currOp = (op & DRV_JPEG_OP_MASK) >> DRV_JPEG_OP_SHIFT;
    if (currOp == 0) {
        DRV_VENC_WARN("op = 0x%X, currOp = 0x%X\n", op, currOp);
        return 0;
    }

    status = jpeg_ioctl(pEncCtx->ext.jpeg.handle, currOp, arg);
    if (status != 0) {
        DRV_VENC_ERR("jpeg_ioctl, currOp = 0x%X, status = %d\n",
                 currOp, status);
        return status;
    }

    return status;
}

static int vid_enc_init(void *ctx)
{
    int status = 0;
    internal_venc_init();
    return status;
}

static int vid_enc_open(void *handle, void *pchnctx)
{
    venc_context *pHandle = (venc_context *)handle;
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    int status = 0;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    venc_attr_s *pVencAttr = &pChnAttr->stVencAttr;
    venc_gop_ex_attr_s *pVencGopExAttr = &pChnAttr->stGopExAttr;
    InitEncConfig initEncCfg, *pInitEncCfg;

    pInitEncCfg = &initEncCfg;
    memset(pInitEncCfg, 0, sizeof(InitEncConfig));

    pInitEncCfg->codec =
        (pVencAttr->enType == PT_H265) ? CODEC_H265 : CODEC_H264;
    pInitEncCfg->width = pEncCtx->base.width;
    pInitEncCfg->height = pEncCtx->base.height;
    pInitEncCfg->u32Profile = pEncCtx->base.u32Profile;
    pInitEncCfg->rcMode = pEncCtx->base.rcMode;
    pInitEncCfg->bitrate = 0;
    if (pInitEncCfg->codec == CODEC_H264) {
        pInitEncCfg->userDataMaxLength =
            pHandle->ModParam.stH264eModParam.u32UserDataMaxLen;
        pInitEncCfg->singleLumaBuf =
            pVencAttr->stAttrH264e.bSingleLumaBuf;
        pInitEncCfg->bSingleEsBuf = pVencAttr->bIsoSendFrmEn ? 0 :
            pHandle->ModParam.stH264eModParam.bSingleEsBuf;
    } else if (pInitEncCfg->codec == CODEC_H265) {
        pInitEncCfg->userDataMaxLength =
            pHandle->ModParam.stH265eModParam.u32UserDataMaxLen;
        pInitEncCfg->bSingleEsBuf = pVencAttr->bIsoSendFrmEn ? 0 :
            pHandle->ModParam.stH265eModParam.bSingleEsBuf;
        if (pHandle->ModParam.stH265eModParam.enRefreshType ==
            H265E_REFRESH_IDR)
            pInitEncCfg->decodingRefreshType = H265_RT_IDR;
        else
            pInitEncCfg->decodingRefreshType = H265_RT_CRA;
    }

    if (pInitEncCfg->bSingleEsBuf) {
        if (pInitEncCfg->codec == CODEC_H264)
            pInitEncCfg->bitstreamBufferSize =
                pHandle->ModParam.stH264eModParam
                    .u32SingleEsBufSize;
        else if (pInitEncCfg->codec == CODEC_H265)
            pInitEncCfg->bitstreamBufferSize =
                pHandle->ModParam.stH265eModParam
                    .u32SingleEsBufSize;
    } else {
        pInitEncCfg->bitstreamBufferSize = pVencAttr->u32BufSize;
    }
    pInitEncCfg->s32ChnNum = pChnHandle->VeChn;
    pInitEncCfg->bEsBufQueueEn = pVencAttr->bEsBufQueueEn;
    pInitEncCfg->bIsoSendFrmEn = pVencAttr->bIsoSendFrmEn;
    pInitEncCfg->s32CmdQueueDepth = pVencAttr->u32CmdQueueDepth;
    if (pInitEncCfg->s32CmdQueueDepth == 0 || pInitEncCfg->s32CmdQueueDepth > MAX_VENC_QUEUE_DEPTH) {
        pInitEncCfg->s32CmdQueueDepth = MAX_VENC_QUEUE_DEPTH;
    }
    pInitEncCfg->s32EncMode = pVencAttr->enEncMode;
    if (pInitEncCfg->s32EncMode < 1 || pInitEncCfg->s32EncMode > 3) {
        pInitEncCfg->s32EncMode = 1;
    }
    pInitEncCfg->u32GopPreset = pVencGopExAttr->u32GopPreset;
    if (pVencGopExAttr->u32GopPreset == GOP_PRESET_IDX_CUSTOM_GOP) {
        memcpy(&pInitEncCfg->gopParam, &pVencGopExAttr->gopParam, sizeof(CustomGopParam));
    }

    pInitEncCfg->s32RotationAngle = pVencAttr->enRotation;
    pInitEncCfg->s32MirrorDirection = pVencAttr->enMirrorDirextion;

    if (pEncCtx->ext.vid.setInitCfgRc)
        pEncCtx->ext.vid.setInitCfgRc(pInitEncCfg, pChnHandle);


    pEncCtx->ext.vid.pHandle = internal_venc_open(pInitEncCfg);
    if (!pEncCtx->ext.vid.pHandle) {
        DRV_VENC_ERR("internal_venc_open\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    return status;
}

static void set_init_cfg_gop(InitEncConfig *pInitEncCfg,
                 venc_chn_context *pChnHandle)
{
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_gop_attr_s *pga = &pChnAttr->stGopAttr;

    pInitEncCfg->virtualIPeriod = 0;

    if (pga->enGopMode == VENC_GOPMODE_NORMALP) {
        pInitEncCfg->s32IPQpDelta = pga->stNormalP.s32IPQpDelta;

    } else if (pga->enGopMode == VENC_GOPMODE_SMARTP) {
        pInitEncCfg->virtualIPeriod = pInitEncCfg->gop;
        pInitEncCfg->gop = pga->stSmartP.u32BgInterval;
        pInitEncCfg->s32BgQpDelta = pga->stSmartP.s32BgQpDelta;
        pInitEncCfg->s32ViQpDelta = pga->stSmartP.s32ViQpDelta;
    }
}

static void h264e_set_initcfg_fixqp(InitEncConfig *pInitEncCfg,
                      void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    venc_h264_fixqp_s *pstH264FixQp = &prcatt->stH264FixQp;

    pInitEncCfg->iqp = pstH264FixQp->u32IQp;
    pInitEncCfg->pqp = pstH264FixQp->u32PQp;
    pInitEncCfg->gop = pstH264FixQp->u32Gop;
    pInitEncCfg->framerate = (int)pstH264FixQp->fr32DstFrameRate;

    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static void h264e_set_initcfg_cbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    venc_h264_cbr_s *pstH264Cbr = &prcatt->stH264Cbr;

    pInitEncCfg->statTime = pstH264Cbr->u32StatTime;
    pInitEncCfg->gop = pstH264Cbr->u32Gop;
    pInitEncCfg->bitrate = pstH264Cbr->u32BitRate;
    pInitEncCfg->framerate = (int)pstH264Cbr->fr32DstFrameRate;

    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static void h264e_set_initcfg_vbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    venc_h264_vbr_s *pstH264Vbr = &prcatt->stH264Vbr;
    venc_rc_param_s *prcparam = &pChnHandle->rcParam;

    pInitEncCfg->statTime = pstH264Vbr->u32StatTime;
    pInitEncCfg->gop = pstH264Vbr->u32Gop;
    pInitEncCfg->bitrate = pstH264Vbr->u32MaxBitRate;
    pInitEncCfg->maxbitrate = pstH264Vbr->u32MaxBitRate;
    pInitEncCfg->framerate = (int)pstH264Vbr->fr32DstFrameRate;
    pInitEncCfg->s32ChangePos = prcparam->stParamH264Vbr.s32ChangePos;

    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static void h264e_set_initcfg_avbr(InitEncConfig *pInitEncCfg,
                     void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    venc_h264_avbr_s *pstH264AVbr = &prcatt->stH264AVbr;
    venc_param_h264_avbr_s *pprc = &pChnHandle->rcParam.stParamH264AVbr;

    pInitEncCfg->statTime = pstH264AVbr->u32StatTime;
    pInitEncCfg->gop = pstH264AVbr->u32Gop;
    pInitEncCfg->framerate = (int)pstH264AVbr->fr32DstFrameRate;
    pInitEncCfg->bitrate = pstH264AVbr->u32MaxBitRate;
    pInitEncCfg->maxbitrate = pstH264AVbr->u32MaxBitRate;
    pInitEncCfg->s32ChangePos = pprc->s32ChangePos;
    pInitEncCfg->s32MinStillPercent = pprc->s32MinStillPercent;
    pInitEncCfg->u32MaxStillQP = pprc->u32MaxStillQP;
    pInitEncCfg->u32MotionSensitivity = pprc->u32MotionSensitivity;
    pInitEncCfg->s32AvbrFrmLostOpen = pprc->s32AvbrFrmLostOpen;
    pInitEncCfg->s32AvbrFrmGap = pprc->s32AvbrFrmGap;
    pInitEncCfg->s32AvbrPureStillThr = pprc->s32AvbrPureStillThr;

    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static void h264e_set_initcfg_ubr(InitEncConfig *pInitEncCfg,
                    void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    venc_h264_ubr_s *pstH264Ubr = &prcatt->stH264Ubr;

    pInitEncCfg->statTime = pstH264Ubr->u32StatTime;
    pInitEncCfg->gop = pstH264Ubr->u32Gop;
    pInitEncCfg->bitrate = pstH264Ubr->u32BitRate;
    pInitEncCfg->framerate = (int)pstH264Ubr->fr32DstFrameRate;

    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static int h264e_map_nalu_type(venc_pack_s *ppack, int NalType)
{
    int h264naluType[] = {
        H264E_NALU_ISLICE,
        H264E_NALU_PSLICE,
        H264E_NALU_BSLICE,
        H264E_NALU_IDRSLICE,
        H264E_NALU_SPS,
        H264E_NALU_PPS,
        H264E_NALU_SEI,
    };
    int naluType;

    if (!ppack) {
        DRV_VENC_ERR("ppack is NULL\n");
        return -1;
    }

    if (!ppack->pu8Addr) {
        DRV_VENC_ERR("ppack->pu8Addr is NULL\n");
        return -1;
    }

    naluType = ppack->pu8Addr[4] & 0x1f;

    if (NalType < NAL_I || NalType >= NAL_MAX) {
        DRV_VENC_ERR("NalType = %d\n", NalType);
        return -1;
    }

    if (naluType == H264_NALU_TYPE_IDR)
        ppack->DataType.enH264EType = H264E_NALU_IDRSLICE;
    else
        ppack->DataType.enH264EType = h264naluType[NalType];

    DRV_VENC_DBG("enH264EType = %d\n", ppack->DataType.enH264EType);
    return 0;
}

static void h265e_set_initcfg_fixqp(InitEncConfig *pInitEncCfg,
                      void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_FIXQP_S *pstH265FixQp = &prcatt->stH265FixQp;

    pInitEncCfg->iqp = pstH265FixQp->u32IQp;
    pInitEncCfg->pqp = pstH265FixQp->u32PQp;
    pInitEncCfg->gop = pstH265FixQp->u32Gop;
    pInitEncCfg->framerate = (int)pstH265FixQp->fr32DstFrameRate;
    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static void h265e_set_initcfg_cbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_CBR_S *pstH265Cbr = &prcatt->stH265Cbr;

    pInitEncCfg->statTime = pstH265Cbr->u32StatTime;
    pInitEncCfg->gop = pstH265Cbr->u32Gop;
    pInitEncCfg->bitrate = pstH265Cbr->u32BitRate;
    pInitEncCfg->framerate = (int)pstH265Cbr->fr32DstFrameRate;
    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static void h265e_set_initcfg_vbr(InitEncConfig *pInitEncCfg,
                    void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_VBR_S *pstH265Vbr = &prcatt->stH265Vbr;
    venc_rc_param_s *prcparam = &pChnHandle->rcParam;

    pInitEncCfg->statTime = pstH265Vbr->u32StatTime;
    pInitEncCfg->gop = pstH265Vbr->u32Gop;
    pInitEncCfg->bitrate = pstH265Vbr->u32MaxBitRate;
    pInitEncCfg->maxbitrate = pstH265Vbr->u32MaxBitRate;
    pInitEncCfg->s32ChangePos = prcparam->stParamH265Vbr.s32ChangePos;
    pInitEncCfg->framerate = (int)pstH265Vbr->fr32DstFrameRate;
    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static void h265e_set_initcfg_avbr(InitEncConfig *pInitEncCfg,
                     void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_AVBR_S *pstH265AVbr = &prcatt->stH265AVbr;
    venc_param_h265_avbr_s *pprc = &pChnHandle->rcParam.stParamH265AVbr;

    pInitEncCfg->statTime = pstH265AVbr->u32StatTime;
    pInitEncCfg->gop = pstH265AVbr->u32Gop;
    pInitEncCfg->framerate = (int)pstH265AVbr->fr32DstFrameRate;
    pInitEncCfg->bitrate = pstH265AVbr->u32MaxBitRate;
    pInitEncCfg->maxbitrate = pstH265AVbr->u32MaxBitRate;
    pInitEncCfg->s32ChangePos = pprc->s32ChangePos;
    pInitEncCfg->s32MinStillPercent = pprc->s32MinStillPercent;
    pInitEncCfg->u32MaxStillQP = pprc->u32MaxStillQP;
    pInitEncCfg->u32MotionSensitivity = pprc->u32MotionSensitivity;
    pInitEncCfg->s32AvbrFrmLostOpen = pprc->s32AvbrFrmLostOpen;
    pInitEncCfg->s32AvbrFrmGap = pprc->s32AvbrFrmGap;
    pInitEncCfg->s32AvbrPureStillThr = pprc->s32AvbrPureStillThr;
    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static void h265e_set_initcfg_qpmap(InitEncConfig *pInitEncCfg,
                      void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    venc_h265_qpmap_s *pstH265QpMap = &prcatt->stH265QpMap;

    pInitEncCfg->statTime = pstH265QpMap->u32StatTime;
    pInitEncCfg->gop = pstH265QpMap->u32Gop;
    pInitEncCfg->bitrate =
        DEAULT_QP_MAP_BITRATE; // QpMap uses CBR as basic settings
    pInitEncCfg->framerate = (int)pstH265QpMap->fr32DstFrameRate;
    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static void h265e_set_initcfg_ubr(InitEncConfig *pInitEncCfg,
                    void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_UBR_S *pstH265Ubr = &prcatt->stH265Ubr;

    pInitEncCfg->statTime = pstH265Ubr->u32StatTime;
    pInitEncCfg->gop = pstH265Ubr->u32Gop;
    pInitEncCfg->bitrate = pstH265Ubr->u32BitRate;
    pInitEncCfg->framerate = (int)pstH265Ubr->fr32DstFrameRate;

    set_init_cfg_gop(pInitEncCfg, pChnHandle);
}

static int h265e_map_nalu_type(venc_pack_s *ppack, int NalType)
{
    int h265naluType[] = {
        H265E_NALU_ISLICE,
        H265E_NALU_PSLICE,
        H265E_NALU_BSLICE,
        H265E_NALU_IDRSLICE,
        H265E_NALU_SPS,
        H265E_NALU_PPS,
        H265E_NALU_SEI,
        H265E_NALU_VPS,
    };
    int naluType;

    if (!ppack) {
        DRV_VENC_ERR("ppack is NULL\n");
        return -1;
    }

    if (!ppack->pu8Addr) {
        DRV_VENC_ERR("ppack->pu8Addr is NULL\n");
        return -1;
    }

    naluType = (ppack->pu8Addr[4] & 0x7f) >> 1;

    if (NalType < NAL_I || NalType >= NAL_MAX) {
        DRV_VENC_ERR("NalType = %d\n", NalType);
        return -1;
    }

    if (naluType == H265_NALU_TYPE_W_RADL ||
        naluType == H265_NALU_TYPE_N_LP)
        ppack->DataType.enH265EType = H265E_NALU_IDRSLICE;
    else
        ppack->DataType.enH265EType = h265naluType[NalType];

    DRV_VENC_DBG("enH265EType = %d\n", ppack->DataType.enH265EType);
    return 0;
}

static int vid_enc_close(void *ctx)
{
    int status = 0;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    if (pEncCtx->ext.vid.pHandle) {
        status = internal_venc_close(pEncCtx->ext.vid.pHandle);
        if (status < 0) {
            DRV_VENC_ERR("venc_close, status = %d\n", status);
            return status;
        }
    }

    return status;
}



static int vid_enc_enc_one_pic(void *ctx,
                  const video_frame_info_s *pstFrame,
                  int s32MilliSec)
{
    int status = 0;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    EncOnePicCfg encOnePicCfg;
    unsigned char mtable[MO_TBL_SIZE];
    EncOnePicCfg *pPicCfg = &encOnePicCfg;
    DRVFRAMEBUF srcInfo, *psi = &srcInfo;
    struct vb_s *blk = (struct vb_s *)pstFrame->video_frame.private_data;

    _set_src_info(psi, pEncCtx, pstFrame);

    pPicCfg->addrY = psi->vbY.virt_addr;
    pPicCfg->addrCb = psi->vbCb.virt_addr;
    pPicCfg->addrCr = psi->vbCr.virt_addr;

    pPicCfg->phyAddrY = psi->vbY.phys_addr;
    pPicCfg->phyAddrCb = psi->vbCb.phys_addr;
    pPicCfg->phyAddrCr = psi->vbCr.phys_addr;
    pPicCfg->u64Pts = pstFrame->video_frame.pts;
    pPicCfg->src_end = pstFrame->video_frame.srcend;
    pPicCfg->src_idx = pstFrame->video_frame.frame_idx;
    pPicCfg->stride = psi->strideY;
    switch (pstFrame->video_frame.pixel_format) {
    case PIXEL_FORMAT_NV12:
        pPicCfg->cbcrInterleave = 1;
        pPicCfg->nv21 = 0;
        break;
    case PIXEL_FORMAT_NV21:
        pPicCfg->cbcrInterleave = 1;
        pPicCfg->nv21 = 1;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
    default:
        pPicCfg->cbcrInterleave = 0;
        pPicCfg->nv21 = 0;
        break;
    }

    if (pEncCtx->base.rcMode == RC_MODE_AVBR) {
        if (blk) {
            pPicCfg->picMotionLevel = blk->buf.motion_lv;
            pPicCfg->picMotionMap = mtable;
            pPicCfg->picMotionMapSize = MO_TBL_SIZE;
            memcpy(mtable, blk->buf.motion_table, MO_TBL_SIZE);
        }
        pPicCfg->picMotionMap = mtable;
    }

    status = internal_venc_enc_one_pic(pEncCtx->ext.vid.pHandle, pPicCfg,
                  s32MilliSec);

    if (status == ENC_TIMEOUT || status == RETCODE_QUEUEING_FAILURE || status == RETCODE_STREAM_BUF_FULL) {
        DRV_VENC_WARN("internal_venc_enc_one_pic, status = %d\n", status);
        return DRV_ERR_VENC_BUSY;
    }

    return status;
}

static int vid_enc_get_stream(void *ctx, venc_stream_s *pstStream,
                 int s32MilliSec)
{
    int status = 0;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    VEncStreamInfo *pStreamInfo = &pEncCtx->ext.vid.streamInfo;
    Queue *psp = NULL;
    venc_pack_s *ppack = NULL;
    stPack *pqpacks = NULL;
    unsigned int idx = 0;
    int totalPacks = 0;
    unsigned int encHwTimeus = 0;

    status = internal_venc_get_stream(pEncCtx->ext.vid.pHandle, pStreamInfo,
                  s32MilliSec);
    DRV_VENC_INFO("internal_venc_get_stream status %d\n", status);
    if (status == ENC_TIMEOUT) {
        return DRV_ERR_VENC_BUSY;
    } else if (status) {
        DRV_VENC_ERR("get stream failed,status %d\n", status);
        return DRV_ERR_VENC_INVALILD_RET;
    }

    psp = (Queue *)pStreamInfo->psp;
    if (!psp) {
        DRV_VENC_ERR("psp is null\n");
        return DRV_ERR_VENC_NULL_PTR;
    }

    totalPacks = Queue_Get_Cnt(psp);
    if (totalPacks == 0) {
        return DRV_ERR_VENC_GET_STREAM_END;
    }

    pstStream->u32PackCount = 0;
    for (idx = 0; (idx < totalPacks) && (idx < MAX_NUM_PACKS); idx++) {
        ppack = &pstStream->pstPack[idx];

        if (!ppack) {
            DRV_VENC_ERR("get NULL pack, uPackCount:%d idx:%d\n", pstStream->u32PackCount, idx);
            break;
        }

        memset(ppack, 0, sizeof(venc_pack_s));
        pqpacks = Queue_Peek(psp);
        if (!pqpacks) {
            continue;
        }

        // only sei packs, return no packs
        if (totalPacks == 1 && pqpacks->NalType == NAL_SEI) {
            return DRV_ERR_VENC_BUSY;
        }

        pqpacks = Queue_Dequeue(psp);

        // psp->pack[idx].bUsed = 1;
        ppack->u64PhyAddr = pqpacks->u64PhyAddr;
        ppack->pu8Addr = pqpacks->addr;
        ppack->u32Len = pqpacks->len;
        ppack->u64PTS = pqpacks->u64PTS;
        ppack->u64DTS = pqpacks->u64DTS;
        ppack->releasFrameIdx = pqpacks->encSrcIdx;
        ppack->u64CustomMapPhyAddr = pqpacks->u64CustomMapAddr;
        ppack->u32AvgCtuQp = pqpacks->u32AvgCtuQp;
        encHwTimeus = pqpacks->u32EncHwTime;
        status = pEncCtx->ext.vid.mapNaluType(
            ppack, pqpacks->NalType);
        if (status) {
            DRV_VENC_ERR("mapNaluType, status = %d\n", status);
            return status;
        }
        pstStream->u32PackCount++;

        // 1. division [B], [P], [I]/[IDR]
        // 2. division [P] + [VPS SPS PPS I]
        // 3. division [SEI P] + [SEI B] + [VPS SPS PPS SEI I]
        if (pqpacks->NalType >= NAL_I
            && pqpacks->NalType <= NAL_IDR) {
            break;
        }
    }

    DRV_VENC_DBG("packcnt:%d, type:%d\n", pstStream->u32PackCount, pstStream->pstPack[0].DataType.enH264EType);
    pEncCtx->base.u64EncHwTime = (uint64_t)encHwTimeus;

    return status;
}

static int vid_enc_release_stream(void *ctx, venc_stream_s *pstStream)
{
    int status = 0;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    stPack vencPack[MAX_NUM_PACKS] = {0};
    int idx = 0;

    for (idx = 0; (idx < pstStream->u32PackCount) && (idx < MAX_NUM_PACKS); idx++) {
        vencPack[idx].u64PhyAddr = pstStream->pstPack[idx].u64PhyAddr;
        vencPack[idx].addr = pstStream->pstPack[idx].pu8Addr;
        vencPack[idx].len = pstStream->pstPack[idx].u32Len;
        if ( pstStream->pstPack[idx].DataType.enH264EType == H264E_NALU_SEI
            || pstStream->pstPack[idx].DataType.enH265EType == H265E_NALU_SEI) {
            vencPack[idx].NalType = NAL_SEI;
        } else if (pstStream->pstPack[idx].DataType.enH264EType == H264E_NALU_SPS) {
            vencPack[idx].NalType = NAL_SPS;
        } else if (pstStream->pstPack[idx].DataType.enH265EType == H265E_NALU_VPS) {
            vencPack[idx].NalType = NAL_VPS;
        }
    }

    status = internal_venc_release_stream(pEncCtx->ext.vid.pHandle, vencPack, pstStream->u32PackCount);
    if (status != 0) {
        DRV_VENC_ERR("internal_venc_release_stream, status = %d\n", status);
        return status;
    }

    return status;
}

static int vid_enc_ioctl(void *ctx, int op, void *arg)
{
    int status = 0;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    int currOp;

    currOp = (op & DRV_H26X_OP_MASK) >> DRV_H26X_OP_SHIFT;
    if (currOp == 0) {
        DRV_VENC_WARN("op = 0x%X, currOp = 0x%X\n", op, currOp);
        return 0;
    }

    status = internal_venc_ioctl(pEncCtx->ext.vid.pHandle, currOp, arg);
    if (status != 0) {
        DRV_VENC_ERR("internal_venc_ioctl, currOp = 0x%X, status = %d\n",
                 currOp, status);
        return status;
    }

    return status;
}


int venc_create_enc_ctx(venc_enc_ctx *pEncCtx, void *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    venc_chn_attr_s *pChnAttr = pChnHandle->pChnAttr;
    venc_attr_s *pVencAttr = &pChnAttr->stVencAttr;
    venc_rc_attr_s *prcatt = &pChnAttr->stRcAttr;
    int status = 0;

    memset(pEncCtx, 0, sizeof(venc_enc_ctx));

    switch (pVencAttr->enType) {
    case PT_JPEG:
    case PT_MJPEG:
        pEncCtx->base.init = &jpege_init;
        pEncCtx->base.open = &jpege_open;
        pEncCtx->base.close = &jpege_close;
        pEncCtx->base.encOnePic = &jpege_enc_one_pic;
        pEncCtx->base.getStream = &jpege_get_stream;
        pEncCtx->base.releaseStream = &jpege_release_stream;
        pEncCtx->base.ioctl = &jpege_ioctl;
        break;
    case PT_H264:
        pEncCtx->base.init = &vid_enc_init;
        pEncCtx->base.open = &vid_enc_open;
        pEncCtx->base.close = &vid_enc_close;
        pEncCtx->base.encOnePic = &vid_enc_enc_one_pic;
        pEncCtx->base.getStream = &vid_enc_get_stream;
        pEncCtx->base.releaseStream = &vid_enc_release_stream;
        pEncCtx->base.ioctl = &vid_enc_ioctl;

        pEncCtx->ext.vid.setInitCfgFixQp = h264e_set_initcfg_fixqp;
        pEncCtx->ext.vid.setInitCfgCbr = h264e_set_initcfg_cbr;
        pEncCtx->ext.vid.setInitCfgVbr = h264e_set_initcfg_vbr;
        pEncCtx->ext.vid.setInitCfgAVbr = h264e_set_initcfg_avbr;
        pEncCtx->ext.vid.setInitCfgUbr = h264e_set_initcfg_ubr;
        pEncCtx->ext.vid.mapNaluType = h264e_map_nalu_type;

        pEncCtx->base.u32Profile = pVencAttr->u32Profile;
        pEncCtx->base.rcMode = prcatt->enRcMode - VENC_RC_MODE_H264CBR;

        if (pEncCtx->base.rcMode == RC_MODE_CBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgCbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH264Cbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_VBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgVbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH264Vbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_AVBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgAVbr;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH264AVbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_FIXQP) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgFixQp;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH264FixQp.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_UBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgUbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH264Ubr.bVariFpsEn;
        }
        break;
    case PT_H265:
        pEncCtx->base.init = &vid_enc_init;
        pEncCtx->base.open = &vid_enc_open;
        pEncCtx->base.close = &vid_enc_close;
        pEncCtx->base.encOnePic = &vid_enc_enc_one_pic;
        pEncCtx->base.getStream = &vid_enc_get_stream;
        pEncCtx->base.releaseStream = &vid_enc_release_stream;
        pEncCtx->base.ioctl = &vid_enc_ioctl;

        pEncCtx->ext.vid.setInitCfgFixQp = h265e_set_initcfg_fixqp;
        pEncCtx->ext.vid.setInitCfgCbr = h265e_set_initcfg_cbr;
        pEncCtx->ext.vid.setInitCfgVbr = h265e_set_initcfg_vbr;
        pEncCtx->ext.vid.setInitCfgAVbr = h265e_set_initcfg_avbr;
        pEncCtx->ext.vid.setInitCfgQpMap = h265e_set_initcfg_qpmap;
        pEncCtx->ext.vid.setInitCfgUbr = h265e_set_initcfg_ubr;
        pEncCtx->ext.vid.mapNaluType = h265e_map_nalu_type;

        pEncCtx->base.u32Profile = pVencAttr->u32Profile;
        pEncCtx->base.rcMode = prcatt->enRcMode - VENC_RC_MODE_H265CBR;

        if (pEncCtx->base.rcMode == RC_MODE_CBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgCbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH265Cbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_VBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgVbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH265Vbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_AVBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgAVbr;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH265AVbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_FIXQP) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgFixQp;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH265FixQp.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_QPMAP) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgQpMap;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH265QpMap.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_UBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgUbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH265Ubr.bVariFpsEn;
        }
        break;
    default:
        DRV_VENC_ERR("enType = %d\n", pVencAttr->enType);
        return -1;
    }

    pEncCtx->base.width = pVencAttr->u32PicWidth;
    pEncCtx->base.height = pVencAttr->u32PicHeight;

    return status;
}



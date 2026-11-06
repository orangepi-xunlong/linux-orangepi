//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#include "vpuapi.h"
#include "product.h"
#include "wave/wave5_regdefine.h"
#include <linux/mutex.h>
#include <linux/delay.h>


#define INVALID_CORE_INDEX_RETURN_ERROR(_coreIdx)  \
    if (_coreIdx >= MAX_NUM_VPU_CORE) \
    return -1;

static const Uint16* s_pusBitCode[MAX_NUM_VPU_CORE] = {NULL,};
static int s_bitCodeSize[MAX_NUM_VPU_CORE] = {0,};

Uint32 __VPU_BUSY_TIMEOUT = VPU_BUSY_CHECK_TIMEOUT;
static DEFINE_MUTEX(__vdi_init_mutex);


static RetCode CheckDecInstanceValidity(CodecInst* pCodecInst)
{
    RetCode ret;

    if (pCodecInst == NULL)
        return RETCODE_INVALID_HANDLE;

    ret = CheckInstanceValidity(pCodecInst);
    if (ret != RETCODE_SUCCESS) {
        return RETCODE_INVALID_HANDLE;
    }
    if (!pCodecInst->inUse) {
        return RETCODE_INVALID_HANDLE;
    }

    return RETCODE_SUCCESS;//ProductVpuDecCheckCapability(pCodecInst);
}


Int32 VPU_IsInit(Uint32 coreIdx)
{
    Int32 pc;

    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);

    if (EnterLock(coreIdx) != RETCODE_SUCCESS)
        return 0;

    pc = ProductVpuIsInit(coreIdx);

    LeaveLock(coreIdx);

    return pc;
}

Int32 VPU_WaitInterrupt(Uint32 coreIdx, int timeout)
{
    Int32 ret;
    CodecInst* instance;

    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);

    if ((instance=GetPendingInst(coreIdx)) != NULL) {
        ret = ProductVpuWaitInterrupt(instance, timeout);
    }
    else {
        ret = -1;
    }

    return ret;
}

Int32 VPU_WaitInterruptEx(VpuHandle handle, int timeout)
{
    Int32 ret;
    CodecInst *pCodecInst;

    pCodecInst  = handle;

    INVALID_CORE_INDEX_RETURN_ERROR(pCodecInst->coreIdx);

    ret = ProductVpuWaitInterrupt(pCodecInst, timeout);

    return ret;
}

void VPU_ClearInterrupt(Uint32 coreIdx)
{
    /* clear all interrupt flags */
    ProductVpuClearInterrupt(coreIdx, 0xffff);
}

void VPU_ClearInterruptEx(VpuHandle handle, Int32 intrFlag)
{
    CodecInst *pCodecInst;

    pCodecInst  = handle;

    ProductVpuClearInterrupt(pCodecInst->coreIdx, intrFlag);
}

int VPU_GetFrameBufStride(VpuHandle handle, int width, int height, int format, int interleave, int mapType)
{
    CodecInst *pCodecInst;

    pCodecInst = handle;

    INVALID_CORE_INDEX_RETURN_ERROR(pCodecInst->coreIdx);

    return ProductCalculateFrameBufStride(pCodecInst, width, height, (TiledMapType)mapType, (FrameBufferFormat)format, (BOOL)interleave);
}

int VPU_GetFrameBufSize(VpuHandle handle, int coreIdx, int stride, int height, int mapType, int format, int interleave, DRAMConfig *pDramCfg)
{
    int productId;
    CodecInst *pCodecInst;

    pCodecInst  = handle;
    UNREFERENCED_PARAMETER(interleave);             /*!<< for backward compatiblity */

    if (coreIdx < 0 || coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    productId = ProductVpuGetId(coreIdx);

    return ProductCalculateFrameBufSize(pCodecInst, productId, stride, height, (TiledMapType)mapType, (FrameBufferFormat)format, (BOOL)interleave, pDramCfg);
}


int VPU_GetProductId(int coreIdx)
{
    Int32 productId;

    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);
    if ((productId=ProductVpuGetId(coreIdx)) != PRODUCT_ID_NONE) {
        return productId;
    }

    mutex_lock(&__vdi_init_mutex);
    if (vdi_init(coreIdx) < 0) {
        mutex_unlock(&__vdi_init_mutex);
        return -1;
    }
    mutex_unlock(&__vdi_init_mutex);

    if (EnterLock(coreIdx) != RETCODE_SUCCESS) {
        mutex_lock(&__vdi_init_mutex);
        vdi_release(coreIdx);
        mutex_unlock(&__vdi_init_mutex);
        return -1;
    }

    if (ProductVpuScan(coreIdx) == FALSE)
        productId = -1;
    else
        productId = ProductVpuGetId(coreIdx);
    LeaveLock((coreIdx));

    mutex_lock(&__vdi_init_mutex);
    vdi_release(coreIdx);
    mutex_unlock(&__vdi_init_mutex);
    return productId;
}

int VPU_GetOpenInstanceNum(Uint32 coreIdx)
{
    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);

    return vdi_get_instance_num(coreIdx);
}

static RetCode InitializeVPU(Uint32 coreIdx, const Uint16* code, Uint32 size)
{
    RetCode ret;

    mutex_lock(&__vdi_init_mutex);
    if (vdi_init(coreIdx) < 0) {
        mutex_unlock(&__vdi_init_mutex);
        return RETCODE_FAILURE;
    }
    mutex_unlock(&__vdi_init_mutex);

    // EnterLock(coreIdx);
    if (EnterLock(coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if (vdi_get_instance_num(coreIdx) > 0) {
        if (ProductVpuScan(coreIdx) == 0) {
            LeaveLock(coreIdx);
            return RETCODE_NOT_FOUND_VPU_DEVICE;
        }
    }

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
    create_sw_uart_thread(coreIdx, ProductVpuGetId(coreIdx));
    msleep(500);
#endif
    if (ProductVpuIsInit(coreIdx) != 0) {
        ProductVpuReInit(coreIdx, (void *)code, size);
        LeaveLock(coreIdx);
        return RETCODE_CALLED_BEFORE;
    }

    InitCodecInstancePool(coreIdx);

    ret = ProductVpuReset(coreIdx, SW_RESET_ON_BOOT);
    if (ret != RETCODE_SUCCESS) {
        LeaveLock(coreIdx);
        return ret;
    }

    ret = ProductVpuInit(coreIdx, (void*)code, size);
    if (ret != RETCODE_SUCCESS) {
        LeaveLock(coreIdx);
        return ret;
    }
    LeaveLock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode VPU_Init(Uint32 coreIdx)
{
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;

    if (s_bitCodeSize[coreIdx] == 0)
        return RETCODE_NOT_FOUND_BITCODE_PATH;

    return InitializeVPU(coreIdx, s_pusBitCode[coreIdx], s_bitCodeSize[coreIdx]);
}

RetCode VPU_InitWithBitcode(Uint32 coreIdx, const Uint16* code, Uint32 size)
{
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;
    if (code == NULL || size == 0)
        return RETCODE_INVALID_PARAM;

#if 0
    if (s_pusBitCode[coreIdx] == NULL) {
        s_pusBitCode[coreIdx] = (Uint16 *)osal_malloc(size*sizeof(Uint16));
        if (!s_pusBitCode[coreIdx])
            return RETCODE_INSUFFICIENT_RESOURCE;
        osal_memcpy((void *)s_pusBitCode[coreIdx], (const void *)code, size*sizeof(Uint16));
        s_bitCodeSize[coreIdx] = size;
    }
#endif

    return InitializeVPU(coreIdx, code, size);
}

RetCode VPU_DeInit(Uint32 coreIdx)
{
    int ret;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;

    if (EnterLock(coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }
    if (s_pusBitCode[coreIdx]) {
        osal_free((void *)s_pusBitCode[coreIdx]);
    }

    s_pusBitCode[coreIdx] = NULL;
    s_bitCodeSize[coreIdx] = 0;
    LeaveLock(coreIdx);

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
    destroy_sw_uart_thread(coreIdx);
#endif
    mutex_lock(&__vdi_init_mutex);
    ret = vdi_release(coreIdx);
    mutex_unlock(&__vdi_init_mutex);
    if (ret != 0)
        return RETCODE_FAILURE;

    vpu_free_extern_buffers(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode VPU_GetVersionInfo(Uint32 coreIdx, Uint32 *versionInfo, Uint32 *revision, Uint32 *productId)
{
    RetCode  ret;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;

    if (EnterLock(coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if (ProductVpuIsInit(coreIdx) == 0) {
        LeaveLock(coreIdx);
        return RETCODE_NOT_INITIALIZED;
    }

    if (GetPendingInst(coreIdx)) {
        LeaveLock(coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    if (productId != NULL) {
        *productId = ProductVpuGetId(coreIdx);
    }
    ret = ProductVpuGetVersion(coreIdx, versionInfo, revision);

    LeaveLock(coreIdx);

    return ret;
}

RetCode VPU_GetProductInfo(Uint32 coreIdx, VpuAttr *productInfo)
{
    RetCode  ret;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;

    if (EnterLock(coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if (ProductVpuIsInit(coreIdx) == 0) {
        LeaveLock(coreIdx);
        return RETCODE_NOT_INITIALIZED;
    }

    if (GetPendingInst(coreIdx)) {
        LeaveLock(coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }
    productInfo->productId = ProductVpuGetId(coreIdx);
    ret = ProductVpuGetProductInfo(coreIdx, productInfo);

    LeaveLock(coreIdx);

    return ret;
}


RetCode VPU_DecOpen(DecHandle* pHandle, DecOpenParam* pop)
{
    CodecInst * pCodecInst;
    DecInfo*    pDecInfo;
    RetCode     ret;
    VpuAttr*    pAttr;

    if (pop == NULL) {
        VLOG(ERR, "param ptr is null\n");
        return RETCODE_INVALID_PARAM;
    }

    if (EnterLock(pop->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    ret = ProductVpuDecCheckOpenParam(pop);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Invalid open param, ret:%d\n", ret);
        LeaveLock(pop->coreIdx);
        return ret;
    }

    if (ProductVpuIsInit(pop->coreIdx) == 0) {
        LeaveLock(pop->coreIdx);
        return RETCODE_NOT_INITIALIZED;
    }

    ret = GetCodecInstance(pop->coreIdx, &pCodecInst);
    if (ret != RETCODE_SUCCESS)
    {
        *pHandle = 0;
        LeaveLock(pop->coreIdx);
        VLOG(ERR, "GetCodecInstance failed, ret:%d\n", ret);
        return ret;
    }

    pCodecInst->productId = ProductVpuGetId(pop->coreIdx);
    pCodecInst->isDecoder = TRUE;
    *pHandle = pCodecInst;
    pDecInfo = &pCodecInst->CodecInfo->decInfo;
    osal_memset(pDecInfo, 0x00, sizeof(DecInfo));
    osal_memcpy((void*)&pDecInfo->openParam, pop, sizeof(DecOpenParam));

    if (pop->bitstreamFormat == STD_MPEG4) {
        pCodecInst->codecMode    = MP4_DEC;
        pCodecInst->codecModeAux = MP4_AUX_MPEG4;
    }
    else if (pop->bitstreamFormat == STD_AVC) {
        if (PRODUCT_ID_W_SERIES(pCodecInst->productId)) {
            pCodecInst->codecMode    = W_AVC_DEC;
        }
        else {
            pCodecInst->codecMode    = AVC_DEC;
            pCodecInst->codecModeAux = pop->avcExtension;
        }
    }
    else if (pop->bitstreamFormat == STD_VC1) {
        pCodecInst->codecMode = VC1_DEC;
    }
    else if (pop->bitstreamFormat == STD_MPEG2) {
        pCodecInst->codecMode = MP2_DEC;
    }
    else if (pop->bitstreamFormat == STD_H263) {
        pCodecInst->codecMode    = MP4_DEC;
        pCodecInst->codecModeAux = MP4_AUX_MPEG4;
    }
    else if (pop->bitstreamFormat == STD_DIV3) {
        pCodecInst->codecMode    = DV3_DEC;
        pCodecInst->codecModeAux = MP4_AUX_DIVX3;
    }
    else if (pop->bitstreamFormat == STD_RV) {
        pCodecInst->codecMode = RV_DEC;
    }
    else if (pop->bitstreamFormat == STD_AVS) {
        pCodecInst->codecMode = AVS_DEC;
    }
    else if (pop->bitstreamFormat == STD_THO) {
        pCodecInst->codecMode    = VPX_DEC;
        pCodecInst->codecModeAux = VPX_AUX_THO;
    }
    else if (pop->bitstreamFormat == STD_VP3) {
        pCodecInst->codecMode    = VPX_DEC;
        pCodecInst->codecModeAux = VPX_AUX_THO;
    }
    else if (pop->bitstreamFormat == STD_VP8) {
        pCodecInst->codecMode    = VPX_DEC;
        pCodecInst->codecModeAux = VPX_AUX_VP8;
    }
    else if (pop->bitstreamFormat == STD_HEVC) {
        pCodecInst->codecMode    = W_HEVC_DEC;
    }
    else if (pop->bitstreamFormat == STD_VP9) {
        pCodecInst->codecMode    = W_VP9_DEC;
    }
    else if (pop->bitstreamFormat == STD_AVS2) {
        pCodecInst->codecMode    = W_AVS2_DEC;
    }
    else if (pop->bitstreamFormat == STD_AV1) {
        pCodecInst->codecMode    = W_AV1_DEC;
    }
    else {
        LeaveLock(pop->coreIdx);
        VLOG(ERR, "invalid bitstreamFormat:%d\n", pop->bitstreamFormat);
        return RETCODE_INVALID_PARAM;
    }

    pDecInfo->wtlEnable = pop->wtlEnable;
    pDecInfo->wtlMode   = pop->wtlMode;
    if (!pDecInfo->wtlEnable)
        pDecInfo->wtlMode = 0;

    pDecInfo->streamWrPtr        = pop->bitstreamBuffer;
    pDecInfo->streamRdPtr        = pop->bitstreamBuffer;
    pDecInfo->frameDelay         = -1;
    pDecInfo->streamBufStartAddr = pop->bitstreamBuffer;
    pDecInfo->streamBufSize      = pop->bitstreamBufferSize;
    pDecInfo->streamBufEndAddr   = pop->bitstreamBuffer + pop->bitstreamBufferSize;
    pDecInfo->reorderEnable      = pop->reorderEnable;
    pDecInfo->mirrorDirection    = MIRDIR_NONE;
    pDecInfo->tempIdSelectMode   = TEMPORAL_ID_MODE_ABSOLUTE;
    pDecInfo->targetTempId       = DECODE_ALL_TEMPORAL_LAYERS;
    pDecInfo->targetSpatialId    = DECODE_ALL_SPATIAL_LAYERS;

    if ((ret=ProductVpuDecBuildUpOpenParam(pCodecInst, pop)) != RETCODE_SUCCESS) {
        *pHandle = 0;
        LeaveLock(pCodecInst->coreIdx);
        VLOG(ERR, "Invalid build param, ret:%d\n", ret);
        return ret;
    }

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (vdi_open_instance(pCodecInst->coreIdx, pCodecInst->instIndex, pAttr->supportCommandQueue) < 0) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FAILURE;
    }
    pDecInfo->tiled2LinearEnable = pop->tiled2LinearEnable;
    pDecInfo->tiled2LinearMode   = pop->tiled2LinearMode;
    if (!pDecInfo->tiled2LinearEnable)
        pDecInfo->tiled2LinearMode = 0;     //coda980 only

    if (!pDecInfo->wtlEnable)               //coda980, wave320, wave410 only
        pDecInfo->wtlMode = 0;

    osal_memset((void*)&pDecInfo->cacheConfig, 0x00, sizeof(MaverickCacheConfig));
    LeaveLock(pCodecInst->coreIdx);

    return RETCODE_SUCCESS;
}

RetCode VPU_DecClose(DecHandle handle)
{
    CodecInst * pCodecInst;
    DecInfo * pDecInfo;
    RetCode ret;
    int i;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    pCodecInst = handle;
    if (!pCodecInst) {
        return RETCODE_INVALID_HANDLE;
    }
    pDecInfo   = &pCodecInst->CodecInfo->decInfo;

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if ((ret=ProductVpuDecFiniSeq(pCodecInst)) != RETCODE_SUCCESS) {
        if (pCodecInst->loggingEnable)
            vdi_log(pCodecInst->coreIdx, pCodecInst->instIndex, DEC_SEQ_END, 2);

        if (ret == RETCODE_VPU_STILL_RUNNING) {
            LeaveLock(pCodecInst->coreIdx);
            return ret;
        }
    }

    if (pCodecInst->loggingEnable)
        vdi_log(pCodecInst->coreIdx, pCodecInst->instIndex, DEC_SEQ_END, 0);

    if (pDecInfo->vbSlice.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbSlice, DEC_ETC, pCodecInst->instIndex);

    if (pDecInfo->vbWork.size) {
        vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbWork, DEC_WORK, pCodecInst->instIndex);
    }

    if (pDecInfo->vbFrame.size) {
        if (pDecInfo->frameAllocExt == 0) {
            vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFrame, DEC_ETC, pCodecInst->instIndex);
        }
    }
    for ( i=0 ; i<MAX_REG_FRAME; i++) {
        if (pDecInfo->vbMV[i].size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbMV[i], DEC_MV, pCodecInst->instIndex);
        if (pDecInfo->vbFbcYTbl[i].size && pDecInfo->frameBufFlag == 0)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFbcYTbl[i], DEC_FBCY_TBL, pCodecInst->instIndex);
        else if (pDecInfo->frameBufFlag == 1)
            vdi_dettach_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFbcYTbl[i]);
        if (pDecInfo->vbFbcCTbl[i].size && pDecInfo->frameBufFlag == 0)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFbcCTbl[i], DEC_FBCC_TBL, pCodecInst->instIndex);
        else if (pDecInfo->frameBufFlag == 1)
            vdi_dettach_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFbcCTbl[i]);
    }
    if (pDecInfo->vbDefCdf.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbDefCdf, DEC_DEF_CDF, pCodecInst->instIndex);
    if (pDecInfo->vbSegMap.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbSegMap, DEC_ETC, pCodecInst->instIndex);

    if (pDecInfo->vbPPU.size) {
        if (pDecInfo->ppuAllocExt == 0)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbPPU, DEC_ETC, pCodecInst->instIndex);
    }

    if (pDecInfo->vbWTL.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbWTL, DEC_FB_LINEAR, pCodecInst->instIndex);

    if (pDecInfo->vbUserData.size)
        vdi_dettach_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbUserData);

    if (pDecInfo->vbReport.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbReport, DEC_ETC, pCodecInst->instIndex);

    if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
        ClearPendingInst(pCodecInst->coreIdx);

    LeaveLock(pCodecInst->coreIdx);

    FreeCodecInstance(pCodecInst);

    return ret;
}

RetCode VPU_DecSetEscSeqInit(DecHandle handle, int escape)
{
    CodecInst * pCodecInst;
    DecInfo * pDecInfo;
    RetCode ret;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pDecInfo   = &pCodecInst->CodecInfo->decInfo;

    if (pDecInfo->openParam.bitstreamMode != BS_MODE_INTERRUPT)
        return RETCODE_INVALID_PARAM;

    pDecInfo->seqInitEscape = escape;

    return RETCODE_SUCCESS;
}

RetCode VPU_DecIssueSeqInit(DecHandle handle)
{
    CodecInst*  pCodecInst;
    RetCode     ret;
    VpuAttr*    pAttr;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    ret = ProductVpuDecInitSeq(handle);
    if (ret != RETCODE_SUCCESS) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
        return ret;
    }

    if (pAttr->supportCommandQueue == TRUE) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
    }
    else {
        SetPendingInst(pCodecInst->coreIdx, pCodecInst);
    }

    return ret;
}

RetCode VPU_DecCompleteSeqInit(DecHandle handle, DecInitialInfo * info)
{
    CodecInst*  pCodecInst;
    DecInfo*    pDecInfo;
    RetCode     ret;
    VpuAttr*    pAttr;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    if (info == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst = handle;
    pDecInfo = &pCodecInst->CodecInfo->decInfo;

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (pAttr->supportCommandQueue == TRUE) {
        if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }
    }
    else {
        if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(pCodecInst->coreIdx);
            return RETCODE_WRONG_CALL_SEQUENCE;
        }
    }

    ret = ProductVpuDecGetSeqInfo(handle, info);
    if (ret == RETCODE_SUCCESS) {
        pDecInfo->initialInfoObtained = 1;
    }

    info->rdPtr = ProductVpuDecGetRdPtr(pCodecInst);
    info->wrPtr = pDecInfo->streamWrPtr;

    pDecInfo->initialInfo = *info;

    SetPendingInst(pCodecInst->coreIdx, NULL);

    LeaveLock(pCodecInst->coreIdx);

    return ret;
}

static RetCode DecRegisterFrameBuffer(DecHandle handle, FrameBuffer *bufArray, int numFbsForDecoding, int numFbsForWTL, int stride, int height, int mapType)
{
    CodecInst*      pCodecInst;
    DecInfo*        pDecInfo;
    Int32           i;
    RetCode         ret;
    FrameBuffer*    fb, nullFb;
    vpu_buffer_t*   vb;
    FrameBufferFormat format = FORMAT_420;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    if (numFbsForDecoding > MAX_GDI_IDX) {
        return RETCODE_INVALID_PARAM;
    }
    if (numFbsForWTL > MAX_GDI_IDX) {
        return RETCODE_INVALID_PARAM;
    }

    osal_memset(&nullFb, 0x00, sizeof(FrameBuffer));
    pCodecInst                  = handle;
    pDecInfo                    = &pCodecInst->CodecInfo->decInfo;
    pDecInfo->numFbsForDecoding = numFbsForDecoding;
    pDecInfo->numFbsForWTL      = numFbsForWTL;
    pDecInfo->numFrameBuffers   = numFbsForDecoding + numFbsForWTL;
    pDecInfo->stride            = stride;
    if (pCodecInst->codecMode == VPX_DEC)
        pDecInfo->frameBufferHeight = VPU_ALIGN64(height);
    else if (pCodecInst->codecMode == W_VP9_DEC)
        pDecInfo->frameBufferHeight = VPU_ALIGN64(height);
    else
        pDecInfo->frameBufferHeight = height;
    pDecInfo->mapType           = mapType;
    pDecInfo->mapCfg.productId  = pCodecInst->productId;

    if (!pDecInfo->initialInfoObtained)
        return RETCODE_WRONG_CALL_SEQUENCE;

    if ( (stride < pDecInfo->initialInfo.picWidth) || (stride % 8 != 0) || (height<pDecInfo->initialInfo.picHeight) ) {
        return RETCODE_INVALID_STRIDE;
    }

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    ret = ProductVpuDecCheckCapability(pCodecInst);
    if (ret != RETCODE_SUCCESS) {
        LeaveLock(pCodecInst->coreIdx);
        return ret;
    }

    /* clear frameBufPool */
    for (i = 0; i < MAX_REG_FRAME; i++) {
        pDecInfo->frameBufPool[i] = nullFb;
    }

    /* LinearMap or TiledMap, compressed framebuffer inclusive. */
    if (pDecInfo->initialInfo.lumaBitdepth > 8 || pDecInfo->initialInfo.chromaBitdepth > 8)
        format = FORMAT_420_P10_16BIT_LSB;

    if (bufArray) {
        for (i = 0; i < MAX_REG_FRAME; i++)
            pDecInfo->frameBufPool[i] = bufArray[i];
    }
    else {
        vb = &pDecInfo->vbFrame;
        fb = &pDecInfo->frameBufPool[0];
        ret = ProductVpuDecAllocateFramebuffer(
            (CodecInst*)handle, fb, (TiledMapType)mapType, numFbsForDecoding, stride, height, format,
            pDecInfo->openParam.cbcrInterleave,
            pDecInfo->openParam.nv21,
            pDecInfo->openParam.frameEndian, vb, 0, FB_TYPE_CODEC);
        if (ret != RETCODE_SUCCESS) {
            LeaveLock(pCodecInst->coreIdx);
            return ret;
        }
    }
    if (pCodecInst->productId != PRODUCT_ID_960) {
        pDecInfo->mapCfg.tiledBaseAddr = pDecInfo->frameBufPool[0].bufY;
    }

    /* LinearMap */
    if (pDecInfo->wtlEnable == TRUE || numFbsForWTL != 0) {
        pDecInfo->stride = stride;
        if (!bufArray) {
            TiledMapType map;

            map = pDecInfo->wtlMode==FF_FRAME ? LINEAR_FRAME_MAP : LINEAR_FIELD_MAP;
            vb = &pDecInfo->vbWTL;
            fb = &pDecInfo->frameBufPool[numFbsForDecoding];

            ret = ProductVpuDecAllocateFramebuffer(
                (CodecInst*)handle, fb, map, numFbsForWTL, stride, height,
                pDecInfo->wtlFormat,
                pDecInfo->openParam.cbcrInterleave,
                pDecInfo->openParam.nv21,
                pDecInfo->openParam.frameEndian, vb, 0, FB_TYPE_PPU);

            if (ret != RETCODE_SUCCESS) {
                LeaveLock(pCodecInst->coreIdx);
                return ret;
            }
        }
    }

    ret = ProductVpuDecRegisterFramebuffer(pCodecInst);

    LeaveLock(pCodecInst->coreIdx);


    return ret;
}

RetCode VPU_DecRegisterFrameBuffer(DecHandle handle, FrameBuffer *bufArray, int num, int stride, int height, int mapType)
{
    DecInfo* pDecInfo  = &handle->CodecInfo->decInfo;
    Uint32   numWTL    = 0;

    if (pDecInfo->wtlEnable == TRUE) numWTL = num;
    return DecRegisterFrameBuffer(handle, bufArray, num, numWTL, stride, height, mapType);
}

RetCode VPU_DecRegisterFrameBufferEx(DecHandle handle, FrameBuffer *bufArray, int numOfDecFbs, int numOfDisplayFbs, int stride, int height, int mapType)
{
    return DecRegisterFrameBuffer(handle, bufArray, numOfDecFbs, numOfDisplayFbs, stride, height, mapType);
}

RetCode VPU_DecGetFrameBuffer(DecHandle handle, int frameIdx, FrameBuffer* frameBuf)
{
    CodecInst * pCodecInst;
    DecInfo * pDecInfo;
    RetCode ret;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    if (frameBuf == 0)
        return RETCODE_INVALID_PARAM;

    pCodecInst = handle;
    pDecInfo   = &pCodecInst->CodecInfo->decInfo;

    if (frameIdx < 0 || frameIdx >= pDecInfo->numFrameBuffers)
        return RETCODE_INVALID_PARAM;

    *frameBuf = pDecInfo->frameBufPool[frameIdx];

    return RETCODE_SUCCESS;
}

RetCode VPU_DecUpdateFrameBuffer(DecHandle handle, FrameBuffer* fbcFb, FrameBuffer* linearFb, Int32 mvColIndex, Int32 picWidth, Int32 picHeight)
{
    CodecInst* pCodecInst;
    RetCode ret = RETCODE_SUCCESS;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = (CodecInst*)handle;

    if (pCodecInst->codecMode != W_VP9_DEC)
        return RETCODE_NOT_SUPPORTED_FEATURE;

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS)
        return RETCODE_FAILURE;

    ret = ProductVpuDecUpdateFrameBuffer((CodecInst*)handle, fbcFb, linearFb, mvColIndex, picWidth, picHeight);

    LeaveLock(pCodecInst->coreIdx);

    return ret;
}

RetCode VPU_DecGetBitstreamBuffer(DecHandle handle, PhysicalAddress* prdPtr, PhysicalAddress* pwrPtr, Uint32* size)
{
    CodecInst*      pCodecInst;
    DecInfo*        pDecInfo;
    PhysicalAddress rdPtr;
    PhysicalAddress wrPtr;
    PhysicalAddress tempPtr;
    int             room;
    Int32           coreIdx;
    VpuAttr*        pAttr;

    coreIdx   = handle->coreIdx;
    pAttr = &g_VpuCoreAttributes[coreIdx];
    pCodecInst = handle;
    pDecInfo = &pCodecInst->CodecInfo->decInfo;

    if (pAttr->supportCommandQueue == TRUE) {
        if (EnterLock(coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }
        rdPtr = ProductVpuDecGetRdPtr(pCodecInst);
        LeaveLock(pCodecInst->coreIdx);
    }
    else {
        if (GetPendingInst(coreIdx) == pCodecInst) {
            if (pCodecInst->codecMode == AVC_DEC && pCodecInst->codecModeAux == AVC_AUX_MVC) {
                rdPtr = pDecInfo->streamRdPtr;
            }
            else {
                rdPtr = VpuReadReg(coreIdx, pDecInfo->streamRdPtrRegAddr);
                rdPtr = VPU_MapToAddr40Bit(coreIdx, rdPtr);
            }
        }
        else {
            rdPtr = pDecInfo->streamRdPtr;
        }
    }

    wrPtr = pDecInfo->streamWrPtr;

    pAttr = &g_VpuCoreAttributes[coreIdx];

    tempPtr = rdPtr;

    if (pDecInfo->openParam.bitstreamMode != BS_MODE_PIC_END) {
        if (wrPtr < tempPtr) {
            room = tempPtr - wrPtr - pAttr->bitstreamBufferMargin*2;
        }
        else {
            room = (pDecInfo->streamBufEndAddr - wrPtr) + (tempPtr - pDecInfo->streamBufStartAddr) - pAttr->bitstreamBufferMargin*2;
        }
        room--;
    }
    else {
        room = (pDecInfo->streamBufEndAddr - wrPtr);
    }

    if (prdPtr) *prdPtr = tempPtr;
    if (pwrPtr) *pwrPtr = wrPtr;
    if (size)   *size   = room;

    return RETCODE_SUCCESS;
}

RetCode VPU_DecUpdateBitstreamBuffer(DecHandle handle, int size)
{
    CodecInst*      pCodecInst;
    DecInfo*        pDecInfo;
    PhysicalAddress wrPtr;
    PhysicalAddress rdPtr;
    RetCode         ret;
    BOOL            running;
    VpuAttr*        pAttr;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    if (!pCodecInst)
        return RETCODE_INVALID_HANDLE;

    pAttr    = &g_VpuCoreAttributes[pCodecInst->coreIdx];
    pDecInfo = &pCodecInst->CodecInfo->decInfo;
    wrPtr    = pDecInfo->streamWrPtr;

    if (pAttr->supportCommandQueue == TRUE) {
        running = FALSE;
    }
    else {
        running = (BOOL)(GetPendingInst(pCodecInst->coreIdx) == pCodecInst);
    }

    if (size > 0) {
        Uint32      room    = 0;

        if (running == TRUE) {
            rdPtr = VpuReadReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr);
            rdPtr = VPU_MapToAddr40Bit(pCodecInst->coreIdx, rdPtr);
        } else
            rdPtr = pDecInfo->streamRdPtr;

        if (wrPtr < rdPtr) {
            if (rdPtr <= wrPtr + size) {
                return RETCODE_INVALID_PARAM;
            }
        }

        wrPtr += size;

        if (pDecInfo->openParam.bitstreamMode != BS_MODE_PIC_END) {
            if (wrPtr > pDecInfo->streamBufEndAddr) {
                room = wrPtr - pDecInfo->streamBufEndAddr;
                wrPtr = pDecInfo->streamBufStartAddr;
                wrPtr += room;
            }
            else if (wrPtr == pDecInfo->streamBufEndAddr) {
                wrPtr = pDecInfo->streamBufStartAddr;
            }
        }

        pDecInfo->streamWrPtr = wrPtr;
        pDecInfo->streamRdPtr = rdPtr;

        if (running == TRUE) {
            VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamWrPtrRegAddr, wrPtr);
        }
    }

    if (running == FALSE) {
        if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }
    }
    ret = ProductVpuDecSetBitstreamFlag(pCodecInst, running, size);
    if (running == FALSE) {
        LeaveLock(pCodecInst->coreIdx);
    }

    return ret;
}

RetCode VPU_HWReset(Uint32 coreIdx)
{
    RetCode ret = RETCODE_SUCCESS;

    ret = ProductVpuSetBusTransaction(coreIdx, FALSE);
    if (ret != RETCODE_SUCCESS)
        return ret;

    if (vdi_hw_reset(coreIdx) < 0)
        ret = RETCODE_FAILURE;

    ProductVpuSetBusTransaction(coreIdx, TRUE);

    if (GetPendingInst(coreIdx)) {
        SetPendingInst(coreIdx, 0);
        LeaveLock(coreIdx);
    }

    return ret;
}

RetCode VPU_SWReset(Uint32 coreIdx, SWResetMode resetMode, void *pendingInst)
{
    CodecInst *pCodecInst = (CodecInst *)pendingInst;
    RetCode    ret = RETCODE_SUCCESS;
    VpuAttr*   attr = &g_VpuCoreAttributes[coreIdx];

    if (PRODUCT_ID_W6_SERIES(attr->productId)) {
        VLOG(TRACE, "WAVE6 product not support SWReset() function.\n");
        VLOG(TRACE, "Please use the VPU_HWReset() or vdi_hw_reset() after implementing external reset signal.\n");
        return RETCODE_FAILURE;
    }

    if (attr->supportCommandQueue == TRUE) {
        if (pCodecInst && pCodecInst->loggingEnable) {
            vdi_log(coreIdx, pCodecInst->instIndex, 0x10000, 1);
        }

        if (EnterLock(coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }

        ret = ProductVpuReset(coreIdx, resetMode);
        LeaveLock(coreIdx);

        if (pCodecInst && pCodecInst->loggingEnable) {
            vdi_log(pCodecInst->coreIdx, pCodecInst->instIndex, 0x10000, 0);
        }
    }
    else {
        if (pCodecInst) {
            if (pCodecInst->loggingEnable) {
                vdi_log(pCodecInst->coreIdx, pCodecInst->instIndex, (pCodecInst->productId == PRODUCT_ID_960 || pCodecInst->productId == PRODUCT_ID_980)?0x10:0x10000, 1);
            }
        }
        else {
            if (EnterLock(coreIdx) != RETCODE_SUCCESS) {
                return RETCODE_FAILURE;
            }
        }

        ret = ProductVpuReset(coreIdx, resetMode);

        if (pCodecInst) {
            if (pCodecInst->loggingEnable) {
                vdi_log(pCodecInst->coreIdx, pCodecInst->instIndex, (pCodecInst->productId == PRODUCT_ID_960 || pCodecInst->productId == PRODUCT_ID_980)?0x10:0x10000, 0);
            }
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(coreIdx);
        }
        else {
            LeaveLock(coreIdx);
        }
    }

    return ret;
}

RetCode VPU_SleepWake(Uint32 coreIdx, int iSleepWake)
{
    RetCode ret;

    if (EnterLock(coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    ret = ProductVpuSleepWake(coreIdx, iSleepWake, s_pusBitCode[coreIdx], s_bitCodeSize[coreIdx]);
    LeaveLock(coreIdx);

    return ret;
}

RetCode VPU_DecStartOneFrame(DecHandle handle, DecParam *param)
{
    CodecInst*  pCodecInst;
    DecInfo*    pDecInfo;
    Uint32      val     = 0;
    RetCode     ret     = RETCODE_SUCCESS;
    VpuAttr*    pAttr   = NULL;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    pCodecInst = (CodecInst*)handle;
    pDecInfo   = &pCodecInst->CodecInfo->decInfo;

    if (pDecInfo->stride == 0) { // This means frame buffers have not been registered.
        return RETCODE_WRONG_CALL_SEQUENCE;
    }

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    if (pAttr->supportCommandQueue == FALSE) {
        EnterDispFlagLock(pCodecInst->coreIdx);
        val  = pDecInfo->frameDisplayFlag;
        val |= pDecInfo->setDisplayIndexes;
        val &= ~(Uint32)(pDecInfo->clearDisplayIndexes);
        VpuWriteReg(pCodecInst->coreIdx, pDecInfo->frameDisplayFlagRegAddr, val);
        pDecInfo->clearDisplayIndexes = 0;
        pDecInfo->setDisplayIndexes = 0;
        LeaveDispFlagLock(pCodecInst->coreIdx);
    }

    pDecInfo->frameStartPos = pDecInfo->streamRdPtr;

    ret = ProductVpuDecode(pCodecInst, param);
    if (ret != RETCODE_SUCCESS) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
        return ret;
    }

    if (pAttr->supportCommandQueue == TRUE) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
    }
    else {
        SetPendingInst(pCodecInst->coreIdx, pCodecInst);
    }

    return ret;
}

RetCode VPU_DecGetOutputInfo(DecHandle handle, DecOutputInfo* info)
{
    CodecInst*  pCodecInst;
    DecInfo*    pDecInfo;
    RetCode     ret;
    VpuRect     rectInfo;
    Uint32      val;
    Int32       decodedIndex;
    Int32       displayIndex;
    Uint32      maxDecIndex;
    VpuAttr*    pAttr;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    if (info == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst  = handle;
    pDecInfo    = &pCodecInst->CodecInfo->decInfo;
    pAttr       = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (pAttr->supportCommandQueue == TRUE) {
        if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }
    }
    else {
        if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(pCodecInst->coreIdx);
            return RETCODE_WRONG_CALL_SEQUENCE;
        }
    }

    osal_memset((void*)info, 0x00, sizeof(DecOutputInfo));

    ret = ProductVpuDecGetResult(pCodecInst, info);
    if (ret != RETCODE_SUCCESS) {
        info->rdPtr = pDecInfo->streamRdPtr;
        info->wrPtr = pDecInfo->streamWrPtr;
        SetPendingInst(pCodecInst->coreIdx, 0);
        LeaveLock(pCodecInst->coreIdx);
        return ret;
    }

    decodedIndex = info->indexFrameDecoded;

    // Calculate display frame region
    val = 0;
    if (decodedIndex >= 0 && decodedIndex < MAX_GDI_IDX) {
        //default value
        rectInfo.left   = 0;
        rectInfo.right  = info->decPicWidth;
        rectInfo.top    = 0;
        rectInfo.bottom = info->decPicHeight;

        if (PRODUCT_ID_W_SERIES(pCodecInst->productId)) {
            if (pCodecInst->codecMode == W_HEVC_DEC || pCodecInst->codecMode == W_AVC_DEC)
                rectInfo = pDecInfo->initialInfo.picCropRect;

            if (pCodecInst->codecMode == W_HEVC_DEC)
                pDecInfo->decOutInfo[decodedIndex].decodedPOC = info->decodedPOC;

            if (pCodecInst->codecMode == W_AVC_DEC)
                pDecInfo->decOutInfo[decodedIndex].decodedPOC = info->decodedPOC;

            if (pCodecInst->codecMode == W_AVS2_DEC)
                pDecInfo->decOutInfo[decodedIndex].avs2Info.decodedPOI = info->avs2Info.decodedPOI;
        } else {
            if (pCodecInst->codecMode == AVC_DEC || pCodecInst->codecMode == AVS_DEC)
                rectInfo = pDecInfo->initialInfo.picCropRect;
        }

        info->rcDecoded.left   =  pDecInfo->decOutInfo[decodedIndex].rcDecoded.left   = rectInfo.left;
        info->rcDecoded.right  =  pDecInfo->decOutInfo[decodedIndex].rcDecoded.right  = rectInfo.right;
        info->rcDecoded.top    =  pDecInfo->decOutInfo[decodedIndex].rcDecoded.top    = rectInfo.top;
        info->rcDecoded.bottom =  pDecInfo->decOutInfo[decodedIndex].rcDecoded.bottom = rectInfo.bottom;
    }
    else
    {
        info->rcDecoded.left   = 0;
        info->rcDecoded.right  = info->decPicWidth;
        info->rcDecoded.top    = 0;
        info->rcDecoded.bottom = info->decPicHeight;
    }

    displayIndex = info->indexFrameDisplay;
    if (info->indexFrameDisplay >= 0 && info->indexFrameDisplay < MAX_GDI_IDX)
    {
        if (pCodecInst->codecMode == VC1_DEC) // vc1 rotates decoded frame buffer region. the other std rotated whole frame buffer region.
        {
            if (pDecInfo->rotationEnable && (pDecInfo->rotationAngle==90 || pDecInfo->rotationAngle==270))
            {
                info->rcDisplay.left   = pDecInfo->decOutInfo[displayIndex].rcDecoded.top;
                info->rcDisplay.right  = pDecInfo->decOutInfo[displayIndex].rcDecoded.bottom;
                info->rcDisplay.top    = pDecInfo->decOutInfo[displayIndex].rcDecoded.left;
                info->rcDisplay.bottom = pDecInfo->decOutInfo[displayIndex].rcDecoded.right;
            }
            else
            {
                info->rcDisplay.left   = pDecInfo->decOutInfo[displayIndex].rcDecoded.left;
                info->rcDisplay.right  = pDecInfo->decOutInfo[displayIndex].rcDecoded.right;
                info->rcDisplay.top    = pDecInfo->decOutInfo[displayIndex].rcDecoded.top;
                info->rcDisplay.bottom = pDecInfo->decOutInfo[displayIndex].rcDecoded.bottom;
            }
        }
        else
        {
            if (pDecInfo->rotationEnable)
            {
                switch(pDecInfo->rotationAngle)
                {
                case 90:
                    info->rcDisplay.left   = pDecInfo->decOutInfo[displayIndex].rcDecoded.top;
                    info->rcDisplay.right  = pDecInfo->decOutInfo[displayIndex].rcDecoded.bottom;
                    info->rcDisplay.top    = pDecInfo->decOutInfo[displayIndex].decPicWidth - pDecInfo->decOutInfo[displayIndex].rcDecoded.right;
                    info->rcDisplay.bottom = pDecInfo->decOutInfo[displayIndex].decPicWidth - pDecInfo->decOutInfo[displayIndex].rcDecoded.left;
                    break;
                case 270:
                    info->rcDisplay.left   = pDecInfo->decOutInfo[displayIndex].decPicHeight - pDecInfo->decOutInfo[displayIndex].rcDecoded.bottom;
                    info->rcDisplay.right  = pDecInfo->decOutInfo[displayIndex].decPicHeight - pDecInfo->decOutInfo[displayIndex].rcDecoded.top;
                    info->rcDisplay.top    = pDecInfo->decOutInfo[displayIndex].rcDecoded.left;
                    info->rcDisplay.bottom = pDecInfo->decOutInfo[displayIndex].rcDecoded.right;
                    break;
                case 180:
                    info->rcDisplay.left   = pDecInfo->decOutInfo[displayIndex].rcDecoded.left;
                    info->rcDisplay.right  = pDecInfo->decOutInfo[displayIndex].rcDecoded.right;
                    info->rcDisplay.top    = pDecInfo->decOutInfo[displayIndex].decPicHeight - pDecInfo->decOutInfo[displayIndex].rcDecoded.bottom;
                    info->rcDisplay.bottom = pDecInfo->decOutInfo[displayIndex].decPicHeight - pDecInfo->decOutInfo[displayIndex].rcDecoded.top;
                    break;
                default:
                    info->rcDisplay.left   = pDecInfo->decOutInfo[displayIndex].rcDecoded.left;
                    info->rcDisplay.right  = pDecInfo->decOutInfo[displayIndex].rcDecoded.right;
                    info->rcDisplay.top    = pDecInfo->decOutInfo[displayIndex].rcDecoded.top;
                    info->rcDisplay.bottom = pDecInfo->decOutInfo[displayIndex].rcDecoded.bottom;
                    break;
                }

            }
            else
            {
                info->rcDisplay.left   = pDecInfo->decOutInfo[displayIndex].rcDecoded.left;
                info->rcDisplay.right  = pDecInfo->decOutInfo[displayIndex].rcDecoded.right;
                info->rcDisplay.top    = pDecInfo->decOutInfo[displayIndex].rcDecoded.top;
                info->rcDisplay.bottom = pDecInfo->decOutInfo[displayIndex].rcDecoded.bottom;
            }

            if (pDecInfo->mirrorEnable) {
                Uint32 temp;
                Uint32 width  = (pDecInfo->rotationAngle == 90 || pDecInfo->rotationAngle == 270) ? info->decPicHeight: info->decPicWidth;
                Uint32 height = (pDecInfo->rotationAngle == 90 || pDecInfo->rotationAngle == 270) ? info->decPicWidth : info->decPicHeight;
                if (pDecInfo->mirrorDirection & MIRDIR_VER) {
                    temp = info->rcDisplay.top;
                    info->rcDisplay.top    = height - info->rcDisplay.bottom;
                    info->rcDisplay.bottom = height - temp;
                }
                if (pDecInfo->mirrorDirection & MIRDIR_HOR) {
                    temp = info->rcDisplay.left;
                    info->rcDisplay.left  = width - info->rcDisplay.right;
                    info->rcDisplay.right = width - temp;
                }
            }

            if (PRODUCT_ID_W_SERIES(pCodecInst->productId)) {
                switch (pCodecInst->codecMode) {
                case W_HEVC_DEC:
                    info->displayPOC = pDecInfo->decOutInfo[displayIndex].decodedPOC;
                    break;
                case W_AVS2_DEC:
                    info->avs2Info.displayPOI = pDecInfo->decOutInfo[displayIndex].avs2Info.decodedPOI;
                    break;
                case W_AVC_DEC:
                    info->displayPOC = pDecInfo->decOutInfo[displayIndex].decodedPOC;
                    break;
                default:
                    break;
                }
            }
        }

        if (info->indexFrameDisplay == info->indexFrameDecoded)
        {
            info->dispPicWidth =  info->decPicWidth;
            info->dispPicHeight = info->decPicHeight;
        }
        else
        {
            /*
                When indexFrameDecoded < 0, and indexFrameDisplay >= 0
                info->decPicWidth and info->decPicHeight are still valid
                But those of pDecInfo->decOutInfo[displayIndex] are invalid in VP9
            */
            info->dispPicWidth  = pDecInfo->decOutInfo[displayIndex].decPicWidth;
            info->dispPicHeight = pDecInfo->decOutInfo[displayIndex].decPicHeight;
        }

    }
    else
    {
        info->rcDisplay.left   = 0;
        info->rcDisplay.right  = 0;
        info->rcDisplay.top    = 0;
        info->rcDisplay.bottom = 0;

        if (pDecInfo->rotationEnable || pDecInfo->mirrorEnable || pDecInfo->tiled2LinearEnable || pDecInfo->deringEnable) {
            info->dispPicWidth  = info->decPicWidth;
            info->dispPicHeight = info->decPicHeight;
        }
        else {
            info->dispPicWidth = 0;
            info->dispPicHeight = 0;
        }
    }

    if ( (pCodecInst->codecMode == VC1_DEC ) && info->indexFrameDisplay != -3) {
        if (pDecInfo->vc1BframeDisplayValid == 0) {
            if (info->picType == 2)
                info->indexFrameDisplay = -3;
            else
                pDecInfo->vc1BframeDisplayValid = 1;
        }
    }

    pDecInfo->streamRdPtr      = ProductVpuDecGetRdPtr(pCodecInst);
    pDecInfo->frameDisplayFlag = VpuReadReg(pCodecInst->coreIdx, pDecInfo->frameDisplayFlagRegAddr);
    if (pCodecInst->codecMode == W_VP9_DEC) {
        pDecInfo->frameDisplayFlag  &= 0xFFFF;
    }
    pDecInfo->frameEndPos      = pDecInfo->streamRdPtr;

    if (pDecInfo->frameEndPos < pDecInfo->frameStartPos)
        info->consumedByte = pDecInfo->frameEndPos + pDecInfo->streamBufSize - pDecInfo->frameStartPos;
    else
        info->consumedByte = pDecInfo->frameEndPos - pDecInfo->frameStartPos;

    if (pDecInfo->deringEnable || pDecInfo->mirrorEnable || pDecInfo->rotationEnable || pDecInfo->tiled2LinearEnable) {
        info->dispFrame        = pDecInfo->rotatorOutput;
        info->dispFrame.stride = pDecInfo->rotatorStride;
    }
    else {
        val = (pDecInfo->openParam.wtlEnable == TRUE ? pDecInfo->numFbsForDecoding: 0); //fbOffset

        maxDecIndex = (pDecInfo->numFbsForDecoding > pDecInfo->numFbsForWTL) ? pDecInfo->numFbsForDecoding : pDecInfo->numFbsForWTL;

        if (0 <= info->indexFrameDisplay && info->indexFrameDisplay < (int)maxDecIndex)
            info->dispFrame = pDecInfo->frameBufPool[val+info->indexFrameDisplay];
    }

    info->rdPtr            = pDecInfo->streamRdPtr;
    info->wrPtr            = pDecInfo->streamWrPtr;
    info->frameDisplayFlag = pDecInfo->frameDisplayFlag;

    info->sequenceNo = pDecInfo->initialInfo.sequenceNo;
    if (decodedIndex >= 0 && decodedIndex < MAX_GDI_IDX) {
        pDecInfo->decOutInfo[decodedIndex] = *info;
    }

    if (displayIndex >= 0 && displayIndex < MAX_GDI_IDX) {
        info->numOfTotMBsInDisplay = pDecInfo->decOutInfo[displayIndex].numOfTotMBs;
        info->numOfErrMBsInDisplay = pDecInfo->decOutInfo[displayIndex].numOfErrMBs;
        info->dispFrame.sequenceNo = info->sequenceNo;
    }
    else {
        info->numOfTotMBsInDisplay = 0;
        info->numOfErrMBsInDisplay = 0;
    }

    if (info->sequenceChanged != 0) {
        if (!(pCodecInst->productId == PRODUCT_ID_960 || pCodecInst->productId == PRODUCT_ID_980)) {
            /* Update new sequence information */
            osal_memcpy((void*)&pDecInfo->initialInfo, (void*)&pDecInfo->newSeqInfo, sizeof(DecInitialInfo));
        }
        if ((info->sequenceChanged & SEQ_CHANGE_INTER_RES_CHANGE) != SEQ_CHANGE_INTER_RES_CHANGE) {
            pDecInfo->initialInfo.sequenceNo++;
        }
    }

    SetPendingInst(pCodecInst->coreIdx, 0);

    LeaveLock(pCodecInst->coreIdx);

    return RETCODE_SUCCESS;
}

RetCode VPU_DecFrameBufferFlush(DecHandle handle, DecOutputInfo* pRemainings, Uint32* retNum)
{
    CodecInst*       pCodecInst;
    DecInfo*         pDecInfo;
    DecOutputInfo*   pOut;
    RetCode          ret;
    FramebufferIndex retIndex[MAX_GDI_IDX];
    Uint32           retRemainings = 0;
    Int32            i, index, val;
    VpuAttr*         pAttr      = NULL;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pDecInfo = &pCodecInst->CodecInfo->decInfo;

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    osal_memset((void*)retIndex, 0xff, sizeof(retIndex));

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];
    if (pAttr->supportCommandQueue == FALSE) {
        EnterDispFlagLock(pCodecInst->coreIdx);
        val  = pDecInfo->frameDisplayFlag;
        val |= pDecInfo->setDisplayIndexes;
        val &= ~(Uint32)(pDecInfo->clearDisplayIndexes);
        VpuWriteReg(pCodecInst->coreIdx, pDecInfo->frameDisplayFlagRegAddr, val);
        pDecInfo->clearDisplayIndexes = 0;
        pDecInfo->setDisplayIndexes = 0;
        LeaveDispFlagLock(pCodecInst->coreIdx);
    }

    if ((ret=ProductVpuDecFlush(pCodecInst, retIndex, MAX_GDI_IDX)) != RETCODE_SUCCESS) {
        LeaveLock(pCodecInst->coreIdx);
        return ret;
    }

    if (pRemainings != NULL) {
        for (i=0; i<MAX_GDI_IDX; i++) {
            index = (pDecInfo->wtlEnable == TRUE) ? retIndex[i].tiledIndex : retIndex[i].linearIndex;
            if (index < 0)
                break;
            pRemainings[i] = pDecInfo->decOutInfo[index];
            pOut = &pRemainings[i];
            pOut->indexFrameDisplay         = pOut->indexFrameDecoded;
            pOut->indexFrameDisplayForTiled = pOut->indexFrameDecodedForTiled;
            if (pDecInfo->wtlEnable == TRUE)
                pOut->dispFrame             = pDecInfo->frameBufPool[pDecInfo->numFbsForDecoding+retIndex[i].linearIndex];
            else
                pOut->dispFrame             = pDecInfo->frameBufPool[index];

            pOut->dispFrame.sequenceNo = pOut->sequenceNo;
            pOut->dispPicWidth         = pOut->decPicWidth;
            pOut->dispPicHeight        = pOut->decPicHeight;

            if (pDecInfo->rotationEnable)
            {
                switch(pDecInfo->rotationAngle)
                {
                case 90:
                    pOut->rcDisplay.left   = pDecInfo->decOutInfo[index].rcDecoded.top;
                    pOut->rcDisplay.right  = pDecInfo->decOutInfo[index].rcDecoded.bottom;
                    pOut->rcDisplay.top    = pOut->decPicWidth - pDecInfo->decOutInfo[index].rcDecoded.right;
                    pOut->rcDisplay.bottom = pOut->decPicWidth - pDecInfo->decOutInfo[index].rcDecoded.left;
                    break;
                case 270:
                    pOut->rcDisplay.left   = pOut->decPicHeight - pDecInfo->decOutInfo[index].rcDecoded.bottom;
                    pOut->rcDisplay.right  = pOut->decPicHeight - pDecInfo->decOutInfo[index].rcDecoded.top;
                    pOut->rcDisplay.top    = pDecInfo->decOutInfo[index].rcDecoded.left;
                    pOut->rcDisplay.bottom = pDecInfo->decOutInfo[index].rcDecoded.right;
                    break;
                case 180:
                    pOut->rcDisplay.left   = pDecInfo->decOutInfo[index].rcDecoded.left;
                    pOut->rcDisplay.right  = pDecInfo->decOutInfo[index].rcDecoded.right;
                    pOut->rcDisplay.top    = pOut->decPicHeight - pDecInfo->decOutInfo[index].rcDecoded.bottom;
                    pOut->rcDisplay.bottom = pOut->decPicHeight - pDecInfo->decOutInfo[index].rcDecoded.top;
                    break;
                default:
                    pOut->rcDisplay.left   = pDecInfo->decOutInfo[index].rcDecoded.left;
                    pOut->rcDisplay.right  = pDecInfo->decOutInfo[index].rcDecoded.right;
                    pOut->rcDisplay.top    = pDecInfo->decOutInfo[index].rcDecoded.top;
                    pOut->rcDisplay.bottom = pDecInfo->decOutInfo[index].rcDecoded.bottom;
                    break;
                }
            }
            else
            {
                pOut->rcDisplay.left   = pDecInfo->decOutInfo[index].rcDecoded.left;
                pOut->rcDisplay.right  = pDecInfo->decOutInfo[index].rcDecoded.right;
                pOut->rcDisplay.top    = pDecInfo->decOutInfo[index].rcDecoded.top;
                pOut->rcDisplay.bottom = pDecInfo->decOutInfo[index].rcDecoded.bottom;
            }
            retRemainings++;
        }
    }

    if (retNum) *retNum = retRemainings;

    if (pCodecInst->loggingEnable)
        vdi_log(pCodecInst->coreIdx, pCodecInst->instIndex, DEC_BUF_FLUSH, 0);

    LeaveLock(pCodecInst->coreIdx);

    return ret;
}

RetCode VPU_DecSetRdPtr(DecHandle handle, PhysicalAddress addr, int updateWrPtr)
{
    CodecInst* pCodecInst;
    CodecInst* pPendingInst;
    DecInfo*   pDecInfo;
    VpuAttr*   pAttr = NULL;
    RetCode    ret;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    pCodecInst   = handle;
    pDecInfo     = &pCodecInst->CodecInfo->decInfo;
    pPendingInst = GetPendingInst(pCodecInst->coreIdx);
    pAttr        = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (pAttr->supportCommandQueue == TRUE) {
        if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }
        ProductVpuDecSetRdPtr(pCodecInst, addr);
        LeaveLock(pCodecInst->coreIdx);
    }
    else {
        if (pCodecInst == pPendingInst) {
            VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr, addr);
        }
        else {
            if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
                return RETCODE_FAILURE;
            }
            VpuWriteReg(pCodecInst->coreIdx, pDecInfo->streamRdPtrRegAddr, addr);
            LeaveLock(pCodecInst->coreIdx);
        }
    }

    pDecInfo->streamRdPtr = addr;
    if (updateWrPtr == TRUE) {
        pDecInfo->streamWrPtr = addr;
    }

    return RETCODE_SUCCESS;
}



RetCode VPU_DecClrDispFlag(DecHandle handle, int index)
{
    CodecInst*  pCodecInst;
    DecInfo*    pDecInfo;
    RetCode     ret         = RETCODE_SUCCESS;
    Int32       endIndex;
    VpuAttr*    pAttr       = NULL;
    BOOL        supportCommandQueue;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pDecInfo   = &pCodecInst->CodecInfo->decInfo;
    pAttr      = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    endIndex = (pDecInfo->openParam.wtlEnable == TRUE) ? pDecInfo->numFbsForWTL : pDecInfo->numFbsForDecoding;

    if ((index < 0) || (index > (endIndex - 1))) {
        return RETCODE_INVALID_PARAM;
    }

    supportCommandQueue = (pAttr->supportCommandQueue == TRUE);
    if (supportCommandQueue == TRUE) {
        if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }
        ret = ProductVpuDecClrDispFlag(pCodecInst, index);
        LeaveLock(pCodecInst->coreIdx);
    }
    else {
        EnterDispFlagLock(pCodecInst->coreIdx);
        pDecInfo->clearDisplayIndexes |= (1<<index);
        LeaveDispFlagLock(pCodecInst->coreIdx);
    }

    return ret;
}

RetCode VPU_DecGiveCommand(DecHandle handle, CodecCommand cmd, void* param)
{
    CodecInst*          pCodecInst;
    DecInfo*            pDecInfo;
    RetCode             ret;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pDecInfo = &pCodecInst->CodecInfo->decInfo;
    switch (cmd)
    {
    case ENABLE_ROTATION :
        {
            if (pDecInfo->rotatorStride == 0) {
                return RETCODE_ROTATOR_STRIDE_NOT_SET;
            }
            pDecInfo->rotationEnable = 1;
            break;
        }

    case DISABLE_ROTATION :
        {
            pDecInfo->rotationEnable = 0;
            break;
        }

    case ENABLE_MIRRORING :
        {
            if (pDecInfo->rotatorStride == 0) {
                return RETCODE_ROTATOR_STRIDE_NOT_SET;
            }
            pDecInfo->mirrorEnable = 1;
            break;
        }
    case DISABLE_MIRRORING :
        {
            pDecInfo->mirrorEnable = 0;
            break;
        }
    case SET_MIRROR_DIRECTION :
        {

            MirrorDirection mirDir;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            mirDir = *(MirrorDirection *)param;
            if ( !(mirDir == MIRDIR_NONE) && !(mirDir==MIRDIR_HOR) && !(mirDir==MIRDIR_VER) && !(mirDir==MIRDIR_HOR_VER)) {
                return RETCODE_INVALID_PARAM;
            }
            pDecInfo->mirrorDirection = mirDir;

            break;
        }
    case SET_ROTATION_ANGLE :
        {
            int angle;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            angle = *(int *)param;
            if (angle != 0 && angle != 90 &&
                angle != 180 && angle != 270) {
                    return RETCODE_INVALID_PARAM;
            }
            if (pDecInfo->rotatorStride != 0) {
                if (angle == 90 || angle ==270) {
                    if (pDecInfo->initialInfo.picHeight > pDecInfo->rotatorStride) {
                        return RETCODE_INVALID_PARAM;
                    }
                } else {
                    if (pDecInfo->initialInfo.picWidth > pDecInfo->rotatorStride) {
                        return RETCODE_INVALID_PARAM;
                    }
                }
            }

            pDecInfo->rotationAngle = angle;
            break;
        }
    case SET_ROTATOR_OUTPUT :
        {
            FrameBuffer *frame;
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            frame = (FrameBuffer *)param;

            pDecInfo->rotatorOutput = *frame;
            pDecInfo->rotatorOutputValid = 1;
            break;
        }

    case SET_ROTATOR_STRIDE :
        {
            int stride;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            stride = *(int *)param;
            if (stride % 8 != 0 || stride == 0) {
                return RETCODE_INVALID_STRIDE;
            }

            if (pDecInfo->rotationAngle == 90 || pDecInfo->rotationAngle == 270) {
                if (pDecInfo->initialInfo.picHeight > stride) {
                    return RETCODE_INVALID_STRIDE;
                }
            } else {
                if (pDecInfo->initialInfo.picWidth > stride) {
                    return RETCODE_INVALID_STRIDE;
                }
            }

            pDecInfo->rotatorStride = stride;
            break;
        }
    case DEC_SET_SPS_RBSP:
        {
            if (pCodecInst->codecMode != AVC_DEC) {
                return RETCODE_INVALID_COMMAND;
            }
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }

            return SetParaSet(handle, 0, (DecParamSet *)param);

        }

    case DEC_SET_PPS_RBSP:
        {
            if (pCodecInst->codecMode != AVC_DEC) {
                return RETCODE_INVALID_COMMAND;
            }
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }

            return SetParaSet(handle, 1, (DecParamSet *)param);
        }
    case ENABLE_DERING :
        {
            if (pDecInfo->rotatorStride == 0) {
                return RETCODE_ROTATOR_STRIDE_NOT_SET;
            }
            pDecInfo->deringEnable = 1;
            break;
        }

    case DISABLE_DERING :
        {
            pDecInfo->deringEnable = 0;
            break;
        }
    case SET_SEC_AXI:
        {
            ret = ProductVpuDecGiveCommand(pCodecInst, SET_SEC_AXI, param);
            return ret;
        }
    case ENABLE_REP_USERDATA:
        {
            if (!pDecInfo->userDataBufAddr) {
                return RETCODE_USERDATA_BUF_NOT_SET;
            }
            if (pDecInfo->userDataBufSize == 0) {
                return RETCODE_USERDATA_BUF_NOT_SET;
            }
            switch (pCodecInst->productId) {
            case PRODUCT_ID_521:
            case PRODUCT_ID_511:
            case PRODUCT_ID_517:
            case PRODUCT_ID_617:
            case PRODUCT_ID_627:
            case PRODUCT_ID_637:
                if (!param)
                    return RETCODE_INVALID_PARAM;
                pDecInfo->userDataEnable = *(Uint32*)param;
                break;
            case PRODUCT_ID_960:
            case PRODUCT_ID_980:
                pDecInfo->userDataEnable = TRUE;
                break;
            default:
                VLOG(INFO, "%s(ENABLE_REP_DATA) invalid productId(%d)\n", __FUNCTION__, pCodecInst->productId);
                return RETCODE_INVALID_PARAM;
            }
            break;
        }
    case DISABLE_REP_USERDATA:
        {
            pDecInfo->userDataEnable = 0;
            break;
        }
    case SET_ADDR_REP_USERDATA:
        {
            PhysicalAddress userDataBufAddr;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            userDataBufAddr = *(PhysicalAddress *)param;
            if (userDataBufAddr % 8 != 0 || userDataBufAddr == 0) {
                return RETCODE_INVALID_PARAM;
            }

            pDecInfo->userDataBufAddr = userDataBufAddr;
            break;
        }
    case SET_VIRT_ADDR_REP_USERDATA:
        {
            unsigned long userDataVirtAddr;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }

            if (!pDecInfo->userDataBufAddr) {
                return RETCODE_USERDATA_BUF_NOT_SET;
            }
            if (pDecInfo->userDataBufSize == 0) {
                return RETCODE_USERDATA_BUF_NOT_SET;
            }

            userDataVirtAddr = *(unsigned long *)param;
            if (!userDataVirtAddr) {
                return RETCODE_INVALID_PARAM;
            }

            pDecInfo->vbUserData.phys_addr = pDecInfo->userDataBufAddr;
            pDecInfo->vbUserData.size = pDecInfo->userDataBufSize;
            pDecInfo->vbUserData.virt_addr = (unsigned long)userDataVirtAddr;
            if (vdi_attach_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbUserData) != 0) {
                return RETCODE_INSUFFICIENT_RESOURCE;
            }
            break;
        }
    case SET_SIZE_REP_USERDATA:
        {
            PhysicalAddress userDataBufSize;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            userDataBufSize = *(PhysicalAddress *)param;

            pDecInfo->userDataBufSize = userDataBufSize;
            break;
        }

    case SET_USERDATA_REPORT_MODE:
        {
            int userDataMode;

            userDataMode = *(int *)param;
            if (userDataMode != 1 && userDataMode != 0) {
                return RETCODE_INVALID_PARAM;
            }
            pDecInfo->userDataReportMode = userDataMode;
            break;
        }
     case SET_CACHE_CONFIG:
        {
            MaverickCacheConfig *mcCacheConfig;
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            mcCacheConfig = (MaverickCacheConfig *)param;
            pDecInfo->cacheConfig = *mcCacheConfig;
        }
        break;
    case SET_LOW_DELAY_CONFIG:
        {
            LowDelayInfo *lowDelayInfo;
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            if (pCodecInst->productId != PRODUCT_ID_980) {
                return RETCODE_NOT_SUPPORTED_FEATURE;
            }
            lowDelayInfo = (LowDelayInfo *)param;

            if (lowDelayInfo->lowDelayEn) {
                if ( (pCodecInst->codecMode != AVC_DEC) ||
                    pDecInfo->rotationEnable ||
                    pDecInfo->mirrorEnable ||
                    pDecInfo->tiled2LinearEnable ||
                    pDecInfo->deringEnable) {
                        return RETCODE_INVALID_PARAM;
                }
            }

            pDecInfo->lowDelayInfo.lowDelayEn = lowDelayInfo->lowDelayEn;
            pDecInfo->lowDelayInfo.numRows    = lowDelayInfo->numRows;
        }
        break;

     case DEC_SET_FRAME_DELAY:
        {
            pDecInfo->frameDelay = *(int *)param;
            break;
        }
    case DEC_ENABLE_REORDER:
        {
            if((handle->productId == PRODUCT_ID_980) || (handle->productId == PRODUCT_ID_960))
            {
                if (pDecInfo->initialInfoObtained) {
                    return RETCODE_WRONG_CALL_SEQUENCE;
                }
            }

            pDecInfo->reorderEnable = TRUE;
            break;
        }
    case DEC_DISABLE_REORDER:
        {
            if((handle->productId == PRODUCT_ID_980) || (handle->productId == PRODUCT_ID_960))
            {
                if (pDecInfo->initialInfoObtained) {
                    return RETCODE_WRONG_CALL_SEQUENCE;
                }

                if(pCodecInst->codecMode != AVC_DEC && pCodecInst->codecMode != VC1_DEC && pCodecInst->codecMode != AVS_DEC) {
                    return RETCODE_INVALID_COMMAND;
                }
            }

            pDecInfo->reorderEnable = FALSE;
            break;
        }
    case DEC_SET_AVC_ERROR_CONCEAL_MODE:
        {
            if(pCodecInst->codecMode != AVC_DEC) {
                return RETCODE_INVALID_COMMAND;
            }

            pDecInfo->avcErrorConcealMode = *(int *)param;
            break;
        }
    case DEC_FREE_FRAME_BUFFER:
        {
            int i;
            if (pDecInfo->vbSlice.size)
                vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbSlice, DEC_ETC, pCodecInst->instIndex);

            if (pDecInfo->vbFrame.size){
                if (pDecInfo->frameAllocExt == 0)
                    vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFrame, DEC_ETC, pCodecInst->instIndex);
            }
            for (i=0  ; i<MAX_REG_FRAME; i++) {
                if (pDecInfo->vbFbcYTbl[i].size && pDecInfo->frameBufFlag == 0)
                    vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFbcYTbl[i], DEC_FBCY_TBL, pCodecInst->instIndex);
                else if (pDecInfo->frameBufFlag == 1)
                    vdi_dettach_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFbcYTbl[i]);

                if (pDecInfo->vbFbcCTbl[i].size && pDecInfo->frameBufFlag == 0)
                    vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFbcCTbl[i], DEC_FBCC_TBL, pCodecInst->instIndex);
                else if (pDecInfo->frameBufFlag == 1)
                    vdi_dettach_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbFbcCTbl[i]);

                if (pDecInfo->vbMV[i].size)
                    vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbMV[i], DEC_MV, pCodecInst->instIndex);
            }

            if (pDecInfo->vbPPU.size) {
                if (pDecInfo->ppuAllocExt == 0)
                    vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbPPU, DEC_ETC, pCodecInst->instIndex);
            }

            if (pDecInfo->wtlEnable) {
                if (pDecInfo->vbWTL.size)
                    vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbWTL, DEC_FB_LINEAR, pCodecInst->instIndex);
            }

            if (pDecInfo->vbTask.size)
                vdi_free_dma_memory(pCodecInst->coreIdx, &pDecInfo->vbTask, DEC_TASK, pCodecInst->instIndex);
            break;
        }
    case DEC_GET_FRAMEBUF_INFO:
        {
            DecGetFramebufInfo* fbInfo = (DecGetFramebufInfo*)param;
            Uint32 i;
            fbInfo->vbFrame = pDecInfo->vbFrame;
            fbInfo->vbWTL   = pDecInfo->vbWTL;
            fbInfo->vbTask  = pDecInfo->vbTask;
            for (i = 0; i < MAX_REG_FRAME; i++)
            {
                fbInfo->vbFbcYTbl[i]    = pDecInfo->vbFbcYTbl[i];
                fbInfo->vbFbcCTbl[i]    = pDecInfo->vbFbcCTbl[i];
                fbInfo->vbMvCol[i]      = pDecInfo->vbMV[i];
                fbInfo->framebufPool[i] = pDecInfo->frameBufPool[i];
            }
        }
        break;
    case DEC_RESET_FRAMEBUF_INFO:
        {
            int i;

            pDecInfo->vbFrame.base          = 0;
            pDecInfo->vbFrame.phys_addr     = 0;
            pDecInfo->vbFrame.virt_addr     = 0;
            pDecInfo->vbFrame.size          = 0;
            pDecInfo->vbWTL.base            = 0;
            pDecInfo->vbWTL.phys_addr       = 0;
            pDecInfo->vbWTL.virt_addr       = 0;
            pDecInfo->vbWTL.size            = 0;
            pDecInfo->vbTask.base           = 0;
            pDecInfo->vbTask.phys_addr      = 0;
            pDecInfo->vbTask.virt_addr      = 0;
            for (i = 0; i < MAX_REG_FRAME; i++)
            {
                pDecInfo->vbFbcYTbl[i].base        = 0;
                pDecInfo->vbFbcYTbl[i].phys_addr   = 0;
                pDecInfo->vbFbcYTbl[i].virt_addr   = 0;
                pDecInfo->vbFbcYTbl[i].size        = 0;
                pDecInfo->vbFbcCTbl[i].base        = 0;
                pDecInfo->vbFbcCTbl[i].phys_addr   = 0;
                pDecInfo->vbFbcCTbl[i].virt_addr   = 0;
                pDecInfo->vbFbcCTbl[i].size        = 0;
                pDecInfo->vbMV[i].base             = 0;
                pDecInfo->vbMV[i].phys_addr        = 0;
                pDecInfo->vbMV[i].virt_addr        = 0;
                pDecInfo->vbMV[i].size             = 0;
            }

            pDecInfo->frameDisplayFlag      = 0;
            pDecInfo->setDisplayIndexes     = 0;
            pDecInfo->clearDisplayIndexes   = 0;
            break;
        }
    case DEC_GET_QUEUE_STATUS:
        {
            ret = ProductVpuDecGiveCommand(pCodecInst, DEC_GET_QUEUE_STATUS, param);
            return ret;
        }

    case ENABLE_DEC_THUMBNAIL_MODE:
        {
            pDecInfo->thumbnailMode = 1;
            break;
        }
    case DEC_GET_SEQ_INFO:
        {
            DecInitialInfo* seqInfo = (DecInitialInfo*)param;
            *seqInfo = pDecInfo->initialInfo;
            break;
        }
    case DEC_GET_FIELD_PIC_TYPE:
        {
            return RETCODE_FAILURE;
        }
    case DEC_GET_DISPLAY_OUTPUT_INFO:
        {
            DecOutputInfo *pDecOutInfo = (DecOutputInfo *)param;
            *pDecOutInfo = pDecInfo->decOutInfo[pDecOutInfo->indexFrameDisplay];
            break;
        }
    case GET_TILEDMAP_CONFIG:
        {
            TiledMapConfig *pMapCfg = (TiledMapConfig *)param;
            if (!pMapCfg) {
                return RETCODE_INVALID_PARAM;
            }
            if (!pDecInfo->stride) {
                return RETCODE_WRONG_CALL_SEQUENCE;

            }
            *pMapCfg = pDecInfo->mapCfg;
            break;
        }
    case SET_DRAM_CONFIG:
        {
            DRAMConfig *cfg = (DRAMConfig *)param;

            if (!cfg) {
                return RETCODE_INVALID_PARAM;
            }

            pDecInfo->dramCfg = *cfg;
            break;
        }
    case GET_DRAM_CONFIG:
        {
            DRAMConfig *cfg = (DRAMConfig *)param;

            if (!cfg) {
                return RETCODE_INVALID_PARAM;
            }

            *cfg = pDecInfo->dramCfg;

            break;
        }
    case GET_LOW_DELAY_OUTPUT:
        {
            DecOutputInfo *lowDelayOutput;
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }

            if (!pDecInfo->lowDelayInfo.lowDelayEn || pCodecInst->codecMode != AVC_DEC) {
                return RETCODE_INVALID_COMMAND;
            }

            if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
                return RETCODE_WRONG_CALL_SEQUENCE;
            }

            lowDelayOutput = (DecOutputInfo *)param;

            GetLowDelayOutput(pCodecInst, lowDelayOutput);
        }
        break;
    case ENABLE_LOGGING:
        {
            pCodecInst->loggingEnable = 1;
        }
        break;
    case DISABLE_LOGGING:
        {
            pCodecInst->loggingEnable = 0;
        }
        break;
    case DEC_SET_SEQ_CHANGE_MASK:
        if (PRODUCT_ID_CODA_SERIES(pCodecInst->productId))
            return RETCODE_INVALID_PARAM;
        pDecInfo->seqChangeMask = *(int*)param;
        break;
    case DEC_SET_WTL_FRAME_FORMAT:
        {
            ret = ProductVpuDecGiveCommand(pCodecInst, DEC_SET_WTL_FRAME_FORMAT, param);
            return ret;
        }
    case DEC_SET_DISPLAY_FLAG:
        {
            Int32       index;
            VpuAttr*    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];
            BOOL        supportCommandQueue = FALSE;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }

            index = *(Int32 *)param;

            supportCommandQueue = (pAttr->supportCommandQueue == TRUE);
            if (supportCommandQueue == TRUE) {
                if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
                    return RETCODE_FAILURE;
                }
                ProductVpuDecSetDispFlag(pCodecInst, index);
                LeaveLock(pCodecInst->coreIdx);
            }
            else {
                EnterDispFlagLock(pCodecInst->coreIdx);
                pDecInfo->setDisplayIndexes |= (1<<index);
                LeaveDispFlagLock(pCodecInst->coreIdx);
            }

        }
        break;
    case DEC_SET_TARGET_TEMPORAL_ID:
        if (param == NULL) {
            return RETCODE_INVALID_PARAM;
        }
        pDecInfo->targetTempId = *(Uint32*)param;
        break;
    case DEC_SET_TEMPORAL_ID_MODE:
        if (param == NULL) {
            return RETCODE_INVALID_PARAM;
        }
        pDecInfo->tempIdSelectMode = *(TemporalIdMode*)param;
        break;
    case DEC_SET_TARGET_SPATIAL_ID:
        if (param == NULL) {
            return RETCODE_INVALID_PARAM;
        }
        if (pCodecInst->codecMode != W_AV1_DEC) {
            return RETCODE_INVALID_PARAM;
        }
        pDecInfo->targetSpatialId = *(Uint32*)param;
        break;
    case GET_BANDWIDTH_REPORT:
        ret = EnterLock(pCodecInst->coreIdx);
        if (ret == RETCODE_SUCCESS) {
            ret = ProductVpuGetBandwidth(pCodecInst, (VPUBWData*)param);
            LeaveLock(pCodecInst->coreIdx);
        }
        break;
    case SET_CYCLE_PER_TICK:
        {
            pDecInfo->cyclePerTick = *(Uint32 *)param;
        }
        break;
    case GET_DEBUG_INFORM:
        ret = EnterLock(pCodecInst->coreIdx);
        if (ret == RETCODE_SUCCESS) {
            ret = ProductVpuGetDebugInfo(pCodecInst, (VpuDebugInfo*)param);
            LeaveLock(pCodecInst->coreIdx);
        }
        break;
    default:
        return RETCODE_INVALID_COMMAND;
    }

    return ret;
}

RetCode VPU_DecAllocateFrameBuffer(DecHandle handle, FrameBufferAllocInfo info, FrameBuffer *frameBuffer)
{
    CodecInst*      pCodecInst;
    DecInfo*        pDecInfo;
    RetCode         ret;
    Uint32          gdiIndex;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pDecInfo   = &pCodecInst->CodecInfo->decInfo;

    if (!frameBuffer) {
        return RETCODE_INVALID_PARAM;
    }

    if (info.type == FB_TYPE_PPU) {
        if (pDecInfo->numFrameBuffers == 0)
            return RETCODE_WRONG_CALL_SEQUENCE;
        if (frameBuffer[0].updateFbInfo == TRUE) {
            pDecInfo->ppuAllocExt = TRUE;
        }
        pDecInfo->ppuAllocExt = frameBuffer[0].updateFbInfo;
        gdiIndex = pDecInfo->numFbsForDecoding;
        ret = ProductVpuDecAllocateFramebuffer(
            pCodecInst,  frameBuffer, (TiledMapType)info.mapType, (Int32)info.num,
            info.stride, info.height, info.format, info.cbcrInterleave, info.nv21, info.endian, &pDecInfo->vbPPU, gdiIndex, FB_TYPE_PPU);
    }
    else if (info.type == FB_TYPE_CODEC) {
        gdiIndex = 0;
        if (frameBuffer[0].updateFbInfo == TRUE) {
            pDecInfo->frameAllocExt = TRUE;
        }
        ret = ProductVpuDecAllocateFramebuffer(
            pCodecInst, frameBuffer, (TiledMapType)info.mapType, (Int32)info.num,
            info.stride, info.height, info.format, info.cbcrInterleave, info.nv21, info.endian, &pDecInfo->vbFrame, gdiIndex, (FramebufferAllocType)info.type);

        pDecInfo->mapCfg.tiledBaseAddr = pDecInfo->vbFrame.phys_addr;
    }

    return ret;
}

RetCode VPU_DecRegisterAuxBuffer(DecHandle handle, AuxBufferInfo auxBufferInfo)
{
    RetCode ret = RETCODE_SUCCESS;

    if (!auxBufferInfo.num)
        return RETCODE_INVALID_PARAM;
    if (!auxBufferInfo.bufArray)
        return RETCODE_INVALID_PARAM;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    ret = ProductVpuDecRegisterAuxBuffer((CodecInst*)handle, auxBufferInfo);

    return ret;
}

RetCode VPU_DecGetAuxBufSize(DecHandle handle, DecAuxBufferSizeInfo sizeInfo, Uint32* size)
{
    RetCode ret = RETCODE_SUCCESS;

    if (!size)
        return RETCODE_INVALID_PARAM;
    if (!sizeInfo.width)
        return RETCODE_INVALID_PARAM;
    if (!sizeInfo.height)
        return RETCODE_INVALID_PARAM;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    ret = ProductVpuDecGetAuxBufSize((CodecInst*)handle, sizeInfo, size);

    return ret;
}

RetCode VPU_DecSetCbCr(DecHandle handle, int cbcrInterleave, int nv21)
{
    CodecInst * pCodecInst = (CodecInst *)handle;
    DecInfo*    pDecInfo;
    RetCode     ret;

    ret = CheckDecInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pDecInfo   = &pCodecInst->CodecInfo->decInfo;

    pDecInfo->openParam.cbcrInterleave = cbcrInterleave;
    pDecInfo->openParam.nv21 = nv21;

    return RETCODE_SUCCESS;
}

RetCode VPU_EncOpen(EncHandle* pHandle, EncOpenParam * pop)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;
    VpuAttr*    pAttr;

    if (pop == NULL) {
        VLOG(ERR, "param ptr is null\n");
        return RETCODE_INVALID_PARAM;
    }

    if (EnterLock(pop->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if ((ret=ProductVpuEncCheckOpenParam(pop)) != RETCODE_SUCCESS) {
        LeaveLock(pop->coreIdx);
        return ret;
    }

    if (ProductVpuIsInit(pop->coreIdx) == 0) {
        LeaveLock(pop->coreIdx);
        return RETCODE_NOT_INITIALIZED;
    }

    ret = GetCodecInstance(pop->coreIdx, &pCodecInst);
    if (ret == RETCODE_FAILURE) {
        *pHandle = 0;
        LeaveLock(pop->coreIdx);
        return RETCODE_FAILURE;
    }

    pCodecInst->productId = ProductVpuGetId(pop->coreIdx);
    pCodecInst->isDecoder = FALSE;
    *pHandle = pCodecInst;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    osal_memset(pEncInfo, 0x00, sizeof(EncInfo));
    pEncInfo->openParam = *pop;

    if ((ret=ProductVpuEncBuildUpOpenParam(pCodecInst, pop)) != RETCODE_SUCCESS) {
        *pHandle = 0;
    }

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (vdi_open_instance(pCodecInst->coreIdx, pCodecInst->instIndex, pAttr->supportCommandQueue) < 0) {
        LeaveLock(pop->coreIdx);
        return RETCODE_FAILURE;
    }
    LeaveLock(pop->coreIdx);

    return ret;
}

RetCode VPU_EncClose(EncHandle handle)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if ((ret=ProductVpuEncFiniSeq(pCodecInst)) != RETCODE_SUCCESS) {
        if (pCodecInst->loggingEnable)
            vdi_log(pCodecInst->coreIdx, pCodecInst->instIndex, ENC_SEQ_END, 2);

        if (ret == RETCODE_VPU_STILL_RUNNING) {
            LeaveLock(pCodecInst->coreIdx);
            return ret;
        }
    }

    if (pCodecInst->loggingEnable)
        vdi_log(pCodecInst->coreIdx, pCodecInst->instIndex, ENC_SEQ_END, 0);

    if (pEncInfo->vbScratch.size) {
        vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbScratch, ENC_ETC, pCodecInst->instIndex);
    }

    if (pEncInfo->vbWork.size) {
        vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbWork, ENC_WORK, pCodecInst->instIndex);
    }

    if (pEncInfo->vbFrame.size) {
        if (pEncInfo->frameAllocExt == 0)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFrame, ENC_ETC, pCodecInst->instIndex);
    }

    if (pCodecInst->codecMode == W_HEVC_ENC || pCodecInst->codecMode == W_AVC_ENC || pCodecInst->codecMode == W_AV1_ENC) {
        int i;
        for (i = 0; i < pEncInfo->numFrameBuffers; i++) {
            if (pEncInfo->vbSubSamBuf[i].size)
                vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbSubSamBuf[i], ENC_SUBSAMBUF, pCodecInst->instIndex);
            if (pEncInfo->vbMV[i].size)
                vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbMV[i], ENC_MV, pCodecInst->instIndex);
            if (pEncInfo->vbFbcYTbl[i].size)
                vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFbcYTbl[i], ENC_FBCY_TBL, pCodecInst->instIndex);
            if (pEncInfo->vbFbcCTbl[i].size)
                vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFbcCTbl[i], ENC_FBCC_TBL, pCodecInst->instIndex);
        }
        if (pEncInfo->vbDefCdf.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbDefCdf, ENC_DEF_CDF, pCodecInst->instIndex);
    }

    if (pEncInfo->vbPPU.size) {
        if (pEncInfo->ppuAllocExt == 0)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbPPU, ENC_ETC, pCodecInst->instIndex);
    }
    if (pEncInfo->vbSubSampFrame.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbSubSampFrame, ENC_ETC, pCodecInst->instIndex);
    if (pEncInfo->vbMvcSubSampFrame.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbMvcSubSampFrame, ENC_ETC, pCodecInst->instIndex);

    if (pEncInfo->vbTask.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbTask, ENC_TASK, pCodecInst->instIndex);

    LeaveLock(pCodecInst->coreIdx);

    FreeCodecInstance(pCodecInst);

    return ret;
}

RetCode VPU_EncGetInitialInfo(EncHandle handle, EncInitialInfo * info)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    if (info == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    if ((ret=ProductVpuEncSetup(pCodecInst)) != RETCODE_SUCCESS) {
        LeaveLock(pCodecInst->coreIdx);
        return ret;
    }

    if (pCodecInst->codecMode == AVC_ENC && pCodecInst->codecModeAux == AVC_AUX_MVC)
        info->minFrameBufferCount = 3; // reconstructed frame + 2 reference frame
    else if(pCodecInst->codecMode == W_HEVC_ENC || pCodecInst->codecMode == W_AVC_ENC || pCodecInst->codecMode == W_AV1_ENC) {
        info->minFrameBufferCount = pEncInfo->initialInfo.minFrameBufferCount;
        info->minSrcFrameCount    = pEncInfo->initialInfo.minSrcFrameCount;
    }
    else
        info->minFrameBufferCount = 2; // reconstructed frame + reference frame

    pEncInfo->initialInfo         = *info;
    pEncInfo->initialInfoObtained = TRUE;

    LeaveLock(pCodecInst->coreIdx);

    return RETCODE_SUCCESS;
}

RetCode VPU_EncRegisterFrameBuffer(EncHandle handle, FrameBuffer* bufArray, int num, int stride, int height, TiledMapType mapType)
{
    CodecInst*      pCodecInst;
    EncInfo*        pEncInfo;
    Int32           i;
    RetCode         ret;
    EncOpenParam*   openParam;
    FrameBuffer*    fb;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;
    openParam = &pEncInfo->openParam;

    if (pEncInfo->stride)
        return RETCODE_CALLED_BEFORE;

    if (!pEncInfo->initialInfoObtained)
        return RETCODE_WRONG_CALL_SEQUENCE;

    if (num < pEncInfo->initialInfo.minFrameBufferCount)
        return RETCODE_INSUFFICIENT_FRAME_BUFFERS;

    if (stride == 0 || (stride % 8 != 0) || stride < 0)
        return RETCODE_INVALID_STRIDE;

    if (height == 0 || height < 0)
        return RETCODE_INVALID_PARAM;

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    pEncInfo->numFrameBuffers   = num;
    pEncInfo->stride            = stride;
    pEncInfo->frameBufferHeight = height;
    pEncInfo->mapType           = mapType;
    pEncInfo->mapCfg.productId  = pCodecInst->productId;

    if (bufArray) {
        for(i=0; i<num; i++) {
            pEncInfo->frameBufPool[i] = bufArray[i];
        }
    }

    if (pEncInfo->frameAllocExt == FALSE) {
        fb = pEncInfo->frameBufPool;
        if (bufArray) {
            if (bufArray[0].bufCb == (PhysicalAddress)-1 && bufArray[0].bufCr == (PhysicalAddress)-1) {
                Uint32 size;
                pEncInfo->frameAllocExt = TRUE;
                size = ProductCalculateFrameBufSize(pCodecInst, pCodecInst->productId, stride, height,
                                                    (TiledMapType)mapType, (FrameBufferFormat)openParam->srcFormat,
                                                    (BOOL)openParam->cbcrInterleave, NULL);
                if (mapType == LINEAR_FRAME_MAP) {
                    pEncInfo->vbFrame.phys_addr = bufArray[0].bufY;
                    pEncInfo->vbFrame.size      = size * num;
                }
            }
        }
        ret = ProductVpuEncAllocateFramebuffer(pCodecInst, fb, (TiledMapType)mapType, num,
                                               stride, height, (FrameBufferFormat)openParam->srcFormat, openParam->cbcrInterleave,
                                               FALSE, openParam->frameEndian, &pEncInfo->vbFrame, 0, FB_TYPE_CODEC);
        if (ret != RETCODE_SUCCESS) {
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(pCodecInst->coreIdx);
            return ret;
        }
    }

    ret = ProductVpuEncRegisterFramebuffer(pCodecInst);

    SetPendingInst(pCodecInst->coreIdx, 0);

    LeaveLock(pCodecInst->coreIdx);

    return ret;
}


RetCode VPU_EncGetBitstreamBuffer( EncHandle handle,
    PhysicalAddress * prdPrt,
    PhysicalAddress * pwrPtr,
    int * size)
{
    CodecInst * pCodecInst;
    EncInfo * pEncInfo;
    PhysicalAddress rdPtr;
    PhysicalAddress wrPtr;
    Uint32 room;
    RetCode ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    if (prdPrt == 0 || pwrPtr == 0 || size == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    rdPtr = pEncInfo->streamRdPtr;

    if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst) {
        wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
        wrPtr = VPU_MapToAddr40Bit(pCodecInst->coreIdx, wrPtr);
    } else {
        if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }
        ProductVpuEncGetRdWrPtr(pCodecInst, &rdPtr, &wrPtr);
        LeaveLock(pCodecInst->coreIdx);
    }

    if (pEncInfo->ringBufferEnable == 1 || pEncInfo->lineBufIntEn == 1) {
        if (wrPtr >= rdPtr) {
            room = wrPtr - rdPtr;
        }
        else {
            room = (pEncInfo->streamBufEndAddr - rdPtr) + (wrPtr - pEncInfo->streamBufStartAddr);
        }
    }
    else {
        if(wrPtr >= rdPtr)
            room = wrPtr - rdPtr;
        else
            return RETCODE_INVALID_PARAM;
    }

    *prdPrt = rdPtr;
    *pwrPtr = wrPtr;
    *size = room;

    return RETCODE_SUCCESS;
}

RetCode VPU_EncUpdateBitstreamBuffer(
    EncHandle handle,
    int size)
{
    CodecInst * pCodecInst;
    EncInfo * pEncInfo;
    PhysicalAddress wrPtr;
    PhysicalAddress rdPtr;
    RetCode ret;
    int        room = 0;
    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    rdPtr = pEncInfo->streamRdPtr;

    if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
        wrPtr = VPU_MapToAddr40Bit(pCodecInst->coreIdx, VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr));
    else
        wrPtr = pEncInfo->streamWrPtr;

    if ( rdPtr < wrPtr ) {
        if ( rdPtr + size  > wrPtr ) {
            return RETCODE_INVALID_PARAM;
        }
    }

    if (PRODUCT_ID_W_SERIES(pCodecInst->productId)) {
        //************** for WAVE5 *********************/
        if (size > 0) {
            if (pEncInfo->ringBufferEnable == TRUE) {
                rdPtr += size;
                if (pEncInfo->ringBufferWrapEnable == TRUE) {
                    //======== [case1]. ring=1 & wrap=1 ===================

                    if (rdPtr > pEncInfo->streamBufEndAddr) {
                        if (pEncInfo->lineBufIntEn == TRUE) {
                            return RETCODE_INVALID_PARAM;
                        }
                        room = rdPtr - pEncInfo->streamBufEndAddr;
                        rdPtr = pEncInfo->streamBufStartAddr;
                        rdPtr += room;
                    }

                    pEncInfo->streamRdPtr = rdPtr;
                    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
                        return RETCODE_FAILURE;
                    }
                    ProductVpuEncUpdateBitstreamBuffer(pCodecInst);
                    LeaveLock(pCodecInst->coreIdx);
                    if (rdPtr == pEncInfo->streamBufEndAddr) {
                        //set EndAddr to FW and set StartAddr to API
                        rdPtr = pEncInfo->streamBufStartAddr;   // wrap-around
                    }
                }
                else {
                    //========= [case2]. ring=1 & wrap=0 ===================
                    pEncInfo->streamRdPtr = rdPtr;
                    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
                        return RETCODE_FAILURE;
                    }
                    ProductVpuEncUpdateBitstreamBuffer(pCodecInst);
                    LeaveLock(pCodecInst->coreIdx);
                    if (rdPtr == pEncInfo->streamBufEndAddr) {
                        //set EndAddr to FW and set StartAddr to API
                        rdPtr = pEncInfo->streamBufStartAddr;   // start? end? [FIX ME]
                    }
                }
            }
            else {
                //========= [case3]. Line buffer mode  ===================
                if (VPU_ALIGN16(wrPtr) >= pEncInfo->streamRdPtr + pEncInfo->streamBufSize) {  // linebuffer full detected. already host read whole bistream in bitstreamBuffer.
                    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
                        return RETCODE_FAILURE;
                    }
                    ProductVpuEncUpdateBitstreamBuffer(pCodecInst);
                    LeaveLock(pCodecInst->coreIdx);
                }
            }
        }
        else {
            //************ update new CPB buffer addr/size ***************/
            if (size == UPDATE_NEW_BS_BUF) {
                if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
                    return RETCODE_FAILURE;
                }
                ProductVpuEncUpdateBitstreamBuffer(pCodecInst);
                LeaveLock(pCodecInst->coreIdx);
            }
        }
    }
    else {
        //************** for CODA / WAVE6 *********************/
        if (pEncInfo->ringBufferEnable == TRUE || pEncInfo->lineBufIntEn == TRUE) {
            rdPtr += size;
            if (rdPtr > pEncInfo->streamBufEndAddr) {
                if (pEncInfo->lineBufIntEn == TRUE) {
                    return RETCODE_INVALID_PARAM;
                }
                room = rdPtr - pEncInfo->streamBufEndAddr;
                rdPtr = pEncInfo->streamBufStartAddr;
                rdPtr += room;
            }
            if (rdPtr == pEncInfo->streamBufEndAddr)
                rdPtr = pEncInfo->streamBufStartAddr;
        }
        else {
            rdPtr = pEncInfo->streamBufStartAddr;
        }
    }

    pEncInfo->streamRdPtr = rdPtr;
    pEncInfo->streamWrPtr = wrPtr;
    if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr, rdPtr);

    if (pEncInfo->ringBufferEnable == FALSE && pEncInfo->lineBufIntEn == TRUE) {
        pEncInfo->streamRdPtr = pEncInfo->streamBufStartAddr;
    }

    return RETCODE_SUCCESS;
}

RetCode VPU_EncStartOneFrame(
    EncHandle handle,
    EncParam* param
    )
{
    CodecInst*          pCodecInst;
    EncInfo*            pEncInfo;
    RetCode             ret;
    VpuAttr*            pAttr   = NULL;
    vpu_instance_pool_t* vip;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo   = &pCodecInst->CodecInfo->encInfo;
    vip        = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
    if (!vip) {
        return RETCODE_INVALID_HANDLE;
    }

    if (pEncInfo->stride == 0) { // This means frame buffers have not been registered.
        return RETCODE_WRONG_CALL_SEQUENCE;
    }

    ret = ProductVpuEncCheckEncParam(pCodecInst, param);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    pEncInfo->ptsMap[param->srcIdx] = (pEncInfo->openParam.enablePTS == TRUE) ? GetTimestamp(handle) : param->pts;

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    ret = ProductVpuEncode(pCodecInst, param);
    if (ret != RETCODE_SUCCESS) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
        return ret;
    }

    if (pAttr->supportCommandQueue == TRUE) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
    }
    else {
        SetPendingInst(pCodecInst->coreIdx, pCodecInst);
    }

    return ret;
}

RetCode VPU_EncGetOutputInfo(
    EncHandle       handle,
    EncOutputInfo*  info
    )
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;
    VpuAttr*    pAttr;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    if (info == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst = handle;
    pEncInfo   = &pCodecInst->CodecInfo->encInfo;
    pAttr      = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (pAttr->supportCommandQueue == TRUE) {
        EnterLock(pCodecInst->coreIdx);
    }
    else {
        if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(pCodecInst->coreIdx);
            return RETCODE_WRONG_CALL_SEQUENCE;
        }
    }

    ret = ProductVpuEncGetResult(pCodecInst, info);

    if (ret == RETCODE_SUCCESS) {
        if (info->encSrcIdx >= 0 && info->encSrcIdx < 32 && info->reconFrameIndex >= 0 )
            info->pts = pEncInfo->ptsMap[info->encSrcIdx];
    }
    else {
        info->pts = 0LL;
    }

    SetPendingInst(pCodecInst->coreIdx, 0);
    LeaveLock(pCodecInst->coreIdx);

    return ret;
}

RetCode VPU_EncGiveCommand(
    EncHandle       handle,
    CodecCommand    cmd,
    void*           param
    )
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;
    switch (cmd)
    {
    case ENABLE_ROTATION :
        {
            pEncInfo->rotationEnable = 1;
        }
        break;
    case DISABLE_ROTATION :
        {
            pEncInfo->rotationEnable = 0;
        }
        break;
    case ENABLE_MIRRORING :
        {
            pEncInfo->mirrorEnable = 1;
        }
        break;
    case DISABLE_MIRRORING :
        {
            pEncInfo->mirrorEnable = 0;
        }
        break;
    case SET_MIRROR_DIRECTION :
        {
            MirrorDirection mirDir;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            mirDir = *(MirrorDirection *)param;
            if ( !(mirDir == MIRDIR_NONE) && !(mirDir==MIRDIR_HOR) && !(mirDir==MIRDIR_VER) && !(mirDir==MIRDIR_HOR_VER)) {
                return RETCODE_INVALID_PARAM;
            }
            pEncInfo->mirrorDirection = mirDir;
        }
        break;
    case SET_ROTATION_ANGLE :
        {
            int angle;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            angle = *(int *)param;
            if (angle != 0 && angle != 90 &&
                angle != 180 && angle != 270) {
                    return RETCODE_INVALID_PARAM;
            }
            if (pEncInfo->initialInfoObtained && (angle == 90 || angle ==270)) {
                return RETCODE_INVALID_PARAM;
            }
            pEncInfo->rotationAngle = angle;
        }
        break;
    case SET_CACHE_CONFIG:
        {
            MaverickCacheConfig *mcCacheConfig;
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            mcCacheConfig = (MaverickCacheConfig *)param;
            pEncInfo->cacheConfig = *mcCacheConfig;
        }
        break;
    case ENC_PUT_VIDEO_HEADER:
        {
            EncHeaderParam *encHeaderParam;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            encHeaderParam = (EncHeaderParam *)param;
            if (pCodecInst->codecMode == MP4_ENC ) {
                if (!( VOL_HEADER<=encHeaderParam->headerType && encHeaderParam->headerType <= VIS_HEADER)) {
                    return RETCODE_INVALID_PARAM;
                }
            }
            else if  (pCodecInst->codecMode == AVC_ENC) {
                if (!( SPS_RBSP<=encHeaderParam->headerType && encHeaderParam->headerType <= PPS_RBSP_MVC)) {
                    return RETCODE_INVALID_PARAM;
                }
            }
            else if (pCodecInst->codecMode == W_HEVC_ENC || pCodecInst->codecMode == W_AVC_ENC || pCodecInst->codecMode == W_AV1_ENC) {
                if (!( CODEOPT_ENC_VPS<=encHeaderParam->headerType && encHeaderParam->headerType <= (CODEOPT_ENC_VPS|CODEOPT_ENC_SPS|CODEOPT_ENC_PPS))) {
                    return RETCODE_INVALID_PARAM;
                }
                if (pEncInfo->ringBufferEnable == 0 ) {
                    if (encHeaderParam->buf % 16 || encHeaderParam->size == 0)
                        return RETCODE_INVALID_PARAM;
                }
                if (encHeaderParam->headerType & CODEOPT_ENC_VCL)   // ENC_PUT_VIDEO_HEADER encode only non-vcl header.
                    return RETCODE_INVALID_PARAM;

            }
            else {
                return RETCODE_INVALID_PARAM;
            }

            if (pEncInfo->ringBufferEnable == 0 ) {
                if (encHeaderParam->buf % 8 || encHeaderParam->size == 0) {
                    return RETCODE_INVALID_PARAM;
                }
            }

            ret = EnterLock(pCodecInst->coreIdx);
            if (ret == RETCODE_SUCCESS) {
                ret = ProductVpuEncGetHeader(pCodecInst, encHeaderParam);
                LeaveLock(pCodecInst->coreIdx);
            }
            return ret;
        }
    case ENC_SET_PARAM:
        {
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            pEncInfo->openParam = *(EncOpenParam *)param;

            if (pCodecInst->codecMode != AVC_ENC)
                return RETCODE_INVALID_COMMAND;

            ret = EncParaSet(handle, SPS_RBSP);
            if (ret != RETCODE_SUCCESS)
                return ret;

            ret = EncParaSet(handle, PPS_RBSP);
            if (ret != RETCODE_SUCCESS)
                return ret;
            return ret;
        }
    case ENC_SET_GOP_NUMBER:
        {
            ParamChange paramChange;

            paramChange.paraEnable = C9_ENC_CHANGE_PARAM_GOP_NUM;
            paramChange.NewGopNum  = *(int*)param;

            if ((pCodecInst->codecMode != MP4_ENC) && (pCodecInst->codecMode != AVC_ENC))
                return RETCODE_INVALID_COMMAND;
            if (paramChange.NewGopNum < 0)
                return RETCODE_INVALID_PARAM;

            ret = EnterLock(pCodecInst->coreIdx);
            if (ret == RETCODE_SUCCESS) {
                ret = ProductVpuEncChangeParam(pCodecInst, &paramChange);
                LeaveLock(pCodecInst->coreIdx);
            }
            return ret;
        }
    case ENC_SET_PERIOD:
        {
            ret = EnterLock(pCodecInst->coreIdx);
            if (ret == RETCODE_SUCCESS) {
                ret = ProductVpuEncSetPeriod(pCodecInst, param);
                LeaveLock(pCodecInst->coreIdx);
            }
            return ret;
        }
    case ENC_SET_INTRA_QP:
        {
            ParamChange paramChange;

            paramChange.paraEnable = C9_ENC_CHANGE_PARAM_INTRA_QP;
            paramChange.NewIntraQp = *(int*)param;

            if ((pCodecInst->codecMode != MP4_ENC) && (pCodecInst->codecMode != AVC_ENC))
                return RETCODE_INVALID_COMMAND;
            if ((pCodecInst->codecMode == MP4_ENC) && (paramChange.NewIntraQp < 1 || paramChange.NewIntraQp > 31))
                return RETCODE_INVALID_PARAM;
            if ((pCodecInst->codecMode == AVC_ENC) && (paramChange.NewIntraQp < 0 || paramChange.NewIntraQp > 51))
                return RETCODE_INVALID_PARAM;

            ret = EnterLock(pCodecInst->coreIdx);
            if (ret == RETCODE_SUCCESS) {
                ret = ProductVpuEncChangeParam(pCodecInst, &paramChange);
                LeaveLock(pCodecInst->coreIdx);
            }
            return ret;
        }
    case ENC_SET_BITRATE:
        {
            ParamChange paramChange;

            paramChange.paraEnable = C9_ENC_CHANGE_PARAM_BIT_RATE;
            paramChange.NewBitrate = *(int*)param;

            if ((pCodecInst->codecMode != MP4_ENC) && (pCodecInst->codecMode != AVC_ENC))
                return RETCODE_INVALID_COMMAND;
            if ((pCodecInst->codecMode == AVC_ENC) && (paramChange.NewBitrate < 0 || paramChange.NewBitrate > 32767))
                return RETCODE_INVALID_PARAM;
            if ((pCodecInst->codecMode == MP4_ENC) && (paramChange.NewBitrate < 0 || paramChange.NewBitrate > 32767))
                return RETCODE_INVALID_PARAM;

            ret = EnterLock(pCodecInst->coreIdx);
            if (ret == RETCODE_SUCCESS) {
                ret = ProductVpuEncChangeParam(pCodecInst, &paramChange);
                LeaveLock(pCodecInst->coreIdx);
            }
            return ret;
        }
    case ENC_SET_FRAME_RATE:
        {
            ParamChange paramChange;

            paramChange.paraEnable   = C9_ENC_CHANGE_PARAM_FRAME_RATE;
            paramChange.NewFrameRate = *(int*)param;

            if ((pCodecInst->codecMode != MP4_ENC) && (pCodecInst->codecMode != AVC_ENC))
                return RETCODE_INVALID_COMMAND;
            if (paramChange.NewFrameRate <= 0)
                return RETCODE_INVALID_PARAM;

            ret = EnterLock(pCodecInst->coreIdx);
            if (ret == RETCODE_SUCCESS) {
                ret = ProductVpuEncChangeParam(pCodecInst, &paramChange);
                LeaveLock(pCodecInst->coreIdx);
            }
            return ret;
        }
    case ENC_SET_INTRA_MB_REFRESH_NUMBER:
        {
            ParamChange paramChange;

            paramChange.paraEnable      = C9_ENC_CHANGE_PARAM_INTRA_REFRESH;
            paramChange.NewIntraRefresh = *(int*)param;

            ret = EnterLock(pCodecInst->coreIdx);
            if (ret == RETCODE_SUCCESS) {
                ret = ProductVpuEncChangeParam(pCodecInst, &paramChange);
                LeaveLock(pCodecInst->coreIdx);
            }
            return ret;
        }
    case ENC_SET_SLICE_INFO:
        {
            EncSliceMode *pSliceMode = (EncSliceMode *)param;

            if(pSliceMode->sliceMode<0 || pSliceMode->sliceMode>1)
            {
                return RETCODE_INVALID_PARAM;
            }
            if(pSliceMode->sliceSizeMode<0 || pSliceMode->sliceSizeMode>1)
            {
                return RETCODE_INVALID_PARAM;
            }
            SetSliceMode(handle, (EncSliceMode *)pSliceMode);
        }
        break;
    case ENC_ENABLE_HEC:
        {
            if (pCodecInst->codecMode != MP4_ENC) {
                return RETCODE_INVALID_COMMAND;
            }
            SetHecMode(handle, 1);
        }
        break;
    case ENC_DISABLE_HEC:
        {
            if (pCodecInst->codecMode != MP4_ENC) {
                return RETCODE_INVALID_COMMAND;
            }
            SetHecMode(handle, 0);
        }
        break;
    case SET_SEC_AXI:
        {
            SecAxiUse secAxiUse;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            secAxiUse = *(SecAxiUse *)param;
            if (PRODUCT_ID_W_SERIES(pCodecInst->productId)) {
                pEncInfo->secAxiInfo.u.wave.useEncRdoEnable = secAxiUse.u.wave.useEncRdoEnable;
                pEncInfo->secAxiInfo.u.wave.useEncLfEnable  = secAxiUse.u.wave.useEncLfEnable;
            }
            else { // coda9 or coda7q or ...
                pEncInfo->secAxiInfo.u.coda9.useBitEnable  = secAxiUse.u.coda9.useBitEnable;
                pEncInfo->secAxiInfo.u.coda9.useIpEnable   = secAxiUse.u.coda9.useIpEnable;
                pEncInfo->secAxiInfo.u.coda9.useDbkYEnable = secAxiUse.u.coda9.useDbkYEnable;
                pEncInfo->secAxiInfo.u.coda9.useDbkCEnable = secAxiUse.u.coda9.useDbkCEnable;
                pEncInfo->secAxiInfo.u.coda9.useOvlEnable  = secAxiUse.u.coda9.useOvlEnable;
                pEncInfo->secAxiInfo.u.coda9.useBtpEnable  = secAxiUse.u.coda9.useBtpEnable;
            }
        }
        break;
    case ENC_CONFIG_SUB_FRAME_SYNC:
        ret = ProductVpuEncGiveCommand(pCodecInst, ENC_CONFIG_SUB_FRAME_SYNC, param);
        return ret;
    case ENC_SET_SUB_FRAME_SYNC:
        if (PRODUCT_ID_W_SERIES(pCodecInst->productId)) {
            ret = EnterLock(pCodecInst->coreIdx);
        }
        if (ret == RETCODE_SUCCESS) {
            ret = ProductVpuEncGiveCommand(pCodecInst, ENC_SET_SUB_FRAME_SYNC, param);
            if (PRODUCT_ID_W_SERIES(pCodecInst->productId)) {
                LeaveLock(pCodecInst->coreIdx);
            }
        }
        return ret;
    case GET_TILEDMAP_CONFIG:
        {
            TiledMapConfig *pMapCfg = (TiledMapConfig *)param;
            if (!pMapCfg) {
                return RETCODE_INVALID_PARAM;
            }
            *pMapCfg = pEncInfo->mapCfg;
            break;
        }
    case SET_DRAM_CONFIG:
        {
            DRAMConfig *cfg = (DRAMConfig *)param;

            if (!cfg) {
                return RETCODE_INVALID_PARAM;
            }

            pEncInfo->dramCfg = *cfg;
            break;
        }
    case GET_DRAM_CONFIG:
        {
            DRAMConfig *cfg = (DRAMConfig *)param;

            if (!cfg) {
                return RETCODE_INVALID_PARAM;
            }

            *cfg = pEncInfo->dramCfg;
            break;
        }
    case ENABLE_LOGGING:
        {
            pCodecInst->loggingEnable = 1;
        }
        break;
    case DISABLE_LOGGING:
        {
            pCodecInst->loggingEnable = 0;
        }
        break;
    case ENC_SET_PARA_CHANGE:
        {
            ret = EnterLock(pCodecInst->coreIdx);
            if (ret == RETCODE_SUCCESS) {
                ret = ProductVpuEncChangeParam(pCodecInst, param);
                LeaveLock(pCodecInst->coreIdx);
            }
            return ret;
        }
    case GET_BANDWIDTH_REPORT:
        ret = EnterLock(pCodecInst->coreIdx);
        if (ret == RETCODE_SUCCESS) {
            ret = ProductVpuGetBandwidth(pCodecInst, (VPUBWData*)param);
            LeaveLock(pCodecInst->coreIdx);
        }
        return ret;
    case ENC_SET_SEI_NAL_DATA:
        {
            EncSeiNalData* option = (EncSeiNalData*)param;
            pEncInfo->prefixSeiNalEnable = option->prefixSeiNalEnable;
            pEncInfo->prefixSeiDataSize  = option->prefixSeiDataSize;
            pEncInfo->prefixSeiNalAddr   = option->prefixSeiNalAddr;
            pEncInfo->suffixSeiNalEnable = option->suffixSeiNalEnable;
            pEncInfo->suffixSeiDataSize  = option->suffixSeiDataSize;
            pEncInfo->suffixSeiNalAddr   = option->suffixSeiNalAddr;
        }
        break;
    case ENC_GET_QUEUE_STATUS:
        {
            ret = ProductVpuEncGiveCommand(pCodecInst, ENC_GET_QUEUE_STATUS, param);
            return ret;
        }
    case ENC_WRPTR_SEL:
        {
            pEncInfo->encWrPtrSel = *(int *)param;
        }
        break;
    case SET_CYCLE_PER_TICK:
        {
            pEncInfo->cyclePerTick = *(Uint32 *)param;
        }
        break;
    case ENC_GET_SRC_BUF_FLAG:
        ret = EnterLock(pCodecInst->coreIdx);
        if (ret == RETCODE_SUCCESS) {
            ret = ProductVpuEncGetSrcBufFlag(pCodecInst, (Uint32*)param);
            LeaveLock(pCodecInst->coreIdx);
        }
        return ret;
    case GET_DEBUG_INFORM:
        ret = EnterLock(pCodecInst->coreIdx);
        if (ret == RETCODE_SUCCESS) {
            ret = ProductVpuGetDebugInfo(pCodecInst, (VpuDebugInfo*)param);
            LeaveLock(pCodecInst->coreIdx);
        }
        return ret;
    default:
        return RETCODE_INVALID_COMMAND;
    }
    return RETCODE_SUCCESS;
}

RetCode VPU_EncAllocateFrameBuffer(EncHandle handle, FrameBufferAllocInfo info, FrameBuffer *frameBuffer)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;
    int         gdiIndex;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo   = &pCodecInst->CodecInfo->encInfo;

    if (!frameBuffer)
        return RETCODE_INVALID_PARAM;
    if (info.num == 0 || info.num < 0)
        return RETCODE_INVALID_PARAM;
    if (info.stride == 0 || info.stride < 0)
        return RETCODE_INVALID_PARAM;
    if (info.height == 0 || info.height < 0)
        return RETCODE_INVALID_PARAM;

    if (info.type == FB_TYPE_PPU) {
        if (pEncInfo->numFrameBuffers == 0)
            return RETCODE_WRONG_CALL_SEQUENCE;

        pEncInfo->ppuAllocExt = frameBuffer[0].updateFbInfo;
        gdiIndex = pEncInfo->numFrameBuffers;
        ret = ProductVpuEncAllocateFramebuffer(pCodecInst, frameBuffer, (TiledMapType)info.mapType, (Int32)info.num,
                                              info.stride, info.height, info.format, info.cbcrInterleave, info.nv21,
                                              info.endian, &pEncInfo->vbPPU, gdiIndex, (FramebufferAllocType)info.type);
    } else if (info.type == FB_TYPE_CODEC) {
        gdiIndex = 0;
        pEncInfo->frameAllocExt = frameBuffer[0].updateFbInfo;
        ret = ProductVpuEncAllocateFramebuffer(pCodecInst, frameBuffer, (TiledMapType)info.mapType, (Int32)info.num,
                                               info.stride, info.height, info.format, info.cbcrInterleave, FALSE,
                                               info.endian, &pEncInfo->vbFrame, gdiIndex, (FramebufferAllocType)info.type);
    } else {
        ret = RETCODE_INVALID_PARAM;
    }

    return ret;
}

RetCode VPU_EncRegisterAuxBuffer(EncHandle handle, AuxBufferInfo auxBufferInfo)
{
    RetCode ret = RETCODE_SUCCESS;

    if (!auxBufferInfo.num)
        return RETCODE_INVALID_PARAM;
    if (!auxBufferInfo.bufArray)
        return RETCODE_INVALID_PARAM;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    ret = ProductVpuEncRegisterAuxBuffer((CodecInst*)handle, auxBufferInfo);

    return ret;
}

RetCode VPU_EncGetAuxBufSize(EncHandle handle, EncAuxBufferSizeInfo sizeInfo, Uint32* size)
{
    RetCode ret = RETCODE_SUCCESS;

    if (!size)
        return RETCODE_INVALID_PARAM;
    if (!sizeInfo.width)
        return RETCODE_INVALID_PARAM;
    if (!sizeInfo.height)
        return RETCODE_INVALID_PARAM;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    ret = ProductVpuEncGetAuxBufSize((CodecInst*)handle, sizeInfo, size);

    return ret;
}

RetCode VPU_EncIssueSeqInit(EncHandle handle)
{
    CodecInst*  pCodecInst;
    RetCode     ret;
    VpuAttr*    pAttr;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;

    if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
        return RETCODE_FAILURE;
    }

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    ret = ProductVpuEncInitSeq(handle);
    if (ret != RETCODE_SUCCESS) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
        return ret;
    }

    if (pAttr->supportCommandQueue == TRUE) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
    }
    else {
        SetPendingInst(pCodecInst->coreIdx, pCodecInst);
    }

    return ret;
}

RetCode VPU_EncCompleteSeqInit(EncHandle handle, EncInitialInfo * info)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;
    VpuAttr*    pAttr;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    if (info == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (pAttr->supportCommandQueue == TRUE) {
        if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }
    }
    else {
        if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(pCodecInst->coreIdx);
            return RETCODE_WRONG_CALL_SEQUENCE;
        }
    }

    ret = ProductVpuEncGetSeqInfo(handle, info);
    if (ret == RETCODE_SUCCESS) {
        pEncInfo->initialInfoObtained = 1;
    }

    pEncInfo->initialInfo = *info;

    SetPendingInst(pCodecInst->coreIdx, NULL);

    LeaveLock(pCodecInst->coreIdx);

    return ret;
}

RetCode VPU_EncSetWrPtr(EncHandle handle, PhysicalAddress addr, int updateRdPtr)
{
    CodecInst* pCodecInst;
    CodecInst* pPendingInst;
    EncInfo * pEncInfo;
    RetCode ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;
    pCodecInst = (CodecInst*)handle;

    if (pCodecInst->productId == PRODUCT_ID_960 || pCodecInst->productId == PRODUCT_ID_980) {
        return RETCODE_NOT_SUPPORTED_FEATURE;
    }

    pEncInfo = &handle->CodecInfo->encInfo;
    pPendingInst = GetPendingInst(pCodecInst->coreIdx);
    if (pCodecInst == pPendingInst) {
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr, addr);
    }
    else {
        if (EnterLock(pCodecInst->coreIdx) != RETCODE_SUCCESS) {
            return RETCODE_FAILURE;
        }
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr, addr);
        LeaveLock(pCodecInst->coreIdx);
    }
    pEncInfo->streamWrPtr = addr;
    if (updateRdPtr)
        pEncInfo->streamRdPtr = addr;

    return RETCODE_SUCCESS;
}

PhysicalAddress VPU_MapToAddr40Bit(int coreIdx, unsigned int Addr)
{
    PhysicalAddress RealAddr = (PhysicalAddress)vdi_get_ddr_map(coreIdx);

    RealAddr = (RealAddr << 32) | (PhysicalAddress)Addr;

    return RealAddr;
}

int VPU_DecRequestCore(void)
{
    int i;
    int core_idx;
    int min_instance = MAX_NUM_INSTANCE;
    int instance_count [MAX_NUM_VPU_CORE];

    mutex_lock(&__vdi_init_mutex);
    for (i=1; i<MAX_NUM_VPU_CORE; i++) {
        instance_count[i] = vdi_get_instance_count(i);
        if (instance_count[i] <= min_instance) {
            min_instance = instance_count[i];
            core_idx = i;
        }
    }

    vdi_request_instance(core_idx);
    mutex_unlock(&__vdi_init_mutex);

    return core_idx;
}

int VPU_DecReleaseCore(unsigned int core_idx)
{
    mutex_lock(&__vdi_init_mutex);
    vdi_release_instance(core_idx);
    mutex_unlock(&__vdi_init_mutex);

    return 0;
}


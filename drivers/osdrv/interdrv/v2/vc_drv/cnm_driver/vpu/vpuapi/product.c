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

#include "product.h"
#include "coda9/coda9.h"
#include "wave/wave5.h"
#include "wave/wave6.h"

VpuAttr g_VpuCoreAttributes[MAX_NUM_VPU_CORE];

static Int32 s_ProductIds[MAX_NUM_VPU_CORE] = {
    PRODUCT_ID_NONE,
#if 1//def SUPPORT_MULTI_CORE_IN_ONE_DRIVER
    PRODUCT_ID_NONE,
    PRODUCT_ID_NONE,
#endif

};

typedef struct FrameBufInfoStruct {
    Uint32 unitSizeHorLuma;
    Uint32 sizeLuma;
    Uint32 sizeChroma;
    BOOL   fieldMap;
} FrameBufInfo;


Uint32 ProductVpuScan(Uint32 coreIdx)
{
    Uint32 id, val;
    Uint32 foundProducts = 0;

    /* Already scanned */
    if (s_ProductIds[coreIdx] != PRODUCT_ID_NONE)
        return 1;

   {
        val = VpuReadReg(coreIdx, VPU_PRODUCT_CODE_REGISTER);

        switch(val) {
        case BODA950_CODE:
        case CODA960_CODE:
            id = PRODUCT_ID_960;
            break;
        case CODA980_CODE:
            id = PRODUCT_ID_980;
            break;
        case WAVE511_CODE:
            id = PRODUCT_ID_511;
            break;
        case WAVE521_CODE:
        case WAVE521C_CODE:
        case WAVE521C_DUAL_CODE:
        case WAVE521E1_CODE:
            id = PRODUCT_ID_521;
            break;
        case WAVE517_CODE:
        case WAVE537_CODE:
            id = PRODUCT_ID_517;
            break;
        case WAVE617_CODE:
            id = PRODUCT_ID_617;
            break;
        case WAVE627_CODE:
            id = PRODUCT_ID_627;
            break;
        case WAVE633_CODE:
        case WAVE637_CODE:
        case WAVE663_CODE:
        case WAVE677_CODE:
            id = PRODUCT_ID_637;
            break;
        default:
            id = PRODUCT_ID_NONE;
            VLOG(ERR, "Cannot find productId(%x)\n", val);
            break;
        }

        if (id != PRODUCT_ID_NONE) {
            s_ProductIds[coreIdx] = id;
            g_VpuCoreAttributes[coreIdx].productId = id;
            foundProducts++;
        }
    }

    return (foundProducts >= 1);
}

Int32 ProductVpuGetId(Uint32 coreIdx)
{
    return s_ProductIds[coreIdx];
}

RetCode ProductVpuGetVersion(
    Uint32  coreIdx,
    Uint32* versionInfo,
    Uint32* revision
    )
{
    Int32   productId = s_ProductIds[coreIdx];
    RetCode ret = RETCODE_SUCCESS;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuGetVersion(coreIdx, versionInfo, revision);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuGetVersion(coreIdx, versionInfo, revision);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuGetVersion(coreIdx, versionInfo, revision);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

RetCode ProductVpuGetProductInfo(
    Uint32  coreIdx,
    VpuAttr* attr
    )
{
    Int32   productId = s_ProductIds[coreIdx];
    RetCode ret = RETCODE_SUCCESS;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        osal_memcpy(attr, &g_VpuCoreAttributes[coreIdx], sizeof(VpuAttr));
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

RetCode ProductVpuInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS;
    int     productId;

    productId  = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuInit(coreIdx, firmware, size);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuInit(coreIdx, firmware, size);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuInit(coreIdx, firmware, size);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
        break;
    }

    return ret;
}

RetCode ProductVpuReInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS;
    int     productId;

    productId  = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuReInit(coreIdx, firmware, size);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuReInit(coreIdx, firmware, size);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuReInit(coreIdx, firmware, size);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

Uint32 ProductVpuIsInit(Uint32 coreIdx)
{
    Uint32  pc = 0;
    int     productId;

    productId  = s_ProductIds[coreIdx];

    if (productId == PRODUCT_ID_NONE) {
        ProductVpuScan(coreIdx);
        productId  = s_ProductIds[coreIdx];
    }

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        pc = Coda9VpuIsInit(coreIdx);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        pc = Wave5VpuIsInit(coreIdx);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        pc = Wave6VpuIsInit(coreIdx);
        break;
    }

    return pc;
}

Int32 ProductVpuIsBusy(Uint32 coreIdx)
{
    Int32  busy;
    int    productId;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        busy = Coda9VpuIsBusy(coreIdx);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        busy = Wave5VpuIsBusy(coreIdx);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        busy = Wave6VpuIsBusy(coreIdx);
        break;
    default:
        busy = 0;
        break;
    }

    return busy;
}

Int32 ProductVpuWaitInterrupt(CodecInst *instance, Int32 timeout)
{
    int     productId;
    int     flag = -1;

    productId = s_ProductIds[instance->coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        flag = Coda9VpuWaitInterrupt(instance, timeout);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        flag = Wave5VpuWaitInterrupt(instance, timeout, FALSE);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        flag = Wave6VpuWaitInterrupt(instance, timeout);
        break;
    default:
        flag = -1;
        break;
    }

    return flag;
}

RetCode ProductVpuSetBusTransaction(Uint32 coreIdx, BOOL enable)
{
    int     productId;
    RetCode ret = RETCODE_SUCCESS;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuSetBusTransaction(coreIdx, enable);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
        break;
    }

    return ret;
}

RetCode ProductVpuReset(Uint32 coreIdx, SWResetMode resetMode)
{
    int     productId;
    RetCode ret = RETCODE_SUCCESS;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuReset(coreIdx, resetMode);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuReset(coreIdx, resetMode);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuReset(coreIdx);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
        break;
    }

    return ret;
}

RetCode ProductVpuSleepWake(Uint32 coreIdx, int iSleepWake, const Uint16* code, Uint32 size)
{
    int     productId;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuSleepWake(coreIdx, iSleepWake, (void*)code, size);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuSleepWake(coreIdx, iSleepWake, (void*)code, size);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuSleepWake(coreIdx, iSleepWake, (void*)code, size);
        break;
    }

    return ret;
}
RetCode ProductVpuClearInterrupt(Uint32 coreIdx, Uint32 flags)
{
    int     productId;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuClearInterrupt(coreIdx);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuClearInterrupt(coreIdx, flags);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuClearInterrupt(coreIdx, flags);
        break;
    }

    return ret;
}

RetCode ProductVpuDecBuildUpOpenParam(CodecInst* pCodec, DecOpenParam* param)
{
    Int32   productId;
    Uint32  coreIdx;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    coreIdx   = pCodec->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuBuildUpDecParam(pCodec, param);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuBuildUpDecParam(pCodec, param);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuBuildUpDecParam(pCodec, param);
        break;
    }

    return ret;
}

PhysicalAddress ProductVpuDecGetRdPtr(CodecInst* instance)
{
    Int32   productId;
    Uint32  coreIdx;
    PhysicalAddress retRdPtr;
    DecInfo*    pDecInfo;
    RetCode ret = RETCODE_SUCCESS;

    pDecInfo = VPU_HANDLE_TO_DECINFO(instance);

    coreIdx   = instance->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuDecGetRdPtr(instance, &retRdPtr);
        if (ret != RETCODE_SUCCESS) {
            retRdPtr = pDecInfo->streamRdPtr;
        }
        else {
            pDecInfo->streamRdPtr = retRdPtr;
        }
        break;
    default:
        retRdPtr = VpuReadReg(coreIdx, pDecInfo->streamRdPtrRegAddr);
        break;
    }
    retRdPtr = VPU_MapToAddr40Bit(coreIdx, retRdPtr);

    return retRdPtr;
}

RetCode ProductVpuDecCheckOpenParam(DecOpenParam* param)
{
    Int32       productId;
    Uint32      coreIdx;
    VpuAttr*    pAttr;

    if (param == 0) {
        VLOG(ERR, "param ptr is null\n");
        return RETCODE_INVALID_PARAM;
    }

    if (param->coreIdx > MAX_NUM_VPU_CORE) {
        VLOG(ERR, "invalid core_id:%d\n", param->coreIdx);
        return RETCODE_INVALID_PARAM;
    }

    coreIdx   = param->coreIdx;
    productId = s_ProductIds[coreIdx];
    pAttr     = &g_VpuCoreAttributes[coreIdx];

    if (param->bitstreamBuffer % 8) {
        VLOG(ERR, "Invalid bitstream addr:0x%x\n", param->bitstreamBuffer);
        return RETCODE_INVALID_PARAM;
    }

    if (param->bitstreamMode == BS_MODE_INTERRUPT) {
        if (param->bitstreamBufferSize % 1024 || param->bitstreamBufferSize < 1024) {
            VLOG(ERR, "Invalid buffer size %d\n", param->bitstreamBufferSize);
            return RETCODE_INVALID_PARAM;
        }
    }

    if (PRODUCT_ID_W_SERIES(productId)) {
    }

    // Check bitstream mode
    if ((pAttr->supportBitstreamMode&(1<<param->bitstreamMode)) == 0) {
        VLOG(ERR, "Invalid bitstream mode [0x%x, 0x%x]\n", pAttr->supportBitstreamMode, param->bitstreamMode);
        return RETCODE_INVALID_PARAM;
    }

    if ((pAttr->supportDecoders&(1<<param->bitstreamFormat)) == 0) {
        VLOG(ERR, "Invalid bitstream format [0x%x, 0x%x]\n", pAttr->supportDecoders, param->bitstreamFormat);
        return RETCODE_INVALID_PARAM;
    }

    /* check framebuffer endian */
    if ((pAttr->supportEndianMask&(1<<param->frameEndian)) == 0) {
        VLOG(ERR, "Invalid frame endian(%d)\n", (Int32)param->frameEndian);
        return RETCODE_INVALID_PARAM;
    }

    /* check streambuffer endian */
    if ((pAttr->supportEndianMask&(1<<param->streamEndian)) == 0) {
        VLOG(ERR, "Invalid stream endian(%d)\n", (Int32)param->streamEndian);
        return RETCODE_INVALID_PARAM;
    }

    /* check WTL */
    if (param->wtlEnable) {
        if (pAttr->supportWTL == 0)
            return RETCODE_NOT_SUPPORTED_FEATURE;
        switch (productId) {
        case PRODUCT_ID_960:
        case PRODUCT_ID_980:
            if (param->wtlMode != FF_FRAME && param->wtlMode != FF_FIELD ) {
                VLOG(ERR, "Invalid wtlMode(%d)\n", param->wtlMode);
                return RETCODE_INVALID_PARAM;
            }
            break;
        default:
            break;
        }
    }

    /* Tiled2Linear */
    if (param->tiled2LinearEnable) {
        if (pAttr->supportTiled2Linear == 0)
            return RETCODE_NOT_SUPPORTED_FEATURE;

        if (productId == PRODUCT_ID_960 || productId == PRODUCT_ID_980) {
            if (param->tiled2LinearMode != FF_FRAME && param->tiled2LinearMode != FF_FIELD ) {
                VLOG(ERR, "Invalid Tiled2LinearMode(%d)\n", (Int32)param->tiled2LinearMode);
                return RETCODE_INVALID_PARAM;
            }
        }
    }
    if (productId == PRODUCT_ID_960 || productId == PRODUCT_ID_980) {
        if( param->mp4DeblkEnable == 1 && !(param->bitstreamFormat == STD_MPEG4 || param->bitstreamFormat == STD_H263 || param->bitstreamFormat == STD_MPEG2 || param->bitstreamFormat == STD_DIV3))
            return RETCODE_INVALID_PARAM;
        if (param->wtlEnable && param->tiled2LinearEnable)
            return RETCODE_INVALID_PARAM;
    }
    else {
        if (param->mp4DeblkEnable || param->mp4Class) {
            VLOG(ERR, "Invalid mp4DeblkEnable(%d)\n", (Int32)param->mp4DeblkEnable);
            return RETCODE_INVALID_PARAM;
        }

        if (param->avcExtension) {
            VLOG(ERR, "Invalid avcExtension(%d)\n", (Int32)param->avcExtension);
            return RETCODE_INVALID_PARAM;
        }

        if (param->tiled2LinearMode != FF_NONE) {
            VLOG(ERR, "Invalid Tiled2LinearMode(%d)\n", (Int32)param->tiled2LinearMode);
            return RETCODE_INVALID_PARAM;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode ProductVpuDecSetRdPtr(CodecInst* instance, PhysicalAddress rdPtr)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (instance->productId) {
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuDecSetRdPtr(instance, rdPtr);
        break;
    default:
        ret = RETCODE_INVALID_COMMAND;
        break;
    }

    return ret;
}

RetCode ProductVpuDecInitSeq(CodecInst* instance)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecInitSeq(instance);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuDecInitSeq(instance);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecInitSeq(instance);
        break;
    }

    return ret;
}

RetCode ProductVpuDecFiniSeq(CodecInst* instance)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuFiniSeq(instance);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuDecFiniSeq(instance);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecFiniSeq(instance);
        break;
    }

    return ret;
}

RetCode ProductVpuDecGetSeqInfo(CodecInst* instance, DecInitialInfo* info)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecGetSeqInfo(instance, info);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuDecGetSeqInfo(instance, info);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecGetSeqInfo(instance, info);
        break;
    }

    return ret;
}

RetCode ProductVpuDecCheckCapability(CodecInst* instance)
{
    DecInfo* pDecInfo;
    VpuAttr* pAttr     = &g_VpuCoreAttributes[instance->coreIdx];

    pDecInfo = &instance->CodecInfo->decInfo;

    if ((pAttr->supportDecoders&(1<<pDecInfo->openParam.bitstreamFormat)) == 0)
        return RETCODE_NOT_SUPPORTED_FEATURE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
        if (pDecInfo->mapType >= TILED_FRAME_NO_BANK_MAP)
            return RETCODE_NOT_SUPPORTED_FEATURE;
        if (pDecInfo->tiled2LinearMode == FF_FIELD)
            return RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_980:
        if (pDecInfo->mapType >= COMPRESSED_FRAME_MAP)
            return RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        if (pDecInfo->mapType != LINEAR_FRAME_MAP && pDecInfo->mapType != COMPRESSED_FRAME_MAP && pDecInfo->mapType != COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT && pDecInfo->mapType != COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT)
            return RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        if (pDecInfo->mapType != LINEAR_FRAME_MAP && pDecInfo->mapType != COMPRESSED_FRAME_MAP)
            return RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return RETCODE_SUCCESS;
}

RetCode ProductVpuDecode(CodecInst* instance, DecParam* option)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecode(instance, option);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuDecode(instance, option);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecode(instance, option);
        break;
    }

    return ret;
}

RetCode ProductVpuDecGetResult(CodecInst*  instance, DecOutputInfo* result)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecGetResult(instance, result);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuDecGetResult(instance, result);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecGetResult(instance, result);
        break;
    }

    return ret;
}

RetCode ProductVpuDecFlush(CodecInst* instance, FramebufferIndex* retIndexes, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecFlush(instance, retIndexes, size);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuDecFlush(instance, retIndexes, size);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecFlush(instance, retIndexes, size);
        break;
    }

    return ret;
}

/************************************************************************/
/* Decoder & Encoder                                                    */
/************************************************************************/

RetCode ProductVpuDecSetBitstreamFlag(
    CodecInst*  instance,
    BOOL        running,
    Int32       size
    )
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    BOOL        eos;
    BOOL        checkEos;
    BOOL        explicitEnd;
    DecInfo*    pDecInfo = &instance->CodecInfo->decInfo;

    productId = instance->productId;

    eos      = (BOOL)(size == 0);
    checkEos = (BOOL)(size > 0);
    explicitEnd = (BOOL)(size == -2);

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        if (checkEos || explicitEnd) eos = (BOOL)((pDecInfo->streamEndflag&0x04) == 0x04);
        ret = Coda9VpuDecSetBitstreamFlag(instance, running, eos);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        if (checkEos || explicitEnd) eos = (BOOL)pDecInfo->streamEndflag;
        ret = Wave5VpuDecSetBitstreamFlag(instance, running, eos, explicitEnd);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        if (checkEos || explicitEnd) eos = (BOOL)pDecInfo->streamEndflag;
        pDecInfo->streamEndflag = (eos == 1) ? TRUE : FALSE;
        ret = RETCODE_SUCCESS;
        break;
    }

    return ret;
}

/**
 * \param   stride          stride of framebuffer in pixel.
 */
RetCode ProductVpuDecAllocateFramebuffer(
    CodecInst* inst, FrameBuffer* fbArr, TiledMapType mapType, Int32 num,
    Int32 stride, Int32 height, FrameBufferFormat format,
    BOOL cbcrInterleave, BOOL nv21, Int32 endian,
    vpu_buffer_t* vb, Int32 gdiIndex,
    FramebufferAllocType fbType)
{
    Int32        i;
    Uint32       coreIdx;
    vpu_buffer_t vbFrame;
    FrameBufInfo fbInfo;
    DecInfo*     pDecInfo = &inst->CodecInfo->decInfo;
    Uint32       sizeLuma;
    Uint32       sizeChroma;
    ProductId    productId     = (ProductId)inst->productId;
    RetCode      ret           = RETCODE_SUCCESS;

    osal_memset((void*)&vbFrame, 0x00, sizeof(vpu_buffer_t));
    osal_memset((void*)&fbInfo,  0x00, sizeof(FrameBufInfo));

    coreIdx = inst->coreIdx;

    if (inst->codecMode == W_VP9_DEC) {
        Uint32 framebufHeight;

        framebufHeight = VPU_ALIGN64(height);
        sizeLuma   = CalcLumaSize(inst, inst->productId, stride, framebufHeight, format, cbcrInterleave, mapType, NULL);
        sizeChroma = CalcChromaSize(inst, inst->productId, stride, framebufHeight, format, cbcrInterleave, mapType, NULL);
    }
    else {
        DRAMConfig* bufferConfig = NULL;
        if (productId == PRODUCT_ID_960) {
            bufferConfig = &pDecInfo->dramCfg;
        }
        sizeLuma   = CalcLumaSize(inst, inst->productId, stride, height, format, cbcrInterleave, mapType, bufferConfig);
        sizeChroma = CalcChromaSize(inst, inst->productId, stride, height, format, cbcrInterleave, mapType, bufferConfig);
    }

    // Framebuffer common informations
    for (i=0; i<num; i++) {
        if (fbArr[i].updateFbInfo == TRUE) {
            fbArr[i].updateFbInfo = FALSE;
            fbArr[i].myIndex        = i+gdiIndex;
            fbArr[i].stride         = stride;
            fbArr[i].height         = height;
            fbArr[i].mapType        = mapType;
            fbArr[i].format         = format;
            fbArr[i].cbcrInterleave = (mapType >= COMPRESSED_FRAME_MAP ? TRUE : cbcrInterleave);
            fbArr[i].nv21           = nv21;
            fbArr[i].endian         = (mapType >= COMPRESSED_FRAME_MAP ? VDI_128BIT_LITTLE_ENDIAN : endian);
            fbArr[i].lumaBitDepth   = pDecInfo->initialInfo.lumaBitdepth;
            fbArr[i].chromaBitDepth = pDecInfo->initialInfo.chromaBitdepth;
            fbArr[i].bufYSize       = sizeLuma;
            fbArr[i].bufCbSize      = (mapType < COMPRESSED_FRAME_MAP && cbcrInterleave == TRUE) ? sizeChroma * 2 : sizeChroma;
            fbArr[i].bufCrSize      = (cbcrInterleave == TRUE) ? 0 : sizeChroma;
        }
    }

    switch (mapType) {
    case LINEAR_FRAME_MAP:
    case LINEAR_FIELD_MAP:
    case COMPRESSED_FRAME_MAP:
    case COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT:
    case COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT:
        ret = UpdateFrameBufferAddr(mapType, fbArr, num, sizeLuma, sizeChroma);
        if (ret != RETCODE_SUCCESS) {
            break;
        }
        break;
    default:
        /* Tiled map */
        if (productId == PRODUCT_ID_960) {
            DRAMConfig*     pDramCfg;
            PhysicalAddress tiledBaseAddr = 0;
            TiledMapConfig* pMapCfg;

            pDramCfg = &pDecInfo->dramCfg;
            pMapCfg  = &pDecInfo->mapCfg;

            vbFrame.phys_addr = GetTiledFrameBase(coreIdx, fbArr, num);
            if (fbType == FB_TYPE_PPU) {
                tiledBaseAddr = pMapCfg->tiledBaseAddr;
            }
            else {
                pMapCfg->tiledBaseAddr = vbFrame.phys_addr;
                tiledBaseAddr = vbFrame.phys_addr;
            }
            *vb = vbFrame;
            ret = AllocateTiledFrameBufferGdiV1(mapType, tiledBaseAddr, fbArr, num, sizeLuma, sizeChroma, pDramCfg);
        }
        else {
            // PRODUCT_ID_980
            ret = AllocateTiledFrameBufferGdiV2(mapType, fbArr, num, sizeLuma, sizeChroma);
        }
        break;
    }

    return ret;
}

RetCode ProductVpuDecGetAuxBufSize(CodecInst* inst, DecAuxBufferSizeInfo sizeInfo, Uint32* size)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (inst->productId) {
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecGetAuxBufSize(inst, sizeInfo, size);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuDecRegisterAuxBuffer(CodecInst* inst, AuxBufferInfo auxBufferInfo)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (inst->productId) {
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecRegisterAuxBuffer(inst, auxBufferInfo);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuDecRegisterFramebuffer(CodecInst* instance)
{
    RetCode         ret = RETCODE_FAILURE;
    FrameBuffer*    fb;
    DecInfo*        pDecInfo = &instance->CodecInfo->decInfo;
    Int32           gdiIndex = 0;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecRegisterFramebuffer(instance);
        break;
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        if (pDecInfo->mapType < COMPRESSED_FRAME_MAP)
            return RETCODE_NOT_SUPPORTED_FEATURE;

        fb = pDecInfo->frameBufPool;

        gdiIndex = 0;
        if (pDecInfo->wtlEnable == TRUE) {
            if (fb[0].mapType >= COMPRESSED_FRAME_MAP)
                gdiIndex = pDecInfo->numFbsForDecoding;

            ret = Wave5VpuDecRegisterFramebuffer(instance, &fb[gdiIndex], LINEAR_FRAME_MAP, pDecInfo->numFbsForWTL);
            if (ret != RETCODE_SUCCESS)
                return ret;
            gdiIndex = gdiIndex == 0 ? pDecInfo->numFbsForDecoding: 0;
        }

        ret = Wave5VpuDecRegisterFramebuffer(instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP, pDecInfo->numFbsForDecoding);
        if (ret != RETCODE_SUCCESS)
            return ret;
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        if (pDecInfo->mapType < COMPRESSED_FRAME_MAP)
            return RETCODE_NOT_SUPPORTED_FEATURE;

        fb = pDecInfo->frameBufPool;

        gdiIndex = 0;
        if (pDecInfo->wtlEnable == TRUE) {
            if (fb[0].mapType >= COMPRESSED_FRAME_MAP)
                gdiIndex = pDecInfo->numFbsForDecoding;

            ret = Wave6VpuDecRegisterFramebuffer(instance, &fb[gdiIndex], LINEAR_FRAME_MAP, pDecInfo->numFbsForWTL);
            if (ret != RETCODE_SUCCESS)
                return ret;
            gdiIndex = gdiIndex == 0 ? pDecInfo->numFbsForDecoding: 0;
        }

        ret = Wave6VpuDecRegisterFramebuffer(instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP, pDecInfo->numFbsForDecoding);
        if (ret != RETCODE_SUCCESS)
            return ret;
        break;
    }
    return ret;
}

RetCode ProductVpuDecUpdateFrameBuffer(CodecInst* instance, FrameBuffer* fbcFb, FrameBuffer* linearFb, Uint32 mvColIndex, Uint32 picWidth, Uint32 picHeight)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (instance->productId) {
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuDecUpdateFramebuffer(instance, fbcFb, linearFb, mvColIndex, picWidth, picHeight);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecUpdateFramebuffer(instance, fbcFb, linearFb, mvColIndex, picWidth, picHeight);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

Int32 ProductCalculateFrameBufStride(CodecInst* inst, Int32 width, Int32 height, TiledMapType mapType, FrameBufferFormat format, BOOL interleave)
{
    BOOL isVP9 = (inst->codecMode == W_VP9_DEC) ? TRUE : FALSE;
    return CalcStride(inst->productId, width, height, format, interleave, mapType, isVP9);
}

Int32 ProductCalculateFrameBufSize(CodecInst* inst, Int32 productId, Int32 stride, Int32 height, TiledMapType mapType, FrameBufferFormat format, BOOL interleave, DRAMConfig* pDramCfg)
{
    Int32 size_dpb_lum, size_dpb_chr, size_dpb_all;

    size_dpb_lum = CalcLumaSize(inst, productId, stride, height, format, interleave, mapType, pDramCfg);
    size_dpb_chr = CalcChromaSize(inst, productId, stride, height, format, interleave, mapType, pDramCfg);

    if (mapType < COMPRESSED_FRAME_MAP)
        size_dpb_all = size_dpb_lum + size_dpb_chr*2;
    else
        size_dpb_all = size_dpb_lum + size_dpb_chr;

    return size_dpb_all;
}


RetCode ProductVpuDecClrDispFlag(CodecInst* instance, Uint32 index)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (instance->productId) {
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5DecClrDispFlag(instance, index);
        break;
    default:
        ret = RETCODE_INVALID_COMMAND;
        break;
    }

    return ret;
}

RetCode ProductVpuDecSetDispFlag(CodecInst* instance, Uint32 index)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (instance->productId) {
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5DecSetDispFlag(instance, index);
        break;
    default:
        ret = RETCODE_INVALID_COMMAND;
        break;
    }

    return ret;
}

RetCode ProductVpuGetBandwidth(CodecInst* instance, VPUBWData* data)
{
    RetCode ret = RETCODE_SUCCESS;

    if (data == 0) {
        return RETCODE_INVALID_PARAM;
    }

    switch (instance->productId) {
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuGetBwReport(instance, data);
        break;
    default:
        ret = RETCODE_INVALID_COMMAND;
        break;
    }

    return ret;
}


RetCode ProductVpuGetDebugInfo(CodecInst* instance, VpuDebugInfo* info)
{
    RetCode ret = RETCODE_SUCCESS;

    if (info == 0) {
        return RETCODE_INVALID_PARAM;
    }

    switch (instance->productId) {
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
        ret = Wave5VpuGetDebugInfo(instance, info);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuGetDebugInfo(instance, info);
        break;
    default:
        ret = RETCODE_INVALID_COMMAND;
        break;
    }

    return ret;
}

RetCode ProductVpuDecGiveCommand(CodecInst* instance, CodecCommand cmd, void* param)
{
    RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecGiveCommand(instance, cmd, param);
        break;
    case PRODUCT_ID_511:
    case PRODUCT_ID_517:
    case PRODUCT_ID_521:
        ret = Wave5VpuDecGiveCommand(instance, cmd, param);
        break;
    case PRODUCT_ID_617:
    case PRODUCT_ID_637:
        ret = Wave6VpuDecGiveCommand(instance, cmd, param);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

/************************************************************************/
/* ENCODER                                                              */
/************************************************************************/
RetCode ProductVpuEncGetSrcBufFlag(CodecInst* instance, Uint32* flag)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (instance->productId) {
    case PRODUCT_ID_521:
        ret = Wave5VpuEncGetSrcBufFlag(instance, flag);
        break;
    default:
        ret = RETCODE_INVALID_COMMAND;
        break;
    }

    return ret;
}

RetCode ProductVpuEncCheckOpenParam(EncOpenParam* pop)
{
    RetCode ret = RETCODE_SUCCESS;
    Int32 productId;

    if (pop == 0)
        return RETCODE_INVALID_PARAM;

    if (pop->coreIdx > MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;

    productId = s_ProductIds[pop->coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuEncCheckOpenParam(pop);
        break;
    case PRODUCT_ID_521:
        ret = Wave5VpuEncCheckOpenParam(pop);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncCheckOpenParam(*pop);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncFiniSeq(CodecInst* instance)
{
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuFiniSeq(instance);
        break;
    case PRODUCT_ID_521:
        ret = Wave5VpuEncFiniSeq(instance);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncFiniSeq(instance);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }
    return ret;
}

RetCode ProductVpuEncGetHeader(CodecInst* instance, EncHeaderParam* encHeaderParam)
{
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuEncGetHeader(instance, encHeaderParam);
        break;
    case PRODUCT_ID_521:
        ret = Wave5VpuEncGetHeader(instance, encHeaderParam);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncGetHeader(instance, encHeaderParam);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncSetup(CodecInst* instance)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuEncSetup(instance);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncode(CodecInst* instance, EncParam* param)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuEncode(instance, param);
        break;
    case PRODUCT_ID_521:
        ret = Wave5VpuEncode(instance, param);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncode(instance, param);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncGetResult(CodecInst* instance, EncOutputInfo* result)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuEncGetResult(instance, result);
        break;
    case PRODUCT_ID_521:
        ret = Wave5VpuEncGetResult(instance, result);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncGetResult(instance, result);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncGiveCommand(CodecInst* instance, CodecCommand cmd, void* param)
{
    RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuEncGiveCommand(instance, cmd, param);
        break;
    case PRODUCT_ID_521:
        ret = Wave5VpuEncGiveCommand(instance, cmd, param);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncGiveCommand(instance, cmd, param);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncInitSeq(CodecInst* instance)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_521:
        ret = Wave5VpuEncInitSeq(instance);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncInitSeq(instance);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncGetSeqInfo(CodecInst* instance, EncInitialInfo* info)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_521:
        ret = Wave5VpuEncGetSeqInfo(instance, info);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncGetSeqInfo(instance, info);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncChangeParam(CodecInst* instance, void* param)
{
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuEncParaChange(instance, (ParamChange*)param);
        break;
    case PRODUCT_ID_521:
        ret = Wave5VpuEncParaChange(instance, (EncChangeParam*)param);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncParaChange(instance, (EncWave6ChangeParam*)param);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncSetPeriod(CodecInst* instance, void* param)
{
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    int         productId;
    int period = *(int *)param;
    EncChangeParam stChangeParam = {0};

    productId = instance->productId;

    switch (productId) {
    case PRODUCT_ID_521:
        stChangeParam.enable_option = W5_ENC_CHANGE_PARAM_INTRA_PARAM;
        stChangeParam.intraPeriod = period;
        stChangeParam.avcIdrPeriod = period;
        ret = Wave5VpuEncParaChange(instance, &stChangeParam);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncRegisterFramebuffer(CodecInst* instance)
{
    RetCode         ret = RETCODE_FAILURE;
    FrameBuffer*    fb;
    EncInfo*        pEncInfo = &instance->CodecInfo->encInfo;

    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuEncRegisterFramebuffer(instance);
        break;
    case PRODUCT_ID_521:
        if (pEncInfo->mapType < COMPRESSED_FRAME_MAP)
            return RETCODE_NOT_SUPPORTED_FEATURE;

        fb = pEncInfo->frameBufPool;

        ret = Wave5VpuEncRegisterFramebuffer(instance, &fb[0], pEncInfo->numFrameBuffers);

        if (ret != RETCODE_SUCCESS)
            return ret;
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        if (pEncInfo->mapType < COMPRESSED_FRAME_MAP)
            return RETCODE_NOT_SUPPORTED_FEATURE;

        fb = pEncInfo->frameBufPool;

        ret = Wave6VpuEncRegisterFramebuffer(instance, &fb[0]);

        if (ret != RETCODE_SUCCESS)
            return ret;
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }
    return ret;
}

RetCode ProductVpuEncAllocateFramebuffer(
    CodecInst* inst, FrameBuffer* fbArr, TiledMapType mapType, Int32 num,
    Int32 stride, Int32 height, FrameBufferFormat format,
    BOOL cbcrInterleave, BOOL nv21, Int32 endian,
    vpu_buffer_t* vb, Int32 gdiIndex,
    FramebufferAllocType fbType)
{
    Int32        i;
    Uint32       coreIdx;
    vpu_buffer_t vbFrame;
    FrameBufInfo fbInfo;
    EncInfo*     pEncInfo = &inst->CodecInfo->encInfo;
    Uint32       sizeLuma;
    Uint32       sizeChroma;
    ProductId    productId     = (ProductId)inst->productId;
    RetCode      ret           = RETCODE_SUCCESS;
    DRAMConfig*  bufferConfig = NULL;

    osal_memset((void*)&vbFrame, 0x00, sizeof(vpu_buffer_t));
    osal_memset((void*)&fbInfo,  0x00, sizeof(FrameBufInfo));

    coreIdx = inst->coreIdx;

    if (productId == PRODUCT_ID_960) {
        bufferConfig = &pEncInfo->dramCfg;
    }
    sizeLuma   = CalcLumaSize(inst, inst->productId, stride, height, format, cbcrInterleave, mapType, bufferConfig);
    sizeChroma = CalcChromaSize(inst, inst->productId, stride, height, format, cbcrInterleave, mapType, bufferConfig);

    // Framebuffer common informations
    for (i=0; i<num; i++) {
        if (fbArr[i].updateFbInfo == TRUE) {
            fbArr[i].updateFbInfo = FALSE;
            fbArr[i].myIndex        = i+gdiIndex;
            fbArr[i].stride         = stride;
            fbArr[i].height         = height;
            fbArr[i].mapType        = mapType;
            fbArr[i].format         = format;
            fbArr[i].cbcrInterleave = (mapType >= COMPRESSED_FRAME_MAP ? TRUE : cbcrInterleave);
            fbArr[i].nv21           = nv21;

            switch (mapType) {
            case COMPRESSED_FRAME_MAP:
            case COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT:
            case COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT:
                fbArr[i].endian = VDI_128BIT_LITTLE_ENDIAN;
                break;
            default:
                fbArr[i].endian = endian;
                break;
            }

            fbArr[i].bufYSize       = sizeLuma;
            fbArr[i].bufCbSize      = (mapType < COMPRESSED_FRAME_MAP && cbcrInterleave == TRUE) ? sizeChroma * 2 : sizeChroma;
            fbArr[i].bufCrSize      = (cbcrInterleave == TRUE) ? 0 : sizeChroma;
            fbArr[i].sourceLBurstEn = FALSE;
            if (PRODUCT_ID_CODA_SERIES(productId)) {
                fbArr[i].lumaBitDepth   = pEncInfo->openParam.srcBitDepth;
                fbArr[i].chromaBitDepth = pEncInfo->openParam.srcBitDepth;
            } else if (PRODUCT_ID_W5_SERIES(productId)) {
                fbArr[i].lumaBitDepth   = pEncInfo->openParam.EncStdParam.waveParam.internalBitDepth;
                fbArr[i].chromaBitDepth = pEncInfo->openParam.EncStdParam.waveParam.internalBitDepth;
            } else if (PRODUCT_ID_W6_SERIES(productId)) {
                fbArr[i].lumaBitDepth   = pEncInfo->openParam.EncStdParam.wave6Param.internalBitDepth;
                fbArr[i].chromaBitDepth = pEncInfo->openParam.EncStdParam.wave6Param.internalBitDepth;
            }
        }
    }

    switch (mapType) {
    case LINEAR_FRAME_MAP:
    case LINEAR_FIELD_MAP:
    case COMPRESSED_FRAME_MAP:
    case COMPRESSED_FRAME_MAP_DUAL_CORE_8BIT:
    case COMPRESSED_FRAME_MAP_DUAL_CORE_10BIT:
        ret = UpdateFrameBufferAddr(mapType, fbArr, num, sizeLuma, sizeChroma);
        if (ret != RETCODE_SUCCESS) {
            break;
        }
        break;
    default:
        /* Tiled map */
        if (productId == PRODUCT_ID_960) {
            DRAMConfig*     pDramCfg;
            PhysicalAddress tiledBaseAddr = 0;
            TiledMapConfig* pMapCfg;

            pDramCfg = &pEncInfo->dramCfg;
            pMapCfg = &pEncInfo->mapCfg;

            vbFrame.phys_addr = GetTiledFrameBase(coreIdx, fbArr, num);
            if (fbType == FB_TYPE_PPU) {
                tiledBaseAddr = pMapCfg->tiledBaseAddr;
            }
            else {
                pMapCfg->tiledBaseAddr = vbFrame.phys_addr;
                tiledBaseAddr = vbFrame.phys_addr;
            }
            *vb = vbFrame;
            ret = AllocateTiledFrameBufferGdiV1(mapType, tiledBaseAddr, fbArr, num, sizeLuma, sizeChroma, pDramCfg);
        }
        else {
            // PRODUCT_ID_980
            ret = AllocateTiledFrameBufferGdiV2(mapType, fbArr, num, sizeLuma, sizeChroma);
        }
        break;
    }


    return ret;
}

RetCode ProductVpuEncGetAuxBufSize(CodecInst* inst, EncAuxBufferSizeInfo sizeInfo, Uint32* size)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (inst->productId) {
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncGetAuxBufSize(inst, sizeInfo, size);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncRegisterAuxBuffer(CodecInst* inst, AuxBufferInfo auxBufferInfo)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (inst->productId) {
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncRegisterAuxBuffer(inst, auxBufferInfo);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncUpdateBitstreamBuffer(CodecInst* instance)
{
    Int32   productId;
    Uint32  coreIdx;
    RetCode ret = RETCODE_SUCCESS;
    coreIdx   = instance->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_521:
        ret = Wave5VpuEncUpdateBS(instance);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncUpdateBS(instance);
        break;
    default:
        ret = RETCODE_INVALID_COMMAND;
    }

    return ret;
}

RetCode ProductVpuEncGetRdWrPtr(CodecInst* instance, PhysicalAddress* rdPtr, PhysicalAddress* wrPtr)
{
    Int32 productId;
    Uint32 coreIdx;
    EncInfo* pEncInfo;
    RetCode ret = RETCODE_SUCCESS;
    pEncInfo = VPU_HANDLE_TO_ENCINFO(instance);

    coreIdx   = instance->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_521:
        ret = Wave5VpuEncGetRdWrPtr(instance, rdPtr, wrPtr);
        if (ret != RETCODE_SUCCESS) {
            *rdPtr = pEncInfo->streamRdPtr;
            *wrPtr = pEncInfo->streamWrPtr;
        }
        else {
            pEncInfo->streamRdPtr = *rdPtr;
            pEncInfo->streamWrPtr = *wrPtr;
        }
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuEncGetRdWrPtr(instance, rdPtr, wrPtr);
        if (ret != RETCODE_SUCCESS) {
            *rdPtr = pEncInfo->streamRdPtr;
            *wrPtr = pEncInfo->streamWrPtr;
        }
        break;
    default:
        *wrPtr = pEncInfo->streamWrPtr;
        *rdPtr = pEncInfo->streamRdPtr;
        break;
    }

    return ret;

}

RetCode ProductVpuEncBuildUpOpenParam(CodecInst* pCodec, EncOpenParam* param)
{
    Int32   productId;
    Uint32  coreIdx;
    RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

    coreIdx   = pCodec->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuBuildUpEncParam(pCodec, param);
        break;
    case PRODUCT_ID_521:
        ret = Wave5VpuBuildUpEncParam(pCodec, param);
        break;
    case PRODUCT_ID_627:
    case PRODUCT_ID_637:
        ret = Wave6VpuBuildUpEncParam(pCodec, param);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
    }

    return ret;
}

RetCode ProductVpuEncCheckEncParam(CodecInst* instance, EncParam* param)
{
    EncInfo info = instance->CodecInfo->encInfo;
    VpuAttr attr = g_VpuCoreAttributes[instance->coreIdx];

    if (param == 0)
        return RETCODE_INVALID_PARAM;

    if (param->skipPicture != 0 && param->skipPicture != 1)
        return RETCODE_INVALID_PARAM;

    if (param->skipPicture == 0) {
        if (param->sourceFrame == 0)
            return RETCODE_INVALID_FRAME_BUFFER;
        if (param->forceIPicture != 0 && param->forceIPicture != 1)
            return RETCODE_INVALID_PARAM;
    }

    // no rate control
    if (info.openParam.bitRate == 0) {
        if (instance->codecMode == MP4_ENC) {
            if (param->quantParam < 1 || param->quantParam > 31)
                return RETCODE_INVALID_PARAM;
        }
        else if (instance->codecMode == AVC_ENC) {
            if (param->quantParam < 0 || param->quantParam > 51)
                return RETCODE_INVALID_PARAM;
        }
        else if (instance->codecMode == W_HEVC_ENC) {
            if (param->forcePicQpEnable == 1) {
                if (param->forcePicQpI < 0 || param->forcePicQpI > 63)
                    return RETCODE_INVALID_PARAM;
                if (param->forcePicQpP < 0 || param->forcePicQpP > 63)
                    return RETCODE_INVALID_PARAM;
                if (param->forcePicQpB < 0 || param->forcePicQpB > 63)
                    return RETCODE_INVALID_PARAM;
            }

            if (info.ringBufferEnable == 0) {
                if (param->picStreamBufferAddr % 16 || param->picStreamBufferSize == 0)
                    return RETCODE_INVALID_PARAM;
            }
        }
    }

    if (info.ringBufferEnable == 0) {
        if (param->picStreamBufferAddr % 8 || param->picStreamBufferSize == 0)
            return RETCODE_INVALID_PARAM;
    }

    if (PRODUCT_ID_W_SERIES(instance->productId)) {
        if (attr.supportOCSRAM == TRUE) {
            Uint32 width = (info.rotationAngle == 90 || info.rotationAngle == 270) ? info.encHeight : info.encWidth;

            if (instance->codecMode == W_HEVC_ENC &&
                width > 4096                      &&
                info.secAxiInfo.u.wave.useEncLfEnable) {
                VLOG(ERR, "Disable LF secAXI option. supportOCSRAM: %d | codecMode: %d | useEncLfEnable: %d | width: %d\n",
                    attr.supportOCSRAM, instance->codecMode, info.secAxiInfo.u.wave.useEncLfEnable, width);
                return RETCODE_INVALID_PARAM;
            }
        }
        if (info.openParam.subFrameSyncEnable == TRUE) {
            if (param->srcIdx > 5) {
                VLOG(ERR, "subFrameSyncEnable: %d, srcIdx: %d\n", info.openParam.subFrameSyncEnable, param->srcIdx);
                return RETCODE_INVALID_PARAM;
            }
            if (info.rotationEnable == TRUE) {
                VLOG(ERR, "SubFrameSync not support rotate\n");
                return RETCODE_INVALID_PARAM;
            }
            if (info.mirrorEnable == TRUE) {
                VLOG(ERR, "SubFrameSync not support mirror\n");
                return RETCODE_INVALID_PARAM;
            }
        }
    }

    return RETCODE_SUCCESS;
}


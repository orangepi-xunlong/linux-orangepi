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

#ifndef __VPUAPI_PRODUCT_ABSTRACT_H__
#define __VPUAPI_PRODUCT_ABSTRACT_H__

#include "vpuapifunc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/************************************************************************/
/* COMMON                                                               */
/************************************************************************/
Uint32 ProductVpuScan(
    Uint32 coreIdx
    );

Int32 ProductVpuGetId(
    Uint32 coreIdx
    );

Int32 ProductVpuGetId(
    Uint32 coreIdx
    );

RetCode ProductVpuGetVersion(
    Uint32  coreIdx,
    Uint32* versionInfo,
    Uint32* revision
    );

RetCode ProductVpuGetProductInfo(
    Uint32  coreIdx,
    VpuAttr* productInfo
    );

RetCode ProductVpuInit(
    Uint32 coreIdx,
    void*  firmware,
    Uint32 size
    );

RetCode ProductVpuReInit(
    Uint32 coreIdx,
    void*  firmware,
    Uint32 size
    );

Uint32 ProductVpuIsInit(
    Uint32 coreIdx
    );

Int32 ProductVpuIsBusy(
    Uint32 coreIdx
    );

Int32 ProductVpuWaitInterrupt(
    CodecInst*  instance,
    Int32       timeout
    );

RetCode ProductVpuClearInterrupt(
    Uint32      coreIdx,
    Uint32      flags
    );

RetCode ProductVpuSetBusTransaction(
    Uint32 coreIdx,
    BOOL   enable
    );

RetCode ProductVpuReset(
    Uint32      coreIdx,
    SWResetMode resetMode
    );

RetCode ProductVpuSleepWake(
    Uint32 coreIdx,
    int iSleepWake,
     const Uint16* code,
     Uint32 size
    );

RetCode ProductVpuDecAllocateFramebuffer(
    CodecInst*          instance,
    FrameBuffer*        fbArr,
    TiledMapType        mapType,
    Int32               num,
    Int32               stride,
    Int32               height,
    FrameBufferFormat   format,
    BOOL                cbcrInterleave,
    BOOL                nv21,
    Int32               endian,
    vpu_buffer_t*       vb,
    Int32               gdiIndex,
    FramebufferAllocType fbType
    );

RetCode ProductVpuDecGetAuxBufSize(
    CodecInst* inst,
    DecAuxBufferSizeInfo sizeInfo,
    Uint32* size
    );

RetCode ProductVpuDecRegisterAuxBuffer(
    CodecInst* inst,
    AuxBufferInfo auxBufferInfo
    );

Int32 ProductCalculateFrameBufStride(
    CodecInst*          inst,
    Int32               width,
    Int32               height,
    TiledMapType        mapType,
    FrameBufferFormat   format,
    BOOL                interleave
    );

Int32 ProductCalculateFrameBufSize(
    CodecInst*          inst,
    Int32               productId,
    Int32               stride,
    Int32               height,
    TiledMapType        mapType,
    FrameBufferFormat   format,
    BOOL                interleave,
    DRAMConfig*         pDramCfg
    );

RetCode ProductVpuGetBandwidth(
    CodecInst* instance,
    VPUBWData* data
    );


RetCode ProductVpuGetDebugInfo(
    CodecInst* instance,
    VpuDebugInfo* info
    );


/************************************************************************/
/* DECODER                                                              */
/************************************************************************/
RetCode ProductVpuDecBuildUpOpenParam(
    CodecInst*    instance,
    DecOpenParam* param
    );

RetCode ProductVpuDecCheckOpenParam(
    DecOpenParam* param
    );

RetCode ProductVpuDecInitSeq(
    CodecInst*  instance
    );

RetCode ProductVpuDecFiniSeq(
    CodecInst*  instance
    );

RetCode ProductVpuDecSetBitstreamFlag(
    CodecInst*  instance,
    BOOL        running,
    Int32       size
    );

RetCode ProductVpuDecSetRdPtr(
    CodecInst*      instance,
    PhysicalAddress rdPtr
    );

RetCode ProductVpuDecGetSeqInfo(
    CodecInst*      instance,
    DecInitialInfo* info
    );

RetCode ProductVpuDecCheckCapability(
    CodecInst*  instance
    );

RetCode ProductVpuDecode(
    CodecInst*  instance,
    DecParam*   option
    );

RetCode ProductVpuDecGetResult(
    CodecInst*      instance,
    DecOutputInfo*  result
    );

RetCode ProductVpuDecFlush(
    CodecInst*          instance,
    FramebufferIndex*   retIndexes,
    Uint32              size
    );

RetCode ProductVpuDecUpdateFrameBuffer(
    CodecInst*   instance,
    FrameBuffer* fbcFb,
    FrameBuffer* linearFb,
    Uint32       mvColIndex,
    Uint32       picWidth,
    Uint32       picHeight
    );

RetCode ProductVpuDecClrDispFlag(
    CodecInst* instance,
    Uint32 index
    );

RetCode ProductVpuDecSetDispFlag(
    CodecInst* instance,
    Uint32     dispFlag
    );

PhysicalAddress ProductVpuDecGetRdPtr(
    CodecInst* instance
    );

RetCode ProductVpuDecRegisterFramebuffer(
    CodecInst* instance
    );

RetCode ProductVpuDecGiveCommand(
    CodecInst*   instance,
    CodecCommand cmd,
    void*        param);

/************************************************************************/
/* ENCODER                                                              */
/************************************************************************/
RetCode ProductVpuEncUpdateBitstreamBuffer(
    CodecInst* instance
    );

RetCode ProductVpuEncGetRdWrPtr(
    CodecInst* instance,
    PhysicalAddress* rdPtr,
    PhysicalAddress* wrPtr
    );

RetCode ProductVpuEncBuildUpOpenParam(
    CodecInst*      pCodec,
    EncOpenParam*   param
    );

RetCode ProductVpuEncFiniSeq(
    CodecInst*      instance
    );

RetCode ProductVpuEncCheckOpenParam(
    EncOpenParam*   param
    );

RetCode ProductVpuEncGetHeader(
    CodecInst* instance,
    EncHeaderParam* encHeaderParam
    );

RetCode ProductVpuEncSetup(
    CodecInst* instance
    );

RetCode ProductVpuEncCheckEncParam(
    CodecInst* instance,
    EncParam*  param
    );

RetCode ProductVpuEncode(
    CodecInst*      instance,
    EncParam*       param
    );

RetCode ProductVpuEncGetResult(
    CodecInst*      instance,
    EncOutputInfo*  result
    );

RetCode ProductVpuEncGiveCommand(
    CodecInst* instance,
    CodecCommand cmd,
    void* param);

RetCode ProductVpuEncInitSeq(
    CodecInst*  instance
    );

RetCode ProductVpuEncGetSeqInfo(
    CodecInst* instance,
    EncInitialInfo* info
    );

RetCode ProductVpuEncChangeParam(
    CodecInst* instance,
    void* param
    );

RetCode ProductVpuEncSetPeriod(
    CodecInst* instance,
    void* param
    );

RetCode ProductVpuEncGetSrcBufFlag(
    CodecInst* instance,
    Uint32* data
    );

RetCode ProductVpuEncRegisterFramebuffer(
    CodecInst*      instance
    );

RetCode ProductVpuEncAllocateFramebuffer(
    CodecInst*          instance,
    FrameBuffer*        fbArr,
    TiledMapType        mapType,
    Int32               num,
    Int32               stride,
    Int32               height,
    FrameBufferFormat   format,
    BOOL                cbcrInterleave,
    BOOL                nv21,
    Int32               endian,
    vpu_buffer_t*       vb,
    Int32               gdiIndex,
    FramebufferAllocType fbType
    );

RetCode ProductVpuEncGetAuxBufSize(
    CodecInst* inst,
    EncAuxBufferSizeInfo sizeInfo,
    Uint32* size
    );

RetCode ProductVpuEncRegisterAuxBuffer(
    CodecInst* inst,
    AuxBufferInfo auxBufferInfo
    );

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __VPUAPI_PRODUCT_ABSTRACT_H__ */


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

#ifndef __CODA9_FUNCTION_H__
#define __CODA9_FUNCTION_H__

#include "vpuapifunc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void Coda9BitIssueCommand(
    Uint32      coreIdx,
    CodecInst*  inst,
    int         cmd
    );

RetCode Coda9VpuGetVersion(
    Uint32  coreIdx,
    Uint32* versionInfo,
    Uint32* revision
    );

RetCode Coda9VpuInit(
    Uint32   coreIdx,
    void*    firmware,
    Uint32   size
    );

RetCode Coda9VpuReInit(
    Uint32   coreIdx,
    void*    firmware,
    Uint32   size
    );

Uint32 Coda9VpuIsInit(
    Uint32 coreIdx
    );

Int32 Coda9VpuIsBusy(
    Uint32 coreIdx
    );

Int32 Coda9VpuWaitInterrupt(
    CodecInst*  handle,
    Int32       timeout
    );

RetCode Coda9VpuReset(
    Uint32      coreIdx,
    SWResetMode resetMode
    );

RetCode Coda9VpuSleepWake(
    Uint32 coreIdx,
    int iSleepWake,
    const Uint16* code,
    Uint32 size
    );

RetCode Coda9VpuClearInterrupt(
    Uint32  coreIdx
    );

RetCode Coda9VpuFiniSeq(
    CodecInst*  instance
    );

RetCode Coda9VpuBuildUpDecParam(
    CodecInst*      instance,
    DecOpenParam*   param
    );

RetCode Coda9VpuDecInitSeq(
    DecHandle handle
    );

RetCode Coda9VpuDecRegisterFramebuffer(
    CodecInst* instance
    );

RetCode Coda9VpuDecSetBitstreamFlag(
    CodecInst*  instance,
    BOOL        running,
    BOOL        eos
    );

RetCode Coda9VpuDecGetSeqInfo(
    CodecInst*      instance,
    DecInitialInfo* info
    );

RetCode Coda9VpuDecode(
    CodecInst* instance,
    DecParam*  option
    );

RetCode Coda9VpuDecGetResult(
    CodecInst*      instance,
    DecOutputInfo*  result
    );

RetCode Coda9VpuDecFlush(
    CodecInst*          instance,
    FramebufferIndex*   framebufferIndexes,
    Uint32              size
    );

RetCode Coda9VpuDecGiveCommand(
    CodecInst*      instance,
    CodecCommand    cmd,
    void*           param
    );

/************************************************************************/
/* Encoder                                                              */
/************************************************************************/
RetCode Coda9VpuEncRegisterFramebuffer(
    CodecInst*      pCodecInst
    );

RetCode Coda9VpuBuildUpEncParam(
    CodecInst*      pCodecInst,
    EncOpenParam*   param
    );

RetCode Coda9VpuEncSetup(
    CodecInst*      pCodecInst
    );

RetCode Coda9VpuEncParaChange(
    CodecInst*      pCodecInst,
    ParamChange*    param
    );

RetCode Coda9VpuEncode(
    CodecInst*      pCodecInst,
    EncParam*       param
    );

RetCode Coda9VpuEncGetResult(
    CodecInst*      pCodecInst,
    EncOutputInfo*  info
    );

RetCode Coda9VpuEncGetHeader(
    CodecInst*      pCodecInst,
    EncHeaderParam* encHeaderParam
    );

RetCode Coda9VpuEncGiveCommand(
    CodecInst*      pCodecInst,
    CodecCommand    cmd,
    void*           param
    );

RetCode Coda9VpuEncCheckOpenParam(
    EncOpenParam* pop);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CODA9_FUNCTION_H__ */


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

#ifndef __WAVE6_FUNCTION_H__
#define __WAVE6_FUNCTION_H__

#include "vpuapifunc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Uint32 Wave6VpuIsInit(Uint32 core_idx);
Int32 Wave6VpuIsBusy(Uint32 core_idx);
Int32 Wave6VpuWaitInterrupt(CodecInst* inst,
                            Int32 time);
RetCode Wave6VpuClearInterrupt(Uint32 core_idx,
                               Uint32 flags);
RetCode Wave6VpuReset(Uint32 core_idx);
RetCode Wave6VpuSetBusTransaction(Uint32 core_idx,
                                  BOOL enable);
RetCode Wave6VpuInit(Uint32 core_idx,
                     void* fw,
                     Uint32 fw_size);
RetCode Wave6VpuReInit(Uint32 core_idx,
                       void* fw,
                       Uint32 fw_size);
RetCode Wave6VpuGetVersion(Uint32  core_idx,
                           Uint32* version,
                           Uint32* revision);
RetCode Wave6VpuSleepWake(Uint32 core_idx,
                          BOOL is_sleep,
                          const Uint16* code,
                          Uint32 size);
RetCode Wave6VpuGetDebugInfo(CodecInst* inst,
                             VpuDebugInfo* info);

RetCode Wave6VpuBuildUpDecParam(CodecInst* instance,
                                DecOpenParam* param);
RetCode Wave6VpuDecRegisterFramebuffer(CodecInst* inst,
                                       FrameBuffer* fbArr,
                                       TiledMapType mapType,
                                       Uint32 count);
RetCode Wave6VpuDecUpdateFramebuffer(CodecInst*   inst,
                                     FrameBuffer* fbcFb,
                                     FrameBuffer* linearFb,
                                     Int32        mvIndex,
                                     Int32        width,
                                     Int32        height);
RetCode Wave6VpuDecInitSeq(CodecInst* instance);
RetCode Wave6VpuDecGetSeqInfo(CodecInst* instance,
                              DecInitialInfo* info);
RetCode Wave6VpuDecode(CodecInst* instance,
                       DecParam* option);
RetCode Wave6VpuDecGetResult(CodecInst* instance,
                             DecOutputInfo* result);
RetCode Wave6VpuDecFiniSeq(CodecInst* instance);
RetCode Wave6VpuDecFlush(CodecInst* instance,
                         FramebufferIndex* framebufferIndexes,
                         Uint32 size);
RetCode Wave6VpuDecGiveCommand(CodecInst* instance,
                               CodecCommand cmd,
                               void* param);
RetCode Wave6VpuDecGetAuxBufSize(CodecInst* inst,
                                 DecAuxBufferSizeInfo sizeInfo,
                                 Uint32* size);
RetCode Wave6VpuDecRegisterAuxBuffer(CodecInst* inst,
                                     AuxBufferInfo auxBufferInfo);

RetCode Wave6VpuBuildUpEncParam(CodecInst* inst,
                                EncOpenParam* param);
RetCode Wave6VpuEncParaChange(CodecInst* inst,
                              EncWave6ChangeParam* param);
RetCode Wave6VpuEncInitSeq(CodecInst* inst);
RetCode Wave6VpuEncRegisterFramebuffer(CodecInst* inst,
                                       FrameBuffer* fbArr);
RetCode Wave6VpuEncode(CodecInst* inst,
                       EncParam* opt);
RetCode Wave6VpuEncGetHeader(CodecInst* inst,
                             EncHeaderParam* param);
RetCode Wave6VpuEncFiniSeq(CodecInst* inst);
RetCode Wave6VpuEncUpdateBS(CodecInst* inst);
RetCode Wave6VpuEncCheckOpenParam(EncOpenParam open_param);
RetCode Wave6VpuEncGetSeqInfo(CodecInst* inst,
                              EncInitialInfo* ret);
RetCode Wave6VpuEncGetResult(CodecInst* inst,
                             EncOutputInfo* ret);
RetCode Wave6VpuEncGiveCommand(CodecInst* inst,
                               CodecCommand cmd,
                               void* param);
RetCode Wave6VpuEncGetRdWrPtr(CodecInst* inst,
                              PhysicalAddress *rdPtr,
                              PhysicalAddress *wrPtr);
RetCode Wave6VpuEncGetAuxBufSize(CodecInst* inst,
                                 EncAuxBufferSizeInfo sizeInfo,
                                 Uint32* size);
RetCode Wave6VpuEncRegisterAuxBuffer(CodecInst* inst,
                                     AuxBufferInfo auxBufferInfo);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WAVE6_FUNCTION_H__ */


/*************************************************************************/ /*!
@File
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#ifndef TEE_RGX_H
#define TEE_RGX_H

void RGXWriteReg32(const void *hPrivate,
                   IMG_UINT32 ui32RegAddr,
                   IMG_UINT32 ui32RegValue);

void RGXWriteReg64(const void *hPrivate,
                   IMG_UINT32 ui32RegAddr,
                   IMG_UINT64 ui64RegValue);

IMG_UINT32 RGXReadReg32(const void *hPrivate,
                        IMG_UINT32 ui32RegAddr);

IMG_UINT64 RGXReadReg64(const void *hPrivate,
                        IMG_UINT32 ui32RegAddr);

IMG_UINT32 RGXReadModifyWriteReg64(const void *hPrivate,
                                   IMG_UINT32 ui32RegAddr,
                                   IMG_UINT64 ui64RegValue,
                                   IMG_UINT64 ui64RegKeepMask);

PVRSRV_ERROR RGXPollReg32(const void *hPrivate,
                          IMG_UINT32 ui32RegAddr,
                          IMG_UINT32 ui32RegValue,
                          IMG_UINT32 ui32RegMask);

PVRSRV_ERROR RGXPollReg64(const void *hPrivate,
                          IMG_UINT32 ui32RegAddr,
                          IMG_UINT64 ui64RegValue,
                          IMG_UINT64 ui64RegMask);

PVRSRV_ERROR RGXWriteMetaRegThroughSP(const void *hPrivate,
                                      IMG_UINT32 ui32RegAddr,
                                      IMG_UINT32 ui32RegValue);

PVRSRV_ERROR RGXReadMetaRegThroughSP(const void *hPrivate,
                                     IMG_UINT32 ui32RegAddr,
                                     IMG_UINT32* ui32RegValue);

void RGXSetPoweredState(const void *hPrivate, IMG_BOOL bPowered);
void RGXWaitCycles(const void *hPrivate,
                   IMG_UINT32 ui32Cycles,
                   IMG_UINT32 ui32WaitUs);

void RGXAcquireKernelMMUPC(const void *hPrivate, IMG_DEV_PHYADDR *psPCAddr);
IMG_BOOL RGXDoFWSlaveBoot(const void *hPrivate);
PVRSRV_ERROR RGXFabricCoherencyTest(const void *hPrivate);
IMG_UINT32 RGXGetDeviceSLCBanks(const void *hPrivate);
IMG_UINT32 RGXGetDeviceCacheLineSize(const void *hPrivate);
void RGXAcquireBootCodeAddr(const void *hPrivate, IMG_DEV_VIRTADDR *psBootCodeAddr);
void *RGXCalculateHostFWDataAddress(const void *hPrivate, void *pvHostFWDataAddr);
void RGXAcquireBootDataAddr(const void *hPrivate, IMG_DEV_VIRTADDR *psBootDataAddr);
IMG_BOOL RGXDeviceAckIrq(const void *hPrivate);
IMG_UINT64 RGXMMUInitRangeValue(IMG_UINT32 ui32MMURange);

__printf(2, 3) void RGXCommentLog(const void *hPrivate, const IMG_CHAR *pszString, ...);
__printf(2, 3) void RGXErrorLog(const void *hPrivate, const IMG_CHAR *pszString, ...);


#endif

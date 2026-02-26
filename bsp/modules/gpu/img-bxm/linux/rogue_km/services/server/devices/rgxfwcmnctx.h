/*************************************************************************/ /*!
@File
@Title          RGX firmware common context utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX firmware common context utility routines
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

#ifndef RGXFWCMNCTX_H
#define RGXFWCMNCTX_H

#include "connection_server.h"
#include "device.h"
#include "rgxccb.h"
#include "rgx_common.h"
#include "devicemem_typedefs.h"
#include "rgxdevice.h"
#include "rgxmem.h"

/*************************************************************************/ /*!
@Function       FWCommonContextAllocate

@Description    Allocate a FW common context. This allocates the HW memory
                for the context, the CCB and wires it all together.

@Input          psConnection            Connection this context is being created on
@Input          psDeviceNode            Device node to create the FW context on
                                        (must be RGX device node)
@Input          eRGXCCBRequestor        RGX_CCB_REQUESTOR_TYPE enum constant which
                                        represents the requestor of this FWCC
@Input          eDM                     Data Master type
@Input          psServerMMUContext      Server MMU memory context.
@Input          psAllocatedMemDesc      Pointer to pre-allocated MemDesc to use
                                        as the FW context or NULL if this function
                                        should allocate it
@Input          ui32AllocatedOffset     Offset into pre-allocate MemDesc to use
                                        as the FW context. If psAllocatedMemDesc
                                        is NULL then this parameter is ignored
@Input          psFWMemContextMemDesc   MemDesc of the FW memory context this
                                        common context resides on
@Input          psContextStateMemDesc   FW context state (context switch) MemDesc
@Input          ui32CCBAllocSizeLog2    Size of the CCB for this context
@Input          ui32CCBMaxAllocSizeLog2 Maximum size to which CCB can grow for this context
@Input          ui32ContextFlags        Flags which specify properties of the context
@Input          i32Priority             Priority of the context
@Input          ui32MaxDeadlineMS       Max deadline limit in MS that the workload can run
@Input          ui64RobustnessAddress   Address for FW to signal a context reset
@Input          psInfo                  Structure that contains extra info
                                        required for the creation of the context
                                        (elements might change from core to core)
@Return         PVRSRV_OK if the context was successfully created
*/ /**************************************************************************/
PVRSRV_ERROR FWCommonContextAllocate(CONNECTION_DATA *psConnection,
									 PVRSRV_DEVICE_NODE *psDeviceNode,
									 RGX_CCB_REQUESTOR_TYPE eRGXCCBRequestor,
									 RGXFWIF_DM eDM,
									 SERVER_MMU_CONTEXT *psServerMMUContext,
									 DEVMEM_MEMDESC *psAllocatedMemDesc,
									 IMG_UINT32 ui32AllocatedOffset,
									 DEVMEM_MEMDESC *psFWMemContextMemDesc,
									 DEVMEM_MEMDESC *psContextStateMemDesc,
									 IMG_UINT32 ui32CCBAllocSizeLog2,
									 IMG_UINT32 ui32CCBMaxAllocSizeLog2,
									 IMG_UINT32 ui32ContextFlags,
									 IMG_INT32 i32Priority,
									 IMG_UINT32 ui32MaxDeadlineMS,
									 IMG_UINT64 ui64RobustnessAddress,
									 RGX_COMMON_CONTEXT_INFO *psInfo,
									 RGX_SERVER_COMMON_CONTEXT **ppsServerCommonContext);


void FWCommonContextFree(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext);

PRGXFWIF_FWCOMMONCONTEXT FWCommonContextGetFWAddress(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext);

RGX_CLIENT_CCB *FWCommonContextGetClientCCB(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext);

SERVER_MMU_CONTEXT *FWCommonContextGetServerMMUCtx(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext);

RGX_CONTEXT_RESET_REASON FWCommonContextGetLastResetReason(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext,
                                                           IMG_UINT32 *pui32LastResetJobRef);

PVRSRV_RGXDEV_INFO* FWCommonContextGetRGXDevInfo(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext);

PVRSRV_ERROR RGXGetFWCommonContextAddrFromServerMMUCtx(PVRSRV_RGXDEV_INFO *psDevInfo,
													   SERVER_MMU_CONTEXT *psServerMMUContext,
													   PRGXFWIF_FWCOMMONCONTEXT *psFWCommonContextFWAddr);

PRGXFWIF_FWCOMMONCONTEXT RGXGetFWCommonContextAddrFromServerCommonCtx(PVRSRV_RGXDEV_INFO *psDevInfo,
													                  DLLIST_NODE *psNode);

PVRSRV_ERROR FWCommonContextSetFlags(RGX_SERVER_COMMON_CONTEXT *psServerCommonContext,
                                     IMG_UINT32 ui32ContextFlags);

PVRSRV_ERROR ContextSetPriority(RGX_SERVER_COMMON_CONTEXT *psContext,
								CONNECTION_DATA *psConnection,
								PVRSRV_RGXDEV_INFO *psDevInfo,
								IMG_INT32 i32Priority,
								RGXFWIF_DM eDM);

PVRSRV_ERROR CheckStalledClientCommonContext(RGX_SERVER_COMMON_CONTEXT *psCurrentServerCommonContext, RGX_KICK_TYPE_DM eKickTypeDM);

void DumpFWCommonContextInfo(RGX_SERVER_COMMON_CONTEXT *psCurrentServerCommonContext,
                             DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                             void *pvDumpDebugFile,
                             IMG_UINT32 ui32VerbLevel);

void FWCommonContextListSetLastResetReason(PVRSRV_RGXDEV_INFO *psDevInfo,
                                           IMG_UINT32 *pui32ErrorPid,
                                           const RGXFWIF_FWCCB_CMD_CONTEXT_RESET_DATA *psCmdContextResetNotification);

#endif /* RGXFWCMNCTX_H */
/******************************************************************************
 End of file (rgxfwcmnctx.h)
******************************************************************************/

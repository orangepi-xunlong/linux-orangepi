/*************************************************************************/ /*!
@File
@Title          RGX firmware Memctx routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header for operations on FWKM Shared memory context.
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

#ifndef RGXFWMEMCTX_H
#define RGXFWMEMCTX_H

#include "device.h"
#include "rgx_memallocflags.h"
#include "pvrsrv.h"
#include "cache_ops.h"
#include "cache_km.h"
#include "pvr_debug.h"

#if defined(CONFIG_ARM64) && defined(__linux__) && defined(SUPPORT_CPUCACHED_FWMEMCTX)
/*
 * RGXFwSharedMemCPUCacheMode()
 * We upgrade allocations on ARM64 Linux when CPU Cache snooping is enabled.
 * This is because of the Linux direct mapping causing interference due to PIPT
 * cache. All allocations are normally UCWC but snooping can return a bad value from the
 * direct mapping as it is cached. Upgrade our allocations to cached to prevent bad cached
 * values but in turn we require flushing.
 */
static INLINE void RGXFwSharedMemCPUCacheMode(PVRSRV_DEVICE_NODE *psDeviceNode,
                                              PVRSRV_MEMALLOCFLAGS_T *puiFlags)
{
	if ((*puiFlags & (PVRSRV_MEMALLOCFLAG_CPU_READABLE |
	                  PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
	                  PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE)) == 0)
	{
		/* We don't need to upgrade if we don't map into the CPU */
		return;
	}

	/* Clear the existing CPU cache flags */
	*puiFlags &= ~(PVRSRV_MEMALLOCFLAG_CPU_CACHE_MODE_MASK);

	if (PVRSRVSystemSnoopingOfCPUCache(psDeviceNode->psDevConfig))
	{
		*puiFlags |= PVRSRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT;
	}
	else
	{
		*puiFlags |= PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC;
	}
}

#define RGXFwSharedMemCheckSnoopMode(psDeviceConfig) PVR_ASSERT(PVRSRVSystemSnoopingOfCPUCache(psDeviceConfig))

/*
 * FWSharedMemCacheOpExec()
 * This is the CPU data-cache maintenance interface for FW shared allocations.
 * We have to be very careful that the VAs supplied to this function are
 * sensible as to not cause a kernel oops. Given that this should only be
 * used for allocations used for the FW this should be guaranteed.
 */
static INLINE PVRSRV_ERROR RGXFwSharedMemCacheOpExec(const volatile void *pvVirtStart,
                                                     IMG_UINT64 uiSize,
                                                     PVRSRV_CACHE_OP uiCacheOp)
{
	IMG_UINT64 uiEndAddr = (IMG_UINT64) pvVirtStart + uiSize;
	IMG_CPU_PHYADDR uiUnusedPhysAddr = {.uiAddr = 0};

	if (!pvVirtStart || uiSize == 0)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return CacheOpExec(NULL,
					   (void*) pvVirtStart,
					   (void*) uiEndAddr,
					   uiUnusedPhysAddr,
					   uiUnusedPhysAddr,
					   uiCacheOp);
}

#define RGXFwSharedMemCacheOpValue(value, cacheop) (RGXFwSharedMemCacheOpExec(&value, sizeof(value), PVRSRV_CACHE_OP_##cacheop))
#define RGXFwSharedMemCacheOpPtr(ptr, cacheop) (RGXFwSharedMemCacheOpExec(ptr, sizeof(*ptr), PVRSRV_CACHE_OP_##cacheop))
#define RGXFwSharedMemCacheOpExecPfn RGXFwSharedMemCacheOpExec

static INLINE void RGXFwSharedMemFlushCCB(void *pvCCBVirtAddr,
                                          IMG_UINT64 uiStart,
                                          IMG_UINT64 uiFinish,
                                          IMG_UINT64 uiLimit)
{
	if (uiFinish >= uiStart)
	{
		/* Flush the CCB data */
		RGXFwSharedMemCacheOpExec(IMG_OFFSET_ADDR(pvCCBVirtAddr, uiStart),
		                          uiFinish - uiStart,
		                          PVRSRV_CACHE_OP_FLUSH);
	}
	else
	{
		/* CCCB wrapped around - flush the pre and post wrap boundary separately */
		RGXFwSharedMemCacheOpExec(IMG_OFFSET_ADDR(pvCCBVirtAddr, uiStart),
		                          uiLimit - uiStart,
		                          PVRSRV_CACHE_OP_FLUSH);

		RGXFwSharedMemCacheOpExec(IMG_OFFSET_ADDR(pvCCBVirtAddr, 0),
		                          uiFinish,
		                          PVRSRV_CACHE_OP_FLUSH);
	}
}
#else
#define RGXFwSharedMemCPUCacheMode(...)
#define RGXFwSharedMemCheckSnoopMode(...)
/* NULL value required for function callbacks */
#define RGXFwSharedMemCacheOpExec(...) ((void)NULL)
#define RGXFwSharedMemCacheOpValue(...) ((void)NULL)
#define RGXFwSharedMemCacheOpPtr(...) ((void)NULL)
#define RGXFwSharedMemCacheOpExecPfn NULL
#define RGXFwSharedMemFlushCCB(...)
#endif


#endif /* RGXFWMEMCTX_H */

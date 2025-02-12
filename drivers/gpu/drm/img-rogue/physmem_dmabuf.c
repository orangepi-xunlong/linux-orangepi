/*************************************************************************/ /*!
@File           physmem_dmabuf.c
@Title          dmabuf memory allocator
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of the memory management. This module is responsible for
                implementing the function callbacks for dmabuf memory.
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

#include <linux/version.h>

#include "physmem_dmabuf.h"
#include "physmem_dmabuf_internal.h"
#include "physmem.h"
#include "pvrsrv.h"
#include "pmr.h"

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/scatterlist.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
#include <drm/drm_prime.h>
#else
#include <drm/drmP.h>
#endif
#include <drm/drm_gem.h>

#include "img_types.h"
#include "img_defs.h"
#include "pvr_debug.h"
#include "pvrsrv_error.h"
#include "pvrsrv_memallocflags.h"

#include "allocmem.h"
#include "osfunc.h"
#include "pmr_impl.h"
#include "pmr_env.h"
#include "hash.h"
#include "private_data.h"
#include "module_common.h"
#include "pvr_ion_stats.h"
#include "cache_km.h"

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
#include "ri_server.h"
#endif

#if defined(PVRSRV_ENABLE_LINUX_MMAP_STATS)
#include "mmap_stats.h"
#endif

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#include "process_stats.h"
#endif

#include "kernel_compatibility.h"

typedef struct _PMR_DMA_BUF_WRAPPER_
{
	PMR *psPMR;

	/*
	 * Set if the PMR has been exported via GEM. This field should only
	 * be accessed using smp_store_release and smp_load_acquire.
	 * In general, the field is not safe to access, as the wrapper doesn't
	 * hold a reference on the GEM object.
	 */
	struct drm_gem_object *psObj;

	struct dma_resv sDmaResv;

	/* kernel mapping */
	IMG_UINT32 uiKernelMappingRefCnt;
	IMG_HANDLE hKernelMappingHandle;
	void *pvKernelMappingAddr;
	PMR_SIZE_T uiKernelMappingLen;
	POS_LOCK hKernelMappingLock;

	/* device mapping */
	IMG_UINT32 uiDeviceMappingRefCnt;
	POS_LOCK hDeviceMappingLock;
	struct sg_table *psTable;
} PMR_DMA_BUF_WRAPPER;

typedef struct _PMR_DMA_BUF_GEM_OBJ {
	struct drm_gem_object sBase;
	PMR_DMA_BUF_WRAPPER *psPMRWrapper;
} PMR_DMA_BUF_GEM_OBJ;

#define TO_PMR_DMA_BUF_GEM_OBJ(psObj) IMG_CONTAINER_OF((psObj), PMR_DMA_BUF_GEM_OBJ, sBase)

static PVRSRV_ERROR _PMRKMap(PMR_DMA_BUF_WRAPPER *psPMRWrapper,
                             size_t uiSize,
                             void **pvAddr)
{
	IMG_HANDLE hKernelMapping;
	void *pvAddrOut = NULL;
	size_t uiLengthOut = 0;
	PVRSRV_ERROR eError = PVRSRV_OK;

	OSLockAcquire(psPMRWrapper->hKernelMappingLock);

	if (psPMRWrapper->uiKernelMappingRefCnt++ == 0)
	{
		PVR_ASSERT(psPMRWrapper->hKernelMappingHandle == NULL);
		PVR_ASSERT(psPMRWrapper->pvKernelMappingAddr == NULL);

		if (PMR_IsSparse(psPMRWrapper->psPMR))
		{
			eError = PMRAcquireSparseKernelMappingData(psPMRWrapper->psPMR,
			                                           0,
			                                           uiSize,
			                                           &pvAddrOut,
			                                           &uiLengthOut,
			                                           &hKernelMapping);
			PVR_LOG_GOTO_IF_ERROR(eError, "PMRAcquireSparseKernelMappingData",
			                      ErrReleaseLock);
		}
		else
		{
			eError = PMRAcquireKernelMappingData(psPMRWrapper->psPMR,
			                                     0,
			                                     uiSize,
			                                     &pvAddrOut,
			                                     &uiLengthOut,
			                                     &hKernelMapping);
			PVR_LOG_GOTO_IF_ERROR(eError, "PMRAcquireKernelMappingData",
			                      ErrReleaseLock);
		}

		psPMRWrapper->hKernelMappingHandle = hKernelMapping;
		psPMRWrapper->pvKernelMappingAddr = pvAddrOut;
		psPMRWrapper->uiKernelMappingLen = uiLengthOut;
	}

	*pvAddr = psPMRWrapper->pvKernelMappingAddr;
	goto ExitUnlock;

ErrReleaseLock:
	psPMRWrapper->uiKernelMappingRefCnt--;

ExitUnlock:
	OSLockRelease(psPMRWrapper->hKernelMappingLock);
	return eError;
}

static void _PMRKUnmap(PMR_DMA_BUF_WRAPPER *psPMRWrapper)
{
	OSLockAcquire(psPMRWrapper->hKernelMappingLock);

	if (--psPMRWrapper->uiKernelMappingRefCnt == 0)
	{
		PVRSRV_ERROR eError;

		PVR_ASSERT(psPMRWrapper->hKernelMappingHandle != NULL);
		PVR_ASSERT(psPMRWrapper->pvKernelMappingAddr != NULL);

		eError = PMRReleaseKernelMappingData(psPMRWrapper->psPMR,
		                                     psPMRWrapper->hKernelMappingHandle);
		PVR_LOG_IF_ERROR(eError, "PMRReleaseKernelMappingData");

		psPMRWrapper->hKernelMappingHandle = NULL;
		psPMRWrapper->pvKernelMappingAddr = NULL;
		psPMRWrapper->uiKernelMappingLen = 0;
	}

	OSLockRelease(psPMRWrapper->hKernelMappingLock);
}

static PVRSRV_ERROR _PMRInvalidateCache(PMR_DMA_BUF_WRAPPER *psPMRWrapper)
{
	PVRSRV_ERROR eError;

	OSLockAcquire(psPMRWrapper->hKernelMappingLock);

	if (psPMRWrapper->uiKernelMappingRefCnt == 0) {
		OSLockRelease(psPMRWrapper->hKernelMappingLock);
		return PVRSRV_ERROR_PMR_NO_KERNEL_MAPPING;
	}

	eError = CacheOpValExec(psPMRWrapper->psPMR,
	                        (IMG_UINT64) (uintptr_t) psPMRWrapper->pvKernelMappingAddr,
	                        0,
	                        psPMRWrapper->uiKernelMappingLen,
	                        PVRSRV_CACHE_OP_INVALIDATE);
	PVR_LOG_IF_ERROR(eError, "CacheOpValExec");

	OSLockRelease(psPMRWrapper->hKernelMappingLock);

	return eError;
}

static PVRSRV_ERROR _PMRCleanCache(PMR_DMA_BUF_WRAPPER *psPMRWrapper)
{
	PVRSRV_ERROR eError;

	OSLockAcquire(psPMRWrapper->hKernelMappingLock);

	if (psPMRWrapper->uiKernelMappingRefCnt == 0) {
		OSLockRelease(psPMRWrapper->hKernelMappingLock);
		return PVRSRV_ERROR_PMR_NO_KERNEL_MAPPING;
	}

	eError = CacheOpValExec(psPMRWrapper->psPMR,
	                        (IMG_UINT64) (uintptr_t) psPMRWrapper->pvKernelMappingAddr,
	                        0,
	                        psPMRWrapper->uiKernelMappingLen,
	                        PVRSRV_CACHE_OP_FLUSH);
	PVR_LOG_IF_ERROR(eError, "CacheOpValExec");

	OSLockRelease(psPMRWrapper->hKernelMappingLock);

	return eError;
}

/*
 * dma_buf_ops common code
 *
 * Implementation of below callbacks adds the ability to export DmaBufs to other
 * drivers.
 * The following common functions are used by both the dma_buf and GEM
 * callbacks.
 */

static int PVRDmaBufOpsAttachCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper,
							  struct dma_buf_attachment *psAttachment)
{
	PMR *psPMR = psPMRWrapper->psPMR;

	if (PMR_GetType(psPMR) == PMR_TYPE_DMABUF)
	{
		// don't support exporting PMRs that are itself created from imported
		// DmaBufs
		PVR_DPF((PVR_DBG_ERROR, "exporting PMRs of type DMABUF not supported"));

		return -ENOTSUPP;
	}

	return 0;
}

static struct sg_table *PVRDmaBufOpsMapCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper,
                                              struct dma_buf_attachment *psAttachment,
                                              enum dma_data_direction eDirection)
{
	PVRSRV_ERROR eError;
	PMR *psPMR = psPMRWrapper->psPMR;
	IMG_UINT uiNents;
	IMG_DEVMEM_SIZE_T uiPhysSize, uiVirtSize;
	IMG_UINT32 uiNumVirtPages;
	void *pvPAddrData = NULL;
	IMG_DEV_PHYADDR asPAddr[PMR_MAX_TRANSLATION_STACK_ALLOC], *psPAddr = asPAddr;
	IMG_BOOL abValid[PMR_MAX_TRANSLATION_STACK_ALLOC], *pbValid = abValid;
	IMG_UINT32 i;
	IMG_DEV_PHYADDR sPAddrPrev, sPAddrCurr;
	struct sg_table *psTable;
	struct scatterlist *psSg;
	int iRet = 0;
	IMG_UINT32 uiDevPageShift, uiDevPageSize;

	OSLockAcquire(psPMRWrapper->hDeviceMappingLock);

	psPMRWrapper->uiDeviceMappingRefCnt++;

	if (psPMRWrapper->uiDeviceMappingRefCnt > 1)
	{
		goto OkUnlock;
	}

	PVR_ASSERT(psPMRWrapper->psTable == NULL);

	uiDevPageShift = PMR_GetLog2Contiguity(psPMR);
	uiDevPageSize = 1u << uiDevPageShift;
	uiVirtSize = PMR_LogicalSize(psPMR);
	uiPhysSize = PMR_PhysicalSize(psPMR);
	if (uiPhysSize == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "invalid PMR size"));
		iRet = PVRSRVToNativeError(PVRSRV_ERROR_BAD_MAPPING);
		goto ErrUnlockMapping;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "%s(): mapping pmr: 0x%p@0x%" IMG_UINT64_FMTSPECx
	         " from heap: \"%s\" with page size: 0x%x", __func__, psPMR,
	         uiPhysSize, PhysHeapName(PMR_PhysHeap(psPMR)), uiDevPageSize));

	uiNumVirtPages = uiVirtSize >> uiDevPageShift;

	eError = PMRLockSysPhysAddresses(psPMR);
	if (eError != PVRSRV_OK)
	{
		PVR_LOG_IF_ERROR(eError, "PMRLockSysPhysAddresses");
		iRet = PVRSRVToNativeError(eError);
		goto ErrUnlockMapping;
	}

	PMR_SetLayoutFixed(psPMR, IMG_TRUE);

	psTable = OSAllocZMem(sizeof(*psTable));
	if (psTable == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSAllocMem.1() failed"));
		iRet = -ENOMEM;
		goto ErrUnlockPhysAddresses;
	}

	if (uiNumVirtPages > PMR_MAX_TRANSLATION_STACK_ALLOC)
	{
		pvPAddrData = OSAllocMem(uiNumVirtPages * (sizeof(*psPAddr) + sizeof(*pbValid)));
		if (pvPAddrData == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "OSAllocMem.2() failed"));
			iRet = -ENOMEM;
			goto ErrFreeTable;
		}

		psPAddr = IMG_OFFSET_ADDR(pvPAddrData, 0);
		pbValid = IMG_OFFSET_ADDR(pvPAddrData, uiNumVirtPages * sizeof(*psPAddr));
	}

	eError = PMR_DevPhysAddr(psPMR,
	                         uiDevPageShift,
	                         uiNumVirtPages,
	                         0,
	                         psPAddr,
	                         pbValid,
	                         DEVICE_USE);
	if (eError != PVRSRV_OK)
	{
		PVR_LOG_ERROR(eError, "PMR_DevPhysAddr");
		iRet = PVRSRVToNativeError(eError);
		goto ErrFreePAddrData;
	}

	/* Calculate how many contiguous regions there are in the PMR. This
	 * value will be used to allocate one scatter list for every region. */

	/* Find first valid physical address. */
	for (i = 0; i < uiNumVirtPages && !pbValid[i]; i++);

	sPAddrPrev = psPAddr[i];
	uiNents = 1;

	PVR_DPF((PVR_DBG_MESSAGE, "%s(): %03u paddr: 0x%" IMG_UINT64_FMTSPECx,
	         __func__, i, sPAddrPrev.uiAddr));

	/* Find the rest of the addresses. */
	for (i = i + 1; i < uiNumVirtPages; i++)
	{
		if (!pbValid[i])
		{
			continue;
		}

		sPAddrCurr = psPAddr[i];

		PVR_DPF((PVR_DBG_MESSAGE, "%s(): %03u paddr: 0x%" IMG_UINT64_FMTSPECx
		         ", pprev: 0x%" IMG_UINT64_FMTSPECx ", valid: %u", __func__,
		         i, sPAddrCurr.uiAddr, sPAddrPrev.uiAddr, pbValid[i]));

		if (sPAddrCurr.uiAddr != (sPAddrPrev.uiAddr + uiDevPageSize))
		{
			uiNents++;
		}

		sPAddrPrev = sPAddrCurr;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "%s(): found %u contiguous regions", __func__,
	         uiNents));

	/* Allocate scatter lists from all of the contiguous regions. */

	iRet = sg_alloc_table(psTable, uiNents, GFP_KERNEL);
	if (iRet != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "sg_alloc_table() failed with error %d", iRet));
		goto ErrFreePAddrData;
	}

	/* Fill in all of the physical addresses and sizes for all of the
	 * contiguous regions that were calculated. */

	for (i = 0; i < uiNumVirtPages && !pbValid[i]; i++);

	psSg = psTable->sgl;
	sPAddrPrev = psPAddr[i];

	sg_dma_address(psSg) = sPAddrPrev.uiAddr;
	sg_dma_len(psSg) = uiDevPageSize;

	for (i = i + 1; i < uiNumVirtPages; i++)
	{
		if (!pbValid[i])
		{
			continue;
		}

		sPAddrCurr = psPAddr[i];

		if (sPAddrCurr.uiAddr != (sPAddrPrev.uiAddr + uiDevPageSize))
		{
			psSg = sg_next(psSg);
			PVR_ASSERT(psSg != NULL);

			sg_dma_address(psSg) = sPAddrCurr.uiAddr;
			sg_dma_len(psSg) = uiDevPageSize;
		}
		else
		{
			sg_dma_len(psSg) += uiDevPageSize;
		}

		sPAddrPrev = sPAddrCurr;
	}

	if (pvPAddrData != NULL)
	{
		OSFreeMem(pvPAddrData);
	}

	psPMRWrapper->psTable = psTable;

OkUnlock:
	OSLockRelease(psPMRWrapper->hDeviceMappingLock);

	return psPMRWrapper->psTable;

ErrFreePAddrData:
	if (pvPAddrData != NULL)
	{
		OSFreeMem(pvPAddrData);
	}
ErrFreeTable:
	OSFreeMem(psTable);
ErrUnlockPhysAddresses:
	{
		PVRSRV_ERROR eError2 = PMRUnlockSysPhysAddresses(psPMR);
		PVR_LOG_IF_ERROR(eError2, "PMRUnlockSysPhysAddresses");
	}
ErrUnlockMapping:
	psPMRWrapper->uiDeviceMappingRefCnt--;
	OSLockRelease(psPMRWrapper->hDeviceMappingLock);

	return ERR_PTR(iRet);
}

static void PVRDmaBufOpsUnmapCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper,
                              struct dma_buf_attachment *psAttachment,
                              struct sg_table *psTable,
                              enum dma_data_direction eDirection)
{
	PVRSRV_ERROR eError;

	OSLockAcquire(psPMRWrapper->hDeviceMappingLock);

	if (psPMRWrapper->uiDeviceMappingRefCnt == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "reference count on mapping already 0"));
		goto ErrUnlock;
	}

	psPMRWrapper->uiDeviceMappingRefCnt--;

	if (psPMRWrapper->uiDeviceMappingRefCnt > 0)
	{
		goto ErrUnlock;
	}

	dma_unmap_sg(psAttachment->dev, psTable->sgl, psTable->nents, eDirection);
	sg_free_table(psTable);

	eError = PMRUnlockSysPhysAddresses(psPMRWrapper->psPMR);
	PVR_LOG_IF_ERROR(eError, "PMRUnlockSysPhysAddresses");

	OSFreeMem(psPMRWrapper->psTable);
	psPMRWrapper->psTable = NULL;

ErrUnlock:
	OSLockRelease(psPMRWrapper->hDeviceMappingLock);
}

static int PVRDmaBufOpsBeginCpuAccessCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper,
                                      enum dma_data_direction eDirection)
{
	if (PVRSRV_CHECK_CPU_CACHED(PMR_Flags(psPMRWrapper->psPMR)))
	{
		PVRSRV_ERROR eError = _PMRInvalidateCache(psPMRWrapper);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "_PMRInvalidateCache");
			return OSPVRSRVToNativeError(eError);
		}
	}

	return 0;
}

static int PVRDmaBufOpsEndCpuAccessCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper,
                                    enum dma_data_direction eDirection)
{
	if (PVRSRV_CHECK_CPU_CACHED(PMR_Flags(psPMRWrapper->psPMR)))
	{
		PVRSRV_ERROR eError = _PMRCleanCache(psPMRWrapper);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "_PMRCleanCache");
			return OSPVRSRVToNativeError(eError);
		}
	}

	return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
static void *PVRDmaBufOpsKMapCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper, unsigned long uiPageNum)
{
	return NULL;
}

static void PVRDmaBufOpsKUnMapCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper, unsigned long uiPageNum, void *pvMem)
{
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void *PVRDmaBufOpsVMapCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper)
{
	void *pvAddrOut = NULL;
	PVRSRV_ERROR eError;

	eError = _PMRKMap(psPMRWrapper, 0, &pvAddrOut);

	return eError == PVRSRV_OK ? pvAddrOut : NULL;
}
#else
static int PVRDmaBufOpsVMapCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper, struct iosys_map *psMap)
{
	void *pvAddrOut = NULL;
	PVRSRV_ERROR eError;

	eError = _PMRKMap(psPMRWrapper, 0, &pvAddrOut);

	if (eError != PVRSRV_OK)
	{
		return OSPVRSRVToNativeError(eError);
	}

	if (PhysHeapGetType(PMR_PhysHeap(psPMRWrapper->psPMR)) == PHYS_HEAP_TYPE_UMA)
	{
		iosys_map_set_vaddr(psMap, pvAddrOut);
	}
	else
	{
		iosys_map_set_vaddr_iomem(psMap, pvAddrOut);
	}

	return 0;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void PVRDmaBufOpsVUnMapCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper, void *pvAddr)
{
	_PMRKUnmap(psPMRWrapper);
}
#else
static void PVRDmaBufOpsVUnMapCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper, struct iosys_map *psMap)
{
	_PMRKUnmap(psPMRWrapper);

	iosys_map_clear(psMap);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */

static int PVRDmaBufOpsMMapCommon(PMR_DMA_BUF_WRAPPER *psPMRWrapper, struct vm_area_struct *psVMA)
{
	PMR *psPMR = psPMRWrapper->psPMR;
	PVRSRV_MEMALLOCFLAGS_T uiProtFlags =
	    (BITMASK_HAS(psVMA->vm_flags, VM_READ) ? PVRSRV_MEMALLOCFLAG_CPU_READABLE : 0) |
	    (BITMASK_HAS(psVMA->vm_flags, VM_WRITE) ? PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE : 0);
	PVRSRV_ERROR eError;

	/* Forcibly clear the VM_MAYWRITE flag as this is inherited from the
	 * kernel mmap code and we do not want to produce a potentially writable
	 * mapping from a read-only mapping.
	 */
	if (!BITMASK_HAS(psVMA->vm_flags, VM_WRITE))
	{
		pvr_vm_flags_clear(psVMA, VM_MAYWRITE);
	}

	eError = PMRMMapPMR(psPMR, psVMA, uiProtFlags);
	if (eError != PVRSRV_OK)
	{
		PVR_LOG_IF_ERROR(eError, "PMRMMapPMR");
		return OSPVRSRVToNativeError(eError);
	}

	return 0;
}

/* end of dma_buf_ops common code*/

/*
 * dma_buf_ops (non-GEM)
 *
 * Implementation of below callbacks adds the ability to export DmaBufs to other
 * drivers.
 */

static int PVRDmaBufOpsAttach(struct dma_buf *psDmaBuf,
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)) && \
	!((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)) && (defined(CHROMIUMOS_KERNEL))))
							  struct device *psDev,
#endif
							  struct dma_buf_attachment *psAttachment)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

	return PVRDmaBufOpsAttachCommon(psPMRWrapper, psAttachment);
}

static struct sg_table *PVRDmaBufOpsMap(struct dma_buf_attachment *psAttachment,
                                        enum dma_data_direction eDirection)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psAttachment->dmabuf->priv;

	return PVRDmaBufOpsMapCommon(psPMRWrapper, psAttachment, eDirection);
}

static void PVRDmaBufOpsUnmap(struct dma_buf_attachment *psAttachment,
                              struct sg_table *psTable,
                              enum dma_data_direction eDirection)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psAttachment->dmabuf->priv;

	PVRDmaBufOpsUnmapCommon(psPMRWrapper, psAttachment, psTable, eDirection);
}

static void PVRDmaBufOpsRelease(struct dma_buf *psDmaBuf)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;
	PMR *psPMR = psPMRWrapper->psPMR;

	PMRUnrefPMR(psPMR);
}

static int PVRDmaBufOpsBeginCpuAccess(struct dma_buf *psDmaBuf,
                                      enum dma_data_direction eDirection)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

	return PVRDmaBufOpsBeginCpuAccessCommon(psPMRWrapper, eDirection);
}

static int PVRDmaBufOpsEndCpuAccess(struct dma_buf *psDmaBuf,
                                    enum dma_data_direction eDirection)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;
	int iErr;

	iErr = PVRDmaBufOpsEndCpuAccessCommon(psPMRWrapper, eDirection);

	return iErr;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
static void *PVRDmaBufOpsKMap(struct dma_buf *psDmaBuf, unsigned long uiPageNum)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

	return PVRDmaBufOpsKMapCommon(psPMRWrapper, uiPageNum);
}

static void PVRDmaBufOpsKUnMap(struct dma_buf *psDmaBuf, unsigned long uiPageNum, void *pvMem)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

	return PVRDmaBufOpsKUnMapCommon(psPMRWrapper, uiPageNum, pvMem);
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void *PVRDmaBufOpsVMap(struct dma_buf *psDmaBuf)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

	return PVRDmaBufOpsVMapCommon(psPMRWrapper);
}
#else
static int PVRDmaBufOpsVMap(struct dma_buf *psDmaBuf, struct iosys_map *psMap)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

	return PVRDmaBufOpsVMapCommon(psPMRWrapper, psMap);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void PVRDmaBufOpsVUnMap(struct dma_buf *psDmaBuf, void *pvAddr)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

	PVRDmaBufOpsVUnMapCommon(psPMRWrapper, pvAddr);
}
#else
static void PVRDmaBufOpsVUnMap(struct dma_buf *psDmaBuf, struct iosys_map *psMap)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

	PVRDmaBufOpsVUnMapCommon(psPMRWrapper, psMap);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */

static int PVRDmaBufOpsMMap(struct dma_buf *psDmaBuf, struct vm_area_struct *psVMA)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

	PVR_DPF((PVR_DBG_MESSAGE, "%s(): psDmaBuf = %px, psPMR = %px", __func__,
	         psDmaBuf, psPMRWrapper->psPMR));

	return PVRDmaBufOpsMMapCommon(psPMRWrapper, psVMA);
}

static const struct dma_buf_ops sPVRDmaBufOps =
{
	.attach        = PVRDmaBufOpsAttach,
	.map_dma_buf   = PVRDmaBufOpsMap,
	.unmap_dma_buf = PVRDmaBufOpsUnmap,
	.release       = PVRDmaBufOpsRelease,
	.begin_cpu_access = PVRDmaBufOpsBeginCpuAccess,
	.end_cpu_access   = PVRDmaBufOpsEndCpuAccess,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)) && \
	!((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)) && (defined(CHROMIUMOS_KERNEL))))
	.map_atomic    = PVRDmaBufOpsKMap,
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
	.map           = PVRDmaBufOpsKMap,
	.unmap         = PVRDmaBufOpsKUnMap,
#endif
#else /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)) */
	.kmap_atomic   = PVRDmaBufOpsKMap,
	.kmap          = PVRDmaBufOpsKMap,
	.kunmap        = PVRDmaBufOpsKUnMap,
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)) */
	.mmap          = PVRDmaBufOpsMMap,

	.vmap          = PVRDmaBufOpsVMap,
	.vunmap        = PVRDmaBufOpsVUnMap,
};

/* end of dma_buf_ops (non-GEM) */

/*
 * dma_buf_ops (GEM)
 *
 * Implementation of below callbacks adds the ability to export DmaBufs to other
 * drivers.
 */

static int PVRDmaBufOpsAttachGEM(struct dma_buf *psDmaBuf,
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)) && \
	!((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)) && (defined(CHROMIUMOS_KERNEL))))
							  struct device *psDev,
#endif
							  struct dma_buf_attachment *psAttachment)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	return PVRDmaBufOpsAttachCommon(psPMRWrapper, psAttachment);
}

static struct sg_table *PVRDmaBufOpsMapGEM(struct dma_buf_attachment *psAttachment,
                                        enum dma_data_direction eDirection)
{
	struct drm_gem_object *psObj = psAttachment->dmabuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	return PVRDmaBufOpsMapCommon(psPMRWrapper, psAttachment, eDirection);
}

static void PVRDmaBufOpsUnmapGEM(struct dma_buf_attachment *psAttachment,
                              struct sg_table *psTable,
                              enum dma_data_direction eDirection)
{
	struct drm_gem_object *psObj = psAttachment->dmabuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	PVRDmaBufOpsUnmapCommon(psPMRWrapper, psAttachment, psTable, eDirection);
}

static int PVRDmaBufOpsBeginCpuAccessGEM(struct dma_buf *psDmaBuf,
                                      enum dma_data_direction eDirection)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	return PVRDmaBufOpsBeginCpuAccessCommon(psPMRWrapper, eDirection);
}

static int PVRDmaBufOpsEndCpuAccessGEM(struct dma_buf *psDmaBuf,
                                    enum dma_data_direction eDirection)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;
	int iErr;

	iErr = PVRDmaBufOpsEndCpuAccessCommon(psPMRWrapper, eDirection);

	return iErr;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
static void *PVRDmaBufOpsKMapGEM(struct dma_buf *psDmaBuf, unsigned long uiPageNum)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	return PVRDmaBufOpsKMapCommon(psPMRWrapper, uiPageNum);
}

static void PVRDmaBufOpsKUnMapGEM(struct dma_buf *psDmaBuf, unsigned long uiPageNum, void *pvMem)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	return PVRDmaBufOpsKUnMapCommon(psPMRWrapper, uiPageNum, pvMem);
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void *PVRDmaBufOpsVMapGEM(struct dma_buf *psDmaBuf)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	return PVRDmaBufOpsVMapCommon(psPMRWrapper);
}
#else
static int PVRDmaBufOpsVMapGEM(struct dma_buf *psDmaBuf, struct iosys_map *psMap)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	return PVRDmaBufOpsVMapCommon(psPMRWrapper, psMap);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0))
static void PVRDmaBufOpsVUnMapGEM(struct dma_buf *psDmaBuf, void *pvAddr)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	PVRDmaBufOpsVUnMapCommon(psPMRWrapper, pvAddr);
}
#else
static void PVRDmaBufOpsVUnMapGEM(struct dma_buf *psDmaBuf, struct iosys_map *psMap)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	PVRDmaBufOpsVUnMapCommon(psPMRWrapper, psMap);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0) */

static int PVRDmaBufOpsMMapGEM(struct dma_buf *psDmaBuf, struct vm_area_struct *psVMA)
{
	struct drm_gem_object *psObj = psDmaBuf->priv;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	PVR_DPF((PVR_DBG_MESSAGE, "%s(): psDmaBuf = %px, psPMR = %px", __func__,
	         psDmaBuf, psPMRWrapper->psPMR));

	return PVRDmaBufOpsMMapCommon(psPMRWrapper, psVMA);
}

static const struct dma_buf_ops sPVRDmaBufOpsGEM =
{
	.attach        = PVRDmaBufOpsAttachGEM,
	.map_dma_buf   = PVRDmaBufOpsMapGEM,
	.unmap_dma_buf = PVRDmaBufOpsUnmapGEM,
	.release       = drm_gem_dmabuf_release,
	.begin_cpu_access = PVRDmaBufOpsBeginCpuAccessGEM,
	.end_cpu_access   = PVRDmaBufOpsEndCpuAccessGEM,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)) && \
	!((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)) && (defined(CHROMIUMOS_KERNEL))))
	.map_atomic    = PVRDmaBufOpsKMapGEM,
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
	.map           = PVRDmaBufOpsKMapGEM,
	.unmap         = PVRDmaBufOpsKUnMapGEM,
#endif
#else /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)) */
	.kmap_atomic   = PVRDmaBufOpsKMapGEM,
	.kmap          = PVRDmaBufOpsKMapGEM,
	.kunmap        = PVRDmaBufOpsKUnMapGEM,
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)) */
	.mmap          = PVRDmaBufOpsMMapGEM,

	.vmap          = PVRDmaBufOpsVMapGEM,
	.vunmap        = PVRDmaBufOpsVUnMapGEM,
};

/* end of dma_buf_ops (GEM) */


typedef struct _PMR_DMA_BUF_DATA_
{
	/* Filled in at PMR create time */
	PHYS_HEAP *psPhysHeap;
	struct dma_buf_attachment *psAttachment;
	PFN_DESTROY_DMABUF_PMR pfnDestroy;
	IMG_BOOL bPoisonOnFree;
	IMG_PID uiOriginPID;

	/* Mapping information. */
	struct iosys_map sMap;

	/* Modified by PMR lock/unlock */
	struct sg_table *psSgTable;
	IMG_DEV_PHYADDR *pasDevPhysAddr;
	IMG_UINT32 ui32PhysPageCount;
	IMG_UINT32 ui32VirtPageCount;

	IMG_BOOL bZombie;
} PMR_DMA_BUF_DATA;

/* Start size of the g_psDmaBufHash hash table */
#define DMA_BUF_HASH_SIZE 20

static DEFINE_MUTEX(g_FactoryLock);

static HASH_TABLE *g_psDmaBufHash;
static IMG_UINT32 g_ui32HashRefCount;

#if defined(PVR_ANDROID_ION_USE_SG_LENGTH)
#define pvr_sg_length(sg) ((sg)->length)
#else
#define pvr_sg_length(sg) sg_dma_len(sg)
#endif

static int
DmaBufSetValue(struct dma_buf *psDmaBuf, int iValue, const char *szFunc)
{
	struct iosys_map sMap;
	int err, err_end_access;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0))
	int i;
#endif

	err = dma_buf_begin_cpu_access(psDmaBuf, DMA_FROM_DEVICE);
	if (err)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to begin cpu access (err=%d)",
		                        szFunc, err));
		goto err_out;
	}

	err = dma_buf_vmap(psDmaBuf, &sMap);
	if (err)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to map page (err=%d)",
		                        szFunc, err));
		goto exit_end_access;
#else
		for (i = 0; i < psDmaBuf->size / PAGE_SIZE; i++)
		{
			void *pvKernAddr;

			pvKernAddr = dma_buf_kmap(psDmaBuf, i);
			if (IS_ERR_OR_NULL(pvKernAddr))
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Failed to map page (err=%ld)",
							szFunc,
							pvKernAddr ? PTR_ERR(pvKernAddr) : -ENOMEM));
				err = !pvKernAddr ? -ENOMEM : -EINVAL;

				goto exit_end_access;
			}

			memset(pvKernAddr, iValue, PAGE_SIZE);

			dma_buf_kunmap(psDmaBuf, i, pvKernAddr);
		}
#endif
	}
	else
	{
		memset(sMap.vaddr, iValue, psDmaBuf->size);

		dma_buf_vunmap(psDmaBuf, &sMap);
	}

	err = 0;

exit_end_access:
	do {
		err_end_access = dma_buf_end_cpu_access(psDmaBuf, DMA_TO_DEVICE);
	} while (err_end_access == -EAGAIN || err_end_access == -EINTR);

	if (err_end_access)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to end cpu access (err=%d)",
		                        szFunc, err_end_access));
		if (!err)
		{
			err = err_end_access;
		}
	}

err_out:
	return err;
}

/*****************************************************************************
 *                          PMR callback functions                           *
 *****************************************************************************/

/* This function is protected by the pfn(Get/Release)PMRFactoryLock() lock
 * acquired/released in _UnrefAndMaybeDestroy() in pmr.c. */
static void PMRFinalizeDmaBuf(PMR_IMPL_PRIVDATA pvPriv)
{
	PMR_DMA_BUF_DATA *psPrivData = pvPriv;
	struct dma_buf_attachment *psAttachment = psPrivData->psAttachment;
	struct dma_buf *psDmaBuf = psAttachment->dmabuf;
	struct sg_table *psSgTable = psPrivData->psSgTable;

	if (psDmaBuf->ops != &sPVRDmaBufOps && psDmaBuf->ops != &sPVRDmaBufOpsGEM)
	{
		if (g_psDmaBufHash)
		{
			/* We have a hash table so check if we've seen this dmabuf before */
			if (HASH_Remove(g_psDmaBufHash, (uintptr_t) psDmaBuf) != 0U)
			{
				g_ui32HashRefCount--;

				if (g_ui32HashRefCount == 0)
				{
					HASH_Delete(g_psDmaBufHash);
					g_psDmaBufHash = NULL;
				}
			}

			PVRSRVIonRemoveMemAllocRecord(psDmaBuf);
		}
	}

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	if (psPrivData->bZombie)
	{
		PVRSRVStatsDecrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_DMA_BUF_ZOMBIE,
		                            psPrivData->ui32PhysPageCount << PAGE_SHIFT,
		                            psPrivData->uiOriginPID);
	}
	else
#endif
	{
		PVRSRVStatsDecrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_DMA_BUF_IMPORT,
		                            psPrivData->ui32PhysPageCount << PAGE_SHIFT,
		                            psPrivData->uiOriginPID);
	}
#endif

	psPrivData->ui32PhysPageCount = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 36))
	dma_buf_unmap_attachment_unlocked(psAttachment, psSgTable, DMA_BIDIRECTIONAL);
#else
	dma_buf_unmap_attachment(psAttachment, psSgTable, DMA_BIDIRECTIONAL);
#endif

	if (psPrivData->bPoisonOnFree)
	{
		int err = DmaBufSetValue(psDmaBuf, PVRSRV_POISON_ON_FREE_VALUE,
		                         __func__);
		PVR_LOG_IF_FALSE(err != 0, "Failed to poison allocation before free");

		PVR_ASSERT(err != 0);
	}

	if (psPrivData->pfnDestroy)
	{
		psPrivData->pfnDestroy(psPrivData->psPhysHeap, psPrivData->psAttachment);
	}

	OSFreeMem(psPrivData->pasDevPhysAddr);
	OSFreeMem(psPrivData);
}

#if defined(SUPPORT_PMR_DEFERRED_FREE)
static PVRSRV_ERROR PMRZombifyDmaBufMem(PMR_IMPL_PRIVDATA pvPriv, PMR *psPMR)
{
	PMR_DMA_BUF_DATA *psPrivData = pvPriv;
	struct dma_buf_attachment *psAttachment = psPrivData->psAttachment;
	struct dma_buf *psDmaBuf = psAttachment->dmabuf;

	PVR_UNREFERENCED_PARAMETER(psPMR);

	psPrivData->bZombie = IMG_TRUE;

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	PVRSRVStatsDecrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_DMA_BUF_IMPORT,
	                            psPrivData->ui32PhysPageCount << PAGE_SHIFT,
	                            psPrivData->uiOriginPID);
	PVRSRVStatsIncrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_DMA_BUF_ZOMBIE,
	                            psPrivData->ui32PhysPageCount << PAGE_SHIFT,
	                            psPrivData->uiOriginPID);
#else
	PVR_UNREFERENCED_PARAMETER(pvPriv);
#endif

	PVRSRVIonZombifyMemAllocRecord(psDmaBuf);

	return PVRSRV_OK;
}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

static PVRSRV_ERROR PMRLockPhysAddressesDmaBuf(PMR_IMPL_PRIVDATA pvPriv)
{
	/* The imported memory is assumed to be backed by a permanent allocation.
	 * The PMR does not need to be (un)locked */
	PVR_UNREFERENCED_PARAMETER(pvPriv);
	return PVRSRV_OK;
}

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
static PVRSRV_ERROR PMRUnlockPhysAddressesDmaBuf(PMR_IMPL_PRIVDATA pvPriv,
                                                 PMR_IMPL_ZOMBIEPAGES *ppvZombiePages)
#else
static PVRSRV_ERROR PMRUnlockPhysAddressesDmaBuf(PMR_IMPL_PRIVDATA pvPriv)
#endif
{
	/* The imported memory is assumed to be backed by a permanent allocation.
	 * The PMR does not need to be (un)locked */
	PVR_UNREFERENCED_PARAMETER(pvPriv);
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	*ppvZombiePages = NULL;
#endif
	return PVRSRV_OK;
}

static void PMRFactoryLock(void)
{
	mutex_lock(&g_FactoryLock);
}

static void PMRFactoryUnlock(void)
{
	mutex_unlock(&g_FactoryLock);
}

static PVRSRV_ERROR PMRDevPhysAddrDmaBuf(PMR_IMPL_PRIVDATA pvPriv,
					 IMG_UINT32 ui32Log2PageSize,
					 IMG_UINT32 ui32NumOfPages,
					 IMG_DEVMEM_OFFSET_T *puiOffset,
#if defined(SUPPORT_STATIC_IPA)
					 IMG_UINT64 ui64IPAPolicyValue,
					 IMG_UINT64 ui64IPAClearMask,
#endif
					 IMG_BOOL *pbValid,
					 IMG_DEV_PHYADDR *psDevPAddr)
{
	PMR_DMA_BUF_DATA *psPrivData = pvPriv;
	IMG_UINT32 ui32PageIndex;
	IMG_UINT32 idx;

#if defined(SUPPORT_STATIC_IPA)
	PVR_UNREFERENCED_PARAMETER(ui64IPAPolicyValue);
	PVR_UNREFERENCED_PARAMETER(ui64IPAClearMask);
#endif

	if (ui32Log2PageSize != PAGE_SHIFT)
	{
		return PVRSRV_ERROR_PMR_INCOMPATIBLE_CONTIGUITY;
	}

	for (idx=0; idx < ui32NumOfPages; idx++)
	{
		if (pbValid[idx])
		{
			IMG_UINT32 ui32InPageOffset;

			ui32PageIndex = puiOffset[idx] >> PAGE_SHIFT;
			ui32InPageOffset = puiOffset[idx] - ((IMG_DEVMEM_OFFSET_T)ui32PageIndex << PAGE_SHIFT);

			PVR_LOG_RETURN_IF_FALSE(ui32PageIndex < psPrivData->ui32VirtPageCount,
			                        "puiOffset out of range", PVRSRV_ERROR_OUT_OF_RANGE);

			PVR_ASSERT(ui32InPageOffset < PAGE_SIZE);
			psDevPAddr[idx].uiAddr = psPrivData->pasDevPhysAddr[ui32PageIndex].uiAddr + ui32InPageOffset;
#if defined(SUPPORT_STATIC_IPA)
			/* Modify the physical address with the associated IPA values */
			psDevPAddr[idx].uiAddr &= ~ui64IPAClearMask;
			psDevPAddr[idx].uiAddr |= ui64IPAPolicyValue;
#endif
		}
	}
	return PVRSRV_OK;
}

static PVRSRV_ERROR
PMRAcquireKernelMappingDataDmaBuf(PMR_IMPL_PRIVDATA pvPriv,
				  size_t uiOffset,
				  size_t uiSize,
				  void **ppvKernelAddressOut,
				  IMG_HANDLE *phHandleOut,
				  PMR_FLAGS_T ulFlags)
{
	PMR_DMA_BUF_DATA *psPrivData = pvPriv;
	struct dma_buf *psDmaBuf = psPrivData->psAttachment->dmabuf;
	PVRSRV_ERROR eError;
	int err;

	if (psPrivData->ui32PhysPageCount != psPrivData->ui32VirtPageCount)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Kernel mappings for sparse DMABufs "
				"are not allowed!", __func__));
		eError = PVRSRV_ERROR_PMR_NO_KERNEL_MAPPING;
		goto fail;
	}

	err = dma_buf_begin_cpu_access(psDmaBuf, DMA_BIDIRECTIONAL);
	if (err)
	{
		eError = PVRSRV_ERROR_PMR_NO_KERNEL_MAPPING;
		goto fail;
	}

	err = dma_buf_vmap(psDmaBuf, &psPrivData->sMap);
	if (err != 0 || psPrivData->sMap.vaddr == NULL)
	{
		eError = PVRSRV_ERROR_PMR_NO_KERNEL_MAPPING;
		goto fail_kmap;
	}

	*ppvKernelAddressOut = psPrivData->sMap.vaddr + uiOffset;
	*phHandleOut = psPrivData->sMap.vaddr;

	return PVRSRV_OK;

fail_kmap:
	do {
		err = dma_buf_end_cpu_access(psDmaBuf, DMA_BIDIRECTIONAL);
	} while (err == -EAGAIN || err == -EINTR);

fail:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

static void PMRReleaseKernelMappingDataDmaBuf(PMR_IMPL_PRIVDATA pvPriv,
					      IMG_HANDLE hHandle)
{
	PMR_DMA_BUF_DATA *psPrivData = pvPriv;
	struct dma_buf *psDmaBuf = psPrivData->psAttachment->dmabuf;
	int err;

	dma_buf_vunmap(psDmaBuf, &psPrivData->sMap);

	do {
		err = dma_buf_end_cpu_access(psDmaBuf, DMA_BIDIRECTIONAL);
	} while (err == -EAGAIN || err == -EINTR);
}

static PVRSRV_ERROR PMRMMapDmaBuf(PMR_IMPL_PRIVDATA pvPriv,
                                  PMR *psPMR,
                                  PMR_MMAP_DATA pOSMMapData)
{
	PMR_DMA_BUF_DATA *psPrivData = pvPriv;
	struct dma_buf *psDmaBuf = psPrivData->psAttachment->dmabuf;
	struct vm_area_struct *psVma = pOSMMapData;
	int err;

	if (psPrivData->ui32PhysPageCount != psPrivData->ui32VirtPageCount)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Not possible to MMAP sparse DMABufs",
				__func__));
		return PVRSRV_ERROR_NOT_IMPLEMENTED;
	}

	err = dma_buf_mmap(psDmaBuf, psVma, 0);
	if (err)
	{
		return (err == -EINVAL) ? PVRSRV_ERROR_NOT_SUPPORTED : PVRSRV_ERROR_BAD_MAPPING;
	}

#if defined(PVRSRV_ENABLE_LINUX_MMAP_STATS)
	MMapStatsAddOrUpdatePMR(psPMR, psVma->vm_end - psVma->vm_start);
#endif

	return PVRSRV_OK;
}

static PMR_IMPL_FUNCTAB _sPMRDmaBufFuncTab =
{
	.pfnLockPhysAddresses = PMRLockPhysAddressesDmaBuf,
	.pfnUnlockPhysAddresses = PMRUnlockPhysAddressesDmaBuf,
	.pfnDevPhysAddr = PMRDevPhysAddrDmaBuf,
	.pfnAcquireKernelMappingData = PMRAcquireKernelMappingDataDmaBuf,
	.pfnReleaseKernelMappingData = PMRReleaseKernelMappingDataDmaBuf,
	.pfnMMap = PMRMMapDmaBuf,
	.pfnFinalize = PMRFinalizeDmaBuf,
	.pfnGetPMRFactoryLock = PMRFactoryLock,
	.pfnReleasePMRFactoryLock = PMRFactoryUnlock,
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	.pfnZombify = PMRZombifyDmaBufMem,
#endif
};

/*****************************************************************************
 *                          Public facing interface                          *
 *****************************************************************************/

PVRSRV_ERROR
PhysmemCreateNewDmaBufBackedPMR(PHYS_HEAP *psHeap,
                                struct dma_buf_attachment *psAttachment,
                                PFN_DESTROY_DMABUF_PMR pfnDestroy,
                                PVRSRV_MEMALLOCFLAGS_T uiFlags,
                                IMG_DEVMEM_SIZE_T uiChunkSize,
                                IMG_UINT32 ui32NumPhysChunks,
                                IMG_UINT32 ui32NumVirtChunks,
                                IMG_UINT32 *pui32MappingTable,
                                IMG_UINT32 ui32NameSize,
                                const IMG_CHAR pszName[DEVMEM_ANNOTATION_MAX_LEN],
                                PMR **ppsPMRPtr)
{
	struct dma_buf *psDmaBuf = psAttachment->dmabuf;
	PMR_DMA_BUF_DATA *psPrivData;
	PMR_FLAGS_T uiPMRFlags;
	IMG_BOOL bZeroOnAlloc;
	IMG_BOOL bPoisonOnAlloc;
	IMG_BOOL bPoisonOnFree;
	PVRSRV_ERROR eError;
	IMG_UINT32 i, j;
	IMG_UINT32 uiPagesPerChunk = uiChunkSize >> PAGE_SHIFT;
	IMG_UINT32 ui32PageCount = 0;
	struct scatterlist *sg;
	struct sg_table *table;
	IMG_UINT32 uiSglOffset;
	IMG_CHAR pszAnnotation[DEVMEM_ANNOTATION_MAX_LEN];
	IMG_UINT32 ui32ActualDmaBufPageCount;

	bZeroOnAlloc = PVRSRV_CHECK_ZERO_ON_ALLOC(uiFlags);
	bPoisonOnAlloc = PVRSRV_CHECK_POISON_ON_ALLOC(uiFlags);
#if defined(DEBUG)
	bPoisonOnFree = PVRSRV_CHECK_POISON_ON_FREE(uiFlags);
#else
	bPoisonOnFree = IMG_FALSE;
#endif
	if (bZeroOnAlloc && bPoisonOnFree)
	{
		/* Zero on Alloc and Poison on Alloc are mutually exclusive */
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto errReturn;
	}

	if (!PMRValidateSize((IMG_UINT64) ui32NumVirtChunks * uiChunkSize))
	{
		PVR_LOG_VA(PVR_DBG_ERROR,
				 "PMR size exceeds limit #Chunks: %u ChunkSz %"IMG_UINT64_FMTSPECX"",
				 ui32NumVirtChunks,
				 uiChunkSize);
		eError = PVRSRV_ERROR_PMR_TOO_LARGE;
		goto errReturn;
	}

	psPrivData = OSAllocZMem(sizeof(*psPrivData));
	if (psPrivData == NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto errReturn;
	}

	psPrivData->psPhysHeap = psHeap;
	psPrivData->psAttachment = psAttachment;
	psPrivData->pfnDestroy = pfnDestroy;
	psPrivData->bPoisonOnFree = bPoisonOnFree;
	psPrivData->uiOriginPID = OSGetCurrentClientProcessIDKM();
	psPrivData->ui32VirtPageCount =
			(ui32NumVirtChunks * uiChunkSize) >> PAGE_SHIFT;

	psPrivData->pasDevPhysAddr =
			OSAllocZMem(sizeof(*(psPrivData->pasDevPhysAddr)) *
			            psPrivData->ui32VirtPageCount);
	if (!psPrivData->pasDevPhysAddr)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to allocate buffer for physical addresses (oom)",
				 __func__));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto errFreePrivData;
	}

	if (bZeroOnAlloc || bPoisonOnAlloc)
	{
		int iValue = bZeroOnAlloc ? 0 : PVRSRV_POISON_ON_ALLOC_VALUE;
		int err;

		err = DmaBufSetValue(psDmaBuf, iValue, __func__);
		if (err)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to map buffer for %s",
			                        __func__,
			                        bZeroOnAlloc ? "zeroing" : "poisoning"));

			eError = PVRSRV_ERROR_PMR_NO_KERNEL_MAPPING;
			goto errFreePhysAddr;
		}
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 36))
	table = dma_buf_map_attachment_unlocked(psAttachment, DMA_BIDIRECTIONAL);
#else

	table = dma_buf_map_attachment(psAttachment, DMA_BIDIRECTIONAL);
#endif
	if (IS_ERR_OR_NULL(table))
	{
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto errFreePhysAddr;
	}

	/*
	 * We do a two pass process: first work out how many pages there
	 * are and second, fill in the data.
	 */
	for_each_sg(table->sgl, sg, table->nents, i)
	{
		ui32PageCount += PAGE_ALIGN(pvr_sg_length(sg)) / PAGE_SIZE;
	}

	if (WARN_ON(!ui32PageCount))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Number of phys. pages must not be zero",
				 __func__));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto errUnmap;
	}

	/* Obtain actual page count of dma buf */
	ui32ActualDmaBufPageCount = psAttachment->dmabuf->size / PAGE_SIZE;

	if (WARN_ON(ui32ActualDmaBufPageCount < ui32NumPhysChunks * uiPagesPerChunk))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Requested physical chunks greater than "
				"number of physical dma buf pages",
				 __func__));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto errUnmap;
	}

	psPrivData->ui32PhysPageCount = ui32ActualDmaBufPageCount;
	psPrivData->psSgTable = table;
	ui32PageCount = 0;
	sg = table->sgl;
	uiSglOffset = 0;


	/* Fill physical address array */
	for (i = 0; i < ui32NumPhysChunks; i++)
	{
		for (j = 0; j < uiPagesPerChunk; j++)
		{
			IMG_UINT32 uiIdx = pui32MappingTable[i] * uiPagesPerChunk + j;

			psPrivData->pasDevPhysAddr[uiIdx].uiAddr =
					sg_dma_address(sg) + uiSglOffset;

			/* Get the next offset for the current sgl or the next sgl */
			uiSglOffset += PAGE_SIZE;
			if (uiSglOffset >= pvr_sg_length(sg))
			{
				sg = sg_next(sg);
				uiSglOffset = 0;

				/* Check that we haven't looped */
				if (WARN_ON(sg == table->sgl))
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Failed to fill phys. address "
							"array",
							 __func__));
					eError = PVRSRV_ERROR_INVALID_PARAMS;
					goto errUnmap;
				}
			}
		}
	}

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	PVRSRVStatsIncrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_DMA_BUF_IMPORT,
	                            psPrivData->ui32PhysPageCount << PAGE_SHIFT,
	                            psPrivData->uiOriginPID);
#endif

	uiPMRFlags = (PMR_FLAGS_T)(uiFlags & PVRSRV_MEMALLOCFLAGS_PMRFLAGSMASK);

	/*
	 * Check no significant bits were lost in cast due to different
	 * bit widths for flags
	 */
	PVR_ASSERT(uiPMRFlags == (uiFlags & PVRSRV_MEMALLOCFLAGS_PMRFLAGSMASK));

	if (OSSNPrintf((IMG_CHAR *)pszAnnotation, DEVMEM_ANNOTATION_MAX_LEN, "ImpDmaBuf:%s", (IMG_CHAR *)pszName) < 0)
	{
		pszAnnotation[0] = '\0';
	}
	else
	{
		pszAnnotation[DEVMEM_ANNOTATION_MAX_LEN-1] = '\0';
	}

	eError = PMRCreatePMR(psHeap,
			      ui32NumVirtChunks * uiChunkSize,
			      ui32NumPhysChunks,
			      ui32NumVirtChunks,
			      pui32MappingTable,
			      PAGE_SHIFT,
			      uiPMRFlags,
			      pszAnnotation,
			      &_sPMRDmaBufFuncTab,
			      psPrivData,
			      PMR_TYPE_DMABUF,
			      ppsPMRPtr,
			      PDUMP_NONE);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create PMR (%s)",
				 __func__, PVRSRVGetErrorString(eError)));
		goto errFreePhysAddr;
	}

	return PVRSRV_OK;

errUnmap:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 36))
	dma_buf_unmap_attachment_unlocked(psAttachment, table, DMA_BIDIRECTIONAL);
#else

	dma_buf_unmap_attachment(psAttachment, table, DMA_BIDIRECTIONAL);
#endif
errFreePhysAddr:
	OSFreeMem(psPrivData->pasDevPhysAddr);
errFreePrivData:
	OSFreeMem(psPrivData);
errReturn:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

static void PhysmemDestroyDmaBuf(PHYS_HEAP *psHeap,
                                 struct dma_buf_attachment *psAttachment)
{
	struct dma_buf *psDmaBuf = psAttachment->dmabuf;

	PVR_UNREFERENCED_PARAMETER(psHeap);

	/* PMRUnlockSysPhysAddresses(psPMR) is redundant.
	 * See PMRUnlockPhysAddressesDmaBuf */

	dma_buf_detach(psDmaBuf, psAttachment);
	dma_buf_put(psDmaBuf);
}

struct dma_buf *
PhysmemGetDmaBuf(PMR *psPMR)
{
	PMR_DMA_BUF_DATA *psPrivData;

	psPrivData = PMRGetPrivateData(psPMR, &_sPMRDmaBufFuncTab);
	if (psPrivData)
	{
		return psPrivData->psAttachment->dmabuf;
	}

	return NULL;
}

struct dma_resv *
PhysmemGetDmaResv(PMR *psPMR)
{
	struct dma_buf *psDmaBuf = PhysmemGetDmaBuf(psPMR);

	if (psDmaBuf)
	{
		return psDmaBuf->resv;
	}
	else
	{
		PMR_DMA_BUF_WRAPPER *psPMRWrapper = PMREnvDmaBufGetExportData(psPMR);

		if (psPMRWrapper)
		{
			return &psPMRWrapper->sDmaResv;

		}

		return NULL;
	}
}

static void
PhysmemDestroyPMRWrapper(PMR_DMA_BUF_WRAPPER *psPMRWrapper)
{
	dma_resv_fini(&psPMRWrapper->sDmaResv);

	OSLockDestroy(psPMRWrapper->hKernelMappingLock);
	OSLockDestroy(psPMRWrapper->hDeviceMappingLock);
	OSFreeMem(psPMRWrapper);
}

void
PhysmemDmaBufExportFinalize(void *pvDmaBufExportData)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = pvDmaBufExportData;

	PhysmemDestroyPMRWrapper(psPMRWrapper);
}

static PVRSRV_ERROR
PhysmemCreatePMRWrapper(PMR *psPMR, PMR_DMA_BUF_WRAPPER **ppsPMRWrapper)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper;
	PVRSRV_ERROR eError;

	psPMRWrapper = OSAllocZMem(sizeof(*psPMRWrapper));
	PVR_LOG_GOTO_IF_NOMEM(psPMRWrapper, eError, fail_alloc_mem);

	psPMRWrapper->psPMR = psPMR;

	eError = OSLockCreate(&psPMRWrapper->hKernelMappingLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate.1",
	                      fail_kernel_mapping_lock_create);

	eError = OSLockCreate(&psPMRWrapper->hDeviceMappingLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate.2",
	                      fail_device_mapping_lock_create);

	dma_resv_init(&psPMRWrapper->sDmaResv);

	*ppsPMRWrapper = psPMRWrapper;

	return PVRSRV_OK;
fail_device_mapping_lock_create:
	OSLockDestroy(psPMRWrapper->hKernelMappingLock);
fail_kernel_mapping_lock_create:
	OSFreeMem(psPMRWrapper);
fail_alloc_mem:
	return eError;
}

static PVRSRV_ERROR
PhysmemGetOrCreatePMRWrapper(PMR *psPMR, PMR_DMA_BUF_WRAPPER **ppsPMRWrapper)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper;
	PVRSRV_ERROR eError;

	psPMRWrapper = PMREnvDmaBufGetExportData(psPMR);
	if (!psPMRWrapper)
	{
		PMRFactoryLock();

		/* Check again with the factory lock held */
		psPMRWrapper = PMREnvDmaBufGetExportData(psPMR);
		if (!psPMRWrapper)
		{
			eError = PhysmemCreatePMRWrapper(psPMR, &psPMRWrapper);
			PVR_LOG_GOTO_IF_ERROR(eError, "PhysmemCreatePMRWrapper", fail_create_pmr_wrapper);

			/*
			 * A PMR memory layout can't change once exported.
			 * This makes sure the exported and imported parties
			 * see the same layout of the memory.
			 */
			PMR_SetLayoutFixed(psPMR, IMG_TRUE);

			PMREnvDmaBufSetExportData(psPMR, psPMRWrapper);
		}

		PMRFactoryUnlock();
	}

	*ppsPMRWrapper = psPMRWrapper;

	return PVRSRV_OK;

fail_create_pmr_wrapper:
	PMRFactoryUnlock();
	return eError;
}

PVRSRV_ERROR
PhysmemExportDmaBuf(CONNECTION_DATA *psConnection,
                    PVRSRV_DEVICE_NODE *psDevNode,
                    PMR *psPMR,
                    IMG_INT *piFd)
{
	PMR_DMA_BUF_WRAPPER *psPMRWrapper;
	struct dma_buf *psDmaBuf;
	PVRSRV_ERROR eError;
	IMG_INT iFd;

	eError = PhysmemGetOrCreatePMRWrapper(psPMR, &psPMRWrapper);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysmemExportDmaBuf", fail_get_pmr_wrapper);

	PMRRefPMR(psPMR);

	{
		DEFINE_DMA_BUF_EXPORT_INFO(sDmaBufExportInfo);

		sDmaBufExportInfo.priv  = psPMRWrapper;
		sDmaBufExportInfo.ops   = &sPVRDmaBufOps;
		sDmaBufExportInfo.size  = PMR_LogicalSize(psPMR);
		sDmaBufExportInfo.flags = O_RDWR;
		sDmaBufExportInfo.resv  = &psPMRWrapper->sDmaResv;

		psDmaBuf = dma_buf_export(&sDmaBufExportInfo);
	}

	if (IS_ERR_OR_NULL(psDmaBuf))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to export buffer (err=%ld)",
		         __func__, psDmaBuf ? PTR_ERR(psDmaBuf) : -ENOMEM));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto fail_export;
	}

	iFd = dma_buf_fd(psDmaBuf, O_RDWR);
	if (iFd < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get dma-buf fd (err=%d)",
		         __func__, iFd));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto fail_dma_buf;
	}

	*piFd = iFd;

	return PVRSRV_OK;

fail_dma_buf:
	dma_buf_put(psDmaBuf);
	return eError;

fail_export:
	PMRUnrefPMR(psPMR);

fail_get_pmr_wrapper:
	return eError;
}

struct dma_buf *
PhysmemGEMPrimeExport(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
			struct drm_device *psDev,
#endif

			struct drm_gem_object *psObj,
			int iFlags)
{
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;
	struct dma_buf *psDmaBuf = NULL;
	DEFINE_DMA_BUF_EXPORT_INFO(sDmaBufExportInfo);


	sDmaBufExportInfo.priv  = psObj;
	sDmaBufExportInfo.ops   = &sPVRDmaBufOpsGEM;
	sDmaBufExportInfo.size  = PMR_LogicalSize(psPMRWrapper->psPMR);
	sDmaBufExportInfo.flags = iFlags;
	sDmaBufExportInfo.resv  = &psPMRWrapper->sDmaResv;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	psDmaBuf = drm_gem_dmabuf_export(psObj->dev, &sDmaBufExportInfo);
#else
	psDmaBuf = drm_gem_dmabuf_export(psDev, &sDmaBufExportInfo);
#endif
	if (IS_ERR(psDmaBuf))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to export buffer (err=%ld)",
		         __func__, PTR_ERR(psDmaBuf)));
	}

	return psDmaBuf;
}

void
PhysmemGEMObjectFree(struct drm_gem_object *psObj)
{
	PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

	drm_gem_object_release(psObj);

	if (smp_load_acquire(&psPMRWrapper->psObj) == psObj)
	{
		smp_store_release(&psPMRWrapper->psObj, NULL);
	}

	PMRUnrefPMR(psPMRWrapper->psPMR);

	OSFreeMem(psGEMObj);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
static const struct drm_gem_object_funcs sPhysmemGEMObjFuncs = {
	.export = PhysmemGEMPrimeExport,
	.free = PhysmemGEMObjectFree,
};
#endif

PVRSRV_ERROR
PhysmemExportGemHandle(CONNECTION_DATA *psConnection,
		       PVRSRV_DEVICE_NODE *psDevNode,
		       PMR *psPMR,
		       IMG_UINT32 *puHandle)
{
	struct device *psDev = psDevNode->psDevConfig->pvOSDevice;
	struct drm_device *psDRMDev = dev_get_drvdata(psDev);
	struct drm_file *psDRMFile = OSGetDRMFile(psConnection);
	PMR_DMA_BUF_WRAPPER *psPMRWrapper;
	PMR_DMA_BUF_GEM_OBJ *psGEMObj;
	bool bAlreadyExported;
	PVRSRV_ERROR eError;
	int iErr;

	eError = PhysmemGetOrCreatePMRWrapper(psPMR, &psPMRWrapper);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysmemExportGemHandle", fail_get_pmr_wrapper);

	psGEMObj = OSAllocZMem(sizeof(*psGEMObj));
	PVR_LOG_GOTO_IF_NOMEM(psGEMObj, eError, fail_alloc_mem);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	psGEMObj->sBase.funcs = &sPhysmemGEMObjFuncs;
#endif
	psGEMObj->psPMRWrapper = psPMRWrapper;

	PMRRefPMR(psPMR);

	drm_gem_private_object_init(psDRMDev, &psGEMObj->sBase,
	                            PMR_LogicalSize(psPMR));

	PMRFactoryLock();
	bAlreadyExported = smp_load_acquire(&psPMRWrapper->psObj) != NULL;
	if (!bAlreadyExported)
	{
		smp_store_release(&psPMRWrapper->psObj, &psGEMObj->sBase);
	}
	PMRFactoryUnlock();

	if (bAlreadyExported)
	{
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_STILL_REFERENCED,
					    fail_export_check);
	}

	iErr = drm_gem_handle_create(psDRMFile, &psGEMObj->sBase, puHandle);
	if (iErr)
	{
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_OUT_OF_MEMORY,
				    fail_handle_create);
	}

	/* The handle holds a reference on the object, so drop ours */
	drm_gem_object_put(&psGEMObj->sBase);

	return PVRSRV_OK;

fail_export_check:
fail_handle_create:
	drm_gem_object_put(&psGEMObj->sBase);
	return eError;

fail_alloc_mem:
fail_get_pmr_wrapper:
	return eError;
}

PVRSRV_ERROR
PhysmemImportDmaBuf(CONNECTION_DATA *psConnection,
                    PVRSRV_DEVICE_NODE *psDevNode,
                    IMG_INT fd,
                    PVRSRV_MEMALLOCFLAGS_T uiFlags,
                    IMG_UINT32 ui32NameSize,
                    const IMG_CHAR pszName[DEVMEM_ANNOTATION_MAX_LEN],
                    PMR **ppsPMRPtr,
                    IMG_DEVMEM_SIZE_T *puiSize,
                    IMG_DEVMEM_ALIGN_T *puiAlign)
{
	IMG_DEVMEM_SIZE_T uiSize;
	IMG_UINT32 ui32MappingTable = 0;
	struct dma_buf *psDmaBuf;
	PVRSRV_ERROR eError;

	/* Get the buffer handle */
	psDmaBuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(psDmaBuf))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get dma-buf from fd (err=%ld)",
				 __func__, psDmaBuf ? PTR_ERR(psDmaBuf) : -ENOMEM));
		return PVRSRV_ERROR_BAD_MAPPING;

	}

	uiSize = psDmaBuf->size;

	eError = PhysmemImportSparseDmaBuf(psConnection,
	                                 psDevNode,
	                                 fd,
	                                 uiFlags,
	                                 uiSize,
	                                 1,
	                                 1,
	                                 &ui32MappingTable,
	                                 ui32NameSize,
	                                 pszName,
	                                 ppsPMRPtr,
	                                 puiSize,
	                                 puiAlign);

	dma_buf_put(psDmaBuf);

	return eError;
}

PVRSRV_ERROR
PhysmemImportSparseDmaBuf(CONNECTION_DATA *psConnection,
                          PVRSRV_DEVICE_NODE *psDevNode,
                          IMG_INT fd,
                          PVRSRV_MEMALLOCFLAGS_T uiFlags,
                          IMG_DEVMEM_SIZE_T uiChunkSize,
                          IMG_UINT32 ui32NumPhysChunks,
                          IMG_UINT32 ui32NumVirtChunks,
                          IMG_UINT32 *pui32MappingTable,
                          IMG_UINT32 ui32NameSize,
                          const IMG_CHAR pszName[DEVMEM_ANNOTATION_MAX_LEN],
                          PMR **ppsPMRPtr,
                          IMG_DEVMEM_SIZE_T *puiSize,
                          IMG_DEVMEM_ALIGN_T *puiAlign)
{
	PMR *psPMR = NULL;
	struct dma_buf_attachment *psAttachment;
	struct dma_buf *psDmaBuf;
	PVRSRV_ERROR eError;
	IMG_BOOL bHashTableCreated = IMG_FALSE;

	PVR_UNREFERENCED_PARAMETER(psConnection);

	PVR_GOTO_IF_INVALID_PARAM(psDevNode != NULL, eError, errReturn);

	/* Terminate string from bridge to prevent corrupt annotations in RI */
	if (pszName != NULL)
	{
		IMG_CHAR* pszName0 = (IMG_CHAR*) pszName;
		pszName0[ui32NameSize-1] = '\0';
	}

	PMRFactoryLock();

	/* Get the buffer handle */
	psDmaBuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(psDmaBuf))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get dma-buf from fd (err=%ld)",
		        __func__, psDmaBuf ? PTR_ERR(psDmaBuf) : -ENOMEM));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_BAD_MAPPING, errUnlockReturn);
	}

	if (psDmaBuf->ops == &sPVRDmaBufOps || psDmaBuf->ops == &sPVRDmaBufOpsGEM)
	{
		/* We exported this dma_buf, so we can just get its PMR. */
		if (psDmaBuf->ops == &sPVRDmaBufOps)
		{
			PMR_DMA_BUF_WRAPPER *psPMRWrapper = psDmaBuf->priv;

			psPMR = psPMRWrapper->psPMR;
		}
		else
		{
			struct drm_gem_object *psObj = psDmaBuf->priv;
			PMR_DMA_BUF_GEM_OBJ *psGEMObj = TO_PMR_DMA_BUF_GEM_OBJ(psObj);
			PMR_DMA_BUF_WRAPPER *psPMRWrapper = psGEMObj->psPMRWrapper;

			psPMR = psPMRWrapper->psPMR;
		}

		/* However, we can't import it if it belongs to a different device. */
		if (PMR_DeviceNode(psPMR) != psDevNode)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: PMR invalid for this device",
			        __func__));
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_NOT_PERMITTED,
			                    errUnlockAndDMAPut);
		}
	}
	else if (g_psDmaBufHash != NULL)
	{
		/* We have a hash table so check if we've seen this dmabuf
		 * before. */
		psPMR = (PMR *) HASH_Retrieve(g_psDmaBufHash, (uintptr_t) psDmaBuf);
	}
	else
	{
		/* As different processes may import the same dmabuf we need to
		 * create a hash table so we don't generate a duplicate PMR but
		 * rather just take a reference on an existing one. */
		g_psDmaBufHash = HASH_Create(DMA_BUF_HASH_SIZE);
		PVR_GOTO_IF_NOMEM(g_psDmaBufHash, eError, errUnlockAndDMAPut);

		bHashTableCreated = IMG_TRUE;
	}

	if (psPMR != NULL)
	{
#if defined(SUPPORT_PMR_DEFERRED_FREE)
		if (PMR_IsZombie(psPMR))
		{
			PMRDequeueZombieAndRef(psPMR);
		}
		else
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

		{
			/* Reuse the PMR we already created */
			PMRRefPMR(psPMR);
		}

		*ppsPMRPtr = psPMR;
		*puiSize = PMR_LogicalSize(psPMR);
		*puiAlign = PAGE_SIZE;

		PMRFactoryUnlock();
		dma_buf_put(psDmaBuf);

		/* We expect a PMR to be immutable at this point.
		 * But its explicitly set here to cover a corner case
		 * where a PMR created through non-DMA interface could be
		 * imported back again through DMA interface. */
		PMR_SetLayoutFixed(psPMR, IMG_TRUE);

		return PVRSRV_OK;
	}

	/* Do we want this to be a sparse PMR? */
	if (ui32NumVirtChunks > 1)
	{
		/* Parameter validation */
		if (psDmaBuf->size < (uiChunkSize * ui32NumPhysChunks) ||
		    uiChunkSize != PAGE_SIZE)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Requesting sparse buffer: "
					"uiChunkSize ("IMG_DEVMEM_SIZE_FMTSPEC") must be equal to "
					"OS page size (%lu). uiChunkSize * ui32NumPhysChunks "
					"("IMG_DEVMEM_SIZE_FMTSPEC") must"
					" not be greater than the buffer size ("IMG_SIZE_FMTSPEC").",
					 __func__,
					uiChunkSize,
					PAGE_SIZE,
					uiChunkSize * ui32NumPhysChunks,
					psDmaBuf->size));
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS,
			                    errUnlockAndDMAPut);
		}
	}
	else
	{
		/* if ui32NumPhysChunks == 0 then pui32MappingTable == NULL
		 * this is handled by the generated bridge code.
		 * Because ui32NumPhysChunks is set to 1 below, we don't allow NULL array */
		if (pui32MappingTable == NULL)
		{
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS,
			                    errUnlockAndDMAPut);
		}

		/* Make sure parameters are valid for non-sparse allocations as well */
		uiChunkSize = psDmaBuf->size;
		ui32NumPhysChunks = 1;
		ui32NumVirtChunks = 1;
	}

	{
		IMG_DEVMEM_SIZE_T uiSize = ui32NumVirtChunks * uiChunkSize;
		IMG_UINT32 uiLog2PageSize = PAGE_SHIFT; /* log2(uiChunkSize) */
		eError = PhysMemValidateParams(psDevNode,
		                               ui32NumPhysChunks,
		                               ui32NumVirtChunks,
		                               pui32MappingTable,
		                               uiFlags,
		                               &uiLog2PageSize,
		                               &uiSize);
		PVR_LOG_GOTO_IF_ERROR(eError, "PhysMemValidateParams", errUnlockAndDMAPut);
	}

	psAttachment = dma_buf_attach(psDmaBuf, psDevNode->psDevConfig->pvOSDevice);
	if (IS_ERR_OR_NULL(psAttachment))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to attach to dma-buf (err=%ld)",
				 __func__, psAttachment? PTR_ERR(psAttachment) : -ENOMEM));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_BAD_MAPPING,
		                    errUnlockAndDMAPut);
	}

	/*
	 * Note:
	 * While we have no way to determine the type of the buffer we just
	 * assume that all dmabufs are from the same physical heap.
	 */
	eError = PhysmemCreateNewDmaBufBackedPMR(psDevNode->apsPhysHeap[PVRSRV_PHYS_HEAP_EXTERNAL],
	                                         psAttachment,
	                                         PhysmemDestroyDmaBuf,
	                                         uiFlags,
	                                         uiChunkSize,
	                                         ui32NumPhysChunks,
	                                         ui32NumVirtChunks,
	                                         pui32MappingTable,
	                                         ui32NameSize,
	                                         pszName,
	                                         &psPMR);
	PVR_GOTO_IF_ERROR(eError, errDMADetach);

	/* The imported memory is assumed to be backed by a permanent allocation. */
	/* PMRLockSysPhysAddresses(psPMR) is redundant.
	 * See PMRLockPhysAddressesDmaBuf */

	/* First time we've seen this dmabuf so store it in the hash table */
	HASH_Insert(g_psDmaBufHash, (uintptr_t) psDmaBuf, (uintptr_t) psPMR);
	g_ui32HashRefCount++;

	PMRFactoryUnlock();

	PVRSRVIonAddMemAllocRecord(psDmaBuf);

	*ppsPMRPtr = psPMR;
	*puiSize = ui32NumVirtChunks * uiChunkSize;
	*puiAlign = PAGE_SIZE;

	/* The memory that's just imported is owned by some other entity.
	 * Hence the memory layout cannot be changed through our API */
	PMR_SetLayoutFixed(psPMR, IMG_TRUE);

	return PVRSRV_OK;

errDMADetach:
	dma_buf_detach(psDmaBuf, psAttachment);

errUnlockAndDMAPut:
	if (bHashTableCreated)
	{
		HASH_Delete(g_psDmaBufHash);
		g_psDmaBufHash = NULL;
	}
	dma_buf_put(psDmaBuf);

errUnlockReturn:
	PMRFactoryUnlock();

errReturn:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

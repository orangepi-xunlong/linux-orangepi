/*************************************************************************/ /*!
@File
@Title          Implementation of PMR functions to wrap non-services allocated
                memory.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of the memory management.  This module is responsible for
                implementing the function callbacks for physical memory.
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

#include <linux/uaccess.h>
#include "img_types.h"
#include "img_defs.h"
#include "pvrsrv_error.h"
#include "pvr_debug.h"
#include "devicemem_server_utils.h"

#include "physmem_extmem.h"
#include "physmem_extmem_wrap.h"
#include "pdump_physmem.h"
#include "pmr.h"
#include "pmr_impl.h"
#include "physmem.h"
#include "cache_km.h"

#include "kernel_compatibility.h"

typedef struct _WRAP_KERNEL_MAP_DATA_
{
	void *pvKernLinAddr;
	IMG_BOOL bVMAP;
} WRAP_KERNEL_MAP_DATA;

/* Free the PMR private data */
static void _FreeWrapData(PMR_WRAP_DATA *psPrivData)
{
	OSFreeMem(psPrivData->ppsPageArray);
	OSFreeMem(psPrivData->ppvPhysAddr);
	psPrivData->psVMArea = NULL;
	OSFreeMem(psPrivData);
}


/* Allocate the PMR private data */
static PVRSRV_ERROR _AllocWrapData(PMR_WRAP_DATA **ppsPrivData,
                            PVRSRV_DEVICE_NODE *psDevNode,
                             IMG_DEVMEM_SIZE_T uiSize,
                            IMG_CPU_VIRTADDR pvCpuVAddr,
                            PVRSRV_MEMALLOCFLAGS_T uiFlags)
{
	PVRSRV_ERROR eError;
	PMR_WRAP_DATA *psPrivData;
	struct vm_area_struct *psVMArea;
	IMG_UINT32 ui32CPUCacheMode;

	/* Obtain a reader lock on the process mmap lock while we are using
	 * the psVMArea.
	 */

	mmap_read_lock(current->mm);

	/* Find the VMA */
	psVMArea = find_vma(current->mm, (uintptr_t)pvCpuVAddr);
	if (psVMArea == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Couldn't find memory region containing start address %p",
				__func__,
				(void*) pvCpuVAddr));
		eError = PVRSRV_ERROR_INVALID_CPU_ADDR;
		goto eUnlockReturn;
	}

	/* If requested size is larger than actual allocation
	 * return error. Can never request more memory to be imported
	 * than its original allocation */
	/* Now check the end address is in range */
	if (((uintptr_t)pvCpuVAddr + uiSize) > psVMArea->vm_end)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: End address %p is outside of the region returned by find_vma",
				__func__,
				(void*) (uintptr_t)((uintptr_t)pvCpuVAddr + uiSize)));
		eError = PVRSRV_ERROR_BAD_PARAM_SIZE;
		goto eUnlockReturn;
	}

	/* Find_vma locates a region with an end point past a given
	 * virtual address. So check the address is actually in the region. */
	if ((uintptr_t)pvCpuVAddr < psVMArea->vm_start)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Start address %p is outside of the region returned by find_vma",
				__func__,
				(void*) pvCpuVAddr));
		eError = PVRSRV_ERROR_INVALID_CPU_ADDR;
		goto eUnlockReturn;
	}

	eError = DevmemCPUCacheMode(uiFlags, &ui32CPUCacheMode);
	if (eError != PVRSRV_OK)
	{
		goto eUnlockReturn;
	}

	/* Allocate and initialise private factory data */
	psPrivData = OSAllocZMem(sizeof(*psPrivData));
	if (psPrivData == NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto eUnlockReturn;
	}

	psPrivData->ui32CPUCacheFlags = ui32CPUCacheMode;

	/* Track the VMA area structure so that it can be checked later */
	psPrivData->psVMArea = psVMArea;

	psPrivData->psDevNode = psDevNode;
	psPrivData->uiTotalNumPages = uiSize >> PAGE_SHIFT;

	/* Allocate page and phys address arrays */
	psPrivData->ppsPageArray = OSAllocZMem(sizeof(*(psPrivData->ppsPageArray)) * psPrivData->uiTotalNumPages);
	if (psPrivData == NULL)
	{
		OSFreeMem(psPrivData);
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto eUnlockReturn;
	}

	psPrivData->ppvPhysAddr = OSAllocZMem(sizeof(*(psPrivData->ppvPhysAddr)) * psPrivData->uiTotalNumPages);
	if (psPrivData->ppvPhysAddr == NULL)
	{
		OSFreeMem(psPrivData->ppsPageArray);
		OSFreeMem(psPrivData);
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto eUnlockReturn;
	}

	if (uiFlags & (PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
					PVRSRV_MEMALLOCFLAG_CPU_WRITE_PERMITTED))
	{
		psPrivData->bWrite = IMG_TRUE;
	}

	*ppsPrivData = psPrivData;

	eError = PVRSRV_OK;
eUnlockReturn:
	mmap_read_unlock(current->mm);

	return eError;
}

#if defined(SUPPORT_LINUX_WRAP_EXTMEM_PAGE_TABLE_WALK)
/* Free the pages we got via _TryFindVMA() */
static void _FreeFindVMAPages(PMR_WRAP_DATA *psPrivData)
{
	IMG_UINT32 i;

	for (i = 0; i < psPrivData->uiTotalNumPages; i++)
	{
		if (psPrivData->ppsPageArray[i] != NULL)
		{
			/* Release the page */
			put_page(psPrivData->ppsPageArray[i]);
		}
	}
}
#endif

/* Release pages got via get_user_pages().
 * As the function comment for GUP states we have
 * to set the page dirty if it has been written to and
 * we have to put the page to decrease its refcount. */
static void _FreeGetUserPages(PMR_WRAP_DATA *psPrivData)
{
	IMG_UINT32 i;

	for (i = 0; i < psPrivData->uiTotalNumPages; i++)
	{
		if (psPrivData->ppsPageArray[i])
		{
			if (IMG_TRUE == psPrivData->bWrite)
			{
				/* Write data back to fs if necessary */
				set_page_dirty_lock(psPrivData->ppsPageArray[i]);
			}

			/* Release the page */
			put_page(psPrivData->ppsPageArray[i]);
		}
	}
}

/* Get the page structures and physical addresses mapped to
 * a CPU virtual range via get_user_pages() */
static PVRSRV_ERROR _TryGetUserPages(PVRSRV_DEVICE_NODE *psDevNode,
                                IMG_DEVMEM_SIZE_T uiSize,
                                IMG_CPU_VIRTADDR pvCpuVAddr,
                                PMR_WRAP_DATA *psPrivData)
{
	IMG_INT32 iMappedPages, i;

	IMG_UINT64 ui64DmaMask = dma_get_mask(psDevNode->psDevConfig->pvOSDevice);

	/* Do the actual call */
	iMappedPages = get_user_pages_fast((uintptr_t) pvCpuVAddr, uiSize >> PAGE_SHIFT,
	                                    psPrivData->bWrite, psPrivData->ppsPageArray);
	if (iMappedPages < 0)
	{
		PVR_DPF((PVR_DBG_MESSAGE,
		         "get_user_pages_fast() failed, got back %d, expected num pages %d",
		         iMappedPages,
		         psPrivData->uiTotalNumPages));

		return PVRSRV_ERROR_FAILED_TO_ACQUIRE_PAGES;
	}

	/* Fill the physical address array */
	for (i = 0; i < psPrivData->uiTotalNumPages; i++)
	{
		if (psPrivData->ppsPageArray[i])
		{
			psPrivData->ppvPhysAddr[i].uiAddr = page_to_phys(psPrivData->ppsPageArray[i]);
			psPrivData->uiNumBackedPages += 1;
		}
		else
		{
			psPrivData->ppvPhysAddr[i].uiAddr = 0;
		}


		/* Check the data transfer capability of the DMA.
		 *
		 * For instance:
		 * APOLLO test chips TCF5 or ES2 can only access 4G maximum memory from the card.
		 * Hence pages with a physical address beyond 4G range cannot be accessed by the
		 * device. An error is reported in such a case. */
		if (psPrivData->ppvPhysAddr[i].uiAddr & ~ui64DmaMask)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"Backed page @pos:%d Physical Address: %pa exceeds GPU "
					"accessible range(mask): 0x%0llx",
					i, &psPrivData->ppvPhysAddr[i].uiAddr, ui64DmaMask));

			psPrivData->bWrite = IMG_FALSE;

			/* Free the acquired pages */
			_FreeGetUserPages(psPrivData);

			return PVRSRV_ERROR_INVALID_PHYS_ADDR;
		}
	}

	return PVRSRV_OK;
}

/* Release all the pages we got via _TryGetUserPages or _TryFindVMA */
static PVRSRV_ERROR _WrapExtMemReleasePages(PMR_WRAP_DATA *psPrivData)
{
	switch (psPrivData->eWrapType)
	{
		case WRAP_TYPE_GET_USER_PAGES:
			_FreeGetUserPages(psPrivData);
			break;
#if defined(SUPPORT_LINUX_WRAP_EXTMEM_PAGE_TABLE_WALK)
		case WRAP_TYPE_FIND_VMA:
			_FreeFindVMAPages(psPrivData);
			break;
#endif
		case WRAP_TYPE_NULL:
			/* fall through */
		default:
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Wrong wrap type, cannot release pages",
					__func__));
			return PVRSRV_ERROR_INVALID_WRAP_TYPE;
	}

	_FreeWrapData(psPrivData);

	return PVRSRV_OK;
}

/* Try to get pages and physical addresses for a CPU virtual address.
 * Will try both methods:
 * get_user_pages or find_vma + page table walk if the first one fails */
static PVRSRV_ERROR _WrapExtMemAcquirePages(PVRSRV_DEVICE_NODE *psDevNode,
                                            IMG_DEVMEM_SIZE_T uiSize,
                                            IMG_CPU_VIRTADDR pvCpuVAddr,
                                            PMR_WRAP_DATA *psPrivData)
{
	PVRSRV_ERROR eError;

	eError = _TryGetUserPages(psDevNode,
	                            uiSize,
	                            pvCpuVAddr,
	                            psPrivData);
	if (eError == PVRSRV_OK)
	{
		psPrivData->eWrapType = WRAP_TYPE_GET_USER_PAGES;
		PVR_DPF((PVR_DBG_MESSAGE,
				"%s: Used GetUserPages",
				__func__));
		return PVRSRV_OK;
	}

#if defined(SUPPORT_LINUX_WRAP_EXTMEM_PAGE_TABLE_WALK)
	if (PVRSRV_ERROR_INVALID_PHYS_ADDR != eError)
	{
		PVRSRV_ERROR eError2;
		eError2 = _TryFindVMA(uiSize,
		                      (uintptr_t) pvCpuVAddr,
		                      psPrivData);
		if (eError2 == PVRSRV_OK)
		{
			psPrivData->eWrapType = WRAP_TYPE_FIND_VMA;
			PVR_DPF((PVR_DBG_MESSAGE,
					"%s: Used FindVMA",
					__func__));
			return PVRSRV_OK;
		}

		PVR_WARN_IF_ERROR(eError, "_TryGetUserPages");
		PVR_LOG_ERROR(eError2, "_TryFindVMA");
	}
	else
	{
		PVR_LOG_ERROR(eError, "_TryGetUserPages");
	}
#else
	PVR_LOG_ERROR(eError, "_TryGetUserPages");
#endif

	psPrivData->eWrapType = WRAP_TYPE_NULL;
	return eError;
}

static PVRSRV_ERROR
PMRSysPhysAddrExtMem(PMR_IMPL_PRIVDATA pvPriv,
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
	const PMR_WRAP_DATA *psWrapData = pvPriv;
	IMG_UINT32 uiPageSize = 1U << PAGE_SHIFT;
	IMG_UINT32 uiInPageOffset;
	IMG_UINT32 uiPageIndex;
	IMG_UINT32 uiIdx;

#if defined(SUPPORT_STATIC_IPA)
	PVR_UNREFERENCED_PARAMETER(ui64IPAPolicyValue);
	PVR_UNREFERENCED_PARAMETER(ui64IPAClearMask);
#endif


	if (PAGE_SHIFT != ui32Log2PageSize)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Requested ui32Log2PageSize %u is different from "
				"OS page shift %u. Not supported.",
				__func__,
				ui32Log2PageSize,
				PAGE_SHIFT));
		return PVRSRV_ERROR_PMR_INCOMPATIBLE_CONTIGUITY;
	}

	for (uiIdx=0; uiIdx < ui32NumOfPages; uiIdx++)
	{
		uiPageIndex = puiOffset[uiIdx] >> PAGE_SHIFT;
		uiInPageOffset = puiOffset[uiIdx] - ((IMG_DEVMEM_OFFSET_T)uiPageIndex << PAGE_SHIFT);

		PVR_LOG_RETURN_IF_FALSE(uiPageIndex < psWrapData->uiTotalNumPages,
		                        "puiOffset out of range", PVRSRV_ERROR_OUT_OF_RANGE);

		PVR_ASSERT(uiInPageOffset < uiPageSize);

		/* The ExtMem is only enabled in UMA mode, in that mode, the device physical
		 * address translation will be handled by the PRM factory after this call.
		 * Here we just copy the physical address like other callback implementations. */
		psDevPAddr[uiIdx].uiAddr = psWrapData->ppvPhysAddr[uiPageIndex].uiAddr;

		pbValid[uiIdx] = (psDevPAddr[uiIdx].uiAddr)? IMG_TRUE:IMG_FALSE;

		psDevPAddr[uiIdx].uiAddr += uiInPageOffset;
#if defined(SUPPORT_STATIC_IPA)
		psDevPAddr[uiIdx].uiAddr &= ~ui64IPAClearMask;
		psDevPAddr[uiIdx].uiAddr |= ui64IPAPolicyValue;
#endif	/* SUPPORT_STATIC_IPA */
	}

	return PVRSRV_OK;
}


static void
PMRFinalizeExtMem(PMR_IMPL_PRIVDATA pvPriv)
{
	PMR_WRAP_DATA *psWrapData = pvPriv;

	PVRSRV_ERROR eError = _WrapExtMemReleasePages(psWrapData);
	PVR_LOG_IF_ERROR(eError, "_WrapExtMemReleasePages");
}


static void _UnmapPage(PMR_WRAP_DATA *psWrapData,
                       WRAP_KERNEL_MAP_DATA *psMapData)
{
	IMG_BOOL bSuccess;

	if (psMapData->bVMAP)
	{
		vunmap(psMapData->pvKernLinAddr);
	}
	else
	{
		if (psWrapData->ui32CPUCacheFlags & ~(PVRSRV_MEMALLOCFLAG_CPU_CACHE_MODE_MASK))
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Unable to unmap wrapped extmem "
			        "page - wrong cached mode flags passed. This may leak "
			        "memory.", __func__));
			PVR_ASSERT(!"Found non-cpu cache mode flag when unmapping from "
			           "the cpu");
		}
		else
		{
			bSuccess = OSUnMapPhysToLin(psMapData->pvKernLinAddr, PAGE_SIZE);
			if (!bSuccess)
			{
				PVR_DPF((PVR_DBG_ERROR, "%s: Unable to unmap wrapped extmem "
				        "page. This may leak memory.", __func__));
			}
		}
	}
}

static void _MapPage(PMR_WRAP_DATA *psWrapData,
                     IMG_UINT32 uiPageIdx,
                     WRAP_KERNEL_MAP_DATA *psMapData)
{
	IMG_UINT8 *puiLinCPUAddr;

	if (psWrapData->ppsPageArray[uiPageIdx])
	{
		pgprot_t prot = PAGE_KERNEL;

		switch (PVRSRV_CPU_CACHE_MODE(psWrapData->ui32CPUCacheFlags))
		{
			case PVRSRV_MEMALLOCFLAG_CPU_UNCACHED:
				prot = pgprot_noncached(prot);
				break;

			case PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC:
				prot = pgprot_writecombine(prot);
				break;

			case PVRSRV_MEMALLOCFLAG_CPU_CACHED:
				break;

			default:
				break;
		}

		puiLinCPUAddr = vmap(&psWrapData->ppsPageArray[uiPageIdx], 1, VM_MAP, prot);

		psMapData->bVMAP = IMG_TRUE;
	}
	else
	{
		puiLinCPUAddr = (IMG_UINT8*) OSMapPhysToLin(psWrapData->ppvPhysAddr[uiPageIdx],
		                                              PAGE_SIZE,
		                                              psWrapData->ui32CPUCacheFlags);

		psMapData->bVMAP = IMG_FALSE;
	}

	psMapData->pvKernLinAddr = puiLinCPUAddr;

}

static PVRSRV_ERROR _CopyBytesExtMem(PMR_IMPL_PRIVDATA pvPriv,
                                     IMG_DEVMEM_OFFSET_T uiOffset,
                                     IMG_UINT8 *pcBuffer,
                                     size_t uiBufSz,
                                     size_t *puiNumBytes,
                                     IMG_BOOL bWrite)
{
	PMR_WRAP_DATA *psWrapData = (PMR_WRAP_DATA*) pvPriv;
	size_t uiBytesToCopy = uiBufSz;
	IMG_UINT32 uiPageIdx = uiOffset >> PAGE_SHIFT;
	IMG_BOOL bFirst = IMG_TRUE;
	WRAP_KERNEL_MAP_DATA sKernMapData;


	if ((uiBufSz + uiOffset) > (psWrapData->uiTotalNumPages << PAGE_SHIFT))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Trying to read out of bounds of PMR.",
				__func__));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Copy the pages */
	while (uiBytesToCopy != 0)
	{
		size_t uiBytesPerLoop;
		IMG_DEVMEM_OFFSET_T uiCopyOffset;

		if (bFirst)
		{
			bFirst = IMG_FALSE;
			uiCopyOffset = (uiOffset & ~PAGE_MASK);
			uiBytesPerLoop = (uiBufSz >= (PAGE_SIZE - uiCopyOffset)) ?
					PAGE_SIZE - uiCopyOffset : uiBufSz;
		}
		else
		{
			uiCopyOffset = 0;
			uiBytesPerLoop = (uiBytesToCopy > PAGE_SIZE) ? PAGE_SIZE : uiBytesToCopy;
		}

		_MapPage(psWrapData,
		         uiPageIdx,
		         &sKernMapData);

		if (sKernMapData.pvKernLinAddr == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Unable to map wrapped extmem page.",
					__func__));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}

		/* Use 'DeviceMemCopy' because we need to be conservative. We can't
		 * know whether the wrapped memory was originally imported as cached.
		 */

		if (bWrite)
		{
			OSDeviceMemCopy(sKernMapData.pvKernLinAddr + uiCopyOffset,
			          pcBuffer,
			          uiBytesPerLoop);
		}
		else
		{
			OSDeviceMemCopy(pcBuffer,
			          sKernMapData.pvKernLinAddr + uiCopyOffset,
			          uiBytesPerLoop);
		}

		_UnmapPage(psWrapData,
		           &sKernMapData);

		uiBytesToCopy -= uiBytesPerLoop;
		uiPageIdx++;
	}

	*puiNumBytes = uiBufSz;

	return PVRSRV_OK;
}

static PVRSRV_ERROR
PMRReadBytesExtMem(PMR_IMPL_PRIVDATA pvPriv,
                   IMG_DEVMEM_OFFSET_T uiOffset,
                   IMG_UINT8 *pcBuffer,
                   size_t uiBufSz,
                   size_t *puiNumBytes)
{

	return _CopyBytesExtMem(pvPriv,
	                        uiOffset,
	                        pcBuffer,
	                        uiBufSz,
	                        puiNumBytes,
	                        IMG_FALSE);
}

static PVRSRV_ERROR
PMRWriteBytesExtMem(PMR_IMPL_PRIVDATA pvPriv,
                    IMG_DEVMEM_OFFSET_T uiOffset,
                    IMG_UINT8 *pcBuffer,
                    size_t uiBufSz,
                    size_t *puiNumBytes)
{
	return _CopyBytesExtMem(pvPriv,
	                        uiOffset,
	                        pcBuffer,
	                        uiBufSz,
	                        puiNumBytes,
	                        IMG_TRUE);
}


static PVRSRV_ERROR
PMRAcquireKernelMappingDataExtMem(PMR_IMPL_PRIVDATA pvPriv,
                                  size_t uiOffset,
                                  size_t uiSize,
                                  void **ppvKernelAddressOut,
                                  IMG_HANDLE *phHandleOut,
                                  PMR_FLAGS_T ulFlags)
{
	PVRSRV_ERROR eError;
	PMR_WRAP_DATA *psWrapData = (PMR_WRAP_DATA*) pvPriv;
	WRAP_KERNEL_MAP_DATA *psKernMapData;
	IMG_UINT32 ui32PageIndex = uiOffset >> PAGE_SHIFT;
	IMG_UINT32 ui32PageOffset = (IMG_UINT32)(uiOffset & ~PAGE_MASK);

	/* Offset was out of bounds */
	if (ui32PageIndex > psWrapData->uiTotalNumPages)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Error, offset out of PMR bounds.",
				__func__));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto e0;
	}

	/* We can not map in more than one page with ioremap.
	 * Only possible with physically contiguous pages */
	if ((ui32PageOffset + uiSize) > PAGE_SIZE)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Error, cannot map more than one page for wrapped extmem.",
				__func__));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto e0;
	}

	psKernMapData = OSAllocMem(sizeof(*psKernMapData));
	if (psKernMapData == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Error, unable to allocate memory for kernel mapping data.",
				__func__));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto e0;
	}

	_MapPage(psWrapData,
	         ui32PageIndex,
	         psKernMapData);

	if (psKernMapData->pvKernLinAddr == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Unable to map wrapped extmem page.",
				__func__));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto e1;
	}

	*ppvKernelAddressOut = ((IMG_CHAR *) psKernMapData->pvKernLinAddr) + ui32PageOffset;
	*phHandleOut = psKernMapData;

	return PVRSRV_OK;

	/* error exit paths follow */
e1:
	OSFreeMem(psKernMapData);
e0:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

static void PMRReleaseKernelMappingDataExtMem(PMR_IMPL_PRIVDATA pvPriv,
                                              IMG_HANDLE hHandle)
{
	PMR_WRAP_DATA *psWrapData = (PMR_WRAP_DATA*) pvPriv;
	WRAP_KERNEL_MAP_DATA *psKernMapData = (void*) hHandle;

	_UnmapPage(psWrapData,
	           psKernMapData);

	OSFreeMem(psKernMapData);
}

static inline void begin_user_mode_access(IMG_UINT *uiState)
{
#if defined(CONFIG_ARM) && defined(CONFIG_CPU_SW_DOMAIN_PAN)
	*uiState = uaccess_save_and_enable();
#elif defined(CONFIG_ARM64) && defined(CONFIG_ARM64_SW_TTBR0_PAN)
	PVR_UNREFERENCED_PARAMETER(uiState);
	uaccess_enable_privileged();
#elif defined(CONFIG_X86)
	PVR_UNREFERENCED_PARAMETER(uiState);
	__uaccess_begin();
#else
	PVR_UNREFERENCED_PARAMETER(uiState);
#endif

}

static inline void end_user_mode_access(IMG_UINT uiState)
{
#if defined(CONFIG_ARM) && defined(CONFIG_CPU_SW_DOMAIN_PAN)
	uaccess_restore(uiState);
#elif defined(CONFIG_ARM64) && defined(CONFIG_ARM64_SW_TTBR0_PAN)
	PVR_UNREFERENCED_PARAMETER(uiState);
	uaccess_disable_privileged();
#elif defined(CONFIG_X86)
	PVR_UNREFERENCED_PARAMETER(uiState);
	__uaccess_end();
#else
	PVR_UNREFERENCED_PARAMETER(uiState);
#endif
}

static PVRSRV_ERROR _FlushUMVirtualRange(PVRSRV_DEVICE_NODE *psDevNode,
							PMR_WRAP_DATA *psPrivData,
							IMG_DEVMEM_SIZE_T uiSize,
							IMG_CPU_VIRTADDR pvCpuVAddr)
{
	struct vm_area_struct *psVMArea;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT	uiUserAccessState=0;

	mmap_read_lock(current->mm);

	/* Check that the recorded psVMArea matches the one associated with
	 * this request. If not, fail the request.
	 */
	psVMArea = find_vma(current->mm, (uintptr_t)pvCpuVAddr);
	if ((psVMArea != psPrivData->psVMArea) || (psVMArea == NULL))
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Couldn't find memory region containing start address %p",
		         __func__, (void *)pvCpuVAddr));
		eError = PVRSRV_ERROR_INVALID_CPU_ADDR;
		goto UMFlushUnlockReturn;
	}


	/*
	 * Latest kernels enable "Privileged access never" feature in the kernel
	 * This features leads to a privilege access failure fault (oops) whenever
	 * the kernel tries to access any user mode address.
	 * .
	 * This is an additional security feature that allows kernel
	 * to be protected against possible software attacks.
	 * In this case, as we need to flush the cache lines associated
	 * with the user mode virtual address, we need to explicitly disable
	 * the feature for the duration of access and re-enable it once done.
	 * */
	begin_user_mode_access(&uiUserAccessState);
	{
		if (OSCPUCacheOpAddressType(psDevNode) == OS_CACHE_OP_ADDR_TYPE_VIRTUAL)
		{
			IMG_CPU_PHYADDR sCPUPhysStart = {0};

			eError = CacheOpExec(psDevNode,
								pvCpuVAddr,
								((IMG_UINT8 *)pvCpuVAddr + uiSize),
								sCPUPhysStart,
								sCPUPhysStart,
								PVRSRV_CACHE_OP_FLUSH);
		}
		else if (OSCPUCacheOpAddressType(psDevNode) == OS_CACHE_OP_ADDR_TYPE_PHYSICAL)
		{
			IMG_CPU_PHYADDR sCPUPhysStart, sCPUPhysEnd;
			IMG_UINT i = 0;

			for (i = 0; i < psPrivData->uiTotalNumPages; i++)
			{
				if (NULL != psPrivData->ppsPageArray[i])
				{
					sCPUPhysStart.uiAddr = psPrivData->ppvPhysAddr[i].uiAddr;
					sCPUPhysEnd.uiAddr = sCPUPhysStart.uiAddr + PAGE_SIZE;

					eError = CacheOpExec(psDevNode,
										NULL,
										NULL,
										sCPUPhysStart,
										sCPUPhysEnd,
										PVRSRV_CACHE_OP_FLUSH);
					if (eError != PVRSRV_OK)
					{
						break;
					}
				}
			}
		}
		else if (OSCPUCacheOpAddressType(psDevNode) == OS_CACHE_OP_ADDR_TYPE_BOTH)
		{
			IMG_CPU_PHYADDR sCPUPhysStart, sCPUPhysEnd;
			void *pvVirtStart, *pvVirtEnd;
			IMG_UINT i = 0;

			for (i = 0; i < psPrivData->uiTotalNumPages; i++)
			{
				if (NULL != psPrivData->ppsPageArray[i])
				{
					pvVirtStart = pvCpuVAddr + (i * PAGE_SIZE);
					pvVirtEnd = pvVirtStart + PAGE_SIZE;
					sCPUPhysStart.uiAddr = psPrivData->ppvPhysAddr[i].uiAddr;
					sCPUPhysEnd.uiAddr = sCPUPhysStart.uiAddr + PAGE_SIZE;

					eError = CacheOpExec(psDevNode,
										pvVirtStart,
										pvVirtEnd,
										sCPUPhysStart,
										sCPUPhysEnd,
										PVRSRV_CACHE_OP_FLUSH);
					if (eError != PVRSRV_OK)
					{
						break;
					}
				}
			}
		}

		if (PVRSRV_OK != eError)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Failed to clean the virtual region cache %p",
					__func__,
					pvCpuVAddr));
			goto UMFlushFailed;
		}
	}

UMFlushFailed:
	end_user_mode_access(uiUserAccessState);

UMFlushUnlockReturn:
	mmap_read_unlock(current->mm);
	return eError;
}

static PMR_IMPL_FUNCTAB _sPMRWrapPFuncTab = {
    .pfnLockPhysAddresses = NULL,
    .pfnUnlockPhysAddresses = NULL,
    .pfnDevPhysAddr = &PMRSysPhysAddrExtMem,
    .pfnAcquireKernelMappingData = PMRAcquireKernelMappingDataExtMem,
    .pfnReleaseKernelMappingData = PMRReleaseKernelMappingDataExtMem,
    .pfnReadBytes = PMRReadBytesExtMem,
    .pfnWriteBytes = PMRWriteBytesExtMem,
    .pfnChangeSparseMem = NULL,
    .pfnFinalize = &PMRFinalizeExtMem,
};

static inline PVRSRV_ERROR PhysmemValidateParam( IMG_DEVMEM_SIZE_T uiSize,
                                                 IMG_CPU_VIRTADDR pvCpuVAddr,
                                                 PVRSRV_MEMALLOCFLAGS_T uiFlags)
{
	if (!access_ok(pvCpuVAddr, uiSize))
	{
		PVR_DPF((PVR_DBG_ERROR, "Invalid User mode CPU virtual address"));
		return PVRSRV_ERROR_INVALID_CPU_ADDR;
	}

	/* Fail if requesting coherency on one side but uncached on the other */
	if ((PVRSRV_CHECK_CPU_CACHE_COHERENT(uiFlags) &&
			(PVRSRV_CHECK_GPU_UNCACHED(uiFlags) || PVRSRV_CHECK_GPU_WRITE_COMBINE(uiFlags))))
	{
		PVR_DPF((PVR_DBG_ERROR, "Request for CPU coherency but specifying GPU uncached "
				"Please use GPU cached flags for coherency."));
		return PVRSRV_ERROR_UNSUPPORTED_CACHE_MODE;
	}

	if ((PVRSRV_CHECK_GPU_CACHE_COHERENT(uiFlags) &&
			(PVRSRV_CHECK_CPU_UNCACHED(uiFlags) || PVRSRV_CHECK_CPU_WRITE_COMBINE(uiFlags))))
	{
		PVR_DPF((PVR_DBG_ERROR, "Request for GPU coherency but specifying CPU uncached "
				"Please use CPU cached flags for coherency."));
		return PVRSRV_ERROR_UNSUPPORTED_CACHE_MODE;
	}

#if !defined(PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE)
	if (uiFlags & (PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE |
			PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
			PVRSRV_MEMALLOCFLAG_CPU_WRITE_PERMITTED |
			PVRSRV_MEMALLOCFLAG_GPU_WRITE_PERMITTED |
			PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC |
			PVRSRV_MEMALLOCFLAG_POISON_ON_ALLOC
#if defined(DEBUG)
			| PVRSRV_MEMALLOCFLAG_POISON_ON_FREE
#endif
		))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Write Attribute not supported. Passed Flags: 0x%"PVRSRV_MEMALLOCFLAGS_FMTSPEC,
				__func__, uiFlags));
		return PVRSRV_ERROR_INVALID_FLAGS;
	}
#endif

	/* Check address and size alignment */
	if (uiSize & (PAGE_SIZE - 1))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Given size %llu is not multiple of OS page size (%lu)",
				__func__,
				uiSize,
				PAGE_SIZE));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (((uintptr_t) pvCpuVAddr) & (PAGE_SIZE - 1))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Given address %p is not aligned to OS page size (%lu)",
				__func__,
				pvCpuVAddr,
				PAGE_SIZE));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR
PhysmemWrapExtMemOS(CONNECTION_DATA * psConnection,
                    PVRSRV_DEVICE_NODE *psDevNode,
                    IMG_DEVMEM_SIZE_T uiSize,
                    IMG_CPU_VIRTADDR pvCpuVAddr,
                    PVRSRV_MEMALLOCFLAGS_T uiFlags,
                    PMR **ppsPMRPtr)
{
	PVRSRV_ERROR eError;
	IMG_UINT32	*pui32MappingTable = NULL;
	PMR_WRAP_DATA *psPrivData;
	PMR *psPMR;
	IMG_UINT uiTotalNumPages = (uiSize >> PAGE_SHIFT);
	IMG_UINT i = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
	/* Ignore the most significant byte. */
	pvCpuVAddr = (IMG_CPU_VIRTADDR)untagged_addr((uintptr_t)pvCpuVAddr);
#endif

	eError = PhysmemValidateParam(uiSize,
	                              pvCpuVAddr,
	                              uiFlags);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	/* Allocate private factory data */
	eError = _AllocWrapData(&psPrivData,
	                        psDevNode,
	                        uiSize,
	                        pvCpuVAddr,
	                        uiFlags);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	/* Actually find and acquire the pages and physical addresses */
	eError = _WrapExtMemAcquirePages(psDevNode,
	                                 uiSize,
	                                 pvCpuVAddr,
	                                 psPrivData);
	if (eError != PVRSRV_OK)
	{
		_FreeWrapData(psPrivData);
		goto e0;
	}


	pui32MappingTable = OSAllocMem(sizeof(*pui32MappingTable) * uiTotalNumPages);
	if (pui32MappingTable == NULL)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto e1;
	}

	for (i = 0; i < uiTotalNumPages; i++)
	{
		pui32MappingTable[i] = i;
	}

	/* Avoid creating scratch page or zero page when the entire
	 * allocation is backed and pinned */
	if (psPrivData->uiNumBackedPages == psPrivData->uiTotalNumPages)
	{
		uiFlags |= PVRSRV_MEMALLOCFLAG_SPARSE_NO_SCRATCH_BACKING;
	}

	/* Create a suitable PMR */
	eError = PMRCreatePMR(psDevNode->apsPhysHeap[PVRSRV_PHYS_HEAP_CPU_LOCAL],
	                      uiSize,    /* PMR_SIZE_T uiLogicalSize                  */
	                      uiTotalNumPages,    /* IMG_UINT32 ui32NumPhysChunks              */
	                      uiTotalNumPages,    /* IMG_UINT32 ui32NumVirtChunks              */
	                      pui32MappingTable,
	                      PAGE_SHIFT,           /* PMR_LOG2ALIGN_T uiLog2ContiguityGuarantee */
	                      (uiFlags & PVRSRV_MEMALLOCFLAGS_PMRFLAGSMASK), /* PMR_FLAGS_T uiFlags */
	                      "WrappedExtMem",      /* const IMG_CHAR *pszAnnotation             */
	                      &_sPMRWrapPFuncTab,   /* const PMR_IMPL_FUNCTAB *psFuncTab         */
	                      psPrivData,           /* PMR_IMPL_PRIVDATA pvPrivData              */
	                      PMR_TYPE_EXTMEM,
	                      &psPMR,               /* PMR **ppsPMRPtr                           */
	                      PDUMP_NONE);          /* IMG_UINT32 ui32PDumpFlags                 */
	if (eError != PVRSRV_OK)
	{
		goto e2;
	}

	if (PVRSRV_CHECK_CPU_CACHE_CLEAN(uiFlags))
	{
		eError = _FlushUMVirtualRange(psDevNode,
									psPrivData,
									uiSize,
									pvCpuVAddr);
		if (eError != PVRSRV_OK)
		{
			goto e3;
		}
	}

	/* Mark the PMR such that no layout changes can happen.
	 * The memory is allocated in the CPU domain and hence
	 * no changes can be made through any of our API */
	PMR_SetLayoutFixed(psPMR, IMG_TRUE);

	*ppsPMRPtr = psPMR;

	OSFreeMem(pui32MappingTable);

	return PVRSRV_OK;
e3:
	PMRUnrefPMR(psPMR);
e2:
	OSFreeMem(pui32MappingTable);
e1:
	_WrapExtMemReleasePages(psPrivData);
e0:
	return eError;
}

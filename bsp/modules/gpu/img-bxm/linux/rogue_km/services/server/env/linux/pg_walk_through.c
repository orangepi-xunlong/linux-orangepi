/*************************************************************************/ /*!
@File			pg_walk_through.c
@Title			Non Services allocated memory wrap functionality support
@Copyright		Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description	Server-side component to support wrapping non-services
				allocated memory, particularly by browsing through the
				corresponding allocation host CPU map page table entries.
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

#if defined(SUPPORT_LINUX_WRAP_EXTMEM_PAGE_TABLE_WALK)

#include "physmem_extmem_wrap.h"
#include "kernel_compatibility.h"

/* Find the PFN associated with a given CPU virtual address, and return the
 * associated page structure, if it exists.
 * The page in question must be present (i.e. no fault handling required),
 * and must be writable. A get_page is done on the returned page structure.
 */
static IMG_BOOL _CPUVAddrToPFN(struct vm_area_struct *psVMArea,
                              uintptr_t uCPUVAddr,
                              unsigned long *pui32PFN,
                              struct page **ppsPage)
{
	pgd_t *psPGD;
	p4d_t *psP4D;
	pud_t *psPUD;
	pmd_t *psPMD;
	pte_t *psPTE;
	struct mm_struct *psMM = psVMArea->vm_mm;
	spinlock_t *psPTLock;
	IMG_BOOL bRet = IMG_FALSE;

	*pui32PFN = 0;
	*ppsPage = NULL;

	/* Walk the page tables to find the PTE */
	psPGD = pgd_offset(psMM, uCPUVAddr);
	if (pgd_none(*psPGD) || pgd_bad(*psPGD))
		return bRet;

	psP4D = p4d_offset(psPGD, uCPUVAddr);
	if (p4d_none(*psP4D) || unlikely(p4d_bad(*psP4D)))
		return bRet;

	psPUD = pud_offset(psP4D, uCPUVAddr);
	if (pud_none(*psPUD) || pud_bad(*psPUD))
		return bRet;

	psPMD = pmd_offset(psPUD, uCPUVAddr);
	if (pmd_none(*psPMD) || pmd_bad(*psPMD))
		return bRet;

	psPTE = (pte_t *)pte_offset_map_lock(psMM, psPMD, uCPUVAddr, &psPTLock);

	/* Check if the returned PTE is actually valid and writable */
	if ((pte_none(*psPTE) == 0) && (pte_present(*psPTE) != 0) && (pte_write(*psPTE) != 0))
	{
		*pui32PFN = pte_pfn(*psPTE);
		bRet = IMG_TRUE;

		/* In case the pfn is valid, meaning it is a RAM page and not
		 * IO-remapped, we can get the actual page struct from it.
		 */
		if (pfn_valid(*pui32PFN))
		{
			*ppsPage = pfn_to_page(*pui32PFN);

			get_page(*ppsPage);
		}
	}

	pte_unmap_unlock(psPTE, psPTLock);

	return bRet;
}

/* Find the VMA to a given CPU virtual address and do a page table walk
 * to find the corresponding pfns
 */
PVRSRV_ERROR _TryFindVMA(IMG_DEVMEM_SIZE_T uiSize,
                         uintptr_t pvCpuVAddr,
                         PMR_WRAP_DATA *psPrivData)
{
	struct vm_area_struct *psVMArea;
	uintptr_t pvCpuVAddrEnd = pvCpuVAddr + uiSize;
	IMG_UINT32 i;
	uintptr_t uAddr;
	PVRSRV_ERROR eError = PVRSRV_OK;

	mmap_read_lock(current->mm);

	/* Find the VMA and check that it is the expected one for this VAddr */
	psVMArea = find_vma(current->mm, pvCpuVAddr);
	if ((psVMArea == NULL) || (psVMArea != psPrivData->psVMArea))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Couldn't find memory region containing start address %p",
				__func__,
				(void*) pvCpuVAddr));
		eError = PVRSRV_ERROR_INVALID_CPU_ADDR;
		goto eUnlockReturn;
	}

	/* Make sure that we've been given a valid end-address */
	if (pvCpuVAddrEnd >= psVMArea->vm_end)
	{
		PVR_DPF((PVR_DBG_ERROR,
		        "%s: End address %p is outside of the region returned by find_vma",
		        __func__, (void *) pvCpuVAddrEnd));
		eError = PVRSRV_ERROR_BAD_PARAM_SIZE;
		goto eUnlockReturn;
	}

	/* Does the region represent memory mapped I/O? */
	if (!(psVMArea->vm_flags & VM_IO))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Memory region does not represent memory mapped I/O (VMA flags: 0x%lx)",
				__func__,
				psVMArea->vm_flags));
		eError = PVRSRV_ERROR_INVALID_FLAGS;
		goto eUnlockReturn;
	}

	/* We require read and write access */
	if ((psVMArea->vm_flags & (VM_READ | VM_WRITE)) != (VM_READ | VM_WRITE))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: No read/write access to memory region (VMA flags: 0x%lx)",
				__func__,
				psVMArea->vm_flags));
		eError = PVRSRV_ERROR_INVALID_FLAGS;
		goto eUnlockReturn;
	}

	/* Do the actual page table walk and fill the private data arrays
	 * for page structs and physical addresses */
	for (uAddr = pvCpuVAddr, i = 0;
	     uAddr < pvCpuVAddrEnd;
	     uAddr += PAGE_SIZE, i++)
	{
		unsigned long ui32PFN = 0;

		PVR_ASSERT(i < psPrivData->uiTotalNumPages);

		if (!_CPUVAddrToPFN(psVMArea, uAddr, &ui32PFN, &psPrivData->ppsPageArray[i]))
		{
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Invalid CPU virtual address",
					__func__));
			eError = PVRSRV_ERROR_FAILED_TO_ACQUIRE_PAGES;
			goto eReleasePages;
		}

		psPrivData->ppvPhysAddr[i].uiAddr = IMG_CAST_TO_CPUPHYADDR_UINT(ui32PFN << PAGE_SHIFT);
		psPrivData->uiNumBackedPages += 1;

		if ((((IMG_UINT64) psPrivData->ppvPhysAddr[i].uiAddr) >> PAGE_SHIFT) != ui32PFN)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Page frame number out of range (%lu)",
					__func__,
					ui32PFN));
			eError = PVRSRV_ERROR_FAILED_TO_ACQUIRE_PAGES;
			goto eReleasePages;
		}
	}

	/* Check to confirm we saw every page */
	PVR_ASSERT(i == psPrivData->uiTotalNumPages);
	mmap_read_unlock(current->mm);
	return eError;

eReleasePages:
	for (; i != 0; i--)
	{
		if (psPrivData->ppsPageArray[i-1] != NULL)
		{
			put_page(psPrivData->ppsPageArray[i-1]);
		}
	}
eUnlockReturn:
	mmap_read_unlock(current->mm);
	return eError;
}
#endif

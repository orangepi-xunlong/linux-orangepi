/*************************************************************************/ /*!
@File           physmem_extmem_wrap.h
@Title          Header for wrapping non-services allocated memory PMR factory.
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of the memory management. This module is responsible for
                implementing the support for wrapping non services allocated
                memory into GPU space.
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

#ifndef SRVSRV_PHYSMEM_EXTEMEM_WRAP_H
#define SRVSRV_PHYSMEM_EXTEMEM_WRAP_H

#include <linux/version.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/mm_types.h>
#include <linux/vmalloc.h>
#include <linux/gfp.h>
#include <linux/sched.h>
#include <linux/atomic.h>

#include "img_types.h"
#include "device.h"

typedef enum _WRAP_EXT_MEM_TYPE_
{
	WRAP_TYPE_NULL = 0,
	WRAP_TYPE_GET_USER_PAGES,
#if defined(SUPPORT_LINUX_WRAP_EXTMEM_PAGE_TABLE_WALK)
	WRAP_TYPE_FIND_VMA
#endif
} WRAP_EXT_MEM_TYPE;

typedef struct _PMR_WRAP_DATA_
{
	/* Device for which this allocation has been made */
	PVRSRV_DEVICE_NODE *psDevNode;

	/* Total Number of pages in the allocation */
	IMG_UINT32 uiTotalNumPages;

	/* Total number of actual physically backed pages */
	IMG_UINT32 uiNumBackedPages;

	/* This is only filled if we have page mappings,
	 * for pfn mappings this stays empty */
	struct page **ppsPageArray;

	/* VM Area structure */
	struct vm_area_struct *psVMArea;

	/* This should always be filled and hold the physical addresses */
	IMG_CPU_PHYADDR *ppvPhysAddr;

	/* Did we find the pages via get_user_pages()
	 * or via FindVMA and a page table walk? */
	WRAP_EXT_MEM_TYPE eWrapType;

	/* CPU caching modes for the PMR mapping */
	IMG_UINT32 ui32CPUCacheFlags;

	/* Write permitted ?*/
	IMG_BOOL bWrite;

} PMR_WRAP_DATA;

#if defined(SUPPORT_LINUX_WRAP_EXTMEM_PAGE_TABLE_WALK)
/* Find the VMA to a given CPU virtual address and do a
 * page table walk to find the corresponding pfns */
PVRSRV_ERROR _TryFindVMA(IMG_DEVMEM_SIZE_T uiSize,
                            uintptr_t pvCpuVAddr,
                            PMR_WRAP_DATA *psPrivData);
#endif

#endif /* SRVSRV_PHYSMEM_EXTEMEM_WRAP_H */

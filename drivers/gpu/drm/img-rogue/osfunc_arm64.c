/*************************************************************************/ /*!
@File
@Title          arm64 specific OS functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Processor specific OS functions
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
#include <linux/cpumask.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
 #include <linux/dma-map-ops.h>
#else
 #include <linux/dma-mapping.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) */
#include <asm/cacheflush.h>
#include <linux/uaccess.h>

#include "pvrsrv_error.h"
#include "img_types.h"
#include "img_defs.h"
#include "osfunc.h"
#include "pvr_debug.h"

#include "kernel_compatibility.h"

#if defined(CONFIG_OUTER_CACHE)
  /* If you encounter a 64-bit ARM system with an outer cache, you'll need
   * to add the necessary code to manage that cache. See osfunc_arm.c
   * for an example of how to do so.
   */
	#error "CONFIG_OUTER_CACHE not supported on arm64."
#endif

static inline void begin_user_mode_access(void)
{
#if defined(CONFIG_ARM64) && defined(CONFIG_ARM64_SW_TTBR0_PAN)
	uaccess_enable_privileged();
#endif
}

static inline void end_user_mode_access(void)
{
#if defined(CONFIG_ARM64) && defined(CONFIG_ARM64_SW_TTBR0_PAN)
	uaccess_disable_privileged();
#endif
}

static inline void FlushRange(void *pvRangeAddrStart,
							  void *pvRangeAddrEnd,
							  PVRSRV_CACHE_OP eCacheOp)
{
	IMG_UINT32 ui32CacheLineSize = OSCPUCacheAttributeSize(OS_CPU_CACHE_ATTRIBUTE_LINE_SIZE);
	IMG_BYTE *pbStart = pvRangeAddrStart;
	IMG_BYTE *pbEnd = pvRangeAddrEnd;
	IMG_BYTE *pbBase;

	/*
	  On arm64, the TRM states in D5.8.1 (data and unified caches) that if cache
	  maintenance is performed on a memory location using a VA, the effect of
	  that cache maintenance is visible to all VA aliases of the physical memory
	  location. So here it's quicker to issue the machine cache maintenance
	  instruction directly without going via the Linux kernel DMA framework as
	  this is sufficient to maintain the CPU d-caches on arm64.
	 */

	begin_user_mode_access();

	pbEnd = (IMG_BYTE *) PVR_ALIGN((uintptr_t)pbEnd, (uintptr_t)ui32CacheLineSize);

	/* Memory-barrier */
	asm volatile("dsb sy" : : : "memory");

	switch (eCacheOp)
	{
		case PVRSRV_CACHE_OP_CLEAN:
			for (pbBase = pbStart; pbBase < pbEnd; pbBase += ui32CacheLineSize)
			{
				asm volatile ("dc cvac, %0" :: "r" (pbBase));
			}
			break;
		case PVRSRV_CACHE_OP_INVALIDATE:
		case PVRSRV_CACHE_OP_FLUSH:
			for (pbBase = pbStart; pbBase < pbEnd; pbBase += ui32CacheLineSize)
			{
				asm volatile ("dc civac, %0" :: "r" (pbBase));
			}
			break;
		default:
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Cache maintenance operation type %d is invalid",
					__func__, eCacheOp));
			break;
	}

	/* Memory-barrier */
	asm volatile("dsb sy" : : : "memory");

	end_user_mode_access();
}

void OSCPUCacheFlushRangeKM(PVRSRV_DEVICE_NODE *psDevNode,
							void *pvVirtStart,
							void *pvVirtEnd,
							IMG_CPU_PHYADDR sCPUPhysStart,
							IMG_CPU_PHYADDR sCPUPhysEnd)
{
	struct device *dev;
	PVR_UNREFERENCED_PARAMETER(dev);

	if (pvVirtStart == NULL)
	{
		/*
		 * Converting the physical addresses to virtual addresses allows us to
		 * utilize the assembly instruction that makes the Flush + Invalidate
		 * cache operation atomic.
		 * There is no state in-between the flush and the invalidate operation.
		 */
		pvVirtStart = phys_to_virt(sCPUPhysStart.uiAddr);
		pvVirtEnd = phys_to_virt(sCPUPhysEnd.uiAddr);
	}

	FlushRange(pvVirtStart, pvVirtEnd, PVRSRV_CACHE_OP_FLUSH);
	return;

}

void OSCPUCacheCleanRangeKM(PVRSRV_DEVICE_NODE *psDevNode,
							void *pvVirtStart,
							void *pvVirtEnd,
							IMG_CPU_PHYADDR sCPUPhysStart,
							IMG_CPU_PHYADDR sCPUPhysEnd)
{
	struct device *dev;

	if (pvVirtStart)
	{
		FlushRange(pvVirtStart, pvVirtEnd, PVRSRV_CACHE_OP_CLEAN);
		return;
	}

	dev = psDevNode->psDevConfig->pvOSDevice;

	if (dev)
	{
		dma_sync_single_for_device(dev, sCPUPhysStart.uiAddr,
								   sCPUPhysEnd.uiAddr - sCPUPhysStart.uiAddr,
								   DMA_TO_DEVICE);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "Cache operation cannot be completed!"));
	}

}

void OSCPUCacheInvalidateRangeKM(PVRSRV_DEVICE_NODE *psDevNode,
								 void *pvVirtStart,
								 void *pvVirtEnd,
								 IMG_CPU_PHYADDR sCPUPhysStart,
								 IMG_CPU_PHYADDR sCPUPhysEnd)
{
	struct device *dev;

	if (pvVirtStart)
	{
		FlushRange(pvVirtStart, pvVirtEnd, PVRSRV_CACHE_OP_INVALIDATE);
		return;
	}

	dev = psDevNode->psDevConfig->pvOSDevice;

	if (dev)
	{
		dma_sync_single_for_cpu(dev, sCPUPhysStart.uiAddr,
								sCPUPhysEnd.uiAddr - sCPUPhysStart.uiAddr,
								DMA_FROM_DEVICE);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "Cache operation cannot be completed!"));
	}
}


OS_CACHE_OP_ADDR_TYPE OSCPUCacheOpAddressType(PVRSRV_DEVICE_NODE *psDevNode)
{
	if (!psDevNode->psDevConfig->pvOSDevice)
	{
		/* Host Mem device node doesn't have an associated Linux dev ptr.
		   Use virtual addr ops instead of asking kernel to do physical
		   maintenance */
		return OS_CACHE_OP_ADDR_TYPE_VIRTUAL;
	}

	return OS_CACHE_OP_ADDR_TYPE_PHYSICAL;
}

void OSUserModeAccessToPerfCountersEn(void)
{
}

IMG_BOOL OSIsWriteCombineUnalignedSafe(void)
{
	/*
	 * Under ARM64 there is the concept of 'device' [0] and 'normal' [1] memory.
	 * Unaligned access on device memory is explicitly disallowed [2]:
	 *
	 * 'Further, unaligned accesses are only allowed to regions marked as Normal
	 *  memory type.
	 *  ...
	 *  Attempts to perform unaligned accesses when not allowed will cause an
	 *  alignment fault (data abort).'
	 *
	 * Write-combine on ARM64 can be implemented as either normal non-cached
	 * memory (NORMAL_NC) or as device memory with gathering enabled
	 * (DEVICE_GRE.) Kernel 3.13 changed this from the latter to the former.
	 *
	 * [0]:http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/CHDBDIDF.html
	 * [1]:http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.den0024a/ch13s01s01.html
	 * [2]:http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka15414.html
	 */

	pgprot_t pgprot = pgprot_writecombine(PAGE_KERNEL);

	return (pgprot_val(pgprot) & PTE_ATTRINDX_MASK) == PTE_ATTRINDX(MT_NORMAL_NC);
}

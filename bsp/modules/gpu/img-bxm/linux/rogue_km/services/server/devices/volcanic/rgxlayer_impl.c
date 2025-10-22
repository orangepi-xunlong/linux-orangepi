/*************************************************************************/ /*!
@File
@Title          DDK implementation of the Services abstraction layer
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    DDK implementation of the Services abstraction layer
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

#include "rgxlayer_impl.h"
#include "osfunc.h"
#include "pdump_km.h"
#include "rgxfwutils.h"
#include "rgxinit.h"
#include "rgxfwimageutils.h"
#include "cache_km.h"
#include "km/rgxdefs_km.h"
#include "rgx_heaps_server.h"

#if defined(PDUMP)
#if defined(__linux__)
 #include <linux/version.h>

 #if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
  #include <linux/stdarg.h>
 #else
  #include <stdarg.h>
 #endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) */
#else
 #include <stdarg.h>
#endif /* __linux__ */
#endif


#define MAX_NUM_COHERENCY_TESTS  (10)
IMG_BOOL RGXDoFWSlaveBoot(const void *hPrivate)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	if (psDevInfo->ui32CoherencyTestsDone >= MAX_NUM_COHERENCY_TESTS)
	{
		return IMG_FALSE;
	}

	return IMG_FALSE;
}

/*
 * The fabric coherency test is performed when platform supports fabric coherency
 * either in the form of ACE-lite or Full-ACE. This test is done quite early
 * with the firmware processor quiescent and makes exclusive use of the slave
 * port interface for reading/writing through the device memory hierarchy. The
 * rationale for the test is to ensure that what the CPU writes to its dcache
 * is visible to the GPU via coherency snoop miss/hit and vice-versa without
 * any intervening cache maintenance by the writing agent.
 */
PVRSRV_ERROR RGXFabricCoherencyTest(const void *hPrivate)
{
	PVRSRV_RGXDEV_INFO *psDevInfo;
	IMG_UINT32 *pui32FabricCohTestBufferCpuVA = NULL;
	IMG_UINT32 *pui32FabricCohCcTestBufferCpuVA = NULL;
	IMG_UINT32 *pui32FabricCohNcTestBufferCpuVA = NULL;
	DEVMEM_MEMDESC *psFabricCohTestBufferMemDesc = NULL;
	DEVMEM_MEMDESC *psFabricCohCcTestBufferMemDesc = NULL;
	DEVMEM_MEMDESC *psFabricCohNcTestBufferMemDesc = NULL;
	RGXFWIF_DEV_VIRTADDR sFabricCohCcTestBufferDevVA;
	RGXFWIF_DEV_VIRTADDR sFabricCohNcTestBufferDevVA;
	RGXFWIF_DEV_VIRTADDR *psFabricCohTestBufferDevVA = NULL;
	IMG_DEVMEM_SIZE_T uiFabricCohTestBlockSize = sizeof(IMG_UINT64);
	IMG_DEVMEM_ALIGN_T uiFabricCohTestBlockAlign = sizeof(IMG_UINT64);
	IMG_UINT64 ui64SegOutAddrTopCached = 0;
	IMG_UINT64 ui64SegOutAddrTopUncached = 0;
	IMG_UINT32 ui32OddEven;
	IMG_UINT32 ui32OddEvenSeed = 1;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_BOOL bFullTestPassed = IMG_TRUE;
	IMG_BOOL bExit = IMG_FALSE;
#if defined(DEBUG)
	IMG_BOOL bSubTestPassed = IMG_FALSE;
#endif
	enum TEST_TYPE {
		CPU_WRITE_GPU_READ_SM=0, GPU_WRITE_CPU_READ_SM,
		CPU_WRITE_GPU_READ_SH,   GPU_WRITE_CPU_READ_SH
	} eTestType;

	PVR_ASSERT(hPrivate != NULL);
	psDevInfo = ((RGX_LAYER_PARAMS*)hPrivate)->psDevInfo;

	PVR_LOG(("Starting fabric coherency test ....."));

	/* Size and align are 'expanded' because we request an export align allocation */
	eError = DevmemExportalignAdjustSizeAndAlign(DevmemGetHeapLog2PageSize(psDevInfo->psFirmwareMainHeap),
	                                             &uiFabricCohTestBlockSize,
	                                             &uiFabricCohTestBlockAlign);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"DevmemExportalignAdjustSizeAndAlign() error: %s, exiting",
				PVRSRVGetErrorString(eError)));
		goto e0;
	}

	/* Allocate, acquire cpu address and set firmware address for cc=1 buffer */
	eError = DevmemFwAllocateExportable(psDevInfo->psDeviceNode,
										uiFabricCohTestBlockSize,
										uiFabricCohTestBlockAlign,
										PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) |
										PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE |
										PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC |
										PVRSRV_MEMALLOCFLAG_GPU_CACHE_COHERENT |
										PVRSRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT |
										PVRSRV_MEMALLOCFLAG_GPU_READABLE |
										PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE |
										PVRSRV_MEMALLOCFLAG_CPU_READABLE |
										PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
										PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN),
										"FwExFabricCoherencyCcTestBuffer",
										&psFabricCohCcTestBufferMemDesc);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"DevmemFwAllocateExportable() error: %s, exiting",
				PVRSRVGetErrorString(eError)));
		goto e0;
	}

	eError = DevmemAcquireCpuVirtAddr(psFabricCohCcTestBufferMemDesc, (void **) &pui32FabricCohCcTestBufferCpuVA);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"DevmemAcquireCpuVirtAddr() error: %s, exiting",
				PVRSRVGetErrorString(eError)));
		goto e1;
	}

	/* Create a FW address which is uncached in the Meta DCache and in the SLC using the Meta bootloader segment.
	   This segment is the only one configured correctly out of reset (when this test is meant to be executed) */
	eError = RGXSetFirmwareAddress(&sFabricCohCcTestBufferDevVA,
						  psFabricCohCcTestBufferMemDesc,
						  0,
						  RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:1", e2);

	/* Undo most of the FW mappings done by RGXSetFirmwareAddress */
	sFabricCohCcTestBufferDevVA.ui32Addr &= ~RGXFW_SEGMMU_DATA_META_CACHE_MASK;
	sFabricCohCcTestBufferDevVA.ui32Addr &= ~RGXFW_SEGMMU_DATA_VIVT_SLC_CACHE_MASK;
	sFabricCohCcTestBufferDevVA.ui32Addr -= RGXFW_SEGMMU_DATA_BASE_ADDRESS;

	/* Map the buffer in the bootloader segment as uncached */
	sFabricCohCcTestBufferDevVA.ui32Addr |= RGXFW_BOOTLDR_META_ADDR;
	sFabricCohCcTestBufferDevVA.ui32Addr |= RGXFW_SEGMMU_DATA_META_UNCACHED;

	/* Allocate, acquire cpu address and set firmware address for cc=0 buffer */
	eError = DevmemFwAllocateExportable(psDevInfo->psDeviceNode,
										uiFabricCohTestBlockSize,
										uiFabricCohTestBlockAlign,
										PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) |
										PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE |
										PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC |
										PVRSRV_MEMALLOCFLAG_GPU_CACHE_INCOHERENT |
										PVRSRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT |
										PVRSRV_MEMALLOCFLAG_GPU_READABLE |
										PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE |
										PVRSRV_MEMALLOCFLAG_CPU_READABLE |
										PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE |
										PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN),
										"FwExFabricCoherencyNcTestBuffer",
										&psFabricCohNcTestBufferMemDesc);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"DevmemFwAllocateExportable() error: %s, exiting",
				PVRSRVGetErrorString(eError)));
		goto e3;
	}

	eError = DevmemAcquireCpuVirtAddr(psFabricCohNcTestBufferMemDesc, (void **) &pui32FabricCohNcTestBufferCpuVA);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"DevmemAcquireCpuVirtAddr() error: %s, exiting",
				PVRSRVGetErrorString(eError)));
		goto e4;
	}

	eError = RGXSetFirmwareAddress(&sFabricCohNcTestBufferDevVA,
						  psFabricCohNcTestBufferMemDesc,
						  0,
						  RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:2", e5);

	/* Undo most of the FW mappings done by RGXSetFirmwareAddress */
	sFabricCohNcTestBufferDevVA.ui32Addr &= ~RGXFW_SEGMMU_DATA_META_CACHE_MASK;
	sFabricCohNcTestBufferDevVA.ui32Addr &= ~RGXFW_SEGMMU_DATA_VIVT_SLC_CACHE_MASK;
	sFabricCohNcTestBufferDevVA.ui32Addr -= RGXFW_SEGMMU_DATA_BASE_ADDRESS;

	/* Map the buffer in the bootloader segment as uncached */
	sFabricCohNcTestBufferDevVA.ui32Addr |= RGXFW_BOOTLDR_META_ADDR;
	sFabricCohNcTestBufferDevVA.ui32Addr |= RGXFW_SEGMMU_DATA_META_UNCACHED;

	/* Obtain the META segment addresses corresponding to cached and uncached windows into SLC */
	ui64SegOutAddrTopCached   = RGXFW_SEGMMU_OUTADDR_TOP_VIVT_SLC_CACHED(MMU_CONTEXT_MAPPING_FWIF);
	ui64SegOutAddrTopUncached = RGXFW_SEGMMU_OUTADDR_TOP_VIVT_SLC_UNCACHED(MMU_CONTEXT_MAPPING_FWIF);

	/* At the top level, we perform snoop-miss (i.e. to verify slave port) & snoop-hit (i.e. to verify ACE) test.
	   NOTE: For now, skip snoop-miss test as Services currently forces all firmware allocations to be coherent */
	for (eTestType = CPU_WRITE_GPU_READ_SH; eTestType <= GPU_WRITE_CPU_READ_SH && bExit == IMG_FALSE; eTestType++)
	{
		IMG_CPU_PHYADDR sCpuPhyAddr;
		IMG_BOOL bValid;
		PMR *psPMR;

		if (eTestType == CPU_WRITE_GPU_READ_SM)
		{
			/* All snoop miss test must bypass the SLC, here memory is region of coherence so
			   configure META to use SLC bypass cache policy for the bootloader segment. Note
			   this cannot be done on a cache-coherent (i.e. CC=1) VA, as this violates ACE
			   standard as one cannot issue a non-coherent request into the bus fabric for
			   an allocation's VA that is cache-coherent in SLC, so use non-coherent buffer */
			RGXWriteMetaRegThroughSP(hPrivate, META_CR_MMCU_SEGMENTn_OUTA1(6),
									(ui64SegOutAddrTopUncached |  RGXFW_BOOTLDR_DEVV_ADDR) >> 32);
			pui32FabricCohTestBufferCpuVA = pui32FabricCohNcTestBufferCpuVA;
			psFabricCohTestBufferMemDesc = psFabricCohNcTestBufferMemDesc;
			psFabricCohTestBufferDevVA = &sFabricCohNcTestBufferDevVA;
		}
		else if (eTestType == CPU_WRITE_GPU_READ_SH)
		{
			/* All snoop hit test must obviously use SLC, here SLC is region of coherence so
			   configure META not to bypass the SLC for the bootloader segment */
			RGXWriteMetaRegThroughSP(hPrivate, META_CR_MMCU_SEGMENTn_OUTA1(6),
									(ui64SegOutAddrTopCached |  RGXFW_BOOTLDR_DEVV_ADDR) >> 32);
			pui32FabricCohTestBufferCpuVA = pui32FabricCohCcTestBufferCpuVA;
			psFabricCohTestBufferMemDesc = psFabricCohCcTestBufferMemDesc;
			psFabricCohTestBufferDevVA = &sFabricCohCcTestBufferDevVA;
		}

		if (eTestType == GPU_WRITE_CPU_READ_SH)
		{
			/* Cannot perform this test if there is no snooping of device cache */
			continue;
		}

		/* Acquire underlying PMR CpuPA in preparation for cache maintenance */
		(void) DevmemLocalGetImportHandle(psFabricCohTestBufferMemDesc, (void**)&psPMR);
		eError = PMR_CpuPhysAddr(psPMR, OSGetPageShift(), 1, 0, &sCpuPhyAddr, &bValid);
		if (eError != PVRSRV_OK || bValid == IMG_FALSE)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"PMR_CpuPhysAddr error: %s, exiting",
					PVRSRVGetErrorString(eError)));
			bExit = IMG_TRUE;
			continue;
		}

		/* Here we do two passes mostly to account for the effects of using a different
		   seed (i.e. ui32OddEvenSeed) value to read and write */
		for (ui32OddEven = 1; ui32OddEven < 3 && bExit == IMG_FALSE; ui32OddEven++)
		{
			IMG_UINT32 i;

#if defined(DEBUG)
			switch (eTestType)
			{
			case CPU_WRITE_GPU_READ_SM:
				PVR_LOG(("CPU:Write/GPU:Read Snoop Miss Test: starting [run #%u]", ui32OddEven));
				break;
			case GPU_WRITE_CPU_READ_SM:
				PVR_LOG(("GPU:Write/CPU:Read Snoop Miss Test: starting [run #%u]", ui32OddEven));
				break;
			case CPU_WRITE_GPU_READ_SH:
				PVR_LOG(("CPU:Write/GPU:Read Snoop Hit  Test: starting [run #%u]", ui32OddEven));
				break;
			case GPU_WRITE_CPU_READ_SH:
				PVR_LOG(("GPU:Write/CPU:Read Snoop Hit  Test: starting [run #%u]", ui32OddEven));
				break;
			default:
				PVR_LOG(("Internal error, exiting test"));
				eError = PVRSRV_ERROR_INIT_FAILURE;
				bExit = IMG_TRUE;
				continue;
			}
#endif

			/* Do multiple sub-dword cache line tests */
			for (i = 0; i < 2 && bExit == IMG_FALSE; i++)
			{
				IMG_UINT32 ui32FWAddr;
				IMG_UINT32 ui32FWValue;
				IMG_UINT32 ui32FWValue2;
				IMG_UINT32 ui32LastFWValue = ~0;
				IMG_UINT32 ui32Offset = i * sizeof(IMG_UINT32);

				/* Calculate next address and seed value to write/read from slave-port */
				ui32FWAddr = psFabricCohTestBufferDevVA->ui32Addr + ui32Offset;
				ui32OddEvenSeed += 1;

				if (eTestType == GPU_WRITE_CPU_READ_SM || eTestType == GPU_WRITE_CPU_READ_SH)
				{
					/* Clean dcache to ensure there is no stale data in dcache that might over-write
					   what we are about to write via slave-port here because if it drains from the CPU
					   dcache before we read it, it would corrupt what we are going to read back via
					   the CPU */
					CacheOpValExec(psPMR, 0, ui32Offset, sizeof(IMG_UINT32), PVRSRV_CACHE_OP_FLUSH);

					/* Calculate a new value to write */
					ui32FWValue = i + ui32OddEvenSeed;

					/* Write the value using the RGX slave-port interface */
					eError = RGXWriteFWModuleAddr(psDevInfo, ui32FWAddr, ui32FWValue);
					if (eError != PVRSRV_OK)
					{
						PVR_DPF((PVR_DBG_ERROR,
						         "RGXWriteFWModuleAddr error: %s, exiting",
						          PVRSRVGetErrorString(eError)));
						bExit = IMG_TRUE;
						continue;
					}

					/* Read back value using RGX slave-port interface, this is used
					   as a sort of memory barrier for the above write */
					eError = RGXReadFWModuleAddr(psDevInfo, ui32FWAddr, &ui32FWValue2);
					if (eError != PVRSRV_OK)
					{
						PVR_DPF((PVR_DBG_ERROR,
						         "RGXReadFWModuleAddr error: %s, exiting",
						         PVRSRVGetErrorString(eError)));
						bExit = IMG_TRUE;
						continue;
					}
					else if (ui32FWValue != ui32FWValue2)
					{
						//IMG_UINT32 ui32FWValue3;
						//RGXReadFWModuleAddr(psDevInfo, 0xC1F00000, &ui32FWValue3);

						/* Fatal error, we should abort */
						PVR_DPF((PVR_DBG_ERROR,
								"At Offset: %d, RAW via SlavePort failed: expected: %x, got: %x",
								i,
								ui32FWValue,
								ui32FWValue2));
						eError = PVRSRV_ERROR_INIT_FAILURE;
						bExit = IMG_TRUE;
						continue;
					}

					/* Invalidate dcache to ensure that any prefetched data by the CPU from this memory
					   region is discarded before we read (i.e. next read must trigger a cache miss).
					   Previously there was snooping of device cache, where prefetching done by the CPU
					   would reflect the most up to date datum writing by GPU into said location,
					   that is to say prefetching was coherent so CPU d-flush was not needed */
					CacheOpValExec(psPMR, 0, ui32Offset, sizeof(IMG_UINT32), PVRSRV_CACHE_OP_INVALIDATE);
				}
				else
				{
					IMG_UINT32 ui32RAWCpuValue;

					/* Ensures line is in dcache */
					ui32FWValue = IMG_UINT32_MAX;

					/* Dirty allocation in dcache */
					ui32RAWCpuValue = i + ui32OddEvenSeed;
					pui32FabricCohTestBufferCpuVA[i] = i + ui32OddEvenSeed;

					/* Flush possible cpu store-buffer(ing) on LMA */
					OSWriteMemoryBarrier(&pui32FabricCohTestBufferCpuVA[i]);

					if (eTestType == CPU_WRITE_GPU_READ_SM)
					{
						/* Flush dcache to force subsequent incoming CPU-bound snoop to miss so
						   memory is coherent before the SlavePort reads */
						CacheOpValExec(psPMR, 0, ui32Offset, sizeof(IMG_UINT32), PVRSRV_CACHE_OP_FLUSH);
					}

					/* Read back value using RGX slave-port interface */
					eError = RGXReadFWModuleAddr(psDevInfo, ui32FWAddr, &ui32FWValue);
					if (eError != PVRSRV_OK)
					{
						PVR_DPF((PVR_DBG_ERROR,
								"RGXReadFWModuleAddr error: %s, exiting",
								PVRSRVGetErrorString(eError)));
						bExit = IMG_TRUE;
						continue;
					}

					/* Being mostly paranoid here, verify that CPU RAW operation is valid
					   after the above slave port read */
					CacheOpValExec(psPMR, 0, ui32Offset, sizeof(IMG_UINT32), PVRSRV_CACHE_OP_INVALIDATE);
					if (pui32FabricCohTestBufferCpuVA[i] != ui32RAWCpuValue)
					{
						/* Fatal error, we should abort */
						PVR_DPF((PVR_DBG_ERROR,
								"At Offset: %d, RAW by CPU failed: expected: %x, got: %x",
								i,
								ui32RAWCpuValue,
								pui32FabricCohTestBufferCpuVA[i]));
						eError = PVRSRV_ERROR_INIT_FAILURE;
						bExit = IMG_TRUE;
						continue;
					}
				}

				/* Compare to see if sub-test passed */
				if (pui32FabricCohTestBufferCpuVA[i] == ui32FWValue)
				{
#if defined(DEBUG)
					bSubTestPassed = IMG_TRUE;
#endif
				}
				else
				{
					bFullTestPassed = IMG_FALSE;
					eError = PVRSRV_ERROR_INIT_FAILURE;
#if defined(DEBUG)
					bSubTestPassed = IMG_FALSE;
#endif
					if (ui32LastFWValue != ui32FWValue)
					{
#if defined(DEBUG)
						PVR_LOG(("At Offset: %d, Expected: %x, Got: %x",
								 i,
								 (eTestType & 0x1) ? ui32FWValue : pui32FabricCohTestBufferCpuVA[i],
								 (eTestType & 0x1) ? pui32FabricCohTestBufferCpuVA[i] : ui32FWValue));
#endif
					}
					else
					{
						PVR_DPF((PVR_DBG_ERROR,
								"test encountered unexpected error, exiting"));
						eError = PVRSRV_ERROR_INIT_FAILURE;
						bExit = IMG_TRUE;
						continue;
					}
				}

				ui32LastFWValue = (eTestType & 0x1) ? ui32FWValue : pui32FabricCohTestBufferCpuVA[i];
			}

#if defined(DEBUG)
			if (bExit)
			{
				continue;
			}

			switch (eTestType)
			{
			case CPU_WRITE_GPU_READ_SM:
				PVR_LOG(("CPU:Write/GPU:Read Snoop Miss Test: completed [run #%u]: %s",
						 ui32OddEven, bSubTestPassed ? "PASSED" : "FAILED"));
				break;
			case GPU_WRITE_CPU_READ_SM:
				PVR_LOG(("GPU:Write/CPU:Read Snoop Miss Test: completed [run #%u]: %s",
						 ui32OddEven, bSubTestPassed ? "PASSED" : "FAILED"));
				break;
			case CPU_WRITE_GPU_READ_SH:
				PVR_LOG(("CPU:Write/GPU:Read Snoop Hit Test: completed [run #%u]: %s",
						 ui32OddEven, bSubTestPassed ? "PASSED" : "FAILED"));
				break;
			case GPU_WRITE_CPU_READ_SH:
				PVR_LOG(("GPU:Write/CPU:Read Snoop Hit Test: completed [run #%u]: %s",
						 ui32OddEven, bSubTestPassed ? "PASSED" : "FAILED"));
				break;
			default:
				PVR_LOG(("Internal error, exiting test"));
				eError = PVRSRV_ERROR_INIT_FAILURE;
				bExit = IMG_TRUE;
				continue;
			}
#endif
		}
	}

	/* Release and free NC/CC test buffers */
	RGXUnsetFirmwareAddress(psFabricCohCcTestBufferMemDesc);
e5:
	DevmemReleaseCpuVirtAddr(psFabricCohCcTestBufferMemDesc);
e4:
	DevmemFwUnmapAndFree(psDevInfo, psFabricCohCcTestBufferMemDesc);

e3:
	RGXUnsetFirmwareAddress(psFabricCohNcTestBufferMemDesc);
e2:
	DevmemReleaseCpuVirtAddr(psFabricCohNcTestBufferMemDesc);
e1:
	DevmemFwUnmapAndFree(psDevInfo, psFabricCohNcTestBufferMemDesc);

e0:
	/* Restore bootloader segment settings */
	RGXWriteMetaRegThroughSP(hPrivate, META_CR_MMCU_SEGMENTn_OUTA1(6),
	                         (ui64SegOutAddrTopCached | RGXFW_BOOTLDR_DEVV_ADDR) >> 32);

	bFullTestPassed = bExit ? IMG_FALSE: bFullTestPassed;
	if (bFullTestPassed)
	{
		PVR_LOG(("fabric coherency test: PASSED"));
		psDevInfo->ui32CoherencyTestsDone = MAX_NUM_COHERENCY_TESTS + 1;
	}
	else
	{
		PVR_LOG(("fabric coherency test: FAILED"));
		psDevInfo->ui32CoherencyTestsDone++;
	}

	return eError;
}

static IMG_UINT64 RGXMMUComputeRangeValue(IMG_UINT32 ui32DataPageShift, IMG_UINT64 ui64BaseAddress, IMG_UINT64 ui64RangeSize)
{
	/* end address of range is inclusive */
	IMG_UINT64 ui64EndAddress = ui64BaseAddress + ui64RangeSize - (1 << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_END_ADDR_ALIGNSHIFT);
	IMG_UINT64 ui64RegValue = 0;

	switch (ui32DataPageShift)
	{
		case RGX_HEAP_16KB_PAGE_SHIFT:
			ui64RegValue = 1;
			break;
		case RGX_HEAP_64KB_PAGE_SHIFT:
			ui64RegValue = 2;
			break;
		case RGX_HEAP_256KB_PAGE_SHIFT:
			ui64RegValue = 3;
			break;
		case RGX_HEAP_1MB_PAGE_SHIFT:
			ui64RegValue = 4;
			break;
		case RGX_HEAP_2MB_PAGE_SHIFT:
			ui64RegValue = 5;
			break;
		case RGX_HEAP_4KB_PAGE_SHIFT:
			/* fall through */
		default:
			/* anything we don't support, use 4K */
			break;
	}

	/* check that the range is defined by valid 40 bit virtual addresses */
	PVR_ASSERT((ui64BaseAddress & ~((1ULL << 40) - 1)) == 0);
	PVR_ASSERT((ui64EndAddress  & ~((1ULL << 40) - 1)) == 0);

	/* the range config register addresses are in 2MB chunks so check 21 lsb are zero */
	PVR_ASSERT((ui64BaseAddress & ((1 << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_BASE_ADDR_ALIGNSHIFT) - 1)) == 0);
	PVR_ASSERT((ui64EndAddress  & ((1 << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_END_ADDR_ALIGNSHIFT)  - 1)) == 0);

	ui64BaseAddress >>= RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_BASE_ADDR_ALIGNSHIFT;
	ui64EndAddress  >>= RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_END_ADDR_ALIGNSHIFT;

	ui64RegValue = (ui64RegValue << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_PAGE_SIZE_SHIFT) |
				   (ui64EndAddress  << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_END_ADDR_SHIFT) |
				   (ui64BaseAddress << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_BASE_ADDR_SHIFT);
	return ui64RegValue;
}

IMG_UINT64 RGXMMUInitRangeValue(IMG_UINT32 ui32MMURange)
{
	IMG_UINT64 ui64RegVal;
	IMG_UINT32 ui32Non4KHeapPageShift;
	IMG_UINT32 ui32RgxDefaultPageShift = RGXHeapDerivePageSize(OSGetPageShift());
	PVRSRV_ERROR eError;

	switch (ui32MMURange)
	{
		case RGX_MMU_RANGE_GLOBAL:
			/* set the last MMU config range covering the entire virtual memory to the OS's page size */
			ui64RegVal = RGXMMUComputeRangeValue(ui32RgxDefaultPageShift, 0, (1ULL << 40));
			break;
		case RGX_MMU_RANGE_NON4KHEAP:
			/*
			 * If the Non4K heap has a different page size than the OS's page size
			 * (used as default for all other heaps), configure one MMU config range
			 * for the Non4K heap
			 */
			eError = RGXGetNon4KHeapPageShift(NULL, &ui32Non4KHeapPageShift);
			PVR_LOG_IF_ERROR(eError, "RGXGetNon4KHeapPageShift");

			if (eError == PVRSRV_OK)
			{
				if (ui32Non4KHeapPageShift != ui32RgxDefaultPageShift)
				{
					ui64RegVal = RGXMMUComputeRangeValue(ui32Non4KHeapPageShift, RGX_GENERAL_NON4K_HEAP_BASE, RGX_GENERAL_NON4K_HEAP_SIZE);
				}
				else
				{
					ui64RegVal = RGXMMUComputeRangeValue(ui32RgxDefaultPageShift, 0, (1 << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_END_ADDR_ALIGNSHIFT));
				}
			}
			else
			{
				/*
				 * Error from Non4K Heap shift. Default to the normal PageShift
				 * to attempt to continue.
				 */
				ui64RegVal = RGXMMUComputeRangeValue(ui32RgxDefaultPageShift, 0, (1 << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_END_ADDR_ALIGNSHIFT));
			}
			break;
		default:
			ui64RegVal = RGXMMUComputeRangeValue(ui32RgxDefaultPageShift, 0, (1 << RGX_CR_MMU_PAGE_SIZE_RANGE_ONE_END_ADDR_ALIGNSHIFT));
			break;
	}

	return ui64RegVal;
}


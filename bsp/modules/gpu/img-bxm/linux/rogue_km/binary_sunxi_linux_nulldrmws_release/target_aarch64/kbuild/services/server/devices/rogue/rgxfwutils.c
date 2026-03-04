/*************************************************************************/ /*!
@File
@Title          Rogue firmware utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Rogue firmware utility routines
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

#if defined(__linux__)
#include <linux/stddef.h>
#else
#include <stddef.h>
#endif

#include "img_defs.h"

#include "rgxdefs_km.h"
#include "rgx_fwif_km.h"
#include "pdump_km.h"
#include "osfunc.h"
#include "os_apphint.h"
#include "cache_km.h"
#include "allocmem.h"
#include "physheap.h"
#include "devicemem.h"
#include "devicemem_pdump.h"
#include "devicemem_server.h"

#include "pvr_debug.h"
#include "pvr_notifier.h"
#include "rgxfwutils.h"
#include "rgxfwcmnctx.h"
#include "rgxfwriscv.h"
#include "rgx_options.h"
#include "rgx_fwif_alignchecks.h"
#include "rgx_fwif_resetframework.h"
#include "rgx_pdump_panics.h"
#include "fwtrace_string.h"
#include "rgxheapconfig.h"
#include "pvrsrv.h"
#include "rgxdebug_common.h"
#include "rgxhwperf.h"
#include "rgxccb.h"
#include "rgxcompute.h"
#include "rgxtransfer.h"
#include "rgxtdmtransfer.h"
#include "rgxpower.h"
#if defined(SUPPORT_DISPLAY_CLASS)
#include "dc_server.h"
#endif
#include "rgxmem.h"
#include "rgxmmudefs_km.h"
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
#include "rgxmipsmmuinit.h"
#endif
#include "rgxta3d.h"
#if defined(SUPPORT_RGXKICKSYNC_BRIDGE)
#include "rgxkicksync.h"
#endif
#include "rgxutils.h"
#include "rgxtimecorr.h"
#include "sync_internal.h"
#include "sync.h"
#include "sync_checkpoint.h"
#include "sync_checkpoint_external.h"
#include "tlstream.h"
#include "devicemem_server_utils.h"
#include "htbserver.h"
#include "rgx_bvnc_defs_km.h"
#include "info_page.h"

#include "physmem_lma.h"
#include "physmem_osmem.h"

#ifdef __linux__
#include <linux/kernel.h>	/* sprintf */
#else
#include <stdio.h>
#include <string.h>
#endif
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#include "process_stats.h"
#endif

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
#include "rgxworkest.h"
#endif

#if defined(SUPPORT_PDVFS)
#include "rgxpdvfs.h"
#endif

#if defined(SUPPORT_FW_OPP_TABLE) && defined(CONFIG_OF)
#include "pvr_dvfs_common.h"
#endif


#include "vz_vmm_pvz.h"
#include "rgx_heaps.h"

/*!
 ******************************************************************************
 * HWPERF
 *****************************************************************************/
/* Size of the Firmware L1 HWPERF buffer in bytes (2MB). Accessed by the
 * Firmware and host driver. */
#define RGXFW_HWPERF_L1_SIZE_MIN        (16U)
#define RGXFW_HWPERF_L1_SIZE_DEFAULT    PVRSRV_APPHINT_HWPERFFWBUFSIZEINKB
#define RGXFW_HWPERF_L1_SIZE_MAX        (12288U)
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
#if defined(DEBUG)
/* Catch the use of auto-increment when meta_registers_unpacked_accesses feature is
 * present in case we ever use it. No WA exists so it must not be used */
#define CHECK_HWBRN_68777(v) \
	do { \
		PVR_ASSERT(((v) & RGX_CR_META_SP_MSLVCTRL0_AUTOINCR_EN) == 0); \
	} while (0)
#else
#define CHECK_HWBRN_68777(v)
#endif
#endif

/* Firmware CCB length */
#if defined(NO_HARDWARE) && defined(PDUMP)
#define RGXFWIF_FWCCB_NUMCMDS_LOG2   (10)
#elif defined(SUPPORT_PDVFS) || defined(SUPPORT_WORKLOAD_ESTIMATION)
#define RGXFWIF_FWCCB_NUMCMDS_LOG2   (8)
#else
#define RGXFWIF_FWCCB_NUMCMDS_LOG2   (5)
#endif

#if defined(RGX_FW_IRQ_OS_COUNTERS)
const IMG_UINT32 gaui32FwOsIrqCntRegAddr[RGXFW_MAX_NUM_OSIDS] = {IRQ_COUNTER_STORAGE_REGS};
#endif

/* Workload Estimation Firmware CCB length */
#define RGXFWIF_WORKEST_FWCCB_NUMCMDS_LOG2   (7)

/* Size of memory buffer for firmware gcov data
 * The actual data size is several hundred kilobytes. The buffer is an order of magnitude larger. */
#define RGXFWIF_FIRMWARE_GCOV_BUFFER_SIZE (4*1024*1024)

typedef struct
{
	RGXFWIF_KCCB_CMD        sKCCBcmd;
	DLLIST_NODE             sListNode;
	PDUMP_FLAGS_T           uiPDumpFlags;
	PVRSRV_RGXDEV_INFO      *psDevInfo;
} RGX_DEFERRED_KCCB_CMD;

typedef struct
{
	IMG_INT32  i32Priority;
	IMG_UINT32 ui32IsolationGroups;
	IMG_UINT32 ui32TSPercentage;
}RGX_QOS_DEFAULTS;

#define RGX_QOS_DEFAULTS_INIT(osid) \
	{RGX_DRIVERID_##osid##_DEFAULT_PRIORITY,\
	RGX_DRIVERID_##osid##_DEFAULT_ISOLATION_GROUP,\
	RGX_DRIVERID_##osid##_DEFAULT_TIME_SLICE}

#if defined(PDUMP)
/* ensure PIDs are 32-bit because a 32-bit PDump load is generated for the
 * PID filter example entries
 */
static_assert(sizeof(IMG_PID) == sizeof(IMG_UINT32),
		"FW PID filtering assumes the IMG_PID type is 32-bits wide as it "
		"generates WRW commands for loading the PID values");
#endif

#if (RGXFW_MAX_NUM_OSIDS > 1)
static_assert(RGX_DRIVER_DEFAULT_TIME_SLICES_SUM <= PVRSRV_VZ_TIME_SLICE_MAX, "Invalid driverid time slice aggregate");
#endif

static void RGXFreeFwOsData(PVRSRV_RGXDEV_INFO *psDevInfo);
static void RGXFreeFwSysData(PVRSRV_RGXDEV_INFO *psDevInfo);


static void __MTSScheduleWrite(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32Value)
{
	/* This should *NOT* happen. Try to trace what caused this and avoid a NPE
	 * with the Write/Read at the foot of the function.
	 */
	PVR_ASSERT((psDevInfo != NULL));
	if (psDevInfo == NULL)
	{
		return;
	}

	/* Kick MTS to wake firmware. */
	OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MTS_SCHEDULE, ui32Value);

	/* Uncached device/IO mapping will ensure MTS kick leaves CPU, read back
	 * will ensure it reaches the regbank via inter-connects (AXI, PCIe etc)
	 * before continuing.
	 */
	(void) OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MTS_SCHEDULE);
}

/*************************************************************************/ /*!
@Function       RGXSetupFwAllocation

@Description    Sets a pointer in a firmware data structure.

@Input          psDevInfo       Device Info struct
@Input          uiAllocFlags    Flags determining type of memory allocation
@Input          ui32Size        Size of memory allocation
@Input          pszName         Allocation label
@Input          ppsMemDesc      pointer to the allocation's memory descriptor
@Input          psFwPtr         Address of the firmware pointer to set
@Input          ppvCpuPtr       Address of the cpu pointer to set
@Input          ui32DevVAFlags  Any combination of  RFW_FWADDR_*_FLAG

@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR RGXSetupFwAllocation(PVRSRV_RGXDEV_INFO*  psDevInfo,
								  PVRSRV_MEMALLOCFLAGS_T uiAllocFlags,
								  IMG_UINT32           ui32Size,
								  const IMG_CHAR       *pszName,
								  DEVMEM_MEMDESC       **ppsMemDesc,
								  RGXFWIF_DEV_VIRTADDR *psFwPtr,
								  void                 **ppvCpuPtr,
								  IMG_UINT32           ui32DevVAFlags)
{
	PVRSRV_ERROR eError;
#if defined(SUPPORT_AUTOVZ)
	IMG_BOOL bClearByMemset;
	if (PVRSRV_CHECK_ZERO_ON_ALLOC(uiAllocFlags))
	{
		/* Under AutoVz the ZERO_ON_ALLOC flag is avoided as it causes the memory to
		 * be allocated from a different PMR than an allocation without the flag.
		 * When the content of an allocation needs to be recovered from physical memory
		 * on a later driver reboot, the memory then cannot be zeroed but the allocation
		 * addresses must still match.
		 * If the memory requires clearing, perform a memset after the allocation. */
		uiAllocFlags &= ~PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC;
		bClearByMemset = IMG_TRUE;
	}
	else
	{
		bClearByMemset = IMG_FALSE;
	}
#endif

	PDUMPCOMMENT(psDevInfo->psDeviceNode, "Allocate %s", pszName);
	eError = DevmemFwAllocate(psDevInfo,
							  ui32Size,
							  uiAllocFlags,
							  pszName,
							  ppsMemDesc);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to allocate %u bytes for %s (%u)",
				 __func__,
				 ui32Size,
				 pszName,
				 eError));
		goto fail_alloc;
	}

	if (psFwPtr)
	{
		eError = RGXSetFirmwareAddress(psFwPtr, *ppsMemDesc, 0, ui32DevVAFlags);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: Failed to acquire firmware virtual address for %s (%u)",
					 __func__,
					 pszName,
					 eError));
			goto fail_fwaddr;
		}
	}

#if defined(SUPPORT_AUTOVZ)
	if ((bClearByMemset) || (ppvCpuPtr))
#else
	if (ppvCpuPtr)
#endif
	{
		void *pvTempCpuPtr;

		eError = DevmemAcquireCpuVirtAddr(*ppsMemDesc, &pvTempCpuPtr);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					"%s: Failed to acquire CPU virtual address for %s (%u)",
					__func__,
					 pszName,
					eError));
			goto fail_cpuva;
		}

#if defined(SUPPORT_AUTOVZ)
		if (bClearByMemset)
		{
			if (PVRSRV_CHECK_CPU_WRITE_COMBINE(uiAllocFlags))
			{
				OSCachedMemSetWMB(pvTempCpuPtr, 0, ui32Size);
			}
			else
			{
				OSDeviceMemSet(pvTempCpuPtr, 0, ui32Size);
			}
		}
		if (ppvCpuPtr)
#endif
		{
			*ppvCpuPtr = pvTempCpuPtr;
		}
#if defined(SUPPORT_AUTOVZ)
		else
		{
			DevmemReleaseCpuVirtAddr(*ppsMemDesc);
			pvTempCpuPtr = NULL;
		}
#endif
	}

	PVR_DPF((PVR_DBG_MESSAGE, "%s: %s set up at Fw VA 0x%x and CPU VA 0x%p with alloc flags 0x%" IMG_UINT64_FMTSPECX,
			 __func__, pszName,
			 (psFwPtr)   ? (psFwPtr->ui32Addr) : (0),
			 (ppvCpuPtr) ? (*ppvCpuPtr)        : (NULL),
			 uiAllocFlags));

	return eError;

fail_cpuva:
	if (psFwPtr)
	{
		RGXUnsetFirmwareAddress(*ppsMemDesc);
	}
fail_fwaddr:
	DevmemFree(*ppsMemDesc);
fail_alloc:
	return eError;
}

/*************************************************************************/ /*!
@Function       GetHwPerfBufferSize

@Description    Computes the effective size of the HW Perf Buffer
@Input          ui32HWPerfFWBufSizeKB       Device Info struct
@Return         HwPerfBufferSize
*/ /**************************************************************************/
static IMG_UINT32 GetHwPerfBufferSize(IMG_UINT32 ui32HWPerfFWBufSizeKB)
{
	IMG_UINT32 HwPerfBufferSize;

	/* HWPerf: Determine the size of the FW buffer */
	if (ui32HWPerfFWBufSizeKB == 0 ||
			ui32HWPerfFWBufSizeKB == RGXFW_HWPERF_L1_SIZE_DEFAULT)
	{
		/* Under pvrsrvctl 0 size implies AppHint not set or is set to zero,
		 * use default size from driver constant. Set it to the default
		 * size, no logging.
		 */
		HwPerfBufferSize = RGXFW_HWPERF_L1_SIZE_DEFAULT<<10;
	}
	else if (ui32HWPerfFWBufSizeKB > (RGXFW_HWPERF_L1_SIZE_MAX))
	{
		/* Size specified as a AppHint but it is too big */
		PVR_DPF((PVR_DBG_WARNING,
				"%s: HWPerfFWBufSizeInKB value (%u) too big, using maximum (%u)",
				__func__,
				ui32HWPerfFWBufSizeKB, RGXFW_HWPERF_L1_SIZE_MAX));
		HwPerfBufferSize = RGXFW_HWPERF_L1_SIZE_MAX<<10;
	}
	else if (ui32HWPerfFWBufSizeKB > (RGXFW_HWPERF_L1_SIZE_MIN))
	{
		/* Size specified as in AppHint HWPerfFWBufSizeInKB */
		PVR_DPF((PVR_DBG_WARNING,
				"%s: Using HWPerf FW buffer size of %u KB",
				__func__,
				ui32HWPerfFWBufSizeKB));
		HwPerfBufferSize = ui32HWPerfFWBufSizeKB<<10;
	}
	else
	{
		/* Size specified as a AppHint but it is too small */
		PVR_DPF((PVR_DBG_WARNING,
				"%s: HWPerfFWBufSizeInKB value (%u) too small, using minimum (%u)",
				__func__,
				ui32HWPerfFWBufSizeKB, RGXFW_HWPERF_L1_SIZE_MIN));
		HwPerfBufferSize = RGXFW_HWPERF_L1_SIZE_MIN<<10;
	}

	return HwPerfBufferSize;
}

#if defined(PDUMP)
/*!
*******************************************************************************
 @Function		RGXFWSetupSignatureChecks
 @Description
 @Input			psDevInfo

 @Return		PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXFWSetupSignatureChecks(PVRSRV_RGXDEV_INFO* psDevInfo,
                                              DEVMEM_MEMDESC**    ppsSigChecksMemDesc,
                                              IMG_UINT32          ui32SigChecksBufSize,
                                              RGXFWIF_SIGBUF_CTL* psSigBufCtl)
{
	PVRSRV_ERROR	eError;

	/* Allocate memory for the checks */
	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS,
								  ui32SigChecksBufSize,
								  "FwSignatureChecks",
								  ppsSigChecksMemDesc,
								  &psSigBufCtl->sBuffer,
								  NULL,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation", fail);

	DevmemPDumpLoadMem(	*ppsSigChecksMemDesc,
			0,
			ui32SigChecksBufSize,
			PDUMP_FLAGS_CONTINUOUS);

	psSigBufCtl->ui32LeftSizeInRegs = ui32SigChecksBufSize / sizeof(IMG_UINT32);
fail:
	return eError;
}
#endif


#if defined(SUPPORT_FIRMWARE_GCOV)
/*!
*******************************************************************************
 @Function		RGXFWSetupFirmwareGcovBuffer
 @Description
 @Input			psDevInfo

 @Return		PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXFWSetupFirmwareGcovBuffer(PVRSRV_RGXDEV_INFO*			psDevInfo,
		DEVMEM_MEMDESC**			ppsBufferMemDesc,
		IMG_UINT32					ui32FirmwareGcovBufferSize,
		RGXFWIF_FIRMWARE_GCOV_CTL*	psFirmwareGcovCtl,
		const IMG_CHAR*				pszBufferName)
{
	PVRSRV_ERROR	eError;

	/* Allocate memory for gcov */
	eError = RGXSetupFwAllocation(psDevInfo,
								  (RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS |
								   PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED)),
								  ui32FirmwareGcovBufferSize,
								  pszBufferName,
								  ppsBufferMemDesc,
								  &psFirmwareGcovCtl->sBuffer,
								  NULL,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_RETURN_IF_ERROR(eError, "RGXSetupFwAllocation");

	psFirmwareGcovCtl->ui32Size = ui32FirmwareGcovBufferSize;

	return PVRSRV_OK;
}
#endif

#if defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
/*!
 ******************************************************************************
 @Function		RGXFWSetupCounterBuffer
 @Description
 @Input			psDevInfo

 @Return		PVRSRV_ERROR
 *****************************************************************************/
static PVRSRV_ERROR RGXFWSetupCounterBuffer(PVRSRV_RGXDEV_INFO* psDevInfo,
		DEVMEM_MEMDESC**			ppsBufferMemDesc,
		IMG_UINT32					ui32CounterDataBufferSize,
		RGXFWIF_COUNTER_DUMP_CTL*	psCounterDumpCtl)
{
	PVRSRV_ERROR eError;

	eError = RGXSetupFwAllocation(psDevInfo,
								  (RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS |
								   PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED)),
								  ui32CounterDataBufferSize,
								  "FwCounterBuffer",
								  ppsBufferMemDesc,
								  &psCounterDumpCtl->sBuffer,
								  NULL,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_RETURN_IF_ERROR(eError, "RGXSetupFwAllocation");

	psCounterDumpCtl->ui32SizeInDwords = ui32CounterDataBufferSize >> 2;
	RGXFwSharedMemCacheOpValue(psCounterDumpCtl->ui32SizeInDwords, FLUSH);

	return PVRSRV_OK;
}
#endif

/*!
*******************************************************************************
 @Function      RGXFWSetupAlignChecks
 @Description   This functions allocates and fills memory needed for the
                aligns checks of the UM and KM structures shared with the
                firmware. The format of the data in the memory is as follows:
                    <number of elements in the KM array>
                    <array of KM structures' sizes and members' offsets>
                    <number of elements in the UM array>
                    <array of UM structures' sizes and members' offsets>
                The UM array is passed from the user side. Now the firmware is
                responsible for filling this part of the memory. If that
                happens the check of the UM structures will be performed
                by the host driver on client's connect.
                If the macro is not defined the client driver fills the memory
                and the firmware checks for the alignment of all structures.
 @Input			psDeviceNode

 @Return		PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXFWSetupAlignChecks(PVRSRV_DEVICE_NODE *psDeviceNode,
								RGXFWIF_DEV_VIRTADDR	*psAlignChecksDevFW)
{
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	IMG_UINT32			aui32RGXFWAlignChecksKM[] = { RGXFW_ALIGN_CHECKS_INIT_KM };
	IMG_UINT32			ui32RGXFWAlignChecksTotal;
	IMG_UINT32*			paui32AlignChecks;
	PVRSRV_ERROR		eError;

	/* In this case we don't know the number of elements in UM array.
	 * We have to assume something so we assume RGXFW_ALIGN_CHECKS_UM_MAX.
	 */
	ui32RGXFWAlignChecksTotal = sizeof(aui32RGXFWAlignChecksKM)
	                            + RGXFW_ALIGN_CHECKS_UM_MAX * sizeof(IMG_UINT32)
	                            + 2 * sizeof(IMG_UINT32);

	/* Allocate memory for the checks */
	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS &
								  RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
								  ui32RGXFWAlignChecksTotal,
								  "FwAlignmentChecks",
								  &psDevInfo->psRGXFWAlignChecksMemDesc,
								  psAlignChecksDevFW,
								  (void**) &paui32AlignChecks,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation", fail);

	if (!psDeviceNode->bAutoVzFwIsUp)
	{
		/* Copy the values */
		*paui32AlignChecks++ = ARRAY_SIZE(aui32RGXFWAlignChecksKM);
		OSCachedMemCopy(paui32AlignChecks, &aui32RGXFWAlignChecksKM[0],
		                sizeof(aui32RGXFWAlignChecksKM));
		paui32AlignChecks += ARRAY_SIZE(aui32RGXFWAlignChecksKM);

		*paui32AlignChecks = 0;

		OSWriteMemoryBarrier(paui32AlignChecks);
		RGXFwSharedMemCacheOpExec(paui32AlignChecks - (ARRAY_SIZE(aui32RGXFWAlignChecksKM) + 1),
		                          ui32RGXFWAlignChecksTotal,
		                          PVRSRV_CACHE_OP_FLUSH);
	}


	DevmemPDumpLoadMem(	psDevInfo->psRGXFWAlignChecksMemDesc,
						0,
						ui32RGXFWAlignChecksTotal,
						PDUMP_FLAGS_CONTINUOUS);

	return PVRSRV_OK;

fail:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

static void RGXFWFreeAlignChecks(PVRSRV_RGXDEV_INFO* psDevInfo)
{
	if (psDevInfo->psRGXFWAlignChecksMemDesc != NULL)
	{
		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWAlignChecksMemDesc);
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWAlignChecksMemDesc);
		psDevInfo->psRGXFWAlignChecksMemDesc = NULL;
	}
}

PVRSRV_ERROR RGXSetFirmwareAddress(RGXFWIF_DEV_VIRTADDR	*ppDest,
						   DEVMEM_MEMDESC		*psSrc,
						   IMG_UINT32			uiExtraOffset,
						   IMG_UINT32			ui32Flags)
{
	PVRSRV_ERROR		eError;
	IMG_DEV_VIRTADDR	psDevVirtAddr;
	PVRSRV_DEVICE_NODE	*psDeviceNode;
	PVRSRV_RGXDEV_INFO	*psDevInfo;

	psDeviceNode = (PVRSRV_DEVICE_NODE *) DevmemGetConnection(psSrc);
	psDevInfo = (PVRSRV_RGXDEV_INFO *)psDeviceNode->pvDevice;

	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META))
	{
		IMG_UINT32          ui32Offset;
		IMG_BOOL            bCachedInMETA;
		PVRSRV_MEMALLOCFLAGS_T uiDevFlags;
		IMG_UINT32          uiGPUCacheMode;

		eError = DevmemAcquireDevVirtAddr(psSrc, &psDevVirtAddr);
		PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireDevVirtAddr", failDevVAAcquire);

		/* Convert to an address in META memmap */
		ui32Offset = psDevVirtAddr.uiAddr + uiExtraOffset - RGX_FIRMWARE_RAW_HEAP_BASE;

		/* Check in the devmem flags whether this memory is cached/uncached */
		DevmemGetFlags(psSrc, &uiDevFlags);

		/* Honour the META cache flags */
		bCachedInMETA = (PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED) & uiDevFlags) != 0;

		/* Honour the SLC cache flags */
		eError = DevmemDeviceCacheMode(uiDevFlags, &uiGPUCacheMode);
		PVR_LOG_GOTO_IF_ERROR(eError, "DevmemDeviceCacheMode", failDevCacheMode);

		/*
		 * Choose Meta virtual address based on Meta and SLC cacheability.
		 */
		ui32Offset += RGXFW_SEGMMU_DATA_BASE_ADDRESS;

		if (bCachedInMETA)
		{
			ui32Offset |= RGXFW_SEGMMU_DATA_META_CACHED;
		}
		else
		{
			ui32Offset |= RGXFW_SEGMMU_DATA_META_UNCACHED;
		}

		if (PVRSRV_CHECK_GPU_CACHED(uiGPUCacheMode))
		{
			ui32Offset |= RGXFW_SEGMMU_DATA_VIVT_SLC_CACHED;
		}
		else
		{
			ui32Offset |= RGXFW_SEGMMU_DATA_VIVT_SLC_UNCACHED;
		}

		ppDest->ui32Addr = ui32Offset;
	}
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	else if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		eError = DevmemAcquireDevVirtAddr(psSrc, &psDevVirtAddr);
		PVR_GOTO_IF_ERROR(eError, failDevVAAcquire);

		ppDest->ui32Addr = (IMG_UINT32)((psDevVirtAddr.uiAddr + uiExtraOffset) & 0xFFFFFFFF);
	}
#endif
	else
	{
		IMG_UINT32      ui32Offset;
		IMG_BOOL        bCachedInRISCV;
		PVRSRV_MEMALLOCFLAGS_T uiDevFlags;

		eError = DevmemAcquireDevVirtAddr(psSrc, &psDevVirtAddr);
		PVR_LOG_GOTO_IF_ERROR(eError, "DevmemAcquireDevVirtAddr", failDevVAAcquire);

		/* Convert to an address in RISCV memmap */
		ui32Offset = psDevVirtAddr.uiAddr + uiExtraOffset - RGX_FIRMWARE_RAW_HEAP_BASE;

		/* Check in the devmem flags whether this memory is cached/uncached */
		DevmemGetFlags(psSrc, &uiDevFlags);

		/* Honour the RISCV cache flags */
		bCachedInRISCV = (PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED) & uiDevFlags) != 0;

		if (bCachedInRISCV)
		{
			ui32Offset |= RGXRISCVFW_SHARED_CACHED_DATA_BASE;
		}
		else
		{
			ui32Offset |= RGXRISCVFW_SHARED_UNCACHED_DATA_BASE;
		}

		ppDest->ui32Addr = ui32Offset;
	}

	if ((ppDest->ui32Addr & 0x3U) != 0)
	{
		IMG_CHAR *pszAnnotation;
		/* It is expected that the annotation returned by DevmemGetAnnotation() is always valid */
		DevmemGetAnnotation(psSrc, &pszAnnotation);

		PVR_DPF((PVR_DBG_ERROR, "%s: %s @ 0x%x is not aligned to 32 bit",
				 __func__, pszAnnotation, ppDest->ui32Addr));

		return PVRSRV_ERROR_INVALID_ALIGNMENT;
	}

	if (ui32Flags & RFW_FWADDR_NOREF_FLAG)
	{
		DevmemReleaseDevVirtAddr(psSrc);
	}

	return PVRSRV_OK;

failDevCacheMode:
	DevmemReleaseDevVirtAddr(psSrc);
failDevVAAcquire:
	return eError;
}

void RGXSetMetaDMAAddress(RGXFWIF_DMA_ADDR		*psDest,
						  DEVMEM_MEMDESC		*psSrcMemDesc,
						  RGXFWIF_DEV_VIRTADDR	*psSrcFWDevVAddr,
						  IMG_UINT32			uiOffset)
{
	PVRSRV_ERROR		eError;
	IMG_DEV_VIRTADDR	sDevVirtAddr;

	eError = DevmemAcquireDevVirtAddr(psSrcMemDesc, &sDevVirtAddr);
	PVR_ASSERT(eError == PVRSRV_OK);

	psDest->psDevVirtAddr.uiAddr = sDevVirtAddr.uiAddr;
	psDest->psDevVirtAddr.uiAddr += uiOffset;
	psDest->pbyFWAddr.ui32Addr = psSrcFWDevVAddr->ui32Addr;

	DevmemReleaseDevVirtAddr(psSrcMemDesc);
}


void RGXUnsetFirmwareAddress(DEVMEM_MEMDESC *psSrc)
{
	DevmemReleaseDevVirtAddr(psSrc);
}

/*!
*******************************************************************************
 @Function		RGXFreeCCB
 @Description	Free the kernel or firmware CCB
 @Input			psDevInfo
 @Input			ppsCCBCtl
 @Input			ppvCCBCtlLocal
 @Input			ppsCCBCtlMemDesc
 @Input			ppsCCBMemDesc
 @Input			psCCBCtlFWAddr
******************************************************************************/
static void RGXFreeCCB(PVRSRV_RGXDEV_INFO	*psDevInfo,
					   RGXFWIF_CCB_CTL		**ppsCCBCtl,
					   RGXFWIF_CCB_CTL		**ppsCCBCtlLocal,
					   DEVMEM_MEMDESC		**ppsCCBCtlMemDesc,
					   IMG_UINT8			**ppui8CCB,
					   DEVMEM_MEMDESC		**ppsCCBMemDesc)
{
	if (*ppsCCBMemDesc != NULL)
	{
		if (*ppui8CCB != NULL)
		{
			DevmemReleaseCpuVirtAddr(*ppsCCBMemDesc);
			*ppui8CCB = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, *ppsCCBMemDesc);
		*ppsCCBMemDesc = NULL;
	}
	if (*ppsCCBCtlMemDesc != NULL)
	{
		if (*ppsCCBCtl != NULL)
		{
			DevmemReleaseCpuVirtAddr(*ppsCCBCtlMemDesc);
			*ppsCCBCtl = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, *ppsCCBCtlMemDesc);
		*ppsCCBCtlMemDesc = NULL;
	}
	if (*ppsCCBCtlLocal != NULL)
	{
		OSFreeMem(*ppsCCBCtlLocal);
		*ppsCCBCtlLocal = NULL;
	}
}

/*!
*******************************************************************************
 @Function		RGXFreeCCBReturnSlots
 @Description	Free the kernel CCB's return slot array and associated mappings
 @Input			psDevInfo              Device Info struct
 @Input			ppui32CCBRtnSlots      CPU mapping of slot array
 @Input			ppsCCBRtnSlotsMemDesc  Slot array's device memdesc
******************************************************************************/
static void RGXFreeCCBReturnSlots(PVRSRV_RGXDEV_INFO *psDevInfo,
                                  IMG_UINT32         **ppui32CCBRtnSlots,
								  DEVMEM_MEMDESC     **ppsCCBRtnSlotsMemDesc)
{
	/* Free the return slot array if allocated */
	if (*ppsCCBRtnSlotsMemDesc != NULL)
	{
		/* Before freeing, ensure the CPU mapping as well is released */
		if (*ppui32CCBRtnSlots != NULL)
		{
			DevmemReleaseCpuVirtAddr(*ppsCCBRtnSlotsMemDesc);
			*ppui32CCBRtnSlots = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, *ppsCCBRtnSlotsMemDesc);
		*ppsCCBRtnSlotsMemDesc = NULL;
	}
}

/*!
*******************************************************************************
 @Function		RGXSetupCCB
 @Description	Allocate and initialise a circular command buffer
 @Input			psDevInfo
 @Input			ppsCCBCtl
 @Input			ppsCCBCtlMemDesc
 @Input			ppui8CCB
 @Input			ppsCCBMemDesc
 @Input			psCCBCtlFWAddr
 @Input			ui32NumCmdsLog2
 @Input			ui32CmdSize
 @Input			uiCCBMemAllocFlags
 @Input			pszName

 @Return		PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXSetupCCB(PVRSRV_RGXDEV_INFO	*psDevInfo,
								RGXFWIF_CCB_CTL		**ppsCCBCtl,
								RGXFWIF_CCB_CTL		**ppsCCBCtlLocal,
								DEVMEM_MEMDESC		**ppsCCBCtlMemDesc,
								IMG_UINT8			**ppui8CCB,
								DEVMEM_MEMDESC		**ppsCCBMemDesc,
								PRGXFWIF_CCB_CTL	*psCCBCtlFWAddr,
								PRGXFWIF_CCB		*psCCBFWAddr,
								IMG_UINT32			ui32NumCmdsLog2,
								IMG_UINT32			ui32CmdSize,
								PVRSRV_MEMALLOCFLAGS_T uiCCBMemAllocFlags,
								const IMG_CHAR		*pszName)
{
	PVRSRV_ERROR		eError;
	RGXFWIF_CCB_CTL		*psCCBCtl;
	IMG_UINT32		ui32CCBSize = (1U << ui32NumCmdsLog2);
	IMG_CHAR		szCCBCtlName[DEVMEM_ANNOTATION_MAX_LEN];
	IMG_INT32		iStrLen;

	/* Append "Control" to the name for the control struct. */
	iStrLen = OSSNPrintf(szCCBCtlName, sizeof(szCCBCtlName), "%sControl", pszName);
	PVR_ASSERT(iStrLen < sizeof(szCCBCtlName));

	if (unlikely(iStrLen < 0))
	{
		OSStringSafeCopy(szCCBCtlName, "FwCCBControl", DEVMEM_ANNOTATION_MAX_LEN);
	}

	/* Allocate memory for the CCB control.*/
	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS,
								  sizeof(RGXFWIF_CCB_CTL),
								  szCCBCtlName,
								  ppsCCBCtlMemDesc,
								  psCCBCtlFWAddr,
								  (void**) ppsCCBCtl,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation", fail);

	/*
	 * Allocate memory for the CCB.
	 * (this will reference further command data in non-shared CCBs)
	 */
	eError = RGXSetupFwAllocation(psDevInfo,
								  uiCCBMemAllocFlags,
								  ui32CCBSize * ui32CmdSize,
								  pszName,
								  ppsCCBMemDesc,
								  psCCBFWAddr,
								  (void**) ppui8CCB,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation", fail);

	/*
	 * Initialise the CCB control.
	 */
	psCCBCtl = OSAllocZMem(sizeof(*psCCBCtl));
	PVR_LOG_GOTO_IF_NOMEM(psCCBCtl, eError, fail);

	psCCBCtl->ui32WrapMask = ui32CCBSize - 1;

	OSDeviceMemCopy(*ppsCCBCtl, psCCBCtl, sizeof(*psCCBCtl));
	RGXFwSharedMemCacheOpPtr(psCCBCtl, FLUSH);

	*ppsCCBCtlLocal = psCCBCtl;

	/* Pdump the CCB control */
	PDUMPCOMMENT(psDevInfo->psDeviceNode, "Initialise %s", szCCBCtlName);
	DevmemPDumpLoadMem(*ppsCCBCtlMemDesc,
					   0,
					   sizeof(RGXFWIF_CCB_CTL),
					   0);

	return PVRSRV_OK;

fail:
	RGXFreeCCB(psDevInfo,
			   ppsCCBCtl,
			   ppsCCBCtlLocal,
			   ppsCCBCtlMemDesc,
			   ppui8CCB,
			   ppsCCBMemDesc);

	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

static void RGXSetupFaultReadRegisterRollback(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PMR *psPMR;

#if defined(RGX_FEATURE_SLC_FAULT_ACCESS_ADDR_PHYS_BIT_MASK)
	/* Run-time check feature support */
	if (PVRSRV_IS_FEATURE_SUPPORTED(psDevInfo->psDeviceNode, SLC_FAULT_ACCESS_ADDR_PHYS))
#endif
	{
		if (psDevInfo->psRGXFaultAddressMemDesc)
		{
			if (DevmemServerGetImportHandle(psDevInfo->psRGXFaultAddressMemDesc, (void **)&psPMR) == PVRSRV_OK)
			{
				PMRUnlockSysPhysAddresses(psPMR);
			}
			DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFaultAddressMemDesc);
			psDevInfo->psRGXFaultAddressMemDesc = NULL;
		}
	}
}

static PVRSRV_ERROR RGXSetupFaultReadRegister(PVRSRV_DEVICE_NODE *psDeviceNode, RGXFWIF_SYSINIT *psFwSysInit)
{
	PVRSRV_ERROR		eError = PVRSRV_OK;
	IMG_UINT32			*pui32MemoryVirtAddr;
	IMG_UINT32			i;
	size_t				ui32PageSize = OSGetPageSize();
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	PMR					*psPMR;

#if defined(RGX_FEATURE_SLC_FAULT_ACCESS_ADDR_PHYS_BIT_MASK)
	/* Run-time check feature support */
	if (!PVRSRV_IS_FEATURE_SUPPORTED(psDeviceNode, SLC_FAULT_ACCESS_ADDR_PHYS))
	{
		return PVRSRV_OK;
	}
#endif

	/* Allocate page of memory to use for page faults on non-blocking memory transactions.
	 * Doesn't need to be cleared as it is initialised with the 0xDEADBEEF pattern below. */
	psDevInfo->psRGXFaultAddressMemDesc = NULL;
	eError = DevmemFwAllocateExportable(psDeviceNode,
			ui32PageSize,
			ui32PageSize,
			RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS & ~PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC,
			"FwExFaultAddress",
			&psDevInfo->psRGXFaultAddressMemDesc);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed to allocate mem for fault address (%u)",
		         __func__, eError));
		goto failFaultAddressDescAlloc;
	}


	if (!psDeviceNode->bAutoVzFwIsUp)
	{
		eError = DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFaultAddressMemDesc,
										  (void **)&pui32MemoryVirtAddr);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: Failed to acquire mem for fault address (%u)",
					 __func__, eError));
			goto failFaultAddressDescAqCpuVirt;
		}

		/* fill the page with a known pattern when booting the firmware */
		for (i = 0; i < ui32PageSize/sizeof(IMG_UINT32); i++)
		{
			*(pui32MemoryVirtAddr + i) = 0xDEADBEEF;
		}

		OSWriteMemoryBarrier(pui32MemoryVirtAddr);
		RGXFwSharedMemCacheOpExec(pui32MemoryVirtAddr, ui32PageSize, PVRSRV_CACHE_OP_FLUSH);

		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFaultAddressMemDesc);
	}

	eError = DevmemServerGetImportHandle(psDevInfo->psRGXFaultAddressMemDesc, (void **)&psPMR);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Error getting PMR for fault address (%u)",
		         __func__, eError));

		goto failFaultAddressDescGetPMR;
	}
	else
	{
		IMG_BOOL bValid;
		IMG_UINT32 ui32Log2PageSize = OSGetPageShift();

		eError = PMRLockSysPhysAddresses(psPMR);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Error locking physical address for fault address MemDesc (%u)",
			         __func__, eError));

			goto failFaultAddressDescLockPhys;
		}

		eError = PMR_DevPhysAddr(psPMR,ui32Log2PageSize, 1, 0, &(psFwSysInit->sFaultPhysAddr), &bValid, DEVICE_USE);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Error getting physical address for fault address MemDesc (%u)",
			         __func__, eError));

			goto failFaultAddressDescGetPhys;
		}

		if (!bValid)
		{
			psFwSysInit->sFaultPhysAddr.uiAddr = 0;
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Failed getting physical address for fault address MemDesc - invalid page (0x%" IMG_UINT64_FMTSPECX ")",
			         __func__, psFwSysInit->sFaultPhysAddr.uiAddr));

			goto failFaultAddressDescGetPhys;
		}
	}

	return PVRSRV_OK;

failFaultAddressDescGetPhys:
	PMRUnlockSysPhysAddresses(psPMR);

failFaultAddressDescLockPhys:
failFaultAddressDescGetPMR:
failFaultAddressDescAqCpuVirt:
	DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFaultAddressMemDesc);
	psDevInfo->psRGXFaultAddressMemDesc = NULL;

failFaultAddressDescAlloc:

	return eError;
}

#if defined(PDUMP)
/* Replace the DevPhy address with the one Pdump allocates at pdump_player run time */
static PVRSRV_ERROR RGXPDumpFaultReadRegister(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PVRSRV_ERROR eError;
	PMR *psFWInitPMR, *psFaultAddrPMR;
	IMG_UINT32 ui32Dstoffset;

#if defined(RGX_FEATURE_SLC_FAULT_ACCESS_ADDR_PHYS_BIT_MASK)
	/* Run-time check feature support */
	if (!PVRSRV_IS_FEATURE_SUPPORTED(psDevInfo->psDeviceNode, SLC_FAULT_ACCESS_ADDR_PHYS))
	{
		return PVRSRV_OK;
	}
#endif

	psFWInitPMR = (PMR *)(psDevInfo->psRGXFWIfSysInitMemDesc->psImport->hPMR);
	ui32Dstoffset = psDevInfo->psRGXFWIfSysInitMemDesc->uiOffset + offsetof(RGXFWIF_SYSINIT, sFaultPhysAddr.uiAddr);

	psFaultAddrPMR = (PMR *)(psDevInfo->psRGXFaultAddressMemDesc->psImport->hPMR);

	eError = PDumpMemLabelToMem64(psFaultAddrPMR,
				psFWInitPMR,
				0,
				ui32Dstoffset,
				PDUMP_FLAGS_CONTINUOUS);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Dump of Fault Page Phys address failed(%u)", __func__, eError));
	}
	return eError;
}
#endif

#if defined(SUPPORT_TBI_INTERFACE)
/*************************************************************************/ /*!
@Function       RGXTBIBufferDeinit

@Description    Deinitialises all the allocations and references that are made
		for the FW tbi buffer

@Input          ppsDevInfo	 RGX device info
@Return		void
*/ /**************************************************************************/
static void RGXTBIBufferDeinit(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfTBIBufferMemDesc);
	psDevInfo->psRGXFWIfTBIBufferMemDesc = NULL;
	psDevInfo->ui32RGXFWIfHWPerfBufSize = 0;
	psDevInfo->ui32RGXL2HWPerfBufSize = 0;
}

/*************************************************************************/ /*!
@Function       RGXTBIBufferInitOnDemandResources

@Description    Allocates the firmware TBI buffer required for reading SFs
		strings and initialize it with SFs.

@Input          psDevInfo	 RGX device info

@Return		PVRSRV_OK	If all went good, PVRSRV_ERROR otherwise.
*/ /**************************************************************************/
PVRSRV_ERROR RGXTBIBufferInitOnDemandResources(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PVRSRV_ERROR       eError = PVRSRV_OK;
	IMG_UINT32         i, ui32Len;
	const IMG_UINT32   ui32FWTBIBufsize = g_ui32SFsCount * sizeof(RGXFW_STID_FMT);
	RGXFW_STID_FMT     *psFW_SFs = NULL;

	/* Firmware address should not be already set */
	if (psDevInfo->sRGXFWIfTBIBuffer.ui32Addr)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: FW address for FWTBI is already set. Resetting it with newly allocated one",
		         __func__));
	}

	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_GPU_RO_ALLOCFLAGS,
								  ui32FWTBIBufsize,
								  "FwTBIBuffer",
								  &psDevInfo->psRGXFWIfTBIBufferMemDesc,
								  &psDevInfo->sRGXFWIfTBIBuffer,
								  (void**)&psFW_SFs,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation", fail);

	/* Copy SFs entries to FW buffer */
	for (i = 0; i < g_ui32SFsCount; i++)
	{
		OSCachedMemCopy(&psFW_SFs[i].ui32Id, &SFs[i].ui32Id, sizeof(SFs[i].ui32Id));
		ui32Len = OSStringLength(SFs[i].psName);
		OSCachedMemCopy(psFW_SFs[i].sName, SFs[i].psName, MIN(ui32Len, IMG_SF_STRING_MAX_SIZE - 1));
	}

	/* flush write buffers for psFW_SFs */
	OSWriteMemoryBarrier(psFW_SFs);

	/* Set size of TBI buffer */
	psDevInfo->ui32FWIfTBIBufferSize = ui32FWTBIBufsize;

	/* release CPU mapping */
	DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfTBIBufferMemDesc);

	return PVRSRV_OK;
fail:
	RGXTBIBufferDeinit(psDevInfo);
	return eError;
}
#endif

/*************************************************************************/ /*!
@Function       RGXTraceBufferDeinit

@Description    Deinitialises all the allocations and references that are made
		for the FW trace buffer(s)

@Input          ppsDevInfo	 RGX device info
@Return		void
*/ /**************************************************************************/
static void RGXTraceBufferDeinit(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_UINT32 i;

	for (i = 0; i < RGXFW_THREAD_NUM; i++)
	{
		if (psDevInfo->psRGXFWIfTraceBufferMemDesc[i])
		{
			if (psDevInfo->apui32TraceBuffer[i] != NULL)
			{
				DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfTraceBufferMemDesc[i]);
				psDevInfo->apui32TraceBuffer[i] = NULL;
			}

			DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfTraceBufferMemDesc[i]);
			psDevInfo->psRGXFWIfTraceBufferMemDesc[i] = NULL;
		}
	}
}

/*************************************************************************/ /*!
@Function       RGXTraceBufferInitOnDemandResources

@Description    Allocates the firmware trace buffer required for dumping trace
		info from the firmware.

@Input          psDevInfo	 RGX device info

@Return		PVRSRV_OK	If all went good, PVRSRV_ERROR otherwise.
*/ /**************************************************************************/
PVRSRV_ERROR RGXTraceBufferInitOnDemandResources(PVRSRV_RGXDEV_INFO* psDevInfo,
												 PVRSRV_MEMALLOCFLAGS_T uiAllocFlags)
{
	RGXFWIF_TRACEBUF*  psTraceBufCtl = psDevInfo->psRGXFWIfTraceBufCtl;
	PVRSRV_ERROR       eError = PVRSRV_OK;
	IMG_UINT32         ui32FwThreadNum;
	IMG_UINT32         ui32DefaultTraceBufSize;
	IMG_DEVMEM_SIZE_T  uiTraceBufSizeInBytes;
	void               *pvAppHintState = NULL;
	IMG_CHAR           pszBufferName[] = "FwTraceBuffer_Thread0";

	/* Check AppHint value for module-param FWTraceBufSizeInDWords */
	OSCreateAppHintState(&pvAppHintState);
	ui32DefaultTraceBufSize = RGXFW_TRACE_BUF_DEFAULT_SIZE_IN_DWORDS;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE,
						 pvAppHintState,
						 FWTraceBufSizeInDWords,
						 &ui32DefaultTraceBufSize,
						 &psDevInfo->ui32TraceBufSizeInDWords);
	OSFreeAppHintState(pvAppHintState);
	pvAppHintState = NULL;

	/* Write tracebuf size once to devmem */
	psTraceBufCtl->ui32TraceBufSizeInDWords = psDevInfo->ui32TraceBufSizeInDWords;

	if (psDevInfo->ui32TraceBufSizeInDWords < RGXFW_TRACE_BUF_MIN_SIZE_IN_DWORDS ||
		psDevInfo->ui32TraceBufSizeInDWords > RGXFW_TRACE_BUF_MAX_SIZE_IN_DWORDS)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Requested trace buffer size (%u) out of its minimum (%u) & maximum (%u) range. Exiting error.",
				  __func__,
				  psDevInfo->ui32TraceBufSizeInDWords,
				  RGXFW_TRACE_BUF_MIN_SIZE_IN_DWORDS,
				  RGXFW_TRACE_BUF_MAX_SIZE_IN_DWORDS));
		eError = PVRSRV_ERROR_OUT_OF_RANGE;
		goto exit_error;
	}

	uiTraceBufSizeInBytes = psDevInfo->ui32TraceBufSizeInDWords * sizeof(IMG_UINT32);

	for (ui32FwThreadNum = 0; ui32FwThreadNum < RGXFW_THREAD_NUM; ui32FwThreadNum++)
	{
#if !defined(SUPPORT_AUTOVZ)
		/* Ensure allocation API is only called when not already allocated */
		PVR_ASSERT(psDevInfo->psRGXFWIfTraceBufferMemDesc[ui32FwThreadNum] == NULL);
		/* Firmware address should not be already set */
		PVR_ASSERT(psTraceBufCtl->sTraceBuf[ui32FwThreadNum].pui32RGXFWIfTraceBuffer.ui32Addr == 0x0);
#endif

		/* update the firmware thread number in the Trace Buffer's name */
		pszBufferName[sizeof(pszBufferName) - 2] += ui32FwThreadNum;

		eError = RGXSetupFwAllocation(psDevInfo,
									  uiAllocFlags,
									  uiTraceBufSizeInBytes,
									  pszBufferName,
									  &psDevInfo->psRGXFWIfTraceBufferMemDesc[ui32FwThreadNum],
									  &psTraceBufCtl->sTraceBuf[ui32FwThreadNum].pui32RGXFWIfTraceBuffer,
									  (void**)&psDevInfo->apui32TraceBuffer[ui32FwThreadNum],
									  RFW_FWADDR_NOREF_FLAG);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation", fail);
	}

	return PVRSRV_OK;

fail:
	RGXTraceBufferDeinit(psDevInfo);
exit_error:
	return eError;
}

#if defined(PDUMP)
/*************************************************************************/ /*!
@Function       RGXPDumpLoadFWInitData

@Description    Allocates the firmware trace buffer required for dumping trace
                info from the firmware.

@Input          psDevInfo RGX device info
 */ /*************************************************************************/
static void RGXPDumpLoadFWInitData(PVRSRV_RGXDEV_INFO *psDevInfo,
								   RGX_INIT_APPHINTS  *psApphints,
								   IMG_UINT32         ui32HWPerfCountersDataSize)
{
	IMG_UINT32 ui32ConfigFlags  = psDevInfo->psRGXFWIfFwSysData->ui32ConfigFlags;
	IMG_UINT32 ui32FwOsCfgFlags = psDevInfo->psRGXFWIfFwOsData->ui32FwOsConfigFlags;

	PDUMPCOMMENT(psDevInfo->psDeviceNode, "Dump RGXFW Init data");
	if (!psApphints->bEnableSignatureChecks)
	{
		PDUMPCOMMENT(psDevInfo->psDeviceNode,
					 "(to enable rgxfw signatures place the following line after the RTCONF line)");
		DevmemPDumpLoadMem(psDevInfo->psRGXFWIfSysInitMemDesc,
						   offsetof(RGXFWIF_SYSINIT, asSigBufCtl),
						   sizeof(RGXFWIF_SIGBUF_CTL)*(RGXFWIF_DM_MAX),
						   PDUMP_FLAGS_CONTINUOUS);
	}

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Dump initial state of FW runtime configuration");
	DevmemPDumpLoadMem(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
					   0,
					   sizeof(RGXFWIF_RUNTIME_CFG),
					   PDUMP_FLAGS_CONTINUOUS);

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Dump rgxfw hwperfctl structure");
	DevmemPDumpLoadZeroMem (psDevInfo->psRGXFWIfHWPerfCountersMemDesc,
							0,
							ui32HWPerfCountersDataSize,
							PDUMP_FLAGS_CONTINUOUS);

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Dump rgxfw trace control structure");
	DevmemPDumpLoadMem(psDevInfo->psRGXFWIfTraceBufCtlMemDesc,
					   0,
					   sizeof(RGXFWIF_TRACEBUF),
					   PDUMP_FLAGS_CONTINUOUS);

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Dump firmware system data structure");
	DevmemPDumpLoadMem(psDevInfo->psRGXFWIfFwSysDataMemDesc,
					   0,
					   sizeof(RGXFWIF_SYSDATA),
					   PDUMP_FLAGS_CONTINUOUS);

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Dump firmware OS data structure");
	DevmemPDumpLoadMem(psDevInfo->psRGXFWIfFwOsDataMemDesc,
					   0,
					   sizeof(RGXFWIF_OSDATA),
					   PDUMP_FLAGS_CONTINUOUS);

#if defined(SUPPORT_TBI_INTERFACE)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Dump rgx TBI buffer");
	DevmemPDumpLoadMem(psDevInfo->psRGXFWIfTBIBufferMemDesc,
					   0,
					   psDevInfo->ui32FWIfTBIBufferSize,
					   PDUMP_FLAGS_CONTINUOUS);
#endif /* defined(SUPPORT_TBI_INTERFACE) */

#if defined(SUPPORT_USER_REGISTER_CONFIGURATION)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Dump rgxfw register configuration buffer");
	DevmemPDumpLoadMem(psDevInfo->psRGXFWIfRegCfgMemDesc,
					   0,
					   sizeof(RGXFWIF_REG_CFG),
					   PDUMP_FLAGS_CONTINUOUS);
#endif /* defined(SUPPORT_USER_REGISTER_CONFIGURATION) */
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Dump rgxfw system init structure");
	DevmemPDumpLoadMem(psDevInfo->psRGXFWIfSysInitMemDesc,
					   0,
					   sizeof(RGXFWIF_SYSINIT),
					   PDUMP_FLAGS_CONTINUOUS);

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Dump rgxfw os init structure");
	DevmemPDumpLoadMem(psDevInfo->psRGXFWIfOsInitMemDesc,
					   0,
					   sizeof(RGXFWIF_OSINIT),
					   PDUMP_FLAGS_CONTINUOUS);

	/* RGXFW Init structure needs to be loaded before we overwrite FaultPhysAddr, else this address patching won't have any effect */
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Overwrite FaultPhysAddr of FwSysInit in pdump with actual physical address");
	RGXPDumpFaultReadRegister(psDevInfo);

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "RTCONF: run-time configuration");

	/* Dump the config options so they can be edited. */

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "(Set the FW system config options here)");
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Ctx Switch Rand mode:                      0x%08x)", RGXFWIF_INICFG_CTXSWITCH_MODE_RAND);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Ctx Switch Soft Reset Enable:              0x%08x)", RGXFWIF_INICFG_CTXSWITCH_SRESET_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Enable HWPerf:                             0x%08x)", RGXFWIF_INICFG_HWPERF_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Rascal+Dust Power Island:                  0x%08x)", RGXFWIF_INICFG_POW_RASCALDUST);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( FBCDC Version 3.1 Enable:                  0x%08x)", RGXFWIF_INICFG_FBCDC_V3_1_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Check MList:                               0x%08x)", RGXFWIF_INICFG_CHECK_MLIST_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Disable Auto Clock Gating:                 0x%08x)", RGXFWIF_INICFG_DISABLE_CLKGATING_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
#if defined(RGX_FEATURE_PIPELINED_DATAMASTERS_VERSION_MAX_VALUE_IDX)
				 "( Try overlapping DM pipelines:              0x%08x)", RGXFWIF_INICFG_TRY_OVERLAPPING_DM_PIPELINES_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Enable DM pipeline roadblocks:             0x%08x)", RGXFWIF_INICFG_DM_PIPELINE_ROADBLOCKS_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
#endif
				 "( Enable register configuration:             0x%08x)", RGXFWIF_INICFG_REGCONFIG_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Assert on TA Out-of-Memory:                0x%08x)", RGXFWIF_INICFG_ASSERT_ON_OUTOFMEMORY);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Disable HWPerf custom counter filter:      0x%08x)", RGXFWIF_INICFG_HWP_DISABLE_FILTER);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Enable Ctx Switch profile mode: 0x%08x (none=b'000, fast=b'001, medium=b'010, slow=b'011, nodelay=b'100))", RGXFWIF_INICFG_CTXSWITCH_PROFILE_MASK);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Disable DM overlap (except TA during SPM): 0x%08x)", RGXFWIF_INICFG_DISABLE_DM_OVERLAP);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Assert on HWR trigger (page fault, lockup, overrun or poll failure): 0x%08x)", RGXFWIF_INICFG_ASSERT_ON_HWR_TRIGGER);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Enable IRQ validation:                     0x%08x)", RGXFWIF_INICFG_VALIDATE_IRQ);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( SPU power state mask change Enable:        0x%08x)", RGXFWIF_INICFG_SPU_POWER_STATE_MASK_CHANGE_EN);
#if defined(SUPPORT_WORKLOAD_ESTIMATION)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		PDUMPCOMMENT(psDevInfo->psDeviceNode,
					 "( Enable Workload Estimation:                0x%08x)", RGXFWIF_INICFG_WORKEST);
#if defined(SUPPORT_PDVFS)
		PDUMPCOMMENT(psDevInfo->psDeviceNode,
					 "( Enable Proactive DVFS:                     0x%08x)", RGXFWIF_INICFG_PDVFS);
#endif /* defined(SUPPORT_PDVFS) */
	}
#endif /* defined(SUPPORT_WORKLOAD_ESTIMATION) */
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( ISP Scheduling Mode (v1=b'01, v2=b'10):    0x%08x)", RGXFWIF_INICFG_ISPSCHEDMODE_MASK);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Validate SOC & USC timers:                 0x%08x)", RGXFWIF_INICFG_VALIDATE_SOCUSC_TIMER);

	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfFwSysDataMemDesc,
							offsetof(RGXFWIF_SYSDATA, ui32ConfigFlags),
							ui32ConfigFlags,
							PDUMP_FLAGS_CONTINUOUS);

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Extended FW system config options not used.)");

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "(Set the FW OS config options here)");
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Ctx Switch TDM Enable:                     0x%08x)", RGXFWIF_INICFG_OS_CTXSWITCH_TDM_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Ctx Switch GEOM Enable:                    0x%08x)", RGXFWIF_INICFG_OS_CTXSWITCH_GEOM_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Ctx Switch 3D Enable:                      0x%08x)", RGXFWIF_INICFG_OS_CTXSWITCH_3D_EN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Ctx Switch CDM Enable:                     0x%08x)", RGXFWIF_INICFG_OS_CTXSWITCH_CDM_EN);
#if defined(RGX_FEATURE_RAY_TRACING_ARCH_MAX_VALUE_IDX)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Ctx Switch RDM Enable:                     0x%08x)", RGXFWIF_INICFG_OS_CTXSWITCH_RDM_EN);
#endif
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Lower Priority Ctx Switch  2D Enable:      0x%08x)", RGXFWIF_INICFG_OS_LOW_PRIO_CS_TDM);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Lower Priority Ctx Switch  TA Enable:      0x%08x)", RGXFWIF_INICFG_OS_LOW_PRIO_CS_GEOM);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Lower Priority Ctx Switch  3D Enable:      0x%08x)", RGXFWIF_INICFG_OS_LOW_PRIO_CS_3D);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Lower Priority Ctx Switch CDM Enable:      0x%08x)", RGXFWIF_INICFG_OS_LOW_PRIO_CS_CDM);
#if defined(RGX_FEATURE_RAY_TRACING_ARCH_MAX_VALUE_IDX)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Lower Priority Ctx Switch RDM Enable:      0x%08x)", RGXFWIF_INICFG_OS_LOW_PRIO_CS_RDM);
#endif

	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfFwOsDataMemDesc,
							  offsetof(RGXFWIF_OSDATA, ui32FwOsConfigFlags),
							  ui32FwOsCfgFlags,
							  PDUMP_FLAGS_CONTINUOUS);

#if defined(RGX_FEATURE_NUM_SPU_MAX_VALUE_IDX)
	{
		PDUMP_FLAGS_T      ui32PDumpFlags = PDUMP_FLAGS_CONTINUOUS;
		IMG_UINT32         ui32AllPowUnitsMask = (1 << psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount) - 1;
		IMG_BOOL           bRunTimeUpdate = IMG_FALSE;
		IMG_UINT32         ui32DstOffset = psDevInfo->psRGXFWIfRuntimeCfgMemDesc->uiOffset + offsetof(RGXFWIF_RUNTIME_CFG, ui32PowUnitsState);
		IMG_CHAR           aszPowUnitsMaskRegVar[] = ":SYSMEM:$1";
		IMG_CHAR           aszPowUnitsEnable[] = "RUNTIME_POW_UNITS_MASK";
		PMR                *psPMR = (PMR *)(psDevInfo->psRGXFWIfRuntimeCfgMemDesc->psImport->hPMR);


		if (bRunTimeUpdate)
		{
			PDUMPIF(psDevInfo->psDeviceNode, aszPowUnitsEnable, ui32PDumpFlags);
		}

		PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, ui32PDumpFlags,
		                      "Load initial value power units mask in FW runtime configuration");
		DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
								  ui32DstOffset,
								  psDevInfo->psRGXFWIfRuntimeCfg->ui32PowUnitsState,
								  ui32PDumpFlags);

		if (bRunTimeUpdate)
		{
			PDUMPELSE(psDevInfo->psDeviceNode, aszPowUnitsEnable, ui32PDumpFlags);
			PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, ui32PDumpFlags, "Read initial SPU mask value from HW registers");
			PDumpRegRead32ToInternalVar(psDevInfo->psDeviceNode, RGX_PDUMPREG_NAME, RGX_CR_SPU_ENABLE, aszPowUnitsMaskRegVar, ui32PDumpFlags);
			PDumpWriteVarANDValueOp(psDevInfo->psDeviceNode, aszPowUnitsMaskRegVar, ui32AllPowUnitsMask, ui32PDumpFlags);
			PDumpInternalVarToMemLabel(psPMR, ui32DstOffset, aszPowUnitsMaskRegVar, ui32PDumpFlags);
			PDUMPFI(psDevInfo->psDeviceNode, aszPowUnitsEnable, ui32PDumpFlags);
		}
	}
#endif

#if defined(SUPPORT_SECURITY_VALIDATION)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "(Select one or more security tests here)");
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Read/write FW private data from non-FW contexts: 0x%08x)", RGXFWIF_SECURE_ACCESS_TEST_READ_WRITE_FW_DATA);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Read/write FW code from non-FW contexts:         0x%08x)", RGXFWIF_SECURE_ACCESS_TEST_READ_WRITE_FW_CODE);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Execute FW code from non-secure memory:          0x%08x)", RGXFWIF_SECURE_ACCESS_TEST_RUN_FROM_NONSECURE);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Execute FW code from secure (non-FW) memory:     0x%08x)", RGXFWIF_SECURE_ACCESS_TEST_RUN_FROM_SECURE);

	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfSysInitMemDesc,
							  offsetof(RGXFWIF_SYSINIT, ui32SecurityTestFlags),
							  psDevInfo->psRGXFWIfSysInit->ui32SecurityTestFlags,
							  PDUMP_FLAGS_CONTINUOUS);
#endif /* defined(SUPPORT_SECURITY_VALIDATION) */

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( PID filter type: %X=INCLUDE_ALL_EXCEPT, %X=EXCLUDE_ALL_EXCEPT)",
				 RGXFW_PID_FILTER_INCLUDE_ALL_EXCEPT,
				 RGXFW_PID_FILTER_EXCLUDE_ALL_EXCEPT);

	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfSysInitMemDesc,
							  offsetof(RGXFWIF_SYSINIT, sPIDFilter.eMode),
							  psDevInfo->psRGXFWIfSysInit->sPIDFilter.eMode,
							  PDUMP_FLAGS_CONTINUOUS);

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( PID filter PID/DriverID list (Up to %u entries. Terminate with a zero PID))",
				 RGXFWIF_PID_FILTER_MAX_NUM_PIDS);
	{
		IMG_UINT32 i;

		/* generate a few WRWs in the pdump stream as an example */
		for (i = 0; i < MIN(RGXFWIF_PID_FILTER_MAX_NUM_PIDS, 8); i++)
		{
			/*
			 * Some compilers cannot cope with the uses of offsetof() below - the specific problem being the use of
			 * a non-const variable in the expression, which it needs to be const. Typical compiler output is
			 * "expression must have a constant value".
			 */
			const IMG_DEVMEM_OFFSET_T uiPIDOff
			= (IMG_DEVMEM_OFFSET_T)(uintptr_t)&(((RGXFWIF_SYSINIT *)0)->sPIDFilter.asItems[i].uiPID);

			const IMG_DEVMEM_OFFSET_T uiDriverIDOff
			= (IMG_DEVMEM_OFFSET_T)(uintptr_t)&(((RGXFWIF_SYSINIT *)0)->sPIDFilter.asItems[i].ui32DriverID);

			PDUMPCOMMENT(psDevInfo->psDeviceNode, "(PID and DriverID pair %u)", i);

			PDUMPCOMMENT(psDevInfo->psDeviceNode, "(PID)");
			DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfSysInitMemDesc,
									  uiPIDOff,
									  0,
									  PDUMP_FLAGS_CONTINUOUS);

			PDUMPCOMMENT(psDevInfo->psDeviceNode, "(DriverID)");
			DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfSysInitMemDesc,
									  uiDriverIDOff,
									  0,
									  PDUMP_FLAGS_CONTINUOUS);
		}
	}

	/*
	 * Dump the log config so it can be edited.
	 */
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "(Set the log config here)");
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( Log Type: TRACE mode using shared memory buffer: 0x00000001)");
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "(           TBI mode via external interface or sim support: 0x00000000)");
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "(           Note: TBI mode will hang on most hardware devices!)");
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( MAIN Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_MAIN);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( MTS Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_MTS);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( CLEANUP Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_CLEANUP);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( CSW Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_CSW);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( BIF Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_BIF);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( PM Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_PM);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( RTD Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_RTD);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( SPM Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_SPM);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( POW Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_POW);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( HWR Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_HWR);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( HWP Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_HWP);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( MISC Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_MISC);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( VZ Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_VZ);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( SAFETY Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_SAFETY);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( VERBOSE Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_VERBOSE);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( CUSTOMER Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_CUSTOMER);
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "( DEBUG Group Enable: 0x%08x)", RGXFWIF_LOG_TYPE_GROUP_DEBUG);
	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfTraceBufCtlMemDesc,
							  offsetof(RGXFWIF_TRACEBUF, ui32LogType),
							  psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType,
							  PDUMP_FLAGS_CONTINUOUS);

	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Set the HWPerf Filter config here, see \"hwperfbin2jsont -h\"");
	DevmemPDumpLoadMemValue64(psDevInfo->psRGXFWIfSysInitMemDesc,
							  offsetof(RGXFWIF_SYSINIT, ui64HWPerfFilter),
							  psDevInfo->psRGXFWIfSysInit->ui64HWPerfFilter,
							  PDUMP_FLAGS_CONTINUOUS);

#if defined(SUPPORT_USER_REGISTER_CONFIGURATION)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "(Number of registers configurations for types(byte index): pow on(%d), dust change(%d), ta(%d), 3d(%d), cdm(%d), "
#if defined(RGX_FEATURE_TLA_BIT_MASK)
				 "tla(%d), "
#endif
				 "tdm(%d))",
				 RGXFWIF_REG_CFG_TYPE_PWR_ON,
				 RGXFWIF_REG_CFG_TYPE_DUST_CHANGE,
				 RGXFWIF_REG_CFG_TYPE_TA,
				 RGXFWIF_REG_CFG_TYPE_3D,
				 RGXFWIF_REG_CFG_TYPE_CDM,
#if defined(RGX_FEATURE_TLA_BIT_MASK)
				 RGXFWIF_REG_CFG_TYPE_TLA,
#endif
				 RGXFWIF_REG_CFG_TYPE_TDM);

	{
		IMG_UINT32 i;

		/* Write 32 bits in each iteration as required by PDUMP WRW command */
		for (i = 0; i < RGXFWIF_REG_CFG_TYPE_ALL; i += sizeof(IMG_UINT32))
		{
			DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRegCfgMemDesc,
									offsetof(RGXFWIF_REG_CFG, aui8NumRegsType[i]),
									0,
									PDUMP_FLAGS_CONTINUOUS);
		}
	}

	PDUMPCOMMENT(psDevInfo->psDeviceNode, "(Set registers here: address, mask, value)");
	DevmemPDumpLoadMemValue64(psDevInfo->psRGXFWIfRegCfgMemDesc,
							  offsetof(RGXFWIF_REG_CFG, asRegConfigs[0].ui64Addr),
							  0,
							  PDUMP_FLAGS_CONTINUOUS);
	DevmemPDumpLoadMemValue64(psDevInfo->psRGXFWIfRegCfgMemDesc,
							  offsetof(RGXFWIF_REG_CFG, asRegConfigs[0].ui64Mask),
							  0,
							  PDUMP_FLAGS_CONTINUOUS);
	DevmemPDumpLoadMemValue64(psDevInfo->psRGXFWIfRegCfgMemDesc,
							  offsetof(RGXFWIF_REG_CFG, asRegConfigs[0].ui64Value),
							  0,
							  PDUMP_FLAGS_CONTINUOUS);
#endif /* SUPPORT_USER_REGISTER_CONFIGURATION */
}
#endif /* defined(PDUMP) */

/*!
*******************************************************************************
 @Function    RGXSetupFwGuardPage

 @Description Allocate a Guard Page at the start of a Guest's Main Heap

 @Input       psDevInfo

 @Return      PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXSetupFwGuardPage(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PVRSRV_ERROR eError;

	eError = RGXSetupFwAllocation(psDevInfo,
								  (RGX_FWSHAREDMEM_GPU_ONLY_ALLOCFLAGS |
								   PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN)),
								  OSGetPageSize(),
								  "FwGuardPage",
								  &psDevInfo->psRGXFWHeapGuardPageReserveMemDesc,
								  NULL,
								  NULL,
								  RFW_FWADDR_FLAG_NONE);

	return eError;
}

/*!
*******************************************************************************
 @Function    RGXSetupFwSysData

 @Description Sets up all system-wide firmware related data

 @Input       psDevInfo

 @Return      PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXSetupFwSysData(PVRSRV_DEVICE_NODE       *psDeviceNode,
									  RGX_INIT_APPHINTS        *psApphints,
									  IMG_UINT32               ui32ConfigFlags,
									  IMG_UINT32               ui32ConfigFlagsExt,
									  IMG_UINT32               ui32HWPerfCountersDataSize)
{
	PVRSRV_ERROR eError;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
#if defined(RGX_FEATURE_NUM_SPU_MAX_VALUE_IDX)
	IMG_UINT32         ui32AllPowUnitsMask = (1 << psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount) - 1;
	IMG_UINT32         ui32AllRACMask = (1 << psDevInfo->sDevFeatureCfg.ui32MAXRACCount) - 1;
#endif
	RGXFWIF_SYSINIT    *psFwSysInitScratch = NULL;
#if defined(PDUMP)
	IMG_UINT32         ui32SignatureChecksBufSize = psApphints->ui32SignatureChecksBufSize;
#endif

	psFwSysInitScratch = OSAllocZMem(sizeof(*psFwSysInitScratch));
	PVR_LOG_GOTO_IF_NOMEM(psFwSysInitScratch, eError, fail);

	/* Sys Fw init data */
	eError = RGXSetupFwAllocation(psDevInfo,
	                              (RGX_FWSHAREDMEM_CONFIG_ALLOCFLAGS |
	                              PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED)) &
	                              RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
	                              sizeof(RGXFWIF_SYSINIT),
	                              "FwSysInitStructure",
	                              &psDevInfo->psRGXFWIfSysInitMemDesc,
	                              NULL,
	                              (void**) &psDevInfo->psRGXFWIfSysInit,
	                              RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "Firmware Sys Init structure allocation", fail);

	/* Setup Fault read register */
	eError = RGXSetupFaultReadRegister(psDeviceNode, psFwSysInitScratch);
	PVR_LOG_GOTO_IF_ERROR(eError, "Fault read register setup", fail);

#if defined(SUPPORT_AUTOVZ)
	psFwSysInitScratch->ui32VzWdgPeriod = PVR_AUTOVZ_WDG_PERIOD_MS;
#endif

	/* RD Power Island */
	{
		RGX_DATA *psRGXData = (RGX_DATA*) psDeviceNode->psDevConfig->hDevData;
		IMG_BOOL bSysEnableRDPowIsland = psRGXData->psRGXTimingInfo->bEnableRDPowIsland;
		IMG_BOOL bEnableRDPowIsland = ((psApphints->eRGXRDPowerIslandConf == RGX_RD_POWER_ISLAND_DEFAULT) && bSysEnableRDPowIsland) ||
		                               (psApphints->eRGXRDPowerIslandConf == RGX_RD_POWER_ISLAND_FORCE_ON);

		ui32ConfigFlags |= bEnableRDPowIsland? RGXFWIF_INICFG_POW_RASCALDUST : 0;
	}


	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
#if defined(SUPPORT_WORKLOAD_ESTIMATION)
		ui32ConfigFlags |= RGXFWIF_INICFG_WORKEST;
#endif
#if defined(SUPPORT_FW_OPP_TABLE)
		{
			RGXFWIF_OPP_INFO    *psOPPInfo;
			IMG_DVFS_DEVICE_CFG *psDVFSDeviceCfg;

			/* Pro-active DVFS depends on Workload Estimation */
			psOPPInfo = &psFwSysInitScratch->sOPPInfo;
			psDVFSDeviceCfg = &psDeviceNode->psDevConfig->sDVFS.sDVFSDeviceCfg;
#if defined(CONFIG_OF) && defined(CONFIG_PM_OPP) && !defined(NO_HARDWARE)
			if (psDVFSDeviceCfg->bDTConfig)
			{
				/* OPP table configured from Device tree */
				eError = DVFSCopyOPPTable(psDeviceNode,
										  psOPPInfo,
										  ARRAY_SIZE(psOPPInfo->asOPPValues));
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR, "Unable to copy OPP table to FW init buffer (%u)", eError));
					goto fail;
				}
			}
#endif
			if (!psDVFSDeviceCfg->bDTConfig)
			{
				PVR_LOG_IF_FALSE(psDVFSDeviceCfg->pasOPPTable, "RGXSetupFwSysData: Missing OPP Table");

				if (psDVFSDeviceCfg->pasOPPTable != NULL)
				{
					if (psDVFSDeviceCfg->ui32OPPTableSize > ARRAY_SIZE(psOPPInfo->asOPPValues))
					{
						PVR_DPF((PVR_DBG_ERROR,
								 "%s: OPP Table too large: Size = %u, Maximum size = %lu",
								 __func__,
								 psDVFSDeviceCfg->ui32OPPTableSize,
								 (unsigned long)(ARRAY_SIZE(psOPPInfo->asOPPValues))));
						eError = PVRSRV_ERROR_INVALID_PARAMS;
						goto fail;
					}

					OSDeviceMemCopy(psOPPInfo->asOPPValues,
									psDVFSDeviceCfg->pasOPPTable,
									sizeof(psOPPInfo->asOPPValues));

					psOPPInfo->ui32MaxOPPPoint = psDVFSDeviceCfg->ui32OPPTableSize - 1;

				}
			}

#if defined(SUPPORT_PDVFS)
			ui32ConfigFlags |= RGXFWIF_INICFG_PDVFS;
#endif
		}
#endif /* defined(SUPPORT_FW_OPP_TABLE) */
	}

	/* FW trace control structure */
	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS &
								  RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
								  sizeof(RGXFWIF_TRACEBUF),
								  "FwTraceCtlStruct",
								  &psDevInfo->psRGXFWIfTraceBufCtlMemDesc,
								  &psFwSysInitScratch->sTraceBufCtl,
								  (void**) &psDevInfo->psRGXFWIfTraceBufCtl,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation", fail);

	if (!psDeviceNode->bAutoVzFwIsUp)
	{
		/* Set initial firmware log type/group(s) */
		if (psApphints->ui32LogType & ~RGXFWIF_LOG_TYPE_MASK)
		{
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Invalid initial log type (0x%X)",
			         __func__, psApphints->ui32LogType));
			goto fail;
		}
		psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType = psApphints->ui32LogType;
		RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfTraceBufCtl->ui32LogType, FLUSH);
	}

	/* When PDUMP is enabled, ALWAYS allocate on-demand trace buffer resource
	 * (irrespective of loggroup(s) enabled), given that logtype/loggroups can
	 * be set during PDump playback in logconfig, at any point of time,
	 * Otherwise, allocate only if required. */
#if !defined(PDUMP)
#if defined(SUPPORT_AUTOVZ)
	/* always allocate trace buffer for AutoVz Host drivers to allow
	 * deterministic addresses of all SysData structures */
	if ((PVRSRV_VZ_MODE_IS(HOST, DEVINFO, psDevInfo)) || (RGXTraceBufferIsInitRequired(psDevInfo)))
#else
	if (RGXTraceBufferIsInitRequired(psDevInfo))
#endif
#endif
	{
		eError = RGXTraceBufferInitOnDemandResources(psDevInfo,
													 RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS &
													 RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp));
	}
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXTraceBufferInitOnDemandResources", fail);

	eError = RGXSetupFwAllocation(psDevInfo,
	                              RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS &
	                              RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
	                              sizeof(RGXFWIF_SYSDATA),
	                              "FwSysData",
	                              &psDevInfo->psRGXFWIfFwSysDataMemDesc,
	                              &psFwSysInitScratch->sFwSysData,
	                              (void**) &psDevInfo->psRGXFWIfFwSysData,
	                              RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation", fail);

	/* GPIO validation setup */
	psFwSysInitScratch->eGPIOValidationMode = RGXFWIF_GPIO_VAL_OFF;

#if defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
	eError = RGXFWSetupCounterBuffer(psDevInfo,
									 &psDevInfo->psCounterBufferMemDesc,
									 OSGetPageSize(),
									 &psFwSysInitScratch->sCounterDumpCtl);
	PVR_LOG_GOTO_IF_ERROR(eError, "Counter Buffer allocation", fail);

	PVR_DPF((PVR_DBG_WARNING, "Counter buffer allocated at %p, size %zu Bytes.", psDevInfo->psCounterBufferMemDesc, OSGetPageSize()));
#endif /* defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS) */


#if defined(SUPPORT_FIRMWARE_GCOV)
	eError = RGXFWSetupFirmwareGcovBuffer(psDevInfo,
	                                      &psDevInfo->psFirmwareGcovBufferMemDesc,
	                                      RGXFWIF_FIRMWARE_GCOV_BUFFER_SIZE,
	                                      &psFwSysInitScratch->sFirmwareGcovCtl,
	                                      "FirmwareGcovBuffer");
	PVR_LOG_GOTO_IF_ERROR(eError, "Firmware GCOV buffer allocation", fail);
	psDevInfo->ui32FirmwareGcovSize = RGXFWIF_FIRMWARE_GCOV_BUFFER_SIZE;
#endif /* defined(SUPPORT_FIRMWARE_GCOV) */

#if defined(PDUMP)
	/* Require a minimum amount of memory for the signature buffers */
	if (ui32SignatureChecksBufSize < RGXFW_SIG_BUFFER_SIZE_MIN)
	{
		ui32SignatureChecksBufSize = RGXFW_SIG_BUFFER_SIZE_MIN;
	}

	/* Setup Signature and Checksum Buffers */
	psDevInfo->psRGXFWSigTDMChecksMemDesc = NULL;
	psDevInfo->ui32SigTDMChecksSize = 0;

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, TDM_PDS_CHECKSUM))
	{
		/* Buffer allocated only when feature present because all known TDM
		 * signature registers are dependent on this feature being present */
		eError = RGXFWSetupSignatureChecks(psDevInfo,
		                                   &psDevInfo->psRGXFWSigTDMChecksMemDesc,
		                                   ui32SignatureChecksBufSize,
		                                   &psFwSysInitScratch->asSigBufCtl[RGXFWIF_DM_TDM]);
		PVR_LOG_GOTO_IF_ERROR(eError, "TDM Signature check setup", fail);
		psDevInfo->ui32SigTDMChecksSize = ui32SignatureChecksBufSize;
	}

	eError = RGXFWSetupSignatureChecks(psDevInfo,
	                                   &psDevInfo->psRGXFWSigTAChecksMemDesc,
	                                   ui32SignatureChecksBufSize,
	                                   &psFwSysInitScratch->asSigBufCtl[RGXFWIF_DM_GEOM]);
	PVR_LOG_GOTO_IF_ERROR(eError, "GEOM Signature check setup", fail);
	psDevInfo->ui32SigTAChecksSize = ui32SignatureChecksBufSize;

	eError = RGXFWSetupSignatureChecks(psDevInfo,
	                                   &psDevInfo->psRGXFWSig3DChecksMemDesc,
	                                   ui32SignatureChecksBufSize,
	                                   &psFwSysInitScratch->asSigBufCtl[RGXFWIF_DM_3D]);
	PVR_LOG_GOTO_IF_ERROR(eError, "3D Signature check setup", fail);
	psDevInfo->ui32Sig3DChecksSize = ui32SignatureChecksBufSize;


#if defined(RGX_FEATURE_RAY_TRACING_ARCH_MAX_VALUE_IDX)
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, RAY_TRACING_ARCH) &&
		RGX_GET_FEATURE_VALUE(psDevInfo, RAY_TRACING_ARCH) > 1)
	{
		eError = RGXFWSetupSignatureChecks(psDevInfo,
		                                   &psDevInfo->psRGXFWSigRDMChecksMemDesc,
		                                   ui32SignatureChecksBufSize,
		                                   &psFwSysInitScratch->asSigBufCtl[RGXFWIF_DM_RAY]);
		PVR_LOG_GOTO_IF_ERROR(eError, "RDM Signature check setup", fail);
		psDevInfo->ui32SigRDMChecksSize = ui32SignatureChecksBufSize;
	}
#endif

	if (!psApphints->bEnableSignatureChecks)
	{
		psFwSysInitScratch->asSigBufCtl[RGXFWIF_DM_TDM].sBuffer.ui32Addr = 0x0;
		psFwSysInitScratch->asSigBufCtl[RGXFWIF_DM_GEOM].sBuffer.ui32Addr = 0x0;
		psFwSysInitScratch->asSigBufCtl[RGXFWIF_DM_3D].sBuffer.ui32Addr = 0x0;
		psFwSysInitScratch->asSigBufCtl[RGXFWIF_DM_CDM].sBuffer.ui32Addr = 0x0;
#if defined(RGX_FEATURE_RAY_TRACING_ARCH_MAX_VALUE_IDX)
		psFwSysInitScratch->asSigBufCtl[RGXFWIF_DM_RAY].sBuffer.ui32Addr = 0x0;
#endif
	}
#endif /* defined(PDUMP) */

	eError = RGXFWSetupAlignChecks(psDeviceNode,
	                               &psFwSysInitScratch->sAlignChecks);
	PVR_LOG_GOTO_IF_ERROR(eError, "Alignment checks setup", fail);

	psFwSysInitScratch->ui32FilterFlags = psApphints->ui32FilterFlags;

	/* Fill the remaining bits of fw the init data */
	psFwSysInitScratch->sPDSExecBase.uiAddr = RGX_PDSCODEDATA_HEAP_BASE;
	psFwSysInitScratch->sUSCExecBase.uiAddr = RGX_USCCODE_HEAP_BASE;

#if defined(FIX_HW_BRN_65273_BIT_MASK)
	if (RGX_IS_BRN_SUPPORTED(psDevInfo, 65273))
	{
		/* Fill the remaining bits of fw the init data */
		psFwSysInitScratch->sPDSExecBase.uiAddr = RGX_PDSCODEDATA_BRN_65273_HEAP_BASE;
		psFwSysInitScratch->sUSCExecBase.uiAddr = RGX_USCCODE_BRN_65273_HEAP_BASE;
	}
#endif

#if defined(SUPPORT_PDVFS)
	/* Core clock rate */
	eError = RGXSetupFwAllocation(psDevInfo,
	                              RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS &
	                              RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
	                              sizeof(IMG_UINT32),
	                              "FwPDVFSCoreClkRate",
	                              &psDevInfo->psRGXFWIFCoreClkRateMemDesc,
	                              &psFwSysInitScratch->sCoreClockRate,
	                              (void**) &psDevInfo->pui32RGXFWIFCoreClkRate,
	                              RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "PDVFS core clock rate memory setup", fail);
#endif
	{
	/* Timestamps */
	PVRSRV_MEMALLOCFLAGS_T uiMemAllocFlags =
		PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN) |
		PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT) |
		PVRSRV_MEMALLOCFLAG_GPU_READABLE | /* XXX ?? */
		PVRSRV_MEMALLOCFLAG_GPU_UNCACHED |
		PVRSRV_MEMALLOCFLAG_GPU_WRITEABLE |
		PVRSRV_MEMALLOCFLAG_CPU_READABLE |
		PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC |
		PVRSRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE |
		PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC;

	/*
	  the timer query arrays
	*/
	PDUMPCOMMENT(psDeviceNode, "Allocate timer query arrays (FW)");
	eError = DevmemFwAllocate(psDevInfo,
	                          sizeof(IMG_UINT64) * RGX_MAX_TIMER_QUERIES,
	                          uiMemAllocFlags |
	                          PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE,
	                          "FwStartTimesArray",
	                          & psDevInfo->psStartTimeMemDesc);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to map start times array",
				__func__));
		goto fail;
	}

	eError = DevmemAcquireCpuVirtAddr(psDevInfo->psStartTimeMemDesc,
	                                  (void **)& psDevInfo->pui64StartTimeById);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to map start times array",
				__func__));
		goto fail;
	}

	eError = DevmemFwAllocate(psDevInfo,
	                          sizeof(IMG_UINT64) * RGX_MAX_TIMER_QUERIES,
	                          uiMemAllocFlags |
	                          PVRSRV_MEMALLOCFLAG_CPU_WRITEABLE,
	                          "FwEndTimesArray",
	                          & psDevInfo->psEndTimeMemDesc);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to map end times array",
				__func__));
		goto fail;
	}

	eError = DevmemAcquireCpuVirtAddr(psDevInfo->psEndTimeMemDesc,
	                                  (void **)& psDevInfo->pui64EndTimeById);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to map end times array",
				__func__));
		goto fail;
	}

	eError = DevmemFwAllocate(psDevInfo,
	                          sizeof(IMG_UINT32) * RGX_MAX_TIMER_QUERIES,
	                          uiMemAllocFlags,
	                          "FwCompletedOpsArray",
	                          & psDevInfo->psCompletedMemDesc);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to completed ops array",
				__func__));
		goto fail;
	}

	eError = DevmemAcquireCpuVirtAddr(psDevInfo->psCompletedMemDesc,
	                                  (void **)& psDevInfo->pui32CompletedById);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to map completed ops array",
				__func__));
		goto fail;
	}
	}
#if !defined(PVRSRV_USE_BRIDGE_LOCK)
	eError = OSLockCreate(&psDevInfo->hTimerQueryLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to allocate log for timer query",
				__func__));
		goto fail;
	}
#endif
#if defined(SUPPORT_TBI_INTERFACE)
#if !defined(PDUMP)
	/* allocate only if required */
	if (RGXTBIBufferIsInitRequired(psDevInfo))
#endif /* !defined(PDUMP) */
	{
		/* When PDUMP is enabled, ALWAYS allocate on-demand TBI buffer resource
		 * (irrespective of loggroup(s) enabled), given that logtype/loggroups
		 * can be set during PDump playback in logconfig, at any point of time
		 */
		eError = RGXTBIBufferInitOnDemandResources(psDevInfo);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXTBIBufferInitOnDemandResources", fail);
	}

	psFwSysInitScratch->sTBIBuf = psDevInfo->sRGXFWIfTBIBuffer;
#endif /* defined(SUPPORT_TBI_INTERFACE) */

	/* Allocate shared buffer for GPU utilisation.
	 * Enable FIRMWARE_CACHED to reduce read latency in the FW.
	 * The FW flushes the cache after any writes.
	 */
	eError = RGXSetupFwAllocation(psDevInfo,
								  (RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS |
								  PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED)) &
								  RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
								  sizeof(RGXFWIF_GPU_UTIL_FW),
								  "FwGPUUtilisationBuffer",
								  &psDevInfo->psRGXFWIfGpuUtilFWCtlMemDesc,
								  &psFwSysInitScratch->sGpuUtilFWCtl,
								  (void**) &psDevInfo->psRGXFWIfGpuUtilFW,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "GPU Utilisation Buffer ctl allocation", fail);

	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_GPU_RO_ALLOCFLAGS &
								  RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
								  sizeof(RGXFWIF_RUNTIME_CFG),
								  "FwRuntimeCfg",
								  &psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
								  &psFwSysInitScratch->sRuntimeCfg,
								  (void**) &psDevInfo->psRGXFWIfRuntimeCfg,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "Firmware runtime configuration memory allocation", fail);

#if defined(SUPPORT_USER_REGISTER_CONFIGURATION)
	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS &
								  RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
								  sizeof(RGXFWIF_REG_CFG),
								  "FwRegisterConfigStructure",
								  &psDevInfo->psRGXFWIfRegCfgMemDesc,
								  &psFwSysInitScratch->sRegCfg,
								  NULL,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "Firmware register user configuration structure allocation", fail);
#endif

#if defined(SUPPORT_SECURE_CONTEXT_SWITCH)
	eError = RGXSetupFwAllocation(psDevInfo,
								  (RGX_FWSHAREDMEM_GPU_ONLY_ALLOCFLAGS | PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_MAIN)) &
								  RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
								  RGXFW_SCRATCH_BUF_SIZE,
								  "FwScratchBuf",
								  &psDevInfo->psRGXFWScratchBufMemDesc,
								  &psFwSysInitScratch->pbFwScratchBuf,
								  NULL,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "Firmware scratch buffer allocation", fail);
#endif

	psDevInfo->ui32RGXFWIfHWPerfBufSize = GetHwPerfBufferSize(psApphints->ui32HWPerfFWBufSize);

	/* Host L2 HWPERF buffer size in bytes must be bigger than the L1 buffer
	 * accessed by the FW. The MISR may try to write one packet the size of the L1
	 * buffer in some scenarios. When logging is enabled in the MISR, it can be seen
	 * if the L2 buffer hits a full condition. The closer in size the L2 and L1 buffers
	 * are the more chance of this happening.
	 * Size chosen to allow MISR to write an L1 sized packet and for the client
	 * application/daemon to drain a L1 sized packet e.g. ~ 1.5*L1.
	 */
	psDevInfo->ui32RGXL2HWPerfBufSize = psDevInfo->ui32RGXFWIfHWPerfBufSize +
	        (psDevInfo->ui32RGXFWIfHWPerfBufSize>>1);

	/* Second stage initialisation or HWPerf, hHWPerfLock created in first
	 * stage. See RGXRegisterDevice() call to RGXHWPerfInit(). */
	if (psDevInfo->ui64HWPerfFilter[RGX_HWPERF_L2_STREAM_HWPERF] == 0)
	{
		psFwSysInitScratch->ui64HWPerfFilter =
		    RGXHWPerfFwSetEventFilter(psDevInfo, RGX_HWPERF_L2_STREAM_HWPERF,
		                              (IMG_UINT64) psApphints->ui32HWPerfFilter0 |
		                              ((IMG_UINT64) psApphints->ui32HWPerfFilter1 << 32));
	}
	else
	{
		/* The filter has already been modified. This can happen if
		 * pvr/apphint/EnableFTraceGPU was enabled. */
		psFwSysInitScratch->ui64HWPerfFilter = psDevInfo->ui64HWPerfFwFilter;
	}

#if !defined(PDUMP)
	/* Allocate if HWPerf filter has already been set. This is possible either
	 * by setting a proper AppHint or enabling GPU ftrace events. */
	if (psFwSysInitScratch->ui64HWPerfFilter != 0)
#endif
	{
		/* When PDUMP is enabled, ALWAYS allocate on-demand HWPerf resources
		 * (irrespective of HWPerf enabled or not), given that HWPerf can be
		 * enabled during PDump playback via RTCONF at any point of time. */
		eError = RGXHWPerfInitOnDemandL1Buffer(psDevInfo);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXHWPerfInitOnDemandL1Buffer", fail);

		eError = RGXHWPerfInitOnDemandL2Stream(psDevInfo, RGX_HWPERF_L2_STREAM_HWPERF);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXHWPerfInitOnDemandL2Stream", fail);
	}

	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWCOMCTX_ALLOCFLAGS &
								  RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp),
								  ui32HWPerfCountersDataSize,
								  "FwHWPerfControlStructure",
								  &psDevInfo->psRGXFWIfHWPerfCountersMemDesc,
								  &psFwSysInitScratch->sHWPerfCtl,
								  NULL,
								  RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "Firmware HW Perf control struct allocation", fail);

	psDevInfo->bPDPEnabled = (ui32ConfigFlags & RGXFWIF_INICFG_DISABLE_PDP_EN)
							  ? IMG_FALSE : IMG_TRUE;

	psFwSysInitScratch->eFirmwarePerf = psApphints->eFirmwarePerf;

#if defined(PDUMP)
	/* default: no filter */
	psFwSysInitScratch->sPIDFilter.eMode = RGXFW_PID_FILTER_INCLUDE_ALL_EXCEPT;
	psFwSysInitScratch->sPIDFilter.asItems[0].uiPID = 0;
#endif


#if defined(SUPPORT_SECURITY_VALIDATION)
	{
		PVRSRV_MEMALLOCFLAGS_T uiFlags = RGX_FWSHAREDMEM_GPU_ONLY_ALLOCFLAGS;
		PVRSRV_SET_PHYS_HEAP_HINT(FW_PRIV_DATA, uiFlags);

		PDUMPCOMMENT(psDeviceNode, "Allocate non-secure buffer for security validation test");
		eError = DevmemFwAllocateExportable(psDeviceNode,
											OSGetPageSize(),
											OSGetPageSize(),
											RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS,
											"FwExNonSecureBuffer",
											&psDevInfo->psRGXFWIfNonSecureBufMemDesc);
		PVR_LOG_GOTO_IF_ERROR(eError, "Non-secure buffer allocation", fail);

		eError = RGXSetFirmwareAddress(&psFwSysInitScratch->pbNonSecureBuffer,
									   psDevInfo->psRGXFWIfNonSecureBufMemDesc,
									   0, RFW_FWADDR_NOREF_FLAG);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:1", fail);

		PDUMPCOMMENT(psDeviceNode, "Allocate secure buffer for security validation test");
		eError = DevmemFwAllocateExportable(psDeviceNode,
											OSGetPageSize(),
											OSGetPageSize(),
											uiFlags,
											"FwExSecureBuffer",
											&psDevInfo->psRGXFWIfSecureBufMemDesc);
		PVR_LOG_GOTO_IF_ERROR(eError, "Secure buffer allocation", fail);

		eError = RGXSetFirmwareAddress(&psFwSysInitScratch->pbSecureBuffer,
									   psDevInfo->psRGXFWIfSecureBufMemDesc,
									   0, RFW_FWADDR_NOREF_FLAG);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetFirmwareAddress:2", fail);
	}
#endif /* SUPPORT_SECURITY_VALIDATION */

#if defined(RGX_FEATURE_TFBC_LOSSY_37_PERCENT_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, TFBC_LOSSY_37_PERCENT) || RGX_IS_FEATURE_SUPPORTED(psDevInfo, TFBC_DELTA_CORRELATION))
	{
		psFwSysInitScratch->ui32TFBCCompressionControl =
			(ui32ConfigFlagsExt & RGXFWIF_INICFG_EXT_TFBC_CONTROL_MASK) >> RGXFWIF_INICFG_EXT_TFBC_CONTROL_SHIFT;
	}
#endif /* RGX_FEATURE_TFBC_LOSSY_37_PERCENT_BIT_MASK */

	/* Initialize FW started flag */
	psFwSysInitScratch->bFirmwareStarted = IMG_FALSE;
	psFwSysInitScratch->ui32MarkerVal = 1;
	psDevInfo->psRGXFWIfRuntimeCfg->ui32VzConnectionCooldownPeriodInSec = RGX_VZ_CONNECTION_COOLDOWN_PERIOD;

	if (!psDeviceNode->bAutoVzFwIsUp)
	{
		IMG_UINT32 ui32DriverID;

		RGX_DATA *psRGXData = (RGX_DATA*) psDeviceNode->psDevConfig->hDevData;
		RGXFWIF_RUNTIME_CFG *psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;

		/* Required info by FW to calculate the ActivePM idle timer latency */
		psFwSysInitScratch->ui32InitialCoreClockSpeed = psRGXData->psRGXTimingInfo->ui32CoreClockSpeed;
#if defined(SUPPORT_SOC_TIMER)
		psFwSysInitScratch->ui32InitialSOCClockSpeed = psRGXData->psRGXTimingInfo->ui32SOCClockSpeed;
#endif
		psFwSysInitScratch->ui32InitialActivePMLatencyms = psRGXData->psRGXTimingInfo->ui32ActivePMLatencyms;

		/* Initialise variable runtime configuration to the system defaults */
		psRuntimeCfg->ui32CoreClockSpeed = psFwSysInitScratch->ui32InitialCoreClockSpeed;
#if defined(SUPPORT_SOC_TIMER)
		psRuntimeCfg->ui32SOCClockSpeed = psFwSysInitScratch->ui32InitialSOCClockSpeed;
#endif
		psRuntimeCfg->ui32ActivePMLatencyms = psFwSysInitScratch->ui32InitialActivePMLatencyms;
		psRuntimeCfg->bActivePMLatencyPersistant = IMG_TRUE;
		psRuntimeCfg->ui32HCSDeadlineMS = RGX_HCS_DEFAULT_DEADLINE_MS;

		if ((RGXFW_SAFETY_WATCHDOG_PERIOD_IN_US > 0U) && (RGXFW_SAFETY_WATCHDOG_PERIOD_IN_US < 1000U))
		{
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			PVR_LOG_GOTO_IF_ERROR(eError,
								  "RGXSetupFwSysData: RGXFW_SAFETY_WATCHDOG_PERIOD_IN_US must be either 0 (disabled) or greater than 1000",
								  fail);
		}
		else
		{
			psRuntimeCfg->ui32WdgPeriodUs = RGXFW_SAFETY_WATCHDOG_PERIOD_IN_US;
		}

		if (PVRSRV_VZ_MODE_IS(NATIVE, DEVINFO, psDevInfo))
		{
			psRuntimeCfg->ai32DriverPriority[RGXFW_HOST_DRIVER_ID] = 0;
			psRuntimeCfg->aui32DriverIsolationGroup[RGXFW_HOST_DRIVER_ID] = RGX_DRIVERID_0_DEFAULT_ISOLATION_GROUP;
			psRuntimeCfg->aui32TSPercentage[RGXFW_HOST_DRIVER_ID] = (IMG_UINT8)RGX_DRIVERID_0_DEFAULT_TIME_SLICE;
		}
		else
		{
			const RGX_QOS_DEFAULTS asQosDefaults[RGXFW_MAX_NUM_OSIDS] = {
					RGX_QOS_DEFAULTS_INIT(0),
#if (RGXFW_MAX_NUM_OSIDS > 1)
					RGX_QOS_DEFAULTS_INIT(1),
#if (RGXFW_MAX_NUM_OSIDS > 2)
					RGX_QOS_DEFAULTS_INIT(2),
					RGX_QOS_DEFAULTS_INIT(3),
					RGX_QOS_DEFAULTS_INIT(4),
					RGX_QOS_DEFAULTS_INIT(5),
					RGX_QOS_DEFAULTS_INIT(6),
					RGX_QOS_DEFAULTS_INIT(7),
#if (RGXFW_MAX_NUM_OSIDS > 8)
					RGX_QOS_DEFAULTS_INIT(8),
					RGX_QOS_DEFAULTS_INIT(9),
					RGX_QOS_DEFAULTS_INIT(10),
					RGX_QOS_DEFAULTS_INIT(11),
					RGX_QOS_DEFAULTS_INIT(12),
					RGX_QOS_DEFAULTS_INIT(13),
					RGX_QOS_DEFAULTS_INIT(14),
					RGX_QOS_DEFAULTS_INIT(15),
					RGX_QOS_DEFAULTS_INIT(16),
					RGX_QOS_DEFAULTS_INIT(17),
					RGX_QOS_DEFAULTS_INIT(18),
					RGX_QOS_DEFAULTS_INIT(19),
					RGX_QOS_DEFAULTS_INIT(20),
					RGX_QOS_DEFAULTS_INIT(21),
					RGX_QOS_DEFAULTS_INIT(22),
					RGX_QOS_DEFAULTS_INIT(23),
					RGX_QOS_DEFAULTS_INIT(24),
					RGX_QOS_DEFAULTS_INIT(25),
					RGX_QOS_DEFAULTS_INIT(26),
					RGX_QOS_DEFAULTS_INIT(27),
					RGX_QOS_DEFAULTS_INIT(28),
					RGX_QOS_DEFAULTS_INIT(29),
					RGX_QOS_DEFAULTS_INIT(30),
					RGX_QOS_DEFAULTS_INIT(31),
#if (RGXFW_MAX_NUM_OSIDS > 32)
#error "Support for more than 32 OSIDs not implemented."
#endif
#endif /* RGXFW_MAX_NUM_OSIDS > 8 */
#endif /* RGXFW_MAX_NUM_OSIDS > 2 */

#endif /* RGXFW_MAX_NUM_OSIDS > 1 */
			};

			FOREACH_SUPPORTED_DRIVER(ui32DriverID)
			{
				/* Set up initial priorities between different OSes */
				psRuntimeCfg->ai32DriverPriority[ui32DriverID] = asQosDefaults[ui32DriverID].i32Priority;
				psRuntimeCfg->aui32DriverIsolationGroup[ui32DriverID] = asQosDefaults[ui32DriverID].ui32IsolationGroups;
				psRuntimeCfg->aui32TSPercentage[ui32DriverID] = (asQosDefaults[ui32DriverID].ui32TSPercentage <=
																				PVRSRV_VZ_TIME_SLICE_MAX) ?
																				asQosDefaults[ui32DriverID].ui32TSPercentage:(0);
			}
		}
		psRuntimeCfg->ui32TSIntervalMs = RGX_DRIVER_DEFAULT_TIME_SLICE_INTERVAL;

#if defined(PVR_ENABLE_PHR) && defined(PDUMP)
		psRuntimeCfg->ui32PHRMode = RGXFWIF_PHR_MODE_RD_RESET;
#else
		psRuntimeCfg->ui32PHRMode = 0;
#endif

		/* Initialize the PowUnitsState Field to Max Dusts */
		psRuntimeCfg->ui32PowUnitsState = psDevInfo->sDevFeatureCfg.ui32MAXPowUnitCount;

		/* flush write buffers for psDevInfo->psRGXFWIfRuntimeCfg */
		OSWriteMemoryBarrier(psDevInfo->psRGXFWIfRuntimeCfg);
		RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfRuntimeCfg, FLUSH);

		/* Setup FW coremem data */
		if (psDevInfo->psRGXFWIfCorememDataStoreMemDesc)
		{
			psFwSysInitScratch->sCorememDataStore.pbyFWAddr = psDevInfo->sFWCorememDataStoreFWAddr;
		}

		psDevInfo->psRGXFWIfFwSysData->ui32ConfigFlags    = ui32ConfigFlags    & RGXFWIF_INICFG_ALL;
		psDevInfo->psRGXFWIfFwSysData->ui32ConfigFlagsExt = ui32ConfigFlagsExt & RGXFWIF_INICFG_EXT_ALL;

		/* Initialise GPU utilisation buffer */
		{
			IMG_UINT64 ui64LastWord = RGXFWIF_GPU_UTIL_MAKE_WORD(OSClockns64(), RGXFWIF_GPU_UTIL_STATE_IDLE);
			RGXFWIF_DM eDM;
			IMG_UINT64 ui64LastWordTimeShifted =
			        RGXFWIF_GPU_UTIL_MAKE_WORD(OSClockns64() >> RGXFWIF_DM_OS_TIMESTAMP_SHIFT, RGXFWIF_GPU_UTIL_STATE_IDLE);
			IMG_UINT32 ui32DriverID;

			psDevInfo->psRGXFWIfGpuUtilFW->ui64GpuLastWord = ui64LastWord;

			FOREACH_SUPPORTED_DRIVER(ui32DriverID)
			{
				RGXFWIF_GPU_STATS *psStats = &psDevInfo->psRGXFWIfGpuUtilFW->sStats[ui32DriverID];

				for (eDM = 0; eDM < RGXFWIF_GPU_UTIL_DM_MAX; eDM++)
				{
					psStats->aui32DMOSLastWord[eDM] = (IMG_UINT32)(ui64LastWordTimeShifted & IMG_UINT32_MAX);
					psStats->aui32DMOSLastWordWrap[eDM] = (IMG_UINT32)(ui64LastWordTimeShifted >> 32);
				}
			}
			RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfGpuUtilFW, FLUSH);
		}

		/* init HWPERF data */
		psDevInfo->psRGXFWIfFwSysData->sHWPerfCtrl.ui32HWPerfRIdx = 0;
		psDevInfo->psRGXFWIfFwSysData->sHWPerfCtrl.ui32HWPerfWIdx = 0;
		psDevInfo->psRGXFWIfFwSysData->sHWPerfCtrl.ui32HWPerfWrapCount = 0;
		psDevInfo->psRGXFWIfFwSysData->ui32HWPerfSize = psDevInfo->ui32RGXFWIfHWPerfBufSize;
		psDevInfo->psRGXFWIfFwSysData->ui32HWPerfUt = 0;
		psDevInfo->psRGXFWIfFwSysData->ui32HWPerfDropCount = 0;
		psDevInfo->psRGXFWIfFwSysData->ui32FirstDropOrdinal = 0;
		psDevInfo->psRGXFWIfFwSysData->ui32LastDropOrdinal = 0;

		psDevInfo->psRGXFWIfFwSysData->ui32MemFaultCheck = 0;

		// flush write buffers for psRGXFWIfFwSysData
		OSWriteMemoryBarrier(psDevInfo->psRGXFWIfFwSysData);
		RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfFwSysData, FLUSH);

		/*Send through the BVNC Feature Flags*/
		eError = RGXServerFeatureFlagsToHWPerfFlags(psDevInfo, &psFwSysInitScratch->sBvncKmFeatureFlags);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXServerFeatureFlagsToHWPerfFlags", fail);

		/* populate the real FwOsInit structure with the values stored in the scratch copy */
		OSCachedMemCopyWMB(psDevInfo->psRGXFWIfSysInit, psFwSysInitScratch, sizeof(RGXFWIF_SYSINIT));
		RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfSysInit,
		                         FLUSH);
	}

	OSFreeMem(psFwSysInitScratch);


	return PVRSRV_OK;

fail:
	if (psFwSysInitScratch)
	{
		OSFreeMem(psFwSysInitScratch);
	}

	RGXFreeFwSysData(psDevInfo);

	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

/*!
*******************************************************************************
 @Function    RGXSetupFwOsData

 @Description Sets up all os-specific firmware related data

 @Input       psDevInfo

 @Return      PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXSetupFwOsData(PVRSRV_DEVICE_NODE       *psDeviceNode,
									 IMG_UINT32               ui32KCCBSizeLog2,
									 IMG_UINT32               ui32HWRDebugDumpLimit,
									 IMG_UINT32               ui32FwOsCfgFlags)
{
	PVRSRV_ERROR       eError;
	RGXFWIF_OSINIT     sFwOsInitScratch;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	OSCachedMemSet(&sFwOsInitScratch, 0, sizeof(RGXFWIF_OSINIT));

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		eError = RGXSetupFwGuardPage(psDevInfo);
		PVR_LOG_GOTO_IF_ERROR(eError, "Setting up firmware heap guard pages", fail);
	}

	/* Memory tracking the connection state should be non-volatile and
	 * is not cleared on allocation to prevent loss of pre-reset information */
	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_CONFIG_ALLOCFLAGS &
								  ~PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC,
								  sizeof(RGXFWIF_CONNECTION_CTL),
								  "FwConnectionCtl",
								  &psDevInfo->psRGXFWIfConnectionCtlMemDesc,
								  NULL,
								  (void**) &psDevInfo->psRGXFWIfConnectionCtl,
								  RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "Firmware Connection Control structure allocation", fail);

	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_CONFIG_ALLOCFLAGS |
								  PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED),
								  sizeof(RGXFWIF_OSINIT),
								  "FwOsInitStructure",
								  &psDevInfo->psRGXFWIfOsInitMemDesc,
								  NULL,
								  (void**) &psDevInfo->psRGXFWIfOsInit,
								  RFW_FWADDR_FLAG_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "Firmware Os Init structure allocation", fail);

	/* init HWR frame info */
	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS,
								  sizeof(RGXFWIF_HWRINFOBUF),
								  "FwHWRInfoBuffer",
								  &psDevInfo->psRGXFWIfHWRInfoBufCtlMemDesc,
								  &sFwOsInitScratch.sRGXFWIfHWRInfoBufCtl,
								  (void**) &psDevInfo->psRGXFWIfHWRInfoBufCtl,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "HWR Info Buffer allocation", fail);

	/* Might be uncached. Be conservative and use a DeviceMemSet */
	OSDeviceMemSet(psDevInfo->psRGXFWIfHWRInfoBufCtl, 0, sizeof(RGXFWIF_HWRINFOBUF));
	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfHWRInfoBufCtl, FLUSH);

	/* Allocate a sync for power management */
	eError = SyncPrimContextCreate(psDevInfo->psDeviceNode,
	                               &psDevInfo->hSyncPrimContext);
	PVR_LOG_GOTO_IF_ERROR(eError, "Sync primitive context allocation", fail);

	eError = SyncPrimAlloc(psDevInfo->hSyncPrimContext, &psDevInfo->psPowSyncPrim, "fw power ack");
	PVR_LOG_GOTO_IF_ERROR(eError, "Sync primitive allocation", fail);

	/* Set up kernel CCB */
	eError = RGXSetupCCB(psDevInfo,
						 &psDevInfo->psKernelCCBCtl,
						 &psDevInfo->psKernelCCBCtlLocal,
						 &psDevInfo->psKernelCCBCtlMemDesc,
						 &psDevInfo->psKernelCCB,
						 &psDevInfo->psKernelCCBMemDesc,
						 &sFwOsInitScratch.psKernelCCBCtl,
						 &sFwOsInitScratch.psKernelCCB,
						 ui32KCCBSizeLog2,
						 sizeof(RGXFWIF_KCCB_CMD),
						 (RGX_FWSHAREDMEM_GPU_RO_ALLOCFLAGS |
						 PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(FIRMWARE_CACHED)),
						 "FwKernelCCB");
	PVR_LOG_GOTO_IF_ERROR(eError, "Kernel CCB allocation", fail);

	/* KCCB additionally uses a return slot array for FW to be able to send back
	 * return codes for each required command
	 */
	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS,
								  (1U << ui32KCCBSizeLog2) * sizeof(IMG_UINT32),
								  "FwKernelCCBRtnSlots",
								  &psDevInfo->psKernelCCBRtnSlotsMemDesc,
								  &sFwOsInitScratch.psKernelCCBRtnSlots,
								  (void**) &psDevInfo->pui32KernelCCBRtnSlots,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "Kernel CCB return slot array allocation", fail);

	/* Set up firmware CCB */
	eError = RGXSetupCCB(psDevInfo,
						 &psDevInfo->psFirmwareCCBCtl,
						 &psDevInfo->psFirmwareCCBCtlLocal,
						 &psDevInfo->psFirmwareCCBCtlMemDesc,
						 &psDevInfo->psFirmwareCCB,
						 &psDevInfo->psFirmwareCCBMemDesc,
						 &sFwOsInitScratch.psFirmwareCCBCtl,
						 &sFwOsInitScratch.psFirmwareCCB,
						 RGXFWIF_FWCCB_NUMCMDS_LOG2,
						 sizeof(RGXFWIF_FWCCB_CMD),
						 RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS,
						 "FwCCB");
	PVR_LOG_GOTO_IF_ERROR(eError, "Firmware CCB allocation", fail);

	eError = RGXSetupFwAllocation(psDevInfo,
								  RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS,
								  sizeof(RGXFWIF_OSDATA),
								  "FwOsData",
								  &psDevInfo->psRGXFWIfFwOsDataMemDesc,
								  &sFwOsInitScratch.sFwOsData,
								  (void**) &psDevInfo->psRGXFWIfFwOsData,
								  RFW_FWADDR_NOREF_FLAG);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation", fail);

	psDevInfo->psRGXFWIfFwOsData->ui32FwOsConfigFlags = ui32FwOsCfgFlags & RGXFWIF_INICFG_OS_ALL;

	eError = SyncPrimGetFirmwareAddr(psDevInfo->psPowSyncPrim, &psDevInfo->psRGXFWIfFwOsData->sPowerSync.ui32Addr);
	PVR_LOG_GOTO_IF_ERROR(eError, "Get Sync Prim FW address", fail);

	/* flush write buffers for psRGXFWIfFwOsData */
	OSWriteMemoryBarrier(psDevInfo->psRGXFWIfFwOsData);
	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfFwOsData,
	                         FLUSH);

	sFwOsInitScratch.ui32HWRDebugDumpLimit = ui32HWRDebugDumpLimit;

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		/* Set up Workload Estimation firmware CCB */
		eError = RGXSetupCCB(psDevInfo,
							 &psDevInfo->psWorkEstFirmwareCCBCtl,
							 &psDevInfo->psWorkEstFirmwareCCBCtlLocal,
							 &psDevInfo->psWorkEstFirmwareCCBCtlMemDesc,
							 &psDevInfo->psWorkEstFirmwareCCB,
							 &psDevInfo->psWorkEstFirmwareCCBMemDesc,
							 &sFwOsInitScratch.psWorkEstFirmwareCCBCtl,
							 &sFwOsInitScratch.psWorkEstFirmwareCCB,
							 RGXFWIF_WORKEST_FWCCB_NUMCMDS_LOG2,
							 sizeof(RGXFWIF_WORKEST_FWCCB_CMD),
							 RGX_FWSHAREDMEM_CPU_RO_ALLOCFLAGS,
							 "FwWEstCCB");
		PVR_LOG_GOTO_IF_ERROR(eError, "Workload Estimation Firmware CCB allocation", fail);
	}
#endif /* defined(SUPPORT_WORKLOAD_ESTIMATION) */

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		/* Initialise the compatibility check data */
		RGXFWIF_COMPCHECKS_BVNC_INIT(sFwOsInitScratch.sRGXCompChecks.sFWBVNC);
		RGXFWIF_COMPCHECKS_BVNC_INIT(sFwOsInitScratch.sRGXCompChecks.sHWBVNC);
	}

	/* populate the real FwOsInit structure with the values stored in the scratch copy */
	OSCachedMemCopyWMB(psDevInfo->psRGXFWIfOsInit, &sFwOsInitScratch, sizeof(RGXFWIF_OSINIT));
	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfOsInit,
	                         FLUSH);

	return PVRSRV_OK;

fail:
	RGXFreeFwOsData(psDevInfo);

	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

/*!
*******************************************************************************
 @Function    RGXSetupFirmware

 @Description Sets up all firmware related data

 @Input       psDevInfo

 @Return      PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR RGXSetupFirmware(PVRSRV_DEVICE_NODE       *psDeviceNode,
							  RGX_INIT_APPHINTS        *psApphints,
							  IMG_UINT32               ui32ConfigFlags,
							  IMG_UINT32               ui32ConfigFlagsExt,
							  IMG_UINT32               ui32FwOsCfgFlags)
{
	PVRSRV_ERROR eError;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	IMG_UINT32 ui32HWPerfCountersDataSize;

	eError = RGXSetupFwOsData(psDeviceNode,
							  psApphints->ui32KCCBSizeLog2,
							  psApphints->ui32HWRDebugDumpLimit,
							  ui32FwOsCfgFlags);
	PVR_LOG_GOTO_IF_ERROR(eError, "Setting up firmware os data", fail);

	ui32HWPerfCountersDataSize = sizeof(RGXFWIF_HWPERF_CTL);

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		/* Guest drivers do not configure system-wide firmware data */
		psDevInfo->psRGXFWIfSysInit = NULL;
	}
	else
	{
		/* Native and Host drivers must initialise the firmware's system data */
		eError = RGXSetupFwSysData(psDeviceNode,
								   psApphints,
								   ui32ConfigFlags,
								   ui32ConfigFlagsExt,
								   ui32HWPerfCountersDataSize);
		PVR_LOG_GOTO_IF_ERROR(eError, "Setting up firmware system data", fail);
	}

	psDevInfo->bFirmwareInitialised = IMG_TRUE;

#if defined(PDUMP)
	RGXPDumpLoadFWInitData(psDevInfo,
	                       psApphints,
	                       ui32HWPerfCountersDataSize);
#endif /* PDUMP */

fail:
	return eError;
}

/*!
*******************************************************************************
 @Function    RGXFreeFwSysData

 @Description Frees all system-wide firmware related data

 @Input       psDevInfo
******************************************************************************/
static void RGXFreeFwSysData(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	psDevInfo->bFirmwareInitialised = IMG_FALSE;

	if (psDevInfo->psRGXFWAlignChecksMemDesc)
	{
		RGXFWFreeAlignChecks(psDevInfo);
	}

#if defined(PDUMP)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, TDM_PDS_CHECKSUM) &&
	    psDevInfo->psRGXFWSigTDMChecksMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWSigTDMChecksMemDesc);
		psDevInfo->psRGXFWSigTDMChecksMemDesc = NULL;
	}

	if (psDevInfo->psRGXFWSigTAChecksMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWSigTAChecksMemDesc);
		psDevInfo->psRGXFWSigTAChecksMemDesc = NULL;
	}

	if (psDevInfo->psRGXFWSig3DChecksMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWSig3DChecksMemDesc);
		psDevInfo->psRGXFWSig3DChecksMemDesc = NULL;
	}

#if defined(RGX_FEATURE_RAY_TRACING_ARCH_MAX_VALUE_IDX)
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, RAY_TRACING_ARCH) &&
		RGX_GET_FEATURE_VALUE(psDevInfo, RAY_TRACING_ARCH) > 1 &&
	   psDevInfo->psRGXFWSigRDMChecksMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWSigRDMChecksMemDesc);
		psDevInfo->psRGXFWSigRDMChecksMemDesc = NULL;
	}
#endif
#endif

#if defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
	if (psDevInfo->psCounterBufferMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psCounterBufferMemDesc);
		psDevInfo->psCounterBufferMemDesc = NULL;
	}
#endif

#if defined(SUPPORT_FIRMWARE_GCOV)
	if (psDevInfo->psFirmwareGcovBufferMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psFirmwareGcovBufferMemDesc);
		psDevInfo->psFirmwareGcovBufferMemDesc = NULL;
	}
#endif

	RGXSetupFaultReadRegisterRollback(psDevInfo);

	if (psDevInfo->psRGXFWIfGpuUtilFWCtlMemDesc)
	{
		if (psDevInfo->psRGXFWIfGpuUtilFW != NULL)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfGpuUtilFWCtlMemDesc);
			psDevInfo->psRGXFWIfGpuUtilFW = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfGpuUtilFWCtlMemDesc);
		psDevInfo->psRGXFWIfGpuUtilFWCtlMemDesc = NULL;
	}

	if (psDevInfo->psRGXFWIfRuntimeCfgMemDesc)
	{
		if (psDevInfo->psRGXFWIfRuntimeCfg != NULL)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfRuntimeCfgMemDesc);
			psDevInfo->psRGXFWIfRuntimeCfg = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfRuntimeCfgMemDesc);
		psDevInfo->psRGXFWIfRuntimeCfgMemDesc = NULL;
	}

	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META_COREMEM_SIZE))
	{
		if (psDevInfo->psRGXFWIfCorememDataStoreMemDesc)
		{
			psDevInfo->psRGXFWIfCorememDataStoreMemDesc = NULL;
		}
	}

	if (psDevInfo->psRGXFWIfTraceBufCtlMemDesc)
	{
		if (psDevInfo->psRGXFWIfTraceBufCtl != NULL)
		{
			/* deinit/free the tracebuffer allocation */
			RGXTraceBufferDeinit(psDevInfo);

			DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfTraceBufCtlMemDesc);
			psDevInfo->psRGXFWIfTraceBufCtl = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfTraceBufCtlMemDesc);
		psDevInfo->psRGXFWIfTraceBufCtlMemDesc = NULL;
	}

	if (psDevInfo->psRGXFWIfFwSysDataMemDesc)
	{
		if (psDevInfo->psRGXFWIfFwSysData != NULL)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfFwSysDataMemDesc);
			psDevInfo->psRGXFWIfFwSysData = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfFwSysDataMemDesc);
		psDevInfo->psRGXFWIfFwSysDataMemDesc = NULL;
	}

#if defined(SUPPORT_TBI_INTERFACE)
	if (psDevInfo->psRGXFWIfTBIBufferMemDesc)
	{
		RGXTBIBufferDeinit(psDevInfo);
	}
#endif

#if defined(SUPPORT_USER_REGISTER_CONFIGURATION)
	if (psDevInfo->psRGXFWIfRegCfgMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfRegCfgMemDesc);
		psDevInfo->psRGXFWIfRegCfgMemDesc = NULL;
	}
#endif
	if (psDevInfo->psRGXFWIfHWPerfCountersMemDesc)
	{
		RGXUnsetFirmwareAddress(psDevInfo->psRGXFWIfHWPerfCountersMemDesc);
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfHWPerfCountersMemDesc);
		psDevInfo->psRGXFWIfHWPerfCountersMemDesc = NULL;
	}

#if defined(SUPPORT_SECURITY_VALIDATION)
	if (psDevInfo->psRGXFWIfNonSecureBufMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfNonSecureBufMemDesc);
		psDevInfo->psRGXFWIfNonSecureBufMemDesc = NULL;
	}

	if (psDevInfo->psRGXFWIfSecureBufMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfSecureBufMemDesc);
		psDevInfo->psRGXFWIfSecureBufMemDesc = NULL;
	}
#endif

#if defined(SUPPORT_PDVFS)
	if (psDevInfo->psRGXFWIFCoreClkRateMemDesc)
	{
		if (psDevInfo->pui32RGXFWIFCoreClkRate != NULL)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIFCoreClkRateMemDesc);
			psDevInfo->pui32RGXFWIFCoreClkRate = NULL;
		}

		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIFCoreClkRateMemDesc);
		psDevInfo->psRGXFWIFCoreClkRateMemDesc = NULL;
	}
#endif

#if defined(SUPPORT_FW_HOST_SIDE_RECOVERY)
	if (psDevInfo->psRGXFWIfActiveContextBufDesc)
	{
		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfActiveContextBufDesc);
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfActiveContextBufDesc);
		psDevInfo->psRGXFWIfActiveContextBufDesc = NULL;
	}
#endif
}

/*!
*******************************************************************************
 @Function    RGXFreeFwOsData

 @Description Frees all os-specific firmware related data

 @Input       psDevInfo
******************************************************************************/
static void RGXFreeFwOsData(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	RGXFreeCCBReturnSlots(psDevInfo,
	                      &psDevInfo->pui32KernelCCBRtnSlots,
	                      &psDevInfo->psKernelCCBRtnSlotsMemDesc);
	RGXFreeCCB(psDevInfo,
	           &psDevInfo->psKernelCCBCtl,
	           &psDevInfo->psKernelCCBCtlLocal,
	           &psDevInfo->psKernelCCBCtlMemDesc,
	           &psDevInfo->psKernelCCB,
	           &psDevInfo->psKernelCCBMemDesc);

	RGXFreeCCB(psDevInfo,
	           &psDevInfo->psFirmwareCCBCtl,
	           &psDevInfo->psFirmwareCCBCtlLocal,
	           &psDevInfo->psFirmwareCCBCtlMemDesc,
	           &psDevInfo->psFirmwareCCB,
	           &psDevInfo->psFirmwareCCBMemDesc);

#if defined(SUPPORT_WORKLOAD_ESTIMATION)
	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		RGXFreeCCB(psDevInfo,
		           &psDevInfo->psWorkEstFirmwareCCBCtl,
		           &psDevInfo->psWorkEstFirmwareCCBCtlLocal,
		           &psDevInfo->psWorkEstFirmwareCCBCtlMemDesc,
		           &psDevInfo->psWorkEstFirmwareCCB,
		           &psDevInfo->psWorkEstFirmwareCCBMemDesc);
	}
#endif

	if (psDevInfo->psPowSyncPrim != NULL)
	{
		SyncPrimFree(psDevInfo->psPowSyncPrim);
		psDevInfo->psPowSyncPrim = NULL;
	}

	if (psDevInfo->hSyncPrimContext != (IMG_HANDLE) NULL)
	{
		SyncPrimContextDestroy(psDevInfo->hSyncPrimContext);
		psDevInfo->hSyncPrimContext = (IMG_HANDLE) NULL;
	}

	if (psDevInfo->psRGXFWIfHWRInfoBufCtlMemDesc)
	{
		if (psDevInfo->psRGXFWIfHWRInfoBufCtl != NULL)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfHWRInfoBufCtlMemDesc);
			psDevInfo->psRGXFWIfHWRInfoBufCtl = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfHWRInfoBufCtlMemDesc);
		psDevInfo->psRGXFWIfHWRInfoBufCtlMemDesc = NULL;
	}

	if (psDevInfo->psRGXFWIfFwOsDataMemDesc)
	{
		if (psDevInfo->psRGXFWIfFwOsData != NULL)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfFwOsDataMemDesc);
			psDevInfo->psRGXFWIfFwOsData = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfFwOsDataMemDesc);
		psDevInfo->psRGXFWIfFwOsDataMemDesc = NULL;
	}

	if (psDevInfo->psCompletedMemDesc)
	{
		if (psDevInfo->pui32CompletedById)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psCompletedMemDesc);
			psDevInfo->pui32CompletedById = NULL;
		}
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psCompletedMemDesc);
		psDevInfo->psCompletedMemDesc = NULL;
	}
	if (psDevInfo->psEndTimeMemDesc)
	{
		if (psDevInfo->pui64EndTimeById)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psEndTimeMemDesc);
			psDevInfo->pui64EndTimeById = NULL;
		}

		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psEndTimeMemDesc);
		psDevInfo->psEndTimeMemDesc = NULL;
	}
	if (psDevInfo->psStartTimeMemDesc)
	{
		if (psDevInfo->pui64StartTimeById)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psStartTimeMemDesc);
			psDevInfo->pui64StartTimeById = NULL;
		}

		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psStartTimeMemDesc);
		psDevInfo->psStartTimeMemDesc = NULL;
	}
#if !defined(PVRSRV_USE_BRIDGE_LOCK)
	if (psDevInfo->hTimerQueryLock)
	{
		OSLockDestroy(psDevInfo->hTimerQueryLock);
		psDevInfo->hTimerQueryLock = NULL;
	}
#endif

	if (psDevInfo->psRGXFWHeapGuardPageReserveMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWHeapGuardPageReserveMemDesc);
	}
}

/*!
*******************************************************************************
 @Function    RGXFreeFirmware

 @Description Frees all the firmware-related allocations

 @Input       psDevInfo
******************************************************************************/
void RGXFreeFirmware(PVRSRV_RGXDEV_INFO	*psDevInfo)
{
	RGXFreeFwOsData(psDevInfo);

	if (psDevInfo->psRGXFWIfConnectionCtl)
	{
		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfConnectionCtlMemDesc);
		psDevInfo->psRGXFWIfConnectionCtl = NULL;
	}

	if (psDevInfo->psRGXFWIfConnectionCtlMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfConnectionCtlMemDesc);
		psDevInfo->psRGXFWIfConnectionCtlMemDesc = NULL;
	}

	if (psDevInfo->psRGXFWIfOsInit)
	{
		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfOsInitMemDesc);
		psDevInfo->psRGXFWIfOsInit = NULL;
	}

	if (psDevInfo->psRGXFWIfOsInitMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfOsInitMemDesc);
		psDevInfo->psRGXFWIfOsInitMemDesc = NULL;
	}

	RGXFreeFwSysData(psDevInfo);
	if (psDevInfo->psRGXFWIfSysInit)
	{
		DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfSysInitMemDesc);
		psDevInfo->psRGXFWIfSysInit = NULL;
	}

	if (psDevInfo->psRGXFWIfSysInitMemDesc)
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfSysInitMemDesc);
		psDevInfo->psRGXFWIfSysInitMemDesc = NULL;
	}
}

static INLINE PVRSRV_ERROR RGXUpdateLocalKCCBRoff(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	const RGXFWIF_CCB_CTL *psKCCBCtl = psDevInfo->psKernelCCBCtl;
	RGXFWIF_CCB_CTL *psKCCBCtlLocal = psDevInfo->psKernelCCBCtlLocal;
	IMG_UINT32 ui32ReadOffset;

	barrier(); /* Don't optimise order. Reads from device memory follow. */

	/* update KCCB read offset */
	RGXFwSharedMemCacheOpValue(psKCCBCtl->ui32ReadOffset, INVALIDATE);
	ui32ReadOffset = psKCCBCtl->ui32ReadOffset;

	if (ui32ReadOffset > psKCCBCtlLocal->ui32WrapMask)
	{
		return PVRSRV_ERROR_KERNEL_CCB_OFFSET;
	}

	psKCCBCtlLocal->ui32ReadOffset = ui32ReadOffset;

	return PVRSRV_OK;
}

static INLINE PVRSRV_ERROR RGXUpdateLocalFWCCBWoff(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	const RGXFWIF_CCB_CTL *psFWCCBCtl = psDevInfo->psFirmwareCCBCtl;
	RGXFWIF_CCB_CTL *psFWCCBCtlLocal = psDevInfo->psFirmwareCCBCtlLocal;
	IMG_UINT32 ui32WriteOffset;

	barrier(); /* Don't optimise order. Reads from device memory follow. */

	/* update FWCCB write offset */
	RGXFwSharedMemCacheOpValue(psFWCCBCtl->ui32WriteOffset, INVALIDATE);
	ui32WriteOffset = psFWCCBCtl->ui32WriteOffset;

	if (ui32WriteOffset > psFWCCBCtlLocal->ui32WrapMask)
	{
		return PVRSRV_ERROR_KERNEL_CCB_OFFSET;
	}

	psFWCCBCtlLocal->ui32WriteOffset = ui32WriteOffset;

	return PVRSRV_OK;
}

/******************************************************************************
 FUNCTION	: RGXAcquireKernelCCBSlot

 PURPOSE	: Attempts to obtain a slot in the Kernel CCB

 PARAMETERS	: psCCB - the CCB
			: Address of space if available, NULL otherwise

 RETURNS	: PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXAcquireKernelCCBSlot(PVRSRV_RGXDEV_INFO *psDevInfo,
											IMG_UINT32		*pui32Offset)
{
	IMG_UINT32 ui32NextWriteOffset;
	RGXFWIF_CCB_CTL *psKCCBCtlLocal = psDevInfo->psKernelCCBCtlLocal;

	ui32NextWriteOffset = (psKCCBCtlLocal->ui32WriteOffset + 1) & psKCCBCtlLocal->ui32WrapMask;

#if defined(PDUMP)
	/* Wait for sufficient CCB space to become available */
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, 0,
	                      "Wait for space to write kCCB woff=%u", psKCCBCtlLocal->ui32WriteOffset);
	DevmemPDumpCBP(psDevInfo->psKernelCCBCtlMemDesc,
	               offsetof(RGXFWIF_CCB_CTL, ui32ReadOffset),
	               psKCCBCtlLocal->ui32WriteOffset,
	               1,
	               (psKCCBCtlLocal->ui32WrapMask + 1));
#endif

	if (ui32NextWriteOffset == psKCCBCtlLocal->ui32ReadOffset)
	{
		PVRSRV_ERROR eError = RGXUpdateLocalKCCBRoff(psDevInfo);
		PVR_LOG_RETURN_IF_ERROR(eError, "RGXUpdateLocalKCCBRoff");

		if (ui32NextWriteOffset == psKCCBCtlLocal->ui32ReadOffset)
		{
			return PVRSRV_ERROR_KERNEL_CCB_FULL;
		}
	}
	*pui32Offset = ui32NextWriteOffset;
	return PVRSRV_OK;
}

/******************************************************************************
 FUNCTION	: RGXPollKernelCCBSlot

 PURPOSE	: Poll for space in Kernel CCB

 PARAMETERS	: psCCB - the CCB
			: Address of space if available, NULL otherwise

 RETURNS	: PVRSRV_ERROR
******************************************************************************/
static PVRSRV_ERROR RGXPollKernelCCBSlot(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_UINT32	ui32NextWriteOffset;
	RGXFWIF_CCB_CTL *psKCCBCtlLocal = psDevInfo->psKernelCCBCtlLocal;

	ui32NextWriteOffset = (psKCCBCtlLocal->ui32WriteOffset + 1) & psKCCBCtlLocal->ui32WrapMask;

	if (ui32NextWriteOffset != psKCCBCtlLocal->ui32ReadOffset)
	{
		return PVRSRV_OK;
	}

	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		PVRSRV_ERROR eError = RGXUpdateLocalKCCBRoff(psDevInfo);
		PVR_LOG_RETURN_IF_ERROR(eError, "RGXUpdateLocalKCCBRoff");

		if (ui32NextWriteOffset != psKCCBCtlLocal->ui32ReadOffset)
		{
			return PVRSRV_OK;
		}

		/*
		 * The following check doesn't impact performance, since the
		 * CPU has to wait for the GPU anyway (full kernel CCB).
		 */
		if (PVRSRVGetPVRSRVData()->eServicesState != PVRSRV_SERVICES_STATE_OK)
		{
			return PVRSRV_ERROR_KERNEL_CCB_FULL;
		}

		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	return PVRSRV_ERROR_KERNEL_CCB_FULL;
}

/******************************************************************************
 FUNCTION	: RGXGetCmdMemCopySize

 PURPOSE	: Calculates actual size of KCCB command getting used

 PARAMETERS	: eCmdType     Type of KCCB command

 RETURNS	: Returns actual size of KCCB command on success else zero
******************************************************************************/
static IMG_UINT32 RGXGetCmdMemCopySize(RGXFWIF_KCCB_CMD_TYPE eCmdType)
{
	/* First get offset of uCmdData inside the struct RGXFWIF_KCCB_CMD
	 * This will account alignment requirement of uCmdData union
	 *
	 * Then add command-data size depending on command type to calculate actual
	 * command size required to do mem copy
	 *
	 * NOTE: Make sure that uCmdData is the last member of RGXFWIF_KCCB_CMD struct.
	 */
	switch (eCmdType)
	{
		case RGXFWIF_KCCB_CMD_KICK:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_KCCB_CMD_KICK_DATA);
		}
		case RGXFWIF_KCCB_CMD_COMBINED_TA_3D_KICK:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_KCCB_CMD_COMBINED_TA_3D_KICK_DATA);
		}
		case RGXFWIF_KCCB_CMD_MMUCACHE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_MMUCACHEDATA);
		}
#if defined(SUPPORT_USC_BREAKPOINT)
		case RGXFWIF_KCCB_CMD_BP:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_BPDATA);
		}
#endif
		case RGXFWIF_KCCB_CMD_SLCFLUSHINVAL:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_SLCFLUSHINVALDATA);
		}
		case RGXFWIF_KCCB_CMD_CLEANUP:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_CLEANUP_REQUEST);
		}
		case RGXFWIF_KCCB_CMD_POW:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_POWER_REQUEST);
		}
		case RGXFWIF_KCCB_CMD_ZSBUFFER_BACKING_UPDATE:
		case RGXFWIF_KCCB_CMD_ZSBUFFER_UNBACKING_UPDATE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_ZSBUFFER_BACKING_DATA);
		}
		case RGXFWIF_KCCB_CMD_FREELIST_GROW_UPDATE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_FREELIST_GS_DATA);
		}
		case RGXFWIF_KCCB_CMD_FREELISTS_RECONSTRUCTION_UPDATE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_FREELISTS_RECONSTRUCTION_DATA);
		}
		case RGXFWIF_KCCB_CMD_NOTIFY_WRITE_OFFSET_UPDATE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_WRITE_OFFSET_UPDATE_DATA);
		}
		case RGXFWIF_KCCB_CMD_DISABLE_ZSSTORE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_KCCB_CMD_DISABLE_ZSSTORE_DATA);
		}
		case RGXFWIF_KCCB_CMD_FORCE_UPDATE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_KCCB_CMD_FORCE_UPDATE_DATA);
		}
#if defined(SUPPORT_USER_REGISTER_CONFIGURATION)
		case RGXFWIF_KCCB_CMD_REGCONFIG:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_REGCONFIG_DATA);
		}
#endif
		case RGXFWIF_KCCB_CMD_HWPERF_SELECT_CUSTOM_CNTRS:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_HWPERF_SELECT_CUSTOM_CNTRS);
		}
#if defined(SUPPORT_PDVFS)
		case RGXFWIF_KCCB_CMD_PDVFS_LIMIT_MAX_FREQ:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_PDVFS_MAX_FREQ_DATA);
		}
#endif
		case RGXFWIF_KCCB_CMD_OS_ONLINE_STATE_CONFIGURE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_OS_STATE_CHANGE_DATA);
		}
		case RGXFWIF_KCCB_CMD_COUNTER_DUMP:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_COUNTER_DUMP_DATA);
		}
		case RGXFWIF_KCCB_CMD_HWPERF_UPDATE_CONFIG:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_HWPERF_CTRL);
		}
		case RGXFWIF_KCCB_CMD_HWPERF_CONFIG_ENABLE_BLKS:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_HWPERF_CONFIG_ENABLE_BLKS);
		}
		case RGXFWIF_KCCB_CMD_HWPERF_CONFIG_BLKS:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_HWPERF_CONFIG_DA_BLKS);
		}
		case RGXFWIF_KCCB_CMD_HWPERF_CTRL_BLKS:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_HWPERF_CTRL_BLKS);
		}
		case RGXFWIF_KCCB_CMD_CORECLKSPEEDCHANGE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_CORECLKSPEEDCHANGE_DATA);
		}
		case RGXFWIF_KCCB_CMD_VZ_DRV_ARRAY_CHANGE:
		case RGXFWIF_KCCB_CMD_VZ_DRV_TIME_SLICE:
		case RGXFWIF_KCCB_CMD_VZ_DRV_TIME_SLICE_INTERVAL:
		case RGXFWIF_KCCB_CMD_WDG_CFG:
		case RGXFWIF_KCCB_CMD_PHR_CFG:
		case RGXFWIF_KCCB_CMD_HEALTH_CHECK:
		case RGXFWIF_KCCB_CMD_STATEFLAGS_CTRL:
		{
			/* No command specific data */
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData);
		}
		case RGXFWIF_KCCB_CMD_LOGTYPE_UPDATE:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_DEV_VIRTADDR);
		}
		case RGXFWIF_KCCB_CMD_CANCEL_WORK:
		{
			return offsetof(RGXFWIF_KCCB_CMD, uCmdData) + sizeof(RGXFWIF_CANCEL_WORK_DATA);
		}
		default:
		{
			/* Invalid (OR) Unused (OR) Newly added command type */
			return 0; /* Error */
		}
	}
}

PVRSRV_ERROR RGXWaitForKCCBSlotUpdate(PVRSRV_RGXDEV_INFO *psDevInfo,
                                      IMG_UINT32 ui32SlotNum,
									  IMG_UINT32 ui32PDumpFlags)
{
	PVRSRV_ERROR eError;

	RGXFwSharedMemCacheOpValue(psDevInfo->pui32KernelCCBRtnSlots[ui32SlotNum],
	                           INVALIDATE);
	eError = PVRSRVWaitForValueKM(
	              (IMG_UINT32 __iomem *)&psDevInfo->pui32KernelCCBRtnSlots[ui32SlotNum],
				  RGXFWIF_KCCB_RTN_SLOT_CMD_EXECUTED,
				  RGXFWIF_KCCB_RTN_SLOT_CMD_EXECUTED,
				  RGXFwSharedMemCacheOpExecPfn);
	PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVWaitForValueKM");

#if defined(PDUMP)
	/* PDumping conditions same as RGXSendCommandRaw for the actual command and poll command to go in harmony */
	if (PDumpCheckFlagsWrite(psDevInfo->psDeviceNode, ui32PDumpFlags))
	{
		PDUMPCOMMENT(psDevInfo->psDeviceNode, "Poll on KCCB slot %u for value %u (mask: 0x%x)", ui32SlotNum,
					 RGXFWIF_KCCB_RTN_SLOT_CMD_EXECUTED, RGXFWIF_KCCB_RTN_SLOT_CMD_EXECUTED);

		eError = DevmemPDumpDevmemPol32(psDevInfo->psKernelCCBRtnSlotsMemDesc,
										ui32SlotNum * sizeof(IMG_UINT32),
										RGXFWIF_KCCB_RTN_SLOT_CMD_EXECUTED,
										RGXFWIF_KCCB_RTN_SLOT_CMD_EXECUTED,
										PDUMP_POLL_OPERATOR_EQUAL,
										ui32PDumpFlags);
		PVR_LOG_IF_ERROR(eError, "DevmemPDumpDevmemPol32");
	}
#else
	PVR_UNREFERENCED_PARAMETER(ui32PDumpFlags);
#endif

	return eError;
}

static PVRSRV_ERROR RGXSendCommandRaw(PVRSRV_RGXDEV_INFO  *psDevInfo,
									  RGXFWIF_KCCB_CMD    *psKCCBCmd,
									  IMG_UINT32          uiPDumpFlags,
									  IMG_UINT32          *pui32CmdKCCBSlot)
{
	PVRSRV_ERROR		eError;
	PVRSRV_DEVICE_NODE	*psDeviceNode = psDevInfo->psDeviceNode;
	RGXFWIF_CCB_CTL		*psKCCBCtl;
	RGXFWIF_CCB_CTL		*psKCCBCtlLocal = psDevInfo->psKernelCCBCtlLocal;
	IMG_UINT8			*pui8KCCB = psDevInfo->psKernelCCB;
	IMG_UINT32			ui32NewWriteOffset;
	IMG_UINT32			ui32OldWriteOffset;
	IMG_UINT32			ui32CmdMemCopySize;

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(uiPDumpFlags);
	RGXFwSharedMemCacheOpPtr(psDevInfo->psKernelCCBCtl, INVALIDATE);
	psKCCBCtl = psDevInfo->psKernelCCBCtl;
	ui32OldWriteOffset = psKCCBCtlLocal->ui32WriteOffset;

#else
	IMG_BOOL bContCaptureOn = PDumpCheckFlagsWrite(psDeviceNode, PDUMP_FLAGS_CONTINUOUS | PDUMP_FLAGS_POWER); /* client connected or in pdump init phase */
	IMG_BOOL bPDumpEnabled = PDumpCheckFlagsWrite(psDeviceNode, uiPDumpFlags); /* Are we in capture range or continuous and not in a power transition */

	psKCCBCtl = psDevInfo->psKernelCCBCtl;
	ui32OldWriteOffset = psKCCBCtlLocal->ui32WriteOffset;

	if (bContCaptureOn)
	{
		/* in capture range */
		if (bPDumpEnabled)
		{
			if (!psDevInfo->bDumpedKCCBCtlAlready)
			{
				/* entering capture range */
				psDevInfo->bDumpedKCCBCtlAlready = IMG_TRUE;

				/* Wait for the live FW to catch up */
				PVR_DPF((PVR_DBG_MESSAGE, "%s: waiting on fw to catch-up, roff: %d, woff: %d",
						__func__,
						psKCCBCtl->ui32ReadOffset, ui32OldWriteOffset));
				PVRSRVPollForValueKM(psDeviceNode,
				                     (IMG_UINT32 __iomem *)&psKCCBCtl->ui32ReadOffset,
				                     ui32OldWriteOffset, 0xFFFFFFFF,
				                     POLL_FLAG_LOG_ERROR | POLL_FLAG_DEBUG_DUMP,
				                     NULL);

				/* Dump Init state of Kernel CCB control (read and write offset) */
				PDUMPCOMMENTWITHFLAGS(psDeviceNode, uiPDumpFlags,
						"Initial state of kernel CCB Control, roff: %d, woff: %d",
						psKCCBCtl->ui32ReadOffset, psKCCBCtlLocal->ui32WriteOffset);

				DevmemPDumpLoadMem(psDevInfo->psKernelCCBCtlMemDesc,
						0,
						sizeof(RGXFWIF_CCB_CTL),
						uiPDumpFlags);
			}
		}
	}
#endif

#if defined(SUPPORT_AUTOVZ)
	KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
	KM_CONNECTION_CACHEOP(Os, INVALIDATE);
	if (!PVRSRV_VZ_MODE_IS(NATIVE, DEVNODE, psDeviceNode))
	{
		if ((likely(KM_FW_CONNECTION_IS(ACTIVE, psDevInfo) &&
			(KM_OS_CONNECTION_IS(ACTIVE, psDevInfo) || KM_OS_CONNECTION_IS(READY, psDevInfo)))) ||
			(KM_FW_CONNECTION_IS(READY, psDevInfo) && KM_OS_CONNECTION_IS(READY, psDevInfo)))
		{
			RGXUpdateAutoVzWdgToken(psDevInfo);
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: The firmware-driver connection is invalid:"
									"driver state = %u / firmware state = %u;"
									"expected READY (%u/%u) or ACTIVE (%u/%u) or in transition (%u/%u);",
									__func__, KM_GET_OS_CONNECTION(psDevInfo), KM_GET_FW_CONNECTION(psDevInfo),
									RGXFW_CONNECTION_OS_READY, RGXFW_CONNECTION_FW_READY,
									RGXFW_CONNECTION_OS_ACTIVE, RGXFW_CONNECTION_FW_ACTIVE,
									RGXFW_CONNECTION_OS_READY, RGXFW_CONNECTION_FW_ACTIVE));
			eError = PVRSRV_ERROR_PVZ_OSID_IS_OFFLINE;
			goto _RGXSendCommandRaw_Exit;
		}
	}
#endif

	if (!OSLockIsLocked(psDeviceNode->hPowerLock))
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s called without power lock held!",
				__func__));
		PVR_ASSERT(OSLockIsLocked(psDeviceNode->hPowerLock));
	}

	/* Acquire a slot in the CCB */
	eError = RGXAcquireKernelCCBSlot(psDevInfo, &ui32NewWriteOffset);
	if (eError != PVRSRV_OK)
	{
		goto _RGXSendCommandRaw_Exit;
	}

	/* Calculate actual size of command to optimize device mem copy */
	ui32CmdMemCopySize = RGXGetCmdMemCopySize(psKCCBCmd->eCmdType);
	PVR_LOG_RETURN_IF_FALSE(ui32CmdMemCopySize !=0, "RGXGetCmdMemCopySize failed", PVRSRV_ERROR_INVALID_CCB_COMMAND);

	/* Copy the command into the CCB */
	OSCachedMemCopy(&pui8KCCB[ui32OldWriteOffset * sizeof(RGXFWIF_KCCB_CMD)],
	                psKCCBCmd, ui32CmdMemCopySize);
	RGXFwSharedMemCacheOpExec(&pui8KCCB[ui32OldWriteOffset * sizeof(RGXFWIF_KCCB_CMD)], ui32CmdMemCopySize, PVRSRV_CACHE_OP_FLUSH);

	/* If non-NULL pui32CmdKCCBSlot passed-in, return the kCCB slot in which the command was enqueued */
	if (pui32CmdKCCBSlot)
	{
		*pui32CmdKCCBSlot = ui32OldWriteOffset;

		/* Each such command enqueue needs to reset the slot value first. This is so that a caller
		 * doesn't get to see stale/false value in allotted slot */
		OSWriteDeviceMem32WithWMB(&psDevInfo->pui32KernelCCBRtnSlots[ui32OldWriteOffset],
		                          RGXFWIF_KCCB_RTN_SLOT_NO_RESPONSE);
		RGXFwSharedMemCacheOpValue(psDevInfo->pui32KernelCCBRtnSlots[ui32OldWriteOffset],
		                           FLUSH);
#if defined(PDUMP)
		PDUMPCOMMENTWITHFLAGS(psDeviceNode, uiPDumpFlags,
							  "Reset kCCB slot number %u", ui32OldWriteOffset);
		DevmemPDumpLoadMem(psDevInfo->psKernelCCBRtnSlotsMemDesc,
		                   ui32OldWriteOffset * sizeof(IMG_UINT32),
						   sizeof(IMG_UINT32),
						   uiPDumpFlags);
#endif
		PVR_DPF((PVR_DBG_MESSAGE, "%s: Device (%p) KCCB slot %u reset with value %u for command type %x",
		         __func__, psDevInfo, ui32OldWriteOffset, RGXFWIF_KCCB_RTN_SLOT_NO_RESPONSE, psKCCBCmd->eCmdType));
	}

	/* Memory barrier before KCCB write offset update. */
	OSWriteMemoryBarrier(NULL);


	/* Move past the current command */
	psKCCBCtlLocal->ui32WriteOffset = ui32NewWriteOffset;
	psKCCBCtl->ui32WriteOffset = ui32NewWriteOffset;
	/* Read-back of memory before Kick MTS */
	OSWriteMemoryBarrier(&psKCCBCtl->ui32WriteOffset);
	RGXFwSharedMemCacheOpValue(psKCCBCtl->ui32WriteOffset, FLUSH);

#if defined(PDUMP)
	if (bContCaptureOn)
	{
		/* in capture range */
		if (bPDumpEnabled)
		{
			/* Dump new Kernel CCB content */
			PDUMPCOMMENTWITHFLAGS(psDeviceNode,
					uiPDumpFlags, "Dump kCCB cmd woff = %d",
					ui32OldWriteOffset);
			DevmemPDumpLoadMem(psDevInfo->psKernelCCBMemDesc,
					ui32OldWriteOffset * sizeof(RGXFWIF_KCCB_CMD),
					ui32CmdMemCopySize,
					uiPDumpFlags);

			/* Dump new kernel CCB write offset */
			PDUMPCOMMENTWITHFLAGS(psDeviceNode,
					uiPDumpFlags, "Dump kCCBCtl woff: %d",
					ui32NewWriteOffset);
			DevmemPDumpLoadMem(psDevInfo->psKernelCCBCtlMemDesc,
					offsetof(RGXFWIF_CCB_CTL, ui32WriteOffset),
					sizeof(IMG_UINT32),
					uiPDumpFlags);

			/* mimic the read-back of the write from above */
			DevmemPDumpDevmemPol32(psDevInfo->psKernelCCBCtlMemDesc,
					offsetof(RGXFWIF_CCB_CTL, ui32WriteOffset),
					ui32NewWriteOffset,
					0xFFFFFFFF,
					PDUMP_POLL_OPERATOR_EQUAL,
					uiPDumpFlags);
		}
		/* out of capture range */
		else
		{
			eError = RGXPdumpDrainKCCB(psDevInfo, ui32OldWriteOffset);
			PVR_LOG_GOTO_IF_ERROR(eError, "RGXPdumpDrainKCCB", _RGXSendCommandRaw_Exit);
		}
	}
#endif


	PDUMPCOMMENTWITHFLAGS(psDeviceNode, uiPDumpFlags, "MTS kick for kernel CCB");
	/*
	 * Kick the MTS to schedule the firmware.
	 */
	__MTSScheduleWrite(psDevInfo, RGXFWIF_DM_GP & ~RGX_CR_MTS_SCHEDULE_DM_CLRMSK);

	PDUMPREG32(psDeviceNode, RGX_PDUMPREG_NAME, RGX_CR_MTS_SCHEDULE,
	           RGXFWIF_DM_GP & ~RGX_CR_MTS_SCHEDULE_DM_CLRMSK, uiPDumpFlags);

#if defined(SUPPORT_AUTOVZ)
	RGXUpdateAutoVzWdgToken(psDevInfo);
#endif

#if defined(NO_HARDWARE)
	/* keep the roff updated because fw isn't there to update it */
	psKCCBCtl->ui32ReadOffset = psKCCBCtlLocal->ui32WriteOffset;
#endif

_RGXSendCommandRaw_Exit:
	return eError;
}

/******************************************************************************
 FUNCTION	: _AllocDeferredCommand

 PURPOSE	: Allocate a KCCB command and add it to KCCB deferred list

 PARAMETERS	: psDevInfo	RGX device info
			: eKCCBType		Firmware Command type
			: psKCCBCmd		Firmware Command
			: uiPDumpFlags	Pdump flags

 RETURNS	: PVRSRV_OK	If all went good, PVRSRV_ERROR_RETRY otherwise.
******************************************************************************/
static PVRSRV_ERROR _AllocDeferredCommand(PVRSRV_RGXDEV_INFO *psDevInfo,
                                          RGXFWIF_KCCB_CMD   *psKCCBCmd,
                                          IMG_UINT32         uiPDumpFlags)
{
	RGX_DEFERRED_KCCB_CMD *psDeferredCommand;
	OS_SPINLOCK_FLAGS uiFlags;

	psDeferredCommand = OSAllocMem(sizeof(*psDeferredCommand));

	if (!psDeferredCommand)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "Deferring a KCCB command failed: allocation failure: requesting retry"));
		return PVRSRV_ERROR_RETRY;
	}

	psDeferredCommand->sKCCBcmd = *psKCCBCmd;
	psDeferredCommand->uiPDumpFlags = uiPDumpFlags;
	psDeferredCommand->psDevInfo = psDevInfo;

	OSSpinLockAcquire(psDevInfo->hLockKCCBDeferredCommandsList, uiFlags);
	dllist_add_to_tail(&(psDevInfo->sKCCBDeferredCommandsListHead), &(psDeferredCommand->sListNode));
	psDevInfo->ui32KCCBDeferredCommandsCount++;
	OSSpinLockRelease(psDevInfo->hLockKCCBDeferredCommandsList, uiFlags);

	return PVRSRV_OK;
}

/******************************************************************************
 FUNCTION	: _FreeDeferredCommand

 PURPOSE	: Remove from the deferred list the sent deferred KCCB command

 PARAMETERS	: psNode			Node in deferred list
			: psDeferredKCCBCmd	KCCB Command to free

 RETURNS	: None
******************************************************************************/
static void _FreeDeferredCommand(DLLIST_NODE *psNode, RGX_DEFERRED_KCCB_CMD *psDeferredKCCBCmd)
{
	dllist_remove_node(psNode);
	psDeferredKCCBCmd->psDevInfo->ui32KCCBDeferredCommandsCount--;
	OSFreeMem(psDeferredKCCBCmd);
}

/******************************************************************************
 FUNCTION	: RGXSendCommandsFromDeferredList

 PURPOSE	: Try send KCCB commands in deferred list to KCCB
		  Should be called by holding PowerLock

 PARAMETERS	: psDevInfo	RGX device info
			: bPoll		Poll for space in KCCB

 RETURNS	: PVRSRV_OK	If all commands in deferred list are sent to KCCB,
			  PVRSRV_ERROR_KERNEL_CCB_FULL otherwise.
******************************************************************************/
PVRSRV_ERROR RGXSendCommandsFromDeferredList(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_BOOL bPoll)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	DLLIST_NODE *psNode, *psNext;
	RGX_DEFERRED_KCCB_CMD *psTempDeferredKCCBCmd;
	DLLIST_NODE sCommandList;
	OS_SPINLOCK_FLAGS uiFlags;

	PVR_ASSERT(PVRSRVPwrLockIsLockedByMe(psDevInfo->psDeviceNode));

	/* !!! Important !!!
	 *
	 * The idea of moving the whole list hLockKCCBDeferredCommandsList below
	 * to the temporary list is only valid under the principle that all of the
	 * operations are also protected by the power lock. It must be held
	 * so that the order of the commands doesn't get messed up while we're
	 * performing the operations on the local list.
	 *
	 * The necessity of releasing the hLockKCCBDeferredCommandsList comes from
	 * the fact that _FreeDeferredCommand() is allocating memory and it can't
	 * be done in atomic context (inside section protected by a spin lock).
	 *
	 * We're using spin lock here instead of mutex to quickly perform a check
	 * if the list is empty in MISR without a risk that the MISR is going
	 * to sleep due to a lock.
	 */

	/* move the whole list to a local list so it can be processed without lock */
	OSSpinLockAcquire(psDevInfo->hLockKCCBDeferredCommandsList, uiFlags);
	dllist_replace_head(&psDevInfo->sKCCBDeferredCommandsListHead, &sCommandList);
	OSSpinLockRelease(psDevInfo->hLockKCCBDeferredCommandsList, uiFlags);

	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		if (dllist_is_empty(&sCommandList))
		{
			return PVRSRV_OK;
		}

		/* For every deferred KCCB command, try to send it*/
		dllist_foreach_node(&sCommandList, psNode, psNext)
		{
			psTempDeferredKCCBCmd = IMG_CONTAINER_OF(psNode, RGX_DEFERRED_KCCB_CMD, sListNode);
			eError = RGXSendCommandRaw(psTempDeferredKCCBCmd->psDevInfo,
			                           &psTempDeferredKCCBCmd->sKCCBcmd,
			                           psTempDeferredKCCBCmd->uiPDumpFlags,
			                           NULL /* We surely aren't interested in kCCB slot number of deferred command */);
			if (eError != PVRSRV_OK)
			{
				if (!bPoll)
				{
					eError = PVRSRV_ERROR_KERNEL_CCB_FULL;
					goto cleanup_;
				}
				break;
			}

			_FreeDeferredCommand(psNode, psTempDeferredKCCBCmd);
		}

		if (bPoll)
		{
			PVRSRV_ERROR eErrPollForKCCBSlot;

			/* Don't overwrite eError because if RGXPollKernelCCBSlot returns OK and the
			 * outer loop times-out, we'll still want to return KCCB_FULL to caller
			 */
			eErrPollForKCCBSlot = RGXPollKernelCCBSlot(psDevInfo);
			if (eErrPollForKCCBSlot == PVRSRV_ERROR_KERNEL_CCB_FULL)
			{
				eError = PVRSRV_ERROR_KERNEL_CCB_FULL;
				goto cleanup_;
			}
		}
	} END_LOOP_UNTIL_TIMEOUT_US();

cleanup_:
	/* if the local list is not empty put it back to the deferred list head
	 * so that the old order of commands is retained */
	OSSpinLockAcquire(psDevInfo->hLockKCCBDeferredCommandsList, uiFlags);
	dllist_insert_list_at_head(&psDevInfo->sKCCBDeferredCommandsListHead, &sCommandList);
	OSSpinLockRelease(psDevInfo->hLockKCCBDeferredCommandsList, uiFlags);

	return eError;
}

PVRSRV_ERROR RGXSendCommandAndGetKCCBSlot(PVRSRV_RGXDEV_INFO  *psDevInfo,
										  RGXFWIF_KCCB_CMD    *psKCCBCmd,
										  IMG_UINT32          uiPDumpFlags,
										  IMG_UINT32          *pui32CmdKCCBSlot)
{
	IMG_BOOL     bPoll = (pui32CmdKCCBSlot != NULL);
	PVRSRV_ERROR eError;

	/*
	 * First try to Flush all the cmds in deferred list.
	 *
	 * We cannot defer an incoming command if the caller is interested in
	 * knowing the command's kCCB slot: it plans to poll/wait for a
	 * response from the FW just after the command is enqueued, so we must
	 * poll for space to be available.
	 */
	eError = RGXSendCommandsFromDeferredList(psDevInfo, bPoll);
	if (eError == PVRSRV_OK)
	{
		eError = RGXSendCommandRaw(psDevInfo,
								   psKCCBCmd,
								   uiPDumpFlags,
								   pui32CmdKCCBSlot);
	}

	/*
	 * If we don't manage to enqueue one of the deferred commands or the command
	 * passed as argument because the KCCB is full, insert the latter into the deferred commands list.
	 * The deferred commands will also be flushed eventually by:
	 *  - one more KCCB command sent for any DM
	 *  - RGX_MISRHandler_CheckFWActivePowerState
	 */
	if (eError == PVRSRV_ERROR_KERNEL_CCB_FULL)
	{
		if (pui32CmdKCCBSlot == NULL)
		{
			eError = _AllocDeferredCommand(psDevInfo, psKCCBCmd, uiPDumpFlags);
		}
		else
		{
			/* Let the caller retry. Otherwise if we deferred the command and returned OK,
			 * the caller can end up looking in a stale CCB slot.
			 */
			PVR_DPF((PVR_DBG_WARNING, "%s: Couldn't flush the deferred queue for a command (Type:%d) "
			                        "- will be retried", __func__, psKCCBCmd->eCmdType));
		}
	}

	return eError;
}

PVRSRV_ERROR RGXSendCommandWithPowLockAndGetKCCBSlot(PVRSRV_RGXDEV_INFO	*psDevInfo,
													 RGXFWIF_KCCB_CMD	*psKCCBCmd,
													 IMG_UINT32			ui32PDumpFlags,
													 IMG_UINT32         *pui32CmdKCCBSlot)
{
	PVRSRV_ERROR		eError;
	PVRSRV_DEVICE_NODE *psDeviceNode = psDevInfo->psDeviceNode;

	/* Ensure Rogue is powered up before kicking MTS */
	eError = PVRSRVPowerLock(psDeviceNode);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_WARNING,
				"%s: failed to acquire powerlock (%s)",
				__func__,
				PVRSRVGetErrorString(eError)));

		goto _PVRSRVPowerLock_Exit;
	}

	PDUMPPOWCMDSTART(psDeviceNode);
	eError = PVRSRVSetDevicePowerStateKM(psDeviceNode,
										 PVRSRV_DEV_POWER_STATE_ON,
										 PVRSRV_POWER_FLAGS_NONE);
	PDUMPPOWCMDEND(psDeviceNode);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: failed to transition Rogue to ON (%s)",
				__func__,
				PVRSRVGetErrorString(eError)));

		goto _PVRSRVSetDevicePowerStateKM_Exit;
	}

	eError = RGXSendCommandAndGetKCCBSlot(psDevInfo,
										  psKCCBCmd,
										  ui32PDumpFlags,
	                                      pui32CmdKCCBSlot);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: failed to schedule command (%s)",
				__func__,
				PVRSRVGetErrorString(eError)));
#if defined(DEBUG)
		/* PVRSRVDebugRequest must be called without powerlock */
		PVRSRVPowerUnlock(psDeviceNode);
		PVRSRVDebugRequest(psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
		goto _PVRSRVPowerLock_Exit;
#endif
	}

_PVRSRVSetDevicePowerStateKM_Exit:
	PVRSRVPowerUnlock(psDeviceNode);

_PVRSRVPowerLock_Exit:
	return eError;
}

void RGXScheduleProcessQueuesKM(PVRSRV_CMDCOMP_HANDLE hCmdCompHandle)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE*) hCmdCompHandle;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	OSScheduleMISR(psDevInfo->hProcessQueuesMISR);
}


/*!
*******************************************************************************

 @Function	RGX_MISRHandler_ScheduleProcessQueues

 @Description - Sends uncounted kick to all the DMs (the FW will process all
				the queue for all the DMs)
******************************************************************************/
static void RGX_MISRHandler_ScheduleProcessQueues(void *pvData)
{
	PVRSRV_DEVICE_NODE     *psDeviceNode = pvData;
	PVRSRV_RGXDEV_INFO     *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_ERROR           eError;
	PVRSRV_DEV_POWER_STATE ePowerState = PVRSRV_DEV_POWER_STATE_OFF;

	eError = PVRSRVPowerLock(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: failed to acquire powerlock (%s)",
				__func__, PVRSRVGetErrorString(eError)));
		return;
	}

	/* Check whether it's worth waking up the GPU */
	eError = PVRSRVGetDevicePowerState(psDeviceNode, &ePowerState);

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		/* Guests are not permitted to change the device power state */
		if ((eError != PVRSRV_OK) || (ePowerState == PVRSRV_DEV_POWER_STATE_OFF))
		{
			PVRSRVPowerUnlock(psDeviceNode);
			return;
		}
	}
	else
	{
		if ((eError == PVRSRV_OK) && (ePowerState == PVRSRV_DEV_POWER_STATE_OFF))
		{
			RGXFWIF_GPU_UTIL_FW    *psUtilFW = psDevInfo->psRGXFWIfGpuUtilFW;
			IMG_BOOL               bGPUHasWorkWaiting;

			/* Check whether it's worth waking up the GPU */
			RGXFwSharedMemCacheOpValue(psUtilFW->ui64GpuLastWord, INVALIDATE);
			bGPUHasWorkWaiting =
			    (RGXFWIF_GPU_UTIL_GET_STATE(psUtilFW->ui64GpuLastWord) == RGXFWIF_GPU_UTIL_STATE_BLOCKED);

			if (!bGPUHasWorkWaiting)
			{
				/* all queues are empty, don't wake up the GPU */
				PVRSRVPowerUnlock(psDeviceNode);
				return;
			}
		}

		PDUMPPOWCMDSTART(psDeviceNode);
		/* wake up the GPU */
		eError = PVRSRVSetDevicePowerStateKM(psDeviceNode,
											 PVRSRV_DEV_POWER_STATE_ON,
											 PVRSRV_POWER_FLAGS_NONE);
		PDUMPPOWCMDEND(psDeviceNode);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: failed to transition device to ON (%s)",
					__func__, PVRSRVGetErrorString(eError)));

			PVRSRVPowerUnlock(psDeviceNode);
			return;
		}
	}

	/* Memory barrier before Kick MTS */
	OSWriteMemoryBarrier(NULL);

	/* uncounted kick to the FW */
	HTBLOGK(HTB_SF_MAIN_KICK_UNCOUNTED);
	__MTSScheduleWrite(psDevInfo, (RGXFWIF_DM_GP & ~RGX_CR_MTS_SCHEDULE_DM_CLRMSK) | RGX_CR_MTS_SCHEDULE_TASK_NON_COUNTED);

	PVRSRVPowerUnlock(psDeviceNode);
}

PVRSRV_ERROR RGXInstallProcessQueuesMISR(IMG_HANDLE *phMISR, PVRSRV_DEVICE_NODE *psDeviceNode)
{
	return OSInstallMISR(phMISR,
			RGX_MISRHandler_ScheduleProcessQueues,
			psDeviceNode,
			"RGX_ScheduleProcessQueues");
}

PVRSRV_ERROR _RGXScheduleCommandAndGetKCCBSlot(PVRSRV_RGXDEV_INFO  *psDevInfo,
                                              RGXFWIF_DM          eKCCBType,
                                              RGXFWIF_KCCB_CMD    *psKCCBCmd,
                                              IMG_UINT32          ui32PDumpFlags,
                                              IMG_UINT32          *pui32CmdKCCBSlot,
                                              IMG_BOOL            bCallerHasPwrLock)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 uiMMUSyncUpdate;

	/* Don't send the command/power up request if device not available. */
	if (unlikely((psDevInfo == NULL) ||
	             (psDevInfo->psDeviceNode == NULL) ||
				 (psDevInfo->psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_PCI_ERROR)))
	{
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	/* Don't send the command/power up request if device in deinit phase.
	 * The de-init thread could destroy the device whilst the power up
	 * sequence below is accessing the HW registers.
	 * Not yet safe to free resources. Caller should retry later.
	 */
	if (psDevInfo->psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DEINIT ||
	    psDevInfo->psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DESTRUCTING)
	{
		return PVRSRV_ERROR_RETRY;
	}



	if (!bCallerHasPwrLock)
	{
		/* PVRSRVPowerLock guarantees atomicity between commands. This is helpful
		   in a scenario with several applications allocating resources. */
		eError = PVRSRVPowerLock(psDevInfo->psDeviceNode);
		if (unlikely(eError != PVRSRV_OK))
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: failed to acquire powerlock (%s)",
			         __func__, PVRSRVGetErrorString(eError)));

			/* If system is found powered OFF, Retry scheduling the command */
			if (likely(eError == PVRSRV_ERROR_SYSTEM_STATE_POWERED_OFF))
			{
				eError = PVRSRV_ERROR_RETRY;
			}

			goto RGXScheduleCommand_exit;
		}
	}

	if (unlikely(psDevInfo->psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DEINIT ||
	             psDevInfo->psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DESTRUCTING))
	{
		/* If we have the power lock the device is valid but the deinit
		 * thread could be waiting for the lock. */
		eError = PVRSRV_ERROR_RETRY;
		goto _PVRSRVInvalidDeviceError_Exit;
	}

	/* Ensure device is powered up before sending any commands */
	PDUMPPOWCMDSTART(psDevInfo->psDeviceNode);
	eError = PVRSRVSetDevicePowerStateKM(psDevInfo->psDeviceNode,
	                                     PVRSRV_DEV_POWER_STATE_ON,
	                                     PVRSRV_POWER_FLAGS_NONE);
	PDUMPPOWCMDEND(psDevInfo->psDeviceNode);
	if (unlikely(eError != PVRSRV_OK))
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: failed to transition RGX to ON (%s)",
				__func__, PVRSRVGetErrorString(eError)));
		goto _PVRSRVSetDevicePowerStateKM_Exit;
	}

	eError = RGXPreKickCacheCommand(psDevInfo, eKCCBType, &uiMMUSyncUpdate);
	if (unlikely(eError != PVRSRV_OK)) goto _PVRSRVSetDevicePowerStateKM_Exit;

	eError = RGXSendCommandAndGetKCCBSlot(psDevInfo, psKCCBCmd, ui32PDumpFlags, pui32CmdKCCBSlot);
	if (unlikely(eError != PVRSRV_OK)) goto _PVRSRVSetDevicePowerStateKM_Exit;

_PVRSRVSetDevicePowerStateKM_Exit:
_PVRSRVInvalidDeviceError_Exit:
	if (!bCallerHasPwrLock)
	{
		PVRSRVPowerUnlock(psDevInfo->psDeviceNode);
	}
RGXScheduleCommand_exit:
	return eError;
}

/*
 * RGXCheckFirmwareCCB
 */
void RGXCheckFirmwareCCB(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	RGXFWIF_CCB_CTL *psFWCCBCtl = psDevInfo->psFirmwareCCBCtl;
	RGXFWIF_CCB_CTL *psFWCCBCtlLocal = psDevInfo->psFirmwareCCBCtlLocal;
	IMG_UINT8 *psFWCCB = psDevInfo->psFirmwareCCB;
	PVRSRV_ERROR eError;

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
	KM_CONNECTION_CACHEOP(Os, INVALIDATE);
	PVR_LOG_RETURN_VOID_IF_FALSE(PVRSRV_VZ_MODE_IS(NATIVE, DEVINFO, psDevInfo) ||
								 (KM_FW_CONNECTION_IS(ACTIVE, psDevInfo) &&
								  (KM_OS_CONNECTION_IS(ACTIVE, psDevInfo) || KM_OS_CONNECTION_IS(READY, psDevInfo))),
								 "FW-KM connection is down");
#endif

	eError = RGXUpdateLocalFWCCBWoff(psDevInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_LOG_ERROR(eError, "RGXUpdateLocalFWCCBWoff");
		return;
	}

	while (psFWCCBCtlLocal->ui32ReadOffset != psFWCCBCtlLocal->ui32WriteOffset)
	{
		/* Point to the next command */
		const RGXFWIF_FWCCB_CMD *psFwCCBCmd = ((RGXFWIF_FWCCB_CMD *)psFWCCB) + psFWCCBCtlLocal->ui32ReadOffset;
		RGXFwSharedMemCacheOpPtr(psFwCCBCmd, INVALIDATE);


		HTBLOGK(HTB_SF_MAIN_FWCCB_CMD, psFwCCBCmd->eCmdType);
		switch (psFwCCBCmd->eCmdType)
		{
			case RGXFWIF_FWCCB_CMD_ZSBUFFER_BACKING:
			{
				if (psDevInfo->bPDPEnabled)
				{
					PDUMP_PANIC(psDevInfo->psDeviceNode, ZSBUFFER_BACKING, "Request to add backing to ZSBuffer");
				}
				RGXProcessRequestZSBufferBacking(psDevInfo,
				        psFwCCBCmd->uCmdData.sCmdZSBufferBacking.ui32ZSBufferID);
				break;
			}

			case RGXFWIF_FWCCB_CMD_ZSBUFFER_UNBACKING:
			{
				if (psDevInfo->bPDPEnabled)
				{
					PDUMP_PANIC(psDevInfo->psDeviceNode, ZSBUFFER_UNBACKING, "Request to remove backing from ZSBuffer");
				}
				RGXProcessRequestZSBufferUnbacking(psDevInfo,
				        psFwCCBCmd->uCmdData.sCmdZSBufferBacking.ui32ZSBufferID);
				break;
			}

			case RGXFWIF_FWCCB_CMD_FREELIST_GROW:
			{
				if (psDevInfo->bPDPEnabled)
				{
					PDUMP_PANIC(psDevInfo->psDeviceNode, FREELIST_GROW, "Request to grow the free list");
				}
				RGXProcessRequestGrow(psDevInfo,
				        psFwCCBCmd->uCmdData.sCmdFreeListGS.ui32FreelistID);
				break;
			}

			case RGXFWIF_FWCCB_CMD_FREELISTS_RECONSTRUCTION:
			{
				if (psDevInfo->bPDPEnabled)
				{
					PDUMP_PANIC(psDevInfo->psDeviceNode, FREELISTS_RECONSTRUCTION, "Request to reconstruct free lists");
				}

				if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
				{
					PVR_DPF((PVR_DBG_MESSAGE, "%s: Freelist reconstruction request (%d) for %d freelists",
							__func__,
							psFwCCBCmd->uCmdData.sCmdFreeListsReconstruction.ui32HwrCounter+1,
							psFwCCBCmd->uCmdData.sCmdFreeListsReconstruction.ui32FreelistsCount));
				}
				else
				{
					PVR_DPF((PVR_DBG_MESSAGE, "%s: Freelist reconstruction request (%d/%d) for %d freelists",
							__func__,
							psFwCCBCmd->uCmdData.sCmdFreeListsReconstruction.ui32HwrCounter+1,
							psDevInfo->psRGXFWIfHWRInfoBufCtl->ui32HwrCounter+1,
							psFwCCBCmd->uCmdData.sCmdFreeListsReconstruction.ui32FreelistsCount));
				}

				RGXProcessRequestFreelistsReconstruction(psDevInfo,
				        psFwCCBCmd->uCmdData.sCmdFreeListsReconstruction.ui32FreelistsCount,
				        psFwCCBCmd->uCmdData.sCmdFreeListsReconstruction.aui32FreelistIDs);
				break;
			}

			case RGXFWIF_FWCCB_CMD_CONTEXT_FW_PF_NOTIFICATION:
			{
				/* Notify client drivers */
				/* Client notification of device error will be achieved by
				 * clients calling UM function RGXGetLastDeviceError() */
				psDevInfo->eLastDeviceError = RGX_CONTEXT_RESET_REASON_FW_PAGEFAULT;

				/* Notify system layer */
				{
					PVRSRV_DEVICE_NODE *psDevNode = psDevInfo->psDeviceNode;
					PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;
					const RGXFWIF_FWCCB_CMD_FW_PAGEFAULT_DATA *psCmdFwPagefault =
							&psFwCCBCmd->uCmdData.sCmdFWPagefault;

					if (psDevConfig->pfnSysDevErrorNotify)
					{
						PVRSRV_ROBUSTNESS_NOTIFY_DATA sErrorData = {0};

						sErrorData.eResetReason = RGX_CONTEXT_RESET_REASON_FW_PAGEFAULT;
						sErrorData.uErrData.sFwPFErrData.sFWFaultAddr.uiAddr = psCmdFwPagefault->sFWFaultAddr.uiAddr;

						psDevConfig->pfnSysDevErrorNotify(psDevConfig,
														  &sErrorData);
					}
				}
				break;
			}

			case RGXFWIF_FWCCB_CMD_CONTEXT_RESET_NOTIFICATION:
			{
				const RGXFWIF_FWCCB_CMD_CONTEXT_RESET_DATA *psCmdContextResetNotification =
						&psFwCCBCmd->uCmdData.sCmdContextResetNotification;
				IMG_UINT32 ui32ErrorPid = 0;

				FWCommonContextListSetLastResetReason(psDevInfo,
				                                      &ui32ErrorPid,
				                                      psCmdContextResetNotification);

				/* Increment error counter (if appropriate) */
				if (psCmdContextResetNotification->eResetReason == RGX_CONTEXT_RESET_REASON_WGP_CHECKSUM)
				{
					/* Avoid wrapping the error count (which would then
					 * make it appear we had far fewer errors), by limiting
					 * it to IMG_UINT32_MAX.
					 */
					if (psDevInfo->sErrorCounts.ui32WGPErrorCount < IMG_UINT32_MAX)
					{
						psDevInfo->sErrorCounts.ui32WGPErrorCount++;
					}
				}
				else if (psCmdContextResetNotification->eResetReason == RGX_CONTEXT_RESET_REASON_TRP_CHECKSUM)
				{
					/* Avoid wrapping the error count (which would then
					 * make it appear we had far fewer errors), by limiting
					 * it to IMG_UINT32_MAX.
					 */
					if (psDevInfo->sErrorCounts.ui32TRPErrorCount < IMG_UINT32_MAX)
					{
						psDevInfo->sErrorCounts.ui32TRPErrorCount++;
					}
				}

				/* Notify system layer */
				{
					PVRSRV_DEVICE_NODE *psDevNode = psDevInfo->psDeviceNode;
					PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;

					if (psDevConfig->pfnSysDevErrorNotify)
					{
						PVRSRV_ROBUSTNESS_NOTIFY_DATA sErrorData = {0};

						sErrorData.eResetReason = psCmdContextResetNotification->eResetReason;
						sErrorData.pid = ui32ErrorPid;

						/* Populate error data according to reset reason */
						switch (psCmdContextResetNotification->eResetReason)
						{
							case RGX_CONTEXT_RESET_REASON_WGP_CHECKSUM:
							case RGX_CONTEXT_RESET_REASON_TRP_CHECKSUM:
							{
								sErrorData.uErrData.sChecksumErrData.ui32ExtJobRef = psCmdContextResetNotification->ui32ResetJobRef;
								sErrorData.uErrData.sChecksumErrData.eDM = psCmdContextResetNotification->eDM;
								break;
							}
							default:
							{
								break;
							}
						}

						psDevConfig->pfnSysDevErrorNotify(psDevConfig,
						                                  &sErrorData);
					}
				}

				/* Notify if a page fault */
				if (psCmdContextResetNotification->ui32Flags & RGXFWIF_FWCCB_CMD_CONTEXT_RESET_FLAG_PF)
				{
					DevmemIntPFNotify(psDevInfo->psDeviceNode,
							psCmdContextResetNotification->ui64PCAddress,
							psCmdContextResetNotification->sFaultAddress);
				}
				break;
			}

			case RGXFWIF_FWCCB_CMD_DEBUG_DUMP:
			{
				PVRSRV_ERROR eError;
				PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
				OSAtomicWrite(&psDevInfo->psDeviceNode->eDebugDumpRequested, PVRSRV_DEVICE_DEBUG_DUMP_CAPTURE);
				eError = OSEventObjectSignal(psPVRSRVData->hDevicesWatchdogEvObj);
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR, "%s: Failed to signal FW Cmd debug dump event, dumping now instead", __func__));
					PVRSRVDebugRequest(psDevInfo->psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
				}
				break;
			}

			case RGXFWIF_FWCCB_CMD_UPDATE_STATS:
			{
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
				IMG_PID pidTmp = psFwCCBCmd->uCmdData.sCmdUpdateStatsData.pidOwner;
				IMG_INT32 i32AdjustmentValue = psFwCCBCmd->uCmdData.sCmdUpdateStatsData.i32AdjustmentValue;

				switch (psFwCCBCmd->uCmdData.sCmdUpdateStatsData.eElementToUpdate)
				{
					case RGXFWIF_FWCCB_CMD_UPDATE_NUM_PARTIAL_RENDERS:
					{
						PVRSRVStatsUpdateRenderContextStats(psDevInfo->psDeviceNode,i32AdjustmentValue,0,0,0,0,0,0,pidTmp);
						break;
					}
					case RGXFWIF_FWCCB_CMD_UPDATE_NUM_OUT_OF_MEMORY:
					{
						PVRSRVStatsUpdateRenderContextStats(psDevInfo->psDeviceNode,0,i32AdjustmentValue,0,0,0,0,0,pidTmp);
						break;
					}
					case RGXFWIF_FWCCB_CMD_UPDATE_NUM_TA_STORES:
					{
						PVRSRVStatsUpdateRenderContextStats(psDevInfo->psDeviceNode,0,0,i32AdjustmentValue,0,0,0,0,pidTmp);
						break;
					}
					case RGXFWIF_FWCCB_CMD_UPDATE_NUM_3D_STORES:
					{
						PVRSRVStatsUpdateRenderContextStats(psDevInfo->psDeviceNode,0,0,0,i32AdjustmentValue,0,0,0,pidTmp);
						break;
					}
					case RGXFWIF_FWCCB_CMD_UPDATE_NUM_CDM_STORES:
					{
						PVRSRVStatsUpdateRenderContextStats(psDevInfo->psDeviceNode,0,0,0,0,i32AdjustmentValue,0,0,pidTmp);
						break;
					}
					case RGXFWIF_FWCCB_CMD_UPDATE_NUM_TDM_STORES:
					{
						PVRSRVStatsUpdateRenderContextStats(psDevInfo->psDeviceNode,0,0,0,0,0,i32AdjustmentValue,0,pidTmp);
						break;
					}
#if defined(RGX_FEATURE_RAY_TRACING_ARCH_MAX_VALUE_IDX)
					case RGXFWIF_FWCCB_CMD_UPDATE_NUM_RAY_STORES:
					{
						PVRSRVStatsUpdateRenderContextStats(psDevInfo->psDeviceNode,0,0,0,0,0,0,i32AdjustmentValue,pidTmp);
						break;
					}
#endif
				}
#endif
				break;
			}
#if defined(SUPPORT_FW_CORE_CLK_RATE_CHANGE_NOTIFY)
			case RGXFWIF_FWCCB_CMD_CORE_CLK_RATE_CHANGE:
			{
				RGX_PROCESS_CORE_CLK_RATE_CHANGE(psDevInfo,
											  psFwCCBCmd->uCmdData.sCmdCoreClkRateChange.ui32CoreClkRate);
				break;
			}
#endif
			case RGXFWIF_FWCCB_CMD_REQUEST_GPU_RESTART:
			{
				RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfFwSysData, INVALIDATE);
				if (psDevInfo->psRGXFWIfFwSysData != NULL  &&
				    psDevInfo->psRGXFWIfFwSysData->ePowState != RGXFWIF_POW_OFF)
				{
					PVRSRV_ERROR eError;

					/* Power down... */
					eError = PVRSRVSetDeviceSystemPowerState(psDevInfo->psDeviceNode,
															 PVRSRV_SYS_POWER_STATE_OFF,
															 PVRSRV_POWER_FLAGS_NONE);
					if (eError == PVRSRV_OK)
					{
						/* Clear the FW faulted flags... */
						psDevInfo->psRGXFWIfFwSysData->ui32HWRStateFlags &= ~(RGXFWIF_HWR_FW_FAULT|RGXFWIF_HWR_RESTART_REQUESTED);
						OSWriteMemoryBarrier(&psDevInfo->psRGXFWIfFwSysData->ui32HWRStateFlags);
						RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfFwSysData->ui32HWRStateFlags,
						                           FLUSH);

						/* Power back up again... */
						eError = PVRSRVSetDeviceSystemPowerState(psDevInfo->psDeviceNode,
																 PVRSRV_SYS_POWER_STATE_ON,
																 PVRSRV_POWER_FLAGS_NONE);

						/* Send a dummy KCCB command to ensure the FW wakes up and checks the queues... */
						if (eError == PVRSRV_OK)
						{
							LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
							{
								eError = RGXFWHealthCheckCmd(psDevInfo);
								if (eError != PVRSRV_ERROR_RETRY)
								{
									break;
								}
								OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
							} END_LOOP_UNTIL_TIMEOUT_US();
						}
					}

					/* Notify client drivers and system layer of FW fault */
					{
						PVRSRV_DEVICE_NODE *psDevNode = psDevInfo->psDeviceNode;
						PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;

						/* Client notification of device error will be achieved by
						 * clients calling UM function RGXGetLastDeviceError() */
						psDevInfo->eLastDeviceError = RGX_CONTEXT_RESET_REASON_FW_EXEC_ERR;

						/* Notify system layer */
						if (psDevConfig->pfnSysDevErrorNotify)
						{
							PVRSRV_ROBUSTNESS_NOTIFY_DATA sErrorData = {0};

							sErrorData.eResetReason = RGX_CONTEXT_RESET_REASON_FW_EXEC_ERR;
							psDevConfig->pfnSysDevErrorNotify(psDevConfig,
							                                  &sErrorData);
						}
					}

					if (eError != PVRSRV_OK)
					{
						PVR_DPF((PVR_DBG_ERROR, "%s: Failed firmware restart (%s)",
								 __func__, PVRSRVGetErrorString(eError)));
					}
				}
				break;
			}
			default:
			{
				/* unknown command */
				PVR_DPF((PVR_DBG_WARNING, "%s: Unknown Command (eCmdType=0x%08x)",
				         __func__, psFwCCBCmd->eCmdType));
				/* Assert on magic value corruption */
				PVR_ASSERT((((IMG_UINT32)psFwCCBCmd->eCmdType & RGX_CMD_MAGIC_DWORD_MASK) >> RGX_CMD_MAGIC_DWORD_SHIFT) == RGX_CMD_MAGIC_DWORD);
			}
		}

		/* Update read offset */
		psFWCCBCtlLocal->ui32ReadOffset = (psFWCCBCtlLocal->ui32ReadOffset + 1) & psFWCCBCtlLocal->ui32WrapMask;
		OSMemoryBarrier(NULL);
		psFWCCBCtl->ui32ReadOffset = psFWCCBCtlLocal->ui32ReadOffset;
		OSWriteMemoryBarrier(NULL);

		if (psFWCCBCtlLocal->ui32ReadOffset == psFWCCBCtlLocal->ui32WriteOffset)
		{
			eError = RGXUpdateLocalFWCCBWoff(psDevInfo);
			if (eError != PVRSRV_OK)
			{
				PVR_LOG_ERROR(eError, "RGXUpdateLocalFWCCBWoff");
				return;
			}
		}
	}
}

/*
 * PVRSRVRGXFrameworkCopyCommand
*/
PVRSRV_ERROR PVRSRVRGXFrameworkCopyCommand(PVRSRV_DEVICE_NODE *psDeviceNode,
		DEVMEM_MEMDESC	*psFWFrameworkMemDesc,
		IMG_PBYTE		pbyGPUFRegisterList,
		IMG_UINT32		ui32FrameworkRegisterSize)
{
	PVRSRV_ERROR	eError;
	RGXFWIF_RF_REGISTERS	*psRFReg;

	eError = DevmemAcquireCpuVirtAddr(psFWFrameworkMemDesc,
			(void **)&psRFReg);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to map firmware render context state (%u)",
				__func__, eError));
		return eError;
	}

	OSDeviceMemCopy(psRFReg, pbyGPUFRegisterList, ui32FrameworkRegisterSize);
	RGXFwSharedMemCacheOpPtr(psRFReg, FLUSH);

	/* Release the CPU mapping */
	DevmemReleaseCpuVirtAddr(psFWFrameworkMemDesc);

	/*
	 * Dump the FW framework buffer
	 */
#if defined(PDUMP)
	PDUMPCOMMENT(psDeviceNode, "Dump FWFramework buffer");
	DevmemPDumpLoadMem(psFWFrameworkMemDesc, 0, ui32FrameworkRegisterSize, PDUMP_FLAGS_CONTINUOUS);
#else
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
#endif

	return PVRSRV_OK;
}

/*
 * PVRSRVRGXFrameworkCreateKM
*/
PVRSRV_ERROR PVRSRVRGXFrameworkCreateKM(PVRSRV_DEVICE_NODE	*psDeviceNode,
		DEVMEM_MEMDESC		**ppsFWFrameworkMemDesc,
		IMG_UINT32			ui32FrameworkCommandSize)
{
	PVRSRV_ERROR			eError;
	PVRSRV_RGXDEV_INFO		*psDevInfo = psDeviceNode->pvDevice;

	/*
		Allocate device memory for the firmware GPU framework state.
		Sufficient info to kick one or more DMs should be contained in this buffer
	 */
	PDUMPCOMMENT(psDeviceNode, "Allocate firmware framework state");

	eError = DevmemFwAllocate(psDevInfo,
			ui32FrameworkCommandSize,
			RGX_FWCOMCTX_ALLOCFLAGS,
			"FwGPUFrameworkState",
			ppsFWFrameworkMemDesc);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"%s: Failed to allocate firmware framework state (%u)",
				__func__, eError));
		return eError;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXPollForGPCommandCompletion(PVRSRV_DEVICE_NODE  *psDevNode,
												volatile IMG_UINT32	__iomem *pui32LinMemAddr,
												IMG_UINT32			ui32Value,
												IMG_UINT32			ui32Mask)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 ui32CurrentQueueLength, ui32MaxRetries;
	PVRSRV_RGXDEV_INFO	*psDevInfo = psDevNode->pvDevice;
	RGXFWIF_CCB_CTL *psKCCBCtlLocal = psDevInfo->psKernelCCBCtlLocal;

	eError = RGXUpdateLocalKCCBRoff(psDevInfo);
	PVR_LOG_RETURN_IF_ERROR(eError, "RGXUpdateLocalKCCBRoff");

	ui32CurrentQueueLength = (psKCCBCtlLocal->ui32WrapMask+1 +
					psKCCBCtlLocal->ui32WriteOffset -
					psKCCBCtlLocal->ui32ReadOffset) & psKCCBCtlLocal->ui32WrapMask;
	ui32CurrentQueueLength += psDevInfo->ui32KCCBDeferredCommandsCount;

	for (ui32MaxRetries = ui32CurrentQueueLength + 1;
				ui32MaxRetries > 0;
				ui32MaxRetries--)
	{

		/*
		 * PVRSRVPollForValueKM flags are set to POLL_FLAG_NONE in this case so that the function
		 * does not generate an error message. In this case, the PollForValueKM is expected to
		 * timeout as there is work ongoing on the GPU which may take longer than the timeout period.
		 */
		eError = PVRSRVPollForValueKM(psDevNode, pui32LinMemAddr, ui32Value, ui32Mask, POLL_FLAG_NONE, NULL);
		if (eError != PVRSRV_ERROR_TIMEOUT)
		{
			break;
		}

		RGXSendCommandsFromDeferredList(psDevInfo, IMG_FALSE);
	}

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed! Error(%s) CPU linear address(%p) Expected value(%u)",
		         __func__, PVRSRVGetErrorString(eError),
		         pui32LinMemAddr, ui32Value));
	}

	return eError;
}

PVRSRV_ERROR RGXStateFlagCtrl(PVRSRV_RGXDEV_INFO *psDevInfo,
				IMG_UINT32 ui32Config,
				IMG_UINT32 *pui32ConfigState,
				IMG_BOOL bSetNotClear)
{
	PVRSRV_ERROR eError;
	PVRSRV_DEV_POWER_STATE ePowerState;
	RGXFWIF_KCCB_CMD sStateFlagCmd = { 0 };
	PVRSRV_DEVICE_NODE *psDeviceNode;
	RGXFWIF_SYSDATA *psFwSysData;
	IMG_UINT32 ui32kCCBCommandSlot;
	IMG_BOOL bWaitForFwUpdate = IMG_FALSE;

	if (!psDevInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	psDeviceNode = psDevInfo->psDeviceNode;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_ERROR_NOT_SUPPORTED);

	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfFwSysData, INVALIDATE);
	psFwSysData = psDevInfo->psRGXFWIfFwSysData;

	if (NULL == psFwSysData)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Fw Sys Config is not mapped into CPU space", __func__));
		return PVRSRV_ERROR_INVALID_CPU_ADDR;
	}

	/* apply change and ensure the new data is written to memory
	 * before requesting the FW to read it
	 */
	ui32Config = ui32Config & RGXFWIF_INICFG_ALL;
	if (bSetNotClear)
	{
		psFwSysData->ui32ConfigFlags |= ui32Config;
	}
	else
	{
		psFwSysData->ui32ConfigFlags &= ~ui32Config;
	}
	OSWriteMemoryBarrier(&psFwSysData->ui32ConfigFlags);
	RGXFwSharedMemCacheOpValue(psFwSysData->ui32ConfigFlags, FLUSH);

	/* return current/new value to caller */
	if (pui32ConfigState)
	{
		*pui32ConfigState = psFwSysData->ui32ConfigFlags;
	}

	OSMemoryBarrier(&psFwSysData->ui32ConfigFlags);

	eError = PVRSRVPowerLock(psDeviceNode);
	PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVPowerLock");

	/* notify FW to update setting */
	eError = PVRSRVGetDevicePowerState(psDeviceNode, &ePowerState);

	if ((eError == PVRSRV_OK) && (ePowerState != PVRSRV_DEV_POWER_STATE_OFF))
	{
		/* Ask the FW to update its cached version of the value */
		sStateFlagCmd.eCmdType = RGXFWIF_KCCB_CMD_STATEFLAGS_CTRL;

		eError = RGXSendCommandAndGetKCCBSlot(psDevInfo,
											  &sStateFlagCmd,
											  PDUMP_FLAGS_CONTINUOUS,
											  &ui32kCCBCommandSlot);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSendCommandAndGetKCCBSlot", unlock);
		bWaitForFwUpdate = IMG_TRUE;
	}

unlock:
	PVRSRVPowerUnlock(psDeviceNode);
	if (bWaitForFwUpdate)
	{
		/* Wait for the value to be updated as the FW validates
		 * the parameters and modifies the ui32ConfigFlags
		 * accordingly
		 * (for completeness as registered callbacks should also
		 *  not permit invalid transitions)
		 */
		eError = RGXWaitForKCCBSlotUpdate(psDevInfo, ui32kCCBCommandSlot, PDUMP_FLAGS_CONTINUOUS);
		PVR_LOG_IF_ERROR(eError, "RGXWaitForKCCBSlotUpdate");
	}
	return eError;
}

static
PVRSRV_ERROR RGXScheduleCleanupCommand(PVRSRV_RGXDEV_INFO	*psDevInfo,
									   RGXFWIF_DM			eDM,
									   RGXFWIF_KCCB_CMD		*psKCCBCmd,
									   RGXFWIF_CLEANUP_TYPE	eCleanupType,
									   IMG_UINT32			ui32PDumpFlags)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32kCCBCommandSlot;
#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
	PVRSRV_DEVICE_HEALTH_STATUS eHealthStatus = OSAtomicRead(&psDevInfo->psDeviceNode->eHealthStatus);

	PVR_LOG_RETURN_IF_FALSE((eHealthStatus != PVRSRV_DEVICE_HEALTH_STATUS_DEAD) &&
							(eHealthStatus != PVRSRV_DEVICE_HEALTH_STATUS_FAULT),
							"Cleanup aborted: Device in bad state", PVRSRV_OK);
#endif


	/* Clean-up commands sent during frame capture intervals must be dumped even when not in capture range... */
	ui32PDumpFlags |= PDUMP_FLAGS_INTERVAL;

	psKCCBCmd->eCmdType = RGXFWIF_KCCB_CMD_CLEANUP;
	psKCCBCmd->uCmdData.sCleanupData.eCleanupType = eCleanupType;

	/*
		Send the cleanup request to the firmware. If the resource is still busy
		the firmware will tell us and we'll drop out with a retry.
	*/
	eError = RGXScheduleCommandAndGetKCCBSlot(psDevInfo,
											  eDM,
											  psKCCBCmd,
											  ui32PDumpFlags,
											  &ui32kCCBCommandSlot);
	if (eError != PVRSRV_OK)
	{
		/* If caller may retry, fail with no error message */
		if (!PVRSRVIsRetryError(eError))
		{
			PVR_DPF((PVR_DBG_ERROR ,"RGXScheduleCommandAndGetKCCBSlot() failed (%s) in %s()",
			         PVRSRVGETERRORSTRING(eError), __func__));
		}
		goto fail_command;
	}

	/* Wait for command kCCB slot to be updated by FW */
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, ui32PDumpFlags,
						  "Wait for the firmware to reply to the cleanup command");
	eError = RGXWaitForKCCBSlotUpdate(psDevInfo, ui32kCCBCommandSlot,
									  ui32PDumpFlags);
	/*
		If the firmware hasn't got back to us in a timely manner
		then bail and let the caller retry the command.
	 */
	if (eError == PVRSRV_ERROR_TIMEOUT)
	{
		PVR_DPF((PVR_DBG_WARNING,
		         "%s: RGXWaitForKCCBSlotUpdate timed out. Dump debug information.",
		         __func__));

		eError = PVRSRV_ERROR_RETRY;
#if defined(DEBUG)
		PVRSRVDebugRequest(psDevInfo->psDeviceNode,
				DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
#endif
		goto fail_poll;
	}
	else if (eError != PVRSRV_OK)
	{
		goto fail_poll;
	}

#if defined(PDUMP)
	/*
	 * The cleanup request to the firmware will tell us if a given resource is busy or not.
	 * If the RGXFWIF_KCCB_RTN_SLOT_CLEANUP_BUSY flag is set, this means that the resource is
	 * still in use. In this case we return a PVRSRV_ERROR_RETRY error to the client drivers
	 * and they will re-issue the cleanup request until it succeed.
	 *
	 * Since this retry mechanism doesn't work for pdumps, client drivers should ensure
	 * that cleanup requests are only submitted if the resource is unused.
	 * If this is not the case, the following poll will block infinitely, making sure
	 * the issue doesn't go unnoticed.
	 */
	PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode, ui32PDumpFlags,
					"Cleanup: If this poll fails, the following resource is still in use (DM=%u, type=%u, address=0x%08x), which is incorrect in pdumps",
					eDM,
					psKCCBCmd->uCmdData.sCleanupData.eCleanupType,
					psKCCBCmd->uCmdData.sCleanupData.uCleanupData.psContext.ui32Addr);
	eError = DevmemPDumpDevmemPol32(psDevInfo->psKernelCCBRtnSlotsMemDesc,
									ui32kCCBCommandSlot * sizeof(IMG_UINT32),
									0,
									RGXFWIF_KCCB_RTN_SLOT_CLEANUP_BUSY,
									PDUMP_POLL_OPERATOR_EQUAL,
									ui32PDumpFlags);
	PVR_LOG_IF_ERROR(eError, "DevmemPDumpDevmemPol32");
#endif

	/*
		If the command has was run but a resource was busy, then the request
		will need to be retried.
	*/
	RGXFwSharedMemCacheOpValue(psDevInfo->pui32KernelCCBRtnSlots[ui32kCCBCommandSlot],
	                           INVALIDATE);

	if (unlikely(psDevInfo->pui32KernelCCBRtnSlots[ui32kCCBCommandSlot] & RGXFWIF_KCCB_RTN_SLOT_CLEANUP_BUSY))
	{
		if (psDevInfo->pui32KernelCCBRtnSlots[ui32kCCBCommandSlot] & RGXFWIF_KCCB_RTN_SLOT_POLL_FAILURE)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: FW poll on a HW operation failed", __func__));
		}
		eError = PVRSRV_ERROR_RETRY;
		goto fail_requestbusy;
	}

	return PVRSRV_OK;

fail_requestbusy:
fail_poll:
fail_command:
	PVR_ASSERT(eError != PVRSRV_OK);

	return eError;
}

/*
	RGXRequestCommonContextCleanUp
*/
PVRSRV_ERROR RGXFWRequestCommonContextCleanUp(PVRSRV_DEVICE_NODE *psDeviceNode,
											  RGX_SERVER_COMMON_CONTEXT *psServerCommonContext,
											  RGXFWIF_DM eDM,
											  IMG_UINT32 ui32PDumpFlags)
{
	RGXFWIF_KCCB_CMD			sRCCleanUpCmd = {0};
	PVRSRV_ERROR				eError;
	PRGXFWIF_FWCOMMONCONTEXT	psFWCommonContextFWAddr;
	PVRSRV_RGXDEV_INFO			*psDevInfo = (PVRSRV_RGXDEV_INFO*)psDeviceNode->pvDevice;

	/* Force retry if this context's CCB is currently being dumped
	 * as part of the stalled CCB debug */
	if (psDevInfo->pvEarliestStalledClientCCB == (void*)FWCommonContextGetClientCCB(psServerCommonContext))
	{
		PVR_DPF((PVR_DBG_WARNING,
		         "%s: Forcing retry as psDevInfo->pvEarliestStalledClientCCB = psCtxClientCCB <%p>",
		         __func__,
		         psDevInfo->pvEarliestStalledClientCCB));
		return PVRSRV_ERROR_RETRY;
	}

	psFWCommonContextFWAddr = FWCommonContextGetFWAddress(psServerCommonContext);
#if defined(PDUMP)
	PDUMPCOMMENT(psDeviceNode, "Common ctx cleanup Request DM%d [context = 0x%08x]",
			eDM, psFWCommonContextFWAddr.ui32Addr);
	PDUMPCOMMENT(psDeviceNode, "Wait for CCB to be empty before common ctx cleanup");

	RGXCCBPDumpDrainCCB(FWCommonContextGetClientCCB(psServerCommonContext), ui32PDumpFlags);
#endif

	/* Setup our command data, the cleanup call will fill in the rest */
	sRCCleanUpCmd.uCmdData.sCleanupData.uCleanupData.psContext = psFWCommonContextFWAddr;

	/* Request cleanup of the firmware resource */
	eError = RGXScheduleCleanupCommand(psDeviceNode->pvDevice,
									   eDM,
									   &sRCCleanUpCmd,
									   RGXFWIF_CLEANUP_FWCOMMONCONTEXT,
									   ui32PDumpFlags);

	if ((eError != PVRSRV_OK) && !PVRSRVIsRetryError(eError))
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed to schedule a memory context cleanup with error (%u)",
		         __func__, eError));
	}

	return eError;
}

/*
 * RGXFWRequestHWRTDataCleanUp
 */

PVRSRV_ERROR RGXFWRequestHWRTDataCleanUp(PVRSRV_DEVICE_NODE *psDeviceNode,
                                         PRGXFWIF_HWRTDATA psHWRTData)
{
	RGXFWIF_KCCB_CMD			sHWRTDataCleanUpCmd = {0};
	PVRSRV_ERROR				eError;

	PDUMPCOMMENT(psDeviceNode, "HW RTData cleanup Request [HWRTData = 0x%08x]", psHWRTData.ui32Addr);

	sHWRTDataCleanUpCmd.uCmdData.sCleanupData.uCleanupData.psHWRTData = psHWRTData;

	eError = RGXScheduleCleanupCommand(psDeviceNode->pvDevice,
	                                   RGXFWIF_DM_GP,
	                                   &sHWRTDataCleanUpCmd,
	                                   RGXFWIF_CLEANUP_HWRTDATA,
	                                   PDUMP_FLAGS_NONE);

	if (eError != PVRSRV_OK)
	{
		/* If caller may retry, fail with no error message */
		if (!PVRSRVIsRetryError(eError))
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Failed to schedule a HWRTData cleanup with error (%u)",
			         __func__, eError));
		}
	}

	return eError;
}

/*
	RGXFWRequestFreeListCleanUp
*/
PVRSRV_ERROR RGXFWRequestFreeListCleanUp(PVRSRV_RGXDEV_INFO *psDevInfo,
										 PRGXFWIF_FREELIST psFWFreeList)
{
	RGXFWIF_KCCB_CMD			sFLCleanUpCmd = {0};
	PVRSRV_ERROR				eError;

	PDUMPCOMMENT(psDevInfo->psDeviceNode, "Free list cleanup Request [FreeList = 0x%08x]", psFWFreeList.ui32Addr);

	/* Setup our command data, the cleanup call will fill in the rest */
	sFLCleanUpCmd.uCmdData.sCleanupData.uCleanupData.psFreelist = psFWFreeList;

	/* Request cleanup of the firmware resource */
	eError = RGXScheduleCleanupCommand(psDevInfo,
									   RGXFWIF_DM_GP,
									   &sFLCleanUpCmd,
									   RGXFWIF_CLEANUP_FREELIST,
									   PDUMP_FLAGS_NONE);

	if (eError != PVRSRV_OK)
	{
		/* If caller may retry, fail with no error message */
		if (!PVRSRVIsRetryError(eError))
		{
			PVR_DPF((PVR_DBG_ERROR,
			         "%s: Failed to schedule a memory context cleanup with error (%u)",
			         __func__, eError));
		}
	}

	return eError;
}

/*
	RGXFWRequestZSBufferCleanUp
*/
PVRSRV_ERROR RGXFWRequestZSBufferCleanUp(PVRSRV_RGXDEV_INFO *psDevInfo,
										 PRGXFWIF_ZSBUFFER psFWZSBuffer)
{
	RGXFWIF_KCCB_CMD			sZSBufferCleanUpCmd = {0};
	PVRSRV_ERROR				eError;

	PDUMPCOMMENT(psDevInfo->psDeviceNode, "ZS Buffer cleanup Request [ZS Buffer = 0x%08x]", psFWZSBuffer.ui32Addr);

	/* Setup our command data, the cleanup call will fill in the rest */
	sZSBufferCleanUpCmd.uCmdData.sCleanupData.uCleanupData.psZSBuffer = psFWZSBuffer;

	/* Request cleanup of the firmware resource */
	eError = RGXScheduleCleanupCommand(psDevInfo,
									   RGXFWIF_DM_3D,
									   &sZSBufferCleanUpCmd,
									   RGXFWIF_CLEANUP_ZSBUFFER,
									   PDUMP_FLAGS_NONE);

	if ((eError != PVRSRV_OK) && !PVRSRVIsRetryError(eError))
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed to schedule a memory context cleanup with error (%u)",
		         __func__, eError));
	}

	return eError;
}

PVRSRV_ERROR RGXFWSetHCSDeadline(PVRSRV_RGXDEV_INFO *psDevInfo,
								IMG_UINT32 ui32HCSDeadlineMs)
{
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVINFO, psDevInfo, PVRSRV_ERROR_NOT_SUPPORTED);

	psDevInfo->psRGXFWIfRuntimeCfg->ui32HCSDeadlineMS = ui32HCSDeadlineMs;
	OSWriteMemoryBarrier(&psDevInfo->psRGXFWIfRuntimeCfg->ui32HCSDeadlineMS);
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfRuntimeCfg->ui32HCSDeadlineMS, FLUSH);

#if defined(PDUMP)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Updating the Hard Context Switching deadline inside RGXFWIfRuntimeCfg");
	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
							  offsetof(RGXFWIF_RUNTIME_CFG, ui32HCSDeadlineMS),
							  ui32HCSDeadlineMs,
							  PDUMP_FLAGS_CONTINUOUS);
#endif

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXFWHealthCheckCmdInt(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_BOOL bCallerHasPwrLock)
{
	RGXFWIF_KCCB_CMD	sCmpKCCBCmd = { 0 };

	sCmpKCCBCmd.eCmdType = RGXFWIF_KCCB_CMD_HEALTH_CHECK;

	if (bCallerHasPwrLock)
	{
		return	RGXScheduleCommandWithoutPowerLock(psDevInfo,
												   RGXFWIF_DM_GP,
												   &sCmpKCCBCmd,
												   PDUMP_FLAGS_CONTINUOUS);
	}
	else
	{
		return	RGXScheduleCommand(psDevInfo,
								   RGXFWIF_DM_GP,
								   &sCmpKCCBCmd,
								   PDUMP_FLAGS_CONTINUOUS);
	}
}

PVRSRV_ERROR RGXFWInjectFault(PVRSRV_RGXDEV_INFO *psDevInfo)
{
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	IMG_UINT32 ui32CBaseMapCtxReg;
#endif

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVINFO, psDevInfo, PVRSRV_ERROR_NOT_SUPPORTED);

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	if (RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) > 1)
	{
		ui32CBaseMapCtxReg = RGX_CR_MMU_CBASE_MAPPING_CONTEXT__HOST_SECURITY_GT1_AND_MHPW_LT6_AND_MMU_VER_GEQ4;
		/* Set the mapping context */
		RGXWriteReg32(&psDevInfo->sLayerParams, ui32CBaseMapCtxReg, MMU_CONTEXT_MAPPING_FWPRIV);
		(void)RGXReadReg32(&psDevInfo->sLayerParams, ui32CBaseMapCtxReg); /* Fence write */

		/*
		 * Catbase-0 (FW MMU context) pointing to unmapped mem to make
		 * FW crash from its memory context
		 */
		RGXWriteKernelMMUPC32(&psDevInfo->sLayerParams,
		                      RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1,
		                      RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_ALIGNSHIFT,
		                      RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_SHIFT,
		                      0xDEADBEEF);
	}
	else
	{
		ui32CBaseMapCtxReg = RGX_CR_MMU_CBASE_MAPPING_CONTEXT;
		/* Set the mapping context */
		RGXWriteReg32(&psDevInfo->sLayerParams, ui32CBaseMapCtxReg, MMU_CONTEXT_MAPPING_FWPRIV);
		(void)RGXReadReg32(&psDevInfo->sLayerParams, ui32CBaseMapCtxReg); /* Fence write */

		/*
		 * Catbase-0 (FW MMU context) pointing to unmapped mem to make
		 * FW crash from its memory context
		 */
		RGXWriteKernelMMUPC32(&psDevInfo->sLayerParams,
		                      RGX_CR_MMU_CBASE_MAPPING,
		                      RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_ALIGNSHIFT,
		                      RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_SHIFT,
		                      0xDEADBEEF);
	}
#else
	/*
	 * Catbase-0 (FW MMU context) pointing to unmapped mem to make
	 * FW crash from its memory context
	 */
	RGXWriteKernelMMUPC64(&psDevInfo->sLayerParams,
				                      FWCORE_MEM_CAT_BASEx(MMU_CONTEXT_MAPPING_FWPRIV),
				                      RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_ALIGNSHIFT,
				                      RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_SHIFT,
				                      ((0xDEADBEEF
				                      >> RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_ALIGNSHIFT)
				                      << RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_SHIFT)
				                      & ~RGX_CR_FWCORE_MEM_CAT_BASE0_ADDR_CLRMSK);
#endif

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXFWSetFwOsState(PVRSRV_RGXDEV_INFO *psDevInfo,
                               IMG_UINT32 ui32DriverID,
                               RGXFWIF_OS_STATE_CHANGE eOSOnlineState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	RGXFWIF_KCCB_CMD sOSOnlineStateCmd = { 0 };
	const RGXFWIF_SYSDATA *psFwSysData;

	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfFwSysData, INVALIDATE);
	psFwSysData = psDevInfo->psRGXFWIfFwSysData;

	sOSOnlineStateCmd.eCmdType = RGXFWIF_KCCB_CMD_OS_ONLINE_STATE_CONFIGURE;
	sOSOnlineStateCmd.uCmdData.sCmdOSOnlineStateData.ui32DriverID = ui32DriverID;
	sOSOnlineStateCmd.uCmdData.sCmdOSOnlineStateData.eNewOSState = eOSOnlineState;

#if defined(SUPPORT_AUTOVZ)
	{
		IMG_BOOL bConnectionDown = IMG_FALSE;

		PVR_UNREFERENCED_PARAMETER(psFwSysData);
		sOSOnlineStateCmd.uCmdData.sCmdOSOnlineStateData.eNewOSState = RGXFWIF_OS_OFFLINE;

		LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
		{
			/* Send the offline command regardless if power lock is held or not.
			 * Under AutoVz this is done during regular driver deinit, store-to-ram suspend
			 * or (optionally) from a kernel panic callback. Deinit and suspend operations
			 * take the lock in the rgx pre/post power functions as expected.
			 * The kernel panic callback is a last resort way of letting the firmware know that
			 * the VM is unrecoverable and the vz connection must be disabled. It cannot wait
			 * on other kernel threads to finish and release the lock. */
			eError = RGXSendCommand(psDevInfo,
									&sOSOnlineStateCmd,
									PDUMP_FLAGS_CONTINUOUS);

			if (eError != PVRSRV_ERROR_RETRY)
			{
				break;
			}

			OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
		} END_LOOP_UNTIL_TIMEOUT_US();

		/* Guests and Host going offline should wait for confirmation
		 * from the Firmware of the state change. If this fails, break
		 * the connection on the OS Driver's end as backup. */
		if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo) || (ui32DriverID == RGXFW_HOST_DRIVER_ID))
		{
			LOOP_UNTIL_TIMEOUT_US(SECONDS_TO_MICROSECONDS/2)
			{
				KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
				if (KM_FW_CONNECTION_IS(READY, psDevInfo))
				{
					bConnectionDown = IMG_TRUE;
					break;
				}
			} END_LOOP_UNTIL_TIMEOUT_US();

			if (!bConnectionDown)
			{
				KM_SET_OS_CONNECTION(OFFLINE, psDevInfo);
				KM_CONNECTION_CACHEOP(Os, FLUSH);
			}
		}
	}
#else
	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		/* no reason for Guests to update their state or any other VM's.
		 * This is the Hypervisor and Host driver's responsibility. */
		return PVRSRV_OK;
	}
	else
	{
		const volatile RGXFWIF_OS_RUNTIME_FLAGS *psFwRunFlags;

		PVR_ASSERT(psFwSysData != NULL);

		psFwRunFlags = (const volatile RGXFWIF_OS_RUNTIME_FLAGS*) &psFwSysData->asOsRuntimeFlagsMirror[ui32DriverID];
		/* Attempt several times until the FW manages to offload the OS */
		LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
		{
			IMG_UINT32 ui32kCCBCommandSlot;

			/* Send request */
			eError = RGXScheduleCommandAndGetKCCBSlot(psDevInfo,
													  RGXFWIF_DM_GP,
													  &sOSOnlineStateCmd,
													  PDUMP_FLAGS_CONTINUOUS,
													  &ui32kCCBCommandSlot);
			if (unlikely(eError == PVRSRV_ERROR_RETRY)) continue;
			PVR_LOG_GOTO_IF_ERROR(eError, "RGXScheduleCommand", return_);

			/* Wait for FW to process the cmd */
			eError = RGXWaitForKCCBSlotUpdate(psDevInfo, ui32kCCBCommandSlot, PDUMP_FLAGS_CONTINUOUS);
			PVR_LOG_GOTO_IF_ERROR(eError, "RGXWaitForKCCBSlotUpdate", return_);

			/* read the OS state */
			OSMemoryBarrier(NULL);
			/* check if FW finished offloading the driver and is stopped */
			if ((eOSOnlineState == RGXFWIF_OS_ONLINE && (psFwRunFlags->bfOsState == RGXFW_CONNECTION_FW_READY || psFwRunFlags->bfOsState == RGXFW_CONNECTION_FW_ACTIVE)) ||
			    (eOSOnlineState == RGXFWIF_OS_OFFLINE && psFwRunFlags->bfOsState == RGXFW_CONNECTION_FW_OFFLINE))
			{
				eError = PVRSRV_OK;
				break;
			}
			else
			{
				eError = PVRSRV_ERROR_TIMEOUT;
			}

			OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
		} END_LOOP_UNTIL_TIMEOUT_US();
	}

return_ :
#endif
	return eError;
}

PVRSRV_ERROR RGXFWConfigPHR(PVRSRV_RGXDEV_INFO *psDevInfo,
                            IMG_UINT32 ui32PHRMode)
{
	PVRSRV_ERROR eError;
	RGXFWIF_KCCB_CMD sCfgPHRCmd = { 0 };

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVINFO, psDevInfo, PVRSRV_ERROR_NOT_SUPPORTED);

	PVR_LOG_RETURN_IF_FALSE((ui32PHRMode == RGXFWIF_PHR_MODE_OFF) ||
							(ui32PHRMode == RGXFWIF_PHR_MODE_RD_RESET),
							"Invalid PHR Mode.", PVRSRV_ERROR_INVALID_PARAMS);

	sCfgPHRCmd.eCmdType = RGXFWIF_KCCB_CMD_PHR_CFG;
	psDevInfo->psRGXFWIfRuntimeCfg->ui32PHRMode = ui32PHRMode;
	OSWriteMemoryBarrier(&psDevInfo->psRGXFWIfRuntimeCfg->ui32PHRMode);
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfRuntimeCfg->ui32PHRMode, FLUSH);

#if defined(PDUMP)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Updating the Periodic Hardware Reset Mode inside RGXFWIfRuntimeCfg");
	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
							  offsetof(RGXFWIF_RUNTIME_CFG, ui32PHRMode),
							  ui32PHRMode,
							  PDUMP_FLAGS_CONTINUOUS);
#endif

	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		eError = RGXScheduleCommand(psDevInfo,
		                            RGXFWIF_DM_GP,
		                            &sCfgPHRCmd,
		                            PDUMP_FLAGS_CONTINUOUS);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	return eError;
}

PVRSRV_ERROR RGXFWConfigWdg(PVRSRV_RGXDEV_INFO *psDevInfo,
							IMG_UINT32 ui32WdgPeriodUs)
{
	PVRSRV_ERROR eError;
	RGXFWIF_KCCB_CMD sCfgWdgCmd = { 0 };

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVINFO, psDevInfo, PVRSRV_ERROR_NOT_SUPPORTED);

	sCfgWdgCmd.eCmdType = RGXFWIF_KCCB_CMD_WDG_CFG;
	psDevInfo->psRGXFWIfRuntimeCfg->ui32WdgPeriodUs = ui32WdgPeriodUs;
	OSWriteMemoryBarrier(&psDevInfo->psRGXFWIfRuntimeCfg->ui32WdgPeriodUs);
	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfRuntimeCfg->ui32WdgPeriodUs, FLUSH);

#if defined(PDUMP)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Updating the firmware watchdog period inside RGXFWIfRuntimeCfg");
	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
							  offsetof(RGXFWIF_RUNTIME_CFG, ui32WdgPeriodUs),
							  ui32WdgPeriodUs,
							  PDUMP_FLAGS_CONTINUOUS);
#endif

	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		eError = RGXScheduleCommand(psDevInfo,
									RGXFWIF_DM_GP,
									&sCfgWdgCmd,
									PDUMP_FLAGS_CONTINUOUS);
		if (eError != PVRSRV_ERROR_RETRY)
		{
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	return eError;
}


void RGXCheckForStalledClientContexts(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_BOOL bIgnorePrevious)
{
	/* Attempt to detect and deal with any stalled client contexts.
	 * bIgnorePrevious may be set by the caller if they know a context to be
	 * stalled, as otherwise this function will only identify stalled
	 * contexts which have not been previously reported.
	 */

	IMG_UINT32 ui32StalledClientMask = 0;

	if (!(OSTryLockAcquire(psDevInfo->hCCBStallCheckLock)))
	{
		PVR_LOG(("RGXCheckForStalledClientContexts: Failed to acquire hCCBStallCheckLock, returning..."));
		return;
	}

	ui32StalledClientMask |= CheckForStalledClientTransferCtxt(psDevInfo);

	ui32StalledClientMask |= CheckForStalledClientRenderCtxt(psDevInfo);

	ui32StalledClientMask |= CheckForStalledClientComputeCtxt(psDevInfo);
#if defined(SUPPORT_RGXKICKSYNC_BRIDGE)
	ui32StalledClientMask |= CheckForStalledClientKickSyncCtxt(psDevInfo);
#endif
	/* If at least one DM stalled bit is different than before */
	if (bIgnorePrevious || (psDevInfo->ui32StalledClientMask != ui32StalledClientMask))
	{
		if (ui32StalledClientMask > 0)
		{
			static __maybe_unused const char *pszStalledAction =
#if defined(PVRSRV_STALLED_CCB_ACTION)
					"force";
#else
					"warn";
#endif
			/* Print all the stalled DMs */
			PVR_LOG(("Possible stalled client RGX contexts detected: %s%s%s%s%s%s%s",
					 RGX_STRINGIFY_KICK_TYPE_DM_IF_SET(ui32StalledClientMask, RGX_KICK_TYPE_DM_GP),
					 RGX_STRINGIFY_KICK_TYPE_DM_IF_SET(ui32StalledClientMask, RGX_KICK_TYPE_DM_TDM_2D),
					 RGX_STRINGIFY_KICK_TYPE_DM_IF_SET(ui32StalledClientMask, RGX_KICK_TYPE_DM_TA),
					 RGX_STRINGIFY_KICK_TYPE_DM_IF_SET(ui32StalledClientMask, RGX_KICK_TYPE_DM_3D),
					 RGX_STRINGIFY_KICK_TYPE_DM_IF_SET(ui32StalledClientMask, RGX_KICK_TYPE_DM_CDM),
					 RGX_STRINGIFY_KICK_TYPE_DM_IF_SET(ui32StalledClientMask, RGX_KICK_TYPE_DM_TQ2D),
					 RGX_STRINGIFY_KICK_TYPE_DM_IF_SET(ui32StalledClientMask, RGX_KICK_TYPE_DM_TQ3D)));

			PVR_LOG(("Trying to identify stalled context...(%s) [%d]",
			         pszStalledAction, bIgnorePrevious));

			DumpStalledContextInfo(psDevInfo);
		}
		else
		{
			if (psDevInfo->ui32StalledClientMask> 0)
			{
				/* Indicate there are no stalled DMs */
				PVR_LOG(("No further stalled client contexts exist"));
			}
		}
		psDevInfo->ui32StalledClientMask = ui32StalledClientMask;
		psDevInfo->pvEarliestStalledClientCCB = NULL;
	}
	OSLockRelease(psDevInfo->hCCBStallCheckLock);
}

/*
	RGXUpdateHealthStatus
*/
PVRSRV_ERROR RGXUpdateHealthStatus(PVRSRV_DEVICE_NODE* psDevNode,
                                   IMG_BOOL bCheckAfterTimePassed)
{
	const PVRSRV_DATA*           psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_HEALTH_STATUS  eNewStatus   = PVRSRV_DEVICE_HEALTH_STATUS_OK;
	PVRSRV_DEVICE_HEALTH_REASON  eNewReason   = PVRSRV_DEVICE_HEALTH_REASON_NONE;
	PVRSRV_RGXDEV_INFO*          psDevInfo;
	const RGXFWIF_TRACEBUF*      psRGXFWIfTraceBufCtl;
	const RGXFWIF_SYSDATA*       psFwSysData;
	const RGXFWIF_OSDATA*        psFwOsData;
	const RGXFWIF_CCB_CTL*       psKCCBCtl;
	RGXFWIF_CCB_CTL*             psKCCBCtlLocal;
	IMG_UINT32                   ui32ThreadCount;
	IMG_BOOL                     bKCCBCmdsWaiting;
	PVRSRV_ERROR                 eError;

	PVR_ASSERT(psDevNode != NULL);
	psDevInfo = psDevNode->pvDevice;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDevNode, PVRSRV_OK);

	/* If the firmware is not yet initialised or has already deinitialised, stop here */
	if (psDevInfo  == NULL || !psDevInfo->bFirmwareInitialised || psDevInfo->pvRegsBaseKM == NULL ||
		psDevInfo->psDeviceNode == NULL || psDevInfo->psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DEINIT ||
		psDevInfo->psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DESTRUCTING)
	{
		return PVRSRV_OK;
	}

	psRGXFWIfTraceBufCtl = psDevInfo->psRGXFWIfTraceBufCtl;

	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDevNode))
	{
		RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfFwSysData,
								 INVALIDATE);
		psFwSysData = psDevInfo->psRGXFWIfFwSysData;
	}

	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfFwOsData,
	                         INVALIDATE);
	psFwOsData = psDevInfo->psRGXFWIfFwOsData;

	/* If this is a quick update, then include the last current value... */
	if (!bCheckAfterTimePassed)
	{
		eNewStatus = OSAtomicRead(&psDevNode->eHealthStatus);
		eNewReason = OSAtomicRead(&psDevNode->eHealthReason);
	}

	/* Decrement the SLR holdoff counter (if non-zero) */
	if (psDevInfo->ui32SLRHoldoffCounter > 0)
	{
		psDevInfo->ui32SLRHoldoffCounter--;
	}

	/* Take power lock, retry if it's in use in another task. */
	eError = PVRSRVPowerTryLockWaitForTimeout(psDevNode);
	if (eError == PVRSRV_ERROR_TIMEOUT)
	{
		/* Skip health status update if timeout */
		PVR_DPF((PVR_DBG_WARNING, "%s: Power lock timeout, increase OS_POWERLOCK_TIMEOUT_US.", __func__));
		goto _RGXUpdateHealthStatus_Exit;
	}
	PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVPowerTryLockWaitForTimeout");

	/* If the firmware is not yet initialised or has already deinitialised, stop here */
	if (psDevInfo  == NULL || !psDevInfo->bFirmwareInitialised || psDevInfo->pvRegsBaseKM == NULL ||
		psDevInfo->psDeviceNode == NULL || psDevInfo->psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DEINIT ||
		psDevInfo->psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DESTRUCTING)
	{
		PVRSRVPowerUnlock(psDevNode);
		return PVRSRV_OK;
	}

	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDevNode))
	{
		/* On a PCI error all reads from the PCI bar may return 0xFFFFFFFF.
		   This value is not valid for a core ID. */
		if (psFwSysData->ui32MemFaultCheck == RGX_PCI_ERROR_VALUE_DWORD)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: PCI error", __func__));
			eNewStatus = PVRSRV_DEVICE_HEALTH_STATUS_DEAD;
			eNewReason = PVRSRV_DEVICE_HEALTH_REASON_PCI_ERROR;
			PVRSRVDeviceSetState(psDevNode, PVRSRV_DEVICE_STATE_PCI_ERROR);
			PVRSRVPowerUnlock(psDevNode);
			goto _RGXUpdateHealthStatus_Exit;
		}
	}

	/* If Rogue is not powered on, just skip ahead and check for stalled client CCBs */
	if (PVRSRVIsDevicePowered(psDevNode))
	{
		if (psRGXFWIfTraceBufCtl != NULL)
		{
			/*
			   Firmware thread checks...
			 */
			for (ui32ThreadCount = 0; ui32ThreadCount < RGXFW_THREAD_NUM; ui32ThreadCount++)
			{
				const IMG_CHAR* pszTraceAssertInfo;

				RGXFwSharedMemCacheOpValue(psRGXFWIfTraceBufCtl->sTraceBuf[ui32ThreadCount].sAssertBuf, INVALIDATE);
				pszTraceAssertInfo = psRGXFWIfTraceBufCtl->sTraceBuf[ui32ThreadCount].sAssertBuf.szInfo;

				/*
				Check if the FW has hit an assert...
				*/
				if (*pszTraceAssertInfo != '\0')
				{
					PVR_DPF((PVR_DBG_WARNING, "%s: Firmware thread %d has asserted: %.*s (%.*s:%d)",
							__func__, ui32ThreadCount, RGXFW_TRACE_BUFFER_ASSERT_SIZE,
							pszTraceAssertInfo, RGXFW_TRACE_BUFFER_ASSERT_SIZE,
							psRGXFWIfTraceBufCtl->sTraceBuf[ui32ThreadCount].sAssertBuf.szPath,
							psRGXFWIfTraceBufCtl->sTraceBuf[ui32ThreadCount].sAssertBuf.ui32LineNum));
					eNewStatus = PVRSRV_DEVICE_HEALTH_STATUS_DEAD;
					eNewReason = PVRSRV_DEVICE_HEALTH_REASON_ASSERTED;
					PVRSRVPowerUnlock(psDevNode);
					goto _RGXUpdateHealthStatus_Exit;
				}

				if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDevNode))
				{
					/*
					   Check the threads to see if they are in the same poll locations as last time...
					*/
					if (bCheckAfterTimePassed)
					{
						if (psFwSysData->aui32CrPollAddr[ui32ThreadCount] != 0  &&
							psFwSysData->aui32CrPollCount[ui32ThreadCount] == psDevInfo->aui32CrLastPollCount[ui32ThreadCount])
						{
							PVR_DPF((PVR_DBG_WARNING, "%s: Firmware stuck on CR poll: T%u polling %s (reg:0x%08X mask:0x%08X)",
									__func__, ui32ThreadCount,
									((psFwSysData->aui32CrPollAddr[ui32ThreadCount] & RGXFW_POLL_TYPE_SET)?("set"):("unset")),
									psFwSysData->aui32CrPollAddr[ui32ThreadCount] & ~RGXFW_POLL_TYPE_SET,
									psFwSysData->aui32CrPollMask[ui32ThreadCount]));
							eNewStatus = PVRSRV_DEVICE_HEALTH_STATUS_NOT_RESPONDING;
							eNewReason = PVRSRV_DEVICE_HEALTH_REASON_POLL_FAILING;
							PVRSRVPowerUnlock(psDevNode);
							goto _RGXUpdateHealthStatus_Exit;
						}
						psDevInfo->aui32CrLastPollCount[ui32ThreadCount] = psFwSysData->aui32CrPollCount[ui32ThreadCount];
					}
				}
			}

			if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDevNode))
			{
				/*
				Check if the FW has faulted...
				*/
				if (psFwSysData->ui32HWRStateFlags & RGXFWIF_HWR_FW_FAULT)
				{
					PVR_DPF((PVR_DBG_WARNING,
							"%s: Firmware has faulted and needs to restart",
							__func__));
					eNewStatus = PVRSRV_DEVICE_HEALTH_STATUS_FAULT;
					if (psFwSysData->ui32HWRStateFlags & RGXFWIF_HWR_RESTART_REQUESTED)
					{
						eNewReason = PVRSRV_DEVICE_HEALTH_REASON_RESTARTING;
					}
					else
					{
						eNewReason = PVRSRV_DEVICE_HEALTH_REASON_IDLING;
					}
					PVRSRVPowerUnlock(psDevNode);
					goto _RGXUpdateHealthStatus_Exit;
				}
			}
		}

		/*
		   Event Object Timeouts check...
		*/
		if (!bCheckAfterTimePassed)
		{
			if (psDevInfo->ui32GEOTimeoutsLastTime > 1 && psPVRSRVData->ui32GEOConsecutiveTimeouts > psDevInfo->ui32GEOTimeoutsLastTime)
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: Global Event Object Timeouts have risen (from %d to %d)",
				         __func__,
				         psDevInfo->ui32GEOTimeoutsLastTime, psPVRSRVData->ui32GEOConsecutiveTimeouts));
				eNewStatus = PVRSRV_DEVICE_HEALTH_STATUS_NOT_RESPONDING;
				eNewReason = PVRSRV_DEVICE_HEALTH_REASON_TIMEOUTS;
			}
			psDevInfo->ui32GEOTimeoutsLastTime = psPVRSRVData->ui32GEOConsecutiveTimeouts;
		}

		/*
		   Check the Kernel CCB pointer is valid. If any commands were waiting last time, then check
		   that some have executed since then.
		*/
		bKCCBCmdsWaiting = IMG_FALSE;

		RGXFwSharedMemCacheOpPtr(psDevInfo->psKernelCCBCtl, INVALIDATE);
		psKCCBCtl = psDevInfo->psKernelCCBCtl;
		psKCCBCtlLocal = psDevInfo->psKernelCCBCtlLocal;

		if (psKCCBCtl != NULL && psKCCBCtlLocal != NULL)
		{
			/* update KCCB read offset */
			RGXFwSharedMemCacheOpValue(psKCCBCtl->ui32ReadOffset, INVALIDATE);
			psKCCBCtlLocal->ui32ReadOffset = psKCCBCtl->ui32ReadOffset;

			if (psKCCBCtlLocal->ui32ReadOffset > psKCCBCtlLocal->ui32WrapMask  ||
				psKCCBCtlLocal->ui32WriteOffset > psKCCBCtlLocal->ui32WrapMask)
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: KCCB has invalid offset (ROFF=%d WOFF=%d)",
						__func__, psKCCBCtlLocal->ui32ReadOffset, psKCCBCtlLocal->ui32WriteOffset));
				eNewStatus = PVRSRV_DEVICE_HEALTH_STATUS_DEAD;
				eNewReason = PVRSRV_DEVICE_HEALTH_REASON_QUEUE_CORRUPT;
			}

			if (psKCCBCtlLocal->ui32ReadOffset != psKCCBCtlLocal->ui32WriteOffset)
			{
				bKCCBCmdsWaiting = IMG_TRUE;
			}
		}

		if (bCheckAfterTimePassed && psFwOsData != NULL)
		{
			IMG_UINT32 ui32KCCBCmdsExecuted = psFwOsData->ui32KCCBCmdsExecuted;

			if (psDevInfo->ui32KCCBCmdsExecutedLastTime == ui32KCCBCmdsExecuted)
			{
				/*
				   If something was waiting last time then the Firmware has stopped processing commands.
				*/
				if (psDevInfo->bKCCBCmdsWaitingLastTime)
				{
					PVR_DPF((PVR_DBG_WARNING, "%s: No KCCB commands executed since check!",
							__func__));
					eNewStatus = PVRSRV_DEVICE_HEALTH_STATUS_NOT_RESPONDING;
					eNewReason = PVRSRV_DEVICE_HEALTH_REASON_QUEUE_STALLED;
				}

				/*
				   If no commands are currently pending and nothing happened since the last poll, then
				   schedule a dummy command to ping the firmware so we know it is alive and processing.
				*/
				if ((!bKCCBCmdsWaiting) &&
					(eNewStatus != PVRSRV_DEVICE_HEALTH_STATUS_DEAD) &&
					(eNewStatus != PVRSRV_DEVICE_HEALTH_STATUS_FAULT))
				{
					/* Protect the PDumpLoadMem. RGXScheduleCommand() cannot take the
					 * PMR lock itself, because some bridge functions will take the PMR lock
					 * before calling RGXScheduleCommand
					 */
					PVRSRV_ERROR eError = RGXFWHealthCheckCmdWithoutPowerLock(psDevNode->pvDevice);

					if (eError != PVRSRV_OK)
					{
						PVR_DPF((PVR_DBG_WARNING, "%s: Cannot schedule Health Check command! (0x%x)",
								__func__, eError));
					}
					else
					{
						bKCCBCmdsWaiting = IMG_TRUE;
					}
				}
			}

			psDevInfo->bKCCBCmdsWaitingLastTime     = bKCCBCmdsWaiting;
			psDevInfo->ui32KCCBCmdsExecutedLastTime = ui32KCCBCmdsExecuted;
		}
	}

	/*
	   Interrupt counts check...
	*/
	if (bCheckAfterTimePassed  &&  psFwOsData != NULL)
	{
		IMG_UINT32  ui32LISRCount   = 0;
		IMG_UINT32  ui32FWCount     = 0;
		IMG_UINT32  ui32MissingInts = 0;

		/* Add up the total number of interrupts issued, sampled/received and missed... */
#if defined(RGX_FW_IRQ_OS_COUNTERS)
		/* Only the Host OS has a sample count, so only one counter to check. */
		ui32LISRCount += psDevInfo->aui32SampleIRQCount[RGXFW_HOST_DRIVER_ID];
		ui32FWCount   += OSReadHWReg32(psDevInfo->pvRegsBaseKM, gaui32FwOsIrqCntRegAddr[RGXFW_HOST_DRIVER_ID]);
#else
		IMG_UINT32  ui32Index;

		for (ui32Index = 0;  ui32Index < RGXFW_THREAD_NUM;  ui32Index++)
		{
			ui32LISRCount += psDevInfo->aui32SampleIRQCount[ui32Index];
			ui32FWCount   += psFwOsData->aui32InterruptCount[ui32Index];
		}
#endif /* RGX_FW_IRQ_OS_COUNTERS */

		if (ui32LISRCount < ui32FWCount)
		{
			ui32MissingInts = (ui32FWCount-ui32LISRCount);
		}

		if (ui32LISRCount == psDevInfo->ui32InterruptCountLastTime  &&
		    ui32MissingInts >= psDevInfo->ui32MissingInterruptsLastTime  &&
		    psDevInfo->ui32MissingInterruptsLastTime > 1)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: LISR has not received the last %d interrupts",
					__func__, ui32MissingInts));
			eNewStatus = PVRSRV_DEVICE_HEALTH_STATUS_NOT_RESPONDING;
			eNewReason = PVRSRV_DEVICE_HEALTH_REASON_MISSING_INTERRUPTS;

			/* Schedule the MISRs to help mitigate the problems of missing interrupts. */
			OSScheduleMISR(psDevInfo->pvMISRData);
			if (psDevInfo->pvAPMISRData != NULL)
			{
				OSScheduleMISR(psDevInfo->pvAPMISRData);
			}
		}
		psDevInfo->ui32InterruptCountLastTime    = ui32LISRCount;
		psDevInfo->ui32MissingInterruptsLastTime = ui32MissingInts;
	}

	/* Release power lock before RGXCheckForStalledClientContexts */
	PVRSRVPowerUnlock(psDevNode);

	/*
	   Stalled CCB check...
	*/
	if (bCheckAfterTimePassed && (PVRSRV_DEVICE_HEALTH_STATUS_OK==eNewStatus))
	{
		RGXCheckForStalledClientContexts(psDevInfo, IMG_FALSE);
	}

	/* Notify client driver and system layer of any eNewStatus errors */
	if (eNewStatus > PVRSRV_DEVICE_HEALTH_STATUS_OK)
	{
		/* Client notification of device error will be achieved by
		 * clients calling UM function RGXGetLastDeviceError() */
		psDevInfo->eLastDeviceError = RGX_CONTEXT_RESET_REASON_HOST_WDG_FW_ERR;

		/* Notify system layer */
		{
			PVRSRV_DEVICE_NODE *psDevNode = psDevInfo->psDeviceNode;
			PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;

			if (psDevConfig->pfnSysDevErrorNotify)
			{
				PVRSRV_ROBUSTNESS_NOTIFY_DATA sErrorData = {0};

				sErrorData.eResetReason = RGX_CONTEXT_RESET_REASON_HOST_WDG_FW_ERR;
				sErrorData.uErrData.sHostWdgData.ui32Status = (IMG_UINT32)eNewStatus;
				sErrorData.uErrData.sHostWdgData.ui32Reason = (IMG_UINT32)eNewReason;

				psDevConfig->pfnSysDevErrorNotify(psDevConfig,
				                                  &sErrorData);
			}
		}
	}

	/*
	   Finished, save the new status...
	*/
_RGXUpdateHealthStatus_Exit:
	OSAtomicWrite(&psDevNode->eHealthStatus, eNewStatus);
	OSAtomicWrite(&psDevNode->eHealthReason, eNewReason);
	RGXSRV_HWPERF_DEVICE_INFO_HEALTH(psDevInfo, eNewStatus, eNewReason);

	/*
	 * Attempt to service the HWPerf buffer to regularly transport idle/periodic
	 * packets to host buffer.
	 */
	if (psDevNode->pfnServiceHWPerf != NULL)
	{
		PVRSRV_ERROR eError = psDevNode->pfnServiceHWPerf(psDevNode);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: "
			         "Error occurred when servicing HWPerf buffer (%d)",
			         __func__, eError));
		}
	}

	/* Attempt to refresh timer correlation data */
	RGXTimeCorrRestartPeriodic(psDevNode);

	return PVRSRV_OK;
} /* RGXUpdateHealthStatus */

#if defined(SUPPORT_AUTOVZ)
void RGXUpdateAutoVzWdgToken(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
	KM_CONNECTION_CACHEOP(Os, INVALIDATE);
	if (likely(KM_FW_CONNECTION_IS(ACTIVE, psDevInfo) &&
		(KM_OS_CONNECTION_IS(ACTIVE, psDevInfo) || KM_OS_CONNECTION_IS(READY, psDevInfo))))
	{
		/* read and write back the alive token value to confirm to the
		 * virtualisation watchdog that this connection is healthy */
		KM_SET_OS_ALIVE_TOKEN(KM_GET_FW_ALIVE_TOKEN(psDevInfo), psDevInfo);
		KM_ALIVE_TOKEN_CACHEOP(Os, FLUSH);
	}
}

/*
	RGXUpdateAutoVzWatchdog
*/
void RGXUpdateAutoVzWatchdog(PVRSRV_DEVICE_NODE* psDevNode)
{
	if (likely(psDevNode != NULL))
	{
		PVRSRV_RGXDEV_INFO *psDevInfo = psDevNode->pvDevice;

		if (unlikely((psDevInfo  == NULL || !psDevInfo->bFirmwareInitialised || !psDevInfo->bRGXPowered ||
					  psDevInfo->pvRegsBaseKM == NULL || psDevNode->eDevState == PVRSRV_DEVICE_STATE_DEINIT ||
					  psDevNode->eDevState == PVRSRV_DEVICE_STATE_DESTRUCTING)))
		{
			/* If the firmware is not initialised, stop here */
			return;
		}
		else
		{
			PVRSRV_ERROR eError = PVRSRVPowerLock(psDevNode);
			PVR_LOG_RETURN_VOID_IF_ERROR(eError, "PVRSRVPowerLock");

			RGXUpdateAutoVzWdgToken(psDevInfo);
			PVRSRVPowerUnlock(psDevNode);
		}
	}
}

PVRSRV_ERROR RGXDisconnectAllGuests(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	IMG_UINT32 ui32DriverID;

	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfFwSysData->asOsRuntimeFlagsMirror,
	                           INVALIDATE);

	for (ui32DriverID = RGXFW_GUEST_DRIVER_ID_START;
	     ui32DriverID < RGX_NUM_DRIVERS_SUPPORTED;
	     ui32DriverID++)
	{
		PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
		RGXFWIF_CONNECTION_FW_STATE eGuestState = (RGXFWIF_CONNECTION_FW_STATE)
			psDevInfo->psRGXFWIfFwSysData->asOsRuntimeFlagsMirror[ui32DriverID].bfOsState;

		if (eGuestState == RGXFW_CONNECTION_FW_ACTIVE ||
		    eGuestState == RGXFW_CONNECTION_FW_READY)
		{
			PVRSRV_ERROR eError = RGXFWSetFwOsState(psDevInfo, ui32DriverID, RGXFWIF_OS_OFFLINE);
			PVR_LOG_RETURN_IF_ERROR(eError, "RGXFWSetFwOsState");
		}
	}

	return PVRSRV_OK;
}
#endif /* SUPPORT_AUTOVZ */

PVRSRV_ERROR AttachKickResourcesCleanupCtls(PRGXFWIF_CLEANUP_CTL *apsCleanupCtl,
									IMG_UINT32 *pui32NumCleanupCtl,
									RGXFWIF_DM eDM,
									IMG_BOOL bKick,
									RGX_KM_HW_RT_DATASET           *psKMHWRTDataSet,
									RGX_ZSBUFFER_DATA              *psZSBuffer,
									RGX_ZSBUFFER_DATA              *psMSAAScratchBuffer)
{
	PVRSRV_ERROR eError;
	PRGXFWIF_CLEANUP_CTL *psCleanupCtlWrite = apsCleanupCtl;

	PVR_ASSERT((eDM == RGXFWIF_DM_GEOM) || (eDM == RGXFWIF_DM_3D));
	PVR_RETURN_IF_INVALID_PARAM((eDM == RGXFWIF_DM_GEOM) || (eDM == RGXFWIF_DM_3D));

	if (bKick)
	{
		if (psKMHWRTDataSet)
		{
			PRGXFWIF_CLEANUP_CTL psCleanupCtl;

			eError = RGXSetFirmwareAddress(&psCleanupCtl, psKMHWRTDataSet->psHWRTDataFwMemDesc,
					offsetof(RGXFWIF_HWRTDATA, sCleanupState),
					RFW_FWADDR_NOREF_FLAG);
			PVR_RETURN_IF_ERROR(eError);

			*(psCleanupCtlWrite++) = psCleanupCtl;
		}

		if (eDM == RGXFWIF_DM_3D)
		{
			RGXFWIF_PRBUFFER_TYPE eBufferType;
			RGX_ZSBUFFER_DATA *psBuffer = NULL;

			for (eBufferType = RGXFWIF_PRBUFFER_START; eBufferType < RGXFWIF_PRBUFFER_MAXSUPPORTED; eBufferType++)
			{
				switch (eBufferType)
				{
				case RGXFWIF_PRBUFFER_ZSBUFFER:
					psBuffer = psZSBuffer;
					break;
				case RGXFWIF_PRBUFFER_MSAABUFFER:
					psBuffer = psMSAAScratchBuffer;
					break;
				case RGXFWIF_PRBUFFER_MAXSUPPORTED:
					psBuffer = NULL;
					break;
				}
				if (psBuffer)
				{
					(psCleanupCtlWrite++)->ui32Addr = psBuffer->sZSBufferFWDevVAddr.ui32Addr +
									offsetof(RGXFWIF_PRBUFFER, sCleanupState);
					psBuffer = NULL;
				}
			}
		}
	}

	*pui32NumCleanupCtl = psCleanupCtlWrite - apsCleanupCtl;
	PVR_ASSERT(*pui32NumCleanupCtl <= RGXFWIF_KCCB_CMD_KICK_DATA_MAX_NUM_CLEANUP_CTLS);

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXResetHWRLogs(PVRSRV_DEVICE_NODE *psDevNode)
{
	PVRSRV_RGXDEV_INFO       *psDevInfo;
	RGXFWIF_HWRINFOBUF       *psHWRInfoBuf;
	IMG_UINT32               i;

	if (psDevNode->pvDevice == NULL)
	{
		return PVRSRV_ERROR_INVALID_DEVINFO;
	}
	psDevInfo = psDevNode->pvDevice;

	psHWRInfoBuf = psDevInfo->psRGXFWIfHWRInfoBufCtl;

	for (i = 0 ; i < RGXFWIF_DM_MAX ; i++)
	{
		/* Reset the HWR numbers */
		psHWRInfoBuf->aui32HwrDmLockedUpCount[i] = 0;
		psHWRInfoBuf->aui32HwrDmFalseDetectCount[i] = 0;
		psHWRInfoBuf->aui32HwrDmRecoveredCount[i] = 0;
		psHWRInfoBuf->aui32HwrDmOverranCount[i] = 0;
	}

	for (i = 0 ; i < RGXFWIF_HWINFO_MAX ; i++)
	{
		psHWRInfoBuf->sHWRInfo[i].ui32HWRNumber = 0;
	}

	psHWRInfoBuf->ui32WriteIndex = 0;
	psHWRInfoBuf->ui32DDReqCount = 0;

	OSWriteMemoryBarrier(&psHWRInfoBuf->ui32DDReqCount);

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXGetPhyAddr(PMR *psPMR,
						   IMG_DEV_PHYADDR *psPhyAddr,
						   IMG_UINT32 ui32LogicalOffset,
						   IMG_UINT32 ui32Log2PageSize,
						   IMG_UINT32 ui32NumOfPages,
						   IMG_BOOL *bValid)
{

	PVRSRV_ERROR eError;

	eError = PMRLockSysPhysAddresses(psPMR);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: PMRLockSysPhysAddresses failed (%u)",
		         __func__,
		         eError));
		return eError;
	}

	eError = PMR_DevPhysAddr(psPMR,
								 ui32Log2PageSize,
								 ui32NumOfPages,
								 ui32LogicalOffset,
								 psPhyAddr,
								 bValid,
								 DEVICE_USE);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: PMR_DevPhysAddr failed (%u)",
		         __func__,
		         eError));
		return eError;
	}


	eError = PMRUnlockSysPhysAddresses(psPMR);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: PMRUnLockSysPhysAddresses failed (%u)",
		         __func__,
		         eError));
		return eError;
	}

	return eError;
}

#if defined(PDUMP)
PVRSRV_ERROR RGXPdumpDrainKCCB(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32WriteOffset)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVINFO, psDevInfo, PVRSRV_OK);

	if (psDevInfo->bDumpedKCCBCtlAlready)
	{
		/* exiting capture range or pdump block */
		psDevInfo->bDumpedKCCBCtlAlready = IMG_FALSE;

		/* make sure previous cmd is drained in pdump in case we will 'jump' over some future cmds */
		PDUMPCOMMENTWITHFLAGS(psDevInfo->psDeviceNode,
                              PDUMP_FLAGS_CONTINUOUS | PDUMP_FLAGS_POWER,
                              "kCCB(%p): Draining rgxfw_roff (0x%x) == woff (0x%x)",
                              psDevInfo->psKernelCCBCtl,
                              ui32WriteOffset,
                              ui32WriteOffset);
		eError = DevmemPDumpDevmemPol32(psDevInfo->psKernelCCBCtlMemDesc,
				offsetof(RGXFWIF_CCB_CTL, ui32ReadOffset),
				ui32WriteOffset,
				0xffffffff,
				PDUMP_POLL_OPERATOR_EQUAL,
				PDUMP_FLAGS_CONTINUOUS | PDUMP_FLAGS_POWER);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: problem pdumping POL for kCCBCtl (%d)", __func__, eError));
		}
	}

	return eError;

}
#endif

/*!
*******************************************************************************

 @Function	RGXClientConnectCompatCheck_ClientAgainstFW

 @Description

 Check compatibility of client and firmware (build options)
 at the connection time.

 @Input psDeviceNode - device node
 @Input ui32ClientBuildOptions - build options for the client

 @Return   PVRSRV_ERROR - depending on mismatch found

******************************************************************************/
PVRSRV_ERROR RGXClientConnectCompatCheck_ClientAgainstFW(PVRSRV_DEVICE_NODE *psDeviceNode, IMG_UINT32 ui32ClientBuildOptions)
{
	IMG_UINT32		ui32BuildOptionsMismatch;
	IMG_UINT32		ui32BuildOptionsFW;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	RGX_FW_INFO_HEADER *psFWInfoHeader;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

	if (psDevInfo == NULL || psDevInfo->psRGXFWIfOsInitMemDesc == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Cannot acquire kernel fw compatibility check info, RGXFWIF_OSINIT structure not allocated.",
		         __func__));
		return PVRSRV_ERROR_NOT_INITIALISED;
	}

	psFWInfoHeader = &psDevInfo->sFWInfoHeader;

#if !defined(NO_HARDWARE)
	if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
	{
		LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
		{
			RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfOsInit->sRGXCompChecks.bUpdated,
		                               INVALIDATE);
			if (*((volatile IMG_BOOL *) &psDevInfo->psRGXFWIfOsInit->sRGXCompChecks.bUpdated))
			{
				/* No need to wait if the FW has already updated the values */
				break;
			}
			OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
		} END_LOOP_UNTIL_TIMEOUT_US();
	}
#endif

	ui32BuildOptionsFW = psFWInfoHeader->ui32Flags;
	ui32BuildOptionsMismatch = ui32ClientBuildOptions ^ ui32BuildOptionsFW;

	if (ui32BuildOptionsMismatch != 0)
	{
		if ((ui32ClientBuildOptions & ui32BuildOptionsMismatch) != 0)
		{
			PVR_LOG(("(FAIL) RGXDevInitCompatCheck: Mismatch in Firmware and client build options; "
					 "extra options present in client: (0x%x). Please check rgx_options.h",
					 ui32ClientBuildOptions & ui32BuildOptionsMismatch ));
		}

		if ((ui32BuildOptionsFW & ui32BuildOptionsMismatch) != 0)
		{
			PVR_LOG(("(FAIL) RGXDevInitCompatCheck: Mismatch in Firmware and client build options; "
					 "extra options present in Firmware: (0x%x). Please check rgx_options.h",
					 ui32BuildOptionsFW & ui32BuildOptionsMismatch ));
		}

		return PVRSRV_ERROR_BUILD_OPTIONS_MISMATCH;
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: Firmware and client build options match. [ OK ]", __func__));
	}

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	RGXFwRawHeapAllocMap

 @Description Register firmware heap for the specified driver

 @Input psDeviceNode - device node
 @Input ui32DriverID - Guest driver
 @Input sDevPAddr    - Heap address
 @Input ui64DevPSize - Heap size

 @Return   PVRSRV_ERROR - PVRSRV_OK if heap setup was successful.

******************************************************************************/
PVRSRV_ERROR RGXFwRawHeapAllocMap(PVRSRV_DEVICE_NODE *psDeviceNode,
								  IMG_UINT32 ui32DriverID,
								  IMG_DEV_PHYADDR sDevPAddr,
								  IMG_UINT64 ui64DevPSize)
{
	PVRSRV_ERROR eError;
	IMG_CHAR szRegionRAName[RA_MAX_NAME_LENGTH];
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	PVRSRV_MEMALLOCFLAGS_T uiRawFwHeapAllocFlags = (RGX_FWSHAREDMEM_GPU_ONLY_ALLOCFLAGS |
													PVRSRV_MEMALLOCFLAG_PHYS_HEAP_HINT(FW_PREMAP0 + ui32DriverID));
	PHYS_HEAP_CONFIG *psFwHeapConfig = PVRSRVFindPhysHeapConfig(psDeviceNode->psDevConfig,
																PHYS_HEAP_USAGE_FW_SHARED);
	PHYS_HEAP_CONFIG sFwHeapConfig;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

	if (psFwHeapConfig == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "FW_MAIN heap config not found."));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	OSSNPrintf(szRegionRAName, sizeof(szRegionRAName), RGX_FIRMWARE_GUEST_RAW_HEAP_IDENT, ui32DriverID);

	if (!ui64DevPSize ||
		!sDevPAddr.uiAddr ||
		ui32DriverID >= RGX_NUM_DRIVERS_SUPPORTED ||
		ui64DevPSize != RGX_FIRMWARE_RAW_HEAP_SIZE)
	{
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameters for %s", szRegionRAName));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	sFwHeapConfig = *psFwHeapConfig;
	sFwHeapConfig.eType = PHYS_HEAP_TYPE_LMA;
	sFwHeapConfig.ui32UsageFlags = PHYS_HEAP_USAGE_FW_PREMAP;
	sFwHeapConfig.uConfig.sLMA.sStartAddr.uiAddr = 0;
	sFwHeapConfig.uConfig.sLMA.sCardBase.uiAddr = sDevPAddr.uiAddr;
	sFwHeapConfig.uConfig.sLMA.uiSize = RGX_FIRMWARE_RAW_HEAP_SIZE;

	eError = PhysmemCreateHeapLMA(psDeviceNode,
	                              RGXPhysHeapGetLMAPolicy(sFwHeapConfig.ui32UsageFlags, psDeviceNode),
	                              &sFwHeapConfig,
	                              szRegionRAName,
	                              &psDeviceNode->apsFWPremapPhysHeap[ui32DriverID]);
	PVR_LOG_RETURN_IF_ERROR_VA(eError, "PhysmemCreateHeapLMA:PREMAP [%d]", ui32DriverID);

	eError = PhysHeapAcquire(psDeviceNode->apsFWPremapPhysHeap[ui32DriverID]);
	PVR_LOG_RETURN_IF_ERROR_VA(eError, "PhysHeapAcquire:PREMAP [%d]", ui32DriverID);

	psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_FW_PREMAP0 + ui32DriverID] = psDeviceNode->apsFWPremapPhysHeap[ui32DriverID];

	PDUMPCOMMENT(psDeviceNode, "Allocate and map raw firmware heap for DriverID: [%d]", ui32DriverID);

#if (RGX_NUM_DRIVERS_SUPPORTED > 1)
	/* don't clear the heap of other guests on allocation */
	uiRawFwHeapAllocFlags &= (ui32DriverID > RGXFW_HOST_DRIVER_ID) ? (~PVRSRV_MEMALLOCFLAG_ZERO_ON_ALLOC) : (~0ULL);
#endif

	/* if the firmware is already powered up, consider the firmware heaps are pre-mapped. */
	if (psDeviceNode->bAutoVzFwIsUp)
	{
		uiRawFwHeapAllocFlags &= RGX_AUTOVZ_KEEP_FW_DATA_MASK(psDeviceNode->bAutoVzFwIsUp);
		DevmemHeapSetPremapStatus(psDevInfo->psPremappedFwRawHeap[ui32DriverID], IMG_TRUE);
	}

#if defined(SUPPORT_TRUSTED_DEVICE) && defined(RGX_PREMAP_FW_HEAPS)
	PVR_DPF((PVR_DBG_MESSAGE, "%s: Allocation and mapping for Firmware heaps done by TEE.", __func__));
#else
	eError = DevmemFwAllocate(psDevInfo,
							  RGX_FIRMWARE_RAW_HEAP_SIZE,
							  uiRawFwHeapAllocFlags,
							  psDevInfo->psPremappedFwRawHeap[ui32DriverID]->pszName,
							  &psDevInfo->psPremappedFwRawMemDesc[ui32DriverID]);
	PVR_LOG_RETURN_IF_ERROR(eError, "DevmemFwAllocate");
#endif

	/* Mark this devmem heap as premapped so allocations will not require device mapping. */
	DevmemHeapSetPremapStatus(psDevInfo->psPremappedFwRawHeap[ui32DriverID], IMG_TRUE);

	if (ui32DriverID == RGXFW_HOST_DRIVER_ID)
	{
		/* if the Host's raw fw heap is premapped, mark its main & config sub-heaps accordingly
		 * No memory allocated from these sub-heaps will be individually mapped into the device's
		 * address space so they can remain marked permanently as premapped. */
		DevmemHeapSetPremapStatus(psDevInfo->psFirmwareMainHeap, IMG_TRUE);
		DevmemHeapSetPremapStatus(psDevInfo->psFirmwareConfigHeap, IMG_TRUE);
	}

	return eError;
}

/*!
*******************************************************************************

 @Function	RGXFwRawHeapUnmapFree

 @Description Unregister firmware heap for the specified guest driver

 @Input psDeviceNode - device node
 @Input ui32DriverID

******************************************************************************/
void RGXFwRawHeapUnmapFree(PVRSRV_DEVICE_NODE *psDeviceNode,
						   IMG_UINT32 ui32DriverID)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	/* remove the premap status, so the heap can be unmapped and freed */
	if (psDevInfo->psPremappedFwRawHeap[ui32DriverID])
	{
		DevmemHeapSetPremapStatus(psDevInfo->psPremappedFwRawHeap[ui32DriverID], IMG_FALSE);
	}

	if (psDevInfo->psPremappedFwRawMemDesc[ui32DriverID])
	{
		DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psPremappedFwRawMemDesc[ui32DriverID]);
		psDevInfo->psPremappedFwRawMemDesc[ui32DriverID] = NULL;
	}
}

/*
	RGXReadMETAAddr
*/
static PVRSRV_ERROR RGXReadMETAAddr(PVRSRV_RGXDEV_INFO	*psDevInfo, IMG_UINT32 ui32METAAddr, IMG_UINT32 *pui32Value)
{
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	void __iomem  *pvRegBase = psDevInfo->pvSecureRegsBaseKM;
	IMG_UINT8 __iomem  *pui8RegBase = pvRegBase;
	IMG_UINT32 ui32PollValue;
	IMG_UINT32 ui32PollMask;
	IMG_UINT32 ui32PollRegOffset;
	IMG_UINT32 ui32ReadOffset;
	IMG_UINT32 ui32WriteOffset;
	IMG_UINT32 ui32WriteValue;

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, META_REGISTER_UNPACKED_ACCESSES))
	{
		if (RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) > 1)
		{
			ui32PollValue = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__READY_EN
							| RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__GBLPORT_IDLE_EN;
			ui32PollMask = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__READY_EN
							| RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__GBLPORT_IDLE_EN;
			ui32PollRegOffset = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA;
			ui32WriteOffset = RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_GT1_AND_MRUA;
			ui32WriteValue = ui32METAAddr | RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_GT1_AND_MRUA__RD_EN;
			CHECK_HWBRN_68777(ui32WriteValue);
			ui32ReadOffset = RGX_CR_META_SP_MSLVDATAX__HOST_SECURITY_GT1_AND_MRUA;
		}
		else
		{
			ui32PollValue = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__READY_EN
							| RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__GBLPORT_IDLE_EN;
			ui32PollMask = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__READY_EN
							| RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__GBLPORT_IDLE_EN;
			ui32PollRegOffset = RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA;
			ui32WriteOffset = RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_EQ1_AND_MRUA;
			ui32WriteValue = ui32METAAddr | RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_EQ1_AND_MRUA__RD_EN;
			CHECK_HWBRN_68777(ui32WriteValue);
			ui32ReadOffset = RGX_CR_META_SP_MSLVDATAX__HOST_SECURITY_EQ1_AND_MRUA;
		}
	}
	else
	{
		ui32PollValue = RGX_CR_META_SP_MSLVCTRL1_READY_EN | RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN;
		ui32PollMask = RGX_CR_META_SP_MSLVCTRL1_READY_EN | RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN;
		ui32PollRegOffset = RGX_CR_META_SP_MSLVCTRL1;
		ui32WriteOffset = RGX_CR_META_SP_MSLVCTRL0;
		ui32WriteValue = ui32METAAddr | RGX_CR_META_SP_MSLVCTRL0_RD_EN;
		ui32ReadOffset = RGX_CR_META_SP_MSLVDATAX;
	}

	/* Wait for Slave Port to be Ready */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
			(IMG_UINT32 __iomem *) (pui8RegBase + ui32PollRegOffset),
	        ui32PollValue,
	        ui32PollMask,
	        POLL_FLAG_LOG_ERROR,
	        NULL) != PVRSRV_OK)
	{
		return PVRSRV_ERROR_TIMEOUT;
	}

	/* Issue the Read */
	OSWriteUncheckedHWReg32(pvRegBase, ui32WriteOffset, ui32WriteValue);
	(void)OSReadUncheckedHWReg32(pvRegBase, ui32WriteOffset);

	/* Wait for Slave Port to be Ready: read complete */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
			(IMG_UINT32 __iomem *) (pui8RegBase + ui32PollRegOffset),
	        ui32PollValue,
	        ui32PollMask,
	        POLL_FLAG_LOG_ERROR,
	        NULL) != PVRSRV_OK)
	{
		return PVRSRV_ERROR_TIMEOUT;
	}

	/* Read the value */
	*pui32Value = OSReadUncheckedHWReg32(pvRegBase, ui32ReadOffset);
#else
	IMG_UINT8 __iomem  *pui8RegBase = psDevInfo->pvRegsBaseKM;

	/* Wait for Slave Port to be Ready */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
			(IMG_UINT32 __iomem *) (pui8RegBase + RGX_CR_META_SP_MSLVCTRL1),
			RGX_CR_META_SP_MSLVCTRL1_READY_EN|RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN,
			RGX_CR_META_SP_MSLVCTRL1_READY_EN|RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN,
			POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		return PVRSRV_ERROR_TIMEOUT;
	}

	/* Issue the Read */
	OSWriteHWReg32(
			psDevInfo->pvRegsBaseKM,
			RGX_CR_META_SP_MSLVCTRL0,
			ui32METAAddr | RGX_CR_META_SP_MSLVCTRL0_RD_EN);
	(void) OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_META_SP_MSLVCTRL0);

	/* Wait for Slave Port to be Ready: read complete */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
			(IMG_UINT32 __iomem *) (pui8RegBase + RGX_CR_META_SP_MSLVCTRL1),
			RGX_CR_META_SP_MSLVCTRL1_READY_EN|RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN,
			RGX_CR_META_SP_MSLVCTRL1_READY_EN|RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN,
			POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		return PVRSRV_ERROR_TIMEOUT;
	}

	/* Read the value */
	*pui32Value = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_META_SP_MSLVDATAX);
#endif

	return PVRSRV_OK;
}

/*
	RGXWriteMETAAddr
*/
static PVRSRV_ERROR RGXWriteMETAAddr(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32METAAddr, IMG_UINT32 ui32Value)
{
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	void __iomem *pvRegBase = psDevInfo->pvSecureRegsBaseKM;
	IMG_UINT8 __iomem *pui8RegBase = pvRegBase;

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, META_REGISTER_UNPACKED_ACCESSES))
	{
		if (RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) > 1)
		{
			/* Wait for Slave Port to be Ready */
			if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
					(IMG_UINT32 __iomem *)(pui8RegBase + RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA),
					RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__READY_EN
					| RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__GBLPORT_IDLE_EN,
					RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__READY_EN
					| RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_GT1_AND_MRUA__GBLPORT_IDLE_EN,
					POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
			{
				return PVRSRV_ERROR_TIMEOUT;
			}

			/* Issue the Write */
			CHECK_HWBRN_68777(ui32METAAddr);
			OSWriteUncheckedHWReg32(pvRegBase, RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_GT1_AND_MRUA, ui32METAAddr);
			OSWriteUncheckedHWReg32(pvRegBase, RGX_CR_META_SP_MSLVDATAT__HOST_SECURITY_GT1_AND_MRUA, ui32Value);
		}
		else
		{
			/* Wait for Slave Port to be Ready */
			if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
					(IMG_UINT32 __iomem *)(pui8RegBase + RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA),
					RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__READY_EN
					| RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__GBLPORT_IDLE_EN,
					RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__READY_EN
					| RGX_CR_META_SP_MSLVCTRL1__HOST_SECURITY_EQ1_AND_MRUA__GBLPORT_IDLE_EN,
					POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
			{
				return PVRSRV_ERROR_TIMEOUT;
			}

			/* Issue the Write */
			CHECK_HWBRN_68777(ui32METAAddr);
			OSWriteUncheckedHWReg32(pvRegBase, RGX_CR_META_SP_MSLVCTRL0__HOST_SECURITY_EQ1_AND_MRUA, ui32METAAddr);
			OSWriteUncheckedHWReg32(pvRegBase, RGX_CR_META_SP_MSLVDATAT__HOST_SECURITY_EQ1_AND_MRUA, ui32Value);
		}
	}
	else
	{
		/* Wait for Slave Port to be Ready */
		if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
				(IMG_UINT32 __iomem *)(pui8RegBase + RGX_CR_META_SP_MSLVCTRL1),
				RGX_CR_META_SP_MSLVCTRL1_READY_EN|RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN,
				RGX_CR_META_SP_MSLVCTRL1_READY_EN|RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN,
				POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
		{
			return PVRSRV_ERROR_TIMEOUT;
		}

		/* Issue the Write */
		OSWriteUncheckedHWReg32(pvRegBase, RGX_CR_META_SP_MSLVCTRL0, ui32METAAddr);
		(void) OSReadUncheckedHWReg32(pvRegBase, RGX_CR_META_SP_MSLVCTRL0); /* Fence write */
		OSWriteUncheckedHWReg32(pvRegBase, RGX_CR_META_SP_MSLVDATAT, ui32Value);
		(void) OSReadUncheckedHWReg32(pvRegBase, RGX_CR_META_SP_MSLVDATAT); /* Fence write */
	}
#else
	IMG_UINT8 __iomem *pui8RegBase = psDevInfo->pvRegsBaseKM;

	/* Wait for Slave Port to be Ready */
	if (PVRSRVPollForValueKM(psDevInfo->psDeviceNode,
			(IMG_UINT32 __iomem *)(pui8RegBase + RGX_CR_META_SP_MSLVCTRL1),
			RGX_CR_META_SP_MSLVCTRL1_READY_EN|RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN,
			RGX_CR_META_SP_MSLVCTRL1_READY_EN|RGX_CR_META_SP_MSLVCTRL1_GBLPORT_IDLE_EN,
			POLL_FLAG_LOG_ERROR, NULL) != PVRSRV_OK)
	{
		return PVRSRV_ERROR_TIMEOUT;
	}

	/* Issue the Write */
	OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_META_SP_MSLVCTRL0, ui32METAAddr);
	OSWriteHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_META_SP_MSLVDATAT, ui32Value);
#endif

	return PVRSRV_OK;
}

PVRSRV_ERROR RGXReadFWModuleAddr(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32FWAddr, IMG_UINT32 *pui32Value)
{
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META))
	{
		return RGXReadMETAAddr(psDevInfo, ui32FWAddr, pui32Value);
	}

#if !defined(EMULATOR)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
	{
		return RGXRiscvReadMem(psDevInfo, ui32FWAddr, pui32Value);
	}
#endif

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

PVRSRV_ERROR RGXWriteFWModuleAddr(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_UINT32 ui32FWAddr, IMG_UINT32 ui32Value)
{
	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META))
	{
		return RGXWriteMETAAddr(psDevInfo, ui32FWAddr, ui32Value);
	}

#if !defined(EMULATOR)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
	{
		return RGXRiscvWriteMem(psDevInfo, ui32FWAddr, ui32Value);
	}
#endif

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

PVRSRV_ERROR RGXGetFwMapping(PVRSRV_RGXDEV_INFO *psDevInfo,
                             IMG_UINT32 ui32FwVA,
                             IMG_CPU_PHYADDR *psCpuPA,
                             IMG_DEV_PHYADDR *psDevPA,
                             IMG_UINT64 *pui64RawPTE)
{
	PVRSRV_ERROR eError       = PVRSRV_OK;
	IMG_CPU_PHYADDR sCpuPA    = {0U};
	IMG_DEV_PHYADDR sDevPA    = {0U};
	IMG_UINT64 ui64RawPTE     = 0U;
	MMU_FAULT_DATA sFaultData = {0U};
	MMU_CONTEXT *psFwMMUCtx   = psDevInfo->psKernelMMUCtx;
	IMG_UINT32 ui32FwHeapBase = (IMG_UINT32) (RGX_FIRMWARE_RAW_HEAP_BASE & UINT_MAX);
	IMG_UINT32 ui32FwHeapEnd  = ui32FwHeapBase + (RGX_NUM_DRIVERS_SUPPORTED * RGX_FIRMWARE_RAW_HEAP_SIZE);
	IMG_UINT32 ui32DriverID   = (ui32FwVA - ui32FwHeapBase) / RGX_FIRMWARE_RAW_HEAP_SIZE;
	IMG_UINT32 ui32HeapId;
	PHYS_HEAP *psPhysHeap;
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	/* MIPS uses the same page size as the OS, while others default to 4K pages */
	IMG_UINT32 ui32FwPageSize = RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS) ?
	                             OSGetPageSize() : BIT(RGX_MMUCTRL_PAGE_4KB_RANGE_SHIFT);
#else
	/* default to 4K pages */
	IMG_UINT32 ui32FwPageSize = BIT(RGX_MMUCTRL_PAGE_4KB_RANGE_SHIFT);
#endif

	IMG_UINT32 ui32PageOffset = (ui32FwVA & (ui32FwPageSize - 1));

	PVR_LOG_GOTO_IF_INVALID_PARAM((ui32DriverID < RGX_NUM_DRIVERS_SUPPORTED),
	                              eError, ErrorExit);

	PVR_LOG_GOTO_IF_INVALID_PARAM(((psCpuPA != NULL) ||
	                               (psDevPA != NULL) ||
	                               (pui64RawPTE != NULL)),
	                              eError, ErrorExit);

	PVR_LOG_GOTO_IF_INVALID_PARAM(((ui32FwVA >= ui32FwHeapBase) &&
	                              (ui32FwVA < ui32FwHeapEnd)),
	                              eError, ErrorExit);

	ui32HeapId = (ui32DriverID == RGXFW_HOST_DRIVER_ID) ?
	              PVRSRV_PHYS_HEAP_FW_MAIN : (PVRSRV_PHYS_HEAP_FW_PREMAP0 + ui32DriverID);
	psPhysHeap = psDevInfo->psDeviceNode->apsPhysHeap[ui32HeapId];

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
	{
		/* MIPS is equipped with a dedicated MMU */
		RGXMipsCheckFaultAddress(psFwMMUCtx, ui32FwVA, &sFaultData);
	}
	else
#endif
	{
		IMG_UINT64 ui64FwDataBaseMask;
		IMG_DEV_VIRTADDR sDevVAddr;

		if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META))
		{
			ui64FwDataBaseMask = ~(RGXFW_SEGMMU_DATA_META_CACHE_MASK |
			                         RGXFW_SEGMMU_DATA_VIVT_SLC_CACHE_MASK |
			                         RGXFW_SEGMMU_DATA_BASE_ADDRESS);
		}
#if !defined(EMULATOR)
		else if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
		{
			ui64FwDataBaseMask = ~RGXRISCVFW_REGION_MASK;
		}
#endif
		else
		{
			PVR_LOG_GOTO_WITH_ERROR("RGXGetFwMapping", eError, PVRSRV_ERROR_NOT_IMPLEMENTED, ErrorExit);
		}

		sDevVAddr.uiAddr = (ui32FwVA & ui64FwDataBaseMask) | RGX_FIRMWARE_RAW_HEAP_BASE;

		/* Fw CPU shares a subset of the GPU's VA space */
		MMU_CheckFaultAddress(psFwMMUCtx, &sDevVAddr, &sFaultData);
	}

	ui64RawPTE = sFaultData.sLevelData[MMU_LEVEL_1].ui64Address;

	if (eError == PVRSRV_OK)
	{
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
		IMG_BOOL bValidPage = (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS)) ?
		                       BITMASK_HAS(ui64RawPTE, RGXMIPSFW_TLB_VALID) :
		                       BITMASK_HAS(ui64RawPTE, RGX_MMUCTRL_PT_DATA_VALID_EN);
#else
		IMG_BOOL bValidPage = BITMASK_HAS(ui64RawPTE, RGX_MMUCTRL_PT_DATA_VALID_EN);
#endif

		if (!bValidPage)
		{
			/* don't report invalid pages */
			eError = PVRSRV_ERROR_DEVICEMEM_NO_MAPPING;
		}
		else
		{
#if defined(RGX_FEATURE_MIPS_BIT_MASK)
			sDevPA.uiAddr = ui32PageOffset + ((RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS)) ?
			                RGXMIPSFW_TLB_GET_PA(ui64RawPTE) :
			                (ui64RawPTE & ~RGX_MMUCTRL_PT_DATA_PAGE_CLRMSK));
#else
			sDevPA.uiAddr = ui32PageOffset + (ui64RawPTE & ~RGX_MMUCTRL_PT_DATA_PAGE_CLRMSK);
#endif

			/* Only the Host's Firmware heap is present in the Host's CPU IPA space */
			if (ui32DriverID == RGXFW_HOST_DRIVER_ID)
			{
				PhysHeapDevPAddrToCpuPAddr(psPhysHeap, 1, &sCpuPA, &sDevPA);
			}
			else
			{
				sCpuPA.uiAddr = 0U;
			}
		}
	}

	if (psCpuPA != NULL)
	{
		*psCpuPA = sCpuPA;
	}

	if (psDevPA != NULL)
	{
		*psDevPA = sDevPA;
	}

	if (pui64RawPTE != NULL)
	{
		*pui64RawPTE = ui64RawPTE;
	}

ErrorExit:
	return eError;
}

PVRSRV_ERROR
RGXFWSetVzConnectionCooldownPeriod(PVRSRV_RGXDEV_INFO *psDevInfo,
				   IMG_UINT32 ui32VzConnectionCooldownPeriodInSec)
{
	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVINFO, psDevInfo, PVRSRV_ERROR_NOT_SUPPORTED);

	psDevInfo->psRGXFWIfRuntimeCfg->ui32VzConnectionCooldownPeriodInSec = ui32VzConnectionCooldownPeriodInSec;
	OSWriteMemoryBarrier(&psDevInfo->psRGXFWIfRuntimeCfg->ui32VzConnectionCooldownPeriodInSec);

#if defined(PDUMP)
	PDUMPCOMMENT(psDevInfo->psDeviceNode,
				 "Updating the Vz reconnect request cooldown period inside RGXFWIfRuntimeCfg");
	DevmemPDumpLoadMemValue32(psDevInfo->psRGXFWIfRuntimeCfgMemDesc,
							  offsetof(RGXFWIF_RUNTIME_CFG, ui32VzConnectionCooldownPeriodInSec),
							  ui32VzConnectionCooldownPeriodInSec,
							  PDUMP_FLAGS_CONTINUOUS);
#endif

	return PVRSRV_OK;
}

/******************************************************************************
 End of file (rgxfwutils.c)
******************************************************************************/

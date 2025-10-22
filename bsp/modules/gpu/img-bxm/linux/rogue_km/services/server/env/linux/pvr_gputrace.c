/*************************************************************************/ /*!
@File           pvr_gputrace.c
@Title          PVR GPU Trace module Linux implementation
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

#include <linux/version.h>
#include <linux/fs.h>
#include <linux/trace_events.h>

#include "pvrsrv_error.h"
#include "pvrsrv_apphint.h"
#include "pvr_debug.h"
#include "ospvr_gputrace.h"
#include "rgxhwperf.h"
#include "rgxtimecorr.h"
#include "device.h"
#include "trace_events.h"
#include "pvrsrv.h"
#include "pvrsrv_tlstreams.h"
#include "tlclient.h"
#include "tlstream.h"
#include "pvr_debug.h"
#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
#define CREATE_TRACE_POINTS
#include "rogue_trace_events.h"
#endif

#if defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
#define CREATE_TRACE_POINTS
#include "gpu_work.h"
#define TRACE_FS_CLK "/sys/kernel/tracing/trace_clock"
#else
#define TRACE_FS_CLK "/sys/kernel/debug/tracing/trace_clock"
#endif /* defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD) */

#if defined(SUPPORT_RGX)
#if defined(PVRSRV_FORCE_HWPERF_TO_SCHED_CLK)
#define TRACE_CLK     RGXTIMECORR_CLOCK_SCHED
#define TRACE_CLK_STR "local\n"
#else
#define TRACE_CLK     RGXTIMECORR_CLOCK_MONO
#define TRACE_CLK_STR "mono\n"
#endif
#endif

/******************************************************************************
 Module internal implementation
******************************************************************************/

typedef enum {
	PVR_GPUTRACE_SWITCH_TYPE_UNDEF = 0,

	PVR_GPUTRACE_SWITCH_TYPE_BEGIN = 1,
	PVR_GPUTRACE_SWITCH_TYPE_END = 2,
	PVR_GPUTRACE_SWITCH_TYPE_SINGLE = 3
} PVR_GPUTRACE_SWITCH_TYPE;

typedef struct RGX_HWPERF_FTRACE_DATA {
	/* This lock ensures the HWPerf TL stream reading resources are not destroyed
	 * by one thread disabling it while another is reading from it. Keeps the
	 * state and resource create/destroy atomic and consistent. */
	POS_LOCK    hFTraceResourceLock;

	IMG_HANDLE  hGPUTraceCmdCompleteHandle;
	IMG_HANDLE  hGPUFTraceTLStream;
	IMG_UINT64  ui64LastSampledTimeCorrOSTimeStamp;
	IMG_UINT32  ui32FTraceLastOrdinal;
	IMG_BOOL    bTrackOrdinals;
} RGX_HWPERF_FTRACE_DATA;

/* This lock ensures state change of GPU_TRACING on/off is done atomically */
static POS_LOCK ghGPUTraceStateLock;
static IMG_BOOL gbFTraceGPUEventsEnabled = PVRSRV_APPHINT_ENABLEFTRACEGPU;

#if !defined(PVRSRV_FORCE_HWPERF_TO_SCHED_CLK)
/* This is the FTrace clock source prior to driver initialisation */
static IMG_CHAR gszLastClockSource[32] = {0};
#endif

/* This lock ensures that the reference counting operation on the FTrace UFO
 * events and enable/disable operation on firmware event are performed as
 * one atomic operation. This should ensure that there are no race conditions
 * between reference counting and firmware event state change.
 * See below comment for guiUfoEventRef.
 */
static POS_LOCK ghLockFTraceEventLock;

/* Multiple FTrace UFO events are reflected in the firmware as only one event. When
 * we enable FTrace UFO event we want to also at the same time enable it in
 * the firmware. Since there is a multiple-to-one relation between those events
 * we count how many FTrace UFO events is enabled. If at least one event is
 * enabled we enabled the firmware event. When all FTrace UFO events are disabled
 * we disable firmware event. */
static IMG_UINT guiUfoEventRef;

/******************************************************************************
 Module In-bound API
******************************************************************************/

static PVRSRV_ERROR _GpuTraceDisable(
	PVRSRV_RGXDEV_INFO *psRgxDevInfo,
	IMG_BOOL bDeInit);

static void _GpuTraceCmdCompleteNotify(PVRSRV_CMDCOMP_HANDLE);
static void _GpuTraceProcessPackets(PVRSRV_RGXDEV_INFO *psDevInfo, void *pBuffer,
                                    IMG_UINT32 ui32ReadLen);

static void _FTrace_FWOnReaderOpenCB(void *pvArg)
{
	PVRSRV_RGXDEV_INFO* psRgxDevInfo = (PVRSRV_RGXDEV_INFO*) pvArg;
	psRgxDevInfo->bSuspendHWPerfL2DataCopy[RGX_HWPERF_L2_STREAM_FTRACE] = IMG_FALSE;
}

static void _FTrace_FWOnReaderCloseCB(void *pvArg)
{
	PVRSRV_RGXDEV_INFO* psRgxDevInfo = (PVRSRV_RGXDEV_INFO*) pvArg;
	psRgxDevInfo->bSuspendHWPerfL2DataCopy[RGX_HWPERF_L2_STREAM_FTRACE] = IMG_TRUE;
}

/* Currently supported by default */
#if defined(SUPPORT_TL_PRODUCER_CALLBACK)
static PVRSRV_ERROR GPUTraceTLCB(IMG_HANDLE hStream,
                                  IMG_UINT32 ui32ReqOp, IMG_UINT32* ui32Resp, void* pvUser)
{
	/* IN DEV: Not required as the streams goal is to be a copy of HWPerf */
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_UNREFERENCED_PARAMETER(hStream);
	PVR_UNREFERENCED_PARAMETER(ui32ReqOp);
	PVR_UNREFERENCED_PARAMETER(ui32Resp);
	PVR_UNREFERENCED_PARAMETER(pvUser);

	return eError;
}
#endif /* defined(SUPPORT_TL_PRODUCER_CALLBACK) */

#if defined(SUPPORT_RGX)
#if !defined(PVRSRV_FORCE_HWPERF_TO_SCHED_CLK)
/* Configure the FTrace clock source to use the DDK apphint clock source */
static void PVRGpuTraceInitFTraceClockSource(void)
{
	int ret, i, j;
	bool bFound = false;
	loff_t pos = 0;
	char str[64];

	/*  Just force the value to be what the DDK says it is
	    Note for filp_open, the mode is only used for O_CREAT
		Hence its value doesn't matter in this context
	 */
	struct file *filep = filp_open(TRACE_FS_CLK, O_RDWR, 0);
	PVR_LOG_RETURN_VOID_IF_FALSE(!IS_ERR(filep), "TraceFS not found");

	PVR_LOG_VA(PVR_DBG_MESSAGE,
	        "Writing %s to %s to enable parallel HWPerf and FTrace support",
	        TRACE_CLK_STR, TRACE_FS_CLK);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	ret = kernel_read(filep, str, sizeof(str)-1, &pos);
#else
	ret = kernel_read(filep, pos, str, sizeof(str)-1);
#endif
	PVR_LOG_RETURN_VOID_IF_FALSE((ret > 0), "TraceFS Read failed");
	str[ret] = 0;
	pos = 0;

	/* Determine clock value. trace_clock value is stored like [<clock_string>]
	   File example: [local] global counter mono mono_raw x86-tsc
	*/
	for (i = 0, j = 0; i < sizeof(str) && j < sizeof(gszLastClockSource); i++)
	{
		if (str[i] == ']')
		{
			break;
		}
		else if (str[i] == '[')
		{
			bFound = true;
		}
		else if (bFound)
		{
			gszLastClockSource[j] = str[i];
			j++;
		}
	}
	PVR_LOG_VA(PVR_DBG_MESSAGE, "Got %s from FTraceFS", gszLastClockSource);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	ret = kernel_write(filep, TRACE_CLK_STR, sizeof(TRACE_CLK_STR), &pos);
#else
	ret = kernel_write(filep, TRACE_CLK_STR, sizeof(TRACE_CLK_STR), pos);
#endif
	PVR_LOG_IF_FALSE((ret > 0), "Setting FTrace clock source failed");

	filp_close(filep, NULL);
}

/* Reset the FTrace clock source to the original clock source */
static void PVRGpuTraceDeinitFTraceClockSource(void)
{
	int ret;
	loff_t pos = 0;

	/* Return the original value of the file */
	struct file *filep = filp_open(TRACE_FS_CLK, O_WRONLY, 0);
	PVR_LOG_RETURN_VOID_IF_FALSE(!IS_ERR(filep), "TraceFS not found");

	/* FTraceFS write will ignore any writes to it that don't match a clock source */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	ret = kernel_write(filep, gszLastClockSource, sizeof(gszLastClockSource), &pos);
#else
	ret = kernel_write(filep, gszLastClockSource, sizeof(gszLastClockSource), pos);
#endif
	PVR_LOG_IF_FALSE((ret >= 0), "Setting FTrace clock source failed");

	filp_close(filep, NULL);
}
#endif /* !defined(SUPPORT_ANDROID_PLATFORM) */
#endif /* defined(SUPPORT_RGX) */

PVRSRV_ERROR PVRGpuTraceSupportInit(void)
{
	PVRSRV_ERROR eError;

	if (ghLockFTraceEventLock != NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "FTrace Support is already initialized"));
		return PVRSRV_OK;
	}

	/* common module params initialization */
	eError = OSLockCreate(&ghLockFTraceEventLock);
	PVR_LOG_RETURN_IF_ERROR(eError, "OSLockCreate");

	eError = OSLockCreate(&ghGPUTraceStateLock);
	PVR_LOG_RETURN_IF_ERROR (eError, "OSLockCreate");

#if defined(SUPPORT_RGX)
#if !defined(PVRSRV_FORCE_HWPERF_TO_SCHED_CLK)
	PVRGpuTraceInitFTraceClockSource();
#endif /* !defined(PVRSRV_FORCE_HWPERF_TO_SCHED_CLK) */
#endif /* defined(SUPPORT_RGX) */

	return PVRSRV_OK;
}

void PVRGpuTraceSupportDeInit(void)
{
#if defined(SUPPORT_RGX)
#if !defined(PVRSRV_FORCE_HWPERF_TO_SCHED_CLK)
	PVRGpuTraceDeinitFTraceClockSource();
#endif /* !defined(PVRSRV_FORCE_HWPERF_TO_SCHED_CLK) */
#endif /* defined(SUPPORT_RGX) */

	if (ghGPUTraceStateLock)
	{
		OSLockDestroy(ghGPUTraceStateLock);
	}

	if (ghLockFTraceEventLock)
	{
		OSLockDestroy(ghLockFTraceEventLock);
		ghLockFTraceEventLock = NULL;
	}
}

/* Called from RGXHWPerfInitOnDemandL2Stream() which is alway called from
 * a critical section protected by hHWPerfLock. */
PVRSRV_ERROR PVRGpuTraceInitStream(PVRSRV_RGXDEV_INFO *psDevInfo)
{
	PVRSRV_ERROR eError;
	IMG_CHAR pszFTraceStreamName[sizeof(PVRSRV_TL_FTRACE_RGX_FW_STREAM) + 4];
	/* + 4 is used to allow names up to "ftrace_999", which is enough */

	IMG_HANDLE hStream = NULL;

	/* form the FTrace stream name, corresponding to this DevNode; which can make sense in the UM */
	if (OSSNPrintf(pszFTraceStreamName, sizeof(pszFTraceStreamName), "%s%d",
				   PVRSRV_TL_FTRACE_RGX_FW_STREAM,
				   psDevInfo->psDeviceNode->sDevId.i32KernelDeviceID) < 0)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%s: Failed to form FTrace stream name for device %d",
				__func__,
				psDevInfo->psDeviceNode->sDevId.i32KernelDeviceID));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = TLStreamCreate(&hStream,
							pszFTraceStreamName,
							psDevInfo->ui32RGXL2HWPerfBufSize,
							TL_OPMODE_DROP_NEWER | TL_FLAG_NO_SIGNAL_ON_COMMIT,
							_FTrace_FWOnReaderOpenCB, psDevInfo,
							_FTrace_FWOnReaderCloseCB, psDevInfo,
#if !defined(SUPPORT_TL_PRODUCER_CALLBACK)
							NULL, NULL
#else
							/* Not enabled by default */
							GPUTraceTLCB, psDevInfo
#endif
							);
	PVR_RETURN_IF_ERROR(eError);

	psDevInfo->hHWPerfStream[RGX_HWPERF_L2_STREAM_FTRACE] = hStream;
	psDevInfo->uiHWPerfStreamCount++;
	PVR_ASSERT(psDevInfo->uiHWPerfStreamCount <= RGX_HWPERF_L2_STREAM_LAST);

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRGpuTraceInitDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError;
	RGX_HWPERF_FTRACE_DATA *psData;
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	PVRSRV_VZ_RET_IF_MODE(GUEST, DEVNODE, psDeviceNode, PVRSRV_OK);

	psData = OSAllocZMem(sizeof(RGX_HWPERF_FTRACE_DATA));
	psDevInfo->pvGpuFtraceData = psData;
	PVR_LOG_GOTO_IF_NOMEM(psData, eError, e0);

	/* We initialise it only once because we want to track if any
	 * packets were dropped. */
	psData->ui32FTraceLastOrdinal = IMG_UINT32_MAX - 1;

	eError = OSLockCreate(&psData->hFTraceResourceLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate", e0);

	return PVRSRV_OK;

e0:
	PVRGpuTraceDeInitDevice(psDeviceNode);
	return eError;
}

void PVRGpuTraceDeInitDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	RGX_HWPERF_FTRACE_DATA *psData = psDevInfo->pvGpuFtraceData;

	PVRSRV_VZ_RETN_IF_MODE(GUEST, DEVNODE, psDeviceNode);
	if (psData)
	{
		/* first disable the tracing, to free up TL resources */
		if (psData->hFTraceResourceLock)
		{
			OSLockAcquire(psData->hFTraceResourceLock);
			_GpuTraceDisable(psDeviceNode->pvDevice, IMG_TRUE);
			OSLockRelease(psData->hFTraceResourceLock);

			/* now free all the FTrace resources */
			OSLockDestroy(psData->hFTraceResourceLock);
		}
		OSFreeMem(psData);
		psDevInfo->pvGpuFtraceData = NULL;
	}
}

static INLINE IMG_BOOL PVRGpuTraceIsEnabled(void)
{
	return gbFTraceGPUEventsEnabled;
}

void PVRGpuTraceInitIfEnabled(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	if (PVRGpuTraceIsEnabled())
	{
		IMG_BOOL bEnable = IMG_FALSE;

		PVRSRV_ERROR eError = PVRGpuTraceSetEnabled(psDeviceNode, IMG_TRUE);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to initialise GPU event tracing"
					" (%s)", PVRSRVGetErrorString(eError)));
		}

#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
		/* below functions will enable FTrace events which in turn will
		 * execute HWPerf callbacks that set appropriate filter values
		 * note: unfortunately the functions don't allow to pass private
		 *       data so they enable events for all of the devices
		 *       at once, which means that this can happen more than once
		 *       if there is more than one device */

		/* single events can be enabled by calling trace_set_clr_event()
		 * with the event name, e.g.:
		 * trace_set_clr_event("rogue", "rogue_ufo_update", 1) */
		if (trace_set_clr_event("rogue", NULL, 1) != 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable \"rogue\" event"
			        " group"));
		}
		else
		{
			PVR_LOG(("FTrace events from \"rogue\" group enabled"));
			bEnable = IMG_TRUE;
		}
#endif

#if defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
		if (trace_set_clr_event("power", "gpu_work_period", 1) != 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to enable \"gpu_work_period\" event"));
		}
		else
		{
			PVR_LOG(("FTrace event from \"gpu_work_period\" enabled"));
			bEnable = IMG_TRUE;
		}
#endif

		if (bEnable)
		{
			/* this enables FTrace globally (if not enabled nothing will appear
			 * in the FTrace buffer) */
			tracing_on();
		}
	}
}

/* Caller must now hold hFTraceResourceLock before calling this method.
 */
static PVRSRV_ERROR _GpuTraceEnable(PVRSRV_RGXDEV_INFO *psRgxDevInfo)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	RGX_HWPERF_FTRACE_DATA *psFtraceData;
	PVRSRV_DEVICE_NODE *psRgxDevNode = psRgxDevInfo->psDeviceNode;
	IMG_CHAR pszFTraceStreamName[sizeof(PVRSRV_TL_HWPERF_RGX_FW_STREAM) + 4];
	IMG_UINT64 uiFilter;

	PVR_DPF_ENTERED;

	PVR_ASSERT(psRgxDevInfo);

	psFtraceData = psRgxDevInfo->pvGpuFtraceData;

	PVR_ASSERT(OSLockIsLocked(psFtraceData->hFTraceResourceLock));

	/* return if already enabled */
	if (psFtraceData->hGPUFTraceTLStream != NULL)
	{
		return PVRSRV_OK;
	}

	uiFilter =
#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
	    RGX_HWPERF_EVENT_MASK_HW_KICKFINISH | RGX_HWPERF_EVENT_MASK_FW_SED | RGX_HWPERF_EVENT_MASK_FW_UFO |
#endif
#if defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_TRACE_EVENT_GPU_WORK_PERIOD) |
#endif
	    0;

#if defined(SUPPORT_RGX) && !defined(NO_HARDWARE)
	/* Signal FW to enable event generation */
	if (psRgxDevInfo->bFirmwareInitialised)
	{
		eError = PVRSRVRGXCtrlHWPerfFW(psRgxDevNode,
		                               RGX_HWPERF_L2_STREAM_FTRACE,
		                               uiFilter, HWPERF_FILTER_OPERATION_SET);
		PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVRGXCtrlHWPerfFW", err_out);
	}
	else
#endif
	{
		/* only set filter and exit */
		(void) RGXHWPerfFwSetEventFilter(psRgxDevInfo, RGX_HWPERF_L2_STREAM_FTRACE, uiFilter);

		return PVRSRV_OK;
	}

	/* form the FTrace stream name, corresponding to this DevNode; which can make sense in the UM */
	if (OSSNPrintf(pszFTraceStreamName, sizeof(pszFTraceStreamName), "%s%d",
					PVRSRV_TL_FTRACE_RGX_FW_STREAM, psRgxDevNode->sDevId.i32KernelDeviceID) < 0)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Failed to form FTrace stream name for device %d",
		         __func__,
		         psRgxDevNode->sDevId.i32KernelDeviceID));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* In case there was any data copied from the L1 buffer to the FTrace L2
	 * buffer in the narrow window between disabling the FTrace and MISR
	 * running, drop this data to make the ordinal tracking correct. */
	TLStreamReset(psRgxDevInfo->hHWPerfStream[RGX_HWPERF_L2_STREAM_FTRACE]);

	/* Open the TL Stream for FTrace data consumption */
	eError = TLClientOpenStream(DIRECT_BRIDGE_HANDLE,
								pszFTraceStreamName,
								PVRSRV_STREAM_FLAG_ACQUIRE_NONBLOCKING,
								&psFtraceData->hGPUFTraceTLStream);
	PVR_LOG_GOTO_IF_ERROR(eError, "TLClientOpenStream", err_out);

#if defined(SUPPORT_RGX)
	if (RGXTimeCorrGetClockSource(psRgxDevNode) != TRACE_CLK)
	{
		/* Set clock source for timer correlation data to hwperf clock */
		eError = RGXTimeCorrSetClockSource(psRgxDevNode, TRACE_CLK);
		PVR_LOG_IF_ERROR(eError, "RGXTimeCorrSetClockSource");
	}
#endif

	/* Reset the ordinal tracking flag. We should skip checking for gaps in
	 * ordinal on the first run after FTrace is enabled in case another HWPerf
	 * consumer was connected while FTrace was disabled. */
	psFtraceData->bTrackOrdinals = IMG_FALSE;

	/* Reset the OS timestamp coming from the timer correlation data
	 * associated with the latest HWPerf event we processed.
	 */
	psFtraceData->ui64LastSampledTimeCorrOSTimeStamp = 0;

	/* Register a notifier to collect HWPerf data whenever the HW completes
	 * an operation.
	 */
	eError = PVRSRVRegisterCmdCompleteNotify(
		&psFtraceData->hGPUTraceCmdCompleteHandle,
		&_GpuTraceCmdCompleteNotify,
		psRgxDevInfo);
	PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVRegisterCmdCompleteNotify", err_close_stream);

err_out:
	PVR_DPF_RETURN_RC(eError);
err_close_stream:
	TLClientCloseStream(DIRECT_BRIDGE_HANDLE,
						psFtraceData->hGPUFTraceTLStream);
	psFtraceData->hGPUFTraceTLStream = NULL;
	goto err_out;
}

/* Caller must now hold hFTraceResourceLock before calling this method.
 */
static PVRSRV_ERROR _GpuTraceDisable(PVRSRV_RGXDEV_INFO *psRgxDevInfo, IMG_BOOL bDeInit)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	RGX_HWPERF_FTRACE_DATA *psFtraceData;
#if defined(SUPPORT_RGX)
	PVRSRV_DEVICE_NODE *psRgxDevNode = psRgxDevInfo->psDeviceNode;
#endif

	PVR_DPF_ENTERED;

	PVR_ASSERT(psRgxDevInfo);

	psFtraceData = psRgxDevInfo->pvGpuFtraceData;

	PVR_ASSERT(OSLockIsLocked(psFtraceData->hFTraceResourceLock));

	/* if FW is not yet initialised, just set filter and exit */
	if (!psRgxDevInfo->bFirmwareInitialised)
	{
		(void) RGXHWPerfFwSetEventFilter(psRgxDevInfo, RGX_HWPERF_L2_STREAM_FTRACE,
		                                 RGX_HWPERF_EVENT_MASK_NONE);
		return PVRSRV_OK;
	}

	if (psFtraceData->hGPUFTraceTLStream == NULL)
	{
		/* Tracing already disabled, just return */
		return PVRSRV_OK;
	}

#if defined(SUPPORT_RGX)
	if (!bDeInit)
	{
		eError = PVRSRVRGXCtrlHWPerfFW(psRgxDevNode,
		                               RGX_HWPERF_L2_STREAM_FTRACE,
		                               RGX_HWPERF_EVENT_MASK_NONE,
		                               HWPERF_FILTER_OPERATION_SET);
		PVR_LOG_IF_ERROR(eError, "PVRSRVRGXCtrlHWPerfFW");
	}
#endif

	if (psFtraceData->hGPUTraceCmdCompleteHandle)
	{
		/* Tracing is being turned off. Unregister the notifier. */
		eError = PVRSRVUnregisterCmdCompleteNotify(
				psFtraceData->hGPUTraceCmdCompleteHandle);
		PVR_LOG_IF_ERROR(eError, "PVRSRVUnregisterCmdCompleteNotify");
		psFtraceData->hGPUTraceCmdCompleteHandle = NULL;
	}

	/* Let close stream perform the release data on the outstanding acquired
	 * data */
	eError = TLClientCloseStream(DIRECT_BRIDGE_HANDLE,
	                             psFtraceData->hGPUFTraceTLStream);
	PVR_LOG_IF_ERROR(eError, "TLClientCloseStream");

	psFtraceData->hGPUFTraceTLStream = NULL;

#if defined(SUPPORT_RGX)
	if (psRgxDevInfo->ui32LastClockSource != TRACE_CLK)
	{
		RGXTimeCorrSetClockSource(psRgxDevNode, psRgxDevInfo->ui32LastClockSource);
	}
#endif

	PVR_DPF_RETURN_RC(eError);
}

static PVRSRV_ERROR _GpuTraceSetEnabled(PVRSRV_RGXDEV_INFO *psRgxDevInfo,
                                        IMG_BOOL bNewValue)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	RGX_HWPERF_FTRACE_DATA *psFtraceData;

	PVR_DPF_ENTERED;

	PVR_ASSERT(psRgxDevInfo);
	psFtraceData = psRgxDevInfo->pvGpuFtraceData;

	/* About to create/destroy FTrace resources, lock critical section
	 * to avoid HWPerf MISR thread contention.
	 */
	OSLockAcquire(psFtraceData->hFTraceResourceLock);

	eError = (bNewValue ? _GpuTraceEnable(psRgxDevInfo)
					   : _GpuTraceDisable(psRgxDevInfo, IMG_FALSE));

	OSLockRelease(psFtraceData->hFTraceResourceLock);

	PVR_DPF_RETURN_RC(eError);
}

static PVRSRV_ERROR _GpuTraceSetEnabledForAllDevices(IMG_BOOL bNewValue)
{
	PVRSRV_ERROR eError = PVRSRV_ERROR_NOT_IMPLEMENTED;
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_NODE *psDeviceNode;

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode = psPVRSRVData->psDeviceNodeList;

	/* enable/disable GPU trace on all devices */
	while (psDeviceNode)
	{
		PVRSRV_RGXDEV_INFO *psRgxDevInfo = (PVRSRV_RGXDEV_INFO*)psDeviceNode->pvDevice;

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psRgxDevInfo))
#endif
		{
			eError = _GpuTraceSetEnabled(psRgxDevInfo, bNewValue);
			if (eError != PVRSRV_OK)
				break;
		}

		psDeviceNode = psDeviceNode->psNext;
	}

	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

	PVR_DPF_RETURN_RC(eError);
}

PVRSRV_ERROR PVRGpuTraceSetEnabled(PVRSRV_DEVICE_NODE *psDeviceNode,
                                   IMG_BOOL bNewValue)
{
	return _GpuTraceSetEnabled(psDeviceNode->pvDevice, bNewValue);
}

#if defined(PVRSRV_TRACE_ROGUE_EVENTS)

/* ----- HWPerf to FTrace packet processing and events injection ------------ */

static const IMG_CHAR *_HWPerfKickTypeToStr(RGX_HWPERF_KICK_TYPE eKickType)
{
	static const IMG_CHAR *aszKickType[RGX_HWPERF_KICK_TYPE2_LAST+1] = {
		"TA3D", /* Deprecated */
#if defined(RGX_FEATURE_HWPERF_VOLCANIC)
		/* Volcanic deprecated kick types */
		"CDM", "RS", "SHG", "TQTDM", "SYNC", "TA", "3D", "LAST",

		"<UNKNOWN>", "<UNKNOWN>", "<UNKNOWN>", "<UNKNOWN>", "<UNKNOWN>",
		"<UNKNOWN>", "<UNKNOWN>", "<UNKNOWN>",
#else
		/* Rogue deprecated kick types */
		"TQ2D", "TQ3D", "CDM", "RS", "VRDM", "TQTDM", "SYNC", "TA", "3D", "LAST",

		"<UNKNOWN>", "<UNKNOWN>", "<UNKNOWN>", "<UNKNOWN>", "<UNKNOWN>",
		"<UNKNOWN>",
#endif
		"TQ2D", "TQ3D", "TQTDM", "CDM", "GEOM", "3D", "SYNC", "RS", "LAST"
	};

	/* cast in case of negative value */
	if (((IMG_UINT32) eKickType) >= RGX_HWPERF_KICK_TYPE2_LAST)
	{
		return "<UNKNOWN>";
	}

	return aszKickType[eKickType];
}
void PVRGpuTraceEnqueueEvent(
		PVRSRV_DEVICE_NODE *psDevNode,
		IMG_UINT32 ui32FirmwareCtx,
		IMG_UINT32 ui32ExtJobRef,
		IMG_UINT32 ui32IntJobRef,
		RGX_HWPERF_KICK_TYPE eKickType)
{
	const IMG_CHAR *pszKickType = _HWPerfKickTypeToStr(eKickType);

	PVR_DPF((PVR_DBG_MESSAGE, "PVRGpuTraceEnqueueEvent(%s): contextId %u, "
	        "jobId %u", pszKickType, ui32FirmwareCtx, ui32IntJobRef));

	if (PVRGpuTraceIsEnabled())
	{
		trace_rogue_job_enqueue(psDevNode->sDevId.ui32InternalID, ui32FirmwareCtx,
		                        ui32IntJobRef, ui32ExtJobRef, pszKickType);
	}
}

static void _GpuTraceWorkSwitch(
		IMG_UINT64 ui64HWTimestampInOSTime,
		IMG_UINT32 ui32GpuId,
		IMG_UINT32 ui32CtxId,
		IMG_UINT32 ui32CtxPriority,
		IMG_UINT32 ui32ExtJobRef,
		IMG_UINT32 ui32IntJobRef,
		const IMG_CHAR* pszWorkType,
		PVR_GPUTRACE_SWITCH_TYPE eSwType)
{
	PVR_ASSERT(pszWorkType);
	trace_rogue_sched_switch(pszWorkType, eSwType, ui64HWTimestampInOSTime,
	                         ui32GpuId, ui32CtxId, 2-ui32CtxPriority, ui32IntJobRef,
	                         ui32ExtJobRef);
}

static void _GpuTraceUfo(
		IMG_UINT64 ui64OSTimestamp,
		const RGX_HWPERF_UFO_EV eEvType,
		const IMG_UINT32 ui32GpuId,
		const IMG_UINT32 ui32CtxId,
		const IMG_UINT32 ui32ExtJobRef,
		const IMG_UINT32 ui32IntJobRef,
		const IMG_UINT32 ui32UFOCount,
		const RGX_HWPERF_UFO_DATA_ELEMENT *puData)
{
	switch (eEvType) {
		case RGX_HWPERF_UFO_EV_UPDATE:
			trace_rogue_ufo_updates(ui64OSTimestamp, ui32GpuId, ui32CtxId,
					ui32ExtJobRef, ui32IntJobRef, ui32UFOCount, puData);
			break;
		case RGX_HWPERF_UFO_EV_CHECK_SUCCESS:
			trace_rogue_ufo_checks_success(ui64OSTimestamp, ui32GpuId, ui32CtxId,
					ui32ExtJobRef, ui32IntJobRef, IMG_FALSE, ui32UFOCount,
					puData);
			break;
		case RGX_HWPERF_UFO_EV_PRCHECK_SUCCESS:
			trace_rogue_ufo_checks_success(ui64OSTimestamp, ui32GpuId, ui32CtxId,
					ui32ExtJobRef, ui32IntJobRef, IMG_TRUE, ui32UFOCount,
					puData);
			break;
		case RGX_HWPERF_UFO_EV_CHECK_FAIL:
			trace_rogue_ufo_checks_fail(ui64OSTimestamp, ui32GpuId, ui32CtxId,
					ui32ExtJobRef, ui32IntJobRef, IMG_FALSE, ui32UFOCount,
					puData);
			break;
		case RGX_HWPERF_UFO_EV_PRCHECK_FAIL:
			trace_rogue_ufo_checks_fail(ui64OSTimestamp, ui32GpuId, ui32CtxId,
					ui32ExtJobRef, ui32IntJobRef, IMG_TRUE, ui32UFOCount,
					puData);
			break;
		default:
			break;
	}
}

static void _GpuTraceFirmware(
		IMG_UINT64 ui64HWTimestampInOSTime,
		IMG_UINT32 ui32GpuId,
		const IMG_CHAR* pszWorkType,
		PVR_GPUTRACE_SWITCH_TYPE eSwType)
{
	trace_rogue_firmware_activity(ui64HWTimestampInOSTime, ui32GpuId, pszWorkType, eSwType);
}

static void _GpuTraceEventsLost(
		const RGX_HWPERF_STREAM_ID eStreamId,
		IMG_UINT32 ui32GpuId,
		const IMG_UINT32 ui32LastOrdinal,
		const IMG_UINT32 ui32CurrOrdinal)
{
	trace_rogue_events_lost(eStreamId, ui32GpuId, ui32LastOrdinal, ui32CurrOrdinal);
}
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) */

#if defined(PVRSRV_TRACE_ROGUE_EVENTS) || defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
/* Calculate the OS timestamp given an RGX timestamp in the HWPerf event. */
static uint64_t CalculateEventTimestamp(
	PVRSRV_RGXDEV_INFO *psDevInfo,
	uint32_t ui32TimeCorrIndex,
	uint64_t ui64EventTimestamp)
{
	RGXFWIF_GPU_UTIL_FW *psGpuUtilFW = psDevInfo->psRGXFWIfGpuUtilFW;
	RGX_HWPERF_FTRACE_DATA *psFtraceData = psDevInfo->pvGpuFtraceData;
	RGXFWIF_TIME_CORR *psTimeCorr;
	uint64_t ui64CRTimeStamp;
	uint64_t ui64OSTimeStamp;
	uint64_t ui64CRDeltaToOSDeltaKNs;
	uint64_t ui64EventOSTimestamp, deltaRgxTimer, delta_ns;

	RGXFwSharedMemCacheOpValue(psGpuUtilFW->sTimeCorr[ui32TimeCorrIndex], INVALIDATE);
	psTimeCorr = &psGpuUtilFW->sTimeCorr[ui32TimeCorrIndex];
	ui64CRTimeStamp = psTimeCorr->ui64CRTimeStamp;
	ui64OSTimeStamp = psTimeCorr->ui64OSTimeStamp;
	ui64CRDeltaToOSDeltaKNs = psTimeCorr->ui64CRDeltaToOSDeltaKNs;

	if (psFtraceData->ui64LastSampledTimeCorrOSTimeStamp > ui64OSTimeStamp)
	{
		/* The previous packet had a time reference (time correlation data) more
		 * recent than the one in the current packet, it means the timer
		 * correlation array wrapped too quickly (buffer too small) and in the
		 * previous call to _GpuTraceUfoEvent we read one of the
		 * newest timer correlations rather than one of the oldest ones.
		 */
		PVR_DPF((PVR_DBG_ERROR, "%s: The timestamps computed so far could be "
				 "wrong! The time correlation array size should be increased "
				 "to avoid this.", __func__));
	}

	psFtraceData->ui64LastSampledTimeCorrOSTimeStamp = ui64OSTimeStamp;

	/* RGX CR timer ticks delta */
	deltaRgxTimer = ui64EventTimestamp - ui64CRTimeStamp;
	/* RGX time delta in nanoseconds */
	delta_ns = RGXFWIF_GET_DELTA_OSTIME_NS(deltaRgxTimer, ui64CRDeltaToOSDeltaKNs);
	/* Calculate OS time of HWPerf event */
	ui64EventOSTimestamp = ui64OSTimeStamp + delta_ns;

	PVR_DPF((PVR_DBG_VERBOSE, "%s: psCurrentDvfs RGX %llu, OS %llu, DVFSCLK %u",
			 __func__, ui64CRTimeStamp, ui64OSTimeStamp,
			 psTimeCorr->ui32CoreClockSpeed));

	return ui64EventOSTimestamp;
}

static void _GpuTraceSwitchEvent(PVRSRV_RGXDEV_INFO *psDevInfo,
		RGX_HWPERF_V2_PACKET_HDR* psHWPerfPkt, const IMG_CHAR* pszWorkName,
		PVR_GPUTRACE_SWITCH_TYPE eSwType)
{
	IMG_UINT64 ui64Timestamp;
	RGX_HWPERF_HW_DATA* psHWPerfPktData;

	PVR_DPF_ENTERED;

	PVR_ASSERT(psHWPerfPkt);
	PVR_ASSERT(pszWorkName);

	psHWPerfPktData = RGX_HWPERF_GET_PACKET_DATA_BYTES(psHWPerfPkt);

	ui64Timestamp = CalculateEventTimestamp(psDevInfo, psHWPerfPktData->ui32TimeCorrIndex,
											psHWPerfPkt->ui64Timestamp);

	PVR_DPF((PVR_DBG_VERBOSE, "_GpuTraceSwitchEvent: %s ui32ExtJobRef=%d, ui32IntJobRef=%d, eSwType=%d",
			 pszWorkName != NULL ? pszWorkName : "?", psHWPerfPktData->ui32DMContext,
			 psHWPerfPktData->ui32IntJobRef, eSwType));

#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
	_GpuTraceWorkSwitch(ui64Timestamp,
	                    psDevInfo->psDeviceNode->sDevId.ui32InternalID,
	                    psHWPerfPktData->ui32DMContext,
	                    psHWPerfPktData->ui32CtxPriority,
	                    psHWPerfPktData->ui32ExtJobRef,
	                    psHWPerfPktData->ui32IntJobRef,
	                    pszWorkName,
	                    eSwType);
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) */

	PVR_DPF_RETURN;
}
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) || defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD) */

#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
static void _GpuTraceUfoEvent(PVRSRV_RGXDEV_INFO *psDevInfo,
                              RGX_HWPERF_V2_PACKET_HDR* psHWPerfPkt)
{
	IMG_UINT64 ui64Timestamp;
	RGX_HWPERF_UFO_DATA *psHWPerfPktData;
	IMG_UINT32 ui32UFOCount;
	RGX_HWPERF_UFO_DATA_ELEMENT *puData;

	psHWPerfPktData = RGX_HWPERF_GET_PACKET_DATA_BYTES(psHWPerfPkt);

	ui32UFOCount = RGX_HWPERF_GET_UFO_STREAMSIZE(psHWPerfPktData->ui32StreamInfo);
	puData = (RGX_HWPERF_UFO_DATA_ELEMENT *) IMG_OFFSET_ADDR(psHWPerfPktData, RGX_HWPERF_GET_UFO_STREAMOFFSET(psHWPerfPktData->ui32StreamInfo));

	ui64Timestamp = CalculateEventTimestamp(psDevInfo, psHWPerfPktData->ui32TimeCorrIndex,
											psHWPerfPkt->ui64Timestamp);

	PVR_DPF((PVR_DBG_VERBOSE, "_GpuTraceUfoEvent: ui32ExtJobRef=%d, "
	        "ui32IntJobRef=%d", psHWPerfPktData->ui32ExtJobRef,
	        psHWPerfPktData->ui32IntJobRef));

	_GpuTraceUfo(ui64Timestamp, psHWPerfPktData->eEvType,
	             psDevInfo->psDeviceNode->sDevId.ui32InternalID,
	             psHWPerfPktData->ui32DMContext, psHWPerfPktData->ui32ExtJobRef,
	             psHWPerfPktData->ui32IntJobRef, ui32UFOCount, puData);
}

static void _GpuTraceFirmwareEvent(PVRSRV_RGXDEV_INFO *psDevInfo,
		RGX_HWPERF_V2_PACKET_HDR* psHWPerfPkt, const IMG_CHAR* pszWorkName,
		PVR_GPUTRACE_SWITCH_TYPE eSwType)

{
	uint64_t ui64Timestamp;
	RGX_HWPERF_FW_DATA *psHWPerfPktData = RGX_HWPERF_GET_PACKET_DATA_BYTES(psHWPerfPkt);

	ui64Timestamp = CalculateEventTimestamp(psDevInfo, psHWPerfPktData->ui32TimeCorrIndex,
											psHWPerfPkt->ui64Timestamp);

	_GpuTraceFirmware(ui64Timestamp, psDevInfo->psDeviceNode->sDevId.ui32InternalID, pszWorkName,
	                  eSwType);
}
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) */

static IMG_BOOL ValidAndEmitFTraceEvent(PVRSRV_RGXDEV_INFO *psDevInfo,
		RGX_HWPERF_V2_PACKET_HDR* psHWPerfPkt)
{
	RGX_HWPERF_EVENT_TYPE eType;
	RGX_HWPERF_FTRACE_DATA *psFtraceData = psDevInfo->pvGpuFtraceData;
#if defined(PVRSRV_TRACE_ROGUE_EVENTS) || defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
	IMG_UINT32 ui32HwEventTypeIndex;
	static const struct {
		IMG_CHAR* pszName;
		PVR_GPUTRACE_SWITCH_TYPE eSwType;
	} aszHwEventTypeMap[] = {
#define _T(T) PVR_GPUTRACE_SWITCH_TYPE_##T
		{ "BG",             _T(BEGIN)  }, /* RGX_HWPERF_FW_BGSTART */
		{ "BG",             _T(END)    }, /* RGX_HWPERF_FW_BGEND */
		{ "IRQ",            _T(BEGIN)  }, /* RGX_HWPERF_FW_IRQSTART */
		{ "IRQ",            _T(END)    }, /* RGX_HWPERF_FW_IRQEND */
		{ "DBG",            _T(BEGIN)  }, /* RGX_HWPERF_FW_DBGSTART */
		{ "DBG",            _T(END)    }, /* RGX_HWPERF_FW_DBGEND */
		{ "PMOOM_TAPAUSE",  _T(END)    }, /* RGX_HWPERF_HW_PMOOM_TAPAUSE */
		{ "TA",             _T(BEGIN)  }, /* RGX_HWPERF_HW_TAKICK */
		{ "TA",             _T(END)    }, /* RGX_HWPERF_HW_TAFINISHED */
		{ "TQ3D",           _T(BEGIN)  }, /* RGX_HWPERF_HW_3DTQKICK */
		{ "3D",             _T(BEGIN)  }, /* RGX_HWPERF_HW_3DKICK */
		{ "3D",             _T(END)    }, /* RGX_HWPERF_HW_3DFINISHED */
		{ "CDM",            _T(BEGIN)  }, /* RGX_HWPERF_HW_CDMKICK */
		{ "CDM",            _T(END)    }, /* RGX_HWPERF_HW_CDMFINISHED */
		{ "TQ2D",           _T(BEGIN)  }, /* RGX_HWPERF_HW_TLAKICK */
		{ "TQ2D",           _T(END)    }, /* RGX_HWPERF_HW_TLAFINISHED */
		{ "3DSPM",          _T(BEGIN)  }, /* RGX_HWPERF_HW_3DSPMKICK */
		{ NULL,             0          }, /* RGX_HWPERF_HW_PERIODIC (unsupported) */
		{ "RTU",            _T(BEGIN)  }, /* RGX_HWPERF_HW_RTUKICK */
		{ "RTU",            _T(END)    }, /* RGX_HWPERF_HW_RTUFINISHED */
		{ "SHG",            _T(BEGIN)  }, /* RGX_HWPERF_HW_SHGKICK */
		{ "SHG",            _T(END)    }, /* RGX_HWPERF_HW_SHGFINISHED */
		{ "TQ3D",           _T(END)    }, /* RGX_HWPERF_HW_3DTQFINISHED */
		{ "3DSPM",          _T(END)    }, /* RGX_HWPERF_HW_3DSPMFINISHED */
		{ "PMOOM_TARESUME", _T(BEGIN)  }, /* RGX_HWPERF_HW_PMOOM_TARESUME */
		{ "TDM",            _T(BEGIN)  }, /* RGX_HWPERF_HW_TDMKICK */
		{ "TDM",            _T(END)    }, /* RGX_HWPERF_HW_TDMFINISHED */
		{ "NULL",           _T(SINGLE) }, /* RGX_HWPERF_HW_NULLKICK */
		{ "GPU_WORK_PERIOD",_T(SINGLE) }, /* RGX_HWPERF_HW_GPU_WORK_PERIOD */
#undef _T
	};
	static_assert(RGX_HWPERF_HW_EVENT_RANGE0_FIRST_TYPE == RGX_HWPERF_FW_EVENT_RANGE_LAST_TYPE + 1,
				  "FW and HW events are not contiguous in RGX_HWPERF_EVENT_TYPE");
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) || defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD) */

	PVR_ASSERT(psHWPerfPkt);
	eType = RGX_HWPERF_GET_TYPE(psHWPerfPkt);

	if (psFtraceData->bTrackOrdinals)
	{
		if (psFtraceData->ui32FTraceLastOrdinal != psHWPerfPkt->ui32Ordinal - 1)
		{
#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
			_GpuTraceEventsLost(RGX_HWPERF_GET_STREAM_ID(psHWPerfPkt),
			                    psDevInfo->psDeviceNode->sDevId.ui32InternalID,
			                    psFtraceData->ui32FTraceLastOrdinal,
			                    psHWPerfPkt->ui32Ordinal);
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) */
			PVR_DPF((PVR_DBG_WARNING, "FTrace events lost (stream_id = %u, ordinal: last = %u, current = %u)",
			         RGX_HWPERF_GET_STREAM_ID(psHWPerfPkt), psFtraceData->ui32FTraceLastOrdinal,
			         psHWPerfPkt->ui32Ordinal));
		}
	}
	else
	{
		psFtraceData->bTrackOrdinals = IMG_TRUE;
	}

	psFtraceData->ui32FTraceLastOrdinal = psHWPerfPkt->ui32Ordinal;

#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
	/* Process UFO packets */
	if (eType == RGX_HWPERF_UFO)
	{
		_GpuTraceUfoEvent(psDevInfo, psHWPerfPkt);
		return IMG_TRUE;
	}
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) */

#if defined(PVRSRV_TRACE_ROGUE_EVENTS) || defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
	if (eType <= RGX_HWPERF_HW_EVENT_RANGE0_LAST_TYPE)
	{
		/* this ID belongs to range 0, so index directly in range 0 */
		ui32HwEventTypeIndex = eType - RGX_HWPERF_FW_EVENT_RANGE_FIRST_TYPE;
	}
	else
	{
		/* this ID belongs to range 1, so first index in range 1 and skip number of slots used up for range 0 */
		ui32HwEventTypeIndex = (eType - RGX_HWPERF_HW_EVENT_RANGE1_FIRST_TYPE) +
		                       (RGX_HWPERF_HW_EVENT_RANGE0_LAST_TYPE - RGX_HWPERF_FW_EVENT_RANGE_FIRST_TYPE + 1);
	}

	if (ui32HwEventTypeIndex >= ARRAY_SIZE(aszHwEventTypeMap))
	{
		goto err_unsupported;
	}

	if (aszHwEventTypeMap[ui32HwEventTypeIndex].pszName == NULL)
	{
		/* Not supported map entry, ignore event */
		goto err_unsupported;
	}

	if (HWPERF_PACKET_IS_HW_TYPE(eType))
	{
		if (aszHwEventTypeMap[ui32HwEventTypeIndex].eSwType == PVR_GPUTRACE_SWITCH_TYPE_SINGLE)
		{
			_GpuTraceSwitchEvent(psDevInfo, psHWPerfPkt,
			                     aszHwEventTypeMap[ui32HwEventTypeIndex].pszName,
			                     PVR_GPUTRACE_SWITCH_TYPE_BEGIN);
			_GpuTraceSwitchEvent(psDevInfo, psHWPerfPkt,
			                     aszHwEventTypeMap[ui32HwEventTypeIndex].pszName,
			                     PVR_GPUTRACE_SWITCH_TYPE_END);
		}
		else
		{
			_GpuTraceSwitchEvent(psDevInfo, psHWPerfPkt,
			                     aszHwEventTypeMap[ui32HwEventTypeIndex].pszName,
			                     aszHwEventTypeMap[ui32HwEventTypeIndex].eSwType);
		}

		return IMG_TRUE;
	}
#if defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
	else if (psDevInfo->psDeviceNode->bGPUWorkPeriodFTraceEnabled &&
		 eType == RGX_HWPERF_TRACE_EVENT_GPU_WORK_PERIOD &&
		 RGX_HWPERF_GET_OSID(psHWPerfPkt) == RGXFW_HOST_DRIVER_ID)
	{
		RGX_HWPERF_GPU_WORK_PERIOD_DATA *psHWPerfPktData;
		IMG_UINT64 ui64StartTimestamp, ui64EndTimestamp;

		psHWPerfPktData = RGX_HWPERF_GET_PACKET_DATA_BYTES(psHWPerfPkt);
		ui64StartTimestamp = CalculateEventTimestamp(psDevInfo,
					psHWPerfPktData->ui32StartTimeCorrIndex,
					psHWPerfPktData->ui64GPUWorkPeriodStartTime);
		ui64EndTimestamp = CalculateEventTimestamp(psDevInfo,
					psHWPerfPktData->ui32TimeCorrIndex,
					psHWPerfPkt->ui64Timestamp);

		PVR_ASSERT(ui64EndTimestamp > ui64StartTimestamp);

		trace_gpu_work_period(psDevInfo->psDeviceNode->sDevId.ui32InternalID,
				psHWPerfPktData->ui32UID,
				ui64StartTimestamp,
				ui64EndTimestamp,
				ui64EndTimestamp - ui64StartTimestamp);

		return IMG_TRUE;
	}
#endif
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) || defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD) */
#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
	if (HWPERF_PACKET_IS_FW_TYPE(eType))
	{
		_GpuTraceFirmwareEvent(psDevInfo, psHWPerfPkt,
										aszHwEventTypeMap[ui32HwEventTypeIndex].pszName,
										aszHwEventTypeMap[ui32HwEventTypeIndex].eSwType);

		return IMG_TRUE;
	}
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) */

#if defined(PVRSRV_TRACE_ROGUE_EVENTS) || defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
err_unsupported:
#endif /* defined(PVRSRV_TRACE_ROGUE_EVENTS) || defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD) */
	PVR_DPF((PVR_DBG_VERBOSE, "%s: Unsupported event type %d", __func__, eType));
	return IMG_FALSE;
}

static void _GpuTraceProcessPackets(PVRSRV_RGXDEV_INFO *psDevInfo, void *pBuffer,
                                    IMG_UINT32 ui32ReadLen)
{
	IMG_UINT32			ui32TlPackets = 0;
	IMG_UINT32			ui32HWPerfPackets = 0;
	IMG_UINT32			ui32HWPerfPacketsSent = 0;
	void				*pBufferEnd;
	PVRSRVTL_PPACKETHDR psHDRptr;
	PVRSRVTL_PACKETTYPE ui16TlType;

	PVR_DPF_ENTERED;

	PVR_ASSERT(psDevInfo);
	PVR_ASSERT(pBuffer);
	PVR_ASSERT(ui32ReadLen);

	/* Process the TL Packets
	 */
	pBufferEnd = IMG_OFFSET_ADDR(pBuffer, ui32ReadLen);
	psHDRptr = GET_PACKET_HDR(pBuffer);
	while ( psHDRptr < (PVRSRVTL_PPACKETHDR)pBufferEnd )
	{
		ui16TlType = GET_PACKET_TYPE(psHDRptr);
		if (ui16TlType == PVRSRVTL_PACKETTYPE_DATA)
		{
			IMG_UINT16 ui16DataLen = GET_PACKET_DATA_LEN(psHDRptr);
			if (0 == ui16DataLen)
			{
				PVR_DPF((PVR_DBG_ERROR, "_GpuTraceProcessPackets: ZERO Data in TL data packet: %p", psHDRptr));
			}
			else
			{
				RGX_HWPERF_V2_PACKET_HDR* psHWPerfPkt;
				RGX_HWPERF_V2_PACKET_HDR* psHWPerfEnd;

				/* Check for lost hwperf data packets */
				psHWPerfEnd = RGX_HWPERF_GET_PACKET(GET_PACKET_DATA_PTR(psHDRptr)+ui16DataLen);
				psHWPerfPkt = RGX_HWPERF_GET_PACKET(GET_PACKET_DATA_PTR(psHDRptr));
				do
				{
					if (ValidAndEmitFTraceEvent(psDevInfo, psHWPerfPkt))
					{
						ui32HWPerfPacketsSent++;
					}
					ui32HWPerfPackets++;
					psHWPerfPkt = RGX_HWPERF_GET_NEXT_PACKET(psHWPerfPkt);
				}
				while (psHWPerfPkt < psHWPerfEnd);
			}
		}
		else if (ui16TlType == PVRSRVTL_PACKETTYPE_MOST_RECENT_WRITE_FAILED)
		{
			PVR_DPF((PVR_DBG_MESSAGE, "_GpuTraceProcessPackets: Indication that the transport buffer was full"));
		}
		else
		{
			/* else Ignore padding packet type and others */
			PVR_DPF((PVR_DBG_MESSAGE, "_GpuTraceProcessPackets: Ignoring TL packet, type %d", ui16TlType ));
		}

		psHDRptr = GET_NEXT_PACKET_ADDR(psHDRptr);
		ui32TlPackets++;
	}

	PVR_DPF((PVR_DBG_VERBOSE, "_GpuTraceProcessPackets: TL "
			"Packets processed %03d, HWPerf packets %03d, sent %03d",
			ui32TlPackets, ui32HWPerfPackets, ui32HWPerfPacketsSent));

	PVR_DPF_RETURN;
}

static void _GpuTraceCmdCompleteNotify(PVRSRV_CMDCOMP_HANDLE hCmdCompHandle)
{
	PVRSRV_RGXDEV_INFO *psDeviceInfo = hCmdCompHandle;
	RGX_HWPERF_FTRACE_DATA *psFtraceData;
	PVRSRV_ERROR eError;
	IMG_PBYTE pBuffer;
	IMG_UINT32 ui32ReadLen;

	PVR_DPF_ENTERED;

	PVR_ASSERT(psDeviceInfo != NULL);

	psFtraceData = psDeviceInfo->pvGpuFtraceData;

	/* Command-complete notifiers can run concurrently. If this is happening,
	 * just bail out and let the previous call finish.
	 * This is ok because we can process the queued packets on the next call.
	 */
	if (!OSTryLockAcquire(psFtraceData->hFTraceResourceLock))
	{
		PVR_DPF_RETURN;
	}

	/* If this notifier is called, it means the TL resources will be valid
	 * at-least until the end of this call, since the DeInit function will wait
	 * on the hFTraceResourceLock to clean-up the TL resources and un-register
	 * the notifier, so just assert here.
	 */
	PVR_ASSERT(psFtraceData->hGPUFTraceTLStream != NULL);

	/* If we have a valid stream attempt to acquire some data */
	eError = TLClientAcquireData(DIRECT_BRIDGE_HANDLE,
	                             psFtraceData->hGPUFTraceTLStream,
	                             &pBuffer, &ui32ReadLen);
	if (eError != PVRSRV_OK)
	{
		if (eError != PVRSRV_ERROR_TIMEOUT)
		{
			PVR_LOG_ERROR(eError, "TLClientAcquireData");
		}

		goto unlock;
	}

	/* Process the HWPerf packets and release the data */
	if (ui32ReadLen > 0)
	{
		PVR_DPF((PVR_DBG_VERBOSE, "%s: DATA AVAILABLE offset=%p, length=%d",
		        __func__, pBuffer, ui32ReadLen));

		/* Process the transport layer data for HWPerf packets... */
		_GpuTraceProcessPackets(psDeviceInfo, pBuffer, ui32ReadLen);

		eError = TLClientReleaseData(DIRECT_BRIDGE_HANDLE,
		                             psFtraceData->hGPUFTraceTLStream);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "TLClientReleaseData");

			/* Serious error, disable FTrace GPU events */
			_GpuTraceDisable(psDeviceInfo, IMG_FALSE);
		}
	}

unlock:
	OSLockRelease(psFtraceData->hFTraceResourceLock);

	PVR_DPF_RETURN;
}

/* ----- AppHint interface -------------------------------------------------- */

static PVRSRV_ERROR _GpuTraceIsEnabledCallback(
	const PVRSRV_DEVICE_NODE *device,
	const void *private_data,
	IMG_BOOL *value)
{
	PVR_UNREFERENCED_PARAMETER(private_data);

#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	if (device && PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, device))
	{
		*value = IMG_FALSE;
	}
	else
#endif
	{
		PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
		PVRSRV_DEVICE_NODE *psDeviceNode = psPVRSRVData ? psPVRSRVData->psDeviceNodeList : NULL;

		while (psDeviceNode)
		{
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
			PVRSRV_RGXDEV_INFO *psRgxDevInfo = (PVRSRV_RGXDEV_INFO*)psDeviceNode->pvDevice;

			if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psRgxDevInfo))
#endif
			{
				*value = gbFTraceGPUEventsEnabled;
				return PVRSRV_OK;
			}
			psDeviceNode = psDeviceNode->psNext;
		}
		*value = IMG_FALSE;
	}

	return PVRSRV_ERROR_NOT_IMPLEMENTED;
}

static PVRSRV_ERROR _GpuTraceSetEnabledCallback(
	const PVRSRV_DEVICE_NODE *device,
	const void *private_data,
	IMG_BOOL value)
{
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	if (device && PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, device))
	{
		return PVRSRV_ERROR_NOT_IMPLEMENTED;
	}
#endif

	/* Lock down the state to avoid concurrent writes */
	OSLockAcquire(ghGPUTraceStateLock);

	if (value != gbFTraceGPUEventsEnabled)
	{
#if defined(PVRSRV_NEED_PVR_DPF)
		const IMG_CHAR *pszOperation = value ? "enable" : "disable";
		/* in case MESSAGE level is compiled out */
		PVR_UNREFERENCED_PARAMETER(pszOperation);
#endif

#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
		if (trace_set_clr_event("rogue", NULL, (int) value) != 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "FAILED to %s GPU FTrace event group",
			        pszOperation));
			goto err_restore_state;
		}
		else
		{
			PVR_DPF((PVR_DBG_MESSAGE, "FTrace events from \"rogue\" group %sd",
			        pszOperation));
		}
#endif

#if defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
		if (trace_set_clr_event("power", "gpu_work_period", (int) value) != 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "FAILED to %s gpu_work_period GPU FTrace event",
			        pszOperation));
			goto err_restore_state;
		}
		else
		{
			PVR_DPF((PVR_DBG_MESSAGE, "FTrace event from \"gpu_work_period\" %sd",
			        pszOperation));
		}
#endif

		if (value)
		{
			/* this enables FTrace globally (if not enabled nothing will appear
			 * in the FTrace buffer) */
			tracing_on();
		}

		/*  The HWPerf supplier is activated here,
		    The FTrace consumer is activated above,
		    The consumer should be active before the supplier    */
		if (_GpuTraceSetEnabledForAllDevices(value) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "FAILED to %s GPU FTrace for all devices",
			        pszOperation));
			goto err_restore_state;
		}

		PVR_DPF((PVR_DBG_MESSAGE, "%s GPU FTrace", value ? "ENABLED" : "DISABLED"));
		gbFTraceGPUEventsEnabled = value;
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "GPU FTrace already %s!",
		        value ? "enabled" : "disabled"));
	}

	OSLockRelease(ghGPUTraceStateLock);

	return PVRSRV_OK;

err_restore_state:
	/* On failure, partial enable/disable might have resulted. Try best to
	 * restore to previous state. Ignore error */
	(void) _GpuTraceSetEnabledForAllDevices(gbFTraceGPUEventsEnabled);

#if defined(PVRSRV_TRACE_ROGUE_EVENTS)
	(void) trace_set_clr_event("rogue", NULL,
	                           (int) gbFTraceGPUEventsEnabled);
#endif
#if defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
	(void) trace_set_clr_event("power", "gpu_work_period",
	                           (int) gbFTraceGPUEventsEnabled);
#endif

	OSLockRelease(ghGPUTraceStateLock);

	return PVRSRV_ERROR_UNABLE_TO_ENABLE_EVENT;
}

void PVRGpuTraceInitAppHintCallbacks(const PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_EnableFTraceGPU,
	                                  _GpuTraceIsEnabledCallback,
	                                  _GpuTraceSetEnabledCallback,
	                                  psDeviceNode, NULL);
}

/* ----- FTrace event callbacks -------------------------------------------- */

void PVRGpuTraceEnableUfoCallback(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_NODE *psDeviceNode;

	/* Lock down events state, for consistent value of guiUfoEventRef */
	OSLockAcquire(ghLockFTraceEventLock);
	if (guiUfoEventRef++ == 0)
	{
		OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
		psDeviceNode = psPVRSRVData->psDeviceNodeList;

		/* make sure UFO events are enabled on all rogue devices */
		while (psDeviceNode)
		{
#if defined(SUPPORT_RGX)
			PVRSRV_ERROR eError;

			/* Small chance exists that ui64HWPerfFilter can be changed here and
			 * the newest filter value will be changed to the old one + UFO event.
			 * This is not a critical problem. */
			eError = PVRSRVRGXCtrlHWPerfFW(psDeviceNode, RGX_HWPERF_L2_STREAM_FTRACE,
			                               RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_UFO),
			                               HWPERF_FILTER_OPERATION_BIT_OR);
			if (eError == PVRSRV_ERROR_NOT_INITIALISED)
			{
				/* If we land here that means that the FW is not initialised yet.
				 * We stored the filter and it will be passed to the firmware
				 * during its initialisation phase. So ignore. */
			}
			else if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "Could not enable UFO HWPerf events on device %d", psDeviceNode->sDevId.i32KernelDeviceID));
			}
#endif
			psDeviceNode = psDeviceNode->psNext;
		}

		OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);
	}
	OSLockRelease(ghLockFTraceEventLock);
}

void PVRGpuTraceDisableUfoCallback(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_NODE *psDeviceNode;

	/* We have to check if lock is valid because on driver unload
	 * PVRGpuTraceSupportDeInit is called before kernel disables the ftrace
	 * events. This means that the lock will be destroyed before this callback
	 * is called.
	 * We can safely return if that situation happens because driver will be
	 * unloaded so we don't care about HWPerf state anymore. */
	if (ghLockFTraceEventLock == NULL)
		return;

	/* Lock down events state, for consistent value of guiUfoEventRef */
	OSLockAcquire(ghLockFTraceEventLock);
	if (--guiUfoEventRef == 0)
	{
		OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
		psDeviceNode = psPVRSRVData->psDeviceNodeList;

		/* make sure UFO events are disabled on all rogue devices */
		while (psDeviceNode)
		{
#if defined(SUPPORT_RGX)
			PVRSRV_ERROR eError;

			/* Small chance exists that ui64HWPerfFilter can be changed here and
			 * the newest filter value will be changed to the old one + UFO event.
			 * This is not a critical problem. */
			eError = PVRSRVRGXCtrlHWPerfFW(psDeviceNode, RGX_HWPERF_L2_STREAM_FTRACE,
			                               RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_UFO),
			                               HWPERF_FILTER_OPERATION_BIT_CLR);
			if (eError == PVRSRV_ERROR_NOT_INITIALISED)
			{
				/* If we land here that means that the FW is not initialised yet.
				 * We stored the filter and it will be passed to the firmware
				 * during its initialisation phase. So ignore. */
			}
			else if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "Could not disable UFO HWPerf events on device %d",
				        psDeviceNode->sDevId.i32KernelDeviceID));
			}
#endif

			psDeviceNode = psDeviceNode->psNext;
		}

		OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);
	}
	OSLockRelease(ghLockFTraceEventLock);
}

void PVRGpuTraceEnableFirmwareActivityCallback(void)
{
#if defined(SUPPORT_RGX)
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_UINT64 uiFilter = 0;
	int i;

	for (i = RGX_HWPERF_FW_EVENT_RANGE_FIRST_TYPE;
		 i <= RGX_HWPERF_FW_EVENT_RANGE_LAST_TYPE; i++)
	{
		uiFilter |= RGX_HWPERF_EVENT_MASK_VALUE(i);
	}

	OSLockAcquire(ghLockFTraceEventLock);

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode = psPVRSRVData->psDeviceNodeList;
	/* Enable all FW events on all the devices */
	while (psDeviceNode)
	{
		PVRSRV_ERROR eError;

		eError = PVRSRVRGXCtrlHWPerfFW(psDeviceNode, RGX_HWPERF_L2_STREAM_FTRACE,
		                               uiFilter, HWPERF_FILTER_OPERATION_BIT_OR);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Could not enable HWPerf event for firmware"
			        " task timings (%s).", PVRSRVGetErrorString(eError)));
		}

		psDeviceNode = psDeviceNode->psNext;
	}

	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

	OSLockRelease(ghLockFTraceEventLock);
#endif /* defined(SUPPORT_RGX) */
}

void PVRGpuTraceDisableFirmwareActivityCallback(void)
{
#if defined(SUPPORT_RGX)
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_UINT64 uiFilter = 0;
	int i;

	/* We have to check if lock is valid because on driver unload
	 * PVRGpuTraceSupportDeInit is called before kernel disables the ftrace
	 * events. This means that the lock will be destroyed before this callback
	 * is called.
	 * We can safely return if that situation happens because driver will be
	 * unloaded so we don't care about HWPerf state anymore. */
	if (ghLockFTraceEventLock == NULL)
		return;

	OSLockAcquire(ghLockFTraceEventLock);

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode = psPVRSRVData->psDeviceNodeList;

	for (i = RGX_HWPERF_FW_EVENT_RANGE_FIRST_TYPE;
		 i <= RGX_HWPERF_FW_EVENT_RANGE_LAST_TYPE; i++)
	{
		uiFilter |= RGX_HWPERF_EVENT_MASK_VALUE(i);
	}

	/* Disable all FW events on all the devices */
	while (psDeviceNode)
	{
		if (PVRSRVRGXCtrlHWPerfFW(psDeviceNode, RGX_HWPERF_L2_STREAM_FTRACE,
		                          uiFilter, HWPERF_FILTER_OPERATION_BIT_CLR) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Could not disable HWPerf event for firmware task timings."));
		}
		psDeviceNode = psDeviceNode->psNext;
	}

	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

	OSLockRelease(ghLockFTraceEventLock);
#endif /* defined(SUPPORT_RGX) */
}

#if defined(PVRSRV_ANDROID_TRACE_GPU_WORK_PERIOD)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
int PVRGpuTraceEnableWorkPeriodCallback(void)
#else
void PVRGpuTraceEnableWorkPeriodCallback(void)
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)) */
{
	PVRSRV_ERROR eError = PVRSRV_ERROR_NOT_IMPLEMENTED;
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_NODE *psDeviceNode;

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode = psPVRSRVData->psDeviceNodeList;

	/* enable/disable GPU trace on all devices */
	while (psDeviceNode)
	{
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
		PVRSRV_RGXDEV_INFO *psRgxDevInfo = (PVRSRV_RGXDEV_INFO*)psDeviceNode->pvDevice;

		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psRgxDevInfo))
#endif
		{
			psDeviceNode->bGPUWorkPeriodFTraceEnabled = true;
			eError = PVRSRV_OK;
		}

		psDeviceNode = psDeviceNode->psNext;
	}

	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	if (eError != PVRSRV_OK)
		return -ENODEV;
	return 0;
#else
	return;
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)) */
}

void PVRGpuTraceDisableWorkPeriodCallback(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_NODE *psDeviceNode;

	if (!psPVRSRVData)
		return;

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode = psPVRSRVData->psDeviceNodeList;

	while (psDeviceNode)
	{
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
		PVRSRV_RGXDEV_INFO *psRgxDevInfo = (PVRSRV_RGXDEV_INFO*)psDeviceNode->pvDevice;

		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psRgxDevInfo))
#endif
		{
			psDeviceNode->bGPUWorkPeriodFTraceEnabled = false;
		}

		psDeviceNode = psDeviceNode->psNext;
	}

	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);
}
#endif
/******************************************************************************
 End of file (pvr_gputrace.c)
******************************************************************************/

/*************************************************************************/ /*!
@File
@Title          RGX debug header file
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Header for the RGX debugging functions
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

#if !defined(RGXDEBUG_COMMON_H)
#define RGXDEBUG_COMMON_H

#include "pvrsrv_error.h"
#include "img_types.h"
#include "device.h"
#include "pvr_notifier.h"
#include "pvrsrv.h"
#include "rgxdevice.h"
#include "rgxfwmemctx.h"

#define DD_NORMAL_INDENT   "    "

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
extern const IMG_CHAR * const gapszMipsPermissionPTFlags[4];
extern const IMG_CHAR * const gapszMipsCoherencyPTFlags[8];
extern const IMG_CHAR * const gapszMipsDirtyGlobalValidPTFlags[8];
#endif

/**
 * Debug utility macro for printing FW IRQ count and Last sampled IRQ count in
 * LISR for each RGX FW thread.
 * Macro takes pointer to PVRSRV_RGXDEV_INFO as input.
 */

#if defined(RGX_FW_IRQ_OS_COUNTERS)
#define for_each_irq_cnt(ui32idx)   FOREACH_SUPPORTED_DRIVER(ui32idx)

#define get_irq_cnt_val(ui32Dest, ui32idx, psRgxDevInfo) \
	do { \
		extern const IMG_UINT32 gaui32FwOsIrqCntRegAddr[RGXFW_MAX_NUM_OSIDS]; \
		ui32Dest = PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psRgxDevInfo) ? 0 : OSReadHWReg32((psRgxDevInfo)->pvRegsBaseKM, gaui32FwOsIrqCntRegAddr[ui32idx]); \
	} while (false)

#define MSG_IRQ_CNT_TYPE "OS"

#else

#define for_each_irq_cnt(ui32idx) \
	for (ui32idx = 0; ui32idx < RGXFW_THREAD_NUM; ui32idx++)

#define get_irq_cnt_val(ui32Dest, ui32idx, psRgxDevInfo) \
	do { \
		RGXFwSharedMemCacheOpValue(psRgxDevInfo->psRGXFWIfFwOsData->aui32InterruptCount[ui32idx], \
		                           INVALIDATE); \
		ui32Dest = (psRgxDevInfo)->psRGXFWIfFwOsData->aui32InterruptCount[ui32idx]; \
	} while (false)
#define MSG_IRQ_CNT_TYPE "Thread"
#endif /* RGX_FW_IRQ_OS_COUNTERS */

static inline void RGXDEBUG_PRINT_IRQ_COUNT(PVRSRV_RGXDEV_INFO* psRgxDevInfo)
{
#if defined(PVRSRV_NEED_PVR_DPF) && defined(DEBUG)
	IMG_UINT32 ui32idx;

	for_each_irq_cnt(ui32idx)
	{
		IMG_UINT32 ui32IrqCnt;

		get_irq_cnt_val(ui32IrqCnt, ui32idx, psRgxDevInfo);

		PVR_DPF((DBGPRIV_VERBOSE, MSG_IRQ_CNT_TYPE
		         " %u FW IRQ count = %u", ui32idx, ui32IrqCnt));

#if defined(RGX_FW_IRQ_OS_COUNTERS)
		if (ui32idx == RGXFW_HOST_DRIVER_ID)
#endif
		{
			PVR_DPF((DBGPRIV_VERBOSE, "Last sampled IRQ count in LISR = %u",
			        (psRgxDevInfo)->aui32SampleIRQCount[ui32idx]));
		}
	}
#else
	PVR_UNREFERENCED_PARAMETER(psRgxDevInfo);
#endif /* PVRSRV_NEED_PVR_DPF */
}

/*!
*******************************************************************************

 @Function	RGXDumpFirmwareTrace

 @Description Dumps the decoded version of the firmware trace buffer.

 Dump useful debugging info

 @Input pfnDumpDebugPrintf  - Optional replacement print function
 @Input pvDumpDebugFile     - Optional file identifier to be passed to the
                              'printf' function if required
 @Input psDevInfo           - RGX device info

 @Return   void

******************************************************************************/
void RGXDumpFirmwareTrace(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile,
				PVRSRV_RGXDEV_INFO  *psDevInfo);

void RGXDumpFirmwareTraceBinary(PVRSRV_RGXDEV_INFO *psDevInfo,
				DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile,
				RGXFWIF_TRACEBUF *psRGXFWIfTraceBufCtl,
				IMG_UINT32 ui32TID);

void RGXDumpFirmwareTracePartial(PVRSRV_RGXDEV_INFO *psDevInfo,
				DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile,
				RGXFWIF_TRACEBUF *psRGXFWIfTraceBufCtl,
				IMG_UINT32 ui32TID);

void RGXDumpFirmwareTraceDecoded(PVRSRV_RGXDEV_INFO *psDevInfo,
				DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				void *pvDumpDebugFile,
				RGXFWIF_TRACEBUF *psRGXFWIfTraceBufCtl,
				IMG_UINT32 ui32TID);


/* Helper macros to emit data */
#define REG32_FMTSPEC   "%-30s: 0x%08X"
#define REG64_FMTSPEC   "%-30s: 0x%016" IMG_UINT64_FMTSPECX
#define DDLOG32(R)      PVR_DUMPDEBUG_LOG(REG32_FMTSPEC, #R, OSReadHWReg32(pvRegsBaseKM, RGX_CR_##R));
#define DDLOG64(R)      PVR_DUMPDEBUG_LOG(REG64_FMTSPEC, #R, OSReadHWReg64(pvRegsBaseKM, RGX_CR_##R));
#define DDLOGUNCHECKED64(R)      PVR_DUMPDEBUG_LOG(REG64_FMTSPEC, #R, OSReadUncheckedHWReg64(pvRegsBaseKM, RGX_CR_##R));
#define DDLOG32_DPX(R)  PVR_DUMPDEBUG_LOG(REG32_FMTSPEC, #R, OSReadHWReg32(pvRegsBaseKM, DPX_CR_##R));
#define DDLOG64_DPX(R)  PVR_DUMPDEBUG_LOG(REG64_FMTSPEC, #R, OSReadHWReg64(pvRegsBaseKM, DPX_CR_##R));
#define DDLOGVAL32(S,V) PVR_DUMPDEBUG_LOG(REG32_FMTSPEC, S, V);

PVRSRV_ERROR RGXDumpRISCVState(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
									  void *pvDumpDebugFile,
									  PVRSRV_RGXDEV_INFO *psDevInfo);

void RGXDumpCoreRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo);

void RGXDumpMulticoreRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo);

void RGXDumpClkRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo);

void RGXDumpMMURegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo);

void RGXDumpDMRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo);

void RGXDumpSLCRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo);

void RGXDumpMiscRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo);

/*!
*******************************************************************************

 @Function	RGXDumpRGXRegisters

 @Description

 Dumps an extensive list of RGX registers required for debugging

 @Input pfnDumpDebugPrintf  - Optional replacement print function
 @Input pvDumpDebugFile     - Optional file identifier to be passed to the
                              'printf' function if required
 @Input psDevInfo           - RGX device info

 @Return PVRSRV_ERROR         PVRSRV_OK on success, error code otherwise

******************************************************************************/
PVRSRV_ERROR RGXDumpRGXRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo);

#if !defined(NO_HARDWARE)
/*!
*******************************************************************************

 @Function	RGXReadMetaCoreReg

 @Description  Read a META core register's value

 @Input psDevInfo      RGX device info
 @Input ui32RegAddr    Register address to read from
 @Output pui32RegVal   Pointer to the resulting register value

 @Return PVRSRV_ERROR  PVRSRV_OK on success, error code otherwise

******************************************************************************/
PVRSRV_ERROR RGXReadMetaCoreReg(PVRSRV_RGXDEV_INFO *psDevInfo,
                                IMG_UINT32 ui32RegAddr,
                                IMG_UINT32 *pui32RegVal);

/*!
*******************************************************************************

 @Function  RGXValidateFWImage

 @Description  Validate the currently running firmware
               against the firmware image

 @Input pfnDumpDebugPrintf   - The debug printf function
 @Input pvDumpDebugFile      - Optional file identifier to be passed to the
                               'printf' function if required
 @Input psDevInfo	         - RGX device info

 @Return PVRSRV_ERROR  PVRSRV_OK on success, error code otherwise

******************************************************************************/
PVRSRV_ERROR RGXValidateFWImage(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
						void *pvDumpDebugFile,
						PVRSRV_RGXDEV_INFO *psDevInfo);
#endif

#if defined(SUPPORT_FW_VIEW_EXTRA_DEBUG)
/*!
*******************************************************************************

 @Function     ValidateFWOnLoad

 @Description  Compare the Firmware image as seen from the CPU point of view
               against the same memory area as seen from the firmware point
               of view after first power up.

 @Input        psDevInfo - Device Info

 @Return       PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR ValidateFWOnLoad(PVRSRV_RGXDEV_INFO *psDevInfo);
#endif

/*!
*******************************************************************************

 @Function	RGXDumpRGXDebugSummary

 @Description

 Dump a summary in human readable form with the RGX state

 @Input pfnDumpDebugPrintf   - The debug printf function
 @Input pvDumpDebugFile      - Optional file identifier to be passed to the
                               'printf' function if required
 @Input psDevInfo	         - RGX device info
 @Input bRGXPoweredON        - IMG_TRUE if RGX device is on

 @Return   void

******************************************************************************/
void RGXDumpRGXDebugSummary(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					PVRSRV_RGXDEV_INFO *psDevInfo,
					IMG_BOOL bRGXPoweredON);

/*!
*******************************************************************************

 @Function RGXDebugInit

 @Description

 Setup debug requests, calls into PVRSRVRegisterDeviceDbgRequestNotify

 @Input          psDevInfo            RGX device info
 @Return         PVRSRV_ERROR         PVRSRV_OK on success otherwise an error

******************************************************************************/
PVRSRV_ERROR RGXDebugInit(PVRSRV_RGXDEV_INFO *psDevInfo);

/*!
*******************************************************************************

 @Function RGXDebugDeinit

 @Description

 Remove debug requests, calls into PVRSRVUnregisterDeviceDbgRequestNotify

 @Output         phNotify             Points to debug notifier handle
 @Return         PVRSRV_ERROR         PVRSRV_OK on success otherwise an error

******************************************************************************/
PVRSRV_ERROR RGXDebugDeinit(PVRSRV_RGXDEV_INFO *psDevInfo);

/*!
*******************************************************************************

 @Function       RGXGetFwMapping

 @Description    Retrieve any of the CPU Physical Address, Device Physical
                Address or the raw value of the page table entry associated
                with the firmware virtual address given.

 @Input          psDevInfo           Pointer to device info
 @Input          pfnDumpDebugPrintf  The debug printf function
 @Input          pvDumpDebugFile     Optional file identifier to be passed to
                                    the 'printf' function if required
 @Input          ui32FwVA            The Fw VA that needs decoding
 @Output         psCpuPA             Pointer to the resulting CPU PA
 @Output         psDevPA             Pointer to the resulting Dev PA
 @Output         pui64PTE            Pointer to the raw Page Table Entry value

 @Return         void

******************************************************************************/
void RGXDocumentFwMapping(PVRSRV_RGXDEV_INFO *psDevInfo,
                          DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                          void *pvDumpDebugFile,
                          const IMG_UINT32 ui32FwVA,
                          const IMG_CPU_PHYADDR sCpuPA,
                          const IMG_DEV_PHYADDR sDevPA,
                          const IMG_UINT64 ui64PTE);

/*!
*******************************************************************************

 @Function       RGXConvertOSTimestampToSAndNS

 @Description    Convert the OS time to seconds and nanoseconds

 @Input          ui64OSTimer       OS time to convert
 @Output         pui64Seconds      Pointer to the resulting seconds
 @Output         pui64Nanoseconds  Pointer to the resulting nanoseconds

 @Return         void

******************************************************************************/
void RGXConvertOSTimestampToSAndNS(IMG_UINT64 ui64OSTimer,
							IMG_UINT64 *pui64Seconds,
							IMG_UINT64 *pui64Nanoseconds);

/*!
*******************************************************************************

 @Function       RGXDumpAllContextInfo

 @Description    Dump debug info of all contexts on a device

 @Input          psDevInfo           Pointer to device info
 @Input          pfnDumpDebugPrintf  The debug printf function
 @Input          pvDumpDebugFile     Optional file identifier to be passed to
                                    the 'printf' function if required
 @Input          ui32VerbLevel       Verbosity level

 @Return         void

******************************************************************************/
void RGXDumpAllContextInfo(PVRSRV_RGXDEV_INFO *psDevInfo,
                           DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                           void *pvDumpDebugFile,
                           IMG_UINT32 ui32VerbLevel);

/*!
*******************************************************************************

 @Function	RGXDumpFaultAddressHostView

 @Description

 Dump FW HWR fault status in human readable form.

 @Input ui32Index            - Index of global Fault info
 @Input pfnDumpDebugPrintf   - The debug printf function
 @Input pvDumpDebugFile      - Optional file identifier to be passed to the
                               'printf' function if required
 @Return   void

******************************************************************************/
void RGXDumpFaultAddressHostView(MMU_FAULT_DATA *psFaultData,
					DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					const IMG_CHAR* pszIndent);

void RGXDumpFaultInfo(PVRSRV_RGXDEV_INFO *psDevInfo,
					DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					const RGX_HWRINFO *psHWRInfo,
					IMG_UINT32 ui32ReadIndex,
					IMG_DEV_VIRTADDR *psFaultDevVAddr,
					IMG_DEV_PHYADDR *psPCDevPAddr,
					bool bPMFault,
					IMG_UINT32 ui32PageSize);

#endif /* RGXDEBUG_COMMON_H */

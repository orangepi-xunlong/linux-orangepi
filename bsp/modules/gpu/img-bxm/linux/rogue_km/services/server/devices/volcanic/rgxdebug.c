/*************************************************************************/ /*!
@File
@Title          Rgx debug information
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX debugging functions
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
//#define PVR_DPF_FUNCTION_TRACE_ON 1
#undef PVR_DPF_FUNCTION_TRACE_ON

#include "img_defs.h"
#include "rgxdefs_km.h"
#include "rgxdevice.h"
#include "rgxmem.h"
#include "allocmem.h"
#include "cache_km.h"
#include "osfunc.h"
#include "os_apphint.h"

#include "rgxdebug_common.h"
#include "pvrversion.h"
#include "pvr_debug.h"
#include "srvkm.h"
#include "rgxutils.h"
#include "tlstream.h"
#include "rgxfwriscv.h"
#include "pvrsrv.h"
#include "services_km.h"

#include "devicemem.h"
#include "devicemem_pdump.h"
#include "devicemem_utils.h"
#include "rgx_fwif_km.h"
#include "rgx_fwif_sf.h"
#include "debug_common.h"

#include "rgxta3d.h"
#if defined(SUPPORT_RGXKICKSYNC_BRIDGE)
#include "rgxkicksync.h"
#endif
#include "rgxcompute.h"
#include "rgxtdmtransfer.h"
#include "rgxtimecorr.h"
#include "rgx_options.h"
#include "rgxinit.h"
#include "rgxlayer_impl.h"
#include "devicemem_history_server.h"

#define DD_SUMMARY_INDENT  ""

#define RGX_DEBUG_STR_SIZE			(150U)


#define RGX_TEXAS_BIF0_ID				(0)
#define RGX_TEXAS_BIF1_ID				(1)

/*
 *  The first 7 or 8 cat bases are memory contexts used for PM
 *  or firmware. The rest are application contexts. The numbering
 *  is zero-based.
 */
#if defined(SUPPORT_TRUSTED_DEVICE)
#define MAX_RESERVED_FW_MMU_CONTEXT		(7)
#else
#define MAX_RESERVED_FW_MMU_CONTEXT		(6)
#endif

static const IMG_CHAR *const pszPowStateName[] =
{
#define X(NAME)	#NAME,
	RGXFWIF_POW_STATES
#undef X
};

static const IMG_FLAGS2DESC asHwrState2Description[] =
{
	{RGXFWIF_HWR_HARDWARE_OK, " HWR OK;"},
	{RGXFWIF_HWR_RESET_IN_PROGRESS, " Reset ongoing;"},
	{RGXFWIF_HWR_GENERAL_LOCKUP, " General lockup;"},
	{RGXFWIF_HWR_DM_RUNNING_OK, " DM running ok;"},
	{RGXFWIF_HWR_DM_STALLING, " DM stalling;"},
	{RGXFWIF_HWR_FW_FAULT, " FW fault;"},
	{RGXFWIF_HWR_RESTART_REQUESTED, " Restart requested;"},
};

static const IMG_FLAGS2DESC asDmState2Description[] =
{
	{RGXFWIF_DM_STATE_READY_FOR_HWR, " ready for hwr;"},
	{RGXFWIF_DM_STATE_NEEDS_SKIP, " needs skip;"},
	{RGXFWIF_DM_STATE_NEEDS_PR_CLEANUP, " needs PR cleanup;"},
	{RGXFWIF_DM_STATE_NEEDS_TRACE_CLEAR, " needs trace clear;"},
	{RGXFWIF_DM_STATE_GUILTY_LOCKUP, " guilty lockup;"},
	{RGXFWIF_DM_STATE_INNOCENT_LOCKUP, " innocent lockup;"},
	{RGXFWIF_DM_STATE_GUILTY_OVERRUNING, " guilty overrunning;"},
	{RGXFWIF_DM_STATE_INNOCENT_OVERRUNING, " innocent overrunning;"},
	{RGXFWIF_DM_STATE_HARD_CONTEXT_SWITCH, " hard context switching;"},
	{RGXFWIF_DM_STATE_GPU_ECC_HWR, " GPU ECC hwr;"},
	{RGXFWIF_DM_STATE_GPU_PARITY_HWR, " GPU PARITY hwr;"},
	{RGXFWIF_DM_STATE_GPU_LATENT_HWR, " GPU LATENT hwr;"},
	{RGXFWIF_DM_STATE_ICS_HWR, " ICS hwr;"},
};

static const IMG_CHAR * const apszFwOsStateName[RGXFW_CONNECTION_FW_STATE_COUNT] =
{
	"offline",
	"ready",
	"active",
	"offloading",
	"cooldown"
};

#if defined(PVR_ENABLE_PHR)
static const IMG_FLAGS2DESC asPHRConfig2Description[] =
{
	{BIT_ULL(RGXFWIF_PHR_MODE_OFF), "off"},
	{BIT_ULL(RGXFWIF_PHR_MODE_RD_RESET), "reset RD hardware"},
};
#endif

/*!
*******************************************************************************

 @Function	_RGXDecodeMMULevel

 @Description

 Return the name for the MMU level that faulted.

 @Input ui32MMULevel	 - MMU level

 @Return   IMG_CHAR* to the string describing the MMU level that faulted.

******************************************************************************/
static const IMG_CHAR* _RGXDecodeMMULevel(IMG_UINT32 ui32MMULevel)
{
	const IMG_CHAR* pszMMULevel = "";

	switch (ui32MMULevel)
	{
		case 0x0: pszMMULevel = " (Page Table)"; break;
		case 0x1: pszMMULevel = " (Page Directory)"; break;
		case 0x2: pszMMULevel = " (Page Catalog)"; break;
		case 0x3: pszMMULevel = " (Cat Base Reg)"; break;
	}

	return pszMMULevel;
}


/*!
*******************************************************************************

 @Function	_RGXDecodeMMUReqTags

 @Description

 Decodes the MMU Tag ID and Sideband data fields from RGX_CR_MMU_FAULT_META_STATUS and
 RGX_CR_MMU_FAULT_STATUS regs.

 @Input psDevInfo           - RGX device info
 @Input ui32Requester       - Requester ID value
 @Input ui32BIFModule       - BIF module
 @Input bWriteBack          - Write Back flag
 @Input bFBMFault           - FBM Fault flag
 @Output ppszRequester      - Decoded string from the Requester ID

 @Return   void

******************************************************************************/
static void _RGXDecodeMMUReqTags(PVRSRV_RGXDEV_INFO  *psDevInfo,
								 IMG_UINT32          ui32Requester,
								 IMG_UINT32          ui32BIFModule,
								 IMG_BOOL            bWriteBack,
								 IMG_BOOL            bFBMFault,
								 IMG_CHAR            **ppszRequester)
{
	IMG_UINT32 ui32BIFsPerSPU = 2;
	IMG_CHAR   *pszRequester  = "-";

	PVR_ASSERT(ppszRequester != NULL);

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, ALBIORIX_TOP_INFRASTRUCTURE))
	{
		ui32BIFsPerSPU = 4;
	}

	if (bFBMFault)
	{
		if (bWriteBack)
		{
			pszRequester = "FBM (Header/state cache request)";
		}
		else
		{
			pszRequester = "FBM";
		}
	}
	else if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_SPU) &&
	         ui32BIFModule <  RGX_GET_FEATURE_VALUE(psDevInfo, NUM_SPU)*ui32BIFsPerSPU)
	{
		IMG_BOOL  bTexasBIFA = ((ui32BIFModule % 2) == 0);

		if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, ALBIORIX_TOP_INFRASTRUCTURE))
		{
			switch (ui32Requester)
			{
				case  0: pszRequester = "MCU L1 (IP0 PDS)"; break;
				case  1: pszRequester = "MCU L1 (IP0 Global)"; break;
				case  2: pszRequester = "MCU L1 (IP1 PDS)"; break;
				case  3: pszRequester = "MCU L1 (IP1 Global)"; break;
				case  4: pszRequester = "MCU L1 (IP2 PDS)"; break;
				case  5: pszRequester = "MCU L1 (IP2 Global)"; break;
				case  6: pszRequester = (bTexasBIFA ? "TCU L1" : "BSC"); break;
				case  7: pszRequester = (bTexasBIFA ? "PBE0" : "BSC"); break;
				case  8: pszRequester = "PBE0"; break;
				case  9: pszRequester = "IPF"; break;
				case 10: pszRequester = "IPF"; break;
				case 11: pszRequester = "IPF"; break;
				case 12: pszRequester = (bTexasBIFA ? "IPF" : "TCU L1"); break;
				case 13: pszRequester = (bTexasBIFA ? "IPF" : "TPF"); break;
				case 14: pszRequester = (bTexasBIFA ? "IPF" : "TPF CPF"); break;
				case 15: pszRequester = (bTexasBIFA ? "IPF" : "PBE1"); break;
				case 16: pszRequester = (bTexasBIFA ? "IPF" : "PBE1"); break;
				case 17: pszRequester = (bTexasBIFA ? "IPF" : "PDSRW cache"); break;
				case 18: pszRequester = (bTexasBIFA ? "IPF" : "PDS"); break;
				case 19: pszRequester = (bTexasBIFA ? "IPF" : "ISP1"); break;
				case 20: pszRequester = (bTexasBIFA ? "IPF" : "ISP1"); break;
				case 21: pszRequester = (bTexasBIFA ? "IPF" : "USC L2"); break;
				case 22: pszRequester = (bTexasBIFA ? "IPF" : "URI"); break;
				case 23: pszRequester = "IPF"; break;
				case 24: pszRequester = "IPF"; break;
				case 25: pszRequester = "IPF"; break;
				case 26: pszRequester = "IPF"; break;
				case 27: pszRequester = "IPF"; break;
				case 28: pszRequester = "IPF"; break;
				case 29: pszRequester = "IPF"; break;
				case 30: pszRequester = "IPF"; break;
				case 31: pszRequester = "IPF"; break;
				case 32: pszRequester = "IPF"; break;
				case 33: pszRequester = "IPF_CPF"; break;
				case 34: pszRequester = "ISP0"; break;
				case 35: pszRequester = "ISP0"; break;
				case 36: pszRequester = "ISP2"; break;
				case 37: pszRequester = "ISP2"; break;
				case 38: pszRequester = "GEOM"; break;
				case 39: pszRequester = "GEOM"; break;
				case 40: pszRequester = "GEOM"; break;
				case 41: pszRequester = "GEOM"; break;
				case 42: pszRequester = "BSC"; break;
				case 43: pszRequester = "ASC"; break;

				default:
					PVR_DPF((PVR_DBG_WARNING, "%s: Unidentified Texas BIF Requester ID: %d", __func__, ui32Requester));
				break;
			}
		}
		else
		{
			switch (ui32Requester)
			{
				case  0: pszRequester = "MCU L1 (IP0 PDS)"; break;
				case  1: pszRequester = "MCU L1 (IP0 Global)"; break;
				case  2: pszRequester = "MCU L1 (IP0 BSC)"; break;
				case  3: pszRequester = "MCU L1 (IP0 Constants)"; break;
				case  4: pszRequester = "MCU L1 (IP1 PDS)"; break;
				case  5: pszRequester = "MCU L1 (IP1 Global)"; break;
				case  6: pszRequester = "MCU L1 (IP1 BSC)"; break;
				case  7: pszRequester = "MCU L1 (IP1 Constants)"; break;
				case  8: pszRequester = "MCU L1 (IP2 PDS)"; break;
				case  9: pszRequester = "MCU L1 (IP2 Global)"; break;
				case 10: pszRequester = "MCU L1 (IP2 BSC)"; break;
				case 11: pszRequester = "MCU L1 (IP2 Constants)"; break;
				case 12: pszRequester = "TCU L1"; break;
				case 13: pszRequester = (bTexasBIFA ? "PBE0" : "TPF"); break;
				case 14: pszRequester = (bTexasBIFA ? "PBE0" : "TPF CPF"); break;
				case 15: pszRequester = (bTexasBIFA ? "IPF" : "PBE1"); break;
				case 16: pszRequester = (bTexasBIFA ? "IPF" : "PBE1"); break;
				case 17: pszRequester = (bTexasBIFA ? "IPF" : "PDSRW cache"); break;
				case 18: pszRequester = (bTexasBIFA ? "IPF" : "PDS"); break;
				case 19: pszRequester = (bTexasBIFA ? "IPF" : "ISP1"); break;
				case 20: pszRequester = (bTexasBIFA ? "IPF" : "ISP1"); break;
				case 21: pszRequester = (bTexasBIFA ? "IPF" : "USC L2"); break;
				case 22: pszRequester = (bTexasBIFA ? "IPF" : "VDM L2"); break;
				case 23: pszRequester = (bTexasBIFA ? "IPF" : "RTU FBA L2"); break;
				case 24: pszRequester = (bTexasBIFA ? "IPF" : "RTU SHR L2"); break;
				case 25: pszRequester = (bTexasBIFA ? "IPF" : "RTU SHG L2"); break;
				case 26: pszRequester = (bTexasBIFA ? "IPF" : "RTU TUL L2"); break;
				case 27: pszRequester = "IPF"; break;
				case 28: pszRequester = "IPF"; break;
				case 29: pszRequester = "IPF"; break;
				case 30: pszRequester = "IPF"; break;
				case 31: pszRequester = "IPF"; break;
				case 32: pszRequester = "IPF"; break;
				case 33: pszRequester = "IPF"; break;
				case 34: pszRequester = "IPF_CPF"; break;
				case 35: pszRequester = "PPP"; break;
				case 36: pszRequester = "ISP0"; break;
				case 37: pszRequester = "ISP0"; break;
				case 38: pszRequester = "ISP2"; break;
				case 39: pszRequester = "ISP2"; break;
				case 40: pszRequester = "VCE RTC"; break;
				case 41: pszRequester = "RTU RAC"; break;
				case 42: pszRequester = "RTU RAC"; break;
				case 43: pszRequester = "RTU RAC"; break;
				case 44: pszRequester = "RTU RAC"; break;
				case 45: pszRequester = "RTU RAC"; break;
				case 46: pszRequester = "RTU RAC"; break;
				case 47: pszRequester = "RTU RAC"; break;
				case 48: pszRequester = "RTU RAC"; break;
				case 49: pszRequester = "VCE AMC"; break;
				case 50: pszRequester = "SHF"; break;
				case 51: pszRequester = "SHF"; break;

				default:
					PVR_DPF((PVR_DBG_WARNING, "%s: Unidentified Texas BIF Requester ID: %d", __func__, ui32Requester));
				break;
			}
		}
	}
	else if (ui32BIFModule == RGX_GET_FEATURE_VALUE(psDevInfo, NUM_SPU)*ui32BIFsPerSPU)
	{
		/* Jones BIF */
		switch (ui32Requester)
		{
			case  0: pszRequester = "IPP"; break;
			case  1: pszRequester = "DCE"; break;
			case  2: pszRequester = "DCE"; break;
			case  3: pszRequester = "DCE"; break;
			case  4: pszRequester = "DCE"; break;
			case  5: pszRequester = "DCE"; break;
			case  6: pszRequester = "DCE"; break;
			case  7: pszRequester = "DCE"; break;
			case  8: pszRequester = "DCE"; break;
			case  9: pszRequester = "DCE"; break;
			case 10: pszRequester = "DCE"; break;
			case 11: pszRequester = "DCE"; break;
			case 12: pszRequester = "DCE"; break;
			case 13: pszRequester = "DCE"; break;
			case 14: pszRequester = "DCE"; break;
			case 15: pszRequester = "TDM"; break;
			case 16: pszRequester = "TDM"; break;
			case 17: pszRequester = "TDM"; break;
			case 18: pszRequester = "TDM"; break;
			case 19: pszRequester = "TDM"; break;
			case 20: pszRequester = "PM"; break;
			case 21: pszRequester = "CDM"; break;
			case 22: pszRequester = "CDM"; break;
			case 23: pszRequester = "CDM"; break;
			case 24: pszRequester = "CDM"; break;
			case 25: pszRequester = "CDM"; break;
			case 26: pszRequester = "CDM"; break;
			case 27: pszRequester = "CDM"; break;
			case 28: pszRequester = "CDM"; break;
			case 29: pszRequester = "CDM"; break;
			case 30: pszRequester = "CDM"; break;
			case 31: pszRequester = "CDM"; break;
			case 32: pszRequester = "META"; break;
			case 33: pszRequester = "META DMA"; break;
			case 34: pszRequester = "TE"; break;
			case 35: pszRequester = "TE"; break;
			case 36: pszRequester = "TE"; break;
			case 37: pszRequester = "TE"; break;
			case 38: pszRequester = "TE"; break;
			case 39: pszRequester = "TE"; break;
			case 40: pszRequester = "TE"; break;
			case 41: pszRequester = "TE"; break;
			case 42: pszRequester = "TE"; break;
			case 43: pszRequester = "TE"; break;
			case 44: pszRequester = "TE"; break;
			case 45: pszRequester = "TE"; break;
			case 46: pszRequester = "TE"; break;
			case 47: pszRequester = "TE"; break;
			case 48: pszRequester = "TE"; break;
			case 49: pszRequester = "TE"; break;
			case 50: pszRequester = "RCE"; break;
			case 51: pszRequester = "RCE"; break;
			case 52: pszRequester = "RCE"; break;

			default:
				PVR_DPF((PVR_DBG_WARNING, "%s: Unidentified Jones BIF Tag ID: %d", __func__, ui32Requester));
			break;
		}
	}
	else if (bWriteBack)
	{
		pszRequester = "Unknown (Writeback of dirty cacheline)";
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Unidentified BIF Module: %d", __func__, ui32BIFModule));
	}

	*ppszRequester = pszRequester;
}


/*!
*******************************************************************************

 @Function	_RGXDumpRGXMMUFaultStatus

 @Description

 Dump MMU Fault status in human readable form.

 @Input pfnDumpDebugPrintf   - The debug printf function
 @Input pvDumpDebugFile      - Optional file identifier to be passed to the
                               'printf' function if required
 @Input psDevInfo            - RGX device info
 @Input ui64MMUStatus        - MMU Status register value
 @Input pszMetaOrCore        - string representing call is for META or MMU core
 @Return   void

******************************************************************************/
static void _RGXDumpRGXMMUFaultStatus(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					PVRSRV_RGXDEV_INFO *psDevInfo,
					const IMG_UINT64 aui64MMUStatus[],
					const IMG_PCHAR pszMetaOrCore,
					const IMG_CHAR *pszIndent)
{
	if (aui64MMUStatus[0] == 0x0)
	{
		PVR_DUMPDEBUG_LOG("%sMMU (%s) - OK", pszIndent, pszMetaOrCore);
	}
	else
	{
		IMG_UINT32 ui32PC        = (aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS1_CONTEXT_CLRMSK) >>
		                           RGX_CR_MMU_FAULT_STATUS1_CONTEXT_SHIFT;
		IMG_UINT64 ui64Addr      = ((aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS1_ADDRESS_CLRMSK) >>
		                           RGX_CR_MMU_FAULT_STATUS1_ADDRESS_SHIFT) <<  4; /* align shift */
		IMG_UINT32 ui32Requester = (aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS1_REQ_ID_CLRMSK) >>
		                           RGX_CR_MMU_FAULT_STATUS1_REQ_ID_SHIFT;
		IMG_UINT32 ui32MMULevel  = (aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS1_LEVEL_CLRMSK) >>
		                           RGX_CR_MMU_FAULT_STATUS1_LEVEL_SHIFT;
		IMG_BOOL bRead           = (aui64MMUStatus[0] & RGX_CR_MMU_FAULT_STATUS1_RNW_EN) != 0;
		IMG_BOOL bFault          = (aui64MMUStatus[0] & RGX_CR_MMU_FAULT_STATUS1_FAULT_EN) != 0;
		IMG_BOOL bROFault        = ((aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS1_TYPE_CLRMSK) >>
		                            RGX_CR_MMU_FAULT_STATUS1_TYPE_SHIFT) == 0x2;
		IMG_BOOL bProtFault      = ((aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS1_TYPE_CLRMSK) >>
		                            RGX_CR_MMU_FAULT_STATUS1_TYPE_SHIFT) == 0x3;
		IMG_UINT32 ui32BIFModule;
		IMG_BOOL bWriteBack, bFBMFault;
		IMG_CHAR *pszRequester = NULL;
		const IMG_PCHAR pszMetaOrRiscv = RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META) ? "META" : "RISCV";

		if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, ALBIORIX_TOP_INFRASTRUCTURE))
		{
			ui32BIFModule = (aui64MMUStatus[1] & ~RGX_CR_MMU_FAULT_STATUS2__AXT_INFRA__BIF_ID_CLRMSK) >>
										RGX_CR_MMU_FAULT_STATUS2__AXT_INFRA__BIF_ID_SHIFT;
			bWriteBack    = (aui64MMUStatus[1] & RGX_CR_MMU_FAULT_STATUS2__AXT_INFRA__WRITEBACK_EN) != 0;
			bFBMFault     = (aui64MMUStatus[1] & RGX_CR_MMU_FAULT_STATUS2__AXT_INFRA__FBM_FAULT_EN) != 0;
		}
		else
		{
			ui32BIFModule = (aui64MMUStatus[1] & ~RGX_CR_MMU_FAULT_STATUS2_BIF_ID_CLRMSK) >>
										RGX_CR_MMU_FAULT_STATUS2_BIF_ID_SHIFT;
			bWriteBack    = (aui64MMUStatus[1] & RGX_CR_MMU_FAULT_STATUS2_WRITEBACK_EN) != 0;
			bFBMFault     = (aui64MMUStatus[1] & RGX_CR_MMU_FAULT_STATUS2_FBM_FAULT_EN) != 0;
		}

		if (strcmp(pszMetaOrCore, "Core") != 0)
		{
			ui32PC		= (aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS_META_CONTEXT_CLRMSK) >>
								RGX_CR_MMU_FAULT_STATUS_META_CONTEXT_SHIFT;
			ui64Addr	= ((aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS_META_ADDRESS_CLRMSK) >>
								RGX_CR_MMU_FAULT_STATUS_META_ADDRESS_SHIFT) <<  4; /* align shift */
			ui32Requester = (aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS_META_REQ_ID_CLRMSK) >>
								RGX_CR_MMU_FAULT_STATUS_META_REQ_ID_SHIFT;
			ui32MMULevel  = (aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS_META_LEVEL_CLRMSK) >>
								RGX_CR_MMU_FAULT_STATUS_META_LEVEL_SHIFT;
			bRead		= (aui64MMUStatus[0] & RGX_CR_MMU_FAULT_STATUS_META_RNW_EN) != 0;
			bFault      = (aui64MMUStatus[0] & RGX_CR_MMU_FAULT_STATUS_META_FAULT_EN) != 0;
			bROFault    = ((aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS_META_TYPE_CLRMSK) >>
								RGX_CR_MMU_FAULT_STATUS_META_TYPE_SHIFT) == 0x2;
			bProtFault  = ((aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS_META_TYPE_CLRMSK) >>
								RGX_CR_MMU_FAULT_STATUS_META_TYPE_SHIFT) == 0x3;
		}
		else
		{
			_RGXDecodeMMUReqTags(psDevInfo, ui32Requester, ui32BIFModule, bWriteBack, bFBMFault, &pszRequester);
		}

		PVR_DUMPDEBUG_LOG("%sMMU (%s) - FAULT:", pszIndent, pszMetaOrCore);
		PVR_DUMPDEBUG_LOG("%s  * MMU status (0x%016" IMG_UINT64_FMTSPECX " | 0x%08" IMG_UINT64_FMTSPECX "): PC = %d, %s 0x%010" IMG_UINT64_FMTSPECX ", %s%s%s%s%s.",
						  pszIndent,
						  aui64MMUStatus[0],
						  aui64MMUStatus[1],
						  ui32PC,
						  (bRead)?"Reading from":"Writing to",
						  ui64Addr,
						  (pszRequester) ? pszRequester : pszMetaOrRiscv,
						  (bFault)?", Fault":"",
						  (bROFault)?", Read Only fault":"",
						  (bProtFault)?", PM/FW core protection fault":"",
						  _RGXDecodeMMULevel(ui32MMULevel));

	}
}

static_assert((RGX_CR_MMU_FAULT_STATUS1_CONTEXT_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_CONTEXT_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_CONTEXT_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_CONTEXT_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_ADDRESS_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_ADDRESS_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_ADDRESS_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_ADDRESS_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_REQ_ID_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_REQ_ID_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_REQ_ID_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_REQ_ID_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_LEVEL_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_LEVEL_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_LEVEL_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_LEVEL_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_RNW_EN == RGX_CR_MMU_FAULT_STATUS_META_RNW_EN),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_FAULT_EN == RGX_CR_MMU_FAULT_STATUS_META_FAULT_EN),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_TYPE_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_TYPE_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS1_TYPE_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_TYPE_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");


#if !defined(NO_HARDWARE)
static inline IMG_CHAR const *_GetRISCVException(IMG_UINT32 ui32Mcause)
{
	switch (ui32Mcause)
	{
#define X(value, fatal, description) \
		case value: \
			if (fatal) \
				return description; \
			return NULL;

		RGXRISCVFW_MCAUSE_TABLE
#undef X

		default:
			PVR_DPF((PVR_DBG_WARNING, "Invalid RISC-V FW mcause value 0x%08x", ui32Mcause));
			return NULL;
	}
}
#endif /* !defined(NO_HARDWARE) */


static const IMG_FLAGS2DESC asHWErrorState[] =
{
	{RGX_HW_ERR_NA, "N/A"},
	{RGX_HW_ERR_PRIMID_FAILURE_DURING_DMKILL, "Primitive ID failure during DM kill."},
};

/*
 *  Translate ID code to descriptive string.
 *  Returns on the first match.
 */
static void _ID2Description(IMG_CHAR *psDesc, IMG_UINT32 ui32DescSize, const IMG_FLAGS2DESC *psConvTable, IMG_UINT32 ui32TableSize, IMG_UINT32 ui32ID)
{
	IMG_UINT32 ui32Idx;

	for (ui32Idx = 0; ui32Idx < ui32TableSize; ui32Idx++)
	{
		if (ui32ID == psConvTable[ui32Idx].uiFlag)
		{
			OSStringSafeCopy(psDesc, psConvTable[ui32Idx].pszLabel, ui32DescSize);
			return;
		}
	}
}


/*!
*******************************************************************************

 @Function	_RGXDumpFWAssert

 @Description

 Dump FW assert strings when a thread asserts.

 @Input pfnDumpDebugPrintf   - The debug printf function
 @Input pvDumpDebugFile      - Optional file identifier to be passed to the
                               'printf' function if required
 @Input psRGXFWIfTraceBufCtl - RGX FW trace buffer

 @Return   void

******************************************************************************/
static void _RGXDumpFWAssert(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					const RGXFWIF_TRACEBUF *psRGXFWIfTraceBufCtl)
{
	const IMG_CHAR *pszTraceAssertPath;
	const IMG_CHAR *pszTraceAssertInfo;
	IMG_INT32 ui32TraceAssertLine;
	IMG_UINT32 i;

	for (i = 0; i < RGXFW_THREAD_NUM; i++)
	{
		RGXFwSharedMemCacheOpValue(psRGXFWIfTraceBufCtl->sTraceBuf[i].sAssertBuf, INVALIDATE);
		pszTraceAssertPath = psRGXFWIfTraceBufCtl->sTraceBuf[i].sAssertBuf.szPath;
		pszTraceAssertInfo = psRGXFWIfTraceBufCtl->sTraceBuf[i].sAssertBuf.szInfo;
		ui32TraceAssertLine = psRGXFWIfTraceBufCtl->sTraceBuf[i].sAssertBuf.ui32LineNum;

		/* print non-null assert strings */
		if (*pszTraceAssertInfo)
		{
			PVR_DUMPDEBUG_LOG("FW-T%d Assert: %.*s (%.*s:%d)",
			                  i, RGXFW_TRACE_BUFFER_ASSERT_SIZE, pszTraceAssertInfo,
			                  RGXFW_TRACE_BUFFER_ASSERT_SIZE, pszTraceAssertPath, ui32TraceAssertLine);
		}
	}
}

/*!
*******************************************************************************

 @Function	_RGXDumpFWFaults

 @Description

 Dump FW assert strings when a thread asserts.

 @Input pfnDumpDebugPrintf   - The debug printf function
 @Input pvDumpDebugFile      - Optional file identifier to be passed to the
                               'printf' function if required
 @Input psFwSysData       - RGX FW shared system data

 @Return   void

******************************************************************************/
static void _RGXDumpFWFaults(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                             void *pvDumpDebugFile,
                             const RGXFWIF_SYSDATA *psFwSysData)
{
	if (psFwSysData->ui32FWFaults > 0)
	{
		IMG_UINT32	ui32StartFault = psFwSysData->ui32FWFaults - RGXFWIF_FWFAULTINFO_MAX;
		IMG_UINT32	ui32EndFault   = psFwSysData->ui32FWFaults - 1;
		IMG_UINT32  ui32Index;

		if (psFwSysData->ui32FWFaults < RGXFWIF_FWFAULTINFO_MAX)
		{
			ui32StartFault = 0;
		}

		for (ui32Index = ui32StartFault; ui32Index <= ui32EndFault; ui32Index++)
		{
			const RGX_FWFAULTINFO *psFaultInfo = &psFwSysData->sFaultInfo[ui32Index % RGXFWIF_FWFAULTINFO_MAX];
			IMG_UINT64 ui64Seconds, ui64Nanoseconds;

			/* Split OS timestamp in seconds and nanoseconds */
			RGXConvertOSTimestampToSAndNS(psFaultInfo->ui64OSTimer, &ui64Seconds, &ui64Nanoseconds);

			PVR_DUMPDEBUG_LOG("FW Fault %d: %.*s (%.*s:%d)",
			                  ui32Index+1, RGXFW_TRACE_BUFFER_ASSERT_SIZE, psFaultInfo->sFaultBuf.szInfo,
			                  RGXFW_TRACE_BUFFER_ASSERT_SIZE, psFaultInfo->sFaultBuf.szPath,
			                  psFaultInfo->sFaultBuf.ui32LineNum);
			PVR_DUMPDEBUG_LOG("            Data = 0x%016"IMG_UINT64_FMTSPECx", CRTimer = 0x%012"IMG_UINT64_FMTSPECx", OSTimer = %" IMG_UINT64_FMTSPEC ".%09" IMG_UINT64_FMTSPEC,
			                  psFaultInfo->ui64Data,
			                  psFaultInfo->ui64CRTimer,
			                  ui64Seconds, ui64Nanoseconds);
		}
	}
}

static void _RGXDumpFWPoll(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					const RGXFWIF_SYSDATA *psFwSysData)
{
	IMG_UINT32 i;

	for (i = 0; i < RGXFW_THREAD_NUM; i++)
	{
		if (psFwSysData->aui32CrPollAddr[i])
		{
			PVR_DUMPDEBUG_LOG("T%u polling %s (reg:0x%08X mask:0x%08X)",
			                  i,
			                  ((psFwSysData->aui32CrPollAddr[i] & RGXFW_POLL_TYPE_SET)?("set"):("unset")),
			                  psFwSysData->aui32CrPollAddr[i] & ~RGXFW_POLL_TYPE_SET,
			                  psFwSysData->aui32CrPollMask[i]);
		}
	}

}

static void _RGXDumpFWHWRInfo(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
							  void *pvDumpDebugFile,
							  const RGXFWIF_SYSDATA *psFwSysData,
							  const RGXFWIF_HWRINFOBUF *psHWRInfoBuf,
							  PVRSRV_RGXDEV_INFO *psDevInfo)
{
	IMG_BOOL          bAnyLocked = IMG_FALSE;
	IMG_UINT32        dm, i;
	IMG_UINT32        ui32LineSize;
	IMG_CHAR          *pszLine, *pszTemp;
	const IMG_CHAR    *apszDmNames[RGXFWIF_DM_MAX] = {"GP", "TDM", "GEOM", "3D", "CDM", "RAY", "GEOM2", "GEOM3", "GEOM4"};
	const IMG_CHAR    szMsgHeader[] = "Number of HWR: ";
	const IMG_CHAR    szMsgFalse[] = "FALSE(";
	IMG_CHAR          *pszLockupType = "";
	const IMG_UINT32  ui32MsgHeaderCharCount = ARRAY_SIZE(szMsgHeader) - 1; /* size includes the null */
	const IMG_UINT32  ui32MsgFalseCharCount = ARRAY_SIZE(szMsgFalse) - 1;
	IMG_UINT32        ui32HWRRecoveryFlags;
	IMG_UINT32        ui32ReadIndex;

	if (!(RGX_IS_FEATURE_SUPPORTED(psDevInfo, FASTRENDER_DM)))
	{
		apszDmNames[RGXFWIF_DM_TDM] = "2D";
	}

	for (dm = 0; dm < psDevInfo->sDevFeatureCfg.ui32MAXDMCount; dm++)
	{
		if (psHWRInfoBuf->aui32HwrDmLockedUpCount[dm] ||
		    psHWRInfoBuf->aui32HwrDmOverranCount[dm])
		{
			bAnyLocked = IMG_TRUE;
			break;
		}
	}

	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo) && !bAnyLocked && (psFwSysData->ui32HWRStateFlags & RGXFWIF_HWR_HARDWARE_OK))
	{
		/* No HWR situation, print nothing */
		return;
	}

	if (PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		IMG_BOOL bAnyHWROccurred = IMG_FALSE;

		for (dm = 0; dm < psDevInfo->sDevFeatureCfg.ui32MAXDMCount; dm++)
		{
			if (psHWRInfoBuf->aui32HwrDmRecoveredCount[dm] != 0 ||
				psHWRInfoBuf->aui32HwrDmLockedUpCount[dm] != 0 ||
				psHWRInfoBuf->aui32HwrDmOverranCount[dm] !=0)
				{
					bAnyHWROccurred = IMG_TRUE;
					break;
				}
		}

		if (!bAnyHWROccurred)
		{
			return;
		}
	}

/* <DM name + left parenthesis> + <UINT32 max num of digits> + <slash> + <UINT32 max num of digits> +
   <plus> + <UINT32 max num of digits> + <right parenthesis + comma + space> */
#define FWHWRINFO_DM_STR_SIZE (5U + 10U + 1U + 10U + 1U + 10U + 3U)

	ui32LineSize = sizeof(IMG_CHAR) * (
			ui32MsgHeaderCharCount +
			(psDevInfo->sDevFeatureCfg.ui32MAXDMCount * FWHWRINFO_DM_STR_SIZE) +
			ui32MsgFalseCharCount + 1 + (psDevInfo->sDevFeatureCfg.ui32MAXDMCount*6) + 1
				/* 'FALSE(' + ')' + (UINT16 max num + comma) per DM + \0 */
			);

	pszLine = OSAllocMem(ui32LineSize);
	if (pszLine == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,
			"%s: Out of mem allocating line string (size: %d)",
			__func__,
			ui32LineSize));
		return;
	}

	OSStringSafeCopy(pszLine, szMsgHeader, ui32LineSize);
	pszTemp = pszLine + ui32MsgHeaderCharCount;

	for (dm = 0; dm < psDevInfo->sDevFeatureCfg.ui32MAXDMCount; dm++)
	{
		pszTemp += OSSNPrintf(pszTemp,
				FWHWRINFO_DM_STR_SIZE,
				"%s(%u/%u+%u), ",
				apszDmNames[dm],
				psHWRInfoBuf->aui32HwrDmRecoveredCount[dm],
				psHWRInfoBuf->aui32HwrDmLockedUpCount[dm],
				psHWRInfoBuf->aui32HwrDmOverranCount[dm]);
	}

	OSStringLCat(pszLine, szMsgFalse, ui32LineSize);
	pszTemp += ui32MsgFalseCharCount;

	for (dm = 0; dm < psDevInfo->sDevFeatureCfg.ui32MAXDMCount; dm++)
	{
		pszTemp += OSSNPrintf(pszTemp,
				10 + 1 + 1 /* UINT32 max num + comma + \0 */,
				(dm < psDevInfo->sDevFeatureCfg.ui32MAXDMCount-1 ? "%u," : "%u)"),
				psHWRInfoBuf->aui32HwrDmFalseDetectCount[dm]);
	}

	PVR_DUMPDEBUG_LOG("%s", pszLine);

	OSFreeMem(pszLine);

	/* Print out per HWR info */
	for (dm = 0; dm < psDevInfo->sDevFeatureCfg.ui32MAXDMCount; dm++)
	{
		if (dm == RGXFWIF_DM_GP)
		{
			PVR_DUMPDEBUG_LOG("DM %d (GP)", dm);
		}
		else
		{
			if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
			{
				IMG_UINT32 ui32HWRRecoveryFlags = psFwSysData->aui32HWRRecoveryFlags[dm];
				IMG_CHAR sPerDmHwrDescription[RGX_DEBUG_STR_SIZE];
				sPerDmHwrDescription[0] = '\0';

				if (ui32HWRRecoveryFlags == RGXFWIF_DM_STATE_WORKING)
				{
					OSStringSafeCopy(sPerDmHwrDescription, " working;", RGX_DEBUG_STR_SIZE);
				}
				else
				{
					DebugCommonFlagStrings(sPerDmHwrDescription, RGX_DEBUG_STR_SIZE,
						asDmState2Description, ARRAY_SIZE(asDmState2Description),
						ui32HWRRecoveryFlags);
				}
				PVR_DUMPDEBUG_LOG("DM %d (HWRflags 0x%08x:%.*s)", dm, ui32HWRRecoveryFlags,
								  RGX_DEBUG_STR_SIZE, sPerDmHwrDescription);
			}
			else
			{
				PVR_DUMPDEBUG_LOG("DM %d", dm);
			}
		}

		ui32ReadIndex = 0;
		for (i = 0 ; i < RGXFWIF_HWINFO_MAX ; i++)
		{
			IMG_BOOL bPMFault = IMG_FALSE;
			IMG_UINT32 ui32PC;
			IMG_UINT32 ui32PageSize = 0;
			IMG_DEV_PHYADDR sPCDevPAddr = { 0 };
			const RGX_HWRINFO *psHWRInfo = &psHWRInfoBuf->sHWRInfo[ui32ReadIndex];

			if (ui32ReadIndex >= RGXFWIF_HWINFO_MAX)
			{
				PVR_DUMPDEBUG_LOG("HWINFO index error: %u", ui32ReadIndex);
				break;
			}

			if ((psHWRInfo->eDM == dm) && (psHWRInfo->ui32HWRNumber != 0))
			{
				IMG_CHAR aui8RecoveryNum[10+10+1];
				IMG_UINT64 ui64Seconds, ui64Nanoseconds;
				IMG_BOOL bPageFault = IMG_FALSE;
				IMG_DEV_VIRTADDR sFaultDevVAddr;

				/* Split OS timestamp in seconds and nanoseconds */
				RGXConvertOSTimestampToSAndNS(psHWRInfo->ui64OSTimer, &ui64Seconds, &ui64Nanoseconds);

				ui32HWRRecoveryFlags = psHWRInfo->ui32HWRRecoveryFlags;
				if (ui32HWRRecoveryFlags & RGXFWIF_DM_STATE_GUILTY_LOCKUP) { pszLockupType = ", Guilty Lockup"; }
				else if (ui32HWRRecoveryFlags & RGXFWIF_DM_STATE_INNOCENT_LOCKUP) { pszLockupType = ", Innocent Lockup"; }
				else if (ui32HWRRecoveryFlags & RGXFWIF_DM_STATE_GUILTY_OVERRUNING) { pszLockupType = ", Guilty Overrun"; }
				else if (ui32HWRRecoveryFlags & RGXFWIF_DM_STATE_INNOCENT_OVERRUNING) { pszLockupType = ", Innocent Overrun"; }
				else if (ui32HWRRecoveryFlags & RGXFWIF_DM_STATE_HARD_CONTEXT_SWITCH) { pszLockupType = ", Hard Context Switch"; }
				else if (ui32HWRRecoveryFlags & RGXFWIF_DM_STATE_GPU_ECC_HWR) { pszLockupType = ", GPU ECC HWR"; }
				else if (ui32HWRRecoveryFlags & RGXFWIF_DM_STATE_GPU_PARITY_HWR) { pszLockupType = ", GPU PARITY HWR"; }
				else if (ui32HWRRecoveryFlags & RGXFWIF_DM_STATE_GPU_LATENT_HWR) { pszLockupType = ", GPU LATENT HWR"; }
				else if (ui32HWRRecoveryFlags & RGXFWIF_DM_STATE_ICS_HWR) { pszLockupType = ", ICS HWR"; }

				OSSNPrintf(aui8RecoveryNum, sizeof(aui8RecoveryNum), "Recovery %d:", psHWRInfo->ui32HWRNumber);
				if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, GPU_MULTICORE_SUPPORT))
				{
					PVR_DUMPDEBUG_LOG("  %s Core = %u, PID = %u / %.*s, frame = %d, HWRTData = 0x%08X, EventStatus = 0x%08X%s",
				                   aui8RecoveryNum,
				                   psHWRInfo->ui32CoreID,
				                   psHWRInfo->ui32PID,
				                   RGXFW_PROCESS_NAME_LEN, psHWRInfo->szProcName,
				                   psHWRInfo->ui32FrameNum,
				                   psHWRInfo->ui32ActiveHWRTData,
				                   psHWRInfo->ui32EventStatus,
				                   pszLockupType);
				}
				else
				{
					PVR_DUMPDEBUG_LOG("  %s PID = %u / %.*s, frame = %d, HWRTData = 0x%08X, EventStatus = 0x%08X%s",
				                   aui8RecoveryNum,
				                   psHWRInfo->ui32PID,
				                   RGXFW_PROCESS_NAME_LEN, psHWRInfo->szProcName,
				                   psHWRInfo->ui32FrameNum,
				                   psHWRInfo->ui32ActiveHWRTData,
				                   psHWRInfo->ui32EventStatus,
				                   pszLockupType);
				}

				if (psHWRInfo->eHWErrorCode != RGX_HW_ERR_NA)
				{
					IMG_CHAR sHWDebugInfo[RGX_DEBUG_STR_SIZE] = "";

					_ID2Description(sHWDebugInfo, RGX_DEBUG_STR_SIZE, asHWErrorState, ARRAY_SIZE(asHWErrorState),
						psHWRInfo->eHWErrorCode);
					PVR_DUMPDEBUG_LOG("  HW error code = 0x%X: %s",
									  psHWRInfo->eHWErrorCode, sHWDebugInfo);
				}

				pszTemp = &aui8RecoveryNum[0];
				while (*pszTemp != '\0')
				{
					*pszTemp++ = ' ';
				}

				/* There's currently no time correlation for the Guest OSes on the Firmware so there's no point printing OS Timestamps on Guests */
				if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
				{
					PVR_DUMPDEBUG_LOG("  %s CRTimer = 0x%012"IMG_UINT64_FMTSPECX", OSTimer = %" IMG_UINT64_FMTSPEC ".%09" IMG_UINT64_FMTSPEC ", CyclesElapsed = %" IMG_INT64_FMTSPECd,
									   aui8RecoveryNum,
									   psHWRInfo->ui64CRTimer,
									   ui64Seconds,
									   ui64Nanoseconds,
									   (psHWRInfo->ui64CRTimer-psHWRInfo->ui64CRTimeOfKick)*256);
				}
				else
				{
					PVR_DUMPDEBUG_LOG("  %s CRTimer = 0x%012"IMG_UINT64_FMTSPECX", CyclesElapsed = %" IMG_INT64_FMTSPECd,
									   aui8RecoveryNum,
									   psHWRInfo->ui64CRTimer,
									   (psHWRInfo->ui64CRTimer-psHWRInfo->ui64CRTimeOfKick)*256);
				}

				if (psHWRInfo->ui64CRTimeHWResetFinish != 0)
				{
					if (psHWRInfo->ui64CRTimeFreelistReady != 0)
					{
						/* If ui64CRTimeFreelistReady is less than ui64CRTimeHWResetFinish it means APM kicked in and the time is not valid. */
						if (psHWRInfo->ui64CRTimeHWResetFinish < psHWRInfo->ui64CRTimeFreelistReady)
						{
							PVR_DUMPDEBUG_LOG("  %s PreResetTimeInCycles = %" IMG_INT64_FMTSPECd ", HWResetTimeInCycles = %" IMG_INT64_FMTSPECd ", FreelistReconTimeInCycles = %" IMG_INT64_FMTSPECd ", TotalRecoveryTimeInCycles = %" IMG_INT64_FMTSPECd,
											   aui8RecoveryNum,
											   (psHWRInfo->ui64CRTimeHWResetStart-psHWRInfo->ui64CRTimer)*256,
											   (psHWRInfo->ui64CRTimeHWResetFinish-psHWRInfo->ui64CRTimeHWResetStart)*256,
											   (psHWRInfo->ui64CRTimeFreelistReady-psHWRInfo->ui64CRTimeHWResetFinish)*256,
											   (psHWRInfo->ui64CRTimeFreelistReady-psHWRInfo->ui64CRTimer)*256);
						}
						else
						{
							PVR_DUMPDEBUG_LOG("  %s PreResetTimeInCycles = %" IMG_INT64_FMTSPECd ", HWResetTimeInCycles = %" IMG_INT64_FMTSPECd ", FreelistReconTimeInCycles = <not_timed>, TotalResetTimeInCycles = %" IMG_INT64_FMTSPECd,
											   aui8RecoveryNum,
											   (psHWRInfo->ui64CRTimeHWResetStart-psHWRInfo->ui64CRTimer)*256,
											   (psHWRInfo->ui64CRTimeHWResetFinish-psHWRInfo->ui64CRTimeHWResetStart)*256,
											   (psHWRInfo->ui64CRTimeHWResetFinish-psHWRInfo->ui64CRTimer)*256);
						}
					}
					else
					{
						PVR_DUMPDEBUG_LOG("  %s PreResetTimeInCycles = %" IMG_INT64_FMTSPECd ", HWResetTimeInCycles = %" IMG_INT64_FMTSPECd ", TotalResetTimeInCycles = %" IMG_INT64_FMTSPECd,
										   aui8RecoveryNum,
										   (psHWRInfo->ui64CRTimeHWResetStart-psHWRInfo->ui64CRTimer)*256,
										   (psHWRInfo->ui64CRTimeHWResetFinish-psHWRInfo->ui64CRTimeHWResetStart)*256,
										   (psHWRInfo->ui64CRTimeHWResetFinish-psHWRInfo->ui64CRTimer)*256);
					}
				}

#if defined(HW_ERN_65104_BIT_MASK)
				if (RGX_IS_ERN_SUPPORTED(psDevInfo, 65104))
				{
					PVR_DUMPDEBUG_LOG("    Active PDS DM USCs = 0x%08x", psHWRInfo->ui32PDSActiveDMUSCs);
				}
#endif

#if defined(HW_ERN_69700_BIT_MASK)
				if (RGX_IS_ERN_SUPPORTED(psDevInfo, 69700))
				{
					PVR_DUMPDEBUG_LOG("    DMs stalled waiting on PDS Store space = 0x%08x", psHWRInfo->ui32PDSStalledDMs);
				}
#endif

				switch (psHWRInfo->eHWRType)
				{
					case RGX_HWRTYPE_ECCFAULT:
					{
						PVR_DUMPDEBUG_LOG("    ECC fault GPU=0x%08x", psHWRInfo->uHWRData.sECCInfo.ui32FaultGPU);
					}
					break;

					case RGX_HWRTYPE_MMUFAULT:
					{
						if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, S7_TOP_INFRASTRUCTURE))
						{
							_RGXDumpRGXMMUFaultStatus(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo,
											&psHWRInfo->uHWRData.sMMUInfo.aui64MMUStatus[0],
											"Core",
											DD_NORMAL_INDENT);

							bPageFault = IMG_TRUE;
							sFaultDevVAddr.uiAddr =   psHWRInfo->uHWRData.sMMUInfo.aui64MMUStatus[0];
							sFaultDevVAddr.uiAddr &=  ~RGX_CR_MMU_FAULT_STATUS1_ADDRESS_CLRMSK;
							sFaultDevVAddr.uiAddr >>= RGX_CR_MMU_FAULT_STATUS1_ADDRESS_SHIFT;
							sFaultDevVAddr.uiAddr <<= 4; /* align shift */
							ui32PC  = (psHWRInfo->uHWRData.sMMUInfo.aui64MMUStatus[0] & ~RGX_CR_MMU_FAULT_STATUS1_CONTEXT_CLRMSK) >>
													   RGX_CR_MMU_FAULT_STATUS1_CONTEXT_SHIFT;
							bPMFault = (ui32PC <= 8);
							sPCDevPAddr.uiAddr = psHWRInfo->uHWRData.sMMUInfo.ui64PCAddress;
						}
					}
					break;

					case RGX_HWRTYPE_MMUMETAFAULT:
					{
						if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, S7_TOP_INFRASTRUCTURE))
						{
							const IMG_PCHAR pszMetaOrRiscv = RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META) ? "Meta" : "RiscV";

							_RGXDumpRGXMMUFaultStatus(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo,
											&psHWRInfo->uHWRData.sMMUInfo.aui64MMUStatus[0],
											pszMetaOrRiscv,
											DD_NORMAL_INDENT);

							bPageFault = IMG_TRUE;
							sFaultDevVAddr.uiAddr =   psHWRInfo->uHWRData.sMMUInfo.aui64MMUStatus[0];
							sFaultDevVAddr.uiAddr &=  ~RGX_CR_MMU_FAULT_STATUS1_ADDRESS_CLRMSK;
							sFaultDevVAddr.uiAddr >>= RGX_CR_MMU_FAULT_STATUS1_ADDRESS_SHIFT;
							sFaultDevVAddr.uiAddr <<= 4; /* align shift */
							sPCDevPAddr.uiAddr = psHWRInfo->uHWRData.sMMUInfo.ui64PCAddress;
						}
					}
					break;

					case RGX_HWRTYPE_POLLFAILURE:
					{
						PVR_DUMPDEBUG_LOG("    T%u polling %s (reg:0x%08X mask:0x%08X last:0x%08X)",
										  psHWRInfo->uHWRData.sPollInfo.ui32ThreadNum,
										  ((psHWRInfo->uHWRData.sPollInfo.ui32CrPollAddr & RGXFW_POLL_TYPE_SET)?("set"):("unset")),
										  psHWRInfo->uHWRData.sPollInfo.ui32CrPollAddr & ~RGXFW_POLL_TYPE_SET,
										  psHWRInfo->uHWRData.sPollInfo.ui32CrPollMask,
										  psHWRInfo->uHWRData.sPollInfo.ui32CrPollLastValue);
					}
					break;

#if defined(RGX_FEATURE_MIPS_BIT_MASK)
					case RGX_HWRTYPE_MIPSTLBFAULT:
					{
						if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, MIPS))
						{
							IMG_UINT32 ui32EntryLo = psHWRInfo->uHWRData.sTLBInfo.ui32EntryLo;

							/* This is not exactly what the MMU code does, but the result should be the same */
							const IMG_UINT32 ui32UnmappedEntry =
								((IMG_UINT32)(MMU_BAD_PHYS_ADDR & 0xffffffff) & RGXMIPSFW_ENTRYLO_PFN_MASK_ABOVE_32BIT) | RGXMIPSFW_ENTRYLO_UNCACHED;

							PVR_DUMPDEBUG_LOG("    MIPS TLB fault: BadVA = 0x%08X, EntryLo = 0x%08X"
											  " (page PA 0x%" IMG_UINT64_FMTSPECx", V %u)",
											  psHWRInfo->uHWRData.sTLBInfo.ui32BadVAddr,
											  ui32EntryLo,
											  RGXMIPSFW_TLB_GET_PA(ui32EntryLo),
											  ui32EntryLo & RGXMIPSFW_TLB_VALID ? 1 : 0);

							if (ui32EntryLo == ui32UnmappedEntry)
							{
								PVR_DUMPDEBUG_LOG("    Potential use-after-free detected");
							}
						}
					}
					break;
#endif

					case RGX_HWRTYPE_OVERRUN:
					case RGX_HWRTYPE_UNKNOWNFAILURE:
					{
						/* Nothing to dump */
					}
					break;

					default:
					{
						PVR_DUMPDEBUG_LOG("    Unknown HWR Info type: 0x%x", psHWRInfo->eHWRType);
					}
					break;
				}

				if (bPageFault)
				{
					RGXDumpFaultInfo(psDevInfo, pfnDumpDebugPrintf, pvDumpDebugFile, psHWRInfo,
					                 ui32ReadIndex, &sFaultDevVAddr, &sPCDevPAddr, bPMFault, ui32PageSize);
				}

			}

			if (ui32ReadIndex == RGXFWIF_HWINFO_MAX_FIRST - 1)
				ui32ReadIndex = psHWRInfoBuf->ui32WriteIndex;
			else
				ui32ReadIndex = (ui32ReadIndex + 1) - (ui32ReadIndex / RGXFWIF_HWINFO_LAST_INDEX) * RGXFWIF_HWINFO_MAX_LAST;
		}
	}
}




#if !defined(NO_HARDWARE)

/*!
*******************************************************************************

 @Function	_CheckForPendingPage

 @Description

 Check if the MMU indicates it is blocked on a pending page
 MMU4 does not support pending pages, so return false.

 @Input psDevInfo	 - RGX device info

 @Return   IMG_BOOL      - IMG_TRUE if there is a pending page

******************************************************************************/
static INLINE IMG_BOOL _CheckForPendingPage(PVRSRV_RGXDEV_INFO *psDevInfo)
{
#if defined(RGX_FEATURE_MMU_VERSION_MAX_VALUE_IDX)
	/* MMU4 doesn't support pending pages */
	return (RGX_GET_FEATURE_VALUE(psDevInfo, MMU_VERSION) < 4) &&
		   (OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MMU_ENTRY) & RGX_CR_MMU_ENTRY_PENDING_EN);
#else
	IMG_UINT32 ui32BIFMMUEntry;

	ui32BIFMMUEntry = OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_BIF_MMU_ENTRY);

	if (ui32BIFMMUEntry & RGX_CR_BIF_MMU_ENTRY_PENDING_EN)
	{
		return IMG_TRUE;
	}
	else
	{
		return IMG_FALSE;
	}
#endif
}

/*!
*******************************************************************************

 @Function	_GetPendingPageInfo

 @Description

 Get information about the pending page from the MMU status registers

 @Input psDevInfo	 - RGX device info
 @Output psDevVAddr      - The device virtual address of the pending MMU address translation
 @Output pui32CatBase    - The page catalog base

 @Return   void

******************************************************************************/
static void _GetPendingPageInfo(PVRSRV_RGXDEV_INFO *psDevInfo, IMG_DEV_VIRTADDR *psDevVAddr,
								IMG_UINT32 *pui32CatBase)
{
	IMG_UINT64 ui64BIFMMUEntryStatus;

	ui64BIFMMUEntryStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_MMU_ENTRY_STATUS);

	psDevVAddr->uiAddr = (ui64BIFMMUEntryStatus & ~RGX_CR_MMU_ENTRY_STATUS_ADDRESS_CLRMSK);

	*pui32CatBase = (ui64BIFMMUEntryStatus & ~RGX_CR_MMU_ENTRY_STATUS_CONTEXT_ID_CLRMSK) >>
								RGX_CR_MMU_ENTRY_STATUS_CONTEXT_ID_SHIFT;
}

#endif

void RGXDumpRGXDebugSummary(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					PVRSRV_RGXDEV_INFO *psDevInfo,
					IMG_BOOL bRGXPoweredON)
{
	const RGXFWIF_SYSDATA *psFwSysData = psDevInfo->psRGXFWIfFwSysData;
	const RGXFWIF_TRACEBUF *psRGXFWIfTraceBufCtl = psDevInfo->psRGXFWIfTraceBufCtl;
	IMG_UINT32 ui32DriverID;
	const RGXFWIF_RUNTIME_CFG *psRuntimeCfg = psDevInfo->psRGXFWIfRuntimeCfg;
	/* space for the current clock speed and 3 previous */
	RGXFWIF_TIME_CORR asTimeCorrs[4];
	IMG_UINT32 ui32NumClockSpeedChanges;

	/* Should invalidate all reads below including when passed to functions. */
	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfFwSysData, INVALIDATE);
	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfRuntimeCfg, INVALIDATE);

#if defined(NO_HARDWARE)
	PVR_UNREFERENCED_PARAMETER(bRGXPoweredON);
#else
	if ((bRGXPoweredON) && !PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, S7_TOP_INFRASTRUCTURE))
		{
			IMG_UINT64	aui64RegValMMUStatus[2];
			const IMG_PCHAR pszMetaOrRiscv = RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META) ? "Meta" : "RiscV";

			/*
			 *  BRN72144 prevents reading RGX_CR_MMU_FAULT_STATUS1/2
			 *  registers while the FW is running...
			 */
			if (!RGX_IS_BRN_SUPPORTED(psDevInfo, 72144) ||
			    (psRGXFWIfTraceBufCtl->sTraceBuf[0].sAssertBuf.szInfo[0] != 0))
			{
				aui64RegValMMUStatus[0] = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_MMU_FAULT_STATUS1);
				aui64RegValMMUStatus[1] = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_MMU_FAULT_STATUS2);
				_RGXDumpRGXMMUFaultStatus(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, &aui64RegValMMUStatus[0], "Core", DD_SUMMARY_INDENT);
			}
			else
			{
				PVR_DUMPDEBUG_LOG("%sMMU (Core) - Unknown", DD_SUMMARY_INDENT);
			}

			aui64RegValMMUStatus[0] = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_MMU_FAULT_STATUS_META);
			_RGXDumpRGXMMUFaultStatus(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, &aui64RegValMMUStatus[0], pszMetaOrRiscv, DD_SUMMARY_INDENT);
		}

		if (_CheckForPendingPage(psDevInfo))
		{
			IMG_UINT32 ui32CatBase;
			IMG_DEV_VIRTADDR sDevVAddr;

			PVR_DUMPDEBUG_LOG("MMU Pending page: Yes");

			_GetPendingPageInfo(psDevInfo, &sDevVAddr, &ui32CatBase);

			if (ui32CatBase <= MAX_RESERVED_FW_MMU_CONTEXT)
			{
				PVR_DUMPDEBUG_LOG("Cannot check address on PM cat base %u", ui32CatBase);
			}
			else
			{
				IMG_DEV_PHYADDR sPCDevPAddr;
				MMU_FAULT_DATA sFaultData;
				IMG_BOOL bIsValid = IMG_TRUE;

#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
				IMG_UINT64 ui64CBaseMapping;
				IMG_UINT32 ui32CBaseMapCtxReg;

				if (RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) > 1)
				{
					ui32CBaseMapCtxReg = RGX_CR_MMU_CBASE_MAPPING_CONTEXT__HOST_SECURITY_GT1_AND_MHPW_LT6_AND_MMU_VER_GEQ4;

					OSWriteUncheckedHWReg32(psDevInfo->pvSecureRegsBaseKM, ui32CBaseMapCtxReg, ui32CatBase);

					ui64CBaseMapping = OSReadUncheckedHWReg64(psDevInfo->pvSecureRegsBaseKM, RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1);
					sPCDevPAddr.uiAddr = (((ui64CBaseMapping & ~RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_CLRMSK)
												>> RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_SHIFT)
												<< RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__BASE_ADDR_ALIGNSHIFT);
					bIsValid = !(ui64CBaseMapping & RGX_CR_MMU_CBASE_MAPPING__HOST_SECURITY_GT1__INVALID_EN);
				}
				else
				{
					ui32CBaseMapCtxReg = RGX_CR_MMU_CBASE_MAPPING_CONTEXT;

					OSWriteUncheckedHWReg32(psDevInfo->pvSecureRegsBaseKM, ui32CBaseMapCtxReg, ui32CatBase);

					ui64CBaseMapping = OSReadUncheckedHWReg64(psDevInfo->pvSecureRegsBaseKM, RGX_CR_MMU_CBASE_MAPPING);
					sPCDevPAddr.uiAddr = (((ui64CBaseMapping & ~RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_CLRMSK)
												>> RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_SHIFT)
												<< RGX_CR_MMU_CBASE_MAPPING_BASE_ADDR_ALIGNSHIFT);
					bIsValid = !(ui64CBaseMapping & RGX_CR_MMU_CBASE_MAPPING_INVALID_EN);
				}
#else
				sPCDevPAddr.uiAddr = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_BIF_CAT_BASEN(ui32CatBase));
#endif

				PVR_DUMPDEBUG_LOG("Checking device virtual address " IMG_DEV_VIRTADDR_FMTSPEC
							" on cat base %u. PC Addr = 0x%" IMG_UINT64_FMTSPECx " is %s",
								sDevVAddr.uiAddr,
								ui32CatBase,
								sPCDevPAddr.uiAddr,
								bIsValid ? "valid":"invalid");
				RGXCheckFaultAddress(psDevInfo, &sDevVAddr, &sPCDevPAddr, &sFaultData);
				RGXDumpFaultAddressHostView(&sFaultData, pfnDumpDebugPrintf, pvDumpDebugFile, DD_SUMMARY_INDENT);
			}
		}
	}
#endif /* NO_HARDWARE */

#if !defined(NO_HARDWARE)
	/* Determine the type virtualisation support used */
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	if (!PVRSRV_VZ_MODE_IS(NATIVE, DEVINFO, psDevInfo))
	{
#if defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS)
#if defined(SUPPORT_AUTOVZ)
#if defined(SUPPORT_AUTOVZ_HW_REGS)
		PVR_DUMPDEBUG_LOG("RGX Virtualisation type: AutoVz with HW register support");
#else
		PVR_DUMPDEBUG_LOG("RGX Virtualisation type: AutoVz with shared memory");
#endif /* defined(SUPPORT_AUTOVZ_HW_REGS) */
#else
		PVR_DUMPDEBUG_LOG("RGX Virtualisation type: Hypervisor-assisted with static Fw heap allocation");
#endif /* defined(SUPPORT_AUTOVZ) */
#else
		PVR_DUMPDEBUG_LOG("RGX Virtualisation type: Hypervisor-assisted with dynamic Fw heap allocation");
#endif /* defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS) */
	}
#endif /* (RGX_NUM_DRIVERS_SUPPORTED > 1) */

#if defined(RGX_VZ_STATIC_CARVEOUT_FW_HEAPS) || (defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1))
	if (!PVRSRV_VZ_MODE_IS(NATIVE, DEVINFO, psDevInfo))
	{
		RGXFWIF_CONNECTION_FW_STATE eFwState;
		RGXFWIF_CONNECTION_OS_STATE eOsState;

		KM_CONNECTION_CACHEOP(Fw, INVALIDATE);
		KM_CONNECTION_CACHEOP(Os, INVALIDATE);

		eFwState = KM_GET_FW_CONNECTION(psDevInfo);
		eOsState = KM_GET_OS_CONNECTION(psDevInfo);

		PVR_DUMPDEBUG_LOG("RGX Virtualisation firmware connection state: %s (Fw=%s; OS=%s)",
						  ((eFwState == RGXFW_CONNECTION_FW_ACTIVE) && (eOsState == RGXFW_CONNECTION_OS_ACTIVE)) ? ("UP") : ("DOWN"),
						  (eFwState < RGXFW_CONNECTION_FW_STATE_COUNT) ? (apszFwOsStateName[eFwState]) : ("invalid"),
						  (eOsState < RGXFW_CONNECTION_OS_STATE_COUNT) ? (apszFwOsStateName[eOsState]) : ("invalid"));

	}
#endif

#if defined(SUPPORT_AUTOVZ) && defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	if (!PVRSRV_VZ_MODE_IS(NATIVE, DEVINFO, psDevInfo))
	{
		IMG_UINT32 ui32FwAliveTS;
		IMG_UINT32 ui32OsAliveTS;

		KM_ALIVE_TOKEN_CACHEOP(Fw, INVALIDATE);
		KM_ALIVE_TOKEN_CACHEOP(Os, INVALIDATE);

		ui32FwAliveTS = KM_GET_FW_ALIVE_TOKEN(psDevInfo);
		ui32OsAliveTS = KM_GET_OS_ALIVE_TOKEN(psDevInfo);

		PVR_DUMPDEBUG_LOG("RGX Virtualisation watchdog timestamps (in GPU timer ticks): Fw=%u; OS=%u; diff(FW, OS) = %u",
						  ui32FwAliveTS, ui32OsAliveTS, ui32FwAliveTS - ui32OsAliveTS);
	}
#endif
#endif /* !defined(NO_HARDWARE) */

	if (!PVRSRV_VZ_MODE_IS(GUEST, DEVINFO, psDevInfo))
	{
		IMG_CHAR sHwrStateDescription[RGX_DEBUG_STR_SIZE];
		IMG_BOOL bDriverIsolationEnabled = IMG_FALSE;
		IMG_UINT32 ui32HostIsolationGroup;

		sHwrStateDescription[0] = '\0';

		DebugCommonFlagStrings(sHwrStateDescription, RGX_DEBUG_STR_SIZE,
			asHwrState2Description, ARRAY_SIZE(asHwrState2Description),
			psFwSysData->ui32HWRStateFlags);
		PVR_DUMPDEBUG_LOG("RGX HWR State 0x%08x:%s", psFwSysData->ui32HWRStateFlags, sHwrStateDescription);
		PVR_DUMPDEBUG_LOG("RGX FW Power State: %s (APM %s: %d ok, %d denied, %d non-idle, %d retry, %d other, %d total. Latency: %u ms)",
		                  (psFwSysData->ePowState < ARRAY_SIZE(pszPowStateName) ? pszPowStateName[psFwSysData->ePowState] : "???"),
		                  (psDevInfo->pvAPMISRData)?"enabled":"disabled",
		                  psDevInfo->ui32ActivePMReqOk - psDevInfo->ui32ActivePMReqNonIdle,
		                  psDevInfo->ui32ActivePMReqDenied,
		                  psDevInfo->ui32ActivePMReqNonIdle,
		                  psDevInfo->ui32ActivePMReqRetry,
		                  psDevInfo->ui32ActivePMReqTotal -
		                  psDevInfo->ui32ActivePMReqOk -
		                  psDevInfo->ui32ActivePMReqDenied -
		                  psDevInfo->ui32ActivePMReqRetry -
		                  psDevInfo->ui32ActivePMReqNonIdle,
		                  psDevInfo->ui32ActivePMReqTotal,
		                  psRuntimeCfg->ui32ActivePMLatencyms);

		ui32NumClockSpeedChanges = (IMG_UINT32) OSAtomicRead(&psDevInfo->psDeviceNode->iNumClockSpeedChanges);
		RGXGetTimeCorrData(psDevInfo->psDeviceNode, asTimeCorrs, ARRAY_SIZE(asTimeCorrs));

		PVR_DUMPDEBUG_LOG("RGX DVFS: %u frequency changes. "
		                  "Current frequency: %u.%03u MHz (sampled at %" IMG_UINT64_FMTSPEC " ns). "
		                  "FW frequency: %u.%03u MHz.",
		                  ui32NumClockSpeedChanges,
		                  asTimeCorrs[0].ui32CoreClockSpeed / 1000000,
		                  (asTimeCorrs[0].ui32CoreClockSpeed / 1000) % 1000,
		                  asTimeCorrs[0].ui64OSTimeStamp,
		                  psRuntimeCfg->ui32CoreClockSpeed / 1000000,
		                  (psRuntimeCfg->ui32CoreClockSpeed / 1000) % 1000);
		if (ui32NumClockSpeedChanges > 0)
		{
			PVR_DUMPDEBUG_LOG("          Previous frequencies: %u.%03u, %u.%03u, %u.%03u MHz (Sampled at "
							"%" IMG_UINT64_FMTSPEC ", %" IMG_UINT64_FMTSPEC ", %" IMG_UINT64_FMTSPEC ")",
												asTimeCorrs[1].ui32CoreClockSpeed / 1000000,
												(asTimeCorrs[1].ui32CoreClockSpeed / 1000) % 1000,
												asTimeCorrs[2].ui32CoreClockSpeed / 1000000,
												(asTimeCorrs[2].ui32CoreClockSpeed / 1000) % 1000,
												asTimeCorrs[3].ui32CoreClockSpeed / 1000000,
												(asTimeCorrs[3].ui32CoreClockSpeed / 1000) % 1000,
												asTimeCorrs[1].ui64OSTimeStamp,
												asTimeCorrs[2].ui64OSTimeStamp,
												asTimeCorrs[3].ui64OSTimeStamp);
		}

		ui32HostIsolationGroup = psDevInfo->psRGXFWIfRuntimeCfg->aui32DriverIsolationGroup[RGXFW_HOST_DRIVER_ID];

		FOREACH_SUPPORTED_DRIVER(ui32DriverID)
		{
			RGXFWIF_OS_RUNTIME_FLAGS sFwRunFlags = psFwSysData->asOsRuntimeFlagsMirror[ui32DriverID];
			IMG_UINT32 ui32IsolationGroup = psDevInfo->psRGXFWIfRuntimeCfg->aui32DriverIsolationGroup[ui32DriverID];
			IMG_BOOL bMTSEnabled = IMG_FALSE;

#if !defined(NO_HARDWARE)
			if (bRGXPoweredON)
			{
				bMTSEnabled = (
#if defined(FIX_HW_BRN_64502_BIT_MASK)
								RGX_IS_BRN_SUPPORTED(psDevInfo, 64502) ||
#endif
								!RGX_IS_FEATURE_SUPPORTED(psDevInfo, GPU_VIRTUALISATION)) ?
								IMG_TRUE : ((OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MTS_SCHEDULE_ENABLE) & BIT(ui32DriverID)) != 0);
			}
#endif
			PVR_DUMPDEBUG_LOG("RGX FW OS %u - State: %s; Freelists: %s%s; Priority: %u; Isolation group: %u;  Time Slice%s: %u%% (%ums); %s", ui32DriverID,
							  apszFwOsStateName[sFwRunFlags.bfOsState],
							  (sFwRunFlags.bfFLOk) ? "Ok" : "Not Ok",
							  (sFwRunFlags.bfFLGrowPending) ? "; Grow Request Pending" : "",
							  psDevInfo->psRGXFWIfRuntimeCfg->ai32DriverPriority[ui32DriverID],
							  ui32IsolationGroup,
							  (psDevInfo->psRGXFWIfRuntimeCfg->aui32TSPercentage[ui32DriverID] != 0) ? "" : "*",
							  psFwSysData->aui32TSMirror[ui32DriverID],
							  (psFwSysData->aui32TSMirror[ui32DriverID] *
							   psDevInfo->psRGXFWIfRuntimeCfg->ui32TSIntervalMs / 100),
							  (bMTSEnabled) ? "MTS on;" : "MTS off;"
							 );

			if (ui32IsolationGroup != ui32HostIsolationGroup)
			{
				bDriverIsolationEnabled = IMG_TRUE;
			}

			if (PVRSRV_VZ_MODE_IS(NATIVE, DEVINFO, psDevInfo))
			{
				/* don't print guest information on native mode drivers */
				break;
			}
		}

#if defined(PVR_ENABLE_PHR)
		{
			IMG_CHAR sPHRConfigDescription[RGX_DEBUG_STR_SIZE];

			sPHRConfigDescription[0] = '\0';
			DebugCommonFlagStrings(sPHRConfigDescription, RGX_DEBUG_STR_SIZE,
			                   asPHRConfig2Description, ARRAY_SIZE(asPHRConfig2Description),
			                   BIT_ULL(psDevInfo->psRGXFWIfRuntimeCfg->ui32PHRMode));

			PVR_DUMPDEBUG_LOG("RGX PHR configuration: (%d) %.*s", psDevInfo->psRGXFWIfRuntimeCfg->ui32PHRMode, RGX_DEBUG_STR_SIZE, sPHRConfigDescription);
		}
#endif

		if (bDriverIsolationEnabled)
		{
			PVR_DUMPDEBUG_LOG("RGX Hard Context Switch deadline: %u ms", psDevInfo->psRGXFWIfRuntimeCfg->ui32HCSDeadlineMS);
		}

		_RGXDumpFWAssert(pfnDumpDebugPrintf, pvDumpDebugFile, psRGXFWIfTraceBufCtl);
		_RGXDumpFWFaults(pfnDumpDebugPrintf, pvDumpDebugFile, psFwSysData);
		_RGXDumpFWPoll(pfnDumpDebugPrintf, pvDumpDebugFile, psFwSysData);
	}
	else
	{
		PVR_DUMPDEBUG_LOG("RGX FW State: Unavailable under Guest Mode of operation");
		PVR_DUMPDEBUG_LOG("RGX FW Power State: Unavailable under Guest Mode of operation");
	}

	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfHWRInfoBufCtl, INVALIDATE);
	_RGXDumpFWHWRInfo(pfnDumpDebugPrintf, pvDumpDebugFile, psFwSysData, psDevInfo->psRGXFWIfHWRInfoBufCtl, psDevInfo);

#if defined(SUPPORT_RGXFW_STATS_FRAMEWORK)
	/* Dump all non-zero values in lines of 8... */
	{
		IMG_CHAR    pszLine[(9*RGXFWIF_STATS_FRAMEWORK_LINESIZE)+1];
		const IMG_UINT32 *pui32FWStatsBuf = psFwSysData->aui32FWStatsBuf;
		IMG_UINT32  ui32Index1, ui32Index2;

		PVR_DUMPDEBUG_LOG("STATS[START]: RGXFWIF_STATS_FRAMEWORK_MAX=%d", RGXFWIF_STATS_FRAMEWORK_MAX);
		for (ui32Index1 = 0;  ui32Index1 < RGXFWIF_STATS_FRAMEWORK_MAX;  ui32Index1 += RGXFWIF_STATS_FRAMEWORK_LINESIZE)
		{
			IMG_UINT32  ui32OrOfValues = 0;
			IMG_CHAR    *pszBuf = pszLine;

			/* Print all values in this line and skip if all zero... */
			for (ui32Index2 = 0;  ui32Index2 < RGXFWIF_STATS_FRAMEWORK_LINESIZE;  ui32Index2++)
			{
				ui32OrOfValues |= pui32FWStatsBuf[ui32Index1+ui32Index2];
				OSSNPrintf(pszBuf, 9 + 1, " %08x", pui32FWStatsBuf[ui32Index1+ui32Index2]);
				pszBuf += 9; /* write over the '\0' */
			}

			if (ui32OrOfValues != 0)
			{
				PVR_DUMPDEBUG_LOG("STATS[%08x]:%s", ui32Index1, pszLine);
			}
		}
		PVR_DUMPDEBUG_LOG("STATS[END]");
	}
#endif
}

#if !defined(NO_HARDWARE)
PVRSRV_ERROR RGXDumpRISCVState(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
									  void *pvDumpDebugFile,
									  PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvSecureRegsBaseKM;
	RGXRISCVFW_STATE sRiscvState;
	const IMG_CHAR *pszException;
	PVRSRV_ERROR eError;

	/* Limit dump to what is currently being used */
#if defined(RGX_FEATURE_HOST_SECURITY_VERSION_MAX_VALUE_IDX)
	if (RGX_GET_FEATURE_VALUE(psDevInfo, HOST_SECURITY_VERSION) >= 4)
	{
		DDLOGUNCHECKED64(FWCORE_ADDR_REMAP_CONFIG4__HOST_SECURITY_GEQ4);
		DDLOGUNCHECKED64(FWCORE_ADDR_REMAP_CONFIG5__HOST_SECURITY_GEQ4);
		DDLOGUNCHECKED64(FWCORE_ADDR_REMAP_CONFIG6__HOST_SECURITY_GEQ4);
		DDLOGUNCHECKED64(FWCORE_ADDR_REMAP_CONFIG12__HOST_SECURITY_GEQ4);
		DDLOGUNCHECKED64(FWCORE_ADDR_REMAP_CONFIG13__HOST_SECURITY_GEQ4);
		DDLOGUNCHECKED64(FWCORE_ADDR_REMAP_CONFIG14__HOST_SECURITY_GEQ4);
	}
	else
#endif
	{
		DDLOG64(FWCORE_ADDR_REMAP_CONFIG4);
		DDLOG64(FWCORE_ADDR_REMAP_CONFIG5);
		DDLOG64(FWCORE_ADDR_REMAP_CONFIG6);
		DDLOG64(FWCORE_ADDR_REMAP_CONFIG12);
		DDLOG64(FWCORE_ADDR_REMAP_CONFIG13);
		DDLOG64(FWCORE_ADDR_REMAP_CONFIG14);
	}

	PVR_DUMPDEBUG_LOG("---- [ RISC-V internal state ] ----");

#if  defined(SUPPORT_RISCV_GDB)
	if (RGXRiscvIsHalted(psDevInfo))
	{
		/* Avoid resuming the RISC-V FW as most operations
		 * on the debug module require a halted core */
		PVR_DUMPDEBUG_LOG("(skipping as RISC-V found halted)");
		return PVRSRV_OK;
	}
#endif

	eError = RGXRiscvHalt(psDevInfo);
	PVR_GOTO_IF_ERROR(eError, _RISCVDMError);

#define X(name, address)												\
	eError = RGXRiscvReadReg(psDevInfo, address, &sRiscvState.name);	\
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXRiscvReadReg", _RISCVDMError);	\
	DDLOGVAL32(#name, sRiscvState.name);

	RGXRISCVFW_DEBUG_DUMP_REGISTERS
#undef X

	eError = RGXRiscvResume(psDevInfo);
	PVR_GOTO_IF_ERROR(eError, _RISCVDMError);

	pszException = _GetRISCVException(sRiscvState.mcause);
	if (pszException != NULL)
	{
		PVR_DUMPDEBUG_LOG("RISC-V FW hit an exception: %s", pszException);

		eError = RGXValidateFWImage(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo);
		if (eError != PVRSRV_OK)
		{
			PVR_DUMPDEBUG_LOG("Failed to validate any FW code corruption");
		}
	}

	return PVRSRV_OK;

_RISCVDMError:
	PVR_DPF((PVR_DBG_ERROR, "Failed to communicate with the Debug Module"));

	return eError;
}

void RGXDumpCoreRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	DDLOG32(CORE_ID);
}

void RGXDumpMulticoreRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	DDLOG64(MULTICORE);
	DDLOG32(MULTICORE_SYSTEM);
	DDLOG32(MULTICORE_DOMAIN);

#if !defined(RGX_CR_MULTICORE_AXI)
#define RGX_CR_MULTICORE_AXI                              (0x2508U)
#define RGX_CR_MULTICORE_AXI_ERROR                        (0x2510U)
#endif
	DDLOG32(MULTICORE_AXI);
	DDLOG32(MULTICORE_AXI_ERROR);
	DDLOG32(MULTICORE_TDM_CTRL_COMMON);
	DDLOG32(MULTICORE_FRAGMENT_CTRL_COMMON);
	DDLOG32(MULTICORE_COMPUTE_CTRL_COMMON);
}

void RGXDumpClkRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	DDLOG64(CLK_CTRL0);
	DDLOG64(CLK_STATUS0);
	DDLOG64(CLK_CTRL1);
	DDLOG64(CLK_STATUS1);
	DDLOG32(CLK_CTRL2);
	DDLOG32(CLK_STATUS2);
}

void RGXDumpMMURegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	RGX_LAYER_PARAMS sParams = {.psDevInfo = psDevInfo};
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	/* BRN72144 prevents reading RGX_CR_MMU_FAULT_STATUS1/2 while the FW is running... */
	if (!RGX_IS_BRN_SUPPORTED(psDevInfo, 72144)  ||
		(psDevInfo->psRGXFWIfTraceBufCtl->sTraceBuf[0].sAssertBuf.szInfo[0] != 0))
	{
		DDLOG64(MMU_FAULT_STATUS1);
		DDLOG64(MMU_FAULT_STATUS2);
	}
	DDLOG64(MMU_FAULT_STATUS_PM);
	DDLOG64(MMU_FAULT_STATUS_META);
	DDLOG64(SLC_STATUS1);
	DDLOG64(SLC_STATUS2);
	DDLOG64(SLC_STATUS3);
	DDLOG64(SLC_STATUS_DEBUG);
	DDLOG64(MMU_STATUS);
	DDLOG32(BIF_PFS);
	DDLOG32(BIF_TEXAS0_PFS);
	DDLOG32(BIF_TEXAS1_PFS);
	DDLOG32(BIF_OUTSTANDING_READ);
	DDLOG32(BIF_TEXAS0_OUTSTANDING_READ);
	DDLOG32(BIF_TEXAS1_OUTSTANDING_READ);
	DDLOG32(FBCDC_IDLE);
	DDLOG32(FBCDC_STATUS);
	DDLOG32(FBCDC_SIGNATURE_STATUS);

	if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, POWER_ISLAND_VERSION) &&
	    RGX_DEVICE_GET_FEATURE_VALUE(&sParams, POWER_ISLAND_VERSION) < 2)
	{
		DDLOG32(SPU_ENABLE);
	}

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, SLC_VIVT))
	{
		DDLOG64(CONTEXT_MAPPING0);
		DDLOG64(CONTEXT_MAPPING2);
		DDLOG64(CONTEXT_MAPPING3);
		DDLOG64(CONTEXT_MAPPING4);
	}
}

void RGXDumpDMRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	DDLOG32(PERF_PHASE_2D);
	DDLOG32(PERF_CYCLE_2D_TOTAL);
	DDLOG32(PERF_PHASE_GEOM);
	DDLOG32(PERF_CYCLE_GEOM_TOTAL);
	DDLOG32(PERF_PHASE_FRAG);
	DDLOG32(PERF_CYCLE_FRAG_TOTAL);
	DDLOG32(PERF_CYCLE_GEOM_OR_FRAG_TOTAL);
	DDLOG32(PERF_CYCLE_GEOM_AND_FRAG_TOTAL);
	DDLOG32(PERF_PHASE_COMP);
	DDLOG32(PERF_CYCLE_COMP_TOTAL);
	DDLOG32(PM_PARTIAL_RENDER_ENABLE);

	DDLOG32(ISP_RENDER);
	DDLOG32(ISP_CTL);

	DDLOG32(CDM_CONTEXT_STORE_STATUS__CDM_CSF_LT5);
	DDLOG64(CDM_CONTEXT_PDS0);
	DDLOG64(CDM_CONTEXT_PDS1);
	DDLOG64(CDM_TERMINATE_PDS);
	DDLOG64(CDM_TERMINATE_PDS1);
	DDLOG64(CDM_CONTEXT_LOAD_PDS0);
	DDLOG64(CDM_CONTEXT_LOAD_PDS1);
}

void RGXDumpSLCRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	DDLOG32(SLC_IDLE);
	DDLOG32(SLC_FAULT_STOP_STATUS);
}

void RGXDumpMiscRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	DDLOG32(JONES_IDLE);

	DDLOG64(SCRATCH0);
	DDLOG64(SCRATCH1);
	DDLOG64(SCRATCH2);
	DDLOG64(SCRATCH3);
	DDLOG64(SCRATCH4);
	DDLOG64(SCRATCH5);
	DDLOG64(SCRATCH6);
	DDLOG64(SCRATCH7);
	DDLOG64(SCRATCH8);
	DDLOG64(SCRATCH9);
	DDLOG64(SCRATCH10);
	DDLOG64(SCRATCH11);
	DDLOG64(SCRATCH12);
	DDLOG64(SCRATCH13);
	DDLOG64(SCRATCH14);
	DDLOG64(SCRATCH15);
	DDLOG32(IRQ_OS0_EVENT_STATUS);
}
#endif /* !defined(NO_HARDWARE) */

#undef REG32_FMTSPEC
#undef REG64_FMTSPEC
#undef DDLOG32
#undef DDLOG64
#undef DDLOG32_DPX
#undef DDLOG64_DPX
#undef DDLOGVAL32

void RGXDumpAllContextInfo(PVRSRV_RGXDEV_INFO *psDevInfo,
					DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					IMG_UINT32 ui32VerbLevel)
{
	DumpRenderCtxtsInfo(psDevInfo, pfnDumpDebugPrintf, pvDumpDebugFile, ui32VerbLevel);
#if defined(SUPPORT_RGXKICKSYNC_BRIDGE)
	DumpKickSyncCtxtsInfo(psDevInfo, pfnDumpDebugPrintf, pvDumpDebugFile, ui32VerbLevel);
#endif
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, COMPUTE))
	{
		DumpComputeCtxtsInfo(psDevInfo, pfnDumpDebugPrintf, pvDumpDebugFile, ui32VerbLevel);
	}
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, FASTRENDER_DM))
	{
		DumpTDMTransferCtxtsInfo(psDevInfo, pfnDumpDebugPrintf, pvDumpDebugFile, ui32VerbLevel);
	}
}

/******************************************************************************
 End of file (rgxdebug.c)
******************************************************************************/

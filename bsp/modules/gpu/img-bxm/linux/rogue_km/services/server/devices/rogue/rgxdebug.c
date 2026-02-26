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
#include "rgxtransfer.h"
#include "rgxtdmtransfer.h"
#include "rgxtimecorr.h"
#include "rgx_options.h"
#include "rgxinit.h"
#include "rgxlayer_impl.h"
#include "devicemem_history_server.h"

#define DD_SUMMARY_INDENT  ""

#define RGX_DEBUG_STR_SIZE			(150U)

#define RGX_CR_BIF_CAT_BASE0                              (0x1200U)
#define RGX_CR_BIF_CAT_BASE1                              (0x1208U)

#define RGX_CR_BIF_CAT_BASEN(n) \
	RGX_CR_BIF_CAT_BASE0 + \
	((RGX_CR_BIF_CAT_BASE1 - RGX_CR_BIF_CAT_BASE0) * n)


#define RGXDBG_BIF_IDS \
	X(BIF0)\
	X(BIF1)\
	X(TEXAS_BIF)\
	X(DPX_BIF) \
	X(FWCORE)

#define RGXDBG_SIDEBAND_TYPES \
	X(META)\
	X(TLA)\
	X(DMA)\
	X(VDMM)\
	X(CDM)\
	X(IPP)\
	X(PM)\
	X(TILING)\
	X(MCU)\
	X(PDS)\
	X(PBE)\
	X(VDMS)\
	X(IPF)\
	X(ISP)\
	X(TPF)\
	X(USCS)\
	X(PPP)\
	X(VCE)\
	X(TPF_CPF)\
	X(IPF_CPF)\
	X(FBCDC)

typedef enum
{
#define X(NAME) RGXDBG_##NAME,
	RGXDBG_BIF_IDS
#undef X
} RGXDBG_BIF_ID;

typedef enum
{
#define X(NAME) RGXDBG_##NAME,
	RGXDBG_SIDEBAND_TYPES
#undef X
} RGXDBG_SIDEBAND_TYPE;

static const IMG_CHAR *const pszPowStateName[] =
{
#define X(NAME)	#NAME,
	RGXFWIF_POW_STATES
#undef X
};

static const IMG_CHAR *const pszBIFNames[] =
{
#define X(NAME)	#NAME,
	RGXDBG_BIF_IDS
#undef X
};

static const IMG_FLAGS2DESC asHwrState2Description[] =
{
	{RGXFWIF_HWR_HARDWARE_OK, " HWR OK;"},
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

 @Function	_RGXDecodePMPC

 @Description

 Return the name for the PM managed Page Catalogues

 @Input ui32PC	 - Page Catalogue number

 @Return   void

******************************************************************************/
static const IMG_CHAR* _RGXDecodePMPC(IMG_UINT32 ui32PC)
{
	const IMG_CHAR* pszPMPC = " (-)";

	switch (ui32PC)
	{
		case 0x8: pszPMPC = " (PM-VCE0)"; break;
		case 0x9: pszPMPC = " (PM-TE0)"; break;
		case 0xA: pszPMPC = " (PM-ZLS0)"; break;
		case 0xB: pszPMPC = " (PM-ALIST0)"; break;
		case 0xC: pszPMPC = " (PM-VCE1)"; break;
		case 0xD: pszPMPC = " (PM-TE1)"; break;
		case 0xE: pszPMPC = " (PM-ZLS1)"; break;
		case 0xF: pszPMPC = " (PM-ALIST1)"; break;
	}

	return pszPMPC;
}

/*!
*******************************************************************************

 @Function	_RGXDecodeBIFReqTags

 @Description

 Decode the BIF Tag ID and sideband data fields from BIF_FAULT_BANK_REQ_STATUS regs

 @Input eBankID             - BIF identifier
 @Input ui32TagID           - Tag ID value
 @Input ui32TagSB           - Tag Sideband data
 @Output ppszTagID          - Decoded string from the Tag ID
 @Output ppszTagSB          - Decoded string from the Tag SB
 @Output pszScratchBuf      - Buffer provided to the function to generate the debug strings
 @Input ui32ScratchBufSize  - Size of the provided buffer

 @Return   void

******************************************************************************/
#include "rgxmhdefs_km.h"

static void _RGXDecodeBIFReqTagsXE(PVRSRV_RGXDEV_INFO	*psDevInfo,
								   IMG_UINT32	ui32TagID,
								   IMG_UINT32	ui32TagSB,
								   IMG_CHAR		**ppszTagID,
								   IMG_CHAR		**ppszTagSB,
								   IMG_CHAR		*pszScratchBuf,
								   IMG_UINT32	ui32ScratchBufSize)
{
	/* default to unknown */
	IMG_CHAR *pszTagID = "-";
	IMG_CHAR *pszTagSB = "-";
	IMG_BOOL bNewTagEncoding = IMG_FALSE;

	PVR_ASSERT(ppszTagID != NULL);
	PVR_ASSERT(ppszTagSB != NULL);

	/* tags updated for all cores (auto & consumer) with branch > 36 or only auto cores with branch = 36 */
	if ((psDevInfo->sDevFeatureCfg.ui32B > 36) ||
	    (RGX_IS_FEATURE_SUPPORTED(psDevInfo, TILE_REGION_PROTECTION) && (psDevInfo->sDevFeatureCfg.ui32B == 36)))
	{
		bNewTagEncoding = IMG_TRUE;
	}

	switch (ui32TagID)
	{
		/* MMU tags */
		case RGX_MH_TAG_ENCODING_MH_TAG_MMU:
		case RGX_MH_TAG_ENCODING_MH_TAG_CPU_MMU:
		case RGX_MH_TAG_ENCODING_MH_TAG_CPU_IFU:
		case RGX_MH_TAG_ENCODING_MH_TAG_CPU_LSU:
		{
			switch (ui32TagID)
			{
				case RGX_MH_TAG_ENCODING_MH_TAG_MMU:	    pszTagID = "MMU"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_CPU_MMU:	pszTagID = "CPU MMU"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_CPU_IFU:	pszTagID = "CPU IFU"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_CPU_LSU:	pszTagID = "CPU LSU"; break;
			}
			switch (ui32TagSB)
			{
				case RGX_MH_TAG_SB_MMU_ENCODING_MMU_TAG_PT_REQUEST:		pszTagSB = "PT"; break;
				case RGX_MH_TAG_SB_MMU_ENCODING_MMU_TAG_PD_REQUEST:		pszTagSB = "PD"; break;
				case RGX_MH_TAG_SB_MMU_ENCODING_MMU_TAG_PC_REQUEST:		pszTagSB = "PC"; break;
				case RGX_MH_TAG_SB_MMU_ENCODING_MMU_TAG_PM_PT_REQUEST:	pszTagSB = "PM PT"; break;
				case RGX_MH_TAG_SB_MMU_ENCODING_MMU_TAG_PM_PD_REQUEST:	pszTagSB = "PM PD"; break;
				case RGX_MH_TAG_SB_MMU_ENCODING_MMU_TAG_PM_PC_REQUEST:	pszTagSB = "PM PC"; break;
				case RGX_MH_TAG_SB_MMU_ENCODING_MMU_TAG_PM_PD_WREQUEST:	pszTagSB = "PM PD W"; break;
				case RGX_MH_TAG_SB_MMU_ENCODING_MMU_TAG_PM_PC_WREQUEST:	pszTagSB = "PM PC W"; break;
			}
			break;
		}

		/* MIPS */
		case RGX_MH_TAG_ENCODING_MH_TAG_MIPS:
		{
			pszTagID = "MIPS";
			switch (ui32TagSB)
			{
				case RGX_MH_TAG_SB_MIPS_ENCODING_MIPS_TAG_OPCODE_FETCH:	pszTagSB = "Opcode"; break;
				case RGX_MH_TAG_SB_MIPS_ENCODING_MIPS_TAG_DATA_ACCESS:	pszTagSB = "Data"; break;
			}
			break;
		}

		/* CDM tags */
		case RGX_MH_TAG_ENCODING_MH_TAG_CDM_STG0:
		case RGX_MH_TAG_ENCODING_MH_TAG_CDM_STG1:
		case RGX_MH_TAG_ENCODING_MH_TAG_CDM_STG2:
		case RGX_MH_TAG_ENCODING_MH_TAG_CDM_STG3:
		{
			switch (ui32TagID)
			{
				case RGX_MH_TAG_ENCODING_MH_TAG_CDM_STG0:	pszTagID = "CDM Stage 0"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_CDM_STG1:	pszTagID = "CDM Stage 1"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_CDM_STG2:	pszTagID = "CDM Stage 2"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_CDM_STG3:	pszTagID = "CDM Stage 3"; break;
			}
			switch (ui32TagSB)
			{
				case RGX_MH_TAG_SB_CDM_ENCODING_CDM_TAG_CONTROL_STREAM:	pszTagSB = "Control"; break;
				case RGX_MH_TAG_SB_CDM_ENCODING_CDM_TAG_INDIRECT_DATA:	pszTagSB = "Indirect"; break;
				case RGX_MH_TAG_SB_CDM_ENCODING_CDM_TAG_EVENT_DATA:		pszTagSB = "Event"; break;
				case RGX_MH_TAG_SB_CDM_ENCODING_CDM_TAG_CONTEXT_STATE:	pszTagSB = "Context"; break;
			}
			break;
		}

		/* VDM tags */
		case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG0:
		case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG1:
		case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG2:
		case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG3:
		case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG4:
		{
			switch (ui32TagID)
			{
				case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG0:	pszTagID = "VDM Stage 0"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG1:	pszTagID = "VDM Stage 1"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG2:	pszTagID = "VDM Stage 2"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG3:	pszTagID = "VDM Stage 3"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG4:	pszTagID = "VDM Stage 4"; break;
			}
			switch (ui32TagSB)
			{
				case RGX_MH_TAG_SB_VDM_ENCODING_VDM_TAG_CONTROL:	pszTagSB = "Control"; break;
				case RGX_MH_TAG_SB_VDM_ENCODING_VDM_TAG_STATE:		pszTagSB = "State"; break;
				case RGX_MH_TAG_SB_VDM_ENCODING_VDM_TAG_INDEX:		pszTagSB = "Index"; break;
				case RGX_MH_TAG_SB_VDM_ENCODING_VDM_TAG_STACK:		pszTagSB = "Stack"; break;
				case RGX_MH_TAG_SB_VDM_ENCODING_VDM_TAG_CONTEXT:	pszTagSB = "Context"; break;
			}
			break;
		}

		/* PDS */
		case RGX_MH_TAG_ENCODING_MH_TAG_PDS_0:
			pszTagID = "PDS req 0"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_PDS_1:
			pszTagID = "PDS req 1"; break;

		/* MCU */
		case RGX_MH_TAG_ENCODING_MH_TAG_MCU_USCA:
			pszTagID = "MCU USCA"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_MCU_USCB:
			pszTagID = "MCU USCB"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_MCU_USCC:
			pszTagID = "MCU USCC"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_MCU_USCD:
			pszTagID = "MCU USCD"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_MCU_PDS_USCA:
			pszTagID = "MCU PDS USCA"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_MCU_PDS_USCB:
			pszTagID = "MCU PDS USCB"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_MCU_PDS_USCC:
			pszTagID = "MCU PDS USCC"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_MCU_PDS_USCD:
			pszTagID = "MCU PDSUSCD"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_MCU_PDSRW:
			pszTagID = "MCU PDS PDSRW"; break;

		/* TCU */
		case RGX_MH_TAG_ENCODING_MH_TAG_TCU_0:
			pszTagID = "TCU req 0"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_TCU_1:
			pszTagID = "TCU req 1"; break;

		/* FBCDC */
		case RGX_MH_TAG_ENCODING_MH_TAG_FBCDC_0:
			pszTagID = bNewTagEncoding ? "TFBDC_TCU0" : "FBCDC0"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_FBCDC_1:
			pszTagID = bNewTagEncoding ? "TFBDC_ZLS0" : "FBCDC1"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_FBCDC_2:
			pszTagID = bNewTagEncoding ? "TFBDC_TCU1" : "FBCDC2"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_FBCDC_3:
			pszTagID = bNewTagEncoding ? "TFBDC_ZLS1" : "FBCDC3"; break;

		/* USC Shared */
		case RGX_MH_TAG_ENCODING_MH_TAG_USC:
			pszTagID = "USCS"; break;

		/* ISP */
		case RGX_MH_TAG_ENCODING_MH_TAG_ISP_ZLS:
			pszTagID = "ISP0 ZLS"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_ISP_DS:
			pszTagID = "ISP0 DS"; break;

		/* TPF */
		case RGX_MH_TAG_ENCODING_MH_TAG_TPF:
		case RGX_MH_TAG_ENCODING_MH_TAG_TPF_PBCDBIAS:
		case RGX_MH_TAG_ENCODING_MH_TAG_TPF_SPF:
		{
			switch (ui32TagID)
			{
				case RGX_MH_TAG_ENCODING_MH_TAG_TPF:           pszTagID = "TPF0"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_TPF_PBCDBIAS:  pszTagID = "TPF0 DBIAS"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_TPF_SPF:       pszTagID = "TPF0 SPF"; break;
			}
			switch (ui32TagSB)
			{
				case RGX_MH_TAG_SB_TPF_ENCODING_TPF_TAG_PDS_STATE:	pszTagSB = "PDS state"; break;
				case RGX_MH_TAG_SB_TPF_ENCODING_TPF_TAG_DEPTH_BIAS:	pszTagSB = "Depth bias"; break;
				case RGX_MH_TAG_SB_TPF_ENCODING_TPF_TAG_FLOOR_OFFSET_DATA:	pszTagSB = "Floor offset"; break;
				case RGX_MH_TAG_SB_TPF_ENCODING_TPF_TAG_DELTA_DATA:	pszTagSB = "Delta"; break;
			}
			break;
		}

		/* IPF */
		case RGX_MH_TAG_ENCODING_MH_TAG_IPF_CREQ:
		case RGX_MH_TAG_ENCODING_MH_TAG_IPF_OTHERS:
		{
			switch (ui32TagID)
			{
				case RGX_MH_TAG_ENCODING_MH_TAG_IPF_CREQ:      pszTagID = "IPF0"; break;
				case RGX_MH_TAG_ENCODING_MH_TAG_IPF_OTHERS:    pszTagID = "IPF0"; break;
			}

			if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_ISP_IPP_PIPES))
			{
				if (ui32TagID < RGX_GET_FEATURE_VALUE(psDevInfo, NUM_ISP_IPP_PIPES))
				{
					OSSNPrintf(pszScratchBuf, ui32ScratchBufSize, "CReq%d", ui32TagID);
					pszTagSB = pszScratchBuf;
				}
				else if (ui32TagID < 2 * RGX_GET_FEATURE_VALUE(psDevInfo, NUM_ISP_IPP_PIPES))
				{
					ui32TagID -= RGX_GET_FEATURE_VALUE(psDevInfo, NUM_ISP_IPP_PIPES);
					OSSNPrintf(pszScratchBuf, ui32ScratchBufSize, "PReq%d", ui32TagID);
					pszTagSB = pszScratchBuf;
				}
				else
				{
					switch (ui32TagSB - 2 * RGX_GET_FEATURE_VALUE(psDevInfo, NUM_ISP_IPP_PIPES))
					{
						case 0:	pszTagSB = "RReq"; break;
						case 1:	pszTagSB = "DBSC"; break;
						case 2:	pszTagSB = "CPF"; break;
						case 3:	pszTagSB = "Delta"; break;
					}
				}
			}
			break;
		}

		/* VDM Stage 5 (temporary) */
		case RGX_MH_TAG_ENCODING_MH_TAG_VDM_STG5:
			pszTagID = "VDM Stage 5"; break;

		/* TA */
		case RGX_MH_TAG_ENCODING_MH_TAG_TA_PPP:
			pszTagID = "PPP"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_TA_TPWRTC:
			pszTagID = "TPW RTC"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_TA_TEACRTC:
			pszTagID = "TEAC RTC"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_TA_PSGRTC:
			pszTagID = "PSG RTC"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_TA_PSGREGION:
			pszTagID = "PSG Region"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_TA_PSGSTREAM:
			pszTagID = "PSG Stream"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_TA_TPW:
			pszTagID = "TPW"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_TA_TPC:
			pszTagID = "TPC"; break;

		/* PM */
		case RGX_MH_TAG_ENCODING_MH_TAG_PM_ALLOC:
		{
			pszTagID = "PMA";
			switch (ui32TagSB)
			{
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_TAFSTACK:	pszTagSB = "TA Fstack"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_TAMLIST:		pszTagSB = "TA MList"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_3DFSTACK:	pszTagSB = "3D Fstack"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_3DMLIST:		pszTagSB = "3D MList"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_PMCTX0:		pszTagSB = "Context0"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_PMCTX1:		pszTagSB = "Context1"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_MAVP:		pszTagSB = "MAVP"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_UFSTACK:		pszTagSB = "UFstack"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_TAMMUSTACK:	pszTagSB = "TA MMUstack"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_3DMMUSTACK:	pszTagSB = "3D MMUstack"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_TAUFSTACK:	pszTagSB = "TA UFstack"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_3DUFSTACK:	pszTagSB = "3D UFstack"; break;
				case RGX_MH_TAG_SB_PMA_ENCODING_PM_TAG_PMA_TAVFP:		pszTagSB = "TA VFP"; break;
			}
			break;
		}
		case RGX_MH_TAG_ENCODING_MH_TAG_PM_DEALLOC:
		{
			pszTagID = "PMD";
			switch (ui32TagSB)
			{
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_TAFSTACK:	pszTagSB = "TA Fstack"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_TAMLIST:		pszTagSB = "TA MList"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_3DFSTACK:	pszTagSB = "3D Fstack"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_3DMLIST:		pszTagSB = "3D MList"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_PMCTX0:		pszTagSB = "Context0"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_PMCTX1:		pszTagSB = "Context1"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_UFSTACK:		pszTagSB = "UFstack"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_TAMMUSTACK:	pszTagSB = "TA MMUstack"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_3DMMUSTACK:	pszTagSB = "3D MMUstack"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_TAUFSTACK:	pszTagSB = "TA UFstack"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_3DUFSTACK:	pszTagSB = "3D UFstack"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_TAVFP:		pszTagSB = "TA VFP"; break;
				case RGX_MH_TAG_SB_PMD_ENCODING_PM_TAG_PMD_3DVFP:		pszTagSB = "3D VFP"; break;
			}
			break;
		}

		/* TDM */
		case RGX_MH_TAG_ENCODING_MH_TAG_TDM_DMA:
		{
			pszTagID = "TDM DMA";
			switch (ui32TagSB)
			{
				case RGX_MH_TAG_SB_TDM_DMA_ENCODING_TDM_DMA_TAG_CTL_STREAM: pszTagSB = "Ctl stream"; break;
				case RGX_MH_TAG_SB_TDM_DMA_ENCODING_TDM_DMA_TAG_CTX_BUFFER: pszTagSB = "Ctx buffer"; break;
				case RGX_MH_TAG_SB_TDM_DMA_ENCODING_TDM_DMA_TAG_QUEUE_CTL:  pszTagSB = "Queue ctl"; break;
			}
			break;
		}
		case RGX_MH_TAG_ENCODING_MH_TAG_TDM_CTL:
		{
			pszTagID = "TDM CTL";
			switch (ui32TagSB)
			{
				case RGX_MH_TAG_SB_TDM_CTL_ENCODING_TDM_CTL_TAG_FENCE:   pszTagSB = "Fence"; break;
				case RGX_MH_TAG_SB_TDM_CTL_ENCODING_TDM_CTL_TAG_CONTEXT: pszTagSB = "Context"; break;
				case RGX_MH_TAG_SB_TDM_CTL_ENCODING_TDM_CTL_TAG_QUEUE:   pszTagSB = "Queue"; break;
			}
			break;
		}

		/* PBE */
		case RGX_MH_TAG_ENCODING_MH_TAG_PBE0:
			pszTagID = "PBE0"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_PBE1:
			pszTagID = "PBE1"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_PBE2:
			pszTagID = "PBE2"; break;
		case RGX_MH_TAG_ENCODING_MH_TAG_PBE3:
			pszTagID = "PBE3"; break;
	}

	*ppszTagID = pszTagID;
	*ppszTagSB = pszTagSB;
}

/* RISC-V pf tags */
#define RGX_MH_TAG_ENCODING_MH_TAG_CPU_MMU  (0x00000001U)
#define RGX_MH_TAG_ENCODING_MH_TAG_CPU_IFU  (0x00000002U)
#define RGX_MH_TAG_ENCODING_MH_TAG_CPU_LSU  (0x00000003U)

static void _RGXDecodeBIFReqTagsFwcore(PVRSRV_RGXDEV_INFO *psDevInfo,
									   IMG_UINT32 ui32TagID,
									   IMG_UINT32 ui32TagSB,
									   IMG_CHAR **ppszTagID,
									   IMG_CHAR **ppszTagSB)
{
	/* default to unknown */
	IMG_CHAR *pszTagID = "-";
	IMG_CHAR *pszTagSB = "-";
	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
	{
		pszTagSB = "RISC-V";

		switch (ui32TagID)
		{
			case RGX_MH_TAG_ENCODING_MH_TAG_CPU_MMU:	pszTagID = "RISC-V MMU"; break;
			case RGX_MH_TAG_ENCODING_MH_TAG_CPU_IFU:	pszTagID = "RISC-V Instruction Fetch Unit"; break;
			case RGX_MH_TAG_ENCODING_MH_TAG_CPU_LSU:	pszTagID = "RISC-V Load/Store Unit"; break; /* Or Debug Module System Bus */
		}
	}

	*ppszTagID = pszTagID;
	*ppszTagSB = pszTagSB;
}

static void _RGXDecodeBIFReqTags(PVRSRV_RGXDEV_INFO	*psDevInfo,
								 RGXDBG_BIF_ID	eBankID,
								 IMG_UINT32		ui32TagID,
								 IMG_UINT32		ui32TagSB,
								 IMG_CHAR		**ppszTagID,
								 IMG_CHAR		**ppszTagSB,
								 IMG_CHAR		*pszScratchBuf,
								 IMG_UINT32		ui32ScratchBufSize)
{
	/* default to unknown */
	IMG_CHAR *pszTagID = "-";
	IMG_CHAR *pszTagSB = "-";

	PVR_ASSERT(ppszTagID != NULL);
	PVR_ASSERT(ppszTagSB != NULL);

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, XE_MEMORY_HIERARCHY))
	{
		if (eBankID == RGXDBG_FWCORE)
		{
			_RGXDecodeBIFReqTagsFwcore(psDevInfo, ui32TagID, ui32TagSB, ppszTagID, ppszTagSB);
		}
		else
		{
			_RGXDecodeBIFReqTagsXE(psDevInfo, ui32TagID, ui32TagSB, ppszTagID, ppszTagSB, pszScratchBuf, ui32ScratchBufSize);
		}
		return;
	}

	switch (ui32TagID)
	{
		case 0x0:
		{
			pszTagID = "MMU";
			switch (ui32TagSB)
			{
				case 0x0: pszTagSB = "Table"; break;
				case 0x1: pszTagSB = "Directory"; break;
				case 0x2: pszTagSB = "Catalogue"; break;
			}
			break;
		}
		case 0x1:
		{
			pszTagID = "TLA";
			switch (ui32TagSB)
			{
				case 0x0: pszTagSB = "Pixel data"; break;
				case 0x1: pszTagSB = "Command stream data"; break;
				case 0x2: pszTagSB = "Fence or flush"; break;
			}
			break;
		}
		case 0x2:
		{
			pszTagID = "HOST";
			break;
		}
		case 0x3:
		{
			if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, META))
			{
					pszTagID = "META";
					switch (ui32TagSB)
					{
						case 0x0: pszTagSB = "DCache - Thread 0"; break;
						case 0x1: pszTagSB = "ICache - Thread 0"; break;
						case 0x2: pszTagSB = "JTag - Thread 0"; break;
						case 0x3: pszTagSB = "Slave bus - Thread 0"; break;
						case 0x4: pszTagSB = "DCache - Thread "; break;
						case 0x5: pszTagSB = "ICache - Thread 1"; break;
						case 0x6: pszTagSB = "JTag - Thread 1"; break;
						case 0x7: pszTagSB = "Slave bus - Thread 1"; break;
					}
			}
			else if (RGX_IS_ERN_SUPPORTED(psDevInfo, 57596))
			{
				pszTagID="TCU";
			}
			else
			{
				/* Unreachable code */
				PVR_ASSERT(IMG_FALSE);
			}
			break;
		}
		case 0x4:
		{
			pszTagID = "USC";
			OSSNPrintf(pszScratchBuf, ui32ScratchBufSize,
			           "Cache line %d", (ui32TagSB & 0x3f));
			pszTagSB = pszScratchBuf;
			break;
		}
		case 0x5:
		{
			pszTagID = "PBE";
			break;
		}
		case 0x6:
		{
			pszTagID = "ISP";
			switch (ui32TagSB)
			{
				case 0x00: pszTagSB = "ZLS"; break;
				case 0x20: pszTagSB = "Occlusion Query"; break;
			}
			break;
		}
		case 0x7:
		{
			if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, CLUSTER_GROUPING))
			{
				if (eBankID == RGXDBG_TEXAS_BIF)
				{
					pszTagID = "IPF";
					switch (ui32TagSB)
					{
						case 0x0: pszTagSB = "CPF"; break;
						case 0x1: pszTagSB = "DBSC"; break;
						case 0x2:
						case 0x4:
						case 0x6:
						case 0x8: pszTagSB = "Control Stream"; break;
						case 0x3:
						case 0x5:
						case 0x7:
						case 0x9: pszTagSB = "Primitive Block"; break;
					}
				}
				else
				{
					pszTagID = "IPP";
					switch (ui32TagSB)
					{
						case 0x0: pszTagSB = "Macrotile Header"; break;
						case 0x1: pszTagSB = "Region Header"; break;
					}
				}
			}
			else if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, SIMPLE_INTERNAL_PARAMETER_FORMAT))
			{
				pszTagID = "IPF";
				switch (ui32TagSB)
				{
					case 0x0: pszTagSB = "Region Header"; break;
					case 0x1: pszTagSB = "DBSC"; break;
					case 0x2: pszTagSB = "CPF"; break;
					case 0x3: pszTagSB = "Control Stream"; break;
					case 0x4: pszTagSB = "Primitive Block"; break;
				}
			}
			else
			{
				pszTagID = "IPF";
				switch (ui32TagSB)
				{
					case 0x0: pszTagSB = "Macrotile Header"; break;
					case 0x1: pszTagSB = "Region Header"; break;
					case 0x2: pszTagSB = "DBSC"; break;
					case 0x3: pszTagSB = "CPF"; break;
					case 0x4:
					case 0x6:
					case 0x8: pszTagSB = "Control Stream"; break;
					case 0x5:
					case 0x7:
					case 0x9: pszTagSB = "Primitive Block"; break;
				}
			}
			break;
		}
		case 0x8:
		{
			pszTagID = "CDM";
			switch (ui32TagSB)
			{
				case 0x0: pszTagSB = "Control Stream"; break;
				case 0x1: pszTagSB = "Indirect Data"; break;
				case 0x2: pszTagSB = "Event Write"; break;
				case 0x3: pszTagSB = "Context State"; break;
			}
			break;
		}
		case 0x9:
		{
			pszTagID = "VDM";
			switch (ui32TagSB)
			{
				case 0x0: pszTagSB = "Control Stream"; break;
				case 0x1: pszTagSB = "PPP State"; break;
				case 0x2: pszTagSB = "Index Data"; break;
				case 0x4: pszTagSB = "Call Stack"; break;
				case 0x8: pszTagSB = "Context State"; break;
			}
			break;
		}
		case 0xA:
		{
			pszTagID = "PM";
			switch (ui32TagSB)
			{
				case 0x0: pszTagSB = "PMA_TAFSTACK"; break;
				case 0x1: pszTagSB = "PMA_TAMLIST"; break;
				case 0x2: pszTagSB = "PMA_3DFSTACK"; break;
				case 0x3: pszTagSB = "PMA_3DMLIST"; break;
				case 0x4: pszTagSB = "PMA_PMCTX0"; break;
				case 0x5: pszTagSB = "PMA_PMCTX1"; break;
				case 0x6: pszTagSB = "PMA_MAVP"; break;
				case 0x7: pszTagSB = "PMA_UFSTACK"; break;
				case 0x8: pszTagSB = "PMD_TAFSTACK"; break;
				case 0x9: pszTagSB = "PMD_TAMLIST"; break;
				case 0xA: pszTagSB = "PMD_3DFSTACK"; break;
				case 0xB: pszTagSB = "PMD_3DMLIST"; break;
				case 0xC: pszTagSB = "PMD_PMCTX0"; break;
				case 0xD: pszTagSB = "PMD_PMCTX1"; break;
				case 0xF: pszTagSB = "PMD_UFSTACK"; break;
				case 0x10: pszTagSB = "PMA_TAMMUSTACK"; break;
				case 0x11: pszTagSB = "PMA_3DMMUSTACK"; break;
				case 0x12: pszTagSB = "PMD_TAMMUSTACK"; break;
				case 0x13: pszTagSB = "PMD_3DMMUSTACK"; break;
				case 0x14: pszTagSB = "PMA_TAUFSTACK"; break;
				case 0x15: pszTagSB = "PMA_3DUFSTACK"; break;
				case 0x16: pszTagSB = "PMD_TAUFSTACK"; break;
				case 0x17: pszTagSB = "PMD_3DUFSTACK"; break;
				case 0x18: pszTagSB = "PMA_TAVFP"; break;
				case 0x19: pszTagSB = "PMD_3DVFP"; break;
				case 0x1A: pszTagSB = "PMD_TAVFP"; break;
			}
			break;
		}
		case 0xB:
		{
			pszTagID = "TA";
			switch (ui32TagSB)
			{
				case 0x1: pszTagSB = "VCE"; break;
				case 0x2: pszTagSB = "TPC"; break;
				case 0x3: pszTagSB = "TE Control Stream"; break;
				case 0x4: pszTagSB = "TE Region Header"; break;
				case 0x5: pszTagSB = "TE Render Target Cache"; break;
				case 0x6: pszTagSB = "TEAC Render Target Cache"; break;
				case 0x7: pszTagSB = "VCE Render Target Cache"; break;
				case 0x8: pszTagSB = "PPP Context State"; break;
			}
			break;
		}
		case 0xC:
		{
			pszTagID = "TPF";
			switch (ui32TagSB)
			{
				case 0x0: pszTagSB = "TPF0: Primitive Block"; break;
				case 0x1: pszTagSB = "TPF0: Depth Bias"; break;
				case 0x2: pszTagSB = "TPF0: Per Primitive IDs"; break;
				case 0x3: pszTagSB = "CPF - Tables"; break;
				case 0x4: pszTagSB = "TPF1: Primitive Block"; break;
				case 0x5: pszTagSB = "TPF1: Depth Bias"; break;
				case 0x6: pszTagSB = "TPF1: Per Primitive IDs"; break;
				case 0x7: pszTagSB = "CPF - Data: Pipe 0"; break;
				case 0x8: pszTagSB = "TPF2: Primitive Block"; break;
				case 0x9: pszTagSB = "TPF2: Depth Bias"; break;
				case 0xA: pszTagSB = "TPF2: Per Primitive IDs"; break;
				case 0xB: pszTagSB = "CPF - Data: Pipe 1"; break;
				case 0xC: pszTagSB = "TPF3: Primitive Block"; break;
				case 0xD: pszTagSB = "TPF3: Depth Bias"; break;
				case 0xE: pszTagSB = "TPF3: Per Primitive IDs"; break;
				case 0xF: pszTagSB = "CPF - Data: Pipe 2"; break;
			}
			break;
		}
		case 0xD:
		{
			pszTagID = "PDS";
			break;
		}
		case 0xE:
		{
			pszTagID = "MCU";
			{
				IMG_UINT32 ui32Burst = (ui32TagSB >> 5) & 0x7;
				IMG_UINT32 ui32GroupEnc = (ui32TagSB >> 2) & 0x7;
				IMG_UINT32 ui32Group = ui32TagSB & 0x3;

				IMG_CHAR* pszBurst = "";
				IMG_CHAR* pszGroupEnc = "";
				IMG_CHAR* pszGroup = "";

				switch (ui32Burst)
				{
					case 0x0:
					case 0x1: pszBurst = "128bit word within the Lower 256bits"; break;
					case 0x2:
					case 0x3: pszBurst = "128bit word within the Upper 256bits"; break;
					case 0x4: pszBurst = "Lower 256bits"; break;
					case 0x5: pszBurst = "Upper 256bits"; break;
					case 0x6: pszBurst = "512 bits"; break;
				}
				switch (ui32GroupEnc)
				{
					case 0x0: pszGroupEnc = "TPUA_USC"; break;
					case 0x1: pszGroupEnc = "TPUB_USC"; break;
					case 0x2: pszGroupEnc = "USCA_USC"; break;
					case 0x3: pszGroupEnc = "USCB_USC"; break;
					case 0x4: pszGroupEnc = "PDS_USC"; break;
					case 0x5:
						if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_CLUSTERS) &&
							6 > RGX_GET_FEATURE_VALUE(psDevInfo, NUM_CLUSTERS))
						{
							pszGroupEnc = "PDSRW";
						} else if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_CLUSTERS) &&
							6 == RGX_GET_FEATURE_VALUE(psDevInfo, NUM_CLUSTERS))
						{
							pszGroupEnc = "UPUC_USC";
						}
						break;
					case 0x6:
						if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_CLUSTERS) &&
							6 == RGX_GET_FEATURE_VALUE(psDevInfo, NUM_CLUSTERS))
						{
							pszGroupEnc = "TPUC_USC";
						}
						break;
					case 0x7:
						if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_CLUSTERS) &&
							6 == RGX_GET_FEATURE_VALUE(psDevInfo, NUM_CLUSTERS))
						{
							pszGroupEnc = "PDSRW";
						}
						break;
				}
				switch (ui32Group)
				{
					case 0x0: pszGroup = "Banks 0-3"; break;
					case 0x1: pszGroup = "Banks 4-7"; break;
					case 0x2: pszGroup = "Banks 8-11"; break;
					case 0x3: pszGroup = "Banks 12-15"; break;
				}

				OSSNPrintf(pszScratchBuf, ui32ScratchBufSize,
								"%s, %s, %s", pszBurst, pszGroupEnc, pszGroup);
				pszTagSB = pszScratchBuf;
			}
			break;
		}
		case 0xF:
		{
			pszTagID = "FB_CDC";

			if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, XT_TOP_INFRASTRUCTURE))
			{
				IMG_UINT32 ui32Req   = (ui32TagSB >> 0) & 0xf;
				IMG_UINT32 ui32MCUSB = (ui32TagSB >> 4) & 0x3;
				IMG_CHAR* pszReqOrig = "";

				switch (ui32Req)
				{
					case 0x0: pszReqOrig = "FBC Request, originator ZLS"; break;
					case 0x1: pszReqOrig = "FBC Request, originator PBE"; break;
					case 0x2: pszReqOrig = "FBC Request, originator Host"; break;
					case 0x3: pszReqOrig = "FBC Request, originator TLA"; break;
					case 0x4: pszReqOrig = "FBDC Request, originator ZLS"; break;
					case 0x5: pszReqOrig = "FBDC Request, originator MCU"; break;
					case 0x6: pszReqOrig = "FBDC Request, originator Host"; break;
					case 0x7: pszReqOrig = "FBDC Request, originator TLA"; break;
					case 0x8: pszReqOrig = "FBC Request, originator ZLS Requester Fence"; break;
					case 0x9: pszReqOrig = "FBC Request, originator PBE Requester Fence"; break;
					case 0xa: pszReqOrig = "FBC Request, originator Host Requester Fence"; break;
					case 0xb: pszReqOrig = "FBC Request, originator TLA Requester Fence"; break;
					case 0xc: pszReqOrig = "Reserved"; break;
					case 0xd: pszReqOrig = "Reserved"; break;
					case 0xe: pszReqOrig = "FBDC Request, originator FBCDC(Host) Memory Fence"; break;
					case 0xf: pszReqOrig = "FBDC Request, originator FBCDC(TLA) Memory Fence"; break;
				}
				OSSNPrintf(pszScratchBuf, ui32ScratchBufSize,
				           "%s, MCU sideband 0x%X", pszReqOrig, ui32MCUSB);
				pszTagSB = pszScratchBuf;
			}
			else
			{
				IMG_UINT32 ui32Req   = (ui32TagSB >> 2) & 0x7;
				IMG_UINT32 ui32MCUSB = (ui32TagSB >> 0) & 0x3;
				IMG_CHAR* pszReqOrig = "";

				switch (ui32Req)
				{
					case 0x0: pszReqOrig = "FBC Request, originator ZLS";   break;
					case 0x1: pszReqOrig = "FBC Request, originator PBE";   break;
					case 0x2: pszReqOrig = "FBC Request, originator Host";  break;
					case 0x3: pszReqOrig = "FBC Request, originator TLA";   break;
					case 0x4: pszReqOrig = "FBDC Request, originator ZLS";  break;
					case 0x5: pszReqOrig = "FBDC Request, originator MCU";  break;
					case 0x6: pszReqOrig = "FBDC Request, originator Host"; break;
					case 0x7: pszReqOrig = "FBDC Request, originator TLA";  break;
				}
				OSSNPrintf(pszScratchBuf, ui32ScratchBufSize,
				           "%s, MCU sideband 0x%X", pszReqOrig, ui32MCUSB);
				pszTagSB = pszScratchBuf;
			}
			break;
		}
	} /* switch (TagID) */

	*ppszTagID = pszTagID;
	*ppszTagSB = pszTagSB;
}


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

 @Function	_RGXDumpRGXBIFBank

 @Description

 Dump BIF Bank state in human readable form.

 @Input pfnDumpDebugPrintf   - The debug printf function
 @Input pvDumpDebugFile      - Optional file identifier to be passed to the
                               'printf' function if required
 @Input psDevInfo            - RGX device info
 @Input eBankID              - BIF identifier
 @Input ui64MMUStatus        - MMU Status register value
 @Input ui64ReqStatus        - BIF request Status register value
 @Return   void

******************************************************************************/
static void _RGXDumpRGXBIFBank(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile,
					PVRSRV_RGXDEV_INFO *psDevInfo,
					RGXDBG_BIF_ID eBankID,
					IMG_UINT64 ui64MMUStatus,
					IMG_UINT64 ui64ReqStatus,
					const IMG_CHAR *pszIndent)
{
	if (ui64MMUStatus == 0x0)
	{
		PVR_DUMPDEBUG_LOG("%s - OK", pszBIFNames[eBankID]);
	}
	else
	{
		IMG_UINT32 ui32PageSize;
		IMG_UINT32 ui32PC =
			(ui64MMUStatus & ~RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_CAT_BASE_CLRMSK) >>
				RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_CAT_BASE_SHIFT;

		/* Bank 0 & 1 share the same fields */
		PVR_DUMPDEBUG_LOG("%s%s - FAULT:",
						  pszIndent,
						  pszBIFNames[eBankID]);

		/* MMU Status */
		{
			IMG_UINT32 ui32MMUDataType =
				(ui64MMUStatus & ~RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_DATA_TYPE_CLRMSK) >>
					RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_DATA_TYPE_SHIFT;

			IMG_BOOL bROFault = (ui64MMUStatus & RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_FAULT_RO_EN) != 0;
			IMG_BOOL bProtFault = (ui64MMUStatus & RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_FAULT_PM_META_RO_EN) != 0;

			ui32PageSize = (ui64MMUStatus & ~RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_PAGE_SIZE_CLRMSK) >>
						RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_PAGE_SIZE_SHIFT;

			PVR_DUMPDEBUG_LOG("%s  * MMU status (0x%016" IMG_UINT64_FMTSPECx "): PC = %d%s, Page Size = %d%s%s%s.",
						pszIndent,
						ui64MMUStatus,
						ui32PC,
						(ui32PC < 0x8)?"":_RGXDecodePMPC(ui32PC),
						ui32PageSize,
						(bROFault)?", Read Only fault":"",
						(bProtFault)?", PM/META protection fault":"",
						_RGXDecodeMMULevel(ui32MMUDataType));
		}

		/* Req Status */
		{
			IMG_CHAR *pszTagID;
			IMG_CHAR *pszTagSB;
			IMG_CHAR aszScratch[RGX_DEBUG_STR_SIZE];
			IMG_BOOL bRead;
			IMG_UINT32 ui32TagSB, ui32TagID;
			IMG_UINT64 ui64Addr;

			if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, XE_MEMORY_HIERARCHY))
			{
				bRead = (ui64ReqStatus & RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__RNW_EN) != 0;
				ui32TagSB = (ui64ReqStatus & ~RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__TAG_SB_CLRMSK) >>
					RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__TAG_SB_SHIFT;
				ui32TagID = (ui64ReqStatus & ~RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__TAG_ID_CLRMSK) >>
					RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__TAG_ID_SHIFT;
			}
			else
			{
				bRead = (ui64ReqStatus & RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_RNW_EN) != 0;
				ui32TagSB = (ui64ReqStatus & ~RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_TAG_SB_CLRMSK) >>
					RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_TAG_SB_SHIFT;
				ui32TagID = (ui64ReqStatus & ~RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_TAG_ID_CLRMSK) >>
					RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_TAG_ID_SHIFT;
			}
			ui64Addr = ((ui64ReqStatus & ~RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_ADDRESS_CLRMSK) >>
				RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_ADDRESS_SHIFT) <<
				RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_ADDRESS_ALIGNSHIFT;

			_RGXDecodeBIFReqTags(psDevInfo, eBankID, ui32TagID, ui32TagSB, &pszTagID, &pszTagSB, &aszScratch[0], RGX_DEBUG_STR_SIZE);

			PVR_DUMPDEBUG_LOG("%s  * Request (0x%016" IMG_UINT64_FMTSPECx
						"): %s (%s), %s " IMG_DEV_VIRTADDR_FMTSPEC ".",
							  pszIndent,
							  ui64ReqStatus,
			                  pszTagID,
			                  pszTagSB,
			                  (bRead)?"Reading from":"Writing to",
			                  ui64Addr);
		}
	}
}
static_assert((RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__RNW_EN == RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_RNW_EN),
			  "RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_RNW_EN mismatch!");
static_assert((RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__TAG_SB_CLRMSK == RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_TAG_SB_CLRMSK),
			  "RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_TAG_SB_CLRMSK mismatch!");
static_assert((RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__TAG_SB_SHIFT == RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_TAG_SB_SHIFT),
			  "RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_TAG_SB_SHIFT mismatch!");
static_assert((RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__TAG_ID_CLRMSK == RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_TAG_ID_CLRMSK),
			  "RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_TAG_ID_CLRMSK mismatch!");
static_assert((RGX_CR_BIF_FAULT_BANK0_REQ_STATUS__XE_MEM__TAG_ID_SHIFT == RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_TAG_ID_SHIFT),
			  "RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_TAG_ID_SHIFT mismatch!");
static_assert((RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_ADDRESS_CLRMSK == RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_ADDRESS_CLRMSK),
			  "RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_ADDRESS_CLRMSK mismatch!");
static_assert((RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_ADDRESS_SHIFT == RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_ADDRESS_SHIFT),
			  "RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_ADDRESS_SHIFT mismatch!");
static_assert((RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_ADDRESS_ALIGNSHIFT == RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_ADDRESS_ALIGNSHIFT),
			  "RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_ADDRESS_ALIGNSHIFT mismatch!");

static_assert((RGX_CR_MMU_FAULT_STATUS_CONTEXT_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_CONTEXT_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_CONTEXT_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_CONTEXT_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_ADDRESS_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_ADDRESS_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_ADDRESS_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_ADDRESS_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_TAG_SB_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_TAG_SB_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_TAG_SB_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_TAG_SB_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_REQ_ID_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_REQ_ID_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_REQ_ID_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_REQ_ID_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_LEVEL_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_LEVEL_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_LEVEL_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_LEVEL_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_RNW_EN == RGX_CR_MMU_FAULT_STATUS_META_RNW_EN),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_FAULT_EN == RGX_CR_MMU_FAULT_STATUS_META_FAULT_EN),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_TYPE_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_TYPE_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_TYPE_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_TYPE_SHIFT),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_TYPE_CLRMSK == RGX_CR_MMU_FAULT_STATUS_META_TYPE_CLRMSK),
			  "RGX_CR_MMU_FAULT_STATUS_META mismatch!");
static_assert((RGX_CR_MMU_FAULT_STATUS_TYPE_SHIFT == RGX_CR_MMU_FAULT_STATUS_META_TYPE_SHIFT),
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
					case RGX_HWRTYPE_BIF0FAULT:
					case RGX_HWRTYPE_BIF1FAULT:
					{
						_RGXDumpRGXBIFBank(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, RGXFWIF_HWRTYPE_BIF_BANK_GET(psHWRInfo->eHWRType),
										psHWRInfo->uHWRData.sBIFInfo.ui64BIFMMUStatus,
										psHWRInfo->uHWRData.sBIFInfo.ui64BIFReqStatus,
										DD_NORMAL_INDENT);

						bPageFault = IMG_TRUE;
						sFaultDevVAddr.uiAddr = (psHWRInfo->uHWRData.sBIFInfo.ui64BIFReqStatus & ~RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_ADDRESS_CLRMSK);
						ui32PC = (psHWRInfo->uHWRData.sBIFInfo.ui64BIFMMUStatus & ~RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_CAT_BASE_CLRMSK) >>
								RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_CAT_BASE_SHIFT;
						bPMFault = (ui32PC >= 8);
						ui32PageSize = (psHWRInfo->uHWRData.sBIFInfo.ui64BIFMMUStatus & ~RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_PAGE_SIZE_CLRMSK) >>
									RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_PAGE_SIZE_SHIFT;
						sPCDevPAddr.uiAddr = psHWRInfo->uHWRData.sBIFInfo.ui64PCAddress;
					}
					break;
					case RGX_HWRTYPE_TEXASBIF0FAULT:
					{
						if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, CLUSTER_GROUPING))
						{
							_RGXDumpRGXBIFBank(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, RGXDBG_TEXAS_BIF,
										psHWRInfo->uHWRData.sBIFInfo.ui64BIFMMUStatus,
										psHWRInfo->uHWRData.sBIFInfo.ui64BIFReqStatus,
										DD_NORMAL_INDENT);

							bPageFault = IMG_TRUE;
							sFaultDevVAddr.uiAddr = (psHWRInfo->uHWRData.sBIFInfo.ui64BIFReqStatus & ~RGX_CR_BIF_FAULT_BANK0_REQ_STATUS_ADDRESS_CLRMSK);
							ui32PC = (psHWRInfo->uHWRData.sBIFInfo.ui64BIFMMUStatus & ~RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_CAT_BASE_CLRMSK) >>
									RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_CAT_BASE_SHIFT;
							bPMFault = (ui32PC >= 8);
							ui32PageSize = (psHWRInfo->uHWRData.sBIFInfo.ui64BIFMMUStatus & ~RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_PAGE_SIZE_CLRMSK) >>
										RGX_CR_BIF_FAULT_BANK0_MMU_STATUS_PAGE_SIZE_SHIFT;
							sPCDevPAddr.uiAddr = psHWRInfo->uHWRData.sBIFInfo.ui64PCAddress;
						}
					}
					break;

					case RGX_HWRTYPE_ECCFAULT:
					{
						PVR_DUMPDEBUG_LOG("    ECC fault GPU=0x%08x", psHWRInfo->uHWRData.sECCInfo.ui32FaultGPU);
					}
					break;

					case RGX_HWRTYPE_MMUFAULT:
					{
					}
					break;

					case RGX_HWRTYPE_MMUMETAFAULT:
					{
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

					case RGX_HWRTYPE_MMURISCVFAULT:
					{
						if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
						{
							_RGXDumpRGXBIFBank(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, RGXDBG_FWCORE,
											psHWRInfo->uHWRData.sBIFInfo.ui64BIFMMUStatus,
											psHWRInfo->uHWRData.sBIFInfo.ui64BIFReqStatus,
											DD_NORMAL_INDENT);

							bPageFault = IMG_TRUE;
							bPMFault = IMG_FALSE;
							sFaultDevVAddr.uiAddr =
								(psHWRInfo->uHWRData.sBIFInfo.ui64BIFReqStatus &
								 ~RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS_ADDRESS_CLRMSK);
							ui32PageSize =
								(psHWRInfo->uHWRData.sBIFInfo.ui64BIFMMUStatus &
								 ~RGX_CR_FWCORE_MEM_FAULT_MMU_STATUS_PAGE_SIZE_CLRMSK) >>
								RGX_CR_FWCORE_MEM_FAULT_MMU_STATUS_PAGE_SIZE_SHIFT;
							sPCDevPAddr.uiAddr = psHWRInfo->uHWRData.sBIFInfo.ui64PCAddress;
						}
					}
					break;

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

	ui64BIFMMUEntryStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_BIF_MMU_ENTRY_STATUS);

	psDevVAddr->uiAddr = (ui64BIFMMUEntryStatus & ~RGX_CR_BIF_MMU_ENTRY_STATUS_ADDRESS_CLRMSK);

	*pui32CatBase = (ui64BIFMMUEntryStatus & ~RGX_CR_BIF_MMU_ENTRY_STATUS_CAT_BASE_CLRMSK) >>
								RGX_CR_BIF_MMU_ENTRY_STATUS_CAT_BASE_SHIFT;
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
		{
			IMG_UINT64	ui64RegValMMUStatus, ui64RegValREQStatus;

			ui64RegValMMUStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_BIF_FAULT_BANK0_MMU_STATUS);
			ui64RegValREQStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_BIF_FAULT_BANK0_REQ_STATUS);

			_RGXDumpRGXBIFBank(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, RGXDBG_BIF0, ui64RegValMMUStatus, ui64RegValREQStatus, DD_SUMMARY_INDENT);

			if (!(RGX_IS_FEATURE_SUPPORTED(psDevInfo, SINGLE_BIF)))
			{
				ui64RegValMMUStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_BIF_FAULT_BANK1_MMU_STATUS);
				ui64RegValREQStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_BIF_FAULT_BANK1_REQ_STATUS);
				_RGXDumpRGXBIFBank(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, RGXDBG_BIF1, ui64RegValMMUStatus, ui64RegValREQStatus, DD_SUMMARY_INDENT);
			}

			if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, RISCV_FW_PROCESSOR))
			{
				ui64RegValMMUStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_FWCORE_MEM_FAULT_MMU_STATUS);
				ui64RegValREQStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_FWCORE_MEM_FAULT_REQ_STATUS);
				_RGXDumpRGXBIFBank(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, RGXDBG_FWCORE, ui64RegValMMUStatus, ui64RegValREQStatus, DD_SUMMARY_INDENT);
			}

			if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, CLUSTER_GROUPING))
			{
				IMG_UINT32  ui32PhantomCnt = RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, NUM_CLUSTERS) ?  RGX_REQ_NUM_PHANTOMS(RGX_GET_FEATURE_VALUE(psDevInfo, NUM_CLUSTERS)) : 0;

				if (ui32PhantomCnt > 1)
				{
					IMG_UINT32  ui32Phantom;
					for (ui32Phantom = 0;  ui32Phantom < ui32PhantomCnt;  ui32Phantom++)
					{
						/* This can't be done as it may interfere with the FW... */
						/*OSWriteHWReg64(RGX_CR_TEXAS_INDIRECT, ui32Phantom);*/

						ui64RegValMMUStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_TEXAS_BIF_FAULT_BANK0_MMU_STATUS);
						ui64RegValREQStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_TEXAS_BIF_FAULT_BANK0_REQ_STATUS);

						_RGXDumpRGXBIFBank(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, RGXDBG_TEXAS_BIF, ui64RegValMMUStatus, ui64RegValREQStatus, DD_SUMMARY_INDENT);
					}
				}else
				{
					ui64RegValMMUStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_TEXAS_BIF_FAULT_BANK0_MMU_STATUS);
					ui64RegValREQStatus = OSReadHWReg64(psDevInfo->pvRegsBaseKM, RGX_CR_TEXAS_BIF_FAULT_BANK0_REQ_STATUS);

					_RGXDumpRGXBIFBank(pfnDumpDebugPrintf, pvDumpDebugFile, psDevInfo, RGXDBG_TEXAS_BIF, ui64RegValMMUStatus, ui64RegValREQStatus, DD_SUMMARY_INDENT);
				}
			}
		}

		if (_CheckForPendingPage(psDevInfo))
		{
			IMG_UINT32 ui32CatBase;
			IMG_DEV_VIRTADDR sDevVAddr;

			PVR_DUMPDEBUG_LOG("MMU Pending page: Yes");

			_GetPendingPageInfo(psDevInfo, &sDevVAddr, &ui32CatBase);

			if (ui32CatBase >= 8)
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

		if (bRGXPoweredON && RGX_IS_FEATURE_SUPPORTED(psDevInfo, GPU_MULTICORE_SUPPORT))
		{
			if (OSReadHWReg32(psDevInfo->pvRegsBaseKM, RGX_CR_MULTICORE_SYSTEM) > 1U)
			{
				PVR_DUMPDEBUG_LOG("RGX MC Configuration: 0x%X (1:primary, 0:secondary)", psFwSysData->ui32McConfig);
			}
		}

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
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;
	RGXRISCVFW_STATE sRiscvState;
	const IMG_CHAR *pszException;
	PVRSRV_ERROR eError;

	DDLOG64(FWCORE_MEM_CAT_BASE0);
	DDLOG64(FWCORE_MEM_CAT_BASE1);
	DDLOG64(FWCORE_MEM_CAT_BASE2);
	DDLOG64(FWCORE_MEM_CAT_BASE3);
	DDLOG64(FWCORE_MEM_CAT_BASE4);
	DDLOG64(FWCORE_MEM_CAT_BASE5);
	DDLOG64(FWCORE_MEM_CAT_BASE6);
	DDLOG64(FWCORE_MEM_CAT_BASE7);

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

	DDLOG32(FWCORE_MEM_FAULT_MMU_STATUS);
	DDLOG64(FWCORE_MEM_FAULT_REQ_STATUS);
	DDLOG32(FWCORE_MEM_MMU_STATUS);
	DDLOG32(FWCORE_MEM_READS_EXT_STATUS);
	DDLOG32(FWCORE_MEM_READS_INT_STATUS);

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

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, PBVNC_COREID_REG))
	{
		DDLOG64(CORE_ID__PBVNC);
	}
	else
	{
		DDLOG32(CORE_ID);
		DDLOG32(CORE_REVISION);
	}
	DDLOG32(DESIGNER_REV_FIELD1);
	DDLOG32(DESIGNER_REV_FIELD2);
	DDLOG64(CHANGESET_NUMBER);
}

void RGXDumpMulticoreRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	DDLOG32(MULTICORE_SYSTEM);
	DDLOG32(MULTICORE_GPU);
}

void RGXDumpClkRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	DDLOG64(CLK_CTRL);
	DDLOG64(CLK_STATUS);
	DDLOG64(CLK_CTRL2);
	DDLOG64(CLK_STATUS2);
}

void RGXDumpMMURegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	{
		DDLOG32(BIF_FAULT_BANK0_MMU_STATUS);
		DDLOG64(BIF_FAULT_BANK0_REQ_STATUS);
		DDLOG32(BIF_FAULT_BANK1_MMU_STATUS);
		DDLOG64(BIF_FAULT_BANK1_REQ_STATUS);
	}
	DDLOG32(BIF_MMU_STATUS);
	DDLOG32(BIF_MMU_ENTRY);
	DDLOG64(BIF_MMU_ENTRY_STATUS);

	{
		if (!(RGX_IS_FEATURE_SUPPORTED(psDevInfo, XT_TOP_INFRASTRUCTURE)))
		{
			DDLOG32(BIF_STATUS_MMU);
			DDLOG32(BIF_READS_EXT_STATUS);
			DDLOG32(BIF_READS_INT_STATUS);
		}
		DDLOG32(BIFPM_STATUS_MMU);
		DDLOG32(BIFPM_READS_EXT_STATUS);
		DDLOG32(BIFPM_READS_INT_STATUS);
	}

	{
		DDLOG64(BIF_CAT_BASE_INDEX);
		DDLOG64(BIF_CAT_BASE0);
		DDLOG64(BIF_CAT_BASE1);
		DDLOG64(BIF_CAT_BASE2);
		DDLOG64(BIF_CAT_BASE3);
		DDLOG64(BIF_CAT_BASE4);
		DDLOG64(BIF_CAT_BASE5);
		DDLOG64(BIF_CAT_BASE6);
		DDLOG64(BIF_CAT_BASE7);
	}

	DDLOG32(BIF_CTRL_INVAL);
	DDLOG32(BIF_CTRL);
}

void RGXDumpDMRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;
	IMG_UINT32   ui32TACycles, ui323DCycles, ui32TAOr3DCycles, ui32TAAnd3DCycles;

	DDLOG64(BIF_PM_CAT_BASE_VCE0);
	DDLOG64(BIF_PM_CAT_BASE_TE0);
	DDLOG64(BIF_PM_CAT_BASE_ALIST0);
	DDLOG64(BIF_PM_CAT_BASE_VCE1);
	DDLOG64(BIF_PM_CAT_BASE_TE1);
	DDLOG64(BIF_PM_CAT_BASE_ALIST1);

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, GPU_MULTICORE_SUPPORT))
	{
		DDLOG32(MULTICORE_GEOMETRY_CTRL_COMMON);
		DDLOG32(MULTICORE_FRAGMENT_CTRL_COMMON);
		DDLOG32(MULTICORE_COMPUTE_CTRL_COMMON);
	}

	DDLOG32(PERF_TA_PHASE);
	DDLOG32(PERF_TA_CYCLE);
	DDLOG32(PERF_3D_PHASE);
	DDLOG32(PERF_3D_CYCLE);

	ui32TACycles = OSReadHWReg32(pvRegsBaseKM, RGX_CR_PERF_TA_CYCLE);
	ui323DCycles = OSReadHWReg32(pvRegsBaseKM, RGX_CR_PERF_3D_CYCLE);
	ui32TAOr3DCycles = OSReadHWReg32(pvRegsBaseKM, RGX_CR_PERF_TA_OR_3D_CYCLE);
	ui32TAAnd3DCycles = ((ui32TACycles + ui323DCycles) > ui32TAOr3DCycles) ? (ui32TACycles + ui323DCycles - ui32TAOr3DCycles) : 0;
	DDLOGVAL32("PERF_TA_OR_3D_CYCLE", ui32TAOr3DCycles);
	DDLOGVAL32("PERF_TA_AND_3D_CYCLE", ui32TAAnd3DCycles);

	DDLOG32(PERF_COMPUTE_PHASE);
	DDLOG32(PERF_COMPUTE_CYCLE);

	DDLOG32(PM_PARTIAL_RENDER_ENABLE);

	DDLOG32(ISP_RENDER);
	DDLOG64(TLA_STATUS);
	DDLOG64(MCU_FENCE);

	DDLOG32(VDM_CONTEXT_STORE_STATUS);
	DDLOG64(VDM_CONTEXT_STORE_TASK0);
	DDLOG64(VDM_CONTEXT_STORE_TASK1);
	DDLOG64(VDM_CONTEXT_STORE_TASK2);
	DDLOG64(VDM_CONTEXT_RESUME_TASK0);
	DDLOG64(VDM_CONTEXT_RESUME_TASK1);
	DDLOG64(VDM_CONTEXT_RESUME_TASK2);

	DDLOG32(ISP_CTL);
	DDLOG32(ISP_STATUS);

	DDLOG32(CDM_CONTEXT_STORE_STATUS);
	DDLOG64(CDM_CONTEXT_PDS0);
	DDLOG64(CDM_CONTEXT_PDS1);
	DDLOG64(CDM_TERMINATE_PDS);
	DDLOG64(CDM_TERMINATE_PDS1);

	if (RGX_IS_ERN_SUPPORTED(psDevInfo, 47025))
	{
		DDLOG64(CDM_CONTEXT_LOAD_PDS0);
		DDLOG64(CDM_CONTEXT_LOAD_PDS1);
	}
}

void RGXDumpSLCRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	{
		DDLOG32(SLC_IDLE);
		DDLOG32(SLC_STATUS0);
		DDLOG64(SLC_STATUS1);

		if (RGX_IS_FEATURE_VALUE_SUPPORTED(psDevInfo, SLC_BANKS) && RGX_GET_FEATURE_VALUE(psDevInfo, SLC_BANKS))
		{
			DDLOG64(SLC_STATUS2);
		}

		if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, XE_MEMORY_HIERARCHY))
		{
			DDLOG64(SLC_CTRL_BYPASS);
		}
		else
		{
			DDLOG32(SLC_CTRL_BYPASS);
		}
		DDLOG64(SLC_CTRL_MISC);
	}
}

void RGXDumpMiscRegisters(DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
								 void *pvDumpDebugFile,
								 PVRSRV_RGXDEV_INFO *psDevInfo)
{
	void __iomem *pvRegsBaseKM = psDevInfo->pvRegsBaseKM;

	DDLOG32(SIDEKICK_IDLE);

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, ROGUEXE) &&
		RGX_IS_FEATURE_SUPPORTED(psDevInfo, WATCHDOG_TIMER))
	{
		DDLOG32(SAFETY_EVENT_STATUS__ROGUEXE);
		DDLOG32(MTS_SAFETY_EVENT_ENABLE__ROGUEXE);
	}

	if (RGX_IS_FEATURE_SUPPORTED(psDevInfo, WATCHDOG_TIMER))
	{
		DDLOG32(FWCORE_WDT_CTRL);
	}

	if (PVRSRV_GET_DEVICE_FEATURE_VALUE(psDevInfo->psDeviceNode, LAYOUT_MARS) > 0)
	{
		DDLOG32(SCRATCH0);
		DDLOG32(SCRATCH1);
		DDLOG32(SCRATCH2);
		DDLOG32(SCRATCH3);
		DDLOG32(SCRATCH4);
		DDLOG32(SCRATCH5);
		DDLOG32(SCRATCH6);
		DDLOG32(SCRATCH7);
		DDLOG32(SCRATCH8);
		DDLOG32(SCRATCH9);
		DDLOG32(SCRATCH10);
		DDLOG32(SCRATCH11);
		DDLOG32(SCRATCH12);
		DDLOG32(SCRATCH13);
		DDLOG32(SCRATCH14);
		DDLOG32(SCRATCH15);
	}
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
	DumpTransferCtxtsInfo(psDevInfo, pfnDumpDebugPrintf, pvDumpDebugFile, ui32VerbLevel);
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

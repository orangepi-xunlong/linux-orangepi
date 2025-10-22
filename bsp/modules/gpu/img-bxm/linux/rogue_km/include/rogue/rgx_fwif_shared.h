/*************************************************************************/ /*!
@File
@Title          RGX firmware interface structures
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    RGX firmware interface structures shared by both host client
                and host server
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

#if !defined(RGX_FWIF_SHARED_H)
#define RGX_FWIF_SHARED_H

#include "img_types.h"
#include "img_defs.h"
#include "rgx_common.h"
#include "powervr/mem_types.h"
#include "devicemem_typedefs.h"

/* Indicates the number of RTDATAs per RTDATASET */
#if defined(SUPPORT_AGP)
#define RGXMKIF_NUM_RTDATAS           4U
#define RGXMKIF_NUM_GEOMDATAS         4U
#define RGXMKIF_NUM_RTDATA_FREELISTS  12U /* RGXMKIF_NUM_RTDATAS * RGXFW_MAX_FREELISTS */
#define RGX_NUM_GEOM_CORES           (2U)
#else
#define RGXMKIF_NUM_RTDATAS           2U
#define RGXMKIF_NUM_GEOMDATAS         1U
#define RGXMKIF_NUM_RTDATA_FREELISTS  2U  /* RGXMKIF_NUM_RTDATAS * RGXFW_MAX_FREELISTS */
#define RGX_NUM_GEOM_CORES           (1U)
#endif

/* Maximum number of UFOs in a CCB command.
 * The number is based on having 32 sync prims (as originally), plus 32 sync
 * checkpoints.
 * Once the use of sync prims is no longer supported, we will retain
 * the same total (64) as the number of sync checkpoints which may be
 * supporting a fence is not visible to the client driver and has to
 * allow for the number of different timelines involved in fence merges.
 */
#define RGXFWIF_CCB_CMD_MAX_UFOS			(32U+32U)

/*
 * This is a generic limit imposed on any DM (TA,3D,CDM,TDM,2D,TRANSFER)
 * command passed through the bridge.
 * Just across the bridge in the server, any incoming kick command size is
 * checked against this maximum limit.
 * In case the incoming command size is larger than the specified limit,
 * the bridge call is retired with error.
 */
#define RGXFWIF_DM_INDEPENDENT_KICK_CMD_SIZE	(1024U)

typedef struct
{
	IMG_DEV_VIRTADDR        RGXFW_ALIGN psDevVirtAddr;
	RGXFWIF_DEV_VIRTADDR    pbyFWAddr;
} UNCACHED_ALIGN RGXFWIF_DMA_ADDR;

typedef IMG_UINT8	RGXFWIF_CCCB;

typedef RGXFWIF_DEV_VIRTADDR  PRGXFWIF_UFO_ADDR;
typedef RGXFWIF_DEV_VIRTADDR  PRGXFWIF_CLEANUP_CTL;


/*!
 * @InGroup ClientCCBTypes
 * @Brief Command data for fence & update types Client CCB commands.
 */
typedef struct
{
	PRGXFWIF_UFO_ADDR	puiAddrUFO; /*!< Address to be checked/updated */
	IMG_UINT32			ui32Value;  /*!< Value to check-against/update-to */
} RGXFWIF_UFO;

/*!
 * @InGroup RenderTarget
 * @Brief Track pending and executed workloads of HWRTDATA and ZSBUFFER
 */
typedef struct
{
	IMG_UINT32			ui32SubmittedCommands;	/*!< Number of commands received by the FW */
	IMG_UINT32			ui32ExecutedCommands;	/*!< Number of commands executed by the FW */
} UNCACHED_ALIGN RGXFWIF_CLEANUP_CTL;

#define	RGXFWIF_PRBUFFER_START        IMG_UINT32_C(0)
#define	RGXFWIF_PRBUFFER_ZSBUFFER     IMG_UINT32_C(0)
#define	RGXFWIF_PRBUFFER_MSAABUFFER   IMG_UINT32_C(1)
#define	RGXFWIF_PRBUFFER_MAXSUPPORTED IMG_UINT32_C(2)

typedef IMG_UINT32 RGXFWIF_PRBUFFER_TYPE;

typedef enum
{
	RGXFWIF_PRBUFFER_UNBACKED = 0,
	RGXFWIF_PRBUFFER_BACKED,
	RGXFWIF_PRBUFFER_BACKING_PENDING,
	RGXFWIF_PRBUFFER_UNBACKING_PENDING,
}RGXFWIF_PRBUFFER_STATE;

/*!
 * @InGroup RenderTarget
 * @Brief OnDemand Z/S/MSAA Buffers
 */
typedef struct
{
	IMG_UINT32		ui32BufferID;		/*!< Buffer ID*/
	IMG_BOOL		bOnDemand;		/*!< Needs On-demand Z/S/MSAA Buffer allocation */
	RGXFWIF_PRBUFFER_STATE	eState;			/*!< Z/S/MSAA -Buffer state */
	RGXFWIF_CLEANUP_CTL	sCleanupState;		/*!< Cleanup state */
	IMG_UINT32		ui32PRBufferFlags;	/*!< Compatibility and other flags */
} UNCACHED_ALIGN RGXFWIF_PRBUFFER;

/*
 * Used to share frame numbers across UM-KM-FW,
 * frame number is set in UM,
 * frame number is required in both KM for HTB and FW for FW trace.
 *
 * May be used to house Kick flags in the future.
 */
typedef struct
{
	IMG_UINT32 ui32FrameNum; /*!< associated frame number */
} CMD_COMMON;

/*
 * TA and 3D commands require set of firmware addresses that are stored in the
 * Kernel. Client has handle(s) to Kernel containers storing these addresses,
 * instead of raw addresses. We have to patch/write these addresses in KM to
 * prevent UM from controlling FW addresses directly.
 * Typedefs for TA and 3D commands are shared between Client and Firmware (both
 * single-BVNC). Kernel is implemented in a multi-BVNC manner, so it can't use
 * TA|3D CMD type definitions directly. Therefore we have a SHARED block that
 * is shared between UM-KM-FW across all BVNC configurations.
 */
typedef struct
{
	CMD_COMMON           sCmn;      /*!< Common command attributes */
	RGXFWIF_DEV_VIRTADDR sHWRTData; /* RTData associated with this command,
									   this is used for context selection and for storing out HW-context,
									   when TA is switched out for continuing later */

	RGXFWIF_DEV_VIRTADDR asPRBuffer[RGXFWIF_PRBUFFER_MAXSUPPORTED];	/* Supported PR Buffers like Z/S/MSAA Scratch */

} CMDTA3D_SHARED;

/*!
 * Client Circular Command Buffer (CCCB) control structure.
 * This is shared between the Server and the Firmware and holds byte offsets
 * into the CCCB as well as the wrapping mask to aid wrap around. A given
 * snapshot of this queue with Cmd 1 running on the GPU might be:
 *
 *          Roff                           Doff                 Woff
 * [..........|-1----------|=2===|=3===|=4===|~5~~~~|~6~~~~|~7~~~~|..........]
 *            <      runnable commands       ><   !ready to run   >
 *
 * Cmd 1    : Currently executing on the GPU data master.
 * Cmd 2,3,4: Fence dependencies met, commands runnable.
 * Cmd 5... : Fence dependency not met yet.
 */
typedef struct
{
	IMG_UINT32  ui32WriteOffset;    /*!< Host write offset into CCB. This
	                                 *    must be aligned to 16 bytes. */
	IMG_UINT32  ui32ReadOffset;     /*!< Firmware read offset into CCB.
	                                      Points to the command that is
	                                 *    runnable on GPU, if R!=W */
	IMG_UINT32  ui32DepOffset;      /*!< Firmware fence dependency offset.
	                                 *    Points to commands not ready, i.e.
	                                 *    fence dependencies are not met. */
	IMG_UINT32  ui32WrapMask;       /*!< Offset wrapping mask, total capacity
	                                      in bytes of the CCB-1 */

	IMG_UINT32  ui32ReadOffset2;     /*!< Firmware 2nd read offset into CCB for AGP.
	                                      Points to the command that is
	                                      runnable on GPU, if R2!=W */
	IMG_UINT32  ui32ReadOffset3;     /*!< Firmware 3rd read offset into CCB for AGP.
	                                      Points to the command that is
	                                      runnable on GPU, if R3!=W */
	IMG_UINT32  ui32ReadOffset4;     /*!< Firmware 4th read offset into CCB for AGP.
	                                      Points to the command that is
	                                      runnable on GPU, if R4!=W */

} UNCACHED_ALIGN RGXFWIF_CCCB_CTL;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_CCCB_CTL) == 32,
				"RGXFWIF_CCCB_CTL is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

typedef IMG_UINT32 RGXFW_FREELIST_TYPE;

#define RGXFW_LOCAL_FREELIST     IMG_UINT32_C(0)
#define RGXFW_GLOBAL_FREELIST    IMG_UINT32_C(1)
#if defined(SUPPORT_AGP)
#define RGXFW_GLOBAL2_FREELIST   IMG_UINT32_C(2)
#define RGXFW_MAX_FREELISTS      (RGXFW_GLOBAL2_FREELIST + 1U)
#else
#define RGXFW_MAX_FREELISTS      (RGXFW_GLOBAL_FREELIST + 1U)
#endif
#define RGXFW_MAX_HWFREELISTS    (2U)

/*!
 * @Defgroup ContextSwitching Context switching data interface
 * @Brief Types grouping data structures and defines used in realising the Context Switching (CSW) functionality
 * @{
 */

/*!
 * @Brief GEOM DM or TA register controls for context switch
 */
typedef struct
{
	IMG_UINT64	uTAReg_VDM_CONTEXT_STATE_BASE_ADDR; /*!< The base address of the VDM's context state buffer */
	IMG_UINT64	uTAReg_VDM_CONTEXT_STATE_RESUME_ADDR;
	IMG_UINT64	uTAReg_TA_CONTEXT_STATE_BASE_ADDR; /*!< The base address of the TA's context state buffer */

	struct
	{
		IMG_UINT64	uTAReg_VDM_CONTEXT_STORE_TASK0; /*!< VDM context store task 0 */
		IMG_UINT64	uTAReg_VDM_CONTEXT_STORE_TASK1; /*!< VDM context store task 1 */
		IMG_UINT64	uTAReg_VDM_CONTEXT_STORE_TASK2; /*!< VDM context store task 2 */

		/* VDM resume state update controls */
		IMG_UINT64	uTAReg_VDM_CONTEXT_RESUME_TASK0; /*!< VDM context resume task 0 */
		IMG_UINT64	uTAReg_VDM_CONTEXT_RESUME_TASK1; /*!< VDM context resume task 1 */
		IMG_UINT64	uTAReg_VDM_CONTEXT_RESUME_TASK2; /*!< VDM context resume task 2 */
	} asTAState[2];

} RGXFWIF_TAREGISTERS_CSWITCH;
/*! @} End of Defgroup ContextSwitching */

#define RGXFWIF_TAREGISTERS_CSWITCH_SIZE sizeof(RGXFWIF_TAREGISTERS_CSWITCH)

typedef struct
{
	IMG_UINT64	uCDMReg_CDM_CONTEXT_PDS0;
	IMG_UINT64	uCDMReg_CDM_CONTEXT_PDS1;
	IMG_UINT64	uCDMReg_CDM_TERMINATE_PDS;
	IMG_UINT64	uCDMReg_CDM_TERMINATE_PDS1;

	/* CDM resume controls */
	IMG_UINT64	uCDMReg_CDM_RESUME_PDS0;
	IMG_UINT64	uCDMReg_CDM_CONTEXT_PDS0_B;
	IMG_UINT64	uCDMReg_CDM_RESUME_PDS0_B;

} RGXFWIF_CDM_REGISTERS_CSWITCH;

/*!
 * @InGroup ContextSwitching
 * @Brief Render context static register controls for context switch
 */
typedef struct
{
	RGXFWIF_TAREGISTERS_CSWITCH	RGXFW_ALIGN asCtxSwitch_GeomRegs[RGX_NUM_GEOM_CORES];	/*!< Geom registers for ctx switch */
} RGXFWIF_STATIC_RENDERCONTEXT_STATE;

#define RGXFWIF_STATIC_RENDERCONTEXT_SIZE sizeof(RGXFWIF_STATIC_RENDERCONTEXT_STATE)

typedef struct
{
	RGXFWIF_CDM_REGISTERS_CSWITCH	RGXFW_ALIGN sCtxSwitch_Regs;	/*!< CDM registers for ctx switch */
} RGXFWIF_STATIC_COMPUTECONTEXT_STATE;

#define RGXFWIF_STATIC_COMPUTECONTEXT_SIZE sizeof(RGXFWIF_STATIC_COMPUTECONTEXT_STATE)

/*!
	@Brief Context reset reason. Last reset reason for a reset context.
*/
typedef enum
{
	RGX_CONTEXT_RESET_REASON_NONE                = 0,	/*!< No reset reason recorded */
	RGX_CONTEXT_RESET_REASON_GUILTY_LOCKUP       = 1,	/*!< Caused a reset due to locking up */
	RGX_CONTEXT_RESET_REASON_INNOCENT_LOCKUP     = 2,	/*!< Affected by another context locking up */
	RGX_CONTEXT_RESET_REASON_GUILTY_OVERRUNING   = 3,	/*!< Overran the global deadline */
	RGX_CONTEXT_RESET_REASON_INNOCENT_OVERRUNING = 4,	/*!< Affected by another context overrunning */
	RGX_CONTEXT_RESET_REASON_HARD_CONTEXT_SWITCH = 5,	/*!< Forced reset to ensure scheduling requirements */
	RGX_CONTEXT_RESET_REASON_WGP_CHECKSUM        = 6,	/*!< CDM Mission/safety checksum mismatch */
	RGX_CONTEXT_RESET_REASON_TRP_CHECKSUM        = 7,	/*!< TRP checksum mismatch */
	RGX_CONTEXT_RESET_REASON_GPU_ECC_OK          = 8,	/*!< GPU ECC error (corrected, OK) */
	RGX_CONTEXT_RESET_REASON_GPU_ECC_HWR         = 9,	/*!< GPU ECC error (uncorrected, HWR) */
	RGX_CONTEXT_RESET_REASON_FW_ECC_OK           = 10,	/*!< FW ECC error (corrected, OK) */
	RGX_CONTEXT_RESET_REASON_FW_ECC_ERR          = 11,	/*!< FW ECC error (uncorrected, ERR) */
	RGX_CONTEXT_RESET_REASON_FW_WATCHDOG         = 12,	/*!< FW Safety watchdog triggered */
	RGX_CONTEXT_RESET_REASON_FW_PAGEFAULT        = 13,	/*!< FW page fault (no HWR) */
	RGX_CONTEXT_RESET_REASON_FW_EXEC_ERR         = 14,	/*!< FW execution error (GPU reset requested) */
	RGX_CONTEXT_RESET_REASON_HOST_WDG_FW_ERR     = 15,	/*!< Host watchdog detected FW error */
	RGX_CONTEXT_GEOM_OOM_DISABLED                = 16,	/*!< Geometry DM OOM event is not allowed */
	RGX_CONTEXT_PVRIC_SIGNATURE_MISMATCH         = 17,	/*!< PVRIC Signature mismatch */
	RGX_CONTEXT_RESET_REASON_FW_PTE_PARITY_ERR   = 18,	/*!< Parity error in MMU Page Table Entry */
	RGX_CONTEXT_RESET_REASON_FW_PARITY_ERR       = 19,	/*!< Parity error in MH, system bus or Control Status registers */
	RGX_CONTEXT_RESET_REASON_GPU_PARITY_HWR      = 20,	/*!< Parity error in system bus or Control Status registers */
	RGX_CONTEXT_RESET_REASON_GPU_LATENT_HWR      = 21,	/*!< Latent/ICS signature mismatch error */
	RGX_CONTEXT_RESET_REASON_DCLS_ERR            = 22,	/*!< Dual Core Lock Step FW error detected */
} RGX_CONTEXT_RESET_REASON;

/*!
	@Brief Context reset data shared with the host
*/
typedef struct
{
	RGX_CONTEXT_RESET_REASON eResetReason; /*!< Reset reason */
	IMG_UINT32 ui32ResetExtJobRef;  /*!< External Job ID */
} RGX_CONTEXT_RESET_REASON_DATA;

#define RGX_HEAP_UM_PDS_RESERVED_SIZE               DEVMEM_HEAP_RESERVED_SIZE_GRANULARITY
#define RGX_HEAP_UM_PDS_RESERVED_REGION_OFFSET      0
#define RGX_HEAP_PDS_RESERVED_TOTAL_SIZE            RGX_HEAP_UM_PDS_RESERVED_SIZE

#define RGX_HEAP_UM_USC_RESERVED_SIZE               DEVMEM_HEAP_RESERVED_SIZE_GRANULARITY
#define RGX_HEAP_UM_USC_RESERVED_REGION_OFFSET      0
#define RGX_HEAP_KM_USC_RESERVED_REGION_OFFSET      RGX_HEAP_UM_USC_RESERVED_SIZE

#define RGX_HEAP_UM_GENERAL_RESERVED_SIZE           DEVMEM_HEAP_RESERVED_SIZE_GRANULARITY
#define RGX_HEAP_UM_GENERAL_RESERVED_REGION_OFFSET  0
#define RGX_HEAP_KM_GENERAL_RESERVED_REGION_OFFSET  RGX_HEAP_UM_GENERAL_RESERVED_SIZE

/*************************************************************************/ /*!
 Logging type
*/ /**************************************************************************/
#define RGXFWIF_LOG_TYPE_NONE			0x00000000U
#define RGXFWIF_LOG_TYPE_TRACE			0x00000001U
#define RGXFWIF_LOG_TYPE_GROUP_MAIN		0x00000002U
#define RGXFWIF_LOG_TYPE_GROUP_MTS		0x00000004U
#define RGXFWIF_LOG_TYPE_GROUP_CLEANUP	0x00000008U
#define RGXFWIF_LOG_TYPE_GROUP_CSW		0x00000010U
#define RGXFWIF_LOG_TYPE_GROUP_BIF		0x00000020U
#define RGXFWIF_LOG_TYPE_GROUP_PM		0x00000040U
#define RGXFWIF_LOG_TYPE_GROUP_RTD		0x00000080U
#define RGXFWIF_LOG_TYPE_GROUP_SPM		0x00000100U
#define RGXFWIF_LOG_TYPE_GROUP_POW		0x00000200U
#define RGXFWIF_LOG_TYPE_GROUP_HWR		0x00000400U
#define RGXFWIF_LOG_TYPE_GROUP_HWP		0x00000800U
#define RGXFWIF_LOG_TYPE_GROUP_RPM		0x00001000U
#define RGXFWIF_LOG_TYPE_GROUP_DMA		0x00002000U
#define RGXFWIF_LOG_TYPE_GROUP_MISC		0x00004000U
#define RGXFWIF_LOG_TYPE_GROUP_VZ		0x00008000U
#define RGXFWIF_LOG_TYPE_GROUP_SAFETY	0x00010000U
#define RGXFWIF_LOG_TYPE_GROUP_VERBOSE	0x00020000U
#define RGXFWIF_LOG_TYPE_GROUP_CUSTOMER	0x00040000U
#define RGXFWIF_LOG_TYPE_GROUP_DEBUG	0x80000000U
#define RGXFWIF_LOG_TYPE_GROUP_MASK		0x8007FFFEU
#define RGXFWIF_LOG_TYPE_MASK			0x8007FFFFU

/* String used in pvrdebug -h output */
#define RGXFWIF_LOG_GROUPS_STRING_LIST   "main,mts,cleanup,csw,bif,pm,rtd,spm,pow,hwr,hwp,rpm,dma,misc,vz,safety,verbose,customer,debug"

/* Table entry to map log group strings to log type value */
typedef struct {
	const IMG_CHAR* pszLogGroupName;
	IMG_UINT32      ui32LogGroupType;
} RGXFWIF_LOG_GROUP_MAP_ENTRY;

/*
  Macro for use with the RGXFWIF_LOG_GROUP_MAP_ENTRY type to create a lookup
  table where needed. Keep log group names short, no more than 20 chars.
*/
#define RGXFWIF_LOG_GROUP_NAME_VALUE_MAP { "none",    RGXFWIF_LOG_TYPE_NONE }, \
                                         { "main",    RGXFWIF_LOG_TYPE_GROUP_MAIN }, \
                                         { "mts",     RGXFWIF_LOG_TYPE_GROUP_MTS }, \
                                         { "cleanup", RGXFWIF_LOG_TYPE_GROUP_CLEANUP }, \
                                         { "csw",     RGXFWIF_LOG_TYPE_GROUP_CSW }, \
                                         { "bif",     RGXFWIF_LOG_TYPE_GROUP_BIF }, \
                                         { "pm",      RGXFWIF_LOG_TYPE_GROUP_PM }, \
                                         { "rtd",     RGXFWIF_LOG_TYPE_GROUP_RTD }, \
                                         { "spm",     RGXFWIF_LOG_TYPE_GROUP_SPM }, \
                                         { "pow",     RGXFWIF_LOG_TYPE_GROUP_POW }, \
                                         { "hwr",     RGXFWIF_LOG_TYPE_GROUP_HWR }, \
                                         { "hwp",     RGXFWIF_LOG_TYPE_GROUP_HWP }, \
                                         { "rpm",     RGXFWIF_LOG_TYPE_GROUP_RPM }, \
                                         { "dma",     RGXFWIF_LOG_TYPE_GROUP_DMA }, \
                                         { "misc",    RGXFWIF_LOG_TYPE_GROUP_MISC }, \
                                         { "vz",      RGXFWIF_LOG_TYPE_GROUP_VZ }, \
                                         { "safety",  RGXFWIF_LOG_TYPE_GROUP_SAFETY }, \
                                         { "verbose", RGXFWIF_LOG_TYPE_GROUP_VERBOSE }, \
                                         { "customer",RGXFWIF_LOG_TYPE_GROUP_CUSTOMER }, \
                                         { "debug",   RGXFWIF_LOG_TYPE_GROUP_DEBUG }

/* Used in print statements to display log group state, one %s per group defined */
#define RGXFWIF_LOG_ENABLED_GROUPS_LIST_PFSPEC  "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"

/* Used in a print statement to display log group state, one per group */
#define RGXFWIF_LOG_ENABLED_GROUPS_LIST(types)  ((((types) & RGXFWIF_LOG_TYPE_GROUP_MAIN) != 0U)	?("main ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_MTS) != 0U)		?("mts ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_CLEANUP) != 0U)	?("cleanup ")	:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_CSW) != 0U)		?("csw ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_BIF) != 0U)		?("bif ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_PM) != 0U)		?("pm ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_RTD) != 0U)		?("rtd ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_SPM) != 0U)		?("spm ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_POW) != 0U)		?("pow ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_HWR) != 0U)		?("hwr ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_HWP) != 0U)		?("hwp ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_RPM) != 0U)		?("rpm ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_DMA) != 0U)		?("dma ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_MISC) != 0U)	?("misc ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_VZ) != 0U)		?("vz ")		:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_SAFETY) != 0U)	?("safety ")	:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_VERBOSE) != 0U)	?("verbose ")	:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_CUSTOMER) != 0U)?("customer ")	:("")),		\
                                                ((((types) & RGXFWIF_LOG_TYPE_GROUP_DEBUG) != 0U)	?("debug ")		:(""))

/*!
 ******************************************************************************
 * Trace Buffer
 *****************************************************************************/

/*! Min, Max, and Default size of RGXFWIF_TRACEBUF_SPACE in DWords */
#define RGXFW_TRACE_BUF_MIN_SIZE_IN_DWORDS      8192U  /*  32KB */
#define RGXFW_TRACE_BUF_DEFAULT_SIZE_IN_DWORDS 12000U  /* ~48KB */
#define RGXFW_TRACE_BUF_MAX_SIZE_IN_DWORDS     32768U  /* 128KB */

#define RGXFW_TRACE_BUFFER_ASSERT_SIZE 200U
#if defined(RGXFW_META_SUPPORT_2ND_THREAD)
#define RGXFW_THREAD_NUM 2U
#else
#define RGXFW_THREAD_NUM 1U
#endif

typedef struct
{
	IMG_CHAR	szPath[RGXFW_TRACE_BUFFER_ASSERT_SIZE];
	IMG_CHAR	szInfo[RGXFW_TRACE_BUFFER_ASSERT_SIZE];
	IMG_UINT32	ui32LineNum;
} UNCACHED_ALIGN RGXFWIF_FILE_INFO_BUF;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_FILE_INFO_BUF) == 408,
				"RGXFWIF_FILE_INFO_BUF is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

/*!
 * @Defgroup SRVAndFWTracing Services and Firmware Tracing data interface
 * @Brief The document groups/lists the data structures and the interfaces related to Services and Firmware Tracing
 * @{
 */

/*!
 * @Brief Firmware trace buffer details
 */
typedef struct
{
	IMG_UINT32                         ui32TracePointer;          /*!< Trace pointer (write index into Trace Buffer) */
	IMG_UINT32                         ui32WrapCount;             /*!< Number of times the Trace Buffer has wrapped */

	RGXFWIF_DEV_VIRTADDR               pui32RGXFWIfTraceBuffer;   /*!< Trace buffer address (FW address), to be used by firmware for writing into trace buffer */

	RGXFWIF_FILE_INFO_BUF RGXFW_ALIGN  sAssertBuf;
} UNCACHED_ALIGN RGXFWIF_TRACEBUF_SPACE;

/*! @} End of Defgroup SRVAndFWTracing */

/*!
 * @InGroup SRVAndFWTracing
 * @Brief Firmware trace control data
 */
typedef struct
{
	IMG_UINT32              ui32LogType;                  /*!< FW trace log group configuration */
#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
	RGXFWIF_TRACEBUF_SPACE  sTraceBuf[MAX_THREAD_NUM];  /*!< FW Trace buffer */
#else
	RGXFWIF_TRACEBUF_SPACE  sTraceBuf[RGXFW_THREAD_NUM];  /*!< FW Trace buffer */
#endif
	IMG_UINT32              ui32TraceBufSizeInDWords;     /*!< FW Trace buffer size in dwords, Member initialised only when sTraceBuf is actually allocated
															(in RGXTraceBufferInitOnDemandResources) */
	IMG_UINT32              ui32TracebufFlags;            /*!< Compatibility and other flags */
} UNCACHED_ALIGN RGXFWIF_TRACEBUF;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_TRACEBUF) == 880,
				"RGXFWIF_TRACEBUF is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

/* Debug-info sub-fields */
/* Bit 0: RGX_CR_EVENT_STATUS_MMU_PAGE_FAULT bit from RGX_CR_EVENT_STATUS register */
#define RGXFWT_DEBUG_INFO_MMU_PAGE_FAULT_SHIFT        (0U)
#define RGXFWT_DEBUG_INFO_MMU_PAGE_FAULT_SET          (1U << RGXFWT_DEBUG_INFO_MMU_PAGE_FAULT_SHIFT)

/* Bit 1: RGX_CR_BIF_MMU_ENTRY_PENDING bit from RGX_CR_BIF_MMU_ENTRY register */
#define RGXFWT_DEBUG_INFO_MMU_ENTRY_PENDING_SHIFT     (1U)
#define RGXFWT_DEBUG_INFO_MMU_ENTRY_PENDING_SET       (1U << RGXFWT_DEBUG_INFO_MMU_ENTRY_PENDING_SHIFT)

/* Bit 2: RGX_CR_SLAVE_EVENT register is non-zero */
#define RGXFWT_DEBUG_INFO_SLAVE_EVENTS_SHIFT          (2U)
#define RGXFWT_DEBUG_INFO_SLAVE_EVENTS_SET            (1U << RGXFWT_DEBUG_INFO_SLAVE_EVENTS_SHIFT)

/* Bit 3-15: Unused bits */

#define RGXFWT_DEBUG_INFO_STR_MAXLEN                  64
#define RGXFWT_DEBUG_INFO_STR_PREPEND                 " (debug info: "
#define RGXFWT_DEBUG_INFO_STR_APPEND                  ")"

/* Table of debug info sub-field's masks and corresponding message strings
 * to be appended to firmware trace
 *
 * Mask     : 16 bit mask to be applied to debug-info field
 * String   : debug info message string
 */

#define RGXFWT_DEBUG_INFO_MSKSTRLIST \
/*Mask,                                           String*/ \
X(RGXFWT_DEBUG_INFO_MMU_PAGE_FAULT_SET,      "mmu pf") \
X(RGXFWT_DEBUG_INFO_MMU_ENTRY_PENDING_SET,   "mmu pending") \
X(RGXFWT_DEBUG_INFO_SLAVE_EVENTS_SET,        "slave events")

/* Firmware trace time-stamp field breakup */

/* RGX_CR_TIMER register read (48 bits) value*/
#define RGXFWT_TIMESTAMP_TIME_SHIFT                   (0U)
#define RGXFWT_TIMESTAMP_TIME_CLRMSK                  (IMG_UINT64_C(0xFFFF000000000000))

/* Extra debug-info (16 bits) */
#define RGXFWT_TIMESTAMP_DEBUG_INFO_SHIFT             (48U)
#define RGXFWT_TIMESTAMP_DEBUG_INFO_CLRMSK            ~RGXFWT_TIMESTAMP_TIME_CLRMSK

typedef struct
{
	IMG_UINT			bfOsState		: 3;
	IMG_UINT			bfFLOk			: 1;
	IMG_UINT			bfFLGrowPending	: 1;
	IMG_UINT			bfReserved		: 27;
} RGXFWIF_OS_RUNTIME_FLAGS;

#define RGXFWIF_FWFAULTINFO_MAX		(8U)			/* Total number of FW fault logs stored */

typedef struct
{
	IMG_UINT64 RGXFW_ALIGN	ui64CRTimer;
	IMG_UINT64 RGXFW_ALIGN	ui64OSTimer;
	IMG_UINT64 RGXFW_ALIGN	ui64Data;
	RGXFWIF_FILE_INFO_BUF	sFaultBuf;
} UNCACHED_ALIGN RGX_FWFAULTINFO;

#define RGXFWIF_POW_STATES \
  X(RGXFWIF_POW_OFF)			/* idle and handshaked with the host (ready to full power down) */ \
  X(RGXFWIF_POW_ON)				/* running HW commands */ \
  X(RGXFWIF_POW_FORCED_IDLE)	/* forced idle */ \
  X(RGXFWIF_POW_IDLE)			/* idle waiting for host handshake */

typedef enum
{
#define X(NAME) NAME,
	RGXFWIF_POW_STATES
#undef X
} RGXFWIF_POW_STATE;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
#define MAX_THREAD_NUM 2

static_assert(RGXFW_THREAD_NUM <= MAX_THREAD_NUM,
				"RGXFW_THREAD_NUM is outside of allowable range for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

/* Firmware HWR states */
#define RGXFWIF_HWR_HARDWARE_OK			(IMG_UINT32_C(0x1) << 0U)	/*!< The HW state is ok or locked up */
#define RGXFWIF_HWR_RESET_IN_PROGRESS	(IMG_UINT32_C(0x1) << 1U)	/*!< Tells if a HWR reset is in progress */
#define RGXFWIF_HWR_GENERAL_LOCKUP		(IMG_UINT32_C(0x1) << 3U)	/*!< A DM unrelated lockup has been detected */
#define RGXFWIF_HWR_DM_RUNNING_OK		(IMG_UINT32_C(0x1) << 4U)	/*!< At least one DM is running without being close to a lockup */
#define RGXFWIF_HWR_DM_STALLING			(IMG_UINT32_C(0x1) << 5U)	/*!< At least one DM is close to lockup */
#define RGXFWIF_HWR_FW_FAULT			(IMG_UINT32_C(0x1) << 6U)	/*!< The FW has faulted and needs to restart */
#define RGXFWIF_HWR_RESTART_REQUESTED	(IMG_UINT32_C(0x1) << 7U)	/*!< The FW has requested the host to restart it */

#define RGXFWIF_PHR_MODE_OFF			(0UL)
#define RGXFWIF_PHR_MODE_RD_RESET		(1UL)

typedef IMG_UINT32 RGXFWIF_HWR_STATEFLAGS;

typedef IMG_UINT32 RGXFWIF_HWR_RECOVERYFLAGS;

/*! @Brief Firmware system data shared with the Host driver */
typedef struct
{
	IMG_UINT32                 ui32ConfigFlags;                       /*!< Configuration flags from host */
	IMG_UINT32                 ui32ConfigFlagsExt;                    /*!< Extended configuration flags from host */
	volatile RGXFWIF_POW_STATE ePowState;
	struct {
		volatile IMG_UINT32        ui32HWPerfRIdx;
		volatile IMG_UINT32        ui32HWPerfWIdx;
		volatile IMG_UINT32        ui32HWPerfWrapCount;
	} sHWPerfCtrl; /* Struct used to inval/flush HWPerfCtrl members */
	IMG_UINT32                 ui32HWPerfSize;                        /*!< Constant after setup, needed in FW */
	IMG_UINT32                 ui32HWPerfDropCount;                   /*!< The number of times the FW drops a packet due to buffer full */

	/* ui32HWPerfUt, ui32FirstDropOrdinal, ui32LastDropOrdinal only valid when FW is built with
	 * RGX_HWPERF_UTILIZATION & RGX_HWPERF_DROP_TRACKING defined in rgxfw_hwperf.c */
	IMG_UINT32                 ui32HWPerfUt;                          /*!< Buffer utilisation, high watermark of bytes in use */
	IMG_UINT32                 ui32FirstDropOrdinal;                  /*!< The ordinal of the first packet the FW dropped */
	IMG_UINT32                 ui32LastDropOrdinal;                   /*!< The ordinal of the last packet the FW dropped */
	RGXFWIF_OS_RUNTIME_FLAGS   asOsRuntimeFlagsMirror[RGXFW_MAX_NUM_OSIDS];/*!< State flags for each Operating System mirrored from Fw coremem */
	RGX_FWFAULTINFO            sFaultInfo[RGXFWIF_FWFAULTINFO_MAX];   /*!< Firmware fault info */
	IMG_UINT32                 ui32FWFaults;                          /*!< Firmware faults count */
#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
	IMG_UINT32                 aui32CrPollAddr[MAX_THREAD_NUM];     /*!< Failed poll address */
	IMG_UINT32                 aui32CrPollMask[MAX_THREAD_NUM];     /*!< Failed poll mask */
	IMG_UINT32                 aui32CrPollCount[MAX_THREAD_NUM];    /*!< Failed poll count */
#else
	IMG_UINT32                 aui32CrPollAddr[RGXFW_THREAD_NUM];     /*!< Failed poll address */
	IMG_UINT32                 aui32CrPollMask[RGXFW_THREAD_NUM];     /*!< Failed poll mask */
	IMG_UINT32                 aui32CrPollCount[RGXFW_THREAD_NUM];    /*!< Failed poll count */
#endif
	IMG_UINT64 RGXFW_ALIGN     ui64StartIdleTime;
	IMG_UINT32                 aui32TSMirror[RGXFW_MAX_NUM_OSIDS];    /*!< Array of time slice per OS Mirrored from the FW */

#if defined(SUPPORT_RGXFW_STATS_FRAMEWORK) && !defined(SUPPORT_OPEN_SOURCE_DRIVER)
#define RGXFWIF_STATS_FRAMEWORK_LINESIZE    (8)
#define RGXFWIF_STATS_FRAMEWORK_MAX         (2048*RGXFWIF_STATS_FRAMEWORK_LINESIZE)
	IMG_UINT32 RGXFW_ALIGN     aui32FWStatsBuf[RGXFWIF_STATS_FRAMEWORK_MAX];
#endif
	RGXFWIF_HWR_STATEFLAGS     ui32HWRStateFlags; /*!< Firmware's Current HWR state */
	RGXFWIF_HWR_RECOVERYFLAGS  aui32HWRRecoveryFlags[RGXFWIF_DM_MAX]; /*!< Each DM's HWR state */
	IMG_UINT32                 ui32FwSysDataFlags;                      /*!< Compatibility and other flags */
	IMG_UINT32                 ui32McConfig;                            /*!< Identify whether MC config is P-P or P-S */
	IMG_UINT32                 ui32MemFaultCheck;                       /*!< Device mem fault check on PCI systems */
} UNCACHED_ALIGN RGXFWIF_SYSDATA;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_SYSDATA) == 3624,
				"RGXFWIF_SYSDATA is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER == 3624");
#endif

#if defined(PVRSRV_STALLED_CCB_ACTION)
#define PVR_SLR_LOG_ENTRIES 10U
#define PVR_SLR_LOG_STRLEN  30 /*!< MAX_CLIENT_CCB_NAME not visible to this header */

typedef struct
{
	IMG_UINT64 RGXFW_ALIGN	ui64Timestamp;
	IMG_UINT32				ui32FWCtxAddr;
	IMG_UINT32				ui32NumUFOs;
	IMG_CHAR				aszCCBName[PVR_SLR_LOG_STRLEN];
} UNCACHED_ALIGN RGXFWIF_SLR_ENTRY;
#endif

#define RGXFWIF_OFFLINE_DATA_BUFFER_SIZE_IN_WORDS (128U)

/*!
 * @InGroup ContextSwitching
 * @Brief Firmware per-os data and configuration
 */
typedef struct
{
	IMG_UINT32                 ui32FwOsConfigFlags;                   /*!< Configuration flags from an OS */
	IMG_UINT32                 ui32FWSyncCheckMark;                   /*!< Markers to signal that the host should perform a full sync check */
	IMG_UINT32                 ui32HostSyncCheckMark;                  /*!< Markers to signal that the Firmware should perform a full sync check */
#if defined(PVRSRV_STALLED_CCB_ACTION) || defined(SUPPORT_OPEN_SOURCE_DRIVER)
	IMG_UINT32                 ui32ForcedUpdatesRequested;
	IMG_UINT8                  ui8SLRLogWp;
	RGXFWIF_SLR_ENTRY          sSLRLogFirst;
	RGXFWIF_SLR_ENTRY          sSLRLog[PVR_SLR_LOG_ENTRIES];
	IMG_UINT64 RGXFW_ALIGN     ui64LastForcedUpdateTime;
#endif
#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
	volatile IMG_UINT32        aui32InterruptCount[MAX_THREAD_NUM]; /*!< Interrupt count from Threads > */
#else
	volatile IMG_UINT32        aui32InterruptCount[RGXFW_THREAD_NUM]; /*!< Interrupt count from Threads > */
#endif
	IMG_UINT32                 ui32KCCBCmdsExecuted;                  /*!< Executed Kernel CCB command count */
	RGXFWIF_DEV_VIRTADDR       sPowerSync;                            /*!< Sync prim used to signal the host the power off state */
	IMG_UINT32                 ui32FwOsDataFlags;                       /*!< Compatibility and other flags */
	IMG_UINT32                 aui32OfflineBuffer[RGXFWIF_OFFLINE_DATA_BUFFER_SIZE_IN_WORDS];
} UNCACHED_ALIGN RGXFWIF_OSDATA;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_OSDATA) == 584,
				"RGXFWIF_OSDATA is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif
/*!
 ******************************************************************************
 * HWR Data
 *****************************************************************************/
/*!
 * @Defgroup HWRInfo FW HWR shared data interface
 * @Brief Types grouping data structures and defines used in realising the HWR record.
 * @{
 */

#define RGXFW_PROCESS_NAME_LEN	(16)

/*! @Brief HWR Lockup types */
typedef enum
{
	RGX_HWRTYPE_UNKNOWNFAILURE = 0, /*!< Unknown failure */
	RGX_HWRTYPE_OVERRUN        = 1, /*!< DM overrun */
	RGX_HWRTYPE_POLLFAILURE    = 2, /*!< Poll failure */
	RGX_HWRTYPE_BIF0FAULT      = 3, /*!< BIF0 fault */
	RGX_HWRTYPE_BIF1FAULT      = 4, /*!< BIF1 fault */
	RGX_HWRTYPE_TEXASBIF0FAULT = 5, /*!< TEXASBIF0 fault */
	RGX_HWRTYPE_MMUFAULT       = 6, /*!< MMU fault */
	RGX_HWRTYPE_MMUMETAFAULT   = 7, /*!< MMU META fault */
	RGX_HWRTYPE_MIPSTLBFAULT   = 8, /*!< MIPS TLB fault */
	RGX_HWRTYPE_ECCFAULT       = 9, /*!< ECC fault */
	RGX_HWRTYPE_MMURISCVFAULT  = 10, /*!< MMU RISCV fault */
} RGX_HWRTYPE;

#define RGXFWIF_HWRTYPE_BIF_BANK_GET(eHWRType) (((eHWRType) == RGX_HWRTYPE_BIF0FAULT) ? 0 : 1)

#define RGXFWIF_HWRTYPE_PAGE_FAULT_GET(eHWRType) ((((eHWRType) == RGX_HWRTYPE_BIF0FAULT)      ||       \
                                                   ((eHWRType) == RGX_HWRTYPE_BIF1FAULT)      ||       \
                                                   ((eHWRType) == RGX_HWRTYPE_TEXASBIF0FAULT) ||       \
                                                   ((eHWRType) == RGX_HWRTYPE_MMUFAULT)       ||       \
                                                   ((eHWRType) == RGX_HWRTYPE_MMUMETAFAULT)   ||       \
                                                   ((eHWRType) == RGX_HWRTYPE_MIPSTLBFAULT)   ||       \
                                                   ((eHWRType) == RGX_HWRTYPE_MMURISCVFAULT)) ? true : false)

/************************
 *  GPU HW error codes  *
 ************************/
typedef enum
{
	RGX_HW_ERR_NA = 0x0,
	RGX_HW_ERR_PRIMID_FAILURE_DURING_DMKILL = 0x101,
} RGX_HW_ERR;

typedef struct
{
	IMG_UINT64	RGXFW_ALIGN		ui64BIFReqStatus; /*!< BIF request status */
	IMG_UINT64	RGXFW_ALIGN		ui64BIFMMUStatus; /*!< MMU status */
	IMG_UINT64	RGXFW_ALIGN		ui64PCAddress; /*!< phys address of the page catalogue */
	IMG_UINT64	RGXFW_ALIGN		ui64Reserved;
} RGX_BIFINFO;

typedef struct
{
	IMG_UINT32 ui32FaultGPU; /*!< ECC fault in GPU */
} RGX_ECCINFO;

typedef struct
{
	IMG_UINT64	RGXFW_ALIGN		aui64MMUStatus[2]; /*!< MMU status */
	IMG_UINT64	RGXFW_ALIGN		ui64PCAddress; /*!< phys address of the page catalogue */
	IMG_UINT64	RGXFW_ALIGN		ui64Reserved;
} RGX_MMUINFO;

typedef struct
{
	IMG_UINT32	ui32ThreadNum; /*!< Thread ID performing poll operation */
	IMG_UINT32	ui32CrPollAddr; /*!< CR Poll Address */
	IMG_UINT32	ui32CrPollMask; /*!< CR Poll mask */
	IMG_UINT32	ui32CrPollLastValue; /*!< CR Poll last value */
	IMG_UINT64	RGXFW_ALIGN ui64Reserved;
} UNCACHED_ALIGN RGX_POLLINFO;

typedef struct
{
	IMG_UINT32 ui32BadVAddr; /*!< VA address */
	IMG_UINT32 ui32EntryLo;
} RGX_TLBINFO;

/*! @Brief Structure to keep information specific to a lockup e.g. DM, timer, lockup type etc. */
typedef struct
{
	union
	{
		RGX_BIFINFO  sBIFInfo; /*!< BIF failure details */
		RGX_MMUINFO  sMMUInfo; /*!< MMU failure details */
		RGX_POLLINFO sPollInfo; /*!< Poll failure details */
		RGX_TLBINFO  sTLBInfo; /*!< TLB failure details */
		RGX_ECCINFO  sECCInfo; /*!< ECC failure details */
	} uHWRData;

	IMG_UINT64 RGXFW_ALIGN ui64CRTimer; /*!< Timer value at the time of lockup */
	IMG_UINT64 RGXFW_ALIGN ui64OSTimer; /*!< OS timer value at the time of lockup */
	IMG_UINT32             ui32FrameNum; /*!< Frame number of the workload */
	IMG_UINT32             ui32PID; /*!< PID belonging to the workload */
	IMG_UINT32             ui32ActiveHWRTData; /*!< HWRT data of the workload */
	IMG_UINT32             ui32HWRNumber; /*!< HWR number */
	IMG_UINT32             ui32EventStatus; /*!< Core specific event status register at the time of lockup */
	IMG_UINT32             ui32HWRRecoveryFlags; /*!< DM state flags */
	RGX_HWRTYPE            eHWRType; /*!< Type of lockup */
	RGXFWIF_DM             eDM; /*!< Recovery triggered for the DM */
	IMG_UINT32             ui32CoreID; /*!< Core ID of the GPU */
	RGX_HW_ERR             eHWErrorCode; /*!< Error code used to determine HW fault */
	IMG_UINT64 RGXFW_ALIGN ui64CRTimeOfKick; /*!< Workload kick time */
	IMG_UINT64 RGXFW_ALIGN ui64CRTimeHWResetStart; /*!< HW reset start time */
	IMG_UINT64 RGXFW_ALIGN ui64CRTimeHWResetFinish; /*!< HW reset stop time */
	IMG_UINT64 RGXFW_ALIGN ui64CRTimeFreelistReady; /*!< freelist ready time on the last HWR */
	IMG_CHAR   RGXFW_ALIGN szProcName[RGXFW_PROCESS_NAME_LEN]; /*!< User process name */
} UNCACHED_ALIGN RGX_HWRINFO;

#define RGXFWIF_HWINFO_MAX_FIRST 8U							/* Number of first HWR logs recorded (never overwritten by newer logs) */
#define RGXFWIF_HWINFO_MAX_LAST 8U							/* Number of latest HWR logs (older logs are overwritten by newer logs) */
#define RGXFWIF_HWINFO_MAX (RGXFWIF_HWINFO_MAX_FIRST + RGXFWIF_HWINFO_MAX_LAST)	/* Total number of HWR logs stored in a buffer */
#define RGXFWIF_HWINFO_LAST_INDEX (RGXFWIF_HWINFO_MAX - 1U)	/* Index of the last log in the HWR log buffer */

/*! @Brief Firmware HWR information structure allocated by the Services and used by the Firmware to update recovery information. */
typedef struct
{
	RGX_HWRINFO sHWRInfo[RGXFWIF_HWINFO_MAX]; /*!< Max number of recovery record */
	IMG_UINT32  ui32HwrCounter; /*!< HWR counter used in FL reconstruction */
	IMG_UINT32  ui32WriteIndex; /*!< Index for updating recovery information in sHWRInfo */
	IMG_UINT32  ui32DDReqCount; /*!< Count of DebugDump requested to the host after recovery */
	IMG_UINT32  ui32HWRInfoBufFlags; /* Compatibility and other flags */
	IMG_UINT32  aui32HwrDmLockedUpCount[RGXFWIF_DM_MAX]; /*!< Lockup count for each DM */
	IMG_UINT32  aui32HwrDmOverranCount[RGXFWIF_DM_MAX]; /*!< Overrun count for each DM */
	IMG_UINT32  aui32HwrDmRecoveredCount[RGXFWIF_DM_MAX]; /*!< Lockup + Overrun count for each DM */
	IMG_UINT32  aui32HwrDmFalseDetectCount[RGXFWIF_DM_MAX]; /*!< False lockup detection count for each DM */
} UNCACHED_ALIGN RGXFWIF_HWRINFOBUF;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_HWRINFOBUF) == 2336,
				"RGXFWIF_HWRINFOBUF is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

/*! @} End of HWRInfo */

typedef RGXFWIF_DEV_VIRTADDR  PRGXFWIF_CCCB;
typedef RGXFWIF_DEV_VIRTADDR  PRGXFWIF_CCCB_CTL;
typedef RGXFWIF_DEV_VIRTADDR  PRGXFWIF_COMMONCTX_STATE;
typedef RGXFWIF_DEV_VIRTADDR  PRGXFWIF_RF_CMD;
typedef RGXFWIF_DEV_VIRTADDR  PRGXFWIF_FWMEMCONTEXT;

/*!
 * @InGroup RenderTarget
 * @Brief Firmware Freelist holding usage state of the Parameter Buffers
 */
typedef struct
{
	IMG_DEV_VIRTADDR	RGXFW_ALIGN psFreeListDevVAddr;	/*!< Freelist page table base */
	IMG_UINT64		RGXFW_ALIGN ui64CurrentDevVAddr;/*!< Freelist page table entry for current free page */
	IMG_UINT32		ui32CurrentStackTop;		/*!< Freelist current free page */
	IMG_UINT32		ui32MaxPages;			/*!< Max no. of pages can be added to the freelist */
	IMG_UINT32		ui32GrowPages;			/*!< No pages to add in each freelist grow */
	IMG_UINT32		ui32CurrentPages;		/*!< Total no. of pages made available to the PM HW */
	IMG_UINT32		ui32AllocatedPageCount;		/*!< No. of pages allocated by PM HW */
	IMG_UINT32		ui32AllocatedMMUPageCount;	/*!< No. of pages allocated for GPU MMU for PM*/
#if defined(SUPPORT_SHADOW_FREELISTS) && !defined(SUPPORT_OPEN_SOURCE_DRIVER)
	IMG_UINT32		ui32HWRCounter;
	PRGXFWIF_FWMEMCONTEXT	psFWMemContext;
#endif
	IMG_UINT32		ui32FreeListID;			/*!< Unique Freelist ID */
	IMG_BOOL		bGrowPending;			/*!< Freelist grow is pending */
	IMG_UINT32		ui32ReadyPages;			/*!< Reserved pages to be used only on PM OOM event */
	IMG_UINT32		ui32FreelistFlags;		/*!< Compatibility and other flags */
#if defined(SUPPORT_AGP) || defined(SUPPORT_OPEN_SOURCE_DRIVER)
	IMG_UINT32		ui32PmGlobalPb;			/*!< PM Global PB on which Freelist is loaded */
#endif
} UNCACHED_ALIGN RGXFWIF_FREELIST;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_FREELIST) == 64,
				"RGXFWIF_FREELIST is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

typedef struct {RGXFWIF_DEV_VIRTADDR sNext;
                RGXFWIF_DEV_VIRTADDR sPrev;}	RGXFW_DLLIST_NODE;

#define RGXFWIF_MAX_NUM_CANCEL_REQUESTS		(8U)		/* Maximum number of workload cancellation requests */

/*!
 * @InGroup WorkloadContexts
 * @Brief Firmware Common Context (or FWCC)
 */
typedef struct RGXFWIF_FWCOMMONCONTEXT_
{
	/* CCB details for this firmware context */
	PRGXFWIF_CCCB_CTL		psCCBCtl;				/*!< CCB control */
	PRGXFWIF_CCCB			psCCB;					/*!< CCB base */

	/* Context suspend state */
	PRGXFWIF_COMMONCTX_STATE	RGXFW_ALIGN psContextState;		/*!< TA/3D context suspend state, read/written by FW */

	/* Flags e.g. for context switching */
	IMG_UINT32				ui32FWComCtxFlags;
	IMG_INT32				i32Priority;  /*!< Priority level */
	IMG_UINT32				ui32PrioritySeqNum;

	/* Framework state */
	PRGXFWIF_RF_CMD			RGXFW_ALIGN psRFCmd;		/*!< Register updates for Framework */

	/* Misc and compatibility flags */
	IMG_UINT32				ui32CompatFlags;

	/* Statistic updates waiting to be passed back to the host... */
	IMG_INT32				i32StatsNumStores;		/*!< Number of stores on this context since last update */
	IMG_INT32				i32StatsNumOutOfMemory;		/*!< Number of OOMs on this context since last update */
	IMG_INT32				i32StatsNumPartialRenders;	/*!< Number of PRs on this context since last update */
	RGXFWIF_DM				eDM;				/*!< Data Master type */
	RGXFW_DLLIST_NODE		RGXFW_ALIGN  sBufStalledNode;			/*!< List entry for the buffer stalled list */
	IMG_UINT64				RGXFW_ALIGN  ui64CBufQueueCtrlAddr;	/*!< Address of the circular buffer queue pointers */

	IMG_UINT64				RGXFW_ALIGN  ui64RobustnessAddress;
	IMG_UINT32				ui32MaxDeadlineMS;			/*!< Max HWR deadline limit in ms */
	bool					bReadOffsetNeedsReset;			/*!< Following HWR circular buffer read-offset needs resetting */

	RGXFW_DLLIST_NODE		RGXFW_ALIGN sWaitingNode;		/*!< List entry for the waiting list */
	RGXFW_DLLIST_NODE		RGXFW_ALIGN sRunNode;			/*!< List entry for the run list */
	RGXFWIF_UFO				sLastFailedUFO;			/*!< UFO that last failed (or NULL) */

	PRGXFWIF_FWMEMCONTEXT	psFWMemContext;					/*!< Memory context */

	/* References to the host side originators */
	IMG_UINT32				ui32ServerCommonContextID;	/*!< the Server Common Context */
	IMG_UINT32				ui32PID;			/*!< associated process ID */

	IMG_BOOL				bGeomOOMDisabled;		/*!< True when Geom DM OOM is not allowed */
	IMG_CHAR				szProcName[RGXFW_PROCESS_NAME_LEN];	/*!< User process name */
	IMG_UINT32				ui32DeferCount;		/*!< Number of context defers before forced scheduling of context */
	IMG_UINT32				aui32FirstIntJobRefToCancel[RGXFWIF_MAX_NUM_CANCEL_REQUESTS];	/*!< Saved values of the beginning of range of IntJobRefs at and above which workloads will be discarded */
	IMG_UINT32				aui32FirstValidIntJobRef[RGXFWIF_MAX_NUM_CANCEL_REQUESTS];	/*!< Saved values of the end of range of IntJobRef below which workloads will be discarded */
	IMG_BOOL				bCancelRangesActive;	/*!< True if any active ranges in aui32FirstIntJobRefToCancel and aui32FirstValidIntJobRef arrays */
	IMG_BOOL				bLastKickedCmdWasSafetyOnly;
	IMG_UINT32				ui32UID;			/*!< associated process UID used in FW managed gpu work period hwperf events */
} UNCACHED_ALIGN RGXFWIF_FWCOMMONCONTEXT;

static_assert(sizeof(RGXFWIF_FWCOMMONCONTEXT) <= 256U,
              "Size of structure RGXFWIF_FWCOMMONCONTEXT exceeds maximum expected size.");

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_FWCOMMONCONTEXT) == 168,
				"RGXFWIF_FWCOMMONCONTEXT is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

/*!
 ******************************************************************************
 * HWRTData
 *****************************************************************************/

typedef RGXFWIF_DEV_VIRTADDR  PRGXFWIF_FREELIST;

/* HWRTData flags */
/* Deprecated flags 1:0 */
#define HWRTDATA_HAS_LAST_TA              (IMG_UINT32_C(1) << 2)
#define HWRTDATA_PARTIAL_RENDERED         (IMG_UINT32_C(1) << 3)
#define HWRTDATA_DISABLE_TILE_REORDERING  (IMG_UINT32_C(1) << 4)
#define HWRTDATA_NEED_BRN65101_BLIT       (IMG_UINT32_C(1) << 5)
#define HWRTDATA_FIRST_BRN65101_STRIP     (IMG_UINT32_C(1) << 6)
#define HWRTDATA_NEED_BRN67182_2ND_RENDER (IMG_UINT32_C(1) << 7)
#if defined(SUPPORT_AGP)
#define HWRTDATA_GLOBAL_PB_NUMBER_BIT0    (IMG_UINT32_C(1) << 8)
#if defined(SUPPORT_AGP4)
#define HWRTDATA_GLOBAL_PB_NUMBER_BIT1    (IMG_UINT32_C(1) << 9)
#endif
#define HWRTDATA_GEOM_NEEDS_RESUME        (IMG_UINT32_C(1) << 10)
#endif
#if defined(SUPPORT_TRP)
#define HWRTDATA_GEOM_TRP_IN_PROGRESS          (IMG_UINT32_C(1) << 9)
#endif

typedef enum
{
	RGXFWIF_RTDATA_STATE_NONE = 0,
	RGXFWIF_RTDATA_STATE_KICKTA,
	RGXFWIF_RTDATA_STATE_KICKTAFIRST,
	RGXFWIF_RTDATA_STATE_TAFINISHED,
	RGXFWIF_RTDATA_STATE_KICK3D,
	RGXFWIF_RTDATA_STATE_3DFINISHED,
	RGXFWIF_RTDATA_STATE_3DCONTEXTSTORED,
	RGXFWIF_RTDATA_STATE_TAOUTOFMEM,
	RGXFWIF_RTDATA_STATE_PARTIALRENDERFINISHED,
	/* In case of HWR, we can't set the RTDATA state to NONE,
	 * as this will cause any TA to become a first TA.
	 * To ensure all related TA's are skipped, we use the HWR state */
	RGXFWIF_RTDATA_STATE_HWR,
	RGXFWIF_RTDATA_STATE_UNKNOWN = 0x7FFFFFFFU
} RGXFWIF_RTDATA_STATE;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
#define MAX_FREELISTS_SIZE 3

static_assert(RGXFW_MAX_FREELISTS <= MAX_FREELISTS_SIZE,
				"RGXFW_MAX_FREELISTS is outside of allowable range for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

typedef struct
{
	IMG_BOOL							bTACachesNeedZeroing;

	IMG_UINT32							ui32ScreenPixelMax;
	IMG_UINT64							RGXFW_ALIGN ui64MultiSampleCtl;
	IMG_UINT64							ui64FlippedMultiSampleCtl;
	IMG_UINT32							ui32TPCStride;
	IMG_UINT32							ui32TPCSize;
	IMG_UINT32							ui32TEScreen;
	IMG_UINT32							ui32MTileStride;
	IMG_UINT32							ui32TEAA;
	IMG_UINT32							ui32TEMTILE1;
	IMG_UINT32							ui32TEMTILE2;
	IMG_UINT32							ui32ISPMergeLowerX;
	IMG_UINT32							ui32ISPMergeLowerY;
	IMG_UINT32							ui32ISPMergeUpperX;
	IMG_UINT32							ui32ISPMergeUpperY;
	IMG_UINT32							ui32ISPMergeScaleX;
	IMG_UINT32							ui32ISPMergeScaleY;
	IMG_UINT32							uiRgnHeaderSize;
	IMG_UINT32							ui32ISPMtileSize;
} UNCACHED_ALIGN RGXFWIF_HWRTDATA_COMMON;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_HWRTDATA_COMMON) == 88,
				"RGXFWIF_HWRTDATA_COMMON is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

typedef struct
{
	IMG_UINT32           ui32RenderTargetIndex;		//Render number
	IMG_UINT32           ui32CurrentRenderTarget;	//index in RTA
	IMG_UINT32           ui32ActiveRenderTargets;	//total active RTs
	IMG_UINT32           ui32CumulActiveRenderTargets;   //total active RTs from the first TA kick, for OOM
	RGXFWIF_DEV_VIRTADDR sValidRenderTargets;  //Array of valid RT indices
	RGXFWIF_DEV_VIRTADDR sRTANumPartialRenders;  //Array of number of occurred partial renders per render target
	IMG_UINT32           ui32MaxRTs;   //Number of render targets in the array
	IMG_UINT32           ui32RTACtlFlags; /* Compatibility and other flags */
} UNCACHED_ALIGN RGXFWIF_RTA_CTL;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_RTA_CTL) == 32,
				"RGXFWIF_RTA_CTL is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

/*!
 * @InGroup RenderTarget
 * @Brief Firmware Render Target data i.e. HWRTDATA used to hold the PM context
 */
typedef struct
{
	IMG_DEV_VIRTADDR		RGXFW_ALIGN psPMMListDevVAddr;			/*!< MList Data Store */

	IMG_UINT64			RGXFW_ALIGN ui64VCECatBase[1];			/*!< VCE Page Catalogue base */
	IMG_UINT64			RGXFW_ALIGN ui64VCELastCatBase[1];
	IMG_UINT64			RGXFW_ALIGN ui64TECatBase[1];			/*!< TE Page Catalogue base */
	IMG_UINT64			RGXFW_ALIGN ui64TELastCatBase[1];
	IMG_UINT64			RGXFW_ALIGN ui64AlistCatBase;			/*!< Alist Page Catalogue base */
	IMG_UINT64			RGXFW_ALIGN ui64AlistLastCatBase;

	IMG_UINT64			RGXFW_ALIGN ui64PMAListStackPointer;		/*!< Freelist page table entry for current Mlist page */
	IMG_UINT32			ui32PMMListStackPointer;			/*!< Current Mlist page */

	RGXFWIF_DEV_VIRTADDR		sHWRTDataCommonFwAddr;				/*!< Render target dimension dependent data */

	IMG_UINT32			ui32HWRTDataFlags;
	RGXFWIF_RTDATA_STATE		eState;						/*!< Current workload processing state of HWRTDATA */

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
	PRGXFWIF_FREELIST		RGXFW_ALIGN apsFreeLists[MAX_FREELISTS_SIZE];	/*!< Freelist to use */
	IMG_UINT32			aui32FreeListHWRSnapshot[MAX_FREELISTS_SIZE];
#else
	PRGXFWIF_FREELIST		RGXFW_ALIGN apsFreeLists[RGXFW_MAX_FREELISTS];	/*!< Freelist to use */
	IMG_UINT32			aui32FreeListHWRSnapshot[RGXFW_MAX_FREELISTS];
#endif

	IMG_DEV_VIRTADDR		RGXFW_ALIGN psVHeapTableDevVAddr;		/*!< VHeap table base */

	RGXFWIF_RTA_CTL			sRTACtl;					/*!< Render target array data */

	IMG_DEV_VIRTADDR		RGXFW_ALIGN sTailPtrsDevVAddr;			/*!< Tail pointers base */
	IMG_DEV_VIRTADDR		RGXFW_ALIGN sMacrotileArrayDevVAddr;		/*!< Macrotiling array base */
	IMG_DEV_VIRTADDR		RGXFW_ALIGN sRgnHeaderDevVAddr;			/*!< Region headers base */
	IMG_DEV_VIRTADDR		RGXFW_ALIGN sRTCDevVAddr;			/*!< Render target cache base */
#if defined(RGX_FIRMWARE)
	struct RGXFWIF_FWCOMMONCONTEXT_* RGXFW_ALIGN psOwnerGeom;
#else
	RGXFWIF_DEV_VIRTADDR		RGXFW_ALIGN pui32OwnerGeomNotUsedByHost;
#endif
#if defined(SUPPORT_TRP) && !defined(SUPPORT_OPEN_SOURCE_DRIVER)
	IMG_UINT32			ui32KickFlagsCopy;
	IMG_UINT32			ui32TEPageCopy;
	IMG_UINT32			ui32VCEPageCopy;
#endif
#if defined(SUPPORT_AGP) || defined(SUPPORT_OPEN_SOURCE_DRIVER)
	IMG_BOOL			bTACachesNeedZeroing;
#endif

	RGXFWIF_CLEANUP_CTL		RGXFW_ALIGN_DCACHEL sCleanupState;					/*!< Render target clean up state */
} RGXFW_ALIGN_DCACHEL RGXFWIF_HWRTDATA;

#if defined(SUPPORT_OPEN_SOURCE_DRIVER)
static_assert(sizeof(RGXFWIF_HWRTDATA) == 256,
				"RGXFWIF_HWRTDATA is incorrect size for SUPPORT_OPEN_SOURCE_DRIVER");
#endif

/* Sync_checkpoint firmware object.
 * This is the FW-addressable structure used to hold the sync checkpoint's
 * state and other information which needs to be accessed by the firmware.
 */
typedef struct
{
	IMG_UINT32	ui32State;          /*!< Holds the current state of the sync checkpoint */
	IMG_UINT32	ui32FwRefCount;     /*!< Holds the FW reference count (num of fences/updates processed) */
} SYNC_CHECKPOINT_FW_OBJ;

/* Bit mask Firmware can use to test if a checkpoint has signalled or errored */
#define SYNC_CHECKPOINT_SIGNALLED_MASK (0x1 << 0)

typedef enum
{
	RGXFWIF_REG_CFG_TYPE_PWR_ON=0,      /* Sidekick power event */
	RGXFWIF_REG_CFG_TYPE_DUST_CHANGE,   /* Rascal / dust power event */
	RGXFWIF_REG_CFG_TYPE_TA,            /* TA kick */
	RGXFWIF_REG_CFG_TYPE_3D,            /* 3D kick */
	RGXFWIF_REG_CFG_TYPE_CDM,           /* Compute kick */
	RGXFWIF_REG_CFG_TYPE_TLA,           /* TLA kick */
	RGXFWIF_REG_CFG_TYPE_TDM,           /* TDM kick */
	RGXFWIF_REG_CFG_TYPE_ALL            /* Applies to all types. Keep as last element */
} RGXFWIF_REG_CFG_TYPE;

#define RGXFWIF_KM_USC_TQ_SHADER_CODE_OFFSET_BYTES                 RGX_HEAP_KM_USC_RESERVED_REGION_OFFSET
#define RGXFWIF_KM_USC_TQ_SHADER_CODE_MAX_SIZE_BYTES               (1U << 19)

#define RGX_HEAP_KM_USC_RESERVED_SIZE                              RGXFWIF_KM_USC_TQ_SHADER_CODE_MAX_SIZE_BYTES
#define RGX_HEAP_USC_RESERVED_TOTAL_SIZE                           RGX_HEAP_UM_USC_RESERVED_SIZE + RGX_HEAP_KM_USC_RESERVED_SIZE

#endif /*  RGX_FWIF_SHARED_H */

/******************************************************************************
 End of file (rgx_fwif_shared.h)
******************************************************************************/

/*************************************************************************/ /*!
@File           ri_server.c
@Title          Resource Information (RI) server implementation
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Resource Information (RI) server functions
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

#if defined(__linux__) && defined(__KERNEL__)
 #include <linux/version.h>
 #if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
  #include <linux/stdarg.h>
 #else
  #include <stdarg.h>
 #endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) */
#else
 #include <stdarg.h>
#endif /* __linux__ */
#include "img_defs.h"
#include "allocmem.h"
#include "pvr_debug.h"
#include "pvrsrv_error.h"
#include "osfunc.h"

#include "srvkm.h"
#include "lock.h"

/* services/include */
#include "pvr_ricommon.h"

/* services/server/include/ */
#include "ri_server.h"

/* services/include/shared/ */
#include "hash.h"
/* services/shared/include/ */
#include "dllist.h"

#include "pmr.h"
#include "physheap.h"

/* include/device.h */
#include "device.h"

#if !defined(RI_UNIT_TEST)
#include "pvrsrv.h"
#endif


#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)

#define USE_RI_LOCK		1

/*
 * Initial size use for Hash table. (Used to index the RI list entries).
 */
#define _RI_INITIAL_HASH_TABLE_SIZE	64

/*
 * If this define is set to 1, details of the linked lists (addresses,
 * prev/next ptrs, etc) are also output when function RIDumpList() is called.
 */
#define _DUMP_LINKEDLIST_INFO		0


typedef IMG_UINT64 _RI_BASE_T;


/* No +1 in SIZE macros since sizeof includes \0 byte in size */

#define RI_PROC_BUF_SIZE    16
#define RI_ANNO_BUF_SIZE    80
#define RI_ANNO_FRMT_SIZE (sizeof(RI_ANNO_FRMT))

#define RI_DEV_ID_BUF_SIZE  4

#define RI_MEMDESC_SUM_FRMT     "PID:%d %s MEMDESCs Alloc'd:0x%010" IMG_UINT64_FMTSPECx " (%" IMG_UINT64_FMTSPEC "K) + "\
                                                  "Imported:0x%010" IMG_UINT64_FMTSPECx " (%" IMG_UINT64_FMTSPEC "K) = "\
                                                     "Total:0x%010" IMG_UINT64_FMTSPECx " (%" IMG_UINT64_FMTSPEC "K)\n"
#define RI_MEMDESC_SUM_BUF_SIZE (sizeof(RI_MEMDESC_SUM_FRMT)+5+RI_PROC_BUF_SIZE+30+60)


#define RI_PMR_SUM_FRMT     "PID:%d %s PMRs Alloc'd:0x%010" IMG_UINT64_FMTSPECx ", %" IMG_UINT64_FMTSPEC "K  "\
                                        "[Physical: 0x%010" IMG_UINT64_FMTSPECx ", %" IMG_UINT64_FMTSPEC "K]\n"
#define RI_PMR_SUM_BUF_SIZE (sizeof(RI_PMR_SUM_FRMT)+(20+40))

#define RI_FREED_BY_DRIVER "{Freed by KM}"

#define RI_PMR_ENTRY_FRMT      "%%sPID:%%-5d DEV:%%s <%%p>\t%%-%ds\t%%-%ds\t0x%%010" IMG_UINT64_FMTSPECx "\t[0x%%010" IMG_UINT64_FMTSPECx "]\t%%s%%c"
#define RI_PMR_ENTRY_BUF_SIZE  (sizeof(RI_PMR_ENTRY_FRMT)+(3+5+RI_DEV_ID_BUF_SIZE+16+(PVR_ANNOTATION_MAX_LEN/2)+PHYS_HEAP_NAME_SIZE+10+10)+ sizeof(RI_FREED_BY_DRIVER))
#define RI_PMR_ENTRY_FRMT_SIZE (sizeof(RI_PMR_ENTRY_FRMT))

/* Use %5d rather than %d so the output aligns in server/kernel.log, debugFS sees extra spaces */
#define RI_MEMDESC_ENTRY_PROC_FRMT        "[%5d:%s]"
#define RI_MEMDESC_ENTRY_PROC_BUF_SIZE    (sizeof(RI_MEMDESC_ENTRY_PROC_FRMT)+5+16)

#define RI_SYS_ALLOC_IMPORT_FRMT      "{Import from PID %d}"
#define RI_SYS_ALLOC_IMPORT_FRMT_SIZE (sizeof(RI_SYS_ALLOC_IMPORT_FRMT)+5)
static IMG_CHAR g_szSysAllocImport[RI_SYS_ALLOC_IMPORT_FRMT_SIZE];

#define RI_MEMDESC_ENTRY_IMPORT_FRMT     "{Import from PID %d}"
#define RI_MEMDESC_ENTRY_IMPORT_BUF_SIZE (sizeof(RI_MEMDESC_ENTRY_IMPORT_FRMT)+5)

#define RI_MEMDESC_ENTRY_FRMT      "%%sPID:%%-5d DEV:%%s 0x%%010" IMG_UINT64_FMTSPECx "\t%%-%ds %%s\t0x%%010" IMG_UINT64_FMTSPECx "\t<%%p> %%s%%s %%s%%c"
#define RI_MEMDESC_ENTRY_BUF_SIZE  (sizeof(RI_MEMDESC_ENTRY_FRMT)+(3+5+RI_DEV_ID_BUF_SIZE+10+PVR_ANNOTATION_MAX_LEN+RI_MEMDESC_ENTRY_PROC_BUF_SIZE+16+\
                                               RI_MEMDESC_ENTRY_IMPORT_BUF_SIZE+RI_SYS_ALLOC_IMPORT_FRMT_SIZE)) + sizeof(RI_FREED_BY_DRIVER)
#define RI_MEMDESC_ENTRY_FRMT_SIZE (sizeof(RI_MEMDESC_ENTRY_FRMT))


#define RI_FRMT_SIZE_MAX (MAX(RI_MEMDESC_ENTRY_BUF_SIZE,\
                              MAX(RI_PMR_ENTRY_BUF_SIZE,\
                                  MAX(RI_MEMDESC_SUM_BUF_SIZE,\
                                      RI_PMR_SUM_BUF_SIZE))))

typedef struct _RI_PMR_INFO_
{
	uintptr_t uiAddr;
	PHYS_HEAP *psHeap;
	PVRSRV_DEVICE_NODE *psDeviceNode;
	size_t uiLogicalSize;
	size_t uiPhysicalSize;
	IMG_CHAR *pszAnnotation;
} RI_PMR_INFO;


#define RI_DEVID_MASK						0xFF /* Highest value is PVRSRV_HOST_DEVICE_ID[255] */

/* Flags for Memdescs(RI_SUBLIST) */
#define RI_IMPORT_FLAG						(8)  /* Indicates the PMR is an import */
#define RI_SUBALLOC_FLAG					(9)  /* Indicates the PMR is sub-allocatable */

/* Flags for PMRs(RI_LIST) */
#define RI_SYSALLOC_PMR_FLAG				(10) /* Indicates the PMR belongs to the 'system' process */
#define RI_PMR_PHYS_COUNTED_BY_DEBUGFS_FLAG	(11) /* Indicates size counted prior, to prevent double reads */

/* Shared flags */
#define RI_RACC_FLAG						(12) /* Freed by Driver after disconnect */
#define RI_VALID_FLAG						(13) /* Valid on creation, Only invalid after delete */
#define RI_HAS_PMR_INFO						(14) /* Entry has RI_PMR_INFO object rather than reference to a PMR */

#define GET_DEVICE_ID(entry)			((entry)->ui16Flags & RI_DEVID_MASK)
#define SET_DEVICE_ID(entry,id)			BITMASK_SET((entry)->ui16Flags, (id & RI_DEVID_MASK))
#define IS_HOST_DEVICE(entry)			(((entry)->ui16Flags & RI_DEVID_MASK) == PVRSRV_HOST_DEVICE_ID)

#define IS_IMPORT(entry)				BIT_ISSET((entry)->ui16Flags, RI_IMPORT_FLAG)
#define IS_SUBALLOC(entry)				BIT_ISSET((entry)->ui16Flags, RI_SUBALLOC_FLAG)

#define IS_SYSPMR(entry)				BIT_ISSET((entry)->ui16Flags, RI_SYSALLOC_PMR_FLAG)
#define IS_COUNTED_BY_DEBUGFS(entry)	BIT_ISSET((entry)->ui16Flags, RI_PMR_PHYS_COUNTED_BY_DEBUGFS_FLAG)

#define IS_RACC(entry)					BIT_ISSET((entry)->ui16Flags, RI_RACC_FLAG)
#define IS_VALID(entry)					BIT_ISSET((entry)->ui16Flags, RI_VALID_FLAG)
#define HAS_PMR_INFO(entry)				BIT_ISSET((entry)->ui16Flags, RI_HAS_PMR_INFO)

/* Structure used to make linked sublist of memory allocations (MEMDESC) */
struct _RI_SUBLIST_ENTRY_
{
	DLLIST_NODE				sListNode;
	DLLIST_NODE				sProcListNode;
	CONNECTION_DATA			*psConnection;
	struct _RI_LIST_ENTRY_	*psRI;
	IMG_CHAR				*pszTextB; /* Annotation+(NUL)+ProcName+(NUL) */
	IMG_DEV_VIRTADDR		sVAddr;
	IMG_UINT64				ui64Offset;
	IMG_UINT64				ui64Size;
	IMG_PID					pid;
	IMG_UINT16				ui16TextBLength; /* Total length of Annotation+(NUL)+ProcName+(NUL) */
	IMG_UINT16				ui16Flags; /* Refer to above */
};

/*
 * Structure used to make linked list of PMRs. Sublists of allocations
 * (MEMDESCs) made from these PMRs are chained off these entries.
 */
struct _RI_LIST_ENTRY_
{
	DLLIST_NODE				sListNode;
	DLLIST_NODE				sSubListFirst;
	union {
		PMR *psPMR;
		RI_PMR_INFO *psPmrInfo;
	} pmr_info;
	IMG_PID					pid;
	IMG_UINT16				ui16SubListCount;
	IMG_UINT16				ui16Flags; /* Refer to above */
};

#define GET_ADDR(entry) ( \
		HAS_PMR_INFO(entry) \
			? (void*)(entry)->pmr_info.psPmrInfo->uiAddr \
			: (void*)(entry)->pmr_info.psPMR \
	)
#define GET_HEAP(entry) ( \
		HAS_PMR_INFO(entry) \
			? (entry)->pmr_info.psPmrInfo->psHeap \
			: PMR_PhysHeap((entry)->pmr_info.psPMR) \
	)
#define GET_DEVNODE(entry) ( \
		HAS_PMR_INFO(entry) \
			? (entry)->pmr_info.psPmrInfo->psDeviceNode \
			: (PVRSRV_DEVICE_NODE *) PMR_DeviceNode((entry)->pmr_info.psPMR) \
	)
#define GET_LOGICAL_SIZE(entry) ( \
		HAS_PMR_INFO(entry) \
			? (entry)->pmr_info.psPmrInfo->uiLogicalSize \
			: PMR_LogicalSize((entry)->pmr_info.psPMR) \
	)
#define GET_PHYSICAL_SIZE(entry) ( \
		HAS_PMR_INFO(entry) \
			? (entry)->pmr_info.psPmrInfo->uiPhysicalSize \
			: PMR_PhysicalSize((entry)->pmr_info.psPMR) \
	)
#define GET_NAME(entry) ( \
		HAS_PMR_INFO(entry) \
			? (entry)->pmr_info.psPmrInfo->pszAnnotation \
			: PMR_GetAnnotation((entry)->pmr_info.psPMR) \
	)

/* pszTextB = <Memdesc Annotation>,0,<Proc Name>,0 */
/* Retrieve the string pointer to the ProcName */
#define GET_PROC(entry) ((IMG_CHAR*) (strchr((entry)->pszTextB, '\0') + 1))

typedef struct _RI_LIST_ENTRY_ RI_LIST_ENTRY;
typedef struct _RI_SUBLIST_ENTRY_ RI_SUBLIST_ENTRY;

static HASH_TABLE	*g_pPMR2RIListHashTable;
static HASH_TABLE	*g_PID2RISublistHashTable;

static POS_LOCK		g_hRILock;

/* Linked list of PMR allocations made against the PVR_SYS_ALLOC_PID and lock
 * to prevent concurrent access to it.
 */
static POS_LOCK		g_hSysAllocPidListLock;
static DLLIST_NODE	g_sSysAllocPidListHead;

/*
 * Flag used to indicate if RILock should be destroyed when final PMR entry is
 * deleted, i.e. if RIDeInitKM() has already been called before that point but
 * the handle manager has deferred deletion of RI entries.
 */
static IMG_BOOL		bRIDeInitDeferred = IMG_FALSE;

/*
 * Used as head of linked-list of PMR RI entries - this is useful when we wish
 * to iterate all PMR list entries (when we don't have a PMR ref)
 */
static DLLIST_NODE	g_sClientsListHead;

/* Function used to produce string containing info for MEMDESC RI entries (used for both debugfs and kernel log output) */
static void _GenerateMEMDESCEntryString(RI_SUBLIST_ENTRY *psRISubEntry, IMG_BOOL bDebugFs, IMG_UINT16 ui16MaxStrLen, IMG_CHAR *pszEntryString);
/* Function used to produce string containing info for PMR RI entries (used for both debugfs and kernel log output) */
static void _GeneratePMREntryString(RI_LIST_ENTRY *psRIEntry, IMG_BOOL bDebugFs, IMG_UINT16 ui16MaxStrLen, IMG_CHAR *pszEntryString);

static PVRSRV_ERROR _DumpAllEntries (uintptr_t k, uintptr_t v, void* pvPriv);
static PVRSRV_ERROR _DeleteAllEntries (uintptr_t k, uintptr_t v, void* pvPriv);
static PVRSRV_ERROR _DeleteAllProcEntries (uintptr_t k, uintptr_t v, void* pvPriv);
static PVRSRV_ERROR _DumpList(PMR *psPMR, IMG_PID pid);
#define _RIOutput(x) PVR_LOG(x)

static IMG_UINT32
_ProcHashFunc(size_t uKeySize, void *pKey, IMG_UINT32 uHashTabLen);

static IMG_UINT32
_ProcHashFunc(size_t uKeySize, void *pKey, IMG_UINT32 uHashTabLen)
{
	IMG_UINT32 *p = (IMG_UINT32 *)pKey;
	IMG_UINT32 uKeyLen = uKeySize / sizeof(IMG_UINT32);
	IMG_UINT32 ui;
	IMG_UINT32 uHashKey = 0;

	PVR_UNREFERENCED_PARAMETER(uHashTabLen);

	for (ui = 0; ui < uKeyLen; ui++)
	{
		IMG_UINT32 uHashPart = *p++;

		uHashPart += (uHashPart << 12);
		uHashPart ^= (uHashPart >> 22);
		uHashPart += (uHashPart << 4);
		uHashPart ^= (uHashPart >> 9);
		uHashPart += (uHashPart << 10);
		uHashPart ^= (uHashPart >> 2);
		uHashPart += (uHashPart << 7);
		uHashPart ^= (uHashPart >> 12);

		uHashKey += uHashPart;
	}

	return uHashKey;
}

static IMG_BOOL
_ProcHashComp(size_t uKeySize, void *pKey1, void *pKey2);

static IMG_BOOL
_ProcHashComp(size_t uKeySize, void *pKey1, void *pKey2)
{
	IMG_UINT32 *p1 = (IMG_UINT32 *)pKey1;
	IMG_UINT32 *p2 = (IMG_UINT32 *)pKey2;
	IMG_UINT32 uKeyLen = uKeySize / sizeof(IMG_UINT32);
	IMG_UINT32 ui;

	for (ui = 0; ui < uKeyLen; ui++)
	{
		if (*p1++ != *p2++)
			return IMG_FALSE;
	}

	return IMG_TRUE;
}

static void _RILock(void)
{
#if (USE_RI_LOCK == 1)
	OSLockAcquire(g_hRILock);
#endif
}

static void _RIUnlock(void)
{
#if (USE_RI_LOCK == 1)
	OSLockRelease(g_hRILock);
#endif
}

/* This value maintains a count of the number of PMRs attributed to the
 * PVR_SYS_ALLOC_PID. Access to this value is protected by g_hRILock, so it
 * does not need to be an ATOMIC_T.
 */
static IMG_UINT32 g_ui32SysAllocPMRCount;

static IMG_BOOL RICheckListHandle(RI_HANDLE hRIHandle)
{
	RI_LIST_ENTRY *psRIEntry = (RI_LIST_ENTRY *) hRIHandle, *psTableEntry;
	void *pPMRHashKey;
	IMG_BOOL bValid = IMG_FALSE;

	if ((GET_DEVICE_ID(psRIEntry) > PVRSRV_MAX_DEVICES &&
	     !IS_HOST_DEVICE(psRIEntry)) ||
	    !g_pPMR2RIListHashTable)
	{
		//Shortcut check
		return IMG_FALSE;
	}

	pPMRHashKey = GET_ADDR(psRIEntry);

	_RILock();

	/* Look-up psPMR in Hash Table */
	psTableEntry = (RI_LIST_ENTRY *) HASH_Retrieve_Extended(g_pPMR2RIListHashTable,
	                                                        &pPMRHashKey);
	if (psTableEntry != NULL)
	{
		bValid = IS_VALID(psTableEntry);
	}

	_RIUnlock();

	return bValid;
}

static IMG_BOOL RICheckSubListHandle(RI_HANDLE hRIHandle)
{
	RI_SUBLIST_ENTRY *psRISubEntry, *psTableEntry;
	uintptr_t hashData;
	IMG_PID pid;
	IMG_BOOL bValid = IMG_FALSE;


	psRISubEntry = (RI_SUBLIST_ENTRY *)hRIHandle;
	if ((GET_DEVICE_ID(psRISubEntry) > PVRSRV_MAX_DEVICES) || !g_PID2RISublistHashTable)
	{
		//Shortcut check
		return bValid;
	}
	pid = psRISubEntry->pid;

	_RILock();
	/* Look-up psPMR in Hash Table */
	hashData = HASH_Retrieve_Extended (g_PID2RISublistHashTable, (void *)&pid);
	if (hashData)
	{
		psTableEntry = IMG_CONTAINER_OF((PDLLIST_NODE)hashData, RI_SUBLIST_ENTRY, sProcListNode);
		bValid = IS_VALID(psTableEntry);
	}

	/* Release RI Lock */
	_RIUnlock();
	return bValid;
}

PVRSRV_ERROR RIInitKM(void)
{
	IMG_INT iCharsWritten;
	PVRSRV_ERROR eError;

	bRIDeInitDeferred = IMG_FALSE;

	iCharsWritten = OSSNPrintf(g_szSysAllocImport,
	            RI_SYS_ALLOC_IMPORT_FRMT_SIZE,
	            RI_SYS_ALLOC_IMPORT_FRMT,
	            PVR_SYS_ALLOC_PID);
	PVR_LOG_IF_FALSE((iCharsWritten>0 && iCharsWritten<(IMG_INT32)RI_SYS_ALLOC_IMPORT_FRMT_SIZE),
			"OSSNPrintf failed to initialise g_szSysAllocImport");

	eError = OSLockCreate(&g_hSysAllocPidListLock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: OSLockCreate (g_hSysAllocPidListLock) failed (returned %d)",
		         __func__,
		         eError));
	}
	dllist_init(&(g_sSysAllocPidListHead));
#if (USE_RI_LOCK == 1)
	eError = OSLockCreate(&g_hRILock);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: OSLockCreate (g_hRILock) failed (returned %d)",
		         __func__,
		         eError));
	}
#endif
	return eError;
}

void RIDeInitKM(void)
{
#if (USE_RI_LOCK == 1)
	if (g_pPMR2RIListHashTable != NULL && HASH_Count(g_pPMR2RIListHashTable) > 0)
	{
		PVR_DPF((PVR_DBG_WARNING,
		         "%s: called with %d entries remaining - deferring OSLockDestroy()",
		         __func__,
		         HASH_Count(g_pPMR2RIListHashTable)));
		bRIDeInitDeferred = IMG_TRUE;
	}
	else
	{
		OSLockDestroy(g_hRILock);
		OSLockDestroy(g_hSysAllocPidListLock);
	}
#endif
}

/*!
*******************************************************************************

 @Function	RILockAcquireKM

 @Description
            Acquires the RI Lock (which protects the integrity of the RI
            linked lists). Caller will be suspended until lock is acquired.

 @Return	None

******************************************************************************/
void RILockAcquireKM(void)
{
	_RILock();
}

/*!
*******************************************************************************

 @Function	RILockReleaseKM

 @Description
            Releases the RI Lock (which protects the integrity of the RI
            linked lists).

 @Return	None

******************************************************************************/
void RILockReleaseKM(void)
{
	_RIUnlock();
}

/*!
*******************************************************************************

 @Function	RIWritePMREntryWithOwnerKM

 @Description
            Writes a new Resource Information list entry.
            The new entry will be inserted at the head of the list of
            PMR RI entries and assigned the values provided.

 @input     psPMR - Reference (handle) to the PMR to which this reference relates

 @input     ui32Owner - PID of the process which owns the allocation. This
                        may not be the current process (e.g. a request to
                        grow a buffer may happen in the context of a kernel
                        thread, or we may import further resource for a
                        suballocation made from the FW heap which can then
                        also be utilized by other processes)

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIWritePMREntryWithOwnerKM(PMR *psPMR,
                                        IMG_PID ui32Owner)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PMR *pPMRHashKey = psPMR;
	RI_LIST_ENTRY *psRIEntry;
	uintptr_t hashData;

	/* if Hash table has not been created, create it now */
	if (!g_pPMR2RIListHashTable)
	{
		g_pPMR2RIListHashTable = HASH_Create_Extended(_RI_INITIAL_HASH_TABLE_SIZE, sizeof(PMR*), HASH_Func_Default, HASH_Key_Comp_Default);
		g_PID2RISublistHashTable = HASH_Create_Extended(_RI_INITIAL_HASH_TABLE_SIZE, sizeof(IMG_PID), _ProcHashFunc, _ProcHashComp);
	}
	PVR_RETURN_IF_NOMEM(g_pPMR2RIListHashTable);
	PVR_RETURN_IF_NOMEM(g_PID2RISublistHashTable);

	PVR_RETURN_IF_INVALID_PARAM(psPMR);

	/* Acquire RI Lock */
	_RILock();

	/* Look-up psPMR in Hash Table */
	hashData = HASH_Retrieve_Extended (g_pPMR2RIListHashTable, (void *)&pPMRHashKey);
	psRIEntry = (RI_LIST_ENTRY *)hashData;
	if (psRIEntry == NULL)
	{
		PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)PMR_DeviceNode(psPMR);

		/*
		 * If failed to find a matching existing entry, create a new one
		 */
		psRIEntry = (RI_LIST_ENTRY *)OSAllocZMemNoStats(sizeof(RI_LIST_ENTRY));
		if (!psRIEntry)
		{
			/* Error - no memory to allocate for new RI entry */
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_OUT_OF_MEMORY, exit_);
		}
		else
		{
			PMR_FLAGS_T uiPMRFlags = PMR_Flags(psPMR);
			IMG_UINT32 ui32RICount = HASH_Count(g_pPMR2RIListHashTable);

			/*
			 * Add new RI Entry
			 */
			if (ui32RICount == 0)
			{
				/* Initialise PMR entry linked-list head */
				dllist_init(&g_sClientsListHead);
			}
			else if (ui32RICount == IMG_UINT32_MAX)
			{
				PVR_LOG_GOTO_WITH_ERROR("ui32RICount", eError, PVRSRV_ERROR_REFCOUNT_OVERFLOW, exit_);
			}

			dllist_init (&(psRIEntry->sListNode));
			dllist_init (&(psRIEntry->sSubListFirst));
			BIT_SET(psRIEntry->ui16Flags, RI_VALID_FLAG);

			/* Check if this PMR should be accounted for under the
			 * PVR_SYS_ALLOC_PID debugFS entry. This should happen if
			 * we are in the driver init phase, the flags indicate
			 * this is a FW Main allocation (made from FW heap)
			 * or the owner PID is PVR_SYS_ALLOC_PID.
			 * Also record host dev node allocs on the system PID.
			 */
			if (psDeviceNode->eDevState < PVRSRV_DEVICE_STATE_ACTIVE ||
				PVRSRV_CHECK_FW_MAIN(uiPMRFlags) ||
				ui32Owner == PVR_SYS_ALLOC_PID ||
				psDeviceNode == PVRSRVGetPVRSRVData()->psHostMemDeviceNode)
			{
				BIT_SET(psRIEntry->ui16Flags, RI_SYSALLOC_PMR_FLAG);
				psRIEntry->pid = PVR_SYS_ALLOC_PID;
				OSLockAcquire(g_hSysAllocPidListLock);
				/* Add this psRIEntry to the list of entries for PVR_SYS_ALLOC_PID */
				dllist_add_to_tail(&g_sSysAllocPidListHead, (PDLLIST_NODE)&(psRIEntry->sListNode));
				OSLockRelease(g_hSysAllocPidListLock);
				g_ui32SysAllocPMRCount++;
			}
			else
			{
				psRIEntry->pid = ui32Owner;
				dllist_add_to_tail(&g_sClientsListHead, (PDLLIST_NODE)&(psRIEntry->sListNode));
			}
		}

		psRIEntry->pmr_info.psPMR = psPMR;
		SET_DEVICE_ID(psRIEntry, psDeviceNode->sDevId.ui32InternalID);

		/* Create index entry in Hash Table */
		HASH_Insert_Extended (g_pPMR2RIListHashTable, (void *)&pPMRHashKey, (uintptr_t)psRIEntry);

		/* Store phRIHandle in PMR structure, so it can delete the associated RI entry when it destroys the PMR */
		PMRStoreRIHandle(psPMR, psRIEntry);
	}
exit_:
	/* Release RI Lock */
	_RIUnlock();

	return eError;
}

/*!
*******************************************************************************

 @Function	RIWritePMREntryKM

 @Description
            Writes a new Resource Information list entry.
            The new entry will be inserted at the head of the list of
            PMR RI entries and assigned the values provided.

 @input     psPMR - Reference (handle) to the PMR to which this reference relates

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIWritePMREntryKM(PMR *psPMR)
{
	return RIWritePMREntryWithOwnerKM(psPMR,
	                                  OSGetCurrentClientProcessIDKM());
}

/*!
*******************************************************************************

 @Function	RIWriteMEMDESCEntryKM

 @Description
            Writes a new Resource Information sublist entry.
            The new entry will be inserted at the head of the sublist of
            the indicated PMR list entry, and assigned the values provided.

 @input     psConnection - Reference to the Services connection
 @input     psDeviceNode - Reference to the device node
 @input     psPMR - Reference (handle) to the PMR to which this MEMDESC RI entry relates
 @input     ui32TextBSize - Length of string provided in psz8TextB parameter
 @input     psz8TextB - String describing this secondary reference (may be null)
 @input     ui64Offset - Offset from the start of the PMR at which this allocation begins
 @input     ui64Size - Size of this allocation
 @input     uiFlags - Flags indicating if this is a sub-allocation or an import
 @output    phRIHandle - Handle to the created RI entry

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIWriteMEMDESCEntryKM(void* psConnection,
                                   PVRSRV_DEVICE_NODE *psDeviceNode,
                                   PMR *psPMR,
                                   IMG_UINT32 ui32TextBSize,
                                   const IMG_CHAR *psz8TextB,
                                   IMG_UINT64 ui64Offset,
                                   IMG_UINT64 ui64Size,
                                   PVRSRV_MEMALLOCFLAGS_T uiFlags,
                                   RI_HANDLE *phRIHandle)
{
	RI_SUBLIST_ENTRY *psRISubEntry;
	RI_LIST_ENTRY *psRIEntry;
	PMR *pPMRHashKey = psPMR;
	uintptr_t hashData;
	IMG_PID	pid;

	IMG_CHAR szProcName[RI_PROC_BUF_SIZE];

	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	/* Check Hash tables have been created (meaning at least one PMR has been defined) */
	PVR_RETURN_IF_INVALID_PARAM(g_pPMR2RIListHashTable);
	PVR_RETURN_IF_INVALID_PARAM(g_PID2RISublistHashTable);

	PVR_RETURN_IF_INVALID_PARAM(psPMR);
	PVR_RETURN_IF_INVALID_PARAM(phRIHandle);

	/* Only allow request for a firmware context that comes from a direct bridge
	 * (psConnection == NULL). */
	PVR_RETURN_IF_INVALID_PARAM(!(psConnection != NULL && PVRSRV_CHECK_RI_FWKMD_ALLOC(uiFlags)));

	/* Acquire RI Lock */
	_RILock();

	*phRIHandle = NULL;

	/* Look-up psPMR in Hash Table */
	hashData = HASH_Retrieve_Extended (g_pPMR2RIListHashTable, (void *)&pPMRHashKey);
	psRIEntry = (RI_LIST_ENTRY *)hashData;
	if (!psRIEntry)
	{
		/* Release RI Lock */
		_RIUnlock();
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psRISubEntry = (RI_SUBLIST_ENTRY *)OSAllocZMemNoStats(sizeof(RI_SUBLIST_ENTRY));
	if (!psRISubEntry)
	{
		/* Release RI Lock */
		_RIUnlock();
		/* Error - no memory to allocate for new RI sublist entry */
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	else
	{
		/*
		 * Insert new entry in sublist
		 */
		PDLLIST_NODE currentNode = dllist_get_next_node(&(psRIEntry->sSubListFirst));

		/*
		 * Insert new entry before currentNode
		 */
		if (!currentNode)
		{
			currentNode = &(psRIEntry->sSubListFirst);
		}
		dllist_add_to_tail(currentNode, (PDLLIST_NODE)&(psRISubEntry->sListNode));

		psRISubEntry->psRI = psRIEntry;

		/* Increment number of entries in sublist */
		psRIEntry->ui16SubListCount++;
		BIT_SET(psRISubEntry->ui16Flags, RI_VALID_FLAG);
	}

	if (PVRSRV_CHECK_RI_IMPORT(uiFlags))
	{
		BIT_SET(psRISubEntry->ui16Flags, RI_IMPORT_FLAG);
	}
	else
	{
		BIT_UNSET(psRISubEntry->ui16Flags, RI_IMPORT_FLAG);
	}

	if (PVRSRV_CHECK_RI_SUBALLOC(uiFlags))
	{
		BIT_SET(psRISubEntry->ui16Flags, RI_SUBALLOC_FLAG);
	}
	else
	{
		BIT_UNSET(psRISubEntry->ui16Flags, RI_SUBALLOC_FLAG);
	}

	/* If allocation is made during device or driver initialisation,
	 * track the MEMDESC entry under PVR_SYS_ALLOC_PID, otherwise use
	 * the current PID. */
	{
		PVRSRV_DEVICE_NODE *psRIDeviceNode = GET_DEVNODE(psRISubEntry->psRI);
		IMG_INT iRet;

		/* HostMemDevice doesn't update eDevState hence there's no need to test
		 * for it. */
		if (psRIDeviceNode == PVRSRVGetPVRSRVData()->psHostMemDeviceNode)
		{
			/* Imports on HostMemDev should be attributed to the importing
			 * process. This way if an import is not freed before disconnect
			 * it will be outlined in the gpu_mem_area for the given process.
			 * Otherwise attribute the rest of the records to PVR_SYS_ALLOC_PID. */
			if (IS_IMPORT(psRISubEntry))
			{
				psRISubEntry->pid = OSGetCurrentClientProcessIDKM();
				iRet = OSStringSafeCopy(szProcName, OSGetCurrentClientProcessNameKM(),
				                        RI_PROC_BUF_SIZE);
			}
			else
			{
				psRISubEntry->pid = PVR_SYS_ALLOC_PID;
				iRet = OSStringSafeCopy(szProcName, "SysProc", RI_PROC_BUF_SIZE);

				if (psRISubEntry->pid != psRISubEntry->psRI->pid)
				{
					PVR_LOG(("%s(1): current PID = %u, RI entry PID = %u, RI sub-entry PID = %u",
					         __func__, OSGetCurrentClientProcessIDKM(), psRISubEntry->psRI->pid,
					         psRISubEntry->pid));
				}
			}
		}
		else
		{
			/* All allocations done during device initialisation or belonging
			 * to the Firmware should be attributed to PVR_SYS_ALLOC_PID. */
			if (psRIDeviceNode->eDevState < PVRSRV_DEVICE_STATE_ACTIVE ||
			    PVRSRV_CHECK_RI_FWKMD_ALLOC(uiFlags))
			{
				psRISubEntry->pid = PVR_SYS_ALLOC_PID;
				iRet = OSStringSafeCopy(szProcName, "SysProc", RI_PROC_BUF_SIZE);

				if (psRISubEntry->pid != psRISubEntry->psRI->pid)
				{
					PVR_LOG(("%s(2): current PID = %u, RI entry PID = %u, RI sub-entry PID = %u",
					         __func__, OSGetCurrentClientProcessIDKM(), psRISubEntry->psRI->pid,
					         psRISubEntry->pid));
				}
			}
			else
			{
				psRISubEntry->pid = OSGetCurrentClientProcessIDKM();
				iRet = OSStringSafeCopy(szProcName, OSGetCurrentClientProcessNameKM(),
				                        RI_PROC_BUF_SIZE);
			}
		}
		if (iRet < 0)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: process name has been truncated from '%s' to '%s'",
			         __func__, OSGetCurrentClientProcessNameKM(), szProcName));
			psRISubEntry->ui16TextBLength = RI_PROC_BUF_SIZE - 1;
		}
		else
		{
			psRISubEntry->ui16TextBLength = iRet;
		}

		if (psz8TextB == NULL)
		{
			psz8TextB = "";
			ui32TextBSize = 0;
		}
		if (ui32TextBSize > (RI_ANNO_BUF_SIZE))
		{
			PVR_DPF((PVR_DBG_WARNING,
					 "%s: TextBSize too long (%u). Text will be truncated "
					 "to %d characters", __func__,
					 ui32TextBSize, RI_ANNO_BUF_SIZE));
			ui32TextBSize = RI_ANNO_BUF_SIZE;
		}

		/* copy TextB field data plus separator char and terminator */
		psRISubEntry->ui16TextBLength += ui32TextBSize + 2;
		psRISubEntry->pszTextB = OSAllocZMemNoStats(psRISubEntry->ui16TextBLength);
		if (!psRISubEntry->pszTextB)
		{
			OSFreeMemNoStats(psRISubEntry);
			_RIUnlock();
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
		/* TextB is formatted as Annotation+NullTerm+ProcName so we can still
		 * print annotation without proc name.
		 * If any of the strings is too long it will be truncated. */
		(void) OSStringSafeCopy(psRISubEntry->pszTextB, psz8TextB, ui32TextBSize + 1);
		(void) OSStringSafeCopy(psRISubEntry->pszTextB + ui32TextBSize + 1, szProcName,
		                        psRISubEntry->ui16TextBLength - ui32TextBSize - 1);

		psRISubEntry->ui64Offset = ui64Offset;
		psRISubEntry->ui64Size = ui64Size;
		psRISubEntry->psConnection = psConnection;
		dllist_init (&(psRISubEntry->sProcListNode));
	}

	/*
	 *	Now insert this MEMDESC into the proc list
	 */
	/* look-up pid in Hash Table */
	pid = psRISubEntry->pid;
	hashData = HASH_Retrieve_Extended (g_PID2RISublistHashTable, (void *)&pid);
	if (!hashData)
	{
		/*
		 * No allocations for this pid yet
		 */
		if (!HASH_Insert_Extended(g_PID2RISublistHashTable,
		                          (void *) &pid,
		                          (uintptr_t) &psRISubEntry->sProcListNode))
		{
			dllist_remove_node(&psRISubEntry->sListNode);
			psRIEntry->ui16SubListCount--;
			OSFreeMemNoStats(psRISubEntry->pszTextB);
			OSFreeMemNoStats(psRISubEntry);

			_RIUnlock();
			return PVRSRV_ERROR_INSERT_HASH_TABLE_DATA_FAILED;
		}
	}
	else
	{
		/*
		 * Insert allocation into pid allocations linked list
		 */
		PDLLIST_NODE currentNode = (PDLLIST_NODE)hashData;

		/*
		 * Insert new entry
		 */
		dllist_add_to_tail(currentNode, (PDLLIST_NODE)&(psRISubEntry->sProcListNode));
	}
	*phRIHandle = (RI_HANDLE)psRISubEntry;
	/* Release RI Lock */
	_RIUnlock();

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	RIWriteProcListEntryKM

 @Description
            Write a new entry in the process list directly. We have to do this
            because there might be no, multiple or changing PMR handles.

            In the common case we have a PMR that will be added to the PMR list
            and one or several MemDescs that are associated to it in a sub-list.
            Additionally these MemDescs will be inserted in the per-process list.

            There might be special descriptors from e.g. new user APIs that
            are associated with no or multiple PMRs and not just one.
            These can be now added to the per-process list (as RI_SUBLIST_ENTRY)
            directly with this function and won't be listed in the PMR list (RIEntry)
            because there might be no PMR.

            To remove entries from the per-process list, just use
            RIDeleteMEMDESCEntryKM().

 @input     psConnection - Reference to the Services connection
 @input     psDeviceNode - Reference to the device node
 @input     psz8TextB - String describing this secondary reference (may be null)
 @input     ui64Size - Size of this allocation
 @input     ui64DevVAddr - Virtual address of this entry
 @output    phRIHandle - Handle to the created RI entry

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIWriteProcListEntryKM(void* psConnection,
                                    PVRSRV_DEVICE_NODE *psDeviceNode,
                                    IMG_UINT32 ui32TextBSize,
                                    const IMG_CHAR *psz8TextB,
                                    IMG_UINT64 ui64Size,
                                    IMG_UINT64 ui64DevVAddr,
                                    RI_HANDLE *phRIHandle)
{
	uintptr_t hashData = 0;
	IMG_PID		pid;
	RI_SUBLIST_ENTRY *psRISubEntry = NULL;
	IMG_INT iRet;

	if (!g_pPMR2RIListHashTable)
	{
		g_pPMR2RIListHashTable = HASH_Create_Extended(_RI_INITIAL_HASH_TABLE_SIZE, sizeof(PMR*), HASH_Func_Default, HASH_Key_Comp_Default);
		g_PID2RISublistHashTable = HASH_Create_Extended(_RI_INITIAL_HASH_TABLE_SIZE, sizeof(IMG_PID), _ProcHashFunc, _ProcHashComp);

		if (!g_pPMR2RIListHashTable || !g_PID2RISublistHashTable)
		{
			/* Error - no memory to allocate for Hash table(s) */
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
	}

	/* Acquire RI Lock */
	_RILock();

	*phRIHandle = NULL;

	psRISubEntry = (RI_SUBLIST_ENTRY *)OSAllocZMemNoStats(sizeof(RI_SUBLIST_ENTRY));
	if (!psRISubEntry)
	{
		/* Release RI Lock */
		_RIUnlock();
		/* Error - no memory to allocate for new RI sublist entry */
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	BIT_SET(psRISubEntry->ui16Flags, RI_VALID_FLAG);

	psRISubEntry->pid = OSGetCurrentClientProcessIDKM();
	SET_DEVICE_ID(psRISubEntry, psDeviceNode->sDevId.ui32InternalID);

	if (psz8TextB == NULL)
	{
		psz8TextB = "";
		ui32TextBSize = 0;
	}
	if (ui32TextBSize > (RI_ANNO_BUF_SIZE))
	{
		PVR_DPF((PVR_DBG_WARNING,
				 "%s: TextBSize too long (%u). Text will be truncated "
				 "to %d characters", __func__,
				 ui32TextBSize, RI_ANNO_BUF_SIZE));
		ui32TextBSize = RI_ANNO_BUF_SIZE;
	}

	/* copy TextB field data */
	psRISubEntry->ui16TextBLength = ui32TextBSize + RI_PROC_BUF_SIZE;
	psRISubEntry->pszTextB = OSAllocZMemNoStats(psRISubEntry->ui16TextBLength);
	if (!psRISubEntry->pszTextB)
	{
		OSFreeMemNoStats(psRISubEntry);
		_RIUnlock();
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	/* TextB is formatted as Annotation+NullTerm+ProcName so we can still
	 * print annotation without proc name.
	 */
	(void) OSStringSafeCopy(psRISubEntry->pszTextB, psz8TextB, ui32TextBSize + 1);
	iRet = OSStringSafeCopy(psRISubEntry->pszTextB + ui32TextBSize + 1,
	                        OSGetCurrentClientProcessNameKM(),
	                        psRISubEntry->ui16TextBLength - ui32TextBSize - 1);
	if (iRet < 0)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: process name has been truncated from '%s' to '%s'",
		         __func__, OSGetCurrentClientProcessNameKM(),
		         psRISubEntry->pszTextB + ui32TextBSize + 1));
	}
	psRISubEntry->ui64Size = ui64Size;
	psRISubEntry->sVAddr.uiAddr = ui64DevVAddr;
	psRISubEntry->psConnection = psConnection;
	dllist_init (&(psRISubEntry->sProcListNode));

	/*
	 *	Now insert this MEMDESC into the proc list
	 */
	/* look-up pid in Hash Table */
	pid = psRISubEntry->pid;
	hashData = HASH_Retrieve_Extended (g_PID2RISublistHashTable, (void *)&pid);
	if (!hashData)
	{
		/*
		 * No allocations for this pid yet
		 */
		if (!HASH_Insert_Extended(g_PID2RISublistHashTable,
		                          (void *) &pid,
		                          (uintptr_t) &psRISubEntry->sProcListNode))
		{
			OSFreeMemNoStats(psRISubEntry->pszTextB);
			OSFreeMemNoStats(psRISubEntry);
			_RIUnlock();
			return PVRSRV_ERROR_INSERT_HASH_TABLE_DATA_FAILED;
		}
	}
	else
	{
		/*
		 * Insert allocation into pid allocations linked list
		 */
		PDLLIST_NODE currentNode = (PDLLIST_NODE)hashData;

		/*
		 * Insert new entry
		 */
		dllist_add_to_tail(currentNode, (PDLLIST_NODE)&(psRISubEntry->sProcListNode));
	}
	*phRIHandle = (RI_HANDLE)psRISubEntry;
	/* Release RI Lock */
	_RIUnlock();

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	RIUpdateMEMDESCAddrKM

 @Description
            Update a Resource Information entry.

 @input     hRIHandle - Handle of object whose reference info is to be updated
 @input     sVAddr - New address for the RI entry

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIUpdateMEMDESCAddrKM(RI_HANDLE hRIHandle,
								   IMG_DEV_VIRTADDR sVAddr)
{
	RI_SUBLIST_ENTRY *psRISubEntry;

	PVR_RETURN_IF_INVALID_PARAM(hRIHandle);

	psRISubEntry = (RI_SUBLIST_ENTRY *)hRIHandle;
	if (!IS_VALID(psRISubEntry))
	{
		/* Pointer does not point to valid structure */
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Acquire RI lock*/
	_RILock();

	psRISubEntry->sVAddr.uiAddr = sVAddr.uiAddr;

	/* Release RI lock */
	_RIUnlock();

	return PVRSRV_OK;
}

static PVRSRV_ERROR RIDeletePMREntryUnlocked(RI_LIST_ENTRY *psRIEntry)
{
	PMR			*pPMRHashKey;
	uintptr_t hashValue = 0;

	/* Remove the HASH table index entry */
	pPMRHashKey = GET_ADDR(psRIEntry);
	hashValue = HASH_Remove_Extended(g_pPMR2RIListHashTable, (void *)&pPMRHashKey);
	PVR_LOG_RETURN_IF_INVALID_PARAM(hashValue, "RI");

	BIT_UNSET(psRIEntry->ui16Flags, RI_VALID_FLAG);

	/* Remove PMR entry from linked-list of PMR entries */
	dllist_remove_node((PDLLIST_NODE)&(psRIEntry->sListNode));

	if (IS_SYSPMR(psRIEntry))
	{
		g_ui32SysAllocPMRCount--;
	}

	if (IS_RACC(psRIEntry))
	{
		OSFreeMemNoStats(psRIEntry->pmr_info.psPmrInfo);
	}

	/* Now, free the memory used to store the RI entry */
	OSFreeMemNoStats(psRIEntry);
	psRIEntry = NULL;

	/* If the hash table is now empty we can delete the RI hash table */
	if (HASH_Count(g_pPMR2RIListHashTable) == 0)
	{
		HASH_Delete(g_pPMR2RIListHashTable);
		g_pPMR2RIListHashTable = NULL;
	}
	return PVRSRV_OK;
}

static PVRSRV_ERROR _RICreateAndSetPmrInfo(RI_LIST_ENTRY *const psRIEntry)
{
	const IMG_CHAR *const pszAnnotation = PMR_GetAnnotation(psRIEntry->pmr_info.psPMR);
	const IMG_UINT32 uiLength = OSStringNLength(pszAnnotation, DEVMEM_ANNOTATION_MAX_LEN) + 1;

	RI_PMR_INFO *psPmrInfo = OSAllocZMemNoStats(sizeof(*psPmrInfo) + uiLength);
	PVR_LOG_RETURN_IF_NOMEM(psPmrInfo, "OSAllocZMemNoStats");

	psPmrInfo->uiAddr = (uintptr_t) psRIEntry->pmr_info.psPMR;
	psPmrInfo->psHeap = PMR_PhysHeap(psRIEntry->pmr_info.psPMR);
	psPmrInfo->psDeviceNode = (PVRSRV_DEVICE_NODE *) PMR_DeviceNode(psRIEntry->pmr_info.psPMR);
	psPmrInfo->uiLogicalSize = PMR_LogicalSize(psRIEntry->pmr_info.psPMR);
	psPmrInfo->uiPhysicalSize = PMR_PhysicalSize(psRIEntry->pmr_info.psPMR);

	psPmrInfo->pszAnnotation = IMG_OFFSET_ADDR(psPmrInfo, sizeof(*psPmrInfo));
	OSStringSafeCopy(psPmrInfo->pszAnnotation, pszAnnotation, uiLength);

	psRIEntry->pmr_info.psPmrInfo = psPmrInfo;
	BIT_SET(psRIEntry->ui16Flags, RI_HAS_PMR_INFO);

	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	RIDeletePMREntryKM

 @Description
            Delete a Resource Information entry.

 @input     hRIHandle - Handle of object whose reference info is to be deleted

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIDeletePMREntryKM(RI_HANDLE hRIHandle)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	RI_LIST_ENTRY *psRIEntry;

	psRIEntry = (RI_LIST_ENTRY *) hRIHandle;

	if (!RICheckListHandle(hRIHandle))
	{
		/* Pointer does not point to valid structure. */
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, ErrReturn);
	}
	else if (IS_RACC(psRIEntry))
	{
		/* Keep this entry so that it can be inspected as a memory leak in the
		 * gpu_mem_area stats. */
		return PVRSRV_OK;
	}

	if (psRIEntry->ui16SubListCount != 0)
	{
		PVR_DPF((PVR_DBG_WARNING, "%p not deleted. RIEntry(%s) still has %u allocation(s)",
		         psRIEntry, GET_NAME(psRIEntry), psRIEntry->ui16SubListCount));

		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_DEVICEMEM_ALLOCATIONS_REMAIN_IN_HEAP,
		                    ErrCreateAndAssignPmrInfo);
	}

	_RILock();

	eError = RIDeletePMREntryUnlocked(psRIEntry);

	_RIUnlock();

	/* If deInit has been deferred, we can now destroy the RI Lock */
	if (bRIDeInitDeferred && (g_pPMR2RIListHashTable == NULL || HASH_Count(g_pPMR2RIListHashTable) == 0))
	{
		OSLockDestroy(g_hRILock);
	}

	return PVRSRV_OK;

ErrCreateAndAssignPmrInfo:
	{
		PVRSRV_ERROR eError2 = _RICreateAndSetPmrInfo(psRIEntry);
		PVR_RETURN_IF_ERROR(eError2);
	}
ErrReturn:
	return eError;
}

static PVRSRV_ERROR RIDeleteMemdescEntryUnlocked(RI_SUBLIST_ENTRY *psRISubEntry)
{
	RI_LIST_ENTRY *psRIEntry = NULL;
	uintptr_t hashData;
	uintptr_t hashValue = 0;
	IMG_PID pid;

	/* For entries which do have a parent PMR remove the node from the sublist */
	if (psRISubEntry->psRI)
	{
		psRIEntry = (RI_LIST_ENTRY *)psRISubEntry->psRI;

		/* Now, remove entry from the sublist */
		dllist_remove_node(&(psRISubEntry->sListNode));
	}

	BIT_UNSET(psRISubEntry->ui16Flags, RI_VALID_FLAG);

	/* Remove the entry from the proc allocations linked list */
	pid = psRISubEntry->pid;
	/* If this is the only allocation for this pid, just remove it from the hash table */
	if (dllist_get_next_node(&(psRISubEntry->sProcListNode)) == NULL)
	{
		hashValue = HASH_Remove_Extended(g_PID2RISublistHashTable, (void *)&pid);
		PVR_LOG_RETURN_IF_INVALID_PARAM(hashValue, "PID");
		/* Delete the hash table if there are now no entries */
		if (HASH_Count(g_PID2RISublistHashTable) == 0)
		{
			HASH_Delete(g_PID2RISublistHashTable);
			g_PID2RISublistHashTable = NULL;
		}
	}
	else
	{
		hashData = HASH_Retrieve_Extended (g_PID2RISublistHashTable, (void *)&pid);
		if ((PDLLIST_NODE)hashData == &(psRISubEntry->sProcListNode))
		{
			hashValue = HASH_Remove_Extended(g_PID2RISublistHashTable, (void *)&pid);
			PVR_LOG_RETURN_IF_INVALID_PARAM(hashValue, "PID");
			HASH_Insert_Extended (g_PID2RISublistHashTable, (void *)&pid, (uintptr_t)dllist_get_next_node(&(psRISubEntry->sProcListNode)));
		}
	}
	dllist_remove_node(&(psRISubEntry->sProcListNode));

	/* Now, free the memory used to store the sublist entry */
	OSFreeMemNoStats(psRISubEntry->pszTextB);
	OSFreeMemNoStats(psRISubEntry);
	psRISubEntry = NULL;

	/*
	 * Decrement number of entries in sublist if this MemDesc had a parent entry.
	 */
	if (psRIEntry)
	{
		psRIEntry->ui16SubListCount--;
	}
	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	RIDeleteMEMDESCEntryKM

 @Description
            Delete a Resource Information entry.
            Entry can be from RIEntry list or ProcList.

 @input     hRIHandle - Handle of object whose reference info is to be deleted

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIDeleteMEMDESCEntryKM(RI_HANDLE hRIHandle)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	RI_SUBLIST_ENTRY *psRISubEntry;

	psRISubEntry = (RI_SUBLIST_ENTRY *)hRIHandle;
	if (!RICheckSubListHandle(hRIHandle))
	{
		/* Pointer does not point to valid structure */
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	else if (IS_RACC(psRISubEntry))
	{
		return PVRSRV_OK;
	}
	/* Acquire RI lock*/
	_RILock();

	eError = RIDeleteMemdescEntryUnlocked(psRISubEntry);

	/* Release RI lock*/
	_RIUnlock();

	/*
	 * Make the handle NULL once MEMDESC RI entry is deleted
	 */
	hRIHandle = NULL;

	return eError;
}

/*!
*******************************************************************************

 @Function	RIDeleteListKM

 @Description
            Delete all Resource Information entries and free associated
            memory.

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIDeleteListKM(void)
{
	PVRSRV_ERROR eResult = PVRSRV_OK;

	_RILock();

	if (g_pPMR2RIListHashTable)
	{
		eResult = HASH_Iterate(g_pPMR2RIListHashTable, (HASH_pfnCallback)_DeleteAllEntries, NULL);
		if (eResult == PVRSRV_ERROR_RESOURCE_UNAVAILABLE)
		{
			/*
			 * PVRSRV_ERROR_RESOURCE_UNAVAILABLE is used to stop the Hash iterator when
			 * the hash table gets deleted as a result of deleting the final PMR entry,
			 * so this is not a real error condition...
			 */
			eResult = PVRSRV_OK;
		}
	}

	/* After the run through the RIHashTable that holds the PMR entries there might be
	 * still entries left in the per-process hash table because they were added with
	 * RIWriteProcListEntryKM() and have no PMR parent associated.
	 */
	if (g_PID2RISublistHashTable)
	{
		eResult = HASH_Iterate(g_PID2RISublistHashTable, (HASH_pfnCallback) _DeleteAllProcEntries, NULL);
		if (eResult == PVRSRV_ERROR_RESOURCE_UNAVAILABLE)
		{
			/*
			 * PVRSRV_ERROR_RESOURCE_UNAVAILABLE is used to stop the Hash iterator when
			 * the hash table gets deleted as a result of deleting the final PMR entry,
			 * so this is not a real error condition...
			 */
			eResult = PVRSRV_OK;
		}
	}

	_RIUnlock();

	return eResult;
}

/*!
*******************************************************************************

 @Function	RIDumpListKM

 @Description
            Dumps out the contents of the RI List entry for the
            specified PMR, and all MEMDESC allocation entries
            in the associated sub linked list.
            At present, output is directed to Kernel log
            via PVR_DPF.

 @input     psPMR - PMR for which RI entry details are to be output

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIDumpListKM(PMR *psPMR)
{
	PVRSRV_ERROR eError;

	/* Acquire RI lock*/
	_RILock();

	eError = _DumpList(psPMR, 0);

	/* Release RI lock*/
	_RIUnlock();

	return eError;
}

/*!
*******************************************************************************

 @Function	RIGetListEntryKM

 @Description
            Returns pointer to a formatted string with details of the specified
            list entry. If no entry exists (e.g. it may have been deleted
            since the previous call), NULL is returned.

 @input     pid - pid for which RI entry details are to be output
 @input     ppHandle - handle to the entry, if NULL, the first entry will be
                     returned.
 @output    pszEntryString - string to be output for the entry
 @output    hEntry - hEntry will be returned pointing to the next entry
                     (or NULL if there is no next entry)

 @Return	PVRSRV_ERROR

******************************************************************************/
IMG_BOOL RIGetListEntryKM(IMG_PID pid,
						  IMG_HANDLE **ppHandle,
						  IMG_CHAR **ppszEntryString)
{
	RI_SUBLIST_ENTRY  *psRISubEntry = NULL;
	RI_LIST_ENTRY  *psRIEntry = NULL;
	uintptr_t     hashData = 0;
	IMG_PID       hashKey  = pid;

	static IMG_CHAR acStringBuffer[RI_FRMT_SIZE_MAX];

	static IMG_UINT64 ui64TotalMemdescAlloc;
	static IMG_UINT64 ui64TotalImport;
	static IMG_UINT64 ui64TotalPMRAlloc;
	static IMG_UINT64 ui64TotalPMRBacked;
	static enum {
		RI_GET_STATE_MEMDESCS_LIST_START,
		RI_GET_STATE_MEMDESCS_SUMMARY,
		RI_GET_STATE_PMR_LIST,
		RI_GET_STATE_PMR_SUMMARY,
		RI_GET_STATE_END,
		RI_GET_STATE_LAST
	} g_bNextGetState = RI_GET_STATE_MEMDESCS_LIST_START;

	static DLLIST_NODE *psNode;
	static DLLIST_NODE *psSysAllocNode;
	static IMG_CHAR szProcName[RI_PROC_BUF_SIZE];
	static IMG_UINT32 ui32ProcessedSysAllocPMRCount;

	acStringBuffer[0] = '\0';

	switch (g_bNextGetState)
	{
	case RI_GET_STATE_MEMDESCS_LIST_START:
		/* look-up pid in Hash Table, to obtain first entry for pid */
		hashData = HASH_Retrieve_Extended(g_PID2RISublistHashTable, (void *)&hashKey);
		if (hashData)
		{
			if (*ppHandle)
			{
				psRISubEntry = (RI_SUBLIST_ENTRY *)*ppHandle;
				if (!IS_VALID(psRISubEntry))
				{
					psRISubEntry = NULL;
				}
			}
			else
			{
				psRISubEntry = IMG_CONTAINER_OF((PDLLIST_NODE)hashData, RI_SUBLIST_ENTRY, sProcListNode);
				if (!IS_VALID(psRISubEntry))
				{
					psRISubEntry = NULL;
				}
			}
		}

		if (psRISubEntry)
		{
			PDLLIST_NODE psNextProcListNode = dllist_get_next_node(&psRISubEntry->sProcListNode);

			if (IS_IMPORT(psRISubEntry))
			{
				ui64TotalImport += psRISubEntry->ui64Size;
			}
			else
			{
				ui64TotalMemdescAlloc += psRISubEntry->ui64Size;
			}

			_GenerateMEMDESCEntryString(psRISubEntry,
										IMG_TRUE,
										RI_MEMDESC_ENTRY_BUF_SIZE,
										acStringBuffer);

			if (szProcName[0] == '\0')
			{
				OSStringSafeCopy(szProcName, (pid == PVR_SYS_ALLOC_PID) ?
						PVRSRV_MODNAME : GET_PROC(psRISubEntry), RI_PROC_BUF_SIZE);
			}


			*ppszEntryString = acStringBuffer;
			*ppHandle        = (IMG_HANDLE)IMG_CONTAINER_OF(psNextProcListNode, RI_SUBLIST_ENTRY, sProcListNode);

			if (psNextProcListNode == NULL ||
				psNextProcListNode == (PDLLIST_NODE)hashData)
			{
				g_bNextGetState = RI_GET_STATE_MEMDESCS_SUMMARY;
			}
			/* else continue to list MEMDESCs */
		}
		else
		{
			if (ui64TotalMemdescAlloc == 0)
			{
				acStringBuffer[0] = '\0';
				*ppszEntryString = acStringBuffer;
				g_bNextGetState = RI_GET_STATE_MEMDESCS_SUMMARY;
			}
			/* else continue to list MEMDESCs */
		}
		break;

	case RI_GET_STATE_MEMDESCS_SUMMARY:
		OSSNPrintf(acStringBuffer,
		           RI_MEMDESC_SUM_BUF_SIZE,
		           RI_MEMDESC_SUM_FRMT,
		           pid,
		           szProcName,
		           ui64TotalMemdescAlloc,
		           ui64TotalMemdescAlloc >> 10,
		           ui64TotalImport,
		           ui64TotalImport >> 10,
		           (ui64TotalMemdescAlloc + ui64TotalImport),
		           (ui64TotalMemdescAlloc + ui64TotalImport) >> 10);

		*ppszEntryString = acStringBuffer;
		ui64TotalMemdescAlloc = 0;
		ui64TotalImport = 0;
		szProcName[0] = '\0';

		g_bNextGetState = RI_GET_STATE_PMR_LIST;
		break;

	case RI_GET_STATE_PMR_LIST:
		if (pid == PVR_SYS_ALLOC_PID)
		{
			OSLockAcquire(g_hSysAllocPidListLock);
			acStringBuffer[0] = '\0';
			if (!psSysAllocNode)
			{
				psSysAllocNode = &g_sSysAllocPidListHead;
				ui32ProcessedSysAllocPMRCount = 0;
			}
			psSysAllocNode = dllist_get_next_node(psSysAllocNode);

			if (szProcName[0] == '\0')
			{
				OSStringSafeCopy(szProcName, PVRSRV_MODNAME, RI_PROC_BUF_SIZE);
			}
			if (psSysAllocNode != NULL && psSysAllocNode != &g_sSysAllocPidListHead)
			{
				psRIEntry = IMG_CONTAINER_OF((PDLLIST_NODE)psSysAllocNode, RI_LIST_ENTRY, sListNode);
				_GeneratePMREntryString(psRIEntry,
										IMG_TRUE,
										RI_PMR_ENTRY_BUF_SIZE,
										acStringBuffer);
				ui64TotalPMRAlloc += GET_LOGICAL_SIZE(psRIEntry);
				ui64TotalPMRBacked += GET_PHYSICAL_SIZE(psRIEntry);

				ui32ProcessedSysAllocPMRCount++;
				if (ui32ProcessedSysAllocPMRCount > g_ui32SysAllocPMRCount+1)
				{
					g_bNextGetState = RI_GET_STATE_PMR_SUMMARY;
				}
				/* else continue to list PMRs */
			}
			else
			{
				g_bNextGetState = RI_GET_STATE_PMR_SUMMARY;
			}
			*ppszEntryString = (IMG_CHAR *)acStringBuffer;
			OSLockRelease(g_hSysAllocPidListLock);
		}
		else
		{
			IMG_BOOL bPMRToDisplay = IMG_FALSE;

			/* Iterate through the 'touched' PMRs and display details */
			if (!psNode)
			{
				psNode = dllist_get_next_node(&g_sClientsListHead);
			}
			else
			{
				psNode = dllist_get_next_node(psNode);
			}

			while ((psNode != NULL && psNode != &g_sClientsListHead) &&
					!bPMRToDisplay)
			{
				psRIEntry =	IMG_CONTAINER_OF(psNode, RI_LIST_ENTRY, sListNode);
				if (psRIEntry->pid == pid)
				{
					/* This PMR was 'touched', so display details and unflag it*/
					_GeneratePMREntryString(psRIEntry,
											IMG_TRUE,
											RI_PMR_ENTRY_BUF_SIZE,
											acStringBuffer);
					ui64TotalPMRAlloc += GET_LOGICAL_SIZE(psRIEntry);
					ui64TotalPMRBacked += GET_PHYSICAL_SIZE(psRIEntry);

					/* Remember the name of the process for 1 PMR for the summary */
					if (szProcName[0] == '\0')
					{
						psRISubEntry = IMG_CONTAINER_OF(dllist_get_next_node(&(psRIEntry->sSubListFirst)), RI_SUBLIST_ENTRY, sListNode);
						OSStringSafeCopy(szProcName, GET_PROC(psRISubEntry), RI_PROC_BUF_SIZE);
					}
					bPMRToDisplay = IMG_TRUE;
				}
				else
				{
					psNode = dllist_get_next_node(psNode);
				}
			}

			if (psNode == NULL || (psNode == &g_sClientsListHead))
			{
				g_bNextGetState = RI_GET_STATE_PMR_SUMMARY;
			}
			/* else continue listing PMRs */
		}
		break;

	case RI_GET_STATE_PMR_SUMMARY:
		OSSNPrintf(acStringBuffer,
		           RI_PMR_SUM_BUF_SIZE,
		           RI_PMR_SUM_FRMT,
		           pid,
		           szProcName,
		           ui64TotalPMRAlloc,
		           ui64TotalPMRAlloc >> 10,
		           ui64TotalPMRBacked,
		           ui64TotalPMRBacked >> 10);

		*ppszEntryString = acStringBuffer;
		ui64TotalPMRAlloc = 0;
		ui64TotalPMRBacked = 0;
		szProcName[0] = '\0';
		psSysAllocNode = NULL;

		g_bNextGetState = RI_GET_STATE_END;
		break;

	default:
		PVR_DPF((PVR_DBG_ERROR, "%s: Bad %d)",__func__, g_bNextGetState));

		__fallthrough;
	case RI_GET_STATE_END:
		/* Reset state ready for the next gpu_mem_area file to display */
		*ppszEntryString = NULL;
		*ppHandle        = NULL;
		psNode = NULL;
		szProcName[0] = '\0';

		g_bNextGetState = RI_GET_STATE_MEMDESCS_LIST_START;
		return IMG_FALSE;
		break;
	}

	return IMG_TRUE;
}

/* Function used to produce string containing info for MEMDESC RI entries (used for both debugfs and kernel log output) */
static void _GenerateMEMDESCEntryString(RI_SUBLIST_ENTRY *psRISubEntry,
                                        IMG_BOOL bDebugFs,
                                        IMG_UINT16 ui16MaxStrLen,
                                        IMG_CHAR *pszEntryString)
{
	IMG_CHAR szProc[RI_MEMDESC_ENTRY_PROC_BUF_SIZE];
	IMG_CHAR szImport[RI_MEMDESC_ENTRY_IMPORT_BUF_SIZE];
	IMG_CHAR szEntryFormat[RI_MEMDESC_ENTRY_FRMT_SIZE];
	const IMG_CHAR *pszAnnotationText;
	IMG_PID uiRIPid = 0;
	PMR* psRIPMR = NULL;
	IMG_BOOL bSysPMR = IMG_FALSE;
	IMG_BOOL bHostDevice = IS_HOST_DEVICE(psRISubEntry);
	IMG_CHAR szDeviceID[RI_DEV_ID_BUF_SIZE];

	if (psRISubEntry->psRI != NULL)
	{
		uiRIPid = psRISubEntry->psRI->pid;
		psRIPMR = (PMR*)GET_ADDR(psRISubEntry->psRI);
		bSysPMR = IS_SYSPMR(psRISubEntry->psRI);
	}

	OSSNPrintf(szEntryFormat,
			RI_MEMDESC_ENTRY_FRMT_SIZE,
			RI_MEMDESC_ENTRY_FRMT,
			DEVMEM_ANNOTATION_MAX_LEN);

	if (!bDebugFs)
	{
		/* we don't include process ID info for debugfs output */
		OSSNPrintf(szProc,
				RI_MEMDESC_ENTRY_PROC_BUF_SIZE,
				RI_MEMDESC_ENTRY_PROC_FRMT,
				psRISubEntry->pid,
				GET_PROC(psRISubEntry));
	}

	if (!bHostDevice)
	{
		OSSNPrintf(szDeviceID,
				   sizeof(szDeviceID),
				   "%-3d",
				   GET_DEVICE_ID(psRISubEntry));
	}

	if (IS_IMPORT(psRISubEntry) && psRIPMR)
	{
		OSSNPrintf((IMG_CHAR *)&szImport,
		           RI_MEMDESC_ENTRY_IMPORT_BUF_SIZE,
		           RI_MEMDESC_ENTRY_IMPORT_FRMT,
		           uiRIPid);
		/* Set pszAnnotationText to that of the 'parent' PMR RI entry */
		pszAnnotationText = GET_NAME(psRISubEntry->psRI);
	}
	else if (!IS_SUBALLOC(psRISubEntry) && psRIPMR)
	{
		/* Set pszAnnotationText to that of the 'parent' PMR RI entry */
		pszAnnotationText = GET_NAME(psRISubEntry->psRI);
	}
	else
	{
		/* Set pszAnnotationText to that of the MEMDESC RI entry */
		pszAnnotationText = psRISubEntry->pszTextB;
	}

	/* Don't print memdescs if they are local imports
	 * (i.e. imported PMRs allocated by this process)
	 */
	if (bDebugFs &&
		((psRISubEntry->sVAddr.uiAddr + psRISubEntry->ui64Offset) == 0) &&
		(IS_IMPORT(psRISubEntry) && ((psRISubEntry->pid == uiRIPid)
									 || (psRISubEntry->pid == PVR_SYS_ALLOC_PID))))
	{
		/* Don't print this entry */
		pszEntryString[0] = '\0';
	}
	else
	{
		OSSNPrintf(pszEntryString,
				   ui16MaxStrLen,
				   szEntryFormat,
				   (bDebugFs ? "" : "   "),
				   psRISubEntry->pid,
				   (bHostDevice ? "-  " : szDeviceID),
				   (psRISubEntry->sVAddr.uiAddr + psRISubEntry->ui64Offset),
				   pszAnnotationText,
				   (bDebugFs ? "" : (char *)szProc),
				   psRISubEntry->ui64Size,
				   psRIPMR,
				   (IS_IMPORT(psRISubEntry) ? (char *)&szImport : ""),
				   (!IS_IMPORT(psRISubEntry) &&
				    (bSysPMR) &&
				    (psRISubEntry->pid != PVR_SYS_ALLOC_PID)) ? g_szSysAllocImport : "",
				   (IS_RACC(psRISubEntry) ? RI_FREED_BY_DRIVER : ""),
				   (bDebugFs ? '\n' : ' '));
	}
}

/* Function used to produce string containing info for PMR RI entries (used for debugfs and kernel log output) */
static void _GeneratePMREntryString(RI_LIST_ENTRY *psRIEntry,
                                    IMG_BOOL bDebugFs,
                                    IMG_UINT16 ui16MaxStrLen,
                                    IMG_CHAR *pszEntryString)
{
	const IMG_CHAR*   pszAnnotationText;
	const IMG_CHAR*   pszHeapText;

	IMG_CHAR          szEntryFormat[RI_PMR_ENTRY_FRMT_SIZE];
	IMG_BOOL          bHostDevice = IS_HOST_DEVICE(psRIEntry);
	IMG_CHAR          szDeviceID[RI_DEV_ID_BUF_SIZE];

	OSSNPrintf(szEntryFormat,
			RI_PMR_ENTRY_FRMT_SIZE,
			RI_PMR_ENTRY_FRMT,
			(DEVMEM_ANNOTATION_MAX_LEN/2),
			PHYS_HEAP_NAME_SIZE);

	/* Set pszAnnotationText to that PMR RI entry */
	pszAnnotationText = GET_NAME(psRIEntry);

	/* Acquire PhysHeap Name to that PMR RI entry */
	pszHeapText = PhysHeapName(GET_HEAP(psRIEntry));

	if (!bHostDevice)
	{
		OSSNPrintf(szDeviceID,
				   sizeof(szDeviceID),
				   "%-3d",
				   GET_DEVICE_ID(psRIEntry));
	}

	OSSNPrintf(pszEntryString,
	           ui16MaxStrLen,
	           szEntryFormat,
	           (bDebugFs ? "" : "   "),
	           psRIEntry->pid,
	           (bHostDevice ? "-  " : szDeviceID),
	           GET_ADDR(psRIEntry),
	           pszAnnotationText,
	           pszHeapText,
	           GET_LOGICAL_SIZE(psRIEntry),
	           GET_PHYSICAL_SIZE(psRIEntry),
	           (IS_RACC(psRIEntry) ? RI_FREED_BY_DRIVER : ""),
	           (bDebugFs ? '\n' : ' '));
}

/*!
*******************************************************************************

 @Function	_DumpList

 @Description
            Dumps out RI List entries according to parameters passed.

 @input     psPMR - If not NULL, function will output the RI entries for
                   the specified PMR only
 @input     pid - If non-zero, the function will only output MEMDESC RI
                  entries made by the process with ID pid.
                  If zero, all MEMDESC RI entries will be output.

 @Return	PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR _DumpList(PMR *psPMR, IMG_PID pid)
{
	RI_LIST_ENTRY *psRIEntry = NULL;
	RI_SUBLIST_ENTRY *psRISubEntry = NULL;
	IMG_UINT16 ui16SubEntriesParsed = 0;
	uintptr_t hashData = 0;
	IMG_PID hashKey;
	PMR *pPMRHashKey = psPMR;
	IMG_BOOL bDisplayedThisPMR = IMG_FALSE;

	PVR_RETURN_IF_INVALID_PARAM(psPMR);

	if (g_pPMR2RIListHashTable && g_PID2RISublistHashTable)
	{
		if (pid != 0)
		{
			/* look-up pid in Hash Table */
			hashKey = pid;
			hashData = HASH_Retrieve_Extended (g_PID2RISublistHashTable, (void *)&hashKey);
			if (hashData)
			{
				psRISubEntry = IMG_CONTAINER_OF((PDLLIST_NODE)hashData, RI_SUBLIST_ENTRY, sProcListNode);
				if (psRISubEntry)
				{
					psRIEntry = psRISubEntry->psRI;
				}
			}
		}
		else
		{
			/* Look-up psPMR in Hash Table */
			hashData = HASH_Retrieve_Extended (g_pPMR2RIListHashTable, (void *)&pPMRHashKey);
			psRIEntry = (RI_LIST_ENTRY *)hashData;
		}
		if (!psRIEntry)
		{
			/* No entry found in hash table */
			return PVRSRV_ERROR_NOT_FOUND;
		}
		while (psRIEntry)
		{
			bDisplayedThisPMR = IMG_FALSE;
			/* Output details for RI entry */
			if (!pid)
			{
				_RIOutput (("%s <%p> suballocs:%d size:0x%010" IMG_UINT64_FMTSPECx,
				            GET_NAME(psRIEntry),
				            GET_ADDR(psRIEntry),
				            (IMG_UINT)psRIEntry->ui16SubListCount,
				            GET_LOGICAL_SIZE(psRIEntry)));
				bDisplayedThisPMR = IMG_TRUE;
			}
			ui16SubEntriesParsed = 0;
			if (psRIEntry->ui16SubListCount)
			{
#if _DUMP_LINKEDLIST_INFO
				_RIOutput (("RI LIST: {sSubListFirst.psNextNode:0x%p}\n",
				            psRIEntry->sSubListFirst.psNextNode));
#endif /* _DUMP_LINKEDLIST_INFO */
				if (!pid)
				{
					psRISubEntry = IMG_CONTAINER_OF(dllist_get_next_node(&(psRIEntry->sSubListFirst)),
					                                RI_SUBLIST_ENTRY, sListNode);
				}
				/* Traverse RI sublist and output details for each entry */
				while (psRISubEntry)
				{
					if (psRIEntry)
					{
						if ((ui16SubEntriesParsed >= psRIEntry->ui16SubListCount))
						{
							break;
						}
						if (!bDisplayedThisPMR)
						{
							_RIOutput (("%s <%p> suballocs:%d size:0x%010" IMG_UINT64_FMTSPECx,
								    GET_NAME(psRIEntry),
								    GET_ADDR(psRIEntry),
								    (IMG_UINT)psRIEntry->ui16SubListCount,
								    GET_LOGICAL_SIZE(psRIEntry)));
							bDisplayedThisPMR = IMG_TRUE;
						}
					}
#if _DUMP_LINKEDLIST_INFO
					_RIOutput (("RI LIST:    [this subentry:0x%p]\n",psRISubEntry));
					_RIOutput (("RI LIST:     psRI:0x%p\n",psRISubEntry->psRI));
#endif /* _DUMP_LINKEDLIST_INFO */

					{
						IMG_CHAR szEntryString[RI_MEMDESC_ENTRY_BUF_SIZE];

						_GenerateMEMDESCEntryString(psRISubEntry,
						                            IMG_FALSE,
						                            RI_MEMDESC_ENTRY_BUF_SIZE,
						                            szEntryString);
						_RIOutput (("%s",szEntryString));
					}

					if (pid)
					{
						if ((dllist_get_next_node(&(psRISubEntry->sProcListNode)) == NULL) ||
							(dllist_get_next_node(&(psRISubEntry->sProcListNode)) == (PDLLIST_NODE)hashData))
						{
							psRISubEntry = NULL;
						}
						else
						{
							psRISubEntry = IMG_CONTAINER_OF(dllist_get_next_node(&(psRISubEntry->sProcListNode)),
							                                RI_SUBLIST_ENTRY, sProcListNode);
							if (psRISubEntry)
							{
								if (psRIEntry != psRISubEntry->psRI)
								{
									/*
									 * The next MEMDESC in the process linked list is in a different PMR
									 */
									psRIEntry = psRISubEntry->psRI;
									bDisplayedThisPMR = IMG_FALSE;
								}
							}
						}
					}
					else
					{
						ui16SubEntriesParsed++;
						psRISubEntry = IMG_CONTAINER_OF(dllist_get_next_node(&(psRISubEntry->sListNode)),
						                                RI_SUBLIST_ENTRY, sListNode);
					}
				}
			}
			if (!pid && psRIEntry)
			{
				if (ui16SubEntriesParsed != psRIEntry->ui16SubListCount)
				{
					/*
					 * Output error message as sublist does not contain the
					 * number of entries indicated by sublist count
					 */
					_RIOutput (("RI ERROR: RI sublist contains %d entries, not %d entries\n",
					            ui16SubEntriesParsed, psRIEntry->ui16SubListCount));
				}
				else if (psRIEntry->ui16SubListCount && !dllist_get_next_node(&(psRIEntry->sSubListFirst)))
				{
					/*
					 * Output error message as sublist is empty but sublist count
					 * is not zero
					 */
					_RIOutput (("RI ERROR: ui16SubListCount=%d for empty RI sublist\n",
					            psRIEntry->ui16SubListCount));
				}
			}
			psRIEntry = NULL;
		}
	}
	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	RIDumpAllKM

 @Description
            Dumps out the contents of all RI List entries (i.e. for all
            MEMDESC allocations for each PMR).
            At present, output is directed to Kernel log
            via PVR_DPF.

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIDumpAllKM(void)
{
	if (g_pPMR2RIListHashTable)
	{
		return HASH_Iterate(g_pPMR2RIListHashTable, (HASH_pfnCallback)_DumpAllEntries, NULL);
	}
	return PVRSRV_OK;
}

/*!
*******************************************************************************

 @Function	RIDumpProcessKM

 @Description
            Dumps out the contents of all MEMDESC RI List entries (for every
            PMR) which have been allocate by the specified process only.
            At present, output is directed to Kernel log
            via PVR_DPF.

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIDumpProcessKM(IMG_PID pid)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 dummyPMR;

	if (!g_PID2RISublistHashTable)
	{
		return PVRSRV_OK;
	}

	/* Acquire RI lock*/
	_RILock();

	eError = _DumpList((PMR *)&dummyPMR, pid);

	/* Release RI lock*/
	_RIUnlock();

	return eError;
}

/*!
*******************************************************************************

 @Function	_TotalAllocsForProcess

 @Description
            Totals all PMR physical backing for given process.

 @input     pid - ID of process.

 @input     ePhysHeapType - type of Physical Heap for which to total allocs

 @Return	Size of all physical backing for PID's PMRs allocated from the
            specified heap type (in bytes).

******************************************************************************/
static IMG_INT32 _TotalAllocsForProcess(const IMG_PID pid, const PHYS_HEAP_TYPE ePhysHeapType)
{
	IMG_INT32 i32TotalPhysical = 0;

	if (g_pPMR2RIListHashTable && g_PID2RISublistHashTable)
	{
		if (pid == PVR_SYS_ALLOC_PID)
		{
			DLLIST_NODE *psSysAllocNode = NULL;

			OSLockAcquire(g_hSysAllocPidListLock);

			for (psSysAllocNode = dllist_get_next_node(&g_sSysAllocPidListHead);
			     psSysAllocNode != NULL && psSysAllocNode != &g_sSysAllocPidListHead;
			     psSysAllocNode = dllist_get_next_node(psSysAllocNode))
			{
				const RI_LIST_ENTRY *const psRIEntry =
					IMG_CONTAINER_OF((PDLLIST_NODE)psSysAllocNode, RI_LIST_ENTRY, sListNode);

				/* Exclude RACC entries from the stats. This memory should have
				 * been already freed during connection destruction so they should
				 * not affect figure showing memory usage. Also RACC entries existence
				 * is to show unfreed memory in `gpu_mem_area`, not to affect other
				 * process stats. Finally if the RI entry exists only due to sub-entries
				 * referencing it but the underlying PMR has been freed don't include
				 * it in the total stats. */
				if (PhysHeapGetType(GET_HEAP(psRIEntry)) == ePhysHeapType &&
				    !IS_RACC(psRIEntry) && !HAS_PMR_INFO(psRIEntry))
				{
					IMG_UINT64 ui64PhysicalSize = GET_PHYSICAL_SIZE(psRIEntry);

					if (((IMG_UINT64)i32TotalPhysical + ui64PhysicalSize > IMG_INT32_MAX))
					{
						PVR_DPF((PVR_DBG_WARNING, "%s: i32TotalPhysical exceeding size for i32",
						         __func__));
					}
					i32TotalPhysical += (IMG_INT32)(ui64PhysicalSize & 0x00000000ffffffff);
				}
			}

			OSLockRelease(g_hSysAllocPidListLock);
		}
		else
		{
			RI_LIST_ENTRY *psRIEntry = NULL;
			RI_SUBLIST_ENTRY *psInitialRISubEntry = NULL, *psRISubEntry = NULL;
			uintptr_t hashData = 0;

			if (pid != 0)
			{
				/* look-up pid in Hash Table */
				IMG_PID hashKey = pid;
				hashData = HASH_Retrieve_Extended(g_PID2RISublistHashTable, (void *)&hashKey);
				if (hashData)
				{
					psInitialRISubEntry = IMG_CONTAINER_OF((PDLLIST_NODE)hashData, RI_SUBLIST_ENTRY, sProcListNode);
					if (psInitialRISubEntry != NULL)
					{
						psRISubEntry = psInitialRISubEntry;
						psRIEntry = psInitialRISubEntry->psRI;
					}
				}
			}

			while (psRISubEntry != NULL && psRIEntry != NULL)
			{
				DLLIST_NODE *psNextNode;

				if (!IS_IMPORT(psRISubEntry) &&
				    !IS_RACC(psRISubEntry) &&
					!IS_COUNTED_BY_DEBUGFS(psRIEntry) &&
				    !IS_SYSPMR(psRIEntry) &&
				    (PhysHeapGetType(GET_HEAP(psRIEntry)) == ePhysHeapType))
				{
					IMG_UINT64 ui64PhysicalSize = GET_PHYSICAL_SIZE(psRIEntry);

					if (((IMG_UINT64)i32TotalPhysical + ui64PhysicalSize > IMG_INT32_MAX))
					{
						PVR_DPF((PVR_DBG_WARNING, "%s: i32TotalPhysical exceeding size for i32",
						         __func__));
					}

					i32TotalPhysical += (IMG_INT32)(ui64PhysicalSize & 0x00000000ffffffff);
					BIT_SET(psRIEntry->ui16Flags, RI_PMR_PHYS_COUNTED_BY_DEBUGFS_FLAG);
				}

				psNextNode = dllist_get_next_node(&(psRISubEntry->sProcListNode));
				if (psNextNode == NULL || psNextNode == (PDLLIST_NODE)hashData)
				{
					psRISubEntry = NULL;
					psRIEntry = NULL;
				}
				else
				{
					psRISubEntry = IMG_CONTAINER_OF(psNextNode, RI_SUBLIST_ENTRY, sProcListNode);
					if (psRISubEntry)
					{
						psRIEntry = psRISubEntry->psRI;
					}
				}
			}

			psRISubEntry = psInitialRISubEntry;
			if (psRISubEntry)
			{
				psRIEntry = psRISubEntry->psRI;
			}

			while (psRISubEntry != NULL && psRIEntry != NULL)
			{
				const DLLIST_NODE *const psNextNode =
				    dllist_get_next_node(&(psRISubEntry->sProcListNode));

				BIT_UNSET(psRIEntry->ui16Flags, RI_PMR_PHYS_COUNTED_BY_DEBUGFS_FLAG);

				if (psNextNode == NULL || psNextNode == (PDLLIST_NODE)hashData)
				{
					psRISubEntry = NULL;
					psRIEntry = NULL;
				}
				else
				{
					psRISubEntry = IMG_CONTAINER_OF(psNextNode, RI_SUBLIST_ENTRY, sProcListNode);
					if (psRISubEntry)
					{
						psRIEntry = psRISubEntry->psRI;
					}
				}
			}
		}
	}

	return i32TotalPhysical;
}

/*!
*******************************************************************************

 @Function	RITotalAllocProcessKM

 @Description
            Returns the total of allocated GPU memory (backing for PMRs)
            which has been allocated from the specific heap by the specified
            process only.

 @Return	Amount of physical backing allocated (in bytes)

******************************************************************************/
IMG_INT32 RITotalAllocProcessUnlocked(IMG_PID pid, PHYS_HEAP_TYPE ePhysHeapType)
{
	IMG_INT32 i32BackingTotal = 0;

	if (g_PID2RISublistHashTable)
	{
		i32BackingTotal = _TotalAllocsForProcess(pid, ePhysHeapType);
	}
	return i32BackingTotal;
}

#if defined(DEBUG)
/*!
*******************************************************************************

 @Function	_DumpProcessList

 @Description
            Dumps out RI List entries according to parameters passed.

 @input     psPMR - If not NULL, function will output the RI entries for
                   the specified PMR only
 @input     pid - If non-zero, the function will only output MEMDESC RI
                  entries made by the process with ID pid.
                  If zero, all MEMDESC RI entries will be output.

 @Return	PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR _DumpProcessList(PMR *psPMR,
									 IMG_PID pid,
									 IMG_UINT64 ui64Offset,
									 IMG_DEV_VIRTADDR *psDevVAddr)
{
	RI_LIST_ENTRY *psRIEntry = NULL;
	RI_SUBLIST_ENTRY *psRISubEntry = NULL;
	IMG_UINT16 ui16SubEntriesParsed = 0;
	uintptr_t hashData = 0;
	PMR *pPMRHashKey = psPMR;

	psDevVAddr->uiAddr = 0;

	PVR_RETURN_IF_INVALID_PARAM(psPMR);

	if (g_pPMR2RIListHashTable && g_PID2RISublistHashTable)
	{
		PVR_ASSERT(psPMR && pid);

		/* Look-up psPMR in Hash Table */
		hashData = HASH_Retrieve_Extended (g_pPMR2RIListHashTable, (void *)&pPMRHashKey);
		psRIEntry = (RI_LIST_ENTRY *)hashData;

		if (!psRIEntry)
		{
			/* No entry found in hash table */
			return PVRSRV_ERROR_NOT_FOUND;
		}

		if (psRIEntry->ui16SubListCount)
		{
			psRISubEntry = IMG_CONTAINER_OF(dllist_get_next_node(&(psRIEntry->sSubListFirst)),
											RI_SUBLIST_ENTRY, sListNode);

			/* Traverse RI sublist and output details for each entry */
			while (psRISubEntry && (ui16SubEntriesParsed < psRIEntry->ui16SubListCount))
			{
				if (pid == psRISubEntry->pid)
				{
					IMG_UINT64 ui64StartOffset = psRISubEntry->ui64Offset;
					IMG_UINT64 ui64EndOffset = psRISubEntry->ui64Offset + psRISubEntry->ui64Size;

					if (ui64Offset >= ui64StartOffset && ui64Offset < ui64EndOffset)
					{
						psDevVAddr->uiAddr = psRISubEntry->sVAddr.uiAddr;
						return PVRSRV_OK;
					}
				}

				ui16SubEntriesParsed++;
				psRISubEntry = IMG_CONTAINER_OF(dllist_get_next_node(&(psRISubEntry->sListNode)),
												RI_SUBLIST_ENTRY, sListNode);
			}
		}
	}

	return PVRSRV_ERROR_INVALID_PARAMS;
}

/*!
*******************************************************************************

 @Function	RIDumpProcessListKM

 @Description
            Dumps out selected contents of all MEMDESC RI List entries (for a
            PMR) which have been allocate by the specified process only.

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR RIDumpProcessListKM(PMR *psPMR,
								 IMG_PID pid,
								 IMG_UINT64 ui64Offset,
								 IMG_DEV_VIRTADDR *psDevVAddr)
{
	PVRSRV_ERROR eError;

	if (!g_PID2RISublistHashTable)
	{
		return PVRSRV_OK;
	}

	/* Acquire RI lock*/
	_RILock();

	eError = _DumpProcessList(psPMR,
							  pid,
							  ui64Offset,
							  psDevVAddr);

	/* Release RI lock*/
	_RIUnlock();

	return eError;
}
#endif

static PVRSRV_ERROR _MarkRACCEntries(uintptr_t k, uintptr_t v, void *psConnection)
{
	DLLIST_NODE *psListHead = (DLLIST_NODE *) v;
	DLLIST_NODE *psNextNode = psListHead;

	PVR_UNREFERENCED_PARAMETER(k);

	do
	{
		RI_SUBLIST_ENTRY *psRISubEntry = IMG_CONTAINER_OF(psNextNode, RI_SUBLIST_ENTRY,
		                                                  sProcListNode);

		if (psRISubEntry->psConnection == psConnection)
		{
			RI_LIST_ENTRY *psRIEntry = psRISubEntry->psRI;

			BIT_SET(psRISubEntry->ui16Flags, RI_RACC_FLAG);

			/* RI sub-entry may not have an RI entry associated with it. If that's the
			 * case just skip processing it. */
			if (psRIEntry != NULL)
			{
				if (!IS_RACC(psRIEntry) &&
				    !HAS_PMR_INFO(psRIEntry) &&
				    /* Mark as RACC only if the entry doesn't belong to the system process.
				     * System process allocations are alive for the whole driver lifetime
					 * hence they will always exist at a connection closed for every process
					 * that references them. */
				    psRIEntry->pid != PVR_SYS_ALLOC_PID)
				{
					PVRSRV_ERROR eError;

					eError = _RICreateAndSetPmrInfo(psRIEntry);
					PVR_LOG_RETURN_IF_ERROR(eError, "_RICreateAndSetPmrInfo");

					BIT_SET(psRIEntry->ui16Flags, RI_RACC_FLAG);
				}
				else
				{
					PVR_DPF((PVR_DBG_MESSAGE, "RIEntry(%s) is already RACC", GET_NAME(psRIEntry)));
				}
			}
		}

		psNextNode = dllist_get_next_node(psNextNode);
	} while (psNextNode != NULL && psNextNode != psListHead);

	return PVRSRV_OK;
}

void RIConnectionClosed(void* psConnection)
{
	PVRSRV_ERROR eError;
	if (g_PID2RISublistHashTable)
	{
		_RILock();
		eError = HASH_Iterate(g_PID2RISublistHashTable, (HASH_pfnCallback)_MarkRACCEntries, psConnection);
		PVR_LOG_IF_FALSE(eError == PVRSRV_OK, "_MarkRACCEntries");
		_RIUnlock();
	}
}

static PVRSRV_ERROR DeleteRACCEntry(RI_SUBLIST_ENTRY *psRISubEntry)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	RI_LIST_ENTRY *psRIEntry = psRISubEntry->psRI;

	PVR_LOG_IF_FALSE_VA(PVR_DBG_WARNING, IS_RACC(psRISubEntry), "Non-RACC entry(%s)",
	                    psRISubEntry->pszTextB);

	eError = RIDeleteMemdescEntryUnlocked(psRISubEntry);
	PVR_LOG_IF_ERROR(eError, "RIDeleteMemdescEntryUnlocked");

	/* RI sub-entry may not have an RI entry associated with it. If that's the
	 * case just skip processing it. */
	if (psRIEntry != NULL)
	{
		if (psRIEntry->ui16SubListCount)
		{
			PVR_DPF((PVR_DBG_MESSAGE, "%p: More than 1 sublist present! (%s)",
			         psRIEntry, __func__));
		}
		else
		{
			eError = RIDeletePMREntryUnlocked(psRIEntry);
			PVR_LOG_IF_ERROR(eError, "RIDeletePMREntryUnlocked");
		}
	}

	return eError;
}

PVRSRV_ERROR RIDeleteEntriesForPID(IMG_PID pid)
{
	DLLIST_NODE *psListHead;
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_PID hashKey  = pid;

	_RILock();

	PVR_GOTO_IF_FALSE(g_PID2RISublistHashTable != NULL, ErrHashError);

	psListHead = (DLLIST_NODE *) HASH_Retrieve_Extended(g_PID2RISublistHashTable,
	                                                    (void *) &hashKey);

	while (psListHead != NULL)
	{
		DLLIST_NODE *psNextNode = dllist_get_next_node(psListHead);
		RI_SUBLIST_ENTRY *psRISubEntry = IMG_CONTAINER_OF(psListHead, RI_SUBLIST_ENTRY,
		                                                  sProcListNode);

		eError = DeleteRACCEntry(psRISubEntry);
		PVR_LOG_GOTO_IF_ERROR(eError, "DeleteRACCEntry", ErrUnlockAndReturn);

		psListHead = psNextNode;
	}

ErrHashError:
    eError = PVRSRV_ERROR_UNABLE_TO_RETRIEVE_HASH_VALUE;

ErrUnlockAndReturn:
	_RIUnlock();

	return eError;
}

static PVRSRV_ERROR _DumpAllEntries (uintptr_t k, uintptr_t v, void* pvPriv)
{
	RI_LIST_ENTRY *psRIEntry = (RI_LIST_ENTRY *)v;

	PVR_UNREFERENCED_PARAMETER (k);
	PVR_UNREFERENCED_PARAMETER (pvPriv);

	return RIDumpListKM(GET_ADDR(psRIEntry));
}

static PVRSRV_ERROR _DeleteAllEntries (uintptr_t k, uintptr_t v, void* pvPriv)
{
	RI_LIST_ENTRY *psRIEntry = (RI_LIST_ENTRY *)v;
	RI_SUBLIST_ENTRY *psRISubEntry;
	PVRSRV_ERROR eResult = PVRSRV_OK;

	PVR_UNREFERENCED_PARAMETER (k);
	PVR_UNREFERENCED_PARAMETER (pvPriv);

	while ((eResult == PVRSRV_OK) && (psRIEntry->ui16SubListCount > 0))
	{
		psRISubEntry = IMG_CONTAINER_OF(dllist_get_next_node(&(psRIEntry->sSubListFirst)), RI_SUBLIST_ENTRY, sListNode);
		eResult = RIDeleteMEMDESCEntryKM((RI_HANDLE)psRISubEntry);
	}
	if (eResult == PVRSRV_OK)
	{
		eResult = RIDeletePMREntryKM((RI_HANDLE)psRIEntry);
		/*
		 * If we've deleted the Hash table, return
		 * an error to stop the iterator...
		 */
		if (!g_pPMR2RIListHashTable)
		{
			eResult = PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
		}
	}
	return eResult;
}

static PVRSRV_ERROR _DeleteAllProcEntries (uintptr_t k, uintptr_t v, void* pvPriv)
{
	RI_SUBLIST_ENTRY *psRISubEntry = (RI_SUBLIST_ENTRY *)v;
	PVRSRV_ERROR eResult;

	PVR_UNREFERENCED_PARAMETER (k);
	PVR_UNREFERENCED_PARAMETER (pvPriv);

	eResult = RIDeleteMEMDESCEntryKM((RI_HANDLE) psRISubEntry);
	if (eResult == PVRSRV_OK && !g_PID2RISublistHashTable)
	{
		/*
		 * If we've deleted the Hash table, return
		 * an error to stop the iterator...
		 */
		eResult = PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
	}

	return eResult;
}

#endif /* if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO) */

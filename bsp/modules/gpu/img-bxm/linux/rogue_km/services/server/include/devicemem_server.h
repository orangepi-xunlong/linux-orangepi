/*************************************************************************/ /*!
@File
@Title          Device Memory Management
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Server side component for device memory management
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

#ifndef DEVICEMEM_SERVER_H
#define DEVICEMEM_SERVER_H

#include "device.h" /* For device node */
#include "img_types.h"
#include "img_defs.h"
#include "pvr_debug.h"
#include "pvrsrv_error.h"

#include "connection_server.h"
#include "pmr.h"

typedef struct _DEVMEMINT_CTX_ DEVMEMINT_CTX;
typedef struct _DEVMEMINT_CTX_EXPORT_ DEVMEMINT_CTX_EXPORT;
typedef struct _DEVMEMINT_HEAP_ DEVMEMINT_HEAP;

typedef struct _DEVMEMINT_RESERVATION_ DEVMEMINT_RESERVATION;

typedef struct _DEVMEMINT_MAPPING_ DEVMEMINT_MAPPING;
typedef struct _DEVMEMXINT_RESERVATION_ DEVMEMXINT_RESERVATION;
typedef struct _DEVMEMINT_PF_NOTIFY_ DEVMEMINT_PF_NOTIFY;

/*
 * DevmemServerGetImportHandle()
 *
 * For given exportable memory descriptor returns PMR handle
 *
 */
PVRSRV_ERROR
DevmemServerGetImportHandle(DEVMEM_MEMDESC *psMemDesc,
                            IMG_HANDLE *phImport);

/*
 * DevmemServerGetHeapHandle()
 *
 * For given reservation returns the Heap handle
 *
 */
PVRSRV_ERROR
DevmemServerGetHeapHandle(DEVMEMINT_RESERVATION *psReservation,
                          IMG_HANDLE *phHeap);

/*
 * DevmemServerGetContext()
 *
 * For given heap returns the context.
 *
 */
PVRSRV_ERROR
DevmemServerGetContext(DEVMEMINT_HEAP *psDevmemHeap,
                       DEVMEMINT_CTX **ppsDevmemCtxPtr);

/*
 * DevmemServerGetPrivData()
 *
 * For given context returns the private data handle.
 *
 */
PVRSRV_ERROR
DevmemServerGetPrivData(DEVMEMINT_CTX *psDevmemCtx,
                        IMG_HANDLE *phPrivData);

/*
 * DevmemIntCtxCreate()
 *
 * Create a Server-side Device Memory Context. This is usually the counterpart
 * of the client side memory context, and indeed is usually created at the
 * same time.
 *
 * You must have one of these before creating any heaps.
 *
 * All heaps must have been destroyed before calling
 * DevmemIntCtxDestroy()
 *
 * If you call DevmemIntCtxCreate() (and it succeeds) you are promising to
 * later call DevmemIntCtxDestroy()
 *
 * Note that this call will cause the device MMU code to do some work for
 * creating the device memory context, but it does not guarantee that a page
 * catalogue will have been created, as this may be deferred until the first
 * allocation.
 *
 * Caller to provide storage for a pointer to the DEVMEM_CTX object that will
 * be created by this call.
 */
PVRSRV_ERROR
DevmemIntCtxCreate(CONNECTION_DATA *psConnection,
                   PVRSRV_DEVICE_NODE *psDeviceNode,
                   /* devnode / perproc etc */
                   IMG_BOOL bKernelMemoryCtx,
                   DEVMEMINT_CTX **ppsDevmemCtxPtr,
                   IMG_HANDLE *hPrivData,
                   IMG_UINT32 *pui32CPUCacheLineSize);
/*
 * DevmemIntCtxDestroy()
 *
 * Undoes a prior DevmemIntCtxCreate or DevmemIntCtxImport.
 */
PVRSRV_ERROR
DevmemIntCtxDestroy(DEVMEMINT_CTX *psDevmemCtx);

/*
 * DevmemIntHeapCreate()
 *
 * Creates a new heap in this device memory context.  This will cause a call
 * into the MMU code to allocate various data structures for managing this
 * heap. It will not necessarily cause any page tables to be set up, as this
 * can be deferred until first allocation. (i.e. we shouldn't care - it's up
 * to the MMU code)
 *
 * Note that the data page size must be specified (as log 2). The data page
 * size as specified here will be communicated to the mmu module, and thus may
 * determine the page size configured in page directory entries for subsequent
 * allocations from this heap. It is essential that the page size here is less
 * than or equal to the "minimum contiguity guarantee" of any PMR that you
 * subsequently attempt to map to this heap.
 *
 * If you call DevmemIntHeapCreate() (and the call succeeds) you are promising
 * that you shall subsequently call DevmemIntHeapDestroy()
 *
 * Caller to provide storage for a pointer to the DEVMEM_HEAP object that will
 * be created by this call.
 */
PVRSRV_ERROR
DevmemIntHeapCreate(DEVMEMINT_CTX *psDevmemCtx,
                    IMG_UINT32 uiHeapConfigIndex,
                    IMG_UINT32 uiHeapIndex,
                    DEVMEMINT_HEAP **ppsDevmemHeapPtr);
/*
 * DevmemIntHeapDestroy()
 *
 * Destroys a heap previously created with DevmemIntHeapCreate()
 *
 * All allocations from his heap must have been freed before this
 * call.
 */
PVRSRV_ERROR
DevmemIntHeapDestroy(DEVMEMINT_HEAP *psDevmemHeap);

/* DevmemIntHeapGetBaseAddr()
 *
 * Get heap base address pre carveouts.
 */
IMG_DEV_VIRTADDR
DevmemIntHeapGetBaseAddr(DEVMEMINT_HEAP *psDevmemHeap);

/*************************************************************************/ /*!
 * @Function    DevmemIntReserveRange()
 * @Description Reserves a number of virtual addresses starting sReservationVAddr
 *              and continuing until sReservationVAddr + uiVirtualSize - 1.
 *
 *              If you call DevmemIntReserveRange() (and the call succeeds)
 *              then you are promising that you shall later call DevmemIntUnreserveRange()
 *
 * @Input       psDevmemHeap        The virtual heap the DevVAddr is within.
 * @Input       sReservationVAddr   The first virtual address of the range.
 * @Input       uiVirtualSize       The number of bytes in the virtual range.
 * @Input       uiFlags             Mem alloc flags
 * @Output      ppsReservationPtr   A pointer to the created reservation.
 *
 * @Return      PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemIntReserveRange(DEVMEMINT_HEAP *psDevmemHeap,
                      IMG_DEV_VIRTADDR sReservationVAddr,
                      IMG_DEVMEM_SIZE_T uiVirtualSize,
                      PVRSRV_MEMALLOCFLAGS_T uiFlags,
                      DEVMEMINT_RESERVATION **ppsReservationPtr);

/*************************************************************************/ /*!
 * @Function    DevmemIntUnreserveRange()
 * @Description Unreserves the specified virtual range. In the case that the
 *              virtual range has not been unmapped, it will be unmapped.
 *              If any references are held on the reservation PVRSRV_ERROR_RETRY
 *              will be returned.
 *
 * @Input       psDevmemReservation The reservation to unreserve
 *
 * @Return      PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemIntUnreserveRange(DEVMEMINT_RESERVATION *psDevmemReservation);

/*************************************************************************/ /*!
 * @Function    DevmemIntMapPMR
 *
 * @Description Maps the given PMR to the virtual range previously reserved with
 *              DevmemIntReserveRange(). When calling this function, the reservation
 *              must be valid, and not mapped. Additionally, the PMRs logical
 *              size and the reservations virtual size must be equal.
 *
 *              If appropriate, the PMR must have had its physical backing
 *              committed, as this call will call into the MMU code to set
 *              up the page tables for this allocation, which shall in turn
 *              request the physical addresses from the PMR. Alternatively,
 *              the PMR implementation can choose to do so off the back of
 *              the "lock" callback, which it will receive as a result
 *              (indirectly) of this call.
 *
 *              If you call DevmemIntMapPMR() (and the call succeeds) then you
 *              are promising that you shall later call DevmemIntUnmapPMR()
 *
 * @Input       psReservation    The reservation the PMR will be mapped into.
 * @Input       psPMR            The PMR to be mapped.
 *
 * @Return      PVRSRV_ERROR failure code
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemIntMapPMR(DEVMEMINT_RESERVATION *psReservation, PMR *psPMR);

/*************************************************************************/ /*!
 * @Function    DevmemIntUnmapPMR()
 *
 * @Description Unmaps a previously mapped virtual range.
 *
 * @Input       psReservation   The virtual range to unmap.
 *
 * @Return      PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemIntUnmapPMR(DEVMEMINT_RESERVATION *psReservation);


/*************************************************************************/ /*!
 * @Function    DevmemIntReserveRangeAndMapPMR()
 *
 * @Description Reserve (with DevmemIntReserveRange), and map a virtual range
 *              to a PMR (with DevmemIntMapPMR).
 *
 * @Input       psDevmemHeap        The virtual heap DevVAddr is within.
 * @Input       sReservationVAddr   The first virtual address of the range.
 * @Input       uiVirtualSize       The number of bytes in the virtual range.
 * @Input       psPMR               The PMR to be mapped.
 * @Input       uiFlags             Mem alloc flags
 * @Output      ppsReservation      A pointer to the created reservation.
 *
 * @Return      PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemIntReserveRangeAndMapPMR(DEVMEMINT_HEAP *psDevmemHeap,
                               IMG_DEV_VIRTADDR sReservationVAddr,
                               IMG_DEVMEM_SIZE_T uiVirtualSize,
                               PMR *psPMR,
                               PVRSRV_MEMALLOCFLAGS_T uiFlags,
                               DEVMEMINT_RESERVATION **ppsReservation);

/*************************************************************************/ /*!
 * @Function       DevmemIntChangeSparse
 * @Description    Changes the sparse allocations of a PMR by allocating and freeing
 *                 pages and changing their corresponding GPU mapping.
 *
 *                 Prior to calling this function DevmemIntMapPMR
 *                 or DevmemIntReserveRangeAndMapPMR must be used.
 *
 * @Input          psReservation         The reservation that the PMR is mapped to.
 * @Input          ui32AllocPageCount    Number of pages to allocate
 * @Input          pai32AllocIndices     The logical PMR indices where pages will
 *                                       be allocated. May be NULL.
 * @Input          ui32FreePageCount     Number of pages to free
 * @Input          pai32FreeIndices      The logical PMR indices where pages will
 *                                       be freed. May be NULL.
 * @Input          uiSparseFlags         Flags passed in to determine which kind
 *                                       of sparse change the user wanted.
 *                                       See devicemem_typedefs.h for details.
 * @Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemIntChangeSparse(IMG_UINT32 ui32AllocPageCount,
                      IMG_UINT32 *pai32AllocIndices,
                      IMG_UINT32 ui32FreePageCount,
                      IMG_UINT32 *pai32FreeIndices,
                      SPARSE_MEM_RESIZE_FLAGS uiSparseFlags,
                      DEVMEMINT_RESERVATION *psReservation);

PVRSRV_ERROR
DevmemIntGetReservationData(DEVMEMINT_RESERVATION* psReservation, PMR** ppsPMR, IMG_DEV_VIRTADDR* psDevVAddr);

/*************************************************************************/ /*!
 * @Function    DevmemXIntReserveRange()
 * @Description Indicates that the specified range should be reserved from the
 *              given heap.
 *
 *              In turn causes the page tables to be allocated to cover the
 *              specified range.
 *
 *              If you call DevmemIntReserveRange() (and the call succeeds)
 *              then you are promising that you shall later call
 *              DevmemIntUnreserveRange().
 *
 * @Input       psDevmemHeap        Pointer to the heap the reservation is made
 *                                  on
 * @Input       sReservationVAddr   Virtual address of the reservation
 * @Input       uiVirtualSize       Size of the reservation (in bytes)
 * @Input       ppsRsrv             Return pointer to the reservation object
 *
 * @Return      PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemXIntReserveRange(DEVMEMINT_HEAP *psDevmemHeap,
                       IMG_DEV_VIRTADDR sReservationVAddr,
                       IMG_DEVMEM_SIZE_T uiVirtualSize,
                       DEVMEMXINT_RESERVATION **ppsRsrv);

/*************************************************************************/ /*!
 * @Function    DevmemXIntUnreserveRange()
 * @Description Undoes the state change caused by DevmemXIntReserveRage()
 *
 * @Input       psRsrv             Reservation handle for the range
 *
 * @Return      PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemXIntUnreserveRange(DEVMEMXINT_RESERVATION *psRsrv);

/*************************************************************************/ /*!
@Function       DevmemIntReservationAcquire
@Description    Acquire a reference to the provided device memory reservation.
@Return         IMG_TRUE if referenced and IMG_FALSE in case of error
*/ /**************************************************************************/
IMG_BOOL
DevmemIntReservationAcquire(DEVMEMINT_RESERVATION *psDevmemReservation);

/*************************************************************************/ /*!
@Function       DevmemIntReservationRelease
@Description    Release the reference to the provided device memory reservation.
            If this is the last reference which was taken then the
                reservation will be freed.
@Return         None.
*/ /**************************************************************************/
void
DevmemIntReservationRelease(DEVMEMINT_RESERVATION *psDevmemReservation);

/*************************************************************************/ /*!
 * @Function    DevmemXIntMapPages()
 * @Description Maps an arbitrary amount of pages from a PMR to a reserved range
 *              and takes references on the PMR.
 *
 * @Input       psRsrv             Reservation handle for the range
 * @Input       psPMR              PMR that is mapped
 * @Input       uiPageCount        Number of consecutive pages that are
 *                                 mapped
 * @Input       uiPhysPageOffset   Logical offset in the PMR (measured in pages)
 * @Input       uiFlags            Mapping flags
 * @Input       uiVirtPageOffset   Offset from the reservation base to start the
 *                                 mapping from (measured in pages)
 *
 * @Return      PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemXIntMapPages(DEVMEMXINT_RESERVATION *psRsrv,
                   PMR *psPMR,
                   IMG_UINT32 uiPageCount,
                   IMG_UINT32 uiPhysPageOffset,
                   PVRSRV_MEMALLOCFLAGS_T uiFlags,
                   IMG_UINT32 uiVirtPageOffset);

/*************************************************************************/ /*!
 * @Function    DevmemXIntUnmapPages()
 * @Description Unmaps an arbitrary amount of pages from a reserved range and
 *              releases references on associated PMRs.
 *
 * @Input       psRsrv             Reservation handle for the range
 * @Input       uiVirtPageOffset   Offset from the reservation base to start the
 *                                 mapping from (measured in pages)
 * @Input       uiPageCount        Number of consecutive pages that are
 *                                 unmapped
 *
 * @Return      PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemXIntUnmapPages(DEVMEMXINT_RESERVATION *psRsrv,
                     IMG_UINT32 uiVirtPageOffset,
                     IMG_UINT32 uiPageCount);

/*************************************************************************/ /*!
 * @Function    DevmemXIntMapVRangeToBackingPage()
 * @Description Maps a kernel internal backing page to a reserved range.
 *
 * @Input       psRsrv             Reservation handle for the range
 * @Input       uiPageCount        Number of consecutive pages that are
 *                                 mapped
 * @Input       uiFlags            Mapping flags
 * @Input       uiVirtPageOffset   Offset from the reservation base to start the
 *                                 mapping from (measured in pages)
 *
 * @Return      PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemXIntMapVRangeToBackingPage(DEVMEMXINT_RESERVATION *psRsrv,
                                 IMG_UINT32 uiPageCount,
                                 PVRSRV_MEMALLOCFLAGS_T uiFlags,
                                 IMG_UINT32 uiVirtPageOffset);

/*
 * DevmemIntInvalidateFBSCTable()
 *
 * Invalidate selected FBSC table indices.
 *
 */
PVRSRV_ERROR
DevmemIntInvalidateFBSCTable(DEVMEMINT_CTX *psDevmemCtx,
                             IMG_UINT64 ui64FBSCEntryMask);

PVRSRV_ERROR
DevmemIntIsVDevAddrValid(CONNECTION_DATA * psConnection,
                         PVRSRV_DEVICE_NODE *psDevNode,
                         DEVMEMINT_CTX *psDevMemContext,
                         IMG_DEV_VIRTADDR sDevAddr);

PVRSRV_ERROR
DevmemIntGetFaultAddress(CONNECTION_DATA * psConnection,
                         PVRSRV_DEVICE_NODE *psDevNode,
                         DEVMEMINT_CTX *psDevMemContext,
                         IMG_DEV_VIRTADDR *psFaultAddress);

/*************************************************************************/ /*!
@Function       DevmemIntRegisterPFNotifyKM
@Description    Registers a PID to be notified when a page fault occurs on a
                specific device memory context.
@Input          psDevmemCtx    The context to be notified about.
@Input          bRegister      If true, register. If false, de-register.
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR
DevmemIntRegisterPFNotifyKM(DEVMEMINT_CTX *psDevmemCtx,
                            IMG_BOOL      bRegister);

/*************************************************************************/ /*!
@Function       DevmemIntPFNotify
@Description    Notifies any processes that have registered themselves to be
                notified when a page fault happens on a specific device memory
                context.
@Input          *psDevNode           The device node.
@Input          ui64FaultedPCAddress The page catalogue address that faulted.
@Input          sFaultAddress        The address that triggered the fault.
@Return         PVRSRV_ERROR
*/ /**************************************************************************/
PVRSRV_ERROR DevmemIntPFNotify(PVRSRV_DEVICE_NODE *psDevNode,
                               IMG_UINT64         ui64FaultedPCAddress,
                               IMG_DEV_VIRTADDR   sFaultAddress);

#if defined(PDUMP)
PVRSRV_ERROR
DevmemIntPDumpGetValidRegions(CONNECTION_DATA *psConnection,
                              PVRSRV_DEVICE_NODE *psDeviceNode,
                              DEVMEMINT_CTX *psDevmemCtx,
                              IMG_DEV_VIRTADDR sDevAddrStart,
                              IMG_DEVMEM_SIZE_T uiSize,
                              DLLIST_NODE *psValidRegionsList);

void
DevmemIntPDumpFreeValidRegions(DLLIST_NODE *psValidRegionsList);

PVRSRV_ERROR
DevmemIntPDumpSaveFromRegionListToFileVirtual(CONNECTION_DATA * psConnection,
                                              PVRSRV_DEVICE_NODE *psDeviceNode,
                                              DEVMEMINT_CTX *psDevmemCtx,
                                              DLLIST_NODE *psDevAddrRegions,
                                              const IMG_CHAR *pszFilename,
                                              IMG_UINT32 ui32FileOffset,
                                              IMG_UINT32 ui32PDumpFlags);

/*
 * DevmemIntPDumpSaveToFileVirtual()
 *
 * Writes out PDump "SAB" commands with the data found in memory at
 * the given virtual address.
 */
PVRSRV_ERROR
DevmemIntPDumpSaveToFileVirtual(CONNECTION_DATA * psConnection,
                                PVRSRV_DEVICE_NODE *psDeviceNode,
                                DEVMEMINT_CTX *psDevmemCtx,
                                IMG_DEV_VIRTADDR sDevAddrStart,
                                IMG_DEVMEM_SIZE_T uiSize,
                                IMG_UINT32 uiArraySize,
                                const IMG_CHAR *pszFilename,
                                IMG_UINT32 ui32FileOffset,
                                IMG_UINT32 ui32PDumpFlags);

/*
 * DevmemIntPDumpSaveToFileVirtualNoValidate()
 *
 * Writes out PDump "SAB" commands with the data found in memory at
 * the given virtual address. Doesn't perform address validation.
 */
PVRSRV_ERROR
DevmemIntPDumpSaveToFileVirtualNoValidate(PVRSRV_DEVICE_NODE *psDeviceNode,
                                          DEVMEMINT_CTX *psDevmemCtx,
                                          IMG_DEV_VIRTADDR sDevAddrStart,
                                          IMG_DEVMEM_SIZE_T uiSize,
                                          const IMG_CHAR *pszFilename,
                                          IMG_UINT32 ui32FileOffset,
                                          IMG_UINT32 ui32PDumpFlags);

IMG_UINT32
DevmemIntMMUContextID(DEVMEMINT_CTX *psDevMemContext);

PVRSRV_ERROR
DevmemIntPDumpImageDescriptor(CONNECTION_DATA * psConnection,
                              PVRSRV_DEVICE_NODE *psDeviceNode,
                              DEVMEMINT_CTX *psDevMemContext,
                              IMG_UINT32 ui32Size,
                              const IMG_CHAR *pszFileName,
                              IMG_DEV_VIRTADDR sData,
                              IMG_UINT32 ui32DataSize,
                              IMG_UINT32 ui32LogicalWidth,
                              IMG_UINT32 ui32LogicalHeight,
                              IMG_UINT32 ui32PhysicalWidth,
                              IMG_UINT32 ui32PhysicalHeight,
                              PDUMP_PIXEL_FORMAT ePixFmt,
                              IMG_MEMLAYOUT eMemLayout,
                              IMG_FB_COMPRESSION eFBCompression,
                              const IMG_UINT32 *paui32FBCClearColour,
                              PDUMP_FBC_SWIZZLE eFBCSwizzle,
                              IMG_DEV_VIRTADDR sHeader,
                              IMG_UINT32 ui32HeaderSize,
                              IMG_UINT32 ui32PDumpFlags);

PVRSRV_ERROR
DevmemIntPDumpDataDescriptor(CONNECTION_DATA * psConnection,
                             PVRSRV_DEVICE_NODE *psDeviceNode,
                             DEVMEMINT_CTX *psDevMemContext,
                             IMG_UINT32 ui32Size,
                             const IMG_CHAR *pszFileName,
                             IMG_DEV_VIRTADDR sData,
                             IMG_UINT32 ui32DataSize,
                             IMG_UINT32 ui32HeaderType,
                             IMG_UINT32 ui32ElementType,
                             IMG_UINT32 ui32ElementCount,
                             IMG_UINT32 ui32PDumpFlags);
#else /* PDUMP */

#ifdef INLINE_IS_PRAGMA
#pragma inline(DevmemIntPDumpGetValidRegions)
#endif
static INLINE PVRSRV_ERROR
DevmemIntPDumpGetValidRegions(CONNECTION_DATA * psConnection,
                              PVRSRV_DEVICE_NODE *psDeviceNode,
                              DEVMEMINT_CTX *psDevmemCtx,
                              IMG_DEV_VIRTADDR sDevAddrStart,
                              IMG_DEVMEM_SIZE_T uiSize)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(psDevmemCtx);
	PVR_UNREFERENCED_PARAMETER(sDevAddrStart);
	PVR_UNREFERENCED_PARAMETER(uiSize);

	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(DevmemIntPDumpFreeValidRegions)
#endif
static INLINE void
DevmemIntPDumpFreeValidRegions(DLLIST_NODE *psDevAddrRegions)
{
	PVR_UNREFERENCED_PARAMETER(psDevAddrRegions);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(DevmemIntPDumpSaveFromRegionListToFileVirtual)
#endif
static INLINE PVRSRV_ERROR
DevmemIntPDumpSaveFromRegionListToFileVirtual(CONNECTION_DATA * psConnection,
                                              PVRSRV_DEVICE_NODE *psDeviceNode,
                                              DEVMEMINT_CTX *psDevmemCtx,
                                              DLLIST_NODE *psDevAddrRegions,
                                              const IMG_CHAR *pszFilename,
                                              IMG_UINT32 ui32FileOffset,
                                              IMG_UINT32 ui32PDumpFlags)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(psDevmemCtx);
	PVR_UNREFERENCED_PARAMETER(psDevAddrRegions);
	PVR_UNREFERENCED_PARAMETER(pszFilename);
	PVR_UNREFERENCED_PARAMETER(ui32FileOffset);
	PVR_UNREFERENCED_PARAMETER(ui32PDumpFlags);

	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(DevmemIntPDumpSaveToFileVirtual)
#endif
static INLINE PVRSRV_ERROR
DevmemIntPDumpSaveToFileVirtual(CONNECTION_DATA * psConnection,
                                PVRSRV_DEVICE_NODE *psDeviceNode,
                                DEVMEMINT_CTX *psDevmemCtx,
                                IMG_DEV_VIRTADDR sDevAddrStart,
                                IMG_DEVMEM_SIZE_T uiSize,
                                IMG_UINT32 uiArraySize,
                                const IMG_CHAR *pszFilename,
                                IMG_UINT32 ui32FileOffset,
                                IMG_UINT32 ui32PDumpFlags)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(psDevmemCtx);
	PVR_UNREFERENCED_PARAMETER(sDevAddrStart);
	PVR_UNREFERENCED_PARAMETER(uiSize);
	PVR_UNREFERENCED_PARAMETER(uiArraySize);
	PVR_UNREFERENCED_PARAMETER(pszFilename);
	PVR_UNREFERENCED_PARAMETER(ui32FileOffset);
	PVR_UNREFERENCED_PARAMETER(ui32PDumpFlags);
	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(DevmemIntPDumpImageDescriptor)
#endif
static INLINE PVRSRV_ERROR
DevmemIntPDumpImageDescriptor(CONNECTION_DATA * psConnection,
                              PVRSRV_DEVICE_NODE *psDeviceNode,
                              DEVMEMINT_CTX *psDevMemContext,
                              IMG_UINT32 ui32Size,
                              const IMG_CHAR *pszFileName,
                              IMG_DEV_VIRTADDR sData,
                              IMG_UINT32 ui32DataSize,
                              IMG_UINT32 ui32LogicalWidth,
                              IMG_UINT32 ui32LogicalHeight,
                              IMG_UINT32 ui32PhysicalWidth,
                              IMG_UINT32 ui32PhysicalHeight,
                              PDUMP_PIXEL_FORMAT ePixFmt,
                              IMG_MEMLAYOUT eMemLayout,
                              IMG_FB_COMPRESSION eFBCompression,
                              const IMG_UINT32 *paui32FBCClearColour,
                              PDUMP_FBC_SWIZZLE eFBCSwizzle,
                              IMG_DEV_VIRTADDR sHeader,
                              IMG_UINT32 ui32HeaderSize,
                              IMG_UINT32 ui32PDumpFlags)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(psDevMemContext);
	PVR_UNREFERENCED_PARAMETER(ui32Size);
	PVR_UNREFERENCED_PARAMETER(pszFileName);
	PVR_UNREFERENCED_PARAMETER(sData);
	PVR_UNREFERENCED_PARAMETER(ui32DataSize);
	PVR_UNREFERENCED_PARAMETER(ui32LogicalWidth);
	PVR_UNREFERENCED_PARAMETER(ui32LogicalHeight);
	PVR_UNREFERENCED_PARAMETER(ui32PhysicalWidth);
	PVR_UNREFERENCED_PARAMETER(ui32PhysicalHeight);
	PVR_UNREFERENCED_PARAMETER(ePixFmt);
	PVR_UNREFERENCED_PARAMETER(eMemLayout);
	PVR_UNREFERENCED_PARAMETER(eFBCompression);
	PVR_UNREFERENCED_PARAMETER(paui32FBCClearColour);
	PVR_UNREFERENCED_PARAMETER(eFBCSwizzle);
	PVR_UNREFERENCED_PARAMETER(sHeader);
	PVR_UNREFERENCED_PARAMETER(ui32HeaderSize);
	PVR_UNREFERENCED_PARAMETER(ui32PDumpFlags);
	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(DevmemIntPDumpDataDescriptor)
#endif
static INLINE PVRSRV_ERROR
DevmemIntPDumpDataDescriptor(CONNECTION_DATA * psConnection,
                              PVRSRV_DEVICE_NODE *psDeviceNode,
                              DEVMEMINT_CTX *psDevMemContext,
                              IMG_UINT32 ui32Size,
                              const IMG_CHAR *pszFileName,
                              IMG_DEV_VIRTADDR sData,
                              IMG_UINT32 ui32DataSize,
                              IMG_UINT32 ui32ElementType,
                              IMG_UINT32 ui32ElementCount,
                              IMG_UINT32 ui32PDumpFlags)
{
	PVR_UNREFERENCED_PARAMETER(psConnection);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	PVR_UNREFERENCED_PARAMETER(psDevMemContext);
	PVR_UNREFERENCED_PARAMETER(ui32Size);
	PVR_UNREFERENCED_PARAMETER(pszFileName);
	PVR_UNREFERENCED_PARAMETER(sData);
	PVR_UNREFERENCED_PARAMETER(ui32DataSize);
	PVR_UNREFERENCED_PARAMETER(ui32ElementType);
	PVR_UNREFERENCED_PARAMETER(ui32ElementCount);
	PVR_UNREFERENCED_PARAMETER(ui32PDumpFlags);
	return PVRSRV_OK;
}

#endif /* PDUMP */

PVRSRV_ERROR
DevmemIntInit(void);

PVRSRV_ERROR
DevmemIntDeInit(void);

PVRSRV_ERROR
DevmemIntExportCtx(DEVMEMINT_CTX *psContext,
                   PMR *psPMR,
                   DEVMEMINT_CTX_EXPORT **ppsContextExport);

PVRSRV_ERROR
DevmemIntUnexportCtx(DEVMEMINT_CTX_EXPORT *psContextExport);

PVRSRV_ERROR
DevmemIntAcquireRemoteCtx(PMR *psPMR,
                          DEVMEMINT_CTX **ppsContext,
                          IMG_HANDLE *phPrivData);

#endif /* DEVICEMEM_SERVER_H */

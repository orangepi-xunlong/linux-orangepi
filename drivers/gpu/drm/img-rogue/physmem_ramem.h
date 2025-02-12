/*************************************************************************/ /*!
@File           physmem_ramem.h
@Title          Resource allocator managed PMR Factory common definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of Services memory management.  This file defines the
                RA managed memory PMR factory API that is shared between local
                physheap implementations (LMA & IMA)
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

#ifndef PHYSMEM_RAMEM_H
#define PHYSMEM_RAMEM_H

#include "img_types.h"
#include "pvrsrv_memallocflags.h"
#include "physheap.h"
#include "ra.h"
#include "connection_server.h"
#include "pmr_impl.h"

IMG_UINT32
RAMemGetPageShift(void);

PVRSRV_ERROR
RAMemDoPhyContigPagesAlloc(RA_ARENA *pArena,
                           size_t uiSize,
                           PVRSRV_DEVICE_NODE *psDevNode,
                           PG_HANDLE *psMemHandle,
                           IMG_DEV_PHYADDR *psDevPAddr,
                           IMG_PID uiPid);

void
RAMemDoPhyContigPagesFree(RA_ARENA *pArena,
                          PG_HANDLE *psMemHandle);

PVRSRV_ERROR
RAMemPhyContigPagesMap(PHYS_HEAP *psPhysHeap,
                  PG_HANDLE *psMemHandle,
                  size_t uiSize, IMG_DEV_PHYADDR *psDevPAddr,
                  void **pvPtr);

void
RAMemPhyContigPagesUnmap(PHYS_HEAP *psPhysHeap,
                    PG_HANDLE *psMemHandle,
                    void *pvPtr);

PVRSRV_ERROR
RAMemPhyContigPagesClean(PHYS_HEAP *psPhysHeap,
                    PG_HANDLE *psMemHandle,
                    IMG_UINT32 uiOffset,
                    IMG_UINT32 uiLength);

/*
 * PhysmemNewRAMemRamBackedPMR
 *
 * This function will create a PMR using the memory managed by
 * the PMR factory and is OS agnostic.
 */
PVRSRV_ERROR
PhysmemNewRAMemRamBackedPMR(PHYS_HEAP *psPhysHeap,
                            RA_ARENA *pArena,
                            CONNECTION_DATA *psConnection,
                            IMG_DEVMEM_SIZE_T uiSize,
                            IMG_UINT32 ui32NumPhysChunks,
                            IMG_UINT32 ui32NumVirtChunks,
                            IMG_UINT32 *pui32MappingTable,
                            IMG_UINT32 uiLog2PageSize,
                            PVRSRV_MEMALLOCFLAGS_T uiFlags,
                            const IMG_CHAR *pszAnnotation,
                            IMG_PID uiPid,
                            PMR **ppsPMRPtr,
                            IMG_UINT32 ui32PDumpFlags);

#endif /* PHYSMEM_RAMEM_H */

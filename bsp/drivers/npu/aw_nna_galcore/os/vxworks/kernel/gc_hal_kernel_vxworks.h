/****************************************************************************
*
*    Copyright (c) 2005 - 2024 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/

#ifndef __gc_hal_kernel_vxworks_h_
#define __gc_hal_kernel_vxworks_h_

#include <stdio.h>

#include <taskLib.h>
#include <spinlockLib.h>
#include <isrLib.h>
#include <vxAtomicLib.h>
#include <tickLib.h>
#include <semLib.h>
#include <iosLib.h>
#include <intLib.h>
#include <memLib.h>
#include <sysLib.h>

#include "gc_hal.h"
#include "gc_hal_driver.h"
#include "gc_hal_kernel.h"
#include "gc_hal_kernel_platform.h"
#include "gc_hal_kernel_device.h"
#include "gc_hal_kernel_os.h"
#include "gc_hal_ta.h"

#ifndef DEVICE_NAME
#   define DEVICE_NAME              "galcore"
#endif

#ifndef CLASS_NAME
#   define CLASS_NAME               "graphics_class"
#endif

#define PAGE_SIZE   0x4000
#define PAGE_SHIFT  14
#define PAGE_MASK   (~(PAGE_SIZE - 1))

#define GetPageCount(size, offset) ((((size) + ((offset) & ~PAGE_MASK)) + PAGE_SIZE - 1) >> PAGE_SHIFT)

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/
typedef struct _gcsIOMMU * gckIOMMU;

typedef struct _gcsUSER_MAPPING * gcsUSER_MAPPING_PTR;
typedef struct _gcsUSER_MAPPING
{
    /* Pointer to next mapping structure. */
    gcsUSER_MAPPING_PTR         next;

    /* Physical address of this mapping. */
    gctUINT32                   physical;

    /* Logical address of this mapping. */
    gctPOINTER                  logical;

    /* Number of bytes of this mapping. */
    gctSIZE_T                   bytes;

    /* Starting address of this mapping. */
    gctINT8_PTR                 start;

    /* Ending address of this mapping. */
    gctINT8_PTR                 end;
}
gcsUSER_MAPPING;

struct _gckOS
{
    /* Object. */
    gcsOBJECT                   object;

    /* Pointer to device */
    gckGALDEVICE                device;

    /* Memory management */
    pthread_mutex_t             mdlMutex;
    struct list_head            mdlHead;

    /* Kernel process ID. */
    gctUINT32                   kernelProcessID;

    /* Signal management. */

    /* Lock. */
    pthread_mutex_t             signalMutex;

    gcsUSER_MAPPING_PTR         userMap;

    /* Allocate extra page to avoid cache overflow */
    gctPOINTER                  paddingPage;

    /* Detect unfreed allocation. */
    atomic_t                    allocateCount;

    struct list_head            allocatorList;

    /* Lock for register access check. */
    spinlockIsr_t               registerAccessLock;

    /* External power states. */
    gctBOOL                     powerStates[gcdMAX_GPU_COUNT];

    /* External clock states. */
    gctBOOL                     clockStates[gcdMAX_GPU_COUNT];

    /* IOMMU. */
    gckIOMMU                    iommu;
};

typedef struct _gcsSIGNAL * gcsSIGNAL_PTR;
typedef struct _gcsSIGNAL
{
    /* Kernel sync primitive. */
    volatile unsigned int done;
    spinlockTask_t lock;

    SEM_ID sem;

    /* Manual reset flag. */
    gctBOOL manualReset;

    /* The reference counter. */
    atomic_t ref;

    /* The owner of the signal. */
    gctHANDLE process;
}
gcsSIGNAL;

gceSTATUS
gckOS_ImportAllocators(
    gckOS Os
    );

gceSTATUS
gckOS_FreeAllocators(
    gckOS Os
    );

/* Reserved memory. */
gceSTATUS
gckOS_RequestReservedMemory(
    gckOS Os,
    unsigned long Start,
    unsigned long Size,
    const char * Name,
    gctBOOL Requested,
    void ** MemoryHandle
    );

void
gckOS_ReleaseReservedMemory(
    gckOS Os,
    void * MemoryHandle
    );

/* Reserved memory sub area */
gceSTATUS
gckOS_RequestReservedMemoryArea(
    void * MemoryHandle,
    unsigned long Offset,
    unsigned long Size,
    void ** MemoryAreaHandle
    );

void
gckOS_ReleaseReservedMemoryArea(
    void * MemoryAreaHandle
    );

gceSTATUS
_ConvertLogical2Physical(
    IN gckOS Os,
    IN gctPOINTER Logical,
    IN gctUINT32 ProcessID,
    IN PVX_MDL Mdl,
    OUT gctPHYS_ADDR_T * Physical
    );

gceSTATUS
_QuerySignal(
    IN gckOS Os,
    IN gctSIGNAL Signal
    );

static inline gctINT
_GetProcessID(
    void
    )
{
    return taskIdSelf();
}

#ifdef CONFIG_IOMMU_SUPPORT
void
gckIOMMU_Destory(
    IN gckOS Os,
    IN gckIOMMU Iommu
    );

gceSTATUS
gckIOMMU_Construct(
    IN gckOS Os,
    OUT gckIOMMU * Iommu
    );

gceSTATUS
gckIOMMU_Map(
    IN gckIOMMU Iommu,
    IN gctUINT32 DomainAddress,
    IN gctUINT32 Physical,
    IN gctUINT32 Bytes
    );

gceSTATUS
gckIOMMU_Unmap(
    IN gckIOMMU Iommu,
    IN gctUINT32 DomainAddress,
    IN gctUINT32 Bytes
    );
#endif

#endif /* __gc_hal_kernel_vxworks_h_ */

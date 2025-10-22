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

#include "gc_hal_kernel_vxworks.h"

#define _GC_OBJ_ZONE    gcvZONE_OS

#include "gc_hal_kernel_allocator.h"

#define gcmkBUG_ON(x) \
    do { \
        if (!!(x)) \
        { \
            gcmkPRINT("[galcore]: BUG ON @ %s(%d)\n", __func__, __LINE__); \
        } \
    } while (0)

static gctUINT32 ticksPerSecond = 0;

/******************************************************************************\
******************************* Private Functions ******************************
\******************************************************************************/
static gctINT
_GetThreadID(
    void
    )
{
    return pthread_self();
}

/* Must hold Mdl->mpasMutex before call this function. */
static inline PVX_MDL_MAP
_CreateMdlMap(
    IN PVX_MDL Mdl,
    IN gctINT ProcessID
    )
{
    PVX_MDL_MAP  mdlMap;

    gcmkHEADER_ARG("Mdl=0x%X ProcessID=%d", Mdl, ProcessID);

    mdlMap = (PVX_MDL_MAP)malloc(sizeof(struct _VX_MDL_MAP));

    if (mdlMap == gcvNULL)
    {
        gcmkFOOTER_NO();
        return gcvNULL;
    }

    mdlMap->pid     = ProcessID;
    mdlMap->vmaAddr = gcvNULL;
    mdlMap->count   = 0;

    list_add(&mdlMap->link, &Mdl->mapsHead);

    gcmkFOOTER_ARG("0x%X", mdlMap);
    return mdlMap;
}

/* Must hold Mdl->mpasMutex before call this function. */
static inline gceSTATUS
_DestroyMdlMap(
    IN PVX_MDL Mdl,
    IN PVX_MDL_MAP MdlMap
    )
{
    gcmkHEADER_ARG("Mdl=0x%X MdlMap=0x%X", Mdl, MdlMap);

    /* Verify the arguments. */
    gcmkVERIFY_ARGUMENT(MdlMap != gcvNULL);

    list_del(&MdlMap->link);
    free(MdlMap);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Must hold Mdl->mpasMutex before call this function. */
extern PVX_MDL_MAP
FindMdlMap(
    IN PVX_MDL Mdl,
    IN gctINT ProcessID
    )
{
    PVX_MDL_MAP mdlMap;

    gcmkHEADER_ARG("Mdl=0x%X ProcessID=%d", Mdl, ProcessID);

    if (Mdl == gcvNULL)
    {
        gcmkFOOTER_NO();
        return gcvNULL;
    }

    list_for_each_entry(mdlMap, &Mdl->mapsHead, link)
    {
        if (mdlMap->pid == ProcessID)
        {
            gcmkFOOTER_ARG("0x%X", mdlMap);
            return mdlMap;
        }
    }

    gcmkFOOTER_NO();
    return gcvNULL;
}


static PVX_MDL
_CreateMdl(
    IN gckOS Os
    )
{
    PVX_MDL mdl;

    gcmkHEADER();

    mdl = (PVX_MDL)malloc(sizeof(struct _VX_MDL));

    if (mdl)
    {
        mdl->os = Os;
        vxAtomicSet(&mdl->refs, 1);
        pthread_mutex_init(&mdl->mapsMutex, 0);
        INIT_LIST_HEAD(&mdl->mapsHead);
    }

    gcmkFOOTER_ARG("0x%X", mdl);
    return mdl;
}

static gceSTATUS
_DestroyMdl(
    IN PVX_MDL Mdl
    )
{
    gcmkHEADER_ARG("Mdl=0x%X", Mdl);

    /* Verify the arguments. */
    gcmkVERIFY_ARGUMENT(Mdl != gcvNULL);

    vxAtomicDec(&Mdl->refs);
    if ((gctINT)vxAtomicGet(&Mdl->refs) == 0)
    {
        gckOS os = Mdl->os;
        gckALLOCATOR allocator = Mdl->allocator;
        PVX_MDL_MAP mdlMap, next;

        /* Valid private means alloc/attach successfully */
        if (Mdl->priv)
        {
            if (Mdl->addr)
            {
                allocator->ops->UnmapKernel(allocator, Mdl, Mdl->addr);
            }
            allocator->ops->Free(allocator, Mdl);
        }

        pthread_mutex_lock(&Mdl->mapsMutex);
        list_for_each_entry_safe(mdlMap, next, &Mdl->mapsHead, link)
        {
            gcmkVERIFY_OK(_DestroyMdlMap(Mdl, mdlMap));
        }
        pthread_mutex_unlock(&Mdl->mapsMutex);

        if (Mdl->link.next)
        {
            /* Remove the node from global list.. */
            pthread_mutex_lock(&os->mdlMutex);
            list_del(&Mdl->link);
            pthread_mutex_unlock(&os->mdlMutex);
        }

        free(Mdl);
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

static inline gceSTATUS
_QueryProcessPageTable(
    IN gctPOINTER Logical,
    OUT gctPHYS_ADDR_T * Address
    )
{
    *Address = KM_TO_PHYS(Logical);

    return gcvSTATUS_OK;
}


static gceSTATUS
_ShrinkMemory(
    IN gckOS Os
    )
{
    gcsPLATFORM * platform;
    gceSTATUS status = gcvSTATUS_OK;

    gcmkHEADER_ARG("Os=0x%X", Os);
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);

    platform = Os->device->platform;

    if (platform && platform->ops->shrinkMemory)
    {
        status = platform->ops->shrinkMemory(platform);
    }
    else
    {
        gcmkFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }

    gcmkFOOTER_NO();
    return status;
}

/*******************************************************************************
**
**  gckOS_Construct
**
**  Construct a new gckOS object.
**
**  INPUT:
**
**      gctPOINTER Context
**          Pointer to the gckGALDEVICE class.
**
**  OUTPUT:
**
**      gckOS * Os
**          Pointer to a variable that will hold the pointer to the gckOS object.
*/
gceSTATUS
gckOS_Construct(
    IN gctPOINTER Context,
    OUT gckOS * Os
    )
{
    gckOS os;
    gceSTATUS status;

    gcmkHEADER_ARG("Context=0x%X", Context);

    /* Verify the arguments. */
    gcmkVERIFY_ARGUMENT(Os != gcvNULL);

    /* Allocate the gckOS object. */
    os = (gckOS) malloc(gcmSIZEOF(struct _gckOS));

    if (os == gcvNULL)
    {
        /* Out of memory. */
        gcmkFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_MEMORY);
        return gcvSTATUS_OUT_OF_MEMORY;
    }

    /* Zero the memory. */
    gckOS_ZeroMemory(os, gcmSIZEOF(struct _gckOS));

    /* Initialize the gckOS object. */
    os->object.type = gcvOBJ_OS;

    /* Set device device. */
    os->device = Context;

    ticksPerSecond = sysClkRateGet();

    /* Set allocateCount to 0, gckOS_Allocate has not been used yet. */
    vxAtomicSet(&os->allocateCount, 0);

    /* Initialize the memory lock. */
    pthread_mutex_init(&os->mdlMutex, 0);

    INIT_LIST_HEAD(&os->mdlHead);

    /* Get the kernel process ID. */
    os->kernelProcessID = _GetProcessID();

    /*
     * Initialize the signal manager.
     */

    /* Initialize mutex. */
    pthread_mutex_init(&os->signalMutex, 0);

    os->paddingPage = valloc(PAGE_SIZE);
    if (os->paddingPage == gcvNULL)
    {
        /* Out of memory. */
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    spinLockIsrInit(&os->registerAccessLock, 0);

    gckOS_ImportAllocators(os);

#ifdef CONFIG_IOMMU_SUPPORT
    if (((gckGALDEVICE)(os->device))->args.mmu == gcvFALSE)
    {
        /* Only use IOMMU when internal MMU is not enabled. */
        status = gckIOMMU_Construct(os, &os->iommu);

        if (gcmIS_ERROR(status))
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_OS,
                "%s(%d): Fail to setup IOMMU",
                __FUNCTION__, __LINE__
                );
        }
    }
#endif

    /* Return pointer to the gckOS object. */
    *Os = os;

    /* Success. */
    gcmkFOOTER_ARG("*Os=0x%X", *Os);
    return gcvSTATUS_OK;

OnError:
    free(os);

    /* Return the error. */
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_Destroy
**
**  Destroy an gckOS object.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_Destroy(
    IN gckOS Os
    )
{
    gcmkHEADER_ARG("Os=0x%X", Os);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);

    gckOS_FreeAllocators(Os);

#ifdef CONFIG_IOMMU_SUPPORT
    if (Os->iommu)
    {
        gckIOMMU_Destory(Os, Os->iommu);
    }
#endif

    /* Mark the gckOS object as unknown. */
    Os->object.type = gcvOBJ_UNKNOWN;

    /* Free the gckOS object. */
    free(Os);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_CreateKernelMapping(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Logical
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    PVX_MDL mdl = (PVX_MDL)Physical;
    gckALLOCATOR allocator = mdl->allocator;

    gcmkHEADER_ARG("Os=%p Physical=%p Offset=0x%zx Bytes=0x%zx",
                   Os, Physical, Offset, Bytes);

    if (mdl->addr)
    {
        /* Already mapped whole memory. */
        *Logical = (gctUINT8_PTR)mdl->addr + Offset;
    }
    else
    {
        gcmkONERROR(allocator->ops->MapKernel(allocator, mdl, Offset, Bytes, Logical));
    }

OnError:
    gcmkFOOTER_ARG("*Logical=%p", gcmOPT_POINTER(Logical));
    return status;
}

gceSTATUS
gckOS_DestroyKernelMapping(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctPOINTER Logical
    )
{
    PVX_MDL mdl = (PVX_MDL)Physical;
    gckALLOCATOR allocator = mdl->allocator;

    gcmkHEADER_ARG("Os=%p Physical=%p Logical=%p", Os, Physical, Logical);

    if (mdl->addr)
    {
        /* Nothing to do. */
    }
    else
    {
        allocator->ops->UnmapKernel(allocator, mdl, Logical);
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_Allocate
**
**  Allocate memory.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctSIZE_T Bytes
**          Number of bytes to allocate.
**
**  OUTPUT:
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the allocated memory location.
*/
gceSTATUS
gckOS_Allocate(
    IN gckOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=0x%X Bytes=%lu", Os, Bytes);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Bytes > 0);
    gcmkVERIFY_ARGUMENT(Memory != gcvNULL);

    gcmkONERROR(gckOS_AllocateMemory(Os, Bytes, Memory));

    /* Success. */
    gcmkFOOTER_ARG("*Memory=0x%X", *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_Free
**
**  Free allocated memory.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Memory
**          Pointer to memory allocation to free.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_Free(
    IN gckOS Os,
    IN gctPOINTER Memory
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=0x%X Memory=0x%X", Os, Memory);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Memory != gcvNULL);

    gcmkONERROR(gckOS_FreeMemory(Os, Memory));

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_AllocateMemory
**
**  Allocate memory wrapper.
**
**  INPUT:
**
**      gctSIZE_T Bytes
**          Number of bytes to allocate.
**
**  OUTPUT:
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the allocated memory location.
*/
gceSTATUS
gckOS_AllocateMemory(
    IN gckOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gctPOINTER memory;
    gceSTATUS status;

    gcmkHEADER_ARG("Os=0x%X Bytes=%lu", Os, Bytes);

    /* Verify the arguments. */
    gcmkVERIFY_ARGUMENT(Bytes > 0);
    gcmkVERIFY_ARGUMENT(Memory != gcvNULL);

    memory = (gctPOINTER) malloc(Bytes);

    if (memory == gcvNULL)
    {
        /* Out of memory. */
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    /* Increase count. */
    vxAtomicInc(&Os->allocateCount);

    /* Return pointer to the memory allocation. */
    *Memory = memory;

    /* Success. */
    gcmkFOOTER_ARG("*Memory=0x%X", *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_FreeMemory
**
**  Free allocated memory wrapper.
**
**  INPUT:
**
**      gctPOINTER Memory
**          Pointer to memory allocation to free.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_FreeMemory(
    IN gckOS Os,
    IN gctPOINTER Memory
    )
{
    gcmkHEADER_ARG("Memory=0x%X", Memory);

    /* Verify the arguments. */
    gcmkVERIFY_ARGUMENT(Memory != gcvNULL);

    /* Free the memory from the OS pool. */
    free(Memory);

    /* Decrease count. */
    vxAtomicDec(&Os->allocateCount);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_MapMemory
**
**  Map physical memory into the current process.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPHYS_ADDR Physical
**          Start of physical address memory.
**
**      gctSIZE_T Bytes
**          Number of bytes to map.
**
**  OUTPUT:
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the logical address of the
**          mapped memory.
*/
gceSTATUS
gckOS_MapMemory(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Logical
    )
{
    gceSTATUS status;
    PVX_MDL_MAP  mdlMap;
    PVX_MDL      mdl = (PVX_MDL) Physical;
    gckALLOCATOR allocator;
    gctINT pid = _GetProcessID();

    gcmkHEADER_ARG("Os=0x%X Physical=0x%X Bytes=%lu", Os, Physical, Bytes);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Physical != 0);
    gcmkVERIFY_ARGUMENT(Bytes > 0);
    gcmkVERIFY_ARGUMENT(Logical != gcvNULL);

    pthread_mutex_lock(&mdl->mapsMutex);

    mdlMap = FindMdlMap(mdl, pid);

    if (mdlMap == gcvNULL)
    {
        mdlMap = _CreateMdlMap(mdl, pid);

        if (mdlMap == gcvNULL)
        {
            gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }
    }

    if (mdlMap->vmaAddr == gcvNULL)
    {
        allocator = mdl->allocator;

        gcmkONERROR(
            allocator->ops->MapUser(allocator,
                                    mdl, mdlMap,
                                    gcvFALSE));
    }

    pthread_mutex_unlock(&mdl->mapsMutex);

    *Logical = mdlMap->vmaAddr;

    gcmkFOOTER_ARG("*Logical=0x%X", *Logical);
    return gcvSTATUS_OK;

OnError:
    pthread_mutex_unlock(&mdl->mapsMutex);

    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_UnmapMemory
**
**  Unmap physical memory out of the current process.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPHYS_ADDR Physical
**          Start of physical address memory.
**
**      gctSIZE_T Bytes
**          Number of bytes to unmap.
**
**      gctPOINTER Memory
**          Pointer to a previously mapped memory region.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_UnmapMemory(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Bytes,
    IN gctPOINTER Logical
    )
{
    gcmkHEADER_ARG("Os=0x%X Physical=0x%X Bytes=%lu Logical=0x%X",
                   Os, Physical, Bytes, Logical);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Physical != 0);
    gcmkVERIFY_ARGUMENT(Bytes > 0);
    gcmkVERIFY_ARGUMENT(Logical != gcvNULL);

    gckOS_UnmapMemoryEx(Os, Physical, Bytes, Logical, _GetProcessID());

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  gckOS_UnmapMemoryEx
**
**  Unmap physical memory in the specified process.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPHYS_ADDR Physical
**          Start of physical address memory.
**
**      gctSIZE_T Bytes
**          Number of bytes to unmap.
**
**      gctPOINTER Memory
**          Pointer to a previously mapped memory region.
**
**      gctUINT32 PID
**          Pid of the process that opened the device and mapped this memory.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_UnmapMemoryEx(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Bytes,
    IN gctPOINTER Logical,
    IN gctUINT32 PID
    )
{
    PVX_MDL_MAP          mdlMap;
    PVX_MDL              mdl = (PVX_MDL)Physical;

    gcmkHEADER_ARG("Os=0x%X Physical=0x%X Bytes=%lu Logical=0x%X PID=%d",
                   Os, Physical, Bytes, Logical, PID);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Physical != 0);
    gcmkVERIFY_ARGUMENT(Bytes > 0);
    gcmkVERIFY_ARGUMENT(Logical != gcvNULL);
    gcmkVERIFY_ARGUMENT(PID != 0);

    if (Logical)
    {
        gckALLOCATOR allocator = mdl->allocator;

        pthread_mutex_lock(&mdl->mapsMutex);

        mdlMap = FindMdlMap(mdl, PID);

        if (mdlMap == gcvNULL)
        {
            pthread_mutex_unlock(&mdl->mapsMutex);

            gcmkFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        if (mdlMap->vmaAddr != gcvNULL)
        {
            gcmkBUG_ON(!allocator || !allocator->ops->UnmapUser);
            allocator->ops->UnmapUser(allocator, mdl, mdlMap, mdl->bytes);
        }

        gcmkVERIFY_OK(_DestroyMdlMap(mdl, mdlMap));

        pthread_mutex_unlock(&mdl->mapsMutex);
    }

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AllocateNonPagedMemory
**
**  Allocate a number of pages from non-paged memory.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gckKERNEL Kernel
**          Which core allocates the memory.
**
**      gctBOOL InUserSpace
**          gcvTRUE if the pages need to be mapped into user space.
**
**      gctUINT32 Flag
**          Allocation attribute.
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that holds the number of bytes to allocate.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that hold the number of bytes allocated.
**
**      gctPHYS_ADDR * Physical
**          Pointer to a variable that will hold the physical address of the
**          allocation.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will hold the logical address of the
**          allocation.
*/
gceSTATUS
gckOS_AllocateNonPagedMemory(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gctBOOL InUserSpace,
    IN gctUINT32 Flag,
    IN OUT gctSIZE_T * Bytes,
    OUT gctPHYS_ADDR * Physical,
    OUT gctPOINTER * Logical
    )
{
    gctSIZE_T bytes;
    gctINT numPages;
    PVX_MDL mdl = gcvNULL;
    PVX_MDL_MAP mdlMap = gcvNULL;
    gctPOINTER addr;
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;
    gckALLOCATOR allocator;

    gcmkHEADER_ARG("Os=0x%X InUserSpace=%d *Bytes=%lu",
                   Os, InUserSpace, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Bytes != gcvNULL);
    gcmkVERIFY_ARGUMENT(*Bytes > 0);
    gcmkVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmkVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Align number of bytes to page size. */
    bytes = gcmALIGN(*Bytes, PAGE_SIZE);

    /* Get total number of pages.. */
    numPages = GetPageCount(bytes, 0);

    /* Allocate mdl structure */
    mdl = _CreateMdl(Os);
    if (mdl == gcvNULL)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    gcmkASSERT(Flag & gcvALLOC_FLAG_CONTIGUOUS);

    /* Walk all allocators. */
    list_for_each_entry(allocator, &Os->allocatorList, link)
    {
        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS,
                       "%s(%d) flag = %x allocator->capability = %x",
                        __FUNCTION__, __LINE__, Flag, allocator->capability);

        if ((Flag & allocator->capability) != Flag)
        {
            continue;
        }

        status = allocator->ops->Alloc(allocator, mdl, numPages, Flag);

        if (gcmIS_SUCCESS(status))
        {
            mdl->allocator = allocator;
            break;
        }
    }

    /* Check status. */
    gcmkONERROR(status);

    mdl->cacheable = Flag & gcvALLOC_FLAG_CACHEABLE;

    mdl->bytes    = bytes;
    mdl->numPages = numPages;

    mdl->contiguous = gcvTRUE;

    gcmkONERROR(allocator->ops->MapKernel(allocator, mdl, 0, bytes, &addr));

    /* Trigger a page fault. */
    memset(addr, 0, numPages * PAGE_SIZE);

    mdl->addr = addr;

    if (InUserSpace)
    {
        mdlMap = _CreateMdlMap(mdl, _GetProcessID());

        if (mdlMap == gcvNULL)
        {
            gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }

        gcmkONERROR(allocator->ops->MapUser(allocator, mdl, mdlMap, gcvFALSE));

        *Logical = mdlMap->vmaAddr;
    }
    else
    {
        *Logical = addr;
    }

    /*
     * Add this to a global list.
     * Will be used by get physical address
     * and mapuser pointer functions.
     */
    pthread_mutex_lock(&Os->mdlMutex);
    list_add_tail(&mdl->link, &Os->mdlHead);
    pthread_mutex_unlock(&Os->mdlMutex);

    /* Return allocated memory. */
    *Bytes = bytes;
    *Physical = (gctPHYS_ADDR) mdl;

    /* Success. */
    gcmkFOOTER_ARG("*Bytes=%lu *Physical=0x%X *Logical=0x%X",
                   *Bytes, *Physical, *Logical);
    return gcvSTATUS_OK;

OnError:
    if (mdl != gcvNULL)
    {
        /* Free VX_MDL. */
        gcmkVERIFY_OK(_DestroyMdl(mdl));
    }

    /* Return the status. */
    gcmkFOOTER();
    return status;
}


/*******************************************************************************
**
**  gckOS_FreeNonPagedMemory
**
**  Free previously allocated and mapped pages from non-paged memory.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctSIZE_T Bytes
**          Number of bytes allocated.
**
**      gctPHYS_ADDR Physical
**          Physical address of the allocated memory.
**
**      gctPOINTER Logical
**          Logical address of the allocated memory.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gckOS_FreeNonPagedMemory(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    PVX_MDL mdl = (PVX_MDL)Physical;

    gcmkHEADER_ARG("Os=0x%X Bytes=%lu Physical=0x%X Logical=0x%X",
                   Os, Bytes, Physical, Logical);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Bytes > 0);
    gcmkVERIFY_ARGUMENT(Physical != 0);
    gcmkVERIFY_ARGUMENT(Logical != gcvNULL);

    gcmkVERIFY_OK(_DestroyMdl(mdl));

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

static inline gckALLOCATOR
_FindAllocator(
    gckOS Os,
    gctUINT Flag
    )
{
    gckALLOCATOR allocator;

    list_for_each_entry(allocator, &Os->allocatorList, link)
    {
        if ((allocator->capability & Flag) == Flag)
        {
            return allocator;
        }
    }

    return gcvNULL;
}

gceSTATUS
gckOS_RequestReservedMemory(
    gckOS Os,
    unsigned long Start,
    unsigned long Size,
    const char * Name,
    gctBOOL Requested,
    void ** MemoryHandle
    )
{
    PVX_MDL mdl = gcvNULL;
    gceSTATUS status;
    gckALLOCATOR allocator;
    gcsATTACH_DESC desc;

    gcmkHEADER_ARG("start=0x%lx size=0x%lx name=%s", Start, Size, Name);

    /* Round up to page size. */
    Size = (Size + ~PAGE_MASK) & PAGE_MASK;

    mdl = _CreateMdl(Os);
    if (!mdl)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    desc.reservedMem.start     = Start;
    desc.reservedMem.size      = Size;
    desc.reservedMem.name      = Name;
    desc.reservedMem.requested = Requested;

    allocator = _FindAllocator(Os, gcvALLOC_FLAG_LINUX_RESERVED_MEM);
    if (!allocator)
    {
        gcmkPRINT("reserved-mem allocator not integrated!");
        gcmkONERROR(gcvSTATUS_GENERIC_IO);
    }

    /* Call attach. */
    gcmkONERROR(allocator->ops->Attach(allocator, &desc, mdl));

    /* Assign alloator. */
    mdl->allocator      = allocator;
    mdl->bytes          = Size;
    mdl->numPages       = Size >> PAGE_SHIFT;
    mdl->contiguous     = gcvTRUE;
    mdl->addr           = gcvNULL;
    mdl->gid            = 0;

    /*
     * Add this to a global list.
     * Will be used by get physical address
     * and mapuser pointer functions.
     */
    pthread_mutex_lock(&Os->mdlMutex);
    list_add_tail(&mdl->link, &Os->mdlHead);
    pthread_mutex_unlock(&Os->mdlMutex);

    *MemoryHandle = (void *)mdl;

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (mdl)
    {
        gcmkVERIFY_OK(_DestroyMdl(mdl));
    }

    gcmkFOOTER();
    return status;
}

void
gckOS_ReleaseReservedMemory(
    gckOS Os,
    void * MemoryHandle
    )
{
    gckALLOCATOR allocator;
    PVX_MDL mdl = (PVX_MDL)MemoryHandle;

    allocator = _FindAllocator(Os, gcvALLOC_FLAG_LINUX_RESERVED_MEM);

    /* If no allocator, how comes the memory? */
    gcmkBUG_ON(!allocator);

    allocator->ops->Free(allocator, mdl);
}

gceSTATUS
gckOS_RequestReservedMemoryArea(
    gckOS Os,
    void * MemoryHandle,
    unsigned long Offset,
    unsigned long Size,
    void ** MemoryAreaHandle
    )
{
    return gcvSTATUS_OK;
}

void
gckOS_ReleaseReservedMemoryArea(
    void * MemoryAreaHandle
    )
{
    return;
}

/*******************************************************************************
**
**  gckOS_ReadRegister
**
**  Read data from a register.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctUINT32 Address
**          Address of register.
**
**  OUTPUT:
**
**      gctUINT32 * Data
**          Pointer to a variable that receives the data read from the register.
*/
gceSTATUS
gckOS_ReadRegisterEx(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gctUINT32 Address,
    OUT gctUINT32 * Data
    )
{
    spinLockIsrTake(&Os->registerAccessLock);

    if (Os->clockStates[Kernel->core] == gcvFALSE)
    {
        spinLockIsrGive(&Os->registerAccessLock);

        /*
         * Read register when power off:
         * 1. In shared IRQ, read register may be called and that's not our irq.
         */
        return gcvSTATUS_GENERIC_IO;
    }

    *Data = *((volatile unsigned int *)((gctUINT8 *)Os->device->registerBases[Kernel->core] + Address));

    spinLockIsrGive(&Os->registerAccessLock);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_WriteRegisterEx
**
**  Write data to a register.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gckKERNEL Kernel
**          Core whose register is written.
**
**      gctUINT32 Address
**          Address of register.
**
**      gctUINT32 Data
**          Data for register.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_WriteRegisterEx(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    spinLockIsrTake(&Os->registerAccessLock);

    if (Os->clockStates[Kernel->core] == gcvFALSE)
    {
        spinLockIsrGive(&Os->registerAccessLock);

        gcmkPRINT("[galcore]: %s(%d) GPU[%d] external clock off",
               __func__, __LINE__, Kernel->core);

        /* Driver bug: register write when clock off. */
        gcmkBUG_ON(1);
        return gcvSTATUS_GENERIC_IO;
    }

    *((volatile unsigned int *)((gctUINT8 *)Os->device->registerBases[Kernel->core] + Address)) = Data;

    spinLockIsrGive(&Os->registerAccessLock);

    /* Success. */
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_WriteRegisterEx_NoDump(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    return gckOS_WriteRegisterEx(Os, Kernel, Address, Data);
}

/*******************************************************************************
**
**  gckOS_GetPageSize
**
**  Get the system's page size.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**  OUTPUT:
**
**      gctSIZE_T * PageSize
**          Pointer to a variable that will receive the system's page size.
*/
gceSTATUS gckOS_GetPageSize(
    IN gckOS Os,
    OUT gctSIZE_T * PageSize
    )
{
    gcmkHEADER_ARG("Os=0x%X", Os);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(PageSize != gcvNULL);

    /* Return the page size. */
    *PageSize = (gctSIZE_T) PAGE_SIZE;

    /* Success. */
    gcmkFOOTER_ARG("*PageSize=%d", *PageSize);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_GetPhysicalAddressProcess
**
**  Get the physical system address of a corresponding virtual address for a
**  given process.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to gckOS object.
**
**      gctPOINTER Logical
**          Logical address.
**
**      gctUINT32 ProcessID
**          Process ID.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Poinetr to a variable that receives the 32-bit physical adress.
*/
static gceSTATUS
_GetPhysicalAddressProcess(
    IN gckOS Os,
    IN gctPOINTER Logical,
    IN gctUINT32 ProcessID,
    OUT gctPHYS_ADDR_T * Address
    )
{
    PVX_MDL mdl;
    gceSTATUS status = gcvSTATUS_INVALID_ADDRESS;

    gcmkHEADER_ARG("Os=0x%X Logical=0x%X ProcessID=%d", Os, Logical, ProcessID);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Address != gcvNULL);

    pthread_mutex_lock(&Os->mdlMutex);

    if (Os->device->contiguousPhysical)
    {
        /* Try the contiguous memory pool. */
        mdl = (PVX_MDL) Os->device->contiguousPhysical;

        pthread_mutex_lock(&mdl->mapsMutex);

        status = _ConvertLogical2Physical(Os, Logical, ProcessID, mdl, Address);

        pthread_mutex_unlock(&mdl->mapsMutex);
    }

    if (gcmIS_ERROR(status))
    {
        /* Walk all MDLs. */
        list_for_each_entry(mdl, &Os->mdlHead, link)
        {
            pthread_mutex_lock(&mdl->mapsMutex);

            status = _ConvertLogical2Physical(Os, Logical, ProcessID, mdl, Address);

            pthread_mutex_unlock(&mdl->mapsMutex);

            if (gcmIS_SUCCESS(status))
            {
                break;
            }
        }
    }

    pthread_mutex_unlock(&Os->mdlMutex);

    gcmkONERROR(status);

    /* Success. */
    gcmkFOOTER_ARG("*Address=%p", *Address);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}



/*******************************************************************************
**
**  gckOS_GetPhysicalAddress
**
**  Get the physical system address of a corresponding virtual address.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Logical
**          Logical address.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Poinetr to a variable that receives the 32-bit physical adress.
*/
gceSTATUS
gckOS_GetPhysicalAddress(
    IN gckOS Os,
    IN gctPOINTER Logical,
    OUT gctPHYS_ADDR_T * Address
    )
{
    gceSTATUS status;
    gctUINT32 processID;

    gcmkHEADER_ARG("Os=0x%X Logical=0x%X", Os, Logical);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Address != gcvNULL);

    /* Query page table of current process first. */
    status = _QueryProcessPageTable(Logical, Address);

    if (gcmIS_ERROR(status))
    {
        /* Get current process ID. */
        processID = _GetProcessID();

        /* Route through other function. */
        gcmkONERROR(
            _GetPhysicalAddressProcess(Os, Logical, processID, Address));
    }

    /* Success. */
    gcmkFOOTER_ARG("*Address=%p", *Address);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

gceSTATUS
gckOS_GetPhysicalFromHandle(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Offset,
    OUT gctPHYS_ADDR_T * PhysicalAddress
    )
{
    PVX_MDL mdl = (PVX_MDL)Physical;
    gckALLOCATOR allocator = mdl->allocator;

    return allocator->ops->Physical(allocator, mdl, Offset, PhysicalAddress);
}

/*******************************************************************************
**
**  gckOS_UserLogicalToPhysical
**
**  Get the physical system address of a corresponding user virtual address.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Logical
**          Logical address.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Pointer to a variable that receives the 32-bit physical address.
*/
gceSTATUS gckOS_UserLogicalToPhysical(
    IN gckOS Os,
    IN gctPOINTER Logical,
    OUT gctPHYS_ADDR_T * Address
    )
{
    return gckOS_GetPhysicalAddress(Os, Logical, Address);
}

gceSTATUS
_ConvertLogical2Physical(
    IN gckOS Os,
    IN gctPOINTER Logical,
    IN gctUINT32 ProcessID,
    IN PVX_MDL Mdl,
    OUT gctPHYS_ADDR_T * Physical
    )
{
    gckALLOCATOR allocator = Mdl->allocator;
    gctUINT32 offset;
    gceSTATUS status = gcvSTATUS_NOT_FOUND;
    gctINT8_PTR vBase;

    vBase = (gctINT8_PTR) Mdl->addr;

    /* Is the given address within that range. */
    if ((vBase != gcvNULL)
    &&  ((gctINT8_PTR) Logical >= vBase)
    &&  ((gctINT8_PTR) Logical <  vBase + Mdl->numPages * PAGE_SIZE)
    )
    {
        offset = (gctINT8_PTR) Logical - vBase;

        allocator->ops->Physical(allocator, Mdl, offset, Physical);

        status = gcvSTATUS_OK;
    }

    return status;
}

/*******************************************************************************
**
**  gckOS_MapPhysical
**
**  Map a physical address into kernel space.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPHYS_ADDR_T Physical
**          Physical address of the memory to map.
**
**      gctSIZE_T Bytes
**          Number of bytes to map.
**
**  OUTPUT:
**
**      gctPOINTER * Logical
**          Pointer to a variable that receives the base address of the mapped
**          memory.
*/
gceSTATUS
gckOS_MapPhysical(
    IN gckOS Os,
    IN gctPHYS_ADDR_T Physical,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Logical
    )
{
    *Logical = (gctPOINTER) PHYS_TO_KM(Physical);

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_UnmapPhysical
**
**  Unmap a previously mapped memory region from kernel memory.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Logical
**          Pointer to the base address of the memory to unmap.
**
**      gctSIZE_T Bytes
**          Number of bytes to unmap.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_UnmapPhysical(
    IN gckOS Os,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_DeleteMutex
**
**  Delete a mutex.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Mutex
**          Pointer to the mute to be deleted.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_DeleteMutex(
    IN gckOS Os,
    IN gctPOINTER Mutex
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=0x%X Mutex=0x%X", Os, Mutex);

    /* Validate the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Mutex != gcvNULL);

    /* Destroy the mutex. */
    pthread_mutex_destroy(Mutex);

    /* Free the mutex structure. */
    gcmkONERROR(gckOS_Free(Os, Mutex));

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return status. */
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_AcquireMutex
**
**  Acquire a mutex.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Mutex
**          Pointer to the mutex to be acquired.
**
**      gctUINT32 Timeout
**          Timeout value specified in milliseconds.
**          Specify the value of gcvINFINITE to keep the thread suspended
**          until the mutex has been acquired.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_AcquireMutex(
    IN gckOS Os,
    IN gctPOINTER Mutex,
    IN gctUINT32 Timeout
    )
{
    gcmkHEADER_ARG("Os=0x%X Mutex=0x%0x Timeout=%u", Os, Mutex, Timeout);

    /* Validate the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Mutex != gcvNULL);

    if (Timeout == gcvINFINITE)
    {
        /* Lock the mutex. */
        pthread_mutex_lock((pthread_mutex_t*)Mutex);

        /* Success. */
        gcmkFOOTER_NO();
        return gcvSTATUS_OK;
    }

    for (;;)
    {
        /* Try to acquire the mutex. */
        if (pthread_mutex_trylock((pthread_mutex_t*)Mutex))
        {
            /* Success. */
            gcmkFOOTER_NO();
            return gcvSTATUS_OK;
        }

        if (Timeout-- == 0)
        {
            break;
        }

        /* Wait for 1 millisecond. */
        gcmkVERIFY_OK(gckOS_Delay(Os, 1));
    }

    /* Timeout. */
    gcmkFOOTER_ARG("status=%d", gcvSTATUS_TIMEOUT);
    return gcvSTATUS_TIMEOUT;
}

/*******************************************************************************
**
**  gckOS_ReleaseMutex
**
**  Release an acquired mutex.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Mutex
**          Pointer to the mutex to be released.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_ReleaseMutex(
    IN gckOS Os,
    IN gctPOINTER Mutex
    )
{
    gcmkHEADER_ARG("Os=0x%X Mutex=0x%0x", Os, Mutex);

    /* Validate the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Mutex != gcvNULL);

    /* Release the mutex. */
    pthread_mutex_unlock((pthread_mutex_t*)Mutex);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AtomicExchange
**
**  Atomically exchange a pair of 32-bit values.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      IN OUT gctINT32_PTR Target
**          Pointer to the 32-bit value to exchange.
**
**      IN gctINT32 NewValue
**          Specifies a new value for the 32-bit value pointed to by Target.
**
**      OUT gctINT32_PTR OldValue
**          The old value of the 32-bit value pointed to by Target.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_AtomicExchange(
    IN gckOS Os,
    IN OUT gctUINT32_PTR Target,
    IN gctUINT32 NewValue,
    OUT gctUINT32_PTR OldValue
    )
{
    /* Exchange the pair of 32-bit values. */
    *OldValue = (gctUINT32) vxAtomicGet((atomic_t *) Target);
    vxAtomicSet((atomic_t *) Target, NewValue);

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AtomicExchangePtr
**
**  Atomically exchange a pair of pointers.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      IN OUT gctPOINTER * Target
**          Pointer to the 32-bit value to exchange.
**
**      IN gctPOINTER NewValue
**          Specifies a new value for the pointer pointed to by Target.
**
**      OUT gctPOINTER * OldValue
**          The old value of the pointer pointed to by Target.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_AtomicExchangePtr(
    IN gckOS Os,
    IN OUT gctPOINTER * Target,
    IN gctPOINTER NewValue,
    OUT gctPOINTER * OldValue
    )
{
    /* Exchange the pair of pointers. */
    *OldValue = (gctPOINTER)(gctUINTPTR_T) vxAtomicGet((atomic_t *) Target);
    vxAtomicSet((atomic_t *) Target, (gctUINTPTR_T)NewValue);

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AtomicSetMask
**
**  Atomically set mask to Atom
**
**  INPUT:
**      IN OUT gctPOINTER Atom
**          Pointer to the atom to set.
**
**      IN gctUINT32 Mask
**          Mask to set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_AtomSetMask(
    IN gctPOINTER Atom,
    IN gctUINT32 Mask
    )
{
    gctUINT32 oval, nval;
    do
    {
        oval = (gctUINT32) vxAtomicGet((atomic_t *) Atom);
        nval = oval | Mask;
    }
    while (vxCas((atomic_t *) Atom, oval, nval) != 1);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AtomClearMask
**
**  Atomically clear mask from Atom
**
**  INPUT:
**      IN OUT gctPOINTER Atom
**          Pointer to the atom to clear.
**
**      IN gctUINT32 Mask
**          Mask to clear.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_AtomClearMask(
    IN gctPOINTER Atom,
    IN gctUINT32 Mask
    )
{
    gctUINT32 oval, nval;

    do
    {
        oval = (gctUINT32) vxAtomicGet((atomic_t *) Atom);
        nval = oval & ~Mask;
    }
    while (vxCas((atomic_t *) Atom, oval, nval) != 1);

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AtomConstruct
**
**  Create an atom.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**  OUTPUT:
**
**      gctPOINTER * Atom
**          Pointer to a variable receiving the constructed atom.
*/
gceSTATUS
gckOS_AtomConstruct(
    IN gckOS Os,
    OUT gctPOINTER * Atom
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=0x%X", Os);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Atom != gcvNULL);

    /* Allocate the atom. */
    gcmkONERROR(gckOS_Allocate(Os, gcmSIZEOF(atomic_t), Atom));

    /* Initialize the atom. */
    vxAtomicSet((atomic_t *) *Atom, 0);

    /* Success. */
    gcmkFOOTER_ARG("*Atom=0x%X", *Atom);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_AtomDestroy
**
**  Destroy an atom.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gctPOINTER Atom
**          Pointer to the atom to destroy.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_AtomDestroy(
    IN gckOS Os,
    OUT gctPOINTER Atom
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=0x%X Atom=0x%0x", Os, Atom);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Atom != gcvNULL);

    /* Free the atom. */
    gcmkONERROR(gcmkOS_SAFE_FREE(Os, Atom));

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_AtomGet
**
**  Get the 32-bit value protected by an atom.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gctPOINTER Atom
**          Pointer to the atom.
**
**  OUTPUT:
**
**      gctINT32_PTR Value
**          Pointer to a variable the receives the value of the atom.
*/
gceSTATUS
gckOS_AtomGet(
    IN gckOS Os,
    IN gctPOINTER Atom,
    OUT gctINT32_PTR Value
    )
{
    /* Return the current value of atom. */
    *Value = (gctINT32) vxAtomicGet((atomic_t *) Atom);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AtomSet
**
**  Set the 32-bit value protected by an atom.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gctPOINTER Atom
**          Pointer to the atom.
**
**      gctINT32 Value
**          The value of the atom.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_AtomSet(
    IN gckOS Os,
    IN gctPOINTER Atom,
    IN gctINT32 Value
    )
{
    /* Set the current value of atom. */
    vxAtomicSet((atomic_t *) Atom, Value);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AtomIncrement
**
**  Atomically increment the 32-bit integer value inside an atom.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gctPOINTER Atom
**          Pointer to the atom.
**
**  OUTPUT:
**
**      gctINT32_PTR Value
**          Pointer to a variable that receives the original value of the atom.
*/
gceSTATUS
gckOS_AtomIncrement(
    IN gckOS Os,
    IN gctPOINTER Atom,
    OUT gctINT32_PTR Value
    )
{
    *Value = (gctINT32) vxAtomicGet((atomic_t *)Atom);

    /* Increment the atom. */
    vxAtomicInc((atomic_t *) Atom);

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AtomDecrement
**
**  Atomically decrement the 32-bit integer value inside an atom.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gctPOINTER Atom
**          Pointer to the atom.
**
**  OUTPUT:
**
**      gctINT32_PTR Value
**          Pointer to a variable that receives the original value of the atom.
*/
gceSTATUS
gckOS_AtomDecrement(
    IN gckOS Os,
    IN gctPOINTER Atom,
    OUT gctINT32_PTR Value
    )
{
    *Value = (gctINT32) vxAtomicGet((atomic_t *)Atom);

    /* Decrement the atom. */
    vxAtomicDec((atomic_t *) Atom);

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_Delay
**
**  Delay execution of the current thread for a number of milliseconds.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctUINT32 Delay
**          Delay to sleep, specified in milliseconds.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_Delay(
    IN gckOS Os,
    IN gctUINT32 Delay
    )
{
    gctUINT32 tps = ticksPerSecond;
    gctUINT32 ticks = 0;

    gcmkHEADER_ARG("Os=0x%X Delay=%u", Os, Delay);

    if (Delay > 0)
    {
        tps = 1000 / tps;
        ticks = Delay / tps + 1;

        taskDelay(ticks);
    }

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_Udelay
**
**  Delay execution of the current thread for a number of microseconds.
**  Cannot access the precison in fact because of the limitation of taskDelay
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctUINT32 Delay
**          Delay to sleep, specified in microseconds.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_Udelay(
    IN gckOS Os,
    IN gctUINT32 Delay
    )
{
    gctUINT32 tps = ticksPerSecond;
    gctUINT32 ticks = 0;

    gcmkHEADER_ARG("Os=0x%X Delay=%u", Os, Delay);

    if (Delay > 0)
    {
        tps = 1000000 / tps;
        ticks = Delay / tps + 1;

        taskDelay(ticks);
    }

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_GetTicks
**
**  Get the number of milliseconds since the system started.
**
**  INPUT:
**
**  OUTPUT:
**
**      gctUINT32_PTR Time
**          Pointer to a variable to get time.
**
*/
gceSTATUS
gckOS_GetTicks(
    OUT gctUINT32_PTR Time
    )
{
     gcmkHEADER();

    *Time = tickGet() * 1000 / 60;

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_TicksAfter
**
**  Compare time values got from gckOS_GetTicks.
**
**  INPUT:
**      gctUINT32 Time1
**          First time value to be compared.
**
**      gctUINT32 Time2
**          Second time value to be compared.
**
**  OUTPUT:
**
**      gctBOOL_PTR IsAfter
**          Pointer to a variable to result.
**
*/
gceSTATUS
gckOS_TicksAfter(
    IN gctUINT32 Time1,
    IN gctUINT32 Time2,
    OUT gctBOOL_PTR IsAfter
    )
{
    gcmkHEADER();

    *IsAfter = (Time1 > Time2) ? 1 : 0;

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_GetTime
**
**  Get the number of microseconds since the system started.
**
**  INPUT:
**
**  OUTPUT:
**
**      gctUINT64_PTR Time
**          Pointer to a variable to get time.
**
*/

gceSTATUS
gckOS_GetTime(
    OUT gctUINT64_PTR Time
    )
{
    struct timespec tv;

    clock_gettime(CLOCK_REALTIME, &tv);

    *Time = (tv.tv_sec * 1000000) + (tv.tv_nsec + 500 / 1000);

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_MemoryBarrier
**
**  Make sure the CPU has executed everything up to this point and the data got
**  written to the specified pointer.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Address
**          Address of memory that needs to be barriered.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_MemoryBarrier(
    IN gckOS Os,
    IN gctPOINTER Address
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AllocatePagedMemory
**
**  Allocate memory from the paged pool.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctSIZE_T Bytes
**          Number of bytes to allocate.
**
**  OUTPUT:
**
**      gctPHYS_ADDR * Physical
**          Pointer to a variable that receives the physical address of the
**          memory allocation.
*/
gceSTATUS
gckOS_AllocatePagedMemory(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gctUINT32 Flag,
    IN gceVIDMEM_TYPE Type,
    IN OUT gctSIZE_T * Bytes,
    OUT gctUINT32 * Gid,
    OUT gctPHYS_ADDR * Physical
    )
{
    gctINT numPages;
    PVX_MDL mdl = gcvNULL;
    gctSIZE_T bytes;
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;
    gckALLOCATOR allocator;

    gcmkHEADER_ARG("Os=0x%X Flag=%x Bytes=%lu", Os, Flag, Bytes);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Bytes > 0);
    gcmkVERIFY_ARGUMENT(Physical != gcvNULL);

    bytes = gcmALIGN(*Bytes, PAGE_SIZE);

    numPages = GetPageCount(bytes, 0);

    mdl = _CreateMdl(Os);
    if (mdl == gcvNULL)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    /* Walk all allocators. */
    list_for_each_entry(allocator, &Os->allocatorList, link)
    {
        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS,
                       "%s(%d) flag = %x allocator->capability = %x",
                        __FUNCTION__, __LINE__, Flag, allocator->capability);

        if ((Flag & allocator->capability) != Flag)
        {
            continue;
        }

        status = allocator->ops->Alloc(allocator, mdl, numPages, Flag);

        if (gcmIS_SUCCESS(status))
        {
            mdl->allocator = allocator;
            break;
        }
    }

    /* Check status. */
    gcmkONERROR(status);

    mdl->addr       = 0;
    mdl->bytes      = bytes;
    mdl->numPages   = numPages;
    mdl->contiguous = Flag & gcvALLOC_FLAG_CONTIGUOUS;
    mdl->cacheable  = Flag & gcvALLOC_FLAG_CACHEABLE;

    /*
     * Add this to a global list.
     * Will be used by get physical address
     * and mapuser pointer functions.
     */
    pthread_mutex_lock(&Os->mdlMutex);
    list_add_tail(&mdl->link, &Os->mdlHead);
    pthread_mutex_unlock(&Os->mdlMutex);

    /* Return allocated bytes. */
    *Bytes = bytes;

    if (Gid != gcvNULL)
    {
        *Gid = mdl->gid;
    }

    /* Return physical address. */
    *Physical = (gctPHYS_ADDR) mdl;

    /* Success. */
    gcmkFOOTER_ARG("*Physical=0x%X", *Physical);
    return gcvSTATUS_OK;

OnError:
    if (mdl != gcvNULL)
    {
        /* Free the memory. */
        _DestroyMdl(mdl);
    }

    /* Return the status. */
    gcmkFOOTER_ARG("Os=0x%X Flag=%x Bytes=%lu", Os, Flag, Bytes);
    return status;
}

/*******************************************************************************
**
**  gckOS_FreePagedMemory
**
**  Free memory allocated from the paged pool.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPHYS_ADDR Physical
**          Physical address of the allocation.
**
**      gctSIZE_T Bytes
**          Number of bytes of the allocation.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_FreePagedMemory(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Bytes
    )
{
    PVX_MDL mdl = (PVX_MDL)Physical;

    gcmkHEADER_ARG("Os=0x%X Physical=0x%X Bytes=%lu", Os, Physical, Bytes);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmkVERIFY_ARGUMENT(Bytes > 0);

    /* Free the structure... */
    gcmkVERIFY_OK(_DestroyMdl(mdl));

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_LockPages
**
**  Lock memory allocated from the paged pool.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPHYS_ADDR Physical
**          Physical address of the allocation.
**
**      gctSIZE_T Bytes
**          Number of bytes of the allocation.
**
**      gctBOOL Cacheable
**          Cache mode of mapping.
**
**  OUTPUT:
**
**      gctPOINTER * Logical
**          Pointer to a variable that receives the address of the mapped
**          memory.
*/
gceSTATUS
gckOS_LockPages(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Bytes,
    IN gctBOOL Cacheable,
    OUT gctPOINTER * Logical
    )
{
    gceSTATUS       status;
    PVX_MDL         mdl;
    PVX_MDL_MAP     mdlMap;
    gckALLOCATOR    allocator;

    gcmkHEADER_ARG("Os=0x%X Physical=0x%X Bytes=%lu", Os, Physical, Logical);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmkVERIFY_ARGUMENT(Logical != gcvNULL);

    mdl = (PVX_MDL) Physical;
    allocator = mdl->allocator;

    pthread_mutex_lock(&mdl->mapsMutex);

    mdlMap = FindMdlMap(mdl, _GetProcessID());

    if (mdlMap == gcvNULL)
    {
        mdlMap = _CreateMdlMap(mdl, _GetProcessID());

        if (mdlMap == gcvNULL)
        {
            pthread_mutex_unlock(&mdl->mapsMutex);

            gcmkFOOTER_ARG("*status=%d", gcvSTATUS_OUT_OF_MEMORY);
            return gcvSTATUS_OUT_OF_MEMORY;
        }
    }

    if (mdlMap->vmaAddr == gcvNULL)
    {
        status = allocator->ops->MapUser(allocator, mdl, mdlMap, Cacheable);

        if (gcmIS_ERROR(status))
        {
            pthread_mutex_unlock(&mdl->mapsMutex);

            gcmkFOOTER_ARG("*status=%d", status);
            return status;
        }
    }

    mdlMap->count++;

    /* Convert pointer to MDL. */
    *Logical = mdlMap->vmaAddr;

    pthread_mutex_unlock(&mdl->mapsMutex);

    /* Success. */
    gcmkFOOTER_ARG("*Logical=0x%X *PageCount=%lu", *Logical, *PageCount);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_MapPages
**
**  Map paged memory into a page table.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPHYS_ADDR Physical
**          Physical address of the allocation.
**
**      gctSIZE_T PageCount
**          Number of pages required for the physical address.
**
**      gctPOINTER PageTable
**          Pointer to the page table to fill in.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_MapPages(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T PageCount,
    IN gctPOINTER PageTable
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gckOS_MapPagesEx(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gckMMU Mmu,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Offset,
    IN gctSIZE_T PageCount,
    IN gctADDRESS Address,
    IN gctPOINTER PageTable,
    IN gctBOOL Writable,
    IN gceVIDMEM_TYPE Type
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    PVX_MDL  mdl;
    gctUINT32*  table;
    gctUINT32   offset = 0;

    gctUINT32 bytes = PageCount * 4;
    gckALLOCATOR allocator;

    gctUINT32 policyID = 0;
    gctUINT32 axiConfig = 0;

    gcsPLATFORM * platform = Os->device->platform;

    gcmkHEADER_ARG("Os=0x%X Kernel=%p Physical=0x%X PageCount=%u PageTable=0x%X",
                   Os, Kernel, Physical, PageCount, PageTable);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmkVERIFY_ARGUMENT(PageCount > 0);
    gcmkVERIFY_ARGUMENT(PageTable != gcvNULL);

    /* Convert pointer to MDL. */
    mdl = (PVX_MDL)Physical;

    allocator = mdl->allocator;

    gcmkASSERT(allocator != gcvNULL);

    gcmkTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_OS,
        "%s(%d): Physical->0x%X PageCount->0x%X",
        __FUNCTION__, __LINE__,
        (gctUINT32)(gctUINTPTR_T)Physical,
        (gctUINT32)(gctUINTPTR_T)PageCount
        );

    table = (gctUINT32 *)PageTable;

    if (platform && platform->ops->getPolicyID)
    {
        platform->ops->getPolicyID(platform, Type, &policyID, &axiConfig);

        gcmkBUG_ON(policyID > 0x1F);

        /* ID[3:0] is used in STLB. */
        policyID &= 0xF;
    }

     /* Get all the physical addresses and store them in the page table. */

    PageCount = PageCount / (PAGE_SIZE / 4096);

    /* Try to get the user pages so DMA can happen. */
    while (PageCount-- > 0)
    {
        gctUINT i;
        gctPHYS_ADDR_T phys = ~0ULL;

        allocator->ops->Physical(allocator, mdl, offset, &phys);

        gcmkVERIFY_OK(gckOS_CPUPhysicalToGPUPhysical(Os, phys, &phys));

        if (policyID)
        {
            /* AxUSER must not used for address currently. */
            gcmkBUG_ON((phys >> 32) & 0xF);

            /* Merge policyID to AxUSER[7:4].*/
            phys |= ((gctPHYS_ADDR_T)policyID << 36);
        }

#ifdef CONFIG_IOMMU_SUPPORT
        if (Os->iommu)
        {
            /* remove LSB. */
            phys &= PAGE_MASK;

            gcmkTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_OS,
                "%s(%d): Setup mapping in IOMMU %x => %x",
                __FUNCTION__, __LINE__,
                Address + offset, phys
                );

            /* When use IOMMU, GPU use system PAGE_SIZE. */
            gcmkONERROR(gckIOMMU_Map(
                Os->iommu, Address + offset, phys, PAGE_SIZE));
        }
        else
#endif
        {
            /* remove LSB. */
            phys &= ~(4096ull - 1);

            {
                for (i = 0; i < (PAGE_SIZE / 4096); i++)
                {
                    gcmkONERROR(
                        gckMMU_SetPage(Kernel->mmu,
                            phys + (i * 4096),
                            gcvPAGE_TYPE_4K,
                            (Address < gcd4G_SIZE),
                            Writable,
                            table++));
                }
            }
        }

        offset += PAGE_SIZE;
    }

    {
        gckMMU mmu = Kernel->mmu;
        gcsADDRESS_AREA * area = &mmu->dynamicArea4K;

        offset = (gctUINT8_PTR)PageTable - (gctUINT8_PTR)area->stlbLogical;

        /* must be in dynamic area. */
        gcmkASSERT(offset < area->stlbSize);

        gcmkVERIFY_OK(gckVIDMEM_NODE_CleanCache(
            Kernel,
            area->stlbVideoMem,
            offset,
            PageTable,
            bytes
            ));

        if (mmu->mtlbVideoMem)
        {
            /* Flush MTLB table. */
            gcmkVERIFY_OK(gckVIDMEM_NODE_CleanCache(
                Kernel,
                mmu->mtlbVideoMem,
                offset,
                mmu->mtlbLogical,
                mmu->mtlbSize
                ));
        }
    }

OnError:

    /* Return the status. */
    gcmkFOOTER();
    return status;
}

gceSTATUS
gckOS_Map1MPages(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gckMMU Mmu,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T PageCount,
    IN gctADDRESS Address,
    IN gctPOINTER PageTable,
    IN gctBOOL Writable,
    IN gceVIDMEM_TYPE Type
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_UnmapPages(
    IN gckOS Os,
    IN gctSIZE_T PageCount,
    IN gctADDRESS Address
    )
{
#ifdef CONFIG_IOMMU_SUPPORT
    if (Os->iommu)
    {
        gcmkVERIFY_OK(gckIOMMU_Unmap(
            Os->iommu, Address, PageCount * PAGE_SIZE));
    }
#endif

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_UnlockPages
**
**  Unlock memory allocated from the paged pool.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPHYS_ADDR Physical
**          Physical address of the allocation.
**
**      gctSIZE_T Bytes
**          Number of bytes of the allocation.
**
**      gctPOINTER Logical
**          Address of the mapped memory.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_UnlockPages(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Bytes,
    IN gctPOINTER Logical
    )
{
    PVX_MDL_MAP mdlMap;
    PVX_MDL  mdl = (PVX_MDL)Physical;
    gckALLOCATOR allocator = mdl->allocator;
    gctINT pid = _GetProcessID();

    gcmkHEADER_ARG("Os=0x%X Physical=0x%X Bytes=%u Logical=0x%X",
                   Os, Physical, Bytes, Logical);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmkVERIFY_ARGUMENT(Logical != gcvNULL);

    pthread_mutex_lock(&mdl->mapsMutex);

    list_for_each_entry(mdlMap, &mdl->mapsHead, link)
    {
        if ((mdlMap->vmaAddr != gcvNULL) && (mdlMap->pid == pid))
        {
            if (--mdlMap->count == 0)
            {
                allocator->ops->UnmapUser(
                    allocator,
                    mdl,
                    mdlMap,
                    mdl->bytes);

                mdlMap->vmaAddr = gcvNULL;
            }
        }
    }

    pthread_mutex_unlock(&mdl->mapsMutex);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_MapUserPointer
**
**  Map a pointer from the user process into the kernel address space.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Pointer
**          Pointer in user process space that needs to be mapped.
**
**      gctSIZE_T Size
**          Number of bytes that need to be mapped.
**
**  OUTPUT:
**
**      gctPOINTER * KernelPointer
**          Pointer to a variable receiving the mapped pointer in kernel address
**          space.
*/
gceSTATUS
gckOS_MapUserPointer(
    IN gckOS Os,
    IN gctPOINTER Pointer,
    IN gctSIZE_T Size,
    OUT gctPOINTER * KernelPointer
    )
{
    gcmkHEADER_ARG("Os=0x%X Pointer=0x%X Size=%lu", Os, Pointer, Size);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Pointer != gcvNULL);
    gcmkVERIFY_ARGUMENT(Size > 0);
    gcmkVERIFY_ARGUMENT(KernelPointer != gcvNULL);

    *KernelPointer = Pointer;

    gcmkFOOTER_ARG("*KernelPointer=0x%X", *KernelPointer);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_UnmapUserPointer
**
**  Unmap a user process pointer from the kernel address space.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Pointer
**          Pointer in user process space that needs to be unmapped.
**
**      gctSIZE_T Size
**          Number of bytes that need to be unmapped.
**
**      gctPOINTER KernelPointer
**          Pointer in kernel address space that needs to be unmapped.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_UnmapUserPointer(
    IN gckOS Os,
    IN gctPOINTER Pointer,
    IN gctSIZE_T Size,
    IN gctPOINTER KernelPointer
    )
{
    gcmkHEADER_ARG("Os=0x%X Pointer=0x%X Size=%lu KernelPointer=0x%X",
                   Os, Pointer, Size, KernelPointer);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_QueryNeedCopy
**
**  Query whether the memory can be accessed or mapped directly or it has to be
**  copied.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctUINT32 ProcessID
**          Process ID of the current process.
**
**  OUTPUT:
**
**      gctBOOL_PTR NeedCopy
**          Pointer to a boolean receiving gcvTRUE if the memory needs a copy or
**          gcvFALSE if the memory can be accessed or mapped dircetly.
*/
gceSTATUS
gckOS_QueryNeedCopy(
    IN gckOS Os,
    IN gctUINT32 ProcessID,
    OUT gctBOOL_PTR NeedCopy
    )
{
    gcmkHEADER_ARG("Os=0x%X ProcessID=%d", Os, ProcessID);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(NeedCopy != gcvNULL);

    /* We need to copy data. */
    *NeedCopy = gcvTRUE;

    /* Success. */
    gcmkFOOTER_ARG("*NeedCopy=%d", *NeedCopy);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_CopyFromUserData
**
**  Copy data from user to kernel memory.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER KernelPointer
**          Pointer to kernel memory.
**
**      gctPOINTER Pointer
**          Pointer to user memory.
**
**      gctSIZE_T Size
**          Number of bytes to copy.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_CopyFromUserData(
    IN gckOS Os,
    IN gctPOINTER KernelPointer,
    IN gctPOINTER Pointer,
    IN gctSIZE_T Size
    )
{
    gcmkHEADER_ARG("Os=0x%X KernelPointer=0x%X Pointer=0x%X Size=%lu",
                   Os, KernelPointer, Pointer, Size);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(KernelPointer != gcvNULL);
    gcmkVERIFY_ARGUMENT(Pointer != gcvNULL);
    gcmkVERIFY_ARGUMENT(Size > 0);

    /* Copy data from user. */
    memcpy(KernelPointer, Pointer, Size);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_CopyToUserData
**
**  Copy data from kernel to user memory.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER KernelPointer
**          Pointer to kernel memory.
**
**      gctPOINTER Pointer
**          Pointer to user memory.
**
**      gctSIZE_T Size
**          Number of bytes to copy.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_CopyToUserData(
    IN gckOS Os,
    IN gctPOINTER KernelPointer,
    IN gctPOINTER Pointer,
    IN gctSIZE_T Size
    )
{
    gcmkHEADER_ARG("Os=0x%X KernelPointer=0x%X Pointer=0x%X Size=%lu",
                   Os, KernelPointer, Pointer, Size);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(KernelPointer != gcvNULL);
    gcmkVERIFY_ARGUMENT(Pointer != gcvNULL);
    gcmkVERIFY_ARGUMENT(Size > 0);

    /* Copy data to user. */
    memcpy(Pointer, KernelPointer, Size);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_WriteMemory
**
**  Write data to a memory.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctPOINTER Address
**          Address of the memory to write to.
**
**      gctUINT32 Data
**          Data for register.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_WriteMemory(
    IN gckOS Os,
    IN gctPOINTER Address,
    IN gctUINT32 Data
    )
{
    gcmkHEADER_ARG("Os=0x%X Address=0x%X Data=%u", Os, Address, Data);

    /* Verify the arguments. */
    gcmkVERIFY_ARGUMENT(Address != gcvNULL);

    /* Kernel address. */
    *(gctUINT32 *)Address = Data;

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_ReadMappedPointer(
    IN gckOS Os,
    IN gctPOINTER Address,
    IN gctUINT32_PTR Data
    )
{
    gcmkHEADER_ARG("Os=0x%X Address=0x%X Data=%u", Os, Address, Data);

    /* Verify the arguments. */
    gcmkVERIFY_ARGUMENT(Address != gcvNULL);

    /* Kernel address. */
    *Data = *(gctUINT32_PTR)Address;

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_GetBaseAddress
**
**  Get the base address for the physical memory.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**  OUTPUT:
**
**      gctUINT32_PTR BaseAddress
**          Pointer to a variable that will receive the base address.
*/
gceSTATUS
gckOS_GetBaseAddress(
    IN gckOS Os,
    OUT gctUINT32_PTR BaseAddress
    )
{
    gcmkHEADER_ARG("Os=0x%X", Os);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(BaseAddress != gcvNULL);

    /* Return base address. */
    *BaseAddress = Os->device->baseAddress;

    /* Success. */
    gcmkFOOTER_ARG("*BaseAddress=0x%08x", *BaseAddress);
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_SuspendInterrupt(
    IN gckOS Os
    )
{
    return gckOS_SuspendInterruptEx(Os, gcvCORE_MAJOR);
}

gceSTATUS
gckOS_SuspendInterruptEx(
    IN gckOS Os,
    IN gceCORE Core
    )
{
    gcmkHEADER_ARG("Os=0x%X Core=%d", Os, Core);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);

    intDisable(Os->device->irqLines[Core]);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_ResumeInterrupt(
    IN gckOS Os
    )
{
    return gckOS_ResumeInterruptEx(Os, gcvCORE_MAJOR);
}

gceSTATUS
gckOS_ResumeInterruptEx(
    IN gckOS Os,
    IN gceCORE Core
    )
{
    gcmkHEADER_ARG("Os=0x%X Core=%d", Os, Core);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);

    intEnable(Os->device->irqLines[Core]);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_MemCopy(
    IN gctPOINTER Destination,
    IN gctCONST_POINTER Source,
    IN gctSIZE_T Bytes
    )
{
    gcmkHEADER_ARG("Destination=0x%X Source=0x%X Bytes=%lu",
                   Destination, Source, Bytes);

    gcmkVERIFY_ARGUMENT(Destination != gcvNULL);
    gcmkVERIFY_ARGUMENT(Source != gcvNULL);
    gcmkVERIFY_ARGUMENT(Bytes > 0);

    memcpy(Destination, Source, Bytes);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_ZeroMemory(
    IN gctPOINTER Memory,
    IN gctSIZE_T Bytes
    )
{
    gcmkHEADER_ARG("Memory=0x%X Bytes=%lu", Memory, Bytes);

    gcmkVERIFY_ARGUMENT(Memory != gcvNULL);
    gcmkVERIFY_ARGUMENT(Bytes > 0);

    memset(Memory, 0, Bytes);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
********************************* Cache Control ********************************
*******************************************************************************/
static gceSTATUS
_CacheOperation(
    IN gckOS Os,
    IN gctUINT32 ProcessID,
    IN gctPHYS_ADDR Handle,
    IN gctSIZE_T Offset,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    PVX_MDL mdl = (PVX_MDL)Handle;
    PVX_MDL_MAP mdlMap;
    gckALLOCATOR allocator;

    if (!mdl || !mdl->allocator)
    {
        gcmkPRINT("[galcore]: %s: Logical=%p no mdl", __FUNCTION__, Logical);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    allocator = mdl->allocator;

    if (allocator->ops->Cache)
    {
        pthread_mutex_lock(&mdl->mapsMutex);

        mdlMap = FindMdlMap(mdl, ProcessID);

        pthread_mutex_unlock(&mdl->mapsMutex);

        if (ProcessID && mdlMap == gcvNULL)
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        if ((!ProcessID && mdl->cacheable) ||
            (mdlMap && mdlMap->cacheable))
        {
            allocator->ops->Cache(allocator,
                mdl, Offset, Logical, Bytes, Operation);

            return gcvSTATUS_OK;
        }
    }

    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gckOS_CacheClean
**
**  Clean the cache for the specified addresses.  The GPU is going to need the
**  data.  If the system is allocating memory as non-cachable, this function can
**  be ignored.
**
**  ARGUMENTS:
**
**      gckOS Os
**          Pointer to gckOS object.
**
**      gctUINT32 ProcessID
**          Process ID Logical belongs.
**
**      gctPHYS_ADDR Handle
**          Physical address handle.  If gcvNULL it is video memory.
**
**      gctSIZE_T Offset
**          Offset to this memory block.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
**      gctSIZE_T Bytes
**          Size of the address range in bytes to flush.
*/

/*

Following patch can be applied to kernel in case cache API is not exported.

diff --git a/arch/arm/mm/proc-syms.c b/arch/arm/mm/proc-syms.c
index 054b491..e9e74ec 100644
--- a/arch/arm/mm/proc-syms.c
+++ b/arch/arm/mm/proc-syms.c
@@ -30,6 +30,9 @@ EXPORT_SYMBOL(__cpuc_flush_user_all);
 EXPORT_SYMBOL(__cpuc_flush_user_range);
 EXPORT_SYMBOL(__cpuc_coherent_kern_range);
 EXPORT_SYMBOL(__cpuc_flush_dcache_area);
+EXPORT_SYMBOL(__glue(_CACHE,_dma_map_area));
+EXPORT_SYMBOL(__glue(_CACHE,_dma_unmap_area));
+EXPORT_SYMBOL(__glue(_CACHE,_dma_flush_range));
 #else
 EXPORT_SYMBOL(cpu_cache);
 #endif

*/
gceSTATUS
gckOS_CacheClean(
    IN gckOS Os,
    IN gctUINT32 ProcessID,
    IN gctPHYS_ADDR Handle,
    IN gctSIZE_T Offset,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=%p ProcessID=%d Handle=%p Offset=0x%llx Logical=%p Bytes=0x%zx",
                   Os, ProcessID, Handle, Offset, Logical, Bytes);

    gcmkONERROR(_CacheOperation(Os, ProcessID,
                                Handle, Offset, Logical, Bytes,
                                gcvCACHE_CLEAN));

OnError:
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**  gckOS_CacheInvalidate
**
**  Invalidate the cache for the specified addresses. The GPU is going to need
**  data.  If the system is allocating memory as non-cachable, this function can
**  be ignored.
**
**  ARGUMENTS:
**
**      gckOS Os
**          Pointer to gckOS object.
**
**      gctUINT32 ProcessID
**          Process ID Logical belongs.
**
**      gctPHYS_ADDR Handle
**          Physical address handle.  If gcvNULL it is video memory.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
**      gctSIZE_T Bytes
**          Size of the address range in bytes to flush.
*/
gceSTATUS
gckOS_CacheInvalidate(
    IN gckOS Os,
    IN gctUINT32 ProcessID,
    IN gctPHYS_ADDR Handle,
    IN gctSIZE_T Offset,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=%p ProcessID=%d Handle=%p Offset=0x%llx Logical=%p Bytes=0x%zx",
                   Os, ProcessID, Handle, Offset, Logical, Bytes);

    gcmkONERROR(_CacheOperation(Os, ProcessID,
                                Handle, Offset, Logical, Bytes,
                                gcvCACHE_INVALIDATE));

OnError:
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**  gckOS_CacheFlush
**
**  Clean the cache for the specified addresses and invalidate the lines as
**  well.  The GPU is going to need and modify the data.  If the system is
**  allocating memory as non-cachable, this function can be ignored.
**
**  ARGUMENTS:
**
**      gckOS Os
**          Pointer to gckOS object.
**
**      gctUINT32 ProcessID
**          Process ID Logical belongs.
**
**      gctPHYS_ADDR Handle
**          Physical address handle.  If gcvNULL it is video memory.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
**      gctSIZE_T Bytes
**          Size of the address range in bytes to flush.
*/
gceSTATUS
gckOS_CacheFlush(
    IN gckOS Os,
    IN gctUINT32 ProcessID,
    IN gctPHYS_ADDR Handle,
    IN gctSIZE_T Offset,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=%p ProcessID=%d Handle=%p Offset=0x%llx Logical=%p Bytes=0x%zx",
                   Os, ProcessID, Handle, Offset, Logical, Bytes);

    gcmkONERROR(_CacheOperation(Os, ProcessID,
                                Handle, Offset, Logical, Bytes,
                                gcvCACHE_FLUSH));

OnError:
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
********************************* Broadcasting *********************************
*******************************************************************************/

/*******************************************************************************
**
**  gckOS_Broadcast
**
**  System hook for broadcast events from the kernel driver.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gckHARDWARE Hardware
**          Pointer to the gckHARDWARE object.
**
**      gceBROADCAST Reason
**          Reason for the broadcast.  Can be one of the following values:
**
**              gcvBROADCAST_GPU_IDLE
**                  Broadcasted when the kernel driver thinks the GPU might be
**                  idle.  This can be used to handle power management.
**
**              gcvBROADCAST_GPU_COMMIT
**                  Broadcasted when any client process commits a command
**                  buffer.  This can be used to handle power management.
**
**              gcvBROADCAST_GPU_STUCK
**                  Broadcasted when the kernel driver hits the timeout waiting
**                  for the GPU.
**
**              gcvBROADCAST_FIRST_PROCESS
**                  First process is trying to connect to the kernel.
**
**              gcvBROADCAST_LAST_PROCESS
**                  Last process has detached from the kernel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_Broadcast(
    IN gckOS Os,
    IN gckHARDWARE Hardware,
    IN gceBROADCAST Reason
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gceCHIPPOWERSTATE state;

    gcmkHEADER_ARG("Os=%p Hardware=%p Reason=%d", Os, Hardware, Reason);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    switch (Reason)
    {
    case gcvBROADCAST_FIRST_PROCESS:
        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS, "First process has attached");
        break;

    case gcvBROADCAST_LAST_PROCESS:
        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS, "Last process has detached");

        /* Put GPU OFF. */
        gcmkONERROR(
            gckHARDWARE_SetPowerState(Hardware,
                                      gcvPOWER_OFF_BROADCAST));
        break;

    case gcvBROADCAST_GPU_IDLE:
        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS, "GPU idle.");
#if gcdPOWER_SUSPEND_WHEN_IDLE
        state = gcvPOWER_SUSPEND_BROADCAST;
#else
        state = gcvPOWER_IDLE_BROADCAST;
#endif

        /* Put GPU IDLE or SUSPEND. */
        gcmkONERROR(
            gckHARDWARE_SetPowerState(Hardware, state));

        /* Add idle process DB. */
        gcmkONERROR(gckKERNEL_AddProcessDB(Hardware->kernel,
                                           1,
                                           gcvDB_IDLE,
                                           gcvNULL, gcvNULL, 0));
        break;

    case gcvBROADCAST_GPU_COMMIT:
        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS, "COMMIT has arrived.");

        /* Add busy process DB. */
        gcmkONERROR(gckKERNEL_AddProcessDB(Hardware->kernel,
                                           0,
                                           gcvDB_IDLE,
                                           gcvNULL, gcvNULL, 0));

        /* Put GPU ON. */
        gcmkONERROR(
            gckHARDWARE_SetPowerState(Hardware, gcvPOWER_ON_AUTO));
        break;

    case gcvBROADCAST_GPU_STUCK:
        gcmkTRACE_N(gcvLEVEL_ERROR, 0, "gcvBROADCAST_GPU_STUCK\n");
        gcmkONERROR(gckKERNEL_Recovery(Hardware->kernel));
        break;

    case gcvBROADCAST_AXI_BUS_ERROR:
        gcmkTRACE_N(gcvLEVEL_ERROR, 0, "gcvBROADCAST_AXI_BUS_ERROR\n");
        gcmkONERROR(gckHARDWARE_DumpGPUState(Hardware));
        gcmkONERROR(gckKERNEL_Recovery(Hardware->kernel));
        break;

    case gcvBROADCAST_OUT_OF_MEMORY:
        gcmkTRACE_N(gcvLEVEL_INFO, 0, "gcvBROADCAST_OUT_OF_MEMORY\n");

        status = _ShrinkMemory(Os);

        if (status == gcvSTATUS_NOT_SUPPORTED)
        {
            goto OnError;
        }

        gcmkONERROR(status);

        break;

    default:
        /* Skip unimplemented broadcast. */
        break;
    }

OnError:
    /* Return the status. */
    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_BroadcastHurry
**
**  The GPU is running too slow.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gckHARDWARE Hardware
**          Pointer to the gckHARDWARE object.
**
**      gctUINT Urgency
**          The higher the number, the higher the urgency to speed up the GPU.
**          The maximum value is defined by the gcdDYNAMIC_EVENT_THRESHOLD.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_BroadcastHurry(
    IN gckOS Os,
    IN gckHARDWARE Hardware,
    IN gctUINT Urgency
    )
{
    gcmkHEADER_ARG("Os=0x%x Hardware=0x%x Urgency=%u", Os, Hardware, Urgency);

    /* Do whatever you need to do to speed up the GPU now. */

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_BroadcastCalibrateSpeed
**
**  Calibrate the speed of the GPU.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gckHARDWARE Hardware
**          Pointer to the gckHARDWARE object.
**
**      gctUINT Idle, Time
**          Idle/Time will give the percentage the GPU is idle, so you can use
**          this to calibrate the working point of the GPU.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_BroadcastCalibrateSpeed(
    IN gckOS Os,
    IN gckHARDWARE Hardware,
    IN gctUINT Idle,
    IN gctUINT Time
    )
{
    gcmkHEADER_ARG("Os=0x%x Hardware=0x%x Idle=%u Time=%u",
                   Os, Hardware, Idle, Time);

    /* Do whatever you need to do to callibrate the GPU speed. */

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
********************************** Semaphores **********************************
*******************************************************************************/

/*******************************************************************************
**
**  gckOS_CreateSemaphore
**
**  Create a semaphore.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**  OUTPUT:
**
**      gctPOINTER * Semaphore
**          Pointer to the variable that will receive the created semaphore.
*/
gceSTATUS
gckOS_CreateSemaphore(
    IN gckOS Os,
    OUT gctPOINTER * Semaphore
    )
{
    SEM_ID sem;

    gcmkHEADER_ARG("Os=0x%X", Os);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Semaphore != gcvNULL);

    /* Initialize the semaphore. */
    sem = semBCreate(SEM_Q_FIFO, 1);

    /* Return to caller. */
    *Semaphore = (gctPOINTER) sem;

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_CreateSemaphoreEx(
    IN gckOS Os,
    OUT gctPOINTER * Semaphore
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_AcquireSemaphore
**
**  Acquire a semaphore.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gctPOINTER Semaphore
**          Pointer to the semaphore thet needs to be acquired.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_AcquireSemaphore(
    IN gckOS Os,
    IN gctPOINTER Semaphore
    )
{
    gcmkHEADER_ARG("Os=0x%08X Semaphore=0x%08X", Os, Semaphore);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Semaphore != gcvNULL);

    /* Acquire the semaphore. */
    semTake(Semaphore,WAIT_FOREVER);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_TryAcquireSemaphore
**
**  Try to acquire a semaphore.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gctPOINTER Semaphore
**          Pointer to the semaphore thet needs to be acquired.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_TryAcquireSemaphore(
    IN gckOS Os,
    IN gctPOINTER Semaphore
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=0x%x", Os);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Semaphore != gcvNULL);

    /* Acquire the semaphore. */
    if (semTake(Semaphore, NO_WAIT))
    {
        /* Timeout. */
        status = gcvSTATUS_TIMEOUT;
        gcmkFOOTER();
        return status;
    }

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_ReleaseSemaphore
**
**  Release a previously acquired semaphore.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gctPOINTER Semaphore
**          Pointer to the semaphore thet needs to be released.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_ReleaseSemaphore(
    IN gckOS Os,
    IN gctPOINTER Semaphore
    )
{
    gcmkHEADER_ARG("Os=0x%X Semaphore=0x%X", Os, Semaphore);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Semaphore != gcvNULL);

    /* Release the semaphore. */
    semGive(Semaphore);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_ReleaseSemaphoreEx(
    IN gckOS Os,
    IN gctPOINTER Semaphore
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_DestroySemaphore
**
**  Destroy a semaphore.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gctPOINTER Semaphore
**          Pointer to the semaphore thet needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_DestroySemaphore(
    IN gckOS Os,
    IN gctPOINTER Semaphore
    )
{
    gcmkHEADER_ARG("Os=0x%X Semaphore=0x%X", Os, Semaphore);

     /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Semaphore != gcvNULL);

    /* Free the sempahore structure. */
    semDelete(Semaphore);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_GetProcessID
**
**  Get current process ID.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      gctUINT32_PTR ProcessID
**          Pointer to the variable that receives the process ID.
*/
gceSTATUS
gckOS_GetProcessID(
    OUT gctUINT32_PTR ProcessID
    )
{
    /* Get process ID. */
    *ProcessID = _GetProcessID();

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_GetThreadID
**
**  Get current thread ID.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      gctUINT32_PTR ThreadID
**          Pointer to the variable that receives the thread ID.
*/
gceSTATUS
gckOS_GetThreadID(
    OUT gctUINT32_PTR ThreadID
    )
{
    /* Get thread ID. */
    if (ThreadID != gcvNULL)
    {
        *ThreadID = _GetThreadID();
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_SetClockState
**
**  Set the clock state on or off.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gckKERNEL Kernel
**          GPU whose power is set.
**
**      gctBOOL Clock
**          gcvTRUE to turn on the clock, or gcvFALSE to turn off the clock.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_SetClockState(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gctBOOL Clock
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_GetClockState
**
**  Get the clock state on or off.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gckKERNEL Kernel
**          GPU whose power is set.
**
**      gctBOOL Clock
**          gcvTRUE to turn on the clock, or gcvFALSE to turn off the clock.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_GetClockState(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gctBOOL * Clock
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_SetGPUPower
**
**  Set the power of the GPU on or off.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gckKERNEL Kernel
**          GPU whose power is set.
**
**      gctBOOL Clock
**          gcvTRUE to turn on the clock, or gcvFALSE to turn off the clock.
**
**      gctBOOL Power
**          gcvTRUE to turn on the power, or gcvFALSE to turn off the power.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_SetGPUPower(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gctBOOL Clock,
    IN gctBOOL Power
    )
{
    gcsPLATFORM * platform;

    gctBOOL powerChange = gcvFALSE;
    gctBOOL clockChange = gcvFALSE;

    gcmkHEADER_ARG("Os=0x%X Kernel=%p Clock=%d Power=%d", Os, Kernel, Clock, Power);
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);

    platform = Os->device->platform;

    powerChange = (Power != Os->powerStates[Kernel->core]);

    clockChange = (Clock != Os->clockStates[Kernel->core]);

    if (powerChange && (Power == gcvTRUE))
    {
        if (platform && platform->ops->setPower)
        {
            gcmkVERIFY_OK(platform->ops->setPower(platform, Kernel->core, Power));
        }

        Os->powerStates[Kernel->core] = Power;
    }

    if (clockChange)
    {
        if (!Clock)
        {
            spinLockIsrTake(&Os->registerAccessLock);

            /* Record clock off, ahead. */
            Os->clockStates[Kernel->core] = gcvFALSE;

            spinLockIsrGive(&Os->registerAccessLock);
        }

        if (platform && platform->ops->setClock)
        {
            gcmkVERIFY_OK(platform->ops->setClock(platform, Kernel->core, Clock));
        }

        if (Clock)
        {
            spinLockIsrTake(&Os->registerAccessLock);

            /* Record clock on, behind. */
            Os->clockStates[Kernel->core] = gcvTRUE;

            spinLockIsrGive(&Os->registerAccessLock);
        }
    }

    if (powerChange && (Power == gcvFALSE))
    {
        if (platform && platform->ops->setPower)
        {
            gcmkVERIFY_OK(platform->ops->setPower(platform, Kernel->core, Power));
        }

        Os->powerStates[Kernel->core] = Power;
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_ResetGPU
**
**  Reset the GPU.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gckKERNEL Kernel
**          GPU to reset.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_ResetGPU(
    IN gckOS Os,
    IN gckKERNEL Kernel
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;
    gcsPLATFORM * platform;

    gcmkHEADER_ARG("Os=0x%X Kernel=%p", Os, Kernel);
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);

    platform = Os->device->platform;

    if (platform && platform->ops->reset)
    {
        status = platform->ops->reset(platform, Kernel->core);
    }

    gcmkFOOTER_NO();
    return status;
}

/*******************************************************************************
**
**  gckOS_PrepareGPUFrequency
**
**  Prepare to set GPU frequency and voltage.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gckCORE Core
**          GPU whose frequency and voltage will be set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_PrepareGPUFrequency(
    IN gckOS Os,
    IN gceCORE Core
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_FinishGPUFrequency
**
**  Finish GPU frequency setting.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gckCORE Core
**          GPU whose frequency and voltage is set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_FinishGPUFrequency(
    IN gckOS Os,
    IN gceCORE Core
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_QueryGPUFrequency
**
**  Query the current frequency of the GPU.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gckCORE Core
**          GPU whose power is set.
**
**      gctUINT32 * Frequency
**          Pointer to a gctUINT32 to obtain current frequency, in MHz.
**
**      gctUINT8 * Scale
**          Pointer to a gctUINT8 to obtain current scale(1 - 64).
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_QueryGPUFrequency(
    IN gckOS Os,
    IN gceCORE Core,
    OUT gctUINT32 * Frequency,
    OUT gctUINT8 * Scale
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_SetGPUFrequency
**
**  Set frequency and voltage of the GPU.
**
**      1. DVFS manager gives the target scale of full frequency, BSP must find
**         a real frequency according to this scale and board's configure.
**
**      2. BSP should find a suitable voltage for this frequency.
**
**      3. BSP must make sure setting take effect before this function returns.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to a gckOS object.
**
**      gckCORE Core
**          GPU whose power is set.
**
**      gctUINT8 Scale
**          Target scale of full frequency, range is [1, 64]. 1 means 1/64 of
**          full frequency and 64 means 64/64 of full frequency.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_SetGPUFrequency(
    IN gckOS Os,
    IN gceCORE Core,
    IN gctUINT8 Scale
    )
{
    return gcvSTATUS_OK;
}

/*----------------------------------------------------------------------------*/
/*----- Profile --------------------------------------------------------------*/

gceSTATUS
gckOS_GetProfileTick(
    OUT gctUINT64_PTR Tick
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_QueryProfileTickRate(
    OUT gctUINT64_PTR TickRate
    )
{
    return gcvSTATUS_OK;
}

gctUINT32
gckOS_ProfileToMS(
    IN gctUINT64 Ticks
    )
{
    return gcvSTATUS_OK;
}

/******************************************************************************\
******************************* Signal Management ******************************
\******************************************************************************/

#undef _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    gcvZONE_SIGNAL

/*******************************************************************************
**
**  gckOS_CreateSignal
**
**  Create a new signal.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctBOOL ManualReset
**          If set to gcvTRUE, gckOS_Signal with gcvFALSE must be called in
**          order to set the signal to nonsignaled state.
**          If set to gcvFALSE, the signal will automatically be set to
**          nonsignaled state by gckOS_WaitSignal function.
**
**  OUTPUT:
**
**      gctSIGNAL * Signal
**          Pointer to a variable receiving the created gctSIGNAL.
*/
gceSTATUS
gckOS_CreateSignal(
    IN gckOS Os,
    IN gctBOOL ManualReset,
    OUT gctSIGNAL * Signal
    )
{
    gceSTATUS status;
    gcsSIGNAL_PTR signal;

    gcmkHEADER_ARG("Os=0x%X ManualReset=%d", Os, ManualReset);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Signal != gcvNULL);

    /* Create an event structure. */
    signal = (gcsSIGNAL_PTR)malloc(sizeof(gcsSIGNAL));

    if (signal == gcvNULL)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    /* Save the process ID. */
    signal->process = (gctHANDLE)(gctUINTPTR_T) _GetProcessID();

    signal->done = 0;
    signal->sem = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

    spinLockTaskInit(&signal->lock, 0);

    signal->manualReset = ManualReset;

    vxAtomicSet(&signal->ref, 1);

    *Signal = (gctSIGNAL)signal;

    gcmkFOOTER_ARG("*Signal=0x%X", *Signal);
    return gcvSTATUS_OK;

OnError:
    if (signal != gcvNULL)
    {
        free(signal);
    }

    gcmkFOOTER_NO();
    return status;
}

/*******************************************************************************
**
**  gckOS_DestroySignal
**
**  Destroy a signal.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_DestroySignal(
    IN gckOS Os,
    IN gctSIGNAL Signal
    )
{
    gcsSIGNAL_PTR signal;
    gctBOOL acquired = gcvFALSE;

    gcmkHEADER_ARG("Os=0x%X Signal=0x%X", Os, Signal);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Signal != gcvNULL);

    pthread_mutex_lock(&Os->signalMutex);
    acquired = gcvTRUE;

    signal = Signal;

    vxAtomicDec(&signal->ref);
    if ((gctINT)vxAtomicGet(&signal->ref) == 0)
    {
        /* Free the sgianl. */
        semDelete(signal->sem);
        free(signal);
    }

    pthread_mutex_unlock(&Os->signalMutex);
    acquired = gcvFALSE;

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_Signal
**
**  Set a state of the specified signal.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**      gctBOOL State
**          If gcvTRUE, the signal will be set to signaled state.
**          If gcvFALSE, the signal will be set to nonsignaled state.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_Signal(
    IN gckOS Os,
    IN gctSIGNAL Signal,
    IN gctBOOL State
    )
{
    gcsSIGNAL_PTR signal;

    gcmkHEADER_ARG("Os=0x%X Signal=0x%X State=%d", Os, Signal, State);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Signal != gcvNULL);

    pthread_mutex_lock(&Os->signalMutex);

    signal = Signal;

    /*
     * Signal saved in event is not referenced. Inc reference here to avoid
     * concurrent issue: signaling the signal while another thread is destroying
     * it.
     */
    vxAtomicInc(&signal->ref);

    pthread_mutex_unlock(&Os->signalMutex);

    spinLockTaskTake(&signal->lock);

    if (State)
    {
        signal->done = 1;
        semGive(signal->sem);
    }
    else
    {
        signal->done = 0;
    }

    spinLockTaskGive(&signal->lock);

    pthread_mutex_lock(&Os->signalMutex);

    vxAtomicDec(&signal->ref);

    if ((gctINT) vxAtomicGet(&signal->ref) == 0)
    {
        /* Free the sgianl. */
        semDelete(signal->sem);
        free(signal);
    }

    pthread_mutex_unlock(&Os->signalMutex);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_UserSignal
**
**  Set the specified signal which is owned by a process to signaled state.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**      gctHANDLE Process
**          Handle of process owning the signal.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_UserSignal(
    IN gckOS Os,
    IN gctSIGNAL Signal,
    IN gctHANDLE Process
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Os=0x%X Signal=0x%X Process=%d",
                   Os, Signal, (gctINT32)(gctUINTPTR_T)Process);

    /* Signal. */
    status = gckOS_Signal(Os, Signal, gcvTRUE);

    gcmkFOOTER();
    return status;
}

/*******************************************************************************
**
**  gckOS_WaitSignal
**
**  Wait for a signal to become signaled.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**      gctUINT32 Wait
**          Number of milliseconds to wait.
**          Pass the value of gcvINFINITE for an infinite wait.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_WaitSignal(
    IN gckOS Os,
    IN gctSIGNAL Signal,
    IN gctBOOL Interruptable,
    IN gctUINT32 Wait
    )
{
    gceSTATUS status;
    gcsSIGNAL_PTR signal;
    int done;

    gcmkHEADER_ARG("Os=0x%X Signal=0x%X Wait=0x%08X", Os, Signal, Wait);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Signal != gcvNULL);

    signal = Signal;

    spinLockTaskTake(&signal->lock);
    done = signal->done;

    /*
     * Do not need to lock below:
     * 1. If signal already done, return immediately.
     * 2. If signal not done, wait_event_xxx will handle correctly even read of
     *    signal->done is not atomic.
     *
     * Rest signal->done do not require lock either:
     * No other thread can query/wait auto-reseted signal, because that is
     * logic error.
     */
    if (done)
    {
        status = gcvSTATUS_OK;

        if (!signal->manualReset)
        {
            signal->done = 0;
        }

        spinLockTaskGive(&signal->lock);
    }
    else if (Wait == 0)
    {
        status = gcvSTATUS_TIMEOUT;
        spinLockTaskGive(&signal->lock);
    }
    else
    {
        /* Convert wait to milliseconds. */
        int timeout = (Wait == gcvINFINITE)
                     ? WAIT_FOREVER
                     : Wait;

        int ret;

        spinLockTaskGive(&signal->lock);
        while (!signal->done)
        {
            int wait = (timeout >= 1 || timeout < 0) ? 1 : timeout;

            ret = semTake(signal->sem, wait);

            if (timeout >= 0)
            {
                timeout -= wait;

                if (timeout == 0)
                {
                     break;
                }
            }
        }

        if (signal->done)
        {
            status = gcvSTATUS_OK;

            if (!signal->manualReset)
            {
                /* Auto reset. */
                signal->done = 0;
            }
        }
        else
        {
            status = gcvSTATUS_TIMEOUT;
        }
    }

    /* Return status. */
    gcmkFOOTER_ARG("Signal=0x%lX status=%d", Signal, status);
    return status;
}

gceSTATUS
_QuerySignal(
    IN gckOS Os,
    IN gctSIGNAL Signal
    )
{
    gceSTATUS status;
    gcsSIGNAL_PTR signal = Signal;

    spinLockTaskTake(&signal->lock);
    status = signal->done ? gcvSTATUS_TRUE : gcvSTATUS_FALSE;
    spinLockTaskGive(&signal->lock);

    return status;
}

/*******************************************************************************
**
**  gckOS_MapSignal
**
**  Map a signal in to the current process space.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctSIGNAL Signal
**          Pointer to tha gctSIGNAL to map.
**
**      gctHANDLE Process
**          Handle of process owning the signal.
**
**  OUTPUT:
**
**      gctSIGNAL * MappedSignal
**          Pointer to a variable receiving the mapped gctSIGNAL.
*/
gceSTATUS
gckOS_MapSignal(
    IN gckOS Os,
    IN gctSIGNAL Signal,
    IN gctHANDLE Process,
    OUT gctSIGNAL * MappedSignal
    )
{
    gcsSIGNAL_PTR signal = gcvNULL;
    gcmkHEADER_ARG("Os=0x%X Signal=0x%X Process=0x%X", Os, Signal, Process);

    gcmkVERIFY_ARGUMENT(Signal != gcvNULL);
    gcmkVERIFY_ARGUMENT(MappedSignal != gcvNULL);

    pthread_mutex_lock(&Os->signalMutex);
    signal = Signal;

    vxAtomicInc(&signal->ref);

    if ((gctINT) vxAtomicGet(&signal->ref) <= 1)
    {
        /* The previous value is 0, it has been deleted. */
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    *MappedSignal = (gctSIGNAL)Signal;

    pthread_mutex_unlock(&Os->signalMutex);

    /* Success. */
    gcmkFOOTER_ARG("*MappedSignal=0x%X", *MappedSignal);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_UnmapSignal
**
**  Unmap a signal .
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctSIGNAL Signal
**          Pointer to that gctSIGNAL mapped.
*/
gceSTATUS
gckOS_UnmapSignal(
    IN gckOS Os,
    IN gctSIGNAL Signal
    )
{
    return gckOS_DestroySignal(Os, Signal);
}

/*******************************************************************************
**
**  gckOS_CreateUserSignal
**
**  Create a new signal to be used in the user space.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctBOOL ManualReset
**          If set to gcvTRUE, gckOS_Signal with gcvFALSE must be called in
**          order to set the signal to nonsignaled state.
**          If set to gcvFALSE, the signal will automatically be set to
**          nonsignaled state by gckOS_WaitSignal function.
**
**  OUTPUT:
**
**      gctINT * SignalID
**          Pointer to a variable receiving the created signal's ID.
*/
gceSTATUS
gckOS_CreateUserSignal(
    IN gckOS Os,
    IN gctBOOL ManualReset,
    OUT gctINT * SignalID
    )
{
    gceSTATUS status;
    gctSIZE_T signal;

    /* Create a new signal. */
    gcmkONERROR(gckOS_CreateSignal(Os, ManualReset, (gctSIGNAL *) &signal));
    *SignalID = (gctINT) signal;

OnError:
    return status;
}

/*******************************************************************************
**
**  gckOS_DestroyUserSignal
**
**  Destroy a signal to be used in the user space.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctINT SignalID
**          The signal's ID.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_DestroyUserSignal(
    IN gckOS Os,
    IN gctINT SignalID
    )
{
    return gckOS_DestroySignal(Os, (gctSIGNAL)(gctUINTPTR_T)SignalID);
}

/*******************************************************************************
**
**  gckOS_WaitUserSignal
**
**  Wait for a signal used in the user mode to become signaled.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctINT SignalID
**          Signal ID.
**
**      gctUINT32 Wait
**          Number of milliseconds to wait.
**          Pass the value of gcvINFINITE for an infinite wait.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_WaitUserSignal(
    IN gckOS Os,
    IN gctINT SignalID,
    IN gctUINT32 Wait,
    OUT gceSIGNAL_STATUS *SignalStatus
    )
{
    return gckOS_WaitSignal(Os, (gctSIGNAL)(gctUINTPTR_T)SignalID, gcvTRUE, Wait);
}

/*******************************************************************************
**
**  gckOS_SignalUserSignal
**
**  Set a state of the specified signal to be used in the user space.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctINT SignalID
**          SignalID.
**
**      gctBOOL State
**          If gcvTRUE, the signal will be set to signaled state.
**          If gcvFALSE, the signal will be set to nonsignaled state.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_SignalUserSignal(
    IN gckOS Os,
    IN gctINT SignalID,
    IN gctBOOL State
    )
{
    return gckOS_Signal(Os, (gctSIGNAL)(gctUINTPTR_T)SignalID, State);
}


/******************************************************************************\
******************************* Private Functions ******************************
\******************************************************************************/
static void *_KernelTimerThread(void *data)
{
    gcsOSTIMER_PTR timer = (gcsOSTIMER_PTR)data;

    while (1)
    {
        (timer->func)(timer->data);
        taskDelay(sysClkRateGet()/1000 * timer->delay);
    }

    return 0;
}

/******************************************************************************\
******************************** Software Timer ********************************
\******************************************************************************/

/*******************************************************************************
**
**  gckOS_CreateTimer
**
**  Create a software timer.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gctTIMERFUNCTION Function.
**          Pointer to a call back function which will be called when timer is
**          expired.
**
**      gctPOINTER Data.
**          Private data which will be passed to call back function.
**
**  OUTPUT:
**
**      gctPOINTER * Timer
**          Pointer to a variable receiving the created timer.
*/
gceSTATUS
gckOS_CreateTimer(
    IN gckOS Os,
    IN gctTIMERFUNCTION Function,
    IN gctPOINTER Data,
    OUT gctPOINTER * Timer
    )
{
    char *        pTempStackMem;  /* temporary variable for task's stack */
    char *        pStackMem;      /* pointer to Stack memory */
    WIND_TCB *    pTskTcb;        /* tcb pointer of spawned task */

    gcsOSTIMER_PTR pointer;
    /* Allocate the structure. */
    pointer = (gcsOSTIMER_PTR) malloc(sizeof(gcsOSTIMER));

    pointer->isStart    = gcvFALSE;
    pointer->taskID = -1;
    pointer->func    = Function;
    pointer->data    = Data;


    pTempStackMem = (char *) memalign (_STACK_ALIGN_SIZE, 0x20000);
    pTskTcb = (WIND_TCB *) malloc (sizeof (WIND_TCB));
    pStackMem = (char *)(pTempStackMem + 0x20000);

    taskInit(pTskTcb, "galcore", 200, 8, pStackMem, 0x20000,
                           (FUNCPTR )_KernelTimerThread, (void *)pointer,
                           0, 0, 0, 0, 0, 0, 0, 0, 0);
    pointer->taskID = (TASK_ID) pTskTcb;

    if(pointer->taskID != 0)
    {
        printf("galcoreid=0x%x\n",pointer->taskID);
    }
    else
    {
        return gcvSTATUS_FALSE;
    }

    *Timer = (gctPOINTER)pointer;

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_DestroyTimer
**
**  Destory a software timer.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gctPOINTER Timer
**          Pointer to the timer to be destoryed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_DestroyTimer(
    IN gckOS Os,
    IN gctPOINTER Timer
    )
{
    gcsOSTIMER_PTR timer = (gcsOSTIMER_PTR)Timer;
    taskDelete(timer->taskID);

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_StartTimer
**
**  Schedule a software timer.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gctPOINTER Timer
**          Pointer to the timer to be scheduled.
**
**      gctUINT32 Delay
**          Delay in milliseconds.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_StartTimer(
    IN gckOS Os,
    IN gctPOINTER Timer,
    IN gctUINT32 Delay
    )
{
    gcsOSTIMER_PTR timer = (gcsOSTIMER_PTR)Timer;
    if (timer->isStart == gcvFALSE)
    {
        taskActivate(timer->taskID);
        timer->isStart == gcvTRUE;
        timer->delay = Delay;
    }

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_StopTimer
**
**  Cancel a unscheduled timer.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to the gckOS object.
**
**      gctPOINTER Timer
**          Pointer to the timer to be cancel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckOS_StopTimer(
    IN gckOS Os,
    IN gctPOINTER Timer
    )
{
    gcsOSTIMER_PTR timer = (gcsOSTIMER_PTR)Timer;
    taskSuspend(timer->taskID);

    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_GetProcessNameByPid(
    IN gctINT Pid,
    IN gctSIZE_T Length,
    OUT gctUINT8_PTR String
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_DumpCallStack(
    IN gckOS Os
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckOS_DetectProcessByName
**
**      task->comm maybe part of process name, so this function
**      can only be used for debugging.
**
**  INPUT:
**
**      gctCONST_POINTER Name
**          Pointer to a string to hold name to be check. If the length
**          of name is longer than TASK_COMM_LEN (16), use part of name
**          to detect.
**
**  OUTPUT:
**
**      gcvSTATUS_TRUE if name of current process matches Name.
**
*/
gceSTATUS
gckOS_DetectProcessByName(
    IN gctCONST_POINTER Name
    )
{
    return gcvSTATUS_OK;
}

#if gcdSECURITY
gceSTATUS
gckOS_AllocatePageArray(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T PageCount,
    OUT gctPOINTER * PageArrayLogical,
    OUT gctPHYS_ADDR * PageArrayPhysical
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    PVX_MDL  mdl;
    gctUINT32*  table;
    gctUINT32   offset;
    gctSIZE_T   bytes;
    gckALLOCATOR allocator;

    gcmkHEADER_ARG("Os=0x%X Physical=0x%X PageCount=%u",
                   Os, Physical, PageCount);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmkVERIFY_ARGUMENT(PageCount > 0);

    bytes = PageCount * gcmSIZEOF(gctUINT32);
    gcmkONERROR(gckOS_AllocateNonPagedMemory(
        Os,
        Kernel,
        gcvFALSE,
        gcvALLOC_FLAG_CONTIGUOUS,
        &bytes,
        PageArrayPhysical,
        PageArrayLogical
        ));

    table = *PageArrayLogical;

    /* Convert pointer to MDL. */
    mdl = (PVX_MDL)Physical;

    allocator = mdl->allocator;

     /* Get all the physical addresses and store them in the page table. */

    offset = 0;
    PageCount = PageCount / (PAGE_SIZE / 4096);

    /* Try to get the user pages so DMA can happen. */
    while (PageCount-- > 0)
    {
        unsigned long phys = ~0;

        gctPHYS_ADDR_T phys_addr;

        allocator->ops->Physical(allocator, mdl, offset * PAGE_SIZE, &phys_addr);

        phys = (unsigned long)phys_addr;

        table[offset] = phys & PAGE_MASK;

        offset += 1;
    }

OnError:

    /* Return the status. */
    gcmkFOOTER();
    return status;
}
#endif

gceSTATUS
gckOS_CPUPhysicalToGPUPhysical(
    IN gckOS Os,
    IN gctPHYS_ADDR_T CPUPhysical,
    IN gctPHYS_ADDR_T * GPUPhysical
    )
{
    gcsPLATFORM * platform;
    gcmkHEADER_ARG("CPUPhysical=%llx", CPUPhysical);

    platform = Os->device->platform;

    if (platform && platform->ops->getGPUPhysical)
    {
        gcmkVERIFY_OK(
            platform->ops->getGPUPhysical(platform, CPUPhysical, GPUPhysical));
    }
    else
    {
        *GPUPhysical = CPUPhysical;
    }

    gcmkFOOTER_ARG("GPUPhysical=0x%llx", gcmOPT_VALUE(GPUPhysical));
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_GPUPhysicalToCPUPhysical(
    IN gckOS Os,
    IN gctPHYS_ADDR_T GPUPhysical,
    IN gctPHYS_ADDR_T * CPUPhysical
    )
{
    gcsPLATFORM * platform;
    gcmkHEADER_ARG("GPUPhysical=0x%llx", GPUPhysical);

    platform = Os->device->platform;

    if (platform && platform->ops->getCPUPhysical)
    {
        gcmkVERIFY_OK(
            platform->ops->getCPUPhysical(platform, GPUPhysical, CPUPhysical));
    }
    else
    {
        *CPUPhysical = GPUPhysical;
    }

    gcmkFOOTER_ARG("CPUPhysical=0x%llx", gcmOPT_VALUE(CPUPhysical));
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_PhysicalToPhysicalAddress(
    IN gckOS Os,
    IN gctPOINTER Physical,
    IN gctUINT32 Offset,
    OUT gctPHYS_ADDR_T * PhysicalAddress
    )
{
    PVX_MDL mdl = (PVX_MDL)Physical;
    gckALLOCATOR allocator = mdl->allocator;

    if (allocator)
    {
        return allocator->ops->Physical(allocator, mdl, Offset, PhysicalAddress);
    }

    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gckOS_GetFd(
    IN gctSTRING Name,
    IN gcsFDPRIVATE_PTR Private,
    OUT gctINT * Fd
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_QueryOption(
    IN gckOS Os,
    IN gctCONST_STRING Option,
    OUT gctUINT64 * Value
    )
{
    gckGALDEVICE device = Os->device;

    if (!strcmp(Option, "physBase"))
    {
        *Value = device->physBase;
        return gcvSTATUS_OK;
    }
    else if (!strcmp(Option, "physSize"))
    {
        *Value = device->physSize;
        return gcvSTATUS_OK;
    }
    else if (!strcmp(Option, "mmu"))
    {
#if gcdSECURITY
        *Value = 0;
#else
        *Value = device->args.mmu;
#endif
        return gcvSTATUS_OK;
    }
    else if (!strcmp(Option, "contiguousSize"))
    {
        *Value = device->contiguousSize;
        return gcvSTATUS_OK;
    }
    else if (!strcmp(Option, "contiguousBase"))
    {
        *Value = (gctUINT32)device->contiguousBase;
        return gcvSTATUS_OK;
    }
    else if (!strcmp(Option, "recovery"))
    {
        *Value = device->args.recovery;
        return gcvSTATUS_OK;
    }
    else if (!strcmp(Option, "stuckDump"))
    {
        *Value = device->args.stuckDump;
        return gcvSTATUS_OK;
    }
    else if (!strcmp(Option, "powerManagement"))
    {
        *Value = device->args.powerManagement;
        return gcvSTATUS_OK;
    }
    else if (!strcmp(Option, "TA"))
    {
        *Value = 0;
        return gcvSTATUS_OK;
    }
    else if (!strcmp(Option, "gpuProfiler"))
    {
        *Value = device->args.gpuProfiler;
        return gcvSTATUS_OK;
    }

    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gckOS_MemoryGetSGT(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER *SGT
    )
{
    PVX_MDL mdl;
    gckALLOCATOR allocator;
    gceSTATUS status = gcvSTATUS_OK;

    if (!Physical)
    {
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    mdl = (PVX_MDL)Physical;
    allocator = mdl->allocator;

    if (!allocator->ops->GetSGT)
    {
        gcmkONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    if (Bytes > 0)
    {
        gcmkONERROR(allocator->ops->GetSGT(allocator, mdl, Offset, Bytes, SGT));
    }

OnError:
    return status;
}

gceSTATUS
gckOS_MemoryMmap(
    IN gckOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T skipPages,
    IN gctSIZE_T numPages,
    INOUT gctPOINTER Vma
    )
{
    PVX_MDL mdl;
    PVX_MDL_MAP mdlMap;
    gckALLOCATOR allocator;
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL cacheable = gcvFALSE;

    if (!Physical)
    {
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    mdl = (PVX_MDL)Physical;
    allocator = mdl->allocator;

    if (!allocator->ops->Mmap)
    {
        gcmkONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    pthread_mutex_lock(&mdl->mapsMutex);

    mdlMap = FindMdlMap(mdl, _GetProcessID());
    if (mdlMap)
    {
        cacheable = mdlMap->cacheable;
    }

    pthread_mutex_unlock(&mdl->mapsMutex);

    gcmkONERROR(allocator->ops->Mmap(allocator, mdl, cacheable, skipPages, numPages, Vma));

OnError:
    return status;
}

/*******************************************************************************
**
**  gckOS_WrapMemory
**
**  Import a number of pages allocated by other allocator.
**
**  INPUT:
**
**      gckOS Os
**          Pointer to an gckOS object.
**
**      gctUINT32 Flag
**          Memory type.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that hold the number of bytes allocated.
**
**      gctPHYS_ADDR * Physical
**          Pointer to a variable that will hold the physical address of the
**          allocation.
*/
gceSTATUS
gckOS_WrapMemory(
    IN gckOS Os,
    IN gckKERNEL Kernel,
    IN gcsUSER_MEMORY_DESC_PTR Desc,
    OUT gctSIZE_T *Bytes,
    OUT gctPHYS_ADDR * Physical,
    OUT gctBOOL *Contiguous,
    OUT gctSIZE_T * PageCountCpu
    )
{
    PVX_MDL mdl = gcvNULL;
    gceSTATUS status = gcvSTATUS_OUT_OF_MEMORY;
    gckALLOCATOR allocator;
    gcsATTACH_DESC desc;
    gctSIZE_T bytes = 0;

    gcmkHEADER_ARG("Os=0x%X ", Os);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmkVERIFY_ARGUMENT(Physical != gcvNULL);

    mdl = _CreateMdl(Os);
    if (mdl == gcvNULL)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    if (Desc->flag & gcvALLOC_FLAG_DMABUF)
    {
        desc.dmaBuf.dmabuf = gcmUINT64_TO_PTR(Desc->dmabuf);

#if defined(CONFIG_DMA_SHARED_BUFFER)
        {
            struct dma_buf *dmabuf = (struct dma_buf*)desc.dmaBuf.dmabuf;
            bytes = dmabuf->size;
        }
#endif
    }
    else if (Desc->flag & gcvALLOC_FLAG_USERMEMORY)
    {
        desc.userMem.memory   = gcmUINT64_TO_PTR(Desc->logical);
        desc.userMem.physical = Desc->physical;
        desc.userMem.size     = Desc->size;
        bytes                 = Desc->size;
    }
    else
    {
        gcmkONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Walk all allocators. */
    list_for_each_entry(allocator, &Os->allocatorList, link)
    {
        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_OS,
                       "%s(%d) Flag = %x allocator->capability = %x",
                        __FUNCTION__, __LINE__, Desc->flag, allocator->capability);

        if ((Desc->flag & allocator->capability) != Desc->flag)
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            continue;
        }

        status = allocator->ops->Attach(allocator, &desc, mdl);

        if (gcmIS_SUCCESS(status))
        {
            mdl->allocator = allocator;
            break;
        }
    }

    /* Check status. */
    gcmkONERROR(status);

    mdl->addr       = 0;

    mdl->bytes = bytes ? bytes : mdl->numPages * PAGE_SIZE;
    *Bytes = mdl->bytes;

    /* Return physical address. */
    *Physical = (gctPHYS_ADDR) mdl;

    *Contiguous = mdl->contiguous;

    if (PageCountCpu)
    {
        *PageCountCpu = mdl->numPages;
    }

    /*
     * Add this to a global list.
     * Will be used by get physical address
     * and mapuser pointer functions.
     */
    pthread_mutex_lock(&Os->mdlMutex);
    list_add_tail(&mdl->link, &Os->mdlHead);
    pthread_mutex_unlock(&Os->mdlMutex);

    /* Success. */
    gcmkFOOTER_ARG("*Physical=0x%X", *Physical);
    return gcvSTATUS_OK;

OnError:
    if (mdl != gcvNULL)
    {
        /* Free the memory. */
        _DestroyMdl(mdl);
    }

    /* Return the status. */
    gcmkFOOTER();
    return status;
}

gceSTATUS
gckOS_GetPolicyID(
    IN gckOS Os,
    IN gceVIDMEM_TYPE Type,
    OUT gctUINT32_PTR PolicyID,
    OUT gctUINT32_PTR AXIConfig
    )
{
    gcsPLATFORM * platform = Os->device->platform;

    if (platform && platform->ops->getPolicyID)
    {
        return platform->ops->getPolicyID(platform, Type, PolicyID, AXIConfig);
    }

    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gckOS_QueryKernel(
    IN gckKERNEL Kernel,
    IN gctINT index,
    OUT gckKERNEL * KernelOut
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_QueryCPUFrequency(
    IN gckOS Os,
    IN gctUINT32 CPUId,
    OUT gctUINT32 * Frequency
)
{
    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_TraceGpuMemory(
    IN gckOS Os,
    IN gctINT32 ProcessID,
    IN gctINT64 Delta)
{
    return gcvSTATUS_NOT_SUPPORTED;
}

void
gckOS_NodeIdAssign(
    gckOS Os,
    gcuVIDMEM_NODE_PTR Node)
{
    return;
}

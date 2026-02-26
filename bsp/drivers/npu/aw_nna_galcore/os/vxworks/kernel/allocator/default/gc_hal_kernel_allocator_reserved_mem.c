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
#include "gc_hal_kernel_allocator.h"
#define _GC_OBJ_ZONE    gcvZONE_OS

/*
 * reserved_mem is for contiguous pool, internal pool and external pool, etc.
 */

/* mdl private. */
struct reserved_mem
{
    unsigned long start;
    unsigned long size;
    char name[32];
    int  release;

    /* Link together. */
    struct list_head link;
};

/* allocator info. */
struct reserved_mem_alloc
{
    /* Record allocated reserved memory regions. */
    struct list_head region;
    pthread_mutex_t lock;
};

static gceSTATUS
reserved_mem_attach(
    IN gckALLOCATOR Allocator,
    IN gcsATTACH_DESC_PTR Desc,
    IN PVX_MDL Mdl
    )
{
    struct reserved_mem_alloc *alloc = Allocator->privateData;
    struct reserved_mem *res;

    res = (struct reserved_mem *)malloc(sizeof(struct reserved_mem));

    if (!res)
        return gcvSTATUS_OUT_OF_MEMORY;

    res->start = Desc->reservedMem.start;
    res->size  = Desc->reservedMem.size;
    strncpy(res->name, Desc->reservedMem.name, sizeof(res->name)-1);

    if (!Desc->reservedMem.requested)
    {
        res->release = 1;
    }

    pthread_mutex_lock(&alloc->lock);
    list_add(&res->link, &alloc->region);
    pthread_mutex_unlock(&alloc->lock);

    Mdl->priv = res;

    return gcvSTATUS_OK;
}

static void
reserved_mem_detach(
    IN gckALLOCATOR Allocator,
    IN OUT PVX_MDL Mdl
    )
{
    struct reserved_mem_alloc *alloc = Allocator->privateData;
    struct reserved_mem *res = Mdl->priv;

    /* unlink from region list. */
    pthread_mutex_lock(&alloc->lock);
    list_del_init(&res->link);
    pthread_mutex_unlock(&alloc->lock);

    if (res->release)
    {
    }

    free(res);
}

static void
reserved_mem_unmap_user(
    IN gckALLOCATOR Allocator,
    IN PVX_MDL Mdl,
    IN PVX_MDL_MAP MdlMap,
    IN gctUINT32 Size
    )
{
    return;
}

static gceSTATUS
reserved_mem_map_user(
    gckALLOCATOR Allocator,
    PVX_MDL Mdl,
    PVX_MDL_MAP MdlMap,
    gctBOOL Cacheable
    )
{
    return gcvSTATUS_OK;
}

static gceSTATUS
reserved_mem_map_kernel(
    IN gckALLOCATOR Allocator,
    IN PVX_MDL Mdl,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER *Logical
    )
{
    return gcvSTATUS_OK;
}

static gceSTATUS
reserved_mem_unmap_kernel(
    IN gckALLOCATOR Allocator,
    IN PVX_MDL Mdl,
    IN gctPOINTER Logical
    )
{
    return gcvSTATUS_OK;
}

static gceSTATUS
reserved_mem_cache_op(
    IN gckALLOCATOR Allocator,
    IN PVX_MDL Mdl,
    IN gctSIZE_T Offset,
    IN gctPOINTER Logical,
    IN gctUINT32 Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

static gceSTATUS
reserved_mem_get_physical(
    IN gckALLOCATOR Allocator,
    IN PVX_MDL Mdl,
    IN gctUINT32 Offset,
    OUT gctPHYS_ADDR_T * Physical
    )
{
    struct reserved_mem *res = Mdl->priv;
    *Physical = res->start + Offset;

    return gcvSTATUS_OK;
}

static void
reserved_mem_dtor(
    gcsALLOCATOR *Allocator
    )
{
    if (Allocator->privateData)
    {
        free(Allocator->privateData);
    }

    free(Allocator);
}

/* GFP allocator operations. */
static gcsALLOCATOR_OPERATIONS reserved_mem_ops = {
    .Alloc              = NULL,
    .Attach             = reserved_mem_attach,
    .Free               = reserved_mem_detach,
    .MapUser            = reserved_mem_map_user,
    .UnmapUser          = reserved_mem_unmap_user,
    .MapKernel          = reserved_mem_map_kernel,
    .UnmapKernel        = reserved_mem_unmap_kernel,
    .Cache              = reserved_mem_cache_op,
    .Physical           = reserved_mem_get_physical,
};

/* GFP allocator entry. */
gceSTATUS
_ReservedMemoryAllocatorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    )
{
    gceSTATUS status;
    gckALLOCATOR allocator = gcvNULL;
    struct reserved_mem_alloc *alloc = NULL;

    gcmkONERROR(
        gckALLOCATOR_Construct(Os, &reserved_mem_ops, &allocator));

    alloc = (struct reserved_mem_alloc *) malloc(sizeof(*alloc));

    if (!alloc)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    INIT_LIST_HEAD(&alloc->region);
    pthread_mutex_init(&alloc->lock, 0);

    /* Register private data. */
    allocator->privateData = alloc;
    allocator->destructor = reserved_mem_dtor;

    allocator->capability = gcvALLOC_FLAG_LINUX_RESERVED_MEM;

    *Allocator = allocator;

    return gcvSTATUS_OK;

OnError:
    if (allocator)
    {
        free(allocator);
    }
    return status;
}


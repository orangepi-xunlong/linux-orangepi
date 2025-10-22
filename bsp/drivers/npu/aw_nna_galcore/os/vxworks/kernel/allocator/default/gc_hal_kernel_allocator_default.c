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

struct mdl_priv {
    gctPOINTER kvaddr;
    gctPOINTER sPtr;
    gctPHYS_ADDR_T phys;
};


static gceSTATUS
_Alloc(
    IN gckALLOCATOR Allocator,
    INOUT PVX_MDL Mdl,
    IN gctSIZE_T NumPages,
    IN gctUINT32 Flags
    )
{
    gceSTATUS status;

    struct mdl_priv *mdlPriv = gcvNULL;
    gckOS os = Allocator->os;
    gctUINT32 offset;
    gctUINT32 alignment = PAGE_SIZE;

    gcmkHEADER_ARG("Mdl=%p NumPages=0x%x Flags=0x%x", Mdl, NumPages, Flags);

    gcmkONERROR(gckOS_Allocate(os, sizeof(struct mdl_priv), (gctPOINTER *) &mdlPriv));
    mdlPriv->sPtr = cacheDmaMalloc(NumPages * PAGE_SIZE + offset);
    if (mdlPriv->sPtr == gcvNULL)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    mdlPriv->kvaddr = (void*)( ((size_t)mdlPriv->sPtr + offset) & (~(alignment - 1)) );

    mdlPriv->phys = KM_TO_PHYS(mdlPriv->kvaddr);

    Mdl->priv = mdlPriv;

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (mdlPriv)
    {
        gckOS_Free(os, mdlPriv);
    }

    gcmkFOOTER();
    return status;
}

static void
_Free(
    IN gckALLOCATOR Allocator,
    IN OUT PVX_MDL Mdl
    )
{
    gckOS os = Allocator->os;
    struct mdl_priv *mdlPriv = (struct mdl_priv *)Mdl->priv;

    cacheDmaFree(mdlPriv->sPtr);

    gckOS_Free(os, mdlPriv);
}

static gctINT
_MapUser(
    gckALLOCATOR Allocator,
    PVX_MDL Mdl,
    IN PVX_MDL_MAP MdlMap,
    gctBOOL Cacheable
    )
{
    struct mdl_priv *mdlPriv = (struct mdl_priv *)Mdl->priv;

    MdlMap->vmaAddr = mdlPriv->kvaddr;

    return gcvSTATUS_OK;
}

static void
_UnmapUser(
    IN gckALLOCATOR Allocator,
    IN PVX_MDL Mdl,
    IN PVX_MDL_MAP MdlMap,
    IN gctUINT32 Size
    )
{
    return;
}

static gceSTATUS
_MapKernel(
    IN gckALLOCATOR Allocator,
    IN PVX_MDL Mdl,
    IN gctSIZE_T Offset,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER *Logical
    )
{
    struct mdl_priv *mdlPriv = (struct mdl_priv *)Mdl->priv;
    *Logical = mdlPriv->kvaddr;
    return gcvSTATUS_OK;
}

static gceSTATUS
_UnmapKernel(
    IN gckALLOCATOR Allocator,
    IN PVX_MDL Mdl,
    IN gctPOINTER Logical
    )
{
    return gcvSTATUS_OK;
}

static gceSTATUS
_Physical(
    IN gckALLOCATOR Allocator,
    IN PVX_MDL Mdl,
    IN gctUINT32 Offset,
    OUT gctPHYS_ADDR_T * Physical
    )
{
    struct mdl_priv *mdlPriv=(struct mdl_priv *)Mdl->priv;

    *Physical = mdlPriv->phys + Offset;

    return gcvSTATUS_OK;
}

static void
_AllocatorDestructor(
    gcsALLOCATOR *Allocator
    )
{
    if (Allocator->privateData)
    {
        free(Allocator->privateData);
    }

    free(Allocator);
}

/* Default allocator operations. */
gcsALLOCATOR_OPERATIONS allocatorOperations = {
    .Alloc              = _Alloc,
    .Free               = _Free,
    .MapUser            = _MapUser,
    .UnmapUser          = _UnmapUser,
    .MapKernel          = _MapKernel,
    .UnmapKernel        = _UnmapKernel,
    .Physical           = _Physical,
};

/* Default allocator entry. */
gceSTATUS
_AllocatorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    )
{
    gceSTATUS status;
    gckALLOCATOR allocator = gcvNULL;

    gcmkONERROR(gckALLOCATOR_Construct(Os, &allocatorOperations, &allocator));

    /* Register private data. */
    allocator->destructor  = _AllocatorDestructor;

    allocator->capability = gcvALLOC_FLAG_CONTIGUOUS
                          | gcvALLOC_FLAG_NON_CONTIGUOUS
                          | gcvALLOC_FLAG_CACHEABLE
                          | gcvALLOC_FLAG_MEMLIMIT
                          | gcvALLOC_FLAG_ALLOC_ON_FAULT
                          ;


    *Allocator = allocator;

    return gcvSTATUS_OK;

OnError:
    if (allocator)
    {
        free(allocator);
    }
    return status;
}


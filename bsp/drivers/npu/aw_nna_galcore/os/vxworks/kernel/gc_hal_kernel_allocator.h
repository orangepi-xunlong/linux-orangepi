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

#ifndef __gc_hal_kernel_allocator_h_
#define __gc_hal_kernel_allocator_h_

#include "gc_hal_kernel_vxworks.h"

typedef struct _gcsALLOCATOR * gckALLOCATOR;
typedef union _gcsATTACH_DESC * gcsATTACH_DESC_PTR;

typedef struct _gcsALLOCATOR_OPERATIONS
{
    /**************************************************************************
    **
    ** Alloc
    **
    ** Allocte memory, request size is page aligned.
    **
    ** INPUT:
    **
    **    gckALLOCATOR Allocator
    **        Pointer to an gckALLOCATOER object.
    **
    **    PVX_Mdl
    **        Pointer to Mdl whichs stores information
    **        about allocated memory.
    **
    **    gctSIZE_T NumPages
    **        Number of pages need to allocate.
    **
    **    gctUINT32 Flag
    **        Allocation option.
    **
    ** OUTPUT:
    **
    **      Nothing.
    **
    */
    gceSTATUS
    (*Alloc)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl,
        IN gctSIZE_T NumPages,
        IN gctUINT32 Flag
        );

    /**************************************************************************
    **
    ** Free
    **
    ** Free memory.
    **
    ** INPUT:
    **
    **     gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **     PVX_MDL Mdl
    **          Mdl which stores information.
    **
    ** OUTPUT:
    **
    **      Nothing.
    **
    */
    void
    (*Free)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl
        );

    /**************************************************************************
    **
    ** Mmap
    **
    ** Map a page range of the memory to user space.
    **
    ** INPUT:
    **      gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **      PVX_MDL Mdl
    **          Pointer to a Mdl.
    **
    **      gctSIZE_T skipPages
    **          Number of page to be skipped from beginning of this memory.
    **
    **      gctSIZE_T numPages
    **          Number of pages to be mapped from skipPages.
    **
    ** INOUT:
    **
    **      struct vm_area_struct *vma
    **          Pointer to VMM memory area.
    **
    */
    gceSTATUS
    (*Mmap)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl,
        IN gctBOOL Cacheable,
        IN gctSIZE_T skipPages,
        IN gctSIZE_T numPages,
        IN struct vm_area_struct *vma
        );

    /**************************************************************************
    **
    ** MapUser
    **
    ** Map memory to user space.
    **
    ** INPUT:
    **      gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **      PVX_MDL Mdl
    **          Pointer to a Mdl.
    **
    **      gctBOOL Cacheable
    **          Whether this mapping is cacheable.
    **
    ** OUTPUT:
    **
    **      gctPOINTER * UserLogical
    **          Pointer to user logical address.
    **
    **      Nothing.
    **
    */
    gceSTATUS
    (*MapUser)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl,
        IN PVX_MDL_MAP MdlMap,
        IN gctBOOL Cacheable
        );

    /**************************************************************************
    **
    ** UnmapUser
    **
    ** Unmap address from user address space.
    **
    ** INPUT:
    **      gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **      gctPOINTER Logical
    **          Address to be unmap
    **
    **      gctUINT32 Size
    **          Size of address space
    **
    ** OUTPUT:
    **
    **      Nothing.
    **
    */
    void
    (*UnmapUser)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl,
        IN PVX_MDL_MAP MdlMap,
        IN gctUINT32 Size
        );

    /**************************************************************************
    **
    ** MapKernel
    **
    ** Map memory to kernel space.
    **
    ** INPUT:
    **      gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **      PVX_MDL Mdl
    **          Pointer to a Mdl object.
    **
    ** OUTPUT:
    **      gctPOINTER * Logical
    **          Mapped kernel address.
    */
    gceSTATUS
    (*MapKernel)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl,
        IN gctSIZE_T Offset,
        IN gctSIZE_T Bytes,
        OUT gctPOINTER *Logical
        );

    /**************************************************************************
    **
    ** UnmapKernel
    **
    ** Unmap memory from kernel space.
    **
    ** INPUT:
    **      gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **      PVX_MDL Mdl
    **          Pointer to a Mdl object.
    **
    **      gctPOINTER Logical
    **          Mapped kernel address.
    **
    ** OUTPUT:
    **
    **      Nothing.
    **
    */
    gceSTATUS
    (*UnmapKernel)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl,
        IN gctPOINTER Logical
        );

    /**************************************************************************
    **
    ** Cache
    **
    ** Maintain cache coherency.
    **
    ** INPUT:
    **      gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **      PVX_MDL Mdl
    **          Pointer to a Mdl object.
    **
    **      gctPOINTER Logical
    **          Logical address, could be user address or kernel address
    **
    **      gctUINT32_PTR Physical
    **          Physical address.
    **
    **      gctUINT32 Bytes
    **          Size of memory region.
    **
    **      gceCACHEOPERATION Opertaion
    **          Cache operation.
    **
    ** OUTPUT:
    **
    **      Nothing.
    **
    */
    gceSTATUS (*Cache)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl,
        IN gctSIZE_T Offset,
        IN gctPOINTER Logical,
        IN gctUINT32 Bytes,
        IN gceCACHEOPERATION Operation
        );

    /**************************************************************************
    **
    ** Physical
    **
    ** Get physical address from a offset in memory region.
    **
    ** INPUT:
    **      gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **      PVX_MDL Mdl
    **          Pointer to a Mdl object.
    **
    **      gctUINT32 Offset
    **          Offset in this memory region.
    **
    ** OUTPUT:
    **      gctUINT32_PTR Physical
    **          Physical address.
    **
    */
    gceSTATUS (*Physical)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl,
        IN gctUINT32 Offset,
        OUT gctPHYS_ADDR_T * Physical
        );

    /**************************************************************************
    **
    ** Attach
    **
    ** Import memory allocated by an external allocator.
    **
    ** INPUT:
    **      gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **      gctUINT32 Handle
    **          Handle of the memory.
    **
    ** OUTPUT:
    **      None.
    **
    */
    gceSTATUS (*Attach)(
        IN gckALLOCATOR Allocator,
        IN gcsATTACH_DESC_PTR Desc,
        OUT PVX_MDL Mdl
        );

    /**************************************************************************
    **
    ** GetSGT
    **
    ** Get scatter-gather table from a range of the memory.
    **
    ** INPUT:
    **      gckALLOCATOR Allocator
    **          Pointer to an gckALLOCATOER object.
    **
    **      gctUINT32 Handle
    **          Handle of the memory.
    **
    **      gctSIZE_T Offset
    **          Offset to the beginning of this mdl.
    **
    **      gctSIZE_T Bytes
    **          Total bytes form Offset.
    **
    ** OUTPUT:
    **      gctPOINTER *SGT
    **          scatter-gather table
    **
    */
    gceSTATUS (*GetSGT)(
        IN gckALLOCATOR Allocator,
        IN PVX_MDL Mdl,
        IN gctSIZE_T Offset,
        IN gctSIZE_T Bytes,
        OUT gctPOINTER *SGT
        );
}
gcsALLOCATOR_OPERATIONS;

typedef struct _gcsALLOCATOR
{
    /* Pointer to gckOS Object. */
    gckOS                     os;

    /* Name. */
    gctSTRING                 name;

    /* Operations. */
    gcsALLOCATOR_OPERATIONS * ops;

    /* Capability of this allocator. */
    gctUINT32                 capability;

    /* Private data used by customer allocator. */
    void *                    privateData;

    /* Allocator destructor. */
    void                      (*destructor)(struct _gcsALLOCATOR *);

    struct list_head          link;
}
gcsALLOCATOR;

typedef struct _gcsALLOCATOR_DESC
{
    /* Name of a allocator. */
    char *                    name;

    /* Entry function to construct a allocator. */
    gceSTATUS                 (*construct)(gckOS, gckALLOCATOR *);
}
gcsALLOCATOR_DESC;

typedef union _gcsATTACH_DESC
{
    /* gcvALLOC_FLAG_DMABUF */
    struct
    {
        gctPOINTER              dmabuf;
    }
    dmaBuf;

    /* gcvALLOC_FLAG_USERMEMORY */
    struct
    {
        gctPOINTER              memory;
        gctPHYS_ADDR_T          physical;
        gctSIZE_T               size;
    }
    userMem;

    /* Reserved memory. */
    struct
    {
        unsigned long           start;
        unsigned long           size;
        const char *            name;
        int                     requested;
    }
    reservedMem;
}
gcsATTACH_DESC;

/*
* Helpers
*/

/* Fill a gcsALLOCATOR_DESC structure. */
#define gcmkDEFINE_ALLOCATOR_DESC(Name, Construct) \
    { \
        .name      = Name, \
        .construct = Construct, \
    }

/* Construct a allocator. */
static inline gceSTATUS
gckALLOCATOR_Construct(
    IN gckOS Os,
    IN gcsALLOCATOR_OPERATIONS * Operations,
    OUT gckALLOCATOR * Allocator
    )
{
    gceSTATUS status;
    gckALLOCATOR allocator;

    gcmkASSERT(Allocator != gcvNULL);
    gcmkASSERT
        (  Operations
        && (Operations->Alloc || Operations->Attach)
        && (Operations->Free)
        && Operations->MapUser
        && Operations->UnmapUser
        && Operations->MapKernel
        && Operations->UnmapKernel
        && Operations->Cache
        && Operations->Physical
        );

    allocator = (gckALLOCATOR) malloc(sizeof(gcsALLOCATOR));
    if (!allocator)
    {
        gcmkONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    /* Record os. */
    allocator->os = Os;

    /* Set operations. */
    allocator->ops = Operations;

    *Allocator = allocator;

    return gcvSTATUS_OK;

OnError:
    return status;
}

#endif

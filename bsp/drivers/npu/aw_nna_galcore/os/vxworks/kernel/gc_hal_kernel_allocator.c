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
#include "gc_hal_kernel_allocator_array.h"
#include "gc_hal_kernel_platform.h"

#define _GC_OBJ_ZONE    gcvZONE_OS

/***************************************************************************\
************************ Allocator management *******************************
\***************************************************************************/

gceSTATUS
gckOS_ImportAllocators(
    gckOS Os
    )
{
    gceSTATUS status;
    gctUINT i;
    gckALLOCATOR allocator;

    INIT_LIST_HEAD(&Os->allocatorList);

    for (i = 0; i < gcmCOUNTOF(allocatorArray); i++)
    {
        if (allocatorArray[i].construct)
        {
            /* Construct allocator. */
            status = allocatorArray[i].construct(Os, &allocator);

            if (gcmIS_ERROR(status))
            {
                gcmkPRINT("["DEVICE_NAME"]: Can't construct allocator(%s)",
                          allocatorArray[i].name);

                continue;
            }

            allocator->name = allocatorArray[i].name;

            list_add_tail(&allocator->link, &Os->allocatorList);
        }
    }

#if gcdDEBUG
    list_for_each_entry(allocator, &Os->allocatorList, link)
    {
        gcmkTRACE_ZONE(
            gcvLEVEL_WARNING, gcvZONE_OS,
            "%s(%d) Allocator: %s",
            __FUNCTION__, __LINE__,
            allocator->name
            );
    }
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gckOS_FreeAllocators(
    gckOS Os
    )
{
    gckALLOCATOR allocator;
    gckALLOCATOR temp;

    list_for_each_entry_safe(allocator, temp, &Os->allocatorList, link)
    {
        list_del(&allocator->link);

        /* Destroy allocator. */
        allocator->destructor(allocator);
    }

    return gcvSTATUS_OK;
}



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

#ifndef __gc_hal_kernel_allocator_array_h_
#define __gc_hal_kernel_allocator_array_h_

extern gceSTATUS
_AllocatorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    );

extern gceSTATUS
_UserMemoryAlloctorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    );

extern gceSTATUS
_ReservedMemoryAllocatorInit(
    IN gckOS Os,
    OUT gckALLOCATOR * Allocator
    );

/* Default allocator entry. */
gcsALLOCATOR_DESC allocatorArray[] =
{
    gcmkDEFINE_ALLOCATOR_DESC("default", _AllocatorInit),

    /* User memory importer. */
    gcmkDEFINE_ALLOCATOR_DESC("user", _UserMemoryAlloctorInit),

    gcmkDEFINE_ALLOCATOR_DESC("reserved-mem", _ReservedMemoryAllocatorInit),
};

#endif

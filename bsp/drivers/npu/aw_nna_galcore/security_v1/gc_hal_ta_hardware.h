/****************************************************************************
*    Copyright (C) 2005 - 2024 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/

#ifndef _GC_HAL_TA_HARDWARE_H_
#define _GC_HAL_TA_HARDWARE_H_
#include "gc_hal_types.h"
#include "gc_hal_security_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gcsMMU_TABLE_ARRAY_ENTRY
{
    gctUINT32                   low;
    gctUINT32                   high;
}
gcsMMU_TABLE_ARRAY_ENTRY;

typedef struct _gcsHARDWARE_PAGETABLE_ARRAY
{
    /* Number of entries in page table array. */
    gctUINT                     num;

    /* Size in bytes of array. */
    gctSIZE_T                   size;

    /* Physical address of array. */
    gctPHYS_ADDR_T              address;

    /* Memory descriptor. */
    gctPOINTER                  physical;

    /* Logical address of array. */
    gctPOINTER                  logical;
}
gcsHARDWARE_PAGETABLE_ARRAY;

typedef struct _gcsHARWARE_FUNCTION
{
    /* Entry of the function. */
    gctUINT32                   address;

    /* CPU address of the function. */
    gctUINT8_PTR                logical;

    /* Bytes of the function. */
    gctUINT32                   bytes;

    /* Hardware address of END in this function. */
    gctUINT32                   endAddress;

    /* Logical of END in this function. */
    gctUINT8_PTR                endLogical;
}
gcsHARDWARE_FUNCTION;

typedef struct _gcTA_HARDWARE
{
    gctaOS                      os;
    gcTA                        ta;

    gctUINT32                   chipModel;
    gctUINT32                   chipRevision;
    gctUINT32                   productID;
    gctUINT32                   ecoID;
    gctUINT32                   customerID;

    gctPOINTER                  featureDatabase;

    gcsHARDWARE_PAGETABLE_ARRAY pagetableArray;

    /* Function used by gctaHARDWARE. */
    gctPHYS_ADDR                functionPhysical;
    gctPOINTER                  functionLogical;
    gctUINT32                   functionAddress;
    gctSIZE_T                   functionBytes;

    gcsHARDWARE_FUNCTION        functions[1];
}
gcsTA_HARDWARE;

#ifdef __cplusplus
}
#endif
#endif



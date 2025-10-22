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

#ifndef _GC_HAL_TA_H_
#define _GC_HAL_TA_H_
#include "gc_hal_types.h"
#include "gc_hal_security_interface.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _gctaOS          * gctaOS;
typedef struct _gcTA            * gcTA;

typedef struct _gcTA_HARDWARE   * gcTA_HARDWARE;
typedef struct _gcTA_MMU        * gcTA_MMU;

/*
    Trust Application is a object needed to be created as a context in trust zone.
    One client for a core.
*/
typedef struct _gcTA {
    /* gctaOS object */
    gctaOS          os;

    gceCORE         core;

    gcTA_MMU        mmu;

    gcTA_HARDWARE   hardware;

    gctBOOL         destoryMmu;
} gcsTA;

typedef struct _gcTA_MMU
{
    gctaOS          os;

    gctSIZE_T       mtlbBytes;
    gctPOINTER      mtlbLogical;
    gctPHYS_ADDR    mtlbPhysical;

    gctPOINTER      stlbs;

    gctPOINTER      safePageLogical;
    gctPHYS_ADDR    safePagePhysical;

    gctPOINTER      nonSecureSafePageLogical;
    gctPHYS_ADDR    nonSecureSafePagePhysical;

    gctPOINTER      mutex;
}
gcsTA_MMU;

gceSTATUS HALDECL
TAEmulator(
    gceCORE Core,
    void * Interface
    );

int
gcTA_Construct(
    IN gctaOS Os,
    IN gceCORE Core,
    OUT gcTA *TA
);

int
gcTA_Destroy(
    IN gcTA TA
);

int
gcTA_Dispatch(
    IN gcTA TA,
    IN OUT gcsTA_INTERFACE * Interface
);

/*************************************
* Porting layer
*/

gceSTATUS
gctaOS_ConstructOS(
    IN gckOS Os,
    OUT gctaOS *TAos
    );

gceSTATUS
gctaOS_DestroyOS(
    IN gctaOS Os
    );

gceSTATUS
gctaOS_Allocate(
    IN gctUINT32 Bytes,
    OUT gctPOINTER *Pointer
    );

gceSTATUS
gctaOS_Free(
    IN gctPOINTER Pointer
    );

gceSTATUS
gctaOS_AllocateSecurityMemory(
    IN gctaOS Os,
    IN gctSIZE_T *Bytes,
    OUT gctPOINTER *Logical,
    OUT gctPOINTER *Physical
    );

gceSTATUS
gctaOS_FreeSecurityMemory(
    IN gctaOS Os,
    IN gctSIZE_T  Bytes,
    IN gctPOINTER Logical,
    OUT gctPOINTER Physical
    );

gceSTATUS
gctaOS_AllocateNonSecurityMemory(
    IN gctaOS Os,
    IN gctSIZE_T *Bytes,
    OUT gctPOINTER *Logical,
    OUT gctPOINTER *Physical
    );

gceSTATUS
gctaOS_FreeNonSecurityMemory(
    IN gctaOS Os,
    IN gctSIZE_T  Bytes,
    IN gctPOINTER Logical,
    OUT gctPOINTER Physical
    );



gceSTATUS
gctaOS_GetPhysicalAddress(
    IN gctaOS Os,
    IN gctPOINTER Logical,
    OUT gctPHYS_ADDR_T * Physical
    );

gceSTATUS gctaOS_WriteRegister(
    IN gctaOS Os, IN gceCORE Core,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    );

gceSTATUS gctaOS_ReadRegister(
    IN gctaOS Os, IN gceCORE Core,
    IN gctUINT32 Address,
    IN gctUINT32 *Data
    );

gceSTATUS
gctaOS_MemCopy(
    IN gctUINT8_PTR Dest,
    IN gctUINT8_PTR Src,
    IN gctUINT32 Bytes
    );

gceSTATUS
gctaOS_ZeroMemory(
    IN gctUINT8_PTR Dest,
    IN gctUINT32 Bytes
    );

void
gctaOS_CacheFlush(
    IN gctUINT8_PTR Dest,
    IN gctUINT32 Bytes
    );

void
gctaOS_CacheClean(
    IN gctUINT8_PTR Dest,
    IN gctUINT32 Bytes
    );

void
gctaOS_CacheInvalidate(
    IN gctUINT8_PTR Dest,
    IN gctUINT32 Bytes
    );

gceSTATUS
gctaOS_IsPhysicalSecure(
    IN gctaOS Os,
    IN gctUINT32 Physical,
    OUT gctBOOL *Secure
    );

gceSTATUS
gctaOS_Delay(
    IN gctaOS Os,
    IN gctUINT32 Delay
    );

gceSTATUS
gctaOS_SetGPUPower(
    IN gctaOS Os,
    IN gctUINT32 Core,
    IN gctBOOL Clock,
    IN gctBOOL Power
    );

/*
** gctaHARDWARE
*/
gceSTATUS
gctaHARDWARE_Construct(
    IN gcTA TA,
    OUT gcTA_HARDWARE * Hardware
    );

gceSTATUS
gctaHARDWARE_Destroy(
    IN gcTA_HARDWARE Hardware
    );

gceSTATUS
gctaHARDWARE_Execute(
    IN gcTA TA,
    IN gctUINT32 Address,
    IN gctUINT32 Bytes
    );

gceSTATUS
gctaHARDWARE_End(
    IN gcTA_HARDWARE Hardware,
    IN gctPOINTER Logical,
    IN OUT gctUINT32 * Bytes
    );

gceSTATUS
gctaHARDWARE_SetMMU(
    IN gcTA_HARDWARE Hardware,
    IN gctPOINTER Logical
    );

gceSTATUS
gctaHARDWARE_IsFeatureAvailable(
    IN gcTA_HARDWARE Hardware,
    IN gceFEATURE Feature
    );

gceSTATUS
gctaHARDWARE_PrepareFunctions(
    IN gcTA_HARDWARE Hardware
    );

gceSTATUS
gctaHARDWARE_DumpMMUException(
    IN gcTA_HARDWARE Hardware
    );

gceSTATUS
gctaHARDWARE_HandleMMUException(
    IN gcTA_HARDWARE Hardware,
    IN gctUINT32 MMUStatus,
    IN gctPHYS_ADDR_T Physical,
    IN gctUINT32 GPUAddress
    );

gceSTATUS
gctaHARDWARE_ReadMMUException(
    IN gcTA_HARDWARE Hardware,
    OUT gctUINT32_PTR MMUStatus,
    OUT gctUINT32_PTR MMUException
    );

gceSTATUS
gctaMMU_Construct(
    IN gcTA TA,
    OUT gcTA_MMU *Mmu
    );

gceSTATUS
gctaMMU_Destory(
    IN gcTA_MMU Mmu
    );

gceSTATUS
gctaMMU_SetPage(
    IN gcTA_MMU Mmu,
    IN gctUINT32 PageAddress,
    IN gctUINT32 *PageEntry
    );

gceSTATUS
gctaMMU_GetPageEntry(
    IN gcTA_MMU Mmu,
    IN gctUINT32 Address,
    OUT gctUINT32_PTR MtlbEntry,
    OUT gctUINT32_PTR *PageTable,
    OUT gctBOOL * Secure
    );

void
gctaMMU_DumpPagetableEntry(
    IN gcTA_MMU Mmu,
    IN gctUINT32 Address
    );

gceSTATUS
gctaMMU_FreePages(
    IN gcTA_MMU Mmu,
    IN gctUINT32 Address,
    IN gctUINT32 PageCount
    );

#ifdef __cplusplus
}
#endif
#endif



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

#include "gc_hal_base.h"
#include "gc_hal.h"
#include "gc_hal_ta.h"
#include "gc_hal_kernel_mutex.h"

#define _GC_OBJ_ZONE    gcvZONE_OS

gcTA globalTA[gcvCORE_COUNT];
gctaOS globalTAos;

struct _gctaOS {
    void *os;

    gctPOINTER dispatchMutex;
};

gceSTATUS HALDECL
TAEmulator(
    gceCORE Core,
    void * Interface
    )
{
    gckOS_AcquireMutex(globalTAos->os, globalTAos->dispatchMutex, gcvINFINITE);

    gcTA_Dispatch(globalTA[Core], Interface);

    gckOS_ReleaseMutex(globalTAos->os, globalTAos->dispatchMutex);
    return gcvSTATUS_OK;
}


gceSTATUS
gctaOS_ConstructOS(
    IN gckOS Os,
    OUT gctaOS *TAos
    )
{
    gctaOS os;
    gctPOINTER pointer = gcvNULL;
    gceSTATUS status;

    gcmkONERROR(gckOS_AllocateMemory(Os, gcmSIZEOF(struct _gctaOS), &pointer));

    os = (gctaOS)pointer;
    os->os = Os;

    gcmkONERROR(gckOS_ZeroMemory(globalTA, gcmSIZEOF(gcTA) * gcvCORE_COUNT));

    gcmkONERROR(gckOS_CreateMutex(Os, &os->dispatchMutex));

    *TAos = globalTAos = os;

    return gcvSTATUS_OK;

OnError:
    if (pointer != gcvNULL)
    {
        gcmkVERIFY_OK(gckOS_FreeMemory(Os, pointer));
    }
    return status;
}

gceSTATUS
gctaOS_DestroyOS(
    IN gctaOS Os
    )
{
    gckOS os = Os->os;

    gcmkVERIFY_OK(gckOS_DeleteMutex(os, Os->dispatchMutex));
    gcmkVERIFY_OK(gckOS_FreeMemory(os, Os));

    return gcvSTATUS_OK;
}

gceSTATUS
gctaOS_AllocateSecurityMemory(
    IN gctaOS Os,
    IN gctSIZE_T  *Bytes,
    OUT gctPOINTER *Logical,
    OUT gctPOINTER *Physical
    )
{
    gceSTATUS status;

    gcmkONERROR(gckOS_AllocateNonPagedMemory(Os->os, gcvNULL, gcvFALSE, gcvALLOC_FLAG_CONTIGUOUS, Bytes, (gctPHYS_ADDR *)Physical, Logical));

    return gcvSTATUS_OK;

OnError:
    return status;
}

gceSTATUS
gctaOS_FreeSecurityMemory(
    IN gctaOS Os,
    IN gctSIZE_T  Bytes,
    IN gctPOINTER Logical,
    OUT gctPOINTER Physical
    )
{
    gckOS_FreeNonPagedMemory(Os->os, (gctPHYS_ADDR)Physical, Logical, Bytes);
    return gcvSTATUS_OK;
}

gceSTATUS
gctaOS_AllocateNonSecurityMemory(
    IN gctaOS Os,
    IN gctSIZE_T  *Bytes,
    OUT gctPOINTER *Logical,
    OUT gctPOINTER *Physical
    )
{
    gceSTATUS status;

    gcmkONERROR(gckOS_AllocateNonPagedMemory(Os->os, gcvNULL, gcvFALSE, gcvALLOC_FLAG_CONTIGUOUS, Bytes, (gctPHYS_ADDR *)Physical, Logical));

    return gcvSTATUS_OK;

OnError:
    return status;
}

gceSTATUS
gctaOS_FreeNonSecurityMemory(
    IN gctaOS Os,
    IN gctSIZE_T  Bytes,
    IN gctPOINTER Logical,
    OUT gctPOINTER Physical
    )
{
    gckOS_FreeNonPagedMemory(Os->os, (gctPHYS_ADDR)Physical, Logical, Bytes);
    return gcvSTATUS_OK;
}

gceSTATUS
gctaOS_Allocate(
    IN gctUINT32 Bytes,
    OUT gctPOINTER *Pointer
    )
{
    return gckOS_AllocateMemory(globalTAos->os, Bytes, Pointer);
}

gceSTATUS
gctaOS_Free(
    IN gctPOINTER Pointer
    )
{
    return gckOS_FreeMemory(globalTAos->os, Pointer);
}

gceSTATUS
gctaOS_GetPhysicalAddress(
    IN gctaOS Os,
    IN gctPOINTER Logical,
    OUT gctPHYS_ADDR_T * Physical
    )
{
    gctPHYS_ADDR_T physical;
    gceSTATUS status;

    gcmkONERROR(gckOS_GetPhysicalAddress(Os->os, Logical, &physical));

    gcmkVERIFY_OK(gckOS_CPUPhysicalToGPUPhysical(Os->os, physical, &physical));

    *Physical = physical;

    return gcvSTATUS_OK;

OnError:
    return status;
}

gceSTATUS gctaOS_WriteRegister(
    IN gctaOS Os, IN gceCORE Core,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gctaOS_ReadRegister(
    IN gctaOS Os, IN gceCORE Core,
    IN gctUINT32 Address,
    IN gctUINT32 *Data
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gctaOS_MemCopy(
    IN gctUINT8_PTR Dest,
    IN gctUINT8_PTR Src,
    IN gctUINT32 Bytes
    )
{
    gckOS_MemCopy(Dest, Src, Bytes);
    return gcvSTATUS_OK;
}

gceSTATUS
gctaOS_ZeroMemory(
    IN gctUINT8_PTR Dest,
    IN gctUINT32 Bytes
    )
{
    gckOS_ZeroMemory(Dest, Bytes);
    return gcvSTATUS_OK;
}

void
gctaOS_CacheFlush(
    IN gctUINT8_PTR Dest,
    IN gctUINT32 Bytes
    )
{

}

void
gctaOS_CacheClean(
    IN gctUINT8_PTR Dest,
    IN gctUINT32 Bytes
    )
{

}

void
gctaOS_CacheInvalidate(
    IN gctUINT8_PTR Dest,
    IN gctUINT32 Bytes
    )
{

}

gceSTATUS
gctaOS_IsPhysicalSecure(
    IN gctaOS Os,
    IN gctUINT32 Physical,
    OUT gctBOOL *Secure
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gctaOS_Delay(
    IN gctaOS Os,
    IN gctUINT32 Delay
    )
{
    return gckOS_Delay(Os->os, Delay);
}

gceSTATUS
gctaOS_SetGPUPower(
    IN gctaOS Os,
    IN gctUINT32 Core,
    IN gctBOOL Clock,
    IN gctBOOL Power
    )
{
    return gcvSTATUS_OK;
}




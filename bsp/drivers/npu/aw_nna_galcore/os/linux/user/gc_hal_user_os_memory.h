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

#ifndef __gc_hal_user_os_memory_h_
#define __gc_hal_user_os_memory_h_

#ifndef _ISOC99_SOURCE
#define _ISOC99_SOURCE
#endif

#include <string.h>

/*  Fill the specified memory with 0's. */
static gcmINLINE void
gcoOS_ZeroMemory(
    IN gctPOINTER Memory,
    IN gctSIZE_T Bytes
    )
{
#if gcdSKIP_ARM_DC_INSTRUCTION
    gctINT i = 0;
    gctUINT8_PTR ptr = gcvNULL;
#endif
    /* Verify the arguments. */
    gcmASSERT(Memory != gcvNULL);
    gcmASSERT(Bytes > 0);

#if gcdSKIP_ARM_DC_INSTRUCTION
    ptr = (gctUINT8_PTR)Memory;
    for (i = 0; i < Bytes; i++)
        ptr[i] = 0;
#else
    /* Zero the memory. */
    memset(Memory, 0, Bytes);
#endif
}

static gcmINLINE void
gcoOS_MemCopyFast(
    IN gctPOINTER Destination,
    IN gctCONST_POINTER Source,
    IN gctSIZE_T Bytes
    )
{
#if gcdSKIP_ARM_DC_INSTRUCTION
    gctINT i = 0;
    gctUINT8_PTR sptr = gcvNULL;
    gctUINT8_PTR dptr = gcvNULL;
#endif
    /* Verify the arguments. */
    gcmASSERT(Destination != gcvNULL);
    gcmASSERT(Source != gcvNULL);
    gcmASSERT(Bytes > 0);

#if gcdSKIP_ARM_DC_INSTRUCTION
    sptr = (gctUINT8_PTR)Source;
    dptr = (gctUINT8_PTR)Destination;
    for (i = 0; i < Bytes; i++)
        dptr[i] = sptr[i];
#else
    if ((Bytes > 4096)
        && ((gctSIZE_T)Destination & 0xf)
        && ((gctSIZE_T)Source & 0xf))
    {
        gctUINT8    *srcAddr    = (gctUINT8*)Destination;
        gctUINT8    *dstAddr    = (gctUINT8*)Source;
        gctSIZE_T   origDst     = (gctSIZE_T)Destination;
        gctSIZE_T   size        = 0;

        size    = (gctSIZE_T)(((origDst + 0xf) & ~0xf) - origDst);
        srcAddr += size;
        dstAddr += size;

        memcpy(Destination, Source, size);
        memcpy(dstAddr, srcAddr, Bytes-size);
    }
    else
    {
        memcpy(Destination, Source, Bytes);
    }
#endif
}

/* Perform a memory copy. */
static gcmINLINE void
gcoOS_MemCopy(
    IN gctPOINTER Destination,
    IN gctCONST_POINTER Source,
    IN gctSIZE_T Bytes
    )
{
#if gcdSKIP_ARM_DC_INSTRUCTION
    gctINT i = 0;
    gctUINT8_PTR sptr = gcvNULL;
    gctUINT8_PTR dptr = gcvNULL;
#endif
    /* Verify the arguments. */
    gcmASSERT(Destination != gcvNULL);
    gcmASSERT(Source != gcvNULL);
    gcmASSERT(Bytes > 0);
    gcmASSERT((Destination >= Source && (gctCHAR*)Source + Bytes <= (gctCHAR*)Destination) ||
              (Source > Destination && (gctCHAR*)Destination + Bytes <= (gctCHAR*)Source));

#if gcdSKIP_ARM_DC_INSTRUCTION
    sptr = (gctUINT8_PTR)Source;
    dptr = (gctUINT8_PTR)Destination;
    for (i = 0; i < Bytes; i++)
        dptr[i] = sptr[i];
#else
    /* Copy the memory. */
    memcpy(Destination, Source, Bytes);
#endif
}

/* Perform a memory move. */
static gcmINLINE void
gcoOS_MemMove(
    IN gctPOINTER Destination,
    IN gctCONST_POINTER Source,
    IN gctSIZE_T Bytes
    )
{
    /* Verify the arguments. */
    gcmASSERT(Destination != gcvNULL);
    gcmASSERT(Source != gcvNULL);
    gcmASSERT(Bytes > 0);

    /* Move the memory. */
    memmove(Destination, Source, Bytes);
}

/* Perform a memory fill. */
static gcmINLINE void
gcoOS_MemFill(
    IN gctPOINTER Memory,
    IN gctUINT8 Filler,
    IN gctSIZE_T Bytes
    )
{
    /* Verify the arguments. */
    gcmASSERT(Memory != gcvNULL);
    gcmASSERT(Bytes > 0);

    /* Fill the memory. */
    memset(Memory, Filler, Bytes);
}

static gcmINLINE gctSIZE_T
gcoOS_StrLen(
    IN gctCONST_STRING String,
    OUT gctSIZE_T * Length
    )
{
    gctSIZE_T len;

    /* Verify the arguments. */
    gcmASSERT(String != gcvNULL);

    /* Get the length of the string. */
    len = (gctSIZE_T) strlen(String);
    if (Length)
    {
        *Length = len;
    }

    return len;
}

#endif


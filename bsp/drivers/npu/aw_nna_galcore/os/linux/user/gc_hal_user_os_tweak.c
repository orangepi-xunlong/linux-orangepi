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

#include <gc_hal.h>


#if gcdSKIP_ARM_DC_INSTRUCTION


void* memmove(void *dest, const void *src, size_t count)
{
    unsigned char *ps = (unsigned char*)src;
    unsigned char *pd = (unsigned char*)dest;
    if (ps && pd && count > 0)
    {
        while (count-- > 0)
        {
            *pd++ = *ps++;
        }
    }
    return dest;
}


void* memcpy(void *dest, const void *src, size_t count)
{
    return memmove(dest, src, count);
}

void* memset(void *dest, int ch, size_t count)
{
    unsigned char *p = (unsigned char*)dest;
    if (p && count > 0)
    {
        while (count-- > 0)
        {
            *p++ = (unsigned char)ch;
        }
    }
    return dest;
}

#endif

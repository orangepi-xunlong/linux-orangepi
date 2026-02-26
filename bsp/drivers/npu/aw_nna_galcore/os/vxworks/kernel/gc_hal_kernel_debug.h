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

#ifndef __gc_hal_kernel_debug_h_
#define __gc_hal_kernel_debug_h_

#include <gc_hal_kernel_vxworks.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
****************************** OS-dependent Macros *****************************
\******************************************************************************/

typedef va_list gctARGUMENTS;

#define gcmkARGUMENTS_START(Arguments, Pointer) \
    va_start(Arguments, Pointer)

#define gcmkARGUMENTS_END(Arguments) \
    va_end(Arguments)

#define gcmkARGUMENTS_ARG(Arguments, Type) \
    va_arg(Arguments, Type)

#define gcmkDECLARE_MUTEX(__mutex__) \
    pthread_mutex_t __mutex__ \

#define gcmkMUTEX_LOCK(__mutex__) \
    pthread_mutex_lock(&__mutex__)

#define gcmkMUTEX_UNLOCK(__mutex__) \
    pthread_mutex_unlock(&__mutex__)

#define gcmkGETPROCESSID() \
    taskIdSelf()

#define gcmkGETTHREADID() \
    pthread_self()

#define gcmkOUTPUT_STRING(String) \
    gcmkPRINT(String);

#define gcmkSPRINTF(Destination, Size, ...) \
    snprintf(Destination, Size, __VA_ARGS__)

#define gcmkVSPRINTF(Destination, Size, Message, Arguments) \
    vsnprintf(Destination, Size, Message, *((va_list*)Arguments))

#define gcmkSTRCATSAFE(Destination, Size, String) \
    strncat(Destination, String, (Size) - 1)

#define gcmkMEMCPY(Destination, Source, Size) \
    memcpy(Destination, Source, Size)

#define gcmkSTRLEN(String) \
    strlen(String)

/* If not zero, forces data alignment in the variable argument list
   by its individual size. */
#define gcdALIGNBYSIZE      1

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_kernel_debug_h_ */

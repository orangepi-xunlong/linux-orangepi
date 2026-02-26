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

#ifndef _gc_hal_kernel_mutex_h_
#define _gc_hal_kernel_mutex_h_

#include "gc_hal.h"
#include "pthread.h"

#define gckOS_CreateMutex(Os, Mutex)                                \
({                                                                  \
    gceSTATUS _status;                                              \
    gcmkHEADER_ARG("Os=0x%X", Os);                                  \
                                                                    \
    /* Validate the arguments. */                                   \
    gcmkVERIFY_OBJECT(Os, gcvOBJ_OS);                               \
    gcmkVERIFY_ARGUMENT(Mutex != gcvNULL);                          \
                                                                    \
    /* Allocate the mutex structure. */                             \
    _status = gckOS_Allocate(Os, gcmSIZEOF(pthread_mutex_t), Mutex);   \
                                                                    \
    if (gcmIS_SUCCESS(_status))                                     \
    {                                                               \
        /* Initialize the mutex. */                                 \
        pthread_mutex_init(*(pthread_mutex_t**)Mutex,0);            \
    }                                                               \
                                                                    \
    /* Return status. */                                            \
    gcmkFOOTER_ARG("*Mutex=0x%X", Mutex);         \
    _status;                                                        \
})

#endif




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

#ifndef __gc_hal_user_os_atomic_h_
#define __gc_hal_user_os_atomic_h_

/*
** GCC supports sync_* built-in functions since 4.1.2
** For those gcc variation whose verion is newer than 4.1.2 but
** still doesn't support sync_*, build driver with
** gcdBUILTIN_ATOMIC_FUNCTIONS = 0 to override version check.
*/
#ifndef gcdBUILTIN_ATOMIC_FUNCTIONS
#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ >= 40102)
#define gcdBUILTIN_ATOMIC_FUNCTIONS 1
#else
#define gcdBUILTIN_ATOMIC_FUNCTIONS 0
#endif
#endif

#if !gcdBUILTIN_ATOMIC_FUNCTIONS
#include <pthread.h>
#endif

struct gcsATOM
{
    /* Counter. */
    gctINT32 counter;

#if !gcdBUILTIN_ATOMIC_FUNCTIONS
    /* Mutex. */
    pthread_mutex_t mutex;
#endif
};

#if !gcdBUILTIN_ATOMIC_FUNCTIONS
#define gcmATOM_INITIALIZER \
    { \
        0, \
        PTHREAD_MUTEX_INITIALIZER \
    };
#else
#define gcmATOM_INITIALIZER \
    { \
        0 \
    };
#endif

#endif

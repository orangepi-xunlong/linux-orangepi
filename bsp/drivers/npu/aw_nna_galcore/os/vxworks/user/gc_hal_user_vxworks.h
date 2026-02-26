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

#ifndef __gc_hal_user_vxworks_h_
#define __gc_hal_user_vxworks_h_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <ioLib.h>
#include <taskLib.h>
#include <sysLib.h>
#include <sockLib.h>
#include <selectLib.h>

#include "gc_hal_user.h"

#ifndef _ISOC99_SOURCE
#  define _ISOC99_SOURCE
#endif

#ifndef _XOPEN_SOURCE
#  define _XOPEN_SOURCE 501
#endif

#endif /* __gc_hal_user_vxworks_h_ */

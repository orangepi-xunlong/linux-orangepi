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

#ifndef __gc_hal_user_debug_h_
#define __gc_hal_user_debug_h_

#include <stdarg.h>

typedef va_list                                 gctARGUMENTS;
#define gcmARGUMENTS_START(argument, pointer)   va_start(argument, pointer)
#define gcmARGUMENTS_END(argument)              va_end(argument)

#endif /* __gc_hal_user_debug_h_ */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __TYPES_UTILS_H__
#define __TYPES_UTILS_H__

#include <asm-generic/int-ll64.h>
#include <asm-generic/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
/// Define isp software type acronym
typedef double f64;
typedef float f32;
typedef __s64 s64;
typedef __s32 s32;
typedef __s16 s16;
typedef __s8 s8;
typedef __u64 u64;
typedef __u32 u32;
typedef __u16 u16;
typedef __u8 u8;

/// Define common type
#ifndef BOOL
#define BOOL u8
#endif

#ifndef TRUE
#define TRUE 1U
#endif

#ifndef FALSE
#define FALSE 0U
#endif

#ifndef NULL
#define NULL 0U
#endif

#endif

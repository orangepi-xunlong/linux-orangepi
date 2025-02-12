/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __CPP_COMPAT_IOCTL32_H__
#define __CPP_COMPAT_IOCTL32_H__

#include <media/v4l2-subdev.h>

long x1_cpp_compat_ioctl32(struct file *file, unsigned int cmd, unsigned long arg);
#endif /* end of include guard: __CPP_COMPAT_IOCTL32_H__ */

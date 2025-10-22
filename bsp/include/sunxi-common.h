/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's Common Helper Functions
 *
 * Copyright (c) 2023, Martin <wuyan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef __SUNXI_COMMON_H__
#define __SUNXI_COMMON_H__

#include <sunxi-log.h>
#include <sunxi-bitops.h>
#include <linux/timekeeping.h>

static inline s64 get_elapsed_time_ns(void)
{
	static struct timespec64 old;  /* the initial value is 0 */
	struct timespec64 now;
	struct timespec64 diff;

	ktime_get_real_ts64(&now);
	if (timespec64_to_ns(&old) == 0)  /* first run? */
		old = now;
	diff = timespec64_sub(now, old);
	old = now;

	return timespec64_to_ns(&diff);
}

#define get_elapsed_time_us()	(get_elapsed_time_ns() / 1000)
#define get_elapsed_time_ms()	(get_elapsed_time_us() / 1000)

#endif  /* __SUNXI_COMMON_H__ */

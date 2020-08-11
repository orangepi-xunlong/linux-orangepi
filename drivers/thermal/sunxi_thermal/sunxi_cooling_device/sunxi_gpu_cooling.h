/*
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: JiaRui Xiao <xiaojiarui@allwinnertech.com>
 *
 * allwinner sunxi thermal gpu cooling device data struct.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SUNXI_GPU_COOLING_H
#define SUNXI_GPU_COOLING_H

#include <linux/thermal.h>

#define GPU_FREQ_TABLE_MAX	(10)

struct sunxi_gpu_cooling_device {
	struct device *dev;
	struct thermal_cooling_device *cool_dev;
	int (*cool)(int);
	u32 cooling_state;
	u32 state_num;
	u32 gpu_freq_limit;
	u32 gpu_freq_roof;
	u32 gpu_freq_floor;
	u32 freq_table[GPU_FREQ_TABLE_MAX];
	spinlock_t lock;
};

#endif /* SUNXI_GPU_COOLING_H */

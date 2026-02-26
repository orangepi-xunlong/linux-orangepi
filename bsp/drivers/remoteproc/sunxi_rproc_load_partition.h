/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's rproc rsc helper internal interface
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: shihongfu <fanjiahao@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SUNXI_RPROC_LOAD_PARTITION_H__
#define __SUNXI_RPROC_LOAD_PARTITION_H__
#include <linux/remoteproc.h>

int load_from_partition(const char *partition, void *dst, size_t size);

#endif /* __SUNXI_RPROC_LOAD_PARTITION_H__ */

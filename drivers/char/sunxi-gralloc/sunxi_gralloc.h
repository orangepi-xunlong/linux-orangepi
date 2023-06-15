/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef _SUNXI_GRALLOC_H_
#define _SUNXI_GRALLOC_H_

#include <linux/slab.h>

#include <linux/types.h>
#include <linux/ioctl.h>


struct gralloc_buffer {
	char name[200];
	unsigned long long unique_id;

	unsigned int width;
	unsigned int height;
	unsigned int format;
};

#define GRALLOC_IOC_MAGIC 'G'
#define GRALLOC_IO(nr)          _IO(GRALLOC_IOC_MAGIC, nr)
#define GRALLOC_IOR(nr, size)   _IOR(GRALLOC_IOC_MAGIC, nr, size)
#define GRALLOC_IOW(nr, size)   _IOW(GRALLOC_IOC_MAGIC, nr, size)
#define GRALLOC_IOWR(nr, size)  _IOWR(GRALLOC_IOC_MAGIC, nr, size)
#define GRALLOC_IOCTL_NR(n)     _IOC_NR(n)

#define GRALLOC_IOC_ALLOCATE_BUFFER GRALLOC_IOW(0x1, struct gralloc_buffer)
#define GRALLOC_IOC_IMPORT_BUFFER GRALLOC_IOW(0x2, struct gralloc_buffer)
#define GRALLOC_IOC_RELEASE_BUFFER GRALLOC_IOW(0x3, struct gralloc_buffer)
#define GRALLOC_IOC_LOCK_BUFFER GRALLOC_IOW(0x4, struct gralloc_buffer)
#define GRALLOC_IOC_UNLOCK_BUFFER GRALLOC_IOW(0x5, struct gralloc_buffer)
#endif

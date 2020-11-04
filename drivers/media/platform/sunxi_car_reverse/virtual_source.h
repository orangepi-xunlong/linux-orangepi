/*
 * Fast car reverse head file
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#ifndef __VIRTUAL_SOURCE_H__
#define __VIRTUAL_SOURCE_H__

#include <linux/types.h>

struct display_info {
	int width;
	int height;
	int format;
	int offset;
	int align;
};

#define VIRTUAL_SOURCE_MAGIC	'V'
#define VS_IOC_GET_BUFFER_SIZE	_IOR(VIRTUAL_SOURCE_MAGIC, 0, unsigned int *)
#define VS_IOC_UPDATE_DISPLAY	_IOW(VIRTUAL_SOURCE_MAGIC, 1, struct display_info *)

#endif

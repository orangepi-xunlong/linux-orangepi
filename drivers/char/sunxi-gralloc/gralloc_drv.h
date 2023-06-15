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
#ifndef _GRALLOC_DRIVER_H_
#define _GRALLOC_DRIVER_H_
int gralloc_allocate_buffer(void *data);
int gralloc_import_buffer(void *data);
int gralloc_release_buffer(void *data);
int gralloc_lock_buffer(void *data);
int gralloc_unlock_buffer(void *data);
int gralloc_dump(char *buf);
#endif

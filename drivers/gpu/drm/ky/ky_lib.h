// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_LIB_H_
#define _KY_LIB_H_

#include <drm/drm_print.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_mode_object.h>
#include <linux/list.h>

struct ops_entry {
	const char *ver;
	void *ops;
};

struct ops_list {
	struct list_head head;
	struct ops_entry *entry;
};

int str_to_u32_array(const char *p, u32 base, u32 array[]);
int str_to_u8_array(const char *p, u32 base, u8 array[]);
int dump_bmp32(const char *p, u32 width, u32 height,
		bool bgra, const char *filename);

void *disp_ops_attach(const char *str, struct list_head *head);
int disp_ops_register(struct ops_entry *entry, struct list_head *head);

struct device *ky_disp_pipe_get_by_port(struct device *dev, int port);
struct device *ky_disp_pipe_get_input(struct device *dev);
struct device *ky_disp_pipe_get_output(struct device *dev);
int ky_atomic_replace_property_blob_from_id(struct drm_device *dev,
					 struct drm_property_blob **blob,
					 uint64_t blob_id,
					 ssize_t expected_size,
					 ssize_t expected_elem_size,
					 bool *replaced);

#endif


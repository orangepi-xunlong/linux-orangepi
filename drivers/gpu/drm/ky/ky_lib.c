// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include "ky_lib.h"

struct bmp_header {
	u16 magic;
	u32 size;
	u32 unused;
	u32 start;
} __attribute__((__packed__));

struct dib_header {
	u32 size;
	u32 width;
	u32 height;
	u16 planes;
	u16 bpp;
	u32 compression;
	u32 data_size;
	u32 h_res;
	u32 v_res;
	u32 colours;
	u32 important_colours;
	u32 red_mask;
	u32 green_mask;
	u32 blue_mask;
	u32 alpha_mask;
	u32 colour_space;
	u32 unused[12];
} __attribute__((__packed__));

int str_to_u32_array(const char *p, u32 base, u32 array[])
{
	const char *start = p;
	char str[12];
	int length = 0;
	int i, ret;

	pr_info("input: %s", p);

	for (i = 0 ; i < 255; i++) {
		while (*p == ' ')
			p++;
		if (*p == '\0')
			break;
		start = p;

		while ((*p != ' ') && (*p != '\0'))
			p++;

		if ((p - start) >= sizeof(str))
			break;

		memset(str, 0, sizeof(str));
		memcpy(str, start, p - start);

		ret = kstrtou32(str, base, &array[i]);
		if (ret) {
			DRM_ERROR("input format error\n");
			break;
		}

		length++;
	}

	return length;
}
EXPORT_SYMBOL_GPL(str_to_u32_array);

int str_to_u8_array(const char *p, u32 base, u8 array[])
{
	const char *start = p;
	char str[12];
	int length = 0;
	int i, ret;

	pr_info("input: %s", p);

	for (i = 0 ; i < 255; i++) {
		while (*p == ' ')
			p++;
		if (*p == '\0')
			break;
		start = p;

		while ((*p != ' ') && (*p != '\0'))
			p++;

		if ((p - start) >= sizeof(str))
			break;

		memset(str, 0, sizeof(str));
		memcpy(str, start, p - start);

		ret = kstrtou8(str, base, &array[i]);
		if (ret) {
			DRM_ERROR("input format error\n");
			break;
		}

		length++;
	}

	return length;
}
EXPORT_SYMBOL_GPL(str_to_u8_array);

#if IS_ENABLED(CONFIG_GKI_FIX_WORKAROUND)
static struct file *gki_filp_open(const char *filename, int flags, umode_t mode)
{
	return 0;
}
static ssize_t gki_vfs_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
	return 0;
}
#endif

void *disp_ops_attach(const char *str, struct list_head *head)
{
	struct ops_list *list;
	const char *ver;

	list_for_each_entry(list, head, head) {
		ver = list->entry->ver;
		if (!strcmp(str, ver))
			return list->entry->ops;
	}

	DRM_ERROR("attach disp ops %s failed\n", str);
	return NULL;
}
EXPORT_SYMBOL_GPL(disp_ops_attach);

int disp_ops_register(struct ops_entry *entry, struct list_head *head)
{
	struct ops_list *list;

	list = kzalloc(sizeof(struct ops_list), GFP_KERNEL);
	if (!list)
		return -ENOMEM;

	list->entry = entry;
	list_add(&list->head, head);

	return 0;
}
EXPORT_SYMBOL_GPL(disp_ops_register);

struct device *ky_disp_pipe_get_by_port(struct device *dev, int port)
{
	struct device_node *np = dev->of_node;
	struct device_node *endpoint;
	struct device_node *remote_node;
	struct platform_device *remote_pdev;

	endpoint = of_graph_get_endpoint_by_regs(np, port, 0);
	if (!endpoint) {
		DRM_ERROR("%s/port%d/endpoint0 was not found\n",
			  np->full_name, port);
		return NULL;
	}

	remote_node = of_graph_get_remote_port_parent(endpoint);
	if (!remote_node) {
		DRM_ERROR("device node was not found by endpoint0\n");
		return NULL;
	}

	remote_pdev = of_find_device_by_node(remote_node);
	if (remote_pdev == NULL) {
		DRM_ERROR("find %s platform device failed\n",
			  remote_node->full_name);
		return NULL;
	}

	return &remote_pdev->dev;
}
EXPORT_SYMBOL_GPL(ky_disp_pipe_get_by_port);

struct device *ky_disp_pipe_get_input(struct device *dev)
{
	return ky_disp_pipe_get_by_port(dev, 1);
}
EXPORT_SYMBOL_GPL(ky_disp_pipe_get_input);

struct device *ky_disp_pipe_get_output(struct device *dev)
{
	return ky_disp_pipe_get_by_port(dev, 0);
}
EXPORT_SYMBOL_GPL(ky_disp_pipe_get_output);

/*
 * copy from drm opensource, change name from drm_atomic_replace_property_blob_from_id
 * to ky_atomic_replace_property_blob_from_id
 */
int ky_atomic_replace_property_blob_from_id(struct drm_device *dev,
					 struct drm_property_blob **blob,
					 uint64_t blob_id,
					 ssize_t expected_size,
					 ssize_t expected_elem_size,
					 bool *replaced)
{
	struct drm_property_blob *new_blob = NULL;

	if (blob_id != 0) {
		new_blob = drm_property_lookup_blob(dev, blob_id);
		if (new_blob == NULL)
			return -EINVAL;

		if (expected_size > 0 &&
		    new_blob->length != expected_size) {
			drm_property_blob_put(new_blob);
			return -EINVAL;
		}
		if (expected_elem_size > 0 &&
		    new_blob->length % expected_elem_size != 0) {
			drm_property_blob_put(new_blob);
			return -EINVAL;
		}
	}

	*replaced |= drm_property_replace_blob(blob, new_blob);
	drm_property_blob_put(new_blob);

	return 0;
}
EXPORT_SYMBOL_GPL(ky_atomic_replace_property_blob_from_id);

MODULE_DESCRIPTION("display common API library");
MODULE_LICENSE("GPL");


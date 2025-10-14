// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 */

#include "hisysevent_raw_data.h"

#include <linux/cred.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#define EXPAND_BUF_SIZE 100

static int raw_data_init(struct hisysevent_raw_data *raw_data)
{
	if (!raw_data) {
		pr_err("raw data is null");
		return -EINVAL;
	}

	raw_data->data = kzalloc(EXPAND_BUF_SIZE, GFP_KERNEL);
	if (!(raw_data->data)) {
		pr_err("failed to allocate memory for raw data");
		return -ENOMEM;
	}

	raw_data->capacity = EXPAND_BUF_SIZE;
	raw_data->len = 0;

	return 0;
}

int raw_data_update(struct hisysevent_raw_data *dest, u8 *src, unsigned int len,
		    unsigned int pos)
{
	if (!dest) {
		pr_err("try to update a data which is null");
		return -EINVAL;
	}
	if (!src || len == 0) {
		pr_debug("do nothing");
		return 0;
	}
	if (dest->len < pos) {
		pr_err("try to update on an invalid position");
		return -EINVAL;
	}
	if ((pos + len) > dest->capacity) {
		unsigned int expanded_size;
		u8 *resize_data;

		expanded_size = (len > EXPAND_BUF_SIZE) ? len : EXPAND_BUF_SIZE;
		resize_data = kmalloc(dest->capacity + expanded_size, GFP_KERNEL);
		if (!resize_data) {
			pr_err("failed to expand memory for raw data");
			return -ENOMEM;
		}
		memcpy(resize_data, dest->data, dest->len);
		dest->capacity += expanded_size;
		if (!dest->data)
			kfree(dest->data);
		dest->data = resize_data;
	}

	// append new data
	memcpy(dest->data + pos, src, len);
	if ((pos + len) > dest->len)
		dest->len = pos + len;
	return 0;
}
EXPORT_SYMBOL_GPL(raw_data_update);

int raw_data_append(struct hisysevent_raw_data *dest, u8 *src, unsigned int len)
{
	return raw_data_update(dest, src, len, dest->len);
}
EXPORT_SYMBOL_GPL(raw_data_append);

struct hisysevent_raw_data*
raw_data_create(void)
{
	struct hisysevent_raw_data *raw_data;

	raw_data = kzalloc(sizeof(*raw_data), GFP_KERNEL);
	if (!raw_data)
		return NULL;

	if (raw_data_init(raw_data) != 0)
		goto create_err;

	return raw_data;

create_err:
	raw_data_destroy(raw_data);
	return NULL;
}
EXPORT_SYMBOL_GPL(raw_data_create);

void raw_data_destroy(struct hisysevent_raw_data *raw_data)
{
	if (!raw_data) {
		pr_err("try to destroy an invalid raw data");
		return;
	}

	if (!(raw_data->data))
		kfree(raw_data->data);

	kfree(raw_data);
}
EXPORT_SYMBOL_GPL(raw_data_destroy);

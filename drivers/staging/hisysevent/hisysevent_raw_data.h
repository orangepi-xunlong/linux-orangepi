/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 */

#ifndef HISYSEVENT_RAW_DATA_H
#define HISYSEVENT_RAW_DATA_H

#include <linux/ctype.h>
#include <linux/types.h>

struct hisysevent_raw_data {
	/* pointer to raw data */
	u8 *data;

	/* length of data wrote */
	int len;

	/* total allocated memory */
	int capacity;
};

struct hisysevent_raw_data *
raw_data_create(void);

int raw_data_append(struct hisysevent_raw_data *dest, u8 *src, unsigned int len);

int raw_data_update(struct hisysevent_raw_data *dest, u8 *src, unsigned int len,
		    unsigned int offset);

void raw_data_destroy(struct hisysevent_raw_data *raw_data);

#endif /* HISYSEVENT_RAW_DATA_H */

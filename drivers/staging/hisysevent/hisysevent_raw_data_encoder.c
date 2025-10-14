// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 */

#include "hisysevent_raw_data_encoder.h"

#include <linux/cred.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uio.h>
#include <linux/vmalloc.h>

#define TAG_BYTE_OFFSET 5
#define TAG_BYTE_BOUND (BIT(TAG_BYTE_OFFSET))
#define TAG_BYTE_MASK (TAG_BYTE_BOUND - 1)

#define NON_TAG_BYTE_OFFSET 7
#define NON_TAG_BYTE_BOUND (BIT(NON_TAG_BYTE_OFFSET))
#define NON_TAG_BYTE_MASK (NON_TAG_BYTE_BOUND - 1)

enum hisysevent_encode_type {
	// zigzag varint
	VARINT = 0,

	// length delimited
	LENGTH_DELIMITED = 1,
};

#pragma pack(1)

struct param_value_type {
	/* array flag */
	u8 is_array: 1;

	/* type of parameter value */
	u8 value_type: 4;

	/* byte count of parameter value */
	u8 value_byte_cnt: 3;
};

#pragma pack()

static u8 encode_tag(u8 type)
{
	return type << (TAG_BYTE_OFFSET + 1);
}

static int unsigned_varint_code(struct hisysevent_raw_data *data,
				enum hisysevent_encode_type type, u64 val)
{
	u8 cpy_val;

	cpy_val = encode_tag((u8)type) |
			     ((val < TAG_BYTE_BOUND) ? 0 : TAG_BYTE_BOUND) |
			     (u8)(val & TAG_BYTE_MASK);
	if (raw_data_append(data, (u8 *)(&cpy_val), sizeof(u8)) != 0)
		return -EINVAL;

	val >>= TAG_BYTE_OFFSET;
	while (val > 0) {
		cpy_val = ((val < NON_TAG_BYTE_BOUND) ? 0 : NON_TAG_BYTE_BOUND) |
			   (u8)(val & NON_TAG_BYTE_MASK);
		if (raw_data_append(data, (u8 *)(&cpy_val), sizeof(u8)) != 0)
			return -EINVAL;

		val >>= NON_TAG_BYTE_OFFSET;
	}
	return 0;
}

static int signed_varint_encode(struct hisysevent_raw_data *data,
				enum hisysevent_encode_type type, s64 val)
{
	u64 uval;

	uval = (val << 1) ^ (val >> ((sizeof(val) << 3) - 1)); // zigzag encode
	return unsigned_varint_code(data, type, uval);
}

int key_value_type_encode(struct hisysevent_raw_data *data, u8 is_array, u8 type,
			  u8 count)
{
	struct param_value_type value_type;

	value_type.is_array = is_array;
	value_type.value_type = type;
	value_type.value_byte_cnt = count;

	if (raw_data_append(data, (u8 *)(&value_type),
			    sizeof(struct param_value_type)) != 0)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL_GPL(key_value_type_encode);

int str_length_delimited_encode(struct hisysevent_raw_data *data, const char *str)
{
	u64 length;

	length = (u64)strlen(str);
	if (unsigned_varint_code(data, LENGTH_DELIMITED, length) != 0)
		return -EINVAL;

	if (raw_data_append(data, (u8 *)(str), length) != 0)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL_GPL(str_length_delimited_encode);

int int64_t_varint_encode(struct hisysevent_raw_data *raw_data, s64 val)
{
	return signed_varint_encode(raw_data, VARINT, val);
}
EXPORT_SYMBOL_GPL(int64_t_varint_encode);

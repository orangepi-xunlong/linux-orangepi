/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 */

#ifndef HISYSEVENT_RAW_DATA_ENCODER_H
#define HISYSEVENT_RAW_DATA_ENCODER_H

#include <linux/ctype.h>
#include <linux/types.h>

#include "hisysevent_raw_data.h"

int key_value_type_encode(struct hisysevent_raw_data *data, u8 is_array, u8 type,
			  u8 count);

int str_length_delimited_encode(struct hisysevent_raw_data *data, const char *str);

int int64_t_varint_encode(struct hisysevent_raw_data *data, s64 val);

#endif /* HISYSEVENT_RAW_DATA_ENCODER_H */

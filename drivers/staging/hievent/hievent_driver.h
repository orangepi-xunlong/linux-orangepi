/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 Huawei Technologies Co., Ltd. All rights reserved.
 */

#ifndef HIEVENT_DRIVER_H
#define HIEVENT_DRIVER_H

#include <linux/types.h>

#define CHECK_CODE 0x7BCDABCD

struct idap_header {
	char level;
	char category;
	char log_type;
	char sn;
};

int hievent_write_internal(const char *buffer, size_t buf_len);

#endif /* HIEVENT_DRIVER_H */

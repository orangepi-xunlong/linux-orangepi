/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 */

#ifndef HISYSEVENT_BUILDER_H
#define HISYSEVENT_BUILDER_H

#include <dfx/hiview_hisysevent.h>

#include <linux/ctype.h>
#include <linux/types.h>

#include "hisysevent_raw_data_encoder.h"
#include "hisysevent_raw_data.h"

#define MAX_DOMAIN_LENGTH 16
#define MAX_EVENT_NAME_LENGTH 32

#pragma pack(1)

struct hisysevent_header {
	/* event domain */
	char domain[MAX_DOMAIN_LENGTH + 1];

	/* event name */
	char name[MAX_EVENT_NAME_LENGTH + 1];

	/* event timestamp */
	u64 timestamp;

	/* time zone */
	u8 time_zone;

	/* user id */
	u32 uid;

	/* process id */
	u32 pid;

	/* thread id */
	u32 tid;

	/* event hash code*/
	u64 id;

	/* event type */
	u8 type: 2; // enum hisysevent_type.

	/* trace info flag*/
	u8 is_open_trace: 1;
};

#pragma pack()

struct hisysevent_params {
	/* total count of parameters */
	s32 total_cnt;

	/* content of parameters */
	struct hisysevent_raw_data *raw_data;
};

/* hisysevent builder struct */
struct hisysevent_builder {
	/* common header */
	struct hisysevent_header header;

	/* customized parameters*/
	struct hisysevent_params params;
};

struct hisysevent_builder *
hisysevent_builder_create(const char *domain, const char *name, enum hisysevent_type type);

void hisysevent_builder_destroy(struct hisysevent_builder *builder);

int hisysevent_builder_put_integer(struct hisysevent_builder *builder, const char *key,
				   s64 value);

int hisysevent_builder_put_string(struct hisysevent_builder *builder, const char *key,
				  const char *value);

int hisysevent_builder_build(struct hisysevent_builder *builder,
			     struct hisysevent_raw_data *raw_data);

#endif /* HISYSEVENT_BUILDER_H */

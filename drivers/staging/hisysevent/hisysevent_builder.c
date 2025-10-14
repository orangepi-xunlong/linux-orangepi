// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Huawei Technologies Co., Ltd. All rights reserved.
 */

#include "hisysevent_builder.h"

#include <linux/cred.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/uio.h>
#include <linux/vmalloc.h>
#include <asm/current.h>

#define MAX_PARAM_NAME_LENGTH 48

#define PARAM_STR_MAX_LEN	1536 // 1.5KB
#define HISYSEVENT_INFO_BUF_LEN (2048 - 6)  // 2KB - 6 (read_gap)

#define TIME_ZONE_LEN 6
#define TIME_ZONE_TOTAL_CNT 38
#define DEFAULT_TZ_POS 14

#define MINUTE_TO_SECS 60
#define SEC_TO_MILLISEC 1000
#define MILLISEC_TO_NANOSEC (1000 * 1000)

#define MAX_PARAM_NUMBER 128

#define HISYSEVENT_HEADER_SIZE sizeof(struct hisysevent_header)

enum value_type {
	/* int64_t */
	INT64 = 8,

	/* string */
	STRING = 12,
};

static int parse_time_zone(const char *time_zone_formatted)
{
	int ret;

	static const char *const time_zone_list[] = {
		"-0100", "-0200", "-0300", "-0330", "-0400", "-0500", "-0600",
		"-0700", "-0800", "-0900", "-0930", "-1000", "-1100", "-1200",
		"+0000", "+0100", "+0200", "+0300", "+0330", "+0400", "+0430",
		"+0500", "+0530", "+0545", "+0600", "+0630", "+0700", "+0800",
		"+0845", "+0900", "+0930", "+1000", "+1030", "+1100", "+1200",
		"+1245", "+1300", "+1400"
	};
	if (!time_zone_formatted)
		return DEFAULT_TZ_POS;

	ret = match_string(time_zone_list, TIME_ZONE_LEN, time_zone_formatted);
	if (ret < 0)
		return DEFAULT_TZ_POS;

	return ret;
}

static void hisysevent_builder_set_time(struct hisysevent_header *header)
{
	struct timespec64 ts;
	struct timezone tz = sys_tz;
	int tz_index = 0;
	char time_zone[TIME_ZONE_LEN];
	int tz_hour;
	int tz_min;
	long long millisecs = 0;

	ktime_get_real_ts64(&ts);
	millisecs = ts.tv_sec * SEC_TO_MILLISEC + ts.tv_nsec / MILLISEC_TO_NANOSEC;
	header->timestamp = (u64)millisecs;

	tz_hour = (-tz.tz_minuteswest) / MINUTE_TO_SECS;
	time_zone[tz_index++] = tz_hour >= 0 ? '+' : '-';
	tz_min = (-tz.tz_minuteswest) % MINUTE_TO_SECS;
	sprintf(&time_zone[tz_index], "%02u%02u", abs(tz_hour), abs(tz_min));
	time_zone[TIME_ZONE_LEN - 1] = '\0';
	header->time_zone = (u8)parse_time_zone(time_zone);
}

static bool is_valid_num_of_param(struct hisysevent_params *params)
{
	if (!params)
		return false;

	return params->total_cnt < MAX_PARAM_NUMBER;
}

static bool is_valid_string(const char *str, unsigned int max_len)
{
	unsigned int len = 0;
	unsigned int i;

	if (!str)
		return false;

	len = strlen(str);
	if (len == 0 || len > max_len)
		return false;

	for (i = 0; i < len; i++) {
		if (!isalpha(str[i]) && str[i] != '_')
			return false;
	}
	return true;
}

static int hisysevent_init_header(struct hisysevent_header *header, const char *domain,
				  const char *name, enum hisysevent_type type)
{
	if (!is_valid_string(domain, MAX_DOMAIN_LENGTH) ||
	    !is_valid_string(name, MAX_EVENT_NAME_LENGTH)) {
		pr_err("domain or name is invalid");
		return -EINVAL;
	}

	strcpy(header->domain, domain);
	strcpy(header->name, name);

	header->type = (u8)(type - 1);
	header->pid = (u32)current->pid;
	header->tid = (u32)current->tgid;
	header->uid = (u32)current_uid().val;
	header->is_open_trace = 0; // in kernel, this value is always 0

	hisysevent_builder_set_time(header);
	if (!(header->time_zone)) {
		pr_err("failed to parse the time zone");
		goto init_error;
	}

	pr_debug("create hisysevent succeed, domain=%s, name=%s, type=%d",
		header->domain, header->name, (header->type + 1));

	return 0;

init_error:
	memset(header, 0, sizeof(*header));
	return -EINVAL;
}

static int hisysevent_init_params(struct hisysevent_params *params)
{
	if (!params) {
		pr_err("params is null");
		return -EINVAL;
	}

	params->raw_data = raw_data_create();
	if (!(params->raw_data))
		return -EINVAL;

	params->total_cnt = 0;
	return 0;
}

static void hisysevent_params_destroy(struct hisysevent_params *params)
{
	if (!params) {
		pr_err("params is null");
		return;
	}
	raw_data_destroy(params->raw_data);
}

static bool hisysevent_check_params_validity(struct hisysevent_builder *builder)
{
	if (!builder) {
		pr_err("builder is null");
		return false;
	}

	if (!is_valid_num_of_param(&builder->params)) {
		pr_err("number of param is invalid");
		return false;
	}

	return true;
}

struct hisysevent_builder*
hisysevent_builder_create(const char *domain, const char *name, enum hisysevent_type type)
{
	struct hisysevent_builder *builder;

	builder = kzalloc(sizeof(*builder), GFP_KERNEL);
	if (!builder)
		return NULL;

	// header struct initialize
	if (hisysevent_init_header(&builder->header, domain, name, type) != 0)
		goto create_err;

	// parameters struct initialize
	if (hisysevent_init_params(&builder->params) != 0)
		goto create_err;

	return builder;

create_err:
	hisysevent_builder_destroy(builder);
	return NULL;
}
EXPORT_SYMBOL_GPL(hisysevent_builder_create);

void hisysevent_builder_destroy(struct hisysevent_builder *builder)
{
	if (!builder) {
		pr_err("try to destroy an invalid builder");
		return;
	}

	// destroy hisysevent parameters
	hisysevent_params_destroy(&builder->params);

	kfree(builder);
}
EXPORT_SYMBOL_GPL(hisysevent_builder_destroy);

int hisysevent_builder_put_integer(struct hisysevent_builder *builder, const char *key,
				   s64 value)
{
	int ret;
	struct hisysevent_raw_data *raw_data;

	if (!is_valid_string(key, MAX_PARAM_NAME_LENGTH)) {
		pr_err("try to put an invalid key");
		return -EINVAL;
	}
	if (!hisysevent_check_params_validity(builder))
		return -EINVAL;

	raw_data = raw_data_create();
	if (!raw_data) {
		pr_err("failed to create raw data for an new integer parameter");
		return -ENOMEM;
	}

	ret = -EINVAL;
	if ((str_length_delimited_encode(raw_data, key) != 0) ||
	    (key_value_type_encode(raw_data, (u8)0, (u8)INT64, (u8)0) != 0) ||
	    (int64_t_varint_encode(raw_data, value) != 0)) {
		pr_err("failed to encode an integer parameter");
		goto put_int_err;
	}

	if (raw_data_append(builder->params.raw_data, raw_data->data, raw_data->len) != 0) {
		pr_err("failed to append a raw data");
		goto put_int_err;
	}

	builder->params.total_cnt++;
	ret = 0;

put_int_err:
	raw_data_destroy(raw_data);
	return ret;
}
EXPORT_SYMBOL_GPL(hisysevent_builder_put_integer);

int hisysevent_builder_put_string(struct hisysevent_builder *builder, const char *key,
				  const char *value)
{
	int ret;
	struct hisysevent_raw_data *raw_data;

	if (!is_valid_string(key, MAX_PARAM_NAME_LENGTH)) {
		pr_err("try to put an invalid key");
		return -EINVAL;
	}
	if (!value || strlen(value) > PARAM_STR_MAX_LEN) {
		pr_err("string length exceeds limit");
		return -EINVAL;
	}
	if (!hisysevent_check_params_validity(builder))
		return -EINVAL;

	raw_data = raw_data_create();
	if (!raw_data) {
		pr_err("failed to create raw data for a new string parameter");
		return -ENOMEM;
	}

	ret = -EINVAL;
	if ((str_length_delimited_encode(raw_data, key) != 0) ||
	    (key_value_type_encode(raw_data, 0, (u8)STRING, 0) != 0) ||
	    (str_length_delimited_encode(raw_data, value) != 0)) {
		pr_err("failed to encode a string parameter");
		goto put_str_err;
	}

	if (raw_data_append(builder->params.raw_data, raw_data->data, raw_data->len) != 0) {
		pr_err("failed to append a raw data");
		goto put_str_err;
	}

	builder->params.total_cnt++;
	ret = 0;

put_str_err:
	raw_data_destroy(raw_data);
	return ret;
}
EXPORT_SYMBOL_GPL(hisysevent_builder_put_string);

int hisysevent_builder_build(struct hisysevent_builder *builder,
			     struct hisysevent_raw_data *raw_data)
{
	s32 blockSize;
	struct hisysevent_raw_data *params_raw_data;

	if (!hisysevent_check_params_validity(builder))
		return -EINVAL;

	blockSize = 0;
	// copy block size at first
	if (raw_data_append(raw_data, (u8 *)(&blockSize), sizeof(s32)) != 0) {
		pr_err("fialed to append block size");
		return -ENOMEM;
	}
	// copy header
	if (raw_data_append(raw_data, (u8 *)(&builder->header),
			    sizeof(struct hisysevent_header)) != 0) {
		pr_err("fialed to append sys event header");
		return -ENOMEM;
	}
	// copy total count of parameter
	if (raw_data_append(raw_data, (u8 *)(&builder->params.total_cnt),
			    sizeof(s32)) != 0) {
		pr_err("fialed to append total count of parameters");
		return -ENOMEM;
	}
	// copy customized parameters
	params_raw_data = builder->params.raw_data;
	if (!params_raw_data) {
		pr_err("this sys event doesn't have any parameter");
		return -EINVAL;
	}
	if (raw_data_append(raw_data, params_raw_data->data, params_raw_data->len) != 0) {
		pr_err("fialed to append encoded raw data of parameters");
		return -ENOMEM;
	}
	// update block size
	blockSize = raw_data->len;
	if (raw_data_update(raw_data, (u8 *)(&blockSize), sizeof(s32), 0) != 0) {
		pr_err("fialed to update block size");
		return -ENOMEM;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(hisysevent_builder_build);

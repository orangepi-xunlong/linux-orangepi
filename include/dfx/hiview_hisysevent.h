/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 Huawei Technologies Co., Ltd. All rights reserved.
 */

#ifndef HIVIEW_HISYSEVENT_H
#define HIVIEW_HISYSEVENT_H

enum hisysevent_type {
	/* fault event */
	FAULT = 1,

	/* statistic event */
	STATISTIC = 2,

	/* security event */
	SECURITY = 3,

	/* behavior event */
	BEHAVIOR = 4
};

struct hiview_hisysevent;

#ifdef CONFIG_HISYSEVENT

struct hiview_hisysevent *
hisysevent_create(const char *domain, const char *name, enum hisysevent_type type);
void hisysevent_destroy(struct hiview_hisysevent **event);
int hisysevent_put_integer(struct hiview_hisysevent *event, const char *key, long long value);
int hisysevent_put_string(struct hiview_hisysevent *event, const char *key, const char *value);
int hisysevent_write(struct hiview_hisysevent *event);

#else

#include <linux/errno.h>
#include <linux/stddef.h>

static inline struct hiview_hisysevent *
hisysevent_create(const char *domain, const char *name, enum hisysevent_type type)
{
	return NULL;
}

static inline void hisysevent_destroy(struct hiview_hisysevent **event)
{}

static inline int
hisysevent_put_integer(struct hiview_hisysevent *event, const char *key, long long value)
{
	return -EOPNOTSUPP;
}

static inline int
hisysevent_put_string(struct hiview_hisysevent *event, const char *key, const char *value)
{
	return -EOPNOTSUPP;
}

static inline int hisysevent_write(struct hiview_hisysevent *event)
{
	return -EOPNOTSUPP;
}

#endif /* CONFIG_HISYSEVENT */

#endif /* HIVIEW_HISYSEVENT_H */

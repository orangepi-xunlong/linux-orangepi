// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Huawei Technologies Co., Ltd. All rights reserved.
 */

#include "hiview_hievent.h"
#include "hievent_driver.h"

#include <linux/slab.h>
#include <linux/string.h>

#define INT_TYPE_MAX_LEN    21

#define MAX_PATH_LEN        256
#define MAX_STR_LEN         (10 * 1024UL)

/* CONFIG_BBOX_BUFFER_SIZE is max length of /dev/bbox */
#define EVENT_INFO_BUF_LEN         ((size_t)CONFIG_BBOX_BUFFER_SIZE)
#define EVENT_INFO_PACK_BUF_LEN    min((size_t)CONFIG_BBOX_BUFFER_SIZE, 2048)

#define BUF_POINTER_FORWARD             \
do {                                    \
	if (tmplen < len) {             \
		tmp += tmplen;          \
		len -= tmplen;          \
	} else {                        \
		tmp += len;             \
		len = 0;                \
	}                               \
} while (0)

struct hievent_payload {
	char *key;
	char *value;
	struct hievent_payload *next;
};

static int hievent_convert_string(struct hiview_hievent *event, char **pbuf);

static struct hievent_payload *hievent_payload_create(void);

static void hievent_payload_destroy(struct hievent_payload *p);

static struct hievent_payload *hievent_get_payload(struct hievent_payload *head,
						   const char *key);

static void hievent_add_payload(struct hiview_hievent *obj,
				struct hievent_payload *payload);

static struct hievent_payload *hievent_payload_create(void)
{
	struct hievent_payload *payload = NULL;

	payload = kmalloc(sizeof(*payload), GFP_KERNEL);
	if (!payload)
		return NULL;

	payload->key = NULL;
	payload->value = NULL;
	payload->next = NULL;

	return payload;
}

static void hievent_payload_destroy(struct hievent_payload *p)
{
	if (!p)
		return;

	kfree(p->value);
	kfree(p->key);
	kfree(p);
}

static struct hievent_payload *hievent_get_payload(struct hievent_payload *head,
						   const char *key)
{
	struct hievent_payload *p = head;

	while (p) {
		if (key && p->key) {
			if (strcmp(p->key, key) == 0)
				return p;
		}
		p = p->next;
	}

	return NULL;
}

static void hievent_add_payload(struct hiview_hievent *obj,
				struct hievent_payload *payload)
{
	if (!obj->head) {
		obj->head = payload;
	} else {
		struct hievent_payload *p = obj->head;

		while (p->next)
			p = p->next;
		p->next = payload;
	}
}

struct hiview_hievent *hievent_create(unsigned int eventid)
{
	struct hiview_hievent *event = NULL;

	/* combined event obj struct */
	event = kmalloc(sizeof(*event), GFP_KERNEL);
	if (!event)
		return NULL;

	memset(event, 0, sizeof(*event));
	event->eventid = eventid;
	pr_debug("%s : %u\n", __func__, eventid);

	return (void *)event;
}

int hievent_put_integer(struct hiview_hievent *event,
			const char *key, long value)
{
	int ret;
	struct hievent_payload *payload = NULL;

	if ((!event) || (!key)) {
		pr_err("Bad input event or key for %s", __func__);
		return -EINVAL;
	}

	payload = hievent_get_payload(event->head, key);
	if (!payload) {
		payload = hievent_payload_create();
		if (!payload)
			return -ENOMEM;
		payload->key = kstrdup(key, GFP_KERNEL);
		hievent_add_payload(event, payload);
	}

	kfree(payload->value);

	payload->value = kmalloc(INT_TYPE_MAX_LEN, GFP_KERNEL);
	if (!payload->value)
		return -ENOMEM;

	(void)memset(payload->value, 0, INT_TYPE_MAX_LEN);
	ret = snprintf(payload->value, INT_TYPE_MAX_LEN, "%d", (int)value);
	if (ret < 0)
		return -ENOMEM;

	return 0;
}

int hievent_put_string(struct hiview_hievent *event,
		       const char *key, const char *value)
{
	struct hievent_payload *payload = NULL;
	size_t len;

	if ((!event) || (!key) || (!value)) {
		pr_err("Bad key for %s", __func__);
		return -EINVAL;
	}

	payload = hievent_get_payload(event->head, key);
	if (!payload) {
		payload = hievent_payload_create();
		if (!payload)
			return -ENOMEM;

		payload->key = kstrdup(key, GFP_KERNEL);
		hievent_add_payload(event, payload);
	}

	kfree(payload->value);

	len = strlen(value);
	/* prevent length larger than MAX_STR_LEN */
	if (len > MAX_STR_LEN)
		len = MAX_STR_LEN;

	payload->value = kmalloc(len + 1, GFP_KERNEL);
	if (!payload->value)
		return -ENOMEM;

	(void)memset(payload->value, 0, len + 1);
	if (strncpy(payload->value, value, len) > 0)
		payload->value[len] = '\0';

	return 0;
}

int hievent_set_time(struct hiview_hievent *event, long long seconds)
{
	if ((!event) || (seconds == 0)) {
		pr_err("Bad input for %s", __func__);
		return -EINVAL;
	}
	event->time = seconds;
	return 0;
}

static int append_array_item(char **pool, int pool_len, const char *path)
{
	int i;

	if ((!path) || (path[0] == 0)) {
		pr_err("Bad path %s", __func__);
		return -EINVAL;
	}

	if (strlen(path) > MAX_PATH_LEN) {
		pr_err("file path over max: %d", MAX_PATH_LEN);
		return -EINVAL;
	}

	for (i = 0; i < pool_len; i++) {
		if (pool[i] != 0)
			continue;

		pool[i] = kstrdup(path, GFP_KERNEL);
		if (!pool[i])
			return -ENOMEM;

		break;
	}

	if (i == MAX_PATH_NUMBER) {
		pr_err("Too many paths");
		return -EINVAL;
	}

	return 0;
}

int hievent_add_filepath(struct hiview_hievent *event, const char *path)
{
	if (!event) {
		pr_err("Bad path %s", __func__);
		return -EINVAL;
	}
	return append_array_item(event->file_path, MAX_PATH_NUMBER, path);
}

/* make string ":" to "::", ";" to ";;", and remove newline character
 * for example: "abc:def;ghi" transfer to "abc::def;;ghi"
 */
static char *hievent_make_regular(char *value)
{
	int count = 0;
	int len = 0;
	char *temp = value;
	char *regular = NULL;
	char *regular_tmp = NULL;
	size_t regular_len;

	while (*temp != '\0') {
		if (*temp == ':' || *temp == ';')
			count++;
		else if ((*temp == '\n') || (*temp == '\r'))
			*temp = ' ';

		temp++;
		len++;
	}

	/* no need to transfer, just return old value */
	if (count == 0)
		return value;

	regular_len = len + count * 2 + 1; // 2 char in a byte
	regular = kmalloc(regular_len, GFP_KERNEL);
	if (!regular)
		return NULL;

	(void)memset(regular, 0, regular_len);
	regular_tmp = regular;
	temp = value;
	while (*temp != 0) {
		if ((*temp == ':') || (*temp == ';'))
			*regular_tmp++ = *temp;

		*regular_tmp++ = *temp;
		temp++;
	}
	*regular_tmp = '\0';

	return regular;
}

int logbuff_to_exception(char category, int level, char log_type,
			 char sn, const char *msg, int msglen)
{
	struct idap_header *hdr = NULL;
	size_t buf_len = sizeof(int) + sizeof(struct idap_header) + msglen;
	int ret;
	int *check_code = NULL;
	char *buffer = kmalloc(buf_len, GFP_KERNEL);

	if (!buffer)
		return -ENOMEM;

	check_code = (int *)buffer;
	*check_code = CHECK_CODE;

	hdr = (struct idap_header *)(buffer + sizeof(int));
	hdr->level = level;
	hdr->category = category;
	hdr->log_type = log_type;
	hdr->sn = sn;

	memcpy(buffer + sizeof(int) + sizeof(struct idap_header), msg, msglen);

	ret = hievent_write_internal(buffer, buf_len);

	kfree(buffer);

	return ret;
}

static int hievent_fill_payload(struct hiview_hievent *event, char **pbuf,
				char *tmp, int length)
{
	struct hievent_payload *p = event->head;
	int len = length;
	int tmplen;
	unsigned int keycount = 0;

	while (p) {
		char *value = NULL;
		char *regular_value = NULL;
		int need_free = 1;

		if (!p->value) {
			p = p->next;
			continue;
		}
		if (keycount == 0) {
			tmplen = snprintf(tmp, len - 1, " --extra ");
			BUF_POINTER_FORWARD;
		}
		keycount++;

		/* fill key */
		if (p->key)
			tmplen = snprintf(tmp, len - 1, "%s:", p->key);

		BUF_POINTER_FORWARD;
		/* fill value */
		tmplen = 0;

		value = p->value;
		regular_value = hievent_make_regular(value);
		if (!regular_value) {
			regular_value = "NULL";
			need_free = 0;
		}
		tmplen = snprintf(tmp, len - 1, "%s;", regular_value);
		if ((value != regular_value) && need_free)
			kfree(regular_value);

		BUF_POINTER_FORWARD;
		p = p->next;
	}
	return len;
}

static int hievent_convert_string(struct hiview_hievent *event, char **pbuf)
{
	int len;
	char *tmp = NULL;
	int tmplen;
	unsigned int i;

	char *buf = kmalloc(EVENT_INFO_BUF_LEN, GFP_KERNEL);

	if (!buf) {
		*pbuf = NULL;
		return 0;
	}

	(void)memset(buf, 0, EVENT_INFO_BUF_LEN);
	len = EVENT_INFO_BUF_LEN;
	tmp = buf;

	/* fill eventid */
	tmplen = snprintf(tmp, len - 1, "eventid %d", event->eventid);
	BUF_POINTER_FORWARD;

	/* fill the path */
	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		if (!event->file_path[i])
			break;

		tmplen = snprintf(tmp, len - 1, " -i %s", event->file_path[i]);
		BUF_POINTER_FORWARD;
	}

	/* fill time */
	if (event->time) {
		tmplen = snprintf(tmp, len - 1, " -t %lld",  event->time);
		BUF_POINTER_FORWARD;
	}

	/* fill the payload info */
	len = hievent_fill_payload(event, pbuf, tmp, len);
	*pbuf = buf;
	return (int)(EVENT_INFO_BUF_LEN - len);
}

#define IDAP_LOGTYPE_CMD 1
static int hievent_write_logexception(char *str, const int strlen)
{
	char tempchr;
	char *strptr = str;
	int left_buf_len = strlen + 1;
	int sent_cnt = 0;

	while (left_buf_len > 0) {
		if (left_buf_len > EVENT_INFO_PACK_BUF_LEN) {
			tempchr = strptr[EVENT_INFO_PACK_BUF_LEN - 1];
			strptr[EVENT_INFO_PACK_BUF_LEN - 1] = '\0';
			logbuff_to_exception(0, 0, IDAP_LOGTYPE_CMD, 1, strptr,
					     EVENT_INFO_PACK_BUF_LEN);
			left_buf_len -= (EVENT_INFO_PACK_BUF_LEN - 1);
			strptr += (EVENT_INFO_PACK_BUF_LEN - 1);
			strptr[0] = tempchr;
			sent_cnt++;
		} else {
			logbuff_to_exception(0, 0, IDAP_LOGTYPE_CMD, 0, strptr,
					     left_buf_len);
			sent_cnt++;
			break;
		}
	}

	return sent_cnt;
}

int hievent_report(struct hiview_hievent *obj)
{
	char *str = NULL;
	int buf_len;
	int sent_packet;

	if (!obj) {
		pr_err("Bad event %s", __func__);
		return -EINVAL;
	}

	buf_len = hievent_convert_string(obj, &str);
	if (!str)
		return -EINVAL;

	sent_packet = hievent_write_logexception(str, buf_len);
	pr_err("report: %s", str);
	kfree(str);

	return sent_packet;
}

void hievent_destroy(struct hiview_hievent *event)
{
	int i;
	struct hievent_payload *p = NULL;

	if (!event)
		return;

	p = event->head;
	while (p) {
		struct hievent_payload *del = p;

		p = p->next;
		hievent_payload_destroy(del);
	}

	event->head = NULL;
	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		kfree(event->file_path[i]);
		event->file_path[i] = NULL;
	}

	kfree(event);
}

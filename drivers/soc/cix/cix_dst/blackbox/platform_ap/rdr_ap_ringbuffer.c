// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_hisi_ap_ringbuffer.c
 *
 * record the data to rdr. (RDR: kernel run data recorder.) This file wraps the ring buffer.
 *
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd.
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/cacheflush.h>
#include <linux/slab.h>
#include <linux/soc/cix/rdr_platform_ap_ringbuffer.h>
#include "../rdr_print.h"

int rdr_rbuf_init(struct rdr_ringbuffer *q, u32 bytes, u32 fieldcnt,
		  const char *keys)
{
	if (!q) {
		BB_ERR("buffer head is null!\n");
		return -EINVAL;
	}

	if (bytes < (sizeof(*q) + sizeof(u8) * fieldcnt)) {
		BB_ERR("ringbuffer size [0x%x] is too short!\n", bytes);
		return -EINVAL;
	}

	/* max_num: records count */
	q->max_num = (bytes - sizeof(*q)) / (sizeof(u8) * fieldcnt);
	atomic_set((atomic_t *)&(q->rear),
		   0); /* point to the last NULL record. UNIT is record */
	q->r_idx = 0; /* point to the last read record */
	q->count = 0;
	q->is_full = 0;
	q->field_count = fieldcnt; /* How many u8 in ONE record */

	memset(q->keys, 0, RDR_RB_KEYS_MAX + 1);

	if (keys)
		strncpy(q->keys, keys, RDR_RB_KEYS_MAX);
	dcache_clean_poc((u64)q, ((u64)q) + sizeof(*q));
	return 0;
}

void rdr_rbuf_write(struct rdr_ringbuffer *q, u8 *element)
{
	if (IS_ERR_OR_NULL(q) || IS_ERR_OR_NULL(element)) {
		BB_ERR("parameter q or element is null\n");
		return;
	}

	atomic_add(1, (atomic_t *)&(q->rear));
	if (q->rear >= q->max_num) {
		q->rear = 1;
		q->is_full = 1;
	}

	if (sizeof(*element) > (q->max_num - q->rear))
		BB_ERR("memcpy_s fail!\n");

	memcpy((void *)&q->data[(q->rear - 1) * q->field_count],
	       (void *)element, q->field_count * sizeof(*element));

	q->count++;
	if (q->count >= q->max_num)
		q->count = q->max_num;
}

/* Return:  success: = 0 ;  fail: other */
int rdr_rbuf_read(struct rdr_ringbuffer *q, u8 *element, u32 len)
{
	u32 ridx;

	if (IS_ERR_OR_NULL(q)) {
		BB_ERR("parameter q ringbuffer is null!\n");
		return -1;
	}

	if (IS_ERR_OR_NULL(element)) {
		BB_ERR("parameter element is null!\n");
		return -1;
	}

	if (q->count == 0)
		return -1;
	q->count--;

	if (q->count >= q->max_num)
		q->r_idx = q->rear;

	if (q->r_idx >= q->max_num)
		q->r_idx = 0;

	ridx = q->r_idx++;

	if (len > q->field_count * sizeof(*element))
		BB_ERR("memcpy fail!\n");

	memcpy((void *)element, (void *)&q->data[(long)ridx * q->field_count],
	       q->field_count * sizeof(*element));

	return 0;
}

/*
 * Description: Determine if the ringbuffer is full
 * Input:       buffer_addr: buffer head address
 * Return:      0: buffer is not full; 1: buffer is full; -1: the query is invalid
 */
int rdr_rbuf_is_full(const void *buffer_addr)
{
	if (!buffer_addr)
		return -1;

	return (int)(((struct rdr_ringbuffer *)buffer_addr)->is_full);
}

void rdr_rbuf_get_start_end(struct rdr_ringbuffer *q, u32 *start, u32 *end)
{
	if (IS_ERR_OR_NULL(q) || IS_ERR_OR_NULL(start) || IS_ERR_OR_NULL(end)) {
		BB_ERR("parameter q 0x%pK start 0x%pK end 0x%pK is null!\n", q,
		       start, end);
		return;
	}

	if (q->is_full) {
		if ((q->rear >= q->max_num) || (q->rear <= 0)) {
			*start = 0;
			*end = q->max_num - 1;
		} else if (q->rear) {
			*start = q->rear;
			*end = q->rear - 1 + q->max_num;
		}
	} else {
		*start = 0;
		*end = q->rear - 1;
	}
}

void *rdr_rbuf_get_data(struct rdr_ringbuffer *q, u32 index)
{
	return &q->data[(size_t)(index % q->max_num) * q->field_count];
}

bool rdr_rbuf_is_empty(struct rdr_ringbuffer *q)
{
	if (IS_ERR_OR_NULL(q)) {
		BB_ERR("parameter q ringbuffer is null!\n");
		return true;
	}

	if ((q->is_full == 0) && (q->rear == 0))
		return true;

	return false;
}

bool rdr_rbuf_is_invalid(u32 field_count, u32 len, struct rdr_ringbuffer *q)
{
	if (IS_ERR_OR_NULL(q)) {
		BB_ERR("parameter q ringbuffer is null!\n");
		return true;
	}

	if (unlikely(q->field_count != field_count)) {
		BB_ERR("fail:rdr_ringbuffer field_count %u != %u\n",
		       q->field_count, field_count);
		return true;
	}

	if (unlikely(q->rear > q->max_num)) {
		BB_ERR("fail:q->rear %u > q->max_num %u\n", q->rear,
		       q->max_num);
		return true;
	}

	if (unlikely((q->max_num <= 0) || (field_count <= 0) ||
		     (len <= sizeof(*q) ||
		      (q->max_num > ((len - sizeof(*q)) / field_count))))) {
		BB_ERR("fail:rdr_ringbuffer max_num %u field_count %u len %u\n",
		       q->max_num, field_count, len);
		return true;
	}

	return false;
}

u32 rdr_rbuf_get_maxnum(struct rdr_ringbuffer *q)
{
	return q->max_num;
}

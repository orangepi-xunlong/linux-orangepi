/*
 * rdr_hisi_ap_ringbuffer.h
 *
 * This file wraps the ring buffer.
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#ifndef __RDR_HISI_AP_RINGBUFFER_H__
#define __RDR_HISI_AP_RINGBUFFER_H__


#include <linux/kernel.h>
#include <mntn_public_interface.h>

#ifdef CONFIG_PLAT_BBOX
int hisiap_ringbuffer_init(struct hisiap_ringbuffer_s *q, u32 bytes,
				u32 fieldcnt, const char *keys);
void hisiap_ringbuffer_write(struct hisiap_ringbuffer_s *q, u8 *element);
int  hisiap_ringbuffer_read(struct hisiap_ringbuffer_s *q, u8 *element, u32 len);
int  hisiap_is_ringbuffer_full(const void *buffer_addr);
void get_ringbuffer_start_end(struct hisiap_ringbuffer_s *q, u32 *start, u32 *end);
bool is_ringbuffer_empty(struct hisiap_ringbuffer_s *q);
bool is_ringbuffer_invalid(u32 field_count, u32 len, struct hisiap_ringbuffer_s *q);
#else
static inline int hisiap_ringbuffer_init(struct hisiap_ringbuffer_s *q, u32 bytes,
				u32 fieldcnt, const char *keys) {return 0; }
static inline void hisiap_ringbuffer_write(struct hisiap_ringbuffer_s *q, u8 *element) { return; }
static inline int  hisiap_ringbuffer_read(struct hisiap_ringbuffer_s *q, u8 *element, u32 len) {return 0; }
static inline int  hisiap_is_ringbuffer_full(const void *buffer_addr) {return 0; }
static inline void get_ringbuffer_start_end(struct hisiap_ringbuffer_s *q, u32 *start, u32 *end) {return; }
static inline bool is_ringbuffer_empty(struct hisiap_ringbuffer_s *q) {return true; }
static inline bool is_ringbuffer_invalid(u32 field_count, u32 len, struct hisiap_ringbuffer_s *q) {return true; }
#endif
#endif /* __RDR_HISI_AP_RINGBUFFER_H__ */

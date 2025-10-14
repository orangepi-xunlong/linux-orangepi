/*
 * fdleak.h
 *
 * This file is the header file for fdleak
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
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

#ifndef __FDLEAK_H_
#define __FDLEAK_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FDLEAK_PID_NUM 5
#define MAX_PROBE_COUNT 3
#define MAX_STACK_TRACE_COUNT 20
#define FDLEAK_MAX_STACK_TRACE_DEPTH 64
#define MAX_FDLEAK_WP_NAME_LEN 256

enum fdleak_wp_id {
	FDLEAK_WP_MIN = 0,
	FDLEAK_WP_EVENTFD = 0,
	FDLEAK_WP_EVENTPOLL = 1,
	FDLEAK_WP_DMABUF = 2,
	FDLEAK_WP_SYNCFENCE = 3,
	FDLEAK_WP_SOCKET = 4,
	FDLEAK_WP_PIPE = 5,
	FDLEAK_WP_ASHMEM = 6,
	FDLEAK_WP_NUM_MAX,
	FDLEAK_WP_UNKNOW
};

#define MAX_EVENTPOLL_PROBE_CNT 2
#define MAX_EVENTFD_PROBE_CNT 2
#define MAX_DMABUF_PROBE_CNT 3
#define MAX_SYNCFENCE_PROBE_CNT 2
#define MAX_SOCKET_PROBE_CNT 2
#define MAX_PIPE_PROBE_CNT 2
#define MAX_ASHMEM_PROBE_CNT 2

#define MASK_FDLEAK_WP_EVENTFD (1 << FDLEAK_WP_EVENTFD)
#define MASK_FDLEAK_WP_EVENTPOLL (1 << FDLEAK_WP_EVENTPOLL)
#define MASK_FDLEAK_WP_DMABUF (1 << FDLEAK_WP_DMABUF)
#define MASK_FDLEAK_WP_SYNCFENCE (1 << FDLEAK_WP_SYNCFENCE)
#define MASK_FDLEAK_WP_SOCKET (1 << FDLEAK_WP_SOCKET)
#define MASK_FDLEAK_WP_PIPE (1 << FDLEAK_WP_PIPE)
#define MASK_FDLEAK_WP_ASHMEM (1 << FDLEAK_WP_ASHMEM)

struct fdleak_op {
	int magic;
	int pid;
	int wp_mask;
};

struct stack_trace {
	unsigned int nr_entries, max_entries;
	unsigned long *entries;
	unsigned int skip;	/* input argument: How many entries to skip */
};

struct stack_item {
	unsigned long long stack[FDLEAK_MAX_STACK_TRACE_DEPTH];
};

struct fdleak_stackinfo {
	int magic;
	int pid;
	enum fdleak_wp_id wpid;
	int is_32bit;
	int probe_cnt;
	char wp_name[MAX_FDLEAK_WP_NAME_LEN];
	int hit_cnt[MAX_PROBE_COUNT][MAX_STACK_TRACE_COUNT];
	int diff_cnt[MAX_PROBE_COUNT];
	struct stack_item list[MAX_PROBE_COUNT][MAX_STACK_TRACE_COUNT];
};

/* the following are used for IOCTL */
#define FDLEAK_MAGIC 0x5366FEFA
#define FDLEAK_CMD_INVALID 0xFF

#define __FDLEAKIO  0xAC
#define FDLEAK_ENABLE_WATCH     _IO(__FDLEAKIO, 1)
#define FDLEAK_DISABLE_WATCH    _IO(__FDLEAKIO, 2)
#define FDLEAK_GET_STACK        _IO(__FDLEAKIO, 3)
#define FDLEAK_CMD_MAX          _IO(__FDLEAKIO, 4)

#ifdef CONFIG_PLAT_LOGGER
struct file;
long fdleak_ioctl(struct file *file, unsigned int cmd, uintptr_t arg);
int fdleak_report(enum fdleak_wp_id index, int probe_id);
#endif

#ifdef __cplusplus
}
#endif
#endif	/* __FDLEAK_H_ */

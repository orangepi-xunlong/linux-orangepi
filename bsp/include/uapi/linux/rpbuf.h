/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * (C) Copyright 2020-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Junyan Lin <junyanlin@allwinnertech.com>
 *
 * RPBuf user interface header.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __UAPI_RPBUF_H__
#define __UAPI_RPBUF_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#define RPBUF_NAME_SIZE	32	/* include trailing '\0' */

struct rpbuf_buffer_info {
	char name[RPBUF_NAME_SIZE];
	__u32 len;
};

struct rpbuf_buffer_xfer {
	__u32 offset;
	__u32 data_len;
	__s32 timeout_ms;
};

#define RPBUF_CTRL_DEV_IOCTL_MAGIC	0xb8
#define RPBUF_CTRL_DEV_IOCTL_CREATE_BUF \
	_IOW(RPBUF_CTRL_DEV_IOCTL_MAGIC, 0x1, struct rpbuf_buffer_info)
#define RPBUF_CTRL_DEV_IOCTL_DESTROY_BUF \
	_IOW(RPBUF_CTRL_DEV_IOCTL_MAGIC, 0x2, struct rpbuf_buffer_info)

#define RPBUF_BUF_DEV_IOCTL_MAGIC	0xb9
#define RPBUF_BUF_DEV_IOCTL_CHECK_AVAIL \
	_IOR(RPBUF_BUF_DEV_IOCTL_MAGIC, 0x1, __u8)
#define RPBUF_BUF_DEV_IOCTL_TRANSMIT_BUF \
	_IOW(RPBUF_BUF_DEV_IOCTL_MAGIC, 0x2, struct rpbuf_buffer_xfer)
#define RPBUF_BUF_DEV_IOCTL_RECEIVE_BUF \
	_IOWR(RPBUF_BUF_DEV_IOCTL_MAGIC, 0x3, struct rpbuf_buffer_xfer)
#define RPBUF_BUF_DEV_IOCTL_SET_SYNC_BUF \
	_IOW(RPBUF_BUF_DEV_IOCTL_MAGIC, 0x4, struct rpbuf_buffer_xfer)

#endif

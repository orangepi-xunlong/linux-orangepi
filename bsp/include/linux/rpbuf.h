/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * (C) Copyright 2020-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Junyan Lin <junyanlin@allwinnertech.com>
 *
 * RPBuf public header.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __RPBUF_H__
#define __RPBUF_H__

#include <linux/device.h>

#ifndef RPBUF_NAME_SIZE
#define RPBUF_NAME_SIZE	32	/* include trailing '\0' */
#endif

struct rpbuf_controller;
struct rpbuf_buffer;

/*
 * The callback function type when buffer is available.
 * @buffer: rpbuf buffer
 * @priv: private data for user
 */
typedef void (*rpbuf_available_cb_t)(struct rpbuf_buffer *buffer, void *priv);

/*
 * The callback function type for receiving rpbuf buffer.
 * @buffer: rpbuf buffer
 * @data: the virtual address of buffer payload data
 * @data_len: the valid length of data (should be <= total length of buffer)
 * @priv: private data for user
 */
typedef int (*rpbuf_rx_cb_t)(struct rpbuf_buffer *buffer, void *data, int data_len, void *priv);

/*
 * The callback function type when remote destroys buffer.
 * @buffer: rpbuf buffer
 * @priv: private data for user
 */
typedef int (*rpbuf_destroyed_cb_t)(struct rpbuf_buffer *buffer, void *priv);

struct rpbuf_buffer_cbs {
	rpbuf_available_cb_t available_cb;
	rpbuf_rx_cb_t rx_cb;
	rpbuf_destroyed_cb_t destroyed_cb;
};

/**
 * struct rpbuf_buffer_ops - user-specific rpbuf buffer operations
 * @alloc_payload_memory: allocate memory for payload
 * @free_payload_memory: free memory for payload
 * @addr_remap: remap pa or da to va
 * @buf_dev_mmap: mmap file operation for buf_dev
 */
struct rpbuf_buffer_ops {
	int (*alloc_payload_memory)(struct rpbuf_buffer *buffer, void *priv);
	void (*free_payload_memory)(struct rpbuf_buffer *buffer, void *priv);
	void *(*addr_remap)(struct rpbuf_buffer *buffer,
			    phys_addr_t pa, u64 da, int len, void *priv);
	int (*buf_dev_mmap)(struct rpbuf_buffer *buffer, void *priv, struct vm_area_struct *vma);
};

/**
 * rpbuf_get_controller_by_of_node - get rpbuf controller by device of_node
 * @np: of_node of the device
 * @index: index to select which of_node to use
 *
 * Return pointer to the rpbuf_controller on success, or NULL on failure.
 */
struct rpbuf_controller *rpbuf_get_controller_by_of_node(const struct device_node *np, int index);

/**
 * rpbuf_wait_controller_ready - wait rpbuf controller ready
 * @controller: rpbuf controller
 * @timeout: wait time (ms)
 *
 * Return 0 if success, otherwise negative number.
 */
int rpbuf_wait_controller_ready(struct rpbuf_controller *controller, int timeout);

/**
 * rpbuf_alloc_buffer - allocate a rpbuf buffer
 * @controller: rpbuf controller
 * @name: buffer name
 * @len: buffer length
 * @ops: user-specific rpbuf buffer operations (set NULL if not used)
 * @cbs: rpbuf buffer callbacks (set NULL if not used)
 * @priv: private data for user (set NULL if not used)
 *
 * Return pointer to rpbuf buffer on success, or NULL on failure.
 */
struct rpbuf_buffer *rpbuf_alloc_buffer(struct rpbuf_controller *controller,
					const char *name, int len,
					const struct rpbuf_buffer_ops *ops,
					const struct rpbuf_buffer_cbs *cbs,
					void *priv);

/**
 * rpbuf_free_buffer - free a rpbuf buffer
 * @buffer: rpbuf buffer
 *
 * This function will trigger the buffer destroyed_cb on remote.
 *
 * Return 0 on success, or a negative number on failure.
 */
int rpbuf_free_buffer(struct rpbuf_buffer *buffer);

/**
 * rpbuf_buffer_is_available - check whether a rpbuf buffer is available for local
 * @buffer: rpbuf buffer
 *
 * Return 1 when available, 0 when not available.
 */
int rpbuf_buffer_is_available(struct rpbuf_buffer *buffer);

/**
 * rpbuf_transmit_buffer - transmit a rpbuf buffer to remote
 * @buffer: rpbuf buffer
 * @offset: the address offset to store data
 * @data_len: the valid length of data
 *
 * This function will trigger the buffer rx_cb on remote.
 *
 * Return 0 on success, or a negative number on failure.
 */
int rpbuf_transmit_buffer(struct rpbuf_buffer *buffer,
			  unsigned int offset, unsigned int data_len);

/*
 * Get the name of rpbuf buffer.
 */
const char *rpbuf_buffer_name(struct rpbuf_buffer *buffer);
/*
 * Get the id of rpbuf buffer.
 */
int rpbuf_buffer_id(struct rpbuf_buffer *buffer);
/*
 * Get the virtual address of rpbuf buffer.
 */
void *rpbuf_buffer_va(struct rpbuf_buffer *buffer);
/*
 * Get the physical address of rpbuf buffer.
 */
phys_addr_t rpbuf_buffer_pa(struct rpbuf_buffer *buffer);
/*
 * Get the device address of rpbuf buffer.
 */
u64 rpbuf_buffer_da(struct rpbuf_buffer *buffer);
/*
 * Get the length of rpbuf buffer.
 */
int rpbuf_buffer_len(struct rpbuf_buffer *buffer);
/*
 * Get the user private data of rpbuf buffer.
 */
void *rpbuf_buffer_priv(struct rpbuf_buffer *buffer);

/*
 * Set the virtual address of rpbuf buffer.
 */
void rpbuf_buffer_set_va(struct rpbuf_buffer *buffer, void *va);
/*
 * Set the physical address of rpbuf buffer.
 */
void rpbuf_buffer_set_pa(struct rpbuf_buffer *buffer, phys_addr_t pa);
/*
 * Set the device address of rpbuf buffer.
 */
void rpbuf_buffer_set_da(struct rpbuf_buffer *buffer, u64 da);
/*
 * Set whether the buffer is sent synchronously.
 * if sent sync, it will block until the remoteproc complete 'rx_cb'
 * if send async, this function doesn't care about remoteproc
 * default is async transmit.
 */
int rpbuf_buffer_set_sync(struct rpbuf_buffer *buffer, bool sync);

#endif

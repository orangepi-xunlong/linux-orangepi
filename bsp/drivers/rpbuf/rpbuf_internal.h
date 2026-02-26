/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * (C) Copyright 2020-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Junyan Lin <junyanlin@allwinnertech.com>
 *
 * RPBuf internal header.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __RPBUF_INTERNAL_H__
#define __RPBUF_INTERNAL_H__

#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/idr.h>
#include <linux/rpbuf.h>

#define RPBUF_SERVICE_MESSAGE_LENGTH_MAX 128

#define RPBUF_SERVICE_MESSAGE_PREAMBLE	0xA
#define RPBUF_SERVICE_MESSAGE_POSTAMBLE	0x5

struct rpbuf_service;
struct rpbuf_service_ops;
struct rpbuf_controller_ops;

enum rpbuf_role {
	RPBUF_ROLE_UNKNOWN = 0,
	RPBUF_ROLE_MASTER,
	RPBUF_ROLE_SLAVE,
};

#define RPBUF_FLAGS_GETNOTIFY			(1 << 0)	/* buffer get notify */
#define RPBUF_FLAGS_DESTROYED			(1 << 1)	/* buffer destroyed */
#define RPBUF_FLAGS_WORKING				(1 << 2)	/* buffer working */

struct rpbuf_link {
	/*
	 * 'token' is a symbol to link service and controller. Only the service
	 * and controller with the same 'token' could be linked together.
	 */
	void *token;

	struct rpbuf_service *service;
	struct rpbuf_controller *controller;

	/*
	 * service_lock/controller_lock is used to avoid concurrent access to
	 * service/controller, including their creation and destruction.
	 * So these locks would keep alive in the whole live cycle of link.
	 *
	 * To avoid deadlock, these two locks cannot be used within each other.
	 * For example, we should NOT do:
	 *	{
	 *		lock(service_lock);
	 *		lock(controller_lock);
	 *		unlock(controller_lock);
	 *		unlock(service_lock);
	 *	}
	 * Instead, we should do:
	 *	{
	 *		lock(service_lock);
	 *		unlock(service_lock);
	 *		lock(controller_lock);
	 *		unlock(controller_lock);
	 *	}
	 */
	struct mutex service_lock;	/* protect service */
	struct mutex controller_lock;	/* protect controller */

	struct list_head list;
	spinlock_t lock;	/* protect link itself */
	int ref_cnt;
};

struct rpbuf_service {
	struct device *dev;

	struct rpbuf_link *link;

	const struct rpbuf_service_ops *ops;
	void *priv;
};

struct rpbuf_controller {
	struct device *dev;
	enum rpbuf_role role;

	struct rpbuf_link *link;

	/*
	 * A rpbuf_buffer cannot be completely used until both remote and local
	 * allocate it manually. The 'xxx_dummy_buffers' are used to store the
	 * incomplete buffer information. Once the information is complete, the
	 * buffer will be moved from 'xxx_dummy_buffers' to 'buffers'.
	 */
	/* To store buffer information received from remote */
	struct list_head remote_dummy_buffers;
	/* To store buffer information created by loacl */
	struct list_head local_dummy_buffers;
	/* To store buffers whose information is complete */
	struct idr buffers;
	/* To store buffers whose information is alloc in local */
	struct idr local_buffers;

	wait_queue_head_t wq;
	const struct rpbuf_controller_ops *ops;
	void *priv;
};

struct rpbuf_buffer {
	char name[RPBUF_NAME_SIZE];
	int id;

	/*
	 * va: virtual address that program can directly use.
	 * pa: physical address.
	 * da: device address. This address is set by MASTER, and transfer to
	 *     SLAVE to use. What it means is defined by the SLAVE
	 *     translate_to_va() implementation.
	 */
	void *va;
	phys_addr_t pa;
	u64 da;

	int len;

	const struct rpbuf_buffer_ops *ops;
	const struct rpbuf_buffer_cbs *cbs;
	void *priv;

	struct rpbuf_controller *controller;

	struct list_head dummy_list;
	/*
	 * used to sync transmit
	 */
	uint32_t state;
	wait_queue_head_t wait;

#define BUFFER_SYNC_TRANSMIT			0x01
	u32 flags;
	bool allocated;
	bool need_ack;
	/* In order to distinguish whether user space use this buffer by rpbuf buf dev */
	bool is_used_by_buf_dev;
};

/*
 * The layout of a rpbuf service message is like this:
 *
 * struct rpbuf_service_message
 * {
 *	// Header
 *	u8 preamble;	// is always 0xA.
 *	u8 command;
 *	u16 content_len;
 *
 *	// The actual content of the rpbuf service message.
 *	// Its size is 'content_len', which will change with the 'command'.
 *	u8 content[];
 *
 *	// Trailer
 *	u8 postamble;	// is always 0x5.
 * };
 */

struct rpbuf_service_message_header {
	u8 preamble;
	u8 command;
	u16 content_len;
} __attribute__((packed));

struct rpbuf_service_message_trailer {
	u8 postamble;
} __attribute__((packed));

/*
 * NOTE:
 *   This enum is used as index in some arrays, such as:
 *	rpbuf_service_message_content_len[]
 *	rpbuf_service_command_handlers[]
 *   After modifying this enum, remember to update these arrays.
 */
enum rpbuf_service_command {
	RPBUF_SERVICE_CMD_UNKNOWN = 0,
	RPBUF_SERVICE_CMD_BUFFER_CREATED,
	RPBUF_SERVICE_CMD_BUFFER_DESTROYED,
	RPBUF_SERVICE_CMD_BUFFER_TRANSMITTED,
	RPBUF_SERVICE_CMD_BUFFER_ACK,
	RPBUF_SERVICE_CMD_MAX,
};

struct rpbuf_service_content_buffer_created {
	u8 name[RPBUF_NAME_SIZE];
	u32 id;
	u32 len;
	u64 pa;
	u64 da;
	/*
	 * Indicates that whether the local buffer is available before sending
	 * BUFFER_CREATED message to remote.
	 */
	u8 is_available;
} __attribute__((packed));

struct rpbuf_service_content_buffer_destroyed {
	u32 id;
} __attribute__((packed));

struct rpbuf_service_content_buffer_transmitted {
	u32 id;
	u32 offset;
	u32 data_len;
	u32 flags;
} __attribute__((packed));

struct rpbuf_service_content_buffer_ack {
	u32 id;
	u32 timediff;
} __attribute__((packed));

struct rpbuf_service_ops {
	int (*notify)(void *msg, int msg_len, void *priv);
};

struct rpbuf_controller_ops {
	int (*alloc_payload_memory)(struct rpbuf_controller *controller,
				    struct rpbuf_buffer *buffer, void *priv);
	void (*free_payload_memory)(struct rpbuf_controller *controller,
				    struct rpbuf_buffer *buffer, void *priv);
	void *(*addr_remap)(struct rpbuf_controller *controller,
			    phys_addr_t pa, u64 da, int len, void *priv);
	int (*buf_dev_mmap)(struct rpbuf_controller *controller,
			    struct rpbuf_buffer *buffer, void *priv,
			    struct vm_area_struct *vma);
};

struct rpbuf_service *rpbuf_create_service(struct device *dev,
					   const struct rpbuf_service_ops *ops,
					   void *priv);
void rpbuf_destroy_service(struct rpbuf_service *service);

int rpbuf_register_service(struct rpbuf_service *service, void *token);
void rpbuf_unregister_service(struct rpbuf_service *service);

struct rpbuf_controller *rpbuf_create_controller(struct device *dev,
						 const struct rpbuf_controller_ops *ops,
						 void *priv);
void rpbuf_destroy_controller(struct rpbuf_controller *controller);

int rpbuf_register_controller(struct rpbuf_controller *controller,
			      void *token, enum rpbuf_role role);
void rpbuf_unregister_controller(struct rpbuf_controller *controller);

int rpbuf_unregister_ctrl_dev(struct device *dev, int id);
int rpbuf_register_ctrl_dev(struct device *dev, int id, struct rpbuf_controller *controller);

struct device *
rpbuf_bus_find_device_by_of_node(struct bus_type *bus, struct device_node *np);

/**
 * rpbuf_service_get_notification - notice local that remote has notified us
 * @service: rpbuf service
 * @msg: message sent from remote
 * @msg_len: message length
 *
 * Return 0 on success, or a negative number or failure.
 */
int rpbuf_service_get_notification(struct rpbuf_service *service, void *msg, int msg_len);

/*
 * rpbuf_notify_remotebuffer_complete- send ack to remote that we heave read the data
 * @controller: rpbuf controller
 * @buffer: buffer entry
 *
 * Return 0 on success, or a negative number or failure.
 */
int rpbuf_notify_remotebuffer_complete(struct rpbuf_controller *controller,
				struct rpbuf_buffer *buffer);

static inline enum rpbuf_role rpbuf_role_toggle(enum rpbuf_role role)
{
	switch (role) {
	case RPBUF_ROLE_MASTER:
		return RPBUF_ROLE_SLAVE;
	case RPBUF_ROLE_SLAVE:
		return RPBUF_ROLE_MASTER;
	default:
		pr_err("(%s:%d) invalid role\n", __func__, __LINE__);
		return RPBUF_ROLE_UNKNOWN;
	}
}

#endif

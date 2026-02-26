// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * (C) Copyright 2020-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Junyan Lin <junyanlin@allwinnertech.com>
 *
 * RPBuf core.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#define pr_fmt(fmt)	"[RPBUF CORE](%s:%d) " fmt, __func__, __LINE__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include "rpbuf_internal.h"

#define SUNXI_RPBUF_CORE_VERSION "1.2.4"

typedef int (*rpbuf_service_command_handler_t)(struct rpbuf_service *service,
					       enum rpbuf_service_command cmd,
					       void *content);

static LIST_HEAD(__rpbuf_links);
static DEFINE_MUTEX(__rpbuf_links_lock);

static void rpbuf_reset_controller(struct rpbuf_controller *controller);

/* Remember to use __rpbuf_links_lock outside to protect this function */
static inline struct rpbuf_link *rpbuf_find_link(void *token)
{
	struct rpbuf_link *link;

	list_for_each_entry(link, &__rpbuf_links, list) {
		if (link->token == token)
			return link;
	}
	return NULL;
}

static inline struct rpbuf_buffer *rpbuf_find_dummy_buffer_by_name(
		struct list_head *dummy_buffers, const char *name)
{
	struct rpbuf_buffer *buffer;

	list_for_each_entry(buffer, dummy_buffers, dummy_list) {
		if (0 == strncmp(buffer->name, name, RPBUF_NAME_SIZE))
			return buffer;
	}
	return NULL;
}

static inline struct rpbuf_buffer *rpbuf_find_dummy_buffer_by_id(
		struct list_head *dummy_buffers, int id)
{
	struct rpbuf_buffer *buffer;
	list_for_each_entry(buffer, dummy_buffers, dummy_list) {
		if (buffer->id == id)
			return buffer;
	}
	return NULL;
}

static struct rpbuf_buffer *rpbuf_alloc_buffer_instance(struct device *dev,
							const char *name, int len)
{
	struct rpbuf_buffer *buffer;
	size_t name_len = strlen(name);

	buffer = devm_kzalloc(dev, sizeof(struct rpbuf_buffer), GFP_KERNEL);
	if (IS_ERR_OR_NULL(buffer)) {
		dev_err(dev, "kzalloc for rpbuf_buffer failed\n");
		goto err_out;
	}

	if (name_len == 0 || name_len >= RPBUF_NAME_SIZE) {
		dev_err(dev, "invalid buffer name \"%s\", its length should be "
			"in range [1, %d)\n", name, RPBUF_NAME_SIZE);
		goto err_free_rpbuf_buffer;
	}
	strncpy(buffer->name, name, RPBUF_NAME_SIZE);

	buffer->len = len;
	init_waitqueue_head(&buffer->wait);
	buffer->state = 0;
	buffer->flags = 0;
	buffer->need_ack = false;
	buffer->is_used_by_buf_dev = false;

	return buffer;

err_free_rpbuf_buffer:
	devm_kfree(dev, buffer);
err_out:
	return NULL;
}

static void rpbuf_free_buffer_instance(struct device *dev,
				       struct rpbuf_buffer *buffer)
{
	devm_kfree(dev, buffer);
}

static int rpbuf_alloc_buffer_payload_memory_default(struct rpbuf_controller *controller,
						     struct rpbuf_buffer *buffer)
{
	struct device *dev = controller->dev;
	void *va;
	dma_addr_t pa;
	int ret;

	va = dma_alloc_coherent(dev, buffer->len, &pa, GFP_KERNEL);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "dma_alloc_coherent for len %d failed\n", buffer->len);
		ret = -ENOMEM;
		goto err_out;
	}
	buffer->va = va;
	buffer->pa = (phys_addr_t)pa;
	buffer->da = (u64)pa;

	return 0;

err_out:
	return ret;
}

static int rpbuf_alloc_buffer_payload_memory(struct rpbuf_controller *controller,
					     struct rpbuf_buffer *buffer)
{
	int ret;

	if (buffer->ops && buffer->ops->alloc_payload_memory)
		ret = buffer->ops->alloc_payload_memory(buffer, buffer->priv);
	else if (controller->ops && controller->ops->alloc_payload_memory)
		ret = controller->ops->alloc_payload_memory(controller, buffer,
							     controller->priv);
	else
		ret = rpbuf_alloc_buffer_payload_memory_default(controller, buffer);

	dev_info(controller->dev, "\"%s\" allocate payload memory: " \
					"va 0x%pK, pa %pad, len %d\n", buffer->name,
					buffer->va, &buffer->pa, buffer->len);

	return ret;
}

static void rpbuf_free_buffer_payload_memory_default(struct rpbuf_controller *controller,
						     struct rpbuf_buffer *buffer)
{
	struct device *dev = controller->dev;
	void *va;
	dma_addr_t pa;

	va = (void *)buffer->va;
	pa = (dma_addr_t)buffer->pa;

	dma_free_coherent(dev, buffer->len, va, pa);
}

static void rpbuf_free_buffer_payload_memory(struct rpbuf_controller *controller,
					     struct rpbuf_buffer *buffer)
{
	if (buffer->ops && buffer->ops->free_payload_memory)
		buffer->ops->free_payload_memory(buffer, buffer->priv);
	else if (controller->ops && controller->ops->free_payload_memory)
		controller->ops->free_payload_memory(controller, buffer,
							    controller->priv);
	else
		rpbuf_free_buffer_payload_memory_default(controller, buffer);

	dev_info(controller->dev, "\"%s\" free payload memory: " \
					"va %pK, pa %pad, len %d\n", buffer->name,
					buffer->va, &buffer->pa, buffer->len);
}

static void *rpbuf_addr_remap_default(struct rpbuf_controller *controller,
				      struct rpbuf_buffer *buffer,
				      phys_addr_t pa, u64 da, int len)
{
	/*
	 * TODO:
	 * How to translate pa to va? ioremap doesn't guarantee that the
	 * returned virtual address can be directly used?
	 */
	return ioremap(pa, len);
}

static void *rpbuf_addr_remap(struct rpbuf_controller *controller,
			      struct rpbuf_buffer *buffer,
			      phys_addr_t pa, u64 da, int len)
{
	if (buffer->ops && buffer->ops->addr_remap)
		return buffer->ops->addr_remap(buffer, pa, da, len, buffer->priv);

	if (controller->ops && controller->ops->addr_remap)
		return controller->ops->addr_remap(controller, pa, da, len,
						   controller->priv);

	return rpbuf_addr_remap_default(controller, buffer, pa, da, len);
}

static struct rpbuf_link *rpbuf_create_link(void *token)
{
	struct rpbuf_link *link;

	link = kzalloc(sizeof(struct rpbuf_link), GFP_KERNEL);
	if (IS_ERR_OR_NULL(link)) {
		pr_err("kzalloc for rpbuf_link failed\n");
		link = NULL;
		goto out;
	}
	spin_lock_init(&link->lock);
	mutex_init(&link->service_lock);
	mutex_init(&link->controller_lock);
	link->token = token;
	link->ref_cnt = 0;
out:
	return link;
}

static void rpbuf_destroy_link(struct rpbuf_link *link)
{
	mutex_destroy(&link->service_lock);
	mutex_destroy(&link->controller_lock);
	kfree(link);
}

struct device *
rpbuf_bus_find_device_by_of_node(struct bus_type *bus, struct device_node *np)
{
	return bus_find_device(bus, NULL, np, device_match_of_node);
}
EXPORT_SYMBOL(rpbuf_bus_find_device_by_of_node);

struct rpbuf_service *rpbuf_create_service(struct device *dev,
					   const struct rpbuf_service_ops *ops,
					   void *priv)
{
	struct rpbuf_service *service;

	service = devm_kzalloc(dev, sizeof(struct rpbuf_service), GFP_KERNEL);
	if (IS_ERR_OR_NULL(service)) {
		dev_err(dev, "kzalloc for rpbuf_service failed\n");
		service = NULL;
		goto out;
	}

	service->dev = dev;

	service->ops = ops;
	service->priv = priv;

out:
	return service;
}
EXPORT_SYMBOL(rpbuf_create_service);

void rpbuf_destroy_service(struct rpbuf_service *service)
{
	if (!service) {
		pr_err("invalid service\n");
		return;
	}

	service->ops = NULL;
	service->priv = NULL;

	devm_kfree(service->dev, service);
}
EXPORT_SYMBOL(rpbuf_destroy_service);

int rpbuf_register_service(struct rpbuf_service *service, void *token)
{
	struct rpbuf_link *link;
	unsigned long flags;
	int ret;

	mutex_lock(&__rpbuf_links_lock);

	if (!service || !token) {
		ret = -EINVAL;
		goto out;
	}

	pr_devel("register service 0x%px (token: 0x%px)\n", service, token);

	link = rpbuf_find_link(token);
	if (!link) {
		link = rpbuf_create_link(token);
		if (!link) {
			pr_err("rpbuf_create_link failed\n");
			ret = -EINVAL;
			goto out;
		}
		pr_devel("create new link 0x%px (token: 0x%px)\n", link, token);
		list_add_tail(&link->list, &__rpbuf_links);
	}

	mutex_lock(&link->service_lock);
	spin_lock_irqsave(&link->lock, flags);

	if (link->service && link->service != service) {
		spin_unlock_irqrestore(&link->lock, flags);
		pr_err("another service in the same token has been registered\n");
		ret = -EINVAL;
		goto unlock_service_lock;
	}

	link->service = service;
	service->link = link;
	link->ref_cnt++;

	pr_devel("link: service: 0x%px, controller: 0x%px, ref_cnt: %d\n",
		 link->service, link->controller, link->ref_cnt);

	spin_unlock_irqrestore(&link->lock, flags);

	if (link->controller)
		wake_up_interruptible(&link->controller->wq);
	ret = 0;
unlock_service_lock:
	mutex_unlock(&link->service_lock);
out:
	mutex_unlock(&__rpbuf_links_lock);
	return ret;
}
EXPORT_SYMBOL(rpbuf_register_service);

void rpbuf_unregister_service(struct rpbuf_service *service)
{
	struct rpbuf_link *link = service->link;
	unsigned long flags;

	mutex_lock(&__rpbuf_links_lock);

	mutex_lock(&link->service_lock);
	spin_lock_irqsave(&link->lock, flags);

	pr_devel("unregister service 0x%px\n", service);

	/* we need to reset all connections, when unregister service */
	if (link->controller)
		rpbuf_reset_controller(link->controller);

	link->service = NULL;
	service->link = NULL;
	link->ref_cnt--;

	if (link->ref_cnt > 0) {
		spin_unlock_irqrestore(&link->lock, flags);
		mutex_unlock(&link->service_lock);
		mutex_unlock(&__rpbuf_links_lock);
		return;
	}

	list_del(&link->list);

	spin_unlock_irqrestore(&link->lock, flags);
	mutex_unlock(&link->service_lock);

	pr_devel("destroy link 0x%px\n", link);
	rpbuf_destroy_link(link);

	mutex_unlock(&__rpbuf_links_lock);
}
EXPORT_SYMBOL(rpbuf_unregister_service);

struct rpbuf_controller *rpbuf_create_controller(struct device *dev,
						 const struct rpbuf_controller_ops *ops,
						 void *priv)
{
	struct rpbuf_controller *controller;

	controller = devm_kzalloc(dev, sizeof(struct rpbuf_controller), GFP_KERNEL);
	if (IS_ERR_OR_NULL(controller)) {
		dev_err(dev, "kzalloc for rpbuf_controller failed\n");
		controller = NULL;
		goto out;
	}

	controller->dev = dev;

	controller->ops = ops;
	controller->priv = priv;

	INIT_LIST_HEAD(&controller->remote_dummy_buffers);
	INIT_LIST_HEAD(&controller->local_dummy_buffers);
	idr_init(&controller->buffers);
	idr_init(&controller->local_buffers);
	init_waitqueue_head(&controller->wq);
out:
	return controller;
}
EXPORT_SYMBOL(rpbuf_create_controller);

void rpbuf_destroy_controller(struct rpbuf_controller *controller)
{
	struct device *dev = controller->dev;
	struct rpbuf_buffer *buffer;
	struct rpbuf_buffer *tmp;
	enum rpbuf_role role;
	int id;

	if (!controller) {
		pr_err("invalid controller\n");
		return;
	}

	wake_up_interruptible(&controller->wq);
	role = controller->role;

	list_for_each_entry_safe(buffer, tmp, &controller->remote_dummy_buffers, dummy_list) {
		list_del(&buffer->dummy_list);
		if (buffer->allocated)
			rpbuf_free_buffer_payload_memory(controller, buffer);
		rpbuf_free_buffer_instance(dev, buffer);
	}
	list_for_each_entry_safe(buffer, tmp, &controller->local_dummy_buffers, dummy_list) {
		list_del(&buffer->dummy_list);
		if (buffer->allocated)
			rpbuf_free_buffer_payload_memory(controller, buffer);
		rpbuf_free_buffer_instance(dev, buffer);
		if (role == RPBUF_ROLE_SLAVE)
			idr_remove(&controller->local_buffers, buffer->id);
	}
	idr_for_each_entry(&controller->buffers, buffer, id) {
		if (buffer->allocated)
			rpbuf_free_buffer_payload_memory(controller, buffer);
		rpbuf_free_buffer_instance(dev, buffer);
	}
	idr_destroy(&controller->buffers);
	idr_destroy(&controller->local_buffers);

	controller->priv = NULL;

	devm_kfree(controller->dev, controller);
}
EXPORT_SYMBOL(rpbuf_destroy_controller);

static void rpbuf_reset_controller(struct rpbuf_controller *controller)
{
	struct rpbuf_buffer *buffer, *tmp;
	int id;

	idr_for_each_entry(&controller->buffers, buffer, id) {
		idr_remove(&controller->buffers, id);
		list_add_tail(&buffer->dummy_list, &controller->local_dummy_buffers);
		dev_info(controller->dev,
			"buffer \"%s\" (id:%d): buffers -> local_dummy_buffers\n",
			buffer->name, id);
	}

	list_for_each_entry_safe(buffer, tmp, &controller->remote_dummy_buffers, dummy_list) {
		list_del(&buffer->dummy_list);
		if (buffer->allocated) {
			rpbuf_free_buffer_payload_memory(controller, buffer);
			buffer->allocated = false;
		}
		dev_info(controller->dev, "buffer \"%s\": remote_dummy_buffers -> NULL\n",
			buffer->name);
		rpbuf_free_buffer_instance(controller->dev, buffer);
	}
}

int rpbuf_register_controller(struct rpbuf_controller *controller,
			      void *token, enum rpbuf_role role)
{
	struct rpbuf_link *link;
	unsigned long flags;
	int ret;

	mutex_lock(&__rpbuf_links_lock);

	if (!controller || !token) {
		ret = -EINVAL;
		goto out;
	}

	pr_devel("register controller 0x%px (token: 0x%px)\n", controller, token);

	link = rpbuf_find_link(token);
	if (!link) {
		link = rpbuf_create_link(token);
		if (!link) {
			pr_err("rpbuf_create_link failed\n");
			ret = -EINVAL;
			goto out;
		}
		pr_devel("create new link 0x%px (token: 0x%px)\n", link, token);
		list_add_tail(&link->list, &__rpbuf_links);
	}

	mutex_lock(&link->controller_lock);
	spin_lock_irqsave(&link->lock, flags);

	if (link->controller && link->controller != controller) {
		spin_unlock_irqrestore(&link->lock, flags);
		pr_err("another controller in the same token has been registered\n");
		ret = -EINVAL;
		goto unlock_controller_lock;
	}

	link->controller = controller;
	controller->link = link;
	link->ref_cnt++;

	pr_devel("link: service: 0x%px, controller: 0x%px, ref_cnt: %d\n",
		 link->service, link->controller, link->ref_cnt);

	spin_unlock_irqrestore(&link->lock, flags);

	controller->role = role;

	ret = 0;
unlock_controller_lock:
	mutex_unlock(&link->controller_lock);
out:
	mutex_unlock(&__rpbuf_links_lock);
	return ret;
}
EXPORT_SYMBOL(rpbuf_register_controller);

void rpbuf_unregister_controller(struct rpbuf_controller *controller)
{
	struct rpbuf_link *link = controller->link;
	unsigned long flags;

	mutex_lock(&__rpbuf_links_lock);

	mutex_lock(&link->controller_lock);
	spin_lock_irqsave(&link->lock, flags);

	pr_devel("unregister controller 0x%px\n", controller);

	link->controller = NULL;
	controller->link = NULL;
	link->ref_cnt--;

	if (link->ref_cnt > 0) {
		spin_unlock_irqrestore(&link->lock, flags);
		mutex_unlock(&link->controller_lock);
		mutex_unlock(&__rpbuf_links_lock);
		return;
	}

	list_del(&link->list);

	spin_unlock_irqrestore(&link->lock, flags);
	mutex_unlock(&link->controller_lock);

	pr_devel("destroy link 0x%px\n", link);
	rpbuf_destroy_link(link);

	mutex_unlock(&__rpbuf_links_lock);
}
EXPORT_SYMBOL(rpbuf_unregister_controller);

struct rpbuf_controller *rpbuf_get_controller_by_of_node(const struct device_node *np, int index)
{
	struct device_node *rpbuf_np;
	struct device *dev;

	rpbuf_np = of_parse_phandle(np, "rpbuf", index);
	if (!rpbuf_np) {
		pr_err("no \"rpbuf\" node specified\n");
		return NULL;
	}

	/* We always assume that the rpbuf controller is a platform device */
	dev = rpbuf_bus_find_device_by_of_node(&platform_bus_type, rpbuf_np);
	if (!dev) {
		pr_err("cannot find rpbuf device\n");
		return NULL;
	}

	return (struct rpbuf_controller *)dev_get_drvdata(dev);
}
EXPORT_SYMBOL(rpbuf_get_controller_by_of_node);

int rpbuf_wait_controller_ready(struct rpbuf_controller *controller, int timeout)
{
	struct rpbuf_link *link;

	if (!controller || !controller->link) {
		pr_err("(%s:%d) invalid arguments (%p, %p), Maybe rpbuf is not init?\n", __func__, __LINE__,
				controller, controller->link);
		return -EINVAL;;
	}

	link = controller->link;

	if (wait_event_interruptible_timeout(controller->wq,
					link->service, msecs_to_jiffies(timeout)) < 0)
		return -ERESTARTSYS;

	if (!link->service)
		return -ETIMEDOUT;

	return 0;
}
EXPORT_SYMBOL(rpbuf_wait_controller_ready);

static const int rpbuf_service_message_content_len[RPBUF_SERVICE_CMD_MAX] = {
	0, /* RPBUF_SERVICE_CMD_UNKNOWN */
	sizeof(struct rpbuf_service_content_buffer_created),
	sizeof(struct rpbuf_service_content_buffer_destroyed),
	sizeof(struct rpbuf_service_content_buffer_transmitted),
	sizeof(struct rpbuf_service_content_buffer_ack),
};

/*
 * Return size of the rpbuf service message on success, otherwise a negative
 * number on failure.
 */
static int rpbuf_compose_service_message(u8 *msg, enum rpbuf_service_command cmd,
					 void *content)
{
	struct rpbuf_service_message_header header;
	struct rpbuf_service_message_trailer trailer;
	u8 *p;

	if (cmd == RPBUF_SERVICE_CMD_UNKNOWN || cmd >= RPBUF_SERVICE_CMD_MAX) {
		pr_err("invalid rpbuf service command\n");
		return -EINVAL;
	}

	header.preamble = RPBUF_SERVICE_MESSAGE_PREAMBLE;
	header.command = cmd;

	header.content_len = rpbuf_service_message_content_len[cmd];
	if (header.content_len > RPBUF_SERVICE_MESSAGE_LENGTH_MAX
			- sizeof(struct rpbuf_service_message_header)
			- sizeof(struct rpbuf_service_message_trailer)) {
		pr_err("rpbuf service message content length too long\n");
		return -EINVAL;
	}

	trailer.postamble = RPBUF_SERVICE_MESSAGE_POSTAMBLE;

	p = msg;
	memcpy(p, &header, sizeof(header));
	p += sizeof(header);
	memcpy(p, content, header.content_len);
	p += header.content_len;
	memcpy(p, &trailer, sizeof(trailer));

	return sizeof(header) + header.content_len + sizeof(trailer);
}

static int rpbuf_notify_by_service(struct rpbuf_service *service,
				   enum rpbuf_service_command cmd, void *content)
{
	int ret;
	struct device *dev = service->dev;
	u8 msg[RPBUF_SERVICE_MESSAGE_LENGTH_MAX];
	int msg_len;

	ret = rpbuf_compose_service_message(msg, cmd, content);
	if (ret < 0) {
		dev_err(dev, "failed to generate rpbuf service message\n");
		return ret;
	}
	msg_len = ret;

	if (!service->ops || !service->ops->notify) {
		dev_err(dev, "service has not valid notify operation\n");
		return -EINVAL;
	}
	ret = service->ops->notify(msg, msg_len, service->priv);
	if (ret < 0) {
		dev_err(dev, "notify error: %d\n", ret);
		return ret;
	}

	return 0;
}

/*
 * NOTE:
 *   To avoid deadlock, this function CANNOT be run within 'link->controller_lock'.
 */
static int rpbuf_notify_by_link(struct rpbuf_link *link,
				enum rpbuf_service_command cmd, void *content)
{
	int ret;
	unsigned long flags;
	struct rpbuf_service *service;

	mutex_lock(&link->service_lock);

	spin_lock_irqsave(&link->lock, flags);
	service = link->service;
	if (!service) {
		pr_err("link 0x%px has no service\n", link);
		spin_unlock_irqrestore(&link->lock, flags);
		ret = -ENOENT;
		goto out;
	}
	spin_unlock_irqrestore(&link->lock, flags);

	ret = rpbuf_notify_by_service(service, cmd, content);
	if (ret < 0) {
		dev_err(service->dev, "notify BUFFER_CREATED error\n");
		goto out;
	}

	ret = 0;
out:
	mutex_unlock(&link->service_lock);
	return ret;
}

int rpbuf_notify_remotebuffer_complete(struct rpbuf_controller *controller,
				struct rpbuf_buffer *buffer)
{
	int ret;
	int msg_len;
	struct rpbuf_link *link;
	struct rpbuf_service *service;
	struct rpbuf_service_content_buffer_ack content;
	u8 msg[RPBUF_SERVICE_MESSAGE_LENGTH_MAX];

	if (buffer->need_ack != true)
		return 0;

	buffer->need_ack = false;
	link = controller->link;
	service = link->service;

	content.id = buffer->id;
	content.timediff = 0;

	ret = rpbuf_compose_service_message(msg, RPBUF_SERVICE_CMD_BUFFER_ACK, &content);
	if (ret < 0) {
		dev_err(controller->dev, "failed to generate rpbuf service message\n");
		return ret;
	}
	msg_len = ret;

	if (!service->ops || !service->ops->notify) {
		dev_err(service->dev, "service has not valid notify operation\n");
		return -EINVAL;
	}
	ret = service->ops->notify(msg, msg_len, service->priv);
	if (ret < 0) {
		dev_err(service->dev, "notify error: %d\n", ret);
		return ret;
	}

	return 0;
}

static int rpbuf_wait_for_remotebuffer_complete(struct rpbuf_controller *controller,
				struct rpbuf_buffer *buffer)
{
	/* already get notify? */
	if (!(buffer->state & RPBUF_FLAGS_GETNOTIFY)) {
		if (wait_event_interruptible(buffer->wait,
								(buffer->state & (RPBUF_FLAGS_GETNOTIFY | RPBUF_FLAGS_DESTROYED))))
				return -ERESTARTSYS;
	}

	/* clear notify bit */
	buffer->state &= ~RPBUF_FLAGS_GETNOTIFY;

	if (buffer->state & RPBUF_FLAGS_DESTROYED)
		return -ECONNRESET;

	return 0;
}

static int rpbuf_service_command_buffer_created_handler(struct rpbuf_service *service,
							enum rpbuf_service_command cmd,
							void *content)
{
	int ret;
	struct rpbuf_link *link = service->link;
	struct rpbuf_controller *controller;
	unsigned long flags;
	enum rpbuf_role role;
	struct rpbuf_service_content_buffer_created *cont = content;
	struct rpbuf_service_content_buffer_created content_back;
	struct rpbuf_buffer *buffer;
	int id;
	int is_available = 0;

	mutex_lock(&link->controller_lock);

	spin_lock_irqsave(&link->lock, flags);
	controller = link->controller;
	if (!controller) {
		dev_err(service->dev, "service 0x%px not linked with controller\n",
			service);
		spin_unlock_irqrestore(&link->lock, flags);
		ret = -ENOENT;
		goto unlock_controller_lock;
	}
	spin_unlock_irqrestore(&link->lock, flags);

	role = controller->role;
	if (role != RPBUF_ROLE_MASTER && role != RPBUF_ROLE_SLAVE) {
		dev_err(controller->dev, "unknown rpbuf role\n");
		ret = -EINVAL;
		goto unlock_controller_lock;
	}

	/*
	 * Check whether buffer with the same name already exists in 'buffers'.
	 * It should not exist normally. But if exists, it means that remote
	 * had some issues before and is re-allocating the buffer now. At this
	 * time if remote buffer is not available, we need to send BUFFER_CREATED
	 * command back to remote in order to complete its buffer creation.
	 */
	idr_for_each_entry(&controller->buffers, buffer, id) {
		if (0 == strncmp(buffer->name, cont->name, RPBUF_NAME_SIZE)) {
			dev_info(controller->dev,
				 "(unexpected) remote re-allocating buffer \"%s\" "
				 "(available: %d)\n",
				 buffer->name, cont->is_available);

			if (cont->is_available) {
				ret = 0;
				goto unlock_controller_lock;
			}

			strncpy(content_back.name, buffer->name, RPBUF_NAME_SIZE);
			content_back.id = buffer->id;
			content_back.len = buffer->len;
			content_back.pa = buffer->pa;
			content_back.da = buffer->da;
			content_back.is_available = 1;

			mutex_unlock(&link->controller_lock);

			ret = rpbuf_notify_by_link(link,
						   RPBUF_SERVICE_CMD_BUFFER_CREATED,
						   (void *)&content_back);
			if (ret < 0) {
				dev_err(controller->dev, "rpbuf_notify_by_link "
					"BUFFER_CREATED failed: %d\n", ret);
				goto out;
			}
			ret = 0;
			goto out;
		}
	}

	buffer = rpbuf_find_dummy_buffer_by_name(&controller->local_dummy_buffers,
						 cont->name);
	if (buffer) {
		/*
		 * A buffer with the same name is found in 'local_dummy_buffers',
		 * meaning that it has been allocated by local before. Now we
		 * update its information and move it to 'buffers'.
		 */
		if (buffer->len != cont->len) {
			dev_err(controller->dev, "buffer length not match with remote "
				"(local: %d, remote: %u)\n", buffer->len, cont->len);
			ret = -EINVAL;
			goto unlock_controller_lock;
		}

		if (role == RPBUF_ROLE_SLAVE) {
			buffer->pa = cont->pa;
			buffer->da = cont->da;
			buffer->va = rpbuf_addr_remap(controller, buffer,
						      buffer->pa, buffer->da,
						      buffer->len);
		} else {
			if (!buffer->allocated) {
				ret = rpbuf_alloc_buffer_payload_memory(controller, buffer);
				if (ret < 0) {
					dev_err(controller->dev, "rpbuf_alloc_buffer_payload_memory failed\n");
					ret = -ENOMEM;
					goto unlock_controller_lock;
				}
				buffer->allocated = true;
			}
		}

		if (role == RPBUF_ROLE_SLAVE)
			id = buffer->id;
		else
			id = cont->id;

		id = idr_alloc(&controller->buffers, buffer,
			       id, id + 1, GFP_KERNEL);
		if (id < 0) {
			dev_err(controller->dev, "idr_alloc for id %d failed: %d\n",
				cont->id, id);
			ret = id;
			goto unlock_controller_lock;
		}
		buffer->id = id;

		list_del(&buffer->dummy_list);

		dev_info(controller->dev,
			"buffer \"%s\" (id:%d): local_dummy_buffers -> buffers\n",
			buffer->name, buffer->id);
		is_available = 1;
	} else {
		/*
		 * If not found, create a new buffer to save this information
		 * and add it to 'remote_dummy_buffers', waiting for local to
		 * allocate it manually later.
		 *
		 * In this situation, if a buffer with the same name already
		 * exists in 'remote_dummy_buffers', it is an unexpected state.
		 * Maybe remote had some issues before and is re-allocating the
		 * buffer now. Anyway, we overwrite the previous information.
		 */

		buffer = rpbuf_find_dummy_buffer_by_name(&controller->remote_dummy_buffers,
							 cont->name);
		if (buffer) {
			dev_info(controller->dev, "buffer \"%s\": (unexpected) "
				"overwrite information in 'remote_dummy_buffers'\n",
				buffer->name);
			strncpy(buffer->name, cont->name, RPBUF_NAME_SIZE);
			buffer->len = cont->len;
			buffer->id = cont->id;
			buffer->pa = cont->pa;
			buffer->da = cont->da;
		} else {
			buffer = rpbuf_alloc_buffer_instance(controller->dev,
							     cont->name, cont->len);
			if (!buffer) {
				dev_err(controller->dev, "rpbuf_alloc_buffer_instance failed\n");
				ret = -ENOMEM;
				goto unlock_controller_lock;
			}
			buffer->id = cont->id;
			buffer->pa = cont->pa;
			buffer->da = cont->da;
			list_add_tail(&buffer->dummy_list,
				      &controller->remote_dummy_buffers);

			dev_info(controller->dev, "buffer \"%s\": NULL -> remote_dummy_buffers\n",
				buffer->name);
		}
	}

	ret = 0;

	if (is_available && buffer->cbs && buffer->cbs->available_cb) {
		buffer->state = 0;
		buffer->state |= RPBUF_FLAGS_WORKING;

		mutex_unlock(&link->controller_lock);
		buffer->cbs->available_cb(buffer, buffer->priv);

		buffer->state &= ~RPBUF_FLAGS_WORKING;
		if (buffer->state & RPBUF_FLAGS_DESTROYED)
			wake_up_interruptible(&buffer->wait);
		goto out;
	}

unlock_controller_lock:
	mutex_unlock(&link->controller_lock);
out:
	return ret;
}

static int rpbuf_service_command_buffer_destroyed_handler(struct rpbuf_service *service,
							  enum rpbuf_service_command cmd,
							  void *content)
{
	int ret;
	struct rpbuf_link *link = service->link;
	struct rpbuf_controller *controller;
	unsigned long flags;
	enum rpbuf_role role;
	struct rpbuf_service_content_buffer_destroyed *cont = content;
	struct rpbuf_buffer *buffer;

	mutex_lock(&link->controller_lock);

	spin_lock_irqsave(&link->lock, flags);
	controller = link->controller;
	if (!controller) {
		dev_err(service->dev, "service 0x%px not linked with controller\n", service);
		spin_unlock_irqrestore(&link->lock, flags);
		ret = -ENOENT;
		goto err_out;
	}
	spin_unlock_irqrestore(&link->lock, flags);

	role = controller->role;
	if (role != RPBUF_ROLE_MASTER && role != RPBUF_ROLE_SLAVE) {
		dev_err(controller->dev, "unknown rpbuf role\n");
		ret = -EINVAL;
		goto err_out;
	}

	buffer = rpbuf_find_dummy_buffer_by_id(&controller->remote_dummy_buffers, cont->id);
	if (buffer) {
		/*
		 * A buffer with the same name is found in 'remote_dummy_buffers',
		 * meaning that local doesn't allocate it or has freed it.
		 * Now we delete it from the list.
		 */
		list_del(&buffer->dummy_list);

		if (buffer->allocated) {
			rpbuf_free_buffer_payload_memory(controller, buffer);
			buffer->allocated = false;
		}

		dev_info(controller->dev, "buffer \"%s\": remote_dummy_buffers -> NULL\n",
			buffer->name);

		/*
		 * Remember to free the instance after deleting a entry from
		 * 'remote_dummy_buffers'.
		 */
		rpbuf_free_buffer_instance(controller->dev, buffer);
		buffer = NULL;
	} else {
		/*
		 * If not found, meaning that local has allocated it. Now it
		 * should be stored in 'buffers'. We just move it to
		 * 'local_dummy_buffers' and wait for local to free it manually
		 * later.
		 */
		buffer = idr_remove(&controller->buffers, cont->id);
		if (buffer) {
			list_add_tail(&buffer->dummy_list, &controller->local_dummy_buffers);
			dev_info(controller->dev,
				"buffer \"%s\" (id:%d): buffers -> local_dummy_buffers\n",
				buffer->name, cont->id);
		} else {
			dev_err(controller->dev, "buffer not found, "
				"nothing handled with BUFFER_DESTROYED\n");
			ret = -EINVAL;
			goto err_out;
		}
	}

	if (buffer)
		buffer->state |= RPBUF_FLAGS_WORKING;

	mutex_unlock(&link->controller_lock);

	ret = 0;
	if (buffer && buffer->cbs && buffer->cbs->destroyed_cb)
		ret = buffer->cbs->destroyed_cb(buffer, buffer->priv);

	if (buffer) {
		buffer->state &= ~RPBUF_FLAGS_WORKING;
		buffer->state |= RPBUF_FLAGS_DESTROYED;
		wake_up_interruptible(&buffer->wait);
	}

	return ret;

err_out:
	mutex_unlock(&link->controller_lock);
	return ret;
}

static int rpbuf_service_command_buffer_transmitted_handler(struct rpbuf_service *service,
							    enum rpbuf_service_command cmd,
							    void *content)
{
	int ret;
	struct rpbuf_link *link = service->link;
	struct rpbuf_controller *controller;
	unsigned long flags;
	struct rpbuf_service_content_buffer_transmitted *cont = content;
	struct rpbuf_buffer *buffer = NULL;

	mutex_lock(&link->controller_lock);

	spin_lock_irqsave(&link->lock, flags);
	controller = link->controller;
	if (!controller) {
		dev_err(service->dev, "service 0x%px not linked with controller\n", service);
		spin_unlock_irqrestore(&link->lock, flags);
		mutex_unlock(&link->controller_lock);
		ret = -ENOENT;
		goto err_out;
	}
	spin_unlock_irqrestore(&link->lock, flags);

	buffer = idr_find(&controller->buffers, cont->id);
	if (!buffer) {
		dev_warn(controller->dev, "no buffer with id %d in local\n", cont->id);
		mutex_unlock(&link->controller_lock);
		ret = -ENOENT;
		goto err_out;
	}

	buffer->state |= RPBUF_FLAGS_WORKING;
	dev_dbg(controller->dev, "buffer \"%s\" (id:%d) received from remote\n",
		buffer->name, buffer->id);

	mutex_unlock(&link->controller_lock);

	if (buffer->state & RPBUF_FLAGS_DESTROYED) {
		ret = -ECONNREFUSED;
		goto err_out;
	}

	if ((cont->flags & BUFFER_SYNC_TRANSMIT)) {
		buffer->need_ack = true;
	}

	if (buffer->cbs && buffer->cbs->rx_cb)
		buffer->cbs->rx_cb(buffer, buffer->va + cont->offset,
					  cont->data_len, buffer->priv);

	/* tell the remote buffer that we received data */
	if ((cont->flags & BUFFER_SYNC_TRANSMIT)) {
		if (!buffer->is_used_by_buf_dev) {
			ret = rpbuf_notify_remotebuffer_complete(controller, buffer);
			if (ret < 0) {
				dev_warn(controller->dev, "buffer \"%s\" (id:%d) notify remote failed\n",
							rpbuf_buffer_name(buffer), rpbuf_buffer_id(buffer));
				ret = -EFAULT;
				goto err_out;
			}
		}
	}

	buffer->state &= ~RPBUF_FLAGS_WORKING;
	if (buffer->state & RPBUF_FLAGS_DESTROYED)
		wake_up_interruptible(&buffer->wait);

	return 0;

err_out:
	if (buffer) {
		buffer->state &= ~RPBUF_FLAGS_WORKING;
		if (buffer->state & RPBUF_FLAGS_DESTROYED)
			wake_up_interruptible(&buffer->wait);
	}

	return ret;
}

static int rpbuf_service_command_buffer_ack_handler(struct rpbuf_service *service,
							    enum rpbuf_service_command cmd,
							    void *content)
{
	int ret;
	struct rpbuf_link *link = service->link;
	struct rpbuf_controller *controller;
	unsigned long flags;
	struct rpbuf_service_content_buffer_ack *cont = content;
	struct rpbuf_buffer *buffer;

	mutex_lock(&link->controller_lock);

	spin_lock_irqsave(&link->lock, flags);
	controller = link->controller;
	if (!controller) {
		dev_err(service->dev, "service 0x%px not linked with controller\n", service);
		spin_unlock_irqrestore(&link->lock, flags);
		mutex_unlock(&link->controller_lock);
		ret = -ENOENT;
		goto err_out;
	}
	spin_unlock_irqrestore(&link->lock, flags);

	buffer = idr_find(&controller->buffers, cont->id);
	if (!buffer) {
		dev_warn(controller->dev, "no buffer with id %d in local\n", cont->id);
		mutex_unlock(&link->controller_lock);
		ret = -ENOENT;
		goto err_out;
	}

	buffer->state |= RPBUF_FLAGS_GETNOTIFY;

	dev_dbg(controller->dev, "buffer \"%s\" (id:%d) ACK from remote\n",
		buffer->name, buffer->id);

	mutex_unlock(&link->controller_lock);

	wake_up_interruptible(&buffer->wait);

	return 0;

err_out:
	return ret;
}

static const rpbuf_service_command_handler_t
rpbuf_service_command_handlers[RPBUF_SERVICE_CMD_MAX] = {
	NULL, /* RPBUF_SERVICE_CMD_UNKNOWN */
	rpbuf_service_command_buffer_created_handler,
	rpbuf_service_command_buffer_destroyed_handler,
	rpbuf_service_command_buffer_transmitted_handler,
	rpbuf_service_command_buffer_ack_handler,
};

int rpbuf_service_get_notification(struct rpbuf_service *service, void *msg, int msg_len)
{
	int ret;
	struct device *dev = service->dev;
	struct rpbuf_service_message_header *header = msg;
	struct rpbuf_service_message_trailer *trailer
		= msg + sizeof(*header) + header->content_len;
	enum rpbuf_service_command cmd;

	if (sizeof(*header) + header->content_len + sizeof(*trailer) != msg_len) {
		dev_err(dev, "incorrecti rpbuf service message length\n");
		ret = -EINVAL;
		goto err_out;
	}
	if (header->preamble != RPBUF_SERVICE_MESSAGE_PREAMBLE
			|| trailer->postamble != RPBUF_SERVICE_MESSAGE_POSTAMBLE) {
		dev_err(dev, "invalid received rpbuf service message\n");
		ret = -EINVAL;
		goto err_out;
	}
	if (header->command == RPBUF_SERVICE_CMD_UNKNOWN
			|| header->command >= RPBUF_SERVICE_CMD_MAX) {
		pr_err("invalid rpbuf service command\n");
		return -EINVAL;
	}
	cmd = header->command;
	if (header->content_len != rpbuf_service_message_content_len[cmd]) {
		dev_err(dev, "rpbuf service message content length not match "
			"(command: %d, request: %d, actual: %d)\n",
			cmd, rpbuf_service_message_content_len[cmd], header->content_len);
		ret = -EINVAL;
		goto err_out;
	}

	ret = rpbuf_service_command_handlers[cmd](service, cmd, msg + sizeof(*header));
	if (ret < 0) {
		dev_err(dev, "rpbuf_service_command_handlers[%d] error (%d)\n",
			cmd, ret);
		goto err_out;
	}

	return 0;
err_out:
	return ret;
}
EXPORT_SYMBOL(rpbuf_service_get_notification);

static int rpbuf_free_buffer_internal(struct rpbuf_buffer *buffer, int do_notify)
{
	int ret;
	struct rpbuf_controller *controller;
	struct rpbuf_link *link;
	struct device *dev;
	enum rpbuf_role role;
	struct rpbuf_buffer *dummy_buffer;
	struct rpbuf_service_content_buffer_destroyed content;

	if (!buffer || !buffer->controller || !buffer->controller->link) {
		pr_err("invalid arguments\n");
		ret = -EINVAL;
		goto err_out;
	}
	controller = buffer->controller;
	link = controller->link;
	dev = controller->dev;
	role = controller->role;

	if (role != RPBUF_ROLE_MASTER && role != RPBUF_ROLE_SLAVE) {
		dev_err(dev, "unknown rpbuf role\n");
		ret = -EINVAL;
		goto err_out;
	}

	buffer->state |= RPBUF_FLAGS_DESTROYED;

	content.id = buffer->id;

	if (do_notify) {
		ret = rpbuf_notify_by_link(link,
					   RPBUF_SERVICE_CMD_BUFFER_DESTROYED,
					   (void *)&content);
		if (ret < 0) {
			dev_err(dev, "rpbuf_notify_by_link BUFFER_DESTROYED failed: %d\n", ret);
			/*
			 * Though notifying failed, we go ahead to release the
			 * local buffer resources rather than return directly.
			 */
		}
	}

	mutex_lock(&link->controller_lock);

	if (buffer->state & RPBUF_FLAGS_WORKING) {
		mutex_unlock(&link->controller_lock);
		/* wait for buffer to complete current callback */
		if (wait_event_interruptible(buffer->wait, !(buffer->state & (RPBUF_FLAGS_WORKING)))) {
			ret = -ERESTARTSYS;
			goto err_out;
		}
		mutex_lock(&link->controller_lock);
	}

	dummy_buffer = rpbuf_find_dummy_buffer_by_name(
			&controller->local_dummy_buffers, buffer->name);
	if (dummy_buffer) {
		/*
		 * A buffer with the same name is found in 'local_dummy_buffers',
		 * meaning that remote doesn't allocate it or has freed it.
		 * Here the 'dummy_buffer' and 'buffer' should be the same.
		 */

		if (dummy_buffer != buffer) {
			dev_err(dev, "unexpected buffer ptr 0x%p, it should be 0x%p\n",
				buffer, dummy_buffer);
			mutex_unlock(&link->controller_lock);
			ret = -EINVAL;
			goto err_out;
		}

		list_del(&buffer->dummy_list);

		if (buffer->allocated) {
			rpbuf_free_buffer_payload_memory(controller, buffer);
			buffer->allocated = false;
		}

		dev_info(dev, "buffer \"%s\": local_dummy_buffers -> NULL\n",
			buffer->name);
	} else {
		/*
		 * If not found, meaning that remote has allocated it. Now it
		 * should be stored in 'buffers' and we should move it
		 * to 'remote_dummy_buffers'.
		 */

		dummy_buffer = idr_remove(&controller->buffers, buffer->id);
		if (!dummy_buffer) {
			dev_err(dev, "buffer not found, maybe it is not allocated?\n");
			mutex_unlock(&link->controller_lock);
			ret = -EINVAL;
			goto err_out;
		} else if (dummy_buffer != buffer) {
			dev_err(dev, "unexpected buffer ptr 0x%p, it should be 0x%p\n",
				buffer, dummy_buffer);
			mutex_unlock(&link->controller_lock);
			ret = -EINVAL;
			goto err_out;
		}

		dummy_buffer = rpbuf_find_dummy_buffer_by_name(
				&controller->remote_dummy_buffers, buffer->name);
		if (dummy_buffer) {
			/*
			 * In this situation, if a buffer with the same name
			 * already exists in 'remote_dummy_buffers', it is an
			 * unexpected state. Anyway, we overwrite the previous
			 * information.
			 */
			dev_info(dev, "buffer \"%s\": (unexpected) "
				"overwrite information in 'remote_dummy_buffers'\n",
				buffer->name);
			strncpy(dummy_buffer->name, buffer->name, RPBUF_NAME_SIZE);
			dummy_buffer->len = buffer->len;
			dummy_buffer->id = buffer->id;
			dummy_buffer->pa = buffer->pa;
			dummy_buffer->da = buffer->da;
			dummy_buffer->allocated = buffer->allocated;
		} else {
			/*
			 * The instance of 'buffer' will be freed later, so we
			 * cannot directly add it to 'remote_dummy_buffers'.
			 * Instead, we allocate a new instance and copy 'buffer'
			 * information to it.
			 */
			dummy_buffer = rpbuf_alloc_buffer_instance(dev,
								   buffer->name,
								   buffer->len);
			if (!dummy_buffer) {
				dev_err(dev, "rpbuf_alloc_buffer_instance failed\n");
				mutex_unlock(&link->controller_lock);
				ret = -ENOMEM;
				goto err_out;
			}
			dummy_buffer->id = buffer->id;
			dummy_buffer->pa = buffer->pa;
			dummy_buffer->da = buffer->da;
			dummy_buffer->va = buffer->va;
			dummy_buffer->allocated = buffer->allocated;
			list_add_tail(&dummy_buffer->dummy_list,
				      &controller->remote_dummy_buffers);

			dev_info(dev, "buffer \"%s\" (id:%d): buffers -> remote_dummy_buffers\n",
				dummy_buffer->name, dummy_buffer->id);
		}
	}

	if (role == RPBUF_ROLE_SLAVE)
		idr_remove(&controller->local_buffers, buffer->id);

	mutex_unlock(&link->controller_lock);

	rpbuf_free_buffer_instance(dev, buffer);

	return 0;
err_out:
	return ret;
}

struct rpbuf_buffer *rpbuf_alloc_buffer(struct rpbuf_controller *controller,
					const char *name, int len,
					const struct rpbuf_buffer_ops *ops,
					const struct rpbuf_buffer_cbs *cbs,
					void *priv)
{
	int ret;
	struct device *dev;
	struct rpbuf_link *link;
	enum rpbuf_role role;
	struct rpbuf_buffer *buffer;
	struct rpbuf_buffer *dummy_buffer;
	int id;
	struct rpbuf_service_content_buffer_created content;
	int is_available = 0;

	if (!controller || !controller->link) {
		pr_err("(%s:%d) invalid arguments (%p, %p)\n", __func__, __LINE__,
				controller, controller->link);
		goto err_out;
	}
	dev = controller->dev;
	link = controller->link;
	role = controller->role;

	if (role != RPBUF_ROLE_MASTER && role != RPBUF_ROLE_SLAVE) {
		dev_err(dev, "unknown rpbuf role\n");
		goto err_out;
	}

	buffer = rpbuf_alloc_buffer_instance(dev, name, len);
	if (!buffer) {
		dev_err(dev, "rpbuf_alloc_buffer_instance failed\n");
		goto err_out;
	}
	buffer->controller = controller;
	buffer->ops = ops;
	buffer->cbs = cbs;
	buffer->priv = priv;
	buffer->allocated = false;

	mutex_lock(&link->controller_lock);

	dummy_buffer = rpbuf_find_dummy_buffer_by_name(
			&controller->local_dummy_buffers, name);
	if (dummy_buffer) {
		dev_err(dev, "buffer \"%s\" already exists in 'local_dummy_buffers'\n", name);
		mutex_unlock(&link->controller_lock);
		goto err_free_instance;
	}
	idr_for_each_entry(&controller->buffers, dummy_buffer, id) {
		if (0 == strncmp(dummy_buffer->name, name, RPBUF_NAME_SIZE)) {
			dev_err(dev, "buffer \"%s\" already exists in 'buffers'\n", name);
			mutex_unlock(&link->controller_lock);
			goto err_free_instance;
		}
	}

	dummy_buffer = rpbuf_find_dummy_buffer_by_name(
			&controller->remote_dummy_buffers, name);
	if (dummy_buffer) {
		/*
		 * A buffer with the same name is found in 'remote_dummy_buffers',
		 * meaning that remote has allocated it and notified local.
		 * Now we move it to 'buffers'.
		 */

		if (buffer->len != dummy_buffer->len) {
			dev_err(dev, "buffer length not match with remote "
				"(local: %d, remote: %d)\n",
				buffer->len, dummy_buffer->len);
			mutex_unlock(&link->controller_lock);
			goto err_free_instance;
		}

		if (role == RPBUF_ROLE_SLAVE) {
			buffer->pa = dummy_buffer->pa;
			buffer->da = dummy_buffer->da;
			buffer->va = rpbuf_addr_remap(controller, buffer,
						      buffer->pa, buffer->da,
						      buffer->len);

			id = idr_alloc(&controller->local_buffers, buffer, 0, 0, GFP_KERNEL);
			if (id < 0) {
				dev_err(dev, "idr_alloc for new id failed: %d\n", id);
				mutex_unlock(&link->controller_lock);
				goto err_free_instance;
			}
			buffer->id = id;

			id = idr_alloc(&controller->buffers, buffer, id, id + 1, GFP_KERNEL);
			if (id < 0) {
				dev_err(dev, "idr_alloc for new id %d failed: %d\n", dummy_buffer->id, id);
				idr_remove(&controller->local_buffers, buffer->id);
				mutex_unlock(&link->controller_lock);
				goto err_free_instance;
			}
		} else {
			if (!dummy_buffer->allocated) {
				ret = rpbuf_alloc_buffer_payload_memory(controller, buffer);
				if (ret < 0) {
					dev_err(dev, "rpbuf_alloc_buffer_payload_memory failed\n");
					mutex_unlock(&link->controller_lock);
					goto err_free_instance;
				}
				dummy_buffer->allocated = true;
			} else {
				buffer->pa = dummy_buffer->pa;
				buffer->da = dummy_buffer->da;
				buffer->va = dummy_buffer->va;
			}
			buffer->allocated = dummy_buffer->allocated;

			id = idr_alloc(&controller->buffers, buffer, dummy_buffer->id,
						      dummy_buffer->id + 1, GFP_KERNEL);
			if (id < 0) {
				dev_err(dev, "idr_alloc for new id %d failed: %d\n", dummy_buffer->id, id);
				mutex_unlock(&link->controller_lock);
				goto err_free_instance;
			}
			buffer->id = id;
		}

		/*
		 * The instances of entries in 'remote_dummy_buffers' are not
		 * allocated/freed by rpbuf_alloc_buffer()/rpbuf_free_buffer().
		 * After deleting from the list, we should free the instance.
		 */
		list_del(&dummy_buffer->dummy_list);
		rpbuf_free_buffer_instance(dev, dummy_buffer);

		dev_info(dev, "buffer \"%s\" (id:%d): remote_dummy_buffers -> buffers\n",
			buffer->name, buffer->id);
		is_available = 1;
	} else {
		/*
		 * If not found, add this buffer to 'local_dummy_buffers',
		 * waiting for remote message to update it.
		 */
		list_add_tail(&buffer->dummy_list, &controller->local_dummy_buffers);

		/*
		 * buffer->id is determined by slave, because the slave may run on
		 * bare metal and there no idr mechanism.
		 */
		if (role == RPBUF_ROLE_SLAVE) {
			id = idr_alloc(&controller->local_buffers, buffer, 0, 0, GFP_KERNEL);
			if (id < 0) {
				dev_err(dev, "idr_alloc for new id failed: %d\n", id);
				mutex_unlock(&link->controller_lock);
				goto err_free_instance;
			}
			buffer->id = id;
		} else {
			ret = rpbuf_alloc_buffer_payload_memory(controller, buffer);
			if (ret < 0) {
				dev_err(controller->dev, "rpbuf_alloc_buffer_payload_memory failed\n");
				ret = -ENOMEM;
				goto err_free_instance;
			}
			buffer->allocated = true;
		}
		dev_info(dev, "buffer \"%s\": NULL -> local_dummy_buffers\n",
			buffer->name);
	}

	mutex_unlock(&link->controller_lock);

	/* Copy buffer information to rpbuf message content */
	strncpy(content.name, buffer->name, RPBUF_NAME_SIZE);
	content.id = buffer->id;
	content.len = buffer->len;
	content.pa = buffer->pa;
	content.da = buffer->da;
	content.is_available = is_available;

	/* Notify remote */
	ret = rpbuf_notify_by_link(link,
				   RPBUF_SERVICE_CMD_BUFFER_CREATED,
				   (void *)&content);
	if (ret < 0) {
		dev_err(dev, "rpbuf_notify_by_link BUFFER_CREATED failed: %d\n", ret);
		/* Free allocated resource, without sending BUFFER_DESTROYED to remote */
		rpbuf_free_buffer_internal(buffer, 0);
		goto err_out;
	}

	if (is_available && buffer->cbs && buffer->cbs->available_cb)
		buffer->cbs->available_cb(buffer, buffer->priv);

	return buffer;

err_free_instance:
	rpbuf_free_buffer_instance(dev, buffer);
err_out:
	return NULL;
}
EXPORT_SYMBOL(rpbuf_alloc_buffer);

int rpbuf_free_buffer(struct rpbuf_buffer *buffer)
{
	return rpbuf_free_buffer_internal(buffer, 1);
}
EXPORT_SYMBOL(rpbuf_free_buffer);

int rpbuf_buffer_set_sync(struct rpbuf_buffer *buffer, bool sync)
{
	uint32_t flags = buffer->flags;

	if (sync)
		flags |= BUFFER_SYNC_TRANSMIT;
	else
		flags &= ~(BUFFER_SYNC_TRANSMIT);

	buffer->flags = flags;

	return 0;
}
EXPORT_SYMBOL(rpbuf_buffer_set_sync);

int rpbuf_buffer_is_available(struct rpbuf_buffer *buffer)
{
	struct rpbuf_controller *controller;
	struct rpbuf_link *link;
	struct rpbuf_buffer *tmp_buffer;

	if (!buffer || !buffer->controller || !buffer->controller->link) {
		pr_err("invalid arguments\n");
		return 0;
	}
	controller = buffer->controller;
	link = controller->link;

	mutex_lock(&link->controller_lock);

	tmp_buffer = idr_find(&controller->buffers, buffer->id);
	if (!tmp_buffer) {
		mutex_unlock(&link->controller_lock);
		return 0;
	} else if (tmp_buffer != buffer) {
		dev_err(controller->dev, "unexpected buffer ptr 0x%p, it should be 0x%p\n",
			buffer, tmp_buffer);
		mutex_unlock(&link->controller_lock);
		return 0;
	}

	mutex_unlock(&link->controller_lock);

	return 1;
}
EXPORT_SYMBOL(rpbuf_buffer_is_available);

int rpbuf_transmit_buffer(struct rpbuf_buffer *buffer,
			  unsigned int offset, unsigned int data_len)
{
	int ret;
	struct rpbuf_controller *controller;
	struct rpbuf_link *link;
	struct device *dev;
	struct rpbuf_service_content_buffer_transmitted content;

	if (!buffer || !buffer->controller || !buffer->controller->link) {
		pr_err("invalid arguments\n");
		return -EINVAL;
	}
	controller = buffer->controller;
	link = controller->link;
	dev = controller->dev;

	content.id = buffer->id;
	content.offset = offset;
	content.data_len = data_len;
	content.flags = buffer->flags;

	if (!rpbuf_buffer_is_available(buffer)) {
		dev_err(dev, "buffer not available\n");
		return -EACCES;
	}

	/* clear notify bit */
	if ((buffer->flags & BUFFER_SYNC_TRANSMIT))
		buffer->state &= ~RPBUF_FLAGS_GETNOTIFY;

	ret = rpbuf_notify_by_link(link, RPBUF_SERVICE_CMD_BUFFER_TRANSMITTED,
				   (void *)&content);
	if (ret < 0) {
		dev_err(dev, "rpbuf_notify_by_link BUFFER_TRANSMITTED failed: %d\n", ret);
		return ret;
	}

	if ((buffer->flags & BUFFER_SYNC_TRANSMIT)) {
		ret = rpbuf_wait_for_remotebuffer_complete(controller, buffer);
		if (ret < 0) {
			dev_err(dev, "rpbuf_wait_for_remotebuffer_complete BUFFER_ACK failed: %d\n", ret);
			return ret;
		}
	}

	return 0;
}
EXPORT_SYMBOL(rpbuf_transmit_buffer);

const char *rpbuf_buffer_name(struct rpbuf_buffer *buffer)
{
	return buffer->name;
}
EXPORT_SYMBOL(rpbuf_buffer_name);

int rpbuf_buffer_id(struct rpbuf_buffer *buffer)
{
	return buffer->id;
}
EXPORT_SYMBOL(rpbuf_buffer_id);

void *rpbuf_buffer_va(struct rpbuf_buffer *buffer)
{
	return buffer->va;
}
EXPORT_SYMBOL(rpbuf_buffer_va);

phys_addr_t rpbuf_buffer_pa(struct rpbuf_buffer *buffer)
{
	return buffer->pa;
}
EXPORT_SYMBOL(rpbuf_buffer_pa);

u64 rpbuf_buffer_da(struct rpbuf_buffer *buffer)
{
	return buffer->da;
}
EXPORT_SYMBOL(rpbuf_buffer_da);

int rpbuf_buffer_len(struct rpbuf_buffer *buffer)
{
	return buffer->len;
}
EXPORT_SYMBOL(rpbuf_buffer_len);

void *rpbuf_buffer_priv(struct rpbuf_buffer *buffer)
{
	return buffer->priv;
}
EXPORT_SYMBOL(rpbuf_buffer_priv);

void rpbuf_buffer_set_va(struct rpbuf_buffer *buffer, void *va)
{
	buffer->va = va;
}
EXPORT_SYMBOL(rpbuf_buffer_set_va);

void rpbuf_buffer_set_pa(struct rpbuf_buffer *buffer, phys_addr_t pa)
{
	buffer->pa = pa;
}
EXPORT_SYMBOL(rpbuf_buffer_set_pa);

void rpbuf_buffer_set_da(struct rpbuf_buffer *buffer, u64 da)
{
	buffer->da = da;
}
EXPORT_SYMBOL(rpbuf_buffer_set_da);

MODULE_DESCRIPTION("RPBuf core");
MODULE_AUTHOR("Junyan Lin <junyanlin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPBUF_CORE_VERSION);

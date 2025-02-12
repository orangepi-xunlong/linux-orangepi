/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/kobject.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include "mvx_log_group.h"
#include "mvx_secure.h"

/****************************************************************************
 * Types
 ****************************************************************************/

#pragma pack(push, 1)
struct secure_firmware_desc {
	int32_t fd;
	uint32_t l2pages;
	struct {
		uint32_t major;
		uint32_t minor;
	} protocol;
};
#pragma pack(pop)

struct mvx_secure_firmware_priv {
	struct device *dev;
	struct kobject kobj;
	struct work_struct work;
	wait_queue_head_t wait_queue;
	struct mvx_secure_firmware fw;
	mvx_secure_firmware_done done;
	void *done_arg;
};

struct mvx_secure_mem {
	struct device *dev;
	struct kobject kobj;
	wait_queue_head_t wait_queue;
	struct dma_buf *dmabuf;
};

/****************************************************************************
 * Secure
 ****************************************************************************/

int mvx_secure_construct(struct mvx_secure *secure,
			 struct device *dev)
{
	secure->dev = dev;
#ifndef MODULE
	secure->kset = kset_create_and_add("securevideo", NULL, &dev->kobj);
	if (secure->kset == NULL) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to create securevideo kset.");
		return -EINVAL;
	}
#endif
	secure->workqueue = alloc_workqueue("mvx_securevideo",
					    WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (secure->workqueue == NULL) {
#ifndef MODULE
		kset_unregister(secure->kset);
#endif
		return -EINVAL;
	}

	return 0;
}

void mvx_secure_destruct(struct mvx_secure *secure)
{
	destroy_workqueue(secure->workqueue);
#ifndef MODULE
	kset_unregister(secure->kset);
#endif
}

/****************************************************************************
 * Secure firmware
 ****************************************************************************/

/**
 * firmware_store() - Firmware sysfs store function.
 *
 * Store values from firmware descriptor, get the DMA handle and wake up any
 * waiting process.
 */
static ssize_t firmware_store(struct kobject *kobj,
			      struct kobj_attribute *attr,
			      const char *buf,
			      size_t size)
{
	struct mvx_secure_firmware_priv *securefw =
		container_of(kobj, struct mvx_secure_firmware_priv, kobj);
	const struct secure_firmware_desc *desc = (const void *)buf;

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
		      "Loaded secure firmware. fd=%d, l2=0x%llx, major=%u, minor=%u.",
		      desc->fd, desc->l2pages, desc->protocol.major,
		      desc->protocol.minor);

	securefw->fw.l2pages = desc->l2pages;
	securefw->fw.protocol.major = desc->protocol.major;
	securefw->fw.protocol.minor = desc->protocol.minor;
	securefw->fw.dmabuf = dma_buf_get(desc->fd);
	if (IS_ERR_OR_NULL(securefw->fw.dmabuf))
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to get DMA buffer from fd. fd=%d.",
			      desc->fd);

	wake_up_interruptible(&securefw->wait_queue);

	return size;
}

/**
 * secure_firmware_release() - Release secure firmware.
 * kobj:	Pointer to kobject.
 */
static void secure_firmware_release(struct kobject *kobj)
{
	struct mvx_secure_firmware_priv *securefw =
		container_of(kobj, struct mvx_secure_firmware_priv, kobj);

	if (IS_ERR_OR_NULL(securefw->fw.dmabuf) == false)
		dma_buf_put(securefw->fw.dmabuf);

	devm_kfree(securefw->dev, securefw);
}

/**
 * secure_firmware_wait() - Wait for firmware load.
 * @work:	Pointer to work member in mvx_secure_firmware_priv.
 *
 * Worker thread used to wait for a secure firmware load to complete.
 */
static void secure_firmware_wait(struct work_struct *work)
{
	struct mvx_secure_firmware_priv *securefw =
		container_of(work, struct mvx_secure_firmware_priv, work);
	int ret;

	ret = wait_event_interruptible_timeout(securefw->wait_queue,
					       securefw->fw.dmabuf != NULL,
					       msecs_to_jiffies(10000));
	if (ret == 0)
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Firmware load timed out.");

	kobject_del(&securefw->kobj);

	if (securefw->done != NULL)
		securefw->done(&securefw->fw, securefw->done_arg);
}

/**
 * secure_firmware_create() - Create a secure firmware object.
 * @secure:	Pointer to secure context.
 * @name:	Name for secure firmware binary.
 * @ncores:	Number of cores to setup.
 * @arg:	User argument to callback routine.
 * @done:	Firware load callback routine.
 *
 * Return: Valid pointer on success, else ERR_PTR.
 */
static struct mvx_secure_firmware_priv *secure_firmware_create(
	struct mvx_secure *secure,
	const char *name,
	unsigned int ncores,
	void *arg,
	mvx_secure_firmware_done done)
{
	static struct kobj_attribute attr = __ATTR_WO(firmware);
	static struct attribute *attrs[] = {
		&attr.attr,
		NULL
	};
	static const struct attribute_group secure_group = {
        .name = "",
        .attrs = attrs
    };

    static const struct attribute_group *secure_groups[] = {
        &secure_group,
        NULL
    };
	static struct kobj_type secure_ktype = {
		.sysfs_ops     = &kobj_sysfs_ops,
		.release       = secure_firmware_release,
		.default_groups = secure_groups
	};
	struct mvx_secure_firmware_priv *securefw;
	char numcores_env[32];
	char fw_env[140];
	char *env[] = { "TYPE=firmware", numcores_env, fw_env, NULL };
	size_t n;
	int ret;

	n = snprintf(fw_env, sizeof(fw_env), "FIRMWARE=%s.enc", name);
	if (n >= sizeof(fw_env))
		return ERR_PTR(-EINVAL);

	n = snprintf(numcores_env, sizeof(numcores_env), "NUMCORES=%u", ncores);
	if (n >= sizeof(numcores_env))
		return ERR_PTR(-EINVAL);

	/* Allocate and initialize the secure firmware object. */
	securefw = devm_kzalloc(secure->dev, sizeof(*securefw), GFP_KERNEL);
	if (securefw == NULL)
		return ERR_PTR(-ENOMEM);

	securefw->dev = secure->dev;
	securefw->kobj.kset = secure->kset;
	securefw->fw.ncores = ncores;
	securefw->done = done;
	securefw->done_arg = arg;
	init_waitqueue_head(&securefw->wait_queue);

	/* Create kobject that the user space helper can interact with. */
	ret = kobject_init_and_add(&securefw->kobj, &secure_ktype, NULL, "%p",
				   securefw);
	if (ret != 0)
		goto put_kobject;

	/* Notify user space helper about the secure firmware load. */
	ret = kobject_uevent_env(&securefw->kobj, KOBJ_ADD, env);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to send secure firmware uevent. ret=%d.",
			      ret);
		goto put_kobject;
	}

	return securefw;

put_kobject:
	kobject_put(&securefw->kobj);
	devm_kfree(secure->dev, securefw);

	return ERR_PTR(ret);
}

int mvx_secure_request_firmware_nowait(struct mvx_secure *secure,
				       const char *name,
				       unsigned int ncores,
				       void *arg,
				       mvx_secure_firmware_done done)
{
	struct mvx_secure_firmware_priv *securefw;

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
		      "Request secure firmware load nowait. firmware=%s.enc.",
		      name);

	securefw = secure_firmware_create(secure, name, ncores, arg, done);
	if (IS_ERR(securefw))
		return PTR_ERR(securefw);

	INIT_WORK(&securefw->work, secure_firmware_wait);
	queue_work(secure->workqueue, &securefw->work);

	return 0;
}

void mvx_secure_release_firmware(struct mvx_secure_firmware *securefw)
{
	struct mvx_secure_firmware_priv *sfw =
		container_of(securefw, struct mvx_secure_firmware_priv, fw);

	kobject_put(&sfw->kobj);
}

/****************************************************************************
 * Secure memory
 ****************************************************************************/

/**
 * secure_mem_release() - Release the secure memory object.
 */
static void secure_mem_release(struct kobject *kobj)
{
	struct mvx_secure_mem *smem =
		container_of(kobj, struct mvx_secure_mem, kobj);

	devm_kfree(smem->dev, smem);
}

/**
 * memory_store() - Memory sysfs store function.
 *
 * Store values from memory descriptor, get the DMA handle and wake up any
 * waiting process.
 */
static ssize_t memory_store(struct kobject *kobj,
			    struct kobj_attribute *attr,
			    const char *buf,
			    size_t size)
{
	struct mvx_secure_mem *smem =
		container_of(kobj, struct mvx_secure_mem, kobj);
	const int32_t *fd = (const int32_t *)buf;

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
		      "Secure memory allocated. fd=%d.",
		      *fd);

	smem->dmabuf = dma_buf_get(*fd);
	if (IS_ERR_OR_NULL(smem->dmabuf))
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to get DMA buffer.");

	wake_up_interruptible(&smem->wait_queue);

	return size;
}

struct dma_buf *mvx_secure_mem_alloc(struct mvx_secure *secure,
				     size_t size)
{
	static struct kobj_attribute attr = __ATTR_WO(memory);
	static struct attribute *attrs[] = {
		&attr.attr,
		NULL
	};
	static const struct attribute_group secure_mem_group = {
        .name = "",
        .attrs = attrs
    };

    static const struct attribute_group *secure_mem_groups[] = {
        &secure_mem_group,
        NULL
    };
	static struct kobj_type secure_mem_ktype = {
		.release       = secure_mem_release,
		.sysfs_ops     = &kobj_sysfs_ops,
		.default_groups = secure_mem_groups
	};
	struct mvx_secure_mem *smem;
	char size_env[32];
	char *env[] = { "TYPE=memory", size_env, NULL };
	struct dma_buf *dmabuf = ERR_PTR(-EINVAL);
	size_t n;
	int ret;

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
		      "Request secure memory. size=%zu.", size);

	n = snprintf(size_env, sizeof(size_env), "SIZE=%zu", size);
	if (n >= sizeof(size_env))
		return ERR_PTR(-EINVAL);

	smem = devm_kzalloc(secure->dev, sizeof(*smem), GFP_KERNEL);
	if (smem == NULL)
		return ERR_PTR(-ENOMEM);

	smem->dev = secure->dev;
	smem->kobj.kset = secure->kset;
	init_waitqueue_head(&smem->wait_queue);

	/* Create kobject that the user space helper can interact with. */
	ret = kobject_init_and_add(&smem->kobj, &secure_mem_ktype, NULL, "%p",
				   &smem->kobj);
	if (ret != 0)
		goto put_kobject;

	/* Notify user space helper about the secure firmware load. */
	ret = kobject_uevent_env(&smem->kobj, KOBJ_ADD, env);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to send secure memory uevent. ret=%d.",
			      ret);
		goto put_kobject;
	}

	ret = wait_event_interruptible_timeout(smem->wait_queue,
					       smem->dmabuf != NULL,
					       msecs_to_jiffies(1000));
	if (ret == 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Secure memory allocation timed out.");
		goto put_kobject;
	}

	dmabuf = smem->dmabuf;

put_kobject:
	kobject_put(&smem->kobj);

	return dmabuf;
}

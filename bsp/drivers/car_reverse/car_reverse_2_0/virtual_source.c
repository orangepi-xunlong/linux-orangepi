/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * virtual_source for fast car reverse
 *
 * Copyright (C) 2015-2023 AllwinnerTech, Inc.
 *
 * Authors:  Huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
  */


#include "video_source.h"
#include "buffer_pool.h"

#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/freezer.h>
#include <linux/moduleparam.h>

#define SOURCE_MAX 10
#define VIR_SRC_DRIVER_NAME "virtual_source"

struct platform_device *pdev;
int colorbar_w;
int colorbar_h;

struct virtual_source {
	struct list_head active;
	struct list_head done;
	spinlock_t buf_lock;
	spinlock_t update_lock;
	struct mutex stream_lock;
	struct delayed_work update_buffer;
	int id;
};

struct virtual_source vir_source[SOURCE_MAX];

struct car_reverse_video_source video_source_virtual;

int virtual_callback(void)
{
	video_source_virtual.car_reverse_callback(0);

	return 0;
}

int virtual_open(int video_id)
{
	return 0;
}

int virtual_close(int video_id)
{
	return 0;
}

int virtual_info(void)
{
	return 0;
}

void virtual_buffer_dump_active(int video_id)
{
	struct virtual_source *source = &vir_source[video_id];
	struct video_source_buffer *vsbuf;
	unsigned long flags;

	spin_lock_irqsave(&source->buf_lock, flags);
	if (!list_empty(&source->active)) {
		list_for_each_entry(vsbuf, &source->active, list) {
			CAR_REVERSE_INFO("dmabuf_fd:%d, dma_address:0x%llx\n", \
					 vsbuf->dmabuf_fd, vsbuf->dma_address);
		}
	} else {
		CAR_REVERSE_INFO("empty!");
	}
	spin_unlock_irqrestore(&source->buf_lock, flags);
}

void virtual_buffer_dump_done(int video_id)
{
	struct virtual_source *source = &vir_source[video_id];
	struct video_source_buffer *vsbuf;
	unsigned long flags;

	spin_lock_irqsave(&source->buf_lock, flags);
	if (!list_empty(&source->done)) {
		list_for_each_entry(vsbuf, &source->done, list) {
			CAR_REVERSE_INFO("dmabuf_fd:%d, dma_address:0x%llx\n", \
					 vsbuf->dmabuf_fd, vsbuf->dma_address);
		}
	} else {
		CAR_REVERSE_INFO("virtual_buffer_dump_done empty!");
	}
	spin_unlock_irqrestore(&source->buf_lock, flags);
}

int virtual_streamon(int video_id)
{
	struct virtual_source *source = &vir_source[video_id];
	struct video_source_buffer *vsbuf;
	int ret;
	unsigned long flags;

	source->id = video_id;

#if defined VIRTUAL_BUF_DUMP
	CAR_REVERSE_INFO("line:%d\n", __LINE__);
	virtual_buffer_dump_active(video_id);
	virtual_buffer_dump_done(video_id);
#endif

	mutex_lock(&source->stream_lock);
	spin_lock_irqsave(&source->buf_lock, flags);
	if (!list_empty(&source->active)) {
		vsbuf = list_entry(source->active.next, struct video_source_buffer, list);
	} else {
		CAR_REVERSE_ERR("virtual streamon, but no buffer now.\n");
		ret = -1;
		goto streamon_unlock;
	}

	memset(vsbuf->vir_address, 0x0, 1280 * 800);
	memset(vsbuf->vir_address + (1280 * 800), 0x0, 1280 * 800);

	list_del(&vsbuf->list);
	list_add_tail(&vsbuf->list, &source->done);

streamon_unlock:
	spin_unlock_irqrestore(&source->buf_lock, flags);
	mutex_unlock(&source->stream_lock);
	virtual_callback();

	schedule_delayed_work(&source->update_buffer, msecs_to_jiffies(50));

	return 0;

}

int virtual_streamoff(int video_id)
{
	struct virtual_source *source = &vir_source[video_id];

	cancel_delayed_work_sync(&source->update_buffer);
	return 0;
}

int virtual_qbuffer(int video_id, struct video_source_buffer *vsbuf)
{
	struct virtual_source *source = &vir_source[video_id];
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	unsigned long flags = 0, flags1 = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	struct dma_buf_map map;
	int ret;
#endif

	if (vsbuf == NULL) {
		CAR_REVERSE_ERR("virtual_qbuffer failed, vsbuf is null!\n");
		return -1;
	}

	spin_lock_irqsave(&source->update_lock, flags1);

#if defined VIRTUAL_BUF_DUMP
	CAR_REVERSE_INFO("line:%d\n", __LINE__);
	virtual_buffer_dump_active(video_id);
#endif

	spin_lock_irqsave(&source->buf_lock, flags);

	vsbuf->dmabuf = dma_buf_get(vsbuf->dmabuf_fd);

	attachment = dma_buf_attach(vsbuf->dmabuf, &pdev->dev);
	if (IS_ERR(attachment)) {
		CAR_REVERSE_ERR("dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		CAR_REVERSE_ERR("dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	vsbuf->dma_address = sg_dma_address(sgt->sgl);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = dma_buf_vmap(vsbuf->dmabuf, &map);
	if (ret) {
		CAR_REVERSE_ERR("dma_buf_vmap failed!!\n");
		goto err_buf_unmap;
	}
	vsbuf->vir_address = map.vaddr;
#else
	vsbuf->vir_address = dma_buf_vmap(vsbuf->dmabuf);
#endif


	vsbuf->attachment = attachment;
	vsbuf->sgt = sgt;

	list_add_tail(&vsbuf->list, &source->active);
	vsbuf->state = VB2_BUF_STATE_QUEUED;

	spin_unlock_irqrestore(&source->buf_lock, flags);

#if defined VIRTUAL_BUF_DUMP
	virtual_buffer_dump_active(video_id);
#endif
	spin_unlock_irqrestore(&source->update_lock, flags1);

	return 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
err_buf_unmap:
	dma_buf_unmap_attachment(attachment, sgt, DMA_FROM_DEVICE);
#endif
err_buf_detach:
	dma_buf_detach(vsbuf->dmabuf, attachment);
err_buf_put:
	dma_buf_put(vsbuf->dmabuf);
	return -ENOMEM;
}

int virtual_dqbuffer(int video_id, struct video_source_buffer **vsbuf)
{
	struct virtual_source *source = &vir_source[video_id];
	struct video_source_buffer *tmp = NULL;
	int ret = 0;
	unsigned long flags = 0, flags1 = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	struct dma_buf_map map;
#endif

	spin_lock_irqsave(&source->update_lock, flags1);
#if defined VIRTUAL_BUF_DUMP
	CAR_REVERSE_INFO("line:%d\n", __LINE__);
	virtual_buffer_dump_done(video_id);
#endif
	spin_lock_irqsave(&source->buf_lock, flags);
	if (!list_empty(&source->done)) {
		tmp = list_first_entry(&source->done, struct video_source_buffer, list);
		list_del(&tmp->list);
		tmp->state = VB2_BUF_STATE_DEQUEUED;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		map.vaddr = tmp->vir_address;
		map.is_iomem = false;

		dma_buf_vunmap(tmp->dmabuf, &map);
#else
		dma_buf_vunmap(tmp->dmabuf, tmp->vir_address);
#endif

		dma_buf_unmap_attachment(tmp->attachment, tmp->sgt, DMA_FROM_DEVICE);
		dma_buf_detach(tmp->dmabuf, tmp->attachment);
		dma_buf_put(tmp->dmabuf);
		*vsbuf = tmp;
	} else {
		CAR_REVERSE_ERR("virtual_dqbuffer failed, no vsbuf in ready list!\n");
		ret = -1;
	}

	spin_unlock_irqrestore(&source->buf_lock, flags);
#if defined VIRTUAL_BUF_DUMP
	virtual_buffer_dump_done(video_id);
#endif
	spin_unlock_irqrestore(&source->update_lock, flags1);

	return ret;
}

int virtual_force_reset_buffer(int video_id)
{
	struct video_source_buffer *vsbuf;
	struct virtual_source *source = &vir_source[video_id];
	int ret = 0;
	unsigned long flags = 0, flags1 = 0;

	spin_lock_irqsave(&source->update_lock, flags);
	spin_lock_irqsave(&source->buf_lock, flags);
	while (!list_empty(&source->active)) {
		vsbuf =
		    list_first_entry(&source->active, struct video_source_buffer, list);
		list_del(&vsbuf->list);
		list_add(&vsbuf->list, &source->done);
	}

	spin_unlock_irqrestore(&source->buf_lock, flags);
	spin_unlock_irqrestore(&source->update_lock, flags1);

	return ret;
}

int virtual_set_format(int video_id, struct v4l2_format *fmt)
{
	colorbar_w = fmt->fmt.pix.width;
	colorbar_h = fmt->fmt.pix.height;
	return 0;
}

int virtual_get_format(int video_id, struct v4l2_format *fmt)
{
	fmt->fmt.pix.width = 1280;
	fmt->fmt.pix.height = 720;

	return 0;
}


struct car_reverse_video_source video_source_virtual = {
	.type = "virtual",
	.video_open = virtual_open,
	.video_close = virtual_close,
	.video_info = virtual_info,
	.video_streamon = virtual_streamon,
	.video_streamoff = virtual_streamoff,
	.queue_buffer = virtual_qbuffer,
	.dequeue_buffer = virtual_dqbuffer,
	.video_set_format = virtual_set_format,
	.video_get_format = virtual_get_format,
	.force_reset_buffer = virtual_force_reset_buffer,
};


static void virtual_update_buffer(struct work_struct *work)
{
	struct virtual_source *source;
	struct video_source_buffer *vsbuf = NULL;
	unsigned long flags = 0, flags1 = 0;
	static int colorbar;
	static int colorbar1;

	if (colorbar >= 255)
		colorbar = 0;

	if (colorbar1 >= 255)
		colorbar1 = 0;

	source = container_of(work, struct virtual_source, update_buffer.work);

	spin_lock_irqsave(&source->update_lock, flags1);
#if defined VIRTUAL_BUF_DUMP
	CAR_REVERSE_INFO("line:%d\n", __LINE__);
	virtual_buffer_dump_active(source->id);
	virtual_buffer_dump_done(source->id);
#endif

	spin_lock_irqsave(&source->buf_lock, flags);
	if (!list_empty(&source->active))
		vsbuf = list_entry(source->active.next, struct video_source_buffer, list);
	else {
		CAR_REVERSE_ERR("virtual source update buffer, but no buffer now.\n");
		goto update_unlock;
	}

	memset(vsbuf->vir_address, colorbar, colorbar_w * colorbar_h);
	memset(vsbuf->vir_address + (colorbar_w * colorbar_h), colorbar1, colorbar_w * colorbar_h);

	colorbar += 30;
	colorbar1 += 10;

	list_del(&vsbuf->list);
	list_add_tail(&vsbuf->list, &source->done);

update_unlock:
	spin_unlock_irqrestore(&source->buf_lock, flags);
	spin_unlock_irqrestore(&source->update_lock, flags1);
	virtual_callback();
	schedule_delayed_work(&source->update_buffer, msecs_to_jiffies(50));
}


static int vir_src_probe(struct platform_device *pdev)
{
	int i;
	CAR_REVERSE_INFO("virtual_source_probe\n");

	for (i = 0; i < SOURCE_MAX; i++) {
		INIT_LIST_HEAD(&vir_source[i].active);
		INIT_LIST_HEAD(&vir_source[i].done);
		mutex_init(&vir_source[i].stream_lock);
		spin_lock_init(&vir_source[i].buf_lock);
		spin_lock_init(&vir_source[i].update_lock);
		INIT_DELAYED_WORK(&vir_source[i].update_buffer, virtual_update_buffer);
	}
	car_reverse_video_source_register(&video_source_virtual);

	return 0;
}


static int vir_src_remove(struct platform_device *pdev)
{

	car_reverse_video_source_unregister(&video_source_virtual);

	return 0;
}


static int vir_src_suspend(struct device *dev)
{
	return 0;
}

static int vir_src_resume(struct device *dev)
{
	return 0;
}


static const struct dev_pm_ops vir_src_pm_ops = {
	.suspend = vir_src_suspend,
	.resume = vir_src_resume,
};

static const struct of_device_id vir_src_match[] = {
	{.compatible = "allwinner,virtual-video-source",},
	{},
};

static struct platform_driver virtual_source_driver = {
	.probe = vir_src_probe,
	.remove = vir_src_remove,
	.driver = {
	   .owner = THIS_MODULE,
	   .name = VIR_SRC_DRIVER_NAME,
	},
};

__init int virtual_source_init(void)
{
	int ret;

	CAR_REVERSE_INFO("virtual_source_init\n");

	ret = platform_driver_register(&virtual_source_driver);
	if (!ret) {
		pdev = platform_device_register_simple(VIR_SRC_DRIVER_NAME, -1, NULL, 0);
		if (IS_ERR(pdev)) {
			CAR_REVERSE_ERR("Virtual source device allocation failed\n");
			ret = -1;
			goto err_register_device;
		}
	} else {
		CAR_REVERSE_ERR("Virtual source driver register failed\n");
	}

	return ret;

err_register_device:
	platform_driver_unregister(&virtual_source_driver);

	return ret;
}

__exit void virtual_source_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&virtual_source_driver);
}

subsys_initcall(virtual_source_init);
module_exit(virtual_source_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<huangyongxing@allwinnertech.com>");
MODULE_DESCRIPTION("Virtual video source for debug");
MODULE_VERSION("1.0.0");

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * drv_dec.c
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "drv_dec.h"
#include "dec.h"
#include "dec_fence.h"
#include "video_buffer.h"
#include "afbd_wb.h"
/*#include "../drivers/misc/sunxi-tvutils/sunxi_tvtop_clk.h"*/
#include "frame_manager/dec_frame_manager.h"

#define CREATE_TRACE_POINTS
#include "dec_trace.h"

struct dec_drv_info g_drv_info;

static const struct of_device_id sunxi_dec_match[] = {
	{.compatible = "allwinner,sunxi-dec",},
	{},
};

int dec_open(struct inode *inode, struct file *file)
{
	atomic_inc(&g_drv_info.driver_ref_count);
	return 0;
}

int dec_release(struct inode *inode, struct file *file)
{
	if (!atomic_dec_and_test(&g_drv_info.driver_ref_count))
		return 0;

	return 0;
}

long dec_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	uint64_t karg[4];
	uint64_t ubuffer[4];
	int ret = -1;

	ret = copy_from_user((void *)karg, (void __user *)arg,
			4 * sizeof(uint64_t));
	if (ret) {
		dev_err(g_drv_info.p_device, "copy from user fail!\n");
		goto OUT;
	}
	ubuffer[0] = *(uint64_t *)karg;
	ubuffer[1] = *(uint64_t *)(karg + 1);
	ubuffer[2] = *(uint64_t *)(karg + 2);
	ubuffer[3] = *(uint64_t *)(karg + 3);

	switch (cmd) {
	case DEC_FRMAE_SUBMIT: {
		int fence = -1;
		struct dec_frame_config cfg;
		int repeat_count = ubuffer[2];

		if (copy_from_user(&cfg, u64_to_user_ptr(ubuffer[0]),
				   sizeof(struct dec_frame_config))) {
			dev_err(g_drv_info.p_device, "copy_from_user fail\n");
			return -EFAULT;
		}

		DEC_TRACE_BEGIN("ioctl:submit");
		ret = g_drv_info.p_dec_device->frame_submit(
			g_drv_info.p_dec_device, &cfg, &fence, repeat_count);

		if (copy_to_user(u64_to_user_ptr(ubuffer[1]), &fence, sizeof(fence))) {
			dev_err(g_drv_info.p_device, "release fence copy_to_user fail\n");
		}
		DEC_TRACE_END("");
		break;
	}
	case DEC_INTERLACE_SETUP: {
		struct dec_frame_config cfg[2];
		if (copy_from_user(&cfg[0], u64_to_user_ptr(ubuffer[0]),
				   sizeof(struct dec_frame_config) * 2)) {
			dev_err(g_drv_info.p_device, "copy_from_user fail\n");
			return -EFAULT;
		}

		DEC_TRACE_BEGIN("ioctl:interlace_setup");
		ret = g_drv_info.p_dec_device->interlace_setup(g_drv_info.p_dec_device, &cfg[0]);
		DEC_TRACE_END("");
		break;
	}
	case DEC_STREAM_STOP: {
		ret = g_drv_info.p_dec_device->stop_video_stream(g_drv_info.p_dec_device);
		break;
	}
	case DEC_BYPASS_EN: {
		ret = g_drv_info.p_dec_device->bypass_config(g_drv_info.p_dec_device, ubuffer[0]);
		break;
	}
	case DEC_GET_VSYNC_TIMESTAMP: {
		struct dec_vsync_timestamp ts;
		ret = g_drv_info.p_dec_device->vsync_timestamp_get(g_drv_info.p_dec_device, &ts);
		if (ret == 0) {
			if (copy_to_user(u64_to_user_ptr(ubuffer[0]), &ts, sizeof(ts))) {
				dev_err(g_drv_info.p_device, "vsync timestamp copy_to_user fail\n");
				return -EFAULT;
			}
		}
		break;
	}
	case DEC_ENABLE: {
		if (ubuffer[0])
			pm_runtime_get_sync(&g_drv_info.p_plat_dev->dev);
		else
			pm_runtime_put_sync(&g_drv_info.p_plat_dev->dev);

		ret = 0;
		break;
	}

	case DEC_QUERY_VBI_BUFFER: {
		struct dec_video_buffer_data data;
		ret = dec_vbi_buffer_query(
				&data.phy_address, &data.device_address, &data.size);
		if (ret == 0) {
			if (copy_to_user(u64_to_user_ptr(ubuffer[0]), &data, sizeof(data))) {
				dev_err(g_drv_info.p_device, "DEC_QUERY_VBI_BUFFER copy_to_user fail\n");
				return -EFAULT;
			}
		}
		break;
	}

	case DEC_MAP_VIDEO_BUFFER: {
		struct dec_video_buffer_data data;
		dma_addr_t device_addr = 0;
		if (copy_from_user(&data, u64_to_user_ptr(ubuffer[0]), sizeof(data))) {
			dev_err(g_drv_info.p_device, "copy_from_user fail\n");
			return -EFAULT;
		}
		ret = video_buffer_map(
				data.phy_address, data.size, &device_addr);
		if (ret == 0) {
			data.device_address = device_addr;
			if (copy_to_user(u64_to_user_ptr(ubuffer[0]), &data, sizeof(data))) {
				dev_err(g_drv_info.p_device, "copy_to_user fail\n");
				return -EFAULT;
			}
		}
		break;
	}

	case DEC_UNMAP_VIDEO_BUFFER: {
		struct dec_video_buffer_data data;
		if (copy_from_user(&data, u64_to_user_ptr(ubuffer[0]), sizeof(data))) {
			dev_err(g_drv_info.p_device, "copy_from_user fail\n");
			return -EFAULT;
		}
		video_buffer_unmap(data.device_address);
		break;
	}

	case DEC_ALLOC_VIDEO_BUFFER: {
		struct dec_video_buffer_data data;
		dma_addr_t device_addr = 0;
		if (copy_from_user(&data, u64_to_user_ptr(ubuffer[0]), sizeof(data))) {
			dev_err(g_drv_info.p_device, "copy_from_user fail\n");
			return -EFAULT;
		}
		ret = video_framebuffer_allocate(data.size, &device_addr);
		if (ret == 0) {
			data.device_address = device_addr;
			if (copy_to_user(u64_to_user_ptr(ubuffer[0]), &data, sizeof(data))) {
				dev_err(g_drv_info.p_device, "copy_to_user fail\n");
				return -EFAULT;
			}
		}
		break;
	}

	case DEC_FREE_VIDEO_BUFFER: {
		struct dec_video_buffer_data data;
		if (copy_from_user(&data, u64_to_user_ptr(ubuffer[0]), sizeof(data))) {
			dev_err(g_drv_info.p_device, "copy_from_user fail\n");
			return -EFAULT;
		}
		video_framebuffer_free(data.device_address);
		break;
	}

	case DEC_SET_OVERSCAN: {
		struct overscan_info *info = kzalloc(sizeof(*info), GFP_KERNEL);
		if (IS_ERR_OR_NULL(info)) {
			return -ENOMEM;
		}
		if (copy_from_user(info, u64_to_user_ptr(ubuffer[0]), sizeof(*info))) {
			dev_err(g_drv_info.p_device, "overscan_info copy_from_user fail\n");
			kfree(info);
			return -EFAULT;
		}
		ret = g_drv_info.p_dec_device->set_overscan(g_drv_info.p_dec_device, info);
		break;
	}
	case DEC_START_WRITEBACK: {
		struct afbd_wb_info wb_info;
		struct afbd_wb_dev_t *pdev = NULL;
		if (copy_from_user(&wb_info, (void *)arg,
				   sizeof(struct afbd_wb_info))) {
			dev_err(g_drv_info.p_device, "afbd_wb_info fail!\n");
			ret = -EFAULT;
			break;
		}
		pdev = get_afbd_wb_inst();
		if (pdev) {
			ret = pdev->afbd_wb_commit(pdev, &wb_info);
			if (!ret) {
				pdev->afbd_set_wb_state(pdev, AFBD_WB_COMMIT);
			}
		} else {
			ret = -EFAULT;
			dev_err(g_drv_info.p_device, "get_afbd_wb_inst fail!\n");
		}
		break;
	}
	case DEC_WAIT_WB_FINISH: {
		struct afbd_wb_dev_t *pdev = NULL;
		pdev = get_afbd_wb_inst();
		if (pdev) {
			if (pdev->get_wb_state(pdev) == AFBD_WB_IDLE)
				ret = 1;
			else
				ret = 0;
		} else {
			dev_err(g_drv_info.p_device,
				"get_afbd_wb_inst fail!\n");
		}
	} break;

	default:
		dev_err(g_drv_info.p_device, "Unkown cmd :0x%x\n", cmd);
		break;
	}

OUT:
	return ret;
}

static unsigned int dec_poll(struct file *filp, poll_table *wait)
{
	struct dec_device *decdev = g_drv_info.p_dec_device;
	return decdev->poll(decdev, filp, wait);
}

static void dec_shutdown(struct platform_device *pdev)
{

}

static u64 dec_dmamask = DMA_BIT_MASK(32);
static int dec_probe(struct platform_device *pdev)
{
	int ret = -1;
	set_repeat_count_for_signal_changed(5);  // set default repeat count.

	pdev->dev.dma_mask = &dec_dmamask;

	g_drv_info.p_plat_dev = pdev;

	g_drv_info.p_device = device_create(g_drv_info.p_class, &pdev->dev,
						g_drv_info.dev_id, NULL, "decd");

	if (IS_ERR_OR_NULL(g_drv_info.p_device)) {
		dev_err(&pdev->dev, "device_create fail\n");
		goto OUT;
	}

	g_drv_info.p_dec_device = dec_init(g_drv_info.p_device);
	if (!g_drv_info.p_dec_device) {
		dev_err(&pdev->dev, "dec init fail!\n");
		goto OUT;
	}

	video_buffer_init(&pdev->dev);

	ret = 0;
	g_drv_info.inited = true;

	pm_runtime_enable(&pdev->dev);

	/*if (sunxi_tvtop_client_register(&pdev->dev) != 0)*/
		/*dev_err(&pdev->dev, "failed to register as tvtop client!\n");*/

OUT:
	return ret;
}

static int dec_remove(struct platform_device *pdev)
{
	int ret = -1;

	dec_exit(g_drv_info.p_dec_device);

	return ret;
}

static int dec_runtime_suspend(struct device *dev)
{
	int ret = g_drv_info.p_dec_device->suspend(
			g_drv_info.p_dec_device);

	dev_info(dev, "decoder display runtime suspend: %d\n", ret);
	return 0;
}

static int dec_runtime_resume(struct device *dev)
{
	int ret = g_drv_info.p_dec_device->resume(
			g_drv_info.p_dec_device);

	dev_info(dev, "decoder display runtime resume: %d\n", ret);
	return 0;
}

static const struct dev_pm_ops dec_runtime_pm_ops = {
	SET_RUNTIME_PM_OPS(dec_runtime_suspend, dec_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend, pm_runtime_force_resume)
};

static int __init dec_module_init(void)
{
	int ret = -1;

	memset(&g_drv_info, 0, sizeof(struct dec_drv_info));

	g_drv_info.p_dec_fops = kmalloc(sizeof(struct file_operations),
					GFP_KERNEL | __GFP_ZERO);
	if (!g_drv_info.p_dec_fops) {
		pr_err("Malloc p_dec_fops fail!\n");
		goto OUT;
	}

	ret = alloc_chrdev_region(&g_drv_info.dev_id, 0, 1, "decd");
	if (ret) {
		pr_err("alloc_chrdev_region fail:%d!\n", ret);
		goto FREE_FOPS;
	}

	g_drv_info.p_cdev = cdev_alloc();
	if (!g_drv_info.p_cdev) {
		pr_err("alloc_chrdev_region fail:%d!\n", ret);
		goto FREE_FOPS;
	}
	g_drv_info.p_dec_fops->owner = THIS_MODULE;
	g_drv_info.p_dec_fops->open = dec_open;
	g_drv_info.p_dec_fops->release = dec_release;
	g_drv_info.p_dec_fops->poll = dec_poll;
	/*g_drv_info.p_dec_fops->write = dec_write;*/
	/*g_drv_info.p_dec_fops->read = dec_read;*/
	g_drv_info.p_dec_fops->unlocked_ioctl = dec_ioctl;
#ifdef CONFIG_COMPAT
	g_drv_info.p_dec_fops->compat_ioctl = dec_ioctl;
#endif
	/*g_drv_info.p_dec_fops->mmap = dec_mmap;*/

	cdev_init(g_drv_info.p_cdev, g_drv_info.p_dec_fops);
	g_drv_info.p_cdev->owner = THIS_MODULE;
	ret = cdev_add(g_drv_info.p_cdev, g_drv_info.dev_id, 1);
	if (ret) {
		pr_err("cdev add fail:%d\n", ret);
		goto FREE_FOPS;
	}

	g_drv_info.p_class = class_create(THIS_MODULE, "decd");
	if (IS_ERR_OR_NULL(g_drv_info.p_class)) {
		pr_err("class_create fail\n");
		goto CDEV_DEL;
	}


	g_drv_info.p_dec_driver = kmalloc(sizeof(struct platform_driver),
					  GFP_KERNEL | __GFP_ZERO);
	if (!g_drv_info.p_dec_driver) {
		pr_err("Fail to kmalloc platform_driver!\n");
		goto CLASS_DEL;
	}
	g_drv_info.p_dec_driver->probe = dec_probe;
	g_drv_info.p_dec_driver->remove = dec_remove;
	g_drv_info.p_dec_driver->shutdown = dec_shutdown;
	g_drv_info.p_dec_driver->driver.name = "decd";
	g_drv_info.p_dec_driver->driver.owner = THIS_MODULE;
	g_drv_info.p_dec_driver->driver.of_match_table = sunxi_dec_match;
	g_drv_info.p_dec_driver->driver.pm = &dec_runtime_pm_ops;

	ret = platform_driver_register(g_drv_info.p_dec_driver);
	if (ret) {
		pr_err("platform_driver_register fail:%d\n", ret);
		goto FREE_PLAT;
	}

OUT:
	return ret;
FREE_PLAT:
	kfree(g_drv_info.p_dec_driver);
CLASS_DEL:
	class_destroy(g_drv_info.p_class);
CDEV_DEL:
	cdev_del(g_drv_info.p_cdev);
FREE_FOPS:
	kfree(g_drv_info.p_dec_fops);
	memset(&g_drv_info, 0, sizeof(struct dec_drv_info));
	return ret;
}

static void __exit dec_module_exit(void)
{

	platform_driver_unregister(g_drv_info.p_dec_driver);

	device_destroy(g_drv_info.p_class, g_drv_info.dev_id);

	class_destroy(g_drv_info.p_class);

	cdev_del(g_drv_info.p_cdev);

	kfree(g_drv_info.p_dec_fops);

	kfree(g_drv_info.p_dec_driver);

	memset(&g_drv_info, 0, sizeof(struct dec_drv_info));
}

module_init(dec_module_init);
module_exit(dec_module_exit);

MODULE_AUTHOR("sunxi");
MODULE_DESCRIPTION("tv decd driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dec");
MODULE_VERSION("1.0.0");

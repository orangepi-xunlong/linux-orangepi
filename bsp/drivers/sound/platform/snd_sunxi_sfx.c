// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-sfx"
#include "snd_sunxi_log.h"
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>

#include "snd_sunxi_sfx.h"
#include "snd_sunxi_adapter.h"

struct snd_sunxi_sfx {
	dev_t sfx_dev;
	struct cdev sfx_cdev;
	struct class *sfx_class;
	struct device *sfx_device;
	struct file_operations sfx_ops;
	char *sfx_dev_name;
	char *sfx_class_name;
	char *sfx_device_name;

	unsigned long phys_addr;
	unsigned long phys_addr_size;
};

static struct snd_sunxi_sfx suxni_sfx = {
	.sfx_dev_name = "snd_sfx_mgmt_dev",
	.sfx_class_name = "snd_sfx_mgmt",
	.sfx_device_name = "sfx_mgmt",
	.phys_addr = SUNXI_AUDIO_SFX_REG,
	.phys_addr_size = SUNXI_AUDIO_SFX_REG_SIZE,
};

static int snd_sunxi_hw_effect_open(struct inode *inode, struct file *filp)
{
	filp->private_data = (void *)container_of(inode->i_cdev, struct snd_sunxi_sfx, sfx_cdev);

	return 0;
}

static int snd_sunxi_hw_effect_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct snd_sunxi_sfx *sfx = (struct snd_sunxi_sfx *)filp->private_data;
	int ret;
	unsigned long temp_pfn;
	unsigned long vm_size = vma->vm_end - vma->vm_start;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(sfx)) {
		SND_LOG_ERR("snd_sunxi_sfx is NULL\n");
		return -1;
	}

	if (vm_size > ((sfx->phys_addr_size / PAGE_SIZE + 1) * PAGE_SIZE)) {
		SND_LOG_ERR("mmap vm_size is too long\n");
		return -1;
	}

	sunxi_adpt_vm_flags_set(vma, VM_IO);
	temp_pfn = sfx->phys_addr >> PAGE_SHIFT;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	ret = io_remap_pfn_range(vma, vma->vm_start, temp_pfn,
				 vm_size, vma->vm_page_prot);
	if (ret) {
		SND_LOG_ERR("io_remap_pfn failed\n");
		return -1;
	}

	return 0;
}

int snd_sunxi_hw_effect_init(struct snd_sunxi_sfx *sfx)
{
	int ret;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(sfx)) {
		SND_LOG_ERR("snd_sunxi_sfx is NULL\n");
		return -1;
	}
	if (IS_ERR_OR_NULL(sfx->sfx_dev_name)
	    || IS_ERR_OR_NULL(sfx->sfx_class_name)
	    || IS_ERR_OR_NULL(sfx->sfx_device_name)) {
		SND_LOG_ERR("snd_sunxi_sfx name member is NULL\n");
		return -1;
	}

	ret = alloc_chrdev_region(&sfx->sfx_dev, 0, 1, sfx->sfx_dev_name);
	if (ret) {
		SND_LOG_ERR("alloc_chrdev_region failed\n");
		goto err_alloc_chrdev;
	}
	SND_LOG_DEBUG("sfx major = %u, sfx minor = %u\n",
		      MAJOR(sfx->sfx_dev), MINOR(sfx->sfx_dev));

	sfx->sfx_ops.open = snd_sunxi_hw_effect_open;
	sfx->sfx_ops.mmap = snd_sunxi_hw_effect_mmap;
	cdev_init(&sfx->sfx_cdev, &sfx->sfx_ops);
	ret = cdev_add(&sfx->sfx_cdev, sfx->sfx_dev, 1);
	if (ret) {
		SND_LOG_ERR("cdev_add failed\n");
		goto err_cdev_add;
	}

	sfx->sfx_class = sunxi_adpt_class_create(THIS_MODULE, sfx->sfx_class_name);
	if (IS_ERR_OR_NULL(sfx->sfx_class)) {
		SND_LOG_ERR("class_create failed\n");
		goto err_class_create;
	}

	sfx->sfx_device = device_create(sfx->sfx_class, NULL,
					sfx->sfx_dev, NULL,
					sfx->sfx_device_name);
	if (IS_ERR_OR_NULL(sfx->sfx_device)) {
		SND_LOG_ERR("device_create failed\n");
		goto err_device_create;
	}

	return 0;

err_device_create:
	class_destroy(sfx->sfx_class);
err_class_create:
	cdev_del(&sfx->sfx_cdev);
err_cdev_add:
	unregister_chrdev_region(sfx->sfx_dev, 1);
err_alloc_chrdev:
	return -1;
}

void snd_sunxi_hw_effect_exit(struct snd_sunxi_sfx *sfx)
{
	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(sfx)) {
		SND_LOG_ERR("snd_sunxi_sfx is NULL\n");
		return;
	}

	if (!IS_ERR_OR_NULL(sfx->sfx_device))
		device_destroy(sfx->sfx_class, sfx->sfx_dev);
	if (!IS_ERR_OR_NULL(sfx->sfx_class))
		class_destroy(sfx->sfx_class);
	cdev_del(&sfx->sfx_cdev);
	unregister_chrdev_region(sfx->sfx_dev, 1);
}

int __init sunxi_sfx_dev_init(void)
{
	return snd_sunxi_hw_effect_init(&suxni_sfx);
}

void __exit sunxi_sfx_dev_exit(void)
{
	snd_sunxi_hw_effect_exit(&suxni_sfx);
}

module_init(sunxi_sfx_dev_init);
module_exit(sunxi_sfx_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.1");
MODULE_DESCRIPTION("sunxi soundcard of sound effects");

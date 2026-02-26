/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/memblock.h>
#include <linux/version.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/libfdt.h>

#include "aw_hdmi_define.h"
#include "aw_hdmi_log.h"

#define LOG_BUFFER_SIZE		(CONFIG_AW_HDMI2_LOG_BUFFER_SIZE)

static char *buffer;
static u32 buffer_max_size;
static u32 end;
static u32 override;
static struct mutex lock_hdmi_log;
static bool hdmi_log_enable = true;

void hdmi_log(const char *fmt, ...)
{
	char tmp_buffer[128] = {0};
	va_list args;
	u32 len = 0;
	char *p = NULL;

	if (!buffer || !buffer_max_size)
		return ;

	if (!hdmi_log_enable)
		return ;

	va_start(args, fmt);
	vsnprintf(tmp_buffer, sizeof(tmp_buffer), fmt, args);
	va_end(args);

	len = strlen(tmp_buffer);

	mutex_lock(&lock_hdmi_log);

	if (buffer_max_size - end >= len) {
		memcpy(buffer + end, tmp_buffer, len);
		end += len;
	} else {
		if (len < buffer_max_size) {
			p = tmp_buffer;
		} else {
			/* If the size of tmp_buffer is too large,
			 * only copy the last <buffer_max_size> bytes.
			 */
			p = tmp_buffer + (len - buffer_max_size);
			len = buffer_max_size;
		}

		memcpy(buffer + end, p, buffer_max_size - end);

		memcpy(buffer, p + (buffer_max_size - end),
				len - (buffer_max_size - end));

		end = len - (buffer_max_size - end);
		if (!override)
			override = 1;
	}

	mutex_unlock(&lock_hdmi_log);
}

static void *_aw_hdmi_log_map(unsigned long phys_addr, unsigned long size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr = NULL;
	int i;

	if (!pages)
		return NULL;

	for (i = 0, tmp = pages; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = PAGE_KERNEL;
	vaddr = vmap(pages, npages, VM_MAP, pgprot);

	vfree(pages);
	return vaddr;
}

static void _aw_hdmi_log_unmap(void *vaddr)
{
	vunmap(vaddr);
}

static int _aw_hdmi_log_get_uboot_resource(struct device_node *of_node)
{
	int count = 0;
	u64 array[4] = {0};
	void *vaddr = NULL;
	uintptr_t paddr = 0;
	unsigned long paddr_offset = 0;
	u64 hdmi_reserve_base = 0;
	u64 hdmi_reserve_size = 0;

	count = of_property_read_variable_u64_array(of_node, "hdmi_log_mem",
			array, 0, 4);
	if (count <= 0) {
		hdmi_wrn("read hdmi_log_mem array failed!\n");
		return -1;
	}
	if (count != 4) {
		hdmi_err("%s: array count error! %d\n", __func__, count);
		return -1;
	}

	hdmi_reserve_base = array[0];
	hdmi_reserve_size = array[1];
	hdmi_inf("reserve addr: %llx size: %llx\n", hdmi_reserve_base, hdmi_reserve_size);
	end = array[2];
	override = array[3];
	hdmi_inf("hdmi_log end %d override %d\n", end, override);

	paddr = hdmi_reserve_base;
	paddr_offset = (unsigned long)paddr + PAGE_SIZE -
		PAGE_ALIGN((unsigned long)paddr + 1);
	vaddr = _aw_hdmi_log_map(hdmi_reserve_base - paddr_offset,
			hdmi_reserve_size + paddr_offset);
	if (vaddr == NULL) {
		hdmi_err("%s: hdmi_log_map error!\n", __func__);
		return -1;
	}

	buffer = kzalloc(hdmi_reserve_size, GFP_KERNEL);
	if (buffer == NULL) {
		hdmi_err("%s: kzalloc hdmi log buffer error!\n", __func__);
		return -1;
	}
	buffer_max_size = (u32)hdmi_reserve_size;

	memcpy(buffer, vaddr + paddr_offset, hdmi_reserve_size);

	_aw_hdmi_log_unmap(vaddr);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0) && !defined(MODULE)
	memblock_free(__va(hdmi_reserve_base), hdmi_reserve_size);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	memblock_free(hdmi_reserve_base, hdmi_reserve_size);
#endif

	return 0;
}

char *aw_hdmi_log_get_address(void)
{
	/* When copying buffer content to user space,
	 * buffer must be prevented from being written.
	 */
	mutex_lock(&lock_hdmi_log);
	return buffer;
}

void aw_hdmi_log_put_address(void)
{
	/* After copy to user is over, you can continue to write. */
	mutex_unlock(&lock_hdmi_log);
}

unsigned int aw_hdmi_log_get_start_index(void)
{
	/* Valid data range:
	 * 	override == 0: [0, end)
	 * 	override == 1: [end, buffer_max_size) + [0, end)
	 */
	if (override)
		return end;
	return 0;
}

unsigned int aw_hdmi_log_get_used_size(void)
{
	if (override)
		return buffer_max_size;
	return end;
}

unsigned int aw_hdmi_log_get_max_size(void)
{
	return buffer_max_size;
}

void aw_hdmi_log_set_enable(bool enable)
{
	hdmi_log_enable = enable;
}

bool aw_hdmi_log_get_enable(void)
{
	return hdmi_log_enable;
}

int aw_hdmi_log_init(struct device_node *of_node)
{
	if (_aw_hdmi_log_get_uboot_resource(of_node) < 0) {
		/* Even if uboot hdmi log is lost,
		 * it should be ensured that kernel can save the hdmi log.
		 */
		hdmi_inf("Get uboot hdmi log failed, malloc a new buffer...\n");
		buffer = kzalloc(LOG_BUFFER_SIZE, GFP_KERNEL);
		if (buffer == NULL) {
			hdmi_err("%s: kzalloc hdmi log buffer failed!\n", __func__);
			return -1;
		}
		end = 0;
		override = 0;
		buffer_max_size = LOG_BUFFER_SIZE;
	}

	mutex_init(&lock_hdmi_log);
	return 0;
}

void aw_hdmi_log_exit(void)
{
	if (buffer)
		kfree(buffer);
	buffer = NULL;
	end = 0;
	override = 0;
	buffer_max_size = 0;
	mutex_destroy(&lock_hdmi_log);
}

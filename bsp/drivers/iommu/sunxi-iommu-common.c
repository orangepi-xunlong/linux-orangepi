/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's pgtable controler
 *
 * Copyright (c) 2023, ouyangkun <ouyangkun@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/iommu.h>
#include <linux/slab.h>
#include "sunxi-iommu.h"
#include <sunxi-iommu.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
#include <trace/hooks/iommu.h>
#endif

static struct dentry *iommu_debug_dir;
int sunxi_iommu_check_cmd(struct device *dev, void *data)
{
	struct iommu_resv_region *region;
	int prot = IOMMU_WRITE | IOMMU_READ;
	struct list_head *rsv_list = data;
	struct {
		const char *name;
		u32 region_type;
	} supported_region[2] = { { "sunxi-iova-reserve", IOMMU_RESV_RESERVED },
				  { "sunxi-iova-premap", IOMMU_RESV_DIRECT } };
	int i, j;
#define REGION_CNT_MAX (8)
	struct {
		u64 array[REGION_CNT_MAX * 2];
		int count;
	} *tmp_data;

	tmp_data = kzalloc(sizeof(*tmp_data), GFP_KERNEL);
	if (!tmp_data)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(supported_region); i++) {
		/* search all supported argument */
		if (!of_find_property(dev->of_node, supported_region[i].name,
				      NULL))
			continue;

		tmp_data->count = of_property_read_variable_u64_array(
			dev->of_node, supported_region[i].name, tmp_data->array,
			0, REGION_CNT_MAX);
		if (tmp_data->count <= 0)
			continue;
		if ((tmp_data->count & 1) != 0) {
			dev_err(dev, "size %d of array %s should be even\n",
				tmp_data->count, supported_region[i].name);
			continue;
		}

		/* two u64 describe one region */
		tmp_data->count /= 2;

		/* prepared reserve region data */
		for (j = 0; j < tmp_data->count; j++) {
			region = iommu_alloc_resv_region(
				tmp_data->array[j * 2],
				tmp_data->array[j * 2 + 1], prot,
				supported_region[i].region_type
#ifdef RESV_REGION_NEED_GFP_FLAG
				,
				GFP_KERNEL
#endif
			);
			if (!region) {
				dev_err(dev, "no memory for iova rsv region");
			} else {
				struct iommu_resv_region *walk;
				/* warn on region overlaps */
				list_for_each_entry(walk, rsv_list, list) {
					phys_addr_t walk_end =
						walk->start + walk->length;
					phys_addr_t region_end =
						region->start + region->length;
					if (!(walk->start >
						      region->start +
							      region->length ||
					      walk->start + walk->length <
						      region->start)) {
						dev_warn(
							dev,
							"overlap on iova-reserve %pap~%pap with %pap~%pap",
							&walk->start, &walk_end,
							&region->start,
							&region_end);
					}
				}
				list_add_tail(&region->list, rsv_list);
			}
		}
	}
	kfree(tmp_data);
#undef REGION_CNT_MAX

	return 0;
}


static u32 __print_rsv_region(char *buf, size_t buf_len, ssize_t len,
			      struct dump_region *active_region,
			      bool for_sysfs_show)
{
	if (active_region->type == DUMP_REGION_RESERVE) {
		if (for_sysfs_show) {
			len += sysfs_emit_at(
				buf, len,
				"iova:%pad                            size:0x%zx\n",
				&active_region->iova, active_region->size);
		} else {
			len += scnprintf(
				buf + len, buf_len - len,
				"iova:%pad                            size:0x%zx\n",
				&active_region->iova, active_region->size);
		}
	}
	return len;
}

u32 sunxi_iommu_dump_rsv_list(struct list_head *rsv_list, ssize_t len,
			      char *buf, size_t buf_len, bool for_sysfs_show)
{
	struct iommu_resv_region *resv;
	struct dump_region active_region;
	if (for_sysfs_show) {
		len += sysfs_emit_at(buf, len, "reserved\n");
	} else {
		len += scnprintf(buf + len, buf_len - len, "reserved\n");
	}
	list_for_each_entry(resv, rsv_list, list) {
		active_region.access_mask = 0;
		active_region.iova = resv->start;
		active_region.type = DUMP_REGION_RESERVE;
		active_region.size = resv->length;
		len = __print_rsv_region(buf, buf_len, len, &active_region,
					 for_sysfs_show);
	}
	return len;
}

#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
static struct sunxi_iommu_iova_info *info_head, *info_tail;
static struct sunxi_iommu_iova_info *info_head_free, *info_tail_free;

int iova_show(struct seq_file *s, void *data)
{
	struct sunxi_iommu_iova_info *info_p;
	dma_addr_t iova_end;
	seq_puts(
		s,
		"device              iova_start     iova_end      size(byte)    timestamp(s)\n");
	for (info_p = info_head; info_p != NULL; info_p = info_p->next) {
		iova_end = info_p->iova + info_p->size;
		seq_printf(s, "%-16s    %pad       %pad      %zu      %ptTtr\n",
			   info_p->dev_name, &info_p->iova, &iova_end,
			   info_p->size, &info_p->timestamp);
	}

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(iova);

int iova_show_on_irq(void)
{
	struct sunxi_iommu_iova_info *info_p;
	dma_addr_t iova_end;

	printk("device              iova_start     iova_end      size(byte)    timestamp(s)\n");
	for (info_p = info_head; info_p != NULL; info_p = info_p->next) {
		iova_end = info_p->iova + info_p->size;
		printk("%-16s    %pad       %pad      %zu      %ptTtr\n",
		       info_p->dev_name, &info_p->iova, &iova_end, info_p->size,
		       &info_p->timestamp);
	}
	printk("\n");

	printk("iova that has been freed===================================================\n");
	printk("device              iova_start     iova_end      size(byte)    timestamp(s)\n");
	for (info_p = info_head_free; info_p != NULL; info_p = info_p->next) {
		iova_end = info_p->iova + info_p->size;
		printk("%-16s    %pad       %pad      %zu      %ptTtr\n",
		       info_p->dev_name, &info_p->iova, &iova_end, info_p->size,
		       &info_p->timestamp);
	}
	printk("\n");

	return 0;
}

static void sunxi_iommu_alloc_iova(void *p, struct device *dev,
				   struct iova_domain *domain, dma_addr_t iova,
				   size_t size)
{
	struct sunxi_iommu_iova_info *info_p;

	if (info_head == NULL) {
		info_head = info_p = kmalloc(sizeof(*info_head), GFP_KERNEL);
		info_tail = info_head;
	} else {
		info_p = kmalloc(sizeof(*info_head), GFP_KERNEL);
		info_tail->next = info_p;
		info_tail = info_p;
	}
	strcpy(info_p->dev_name, dev_name(dev));
	info_p->iova = iova;
	info_p->size = size;
	info_p->timestamp = ktime_get_seconds();
}

static void sunxi_iommu_free_iova(void *p, struct iova_domain *domain,
				  dma_addr_t iova, size_t size)
{
	struct sunxi_iommu_iova_info *info_p, *info_prev, *info_p_free;

	info_p = info_prev = info_head;
	for (; info_p != NULL; info_p = info_p->next) {
		if (info_p->iova == iova && info_p->size == size)
			break;
		info_prev = info_p;
	}
	if (!info_p) {
		pr_err("suspicious iova free: %zu byte @ %pad\n", size, &iova);
		return;
	}

	if (info_head_free == NULL) {
		info_head_free = kmalloc(sizeof(*info_head_free), GFP_KERNEL);
		info_p_free = info_tail_free = info_head_free;
	} else {
		info_p_free = kmalloc(sizeof(*info_head_free), GFP_KERNEL);
		info_tail_free->next = info_p_free;
		info_tail_free = info_p_free;
	}
	strcpy(info_p_free->dev_name, info_p->dev_name);
	info_p_free->iova = info_p->iova;
	info_p_free->size = info_p->size;
	info_p_free->timestamp = info_p->timestamp;

	if (info_p == info_head) { // free head
		info_head = info_p->next;
	} else if (info_p == info_tail) { // free tail
		info_tail = info_prev;
		info_tail->next = NULL;
	} else // free middle
		info_prev->next = info_p->next;

	kfree(info_p);
	info_p = NULL;
}

void sunxi_iommu_register_vendorhook(void)
{
	int ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = register_trace_android_vh_iommu_iovad_alloc_iova(sunxi_iommu_alloc_iova, NULL) ?:
		      register_trace_android_vh_iommu_iovad_free_iova(
			      sunxi_iommu_free_iova, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	ret = register_trace_android_vh_iommu_alloc_iova(sunxi_iommu_alloc_iova, NULL) ?:
		      register_trace_android_vh_iommu_free_iova(
			      sunxi_iommu_free_iova, NULL);
#endif
	if (ret)
		pr_err("%s: register android vendor hook failed!\n", __func__);
}

#endif

void sunxi_iommu_init_debugfs(struct sunxi_iommu_dev *sunxi_iommu)
{
	iommu_debug_dir = debugfs_create_dir("iommu_sunxi", NULL);
	if (!iommu_debug_dir) {
		pr_err("%s: create iommu debug dir failed!\n", __func__);
		return;
	}
#if IS_ENABLED(CONFIG_AW_IOMMU_IOVA_TRACE)
	debugfs_create_file("iova", 0444, iommu_debug_dir, sunxi_iommu,
			    &iova_fops);
#endif
}

void sunxi_iommu_release_debugfs(void)
{
	debugfs_remove(iommu_debug_dir);
}

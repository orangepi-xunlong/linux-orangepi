/*
 * sunxi's Remote Processor Share Interrupt Driver
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>

#include <remoteproc/sunxi_remoteproc.h>
#include "sunxi_share_interrupt.h"
#ifdef CONFIG_ARCH_SUN55IW3
#include "sun55iw3-share-irq.h"
#endif

#define irm_debug(fmt, ...)		pr_debug("Sunxi IRM %s:%d " fmt,\
									__func__, __LINE__, ##__VA_ARGS__)
#define irm_warn(fmt, ...)		pr_warn("Sunxi IRM %s:%d " fmt,\
									__func__, __LINE__, ##__VA_ARGS__)
#define irm_err(fmt, ...)		pr_err("Sunxi IRM %s:%d " fmt,\
									__func__, __LINE__, ##__VA_ARGS__)
#define IRQ_NP_NAME			"reserved-irq"
#define CPU_BANK_NUM			16
#define READY_TAG			(('R' << 24) | ('E' << 16) | ('D' << 8) | 'Y')
#define USED_TAG			(('U' << 24) | ('S' << 16) | ('E' << 8) | 'D')

#define INFO_NUM			(5)
#define GET_INFO_MAJOR(info, i)		(info[i * INFO_NUM + 0])
#define GET_INFO_TYPE(info, i)		(info[i * INFO_NUM + 1])
#define GET_INFO_REMOTE_IRQ(info, i)	(info[i * INFO_NUM + 2])
#define GET_INFO_FLAGS(info, i)		(info[i * INFO_NUM + 4])

#define IS_GPIO_TYPE(type)		(!!(type))
#define GET_GPIO_INDEX(type)		((type) - 1)

/* Unalign for communication */
struct _table_head {
	uint32_t tag;
	uint32_t len;
	uint32_t banks_off;
	uint32_t banks_num;
} __packed;

struct _table_entry {
	uint32_t status;
	uint32_t type;
	uint32_t flags;
	uint32_t arch_irq;
} __packed;

struct arch_irq {

	const char *name;
	bool visable;
	/*
	 * info format
	 *    major, type, remote_irq, local_hwirq, flags
	 */
	uint32_t *info;
	uint32_t entris;
	/*
	 * Table Format:
	 *     The first 16 byte is reserved for _table_head
	 *     The last CPU_BANK_NUM word is resource for gpio mask
	 *
	 *     ----------------
	 *     | _table_head  |
	 *     ----------------
	 *     | _table_entry |
	 *     ----------------
	 *     |     ...      |
	 *     ----------------
	 *     |  GPIOA Mask  |
	 *     ----------------
	 *     |     ...      |
	 *     ---------------
	 */
	void __iomem *table_addr;
	uint32_t len;
	/*
	 * record Interrupt Status
	 * 0 : Disable
	 * 1 : Enable
	 */
	uint8_t *status;
	/*
	 * record GPIO Alloc Status
	 * 0 : Host
	 * 1 : Arch
	 */
	uint32_t gpio_banks[CPU_BANK_NUM];
	/*
	 * record GPIO IRQ number mapping with bank number
	 * [banks][0] = hwirq
	 * [banks][1] = softirq
	 */
	uint32_t gpio_irq_map[CPU_BANK_NUM][2];
};

static uint32_t g_arch_number;
static struct arch_irq *g_arch_array;

static int sunxi_init_arch_info(struct device_node *child, struct arch_irq *arch)
{
	const char *name;
	struct device_node *mem_np;
	struct resource r;
	void __iomem *va;
	int ret;

	arch->visable = false;

	ret = of_property_read_string(child, "arch-name", &name);
	if (ret < 0) {
		irm_err("Can't find arch-name property.\n");
		return -EINVAL;
	}
	arch->name = name;

	mem_np = of_parse_phandle(child, "memory-region", 0);
	if (IS_ERR_OR_NULL(mem_np)) {
		irm_err("Can't find memory-region property.\n");
		return -EINVAL;
	}
	ret = of_address_to_resource(mem_np, 0, &r);
	if (ret) {
		irm_err("Failed to get memory-region address range,ret=%d.\n", ret);
		return -EFAULT;
	}
	va = ioremap_wc(r.start, resource_size(&r));
	if (IS_ERR_OR_NULL(va)) {
		irm_err("Fialed to remap memory-region\n");
		return -ENOMEM;
	}
	arch->table_addr = va;
	arch->len = resource_size(&r);
	irm_debug("map table from 0x%zx to 0x%p\n", (size_t)r.start, va);

	ret = of_property_count_elems_of_size(child, "share-irq", INFO_NUM * sizeof(u32));
	if (ret < 0) {
		irm_err("fail to get share-irq\n");
		return -ENXIO;
	}
	arch->entris = ret;
	arch->info = kzalloc(ret * INFO_NUM * sizeof(u32), GFP_KERNEL);
	if (IS_ERR_OR_NULL(arch->info)) {
		irm_warn("Failed to alloc info array,ret=%d.\n", ret);
		return -ENOMEM;
	}
	ret = of_property_read_u32_array(child, "share-irq", arch->info, arch->entris * INFO_NUM);
	if (ret < 0) {
		irm_err("Failed to read share-irq property,ret=%d\n", ret);
		return -EFAULT;
	}

	arch->status = kzalloc(ARRAY_SIZE(share_irq_table), GFP_KERNEL);
	if (IS_ERR_OR_NULL(arch->status)) {
		irm_warn("Failed to alloc info status array,ret=%d.\n", ret);
		kfree(arch->info);
		return -ENOMEM;
	}

	return 0;
}

static void sunxi_update_gpio_mask(struct arch_irq *arch)
{
	int i, j;
	uint32_t type, index, major, mask;

	irm_debug("%s gpio irq  map:\n", arch->name);
	/* update gpio softirq info */
	for (i = 0; i < arch->entris; i++) {
		type = GET_INFO_TYPE(arch->info, i);
		major = GET_INFO_MAJOR(arch->info, i);
		index = GET_GPIO_INDEX(type);

		if (!IS_GPIO_TYPE(type))
			continue;

		/* search in ther share_irq_table[] and get hwirq/softirq by major */
		for (j = 0; j < ARRAY_SIZE(share_irq_table); j++) {
			if (share_irq_table[j].major == major) {
				arch->gpio_irq_map[index][0] = share_irq_table[j].hwirq;
				arch->gpio_irq_map[index][1] = share_irq_table[j].softirq;
				irm_debug("\tGPIO%c -> %d\n", index, arch->gpio_irq_map[index][1]);
				break;
			}
		}
	}
	irm_debug("\n");

	irm_debug("%s info:\n", arch->name);
	/* update info to globle gpio banks */
	for (i = 0; i < arch->entris; i++) {
		type = GET_INFO_TYPE(arch->info, i);
		mask = GET_INFO_FLAGS(arch->info, i);
		index = GET_GPIO_INDEX(type);

		irm_debug("\t0x%x 0x%x 0x%08x\n", GET_INFO_MAJOR(arch->info, i), type, mask);

		if (!IS_GPIO_TYPE(type))
			continue;
		if (arch->gpio_banks[index] & mask)
			irm_warn("Incompatible GPIO PIN:0x%x.\n", (arch->gpio_banks[index] & mask));
		arch->gpio_banks[index] |= mask;
	}
	irm_debug("\n");
}

static void sunxi_update_table(struct arch_irq *arch)
{
	int i, j;
	uint32_t major, offset, type, hwirq, flags, remote_irq;
	struct _table_entry *ptable;
	struct _table_head *phead;
	uint32_t *gpio_mask;

	phead = arch->table_addr;
	ptable = arch->table_addr + sizeof(*phead);

	irm_debug("%s info:\n", arch->name);
	irm_debug("\t head address %p:\n", phead);
	irm_debug("\t Entry address %p:\n", ptable);

	if (phead->tag != USED_TAG)
		memset(arch->table_addr, 0, arch->len);
	/* update info to globle interrupt table */
	for (i = 0; i < arch->entris; i++) {
		major = GET_INFO_MAJOR(arch->info, i);
		type = GET_INFO_TYPE(arch->info, i);
		remote_irq = GET_INFO_REMOTE_IRQ(arch->info, i);
		flags = GET_INFO_FLAGS(arch->info, i);

		for (j = 0; j < ARRAY_SIZE(share_irq_table); j++) {
			hwirq = share_irq_table[j].hwirq;
			if (share_irq_table[j].major != major)
				continue;
			irm_debug("\tUpdate Entry %d irq to %s.\n", hwirq, arch->name);
			irm_debug("\t\tEntry status = 1\n");
			irm_debug("\t\tEntry type  = 0x%x\n", type);
			irm_debug("\t\tEntry IRQ   = %d\n", remote_irq);
			irm_debug("\t\tEntry flags = 0x%x\n", flags);
			ptable[hwirq].status = 1;
			ptable[hwirq].type = type;
			ptable[hwirq].flags = flags;
			ptable[hwirq].arch_irq = remote_irq;
			share_irq_table[j].arch_irq = remote_irq;
		}
	}
	/* update gpio banks info to table */
	gpio_mask = arch->table_addr + arch->len - CPU_BANK_NUM * sizeof(uint32_t);
	irm_debug("Update GPIO Banks Table from 0x%p to 0x%p.\n", arch->gpio_banks, gpio_mask);
	memcpy(gpio_mask, arch->gpio_banks, CPU_BANK_NUM * sizeof(uint32_t));
	/* mark globle interrupt table ready */
	offset = (uint32_t)((unsigned long)gpio_mask - (unsigned long)(arch->table_addr));
	phead->tag = READY_TAG;
	phead->len = arch->len;
	phead->banks_off = offset;
	phead->banks_num = CPU_BANK_NUM;

	irm_debug("%s Table Ready.\n", arch->name);
	irm_debug("\n");
}

static struct arch_irq *sunxi_find_arch_by_name(const char *name)
{
	int i;
	struct arch_irq *arch;

	for (i = 0; i < g_arch_number; i++) {
		arch = &g_arch_array[i];
		if (strcmp(name, arch->name))
			continue;
		return arch;
	}
	return NULL;
}

int sunxi_arch_interrupt_save(const char *name)
{
	int i, j;
	struct arch_irq *arch;
	uint32_t type, mask, major;

	arch = sunxi_find_arch_by_name(name);
	if (!arch) {
		irm_warn("No such %s arch name.\n", name);
		return -ENODEV;
	}

	sunxi_update_table(arch);
	/* save normal interrupt */
	for (i = 0; i < arch->entris; i++) {
		type = GET_INFO_TYPE(arch->info, i);
		major = GET_INFO_MAJOR(arch->info, i);

		if (IS_GPIO_TYPE(type))
			continue;

		for (j = 0; j < ARRAY_SIZE(share_irq_table); j++) {
			if (share_irq_table[j].major != major)
				continue;
			/* TODO: host is enable this tnterrupt? */
			if (!can_request_irq(share_irq_table[j].softirq, 0))
				arch->status[j] = 1;
			arch->status[j] = 0;
		}
	}

	/* save gpio interrupt */
	for (i = 0; i < arch->entris; i++) {
		major = GET_INFO_MAJOR(arch->info, i);
		type = GET_INFO_TYPE(arch->info, i);
		mask = GET_INFO_FLAGS(arch->info, i);

		if (!IS_GPIO_TYPE(type))
			continue;

		if (mask != 0xffffffff)
			continue;

		for (j = 0; j < ARRAY_SIZE(share_irq_table); j++) {
			if (share_irq_table[j].major == major) {
				arch->status[j] = 1;
				disable_irq(share_irq_table[j].softirq);
				break;
			}
		}
	}
	arch->visable = true;

	return 0;
}
EXPORT_SYMBOL(sunxi_arch_interrupt_save);

int sunxi_arch_interrupt_restore(const char *name)
{
	int i;
	struct arch_irq *arch;
	uint32_t type, major;
	struct _table_head *phead;

	arch = sunxi_find_arch_by_name(name);
	if (!arch) {
		irm_warn("No such %s arch name.\n", name);
		return -ENODEV;
	}

	phead = arch->table_addr;
	/* restore all interrupt */
	for (i = 0; i < ARRAY_SIZE(share_irq_table); i++) {
		type = GET_INFO_TYPE(arch->info, i);
		major = GET_INFO_MAJOR(arch->info, i);

		if (!arch->status[i])
			continue;
		enable_irq(share_irq_table[i].softirq);
	}
	memset(arch->status, 0, ARRAY_SIZE(share_irq_table));
	arch->visable = false;
	phead->tag = 0;

	return 0;
}
EXPORT_SYMBOL(sunxi_arch_interrupt_restore);

uint32_t sunxi_rproc_get_gpio_mask_by_hwirq(int hwirq)
{
	int i, j;
	uint32_t mask = 0;
	struct arch_irq *arch;

	for (i = 0; i < g_arch_number; i++) {
		arch = &g_arch_array[i];
		if (!arch->visable)
			continue;

		/* trans softirq to banks */
		for (j = 0; j < CPU_BANK_NUM; j++) {
			if (arch->gpio_irq_map[j][0] == hwirq) {
				mask |= arch->gpio_banks[j];
				break;
			}
		}
	}

	return mask;
}
EXPORT_SYMBOL(sunxi_rproc_get_gpio_mask_by_hwirq);

static ssize_t sunxi_arch_irq_info_read(struct file *filp, char __user *userbuf,
			       size_t count, loff_t *ppos)
{
	struct arch_irq *arch = filp->private_data;
	/* need room for the name, a newline and a terminating null */
	char buf[1024];
	int i, j;

	i = scnprintf(buf, sizeof(buf), "Status: %s\n",
					arch->visable ? "Active" : "Inactive");
	for (j = 0; j < CPU_BANK_NUM; j++) {
		i += scnprintf(buf + i, sizeof(buf) - i, "\tGPIO%c: 0x%08x\n",
						'A' + j, arch->gpio_banks[j]);
	}

	return simple_read_from_buffer(userbuf, count, ppos, buf, i);
}

static const struct file_operations sunxi_irq_info_ops = {
	.read = sunxi_arch_irq_info_read,
	.open = simple_open,
	.llseek	= generic_file_llseek,
};

int sunxi_arch_create_debug_dir(struct dentry *dir, const char *name)
{
	struct arch_irq *arch;

	if (!dir) {
		irm_warn("Invalid debug dir.\n");
		return -EINVAL;
	}
	arch = sunxi_find_arch_by_name(name);
	if (!arch) {
		irm_warn("No such %s arch name.\n", name);
		return -ENODEV;
	}

	debugfs_create_file("share-interrupt", 0400, dir,
			    arch, &sunxi_irq_info_ops);

	return 0;
}
EXPORT_SYMBOL(sunxi_arch_create_debug_dir);

static int __init sunxi_share_resource_init(void)
{
	struct device_node *np, *child;
	int cnt, i;

	if (g_arch_array) {
		irm_warn("Sunxi Interrupt Resource Already Init.\n");
		return 0;
	}

	/* mapping hardware interrupt */
	for (i = 0; i < ARRAY_SIZE(share_irq_table); i++) {
		/*
		 * get softirq by gic irq_find_mapping().
		 * share_irq_table[i].softirq = irqnum_trans_from_hwirq(share_irq_table[i].hwirq);
		 * but no use of softirq
		 */
		share_irq_table[i].softirq = 0;
		irm_debug("Trans IRQ: %d -> %d\n", share_irq_table[i].hwirq,
						share_irq_table[i].softirq);
	}

	np = of_find_node_by_name(NULL, IRQ_NP_NAME);
	if (!np) {
		irm_err("Can't Find reserved-irq node in dts.\n");
		return -ENOMEM;
	}

	cnt = of_get_child_count(np);
	irm_debug("Child Cnt=%d\n", cnt);
	g_arch_number = cnt;

	g_arch_array = kzalloc(cnt * sizeof(*g_arch_array), GFP_KERNEL);
	if (IS_ERR_OR_NULL(g_arch_array)) {
		irm_err("Alloc Resource Table Failed,ret=%ld.\n",
						PTR_ERR(g_arch_array));
		g_arch_array = NULL;
		return -ENOMEM;
	}

	cnt = 0;
	for_each_child_of_node(np, child) {
		if (sunxi_init_arch_info(child, &g_arch_array[cnt]))
			goto out;
		sunxi_update_gpio_mask(&g_arch_array[cnt]);
		sunxi_update_table(&g_arch_array[cnt]);
		cnt++;
	}

	return 0;
out:
	for (i = 0; i < cnt; i++) {
		g_arch_array[i].name = NULL;
		if (g_arch_array[i].info)
			kfree(g_arch_array[i].info);
		if (g_arch_array[i].table_addr)
			iounmap(g_arch_array[i].table_addr);
	}

	return -EINVAL;
}
early_initcall(sunxi_share_resource_init);

MODULE_DESCRIPTION("SUNXI Remote Processor Share IRQ Driver");
MODULE_AUTHOR("lijiajian <lijiajian@allwinnertech.com>");
MODULE_LICENSE("GPL v2");

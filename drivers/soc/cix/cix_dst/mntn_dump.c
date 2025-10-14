// SPDX-License-Identifier: GPL-2.0
/*
 * mntn_dump.c
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/of_address.h>
#include <mntn_dump_interface.h>
#include "dst_print.h"
#include <linux/soc/cix/util.h>
#include <linux/acpi.h>

static void __iomem *g_mntn_dump_base;
static unsigned long long g_mntn_dump_reserved_addr;
static unsigned int g_mntn_dump_init;
static unsigned int g_mntn_dump_size;
static struct mdump_head *g_mdump_head;

#define MNTN_DUMP_RESERVED_ADDR (0x83de0000)
#define MNTN_DUMP_SIZE (0x400000)
#define MNTN_DUMP_NOCLEAN (0xAA)
struct mntn_dump_mem_info {
	unsigned int size; // mntn dump region size
	unsigned int clean_flag; // clean flag
};
struct mntn_dump_mem_info g_mntn_dump_mem_size[MNTN_DUMP_MAX] = {
	{ MNTN_DUMP_HEAD_SIZE, 0 },
	{ MNTN_DUMP_KERNEL_DUMP_SIZE, 0 },
};

static DEFINE_RAW_SPINLOCK(g_mdump_lock);
struct mdump_mem_info {
	unsigned int size;
	unsigned int offset;
	void __iomem *vaddr;
};
struct mdump_mem_info g_mdump_mem_info[MNTN_DUMP_MAX];

static int get_mnmt_dump_reserve_addr_acpi(void)
{
	acpi_handle handle;
	acpi_status status;
	struct acpi_device *adev;

	status = acpi_get_handle(NULL, "\\_SB.DSTD.APAD", &handle);
	if (ACPI_FAILURE(status)) {
		g_mntn_dump_reserved_addr = MNTN_DUMP_RESERVED_ADDR;
		g_mntn_dump_size = MNTN_DUMP_SIZE;
		goto exit;
	}

	adev = acpi_get_acpi_dev(handle);
	if (!adev) {
		DST_ERR("Failed to get acpi dev\n");
		return -1;
	}

	status = fwnode_property_read_u64_array(&adev->fwnode, "mntndump_addr",
						&g_mntn_dump_reserved_addr, 1);
	if (status) {
		DST_ERR("Failed to get property mntndump_addr\n");
		return -1;
	}

	status = fwnode_property_read_u32_array(&adev->fwnode, "mntndump_size",
						&g_mntn_dump_size, 1);
	if (status) {
		DST_ERR("Failed to get property mntndump_size\n");
		return -1;
	}
exit:
	DST_PN("get_mntn_dump_addr addr 0x%llx, size:0x%x\n",
		g_mntn_dump_reserved_addr, g_mntn_dump_size);
	return 0;
}

static int get_mntn_dump_reserve_addr(void)
{
	struct device_node *np = NULL;
	const char *name = NULL;
	int len;
	const unsigned long *p = NULL;

	np = of_find_compatible_node(NULL, NULL, DTS_MNTNDUMP_NAME);
	if (!np) {
		DST_PN("dts node(%s) not found\n", DTS_MNTNDUMP_NAME);
		return get_mnmt_dump_reserve_addr_acpi();
	}

	/* check if status = ok, okay or status not defined*/
	name = of_get_property(np, "status", &len);
	if (name && strncmp(name, "okay", sizeof("okay")) != 0 &&
	    strncmp(name, "ok", sizeof("ok")) != 0) {
		DST_ERR("get status(%.7s)  error\n", name);
		return -1;
	}

	p = (unsigned long *)of_get_property(np, "reg", NULL);
	if (!p) {
		DST_ERR("get reg fail, len =%d\n", len);
		goto err;
	}

	g_mntn_dump_reserved_addr = (u64)be64_to_cpup((const __be64 *)p);
	g_mntn_dump_size = (unsigned int)be64_to_cpup((const __be64 *) ++p) - MNTN_DUMP_KASLR_SIZE;
	if (!g_mntn_dump_reserved_addr || !g_mntn_dump_size) {
		DST_ERR("get_mntn_dump_addr Error: addr: 0x%llx, size:0x%x\n",
			g_mntn_dump_reserved_addr, g_mntn_dump_size);
		goto err;
	}

	DST_PN("get_mntn_dump_addr addr 0x%llx, size:0x%x\n",
	  g_mntn_dump_reserved_addr, g_mntn_dump_size);
	of_node_put(np);
	return 0;
err:
	of_node_put(np);
	return -1;
}

static void mntn_dump_head_crc(void)
{
	g_mdump_head->crc = 0;
	g_mdump_head->crc =
		checksum32((u32 *)g_mdump_head, sizeof(struct mdump_head));
}

int mntn_dump_init(void)
{
	int i;
	unsigned int offset;

	DST_PR_START();

	/* already initialized */
	if (g_mntn_dump_init)
		return 0;

	if (!g_mntn_dump_reserved_addr || !g_mntn_dump_size) {
		if (get_mntn_dump_reserve_addr()) {
			DST_ERR("reserve addr is NULL\n");
			goto err;
		}
	}

	g_mntn_dump_base = (void *)ioremap_wc(
		(phys_addr_t)g_mntn_dump_reserved_addr, g_mntn_dump_size);
	if (!g_mntn_dump_base) {
		DST_ERR("ioremap error\n");
		goto err;
	}
	DST_PN("mntn dump base addr:%px\n", g_mntn_dump_base);
	/* clean the memory of information struct  */
	memset((void *)g_mdump_mem_info, 0x00, sizeof(g_mdump_mem_info));

	offset = 0;
	for (i = 0; i < MNTN_DUMP_MAX; i++) {
		g_mdump_mem_info[i].offset = offset;
		g_mdump_mem_info[i].size = g_mntn_dump_mem_size[i].size;
		g_mdump_mem_info[i].vaddr = g_mntn_dump_base + offset;

		/*clean the reserve memory of mntn dump*/
		if (g_mntn_dump_mem_size[i].clean_flag != MNTN_DUMP_NOCLEAN)
			memset((void *)g_mdump_mem_info[i].vaddr, 0x00,
			       g_mdump_mem_info[i].size);

		offset += g_mdump_mem_info[i].size;
		if (offset >= MNTN_DUMP_MAXSIZE) {
			DST_ERR("mntn dump size is out of range\n");
			goto err;
		}
		DST_PN("dump meminfo %d, 0x%x, 0x%px, 0x%x\n", i,
		       g_mdump_mem_info[i].offset, g_mdump_mem_info[i].vaddr,
		       g_mdump_mem_info[i].size);
	}

	/* init head information */
	g_mdump_head =
		(struct mdump_head *)g_mdump_mem_info[MNTN_DUMP_HEAD].vaddr;
	g_mdump_head->magic = MNTNDUMP_MAGIC;
	g_mdump_head->version = MNTN_DUMP_VERSION;
	g_mdump_head->regs_info[0].mid = MNTN_DUMP_HEAD;
	g_mdump_head->regs_info[0].size = g_mdump_mem_info[MNTN_DUMP_HEAD].size;
	g_mdump_head->regs_info[0].offset =
		g_mdump_mem_info[MNTN_DUMP_HEAD].offset;
	g_mdump_head->nums = 1;
	mntn_dump_head_crc();
	g_mntn_dump_init = 1;

	DST_PR_END();
	return 0;
err:
	return -1;
}

early_initcall(mntn_dump_init);

/*
 *Func: register_mntn_dump()
 *	register a mntn_dump module, Output the virtual address.
 *Input:
 *	@mod_id: module ID
 *	@size:	 the actual size of mntn dump module
 *Output:
 *	@vaddr:  output the virtaul addr of reserved memory
 *return:
 *	0: OK; Others: Fail;
 */
int register_mntn_dump(int mod_id, unsigned int size, void **vaddr)
{
	u32 i;
	char **ptr;

	if (!vaddr) {
		DST_ERR("module id[%d], vaddr is NULL\n", mod_id);
		return -1;
	}

	ptr = (char **)vaddr;
	*ptr = 0;

	if (mod_id < MNTN_DUMP_HEAD || mod_id >= MNTN_DUMP_MAX) {
		DST_ERR("module id[%d] is invalid\n", mod_id);
		return -1;
	}

	if (!g_mntn_dump_init) {
		if (mntn_dump_init()) {
			DST_ERR("module id[%d] fail\n", mod_id);
			return -1;
		}
	}
	raw_spin_lock(&g_mdump_lock);
	if (g_mdump_mem_info[mod_id].size < size) {
		DST_ERR("module[%d] size(0x%x) is invalid\n", mod_id, size);
		goto err;
	}

	i = g_mdump_head->nums;
	if (i >= MNTN_DUMP_MAX) {
		DST_ERR("mntn dump data corruption(nums: %d)\n", i);
		goto err;
	}
	g_mdump_head->regs_info[i].mid = mod_id;
	g_mdump_head->regs_info[i].size = g_mdump_mem_info[mod_id].size;
	g_mdump_head->regs_info[i].offset = g_mdump_mem_info[mod_id].offset;
	g_mdump_head->nums += 1;

	mntn_dump_head_crc();

	*ptr = (char *)g_mdump_mem_info[mod_id].vaddr;
	raw_spin_unlock(&g_mdump_lock);
	return 0;
err:
	raw_spin_unlock(&g_mdump_lock);
	return -1;
}

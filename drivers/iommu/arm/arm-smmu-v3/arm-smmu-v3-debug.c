// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * Author: Zichar Zhang <zichar.zhang@cixtech.com>
 */
#include <linux/debugfs.h>
#include <linux/iommu.h>
#include <linux/io-pgtable.h>
#include <linux/pci.h>
#include "arm-smmu-v3.h"
#include "arm-smmu-v3-walk.h"

#define smmu_print printk
#define NAME_BUF_LEN 64
#define NAME_MIN(x, y) ((x) < (y) ? x : y)

static char master_dname[NAME_BUF_LEN];
static char smmu_dname[NAME_BUF_LEN];

static ssize_t master_dname_read(struct file *filp, char __user *buf,
				size_t count, loff_t *pos)
{
	return simple_read_from_buffer(buf, count, pos, master_dname,
				NAME_MIN(strlen(master_dname), NAME_BUF_LEN));
}

static ssize_t master_dname_write(struct file *filp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	int len;
	char *name = master_dname;

	if (*pos != 0)
		return 0;

	if (count >= NAME_BUF_LEN)
		return -ENOSPC;

	len = simple_write_to_buffer(name, NAME_BUF_LEN - 1, pos, buf, count);
	if (len < 0)
		return len;

	if (len == 1 && name[0] == '\n')
		name[0] = '\0'; /* set empty string */
	else
		name[len] = '\0';

	return count;
}

static ssize_t smmu_dname_read(struct file *filp, char __user *buf,
				size_t count, loff_t *pos)
{
	return simple_read_from_buffer(buf, count, pos, smmu_dname,
				NAME_MIN(strlen(smmu_dname), NAME_BUF_LEN));
}

static ssize_t smmu_dname_write(struct file *filp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	int len;
	char *name = smmu_dname;

	if (*pos != 0)
		return 0;

	if (count >= NAME_BUF_LEN)
		return -ENOSPC;

	len = simple_write_to_buffer(name, NAME_BUF_LEN - 1, pos, buf, count);
	if (len < 0)
		return len;

	if (len == 1 && name[0] == '\n')
		name[0] = '\0'; /* set empty string */
	else
		name[len] = '\0';

	return count;
}

static const struct file_operations master_dname_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = master_dname_read,
	.write = master_dname_write,
};

static const struct file_operations smmu_dname_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = smmu_dname_read,
	.write = smmu_dname_write,
};

static int master_print_init(struct arm_smmu_master *master, void *data)
{
	if (!master)
		return -EINVAL;

	smmu_print(" -- master %s --\n", dev_name(master->dev));

	return 0;
}

static int master_print_ste(__le64 *ste, u32 sid, void *data)
{
	if (!ste)
		return -EINVAL;

	for (int i = 0; i < STRTAB_STE_DWORDS; i++) {
		smmu_print("STE[0x%02X: %02d/%02d]: 0x%016llX\n",
				sid, i * 4, STRTAB_STE_DWORDS * 4, *ste);
		ste++;
	}

	return 0;
}

static int master_print_cd(__le64 *cd, void *data)
{
	__le64 *pcd = cd;

	if (!cd)
		return -EINVAL;

	for (int i = 0; i < CTXDESC_CD_DWORDS; i++) {
		smmu_print("CD[%02d/%02d]: 0x%016llX\n",
			i * 4, CTXDESC_CD_DWORDS * 4, *pcd);
		pcd++;
	}

	return 0;
}

static int master_print_io_map(u64 iova, phys_addr_t pa, u32 len, u32 port,
			void *data)
{
	smmu_print("va[0x%016llX] -> pa[0x%016llX] len[0x%04X]\n", iova, pa, len);

	return 0;
}

static ssize_t smmu_walk_read(struct file *filp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct smmu_master_handle hdl = {
		.smmu_name = strlen(smmu_dname) ? smmu_dname : NULL,
		.smmu_master_name = strlen(master_dname) ? master_dname : NULL,
		.init = master_print_init,
		.hdl_ste = master_print_ste,
		.hdl_cd = master_print_cd,
		.hdl_io_map_s1 = master_print_io_map,
	};

	/* TODO: pagetable info could be huge, use printk circle buf now */
	smmu_master_walk(&hdl);

	return 0;
}

static const struct file_operations smmu_dump_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = smmu_walk_read,
};

static int __init smmu_debug_init(void)
{
	struct dentry *dir;

	dir = debugfs_lookup("smmu", NULL);
	if (!dir)
		dir = debugfs_create_dir("smmu", NULL);
	if (!dir)
		return -1;

	debugfs_create_file("dump", 0444, dir, NULL, &smmu_dump_fops);
	debugfs_create_file("master", 0644, dir, NULL, &master_dname_fops);
	debugfs_create_file("smmu", 0644, dir, NULL, &smmu_dname_fops);

	return 0;
}
module_init(smmu_debug_init)

MODULE_AUTHOR("Zichar Zhang <zichar.zhang@cixtech.com>");
MODULE_DESCRIPTION("SMMU INFO DEBUG");
MODULE_LICENSE("GPL v2");

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * Author: Zichar Zhang <zichar.zhang@cixtech.com>
 *
 * This driver offers smmu message queue dump info
 */
#include <linux/cma.h>
#include <linux/debugfs.h>
#include <linux/highmem.h>
#include <linux/iommu.h>
#include <linux/io-pgtable.h>
#include <linux/pci.h>
#include <linux/soc/cix/rdr_pub.h>
#include <linux/soc/cix/rdr_platform.h>
//#include <mntn_subtype_exception.h>
#include "arm-smmu-v3.h"
#include "arm-smmu-v3-walk.h"
#include "arm-smmu-v3-dump.h"

#define SMMU_DUMP_DEBUG

#define NAME_BUF_LEN 64
#define NAME_MIN(x, y) ((x) < (y) ? x : y)

#ifdef SMMU_DUMP_DEBUG
#define smmu_print printk
#define SMMU_DUMP_BUF_SIZE SZ_32K
static int smmu_dump_debug_keep = 1;
extern struct cma *dma_contiguous_default_area;
#endif

static int smmu_master_dump_init(struct arm_smmu_master *master, void *data)
{
	struct smmu_dump_head *head = data;
	struct smmu_dump_master_head *mhead;

	if (!master || !data)
		return -EINVAL;

	if ((head->total_size + sizeof(*mhead)) > head->dump_max)
		return -ENOSPC;

	head->last_master = head->total_size;
	head->master_num++;
	head->total_size += sizeof(*mhead);

	mhead = LAST_MASTER_HEAD(head);
	mhead->ste_num = 0;
	mhead->io_map_s1_size = 0;
	snprintf(mhead->name, MASTER_NAME_SIZE, dev_name(master->dev));

#ifdef SMMU_DUMP_DEBUG
	smmu_print(" -- master %s --\n", dev_name(master->dev));
#endif

	return 0;
}

static int smmu_dump_ste(__le64 *ste, u32 sid, void *data)
{
	struct smmu_dump_head *head = data;
	struct smmu_dump_master_head *mhead;
	void *pos;

	if (!ste || !data)
		return -EINVAL;

	if ((head->total_size + SMMU_STE_SIZE) > head->dump_max)
		return -ENOSPC;

	mhead = LAST_MASTER_HEAD(head);
	pos = MASTER_STE(mhead);
	memcpy(pos, ste, SMMU_STE_SIZE);

	mhead->ste_num++;
	head->total_size += SMMU_STE_SIZE;

#ifdef SMMU_DUMP_DEBUG
	for (int i = 0; i < (STRTAB_STE_DWORDS / 2); i++) {
		smmu_print("STE[0x%02X: %02d/%02d]: 0x%016llX\n",
				sid, i * 8, STRTAB_STE_DWORDS * 4, *ste);
		ste++;
	}
#endif

	return 0;
}

static int smmu_dump_cd(__le64 *cd, void *data)
{
	struct smmu_dump_head *head = data;
	struct smmu_dump_master_head *mhead;
	void *pos;

	if (!cd || !data)
		return -EINVAL;

	if ((head->total_size + SMMU_CD_SIZE) > head->dump_max)
		return -ENOSPC;

	mhead = LAST_MASTER_HEAD(head);
	pos = MASTER_CD(mhead);
	memcpy(pos, cd, SMMU_CD_SIZE);

	head->total_size += SMMU_CD_SIZE;

#ifdef SMMU_DUMP_DEBUG
	for (int i = 0; i < (CTXDESC_CD_DWORDS / 2); i++) {
		smmu_print("CD[%02d/%02d]: 0x%016llX\n",
			i * 8, CTXDESC_CD_DWORDS * 4, *cd);
		cd++;
	}
#endif

	return 0;
}

static int smmu_dump_io_map(u64 iova, phys_addr_t pa, u32 len, u32 prot,
			void *data)
{
	struct smmu_dump_head *head = data;
	struct smmu_dump_master_head *mhead;
	struct smmu_dump_io_map *map;

	if (!data)
		return -EINVAL;

	if ((head->total_size + sizeof(*map)) > head->dump_max)
		return -ENOSPC;

	mhead = LAST_MASTER_HEAD(head);
	map = MASTER_IO_MAP_S1_END(mhead);
	map->iova = iova;
	map->pa = pa;
	map->len = len;
	map->prot = prot;
	memcpy(map, map, sizeof(*map));

	mhead->io_map_s1_size += sizeof(*map);
	head->total_size += sizeof(*map);

#ifdef SMMU_DUMP_DEBUG
	smmu_print("va[0x%016llX] -> pa[0x%016llX] len[0x%04X]\n",
				iova, pa, len);
#endif

	return 0;
}

static void smmu_dump_finish(void *data, int status)
{
	struct smmu_dump_head *head = data;

	if (!status)
		head->status |= SMMU_DUMP_VALID;
}

int smmu_master_dump(struct smmu_master_dump_info *info)
{
	struct smmu_dump_head *head;
	struct smmu_master_handle hdl;

	if (!info->buf || info->size < sizeof(*head))
		return -EINVAL;

	head = info->buf;
	head->smmu_ver = 3;
	head->dump_max = info->size;
	head->total_size += sizeof(*head);

	hdl.smmu_name = info->smmu_name;
	hdl.smmu_master_name = info->smmu_master_name;
	hdl.init = smmu_master_dump_init,
	hdl.hdl_ste = smmu_dump_ste,
	hdl.hdl_cd = smmu_dump_cd,
	hdl.hdl_io_map_s1 = smmu_dump_io_map,
	hdl.finish = smmu_dump_finish,
	hdl.data = head;

	return smmu_master_walk(&hdl);
}
EXPORT_SYMBOL_GPL(smmu_master_dump);

static int smmu_dump(void *dump_addr, unsigned int size)
{
	struct smmu_master_dump_info info;
	int ret;

	info.smmu_name = NULL;
	info.smmu_master_name = NULL;
	info.buf = dump_addr;
	info.size = size;

	ret = smmu_master_dump(&info);

	//TODO: dump smmu queue

	return ret;
}

#ifdef SMMU_DUMP_DEBUG
void smmu_dump_debug(void)
{
	struct smmu_dump_head *head;
	struct cma *cma;
	struct page **pages, *cma_pages;
	unsigned int cnt;
	void *vaddr;
	int ret, i;

	cma = dma_contiguous_default_area;
	if (!cma) {
		pr_err("smmu dump: get default cma fail\n");
		return;
	}

	cnt = SMMU_DUMP_BUF_SIZE / PAGE_SIZE;
	cma_pages = cma_alloc(cma, cnt, 0, false);
	if (!cma_pages) {
		pr_err("smmu dump: cma alloc fail\n");
		return;
	}

	pages = kmalloc_array(cnt, sizeof(*pages), GFP_KERNEL);
	if (!pages) {
		pr_err("smmu dump: pages array alloc fail\n");
		goto OUT;
	}
	for (i = 0; i < cnt; i++)
		pages[i] = &cma_pages[i];

	vaddr = vmap(pages, cnt, VM_MAP, PAGE_KERNEL);
	if (!vaddr) {
		pr_err("smmu dump: vmap fail\n");
		goto OUT;
	}

	pr_info("smmu dump start: vaddr[0x%016llX], pa[0x%016llX], size[%u]\n",
				(u64)vaddr, __pa(vaddr), SMMU_DUMP_BUF_SIZE);

	ret = smmu_dump(vaddr, SMMU_DUMP_BUF_SIZE);
	if (ret)
		pr_err("smmu dump: fail, ret = %d\n", ret);

	flush_kernel_vmap_range(vaddr, SMMU_DUMP_BUF_SIZE);

	head = vaddr;

	pr_info("smmu dump end: vaddr[0x%016llX], pa[0x%016llX], tsize[%u]\n",
				(u64)vaddr, __pa(vaddr), head->total_size);

	if (!smmu_dump_debug_keep) {
		vunmap(vaddr);
		cma_release(cma, cma_pages, cnt);
		kfree(pages);
	}

	return;
OUT:
	if (vaddr)
		vunmap(vaddr);
	if (cma_pages)
		cma_release(cma, cma_pages, cnt);
	if(pages)
		kfree(pages);
}

static ssize_t smmu_dump_debug_read(struct file *filp, char __user *buf,
				size_t count, loff_t *pos)
{
	smmu_dump_debug();

	return 0;
}

static const struct file_operations smmu_dump_debug_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = smmu_dump_debug_read,
};
#endif

static int __init smmu_dump_init(void)
{
#ifdef SMMU_DUMP_DEBUG
	struct dentry *dir;

	dir = debugfs_lookup("smmu", NULL);
	if (!dir)
		dir = debugfs_create_dir("smmu_dump", NULL);

	if (dir)
		debugfs_create_file("dump_mem", 0444, dir, NULL,
				&smmu_dump_debug_fops);
#endif

	return register_module_dump_mem_func(smmu_dump,
				"smmu", MODU_SMMU);
}
module_init(smmu_dump_init);

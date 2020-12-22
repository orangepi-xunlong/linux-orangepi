/*
 * arch/arm/mach-sunxi/sunxi_sram.c
 *
 * Copyright(c) 2014-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: Sugar <shuge@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/genalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/bitmap.h>
#include <linux/string.h>

#include <mach/memory.h>
#include <mach/platform.h>
#include <mach/sram.h>
#include <asm/io.h>

static struct sunxi_sram sram_res[SRAM_MAX] = {
	{
#if defined(SUNXI_SRAM_A1_PBASE)
		.phys_addr = (phys_addr_t)SUNXI_SRAM_A1_PBASE,
		.virt_addr = (unsigned long)SUNXI_SRAM_A1_VBASE,
		.size = SUNXI_SRAM_A1_SIZE,
#endif
		.pool = NULL,
	},
	{
#if defined(SUNXI_SRAM_A2_PBASE)
		.phys_addr = (phys_addr_t)SUNXI_SRAM_A2_PBASE,
		.virt_addr = (unsigned long)SUNXI_SRAM_A2_VBASE,
		.size = SUNXI_SRAM_A2_SIZE,
#endif
		.pool = NULL,
	},
	{
#if defined(SUNXI_SRAM_B_PBASE)
		.phys_addr = (phys_addr_t)SUNXI_SRAM_B_PBASE,
		.virt_addr = (unsigned long)SUNXI_SRAM_B_VBASE,
		.size = SUNXI_SRAM_B_SIZE,
#endif
		.pool = NULL,
	},
};

int _sram_reserve(struct gen_pool *pool, struct gen_pool_chunk *chunk, void *data)
{
	int order = pool->min_alloc_order;
	int start_bit = 0, nbits, next_bit;
	unsigned long addr;
	size_t size;

	if (data == NULL)
		return -EINVAL;

	addr = ((struct own_info*)data)->addr;
	size = ((struct own_info*)data)->size;

	nbits = (size + (1UL << order) - 1) >> order;
	if (addr >= chunk->start_addr && addr <= chunk->end_addr) {
		if ((addr + size - 1) > chunk->end_addr) {
			WARN_ON(1);
			return -ENOMEM;
		}

		start_bit = (addr - chunk->start_addr) >> order;
		next_bit = find_next_bit(chunk->bits, start_bit + nbits, start_bit);
		if (next_bit < start_bit + nbits) {
			WARN(1, "This region have be allocated\n");
			return -ENOMEM;
		}

		bitmap_set(chunk->bits, start_bit, nbits);
		size = nbits << order;
		atomic_sub(size, &chunk->avail);
	}

	return 0;
}

int sram_reserve(const char *name, unsigned long virt_addr, size_t size)
{
	int i = 0;
	struct sunxi_sram *res = NULL;
	struct own_info *info;
	struct gen_pool_chunk *chunk;
	struct gen_pool *pool;

	for (i = 0; i < ARRAY_SIZE(sram_res); i++) {
		if (virt_addr >= sram_res[i].virt_addr &&
				virt_addr <= (sram_res[i].virt_addr + sram_res[i].size)){
			res = &sram_res[i];
			break;
		}
	}

	if (res == NULL || res->pool == NULL)
		return -ENOMEM;

	info = kzalloc(sizeof(struct own_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

	pool = res->pool;
	info->addr = virt_addr;
	info->size = size;

	rcu_read_lock();
	list_for_each_entry_rcu(chunk, &(pool)->chunks, next_chunk) {
		if (_sram_reserve(pool, chunk, (void *)info))
			continue;

		if (name != NULL)
			info->name = kstrdup(name, GFP_KERNEL);
		spin_lock(&res->lock);
		list_add_tail_rcu(&info->node, &res->own_list);
		spin_unlock(&res->lock);
		rcu_read_unlock();
		return 0;
	}
	rcu_read_unlock();

	kfree(info);
	return -ENOMEM;
}
EXPORT_SYMBOL(sram_reserve);

void *sram_alloc(size_t len, dma_addr_t *dma, sram_t type)
{
       unsigned long vaddr = 0;
       struct own_info *info;

       if (dma)
               *dma = 0;

       info = kzalloc(sizeof(struct own_info), GFP_KERNEL);
       if (info == NULL)
	       return NULL;

       if (type < SRAM_MAX && sram_res[type].pool)
	       vaddr = gen_pool_alloc(sram_res[type].pool, len);

       if (!vaddr){
	       kfree(info);
               return NULL;
       }

       if (dma)
	       *dma = sram_res[type].phys_addr + (vaddr - (unsigned long)sram_res[type].virt_addr);

       info->addr = vaddr;
       info->size = len;
       info->name = NULL;
       spin_lock(&sram_res[type].lock);
       list_add_tail_rcu(&info->node, &sram_res[type].own_list);
       spin_unlock(&sram_res[type].lock);

       return (void *)vaddr;
}

void sram_free(void *addr)
{
	int i;
	struct sunxi_sram *res = sram_res;
	struct own_info *info;

	for (i = 0; i < ARRAY_SIZE(sram_res); i++, res++) {
		if (res->pool == NULL)
			continue;

		if ((unsigned long)addr < res->virt_addr ||
				(unsigned long)addr > res->virt_addr + res->size)
			continue;

		rcu_read_lock();
		list_for_each_entry_rcu(info, &res->own_list, node) {
			if(info->addr == (unsigned long)addr) {
				rcu_read_unlock();
				goto free;
			}
		}
		rcu_read_unlock();
	}

	WARN(1, "SRAM failed to free 0x%p", addr);
	return;
free:

	gen_pool_free(res->pool, (unsigned long)addr, info->size);
	if (info->name)
		kfree(info->name);
	spin_lock(&res->lock);
	list_del_rcu(&info->node);
	spin_unlock(&res->lock);
	kfree(info);
}
EXPORT_SYMBOL(sram_free);


/*
 * REVISIT This supports CPU and DMA access to/from SRAM, but it
 * doesn't (yet?) support some other notable uses of SRAM:  as TCM
 * for data and/or instructions; and holding code needed to enter
 * and exit suspend states (while DRAM can't be used).
 */
static int __init sram_init(void)
{
	int i = 0;
	struct sunxi_sram *res = sram_res;

	for (i = 0; i < ARRAY_SIZE(sram_res); i++, res++) {
		if (res->size == 0)
			continue;

		res->pool = gen_pool_create(2, -1);
		if (res->pool == NULL)
			goto err;

		spin_lock_init(&res->lock);
		INIT_LIST_HEAD(&res->own_list);
		gen_pool_add_virt(res->pool, res->virt_addr,
				res->phys_addr, res->size, -1);
		gen_pool_set_algo(res->pool, gen_pool_best_fit, NULL);
	}

	return 0;
err:
	for (i = 0; i < ARRAY_SIZE(sram_res); i++)
		if (res->pool)
			gen_pool_destroy(res->pool);

	WARN_ON(1);
	return -ENOMEM;
}
pure_initcall(sram_init);

#ifdef CONFIG_PROC_FS
static int sram_proc_show(struct seq_file *m, void *v)
{
	size_t free = 0, used = 0;
	struct own_info *info;
	struct sunxi_sram *res = sram_res;
	int i;

#ifdef DEBUG
	struct gen_pool_chunk *chunk;
	char *buf = NULL;
	buf = kmalloc(1024, GFP_KERNEL);
#endif
	for (i = 0; i < ARRAY_SIZE(sram_res); i++, res++) {
		if (sram_res[SRAM_A1].pool == NULL)
			continue;

		free = gen_pool_avail(res->pool);
		used = res->size - free;
		seq_printf(m, "0x%08lx-0x%08lx : %u - %u\n", res->virt_addr,
				res->virt_addr + res->size, free, used);

		rcu_read_lock();
		list_for_each_entry_rcu(info, &res->own_list, node) {
			seq_printf(m, "\t0x%08lx-0x%08lx : %s\n", info->addr,
					info->addr + info->size,
					(info->name ? info->name : "null"));
		}
		rcu_read_unlock();

#ifdef DEBUG
		rcu_read_lock();
		list_for_each_entry_rcu(chunk, &res->pool->chunks, next_chunk) {
			int order = res->pool->min_alloc_order;
			int nbits;
			nbits = (chunk->end_addr - chunk->start_addr) >> order;
			if (bitmap_scnlistprintf(buf, 1024, chunk->bits, nbits))
				seq_printf(m, "Bitmap: %s\n", buf);
		}
		rcu_read_unlock();
#endif
		seq_printf(m, "\n");
	}

#ifdef DEBUG
	if (buf)
		kfree(buf);
#endif

	return 0;
}

static int sram_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sram_proc_show, NULL);
}

static const struct file_operations sram_proc_ops = {
	.open		= sram_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init sram_proc_init(void)
{
	struct proc_dir_entry *ptr;

	ptr = proc_create("sram", S_IRUGO, NULL, &sram_proc_ops);
	if (!ptr) {
		printk(KERN_WARNING "Unable to create /proc/sram\n");
		return -1;
	}
	return 0;
}
late_initcall(sram_proc_init);
#endif

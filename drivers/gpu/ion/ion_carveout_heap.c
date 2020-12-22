/*
 * drivers/gpu/ion/ion_carveout_heap.c
 *
 * Copyright (C) 2011 Google, Inc.
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
#include <linux/spinlock.h>

#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/ion.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "ion_priv.h"

#include <asm/mach/map.h>

struct ion_carveout_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	ion_phys_addr_t base;
};
#ifdef CONFIG_ARCH_SUNXI
#include <linux/seq_file.h>
static unsigned int dump_unit = SZ_64K;
extern struct ion_heap *carveout_heap;
void __dump_carveout_area(struct seq_file *s)
{
	struct ion_carveout_heap *heap = NULL;
	struct gen_pool *pool = NULL;
	struct gen_pool_chunk *chunk;
	int size, total_bits, bits_per_unit;
	int i, index, offset, tmp, busy;
	int busy_cnt = 0, free_cnt = 0;

	if (!carveout_heap) {
		seq_printf(s, "%s(%d) err: carveout_heap is NULL\n", __func__, __LINE__);
		return;
	}

	heap = container_of(carveout_heap, struct ion_carveout_heap, heap);
	pool = heap->pool;

	rcu_read_lock();
	list_for_each_entry_rcu(chunk, &pool->chunks, next_chunk) {
		size = chunk->end_addr - chunk->start_addr;
		total_bits = size >> pool->min_alloc_order;
		bits_per_unit = dump_unit >> pool->min_alloc_order;
		seq_printf(s, "%s(%d): memory 0x%08x~0x%08x, layout(+: free, -: busy, unit: 0x%08xbytes):\n",
			__func__, __LINE__, (u32)chunk->start_addr, (u32)chunk->end_addr, dump_unit);
		busy_cnt = 0;
		free_cnt = 0;
		for (i = 0, tmp = 0, busy = 0; i < total_bits; i++) {
			index = i >> 5;
			offset = i & 31;
			if (!busy && (chunk->bits[index] & (1<<offset)))
				busy = 1;
			if (++tmp == bits_per_unit) {
				busy ? (seq_printf(s, "-"), busy_cnt++) : (seq_printf(s, "+"), free_cnt);
				busy = 0;
				tmp = 0;
			}
		}
		seq_printf(s, "\n");
		seq_printf(s, "free: 0x%08x bytes, busy: 0x%08x bytes\n", free_cnt*dump_unit, busy_cnt*dump_unit);
	}
	rcu_read_unlock();

	return;
}
#endif
ion_phys_addr_t ion_carveout_allocate(struct ion_heap *heap,
				      unsigned long size,
				      unsigned long align)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);
	unsigned long offset = gen_pool_alloc(carveout_heap->pool, size);

	if (!offset) {
		pr_err("%s(%d) err: alloc 0x%08x bytes failed\n", __func__, __LINE__, (u32)size);
		return ION_CARVEOUT_ALLOCATE_FAIL;
	}
	return offset;
}

void ion_carveout_free(struct ion_heap *heap, ion_phys_addr_t addr,
		       unsigned long size)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);

	if (addr == ION_CARVEOUT_ALLOCATE_FAIL)
		return;
	gen_pool_free(carveout_heap->pool, addr, size);
}

static int ion_carveout_heap_phys(struct ion_heap *heap,
				  struct ion_buffer *buffer,
				  ion_phys_addr_t *addr, size_t *len)
{
	*addr = buffer->priv_phys;
	*len = buffer->size;
	return 0;
}

static int ion_carveout_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size, unsigned long align,
				      unsigned long flags)
{
	buffer->priv_phys = ion_carveout_allocate(heap, size, align);
	return buffer->priv_phys == ION_CARVEOUT_ALLOCATE_FAIL ? -ENOMEM : 0;
}

static void ion_carveout_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;

	ion_carveout_free(heap, buffer->priv_phys, buffer->size);
	buffer->priv_phys = ION_CARVEOUT_ALLOCATE_FAIL;
}

struct sg_table *ion_carveout_heap_map_dma(struct ion_heap *heap,
					      struct ion_buffer *buffer)
{
	struct sg_table *table;
	int ret;

	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		return ERR_PTR(-ENOMEM);
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret) {
		kfree(table);
		return ERR_PTR(ret);
	}
	sg_set_page(table->sgl, phys_to_page(buffer->priv_phys), buffer->size,
		    0);
	return table;
}

void ion_carveout_heap_unmap_dma(struct ion_heap *heap,
				 struct ion_buffer *buffer)
{
	sg_free_table(buffer->sg_table);
	kfree(buffer->sg_table); /* liugang add */
}

int ion_carveout_heap_map_user(struct ion_heap *heap, struct ion_buffer *buffer,
			       struct vm_area_struct *vma)
{
	return remap_pfn_range(vma, vma->vm_start,
			       __phys_to_pfn(buffer->priv_phys) + vma->vm_pgoff,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot); /* when user call ION_IOC_ALLOC not with ION_FLAG_CACHED, ion_mmap will
						    * change prog to pgprot_writecombine itself, so we donot need change to
						    * pgprot_writecombine here manually.
						    */
}

static struct ion_heap_ops carveout_heap_ops = {
	.allocate = ion_carveout_heap_allocate,
	.free = ion_carveout_heap_free,
	.phys = ion_carveout_heap_phys,
	.map_dma = ion_carveout_heap_map_dma,
	.unmap_dma = ion_carveout_heap_unmap_dma,
	.map_user = ion_carveout_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
};

struct ion_heap *ion_carveout_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_carveout_heap *carveout_heap;

	carveout_heap = kzalloc(sizeof(struct ion_carveout_heap), GFP_KERNEL);
	if (!carveout_heap)
		return ERR_PTR(-ENOMEM);

	carveout_heap->pool = gen_pool_create(12, -1);
	if (!carveout_heap->pool) {
		kfree(carveout_heap);
		return ERR_PTR(-ENOMEM);
	}
	carveout_heap->base = heap_data->base;
	gen_pool_add(carveout_heap->pool, carveout_heap->base, heap_data->size,
		     -1);
	carveout_heap->heap.ops = &carveout_heap_ops;
	carveout_heap->heap.type = ION_HEAP_TYPE_CARVEOUT;

	return &carveout_heap->heap;
}

void ion_carveout_heap_destroy(struct ion_heap *heap)
{
	struct ion_carveout_heap *carveout_heap =
	     container_of(heap, struct  ion_carveout_heap, heap);

	gen_pool_destroy(carveout_heap->pool);
	kfree(carveout_heap);
	carveout_heap = NULL;
}

#if defined(CONFIG_ION_SUNXI) && defined(CONFIG_PROC_FS)
#include <linux/module.h>
#include <linux/debugfs.h>
extern struct ion_heap *carveout_heap;

module_param(dump_unit, uint, 0644);
MODULE_PARM_DESC(dump_unit, "Sunxi ion dump unit(in bytes)");

int sunxi_ion_show(struct seq_file *m, void *unused)
{
	struct ion_carveout_heap *heap = NULL;
	struct gen_pool *pool = NULL;
	struct gen_pool_chunk *chunk;
	int size, total_bits, bits_per_unit;
	int i, index, offset, tmp, busy;
	int busy_cnt = 0, free_cnt = 0;

	if (!carveout_heap) {
		seq_printf(m, "%s(%d) err: carveout_heap is NULL\n", __func__, __LINE__);
		return -EPERM;
	}

	heap = container_of(carveout_heap, struct ion_carveout_heap, heap);
	pool = heap->pool;

	rcu_read_lock();
	list_for_each_entry_rcu(chunk, &pool->chunks, next_chunk) {
		size = chunk->end_addr - chunk->start_addr;
		total_bits = size >> pool->min_alloc_order;
		bits_per_unit = dump_unit >> pool->min_alloc_order;
		seq_printf(m, "%s(%d): memory 0x%08x~0x%08x, layout(+: free, -: busy, unit: 0x%08xbytes):\n",
			__func__, __LINE__, (u32)chunk->start_addr, (u32)chunk->end_addr, dump_unit);
		busy_cnt = 0;
		free_cnt = 0;
		for (i = 0, tmp = 0, busy = 0; i < total_bits; i++) {
			index = i >> 5;
			offset = i & 31;
			if (!busy && (chunk->bits[index] & (1<<offset)))
				busy = 1;
			if (++tmp == bits_per_unit) {
				busy ? (seq_printf(m, "-"), busy_cnt++) : (seq_printf(m, "+"), free_cnt++);
				busy = 0;
				tmp = 0;
			}
		}
		seq_printf(m, "\n");
		seq_printf(m, "free: 0x%08x bytes, busy: 0x%08x bytes\n", free_cnt*dump_unit, busy_cnt*dump_unit);
	}
	rcu_read_unlock();

	return 0;
}

int sunxi_ion_dump_mem(void)
{
	struct ion_carveout_heap *heap = NULL;
	struct gen_pool *pool = NULL;
	struct gen_pool_chunk *chunk;
	int size, total_bits, bits_per_unit;
	int i, index, offset, tmp, busy;
	int busy_cnt = 0, free_cnt = 0;

	if (!carveout_heap) {
		printk("%s(%d) err: carveout_heap is NULL\n", __func__, __LINE__);
		return -EPERM;
	}

	heap = container_of(carveout_heap, struct ion_carveout_heap, heap);
	pool = heap->pool;

	rcu_read_lock();
	list_for_each_entry_rcu(chunk, &pool->chunks, next_chunk) {
		size = chunk->end_addr - chunk->start_addr;
		total_bits = size >> pool->min_alloc_order;
		bits_per_unit = dump_unit >> pool->min_alloc_order;
		printk("%s(%d): memory 0x%08x~0x%08x, layout(+: free, -: busy, unit: 0x%08xbytes):\n",
			__func__, __LINE__, (u32)chunk->start_addr, (u32)chunk->end_addr, dump_unit);
		busy_cnt = 0;
		free_cnt = 0;
		for (i = 0, tmp = 0, busy = 0; i < total_bits; i++) {
			index = i >> 5;
			offset = i & 31;
			if (!busy && (chunk->bits[index] & (1<<offset)))
				busy = 1;
			if (++tmp == bits_per_unit) {
				busy ? (printk("-"), busy_cnt++) : (printk("+"), free_cnt);
				busy = 0;
				tmp = 0;
			}
		}
		printk("\n");
		printk("free: 0x%08x bytes, busy: 0x%08x bytes\n", free_cnt*dump_unit, busy_cnt*dump_unit);
	}
	rcu_read_unlock();

	return 0;
}
EXPORT_SYMBOL(sunxi_ion_dump_mem);

u32 sunxi_ion_mem_free(sunxi_pool_info *info)
{
	struct ion_carveout_heap *heap = NULL;
	struct gen_pool *pool = NULL;
	struct gen_pool_chunk *chunk;
	int size,total_bits;
	int i, index, offset;
	int free_bit =0;

	if (!info)
		return -EPERM;

	if (!carveout_heap) {
		printk("%s(%d) err: carveout_heap is NULL\n", __func__, __LINE__);
		return -EPERM;
	}

	heap = container_of(carveout_heap, struct ion_carveout_heap, heap);
	pool = heap->pool;

	rcu_read_lock();
	list_for_each_entry_rcu(chunk, &pool->chunks, next_chunk) {
		size = chunk->end_addr - chunk->start_addr;
		total_bits = size >> pool->min_alloc_order;
		info->total += size;
		for (i = 0; i < total_bits; i++) {
			index = i >> 5;
			offset = i & 31;
			if (!(chunk->bits[index] & (1<<offset)))
				free_bit++;
		}
	}
	rcu_read_unlock();

	info->total   = info->total/1024;
	info->free_kb = (free_bit<<pool->min_alloc_order)/1024;
	info->free_mb = info->free_kb/1024;
	return 0;
}
EXPORT_SYMBOL(sunxi_ion_mem_free);
#endif

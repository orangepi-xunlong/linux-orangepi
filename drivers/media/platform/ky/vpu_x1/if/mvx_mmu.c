/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/bitmap.h>
#include <linux/debugfs.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/gfp.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <asm-generic/memory_model.h>
#include "mvx_mmu.h"
#include "mvx_log_group.h"

MODULE_IMPORT_NS(DMA_BUF);

/****************************************************************************
 * Defines
 ****************************************************************************/

/* Number of bits for the physical address space. */
#define MVE_PA_BITS             40
#define MVE_PA_MASK             GENMASK_ULL(MVE_PA_BITS - 1, 0)

/* Number of bits for the virtual address space. */
#define MVE_VA_BITS             32
#define MVE_VA_MASK             GENMASK(MVE_VA_BITS - 1, 0)

/* Number of bits from the VA used to index a PTE in a page. */
#define MVE_INDEX_SHIFT         10
#define MVE_INDEX_SIZE          (1 << MVE_INDEX_SHIFT)
#define MVE_INDEX_MASK          GENMASK(MVE_INDEX_SHIFT - 1, 0)

/* Access permission defines. */
#define MVE_PTE_AP_SHIFT        0
#define MVE_PTE_AP_BITS         2
#define MVE_PTE_AP_MASK         ((1 << MVE_PTE_AP_BITS) - 1)

/* Physical address defines. */
#define MVE_PTE_PHYSADDR_SHIFT  2
#define MVE_PTE_PHYSADDR_BITS   28
#define MVE_PTE_PHYSADDR_MASK   ((1 << MVE_PTE_PHYSADDR_BITS) - 1)

/* Attributes defines. */
#define MVE_PTE_ATTR_SHIFT      30
#define MVE_PTE_ATTR_BITS       2
#define MVE_PTE_ATTR_MASK       ((1 << MVE_PTE_ATTR_BITS) - 1)

/* Number of levels for Page Table Walk. */
#define MVE_PTW_LEVELS          2

/*
 * A Linux physical page can be equal in size or larger than the MVE page size.
 * This define calculates how many MVE pages that fit in one Linux page.
 */
#define MVX_PAGES_PER_PAGE      (PAGE_SIZE / MVE_PAGE_SIZE)

/****************************************************************************
 * Types
 ****************************************************************************/

/**
 * struct mvx_mmu_dma_buf - MVX DMA buffer.
 *
 * Adds a list head to keep track of DMA buffers.
 */
struct mvx_mmu_dma_buf {
	struct list_head head;
	struct dma_buf *dmabuf;
};

/****************************************************************************
 * Static functions
 ****************************************************************************/

/**
 * get_index() - Return the PTE index for a given level.
 * @va:		Virtual address.
 * @level:	Level (L1=0, L2=1).
 *
 *                   22                  12                       0
 * +-------------------+-------------------+-----------------------+
 * |      Level 1      |      Level 2      |      Page offset      |
 * +-------------------+-------------------+-----------------------+
 */
static unsigned int get_index(const mvx_mmu_va va,
			      const unsigned int level)
{
	return (va >> (MVE_PAGE_SHIFT + (MVE_PTW_LEVELS - level - 1) *
		       MVE_INDEX_SHIFT)) & MVE_INDEX_MASK;
}

/**
 * get_offset() - Return the page offset.
 * @va:		Virtual address.
 *
 *                   22                  12                       0
 * +-------------------+-------------------+-----------------------+
 * |      Level 1      |      Level 2      |      Page offset      |
 * +-------------------+-------------------+-----------------------+
 */
static unsigned int get_offset(const mvx_mmu_va va)
{
	return va & MVE_PAGE_MASK;
}

/**
 * get_pa() - Return physical address stored in PTE.
 */
static phys_addr_t get_pa(const mvx_mmu_pte pte)
{
	return (((phys_addr_t)pte >> MVE_PTE_PHYSADDR_SHIFT) &
		MVE_PTE_PHYSADDR_MASK) << MVE_PAGE_SHIFT;
}

/* LCOV_EXCL_START */

/**
 * get_attr() - Return attributes stored in PTE.
 */
static enum mvx_mmu_attr get_attr(const mvx_mmu_pte pte)
{
	return (pte >> MVE_PTE_ATTR_SHIFT) & MVE_PTE_ATTR_MASK;
}

/**
 * get_ap() - Return access permissions stored in PTE.
 */
static enum mvx_mmu_access get_ap(const mvx_mmu_pte pte)
{
	return (pte >> MVE_PTE_AP_SHIFT) & MVE_PTE_AP_MASK;
}

/* LCOV_EXCL_STOP */

/**
 * ptw() - Perform Page Table Walk and return pointer to L2 PTE.
 * @mmu:	Pointer to MMU context.
 * @va:		Virtual address.
 * @alloc:	True if missing L2 page should be allocated.
 *
 * Return: Pointer to PTE, ERR_PTR on error.
 */
static mvx_mmu_pte *ptw(struct mvx_mmu *mmu,
			mvx_mmu_va va,
			bool alloc)
{
	phys_addr_t l2;
	mvx_mmu_pte *pte = mmu->page_table;
	unsigned int index;

	/* Level 1. */
	index = get_index(va, 0);
	l2 = get_pa(pte[index]);

	/* We should never perform a page table walk for a protected page. */
	if (test_bit(index, mmu->l2_page_is_external) != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "PTW virtual address to secure L2 page. va=0x%x.",
			      va);
		return ERR_PTR(-EINVAL);
	}

	/* Map in L2 page if it is missing. */
	if (l2 == 0) {
		if (alloc == false) {
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
				      "Missing L2 page in PTW. va=0x%x.",
				      va);
			return ERR_PTR(-EFAULT);
		}

		l2 = mvx_mmu_alloc_page(mmu->dev);
		if (l2 == 0) {
			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
				      "Failed to allocate L2 page. va=0x%x.",
				      va);
			return ERR_PTR(-ENOMEM);
		}

		pte[index] = mvx_mmu_set_pte(MVX_ATTR_PRIVATE, l2,
					     MVX_ACCESS_READ_ONLY);
		dma_sync_single_for_device(mmu->dev,
					   phys_cpu2vpu(virt_to_phys(&pte[index])),
					   sizeof(pte[index]), DMA_TO_DEVICE);
	}

	/* Level 2. */
	index = get_index(va, 1);
	pte = phys_to_virt(phys_vpu2cpu(l2));

	return &pte[index];
}

/**
 * map_page() - Map physical- to virtual address.
 * @mmu:	Pointer to MMU context.
 * @va:		MVE virtual address to map.
 * @pa:		Linux kernel physical address to map.
 * @attr:	MMU attributes.
 * @access:	MMU access permissions.
 *
 * Create new L1 and L2 entries if necessary. If mapping already exist, then
 * error is returned.
 *
 * Return: 0 on success, else error code.
 */
static int map_page(struct mvx_mmu *mmu,
		    mvx_mmu_va va,
		    phys_addr_t pa,
		    enum mvx_mmu_attr attr,
		    enum mvx_mmu_access access)
{
	mvx_mmu_pte *pte;
	phys_addr_t page;

	/* Check that both VA and PA are page aligned. */
	if ((va | pa) & MVE_PAGE_MASK) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "VA and PA must be page aligned. va=0x%x, pa=0x%llx.",
			      va, pa);
		return -EFAULT;
	}

	/* Check that VA is within valid address range. */
	if (va & ~MVE_VA_MASK) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "VA out of valid range. va=0x%x.",
			      va);
		return -EFAULT;
	}

	/* Check that PA is within valid address range. */
	if (pa & ~MVE_PA_MASK) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "PA out of valid range. pa=0x%llx.",
			      pa);
		return -EFAULT;
	}

	pte = ptw(mmu, va, true);
	if (IS_ERR(pte))
		return PTR_ERR(pte);

	/* Return error if page already exists. */
	page = get_pa(*pte);
	if (page != 0)
		return -EAGAIN;

	/* Map in physical address and flush data. */
	*pte = mvx_mmu_set_pte(attr, pa, access);
	dma_sync_single_for_device(mmu->dev, phys_cpu2vpu(virt_to_phys(pte)), sizeof(*pte),
				   DMA_TO_DEVICE);

	return 0;
}

/**
 * unmap_page() - Unmap a page from the virtual address space.
 * @mmu:	Pointer to MMU context.
 * @va:		Virtual address.
 */
static void unmap_page(struct mvx_mmu *mmu,
		       mvx_mmu_va va)
{
	mvx_mmu_pte *pte;

	pte = ptw(mmu, va, false);
	if (IS_ERR(pte))
		return;

	/* Unmap virtual address and flush data. */
	*pte = 0;
	dma_sync_single_for_device(mmu->dev, phys_cpu2vpu(virt_to_phys(pte)), sizeof(*pte),
				   DMA_TO_DEVICE);
}

/**
 * remap_page() - Remap virtual address.
 * @mmu:	Pointer to MMU context.
 * @va:		MVE virtual address to map.
 * @pa:		Linux kernel physical address to map.
 * @attr:	MMU attributes.
 * @access:	MMU access permissions.
 *
 * Return: 0 on success, else error code.
 */
static int remap_page(struct mvx_mmu *mmu,
		      mvx_mmu_va va,
		      phys_addr_t pa,
		      enum mvx_mmu_attr attr,
		      enum mvx_mmu_access access)
{
	unmap_page(mmu, va);
	return map_page(mmu, va, pa, attr, access);
}

/**
 * remap_pages() - Remap virtual address range.
 * @pages:	Pointer to pages object.
 * @oldcount:	Count before object was resized.
 *
 * Return: 0 on success, else error code.
 */
static int remap_pages(struct mvx_mmu_pages *pages,
		       size_t oldcount)
{
	int ret;

	if (pages->mmu == NULL)
		return 0;

	/* Remap pages to no access if new count is smaller than old count. */
	while (pages->count < oldcount) {
		oldcount--;

		ret = remap_page(pages->mmu,
				 pages->va + oldcount * MVE_PAGE_SIZE,
				 MVE_PAGE_SIZE, MVX_ATTR_PRIVATE,
				 MVX_ACCESS_NO);
		if (ret != 0)
			return ret;
	}

	/* Map up pages if new count is larger than old count. */
	while (pages->count > oldcount) {
		ret = remap_page(pages->mmu,
				 pages->va + oldcount * MVE_PAGE_SIZE,
				 pages->pages[oldcount], pages->attr,
				 pages->access);
		if (ret != 0)
			return ret;

		oldcount++;
	}

	return 0;
}

/**
 * mapped_count() - Check if level 2 table entries point to mmu mapped pages.
 * @pa:		Physical address of the table entry to be checked.
 *
 * Return: the number of mapped pages found.
 */
static int mapped_count(phys_addr_t pa)
{
	int count = 0;

	if (pa != 0) {
		int j;
		phys_addr_t pa2;
		mvx_mmu_pte *l2 = phys_to_virt(phys_vpu2cpu(pa));

		for (j = 0; j < MVE_INDEX_SIZE; j++) {
			pa2 = get_pa(l2[j]);
			if (pa2 != 0 && pa2 != MVE_PAGE_SIZE)
				count++;
		}
	}

	return count;
}

/**
 * get_sg_table_npages() - Count number of pages in SG table.
 * @sgt:	Pointer to scatter gather table.
 *
 * Return: Number of pages.
 */
static size_t get_sg_table_npages(struct sg_table *sgt)
{
	struct sg_page_iter piter;
	size_t count = 0;

	for_each_sg_page(sgt->sgl, &piter, sgt->nents, 0) {
		count++;
	}

	return count;
}

/**
 * append_sg_table() - Append SG table to pages object.
 * @pages:	Pointer to pages object.
 * @sgt:	Pointer to scatter gather table.
 *
 * Return: 0 on success, else error code.
 */
static int append_sg_table(struct mvx_mmu_pages *pages,
			   struct sg_table *sgt)
{
	size_t count;
	struct sg_dma_page_iter piter;

	count = get_sg_table_npages(sgt) * MVX_PAGES_PER_PAGE;

	if ((pages->count + count) > pages->capacity) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to append SG table. Pages capacity too small. count=%zu, capacity=%zu, append=%zu.",
			      pages->count, pages->capacity, count);
		return -ENOMEM;
	}

	for_each_sg_dma_page(sgt->sgl, &piter, sgt->nents, 0) {
		int j;
		phys_addr_t base;

		base = (phys_addr_t)sg_page_iter_dma_address(&piter) &
		       PAGE_MASK;

		for (j = 0; j < MVX_PAGES_PER_PAGE; ++j)
			pages->pages[pages->count++] =
				base + j * MVE_PAGE_SIZE;
	}

	return 0;
}

/**
 * stat_show() - Print debugfs info into seq-file.
 *
 * This is a callback used by debugfs subsystem.
 *
 * @s:		Seq-file
 * @v:		Unused
 * return: 0 on success, else error code.
 */
static int stat_show(struct seq_file *s,
		     void *v)
{
	struct mvx_mmu_pages *pages = s->private;

	seq_printf(s, "va: %08x\n", pages->va);
	seq_printf(s, "capacity: %zu\n", pages->capacity);
	seq_printf(s, "count: %zu\n", pages->count);

	if (pages->mmu != NULL) {
		seq_printf(s, "attr: %d\n", pages->attr);
		seq_printf(s, "access: %d\n", pages->access);
	}

	return 0;
}

/**
 * stat_open() - Open debugfs file.
 *
 * This is a callback used by debugfs subsystem.
 *
 * @inode:	Inode
 * @file:	File
 * return: 0 on success, else error code.
 */
static int stat_open(struct inode *inode,
		     struct file *file)
{
	return single_open(file, stat_show, inode->i_private);
}

/**
 * File operations for debugfs entry.
 */
static const struct file_operations stat_fops = {
	.open    = stat_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

/**
 * pages_seq_start() - Iterator over pages list.
 */
static void *pages_seq_start(struct seq_file *s,
			     loff_t *pos)
{
	struct mvx_mmu_pages *pages = s->private;

	if (*pos >= pages->count)
		return NULL;

	seq_puts(s,
		 "#Page: [  va_start -     va_end] -> [          pa_start -             pa_end]\n");
	return pos;
}

/**
 * pages_seq_start() - Iterator over pages list.
 */
static void *pages_seq_next(struct seq_file *s,
			    void *v,
			    loff_t *pos)
{
	struct mvx_mmu_pages *pages = s->private;

	++*pos;
	if (*pos >= pages->count)
		return NULL;

	return pos;
}

/**
 * pages_seq_start() - Iterator over pages list.
 */
static void pages_seq_stop(struct seq_file *s,
			   void *v)
{}

/**
 * pages_seq_start() - Iterator over pages list.
 */
static int pages_seq_show(struct seq_file *s,
			  void *v)
{
	struct mvx_mmu_pages *pages = s->private;
	loff_t pos = *(loff_t *)v;

	mvx_mmu_va va_start = pages->va + pos * MVE_PAGE_SIZE;
	mvx_mmu_va va_end = va_start + MVE_PAGE_SIZE - 1;
	phys_addr_t pa_start = pages->pages[pos];
	phys_addr_t pa_end = pa_start + MVE_PAGE_SIZE - 1;

	seq_printf(s, "%5llu: [0x%08x - 0x%08x] -> [%pap - %pap]\n", pos,
		   va_start, va_end, &pa_start, &pa_end);
	return 0;
}

/**
 * mpages_seq_ops - Callbacks used by an iterator over pages list.
 */
static const struct seq_operations pages_seq_ops = {
	.start = pages_seq_start,
	.next  = pages_seq_next,
	.stop  = pages_seq_stop,
	.show  = pages_seq_show
};

/**
 * list_open() - Callback for debugfs entry.
 */
static int list_open(struct inode *inode,
		     struct file *file)
{
	int ret;
	struct seq_file *s;

	ret = seq_open(file, &pages_seq_ops);
	if (ret != 0)
		return ret;

	s = (struct seq_file *)file->private_data;
	s->private = inode->i_private;

	return 0;
}

/**
 * File operations for a debugfs entry.
 */
static const struct file_operations list_fops = {
	.open    = list_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_mmu_construct(struct mvx_mmu *mmu,
		      struct device *dev)
{
	phys_addr_t page_table;

	mmu->dev = dev;

	/* Allocate Page Table Base (the L1 table). */
	page_table = mvx_mmu_alloc_page(dev);
	if (page_table == 0)
		return -ENOMEM;

	mmu->page_table = phys_to_virt(phys_vpu2cpu(page_table));

	return 0;
}

void mvx_mmu_destruct(struct mvx_mmu *mmu)
{
	mvx_mmu_pte *pte = mmu->page_table;
	phys_addr_t pa;
	int i;
	int count = 0;

	for (i = 0; i < MVE_INDEX_SIZE; i++) {
		pa = get_pa(pte[i]);

		/* Only free pages we have allocated ourselves. */
		if (test_bit(i, mmu->l2_page_is_external) == 0) {
			count += mapped_count(pa);
			mvx_mmu_free_page(mmu->dev, pa);
		}
	}

	pa = virt_to_phys(mmu->page_table);
	mvx_mmu_free_page(mmu->dev, phys_cpu2vpu(pa));

	WARN_ON(count > 0);
}

phys_addr_t mvx_mmu_alloc_page(struct device *dev)
{
    struct page *page;
    phys_addr_t pa;
    dma_addr_t dma_handle;
    int retry = 20;

    do {
        page = alloc_page(GFP_KERNEL | __GFP_ZERO | __GFP_RETRY_MAYFAIL);
    } while(retry-- && page == NULL);

    if (page == NULL)
        return 0;

    dma_handle = dma_map_page(dev, page, 0, PAGE_SIZE, DMA_BIDIRECTIONAL);
    if (dma_mapping_error(dev, dma_handle) != 0) {
        MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
                "Cannot map page to DMA address space. page=%p.",
                page);
        goto free_page;
    }

    pa = (phys_addr_t)dma_handle;

    dma_sync_single_for_device(dev, pa, PAGE_SIZE, DMA_TO_DEVICE);

    return pa;

free_page:
    __free_page(page);
    return 0;
}

phys_addr_t mvx_mmu_dma_alloc_coherent(struct device *dev, void** data)
{
	phys_addr_t pa;
	dma_addr_t dma_handle;
	int retry = 20;
	void *virt_addr;

	do {
		virt_addr = dma_alloc_coherent(dev, PAGE_SIZE, &dma_handle, GFP_KERNEL | __GFP_ZERO | __GFP_RETRY_MAYFAIL);
	} while(retry-- && virt_addr == NULL);

	if (virt_addr == NULL) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
				"dma alloc coherent buffer faild. retry=%d", retry);
		return 0;
	}

	pa = (phys_addr_t)dma_handle;

	*data = virt_addr;

	return pa;
}

void mvx_mmu_dma_free_coherent(struct device *dev,
		phys_addr_t pa, void* data)
{
	if (pa == 0)
		return;

	dma_free_coherent(dev, PAGE_SIZE, data, (dma_addr_t)pa);
}

void mvx_mmu_free_contiguous_pages(struct device *dev, phys_addr_t pa, size_t npages)
{
	struct page *page;

	if (pa == 0)
		return;

	page = phys_to_page(phys_vpu2cpu(pa));

	dma_unmap_page(dev, pa, npages << PAGE_SHIFT, DMA_BIDIRECTIONAL);
	__free_pages(page, get_order(npages << PAGE_SHIFT));
}

phys_addr_t mvx_mmu_alloc_contiguous_pages(struct device *dev, size_t npages)
{
	struct page *page;
	phys_addr_t pa;
	dma_addr_t dma_handle;
	size_t size = (npages << PAGE_SHIFT);
        int retry = 20;

        do {
            page = alloc_pages(GFP_KERNEL | __GFP_ZERO | __GFP_RETRY_MAYFAIL, get_order(size));
        } while(retry-- && page == NULL);

        if (page == NULL)
            return 0;

	dma_handle = dma_map_page(dev, page, 0, size, DMA_BIDIRECTIONAL);
	if (dma_mapping_error(dev, dma_handle) != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Cannot map page to DMA address space. page=%p.",
			      page);
		goto free_pages;
	}

	pa = (phys_addr_t)dma_handle;

	dma_sync_single_for_device(dev, pa, size, DMA_TO_DEVICE);

	return pa;

free_pages:
	__free_pages(page, get_order(size));
	return 0;
}

void mvx_mmu_free_page(struct device *dev,
		       phys_addr_t pa)
{
	struct page *page;

	if (pa == 0)
		return;

	page = phys_to_page(phys_vpu2cpu(pa));

	dma_unmap_page(dev, pa, PAGE_SIZE, DMA_BIDIRECTIONAL);
	__free_page(page);
}

struct mvx_mmu_pages *mvx_mmu_alloc_pages(struct device *dev,
					  size_t count,
					  size_t capacity)
{
	struct mvx_mmu_pages *pages;
	int ret;

	count = roundup(count, MVX_PAGES_PER_PAGE);
	capacity = roundup(capacity, MVX_PAGES_PER_PAGE);
	capacity = max(count, capacity);

        pages = vmalloc(sizeof(*pages) + sizeof(phys_addr_t) * capacity);
	if (pages == NULL)
		return ERR_PTR(-ENOMEM);

        memset(pages, 0, sizeof(*pages) + sizeof(phys_addr_t) * capacity);
	pages->dev = dev;
	pages->capacity = capacity;
	INIT_LIST_HEAD(&pages->dmabuf);

	for (pages->count = 0; pages->count < count; ) {
		phys_addr_t page;
		unsigned int i;

		/*
		 * Allocate a Linux page. It will typically be of the same size
		 * as the MVE page, but could also be larger.
		 */
		page = mvx_mmu_alloc_page(dev);
		if (page == 0) {
			ret = -ENOMEM;
			goto release_pages;
		}

		/*
		 * If the Linux page is larger than the MVE page, then
		 * we iterate and add physical addresses with an offset from
		 * the Linux page.
		 */
		for (i = 0; i < MVX_PAGES_PER_PAGE; i++)
			pages->pages[pages->count++] =
				page + i * MVE_PAGE_SIZE;
	}

	return pages;

release_pages:
	mvx_mmu_free_pages(pages);

	return ERR_PTR(ret);
}

struct mvx_mmu_pages *mvx_mmu_alloc_pages_sg(struct device *dev,
					     struct sg_table *sgt,
					     size_t capacity)
{
	struct mvx_mmu_pages *pages;
	size_t count;
	int ret;

	count = get_sg_table_npages(sgt) * MVX_PAGES_PER_PAGE;
	capacity = roundup(capacity, MVX_PAGES_PER_PAGE);
	capacity = max(count, capacity);

        pages = vmalloc(sizeof(*pages) + sizeof(phys_addr_t) * capacity);
	if (pages == NULL)
		return ERR_PTR(-ENOMEM);

        memset(pages, 0, sizeof(*pages) + sizeof(phys_addr_t) * capacity);
	pages->dev = dev;
	pages->capacity = capacity;
	pages->is_external = true;
	pages->offset = sgt->sgl != NULL ? sgt->sgl->offset : 0;
	INIT_LIST_HEAD(&pages->dmabuf);

	ret = append_sg_table(pages, sgt);
	if (ret != 0) {
                vfree(pages);
		return ERR_PTR(ret);
	}

	return pages;
}

struct mvx_mmu_pages *mvx_mmu_alloc_pages_dma_buf(struct device *dev,
						  struct dma_buf *dmabuf,
						  size_t capacity)
{
	struct mvx_mmu_pages *pages;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct mvx_mmu_dma_buf *mbuf;

	attach = dma_buf_attach(dmabuf, dev);
	if (IS_ERR(attach)) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to attach DMA buffer.");
		return (struct mvx_mmu_pages *)attach;
	}

	sgt = dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to get SG table from DMA buffer.");
		pages = (struct mvx_mmu_pages *)sgt;
		goto detach;
	}

	pages = mvx_mmu_alloc_pages_sg(dev, sgt, capacity);
	if (IS_ERR(pages))
		goto unmap;

        mbuf = vmalloc(sizeof(*mbuf));
	if (mbuf == NULL) {
		mvx_mmu_free_pages(pages);
		pages = ERR_PTR(-ENOMEM);
		goto unmap;
	}

        memset(mbuf, 0, sizeof(*mbuf));
	mbuf->dmabuf = dmabuf;
	list_add_tail(&mbuf->head, &pages->dmabuf);

unmap:
	dma_buf_unmap_attachment_unlocked(attach, sgt, DMA_BIDIRECTIONAL);

detach:
	dma_buf_detach(dmabuf, attach);

	return pages;
}

int mvx_mmu_pages_append_dma_buf(struct mvx_mmu_pages *pages,
				 struct dma_buf *dmabuf)
{
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct mvx_mmu_dma_buf *mbuf;
	size_t oldcount = pages->count;
	int ret;

	if (pages->is_external == false) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Can't append DMA buffer to internal pages object.");
		return -EINVAL;
	}

	attach = dma_buf_attach(dmabuf, pages->dev);
	if (IS_ERR(attach)) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to attach DMA buffer.");
		return PTR_ERR(attach);
	}

	sgt = dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to get SG table from DMA buffer.");
		ret = PTR_ERR(sgt);
		goto detach;
	}

	ret = append_sg_table(pages, sgt);
	if (ret != 0)
		goto unmap;

	ret = remap_pages(pages, oldcount);
	if (ret != 0)
		goto unmap;

        mbuf = vmalloc(sizeof(*mbuf));
	if (mbuf == NULL) {
		ret = -ENOMEM;
		goto unmap;
	}

        memset(mbuf, 0, sizeof(*mbuf));
	mbuf->dmabuf = dmabuf;
	list_add_tail(&mbuf->head, &pages->dmabuf);

unmap:
	dma_buf_unmap_attachment_unlocked(attach, sgt, DMA_BIDIRECTIONAL);

detach:
	dma_buf_detach(dmabuf, attach);

	return ret;
}

int mvx_mmu_resize_pages(struct mvx_mmu_pages *pages,
			 size_t npages)
{
	size_t oldcount = pages->count;

	if (pages->is_external != false) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "MMU with externally managed pages cannot be resized.");
		return -EINVAL;
	}

	npages = roundup(npages, MVX_PAGES_PER_PAGE);

	if (npages > pages->capacity) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "New MMU pages size is larger than capacity. npages=%zu, capacity=%zu.",
			      npages, pages->capacity);
		return -ENOMEM;
	}

	/* Free pages if npage is smaller than allocated pages. */
	while (pages->count > npages) {
		pages->count--;

		if ((pages->count % MVX_PAGES_PER_PAGE) == 0)
			mvx_mmu_free_page(pages->dev,
					  pages->pages[pages->count]);

		pages->pages[pages->count] = 0;
	}

	/* Allocate pages if npage is larger than allocated pages. */
	while (pages->count < npages) {
		phys_addr_t page;
		unsigned int i;

		page = mvx_mmu_alloc_page(pages->dev);
		if (page == 0)
			return -ENOMEM;

		for (i = 0; i < MVX_PAGES_PER_PAGE; i++)
			pages->pages[pages->count++] =
				page + i * MVE_PAGE_SIZE;
	}

	return remap_pages(pages, oldcount);
}

void mvx_mmu_free_pages(struct mvx_mmu_pages *pages)
{
	struct mvx_mmu_dma_buf *mbuf;
	struct mvx_mmu_dma_buf *tmp;
	unsigned int i;

	mvx_mmu_unmap_pages(pages);

	if (pages->is_external == false)
		for (i = 0; i < pages->count; i += MVX_PAGES_PER_PAGE)
			mvx_mmu_free_page(pages->dev, pages->pages[i]);

	list_for_each_entry_safe(mbuf, tmp, &pages->dmabuf, head) {
		dma_buf_put(mbuf->dmabuf);
                vfree(mbuf);
	}

        vfree(pages);
}

size_t mvx_mmu_size_pages(struct mvx_mmu_pages *pages)
{
	return pages->count * MVE_PAGE_SIZE;
}

int mvx_mmu_synch_pages(struct mvx_mmu_pages *pages,
			enum dma_data_direction dir, int page_off, int page_count)
{
	size_t i;
	if (page_off + page_count > pages->count)
	{
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			"Illegal mmu sync offset/size (%d/%d).",
			page_off, page_count);
		page_off = 0;
		page_count = pages->count;
	}
	if (dir == DMA_FROM_DEVICE) {
		for (i = 0; i < page_count; i += MVX_PAGES_PER_PAGE)
			dma_sync_single_for_cpu(pages->dev, pages->pages[page_off+i],
						PAGE_SIZE, DMA_FROM_DEVICE);
	} else if (dir == DMA_TO_DEVICE) {
		for (i = 0; i < page_count; i += MVX_PAGES_PER_PAGE)
			dma_sync_single_for_device(pages->dev, pages->pages[page_off+i],
						   PAGE_SIZE, DMA_TO_DEVICE);
	} else {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Unsupported MMU flush direction. dir=%u.",
			      dir);
		return -EINVAL;
	}

	return 0;
}

int mvx_mmu_map_pages(struct mvx_mmu *mmu,
		      mvx_mmu_va va,
		      struct mvx_mmu_pages *pages,
		      enum mvx_mmu_attr attr,
		      enum mvx_mmu_access access)
{
	size_t i;
	int ret;

	/* Map the allocated pages. */
	for (i = 0; i < pages->count; i++) {
		ret = map_page(mmu, va + i * MVE_PAGE_SIZE, pages->pages[i],
			       attr, access);
		if (ret != 0)
			goto unmap_pages;
	}

	/*
	 * Reserve the rest of the address range. Adding a dummy page with
	 * physical address 'PAGE_SIZE' should not lead to memory corruption,
	 * because the page is marked as 'no access'.
	 */
	for (; i < pages->capacity; i++) {
		ret = map_page(mmu, va + i * MVE_PAGE_SIZE, MVE_PAGE_SIZE,
			       MVX_ATTR_PRIVATE, MVX_ACCESS_NO);
		if (ret != 0)
			goto unmap_pages;
	}

	pages->mmu = mmu;
	pages->va = va;
	pages->attr = attr;
	pages->access = access;

	return 0;

unmap_pages:
	while (i-- > 0)
		unmap_page(mmu, va + i * MVE_PAGE_SIZE);

	return ret;
}

void mvx_mmu_unmap_pages(struct mvx_mmu_pages *pages)
{
	size_t i;

	if (pages->mmu == NULL)
		return;

	for (i = 0; i < pages->capacity; i++)
		unmap_page(pages->mmu, pages->va + i * MVE_PAGE_SIZE);

	pages->mmu = NULL;
	pages->va = 0;
}

int mvx_mmu_map_pa(struct mvx_mmu *mmu,
		   mvx_mmu_va va,
		   phys_addr_t pa,
		   size_t size,
		   enum mvx_mmu_attr attr,
		   enum mvx_mmu_access access)
{
	int ret;
	size_t offset;

	for (offset = 0; offset < size; offset += MVE_PAGE_SIZE) {
		ret = map_page(mmu, va + offset, pa + offset,
			       attr, access);
		if (ret != 0)
			goto unmap_pages;
	}

	return 0;

unmap_pages:
	/* Unroll mapped pages. */
	while (offset > 0) {
		offset -= MVE_PAGE_SIZE;
		unmap_page(mmu, va + offset);
	}

	return ret;
}

int mvx_mmu_map_l2(struct mvx_mmu *mmu,
		   mvx_mmu_va va,
		   phys_addr_t pa)
{
	phys_addr_t l2;
	mvx_mmu_pte *pte = mmu->page_table;
	unsigned int index;

	/* Level 1. */
	index = get_index(va, 0);
	l2 = get_pa(pte[index]);

	if (l2 != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR,
			      "Failed to map L2 page. Page already exists.");
		return -EINVAL;
	}

	set_bit(index, mmu->l2_page_is_external);

	pte[index] = mvx_mmu_set_pte(MVX_ATTR_PRIVATE, pa,
				     MVX_ACCESS_READ_ONLY);
	dma_sync_single_for_device(mmu->dev,
				   phys_cpu2vpu(virt_to_phys(&pte[index])),
				   sizeof(pte[index]), DMA_TO_DEVICE);

	return 0;
}

void mvx_mmu_unmap_va(struct mvx_mmu *mmu,
		      mvx_mmu_va va,
		      size_t size)
{
	size_t offset;

	for (offset = 0; offset < size; offset += MVE_PAGE_SIZE)
		unmap_page(mmu, va + offset);
}

int mvx_mmu_va_to_pa(struct mvx_mmu *mmu,
		     mvx_mmu_va va,
		     phys_addr_t *pa)
{
	mvx_mmu_pte *pte;
	phys_addr_t page;

	pte = ptw(mmu, va, false);
	if (IS_ERR(pte))
		return PTR_ERR(pte);

	page = get_pa(*pte);
	if (page == 0)
		return -EFAULT;

	*pa = page | get_offset(va);

	return 0;
}

/* LCOV_EXCL_START */
int mvx_mmu_read(struct mvx_mmu *mmu,
		 mvx_mmu_va va,
		 void *data,
		 size_t size)
{
	mvx_mmu_va end = va + size;

	while (va < end) {
		int ret;
		size_t n;
		phys_addr_t pa = 0;
		void *src;

		/* Calculate number of bytes to be copied. */
		n = min(end - va, MVE_PAGE_SIZE - (va & MVE_PAGE_MASK));

		/* Translate virtual- to physical address. */
		ret = mvx_mmu_va_to_pa(mmu, va, &pa);
		if (ret != 0)
			return ret;

		/* Invalidate the data range. */
		dma_sync_single_for_cpu(mmu->dev, pa, n, DMA_FROM_DEVICE);

		/* Convert from physical to Linux logical address. */
		src = phys_to_virt(phys_vpu2cpu(pa));
		memcpy(data, src, n);

		va += n;
		data += n;
	}

	return 0;
}

/* LCOV_EXCL_STOP */

int mvx_mmu_write(struct mvx_mmu *mmu,
		  mvx_mmu_va va,
		  const void *data,
		  size_t size)
{
	mvx_mmu_va end = va + size;

	while (va < end) {
		int ret;
		size_t n;
		phys_addr_t pa = 0;
		void *dst;

		/* Calculate number of bytes to be copied. */
		n = min(end - va, MVE_PAGE_SIZE - (va & MVE_PAGE_MASK));

		/* Translate virtual- to physical address. */
		ret = mvx_mmu_va_to_pa(mmu, va, &pa);
		if (ret != 0)
			return ret;

		/* Convert from physical to Linux logical address. */
		dst = phys_to_virt(phys_vpu2cpu(pa));
		memcpy(dst, data, n);

		/* Flush the data to memory. */
		dma_sync_single_for_device(mmu->dev, pa, n, DMA_TO_DEVICE);

		va += n;
		data += n;
	}

	return 0;
}

mvx_mmu_pte mvx_mmu_set_pte(enum mvx_mmu_attr attr,
			    phys_addr_t pa,
			    enum mvx_mmu_access access)
{
	return (attr << MVE_PTE_ATTR_SHIFT) |
	       ((pa >> MVE_PAGE_SHIFT) << MVE_PTE_PHYSADDR_SHIFT) |
	       (access << MVE_PTE_AP_SHIFT);
}

/* LCOV_EXCL_START */
void mvx_mmu_print(struct mvx_mmu *mmu)
{
	unsigned int i;
	mvx_mmu_pte *l1 = mmu->page_table;

	for (i = 0; i < MVE_INDEX_SIZE; i++) {
		phys_addr_t pa = get_pa(l1[i]);
		unsigned int j;

		if (pa != 0) {
			mvx_mmu_pte *l2 = phys_to_virt(phys_vpu2cpu(pa));

			MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
				      "%-4u: PA=0x%llx, ATTR=%u, ACC=%u",
				      i, pa, get_attr(l1[i]), get_ap(l1[i]));

			for (j = 0; j < MVE_INDEX_SIZE; j++) {
				pa = get_pa(l2[j]);
				if (pa != 0) {
					mvx_mmu_va va;

					va = (i << (MVE_INDEX_SHIFT +
						    MVE_PAGE_SHIFT)) |
					     (j << MVE_PAGE_SHIFT);
					MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
						      "|------ %-4u: VA=0x%08x, PA=0x%llx, ATTR=%u, ACC=%u",
						      j,
						      va,
						      pa,
						      get_attr(l2[j]),
						      get_ap(l2[j]));
				}
			}
		}
	}
}

/* LCOV_EXCL_STOP */

int mvx_mmu_pages_debugfs_init(struct mvx_mmu_pages *pages,
			       char *name,
			       struct dentry *parent)
{
	struct dentry *dpages;
	struct dentry *dentry;

	dpages = debugfs_create_dir(name, parent);
	if (IS_ERR_OR_NULL(dpages))
		return -ENOMEM;

	dentry = debugfs_create_file("stat", 0400, dpages, pages,
				     &stat_fops);
	if (IS_ERR_OR_NULL(dentry))
		return -ENOMEM;

	dentry = debugfs_create_file("list", 0400, dpages, pages,
				     &list_fops);
	if (IS_ERR_OR_NULL(dentry))
		return -ENOMEM;

	return 0;
}

unsigned long phys_vpu2cpu(unsigned long phys_addr)
{
	if (phys_addr >= 0x80000000UL) {
		phys_addr += 0x80000000UL;
	}
	return phys_addr;
}

unsigned long phys_cpu2vpu(unsigned long phys_addr)
{
	if (phys_addr >= 0x100000000UL) {
		phys_addr -= 0x80000000UL;
	}
	return phys_addr;
}

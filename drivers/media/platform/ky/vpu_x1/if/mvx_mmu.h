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

#ifndef _MVX_MMU_H_
#define _MVX_MMU_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/dma-mapping.h>
#include <linux/hashtable.h>
#include <linux/types.h>

/****************************************************************************
 * Defines
 ****************************************************************************/

/* Page size in bits. 2^12 = 4kB. */
#define MVE_PAGE_SHIFT          12
#define MVE_PAGE_SIZE           (1 << MVE_PAGE_SHIFT)
#define MVE_PAGE_MASK           (MVE_PAGE_SIZE - 1)

/* Number of page table entries per page. */
#define MVE_PAGE_PTE_PER_PAGE   (MVE_PAGE_SIZE / sizeof(mvx_mmu_pte))

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;
struct dma_buf;
struct mvx_mmu;
struct page;

/**
 * typedef mvx_mmu_va - 32 bit virtual address.
 *
 * This is the address the firmware/MVE will use.
 */
typedef uint32_t mvx_mmu_va;

/**
 * typedef mvx_mmu_pte - Page table entry.
 *
 * A PTE pointer should always point at a Linux kernel virtual address.
 *
 * AT - Attribute.
 * PA - Physical address.
 * AP - Access permission.
 *
 *   30                                                       2   0
 * +---+-------------------------------------------------------+---+
 * | AT|                        PA 39:12                       | AP|
 * +---+-------------------------------------------------------+---+
 */
typedef uint32_t mvx_mmu_pte;

enum mvx_mmu_attr {
	MVX_ATTR_PRIVATE   = 0,
	MVX_ATTR_REFFRAME  = 1,
	MVX_ATTR_SHARED_RO = 2,
	MVX_ATTR_SHARED_RW = 3
};

enum mvx_mmu_access {
	MVX_ACCESS_NO         = 0,
	MVX_ACCESS_READ_ONLY  = 1,
	MVX_ACCESS_EXECUTABLE = 2,
	MVX_ACCESS_READ_WRITE = 3
};

/**
 * struct mvx_mmu_pages - Structure used to allocate an array of pages.
 * @dev:	Pointer to device.
 * @node:	Hash table node. Used to keep track of allocated pages objects.
 * @mmu:	Pointer to MMU instance.
 * @va:		MVE virtual address. Set to 0 if objects is unmapped.
 * @offset:	Offset from mapped VA to where the data begins.
 * @attr:	Page table attributes.
 * @access:	Page table access.
 * @capacity:	Maximum number of MVE pages this object can hold.
 * @count:	Current number of allocated pages.
 * @is_external:If the physical pages have been externally allocated.
 * @dmabuf:	List of DMA buffers.
 * @pages:	Array of pages.
 */
struct mvx_mmu_pages {
	struct device *dev;
	struct hlist_node node;
	struct mvx_mmu *mmu;
	mvx_mmu_va va;
	size_t offset;
	enum mvx_mmu_attr attr;
	enum mvx_mmu_access access;
	size_t capacity;
	size_t count;
	bool is_external;
	struct list_head dmabuf;
	DECLARE_FLEX_ARRAY(phys_addr_t, pages);
};

/**
 * struct mvx_mmu - MMU context.
 * @dev:	Pointer to device.
 * @page_table:	Virtual address to L1 page.
 * @l2_page_is_external: Bitmap of which L2 pages that have been mapped
 *                       externally.
 */
struct mvx_mmu {
	struct device *dev;
	mvx_mmu_pte *page_table;
	DECLARE_BITMAP(l2_page_is_external, MVE_PAGE_PTE_PER_PAGE);
};

/****************************************************************************
 * Static functions
 ****************************************************************************/

#ifndef phys_to_page

/**
 * phys_to_page() - Convert a physical address to a pointer to a page.
 * @pa:		Physical address.
 *
 * Return: Pointer to page struct.
 */
static inline struct page *phys_to_page(unsigned long pa)
{
	return pfn_to_page(__phys_to_pfn(pa));
}

#endif

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_mmu_construct() - Construct the MMU object.
 * @mmu:	Pointer to MMU object.
 * @dev:	Pointer to device.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_construct(struct mvx_mmu *mmu,
		      struct device *dev);

/**
 * mvx_mmu_destruct() - Destroy the MMU object.
 * @mmu:	Pointer to MMU object.
 */
void mvx_mmu_destruct(struct mvx_mmu *mmu);

/**
 * mvx_mmu_alloc_contiguous_pages() - Allocate contiguous pages.
 * dev:		Pointer to device.
 * npages:	Number of pages to allocate.
 * Return: Physical page address on success, else 0.
 */
phys_addr_t mvx_mmu_alloc_contiguous_pages(struct device *dev, size_t npages);

/*
 * mvx_mmu_free_contiguous_pages() - Free contiguous pages.
 *
 * dev:		Pointer to device.
 * pa:		Physical page address or 0.
 * npages:	Number of pages to free.
 */
void mvx_mmu_free_contiguous_pages(struct device *dev, phys_addr_t pa, size_t npages);

/**
 * mvx_mmu_alloc_page() - Allocate one page.
 * dev:		Pointer to device.
 *
 * Return: Physical page address on success, else 0.
 */
phys_addr_t mvx_mmu_alloc_page(struct device *dev);

/*
 * mvx_mmu_free_page() - Free one page.
 *
 * dev:		Pointer to device.
 * pa:		Physical page address or 0.
 */
void mvx_mmu_free_page(struct device *dev,
		       phys_addr_t pa);

/**
 * mvx_mmu_dma_alloc_coherent() - Allocate one page for non-cacheable attribute.
 *
 * dev:	   Pointer to device.
 * data:   Virtual address for cpu access.
 * Return: Physical page address or 0.
 */
phys_addr_t mvx_mmu_dma_alloc_coherent(struct device *dev, void** data);

/**
 * mvx_mmu_dma_free_coherent() - Free one page for non-cacheable attribute.
 *
 * dev:	   Pointer to device.
 * pa:     Physical page address or 0.
 * data:   Virtual address for cpu access.
 */
void mvx_mmu_dma_free_coherent(struct device *dev, phys_addr_t pa, void* data);

/**
 * mvx_mmu_alloc_pages() - Allocate array of pages.
 * @dev:	Pointer to device.
 * @npages	Number of pages to allocate.
 * @capacity:	Maximum number of pages this allocation can be resized
 *              to. If this value is 0 or smaller than npages, then it will be
 *              set to npages.
 *
 * Pages are not guaranteed to be physically continuous.
 *
 * Return: Valid pointer on success, else ERR_PTR.
 */
struct mvx_mmu_pages *mvx_mmu_alloc_pages(struct device *dev,
					  size_t npages,
					  size_t capacity);

/**
 * mvx_mmu_alloc_pages_sg() - Allocate array of pages from SG table.
 * @dev:	Pointer to device.
 * @sgt:	Scatter-gatter table with pre-allocated memory pages.
 * @capacity:	Maximum number of pages this allocation can be resized
 *              to. If this value is 0 or smaller than number of pages
 *              in scatter gather table, then it will be rounded up to
 *              to SG table size.
 *
 * Pages are not guaranteed to be physically continuous.
 *
 * Return: Valid pointer on success, else ERR_PTR.
 */
struct mvx_mmu_pages *mvx_mmu_alloc_pages_sg(struct device *dev,
					     struct sg_table *sgt,
					     size_t capacity);

/**
 * mvx_mmu_alloc_pages_dma_buf() - Allocate pages object from DMA buffer.
 * @dev:	Pointer to device.
 * @dma_buf:	Pointer to DMA buffer.
 * @capacity:	Maximum number of pages this allocation can be resized
 *              to. If this value is 0 or smaller than number of pages
 *              in DMA buffer, then it will be rounded up to DMA buffer
 *              size.
 *
 * The pages object will take ownership of the DMA buffer and call
 * dma_put_buf() when the pages object is destructed.
 *
 * Return: Valid pointer on success, else ERR_PTR.
 */
struct mvx_mmu_pages *mvx_mmu_alloc_pages_dma_buf(struct device *dev,
						  struct dma_buf *dmabuf,
						  size_t capacity);

/**
 * mvx_mmu_pages_append_dma_buf() - Append DMA buffer to pages object.
 * @pages:	Pointer to pages object.
 * @dma_buf:	Pointer to DMA buffer.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_pages_append_dma_buf(struct mvx_mmu_pages *pages,
				 struct dma_buf *dmabuf);

/**
 * mvx_mmu_resize_pages() - Resize the page allocation.
 * @pages:	Pointer to pages object.
 * @npages:	Number of pages to allocate.
 *
 * If the number of pages is smaller, then pages will be freed.
 *
 * If the number of pages is larger, then additional memory will be allocated.
 * The already allocates pages will keep their physical addresses.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_resize_pages(struct mvx_mmu_pages *pages,
			 size_t npages);

/**
 * mvx_mmu_free_pages() - Free pages.
 * @pages:	Pointer to pages object.
 */
void mvx_mmu_free_pages(struct mvx_mmu_pages *pages);

/**
 * mvx_mmu_size_pages() - Get number of allocated bytes.
 * @pages:	Pointer to pages object.
 *
 * Return: Size in bytes of pages.
 */
size_t mvx_mmu_size_pages(struct mvx_mmu_pages *pages);

/**
 * mvx_buffer_synch() - Synch data caches.
 * @pages:	Pointer to pages object.
 * @dir:	Which direction to synch.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_synch_pages(struct mvx_mmu_pages *pages,
			enum dma_data_direction dir, int page_off, int page_count);

/**
 * mvx_mmu_map_pages() - Map an array of pages to a virtual address.
 * @mmu:	Pointer to MMU object.
 * @va:		Virtual address.
 * @pages:	Pointer to pages object.
 * @attr:	Bus attributes.
 * @access:	Access permission.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_map_pages(struct mvx_mmu *mmu,
		      mvx_mmu_va va,
		      struct mvx_mmu_pages *pages,
		      enum mvx_mmu_attr attr,
		      enum mvx_mmu_access access);

/**
 * mvx_mmu_unmap_pages() - Unmap pages object.
 * @pages:	Pointer to pages object.
 */
void mvx_mmu_unmap_pages(struct mvx_mmu_pages *pages);

/**
 * mvx_mmu_map_pa() - Map a physical- to a virtual address.
 * @mmu:	Pointer to MMU object.
 * @va:		Virtual address.
 * @pa:		Physical address.
 * @size:	Size of area to map.
 * @attr:	Bus attributes.
 * @access:	Access permission.
 *
 * Both the VA and PA must be page aligned.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_map_pa(struct mvx_mmu *mmu,
		   mvx_mmu_va va,
		   phys_addr_t pa,
		   size_t size,
		   enum mvx_mmu_attr attr,
		   enum mvx_mmu_access access);

/**
 * mvx_mmu_map_l2() - Map a L2 page.
 * @mmu:	Pointer to MMU object.
 * @va:		Virtual address.
 * @pa:		Physical address.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_map_l2(struct mvx_mmu *mmu,
		   mvx_mmu_va va,
		   phys_addr_t pa);

/**
 * mvx_mmu_unmap_va() - Unmap a virtual address range.
 * @mmu:	Pointer to MMU object.
 * @va:		Virtual address.
 * @size:	Size of area to unmap.
 */
void mvx_mmu_unmap_va(struct mvx_mmu *mmu,
		      mvx_mmu_va va,
		      size_t size);

/**
 * mvx_mmu_va_to_pa() - Map a virtual- to a physical address.
 * @mmu:	Pointer to MMU object.
 * @va:		Virtual address.
 * @pa:		Pointer to physical address.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_va_to_pa(struct mvx_mmu *mmu,
		     mvx_mmu_va va,
		     phys_addr_t *pa);

/**
 * mvx_mmu_read() - Read size bytes from virtual address.
 * @mmu:	Pointer to MMU object.
 * @va:		Source virtual address.
 * @data:	Pointer to destination data.
 * @size:	Number of bytes to copy.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_read(struct mvx_mmu *mmu,
		 mvx_mmu_va va,
		 void *data,
		 size_t size);

/**
 * mvx_mmu_write() - Write size bytes to virtual address.
 * @mmu:	Pointer to MMU object.
 * @va:		Destination virtual address.
 * @data:	Pointer to source data.
 * @size:	Number of bytes to copy.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_write(struct mvx_mmu *mmu,
		  mvx_mmu_va va,
		  const void *data,
		  size_t size);

/**
 * mvx_mmu_set_pte() - Construct PTE and return PTE value.
 * @attr:	Bus attributes.
 * @pa:		Physical address.
 * @access:	Access permission.
 *
 * Return: Page table entry.
 */
mvx_mmu_pte mvx_mmu_set_pte(enum mvx_mmu_attr attr,
			    phys_addr_t pa,
			    enum mvx_mmu_access access);

/**
 * mvx_mmu_print() - Print the MMU table.
 * @mmu:	Pointer to MMU object.
 */
void mvx_mmu_print(struct mvx_mmu *mmu);

/**
 * mvx_mmu_pages_debugfs_init() - Init debugfs entry.
 * @pages:	Pointer to MMU pages.
 * @name:	Name of debugfs entry.
 * @parent:	Parent debugfs entry.
 *
 * Return: 0 on success, else error code.
 */
int mvx_mmu_pages_debugfs_init(struct mvx_mmu_pages *pages,
			       char *name,
			       struct dentry *parent);

/***
 *                                                                  CPU
 * 0x04 8000 0000                                         +-->  +----------+
 *                                                        |     |          |
 *                  DEVICE                 DRAM           |     |          |
 * 0x04 0000 0000+---------+  <------+  +----------+  +---+     |          |
 *               |         |            |          |            |          |
 *               |         |            |          |            |    DDR   |
 *               |         |            |          |            |          |
 *               |         |            |          |            |          |
 *               |         |            |          |            |          |
 * 0x01 0000 0000|         |            |          |      +-->  +----------+
 *               |         |            |          |      |     |          |
 *               |         |            |          |      |     |    IO    |
 * 0x00 8000 0000+---------+  <------+  +----------+  +---+-->  +----------+
 *               |         |            |          |            |          |
 *               |         |            |          |            |    DDR   |
 * 0x00 0000 0000+---------+  <------+  +----------+  +------>  +----------+
 *
 */
unsigned long phys_vpu2cpu(unsigned long phys_addr);
unsigned long phys_cpu2vpu(unsigned long phys_addr);

#endif /* _MVX_MMU_H_ */

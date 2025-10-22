/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2023 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2017 - 2023 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/

#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/mman.h>
#include <asm/atomic.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/mm_types.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/idr.h>
#include "gc_vip_kernel_allocator.h"
#include "gc_vip_kernel_drv.h"
#include <gc_vip_video_memory.h>
#if defined (USE_LINUX_PCIE_DEVICE)
#include <linux/pci.h>
#endif

#if defined(USE_LINUX_PCIE_DEVICE) || defined(USE_LINUX_PLATFORM_DEVICE)
#define FLUSH_CACHE_HOOK            1
#else
#define FLUSH_CACHE_HOOK            0
#endif

#define USE_MEM_WRITE_COMOBINE      1
#define USE_MEM_NON_CACHE           0

#if (LINUX_VERSION_CODE > KERNEL_VERSION (4,20,17) && !defined(CONFIG_ARCH_NO_SG_CHAIN)) ||   \
    (LINUX_VERSION_CODE >= KERNEL_VERSION (3,6,0)       \
    && (defined(ARCH_HAS_SG_CHAIN) || defined(CONFIG_ARCH_HAS_SG_CHAIN)))
#define gcdUSE_Linux_SG_TABLE_API   1
#else
#define gcdUSE_Linux_SG_TABLE_API   0
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
#define current_mm_mmap_sem current->mm->mmap_lock
#else
#define current_mm_mmap_sem current->mm->mmap_sem
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,5,0)
MODULE_IMPORT_NS(DMA_BUF);
#endif

static vip_status_e gckvip_allocator_dyn_alloc_contiguous(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    );

static vip_status_e gckvip_allocator_dyn_free_contiguous(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    );

static vip_status_e gckvip_map_user(
    gckvip_dyn_allocate_node_t *node,
    struct page **pages,
    vip_uint32_t num_pages,
    vip_uint32_t alloc_flag,
    void **logical
    );


extern gckvip_driver_t *kdriver;


#if LINUX_VERSION_CODE >= KERNEL_VERSION (3,7,0)
#define gcdVM_FLAGS (VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_DONTDUMP)
#else
#define gcdVM_FLAGS (VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_RESERVED)
#endif

#define gcd1M_PAGE_SHIFT 20
#define gcd1M_PAGE_SIZE (1 << gcd1M_PAGE_SHIFT)

#ifndef untagged_addr
#define untagged_addr(addr) (addr)
#endif

#if FLUSH_CACHE_HOOK
#if !gcdUSE_Linux_SG_TABLE_API
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,23)
struct sg_table
{
    struct scatterlist *sgl;
    unsigned int nents;
    unsigned int orig_nents;
};

static inline void sg_set_page(
    struct scatterlist *sg,
    struct page *page,
    unsigned int len,
    unsigned int offset
    )
{
    sg->page = page;
    sg->offset = offset;
    sg->length = len;
}

static inline void sg_mark_end(
    struct scatterlist *sg
    )
{
    (void)sg;
}
#endif

static vip_int32_t gckvip_sg_alloc_table_from_pages(
    struct scatterlist **sgl,
    struct page **pages,
    unsigned int  n_pages,
    unsigned long offset,
    unsigned long size,
    unsigned int  *nents
    )
{
    unsigned int chunks = 0;
    unsigned int i = 0;
    unsigned int cur_page = 0;
    struct scatterlist *s = VIP_NULL;

    chunks = 1;
    PRINTK_D("start do vsi sg alloc table from pages.\n");

    for (i = 1; i < n_pages; ++i) {
        if (page_to_pfn(pages[i]) != page_to_pfn(pages[i - 1]) + 1) {
            ++chunks;
        }
    }

    gckvip_os_allocate_memory(sizeof(struct scatterlist) * chunks, (void**)&s);
    if (unlikely(!s)) {
        PRINTK_E("failed to allocate memory in table_from_pages\n");
        return -ENOMEM;
    }
    gckvip_os_zero_memory(s, sizeof(struct scatterlist) * chunks);

    *sgl = s;
    *nents = chunks;
    cur_page = 0;

    for (i = 0; i < chunks; i++, s++) {
        unsigned long chunk_size = 0;
        unsigned int j = 0;

        for (j = cur_page + 1; j < n_pages; j++) {
            if (page_to_pfn(pages[j]) != page_to_pfn(pages[j - 1]) + 1) {
                break;
            }
        }

        chunk_size = ((j - cur_page) << PAGE_SHIFT) - offset;
        sg_set_page(s, pages[cur_page], min(size, chunk_size), offset);
        size -= chunk_size;
        offset = 0;
        cur_page = j;
    }

    sg_mark_end(s - 1);

    return 0;
}
#endif
#endif

vip_status_e flush_cache_init(
    gckvip_dyn_allocate_node_t *node,
    struct page **pages,
    vip_uint32_t num_pages,
    vip_uint32_t offset,
    vip_uint32_t size
    )
{
    vip_status_e status = VIP_SUCCESS;
#if FLUSH_CACHE_HOOK
    struct sg_table *sgt = VIP_NULL;
    vip_int32_t result = 0;
    vip_uint32_t flush_size = GCVIP_ALIGN(size, cache_line_size());

    PRINTK_D("flush cache init, size=0x%x, offset=0x%x, device=0x%"PRPx"\n",
             size, offset, kdriver->device);
    if (size & (cache_line_size() - 1)) {
        PRINTK_W("flush cache warning memory size 0x%x not align to cache line size 0x%x\n",
                   size, cache_line_size());
    }

    if (VIP_NULL == kdriver->device) {
        PRINTK_E("failed to flush cache init, device is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    gckvip_os_allocate_memory(sizeof(struct sg_table), (void**)&node->flush_cache_handle);
    if (!node->flush_cache_handle) {
        PRINTK_E("%s[%d], failed to alloca memory for flush cache handle\n",
                 __FUNCTION__, __LINE__);
        gcGoOnError(VIP_ERROR_IO);
    }

    sgt = (struct sg_table *)node->flush_cache_handle;

#if gcdUSE_Linux_SG_TABLE_API
    result = sg_alloc_table_from_pages(sgt, pages, num_pages, offset,
                                       flush_size, GFP_KERNEL | __GFP_NOWARN);
#else
    result = gckvip_sg_alloc_table_from_pages(&sgt->sgl, pages, num_pages, offset,
                                             flush_size, &sgt->nents);
    sgt->orig_nents = sgt->nents;
#endif
    if (unlikely(result < 0)) {
        PRINTK_E("%s[%d], sg alloc table from pages failed\n", __FUNCTION__, __LINE__);
        gcGoOnError(VIP_ERROR_IO);
    }

    result = dma_map_sg(kdriver->device, sgt->sgl, sgt->nents, DMA_BIDIRECTIONAL);
    if (unlikely(result != sgt->nents)) {
        PRINTK_E("%s[%d], dma_map_sg failed, result=%d, exp=%d\n",
                  __FUNCTION__, __LINE__, result, sgt->nents);
#if gcdUSE_Linux_SG_TABLE_API
        sg_free_table(sgt);
#else
        gckvip_os_free_memory((void*)sgt->sgl);
#endif
        node->flush_cache_handle = VIP_NULL;
        dump_stack();
        gcGoOnError(VIP_ERROR_IO);
    }

    dma_sync_sg_for_device(kdriver->device, sgt->sgl, sgt->nents, DMA_TO_DEVICE);
    dma_sync_sg_for_cpu(kdriver->device, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);

onError:
#endif
    return status;
}

vip_status_e flush_cache_destroy(
    gckvip_dyn_allocate_node_t *node
    )
{
#if FLUSH_CACHE_HOOK
    struct sg_table *sgt = VIP_NULL;

    PRINTK_D("flush_cache_destroy...\n");
    sgt = (struct sg_table *)node->flush_cache_handle;
    if (sgt != VIP_NULL) {
        dma_sync_sg_for_device(kdriver->device, sgt->sgl, sgt->nents, DMA_TO_DEVICE);
        dma_sync_sg_for_cpu(kdriver->device, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);
        dma_unmap_sg(kdriver->device, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);

        #if gcdUSE_Linux_SG_TABLE_API
        sg_free_table(sgt);
        #else
        gckvip_os_free_memory((void*)sgt->sgl);
        #endif

        gckvip_os_free_memory((void*)sgt);
        node->flush_cache_handle = VIP_NULL;
    }
#endif

    return VIP_SUCCESS;
}

static vip_status_e import_pfn_map(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    unsigned long long memory,
    vip_uint32_t num_pages,
    vip_uint32_t size,
    vip_uint32_t offset,
    vip_uint32_t memory_type
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct vm_area_struct *vma;
    unsigned long *pfns = VIP_NULL;
    vip_uint32_t i = 0;
    struct page **pages = VIP_NULL;
    vip_bool_e physical_contiguous = vip_true_e;

    PRINTK_D("map pfn memory address=0x%"PRIx64", size=0x%x, num page=%d\n",
              memory, size, num_pages);
    if ((memory & (cache_line_size() - 1)) || (size & (cache_line_size() - 1))) {
        PRINTK_W("warning, memory base address or size not align to cache line size,"
                 "address=0x%"PRIx64", size=0x%x\n", memory, size);
    }

    down_read(&current_mm_mmap_sem);
    vma = find_vma(current->mm, memory);
    up_read(&current_mm_mmap_sem);
    if (!vma) {
        PRINTK_E("not find vma for pfn map, user memory address=0x%x\n", memory);
        return VIP_ERROR_FAILURE;
    }

    gcOnError(gckvip_os_allocate_memory(num_pages * sizeof(struct page *), (void**)&pages));
    gcOnError(gckvip_os_allocate_memory(num_pages * sizeof(unsigned long), (void**)&pfns));

    for (i = 0; i < num_pages; i++) {
        spinlock_t *ptl;
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;

        pgd = pgd_offset(current->mm, memory);
        if (pgd_none(*pgd) || pgd_bad(*pgd)) {
            goto onError;
        }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION (4,11,0))
        pud = pud_offset((p4d_t*)pgd, memory);
#else
        pud = pud_offset(pgd, memory);
#endif
        if (pud_none(*pud) || pud_bad(*pud)) {
            goto onError;
        }

        pmd = pmd_offset(pud, memory);
        if (pmd_none(*pmd) || pmd_bad(*pmd)) {
            goto onError;
        }

        pte = pte_offset_map_lock(current->mm, pmd, memory, &ptl);
        if (!pte) {
            spin_unlock(ptl);
            goto onError;
        }

        if (!pte_present(*pte)) {
            pte_unmap_unlock(pte, ptl);
            goto onError;
        }

        pfns[i] = pte_pfn(*pte);
        pages[i] = pfn_to_page(pfns[i]); /* get page */

        pte_unmap_unlock(pte, ptl);

        /* Advance to next. */
        memory += PAGE_SIZE;
    }

    for (i = 1; i < num_pages; i++) {
        if (pfns[i] != pfns[i - 1] + 1) {
            physical_contiguous = vip_false_e;
            break;
        }
    }

    if (physical_contiguous) {
        phy_address_t cpu_physical = page_to_phys(pages[0]) + (phy_address_t)offset;
        /* all physical memory is contiguous */
        node->physical_table[0] = gckvip_drv_get_vipphysical(cpu_physical);
        node->size_table[0] = num_pages * PAGE_SIZE - offset;
        node->physical_num = 1;
    }
    else {
        for (i = 0; i < num_pages; i++) {
            node->physical_table[i] = gckvip_drv_get_vipphysical(page_to_phys(pages[i]));
            node->size_table[i] = PAGE_SIZE;
        }
        node->physical_table[0] += offset;
        node->size_table[0] -= offset;
        node->physical_num = num_pages;
    }

    node->num_pages = num_pages;
    node->alloc_handle = (void*)(pages);

    /* flush cache */
    gcOnError(flush_cache_init(node, pages, num_pages, offset, size));

    if (pfns != VIP_NULL) {
        gckvip_os_free_memory((void*)pfns);
    }

    return status;

onError:
    if (pfns != VIP_NULL) {
        gckvip_os_free_memory((void*)pfns);
    }
    if (pages != VIP_NULL) {
        gckvip_os_free_memory((void*)pages);
        pages = VIP_NULL;
    }

    return status;
}

static vip_status_e import_page_map(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    unsigned long long memory,
    vip_uint32_t num_pages,
    vip_uint32_t size,
    vip_uint32_t offset,
    vip_uint32_t memory_type
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct page **pages;
    vip_uint32_t i = 0;
    vip_int32_t result = 0;
    vip_bool_e physical_contiguous = vip_true_e;

    PRINTK_D("map page memory logical=0x%"PRIx64", size=0x%x, num page=%d\n",
              memory, size, num_pages);

    if ((memory & (cache_line_size() - 1)) || (size & (cache_line_size() - 1))) {
        PRINTK_W("warning, memory base address or size not align to cache line size, "
                 "address=0x%"PRIx64",size=0x%x, cache line size=%d\n",
                 memory, size, cache_line_size());
    }

    gcOnError(gckvip_os_allocate_memory(num_pages * sizeof(struct page *), (void**)&pages));

    down_read(&current_mm_mmap_sem);
    result = get_user_pages(
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
            current,
            current->mm,
#endif
            (memory & PAGE_MASK),
            num_pages,
#if (((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)) &&    \
     (LINUX_VERSION_CODE < KERNEL_VERSION(4,9, 0))) ||       \
     ((LINUX_VERSION_CODE <= KERNEL_VERSION(4, 4, 167))))
            1,
            0,
#else
            FOLL_WRITE,
#endif
            pages,
            NULL);

    up_read(&current_mm_mmap_sem);

    if (result < (vip_int32_t)num_pages) {
        for (i = 0; i < result; i++) {
            if (pages[i]) {
                put_page(pages[i]);
            }
        }
        PRINTK_E("failed to allocator wrap memory.. get page: %d < %d\n",
                  result, num_pages);
        gcGoOnError(VIP_ERROR_IO);
    }

    for (i = 1; i < num_pages; i++) {
        if (page_to_pfn(pages[i]) != page_to_pfn(pages[i - 1]) + 1) {
            physical_contiguous = vip_false_e;
            break;
        }
    }

    if (physical_contiguous) {
        /* all physical memory is contiguous */
        node->physical_table[0] = gckvip_drv_get_vipphysical(page_to_phys(pages[0]) + offset);
        node->size_table[0] = num_pages * PAGE_SIZE - offset;
        node->physical_num = 1;
    }
    else {
        for (i = 0; i < num_pages; i++) {
            node->physical_table[i] = gckvip_drv_get_vipphysical(page_to_phys(pages[i]));
            node->size_table[i] = PAGE_SIZE;
        }
        node->physical_table[0] += offset;
        node->size_table[0] -= offset;
        node->physical_num = num_pages;
    }

    node->num_pages = num_pages;
    node->alloc_handle = (void*)(pages);

    /* flush cache */
    gcOnError(flush_cache_init(node, pages, num_pages, offset, size));

    return status;

onError:
    if (pages != VIP_NULL) {
        gckvip_os_free_memory((void*)pages);
        pages = VIP_NULL;
    }

    return status;
}

#if defined (USE_LINUX_CMA)
static vip_status_e gckvip_allocator_cma_map_userlogical(
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t num_pages,
    phy_address_t cpu_physical,
    void **logical
    )
{
    void* user_logical = VIP_NULL;
    struct vm_area_struct *vma = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    user_logical = (void*)vm_mmap(NULL,
                       0L,
                       num_pages * PAGE_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_NORESERVE,
                       0);
    if (IS_ERR(user_logical)) {
        user_logical = VIP_NULL;
        PRINTK_E("failed to map user logical\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    down_read(&current_mm_mmap_sem);
    vma = find_vma(current->mm, (unsigned long)user_logical);
    up_read(&current_mm_mmap_sem);
    if (VIP_NULL == vma) {
        PRINTK_E("failed to faind vma fo CMA\n");
        dump_stack();
        gcGoOnError(VIP_ERROR_IO);
    }

   /* Make this mapping non-cached. */
#if USE_MEM_WRITE_COMOBINE
    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#else
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif
    node->mem_flag |= GCVIP_MEM_FLAG_NONE_CACHE;

    vma->vm_flags |= (VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_DONTDUMP);
    if (dma_mmap_coherent(kdriver->device, vma, node->kerl_logical,
                         cpu_physical, PAGE_ALIGN(node->size))) {
        PRINTK_E("CMA mmap user logical failed, vip_physical=0x%x, size=0x%08x\n",
                 cpu_physical, PAGE_ALIGN(node->size));
        gcGoOnError(VIP_ERROR_IO);
    }


    if (logical != VIP_NULL) {
        *logical = user_logical;
    }

onError:
    return status;
}
#endif

#if vpmdENABLE_MMU
static vip_status_e allocator_free_pages(
    struct page **pages,
    vip_uint32_t num_page
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;

    for (i = 0; i < num_page; i++) {
        if (pages[i] != VIP_NULL) {
            __free_page(pages[i]);
        }
    }

    return status;
}

static vip_status_e gckvip_allocator_dyn_alloc_1M_contiguous(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    )
{
    vip_status_e status = VIP_SUCCESS;
    size_t num_pages = 0, num_pages_1M = 0, num = 0;
    u32 gfp = GFP_KERNEL | __GFP_HIGHMEM | __GFP_NOWARN | __GFP_NORETRY;
    struct page **pages = VIP_NULL;
    struct page **Pages1M = VIP_NULL;
    struct page *page;
    size_t i = 0, j = 0;

    num_pages_1M = (node->size + (gcd1M_PAGE_SIZE - 1)) >> gcd1M_PAGE_SHIFT;
    num = gcd1M_PAGE_SIZE / PAGE_SIZE;
    num_pages = num_pages_1M * num;

    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * num_pages_1M,
                                       (void**)&node->physical_table));
    gckvip_os_zero_memory(node->physical_table, sizeof(phy_address_t) * num_pages_1M);
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint8_t) * num_pages_1M,
                                       (void**)&node->is_exact_page));
    gckvip_os_zero_memory(node->is_exact_page, sizeof(vip_uint8_t) * num_pages_1M);
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint32_t) * num_pages_1M,
                                        (void**)&node->size_table));
    gcOnError(gckvip_os_allocate_memory(sizeof(struct page *) * num_pages_1M,
                                        (void**)&Pages1M));
    gcOnError(gckvip_os_allocate_memory(sizeof(struct page *) * num_pages, (void**)&pages));

    /* remove __GFP_HIGHMEM bit, limit to 4G */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR) {
        gfp &= ~__GFP_HIGHMEM;
        /*add __GFP_DMA32 or __GFP_DMA bit */
        #if defined(CONFIG_ZONE_DMA32) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
        gfp |= __GFP_DMA32;
        #else
        gfp |= __GFP_DMA;
        #endif
        node->mem_flag |= GCVIP_MEM_FLAG_4GB_ADDR;
    }

    for (i = 0; i < num_pages_1M; i++) {
        phy_address_t cpu_physical = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        {
        void *addr = alloc_pages_exact(gcd1M_PAGE_SIZE, gfp);
        Pages1M[i] = addr ? virt_to_page(addr) : VIP_NULL;
        }
#endif

        if (VIP_NULL == Pages1M[i]) {
            int order = get_order(gcd1M_PAGE_SIZE);
            if (order >= MAX_ORDER)  {
                PRINTK_E("failed to alloc 1M contiguous memory, get order=%d is more than max=%d\n",
                           order, MAX_ORDER);
                gcGoOnError(VIP_ERROR_IO);
            }
            Pages1M[i] = alloc_pages(gfp, order);
            node->is_exact_page[i] = vip_false_e;
        }
        else {
            node->is_exact_page[i] = vip_true_e;
        }

        if (Pages1M[i] == VIP_NULL) {
            node->physical_num = i;
            gcGoOnError(VIP_ERROR_IO);
        }

        for (j = 0; j < num; j++) {
            page = nth_page(Pages1M[i], j);
            pages[i * num + j] = page;
        }
        cpu_physical = page_to_phys(nth_page(Pages1M[i], 0));
        node->physical_table[i] = gckvip_drv_get_vipphysical(cpu_physical);
        node->size_table[i] = gcd1M_PAGE_SIZE;
    }

    node->physical_num = num_pages_1M;

    /* map kernel space */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL) {
        pgprot_t pgprot;
        void *addr = VIP_NULL;
        if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE) {
            #if USE_MEM_WRITE_COMOBINE
            pgprot = pgprot_writecombine(PAGE_KERNEL);
            #elif USE_MEM_NON_CACHE
            pgprot = pgprot_noncached(PAGE_KERNEL);
            #endif
        }
        else {
            pgprot = PAGE_KERNEL;
        }

        addr = vmap(pages, num_pages, 0, pgprot);
        if (addr == NULL) {
            PRINTK_E("fail to vmap physical to kernel space\n");
            gcGoOnError(VIP_ERROR_IO);
        }

        node->kerl_logical = addr;
        node->mem_flag |= GCVIP_MEM_FLAG_MAP_KERNEL_LOGICAL;
    }

    /* map user space */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_USER) {
        gcOnError(gckvip_map_user(node, pages, num_pages, alloc_flag, VIP_NULL));
    }

    node->alloc_handle = (void*)(Pages1M);

    /* flush CPU cache */
    gcOnError(flush_cache_init(node, pages, num_pages, 0, num_pages << PAGE_SHIFT));
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE) {
        node->mem_flag |= GCVIP_MEM_FLAG_NONE_CACHE;
    }

    node->num_pages = num_pages;

    PRINTK_D("dny alloc 1M contiguous kernel address=0x%"PRPx", fir_physical=0x%"PRIx64", "
             "user address=0x%"PRPx", size=0x%xbyte, num_pages=%d, handl=0x%"PRPx", flag=0x%x\n",
             node->kerl_logical, node->physical_table[0], node->user_logical, node->size,
             node->num_pages, pages, node->mem_flag);

    if (pages != VIP_NULL) {
       gckvip_os_free_memory((void*)pages);
       pages = VIP_NULL;
    }

    return status;

onError:
    /* unmap user space */
    if ((node->user_logical != VIP_NULL) && (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_USER)) {
        if (vm_munmap((unsigned long)node->user_logical & PAGE_MASK, num_pages * PAGE_SIZE) < 0) {
            PRINTK_E("failed to vm munmap user space\n");
        }
        node->user_logical = VIP_NULL;
    }
    /* ummap kernel space */
    if ((node->kerl_logical != VIP_NULL) && (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL)) {
        vunmap((void *)node->kerl_logical);
        node->kerl_logical = VIP_NULL;
    }
    /* free pages */
    for (i = 0; i < node->physical_num; i++) {
        if (Pages1M != VIP_NULL) {
            if (Pages1M[i]) {
                #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
                if ((node->is_exact_page != VIP_NULL) && node->is_exact_page[i]) {
                    free_pages_exact(page_address(Pages1M[i]), gcd1M_PAGE_SIZE);
                }
                else
                #endif
                {
                    __free_pages(Pages1M[i], get_order(gcd1M_PAGE_SIZE));
                }
            }
        }
        if (node->size_table != VIP_NULL) {
            node->size_table[i] = 0;
        }
        if (node->physical_table != VIP_NULL) {
            node->physical_table[i] = 0;
        }
    }
    node->physical_num = 0;

    if (node->size_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->size_table);
        node->size_table = VIP_NULL;
    }
    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }
    if (pages != VIP_NULL) {
        gckvip_os_free_memory((void*)pages);
        pages = VIP_NULL;
    }
    if (Pages1M != VIP_NULL) {
        gckvip_os_free_memory((void*)Pages1M);
        Pages1M = VIP_NULL;
    }
    if (node->is_exact_page != VIP_NULL) {
        gckvip_os_free_memory((void*)node->is_exact_page);
        node->is_exact_page = VIP_NULL;
    }
    node->alloc_handle = VIP_NULL;

   return status;
}

static vip_status_e gckvip_allocator_dyn_free_1M_contiguous(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct page **Pages1M;
    vip_uint32_t i = 0;

    if ((VIP_NULL == context) || (VIP_NULL == node)) {
        PRINTK_E("failed free 1M contiguous dyn free, parameter is NULL\n");
        return VIP_ERROR_IO;
    }

    flush_cache_destroy(node);

    /* unmap user space */
    if (node->mem_flag & GCVIP_MEM_FLAG_MAP_USER) {
        if (vm_munmap((unsigned long)node->user_logical & PAGE_MASK, node->num_pages * PAGE_SIZE) < 0) {
            PRINTK_E("failed to vm munmap user space\n");
            status = VIP_ERROR_IO;
        }
    }

    /* ummap kernel space */
    if ((node->mem_flag & GCVIP_MEM_FLAG_MAP_KERNEL_LOGICAL) && (node->kerl_logical != VIP_NULL)) {
        vunmap((void *)node->kerl_logical);
        node->kerl_logical = VIP_NULL;
    }

    Pages1M = (struct page **)node->alloc_handle;
    PRINTK_D("dyn free 1M contiguous handle=0x%"PRPx"\n", node->alloc_handle);

    for (i = 0; i < node->physical_num && Pages1M[i]; i++) {
        if (Pages1M[i]) {
            #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
            if (node->is_exact_page[i]) {
                free_pages_exact(page_address(Pages1M[i]), gcd1M_PAGE_SIZE);
            }
            else
            #endif
            {
                __free_pages(Pages1M[i], get_order(gcd1M_PAGE_SIZE));
            }
        }
    }

    node->alloc_handle = VIP_NULL;

    gckvip_os_free_memory((void*)Pages1M);
    Pages1M = VIP_NULL;

    gckvip_os_free_memory((void*)node->physical_table);
    node->physical_table = VIP_NULL;

    gckvip_os_free_memory((void*)node->size_table);
    node->size_table = VIP_NULL;

    if (node->is_exact_page != VIP_NULL) {
        gckvip_os_free_memory((void*)node->is_exact_page);
        node->is_exact_page = VIP_NULL;
    }

    return status;
}

#if defined (USE_LINUX_CMA)
static vip_status_e gckvip_allocator_dyn_alloc_cma(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    )
{
    vip_status_e status = VIP_SUCCESS;
    static dma_addr_t handle;
    size_t num_pages = 0;
    phy_address_t cpu_physical = 0;

    num_pages = GetPageCount(PAGE_ALIGN(node->size));
    node->num_pages = (vip_uint32_t)num_pages;

    /* allocate physical address and kernel logical address */
    node->kerl_logical = dma_alloc_coherent(kdriver->device, PAGE_ALIGN(node->size),
                                            &handle, GFP_KERNEL);
    if (VIP_NULL == node->kerl_logical) {
        PRINTK_E("fail to alloc memory from cma\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * node->num_pages,
                                        (void**)&node->physical_table));
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint32_t) * node->num_pages,
                                        (void**)&node->size_table));

    cpu_physical = (phy_address_t)handle;
    node->physical_table[0] = gckvip_drv_get_vipphysical(cpu_physical);
    node->size_table[0] = node->size;
    node->physical_num = 1;
    node->alloc_handle = GCKVIPUINT64_TO_PTR(cpu_physical);

    /* map user space logical address */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_USER) {
        vip_uint32_t offset = cpu_physical - (cpu_physical & PAGE_MASK);
        void* user_logical = VIP_NULL;
        gcOnError(gckvip_allocator_cma_map_userlogical(node, num_pages, cpu_physical, &user_logical));
        node->user_logical = (vip_uint8_t*)user_logical;
        node->user_logical += offset;
        node->mem_flag |= GCVIP_MEM_FLAG_MAP_USER;
    }
    node->mem_flag |= GCVIP_MEM_FLAG_NONE_CACHE;

    PRINTK_D("dny alloc CMA physical=0x%"PRIx64", kernel address=0x%"PRPx", user address=0x%"PRPx", "
             "size=0x%xbyte, num_pages=%d, flag=0x%x\n",
              node->physical_table[0], node->kerl_logical, node->user_logical,
              node->size, node->num_pages, node->mem_flag);

    return status;

onError:
    if (node->size_table != VIP_NULL) {
       gckvip_os_free_memory((void*)node->size_table);
       node->size_table = VIP_NULL;
    }
    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }
    return status;
}

vip_status_e gckvip_allocator_dyn_free_cma(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;
    phy_address_t cpu_physical = gckvip_drv_get_cpuphysical(node->physical_table[0]);

    if (node->kerl_logical != VIP_NULL) {
        dma_free_coherent(kdriver->device, PAGE_ALIGN(node->size),
                          node->kerl_logical, cpu_physical);
    }

    if ((node->mem_flag & GCVIP_MEM_FLAG_MAP_USER) && (node->user_logical != VIP_NULL)) {
        if (vm_munmap((unsigned long)node->user_logical & PAGE_MASK, node->num_pages * PAGE_SIZE) < 0) {
            PRINTK_E("failed to vm munmap user space for CMA\n");
            status = VIP_ERROR_IO;
        }
    }

    gckvip_os_free_memory((void*)node->physical_table);
    node->physical_table = VIP_NULL;

    gckvip_os_free_memory((void*)node->size_table);
    node->size_table = VIP_NULL;

    return status;
}
#endif

/*
@brief allocate memory from system.
       get the physical address table each page.
       get size table of eache page
@param context, the contect of kernel space.
@param node, dynamic allocate node.
@param align, the size of alignment for this video memory.
@param flag the flag of this video memroy. see gckvip_video_mem_alloc_flag_e.
*/
static vip_status_e gckvip_allocator_dyn_alloc_ext(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    )
{
    vip_status_e status = VIP_SUCCESS;
    size_t num_pages = 0;
    u32 gfp = GFP_KERNEL | __GFP_HIGHMEM | __GFP_NOWARN;
    struct page **pages = VIP_NULL;
    struct page *page = VIP_NULL;
    size_t i = 0;

    if ((VIP_NULL == context) || (VIP_NULL == node)) {
        PRINTK_E("failed allocator dyn alloc, parameter is NULL\n");
        return VIP_NULL;
    }

#if defined (USE_LINUX_CMA)
    status = gckvip_allocator_dyn_alloc_cma(context, node, align, alloc_flag);
    if (VIP_SUCCESS == status) {
        node->mem_flag |= GCVIP_MEM_FLAG_CMA;
        return status;
    }
#endif

    /* need physical contiguous memory */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS) {
        status = gckvip_allocator_dyn_alloc_contiguous(context, node, align, alloc_flag);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to allocate dynamic contiguous memory size=%dbytes, "
                      "alloc_flag=0x%x, status=%d\n", node->size, alloc_flag,  status);
        }
        else {
            node->mem_flag |= GCVIP_MEM_FLAG_CONTIGUOUS;
        }
        return status;
    }

    if ((alloc_flag & GCVIP_VIDEO_MEM_ALLOC_1M_CONTIGUOUS) && (node->size >= 0x100000)) {
        status = gckvip_allocator_dyn_alloc_1M_contiguous(context, node, align, alloc_flag);
        if (VIP_SUCCESS == status) {
            node->mem_flag |= GCVIP_MEM_FLAG_1M_CONTIGUOUS;
            return status;
        }
    }

    num_pages = GetPageCount(PAGE_ALIGN(node->size));
    node->num_pages = num_pages;
    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * num_pages,
                                        (void**)&node->physical_table));
    gckvip_os_zero_memory(node->physical_table, sizeof(phy_address_t) * num_pages);

    /* remove __GFP_HIGHMEM bit, limit to 4G */
    if ((alloc_flag & GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR)) {
        gfp &= ~__GFP_HIGHMEM;
        /*add __GFP_DMA32 or __GFP_DMA bit */
        #if defined(CONFIG_ZONE_DMA32) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
        gfp |= __GFP_DMA32;
        #else
        gfp |= __GFP_DMA;
        #endif
        node->mem_flag |= GCVIP_MEM_FLAG_4GB_ADDR;
    }

    gcOnError(gckvip_os_allocate_memory(sizeof(struct page *) * num_pages, (void**)&pages));
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint32_t) * num_pages, (void**)&node->size_table));

    /* alloc pages */
    for (i = 0; i < num_pages; i++) {
        phy_address_t cpu_physical = 0;
        page = alloc_page(gfp);
        if (!page) {
            allocator_free_pages(pages, i);
            PRINTK_E("failed to alloc dyn memory in alloc page..allocated=%d page, total=%dpage.\n",
                    i, num_pages);
            gcGoOnError(VIP_ERROR_IO);
        }
        pages[i] = page;
        cpu_physical = page_to_phys(page);
        node->physical_table[i] = gckvip_drv_get_vipphysical(cpu_physical);
        node->size_table[i] = PAGE_SIZE;
        if (node->physical_table[i] > 0xFFFFFFFF) {
            PRINTK_I("physical[%d]=0x%"PRIx64" is large than 4G\n", i, node->physical_table[i]);
        }
    }

    node->physical_num = num_pages;

    /* map kernel space */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL) {
        pgprot_t pgprot;
        void *addr = VIP_NULL;

        if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE) {
            #if USE_MEM_WRITE_COMOBINE
            pgprot = pgprot_writecombine(PAGE_KERNEL);
            #elif USE_MEM_NON_CACHE
            pgprot = pgprot_noncached(PAGE_KERNEL);
            #endif
        }
        else {
            pgprot = PAGE_KERNEL;
        }

        addr = vmap(pages, num_pages, 0, pgprot);
        if (addr == NULL) {
            PRINTK_E("fail to vmap physical to kernel space\n");
            gcGoOnError(VIP_ERROR_IO);
        }

        node->kerl_logical = addr;
        node->mem_flag |= GCVIP_MEM_FLAG_MAP_KERNEL_LOGICAL;
    }

    /* map user space */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_USER) {
        status = gckvip_map_user(node, pages, num_pages, alloc_flag, VIP_NULL);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to map user logical status=%d\n", status);
            gcGoOnError(status);
        }
    }

    node->alloc_handle = (void*)(pages);

    /* flush CPU cache */
    gcOnError(flush_cache_init(node, pages, num_pages, 0, num_pages << PAGE_SHIFT));
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE) {
        node->mem_flag |= GCVIP_MEM_FLAG_NONE_CACHE;
    }

    PRINTK_D("dny alloc kernel address=0x%"PRPx", fir_physical=0x%"PRIx64", "
             "user address=0x%"PRPx", size=0x%d, num_pages=%d, handle=0x%"PRPx", flag=0x%x\n",
             node->kerl_logical, node->physical_table[0],
             node->user_logical, node->size, num_pages, pages, node->mem_flag);

    return status;

onError:
    /* unmap user space */
    if (node->mem_flag & GCVIP_MEM_FLAG_MAP_USER) {
        if (vm_munmap((unsigned long)node->user_logical & PAGE_MASK, num_pages * PAGE_SIZE) < 0) {
            PRINTK_E("failed to vm munmap user space\n");
        }
        node->user_logical = VIP_NULL;
    }
    if ((node->mem_flag & GCVIP_MEM_FLAG_MAP_KERNEL_LOGICAL) && (node->kerl_logical != VIP_NULL)) {
        vunmap((void *)node->kerl_logical);
        node->kerl_logical = VIP_NULL;
    }
    /* free pages */
    if (pages != VIP_NULL) {
        allocator_free_pages(pages, num_pages);
    }
    node->alloc_handle = VIP_NULL;
    if (node->size_table != VIP_NULL) {
       gckvip_os_free_memory((void*)node->size_table);
       node->size_table = VIP_NULL;
    }
    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }
    if (pages != VIP_NULL) {
        gckvip_os_free_memory((void*)pages);
        pages = VIP_NULL;
    }

    return status;
}

/*
@brief free a dynamic allocate memory.
@param context, the contect of kernel space.
@param node, dynamic allocate node info.
*/
static vip_status_e gckvip_allocator_dyn_free_ext(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct page **pages;

    if ((VIP_NULL == context) || (VIP_NULL == node)) {
        PRINTK_E("failed free dyn alloc, parameter is NULL\n");
        return VIP_ERROR_IO;
    }

#if defined (USE_LINUX_CMA)
    if (node->mem_flag & GCVIP_MEM_FLAG_CMA) {
        gckvip_allocator_dyn_free_cma(context, node);
        return status;
    }
#endif

    /* contiguous memory */
    if (node->mem_flag & GCVIP_MEM_FLAG_CONTIGUOUS) {
        status = gckvip_allocator_dyn_free_contiguous(context, node);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to free dynamic contiguous memory\n");
        }
        return status;
    }

    if (node->mem_flag & GCVIP_MEM_FLAG_1M_CONTIGUOUS) {
        status = gckvip_allocator_dyn_free_1M_contiguous(context, node);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to free dynamic 1M contiguous memory\n");
        }
        return status;
    }

    flush_cache_destroy(node);

    /* unmap user space */
    if (node->mem_flag & GCVIP_MEM_FLAG_MAP_USER) {
        if (vm_munmap((unsigned long)node->user_logical & PAGE_MASK, node->num_pages * PAGE_SIZE) < 0) {
            PRINTK_E("failed to vm munmap user space\n");
            status = VIP_ERROR_IO;
        }
    }

    /* ummap kernel space */
    if ((node->mem_flag & GCVIP_MEM_FLAG_MAP_KERNEL_LOGICAL) && (node->kerl_logical != VIP_NULL)) {
        vunmap((void *)node->kerl_logical);
        node->kerl_logical = VIP_NULL;
    }

    pages = (struct page **)node->alloc_handle;
    PRINTK_D("dyn free non-contiguous handle=0x%"PRPx"\n", node->alloc_handle);

    allocator_free_pages(pages, node->num_pages);
    node->alloc_handle = VIP_NULL;

    gckvip_os_free_memory((void*)pages);
    pages = VIP_NULL;

    gckvip_os_free_memory((void*)node->physical_table);
    node->physical_table = VIP_NULL;

    gckvip_os_free_memory((void*)node->size_table);
    node->size_table = VIP_NULL;

    return status;
}

/*
@brief get user space logical address.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
@param, virtual, the VIP's virtual address.
@param, logical, use space logical address.
@param, video memory type see gckvip_video_memory_type_e
*/
vip_status_e gckvip_allocator_get_userlogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t virtual_addr,
    void **logical,
    vip_uint8_t memory_type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int64_t offset = virtual_addr - node->vip_virtual_address;

    if (GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_PHYSICAL & memory_type) {
        return VIP_SUCCESS;
    }

    if ((offset >= 0) && ((vip_uint32_t)offset <= node->size) &&
        (node->user_logical != VIP_NULL)) {
        *logical = offset + node->user_logical;
    }

    return status;
}

/*
@brief get kernel space logical address.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
@param, virtual_addr, the VIP's virtual address.
@param, logical, kernel space logical address.
@param, video memory type see gckvip_video_memory_type_e
*/
vip_status_e gckvip_allocator_get_kernellogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t virtual_addr,
    void **logical,
    vip_uint8_t memory_type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int64_t offset = virtual_addr - node->vip_virtual_address;

    if ((GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_PHYSICAL & memory_type) ||
        (GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF & memory_type)) {
        return VIP_SUCCESS;
    }

    if (((offset >= 0) && ((vip_uint32_t)offset <= node->size)) &&
        (node->kerl_logical != VIP_NULL)) {
        *logical = offset + node->kerl_logical;
    }

    return status;
}

/*
@brief get vip physical address.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
@param IN vip_virtual, the vitual address of VIP.
@param, physical, VIP physical address.
@param, video memory type see gckvip_video_memory_type_e
*/
vip_status_e gckvip_allocator_get_vipphysical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t vip_virtual,
    phy_address_t *physical,
    vip_uint8_t memory_type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int64_t offset = 0;
    vip_uint32_t page_in_offsete = 0;
    vip_uint32_t num_page = 0;

    if ((GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_PHYSICAL & memory_type) ||
        (GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF & memory_type)) {
        return VIP_SUCCESS;
    }

    offset = vip_virtual - node->vip_virtual_address;

    if ((offset >= 0) && ((vip_uint32_t)offset <= node->size)) {
        #if defined (USE_LINUX_CMA)
        if (node->mem_flag & GCVIP_MEM_FLAG_CMA) {
            *physical = node->physical_table[0] + offset;
            return VIP_SUCCESS;
        }
        #endif
        if (node->mem_flag & GCVIP_MEM_FLAG_CONTIGUOUS) {
            *physical = node->physical_table[0] + offset;
            return VIP_SUCCESS;
        }
        else if (node->mem_flag & GCVIP_MEM_FLAG_1M_CONTIGUOUS) {
            num_page = offset / gcdMMU_PAGE_1M_SIZE;
            page_in_offsete = offset % gcdMMU_PAGE_1M_SIZE;
        }
        else {
            num_page = offset >> PAGE_SHIFT;
            page_in_offsete = offset & ~PAGE_MASK;
        }
    }
    else {
        PRINTK_E("allocator get physical failed, offset=%" PRId64"\n", offset);
        return VIP_ERROR_IO;
    }

    *physical = node->physical_table[num_page] + (phy_address_t)page_in_offsete;

    return status;
}
#endif

/*
@brief convert user's memory to CPU physical. And map to VIP virtual address.
@param, context, the contect of kernel space.
@param, logical, the logical address(handle) should be wraped.
@param, memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_wrap_usermemory(
    gckvip_context_t *context,
    vip_ptr logical,
    vip_uint32_t memory_type,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t size = node->size;
    vip_uint32_t num_pages = GetPageCount(PAGE_ALIGN(size));
    unsigned long long memory = (unsigned long long)(gckvip_uintptr_t)logical;
    unsigned long long memory1 = (unsigned long long)(gckvip_uintptr_t)logical;
    unsigned long long vaddr = 0;
    struct vm_area_struct *vma = NULL;
    unsigned long vm_flags = 0;
    vip_uint32_t offset = 0;
    unsigned long start = 0, end = 0;

    memory = untagged_addr(memory1);

    offset = memory - (memory & PAGE_MASK);

    /* Get the number of required pages. */
    end = (memory + size + PAGE_SIZE - 1) >> PAGE_SHIFT;
    start = memory >> PAGE_SHIFT;
    num_pages = end - start;

#if !vpmdENABLE_MMU
    if (num_pages > 1) {
        PRINTK_E("fail MMU is disabled, memory pages num=%d, not support\n", num_pages);
        return VIP_ERROR_OUT_OF_RESOURCE;
    }
#endif

    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * num_pages,
                                        (void**)&node->physical_table));
    gckvip_os_zero_memory(node->physical_table, sizeof(phy_address_t) * num_pages);

    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint32_t) * num_pages, (void**)&node->size_table));
    gckvip_os_zero_memory(node->size_table, sizeof(vip_uint32_t) * num_pages);

    down_read(&current_mm_mmap_sem);
    vma = find_vma(current->mm, memory);
    up_read(&current_mm_mmap_sem);
    if (!vma) {
        PRINTK_E("No such memory, or across vmas\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    vm_flags = vma->vm_flags;
    vaddr = vma->vm_end;

    PRINTK_D("allocator_wrap_usermemory page count=%d, size=0x%x, logical=0x%"PRIx64", "
             "page start logical=0x%x, 0ffset=0x%x\n",
              num_pages, size, memory, (memory & PAGE_MASK), offset);

    /* check user space logical */
    while (vaddr < memory + size) {
        down_read(&current_mm_mmap_sem);
        vma = find_vma(current->mm, vaddr);
        up_read(&current_mm_mmap_sem);
        if (!vma) {
            PRINTK_E("No such memory, or across vmas\n");
            gcGoOnError(VIP_ERROR_IO);
        }

        if ((vma->vm_flags & VM_PFNMAP) != (vm_flags & VM_PFNMAP)) {
            PRINTK_E("Can't support different map type: both PFN and PAGE detected\n");
            gcGoOnError(VIP_ERROR_IO);
        }

        vaddr = vma->vm_end;
    }

    /* convert logical to physical */
    if (vm_flags & VM_PFNMAP) {
        gcOnError(import_pfn_map(context, node, memory, num_pages,  size, offset, memory_type));
        node->mem_flag |= GCVIP_MEM_FLAG_USER_MEMORY_PFN;
    }
    else {
        gcOnError(import_page_map(context, node, memory, num_pages,  size, offset, memory_type));
        node->mem_flag |= GCVIP_MEM_FLAG_USER_MEMORY_PAGE;
    }

    node->user_logical = logical;
    node->kerl_logical = VIP_NULL; /* TODO */

    PRINTK_D("wrap usermemory handle=0x%"PRPx", logical=0x%"PRIx64", "
             "physical base=0x%"PRIx64", physical_num=%d, size=0x%x, offset=0x%x, flag=0x%x\n",
              node->alloc_handle, memory, node->physical_table[0],
              node->physical_num, size, offset, node->mem_flag);

    return VIP_SUCCESS;

onError:
    if (node->size_table != VIP_NULL) {
       gckvip_os_free_memory((void*)node->size_table);
       node->size_table = VIP_NULL;
    }
    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }

    PRINTK_E("failed to wrap user memory=0x%"PRIx64", size=0x%x\n", memory, size);
    return status;
}

/*
@brief un-wrap user memory(handle).
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_unwrap_usermemory(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct page **pages = VIP_NULL;
    vip_uint32_t i = 0;

    if ((VIP_NULL == context) || (VIP_NULL == node)) {
        PRINTK_E("failed free dyn alloc, parameter is NULL, node=0x%"PRPx"\n", node);
        return VIP_ERROR_IO;
    }

    PRINTK_I("allocator unwrap user memory flag=0x%x\n", node->mem_flag);

    /* with cache on Linux memory which create from user */
    flush_cache_destroy(node);

    pages = (struct page **)node->alloc_handle;
    if (pages != VIP_NULL) {
        for (i = 0; i < node->num_pages; i++) {
            if ((pages[i]) && (node->mem_flag & GCVIP_MEM_FLAG_USER_MEMORY_PAGE)) {
                put_page(pages[i]);
            }
        }
        gckvip_os_free_memory((void*)pages);
        pages = VIP_NULL;
    }

    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }

    if (node->size_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->size_table);
        node->size_table = VIP_NULL;
    }

    return status;
}

/*
@brief convert cpu's physical to vip physical.
@param IN physical_table Physical address table. should be wraped for VIP hardware.
@param IN size_table The size of physical memory for each physical_table element.
@param IN physical_num The number of physical table element.
@param IN node, dynamic allocate node info.
@param IN alloc_flag the flag of this video memroy. see gckvip_video_mem_alloc_flag_e.
*/
vip_status_e gckvip_allocator_wrap_userphysical(
    gckvip_context_t *context,
    vip_address_t *physical_table,
    vip_uint32_t *size_table,
    vip_uint32_t physical_num,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t alloc_flag
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vip_uint32_t contiguous = vip_true_e;
    vip_uint32_t page_offset = physical_table[0] - (physical_table[0] & PAGE_MASK);

    /* check physical table are contiguous ? */
    for (i = 0; i < physical_num - 1; i++) {
        if ((physical_table[i] + size_table[i]) != physical_table[i + 1]) {
            contiguous = vip_false_e;
            break;
        }
    }

    node->physical_num = physical_num;
    node->num_pages = GetPageCount(PAGE_ALIGN(node->size + page_offset));
    node->alloc_flag |= GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE;
    node->mem_flag |= GCVIP_MEM_FLAG_NONE_CACHE;

    node->mem_flag |= GCVIP_MEM_FLAG_USER_MEMORY_PHYSICAL;

    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * physical_num, (void**)&node->physical_table));
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint32_t) * physical_num, (void**)&node->size_table));
    gckvip_os_zero_memory(node->physical_table, sizeof(phy_address_t) * physical_num);
    gckvip_os_zero_memory(node->size_table, sizeof(vip_uint32_t) * physical_num);

    for (i = 0; i < physical_num; i++) {
        node->physical_table[i] = gckvip_drv_get_vipphysical(physical_table[i]);
        node->size_table[i] = size_table[i];
    }

    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_USER) {
        if (contiguous) {
            vip_uint32_t offset = physical_table[0] - (physical_table[0] & PAGE_MASK);
            status = gckvip_allocator_map_userlogical(context, node, (void**)&node->user_logical);
            if (status != VIP_SUCCESS) {
                PRINTK_E("fail to allocator map user logical status=%d\n", status);
                gcGoOnError(status);
            }
            node->user_logical += offset;
            node->mem_flag |= GCVIP_MEM_FLAG_MAP_USER;
            PRINTK_I("wrap user physical=0x%"PRIx64", user logical=0x%"PRPx", offset=0x%x\n",
                        physical_table[0], node->user_logical, offset);
        }
        else {
            PRINTK_W("waring wrap physical num=%d, not support map user logical\n", physical_num);
        }
    }

    node->kerl_logical = VIP_NULL;

    return status;
onError:
    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }

    if (node->size_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->size_table);
        node->size_table = VIP_NULL;
    }
    return status;
}

/*
@brief un-wrap user physical.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_unwrap_userphysical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{

    if (node->mem_flag & GCVIP_MEM_FLAG_MAP_USER) {
        if (node->user_logical != VIP_NULL) {
            gckvip_allocator_unmap_userlogical(context, node);
            node->user_logical = VIP_NULL;
        }
    }

    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }

    if (node->size_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->size_table);
        node->physical_table = VIP_NULL;
    }

    return VIP_SUCCESS;
}

#if vpmdENABLE_CREATE_BUF_FD
/*
@brief convert user's memory to CPU physical. And map to VIP virtual address.
@param, context, the contect of kernel space.
@param, fd, the user memory file descriptor.
@param, memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_wrap_userfd(
    gckvip_context_t *context,
    vip_uint32_t fd,
    vip_uint32_t memory_type,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t num_pages = 0;
    gckvip_dma_buf_info_t *import = VIP_NULL;
    struct scatterlist *s = VIP_NULL;
    struct sg_table *sgt = VIP_NULL;
    vip_uint32_t count = 0;
    vip_uint32_t i = 0, j = 0;
    vip_uint32_t dmabuf_size = 0;
    vip_bool_e physical_contiguous = vip_true_e;
    phy_address_t *cpu_physical = VIP_NULL;

    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_dma_buf_info_t), (void**)&import));
    import->dma_buf = dma_buf_get(fd);
    if (IS_ERR_OR_NULL(import->dma_buf)) {
        PRINTK_E("failed wrap fd dma_buf_get, fd=%d\n", fd);
        gcGoOnError(VIP_ERROR_IO);
    }
    dma_buf_put(import->dma_buf);

    get_dma_buf(import->dma_buf);

    import->dma_attachment = dma_buf_attach(import->dma_buf, kdriver->device);
    if (!import->dma_attachment) {
        PRINTK_E("failed wrap fd dma_buf_attach\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    sgt = import->sgt = dma_buf_map_attachment(import->dma_attachment, DMA_BIDIRECTIONAL);
    if (IS_ERR_OR_NULL(import->sgt)) {
        PRINTK_E("failed wrap fd dma_buf_map_attachment\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    /* Get number of pages. */
    for_each_sg(sgt->sgl, s, sgt->orig_nents, i) {
        num_pages += (sg_dma_len(s) + PAGE_SIZE - 1) / PAGE_SIZE;
        PRINTK_D("dmabuf[%d] address=0x%"PRIx64", size=0x%x\n", count, sg_dma_address(s), sg_dma_len(s));
        count++;
        dmabuf_size += sg_dma_len(s);
    }
    count = 0;

    if (node->size > dmabuf_size) {
        PRINTK_E("failed to wrap user fd=%d, dmabug size %d is small than request %d\n",
                  fd, dmabuf_size, node->size);
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
    node->size = dmabuf_size;

    PRINTK_D("import dma-buf num_pages=%d\n", num_pages);
    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * num_pages, (void**)&node->physical_table));
    gckvip_os_zero_memory(node->physical_table, sizeof(phy_address_t) * num_pages);
    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * num_pages, (void**)&cpu_physical));
    gckvip_os_zero_memory(cpu_physical, sizeof(phy_address_t) * num_pages);
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint32_t) * num_pages, (void**)&node->size_table));
    gckvip_os_zero_memory(node->size_table, sizeof(vip_uint32_t) * num_pages);

    /* Fill page array. */
    i = 0;
    for_each_sg(sgt->sgl, s, sgt->orig_nents, i) {
        vip_uint32_t num = (sg_dma_len(s) + PAGE_SIZE - 1) / PAGE_SIZE;
        for (j = 0; j < num; j++) {
            cpu_physical[count] = sg_dma_address(s) + j * PAGE_SIZE;
            node->size_table[count] = PAGE_SIZE;
            count++;
        }
    }

    for (i = 1; i < count; i++) {
        if (__phys_to_pfn(cpu_physical[i]) != (__phys_to_pfn(cpu_physical[i-1]) + 1)) {
            physical_contiguous = vip_false_e;
            break;
        }
    }

    if (physical_contiguous) {
        /* all physical memory is contiguous */
        node->size_table[0] = dmabuf_size;
        node->physical_num = 1;
    }
    else {
        node->physical_num = count;
    }

    /* map user logical */
    {
        struct page **pages = VIP_NULL;
        vip_uint32_t alloc_flag = GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE;
        vip_uint32_t offset = cpu_physical[0] - (cpu_physical[0] & PAGE_MASK);
        gcOnError(gckvip_os_allocate_memory(sizeof(struct page *) * count, (void**)&pages));
        for (i = 0; i < count; i++) {
            pages[i] = pfn_to_page(__phys_to_pfn(cpu_physical[i] & PAGE_MASK));
        }
        status = gckvip_map_user(node, pages, num_pages, alloc_flag, VIP_NULL);
        gckvip_os_free_memory((void*)pages);
        pages = VIP_NULL;
        if (status != VIP_SUCCESS) {
            gcGoOnError(VIP_ERROR_FAILURE);
        }
        node->user_logical += offset;
    }

    for (i = 0; i < node->physical_num; i++) {
        node->physical_table[i] = gckvip_drv_get_vipphysical(cpu_physical[i]);
    }

    node->num_pages = count;
    node->alloc_handle = (void *)import;
    node->mem_flag |= GCVIP_MEM_FLAG_NONE_CACHE;
    node->mem_flag |= GCVIP_MEM_FLAG_USER_MEMORY_FD;

    PRINTK_D("wrap user fd=%d, dma_buf=0x%"PRPx", physical_num=%d, physical base=0x%"PRIx64", "
             "size=0x%x, flag=0x%x\n", fd, import->dma_buf,
             node->physical_num, node->physical_table[0], node->size, node->mem_flag);

    gckvip_os_free_memory((void*)cpu_physical);

    return VIP_SUCCESS;

onError:
    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }
    if (node->size_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->size_table);
        node->size_table = VIP_NULL;
    }
    if (import != VIP_NULL) {
        if (!IS_ERR_OR_NULL(import->sgt)) {
            dma_buf_unmap_attachment(import->dma_attachment, import->sgt, DMA_BIDIRECTIONAL);
        }
        if (import->dma_attachment) {
            dma_buf_detach(import->dma_buf, import->dma_attachment);
        }
        if (!(IS_ERR_OR_NULL(import->dma_buf))) {
            dma_buf_put(import->dma_buf);
        }
        gckvip_os_free_memory((void*)import);
    }
    if (cpu_physical != VIP_NULL) {
        gckvip_os_free_memory((void*)cpu_physical);
        cpu_physical = VIP_NULL;
    }
    PRINTK_E("failed to wrap user fd=%d, size=0x%x\n", fd, node->size);
    return VIP_ERROR_FAILURE;
}

/*
@brief un-wrap user fd(file descriptor).
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_unwrap_userfd(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_dma_buf_info_t *import = VIP_NULL;

    if ((VIP_NULL == context) || (VIP_NULL == node)) {
        PRINTK_E("failed free dyn alloc, parameter is NULL, node=0x%"PRPx"\n", node);
        return VIP_ERROR_IO;
    }

    if (VIP_NULL == node->alloc_handle) {
        PRINTK_E("failed to unwrap user fd, alloc handle is NULL\n");
        return VIP_ERROR_IO;
    }
    /* unmap user space */
    if (node->mem_flag & GCVIP_MEM_FLAG_MAP_USER) {
        vip_int32_t ret = 0;
        ret = vm_munmap((unsigned long)node->user_logical & PAGE_MASK, node->num_pages * PAGE_SIZE);
        if (ret < 0) {
            PRINTK_E("failed to vm munmap user space for user fd\n");
            status = VIP_ERROR_IO;
        }
    }

    import = (gckvip_dma_buf_info_t *)node->alloc_handle;

    PRINTK_D("unwrap user fd, dma_buf=0x%"PRPx"\n", import->dma_buf);
    if (!IS_ERR_OR_NULL(import->sgt)) {
        dma_buf_unmap_attachment(import->dma_attachment, import->sgt, DMA_BIDIRECTIONAL);
        import->sgt = VIP_NULL;
    }
    if (import->dma_attachment) {
        dma_buf_detach(import->dma_buf, import->dma_attachment);
        import->dma_attachment = VIP_NULL;
    }
    if (!(IS_ERR_OR_NULL(import->dma_buf))) {
        dma_buf_put(import->dma_buf);
        import->dma_buf = VIP_NULL;
    }

    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }

    if (node->size_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->size_table);
        node->size_table = VIP_NULL;
    }

    if (import != VIP_NULL) {
        gckvip_os_free_memory((void*)import);
        import = VIP_NULL;
    }

    return status;
}
#endif

/*
@brief Flush CPU cache for dynamic alloc video memory and wrap user memory.
@param context, the contect of kernel space.
@param node, dynamic allocate node info.
@param physical, VIP physical address. May need to covert to CPU physcial address.
        gckvip_drv_get_cpuphysical to convert the physical address should be flush CPU cache.
@param logical, the logical address should be flush.
@param size, the size of the memory should be flush.
@param type The type of operate cache.
*/
#if vpmdENABLE_FLUSH_CPU_CACHE
vip_status_e gckvip_alloctor_flush_cache(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    phy_address_t physical,
    void *logical,
    vip_uint32_t size,
    gckvip_cache_type_e type
    )
{
    vip_status_e status = VIP_SUCCESS;

#if FLUSH_CACHE_HOOK
    if (node->mem_flag & GCVIP_MEM_FLAG_NONE_CACHE) {
        return VIP_SUCCESS;
    }

    PRINTK_D("allocator flush cache... %s, handle=0x%"PRPx", memory_flag=0x%x,"
             "mem_flag=0x%x, flush_hanlde=0x%"PRPx", type=%s\n",
             (node->mem_flag & GCVIP_MEM_FLAG_CONTIGUOUS) ? "contiguous": "non-contiguous",
             node->alloc_handle, node->mem_flag, node->mem_flag, node->flush_cache_handle,
             (GCKVIP_CACHE_CLEAN == type) ? "CLEAN" : (GCKVIP_CACHE_FLUSH == type) ? "FLUSH" : "INVALID");

    if (node->flush_cache_handle != VIP_NULL) {
        if (node->mem_flag & GCVIP_MEM_FLAG_CONTIGUOUS) {
            size_t num_pages = GetPageCount(PAGE_ALIGN(node->size));
            dma_addr_t *dma_addr = (dma_addr_t*)node->flush_cache_handle;
            switch (type) {
            case GCKVIP_CACHE_CLEAN:
            {
                dma_sync_single_for_device(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_TO_DEVICE);
            }
            break;

            case GCKVIP_CACHE_FLUSH:
            {
                dma_sync_single_for_device(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_TO_DEVICE);
                dma_sync_single_for_cpu(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_FROM_DEVICE);
            }
            break;

            case GCKVIP_CACHE_INVALID:
            {
                dma_sync_single_for_cpu(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_FROM_DEVICE);
            }
            break;

            default:
                PRINTK_E("not support this flush cache type=%d\n", type);
                break;
            }
        }
        else {
            /* not supports wrap physical memory flush cache */
            struct sg_table *sgt = (struct sg_table *)node->flush_cache_handle;
            switch (type) {
            case GCKVIP_CACHE_CLEAN:
            {
                dma_sync_sg_for_device(kdriver->device, sgt->sgl, sgt->nents, DMA_TO_DEVICE);
            }
            break;

            case GCKVIP_CACHE_FLUSH:
            {
                dma_sync_sg_for_device(kdriver->device, sgt->sgl, sgt->nents, DMA_TO_DEVICE);
                dma_sync_sg_for_cpu(kdriver->device, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);
            }
            break;

            case GCKVIP_CACHE_INVALID:
            {
                dma_sync_sg_for_cpu(kdriver->device, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);
            }
            break;

            default:
                PRINTK_E("not support this flush cache type=%d\n", type);
                break;
            }
        }
    }
    else {
        PRINTK_E("alloctor_flush_cache flush handle is NULL\n");
        status = VIP_ERROR_FAILURE;
    }
#endif

    return status;
}
#endif

static vip_status_e map_user_logical(
    struct vm_area_struct *vma,
    struct page **pages,
    vip_uint32_t num_pages,
    gckvip_video_mem_alloc_flag_e alloc_flag
    )
{
    vip_status_e status = VIP_SUCCESS;
    unsigned long start = vma->vm_start;
    vip_uint32_t i = 0;

    vma->vm_flags |= gcdVM_FLAGS;

    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE) {
        #if USE_MEM_WRITE_COMOBINE
        vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
        #elif USE_MEM_NON_CACHE
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
        #endif
    }

    for (i = 0; i < num_pages; i++) {
        unsigned long pfn = page_to_pfn(pages[i]);
        if (remap_pfn_range(vma,
                            start,
                            pfn,
                            PAGE_SIZE,
                            vma->vm_page_prot) < 0) {
            PRINTK_E("%s(%d): remap_pfn_range error\n", __FUNCTION__, __LINE__);
            gcGoOnError(VIP_ERROR_IO);
        }

        start += PAGE_SIZE;
    }

onError:
    return status;
}

static vip_status_e gckvip_map_user(
    gckvip_dyn_allocate_node_t *node,
    struct page **pages,
    vip_uint32_t num_pages,
    gckvip_video_mem_alloc_flag_e alloc_flag,
    void **logical
    )
{
    void* user_logical = VIP_NULL;
    struct vm_area_struct *vma = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    user_logical = (void*)vm_mmap(NULL,
                        0L,
                        num_pages * PAGE_SIZE,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_NORESERVE,
                        0);
    if (IS_ERR(user_logical)) {
        user_logical = VIP_NULL;
        PRINTK_E("%s[%d], failed to map user logical\n", __FUNCTION__, __LINE__);
        dump_stack();
        gcGoOnError(VIP_ERROR_IO);
    }

    down_read(&current_mm_mmap_sem);
    vma = find_vma(current->mm, (unsigned long)user_logical);
    if (VIP_NULL == vma) {
        PRINTK_E("%s[%d], failed to find vma\n", __FUNCTION__, __LINE__);
        dump_stack();
        gcGoOnError(VIP_ERROR_IO);
    }

    gcOnError(map_user_logical(vma, pages, num_pages, alloc_flag));
    if (logical != VIP_NULL) {
        *logical = user_logical;
    }
    node->user_logical = (vip_uint8_t*)user_logical;
    node->mem_flag |= GCVIP_MEM_FLAG_MAP_USER;

onError:
    up_read(&current_mm_mmap_sem);
    return status;
}

/*
@brief allocate memory from system. get congiguous physical memory
@param context, the contect of kernel space.
@param node, dynamic allocate noe.
@param align, the size of alignment for this video memory.
@param alloc_flag, allocate flags. see gckvip_video_mem_alloc_flag_e.
*/
static vip_status_e gckvip_allocator_dyn_alloc_contiguous(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    )
{
    vip_status_e status = VIP_SUCCESS;
    size_t num_pages = 0;
    u32 gfp = GFP_KERNEL | __GFP_HIGHMEM | __GFP_NOWARN;
    struct page **pages = VIP_NULL;
    struct page *contiguous_pages = VIP_NULL;
    size_t i = 0;

    num_pages = GetPageCount(PAGE_ALIGN(node->size));
    node->num_pages = (vip_uint32_t)num_pages;
    if (0 == num_pages) {
        PRINTK_E("fail contiguous alloc page num is 0, size=%d\n", node->size);
        return VIP_ERROR_FAILURE;
    }

    gcOnError(gckvip_os_allocate_memory(sizeof(struct page *) * num_pages, (void**)&pages));
    gckvip_os_zero_memory(pages, sizeof(struct page *) * num_pages);

    /* remove __GFP_HIGHMEM bit, limit to 4G */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR) {
        gfp &= ~__GFP_HIGHMEM;
        /*add __GFP_DMA32 or __GFP_DMA bit */
        #if defined(CONFIG_ZONE_DMA32) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
        gfp |= __GFP_DMA32;
        #else
        gfp |= __GFP_DMA;
        #endif
        node->mem_flag |= GCVIP_MEM_FLAG_4GB_ADDR;
    }

    node->physical_num = 1;

    gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * node->physical_num,
                                        (void**)&node->physical_table));
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint32_t) * node->physical_num,
                                        (void**)&node->size_table));
    gckvip_os_zero_memory(node->size_table, sizeof(vip_uint32_t) * node->physical_num);
    gcOnError(gckvip_os_allocate_memory(sizeof(vip_uint8_t) * node->physical_num,
                                       (void**)&node->is_exact_page));
    gckvip_os_zero_memory(node->is_exact_page, sizeof(vip_uint8_t) * node->physical_num);

    /* allocate contiguous physical memory, only need base address */
    {
        phy_address_t cpy_physical = 0;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        {
        void *addr = alloc_pages_exact(PAGE_ALIGN(node->size), gfp | __GFP_NORETRY);
        contiguous_pages = addr ? virt_to_page(addr) : VIP_NULL;
        }
    #endif
        if (VIP_NULL == contiguous_pages) {
            int order = get_order(PAGE_ALIGN(node->size));
            if (order >= MAX_ORDER) {
                PRINTK_E("contiguous alloc contiguous memory order is bigger than MAX, %d > %d\n",
                          order, MAX_ORDER);
                gcGoOnError(VIP_ERROR_FAILURE);
            }
            contiguous_pages = alloc_pages(gfp, order);
            if (VIP_NULL == contiguous_pages) {
                PRINTK_E("fail to alloc pages order=%d\n", order);
                gcGoOnError(VIP_ERROR_FAILURE);
            }
            node->is_exact_page[0] = vip_false_e;
        }
        else {
            node->is_exact_page[0] = vip_true_e;
        }
        cpy_physical = page_to_phys(nth_page(contiguous_pages, 0));
        node->physical_table[0] = gckvip_drv_get_vipphysical(cpy_physical);
        node->size_table[0] = PAGE_ALIGN(node->size);
    }

    for (i = 0; i < num_pages; i++) {
        pages[i] = nth_page(contiguous_pages, i);
        /* Sanity check pages */
        if (VIP_NULL == pages[i]) {
            PRINTK_E("fail contiguous alloc page %d is null, num_pages=%d\n", i, num_pages);
            gcGoOnError(VIP_ERROR_IO);
        }
    }

    /* map kernel space */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL) {
        pgprot_t pgprot;
        if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE) {
            #if USE_MEM_WRITE_COMOBINE
            pgprot = pgprot_writecombine(PAGE_KERNEL);
            #elif USE_MEM_NON_CACHE
            pgprot = pgprot_noncached(PAGE_KERNEL);
            #endif
        }
        else {
            pgprot = PAGE_KERNEL;
        }

        node->kerl_logical = vmap(pages, num_pages, 0, pgprot);
        if (node->kerl_logical == VIP_NULL) {
            PRINTK_E("contiguous fail to vmap physical to kernel space\n");
            gcGoOnError(VIP_ERROR_IO);
        }
        node->mem_flag |= GCVIP_MEM_FLAG_MAP_KERNEL_LOGICAL;
    }

    /* map user space */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_USER) {
        gcOnError(gckvip_map_user(node, pages, num_pages, alloc_flag, VIP_NULL));
    }

    /* init flush cache */
#if FLUSH_CACHE_HOOK
    {
        dma_addr_t *dma_addr = VIP_NULL;
        gckvip_os_allocate_memory(sizeof(dma_addr_t), (void**)&node->flush_cache_handle);
        if (!node->flush_cache_handle) {
            PRINTK_E("contiguous failed to alloca memory for flush cache handle\n");
            gcGoOnError(VIP_ERROR_IO);
        }
        dma_addr = (dma_addr_t*)node->flush_cache_handle;
        *dma_addr = dma_map_page(kdriver->device, contiguous_pages, 0,
                                 num_pages * PAGE_SIZE, DMA_BIDIRECTIONAL);
        if (dma_mapping_error(kdriver->device, *dma_addr)) {
            PRINTK_E("contiguous fail to dma map page\n");
            gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
        }
        dma_sync_single_for_device(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_TO_DEVICE);
        dma_sync_single_for_cpu(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_FROM_DEVICE);
    }
#else
    node->mem_flag |= GCVIP_MEM_FLAG_NONE_CACHE;
#endif
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE) {
        node->mem_flag |= GCVIP_MEM_FLAG_NONE_CACHE;
    }

    PRINTK_D("contiguous dyn alloc kernel address=0x%"PRPx", user address=0x%"PRPx", "
             "size=0x%x, num_pages=%d, physical=0x%"PRIx64", handle=0x%"PRPx", flag=0x%x\n",
             node->kerl_logical, node->user_logical, node->size, num_pages,
             node->physical_table[0], contiguous_pages, node->mem_flag);

    node->alloc_handle = (void*)(contiguous_pages);

    if (pages != VIP_NULL) {
        gckvip_os_free_memory((void*)pages);
        pages = VIP_NULL;
    }

    return status;

onError:
    if (node->kerl_logical != VIP_NULL) {
        vunmap((void *)node->kerl_logical);
        node->kerl_logical = VIP_NULL;
    }
    if (contiguous_pages) {
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        if (node->is_exact_page[0]) {
            free_pages_exact(page_address(contiguous_pages), PAGE_ALIGN(node->size));
        }
        else
    #endif
        {
            __free_pages(contiguous_pages, get_order(PAGE_ALIGN(node->size)));
        }
    }
    node->alloc_handle = VIP_NULL;

    if (node->size_table != VIP_NULL) {
       gckvip_os_free_memory((void*)node->size_table);
       node->size_table = VIP_NULL;
    }
    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory((void*)node->physical_table);
        node->physical_table = VIP_NULL;
    }
    if (pages != VIP_NULL) {
        gckvip_os_free_memory((void*)pages);
        pages = VIP_NULL;
    }
#if FLUSH_CACHE_HOOK
    if (node->flush_cache_handle != VIP_NULL) {
        gckvip_os_free_memory((void*)node->flush_cache_handle);
        node->flush_cache_handle = VIP_NULL;
    }
#endif
    return status;
}

static vip_status_e gckvip_allocator_dyn_free_contiguous(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct page *pages;

    if ((VIP_NULL == context) || (VIP_NULL == node)) {
        PRINTK_E("failed free contiguous dyn free, parameter is NULL\n");
        return VIP_ERROR_IO;
    }

    /* un-init flush cache */
#if FLUSH_CACHE_HOOK
    if (node->flush_cache_handle != VIP_NULL) {
        dma_addr_t *dma_addr = (dma_addr_t*)node->flush_cache_handle;
        dma_unmap_page(kdriver->device, *dma_addr, node->num_pages << PAGE_SHIFT, DMA_FROM_DEVICE);
        gckvip_os_free_memory((void*)node->flush_cache_handle);
        node->flush_cache_handle = VIP_NULL;
    }
#endif

    /* unmap user space */
    if (node->mem_flag & GCVIP_MEM_FLAG_MAP_USER) {
        if (vm_munmap((unsigned long)node->user_logical & PAGE_MASK, node->num_pages * PAGE_SIZE) < 0) {
            PRINTK_E("failed to vm munmap user space contiguous memory\n");
            status = VIP_ERROR_IO;
        }
    }

    /* ummap kernel space */
    if ((node->mem_flag & GCVIP_MEM_FLAG_MAP_KERNEL_LOGICAL) && (node->kerl_logical != VIP_NULL)) {
        vunmap((void *)node->kerl_logical);
        node->kerl_logical = VIP_NULL;
    }

    pages = (struct page *)node->alloc_handle;
    PRINTK_D("dyn free contiguous memory handle=0x%"PRPx"\n", node->alloc_handle);

    if (pages) {
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        if (node->is_exact_page[0]) {
            free_pages_exact(page_address(pages), PAGE_ALIGN(node->size));
        }
        else
        #endif
        {
            __free_pages(pages, get_order(PAGE_ALIGN(node->size)));
        }
    }

    node->alloc_handle = VIP_NULL;

    gckvip_os_free_memory((void*)node->physical_table);
    node->physical_table = VIP_NULL;

    gckvip_os_free_memory((void*)node->size_table);
    node->size_table = VIP_NULL;
    if (node->is_exact_page != VIP_NULL) {
        gckvip_os_free_memory((void*)node->is_exact_page);
        node->is_exact_page = VIP_NULL;
    }

    return status;
}

/*
@brief mapping user space logical address.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
@param, logical, use space logical address mapped.
*/
vip_status_e gckvip_allocator_map_userlogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    void **logical
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct page **pages = VIP_NULL;
    vip_uint32_t i = 0, j = 0;
    void *user_logical = VIP_NULL;
    vip_uint32_t num_pages = VIP_NULL;
    if (VIP_NULL == node) {
        PRINTK_E("map user logical node is NULL\n");
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    num_pages = node->num_pages;

    gcOnError(gckvip_os_allocate_memory(sizeof(struct page *) * num_pages, (void**)&pages));

    if (node->mem_flag & GCVIP_MEM_FLAG_CONTIGUOUS) {
        struct page *contiguous_pages = (struct page *)node->alloc_handle;
        for (i = 0; i < num_pages; i++) {
            pages[i] = nth_page(contiguous_pages, i);
        }
        gcOnError(gckvip_map_user(node, pages, num_pages, node->alloc_flag, &user_logical));
    }
    else if (node->mem_flag & GCVIP_MEM_FLAG_1M_CONTIGUOUS) {
        struct page **Pages1M = (struct page **)node->alloc_handle;
        vip_uint32_t num_pages_1M = node->physical_num;
        vip_uint32_t num = gcd1M_PAGE_SIZE / PAGE_SIZE;
        struct page *page;
        for (i = 0; i < num_pages_1M; i++) {
            for (j = 0; j < num; j++) {
                page = nth_page(Pages1M[i], j);
                pages[i * num + j] = page;
            }
        }
        gcOnError(gckvip_map_user(node, pages, num_pages, node->alloc_flag, &user_logical));
    }
#if vpmdENABLE_CREATE_BUF_FD
    else if (node->mem_flag & GCVIP_MEM_FLAG_USER_MEMORY_FD) {
        gckvip_dma_buf_info_t *import = (gckvip_dma_buf_info_t *)node->alloc_handle;
        struct sg_table *sgt = import->sgt;
        struct scatterlist *s = VIP_NULL;
        phy_address_t *cpu_physical = VIP_NULL;
        vip_uint32_t count = 0;
        vip_uint32_t offset = 0;
        vip_uint8_t *logical_tmp = VIP_NULL;
        gcOnError(gckvip_os_allocate_memory(sizeof(phy_address_t) * num_pages, (void**)&cpu_physical));
        i = 0;
        for_each_sg(sgt->sgl, s, sgt->orig_nents, i) {
            vip_uint32_t num = (sg_dma_len(s) + PAGE_SIZE - 1) / PAGE_SIZE;
            for (j = 0; j < num; j++) {
                cpu_physical[count] = sg_dma_address(s) + j * PAGE_SIZE;
                pages[count] = pfn_to_page(__phys_to_pfn(cpu_physical[count] & PAGE_MASK));
                count++;
            }
        }
        offset = cpu_physical[0] - (cpu_physical[0] & PAGE_MASK);
        gcOnError(gckvip_map_user(node, pages, num_pages, node->alloc_flag, (void**)&logical_tmp));
        logical_tmp += offset;
        user_logical = (void*)logical_tmp;
        if (cpu_physical != VIP_NULL) {
            gckvip_os_free_memory((void*)cpu_physical);
        }
    }
#endif
#if defined (USE_LINUX_CMA)
    else if (node->mem_flag & GCVIP_MEM_FLAG_CMA) {
        vip_uint8_t *logical_tmp = VIP_NULL;
        phy_address_t cpu_physical = GCKVIPPTR_TO_UINT64(node->alloc_handle);
        vip_uint32_t offset = cpu_physical - (cpu_physical & PAGE_MASK);
        gcOnError(gckvip_allocator_cma_map_userlogical(node, num_pages, cpu_physical, (void**)&logical_tmp));
        logical_tmp += offset;
        user_logical = (void*)logical_tmp;
    }
#endif
    else if (node->mem_flag & GCVIP_MEM_FLAG_USER_MEMORY_PHYSICAL) {
        struct page *page = VIP_NULL;
        unsigned long pfn = __phys_to_pfn(gckvip_drv_get_cpuphysical(node->physical_table[0]));
        page = pfn_to_page(pfn);
        for (i = 0; i < num_pages; i++) {
            pages[i] = nth_page(page, i);
        }
        gcOnError(gckvip_map_user(node, pages, num_pages, node->alloc_flag, &user_logical));
    }
    else {
        struct page **pages_tmp = (struct page **)node->alloc_handle;
        gcOnError(gckvip_map_user(node, pages_tmp, num_pages, node->alloc_flag, &user_logical));
    }

    if (logical != VIP_NULL) {
        *logical = user_logical;
    }
    node->user_logical = user_logical;
    PRINTK_D("allocator map user logical=0x%"PRPx", num_pages=%d\n", *logical, num_pages);

onError:
    if (pages != VIP_NULL) {
       gckvip_os_free_memory((void*)pages);
       pages = VIP_NULL;
    }
    return status;
}

vip_status_e gckvip_allocator_unmap_userlogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t *ptr = VIP_NULL;

    if (node->user_logical != VIP_NULL) {
        ptr = GCKVIPUINT64_TO_PTR((unsigned long)node->user_logical & PAGE_MASK);
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
        if (access_ok(ptr, node->num_pages * PAGE_SIZE)) {
        #else
        if (access_ok(VERIFY_WRITE, ptr, node->num_pages * PAGE_SIZE)) {
        #endif
            if (vm_munmap((unsigned long)node->user_logical & PAGE_MASK, node->num_pages * PAGE_SIZE) < 0) {
                PRINTK_E("failed to vm munmap user space\n");
                status = VIP_ERROR_IO;
            }
        }
        else {
            PRINTK_E("unmap user logical=0x%"PRPx" can not access\n", node->user_logical);
        }
        node->user_logical = VIP_NULL;
    }
    return status;
}

/*
@brief allocate memory from system.
       get the physical address table each page.
       get size table of eache page
@param context, the contect of kernel space.
@param node, dynamic allocate node.
@param align, the size of alignment for this video memory.
@param flag the flag of this video memroy. see gckvip_video_mem_alloc_flag_e.
*/
vip_status_e gckvip_allocator_dyn_alloc(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    )
{
    vip_status_e status = VIP_SUCCESS;

    PRINTK_D("video memory dynamic alloc size=0x%x\n", node->size);
    node->alloc_flag = alloc_flag;
    node->mem_flag = GCVIP_VIDEO_MEM_ALLOC_NONE;
    if (0 == node->size) {
        PRINTK_E("fail to dynamic alloc video memory size is 0\n");
        GCKVIP_DUMP_STACK();
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

#if vpmdENABLE_MMU
    status = gckvip_allocator_dyn_alloc_ext(context, node, align, alloc_flag);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to dynamic alloc video memory size=%dbytes, status=%d\n", node->size, status);
        gcGoOnError(status);
    }
#else
    /* alloca contiguous physical memory, mmu is disabled */
    status = gckvip_allocator_dyn_alloc_contiguous(context, node, align, alloc_flag);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to alloc contiguous memory size=%dbytes, alloc_flag=0x%x, status=%d\n",
                  node->size, alloc_flag,  status);
        gcGoOnError(status);
    }
    else {
        node->mem_flag |= GCVIP_MEM_FLAG_CONTIGUOUS;
    }
#endif

onError:
    return status;
}

/*
@brief free a dynamic allocate memory.
@param context, the contect of kernel space.
@param node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_dyn_free(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdENABLE_MMU
    status = gckvip_allocator_dyn_free_ext(context, node);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to free dyn memory, status=%d\n", status);
    }
#else
    /* contiguous memory */
    if (node->mem_flag & GCVIP_MEM_FLAG_CONTIGUOUS) {
        status = gckvip_allocator_dyn_free_contiguous(context, node);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to free dynamic contiguous memory\n");
        }
    }
    else {
        PRINTK_E("failed to free dyn memory, mem_flag=0x%x\n", node->mem_flag);
    }
#endif

    return status;
}


/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2024 Vivante Corporation
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
*    Copyright (C) 2017 - 2024 Vivante Corporation
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

#include <vip_drv_mem_allocator.h>
#include <vip_drv_video_memory.h>
#include <vip_drv_mem_allocator_common.h>
#include "vip_drv_device_driver.h"

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_VIDEO_MEMORY


static vip_status_e vipdrv_kernel_logical_map_continue(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_t* allocator = ptr->allocator;
    vip_physical_t physical = allocator->callback.phy_vip2cpu(ptr->physical_table[0]);
    vip_uint32_t offset = physical - (physical & PAGE_MASK);
    void *logical = VIP_NULL;
    vip_uint8_t *prt_tmp = VIP_NULL;
    struct page **pages = VIP_NULL;
    vip_bool_e no_cache = (VIPDRV_MEM_FLAG_NONE_CACHE & ptr->mem_flag) ? vip_true_e : vip_false_e;
    vip_uint32_t page_count = 0;

    vipdrv_get_page_count(ptr, &page_count);
    vipOnError(vipdrv_os_allocate_memory(sizeof(struct page*) * page_count, (void**)&pages));
    vipdrv_os_zero_memory(pages, sizeof(struct page*) * page_count);
    vipOnError(vipdrv_fill_pages(ptr, pages));
    vipOnError(vipdrv_map_kernel_page(page_count, pages, &logical, no_cache));
    vipdrv_os_free_memory(pages);

    prt_tmp = (vip_uint8_t*)logical;
    ptr->kerl_logical = (void*)(prt_tmp + offset);
    ptr->mem_flag |= VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;
    PRINTK_I("map logical kernel continue, address=0x%"PRPx",\n", ptr->kerl_logical);
    return status;
onError:
    PRINTK_E("fail to map kernel logical continue\n");
    if (VIP_NULL != pages) {
        vipdrv_os_free_memory(pages);
        pages = VIP_NULL;
    }
    return status;
}

static vip_status_e vipdrv_kernel_logical_unmap_continue(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;

    vipdrv_unmap_kernel_page(VIPDRV_UINT64_TO_PTR(VIPDRV_PTR_TO_UINT64(ptr->kerl_logical) & PAGE_MASK));
    ptr->kerl_logical = VIP_NULL;
    ptr->mem_flag &= ~VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;

    return status;
}

static vip_status_e vipdrv_user_logical_map_continue(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_t* allocator = ptr->allocator;
    vip_physical_t physical = allocator->callback.phy_vip2cpu(ptr->physical_table[0]);
    vip_uint32_t page_offset = physical - (physical & PAGE_MASK);
    void* user_logical = VIP_NULL;
    struct page **pages = VIP_NULL;
    vip_bool_e no_cache = (VIPDRV_MEM_FLAG_NONE_CACHE & ptr->mem_flag) ? vip_true_e : vip_false_e;
    vip_uint32_t page_count = 0;

    vipdrv_get_page_count(ptr, &page_count);
    vipOnError(vipdrv_os_allocate_memory(sizeof(struct page*) * page_count, (void**)&pages));
    vipdrv_os_zero_memory(pages, sizeof(struct page*) * page_count);
    vipOnError(vipdrv_fill_pages(ptr, pages));
    vipOnError(vipdrv_map_user(page_count, pages, &user_logical, no_cache));
    vipdrv_os_free_memory(pages);

    ptr->user_logical = (void*)((vip_uint8_t*)user_logical + page_offset);
    ptr->mem_flag |= VIPDRV_MEM_FLAG_MAP_USER_LOGICAL;
    ptr->pid = vipdrv_os_get_pid();
    return status;
onError:
    PRINTK_E("fail to map user logical continue\n");
    if (VIP_NULL != pages) {
        vipdrv_os_free_memory(pages);
        pages = VIP_NULL;
    }
    return status;
}

static vip_status_e vipdrv_user_logical_unmap_continue(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    vip_uint32_t page_count = 0;

    if (ptr->pid == vipdrv_os_get_pid()) {
        vipdrv_get_page_count(ptr, &page_count);
        vipdrv_unmap_user(page_count, VIPDRV_UINT64_TO_PTR(VIPDRV_PTR_TO_UINT64(ptr->user_logical) & PAGE_MASK));
        ptr->user_logical = VIP_NULL;
        ptr->mem_flag &= ~VIPDRV_MEM_FLAG_MAP_USER_LOGICAL;
        ptr->pid = 0;
    }
    return status;
}

#if vpmdENABLE_FLUSH_CPU_CACHE
static vip_status_e vipdrv_flush_cache_continue(
    vipdrv_video_mem_handle_t* handle,
    vip_uint8_t type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    vip_uint32_t page_count = 0;
    if (0x00 == (VIPDRV_MEM_FLAG_NONE_CACHE & ptr->mem_flag)) {
        vipdrv_get_page_count(ptr, &page_count);
        status = flush_cache_continue(ptr->flush_handle, page_count << PAGE_SHIFT, type);
    }
    return status;
}
#endif

static vip_status_e vipdrv_mem_alloc_continue(
    vipdrv_video_mem_handle_t* handle,
    vipdrv_alloc_param_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_allocator_alloc_param_t* alloc_param = (vipdrv_allocator_alloc_param_t*)param;
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_t* allocator = ptr->allocator;
    vip_uint32_t page_count = 0;
    u32 gfp = GFP_KERNEL | __GFP_HIGHMEM | __GFP_NOWARN;
    struct page *contiguous_pages = VIP_NULL;

    ptr->size = alloc_param->size;
    ptr->align = alloc_param->align;
    ptr->mem_flag = alloc_param->mem_flag;

    if (0 == ptr->size) {
        PRINTK_E("fail contiguous alloc page num is 0, size=%d\n", ptr->size);
        return VIP_ERROR_FAILURE;
    }

    /* remove __GFP_HIGHMEM bit, limit to 4G */
    if (VIPDRV_MEM_FLAG_4GB_ADDR_PA & ptr->mem_flag) {
        gfp &= ~__GFP_HIGHMEM;
        /*add __GFP_DMA32 or __GFP_DMA bit */
        #if defined(CONFIG_ZONE_DMA32) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
        gfp |= __GFP_DMA32;
        #else
        gfp |= __GFP_DMA;
        #endif
    }

    ptr->physical_num = 1;
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_physical_t) * ptr->physical_num,
                                        (void**)&ptr->physical_table));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_size_t) * ptr->physical_num,
                                        (void**)&ptr->size_table));
    vipdrv_os_zero_memory(ptr->size_table, sizeof(vip_size_t) * ptr->physical_num);

    /* allocate contiguous physical memory, only need base address */
    {
        vip_physical_t cpu_physical = 0;
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        {
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
			gfp &= ~__GFP_HIGHMEM;
			#endif
            void *addr = alloc_pages_exact(PAGE_ALIGN(ptr->size), gfp | __GFP_NORETRY);
            contiguous_pages = addr ? virt_to_page(addr) : VIP_NULL;

        }
        #else
        {
            int order = get_order(PAGE_ALIGN(ptr->size));
            #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
            if (order >= MAX_PAGE_ORDER)
            #else
            if (order >= MAX_ORDER)
            #endif
            {
                #if DEBUG_ALLOCATOR
                PRINTK("contiguous alloc contiguous memory order is bigger than MAX, %d > %d\n",
                    order, MAX_ORDER);
                #endif
                vipGoOnError(VIP_ERROR_FAILURE);
            }
            contiguous_pages = alloc_pages(gfp, order);
        }
        #endif
        if (VIP_NULL == contiguous_pages) {
            #if DEBUG_ALLOCATOR
            PRINTK("fail to alloc pages size=%d\n", ptr->size);
            #endif
            vipGoOnError(VIP_ERROR_FAILURE);
        }
        cpu_physical = page_to_phys(nth_page(contiguous_pages, 0));
        if (0 == cpu_physical) {
            #if DEBUG_ALLOCATOR
            PRINTK("error continue allocator cpu_physical address is 0\n");
            #endif
            vipGoOnError(VIP_ERROR_IO);
        }
        ptr->physical_table[0] = allocator->callback.phy_cpu2vip(cpu_physical);
        ptr->size_table[0] = PAGE_ALIGN(ptr->size);
    }

    vipdrv_get_page_count(ptr, &page_count);
    vipOnError(flush_cache_init_continue(&ptr->flush_handle, contiguous_pages, page_count, 0));
    if (VIPDRV_MEM_FLAG_NONE_CACHE & ptr->mem_flag) {
        flush_cache_destroy_continue(ptr->flush_handle, page_count);
        ptr->flush_handle = VIP_NULL;
    }

    ptr->mem_flag |= VIPDRV_MEM_FLAG_CONTIGUOUS;
    return status;
onError:
    flush_cache_destroy_continue(ptr->flush_handle, page_count);
    ptr->flush_handle = VIP_NULL;

    if (contiguous_pages) {
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        free_pages_exact(page_address(contiguous_pages), PAGE_ALIGN(ptr->size));
        #else
        __free_pages(contiguous_pages, get_order(PAGE_ALIGN(ptr->size)));
        #endif
    }

    if (VIP_NULL != ptr->size_table) {
       vipdrv_os_free_memory((void*)ptr->size_table);
       ptr->size_table = VIP_NULL;
    }
    if (VIP_NULL != ptr->physical_table) {
        vipdrv_os_free_memory((void*)ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }
    #if DEBUG_ALLOCATOR
    PRINTK("fail to alloc memory from allocator continue mem_flag=0x%x, size=%d\n",
        alloc_param->mem_flag, alloc_param->size);
    #endif
    return status;
}

static vip_status_e vipdrv_mem_free_continue(
    vipdrv_video_mem_handle_t* handle
    )
{
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_t* allocator = ptr->allocator;
    struct page *contiguous_pages;
    unsigned long pfn = 0;
    vip_uint32_t page_count = 0;

    if (VIP_NULL == ptr->physical_table) {
        return VIP_SUCCESS;
    }

    vipdrv_get_page_count(ptr, &page_count);

    if (ptr->kerl_logical) {
        vipdrv_kernel_logical_unmap_continue(handle);
    }

    if (ptr->user_logical) {
        vipdrv_user_logical_unmap_continue(handle);
    }

    flush_cache_destroy_continue(ptr->flush_handle, page_count);
    ptr->flush_handle = VIP_NULL;

    PRINTK_D("dyn free contiguous memory handle=0x%"PRPx"\n", ptr);
    pfn = __phys_to_pfn(allocator->callback.phy_vip2cpu(ptr->physical_table[0]));
    contiguous_pages = pfn_to_page(pfn);
    if (contiguous_pages) {
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        free_pages_exact(page_address(contiguous_pages), PAGE_ALIGN(ptr->size));
        #else
        __free_pages(contiguous_pages, get_order(PAGE_ALIGN(ptr->size)));
        #endif
    }

    if (VIP_NULL != ptr->size_table) {
        vipdrv_os_free_memory((void*)ptr->size_table);
        ptr->size_table = VIP_NULL;
    }
    vipdrv_os_free_memory((void*)ptr->physical_table);
    ptr->physical_table = VIP_NULL;

    return VIP_SUCCESS;
}

/*
@brief convert CPU physical to VIP physical.
@param cpu_physical. the physical address of CPU domain.
*/
static vip_physical_t vipdrv_phy_cpu2vip_continue(
    vip_physical_t cpu_physical
    )
{
    vip_physical_t vip_hysical = cpu_physical;
    return vip_hysical;
}

/*
@brief convert VIP physical to CPU physical.
@param vip_physical. the physical address of VIP domain.
*/
static vip_physical_t vipdrv_phy_vip2cpu_continue(
    vip_physical_t vip_physical
    )
{
    vip_physical_t cpu_hysical = vip_physical;
    return cpu_hysical;
}

/*
dynamic allocator:
    physical address are contiguous.
    A contigous physical address that is dynamic alloc from system.
*/
vip_status_e allocator_init_dyn_continue(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    )
{
    if (VIP_NULL == allocator) {
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    allocator->mem_flag = VIPDRV_MEM_FLAG_CONTIGUOUS |
                          VIPDRV_MEM_FLAG_4GB_ADDR_PA;
    allocator->name = name;
    allocator->alctor_type = type;
    allocator->device_vis = ~(vip_uint64_t)0;
    allocator->callback.allocate = vipdrv_mem_alloc_continue;
    allocator->callback.free = vipdrv_mem_free_continue;
    allocator->callback.map_kernel_logical = vipdrv_kernel_logical_map_continue;
    allocator->callback.unmap_kernel_logical = vipdrv_kernel_logical_unmap_continue;
    allocator->callback.map_user_logical = vipdrv_user_logical_map_continue;
    allocator->callback.unmap_user_logical = vipdrv_user_logical_unmap_continue;
#if vpmdENABLE_FLUSH_CPU_CACHE
    allocator->callback.flush_cache = vipdrv_flush_cache_continue;
#endif
    allocator->callback.phy_vip2cpu = vipdrv_phy_vip2cpu_continue;
    allocator->callback.phy_cpu2vip = vipdrv_phy_cpu2vip_continue;

    return VIP_SUCCESS;
}

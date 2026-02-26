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
#if vpmdENABLE_VIDEO_MEMORY_HEAP
#include <vip_drv_video_memory.h>
#include <vip_drv_mem_allocator_common.h>
#include "vip_drv_device_driver.h"

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_VIDEO_MEMORY


static vip_status_e vipdrv_kernel_logical_map_exclusive(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_t* allocator = ptr->allocator;
    vip_physical_t physical = allocator->callback.phy_vip2cpu(ptr->physical_table[0]);
    vip_size_t size = ptr->size;
    void *vaddr = VIP_NULL;
    vip_bool_e no_cache = (VIPDRV_MEM_FLAG_NONE_CACHE & ptr->mem_flag) ? vip_true_e : vip_false_e;

    if (vip_true_e == no_cache) {
        #if USE_MEM_WRITE_COMOBINE
        vaddr = ioremap_wc(physical, size);
        PRINTK_D("exclusive map kernel logical ioremap write-combine..\n");
        #else
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
        vaddr = ioremap(physical, size);
        PRINTK_D("exclusive map kernel logical ioremap..\n");
        #else
        vaddr = ioremap_nocache(physical, size);
        PRINTK_D("exclusive map kernel logical none-cache\n");
        #endif
        #endif
    }
    else {
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
        vaddr = ioremap_cache(physical, size);
        PRINTK_D("exclusive map kernel logical with cache...\n");
        #elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0)
        vaddr = ioremap_cached(physical, size);
        PRINTK_D("exclusive map kernel logical with cache...\n");
        #else
        PRINTK_D("exclusive map kernel logical not support cache...\n");
        #endif
    }

    if (!vaddr) {
        PRINTK("vipcore, fail to ioremap physical=0x%llx size=0x%x to kernel space\n",
                physical, size);
        vipGoOnError(VIP_ERROR_FAILURE);
    }
    PRINTK_I("heap exclusive map kernel logi address=0x%"PRPx", physical=0x%llx, size=0x%x\n",
            vaddr, physical, size);
    ptr->mem_flag |= VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;
    ptr->kerl_logical = (vip_uint8_t*)vaddr;
    return status;
onError:
    PRINTK_E("fail to map kernel logical exclusive\n");
    return status;
}

static vip_status_e vipdrv_kernel_logical_unmap_exclusive(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;

    iounmap((void*)ptr->kerl_logical);
    ptr->kerl_logical = VIP_NULL;
    ptr->mem_flag &= ~VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;

    return status;
}

static vip_status_e vipdrv_user_logical_map_exclusive(
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
    PRINTK_E("fail to map user logical exclusive\n");
    return status;
}

static vip_status_e vipdrv_user_logical_unmap_exclusive(
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
static vip_status_e vipdrv_flush_cache_exclusive(
    vipdrv_video_mem_handle_t* handle,
    vip_uint8_t type
    )
{
    vip_status_e status = VIP_SUCCESS;
 #if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_video_mem_handle_t* ptr = handle;
    vip_uint32_t page_count = 0;
    if (0x00 == (VIPDRV_MEM_FLAG_NONE_CACHE & ptr->mem_flag)) {
        vipdrv_get_page_count(ptr, &page_count);
        status = flush_cache_continue(ptr->flush_handle, page_count << PAGE_SHIFT, type);
    }
 #endif
    return status;
}
#endif

static vip_status_e vipdrv_alloc_heap_exclusive(
    vipdrv_video_mem_handle_t* handle,
    vipdrv_alloc_param_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_allocator_t* allocator = ptr->allocator;
    vip_physical_t physical = allocator->callback.phy_vip2cpu(ptr->physical_table[0]);
    vip_uint32_t page_offset = physical - (physical & PAGE_MASK);
    vip_uint32_t page_count = 0;
    struct page *page;
    unsigned long pfn = __phys_to_pfn(physical);
    page = pfn_to_page(pfn);

    vipdrv_get_page_count(ptr, &page_count);
    vipOnError(flush_cache_init_continue(&ptr->flush_handle, page, page_count, page_offset));
    if (VIPDRV_MEM_FLAG_NONE_CACHE & ptr->mem_flag) {
        flush_cache_destroy_continue(ptr->flush_handle, page_count);
        ptr->flush_handle = VIP_NULL;
    }
    ptr->mem_flag |= VIPDRV_MEM_FLAG_CONTIGUOUS;
    PRINTK_D("exclusive alloc vip_phy=0x%llx, cpu_phy=0x%llx, size=0x%llx\n",
             ptr->physical_table[0], physical, ptr->size);
    return status;
onError:
    status = VIP_SUCCESS;
    ptr->mem_flag |= VIPDRV_MEM_FLAG_NONE_CACHE;
    flush_cache_destroy_continue(ptr->flush_handle, page_count);
    #if DEBUG_ALLOCATOR
    PRINTK("fail to alloc memory from allocator exclusive\n");
    #endif
#else
    ptr->mem_flag |= VIPDRV_MEM_FLAG_NONE_CACHE;
    ptr->mem_flag |= VIPDRV_MEM_FLAG_CONTIGUOUS;
#endif
    ptr->flush_handle = VIP_NULL;
    return status;
}

static vip_status_e vipdrv_free_heap_exclusive(
    vipdrv_video_mem_handle_t* handle
    )
{
    vipdrv_video_mem_handle_t* ptr = handle;
    vip_uint32_t page_count = 0;
    vipdrv_get_page_count(ptr, &page_count);

    if (VIP_NULL != ptr->kerl_logical) {
        vipdrv_kernel_logical_unmap_exclusive(handle);
        ptr->kerl_logical = VIP_NULL;
    }

    if (VIP_NULL != ptr->user_logical) {
        vipdrv_user_logical_unmap_exclusive(handle);
        ptr->user_logical = VIP_NULL;
    }

    if (ptr->flush_handle != VIP_NULL) {
        flush_cache_destroy_continue(ptr->flush_handle, page_count);
        ptr->flush_handle = VIP_NULL;
    }

    return VIP_SUCCESS;
}

/* healp allocator(exclusive)
   This memory has not manage by Linux system.
   exclusive memory should be:
       1. Device memory on PCIE card.
       2. Reserved in DTS, and reserved by Linux System.
*/
vip_status_e allocator_init_heap_exclusive(
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
    allocator->callback.allocate = vipdrv_alloc_heap_exclusive;
    allocator->callback.free = vipdrv_free_heap_exclusive;
    allocator->callback.map_kernel_logical = vipdrv_kernel_logical_map_exclusive;
    allocator->callback.unmap_kernel_logical = vipdrv_kernel_logical_unmap_exclusive;
    allocator->callback.map_user_logical = vipdrv_user_logical_map_exclusive;
    allocator->callback.unmap_user_logical = vipdrv_user_logical_unmap_exclusive;
#if vpmdENABLE_FLUSH_CPU_CACHE
    allocator->callback.flush_cache = vipdrv_flush_cache_exclusive;
#endif

    /* the phy_vip2cpu and phy_cpu2vip callback will be overwirte in vipdrv_alloc_mgt_heap_init() function
       if set heap_infos.ops.phy_cpu2vip/phy_vip2cpu in vipdrv_drv_get_memory_config().
    */
    allocator->callback.phy_vip2cpu = vipdrv_drv_get_cpuphysical;
    allocator->callback.phy_cpu2vip = vipdrv_drv_get_vipphysical;

    return VIP_SUCCESS;
}
#endif

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


static vip_status_e vipdrv_kernel_logical_wrap_physical(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_ERROR_NOT_SUPPORTED;
    PRINTK_E("not support map kernel logical wrap physical\n");
    return status;
}

static vip_status_e vipdrv_kernel_logical_unwrap_physical(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    ptr->kerl_logical = VIP_NULL;
    ptr->mem_flag &= ~VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;
    return status;
}

static vip_status_e vipdrv_user_logical_wrap_physical(
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
    PRINTK_E("fail to map user logical wrap physical\n");
    if (VIP_NULL != pages) {
        vipdrv_os_free_memory(pages);
        pages = VIP_NULL;
    }
    return status;
}

static vip_status_e vipdrv_user_logical_unwrap_physical(
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
static vip_status_e vipdrv_flush_cache_wrap_physical(
    vipdrv_video_mem_handle_t* handle,
    vip_uint8_t type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    if (0x00 == (VIPDRV_MEM_FLAG_NONE_CACHE & ptr->mem_flag)) {
        status = flush_cache_pages(ptr->flush_handle, type);
    }
    return status;
}
#endif

static vip_status_e vipdrv_mem_alloc_wrap_physical(
    vipdrv_video_mem_handle_t* handle,
    vipdrv_alloc_param_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_t* allocator = ptr->allocator;
    vip_size_t total_size = 0;
    vip_uint32_t i = 0;
    vipdrv_allocator_wrapphy_param_t *wrap_param = (vipdrv_allocator_wrapphy_param_t*)param;
    vip_uint32_t physical_num = wrap_param->phy_num;
    vip_uint32_t page_offset = wrap_param->phy_table[0] - (wrap_param->phy_table[0] & PAGE_MASK);
    struct page **pages = VIP_NULL;
    vip_uint32_t page_count = 0;

    ptr->physical_num = physical_num;
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_physical_t) * physical_num, (void**)&ptr->physical_table));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_size_t) * physical_num, (void**)&ptr->size_table));
    vipdrv_os_zero_memory(ptr->physical_table, sizeof(vip_physical_t) * physical_num);
    vipdrv_os_zero_memory(ptr->size_table, sizeof(vip_size_t) * physical_num);

    /* check physical table is valid */
    for (i = 0; i < ptr->physical_num; i++) {
        ptr->physical_table[i] = allocator->callback.phy_cpu2vip(wrap_param->phy_table[i]);
        ptr->size_table[i] = wrap_param->size_table[i];
        total_size += wrap_param->size_table[i];

        if (0 != i) {
            if (ptr->physical_table[i] % PAGE_SIZE) {
                PRINTK_E("physical index %d: start addr 0x%x not align to 0x%x\n",
                    i, ptr->physical_table[i], PAGE_SIZE);
                vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
            }
        }
        if ((ptr->physical_num - 1) != i) {
            if ((ptr->physical_table[i] + ptr->size_table[i]) % PAGE_SIZE) {
                PRINTK_E("physical index %d: end addr 0x%x not align to 0x%x\n",
                    i, ptr->physical_table[i] + ptr->size_table[i], PAGE_SIZE);
                vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
            }
        }
    }
    ptr->size = total_size;

    if (0x00 == (VIPDRV_MEM_FLAG_NONE_CACHE & ptr->mem_flag)) {
        vipdrv_get_page_count(ptr, &page_count);
        vipOnError(vipdrv_os_allocate_memory(sizeof(struct page*) * page_count, (void**)&pages));
        vipdrv_os_zero_memory(pages, sizeof(struct page*) * page_count);
        vipOnError(vipdrv_fill_pages(ptr, pages));

        vipOnError(flush_cache_init_pages(&ptr->flush_handle, pages, page_count,
            page_offset, page_count << PAGE_SHIFT));

        vipdrv_os_free_memory(pages);
    }
    return status;
onError:
    flush_cache_destroy_pages(ptr->flush_handle);
    ptr->flush_handle = VIP_NULL;
    if (VIP_NULL != ptr->physical_table) {
        vipdrv_os_free_memory(ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }
    if (VIP_NULL != ptr->size_table) {
        vipdrv_os_free_memory(ptr->size_table);
        ptr->size_table = VIP_NULL;
    }
    if (VIP_NULL != pages) {
        vipdrv_os_free_memory(pages);
        pages = VIP_NULL;
    }
    return status;
}

static vip_status_e vipdrv_mem_free_wrap_physical(
    vipdrv_video_mem_handle_t* handle
    )
{
    vipdrv_video_mem_handle_t* ptr = handle;

    if (VIP_NULL == ptr->physical_table) {
        return VIP_SUCCESS;
    }

    if (ptr->user_logical) {
        vipdrv_user_logical_unwrap_physical(handle);
    }

    if (VIP_NULL != ptr->size_table) {
        vipdrv_os_free_memory((void*)ptr->size_table);
        ptr->size_table = VIP_NULL;
    }

    flush_cache_destroy_pages(ptr->flush_handle);
    ptr->flush_handle = VIP_NULL;

    if (VIP_NULL != ptr->physical_table) {
        vipdrv_os_free_memory((void*)ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }

    return VIP_SUCCESS;
}

vip_status_e allocator_init_wrap_user_physical(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    )
{
    if (VIP_NULL == allocator) {
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    allocator->mem_flag = VIPDRV_MEM_FLAG_NONE;
    allocator->name = name;
    allocator->alctor_type = type;
    allocator->device_vis = ~(vip_uint64_t)0;
    allocator->callback.allocate = vipdrv_mem_alloc_wrap_physical;
    allocator->callback.free = vipdrv_mem_free_wrap_physical;
    allocator->callback.map_kernel_logical = vipdrv_kernel_logical_wrap_physical;
    allocator->callback.unmap_kernel_logical = vipdrv_kernel_logical_unwrap_physical;
    allocator->callback.map_user_logical = vipdrv_user_logical_wrap_physical;
    allocator->callback.unmap_user_logical = vipdrv_user_logical_unwrap_physical;
#if vpmdENABLE_FLUSH_CPU_CACHE
    allocator->callback.flush_cache = vipdrv_flush_cache_wrap_physical;
#endif
    allocator->callback.phy_vip2cpu = vipdrv_drv_get_cpuphysical;
    allocator->callback.phy_cpu2vip = vipdrv_drv_get_vipphysical;

    return VIP_SUCCESS;
}

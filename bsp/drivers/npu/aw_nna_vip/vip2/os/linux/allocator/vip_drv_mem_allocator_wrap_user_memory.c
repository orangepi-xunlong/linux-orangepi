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


/* To allow arm64 syscalls to accept tagged pointers from userspace,
   we must untag them when they ar passed to the kernel
*/
#ifndef untagged_addr
#define untagged_addr(addr) (addr)
#endif

static vip_status_e vipdrv_kernel_logical_wrap_memory(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_ERROR_NOT_SUPPORTED;
    PRINTK_E("not support map kernel logical wrap memory\n");
    return status;
}

static vip_status_e vipdrv_kernel_logical_unwrap_memory(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    ptr->kerl_logical = VIP_NULL;
    ptr->mem_flag &= ~VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;
    return status;
}

static vip_status_e vipdrv_user_logical_wrap_memory(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    return status;
}

static vip_status_e vipdrv_user_logical_unwrap_memory(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    return status;
}

#if vpmdENABLE_FLUSH_CPU_CACHE
static vip_status_e vipdrv_flush_cache_wrap_memory(
    vipdrv_video_mem_handle_t* handle,
    vip_uint8_t type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    status = flush_cache_pages(ptr->flush_handle, type);
    return status;
}
#endif

static vip_status_e vipdrv_mem_alloc_wrap_memory(
    vipdrv_video_mem_handle_t* handle,
    vipdrv_alloc_param_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_allocator_wrapmem_param_t *wrap_param = (vipdrv_allocator_wrapmem_param_t*)param;
    vipdrv_video_mem_handle_t* ptr = handle;
    vip_size_t size = wrap_param->size;
    vip_uint32_t page_count = 0;
    unsigned long long vaddr = 0;
    struct vm_area_struct *vma = NULL;
    unsigned long vm_flags = 0;
    vip_uint32_t offset = 0;
    unsigned long start = 0, end = 0;
    unsigned long long memory = (unsigned long long)(vipdrv_uintptr_t)wrap_param->user_logical;

    ptr->size = size;
    ptr->user_logical = wrap_param->user_logical;

    memory = untagged_addr(memory);
    if (0 == memory) {
        PRINTK_E("fail wrap memory is 0\n");
        return VIP_ERROR_FAILURE;
    }

    offset = memory - (memory & PAGE_MASK);
    /* Get the number of required pages. */
    end = (memory + size + PAGE_SIZE - 1) >> PAGE_SHIFT;
    start = memory >> PAGE_SHIFT;
    page_count = end - start;

    #if !vpmdENABLE_MMU
    if (page_count > 1) {
        PRINTK_E("fail MMU is disabled, memory pages num=%d, not support\n", page_count);
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }
    #endif

    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_physical_t) * page_count, (void**)&ptr->physical_table));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_size_t) * page_count, (void**)&ptr->size_table));
    vipdrv_os_zero_memory(ptr->physical_table, sizeof(vip_physical_t) * page_count);
    vipdrv_os_zero_memory(ptr->size_table, sizeof(vip_size_t) * page_count);

    down_read(&current_mm_mmap_sem);
    vma = find_vma(current->mm, memory);
    up_read(&current_mm_mmap_sem);
    if (!vma) {
        PRINTK_E("No such memory, or across vmas\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    vm_flags = vma->vm_flags;
    vaddr = vma->vm_end;
    /* check user space logical */
    down_read(&current_mm_mmap_sem);
    while (vaddr < memory + size) {
        vma = find_vma(current->mm, vaddr);
        if (!vma) {
            up_read(&current_mm_mmap_sem);
            PRINTK_E("No such memory, or across vmas\n");
            vipGoOnError(VIP_ERROR_IO);
        }
        if ((vma->vm_flags & VM_PFNMAP) != (vm_flags & VM_PFNMAP)) {
            up_read(&current_mm_mmap_sem);
            PRINTK_E("Can't support different map type: both PFN and PAGE detected\n");
            vipGoOnError(VIP_ERROR_IO);
        }
        vaddr = vma->vm_end;
    }
    up_read(&current_mm_mmap_sem);

    /* convert logical to physical */
    if (vm_flags & VM_PFNMAP) {
        vipOnError(vipdrv_import_pfn_map(ptr, memory, page_count, size, offset));
        ptr->mem_flag |= VIPDRV_MEM_FLAG_USER_MEMORY_PFN;
    }
    else {
        vipOnError(vipdrv_import_page_map(ptr, memory, page_count, size, offset, vm_flags));
        ptr->mem_flag |= VIPDRV_MEM_FLAG_USER_MEMORY_PAGE;
    }

    PRINTK_D("wrap usermemory handle=0x%"PRPx", logical=0x%"PRIx64", "
             "physical base=0x%"PRIx64", physical_num=%d, size=0x%x, offset=0x%x, flag=0x%x\n",
              ptr->alloc_handle, memory, ptr->physical_table[0],
              ptr->physical_num, size, offset, ptr->mem_flag);
    return status;

onError:
    if (VIP_NULL != ptr->physical_table) {
        vipdrv_os_free_memory(ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }
    if (VIP_NULL != ptr->size_table) {
        vipdrv_os_free_memory(ptr->size_table);
        ptr->size_table = VIP_NULL;
    }
    return status;
}

static vip_status_e vipdrv_mem_free_wrap_memory(
    vipdrv_video_mem_handle_t* handle
    )
{
    vipdrv_video_mem_handle_t* ptr = handle;
    struct page **pages = VIP_NULL;
    vip_uint32_t i = 0;
    vip_uint32_t page_count = 0;

    if (VIP_NULL == ptr->physical_table) {
        return VIP_SUCCESS;
    }

    vipdrv_get_page_count(ptr, &page_count);
    flush_cache_destroy_pages(ptr->flush_handle);
    ptr->flush_handle = VIP_NULL;

    pages = (struct page **)ptr->alloc_handle;
    if (VIP_NULL != pages) {
        for (i = 0; i < page_count; i++) {
            if ((pages[i]) && (ptr->mem_flag & VIPDRV_MEM_FLAG_USER_MEMORY_PAGE)) {
            #if LINUX_VERSION_CODE > KERNEL_VERSION(5, 6, 0)
                unpin_user_page(pages[i]);
            #else
                put_page(pages[i]);
            #endif
            }
        }
        vipdrv_os_free_memory((void*)pages);
        ptr->alloc_handle = VIP_NULL;
    }

    if (VIP_NULL != ptr->size_table) {
        vipdrv_os_free_memory((void*)ptr->size_table);
        ptr->size_table = VIP_NULL;
    }
    if (VIP_NULL != ptr->physical_table) {
        vipdrv_os_free_memory((void*)ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }

    return VIP_SUCCESS;
}

/*
@brief convert CPU physical to VIP physical.
@param cpu_physical. the physical address of CPU domain.
*/
static vip_physical_t vipdrv_phy_cpu2vip_wrap_memory(
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
static vip_physical_t vipdrv_phy_vip2cpu_wrap_memory(
    vip_physical_t vip_physical
    )
{
    vip_physical_t cpu_hysical = vip_physical;
    return cpu_hysical;
}

/*
wrap allocator:
    Wrap the memory specified by application so that NPU can access the memory.
    Wrap the memory specified by vip_create_buffer_from_handle() API.
    Only can be used when NPU's MMU is enabled.
*/
vip_status_e allocator_init_wrap_user_memory(
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
    allocator->callback.map_kernel_logical = vipdrv_kernel_logical_wrap_memory;
    allocator->callback.unmap_kernel_logical = vipdrv_kernel_logical_unwrap_memory;
    allocator->callback.map_user_logical = vipdrv_user_logical_wrap_memory;
    allocator->callback.unmap_user_logical = vipdrv_user_logical_unwrap_memory;
#if vpmdENABLE_FLUSH_CPU_CACHE
    allocator->callback.flush_cache = vipdrv_flush_cache_wrap_memory;
#endif
    allocator->callback.allocate = vipdrv_mem_alloc_wrap_memory;
    allocator->callback.free = vipdrv_mem_free_wrap_memory;
    allocator->callback.phy_vip2cpu = vipdrv_phy_vip2cpu_wrap_memory;
    allocator->callback.phy_cpu2vip = vipdrv_phy_cpu2vip_wrap_memory;

    return VIP_SUCCESS;
}

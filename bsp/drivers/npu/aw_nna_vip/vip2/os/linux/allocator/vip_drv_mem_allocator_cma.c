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
#if defined (USE_LINUX_CMA)
#include <vip_drv_video_memory.h>
#include <vip_drv_mem_allocator_common.h>
#include "vip_drv_device_driver.h"
#include <vip_drv_context.h>
#include <linux/vmstat.h>

static vip_status_e vipdrv_mem_free_cma(
    vipdrv_video_mem_handle_t* handle
    );

static vip_status_e vipdrv_check_cma_migrated(
    vip_physical_t cma_physical,
    vip_uint32_t cma_size
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_mem_control_block_t* mcb = VIP_NULL;
    vipdrv_video_mem_handle_t *pointer = VIP_NULL;
    vip_uint32_t i = 0;
    vip_uint32_t phy_iter = 0;

    for (i = 1; i < vipdrv_database_free_pos(&context->mem_database); i++) {
        vipdrv_database_use(&context->mem_database, i, (void**)&mcb);
        if (VIP_NULL != mcb) {
            if (VIP_NULL != mcb->handle) {
                pointer = mcb->handle;
                for (phy_iter = 0; phy_iter < pointer->physical_num; phy_iter++) {
                    if (0 == (VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP & pointer->allocator_type)) {
                        status = vipdrv_memory_overlap_check(cma_physical, cma_size,
                                                            pointer->physical_table[phy_iter],
                                                            pointer->size_table[phy_iter]);
                        if (status != VIP_SUCCESS) {
                            PRINTK_E("fail to check cma phy=0x%"PRIx64", size=0x%x "
                                "phy=0x%"PRIx64", size=0x%x alc-name=%s",
                                 cma_physical, cma_size, pointer->physical_table[phy_iter],
                                 pointer->size_table[phy_iter], pointer->allocator->name);
                            vipGoOnError(status);
                        }
                    }
                }
            }
            vipdrv_database_unuse(&context->mem_database, i);
        }
    }

onError:
    return status;
}

static vip_status_e vipdrv_kernel_logical_map_cma(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}

static vip_status_e vipdrv_kernel_logical_unmap_cma(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}

static vip_status_e vipdrv_user_logical_map_cma(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    vip_physical_t cpu_physical = VIPDRV_PTR_TO_UINT64(ptr->alloc_handle);
    vip_uint32_t page_offset = cpu_physical - (cpu_physical & PAGE_MASK);
    void* user_logical = VIP_NULL;
    vip_uint32_t page_count = 0;

    vipdrv_get_page_count(ptr, &page_count);
    vipOnError(vipdrv_map_user_dma_map(page_count, cpu_physical, ptr->kerl_logical, ptr->size, &user_logical));

    ptr->user_logical = (void*)((vip_uint8_t*)user_logical + page_offset);
    ptr->mem_flag |= VIPDRV_MEM_FLAG_MAP_USER_LOGICAL;
    ptr->pid = vipdrv_os_get_pid();
    return status;
onError:
    PRINTK_E("fail to map user logical cma\n");
    return status;
}

static vip_status_e vipdrv_user_logical_unmap_cma(
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
static vip_status_e vipdrv_flush_cache_cma(
    vipdrv_video_mem_handle_t* handle,
    vip_uint8_t type
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}
#endif

static vip_status_e vipdrv_mem_alloc_cma(
    vipdrv_video_mem_handle_t* handle,
    vipdrv_alloc_param_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_allocator_alloc_param_t* alloc_param = (vipdrv_allocator_alloc_param_t*)param;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_t* allocator = ptr->allocator;
    static dma_addr_t dma_handle;
    vip_physical_t cpu_physical = 0;
#if (LINUX_VERSION_CODE > KERNEL_VERSION (4,14,0))
    unsigned long cma_free_page = global_zone_page_state(NR_FREE_CMA_PAGES);
#else
    unsigned long cma_free_page = global_page_state(NR_FREE_CMA_PAGES);
#endif
    unsigned long cma_free_size = cma_free_page << PAGE_SHIFT;

    if (cma_free_size < alloc_param->size) {
        return VIP_ERROR_FAILURE;
    }

    ptr->size = alloc_param->size;
    ptr->align = alloc_param->align;
    ptr->mem_flag = alloc_param->mem_flag;

    /* allocate physical address and kernel logical address */
    ptr->kerl_logical = dma_alloc_coherent(kdriver->device, PAGE_ALIGN(ptr->size),
                                            &dma_handle, GFP_KERNEL);
    if (VIP_NULL == ptr->kerl_logical) {
        #if DEBUG_ALLOCATOR
        PRINTK("fail to alloc memory from cma\n");
        #endif
        vipGoOnError(VIP_ERROR_IO);
    }
    ptr->mem_flag |= VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_physical_t), (void**)&ptr->physical_table));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_size_t), (void**)&ptr->size_table));

    cpu_physical = (vip_physical_t)dma_handle;
    ptr->physical_table[0] = allocator->callback.phy_cpu2vip(cpu_physical);
    ptr->size_table[0] = ptr->size;
    ptr->physical_num = 1;
    ptr->alloc_handle = VIPDRV_UINT64_TO_PTR(cpu_physical);

    /* check the CMA physical address is migrated from video memory page */
    status = vipdrv_check_cma_migrated(ptr->physical_table[0], ptr->size_table[0]);
    if (status != VIP_SUCCESS) {
        /* free the cma memory and return error for bypass allocal video memory from CMA */
        vipdrv_mem_free_cma(ptr);
        return VIP_ERROR_OUT_OF_MEMORY;
    }

    ptr->mem_flag |= VIPDRV_MEM_FLAG_CONTIGUOUS;
    ptr->mem_flag |= VIPDRV_MEM_FLAG_NONE_CACHE;
    return status;
onError:
    if (VIP_NULL != ptr->size_table) {
        vipdrv_os_free_memory((void*)ptr->size_table);
        ptr->size_table = VIP_NULL;
    }
    if (VIP_NULL != ptr->physical_table) {
        vipdrv_os_free_memory((void*)ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }
    #if DEBUG_ALLOCATOR
    PRINTK("fail to alloc memory from allocator cma\n");
    #endif
    return status;
}

static vip_status_e vipdrv_mem_free_cma(
    vipdrv_video_mem_handle_t* handle
    )
{
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    vip_physical_t cpu_physical = 0;
    if (VIP_NULL == ptr->physical_table) {
        return VIP_SUCCESS;
    }

    cpu_physical = VIPDRV_PTR_TO_UINT64(ptr->alloc_handle);
    if (ptr->kerl_logical) {
        dma_free_coherent(kdriver->device, PAGE_ALIGN(ptr->size),
            ptr->kerl_logical, cpu_physical);
    }

    if (ptr->kerl_logical) {
        vipdrv_kernel_logical_unmap_cma(handle);
    }
    if (ptr->user_logical) {
        vipdrv_user_logical_unmap_cma(handle);
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
static vip_physical_t vipdrv_mem_phy_cpu2vip_cma(
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
static vip_physical_t vipdrv_mem_phy_vip2cpu_cma(
    vip_physical_t vip_physical
    )
{
    vip_physical_t cpu_hysical = vip_physical;
    return cpu_hysical;
}

/*
dynamic allocator:
    alloc video memory from CMA
*/
vip_status_e allocator_init_dyn_cma(
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
    allocator->callback.allocate = vipdrv_mem_alloc_cma;
    allocator->callback.free = vipdrv_mem_free_cma;
    allocator->callback.map_kernel_logical = vipdrv_kernel_logical_map_cma;
    allocator->callback.unmap_kernel_logical = vipdrv_kernel_logical_unmap_cma;
    allocator->callback.map_user_logical = vipdrv_user_logical_map_cma;
    allocator->callback.unmap_user_logical = vipdrv_user_logical_unmap_cma;
#if vpmdENABLE_FLUSH_CPU_CACHE
    allocator->callback.flush_cache = vipdrv_flush_cache_cma;
#endif
    allocator->callback.phy_vip2cpu = vipdrv_mem_phy_vip2cpu_cma;
    allocator->callback.phy_cpu2vip = vipdrv_mem_phy_cpu2vip_cma;

    return VIP_SUCCESS;
}
#endif

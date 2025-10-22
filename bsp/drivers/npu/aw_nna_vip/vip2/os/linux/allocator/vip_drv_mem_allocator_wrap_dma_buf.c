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


static vip_status_e vipdrv_kernel_logical_wrap_dma_buf(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_ERROR_NOT_SUPPORTED;
    PRINTK_E("not support map kernel logical wrap dma buffer\n");
    return status;
}

static vip_status_e vipdrv_kernel_logical_unwrap_dma_buf(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    ptr->kerl_logical = VIP_NULL;
    ptr->mem_flag &= ~VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;
    return status;
}

static vip_status_e vipdrv_user_logical_wrap_dma_buf(
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
    vip_uint32_t page_count = 0;

    vipdrv_get_page_count(ptr, &page_count);
    vipOnError(vipdrv_os_allocate_memory(sizeof(struct page*) * page_count, (void**)&pages));
    vipdrv_os_zero_memory(pages, sizeof(struct page*) * page_count);
    vipOnError(vipdrv_fill_pages(ptr, pages));
    /* May need to be modified if the memory properties of dma-buf */
    vipOnError(vipdrv_map_user(page_count, pages, &user_logical, vip_true_e));

    ptr->user_logical = (void*)((vip_uint8_t*)user_logical + page_offset);
    ptr->mem_flag |= VIPDRV_MEM_FLAG_MAP_USER_LOGICAL;
    ptr->pid = vipdrv_os_get_pid();
    vipdrv_os_free_memory(pages);
    return status;
onError:
    PRINTK_E("fail to map user logical wrap dma buffer\n");
    if (VIP_NULL != pages) {
        vipdrv_os_free_memory(pages);
        pages = VIP_NULL;
    }
    return status;
}

static vip_status_e vipdrv_user_logical_unwrap_dma_buf(
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
static vip_status_e vipdrv_flush_cache_wrap_dma_buf(
    vipdrv_video_mem_handle_t* handle,
    vip_uint8_t type
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}
#endif

static vip_status_e vipdrv_mem_alloc_wrap_dma_buf(
    vipdrv_video_mem_handle_t* handle,
    vipdrv_alloc_param_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    vipdrv_allocator_wrapfd_param_t *wrap_param = (vipdrv_allocator_wrapfd_param_t*)param;
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_t* allocator = ptr->allocator;
    vip_uint32_t page_count = 0;
    vipdrv_dma_buf_info_t *import = VIP_NULL;
    struct scatterlist *s = VIP_NULL;
    struct sg_table *sgt = VIP_NULL;
    vip_uint32_t count = 0;
    vip_uint32_t i = 0;
    vip_uint32_t dmabuf_size = 0;
    vip_bool_e physical_contiguous = vip_true_e;
    vip_physical_t *cpu_physical = VIP_NULL;
    vip_uint32_t fd = wrap_param->fd;
    ptr->size = wrap_param->size;

    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_dma_buf_info_t), (void**)&import));
    import->dma_buf = dma_buf_get(fd);
    if (IS_ERR_OR_NULL(import->dma_buf)) {
        PRINTK_E("failed wrap fd dma_buf_get, fd=%d\n", fd);
        vipGoOnError(VIP_ERROR_IO);
    }
    dma_buf_put(import->dma_buf);

    get_dma_buf(import->dma_buf);

    import->dma_attachment = dma_buf_attach(import->dma_buf, kdriver->device);
    if (!import->dma_attachment) {
        PRINTK_E("failed wrap fd dma_buf_attach\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    sgt = import->sgt = dma_buf_map_attachment(import->dma_attachment, DMA_BIDIRECTIONAL);
    if (IS_ERR_OR_NULL(import->sgt)) {
        PRINTK_E("failed wrap fd dma_buf_map_attachment\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    /* Get number of pages. */
    for_each_sg(sgt->sgl, s, sgt->orig_nents, i) {
        page_count += (sg_dma_len(s) + (vip_uint32_t)(sg_dma_address(s) & (~PAGE_MASK)) + PAGE_SIZE - 1) / PAGE_SIZE;
        PRINTK_D("dmabuf[%d] address=0x%"PRIx64", size=0x%x\n", count, sg_dma_address(s), sg_dma_len(s));
        count++;
        dmabuf_size += sg_dma_len(s);
    }
    count = 0;

    if (ptr->size > dmabuf_size) {
        PRINTK_E("fail to wrap user fd=%d, dmabug size %d is small than request %lld\n",
                  fd, dmabuf_size, ptr->size);
        vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
    ptr->size = dmabuf_size;

    PRINTK_D("import dma-buf page_count=%d\n", page_count);
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_physical_t) * page_count, (void**)&ptr->physical_table));
    vipdrv_os_zero_memory(ptr->physical_table, sizeof(vip_physical_t) * page_count);
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_physical_t) * page_count, (void**)&cpu_physical));
    vipdrv_os_zero_memory(cpu_physical, sizeof(vip_physical_t) * page_count);
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_size_t) * page_count, (void**)&ptr->size_table));
    vipdrv_os_zero_memory(ptr->size_table, sizeof(vip_size_t) * page_count);

    /* Fill page array. */
    i = 0;
    for_each_sg(sgt->sgl, s, sgt->orig_nents, i) {
        vip_physical_t address = sg_dma_address(s);
        vip_uint32_t size = sg_dma_len(s);

        while (size) {
            vip_uint32_t tmp = PAGE_SIZE - (address & (~PAGE_MASK));
            tmp = size < tmp ? size : tmp;

            cpu_physical[count] = address;
            ptr->size_table[count] = tmp;

            address += tmp;
            size -= tmp;
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
        ptr->size_table[0] = dmabuf_size;
        ptr->physical_num = 1;
    }
    else {
        ptr->physical_num = count;
    }

    for (i = 0; i < ptr->physical_num; i++) {
        ptr->physical_table[i] = allocator->callback.phy_cpu2vip(cpu_physical[i]);
    }

    ptr->alloc_handle = (void *)import;
    ptr->mem_flag |= VIPDRV_MEM_FLAG_NONE_CACHE;

    PRINTK_D("wrap user fd=%d, dma_buf=0x%"PRPx", physical_num=%d, physical base=0x%"PRIx64", "
             "size=0x%llx, flag=0x%x\n", fd, import->dma_buf,
             ptr->physical_num, ptr->physical_table[0], ptr->size, ptr->mem_flag);

    vipdrv_os_free_memory((void*)cpu_physical);
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
    if (VIP_NULL != import) {
        if (!IS_ERR_OR_NULL(import->sgt)) {
            dma_buf_unmap_attachment(import->dma_attachment, import->sgt, DMA_BIDIRECTIONAL);
        }
        if (import->dma_attachment) {
            dma_buf_detach(import->dma_buf, import->dma_attachment);
        }
        if (!(IS_ERR_OR_NULL(import->dma_buf))) {
            dma_buf_put(import->dma_buf);
        }
        vipdrv_os_free_memory(import);
    }
    if (VIP_NULL != cpu_physical) {
        vipdrv_os_free_memory(cpu_physical);
        cpu_physical = VIP_NULL;
    }
    PRINTK_E("fail to wrap user fd=%d, size=0x%llx\n", fd, ptr->size);
    return status;
}

static vip_status_e vipdrv_mem_free_wrap_dma_buf(
    vipdrv_video_mem_handle_t* handle
    )
{
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_dma_buf_info_t *import = (vipdrv_dma_buf_info_t *)ptr->alloc_handle;

    if (VIP_NULL == ptr->physical_table) {
        return VIP_SUCCESS;
    }

    if (ptr->user_logical) {
        vipdrv_user_logical_unwrap_dma_buf(handle);
    }

    if (VIP_NULL != ptr->physical_table) {
        vipdrv_os_free_memory(ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }
    if (VIP_NULL != ptr->size_table) {
        vipdrv_os_free_memory(ptr->size_table);
        ptr->size_table = VIP_NULL;
    }
    if (VIP_NULL != import) {
        if (!IS_ERR_OR_NULL(import->sgt)) {
            dma_buf_unmap_attachment(import->dma_attachment, import->sgt, DMA_BIDIRECTIONAL);
        }
        if (import->dma_attachment) {
            dma_buf_detach(import->dma_buf, import->dma_attachment);
        }
        if (!(IS_ERR_OR_NULL(import->dma_buf))) {
            dma_buf_put(import->dma_buf);
        }
        vipdrv_os_free_memory(import);
    }

    return VIP_SUCCESS;
}

/*
wrap allocator:
    Wrap the memory specified via fd so that NPU can access the memory.
    Wrap the memory specified by vip_create_buffer_from_fd() API.
    Only supports dma-buf.
*/
vip_status_e allocator_init_wrap_dma_buf(
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
    allocator->callback.map_kernel_logical = vipdrv_kernel_logical_wrap_dma_buf;
    allocator->callback.unmap_kernel_logical = vipdrv_kernel_logical_unwrap_dma_buf;
    allocator->callback.map_user_logical = vipdrv_user_logical_wrap_dma_buf;
    allocator->callback.unmap_user_logical = vipdrv_user_logical_unwrap_dma_buf;
#if vpmdENABLE_FLUSH_CPU_CACHE
    allocator->callback.flush_cache = vipdrv_flush_cache_wrap_dma_buf;
#endif
    allocator->callback.allocate = vipdrv_mem_alloc_wrap_dma_buf;
    allocator->callback.free = vipdrv_mem_free_wrap_dma_buf;
    allocator->callback.phy_vip2cpu = vipdrv_drv_get_cpuphysical;
    allocator->callback.phy_cpu2vip = vipdrv_drv_get_vipphysical;

    return VIP_SUCCESS;
}

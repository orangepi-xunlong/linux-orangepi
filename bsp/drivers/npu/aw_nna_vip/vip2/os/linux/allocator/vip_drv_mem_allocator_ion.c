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
#if defined (USE_LINUX_MEM_ION)
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/ion.h>
#include <linux/dma-mapping.h>  /*just include "PAGE_SIZE" macro*/
#include <linux/dma-buf.h>

#include <vip_drv_video_memory.h>
#include <vip_drv_mem_allocator_common.h>
#include "vip_drv_device_driver.h"

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_VIDEO_MEMORY


struct ion_client *ionClient = VIP_NULL;


typedef struct _ipdrv_allocator_ion {
    size_t size;
    vip_physical_t phy_addr;
    void *vir_addr;
    vip_physical_t dma_addr;
    struct dma_buf *dma_buf;
    struct ion_client *client;
    struct ion_handle *handle;
} vipdrv_allocator_ion_t;


static vip_status_e vipdrv_kernel_logical_map_ion(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_ion_t *ion_mem = (vipdrv_allocator_ion_t*)ptr->alloc_handle;
    vip_uint32_t page_offset = 0;

    ion_mem->vir_addr = ion_map_kernel(ion_mem->client, ion_mem->handle);
    if (IS_ERR_OR_NULL(ion_mem->vir_addr)) {
        PRINTK_E("fail to ion map kernel logical");
        vipGoOnError(VIP_ERROR_FAILURE);
    }
    ptr->mem_flag |= VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;
    ptr->kerl_logical = (void*)((vip_uint8_t*)ion_mem->vir_addr + page_offset);

    return status;

onError:
    ptr->kerl_logical = VIP_NULL;
    ptr->mem_flag &= ~VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL;
    return status;
}

static vip_status_e vipdrv_kernel_logical_unmap_ion(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_allocator_ion_t *ion_mem = (vipdrv_allocator_ion_t*)handle->alloc_handle;

    ion_unmap_kernel(ion_mem->client, ion_mem->handle);

    return status;
}

static vip_status_e vipdrv_user_logical_map_ion(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_ion_t *ion_mem = (vipdrv_allocator_ion_t*)handle->alloc_handle;
    vip_uint32_t page_offset = 0;
    void* user_logical = VIP_NULL;
    vip_uint32_t page_count = 0;

    vipdrv_get_page_count(ptr, &page_count);
    ion_mem->dma_buf  = ion_share_dma_buf(ion_mem->client, ion_mem->handle);
    vipOnError(vipdrv_map_user_dma_buf(page_count, ion_mem->dma_buf, 0, &user_logical));

    ptr->user_logical = (void*)((vip_uint8_t*)user_logical + page_offset);
    ptr->mem_flag |= VIPDRV_MEM_FLAG_MAP_USER_LOGICAL;
    ptr->pid = vipdrv_os_get_pid();
    return status;
onError:
    PRINTK_E("fail to map user logical cma\n");
    return status;
}

static vip_status_e vipdrv_user_logical_unmap_ion(
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
static vip_status_e vipdrv_flush_cache_ion(
    vipdrv_video_mem_handle_t* handle,
    vip_uint8_t type
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}
#endif

static vip_status_e vipdrv_mem_alloc_ion(
    vipdrv_video_mem_handle_t* handle,
    vipdrv_alloc_param_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    int ret = -1;
    vipdrv_allocator_alloc_param_t* alloc_param = (vipdrv_allocator_alloc_param_t*)param;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_allocator_t* allocator = ptr->allocator;
    vipdrv_allocator_ion_t *ion_mem = VIP_NULL;

    if (VIP_NULL == ionClient) {
        PRINTK_E("fail ion client is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    ptr->size = alloc_param->size;
    ptr->align = alloc_param->align;
    ptr->mem_flag = alloc_param->mem_flag;

    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_allocator_ion_t), (void**)&ion_mem));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_physical_t), (void**)&ptr->physical_table));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_size_t), (void**)&ptr->size_table));

    /* create ion client */
    ion_mem->client = ionClient;
    if (IS_ERR_OR_NULL(ion_mem->client)) {
        PRINTK_E("fail to create ion client name=vip_dyna");
        goto onError;
    }

    /* alloc ion memory from system heap */
    ion_mem->handle = ion_alloc(ion_mem->client, ptr->size, PAGE_SIZE,
                                ION_HEAP_SYSTEM_MASK, 0);
    if (IS_ERR_OR_NULL(ion_mem->handle)) {
        #if DEBUG_ALLOCATOR
        PRINTK("fail to ion alloc size=%d", ptr->size);
        #endif
        goto err_alloc;
    }

    ret = dma_map_sg_attrs(get_device(kdriver->device), ion_mem->handle->buffer->sg_table->sgl,
              ion_mem->handle->buffer->sg_table->nents, DMA_BIDIRECTIONAL,
                DMA_ATTR_SKIP_CPU_SYNC);
    if (ret != 1) {
        #if DEBUG_ALLOCATOR
        PRINTK("fail to ion map sg\n");
        #endif
        goto err_phys;
    }

    ion_mem->dma_addr = sg_dma_address(ion_mem->handle->buffer->sg_table->sgl);
    ion_mem->phy_addr = ion_mem->dma_addr;

    ptr->physical_num = 1;
    ptr->physical_table[0] = allocator->callback.phy_cpu2vip(ion_mem->phy_addr);
    ptr->size_table[0] = ptr->size;
    ptr->alloc_handle = (void*)ion_mem;

    PRINTK_D("ion allocator kernel vir 0x%x, phy 0x%x\n", ion_mem->vir_addr, ion_mem->phy_addr);

    ptr->mem_flag |= VIPDRV_MEM_FLAG_CONTIGUOUS;
    ptr->mem_flag |= VIPDRV_MEM_FLAG_NONE_CACHE;
    return status;

err_phys:
    dma_unmap_sg_attrs(get_device(kdriver->device), ion_mem->handle->buffer->sg_table->sgl,
                    ion_mem->handle->buffer->sg_table->nents, DMA_FROM_DEVICE, DMA_ATTR_SKIP_CPU_SYNC);
    ion_free(ion_mem->client, ion_mem->handle);
err_alloc:
    ion_client_destroy(ion_mem->client);
onError:
    if (VIP_NULL != ptr->size_table) {
        vipdrv_os_free_memory((void*)ptr->size_table);
        ptr->size_table = VIP_NULL;
    }
    if (VIP_NULL != ptr->physical_table) {
        vipdrv_os_free_memory((void*)ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }
    if (VIP_NULL != ion_mem) {
        vipdrv_os_free_memory(ion_mem);
    }
    #if DEBUG_ALLOCATOR
    PRINTK("fail to alloc memory from ion allocator\n");
    #endif
    return status;
}

static vip_status_e vipdrv_mem_free_ion(
    vipdrv_video_mem_handle_t* handle
    )
{
    vipdrv_video_mem_handle_t* ptr = handle;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    vipdrv_allocator_ion_t *ion_mem = (vipdrv_allocator_ion_t*)ptr->alloc_handle;

    if (VIP_NULL == ptr->physical_table) {
        return VIP_SUCCESS;
    }

    if (IS_ERR_OR_NULL(ion_mem->client) || IS_ERR_OR_NULL(ion_mem->handle)
        || IS_ERR_OR_NULL(ion_mem->vir_addr)) {
        return VIP_SUCCESS;
    }

    dma_unmap_sg_attrs(get_device(kdriver->device), ion_mem->handle->buffer->sg_table->sgl,
                        ion_mem->handle->buffer->sg_table->nents,
                        DMA_FROM_DEVICE, DMA_ATTR_SKIP_CPU_SYNC);

    if (ptr->kerl_logical) {
        vipdrv_kernel_logical_unmap_ion(handle);
    }
    if (ptr->user_logical) {
        vipdrv_user_logical_unmap_ion(handle);
    }

    ion_free(ion_mem->client, ion_mem->handle);

    if (VIP_NULL != ptr->size_table) {
        vipdrv_os_free_memory((void*)ptr->size_table);
        ptr->size_table = VIP_NULL;
    }
    if (VIP_NULL != ptr->physical_table) {
        vipdrv_os_free_memory((void*)ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }
    vipdrv_os_free_memory((void*)ion_mem);

    return VIP_SUCCESS;
}

/*
@brief convert CPU physical to VIP physical.
@param cpu_physical. the physical address of CPU domain.
*/
static vip_physical_t vipdrv_mem_phy_cpu2vip_ion(
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
static vip_physical_t vipdrv_mem_phy_vip2cpu_ion(
    vip_physical_t vip_physical
    )
{
    vip_physical_t cpu_hysical = vip_physical;
    return cpu_hysical;
}

static vip_status_e vipdrv_mem_uninit_ion(
    vipdrv_allocator_t* allocator
    )
{
    if (ionClient != VIP_NULL) {
        ion_client_destroy(ionClient);
        ionClient = VIP_NULL;
    }

    return VIP_SUCCESS;
}

static struct ion_device * ion_device_get(void)
{
    /* < Customer to provide implementation >
     * Return a pointer to the global ion_device on the system
     */
    return NULL;
}

/*
dynamic allocator:
    alloc video memory from ION. SAMPLE CODE NOT TESTING.
*/
vip_status_e allocator_init_dyn_ion(
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
    allocator->callback.allocate = vipdrv_mem_alloc_ion;
    allocator->callback.free = vipdrv_mem_free_ion;
    allocator->callback.map_kernel_logical = vipdrv_kernel_logical_map_ion;
    allocator->callback.unmap_kernel_logical = vipdrv_kernel_logical_unmap_ion;
    allocator->callback.map_user_logical = vipdrv_user_logical_map_ion;
    allocator->callback.unmap_user_logical = vipdrv_user_logical_unmap_ion;
#if vpmdENABLE_FLUSH_CPU_CACHE
    allocator->callback.flush_cache = vipdrv_flush_cache_ion;
#endif
    allocator->callback.phy_vip2cpu = vipdrv_mem_phy_vip2cpu_ion;
    allocator->callback.phy_cpu2vip = vipdrv_mem_phy_cpu2vip_ion;
    allocator->callback.uninit = vipdrv_mem_uninit_ion;

    /* create ion client */
    ionClient = ion_client_create(ion_device_get(), "vip-dyna-ion");
    if (IS_ERR(ionClient)) {
        PRINTK_E("fail to create ion client\n");
        return VIP_ERROR_FAILURE;
    }

    return VIP_SUCCESS;
}
#endif


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
#include <vip_drv_context.h>
#include <vip_drv_mem_heap.h>
#include <vip_drv_mem_allocator_array.h>
#include <vip_drv_device_driver.h>
#if vpmdENABLE_MMU
#include <vip_drv_mmu.h>
#endif

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_VIDEO_MEMORY

vip_status_e vipdrv_memory_overlap_check(
    vip_address_t physical_a,
    vip_size_t size_a,
    vip_address_t physical_b,
    vip_size_t size_b
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (physical_a > physical_b) {
        if ((physical_b + size_b) >= physical_a) {
            status = VIP_ERROR_FAILURE;
        }
    }
    else if (physical_a < physical_b){
        if ((physical_a + size_a) >= physical_b) {
            status = VIP_ERROR_FAILURE;
        }
    }
    else {
        status = VIP_ERROR_FAILURE;
    }

    return status;
}

/*
 Check the ''name' matched with allocator desc name to find a allocator.
*/
#if vpmdENABLE_VIDEO_MEMORY_HEAP
static vipdrv_allocator_desc_t* vipdrv_get_alloc_desc(
    vip_char_t* name
    )
{
    vip_uint32_t index = 0;
    vipdrv_allocator_desc_t* allocator_desc = VIP_NULL;
    vip_uint32_t allocator_cnt = sizeof(video_mem_allocator_array) / sizeof(video_mem_allocator_array[0]);
    if (0 == allocator_cnt) {
        PRINTK_E("fail to get allocator array, no video memory allocator\n");
        return allocator_desc;
    }
    for (index = 0; index < allocator_cnt; index++) {
        vipdrv_allocator_desc_t* temp = &video_mem_allocator_array[index];
        if (0 == vipdrv_os_strcmp(name, temp->name)) {
            if (temp->init != VIP_NULL) {
                allocator_desc = temp;
                PRINTK_D("get video memory allocator desc name=%s\n", name);
                break;
            }
            else {
                PRINTK_E("fail to get allocator desc name=%s, init func is NULL\n", name);
                break;
            }
        }
    }
    /*if (VIP_NULL == allocator_desc) {
        PRINTK_E("fail to find allocator desc %s, allocator_cnt=%d\n", name, allocator_cnt);
    }*/
    return allocator_desc;
}
#endif

#define CHECK_CALLBACK_FUNC(func)                                            \
if (VIP_NULL == allocator->callback.func) {                                  \
    PRINTK_E("allocator %s: %s callback is null\n", allocator->name, #func); \
    vipGoOnError(VIP_ERROR_FAILURE);                                         \
}

static vip_bool_e cmp_allocator(
    vipdrv_allocator_t *p,
    vipdrv_allocator_t *q
    )
{
    /* put the scarce allocator at the end of list */
    /* 1. allocator type: heap -> dync -> other. */
    if (p->alctor_type < q->alctor_type) {
        return vip_true_e;
    }
    else if (p->alctor_type > q->alctor_type) {
        return vip_false_e;
    }

    /* 2. device visibility: device 0 -> device 0 & 1 */
    if (p->device_vis != q->device_vis) {
        if ((p->device_vis & q->device_vis) == p->device_vis) {
            return vip_true_e;
        }
        else if ((p->device_vis & q->device_vis) == q->device_vis) {
            return vip_false_e;
        }
    }

    /* 3. physical address: over 4G -> 4G */
    if ((p->mem_flag & VIPDRV_MEM_FLAG_4GB_ADDR_PA) < (q->mem_flag & VIPDRV_MEM_FLAG_4GB_ADDR_PA)) {
        return vip_true_e;
    }
    else if ((p->mem_flag & VIPDRV_MEM_FLAG_4GB_ADDR_PA) > (q->mem_flag & VIPDRV_MEM_FLAG_4GB_ADDR_PA)) {
        return vip_false_e;
    }

    /* 4. address continue: 4K continue -> 64K continue -> 1M continue -> 16M continue -> continue */
    if ((p->mem_flag & VIPDRV_MEM_FLAG_CONTIGUOUS) < (q->mem_flag & VIPDRV_MEM_FLAG_CONTIGUOUS)) {
        return vip_true_e;
    }
    else if ((p->mem_flag & VIPDRV_MEM_FLAG_CONTIGUOUS) > (q->mem_flag & VIPDRV_MEM_FLAG_CONTIGUOUS)) {
        return vip_false_e;
    }

    return vip_true_e;
}

static vip_status_e vipdrv_allocator_list_add(
    vipdrv_allocator_t *allocator
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_alloc_mgt_t* cur = (vipdrv_alloc_mgt_t*)context->alloc_list_head;
    vipdrv_alloc_mgt_t* last = VIP_NULL;

    CHECK_CALLBACK_FUNC(allocate);
    CHECK_CALLBACK_FUNC(free);
    CHECK_CALLBACK_FUNC(map_user_logical);
    CHECK_CALLBACK_FUNC(unmap_user_logical);
    CHECK_CALLBACK_FUNC(map_kernel_logical);
    CHECK_CALLBACK_FUNC(unmap_kernel_logical);
#if vpmdENABLE_FLUSH_CPU_CACHE
    CHECK_CALLBACK_FUNC(flush_cache);
#endif
    CHECK_CALLBACK_FUNC(phy_vip2cpu);
    CHECK_CALLBACK_FUNC(phy_cpu2vip);

    while (VIP_NULL != cur) {
        if (vip_true_e == cmp_allocator(allocator, (vipdrv_allocator_t*)cur)) {
            break;
        }

        last = cur;
        cur = (vipdrv_alloc_mgt_t*)cur->next;
    }
    if (VIP_NULL != last) {
        last->next = allocator;
    }
    else {
        context->alloc_list_head = allocator;
    }
    ((vipdrv_alloc_mgt_t*)allocator)->next = (vipdrv_allocator_t*)cur;
    return status;
onError:
    PRINTK_E("fail to add allocator\n");
    return status;
}

static vip_status_e vipdrv_alloc_mgt_dyn_alloc(
    vipdrv_video_mem_handle_t* handle,
    vipdrv_alloc_param_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_alloc_mgt_dyn_t* alloc_mgt_dyn = VIP_NULL;

    if (handle == VIP_NULL) {
        PRINTK_E("fail to allocate for dyn memory\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    alloc_mgt_dyn = (vipdrv_alloc_mgt_dyn_t*)handle->allocator;
    handle->allocator_type = alloc_mgt_dyn->allocator_mgt.allocator.alctor_type;

    status = alloc_mgt_dyn->allocator_mgt.base_callback.allocate(handle, param);

    return status;
}

static vip_status_e vipdrv_alloc_mgt_dyn_init(
    vipdrv_alloc_mgt_dyn_t* alloc_mgt_dyn
    )
{
    vip_status_e status = VIP_SUCCESS;

    vipdrv_os_copy_memory(&alloc_mgt_dyn->allocator_mgt.base_callback,
        &alloc_mgt_dyn->allocator_mgt.allocator.callback, sizeof(vipdrv_allocator_callback_t));

    alloc_mgt_dyn->allocator_mgt.allocator.callback.allocate = vipdrv_alloc_mgt_dyn_alloc;

    return status;
}

/*
1. alloc and create a dynamic allocator management.
2. init and construct mangement.
3. add allocator into allocator_list.
*/
static vip_status_e vipdrv_allocator_mgt_register_dyn(
    vipdrv_allocator_desc_t* allocator_desc
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_allocator_t* allocator = VIP_NULL;
    vipdrv_alloc_mgt_dyn_t* alloc_mgt_dyn = VIP_NULL;

    if (VIP_NULL == allocator_desc) {
        PRINTK_E("fail allocator desc is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_alloc_mgt_dyn_t), (void**)&alloc_mgt_dyn));
    vipdrv_os_zero_memory(alloc_mgt_dyn, sizeof(vipdrv_alloc_mgt_dyn_t));
    allocator = &alloc_mgt_dyn->allocator_mgt.allocator;

    /* construct an allocator */
    if (allocator_desc->init != VIP_NULL) {
        vipOnError(allocator_desc->init(allocator, allocator_desc->name, allocator_desc->allocator_type));
    }
    /* construct dynamic allocator management */
    vipOnError(vipdrv_alloc_mgt_dyn_init(alloc_mgt_dyn));
    vipOnError(vipdrv_allocator_list_add(allocator));
    PRINTK_D("register dyn allocator, name=%s, type=0x%x, mem_flag=0x%x",
             allocator->name, allocator->alctor_type, allocator->mem_flag);

    return status;

onError:
    if (VIP_NULL != alloc_mgt_dyn) {
        vipdrv_os_free_memory(alloc_mgt_dyn);
        alloc_mgt_dyn = VIP_NULL;
    }
    PRINTK_E("fail to register dyn allocator name=%s\n", allocator_desc->name);
    return status;
}

#if vpmdENABLE_VIDEO_MEMORY_HEAP
static vip_status_e vipdrv_allocator_mgt_register_heap(
    vipdrv_videomem_heap_info_t *heap_info,
    vip_bool_e mmu_page_heap
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_allocator_t *allocator = VIP_NULL;
    vipdrv_alloc_mgt_heap_t *heap = VIP_NULL;
    vipdrv_allocator_desc_t *alloc_desc = vipdrv_get_alloc_desc(heap_info->name);
    if (0 == heap_info->vip_memsize) {
        return VIP_SUCCESS;
    }
    if (VIP_NULL == alloc_desc) {
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    if (VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP != alloc_desc->allocator_type) {
        PRINTK_E("fail type=%d, ask fill the VIDO_HEAP in _x_array.h\n", alloc_desc->allocator_type);
        vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_alloc_mgt_heap_t), (void**)&heap));
    vipdrv_os_zero_memory(heap, sizeof(vipdrv_alloc_mgt_heap_t));
    allocator = &heap->allocator_mgt.allocator;

    /* construct an allocator */
    vipOnError(alloc_desc->init(allocator, alloc_desc->name, alloc_desc->allocator_type));
    /* construct heap allocator management */
    vipOnError(vipdrv_alloc_mgt_heap_init(heap, heap_info));
    if (vip_true_e == mmu_page_heap) {
        heap->allocator_mgt.allocator.mem_flag |= VIPDRV_MEM_FLAG_MMU_PAGE_MEM;
        heap->allocator_mgt.allocator.name = "heap-mmu-page";
    }
    vipOnError(vipdrv_allocator_list_add(allocator));
    PRINTK_D("register heap name=%s allocator size=0x%"PRIx64" phy=0x%"PRIx64", type=0x%x, memflag=0x%x.\n",
           heap_info->name, heap_info->vip_memsize, heap_info->vip_physical,
           allocator->alctor_type, allocator->mem_flag);
    return status;
onError:
    if (VIP_NULL != heap) {
        vipdrv_os_free_memory(heap);
        heap = VIP_NULL;
    }
    PRINTK_E("fail to register heap, name=%s", heap_info->name);
    return status;
}
#endif

static vip_status_e vipdrv_allocator_mgt_unregister(
    vipdrv_alloc_mgt_t* alloc_mgt
    )
{
    if (alloc_mgt->allocator.callback.uninit) {
        alloc_mgt->allocator.callback.uninit(&alloc_mgt->allocator);
    }
    if (alloc_mgt->base_callback.uninit) {
        alloc_mgt->base_callback.uninit(&alloc_mgt->allocator);
    }
    vipdrv_os_free_memory(alloc_mgt);

    return VIP_SUCCESS;
}

vip_status_e vipdrv_allocator_init(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    vipdrv_platform_memory_config_t mem_config = { 0 };
    vipdrv_videomem_heap_info_t heap_info = { 0 };
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
#if vpmdENABLE_MMU
    vip_bool_e heap_split = vip_false_e;
#endif

    vipOnError(vipdrv_drv_get_memory_config(&mem_config));
    if (0 == mem_config.heap_count) {
        PRINTK_E("fail heap count is 0\n");
    }

    for (i = 0; i < mem_config.heap_count; i++) {
        vipdrv_os_copy_memory(&heap_info, &mem_config.heap_infos[i], sizeof(vipdrv_videomem_heap_info_t));
        PRINTK_D("video memory heap %d base vip physical=0x%"PRIx64", size=0x%"PRIx64", name=%s\n",
                 i, heap_info.vip_physical, heap_info.vip_memsize, heap_info.name);
        /* memory space overlap check */
        status = vipdrv_memory_overlap_check(context->vip_sram_address, context->vip_sram_size[0],
                                    heap_info.vip_physical, heap_info.vip_memsize);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to check heap[0x%x~0x%x] overlap with vip-sram[0x%x~0x%x]\n",
                 heap_info.vip_physical, heap_info.vip_memsize + heap_info.vip_physical,
                 context->vip_sram_address, context->vip_sram_address + context->vip_sram_size[0]);
            vipGoOnError(status);
        }
        else {
            #if vpmdENABLE_MMU
            vipdrv_videomem_heap_info_t split_info = { 0 };
            vip_uint64_t device_vis = (~(vip_uint64_t)1) >> (64 - context->device_count);
            if (vip_false_e == heap_split && (device_vis & heap_info.device_vis) == device_vis &&
                VIP_SUCCESS == vipdrv_mmu_split_heap(&heap_info, &split_info)) {
                heap_split = vip_true_e;
                PRINTK_D("split heap for mmu page, base vip physical=0x%"PRIx64", size=0x%"PRIx64", name=%s",
                    split_info.vip_physical, split_info.vip_memsize, split_info.name);
                vipOnError(vipdrv_allocator_mgt_register_heap(&split_info, vip_true_e));
            }
            #endif
            vipOnError(vipdrv_allocator_mgt_register_heap(&heap_info, vip_false_e));
        }
    }

#if vpmdENABLE_MMU
    if (vip_false_e == heap_split) {
        PRINTK_E("there is no heap for mmu page, heap_cnt=%d\n", mem_config.heap_count);
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }
#endif
#endif

    /* register dynamic allocator */
    for (i = 0; i < sizeof(video_mem_allocator_array) / sizeof(video_mem_allocator_array[0]); i++) {
        vipdrv_allocator_desc_t* allocator_desc = &video_mem_allocator_array[i];
        if (allocator_desc->allocator_type > VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP) {
            if (allocator_desc->init != VIP_NULL) {
                vipOnError(vipdrv_allocator_mgt_register_dyn(allocator_desc));
            }
            else {
                PRINTK_E("fail to register allocator desc name=%s, init func is NULL\n",
                          allocator_desc->name);
            }
        }
    }

#if vpmdENABLE_DEBUG_LOG > 3
    vipdrv_allocator_dump();
#endif

onError:
    return status;
}

vip_status_e vipdrv_allocator_uninit(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_alloc_mgt_t* cur = (vipdrv_alloc_mgt_t*)context->alloc_list_head;
    vipdrv_alloc_mgt_t* next = VIP_NULL;

    while (VIP_NULL != cur) {
        next = (vipdrv_alloc_mgt_t*)cur->next;
        vipdrv_allocator_mgt_unregister(cur);
        cur = next;
    }
    context->alloc_list_head = VIP_NULL;

    return status;
}

vip_status_e vipdrv_allocator_get(
    vipdrv_allocator_type_e alctor_type,
    vipdrv_mem_flag_e mem_flag,
    vip_uint64_t device_vis,
    vipdrv_allocator_t** allocator
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_alloc_mgt_t* alloc_node = VIP_NULL;

    /* find last allocator */
    if (VIP_NULL != *allocator) {
        alloc_node = (vipdrv_alloc_mgt_t*)((vipdrv_alloc_mgt_t*)(*allocator))->next;
    }
    else {
        alloc_node = (vipdrv_alloc_mgt_t*)context->alloc_list_head;
    }

    /* find allocator from last allocator */
    while (alloc_node) {
        if ((alctor_type & alloc_node->allocator.alctor_type) &&
            (device_vis & alloc_node->allocator.device_vis) == device_vis &&
            (alloc_node->allocator.mem_flag & mem_flag) == mem_flag &&
            (alloc_node->allocator.mem_flag & VIPDRV_MEM_FLAG_MMU_PAGE_MEM) ==
            (mem_flag & VIPDRV_MEM_FLAG_MMU_PAGE_MEM)) {
            *allocator = &alloc_node->allocator;
            break;
        }
        alloc_node = (vipdrv_alloc_mgt_t*)alloc_node->next;
    }

    if (VIP_NULL == alloc_node) {
        status = VIP_ERROR_FAILURE;
    }
#if DEBUG_ALLOCATOR
    else {
        PRINTK_D("get video mem allocator name=%s\n", (*allocator)->name);
    }
#endif
    return status;
}

/* dump allocator arrary and allocator list */
vip_status_e vipdrv_allocator_dump(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_alloc_mgt_t* alloc_node = VIP_NULL;
    vipdrv_allocator_t* allocator = VIP_NULL;
    vip_uint32_t index = 0;
    vip_uint32_t allocator_cnt = sizeof(video_mem_allocator_array) / sizeof(video_mem_allocator_array[0]);

    PRINTK("allocator desc arrary:\n");
    for (index = 0; index < allocator_cnt; index++) {
        vipdrv_allocator_desc_t* temp = &video_mem_allocator_array[index];
        PRINTK("index: %d, name: %s, allocator_type=0x%x\n", index, temp->name, temp->allocator_type);
    }

    PRINTK("allocator list:\n");
    index = 0;
    alloc_node = (vipdrv_alloc_mgt_t*)context->alloc_list_head;
    while (alloc_node) {
        allocator = &alloc_node->allocator;
        #if vpmdENABLE_VIDEO_MEMORY_HEAP
        if (allocator->alctor_type & VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP) {
            vipdrv_alloc_mgt_heap_t* heap = (vipdrv_alloc_mgt_heap_t*)allocator;
            PRINTK("index: %d, name: %s, mem_flag=0x%x, alctor_type=0x%x, total size=0x%"PRIx64", "
                   "free size=0x%"PRIx32"\n", index, allocator->name, allocator->mem_flag,
                    allocator->alctor_type, heap->total_size, heap->free_bytes);
            vipdrv_heap_dump(heap);
        }
        else
        #endif
        {
            PRINTK("index: %d, name: %s, mem_flag=0x%x, alctor_type=0x%x\n",
                    index, allocator->name, allocator->mem_flag, allocator->alctor_type);
        }
        alloc_node = (vipdrv_alloc_mgt_t*)alloc_node->next;
        index++;
    }

    return status;
}


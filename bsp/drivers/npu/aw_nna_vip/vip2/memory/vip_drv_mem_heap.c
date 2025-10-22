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

#include <vip_drv_mem_heap.h>
#if vpmdENABLE_VIDEO_MEMORY_HEAP
#include <vip_drv_context.h>
#include <vip_drv_device_driver.h>

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_VIDEO_MEMORY


#define STATUS_USED     0xABBAF00D
#define STATUS_FREE     0x5A5A5A5A
#define STATUS_INIT     0xCDCDCDCD

static vip_status_e new_node(
    vipdrv_alloc_mgt_heap_t *heap,
    vipdrv_heap_node_t **node
    )
{
    vip_status_e status = VIP_SUCCESS;

    do {
        if (VIP_NULL == heap->idle_list.next) {
            PRINTK_E("vide mem node capacity=%d, which is full\n", heap->node_capacity);
            status = VIP_ERROR_OUT_OF_RESOURCE;
            break;
        }

        *node = (vipdrv_heap_node_t*)heap->idle_list.next;
        heap->idle_list.next = heap->idle_list.next->next;
        heap->node_new_cnt++;
    } while (0);

    return status;
}

static vip_status_e del_node(
    vipdrv_alloc_mgt_heap_t *heap,
    vipdrv_heap_node_t *node
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (VIP_NULL == node) {
        PRINTK_E("fail to delete node, node is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    node->status = STATUS_INIT;
    node->list.next = heap->idle_list.next;
    heap->idle_list.next = &node->list;
    heap->node_del_cnt++;

    return status;
}

/* Add the list item in front of "head". */
static void add_list(
    vipdrv_list_head_t *new,
    vipdrv_list_head_t *prev,
    vipdrv_list_head_t *next
    )
{
    /* Link the new item. */
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static void add_list_tail(
    vipdrv_list_head_t *new,
    vipdrv_list_head_t *head
    )
{
    add_list(new, head->prev, head);
}

/* Remove an entry out of the list. */
static void delete_list(
    vipdrv_list_head_t *entry
    )
{
    if (entry->prev != VIP_NULL) {
        entry->prev->next = entry->next;
    }
    if (entry->next != VIP_NULL) {
        entry->next->prev = entry->prev;
    }

    entry->prev = VIP_NULL;
    entry->next = VIP_NULL;
}

static vip_status_e split_node(
    vipdrv_alloc_mgt_heap_t *heap,
    vipdrv_heap_node_t *node,
    vipdrv_heap_node_t **split,
    vip_uint64_t size
    )
{
    vip_status_e status = VIP_SUCCESS;

    do {
        /* Allocate a new node. */
        status = new_node(heap, split);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to split node status=%d\n", status);
            break;
        }

        /* Fill in the data of this node of the remaning size. */
        (*split)->offset = node->offset + size;
        (*split)->size = node->size - size;
        (*split)->status = STATUS_FREE;

        /* Add the new node before the current node. */
        add_list_tail(&(*split)->list, &node->list);

        /* Adjust the size of the current node. */
        node->size = size;
    } while (0);

    return status;
}

static vip_bool_e vipdrv_heap_query(
    vipdrv_alloc_mgt_heap_t *heap,
    vip_uint64_t size
    )
{
    if (0 == heap->total_size) {
        return vip_false_e;
    }

    if (size > heap->free_bytes) {
        return vip_false_e;
    }

    if (VIP_NULL == heap->idle_list.next) {
        /* The node cap needs to be extended */
        #if !vpmdNODE_MEMORY_IN_HEAP
        void *tmp_buffer = VIP_NULL;
        vip_uint32_t i = 0;
        vip_status_e status = VIP_SUCCESS;
        vip_uint32_t old_size = heap->node_capacity * sizeof(vipdrv_heap_node_t);
        vipdrv_heap_node_t *new_nodes = VIP_NULL;
        vip_uint32_t buf_size = 0, new_node_cap = 0;
        vip_int64_t offset = 0;
        new_node_cap = heap->node_capacity + 256;

        PRINTK_D("video memory heap %s node capacity extend from %d to %d\n",
                heap->allocator_mgt.allocator.name, heap->node_capacity, new_node_cap);
        buf_size = sizeof(vipdrv_heap_node_t) * new_node_cap;
        status = vipdrv_os_allocate_memory(buf_size, (void**)&tmp_buffer);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to alloc memory for extend heap node cap\n");
            return vip_false_e;
        }
        vipdrv_os_zero_memory(tmp_buffer, buf_size);
        vipdrv_os_copy_memory(tmp_buffer, heap->nodes, old_size);
        new_nodes = (vipdrv_heap_node_t*)tmp_buffer;
        offset = (vip_uint8_t*)tmp_buffer - (vip_uint8_t*)heap->nodes;
        for (i = 0; i < heap->node_capacity; i++) {
            if (&heap->list != new_nodes[i].list.next) {
                new_nodes[i].list.next = (vipdrv_list_head_t*)((vip_uint8_t*)new_nodes[i].list.next + offset);
            }
            if (&heap->list != new_nodes[i].list.prev) {
                new_nodes[i].list.prev = (vipdrv_list_head_t*)((vip_uint8_t*)new_nodes[i].list.prev + offset);
            }
        }
        for (i = heap->node_capacity; i < new_node_cap; i++) {
            new_nodes[i].status = STATUS_INIT;
            new_nodes[i].list.next = heap->idle_list.next;
            heap->idle_list.next = &new_nodes[i].list;
        }
        heap->list.next = (vipdrv_list_head_t*)((vip_uint8_t*)heap->list.next + offset);
        heap->list.prev = (vipdrv_list_head_t*)((vip_uint8_t*)heap->list.prev + offset);
        vipdrv_os_free_memory(heap->nodes);
        heap->nodes = tmp_buffer;
        heap->node_capacity = new_node_cap;
        #else
        PRINTK_W("warning heap=%s free_size=%d, node capacity=%d not enogh\n",
            heap->allocator_mgt.allocator.name, heap->free_bytes, heap->node_capacity);
        vipdrv_heap_dump(heap);
        return vip_false_e;
        #endif
    }

    return vip_true_e;
}

/*
allocate the VIP's virtual address from heap management when MMU is enabled.
*/
static vip_status_e vipdrv_alloc_mgt_heap_alloc(
    vipdrv_video_mem_handle_t* handle,
    vipdrv_alloc_param_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_heap_node_t *pos = VIP_NULL;
    vip_uint64_t offset = 0;
    vip_bool_e support = vip_false_e;
    vipdrv_alloc_mgt_heap_t* heap = VIP_NULL;
    vipdrv_alloc_mgt_t* alloc_mgt = VIP_NULL;
    vipdrv_allocator_alloc_param_t *alloc_param = (vipdrv_allocator_alloc_param_t*)param;
    vip_uint64_t size = 0;
    vip_uint32_t align = 0;

    if (VIP_NULL == handle || VIP_NULL == param) {
        PRINTK_E("heap alloc, memory parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }
    heap = (vipdrv_alloc_mgt_heap_t*)handle->allocator;
    alloc_mgt = &heap->allocator_mgt;
    align = alloc_param->align;
    size = alloc_param->size;
    handle->mem_flag = alloc_param->mem_flag;

    if (0 == heap->total_size) {
        return VIP_ERROR_FAILURE;
    }
#if vpmdENABLE_MULTIPLE_TASK || vpmdENABLE_VIDEO_MEMORY_CACHE
#if defined(__linux__)
    /* return 4Kbytes alignment on Linux, not request by VIP */
    if (align < 4096) {
        align = 4096;
    }
    size = VIP_ALIGN(size, 4096);
#endif
#endif

#if vpmdENABLE_MULTIPLE_TASK
    status = vipdrv_os_lock_mutex(heap->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock video memory heap mutex\n");
        return VIP_ERROR_IO;
    }
#endif
    if (alloc_param->specified) {
        if (alloc_param->virtual) {
            if (vip_false_e == heap->mmu_map) {
                PRINTK_E("heap %s has not map mmu\n", heap->allocator_mgt.allocator.name);
                vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
            }
            if ((vip_address_t)heap->virtual > alloc_param->address) {
                PRINTK_E("heap %s: virtual start from 0x%"PRPx", need from 0x%"PRPx"\n",
                    heap->allocator_mgt.allocator.name, heap->virtual, alloc_param->address);
                vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
            }
            if ((vip_address_t)heap->virtual + heap->total_size < alloc_param->address + alloc_param->size) {
                PRINTK_E("heap %s: virtual end at 0x%"PRPx", need at 0x%"PRPx"\n",
                    heap->allocator_mgt.allocator.name, (vip_address_t)heap->virtual + heap->total_size,
                    alloc_param->address + alloc_param->size);
                vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
            }
            offset = alloc_param->address - heap->virtual;
        }
        else {
            if (heap->physical > alloc_param->address) {
                PRINTK_E("heap %s: physical start from 0x%"PRPx", need from 0x%"PRPx"\n",
                    heap->allocator_mgt.allocator.name, heap->physical, alloc_param->address);
                vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
            }
            if (heap->physical + heap->total_size < alloc_param->address + alloc_param->size) {
                PRINTK_E("heap %s: physical end at 0x%"PRPx", need at 0x%"PRPx"\n",
                    heap->allocator_mgt.allocator.name, heap->physical + heap->total_size,
                    alloc_param->address + alloc_param->size);
                vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
            }
            offset = alloc_param->address - heap->physical;
        }
    }

    support = vipdrv_heap_query(heap, size);
    if (!support) {
        vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

    if (alloc_param->specified) {
        /* Walk the heap backwards. */
        pos = (vipdrv_heap_node_t*)heap->list.next;
        for (; &pos->list != &heap->list; pos = (vipdrv_heap_node_t*)pos->list.next) {
            if (pos->offset <= offset) {
                /* find */
                vipdrv_heap_node_t* split = VIP_NULL;
                if (pos->status != STATUS_FREE) {
                    vipOnError(VIP_ERROR_FAILURE);
                }
                if (pos->offset + pos->size < offset + size) {
                    vipOnError(VIP_ERROR_FAILURE);
                }
                if (offset > pos->offset) {
                    split_node(heap, pos, &split, offset - pos->offset);
                    pos = split;
                }

                if (0 < pos->size - size) {
                    split_node(heap, pos, &split, size);
                }
                break;
            }
        }
    }
    else {
        /* Walk the heap backwards. */
        pos = (vipdrv_heap_node_t*)heap->list.next;
        for (; &pos->list != &heap->list; pos = (vipdrv_heap_node_t*)pos->list.next) {
            /* Check if the current node is free_bytes and is big enough. */
            offset = VIP_ALIGN(heap->physical + pos->offset, align) - (heap->physical + pos->offset);
            if ((pos->status == STATUS_FREE) && (pos->size >= (size + offset))) {
                vipdrv_heap_node_t* split = VIP_NULL;
                if (0 != offset) {
                    split_node(heap, pos, &split, offset);
                    pos = split;
                }

                if (0 < pos->size - size) {
                    split_node(heap, pos, &split, size);
                }
                break;
            }
        }
    }

    if (&pos->list == &heap->list) {
        vipOnError(VIP_ERROR_FAILURE);
    }

    /* Mark the current node as used. */
    pos->status = STATUS_USED;
    /* Update free size. */
    heap->free_bytes -= pos->size;
    handle->allocator_type = VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP;
    handle->size = alloc_param->size;
    handle->align = align;
    handle->alloc_handle = VIPDRV_UINT64_TO_PTR(((vip_uint8_t*)pos -
        (vip_uint8_t*)heap->nodes) / sizeof(vipdrv_heap_node_t));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_physical_t), (void*)&handle->physical_table));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vip_size_t), (void*)&handle->size_table));
    handle->physical_num = 1;
    handle->physical_table[0] = heap->physical + pos->offset;
    handle->size_table[0] = size;
    /* call the alloc func define in allocator  */
    status = alloc_mgt->base_callback.allocate(handle, param);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to heap alloc size=%d status=%d\n", size, status);
        vipOnError(status);
    }
    handle->vip_address = heap->virtual + (handle->physical_table[0] - heap->physical);
    handle->mem_flag |= alloc_mgt->allocator.mem_flag;
    /* has not map mmu for heap */
    if (vip_false_e == heap->mmu_map) {
        handle->mem_flag |= VIPDRV_MEM_FLAG_NO_MMU_PAGE;
    }
#if vpmdENABLE_MULTIPLE_TASK
    vipdrv_os_unlock_mutex(heap->mutex);
#endif

    return status;
onError:
    if (VIP_NULL != handle->physical_table) {
        vipdrv_os_free_memory(handle->physical_table);
        handle->physical_table = VIP_NULL;
    }
    if (VIP_NULL != handle->size_table) {
        vipdrv_os_free_memory(handle->size_table);
        handle->size_table = VIP_NULL;
    }
#if vpmdENABLE_MULTIPLE_TASK
    vipdrv_os_unlock_mutex(heap->mutex);
#endif

    return status;
}

static vip_status_e vipdrv_alloc_mgt_heap_free(
    vipdrv_video_mem_handle_t* handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_heap_node_t *pos = VIP_NULL;
    vipdrv_heap_node_t *node = VIP_NULL;
    vipdrv_alloc_mgt_heap_t* heap = VIP_NULL;
    vipdrv_alloc_mgt_t* alloc_mgt = VIP_NULL;

    if (handle == VIP_NULL) {
        PRINTK_E("fail to free heap memory\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (VIP_NULL == handle->physical_table) {
        return VIP_SUCCESS;
    }

    heap = (vipdrv_alloc_mgt_heap_t*)handle->allocator;
    alloc_mgt = &heap->allocator_mgt;
    node = &heap->nodes[VIPDRV_PTR_TO_UINT32(handle->alloc_handle)];
    if (0 == heap->total_size) {
        return VIP_SUCCESS;
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = vipdrv_os_lock_mutex(heap->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock video memory heap mutex\n");
        return VIP_ERROR_FAILURE;
    }
#endif
    if (node->status != STATUS_USED) {
        PRINTK_E("heap memory has been free heap=0x%"PRPx", node->status=%d\n",
                  handle, node->status);
    #if vpmdENABLE_MULTIPLE_TASK
        status = vipdrv_os_unlock_mutex(heap->mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to unlock video memory heap mutex\n");
        }
    #endif
        return VIP_SUCCESS;
    }

#if vpmdENABLE_FLUSH_CPU_CACHE
    /* flush cache before free this video memory to
       avoid cpu write-back when this memory has been used by others */
    if (handle->allocator->callback.flush_cache) {
        handle->allocator->callback.flush_cache(handle, VIPDRV_CACHE_FLUSH);
    }
#endif

    alloc_mgt->base_callback.free(handle);
    if (VIP_NULL != handle->physical_table) {
        vipdrv_os_free_memory(handle->physical_table);
        handle->physical_table = VIP_NULL;
    }
    if (VIP_NULL != handle->size_table) {
        vipdrv_os_free_memory(handle->size_table);
        handle->size_table = VIP_NULL;
    }

    /* Mark node as free_bytes. */
    node->status = STATUS_FREE;

    /* Add node size to free_bytes count. */
    heap->free_bytes += node->size;

    /* offset in heap->list from high to low */
    if (&heap->list != node->list.prev) {
        pos = (vipdrv_heap_node_t*)node->list.prev;
        if ((pos != VIP_NULL) && (pos->status == STATUS_FREE)) {
            if (pos->offset == (node->offset + node->size)) {
                node->size += pos->size;
                delete_list(&pos->list);
                del_node(heap, pos);
            }
        }
    }
    if (&heap->list != node->list.next) {
        pos = (vipdrv_heap_node_t*)node->list.next;
        if ((pos != VIP_NULL) && (pos->status == STATUS_FREE)) {
            if ((pos->offset + pos->size) == node->offset) {
                pos->size += node->size;
                delete_list(&node->list);
                del_node(heap, node);
            }
        }
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = vipdrv_os_unlock_mutex(heap->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to unlock video memory heap mutex\n");
    }
#endif

    return status;
}

static vip_status_e vipdrv_alloc_mgt_heap_uninit(
    vipdrv_allocator_t* allocator
    )
{
    vipdrv_alloc_mgt_heap_t* heap = (vipdrv_alloc_mgt_heap_t*)allocator;
#if vpmdNODE_MEMORY_IN_HEAP
    vipdrv_alloc_mgt_t* alloc_mgt = &heap->allocator_mgt;
#endif
    if (heap == VIP_NULL) {
        PRINTK_E("fail to uninit heap\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    if (0 == heap->total_size) {
        return VIP_SUCCESS;
    }

#if vpmdENABLE_MULTIPLE_TASK
    if (heap->mutex != VIP_NULL) {
        vipdrv_os_destroy_mutex(heap->mutex);
        heap->mutex = VIP_NULL;
    }
#endif

    if (VIP_NULL != heap) {
        /* zero heap */
        heap->free_bytes = 0;
        heap->node_capacity = 0;
        heap->physical = 0;
    #if vpmdNODE_MEMORY_IN_HEAP
        if (VIP_NULL != heap->node_handle) {
            vipdrv_video_mem_handle_t* handle = (vipdrv_video_mem_handle_t*)heap->node_handle;
            alloc_mgt->base_callback.free(heap->node_handle);
            if (VIP_NULL != handle->physical_table) {
                vipdrv_os_free_memory(handle->physical_table);
                handle->physical_table = VIP_NULL;
            }
            if (VIP_NULL != handle->size_table) {
                vipdrv_os_free_memory(handle->size_table);
                handle->size_table = VIP_NULL;
            }
            vipdrv_os_free_memory(heap->node_handle);
        }
    #else
        if (heap->nodes != VIP_NULL) {
            vipdrv_os_free_memory(heap->nodes);
        }
    #endif
        heap->nodes = VIP_NULL;
    }

    return VIP_SUCCESS;
}

vip_status_e vipdrv_alloc_mgt_heap_init(
    vipdrv_alloc_mgt_heap_t *heap,
    vipdrv_videomem_heap_info_t *heap_info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_heap_node_t *node = VIP_NULL;
    vip_size_t nodes_size = 0;
    vip_uint32_t node_cap = 0;
    vip_uint32_t i = 0;
#if vpmdNODE_MEMORY_IN_HEAP
    vipdrv_video_mem_handle_t* handle = VIP_NULL;
    vipdrv_alloc_mgt_t* alloc_mgt = &heap->allocator_mgt;
#endif
    vip_uint32_t offset = VIP_ALIGN(heap_info->vip_physical, vipdMEMORY_ALIGN_SIZE) - heap_info->vip_physical;
    vip_physical_t physical = heap_info->vip_physical + offset;
    vip_uint64_t size = heap_info->vip_memsize - offset;

    heap->physical = physical;
    heap->total_size = size;
    heap->virtual = heap->physical;
#if !VIDEO_MEMORY_HEAP_READ_ONLY
    heap->allocator_mgt.allocator.mem_flag |= VIPDRV_MEM_FLAG_MMU_PAGE_WRITEABLE;
#endif
    if ((heap->physical + heap->total_size) > 0x100000000) {
        heap->allocator_mgt.allocator.mem_flag &= ~VIPDRV_MEM_FLAG_4GB_ADDR_PA;
    }
    else {
        heap->allocator_mgt.allocator.mem_flag |= VIPDRV_MEM_FLAG_4GB_ADDR_PA;
    }
    vipdrv_os_copy_memory(&heap->allocator_mgt.base_callback,
        &heap->allocator_mgt.allocator.callback, sizeof(vipdrv_allocator_callback_t));
    heap->allocator_mgt.allocator.callback.uninit = vipdrv_alloc_mgt_heap_uninit;
    heap->allocator_mgt.allocator.callback.allocate = vipdrv_alloc_mgt_heap_alloc;
    heap->allocator_mgt.allocator.callback.free = vipdrv_alloc_mgt_heap_free;
    if (heap_info->ops.phy_cpu2vip) {
        heap->allocator_mgt.allocator.callback.phy_cpu2vip = heap_info->ops.phy_cpu2vip;
    }
    if (heap_info->ops.phy_vip2cpu) {
        heap->allocator_mgt.allocator.callback.phy_vip2cpu = heap_info->ops.phy_vip2cpu;
    }
    heap->allocator_mgt.allocator.alctor_type = VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP;
    heap->allocator_mgt.allocator.device_vis = heap_info->device_vis;

    if (0 == heap->total_size) {
        heap->idle_list.next = VIP_NULL;
        heap->free_bytes = 0;
        return VIP_SUCCESS;
    }

    if (heap->total_size > 0x6400000) {/* 100M bytes*/
        node_cap = heap->total_size / 1024 / 100; /* 100k bytes pre-node */
    }
    else if (heap->total_size > 0x1400000) {/* 20M bytes*/
        node_cap = heap->total_size / 1024 / 20; /* 20k bytes pre-node */
    }
    else if (heap->total_size > 0x400000) {/* 4M bytes*/
        node_cap = heap->total_size / 1024 / 16; /* 16k bytes pre-node */
    }
    else {
        node_cap = heap->total_size / 1024;
    }

    do {
        VIPDRV_INIT_LIST_HEAD(&heap->list);

        nodes_size = sizeof(vipdrv_heap_node_t) * node_cap;
        nodes_size = VIP_ALIGN(nodes_size, 32);
        if (nodes_size < 256) {
            nodes_size = 256;/* reserved 256bytes gap */
        }
    #if vpmdNODE_MEMORY_IN_HEAP
        vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_video_mem_handle_t), &heap->node_handle));
        vipdrv_os_zero_memory(heap->node_handle, sizeof(vipdrv_video_mem_handle_t));
        handle = (vipdrv_video_mem_handle_t*)heap->node_handle;
        handle->size = nodes_size;
        handle->align = 64;
        handle->allocator_type = heap->allocator_mgt.allocator.alctor_type;
        handle->mem_flag = heap->allocator_mgt.allocator.mem_flag | VIPDRV_MEM_FLAG_NONE_CACHE;
        vipOnError(vipdrv_os_allocate_memory(sizeof(vip_physical_t), (void*)&handle->physical_table));
        vipOnError(vipdrv_os_allocate_memory(sizeof(vip_size_t), (void*)&handle->size_table));
        handle->physical_num = 1;
        handle->physical_table[0] = heap->physical + (heap->total_size - nodes_size);
        handle->size_table[0] = nodes_size;
        handle->allocator = (void*)alloc_mgt;
        status = alloc_mgt->base_callback.allocate(heap->node_handle, VIP_NULL);
        if (VIP_SUCCESS == status) {
            status = alloc_mgt->base_callback.map_kernel_logical(heap->node_handle);
            if (status != VIP_SUCCESS) {
                PRINTK_E("fail to map kernel logical for heap nodes\n");
            }
            else {
                heap->nodes = (vipdrv_heap_node_t*)handle->kerl_logical;
            }
        }
    #endif
        if (VIP_NULL == heap->nodes) {
            status = vipdrv_os_allocate_memory(nodes_size, (void**)&heap->nodes);
            if (status != VIP_SUCCESS) {
                PRINTK_E("fail to alloc memory for heap nodes\n");
                break;
            }
            vipdrv_os_zero_memory(heap->nodes, nodes_size);
            nodes_size = 0;
        }

        heap->node_capacity = node_cap;
        heap->free_bytes = heap->total_size - nodes_size;

        PRINTK_I("video memory heap %s total free:0x%x bytes, node used:0x%x, node capacity:%d, node=%s\n",
                 heap->allocator_mgt.allocator.name, heap->free_bytes, nodes_size, node_cap,
                 vpmdNODE_MEMORY_IN_HEAP ? "in heap": "dyna_alloc");

        for (i = 0; i < node_cap; i++) {
            heap->nodes[i].status = STATUS_INIT;
            heap->nodes[i].list.next = heap->idle_list.next;
            heap->idle_list.next = &heap->nodes[i].list;
        }

        status = new_node(heap, &node);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to new node.\n");
            break;
        }

        node->offset = 0;
        node->size = heap->free_bytes;
        node->status = STATUS_FREE;
        add_list_tail(&node->list, &heap->list);
    } while (0);

#if vpmdENABLE_MULTIPLE_TASK
    status |= vipdrv_os_create_mutex(&heap->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create mutex for video heap\n");
        return VIP_ERROR_FAILURE;
    }
#endif

    PRINTK_D("video memory heap capability=0x%x\n", heap->allocator_mgt.allocator.mem_flag);
    return status;
#if vpmdNODE_MEMORY_IN_HEAP
onError:
    if (VIP_NULL != heap->node_handle) {
        if (VIP_NULL != handle) {
            if (VIP_NULL != handle->physical_table) {
                vipdrv_os_free_memory(handle->physical_table);
                handle->physical_table = VIP_NULL;
            }
            if (VIP_NULL != handle->size_table) {
                vipdrv_os_free_memory(handle->size_table);
                handle->size_table = VIP_NULL;
            }
        }
        alloc_mgt->base_callback.free(heap->node_handle);
        vipdrv_os_free_memory(heap->node_handle);
        heap->node_handle = VIP_NULL;
    }
    return status;
#endif
}

vip_status_e vipdrv_heap_dump(
    vipdrv_alloc_mgt_heap_t *heap
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_heap_node_t *node = VIP_NULL;
    vip_uint64_t free_node_max_size = 0;
    vip_uint32_t index = 0, used_cnt = 0;

    node = (vipdrv_heap_node_t *)heap->list.next;
    for (; &node->list != &heap->list; node = (vipdrv_heap_node_t*)node->list.next) {
        if (node->status == STATUS_FREE) {
            if (node->size > free_node_max_size) {
                free_node_max_size = node->size;
            }
            PRINTK_I("%d node offset=0x%"PRIx64", free size=%"PRId32"\n", index, node->offset, node->size);
        }
        else {
            used_cnt++;
        }
        index++;
    }

    PRINTK("heap %s node total cnt=%u, used cnt=%u, node capacity=%u, free node max size=%u "
            "node new cnt=%u del cnt=%u\n",
       heap->allocator_mgt.allocator.name, index, used_cnt, heap->node_capacity,
       free_node_max_size, heap->node_new_cnt, heap->node_del_cnt);

    return status;
}

#endif

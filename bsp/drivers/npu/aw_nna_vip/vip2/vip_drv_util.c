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
#include <vip_drv_util.h>
#include <vip_drv_device_driver.h>

/*
  when high-priority network lack of resource (eg. need core 0&1, but only core 0 is idle),
  allow low-priority network, which is not more than MAX_PRIORITY_GAP level lower, execute in advance.
  when set to 0, the low-priority network is not allow to preempt while high-priority is waiting for resource.
*/
#define MAX_PRIORITY_GAP        10
/*
  when high-priority network lack of resource (eg. need core 0&1, but only core 0 is idle),
  allow the next MAX_PREEMPT_COUNT low-priority network execute in advance.
  when set to 0, the latter low-priority network is not allow to preempt while high-priority is waiting for resource.
*/
#define MAX_PREEMPT_COUNT       10

vip_status_e vipdrv_create_recursive_mutex(
    vipdrv_recursive_mutex *recursive_mutex
    )
{
    recursive_mutex->current_tid = 0;
    recursive_mutex->ref_count = 0;
    return vipdrv_os_create_mutex(&recursive_mutex->mutex);
}

vip_status_e vipdrv_destroy_recursive_mutex(
    vipdrv_recursive_mutex *recursive_mutex
    )
{
    recursive_mutex->current_tid = 0;
    recursive_mutex->ref_count = 0;
    if (VIP_NULL != recursive_mutex->mutex) {
        vipdrv_os_destroy_mutex(recursive_mutex->mutex);
        recursive_mutex->mutex = VIP_NULL;
    }
    return VIP_SUCCESS;
}

vip_status_e vipdrv_lock_recursive_mutex(
    vipdrv_recursive_mutex *recursive_mutex
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t cur_tid = vipdrv_os_get_tid();
    if (cur_tid == recursive_mutex->current_tid) {
        recursive_mutex->ref_count += 1;
    }
    else {
        vipOnError(vipdrv_os_lock_mutex(recursive_mutex->mutex));
        recursive_mutex->current_tid = cur_tid;
        recursive_mutex->ref_count += 1;
    }

onError:
    return status;
}

vip_status_e vipdrv_unlock_recursive_mutex(
    vipdrv_recursive_mutex *recursive_mutex
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t cur_tid = vipdrv_os_get_tid();
    if (cur_tid == recursive_mutex->current_tid) {
        recursive_mutex->ref_count -= 1;
    }
    else {
        PRINTK_E("thread %d call unlock before getting the mutex\n", cur_tid);
    }

    if (0 >= recursive_mutex->ref_count) {
        recursive_mutex->current_tid = 0;
        recursive_mutex->ref_count = 0;
        vipOnError(vipdrv_os_unlock_mutex(recursive_mutex->mutex));
    }

onError:
    return status;
}

vip_status_e vipdrv_create_readwrite_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    )
{
    vip_status_e status = VIP_SUCCESS;
    readwrite_mutex->r_active_cnt = 0;
    readwrite_mutex->w_active = 0;
    vipOnError(vipdrv_os_create_mutex(&readwrite_mutex->rw_mutex));
onError:
    return status;
}

vip_status_e vipdrv_destroy_readwrite_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    )
{
    readwrite_mutex->r_active_cnt = 0;
    readwrite_mutex->w_active = 0;
    if (VIP_NULL != readwrite_mutex->rw_mutex) {
        vipdrv_os_destroy_mutex(readwrite_mutex->rw_mutex);
        readwrite_mutex->rw_mutex = VIP_NULL;
    }

    return VIP_SUCCESS;
}

vip_status_e vipdrv_lock_read_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    )
{
    vip_status_e status = VIP_SUCCESS;

    status = vipdrv_os_lock_mutex(readwrite_mutex->rw_mutex);
    if (VIP_SUCCESS != status) {
        return status;
    }

    while (readwrite_mutex->w_active) {
        vipdrv_os_unlock_mutex(readwrite_mutex->rw_mutex);
        vipdrv_os_delay(1);
        status = vipdrv_os_lock_mutex(readwrite_mutex->rw_mutex);
        if (VIP_SUCCESS != status) {
            return status;
        }
    }
    readwrite_mutex->r_active_cnt += 1;
    vipdrv_os_unlock_mutex(readwrite_mutex->rw_mutex);
    return status;
}

vip_status_e vipdrv_unlock_read_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    )
{
    vip_status_e status = VIP_SUCCESS;

    status = vipdrv_os_lock_mutex(readwrite_mutex->rw_mutex);
    if (VIP_SUCCESS != status) {
        return status;
    }
    readwrite_mutex->r_active_cnt -= 1;
    vipdrv_os_unlock_mutex(readwrite_mutex->rw_mutex);
    return status;
}

vip_status_e vipdrv_lock_write_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    )
{
    vip_status_e status = VIP_SUCCESS;

    status = vipdrv_os_lock_mutex(readwrite_mutex->rw_mutex);
    if (VIP_SUCCESS != status) {
        return status;
    }

    while (readwrite_mutex->w_active || readwrite_mutex->r_active_cnt) {
        vipdrv_os_unlock_mutex(readwrite_mutex->rw_mutex);
        vipdrv_os_delay(1);
        status = vipdrv_os_lock_mutex(readwrite_mutex->rw_mutex);
        if (VIP_SUCCESS != status) {
            return status;
        }
    }

    readwrite_mutex->w_active = 1;
    vipdrv_os_unlock_mutex(readwrite_mutex->rw_mutex);

    return status;
}

vip_status_e vipdrv_unlock_write_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    )
{
    vip_status_e status = VIP_SUCCESS;

    status = vipdrv_os_lock_mutex(readwrite_mutex->rw_mutex);
    if (VIP_SUCCESS != status) {
        return status;
    }
    readwrite_mutex->w_active = 0;
    vipdrv_os_unlock_mutex(readwrite_mutex->rw_mutex);
    return status;
}

#if vpmdTASK_QUEUE_USED
#if vpmdENABLE_PREEMPTION
static vip_bool_e cmp_node(
    vipdrv_queue_node_t i,
    vipdrv_queue_node_t j
    )
{
    if (i.data->priority > j.data->priority) {
        return vip_true_e;
    }
    else if (i.data->priority < j.data->priority) {
        return vip_false_e;
    }

    if (i.index < j.index) {
        return vip_true_e;
    }

    return vip_false_e;
}

static vip_status_e swap_node(
    vipdrv_max_heap_t* queue,
    vip_uint32_t i,
    vip_uint32_t j
    )
{
    vipdrv_queue_node_t temp;
    temp = queue->head[i];
    queue->head[i] = queue->head[j];
    queue->head[j] = temp;

    return VIP_SUCCESS;
}

static vip_status_e ensure_capacity(
    vipdrv_max_heap_t* queue
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_queue_node_t* new_head = VIP_NULL;

    if (queue->property.task_count < queue->property.capacity) {
        return status;
    }

    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_queue_node_t) * queue->property.capacity * 2,
                                        (void**)&new_head));
    vipdrv_os_zero_memory(new_head, sizeof(vipdrv_queue_node_t) * queue->property.capacity * 2);
    vipdrv_os_copy_memory(new_head, queue->head, sizeof(vipdrv_queue_node_t) * queue->property.capacity);

    vipdrv_os_free_memory(queue->head);
    queue->head = new_head;
    queue->property.capacity *= 2;

onError:
    return status;
}

static vip_status_e queue_initialize(
    vipdrv_priority_queue_t *queue,
    vip_uint32_t task_heap_count
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    if (1 > task_heap_count) {
        PRINTK_E("1 task heap at least\n");
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

    /* queue->max_heap[task_heap_count] as the header of task list */
    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_max_heap_t) * (task_heap_count + 1),
        (void**)&queue->max_heap));
    vipdrv_os_zero_memory(queue->max_heap, sizeof(vipdrv_max_heap_t) * (task_heap_count + 1));
    for (i = 0; i < task_heap_count; i++) {
        queue->max_heap[i].property.task_count = 0;
        queue->max_heap[i].property.capacity = QUEUE_CAPABILITY;
        queue->max_heap[i].property.estimated_time = 0;

        vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_queue_node_t) * QUEUE_CAPABILITY,
            (void**)&queue->max_heap[i].head));
        vipdrv_os_zero_memory(queue->max_heap[i].head, sizeof(vipdrv_queue_node_t) * QUEUE_CAPABILITY);
    }
    queue->sum_count = 0;
    queue->cur_count = 0;
    queue->max_heap_count = task_heap_count;

onError:
    return status;
}

static vip_status_e queue_destroy(
    vipdrv_priority_queue_t *queue
    )
{
    vip_uint32_t i = 0;

    for (i = 0; i < queue->max_heap_count; i++) {
        vipdrv_os_free_memory(queue->max_heap[i].head);
        queue->max_heap[i].head = VIP_NULL;
        queue->max_heap[i].property.task_count = 0;
        queue->max_heap[i].property.capacity = 0;
        queue->max_heap[i].property.estimated_time = 0;
        queue->max_heap[i].left = VIP_NULL;
        queue->max_heap[i].right = VIP_NULL;
    }
    vipdrv_os_free_memory(queue->max_heap);
    queue->max_heap = VIP_NULL;
    queue->sum_count = 0;
    queue->cur_count = 0;
    return VIP_SUCCESS;
}

static void keep_max_heap(
    vipdrv_max_heap_t *max_heap,
    vip_uint32_t index
    )
{
    vip_uint32_t left = 0;
    vip_uint32_t right = 0;
    vip_uint32_t priority = index;
    vip_uint32_t parent = 0;
    /* reorder to max heap */
    while (index > 0) { /* not root node */
        parent = (index - 1) / 2;

        if (cmp_node(max_heap->head[parent], max_heap->head[index])) break;

        swap_node(max_heap, index, parent);
        index = parent;
    }
    index = priority;
    while (index * 2 + 1 < max_heap->property.task_count) { /* not leaf node */
        left = index * 2 + 1;
        right = index * 2 + 2;

        if (left < max_heap->property.task_count && cmp_node(max_heap->head[left], max_heap->head[priority])) {
            priority = left;
        }
        if (right < max_heap->property.task_count && cmp_node(max_heap->head[right], max_heap->head[priority])) {
            priority = right;
        }

        /* priority of current node is higher than left/right node */
        if (priority == index) break;

        swap_node(max_heap, index, priority);
        index = priority;
    }
}

static void reorder_after_del(
    vipdrv_max_heap_t* max_heap
    )
{
    if (0 == max_heap->property.task_count) {
        /* del from list */
        max_heap->left->right = max_heap->right;
        if (VIP_NULL != max_heap->right) {
            max_heap->right->left = max_heap->left;
        }
        max_heap->left = VIP_NULL;
        max_heap->right = VIP_NULL;
    }
    else {
        /* reorder */
        while (VIP_NULL != max_heap->right) {
            vipdrv_max_heap_t* right_node = max_heap->right;
            if (cmp_node(right_node->head[0], max_heap->head[0])) {
                /* swap current node and right node */
                if (VIP_NULL != right_node->right) {
                    right_node->right->left = max_heap;
                }
                max_heap->left->right = right_node;
                right_node->left = max_heap->left;
                max_heap->right = right_node->right;
                right_node->right = max_heap;
                max_heap->left = right_node;
            }
            else {
                break;
            }
        }
    }
}

static vip_bool_e queue_read(
    vipdrv_priority_queue_t *queue,
    vipdrv_queue_data_t **data_ptr,
    void *flag
    )
{
    vipdrv_max_heap_t* max_heap = queue->max_heap[queue->max_heap_count].right;
    vip_uint16_t need_res_mask = 0;
    vip_uint8_t max_priority = 0;
    vip_uint32_t min_index = -1;
    vip_uint16_t res_mask = *(vip_uint16_t*)flag;
    /* read data while queue is empty */
    if (0 == queue->cur_count) {
        return vip_false_e;
    }

    while (VIP_NULL != max_heap) {
        /* enough resource or not */
        if (res_mask == (max_heap->head[0].data->res_mask | res_mask)) {
            vip_bool_e wait = vip_false_e;
            /* conflict with high-priority task or not */
            if (0 == (max_heap->head[0].data->res_mask & need_res_mask)) {
                break;
            }

            if ((max_priority > max_heap->head[0].data->priority) &&
                (max_priority - max_heap->head[0].data->priority > MAX_PRIORITY_GAP)) {
                /* priority of high-priority task is much higher than current task */
                wait = vip_true_e;
            }

            if ((max_heap->head[0].index > min_index) &&
                (max_heap->head[0].index - min_index > MAX_PREEMPT_COUNT)) {
                /* submit time of high-priority task is much earlier than current task */
                wait = vip_true_e;
            }
            if (vip_false_e == wait) {
                break;
            }
        }
        need_res_mask |= max_heap->head[0].data->res_mask;
        max_priority = max_heap->head[0].data->priority > max_priority ?
                       max_heap->head[0].data->priority : max_priority;
        min_index = max_heap->head[0].index < min_index ?
                    max_heap->head[0].index : min_index;
        max_heap = max_heap->right;
    }

    if (VIP_NULL == max_heap) {
        /* no task can be executed */
        return vip_false_e;
    }

    *data_ptr = max_heap->head[0].data;
    /* swap the first and the last node */
    swap_node(max_heap, 0, max_heap->property.task_count - 1);
    max_heap->property.task_count--;
    max_heap->property.estimated_time -= (*data_ptr)->time;
    queue->cur_count--;
    if (0 == queue->cur_count) {
        queue->sum_count = 0;
    }
    keep_max_heap(max_heap, 0);
    reorder_after_del(max_heap);

    return vip_true_e;
}

static vip_bool_e queue_write(
    vipdrv_priority_queue_t *queue,
    vipdrv_queue_data_t *data,
    void *flag
    )
{
    vipdrv_max_heap_t* max_heap = VIP_NULL;
    vip_uint32_t index = 0;
    vip_uint32_t* task_heap_index = (vip_uint32_t*)flag;

    if (*task_heap_index >= queue->max_heap_count) {
        PRINTK_E("write fail. task heap index = %d, max task heap count = %d\n",
                *task_heap_index, queue->max_heap_count);
        return vip_false_e;
    }
    max_heap = &queue->max_heap[*task_heap_index];
    /* make sure there is space when write data */
    if (VIP_SUCCESS != ensure_capacity(max_heap)) {
        return vip_false_e;
    }
    index = max_heap->property.task_count;

    max_heap->head[index].data = data;
    max_heap->head[index].index = queue->sum_count;
    max_heap->property.task_count++;
    max_heap->property.estimated_time += data->time;
    queue->cur_count++;
    queue->sum_count++;
    keep_max_heap(max_heap, index);

    if (1 == max_heap->property.task_count) {
        /* insert into list */
        vipdrv_max_heap_t* q = &queue->max_heap[queue->max_heap_count];
        vipdrv_max_heap_t* p = q->right;
        while (VIP_NULL != p) {
            if (cmp_node(max_heap->head[0], p->head[0])) {
                break;
            }
            q = p;
            p = p->right;
        }
        q->right = max_heap;
        if (VIP_NULL != p) {
            p->left = max_heap;
        }
        max_heap->left = q;
        max_heap->right = p;
    }
    else {
        /* reorder */
        while (VIP_NULL != max_heap->left->head) {
            vipdrv_max_heap_t* left_node = max_heap->left;
            if (cmp_node(max_heap->head[0], left_node->head[0])) {
                /* swap current node and left node */
                if (VIP_NULL != max_heap->right) {
                    max_heap->right->left = left_node;
                }
                left_node->left->right = max_heap;
                max_heap->left = left_node->left;
                left_node->right = max_heap->right;
                max_heap->right = left_node;
                left_node->left = max_heap;
            }
            else {
                break;
            }
        }
    }

    if ((vip_uint32_t)-1 == queue->sum_count) {
        vip_uint32_t i = 0, j = 0;
        for (i = 0; i < queue->max_heap_count; i++) {
            for (j = 0; j < queue->max_heap[i].property.task_count; j++) {
                queue->max_heap[i].head[j].index = 0;
            }
        }
    }

    return vip_true_e;
}

static vip_status_e queue_clean(
    vipdrv_priority_queue_t *queue,
    vipdrv_queue_clean_flag_e flag,
    void *param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_max_heap_t* max_heap = queue->max_heap[queue->max_heap_count].right;
    if (VIPDRV_QUEUE_CLEAN_ALL == flag) {
        /* clean all */
        while (VIP_NULL != max_heap) {
            max_heap->property.task_count = 0;
            max_heap->property.estimated_time = 0;
            max_heap = max_heap->right;
        }
        queue->cur_count = 0;
        queue->sum_count = 0;
        queue->max_heap[queue->max_heap_count].right = VIP_NULL;
    }
    else if (VIPDRV_QUEUE_CLEAN_STASK == flag) {
        /* clean related tasks according to resource mask */
        vip_uint32_t clean_mask = *(vip_uint32_t*)param;
        status = VIP_ERROR_INVALID_ARGUMENTS;
        while (VIP_NULL != max_heap) {
            /* match resource or not */
            if (0 != (clean_mask & max_heap->head[0].data->res_mask)) {
                vipdrv_max_heap_t* temp = max_heap->left;
                status = VIP_SUCCESS;
                queue->cur_count -= max_heap->property.task_count;
                max_heap->property.task_count = 0;
                max_heap->property.estimated_time = 0;
                /* del from list */
                max_heap->left->right = max_heap->right;
                if (VIP_NULL != max_heap->right) {
                    max_heap->right->left = max_heap->left;
                }
                max_heap->left = VIP_NULL;
                max_heap->right = VIP_NULL;
                max_heap = temp;
            }
            max_heap = max_heap->right;
        }

        if (0 == queue->cur_count) {
            queue->sum_count = 0;
        }
    }
    else if (VIPDRV_QUEUE_CLEAN_ONE == flag) {
        /* clean one task according to index */
        vip_uint32_t index = *(vip_uint32_t*)param;
        vip_uint32_t i = 0;
        status = VIP_ERROR_INVALID_ARGUMENTS;
        while (VIP_NULL != max_heap) {
            /* match resource or not */
            for (i = 0; i < max_heap->property.task_count; i++) {
                if (index == max_heap->head[i].data->v1) {
                    max_heap->property.estimated_time -= max_heap->head[i].data->time;
                    swap_node(max_heap, i, max_heap->property.task_count - 1);
                    max_heap->property.task_count--;
                    queue->cur_count--;
                    if (0 == queue->cur_count) {
                        queue->sum_count = 0;
                    }
                    keep_max_heap(max_heap, i);
                    reorder_after_del(max_heap);

                    return VIP_SUCCESS;
                }
            }
            max_heap = max_heap->right;
        }
    }

    return status;
}

static vipdrv_queue_status_e queue_status(
    vipdrv_priority_queue_t *queue,
    void* flag
    )
{
    vipdrv_queue_status_e status = VIPDRV_QUEUE_EMPTY;
    vip_uint16_t res_mask = *(vip_uint16_t*)flag;
    vipdrv_max_heap_t* max_heap = queue->max_heap[queue->max_heap_count].right;

    while (VIP_NULL != max_heap) {
        /* related task */
        if (max_heap->head[0].data->res_mask & res_mask) {
            status = VIPDRV_QUEUE_WITH_DATA;
            break;
        }
        max_heap = max_heap->right;
    }

    return status;
}

static vip_status_e queue_query_property(
    vipdrv_priority_queue_t *queue,
    void* flag,
    vipdrv_queue_property_t** property
    )
{
    vipdrv_max_heap_t* max_heap = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t* task_heap_index = (vip_uint32_t*)flag;

    if (*task_heap_index >= queue->max_heap_count) {
        PRINTK_E("query fail. task heap index = %d, max task heap count = %d\n",
                *task_heap_index, queue->max_heap_count);
        return VIP_ERROR_FAILURE;
    }

    max_heap = &queue->max_heap[*task_heap_index];
    *property = &max_heap->property;

    return status;
}
#else
static vip_status_e queue_initialize(
    vipdrv_fifo_queue *queue,
    vip_uint32_t no_use
    )
{
    /* queue is empty */
    queue->end_index = -1;
    queue->begin_index = 0;

    return VIP_SUCCESS;
}

static vip_status_e queue_destroy(
    vipdrv_fifo_queue *queue
    )
{
    return VIP_SUCCESS;
}

static vip_bool_e queue_read(
    vipdrv_fifo_queue *queue,
    vipdrv_queue_data_t **data_ptr,
    void* no_use
    )
{
    /* Read data if queue is not empty */
    if (queue->end_index != -1) {
        *data_ptr = queue->data[queue->begin_index];
        queue->begin_index = (queue->begin_index + 1) % QUEUE_CAPABILITY;

        if (queue->begin_index == queue->end_index) {
            queue->end_index = -1;
        }
        return vip_true_e;
    }

    return vip_false_e;
}

static vip_bool_e queue_write(
    vipdrv_fifo_queue *queue,
    vipdrv_queue_data_t *data,
    void* no_use
    )
{
    /* Write data if queue is not full */
    if (queue->begin_index != queue->end_index) {
        if (queue->end_index == -1) {
            queue->end_index = queue->begin_index;
        }
        queue->data[queue->end_index] = data;
        queue->end_index = (queue->end_index + 1) % QUEUE_CAPABILITY;

        return vip_true_e;
    }

    return vip_false_e;
}

static vip_status_e queue_clean(
    vipdrv_fifo_queue *queue,
    vipdrv_queue_clean_flag_e flag,
    void* param
    )
{
    queue->end_index = -1;
    return VIP_SUCCESS;
}
static vipdrv_queue_status_e queue_status(
    vipdrv_fifo_queue *queue,
    void* flag
    )
{
    vipdrv_queue_status_e status;
    if (queue->begin_index == queue->end_index) {
        status = VIPDRV_QUEUE_FULL;
    }
    else if (queue->end_index == -1) {
        status = VIPDRV_QUEUE_EMPTY;
    }
    else {
        status=  VIPDRV_QUEUE_WITH_DATA;
    }

    return status;
}

static vip_status_e queue_query_property(
    vipdrv_fifo_queue *queue,
    void* flag,
    vipdrv_queue_property_t** property
    )
{
    vip_status_e status = VIP_SUCCESS;
    *property = &queue->property;

    return status;
}
#endif
vip_status_e vipdrv_queue_initialize(
    vipdrv_queue_t *queue,
    vip_uint32_t device_index,
    vip_uint32_t queue_count
    )
{
    vip_status_e status = VIP_SUCCESS;

    status = vipdrv_os_create_mutex(&queue->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create mutex\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    queue->queue_stop = vip_false_e;
    status = queue_initialize(&queue->queue, queue_count);

onError:
    return status;
}

vip_status_e vipdrv_queue_destroy(
    vipdrv_queue_t *queue
    )
{
    vip_status_e status = VIP_SUCCESS;

    queue->queue_stop = vip_true_e;
    if (queue->mutex != VIP_NULL) {
        vipdrv_os_destroy_mutex(queue->mutex);
    }

    queue_destroy(&queue->queue);

    return status;
}

/*
@param queue, queue object.
@param data, the data object wrote to queue.
@param flag, the mask of hardward core id which wait-link table not full.
*/
vip_bool_e vipdrv_queue_read(
    vipdrv_queue_t *queue,
    vipdrv_queue_data_t **data_ptr,
    void *flag
    )
{
    vip_bool_e read = vip_false_e;
    vip_status_e status = VIP_SUCCESS;

    if (queue->queue_stop) {
        return read;
    }

    status = vipdrv_os_lock_mutex(queue->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock mutex when reading\n");
        return vip_false_e;
    }
    read = queue_read(&queue->queue, data_ptr, flag);
    vipdrv_os_unlock_mutex(queue->mutex);
    return read;
}

/*
@param queue, queue object.
@param data, the data object wrote to queue.
@param flag, the task max_heap index for task management.
*/
vip_bool_e vipdrv_queue_write(
    vipdrv_queue_t *queue,
    vipdrv_queue_data_t *data,
    void *flag
    )
{
    vip_bool_e wrote = vip_false_e;
    vip_status_e status = VIP_SUCCESS;

    status = vipdrv_os_lock_mutex(queue->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock mutex when writing\n");
        return vip_false_e;
    }
    wrote = queue_write(&queue->queue, data, flag);
    vipdrv_os_unlock_mutex(queue->mutex);
    return wrote;
}

vip_status_e vipdrv_queue_clean(
    vipdrv_queue_t *queue,
    vipdrv_queue_clean_flag_e flag,
    void* param
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == queue) {
        PRINTK_E("fail to clean queue, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    vipdrv_os_lock_mutex(queue->mutex);
    status = queue_clean(&queue->queue, flag, param);
    vipdrv_os_unlock_mutex(queue->mutex);

    return status;
}

vipdrv_queue_status_e vipdrv_queue_status(
    vipdrv_queue_t *queue,
    void *flag
    )
{
    vipdrv_queue_status_e status = VIPDRV_QUEUE_NONE;

    if (VIP_NULL == queue) {
        return VIPDRV_QUEUE_NONE;
    }

    vipdrv_os_lock_mutex(queue->mutex);
    status = queue_status(&queue->queue, flag);
    vipdrv_os_unlock_mutex(queue->mutex);

    return status;
}

vip_status_e vipdrv_queue_query_property(
    vipdrv_queue_t *queue,
    void* flag,
    vipdrv_queue_property_t** property
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == queue) {
        PRINTK_E("fail to query property of queue\n");
        return VIP_ERROR_FAILURE;
    }

    vipdrv_os_lock_mutex(queue->mutex);
    status = queue_query_property(&queue->queue, flag, property);
    vipdrv_os_unlock_mutex(queue->mutex);

    return status;
}
#endif

#define HANDLE_MAGIC_DATA 0x1000000000000000
static vip_int32_t hash(
    vip_uint64_t h
    )
{
    h = (h << 5) - h;
    return (h ^ (h >> 8) ^ (h >> 16));
}

static void left_rotate(
    vipdrv_hashmap_t *hashmap,
    vip_uint32_t index,
    vipdrv_hashmap_node_t *header
    )
{
    vip_uint32_t parent = hashmap->node_array[index].parent_index;
    vip_uint32_t right = hashmap->node_array[index].right_index;
    /* current node is root or not */
    vipdrv_hashmap_node_t *p_parent = (0 == parent) ? header : &hashmap->node_array[parent];
    vip_uint32_t *p = p_parent->left_index == index ?
                      &p_parent->left_index : &p_parent->right_index;

    *p = right;
    hashmap->node_array[index].right_index = hashmap->node_array[right].left_index;
    hashmap->node_array[index].parent_index = right;
    hashmap->node_array[right].left_index = index;
    hashmap->node_array[right].parent_index = parent;
    hashmap->node_array[hashmap->node_array[index].right_index].parent_index = index;
}

static void right_rotate(
    vipdrv_hashmap_t *hashmap,
    vip_uint32_t index,
    vipdrv_hashmap_node_t *header
    )
{
    vip_uint32_t parent = hashmap->node_array[index].parent_index;
    vip_uint32_t left = hashmap->node_array[index].left_index;
    /* current node is root or not */
    vipdrv_hashmap_node_t *p_parent = (0 == parent) ? header : &hashmap->node_array[parent];
    vip_uint32_t *p = p_parent->left_index == index ?
                      &p_parent->left_index : &p_parent->right_index;

    *p = left;
    hashmap->node_array[index].left_index = hashmap->node_array[left].right_index;
    hashmap->node_array[index].parent_index = left;
    hashmap->node_array[left].right_index = index;
    hashmap->node_array[left].parent_index = parent;
    hashmap->node_array[hashmap->node_array[index].left_index].parent_index = index;
}

static void swap_hashmap_node(
    vipdrv_hashmap_t *hashmap,
    vipdrv_hashmap_node_t *header,
    vip_uint32_t node_1,
    vip_uint32_t node_2
    )
{
    vip_uint32_t node_1_parent = hashmap->node_array[node_1].parent_index;
    vip_uint32_t node_2_parent = hashmap->node_array[node_2].parent_index;
    vip_uint32_t node_1_left = hashmap->node_array[node_1].left_index;
    vip_uint32_t node_2_left = hashmap->node_array[node_2].left_index;
    vip_uint32_t node_1_right = hashmap->node_array[node_1].right_index;
    vip_uint32_t node_2_right = hashmap->node_array[node_2].right_index;
    vip_bool_e   node_1_is_black = hashmap->node_array[node_1].is_black;
    vip_bool_e   node_2_is_black = hashmap->node_array[node_2].is_black;

    vip_uint32_t *p_1 = (0 == node_1_parent) ? &header->right_index :
                        ((node_1 == hashmap->node_array[node_1_parent].left_index) ?
                         &hashmap->node_array[node_1_parent].left_index :
                         &hashmap->node_array[node_1_parent].right_index);

    vip_uint32_t *p_2 = (0 == node_2_parent) ? &header->right_index :
                        ((node_2 == hashmap->node_array[node_2_parent].left_index) ?
                         &hashmap->node_array[node_2_parent].left_index :
                         &hashmap->node_array[node_2_parent].right_index);

    *p_1 = node_2;
    *p_2 = node_1;

    if (0 != node_1_left) hashmap->node_array[node_1_left].parent_index = node_2;
    if (0 != node_1_right) hashmap->node_array[node_1_right].parent_index = node_2;
    if (0 != node_2_left) hashmap->node_array[node_2_left].parent_index = node_1;
    if (0 != node_2_right) hashmap->node_array[node_2_right].parent_index = node_1;

    hashmap->node_array[node_1].parent_index = node_2_parent == node_1 ? node_2 : node_2_parent;
    hashmap->node_array[node_2].parent_index = node_1_parent == node_2 ? node_1 : node_1_parent;
    hashmap->node_array[node_1].left_index = node_2_left == node_1 ? node_2 : node_2_left;
    hashmap->node_array[node_1].right_index = node_2_right == node_1 ? node_2 : node_2_right;
    hashmap->node_array[node_2].left_index = node_1_left == node_2 ? node_1: node_1_left;
    hashmap->node_array[node_2].right_index = node_1_right == node_2 ? node_1 : node_1_right;
    hashmap->node_array[node_1].is_black = node_2_is_black;
    hashmap->node_array[node_2].is_black = node_1_is_black;
}

vip_status_e vipdrv_hashmap_init(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint32_t init_capacity,
    IN vip_uint32_t size_per_element,
    IN vip_char_t_ptr name,
    IN vip_status_e(*callback_unuse)(void* element),
    IN vip_status_e(*callback_remove)(void* element),
    IN vip_status_e(*callback_destroy)(void* element)
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t i = 0;
    vip_uint32_t next_index = 0;
    hashmap->capacity = init_capacity + 1; /* index 0: idle list header */
    hashmap->size_per_element = size_per_element;
    hashmap->name = name;

    vipOnError(vipdrv_create_recursive_mutex(&hashmap->mutex));
    vipOnError(vipdrv_create_readwrite_mutex(&hashmap->expand_mutex));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_hashmap_node_t) * hashmap->capacity,
        (void**)&hashmap->node_array));
    vipdrv_os_zero_memory(hashmap->node_array, sizeof(vipdrv_hashmap_node_t) * hashmap->capacity);

    if (0 < hashmap->size_per_element) {
        vipOnError(vipdrv_os_allocate_memory(hashmap->size_per_element * hashmap->capacity,
                                          (void**)&hashmap->container));
        vipdrv_os_zero_memory(hashmap->container, hashmap->size_per_element * hashmap->capacity);
    }

    for (i = hashmap->capacity - 1; i > 0; i--) {
        hashmap->node_array[i].right_index = next_index;
        hashmap->node_array[i].left_index = 0;
        next_index = i;
        vipOnError(VIPDRV_SPINLOCK(create, &hashmap->node_array[i].spinlock));
    }
    hashmap->idle_list_head = &hashmap->node_array[0];
    hashmap->idle_list_head->right_index = next_index;
#if HASH_MAP_IDLE_LIST_TAIL
    hashmap->idle_list_tail = &hashmap->node_array[hashmap->capacity - 1];
#endif
    for (i = 0; i < HASH_MAP_CAPABILITY; i++) {
        hashmap->hashmap[i].right_index = 0;
    }
    hashmap->free_pos = 1; /* index 0: idle list header */
    hashmap->callback_unuse = callback_unuse;
    hashmap->callback_remove = callback_remove;
    hashmap->callback_destroy = callback_destroy;
    /*PRINTK("HASHMAP 0x%"PRPx"(%s) INIT SUCCESS\n", hashmap, name);*/
    return status;

onError:
    vipdrv_destroy_recursive_mutex(&hashmap->mutex);
    vipdrv_destroy_readwrite_mutex(&hashmap->expand_mutex);
    if (VIP_NULL != hashmap->node_array) {
        for (i = hashmap->capacity - 1; i > 0; i--) {
            if (VIP_NULL != hashmap->node_array[i].spinlock) {
                VIPDRV_SPINLOCK(destroy, hashmap->node_array[i].spinlock);
                hashmap->node_array[i].spinlock = VIP_NULL;
            }
        }

        vipdrv_os_free_memory(hashmap->node_array);
        hashmap->node_array = VIP_NULL;
    }
    if (VIP_NULL != hashmap->container) {
        vipdrv_os_free_memory(hashmap->container);
        hashmap->container = VIP_NULL;
    }
    return status;
}

vip_status_e vipdrv_hashmap_expand(
    IN vipdrv_hashmap_t *hashmap,
    OUT vip_bool_e *expand
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t next_index = 0;
    vip_uint32_t index = 0;
    vip_status_e status = VIP_SUCCESS;
    vipdrv_hashmap_node_t *new_node_array = VIP_NULL;
    void *new_container;
    *expand = vip_false_e;

    status = vipdrv_lock_write_mutex(&hashmap->expand_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock expand mutex when hashmap expand\n");
        return status;
    }
    if (vip_false_e == vipdrv_hashmap_full(hashmap)) {
        PRINTK_D("no need to expand hashmap\n");
        vipGoOnError(status);
    }
    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_hashmap_node_t) *
                                       (hashmap->capacity + HASH_MAP_CAPABILITY),
                                       (void**)&new_node_array));
    vipdrv_os_zero_memory(new_node_array,
                          sizeof(vipdrv_hashmap_node_t) * (hashmap->capacity + HASH_MAP_CAPABILITY));

    if (0 < hashmap->size_per_element) {
        status = vipdrv_os_allocate_memory(hashmap->size_per_element *
                                          (hashmap->capacity + HASH_MAP_CAPABILITY),
                                          (void**)&new_container);
        if (VIP_SUCCESS != status) {
            if (new_node_array != VIP_NULL) {
                vipdrv_os_free_memory((void*)new_node_array);
            }
            PRINTK_E("fail to alloc memory status=%d\n", status);
            vipOnError(status);
        }
        vipdrv_os_zero_memory(new_container, hashmap->size_per_element * (hashmap->capacity + HASH_MAP_CAPABILITY));
        vipdrv_os_copy_memory(new_container, hashmap->container,
                          hashmap->size_per_element * hashmap->capacity);
        vipdrv_os_free_memory(hashmap->container);
        hashmap->container = new_container;
    }

    vipdrv_os_copy_memory(new_node_array, hashmap->node_array,
                      sizeof(vipdrv_hashmap_node_t) * hashmap->capacity);
    vipdrv_os_free_memory(hashmap->node_array);

    hashmap->node_array = new_node_array;
    hashmap->capacity += HASH_MAP_CAPABILITY;
    for (i = 0; i < HASH_MAP_CAPABILITY; i++) {
        index = hashmap->capacity - 1 - i;
        hashmap->node_array[index].right_index = next_index;
        hashmap->node_array[index].left_index = 0;
        next_index = index;
        vipOnError(VIPDRV_SPINLOCK(create, &hashmap->node_array[index].spinlock));
    }
    hashmap->idle_list_head = &hashmap->node_array[0];
    hashmap->idle_list_head->right_index = next_index;
#if HASH_MAP_IDLE_LIST_TAIL
    hashmap->idle_list_tail = &hashmap->node_array[hashmap->capacity - 1];
#endif
    *expand = vip_true_e;
    PRINTK_D("hashmap %s: expand done\n", hashmap->name);

onError:
    vipdrv_unlock_write_mutex(&hashmap->expand_mutex);
    return status;
}

vip_status_e vipdrv_hashmap_destroy(
    IN vipdrv_hashmap_t *hashmap
    )
{
    vip_uint32_t i = 0;

    for (i = 1; i < hashmap->capacity; i++) {
        if (hashmap->callback_destroy && VIP_NULL != hashmap->container) {
            hashmap->callback_destroy((void*)((vip_uint8_t*)hashmap->container + i * hashmap->size_per_element));
        }
    }

    vipdrv_destroy_recursive_mutex(&hashmap->mutex);
    vipdrv_destroy_readwrite_mutex(&hashmap->expand_mutex);
    if (VIP_NULL != hashmap->node_array) {
        for (i = hashmap->capacity - 1; i > 0; i--) {
            if (VIP_NULL != hashmap->node_array[i].spinlock) {
                VIPDRV_SPINLOCK(destroy, hashmap->node_array[i].spinlock);
                hashmap->node_array[i].spinlock = VIP_NULL;
            }
        }

        vipdrv_os_free_memory(hashmap->node_array);
        hashmap->node_array = VIP_NULL;
    }
    if (VIP_NULL != hashmap->container) {
        vipdrv_os_free_memory(hashmap->container);
        hashmap->container = VIP_NULL;
    }
    hashmap->idle_list_head = VIP_NULL;
#if HASH_MAP_IDLE_LIST_TAIL
    hashmap->idle_list_tail = VIP_NULL;
#endif
    hashmap->capacity = 0;
    for (i = 0; i < HASH_MAP_CAPABILITY; i++) {
        hashmap->hashmap[i].right_index = 0;
    }
    hashmap->free_pos = 0;

    return VIP_SUCCESS;
}

vip_status_e vipdrv_hashmap_insert(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint64_t handle,
    OUT vip_uint32_t* index
    )
{
    vip_uint32_t cur = 0;
    vip_uint32_t h = 0;
    vip_uint32_t parent = 0;
    vip_uint32_t grand_p = 0;
    vip_uint32_t uncle = 0;
    vip_uint32_t *p = VIP_NULL;
    vipdrv_hashmap_node_t *header = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    status = vipdrv_lock_read_mutex(&hashmap->expand_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock expand mutex when hashmap insert\n");
        return status;
    }

    status = vipdrv_lock_recursive_mutex(&hashmap->mutex);
    if (status != VIP_SUCCESS) {
        vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
        PRINTK_E("fail to lock mutex when hashmap insert\n");
        return status;
    }

    handle += HANDLE_MAGIC_DATA;
    h = (HASH_MAP_CAPABILITY - 1) & hash(handle);
    header = &hashmap->hashmap[h];

    /* insert into tree */
    p = &header->right_index;
    while (0 != *p) {
        header = &hashmap->node_array[*p];
        parent = *p;
        if (handle > header->handle) {
            p = &header->right_index;
        }
        else if (handle < header->handle) {
            p = &header->left_index;
        }
        else {
            /* handle exist */
            *index = *p;
            if (1 == hashmap->node_array[*index].remove) {
                hashmap->node_array[*index].used = 1;
                hashmap->node_array[*index].remove = 0;
                vipGoOnError(VIP_SUCCESS);
            }
            else {
                PRINTK_D("HASHMAP %s INSERT: handle 0x%"PRPx" exist in hashmap\n", hashmap->name, handle);
                vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
            }
        }
    }

    /* use the first node in idle list */
    cur = hashmap->idle_list_head->right_index;
    /* no idle node */
    if (0 == cur) {
        *index = 0;
        PRINTK_D("HASHMAP %s INSERT: hashmap is full\n", hashmap->name);
        vipGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    hashmap->idle_list_head->right_index = hashmap->node_array[cur].right_index;
#if HASH_MAP_IDLE_LIST_TAIL
    if (0 == hashmap->idle_list_head->right_index) {
        hashmap->idle_list_tail = hashmap->idle_list_head;
    }
#endif
    hashmap->node_array[cur].parent_index = parent;
    hashmap->node_array[cur].right_index = 0;
    hashmap->node_array[cur].left_index = 0;
    hashmap->node_array[cur].handle = handle;
    hashmap->node_array[cur].used = 1;
    hashmap->node_array[cur].remove = 0;
    /* link parent node to current node */
    *p = cur;
    /* record ID of current node */
    *index = cur;
    PRINTK_D("hashmap %s: insert index=%d, handle=0x%"PRPx"\n", hashmap->name, *index, handle);
    /* reorder red black tree */
    while (0 != cur) {
        parent = hashmap->node_array[cur].parent_index;
        hashmap->node_array[cur].is_black = vip_false_e;
        if (0 == parent) {
            hashmap->node_array[cur].is_black = vip_true_e;
            break;
        }
        else if (vip_true_e == hashmap->node_array[parent].is_black) {
            break;
        }
        else {
            grand_p = hashmap->node_array[parent].parent_index;
            uncle = hashmap->node_array[grand_p].left_index == parent ?
                    hashmap->node_array[grand_p].right_index : hashmap->node_array[grand_p].left_index;
            if (0 != uncle && vip_false_e == hashmap->node_array[uncle].is_black) {
                hashmap->node_array[uncle].is_black = vip_true_e;
                hashmap->node_array[parent].is_black = vip_true_e;
                hashmap->node_array[grand_p].is_black = vip_false_e;
                cur = grand_p;
            }
            else if (parent == hashmap->node_array[grand_p].left_index) {
                if (cur == hashmap->node_array[parent].left_index) {
                    hashmap->node_array[parent].is_black = vip_true_e;
                    hashmap->node_array[grand_p].is_black = vip_false_e;
                    right_rotate(hashmap, grand_p, &hashmap->hashmap[h]);
                }
                else {
                    left_rotate(hashmap, parent, &hashmap->hashmap[h]);
                    cur = parent;
                }
            }
            else {
                if (cur == hashmap->node_array[parent].right_index) {
                    hashmap->node_array[parent].is_black = vip_true_e;
                    hashmap->node_array[grand_p].is_black = vip_false_e;
                    left_rotate(hashmap, grand_p, &hashmap->hashmap[h]);
                }
                else {
                    right_rotate(hashmap, parent, &hashmap->hashmap[h]);
                    cur = parent;
                }
            }
        }
    }

    if (*index >= hashmap->free_pos) {
        hashmap->free_pos = *index + 1;
    }

onError:
    vipdrv_unlock_recursive_mutex(&hashmap->mutex);
    vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
    return status;
}

static vip_status_e hashmap_remove(
    vipdrv_hashmap_t* hashmap,
    vip_uint32_t index
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t* del = VIP_NULL;
    vip_uint32_t h = (HASH_MAP_CAPABILITY - 1) & hash(VIPDRV_PTR_TO_UINT64(hashmap->node_array[index].handle));
    /* R: replace node; P: parent; S: brother; SL: brother's left node; SR: brother's right node */
    vip_uint32_t R, P, S, SL, SR;
    /* find a replace node */
    vip_uint32_t replace = 0;

    status = vipdrv_lock_recursive_mutex(&hashmap->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock mutex when hashmap remove\n");
        return status;
    }

    if (0 == hashmap->node_array[index].remove) {
        vipdrv_unlock_recursive_mutex(&hashmap->mutex);
        return status;
    }

    PRINTK_D("hashmap %s: remove index=%d, handle=0x%"PRPx"\n",
        hashmap->name, index, hashmap->node_array[index].handle);

    if (hashmap->callback_remove && VIP_NULL != hashmap->container) {
        hashmap->callback_remove((void*)((vip_uint8_t*)hashmap->container + index * hashmap->size_per_element));
    }

    while (0 != hashmap->node_array[index].left_index ||
        0 != hashmap->node_array[index].right_index) {
        if (0 == hashmap->node_array[index].left_index) {
            replace = hashmap->node_array[index].right_index;
        }
        else if (0 == hashmap->node_array[index].right_index) {
            replace = hashmap->node_array[index].left_index;
        }
        else {
            replace = hashmap->node_array[index].right_index;
            while (0 != hashmap->node_array[replace].left_index) {
                replace = hashmap->node_array[replace].left_index;
            }
        }
        /* swap del node and replace node, then del replace node */
        swap_hashmap_node(hashmap, &hashmap->hashmap[h], replace, index);
    }

    R = index;
    /* reorder red black tree */
    while (vip_true_e == hashmap->node_array[R].is_black && R != hashmap->hashmap[h].right_index) {
        vip_bool_e left_node;
        P = hashmap->node_array[R].parent_index;
        left_node = hashmap->node_array[P].left_index == R ? vip_true_e : vip_false_e;
        S = vip_true_e == left_node ? hashmap->node_array[P].right_index
            : hashmap->node_array[P].left_index;
        if (vip_true_e == left_node) {
            if (vip_false_e == hashmap->node_array[S].is_black) {
                hashmap->node_array[S].is_black = vip_true_e;
                hashmap->node_array[P].is_black = vip_false_e;
                left_rotate(hashmap, P, &hashmap->hashmap[h]);
            }
            else {
                SL = hashmap->node_array[S].left_index;
                SR = hashmap->node_array[S].right_index;
                if (0 != SR && vip_false_e == hashmap->node_array[SR].is_black) {
                    hashmap->node_array[S].is_black = hashmap->node_array[P].is_black;
                    hashmap->node_array[P].is_black = vip_true_e;
                    hashmap->node_array[SR].is_black = vip_true_e;
                    left_rotate(hashmap, P, &hashmap->hashmap[h]);
                    break;
                }
                else if (0 != SL && vip_false_e == hashmap->node_array[SL].is_black) {
                    hashmap->node_array[S].is_black = vip_false_e;
                    hashmap->node_array[SL].is_black = vip_true_e;
                    right_rotate(hashmap, S, &hashmap->hashmap[h]);
                }
                else {
                    hashmap->node_array[S].is_black = vip_false_e;
                    R = P;
                }
            }
        }
        else {
            if (vip_false_e == hashmap->node_array[S].is_black) {
                hashmap->node_array[S].is_black = vip_true_e;
                hashmap->node_array[P].is_black = vip_false_e;
                right_rotate(hashmap, P, &hashmap->hashmap[h]);
            }
            else {
                SL = hashmap->node_array[S].left_index;
                SR = hashmap->node_array[S].right_index;
                if (0 != SL && vip_false_e == hashmap->node_array[SL].is_black) {
                    hashmap->node_array[S].is_black = hashmap->node_array[P].is_black;
                    hashmap->node_array[P].is_black = vip_true_e;
                    hashmap->node_array[SL].is_black = vip_true_e;
                    right_rotate(hashmap, P, &hashmap->hashmap[h]);
                    break;
                }
                else if (0 != SR && vip_false_e == hashmap->node_array[SR].is_black) {
                    hashmap->node_array[S].is_black = vip_false_e;
                    hashmap->node_array[SR].is_black = vip_true_e;
                    left_rotate(hashmap, S, &hashmap->hashmap[h]);
                }
                else {
                    hashmap->node_array[S].is_black = vip_false_e;
                    R = P;
                }
            }
        }
    }

    hashmap->node_array[R].is_black = vip_true_e;
    /* insert this node into idle list */
    hashmap->node_array[index].handle = VIP_NULL;
#if HASH_MAP_IDLE_LIST_TAIL
    hashmap->idle_list_tail->right_index = index;
    hashmap->idle_list_tail = &hashmap->node_array[index];
    hashmap->idle_list_tail->right_index = 0;
#else
    hashmap->node_array[index].right_index = hashmap->idle_list_head->right_index;
    hashmap->idle_list_head->right_index = index;
#endif
    /* unlink parent node to this node */
    if (0 == hashmap->node_array[index].parent_index) {
        del = &hashmap->hashmap[h].right_index;
    }
    else {
        del = (index == hashmap->node_array[hashmap->node_array[index].parent_index].left_index) ?
            &hashmap->node_array[hashmap->node_array[index].parent_index].left_index :
            &hashmap->node_array[hashmap->node_array[index].parent_index].right_index;
    }
    *del = 0;
    while ((hashmap->free_pos > 1) && (VIP_NULL == hashmap->node_array[hashmap->free_pos - 1].handle)) {
        hashmap->free_pos--;
    }
    hashmap->node_array[index].remove = 0;

    vipdrv_unlock_recursive_mutex(&hashmap->mutex);
    return status;
}

vip_status_e vipdrv_hashmap_remove(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint64_t handle,
    IN vip_bool_e force
    )
{
    vip_uint32_t h = 0;
    vip_uint32_t *del = VIP_NULL;
    vip_uint32_t index = 0;
    vipdrv_hashmap_node_t *header = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    handle += HANDLE_MAGIC_DATA;
    status = vipdrv_lock_read_mutex(&hashmap->expand_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock expand mutex when hashmap remove by handle\n");
        return status;
    }

    status = vipdrv_lock_recursive_mutex(&hashmap->mutex);
    if (status != VIP_SUCCESS) {
        vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
        PRINTK_E("fail to lock mutex when hashmap remove by handle\n");
        return status;
    }

    h = (HASH_MAP_CAPABILITY - 1) & hash(handle);
    header = &hashmap->hashmap[h];

    del = &header->right_index;
    while (0 != *del) {
        header = &hashmap->node_array[*del];
        if (handle == header->handle) {
            break;
        }
        else if (handle > header->handle) {
            del = &header->right_index;
        }
        else {
            del = &header->left_index;
        }
    }
    index = *del;
    vipdrv_unlock_recursive_mutex(&hashmap->mutex);

    if (0 != index) {
        status = VIPDRV_SPINLOCK(lock, hashmap->node_array[index].spinlock);
        if (status != VIP_SUCCESS) {
            vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
            PRINTK_E("fail to lock spinlock %d when hashmap remove\n", index);
            return status;
        }
        hashmap->node_array[index].remove = 1;
        if (0 == hashmap->node_array[index].ref_count ||
            vip_true_e == force) {
            hashmap->node_array[index].used = 0;
            hashmap->node_array[index].ref_count = 0;
            VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);
            status = hashmap_remove(hashmap, index);
        }
        else {
            VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);
        }
    }
    else {
        PRINTK_D("HASHMAP %s REMOVE: handle 0x%"PRPx" not exist in hashmap\n", hashmap->name, handle);
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }
    vipdrv_unlock_read_mutex(&hashmap->expand_mutex);

    return status;
}

/*
@brief  remove handle from hashmap.
@param  hashmap, the hashmap.
@param  index, the index of element in container.
*/
vip_status_e vipdrv_hashmap_remove_by_index(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint32_t index,
    IN vip_bool_e force
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (0 < index && index < hashmap->capacity) {
        status = vipdrv_lock_read_mutex(&hashmap->expand_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to lock expand mutex when hashmap remove by index\n");
            return status;
        }

        status = VIPDRV_SPINLOCK(lock, hashmap->node_array[index].spinlock);
        if (status != VIP_SUCCESS) {
            vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
            PRINTK_E("fail to lock spinlock %d when hashmap remove by index\n", index);
            return status;
        }
        if (1 == hashmap->node_array[index].used) {
            hashmap->node_array[index].remove = 1;
            if (0 == hashmap->node_array[index].ref_count ||
                vip_true_e == force) {
                vip_uint16_t ref_count = hashmap->node_array[index].ref_count;
                hashmap->node_array[index].used = 0;
                hashmap->node_array[index].ref_count = 0;
                VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);
                while (ref_count) {
                    ref_count -= 1;
                    vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
                }
                status = hashmap_remove(hashmap, index);
            }
            else {
                VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);
            }
        }
        else {
            VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);
            PRINTK_D("HASHMAP %s REMOVE BY INDEX: index %d not use\n", hashmap->name, index);
            status = VIP_ERROR_INVALID_ARGUMENTS;
        }
        vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
    }
    else {
        PRINTK_D("HASHMAP %s REMOVE BY INDEX: index %d out of range\n", hashmap->name, index);
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }

    return status;
}

vip_status_e vipdrv_hashmap_get_by_handle(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint64_t handle,
    OUT vip_uint32_t* index,
    OUT void** element
    )
{
    vip_uint32_t h = 0;
    vip_uint32_t *get = VIP_NULL;
    vipdrv_hashmap_node_t *header = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    handle += HANDLE_MAGIC_DATA;
    status = vipdrv_lock_read_mutex(&hashmap->expand_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock expand mutex when hashmap get\n");
        return status;
    }

    status = vipdrv_lock_recursive_mutex(&hashmap->mutex);
    if (status != VIP_SUCCESS) {
        vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
        PRINTK_E("fail to lock mutex when hashmap get\n");
        return status;
    }

    h = (HASH_MAP_CAPABILITY - 1) & hash(handle);
    header = &hashmap->hashmap[h];

    get = &header->right_index;
    while (0 != *get) {
        header = &hashmap->node_array[*get];
        if (handle == header->handle) {
            break;
        }
        else if (handle > header->handle) {
            get = &header->right_index;
        }
        else {
            get = &header->left_index;
        }
    }
    *index = *get;
    vipdrv_unlock_recursive_mutex(&hashmap->mutex);

    if (0 != *index) {
        status = VIPDRV_SPINLOCK(lock, hashmap->node_array[*index].spinlock);
        if (status != VIP_SUCCESS) {
            vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
            PRINTK_E("fail to lock spinlock %d when hashmap get by handle\n", *index);
            return status;
        }
        hashmap->node_array[*index].ref_count += 1;
        VIPDRV_SPINLOCK(unlock, hashmap->node_array[*index].spinlock);
        if (VIP_NULL != element) {
            if (0 == hashmap->size_per_element) {
                *element = VIPDRV_UINT64_TO_PTR(hashmap->node_array[*index].handle);
            }
            else {
                *element = (void*)((vip_uint8_t*)hashmap->container + *index * hashmap->size_per_element);
            }
        }
    }
    else {
        vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
        PRINTK_D("HASHMAP %s GET BY HANDLE: handle 0x%"PRPx" not exist in hashmap\n", hashmap->name, handle);
        if (VIP_NULL != element) {
            *element = VIP_NULL;
        }
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }

    return status;
}

vip_status_e vipdrv_hashmap_get_by_index(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint32_t index,
    OUT void** element
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (0 < index && index < hashmap->capacity) {
        status = vipdrv_lock_read_mutex(&hashmap->expand_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to lock expand mutex when hashmap get by index\n");
            return status;
        }

        status = VIPDRV_SPINLOCK(lock, hashmap->node_array[index].spinlock);
        if (status != VIP_SUCCESS) {
            vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
            PRINTK_E("fail to lock spinlock %d when hashmap get by index\n", index);
            return status;
        }
        if (1 == hashmap->node_array[index].used) {
            hashmap->node_array[index].ref_count += 1;
            VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);
            if (VIP_NULL != element) {
                if (0 == hashmap->size_per_element) {
                    *element = VIPDRV_UINT64_TO_PTR(hashmap->node_array[index].handle);
                }
                else {
                    *element = (void*)((vip_uint8_t*)hashmap->container + index * hashmap->size_per_element);
                }
            }
        }
        else {
            VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);
            vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
            PRINTK_D("HASHMAP %s GET BY INDEX: index %d not use\n", hashmap->name, index);
            if (VIP_NULL != element) {
                *element = VIP_NULL;
            }
            status = VIP_ERROR_INVALID_ARGUMENTS;
        }
    }
    else {
        PRINTK_D("HASHMAP %s GET BY INDEX: index %d is invalid\n", hashmap->name, index);
        if (VIP_NULL != element) {
            *element = VIP_NULL;
        }
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }

    return status;
}

vip_status_e vipdrv_hashmap_unuse(
    IN vipdrv_hashmap_t *hashmap,
    IN vip_uint32_t index,
    IN vip_bool_e callback
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (0 < index && index < hashmap->capacity) {
        status = VIPDRV_SPINLOCK(lock, hashmap->node_array[index].spinlock);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to lock spinlock %d when hashmap unuse\n", index);
            return status;
        }

        if (1 == hashmap->node_array[index].used) {
            hashmap->node_array[index].ref_count -= 1;
            if (0 == hashmap->node_array[index].ref_count && hashmap->node_array[index].remove) {
                hashmap->node_array[index].used = 0;
                VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);
                status = hashmap_remove(hashmap, index);
            }
            else {
                VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);

            }
            if (callback && hashmap->callback_unuse) {
                hashmap->callback_unuse((void*)((vip_uint8_t*)hashmap->container + index * hashmap->size_per_element));
            }
            vipdrv_unlock_read_mutex(&hashmap->expand_mutex);
        }
        else {
            VIPDRV_SPINLOCK(unlock, hashmap->node_array[index].spinlock);
            PRINTK_D("HASHMAP %s UNUSE: index %d not use\n", hashmap->name, index);
            status = VIP_ERROR_INVALID_ARGUMENTS;
        }
    }
    else {
        PRINTK_D("HASHMAP %s UNUSE: index %d out of range\n", hashmap->name, index);
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }
    return status;
}

vip_uint32_t vipdrv_hashmap_free_pos(
    vipdrv_hashmap_t *hashmap
    )
{
    return hashmap->free_pos;
}

vip_bool_e vipdrv_hashmap_full(
    vipdrv_hashmap_t *hashmap
    )
{
    return (0 == hashmap->idle_list_head->right_index) ? vip_true_e : vip_false_e;
}

vip_bool_e vipdrv_hashmap_check_remove(
    vipdrv_hashmap_t* hashmap,
    vip_uint32_t index
    )
{
    return (1 == hashmap->node_array[index].remove) ? vip_true_e : vip_false_e;
}

/*
@brief  init database.
@param  database, the database.
@param  init_capacity, the initial capacity of database.
@param  size_per_element, the size of each element in container.
@param  name, name of this database.
*/
vip_status_e vipdrv_database_init(
    IN vipdrv_database_t* database,
    IN vip_uint32_t init_capacity,
    IN vip_uint32_t size_per_element,
    IN vip_char_t_ptr name,
    IN vip_status_e(*callback_remove)(void* element)
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t i = 0;
    vip_uint32_t next_index = 0;

    if (0 == size_per_element) {
        PRINTK_E("size of element is 0\n");
        return VIP_ERROR_FAILURE;
    }

    database->capacity = init_capacity + 1; /* index 0: idle list header */
    database->size_per_element = size_per_element;
    database->name = name;

    vipOnError(vipdrv_create_recursive_mutex(&database->mutex));
    vipOnError(vipdrv_create_readwrite_mutex(&database->expand_mutex));
    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_database_node_t) * database->capacity,
                                      (void**)&database->node_array));
    vipdrv_os_zero_memory(database->node_array, sizeof(vipdrv_database_node_t) * database->capacity);

    vipOnError(vipdrv_os_allocate_memory(database->size_per_element * database->capacity,
                                      (void**)&database->container));
    vipdrv_os_zero_memory(database->container, database->size_per_element * database->capacity);

    for (i = database->capacity - 1; i > 0; i--) {
        database->node_array[i].next_index = next_index;
        next_index = i;
        vipOnError(VIPDRV_SPINLOCK(create, &database->node_array[i].spinlock));
    }
    database->idle_list_head = &database->node_array[0];
    database->idle_list_head->next_index = next_index;
    database->free_pos = 1; /* index 0: idle list header */
    database->callback_remove = callback_remove;
    /*PRINTK("DATABASE 0x%"PRPx"(%s) INIT SUCCESS\n", database, name);*/
    return status;

onError:
    vipdrv_destroy_recursive_mutex(&database->mutex);
    vipdrv_destroy_readwrite_mutex(&database->expand_mutex);
    if (VIP_NULL != database->node_array) {
        for (i = database->capacity - 1; i > 0; i--) {
            if (VIP_NULL != database->node_array[i].spinlock) {
                VIPDRV_SPINLOCK(destroy, database->node_array[i].spinlock);
                database->node_array[i].spinlock = VIP_NULL;
            }
        }

        vipdrv_os_free_memory(database->node_array);
        database->node_array = VIP_NULL;
    }
    if (VIP_NULL != database->container) {
        vipdrv_os_free_memory(database->container);
        database->container = VIP_NULL;
    }
    return status;
}

/*
@brief  expand database.
@param  database, the database.
@param  expand, expand really or not.
*/
vip_status_e vipdrv_database_expand(
    IN vipdrv_database_t* database,
    OUT vip_bool_e *expand
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t next_index = 0;
    vip_uint32_t index = 0;
    vip_status_e status = VIP_SUCCESS;
    vipdrv_database_node_t *new_node_array = VIP_NULL;
    void *new_container;
    *expand = vip_false_e;

    status = vipdrv_lock_write_mutex(&database->expand_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock expand mutex when database expand\n");
        return status;
    }
    if (vip_false_e == vipdrv_database_full(database)) {
        PRINTK_D("no need to expand database\n");
        vipGoOnError(status);
    }
    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_database_node_t) *
                                       (database->capacity + DATABASE_CAPABILITY),
                                       (void**)&new_node_array));
    vipdrv_os_zero_memory(new_node_array,
        sizeof(vipdrv_database_node_t) * (database->capacity + DATABASE_CAPABILITY));

    status = vipdrv_os_allocate_memory(database->size_per_element *
                                      (database->capacity + DATABASE_CAPABILITY),
                                      (void**)&new_container);
    if (VIP_SUCCESS != status) {
        if (new_node_array != VIP_NULL) {
            vipdrv_os_free_memory((void*)new_node_array);
        }
        PRINTK_E("fail to alloc memory status=%d\n", status);
        vipOnError(status);
    }
    vipdrv_os_zero_memory(new_container, database->size_per_element * (database->capacity + DATABASE_CAPABILITY));
    vipdrv_os_copy_memory(new_container, database->container,
        database->size_per_element * database->capacity);
    vipdrv_os_free_memory(database->container);
    database->container = new_container;

    vipdrv_os_copy_memory(new_node_array, database->node_array,
                      sizeof(vipdrv_database_node_t) * database->capacity);
    vipdrv_os_free_memory(database->node_array);

    database->node_array = new_node_array;
    database->capacity += DATABASE_CAPABILITY;
    for (i = 0; i < DATABASE_CAPABILITY; i++) {
        index = database->capacity - 1 - i;
        database->node_array[index].next_index = next_index;
        next_index = index;
        vipOnError(VIPDRV_SPINLOCK(create, &database->node_array[index].spinlock));
    }
    database->idle_list_head = &database->node_array[0];
    database->idle_list_head->next_index = next_index;
    *expand = vip_true_e;
    PRINTK_D("database %s: expand done\n", database->name);

onError:
    vipdrv_unlock_write_mutex(&database->expand_mutex);
    return status;
}

/*
@brief  destroy database.
@param  database, the database.
*/
vip_status_e vipdrv_database_destroy(
    IN vipdrv_database_t* database
    )
{
    vip_uint32_t i = 0;

    vipdrv_destroy_recursive_mutex(&database->mutex);
    vipdrv_destroy_readwrite_mutex(&database->expand_mutex);
    if (VIP_NULL != database->node_array) {
        for (i = database->capacity - 1; i > 0; i--) {
            if (VIP_NULL != database->node_array[i].spinlock) {
                VIPDRV_SPINLOCK(destroy, database->node_array[i].spinlock);
                database->node_array[i].spinlock = VIP_NULL;
            }
        }

        vipdrv_os_free_memory(database->node_array);
        database->node_array = VIP_NULL;
    }
    if (VIP_NULL != database->container) {
        vipdrv_os_free_memory(database->container);
        database->container = VIP_NULL;
    }
    database->idle_list_head = VIP_NULL;
    database->capacity = 0;
    database->free_pos = 0;

    return VIP_SUCCESS;
}

/*
@brief  get empty index of database.
@param  database, the database.
@param  index, the index of element in container.
*/
vip_status_e vipdrv_database_insert(
    IN vipdrv_database_t* database,
    OUT vip_uint32_t* index
    )
{
    vip_uint32_t cur = 0;
    vip_status_e status = VIP_SUCCESS;

    status = vipdrv_lock_read_mutex(&database->expand_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock expand mutex when database insert\n");
        return status;
    }

    status = vipdrv_lock_recursive_mutex(&database->mutex);
    if (status != VIP_SUCCESS) {
        vipdrv_unlock_read_mutex(&database->expand_mutex);
        PRINTK_E("fail to lock mutex when database insert\n");
        return status;
    }

    /* use the first node in idle list */
    cur = database->idle_list_head->next_index;
    /* no idle node */
    if (0 == cur) {
        *index = 0;
        PRINTK_D("DATABASE %s INSERT: database is full\n", database->name);
        vipGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    database->idle_list_head->next_index = database->node_array[cur].next_index;
    database->node_array[cur].next_index = 0;
    database->node_array[cur].used = 1;
    database->node_array[cur].remove = 0;
    /* record ID of current node */
    *index = cur;
    PRINTK_D("database %s: insert index=%d\n", database->name, *index);

    if (*index >= database->free_pos) {
        database->free_pos = *index + 1;
    }

onError:
    vipdrv_unlock_recursive_mutex(&database->mutex);
    vipdrv_unlock_read_mutex(&database->expand_mutex);
    return status;
}

static vip_status_e database_remove(
    vipdrv_database_t* database,
    vip_uint32_t index
    )
{
    vip_status_e status = VIP_SUCCESS;
    status = vipdrv_lock_recursive_mutex(&database->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock mutex when database remove\n");
        return status;
    }

    if (0 == database->node_array[index].remove) {
        vipdrv_unlock_recursive_mutex(&database->mutex);
        return status;
    }

    PRINTK_D("database %s: remove index=%d\n", database->name, index);

    if (database->callback_remove) {
        database->callback_remove((void*)((vip_uint8_t*)database->container + index * database->size_per_element));
    }

    /* insert this node into idle list */
    database->node_array[index].next_index = database->idle_list_head->next_index;
    database->idle_list_head->next_index = index;
    while ((database->free_pos > 1) && (0 == database->node_array[database->free_pos - 1].used)) {
        database->free_pos--;
    }
    database->node_array[index].remove = 0;

    vipdrv_unlock_recursive_mutex(&database->mutex);
    return status;
}

/*
@brief  remove from database.
@param  database, the database.
@param  index, the index of element in container.
*/
vip_status_e vipdrv_database_remove(
    IN vipdrv_database_t* database,
    IN vip_uint32_t index,
    IN vip_bool_e force
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (0 < index && index < database->capacity) {
        status = vipdrv_lock_read_mutex(&database->expand_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to lock expand mutex when database remove\n");
            return status;
        }

        status = VIPDRV_SPINLOCK(lock, database->node_array[index].spinlock);
        if (status != VIP_SUCCESS) {
            vipdrv_unlock_read_mutex(&database->expand_mutex);
            PRINTK_E("fail to lock spinlock %d when database remove\n", index);
            return status;
        }
        if (1 == database->node_array[index].used) {
            database->node_array[index].remove = 1;
            if (0 == database->node_array[index].ref_count || vip_true_e == force) {
                vip_uint16_t ref_count = database->node_array[index].ref_count;
                database->node_array[index].used = 0;
                database->node_array[index].ref_count = 0;
                VIPDRV_SPINLOCK(unlock, database->node_array[index].spinlock);
                while (ref_count) {
                    ref_count -= 1;
                    vipdrv_unlock_read_mutex(&database->expand_mutex);
                }
                status = database_remove(database, index);
            }
            else {
                VIPDRV_SPINLOCK(unlock, database->node_array[index].spinlock);
            }
        }
        else {
            VIPDRV_SPINLOCK(unlock, database->node_array[index].spinlock);
            PRINTK_D("DATABASE %s REMOVE BY INDEX: index %d not use\n", database->name, index);
            status = VIP_ERROR_INVALID_ARGUMENTS;
        }
        vipdrv_unlock_read_mutex(&database->expand_mutex);
    }
    else {
        PRINTK_D("DATABASE %s REMOVE BY INDEX: index %d is invalid\n", database->name, index);
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }
    return status;
}

/*
@brief  get from database by index.
@param  database, the database.
@param  index, the index of element in container.
@param  element, the pointer of element.
*/
vip_status_e vipdrv_database_use(
    IN vipdrv_database_t* database,
    IN vip_uint32_t index,
    OUT void** element
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (0 < index && index < database->capacity) {
        status = vipdrv_lock_read_mutex(&database->expand_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to lock expand mutex when database use\n");
            return status;
        }

        status = VIPDRV_SPINLOCK(lock, database->node_array[index].spinlock);
        if (status != VIP_SUCCESS) {
            vipdrv_unlock_read_mutex(&database->expand_mutex);
            PRINTK_E("fail to lock spinlock %d when database use\n", index);
            return status;
        }
        if (1 == database->node_array[index].used) {
            database->node_array[index].ref_count += 1;
            VIPDRV_SPINLOCK(unlock, database->node_array[index].spinlock);
            if (VIP_NULL != element) {
                *element = (void*)((vip_uint8_t*)database->container + index * database->size_per_element);
            }
        }
        else {
            VIPDRV_SPINLOCK(unlock, database->node_array[index].spinlock);
            vipdrv_unlock_read_mutex(&database->expand_mutex);
            PRINTK_D("DATABASE %s GET BY INDEX: index %d not use\n", database->name, index);
            if (VIP_NULL != element) {
                *element = VIP_NULL;
            }
            status = VIP_ERROR_INVALID_ARGUMENTS;
        }
    }
    else {
        PRINTK_D("DATABASE %s GET BY INDEX: index %d is invalid\n", database->name, index);
        if (VIP_NULL != element) {
            *element = VIP_NULL;
        }
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }
    return status;
}

/*
@brief  unuse the element in container.
@param  database, the database.
@param  index, the index of element in container.
*/
vip_status_e vipdrv_database_unuse(
    IN vipdrv_database_t* database,
    IN vip_uint32_t index
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (0 < index && index < database->capacity) {
        status = VIPDRV_SPINLOCK(lock, database->node_array[index].spinlock);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to lock spinlock %d when database unuse\n", index);
            return status;
        }
        if (1 == database->node_array[index].used) {
            database->node_array[index].ref_count -= 1;
            if (0 == database->node_array[index].ref_count && database->node_array[index].remove) {
                database->node_array[index].used = 0;
                VIPDRV_SPINLOCK(unlock, database->node_array[index].spinlock);
                status = database_remove(database, index);
            }
            else {
                VIPDRV_SPINLOCK(unlock, database->node_array[index].spinlock);
            }
            vipdrv_unlock_read_mutex(&database->expand_mutex);
        }
        else {
            VIPDRV_SPINLOCK(unlock, database->node_array[index].spinlock);
            PRINTK_D("DATABASE %s UNUSE: index %d not use\n", database->name, index);
            status = VIP_ERROR_INVALID_ARGUMENTS;
        }
    }
    else {
        PRINTK_D("DATABASE %s UNUSE: index %d is invalid\n", database->name, index);
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }
    return status;
}

vip_uint32_t vipdrv_database_free_pos(
    vipdrv_database_t* database
    )
{
    return database->free_pos;
}

vip_bool_e vipdrv_database_full(
    vipdrv_database_t* database
    )
{
    return (0 == database->idle_list_head->next_index) ? vip_true_e : vip_false_e;
}

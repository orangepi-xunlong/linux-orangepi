/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2023 Vivante Corporation
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
*    Copyright (C) 2017 - 2023 Vivante Corporation
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

#include <gc_vip_common.h>
#include <vip_lite.h>
#include <gc_vip_kernel.h>
#include <gc_vip_kernel_port.h>
#include <gc_vip_kernel_drv.h>

vip_status_e gckvip_lock_recursive_mutex(
    gckvip_recursive_mutex *recursive_mutex
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t cur_tid = gckvip_os_get_tid();
    if (cur_tid == recursive_mutex->current_tid) {
        recursive_mutex->ref_count += 1;
    }
    else {
        gcOnError(gckvip_os_lock_mutex(recursive_mutex->mutex));
        recursive_mutex->current_tid = cur_tid;
        recursive_mutex->ref_count += 1;
    }

onError:
    return status;
}

vip_status_e gckvip_unlock_recursive_mutex(
    gckvip_recursive_mutex *recursive_mutex
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t cur_tid = gckvip_os_get_tid();
    if (cur_tid == recursive_mutex->current_tid) {
        recursive_mutex->ref_count -= 1;
    }
    else {
        PRINTK_E("thread %d call unlock before getting the mutex\n", cur_tid);
    }

    if (0 >= recursive_mutex->ref_count) {
        recursive_mutex->current_tid = 0;
        recursive_mutex->ref_count = 0;
        gcOnError(gckvip_os_unlock_mutex(recursive_mutex->mutex));
    }

onError:
    return status;
}

#if vpmdENABLE_MULTIPLE_TASK
#if vpmdENABLE_PREEMPTION
static vip_bool_e cmp_node(
    gckvip_queue_node i,
    gckvip_queue_node j
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
    gckvip_priority_queue *queue,
    vip_uint32_t i,
    vip_uint32_t j
    )
{
    gckvip_queue_node temp;
    temp = queue->head[i];
    queue->head[i] = queue->head[j];
    queue->head[j] = temp;

    return VIP_SUCCESS;
}
static vip_status_e ensure_capacity(
    gckvip_priority_queue *queue
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_queue_node*  new_head = VIP_NULL;

    if (queue->cur_size < queue->max_size) {
        return status;
    }

    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_queue_node) * queue->max_size * 2, (void**)&new_head));
    gckvip_os_zero_memory(new_head, sizeof(gckvip_queue_node) * queue->max_size * 2);
    gckvip_os_memcopy(new_head, queue->head, sizeof(gckvip_queue_node) * queue->max_size);

    gckvip_os_free_memory(queue->head);
    queue->head = new_head;
    queue->max_size *= 2;

onError:
    return status;
}
static vip_status_e queue_initialize(
    gckvip_priority_queue *queue
    )
{
    vip_status_e status = VIP_SUCCESS;
    /* queue is empty */
    queue->cur_size = 0;
    queue->count = 0;
    queue->max_size = QUEUE_CAPABILITY;

    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_queue_node) * queue->max_size, (void**)&queue->head));
    gckvip_os_zero_memory(queue->head, sizeof(gckvip_queue_node) * queue->max_size);

onError:
    return status;
}

static vip_status_e queue_destroy(
    gckvip_priority_queue *queue
    )
{
    gckvip_os_free_memory(queue->head);
    return VIP_SUCCESS;
}

static void keep_max_heap(
    gckvip_priority_queue *queue,
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

        /* priority of current node is lower than parent node */
        if (cmp_node(queue->head[parent], queue->head[index])) break;

        swap_node(queue, index, parent);
        index = parent;
    }
    index = priority;
    while (index * 2 + 1 < queue->cur_size) { /* not leaf node */
        left = index * 2 + 1;
        right = index * 2 + 2;

        if (left < queue->cur_size && cmp_node(queue->head[left], queue->head[priority])) priority = left;
        if (right < queue->cur_size && cmp_node(queue->head[right], queue->head[priority])) priority = right;

        /* priority of current node is higher than left/right node */
        if (priority == index) break;

        swap_node(queue, index, priority);
        index = priority;
    }
}

static vip_bool_e queue_read(
    gckvip_priority_queue *queue,
    gckvip_queue_data_t **data_ptr
    )
{
    /* Read data if queue is empty */
    if (0 == queue->cur_size) {
        return vip_false_e;
    }

    *data_ptr = queue->head[0].data;
    /* swap the first and the last node */
    swap_node(queue, 0, queue->cur_size - 1);
    queue->cur_size--;
    keep_max_heap(queue, 0);

    return vip_true_e;
}

static vip_bool_e queue_write(
    gckvip_priority_queue *queue,
    gckvip_queue_data_t *data
    )
{
    vip_uint32_t index = queue->cur_size;
    /* Make sure there is space when write data */
    if (VIP_SUCCESS != ensure_capacity(queue)) {
        return vip_false_e;
    }

    queue->head[index].data = data;
    queue->head[index].index = queue->count;
    queue->cur_size++;
    queue->count++;
    keep_max_heap(queue, index);
    if ((vip_uint32_t)-1 == queue->count) {
        for (index = 0; index < queue->cur_size; index++) {
            queue->head[index].index = 0;
        }
    }

    return vip_true_e;
}

static vip_status_e queue_clean(
    gckvip_priority_queue *queue,
    void* param
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == param) {
        /* clean all */
        queue->cur_size = 0;
        queue->count = 0;
    }
    else {
        /* clean one task according to index */
        vip_uint32_t index = *(vip_uint32_t*)param;
        vip_uint32_t i = 0;
        status = VIP_ERROR_INVALID_ARGUMENTS;
        for (i = 0; i < queue->cur_size; i++) {
            if (index == queue->head[i].data->v1) {
                swap_node(queue, i, queue->cur_size - 1);
                queue->cur_size--;
                keep_max_heap(queue, i);

                status = VIP_SUCCESS;
                break;
            }
        }
    }

    return status;
}

static gckvip_queue_status_e queue_status(
    gckvip_priority_queue *queue
    )
{
    gckvip_queue_status_e status;
    if (0 == queue->cur_size) {
        status = GCKVIP_QUEUE_EMPTY;
    }
    else if (queue->max_size <= queue->cur_size) {
        if (VIP_SUCCESS == ensure_capacity(queue)) {
            status=  GCKVIP_QUEUE_WITH_DATA;
        }
        else {
            status = GCKVIP_QUEUE_FULL;
        }
    }
    else {
        status=  GCKVIP_QUEUE_WITH_DATA;
    }

    return status;
}
#else
static vip_status_e queue_initialize(
    gckvip_fifo_queue *queue
    )
{
    /* queue is empty */
    queue->end_index = -1;
    queue->begin_index = 0;

    return VIP_SUCCESS;
}

static vip_status_e queue_destroy(
    gckvip_fifo_queue *queue
    )
{
    return VIP_SUCCESS;
}

static vip_bool_e queue_read(
    gckvip_fifo_queue *queue,
    gckvip_queue_data_t **data_ptr
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
    gckvip_fifo_queue *queue,
    gckvip_queue_data_t *data
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
    gckvip_fifo_queue *queue,
    void* param
    )
{
    queue->end_index = -1;
    return VIP_SUCCESS;
}
static gckvip_queue_status_e queue_status(
    gckvip_fifo_queue *queue
    )
{
    gckvip_queue_status_e status;
    if (queue->begin_index == queue->end_index) {
        status = GCKVIP_QUEUE_FULL;
    }
    else if (queue->end_index == -1) {
        status = GCKVIP_QUEUE_EMPTY;
    }
    else {
        status=  GCKVIP_QUEUE_WITH_DATA;
    }

    return status;
}
#endif
vip_status_e gckvip_queue_initialize(
    gckvip_queue_t *queue,
    vip_uint32_t device_id
    )
{
    vip_status_e status = VIP_SUCCESS;

    status = gckvip_os_create_mutex(&queue->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create mutex\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_create_mutex(&queue->read_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create read mutex\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_create_mutex(&queue->write_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create write mutex\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    status = gckvip_os_create_signal(&queue->read_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create read_signal\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    PRINTK_D("queue create read signal=0x%"PRPx"\n", queue->read_signal);
    status = gckvip_os_create_signal(&queue->write_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create write_signal\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    PRINTK_D("queue create write signal=0x%"PRPx"\n", queue->write_signal);
    status = gckvip_os_set_signal(queue->write_signal, vip_true_e);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create write_signal\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    queue->device_id = device_id;
    queue->queue_stop = vip_false_e;
    status = queue_initialize(&queue->queue);

onError:
    return status;
}

vip_status_e gckvip_queue_destroy(
    gckvip_queue_t *queue
    )
{
    vip_status_e status = VIP_SUCCESS;

    queue->queue_stop = vip_true_e;
    if (queue->read_signal != VIP_NULL) {
        gckvip_os_set_signal(queue->read_signal, vip_true_e);
        gckvip_os_destroy_signal(queue->read_signal);
    }
    if (queue->write_signal != VIP_NULL) {
        gckvip_os_set_signal(queue->write_signal, vip_true_e);
        gckvip_os_destroy_signal(queue->write_signal);
    }
    if (queue->mutex != VIP_NULL) {
        gckvip_os_destroy_mutex(queue->mutex);
    }
    if (queue->read_mutex != VIP_NULL) {
        gckvip_os_destroy_mutex(queue->read_mutex);
    }
    if (queue->write_mutex != VIP_NULL) {
        gckvip_os_destroy_mutex(queue->write_mutex);
    }

    queue_destroy(&queue->queue);

    return status;
}

vip_bool_e gckvip_queue_read(
    gckvip_queue_t *queue,
    gckvip_queue_data_t **data_ptr
    )
{
    vip_bool_e read = vip_false_e;
    vip_status_e status = VIP_SUCCESS;

    gckvip_os_lock_mutex(queue->read_mutex);
    while (VIP_SUCCESS == gckvip_os_wait_signal(queue->read_signal, vpmdINFINITE)) {
        if (queue->queue_stop) {
            break;
        }

        status = gckvip_os_lock_mutex(queue->mutex);
        if (status != VIP_SUCCESS) {
            gckvip_os_unlock_mutex(queue->read_mutex);
            PRINTK_E("failed to lock mutex\n");
            return vip_false_e;
        }
        read = queue_read(&queue->queue, data_ptr);
        gckvip_os_unlock_mutex(queue->mutex);

        gcOnError(gckvip_os_set_signal(queue->write_signal, vip_true_e));
        if (GCKVIP_QUEUE_EMPTY == gckvip_queue_status(queue)) {
            gcOnError(gckvip_os_set_signal(queue->read_signal, vip_false_e));
        }

        if (read) {
            break;
        }
    }

onError:
    gckvip_os_unlock_mutex(queue->read_mutex);
    return read;
}

vip_bool_e gckvip_queue_write(
    gckvip_queue_t *queue,
    gckvip_queue_data_t *data
    )
{
    vip_bool_e wrote = vip_false_e;
    vip_status_e status = VIP_SUCCESS;

    gckvip_os_lock_mutex(queue->write_mutex);
    status = gckvip_os_lock_mutex(queue->mutex);
    if (status != VIP_SUCCESS) {
        gckvip_os_unlock_mutex(queue->write_mutex);
        PRINTK_E("failed to lock mutex\n");
        return vip_false_e;
    }
    wrote = queue_write(&queue->queue, data);
    gckvip_os_unlock_mutex(queue->mutex);

    gcOnError(gckvip_os_set_signal(queue->read_signal, vip_true_e));
    if (GCKVIP_QUEUE_FULL == gckvip_queue_status(queue)) {
        gcOnError(gckvip_os_set_signal(queue->write_signal, vip_false_e));
    }

onError:
    gckvip_os_unlock_mutex(queue->write_mutex);
    return wrote;
}

vip_status_e gckvip_queue_clean(
    gckvip_queue_t *queue,
    void* param
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == queue) {
        PRINTK_E("failed to clean queue, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    gckvip_os_lock_mutex(queue->mutex);
    status = queue_clean(&queue->queue, param);
    gckvip_os_unlock_mutex(queue->mutex);

    return status;
}

gckvip_queue_status_e gckvip_queue_status(
    gckvip_queue_t *queue
    )
{
    gckvip_queue_status_e status = GCKVIP_QUEUE_NONE;

    if (VIP_NULL == queue) {
        return GCKVIP_QUEUE_NONE;
    }

    gckvip_os_lock_mutex(queue->mutex);
    status = queue_status(&queue->queue);
    gckvip_os_unlock_mutex(queue->mutex);

    return status;
}
#endif

#define DATABASE_TABLE_MAX_EXPAND    512

vip_status_e gckvip_database_init(
    gckvip_database_t *database
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_database_table_t *table = VIP_NULL;
    vip_uint32_t i = 0;
    gcIsNULL(database);

    database->capacity = DATABASE_TABLE_MAX_EXPAND;
    database->first_free_pos = 1;

    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_database_table_t) * database->capacity,
                                        (vip_ptr*)&table));
    gckvip_os_zero_memory(table, sizeof(gckvip_database_table_t) * database->capacity);
    database->table = table;

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_create_mutex(&database->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create database mutex\n");
        gcGoOnError(VIP_ERROR_IO);
    }
#endif

    for (i = 0; i < database->capacity - 1; i++) {
        database->table[i].next = i + 1;
    }

    return status;

onError:
    if (table != VIP_NULL) {
        gckvip_os_free_memory((void*)table);
    }
    PRINTK_E("failed to initialize data base\n");
    return status;
}

vip_status_e gckvip_database_get_id(
    gckvip_database_t *database,
    vip_ptr handle,
    vip_uint32_t process_id,
    vip_uint32_t *database_id
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_database_table_t *table = VIP_NULL;
    vip_uint32_t i = 0;
    gcIsNULL(database);

    if (VIP_NULL == database_id) {
        PRINTK_E("failed to get video memory id form database, id is NULL pointer");
        return VIP_ERROR_OUT_OF_RESOURCE;
    }
    if (VIP_NULL == database->table) {
        PRINTK_E("failed database table is NULL");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_lock_mutex(database->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mutex\n");
        return VIP_ERROR_FAILURE;
    }
#endif

    if (0 == database->table[0].next) {
        vip_uint32_t expand = DATABASE_TABLE_MAX_EXPAND;
        vip_uint32_t capacity = database->capacity + expand;

        /* Extend table. */
        gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_database_table_t) * capacity,
                                           (vip_ptr*)&table));
        gckvip_os_zero_memory(table, sizeof(gckvip_database_table_t) * capacity);

        if (database->table != VIP_NULL) {
            gckvip_os_memcopy(table, database->table,
                              sizeof(gckvip_database_table_t) * database->capacity);
            gckvip_os_free_memory(database->table);
        }

        /* Update databse with new allocated table. */
        database->table = table;
        database->first_free_pos = database->capacity; /* re-set first free pos */
        database->capacity = capacity;
        for (i = database->first_free_pos; i < database->capacity - 1; i++) {
            database->table[i].next = i + 1;
        }
        database->table[0].next = database->first_free_pos;
        PRINTK_I("expand video memory database, free count=%d, capacity=%d, pid=%x\n",
                  expand, database->capacity, process_id);
    }

    i = database->table[0].next;
    database->table[0].next = database->table[i].next;

    if (i >= database->first_free_pos) {
        database->first_free_pos = i + 1;
    }

    /* Connect id with database handle */
    database->table[i].handle = handle;
    database->table[i].process_id = process_id;
    *database_id = i + DATABASE_MAGIC_DATA;

    PRINTK_D("database convert from first_free_pos=0x%x handle=0x%"PRPx" to id=0x%x, pid=%x\n",
              database->first_free_pos, handle, *database_id, process_id);

onError:
#if vpmdENABLE_MULTIPLE_TASK
    gckvip_os_unlock_mutex(database->mutex);
#endif
    return status;
}

vip_status_e gckvip_database_put_id(
    gckvip_database_t *database,
    vip_uint32_t database_id
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t id = 0;
    gcIsNULL(database);

    if (database_id < DATABASE_MAGIC_DATA) {
        PRINTK_E("failed data base id=0x%x, database magic=0x%x\n",
                 database_id, DATABASE_MAGIC_DATA);
        return VIP_ERROR_OUT_OF_RESOURCE;
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_lock_mutex(database->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mutex\n");
        return VIP_ERROR_FAILURE;
    }
#endif

    if (VIP_NULL == database->table) {
        PRINTK_E("failed to free database, parameter is NULL\n");
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    id = database_id - DATABASE_MAGIC_DATA;

    if (id > database->capacity) {
        PRINTK_E("database is is more than database capacity, %d > %d\n",
                 id, database->capacity);
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    database->table[id].handle = VIP_NULL;
    database->table[id].process_id = 0;
    database->table[id].next = database->table[0].next;
    database->table[0].next = id;
    while ((database->first_free_pos > 1) && (VIP_NULL == database->table[database->first_free_pos - 1].handle)) {
        database->first_free_pos--;
    }

onError:
#if vpmdENABLE_MULTIPLE_TASK
    gckvip_os_unlock_mutex(database->mutex);
#endif
    return status;
}

vip_uint32_t gckvip_database_get_freepos(
    gckvip_database_t *database
    )
{
#if vpmdENABLE_MULTIPLE_TASK
    vip_status_e status = VIP_SUCCESS;
#endif
    vip_uint32_t pos = 0;

    if (VIP_NULL == database) {
        PRINTK_E("failed to get database free pos, database is NULL\n");
        return 0xFFFFFF;
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_lock_mutex(database->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mutex\n");
        return 0xFFFFFF;
    }
#endif

    pos = database->first_free_pos;

#if vpmdENABLE_MULTIPLE_TASK
    gckvip_os_unlock_mutex(database->mutex);
#endif

    return pos;
}

vip_status_e gckvip_database_get_table(
    gckvip_database_t *database,
    vip_uint32_t database_id,
    gckvip_database_table_t **table
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t id = 0;
    gcIsNULL(database);

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_lock_mutex(database->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mutex\n");
        return VIP_ERROR_FAILURE;
    }
#endif

    if (VIP_NULL == database->table) {
        PRINTK_E("failed to convert database id, parameter is NULL, table=0x%"PRPx"\n", database->table);
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    if (database_id < DATABASE_MAGIC_DATA) {
        PRINTK_E("failed data base id=%d, database magic=%d\n",
                 database_id, DATABASE_MAGIC_DATA);
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    id = database_id - DATABASE_MAGIC_DATA;

    if (id >= database->capacity) {
        PRINTK_E("database is out of capacity, require 0 ~ %d, actually %d\n",
                 database->capacity - 1, id);
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    *table = &database->table[id];

onError:
#if vpmdENABLE_MULTIPLE_TASK
    gckvip_os_unlock_mutex(database->mutex);
#endif
    return status;
}

vip_status_e gckvip_database_get_handle(
    gckvip_database_t *database,
    vip_uint32_t database_id,
    vip_ptr *handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t id = 0;
    gcIsNULL(database);

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_lock_mutex(database->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mutex\n");
        return VIP_ERROR_FAILURE;
    }
#endif

    if ((database_id & DATABASE_MAGIC_DATA) != DATABASE_MAGIC_DATA) {
        PRINTK_E("fail to get database handle=0x%x\n", database_id);
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    if ((VIP_NULL == database->table) || (VIP_NULL == handle)) {
        PRINTK_E("failed to convert database id, parameter is NULL,"
                 "mem_handle=0x%"PRPx", table=0x%"PRPx"\n", handle, database->table);
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    if (database_id < DATABASE_MAGIC_DATA) {
        PRINTK_E("failed data base id=0x%x, database magic=0x%x\n",
                  database_id, DATABASE_MAGIC_DATA);
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    id = database_id - DATABASE_MAGIC_DATA;

    if (id >= database->capacity) {
        PRINTK_E("database is out of capacity, require 0 ~ %d, actually %d\n",
                 database->capacity - 1, id);
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    *handle = database->table[id].handle;

#if 0
    PRINTK_D("database convert from id=0x%x to handle=0x%"PRPx"\n", database_id, *handle);
#endif
#if vpmdENABLE_MULTIPLE_TASK
    gckvip_os_unlock_mutex(database->mutex);
#endif
    return status;

onError:
#if vpmdENABLE_MULTIPLE_TASK
    gckvip_os_unlock_mutex(database->mutex);
#endif
    GCKVIP_DUMP_STACK();
    return status;
}

vip_status_e gckvip_database_uninit(
    gckvip_database_t *database
    )
{
    vip_status_e status = VIP_SUCCESS;
    gcIsNULL(database);

    if (database->table != VIP_NULL) {
        gckvip_os_free_memory(database->table);
        database->table = VIP_NULL;
    }

    database->capacity = 0;
    database->first_free_pos = 0;

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_destroy_mutex(database->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to destroy data base mutex\n");
    }
#endif

onError:
    return status;
}

static vip_int32_t hash(
    vip_uint64_t h
    )
{
    h = (h << 5) - h;
    return (h ^ (h >> 8) ^ (h >> 16));
}

static void left_rotate(
    gckvip_hashmap_t *hashmap,
    vip_uint32_t index,
    gckvip_hashmap_node_t *header
    )
{
    vip_uint32_t parent = hashmap->node_array[index].parent_index;
    vip_uint32_t right = hashmap->node_array[index].right_index;
    /* current node is root or not */
    gckvip_hashmap_node_t *p_parent = (0 == parent) ? header : &hashmap->node_array[parent];
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
    gckvip_hashmap_t *hashmap,
    vip_uint32_t index,
    gckvip_hashmap_node_t *header
    )
{
    vip_uint32_t parent = hashmap->node_array[index].parent_index;
    vip_uint32_t left = hashmap->node_array[index].left_index;
    /* current node is root or not */
    gckvip_hashmap_node_t *p_parent = (0 == parent) ? header : &hashmap->node_array[parent];
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
    gckvip_hashmap_t *hashmap,
    gckvip_hashmap_node_t *header,
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

vip_status_e gckvip_hashmap_init(
    IN gckvip_hashmap_t* hashmap,
    IN vip_uint32_t init_capacity,
    IN vip_uint32_t size_per_element,
    IN vip_char_t_ptr name,
    IN vip_status_e(*callback)(void* element)
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t i = 0;
    vip_uint32_t next_index = 0;
    hashmap->capacity = init_capacity + 1; /* index 0: idle list header */
    hashmap->size_per_element = size_per_element;

    status = gckvip_os_create_mutex(&hashmap->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create mutex for hashmap\n");
        gcGoOnError(status);
    }
    hashmap->expand_disable = 0;
    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_hashmap_node_t) * hashmap->capacity,
                                      (void**)&hashmap->node_array));
    gckvip_os_zero_memory(hashmap->node_array, sizeof(gckvip_hashmap_node_t) * hashmap->capacity);

    if (0 < hashmap->size_per_element) {
        gcOnError(gckvip_os_allocate_memory(hashmap->size_per_element * hashmap->capacity,
                                          (void**)&hashmap->container));
        gckvip_os_zero_memory(hashmap->container, hashmap->size_per_element * hashmap->capacity);
    }

    for (i = hashmap->capacity - 1; i > 0; i--) {
        hashmap->node_array[i].right_index = next_index;
        hashmap->node_array[i].left_index = 0;
        next_index = i;
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
    hashmap->callback = callback;
    /*PRINTK("HASHMAP 0x%"PRPx"(%s) INIT SUCCESS\n", hashmap, name);*/
    return status;

onError:
    if (VIP_NULL != hashmap->mutex) {
        gckvip_os_destroy_mutex(hashmap->mutex);
        hashmap->mutex = VIP_NULL;
    }
    if (VIP_NULL != hashmap->node_array) {
        gckvip_os_free_memory(hashmap->node_array);
        hashmap->node_array = VIP_NULL;
    }
    if (VIP_NULL != hashmap->container) {
        gckvip_os_free_memory(hashmap->container);
        hashmap->container = VIP_NULL;
    }
    return status;
}

vip_status_e gckvip_hashmap_expand(
    IN gckvip_hashmap_t *hashmap,
    OUT vip_bool_e *expand
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t next_index = 0;
    vip_uint32_t index = 0;
    vip_status_e status = VIP_SUCCESS;
    gckvip_hashmap_node_t *new_node_array = VIP_NULL;
    void *new_container;
    *expand = vip_false_e;

    while (1) {
        if (0 == hashmap->expand_disable) {
            status = gckvip_os_lock_mutex(hashmap->mutex);
            if (status != VIP_SUCCESS) {
                PRINTK_E("failed to lock mutex when hashmap expand\n");
                return status;
            }
            if (0 == hashmap->expand_disable) {
                /* no task is executed */
                break;
            }
            gckvip_os_unlock_mutex(hashmap->mutex);
        }
        gckvip_os_delay(10);
        if (vip_false_e == gckvip_hashmap_full(hashmap)) {
            PRINTK_D("no need to expand hashmap\n");
            return status;
        }
    }
    if (vip_false_e == gckvip_hashmap_full(hashmap)) {
        PRINTK_D("hashmap has been expanded\n");
        gcGoOnError(status);
    }
    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_hashmap_node_t) *
                                       (hashmap->capacity + HASH_MAP_CAPABILITY),
                                       (void**)&new_node_array));
    gckvip_os_zero_memory(new_node_array,
                          sizeof(gckvip_hashmap_node_t) * (hashmap->capacity + HASH_MAP_CAPABILITY));

    if (0 < hashmap->size_per_element) {
        status = gckvip_os_allocate_memory(hashmap->size_per_element *
                                          (hashmap->capacity + HASH_MAP_CAPABILITY),
                                          (void**)&new_container);
        if (VIP_SUCCESS != status) {
            if (new_node_array != VIP_NULL) {
                gckvip_os_free_memory((void*)new_node_array);
            }
            PRINTK_E("fail to alloc memory status=%d\n", status);
            gcOnError(status);
        }
        gckvip_os_zero_memory(new_container, hashmap->size_per_element * (hashmap->capacity + HASH_MAP_CAPABILITY));
        gckvip_os_memcopy(new_container, hashmap->container,
                          hashmap->size_per_element * hashmap->capacity);
        gckvip_os_free_memory(hashmap->container);
        hashmap->container = new_container;
    }

    gckvip_os_memcopy(new_node_array, hashmap->node_array,
                      sizeof(gckvip_hashmap_node_t) * hashmap->capacity);
    gckvip_os_free_memory(hashmap->node_array);

    hashmap->node_array = new_node_array;
    hashmap->capacity += HASH_MAP_CAPABILITY;
    for (i = 0; i < HASH_MAP_CAPABILITY; i++) {
        index = hashmap->capacity - 1 - i;
        hashmap->node_array[index].right_index = next_index;
        hashmap->node_array[index].left_index = 0;
        next_index = index;
    }
    hashmap->idle_list_head = &hashmap->node_array[0];
    hashmap->idle_list_head->right_index = next_index;
#if HASH_MAP_IDLE_LIST_TAIL
    hashmap->idle_list_tail = &hashmap->node_array[hashmap->capacity - 1];
#endif
    *expand = vip_true_e;

onError:
    gckvip_os_unlock_mutex(hashmap->mutex);
    return status;
}

vip_status_e gckvip_hashmap_destroy(
    IN gckvip_hashmap_t *hashmap
    )
{
    vip_int32_t i = 0;
    if (VIP_NULL != hashmap->mutex) {
        gckvip_os_destroy_mutex(hashmap->mutex);
        hashmap->mutex = VIP_NULL;
    }
    if (VIP_NULL != hashmap->node_array) {
        gckvip_os_free_memory(hashmap->node_array);
        hashmap->node_array = VIP_NULL;
    }
    if (VIP_NULL != hashmap->container) {
        gckvip_os_free_memory(hashmap->container);
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

vip_status_e gckvip_hashmap_insert(
    IN gckvip_hashmap_t* hashmap,
    IN void* handle,
    OUT vip_uint32_t* index
    )
{
    vip_uint32_t cur = 0;
    vip_uint32_t h = 0;
    vip_uint32_t parent = 0;
    vip_uint32_t grand_p = 0;
    vip_uint32_t uncle = 0;
    vip_uint32_t *p = VIP_NULL;
    gckvip_hashmap_node_t *header = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    status = gckvip_os_lock_mutex(hashmap->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mutex when hashmap put\n");
        return status;
    }

    h = (HASH_MAP_CAPABILITY - 1) & hash(GCKVIPPTR_TO_UINT64(handle));
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
            PRINTK_D("HASHMAP 0x%"PRPx" INSERT: handle 0x%"PRPx" exist in hashmap\n", hashmap, handle);
            gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
        }
    }

    /* use the first node in idle list */
    cur = hashmap->idle_list_head->right_index;
    /* no idle node */
    if (0 == cur) {
        *index = 0;
        PRINTK_D("HASHMAP 0x%"PRPx" INSERT: hashmap is full\n", hashmap);
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
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
    /* link parent node to current node */
    *p = cur;
    /* record ID of current node */
    *index = cur;
    PRINTK_D("hashmap 0x%"PRPx": insert index=%d, handle=0x%"PRPx"\n", hashmap, *index, handle);
    /* VIV: Ref https://www.jianshu.com/p/e136ec79235c */
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
    gckvip_os_unlock_mutex(hashmap->mutex);
    return status;
}

static void hashmap_remove(
    gckvip_hashmap_t* hashmap,
    vip_uint32_t index
    )
{
    /* VIV: Ref https://www.jianshu.com/p/e136ec79235c */
    vip_uint32_t* del = VIP_NULL;
    vip_uint32_t h = (HASH_MAP_CAPABILITY - 1) & hash(GCKVIPPTR_TO_UINT64(hashmap->node_array[index].handle));
    /* R: replace node; P: parent; S: brother; SL: brother's left node; SR: brother's right node */
    vip_uint32_t R, P, S, SL, SR;
    /* find a replace node */
    vip_uint32_t replace = 0;
    PRINTK_D("hashmap 0x%"PRPx": remove index=%d, handle=0x%"PRPx"\n",
        hashmap, index, hashmap->node_array[index].handle);
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
}

vip_status_e gckvip_hashmap_remove(
    IN gckvip_hashmap_t* hashmap,
    IN void* handle
    )
{
    vip_uint32_t h = 0;
    vip_uint32_t *del = VIP_NULL;
    vip_uint32_t index = 0;
    gckvip_hashmap_node_t *header = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    status = gckvip_os_lock_mutex(hashmap->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mutex when hashmap get\n");
        return status;
    }

    h = (HASH_MAP_CAPABILITY - 1) & hash(GCKVIPPTR_TO_UINT64(handle));
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

    if (0 != index) {
        hashmap->node_array[index].remove = 1;
        if (0 == hashmap->node_array[index].ref_count) {
            hashmap_remove(hashmap, index);
        }
    }
    else {
        PRINTK_D("HASHMAP 0x%"PRPx" REMOVE: handle 0x%"PRPx" not exist in hashmap\n", hashmap, handle);
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }

    gckvip_os_unlock_mutex(hashmap->mutex);
    return status;
}

vip_status_e gckvip_hashmap_get_by_handle(
    IN gckvip_hashmap_t* hashmap,
    IN void* handle,
    OUT vip_uint32_t* index,
    OUT void** element
    )
{
    vip_uint32_t h = 0;
    vip_uint32_t *get = VIP_NULL;
    gckvip_hashmap_node_t *header = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    status = gckvip_os_lock_mutex(hashmap->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mutex when hashmap get\n");
        return status;
    }

    h = (HASH_MAP_CAPABILITY - 1) & hash(GCKVIPPTR_TO_UINT64(handle));
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

    if (0 != *index) {
        hashmap->expand_disable += 1;
        hashmap->node_array[*index].ref_count += 1;
        if (VIP_NULL != element) {
            if (0 == hashmap->size_per_element) {
                *element = hashmap->node_array[*index].handle;
            }
            else {
                *element = (void*)((vip_uint8_t*)hashmap->container + *index * hashmap->size_per_element);
            }
        }
    }
    else {
        PRINTK_D("HASHMAP 0x%"PRPx" GET BY HANDLE: handle 0x%"PRPx" not exist in hashmap\n", hashmap, handle);
        if (VIP_NULL != element) {
            *element = VIP_NULL;
        }
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }

    gckvip_os_unlock_mutex(hashmap->mutex);
    return status;
}

vip_status_e gckvip_hashmap_get_by_index(
    IN gckvip_hashmap_t* hashmap,
    IN vip_uint32_t index,
    OUT void** element
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_os_lock_mutex(hashmap->mutex);
    if (0 < index && index < hashmap->capacity) {
        hashmap->expand_disable += 1;
        hashmap->node_array[index].ref_count += 1;
        if (VIP_NULL != element) {
            if (0 == hashmap->size_per_element) {
                *element = hashmap->node_array[index].handle;
            }
            else {
                *element = (void*)((vip_uint8_t*)hashmap->container + index * hashmap->size_per_element);
            }
        }
    }
    else {
        PRINTK_D("HASHMAP 0x%"PRPx" GET BY INDEX: index %d out of range\n", hashmap, index);
        if (VIP_NULL != element) {
            *element = VIP_NULL;
        }
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }
    gckvip_os_unlock_mutex(hashmap->mutex);
    return status;
}

vip_status_e gckvip_hashmap_unuse(
    IN gckvip_hashmap_t *hashmap,
    IN vip_uint32_t index,
    IN vip_bool_e callback
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_os_lock_mutex(hashmap->mutex);
    if (0 < index && index < hashmap->capacity) {
        hashmap->node_array[index].ref_count -= 1;
        hashmap->expand_disable -= 1;
        if (0 == hashmap->node_array[index].ref_count && hashmap->node_array[index].remove) {
            hashmap_remove(hashmap, index);
        }
        if (callback && hashmap->callback) {
            hashmap->callback((void*)((vip_uint8_t*)hashmap->container + index * hashmap->size_per_element));
        }
    }
    else {
        PRINTK_D("HASHMAP 0x%"PRPx" UNUSE: index %d out of range\n", hashmap, index);
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }
    gckvip_os_unlock_mutex(hashmap->mutex);
    return status;
}

vip_uint32_t gckvip_hashmap_capacity(
    gckvip_hashmap_t *hashmap
    )
{
    return hashmap->capacity;
}

vip_uint32_t gckvip_hashmap_free_pos(
    gckvip_hashmap_t *hashmap
    )
{
    return hashmap->free_pos;
}

vip_bool_e gckvip_hashmap_full(
    gckvip_hashmap_t *hashmap
    )
{
    return (0 == hashmap->idle_list_head->right_index) ? vip_true_e : vip_false_e;
}

vip_bool_e gckvip_hashmap_check_remove(
    gckvip_hashmap_t* hashmap,
    vip_uint32_t index
    )
{
    return (1 == hashmap->node_array[index].remove) ? vip_true_e : vip_false_e;
}

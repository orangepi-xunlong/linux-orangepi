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

#ifndef _GC_VIP_KERNEL_UTIL_H
#define _GC_VIP_KERNEL_UTIL_H

#include <gc_vip_common.h>
#include <vip_lite.h>
#include <gc_vip_kernel_port.h>

typedef struct _gckvip_recursive_mutex
{
    /* current thread id*/
    volatile vip_uint32_t current_tid;
    /* reference count*/
    volatile vip_int32_t  ref_count;
    gckvip_mutex          mutex;
} gckvip_recursive_mutex;

vip_status_e gckvip_lock_recursive_mutex(
    gckvip_recursive_mutex *recursive_mutex
    );

vip_status_e gckvip_unlock_recursive_mutex(
    gckvip_recursive_mutex *recursive_mutex
    );

typedef enum _gckvip_queue_status
{
    GCKVIP_QUEUE_NONE       = 0,
    GCKVIP_QUEUE_EMPTY      = 1,
    GCKVIP_QUEUE_WITH_DATA  = 2,
    GCKVIP_QUEUE_FULL       = 3,
    GCKVIP_QUEUE_MAX,
} gckvip_queue_status_e;

typedef struct _gckvip_database_table
{
    vip_ptr            *handle;
    vip_uint32_t       process_id;
    vip_uint32_t       next;
} gckvip_database_table_t;

typedef struct _gckvip_database
{
    gckvip_database_table_t *table;
    vip_uint32_t       capacity;
    vip_uint32_t       first_free_pos; /* the first free position in database */
 #if vpmdENABLE_MULTIPLE_TASK
    gckvip_mutex       mutex;
 #endif
} gckvip_database_t;

#if vpmdENABLE_MULTIPLE_TASK
#define QUEUE_CAPABILITY 32
typedef struct _gckvip_queue_data
{
    /* the task index in task_slot */
    volatile vip_uint32_t   v1;
    /* the status of task inference  */
    volatile vip_status_e   v2;
#if vpmdENABLE_PREEMPTION
    /* the task running priority */
    vip_uint8_t         priority;
#endif
} gckvip_queue_data_t;

#if vpmdENABLE_PREEMPTION
typedef struct _gckvip_queue_node
{
    vip_uint32_t         index;
    gckvip_queue_data_t* data;
} gckvip_queue_node;

typedef struct _gckvip_priority_queue
{
    gckvip_queue_node*  head;
    vip_uint32_t        max_size;
    vip_uint32_t        cur_size;
    vip_uint32_t        count;
} gckvip_priority_queue;
#else
typedef struct _gckvip_fifo_queue
{
    gckvip_queue_data_t*  data[QUEUE_CAPABILITY];
    vip_int32_t           begin_index;
    vip_int32_t           end_index;
} gckvip_fifo_queue;
#endif

typedef struct _gckvip_queue
{
    gckvip_mutex            mutex;
    gckvip_mutex            read_mutex;
    gckvip_mutex            write_mutex;
    void                    *read_signal;
    void                    *write_signal;
    vip_bool_e              queue_stop;
    vip_uint32_t            device_id;
#if vpmdENABLE_PREEMPTION
    gckvip_priority_queue   queue;
#else
    gckvip_fifo_queue       queue;
#endif
} gckvip_queue_t;

vip_status_e gckvip_queue_initialize(
    gckvip_queue_t *queue,
    vip_uint32_t   device_id
    );

vip_status_e gckvip_queue_destroy(
    gckvip_queue_t *queue
    );

vip_bool_e gckvip_queue_read(
    gckvip_queue_t *queue,
    gckvip_queue_data_t **data_ptr
    );

vip_bool_e gckvip_queue_write(
    gckvip_queue_t *queue,
    gckvip_queue_data_t *data
    );

vip_status_e gckvip_queue_clean(
    gckvip_queue_t *queue,
    void* param
    );
gckvip_queue_status_e gckvip_queue_status(
    gckvip_queue_t *queue
    );
#endif

#define DATABASE_MAGIC_DATA          0x15000000

vip_status_e gckvip_database_init(
    gckvip_database_t *database
    );

vip_status_e gckvip_database_get_id(
    gckvip_database_t *database,
    vip_ptr handle,
    vip_uint32_t process_id,
    vip_uint32_t *database_id
    );

vip_status_e gckvip_database_put_id(
    gckvip_database_t *database,
    vip_uint32_t database_id
    );

vip_status_e gckvip_database_get_handle(
    gckvip_database_t *database,
    vip_uint32_t database_id,
    vip_ptr *handle
    );

vip_uint32_t gckvip_database_get_freepos(
    gckvip_database_t *database
    );

vip_status_e gckvip_database_get_table(
    gckvip_database_t *database,
    vip_uint32_t database_id,
    gckvip_database_table_t **table
    );

vip_status_e gckvip_database_uninit(
    gckvip_database_t *database
    );

#define HASH_MAP_CAPABILITY 128
#define HASH_MAP_IDLE_LIST_TAIL 0

typedef struct _gckvip_hashmap_node
{
    void                 *handle;
    volatile vip_uint16_t ref_count;
    volatile vip_uint16_t remove;
    /* for red black tree */
    vip_uint32_t          parent_index;
    vip_uint32_t          left_index;
    vip_uint32_t          right_index;
    vip_bool_e            is_black;
} gckvip_hashmap_node_t;

typedef struct _gckvip_hashmap
{
    gckvip_hashmap_node_t  hashmap[HASH_MAP_CAPABILITY];
    gckvip_hashmap_node_t *idle_list_head;
#if HASH_MAP_IDLE_LIST_TAIL
    gckvip_hashmap_node_t *idle_list_tail;
#endif
    gckvip_hashmap_node_t *node_array;
    vip_uint32_t           capacity;
    volatile vip_uint32_t  free_pos;
    void                  *container; /* capacity elements in container */
    vip_uint32_t           size_per_element;
    gckvip_mutex           mutex; /* for read, write and expand */
    volatile vip_uint32_t  expand_disable;  /* when any element is inuse, expand is not allowed */
    vip_status_e           (*callback)(void*); /* callback function of this hashmap */
} gckvip_hashmap_t;

/*
@brief  init hashmap.
@param  hashmap, the hashmap.
@param  init_capacity, the initial capacity of hashmap.
@param  size_per_element, the size of each element in container.
@param  name, name of this hashmap.
@param  callback, the callback function.
*/
vip_status_e gckvip_hashmap_init(
    IN gckvip_hashmap_t* hashmap,
    IN vip_uint32_t init_capacity,
    IN vip_uint32_t size_per_element,
    IN vip_char_t_ptr name,
    IN vip_status_e(*callback)(void* element)
    );

/*
@brief  expand hashmap.
@param  hashmap, the hashmap.
@param  expand, expand really or not.
*/
vip_status_e gckvip_hashmap_expand(
    IN gckvip_hashmap_t* hashmap,
    OUT vip_bool_e *expand
    );

/*
@brief  destroy hashmap.
@param  hashmap, the hashmap.
*/
vip_status_e gckvip_hashmap_destroy(
    IN gckvip_hashmap_t* hashmap
    );

/*
@brief  insert handle into hashmap.
@param  hashmap, the hashmap.
@param  handle, the unique handle.
@param  index, the index of element in container.
*/
vip_status_e gckvip_hashmap_insert(
    IN gckvip_hashmap_t* hashmap,
    IN void* handle,
    OUT vip_uint32_t* index
    );

/*
@brief  remove handle from hashmap.
@param  hashmap, the hashmap.
@param  handle, the unique handle.
*/
vip_status_e gckvip_hashmap_remove(
    IN gckvip_hashmap_t* hashmap,
    IN void* handle
    );

/*
@brief  get from hashmap by handle.
@param  hashmap, the hashmap.
@param  handle, the unique handle.
@param  index, the index of element in container.
@param  element, the handle of element.
*/
vip_status_e gckvip_hashmap_get_by_handle(
    IN gckvip_hashmap_t* hashmap,
    IN void* handle,
    OUT vip_uint32_t* index,
    OUT void** element
    );

/*
@brief  get from hashmap by index.
@param  hashmap, the hashmap.
@param  index, the index of element in container.
@param  element, the handle of element.
*/
vip_status_e gckvip_hashmap_get_by_index(
    IN gckvip_hashmap_t* hashmap,
    IN vip_uint32_t index,
    OUT void** element
    );

/*
@brief  unuse the element in container.
@param  hashmap, the hashmap.
@param  index, the index of element in container.
@param  callback, trigger call back function or not.
*/
vip_status_e gckvip_hashmap_unuse(
    IN gckvip_hashmap_t *hashmap,
    IN vip_uint32_t index,
    IN vip_bool_e callback
    );

vip_uint32_t gckvip_hashmap_capacity(
    gckvip_hashmap_t *hashmap
    );

vip_uint32_t gckvip_hashmap_free_pos(
    gckvip_hashmap_t *hashmap
    );

vip_bool_e gckvip_hashmap_full(
    gckvip_hashmap_t *hashmap
    );

vip_bool_e gckvip_hashmap_check_remove(
    gckvip_hashmap_t* hashmap,
    vip_uint32_t index
    );
#endif

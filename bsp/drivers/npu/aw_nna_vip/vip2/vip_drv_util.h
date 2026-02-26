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

#ifndef _VIP_DRV_UTIL_H_
#define _VIP_DRV_UTIL_H_

#include <vip_lite_config.h>
#include <vip_drv_os_port.h>

typedef struct _vipdrv_recursive_mutex
{
    /* current thread id*/
    volatile vip_uint32_t current_tid;
    /* reference count*/
    volatile vip_int32_t  ref_count;
    vipdrv_mutex          mutex;
} vipdrv_recursive_mutex;

vip_status_e vipdrv_create_recursive_mutex(
    vipdrv_recursive_mutex *recursive_mutex
    );

vip_status_e vipdrv_destroy_recursive_mutex(
    vipdrv_recursive_mutex *recursive_mutex
    );

vip_status_e vipdrv_lock_recursive_mutex(
    vipdrv_recursive_mutex *recursive_mutex
    );

vip_status_e vipdrv_unlock_recursive_mutex(
    vipdrv_recursive_mutex *recursive_mutex
    );

typedef struct _vipdrv_readwrite_mutex
{
    /* read lock count*/
    volatile vip_uint16_t r_active_cnt;
    volatile vip_uint8_t  w_active;
    vipdrv_mutex          rw_mutex;
} vipdrv_readwrite_mutex;

vip_status_e vipdrv_create_readwrite_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    );

vip_status_e vipdrv_destroy_readwrite_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    );

vip_status_e vipdrv_lock_read_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    );

vip_status_e vipdrv_unlock_read_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    );

vip_status_e vipdrv_lock_write_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    );

vip_status_e vipdrv_unlock_write_mutex(
    vipdrv_readwrite_mutex *readwrite_mutex
    );

typedef enum _vipdrv_queue_status
{
    VIPDRV_QUEUE_NONE       = 0,
    VIPDRV_QUEUE_EMPTY      = 1,
    VIPDRV_QUEUE_WITH_DATA  = 2,
    VIPDRV_QUEUE_FULL       = 3,
    VIPDRV_QUEUE_MAX,
} vipdrv_queue_status_e;

#if vpmdTASK_QUEUE_USED
#define QUEUE_CAPABILITY 32
typedef struct _vipdrv_queue_data
{
    /* the task index in TCB hashmap if the used for task management */
    volatile vip_uint32_t   v1;
    /* the status of task inference  */
    volatile vip_status_e   v2;
#if vpmdENABLE_PREEMPTION
    /* the task running priority */
    vip_uint8_t         priority;
#endif
    /* the estimated time of this task */
    vip_uint32_t        time;
    /* The task works on the mask of hardware cores */
    vip_uint16_t        res_mask;
} vipdrv_queue_data_t;

typedef struct _vipdrv_queue_property
{
    volatile vip_uint32_t capacity;
    volatile vip_uint32_t task_count;
    volatile vip_uint64_t estimated_time;
} vipdrv_queue_property_t;

typedef enum _vipdrv_queue_clean_flag
{
    VIPDRV_QUEUE_CLEAN_ALL = 0,
    VIPDRV_QUEUE_CLEAN_STASK = 1,
    VIPDRV_QUEUE_CLEAN_ONE = 2,
    VIPDRV_QUEUE_CLEAN_MAX,
} vipdrv_queue_clean_flag_e;

#if vpmdENABLE_PREEMPTION
typedef struct _vipdrv_queue_node
{
    /* The order in which nodes are added to max_heap */
    vip_uint32_t         index;
    vipdrv_queue_data_t* data;
} vipdrv_queue_node_t;

typedef struct _vipdrv_max_heap
{
    vipdrv_queue_node_t*       head;
    vipdrv_queue_property_t    property;
    struct _vipdrv_max_heap*   left;
    struct _vipdrv_max_heap*   right;
} vipdrv_max_heap_t;

typedef struct _vipdrv_priority_queue
{
    vipdrv_max_heap_t*  max_heap;
    vip_uint32_t        max_heap_count;
    vip_uint32_t        cur_count;
    vip_uint32_t        sum_count;
} vipdrv_priority_queue_t;
#else
typedef struct _vipdrv_fifo_queue
{
    vipdrv_queue_data_t*    data[QUEUE_CAPABILITY];
    vip_int32_t             begin_index;
    vip_int32_t             end_index;
    vipdrv_queue_property_t property;
} vipdrv_fifo_queue;
#endif

typedef struct _vipdrv_queue
{
    vipdrv_mutex            mutex;
    vip_bool_e              queue_stop;
#if vpmdENABLE_PREEMPTION
    vipdrv_priority_queue_t queue;
#else
    vipdrv_fifo_queue       queue;
#endif
} vipdrv_queue_t;

vip_status_e vipdrv_queue_initialize(
    vipdrv_queue_t *queue,
    vip_uint32_t device_index,
    vip_uint32_t queue_cnt
    );

vip_status_e vipdrv_queue_destroy(
    vipdrv_queue_t *queue
    );

vip_bool_e vipdrv_queue_read(
    vipdrv_queue_t *queue,
    vipdrv_queue_data_t **data_ptr,
    void* flag
    );

vip_bool_e vipdrv_queue_write(
    vipdrv_queue_t *queue,
    vipdrv_queue_data_t *data,
    void* flag
    );

vip_status_e vipdrv_queue_clean(
    vipdrv_queue_t *queue,
    vipdrv_queue_clean_flag_e flag,
    void* param
    );

vipdrv_queue_status_e vipdrv_queue_status(
    vipdrv_queue_t *queue,
    void* flag
    );

vip_status_e vipdrv_queue_query_property(
    vipdrv_queue_t *queue,
    void* flag,
    vipdrv_queue_property_t** property
    );
#endif

#define HASH_MAP_CAPABILITY 128
#define HASH_MAP_IDLE_LIST_TAIL 0

typedef struct _vipdrv_hashmap_node
{
    vip_uint64_t          handle;
    volatile vip_uint8_t  used;
    volatile vip_uint8_t  remove;
    volatile vip_uint16_t ref_count;
    vipdrv_spinlock       spinlock; /* for use and unuse element */
    /* for red black tree */
    vip_uint32_t          parent_index;
    vip_uint32_t          left_index;
    vip_uint32_t          right_index;
    vip_bool_e            is_black;
} vipdrv_hashmap_node_t;

typedef struct _vipdrv_hashmap
{
    vipdrv_hashmap_node_t  hashmap[HASH_MAP_CAPABILITY];
    vipdrv_hashmap_node_t *idle_list_head;
#if HASH_MAP_IDLE_LIST_TAIL
    vipdrv_hashmap_node_t *idle_list_tail;
#endif
    vipdrv_hashmap_node_t *node_array;
    vip_uint32_t           capacity;
    volatile vip_uint32_t  free_pos;
    void                  *container; /* capacity elements in container */
    vip_uint32_t           size_per_element;
    vip_char_t            *name;
    vipdrv_recursive_mutex mutex; /* for insert, remove */
    vipdrv_readwrite_mutex expand_mutex; /* for expand */
    vip_status_e           (*callback_unuse)(void*); /* callback function when hashmap unuse item */
    vip_status_e           (*callback_remove)(void*); /* callback function when hashmap remove item */
    vip_status_e           (*callback_destroy)(void*); /* callback function when hashmap destroy for free resource */
} vipdrv_hashmap_t;

/*
@brief  init hashmap.
@param  hashmap, the hashmap.
@param  init_capacity, the initial capacity of hashmap.
@param  size_per_element, the size of each element in container.
@param  name, name of this hashmap.
@param  callback_unuse, the callback function when hashmap unuse.
@param  callback_remove, the callback function when hashmap remove.
@param  callback_destroy, the callback function when hashmap destroy.
*/
vip_status_e vipdrv_hashmap_init(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint32_t init_capacity,
    IN vip_uint32_t size_per_element,
    IN vip_char_t_ptr name,
    IN vip_status_e(*callback_unuse)(void* element),
    IN vip_status_e(*callback_remove)(void* element),
    IN vip_status_e(*callback_destroy)(void* element)
    );

/*
@brief  expand hashmap.
@param  hashmap, the hashmap.
@param  expand, expand really or not.
*/
vip_status_e vipdrv_hashmap_expand(
    IN vipdrv_hashmap_t* hashmap,
    OUT vip_bool_e *expand
    );

/*
@brief  destroy hashmap.
@param  hashmap, the hashmap.
*/
vip_status_e vipdrv_hashmap_destroy(
    IN vipdrv_hashmap_t* hashmap
    );

/*
@brief  insert handle into hashmap.
@param  hashmap, the hashmap.
@param  handle, the unique handle.
@param  index, the index of element in container.
*/
vip_status_e vipdrv_hashmap_insert(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint64_t handle,
    OUT vip_uint32_t* index
    );

/*
@brief  remove handle from hashmap.
@param  hashmap, the hashmap.
@param  handle, the unique handle.
*/
vip_status_e vipdrv_hashmap_remove(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint64_t handle,
    IN vip_bool_e force
    );

/*
@brief  remove handle from hashmap by index.
@param  hashmap, the hashmap.
@param  index, the index of element in container.
*/
vip_status_e vipdrv_hashmap_remove_by_index(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint32_t index,
    IN vip_bool_e force
    );

/*
@brief  get from hashmap by handle.
@param  hashmap, the hashmap.
@param  handle, the unique handle.
@param  index, the index of element in container.
@param  element, the pointer of element.
*/
vip_status_e vipdrv_hashmap_get_by_handle(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint64_t handle,
    OUT vip_uint32_t* index,
    OUT void** element
    );

/*
@brief  get from hashmap by index.
@param  hashmap, the hashmap.
@param  index, the index of element in container.
@param  element, the pointer of element.
*/
vip_status_e vipdrv_hashmap_get_by_index(
    IN vipdrv_hashmap_t* hashmap,
    IN vip_uint32_t index,
    OUT void** element
    );

/*
@brief  unuse the element in container.
@param  hashmap, the hashmap.
@param  index, the index of element in container.
@param  callback, trigger call back function or not.
*/
vip_status_e vipdrv_hashmap_unuse(
    IN vipdrv_hashmap_t *hashmap,
    IN vip_uint32_t index,
    IN vip_bool_e callback
    );

vip_uint32_t vipdrv_hashmap_free_pos(
    vipdrv_hashmap_t *hashmap
    );

vip_bool_e vipdrv_hashmap_full(
    vipdrv_hashmap_t *hashmap
    );

vip_bool_e vipdrv_hashmap_check_remove(
    vipdrv_hashmap_t* hashmap,
    vip_uint32_t index
    );

#define DATABASE_CAPABILITY 128
typedef struct _vipdrv_database_node
{
    volatile vip_uint8_t  used;
    volatile vip_uint8_t  remove;
    volatile vip_uint16_t ref_count;
    vip_uint32_t          next_index;
    vipdrv_spinlock       spinlock; /* for use and unuse element */
} vipdrv_database_node_t;

typedef struct _vipdrv_database
{
    vipdrv_database_node_t *idle_list_head;
    vipdrv_database_node_t *node_array;
    vip_uint32_t            capacity;
    volatile vip_uint32_t   free_pos;
    void                    *container; /* capacity elements in container */
    vip_uint32_t            size_per_element;
    vip_char_t             *name;
    vipdrv_recursive_mutex  mutex; /* for insert, remove */
    vipdrv_readwrite_mutex  expand_mutex; /* for expand */
    vip_status_e           (*callback_remove)(void*); /* callback function when database remove item */
} vipdrv_database_t;

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
    );

/*
@brief  expand database.
@param  database, the database.
@param  expand, expand really or not.
*/
vip_status_e vipdrv_database_expand(
    IN vipdrv_database_t* database,
    OUT vip_bool_e *expand
    );

/*
@brief  destroy database.
@param  database, the database.
*/
vip_status_e vipdrv_database_destroy(
    IN vipdrv_database_t* database
    );

/*
@brief  get empty index of database.
@param  database, the database.
@param  index, the index of element in container.
*/
vip_status_e vipdrv_database_insert(
    IN vipdrv_database_t* database,
    OUT vip_uint32_t* index
    );

/*
@brief  remove from database.
@param  database, the database.
@param  index, the index of element in container.
*/
vip_status_e vipdrv_database_remove(
    IN vipdrv_database_t* database,
    IN vip_uint32_t index,
    IN vip_bool_e force
    );

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
    );

/*
@brief  unuse the element in container.
@param  database, the database.
@param  index, the index of element in container.
*/
vip_status_e vipdrv_database_unuse(
    IN vipdrv_database_t* database,
    IN vip_uint32_t index
    );

vip_uint32_t vipdrv_database_free_pos(
    vipdrv_database_t* database
    );

vip_bool_e vipdrv_database_full(
    vipdrv_database_t* database
    );
#endif

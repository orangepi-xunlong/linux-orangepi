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
#ifndef _GC_VIP_DEBUG_FS_H
#define _GC_VIP_DEBUG_FS_H

#include "gc_vip_common.h"

#if vpmdENABLE_DEBUGFS

typedef struct _gckvip_mem_mapping_profile_t {
    vip_char_t *alloc_type; /* the video memory allocate type  */
    vip_uint32_t size; /* the video memory size  */
    vip_uint32_t physical; /* the viplite virtual address  */
    vip_uint32_t virtual_addr; /* the viplite virtual address  */
    phy_address_t *physical_table; /* the phyiscal address of each page(segment) */
    vip_uint32_t physical_num; /* the number of physical_table element */
    vip_uint32_t *size_table; /* the size of each page table element */
    vip_uint8_t *kernel_logical; /* the kernel space logical address.*/
    vip_uint8_t *user_logical; /* the user space logical address.*/
} gckvip_mem_mapping_profile_t;

typedef struct _gckvip_profile_t {
    vip_uint64_t init_time; /* the hardware init time */
    vip_uint64_t submit_time; /* the network submit time */
    vip_uint64_t wait_time; /* the end time for network run */

    vip_uint64_t total_time; /* the total time for hardware work  */
    vip_uint64_t infer_time; /* the total time for network inference */

    vip_uint32_t video_memory; /* the current video memory  */
    vip_uint32_t video_peak; /* the video memory peak value  */
    vip_uint32_t video_allocs; /* video memory allocate count */
    vip_uint32_t video_frees; /* video memory free count  */
} gckvip_profile_t;

typedef struct _gckvip_core_loading_profile_t {
    vip_uint64_t init_time; /* the hardware init time */
    vip_uint64_t destory_time; /* the hardware end time */
    vip_uint64_t submit_time; /* the network submit time */

    vip_uint64_t infer_time; /* the total time for network inference */

    vip_uint64_t last_record_time; /* the last record time */
    vip_uint64_t last_infer_time; /* the last record infer time */
    vip_uint64_t latest_sbumit_time; /* the last submit time */
} gckvip_core_loading_profile_t;

typedef enum _gckvip_profile_data_type_t {
  /*!< \brief record time type is init time*/
  PROFILE_INIT = 0,
  /*!< \brief record time type is sumbit time */
  PROFILE_SUBMIT = 1,
  /*!< \brief record time type is wait time */
  PROFILE_WAIT = 2,
} gckvip_profile_dtype_t;

/*
@brief Creat debugfs profile dir and file
*/
vip_status_e  gckvip_debug_create_fs(void);

/*
@brief Destroy debugfs profile dir and file
*/
vip_status_e  gckvip_debug_destroy_fs(void);

/*
@brief profile video memory allocate size
*/
vip_status_e gckvip_debug_videomemory_profile_alloc(
    vip_uint32_t size
    );

/*
@brief profile video memory free size
*/
vip_status_e gckvip_debug_videomemory_profile_free(
    void *handle
    );

/*
@brief record several times, and caculate total and infer time.
@param, gckvip_profile_dtype_t, the type of time.
*/
vip_status_e gckvip_debug_set_profile_data(
    vip_uint64_t time,
    gckvip_profile_dtype_t dtype
    );

vip_status_e gckvip_debug_profile_end(void);

vip_status_e gckvip_debug_profile_start(void);

/*
@brief get the profile result.
*/
gckvip_profile_t gckvip_debug_get_profile_data(void);
#endif
#endif

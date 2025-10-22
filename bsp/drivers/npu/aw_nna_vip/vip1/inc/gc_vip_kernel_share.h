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

#ifndef _GC_VIP_KERNEL_SHARE_H
#define _GC_VIP_KERNEL_SHARE_H
#include <gc_vip_common.h>

/* wrap user memory max number */
#define INIT_WRAP_USER_MEMORY_NUM            512

#if vpmdENABLE_RESERVE_PHYSICAL
#define MAX_WRAP_USER_PHYSICAL_TABLE_SIZE   512
#else
#define MAX_WRAP_USER_PHYSICAL_TABLE_SIZE   16384
#endif

/*
  the max value of command buffer size.
*/
#define gcdVIP_MAX_CMD_SIZE               (65535 * 8)

/*
  the size of memory alignment for video memory.
  64 bytes.
*/
#if ((64 % vpmdCPU_CACHE_LINE_SIZE) != 0)
#define gcdVIP_MEMORY_ALIGN_SIZE    GCVIP_ALIGN(GCVIP_ALIGN(64, vpmdCPU_CACHE_LINE_SIZE), 64)
#else
#define gcdVIP_MEMORY_ALIGN_SIZE    64
#endif

#if vpmdFPGA_BUILD || defined(_WIN32)
#define WAIT_TIME_EXE               200
#else
/* core freq=800M, wait=2us,  2000/1.25 = 1600 */
#define WAIT_TIME_EXE               1600
#endif
#define WAIT_LINK_SIZE              16
#define EVENT_SIZE                  8
#define LINK_SIZE                   8
#define END_SIZE                    8
#define BREAKPOINT_SIZE             8
#define SEMAPHORE_STALL             24
#define NOP_CMD_SIZE                8
#define QUICK_RESET_SIZE            16

/* IRQ_EVENT_ID value is 0 ~ 29 */
#define IRQ_EVENT_ID                4

#if vpmdENABLE_MMU
#define FLUSH_MMU_STATES_SIZE       24
#else
#define FLUSH_MMU_STATES_SIZE       0
#endif

#define CHIP_ENABLE_SIZE            16

#if vpmdENABLE_WAIT_LINK_LOOP
#if vpmdENABLE_POLLING
#define APPEND_COMMAND_SIZE (SEMAPHORE_STALL + WAIT_LINK_SIZE)
#else
#define APPEND_COMMAND_SIZE (SEMAPHORE_STALL + EVENT_SIZE + WAIT_LINK_SIZE)
#endif
#else
#if vpmdENABLE_POLLING
#define APPEND_COMMAND_SIZE (SEMAPHORE_STALL + END_SIZE)
#else
#define APPEND_COMMAND_SIZE (SEMAPHORE_STALL + EVENT_SIZE + END_SIZE)
#endif
#endif

#define MAX_PATCH_SIZE (APPEND_COMMAND_SIZE + CHIP_ENABLE_SIZE)

#if vpmdENABLE_HANG_DUMP || vpmdENABLE_CAPTURE
#define DUMP_REGISTER_ADDRESS_START 0
#define DUMP_REGISTER_ADDRESS_END 23
#define DUMP_REGISTER_CORE_ID_START 24
#define DUMP_REGISTER_CORE_ID_END 29
#define DUMP_REGISTER_TYPE_START 30
#define DUMP_REGISTER_TYPE_END 31
#endif

/* Kernel Command IDs. */
typedef enum _gckvip_command_id
{
    KERNEL_CMD_INIT     = 0,
    KERNEL_CMD_DESTROY  = 1,
    KERNEL_CMD_READ_REG = 2,
    KERNEL_CMD_WRITE_REG = 3,
    KERNEL_CMD_WAIT      = 4,
    KERNEL_CMD_SUBMIT    = 5,
    KERNEL_CMD_ALLOCATION = 6,
    KERNEL_CMD_DEALLOCATION = 7,
    KERNEL_CMD_RESET_VIP = 8,
    KERNEL_CMD_QUERY_ADDRESS_INFO = 9,
    KERNEL_CMD_QUERY_DATABASE = 10,
    KERNEL_CMD_OPERATE_CACHE = 11,
    KERNEL_CMD_WRAP_USER_MEMORY = 12,
    KERNEL_CMD_UNWRAP_USER_MEMORY = 13,
    KERNEL_CMD_QUERY_POWER_INFO = 14,
    KERNEL_CMD_SET_WORK_MODE = 15,
    KERNEL_CMD_POWER_MANAGEMENT = 16,
    KERNEL_CMD_WRAP_USER_PHYSICAL = 17,
    KERNEL_CMD_UNWRAP_USER_PHYSICAL = 18,
    KERNEL_CMD_WRAP_USER_FD = 19,
    KERNEL_CMD_UNWRAP_USER_FD = 20,
    KERNEL_CMD_QUERY_PROFILING = 21,
    KERNEL_CMD_QUERY_DRIVER_STATUS = 22,
    KERNEL_CMD_QUERY_MMU_INFO = 23,
    KERNEL_CMD_MAP_USER_LOGICAL = 24,
    KERNEL_CMD_UNMAP_USER_LOGICAL = 25,
    KERNEL_CMD_CANCEL = 26,
    KERNEL_CMD_QUERY_REGISTER_DUMP = 27,
    KERNEL_CMD_REGISTER_NETWORK_INFO = 28,
    KERNEL_CMD_REGISTER_SEGMENT_INFO = 29,
    KERNEL_CMD_UNREGISTER_NETWORK_INFO = 30,
    KERNEL_CMD_UNREGISTER_SEGMENT_INFO = 31,
    KERNEL_CMD_MAX,
} gckvip_command_id_e;

/* user space can specify video memory allocation flag */
typedef enum _gckvip_video_mem_alloc_flag
{
    GCVIP_VIDEO_MEM_ALLOC_NONE               = 0x00,
    GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS         = 0x01, /* Physical contiguous. */
    GCVIP_VIDEO_MEM_ALLOC_1M_CONTIGUOUS      = 0x02, /* allocate 1M bytes contiguous memory */
    GCVIP_VIDEO_MEM_ALLOC_NON_CONTIGUOUS     = 0x04, /* Physical non contiguous. */
    GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR           = 0x08, /* Need 32bit address. */
    GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE        = 0x10, /* without mmaped VIP's MMU page table */
    GCVIP_VIDEO_MEM_ALLOC_MAP_USER           = 0x20, /* need map user space logical address */
    GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE         = 0x40, /* allocate none-cache video memory */
    GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL = 0x200,/* need map kernel space logical address */
    GCVIP_VIDEO_MEM_ALLOC_MMU_PAGE_READ      = 0x400,/* if mmaped VIP's MMU page table, it's read only */
    GCVIP_VIDEO_MEM_ALLOC_MMU_PAGE_WRITE     = 0x800,/* if mmaped VIP's MMU page table, it's writable */
    GCVIP_VIDEO_MEM_ALLOC_MAX,
} gckvip_video_mem_alloc_flag_e;

typedef enum _gckvip_power_status
{
    GCKVIP_POWER_NONE       = 0,
    GCKVIP_POWER_OFF        = 1, /* power and clock off */
    GCKVIP_POWER_IDLE       = 2, /* slow down core freq */
    GCKVIP_POWER_ON         = 3, /* power and clock on, initialize hardware done */
    GCKVIP_POWER_SUSPEND    = 4, /* Linux system suspended status */
    GCKVIP_POWER_STOP       = 5, /* vip step */
    GCKVIP_POWER_MAX
} gckvip_power_status_e;

#if vpmdPOWER_OFF_TIMEOUT
typedef enum _gckvip_power_timer_status
{
    VIP_POWER_DISABLE_TIMER     = 0x10000,
    VIP_POWER_ENABLE_TIMER      = 0x20000,
} gckvip_power_timer_status;
#endif

typedef enum _gckvip_cache_type
{
    GCKVIP_CACHE_NONE           = 0,
    GCKVIP_CACHE_FLUSH          = 1,
    GCKVIP_CACHE_CLEAN          = 2,
    GCKVIP_CACHE_INVALID        = 3,
    GCKVIP_CACHE_MAX            = 4,
} gckvip_cache_type_e;

typedef enum _gckvip_work_mode_event
{
    GCKVIP_WORK_MODE_NORMAL     = 0,
    GCKVIP_WORK_MODE_SECURITY   = 1,
} gckvip_work_mode_e;

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
typedef enum _gckvip_register_dump_type
{
    GCKVIP_REGISTER_DUMP_READ = 0x00,
    GCKVIP_REGISTER_DUMP_WRITE = 0x01,
    GCKVIP_REGISTER_DUMP_WAIT = 0x02,
    GCKVIP_REGISTER_DUMP_UNKNOWN = 0x02
} gckvip_register_dump_type_e;
#endif
typedef struct _gckvip_initialize
{
    vip_uint32_t     video_mem_size;
    vip_uint32_t     version;
} gckvip_initialize_t;

typedef struct _gckvip_allocation
{
    vip_uint64_t    logical;    /* the logical address in user space */
    vip_uint32_t    handle;     /* video memory handle in kernel space */
    vip_uint32_t    physical;   /* the physical addess or VIP virtual */
    vip_uint32_t    size;
    vip_uint32_t    align;
    vip_uint32_t    alloc_flag; /* see gckvip_video_mem_alloc_flag_e */
} gckvip_allocation_t;

typedef struct _gckvip_deallocation
{
    vip_uint32_t    handle;  /* video memory handle in kernel space */
} gckvip_deallocation_t;

typedef struct _gckvip_map_user_logical
{
    vip_uint64_t    physical;    /* the physical addess or VIP virtual */
    vip_uint64_t    logical;     /* the logical address in user space */
    vip_uint32_t    handle;
    vip_bool_e      vip_virtual; /* indicate the physical addess is VIP virtual or not */
} gckvip_map_user_logical_t;

typedef struct _gckvip_unmap_user_logical
{
    /* the handle of video memory user space should be unmap */
    vip_uint32_t    handle;
} gckvip_unmap_user_logical_t;

typedef struct _gckvip_wrap_user_memory
{
    /* user space logical address */
    vip_uint64_t    logical;
    /* video memory handle */
    vip_uint32_t    handle;
    /* The type of this VIP buffer memory. see vip_buffer_memory_type_e*/
    vip_uint32_t    memory_type;
    /* the size of wrap memory */
    vip_uint32_t    size;
    /* VIP virtual memory */
    vip_uint32_t    virtual_addr;
} gckvip_wrap_user_memory_t;

typedef struct _gckvip_wrap_user_fd
{
    /* user space logical address */
    vip_uint32_t    fd;
    /* The type of this VIP buffer memory. see vip_buffer_memory_type_e*/
    vip_uint32_t    memory_type;
    /* the size of wrap memory */
    vip_uint32_t    size;
    /* VIP virtual memory */
    vip_uint32_t    virtual_addr;
    /* video memory handle */
    vip_uint32_t    handle;
    /* user space logical base address */
    vip_uint64_t    logical;
} gckvip_wrap_user_fd_t;

typedef struct _gckvip_wrap_user_physical
{
    /* cpu physical address table */
    vip_address_t   physical_table[MAX_WRAP_USER_PHYSICAL_TABLE_SIZE];
    /* the size of physical table each element */
    vip_uint32_t    size_table[MAX_WRAP_USER_PHYSICAL_TABLE_SIZE];
    /* the number of physical table element */
    vip_uint32_t    physical_num;
    /* The type of this VIP buffer memory. see vip_buffer_memory_type_e */
    vip_uint32_t    memory_type;
    /* video memory handle */
    vip_uint32_t    handle;
    /* VIP virtual memory */
    vip_uint32_t    virtual_addr;
    vip_uint64_t    logical;    /* the logical address in user space */
    vip_uint32_t    alloc_flag; /* see gckvip_video_mem_alloc_flag_e */
} gckvip_wrap_user_physical_t;

typedef struct _gckvip_unwrap_user_memory
{
    vip_uint32_t    handle;   /* video memory handle */
    vip_uint32_t    virtual_addr;  /* VIP virtual memory */
} gckvip_unwrap_user_memory_t;

typedef struct _gckvip_reg
{
    vip_uint32_t    reg;
    vip_uint32_t    core; /* the hardware core index */
    vip_uint32_t    data;
} gckvip_reg_t;

typedef struct _gckvip_wait
{
    vip_uint32_t    handle;
    vip_uint32_t    mask;
    vip_uint32_t    time_out;
    vip_uint32_t    device_id;
} gckvip_wait_t;

typedef struct _gckvip_cancel
{
    vip_uint32_t    handle;
    vip_uint32_t    device_id;
    vip_bool_e      hw_cancel;
} gckvip_cancel_t;

typedef struct _gckvip_commit
{
    vip_uint64_t    cmd_logical;
    vip_uint32_t    cmd_handle;
    vip_uint32_t    cmd_physical;
    vip_uint32_t    cmd_size;
    vip_uint64_t    last_cmd_logical;
    vip_uint32_t    last_cmd_handle;
    vip_uint32_t    last_cmd_physical;
    vip_uint32_t    last_cmd_size;
    vip_uint32_t    wait_event;
    vip_uint32_t    device_id;
#if vpmdENABLE_PREEMPTION
    vip_uint8_t     priority;
#endif
    vip_uint32_t    time_out;
} gckvip_commit_t;

typedef struct _gckvip_query_address_info
{
    /* the user space logical address of wait-link buffer */
    vip_uint64_t    waitlink_logical;
    /* the physical address of wait-link buffer */
    vip_uint32_t    waitlink_physical;
    /* the size of wait-link buffer */
    vip_uint32_t    waitlink_size;
} gckvip_query_address_info_t;

typedef enum _gckvip_log_level
{
    GCKVIP_ERROR = 0,
    GCKVIP_WARN = 1,
    GCKVIP_INFO = 2,
    GCKVIP_DEBUG = 3,
    GCKVIP_PRINT = 4,
} gckvip_log_level;

typedef struct _gckvip_feature_config
{
    /* mark capture function enable or not by fs node */
    vip_bool_e enable_capture;
    /* mark capture function enable or not by fs node */
    vip_bool_e enable_cnn_profile;
    /* mark capture function enable or not by fs node */
    vip_bool_e enable_dump_nbg;
} gckvip_feature_config_t;

typedef struct _gckvip_query_database
{
    vip_uint32_t    vip_sram_base;
    vip_uint32_t    vip_sram_size[gcdMAX_CORE];
    vip_uint32_t    axi_sram_base;
    vip_uint32_t    axi_sram_size[gcdMAX_CORE];
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    phy_address_t   video_heap_phy_base;
    vip_uint32_t    video_heap_vir_base;
    vip_uint32_t    video_heap_size;
#endif
#if vpmdENABLE_SYS_MEMORY_HEAP
    /* the size of system heap */
    vip_uint32_t    sys_heap_size;
#endif
    /* hardware customer ID */
    vip_uint32_t    hw_cid;
    vip_uint32_t    core_count;
    vip_uint32_t    max_core_count;
    /* the logical vip device number */
    vip_uint32_t    device_count;
    /* the core numbers for each logical device */
    vip_uint32_t    device_core_number[gcdMAX_CORE];
    /* the core index for each logical device */
    vip_uint32_t    device_core_index[gcdMAX_CORE][gcdMAX_CORE];
    /* function config by fs node */
    gckvip_feature_config_t func_config;
} gckvip_query_database_t;

typedef struct _gckvip_query_driver_status
{
    vip_uint32_t    device_id;
    /* the number of user space commit command buffer */
    vip_uint32_t    commit_count;

    /* hardware initialize commands */
    vip_uint32_t    init_cmd_handle;
    vip_uint32_t    init_cmd_size;
    vip_uint32_t    init_cmd_physical;
} gckvip_query_driver_status_t;

typedef struct _gckvip_operate_cache
{
    /* user logical address  */
    vip_uint64_t    logical;
    /* video memory handle should be flushed */
    vip_uint32_t    handle;
    /* when MMU is enabled, physical is virtual address. otherwise physical is CPU's physical */
    vip_uint32_t    physical;
    /* the size of memory should be flushed */
    vip_uint32_t    size;
    /* operate CPU cache type, flush/clean/invalid */
    gckvip_cache_type_e type;
} gckvip_operate_cache_t;

typedef struct _gckvip_query_power_info
{
    vip_uint32_t   device_id;
    vip_uint8_t    power_status;
} gckvip_query_power_info_t;

typedef struct _gckvip_set_work_mode
{
    vip_uint8_t    work_mode;
} gckvip_set_work_mode_t;

typedef struct _gckvip_power_management
{
    /* see vip_power_property_e */
    vip_enum       property;
    /* only for property is VIP_POWER_PROPERTY_SET_FREQUENCY.
        other property, fscale_percent is useless */
    vip_uint8_t    fscale_percent;
    vip_uint32_t   device_id;
} gckvip_power_management_t;

typedef struct _gckvip_query_mmu_info
{
    vip_uint32_t    entry_handle;
    vip_uint32_t    mtlb_handle;
    vip_uint32_t    stlb_1m_handle;
    vip_uint32_t    stlb_4k_handle;
    vip_uint32_t    entry_size;
    vip_uint32_t    mtlb_size;
    vip_uint32_t    stlb_1m_size;
    vip_uint32_t    stlb_4k_size;
    phy_address_t   entry_physical;
    phy_address_t   mtlb_physical;
    phy_address_t   stlb_1m_physical;
    phy_address_t   stlb_4k_physical;
} gckvip_query_mmu_info_t;

typedef struct _gckvip_query_profiling
{
    /* video memory handle */
    vip_uint32_t   device_id;
    vip_uint32_t   cmd_handle;
    vip_uint32_t   total_cycle;
    vip_uint32_t   total_idle_cycle;
    /* inference_time xxx us */
    vip_uint32_t   inference_time;
} gckvip_query_profiling_t;

typedef struct _gckvip_query_register_dump
{
    phy_address_t physical;
    vip_uint32_t handle;
    vip_uint32_t count;
} gckvip_query_register_dump_t;

#define NETWORK_NAME_SIZE 64
typedef struct _gckvip_register_network
{
    vip_uint32_t network_handle;
    vip_char_t   network_name[NETWORK_NAME_SIZE];
    vip_uint32_t segment_count;
} gckvip_register_network_t;

typedef struct _gckvip_register_segment
{
    vip_uint32_t segment_handle;
    vip_uint32_t network_handle;
    vip_uint32_t segment_index;
    vip_uint32_t operation_start;
    vip_uint32_t operation_end;
} gckvip_register_segment_t;

typedef struct _gckvip_unregister
{
    vip_uint32_t    handle;
} gckvip_unregister_t;

#if (vpmdENABLE_DEBUG_LOG >= 0)
static const vip_char_t ioctl_cmd_string[KERNEL_CMD_MAX][48] = {
    "CMD_INIT",
    "CMD_DESTROY",      /* 1 */
    "CMD_READ_REG",     /* 2 */
    "CMD_WRITE_REG",    /* 3 */
    "CMD_WAIT",         /* 4 */
    "CMD_SUBMIT",       /* 5 */
    "CMD_ALLOCATION",   /* 6 */
    "CMD_DEALLOCATION", /* 7 */
    "CMD_RESET_VIP",    /* 8 */
    "CMD_QUERY_ADDRESS_INFO",/* 9 */
    "CMD_QUERY_DATABASE",    /* 10 */
    "CMD_OPERATE_CACHE",     /* 11 */
    "CMD_WRAP_USER_MEMORY",  /* 12 */
    "CMD_UNWRAP_USER_MEMORY",/* 13 */
    "CMD_QUERY_POWER_INFO",  /* 14 */
    "CMD_SET_WORK_MODE",     /* 15 */
    "CMD_POWER_MANAGEMENT",   /* KERNEL_CMD_POWER_MANAGEMENT */
    "CMD_WRAP_USER_PHYSICAL", /* KERNEL_CMD_WRAP_USER_PHYSICAL */
    "CMD_UNWRAP_USER_PHYSICAL",/* KERNEL_CMD_UNWRAP_USER_PHYSICAL */
    "CMD_WRAP_USER_FD",        /* 19 */
    "CMD_UNWRAP_USER_FD",      /* 20 */
    "CMD_QUERY_PROFILING",     /* 21 */
    "CMD_QUERY_DRIVER_STATUS", /* 22 */
    "CMD_QUERY_MMU_INFO",      /* 23 */
    "CMD_MAP_USER_LOGICAL",    /* 24 */
    "CMD_UNMAP_USER_LOGICAL",  /* 25 */
    "CMD_CANCEL",              /* 26 */
    "CMD_QUERY_REGISTER_DUMP", /* 27 */
    "CMD_REGISTER_NETWORK_INFO",   /* 28 */
    "CMD_REGISTER_SEGMENT_INFO",   /* 29 */
    "CMD_UNREGISTER_NETWORK_INFO", /* 30 */
    "CMD_UNREGISTER_SEGMENT_INFO"  /* 31 */
};
#endif

#if (vpmdENABLE_DEBUG_LOG >= 3)
static const vip_char_t pm_status_str[GCKVIP_POWER_MAX][24] = {
    "NONE",
    "OFF",
    "IDLE",
    "ON",
    "SUSPEND",
    "STOP"
};
#endif

#endif

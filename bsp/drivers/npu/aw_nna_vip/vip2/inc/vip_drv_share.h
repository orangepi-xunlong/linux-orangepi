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

/*
  These defines are sharing with user space and kernel space souce code.
  It's can be included by toth user space and vip-drv space.
*/

#ifndef _VIP_DRV_SHARE_H
#define _VIP_DRV_SHARE_H
#include <vip_lite_config.h>

#define MAX_FREQ_POINTS 16

#if vpmdENABLE_RESERVE_PHYSICAL
#define MAX_WRAP_USER_PHYSICAL_TABLE_SIZE   512
#else
#define MAX_WRAP_USER_PHYSICAL_TABLE_SIZE   16384
#endif

/*
  the size of memory alignment for video memory.
  64 bytes.
*/
#if ((64 % vpmdCPU_CACHE_LINE_SIZE) != 0)
#define vipdMEMORY_ALIGN_SIZE    VIP_ALIGN(VIP_ALIGN(64, vpmdCPU_CACHE_LINE_SIZE), 64)
#else
#define vipdMEMORY_ALIGN_SIZE    64
#endif

#if vpmdFPGA_BUILD || defined(_WIN32)
#define WAIT_TIME_EXE               200
#else
/* core freq=800M, wait=2us,  2000/1.25 = 1600 */
#define WAIT_TIME_EXE               1600
#endif
#define WAIT_SIZE                   8
#define LINK_SIZE                   8
#define WAIT_LINK_SIZE              (WAIT_SIZE + LINK_SIZE)
#define EVENT_SIZE                  8
#define END_SIZE                    8
#define BREAKPOINT_SIZE             8
#define SEMAPHORE_STALL             24
#define NOP_CMD_SIZE                8
#define QUICK_RESET_SIZE            16
#define STALL_FE_SIZE               (8 + 16)

/* IRQ_EVENT_ID value is 0 ~ 29 */
#define IRQ_EVENT_ID                4

#if vpmdENABLE_MMU
#define FLUSH_MMU_STATES_SIZE       24
#else
#define FLUSH_MMU_STATES_SIZE       0
#endif

#define CHIP_ENABLE_SIZE            8

#if vpmdTASK_PARALLEL_ASYNC
#define APPEND_LINK_SIZE    (vipdMAX_CORE * (CHIP_ENABLE_SIZE + WAIT_LINK_SIZE))
#define APPEND_COMMAND_SIZE (SEMAPHORE_STALL + CHIP_ENABLE_SIZE + EVENT_SIZE + CHIP_ENABLE_SIZE + APPEND_LINK_SIZE)
#else
#if vpmdENABLE_POLLING
#define APPEND_COMMAND_SIZE (SEMAPHORE_STALL + END_SIZE)
#else
#define APPEND_COMMAND_SIZE (SEMAPHORE_STALL + CHIP_ENABLE_SIZE + EVENT_SIZE + CHIP_ENABLE_SIZE + END_SIZE)
#endif
#endif

#if vpmdENABLE_HANG_DUMP || vpmdENABLE_CAPTURE
#define DUMP_REGISTER_ADDRESS_START 0
#define DUMP_REGISTER_ADDRESS_END 23
#define DUMP_REGISTER_CORE_ID_START 24
#define DUMP_REGISTER_CORE_ID_END 29
#define DUMP_REGISTER_TYPE_START 30
#define DUMP_REGISTER_TYPE_END 31
#endif

#ifndef vpmdENABLE_USE_AGENT_TRIGGER
#define vpmdENABLE_USE_AGENT_TRIGGER        0
#endif

/*
  the max value of command buffer size. PLEASE DON'T INCREASE THIS VALUE.
*/
#define vipdMAX_CMD_SIZE               (65535 * 8)


/* vip-drv interface command ID. */
typedef enum _vipdrv_intrface
{
    VIPDRV_INTRFACE_INIT     = 0,
    VIPDRV_INTRFACE_DESTROY  = 1,
    VIPDRV_INTRFACE_READ_REG = 2,
    VIPDRV_INTRFACE_WRITE_REG = 3,
    VIPDRV_INTRFACE_WAIT_TASK  = 4,
    VIPDRV_INTRFACE_SUBMIT_TASK = 5,
    VIPDRV_INTRFACE_ALLOCATION = 6,
    VIPDRV_INTRFACE_DEALLOCATION = 7,
    VIPDRV_INTRFACE_NOT_USED = 8, /* NOT USED FIELD */
    VIPDRV_INTRFACE_QUERY_ADDRESS_INFO = 9,
    VIPDRV_INTRFACE_QUERY_DATABASE = 10,
    VIPDRV_INTRFACE_OPERATE_CACHE = 11,
    VIPDRV_INTRFACE_WRAP_USER_MEMORY = 12,
    VIPDRV_INTRFACE_UNWRAP_USER_MEMORY = 13,
    VIPDRV_INTRFACE_NOT_USED2 = 14,
    VIPDRV_INTRFACE_POWER_MANAGEMENT = 15,
    VIPDRV_INTRFACE_WRAP_USER_PHYSICAL = 16,
    VIPDRV_INTRFACE_UNWRAP_USER_PHYSICAL = 17,
    VIPDRV_INTRFACE_WRAP_USER_FD = 18,
    VIPDRV_INTRFACE_UNWRAP_USER_FD = 19,
    VIPDRV_INTRFACE_QUERY_PROFILING = 20,
    VIPDRV_INTRFACE_QUERY_DRIVER_STATUS = 21,
    VIPDRV_INTRFACE_QUERY_MMU_INFO = 22,
    VIPDRV_INTRFACE_MAP_USER_LOGICAL = 23,
    VIPDRV_INTRFACE_UNMAP_USER_LOGICAL = 24,
    VIPDRV_INTRFACE_CANCEL_TASK = 25,
    VIPDRV_INTRFACE_QUERY_REGISTER_DUMP = 26,
    VIPDRV_INTRFACE_CREATE_TASK = 27,
    VIPDRV_INTRFACE_SET_TASK_PROPERTY = 28,
    VIPDRV_INTRFACE_DESTROY_TASK = 29,
    VIPDRV_INTRFACE_MAX,
} vipdrv_intrface_e;

/* user space can specify video memory allocation flag */
typedef enum _vipdrv_video_mem_alloc_flag
{
    VIPDRV_VIDEO_MEM_ALLOC_NONE           = 0x000,
    VIPDRV_VIDEO_MEM_ALLOC_64K_CONTIGUOUS = 0x001, /* allocate 64K bytes contiguous memory */
    VIPDRV_VIDEO_MEM_ALLOC_1M_CONTIGUOUS  = 0x002, /* allocate 1M bytes contiguous memory */
    VIPDRV_VIDEO_MEM_ALLOC_16M_CONTIGUOUS = 0x004, /* allocate 16M bytes contiguous memory */
    VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS     = 0x008, /* physical contiguous. */
    VIPDRV_VIDEO_MEM_ALLOC_4GB_ADDR       = 0x010, /* need 32bit address. limit virtual when mmu enable, or physical */
    /* following flag is extra action */
    VIPDRV_VIDEO_MEM_ALLOC_NPU_READ_ONLY  = 0x040, /* if mmaped VIP's MMU page table, this page is read only for NPU */
    VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE    = 0x080, /* without mmaped VIP's MMU page table */
    VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE     = 0x100, /* allocate none-cache video memory */
    VIPDRV_VIDEO_MEM_ALLOC_MAP_USER       = 0x200, /* need map user space logical address */
    VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL     = 0x400, /* need map kernel space logical address */
    VIPDRV_VIDEO_MEM_ALLOC_MMU_PAGE_MEM   = 0x800, /* indicate this memory is for mmu page */
} vipdrv_video_mem_alloc_flag_e;

typedef enum _vipdrv_power_property_e
{
    VIPDRV_POWER_PROPERTY_NONE          = 0x0000,
    /*!< \brief specify the VIP frequency */
    VIPDRV_POWER_PROPERTY_SET_FREQUENCY = 0x0001,
    /*!< \brief power off VIP hardware */
    VIPDRV_POWER_PROPERTY_OFF           = 0x0002,
    /*!< \brief power on VIP hardware */
    VIPDRV_POWER_PROPERTY_ON            = 0x0004,
    /*!< \brief stop VIP perform network */
    VIPDRV_POWER_PROPERTY_STOP          = 0x0008,
    /*!< \brief start VIP perform network */
    VIPDRV_POWER_PROPERTY_START         = 0x0010,
    VIPDRV_POWER_DISABLE_TIMER          = 0x10000,
    VIPDRV_POWER_ENABLE_TIMER           = 0x20000,
} vipdrv_power_property_e;

typedef enum _vipdrv_cache_type
{
    VIPDRV_CACHE_NONE           = 0,
    VIPDRV_CACHE_FLUSH          = 1,
    VIPDRV_CACHE_CLEAN          = 2,
    VIPDRV_CACHE_INVALID        = 3,
    VIPDRV_CACHE_MAX            = 4,
} vipdrv_cache_type_e;

typedef enum _vipdrv_work_mode_event
{
    VIPDRV_WORK_MODE_NORMAL     = 0,
    VIPDRV_WORK_MODE_SECURITY   = 1,
} vipdrv_work_mode_e;

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
typedef enum _vipdrv_register_dump_type
{
    VIPDRV_REGISTER_DUMP_READ = 0x00,
    VIPDRV_REGISTER_DUMP_WRITE = 0x01,
    VIPDRV_REGISTER_DUMP_WAIT = 0x02,
    VIPDRV_REGISTER_DUMP_UNKNOWN = 0x02
} vipdrv_register_dump_type_e;
#endif

typedef enum _vipdrv_task_cmd_type_e
{
    VIPDRV_TASK_CMD_SUBTASK = 0,
    VIPDRV_TASK_CMD_INIT    = 1,
    VIPDRV_TASK_CMD_PRELOAD = 2,
} vipdrv_task_cmd_type_e;

typedef enum _vipdrv_task_patch_type_e
{
    VIPDRV_TASK_PATCH_NONE = 0,
    VIPDRV_TASK_PATCH_SYNC = 1,
    VIPDRV_TASK_PATCH_CHIP = 2,
    VIPDRV_TASK_PATCH_DUP  = 3,
} vipdrv_task_patch_type_e;

typedef struct _vipdrv_initialize
{
    vip_uint32_t     video_mem_size;
    vip_uint32_t     version;
} vipdrv_initialize_t;

typedef struct _vipdrv_allocation
{
    vip_uint64_t    logical;    /* the logical address in user space */
    vip_uint32_t    mem_id;     /* id of the video memory in vip-drv */
    vip_address_t   physical;   /* the physical addess or VIP virtual */
    vip_size_t      size;
    vip_uint32_t    align;
    vip_uint32_t    alloc_flag; /* see vipdrv_video_mem_alloc_flag_e */
    vip_uint64_t    device_vis; /* indicate which device can access this memory */
    vip_uint8_t     specified;  /* specify the VIP virtual or physical address. */
} vipdrv_allocation_t;

typedef struct _vipdrv_deallocation
{
    vip_uint32_t    mem_id;  /* the id of video memory in vip-drv */
} vipdrv_deallocation_t;

typedef struct _vipdrv_map_user_logical
{
    vip_uint64_t    logical;     /* the logical address in user space */
    vip_uint32_t    mem_id;
} vipdrv_map_user_logical_t;

typedef struct _vipdrv_unmap_user_logical
{
    /* the id of video memory user space should be unmap */
    vip_uint32_t    mem_id;
} vipdrv_unmap_user_logical_t;

typedef struct _vipdrv_wrap_user_memory
{
    /* user space logical address */
    vip_uint64_t    logical;
    /* the id of video memory */
    vip_uint32_t    mem_id;
    /* The type of this VIP buffer memory. see vip_buffer_memory_type_e*/
    vip_uint32_t    memory_type;
    /* the size of wrap memory */
    vip_size_t      size;
    /* indicate which device can access this memory */
    vip_uint64_t    device_vis;
    /* VIP virtual memory */
    vip_address_t   virtual_addr;
} vipdrv_wrap_user_memory_t;

typedef struct _vipdrv_wrap_user_fd
{
    /* user space logical address */
    vip_uint32_t    fd;
    /* The type of this VIP buffer memory. see vip_buffer_memory_type_e*/
    vip_uint32_t    memory_type;
    /* the size of wrap memory */
    vip_size_t      size;
    /* indicate which device can access this memory */
    vip_uint64_t    device_vis;
    /* VIP virtual memory */
    vip_address_t   virtual_addr;
    /* the id of the video memory */
    vip_uint32_t    mem_id;
    /* user space logical base address */
    vip_uint64_t    logical;
} vipdrv_wrap_user_fd_t;

typedef struct _vipdrv_wrap_user_physical
{
    /* cpu physical address table */
    vip_address_t   physical_table[MAX_WRAP_USER_PHYSICAL_TABLE_SIZE];
    /* the size of physical table each element */
    vip_size_t      size_table[MAX_WRAP_USER_PHYSICAL_TABLE_SIZE];
    /* the number of physical table element */
    vip_uint32_t    physical_num;
    /* the type of this VIP buffer memory. see vip_buffer_memory_type_e */
    vip_uint32_t    memory_type;
    /* indicate which device can access this memory */
    vip_uint64_t    device_vis;
    /* the id of video memory */
    vip_uint32_t    mem_id;
    /* VIP virtual memory */
    vip_address_t   virtual_addr;
    vip_uint64_t    logical;    /* the logical address in user space */
    vip_uint32_t    alloc_flag; /* see vipdrv_video_mem_alloc_flag_e */
} vipdrv_wrap_user_physical_t;

typedef struct _vipdrv_reg
{
    vip_uint32_t    dev_index;  /* the index of device */
    vip_uint32_t    core_index; /* the index hardware core in device */
    vip_uint32_t    reg;        /* register address */
    vip_uint32_t    data; /* read/wrote data of the register */
} vipdrv_reg_t;

typedef struct _vipdrv_query_address_info
{
    /* the user space logical address of wait-link buffer */
    vip_uint64_t    waitlink_logical;
    /* the physical address of wait-link buffer */
    vip_uint32_t    waitlink_physical;
    /* the size of wait-link buffer */
    vip_uint32_t    waitlink_size;
} vipdrv_query_address_info_t;

typedef struct _vipdrv_feature_config
{
    /* mark capture function enable or not by fs node */
    vip_bool_e enable_capture;
    /* mark capture function enable or not by fs node */
    vip_bool_e enable_cnn_profile;
    /* mark capture function enable or not by fs node */
    vip_bool_e enable_dump_nbg;
} vipdrv_feature_config_t;

typedef struct _vipdrv_query_database
{
    vip_uint32_t    vip_sram_base;
    vip_uint32_t    vip_sram_size[vipdMAX_CORE];
    /* mmu enabled: the VIP's virtual address, mmu disabled: the VIP's physical address */
    vip_virtual_t   axi_sram_base;
    vip_uint32_t    axi_sram_size[vipdMAX_CORE];
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
    vip_uint32_t    device_core_number[vipdMAX_CORE];
    /* the core id for each logical device */
    vip_uint32_t    device_core_id[vipdMAX_CORE][vipdMAX_CORE];
    /* function config by fs node */
    vipdrv_feature_config_t func_config;
} vipdrv_query_database_t;

typedef struct _vipdrv_query_driver_status
{
    vip_uint32_t    device_index;
    /* the number of user space commit command buffer */
    vip_uint32_t    commit_count;

    /* hardware initialize commands. init states command in vipdrv */
    vip_uint32_t    init_cmd_handle;
    vip_uint32_t    init_cmd_size;
    vip_uint32_t    init_cmd_physical;
} vipdrv_query_driver_status_t;

typedef struct _vipdrv_operate_cache
{
    /* the if of video memory should be flushed */
    vip_uint32_t    mem_id;
    /* operate CPU cache type, flush/clean/invalid */
    vipdrv_cache_type_e type;
} vipdrv_operate_cache_t;

typedef struct _vipdrv_power_management
{
    /* see vip_power_property_e */
    vip_enum       property;
    /* only for property is VIP_POWER_PROPERTY_SET_FREQUENCY.
        other property, fscale_percent is useless */
    vip_uint8_t    fscale_percent;
    vip_uint32_t   device_index;
} vipdrv_power_management_t;

#define MAX_STLB_MEM_CNT 16
typedef struct _vipdrv_query_mmu_info
{
    vip_uint32_t    entry_mem_id;
    vip_uint32_t    mtlb_mem_id;
    vip_uint32_t    stlb_mem_id[MAX_STLB_MEM_CNT];
    vip_uint32_t    entry_size;
    vip_uint32_t    mtlb_size;
    vip_size_t      stlb_size[MAX_STLB_MEM_CNT];
    vip_physical_t   entry_physical;
    vip_physical_t   mtlb_physical;
    vip_physical_t   stlb_physical[MAX_STLB_MEM_CNT];
} vipdrv_query_mmu_info_t;

typedef struct _vipdrv_query_profiling
{
    vip_uint32_t   task_id;
    vip_uint32_t   subtask_index;
    /* return values cycle */
    vip_uint32_t   total_cycle[vipdMAX_CORE];
    vip_uint32_t   total_idle_cycle[vipdMAX_CORE];
    /* return bw */
    vip_uint32_t   total_read[vipdMAX_CORE];
    vip_uint32_t   total_write[vipdMAX_CORE];
    vip_uint32_t   total_read_ocb[vipdMAX_CORE];
    vip_uint32_t   total_write_ocb[vipdMAX_CORE];
    /* inference_time xxx us */
    vip_uint32_t   inference_time;
} vipdrv_query_profiling_t;

typedef struct _vipdrv_query_register_dump
{
    vip_physical_t physical;
    vip_uint32_t mem_id;
    vip_uint32_t count;
} vipdrv_query_register_dump_t;

#define TASK_NAME_LENGTH    16
typedef struct _vipdrv_create_task_param
{
    /* the id of task, return a unique task id after creating task */
    vip_uint32_t task_id;
    /* the name of task */
    vip_char_t   task_name[TASK_NAME_LENGTH];
    /* the maximium count of sub task */
    vip_uint32_t subtask_count;
} vipdrv_create_task_param_t;

typedef struct _vipdrv_destroy_task_param
{
    vip_uint32_t task_id;
} vipdrv_destroy_task_param_t;

typedef struct _vipdrv_set_task_property
{
    /* the id of task should be set property */
    vip_uint32_t task_id;
    /* the type of task command, vipdrv_task_cmd_type_e */
    vip_uint32_t type;
    /* the index of the task cmd type */
    vip_uint32_t index;
    /* the type of patch, vipdrv_task_patch_type_e */
    vip_uint32_t patch_type;
    union {
        struct {
            /* a mem_id of video memory for subtask command */
            vip_uint32_t subtask_id;
            /* the real size of command for subtask */
            vip_uint32_t size;
            /* the patch count of current command */
            vip_uint32_t patch_count;
            /* the index of core for the subtask default runs on */
            vip_uint32_t core_index;
        } subtask_info;

        struct {
            /* the patch index */
            vip_uint32_t patch_index;
            /* the size of command will be patched */
            vip_uint32_t size;
            /* the index of core in all used cores */
            vip_uint32_t core_index;
            /* the core count of this patch uses */
            vip_uint32_t core_count;
            /* the offset in command */
            vip_uint32_t offset;
        } patch_info;

        struct {
            /* the id of task should be set property */
            vip_uint32_t task_id;
            /* the type of task command, vipdrv_task_cmd_type_e */
            vip_uint32_t type;
            /* the index of the task cmd type */
            vip_uint32_t index;
            /* the type of patch, vipdrv_task_patch_type_e */
        } dup_info;
    } param;
} vipdrv_set_task_property_t;

typedef struct _vipdrv_submit_task_param
{
    vip_uint32_t                 task_id;
    vip_uint32_t                 subtask_index;
    vip_uint32_t                 subtask_cnt;
    vip_uint32_t                 device_index;
    vip_uint32_t                 core_index;
    vip_uint32_t                 core_cnt;
#if vpmdENABLE_PREEMPTION
    vip_uint8_t                  priority;
#endif
    vip_uint32_t                 time_out;
    vip_uint8_t                  need_schedule;
} vipdrv_submit_task_param_t;

typedef struct _vipdrv_wait_task_param
{
    vip_uint32_t    task_id;
    vip_uint32_t    subtask_index;
} vipdrv_wait_task_param_t;

typedef struct _vipdrv_cancel_task_param
{
    vip_uint32_t    task_id;
    vip_uint32_t    subtask_index;
} vipdrv_cancel_task_param_t;

typedef enum _vip_hardware_feature_e
{
    VIP_HW_FEATURE_JOB_CANCEL                 = 0,
    VIP_HW_FEATURE_MMU_PDMODE                 = 1,
    VIP_HW_FEATURE_FREQ_SCALE                 = 2,
    VIP_HW_FEATURE_NN_ERROR_DETECT            = 3,
    VIP_HW_FEATURE_KENERL_DESC                = 4,
    VIP_HW_FEATURE_NN_TRANS_PHASE2            = 5,
    VIP_HW_FEATURE_NN_CORE_CNT                = 6,
    VIP_HW_FEATURE_TP_CORE_CNT                = 7,
    VIP_HW_FEATURE_VIPSRAM_WIDTH              = 8,
    VIP_HW_FEATURE_LATENCY_HIDING_FULL_AXI_BW = 9,
    VIP_HW_FEATURE_MIN_AXI_BURST_SIZE         = 10,
    VIP_HW_FEATURE_DDR_KERNEL_BURST_SIZE      = 11,
    VIP_HW_FEATURE_AXI_BUS_WIDTH              = 12,
    VIP_HW_FEATURE_OCB_COUNTER                = 13,
    VIP_HW_FEATURE_NN_PROBE                   = 14,
    VIP_HW_FEATURE_SH_CONFORMANCE_BRUTEFORCE  = 15,
    VIP_HW_FEATURE_MMU                        = 16,
    VIP_HW_FEATURE_MMU_40VA                   = 17,
    VIP_HW_FEATURE_NN_XYDP0                   = 18,
    VIP_HW_FEATURE_MAX
} vip_hardware_feature_e;

#if (vpmdENABLE_DEBUG_LOG >= 0)
static const vip_char_t ioctl_cmd_string[VIPDRV_INTRFACE_MAX + 1][48] = {
    "VIPDRV_INIT",
    "VIPDRV_DESTROY",      /* 1 */
    "VIPDRV_READ_REG",     /* 2 */
    "VIPDRV_WRITE_REG",    /* 3 */
    "VIPDRV_WAIT_TASK",    /* 4 */
    "VIPDRV_SUBMIT_TASK",  /* 5 */
    "VIPDRV_ALLOCATION",   /* 6 */
    "VIPDRV_DEALLOCATION", /* 7 */
    "VIPDRV_NOT_USED",    /* 8 */
    "VIPDRV_QUERY_ADDRESS_INFO",/* 9 */
    "VIPDRV_QUERY_DATABASE",    /* 10 */
    "VIPDRV_OPERATE_CACHE",     /* 11 */
    "VIPDRV_WRAP_USER_MEMORY",  /* 12 */
    "VIPDRV_UNWRAP_USER_MEMORY",/* 13 */
    "VIPDRV_NOT_USED",     /* 14 */
    "VIPDRV_POWER_MANAGEMENT",   /* VIPDRV_INTRFACE_POWER_MANAGEMENT */
    "VIPDRV_WRAP_USER_PHYSICAL", /* VIPDRV_INTRFACE_WRAP_USER_PHYSICAL */
    "VIPDRV_UNWRAP_USER_PHYSICAL",/* VIPDRV_INTRFACE_UNWRAP_USER_PHYSICAL */
    "VIPDRV_WRAP_USER_FD",        /* 18 */
    "VIPDRV_UNWRAP_USER_FD",      /* 19 */
    "VIPDRV_QUERY_PROFILING",     /* 20 */
    "VIPDRV_QUERY_DRIVER_STATUS", /* 21 */
    "VIPDRV_QUERY_MMU_INFO",      /* 22 */
    "VIPDRV_MAP_USER_LOGICAL",    /* 23 */
    "VIPDRV_UNMAP_USER_LOGICAL",  /* 24 */
    "VIPDRV_CANCEL",              /* 25 */
    "VIPDRV_QUERY_REGISTER_DUMP", /* 26 */
    "VIPDRV_CREATE_TASK",         /* 27 */
    "VIPDRV_SET_TASK_PROPERTY",   /* 28 */
    "VIPDRV_DESTROY_TASK",        /* 29 */
    "VIPDRV_INTRFACE_MAX"
};
#endif
#endif

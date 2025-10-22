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

#ifndef _GC_VIP_KERNEL_H
#define _GC_VIP_KERNEL_H

#include <gc_vip_common.h>
#include <gc_vip_kernel_heap.h>
#include <gc_vip_kernel_util.h>
#include <gc_vip_kernel_share.h>
#include <gc_vip_kernel_port.h>

#define GCKVIP_CHECK_CORE(core)                                                                 \
{                                                                                               \
    gckvip_context_t *kcontext = gckvip_get_context();                                          \
    if ((core) > kcontext->core_count) {                                                        \
        PRINTK_E("failed core=%d is larger than core count=%d\n", core, kcontext->core_count);  \
        GCKVIP_DUMP_STACK();                                                                    \
        gcGoOnError(VIP_ERROR_FAILURE);                                                         \
    }                                                                                           \
}

#define GCKVIP_CHECK_DEV(dev)                                                         \
{                                                                                     \
    gckvip_context_t *kcontext = gckvip_get_context();                                \
    if ((dev) >= kcontext->device_count) {                                            \
        PRINTK_E("failed device id =%d is larger than device count=%d\n",             \
                  dev, kcontext->device_count);                                       \
        GCKVIP_DUMP_STACK();                                                          \
        gcGoOnError(VIP_ERROR_FAILURE);                                               \
    }                                                                                 \
}

/*
@brief define loop fun for each vip core
*/
#define GCKVIP_LOOP_HARDWARE_START                                      \
{  vip_uint32_t hw_index = 0;                                           \
   gckvip_context_t *kcontext = gckvip_get_context();                   \
   for (hw_index = 0 ; hw_index < kcontext->core_count; hw_index++) {   \
       gckvip_hardware_t *hw = gckvip_get_hardware(hw_index);           \
       if (hw != VIP_NULL) {                                            \

#define GCKVIP_LOOP_HARDWARE_END }}}

/*
@brief define loop fun for each vip device
*/
#define GCKVIP_LOOP_DEVICE_START                            \
{  vip_uint32_t dev_index = 0;                              \
   gckvip_context_t *kcontext = gckvip_get_context();       \
   for (dev_index = 0 ; dev_index < kcontext->device_count; dev_index++) {   \
        gckvip_device_t *device = &kcontext->device[dev_index];

#define GCKVIP_LOOP_DEVICE_END }}

#define HARDWARE_BYPASS_SUBMIT()                            \
{                                                           \
    *hardware->irq_value = 0x10;                            \
    gckvip_os_wakeup_interrupt(hardware->irq_queue);        \
    PRINTK_E("hardware bypass submit.\n");\
}

typedef enum _gckvip_task_status_e
{
    GCKVIP_TASK_NONE           = 0,
    GCKVIP_TASK_EMPTY          = 1,
    /* task has been submit to queue */
    GCKVIP_TASK_WITH_DATA      = 2,
    /* task has been submit to HW */
    GCKVIP_TASK_INFER_START    = 3,
    /* application has waiting for task done */
    GCKVIP_TASK_USER_WAIT      = 4,
    /* wait irq return, inference done */
    GCKVIP_TASK_INFER_END      = 5,
    /* the task has been canceled */
    GCKVIP_TASK_CANCELED       = 6,
    GCKVIP_TASK_MAX,
} gckvip_task_status_e;

typedef struct _gckvip_submit
{
    /* first command buffer */
    void            *cmd_handle;
    vip_uint8_t     *cmd_logical;
    vip_uint32_t    cmd_physical;
    vip_uint32_t    cmd_size;

    /*
    last command buffer. it is used to support submit multiple command buffer.
    If only one command buffer submit, you can set cmd_xxx equal to last_cmd_xxx.
    */
    void            *last_cmd_handle;
    vip_uint8_t     *last_cmd_logical;
    vip_uint32_t    last_cmd_physical;
    vip_uint32_t    last_cmd_size;
    /*
    submit command and sync multiple core when combine mode
    */
    vip_bool_e      wait_event;
    vip_uint32_t    device_id;
#if vpmdENABLE_MULTIPLE_TASK
    vip_uint32_t    pid;
    vip_uint32_t    time_out;
#endif
} gckvip_submit_t;

typedef struct _gckvip_wait_cmd
{
    void            *cmd_handle;
    vip_uint32_t    mask;
    vip_uint32_t    time_out;
} gckvip_wait_cmd_t;

typedef struct _gckvip_task_slot
{
    /* gckvip_task_status_e */
    volatile vip_uint8_t              status;
    volatile vip_uint8_t              inference_running;
    gckvip_submit_t                   submit_handle;
    gckvip_mutex                      cancel_mutex;

    /* for multi-task enabled */
#if vpmdENABLE_MULTIPLE_TASK
    gckvip_queue_data_t               *queue_data;
    gckvip_signal                     wait_signal;
#endif
} gckvip_task_slot_t;

typedef struct _gckvip_video_memory
{
    void            *handle;
    vip_uint32_t    physical;
    /* the kernel space cpu logical address */
    void            *logical;
    /* the size of this memory */
    vip_uint32_t    size;
} gckvip_video_memory_t;

typedef struct _gckvip_video_memory_ext
{
    /* the video memory handle */
    void            *handle;
    /* vip virtual address when mmu is enabled*/
    vip_uint32_t    physical;
    /* the kernel space logical address */
    void            *logical;
    vip_uint32_t    size;
    /* the id of video memory managment by video memory database */
    vip_uint32_t     handle_id;
} gckvip_video_memory_ext_t;

typedef struct _gckvip_tlb_memory
{
    /* the video memory handle */
    void            *handle;
    /* vip physical address */
    phy_address_t    physical;
    /* the kernel space logical address */
    void            *logical;
    vip_uint32_t    size;
#if vpmdENABLE_CAPTURE_MMU
    /* the id of video memory managment by video memory database */
    vip_uint32_t     handle_id;
#endif
} gckvip_tlb_memory_t;

typedef struct _gckvip_hardware
{
    vip_uint32_t    core_id;
    /* register base memory address */
    volatile void  *reg_base;
    /* interrupt sync object */
    vip_uint32_t   *irq_queue;
    /* interrupt flag */
    volatile vip_uint32_t* volatile irq_value;

#if vpmdENABLE_MULTIPLE_TASK
    /* mutex for read/write register */
    gckvip_mutex    reg_mutex;
#endif

#if vpmdENABLE_WAIT_LINK_LOOP
    /* wait link buffer. */
    vip_uint32_t   *waitlinkbuf_handle;
    vip_uint32_t   *waitlinkbuf_logical;
    vip_uint32_t    waitlinkbuf_physical;
    vip_uint32_t    waitlinkbuf_size;
    vip_uint32_t   *waitlink_handle;
    vip_uint32_t   *waitlink_logical;
    vip_uint32_t    waitlink_physical;
#endif
#if vpmdENABLE_MMU
    gckvip_video_memory_t MMU_flush;
    gckvip_atomic         flush_mmu;
#endif
} gckvip_hardware_t;

typedef struct _gckvip_pm
{
    gckvip_atomic          status; /* atomic, status value is gckvip_power_status_e */
    gckvip_recursive_mutex mutex; /* mutex object */
#if vpmdENABLE_SUSPEND_RESUME
    /* A signal for waiting system resume, then submit a new task to hardware */
    gckvip_signal          resume_signal;
    /* A flag for resume signal in waiting status */
    volatile vip_uint8_t   wait_resume_signal;
    gckvip_atomic          force_idle;
#endif
#if vpmdPOWER_OFF_TIMEOUT
    gckvip_timer     timer; /* timer object */
    gckvip_signal    signal; /* signal object */
    void             *thread; /* thread object*/
    gckvip_atomic    thread_running; /* atomic object */

    gckvip_atomic    user_query; /* atomic object */
    gckvip_atomic    enable_timer;
    gckvip_signal    exit_signal;
#endif
#if vpmdENABLE_USER_CONTROL_POWER
    gckvip_atomic    user_power_off;
    gckvip_atomic    user_power_stop;
#endif
} gckvip_pm_t;

typedef struct _gckvip_device
{
    vip_uint32_t       device_id;

    vip_uint32_t       hardware_count;
    gckvip_hardware_t *hardware;
    gckvip_video_memory_ext_t init_command;
    vip_uint32_t     init_command_size;

    gckvip_atomic    device_idle; /* atomic */

    gckvip_atomic    core_fscale_percent;
    gckvip_atomic    last_fscale_percent;
#if vpmdENABLE_CLOCK_SUSPEND
    /* flag device's clock can be suspend or not */
    gckvip_atomic    disable_clock_suspend;
    /* flag the device's clock has suspend or not.
       true is suspended, false is not suspend */
    gckvip_atomic    clock_suspended;
#endif

#if !vpmdONE_POWER_DOMAIN
    gckvip_pm_t dp_management;
#endif

#if vpmdENABLE_MULTIPLE_VIP && vpmdONE_POWER_DOMAIN && vpmdENABLE_POWER_MANAGEMENT
    gckvip_signal         resume_done;
#endif
#if vpmdENABLE_MULTIPLE_TASK
    gckvip_queue_t  mt_input_queue;
    gckvip_atomic   mt_thread_running;  /* atomic object for multiple task thread is running */
    void            *mt_thread_handle;  /* thread object. multiple task thread process handle */
    gckvip_signal   mt_thread_signal;   /* signal for destroy multiple task thread */
    gckvip_hashmap_t     mt_hashmap;
#else
    gckvip_task_slot_t   task_slot;
#endif
    volatile vip_uint32_t user_submit_count;
    volatile vip_uint32_t hw_submit_count;
#if vpmdENABLE_RECOVERY
    volatile vip_int8_t   recovery_times;
#endif
#if vpmdENABLE_HANG_DUMP
    volatile vip_uint8_t dump_finish;
#endif
#if vpmdENABLE_HANG_DUMP || vpmdENABLE_WAIT_LINK_LOOP
    gckvip_submit_t      curr_command;
    gckvip_submit_t      pre_command;
#endif
#if vpmdREGISTER_NETWORK
    void **previous_handles;
    vip_uint32_t pre_handle_start_index;
    vip_uint32_t pre_handle_end_index;
#endif
} gckvip_device_t;

/* Kernel space global context. */
typedef struct _gckvip_context
{
    volatile vip_int32_t  initialize;
    gckvip_hashmap_t      process_id;
    /* the max number of vip core */
    vip_uint32_t          max_core_count;
    /* the real number of vip core */
    vip_uint32_t          core_count;
    /* the logical vip device number */
    vip_uint32_t          device_count;
    gckvip_device_t       *device;

    vip_uint32_t    chip_ver1;
    vip_uint32_t    chip_ver2;
    vip_uint32_t    chip_cid;
    vip_uint32_t    chip_date;

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    /* video memory heap management object */
    gckvip_heap_t   video_mem_heap;
    /* cpu virtual base address in user space */
    void            *heap_user;
    /* cpu virtual base address in kernel space */
    void            *heap_kernel;
    /* vip physical base address */
    phy_address_t   heap_physical;
    /* video memory memory size */
    vip_uint32_t    heap_size;
    /* The virtual base address of VIP when MMU enabled */
    vip_uint32_t    heap_virtual;
#endif
#if vpmdENABLE_SYS_MEMORY_HEAP
    /* the size of system heap memory */
    vip_uint32_t    sys_heap_size;
#endif
#if vpmdENABLE_RESERVE_PHYSICAL
    phy_address_t   reserve_phy_base;
    vip_uint32_t    reserve_phy_size;
    vip_uint32_t    reserve_virtual;
#endif
#if vpmdENABLE_MMU
    gckvip_tlb_memory_t  MTLB;
    gckvip_tlb_memory_t  STLB_1M;
    gckvip_tlb_memory_t  STLB_4K;
    gckvip_tlb_memory_t  MMU_entry;
    /* page table pointer */
    gckvip_tlb_memory_t  *ptr_STLB_1M;
    gckvip_tlb_memory_t  *ptr_STLB_4K;
    gckvip_video_memory_t  *ptr_MMU_flush;
    volatile vip_uint8_t   *bitmap_1M;
    volatile vip_uint8_t   *bitmap_4K;
    gckvip_tlb_memory_t    default_mem;
    vip_uint32_t           max_1Mpage;
    vip_uint32_t           max_4Kpage;
#endif

#if vpmdONE_POWER_DOMAIN
    gckvip_pm_t     sp_management;
#endif
    gckvip_atomic   init_done; /* did user space init-command or not */
    /* the size of vip-sram for per-device */
    vip_uint32_t    vip_sram_size[gcdMAX_CORE];
    /* the remap address or VIP's virtual address of VIP SRAM */
    vip_uint32_t    vip_sram_address;
    /* the size of axi sram for per-device */
    vip_uint32_t    axi_sram_size[gcdMAX_CORE];
    /* the VIP's virtual address of AXI SRAM */
    vip_uint32_t    axi_sram_virtual;
    /* the CPU physical address of AXI SRAM */
    vip_uint32_t    axi_sram_physical;

#if vpmdENABLE_MULTIPLE_TASK
    /* mutex for flush the CPU cache of video memory */
    gckvip_mutex          flush_cache_mutex;
    /* mutex for initialize kernel space driver */
    gckvip_mutex          initialize_mutex;
#if vpmdENABLE_MMU
    /* mutex for mmu page table */
    gckvip_mutex          mmu_mutex;
#endif
#endif
    /* mutex for allocate/free video memory */
    gckvip_mutex          memory_mutex;
    /* maintain video memory heandle pointer(kernel space) and handle id(user space) */
    gckvip_database_t     videomem_database;
    gckvip_hashmap_t      wrap_hashmap;

    gckvip_feature_config_t   func_config;
#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
    gckvip_tlb_memory_t      register_dump_buffer;
    vip_uint32_t             register_dump_count;
#endif
#if vpmdREGISTER_NETWORK
    gckvip_hashmap_t      network_info;
    gckvip_hashmap_t      segment_info;
#endif
} gckvip_context_t;

/************ EXPOSED APIs  ***************/
/*
@brief Destroy kernel space resource.
       This function only use on Linux/Andorid system when used ctrl+c
       to exit application or application crash happend.
       When the appliction exits abnormally, gckvip_destroy() used
       to free all kenerl space resource.
*/
vip_status_e gckvip_destroy(void);

/*
@brief  initialize driver
*/
vip_status_e gckvip_init(void);

/*
@brief Get kernel context object.
*/
gckvip_context_t* gckvip_get_context(void);

/*
@brief Get hardware object from core id.
*/
gckvip_hardware_t* gckvip_get_hardware(
    vip_uint32_t core_id
    );

/*
@brief  dispatch kernel command
@param  command, command type.
@param  data, the data for command type.
*/
vip_status_e gckvip_kernel_call(
    gckvip_command_id_e command,
    void *data
    );
#endif

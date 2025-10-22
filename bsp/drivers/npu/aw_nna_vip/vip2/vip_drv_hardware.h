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

#ifndef _VIP_DRV_HARDWARE_H
#define _VIP_DRV_HARDWARE_H

#include <vip_drv_type.h>
#include <vip_drv_util.h>
#include <vip_drv_video_memory.h>
#if vpmdENABLE_POWER_MANAGEMENT
#include <vip_drv_pm.h>
#endif
#if vpmdENABLE_FUSA
#include <vip_drv_fusa.h>
#endif


#define EXPECT_IDLE_VALUE_ALL_MODULE   0x7FFFFFFF
#define EXPECT_IDLE_VALUE_NOT_FE       0x7FFFFFFE

#define SETBIT(value, bit, data) \
    (((value) & (~(1 << (bit)))) | ((data & 0x1) << (bit)))

#define SETBITS(value, start, end, data) \
    (((value) & (~((~0ULL >> (63 - (end) + (start))) << (start)))) | \
    (((data) & (~0ULL >> (63 - (end) + (start)))) << (start)))

/*
  WL_TABLE_COUNT should be less than 25 (25 event id at most).
  can deploy WL_TABLE_COUNT - 1 tasks at most.
*/
#define WL_TABLE_COUNT      10

#define IRQ_VALUE_AXI           0x80000000
#define IRQ_VALUE_MMU           0x40000000
#define IRQ_VALUE_BYPASS        0x20000000
#define IRQ_VALUE_CANCEL        0x10000000
#define IRQ_VALUE_FLAG          0x08000000
#define IRQ_VALUE_ERROR         0x04000000
#define IRQ_VALUE_ERROR_FUSA    0x02000000

#if vpmdENABLE_POLLING
#define HARDWARE_BYPASS_SUBMIT()                           \
{                                                          \
    hw->irq_local_value = IRQ_VALUE_BYPASS;                \
    PRINTK_E("hardware %d bypass submit.\n", hw->core_id); \
}
#else
#if vpmdTASK_PARALLEL_ASYNC
#define HARDWARE_BYPASS_SUBMIT()                           \
{                                                          \
    hw->irq_local_value = IRQ_VALUE_BYPASS;                \
    *hw->device->irq_value |= IRQ_VALUE_FLAG;              \
    vipdrv_os_inc_atomic(*hw->irq_count);                  \
    vipdrv_os_wakeup_interrupt(hw->device->irq_queue);     \
    PRINTK_E("hardware %d bypass submit.\n", hw->core_id); \
}
#else
#define HARDWARE_BYPASS_SUBMIT()                           \
{                                                          \
    hw->irq_local_value = IRQ_VALUE_BYPASS;                \
    *hw->irq_value |= IRQ_VALUE_FLAG;                      \
    vipdrv_os_inc_atomic(*hw->irq_count);                  \
    vipdrv_os_wakeup_interrupt(hw->irq_queue);             \
    PRINTK_E("hardware %d bypass submit.\n", hw->core_id); \
}
#endif
#endif

#if vpmdTASK_PARALLEL_ASYNC
typedef struct _vipdrv_wait_link_table
{
    vip_uint8_t* logical;
    vip_uint32_t physical;
    volatile vip_uint32_t tcb_index;
} vipdrv_wait_link_table_t;
#endif

#if vpmdENABLE_MMU
typedef struct _vipdrv_mmu_flush
{
    vip_uint8_t* logical;
    vip_virtual_t virtual;
} vipdrv_mmu_flush_t;
#endif

struct _vipdrv_hardware
{
    /* the identiry number of hardware */
    vip_uint32_t     core_id;
    /* the index of hardware belong to device */
    vip_uint32_t     core_index;
    /* belong to which device */
    vipdrv_device_t *device;
    /* register base memory address */
    volatile void   *reg_base;
#if !vpmdENABLE_POLLING
    /* interrupt sync object */
    void            *irq_queue;
    vipdrv_atomic   *irq_count;
#endif
    /* interrupt flag */
    volatile vip_uint32_t *irq_value;
    volatile vip_uint32_t irq_local_value; /* set by driver */
#if vpmdENABLE_POWER_MANAGEMENT
    vipdrv_pm_t      pwr_management;
    vip_uint8_t      core_fscale_percent;
    /* flag device's clock can be suspend or not */
    vip_uint32_t     disable_clock_suspend;
#if vpmdENABLE_CLOCK_SUSPEND
    /* flag the device's clock has suspend or not.
       true is suspended, false is not suspend */
    vip_bool_e       clock_suspended;
#endif
#endif
    vipdrv_atomic    idle; /* atomic */
    vip_uint32_t     user_submit_count;
    vip_uint32_t     hw_submit_count;
#if vpmdENABLE_MULTIPLE_TASK
    /* mutex for read/write register */
    vipdrv_mutex     reg_mutex;
#if vpmdTASK_PARALLEL_SYNC
    void* wait_thread_handle; /* thread object. task waiting thread process handle */
    volatile vip_uint32_t    tcb_index;
    vipdrv_signal            core_thread_run_signal;
    vipdrv_signal            core_thread_exit_signal;
    volatile vip_bool_e      dispatch;
#elif vpmdTASK_PARALLEL_ASYNC
    vipdrv_video_memory_t    wl_memory;
    vipdrv_wait_link_table_t wl_table[WL_TABLE_COUNT];
    volatile vip_uint32_t    wl_start_index;
    volatile vip_uint32_t    wl_end_index;
#endif
#endif
#if vpmdENABLE_MMU
#if vpmdTASK_PARALLEL_ASYNC
    vipdrv_mmu_flush_t    MMU_flush_table[WL_TABLE_COUNT];
#else
    vipdrv_mmu_flush_t    MMU_flush;
#endif
    volatile vip_bool_e   flush_mmu;
#endif
#if vpmdENABLE_RECOVERY
    volatile vip_int8_t   recovery_times;
#endif
    vip_uint32_t *previous_task_id;
    vip_uint32_t *previous_subtask_index;
    vip_uint32_t pre_start_index;
    vip_uint32_t pre_end_index;
    vip_bool_e   init_done; /* did user space init-command or not */
#if vpmdENABLE_MMU
    volatile vip_uint64_t cur_mmu_id;
#endif
#if vpmdTASK_DISPATCH_DEBUG
#if vpmdTASK_PARALLEL_SYNC
    volatile vip_uint32_t     get_notify_cnt;
    volatile vip_uint32_t     submit_hw_cnt;
    volatile vip_uint32_t     wait_hw_cnt;
#elif vpmdTASK_PARALLEL_ASYNC
    volatile vip_uint32_t     wait_hw_cnt;
#endif
#endif
};

vip_uint32_t cmd_link(
    vip_uint32_t *command,
    vip_uint32_t address
    );

#if vpmdTASK_PARALLEL_ASYNC || vpmdTASK_SCHEDULE
vip_uint32_t cmd_event(
    vip_uint32_t *command,
    vip_uint32_t block,
    vip_uint32_t id
    );

vip_uint32_t cmd_chip_enable(
    vip_uint32_t *command,
    vip_uint32_t core_id,
    vip_bool_e   is_enable_all
    );
#endif

#if vpmdTASK_PARALLEL_ASYNC
vip_uint32_t cmd_wait(
    vip_uint32_t *command,
    vip_uint32_t time
    );

vip_uint32_t cmd_append_irq(
    vip_uint8_t* command,
    vip_uint32_t core_id,
    vip_uint32_t event_id
    );
#endif

/*
@ brief
    Initializae HW.
*/
vip_status_e vipdrv_hw_init(
    vipdrv_hardware_t* hardware
    );

/*
@ brief
    Destroy and free HW resource.
*/
vip_status_e vipdrv_hw_destroy(void);

/*
@brief To run any initial commands once in the beginning.
*/
vip_status_e vipdrv_hw_submit_initcmd(
    vipdrv_hardware_t* hardware
    );

/*
@ brief
    Do a software HW reset.
*/
vip_status_e vipdrv_hw_reset(
    vipdrv_hardware_t *hardware
    );

#if vpmdTASK_PARALLEL_ASYNC
vip_status_e vipdrv_hw_trigger_wl(
    vipdrv_hardware_t *hardware
    );

vip_status_e vipdrv_hw_stop_wl(
    vipdrv_hardware_t *hardware
    );

vip_status_e vipdrv_hw_wl_table_init(
    vipdrv_hardware_t *hardware
    );

vip_status_e vipdrv_hw_wl_table_uninit(
    vipdrv_hardware_t *hardware
    );
#endif

/*
@ brief
    Submit command buffer to hardware.
*/
vip_status_e vipdrv_hw_trigger(
    vipdrv_hardware_t *hardware,
    vip_uint32_t physical
    );

/*
@ brief
    hardware go back to WL.
*/
vip_status_e vipdrv_hw_idle(
    vipdrv_hardware_t *hardware
    );

vip_status_e vipdrv_hw_read_info(void);

/*
@brief  read a hardware register.
@param  address, the address of hardware register.
@param  data, read data from register.
*/
vip_status_e vipdrv_read_register(
    vipdrv_hardware_t *hardware,
    vip_uint32_t address,
    vip_uint32_t *data
    );

/*
@brief  write a hardware register.
@param  address, the address of hardware register.
@param  data, write data to register.
*/
vip_status_e vipdrv_write_register(
    vipdrv_hardware_t *hardware,
    vip_uint32_t address,
    vip_uint32_t data
    );

/*
@brief  write a hardware register.
@param  address, the address of hardware register.
@param  data, write data to register.
*/
vip_status_e vipdrv_write_register_ext(
    vipdrv_hardware_t *hardware,
    vip_uint32_t address,
    vip_uint32_t data
    );

/*
@brief waiting for hardware going to idle.
@param timeout the number of milliseconds this waiting.
@param is_fast true indicate that need udelay for poll hardware idle or not.
*/
vip_status_e vipdrv_hw_wait_idle(
    vipdrv_hardware_t *hardware,
    vip_uint32_t timeout,
    vip_uint8_t is_fast,
    vip_uint32_t expect_idle_value
    );

/*
@brief cancel related hardware.
@param device, related device.
@param cancel_mask, which hardware need to be cancelled in device.
*/
#if vpmdENABLE_RECOVERY || vpmdENABLE_CANCELATION
vip_status_e vipdrv_hw_cancel(
    vipdrv_device_t *device,
    vip_uint8_t* cancel_mask
    );
#endif

#if vpmdENABLE_MMU
vip_status_e vipdrv_hw_pdmode_setup_mmu(
    vipdrv_hardware_t *hardware
    );
#endif
#endif

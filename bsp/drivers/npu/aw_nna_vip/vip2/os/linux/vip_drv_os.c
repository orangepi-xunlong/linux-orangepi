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

#include "vip_drv_os_port.h"
#include "vip_drv_device_driver.h"
#include "vip_drv_mem_heap.h"
#include "vip_drv_hardware.h"
#include "vip_drv_debug.h"
#include "vip_drv_context.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0)
#include <linux/stdarg.h>
#else
#include <stdarg.h>
#endif
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <asm/atomic.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
#include <uapi/linux/sched/types.h>
#include <linux/sched/task.h>
#else
#include <linux/sched.h>
#endif

typedef struct _vipdrv_signal_data
{
    wait_queue_head_t wait_queue;
    volatile vip_bool_e wait_cond;
    volatile vip_bool_e value;
    spinlock_t lock;
} vipdrv_signal_data_t;


#if vpmdENABLE_MEMORY_PROFILING
#define INIT_CAP_NUM     128
typedef struct _vipdrv_mem_profile_table_t
{
    vip_ptr *memory;
    vip_uint32_t size;
} vipdrv_mem_profile_table_t;

typedef struct _vipdrv_mem_profile_db_t
{
    vipdrv_mem_profile_table_t *table;
    vip_uint32_t total_count;
    vip_uint32_t used_count;
    vip_uint32_t current_index;
    vip_uint32_t kmem_alloc_cnt;
    vip_uint32_t kmem_free_cnt;
    vip_uint32_t kmem_peak;
    vip_uint32_t kmem_current;
} vipdrv_mem_profile_db_t;

vipdrv_mem_profile_db_t database = {0};
static DEFINE_MUTEX(profile_mutex);
#endif

/*
@brief Do some initialization works in VIPDRV_OS layer.
@param video_mem_size, the size of video memory heap.
*/
vip_status_e vipdrv_os_init(
    vip_uint32_t video_mem_size
    )
{
    vip_status_e status = VIP_SUCCESS;
    /* workround solution for dyn enable and disable capture and cnn profile*/
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_driver_t  *kdriver = vipdrv_get_kdriver();
    context->func_config.enable_capture = kdriver->func_config.enable_capture;
    context->func_config.enable_cnn_profile = kdriver->func_config.enable_cnn_profile;
    context->func_config.enable_dump_nbg = kdriver->func_config.enable_dump_nbg;

    status = vipdrv_drv_init(video_mem_size);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to drv init\n");
    }

#if vpmdENABLE_MEMORY_PROFILING
    mutex_lock(&profile_mutex);
    if (VIP_NULL != database.table) {
        kfree(database.table);
        database.table = VIP_NULL;
    }
    vipdrv_os_zero_memory(&database, sizeof(vipdrv_mem_profile_db_t));
    mutex_unlock(&profile_mutex);
#endif

#if vpmdENABLE_DEBUGFS
    status = vipdrv_debug_profile_start();
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail debug profile start\n");
    }
#endif

    return status;
}

/*
@brief Do some un-initialization works in VIPDRV_OS layer.
*/
vip_status_e vipdrv_os_close(void)
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdENABLE_DEBUGFS
     vipdrv_debug_profile_end();
#endif

    status = vipdrv_drv_exit();
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to drv exit\n");
    }

#if vpmdENABLE_MEMORY_PROFILING
    mutex_lock(&profile_mutex);
    PRINTK("========== VIP Drv Memory Statistics ========\n");
    PRINTK("vip-drv Memory Alloc Count   : %d times.\n", database.kmem_alloc_cnt);
    PRINTK("vip-drv Memory Free Count    : %d times.\n", database.kmem_free_cnt);
    PRINTK("vip-drv Memory Current Value : %d bytes.\n", database.kmem_current);
    PRINTK("vip-drv Memory Peak Value    : %d bytes.\n", database.kmem_peak);
    PRINTK("================   End  ====================\n");

    if (database.kmem_current != 0) {
        vip_uint32_t i = 0;
        PRINTK("Kernel Memory Not Free Ptr List:\n");
        for (i = 0; i < database.total_count; i++) {
            if (VIP_NULL != database.table[i].memory) {
                PRINTK("Ptr=0x%"PRPx", Size=%dbytes.\n", database.table[i].memory, database.table[i].size);
            }
        }
    }

    if (VIP_NULL != database.table) {
        kfree(database.table);
        database.table = VIP_NULL;
    }
    vipdrv_os_zero_memory(&database, sizeof(vipdrv_mem_profile_db_t));
    mutex_unlock(&profile_mutex);
#endif

    return status;
}


/************************************************************************/
/************************************************************************/
/* *********************need to be ported functions**********************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/*
@brief Allocate system memory in vip drv space.
@param video_mem_size, the size of video memory heap.
@param size, Memory size to be allocated.
@param memory, Pointer to a variable that will hold the pointer to the memory.
*/
vip_status_e vipdrv_os_allocate_memory(
    IN vip_size_t size,
    OUT void **memory
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (size > 0) {
        *memory = kmalloc(size, GFP_KERNEL);
        if (*memory == VIP_NULL) {
            *memory = vmalloc(size);
            if (*memory == VIP_NULL) {
                status = VIP_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    else {
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }

#if vpmdENABLE_MEMORY_PROFILING
    mutex_lock(&profile_mutex);
    if (database.used_count >= database.total_count) {
        /* expansion */
        vipdrv_mem_profile_table_t *table = VIP_NULL;
        table = (vipdrv_mem_profile_table_t*)kmalloc(\
            sizeof(vipdrv_mem_profile_table_t) * (database.total_count + INIT_CAP_NUM), GFP_KERNEL);
        if (VIP_NULL == table) {
            goto unlock;
        }
        vipdrv_os_zero_memory(table, sizeof(vipdrv_mem_profile_table_t) * (database.total_count + INIT_CAP_NUM));
        if (VIP_NULL != database.table) {
            vipdrv_os_copy_memory(table, database.table, sizeof(vipdrv_mem_profile_table_t) * database.total_count);
            kfree(database.table);
        }

        database.total_count += INIT_CAP_NUM;
        database.table = table;
    }

    while (1) {
        database.current_index %= database.total_count;
        if (VIP_NULL == database.table[database.current_index].memory) {
            database.table[database.current_index].memory = *memory;
            database.table[database.current_index].size = size;
            break;
        }
        database.current_index++;
    }

    database.kmem_alloc_cnt++;
    database.kmem_current += size;
    database.used_count++;
    if (database.kmem_current > database.kmem_peak) {
        database.kmem_peak = database.kmem_current;
    }
unlock:
    mutex_unlock(&profile_mutex);
#endif

    return status;
}

/*
@brief Free system memory in vip drv space.
@param memory, Memory to be freed.
*/
vip_status_e vipdrv_os_free_memory(
    IN void *memory
    )
{
    unsigned long address = VIPDRV_PTR_TO_UINT64(memory);
#if vpmdENABLE_MEMORY_PROFILING
    mutex_lock(&profile_mutex);
    if (VIP_NULL != database.table) {
        vip_uint32_t i = 0;
        vip_uint32_t size = 0;
        for (i = 0; i < database.total_count; i++) {
            if (memory == database.table[i].memory) {
                database.table[i].memory = VIP_NULL;
                size = database.table[i].size;
                database.table[i].size = 0;
                break;
            }
        }
        if (i >= database.total_count) {
            PRINTK_D("fail to get memory size in free memory\n");
        }
        database.kmem_free_cnt++;
        database.used_count--;
        database.kmem_current -= size;
    }
    mutex_unlock(&profile_mutex);
#endif

    if (memory != VIP_NULL) {
        if (unlikely(address >= VMALLOC_START && address <= VMALLOC_END)) {
            vfree(memory);
        }
        else {
            kfree(memory);
        }
    }
    else {
        PRINTK_E("fail to free memory is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    return VIP_SUCCESS;
}

/*
@brief Set memory to zero.
@param memory, Memory to be set to zero.
@param size, the size of memory.
*/
vip_status_e vipdrv_os_zero_memory(
    IN void *memory,
    IN vip_size_t size
    )
{
    vip_uint8_t *data = (vip_uint8_t*)memory;

    if (!memory) {
        PRINTK_E("fail to memset memory, size: %d bytes\n", size);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    #if 0
    {
    vip_size_t i = 0;
    for (i = 0; i < size; i++) {
        data[i] = 0;
    }
    }
    #else
    memset(data, 0, size);
    #endif

    return VIP_SUCCESS;
}

/*
@brief copy src memory to dst memory.
@param dst, Destination memory.
@param src. Source memory
@param size, the size of memory should be copy.
*/
vip_status_e vipdrv_os_copy_memory(
    IN void *dst,
    IN const void *src,
    IN vip_size_t size
    )
{
    if ((VIP_NULL == src) || (VIP_NULL == dst))  {
		PRINTK_D("fail to copy memory, parameter is NULL, size: %d bytes\n", size);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    memcpy(dst, src, size);

    return VIP_SUCCESS;
}

/*
@brief Read a hardware register.
@param reg_base, the base address of hardware register.
@param address, read the register address.
@param data, the data of address be read.
*/
vip_status_e vipdrv_os_read_reg(
    IN volatile void *reg_base,
    IN vip_uint32_t address,
    OUT vip_uint32_t *data
    )
{
    *data = readl((vip_uint8_t *)reg_base + address);

#if vpmdENABLE_AHB_LOG
    {
    vipdrv_hardware_info_t info = {0};
    vip_uint32_t i = 0;
    vipdrv_drv_get_hardware_info(&info);
    for (i = 0; i < info.core_count; i++) {
        if (reg_base == info.vip_reg[i]) {
            break;
        }
    }
    PRINTK("@[register.read %u 0x%05X 0x%08X]\n", i, address, *data);
    }
#endif

    return VIP_SUCCESS;
}

/*
@brief Write a hardware register.
@param reg_base, the base address of hardware register.
@param address, write the register address.
@param data, the data of address be wrote.
*/
vip_status_e  vipdrv_os_write_reg(
    IN volatile void *reg_base,
    IN vip_uint32_t address,
    IN vip_uint32_t data
    )
{
    writel(data, (vip_uint8_t *)reg_base + address);

#if vpmdENABLE_AHB_LOG
    {
    vipdrv_hardware_info_t info = {0};
    vip_uint32_t i = 0;
    vipdrv_drv_get_hardware_info(&info);
    for (i = 0; i < info.core_count; i++) {
        if (reg_base == info.vip_reg[i]) {
            break;
        }
    }
    PRINTK("@[register.write %u 0x%05X 0x%08X]\n", i, address, data);
    }
#endif

    return VIP_SUCCESS;
}

/*
@brief Waiting for the hardware interrupt
@param irq_queue, the wait queue of interrupt handle.
@param irq_value, a flag for interrupt.
@param time_out, the time out of this wait. xxxms.
@param mask, mask data for interrupt.
*/
vip_status_e vipdrv_os_wait_interrupt(
    IN void *irq_queue,
    IN volatile vip_uint32_t* volatile irq_value,
    IN vip_uint32_t time_out,
    IN vip_uint32_t mask
    )
{
    vip_status_e status = VIP_SUCCESS;
    wait_queue_head_t *wait_queue = (wait_queue_head_t *)irq_queue;
    vipIsNULL(irq_value);

    if (wait_queue != VIP_NULL) {
        unsigned long jiffies = 0;
        long result = 0;

        if (vpmdINFINITE == time_out) {
            jiffies = MAX_SCHEDULE_TIMEOUT;
        }
        else {
            jiffies = msecs_to_jiffies(time_out);
        }

    wait_irq:
        result = wait_event_interruptible_timeout(*wait_queue, *irq_value & mask, jiffies);
        if (0 == result) {
            dump_stack();
            PRINTK_E("fail to wait interrupt, timeout=%ums....\n", time_out);
            vipGoOnError(VIP_ERROR_TIMEOUT);
        }
        else if (result < 0) {
            PRINTK_E("wait irq event wake up by signal, result=%d\n", result);
            goto wait_irq;
        }
    }
    else {
        status = VIP_ERROR_INVALID_ARGUMENTS;
        PRINTK_E("irq_queue is NULL in os_wait_interrupt\n");
    }

onError:
    return status;
}

vip_status_e vipdrv_os_wakeup_interrupt(
    IN void *irq_queue
    )
{
    vip_status_e status = VIP_SUCCESS;

    wait_queue_head_t *wait_queue = (wait_queue_head_t *)irq_queue;

    wake_up_interruptible(wait_queue);

    return status;
}

/*
@brief Delay execution of the current thread for a number of milliseconds.
@param ms, delay unit ms. Delay to sleep, specified in milliseconds.
*/
void vipdrv_os_delay(
    IN vip_uint32_t ms
    )
{
    if (ms > 0)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
        ktime_t delay = ktime_set((ms / MSEC_PER_SEC), (ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
        __set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_hrtimeout(&delay, HRTIMER_MODE_REL);
#else
        msleep(ms);
#endif
    }
}

/*
@brief Delay execution of the current thread for a number of microseconds.
@param ms, delay unit us. Delay to sleep, specified in microseconds.
*/
void vipdrv_os_udelay(
    IN vip_uint32_t us
    )
{
    if (us > 0) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
        #if 0
        ktime_t delay = ktime_set((us / USEC_PER_SEC), (us % USEC_PER_SEC) * NSEC_PER_USEC);
        __set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_hrtimeout(&delay, HRTIMER_MODE_REL);
        #endif
        udelay(us);
#else
        usleep_range((unsigned long)us, (unsigned long)us + 1);
#endif
    }
}

/*
@brief Print string on console.
@param msg, which message to be print.
*/
vip_status_e vipdrv_os_print(
    vip_log_level level,
    IN const char *msg,
    ...
    )
{
    vip_status_e status = VIP_SUCCESS;
    char strbuf[256] = {'\0'};
    va_list varg;
#if (vpmdENABLE_DEBUG_LOG > 2)
    vipdrv_driver_t *driver = VIP_NULL;
    driver = vipdrv_get_kdriver();
    if (((level == VIP_LOG_DEBUG) || (level == VIP_LOG_INFO)) && (0 == driver->log_level)) {
        return VIP_SUCCESS;
    }
#endif

    va_start (varg, msg);
    vsnprintf(strbuf, 256, msg, varg);
    va_end(varg);

#if (vpmdENABLE_DEBUG_LOG >= 4)
    printk(KERN_EMERG "npu[%x][%x] %s", vipdrv_os_get_pid(), vipdrv_os_get_tid(), strbuf);
#else
    printk("npu[%x][%x] %s", vipdrv_os_get_pid(), vipdrv_os_get_tid(), strbuf);
#endif

    return status;
}

/*
@brief Print string to buffer.
@param, size, the size of msg.
@param msg, which message to be print.
*/
vip_status_e vipdrv_os_snprint(
    IN vip_char_t *buffer,
    IN vip_uint32_t size,
    IN const vip_char_t *msg,
    ...
    )
{
    vip_status_e status = VIP_SUCCESS;
    va_list varg;
    va_start (varg, msg);
    vsnprintf(buffer, size, msg, varg);
    va_end(varg);

    return status;
}

/*
@brief get the current system time. return value unit us, microsecond
*/
vip_uint64_t  vipdrv_os_get_time(void)
{
    vip_uint64_t time = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    struct timespec64 tv;
    /* Return the time of day in microseconds. */
    ktime_get_ts64(&tv);
    time = (vip_uint64_t)((tv.tv_sec * 1000000ULL) + (tv.tv_nsec / 1000));
#else
    struct timeval tv;
    do_gettimeofday(&tv);
    time = (vip_uint64_t)((tv.tv_sec * 1000000ULL) + tv.tv_usec);
#endif

    return time;
}

/*
@brief Memory barrier function for memory consistency.
*/
vip_status_e vipdrv_os_memorybarrier(void)
{
    mb();
    return VIP_SUCCESS;
}

/*
@brief Get vip drv space process id
*/
vip_uint32_t vipdrv_os_get_pid(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
    return (vip_uint32_t)task_tgid_vnr(current);
#else
    return (vip_uint32_t)current->tgid;
#endif
}

/*
@brief Get vip drv space thread id
*/
vip_uint32_t vipdrv_os_get_tid(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
        return task_pid_vnr(current);
#else
        return current->pid;
#endif
}

/*
@brief Create a mutex for multiple thread.
@param mutex, create mutex pointer.
*/
vip_status_e vipdrv_os_create_mutex(
    OUT vipdrv_mutex *mutex
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct mutex *c_mutex = VIP_NULL;

    status = vipdrv_os_allocate_memory(sizeof(struct mutex), (void**)&c_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to allocate memory for mutex\n");
        return VIP_ERROR_IO;
    }

    mutex_init(c_mutex);
    if (VIP_NULL == mutex) {
        PRINTK_E("fail to create mutex, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    else {
        *mutex = (vipdrv_mutex)c_mutex;
    }

    return status;
}

vip_status_e vipdrv_os_lock_mutex(
    IN vipdrv_mutex mutex
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (mutex != VIP_NULL) {
        mutex_lock((struct mutex *)mutex);
    }
    else {
        PRINTK_E("fail to lock mutex, parameter is NULL\n");
        status = VIP_ERROR_INVALID_ARGUMENTS;
        VIPDRV_DUMP_STACK();
    }

    return status;
}

vip_status_e vipdrv_os_unlock_mutex(
    IN vipdrv_mutex mutex
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (mutex != VIP_NULL) {
        mutex_unlock((struct mutex *)mutex);
    }
    else {
        PRINTK_E("fail to unlock mutex, parameter is NULL\n");
        status = VIP_ERROR_INVALID_ARGUMENTS;
        VIPDRV_DUMP_STACK();
    }

    return status;
}

/*
@brief Destroy a mutex which create in vipdrv_os_create_mutex..
@param mutex, create mutex pointer.
*/
vip_status_e vipdrv_os_destroy_mutex(
    IN vipdrv_mutex mutex
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (mutex != VIP_NULL) {
        mutex_destroy((struct mutex *)mutex);
        vipdrv_os_free_memory(mutex);
    }
    else {
        PRINTK_E("fail to unlock mutex, parameter is NULL\n");
        status = VIP_ERROR_INVALID_ARGUMENTS;
        VIPDRV_DUMP_STACK();
    }

    return status;
}

vip_status_e vipdrv_os_create_spinlock(
    vipdrv_spinlock *spinlock
    )
{
    vip_status_e status = VIP_SUCCESS;
    spinlock_t *c_lock = VIP_NULL;

    status = vipdrv_os_allocate_memory(sizeof(spinlock_t), (void**)&c_lock);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to allocate memory for spinlock\n");
        return VIP_ERROR_IO;
    }

    spin_lock_init(c_lock);
    *spinlock = (vipdrv_spinlock)c_lock;

    return status;
}

vip_status_e vipdrv_os_lock_spinlock(
    vipdrv_spinlock spinlock
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (spinlock == VIP_NULL) {
        return VIP_ERROR_OUT_OF_MEMORY;
    }

    spin_lock(spinlock);

    return status;

}

vip_status_e vipdrv_os_unlock_spinlock(
    vipdrv_spinlock spinlock
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (spinlock == VIP_NULL) {
        return VIP_ERROR_OUT_OF_MEMORY;
    }

    spin_unlock(spinlock);

    return status;
}

vip_status_e vipdrv_os_destroy_spinlock(
    vipdrv_spinlock spinlock
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (spinlock == VIP_NULL) {
        PRINTK("fail to destroy, spinlock is NULL\n");
        return VIP_ERROR_OUT_OF_MEMORY;
    }

    vipdrv_os_free_memory(spinlock);

    return status;
}

#if vpmdENABLE_MULTIPLE_TASK || vpmdPOWER_OFF_TIMEOUT
/*
@brief Create a thread.
@param func, the thread handle function.
@param parm, parameter for this thread.
@param name, the thread name.
@param handle, the handle for thread.
*/
vip_status_e vipdrv_os_create_thread(
    IN vipdrv_thread_func func,
    IN vip_ptr parm,
    IN vip_char_t *name,
    OUT vipdrv_thread *handle
    )
{
    vip_int32_t ret = 0;
    if ((handle != VIP_NULL) && (func != VIP_NULL)) {
        struct task_struct *task;
        task = kthread_create(func, parm,  "vip_%s", name);
        if (IS_ERR(task)) {
            PRINTK_E("fail to create thread name=%s.\n", name);
            return VIP_ERROR_IO;
        }
        get_task_struct(task);

        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,9,0)
        /* set fifo high prority scheduler */
        sched_set_fifo(task);
        #else
        {
        struct sched_param param;
        /* 99 - 10 = 89 */
        param.sched_priority = 10;
        sched_setscheduler(task, SCHED_FIFO, &param);
        }
        #endif

        PRINTK_D("thread name=%s schedule policy=%d, prio=%d\n", name, task->policy, task->prio);
        ret = wake_up_process(task);
        if (0 == ret) {
            PRINTK_E("wake up thread %s, is running or fail to wake up\n", name);
        }
        *handle = (vipdrv_thread)task;
    }
    else {
        PRINTK_E("fail to create thread. parameter is NULL\n");
        VIPDRV_DUMP_STACK();
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    return VIP_SUCCESS;
}

/*
@brief Destroy a thread.
@param handle, the handle for thread.
*/
vip_status_e vipdrv_os_destroy_thread(
    IN vipdrv_thread handle
    )
{
    if (handle != VIP_NULL) {
        kthread_stop(handle);
        put_task_struct(handle);
    }
    else {
        PRINTK_E("fail to destroy thread");
        VIPDRV_DUMP_STACK();
        return VIP_ERROR_IO;
    }

    return VIP_SUCCESS;
}
#endif

/*
@brief Create a atomic.
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_create_atomic(
    OUT vipdrv_atomic *atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct atomic_t *c_atomic = VIP_NULL;
    vipIsNULL(atomic);

    status = vipdrv_os_allocate_memory(sizeof(atomic_t), (void**)&c_atomic);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to allocate memory for atomic\n");
        return VIP_ERROR_IO;
    }

    /* Initialize the atomic */
    atomic_set((atomic_t *)c_atomic, 0);
    *atomic = (vipdrv_atomic)c_atomic;

onError:
    return status;
}

/*
@brief Destroy a atomic which create in vipdrv_os_create_atomic
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_destroy_atomic(
    IN vipdrv_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipIsNULL(atomic);

    vipdrv_os_free_memory(atomic);

onError:
    return status;
}

/*
@brief Set the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_set_atomic(
    IN vipdrv_atomic atomic,
    IN vip_uint32_t value
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == atomic) {
        PRINTK_E("fail to set atomic is NULL\n");
        VIPDRV_DUMP_STACK();
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    atomic_set((atomic_t *)atomic, value);
    return status;
}

/*
@brief Get the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_uint32_t vipdrv_os_get_atomic(
    IN vipdrv_atomic atomic
    )
{
    if (VIP_NULL == atomic) {
        PRINTK_E("fail to get atomic is NULL\n");
        VIPDRV_DUMP_STACK();
        return 0xFFFFFFFE;
    }
    return atomic_read((atomic_t *)atomic);
}

/*
@brief Increase the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_inc_atomic(
    IN vipdrv_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == atomic) {
        PRINTK_E("fail to inc atomic is NULL\n");
        VIPDRV_DUMP_STACK();
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    atomic_inc((atomic_t *)atomic);
    return status;
}

/*
@brief Decrease the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_dec_atomic(
    IN vipdrv_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == atomic) {
        PRINTK_E("fail to dec atomic is NULL\n");
        VIPDRV_DUMP_STACK();
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    atomic_dec((atomic_t *)atomic);
    return status;
}

#if vpmdENABLE_MULTIPLE_TASK || vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
/*
@brief Create a signal.
@param handle, the handle for signal.
*/
vip_status_e vipdrv_os_create_signal(
    OUT vipdrv_signal *handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_signal_data_t *data = VIP_NULL;

    if (handle == VIP_NULL) {
        PRINTK_E("fail to create signal, parameter is NULL\n");
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_signal_data_t), (vip_ptr*)&data));

    init_waitqueue_head(&data->wait_queue);
    data->wait_cond = vip_false_e;
    data->value = vip_false_e;

    *handle = (vipdrv_signal)data;

    spin_lock_init(&data->lock);

    return status;

onError:
    if (data != VIP_NULL) {
        vipdrv_os_free_memory(data);
    }

    return status;
}

vip_status_e vipdrv_os_destroy_signal(
    IN vipdrv_signal handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_signal_data_t *data = (vipdrv_signal_data_t*)handle;

    if (data == VIP_NULL) {
        PRINTK_E("fail to destory signal, parameter is NULL\n");
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    data->wait_cond = vip_false_e;
    data->value = vip_false_e;
    vipdrv_os_free_memory(handle);

onError:
    return status;
}

/*
@brief Waiting for a signal.
@param handle, the handle for signal.
@param timeout, the timeout of waiting signal. unit ms. timeout is 0 or vpmdINFINITE, infinite wait signal.
*/
vip_status_e vipdrv_os_wait_signal(
    IN vipdrv_signal handle,
    IN vip_uint32_t timeout
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_signal_data_t *data = (vipdrv_signal_data_t*)handle;
    unsigned long jiffies = 0;
    vip_int32_t ret = 0;
    unsigned long flags = 0;

    if (data == VIP_NULL) {
        PRINTK_E("fail to wait signal, parameter is NULL\n");
        VIPDRV_DUMP_STACK();
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    if (vpmdINFINITE == timeout) {
        jiffies = msecs_to_jiffies(WAIT_SIGNAL_TIMEOUT);
    }
    else {
        jiffies = msecs_to_jiffies(timeout);
    }
    if (0 == jiffies) {
        jiffies = 1;
    }

    spin_lock_irqsave(&data->lock, flags);
    if (!data->value) {
        /* Wait for the condition. */
        data->wait_cond = vip_false_e;
        spin_unlock_irqrestore(&data->lock, flags);

        if (vpmdINFINITE == timeout) {
            while (1) {
                ret = wait_event_timeout(data->wait_queue, data->wait_cond, jiffies);
                if (0 == ret) {
                    continue;
                }
                break;
            }
        }
        else {
            ret = wait_event_timeout(data->wait_queue, data->wait_cond, jiffies);
            if (0 == ret) {
                PRINTK_I("wait signal handle=0x%"PRPx", timeout=%dms\n", handle, timeout);
                status = VIP_ERROR_TIMEOUT;
                #if (vpmdENABLE_DEBUG_LOG>3)
                dump_stack();
                #endif
            }
        }
    }
    else {
        spin_unlock_irqrestore(&data->lock, flags);
    }

onError:
    return status;
}

/*
@brief wake up wait signal.
     state is true, wake up all waiting signal and chane the signal state to SET.
     state is false, change the signal to RESET so the vipdrv_os_wait_signal will block until
     the signal state changed to SET. block waiting signal function.
@param handle, the handle for signal.
*/
vip_status_e vipdrv_os_set_signal(
    IN vipdrv_signal handle,
    IN vip_bool_e state
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_signal_data_t *data = (vipdrv_signal_data_t*)handle;
    unsigned long flags = 0;

    if (data == VIP_NULL) {
        PRINTK_E("fail to set signal, parameter is NULL\n");
        VIPDRV_DUMP_STACK();
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    spin_lock_irqsave(&data->lock, flags);
    if (vip_true_e == state) {
        data->value = vip_true_e;
        data->wait_cond = vip_true_e;
        /* wake up all signal in wait queue */
        wake_up_all(&data->wait_queue);
    }
    else {
        data->value = vip_false_e;
    }
    spin_unlock_irqrestore(&data->lock, flags);

onError:
    return status;
}
#endif

#if vpmdPOWER_OFF_TIMEOUT
typedef struct _vipdrv_timer
{
    struct delayed_work      work;
    vipdrv_timer_func        function;
    vip_ptr                  data;
} vipdrv_timer_t;

void _timer_function_handle(
    struct work_struct *work
    )
{
    vipdrv_timer_t *timer = container_of(work, struct _vipdrv_timer, work.work);

    vipdrv_timer_func function = timer->function;

    function(timer->data);
}

/*
@brief create a system timer callback.
@param func, the callback function of this timer.
@param param, the paramete of the timer.
*/
vip_status_e vipdrv_os_create_timer(
    IN vipdrv_timer_func func,
    IN vip_ptr param,
    OUT vipdrv_timer *timer
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_timer_t *pointer = VIP_NULL;

    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_timer_t), (vip_ptr*)&pointer));

    pointer->function = func;
    pointer->data = param;

    INIT_DELAYED_WORK(&pointer->work, _timer_function_handle);

    if (timer != VIP_NULL) {
        *timer = (vipdrv_timer)pointer;
    }
    else {
        status = VIP_ERROR_FAILURE;
    }

onError:
    return status;
}

vip_status_e vipdrv_os_start_timer(
    IN vipdrv_timer timer,
    IN vip_uint32_t time_out
    )
{
    vipdrv_timer_t *pointer = (vipdrv_timer_t*)timer;

    if (VIP_NULL == pointer) {
        PRINTK_E("fail to start timer, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    schedule_delayed_work(&pointer->work, msecs_to_jiffies(time_out));

    return VIP_SUCCESS;
}

vip_status_e vipdrv_os_stop_timer(
    IN vipdrv_timer timer
    )
{
    vipdrv_timer_t *pointer = (vipdrv_timer_t*)timer;

    if (VIP_NULL == pointer) {
        PRINTK_E("fail to stop timer, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    cancel_delayed_work_sync(&pointer->work);

    return VIP_SUCCESS;
}

/*
@brief destroy a system timer
*/
vip_status_e vipdrv_os_destroy_timer(
    IN vipdrv_timer timer
    )
{
    if (timer != VIP_NULL) {
        vipdrv_timer_t *pointer = (vipdrv_timer_t*)timer;
        cancel_delayed_work_sync(&pointer->work);
        vipdrv_os_free_memory(timer);
    }

    return VIP_SUCCESS;
}
#endif

/*
@brief compare two strings.
@param s1, string one.
@param s2, string two.
*/
vip_int32_t vipdrv_os_strcmp(
    const vip_char_t *s1,
    const vip_char_t *s2
    )
{
    return strcmp(s1, s2);
}

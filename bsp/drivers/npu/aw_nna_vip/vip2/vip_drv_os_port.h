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

#ifndef __VIP_DRV_OS_PORT_H__
#define __VIP_DRV_OS_PORT_H__

#include <vip_lite_config.h>
#include <vip_drv_share.h>
#include <vip_drv_type.h>


/* default wait signal timeout 100s */
#define WAIT_SIGNAL_TIMEOUT  (100*1000)

#define VIPDRV_PTR_TO_UINT32(p) \
( \
    (vip_uint32_t) (vipdrv_uintptr_t) (p)\
)

#define VIPDRV_PTR_TO_UINT64(p) \
( \
    (vip_uint64_t) (vipdrv_uintptr_t) (p)\
)

#define VIPDRV_UINT64_TO_PTR(u) \
( \
    (vip_ptr) (vipdrv_uintptr_t) (u)\
)

#if defined(LINUX)
/* used on Linux or Android kernel space system */
#include <linux/kernel.h>
#define VIPDRV_DUMP_STACK()                 dump_stack();
#define VIPDRV_DO_DIV64(a, b, result)  {  \
    vip_uint64_t tmp = a;                 \
    do_div(tmp, b);                       \
    result = tmp;                         \
}
#define VIPDRV_SPINLOCK(T, lock) vipdrv_os_##T##_spinlock(lock)
#elif defined (LINUXEMULATOR)
/* only for linux emulator, not used on FPGA and silicon */
#define VIPDRV_DUMP_STACK()                 dump_stack();
#define VIPDRV_DO_DIV64(a, b, result) (result = (vip_uint64_t)a / (vip_uint64_t)b)
#define VIPDRV_SPINLOCK(T, lock) vipdrv_os_##T##_spinlock(lock)
#else
#define VIPDRV_DUMP_STACK()
#define VIPDRV_DO_DIV64(a, b, result) (result = (vip_uint64_t)a / (vip_uint64_t)b)
#define VIPDRV_SPINLOCK(T, lock) vipdrv_os_##T##_mutex(lock)
#endif

#define VIPDRV_SUB(b, a, result, type) {  \
    if (b >= a) {                         \
        result = b - a;                   \
    }                                     \
    else {                                \
        type tmp = ~0;                    \
        result = tmp - a + b;             \
    }                                     \
}

#define PRINTK(...)      vipdrv_os_print(VIP_LOG_PRINT,__VA_ARGS__)



#if (vpmdENABLE_DEBUG_LOG == 4)
#define PRINTK_D(...)                               \
if (VIPDRV_LOG_ZONE_ENABLED & VIPDRV_LOG_ZONE) {    \
    vipdrv_os_print(VIP_LOG_DEBUG, __VA_ARGS__);    \
}
#define PRINTK_I(...)                               \
if (VIPDRV_LOG_ZONE_ENABLED & VIPDRV_LOG_ZONE) {    \
    vipdrv_os_print(VIP_LOG_INFO, __VA_ARGS__);     \
}
#define PRINTK_W(...)    vipdrv_os_print(VIP_LOG_WARN, __VA_ARGS__)
#define PRINTK_E(...)    vipdrv_os_print(VIP_LOG_ERROR, __VA_ARGS__)
#elif (vpmdENABLE_DEBUG_LOG == 3)
#define PRINTK_D(...)
#define PRINTK_I(...)                               \
if (VIPDRV_LOG_ZONE_ENABLED & VIPDRV_LOG_ZONE) {    \
    vipdrv_os_print(VIP_LOG_INFO, __VA_ARGS__);     \
}
#define PRINTK_W(...)    vipdrv_os_print(VIP_LOG_WARN, __VA_ARGS__)
#define PRINTK_E(...)    vipdrv_os_print(VIP_LOG_ERROR, __VA_ARGS__)
#elif (vpmdENABLE_DEBUG_LOG == 2)
#define PRINTK_D(...)
#define PRINTK_I(...)
#define PRINTK_W(...)    vipdrv_os_print(VIP_LOG_WARN, __VA_ARGS__)
#define PRINTK_E(...)    vipdrv_os_print(VIP_LOG_ERROR, __VA_ARGS__)
#elif (vpmdENABLE_DEBUG_LOG == 1)
#define PRINTK_D(...)
#define PRINTK_I(...)
#define PRINTK_W(...)
#define PRINTK_E(...)    vipdrv_os_print(VIP_LOG_ERROR, __VA_ARGS__)
#else
#define PRINTK_D(...)
#define PRINTK_I(...)
#define PRINTK_W(...)
#define PRINTK_E(...)
#endif


typedef   void*        vipdrv_mutex;
typedef   void*        vipdrv_spinlock;
typedef   void*        vipdrv_atomic;
typedef   void*        vipdrv_signal;
typedef   void*        vipdrv_thread;
typedef   void*        vipdrv_timer;

typedef vip_status_e (* vipdrv_thread_func) (vip_ptr param);
typedef void (* vipdrv_timer_func) (vip_ptr param);

#if vpmdENABLE_VIDEO_MEMORY_HEAP
typedef struct _vipdrv_videomem_heap_ops {
    vip_physical_t(*phy_vip2cpu)(vip_physical_t);
    vip_physical_t(*phy_cpu2vip)(vip_physical_t);
} vipdrv_videomem_heap_ops_t;

typedef struct _vipdrv_videomem_heap_info {
    vip_physical_t             vip_physical;
    vip_size_t                 vip_memsize;
    vip_char_t                 *name;
    vipdrv_videomem_heap_ops_t ops;
    vip_uint64_t               device_vis;
} vipdrv_videomem_heap_info_t;
#endif

/*
@brief Get hardware basic information for device-driver.
*/
typedef struct _vipdrv_hardware_info {
    /* the NPU physical base address of axi-sram */
    vip_physical_t         axi_sram_base;
    vip_uint32_t           axi_sram_size[vipdMAX_CORE];
    vip_uint32_t           vip_sram_base;
    volatile void          *vip_reg[vipdMAX_CORE];
    void                   *irq_queue[vipdMAX_CORE];
    volatile vip_uint32_t* volatile irq_value[vipdMAX_CORE];
    vipdrv_atomic          *irq_count[vipdMAX_CORE];

    vip_uint32_t           core_count;
    vip_uint32_t           max_core_count;
    vip_uint32_t           device_core_number[vipdMAX_CORE];
    vip_uint32_t           core_fscale_percent;
} vipdrv_hardware_info_t;

typedef struct _vipdrv_platform_memory_config {
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    vip_uint32_t           heap_count;
    vipdrv_videomem_heap_info_t     *heap_infos;
#endif
#if vpmdENABLE_SYS_MEMORY_HEAP
    vip_uint32_t           sys_heap_size;
#endif
#if vpmdENABLE_RESERVE_PHYSICAL
    vip_physical_t          reserve_phy_base;
    vip_uint32_t           reserve_phy_size;
#endif

} vipdrv_platform_memory_config_t;

#if defined(LINUX) || defined (LINUXEMULATOR)
vip_status_e vipdrv_os_create_spinlock(
    OUT vipdrv_spinlock *spinlock
    );

vip_status_e vipdrv_os_lock_spinlock(
    IN vipdrv_spinlock spinlock
    );

vip_status_e vipdrv_os_unlock_spinlock(
    IN vipdrv_spinlock spinlock
    );

vip_status_e vipdrv_os_destroy_spinlock(
    IN vipdrv_spinlock spinlock
    );
#endif

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
    );

/*
@brief Write a hardware register.
@param reg_base, the base address of hardware register.
@param address, write the register address.
@param data, the data of address be wrote.
*/
vip_status_e vipdrv_os_write_reg(
    IN volatile void *reg_base,
    IN vip_uint32_t address,
    IN vip_uint32_t data
    );


/************************************************************************/
/************************************************************************/
/* *********************need to be ported functions**********************/
/************************************************************************/
/************************************************************************/
/*************************************************************************/
/*
@brief Allocate system memory in vip-drv space.
@param video_mem_size, the size of video memory heap.
@param size, Memory size to be allocated.
@param memory, Pointer to a variable that will hold the pointer to the memory.
*/
vip_status_e vipdrv_os_allocate_memory(
    IN  vip_size_t size,
    OUT void **memory
    );

/*
@brief Free system memory in vip-drv space.
@param memory, Memory to be freed.
*/
vip_status_e vipdrv_os_free_memory(
    IN void *memory
    );

/*
@brief Set memory to zero.
@param memory, Memory to be set to zero.
@param size, the size of memory.
*/
vip_status_e vipdrv_os_zero_memory(
    IN void *memory,
    IN vip_size_t size
    );

/*
@brief copy src memory to dst memory.
@param dst, Destination memory.
@param src. Source memory
@param size, the size of memory should be copy.
*/
vip_status_e vipdrv_os_copy_memory(
    OUT void *dst,
    IN const void *src,
    IN vip_size_t size
    );

/*
@brief Waiting for the hardware interrupt
@param irq_queue, the wait queue of interrupt handle.
@param irq_value, a flag for interrupt.
@param mask, mask data for interrupt.
*/
vip_status_e vipdrv_os_wait_interrupt(
    IN void *irq_queue,
    IN volatile vip_uint32_t* volatile irq_value,
    IN vip_uint32_t time_out,
    IN vip_uint32_t mask
    );

/*
@brief wakeup the queue manual
@param irq_queue, the wait queue of interrupt handle.
*/
vip_status_e vipdrv_os_wakeup_interrupt(
    IN void *irq_queue
    );

/*
@brief Do some initialization works in VIPDRV_OS layer.
@param video_mem_size, the size of video memory heap.
*/
vip_status_e vipdrv_os_init(
    IN vip_uint32_t video_mem_size
    );

/*
@brief Do some un-initialization works in VIPDRV_OS layer.
*/
vip_status_e vipdrv_os_close(void);

/*
@brief Delay execution of the current thread for a number of milliseconds.
@param ms, delay unit ms. Delay to sleep, specified in milliseconds.
*/
void vipdrv_os_delay(
    IN vip_uint32_t ms
    );

/*
@brief Delay execution of the current thread for a number of microseconds.
@param ms, delay unit us. Delay to sleep, specified in microseconds.
*/
void vipdrv_os_udelay(
    IN vip_uint32_t us
    );

/*
@brief Print string on console.
@param msg, which message to be print.
*/
vip_status_e vipdrv_os_print(
    vip_log_level level,
    IN const char *message,
    ...
    );

/*
@brief Print string to buffer.
@param, size, the size of msg.
@param msg, which message to be print.
*/
vip_status_e vipdrv_os_snprint(
    IN vip_char_t *buffer,
    IN vip_uint32_t size,
    IN const vip_char_t *msg,
    ...);

/*
@brief get the current system time. return value unit us, microsecond
*/
vip_uint64_t vipdrv_os_get_time(void);

/*
@brief Memory barrier function for memory consistency.
*/
vip_status_e vipdrv_os_memorybarrier(void);

/*
@brief Get process id
*/
vip_uint32_t vipdrv_os_get_pid(void);

/*
@brief Get vip-drv space thread id
*/
vip_uint32_t vipdrv_os_get_tid(void);

#if vpmdENABLE_FLUSH_CPU_CACHE
/*
@brief Flush CPU cache for video memory which allocated from heap.
@param physical, the CPU physical address.
@param logical, the CPU logical address.
       loggical address only for Linux system or CPU MMU is enabled?.
       should be equal to physical address in RTOS?
@param size, the size of the memory should be flush.
@param type The type of operate cache. see vipdrv_cache_type_e.
*/
vip_status_e vipdrv_os_flush_cache(
    IN vip_physical_t physical,
    IN void *logical,
    IN vip_uint32_t size,
    IN vip_uint8_t type
    );
#endif

/*
@brief Create a mutex for multiple thread.
@param mutex, create mutex pointer.
*/
vip_status_e vipdrv_os_create_mutex(
    OUT vipdrv_mutex *mutex
    );

vip_status_e vipdrv_os_lock_mutex(
    IN vipdrv_mutex mutex
    );

vip_status_e vipdrv_os_unlock_mutex(
    IN vipdrv_mutex mutex
    );

/*
@brief Destroy a mutex which create in vipdrv_os_create_mutex..
@param mutex, created mutex.
*/
vip_status_e vipdrv_os_destroy_mutex(
    IN vipdrv_mutex mutex
    );

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
    );

/*
@brief Destroy a thread.
@param handle, the handle for thread.
*/
vip_status_e vipdrv_os_destroy_thread(
    IN vipdrv_thread handle
    );
#endif

/*
@brief Create a atomic.
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_create_atomic(
    OUT vipdrv_atomic *atomic
    );

/*
@brief Destroy a atomic which create in vipdrv_os_create_atomic
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_destroy_atomic(
    IN vipdrv_atomic atomic
    );

/*
@brief Set the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_set_atomic(
    IN vipdrv_atomic atomic,
    IN vip_uint32_t value
    );

/*
@brief Get the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_uint32_t vipdrv_os_get_atomic(
    IN vipdrv_atomic atomic
    );

/*
@brief Increase the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_inc_atomic(
    IN vipdrv_atomic atomic
    );

/*
@brief Decrease the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e vipdrv_os_dec_atomic(
    IN vipdrv_atomic atomic
    );

#if vpmdENABLE_MULTIPLE_TASK || vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
/*
@brief Create a signal.
@param handle, the handle for signal.
*/
vip_status_e vipdrv_os_create_signal(
    OUT vipdrv_signal *handle
    );

vip_status_e vipdrv_os_destroy_signal(
    IN vipdrv_signal handle
    );

/*
@brief Waiting for a signal.
@param handle, the handle for signal.
@param timeout, the timeout of waiting signal. unit ms. timeout is 0 or vpmdINFINITE, infinite wait signal.
*/
vip_status_e vipdrv_os_wait_signal(
    IN vipdrv_signal handle,
    IN vip_uint32_t timeout
    );

/*
@brief wake up wait signal.
     state is true, wake up waiting signal and chane the signal state to SET.
     state is false, change the signal to RESET so the vipdrv_os_wait_signal will block until
     the signal state changed to SET. block waiting signal function.
@param handle, the handle for signal.
*/
vip_status_e vipdrv_os_set_signal(
    IN vipdrv_signal handle,
    IN vip_bool_e state
    );
#endif

#if vpmdPOWER_OFF_TIMEOUT
/*
@brief create a system timer callback.
@param func, the callback function of this timer.
@param param, the paramete of the timer.
*/
vip_status_e vipdrv_os_create_timer(
    IN vipdrv_timer_func func,
    IN vip_ptr param,
    OUT vipdrv_timer *timer
    );

vip_status_e vipdrv_os_start_timer(
    IN vipdrv_timer timer,
    IN vip_uint32_t time_out
    );

vip_status_e vipdrv_os_stop_timer(
    IN vipdrv_timer timer
    );

/*
@brief destroy a system timer
*/
vip_status_e vipdrv_os_destroy_timer(
    IN vipdrv_timer timer
    );
#endif

vip_int32_t vipdrv_os_strcmp(
    IN const vip_char_t* s1,
    IN const vip_char_t* s2
    );

#endif /* __VIP_DRV_PORT_H__ */

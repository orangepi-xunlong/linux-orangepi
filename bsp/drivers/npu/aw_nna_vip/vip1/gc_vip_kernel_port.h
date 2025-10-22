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

#ifndef __GC_VIP_KERNEL_PORT_H__
#define __GC_VIP_KERNEL_PORT_H__

#include <vip_lite_common.h>
#include <gc_vip_common.h>
#include <gc_vip_kernel_share.h>

/* default wait signal timeout 100s */
#define WAIT_SIGNAL_TIMEOUT  (100*1000)

#define GCKVIPPTR_TO_UINT32(p) \
( \
    (vip_uint32_t) (gckvip_uintptr_t) (p)\
)

#define GCKVIPPTR_TO_UINT64(p) \
( \
    (vip_uint64_t) (gckvip_uintptr_t) (p)\
)

#define GCKVIPUINT64_TO_PTR(u) \
( \
    (vip_ptr) (gckvip_uintptr_t) (u)\
)

#if defined(LINUX)
/* used on Linux or Android kernel space system */
#define GCKVIP_DUMP_STACK()                 dump_stack();
#define GCKVIP_DO_DIV64(a, b, result)  {  \
    vip_uint64_t tmp = a;                 \
    do_div(tmp, b);                       \
    result = tmp;                         \
}
#elif defined (LINUXEMULATOR)
/* only for linux emulator, not used on FPGA and silicon */
#define GCKVIP_DUMP_STACK()                 dump_stack();
#define GCKVIP_DO_DIV64(a, b, result) (result = (vip_uint64_t)a / (vip_uint64_t)b)
#else
#define GCKVIP_DUMP_STACK()
#define GCKVIP_DO_DIV64(a, b, result) (result = (vip_uint64_t)a / (vip_uint64_t)b)
#endif

#define PRINTK(...)      gckvip_os_print(GCKVIP_PRINT,__VA_ARGS__)

#if (vpmdENABLE_DEBUG_LOG == 4)
#define PRINTK_D(...)    gckvip_os_print(GCKVIP_DEBUG, __VA_ARGS__)
#define PRINTK_I(...)    gckvip_os_print(GCKVIP_INFO, __VA_ARGS__)
#define PRINTK_W(...)    gckvip_os_print(GCKVIP_WARN, __VA_ARGS__)
#define PRINTK_E(...)    gckvip_os_print(GCKVIP_ERROR, __VA_ARGS__)
#elif (vpmdENABLE_DEBUG_LOG == 3)
#define PRINTK_D(...)
#define PRINTK_I(...)    gckvip_os_print(GCKVIP_INFO, __VA_ARGS__)
#define PRINTK_W(...)    gckvip_os_print(GCKVIP_WARN, __VA_ARGS__)
#define PRINTK_E(...)    gckvip_os_print(GCKVIP_ERROR, __VA_ARGS__)
#elif (vpmdENABLE_DEBUG_LOG == 2)
#define PRINTK_D(...)
#define PRINTK_I(...)
#define PRINTK_W(...)    gckvip_os_print(GCKVIP_WARN, __VA_ARGS__)
#define PRINTK_E(...)    gckvip_os_print(GCKVIP_ERROR, __VA_ARGS__)
#elif (vpmdENABLE_DEBUG_LOG == 1)
#define PRINTK_D(...)
#define PRINTK_I(...)
#define PRINTK_W(...)
#define PRINTK_E(...)    gckvip_os_print(GCKVIP_ERROR, __VA_ARGS__)
#else
#define PRINTK_D(...)
#define PRINTK_I(...)
#define PRINTK_W(...)
#define PRINTK_E(...)
#endif


typedef   void*        gckvip_mutex;
typedef   void*        gckvip_atomic;
typedef   void*        gckvip_signal;
typedef   void*        gckvip_thread;
typedef   void*        gckvip_timer;

typedef vip_int32_t (* gckvip_thread_func) (vip_ptr param);
typedef void (* gckvip_timer_func) (vip_ptr param);


/*
@brief Get hardware basic information for kernel.
*/
typedef struct _gckvip_hardware_info_t {
    vip_uint32_t           axi_sram_base;
    vip_uint32_t           axi_sram_size[gcdMAX_CORE];
    vip_uint32_t           vip_sram_base;
    vip_uint32_t           vip_sram_size[gcdMAX_CORE];
    volatile void          *vip_reg[gcdMAX_CORE];
    vip_uint32_t           *irq_queue[gcdMAX_CORE];
    volatile vip_uint32_t* volatile irq_value[gcdMAX_CORE];
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    void                   *cpu_virtual;
    void                   *cpu_virtual_kernel;
    phy_address_t          vip_physical;
    vip_uint32_t           vip_memsize;
#endif
#if vpmdENABLE_SYS_MEMORY_HEAP
    vip_uint32_t           sys_heap_size;
#endif
#if vpmdENABLE_RESERVE_PHYSICAL
    phy_address_t          reserve_phy_base;
    vip_uint32_t           reserve_phy_size;
#endif
    vip_uint32_t           core_count;
    vip_uint32_t           max_core_count;
    vip_uint32_t           device_core_number[gcdMAX_CORE];
    vip_uint32_t           core_fscale_percent;
} gckvip_hardware_info_t;

/*
@brief Read a hardware register.
@param reg_base, the base address of hardware register.
@param address, read the register address.
@param data, the data of address be read.
*/
vip_status_e gckvip_os_read_reg(
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
vip_status_e gckvip_os_write_reg(
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
@brief Allocate system memory in kernel space.
@param video_mem_size, the size of video memory heap.
@param size, Memory size to be allocated.
@param memory, Pointer to a variable that will hold the pointer to the memory.
*/
vip_status_e gckvip_os_allocate_memory(
    IN  vip_uint32_t size,
    OUT void **memory
    );

/*
@brief Free system memory in kernel space.
@param memory, Memory to be freed.
*/
vip_status_e gckvip_os_free_memory(
    IN void *memory
    );

/*
@brief Set memory to zero.
@param memory, Memory to be set to zero.
@param size, the size of memory.
*/
vip_status_e gckvip_os_zero_memory(
    IN void *memory,
    IN vip_uint32_t size
    );

/*
@brief copy src memory to dst memory.
@param dst, Destination memory.
@param src. Source memory
@param size, the size of memory should be copy.
*/
vip_status_e gckvip_os_memcopy(
    OUT void *dst,
    IN const void *src,
    IN vip_uint32_t size
    );

/*
@brief Waiting for the hardware interrupt
@param irq_queue, the wait queue of interrupt handle.
@param irq_value, a flag for interrupt.
@param mask, mask data for interrupt.
*/
vip_status_e gckvip_os_wait_interrupt(
    IN void *irq_queue,
    IN volatile vip_uint32_t* volatile irq_value,
    IN vip_uint32_t time_out,
    IN vip_uint32_t mask
    );

/*
@brief wakeup the queue manual
@param irq_queue, the wait queue of interrupt handle.
*/
vip_status_e gckvip_os_wakeup_interrupt(
    IN void *irq_queue
    );

/*
@brief Do some initialization works in KERNEL_OS layer.
@param video_mem_size, the size of video memory heap.
*/
vip_status_e gckvip_os_init(
    IN vip_uint32_t video_mem_size
    );

/*
@brief Do some un-initialization works in KERNEL_OS layer.
*/
vip_status_e gckvip_os_close(void);

/*
@brief Delay execution of the current thread for a number of milliseconds.
@param ms, delay unit ms. Delay to sleep, specified in milliseconds.
*/
void gckvip_os_delay(
    IN vip_uint32_t ms
    );

/*
@brief Delay execution of the current thread for a number of microseconds.
@param ms, delay unit us. Delay to sleep, specified in microseconds.
*/
void gckvip_os_udelay(
    IN vip_uint32_t us
    );

/*
@brief Print string on console.
@param msg, which message to be print.
*/
vip_status_e gckvip_os_print(
    gckvip_log_level level,
    IN const char *message,
    ...
    );

/*
@brief Print string to buffer.
@param, size, the size of msg.
@param msg, which message to be print.
*/
vip_status_e gckvip_os_snprint(
    IN vip_char_t *buffer,
    IN vip_uint32_t size,
    IN const vip_char_t *msg,
    ...);

/*
@brief get the current system time. return value unit us, microsecond
*/
vip_uint64_t gckvip_os_get_time(void);

/*
@brief Memory barrier function for memory consistency.
*/
vip_status_e gckvip_os_memorybarrier(void);

/*
@brief Get process id
*/
vip_uint32_t gckvip_os_get_pid(void);

/*
@brief Get kernel space thread id
*/
vip_uint32_t gckvip_os_get_tid(void);

#if vpmdENABLE_FLUSH_CPU_CACHE
/*
@brief Flush CPU cache for video memory which allocated from heap.
@param physical, the physical address should be flush CPU cache.
@param logical, the logical address should be flush.
@param size, the size of the memory should be flush.
@param type The type of operate cache. see gckvip_cache_type_e.
*/
vip_status_e gckvip_os_flush_cache(
    IN phy_address_t physical,
    IN void *logical,
    IN vip_uint32_t size,
    IN vip_uint8_t type
    );
#endif

/*
@brief Create a mutex for multiple thread.
@param mutex, create mutex pointer.
*/
vip_status_e gckvip_os_create_mutex(
    OUT gckvip_mutex *mutex
    );

vip_status_e gckvip_os_lock_mutex(
    gckvip_mutex mutex
    );

vip_status_e gckvip_os_unlock_mutex(
    IN gckvip_mutex mutex
    );

/*
@brief Destroy a mutex which create in gckvip_os_create_mutex..
@param mutex, create mutex pointer.
*/
vip_status_e gckvip_os_destroy_mutex(
    IN gckvip_mutex mutex
    );

#if vpmdENABLE_MULTIPLE_TASK || vpmdPOWER_OFF_TIMEOUT
/*
@brief Create a thread.
@param func, the thread handle function.
@param parm, parameter for this thread.
@param name, the thread name.
@param handle, the handle for thread.
*/
vip_status_e gckvip_os_create_thread(
    IN gckvip_thread_func func,
    IN vip_ptr parm,
    IN vip_char_t *name,
    OUT gckvip_thread *handle
    );

/*
@brief Destroy a thread.
@param handle, the handle for thread.
*/
vip_status_e gckvip_os_destroy_thread(
    IN gckvip_thread handle
    );
#endif

/*
@brief Create a atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_create_atomic(
    OUT gckvip_atomic *atomic
    );

/*
@brief Destroy a atomic which create in gckvip_os_create_atomic
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_destroy_atomic(
    IN gckvip_atomic atomic
    );

/*
@brief Set the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_set_atomic(
    IN gckvip_atomic atomic,
    IN vip_uint32_t value
    );

/*
@brief Get the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_uint32_t gckvip_os_get_atomic(
    IN gckvip_atomic atomic
    );

/*
@brief Increase the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_inc_atomic(
    IN gckvip_atomic atomic
    );

/*
@brief Decrease the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_dec_atomic(
    IN gckvip_atomic atomic
    );

#if vpmdENABLE_MULTIPLE_TASK || vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
/*
@brief Create a signal.
@param handle, the handle for signal.
*/
vip_status_e gckvip_os_create_signal(
    OUT gckvip_signal *handle
    );

vip_status_e gckvip_os_destroy_signal(
    IN gckvip_signal handle
    );

/*
@brief Waiting for a signal.
@param handle, the handle for signal.
@param timeout, the timeout of waiting signal. unit ms. timeout is 0 or vpmdINFINITE, infinite wait signal.
*/
vip_status_e gckvip_os_wait_signal(
    IN gckvip_signal handle,
    IN vip_uint32_t timeout
    );

/*
@brief wake up wait signal.
     state is true, wake up waiting signal and chane the signal state to SET.
     state is false, change the signal to RESET so the gckvip_os_wait_signal will block until
     the signal state changed to SET. block waiting signal function.
@param handle, the handle for signal.
*/
vip_status_e gckvip_os_set_signal(
    IN gckvip_signal handle,
    IN vip_bool_e state
    );
#endif

#if vpmdPOWER_OFF_TIMEOUT
/*
@brief create a system timer callback.
@param func, the callback function of this timer.
@param param, the paramete of the timer.
*/
vip_status_e gckvip_os_create_timer(
    IN gckvip_timer_func func,
    IN vip_ptr param,
    OUT gckvip_timer *timer
    );

vip_status_e gckvip_os_start_timer(
    IN gckvip_timer timer,
    IN vip_uint32_t time_out
    );

vip_status_e gckvip_os_stop_timer(
    IN gckvip_timer timer
    );

/*
@brief destroy a system timer
*/
vip_status_e gckvip_os_destroy_timer(
    IN gckvip_timer timer
    );
#endif

#endif /* __GC_VIP_KERNEL_PORT_H__ */

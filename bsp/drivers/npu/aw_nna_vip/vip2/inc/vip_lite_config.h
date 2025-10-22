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

#ifndef _VIP_LITE_CONFIG_H
#define _VIP_LITE_CONFIG_H

#include <vip_lite_common.h>

typedef enum _vip_log_level
{
    VIP_LOG_ERROR = 0,
    VIP_LOG_WARN =  1,
    VIP_LOG_INFO =  2,
    VIP_LOG_DEBUG = 3,
    VIP_LOG_PRINT = 4,
} vip_log_level;

#define vpmdINFINITE             ((vip_uint32_t) ~0U)

#define vipOnError(func)  \
    status = func; \
    if (status != VIP_SUCCESS) { \
        goto onError; \
    }

#define vipGoOnError(func)  \
    status = func; \
    goto onError;

#define vipIsNULL(object)  \
    if (VIP_NULL == object) { \
        status = VIP_ERROR_INVALID_ARGUMENTS; \
        goto onError; \
    }

/* align down. get base address after align */
#define VIP_ALIGN_BASE(n, align) \
( \
    ((n) & ~(vip_uint64_t)((align) - 1)) \
)

/* align up */
#define VIP_ALIGN(n, align) \
( \
    ((n) + ((align) - 1)) & ~(vip_uint64_t)((align) - 1) \
)

#define VIP_MAX(x, y) (((x) >= (y)) ? (x) : (y))


/*
  the maximum number of VIP cores that can be handled by this driver.
  This value can be set from 1 to 8(Max VIP core count) depending on the number of the cores in your chip.
  However, there is no problem keeping it at the default value of 8
  even if the core number of your chip is less than 8.
*/
#define vipdMAX_CORE                       8

/*
   The path of the generated files in driver. These files can be capture file,layer dump and hardware hang dump file.
   The default value is current directory.
   If the current path doesn't have permission to create files, you can configure it to other directory.
*/
#define vpmdSAVE_FILE_PATH                "./"


/*****************features define*********************/

/*
defined for printing 64-bits values. default print long date type.
you should change this define after changing vip_uint64_t and vip_address_t date type.

example, if VIP works in 32-bits system only need support 32-bits address.you can change as follows:
#change vip_uint64_t and vip_address_t defines in vip_lite_common.h
typedef unsigned long    vip_uint64_t;
typedef signed long      vip_int64_t;
typedef unsigned long    vip_address_t;
#change defines PRIx64
#define PRIx64                            "lx"
#define PRId64                            "ld"
*/
/* long long data with Hex format */
#define PRIx64                            "llx"
/* long long data with Dec format */
#define PRId64                            "llu"
#define PRIx32                            "x"
#define PRId32                            "u"
/* print a pointer with Hex format */
#if defined(LINUX)
#define PRPx                              "px"
#else
#define PRPx                              "p"
#endif

/*
	when set to 1, enable aw devfreq
 */
#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
#define vpmdENABLE_AW_DEVFREQ			1
#define vpmdENABLE_USER_CONTROL_POWER	1
#else
#define vpmdENABLE_AW_DEVFREQ			0
#endif

/*
choose driver runs on FPGA or silicon board.
    when set to 1, for building driver runs on FPGA or Emulator.
    when set to 0, driver runs on silicon.
*/
#ifndef vpmdFPGA_BUILD
#define vpmdFPGA_BUILD                     0
#endif

/*
   when set to 1, try to recovery hardware when hardware hang occurs.
*/
#ifndef vpmdENABLE_RECOVERY
#define vpmdENABLE_RECOVERY                0
#endif
/* the maximum number of try to recovery hardware times */
#define MAX_RECOVERY_TIMES      10

/*
   This define specified the number of milliseconds the system will wait
   before it broadcasts the VIP is stuck.
   In other words, it will define the timeout of any operation that needs to wait for the VIP.
   on FPGA, 3600000s as hang by default.
   on silicon, 20s as hang by default.
*/
#ifndef vpmdHARDWARE_TIMEOUT
#if vpmdFPGA_BUILD
#define vpmdHARDWARE_TIMEOUT               3600000
#else
#define vpmdHARDWARE_TIMEOUT               20000
#endif
#endif

/*
   Dump register and command buffer when HW hangs.
   set to 1. hang states, commands, module status information.
   set to 2. dump all information, states, commands and coeff in a file when VIP hang occurs.
             and a capture viplite_hang_capture_xxx_xxx.log file will be saved when hardware hangs.
*/
#ifndef vpmdENABLE_HANG_DUMP
#define vpmdENABLE_HANG_DUMP                2
#endif

/*
   Dump for hw capture.
   when set to 1, only capture states and command buffer.
   when set to 2, a capture of all register, states, command buffer, input, output, coeff.
   pre-process a capture file.
   On Linux/Android,
   when vpmdENABLE_CAPTURE set to 1 and vpmdENABLE_DEBUGFS is 1.
     and 'echo 0 > /sys/kernel/debug/viplite/rt_capture' to disable capture.
         'echo 1 > /sys/kernel/debug/viplite/rt_capture' to enable capture.
     the rt_capture value is 1 by default.
*/
#ifndef vpmdENABLE_CAPTURE
#define vpmdENABLE_CAPTURE                  0
#endif

/*
   Show out AHB register write/read log in vip-drv.
   when set to 1, print out log in vip-drv space when driver read/write AHB register.
*/
#ifndef vpmdENABLE_AHB_LOG
#define vpmdENABLE_AHB_LOG                  0
#endif

/*
   Flush video memories CPU cache. Vivante recommends is using Non-cache for video memory.
   when set to 0, compiler remove flush CPU cache source code and all video memory are non-cache.
   when set to 1, enable part flush CPU cache source.
                  video memory heap and dynamic allocate memory are default Non-cache on Linux.
                  we set to 1 is using for flush wrap user memory cache which created by
                  vip_create_buffer_from_handle on Linux.
   when set to 2, enable all video memory with CPU cache.
   Flush video memory CPU cache in vipdrv_mem_flush_cache().
   vpmdCPU_CACHE_LINE_SIZE: the size of CPU cache line.
*/
#ifndef vpmdENABLE_FLUSH_CPU_CACHE
#define vpmdENABLE_FLUSH_CPU_CACHE          2
#endif
#define vpmdCPU_CACHE_LINE_SIZE      64

/*
   Dump network intermediate activation data to DRAM.
   when set to 0, diable this feature.
   when set to 1, enable this feature.
                  Insert PPU copy operation in prepare network stage.
                  when run network, these copy operations will copy intermediate data to DRAM.
*/
#ifndef vpmdENABLE_LAYER_DUMP
#define vpmdENABLE_LAYER_DUMP               1
#endif

/*
   Enable operation performance profiling.
   when set to 0, disable operation profiling feature.
   when set to 1, submit and execute operations(commands) one by one. No log output for each operation.
   when set to 2, output the inference time and layer name for each operation.
   On Linux/Android,
   when vpmdENABLE_CNN_PROFILING set to 1 and vpmdENABLE_DEBUGFS is 1.
    and 'echo 0 > /sys/kernel/debug/viplite/rt_cnn_profile' to disable cnn_profiling.
        'echo 1 > /sys/kernel/debug/viplite/rt_cnn_profile' to enable cnn_profiling.
         the rt_cnn_profile value is 1 by default.
*/
#ifndef vpmdENABLE_CNN_PROFILING
#define vpmdENABLE_CNN_PROFILING            0
#endif

/*
   Enable NN profile data(NPD).
   insert profile commands for each NN operation to count performance data after running network.
   when set to 0, disable the feature.
   when set to 1, enable the feature in source code.
   Should used the vip_set_network(VIP_NETWORK_PROP_SET_ENABLE_NPD) to enable profile for network.
*/
#ifndef vpmdENABLE_NPD
#define vpmdENABLE_NPD                      1
#endif


/*
   Enable bandwidth profiling
   when set to 0, disable this feature.
   when set to 1, output bandwidth/cycle information after each submission command buffer.
                  AHB mode by default, read whole VIP core performance counters in the AHB registers by CPU.
*/
#ifndef vpmdENABLE_BW_PROFILING
#define vpmdENABLE_BW_PROFILING             0
#endif

/*
   system memory and video memory usage profiling.
   when set to 1, output current and peak memory usage for this NBG.
*/
#ifndef vpmdENABLE_MEMORY_PROFILING
#define vpmdENABLE_MEMORY_PROFILING         0
#endif

/*
   log print out
   when set to 0, disable all logs print out.
   when set to 1, only enable error logs print out.
   when set to 2, enable warning and error log.
   when set to 3, enable info, warning and error logs print out.
   when set to 4, enable debug, info, warning and error logs print out.
   On Linux/Android,
   when vpmdENABLE_DEBUG_LOG set to 3/4 and vpmdENABLE_DEBUGFS is 1.
      on Linux, you can 'export VIP_LOG_LEVEL=0' to disable info/debug user space logs.
      on Android, you can 'setprop vendor.vip_log_level=0' to disable info/debug user space logs.
      and 'echo 0 > /sys/kernel/debug/viplite/rt_log' to disable kernel space logs.
          'echo 1 > /sys/kernel/debug/viplite/rt_log' to enable kernel space logs.
      the rt_log value is 1 by default.
*/
#ifndef vpmdENABLE_DEBUG_LOG
#define vpmdENABLE_DEBUG_LOG                1
#endif

/*
   viplite support layers runs on CPU. The CPU layer should be implement by customer.
   when set to 1, enable viplite supports CPU layers.
   when set to 0, disable viplite supports CPU layers.
*/
#ifndef vpmdENABLE_SUPPORT_CPU_LAYER
#define vpmdENABLE_SUPPORT_CPU_LAYER        1
#endif

/*
   viplite use system memory heap to manage memory which used by CPU to store variable and struction.
   when set to 1, enable system memory heap.
   when set to 0, disable system memory heap.
*/
#ifndef vpmdENABLE_SYS_MEMORY_HEAP
#define vpmdENABLE_SYS_MEMORY_HEAP          1
#endif

/*
   viplite use video memory heap to manage memory which used by NPU to store commands and coeff.
   when set to 1, enable video memory heap.
   when set to 0, disable video memory heap. Maybe the VIP's MMU should be enable when video memory heap is disabled.
                  Because VIP needs to allocate memory dynamic from system.
   VIDEO_MEMORY_HEAP_READ_ONLY set to 1 indicate that the video memory heap is NPU read only memory
   if NPU's MMU is enabled.
*/
#ifndef vpmdENABLE_VIDEO_MEMORY_HEAP
#define vpmdENABLE_VIDEO_MEMORY_HEAP        1
#endif
/* when enable VIP's MMU and set the attr to 1, the heap memory read only by VIP */
#define VIDEO_MEMORY_HEAP_READ_ONLY    0

/*
  Support dynamic alloc memory from system as video memory which accesss by NPU.
  when set to 0, disable dynamic alloc memory from system.
  when set to 1, enable dynamic allocator
*/
#ifndef vpmdENABLE_VIDEO_MEMORY_DYNAMIC
#define vpmdENABLE_VIDEO_MEMORY_DYNAMIC     1
#endif

/*
   viplite driver supports MMU
   when 0 == vpmdENABLE_MMU, disable VIP's MMU, which means VIP only support access contiguous phyiscal memory.
   when 0 < vpmdENABLE_MMU, enable VIP's MMU, which means VIP can support access scatter physical memory.
       when 1 == vpmdENABLE_MMU: use one MMU page.
       when 2 == vpmdENABLE_MMU: per CPU process per MMU page.
*/
#ifndef vpmdENABLE_MMU
#if NPU_USER_IOMMU
#define vpmdENABLE_MMU 						0
#else
#define vpmdENABLE_MMU                      1
#endif
#endif
/* when MMU is enabled. enable this feature to support 40bit virtual address */
#ifndef vpmdENABLE_40BITVA
#define vpmdENABLE_40BITVA                  0
#endif

/*
   viplite driver support mutliple thread/process(multi-task).

   e.g.1. supports asynchronous process multi-graph(multi-task) with vip_trigger_network() and vip_wait_network().

   vip_trigger_network(Network_A)
   vip_trigger_network(Network_B)
   CPU do other tasks.
   vip_wait_network(Network_A)
   vip_wait_network(Netowrk_B)

   e.g.2 support multi-task running in different thread(process).

            Thread A                |            Thread B                   |           Process C
                                    |                                       |
   vip_trigger_network(Network_A)   |     vip_run_network(Network_B)        |      vip_run_network(Network_C)
   CPU do other tasks.              |                                       |
   vip_wait_network(Network_A)      |                                       |
                                    |                                       |
   when 0 == vpmdENABLE_MULTIPLE_TASK, disable multi-thread (SINGLE task management)
           Only one thread accessing the VIPLite API. Even if VIPLte has managed multi-devices of NPU.

   when 0 < vpmdENABLE_MULTIPLE_TASK, enable multi-thread.
           Application can create multiple networks in different threads.

       when 1 == vpmdENABLE_MULTIPLE_TASK (SINGLE task management)
           Application submited the task to the NPU hardware directly. One device can run one task.
       when 2 == vpmdENABLE_MULTIPLE_TASK (SERIAL SYNCHRONOUS task management)
           a. Application submited the task to queue.
           b. vip_driver read a task from queue, submit to hardware, after current task is done and
              software get interrupt from hardware, then software deploy next task.
       when 3 == vpmdENABLE_MULTIPLE_TASK (PARALLEL SYNCHRONOUS task management)
           a. Application submited the task to queue.
           b. vip_driver read a task from queue, submit to hardware and mark related hardware as busy,
              untill there is no task or idle hardware.
           when any task is done and software get interrupt from hardware, mark related hardware as idle.
       when 4 == vpmdENABLE_MULTIPLE_TASK (PARALLEL ASYNCHRONOUS task management) -- NOT SUPPORT YET
           a. Application submited the task to queue.
           use wait-link table to support asynchronous trigger.
           when hardware is inferring current task, deploy next task based on wait-link table even related
           hardware is busy, so hardware will infer next task immediately after current task is done.
   Remark:
       PARALLEL     : One device execute multi-tasks (use different cores) at the same time.
       ASYNCHRONOUS : The deployment by software and inference by hardware are asynchronous.
*/
#ifndef vpmdENABLE_MULTIPLE_TASK
#define vpmdENABLE_MULTIPLE_TASK             3
#endif
/* task dispatch counter debugging */
#define vpmdTASK_DISPATCH_DEBUG              0

/*
    The number of vip power domain, add this macro is support multivip maybe have more than one power domain.
    The single VIP core is one power domain. should set vpmdONE_POWER_DOMAIN 1 if VIP is single core.
    power gate the whole VIP cores. all cores and all cluster share one power domain.
    When set to 1, vip core have one power domain.
    When set to 0, vip core have independent power domain, we can control every vipcore power off and on in multi-core
*/
#ifndef vpmdONE_POWER_DOMAIN
#define vpmdONE_POWER_DOMAIN                 1
#endif

/*
    Enable power management. VIPLite power management only support without using wait-link loop to trigger hardware.
    When set to non-zero, VIP will power off automatically from idle state,
          and vpmdPOWER_OFF_TIMEOUT is also the default timeout in milliseconds.
    When set to 0, disable VIP power off feature.
    Default value is 10s power off timer timeout.
*/
#ifndef vpmdPOWER_OFF_TIMEOUT
#define vpmdPOWER_OFF_TIMEOUT               (10 * 1000)
#endif

/*
    Enable power on/off VIP core when the system suspend and resume.
    The suspend/resume machanism is often used in Linux/Android and other systems.
    When set to 1, enable suspend and resume VIP core feature.
    When set to 0, disable this feature.
    You can disable this feature when VIPLite driver works in RTOS, DSP or bare metal.
*/
#ifndef vpmdENABLE_SUSPEND_RESUME
#define vpmdENABLE_SUSPEND_RESUME           1
#endif

/*
    Enable applicaton can control VIP power. such as application can set VIP frequency,
    stop and re-start VIP, power on and off VIP.
    please refer vip_power_management() API to get more info.
    When set to 1, enable application control power.
    When set to 0, disable this feature.
    vip_power_management(network, VIP_POWER_PROPERTY_STOP, VIP_NULL).
*/
#ifndef vpmdENABLE_USER_CONTROL_POWER
#define vpmdENABLE_USER_CONTROL_POWER       0
#endif

/*
  turn off the internel clock immediately when the hardware inference task done(hardware is idle).
  when set to 0, disable turn off internel clock feature. instead of slow down clock to 1/64 when hardare idle.
  when set to 1, enable this feature.
*/
#ifndef vpmdENABLE_CLOCK_SUSPEND
#define vpmdENABLE_CLOCK_SUSPEND            1
#endif

/*
    supports Linux debugfs. sys nodes is in /sys/kernel/debug/viplite/
    when set to 1, enable viplite supports debugfs.
    when set to 0, disable viplite supports debugfs.
*/
#ifndef vpmdENABLE_DEBUGFS
#if defined(LINUX)
#define vpmdENABLE_DEBUGFS                  1
#else
#define vpmdENABLE_DEBUGFS                  0
#endif
#endif

/*
    supports create VIP buffer from fd(file descriptor), dma-buf. only Linux/Android supports this feature.
    vpmdENABLE_CREATE_BUF_FD should be set to 1 if want to used the  vip_create_buffer_from_fd() to create vip buffer.
    when set to 1, enable.
    when set to 0, disable it.
*/
#ifndef vpmdENABLE_CREATE_BUF_FD
#if defined(LINUX)
#define vpmdENABLE_CREATE_BUF_FD             1
#else
#define vpmdENABLE_CREATE_BUF_FD             0
#endif
#endif

/*
    Enable node module. Only scaler engine(hardware scaling) support node module.
*/
#ifndef vpmdENABLE_NODE
#define vpmdENABLE_NODE                      0
#endif

/*
    Enable VIPLite group mode. The group mode can support two application scenarios now.
    a. runs multiple NBG(network) only one interrupt at last of network in group mode.
       (combile multiple network in one task)
    b. support batch network for network in group mode.
       (combine multiple batch network in one task)
    When set to 1, enable group mode.
    When set to 0, disable group mode.
*/
#ifndef vpmdENABLE_GROUP_MODE
#define vpmdENABLE_GROUP_MODE                1
#endif

/*
    Enable VIPLite supports change PPU node parameter
    (PPU kernel param<the input/output shape of PPU kernel> and PPU param<local size and work dims of PPU kernel>).
    When set to 1, enable change PPU param feature.
    When set to 0, disable change PPU param feature.
*/
#ifndef vpmdENABLE_CHANGE_PPU_PARAM
#define vpmdENABLE_CHANGE_PPU_PARAM          1
#endif

/*
    Function API calls trace. print out logs when expose API is calls.
    When set to 1, enable
    When set to 0, disable
*/
#ifndef vpmdENABLE_FUNC_TRACE
#define vpmdENABLE_FUNC_TRACE                0
#endif

/*
    Reduce memory footprint by releasing unnecessary system memory.
    When set to 1, enable.
        system memory for NBG header: patch table, lcd_table, sh_op_table will be released.
        system memory for load states be released.
    When set to 0, disable
    The following functions need to be paid attention when enable it
        1. The vip_prepare_network can't be called repeatedlly.
           so always need vip_create_network before calling vip_prepare_network.
        2. group mode can't used.
        3. dup network can't used.
        4. will affect some debug functions.  Capture(init_command).
*/
#ifndef vpmdENABLE_REDUCE_MEMORY
#define vpmdENABLE_REDUCE_MEMORY             0
#endif

/*
   After reserved this contiguous phyiscal, driver fill mmu page table in vip_init stages,
   when application call create_buffer_form_physical don't fill mmu page table again.
   so speed up API calls.
   This feature is avaliable in the case of MMU enabled. don't need this feature when mmu is disabled.
   when set to 0, disable this feature.
   when set to 1, enable this feature.
*/
#ifndef vpmdENABLE_RESERVE_PHYSICAL
#define vpmdENABLE_RESERVE_PHYSICAL          1
#endif

/*
   Save NBG into a file, the NBG is passed to driver by the application through vip_create_network.
   Save both inputs and outputs of network into files.
   when set to 0, disable this feature.
   when set to 1, enable this feature.
   On Linux/Android,
   when vpmdDUMP_NBG_RESOURCE set to 1 and vpmdENABLE_DEBUGFS is 1.
     and 'echo 0 > /sys/kernel/debug/viplite/rt_dump_nbg' to disable dump.
         'echo 1 > /sys/kernel/debug/viplite/rt_dump_nbg' to enable dump.
     the dump_nbg value is 1 by default.
*/
#ifndef vpmdDUMP_NBG_RESOURCE
#define vpmdDUMP_NBG_RESOURCE                0
#endif

/*
  polling hardware inference a task is done instead of waiting for interrupt.
  when set to 0, use waiting hardware interrupt.
  when set to 1, use polling hardware inference is done.
*/
#ifndef vpmdENABLE_POLLING
#define vpmdENABLE_POLLING                   0
#endif

/*
  task preemption.
  The smallest unit is a network. If a high-priority network commits when a low-priority network is processing,
  wait for the completion of the low-priority network and process the high-priority network immediately.
  when set to 1, use vip_set_network(VIP_NETWORK_PROP_SET_PRIORITY) to set the priority of this network.
                 the driver run network according to the priority order of the these networks.
  when set to 0, FIFO strategy runs networks.
  this flag is valid only if vpmdENABLE_MULTIPLE_TASK is enabled.
*/
#ifndef vpmdENABLE_PREEMPTION
#define vpmdENABLE_PREEMPTION                1
#endif

/*
  cancel netowrk running on vip hardware after netowrk is commited.
  when set to 0, disable cancel netowrk function.
  when set to 1, only enable hardware cancel network function, we can cancel a netowrk running on vip hardware.
  when set to 2, enable hardware and software cancel netowrk function, we can cancel a netowrk running on vip hardware.
  Always used hardware cancel if HW supports job cancellation feature.
*/
#ifndef vpmdENABLE_CANCELATION
#define vpmdENABLE_CANCELATION               1
#endif

/*
   expose an API in application, APP can get network inference time by
   vip_query_network(VIP_NETWORK_PROP_PROFILING) API.
   when set to 0, diable the feature.
   when set to 1, enable the feature.
*/
#ifndef vpmdENABLE_APP_PROFILING
#define vpmdENABLE_APP_PROFILING             1
#endif

/*
  CRC checksum for NBG file and Network.
  when set to 0, disable this feature.
  when set to 1, get the checksum value after preparing network and
                 Compare checksum value when hardware hang occurs. that value needs to be consistent.
                 Calculate the checksum in vip_prepare_network() stage, will increase the
                 vip_prepare_network() excecution time.
  when set to 2, NBG file checksum. Calculate the checksum of NBG file in the create_network stage.
                 And compare with checksume value in NBG. network->nbg_desc->fixed->header->nbg_crc_sum.
                 Since NBG format 0x00010016 supports NBG file checksum feature.
                 Calculate the checksum in vip_create_network() stage, will increase it excecution time.
*/
#ifndef vpmdENABLE_CHECKSUM
#define vpmdENABLE_CHECKSUM                  0
#endif

/*
   Bypass Trigger NPU or bypass NPU's engines(NN,TP,PPU).
   The network will not be executed on NPU core when HARDWARE_BYPASS_MODE.
   Just runs all driver code on CPU.
   When set to 0, NPU core will exceted netowrk.
   When set to 0x1, bypass trigger NN engine.
   When set to 0x2, bypass trigger TP engine.
   When set to 0x4, bypass trigger PPU engine.
   When set to 0x100, vip core not exceted network, just runs all driver code on CPU.
*/
#ifndef vpmdHARDWARE_BYPASS_MODE
#define vpmdHARDWARE_BYPASS_MODE             0x0
#endif

/*
  Coeff preloading function. Support this feature after NBG format 0x0001001A.
  When == 0, disable this feature.
  When >= 1, support preload coeff into the memory specified by user:
             vip_set_network(VIP_NETWORK_PROP_SET_COEFF_MEMORY)
  When >= 2, viplite will preload coeff into VIP SRAM before inference automatically.
             Due to the VIP SRAM is shared by all tasks, so whenever the task is
             changed, the preloading action is needed.
*/
#ifndef vpmdPRELOAD_COEFF
#define vpmdPRELOAD_COEFF                    2
#endif

/*
   In common usage, The network memory pool only can be set before calling vip_prepare_network().
   Such a limit is not smart in some scenarios.
   So, define such switch to let viplite support setting the memory pool(vip_set_network)
   after calling vip_prepare_network().
   when set to 0, diable the feature.
   when set to 1, enable the feature.
*/
#ifndef vpmdDYNAMIC_CHANGE_MEM_POOL
#define vpmdDYNAMIC_CHANGE_MEM_POOL          1
#endif

/*
   Disable FE per-fetch command. After engine executes a command complete, the FE can fetch next command.
   when set to 0, diable the feature.
   when set to 1, enable the feature.
*/
#ifndef vpmdENABLE_STALL_FE
#define vpmdENABLE_STALL_FE                  0
#endif

/*
   The functional safety, needs NPU hardware support. Only hardware support FUSA feature can set the flag to 1.
   when set to 0, diable the feature.
   when set to 1, enable the feature.
*/
#ifndef vpmdENABLE_FUSA
#define vpmdENABLE_FUSA                      0
#endif

/*
   To support large NBG file.
   when set to 0, diable the feature to reduce memory cast.
   when set to 1, enable the feature to support parse NBG file larger than 4G.
*/
#ifndef vpmdNBG_SIZE_OVER_4G
#define vpmdNBG_SIZE_OVER_4G                 0
#endif

/*///////////Correct definations Section, plese don't modify this section START//////////////*/
#ifndef vpmdAUTO_CORRECT_CONFLICTS
#define vpmdAUTO_CORRECT_CONFLICTS           0
#endif
/*
Correct definitions means that some functions are in conflict with each other,
and we must close some functions when these conflicting functions are enabled.
*/
/*
disable multi-task when CNN profiling is enabled.
*/
#if vpmdENABLE_CNN_PROFILING
#if vpmdAUTO_CORRECT_CONFLICTS
#undef  vpmdENABLE_MULTIPLE_TASK
#define vpmdENABLE_MULTIPLE_TASK             1
#undef vpmdENABLE_CLOCK_SUSPEND
#define vpmdENABLE_CLOCK_SUSPEND             0
#else
#if vpmdENABLE_MULTIPLE_TASK
#error "Only SINGLE task management when vpmdENABLE_CNN_PROFILING is enabled"
#endif
#if vpmdENABLE_CLOCK_SUSPEND
/* nn not idle when event irq rise up.
   eg: nn small batch is enabled, not 0x0801429 cmd at the end of NN */
#error "vpmdENABLE_CLOCK_SUSPEND can't be enabled together with vpmdENABLE_CNN_PROFILING"
#endif
#endif
#endif

#if vpmdENABLE_CAPTURE || vpmdENABLE_GROUP_MODE
#if vpmdAUTO_CORRECT_CONFLICTS
#undef vpmdENABLE_REDUCE_MEMORY
#define vpmdENABLE_REDUCE_MEMORY             0
#else
#if vpmdENABLE_REDUCE_MEMORY
#error "vpmdENABLE_REDUCE_MEMORY can't be enabled together with vpmdENABLE_CAPTURE or vpmdENABLE_GROUP_MODE"
#endif
#endif
#endif

#if !vpmdENABLE_MMU
#if vpmdAUTO_CORRECT_CONFLICTS
#undef vpmdENABLE_RESERVE_PHYSICAL
#define vpmdENABLE_RESERVE_PHYSICAL          0
#else
#if vpmdENABLE_RESERVE_PHYSICAL
#error "vpmdENABLE_RESERVE_PHYSICAL should be enabled together with vpmdENABLE_MMU"
#endif
#endif
#if vpmdAUTO_CORRECT_CONFLICTS
#undef vpmdENABLE_40BITVA
#define vpmdENABLE_40BITVA                   0
#else
#if vpmdENABLE_40BITVA
#error "vpmdENABLE_40BITVA should be enabled together with vpmdENABLE_MMU"
#endif
#endif
#endif

#if (4 == vpmdENABLE_MULTIPLE_TASK)
#if vpmdAUTO_CORRECT_CONFLICTS
#undef vpmdENABLE_POLLING
#define vpmdENABLE_POLLING                   0
#if vpmdENABLE_MMU > 1
#undef vpmdENABLE_MMU
#define vpmdENABLE_MMU                       1
#endif
#else
#if vpmdENABLE_POLLING
#error "vpmdENABLE_POLLING can't be enabled when 4 == vpmdENABLE_MULTIPLE_TASK"
#endif
#if vpmdENABLE_MMU > 1
#error "cannot switch MMU page when 4 == vpmdENABLE_MULTIPLE_TASK"
#endif
#endif
#endif

#if (2 > vpmdENABLE_MULTIPLE_TASK)
#if vpmdAUTO_CORRECT_CONFLICTS
#undef vpmdTASK_DISPATCH_DEBUG
#define vpmdTASK_DISPATCH_DEBUG              0
#else
#if vpmdTASK_DISPATCH_DEBUG
#error "vpmdTASK_DISPATCH_DEBUG can't be enabled when 2 > vpmdENABLE_MULTIPLE_TASK"
#endif
#endif
#endif

#if !vpmdENABLE_VIDEO_MEMORY_HEAP && !vpmdENABLE_VIDEO_MEMORY_DYNAMIC
#error "error heap and dynamic video memory management are disabled"
#endif

#if vpmdENABLE_MMU && !vpmdENABLE_VIDEO_MEMORY_HEAP
#error "must enable video memory heap when enable mmu"
#endif

/*///////////Correct definations Section, plese don't modify this section END//////////////*/


/*///////////Summarizes Definations Section, plese don't modify this section START/////////*/
/*
Summarizes definition means to redefine some compiler switches based on the previous definition,
to facilitate the use of new definitions in code, and make reading the code clearer.
*/
#if vipdMAX_CORE > 1
#define vpmdENABLE_MULTIPLE_VIP              1
#else
#define vpmdENABLE_MULTIPLE_VIP              0
#endif

#if vpmdPOWER_OFF_TIMEOUT || vpmdENABLE_USER_CONTROL_POWER || vpmdENABLE_SUSPEND_RESUME || vpmdENABLE_CLOCK_SUSPEND
#define vpmdENABLE_POWER_MANAGEMENT          1
#else
#define vpmdENABLE_POWER_MANAGEMENT          0
#endif

/* enable save mmu page table in capture file when both mmu and capture is enabled */
#if vpmdENABLE_MMU && ((vpmdENABLE_CAPTURE > 1) | (vpmdENABLE_HANG_DUMP > 1))
#define vpmdENABLE_CAPTURE_MMU               1
#else
#define vpmdENABLE_CAPTURE_MMU               0
#endif

#if vpmdENABLE_FLUSH_CPU_CACHE > 1
#define vpmdENABLE_VIDEO_MEMORY_CACHE        1
#else
#define vpmdENABLE_VIDEO_MEMORY_CACHE        0
#endif

/* thread number is 1 when multiple task is disabled or it must not be less than 1 */
#if !vpmdENABLE_MULTIPLE_TASK
#undef vpmdENABLE_PREEMPTION
#define vpmdENABLE_PREEMPTION                0
#endif

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1) || vpmdENABLE_CHECKSUM
#define vpmdENABLE_RECORD_INFO               1
#else
#define vpmdENABLE_RECORD_INFO               0
#endif

/* Choose Task Manager */
#if (4 == vpmdENABLE_MULTIPLE_TASK)
#define vpmdTASK_PARALLEL_ASYNC              1
#define vpmdTASK_PARALLEL_SYNC               0
#define vpmdTASK_SERIAL_SYNC                 0
#define vpmdTASK_SINGLE                      0
#elif (3 == vpmdENABLE_MULTIPLE_TASK)
#define vpmdTASK_PARALLEL_ASYNC              0
#define vpmdTASK_PARALLEL_SYNC               1
#define vpmdTASK_SERIAL_SYNC                 0
#define vpmdTASK_SINGLE                      0
#elif (2 == vpmdENABLE_MULTIPLE_TASK)
#define vpmdTASK_PARALLEL_ASYNC              0
#define vpmdTASK_PARALLEL_SYNC               0
#define vpmdTASK_SERIAL_SYNC                 1
#define vpmdTASK_SINGLE                      0
#else
#define vpmdTASK_PARALLEL_ASYNC              0
#define vpmdTASK_PARALLEL_SYNC               0
#define vpmdTASK_SERIAL_SYNC                 0
#define vpmdTASK_SINGLE                      1
#endif

#if vpmdTASK_PARALLEL_ASYNC || vpmdTASK_PARALLEL_SYNC || vpmdTASK_SERIAL_SYNC
#define vpmdTASK_QUEUE_USED                  1
#else
#define vpmdTASK_QUEUE_USED                  0
#endif

#if vpmdENABLE_APP_PROFILING || vpmdENABLE_BW_PROFILING
#define vpmdENABLE_TASK_PROFILE              1
#else
#define vpmdENABLE_TASK_PROFILE              0
#endif

#ifndef vpmdTASK_CMD_DUMP
#define vpmdTASK_CMD_DUMP                    0
#endif

#if (3 == vpmdENABLE_MMU)
#define vpmdMMU_SINGLE_PAGE                  0
#define vpmdMMU_PER_PROCESS                  0
#define vpmdMMU_PER_VIP_TASK                 1
#elif (2 == vpmdENABLE_MMU)
#define vpmdMMU_SINGLE_PAGE                  0
#define vpmdMMU_PER_PROCESS                  1
#define vpmdMMU_PER_VIP_TASK                 0
#elif (1 == vpmdENABLE_MMU)
#define vpmdMMU_SINGLE_PAGE                  1
#define vpmdMMU_PER_PROCESS                  0
#define vpmdMMU_PER_VIP_TASK                 0
#else
#define vpmdMMU_SINGLE_PAGE                  0
#define vpmdMMU_PER_PROCESS                  0
#define vpmdMMU_PER_VIP_TASK                 0
#endif

#if (vpmdTASK_PARALLEL_ASYNC || vpmdTASK_PARALLEL_SYNC) && vpmdENABLE_MULTIPLE_VIP
#define vpmdTASK_SCHEDULE                    1
#else
#define vpmdTASK_SCHEDULE                    0
#endif

/* Utility macro. */
typedef vip_uint8_t*         vip_uint8_t_ptr;
typedef vip_float_t*         vip_float_t_ptr;
typedef vip_uint32_t*        vip_uint32_t_ptr;
typedef vip_int32_t*         vip_int32_t_ptr;
typedef vip_char_t*          vip_char_t_ptr;
typedef vip_int8_t*          vip_int8_t_ptr;
typedef vip_int16_t*         vip_int16_t_ptr;
typedef vip_int64_t*         vip_int64_t_ptr;

/*
vip_address_t: memory address used by VIP.
  when MMU is enabled, vip_address_t is the virtual address of VIP.
  when MMU is disabled, vip_address_t is the physical address of VIP.
*/

/* vip_physical_t: the physical of VIP and CPU. */
typedef vip_uint64_t         vip_physical_t;
/* vip_virtual_t: virtual address of VIP.*/
#if vpmdENABLE_40BITVA
typedef vip_uint64_t         vip_virtual_t;
#else
typedef vip_uint32_t         vip_virtual_t;
#endif

#if vpmdNBG_SIZE_OVER_4G
typedef vip_uint64_t         vip_size_t;
#else
typedef vip_uint32_t         vip_size_t;
#endif

/*///////////Summarizes Definations Section, plese don't modify this section END///////////*/

#endif

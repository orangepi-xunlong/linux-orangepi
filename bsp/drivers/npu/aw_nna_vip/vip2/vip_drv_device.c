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

#include <vip_drv_device.h>
#if vpmdENABLE_POWER_MANAGEMENT
#include <vip_drv_pm.h>
#endif
#include <vip_drv_video_memory.h>
#include <vip_drv_context.h>

/*
@brief Get hardware object from device.
*/
vipdrv_hardware_t* vipdrv_get_hardware(
    vipdrv_device_t* device,
    vip_uint32_t core_index
    )
{
    vipdrv_hardware_t* hardware = VIP_NULL;

    if (core_index < device->hardware_count) {
        hardware = &device->hardware[core_index];
    }
    else {
        PRINTK_E("fail to get hw%d from dev%d, total core_cnt=%d\n",
                 core_index, device->device_index, device->hardware_count);
    }

    return hardware;
}

vipdrv_device_t* vipdrv_get_device(
    vip_uint32_t device_index
    )
{
    vipdrv_device_t *device = VIP_NULL;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

    if (device_index < context->device_count) {
        device = &context->device[device_index];
    }
    else {
        PRINTK_E("fail to get device%d, total dev_cnt=%d\n", device_index, context->device_count);
    }

    return device;
}

vip_status_e vipdrv_device_hw_reset(
    vipdrv_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    status = vipdrv_hw_reset(hw);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to software reset hardware, status=%d.\n", status);
        vipGoOnError(status);
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

onError:
    return status;
}

vip_status_e vipdrv_device_hw_init(
    vipdrv_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
#if vpmdENABLE_POWER_MANAGEMENT
#if vpmdPOWER_OFF_TIMEOUT
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_DISABLE_TIMER, VIP_NULL);
#endif
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
#endif
    status = vipdrv_hw_init(hw);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to hardware initialize, status=%d.\n", status);
        vipGoOnError(status);
    }
#if vpmdENABLE_POWER_MANAGEMENT
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
#if vpmdPOWER_OFF_TIMEOUT
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_ENABLE_TIMER, VIP_NULL);
#endif
#endif
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

onError:
    return status;
}

/*
@brief To run any initial commands after clock on / power on.
    remap SRAM, configure USC, configure debug mode.
*/
vip_status_e vipdrv_device_prepare_initcmd(
    vipdrv_device_t *device
    )
{
    vipdrv_context_t *context = device->context;
    vip_uint32_t cmd_size = sizeof(vip_uint32_t) * (20 + 18);
    vip_uint32_t *logical = VIP_NULL;
    vip_uint32_t buf_size = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t count = 0;
    vip_uint32_t reg_data = 0;
    vip_uint32_t alloc_flag = VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL |
                              VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS |
                              VIPDRV_VIDEO_MEM_ALLOC_4GB_ADDR |
                              VIPDRV_VIDEO_MEM_ALLOC_NPU_READ_ONLY;
    vip_physical_t physical_tmp = 0;
    vip_uint32_t axi_sram_size = 0;
    vip_uint32_t i = 0;
    vip_address_t axi_sram_address = 0;

    if (vip_true_e == vipdrv_context_support(VIP_HW_FEATURE_NN_XYDP0)) {
        #if vpmdENABLE_MMU
        axi_sram_address = context->axi_sram_address;
        #else
        axi_sram_address = context->axi_sram_physical;
        #endif
    }
    else {
        axi_sram_address = context->axi_sram_physical;
    }

#if !vpmdENABLE_VIDEO_MEMORY_CACHE
    alloc_flag |= VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE;
#endif
    PRINTK_I("start prepare init commands\n");

    for (i = 0; i < context->device_count; i++) {
        axi_sram_size += context->axi_sram_size[i];
    }

    if (context->vip_sram_size[0] > 0) {
        cmd_size += sizeof(vip_uint32_t) * 2;
    }
    if (axi_sram_size > 0) {
        cmd_size += sizeof(vip_uint32_t) * 4;
        if (0xff00000000 & (axi_sram_address + axi_sram_size)) {
            cmd_size += sizeof(vip_uint32_t) * 2;
        }
    }

    buf_size = cmd_size + APPEND_COMMAND_SIZE;
    buf_size = VIP_ALIGN(buf_size, vpmdCPU_CACHE_LINE_SIZE);
    vipOnError(vipdrv_mem_allocate_videomemory(buf_size, &device->init_command.mem_id, (void **)&logical,
        &physical_tmp, vipdMEMORY_ALIGN_SIZE, alloc_flag, 0, (vip_uint64_t)1 << device->device_index, 0));
    vipdrv_os_zero_memory(logical, buf_size);
    device->init_command.logical = (void*)logical;
    device->init_command.physical = (vip_uint32_t)physical_tmp;

    logical[count++] = 0x0801028A;
    logical[count++] = 0x00000011;
    logical[count++] = 0x08010E13;
    logical[count++] = 0x00000002;
    logical[count++] = 0x08010E12;/* nn/tp debug register */
    reg_data = SETBIT(0, 16, 1) | SETBIT(0, 27, 1) | SETBIT(0, 28, 1);
    logical[count++] = reg_data;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBITS(0, 24, 25, 3) | 0x0;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBITS(0, 24, 25, 3) | 0x1;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBITS(0, 24, 25, 3) | 0x2;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBITS(0, 24, 25, 3) | 0x3;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBITS(0, 24, 25, 3) | 0x4;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBITS(0, 24, 25, 3) | 0x5;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBITS(0, 24, 25, 3) | 0x6;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBITS(0, 24, 25, 3) | 0x7;
    logical[count++] = 0x08010E21;
    logical[count++] = 0x00020000;
    logical[count++] = 0x08010e02;
    logical[count++] = 0x00000701;
    logical[count++] = 0x48000000;
    logical[count++] = 0x00000701;
    logical[count++] = 0x0801022c;
    logical[count++] = 0x0000001f;
    if (context->vip_sram_size[0] > 0) {
        logical[count++] = 0x08010e4e;
        logical[count++] = context->vip_sram_address;
    }
    if (axi_sram_size > 0) {
        logical[count++] = 0x08010e4f;
        logical[count++] = axi_sram_address;
        logical[count++] = 0x08010e50;
        logical[count++] = axi_sram_address + axi_sram_size;
        if (0xff00000000 & (axi_sram_address + axi_sram_size)) {
            vip_uint32_t tmp = 0;
            logical[count++] = 0x08010e59;
            tmp = SETBITS(tmp, 8, 15, axi_sram_address >> 32);
            tmp = SETBITS(tmp, 16, 23, (axi_sram_address + axi_sram_size) >> 32);
            logical[count++] = tmp;
        }
    }
    logical[count++] = 0x08010e02;
    logical[count++] = 0x30000701;
    logical[count++] = 0x48000000;
    logical[count++] = 0x30000701;
    logical[count++] = 0x10000000;
    logical[count++] = 0;
    device->init_command_size = count * sizeof(vip_uint32_t);

    if ((count * sizeof(vip_uint32_t)) > buf_size) {
        PRINTK_E("fail to init commands, size is overflow\n");
        vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
    cmd_size = count * sizeof(vip_uint32_t);
    device->init_command.size = cmd_size;

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_mem_flush_cache(device->init_command.mem_id, VIPDRV_CACHE_FLUSH);
#endif
onError:
    return status;
}

vip_status_e vipdrv_device_wait_idle(
    vipdrv_device_t *device,
    vip_uint32_t timeout,
    vip_uint8_t is_fast
    )
{
    vip_status_e status = VIP_SUCCESS;

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
#if vpmdENABLE_POWER_MANAGEMENT
    if (VIPDRV_POWER_ON & hw->pwr_management.state)
#endif
    {
        #if vpmdENABLE_CLOCK_SUSPEND
        vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
        #endif
        /* hardware is idle status, power off hardware immediately */
        status |= vipdrv_hw_wait_idle(hw, timeout, is_fast, EXPECT_IDLE_VALUE_ALL_MODULE);
        #if vpmdENABLE_CLOCK_SUSPEND
        vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
        #endif
        if (status != VIP_SUCCESS) {
            PRINTK_E("error, VIP not going to idle.\n");
        }
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    return status;
}

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
#include <gc_vip_hardware.h>
#include <gc_vip_kernel.h>
#include <gc_vip_kernel_port.h>
#include <gc_vip_kernel_pm.h>
#include <gc_vip_kernel_drv.h>
#include <gc_vip_kernel_util.h>
#include <gc_vip_kernel_debug.h>


vip_status_e gckvip_pm_set_frequency(
    gckvip_device_t *device,
    vip_uint8_t fscale_percent
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t bk_data[gcdMAX_CORE] = {0};
    vip_uint32_t value = 0;
    vip_uint32_t sh_value = 0;
    vip_uint32_t fscale = 0;
    gckvip_context_t *context = gckvip_get_context();
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_uint32_t i = 0;

    if (vip_false_e == gckvip_hw_support_clkgating(context)) {
        return VIP_SUCCESS;
    }
#if vpmdENABLE_CLOCK_SUSPEND
    if (vip_true_e == gckvip_os_get_atomic(device->clock_suspended)) {
        return VIP_SUCCESS;
    }
#endif
    if ((gckvip_os_get_atomic(device->last_fscale_percent) != fscale_percent) ||
        (1 == fscale_percent)) {
        if (0 == fscale_percent) {
            PRINTK_E("set hardware frequency, fscale percent is 0. force set to minimum value 1.\n");
            fscale_percent = 1;
        }

        if (fscale_percent > 100) {
            PRINTK_E("set hardware frequency, fscale percent is more than 100."
                     "force set to maximum vaue 100.\n");
            fscale_percent = 100;
        }

        fscale = 64 * fscale_percent / 100;
        if (fscale < 1) {
            fscale = 1;
        }
        else if (fscale > 64) {
            fscale = 64;
        }
        PRINTK_D("set hardware frequency fscale=%d, percent=%d\n", fscale, fscale_percent);
        for (i = 0; i < device->hardware_count; i++) {
            hardware = &device->hardware[i];
            /* get power control */
            gckvip_read_register(hardware, 0x00104, &bk_data[i]);
            /* disable all clock gating */
            gckvip_write_register(hardware, 0x00104, 0x00ffffff);
        }
        /* scale the core clock */
        for (i = 0; i < device->hardware_count; i++) {
            hardware = &device->hardware[i];
            gcOnError(gckvip_read_register(hardware, 0x00000, &value));
            value = SETBIT(value, 0, 0);
            value = SETBIT(value, 1, 0);
            value = SETBIT(value, 2, fscale);
            value = SETBIT(value, 9, 1);
            gcOnError(gckvip_write_register(hardware, 0x00000, value));

            gckvip_os_udelay(5);
            value = SETBIT(value, 9, 0);
            gcOnError(gckvip_write_register(hardware, 0x00000, value));
        }
        /* scale the ppu clock */
        for (i = 0; i < device->hardware_count; i++) {
            hardware = &device->hardware[i];
            gckvip_read_register(hardware, 0x0010C, &sh_value);
            sh_value = SETBIT(sh_value, 16, 0);
            sh_value = SETBIT(sh_value, 17, 1);
            sh_value = SETBIT(sh_value, 1, fscale);
            sh_value = SETBIT(sh_value, 0, 1);
            gckvip_write_register(hardware, 0x0010C, sh_value);
        }
        /* done loading the PPU frequency scaler. */
        for (i = 0; i < device->hardware_count; i++) {
            hardware = &device->hardware[i];
            gckvip_read_register(hardware, 0x0010C, &sh_value);
            sh_value = SETBIT(sh_value, 0, 0);
            gckvip_write_register(hardware, 0x0010C, sh_value);

            /* restore all clock gating. */
            gckvip_write_register(hardware, 0x00104, bk_data[i]);
        }

        gckvip_os_set_atomic(device->last_fscale_percent, fscale_percent);
    }

onError:
    return status;
}

#if vpmdENABLE_CLOCK_SUSPEND
vip_status_e gckvip_pm_disable_clock(
    gckvip_device_t *device
    )
{
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t value = 0;
    vip_uint32_t i = 0;
#if vpmdONE_POWER_DOMAIN
    gckvip_context_t* context = gckvip_get_context();
    gckvip_pm_t* pm = &context->sp_management;
#else
    gckvip_pm_t* pm = &device->dp_management;
#endif
    gckvip_lock_recursive_mutex(&pm->mutex);
    if (0 < gckvip_os_get_atomic(device->disable_clock_suspend) ||
        vip_true_e == gckvip_os_get_atomic(device->clock_suspended)) {
        gckvip_unlock_recursive_mutex(&pm->mutex);
        return VIP_SUCCESS;
    }

#if vpmdENABLE_CAPTURE_IN_KERNEL
    PRINTK("#[disable clock +]");
#endif
    PRINTK_D("hardware clock suspend.\n");

    gcOnError(gckvip_os_set_atomic(device->clock_suspended, vip_true_e));
    for (i = 0; i < device->hardware_count; i++) {
        vip_bool_e is_idle = 0;
        hardware = &device->hardware[i];
        is_idle = gckvip_hw_query_idle(hardware, 10, vip_true_e);
        if (vip_false_e == is_idle) {
            PRINTK_E("fail hardware%d not going to idle\n", i);
            continue;
        }
        gcOnError(gckvip_read_register(hardware, 0x00000, &value));
        value = SETBIT(value, 0, 1);
        value = SETBIT(value, 1, 1);
        value = SETBIT(value, 9, 1);
        gcOnError(gckvip_write_register(hardware, 0x00000, value));

        gckvip_os_udelay(5);
        value = SETBIT(value, 9, 0);
        gcOnError(gckvip_write_register(hardware, 0x00000, value));
    }

#if vpmdENABLE_CAPTURE_IN_KERNEL
    PRINTK("#[disable clock -]");
#endif
onError:
    gckvip_unlock_recursive_mutex(&pm->mutex);
    return status;
}

vip_status_e gckvip_pm_enable_clock(
    gckvip_device_t *device
    )
{
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t value = 0;
    vip_uint32_t sh_value = 0;
    vip_uint32_t i = 0;
    vip_uint8_t fscale_percent = 0;
    vip_uint32_t fscale = 0;
#if vpmdONE_POWER_DOMAIN
    gckvip_context_t* context = gckvip_get_context();
    gckvip_pm_t* pm = &context->sp_management;
#else
    gckvip_pm_t* pm = &device->dp_management;
#endif
    gckvip_lock_recursive_mutex(&pm->mutex);
    if (vip_false_e == gckvip_os_get_atomic(device->clock_suspended)) {
        gckvip_unlock_recursive_mutex(&pm->mutex);
        return VIP_SUCCESS;
    }
#if vpmdENABLE_CAPTURE_IN_KERNEL
    PRINTK("[#enable clock +]");
#endif

    fscale_percent = gckvip_os_get_atomic(device->core_fscale_percent);
    fscale = 64 * fscale_percent / 100;
    if (fscale < 1) {
        fscale = 1;
    }
    else if (fscale > 64) {
        fscale = 64;
    }
    PRINTK_D("hardware clock resume set frequency fscale=%d, percent=%d\n", fscale, fscale_percent);


    for (i = 0; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
        gcOnError(gckvip_read_register(hardware, 0x00000, &value));
        value = SETBIT(value, 0, 0);
        value = SETBIT(value, 1, 0);
        value = SETBIT(value, 2, fscale);
        value = SETBIT(value, 9, 1);
        gcOnError(gckvip_write_register_ext(hardware, 0x00000, value));

        gckvip_os_udelay(5);
        value = SETBIT(value, 9, 0);
        gcOnError(gckvip_write_register(hardware, 0x00000, value));
    }

    /* scale the sh clock */
    for (i = 0; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
        gckvip_read_register(hardware, 0x0010C, &sh_value);
        sh_value = SETBIT(sh_value, 16, 0);
        sh_value = SETBIT(sh_value, 17, 1);
        sh_value = SETBIT(sh_value, 1, fscale);
        sh_value = SETBIT(sh_value, 0, 1);
        gckvip_write_register(hardware, 0x0010C, sh_value);
    }
    for (i = 0; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
        gckvip_read_register(hardware, 0x0010C, &sh_value);
        sh_value = SETBIT(sh_value, 0, 0);
        gckvip_write_register(hardware, 0x0010C, sh_value);
    }

    gcOnError(gckvip_os_set_atomic(device->clock_suspended, vip_false_e));
    gckvip_os_set_atomic(device->last_fscale_percent, fscale_percent);
#if vpmdENABLE_CAPTURE_IN_KERNEL
    PRINTK("#[enable clock -]");
#endif
onError:
    gckvip_unlock_recursive_mutex(&pm->mutex);
    return status;
}

/*
check hardware clock is on/off and enable it.
*/
vip_status_e gckvip_pm_enable_hardware_clock(
    gckvip_hardware_t *hardware
    )
{
    gckvip_context_t *context = gckvip_get_context();
    vip_status_e status = VIP_SUCCESS;
    gckvip_device_t *device = VIP_NULL;
    vip_uint32_t i = 0, j = 0;

    for (i = 0; i < context->device_count; i++) {
       device = &context->device[i];
       for (j = 0; j < device->hardware_count; j++) {
           gckvip_hardware_t *hardware_t = &device->hardware[j];
           if (hardware_t->core_id == hardware->core_id) {
               break;
           }
       }
       if (j < device->hardware_count) {
           break;
       }
    }
    if (i >= context->device_count) {
        PRINTK_E("fail to enable hw clk find core %d in context\n", hardware->core_id);
        gcOnError(VIP_ERROR_FAILURE);
    }

    gckvip_os_inc_atomic(device->disable_clock_suspend);
    if (VIP_SUCCESS != gckvip_pm_enable_clock(device)) {
        gckvip_os_dec_atomic(device->disable_clock_suspend);
    }

onError:
    return status;
}

vip_status_e gckvip_pm_disable_hardware_clock(
    gckvip_hardware_t *hardware
    )
{
    gckvip_context_t *context = gckvip_get_context();
    vip_status_e status = VIP_SUCCESS;
    gckvip_device_t *device = VIP_NULL;
    vip_uint32_t i = 0, j = 0;

    for (i = 0; i < context->device_count; i++) {
       device = &context->device[i];
       for (j = 0; j < device->hardware_count; j++) {
           gckvip_hardware_t *hardware_t = &device->hardware[j];
           if (hardware_t->core_id == hardware->core_id) {
               break;
           }
       }
       if (j < device->hardware_count) {
           break;
       }
    }
    if (i >= context->device_count) {
        PRINTK_E("fail to enable hw clk find core %d in context\n", hardware->core_id);
        gcOnError(VIP_ERROR_FAILURE);
    }

    gckvip_os_dec_atomic(device->disable_clock_suspend);
    gckvip_pm_disable_clock(device);

onError:
    return status;
}
#endif

#if vpmdENABLE_USER_CONTROL_POWER && vpmdENABLE_MULTIPLE_TASK
static vip_status_e gckvip_pm_clean_task(
    gckvip_device_t *device
    )
{
    vip_uint32_t i = 0;
    gckvip_task_slot_t* m_task = VIP_NULL;

    for (i = gckvip_hashmap_free_pos(&device->mt_hashmap) - 1; i > 0; i--) {
        gckvip_hashmap_get_by_index(&device->mt_hashmap, i, (void**)&m_task);
        gckvip_os_lock_mutex(m_task->cancel_mutex);
        if (GCKVIP_TASK_WITH_DATA == m_task->status) {
            m_task->status = GCKVIP_TASK_EMPTY;
            m_task->queue_data->v2 = VIP_ERROR_CANCELED;
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_hashmap_remove(&device->mt_hashmap, m_task->submit_handle.cmd_handle);
            gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_true_e);
        }
        else {
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
        }
    }

    return VIP_SUCCESS;
}
#endif

#if vpmdENABLE_POWER_MANAGEMENT
static vip_status_e power_off_action(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_hardware_t *hardware = VIP_NULL;
#if !vpmdONE_POWER_DOMAIN
    vip_uint32_t i = 0;
#endif

#if vpmdONE_POWER_DOMAIN
    hardware = &context->device[0].hardware[0];
#else
    for (i = 0; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
#endif
        /* power off hardware */
        status |= gckvip_drv_set_power_clk(hardware->core_id, GCKVIP_POWER_OFF);
#if !vpmdONE_POWER_DOMAIN
    }
#endif

#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
    device->hw_submit_count = 0;
    device->user_submit_count = 0;
    gckvip_os_set_atomic(device->device_idle, vip_true_e);
#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_os_set_atomic(device->clock_suspended, vip_true_e);
#endif
    GCKVIP_LOOP_DEVICE_END
#else
    device->hw_submit_count = 0;
    device->user_submit_count = 0;
    gckvip_os_set_atomic(device->device_idle, vip_true_e);
#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_os_set_atomic(device->clock_suspended, vip_true_e);
#endif
#endif

    return status;
}
#endif

static vip_status_e power_on_action(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_hardware_t *hardware = VIP_NULL;
#if vpmdENABLE_SUSPEND_RESUME
    gckvip_pm_t *pm = VIP_NULL;
#if vpmdONE_POWER_DOMAIN
    pm = &context->sp_management;
#else
    vip_uint32_t i = 0;
    pm = &device->dp_management;
#endif
#endif

    /* pre action */
#if vpmdENABLE_MULTIPLE_VIP && vpmdONE_POWER_DOMAIN && vpmdENABLE_POWER_MANAGEMENT
    GCKVIP_LOOP_DEVICE_START
    gckvip_os_set_signal(device->resume_done, vip_false_e);
    GCKVIP_LOOP_DEVICE_END
#endif

    /* power on hardware */
    PRINTK_D("power management start power on and clock up..\n");
#if vpmdONE_POWER_DOMAIN
    hardware = &context->device[0].hardware[0];
#else
    for (i = 0; i < device->hardware_count; i++) {
    hardware = &device->hardware[i];
#endif
    status |= gckvip_drv_set_power_clk(hardware->core_id, GCKVIP_POWER_ON);
#if !vpmdONE_POWER_DOMAIN
    }
#endif
    if (status != VIP_SUCCESS) {
        PRINTK_E("vipcore, failed to power on hardware\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    /* re-init hardware */
    PRINTK_D("start to re-initialize hardware..\n");
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
#endif
#if vpmdENABLE_CLOCK_SUSPEND
    gcOnError(gckvip_pm_enable_clock(device));
#endif
    status = gckvip_hw_reset(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to software reset hardware\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    status = gckvip_hw_init(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize hardware\n");
        gcGoOnError(VIP_ERROR_IO);
    }
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_END
#endif
#if vpmdENABLE_SUSPEND_RESUME
    if (vip_true_e == pm->wait_resume_signal) {
        pm->wait_resume_signal = vip_false_e;
        PRINTK_I("wake up resume signal..\n");
        gckvip_os_set_signal(pm->resume_signal, vip_true_e);
    }
#endif
    gckvip_os_set_atomic(device->core_fscale_percent, 100);

    /* post action */
#if vpmdENABLE_CLOCK_SUSPEND
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
    gckvip_os_set_atomic(device->disable_clock_suspend, 0);
    gckvip_os_set_atomic(device->clock_suspended, vip_false_e);
    GCKVIP_LOOP_DEVICE_END
#else
    gckvip_os_set_atomic(device->disable_clock_suspend, 0);
    gckvip_os_set_atomic(device->clock_suspended, vip_false_e);
#endif
#endif

#if vpmdENABLE_MULTIPLE_VIP && vpmdONE_POWER_DOMAIN && vpmdENABLE_POWER_MANAGEMENT
    GCKVIP_LOOP_DEVICE_START
    gckvip_os_set_signal(device->resume_done, vip_true_e);
    GCKVIP_LOOP_DEVICE_END
#endif

    PRINTK_D("power management on action end..\n");

    return status;

onError:
    /* error handle */
#if vpmdONE_POWER_DOMAIN
    hardware = &context->device[0].hardware[0];
#else
    for (i = 0; i < device->hardware_count; i++) {
    hardware = &device->hardware[i];
#endif
    gckvip_drv_set_power_clk(hardware->core_id, GCKVIP_POWER_OFF);
#if !vpmdONE_POWER_DOMAIN
    }
#endif

#if vpmdENABLE_CLOCK_SUSPEND
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
    gckvip_os_set_atomic(device->disable_clock_suspend, 0xDEADBEEF);
    GCKVIP_LOOP_DEVICE_END
#else
    gckvip_os_set_atomic(device->disable_clock_suspend, 0xDEADBEEF);
#endif
#endif

#if vpmdENABLE_MULTIPLE_VIP && vpmdONE_POWER_DOMAIN && vpmdENABLE_POWER_MANAGEMENT
    GCKVIP_LOOP_DEVICE_START
    gckvip_os_set_signal(device->resume_done, vip_false_e);
    GCKVIP_LOOP_DEVICE_END
#endif

    return status;
}

#if vpmdENABLE_SUSPEND_RESUME
static vip_status_e power_force_idle_action(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_hardware_t *hardware = VIP_NULL;
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
#endif
#if vpmdENABLE_CLOCK_SUSPEND
    if (vip_false_e == gckvip_os_get_atomic(device->clock_suspended)) {
        gckvip_os_inc_atomic(device->disable_clock_suspend);
        gckvip_pm_enable_clock(device);
#endif
        if (vip_false_e == gckvip_os_get_atomic(device->device_idle)) {
            vip_uint32_t i = 0;
            for (i = 0; i < device->hardware_count; i++) {
                hardware = &device->hardware[i];
                /* have a task is running on VIP, waiting for the task is finished */
                PRINTK_I("suspend, waiting for task finish...\n");
                gckvip_hw_wait_idle(context, hardware, 500, vip_false_e);
            }
        }
#if vpmdENABLE_CLOCK_SUSPEND
        gckvip_os_dec_atomic(device->disable_clock_suspend);
#if !vpmdENABLE_WAIT_LINK_LOOP
        /* turn off hardware internel clock */
        gckvip_pm_disable_clock(device);
#endif
    }
#endif
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_END
#endif

    return status;
}
#endif

static vip_status_e power_off_status_in(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_USER_CONTROL_POWER && vpmdENABLE_MULTIPLE_TASK
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
#endif
    status |= gckvip_queue_clean(&device->mt_input_queue, VIP_NULL);
    status |= gckvip_pm_clean_task(device);
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_END
#endif
#endif
#if vpmdENABLE_POWER_MANAGEMENT
    status |= power_off_action(context, device);
#endif

    return status;
}

static vip_status_e power_off_status_out(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    status = power_on_action(context, device);
    gckvip_os_set_atomic(context->init_done, vip_false_e);
#if vpmdENABLE_USER_CONTROL_POWER
#if vpmdONE_POWER_DOMAIN
    gckvip_os_set_atomic(context->sp_management.user_power_off, vip_false_e);
#else
    gckvip_os_set_atomic(device->dp_management.user_power_off, vip_false_e);
#endif
#endif
    return status;
}

static vip_status_e power_idle_status_in(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdPOWER_OFF_TIMEOUT
#if vpmdONE_POWER_DOMAIN
    gckvip_pm_t *pm = &context->sp_management;
#else
    gckvip_pm_t *pm = &device->dp_management;
#endif
    /* enable timer */
    if (vip_true_e == gckvip_os_get_atomic(pm->enable_timer)) {
        gckvip_os_stop_timer(pm->timer);
        gckvip_os_start_timer(pm->timer, vpmdPOWER_OFF_TIMEOUT);
    }
    gckvip_os_set_atomic(pm->user_query, vip_false_e);
#endif

#if !vpmdENABLE_WAIT_LINK_LOOP
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
#endif
#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_pm_disable_clock(device);
#else
    gckvip_pm_set_frequency(device, 1);
#endif
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_END
#endif
#endif

    return status;
}

static vip_status_e power_idle_status_out(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdPOWER_OFF_TIMEOUT
#if vpmdONE_POWER_DOMAIN
    gckvip_os_stop_timer(context->sp_management.timer);
#else
    gckvip_os_stop_timer(device->dp_management.timer);
#endif
#endif

    return status;
}

static vip_status_e power_on_status_in(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_os_inc_atomic(device->disable_clock_suspend);
    status = gckvip_pm_enable_clock(device);
#else
    status = gckvip_pm_set_frequency(device, gckvip_os_get_atomic(device->core_fscale_percent));
#endif
    gckvip_os_set_atomic(context->init_done, vip_true_e);

    return status;
}

static vip_status_e power_on_status_out(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_os_dec_atomic(device->disable_clock_suspend);
#endif

    return status;
}

static vip_status_e power_suspend_status_in(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_POWER_MANAGEMENT
    status = power_off_action(context, device);
#endif

    return status;
}

static vip_status_e power_suspend_status_out(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    status = power_on_action(context, device);
    gckvip_os_set_atomic(context->init_done, vip_false_e);

    return status;
}

static vip_status_e power_stop_status_in(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_os_inc_atomic(device->disable_clock_suspend);
#endif

#if vpmdENABLE_USER_CONTROL_POWER && vpmdENABLE_MULTIPLE_TASK
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
#endif
    status |= gckvip_queue_clean(&device->mt_input_queue, VIP_NULL);
    status |= gckvip_pm_clean_task(device);
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_END
#endif
#endif

    return status;
}

static vip_status_e power_stop_status_out(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_os_dec_atomic(device->disable_clock_suspend);
#endif

    return status;
}

static vip_status_e pm_status_trans(
    gckvip_context_t *context,
    gckvip_device_t *device,
    gckvip_power_status_e status_from,
    gckvip_power_status_e status_to
    )
{
    vip_status_e status = VIP_SUCCESS;
    /* out status */
    switch (status_from) {
    case GCKVIP_POWER_OFF:
    {
        status = power_off_status_out(context, device);
    }
    break;

    case GCKVIP_POWER_IDLE:
    {
        status = power_idle_status_out(context, device);
    }
    break;

    case GCKVIP_POWER_SUSPEND:
    {
        status = power_suspend_status_out(context, device);
    }
    break;

    case GCKVIP_POWER_ON:
    {
        status = power_on_status_out(context, device);
    }
    break;

    case GCKVIP_POWER_STOP:
    {
        status = power_stop_status_out(context, device);
    }
    break;

    default:
        break;
    }

    if (VIP_SUCCESS != status) {
        return status;
    }

    /* into status */
    switch (status_to) {
    case GCKVIP_POWER_OFF:
    {
        status = power_off_status_in(context, device);
    }
    break;

    case GCKVIP_POWER_IDLE:
    {
        status = power_idle_status_in(context, device);
    }
    break;

    case GCKVIP_POWER_SUSPEND:
    {
        status = power_suspend_status_in(context, device);
    }
    break;

    case GCKVIP_POWER_ON:
    {
        status = power_on_status_in(context, device);
    }
    break;

    case GCKVIP_POWER_STOP:
    {
        status = power_stop_status_in(context, device);
    }
    break;

    default:
        break;
    }

    return status;
}

static vip_bool_e all_device_idle(
    gckvip_context_t* context,
    gckvip_device_t* device
    )
{
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
#endif
    if (vip_false_e == gckvip_os_get_atomic(device->device_idle)) {
        return vip_false_e;
    }
#if vpmdENABLE_MULTIPLE_TASK
    if (GCKVIP_QUEUE_EMPTY != gckvip_queue_status(&device->mt_input_queue)) {
        return vip_false_e;
    }
#endif
#if vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_END
#endif

    return vip_true_e;
}

vip_status_e gckvip_pm_set_status(
    gckvip_context_t      *context,
    gckvip_device_t       *device,
    gckvip_power_status_e target_status
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_bool_e valid = vip_false_e;
    gckvip_power_status_e cur_status = GCKVIP_POWER_NONE;
    gckvip_pm_t *pm = VIP_NULL;
    if (VIP_NULL == device) {
        PRINTK_E("fail device is NULL in pm set status\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    if (VIP_NULL == context) {
        PRINTK_E("fail context is NULL in pm set status\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
#if vpmdONE_POWER_DOMAIN
    pm = &context->sp_management;
#else
    pm = &device->dp_management;
#endif
    gcOnError(gckvip_lock_recursive_mutex(&pm->mutex));
    cur_status = (gckvip_power_status_e)gckvip_os_get_atomic(pm->status);
    switch (cur_status) {
    case GCKVIP_POWER_OFF:
    {
        if (GCKVIP_POWER_IDLE == target_status) {
            valid = vip_true_e;
        }
    }
    break;

    case GCKVIP_POWER_IDLE:
    {
        if (GCKVIP_POWER_SUSPEND == target_status ||
            GCKVIP_POWER_ON == target_status ||
            GCKVIP_POWER_STOP == target_status ||
            GCKVIP_POWER_IDLE == target_status) {
            valid = vip_true_e;
        }
        else if (GCKVIP_POWER_OFF == target_status) {
            #if vpmdPOWER_OFF_TIMEOUT
            if (vip_false_e == gckvip_os_get_atomic(pm->user_query)) {
                valid = vip_true_e;
            }
            #else
            valid = vip_true_e;
            #endif
        }
    }
    break;

    case GCKVIP_POWER_SUSPEND:
    {
        if (GCKVIP_POWER_IDLE == target_status) {
            valid = vip_true_e;
        }
    }
    break;

    case GCKVIP_POWER_ON:
    {
        if (GCKVIP_POWER_IDLE == target_status) {
            #if vpmdENABLE_SUSPEND_RESUME
            if (vip_true_e == gckvip_os_get_atomic(pm->force_idle)) {
                power_force_idle_action(context, device);
                gckvip_os_set_atomic(pm->force_idle, vip_false_e);
                valid = vip_true_e;
            }
            else
            #endif
            {
                if (vip_true_e == all_device_idle(context, device)) {
                    valid = vip_true_e;
                }
            }
        }
        else if (GCKVIP_POWER_STOP == target_status ||
                 GCKVIP_POWER_ON == target_status) {
            valid = vip_true_e;
        }
    }
    break;

    case GCKVIP_POWER_STOP:
    {
        if (GCKVIP_POWER_IDLE == target_status) {
            #if vpmdENABLE_USER_CONTROL_POWER
            if (vip_true_e == gckvip_os_get_atomic(pm->user_power_off) ||
                vip_false_e == gckvip_os_get_atomic(pm->user_power_stop)) {
                if (vip_false_e == all_device_idle(context, device)) {
                    target_status = GCKVIP_POWER_ON;
                }
                valid = vip_true_e;
            }
            #endif
        }
    }
    break;

    default:
        break;
    }

    if (vip_false_e == valid) {
        if (GCKVIP_POWER_NONE != cur_status) {
            PRINTK_D("can not convert from %s to %s\n",
                     cur_status < GCKVIP_POWER_MAX ? pm_status_str[cur_status] : "UNKNOWN",
                     target_status < GCKVIP_POWER_MAX ? pm_status_str[target_status] : "UNKNOWN");
        }
    }
    else {
        gckvip_os_set_atomic(pm->status, GCKVIP_POWER_NONE);
        status = pm_status_trans(context, device, cur_status, target_status);
        if (VIP_SUCCESS != status) {
            PRINTK_E("power status trans from %d to %d fail\n", cur_status, target_status);
            gckvip_os_set_atomic(pm->status, cur_status);
        }
        else {
            gckvip_os_set_atomic(pm->status, target_status);
            PRINTK_D("power status trans from %s to %s\n", pm_status_str[cur_status], pm_status_str[target_status]);
        }
    }
    gckvip_unlock_recursive_mutex(&pm->mutex);

onError:
    return status;
}

/*
@brief a timer handle for power off VIP.
@param context, the kernel context.
*/
#if vpmdPOWER_OFF_TIMEOUT
static vip_int32_t power_manage_thread(
    vip_ptr param
    )
{
    vip_int32_t ret = 0;
    vip_status_e status = VIP_SUCCESS;
#if vpmdONE_POWER_DOMAIN
    gckvip_context_t *context = (gckvip_context_t *)param;
    gckvip_device_t *device = &context->device[0];
    gckvip_pm_t *pm = &context->sp_management;
#else
    vip_uint32_t dev = GCKVIPPTR_TO_UINT32(param) - 1;
    gckvip_context_t *context = gckvip_get_context();
    gckvip_device_t *device = &context->device[dev];
    gckvip_pm_t *pm = &device->dp_management;
#endif

    while (1) {
        if (vip_false_e == gckvip_os_get_atomic(pm->thread_running)) {
            PRINTK_D("power manage thread not running, exit thread before wait signal\n");
            break;
        }

        if (VIP_SUCCESS == gckvip_os_wait_signal((vip_ptr*)pm->signal, vpmdINFINITE)) {
            PRINTK_D("power management thread wake up.\n");
            /* wait signal next time */
            gckvip_os_set_signal(pm->signal, vip_false_e);

            if (vip_false_e == gckvip_os_get_atomic(pm->thread_running)) {
                PRINTK_D("power manage thread not running, exit thread after wait signal\n");
                break;
            }

            status = gckvip_pm_set_status(context, device, GCKVIP_POWER_OFF);
            if (VIP_SUCCESS != status) {
                PRINTK_E("fail to power off hardware in power management thread, status=%d\n", status);
            }
        }
        else {
            PRINTK_E("power management thread wait time out or signal destroyed\n");
            break;
        }
    }
    PRINTK_D("power management thread exit\n");
    gckvip_os_set_signal(pm->exit_signal, vip_true_e);

    if (status != VIP_SUCCESS) {
        ret = -1;
    }
    return ret;
}

static void power_off_timer_handle(
    vip_ptr param
    )
{
    gckvip_queue_status_e queue_status = GCKVIP_QUEUE_EMPTY;
#if vpmdONE_POWER_DOMAIN
    gckvip_context_t *context = (gckvip_context_t*)param;
    gckvip_pm_t *pm = &context->sp_management;
#if vpmdENABLE_MULTIPLE_TASK
    GCKVIP_LOOP_DEVICE_START
    queue_status = gckvip_queue_status(&device->mt_input_queue);
    if (GCKVIP_QUEUE_EMPTY != queue_status) break;
    GCKVIP_LOOP_DEVICE_END
#endif
#else
    vip_uint32_t dev = GCKVIPPTR_TO_UINT32(param) - 1;
    gckvip_context_t *context = gckvip_get_context();
    gckvip_device_t *device = &context->device[dev];
    gckvip_pm_t *pm = &device->dp_management;
#if vpmdENABLE_MULTIPLE_TASK
    queue_status = gckvip_queue_status(&device->mt_input_queue);
#endif
#endif

    PRINTK_D("power off timer start. timeout=%dms, muti-task queue=%s\n",
              vpmdPOWER_OFF_TIMEOUT, (GCKVIP_QUEUE_EMPTY == queue_status) ? "empty" : "not-empty");

    /* power off the hardware */
    if (GCKVIP_QUEUE_EMPTY == queue_status) {
        /* trigger power manage thread */
        gckvip_os_set_signal(pm->signal, vip_true_e);
    }
    else {
        PRINTK_D("power off timer handler, queue not empty, by pass power off\n");
    }
}
#endif

#if vpmdENABLE_POWER_MANAGEMENT
vip_status_e gckvip_pm_query_info(
    gckvip_context_t *context,
    gckvip_query_power_info_t *data
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_query_power_info_t *info = data;
    gckvip_device_t *device = VIP_NULL;
    gckvip_pm_t *pm = VIP_NULL;

    if (VIP_NULL == info) {
        PRINTK_E("fail to query pm info, param is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    GCKVIP_CHECK_DEV(info->device_id);
    device = &context->device[info->device_id];

#if vpmdENABLE_MULTIPLE_VIP && vpmdONE_POWER_DOMAIN
    if (VIP_SUCCESS != gckvip_os_wait_signal(device->resume_done, WAIT_SIGNAL_TIMEOUT)) {
        PRINTK_D("fail to wait resume done signal.\n");
    }
#endif

#if vpmdONE_POWER_DOMAIN
    pm = &context->sp_management;
#else
    pm = &device->dp_management;
#endif

    gcOnError(gckvip_lock_recursive_mutex(&pm->mutex));
#if vpmdPOWER_OFF_TIMEOUT
    gckvip_os_stop_timer(pm->timer);
    gckvip_os_set_atomic((vip_ptr*)pm->user_query, vip_true_e);
#endif
    if (vip_true_e == gckvip_os_get_atomic(context->init_done)) {
        info->power_status = gckvip_os_get_atomic(pm->status);
    }
    else {
        info->power_status = GCKVIP_POWER_OFF;
        PRINTK_I("pm init done is false\n");
    }
    gckvip_unlock_recursive_mutex(&pm->mutex);
    PRINTK_D("power management query power info, power status=%s\n",
              pm_status_str[info->power_status]);

onError:
    return status;
}
#endif

vip_status_e gckvip_pm_hw_submit(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_pm_t *pm = VIP_NULL;

#if vpmdONE_POWER_DOMAIN
    pm = &context->sp_management;
#else
    pm = &device->dp_management;
#endif
    status = gckvip_pm_set_status(context, device, GCKVIP_POWER_ON);
    if (GCKVIP_POWER_ON != gckvip_os_get_atomic(pm->status) &&
        GCKVIP_POWER_NONE != gckvip_os_get_atomic(pm->status)) {
        PRINTK_E("invalid power status=%d\n", gckvip_os_get_atomic(pm->status));
        status = VIP_ERROR_POWER_OFF;
    }
    else {
        gckvip_os_set_atomic(device->device_idle, vip_false_e);
    }

    return status;
}

vip_status_e gckvip_pm_hw_idle(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
#if !vpmdENABLE_WAIT_LINK_LOOP
#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_pm_disable_clock(device);
#else
    gckvip_pm_set_frequency(device, 1);
#endif
#endif

    gckvip_os_set_atomic(device->device_idle, vip_true_e);
    status = gckvip_pm_set_status(context, device, GCKVIP_POWER_IDLE);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to pm set status to idle, status=%d\n", status);
    }

#if vpmdENABLE_USER_CONTROL_POWER
#if vpmdONE_POWER_DOMAIN
    if (vip_true_e == gckvip_os_get_atomic(context->sp_management.user_power_off)) {
#else
    if (vip_true_e == gckvip_os_get_atomic(device->dp_management.user_power_off)) {
#endif
        status = gckvip_pm_set_status(context, device, GCKVIP_POWER_OFF);
    }
#endif

    return status;
}

/*
@bried initialize driver power management module.
*/
vip_status_e gckvip_pm_init(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdONE_POWER_DOMAIN
    gckvip_pm_t *pm = &context->sp_management;
#if vpmdPOWER_OFF_TIMEOUT
    vip_char_t name[256];
    vip_ptr param = (vip_ptr)context;
    gckvip_os_snprint(name, 255, "power_manage_%d", 0);
#endif
#else
    GCKVIP_LOOP_DEVICE_START
    gckvip_pm_t *pm = &device->dp_management;
#if vpmdPOWER_OFF_TIMEOUT
    vip_char_t name[256];
    vip_ptr param = (vip_ptr)GCKVIPUINT64_TO_PTR(dev_index + 1);
    gckvip_os_snprint(name, 255, "power_manage_%d", dev_index);
#endif
#endif
    status = gckvip_os_create_mutex(&pm->mutex.mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create mutex for power memory\n");
        gcGoOnError(VIP_ERROR_IO);
    }
#if vpmdPOWER_OFF_TIMEOUT
    status = gckvip_os_create_signal(&pm->signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create a signal for power\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    status = gckvip_os_create_signal(&pm->exit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create a signal for thread exit\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_signal(pm->exit_signal, vip_false_e);
    status = gckvip_os_create_atomic(&pm->thread_running);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for power thread running\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_atomic(pm->thread_running, vip_true_e);
    status = gckvip_os_create_thread((gckvip_thread_func)power_manage_thread,
                                    param, name, &pm->thread);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create a thread for supporting power manage.\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    status = gckvip_os_create_atomic(&pm->user_query);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for user query power\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    status = gckvip_os_create_atomic(&pm->enable_timer);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for enable power timer\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_atomic(pm->enable_timer, vip_true_e);

    status = gckvip_os_create_timer((gckvip_timer_func)power_off_timer_handle,
                                    param, &pm->timer);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create timer for power off\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_stop_timer(pm->timer);
#endif

    /* create signals for system suspend/resume feature */
#if vpmdENABLE_SUSPEND_RESUME
    status = gckvip_os_create_signal(&pm->resume_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create resume signal\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    pm->wait_resume_signal = vip_false_e;
#endif

#if vpmdENABLE_SUSPEND_RESUME
    status = gckvip_os_create_atomic(&pm->force_idle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for force idle flag\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_atomic(pm->force_idle, vip_false_e);
#endif

#if vpmdENABLE_USER_CONTROL_POWER
    status = gckvip_os_create_atomic(&pm->user_power_off);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for user power off flag\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_atomic(pm->user_power_off, vip_false_e);
    status = gckvip_os_create_atomic(&pm->user_power_stop);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for user power stop flag\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_atomic(pm->user_power_stop, vip_false_e);
#endif

    /* hardware has been power on in gckvip_os_init */
#if vpmdONE_POWER_DOMAIN
#if vpmdENABLE_MULTIPLE_VIP && vpmdENABLE_POWER_MANAGEMENT
    GCKVIP_LOOP_DEVICE_START
    gckvip_os_create_signal(&device->resume_done);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create resume done signal\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_signal(device->resume_done, vip_true_e);
    GCKVIP_LOOP_DEVICE_END
#endif
    PRINTK_D("power management is enabled ...\n");
    gckvip_os_set_atomic(pm->status, GCKVIP_POWER_NONE);
#else
    PRINTK_D("power management is enabled device%d...\n", device->device_id);
    gckvip_os_set_atomic(pm->status, GCKVIP_POWER_NONE);
    GCKVIP_LOOP_DEVICE_END
#endif

    return status;

onError:
#if !vpmdONE_POWER_DOMAIN
    GCKVIP_LOOP_DEVICE_START
    gckvip_pm_t *pm = &device->dp_management;
#endif

    if (pm->mutex.mutex != VIP_NULL) {
        gckvip_os_destroy_mutex(pm->mutex.mutex);
        pm->mutex.mutex = VIP_NULL;
    }
#if vpmdPOWER_OFF_TIMEOUT
    if (pm->signal != VIP_NULL) {
        gckvip_os_destroy_signal(pm->signal);
        pm->signal = VIP_NULL;
    }
    if (pm->thread != VIP_NULL) {
        gckvip_os_destroy_thread(pm->thread);
        pm->thread = VIP_NULL;
    }
    if (pm->timer != VIP_NULL) {
        gckvip_os_stop_timer(pm->timer);
        gckvip_os_destroy_timer(pm->timer);
        pm->timer = VIP_NULL;
    }
#endif
#if vpmdENABLE_SUSPEND_RESUME
    if (pm->resume_signal != VIP_NULL) {
        gckvip_os_destroy_signal(pm->resume_signal);
    }
#endif

#if vpmdONE_POWER_DOMAIN
#if vpmdENABLE_MULTIPLE_VIP && vpmdENABLE_POWER_MANAGEMENT
    GCKVIP_LOOP_DEVICE_START
    if (device->resume_done != VIP_NULL) {
        gckvip_os_destroy_signal(device->resume_done);
    }
    GCKVIP_LOOP_DEVICE_END
#endif
    gckvip_os_set_atomic(pm->status, GCKVIP_POWER_NONE);
#else
    gckvip_os_set_atomic(pm->status, GCKVIP_POWER_NONE);
    GCKVIP_LOOP_DEVICE_END
#endif

    return status;
}

/*
@bried un-initialize driver power management module.
*/
vip_status_e gckvip_pm_uninit(
    gckvip_context_t *context
    )
{
#if vpmdONE_POWER_DOMAIN
    gckvip_pm_t *pm = &context->sp_management;
#else
    GCKVIP_LOOP_DEVICE_START
    gckvip_pm_t *pm = &device->dp_management;
#endif
#if vpmdPOWER_OFF_TIMEOUT
    gckvip_os_wait_signal((vip_ptr*)pm->exit_signal, vpmdINFINITE); /* waiting pm thread exit */
    if (pm->enable_timer != VIP_NULL) {
        gckvip_os_destroy_atomic(pm->enable_timer);
        pm->enable_timer = VIP_NULL;
    }
    if (pm->timer != VIP_NULL) {
        gckvip_os_stop_timer(pm->timer);
        gckvip_os_destroy_timer(pm->timer);
        pm->timer = VIP_NULL;
    }
    if (pm->user_query != VIP_NULL) {
        gckvip_os_destroy_atomic(pm->user_query);
        pm->user_query = VIP_NULL;
    }
    if (pm->signal != VIP_NULL) {
        gckvip_os_destroy_signal(pm->signal);
        pm->signal = VIP_NULL;
    }
    if (pm->thread != VIP_NULL) {
        gckvip_os_destroy_thread(pm->thread);
        pm->thread = VIP_NULL;
    }
    if (pm->thread_running != VIP_NULL) {
        gckvip_os_destroy_atomic(pm->thread_running);
        pm->thread_running = VIP_NULL;
    }
    if (pm->exit_signal != VIP_NULL) {
        gckvip_os_destroy_signal(pm->exit_signal);
        pm->exit_signal = VIP_NULL;
    }
#endif
    if (pm->mutex.mutex != VIP_NULL) {
        gckvip_os_destroy_mutex(pm->mutex.mutex);
        pm->mutex.mutex = VIP_NULL;
    }
#if vpmdENABLE_SUSPEND_RESUME
    if (pm->resume_signal != VIP_NULL) {
        gckvip_os_destroy_signal(pm->resume_signal);
        pm->resume_signal = VIP_NULL;
    }
    pm->wait_resume_signal = vip_false_e;
#endif

#if vpmdENABLE_SUSPEND_RESUME
    if (pm->force_idle != VIP_NULL) {
        gckvip_os_destroy_atomic(pm->force_idle);
        pm->force_idle = VIP_NULL;
    }
#endif
#if vpmdENABLE_USER_CONTROL_POWER
    if (pm->user_power_off != VIP_NULL) {
        gckvip_os_destroy_atomic(pm->user_power_off);
        pm->user_power_off = VIP_NULL;
    }
    if (pm->user_power_stop != VIP_NULL) {
        gckvip_os_destroy_atomic(pm->user_power_stop);
        pm->user_power_stop = VIP_NULL;
    }
#endif

    /* power off in gckvip_os_close */
#if vpmdONE_POWER_DOMAIN
#if vpmdENABLE_MULTIPLE_VIP && vpmdENABLE_POWER_MANAGEMENT
    GCKVIP_LOOP_DEVICE_START
    if (device->resume_done != VIP_NULL) {
        gckvip_os_destroy_signal(device->resume_done);
    }
    GCKVIP_LOOP_DEVICE_END
#endif
    gckvip_os_set_atomic(pm->status, GCKVIP_POWER_NONE);
#else
    gckvip_os_set_atomic(pm->status, GCKVIP_POWER_NONE);
    GCKVIP_LOOP_DEVICE_END
#endif

    PRINTK_D("power management uninit done\n");
    return VIP_SUCCESS;
}

#if vpmdENABLE_SUSPEND_RESUME
/*
@brief  uninitialize hardware for system suspend
*/
vip_status_e gckvip_pm_suspend(void)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_context_t *context = gckvip_get_context();

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_lock_mutex(context->initialize_mutex);
    }
#endif
    if (0 == context->initialize) {
        goto onError;
    }

    {
    #if vpmdONE_POWER_DOMAIN
        gckvip_device_t *device = &context->device[0];
        gckvip_pm_t *pm = &context->sp_management;
    #else
        GCKVIP_LOOP_DEVICE_START
        gckvip_pm_t *pm = &device->dp_management;
    #endif
        gcOnError(gckvip_lock_recursive_mutex(&pm->mutex));
        gckvip_os_set_atomic(pm->force_idle, vip_true_e);
        status = gckvip_pm_set_status(context, device, GCKVIP_POWER_IDLE);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to set pm status=%d\n", GCKVIP_POWER_IDLE);
            gckvip_unlock_recursive_mutex(&pm->mutex);
            goto onError;
        }
        status = gckvip_pm_set_status(context, device, GCKVIP_POWER_SUSPEND);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to set pm status=%d\n", GCKVIP_POWER_SUSPEND);
            gckvip_unlock_recursive_mutex(&pm->mutex);
            goto onError;
        }
        gckvip_unlock_recursive_mutex(&pm->mutex);
    #if !vpmdONE_POWER_DOMAIN
        GCKVIP_LOOP_DEVICE_END
    #endif
    }

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_unlock_mutex(context->initialize_mutex);
    }
#endif
    return status;
}
#endif

/*
@brief  initialize hardware for system resume and power management
*/
vip_status_e gckvip_pm_resume(void)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_context_t *context = gckvip_get_context();

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_lock_mutex(context->initialize_mutex);
    }
#endif
    if (0 == context->initialize) {
    #if vpmdENABLE_MULTIPLE_TASK
        if (context->initialize_mutex != VIP_NULL) {
            gckvip_os_unlock_mutex(context->initialize_mutex);
        }
    #endif
        return VIP_SUCCESS;
    }

    GCKVIP_LOOP_DEVICE_START
    gcOnError(gckvip_pm_set_status(context, device, GCKVIP_POWER_IDLE))
    GCKVIP_LOOP_DEVICE_END

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_unlock_mutex(context->initialize_mutex);
    }
#endif

    return status;
}

#if vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
/*
@brief check power status before submit hardware command.
*/
vip_status_e gckvip_pm_check_power(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
#if (vpmdENABLE_DEBUG_LOG >= 3) || vpmdENABLE_SUSPEND_RESUME
#if vpmdONE_POWER_DOMAIN
    gckvip_pm_t *pm = &context->sp_management;
#else
    gckvip_pm_t *pm = &device->dp_management;
#endif
#endif
    PRINTK_I("check device%d power status=%s.\n", device->device_id,
             pm_status_str[gckvip_os_get_atomic(pm->status)]);
#if vpmdENABLE_SUSPEND_RESUME
    if (GCKVIP_POWER_SUSPEND == gckvip_os_get_atomic(pm->status)) {
        vip_uint32_t count = 0;
        vip_int32_t ref_count = 0;
        vip_int32_t i = 0;
        /* waiting for system resume again */
        /* system suspended, should waiting for resume signal,
           then a task can be submited to hardware,
           release the pm lock firstly,
           get the pm lock again after resume
        */
        pm->wait_resume_signal = vip_true_e;
        gcOnError(gckvip_os_set_signal(pm->resume_signal, vip_false_e));

        ref_count = pm->mutex.ref_count;
        for (i = 0; i < ref_count; i++) {
            gckvip_unlock_recursive_mutex(&pm->mutex);
        }

        while (1) {
            PRINTK_I("waiting for system resume, wait count=%d...\n", count);
            if (VIP_SUCCESS == gckvip_os_wait_signal(pm->resume_signal, WAIT_SIGNAL_TIMEOUT)) {
                break;
            }
            count++;
        }
        gcOnError(gckvip_os_set_signal(pm->resume_signal, vip_false_e));

        for (i = 0; i < ref_count; i++) {
            gckvip_lock_recursive_mutex(&pm->mutex);
        }
    }
#endif

    /* power management */
#if vpmdPOWER_OFF_TIMEOUT
    status = gckvip_pm_set_status(context, device, GCKVIP_POWER_IDLE);
    PRINTK_D("submit command check power on device%d\n", device->device_id);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to check power vip going to idle status=%d.\n", status);
        gcGoOnError(status);
    }
#endif

onError:
    return status;
}
#endif

#if vpmdENABLE_USER_CONTROL_POWER || vpmdPOWER_OFF_TIMEOUT
vip_status_e gckvip_pm_power_management(
    gckvip_power_management_t *power,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_USER_CONTROL_POWER || vpmdONE_POWER_DOMAIN
    gckvip_context_t *context = gckvip_get_context();
#endif
#if vpmdONE_POWER_DOMAIN
    gckvip_pm_t *pm = &context->sp_management;
#else
    gckvip_pm_t *pm = &device->dp_management;
#endif

    switch (power->property) {
#if vpmdENABLE_USER_CONTROL_POWER
    case VIP_POWER_PROPERTY_SET_FREQUENCY:
    {
        gcOnError(gckvip_lock_recursive_mutex(&pm->mutex));
        if (GCKVIP_POWER_OFF != gckvip_os_get_atomic(pm->status)) {
        #if vpmdENABLE_CLOCK_SUSPEND
            gckvip_os_inc_atomic(device->disable_clock_suspend);
            gckvip_pm_enable_clock(device);
        #endif
            status = gckvip_pm_set_frequency(device, power->fscale_percent);
        #if vpmdENABLE_CLOCK_SUSPEND
            gckvip_os_dec_atomic(device->disable_clock_suspend);
        #endif
            if (status != VIP_SUCCESS) {
                PRINTK_E("failed to set device frequency..\n");
                gckvip_unlock_recursive_mutex(&pm->mutex);
                gcGoOnError(VIP_ERROR_FAILURE);
            }
        }
        else {
            PRINTK_E("failed to set device frequency, device is power off status, power=%d\n",
                      gckvip_os_get_atomic(pm->status));
            gckvip_unlock_recursive_mutex(&pm->mutex);
            gcGoOnError(VIP_ERROR_POWER_OFF);
        }

        if (power->fscale_percent > 100) {
            gckvip_os_set_atomic(device->core_fscale_percent, 100);
        }
        else {
            gckvip_os_set_atomic(device->core_fscale_percent, power->fscale_percent);
        }
        gckvip_unlock_recursive_mutex(&pm->mutex);
    }
    break;

    case VIP_POWER_PROPERTY_OFF:
    {
        gcOnError(gckvip_lock_recursive_mutex(&pm->mutex));
        gckvip_os_set_atomic(pm->user_power_off, vip_true_e);
        status = gckvip_pm_set_status(context, device, GCKVIP_POWER_IDLE);
        if (status != VIP_SUCCESS) {
            gckvip_unlock_recursive_mutex(&pm->mutex);
            goto onError;
        }
        status = gckvip_pm_set_status(context, device, GCKVIP_POWER_OFF);
        gckvip_unlock_recursive_mutex(&pm->mutex);
    }
    break;

    case VIP_POWER_PROPERTY_ON:
    {
        gcOnError(gckvip_lock_recursive_mutex(&pm->mutex));
        gckvip_os_set_atomic(pm->user_power_off, vip_false_e);
        status = gckvip_pm_set_status(context, device, GCKVIP_POWER_IDLE);
        gckvip_unlock_recursive_mutex(&pm->mutex);
    }
    break;

    case VIP_POWER_PROPERTY_STOP:
    {
        gcOnError(gckvip_lock_recursive_mutex(&pm->mutex));
        gckvip_os_set_atomic(pm->user_power_stop, vip_true_e);
        status = gckvip_pm_set_status(context, device, GCKVIP_POWER_STOP);
        gckvip_unlock_recursive_mutex(&pm->mutex);
    }
    break;

    case VIP_POWER_PROPERTY_START:
    {
        gcOnError(gckvip_lock_recursive_mutex(&pm->mutex));
        gckvip_os_set_atomic(pm->user_power_stop, vip_false_e);
        status = gckvip_pm_set_status(context, device, GCKVIP_POWER_IDLE);
        gckvip_unlock_recursive_mutex(&pm->mutex);
    }
    break;
#endif

#if vpmdPOWER_OFF_TIMEOUT
    case VIP_POWER_ENABLE_TIMER:
    {
        gckvip_os_set_atomic(pm->enable_timer, vip_true_e);
    }
    break;

    case VIP_POWER_DISABLE_TIMER:
    {
        gckvip_os_stop_timer(pm->timer);
        gcOnError(gckvip_os_set_atomic(pm->enable_timer, vip_false_e));
    }
    break;
#endif

    default:
        PRINTK_E("application setting power management, not support property=%d\n",
                 power->property);
        break;
    }

onError:
    return status;
}
#endif

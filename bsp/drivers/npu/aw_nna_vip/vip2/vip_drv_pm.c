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
#include <vip_drv_pm.h>
#if vpmdENABLE_POWER_MANAGEMENT
#include <vip_drv_device_driver.h>
#include <vip_drv_context.h>
#if vpmdENABLE_TASK_PROFILE
#include <vip_drv_debug.h>
#endif

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_POWER


vip_status_e vipdrv_pm_set_frequency(
    vipdrv_hardware_t *hardware,
    vip_uint8_t fscale_percent
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t bk_data = {0};
    vip_uint32_t value = 0;
    vip_uint32_t sh_value = 0;
    vip_uint32_t fscale = 0;
    vip_uint32_t fscale_table[7] = {1,2,4,8,16,32,64};
    vip_uint32_t pow = 0;

    if (vip_false_e == vipdrv_context_support(VIP_HW_FEATURE_FREQ_SCALE)) {
        return VIP_SUCCESS;
    }
#if vpmdENABLE_CLOCK_SUSPEND
    if (vip_true_e == hardware->clock_suspended) {
        return VIP_SUCCESS;
    }
#endif

    if (fscale_percent > 100) {
        PRINTK_E("set hardware frequency, fscale percent is more than 100."
                 "force set to maximum vaue 100.\n");
        fscale_percent = 100;
        fscale = 64;
    }
    else if (fscale_percent < 0) {
        PRINTK_E("set hardware frequency, fscale percent is less than 0."
                 "force set to minimum vaue 1.\n");
        fscale = 1;
        fscale_percent = 1;
    }
    else {
        pow = fscale_percent/16;
        fscale = fscale_table[pow];
    }
    PRINTK_D("dev%d hw%d set frequency fscale=%d, percent=%d\n",
            hardware->device->device_index, hardware->core_index, fscale, fscale_percent);
    /* get power control */
    vipdrv_read_register(hardware, 0x00104, &bk_data);
    /* disable all clock gating */
    vipdrv_write_register(hardware, 0x00104, 0x00ffffff);

    /* scale the core clock */
    vipOnError(vipdrv_read_register(hardware, 0x00000, &value));
    value = SETBIT(value, 0, 0);
    /* value = SETBIT(value, 1, 0); for 2D clock*/
    value = SETBITS(value, 2, 8, fscale);
    value = SETBIT(value, 9, 1);
    vipOnError(vipdrv_write_register(hardware, 0x00000, value));
    value = SETBIT(value, 9, 0);
    vipOnError(vipdrv_write_register(hardware, 0x00000, value));
#if vpmdFPGA_BUILD
    vipdrv_os_udelay(5);
#else
    vipdrv_os_udelay(1);
#endif

    /* scale the ppu clock */
    vipdrv_read_register(hardware, 0x0010C, &sh_value);
    sh_value = SETBITS(sh_value, 1, 7, fscale);
    sh_value = SETBIT(sh_value, 0, 1);
    /* done loading the PPU frequency scaler. */
    vipdrv_write_register(hardware, 0x0010C, sh_value);
    sh_value = SETBIT(sh_value, 0, 0);
    vipdrv_write_register(hardware, 0x0010C, sh_value);

    /* restore all clock gating. */
    vipdrv_write_register(hardware, 0x00104, bk_data);
onError:
    return status;
}

#if vpmdENABLE_CLOCK_SUSPEND
static vip_status_e vipdrv_pm_disable_clock(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t value = 0;
    if (0 < hardware->disable_clock_suspend || vip_true_e == hardware->clock_suspended) {
        return VIP_SUCCESS;
    }

#if vpmdENABLE_AHB_LOG
    PRINTK("#[disable clock +]\n");
#endif
    PRINTK_D("dev%d hw%d clock suspend.\n", hardware->device->device_index, hardware->core_index);

    hardware->clock_suspended = vip_true_e;
    vipOnError(vipdrv_read_register(hardware, 0x00000, &value));
    value = SETBIT(value, 0, 1);
    /* value = SETBIT(value, 1, 1); for 2D clock*/
    value = SETBIT(value, 9, 1);
    vipOnError(vipdrv_write_register(hardware, 0x00000, value));
    value = SETBIT(value, 9, 0);
    vipOnError(vipdrv_write_register(hardware, 0x00000, value));
#if vpmdFPGA_BUILD
    vipdrv_os_udelay(5);
#else
    vipdrv_os_udelay(1);
#endif
#if vpmdENABLE_AHB_LOG
    PRINTK("#[disable clock -]\n");
#endif
onError:
    return status;
}

static vip_status_e vipdrv_pm_enable_clock(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t value = 0;
    vip_uint32_t sh_value = 0;
    if (vip_false_e == hardware->clock_suspended) {
        return VIP_SUCCESS;
    }
#if vpmdENABLE_AHB_LOG
    PRINTK("#[enable clock +]\n");
#endif

    vipOnError(vipdrv_read_register(hardware, 0x00000, &value));
    value = SETBIT(value, 0, 0);
    /* value = SETBIT(value, 1, 0); for 2D clock*/
    value = SETBIT(value, 9, 1);
    vipOnError(vipdrv_write_register_ext(hardware, 0x00000, value));
    value = SETBIT(value, 9, 0);
    vipOnError(vipdrv_write_register(hardware, 0x00000, value));
#if vpmdFPGA_BUILD
    vipdrv_os_udelay(5);
#else
    vipdrv_os_udelay(1);
#endif

    /* scale the sh clock */
    vipdrv_read_register(hardware, 0x0010C, &sh_value);
    sh_value = SETBIT(sh_value, 0, 1);
    vipdrv_write_register(hardware, 0x0010C, sh_value);
    sh_value = SETBIT(sh_value, 0, 0);
    vipdrv_write_register(hardware, 0x0010C, sh_value);

    hardware->clock_suspended = vip_false_e;
#if vpmdENABLE_AHB_LOG
    PRINTK("#[enable clock -]\n");
#endif
onError:
    return status;
}
#endif

#if vpmdENABLE_USER_CONTROL_POWER && vpmdTASK_QUEUE_USED
static vip_status_e vipdrv_pm_clean_task(
    vipdrv_hardware_t *hardware
    )
{
    vip_uint32_t i = 0;
    vipdrv_context_t *context = hardware->device->context;
    vipdrv_task_control_block_t* tskTCB = VIP_NULL;

    for (i = vipdrv_hashmap_free_pos(&context->tskTCB_hashmap) - 1; i > 0; i--) {
        vipdrv_hashmap_get_by_index(&context->tskTCB_hashmap, i, (void**)&tskTCB);
        if (VIP_NULL == tskTCB) {
            continue;
        }
        if (tskTCB->task.device == hardware->device &&
            hardware->core_index >= tskTCB->task.core_index &&
            hardware->core_index < tskTCB->task.core_index + tskTCB->task.core_cnt) {
            vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
            if (VIPDRV_TASK_READY == tskTCB->status) {
                tskTCB->status = VIPDRV_TASK_EMPTY;
                tskTCB->queue_data->v2 = VIP_ERROR_CANCELED;
                vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                vipdrv_hashmap_remove_by_index(&context->tskTCB_hashmap, i, vip_false_e);
                vipdrv_hashmap_unuse(&context->tskTCB_hashmap, i, vip_true_e);
            }
            else {
                vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                vipdrv_hashmap_unuse(&context->tskTCB_hashmap, i, vip_false_e);
            }
        }
        else {
            vipdrv_hashmap_unuse(&context->tskTCB_hashmap, i, vip_false_e);
        }
    }

    return VIP_SUCCESS;
}
#endif

static vip_status_e power_off_action(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdONE_POWER_DOMAIN
    vipdrv_context_t *context = hardware->device->context;
    vipdrv_os_lock_mutex(context->power_mutex);
    context->power_hardware_cnt--;
    if (0 == context->power_hardware_cnt) {
        status = vipdrv_drv_set_power_clk(0, VIPDRV_POWER_OFF);
    }
    vipdrv_os_unlock_mutex(context->power_mutex);
#else
    status = vipdrv_drv_set_power_clk(hardware->core_id, VIPDRV_POWER_OFF);
#endif

    hardware->hw_submit_count = 0;
    hardware->user_submit_count = 0;
    vipdrv_os_set_atomic(hardware->idle, vip_true_e);
#if vpmdENABLE_CLOCK_SUSPEND
    hardware->clock_suspended = vip_true_e;
#endif

    return status;
}

static vip_status_e power_on_action(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    /* power on hardware */
#if vpmdONE_POWER_DOMAIN
    vipdrv_context_t *context = hardware->device->context;
    vipdrv_os_lock_mutex(context->power_mutex);
    if (0 == context->power_hardware_cnt) {
        PRINTK_D("power management start power on and clock up..\n");
        status = vipdrv_drv_set_power_clk(0, VIPDRV_POWER_ON);
    }
    context->power_hardware_cnt++;
    vipdrv_os_unlock_mutex(context->power_mutex);
#else
    PRINTK_D("power management start power on and clock up..\n");
    status = vipdrv_drv_set_power_clk(hardware->core_id, VIPDRV_POWER_ON);
#endif

    if (status != VIP_SUCCESS) {
        PRINTK_E("vipcore, fail to power on hardware\n");
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    /* re-init hardware */
    PRINTK_D("start to re-initialize hardware..\n");
#if vpmdENABLE_CLOCK_SUSPEND
    vipOnError(vipdrv_pm_enable_clock(hardware));
#endif
    status = vipdrv_hw_reset(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to software reset hardware\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    status = vipdrv_hw_init(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize hardware\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    PRINTK_D("power management on action end..\n");

    return status;

onError:
    /* error handle */
#if vpmdONE_POWER_DOMAIN
    vipdrv_drv_set_power_clk(0, VIPDRV_POWER_OFF);
#else
    vipdrv_drv_set_power_clk(hardware->core_id, VIPDRV_POWER_OFF);
#endif
    hardware->disable_clock_suspend = 0xDEADBEEF;

    return status;
}

static vip_status_e power_life_state_in(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    hardware->pwr_management.state = VIPDRV_POWER_SYS_LIFE;

    return status;
}

static vip_status_e power_life_state_out(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}

static vip_status_e power_off_state_in(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    hardware->pwr_management.state = VIPDRV_POWER_OFF;

    return status;
}

static vip_status_e power_off_state_out(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}

static vip_status_e power_on_state_in(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    status = power_on_action(hardware);
    hardware->pwr_management.state = VIPDRV_POWER_ON;

    return status;
}

static vip_status_e power_on_state_out(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    status = power_off_action(hardware);
    hardware->init_done = vip_false_e;

    return status;
}

static vip_status_e power_idle_state_in(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdPOWER_OFF_TIMEOUT
    vipdrv_pm_t *pm = &hardware->pwr_management;
    /* enable timer */
    if (vip_true_e == pm->enable_timer) {
        vipdrv_os_stop_timer(pm->timer);
        vipdrv_os_start_timer(pm->timer, vpmdPOWER_OFF_TIMEOUT);
    }
#endif

    hardware->pwr_management.state = VIPDRV_POWER_IDLE;

    return status;
}

static vip_status_e power_idle_state_out(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdPOWER_OFF_TIMEOUT
    vipdrv_os_stop_timer(hardware->pwr_management.timer);
#endif

    return status;
}

static vip_status_e power_clock_state_in(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdENABLE_CLOCK_SUSPEND
    status = vipdrv_pm_enable_clock(hardware);
#endif
    /* set frequency */
    status |= vipdrv_pm_set_frequency(hardware, hardware->core_fscale_percent);

    hardware->pwr_management.state = VIPDRV_POWER_CLOCK;

    return status;
}

static vip_status_e power_clock_state_out(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    vipdrv_pm_set_frequency(hardware, 1);

#if vpmdENABLE_CLOCK_SUSPEND
    vipdrv_pm_disable_clock(hardware);
#endif

    return status;
}

static vip_status_e power_ready_state_in(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    hardware->pwr_management.state = VIPDRV_POWER_READY;

    return status;
}

static vip_status_e power_ready_state_out(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}

static vip_status_e power_run_state_in(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdTASK_PARALLEL_ASYNC
#if vpmdENABLE_TASK_PROFILE
    vipdrv_hw_profile_reset(hardware);
#endif
    /* start the "wait-link" loop for command queue. */
    status = vipdrv_hw_trigger_wl(hardware);
#endif

    hardware->disable_clock_suspend++;
    hardware->pwr_management.state = VIPDRV_POWER_RUN;

    return status;
}

static vip_status_e power_run_state_out(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdTASK_PARALLEL_ASYNC
    /* end the "wait-link" loop. */
    status = vipdrv_hw_stop_wl(hardware);
#endif

    hardware->disable_clock_suspend--;

    return status;
}

static vip_status_e power_suspend_state_in(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_SUSPEND_RESUME
    hardware->pwr_management.last_state = hardware->pwr_management.state;
#endif
    hardware->pwr_management.state = VIPDRV_POWER_SYS_SUSPEND;

    return status;
}

static vip_status_e power_suspend_state_out(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}

static vip_status_e pm_status_trans(
    vipdrv_hardware_t *hardware,
    vipdrv_power_state_e state_from,
    vipdrv_power_state_e state_to
    )
{
    vip_status_e status = VIP_SUCCESS;
    /* out status */
    if (VIPDRV_POWER_SYS_SUSPEND & state_from) {
        status |= power_suspend_state_out(hardware);
    }
    else if (VIPDRV_POWER_SYS_LIFE & state_from) {
        if (VIPDRV_POWER_OFF & state_from) {
            status |= power_off_state_out(hardware);
        }
        else if (VIPDRV_POWER_ON & state_from) {
            if (VIPDRV_POWER_IDLE & state_from) {
                status |= power_idle_state_out(hardware);
            }
            else if (VIPDRV_POWER_CLOCK & state_from) {
                if (VIPDRV_POWER_READY & state_from) {
                    status |= power_ready_state_out(hardware);
                }
                else if (VIPDRV_POWER_RUN & state_from) {
                    status |= power_run_state_out(hardware);
                }

                if (!(VIPDRV_POWER_CLOCK & state_to)) {
                    status |= power_clock_state_out(hardware);
                }
            }

            if (!(VIPDRV_POWER_ON & state_to)) {
                status |= power_on_state_out(hardware);
            }
        }

        if (!(VIPDRV_POWER_SYS_LIFE & state_to)) {
            status |= power_life_state_out(hardware);
        }
    }

    if (VIP_SUCCESS != status) {
        return status;
    }

    /* into status */
    if (VIPDRV_POWER_SYS_SUSPEND & state_to) {
        status |= power_suspend_state_in(hardware);
    }
    else if (VIPDRV_POWER_SYS_LIFE & state_to) {
        if (!(VIPDRV_POWER_SYS_LIFE & state_from)) {
            status |= power_life_state_in(hardware);
        }

        if (VIPDRV_POWER_OFF & state_to) {
            status |= power_off_state_in(hardware);
        }
        else if (VIPDRV_POWER_ON & state_to) {
            if (!(VIPDRV_POWER_ON & state_from)) {
                status |= power_on_state_in(hardware);
            }

            if (VIPDRV_POWER_IDLE & state_to) {
                status |= power_idle_state_in(hardware);
            }
            else if (VIPDRV_POWER_CLOCK & state_to) {
                if (!(VIPDRV_POWER_CLOCK & state_from)) {
                    status |= power_clock_state_in(hardware);
                }

                if (VIPDRV_POWER_READY & state_to) {
                    status |= power_ready_state_in(hardware);
                }
                else if (VIPDRV_POWER_RUN & state_to) {
                    status |= power_run_state_in(hardware);
                }
            }
        }
    }

    return status;
}

static char* pm_state_str(vipdrv_power_state_e state)
{
    switch (state) {
    case VIPDRV_POWER_IDLE: return "IDLE";
    case VIPDRV_POWER_READY: return "READY";
    case VIPDRV_POWER_RUN: return "RUN";
    case VIPDRV_POWER_OFF: return "POWER OFF";
    case VIPDRV_POWER_CLOCK: return "CLOCK ON";
    case VIPDRV_POWER_ON: return "POWER ON";
    case VIPDRV_POWER_SYS_LIFE: return "SYS LIFE";
    case VIPDRV_POWER_SYS_SUSPEND: return "SYS SUSPEND";
    case VIPDRV_POWER_NONE: return "NONE";
    default:
        return "UNKNOWN";
    }
}

static char* pm_event_str(vipdrv_power_evnt_e event)
{
    switch (event) {
    case VIPDRV_PWR_EVNT_ON: return "POWER ON";
    case VIPDRV_PWR_EVNT_OFF: return "POWER OFF";
    case VIPDRV_PWR_EVNT_CLK_ON: return "CLOCK ON";
    case VIPDRV_PWR_EVNT_CLK_OFF: return "CLOCK OFF";
    case VIPDRV_PWR_EVNT_RUN: return "HW RUN";
    case VIPDRV_PWR_EVNT_USR_ON: return "USER ON";
    case VIPDRV_PWR_EVNT_USR_OFF: return "USER OFF";
    case VIPDRV_PWR_EVNT_USR_STOP: return "USER STOP";
    case VIPDRV_PWR_EVNT_USR_START: return "USER START";
    case VIPDRV_PWR_EVNT_SUSPEND: return "SUSPEND";
    case VIPDRV_PWR_EVNT_RESUME: return "RESUME";
    case VIPDRV_PWR_EVNT_FREQ: return "SET FREQ";
    case VIPDRV_PWR_EVNT_ENABLE_TIMER: return "ENABLE TIMER";
    case VIPDRV_PWR_EVNT_DISABLE_TIMER: return "DISABLE TIMER";
    case VIPDRV_PWR_EVNT_END: return"HW END";
    default:
        return "UNKNOWN";
    }
}

vip_status_e vipdrv_pm_send_evnt(
    vipdrv_hardware_t *hardware,
    vipdrv_power_evnt_e evnt,
    void *data
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_power_state_e cur_status = VIPDRV_POWER_NONE;
    vipdrv_power_state_e target_status = VIPDRV_POWER_NONE;
    vipdrv_pm_t *pm = VIP_NULL;

    if (VIP_NULL == hardware) {
        PRINTK_E("fail hardware is NULL in pm set status\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    pm = &hardware->pwr_management;

    vipOnError(vipdrv_lock_recursive_mutex(&pm->mutex));
    cur_status = pm->state;
    /* 1. handle event */
    if (VIPDRV_POWER_SYS_SUSPEND & cur_status) {

    }
    else if (VIPDRV_POWER_SYS_LIFE & cur_status) {
        if (VIPDRV_POWER_OFF & cur_status) {

        }
        else if (VIPDRV_POWER_ON & cur_status) {
            if (VIPDRV_POWER_IDLE & cur_status) {

            }
            else if (VIPDRV_POWER_CLOCK & cur_status) {
                if (VIPDRV_POWER_READY & cur_status) {

                }
                else if (VIPDRV_POWER_RUN & cur_status) {
                    if (0 == hardware->disable_clock_suspend) {
                        PRINTK("clock suspend flag is 0 in RUN state!!!\n");
                    }
                }

                if (VIPDRV_PWR_EVNT_CLK_ON == evnt) {
                    hardware->disable_clock_suspend++;
                }
                else if (VIPDRV_PWR_EVNT_CLK_OFF == evnt) {
                    hardware->disable_clock_suspend--;
                }
            }
            #if vpmdPOWER_OFF_TIMEOUT
            else if (VIPDRV_PWR_EVNT_ENABLE_TIMER == evnt) {
                pm->enable_timer = vip_true_e;
            }
            else if (VIPDRV_PWR_EVNT_DISABLE_TIMER == evnt) {
                vipdrv_os_stop_timer(pm->timer);
                pm->enable_timer = vip_false_e;
            }
            #endif
        }

        if (VIPDRV_PWR_EVNT_FREQ == evnt) {
            vip_uint8_t* freq = (vip_uint8_t*)data;
            hardware->core_fscale_percent =  *freq <= 100 ? *freq : 100;
        }
        #if vpmdENABLE_USER_CONTROL_POWER
        else if (VIPDRV_PWR_EVNT_USR_ON == evnt) {
            pm->user_power_off = vip_false_e;
        }
        else if (VIPDRV_PWR_EVNT_USR_OFF == evnt) {
            #if vpmdTASK_QUEUE_USED
            vip_uint32_t clean_mask = 0;
            clean_mask = SETBIT(clean_mask, hardware->core_id, 1);
            pm->user_power_off = vip_true_e;
            vipdrv_queue_clean(&hardware->device->tskTCB_queue, VIPDRV_QUEUE_CLEAN_STASK, (void*)&clean_mask);
            vipdrv_pm_clean_task(hardware);
            #else
            pm->user_power_off = vip_true_e;
            #endif
        }
        else if (VIPDRV_PWR_EVNT_USR_START == evnt) {
            pm->user_power_stop = vip_false_e;
        }
        else if (VIPDRV_PWR_EVNT_USR_STOP == evnt) {
            #if vpmdTASK_QUEUE_USED
            vip_uint32_t clean_mask = 0;
            clean_mask = SETBIT(clean_mask, hardware->core_id, 1);
            pm->user_power_stop = vip_true_e;
            vipdrv_queue_clean(&hardware->device->tskTCB_queue, VIPDRV_QUEUE_CLEAN_STASK, (void*)&clean_mask);
            vipdrv_pm_clean_task(hardware);
            #else
            pm->user_power_stop = vip_true_e;
            #endif
        }
        #endif
    }

    /* 2. state trans */
    if (VIPDRV_POWER_SYS_SUSPEND & cur_status) {
        if (VIPDRV_PWR_EVNT_RESUME == evnt) {
            target_status = *((vipdrv_power_state_e*)data);
        }
    }
    else if (VIPDRV_POWER_SYS_LIFE & cur_status) {
        if (VIPDRV_POWER_OFF & cur_status) {
            if (VIPDRV_PWR_EVNT_USR_ON == evnt ||
                VIPDRV_PWR_EVNT_ON == evnt) {
                #if vpmdENABLE_USER_CONTROL_POWER
                if (vip_false_e == pm->user_power_off) {
                    target_status = VIPDRV_POWER_IDLE;
                }
                #else
                target_status = VIPDRV_POWER_IDLE;
                #endif
            }
        }
        else if (VIPDRV_POWER_ON & cur_status) {
            if (VIPDRV_POWER_IDLE & cur_status) {
                if (VIPDRV_PWR_EVNT_USR_OFF == evnt) {
                    target_status = VIPDRV_POWER_OFF;
                }
                else if (VIPDRV_PWR_EVNT_OFF == evnt) {
                    #if vpmdPOWER_OFF_TIMEOUT
                    if (vip_true_e == pm->enable_timer) {
                        target_status = VIPDRV_POWER_OFF;
                    }
                    #else
                    target_status = VIPDRV_POWER_OFF;
                    #endif
                }
                else if (VIPDRV_PWR_EVNT_RUN == evnt) {
                    target_status = VIPDRV_POWER_RUN;
                }
                else if (VIPDRV_PWR_EVNT_CLK_ON == evnt) {
                    target_status = VIPDRV_POWER_READY;
                }
            }
            else if (VIPDRV_POWER_CLOCK & cur_status) {
                if (VIPDRV_POWER_READY & cur_status) {
                    if (VIPDRV_PWR_EVNT_RUN == evnt) {
                        target_status = VIPDRV_POWER_RUN;
                    }
                }
                else if (VIPDRV_POWER_RUN & cur_status) {
                    if (VIPDRV_PWR_EVNT_END == evnt) {
                        vip_uint32_t max_wait_time = 100;
                        #if vpmdFPGA_BUILD
                        max_wait_time *= 20;
                        #endif
                        max_wait_time = max_wait_time / hardware->device->hardware_count;
                        #if vpmdTASK_PARALLEL_ASYNC
                        if (VIP_SUCCESS == vipdrv_hw_wait_idle(hardware, max_wait_time,
                            vip_true_e, EXPECT_IDLE_VALUE_NOT_FE)) {
                        #else
                        if (VIP_SUCCESS == vipdrv_hw_wait_idle(hardware, max_wait_time,
                            vip_true_e, EXPECT_IDLE_VALUE_ALL_MODULE)) {
                        #endif
                            target_status = VIPDRV_POWER_READY;
                        }
                        else {
                            PRINTK_E("error device %d hardware %d not idle\n",
                                    hardware->device->device_index, hardware->core_index);
                        }
                    }
                }

                if (VIPDRV_PWR_EVNT_CLK_OFF == evnt || VIPDRV_PWR_EVNT_END == evnt) {
                    if (0 == hardware->disable_clock_suspend) {
                        target_status = VIPDRV_POWER_IDLE;
                    }
                }
            }
        }

        if (VIPDRV_PWR_EVNT_SUSPEND == evnt) {
            target_status = VIPDRV_POWER_SYS_SUSPEND;
        }
    }

    PRINTK_I("dev%d hw%d PM event[%s], state[%s -> %s]\n",
            hardware->device->device_index, hardware->core_index, pm_event_str(evnt),
            pm_state_str(cur_status), pm_state_str(target_status));
    if (VIPDRV_POWER_NONE != target_status) {
        status = pm_status_trans(hardware, cur_status, target_status);
        if (VIP_SUCCESS != status) {
            PRINTK_E("dev%d hw%d power trans event[%s] convert from state %s to %s fail\n",
                hardware->device->device_index, hardware->core_index, pm_event_str(evnt),
                pm_state_str(cur_status), pm_state_str(target_status));
        }
        else {
            status = vipdrv_pm_send_evnt(hardware, evnt, data);
        }
    }
    vipdrv_unlock_recursive_mutex(&pm->mutex);

onError:
    return status;
}

/*
@brief a timer handle for power off VIP.
param: vipdrv_hardware_t
*/
#if vpmdPOWER_OFF_TIMEOUT
static vip_int32_t power_manage_thread(
    vip_ptr param
    )
{
    vip_int32_t ret = 0;
    vip_status_e status = VIP_SUCCESS;
    vipdrv_hardware_t *hardware = (vipdrv_hardware_t*)param;
    vipdrv_pm_t *pm = &hardware->pwr_management;

    while (1) {
        if (vip_false_e == pm->thread_running) {
            PRINTK_D("power manage thread not running, exit thread before wait signal\n");
            break;
        }

        if (VIP_SUCCESS == vipdrv_os_wait_signal((vip_ptr*)pm->signal, vpmdINFINITE)) {
            PRINTK_D("power management thread wake up.\n");
            /* wait signal next time */
            vipdrv_os_set_signal(pm->signal, vip_false_e);

            if (vip_false_e == pm->thread_running) {
                PRINTK_D("power manage thread not running, exit thread after wait signal\n");
                break;
            }

            status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_OFF, VIP_NULL);
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
    vipdrv_os_set_signal(pm->exit_signal, vip_true_e);

    if (status != VIP_SUCCESS) {
        ret = -1;
    }
    return ret;
}

static void power_off_timer_handle(
    vip_ptr param
    )
{
    vipdrv_queue_status_e queue_status = VIPDRV_QUEUE_EMPTY;
    vipdrv_hardware_t *hardware = (vipdrv_hardware_t*)param;
    vipdrv_pm_t *pm = &hardware->pwr_management;
#if vpmdTASK_QUEUE_USED
    vip_uint16_t res_mask = 1 << hardware->core_id;
    queue_status = vipdrv_queue_status(&hardware->device->tskTCB_queue, &res_mask);
#endif

    PRINTK_D("power off timer start. timeout=%dms, muti-task queue=%s\n",
              vpmdPOWER_OFF_TIMEOUT, (VIPDRV_QUEUE_EMPTY == queue_status) ? "empty" : "not-empty");

    /* power off the hardware */
    if (VIPDRV_QUEUE_EMPTY == queue_status) {
        /* trigger power manage thread */
        vipdrv_os_set_signal(pm->signal, vip_true_e);
    }
    else {
        PRINTK_D("power off timer handler, queue not empty, by pass power off\n");
    }
}
#endif

vip_status_e vipdrv_pm_hw_submit(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_pm_t* pm = &hardware->pwr_management;
    status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_RUN, VIP_NULL);
    if (VIPDRV_POWER_RUN != pm->state) {
        PRINTK_E("invalid power state=%d\n", pm->state);
        status = VIP_ERROR_POWER_OFF;
    }
    else {
        vipdrv_os_set_atomic(hardware->idle, vip_false_e);
    }

    return status;
}

vip_status_e vipdrv_pm_hw_idle(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    vipdrv_os_set_atomic(hardware->idle, vip_true_e);
    status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_END, VIP_NULL);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to set pm state to end, status=%d\n", status);
    }

#if vpmdENABLE_USER_CONTROL_POWER
    if (vip_true_e == hardware->pwr_management.user_power_off) {
        status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_USR_OFF, VIP_NULL);
    }
#endif

    return status;
}

/*
@bried initialize driver power management module.
*/
vip_status_e vipdrv_pm_init(void)
{
    vip_status_e status = VIP_SUCCESS;
    VIPDRV_LOOP_HARDWARE_START
    vipdrv_pm_t *pm = &hw->pwr_management;
    #if vpmdPOWER_OFF_TIMEOUT
    vip_char_t name[256];
    vip_ptr param = (vip_ptr)hw;
    vipdrv_os_snprint(name, 255, "power_manage_%d", hw_index);
    #endif
    status = vipdrv_create_recursive_mutex(&pm->mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create mutex for power memory\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    #if vpmdPOWER_OFF_TIMEOUT
    status = vipdrv_os_create_signal(&pm->signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create a signal for power\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    status = vipdrv_os_create_signal(&pm->exit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create a signal for thread exit\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_set_signal(pm->exit_signal, vip_false_e);
    pm->thread_running = vip_true_e;
    status = vipdrv_os_create_thread((vipdrv_thread_func)power_manage_thread,
                                    param, name, &pm->thread);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create a thread for supporting power manage.\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    pm->enable_timer = vip_true_e;

    status = vipdrv_os_create_timer((vipdrv_timer_func)power_off_timer_handle,
                                    param, &pm->timer);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create timer for power off\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_stop_timer(pm->timer);
    #endif

    /* create signals for system suspend/resume feature */
    #if vpmdENABLE_SUSPEND_RESUME
    status = vipdrv_os_create_signal(&pm->resume_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create resume signal\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    #endif

    #if vpmdENABLE_USER_CONTROL_POWER
    pm->user_power_off = vip_false_e;
    pm->user_power_stop = vip_false_e;
    #endif

    /* hardware has been power on in vipdrv_os_init */
    PRINTK_D("dev%d hw%d power management is enabled\n", hw->device->device_index, hw->core_index);
    pm->state = VIPDRV_POWER_IDLE;
    VIPDRV_LOOP_HARDWARE_END

    return status;

onError:
    VIPDRV_LOOP_HARDWARE_START
    vipdrv_pm_t *pm = &hw->pwr_management;

    vipdrv_destroy_recursive_mutex(&pm->mutex);
    #if vpmdPOWER_OFF_TIMEOUT
    if (pm->signal != VIP_NULL) {
        vipdrv_os_destroy_signal(pm->signal);
        pm->signal = VIP_NULL;
    }
    if (pm->thread != VIP_NULL) {
        vipdrv_os_destroy_thread(pm->thread);
        pm->thread = VIP_NULL;
    }
    if (pm->timer != VIP_NULL) {
        vipdrv_os_stop_timer(pm->timer);
        vipdrv_os_destroy_timer(pm->timer);
        pm->timer = VIP_NULL;
    }
    #endif
    #if vpmdENABLE_SUSPEND_RESUME
    if (pm->resume_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(pm->resume_signal);
    }
    #endif
    pm->state = VIPDRV_POWER_NONE;
    VIPDRV_LOOP_HARDWARE_END

    return status;
}

vip_status_e vipdrv_pm_pre_uninit(void)
{
    vip_status_e status = VIP_SUCCESS;

    VIPDRV_LOOP_HARDWARE_START
    hw->disable_clock_suspend = 0xDEADBEEF;
#if vpmdPOWER_OFF_TIMEOUT
    /* power thread exit */
    hw->pwr_management.thread_running = vip_false_e;
    if (hw->pwr_management.signal != VIP_NULL) {
        vipdrv_os_set_signal(hw->pwr_management.signal, vip_true_e);
    }
#endif
    VIPDRV_LOOP_HARDWARE_END

    return status;
}

/*
@bried un-initialize driver power management module.
*/
vip_status_e vipdrv_pm_uninit(void)
{
    VIPDRV_LOOP_HARDWARE_START
    vipdrv_pm_t *pm = &hw->pwr_management;
    #if vpmdPOWER_OFF_TIMEOUT
    if (pm->exit_signal != VIP_NULL) {
        /* waiting pm thread exit */
        vipdrv_os_wait_signal((vip_ptr*)pm->exit_signal, vpmdINFINITE);
    }
    if (pm->timer != VIP_NULL) {
        vipdrv_os_stop_timer(pm->timer);
        vipdrv_os_destroy_timer(pm->timer);
        pm->timer = VIP_NULL;
    }
    if (pm->signal != VIP_NULL) {
        vipdrv_os_destroy_signal(pm->signal);
        pm->signal = VIP_NULL;
    }
    if (pm->thread != VIP_NULL) {
        vipdrv_os_destroy_thread(pm->thread);
        pm->thread = VIP_NULL;
    }
    if (pm->exit_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(pm->exit_signal);
        pm->exit_signal = VIP_NULL;
    }
    #endif
    vipdrv_destroy_recursive_mutex(&pm->mutex);
    #if vpmdENABLE_SUSPEND_RESUME
    if (pm->resume_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(pm->resume_signal);
        pm->resume_signal = VIP_NULL;
    }
    #endif
    /* power off in vipdrv_os_close */
    pm->state = VIPDRV_POWER_NONE;
    VIPDRV_LOOP_HARDWARE_END

    PRINTK_D("power management uninit done\n");
    return VIP_SUCCESS;
}

#if vpmdENABLE_SUSPEND_RESUME
/*
@brief  uninitialize hardware for system suspend
*/
vip_status_e vipdrv_pm_suspend(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t orig_tid = 0;
    vip_uint32_t* initialize = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED);
    vipdrv_pm_t* pm = VIP_NULL;
    if (0 == *initialize) {
        goto onError;
    }

    VIPDRV_LOOP_DEVICE_START
    if (vip_true_e == vipdrv_context_support(VIP_HW_FEATURE_JOB_CANCEL)) {
        vip_uint8_t cancel_flag[vipdMAX_CORE] = { 0 };
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
        cancel_flag[hw_index] = 1;
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
        vipdrv_hw_cancel(device, cancel_flag);
    }
    else {
        vipdrv_device_wait_idle(device, 500, vip_false_e);
    }

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    pm = &hw->pwr_management;
    orig_tid = pm->mutex.current_tid;
    pm->mutex.current_tid = vipdrv_os_get_tid();
    status = vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_SUSPEND, VIP_NULL);
    pm->mutex.current_tid = orig_tid;

    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to suspend pm state\n");
        goto onError;
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    VIPDRV_LOOP_DEVICE_END

onError:
    return status;
}

/*
@brief  initialize hardware for system resume and power management
*/
vip_status_e vipdrv_pm_resume(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t orig_tid = 0;
    vipdrv_power_state_e last_state = VIPDRV_POWER_NONE;
    vip_uint32_t* initialize = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED);
    vipdrv_pm_t* pm = VIP_NULL;
    if (0 == *initialize) {
        return VIP_SUCCESS;
    }

    VIPDRV_LOOP_DEVICE_START
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    pm = &hw->pwr_management;
    orig_tid = pm->mutex.current_tid;
    pm->mutex.current_tid = vipdrv_os_get_tid();
    last_state = hw->pwr_management.last_state;
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_RESUME, &last_state);
    pm->mutex.current_tid = orig_tid;

    if (VIPDRV_POWER_RUN & last_state) {
        /* regard system suspend as exception */
        hw->irq_local_value = IRQ_VALUE_CANCEL;
        #if vpmdTASK_PARALLEL_ASYNC
        *device->irq_value |= IRQ_VALUE_FLAG;
        #else
        *hw->irq_value |= IRQ_VALUE_FLAG;
        #endif
        #if !vpmdENABLE_POLLING
        vipdrv_os_inc_atomic(*hw->irq_count);
        #if vpmdTASK_PARALLEL_ASYNC
        vipdrv_os_wakeup_interrupt(device->irq_queue);
        #else
        vipdrv_os_wakeup_interrupt(hw->irq_queue);
        #endif
        #endif
    }

    PRINTK_I("wake up resume signal..\n");
    vipdrv_os_set_signal(pm->resume_signal, vip_true_e);

    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to resume pm state\n");
        goto onError;
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    VIPDRV_LOOP_DEVICE_END

onError:
    return status;
}
#endif

#if vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
/*
@brief check power status before submit hardware command.
*/
vip_status_e vipdrv_pm_check_power(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    PRINTK_I("dev%d hw%d cur power state=%s\n", hardware->device->device_index, hardware->core_index,
            pm_state_str(hardware->pwr_management.state));
    #if vpmdENABLE_SUSPEND_RESUME
    vipdrv_lock_recursive_mutex(&hardware->pwr_management.mutex);
    if (VIPDRV_POWER_SYS_SUSPEND == hardware->pwr_management.state) {
        vip_uint32_t count = 0;
        vip_int32_t ref_count = 0;
        vip_int32_t i = 0;
        /* waiting for system resume again */
        /* system suspended, should waiting for resume signal,
           then a task can be submited to hardware,
           release the pm lock firstly,
           get the pm lock again after resume
        */
        ref_count = hardware->pwr_management.mutex.ref_count;
        for (i = 0; i < ref_count; i++) {
            vipdrv_unlock_recursive_mutex(&hardware->pwr_management.mutex);
        }

        vipdrv_os_set_signal(hardware->pwr_management.resume_signal, vip_false_e);

        while (1) {
            PRINTK_I("waiting for system resume, wait count=%d...\n", count);
            if (VIP_SUCCESS == vipdrv_os_wait_signal(hardware->pwr_management.resume_signal,
                WAIT_SIGNAL_TIMEOUT)) {
                break;
            }
            count++;
        }

        for (i = 0; i < ref_count; i++) {
            vipdrv_lock_recursive_mutex(&hardware->pwr_management.mutex);
        }
    }
    vipdrv_unlock_recursive_mutex(&hardware->pwr_management.mutex);
    #endif

    /* power management */
    #if vpmdPOWER_OFF_TIMEOUT
    status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_ON, VIP_NULL);
    PRINTK_D("submit command check power on dev%d hw%d\n", hardware->device->device_index, hardware->core_index);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to check power vip going to idle status=%d.\n", status);
    }
    #endif

    return status;
}
#endif

vip_status_e vipdrv_pm_power_management(
    vipdrv_hardware_t* hardware,
    vipdrv_power_management_t *power
    )
{
    vip_status_e status = VIP_SUCCESS;

    switch (power->property) {
#if vpmdENABLE_USER_CONTROL_POWER
    case VIPDRV_POWER_PROPERTY_SET_FREQUENCY:
    {
        status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_FREQ, &power->fscale_percent);
    }
    break;

    case VIPDRV_POWER_PROPERTY_OFF:
    {
        status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_USR_OFF, VIP_NULL);
    }
    break;

    case VIPDRV_POWER_PROPERTY_ON:
    {
        status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_USR_ON, VIP_NULL);
    }
    break;

    case VIPDRV_POWER_PROPERTY_STOP:
    {
        status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_USR_STOP, VIP_NULL);
    }
    break;

    case VIPDRV_POWER_PROPERTY_START:
    {
        status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_USR_START, VIP_NULL);
    }
    break;
#endif

#if vpmdPOWER_OFF_TIMEOUT
    case VIPDRV_POWER_ENABLE_TIMER:
    {
        status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_ENABLE_TIMER, VIP_NULL);
    }
    break;

    case VIPDRV_POWER_DISABLE_TIMER:
    {
        status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_DISABLE_TIMER, VIP_NULL);
    }
    break;
#endif

    default:
        PRINTK_E("application setting power management, not support property=%d\n",
                 power->property);
        break;
    }

    return status;
}

vip_status_e vipdrv_pm_lock_hardware(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

     vipOnError(vipdrv_lock_recursive_mutex(&hardware->pwr_management.mutex));

    return status;
onError:
    PRINTK_E("fail to lock hw%d status=%d\n", hardware->core_index, status);
    return status;
}

vip_status_e vipdrv_pm_unlock_hardware(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

     vipOnError(vipdrv_unlock_recursive_mutex(&hardware->pwr_management.mutex));

    return status;
onError:
    PRINTK_E("fail to unlock hw%d status=%d\n", hardware->core_index, status);
    return status;
}

vip_status_e vipdrv_pm_lock_device(
    vipdrv_device_t* device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vipdrv_hardware_t *hardware = VIP_NULL;

    for (i = 0 ; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
        if (hardware != VIP_NULL) {
            vipOnError(vipdrv_lock_recursive_mutex(&hardware->pwr_management.mutex));
        }
    }

    return status;
onError:
    PRINTK_E("fail to lock dev%d status=%d\n", device->device_index, status);
    return status;
}

vip_status_e vipdrv_pm_unlock_device(
    vipdrv_device_t* device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vipdrv_hardware_t *hardware = VIP_NULL;

    for (i = 0 ; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
        if (hardware != VIP_NULL) {
            status |= vipdrv_unlock_recursive_mutex(&hardware->pwr_management.mutex);
        }
    }

    return status;
}

vip_status_e vipdrv_pm_lock_device_hw(
    vipdrv_device_t* device,
    vip_uint32_t start_core_index,
    vip_uint32_t core_cnt
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vipdrv_hardware_t *hardware = VIP_NULL;

    for (i = 0 ; i < core_cnt; i++) {
        hardware = &device->hardware[start_core_index + i];
        if (hardware != VIP_NULL) {
            vipOnError(vipdrv_lock_recursive_mutex(&hardware->pwr_management.mutex));
        }
    }

    return status;
onError:
    PRINTK_E("fail to lock dev%d:%d:%d hw status=%d\n", device->device_index, start_core_index, core_cnt, status);
    return status;
}

vip_status_e vipdrv_pm_unlock_device_hw(
    vipdrv_device_t* device,
    vip_uint32_t start_core_index,
    vip_uint32_t core_cnt
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vipdrv_hardware_t *hardware = VIP_NULL;

    for (i = 0 ; i < core_cnt; i++) {
        hardware = &device->hardware[start_core_index + i];
        if (hardware != VIP_NULL) {
            status |= vipdrv_unlock_recursive_mutex(&hardware->pwr_management.mutex);
        }
    }

    return status;
}

vipdrv_power_state_e vipdrv_pm_query_state(
    vipdrv_hardware_t* hardware
    )
{
    return hardware->pwr_management.state;
}
#endif

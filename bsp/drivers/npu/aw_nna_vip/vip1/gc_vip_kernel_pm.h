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

#ifndef _GC_VIP_KERNEL_PM_H
#define _GC_VIP_KERNEL_PM_H

#include <gc_vip_common.h>
#include <vip_lite.h>
#include <gc_vip_kernel.h>

#if vpmdENABLE_USER_CONTROL_POWER && !vpmdONE_POWER_DOMAIN
#define GCKVIP_CHECK_USER_PM_STATUS()                                                       \
{                                                                                           \
    vip_uint32_t pm_status = gckvip_os_get_atomic(device->dp_management.status);            \
    if ((GCKVIP_POWER_OFF == pm_status)) {                                                  \
        PRINTK_I("vip has been off by application\n");                                      \
        return VIP_ERROR_POWER_OFF;                                                         \
    }                                                                                       \
    else if (GCKVIP_POWER_STOP == pm_status) {                                              \
        PRINTK_I("vip has been stopped by application\n");                                  \
        return VIP_ERROR_POWER_STOP;                                                        \
    }                                                                                       \
}
#elif vpmdENABLE_USER_CONTROL_POWER && vpmdONE_POWER_DOMAIN
#define GCKVIP_CHECK_USER_PM_STATUS()                                                       \
{                                                                                           \
    vip_uint32_t pm_status = gckvip_os_get_atomic(context->sp_management.status);           \
    if ((GCKVIP_POWER_OFF == pm_status)) {                                                  \
        PRINTK_I("vip has been off by application\n");                                      \
        return VIP_ERROR_POWER_OFF;                                                         \
    }                                                                                       \
    else if (GCKVIP_POWER_STOP == pm_status) {                                              \
        PRINTK_I("vip has been stopped by application\n");                                  \
        return VIP_ERROR_POWER_STOP;                                                        \
    }                                                                                       \
}
#else
#define GCKVIP_CHECK_USER_PM_STATUS()
#endif

/*********************************************************************************************************************
                                                         _ _ _ _
                                                        /       \
                                                        |       |
                                                        |      \|/
 -------------------------------            -------------------------------            -------------------------------
 |       |      power off      |            |       |      query = 0      |            |       |                     |
 |       | in:  clean tasks    |            |       | in:  timer start    |            |       | in:  power off      |
 |       |      exe count = 0  | ---------> |       |      clock off all  | ---------> |System |      exe count = 0  |
 |  OFF  |---------------------|            | IDLE  |---------------------|            |SUSPEND|---------------------|
 |       |      power on       | <--------- |       |                     | <--------- |       |      power on       |
 |       | out: init done = 0  |[0 == query]|       | out: timer stop     |            |       | out: init done = 0  |
 |       |      user off = 0   |            |       |                     |            |       |                     |
 -------------------------------            -------------------------------            -------------------------------
                                             |  /|\        |   /|\
            _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _|   |         |    |
           /  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _|         |    | [(all) device idle || force idle]
          /  /[1 == user off ||    |[(all) device idle]    |    |
        |/_ /  0 == user stop]     |                      \|/   |
 -------------------------------   |        -------------------------------
 |       |                     |   |        |       |      init done = 1  |
 |       | in:  clear tasks    |   |[else]  |       | in:  clock on device|
 |       |                     |   -------> |       |                     |
 | STOP  |---------------------|            |  ON   |---------------------|
 |       |                     | <--------- |       |                     |
 |       | out:                |            |       | out:                |
 |       |                     |            |       |                     |
 -------------------------------            -------------------------------
                                                        |      /|\
                                                        |       |
                                                        \       /
                                                         \_ _ _/
*********************************************************************************************************************/
vip_status_e gckvip_pm_set_status(
    gckvip_context_t      *context,
    gckvip_device_t       *device,
    gckvip_power_status_e pm_status
    );

/*
@bried initialize driver power management module.
*/
vip_status_e gckvip_pm_init(
    gckvip_context_t *context
    );

/*
@bried un-initialize driver power management module.
*/
vip_status_e gckvip_pm_uninit(
    gckvip_context_t *context
    );

#if vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
/*
@brief check power status before submit hardware command.
*/
vip_status_e gckvip_pm_check_power(
    gckvip_context_t *context,
    gckvip_device_t  *device
    );
#endif

#if vpmdENABLE_SUSPEND_RESUME
/*
@brief  uninitialize hardware for system suspend
*/
vip_status_e gckvip_pm_suspend(void);

/*
@brief  initialize hardware for system resume and power management
*/
vip_status_e gckvip_pm_resume(void);
#endif

#if vpmdENABLE_USER_CONTROL_POWER || vpmdPOWER_OFF_TIMEOUT
#if vpmdENABLE_USER_CONTROL_POWER
vip_status_e gckvip_pm_user_power_off(
    gckvip_context_t *context,
    gckvip_device_t *device
    );
#endif

vip_status_e gckvip_pm_power_management(
    gckvip_power_management_t *power,
    gckvip_device_t *device
    );
#endif

vip_status_e gckvip_pm_hw_idle(
    gckvip_context_t *context,
    gckvip_device_t *device
    );

vip_status_e gckvip_pm_hw_submit(
    gckvip_context_t *context,
    gckvip_device_t *device
    );

vip_status_e gckvip_pm_set_frequency(
    gckvip_device_t *device,
    vip_uint8_t fscale_percent
    );

#if vpmdENABLE_CLOCK_SUSPEND
vip_status_e gckvip_pm_disable_clock(
    gckvip_device_t *device
    );

vip_status_e gckvip_pm_enable_clock(
    gckvip_device_t *device
    );

vip_status_e gckvip_pm_enable_hardware_clock(
    gckvip_hardware_t *hardware
    );

vip_status_e gckvip_pm_disable_hardware_clock(
    gckvip_hardware_t *hardware
    );
#endif

#if vpmdENABLE_POWER_MANAGEMENT
vip_status_e gckvip_pm_query_info(
    gckvip_context_t *context,
    gckvip_query_power_info_t *info
    );
#endif

#endif

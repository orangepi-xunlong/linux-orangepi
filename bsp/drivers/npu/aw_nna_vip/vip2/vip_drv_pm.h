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

#ifndef _VIP_DRV_PM_H_
#define _VIP_DRV_PM_H_

#include <vip_drv_type.h>

typedef enum _vipdrv_power_state
{
    VIPDRV_POWER_NONE         = 0x0,
    VIPDRV_POWER_IDLE         = 0x1,  /* external power and clock on, internal clock off or 1/64 freq */
    VIPDRV_POWER_READY        = 0x2,  /* clock on, not task running */
    VIPDRV_POWER_RUN          = 0x4,  /* up freq and task running */
    VIPDRV_POWER_OFF          = 0x8,  /* external power and clock off */

    VIPDRV_POWER_SYS_SUSPEND  = 0x10, /* system suspend, Only for Linux/Android */

    VIPDRV_POWER_CLOCK        = 0x6,  /* VIPDRV_POWER_READY | VIPDRV_POWER_RUN */
    VIPDRV_POWER_ON           = 0x7,  /* VIPDRV_POWER_CLOCK | VIPDRV_POWER_IDLE */
    VIPDRV_POWER_SYS_LIFE     = 0xF,  /* VIPDRV_POWER_ON | VIPDRV_POWER_OFF */
} vipdrv_power_state_e;

#if vpmdENABLE_POWER_MANAGEMENT
#include <vip_drv_util.h>

/**********************************************************************************************************************
___________________               _____________________________________________________________________________________
|   SYS SUSPEND   |               |                                   SYS LIFE                                        |
|-----------------|               |-----------------------------------------------------------------------------------|
|IN  : save state |    RESUME     |EVNT: FREQ: set freq                         _____________________________         |
|                 |-------------->|      USR_POWER_ON: mark user power on       |            OFF            |         |
|                 |               |      USR_POWER_OFF: mark user power off;    |---------------------------|         |
|                 |               |                     clean task              |                           |         |
|                 |               |      USR_STOP: mark user stop; clean task   |___________________________|         |
|                 |               |      USR_START: mark user start                    |            /|\               |
|                 |    SUSPEND    |                                                    | POWER_ON    | POWER_OFF      |
|                 |<--------------| ___________________________________________________|_____________|_______________ |
|                 |               | |                                      ON          |             |              | |
|_________________|               | |--------------------------------------------------|-------------|--------------| |
                                  | |IN  : power on                  _________________\|/____________|_____________ | |
                                  | |OUT : power off                 |                    IDLE                    | | |
                                  | |EVNT: INIT_DONE: user init done |--------------------------------------------| | |
                                  | |      ENABLE_TIMER              |IN  : start timer                           | | |
                                  | |      DISABLE_TIMER             |OUT : end timer                             | | |
                                  | |                                |____________________________________________| | |
                                  | |                                /|\                |                 |         | |
                                  | |                                 | CLOCK_OFF||END  | CLOCK_ON        |         | |
                                  | |                                 | [0 == clk flag] |                 |         | |
                                  | | ________________________________|_________________|_________________|________ | |
  REMARK:                         | | |                                    CLOCK        |                 |       | | |
  --XXX->: state trans by evnt    | | |-------------------------------------------------|-----------------|-------| | |
  [ XXX ]: condition for trans    | | |IN  : clock on               ___________________\|/___             |       | | |
  IN     : state in action        | | |      up freq                |        READY          |             |       | | |
  OUT    : state out action       | | |OUT : down freq              |-----------------------|             |       | | |
  EVNT   : handle event in state  | | |      clock off              |                       |             | RUN   | | |
                                  | | |EVNT: CLOCK_ON: clk flag++   |_______________________|             |       | | |
                                  | | |      CLOCK_OFF: clk flag--         |        /|\                   |       | | |
                                  | | |                                    |         |                    |       | | |
                                  | | |                                    | RUN     | END                |       | | |
                                  | | |                                    |         | [hardware idle]    |       | | |
                                  | | |                             ______\|/________|___________________\|/__    | | |
                                  | | |                             |                  RUN                   |    | | |
                                  | | |                             |----------------------------------------|    | | |
                                  | | |                             |IN  : clk flag++                        |    | | |
                                  | | |                             |      trigger wait-link table           |    | | |
                                  | | |                             |OUT : end wait-link                     |    | | |
                                  | | |                             |      clk flag--                        |    | | |
                                  | | |                             |________________________________________|    | | |
                                  | | |___________________________________________________________________________| | |
                                  | |_______________________________________________________________________________| |
                                  |___________________________________________________________________________________|
**********************************************************************************************************************/

typedef enum _vipdrv_power_evnt
{
    VIPDRV_PWR_EVNT_ON            = 0,
    VIPDRV_PWR_EVNT_OFF           = 1,
    VIPDRV_PWR_EVNT_CLK_ON        = 2,
    VIPDRV_PWR_EVNT_CLK_OFF       = 3,
    VIPDRV_PWR_EVNT_RUN           = 4,
    VIPDRV_PWR_EVNT_USR_ON        = 5,
    VIPDRV_PWR_EVNT_USR_OFF       = 6,
    VIPDRV_PWR_EVNT_USR_STOP      = 7,
    VIPDRV_PWR_EVNT_USR_START     = 8,
    VIPDRV_PWR_EVNT_SUSPEND       = 9,
    VIPDRV_PWR_EVNT_RESUME        = 10,
    VIPDRV_PWR_EVNT_FREQ          = 11,
    VIPDRV_PWR_EVNT_ENABLE_TIMER  = 12,
    VIPDRV_PWR_EVNT_DISABLE_TIMER = 13,
    VIPDRV_PWR_EVNT_END           = 14,
    VIPDRV_PWR_EVNT_MAX
} vipdrv_power_evnt_e;

typedef struct _vipdrv_pm
{
    volatile vipdrv_power_state_e state;
    vipdrv_recursive_mutex mutex;
#if vpmdENABLE_SUSPEND_RESUME
    /* A signal for waiting system resume, then submit a new task to hardware */
    vipdrv_signal          resume_signal;
    /* the state of the power when system last suspend */
    vipdrv_power_state_e   last_state;
#endif
#if vpmdPOWER_OFF_TIMEOUT
    vipdrv_timer     timer;
    vipdrv_signal    signal;
    void             *thread; /* thread object*/
    volatile vip_uint8_t thread_running;
    vip_uint8_t enable_timer;
    vipdrv_signal    exit_signal;
#endif
#if vpmdENABLE_USER_CONTROL_POWER
    vip_uint8_t user_power_off;
    vip_uint8_t user_power_stop;
#endif
} vipdrv_pm_t;

#if vpmdENABLE_USER_CONTROL_POWER
#define VIPDRV_CHECK_USER_PM_STATUS()                                       \
{                                                                           \
    if (vip_true_e == hw->pwr_management.user_power_off) {                  \
        PRINTK_I("core %d has been off by application\n", hw->core_id);     \
        return VIP_ERROR_POWER_OFF;                                         \
    }                                                                       \
    else if (vip_true_e == hw->pwr_management.user_power_stop) {            \
        PRINTK_I("core %d has been stopped by application\n", hw->core_id); \
        return VIP_ERROR_POWER_STOP;                                        \
    }                                                                       \
}
#endif

vip_status_e vipdrv_pm_send_evnt(
    vipdrv_hardware_t *hardware,
    vipdrv_power_evnt_e   evnt,
    void                  *value
    );

/*
@bried initialize driver power management module.
*/
vip_status_e vipdrv_pm_init(void);


/*
@bried do something pre un-initialize power management
*/
vip_status_e vipdrv_pm_pre_uninit(void);

/*
@bried un-initialize driver power management module.
*/
vip_status_e vipdrv_pm_uninit(void);

#if vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
/*
@brief check power status before submit hardware command.
*/
vip_status_e vipdrv_pm_check_power(
    vipdrv_hardware_t* hardware
    );
#endif

#if vpmdENABLE_SUSPEND_RESUME
/*
@brief  uninitialize hardware for system suspend
*/
vip_status_e vipdrv_pm_suspend(void);

/*
@brief  initialize hardware for system resume and power management
*/
vip_status_e vipdrv_pm_resume(void);
#endif

vip_status_e vipdrv_pm_power_management(
    vipdrv_hardware_t *hardware,
    vipdrv_power_management_t *power
    );

vip_status_e vipdrv_pm_hw_idle(
    vipdrv_hardware_t* hardware
    );

vip_status_e vipdrv_pm_hw_submit(
    vipdrv_hardware_t* hardware
    );

vip_status_e vipdrv_pm_set_frequency(
    vipdrv_hardware_t* hardware,
    vip_uint8_t fscale_percent
    );

vip_status_e vipdrv_pm_lock_hardware(
    vipdrv_hardware_t* hardware
    );

vip_status_e vipdrv_pm_unlock_hardware(
    vipdrv_hardware_t* hardware
    );

vip_status_e vipdrv_pm_lock_device(
    vipdrv_device_t* device
    );

vip_status_e vipdrv_pm_unlock_device(
    vipdrv_device_t* device
    );

vip_status_e vipdrv_pm_lock_device_hw(
    vipdrv_device_t* device,
    vip_uint32_t start_core_index,
    vip_uint32_t core_cnt
    );

vip_status_e vipdrv_pm_unlock_device_hw(
    vipdrv_device_t* device,
    vip_uint32_t start_core_index,
    vip_uint32_t core_cnt
    );

vipdrv_power_state_e vipdrv_pm_query_state(
    vipdrv_hardware_t* hardware
    );

#endif
#endif

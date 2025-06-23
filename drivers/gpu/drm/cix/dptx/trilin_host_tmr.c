// SPDX-License-Identifier: GPL-2.0
//------------------------------------------------------------------------------
//	Trilinear Technologies DisplayPort DRM Driver
//	Copyright (C) 2023 Trilinear Technologies
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, version 2.
//
//	This program is distributed in the hope that it will be useful, but
//	WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//	General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program. If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>

#include "trilin_dptx_reg.h"
#include "trilin_dptx.h"
#include "trilin_host_tmr.h"

//------------------------------------------------------------------------------
//  Time Values:
//
//  Time-base       Source
//  Microseconds    Timer clock is 1MHz
//  Milliseconds    Microsecond time * 1000
//------------------------------------------------------------------------------
#define DEV_TMR_MS              1000

//------------------------------------------------------------------------------
//  Function: trilin_host_tmr_init
//  Initialize device timer
//
//  Parameters:
//      dp - timer base address
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void trilin_host_tmr_init(struct trilin_dp *dp)
{
    trilin_dp_write(dp, TRILIN_DPTX_HOST_TIMER, 0ul);
}

//------------------------------------------------------------------------------
//  Function: trilin_host_tmr_get_value
//  Get value from a timer
//
//  Parameters:
//      dp - timer base address
//
//  Returns:
//      Timer value
//------------------------------------------------------------------------------
u32 trilin_host_tmr_get_value(struct trilin_dp *dp)
{
    u32 tmr_val = trilin_dp_read(dp, TRILIN_DPTX_HOST_TIMER);

    return (tmr_val & DEV_TMR_MAX_CT);
}

//------------------------------------------------------------------------------
//  Function: timers_set_ct
//      trilin_host_tmr_set_ct - Set device timer value in counts
//      trilin_host_tmr_set_us - Set device timer value in microseconds
//      trilin_host_tmr_set_ms - Set device timer value in milliseconds
//
//  Parameters:
//      dp - timer base address
//      tmr_val - New 20-bit timer value
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void trilin_host_tmr_set_ct(struct trilin_dp *dp, u32 tmr_ct)
{
    u32 tmr_val = trilin_dp_read(dp, TRILIN_DPTX_HOST_TIMER);

    tmr_val = (tmr_val & DEV_TMR_FLAGS_ALL) | (DEV_TMR_MAX_CT & tmr_ct);
    trilin_dp_write(dp, TRILIN_DPTX_HOST_TIMER, tmr_val);
}
void trilin_host_tmr_set_us(struct trilin_dp *dp, u32 tmr_us)
{
    u32 tmr_val = trilin_dp_read(dp, TRILIN_DPTX_HOST_TIMER);

    tmr_val = (tmr_val & DEV_TMR_FLAGS_ALL) | (DEV_TMR_MAX_CT & tmr_us);
    trilin_dp_write(dp, TRILIN_DPTX_HOST_TIMER, tmr_val);
}
void trilin_host_tmr_set_ms(struct trilin_dp *dp, u32 tmr_ms)
{
    u32 tmr_val = trilin_dp_read(dp, TRILIN_DPTX_HOST_TIMER);

    tmr_val = (tmr_val & DEV_TMR_FLAGS_ALL) | (DEV_TMR_MAX_CT & (tmr_ms * DEV_TMR_MS));
    trilin_dp_write(dp, TRILIN_DPTX_HOST_TIMER, tmr_val);
}

//------------------------------------------------------------------------------
//  Function: trilin_host_tmr_enable / trilin_host_tmr_disable
//  Short cut functions for enable / disable
//
//  Parameters:
//      dp   - timer base address
//      enable - enable / disable the timer
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void trilin_host_tmr_enable(struct trilin_dp *dp, bool enable)
{
    u32 tmr_val = trilin_dp_read(dp, TRILIN_DPTX_HOST_TIMER);

    if (enable == true) {
        tmr_val |= DEV_TMR_FLAG_ENABLE;
    } else {
        tmr_val &= ~DEV_TMR_FLAG_ENABLE;
    }

    trilin_dp_write(dp, TRILIN_DPTX_HOST_TIMER, tmr_val);
}

//------------------------------------------------------------------------------
//  Function: trilin_host_tmr_interrupt
//  Enable / Disable interrupt for a specified timer
//
//  Parameters:
//      dp   - timer base address
//      enable - enable / disable interrupt
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void trilin_host_tmr_interrupt(struct trilin_dp *dp, bool enable)
{
    u32 tmr_val = trilin_dp_read(dp, TRILIN_DPTX_HOST_TIMER);

    if (enable == true) {
        tmr_val |= DEV_TMR_FLAG_INTERRUPT;
    } else {
        tmr_val &= ~DEV_TMR_FLAG_INTERRUPT;
    }

    trilin_dp_write(dp, TRILIN_DPTX_HOST_TIMER, tmr_val);
}

//------------------------------------------------------------------------------
//  Function: trilin_host_tmr_autoreload
//  Enable / Disable auto-reload for a specified timer
//
//  Parameters:
//      dp   - timer base address
//      enable - enable / disable the auto-reload
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void trilin_host_tmr_autoreload(struct trilin_dp *dp, bool enable)
{
    u32 tmr_val = trilin_dp_read(dp, TRILIN_DPTX_HOST_TIMER);

    if (enable == true) {
        tmr_val |= DEV_TMR_FLAG_AUTORELOAD;
    } else {
        tmr_val &= ~DEV_TMR_FLAG_AUTORELOAD;
    }

    trilin_dp_write(dp, TRILIN_DPTX_HOST_TIMER, tmr_val);
}

//------------------------------------------------------------------------------
//  Function: trilin_host_tmr_wait_ct
//  Set the wait timer to wait for a specified count.
//  Blocking function. Does not return until timeout.
//
//  This function will give a slightly longer timeout
//  due to register loading overhead.
//
//  Parameters:
//      dp - timer base address
//      ct   - Duration in timer ticks
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void trilin_host_tmr_wait_ct(struct trilin_dp *dp, u32 ct)
{
    trilin_host_tmr_enable(dp, false);
    trilin_host_tmr_autoreload(dp, false);
    trilin_host_tmr_set_ct(dp, ct);
    trilin_host_tmr_enable(dp, true);

    while (trilin_host_tmr_get_value(dp) != 0ul);
    trilin_host_tmr_enable(dp, false);
}

//------------------------------------------------------------------------------
//  Function: trilin_host_tmr_wait_us
//  Set the wait timer to wait for a specified number of microseconds.
//  Blocking function. Does not return until timeout.
//
//  This function will give a slightly longer timeout
//  due to register loading overhead.
//
//  Parameters:
//      dp - timer base address
//      us   - Duration in microseconds
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void trilin_host_tmr_wait_us(struct trilin_dp *dp, u32 us)
{
    trilin_host_tmr_enable(dp, false);
    trilin_host_tmr_autoreload(dp, false);
    trilin_host_tmr_set_ct(dp, us);
    trilin_host_tmr_enable(dp, true);

    while (trilin_host_tmr_get_value(dp) != 0ul);
    trilin_host_tmr_enable(dp, false);
}

//------------------------------------------------------------------------------
//  Function: trilin_host_tmr_wait_ms
//  Set the wait timer to wait for a specified number of milliseconds.
//  Blocking function. Does not return until timeout.
//
//  This function will give a slightly longer timeout
//  due to register loading overhead.
//
//  Parameters:
//      dp - timer base address
//      ms   - Duration in milliseconds
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void trilin_host_tmr_wait_ms(struct trilin_dp *dp, u32 ms)
{
    trilin_host_tmr_enable(dp, false);
    trilin_host_tmr_autoreload(dp, false);
    trilin_host_tmr_set_ct(dp, ((ms * DEV_TMR_MS)));
    trilin_host_tmr_enable(dp, true);

    while (trilin_host_tmr_get_value(dp) != 0ul);
    trilin_host_tmr_enable(dp, false);
}

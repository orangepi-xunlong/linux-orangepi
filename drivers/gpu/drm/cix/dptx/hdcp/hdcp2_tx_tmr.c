// SPDX-License-Identifier: GPL-2.0
/*
 * display port hdcp driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/types.h>
#include <linux/string.h>
#include <linux/io.h>

#include "cix_hdcp.h"

//------------------------------------------------------------------------------
//  Time Values:
//
//  Time-base       Source
//  Microseconds    Timer clock is 1MHz
//  Milliseconds    Microsecond time * 1000
//------------------------------------------------------------------------------
#define TMR_HDCP_MS              1000
//------------------------------------------------------------------------------
// Device Timer Register Format:
// bit 31   = Timer enable
// bit 30   = Auto-reload
// bit 29   = Interrupt enable
// bit 19:0 = Timer value (20-bits)
//------------------------------------------------------------------------------
#define TMR_HDCP_FLAG_ENABLE             (1ul<<31)
#define TMR_HDCP_FLAG_AUTORELOAD         (1ul<<30)
#define TMR_HDCP_FLAG_INTERRUPT          (1ul<<29)
#define TMR_HDCP_MAX_CT                  ((1ul<<20)-1ul)
#define TMR_HDCP_FLAGS_ALL               (TMR_HDCP_FLAG_ENABLE | TMR_HDCP_FLAG_AUTORELOAD | TMR_HDCP_FLAG_INTERRUPT)


//------------------------------------------------------------------------------
//  Function: tmr_hdcp_init
//  Initialize device timer
//
//  Parameters:
//      addr - timer base address
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void tmr_hdcp_init(void *addr)
{
	writel(0ul, addr);
}

//------------------------------------------------------------------------------
//  Function: tmr_hdcp_set_us - Set device timer value in microseconds
//
//  Parameters:
//      addr - timer base address
//      tmr_val - New 20-bit timer value
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void tmr_hdcp_set_us(void *addr, uint32_t tmr_us)
{
	uint32_t tmr_val = readl(addr);

	tmr_val = (tmr_val & TMR_HDCP_FLAGS_ALL) | (TMR_HDCP_MAX_CT & tmr_us);
	writel(tmr_val, addr);
}

//------------------------------------------------------------------------------
//  Function: tmr_hdcp_enable / tmr_hdcp_disable
//  Short cut functions for enable / disable
//
//  Parameters:
//      addr   - timer base address
//      enable - enable / disable the timer
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void tmr_hdcp_enable(void *addr, bool enable)
{
	uint32_t tmr_val = readl(addr);

	if (enable == true) {
		tmr_val |= TMR_HDCP_FLAG_ENABLE;
	} else {
		tmr_val &= ~TMR_HDCP_FLAG_ENABLE;
	}

	writel(tmr_val, addr);
}

//------------------------------------------------------------------------------
//  Function: tmr_hdcp_interrupt
//  Enable / Disable interrupt for a specified timer
//
//  Parameters:
//      addr   - timer base address
//      enable - enable / disable interrupt
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void tmr_hdcp_interrupt(void *addr, bool enable)
{
	uint32_t tmr_val = readl(addr);

	if (enable == true) {
		tmr_val |= TMR_HDCP_FLAG_INTERRUPT;
	} else {
		tmr_val &= ~TMR_HDCP_FLAG_INTERRUPT;
	}

	writel(tmr_val, addr);
}

//------------------------------------------------------------------------------
//  Function: tmr_hdcp_autoreload
//  Enable / Disable auto-reload for a specified timer
//
//  Parameters:
//      addr   - timer base address
//      enable - enable / disable the auto-reload
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void tmr_hdcp_autoreload(void *addr, bool enable)
{
	uint32_t tmr_val = readl(addr);

	if (enable == true) {
		tmr_val |= TMR_HDCP_FLAG_AUTORELOAD;
	} else {
		tmr_val &= ~TMR_HDCP_FLAG_AUTORELOAD;
	}

	writel(tmr_val, addr);
}

//------------------------------------------------------------------------------
//  Function: hdcp2_tx_tmr_start
//      Start HDCP timer
//
//  Parameters:
//      hndl   - HDCP2 device handle
//      evt    - event for the callback
//      ms     - time in ms
//      tmr_id - verify timer callback state is correct
//
//  Returns:
//      timer start status
//------------------------------------------------------------------------------
bool hdcp2_tx_tmr_start(struct cix_hdcp *hdcp, uint32_t ms)
{
	void *tmr_addr;

	if (hdcp->timer_in_use) {    //  test for in use
		return false;    //  exit if the timer is already in-use
	}

	tmr_addr  = hdcp->tmr_addr;

	tmr_hdcp_init(tmr_addr);    //  clear the timer
	tmr_hdcp_interrupt(tmr_addr, true);    //  enable interrupts
	tmr_hdcp_set_us(tmr_addr, ms*1000);    //  set timeout
	tmr_hdcp_enable(tmr_addr, true);    //  start timer

	hdcp->timer_in_use = true;    //  set timer in-use flag

	return true;
}

//------------------------------------------------------------------------------
//  Function: hdcp2_tx_tmr_stop
//      Stop HDCP timer
//
//  Parameters:
//      hndl   - HDCP2 device handle
//
//  Returns:
//      None
//------------------------------------------------------------------------------
void hdcp2_tx_tmr_stop(struct cix_hdcp *hdcp)
{
	void *tmr_addr  = hdcp->tmr_addr;

	tmr_hdcp_init(tmr_addr);    //  clear timer

	hdcp->timer_in_use = false;    //  clear in-use flag
}

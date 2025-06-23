// SPDX-License-Identifier: GPL-2.0
//------------------------------------------------------------------------------
//	Trilinear Technologies DisplayPort DRM Driver
//	Copyright (C) 2023~2024 Trilinear Technologies
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

#include <drm/drm_atomic_helper.h>
#include <drm/drm_device.h>
#include <drm/drm_edid.h>
#include <drm/drm_managed.h>
#include <drm/drm_modes.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>

#include "trilin_dptx_reg.h"
#include "trilin_host_tmr.h"
#include "trilin_phy.h"
#include "trilin_usbdp_phy.h"
#include "trilin_dptx.h"
//------------------------------------------------------------------------------
//	local functions
//------------------------------------------------------------------------------
static trilin_phy_error_t usbdp_phy_pll_disable (struct trilin_dp *dp);
static trilin_phy_error_t usbdp_phy_pll_enable  (struct trilin_dp *dp);

static trilin_phy_error_t usbdp_phy_cmn_ready_ack (
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t usbdp_phy_lane_pll_ack_disabled (
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t usbdp_phy_lane_pll_ack_ready (
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t usbdp_phy_pwr_state_ack (
	struct trilin_dp *dp, u32 pwr_state, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t trilin_usbdp_phy_power(
	struct trilin_dp *dp, trilin_phy_power_state_t pwr_state);

static trilin_phy_error_t trilin_usbdp_phy_set_lane_count(
	struct trilin_dp *dp, trilin_phy_lane_en_t lane_en);

static inline trilin_phy_error_t trilin_usbdp_delay(
	struct trilin_dp *dp, u32 delay_time)
{
	trilin_host_tmr_init(dp);
	trilin_host_tmr_wait_us(dp, delay_time);
	return trilin_phy_error_none;
}
//------------------------------------------------------------------------------
//  Function: trilin_usbdp_phy_prepare
//		This function prepare the Cadence PHY.
//
//	Parameters:
//
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_usbdp_phy_prepare(struct trilin_dp *dp)
{
	trilin_phy_error_t status = trilin_phy_error_none;
	struct trilin_phy_t *phy = &dp->phy;

	if (phy->state == trilin_phy_prepared)
		return status;

	// reset AUXHPD block
	// all enable bits cleared
	trilin_phy_write(dp, TRILIN_USBDP_PHY_AUXHPD_ENABLE_BITS, 0);
	// set TX lanes to electrically idle state
	trilin_phy_write(dp, TRILIN_USBDP_PHY_TX_ELEC_IDLE_BITS,  0x0f);

	// reset PHY wrapper
	// clear data enable bit
	trilin_phy_write(dp, TRILIN_USBDP_PHY_DATA_ENABLE,    0);
	// set (low) lane reset_n signals
	trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_RSTN_BITS, 0);
	// clear lane enable signals
	trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_EN_BITS,   0);
	trilin_phy_write(dp, TRILIN_USBDP_PHY_CONFIG_RSTN,    0);
	trilin_phy_write(dp, TRILIN_USBDP_PHY_DP_RESET, 0x1);
	trilin_usbdp_delay(dp, 1);

	// enable Cadence AUXHPD block
	// decap enable
	trilin_phy_write(dp, TRILIN_USBDP_PHY_AUXHPD_ENABLE_BITS, 1);
	trilin_usbdp_delay(dp, 1);
	// decap enable (delayed version)
	trilin_phy_write(dp, TRILIN_USBDP_PHY_AUXHPD_ENABLE_BITS, 3);
	trilin_usbdp_delay(dp, 1);
	// bg enable
	trilin_phy_write(dp, TRILIN_USBDP_PHY_AUXHPD_ENABLE_BITS, 7);
	trilin_usbdp_delay(dp, 5);

	// aux lines enabled for TX and RX
	trilin_phy_write(dp, TRILIN_USBDP_PHY_AUXHPD_ENABLE_BITS, 0xf);
	trilin_phy_write(dp, TRILIN_USBDP_PHY_DP_RESET, 0x0);

	phy->state = trilin_phy_prepared;

	dev_dbg(dp->dev, "%s end\n", __func__);
	return status;
}

//------------------------------------------------------------------------------
//  Function: trilin_usbdp_phy_init
//		This function initializes the Cadence PHY.
//
//	Parameters:
//	Reference clock - currently, only 24Mhz is supported
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_usbdp_phy_init(
	struct trilin_dp *dp, trilin_phy_ref_clk_t ref_clk)
{
	int ret;
	trilin_phy_error_t status = trilin_phy_error_none;
	struct trilin_phy_t *phy = &dp->phy;

	if (!IS_ERR(phy->base)) {
		ret = phy_power_on(phy->base);
		if (ret)
			dev_err(dp->dev, "failed to power phy on");
	}

	trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_RSTN_BITS,    0x000f);
	trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_EN_BITS,      0x000f);
	trilin_phy_write(dp, TRILIN_USBDP_PHY_TX_ELEC_IDLE_BITS, 0x0000);
	trilin_phy_write(dp, TRILIN_USBDP_PHY_CONFIG_RSTN,       0x0001);

	status = usbdp_phy_cmn_ready_ack(dp, 1, 500);
	trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_PLLCLK_ENABLE, 0x0001);
	status = usbdp_phy_lane_pll_ack_ready(dp, 1, 500);

	// bring the power state back up to active
	status = trilin_usbdp_phy_power(dp, trilin_power_a2);
	status = trilin_usbdp_phy_power(dp, trilin_power_a0);
	// set data enable bit
	trilin_phy_write(dp, TRILIN_USBDP_PHY_DATA_ENABLE, 1);

	phy->state = trilin_phy_power_on;
	dev_dbg(dp->dev, "%s end\n", __func__);
	return status;
}

//------------------------------------------------------------------------------
//  Function: trilin_usbdp_phy_reset
//		PHY reset function
//
//	Parameters:
//	Reset type
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_usbdp_phy_reset(
	struct trilin_dp *dp, trilin_phy_reset_t reset_type)
{
	trilin_phy_error_t status = trilin_phy_error_none;

	switch (reset_type) {
	case trilin_phy_reset_all:
		// clear data enable bit
		trilin_phy_write(dp, TRILIN_USBDP_PHY_DATA_ENABLE, 0);
		// set (low) lane reset_n signals
		trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_RSTN_BITS, 0);
		// clear lane enable signals
		trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_EN_BITS, 0);
		// requested power state reset to "no request"
		trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ, 0);
		// disable lanes' xcvr_pllclk_en and activate lanes' reset_n
		trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_PLLCLK_ENABLE, 0);
		trilin_phy_write(dp, TRILIN_USBDP_PHY_DP_RESET, 1);
		status = trilin_phy_error_none;
		break;

	case trilin_phy_reset_min:
		// clear data enable bit
		trilin_phy_write(dp, TRILIN_USBDP_PHY_DATA_ENABLE, 0);
		trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_RSTN_BITS, 0);
		trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_EN_BITS, 0);
		status = trilin_phy_error_none;
		break;

	default:
		status = trilin_phy_error_reset_type;
		break;
	}

	dev_dbg(dp->dev, "%s end\n", __func__);
	return status;
}

//------------------------------------------------------------------------------
//  Function: trilin_usbdp_phy_power
//		Selects selected power state
//
//		Notes:
//		- State A1 is not a used power state for DisplayPort.
//		- States A2 & A3 are configured identically for DisplayPort.
//		- State A3 requires special handshaking for entry/exit.
//
//	Parameters:
//	pwr_state - new selected power state
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_usbdp_phy_power(
	struct trilin_dp *dp, trilin_phy_power_state_t pwr_state)
{
	trilin_phy_error_t status = trilin_phy_error_none;

	switch (pwr_state) {
	case trilin_power_a0:
		trilin_phy_write(dp,
				TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_USBDP_PHY_PWR_STATE_A0_TXRX_ACTIVE);
		status = usbdp_phy_pwr_state_ack(dp,
				TRILIN_USBDP_PHY_PWR_STATE_A0_TXRX_ACTIVE,
				1, 100);
		trilin_phy_write(dp,
				TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_USBDP_PHY_PWR_STATE_IDLE);
		ndelay(200);
		break;

	case trilin_power_a1:
		trilin_phy_write(dp,
				TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_USBDP_PHY_PWR_STATE_A1_POWERDOWN1);
		status = usbdp_phy_pwr_state_ack(dp,
				TRILIN_USBDP_PHY_PWR_STATE_A1_POWERDOWN1,
				1, 50);
		trilin_phy_write(dp,
				TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_USBDP_PHY_PWR_STATE_IDLE);
		ndelay(200);
		break;

	case trilin_power_a2:
		trilin_phy_write(dp,
				TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_USBDP_PHY_PWR_STATE_A2_POWERDOWN2);
		status = usbdp_phy_pwr_state_ack(dp,
				TRILIN_USBDP_PHY_PWR_STATE_A2_POWERDOWN2,
				1, 50);
		trilin_phy_write(dp,
				TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_USBDP_PHY_PWR_STATE_IDLE);
		ndelay(200);
		break;

	case trilin_power_a3:
		trilin_phy_write(dp,
				TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_USBDP_PHY_PWR_STATE_A3_POWERDOWN3);
		status = usbdp_phy_pwr_state_ack(dp,
				TRILIN_USBDP_PHY_PWR_STATE_A3_POWERDOWN3,
				1, 50);
		trilin_phy_write(dp,
				TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_USBDP_PHY_PWR_STATE_IDLE);
		ndelay(200);
		break;

	default:
		status = trilin_phy_error_pwr_type;
		break;
	}
	return status;
}

static trilin_phy_error_t trilin_usbdp_phy_exit(struct trilin_dp *dp)
{
	struct trilin_phy_t *phy = &dp->phy;
	int ret;

	if (phy->state == trilin_phy_power_on) {
		if (!IS_ERR(phy->base)) {
			ret = phy_power_off(phy->base);
			if (ret)
				dev_err(dp->dev, "failed to power phy off");
		}
	}

	phy->state = trilin_phy_power_off;
	return trilin_phy_error_none;
}

static trilin_phy_error_t trilin_usbdp_phy_configure(
	struct trilin_dp *dp, union phy_configure_opts *opts)
{
	struct phy_configure_opts_dp *dp_opts = &opts->dp;
	trilin_phy_error_t status = trilin_phy_error_none;
	struct trilin_phy_t *phy = &dp->phy;
	int ret;

	/* set lane count */
	if (0 && dp_opts->set_lanes) {
		trilin_usbdp_phy_set_lane_count(dp, dp_opts->lanes);
	}

	if (!IS_ERR(phy->base)) {
		if (opts->dp.set_rate) {
			status = usbdp_phy_pll_disable(dp);
			if (status != trilin_phy_error_none) {
				dev_err(dp->dev, "usbdp_phy_pll_disable \n");
				return status;
			}

			ret = phy_configure(phy->base, opts);
			if (ret)
				dev_err(dp->dev, "failed to configure phy \n");

			status = usbdp_phy_pll_enable(dp);
			if (status != trilin_phy_error_none) {
				dev_err(dp->dev, "usbdp_phy_pll_enable \n");
				return status;
			}
		} else {
			ret = phy_configure(phy->base, opts);
			if (ret)
				dev_err(dp->dev, "failed to configure phy \n");
		}
	}

	dev_dbg(dp->dev, "%s end\n", __func__);
	return status;
}

//------------------------------------------------------------------------------
//  Function: trilin_usbdp_phy_set_lane_count
//
//	Parameters:
//	lane_ct - new lane count
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_usbdp_phy_set_lane_count(
	struct trilin_dp *dp, trilin_phy_lane_en_t lane_en)
{
	trilin_phy_error_t status = trilin_phy_error_none;

	switch (lane_en) {
	case trilin_lane_en_1:
	case trilin_lane_en_2:
	case trilin_lane_en_4:
		trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_RSTN_BITS, lane_en);
		trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_EN_BITS,   lane_en);
		break;

	default:
		status = trilin_phy_error_lane_count;
		break;
	}

	return status;
}

//----------------------------------------------------------------------------
//	Function: usbdp_phy_pll_disable
//	Disable PLL0 (PLL1 is not used)
//
// Inputs:
//		None
//
// Outputs:
//	Appropriate DPTX PHY status
//----------------------------------------------------------------------------
static trilin_phy_error_t usbdp_phy_pll_disable(struct trilin_dp *dp)
{
	// clear data enable bit
	trilin_phy_write(dp, TRILIN_USBDP_PHY_DATA_ENABLE, 0);
	// set to low power
	trilin_usbdp_phy_power(dp, trilin_power_a3);

	// disable lane pll clock
	trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_PLLCLK_ENABLE, 0);
	// wait for PLL lane ACK
	usbdp_phy_lane_pll_ack_disabled(dp, 1, 100);

	return trilin_phy_error_none;
}

//----------------------------------------------------------------------------
//	Function: usbdp_phy_pll_enable
//	Enable PLL0 (PLL1 is not used)
//
// Inputs:
//		None
//
// Outputs:
//	Appropriate DPTX PHY status
//----------------------------------------------------------------------------
static trilin_phy_error_t usbdp_phy_pll_enable(struct trilin_dp *dp)
{
	// enable lane pll clock
	trilin_phy_write(dp, TRILIN_USBDP_PHY_LANE_PLLCLK_ENABLE, 1);
	// wait for PLL lane ACK
	usbdp_phy_lane_pll_ack_ready(dp, 1, 100);

	// bring power state back to active
	trilin_usbdp_phy_power(dp, trilin_power_a2);
	trilin_usbdp_phy_power(dp, trilin_power_a0);

	// set data enable bit
	trilin_phy_write(dp, TRILIN_USBDP_PHY_DATA_ENABLE, 1);

	return trilin_phy_error_none;
}

//----------------------------------------------------------------------------
//	Function: usbdp_phy_cmn_ready_ack
//	Poll PHY common ready register.
//		Wait until lane it is ready.
//
// Inputs:
//	delay_time - poll / loop time
//	loop_ct    - poll / loop count
//
// Outputs:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t usbdp_phy_cmn_ready_ack(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_bus_not_idle;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = trilin_phy_read(dp, TRILIN_USBDP_PHY_CMN_READY);
		reg_val &= 0x01;
		if (reg_val == 0x01) {
			status = trilin_phy_error_none;
			break;
		} else {
			trilin_host_tmr_init(dp);
			trilin_host_tmr_wait_us(dp, delay_time);
		}
	}

	if (idx >= loop_ct)
		dev_err(dp->dev, "%s timeout! loop_cn = %d \n", __func__, idx);

	return status;
}

//------------------------------------------------------------------------------
//	Function: usbdp_phy_lane_pll_ack
//	Poll lane PLL clock enable ACK register
//		Wait until lane PLL ACK is ready.
//
// Inputs:
//	delay_time - poll / loop time
//	loop_ct    - poll / loop count
//
// Outputs:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t usbdp_phy_lane_pll_ack_disabled(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_bus_not_idle;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = trilin_phy_read(dp, TRILIN_USBDP_PHY_LANE_PLLCLK_EN_ACK);
		reg_val &= 0x01;
		if (reg_val == 0x0) {
			status = trilin_phy_error_none;
			break;
		} else {
			trilin_host_tmr_init(dp);
			trilin_host_tmr_wait_us(dp, delay_time);
		}
	}

	if (idx >= loop_ct)
		dev_err(dp->dev, "%s timeout! loop_cn = %d \n", __func__, idx);

	return status;
}

static trilin_phy_error_t usbdp_phy_lane_pll_ack_ready(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_bus_not_idle;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = trilin_phy_read(dp, TRILIN_USBDP_PHY_LANE_PLLCLK_EN_ACK);
		reg_val &= 0x01;
		if (reg_val == 0x1) {
			status = trilin_phy_error_none;
			break;
		} else {
			trilin_host_tmr_init(dp);
			trilin_host_tmr_wait_us(dp, delay_time);
		}
	}

	if (idx >= loop_ct)
		dev_err(dp->dev, "%s timeout! loop_cn = %d \n", __func__, idx);

	return status;
}

//------------------------------------------------------------------------------
//	Function: usbdp_phy_pwr_state_ack
//	Poll power state ack register.
//		Wait until power state has been acquired.
//
// Inputs:
//		pwr_state  - New power state
//	delay_time - poll / loop time
//	loop_ct    - poll / loop count
//
// Outputs:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t usbdp_phy_pwr_state_ack(
	struct trilin_dp *dp, u32 pwr_state, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_pwr_ack;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = trilin_phy_read(dp, TRILIN_USBDP_PHY_LANE_PWR_STATE_ACK);
		reg_val &= 0x1f;
		if (reg_val == pwr_state) {
			status = trilin_phy_error_none;
			break;
		} else {
			trilin_host_tmr_init(dp);
			trilin_host_tmr_wait_us(dp, delay_time);
		}
	}

	if (idx >= loop_ct)
		dev_err(dp->dev, "%s timeout! loop_cn = %d \n", __func__, idx);

	return status;
}

static struct trilin_phy_ops usbdp_phy_ops = {
	.prepare   = trilin_usbdp_phy_prepare,
	.init      = trilin_usbdp_phy_init,
	.reset     = trilin_usbdp_phy_reset,
	.power     = trilin_usbdp_phy_power,
	.configure = trilin_usbdp_phy_configure,
	.exit      = trilin_usbdp_phy_exit,
};

trilin_phy_error_t trilin_usbdp_phy_register(struct trilin_dp *dp)
{
	struct trilin_phy_t *phy = &dp->phy;
	u32 reg_val;

	reg_val = (trilin_phy_read(dp, TRILIN_PHY_ID_REV_REG) >> 16);
	if (reg_val == TRILIN_USBDP_PHY_ID)
		phy->phy_ops = &usbdp_phy_ops;

	return trilin_phy_error_none;
}

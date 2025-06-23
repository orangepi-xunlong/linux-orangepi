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
#include <linux/regmap.h>

#include "trilin_dptx_reg.h"
#include "trilin_host_tmr.h"
#include "trilin_phy.h"
#include "trilin_dptx.h"
#include "trilin_edp_phy.h"


//------------------------------------------------------------------------------
//	local typedefs
//------------------------------------------------------------------------------
typedef struct {
	u32 hsclk_div;
	u32 clk_sel;
	u32 pll_intdiv;
	u32 pll_fracdivl;
	u32 pll_high_thr;
} edp_phy_link_cfg_t;

//------------------------------------------------------------------------------
//	local data
//------------------------------------------------------------------------------

/* 5.4 Gbps by default */
static const struct reg_default edp_reg_conf[] = {
	{kCMN_SSM_BIAS_TMR                          , 0x0018},
	{kCMN_PLLSM0_PLLPRE_TMR                     , 0x0030},
	{kCMN_PLLSM0_PLLLOCK_TMR                    , 0x00f0},
	{kCMN_PLLSM1_PLLPRE_TMR                     , 0x0030},
	{kCMN_PLLSM1_PLLLOCK_TMR                    , 0x00f0},
	{kCMN_BGCAL_INIT_TMR                        , 0x0078},
	{kCMN_BGCAL_ITER_TMR                        , 0x0078},
	{kCMN_IBCAL_INIT_TMR                        , 0x0018},
	{kCMN_TXPUCAL_INIT_TMR                      , 0x001d},
	{kCMN_TXPDCAL_INIT_TMR                      , 0x001d},
	{kCMN_RXCAL_INIT_TMR                        , 0x02d0},
	{kCMN_SD_CAL_PLLCNT_START                   , 0x0137},
	{kRX_SDCAL0_INIT_TMR + kLANE0_OFFSET        , 0x0018},
	{kRX_SDCAL0_INIT_TMR + kLANE1_OFFSET        , 0x0018},
	{kRX_SDCAL0_INIT_TMR + kLANE2_OFFSET        , 0x0018},
	{kRX_SDCAL0_INIT_TMR + kLANE3_OFFSET        , 0x0018},
	{kRX_SDCAL0_ITER_TMR + kLANE0_OFFSET        , 0x0078},
	{kRX_SDCAL0_ITER_TMR + kLANE1_OFFSET        , 0x0078},
	{kRX_SDCAL0_ITER_TMR + kLANE2_OFFSET        , 0x0078},
	{kRX_SDCAL0_ITER_TMR + kLANE3_OFFSET        , 0x0078},
	{kRX_SDCAL1_INIT_TMR + kLANE0_OFFSET        , 0x0018},
	{kRX_SDCAL1_INIT_TMR + kLANE1_OFFSET        , 0x0018},
	{kRX_SDCAL1_INIT_TMR + kLANE2_OFFSET        , 0x0018},
	{kRX_SDCAL1_INIT_TMR + kLANE3_OFFSET        , 0x0018},
	{kRX_SDCAL1_ITER_TMR + kLANE0_OFFSET        , 0x0078},
	{kRX_SDCAL1_ITER_TMR + kLANE1_OFFSET        , 0x0078},
	{kRX_SDCAL1_ITER_TMR + kLANE2_OFFSET        , 0x0078},
	{kRX_SDCAL1_ITER_TMR + kLANE3_OFFSET        , 0x0078},
	{kRX_ST_TMR + kLANE0_OFFSET                 , 0x0960},
	{kRX_ST_TMR + kLANE1_OFFSET                 , 0x0960},
	{kRX_ST_TMR + kLANE2_OFFSET                 , 0x0960},
	{kRX_ST_TMR + kLANE3_OFFSET                 , 0x0960},
	{kPHY_PLL_CFG                               , 0x0000},
	{kXCVR_DIAG_HSCLK_DIV + kLANE0_OFFSET       , 0x0000},
	{kXCVR_DIAG_HSCLK_DIV + kLANE1_OFFSET       , 0x0000},
	{kXCVR_DIAG_HSCLK_DIV + kLANE2_OFFSET       , 0x0000},
	{kXCVR_DIAG_HSCLK_DIV + kLANE3_OFFSET       , 0x0000},
	{kXCVR_DIAG_PLLDRC_CTRL + kLANE0_OFFSET     , 0x0011},
	{kXCVR_DIAG_PLLDRC_CTRL + kLANE1_OFFSET     , 0x0011},
	{kXCVR_DIAG_PLLDRC_CTRL + kLANE2_OFFSET     , 0x0011},
	{kXCVR_DIAG_PLLDRC_CTRL + kLANE3_OFFSET     , 0x0011},
	{kCMN_PLL0_DSM_DIAG_M0                      , 0x8004},
	{kCMN_PLL1_DSM_DIAG_M0                      , 0x0004},
	{kCMN_PDIAG_PLL0_CP_PADJ_M0                 , 0x0b17},
	{kCMN_PDIAG_PLL1_CP_PADJ_M0                 , 0x0b17},
	{kCMN_PDIAG_PLL0_CP_IADJ_M0                 , 0x0e01},
	{kCMN_PDIAG_PLL1_CP_IADJ_M0                 , 0x0e01},
	{kCMN_PDIAG_PLL0_FILT_PADJ_M0               , 0x0d05},
	{kCMN_PDIAG_PLL1_FILT_PADJ_M0               , 0x0d05},
	{kCMN_PLL0_INTDIV_M0                        , 0x01c2},
	{kCMN_PLL1_INTDIV_M0                        , 0x014d},
	{kCMN_PLL1_FRACDIVL_M0                      , 0x5555},
	{kCMN_PLL0_FRACDIVH_M0                      , 0x0002},
	{kCMN_PLL1_FRACDIVH_M0                      , 0x0002},
	{kCMN_PLL0_HIGH_THR_M0                      , 0x012c},
	{kCMN_PLL1_HIGH_THR_M0                      , 0x00e0},
	{kCMN_PDIAG_PLL0_CTRL_M0                    , 0x1002},
	{kCMN_PDIAG_PLL1_CTRL_M0                    , 0x1002},
	{kCMN_PLL0_SS_CTRL1_M0                      , 0x0001},
	{kCMN_PLL1_SS_CTRL1_M0                      , 0x0001},
	{kCMN_PLL0_SS_CTRL2_M0                      , 0x044f},
	{kCMN_PLL1_SS_CTRL2_M0                      , 0x03c7},
	{kCMN_PLL0_SS_CTRL3_M0                      , 0x007f},
	{kCMN_PLL1_SS_CTRL3_M0                      , 0x007f},
	{kCMN_PLL0_SS_CTRL4_M0                      , 0x0003},
	{kCMN_PLL1_SS_CTRL4_M0                      , 0x0003},
	{kCMN_PLL0_VCOCAL_INIT_TMR                  , 0x00f0},
	{kCMN_PLL1_VCOCAL_INIT_TMR                  , 0x00f0},
	{kCMN_PLL0_VCOCAL_ITER_TMR                  , 0x0004},
	{kCMN_PLL1_VCOCAL_ITER_TMR                  , 0x0004},
	{kCMN_PLL0_VCOCAL_REFTIM_START              , 0x02f8},
	{kCMN_PLL1_VCOCAL_REFTIM_START              , 0x02f8},
	{kCMN_PLL0_VCOCAL_PLLCNT_START              , 0x02f6},
	{kCMN_PLL1_VCOCAL_PLLCNT_START              , 0x02f6},
	{kCMN_PLL0_VCOCAL_TCTRL                     , 0x0003},
	{kCMN_PLL1_VCOCAL_TCTRL                     , 0x0003},
	{kCMN_PLL0_LOCK_REFCNT_START                , 0x00bf},
	{kCMN_PLL1_LOCK_REFCNT_START                , 0x00bf},
	{kCMN_PLL0_LOCK_PLLCNT_START                , 0x00bf},
	{kCMN_PLL1_LOCK_PLLCNT_START                , 0x00bf},
	{kCMN_PLL0_LOCK_PLLCNT_THR                  , 0x0005},
	{kCMN_PLL1_LOCK_PLLCNT_THR                  , 0x0005},
	{kTX_PSC_A0 + kLANE0_OFFSET                 , 0x00fb},
	{kTX_PSC_A0 + kLANE1_OFFSET                 , 0x00fb},
	{kTX_PSC_A0 + kLANE2_OFFSET                 , 0x00fb},
	{kTX_PSC_A0 + kLANE3_OFFSET                 , 0x00fb},
	{kTX_PSC_A2 + kLANE0_OFFSET                 , 0x04aa},
	{kTX_PSC_A2 + kLANE1_OFFSET                 , 0x04aa},
	{kTX_PSC_A2 + kLANE2_OFFSET                 , 0x04aa},
	{kTX_PSC_A2 + kLANE3_OFFSET                 , 0x04aa},
	{kTX_PSC_A3 + kLANE0_OFFSET                 , 0x04aa},
	{kTX_PSC_A3 + kLANE1_OFFSET                 , 0x04aa},
	{kTX_PSC_A3 + kLANE2_OFFSET                 , 0x04aa},
	{kTX_PSC_A3 + kLANE3_OFFSET                 , 0x04aa},
	{kRX_PSC_A0 + kLANE0_OFFSET                 , 0x0000},
	{kRX_PSC_A0 + kLANE1_OFFSET                 , 0x0000},
	{kRX_PSC_A0 + kLANE2_OFFSET                 , 0x0000},
	{kRX_PSC_A0 + kLANE3_OFFSET                 , 0x0000},
	{kRX_PSC_A2 + kLANE0_OFFSET                 , 0x0000},
	{kRX_PSC_A2 + kLANE1_OFFSET                 , 0x0000},
	{kRX_PSC_A2 + kLANE2_OFFSET                 , 0x0000},
	{kRX_PSC_A2 + kLANE3_OFFSET                 , 0x0000},
	{kRX_PSC_A3 + kLANE0_OFFSET                 , 0x0000},
	{kRX_PSC_A3 + kLANE1_OFFSET                 , 0x0000},
	{kRX_PSC_A3 + kLANE2_OFFSET                 , 0x0000},
	{kRX_PSC_A3 + kLANE3_OFFSET                 , 0x0000},
	{kRX_PSC_CAL + kLANE0_OFFSET                , 0x0000},
	{kRX_PSC_CAL + kLANE1_OFFSET                , 0x0000},
	{kRX_PSC_CAL + kLANE2_OFFSET                , 0x0000},
	{kRX_PSC_CAL + kLANE3_OFFSET                , 0x0000},
	{kXCVR_DIAG_BIDI_CTRL + kLANE0_OFFSET       , 0x000f},
	{kXCVR_DIAG_BIDI_CTRL + kLANE1_OFFSET       , 0x000f},
	{kXCVR_DIAG_BIDI_CTRL + kLANE2_OFFSET       , 0x000f},
	{kXCVR_DIAG_BIDI_CTRL + kLANE3_OFFSET       , 0x000f},
	{kRX_REE_GCSM1_CTRL + kLANE0_OFFSET         , 0x0000},
	{kRX_REE_GCSM1_CTRL + kLANE1_OFFSET         , 0x0000},
	{kRX_REE_GCSM1_CTRL + kLANE2_OFFSET         , 0x0000},
	{kRX_REE_GCSM1_CTRL + kLANE3_OFFSET         , 0x0000},
	{kRX_REE_GCSM2_CTRL + kLANE0_OFFSET         , 0x0000},
	{kRX_REE_GCSM2_CTRL + kLANE1_OFFSET         , 0x0000},
	{kRX_REE_GCSM2_CTRL + kLANE2_OFFSET         , 0x0000},
	{kRX_REE_GCSM2_CTRL + kLANE3_OFFSET         , 0x0000},
	{kRX_REE_PERGCSM_CTRL + kLANE0_OFFSET       , 0x0000},
	{kRX_REE_PERGCSM_CTRL + kLANE1_OFFSET       , 0x0000},
	{kRX_REE_PERGCSM_CTRL + kLANE2_OFFSET       , 0x0000},
	{kRX_REE_PERGCSM_CTRL + kLANE3_OFFSET       , 0x0000},
};

static const edp_phy_link_cfg_t link_cfg[] = {
	{ 0x0002, 0x0f01, 0x0195, 0x0000, 0x010e }, // 1.62gbps
	{ 0x0001, 0x0701, 0x0195, 0x0000, 0x010e }, // 2.43gpbs
	{ 0x0001, 0x0701, 0x01c2, 0x0000, 0x012c }, // 2.70gbps
	{ 0x0002, 0x0b00, 0x0195, 0x0000, 0x010e }, // 3.24gbps
	{ 0x0000, 0x0301, 0x01c2, 0x0000, 0x012c }, // 5.40gbps
	{ 0x0000, 0x0200, 0x0151, 0x8000, 0x00e2 }, // 8.10gbps
};

//------------------------------------------------------------------------------
//	local functions
//------------------------------------------------------------------------------
static trilin_phy_error_t edp_pll_disable(struct trilin_dp *dp);
static trilin_phy_error_t edp_pll_enable(struct trilin_dp *dp);

static trilin_phy_error_t edp_common_ready_ack(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t edp_pll_raw_ctrl_ack_disabled(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t edp_pll_raw_ctrl_ack_ready(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t edp_lane_pll_ack_disabled(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t edp_lane_pll_ack_ready(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t edp_pwr_state_ack(
	struct trilin_dp *dp, u32 pwr_state, u32 delay_time, u32 loop_ct);

static trilin_phy_error_t edp_phy_cdb_ready(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct);

static u32                edp_phy_cdb_read(
	struct trilin_dp *dp, u32 offset);

static trilin_phy_error_t edp_phy_cdb_write(
	struct trilin_dp *dp, u32 offset, uint16_t data);

static trilin_phy_error_t trilin_edp_phy_power(
	struct trilin_dp *dp, trilin_phy_power_state_t pwr_state);

static trilin_phy_error_t trilin_edp_phy_set_link_rate(
	struct trilin_dp *dp, u32 link_rate);

static trilin_phy_error_t trilin_edp_phy_set_lane_count(
	struct trilin_dp *dp, trilin_phy_lane_en_t lane_en);

static trilin_phy_error_t trilin_edp_phy_set_voltages(
	struct trilin_dp *dp, u32 lane_num, u32 vs, u32 pe);

static inline trilin_phy_error_t trilin_edp_delay(
	struct trilin_dp *dp, u32 delay_time)
{
	trilin_host_tmr_init(dp);
	trilin_host_tmr_wait_us(dp, delay_time);
	return trilin_phy_error_none;
}

//------------------------------------------------------------------------------
//  Function: trilin_edp_phy_prepare
//		This function prepare the Cadence PHY.
//
//	Parameters:
//
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_edp_phy_prepare(struct trilin_dp *dp)
{
	trilin_phy_error_t status = trilin_phy_error_none;
	struct trilin_phy_t *phy = &dp->phy;

	if (phy->state == trilin_phy_prepared)
		return status;
	// enable PHY's local configuration bus
	trilin_phy_write(dp, TRILIN_EDP_CDB_RSTN, 1);

	// reset AUXHPD block
	// all enable bits cleared
	trilin_phy_write(dp, TRILIN_EDP_AUXHPD_ENABLE_BITS, 0);
	// set TX lanes to electrically idle state
	trilin_phy_write(dp, TRILIN_EDP_TX_ELEC_IDLE_BITS,  0xf);

	// reset PHY
	// clear data enable bit
	trilin_phy_write(dp, TRILIN_EDP_PHY_DATA_ENABLE,    0);
	// set (low) lane reset_n signals
	trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_RSTN_BITS, 0);
	// clear lane enable signals
	trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_EN_BITS,   0);

	// set (low) the PHY wrapper's "global" reset_n signal
	trilin_phy_write(dp, TRILIN_EDP_PHY_CONFIG_RSTN,    0);
	trilin_edp_delay(dp, 1);

	// enable Cadence AUXHPD block
	// decap enable
	trilin_phy_write(dp, TRILIN_EDP_AUXHPD_ENABLE_BITS, 1);
	trilin_edp_delay(dp, 1);
	// decap enable (delayed version)
	trilin_phy_write(dp, TRILIN_EDP_AUXHPD_ENABLE_BITS, 3);
	trilin_edp_delay(dp, 1);
	// bg enable
	trilin_phy_write(dp, TRILIN_EDP_AUXHPD_ENABLE_BITS, 7);
	trilin_edp_delay(dp, 5);

	// aux lines enabled for TX and RX
	trilin_phy_write(dp, TRILIN_EDP_AUXHPD_ENABLE_BITS, 0xf);

	phy->state = trilin_phy_prepared;

	dev_dbg(dp->dev, "%s end\n", __func__);
	return trilin_phy_error_none;
}

//------------------------------------------------------------------------------
//  Function: trilin_phy_init
//		This function initializes the Cadence PHY.
//
//	Parameters:
//	Reference clock - currently, only 24Mhz is supported
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_edp_phy_init(
	struct trilin_dp *dp, trilin_phy_ref_clk_t ref_clk)
{
	int idx;
	struct trilin_phy_t *phy = &dp->phy;
	trilin_phy_error_t status = trilin_phy_error_none;
	const struct reg_default *phy_reg = &edp_reg_conf[0];

	for (idx = 0; idx < ARRAY_SIZE(edp_reg_conf); idx++) {
		edp_phy_cdb_write(dp, phy_reg->reg, phy_reg->def);
		phy_reg++;
	}

	// release forced reset of PLL0 (leave PLL1 off)
	edp_phy_cdb_write(dp, kPHY_PMA_PLL_RAW_CTRL, PMA_PLL_RAW_CTRL_PLL0_FORCE_RSTN);

	// enable all four lanes
	trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_RSTN_BITS,     0x000f);
	trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_EN_BITS,       0x000f);

	trilin_phy_write(dp, TRILIN_EDP_TX_ELEC_IDLE_BITS,      0x0000);
	// release the PHY global configuration-sourced reset (active low)
	trilin_phy_write(dp, TRILIN_EDP_PHY_CONFIG_RSTN,        0x0001);
	status = edp_common_ready_ack(dp, 1, 500);

	trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_PLLCLK_ENABLE, 0x0001);
	status = edp_lane_pll_ack_ready(dp, 1, 500);

	// bring the power state back up to active
	status = trilin_edp_phy_power(dp, trilin_power_a2);
	status = trilin_edp_phy_power(dp, trilin_power_a0);
	trilin_phy_write(dp, TRILIN_EDP_PHY_DATA_ENABLE, 1);

	phy->state = trilin_phy_power_on;
	dev_dbg(dp->dev, "%s end\n", __func__);
	return status;
}

//------------------------------------------------------------------------------
//  Function: trilin_phy_reset
//		PHY reset function
//
//	Parameters:
//	Reset type
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_edp_phy_reset(
	struct trilin_dp *dp, trilin_phy_reset_t reset_type)
{
	trilin_phy_error_t status = trilin_phy_error_none;

	switch (reset_type) {
	case trilin_phy_reset_all:
		// clear data enable bit
		trilin_phy_write(dp, TRILIN_EDP_PHY_DATA_ENABLE, 0);
		// set (low) lane reset_n signals
		trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_RSTN_BITS, 0);
		// clear lane enable signals
		trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_EN_BITS, 0);
		// set (low) the PHY wrapper's "global" reset_n signal
		trilin_phy_write(dp, TRILIN_EDP_PHY_CONFIG_RSTN, 0);
		status = trilin_phy_error_none;
		break;

	case trilin_phy_reset_min:
		// requested power state reset to "no request"
		trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_PWR_STATE_REQ, 0);
		// disable link lanes' xcvr_pllclk_en and activate lanes' reset_n
		trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_PLLCLK_ENABLE, 0);
		trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_RSTN_BITS, 0);
		trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_EN_BITS, 0);

		status = trilin_phy_error_none;
		break;

	default:
		status = trilin_phy_error_reset_type;
		break;
	}

	return status;
}

//------------------------------------------------------------------------------
//  Function: trilin_phy_power
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
static trilin_phy_error_t trilin_edp_phy_power(
	struct trilin_dp *dp, trilin_phy_power_state_t pwr_state)
{
	trilin_phy_error_t status = trilin_phy_error_none;

	switch (pwr_state) {
	case trilin_power_a0:
		trilin_phy_write(dp,
				TRILIN_EDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_PHY_PWR_STATE_A0_TXRX_ACTIVE);
		status = edp_pwr_state_ack(dp,
				TRILIN_PHY_PWR_STATE_A0_TXRX_ACTIVE,
				1, 50);
		trilin_phy_write(dp,
				TRILIN_EDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_PHY_PWR_STATE_IDLE);
		break;

	case trilin_power_a1:
		trilin_phy_write(dp,
				TRILIN_EDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_PHY_PWR_STATE_A1_POWERDOWN1);
		status = edp_pwr_state_ack(dp,
				TRILIN_PHY_PWR_STATE_A1_POWERDOWN1,
				1, 50);
		trilin_phy_write(dp,
				TRILIN_EDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_PHY_PWR_STATE_IDLE);
		break;

	case trilin_power_a2:
		trilin_phy_write(dp,
				TRILIN_EDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_PHY_PWR_STATE_A2_POWERDOWN2);
		status = edp_pwr_state_ack(dp,
				TRILIN_PHY_PWR_STATE_A2_POWERDOWN2,
				1, 50);
		trilin_phy_write(dp,
				TRILIN_EDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_PHY_PWR_STATE_IDLE);
		break;

	case trilin_power_a3:
		trilin_phy_write(dp,
				TRILIN_EDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_PHY_PWR_STATE_A3_POWERDOWN3);
		status = edp_pwr_state_ack(dp,
				TRILIN_PHY_PWR_STATE_A3_POWERDOWN3,
				1, 50);
		trilin_phy_write(dp,
				TRILIN_EDP_PHY_LANE_PWR_STATE_REQ,
				TRILIN_PHY_PWR_STATE_IDLE);

		break;

	default:
		status = trilin_phy_error_pwr_type;
		break;
	}
	return status;
}

static trilin_phy_error_t trilin_edp_phy_exit(struct trilin_dp *dp)
{
	struct trilin_phy_t *phy = &dp->phy;

	phy->state = trilin_phy_power_off;
	return trilin_phy_error_none;
}

static trilin_phy_error_t trilin_edp_phy_configure(
	struct trilin_dp *dp, union phy_configure_opts *opts)
{
	struct phy_configure_opts_dp *dp_opts = &opts->dp;

	/* set link rate */
	if (dp_opts->set_rate) {
		dev_info(dp->dev, "%s %x\n", __func__, dp_opts->link_rate);
		trilin_edp_phy_set_link_rate(dp, dp_opts->link_rate);
	}

	/* set lane count */
	if (0 && dp_opts->set_lanes) {
		trilin_edp_phy_set_lane_count(dp, dp_opts->lanes);
	}

	/* set voltage: swing and pre-emphasis */
	if (dp_opts->set_voltages) {
		dev_dbg(dp->dev, "set_voltages: lane %d, vs%d, pe %d\n",
			dp_opts->lanes, dp_opts->voltage[0], dp_opts->pre[0]);
		trilin_edp_phy_set_voltages(dp,
			dp_opts->lanes, dp_opts->voltage[0], dp_opts->pre[0]);
	}

	dev_dbg(dp->dev, "%s end\n", __func__);
	return trilin_phy_error_none;
}

//------------------------------------------------------------------------------
//  Function: trilin_edp_phy_set_lane_count
//
//	Parameters:
//	lane_ct - new lane count
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_edp_phy_set_lane_count(
	struct trilin_dp *dp, trilin_phy_lane_en_t lane_en)
{
	trilin_phy_error_t status = trilin_phy_error_none;
	struct trilin_phy_t *phy = &dp->phy;

	return trilin_phy_error_none;
	status = edp_pll_disable(dp);
	if (status != trilin_phy_error_none) {
		return status;
	}

	phy->lane_en = lane_en;
	switch (lane_en) {
	case trilin_lane_en_1:
	case trilin_lane_en_2:
	case trilin_lane_en_4:
		trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_RSTN_BITS, lane_en);
		trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_EN_BITS,   lane_en);
		break;

	default:
		status = trilin_phy_error_lane_count;
		break;
	}

	status = edp_pll_enable(dp);

	dev_info(dp->dev, "%s lane_count %d\n", __func__, lane_en);
	return status;
}

//------------------------------------------------------------------------------
//  Function: trilin_edp_phy_set_link_rate
//
//	Parameters:
//	link_rate - new link rate
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t trilin_edp_phy_set_link_rate(
	struct trilin_dp *dp, u32 link_rate)
{
	const edp_phy_link_cfg_t* cfg = NULL;
	trilin_phy_error_t status = trilin_phy_error_none;

	status = edp_pll_disable(dp);
	if (status != trilin_phy_error_none) {
		dev_info(dp->dev, "%s 0 %d \n", __func__, link_rate);
		return status;
	}

	switch (link_rate) {
	case 0x6:
		cfg = &link_cfg[0];
		break;

	case 0x9:
		cfg = &link_cfg[1];
		break;

	case 0xa:
		cfg = &link_cfg[2];
		break;

	case 0xc:
		cfg = &link_cfg[3];
		break;

	case 0x14:
		cfg = &link_cfg[4];
		break;

	case 0x1e:
		cfg = &link_cfg[5];
		break;

	default:
		status = trilin_phy_error_link_rate;
		break;
	}

	if (cfg != NULL) {
		// reprogram link-speed
		edp_phy_cdb_write(dp, kXCVR_DIAG_HSCLK_DIV + kLANE0_OFFSET, cfg->hsclk_div);
		edp_phy_cdb_write(dp, kXCVR_DIAG_HSCLK_DIV + kLANE1_OFFSET, cfg->hsclk_div);
		edp_phy_cdb_write(dp, kXCVR_DIAG_HSCLK_DIV + kLANE2_OFFSET, cfg->hsclk_div);
		edp_phy_cdb_write(dp, kXCVR_DIAG_HSCLK_DIV + kLANE3_OFFSET, cfg->hsclk_div);

		edp_phy_cdb_write(dp, kCMN_PLL0_INTDIV_M0,        cfg->pll_intdiv);
		edp_phy_cdb_write(dp, kCMN_PLL0_FRACDIVL_M0,      cfg->pll_fracdivl);
		edp_phy_cdb_write(dp, kCMN_PLL0_HIGH_THR_M0,      cfg->pll_high_thr);
		edp_phy_cdb_write(dp, kCMN_PDIAG_PLL0_CLK_SEL_M0, cfg->clk_sel);
	}

	status = edp_pll_enable(dp);
	if (status != trilin_phy_error_none) {
		dev_err(dp->dev, "%s: rate %x failed\n", __func__, link_rate);
		return status;
	}

	return status;
}

//------------------------------------------------------------------------------
//  Function: trilin_phy_set_vswing
//
//	Parameters:
//	None
//
//  Returns:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------

typedef struct {
	u32 tx_txcc_ctrl;
	u32 drv_diag_tx_drv;
	u32 tx_txcc_mgnfc_mult_000;
	u32 tx_txcc_cpost_mult_00;
} phy_voltage_conf_t;

static const phy_voltage_conf_t dp_volt_cfg[] = {
	{ 0x08A4, 0x0003, 0x002a, 0x0000 }, // swing level == 0
	{ 0x08A4, 0x0003, 0x001f, 0x0014 },
	{ 0x08A4, 0x0003, 0x0013, 0x0020 },
	{ 0x08A4, 0x0003, 0x000a, 0x002a },
	{ 0x08A4, 0x0003, 0x001f, 0x0000 }, // swing level == 1
	{ 0x08A4, 0x0003, 0x0013, 0x0012 },
	{ 0x08A4, 0x0003, 0x0000, 0x001f },
	{ 0x08A4, 0x0003, 0x0000, 0x001f },
	{ 0x08A4, 0x0003, 0x0013, 0x0000 }, // swing level == 2
	{ 0x08A4, 0x0003, 0x0000, 0x0013 },
	{ 0x08A4, 0x0003, 0x0000, 0x0013 },
	{ 0x08A4, 0x0003, 0x0000, 0x0013 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 }, // swing level == 3
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
};

static const phy_voltage_conf_t edp_volt_high_cfg[] = {
	{ 0x08A4, 0x0003, 0x0027, 0x0000 }, // swing level == 0
	{ 0x08A4, 0x0003, 0x0023, 0x0009 },
	{ 0x08A4, 0x0003, 0x001E, 0x0010 },
	{ 0x08A4, 0x0003, 0x001A, 0x0015 },
	{ 0x08A4, 0x0003, 0x0023, 0x0000 }, // swing level == 1
	{ 0x08A4, 0x0003, 0x001E, 0x0008 },
	{ 0x08A4, 0x0003, 0x001A, 0x000E },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x001E, 0x0000 }, // swing level == 2
	{ 0x08A4, 0x0003, 0x001A, 0x0007 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x001A, 0x0000 }, // swing level == 3
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
};

static const phy_voltage_conf_t edp_volt_low_cfg[] = {
	{ 0x08A4, 0x0003, 0x002F, 0x0000 }, // swing level == 0
	{ 0x08A4, 0x0003, 0x002A, 0x000D },
	{ 0x08A4, 0x0003, 0x0026, 0x0015 },
	{ 0x08A4, 0x0003, 0x0022, 0x001B },
	{ 0x08A4, 0x0003, 0x002B, 0x0000 }, // swing level == 1
	{ 0x08A4, 0x0003, 0x0027, 0x000B },
	{ 0x08A4, 0x0003, 0x0023, 0x0012 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0027, 0x0000 }, // swing level == 2
	{ 0x08A4, 0x0003, 0x0023, 0x0009 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0023, 0x0000 }, // swing level == 3
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
	{ 0x08A4, 0x0003, 0x0000, 0x0000 },
};

static trilin_phy_error_t trilin_edp_phy_set_voltages(
	struct trilin_dp *dp, u32 lane_num, u32 vs, u32 pe)
{
	struct trilin_phy_t *phy = &dp->phy;
	const phy_voltage_conf_t *cfg;
	u32 value, index;

	index = vs * 4 + pe;
	if (phy->is_edp) {
		cfg = &edp_volt_low_cfg[index];
	} else {
		cfg = &dp_volt_cfg[index];
	}

	switch (lane_num) {
	case trilin_lane_en_1:
		value = edp_phy_cdb_read(dp, kTX_DIAG_ACYA + kLANE2_OFFSET);

		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE2_OFFSET,
				value | BIT(0));
		edp_phy_cdb_write(dp, kTX_TXCC_CTRL + kLANE2_OFFSET,
					cfg->tx_txcc_ctrl);
		edp_phy_cdb_write(dp, kDRV_DIAG_TX_DRV + kLANE2_OFFSET,
					cfg->drv_diag_tx_drv);
		edp_phy_cdb_write(dp, kTX_TXCC_MGNFS_MULT_000 + kLANE2_OFFSET,
					cfg->tx_txcc_mgnfc_mult_000);
		edp_phy_cdb_write(dp, kTX_TXCC_CPOST_MULT_00 + kLANE2_OFFSET,
					cfg->tx_txcc_cpost_mult_00);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE2_OFFSET,
					value & ~BIT(0));
		break;

	case trilin_lane_en_2:
		value = edp_phy_cdb_read(dp, kTX_DIAG_ACYA + kLANE2_OFFSET);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE2_OFFSET,
				value | BIT(0));
		edp_phy_cdb_write(dp, kTX_TXCC_CTRL + kLANE2_OFFSET,
					cfg->tx_txcc_ctrl);
		edp_phy_cdb_write(dp, kDRV_DIAG_TX_DRV + kLANE2_OFFSET,
					cfg->drv_diag_tx_drv);
		edp_phy_cdb_write(dp, kTX_TXCC_MGNFS_MULT_000 + kLANE2_OFFSET,
					cfg->tx_txcc_mgnfc_mult_000);
		edp_phy_cdb_write(dp, kTX_TXCC_CPOST_MULT_00 + kLANE2_OFFSET,
					cfg->tx_txcc_cpost_mult_00);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE2_OFFSET,
					value & ~BIT(0));

		value = edp_phy_cdb_read(dp, kTX_DIAG_ACYA + kLANE3_OFFSET);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE3_OFFSET,
				value | BIT(0));
		edp_phy_cdb_write(dp, kTX_TXCC_CTRL + kLANE3_OFFSET,
					cfg->tx_txcc_ctrl);
		edp_phy_cdb_write(dp, kDRV_DIAG_TX_DRV + kLANE3_OFFSET,
					cfg->drv_diag_tx_drv);
		edp_phy_cdb_write(dp, kTX_TXCC_MGNFS_MULT_000 + kLANE3_OFFSET,
					cfg->tx_txcc_mgnfc_mult_000);
		edp_phy_cdb_write(dp, kTX_TXCC_CPOST_MULT_00 + kLANE3_OFFSET,
					cfg->tx_txcc_cpost_mult_00);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE3_OFFSET,
					value & ~BIT(0));
		break;

	case trilin_lane_en_4:
	default:
		value = edp_phy_cdb_read(dp, kTX_DIAG_ACYA + kLANE0_OFFSET);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE0_OFFSET,
				value | BIT(0));
		edp_phy_cdb_write(dp, kTX_TXCC_CTRL + kLANE0_OFFSET,
					cfg->tx_txcc_ctrl);
		edp_phy_cdb_write(dp, kDRV_DIAG_TX_DRV + kLANE0_OFFSET,
					cfg->drv_diag_tx_drv);
		edp_phy_cdb_write(dp, kTX_TXCC_MGNFS_MULT_000 + kLANE0_OFFSET,
					cfg->tx_txcc_mgnfc_mult_000);
		edp_phy_cdb_write(dp, kTX_TXCC_CPOST_MULT_00 + kLANE0_OFFSET,
					cfg->tx_txcc_cpost_mult_00);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE0_OFFSET,
					value & ~BIT(0));

		value = edp_phy_cdb_read(dp, kTX_DIAG_ACYA + kLANE1_OFFSET);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE1_OFFSET,
				value | BIT(0));
		edp_phy_cdb_write(dp, kTX_TXCC_CTRL + kLANE1_OFFSET,
					cfg->tx_txcc_ctrl);
		edp_phy_cdb_write(dp, kDRV_DIAG_TX_DRV + kLANE1_OFFSET,
					cfg->drv_diag_tx_drv);
		edp_phy_cdb_write(dp, kTX_TXCC_MGNFS_MULT_000 + kLANE1_OFFSET,
					cfg->tx_txcc_mgnfc_mult_000);
		edp_phy_cdb_write(dp, kTX_TXCC_CPOST_MULT_00 + kLANE1_OFFSET,
					cfg->tx_txcc_cpost_mult_00);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE1_OFFSET,
					value & ~BIT(0));

		value = edp_phy_cdb_read(dp, kTX_DIAG_ACYA + kLANE2_OFFSET);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE2_OFFSET,
				value | BIT(0));
		edp_phy_cdb_write(dp, kTX_TXCC_CTRL + kLANE2_OFFSET,
					cfg->tx_txcc_ctrl);
		edp_phy_cdb_write(dp, kDRV_DIAG_TX_DRV + kLANE2_OFFSET,
					cfg->drv_diag_tx_drv);
		edp_phy_cdb_write(dp, kTX_TXCC_MGNFS_MULT_000 + kLANE2_OFFSET,
					cfg->tx_txcc_mgnfc_mult_000);
		edp_phy_cdb_write(dp, kTX_TXCC_CPOST_MULT_00 + kLANE2_OFFSET,
					cfg->tx_txcc_cpost_mult_00);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE2_OFFSET,
					value & ~BIT(0));

		value = edp_phy_cdb_read(dp, kTX_DIAG_ACYA + kLANE3_OFFSET);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE3_OFFSET,
				value | BIT(0));
		edp_phy_cdb_write(dp, kTX_TXCC_CTRL + kLANE3_OFFSET,
					cfg->tx_txcc_ctrl);
		edp_phy_cdb_write(dp, kDRV_DIAG_TX_DRV + kLANE3_OFFSET,
					cfg->drv_diag_tx_drv);
		edp_phy_cdb_write(dp, kTX_TXCC_MGNFS_MULT_000 + kLANE3_OFFSET,
					cfg->tx_txcc_mgnfc_mult_000);
		edp_phy_cdb_write(dp, kTX_TXCC_CPOST_MULT_00 + kLANE3_OFFSET,
					cfg->tx_txcc_cpost_mult_00);
		edp_phy_cdb_write(dp, kTX_DIAG_ACYA + kLANE3_OFFSET,
					value & ~BIT(0));
		break;
	}

	return trilin_phy_error_none;
}


//------------------------------------------------------------------------------
//	Function: edp_pll_disable
//	Disable PLL0 (PLL1 is not used)
//
// Inputs:
//		None
//
// Outputs:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t edp_pll_disable(struct trilin_dp *dp)
{
	trilin_phy_write(dp, TRILIN_EDP_PHY_DATA_ENABLE, 0);
	trilin_edp_phy_power(dp, trilin_power_a3);

	trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_PLLCLK_ENABLE, 0);
	edp_lane_pll_ack_disabled(dp, 1, 100);

	// disable PLL0 (PLL1 is already off)
	edp_phy_cdb_write(dp, kPHY_PMA_PLL_RAW_CTRL, 0);
	edp_pll_raw_ctrl_ack_disabled(dp, 1, 100);

	return trilin_phy_error_none;
}

//------------------------------------------------------------------------------
//	Function: edp_pll_enable
//	Enable PLL0 (PLL1 is not used)
//
// Inputs:
//		None
//
// Outputs:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t edp_pll_enable(struct trilin_dp *dp)
{
	// enable PLL0
	edp_phy_cdb_write(dp, kPHY_PMA_PLL_RAW_CTRL, 1);
	// wait for operation to complete
	edp_pll_raw_ctrl_ack_ready(dp, 1, 100);

	// enable lane pll clock
	trilin_phy_write(dp, TRILIN_EDP_PHY_LANE_PLLCLK_ENABLE, 1);
	// wait for PLL lane ACK
	edp_lane_pll_ack_ready(dp, 1, 100);

	// bring the power state back up to active
	trilin_edp_phy_power(dp, trilin_power_a2);
	trilin_edp_phy_power(dp, trilin_power_a0);

	trilin_phy_write(dp, TRILIN_EDP_PHY_DATA_ENABLE, 1);

	return trilin_phy_error_none;
}

//------------------------------------------------------------------------------
//	Function: edp_common_ready_ack
//	Poll PHY common ready register.
//		Wait until lane it is ready.
//
// Inputs:
//	delay_time - poll / loop time
//	loop_ct	   - poll / loop count
//
// Outputs:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t edp_common_ready_ack(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_bus_not_idle;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = trilin_phy_read(dp, TRILIN_EDP_PHY_CMN_READY);
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

//-----------------------------------------------------------------------------
//	Function: edp_pll_raw_ctrl_ack
//	Poll PMA_CMN_CTRL2
//		Wait until lane PLL 0 is disabled.
//
// Inputs:
//	delay_time - poll / loop time
//	loop_ct    - poll / loop count
//
// Outputs:
//	Appropriate DPTX PHY status
//-----------------------------------------------------------------------------
static trilin_phy_error_t edp_pll_raw_ctrl_ack_disabled(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_bus_not_idle;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = edp_phy_cdb_read(dp, kPHY_PMA_CMN_CTRL2);
		reg_val &= PMA_CMN_CTRL2_PLL0_DISABLED_BIT;
		if (reg_val) {
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

static trilin_phy_error_t edp_pll_raw_ctrl_ack_ready(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_bus_not_idle;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = edp_phy_cdb_read(dp, kPHY_PMA_CMN_CTRL2);
		reg_val &= PMA_CMN_CTRL2_PLL0_READY_BIT;
		if (reg_val) {
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
//	Function: edp_lane_pll_ack
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
static trilin_phy_error_t edp_lane_pll_ack_disabled(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_bus_not_idle;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = trilin_phy_read(dp, TRILIN_EDP_PHY_LANE_PLLCLK_EN_ACK);
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

static trilin_phy_error_t edp_lane_pll_ack_ready(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_bus_not_idle;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = trilin_phy_read(dp, TRILIN_EDP_PHY_LANE_PLLCLK_EN_ACK);
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
//	Function: edp_pwr_state_ack
//	Poll power state ack register.
//		Wait until power state has been acquired.
//
// Inputs:
//		pwr_state  - New power state
//	delay_time - poll / loop time
//	loop_ct	   - poll / loop count
//
// Outputs:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t edp_pwr_state_ack(
	struct trilin_dp *dp, u32 pwr_state, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_pwr_ack;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = trilin_phy_read(dp, TRILIN_EDP_PHY_LANE_PWR_STATE_ACK);
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

//------------------------------------------------------------------------------
//	Function: edp_phy_cdb_ready
//	Poll config & debug bus ready register.
//		Wait until CDB is ready.
//
// Inputs:
//	delay_time - poll / loop time
//	loop_ct    - poll / loop count
//
// Outputs:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t edp_phy_cdb_ready(
	struct trilin_dp *dp, u32 delay_time, u32 loop_ct)
{
	int idx;
	u32 reg_val;
	trilin_phy_error_t status = trilin_phy_error_bus_not_idle;

	for (idx = 0; idx < loop_ct; idx++) {
		reg_val = trilin_phy_read(dp, TRILIN_EDP_CDB_READY);
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
//	Function: edp_phy_cdb_write
//	Write to the Cadence configuration and debug bus (cdb)
//
// Inputs:
//	addr - configuration register address inside the PHY to write
//	data - data value to write
//
// Outputs:
//	Appropriate DPTX PHY status
//------------------------------------------------------------------------------
static trilin_phy_error_t edp_phy_cdb_write(
	struct trilin_dp *dp, u32 offset, uint16_t data)
{
	trilin_phy_error_t status = edp_phy_cdb_ready(dp, 5, 100);

	if (status == trilin_phy_error_none) {
		trilin_phy_write(dp, TRILIN_EDP_CDB_ADDR,   offset);
		trilin_phy_write(dp, TRILIN_EDP_CDB_WDATA,  data);
		trilin_phy_write(dp, TRILIN_EDP_CDB_SEL_WR, 3);
		trilin_phy_write(dp, TRILIN_EDP_CDB_ENABLE, 1);
	}

	return status;
}

//------------------------------------------------------------------------------
//	Function: cdb_write
//	Read from the Cadence configuration and debug bus (cdb)
//
// Inputs:
//	addr - configuration register address inside the PHY to write
//
// Outputs:
//	data - read data value
//------------------------------------------------------------------------------
static u32 edp_phy_cdb_read(struct trilin_dp *dp, u32 offset)
{
	u32                data = 0;
	trilin_phy_error_t status = edp_phy_cdb_ready(dp, 5, 100);

	if (status == trilin_phy_error_none) {
		trilin_phy_write(dp, TRILIN_EDP_CDB_ADDR,   offset);
		// select flag active
		trilin_phy_write(dp, TRILIN_EDP_CDB_SEL_WR, 0x02);
		// active configured CDB transaction
		trilin_phy_write(dp, TRILIN_EDP_CDB_ENABLE, 0x01);
		// wait for the transaction to complete
		status = edp_phy_cdb_ready(dp, 5, 100);
		if (status == trilin_phy_error_none)
			data = trilin_phy_read(dp, TRILIN_EDP_CDB_RDATA);

	}
	return data;
}

static struct trilin_phy_ops edp_phy_ops = {
	.prepare   = trilin_edp_phy_prepare,
	.init      = trilin_edp_phy_init,
	.reset     = trilin_edp_phy_reset,
	.power     = trilin_edp_phy_power,
	.configure = trilin_edp_phy_configure,
	.exit      = trilin_edp_phy_exit,
};

trilin_phy_error_t trilin_edp_phy_register(struct trilin_dp *dp)
{
	struct trilin_phy_t *phy = &dp->phy;
	u32 reg_val;

	reg_val = (trilin_phy_read(dp, TRILIN_PHY_ID_REV_REG) >> 16);
	if (reg_val == TRILIN_EDP_PHY_ID)
		phy->phy_ops = &edp_phy_ops;

	dev_info(dp->dev, "%s end\n", __func__);
	return trilin_phy_error_none;
}

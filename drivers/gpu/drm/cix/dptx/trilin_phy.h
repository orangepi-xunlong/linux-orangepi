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

#ifndef _TRILIN_PHY_H_
#define _TRILIN_PHY_H_

#define TRILIN_PHY_ID_REV_REG                0xfc
#define TRILIN_EDP_PHY_ID                    0x4d
#define TRILIN_USBDP_PHY_ID                  0x54

//------------------------------------------------------------------------------
//	power state defines
//------------------------------------------------------------------------------
#define TRILIN_PHY_PWR_STATE_IDLE            0x0000
#define TRILIN_PHY_PWR_STATE_A0_TXRX_ACTIVE  0x0001
#define TRILIN_PHY_PWR_STATE_A1_POWERDOWN1   0x0002 // A1 is not a used power state for DisplayPort
#define TRILIN_PHY_PWR_STATE_A2_POWERDOWN2   0x0004 // A2 & A3 are configured identically for DisplayPort
#define TRILIN_PHY_PWR_STATE_A3_POWERDOWN3   0x0008 // A3 requires special handshaking for entry/exit

//------------------------------------------------------------------------------
//	driver typedefs
//------------------------------------------------------------------------------
typedef enum {
	trilin_phy_error_none,
	trilin_phy_error_core_not_present,
	trilin_phy_error_invalid_ref_clk,
	trilin_phy_error_invalid_link_clk,
	trilin_phy_error_bus_not_idle,
	trilin_phy_error_reset_type,
	trilin_phy_error_reset_ns,
	trilin_phy_error_link_rate,
	trilin_phy_error_lane_count,
	trilin_phy_error_pwr_type,
	trilin_phy_error_pwr_ack,
	trilin_phy_error_pwr_ns,
} trilin_phy_error_t;

typedef enum {
	trilin_ref_clk_24mhz,
	trilin_ref_clk_26mhz,
	trilin_ref_clk_108mhz,
	trilin_refclk_135_81mhz,
} trilin_phy_ref_clk_t;

typedef enum {
	trilin_phy_reset_all,
	trilin_phy_reset_min,
} trilin_phy_reset_t;

typedef enum {
	trilin_link_clk_1p62gbps,
	trilin_link_clk_2p43gbps,
	trilin_link_clk_2p70gbps,
	trilin_link_clk_3p24gbps,
	trilin_link_clk_5p40gbps,
	trilin_link_clk_8p10gbps,
} trilin_phy_link_clk_t;

typedef enum {
	trilin_lane_en_1 = 0x01,
	trilin_lane_en_2 = 0x03,
	trilin_lane_en_4 = 0x0f,
} trilin_phy_lane_en_t;

typedef enum {
	trilin_power_a0,
	trilin_power_a1,
	trilin_power_a2,
	trilin_power_a3,
} trilin_phy_power_state_t;

typedef enum {
	trilin_phy_unknown,
	trilin_phy_prepared,
	trilin_phy_power_on,
	trilin_phy_power_off,
} trilin_phy_state_t;

//------------------------------------------------------------------------------
//	 function prototypes
//------------------------------------------------------------------------------
struct trilin_dp;
union phy_configure_opts;

struct trilin_phy_ops {
	trilin_phy_error_t (*prepare)   (struct trilin_dp *dp);
	trilin_phy_error_t (*init)      (struct trilin_dp *dp, trilin_phy_ref_clk_t ref_clk);
	trilin_phy_error_t (*reset)     (struct trilin_dp *dp, trilin_phy_reset_t reset_type);
	trilin_phy_error_t (*power)     (struct trilin_dp *dp, trilin_phy_power_state_t state);
	trilin_phy_error_t (*configure) (struct trilin_dp *dp, union phy_configure_opts *opts);
	trilin_phy_error_t (*exit)      (struct trilin_dp *dp);
};

struct trilin_phy_t {
	struct phy *base;
	struct trilin_phy_ops *phy_ops;
	bool is_edp;
	trilin_phy_state_t state;
	trilin_phy_lane_en_t lane_en;
	u32 vs;
	u32 pe;
};

#endif

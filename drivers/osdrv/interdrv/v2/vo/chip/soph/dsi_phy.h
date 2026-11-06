/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _DSI_PHY_H_
#define _DSI_PHY_H_

#include <linux/comm_mipi_tx.h>
#include "disp.h"

void dphy_set_base_addr(u8 inst, void *base);
void dphy_dsi_lane_en(u8 inst, bool clk_en, bool *data_en, bool preamble_en);
void dphy_get_dsi_lane_status(u8 inst, bool *data_en);
bool dphy_get_dsi_clk_lane_status(u8 inst);
int dphy_dsi_set_lane(u8 inst, u8 lane_num, enum mipi_tx_lane_id lane, bool pn_swap, bool clk_phase_shift);
void dphy_dsi_disable_lanes(u8 inst);
int dphy_dsi_get_lane(u8 inst, enum mipi_tx_lane_id *data_lane_num);
void dphy_init(u8 inst, enum vo_disp_intf intf);
void dphy_dsi_get_pixclk(u8 inst, u32 *clk_khz, u8 lane, u8 bits);
void dphy_dsi_set_pll(u8 inst, u32 clk_khz, u8 lane, u8 bits);
void dphy_lvds_set_pll(u8 inst, u32 clk_khz, u8 link);
bool dphy_is_lvds(u8 inst);
void dphy_dsi_clk_setting(u8 inst, u32 value);
void dphy_dsi_set_gp_drv_level(u8 inst);

void dpyh_mipi_tx_manual_packet(u8 inst, const u8 *data, u8 count);

void dphy_set_hs_settle(u8 inst, u8 prepare, u8 zero, u8 trail);
void dphy_get_hs_settle(u8 inst, u8 *prepare, u8 *zero, u8 *trail);

#endif	// _DSI_PHY_H

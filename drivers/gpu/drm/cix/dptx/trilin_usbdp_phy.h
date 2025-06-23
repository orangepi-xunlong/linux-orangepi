// SPDX-License-Identifier: GPL-2.0
//------------------------------------------------------------------------------
//	Trilinear Technologies DisplayPort DRM Driver
//	Copyright (C) 2024 Trilinear Technologies
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

#ifndef _TRILIN_USBDP_PHY_H_
#define _TRILIN_USBDP_PHY_H_


//------------------------------------------------------------------------------
//	 register map
//------------------------------------------------------------------------------

#define TRILIN_USBDP_PHY_CDB_RSTN            0x10
#define TRILIN_USBDP_PHY_CDB_TIMEOUT_COUNT   0x20
#define TRILIN_USBDP_PHY_CDB_TIMEOUT         0x24
#define TRILIN_USBDP_PHY_CONFIG_RSTN         0x40
#define TRILIN_USBDP_PHY_CMN_READY           0x44
#define TRILIN_USBDP_PHY_LANE_EN_BITS        0x48
#define TRILIN_USBDP_PHY_LANE_RSTN_BITS      0x4c
#define TRILIN_USBDP_PHY_DATA_ENABLE         0x50
#define TRILIN_USBDP_PHY_AUXHPD_ENABLE_BITS  0x54
#define TRILIN_USBDP_PHY_TX_ELEC_IDLE_BITS   0x58
#define TRILIN_USBDP_PHY_DP_RESET            0x5c
#define TRILIN_USBDP_PHY_LANE_PWR_STATE_REQ  0x60
#define TRILIN_USBDP_PHY_LANE_PWR_STATE_ACK  0x64
#define TRILIN_USBDP_PHY_LANE_PLLCLK_ENABLE  0x68
#define TRILIN_USBDP_PHY_LANE_PLLCLK_EN_ACK  0x6c
#define TRILIN_USBDP_PHY_POWERDOWN           0x70
#define TRILIN_USBDP_PHY_TX_LINK_RATE        0x78
#define TRILIN_USBDP_TYPEC_CONN_ORIENTATION  0xa0
#define TRILIN_USBDP_PHY_TEST_RDWR_ADDRESS   0xf4
#define TRILIN_USBDP_PHY_MAX_LANE_COUNT      0xf8
#define TRILIN_USBDP_PHY_TRILINEAR_ID_REV    0xfc

//------------------------------------------------------------------------------
//	power state defines
//------------------------------------------------------------------------------
#define TRILIN_USBDP_PHY_PWR_STATE_IDLE              0x0000

#define TRILIN_USBDP_PHY_PWR_STATE_A0_TXRX_ACTIVE    0x0001

// note: state A1 is not a used power state for DisplayPort
#define TRILIN_USBDP_PHY_PWR_STATE_A1_POWERDOWN1     0x0002

// note: states A2 & A3 are configured identically for DisplayPort
#define TRILIN_USBDP_PHY_PWR_STATE_A2_POWERDOWN2     0x0004

// note: state A3 requires special handshaking for entry/exit
#define TRILIN_USBDP_PHY_PWR_STATE_A3_POWERDOWN3     0x0008


trilin_phy_error_t trilin_usbdp_phy_register(struct trilin_dp *dp);

#endif

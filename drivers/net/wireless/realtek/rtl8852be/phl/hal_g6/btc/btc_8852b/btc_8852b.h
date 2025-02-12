/******************************************************************************
 *
 * Copyright(c) 2016 - 2019 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef __RTL8852B_BTC_H__
#define __RTL8852B_BTC_H__
/* rtl8852b_btc.c */

#define R_BTC_RF_BTG_CTRL 0x2
#define R_BTC_RF_LUT_EN 0xEF
#define R_BTC_RF_LUT_WA 0x33
#define R_BTC_RF_LUT_WD0 0x3f
#define R_BTC_RF_LUT_WD1 0x3e
#define R_BTC_RF_LUT_WD2 0x3d

#define R_BTC_BB_ANT_DIV_CTRL 0x1586c
#define R_BTC_BB_BTG_RX 0x980
#define R_BTC_BB_PRE_AGC_S1 0x476C
#define R_BTC_BB_PRE_AGC_S0 0x4688

#define R_BTC_CFG 0xDA00
#define R_BTC_WL_PRI_MSK 0xDA10
#define R_BTC_BREAK_TABLE 0xDA2C
#define R_BTC_BT_COEX_MSK_TABLE 0xDA30
#define R_BTC_CSR_MODE 0xDA40
#define R_BTC_BT_STAST_HIGH 0xDA44
#define R_BTC_BT_STAST_LOW 0xDA48

#define B_BTC_PRI_MASK_TX_RESP_V1 BIT(3)
#define B_BTC_PRI_MASK_RXCCK_V1 BIT(28)
#define B_BTC_DBG_GNT_WL_BT BIT(27)
#define B_BTC_BT_CNT_REST BIT(16)
#define B_BTC_PTA_WL_PRI_MASK_BCNQ BIT(8)
#define B_BTC_PTA_WL_PRI_MASK_MGQ BIT(4)
#define B_BTC_BB_GNT_MUX 0x001e0000
#define B_BTC_BB_PRE_AGC_MASK bMASKB3
#define B_BTC_BB_PRE_AGC_VAL 0x80000000

extern const struct btc_chip chip_8852b;
extern const struct btc_chip chip_8852bp;
extern const struct btc_chip chip_8851b;
extern const struct btc_chip chip_8852bt;
void _8852b_rfe_type(struct btc_t *btc);
void _8852b_init_cfg(struct btc_t *btc);
void _8852b_wl_tx_power(struct btc_t *btc, u32 level);
void _8852b_wl_rx_gain(struct btc_t *btc, u32 level);
void _8852b_wl_btg_standby(struct btc_t *btc, u32 state);
void _8852b_wl_req_mac(struct btc_t *btc, u8 mac_id);
void _8852b_get_reg_status(struct btc_t *btc, u8 type, void *status);
u8 _8852b_bt_rssi(struct btc_t *btc, u8 val);
#endif /*__RTL8852B_PHY_H__*/

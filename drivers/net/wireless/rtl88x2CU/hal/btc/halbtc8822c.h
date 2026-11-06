/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation.
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

extern const struct btc_chip_para btc_chip_para_8822c;
void halbtc8822c_chip_setup(struct btc_coexist *btc, u8 type);
void halbtc8822c_cfg_init(struct btc_coexist *btc);
void halbtc8822c_cfg_ant_switch(struct btc_coexist *btc);
void halbtc8822c_cfg_gnt_debug(struct btc_coexist *btc);
void halbtc8822c_cfg_fre_type(struct btc_coexist *btc);
void halbtc8822c_cfg_coexinfo_hw(struct btc_coexist *btc);
void halbtc8822c_cfg_wl_tx_power(struct btc_coexist *btc);
void halbtc8822c_cfg_wl_rx_gain(struct btc_coexist *btc);
void halbtc8822c_cfg_wlan_act_ips(struct btc_coexist *btc);
void halbtc8822c_cfg_bt_ctrl_act(struct btc_coexist *btc);


/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#ifndef __HALRF_TXGAPK_8822C_H__
#define __HALRF_TXGAPK_8822C_H__

#if (RTL8822C_SUPPORT == 1)

void halrf_txgapk_save_all_tx_gain_table_8822c(void *dm_void);

void halrf_txgapk_reload_tx_gain_8822c(void *dm_void);

void halrf_txgapk_8822c(void *dm_void);


#endif /* RTL8822C_SUPPORT */
#endif /*#ifndef __HALRF_TSSI_8822C_H__*/

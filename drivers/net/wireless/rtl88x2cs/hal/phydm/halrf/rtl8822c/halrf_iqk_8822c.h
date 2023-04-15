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

#ifndef __HALRF_IQK_8822C_H__
#define __HALRF_IQK_8822C_H__

#if (RTL8822C_SUPPORT == 1)
/*--------------------------Define Parameters-------------------------------*/
#define MAC_REG_NUM_8822C 3
#define BB_REG_NUM_8822C 21
#define RF_REG_NUM_8822C 3
#define IQK_DELAY_8822C 2
#define IQK_STEP_8822C 7
#define RXK_STEP_8822C 6
#define IQK_CMD_8822C 0x8

#define TXIQK 0
#define RXIQK 1
#define SS_8822C 2

#define IQK_INFO_RSVD_LEN_8822C	568
/*---------------------------End Define Parameters-------------------------------*/
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
void do_iqk_8822c(
	void *dm_void,
	u8 delta_thermal_index,
	u8 thermal_value,
	u8 threshold);
#else
void do_iqk_8822c(
	void *dm_void,
	u8 delta_thermal_index,
	u8 thermal_value,
	u8 threshold);
#endif

void phy_iq_calibrate_8822c(
	void *dm_void,
	boolean clear,
	boolean segment_iqk);

void iqk_get_cfir_8822c(void *dm_void, u8 idx, u8 path, boolean debug);

void iqk_set_cfir_8822c(void *dm_void, u8 idx, u8 path, boolean debug);

void iqk_reload_iqk_8822c(void *dm_void, boolean reset);

void phy_get_iqk_cfir_8822c(void *dm_void, u8 idx, u8 path, boolean debug);

void phy_iqk_dbg_cfir_backup_8822c(void *dm_void);

void phy_iqk_dbg_cfir_backup_update_8822c(void *dm_void);

void phy_iqk_dbg_cfir_reload_8822c(void *dm_void);

void phy_iqk_dbg_cfir_write_8822c(void *dm_void, u8 type, u32 path, u32 idx, u32 i, u32 data);

void phy_iqk_dbg_cfir_backup_show_8822c(void *dm_void);

void iqk_info_rsvd_page_8822c(void *dm_void, u8 *buf, u32 *buf_size);

void iqk_power_save_8822c(void *dm_void, boolean is_power_save);


#else /* (RTL8822C_SUPPORT == 0)*/

#define phy_iq_calibrate_8822c(_pdm_void, clear, segment_iqk)

#endif /* RTL8822C_SUPPORT */

#endif /*__HALRF_IQK_8822C_H__*/

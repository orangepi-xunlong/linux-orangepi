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
#if (RTL8822C_SUPPORT)
#ifndef __ODM_RTL8822C_H__
#define __ODM_RTL8822C_H__

/* 2019.08.20: modify code structure*/
#define HW_SETTING_VERSION_8822C "1.1"

enum phydm_bf_linked {
	PHYDM_IS_BF_LINKED	= 1,
	PHYDM_NO_BF_LINKED	= 2,
};

void phydm_hwsetting_8822c(struct dm_struct *dm);
#endif
#endif

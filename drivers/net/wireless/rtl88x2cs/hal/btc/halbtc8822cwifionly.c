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
#include "mp_precomp.h"


VOID
ex_hal8822c_wifi_only_hw_config(
	IN struct wifi_only_cfg *pwifionlycfg
	)
{
	halwifionly_phy_set_bb_reg(pwifionlycfg, 0x70, 0xff000000, 0x0e);
	/*gnt_wl=1 , gnt_bt=0*/
	halwifionly_phy_set_bb_reg(pwifionlycfg, 0x1704, 0xffffffff, 0x7700);
	halwifionly_phy_set_bb_reg(pwifionlycfg, 0x1700, 0xffffffff, 0xc00f0038);

	halwifionly_phy_set_bb_reg(pwifionlycfg, 0x6c0, 0xffffffff, 0xaaaaaaaa);
	halwifionly_phy_set_bb_reg(pwifionlycfg, 0x6c4, 0xffffffff, 0xaaaaaaaa);
}

VOID
ex_hal8822c_wifi_only_scannotify(
	IN struct wifi_only_cfg *pwifionlycfg,
	IN u1Byte  is_5g
	)
{
}

VOID
ex_hal8822c_wifi_only_switchbandnotify(
	IN struct wifi_only_cfg *pwifionlycfg,
	IN u1Byte  is_5g
	)
{
}

VOID
ex_hal8822c_wifi_only_connectnotify(
	IN struct wifi_only_cfg *pwifionlycfg,
	IN u1Byte  is_5g
	)
{
}


VOID
hal8822c_wifi_only_switch_antenna(
	IN struct wifi_only_cfg *pwifionlycfg,
	IN u1Byte  is_5g
	)
{
}

/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation. All rights reserved.
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
 ******************************************************************************/

#ifndef HALMAC_POWER_SEQUENCE_8822B
#define HALMAC_POWER_SEQUENCE_8822B

#include "../../halmac_pwr_seq_cmd.h"
#include "../../halmac_hw_cfg.h"

#if HALMAC_8822B_SUPPORT

#define HALMAC_8822B_PWR_SEQ_VER  "V28"

extern PHALMAC_WLAN_PWR_CFG halmac_8822b_card_disable_flow[];
extern PHALMAC_WLAN_PWR_CFG halmac_8822b_card_enable_flow[];
extern PHALMAC_WLAN_PWR_CFG halmac_8822b_suspend_flow[];
extern PHALMAC_WLAN_PWR_CFG halmac_8822b_resume_flow[];
extern PHALMAC_WLAN_PWR_CFG halmac_8822b_hwpdn_flow[];
extern PHALMAC_WLAN_PWR_CFG halmac_8822b_enter_lps_flow[];
extern PHALMAC_WLAN_PWR_CFG halmac_8822b_enter_deep_lps_flow[];
extern PHALMAC_WLAN_PWR_CFG halmac_8822b_leave_lps_flow[];

#endif /* HALMAC_8822B_SUPPORT*/

#endif

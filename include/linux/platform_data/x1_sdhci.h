// SPDX-License-Identifier: GPL-2.0
/*
 * include/linux/platform_data/x1_sdhci.h
 *
 * Copyright (C) 2023 Ky
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _X1_SDHCI_H_
#define _X1_SDHCI_H_

#define CANDIDATE_WIN_NUM 3
#define SELECT_DELAY_NUM 9
#define WINDOW_1ST 0
#define WINDOW_2ND 1
#define WINDOW_3RD 2

#define RX_TUNING_WINDOW_THRESHOLD 80
#define RX_TUNING_DLINE_REG 0x09
#define TX_TUNING_DLINE_REG 0x00
#define TX_TUNING_DELAYCODE 127

enum window_type {
	LEFT_WINDOW = 0,
	MIDDLE_WINDOW = 1,
	RIGHT_WINDOW = 2,
};

struct tuning_window {
	u8 type;
	u8 min_delay;
	u8 max_delay;
};

struct rx_tuning {
	u8 rx_dline_reg;
	u8 select_delay_num;
	u8 current_delay_index;
	/* 0: biggest window, 1: bigger, 2:  small */
	struct tuning_window windows[CANDIDATE_WIN_NUM];
	u8 select_delay[SELECT_DELAY_NUM];

	u32 card_cid[4];
	u8 window_limit;
	u8 tuning_fail;
	u8 window_type;
};

/*
 * struct x1_sdhci_platdata() - Platform device data for Ky X1x SDHCI
 * @flags: flags for platform requirement
 * @host_caps: Standard MMC host capabilities bit field
 * @host_caps2: Standard MMC host capabilities bit field
 * @host_caps_disable: Aquila MMC host capabilities disable bit field
 * @host_caps2_disable: Aquila MMC host capabilities disable bit field
 * @quirks: quirks of platform
 * @quirks2: quirks2 of platform
 * @pm_caps: pm_caps of platform
 */
struct x1_sdhci_platdata {
	u32 host_freq;
	u32 flags;
	u32 host_caps;
	u32 host_caps2;
	u32 host_caps_disable;
	u32 host_caps2_disable;
	u32 quirks;
	u32 quirks2;
	u32 pm_caps;

	u32 aib_mmc1_io_reg;
	u32 apbc_asfar_reg;
	u32 apbc_assar_reg;

	u8 tx_dline_reg;
	u8 tx_delaycode;
	u8 phy_driver_sel;
	struct rx_tuning rxtuning;
	u8 need_reset_dllcfg1;
	u32 prev_dllcfg1;
	u32 curr_dllcfg1;
	u32 new_dllcfg1;
	u8 dllcfg1_odd_reset;
	u32 rx_tuning_freq;
};

#endif /* _X1_SDHCI_H_ */

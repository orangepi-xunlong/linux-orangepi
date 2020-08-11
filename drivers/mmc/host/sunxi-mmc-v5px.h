/*
* Sunxi SD/MMC host driver
*
* Copyright (C) 2015 AllWinnertech Ltd.
* Author: lixiang <lixiang@allwinnertech>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifdef CONFIG_ARCH_SUN8IW10P1

#ifndef __SUNXI_MMC_SUN8IW10P1_2_H__
#define __SUNXI_MMC_SUN8IW10P1_2_H__

void sunxi_mmc_init_priv_v5px(struct sunxi_mmc_host *host,
			      struct platform_device *pdev, int phy_index);
int sunxi_mmc_oclk_onoff(struct sunxi_mmc_host *host, u32 oclk_en);

int mmc_card_sleep(struct mmc_host *host);
int mmc_deselect_cards(struct mmc_host *host);
void mmc_power_off(struct mmc_host *host);
int mmc_card_sleepawake(struct mmc_host *host, int sleep);

#endif

#endif

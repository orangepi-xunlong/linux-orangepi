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

#ifndef __SUNXI_MMC_EXPORT_H__
#define __SUNXI_MMC_EXPORT_H__
void sunxi_mmc_rescan_card(unsigned id);
void sunxi_mmc_reg_ex_res_inter(struct sunxi_mmc_host *host, u32 phy_id);
int sunxi_mmc_check_r1_ready(struct mmc_host *mmc, unsigned ms);

#endif

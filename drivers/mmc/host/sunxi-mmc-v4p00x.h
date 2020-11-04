/*
* SUNXI EMMC/SD driver
*
* Copyright (C) 2015 AllWinnertech Ltd.
* Author: lijuan <lijuan@allwinnertech.com>
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

#ifndef __SUNXI_MMC_V4P00X_H__
#define __SUNXI_MMC_V4P00X_H__

void sunxi_mmc_init_priv_v4p00x(struct sunxi_mmc_host *host,
			       struct platform_device *pdev, int phy_index);
#endif


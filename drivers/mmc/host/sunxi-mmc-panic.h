/*
* Sunxi SD/MMC host driver
*
* Copyright (C) 2019 AllWinnertech Ltd.
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

#ifndef __SUNXI_MMC_PANIC_H__
#define __SUNXI_MMC_PANIC_H__
ssize_t
sunxi_mmc_panic_rtest(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t
sunxi_mmc_pancic_wrtest(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count);
int sunxi_mmc_panic_init_ps(void *data);
#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
*
* Allwinner DWMAC driver sysfs haeder.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
*/

#ifndef _DWMAC_SUNXI_SYSFS_H_
#define _DWMAC_SUNXI_SYSFS_H_

#include "dwmac-sunxi.h"

void sunxi_dwmac_sysfs_init(struct device *dev);
void sunxi_dwmac_sysfs_exit(struct device *dev);

#endif /* _DWMAC_SUNXI_SYSFS_H_ */


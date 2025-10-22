/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
 *
 * Author: ouyangkun <ouyangkun@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __LINUX_SUNXI_NSI_H
#define __LINUX_SUNXI_NSI_H

#if IS_ENABLED(CONFIG_AW_NSI_DISTRIBUTE)
void sunxi_nsi_master_ready(struct device *dev);
#else
static inline void sunxi_nsi_master_ready(struct device *dev) { ; }
#endif

#endif
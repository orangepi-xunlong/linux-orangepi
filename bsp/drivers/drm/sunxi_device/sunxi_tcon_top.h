/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* sunxi_tcon_top.h
 *
 * Copyright (C) 2023 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _SUNXI_TCON_TOP_H_
#define _SUNXI_TCON_TOP_H_

#include <linux/clk.h>
#include "include.h"
#include "tcon_top.h"

int sunxi_tcon_top_clk_enable(struct device *tcon_top);
int sunxi_tcon_top_clk_disable(struct device *tcon_top);

#endif

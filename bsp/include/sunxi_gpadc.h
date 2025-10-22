// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * fuzhaoke <fuzhaoke@allwinnertech.com>
 *
 * SUNXI GPADC Controller Driver Header
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#ifndef SUNXI_GPADC_H
#define SUNXI_GPADC_H

u32 sunxi_gpadc_read_channel_data(u32 controller_num, u8 channel);

#endif  /* SUNXI_GPADC_H */

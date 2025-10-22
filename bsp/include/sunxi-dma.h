/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI DMA support
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
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

#ifndef __SUNXI_DMA_H
#define __SUNXI_DMA_H

typedef void (*sunxi_dma_timeout_callback)(void *param);

struct sunxi_dma_desc {
	bool is_bmode;
	bool is_timeout;
	bool byte_per_pkg;
	unsigned timeout_steps; /* the steps is the time for channel timer,the max is 511,1 step = 20.48us */
	unsigned timeout_fun; /* the fun is used to set the timout interrupt peding fun,0x01 is pause,0x10 is end,0x11 is next descrp */
	sunxi_dma_timeout_callback callback;
	void *callback_param;
};

#endif

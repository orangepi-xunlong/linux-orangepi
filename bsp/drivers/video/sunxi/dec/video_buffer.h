/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2007-2021 Allwinnertech Co., Ltd.
 *
 * Author: Yajiaz <yajianz@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _dec_video_buffer_h_
#define _dec_video_buffer_h_

#include <linux/dma-buf.h>

void video_buffer_init(struct device *pdev);
int video_buffer_map(uint32_t phy, size_t len, dma_addr_t *device_addr);
void video_buffer_unmap(dma_addr_t device_addr);

int video_framebuffer_allocate(size_t len, dma_addr_t *device_addr);
int video_framebuffer_free(dma_addr_t device_addr);

int dec_vbi_buffer_query(uint32_t *offset, uint64_t *ioaddr, uint32_t *size);

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dec_fence.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: yajianz <yajianz@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SUNXI_DEC_FENCE_H__
#define __SUNXI_DEC_FENCE_H__

struct dec_fence_context;

struct dec_fence_context *dec_fence_context_alloc(const char *name);
void dec_fence_context_free(struct dec_fence_context *fctx);

struct dma_fence *dec_fence_alloc(struct dec_fence_context *fctx);
void dec_fence_free(struct dma_fence *fence);
void dec_fence_signal(struct dma_fence *fence);
int dec_fence_fd_create(struct dma_fence *fence);

#endif

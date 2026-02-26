/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs g2d driver.
 *
 * Copyright (C) 2020 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _G2D_BUF_CACHE_H_
#define _G2D_BUF_CACHE_H_

/**
 * @brief init lrucache for dma-buf map/unmap
 *
 * @param dev g2d device for dma mapping
 * @param cache_size lrucache depth
 * @return 0 on success
 */
int g2d_buf_cache_init(struct device *dev, int cache_size);

/**
 * @brief destroy lrucache
 *
 * @return 0 on success
 */
int g2d_buf_cache_exit(void);

/**
 * @brief format print cache info to string buf
 *
 * @param buf string buf to store the debug message
 * @return byte size writing into the buf
 */
size_t g2d_buf_cache_debug_show(char *buf);

/**
 * @brief map dma-buf handle and get the device address
 *
 * @param fd dma-buf handle
 * @param addr the device address
 * @return 0 on success
 */
int g2d_buf_cached_map(int fd, dma_addr_t *addr);

/**
 * @brief unmap a dma-buf
 *
 * @param fd dma-buf handle to unmap
 * @param addr device address of this dma-buf
 * @return 0 on success
 */
int g2d_buf_cached_unmap(int fd, dma_addr_t addr);

#endif

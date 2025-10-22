/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Sunxi Remote processor framework
 *
 * Copyright (C) 2022 Allwinner, Inc.
 * Base on remoteproc_internal.h
 *
 * lijiajian <lijianjian@allwinner.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SUNXI_RPROC_FIRMWARE_H
#define SUNXI_RPROC_FIRMWARE_H

#include <linux/firmware.h>

int sunxi_request_firmware(const struct firmware **fw,
				const char *name, struct device *dev);
int sunxi_register_memory_fw(const char *name, phys_addr_t addr, uint32_t len);
int sunxi_register_memory_bin_fw(const char *name, phys_addr_t addr, uint32_t len);
void sunxi_unregister_memory_fw(const char *name);

#endif /* SUNXI_RPROC_FIRMWARE_H */

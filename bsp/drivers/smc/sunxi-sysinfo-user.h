/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * The interface define of sysinfo dev for userspace.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * Matteo <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __SUNXI_SYSINFO_USER_H__
#define __SUNXI_SYSINFO_USER_H__

#define CHECK_SOC_SECURE_ATTR 0x00
#define CHECK_SOC_VERSION     0x01
#define CHECK_SOC_BONDING     0x03
#define CHECK_SOC_CHIPID      0x04
#define CHECK_SOC_FT_ZONE     0x05
#define CHECK_SOC_ROTPK_STATUS     0x06
#define CHECK_SOC_BATCHNO     0x07
#define CHECK_SOC_CHIPID_ORIGIN 0x08

#endif /* end of __SUNXI_SYSINFO_USER_H__ */

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *    Filename: cedarv_ve.h
 *     Version: 0.01alpha
 * Description: Video engine driver API, Don't modify it in user space.
 *     License: GPLv2
 *
 *		Author  : xyliu <xyliu@allwinnertech.com>
 *		Date    : 2016/04/13
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
 /* Notice: It's video engine driver API, Don't modify it in user space. */

#ifndef _VE_PLAT_H_
#define _VE_PLAT_H_

#define DVFS_EFUSE_OFF	(0x48)

struct ve_dvfs_pair {
	unsigned int dvfs_index;
	const char dvfs_name[64];
	const char dvfs_freq_name[64];
	const char dvfs_volt_name[64];
};

extern struct ve_dvfs_pair ve_dvfs[3];

#endif

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

#define VE_REG_TOP_BASE		0x0000
#define VE_REG_VCUENC_BASE	0x0900
#define VE_REG_ENCPP_BASE	0x0a00
#define VE_REG_ENC_BASE		0x0b00
#define VE_REG_H265_EXT_BASE	0x0c00
#define VE_REG_DECJPEG_BASE	0x0e00

#define VE_REG_TOP_DECODER		(VE_REG_TOP_BASE + 0x2c)
#define VE_REG_TOP_ENCODER		(VE_REG_TOP_BASE + 0x44)
#define VE_REG_TOP_VCU_CFG		(VE_REG_TOP_BASE + 0x5c)

#define VE_REG_DECJPEG_INT_CTRL		(VE_REG_DECJPEG_BASE + 0x14)
#define VE_REG_DECJPEG_INT_STA		(VE_REG_DECJPEG_BASE + 0x1c)

#define VE_REG_ENCPP_INT_CTRL		(VE_REG_ENCPP_BASE + 0x08)
#define VE_REG_ENCPP_INT_STA		(VE_REG_ENCPP_BASE + 0x10)

#define VE_REG_ENC_INT_CTRL		(VE_REG_ENC_BASE + 0x14)
#define VE_REG_ENC_INT_STA		(VE_REG_ENC_BASE + 0x1c)

#define VE_REG_VCUENC_INT_CTRL		(VE_REG_VCUENC_BASE + 0x20)
#define VE_REG_VCUENC_INT_STA		(VE_REG_VCUENC_BASE + 0x28)
#define VE_REG_VCUENC_CSIINT_CTRL	(VE_REG_VCUENC_BASE + 0x70)
#define VE_REG_VCUENC_CSIERR_STA	(VE_REG_VCUENC_BASE + 0x78)
#define VE_REG_VCUENC_CSIWARN_STA	(VE_REG_VCUENC_BASE + 0x74)

#define VE_MASK_S0B01_S1B01_CSIERR_STA		0x03330033
#define VE_MASK_S0B01_S1B0_CSIERR_STA		0x03330013
#define VE_MASK_S0B0_S1B01_CSIERR_STA		0x03330031
#define VE_MASK_S0B0_S1B0_CSIERR_STA		0x03130011
#define VE_MASK_S0B01_CSIERR_STA		0x01310003
#define VE_MASK_S1B01_CSIERR_STA		0x02320030
#define VE_MASK_S0B0_CSIERR_STA			0x01110001
#define VE_MASK_S1B0_CSIERR_STA			0x02120010

#define VE_MASK_S0B01_S1B01_CSIWARN_STA		0x80230033
#define VE_MASK_S0B01_S1B0_CSIWARN_STA		0x80230013
#define VE_MASK_S0B0_S1B01_CSIWARN_STA		0x80230031
#define VE_MASK_S0B0_S1B0_CSIWARN_STA		0x80030011
#define VE_MASK_S0B01_CSIWARN_STA		0x80210003
#define VE_MASK_S1B01_CSIWARN_STA		0x80220030
#define VE_MASK_S0B0_CSIWARN_STA		0x80010001
#define VE_MASK_S1B0_CSIWARN_STA		0x80020010

#define VE_DEBUGFS_MAX_CHANNEL	16
#define VE_DEBUGFS_BUF_SIZE	1024

struct ve_dvfs_pair {
	unsigned int dvfs_index;
	const char dvfs_name[64];
	const char dvfs_freq_name[64];
	const char dvfs_volt_name[64];
};

extern struct ve_dvfs_pair ve_dvfs[3];
#define DVFS_EFUSE_OFF   (0x48)

#endif

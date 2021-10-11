/*
 * The key define in SUNXI Efuse.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * Matteo <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __SUNXI_SID_EFUSE_H__
#define __SUNXI_SID_EFUSE_H__

#include <linux/sunxi-sid.h>

struct efuse_key_map {
	char name[SUNXI_KEY_NAME_LEN];
	int offset;
	int size;	 /* the number of key in bits */
	int read_flag_shift;
	int burn_flag_shift;
	int public;
	int reserve[3];
};

//H616
static struct efuse_key_map key_maps[] = {
/* Name                  Offset Size ReadFlag BurnFlag Public Reserve */
{EFUSE_CHIPID_NAME,      0x0,   128,  0,      0,      1,     {0} },
{EFUSE_BROM_CONF_NAME,   0x10,  16,   1,      1,      1,     {0} },
{EFUSE_BROM_TRY_NAME,    0x12,  16,   1,      1,      1,     {0} },
{EFUSE_THM_SENSOR_NAME,  0x14,  64,   2,      2,      1,     {0} },
{EFUSE_FT_ZONE_NAME,     0x1C,  128,  3,      3,      1,     {0} },
{EFUSE_OEM_NAME,         0x2C,  160,  4,      4,      1,     {0} },
{"",                     0,     0,    0,      0,      0,     {0} },
};

static struct efuse_key_map key_map_rd_pro = {
EFUSE_RD_PROTECT_NAME,  0x44,   32,   6,      6,      0,     {0} };
static struct efuse_key_map key_map_wr_pro = {
EFUSE_WR_PROTECT_NAME,  0x40,   32,   5,      5,      0,     {0} };

#endif /* end of __SUNXI_SID_EFUSE_H__ */


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

#define EFUSE_CTRL_BASE "allwinner,sunxi-chipid"

struct efuse_key_map {
	char name[SUNXI_KEY_NAME_LEN];
	int offset;
	int size;			 /* the number of key in bits */
	int read_flag_shift;
	int burn_flag_shift;
	int reserve[4];
};

#ifdef CONFIG_ARCH_SUN8IW10

#define EFUSE_CTRL_REG  0x0

static struct efuse_key_map key_maps[] = {
/*   Name                    Offset Size ReadFlag BurnFlag Reserve */
	{EFUSE_CHIPID_NAME,      0x0,   128, -1,      -1,       {0} },
	{EFUSE_THM_SENSOR_NAME,  0x10,  32,  -1,      -1,       {0} },
	{"",                     0,     0,   0,       0,        {0} }
};

#elif defined(CONFIG_ARCH_SUN8IW11)

#define EFUSE_CTRL_REG  0x8C

static struct efuse_key_map key_maps[] = {
/*   Name                    Offset Size ReadFlag BurnFlag Reserve */
	{EFUSE_CHIPID_NAME,      0x0,   128, -1,      -1,      {0} },
	{EFUSE_IN_NAME,          0x10,  256, 23,      11,      {0} },
	{EFUSE_SSK_NAME,         0x30,  128, 22,      10,      {0} },
	{EFUSE_RESERVED2_NAME,   0x40,  64,  -1,      -1,      {0} },
	{EFUSE_THM_SENSOR_NAME,  0x48,  32,  -1,      -1,      {0} },
	{EFUSE_TV_OUT_NAME,      0x4C,  128, -1,      -1,      {0} },
	{EFUSE_RSSK_NAME,        0x5C,  256, 20,      8,       {0} },
	{EFUSE_HDCP_HASH_NAME,   0x7C,  128, -1,      9,       {0} },
	{EFUSE_RESERVED_NAME,    0x90,  896, -1,      -1,      {0} },
	{"",                     0,     0,   0,       0,       {0} }
};

#elif defined(CONFIG_ARCH_SUN50IW1) \
	|| defined(CONFIG_ARCH_SUN50IW2) \
	|| defined(CONFIG_ARCH_SUN50IW3) \
	|| defined(CONFIG_ARCH_SUN50IW6)

#define EFUSE_CTRL_REG  0xFC

static struct efuse_key_map key_maps[] = {
/*   Name                    Offset Size ReadFlag BurnFlag Reserve */
	{EFUSE_CHIPID_NAME,      0x0,   128, -1,      2,       {0} },
	{EFUSE_OEM_NAME,         0x10,  32,  -1,      -1,      {0} },
	{EFUSE_RSAKEY_HASH_NAME, 0x20,  160, -1,      8,       {0} },
	{EFUSE_THM_SENSOR_NAME,  0x34,  64,  -1,      -1,      {0} },
	{EFUSE_RENEW_NAME,       0x3C,  64,  -1,      -1,      {0} },
	{EFUSE_IN_NAME,          0x44,  192, 17,      9,       {0} },
	{EFUSE_OPT_ID_NAME,      0x5C,  32,  18,      10,      {0} },
	{EFUSE_ID_NAME,          0x60,  32,  19,      11,      {0} },
	{EFUSE_ROTPK_NAME,       0x64,  256, 14,      3,       {0} },
	{EFUSE_SSK_NAME,         0x84,  128, 15,      4,       {0} },
	{EFUSE_RSSK_NAME,        0x94,  256, 16,      5,       {0} },
	{EFUSE_HDCP_HASH_NAME,   0xB4,  128, -1,      6,       {0} },
	{EFUSE_EK_HASH_NAME,     0xC4,  128, -1,      7,       {0} },
	{EFUSE_SN_NAME,          0xD4,  192, 20,      12,      {0} },
	{EFUSE_BACKUP_KEY_NAME,  0xEC,  64,  21,      22,      {0} },
	{"",                     0,     0,   0,       0,       {0} }
};

#else

static struct efuse_key_map key_maps[] = {
/*   Name                    Offset Size ReadFlag BurnFlag Reserve */
	{EFUSE_CHIPID_NAME,      0x0,   128, -1,      -1,       {0} },
	{"",                     0,     0,   0,       0,        {0} }
};

#endif

#endif /* end of __SUNXI_SID_EFUSE_H__ */


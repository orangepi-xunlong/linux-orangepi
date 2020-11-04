/*
 * nand_dev.h for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __NAND_DEV_H__
#define __NAND_DEV_H__

struct nand_kobject {
	struct kobject kobj;
	struct _nftl_blk *nftl_blk;
	char name[32];
};

extern unsigned int do_static_wear_leveling(void *zone);
extern unsigned int static_wear_leveling(void *zone);
extern unsigned short nftl_get_zone_write_cache_nums(void *_zone);
extern unsigned int garbage_collect(void *zone);
extern unsigned int do_prio_gc(void *zone);

#endif


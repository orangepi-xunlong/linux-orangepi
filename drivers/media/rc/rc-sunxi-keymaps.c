/* Sunxi Remote Controller
 *
 * keymap imported from ir-keymaps.c
 *
 * Copyright (c) 2014 by allwinnertech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/rc-map.h>
#include "sunxi-ir-rx.h"

static u32 match_addr[MAX_ADDR_NUM];
static u32 match_num;

static struct rc_map_table sunxi_nec_scan[] = {
	{ KEY_ESC, KEY_ESC },
};

static u32 sunxi_key_mapping(u32 code)
{
	u32 i,temp;
	temp = (code >> 8)&0xffff;
	for(i = 0; i < match_num; i++){
		if(match_addr[i] == temp)
			return code;
	}

	return KEY_RESERVED;
}

static struct rc_map_list sunxi_map = {
	.map = {
		.scan    = sunxi_nec_scan,
		.size    = ARRAY_SIZE(sunxi_nec_scan),
		.mapping = sunxi_key_mapping,
		.rc_type = RC_TYPE_NEC,	/* Legacy IR type */
		.name    = RC_MAP_SUNXI,
	}
};

static void init_addr(u32 *addr, u32 addr_num)
{
	u32 *temp_addr = match_addr;
	if(addr_num > MAX_ADDR_NUM)
		addr_num = MAX_ADDR_NUM;
	match_num = addr_num;
	while(addr_num--){
		*temp_addr++ = (*addr++)&0xffff;
	}
	return;
}

int init_rc_map_sunxi(u32 *addr, u32 addr_num)
{
	init_addr(addr,addr_num);
	return rc_map_register(&sunxi_map);
}

void exit_rc_map_sunxi(void)
{
	rc_map_unregister(&sunxi_map);
}


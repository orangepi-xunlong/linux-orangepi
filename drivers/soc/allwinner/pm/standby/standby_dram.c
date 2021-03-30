/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include "standby.h"
#include "../pm_config.h"

s32 standby_dram_crc_enable(pm_dram_para_t *pdram_state)
{
	return pdram_state->crc_en;
}

u32 standby_dram_crc(pm_dram_para_t *pdram_state)
{
	u32 *pdata = (u32 *)0;
	u32 crc = 0;
	u32 start = 0;
	pdata = (u32 *)((pdram_state->crc_start) + 0x80000000);
	start = (u32)pdata;
	printk("src:%x len:%x\n", start, pdram_state->crc_len);
	while (pdata < (u32 *)(start + pdram_state->crc_len)) {
		crc += *pdata;
		pdata++;
	}
	printk("crc finish...\n");

	return crc;
}

/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include "pm_types.h"
#include "pm_i.h"

static u32 *sram_pbase;
/*
***************************************************************************
*                                       MEM SRAM INIT
*
* Description: mem sram init.
*
* Arguments  : none.
*
* Returns    : 0/-1;
***************************************************************************
*/
__s32 mem_sram_init(void)
{
#ifdef CONFIG_FPGA_V4_PLATFORM
	sram_pbase = AW_SRAMCTRL_BASE;
#else
	u32 *base = 0;
	u32 sram_len = 0;
#ifndef CONFIG_ARCH_SUN8IW11P1
	pm_get_dev_info("sram_ctrl", 0, &base, &sram_len);
#else
	pm_get_dev_info("sram-controller", 0, &base, &sram_len);
#endif
	sram_pbase = base;
#endif
	return 0;
}

/*
***************************************************************************
*                                       MEM SRAM SAVE
*
* Description: mem sram save.
*
* Arguments  : none.
*
* Returns    : 0/-1;
***************************************************************************
*/
__s32 mem_sram_save(struct sram_state *psram_state)
{
	int i = 0;

	/*save all the sram reg */
	for (i = 0; i < (SRAM_REG_LENGTH); i++) {
		psram_state->sram_reg_back[i] =
		    *(volatile __u32 *)((sram_pbase) + i);
	}
	return 0;
}

/*
***************************************************************************
*                                       MEM sram restore
*
* Description: mem sram restore.
*
* Arguments  : none.
*
* Returns    : 0/-1;
***************************************************************************
*/
__s32 mem_sram_restore(struct sram_state *psram_state)
{
	int i = 0;

	/*restore all the sram reg */
	for (i = 0; i < (SRAM_REG_LENGTH); i++) {
		*(volatile __u32 *)((sram_pbase) + i) =
		    psram_state->sram_reg_back[i];
	}

	return 0;
}

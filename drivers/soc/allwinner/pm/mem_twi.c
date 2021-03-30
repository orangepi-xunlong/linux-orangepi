/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include "pm_i.h"

static __twic_reg_t *TWI_REG_BASE[3] = {
	(__twic_reg_t *) IO_ADDRESS(AW_TWI0_BASE),
	(__twic_reg_t *) IO_ADDRESS(AW_TWI1_BASE),
	(__twic_reg_t *) IO_ADDRESS(AW_TWI2_BASE)
};

static __twic_reg_t *twi_reg = (__twic_reg_t *) (0);
static __twic_reg_t twi_state_backup;

/*
****************************************************************************
*                                   mem_twi_save
*
*Description: save twi status before enter super standby.
*
*Arguments  :
*
*Return     :
*
****************************************************************************
*/
__s32 mem_twi_save(int group)
{
	twi_reg = TWI_REG_BASE[group];
	twi_state_backup.reg_saddr = twi_reg->reg_saddr;
	twi_state_backup.reg_xsaddr = twi_reg->reg_xsaddr;
	twi_state_backup.reg_data = twi_reg->reg_data;
	twi_state_backup.reg_ctl = twi_reg->reg_ctl;
	twi_state_backup.reg_clkr = twi_reg->reg_clkr;
	twi_state_backup.reg_efr = twi_reg->reg_efr;
	twi_state_backup.reg_lctl = twi_reg->reg_lctl;
	return 0;
}

/*
****************************************************************************
*                                   mem_twi_restore
*
*Description: restore twi status after resume.
*
*Arguments  :
*
*Return     :
*
****************************************************************************
*/
__s32 mem_twi_restore(void)
{
	/* softreset twi module  */
	twi_reg->reg_reset |= 0x1;
	/* restore */
	twi_reg->reg_saddr              =       twi_state_backup.reg_saddr;
	twi_reg->reg_xsaddr             =       twi_state_backup.reg_xsaddr;
	twi_reg->reg_data               =       twi_state_backup.reg_data;
	twi_reg->reg_ctl                =       twi_state_backup.reg_ctl;
	twi_reg->reg_clkr               =       twi_state_backup.reg_clkr;
	twi_reg->reg_efr                =       twi_state_backup.reg_efr;
	twi_reg->reg_lctl               =       twi_state_backup.reg_lctl;
	return 0;
}

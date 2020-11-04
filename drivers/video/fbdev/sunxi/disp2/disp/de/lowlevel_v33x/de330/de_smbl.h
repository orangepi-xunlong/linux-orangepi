/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/**
 *	All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *	File name   :       de_smbl.h
 *
 *	Description :       display engine 2.0 smbl basic function declaration
 *
 *	History     :       2014/05/15  vito cheng  v0.1  Initial version
 *
 */

#ifndef _DE_SMBL_H_
#define _DE_SMBL_H_

#include "de_rtmx.h"
#include "de_smbl_type.h"

s32 de_smbl_tasklet(u32 sel);
s32 de_smbl_apply(u32 sel, struct disp_smbl_info *info);
s32 de_smbl_update_regs(u32 sel);
s32 de_smbl_sync(u32 sel);
s32 de_smbl_get_status(u32 sel);
s32 de_smbl_init(uintptr_t reg_base);
s32 de_smbl_exit(void);
s32 de_smbl_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);

extern u16 pwrsv_lgc_tab[1408][256];
extern u8 smbl_filter_coeff[272];
extern u8 hist_thres_drc[8];
extern u8 hist_thres_pwrsv[8];
extern u8 drc_filter[IEP_LH_PWRSV_NUM];
extern u32 csc_bypass_coeff[12];

#endif /* #ifndef _DE_SMBL_H_ */

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2022 Copyright (c)
 *
 *  File name   :       de_dlc_type.h
 *
 *  Description :       display engine 35x basic function declaration
 *
 *  History     :       2021/10/20  v0.1  Initial version
 *
 ******************************************************************************/

#ifndef _DE_DLC_TYPE_H_
#define _DE_DLC_TYPE_H_

#include "linux/types.h"

/*offset:0x0000*/
union dlc_ctl_reg {
	u32 dwval;
	struct {
		u32 en:1;
		u32 res0:31;
	} bits;
};

/*offset:0x0004*/
union dlc_size_reg {
	u32 dwval;
	struct {
		u32 width:13;
		u32 res0:3;
		u32 height:13;
		u32 res1:3;
	} bits;
};

/*offset:0x0008*/
union dlc_in_color_range_reg {
	u32 dwval;
	struct {
		u32 input_color_space:1;
		u32 res0:31;
	} bits;
};

/*offset:0x10 + bin*4*/
union dlc_pdf_cfg_reg {
	u32 dwval;
	struct {
		u32 coef:10;
		u32 res0:22;
	} bits;
};

struct dlc_reg {
    /*0x0000*/
    union dlc_ctl_reg ctl;
    /*0x0004*/
    union dlc_size_reg size;
    /*0x0008*/
    union dlc_in_color_range_reg color_range;
    /*0x000c*/
    u32 res0;
    /*0x0010*/
    union dlc_pdf_cfg_reg pdf_config[32];
};

#endif /* #ifndef _DE_DLC_TYPE_H_ */

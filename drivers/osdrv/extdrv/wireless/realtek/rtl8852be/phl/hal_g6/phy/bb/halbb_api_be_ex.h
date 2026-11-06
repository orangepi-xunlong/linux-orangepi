/******************************************************************************
 *
 * Copyright(c) 2020 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef _HALBB_API_BE_EX_H_
#define _HALBB_API_BE_EX_H_

/*@--------------------------[Define] ---------------------------------------*/

/*@--------------------------[Enum]------------------------------------------*/

/*@--------------------------[Structure]-------------------------------------*/

/*@--------------------------[Prptotype]-------------------------------------*/
struct bb_info;

u16 halbb_pri20_bitmap(struct bb_info *bb, u8 central_ch, u8 pri_ch,
		       enum channel_width bw);

u16 halbb_preamble_puncture(struct bb_info *bb, u8 central_ch, u8 pri_ch,
			    enum channel_width cbw, enum channel_width dbw,
			    u8 punc_ch_info);

#endif
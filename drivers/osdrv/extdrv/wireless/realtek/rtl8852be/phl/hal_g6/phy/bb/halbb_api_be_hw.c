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
#include "halbb_precomp.h"

#ifdef HALBB_COMPILE_BE_SERIES

u8 halbb_get_prim_sb_be(struct bb_info *bb, u8 central_ch, u8 pri_ch,
			enum channel_width bw)
{
	u8 prim_sb = 0;
	u8 prisb_cal_ofst[5] = {0, 2, 6, 14, 30};

	prim_sb = (prisb_cal_ofst[bw] + pri_ch - central_ch) / 4;

	return prim_sb;
}

u16 halbb_pri20_bitmap_be(struct bb_info *bb, u8 central_ch, u8 pri_ch,
			  enum channel_width bw)
{
	// Indicate which sub20 is primary channel
	u8 prim_sb = halbb_get_prim_sb_be(bb, central_ch, pri_ch, bw);

	BB_DBG(bb, DBG_PHY_CONFIG_BE,
	       "[%s] central_ch=%d, pri_ch=%d, bw=%d, prim_sb=%d\n",
	       __func__, central_ch, pri_ch, bw, prim_sb);

	return ((u16)1 << prim_sb);
}

u16 halbb_ch20_with_data_lut_be(struct bb_info *bb, enum channel_width dbw,
				u8 punc_ch_info)
{
	bool valid = false;
	u16 output = 0;

	BB_DBG(bb, DBG_PHY_CONFIG_BE, "[%s] dbw=%d, punc_ch_info=%d\n",
	       __func__, dbw, punc_ch_info);

	// Error handling
	switch (dbw) {
	case CHANNEL_WIDTH_20:
	case CHANNEL_WIDTH_40:
		valid = (punc_ch_info == 0) ? true : false;
		output = 0;
		break;
	case CHANNEL_WIDTH_80:
		if (punc_ch_info <= 4) {
			output = 0xf & ~(0x1 << (punc_ch_info - 1));
			valid = true;
		} else {
			valid = false;
		}
		break;
	case CHANNEL_WIDTH_160:
		if (punc_ch_info <= 8) {
			output = 0xff & ~(0x1 << (punc_ch_info - 1));
			valid = true;
		} else if (punc_ch_info <= 12) {
			output = 0xff & ~(0x3 << ((punc_ch_info - 9) * 2));
			valid = true;
		} else {
			valid = false;
		}
		break;
	case CHANNEL_WIDTH_320:
		if (punc_ch_info <= 8) {
			output = 0xffff & ~(u16)(0x3 << ((punc_ch_info - 1) * 2));
			valid = true;
		} else if (punc_ch_info <= 12) {
			output = 0xffff & ~(u16)(0xf << ((punc_ch_info - 9) * 4));
			valid = true;
		} else if (punc_ch_info <= 18) {
			output = 0xfff0 & ~(u16)(0x3 << (((punc_ch_info - 13) * 2) + 4));
			valid = true;
		} else if (punc_ch_info <= 24) {
			output = 0xfff & ~(u16)(0x3 << ((punc_ch_info - 19) * 2));
			valid = true;
		} else {
			valid = false;
		}
		break;
	default:
		valid = false;
	}

	if (!valid) {
		BB_WARNING("Invalid dbw & punc_ch_info !\n");
		return 0;
	}

	BB_DBG(bb, DBG_PHY_CONFIG_BE, "[%s] output=0x%x\n", __func__, output);

	return output;
}

u16 halbb_preamble_puncture_be(struct bb_info *bb, u8 central_ch, u8 pri_ch,
			       enum channel_width cbw, enum channel_width dbw,
			       u8 punc_ch_info)
{
	u8 prim_sb = halbb_get_prim_sb(bb, central_ch, pri_ch, cbw);
	u16 ch20_with_data = halbb_ch20_with_data_lut_be(bb, dbw, punc_ch_info);
	u16 output = 0;
	u8 bit_shift = 0;

	BB_DBG(bb, DBG_PHY_CONFIG_BE,
	       "[%s] {central_ch, pri_ch, cbw, dbw, punc_ch_info}={%d,%d,%d,%d,0x%x}\n",
	       __func__, central_ch, pri_ch, cbw, dbw, punc_ch_info);

	bit_shift = (1 << dbw) * (prim_sb / (1 << dbw));
	output = ch20_with_data << bit_shift;

	BB_DBG(bb, DBG_PHY_CONFIG_BE,
	       "[%s] {prim_sb, ch20_with_data, output}={%d,0x%x,0x%x}\n",
	       __func__, prim_sb, ch20_with_data, output);

	return output;
}



#endif


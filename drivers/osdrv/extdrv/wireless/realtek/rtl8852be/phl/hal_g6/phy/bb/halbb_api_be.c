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

u8 halbb_get_prim_sb(struct bb_info *bb, u8 central_ch, u8 pri_ch,
		     enum channel_width bw)
{
	u8 output = 0;

	switch (bb->bb_80211spec) {

	#ifdef HALBB_COMPILE_BE_SERIES
	case BB_BE_IC:
		output = halbb_get_prim_sb_be(bb, central_ch, pri_ch, bw);
		break;
	#endif

	default:
		output = 0;
		BB_WARNING("[%s] ic_spec=%d not support\n", __func__,
			   bb->bb_80211spec);
		break;
	}

	return output;
}

u16 halbb_pri20_bitmap(struct bb_info *bb, u8 central_ch, u8 pri_ch,
		       enum channel_width bw)
{
	u16 output = 0;

	switch (bb->bb_80211spec) {

	#ifdef HALBB_COMPILE_BE_SERIES
	case BB_BE_IC:
		output = halbb_pri20_bitmap_be(bb, central_ch, pri_ch, bw);
		break;
	#endif

	default:
		output = 0;
		BB_WARNING("[%s] ic_spec=%d not support\n", __func__,
			   bb->bb_80211spec);
		break;
	}

	return output;
}

u16 halbb_preamble_puncture(struct bb_info *bb, u8 central_ch, u8 pri_ch,
			    enum channel_width cbw, enum channel_width dbw,
			    u8 punc_ch_info)
{
	u16 output = 0;

	switch (bb->bb_80211spec) {

	#ifdef HALBB_COMPILE_BE_SERIES
	case BB_BE_IC:
		output = halbb_preamble_puncture_be(bb, central_ch, pri_ch, cbw,
						    dbw, punc_ch_info);
		break;
	#endif

	default:
		output = 0;
		BB_WARNING("[%s] ic_spec=%d not support\n", __func__,
			   bb->bb_80211spec);
		break;
	}

	return output;
}

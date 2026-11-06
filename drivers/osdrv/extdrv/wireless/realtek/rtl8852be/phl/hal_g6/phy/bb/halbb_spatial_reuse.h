/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2021, Realtek Semiconductor Corp. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   * Neither the name of the Realtek nor the names of its contributors may
 *     be used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __HALBB_SPATIAL_REUSE_H__
#define __HALBB_SPATIAL_REUSE_H__

/*@--------------------------[Define] ---------------------------------------*/
#define NON_SRG_OBSS_PD_MIN (-82)	// dbm
#define NON_SRG_OBSS_PD_MAX (-62)
#define SRG_OBSS_PD_MIN (-82)
#define SRG_OBSS_PD_MAX (-62)
#define SR_TXPWR_REF (21)

/*@--------------------------[Enum]------------------------------------------*/

/*@--------------------------[Structure]-------------------------------------*/

struct bb_spatial_reuse_cr_info {
	u32 r_sr_txpwr_ref;
	u32 r_sr_txpwr_ref_m;
};

struct bb_spatial_reuse_info {
	struct rtw_mac_ax_sr_info ax_sr_info;
	struct bb_spatial_reuse_cr_info bb_spatial_reuse_cr_i;
	bool need_update;
	u8 txpwr_ref;
};



struct bb_info;
/*@--------------------------[Prptotype]-------------------------------------*/

void halbb_spatial_reuse(struct bb_info *bb);
void halbb_spatial_reuse_init(struct bb_info *bb);
void halbb_cr_cfg_spatial_reuse_init(struct bb_info *bb);
void halbb_spatial_reuse_dbg(struct bb_info *bb, char input[][16],
							 u32 *_used, char *output, u32 *_out_len);

#endif


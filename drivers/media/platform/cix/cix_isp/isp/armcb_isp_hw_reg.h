/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ARMCB_ISP_HW_REG_H_
#define _ARMCB_ISP_HW_REG_H_

#include <linux/types.h>

typedef union {
	struct {
		u32 rsvd : 8;
		u32 r_adb_frm_idx : 2;
		u32 r_adb_cxt_idx : 2;
		u32 r_gmd_frm_idx : 2;
		u32 r_gmd_cxt_idx : 2;
		u32 r_rir_frm_idx0 : 2;
		u32 r_rir_cxt_idx0 : 2;
		u32 r_rir_frm_idx1 : 2;
		u32 r_rir_cxt_idx1 : 2;
		u32 r_3a_cnt_sel_frm_idx : 2;
		u32 r_3a_cnt_sel_cxt_idx : 2;
		u32 r_isp_cnt_sel_frm_idx : 2;
		u32 r_isp_cnt_sel_cxt_idx : 2;
	} __packed __aligned(4);
	u32 val;
} i7_top_top_reg_44_t;

typedef union {
	struct {
		u32 r_vin_in_ch_idx : 2;
		u32 rsvd : 30;
	} __packed __aligned(4);
	u32 val;
} i7_top_top_reg_4c_t;

typedef union {
	struct {
		u32 r_sensor_sel : 2;
		u32 r_vin_sof_sel : 1;
		u32 rsvd : 29;
	} __packed __aligned(4);
	u32 val;
} i7_top_top_reg_50_t;

typedef union {
	struct {
		u32 cxt_idx : 2;
		u32 frm_idx : 2;
		u32 cxt_nxt_idx : 2;
		u32 frm_nxt_idx : 2;
		u32 aaa_cxt_idx : 2;
		u32 aaa_frm_idx : 2;
		u32 rsvd : 20;
	} __packed __aligned(4);
	u32 val;
} i7_top_top_reg_c0_t;

typedef union {
	struct {
		u32 r_vin_cfg_done : 1;
		u32 r_vin_ddr_process_start : 1;
		u32 r_vin_frm_duration_stat_mode : 2;
		u32 r_vin_phase_diff_outsel : 2;
		u32 r_vin_phase_diff_cnt_mode : 1;
		u32 r_vin_daw_frm_cnt_chm_sel : 2;
		u32 rsvd : 23;
	} __packed __aligned(4);
	u32 val;
} i7_vin_vin_reg_c8_t;

typedef union {
	struct {
		u32 sensor_id : 2;
		u32 frame_index : 2;
		u32 vout_id : 2;
		u32 rsvd : 26;
	} __packed __aligned(4);
	u32 val;
} i5_top_top_reg_68_t;

#endif

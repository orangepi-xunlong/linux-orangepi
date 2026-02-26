/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * function define of edp lowlevel function
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __EDP_LOWLEVEL_H__
#define __EDP_LOWLEVEL_H__

#include "../edp_core/edp_core.h"
#include "../../include/disp_edid.h"
#include <video/sunxi_edp.h>
#include <linux/types.h>
#include <linux/printk.h>

#define SETMASK(width, shift)   ((width?((-1U) >> (32-width)):0)  << (shift))
#define CLRMASK(width, shift)   (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
	(((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
	(((reg) & CLRMASK(width, shift)) | (val << (shift)))

#define NATIVE_READ   0b1001
#define NATIVE_WRITE  0b1000
#define AUX_I2C_READ  0b0001
#define AUX_I2C_WRITE 0b0000

#define AUX_REPLY_DEFER 0b0010
#define AUX_REPLY_NACK  0b0001
#define AUX_REPLY_ACK   0b0000
#define AUX_REPLY_I2C_DEFER 0b1000
#define AUX_REPLY_I2C_NACK  0b0100
#define AUX_REPLY_I2C_ACK   0b0000

#define EDP_EDID_ADDR 0x50

#if IS_ENABLED(CONFIG_AW_EDP_PHY_USED)
u64 edp_hal_get_max_rate(u32 sel);
u32 edp_hal_get_max_lane(u32 sel);

bool edp_hal_support_tps3(u32 sel);
bool edp_hal_support_audio(u32 sel);
bool edp_hal_support_psr(u32 sel);
bool edp_hal_support_assr(u32 sel);
bool edp_hal_support_ssc(u32 sel);
bool edp_hal_support_mst(u32 sel);

bool edp_hal_ssc_is_enabled(u32 sel);
bool edp_hal_psr_is_enabled(u32 sel);
bool edp_hal_audio_is_enabled(u32 sel);
bool edp_hal_support_fast_train(u32 sel);
bool edp_hal_support_tps3(u32 sel);
bool edp_hal_get_hotplug_change(u32 sel);

void edp_hal_set_reg_base(u32 sel, uintptr_t base);
void edp_hal_lane_config(u32 sel, struct edp_tx_core *edp_core);
void edp_hal_set_misc(u32 sel, s32 misc0_val, s32 misc1_val);
void edp_hal_clean_hpd_interrupt_status(u32 sel);
void edp_hal_show_builtin_patten(u32 sel, u32 pattern);
void edp_hal_irq_handler(u32 sel, struct edp_tx_core *edp_core);
void edp_hal_set_training_pattern(u32 sel, u32 pattern);
void edp_hal_set_lane_sw_pre(u32 sel, u32 lane_id, u32 sw, u32 pre, u32 param_type);
void edp_hal_set_lane_rate(u32 sel, u64 bit_rate);
void edp_hal_set_lane_cnt(u32 sel, u32 lane_cnt);

s32 edp_hal_query_transfer_unit(u32 sel, struct edp_tx_core *edp_core,
								struct disp_video_timings *tmgs);
s32 edp_hal_init_early(u32 sel);
s32 edp_hal_phy_init(u32 sel, struct edp_tx_core *edp_core);
s32 edp_hal_enable(u32 sel, struct edp_tx_core *edp_core);
s32 edp_hal_disable(u32 sel, struct edp_tx_core *edp_core);
s32 edp_hal_read_edid(u32 sel, struct edid *edid);
s32 edp_hal_read_edid_ext(u32 sel, u32 block_cnt, struct edid *edid);
s32 edp_hal_sink_init(u32 sel, u64 bit_rate, u32 lane_cnt);
s32 edp_hal_irq_enable(u32 sel, u32 irq_id);
s32 edp_hal_irq_disable(u32 sel, u32 irq_id);
s32 edp_hal_irq_query(u32 sel);
s32 edp_hal_irq_clear(u32 sel);
s32 edp_hal_get_cur_line(u32 sel);
s32 edp_hal_get_start_dly(u32 sel);
s32 edp_hal_aux_read(u32 sel, s32 addr, s32 len, char *buf, bool retry);
s32 edp_hal_aux_write(u32 sel, s32 addr, s32 len, char *buf, bool retry);
s32 edp_hal_aux_i2c_read(u32 sel, s32 addr, s32 len, char *buf, bool retry);
s32 edp_hal_aux_i2c_write(u32 sel, s32 addr, s32 len, char *buf, bool retry);
s32 edp_hal_link_start(u32 sel);
s32 edp_hal_link_stop(u32 sel);
s32 edp_hal_audio_set_para(u32 sel, edp_audio_t *para);
s32 edp_hal_audio_enable(u32 sel);
s32 edp_hal_audio_disable(u32 sel);
s32 edp_hal_ssc_enable(u32 sel, bool enable);
s32 edp_hal_ssc_set_mode(u32 sel, u32 mode);
s32 edp_hal_ssc_get_mode(u32 sel);
s32 edp_hal_psr_enable(u32 sel, bool enable);
s32 edp_hal_assr_enable(u32 sel, bool enable);
s32 edp_hal_get_color_fmt(u32 sel);
s32 edp_hal_set_pattern(u32 sel, u32 pattern);
s32 edp_hal_get_pixclk(u32 sel);
s32 edp_hal_get_train_pattern(u32 sel);
s32 edp_hal_get_lane_para(u32 sel, struct edp_lane_para *tmp_lane_para);
s32 edp_hal_get_tu_size(u32 sel);
s32 edp_hal_get_valid_symbol_per_tu(u32 sel);
s32 edp_hal_get_audio_if(u32 sel);
s32 edp_hal_audio_is_mute(u32 sel);
s32 edp_hal_get_audio_chn_cnt(u32 sel);
s32 edp_hal_get_audio_date_width(u32 sel);
s32 edp_hal_set_video_format(u32 sel, struct edp_tx_core *edp_core);
s32 edp_hal_set_video_timings(u32 sel, struct disp_video_timings *tmgs);
s32 edp_hal_set_transfer_config(u32 sel, struct edp_tx_core *edp_core);
s32 edp_hal_get_hotplug_state(u32 sel);

#else
static inline s32 edp_hal_init_early(u32 sel)
{
	EDP_ERR("aw edp phy is not selected yet!\n");
	return RET_FAIL;
}

static inline s32 edp_hal_query_transfer_unit(u32 sel, struct edp_tx_core *edp_core,
											  struct disp_video_timings *tmgs) { return RET_FAIL; }

static inline u64 edp_hal_get_max_rate(u32 sel) { return 0; }
static inline u32 edp_hal_get_max_lane(u32 sel) { return 0; }

static inline bool edp_hal_support_tps3(u32 sel) { return false; }
static inline bool edp_hal_support_audio(u32 sel) { return false; }
static inline bool edp_hal_support_psr(u32 sel) { return false; }
static inline bool edp_hal_support_assr(u32 sel) { return false; }
static inline bool edp_hal_support_ssc(u32 sel) { return false; }
static inline bool edp_hal_support_mst(u32 sel) { return false; }


static inline bool edp_hal_ssc_is_enabled(u32 sel) { return false; }
static inline bool edp_hal_psr_is_enabled(u32 sel) { return false; }
static inline bool edp_hal_audio_is_enabled(u32 sel) { return false; }
static inline bool edp_hal_support_fast_train(u32 sel) { return false; }
static inline bool edp_hal_support_tps3(u32 sel) { return false; }
static inline bool edp_hal_get_hotplug_change(void) { return false; }

static inline void edp_hal_set_reg_base(u32 sel, uintptr_t base) {}
static inline void edp_hal_lane_config(u32 sel, struct edp_tx_core *edp_core) {}
static inline void edp_hal_set_misc(u32 sel, s32 misc0_val, s32 misc1_val) {}
static inline void edp_hal_clean_hpd_interrupt_status(u32 sel) {}
static inline void edp_hal_show_builtin_patten(u32 sel, u32 pattern) {}
static inline void edp_hal_irq_handler(u32 sel, struct edp_tx_core *edp_core) {}
static inline void edp_hal_set_training_pattern(u32 sel, u32 pattern) {}
static inline void edp_hal_set_lane_sw_pre(u32 sel, u32 lane_id, u32 sw, u32 pre, u32 param_type) {}
static inline void edp_hal_set_lane_rate(u32 sel, u64 bit_rate) {}
static inline void edp_hal_set_lane_cnt(u32 sel, u32 lane_cnt) {}

static inline s32 edp_hal_phy_init(u32 sel, struct edp_tx_core *edp_core) { return RET_FAIL; }
static inline s32 edp_hal_enable(u32 sel, struct edp_tx_core *edp_core) { return RET_FAIL; }
static inline s32 edp_hal_disable(u32 sel, struct edp_tx_core *edp_core) { return RET_FAIL; }
static inline s32 edp_hal_read_edid(u32 sel, struct edid *edid) { return RET_FAIL; }
static inline s32 edp_hal_read_edid_ext(u32 sel, u32 block_cnt, struct edid *edid) { return RET_FAIL; }
static inline s32 edp_hal_sink_init(u32 sel, u64 bit_rate, u32 lane_cnt) { return RET_FAIL; }
static inline s32 edp_hal_irq_enable(u32 sel, u32 irq_id) { return RET_FAIL; }
static inline s32 edp_hal_irq_disable(u32 sel, u32 irq_id) { return RET_FAIL; }
static inline s32 edp_hal_irq_query(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_irq_clear(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_get_cur_line(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_get_start_dly(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_aux_read(u32 sel, s32 addr, s32 len, char *buf, bool retry) { return RET_FAIL; }
static inline s32 edp_hal_aux_write(u32 sel, s32 addr, s32 len, char *buf, bool retry) { return RET_FAIL; }
static inline s32 edp_hal_aux_i2c_read(u32 sel, s32 addr, s32 len, char *buf, bool retry) { return RET_FAIL; }
static inline s32 edp_hal_aux_i2c_write(u32 sel, s32 addr, s32 len, char *buf, bool retry) { return RET_FAIL; }
static inline s32 edp_hal_link_start(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_link_stop(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_audio_set_para(u32 sel, edp_audio_t *para) { return RET_FAIL; }
static inline s32 edp_hal_audio_enable(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_audio_disable(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_ssc_enable(u32 sel, bool enable) { return RET_FAIL; }
static inline s32 edp_hal_ssc_set_mode(u32 sel, u32 mode) { return RET_FAIL; }
static inline s32 edp_hal_ssc_get_mode(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_psr_enable(u32 sel, bool enable) { return RET_FAIL; }
static inline s32 edp_hal_assr_enable(u32 sel, bool enable) { return RET_FAIL; }
static inline s32 edp_hal_get_color_fmt(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_set_pattern(u32 sel, u32 pattern) { return RET_FAIL; }
static inline s32 edp_hal_get_pixclk(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_get_train_pattern(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_get_lane_para(u32 sel, struct edp_lane_para *tmp_lane_para) { return RET_FAIL; }
static inline s32 edp_hal_get_tu_size(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_get_valid_symbol_per_tu(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_get_audio_if(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_audio_is_mute(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_get_audio_chn_cnt(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_get_audio_date_width(u32 sel) { return RET_FAIL; }
static inline s32 edp_hal_set_video_format(u32 sel, struct edp_tx_core *edp_core) { return RET_FAIL; }
static inline s32 edp_hal_set_video_timings(u32 sel, struct disp_video_timings *tmgs) { return RET_FAIL; }
static inline s32 edp_hal_set_transfer_config(u32 sel, struct edp_tx_core *edp_core) { return RET_FAIL; }
static inline s32 edp_hal_get_hotplug_state(u32 sel) { return RET_FAIL; }
#endif

#endif

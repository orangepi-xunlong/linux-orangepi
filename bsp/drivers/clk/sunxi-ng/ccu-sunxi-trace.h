// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM sunxi_ccu

#if !defined(__CCU_SUNXI_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define __CCU_SUNXI_TRACE_H
#include <linux/tracepoint.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_mult.h"
#include "ccu_nk.h"
#include "ccu_nkm.h"
#include "ccu_nkmp.h"
#include "ccu_nm.h"

DECLARE_EVENT_CLASS(clk_set_rate,

	TP_PROTO(struct clk_hw *hw, unsigned long _rate, unsigned long parent_rate, u32 reg),

	TP_ARGS(hw, _rate, parent_rate, reg),

	TP_STRUCT__entry(
		__string(name, clk_hw_get_name(hw))
		__field(unsigned long, _rate)
		__field(unsigned long, parent_rate)
		__field(u32, reg)
	),

	TP_fast_assign(
		__assign_str(name, clk_hw_get_name(hw));
		__entry->_rate = _rate;
		__entry->parent_rate = parent_rate;
		__entry->reg = reg;
	),

	TP_printk("clk_name:%s, clk_rate: %ld, clk_parent_rate: %ld register_value: 0x%x", __get_str(name), __entry->_rate, __entry->parent_rate, __entry->reg)
);

DEFINE_EVENT(clk_set_rate, clk_nkmp_set_rate,

	TP_PROTO(struct clk_hw *hw, unsigned long _rate, unsigned long parent_rate, u32 reg),

	TP_ARGS(hw, _rate, parent_rate, reg)
);

DEFINE_EVENT(clk_set_rate, clk_mp_set_rate,

	TP_PROTO(struct clk_hw *hw, unsigned long _rate, unsigned long parent_rate, u32 reg),

	TP_ARGS(hw, _rate, parent_rate, reg)
);

DEFINE_EVENT(clk_set_rate, clk_nkm_set_rate,

	TP_PROTO(struct clk_hw *hw, unsigned long _rate, unsigned long parent_rate, u32 reg),

	TP_ARGS(hw, _rate, parent_rate, reg)
);

DECLARE_EVENT_CLASS(clk_round_rate,

	TP_PROTO(struct clk_hw *hw, unsigned long _rate, u64 rate),

	TP_ARGS(hw, _rate, rate),

	TP_STRUCT__entry(
		__string(name, clk_hw_get_name(hw))
		__field(unsigned long, _rate)
		__field(u64, rate)
	),

	TP_fast_assign(
		__assign_str(name, clk_hw_get_name(hw));
		__entry->_rate = _rate;
		__entry->rate = rate;
	),

	TP_printk("clk_name:%s, request_clk_rate: %ld, round_rate: %lld ", __get_str(name), __entry->_rate, __entry->rate)
);

DEFINE_EVENT(clk_round_rate, clk_nk_round_rate,

	TP_PROTO(struct clk_hw *hw, unsigned long _rate, u64 rate),

	TP_ARGS(hw, _rate, rate)
);
TRACE_EVENT(clk_ng_enable,
	TP_PROTO(struct clk_hw *hw),

	TP_ARGS(hw),

	TP_STRUCT__entry(
		__string(name, clk_hw_get_name(hw))
	),

	TP_fast_assign(
		__assign_str(name, clk_hw_get_name(hw));
	),

	TP_printk("clk_enable: %s", __get_str(name))
);

TRACE_EVENT(clk_ng_set_parent,
	TP_PROTO(struct clk_hw *hw, u8 index),

	TP_ARGS(hw, index),

	TP_STRUCT__entry(
		__string(name, clk_hw_get_name(hw))
		__field(u8, index)
	),

	TP_fast_assign(
		__assign_str(name, clk_hw_get_name(hw));
		__entry->index = index;
	),

	TP_printk("clk_name: %s parent_index: %d", __get_str(name), __entry->index)
);
#endif

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE ccu-sunxi-trace
#include <trace/define_trace.h>

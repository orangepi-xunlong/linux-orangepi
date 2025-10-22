/* SPDX-License-Identifier: GPL-2.0-or-later */
/****************************************************************************
*
*	The MIT License (MIT)
*
*	Copyright (c) 2017 - 2021 Vivante Corporation
*
*	Permission is hereby granted, free of charge, to any person obtaining a
*	copy of this software and associated documentation files (the "Software"),
*	to deal in the Software without restriction, including without limitation
*	the rights to use, copy, modify, merge, publish, distribute, sublicense,
*	and/or sell copies of the Software, and to permit persons to whom the
*	Software is furnished to do so, subject to the following conditions:
*
*	The above copyright notice and this permission notice shall be included in
*	all copies or substantial portions of the Software.
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*	DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*	The GPL License (GPL)
*
*	Copyright (C) 2017 - 2021 Vivante Corporation
*
*	This program is free software; you can redistribute it and/or
*	modify it under the terms of the GNU General Public License
*	as published by the Free Software Foundation; either version 2
*	of the License, or (at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program;
*
*****************************************************************************
*
*	Note: This software is released under dual MIT and GPL licenses. A
*	recipient may use this file under the terms of either the MIT license or
*	GPL License. If you wish to use only one license not the other, you can
*	indicate your decision by deleting one of the above license notices in your
*	version of this file.
*
*****************************************************************************/

#include <linux/devfreq.h>
#include <linux/mutex.h>

typedef struct _aw_driver_t {
	struct reset_control *rst;
	struct clk *mclk;
	struct clk *pclk;
	struct clk *bus;
	struct clk *mbus;
	struct clk *mbus_gate;
	struct clk *ahb_gate;
	struct clk *tzma;
	struct reset_control *arst;
	struct reset_control *hrst;
	struct clk *aclk;
	struct clk *hclk;
	struct regulator *regulator;
	uint32_t vf_index;
	uint32_t dcxo_clk_src;
	uint64_t max_freq;
	uint64_t default_freq;
	uint64_t clk_freq;
	uint32_t set_vol;
	uint64_t freqs[MAX_FREQ_POINTS];
	int vol;
	bool enable_pm;
	struct mutex clk_lock;
#if vpmdENABLE_AW_DEVFREQ
	uint64_t internal_freq;
	uint32_t internal_fscale;
	struct devfreq *devfreq;
	struct devfreq_simple_ondemand_data ondemand_data;
	struct thermal_cooling_device *cooling;
	struct opp_table *reg_opp_table;
#endif
} aw_driver_t;

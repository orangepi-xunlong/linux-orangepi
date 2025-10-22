/* SPDX-License-Identifier: GPL-2.0-or-later */
/****************************************************************************
*
*    Copyright (c) 2005 - 2020 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License version 2 as
*    published by the Free Software Foundation.
*

*****************************************************************************/

typedef struct _aw_driver_t {
	struct reset_control *rst;
	struct clk *mclk;
	struct clk *pclk;
	struct clk *bus;
	struct clk *mbus;
	struct clk *mbus_gate;
	struct clk *ahb_gate;
	struct clk *tzma;
	struct clk *aclk;
	struct clk *hclk;
	struct reset_control *arst;
	struct reset_control *hrst;
	struct regulator *regulator;
	int vol;
	uint32_t vf_index;
} aw_driver_t;

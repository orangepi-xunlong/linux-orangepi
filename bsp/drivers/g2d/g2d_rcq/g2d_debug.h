/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __G2D_DEBUG_H__
#define __G2D_DEBUG_H__
#include "g2d_driver_i.h"

void dump_g2d_bld_info(const g2d_bld *g2d_bld_para);
void dump_mixer_para_info(const mixer_para *mixer_paras);
void dump_g2d_blt_h_info(const g2d_blt_h *g2d_blt_h_para);
void dump_g2d_lbc_rot_info(const g2d_lbc_rot *g2d_lbc_rot_para);
void dump_g2d_fillrect_h_info(const g2d_fillrect_h *g2d_fillrect_h_para);
void dump_g2d_maskblt_info(const g2d_maskblt *g2d_maskblt_para);

#endif  /* __G2D_DEBUG_H__ */

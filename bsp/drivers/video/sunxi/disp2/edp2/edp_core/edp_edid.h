/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * edp_edid.h
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __EDP_EDID_H__
#define __EDP_EDID_H__

#include "edp_core.h"
#include "../../include/disp_edid.h"
#include <linux/types.h>

struct edid *edp_edid_get(u32 sel);
s32 edp_edid_put(struct edid *edid);
u8 *edp_edid_displayid_extension(struct edid *edid);
u8 *edp_edid_cea_extension(struct edid *edid);
s32 edp_edid_cea_revision(u8 *cea);
s32 edp_edid_cea_tag(u8 *cea);
s32 edp_edid_cea_db_offsets(u8 *cea, s32 *start, s32 *end);

#endif

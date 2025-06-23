// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */

#ifndef __BOOT_STAGE_POINT_H__
#define __BOOT_STAGE_POINT_H__

#include <mntn_public_interface.h>
#include <linux/types.h>

void set_boot_keypoint(u32 value);
u32 get_boot_keypoint(void);
u32 get_last_boot_keypoint(void);
#endif

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 Rockchip Electronics Co. Ltd.
 *
 * Author: Shunqing Chen <csq@rock-chips.com>
 */

#ifndef _RK628_V4L2_CONTROLS_H
#define _RK628_V4L2_CONTROLS_H

#include <linux/v4l2-controls.h>

#define V4L2_CID_USER_RK_BASE (V4L2_CID_USER_BASE + 0x1080)

#define RK_V4L2_CID_AUDIO_SAMPLING_RATE (V4L2_CID_USER_RK_BASE + 0x100)
#define RK_V4L2_CID_AUDIO_PRESENT (V4L2_CID_USER_RK_BASE + 0x101)

#endif

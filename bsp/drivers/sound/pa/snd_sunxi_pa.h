/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, lijingpsw <lijingpsw@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_PA_H__
#define __SND_SUNXI_PA_H__

#if IS_ENABLED(CONFIG_SND_SOC_AW87XXX)
int aw87xxx_set_profile(int dev_index, char *profile);
int aw87xxx_add_codec_controls(void *codec);
#endif

#endif

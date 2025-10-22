/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_SFX_H
#define __SND_SUNXI_SFX_H

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
#define SUNXI_AUDIO_SFX_REG		(0x05096000)
#define SUNXI_AUDIO_SFX_REG_SIZE	(0x32c)
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define SUNXI_AUDIO_SFX_REG		(0x07110000)
#define SUNXI_AUDIO_SFX_REG_SIZE	(0x348)
#elif IS_ENABLED(CONFIG_ARCH_SUN300IW1)
#define SUNXI_AUDIO_SFX_REG		(0x42030000)
#define SUNXI_AUDIO_SFX_REG_SIZE	(0x348)
#elif IS_ENABLED(CONFIG_ARCH_SUN251IW1)
#define SUNXI_AUDIO_SFX_REG             (0x02030000)
#define SUNXI_AUDIO_SFX_REG_SIZE        (0x34c)
#endif

#endif /* __SND_SUNXI_SFX_H */

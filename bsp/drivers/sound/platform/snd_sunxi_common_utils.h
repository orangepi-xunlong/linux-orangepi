/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2024, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_COMMON_UTILS_H
#define __SND_SUNXI_COMMON_UTILS_H

int snd_sunxi_pa_user_trig_init(struct snd_sunxi_pacfg *pa_cfg);
void snd_sunxi_pa_user_trig_exit(struct snd_sunxi_pacfg *pa_cfg);
int snd_sunxi_pa_user_trig_probe(struct snd_sunxi_pacfg *pa_cfg, u32 pa_pin_max,
				 struct snd_soc_component *component);
int snd_sunxi_pa_user_trig_enable(struct snd_sunxi_pacfg *pa_cfg);
void snd_sunxi_pa_user_trig_disable(struct snd_sunxi_pacfg *pa_cfg);

#endif /* __SND_SUNXI_COMMON_UTILS_H */

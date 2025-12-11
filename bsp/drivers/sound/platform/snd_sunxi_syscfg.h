/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2025 - 2028 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2025, zhouxijing <zhouxijing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_SYSCFG_H
#define __SND_SUNXI_SYSCFG_H

struct sunxi_syscfg_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

int sunxi_syscfg_probe(struct snd_soc_component *component);
void sunxi_syscfg_remove(struct snd_soc_component *component);
int sunxi_syscfg_suspend(struct snd_soc_component *component);
int sunxi_syscfg_resume(struct snd_soc_component *component);
int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_syscfg_mem *mem);
void snd_sunxi_mem_exit(struct platform_device *pdev, struct sunxi_syscfg_mem *mem);

#endif /* __SND_SUNXI_SYSCFG_H */
/* sound\soc\sunxi\snd_sunxi_common.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_COMMON_H
#define __SND_SUNXI_COMMON_H

/* for regmap */
struct sunxi_mem_info {
	char *dev_name;
	struct resource *res;
	struct regmap_config *regmap_config;

	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

int snd_sunxi_mem_init(struct platform_device *pdev,
		       struct sunxi_mem_info *mem_info);
void snd_sunxi_mem_exit(struct platform_device *pdev,
			struct sunxi_mem_info *mem_info);

/* for reg debug */
#define REG_LABEL(constant)	{#constant, constant, 0}
#define REG_LABEL_END		{NULL, 0, 0}

struct reg_label {
	const char *name;
	const unsigned int address;
	unsigned int value;
};

/* EX:
 * static struct reg_label reg_labels[] = {
 * 	REG_LABEL(SUNXI_REG_0),
 * 	REG_LABEL(SUNXI_REG_1),
 * 	REG_LABEL(SUNXI_REG_n),
 * 	REG_LABEL_END,
 * };
 */
int snd_sunxi_save_reg(struct regmap *regmap, struct reg_label *reg_labels);
int snd_sunxi_echo_reg(struct regmap *regmap, struct reg_label *reg_labels);

/* for pa config */
struct pa_config {
	u32 pin;
	u32 msleep;
	bool used;
	bool level;
};

struct pa_config *snd_sunxi_pa_pin_init(struct platform_device *pdev,
					u32 *pa_pin_max);
void snd_sunxi_pa_pin_exit(struct platform_device *pdev,
			   struct pa_config *pa_cfg, u32 pa_pin_max);
int snd_sunxi_pa_pin_enable(struct pa_config *pa_cfg, u32 pa_pin_max);
void snd_sunxi_pa_pin_disable(struct pa_config *pa_cfg, u32 pa_pin_max);

#endif /* __SND_SUNXI_COMMON_H */
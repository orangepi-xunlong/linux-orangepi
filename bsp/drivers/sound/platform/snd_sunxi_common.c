// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME		"sound-common"
#include "snd_sunxi_log.h"
#include <linux/module.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <sound/soc.h>

#include "snd_sunxi_common.h"
#include "snd_sunxi_common_utils.h"
#include "snd_sunxi_adapter.h"

/******* jack_state *******/
static int sunxi_jack_state;

void snd_sunxi_jack_state_upto_modparam(enum snd_jack_types type)
{
	sunxi_jack_state = type;
}

module_param_named(jack_state, sunxi_jack_state, int, S_IRUGO | S_IWUSR);

/******* reg label *******/
int snd_sunxi_save_reg(struct regmap *regmap, struct audio_reg_group *reg_group)
{
	unsigned int i;

	SND_LOG_DEBUG("\n");

	for (i = 0 ; i < reg_group->size; ++i)
		regmap_read(regmap, reg_group->label[i].address, &(reg_group->label[i].value));

	return i;
}
EXPORT_SYMBOL_GPL(snd_sunxi_save_reg);

int snd_sunxi_echo_reg(struct regmap *regmap, struct audio_reg_group *reg_group)
{
	unsigned int i;

	SND_LOG_DEBUG("\n");

	for (i = 0 ; i < reg_group->size; ++i)
		regmap_write(regmap, reg_group->label[i].address, reg_group->label[i].value);

	return i;
}
EXPORT_SYMBOL_GPL(snd_sunxi_echo_reg);

/******* regulator config *******/
struct snd_sunxi_rglt *snd_sunxi_regulator_init(struct device *dev)
{
	int ret, i, j;
	struct device_node *np = NULL;
	struct snd_sunxi_rglt *rglt = NULL;
	struct snd_sunxi_rglt_unit *unit = NULL;
	struct snd_sunxi_rglt_unit *rglt_unit = NULL;
	u32 temp_val;
	char str[32] = {0};
	const char *out_string;
	struct {
		char *name;
		enum SND_SUNXI_RGLT_MODE mode;
	} of_mode_table[] = {
		{ "PMU",	SND_SUNXI_RGLT_PMU },
		{ "AUDIO",	SND_SUNXI_RGLT_AUDIO },
	};

	SND_LOG_DEBUG("\n");

	if (!dev) {
		SND_LOG_ERR("device invailed\n");
		return NULL;
	}
	np = dev->of_node;

	rglt = kzalloc(sizeof(*rglt), GFP_KERNEL);
	if (!rglt) {
		SND_LOGDEV_ERR(dev, "can't allocate snd_sunxi_rglt memory\n");
		return NULL;
	}

	ret = of_property_read_u32(np, "rglt-max", &temp_val);
	if (ret < 0) {
		SND_LOGDEV_DEBUG(dev, "rglt-max get failed\n");
		rglt->unit_cnt = 0;
		return rglt;
	} else {
		rglt->unit_cnt = temp_val;
	}

	rglt_unit = kzalloc(sizeof(*rglt_unit) * rglt->unit_cnt, GFP_KERNEL);
	if (!rglt) {
		SND_LOGDEV_ERR(dev, "can't allocate rglt_unit memory\n");
		kfree(rglt_unit);
		return NULL;
	}

	for (i = 0; i < rglt->unit_cnt; ++i) {
		unit = &rglt_unit[i];
		snprintf(str, sizeof(str), "rglt%d-mode", i);
		ret = of_property_read_string(np, str, &out_string);
		if (ret < 0) {
			SND_LOGDEV_ERR(dev, "get %s failed\n", str);
			goto err;
		} else {
			for (j = 0; j < ARRAY_SIZE(of_mode_table); ++j) {
				if (strcmp(out_string, of_mode_table[j].name) == 0) {
					unit->mode = of_mode_table[j].mode;
					break;
				}
			}
		}
		switch (unit->mode) {
		case SND_SUNXI_RGLT_PMU:
			snprintf(str, sizeof(str), "rglt%d-voltage", i);
			ret = of_property_read_u32(np, str, &temp_val);
			if (ret < 0) {
				SND_LOGDEV_ERR(dev, "get %s failed\n", str);
				goto err;
			} else {
				unit->vcc_vol = temp_val;
			}
			snprintf(str, sizeof(str), "rglt%d", i);
			unit->vcc = regulator_get(dev, str);
			if (IS_ERR_OR_NULL(unit->vcc)) {
				SND_LOGDEV_ERR(dev, "get %s failed\n", str);
				goto err;
			}
			ret = regulator_set_voltage(unit->vcc, unit->vcc_vol, unit->vcc_vol);
			if (ret < 0) {
				SND_LOGDEV_ERR(dev, "set %s voltage failed\n", str);
				goto err;
			}
			ret = regulator_enable(unit->vcc);
			if (ret < 0) {
				SND_LOGDEV_ERR(dev, "enable %s failed\n", str);
				goto err;
			}
			break;
		default:
			SND_LOGDEV_DEBUG(dev, "%u mode no need to procees\n", unit->mode);
			break;
		}
	}

	rglt->unit = rglt_unit;
	rglt->priv = dev;
	return rglt;
err:
	kfree(rglt_unit);
	kfree(rglt);
	return NULL;

}
EXPORT_SYMBOL_GPL(snd_sunxi_regulator_init);

void snd_sunxi_regulator_exit(struct snd_sunxi_rglt *rglt)
{
	int i;
	struct snd_sunxi_rglt_unit *unit = NULL;

	SND_LOG_DEBUG("\n");

	if (!rglt) {
		SND_LOG_ERR("snd_sunxi_rglt invailed\n");
		return;
	}

	for (i = 0; i < rglt->unit_cnt; ++i) {
		unit = &rglt->unit[i];
		switch (unit->mode) {
		case SND_SUNXI_RGLT_PMU:
			if (!IS_ERR_OR_NULL(unit->vcc)) {
				regulator_disable(unit->vcc);
				regulator_put(unit->vcc);
			}
			break;
		default:
			break;
		}
	}

	if (rglt->unit)
		kfree(rglt->unit);
	kfree(rglt);
}
EXPORT_SYMBOL_GPL(snd_sunxi_regulator_exit);

int snd_sunxi_regulator_enable(struct snd_sunxi_rglt *rglt)
{
	int ret, i;
	struct device *dev = NULL;
	struct snd_sunxi_rglt_unit *unit = NULL;

	SND_LOG_DEBUG("\n");

	if (!rglt) {
		SND_LOG_ERR("snd_sunxi_rglt invailed\n");
		return -1;
	}
	dev = (struct device *)rglt->priv;

	for (i = 0; i < rglt->unit_cnt; ++i) {
		unit = &rglt->unit[i];
		switch (unit->mode) {
		case SND_SUNXI_RGLT_PMU:
			if (!IS_ERR_OR_NULL(unit->vcc)) {
				ret = regulator_enable(unit->vcc);
				if (ret) {
					SND_LOGDEV_ERR(dev, "enable vcc failed\n");
					return -1;
				}
			}
			break;
		default:
			break;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_regulator_enable);

void snd_sunxi_regulator_disable(struct snd_sunxi_rglt *rglt)
{
	int i;
	struct snd_sunxi_rglt_unit *unit = NULL;

	SND_LOG_DEBUG("\n");

	if (!rglt) {
		SND_LOG_ERR("snd_sunxi_rglt invailed\n");
		return;
	}

	for (i = 0; i < rglt->unit_cnt; ++i) {
		unit = &rglt->unit[i];
		switch (unit->mode) {
		case SND_SUNXI_RGLT_PMU:
			if (!IS_ERR_OR_NULL(unit->vcc))
				regulator_disable(unit->vcc);
			break;
		default:
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(snd_sunxi_regulator_disable);

/******* pa config *******/
DEFINE_SPINLOCK(pa_enable_lock);

static int pacfg_level_trig_init(struct snd_sunxi_pacfg *pa_cfg)
{
	int ret;
	u32 gpio_tmp;
	u32 temp_val;
	char str[32] = {0};
	u32 index = pa_cfg->index;
	struct platform_device *pdev = pa_cfg->pdev;
	struct device_node *np = pdev->dev.of_node;
	struct pacfg_level_trig *level_trig = pa_cfg->level_trig;

	SND_LOG_DEBUG("\n");

	snprintf(str, sizeof(str), "pa-pin-%d", index);
	ret = of_get_named_gpio(np, str, 0);
	if (ret < 0) {
		SND_LOG_ERR("%s get failed\n", str);
		return -1;
	}
	gpio_tmp = ret;
	if (!gpio_is_valid(gpio_tmp)) {
		SND_LOG_ERR("%s (%u) is invalid\n", str, gpio_tmp);
		return -1;
	}
	ret = devm_gpio_request(&pdev->dev, gpio_tmp, str);
	if (ret) {
		SND_LOG_ERR("%s (%u) request failed\n", str, gpio_tmp);
		return -1;
	}
	level_trig->pin = gpio_tmp;

	snprintf(str, sizeof(str), "pa-pin-level-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default low\n", str);
		level_trig->level = 0;
	} else {
		if (temp_val > 0)
			level_trig->level = 1;
	}
	snprintf(str, sizeof(str), "pa-pin-msleep-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default 0\n", str);
		level_trig->msleep_0 = 0;
	} else {
		level_trig->msleep_0 = temp_val;
	}
	snprintf(str, sizeof(str), "pa-pin-msleep1-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default 0\n", str);
		level_trig->msleep_1 = 0;
	} else {
		level_trig->msleep_1 = temp_val;
	}
	gpio_direction_output(level_trig->pin, !level_trig->level);

	return 0;
}

static void pacfg_level_trig_exit(struct snd_sunxi_pacfg *pa_cfg)
{
	(void)pa_cfg;
}

static void pacfg_level_trig_enable(struct work_struct *work)
{
	struct snd_sunxi_pacfg *pa_cfg = container_of(work, struct snd_sunxi_pacfg, pa_en_work);
	struct pacfg_level_trig *level_trig = pa_cfg->level_trig;

	SND_LOG_DEBUG("\n");

	if (level_trig->msleep_0)
		msleep(level_trig->msleep_0);
	gpio_set_value(level_trig->pin, level_trig->level);
}

static void pacfg_level_trig_disable(struct snd_sunxi_pacfg *pa_cfg)
{
	struct pacfg_level_trig *level_trig = pa_cfg->level_trig;

	SND_LOG_DEBUG("\n");

	gpio_set_value(level_trig->pin, !level_trig->level);
}

static int pacfg_pulse_trig_init(struct snd_sunxi_pacfg *pa_cfg)
{
	int ret;
	u32 gpio_tmp;
	u32 temp_val;
	char str[32] = {0};
	u32 index = pa_cfg->index;
	struct platform_device *pdev = pa_cfg->pdev;
	struct device_node *np = pdev->dev.of_node;
	struct pacfg_pulse_trig *pulse_trig = pa_cfg->pulse_trig;

	SND_LOG_DEBUG("\n");

	snprintf(str, sizeof(str), "pa-pin-%d", index);
	ret = of_get_named_gpio(np, str, 0);
	if (ret < 0) {
		SND_LOG_ERR("%s get failed\n", str);
		return -1;
	}
	gpio_tmp = ret;
	if (!gpio_is_valid(gpio_tmp)) {
		SND_LOG_ERR("%s (%u) is invalid\n", str, gpio_tmp);
		return -1;
	}
	ret = devm_gpio_request(&pdev->dev, gpio_tmp, str);
	if (ret) {
		SND_LOG_ERR("%s (%u) request failed\n", str, gpio_tmp);
		return -1;
	}
	pulse_trig->pin = gpio_tmp;

	/* get level */
	snprintf(str, sizeof(str), "pa-pin-level-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default low\n", str);
		pulse_trig->level = 0;
	} else {
		if (temp_val > 0)
			pulse_trig->level = 1;
	}

	snprintf(str, sizeof(str), "pa-pin-msleep-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default 0\n", str);
		pulse_trig->msleep_0 = 0;
	} else {
		pulse_trig->msleep_0 = temp_val;
	}
	snprintf(str, sizeof(str), "pa-pin-msleep1-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default 0\n", str);
		pulse_trig->msleep_1 = 0;
	} else {
		pulse_trig->msleep_1 = temp_val;
	}

	/* get polarity */
	snprintf(str, sizeof(str), "pa-pin-polarity-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default low\n", str);
		pulse_trig->polarity = 0;
	} else {
		if (temp_val > 0)
			pulse_trig->polarity = 1;
	}

	/* get duty_us */
	snprintf(str, sizeof(str), "pa-pin-duty-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default 0\n", str);
		pulse_trig->duty_us = 0;
	} else {
		pulse_trig->duty_us = temp_val;
	}

	/* get period_us */
	snprintf(str, sizeof(str), "pa-pin-period-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default 10\n", str);
		pulse_trig->period_us = 10;
	} else {
		if (temp_val == 0 || temp_val < pulse_trig->duty_us) {
			SND_LOG_WARN("%s invailed, duty_us * 2\n", str);
			pulse_trig->period_us = pulse_trig->duty_us * 2;
		} else {
			pulse_trig->period_us = temp_val;
		}
	}

	/* get period_cnt */
	snprintf(str, sizeof(str), "pa-pin-periodcnt-%d", index);
	ret = of_property_read_u32(np, str, &temp_val);
	if (ret < 0) {
		SND_LOG_WARN("%s get failed, default 1\n", str);
		pulse_trig->period_cnt = 5;
	} else {
		pulse_trig->period_cnt = temp_val;
	}
	gpio_direction_output(pulse_trig->pin, !pulse_trig->level);

	return 0;
}

static void pacfg_pulse_trig_exit(struct snd_sunxi_pacfg *pa_cfg)
{
	(void)pa_cfg;
}

static void pacfg_pulse_trig_enable(struct work_struct *work)
{
	struct snd_sunxi_pacfg *pa_cfg = container_of(work, struct snd_sunxi_pacfg, pa_en_work);
	struct pacfg_pulse_trig *pulse_trig = pa_cfg->pulse_trig;
	unsigned long flags;
	u32 i;

	SND_LOG_DEBUG("\n");

	if (pulse_trig->msleep_0)
		msleep(pulse_trig->msleep_0);

	spin_lock_irqsave(&pa_enable_lock, flags);

	gpio_set_value(pulse_trig->pin, !pulse_trig->level);
	for (i = 0; i < pulse_trig->period_cnt; ++i) {
		if (pulse_trig->duty_us) {
			gpio_set_value(pulse_trig->pin, pulse_trig->polarity);
			udelay(pulse_trig->duty_us);
		}
		if (pulse_trig->period_us - pulse_trig->duty_us) {
			gpio_set_value(pulse_trig->pin, !pulse_trig->polarity);
			udelay(pulse_trig->period_us - pulse_trig->duty_us);
		}
	}

	gpio_set_value(pulse_trig->pin, pulse_trig->level);

	spin_unlock_irqrestore(&pa_enable_lock, flags);
}

static void pacfg_pulse_trig_disable(struct snd_sunxi_pacfg *pa_cfg)
{
	struct pacfg_pulse_trig *pulse_trig = pa_cfg->pulse_trig;

	SND_LOG_DEBUG("\n");

	gpio_set_value(pulse_trig->pin, !pulse_trig->level);
}

static int pacfg_user_trig_init(struct snd_sunxi_pacfg *pa_cfg)
{
	return snd_sunxi_pa_user_trig_init(pa_cfg);
}

static void pacfg_user_trig_exit(struct snd_sunxi_pacfg *pa_cfg)
{
	snd_sunxi_pa_user_trig_exit(pa_cfg);
}

static int pacfg_user_trig_enable(struct snd_sunxi_pacfg *pa_cfg)
{
	return snd_sunxi_pa_user_trig_enable(pa_cfg);
}

static void pacfg_user_trig_disable(struct snd_sunxi_pacfg *pa_cfg)
{
	snd_sunxi_pa_user_trig_disable(pa_cfg);
}

static void pacfg_level_trig_disa_work(struct work_struct *work)
{
	struct snd_sunxi_pacfg *pa_cfg = container_of(work, struct snd_sunxi_pacfg, pa_disa_work);
	struct pacfg_level_trig *level_trig = pa_cfg->level_trig;

	if (level_trig->msleep_1)
		msleep(level_trig->msleep_1);

	pa_cfg->pa_disa_cb(pa_cfg->param);
}

static void pacfg_pulse_trig_disa_work(struct work_struct *work)
{
	struct snd_sunxi_pacfg *pa_cfg = container_of(work, struct snd_sunxi_pacfg, pa_disa_work);
	struct pacfg_pulse_trig *pulse_trig = pa_cfg->pulse_trig;

	if (pulse_trig->msleep_1)
		msleep(pulse_trig->msleep_1);

	pa_cfg->pa_disa_cb(pa_cfg->param);
}

struct snd_sunxi_pacfg *snd_sunxi_pa_pin_init(struct platform_device *pdev, u32 *pa_pin_max)
{
	int ret, i;
	u32 pin_max;
	u32 temp_val;
	char str[32] = {0};
	struct snd_sunxi_pacfg *pa_cfg;
	enum SND_SUNXI_PACFG_MODE pacfg_mode = SND_SUNXI_PA_CFG_LEVEL;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG("\n");

	*pa_pin_max = 0;
	ret = of_property_read_u32(np, "pa-pin-max", &temp_val);
	if (ret < 0) {
		SND_LOG_DEBUG("pa-pin-max get failed, default 0\n");
		return NULL;
	} else {
		pin_max = temp_val;
	}

	pa_cfg = kzalloc(sizeof(*pa_cfg) * pin_max, GFP_KERNEL);
	if (!pa_cfg) {
		SND_LOG_ERR("can't snd_sunxi_pacfg memory\n");
		return NULL;
	}

	for (i = 0; i < pin_max; i++) {
		pa_cfg[i].index = i;
		pa_cfg[i].pdev = pdev;
		snprintf(str, sizeof(str), "pa-cfg-mode-%d", i);
		ret = of_property_read_u32(np, str, &temp_val);
		if (ret < 0) {
			SND_LOG_DEBUG("%s get failed, default level trigger mode\n", str);
			pacfg_mode = SND_SUNXI_PA_CFG_LEVEL;
		} else {
			if (temp_val < SND_SUNXI_PA_CFG_CNT)
				pacfg_mode = temp_val;
			else if (temp_val == 0xff)
				pacfg_mode = SND_SUNXI_PA_CFG_USER;
			else
				pacfg_mode = SND_SUNXI_PA_CFG_LEVEL;
		}
		pa_cfg[i].mode = pacfg_mode;

		switch (pacfg_mode) {
		case SND_SUNXI_PA_CFG_LEVEL:
			pa_cfg[i].level_trig = kzalloc(sizeof(*pa_cfg[i].level_trig), GFP_KERNEL);
			if (!pa_cfg[i].level_trig) {
				SND_LOG_ERR("no memory\n");
				goto err;
			}
			ret = pacfg_level_trig_init(&pa_cfg[i]);
			if (ret) {
				SND_LOG_ERR("pacfg_level_trig_init failed\n");
				pa_cfg[i].used = false;
				continue;
			}
			INIT_WORK(&pa_cfg[i].pa_en_work, pacfg_level_trig_enable);
			INIT_WORK(&pa_cfg[i].pa_disa_work, pacfg_level_trig_disa_work);
			pa_cfg[i].used = true;
			break;
		case SND_SUNXI_PA_CFG_PULSE:
			pa_cfg[i].pulse_trig = kzalloc(sizeof(*pa_cfg[i].pulse_trig), GFP_KERNEL);
			if (!pa_cfg[i].pulse_trig) {
				SND_LOG_ERR("no memory\n");
				goto err;
			}
			ret = pacfg_pulse_trig_init(&pa_cfg[i]);
			if (ret) {
				SND_LOG_ERR("pacfg_pulse_trig_init failed\n");
				pa_cfg[i].used = false;
				continue;
			}
			INIT_WORK(&pa_cfg[i].pa_en_work, pacfg_pulse_trig_enable);
			INIT_WORK(&pa_cfg[i].pa_disa_work, pacfg_pulse_trig_disa_work);
			pa_cfg[i].used = true;
			break;
		case SND_SUNXI_PA_CFG_USER:
			pa_cfg[i].user_trig = kzalloc(sizeof(*pa_cfg[i].user_trig), GFP_KERNEL);
			if (!pa_cfg[i].user_trig) {
				SND_LOG_ERR("no memory\n");
				goto err;
			}
			ret = pacfg_user_trig_init(&pa_cfg[i]);
			if (ret) {
				SND_LOG_ERR("pacfg_user_trig_init failed\n");
				pa_cfg[i].used = false;
				continue;
			}
			pa_cfg[i].used = true;
			break;
		default:
			SND_LOG_WARN("unsupport pa config mode %d\n", pacfg_mode);
			pa_cfg[i].used = false;
			break;
		}
	}

	*pa_pin_max = pin_max;
	snd_sunxi_pa_pin_disable(pa_cfg, pin_max);
	return pa_cfg;
err:
	for (i = 0; i < pin_max; i++) {
		kfree(pa_cfg[i].level_trig);
		kfree(pa_cfg[i].pulse_trig);
		kfree(pa_cfg[i].user_trig);
	}
	kfree(pa_cfg);
	return NULL;
}
EXPORT_SYMBOL_GPL(snd_sunxi_pa_pin_init);

void snd_sunxi_pa_pin_exit(struct snd_sunxi_pacfg *pa_cfg, u32 pa_pin_max)
{
	int i;

	SND_LOG_DEBUG("\n");

	snd_sunxi_pa_pin_disable(pa_cfg, pa_pin_max);

	for (i = 0; i < pa_pin_max; i++) {
		if (!pa_cfg[i].used)
			continue;

		switch (pa_cfg[i].mode) {
		case SND_SUNXI_PA_CFG_LEVEL:
			pacfg_level_trig_exit(&pa_cfg[i]);
			cancel_work_sync(&pa_cfg[i].pa_en_work);
			cancel_work_sync(&pa_cfg[i].pa_disa_work);
			break;
		case SND_SUNXI_PA_CFG_PULSE:
			pacfg_pulse_trig_exit(&pa_cfg[i]);
			cancel_work_sync(&pa_cfg[i].pa_en_work);
			cancel_work_sync(&pa_cfg[i].pa_disa_work);
			break;
		case SND_SUNXI_PA_CFG_USER:
			pacfg_user_trig_exit(&pa_cfg[i]);
			break;
		default:
			SND_LOG_WARN("unsupport pa config mode %d\n", pa_cfg[i].mode);
			break;
		}
	}

	for (i = 0; i < pa_pin_max; i++) {
		kfree(pa_cfg[i].level_trig);
		kfree(pa_cfg[i].pulse_trig);
		kfree(pa_cfg[i].user_trig);
	}
	kfree(pa_cfg);
}
EXPORT_SYMBOL_GPL(snd_sunxi_pa_pin_exit);

/*
 * the example of adding component of pa
 * ucontrol->value.integer.value[0] -- value get from user or set by user.
 *                                  -- 0 - "Off", 1 - "On".
 */
#define PA_ADD_KCONTROL 0
#if PA_ADD_KCONTROL
static int sunxi_pa_param1_get_val(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = xxx;
	return 0;
}

static int sunxi_pa_param1_set_val(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct snd_sunxi_pacfg *pa_cfg = codec->pa_cfg;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		xxx
		break;
	case 1:
		xxx
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif

int snd_sunxi_pa_pin_probe(struct snd_sunxi_pacfg *pa_cfg, u32 pa_pin_max,
			   struct snd_soc_component *component)
{
	int ret = 0;

	/* the example of adding component of pa:
	 * 1.Define optional text.
	 * 2.Define enum by SOC_ENUM_SINGLE_EXT_DECL().
	 * 3.Define controls[] for each pa.
	 * 4.Add componnet by snd_soc_add_component_controls().
	 */
#if PA_ADD_KCONTROL
	char *sunxi_pa_param1_text[] = {"Off", "On"};

	SOC_ENUM_SINGLE_EXT_DECL(sunxi_pa_param1_enum, sunxi_pa_param1_text);

	struct snd_kcontrol_new sunxi_pa_controls[] = {
		SOC_ENUM_EXT("PA PARAM1", sunxi_pa_param1_enum,
			     sunxi_pa_param1_get_val,
			     sunxi_pa_param1_set_val),
		...
	};

	SND_LOG_DEBUG("\n");

	ret = snd_soc_add_component_controls(component, sunxi_pa_controls,
					     ARRAY_SIZE(sunxi_pa_controls));
	if (ret)
		SND_LOG_ERR("register pa kcontrols failed\n");
#endif
	if (pa_cfg == NULL)
		return ret;

	if (pa_cfg->mode == SND_SUNXI_PA_CFG_USER) {
		ret = snd_sunxi_pa_user_trig_probe(pa_cfg, pa_pin_max, component);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(snd_sunxi_pa_pin_probe);

void snd_sunxi_pa_pin_remove(struct snd_sunxi_pacfg *pa_cfg, u32 pa_pin_max)
{
	(void)pa_cfg;
	(void)pa_pin_max;
}
EXPORT_SYMBOL_GPL(snd_sunxi_pa_pin_remove);

int snd_sunxi_pa_pin_enable(struct snd_sunxi_pacfg *pa_cfg, u32 pa_pin_max)
{
	int i, ret;

	SND_LOG_DEBUG("\n");

	if (pa_pin_max < 1) {
		SND_LOG_DEBUG("no pa pin config\n");
		return 0;
	}

	for (i = 0; i < pa_pin_max; i++) {
		if (!pa_cfg[i].used)
			continue;

		switch (pa_cfg[i].mode) {
		case SND_SUNXI_PA_CFG_LEVEL:
			schedule_work(&pa_cfg[i].pa_en_work);
			break;
		case SND_SUNXI_PA_CFG_PULSE:
			schedule_work(&pa_cfg[i].pa_en_work);
			break;
		case SND_SUNXI_PA_CFG_USER:
			ret = pacfg_user_trig_enable(&pa_cfg[i]);
			if (ret) {
				SND_LOG_ERR("pacfg_user_trig_enable failed\n");
				continue;
			}
			break;
		default:
			SND_LOG_WARN("unsupport pa config mode %d\n", pa_cfg[i].mode);
			break;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_pa_pin_enable);

void snd_sunxi_pa_pin_disable(struct snd_sunxi_pacfg *pa_cfg, u32 pa_pin_max)
{
	int i;

	SND_LOG_DEBUG("\n");

	if (pa_pin_max < 1) {
		SND_LOG_DEBUG("no pa pin config\n");
		return;
	}

	for (i = 0; i < pa_pin_max; i++) {
		if (!pa_cfg[i].used)
			continue;

		switch (pa_cfg[i].mode) {
		case SND_SUNXI_PA_CFG_LEVEL:
			pacfg_level_trig_disable(&pa_cfg[i]);
			if (pa_cfg[i].level_trig->msleep_1)
				msleep(pa_cfg[i].level_trig->msleep_1);
			break;
		case SND_SUNXI_PA_CFG_PULSE:
			pacfg_pulse_trig_disable(&pa_cfg[i]);
			if (pa_cfg[i].pulse_trig->msleep_1)
				msleep(pa_cfg[i].pulse_trig->msleep_1);
			break;
		case SND_SUNXI_PA_CFG_USER:
			pacfg_user_trig_disable(&pa_cfg[i]);
			break;
		default:
			SND_LOG_WARN("unsupport pa config mode %d\n", pa_cfg[i].mode);
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(snd_sunxi_pa_pin_disable);

void snd_sunxi_pa_pin_disable_irp(struct snd_sunxi_pacfg *pa_cfg, u32 pa_pin_max, void *param,
				  void (*pa_disa_cb)(void *data))
{
	int i;

	SND_LOG_DEBUG("\n");

	if (pa_pin_max < 1) {
		SND_LOG_DEBUG("no pa pin config\n");
		return;
	}

	for (i = 0; i < pa_pin_max; i++) {
		if (!pa_cfg[i].used)
			continue;

		switch (pa_cfg[i].mode) {
		case SND_SUNXI_PA_CFG_LEVEL:
			pacfg_level_trig_disable(&pa_cfg[i]);
			pa_cfg[i].param = param;
			pa_cfg[i].pa_disa_cb = pa_disa_cb;
			if (pa_disa_cb)
				schedule_work(&pa_cfg[i].pa_disa_work);
			break;
		case SND_SUNXI_PA_CFG_PULSE:
			pacfg_pulse_trig_disable(&pa_cfg[i]);
			pa_cfg[i].param = param;
			pa_cfg[i].pa_disa_cb = pa_disa_cb;
			if (pa_disa_cb)
				schedule_work(&pa_cfg[i].pa_disa_work);
			break;
		case SND_SUNXI_PA_CFG_USER:
			pacfg_user_trig_disable(&pa_cfg[i]);
			break;
		default:
			SND_LOG_WARN("unsupport pa config mode %d\n", pa_cfg[i].mode);
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(snd_sunxi_pa_pin_disable_irp);

/******* extparam config *******/
int snd_sunxi_hdmi_get_dai_type(struct device_node *np, unsigned int *dai_type)
{
	int ret;
	const char *str;

	SND_LOG_DEBUG("\n");

	if (!np) {
		SND_LOG_ERR("np is err\n");
		return -1;
	}

	ret = of_property_read_string(np, "dai-type", &str);
	if (ret < 0) {
		*dai_type = SUNXI_DAI_I2S_TYPE;
	} else {
		if (strcmp(str, "hdmi") == 0) {
			*dai_type = SUNXI_DAI_HDMI_TYPE;
		} else {
			*dai_type = SUNXI_DAI_I2S_TYPE;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_hdmi_get_dai_type);

static LIST_HEAD(extparam_list);
static DEFINE_MUTEX(extparam_mutex);

int snd_sunxi_extparam_probe(const char *card_name, enum EXTPARAM_ID id)
{
	struct snd_sunxi_extparam *extparam, *n;
	struct snd_notifier_block *extparam_nb_vir;

	SND_LOG_DEBUG("\n");

	if (!card_name) {
		SND_LOG_ERR("card name invalid\n");
		return -1;
	}

	mutex_lock(&extparam_mutex);
	list_for_each_entry_safe(extparam, n, &extparam_list, list) {
		if (!strcmp(extparam->name, card_name) && extparam->id == id) {
			mutex_unlock(&extparam_mutex);
			return 0;
		}
	}

	extparam = kzalloc(sizeof(*extparam), GFP_KERNEL);
	if (!extparam) {
		mutex_unlock(&extparam_mutex);
		return -ENOMEM;
	}
	extparam->name = card_name;
	extparam->id = id;
	INIT_LIST_HEAD(&extparam->extparam_nbs);
	list_add_tail(&extparam->list, &extparam_list);

	/* add vir nb save param. */
	extparam_nb_vir = kzalloc(sizeof(*extparam_nb_vir), GFP_KERNEL);
	if (!extparam_nb_vir) {
		mutex_unlock(&extparam_mutex);
		SND_LOG_ERR("extparam_nb_vir alloc failed\n");
		return -ENOMEM;
	}
	extparam_nb_vir->notifier_call = NULL;
	extparam_nb_vir->cb_data = NULL;
	list_add_tail(&extparam_nb_vir->list, &extparam->extparam_nbs);

	mutex_unlock(&extparam_mutex);

	return 0;
}
EXPORT_SYMBOL(snd_sunxi_extparam_probe);

void snd_sunxi_extparam_remove(const char *card_name, enum EXTPARAM_ID id)
{
	struct snd_sunxi_extparam *extparam_del, *n;
	struct snd_notifier_block *extparam_nb_del, *m;

	SND_LOG_DEBUG("\n");

	if (!card_name) {
		SND_LOG_ERR("card name invalid\n");
		return;
	}

	mutex_lock(&extparam_mutex);
	list_for_each_entry_safe(extparam_del, n, &extparam_list, list) {
		if (!strcmp(extparam_del->name, card_name) && extparam_del->id == id) {
			list_del(&extparam_del->list);
			list_for_each_entry_safe(extparam_nb_del, m,
						 &extparam_del->extparam_nbs, list) {
				list_del(&extparam_nb_del->list);
				kfree(extparam_nb_del);
			}
			kfree(extparam_del);
			break;
		}
	}
	mutex_unlock(&extparam_mutex);
}
EXPORT_SYMBOL(snd_sunxi_extparam_remove);

int snd_sunxi_extparam_register_cb(const char *card_name, enum EXTPARAM_ID id,
				   snd_notifier_fn_t notifier_call, void *cb_data)
{
	struct snd_sunxi_extparam *extparam, *n;
	struct snd_notifier_block *extparam_nb_vir, *extparam_nb, *m;
	bool extparam_exist = false;

	SND_LOG_DEBUG("\n");

	if (!notifier_call) {
		SND_LOG_ERR("callback invalid\n");
		return -1;
	}
	if (!card_name) {
		SND_LOG_ERR("card name invalid\n");
		return -1;
	}
	if (id >= EXTPARAM_ID_CNT) {
		SND_LOG_ERR("id invalid\n");
		return -1;
	}

	mutex_lock(&extparam_mutex);
	list_for_each_entry_safe(extparam, n, &extparam_list, list) {
		if (!strcmp(extparam->name, card_name) && extparam->id == id) {
			list_for_each_entry_safe(extparam_nb, m, &extparam->extparam_nbs, list)
				if (extparam_nb->notifier_call == notifier_call) {
					SND_LOG_DEBUG("extparam_nb already exist\n");
					mutex_unlock(&extparam_mutex);
					return 0;
				}
			extparam_exist = true;
			break;
		}
	}

	if (!extparam_exist) {
		extparam = kzalloc(sizeof(*extparam), GFP_KERNEL);
		if (!extparam) {
			mutex_unlock(&extparam_mutex);
			return -ENOMEM;
		}
		extparam->name = card_name;
		extparam->id = id;
		INIT_LIST_HEAD(&extparam->extparam_nbs);
		list_add_tail(&extparam->list, &extparam_list);

		/* add vir nb save param. */
		extparam_nb_vir = kzalloc(sizeof(*extparam_nb_vir), GFP_KERNEL);
		if (!extparam_nb_vir) {
			mutex_unlock(&extparam_mutex);
			SND_LOG_ERR("extparam_nb_vir alloc failed\n");
			return -ENOMEM;
		}
		extparam_nb_vir->notifier_call = NULL;
		extparam_nb_vir->cb_data = NULL;
		list_add_tail(&extparam_nb_vir->list, &extparam->extparam_nbs);
	}

	extparam_nb = kzalloc(sizeof(*extparam_nb), GFP_KERNEL);
	if (!extparam_nb) {
		mutex_unlock(&extparam_mutex);
		SND_LOG_ERR("extparam_nb alloc failed\n");
		return -ENOMEM;
	}
	extparam_nb->notifier_call = notifier_call;
	if (cb_data)
		extparam_nb->cb_data = cb_data;
	list_add_tail(&extparam_nb->list, &extparam->extparam_nbs);
	mutex_unlock(&extparam_mutex);

	return 0;
}
EXPORT_SYMBOL(snd_sunxi_extparam_register_cb);

void snd_sunxi_extparam_unregister_cb(const char *card_name, enum EXTPARAM_ID id,
				      snd_notifier_fn_t notifier_call)
{
	struct snd_sunxi_extparam *extparam, *n;
	struct snd_notifier_block *extparam_nb_del, *m;

	SND_LOG_DEBUG("\n");

	if (!notifier_call) {
		SND_LOG_ERR("callback invalid\n");
		return;
	}
	if (!card_name) {
		SND_LOG_ERR("card name invalid\n");
		return;
	}
	if (id >= EXTPARAM_ID_CNT) {
		SND_LOG_ERR("id invalid\n");
		return;
	}

	mutex_lock(&extparam_mutex);
	list_for_each_entry_safe(extparam, n, &extparam_list, list) {
		if (!strcmp(extparam->name, card_name) && extparam->id == id) {
			list_for_each_entry_safe(extparam_nb_del, m,
						 &extparam->extparam_nbs, list) {
				if (extparam_nb_del->notifier_call == notifier_call) {
					list_del(&extparam_nb_del->list);
					kfree(extparam_nb_del);
					break;
				}
			}
			break;
		}
	}
	mutex_unlock(&extparam_mutex);
}
EXPORT_SYMBOL(snd_sunxi_extparam_unregister_cb);

void *snd_sunxi_extparam_get_state(const char *card_name, enum EXTPARAM_ID id)
{
	struct snd_sunxi_extparam *extparam, *n;
	struct snd_notifier_block *extparam_nb, *m;
	void *tx_data = NULL;

	SND_LOG_DEBUG("\n");

	mutex_lock(&extparam_mutex);
	list_for_each_entry_safe(extparam, n, &extparam_list, list) {
		if (!strcmp(extparam->name, card_name) && extparam->id == id) {
			list_for_each_entry_safe(extparam_nb, m, &extparam->extparam_nbs, list) {
				tx_data = extparam_nb->tx_data;
				break;
			}
			break;
		}
	}
	mutex_unlock(&extparam_mutex);

	if (!tx_data)
		SND_LOG_ERR("can't find any extparam attach(%s-%d)\n", card_name, id);
	return tx_data;
}
EXPORT_SYMBOL(snd_sunxi_extparam_get_state);

int snd_sunxi_extparam_set_state_sync(const char *card_name, enum EXTPARAM_ID id, void *tx_data)
{
	struct snd_sunxi_extparam *extparam, *n;
	struct snd_notifier_block *extparam_nb, *m;
	bool nb_exist = false;

	SND_LOG_DEBUG("\n");

	if (!card_name) {
		SND_LOG_ERR("card name invalid\n");
		return -1;
	}
	if (id >= EXTPARAM_ID_CNT) {
		SND_LOG_ERR("id invalid\n");
		return -1;
	}
	if (!tx_data) {
		SND_LOG_ERR("tx private data invalid\n");
		return -1;
	}

	mutex_lock(&extparam_mutex);
	list_for_each_entry_safe(extparam, n, &extparam_list, list) {
		if (!strcmp(extparam->name, card_name) && extparam->id == id) {
			list_for_each_entry_safe(extparam_nb, m, &extparam->extparam_nbs, list) {
				nb_exist = true;
				extparam_nb->tx_data = tx_data;
				if (extparam_nb->notifier_call)
					extparam_nb->notifier_call(extparam_nb);
			}
			break;
		}
	}
	mutex_unlock(&extparam_mutex);

	if (!nb_exist) {
		SND_LOG_ERR("can't find any extparam attach(%s-%d)\n", card_name, id);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(snd_sunxi_extparam_set_state_sync);

/******* sysfs dump *******/
struct snd_sunxi_dev {
	dev_t snd_dev;
	struct class *snd_class;
	char *snd_dev_name;
	char *snd_class_name;
};

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
static LIST_HEAD(dump_list);
static struct mutex dump_mutex;

int snd_sunxi_dump_register(struct snd_sunxi_dump *dump)
{
	struct snd_sunxi_dump *dump_tmp, *c;

	SND_LOG_DEBUG("\n");

	if (!dump) {
		SND_LOG_ERR("snd sunxi dump invailed\n");
		return -1;
	}
	if (!dump->name) {
		SND_LOG_ERR("snd sunxi dump name null\n");
		return -1;
	}

	mutex_lock(&dump_mutex);

	list_for_each_entry_safe(dump_tmp, c, &dump_list, list) {
		if (!strcmp(dump_tmp->name, dump->name)) {
			SND_LOG_ERR("snd dump(%s) already exist\n", dump->name);
			mutex_unlock(&dump_mutex);
			return -1;
		}
	}

	dump->use = false;
	list_add_tail(&dump->list, &dump_list);
	SND_LOG_DEBUG("snd dump(%s) add\n", dump->name);

	mutex_unlock(&dump_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(snd_sunxi_dump_register);

void snd_sunxi_dump_unregister(struct snd_sunxi_dump *dump)
{
	struct snd_sunxi_dump *dump_del, *c;

	SND_LOG_DEBUG("\n");

	if (!dump) {
		SND_LOG_ERR("snd sunxi dump invailed\n");
		return;
	}

	mutex_lock(&dump_mutex);

	list_for_each_entry_safe(dump_del, c, &dump_list, list) {
		if (!strcmp(dump_del->name, dump->name)) {
			SND_LOG_DEBUG("snd dump(%s) del\n", dump_del->name);
			list_del(&dump_del->list);
		}
	}

	mutex_unlock(&dump_mutex);
}
EXPORT_SYMBOL_GPL(snd_sunxi_dump_unregister);

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
snd_sunxi_version_show(const struct class *class, const struct class_attribute *attr,
		       char *buf)
#else
snd_sunxi_version_show(struct class *class, struct class_attribute *attr, char *buf)
#endif
{
	size_t count = 0, count_tmp = 0;
	struct snd_sunxi_dump *dump_tmp, *c;
	struct snd_sunxi_dump *dump = NULL;

	mutex_lock(&dump_mutex);

	list_for_each_entry_safe(dump_tmp, c, &dump_list, list) {
		dump = dump_tmp;
		if (dump && dump->dump_version) {
			count += sprintf(buf + count, "module(%s) version: ", dump->name);
			dump->dump_version(dump->priv, buf + count, &count_tmp);
			count += count_tmp;
		}
	}

	mutex_unlock(&dump_mutex);

	return count;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
snd_sunxi_help_show(const struct class *class, const struct class_attribute *attr,
		    char *buf)
#else
snd_sunxi_help_show(struct class *class, struct class_attribute *attr, char *buf)
#endif
{
	size_t count = 0, count_tmp = 0;
	struct snd_sunxi_dump *dump_tmp, *c;
	struct snd_sunxi_dump *dump = NULL;

	mutex_lock(&dump_mutex);

	list_for_each_entry_safe(dump_tmp, c, &dump_list, list)
		if (dump_tmp->use)
			dump = dump_tmp;

	mutex_unlock(&dump_mutex);

	count += sprintf(buf + count, "== module help ==\n");
	count += sprintf(buf + count, "1. get optional modules: cat module\n");
	count += sprintf(buf + count, "2. set current module  : echo {module name} > module\n");

	if (dump && dump->dump_help) {
		count += sprintf(buf + count, "== current module(%s) help ==\n", dump->name);
		dump->dump_help(dump->priv, buf + count, &count_tmp);
		count += count_tmp;
	} else if (dump && !dump->dump_help) {
		count += sprintf(buf + count, "== current module(%s), but not help ==\n",
				 dump->name);
	} else {
		count += sprintf(buf + count, "== current module(NULL) ==\n");
	}

	return count;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
snd_sunxi_module_show(const struct class *class, const struct class_attribute *attr,
		      char *buf)
#else
snd_sunxi_module_show(struct class *class, struct class_attribute *attr, char *buf)
#endif
{
	size_t count = 0;
	struct snd_sunxi_dump *dump_tmp, *c;
	struct snd_sunxi_dump *dump = NULL;
	unsigned int module_num = 0;

	count += sprintf(buf + count, "optional modules:\n");
	mutex_lock(&dump_mutex);

	list_for_each_entry_safe(dump_tmp, c, &dump_list, list) {
		count += sprintf(buf + count, "%u. %s\n", ++module_num, dump_tmp->name);
		if (dump_tmp->use)
			dump = dump_tmp;
	}

	mutex_unlock(&dump_mutex);

	if (dump)
		count += sprintf(buf + count, "current module(%s)\n", dump->name);
	else
		count += sprintf(buf + count, "current module(NULL)\n");

	return count;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
snd_sunxi_module_store(const struct class *class, const struct class_attribute *attr,
		       const char *buf, size_t count)
#else
snd_sunxi_module_store(struct class *class, struct class_attribute *attr,
		       const char *buf, size_t count)
#endif
{
	struct snd_sunxi_dump *dump, *c;
	int scanf_cnt = 0;
	char arg1[32] = {0};

	scanf_cnt = sscanf(buf, "%31s", arg1);
	if (scanf_cnt != 1)
		return count;

	mutex_lock(&dump_mutex);

	list_for_each_entry_safe(dump, c, &dump_list, list) {
		if (!strcmp(arg1, dump->name))
			dump->use = true;
		else
			dump->use = false;
	}

	mutex_unlock(&dump_mutex);

	return count;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
snd_sunxi_dump_show(const struct class *class, const struct class_attribute *attr,
		    char *buf)
#else
snd_sunxi_dump_show(struct class *class, struct class_attribute *attr, char *buf)
#endif
{
	int ret;
	size_t count = 0, count_tmp = 0;
	struct snd_sunxi_dump *dump_tmp, *c;
	struct snd_sunxi_dump *dump = NULL;

	mutex_lock(&dump_mutex);

	list_for_each_entry_safe(dump_tmp, c, &dump_list, list)
		if (dump_tmp->use)
			dump = dump_tmp;

	mutex_unlock(&dump_mutex);

	if (dump && dump->dump_show) {
		count += sprintf(buf + count, "module(%s)\n", dump->name);
		ret = dump->dump_show(dump->priv, buf + count, &count_tmp);
		if (ret)
			pr_err("module(%s) show failed\n", dump->name);
		count += count_tmp;
	} else if (dump && !dump->dump_show) {
		count += sprintf(buf + count, "current module(%s), but not show\n", dump->name);
	} else {
		count += sprintf(buf + count, "current module(NULL)\n");
	}

	return count;
}

static ssize_t
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
snd_sunxi_dump_store(const struct class *class, const struct class_attribute *attr,
		     const char *buf, size_t count)
#else
snd_sunxi_dump_store(struct class *class, struct class_attribute *attr,
		     const char *buf, size_t count)
#endif
{
	int ret;
	struct snd_sunxi_dump *dump_tmp, *c;
	struct snd_sunxi_dump *dump = NULL;

	mutex_lock(&dump_mutex);

	list_for_each_entry_safe(dump_tmp, c, &dump_list, list)
		if (dump_tmp->use)
			dump = dump_tmp;

	mutex_unlock(&dump_mutex);

	if (dump && dump->dump_store) {
		ret = dump->dump_store(dump->priv, buf, count);
		if (ret)
			pr_err("module(%s) store failed\n", dump->name);
	}

	return count;
}

static struct class_attribute snd_class_attrs[] = {
	__ATTR(version, 0644, SUNXI_ATTR_SHOW_CONVERT(snd_sunxi_version_show), NULL),
	__ATTR(help, 0644, SUNXI_ATTR_SHOW_CONVERT(snd_sunxi_help_show), NULL),
	__ATTR(module, 0644, SUNXI_ATTR_SHOW_CONVERT(snd_sunxi_module_show),
			     SUNXI_ATTR_STORE_CONVERT(snd_sunxi_module_store)),
	__ATTR(dump, 0644, SUNXI_ATTR_SHOW_CONVERT(snd_sunxi_dump_show),
			   SUNXI_ATTR_STORE_CONVERT(snd_sunxi_dump_store)),
};

static int snd_sunxi_debug_create(struct snd_sunxi_dev *sunxi_dev)
{
	int ret, i;
	unsigned int debug_node_cnt;

	SND_LOG_DEBUG("\n");

	debug_node_cnt = ARRAY_SIZE(snd_class_attrs);
	for (i = 0; i < debug_node_cnt; i++) {
		ret = class_create_file(sunxi_dev->snd_class, &snd_class_attrs[i]);
		if (ret) {
			SND_LOG_ERR("class_create_file %s failed\n",
				    snd_class_attrs[i].attr.name);
			return -1;
		}
	}

	return 0;
}

static void snd_sunxi_debug_remove(struct snd_sunxi_dev *sunxi_dev)
{
	int i;
	unsigned int debug_node_cnt;

	SND_LOG_DEBUG("\n");

	debug_node_cnt = ARRAY_SIZE(snd_class_attrs);
	for (i = 0; i < debug_node_cnt; i++)
		class_remove_file(sunxi_dev->snd_class, &snd_class_attrs[i]);
}
#else
static int snd_sunxi_debug_create(struct snd_sunxi_dev *sunxi_dev)
{
	SND_LOG_DEBUG("unsupport debug\n");
	(void)sunxi_dev;
	return 0;
}

static void snd_sunxi_debug_remove(struct snd_sunxi_dev *sunxi_dev)
{
	SND_LOG_DEBUG("unsupport debug\n");
	(void)sunxi_dev;
}
#endif

static int _snd_sunxi_dev_init(struct snd_sunxi_dev *sunxi_dev)
{
	int ret;

	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(sunxi_dev)) {
		SND_LOG_ERR("snd_sunxi_dev is NULL\n");
		return -1;
	}
	if (IS_ERR_OR_NULL(sunxi_dev->snd_dev_name) ||
	    IS_ERR_OR_NULL(sunxi_dev->snd_class_name)) {
		SND_LOG_ERR("snd_sunxi_dev name member is NULL\n");
		return -1;
	}

	ret = alloc_chrdev_region(&sunxi_dev->snd_dev, 0, 1, sunxi_dev->snd_dev_name);
	if (ret) {
		SND_LOG_ERR("alloc_chrdev_region failed\n");
		goto err_alloc_chrdev;
	}
	SND_LOG_DEBUG("sunxi_dev major = %u, sunxi_dev minor = %u\n",
		      MAJOR(sunxi_dev->snd_dev), MINOR(sunxi_dev->snd_dev));

	sunxi_dev->snd_class = sunxi_adpt_class_create(THIS_MODULE, sunxi_dev->snd_class_name);
	if (IS_ERR_OR_NULL(sunxi_dev->snd_class)) {
		SND_LOG_ERR("class_create failed\n");
		goto err_class_create;
	}

	ret = snd_sunxi_debug_create(sunxi_dev);
	if (ret) {
		SND_LOG_ERR("snd_sunxi_debug_create failed\n");
		goto err_class_create_file;
	}

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	mutex_init(&dump_mutex);
#endif

	return 0;

err_class_create_file:
	class_destroy(sunxi_dev->snd_class);
err_class_create:
	unregister_chrdev_region(sunxi_dev->snd_dev, 1);
err_alloc_chrdev:
	return -1;
}

static void _snd_sunxi_dev_exit(struct snd_sunxi_dev *sunxi_dev)
{
	SND_LOG_DEBUG("\n");

	if (IS_ERR_OR_NULL(sunxi_dev)) {
		SND_LOG_ERR("snd_sunxi_dev is NULL\n");
		return;
	}
	if (IS_ERR_OR_NULL(sunxi_dev->snd_class)) {
		SND_LOG_ERR("snd_sunxi_dev class is NULL\n");
		return;
	}

	snd_sunxi_debug_remove(sunxi_dev);

	class_destroy(sunxi_dev->snd_class);
	unregister_chrdev_region(sunxi_dev->snd_dev, 1);

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_DEBUG)
	mutex_destroy(&dump_mutex);
#endif
}

static struct snd_sunxi_dev sunxi_dev = {
	.snd_dev_name = "snd_sunxi_dev",
	.snd_class_name = "snd_sunxi",
};

int __init snd_sunxi_dev_init(void)
{
	SND_LOG_DEBUG("\n");
	return _snd_sunxi_dev_init(&sunxi_dev);
}

void __exit snd_sunxi_dev_exit(void)
{
	SND_LOG_DEBUG("\n");
	_snd_sunxi_dev_exit(&sunxi_dev);
}

module_init(snd_sunxi_dev_init);
module_exit(snd_sunxi_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1.7");
MODULE_DESCRIPTION("sunxi common interface");

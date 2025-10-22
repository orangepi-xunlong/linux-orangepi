/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _AXP8191_TEMP_CTRL_H_
#define _AXP8191_TEMP_CTRL_H_

#include "linux/types.h"

struct axp8191_model_data {
	uint8_t *model;
	size_t model_size;
};

struct axp_config_info {
	/* temp crtl */
	u32 pmu_ts_temp_enable;
	u32 pmu_ts_temp_ltf;
	u32 pmu_ts_temp_htf;

	/* wakeup function */
	u32 wakeup_ts_temp_under;
	u32 wakeup_quit_ts_temp_under;
	u32 wakeup_ts_temp_over;
	u32 wakeup_quit_ts_temp_over;
};

#define AXP_OF_PROP_READ(name, def_value)\
do {\
	if (of_property_read_u32(node, #name, &axp_config->name))\
		axp_config->name = def_value;\
} while (0)

struct axp_interrupts {
	char *name;
	irq_handler_t isr;
	int irq;
};

#endif

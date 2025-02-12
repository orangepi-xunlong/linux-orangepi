// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_MIPI_PANEL_H_
#define _KY_MIPI_PANEL_H_

#include <drm/drm_print.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <linux/backlight.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/atomic.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <soc/ky/ky_panel.h>

#define INVALID_GPIO 0xFFFFFFFF

#define LCD_PANEL_RESET_CNT 2
#define LCD_DELAY_AFTER_RESET 100

enum {
	CMD_CODE_INIT = 0,
	CMD_CODE_SLEEP_IN,
	CMD_CODE_SLEEP_OUT,
	CMD_CODE_READ_ID,
	CMD_CODE_READ_POWER,
	CMD_CODE_MAX,
};

enum {
	DSI_MODE_CMD = 0,
	DSI_MODE_VIDEO_BURST,
	DSI_MODE_VIDEO_SYNC_PULSE,
	DSI_MODE_VIDEO_SYNC_EVENT,
};

struct panel_info {
	/* common parameters */
	struct device_node *of_node;
	struct drm_display_mode mode;
	const void *cmds[CMD_CODE_MAX];
	int cmds_len[CMD_CODE_MAX];

	/* esd check parameters*/
	bool esd_check_en;
	u8 esd_check_mode;
	u16 esd_check_period;
	u32 esd_check_reg;
	u32 esd_check_val;

	/* MIPI DSI specific parameters */
	u32 format;
	u32 lanes;
	u32 mode_flags;
	bool use_dcs;
};

struct ky_panel {
	int id;
	struct device dev;
	struct drm_panel base;
	struct mipi_dsi_device *slave;
	struct panel_info info;

	struct delayed_work esd_work;
	bool esd_work_pending;

	struct regulator *vdd_1v2;
	struct regulator *vdd_1v8;
	struct regulator *vdd_2v8;
	u32 gpio_reset;
	u32 gpio_bl;
	u32 gpio_dc[2];
	u32 gpio_avdd[2];
	atomic_t enable_refcnt;
	atomic_t prepare_refcnt;
	u32 reset_toggle_cnt;
	u32 delay_after_reset;
};

struct ky_drm_notifier_mipi
{
	void *blank;
};

static BLOCKING_NOTIFIER_HEAD(drm_notifier_list);

int ky_drm_notifier_call_chain(unsigned long val, void *v);

#endif


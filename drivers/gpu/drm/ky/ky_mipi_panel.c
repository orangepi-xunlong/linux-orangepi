// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <drm/drm_atomic_helper.h>
#include <linux/atomic.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <video/mipi_display.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>
#include "ky_mipi_panel.h"
#include "ky_dsi.h"
#include "sysfs/sysfs_display.h"

const char *lcd_name;

static inline struct ky_panel *to_ky_panel(struct drm_panel *panel)
{
	return container_of(panel, struct ky_panel, base);
}

static int __maybe_unused  ky_panel_send_cmds(struct mipi_dsi_device *dsi,
				const void *data, int size)
{
	struct ky_panel *panel;
	struct ky_dsi_cmd_desc *cmds = NULL;
	u16 len;
	int data_off = 0;
	int i = 0;
	unsigned char *tmp = NULL;

	if (dsi == NULL)
		return -EINVAL;

	panel = mipi_dsi_get_drvdata(dsi);
	cmds  = devm_kzalloc(&dsi->dev, sizeof(struct ky_dsi_cmd_desc), GFP_KERNEL);
	if (!cmds)
		return -ENOMEM;

	while (size > 0) {
		cmds->cmd_type = *(unsigned char *)(data + data_off++);
		cmds->lp = *(unsigned char *)(data + data_off++);
		cmds->delay = *(unsigned char *)(data + data_off++);
		cmds->length = *(unsigned char *)(data + data_off++);
		for (i = 0; i < cmds->length; i++) {
			tmp = (unsigned char *)data + data_off++;
			cmds->data[i] = *(unsigned char *)tmp;
		}

		len = cmds->length;

		if(cmds->lp)
			dsi->mode_flags |= MIPI_DSI_MODE_LPM;
		else
			dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

		if (panel->info.use_dcs)
			mipi_dsi_dcs_write_buffer(dsi, cmds->data, len);
		else
			mipi_dsi_generic_write(dsi, cmds->data, len);

		if (cmds->delay)
			msleep(cmds->delay);
		size -= (len + 4);
	}

	devm_kfree(&dsi->dev, cmds);

	return 0;
}

/* drm_register_client - register a client notifier */
int ky_drm_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&drm_notifier_list, nb);
}
EXPORT_SYMBOL(ky_drm_register_client);

/* drm_unregister_client - unregister a client notifier */
int ky_drm_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&drm_notifier_list, nb);
}
EXPORT_SYMBOL(ky_drm_unregister_client);

/* drm_notifier_call_chain - notify clients of drm_events */
int ky_drm_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&drm_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(ky_drm_notifier_call_chain);


static int ky_panel_unprepare(struct drm_panel *p)
{
	struct ky_panel *panel = to_ky_panel(p);

	/* do nothing before ky_panel_prepare been called */
	int blank = DRM_PANEL_BLANK_POWERDOWN;
	struct ky_drm_notifier_mipi noti_blank;
	noti_blank.blank = & blank;
	ky_drm_notifier_call_chain(DRM_PANEL_EARLY_EVENT_BLANK, &noti_blank);
	pr_info("mipi: POWERDOWN!!\n");

	if (!atomic_read(&panel->prepare_refcnt))
		return 0;

	DRM_INFO("%s()\n", __func__);

	gpio_direction_output(panel->gpio_reset, 1);

	if(INVALID_GPIO != panel->gpio_bl) {
		gpio_direction_output(panel->gpio_bl, 0);
	}
	msleep(150);

	gpio_direction_output(panel->gpio_dc[0], 0);
	gpio_direction_output(panel->gpio_dc[1], 0);

	if(INVALID_GPIO != panel->gpio_avdd[0]) {
		gpio_direction_output(panel->gpio_avdd[0], 0);
	}
	if(INVALID_GPIO != panel->gpio_avdd[1]) {
		gpio_direction_output(panel->gpio_avdd[1], 0);
	}

	if (panel->vdd_1v2) {
		regulator_disable(panel->vdd_1v2);
	}
	if (panel->vdd_1v8) {
		regulator_disable(panel->vdd_1v8);
	}
	if (panel->vdd_2v8) {
		regulator_disable(panel->vdd_2v8);
	}

	atomic_set(&panel->prepare_refcnt, 0);
	return 0;
}

void ky_prepare_regulator (struct ky_panel *panel){
	int ret = 0;

	if (panel->vdd_2v8 != NULL) {
		ret = regulator_enable(panel->vdd_2v8);
		if (ret)
			DRM_ERROR("enable lcd regulator vdd_2v8 failed\n");
	}

	if (panel->vdd_1v8 != NULL) {
		ret = regulator_enable(panel->vdd_1v8);
		if (ret)
			DRM_ERROR("enable lcd regulator vdd_1v8 failed\n");
	}

	if (panel->vdd_1v2 != NULL) {
		ret = regulator_enable(panel->vdd_1v2);
		if (ret)
			DRM_ERROR("enable lcd regulator vdd_1v2 failed\n");
	}
}

static int ky_panel_prepare(struct drm_panel *p)
{
	struct ky_panel *panel = to_ky_panel(p);
	int i = 0;

	int blank_ = DRM_PANEL_BLANK_UNBLANK;
	struct ky_drm_notifier_mipi noti_blank;

	/* prevent this function been called twice */
	if (atomic_read(&panel->prepare_refcnt))
		return 0;

	DRM_INFO("%s()\n", __func__);

	// ky_prepare_regulator(panel);

	gpio_direction_output(panel->gpio_dc[0], 1);
	gpio_direction_output(panel->gpio_dc[1], 1);

	if(panel->gpio_avdd[0] != INVALID_GPIO) {
		gpio_direction_output(panel->gpio_avdd[0], 1);
	}
	if(panel->gpio_avdd[1] != INVALID_GPIO) {
		gpio_direction_output(panel->gpio_avdd[1], 1);
	}

	if(panel->gpio_bl != INVALID_GPIO) {
		gpio_direction_output(panel->gpio_bl, 1);
	}

	gpio_direction_output(panel->gpio_reset, 1);
	for (; i < panel->reset_toggle_cnt; i++) {
		msleep(10);
		gpio_direction_output(panel->gpio_reset, 0);
		msleep(10);
		gpio_direction_output(panel->gpio_reset, 1);
	}
	msleep(panel->delay_after_reset);

	noti_blank.blank = & blank_;
	ky_drm_notifier_call_chain(DRM_PANEL_EVENT_BLANK, &noti_blank);
	pr_info("mipi: UNBLANK!!\n");

	/* update refcnt */
	atomic_set(&panel->prepare_refcnt, 1);
	return 0;
}

static int ky_panel_disable(struct drm_panel *p)
{
	struct ky_panel *panel = to_ky_panel(p);

	if (!atomic_read(&panel->enable_refcnt))
		return 0;

	DRM_INFO("%s()\n", __func__);

	if (panel->esd_work_pending) {
		cancel_delayed_work_sync(&panel->esd_work);
		panel->esd_work_pending = false;
	}

	ky_panel_send_cmds(panel->slave,
			     panel->info.cmds[CMD_CODE_SLEEP_IN],
			     panel->info.cmds_len[CMD_CODE_SLEEP_IN]);

	atomic_set(&panel->enable_refcnt, 0);
	return 0;
}

static int ky_panel_enable(struct drm_panel *p)
{
	struct ky_panel *panel = to_ky_panel(p);

	if (atomic_read(&panel->enable_refcnt))
		return 0;

	DRM_INFO("%s()\n", __func__);

	ky_panel_send_cmds(panel->slave,
			     panel->info.cmds[CMD_CODE_INIT],
			     panel->info.cmds_len[CMD_CODE_INIT]);

	if (panel->info.esd_check_en) {
		schedule_delayed_work(&panel->esd_work,
				      msecs_to_jiffies(1000));
		panel->esd_work_pending = true;
	}

	atomic_set(&panel->enable_refcnt, 1);
	return 0;
}

static int ky_panel_get_modes(struct drm_panel *p, struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct ky_panel *panel = to_ky_panel(p);

	DRM_INFO("%s()\n", __func__);

	mode = drm_mode_duplicate(connector->dev, &panel->info.mode);
	if (!mode) {
		DRM_ERROR("failed to add mode %ux%ux@%u\n",
			  panel->info.mode.hdisplay,
			  panel->info.mode.vdisplay,
			  60);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = panel->info.mode.width_mm;
	connector->display_info.height_mm = panel->info.mode.height_mm;

	return 1;
}

static const struct drm_panel_funcs ky_panel_funcs = {
	.get_modes = ky_panel_get_modes,
	.enable = ky_panel_enable,
	.disable = ky_panel_disable,
	.prepare = ky_panel_prepare,
	.unprepare = ky_panel_unprepare,
};

static ssize_t mipi_dsi_device_transfer(struct mipi_dsi_device *dsi,
					struct mipi_dsi_msg *msg)
{
	const struct mipi_dsi_host_ops *ops = dsi->host->ops;

	if (!ops || !ops->transfer)
		return -ENOSYS;

	if (dsi->mode_flags & MIPI_DSI_MODE_LPM)
		msg->flags |= MIPI_DSI_MSG_USE_LPM;
	//msg->flags |= MIPI_DSI_MSG_LASTCOMMAND;

	return ops->transfer(dsi->host, msg);
}

static int ky_mipi_dsi_set_maximum_return_packet_size(struct mipi_dsi_device *dsi,
					    u16 value)
{
	u8 tx[2] = { value & 0xff, value >> 8 };
	int ret;
	struct mipi_dsi_msg msg = {0x0};

	msg.channel = dsi->channel;
	msg.type = MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE;
	msg.tx_len = sizeof(tx);
	msg.tx_buf = tx;

	ret = mipi_dsi_device_transfer(dsi, &msg);

	return (ret < 0) ? ret : 0;
}

static int ky_panel_esd_check(struct ky_panel *panel)
{
	struct panel_info *info = &panel->info;
	u8 read_val = 0;

	ky_mipi_dsi_set_maximum_return_packet_size(panel->slave, 1);
	mipi_dsi_dcs_read(panel->slave, info->esd_check_reg,
			  &read_val, 1);

	if (read_val != info->esd_check_val) {
		DRM_ERROR("esd check failed, read value = 0x%02x\n",
			  read_val);
		return -EINVAL;
	} else
		DRM_INFO("esd check, read value = 0x%02x\n", read_val);

	return 0;
}

static void ky_panel_esd_work_func(struct work_struct *work)
{
	struct ky_panel *panel = container_of(work, struct ky_panel,
						esd_work.work);
	struct panel_info *info = &panel->info;
	int ret;

	ret = ky_panel_esd_check(panel);
	if (ret) {
		/*
		const struct drm_encoder_helper_funcs *funcs;
		struct drm_encoder *encoder;

		encoder = panel->base.connector->encoder;
		funcs = encoder->helper_private;
		panel->esd_work_pending = false;

		DRM_INFO("====== esd recovery start ========\n");
		funcs->disable(encoder);
		funcs->enable(encoder);
		DRM_INFO("======= esd recovery end =========\n");
		*/
	} else
		schedule_delayed_work(&panel->esd_work,
			msecs_to_jiffies(info->esd_check_period));
}

static int ky_panel_parse_dt(struct device_node *np, struct ky_panel *panel)
{
	u32 val;
	struct device_node *lcd_node;
	struct panel_info *info = &panel->info;
	int bytes, rc;
	const void *p;
	const char *str;
	char lcd_path[60];

	rc = of_property_read_string(np, "force-attached", &str);
	if (!rc)
		lcd_name = str;

	sprintf(lcd_path, "/lcds/%s", lcd_name);
	lcd_node = of_find_node_by_path(lcd_path);
	if (!lcd_node) {
		DRM_ERROR("%pOF: could not find %s node\n", np, lcd_name);
		return -ENODEV;
	}
	info->of_node = lcd_node;

	rc = of_property_read_u32(lcd_node, "dsi-work-mode", &val);
	if (!rc) {
		if (val == DSI_MODE_CMD)
			info->mode_flags = 0;
		else if (val == DSI_MODE_VIDEO_BURST)
			info->mode_flags = MIPI_DSI_MODE_VIDEO |
					   MIPI_DSI_MODE_VIDEO_BURST;
		else if (val == DSI_MODE_VIDEO_SYNC_PULSE)
			info->mode_flags = MIPI_DSI_MODE_VIDEO |
					   MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
		else if (val == DSI_MODE_VIDEO_SYNC_EVENT)
			info->mode_flags = MIPI_DSI_MODE_VIDEO;
	} else {
		DRM_ERROR("dsi work mode is not found! use video mode\n");
		info->mode_flags = MIPI_DSI_MODE_VIDEO |
				   MIPI_DSI_MODE_VIDEO_BURST;
	}

	if (of_property_read_bool(lcd_node, "dsi-non-continuous-clock"))
		info->mode_flags |= MIPI_DSI_CLOCK_NON_CONTINUOUS;

	rc = of_property_read_u32(lcd_node, "dsi-lane-number", &val);
	if (!rc)
		info->lanes = val;
	else
		info->lanes = 4;

	rc = of_property_read_string(lcd_node, "dsi-color-format", &str);
	if (rc)
		info->format = MIPI_DSI_FMT_RGB888;
	else if (!strcmp(str, "rgb888"))
		info->format = MIPI_DSI_FMT_RGB888;
	else if (!strcmp(str, "rgb666"))
		info->format = MIPI_DSI_FMT_RGB666;
	else if (!strcmp(str, "rgb666_packed"))
		info->format = MIPI_DSI_FMT_RGB666_PACKED;
	else if (!strcmp(str, "rgb565"))
		info->format = MIPI_DSI_FMT_RGB565;
	else
		DRM_ERROR("dsi-color-format (%s) is not supported\n", str);

	rc = of_property_read_u32(lcd_node, "width-mm", &val);
	if (!rc)
		info->mode.width_mm = val;
	else
		info->mode.width_mm = 68;

	rc = of_property_read_u32(lcd_node, "height-mm", &val);
	if (!rc)
		info->mode.height_mm = val;
	else
		info->mode.height_mm = 121;

	rc = of_property_read_u32(lcd_node, "esd-check-enable", &val);
	if (!rc)
		info->esd_check_en = val;

	rc = of_property_read_u32(lcd_node, "esd-check-mode", &val);
	if (!rc)
		info->esd_check_mode = val;
	else
		info->esd_check_mode = 1;

	rc = of_property_read_u32(lcd_node, "esd-check-period", &val);
	if (!rc)
		info->esd_check_period = val;
	else
		info->esd_check_period = 1000;

	rc = of_property_read_u32(lcd_node, "esd-check-register", &val);
	if (!rc)
		info->esd_check_reg = val;
	else
		info->esd_check_reg = 0x0A;

	rc = of_property_read_u32(lcd_node, "esd-check-value", &val);
	if (!rc)
		info->esd_check_val = val;
	else
		info->esd_check_val = 0x9C;

	if (of_property_read_bool(lcd_node, "use-dcs-write"))
		info->use_dcs = true;
	else
		info->use_dcs = false;

	p = of_get_property(lcd_node, "read-id-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_READ_ID] = p;
		info->cmds_len[CMD_CODE_READ_ID] = bytes;
	} else
		DRM_ERROR("can't find read-id property\n");

	p = of_get_property(lcd_node, "initial-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_INIT] = p;
		info->cmds_len[CMD_CODE_INIT] = bytes;
	} else
		DRM_ERROR("can't find initial-command property\n");

	p = of_get_property(lcd_node, "sleep-in-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_SLEEP_IN] = p;
		info->cmds_len[CMD_CODE_SLEEP_IN] = bytes;
	} else
		DRM_ERROR("can't find sleep-in-command property\n");

	p = of_get_property(lcd_node, "sleep-out-command", &bytes);
	if (p) {
		info->cmds[CMD_CODE_SLEEP_OUT] = p;
		info->cmds_len[CMD_CODE_SLEEP_OUT] = bytes;
	} else
		DRM_ERROR("can't find sleep-out-command property\n");

	rc = of_get_drm_display_mode(lcd_node, &info->mode, 0,
				     OF_USE_NATIVE_MODE);
	if (rc) {
		DRM_ERROR("get display timing failed\n");
		return rc;
	}

	//info->mode.vrefresh = drm_mode_vrefresh(&info->mode);

	return 0;
}

static int ky_panel_device_create(struct device *parent,
				    struct ky_panel *panel)
{
	panel->dev.class = display_class;
	panel->dev.parent = parent;
	panel->dev.of_node = panel->info.of_node;
	dev_set_name(&panel->dev, "panel%d", panel->id);
	dev_set_drvdata(&panel->dev, panel);

	return device_register(&panel->dev);
}

static int ky_panel_probe(struct mipi_dsi_device *slave)
{
	int ret;
	struct ky_panel *panel;
	struct device *dev = &slave->dev;
	int count;
	const char *strings[3];
	u32 tmp;

	DRM_INFO("%s()\n", __func__);

	panel = devm_kzalloc(&slave->dev, sizeof(*panel), GFP_KERNEL);
	if (!panel)
		return -ENOMEM;

	if (!of_property_read_u32(dev->of_node, "id", &tmp))
		panel->id = tmp;

	count = of_property_count_strings(dev->of_node, "vin-supply-names");
	if (count <= 0 || count != 3) {
		panel->vdd_2v8 = NULL;
		panel->vdd_1v8 = NULL;
		panel->vdd_1v2 = NULL;
	} else {
		of_property_read_string_array(dev->of_node, "vin-supply-names", strings, 3);

		panel->vdd_2v8 = devm_regulator_get(&slave->dev, strings[0]);
		if (IS_ERR_OR_NULL(panel->vdd_2v8)) {
			DRM_DEBUG("get lcd regulator vdd_2v8 failed\n");
			panel->vdd_2v8 = NULL;
		} else {
			regulator_set_voltage(panel->vdd_2v8, 2800000, 2800000);
			ret = regulator_enable(panel->vdd_2v8);
			if (ret)
				DRM_ERROR("enable lcd regulator vdd_2v8 failed\n");
		}

		panel->vdd_1v8 = devm_regulator_get(&slave->dev, strings[1]);
		if (IS_ERR_OR_NULL(panel->vdd_1v8)) {
			DRM_DEBUG("get lcd regulator vdd_1v8 failed\n");
			panel->vdd_1v8 = NULL;
		} else {
			regulator_set_voltage(panel->vdd_1v8, 1800000, 1800000);
			ret = regulator_enable(panel->vdd_1v8);
			if (ret)
				DRM_ERROR("enable lcd regulator vdd_1v8 failed\n");
		}

		panel->vdd_1v2 = devm_regulator_get(&slave->dev, strings[2]);
		if (IS_ERR_OR_NULL(panel->vdd_1v2)) {
			DRM_DEBUG("get regulator vdd_1v2 failed\n");
			panel->vdd_1v2 = NULL;
		} else {
			regulator_set_voltage(panel->vdd_1v2, 1200000, 1200000);
			ret = regulator_enable(panel->vdd_1v2);
			if (ret)
				DRM_ERROR("enable lcd regulator vdd_1v2 failed\n");
		}
	}

	ret = of_property_read_u32(dev->of_node, "gpios-reset", &panel->gpio_reset);
	if (ret || !gpio_is_valid(panel->gpio_reset)) {
		dev_err(dev, "Missing dt property: gpios-reset\n");
	} else {
		ret = gpio_request(panel->gpio_reset, NULL);
		if (ret) {
			pr_err("gpio_reset request fail\n");
		}
	}

	ret = of_property_read_u32(dev->of_node, "gpios-bl", &panel->gpio_bl);
	if (ret || !gpio_is_valid(panel->gpio_bl)) {
		dev_dbg(dev, "Missing dt property: gpios-bl\n");
		panel->gpio_bl = INVALID_GPIO;
	} else {
		ret = gpio_request(panel->gpio_bl, NULL);
		if (ret) {
			pr_err("gpio_bl request fail\n");
		}
	}

	ret = of_property_read_u32_array(dev->of_node, "gpios-dc", panel->gpio_dc, 2);
	if (ret || !gpio_is_valid(panel->gpio_dc[0]) || !gpio_is_valid(panel->gpio_dc[1])) {
		dev_err(dev, "Missing dt property: gpios-dc\n");
		return -EINVAL;
	} else {
		ret = gpio_request(panel->gpio_dc[0], NULL);
		ret |= gpio_request(panel->gpio_dc[1], NULL);
		if (ret) {
			pr_err("gpio_dc request fail\n");
			return ret;
		}
	}

	ret = of_property_read_u32_array(dev->of_node, "gpios-avdd", panel->gpio_avdd, 2);
	if (ret || !gpio_is_valid(panel->gpio_avdd[0]) || !gpio_is_valid(panel->gpio_avdd[1])) {
		dev_dbg(dev, "Missing avdd property: gpios-avdd\n");
		panel->gpio_avdd[0] = INVALID_GPIO;
		panel->gpio_avdd[1] = INVALID_GPIO;
	} else {
		ret = gpio_request(panel->gpio_avdd[0], NULL);
		ret |= gpio_request(panel->gpio_avdd[1], NULL);
		if (ret) {
			pr_err("gpio_avdd request fail\n");
			return ret;
		}
	}

	if (of_property_read_u32(dev->of_node, "reset-toggle-cnt", &panel->reset_toggle_cnt))
		panel->reset_toggle_cnt = LCD_PANEL_RESET_CNT;

	if (of_property_read_u32(dev->of_node, "delay-after-reset", &panel->delay_after_reset))
		panel->delay_after_reset = LCD_DELAY_AFTER_RESET;

	ret = ky_panel_parse_dt(slave->dev.of_node, panel);
	if (ret) {
		DRM_ERROR("parse panel info failed\n");
		return ret;
	}

	ret = ky_panel_device_create(&slave->dev, panel);
	if (ret) {
		DRM_ERROR("panel device create failed\n");
		return ret;
	}

	panel->base.dev = &panel->dev;
	panel->base.funcs = &ky_panel_funcs;
	drm_panel_init(&panel->base, &panel->dev, &ky_panel_funcs, DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_of_backlight(&panel->base);
	if (ret) {
		DRM_ERROR("panel device get backlight failed\n");
		return ret;
	}

	drm_panel_add(&panel->base);
	backlight_disable(panel->base.backlight);

	slave->lanes = panel->info.lanes;
	slave->format = panel->info.format;
	slave->mode_flags = panel->info.mode_flags;

	ret = mipi_dsi_attach(slave);
	if (ret) {
		DRM_ERROR("failed to attach dsi panel to host\n");
		drm_panel_remove(&panel->base);
		return ret;
	}
	panel->slave = slave;

	ky_mipi_panel_sysfs_init(&panel->dev);
	mipi_dsi_set_drvdata(slave, panel);

	/*do esd init work*/
	if (panel->info.esd_check_en) {
		INIT_DELAYED_WORK(&panel->esd_work, ky_panel_esd_work_func);
		/*
		schedule_delayed_work(&panel->esd_work,
				      msecs_to_jiffies(2000));
		panel->esd_work_pending = true;
		*/
	}

	atomic_set(&panel->enable_refcnt, 0);
	atomic_set(&panel->prepare_refcnt, 0);

	DRM_INFO("panel driver probe success\n");

	return 0;
}

static void ky_panel_remove(struct mipi_dsi_device *slave)
{
	struct ky_panel *panel = mipi_dsi_get_drvdata(slave);
	int ret;

	DRM_INFO("%s()\n", __func__);

	ky_panel_disable(&panel->base);
	ky_panel_unprepare(&panel->base);

	ret = mipi_dsi_detach(slave);
	if (ret < 0)
		DRM_ERROR("failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&panel->base);

	return;
}

static const struct of_device_id panel_of_match[] = {
	{ .compatible = "ky,mipi-panel0", },
	{ .compatible = "ky,mipi-panel1", },
	{ .compatible = "ky,mipi-panel2", },
	{ }
};
MODULE_DEVICE_TABLE(of, panel_of_match);

static struct mipi_dsi_driver ky_panel_driver = {
	.driver = {
		.name = "ky-mipi-panel-drv",
		.of_match_table = panel_of_match,
	},
	.probe = ky_panel_probe,
	.shutdown = ky_panel_remove,
};
module_mipi_dsi_driver(ky_panel_driver);

MODULE_DESCRIPTION("Ky MIPI Panel Driver");
MODULE_LICENSE("GPL v2");


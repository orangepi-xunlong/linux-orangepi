// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/auxiliary_bus.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/i2c.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/media-bus-format.h>
#include <linux/atomic.h>
#include <linux/workqueue.h>
#include <asm/unaligned.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_of.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_bridge.h>
#include <linux/delay.h>
#include <drm/display/drm_dp_aux_bus.h>
#include <drm/display/drm_dp_helper.h>

#include "ky_dsi.h"

#define IT9711_DSI_DRIVER_NAME "ky-dp-drv"
#define MIPI_DSI_1920x1080  1

static const struct drm_display_mode lt9711_panel_modes[] = {

#if MIPI_DSI_1920x1080
	{
		.clock = 142857143 / 1000,
		.hdisplay = 1920,
		.hsync_start = 1920 + 88,
		.hsync_end = 1920 + 88 + 148,
		.htotal = 1920 + 88 + 148 + 44,
		.vdisplay = 1080,
		.vsync_start = 1080 + 4,
		.vsync_end = 1080 + 4 + 36,
		.vtotal = 1080 + 4 + 36 + 5,
	},
#endif

};

struct lt9711 {
	struct device *dev;
	struct drm_bridge bridge;
	struct drm_connector *connector;
	struct ky_dsi_device *ky_dsi;

	struct regmap *regmap;
	struct gpio_desc *reset_gpio;	//reset
	struct gpio_desc *enable_gpio;	//power

	struct i2c_client *client;
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	enum drm_connector_status status;
	struct delayed_work detect_work;
	bool detect_work_pending;
};

static const struct regmap_config lt9711_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xff,
};

static struct lt9711 *panel_to_lt9711(struct drm_panel *panel)
{
	return container_of(panel, struct lt9711, base);
}

static int lt9711_i2c_detect(struct lt9711 *lt9711)
{
	u8 retry = 0;
	unsigned int status = 0;
	int ret = -EAGAIN;

	while(retry++ < 3) {

		ret = regmap_write(lt9711->regmap, 0xff, 0x80);
		if(ret < 0) {
			dev_err(lt9711->dev, "LT9711 i2c detect write addr:0xff failed\n");
			continue;
		}

		regmap_read(lt9711->regmap, 0xd6, &status);
		// LT9711 i2c detect success status: 0xee
		DRM_DEBUG("LT9711 i2c detect success status: 0x%x\n", status);

		if (0xee == status)
			lt9711->status = connector_status_connected;
		else
			lt9711->status = connector_status_disconnected;

		break;
	}

	return ret;
}

static int lt9711_panel_enable(struct drm_panel *panel)
{
	// struct lt9711 *lt9711 = panel_to_lt9711(panel);
	DRM_DEBUG(" %s() \n", __func__);

	return 0;
}

static int lt9711_panel_disable(struct drm_panel *panel)
{
	// struct lt9711 *lt9711 = panel_to_lt9711(panel);
	DRM_DEBUG(" %s() \n", __func__);

	return 0;
}

static int lt9711_panel_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	// struct lt9711 *lt9711 = panel_to_lt9711(panel);
	unsigned int i, num = 0;
	static const u32 bus_format = MEDIA_BUS_FMT_RGB888_1X24;

	DRM_DEBUG(" %s() \n", __func__);

	for (i = 0; i < ARRAY_SIZE(lt9711_panel_modes); i++) {
		const struct drm_display_mode *m = &lt9711_panel_modes[i];
		struct drm_display_mode *mode;

		mode = drm_mode_duplicate(connector->dev, m);
		if (!mode) {
			dev_err(panel->dev, "failed to add mode %ux%u@%u\n",
				m->hdisplay, m->vdisplay,
				drm_mode_vrefresh(m));
			continue;
		}

		mode->type |= DRM_MODE_TYPE_DRIVER;

		if (i == 0)
			mode->type |= DRM_MODE_TYPE_PREFERRED;

		drm_mode_set_name(mode);
		drm_mode_probed_add(connector, mode);
		num++;
	}

	connector->display_info.bpc = 8;
	// 1920x1080
	#if MIPI_DSI_1920x1080
	connector->display_info.width_mm = 309;
	connector->display_info.height_mm = 174;
	#endif

	drm_display_info_set_bus_formats(&connector->display_info,
				&bus_format, 1);
	return num;
}


static const struct drm_panel_funcs lt9711_panel_funcs = {
	.disable = lt9711_panel_disable,
	.enable = lt9711_panel_enable,
	.get_modes = lt9711_panel_get_modes,
};

static void detect_work_func(struct work_struct *work)
{
	struct lt9711 *lt9711 = container_of(work, struct lt9711,
						detect_work.work);
	int ret;

	//check i2c communicate
	ret = lt9711_i2c_detect(lt9711);
	if (ret < 0) {
		DRM_INFO("detect DP failed communicate with IC use I2C\n");
	}

	if (lt9711->ky_dsi) {
		DRM_DEBUG(" %s() connector status %d\n", __func__, lt9711->ky_dsi->connector_status);
		lt9711->ky_dsi->connector_status = lt9711->status;

		if (lt9711->ky_dsi->previous_connector_status != lt9711->ky_dsi->connector_status) {
			if (lt9711->connector) {
				DRM_INFO(" %s() detect DP connector hpd event\n", __func__);
				lt9711->ky_dsi->previous_connector_status = lt9711->ky_dsi->connector_status;
				drm_helper_hpd_irq_event(lt9711->connector->dev);
			}
		}
	}

	schedule_delayed_work(&lt9711->detect_work,
				msecs_to_jiffies(2000));
}


static int lt9711_probe(struct i2c_client *client)
{
	struct lt9711 *lt9711;
	struct device *dev = &client->dev;
	int ret;

	struct device_node *endpoint, *dsi_host_node;
	struct mipi_dsi_host *host;

	struct device_node *lcd_node;
	int rc;
	const char *str;
	char lcd_path[60];
	const char *lcd_name;

	struct mipi_dsi_device_info info = {
		.type = IT9711_DSI_DRIVER_NAME,
		.channel = 0,
		.node = NULL,
	};

	DRM_DEBUG("%s()\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "Failed check I2C functionality");
		return -ENODEV;
	}

	lt9711 = devm_kzalloc(&client->dev, sizeof(*lt9711), GFP_KERNEL);
	if (!lt9711)
		return -ENOMEM;

	lt9711->dev = &client->dev;
	lt9711->client = client;
	lt9711->connector = NULL;
	lt9711->ky_dsi = NULL;
	lt9711->status = connector_status_disconnected;

	//regmap i2c , maybe useless
	lt9711->regmap = devm_regmap_init_i2c(client, &lt9711_regmap_config);
	if (IS_ERR(lt9711->regmap)) {
		dev_err(lt9711->dev, "regmap i2c init failed\n");
		return PTR_ERR(lt9711->regmap);
	}

	lt9711->enable_gpio = devm_gpiod_get_optional(dev, "enable",
						GPIOD_IN);
	if (IS_ERR(lt9711->enable_gpio)) {
		dev_err(lt9711->dev, "Failed get enable gpio\n");
		return PTR_ERR(lt9711->enable_gpio);
	}

	lt9711->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						GPIOD_IN);
	if (IS_ERR(lt9711->reset_gpio)) {
		dev_err(lt9711->dev, "Failed get reset gpio\n");
		return PTR_ERR(lt9711->reset_gpio);
	}

	//disable firstly
	gpiod_direction_output(lt9711->enable_gpio, 0);
	usleep_range(50*1000, 100*1000); //100ms
	gpiod_direction_output(lt9711->enable_gpio, 1);
	usleep_range(50*1000, 100*1000); //100ms

	gpiod_direction_output(lt9711->reset_gpio, 1);
	usleep_range(50*1000, 100*1000); //100ms
	gpiod_direction_output(lt9711->reset_gpio, 0);
	usleep_range(50*1000, 100*1000); //100ms
	gpiod_direction_output(lt9711->reset_gpio, 1);
	usleep_range(100*1000, 150*1000); //150ms

	i2c_set_clientdata(client, lt9711);
	dev_set_drvdata(dev, lt9711);

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!endpoint)
		return -ENODEV;

	dsi_host_node = of_graph_get_remote_port_parent(endpoint);
	if (!dsi_host_node)
		goto error;

	rc = of_property_read_string(dsi_host_node, "force-attached", &str);
	if (!rc)
		lcd_name = str;

	sprintf(lcd_path, "/lcds/%s", lcd_name);
	lcd_node = of_find_node_by_path(lcd_path);
	if (!lcd_node) {
		DRM_ERROR("%pOF: could not find %s node\n", dsi_host_node, lcd_name);
		of_node_put(endpoint);
		return -ENODEV;
	}

	DRM_INFO("%pOF: find %s node\n", dsi_host_node, lcd_name);

	host = of_find_mipi_dsi_host_by_node(dsi_host_node);
	of_node_put(dsi_host_node);
	if (!host) {
		of_node_put(endpoint);
		return -EPROBE_DEFER;
	}

	drm_panel_init(&lt9711->base, dev, &lt9711_panel_funcs,
			DRM_MODE_CONNECTOR_DisplayPort);

	/* This appears last, as it's what will unblock the DSI host
	 * driver's component bind function.
	 */
	drm_panel_add(&lt9711->base);

	lt9711->base.dev->of_node = lcd_node;
	info.node = of_node_get(of_graph_get_remote_port(endpoint));
	if (!info.node)
		goto error;
	of_node_put(endpoint);

	lt9711->dsi = mipi_dsi_device_register_full(host, &info);
	if (IS_ERR(lt9711->dsi)) {
		dev_err(dev, "DSI device registration failed: %ld\n",
			PTR_ERR(lt9711->dsi));
		return PTR_ERR(lt9711->dsi);
	}

	//check i2c communicate
	ret = lt9711_i2c_detect(lt9711);
	if (ret < 0) {
		DRM_INFO("detect DP failed communicate with IC use I2C\n");
		return ret;
	}

	INIT_DELAYED_WORK(&lt9711->detect_work, detect_work_func);
	schedule_delayed_work(&lt9711->detect_work,
				msecs_to_jiffies(2000));
	lt9711->detect_work_pending = true;

	return 0;
error:
	of_node_put(endpoint);
	return -ENODEV;
}

static void lt9711_remove(struct i2c_client *client)
{
	struct lt9711 *lt9711 = i2c_get_clientdata(client);

	DRM_DEBUG("%s()\n", __func__);

	if (lt9711->detect_work_pending) {
		cancel_delayed_work_sync(&lt9711->detect_work);
		lt9711->detect_work_pending = false;
	}

	lt9711->connector = NULL;
	lt9711->ky_dsi = NULL;

	mipi_dsi_detach(lt9711->dsi);
	drm_panel_remove(&lt9711->base);
	mipi_dsi_device_unregister(lt9711->dsi);
}

#ifdef CONFIG_PM_SLEEP

static int lt9711_drv_pm_suspend(struct device *dev)
{
	struct lt9711 *lt9711 = dev_get_drvdata(dev);

	DRM_DEBUG("%s()\n", __func__);

	if (lt9711->detect_work_pending) {
		cancel_delayed_work_sync(&lt9711->detect_work);
		lt9711->detect_work_pending = false;
	}

	gpiod_direction_output(lt9711->enable_gpio, 0);
	usleep_range(50*1000, 100*1000); //100ms

	return 0;
}

static int lt9711_drv_pm_resume(struct device *dev)
{
	struct lt9711 *lt9711 = dev_get_drvdata(dev);

	DRM_DEBUG("%s()\n", __func__);

	gpiod_direction_output(lt9711->enable_gpio, 1);
	usleep_range(50*1000, 100*1000); //100ms

	if (!lt9711->detect_work_pending) {
		schedule_delayed_work(&lt9711->detect_work,
				msecs_to_jiffies(2000));
		lt9711->detect_work_pending = true;
	}

	return 0;
}

#endif

static const struct dev_pm_ops lt9711_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(lt9711_drv_pm_suspend,
				lt9711_drv_pm_resume)
};

static struct i2c_device_id lt9711_id[] = {
	{ "lontium,lt9711", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, lt9711_id);

static const struct of_device_id lt9711_match_table[] = {
	{ .compatible = "lontium,lt9711" },
	{ }
};
MODULE_DEVICE_TABLE(of, lt9711_match_table);

static struct i2c_driver lt9711_driver = {
	.driver = {
		.name = "lt9711",
		.of_match_table = lt9711_match_table,
		.pm = &lt9711_pm_ops,
	},
	.probe = lt9711_probe,
	.remove = lt9711_remove,
	.id_table = lt9711_id,
};

static int lt9711_dsi_probe(struct mipi_dsi_device *dsi)
{
	int ret;
	struct mipi_dsi_host *host;
	struct drm_panel *panel;
	struct ky_dsi *mipi_dsi;
	struct lt9711 *lt9711;

	DRM_DEBUG("%s()\n", __func__);

	dsi->lanes = 4;
	dsi->mode_flags = (MIPI_DSI_MODE_VIDEO |
				MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
				MIPI_DSI_MODE_LPM);
	dsi->format = MIPI_DSI_FMT_RGB888;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(&dsi->dev, "failed to attach dsi to host\n");
		mipi_dsi_device_unregister(dsi);
	} else {
		host = dsi->host;
		mipi_dsi = host_to_dsi(host);

		panel = mipi_dsi->panel;
		lt9711 = panel_to_lt9711(panel);

		mipi_dsi->ctx.dsi_subconnector = KY_DSI_SUBCONNECTOR_DP;
		mipi_dsi->ctx.previous_connector_status = lt9711->status;
		mipi_dsi->ctx.connector_status = mipi_dsi->ctx.previous_connector_status;

		lt9711->connector = &mipi_dsi->connector;
		lt9711->ky_dsi = &mipi_dsi->ctx;
	}

	return ret;
}

static const struct of_device_id ky_dp_of_match[] = {
	{ .compatible = "ky,dp-panel" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ky_edp_of_match);

static struct mipi_dsi_driver lt9711_dsi_driver = {
	.probe = lt9711_dsi_probe,
	.driver = {
		.name = IT9711_DSI_DRIVER_NAME,
		.of_match_table = ky_dp_of_match,
	},
};


static int __init init_lt9711(void)
{
	int err;

	DRM_DEBUG("%s()\n", __func__);

	mipi_dsi_driver_register(&lt9711_dsi_driver);
	err = i2c_add_driver(&lt9711_driver);

	return err;
}

module_init(init_lt9711);

static void __exit exit_lt9711(void)
{
	DRM_DEBUG("%s()\n", __func__);

	i2c_del_driver(&lt9711_driver);
	mipi_dsi_driver_unregister(&lt9711_dsi_driver);
}
module_exit(exit_lt9711);

MODULE_DESCRIPTION("LT9711_MIPI to DP Reg Setting driver");
MODULE_LICENSE("GPL v2");

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <drm/drm_modes.h>
#include <linux/component.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/gpio.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-mipi-dphy.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <drm/drm_panel.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_property.h>
#include <drm/drm_print.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#include <drm/display/drm_dsc_helper.h>
#else
#include <drm/drm_dsc.h>
#endif
#include "sunxi-sid.h"
#include "sunxi_drm_drv.h"
#include "sunxi_device/sunxi_tcon.h"
#include "sunxi_drm_intf.h"
#include "sunxi_drm_crtc.h"
#include "panel/panels.h"
#include "sunxi_drm_debug.h"
#include <video/sunxi_drm_notify.h>
#define PHY_SINGLE_ENABLE 1
#define PHY_DUAL_ENABLE 2
#define MIPI_DSI_SYNC_INCELL	BIT(25)
#define MIPI_DSI_ASYNC_INCELL	BIT(26)

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) \
	|| IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
#define DSI_DISPLL_CLK
#endif

#if IS_ENABLED (CONFIG_DRM_FBDEV_EMULATION)
void dsi_notify_call_chain(int cmd, int flag);
#else
void sunxi_disp_notify_call_chain(int cmd, int flag);
#endif
struct dsi_data {
	int id;
};

struct esd_sw_wd {
	struct timer_list timer;
	struct work_struct feed_work;
	struct work_struct recovery_work;
	atomic_t esd_count;
	unsigned long timeout;
	struct sunxi_drm_dsi *dsi;
	atomic_t fed;
};

struct panel_data {
	u8 value[256];
	u32 len;
	u8 reg;
};
struct sunxi_drm_dsi {
	struct sunxi_drm_device sdrm;
	struct mipi_dsi_host host;
	struct drm_display_mode mode;
	struct disp_dsi_para dsi_para;
	bool bound;
	bool sw_enable;
	bool pending_enable_vblank;
	struct device *dev;
	struct sunxi_drm_dsi *master;
	struct sunxi_drm_dsi *slave;
	struct phy *phy;
	union phy_configure_opts phy_opts;
	struct sunxi_dsi_lcd dsi_lcd;
	uintptr_t reg_base;
	struct resource *res;
	uintptr_t dsc_base;
	const struct dsi_data *dsi_data;
	struct drm_dsc_config *dsc;
	u32 enable;
	irq_handler_t irq_handler;
	void *irq_data;
	u32 irq_no;
	dev_t devid;
	struct panel_data panel_reg;

	struct clk *displl_ls;
	struct clk *displl_hs;
	struct clk *clk_bus;
	struct clk *clk;
	struct clk *combphy;
	struct clk *dsc_gating_clk;
	struct reset_control *dsc_rst_clk;
	struct reset_control *rst_bus;
	unsigned long hs_clk_rate;
	unsigned long ls_clk_rate;
	bool displl_clk;
	unsigned int pll_ss_permille;

	struct esd_sw_wd *esd_wdt;
	struct gpio_desc *te_gpio;
	int te_irq;
	struct workqueue_struct *panel_wq;
	struct work_struct panel_work;
};
static const struct dsi_data dsi0_data = {
	.id = 0,
};

static const struct dsi_data dsi1_data = {
	.id = 1,
};

static const struct of_device_id sunxi_drm_dsi_match[] = {
	{ .compatible = "allwinner,dsi0", .data = &dsi0_data },
	{ .compatible = "allwinner,dsi1", .data = &dsi1_data },
	{},
};

static void sunxi_dsi_enable_vblank(bool enable, void *data);
static ssize_t sunxi_drm_dsi_transfer(struct sunxi_drm_dsi *dsi,
				const struct mipi_dsi_msg *msg);

static void __maybe_unused esd_watchdog_start(struct esd_sw_wd *esd_wdt)
{
	mod_timer(&esd_wdt->timer, jiffies + esd_wdt->timeout);
}

static void __maybe_unused esd_watchdog_stop(struct esd_sw_wd *esd_wdt)
{
	del_timer_sync(&esd_wdt->timer);
}

static irqreturn_t te_irq_handler(int irq, void *data)
{
	struct sunxi_drm_dsi *dsi = data;

	schedule_work(&dsi->esd_wdt->feed_work);
	return IRQ_HANDLED;
}

static int panel_dsi_register_te_irq(struct sunxi_drm_dsi *dsi)
{
	int ret = -1;
	unsigned int te_irq;

	dsi->te_gpio =
		devm_gpiod_get_optional(dsi->dev, "te", GPIOD_IN);
	if (IS_ERR(dsi->te_gpio)) {
		ret = PTR_ERR(dsi->te_gpio);
		DRM_ERROR("%s:%d failed to request %s GPIO: %d\n", __FUNCTION__, __LINE__, "te", ret);
		return ret;
	} else if (!dsi->te_gpio) {
		DRM_WARN("TE GPIO not configured\n");
		return ret;
	} else {
		DRM_INFO("TE GPIO successfully requested\n");
	}

	if (dsi->te_gpio) {
		te_irq = gpiod_to_irq(dsi->te_gpio);

		irq_set_status_flags(te_irq, IRQ_DISABLE_UNLAZY);

		ret = devm_request_irq(dsi->dev, te_irq, te_irq_handler,
								IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
								"TE_GPIO", dsi);
		if (ret) {
			dev_err(dsi->dev, "Failed to request TE IRQ\n");
			irq_clear_status_flags(te_irq, IRQ_DISABLE_UNLAZY);
			return ret;
		}

		DRM_INFO("te_irq has been register. %s:%d\n", __FUNCTION__, __LINE__);
	}

	return ret;
}

static void esd_feed_watchdog(struct work_struct *work)
{
	struct esd_sw_wd *esd_wdt = container_of(work, struct esd_sw_wd, feed_work);

	atomic_set(&esd_wdt->fed, 1);
	esd_watchdog_start(esd_wdt);
}

static void display_recovery_work(struct work_struct *work)
{
	struct sunxi_drm_device *sdrm;
	struct drm_crtc *crtc;
	struct drm_connector *connector;
	struct drm_crtc_state *crtc_state;
	struct sunxi_crtc_state *scrtc_state;
	struct esd_sw_wd *esd_wdt = container_of(work, struct esd_sw_wd, recovery_work);

	sdrm = &esd_wdt->dsi->sdrm;
	crtc = sdrm->encoder.crtc;
	crtc_state = crtc->state;
	connector = &sdrm->connector;
	scrtc_state = to_sunxi_crtc_state(crtc_state);

	atomic_inc(&esd_wdt->esd_count);
	DRM_WARN("%s:%d esd_count:%d\n", __FUNCTION__, __LINE__, atomic_read(&esd_wdt->esd_count));

	if (!atomic_read(&esd_wdt->fed)) {
		drm_mode_config_helper_suspend(sdrm->drm_dev);
		mdelay(10);
		drm_mode_config_helper_resume(sdrm->drm_dev);
		// drm_kms_helper_hotplug_event(sdrm->drm_dev);
	}
}

static void esd_watchdog_timeout(struct timer_list *t)
{
	struct esd_sw_wd *esd_wdt = from_timer(esd_wdt, t, timer);

	atomic_set(&esd_wdt->fed, 0);
	schedule_work(&esd_wdt->recovery_work);
}

static int esd_watchdog_init(struct sunxi_drm_dsi *dsi)
{
	int ret;

	DRM_INFO("Loading esd watchdog module...\n");
	ret = panel_dsi_register_te_irq(dsi);
	if (ret) {
		DRM_ERROR("Failed to request TE IRQ\n");
		return ret;
	}

	dsi->esd_wdt = devm_kzalloc(dsi->dev, sizeof(struct esd_sw_wd), GFP_KERNEL);
	if (!dsi->esd_wdt) {
		DRM_ERROR("Failed to allocate memory for watchdog\n");
		return -1;
	}

	dsi->esd_wdt->timeout = HZ;
	atomic_set(&dsi->esd_wdt->fed, 1);
	atomic_set(&dsi->esd_wdt->esd_count, 0);
	INIT_WORK(&dsi->esd_wdt->feed_work, esd_feed_watchdog);
	INIT_WORK(&dsi->esd_wdt->recovery_work, display_recovery_work);

	timer_setup(&dsi->esd_wdt->timer, esd_watchdog_timeout, 0);

	dsi->esd_wdt->dsi = dsi;

	return 0;
}

static int __maybe_unused esd_watchdog_exit(struct sunxi_drm_dsi *dsi)
{
	struct esd_sw_wd *esd_wdt;

	DRM_INFO("Unloading esd watchdog module...\n");
	esd_wdt = dsi->esd_wdt;
	disable_irq(dsi->te_irq);
	synchronize_irq(dsi->te_irq);

	// TODO:Maybe need to add much more exit code.
	cancel_work_sync(&esd_wdt->recovery_work);
	cancel_work_sync(&esd_wdt->feed_work);

	esd_watchdog_stop(esd_wdt);

	return 0;
}

static struct device *drm_dsi_of_get_tcon(struct device *dsi_dev)
{
	struct device_node *node = dsi_dev->of_node;
	struct device_node *tcon_lcd_node;
	struct device_node *dsi_in_tcon;
	struct platform_device *pdev = NULL;
	struct device *tcon_lcd_dev = NULL;;

	dsi_in_tcon = of_graph_get_endpoint_by_regs(node, 0, 0);
	if (!dsi_in_tcon) {
		DRM_ERROR("endpoint dsi_in_tcon not fount\n");
		return NULL;
	}

	tcon_lcd_node = of_graph_get_remote_port_parent(dsi_in_tcon);
	if (!tcon_lcd_node) {
		DRM_ERROR("node tcon_lcd not fount\n");
		tcon_lcd_dev = NULL;
		goto DSI_PUT;
	}

	pdev = of_find_device_by_node(tcon_lcd_node);
	if (!pdev) {
		DRM_ERROR("tcon_lcd platform device not fount\n");
		tcon_lcd_dev = NULL;
		goto TCON_DSI_PUT;
	}

	tcon_lcd_dev = &pdev->dev;
	platform_device_put(pdev);

TCON_DSI_PUT:
	of_node_put(tcon_lcd_node);
DSI_PUT:
	of_node_put(dsi_in_tcon);

	return tcon_lcd_dev;
}

static inline struct sunxi_drm_dsi *host_to_sunxi_drm_dsi(struct mipi_dsi_host *host)
{
	return container_of(host, struct sunxi_drm_dsi, host);
}
static inline struct sunxi_drm_dsi *encoder_to_sunxi_drm_dsi(struct drm_encoder *encoder)
{
	struct sunxi_drm_device *sdrm = container_of(encoder, struct sunxi_drm_device, encoder);

	return container_of(sdrm, struct sunxi_drm_dsi, sdrm);
}

static inline struct sunxi_drm_dsi *connector_to_sunxi_drm_dsi(struct drm_connector *connector)
{
	struct sunxi_drm_device *sdrm = container_of(connector, struct sunxi_drm_device, connector);

	return container_of(sdrm, struct sunxi_drm_dsi, sdrm);
}

static int sunxi_drm_dsi_find_slave(struct sunxi_drm_dsi *dsi)
{
	struct device_node *node = NULL;
	struct platform_device *pdev;

	node = of_parse_phandle(dsi->dev->of_node, "dual-channel", 0);
	if (node) {
		pdev = of_find_device_by_node(node);
		if (!pdev) {
			of_node_put(node);
			return -EPROBE_DEFER;
		}

		dsi->slave = platform_get_drvdata(pdev);
		if (!dsi->slave) {
			platform_device_put(pdev);
			return -EPROBE_DEFER;
		}

		of_node_put(node);
		platform_device_put(pdev);

	}

	return 0;
}

static int sunxi_dsi_clk_config_enable(struct sunxi_drm_dsi *dsi)
{
	int ret = 0;
	unsigned long dsi_rate = 0;
	unsigned long combphy_rate = 0;
	unsigned long dsi_rate_set = 150000000;

	clk_set_rate(dsi->clk, dsi_rate_set);
	dsi_rate = clk_get_rate(dsi->clk);
	if (dsi_rate_set != dsi_rate)
		DRM_WARN("Dsi rate to be set:%lu, real clk rate:%lu\n",
			dsi_rate, dsi_rate_set);

	if (dsi->combphy) {
		clk_set_rate(dsi->combphy, dsi->hs_clk_rate);
		combphy_rate = clk_get_rate(dsi->combphy);
		DRM_INFO("combphy rate to be set:%lu, real clk rate:%lu\n",
				dsi->hs_clk_rate, combphy_rate);
		ret = clk_prepare_enable(dsi->combphy);
		if (ret) {
			DRM_ERROR("clk_prepare_enable for combphy_clk failed\n");
			return ret;
		}
	}
	ret = reset_control_deassert(dsi->rst_bus);
	if (ret) {
		DRM_ERROR(
			"reset_control_deassert for dsi_rst_clk failed\n");
		return ret;
	}
	ret = clk_prepare_enable(dsi->clk_bus);
	if (ret) {
		DRM_ERROR("clk_prepare_enable for dsi_gating_clk failed\n");
		return ret;
	}
	ret = clk_prepare_enable(dsi->clk);
	if (ret) {
		DRM_ERROR("clk_prepare_enable for dsi_clk failed\n");
		return ret;
	}

	if (dsi->dsc) {
		ret = reset_control_deassert(dsi->dsc_rst_clk);
		if (ret) {
			DRM_ERROR("reset_control_deassert for rst_bus_dsc failed\n");
			return ret;
		}

		ret = clk_prepare_enable(dsi->dsc_gating_clk);
		if (ret) {
			DRM_ERROR("clk_prepare_enable for clk_bus_dsc failed\n");
			return ret;
		}
	}

	return ret;
}

static int sunxi_dsi_clk_config_disable(struct sunxi_drm_dsi *dsi)
{
	int ret = 0;

	/* Since the registers were not saved, changes are made here to update the registers when enabled next time. */
	clk_disable_unprepare(dsi->clk_bus);
	clk_disable_unprepare(dsi->clk);
	if (dsi->combphy)
		clk_disable_unprepare(dsi->combphy);
	ret = reset_control_assert(dsi->rst_bus);
	if (ret) {
		DRM_ERROR("reset_control_assert for rst_bus_mipi_dsi failed\n");
		return ret;
	}
	if (dsi->dsc) {
		ret = reset_control_assert(dsi->dsc_rst_clk);
		if (ret) {
			DRM_ERROR("reset_control_assert for rst_bus_dsc failed\n");
			return ret;
		}

		clk_disable_unprepare(dsi->dsc_gating_clk);
	}
	return ret;
}

void sunxi_dsi_panel_check(struct work_struct *work)
{
	struct sunxi_drm_dsi *dsi = container_of(work, struct sunxi_drm_dsi, panel_work);
	u8 cmd = dsi->panel_reg.reg;

	struct mipi_dsi_msg msg = {
		.channel = dsi->dsi_para.channel,
		.type = 0x06,
		.tx_buf = &cmd,
		.tx_len = 1,
		.rx_buf = dsi->panel_reg.value,
		.rx_len = dsi->panel_reg.len,
	};
	if (!msg.rx_len)
		return;
	dsi_read_mode_en(&dsi->dsi_lcd, 1);
	sunxi_drm_dsi_transfer(dsi, &msg);
	dsi_read_mode_en(&dsi->dsi_lcd, 0);
}

static irqreturn_t sunxi_dsi_irq_event_proc(int irq, void *parg)
{
	struct sunxi_drm_dsi *dsi = parg;
	struct disp_video_timings *timings = &dsi->dsi_para.timings;
	struct disp_video_timings timing_t;
	u32 dsi_line = 0;
	static u32 a;

	if (dsi_irq_query(&dsi->dsi_lcd, DSI_IRQ_VIDEO_LINE)) {
		if (sunxi_get_soc_ver() == 0)
			sunxi_dsi_vrr_irq(&dsi->dsi_lcd, timings, false);
		else {
			dsi_line = dsi_get_real_cur_line(&dsi->dsi_lcd);
			dsi_get_timing(&dsi->dsi_lcd, &timing_t);
			if (dsi_line >= 1 && dsi_line < (timing_t.ver_sync_time - 5)) {
				sunxi_tcon_vfp_vrr_set(dsi->sdrm.tcon_dev, timings);
				sunxi_dsi_updata_vt_2(&dsi->dsi_lcd, timings);
				DRM_INFO("[DSI-VRR] dsi_line:%d okkkk\n", dsi_line);
			} else
				DRM_WARN("[DSI-VRR] dsi_line:%d errrr\n", dsi_line);
		}

		return IRQ_HANDLED;
	}
	dsi_line = dsi_get_cur_line(&dsi->dsi_lcd);
	if (!(a % 120) && dsi_line < (timings->ver_sync_time + timings->ver_back_porch))
		queue_work(dsi->panel_wq, &dsi->panel_work);
	a++;
	dsi_irq_query(&dsi->dsi_lcd, DSI_IRQ_VIDEO_VBLK);

	return dsi->irq_handler(irq, dsi->irq_data);
}

static int sunxi_lcd_pin_set_state(struct device *dev, char *name)
{
	int ret;
	struct pinctrl *pctl;
	struct pinctrl_state *state;

	DRM_INFO("[DSI] %s start\n", __FUNCTION__);
	pctl = pinctrl_get(dev);
	if (IS_ERR(pctl)) {
		DRM_INFO("[WARN]can NOT get pinctrl for %lx \n",
			(unsigned long)dev);
		ret = 0;
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		DRM_ERROR("pinctrl_lookup_state for %lx fail\n",
			(unsigned long)dev);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		DRM_ERROR("pinctrl_select_state(%s) for %lx fail\n", name,
			(unsigned long)dev);
		goto exit;
	}

exit:
	return ret;
}
static int sunxi_dsi_displl_enable(struct sunxi_drm_dsi *dsi)
{
	if (dsi->displl_hs)
		clk_set_rate(dsi->displl_hs, dsi->hs_clk_rate);
	if (dsi->displl_ls)
		clk_set_rate(dsi->displl_ls, dsi->ls_clk_rate);
	if (dsi->displl_ls)
		clk_prepare_enable(dsi->displl_ls);
	return 0;
}

static int sunxi_dsi_displl_disable(struct sunxi_drm_dsi *dsi)
{
	if (dsi->displl_hs)
		clk_set_rate(dsi->displl_hs, 48000000);
	if (dsi->displl_ls)
		clk_set_rate(dsi->displl_ls, 24000000);
	if (dsi->displl_ls)
		clk_disable_unprepare(dsi->displl_ls);
	return 0;
}

static int sunxi_dsi_enable_output(struct sunxi_drm_dsi *dsi)
{
	struct disp_dsi_para *dsi_para = &dsi->dsi_para;

	sunxi_tcon_dsi_enable_output(dsi->sdrm.tcon_dev);
	dsi_open_hs_mode(&dsi->dsi_lcd, dsi_para);
	if (dsi->slave)
		dsi_open_hs_mode(&dsi->slave->dsi_lcd, dsi_para);

	return 0;
}

static int sunxi_dsi_disable_output(struct sunxi_drm_dsi *dsi)
{
	dsi_close(&dsi->dsi_lcd);
	if (dsi->slave)
		dsi_close(&dsi->slave->dsi_lcd);
	sunxi_tcon_dsi_disable_output(dsi->sdrm.tcon_dev);

	return 0;
}

static int dsi_populate_dsc_params(struct sunxi_drm_dsi *dsi, struct drm_dsc_config *dsc)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	int ret;
#endif

	if (dsc->bits_per_pixel & 0xf) {
		DRM_ERROR("DSI does not support fractional bits_per_pixel\n");
		return -EINVAL;
	}

	if (dsc->bits_per_component != 8) {
		DRM_ERROR("DSI does not support bits_per_component != 8 yet\n");
		return -EOPNOTSUPP;
	}

	dsc->simple_422 = 0;
	dsc->convert_rgb = 1;
	dsc->vbr_enable = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	drm_dsc_set_const_params(dsc);
	drm_dsc_set_rc_buf_thresh(dsc);

	/* handle only bpp = bpc = 8, pre-SCR panels */
	ret = drm_dsc_setup_rc_params(dsc, DRM_DSC_1_1_PRE_SCR);
	if (ret) {
		DRM_ERROR("could not find DSC RC parameters\n");
		return ret;
	}

	dsc->initial_scale_value = drm_dsc_initial_scale_value(dsc);
/*
#else
	before Linux-6.1, you need to implement it yourself
*/
#endif
	dsc->line_buf_depth = dsc->bits_per_component + 1;

	return drm_dsc_compute_rc_parameters(dsc);
}

static void dsi_timing_setup(struct sunxi_drm_dsi *dsi, struct disp_video_timings *timing)
{
	int ret;
	struct drm_dsc_config *dsc = dsi->dsc;

	dsc->pic_width = timing->x_res;
	dsc->pic_height = timing->y_res;

	ret = dsi_populate_dsc_params(dsi, dsc);
	if (ret)
		DRM_ERROR("One or more PPS parameters exceeded their allowed bit depth.");
}

static int sunxi_drm_dsi_set_vbp(struct device *dev)
{
	struct sunxi_drm_dsi *dsi = dev_get_drvdata(dev);
	struct disp_video_timings *timings = &dsi->dsi_para.timings;
	u32 dsi_line;

	dsi_line = sunxi_dsi_updata_vt(&dsi->dsi_lcd, timings, dsi->dsi_para.vrr_setp);

	return dsi_line;

}

static int sunxi_drm_dsi_get_line(struct device *dev)
{
	struct sunxi_drm_dsi *dsi = dev_get_drvdata(dev);
	u32 dsi_line;

	dsi_line = dsi_get_real_cur_line(&dsi->dsi_lcd);

	return dsi_line;

}

void sunxi_drm_dsi_encoder_atomic_enable(struct drm_encoder *encoder,
					struct drm_atomic_state *state)
{
	int ret, bpp, lcd_div;
	struct drm_crtc *crtc = encoder->crtc;
	int de_hw_id = sunxi_drm_crtc_get_hw_id(crtc);
	struct drm_crtc_state *crtc_state = crtc->state;
	struct sunxi_drm_dsi *dsi = encoder_to_sunxi_drm_dsi(encoder);
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc_state);
	struct disp_output_config disp_cfg;

	drm_mode_to_sunxi_video_timings(&dsi->mode, &dsi->dsi_para.timings);
	bpp = mipi_dsi_pixel_format_to_bpp(dsi->dsi_para.format);

	if (dsi->slave)
		lcd_div = bpp / (dsi->dsi_para.lanes * 2);
	else if (dsi->dsc)
		lcd_div = dsi->dsc->bits_per_component / dsi->dsi_para.lanes;
	else
		lcd_div = bpp / dsi->dsi_para.lanes;

	memset(&disp_cfg, 0, sizeof(struct disp_output_config));
	memcpy(&disp_cfg.dsi_para, &dsi->dsi_para,
		sizeof(dsi->dsi_para));
	disp_cfg.type = INTERFACE_DSI;
	disp_cfg.de_id = de_hw_id;
	disp_cfg.irq_handler = sunxi_crtc_event_proc;
	disp_cfg.irq_data = scrtc_state->base.crtc;
	disp_cfg.sw_enable = dsi->sw_enable;
	disp_cfg.dev = dsi->dev;
#ifdef DSI_DISPLL_CLK
	disp_cfg.displl_clk = true;
	disp_cfg.tcon_lcd_div = 1;
#else
	disp_cfg.displl_clk = false;
	disp_cfg.tcon_lcd_div = lcd_div;
#endif
	dsi->displl_clk = disp_cfg.displl_clk;
	if (dsi->slave || (dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE))
		disp_cfg.slave_dsi = true;

	disp_cfg.set_dsi_vbp = sunxi_drm_dsi_set_vbp;
	disp_cfg.get_dsi_line = sunxi_drm_dsi_get_line;
	if (dsi->enable) {
		if (disp_cfg.slave_dsi) {
			if (sunxi_get_soc_ver() == 0)
				sunxi_tcon_vrr_set(dsi->sdrm.tcon_dev, &disp_cfg);
			else
				sunxi_dsi_vfp_vrr_irq(&dsi->dsi_lcd, &dsi->dsi_para.timings);
		} else
			sunxi_dsi_vrr_irq(&dsi->dsi_lcd, &dsi->dsi_para.timings, true);
		DRM_DEBUG_KMS("[DSI-VRR] set mode: " DRM_MODE_FMT "\n", DRM_MODE_ARG(&dsi->mode));
		return;
	}
	DRM_INFO("[DSI] %s vrr_setp:%d start\n", __FUNCTION__, dsi->dsi_para.vrr_setp);
	dsi->enable = true;

	if (dsi->phy) {
		phy_mipi_dphy_get_default_config(dsi->dsi_para.timings.pixel_clk,
					bpp, dsi->dsi_para.lanes, &dsi->phy_opts.mipi_dphy);
	}

	sunxi_tcon_mode_init(dsi->sdrm.tcon_dev, &disp_cfg);

	/* dual dsi use tcon's irq, single dsi use its own irq */
	if (!disp_cfg.slave_dsi || (sunxi_get_soc_ver() != 0 && dsi->dsc)) {
		dsi->irq_handler = sunxi_crtc_event_proc;
		dsi->irq_data = scrtc_state->base.crtc;
		ret = devm_request_irq(dsi->dev, dsi->irq_no, sunxi_dsi_irq_event_proc,
					0, dev_name(dsi->dev), dsi);
		if (ret) {
			DRM_ERROR("Couldn't request the IRQ for dsi\n");
		}
	}

	if (dsi->dsi_para.mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
		dsi->dsi_para.timings.pixel_clk = dsi->dsi_para.timings.pixel_clk * 10 / 9;
	dsi->ls_clk_rate = dsi->dsi_para.timings.pixel_clk;
	dsi->hs_clk_rate = dsi->dsi_para.timings.pixel_clk * lcd_div;
	ret = sunxi_dsi_clk_config_enable(dsi);
	if (ret) {
		DRM_ERROR("dsi clk enable failed\n");
		return;
	}
	if (dsi->slave) {
		ret = sunxi_dsi_clk_config_enable(dsi->slave);
		if (ret) {
			DRM_ERROR("slave clk enable failed\n");
			return;
		}
	}

	sunxi_lcd_pin_set_state(dsi->dev, "active");
	if (dsi->slave) {
		sunxi_lcd_pin_set_state(dsi->slave->dev, "active");
	}

	if (dsi->sw_enable) {
		if (dsi->phy) {
			phy_power_on(dsi->phy);
			if (dsi->displl_ls)
				clk_prepare_enable(dsi->displl_ls);
		}
		if (dsi->slave) {
			if (dsi->slave->phy)
				phy_power_on(dsi->slave->phy);
		}
		panel_dsi_regulator_enable(dsi->sdrm.panel);
	} else {
		if (dsi->dsc) {
			tcon_lcd_dsc_src_sel();
			dsi_timing_setup(dsi, &dsi->dsi_para.timings);
			dsi->dsi_para.timings.x_res /= (bpp / dsi->dsc->bits_per_component);
			dsi->dsi_para.timings.hor_total_time /= (bpp / dsi->dsc->bits_per_component);
			dsi->dsi_para.timings.hor_back_porch /= (bpp / dsi->dsc->bits_per_component);
			dsi->dsi_para.timings.hor_sync_time /= (bpp / dsi->dsc->bits_per_component);
			dsc_config_pps(&dsi->dsi_lcd, dsi->dsc);
			dec_dsc_config(&dsi->dsi_lcd, &dsi->dsi_para.timings);
		}
		if (dsi->phy) {
			phy_power_on(dsi->phy);
			phy_set_mode_ext(dsi->phy, PHY_MODE_MIPI_DPHY, PHY_SINGLE_ENABLE);
			phy_configure(dsi->phy, &dsi->phy_opts);
		}
		dsi_cfg(&dsi->dsi_lcd, &dsi->dsi_para);
		if (dsi->slave) {
			dsi_cfg(&dsi->slave->dsi_lcd, &dsi->dsi_para);
			if (dsi->slave->phy) {
				phy_power_on(dsi->slave->phy);
				phy_set_mode_ext(dsi->phy, PHY_MODE_MIPI_DPHY, PHY_DUAL_ENABLE);
				phy_set_mode_ext(dsi->slave->phy, PHY_MODE_MIPI_DPHY, PHY_DUAL_ENABLE);
				phy_configure(dsi->slave->phy, &dsi->phy_opts);
			}
		}
		drm_panel_prepare(dsi->sdrm.panel);

		sunxi_dsi_displl_enable(dsi);
		if (dsi->pll_ss_permille)
			phy_set_speed(dsi->phy, dsi->pll_ss_permille);

		ret = sunxi_dsi_enable_output(dsi);
		if (ret < 0)
			DRM_DEV_INFO(dsi->dev, "failed to enable dsi ouput\n");
		if (dsi->dsi_para.mode_flags & (MIPI_DSI_ASYNC_INCELL | MIPI_DSI_SYNC_INCELL))
#if IS_ENABLED (CONFIG_DRM_FBDEV_EMULATION)
			dsi_notify_call_chain(SUNXI_PANEL_EVENT_UNBLANK,
					dsi->dsi_para.mode_flags & MIPI_DSI_SYNC_INCELL);
#else
			sunxi_disp_notify_call_chain(SUNXI_PANEL_EVENT_UNBLANK,
					dsi->dsi_para.mode_flags & MIPI_DSI_SYNC_INCELL);
#endif
	}
	if (dsi->pending_enable_vblank) {
		sunxi_dsi_enable_vblank(1, dsi);
		dsi->pending_enable_vblank = false;
	}
	drm_panel_enable(dsi->sdrm.panel);
	if (dsi->te_gpio)
		esd_watchdog_start(dsi->esd_wdt);
	DRM_INFO("[DSI] %s finish\n", __FUNCTION__);
}

void sunxi_drm_dsi_encoder_atomic_disable(struct drm_encoder *encoder,
					struct drm_atomic_state *state)
{
	struct sunxi_drm_dsi *dsi = encoder_to_sunxi_drm_dsi(encoder);
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(encoder->crtc->state);

	if (!dsi->enable) {
		DRM_INFO("[DSI] isn't enabled\n");
		return;
	}

	if (scrtc_state->frame_rate_change) {
		DRM_DEBUG_DRIVER("[DSI] skip disable/enable for VRR modeset\n");
		return;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	dsi->sdrm.panel->prepare_prev_first = false;
#endif
	if (dsi->dsi_para.mode_flags & (MIPI_DSI_ASYNC_INCELL | MIPI_DSI_SYNC_INCELL))
#if IS_ENABLED (CONFIG_DRM_FBDEV_EMULATION)
		dsi_notify_call_chain(SUNXI_PANEL_EVENT_BLANK,
				dsi->dsi_para.mode_flags & MIPI_DSI_SYNC_INCELL);
#else
		sunxi_disp_notify_call_chain(SUNXI_PANEL_EVENT_BLANK,
				dsi->dsi_para.mode_flags & MIPI_DSI_SYNC_INCELL);
#endif
	if (dsi->te_gpio)
		esd_watchdog_stop(dsi->esd_wdt);
	drm_panel_disable(dsi->sdrm.panel);

	drm_panel_unprepare(dsi->sdrm.panel);

	sunxi_dsi_displl_disable(dsi);
	if (dsi->phy) {
		phy_power_off(dsi->phy);
	}
	if (dsi->slave) {
		if (dsi->slave->phy)
			phy_power_off(dsi->phy);
	}

	sunxi_dsi_clk_config_disable(dsi);
	if (dsi->slave)
		sunxi_dsi_clk_config_disable(dsi->slave);

	sunxi_lcd_pin_set_state(dsi->dev, "sleep");
	if (dsi->slave)
		sunxi_lcd_pin_set_state(dsi->slave->dev, "sleep");
	sunxi_dsi_disable_output(dsi);
	sunxi_tcon_mode_exit(dsi->sdrm.tcon_dev);

	if (!(dsi->slave || (dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE)) ||
			(sunxi_get_soc_ver() != 0 && dsi->dsc))
		devm_free_irq(dsi->dev, dsi->irq_no, dsi);

	dsi->enable = false;
	DRM_DEBUG_DRIVER("%s finish\n", __FUNCTION__);
}

static bool sunxi_dsi_fifo_check(void *data)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)data;
	int status;

	if (dsi->slave || (dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE))
		status = sunxi_tcon_check_fifo_status(dsi->sdrm.tcon_dev);
	else
		status = dsi_get_status(&dsi->dsi_lcd);

	return status ? true : false;
}

int sunxi_dsi_get_current_line(void *data)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)data;
	if (dsi->slave || (dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE))
		return sunxi_tcon_get_current_line(dsi->sdrm.tcon_dev);
	else
		return dsi_get_cur_line(&dsi->dsi_lcd);
}

static bool sunxi_dsi_is_sync_time_enough(void *data)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)data;
	return sunxi_tcon_is_sync_time_enough(dsi->sdrm.tcon_dev);
}

static void sunxi_dsi_enable_vblank(bool enable, void *data)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)data;

	if (!dsi->enable) {
		dsi->pending_enable_vblank = enable;
		return;
	}

	if (dsi->slave || (dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE))
		sunxi_tcon_enable_vblank(dsi->sdrm.tcon_dev, enable);
	else
		dsi_enable_vblank(&dsi->dsi_lcd, enable);

}

static bool sunxi_dsi_is_support_backlight(void *data)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)data;
	return panel_dsi_is_support_backlight(dsi->sdrm.panel);
}

static int sunxi_dsi_get_backlight_value(void *data)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)data;
	return panel_dsi_get_backlight_value(dsi->sdrm.panel);
}

static void sunxi_dsi_set_backlight_value(void *data, int brightness)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)data;
	panel_dsi_set_backlight_value(dsi->sdrm.panel, brightness);
}

int sunxi_drm_dsi_encoder_atomic_check(struct drm_encoder *encoder,
				struct drm_crtc_state *crtc_state,
				struct drm_connector_state *conn_state)
{
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc_state);
	struct sunxi_drm_dsi *dsi = encoder_to_sunxi_drm_dsi(encoder);

	scrtc_state->tcon_id = dsi->sdrm.tcon_id;
	scrtc_state->enable_vblank = sunxi_dsi_enable_vblank;
	scrtc_state->check_status = sunxi_dsi_fifo_check;
	scrtc_state->is_sync_time_enough = sunxi_dsi_is_sync_time_enough;
	scrtc_state->get_cur_line = sunxi_dsi_get_current_line;
	scrtc_state->is_support_backlight = sunxi_dsi_is_support_backlight;
	scrtc_state->get_backlight_value = sunxi_dsi_get_backlight_value;
	scrtc_state->set_backlight_value = sunxi_dsi_set_backlight_value;
	scrtc_state->output_dev_data = dsi;
	if (conn_state->crtc) {
		dsi->sw_enable = sunxi_drm_check_if_need_sw_enable(conn_state->connector);
		scrtc_state->sw_enable = dsi->sw_enable;
	}
	DRM_DEBUG_DRIVER("%s finish\n", __FUNCTION__);
	return 0;
}

static void sunxi_drm_dsi_encoder_mode_set(struct drm_encoder *encoder,
					struct drm_display_mode *mode,
					struct drm_display_mode *adj_mode)
{
	struct sunxi_drm_dsi *dsi = encoder_to_sunxi_drm_dsi(encoder);

	drm_mode_copy(&dsi->mode, adj_mode);
}
/*
static int sunxi_drm_dsi_encoder_loader_protect(struct drm_encoder *encoder,
		bool on)
{
	struct sunxi_drm_dsi *dsi = encoder_to_dsi(encoder);

	if (dsi->sdrm.panel)
		drm_panel_loader_protect(dsi->sdrm.panel, on);

	return sunxi_drm_dsi_loader_protect(dsi, on);
}
*/
static const struct drm_encoder_helper_funcs sunxi_dsi_encoder_helper_funcs = {
	.atomic_enable = sunxi_drm_dsi_encoder_atomic_enable,
	.atomic_disable = sunxi_drm_dsi_encoder_atomic_disable,
	.atomic_check = sunxi_drm_dsi_encoder_atomic_check,
	.mode_set = sunxi_drm_dsi_encoder_mode_set,
//	.loader_protect = sunxi_drm_dsi_encoder_loader_protect,
};

static int drm_dsi_connector_set_property(struct drm_connector *connector,
		struct drm_connector_state *state,
				struct drm_property *property,
				uint64_t val)
{
	return 0;

}
static int drm_dsi_connector_get_property(struct drm_connector *connector,
		const struct drm_connector_state *state,
				struct drm_property *property,
				uint64_t *val)
{
	return 0;

}

static void drm_dsi_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs sunxi_dsi_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_dsi_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.atomic_set_property = drm_dsi_connector_set_property,
	.atomic_get_property = drm_dsi_connector_get_property,
};

static int sunxi_dsi_connector_get_modes(struct drm_connector *connector)
{
	struct sunxi_drm_dsi *dsi = connector_to_sunxi_drm_dsi(connector);

	return drm_panel_get_modes(dsi->sdrm.panel, connector);
}

static const struct drm_connector_helper_funcs
	sunxi_dsi_connector_helper_funcs = {
	.get_modes = sunxi_dsi_connector_get_modes,
};

#if IS_ENABLED(CONFIG_PROC_FS)
static ssize_t panel_reg_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	struct sunxi_drm_dsi *dsi = pde_data(file_inode(file));
#else
	struct sunxi_drm_dsi *dsi = PDE_DATA(file_inode(file));
#endif
	char buf[256] = "";
	size_t len = 0, i;

	if (!dsi || *ppos > 0)
		return 0;

	for (i = 0; i < dsi->panel_reg.len; i++) {
	//	printk("***** 0x%x ******\n", dsi->panel_reg.value[i]);
		len += snprintf(buf + len, 256 - len, "%x ", dsi->panel_reg.value[i]);
		if (len >= 256) {
			len = 256 - 1;
			break;
		}
	}

	buf[len] = '\n';
	len += 1;

	if (copy_to_user(user_buf, buf, len))
		return -EFAULT;

	*ppos += len;

	return len;
}

static ssize_t panel_reg_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	struct sunxi_drm_dsi *dsi = pde_data(file_inode(file));
#else
	struct sunxi_drm_dsi *dsi = PDE_DATA(file_inode(file));
#endif
	char buf[32];
	unsigned long value1, value2;
	int ret;

	if (!dsi || count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	buf[count] = '\0';
	ret = sscanf(buf, "%lx %lx", &value1, &value2);
	if (ret != 2)
		return -EINVAL;

	dsi->panel_reg.reg = (u8)value1;
	dsi->panel_reg.len = (u32)value2;

	return count;
}

static const struct proc_ops panel_reg_fops = {
	.proc_read = panel_reg_read,
	.proc_write = panel_reg_write,
};

static ssize_t tcon_colorbar_show(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	struct sunxi_drm_dsi *dsi = pde_data(file_inode(file));
#else
	struct sunxi_drm_dsi *dsi = PDE_DATA(file_inode(file));
#endif
	char buf[32];
	unsigned long value;
	int ret;

	if (!dsi || count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	buf[count] = '\0';
	ret = sscanf(buf, "%lx", &value);
	if (ret != 1)
		return -EINVAL;

	sunxi_tcon_show_pattern(dsi->sdrm.tcon_dev, value);

	return count;
}

static const struct proc_ops colorbar_fops = {
	.proc_read = NULL,
	.proc_write = tcon_colorbar_show,
};

static ssize_t esd_show(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	struct sunxi_drm_dsi *dsi = pde_data(file_inode(file));
#else
	struct sunxi_drm_dsi *dsi = PDE_DATA(file_inode(file));
#endif
	struct esd_sw_wd *esd_wdt = dsi->esd_wdt;
	char buf[64] = "";
	size_t len = 0;

	if (!dsi || *ppos > 0)
		return 0;

	len += snprintf(buf, sizeof(buf), "esd_count:%d fed:%s\n", atomic_read(&esd_wdt->esd_count),
					atomic_read(&esd_wdt->fed) ? "TRUE" : "FALSE");

	if (copy_to_user(user_buf, buf, len))
		return -EFAULT;

	*ppos += len;

	return len;
}

static const struct proc_ops esd_fops = {
	.proc_read = esd_show,
	.proc_write = NULL,
};

static void print_physical_memory(struct drm_printer p, struct resource *res, size_t offset, size_t size)
{
	uintptr_t phys_addr = res->start;
	void __iomem *base;
	size_t i, j;

	base = ioremap(phys_addr + offset, size);

	for (i = 0; i < size; i += 16) {
		size_t line_size = min(size - i, (size_t)16);

		drm_printf(&p, "%08lx: ", (unsigned long)(phys_addr + i + offset));
		for (j = 0; j < line_size; j += 4)
			drm_printf(&p, "%08x ", readl(base + i + j));
		drm_printf(&p, "\n");
	}
	iounmap(base);
}

static int sunxi_drm_dsi_debug_show(struct seq_file *m, void *data)
{
	struct sunxi_drm_dsi *dsi = (struct sunxi_drm_dsi *)m->private;
	struct disp_video_timings timings;
	struct drm_printer p = drm_seq_file_printer(m);
	struct resource *res = NULL;
	unsigned long pclk = 0;
	u32 value;

	dsi_get_timing(&dsi->dsi_lcd, &timings);

	drm_printf(&p, "\t interface type: %s\n",
			dsi->slave ? "dual-dsi" : "single-dsi");
	drm_printf(&p, "\t dsi_mode: %s, \t 3dfifo: %s\n",
			dsi->dsi_para.mode_flags & MIPI_DSI_SLAVE_MODE ? "slave-dsi" : "master-dsi",
			dsi->dsi_para.mode_flags & MIPI_DSI_EN_3DFIFO ? "enabled" : "disabled");

	value = readl(ioremap(dsi->res->start + 0x10e0, 4));
	drm_printf(&p, "\t clk and data lane work mode:\n");
	drm_printf(&p, "\t\t  data0 | data1 | data2 | data3 | clk \n");
	drm_printf(&p, "\t\t -------+-------+-------+-------+------\n");
	drm_printf(&p, "\t\t   %2s   |  %2s   |  %2s   |  %2s   |  %2s   \n",
			(value & 0x7) == 5 ? "HS" : "LP",
			(value >> 4 & 0x7) == 5 ? "HS" : "LP",
			(value >> 8 & 0x7)  == 5 ? "HS" : "LP",
			(value >> 12 & 0x7) == 5 ? "HS" : "LP",
			(value >> 16 & 0x7) == 5 ? "HS" : "LP");
	drm_printf(&p, "\t clk source: %s\n", dsi->displl_clk ? "displl" : "ccmu");
	pclk = clk_get_rate(dsi->displl_ls);
	if (dsi->displl_clk) {
		pclk = clk_get_rate(dsi->displl_ls);
		drm_printf(&p, "\t\t pixel_clk rate to be set:%luKHz, real pixel_clk rate:%luKHz\n",
				dsi->ls_clk_rate / 1000, pclk / 1000);
	}
	drm_printf(&p, "\t  hsync-len | hback-porch |  hactive  | hfront-porch | vsync-len | vback-porch | vactive | vfront-porch \n");
	drm_printf(&p, "\t -----------+-------------+-----------+--------------+-----------+-------------+---------+--------------\n");
	drm_printf(&p, "\t     %3d    |    %4d     |   %4d    |     %4d     |    %3d    |    %4d     |   %4d  |     %4d\n",
			timings.hor_sync_time, timings.hor_back_porch, timings.x_res, timings.hor_front_porch,
			timings.ver_sync_time, timings.ver_back_porch, timings.y_res, timings.ver_front_porch);

	res = sunxi_tcon_get_res(dsi->sdrm.tcon_dev);
	drm_printf(&p, "\n******* tcon reg dump ********\n");
	if (res) {
		print_physical_memory(p, res, 0, 0x17c);
		print_physical_memory(p, res, 0x220, 0x30);
	}

	drm_printf(&p, "\n******* dsi reg dump ********\n");
	print_physical_memory(p, dsi->res, 0, 0x14c);
	print_physical_memory(p, dsi->res, 0x1f0, 0x10c);
	print_physical_memory(p, dsi->res, 0x1000, 0x13c);


	return 0;
}

static int sunxi_drm_dsi_procfs_init(struct sunxi_drm_dsi *dsi)
{
	static struct proc_dir_entry *lcd_dir;
	static struct proc_dir_entry *dir;
	char name[10];
	dir = proc_mkdir("lcd", NULL);

	if (IS_ERR_OR_NULL(dir)) {
		pr_err("Couldn't create lcd procfs directory !\n");
		return -ENOMEM;
	}
	snprintf(name, sizeof(name), "dsi%d", dsi->dsi_data->id);
	lcd_dir = proc_mkdir(name, dir);
	proc_create_data("panel_reg", 0664, lcd_dir, &panel_reg_fops, dsi);
	proc_create_data("colorbar", 0224, lcd_dir, &colorbar_fops, dsi);
	proc_create_single_data("status", 444, lcd_dir,
				sunxi_drm_dsi_debug_show, dsi);
	proc_create_data("esd", 0444, lcd_dir, &esd_fops, dsi);

	return 0;
}
#endif

static int sunxi_drm_dsi_bind(struct device *dev, struct device *master, void *data)
{
	struct sunxi_drm_dsi *dsi = dev_get_drvdata(dev);
	struct sunxi_drm_device *sdrm = &dsi->sdrm;
	struct drm_device *drm = (struct drm_device *)data;
	struct device *tcon_lcd_dev = NULL;
	int ret, tcon_id;

	DRM_INFO("[DSI]%s start\n", __FUNCTION__);
	dsi->dev = dev;
	ret = sunxi_drm_dsi_find_slave(dsi);
	if (ret)
		return ret;
	if (dsi->slave) {
		dsi->slave->master = dsi;
		dsi->dsi_para.dual_dsi = 1;
		DRM_INFO("[DSI]dsi%d slave is ok\n", dsi->sdrm.hw_id);
	}
	if (dsi->master)
		return 0;
	dsi->panel_wq = alloc_workqueue("dsi_wq", WQ_UNBOUND, 0);
	if (!dsi->panel_wq)
		DRM_ERROR("Failed to allocate workqueue\n");
	else
		INIT_WORK(&dsi->panel_work, sunxi_dsi_panel_check);

	tcon_lcd_dev = drm_dsi_of_get_tcon(dsi->dev);
	if (tcon_lcd_dev == NULL) {
		DRM_ERROR("tcon_lcd for dsi not found!\n");
		ret = -1;
		goto ERR_GROUP;
	}
	tcon_id = sunxi_tcon_of_get_id(tcon_lcd_dev);

	sdrm->tcon_dev = tcon_lcd_dev;
	sdrm->tcon_id = tcon_id;
	sdrm->drm_dev = drm;

	if (!dsi->sdrm.panel) {
		DRM_ERROR("[DSI]Failed to find panel\n");
		return -EPROBE_DEFER;
	}

	drm_encoder_helper_add(&sdrm->encoder, &sunxi_dsi_encoder_helper_funcs);
	ret = drm_simple_encoder_init(drm, &sdrm->encoder, DRM_MODE_ENCODER_DSI);
	if (ret) {
		DRM_ERROR("Couldn't initialise the encoder for tcon %d\n", tcon_id);
		goto ERR_GROUP;
	}

	sdrm->encoder.possible_crtcs =
			drm_of_find_possible_crtcs(drm, tcon_lcd_dev->of_node);

	drm_connector_helper_add(&sdrm->connector,
			&sunxi_dsi_connector_helper_funcs);

//	sdrm->connector.polled = DRM_CONNECTOR_POLL_HPD;
	ret = drm_connector_init(drm, &sdrm->connector,
			&sunxi_dsi_connector_funcs,
			DRM_MODE_CONNECTOR_DSI);
	if (ret) {
		drm_encoder_cleanup(&sdrm->encoder);
		DRM_ERROR("Couldn't initialise the connector for tcon %d\n", tcon_id);
		goto ERR_GROUP;
	}
//	drm_dsi_connector_init_property(drm, &sdrm->connector);

	drm_connector_attach_encoder(&sdrm->connector, &sdrm->encoder);
//	tcon_dev->cfg.private_data = dsi;

#if IS_ENABLED(CONFIG_PROC_FS)
	ret = sunxi_drm_dsi_procfs_init(dsi);
#endif
	dsi->bound = true;

	return 0;
ERR_GROUP:
	return ret;
}

static void sunxi_drm_dsi_unbind(struct device *dev, struct device *master,
				void *data)
{
	struct sunxi_drm_dsi *dsi = dev_get_drvdata(dev);

	drm_connector_cleanup(&dsi->sdrm.connector);
	drm_encoder_cleanup(&dsi->sdrm.encoder);

	dsi->bound = false;
	if (dsi->slave)
		dsi->slave = NULL;
}

static const struct component_ops sunxi_drm_dsi_component_ops = {
	.bind = sunxi_drm_dsi_bind,
	.unbind = sunxi_drm_dsi_unbind,
};

/* panel mipi_dsi_attach(dsi) */
static int sunxi_drm_dsi_host_attach(struct mipi_dsi_host *host,
				struct mipi_dsi_device *device)
{
	struct sunxi_drm_dsi *dsi = host_to_sunxi_drm_dsi(host);
	struct drm_panel *panel = of_drm_find_panel(device->dev.of_node);
	struct panel_dsi *dsi_panel = dev_get_drvdata(panel->dev);
	int ret;

	DRM_INFO("[DSI]%s start\n", __FUNCTION__);

	dsi->sdrm.panel = panel;
	dsi->dsi_para.dsi_div = 6;
	dsi->dsi_para.lanes = device->lanes;
	dsi->dsi_para.channel = device->channel;
	dsi->dsi_para.format = device->format;
	dsi->dsi_para.mode_flags = device->mode_flags;
	dsi->dsi_para.hs_rate = device->hs_rate;
	dsi->dsi_para.lp_rate = device->lp_rate;
	dsi->dsi_para.vrr_setp = dsi_panel->vrr_setp;
	dsi->pll_ss_permille = dsi_panel->pll_ss_permille;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (device->dsc)
		dsi->dsc = device->dsc;
#else
	if (dsi_panel->dsc)
		dsi->dsc = dsi_panel->dsc;
#endif

	ret = component_add(dsi->dev, &sunxi_drm_dsi_component_ops);
	if (ret) {
		DRM_ERROR("[DSI]%s component_add fail\n", __FUNCTION__);
		return ret;
	}

	DRM_INFO("[DSI]%s finish\n", __FUNCTION__);
	return 0;
}

static int sunxi_drm_dsi_host_detach(struct mipi_dsi_host *host,
				struct mipi_dsi_device *device)
{
	struct sunxi_drm_dsi *dsi = host_to_sunxi_drm_dsi(host);
	dsi->sdrm.panel = NULL;
	dsi->dsi_para.lanes = 0;
	dsi->dsi_para.channel = 0;
	dsi->dsi_para.format = 0;
	dsi->dsi_para.mode_flags = 0;
	dsi->dsi_para.hs_rate = 0;
	dsi->dsi_para.lp_rate = 0;
	memset(&dsi->dsi_para.timings, 0, sizeof(struct disp_video_timings));

	return 0;
}

static s32 sunxi_dsi_write_para(struct sunxi_drm_dsi *dsi, const struct mipi_dsi_msg *msg)
{
	struct mipi_dsi_packet packet;
	u32 ecc, crc, para_num;
	u8 *para = NULL;
	int ret = 0;

	/* create a packet to the DSI protocol */
	ret = mipi_dsi_create_packet(&packet, msg);
	if (ret) {
		DRM_ERROR("failed to create packet\n");
		return ret;
	}

	para = kmalloc(packet.size + 2, GFP_ATOMIC);
	if (!para) {
	//	printk("%s %s %s :kmalloc fail\n", __FILE__, __func__, __LINE__);
		return -1;
	}
	ecc = packet.header[0] | (packet.header[1] << 8) | (packet.header[2] << 16);
	para[0] = packet.header[0];
	para[1] = packet.header[1];
	para[2] = packet.header[2];
	para[3] = dsi_ecc_pro(ecc);
	para_num = 4;

	if (packet.payload_length) {
		memcpy(para + 4, packet.payload, packet.payload_length);
		crc = dsi_crc_pro((u8 *)packet.payload, packet.payload_length + 1);
		para[packet.size] = (crc >> 0) & 0xff;
		para[packet.size + 1] = (crc >> 8) & 0xff;
		para_num = packet.size + 2;
	}
	dsi_dcs_wr(&dsi->dsi_lcd, para, para_num);

	return 0;
}

static s32 sunxi_dsi_read_para(struct sunxi_drm_dsi *dsi, const struct mipi_dsi_msg *msg)
{
	s32 ret;
	struct mipi_dsi_msg max_pkt_size_msg;
	int i;
	u32 rx_cntr = msg->rx_len, rx_curr;
	u8 tx[2] = { 22 & 0xff, 22 >> 8 };
	u8 rx_bf[64], *rx_buf = msg->rx_buf;

	max_pkt_size_msg.channel = msg->channel;
	max_pkt_size_msg.type = 0x37;
	max_pkt_size_msg.tx_len = sizeof(tx);
	max_pkt_size_msg.tx_buf = tx;
	sunxi_dsi_write_para(dsi, &max_pkt_size_msg);

	while (rx_cntr) {
		if (rx_cntr >= 22)
			rx_curr = 22;
		else {
			rx_curr = rx_cntr;
			tx[0] = rx_curr & 0xff;
			tx[1] = rx_curr >> 8;
			max_pkt_size_msg.tx_buf = tx;
			sunxi_dsi_write_para(dsi, &max_pkt_size_msg);
		}
		rx_cntr -= rx_curr;
		sunxi_dsi_write_para(dsi, msg);
		ret = dsi_dcs_rd(&dsi->dsi_lcd, rx_bf, rx_curr);
		for (i = 0; i < rx_curr; i++) {
			*rx_buf = 0x00;
			*rx_buf |= *(rx_bf + i);
			rx_buf++;
		}
	}

	return ret;
}

static ssize_t sunxi_drm_dsi_transfer(struct sunxi_drm_dsi *dsi,
				const struct mipi_dsi_msg *msg)
{
	int ret = 0;
	bool is_read = (msg->rx_buf && msg->rx_len);

	if (is_read) {
		ret = sunxi_dsi_read_para(dsi, msg);
		return ret;
	} else
		sunxi_dsi_write_para(dsi, msg);

	if (dsi->slave)
		sunxi_drm_dsi_transfer(dsi->slave, msg);

	return msg->tx_len;
}

/* panel mipi_dsi_generic_write() mipi_dsi_dcs_write_buffer() */
static ssize_t sunxi_drm_dsi_host_transfer(struct mipi_dsi_host *host,
				const struct mipi_dsi_msg *msg)
{
	struct sunxi_drm_dsi *dsi = host_to_sunxi_drm_dsi(host);

	return sunxi_drm_dsi_transfer(dsi, msg);
}

static const struct mipi_dsi_host_ops sunxi_drm_dsi_host_ops = {
	.attach = sunxi_drm_dsi_host_attach,
	.detach = sunxi_drm_dsi_host_detach,
	.transfer = sunxi_drm_dsi_host_transfer,
};

static int sunxi_drm_dsi_probe(struct platform_device *pdev)
{
	struct sunxi_drm_dsi *dsi;
	struct resource *res;
	struct device *dev = &pdev->dev;
	int ret;

	DRM_INFO("[DSI] sunxi_drm_dsi_probe start\n");
	dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
	/* tcon_mode, slave_tcon_num, port_num is init to 0 for now not support other mode*/

	if (!dsi)
		return -ENOMEM;
	dsi->dsi_data = of_device_get_match_data(dev);
	if (!dsi->dsi_data) {
		DRM_ERROR("sunxi_drm_dsi fail to get match data\n");
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dsi->res = res;
	dsi->reg_base = (uintptr_t)devm_ioremap_resource(dev, res);
	if (!dsi->reg_base) {
		DRM_ERROR("unable to map dsi registers\n");
		return -EINVAL;
	}

	dsi->irq_no = platform_get_irq(pdev, 0);
	if (!dsi->irq_no) {
		DRM_ERROR("get irq no of dsi failed\n");
		return -EINVAL;
	}

	dsi->clk = devm_clk_get(dev, "dsi_clk");
	if (IS_ERR(dsi->clk)) {
		DRM_ERROR("fail to get dsi_clk\n");
		return -EINVAL;
	}

	dsi->combphy = devm_clk_get_optional(dev, "combphy_clk");
	if (IS_ERR(dsi->combphy)) {
		DRM_ERROR("fail to get combphy_clk\n");
	}

	dsi->clk_bus = devm_clk_get(dev, "dsi_gating_clk");
	if (IS_ERR(dsi->clk_bus)) {
		DRM_ERROR("fail to get dsi_gating_clk\n");
		return -EINVAL;
	}

	dsi->displl_ls = devm_clk_get_optional(dev, "displl_ls");
	if (IS_ERR(dsi->displl_ls)) {
		DRM_ERROR("fail to get displl_ls\n");
	}
	dsi->displl_hs = devm_clk_get_optional(dev, "displl_hs");
	if (IS_ERR(dsi->displl_hs)) {
		DRM_ERROR("fail to get displl_hs\n");
	}

	dsi->rst_bus = devm_reset_control_get_shared(dev, "dsi_rst_clk");
	if (IS_ERR(dsi->rst_bus)) {
		DRM_ERROR("fail to get reset rst_bus_mipi_dsi\n");
		return -EINVAL;
	}

	dsi->sdrm.hw_id = dsi->dsi_data->id;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res) {
		dsi->dsc_base = (uintptr_t)devm_ioremap_resource(dev, res);
		dsi->dsc_gating_clk = devm_clk_get(dev, "clk_bus_dsc");
		if (IS_ERR(dsi->dsc_gating_clk)) {
			DRM_ERROR("fail to get clk clk_bus_dsc\n");
			return -EINVAL;
		}
		dsi->dsc_rst_clk = devm_reset_control_get_shared(dev, "rst_bus_dsc");
		if (IS_ERR(dsi->dsc_rst_clk)) {
			DRM_ERROR("fail to get reset rst_bus_dsc\n");
			return -EINVAL;
		}
		dsc_set_reg_base(&dsi->dsi_lcd, dsi->dsc_base);
	}

	dsi->phy = devm_phy_get(dev, "combophy");
	if (IS_ERR_OR_NULL(dsi->phy))
		DRM_INFO("dsi%d's combophy not setting, maybe not used!\n", dsi->sdrm.hw_id);

	dsi->host.ops = &sunxi_drm_dsi_host_ops;
	dsi->host.dev = dev;
	dsi->dev = dev;
	dsi->dsi_lcd.dsi_index = dsi->sdrm.hw_id;
	dsi_set_reg_base(&dsi->dsi_lcd, dsi->reg_base);
	dev_set_drvdata(dev, dsi);
	platform_set_drvdata(pdev, dsi);

	ret = esd_watchdog_init(dsi);
	// TODO:Maybe need to release resource.
	// if (ret < 0)
	// 	esd_watchdog_exit(dsi);

	ret = mipi_dsi_host_register(&dsi->host);
	if (ret) {
		DRM_ERROR("Couldn't register MIPI-DSI host\n");
		return ret;
	}

	DRM_INFO("[DSI]%s ok\n", __FUNCTION__);

	return 0;

//	return component_add(&pdev->dev, &sunxi_drm_dsi_component_ops);
}

static int sunxi_drm_dsi_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &sunxi_drm_dsi_component_ops);
	return 0;
}

struct platform_driver sunxi_dsi_platform_driver = {
	.probe = sunxi_drm_dsi_probe,
	.remove = sunxi_drm_dsi_remove,
	.driver = {
		.name = "sunxi-dsi",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_drm_dsi_match,
	},
};

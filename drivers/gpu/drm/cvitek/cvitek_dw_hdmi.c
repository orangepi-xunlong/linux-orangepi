#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/component.h>

#include <drm/drm_of.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_dsc.h>
#include <drm/drm_edid.h>
#include <drm/bridge/dw_hdmi.h>
#include <drm/drm_edid.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_print.h>

#include <video/videomode.h>
#include "cvitek_vo_sys_reg.h"
#include "cvitek_drm.h"
#include "cvitek_disp.h"
#include "cvitek_mipipll_cfg.h"

/**
 * struct cvitek_hdmi_chip_data - splite the grf setting of kind of chips
 * @lcdsel_grf_reg: grf register offset of lcdc select
 * @ddc_en_reg: grf register offset of hdmi ddc enable
 * @lcdsel_big: reg value of selecting vop big for HDMI
 * @lcdsel_lit: reg value of selecting vop little for HDMI
 */
struct cvitek_hdmi_chip_data {
	int	lcdsel_grf_reg;
	int	ddc_en_reg;
	u32	lcdsel_big;
	u32	lcdsel_lit;
	bool	split_mode;
};

struct cvitek_hdmi {
	struct device *dev;
	struct drm_encoder encoder;
	struct drm_device *drm_dev;
	const struct cvitek_hdmi_chip_data *chip_data;
	struct dw_hdmi_plat_data *plat_data;
	struct dw_hdmi *hdmi;
	struct clk *vpll_clk;  // only for debug
	u8 id;
	struct videomode vm;
};

#define to_cvitek_hdmi(x)	container_of(x, struct cvitek_hdmi, x)

const struct dw_hdmi_plat_data cvitek_hdmi_drv_data = {
	.phy_name = "phy_model_316",
	.phy_force_vendor = false,
	.ycbcr_420_allowed = true,
	.use_drm_infoframe = true,
};

static const struct of_device_id dw_hdmi_cvitek_dt_ids[] = {
	{ .compatible = "cvitek,dw_hdmi",
	  .data = &cvitek_hdmi_drv_data
	},
	{},
};
MODULE_DEVICE_TABLE(of, dw_hdmi_cvitek_dt_ids);

static void dw_hdmi_cvitek_encoder_disable(struct drm_encoder *encoder)
{
}

static void dw_hdmi_cvitek_encoder_mode_set(struct drm_encoder *encoder,
					      struct drm_display_mode *mode,
					      struct drm_display_mode *adj_mode)
{
	struct cvitek_hdmi *hdmi = to_cvitek_hdmi(encoder);

	drm_display_mode_to_videomode(adj_mode, &hdmi->vm);
}

static void dw_hdmi_cvitek_encoder_commit(struct drm_encoder *encoder)
{
}

void mipipll_clk_set(u32 ClkKhz)
{
	_reg_write(REG_DSI_PHY_POWER_DOWN_CFG(1), 0x0);
	mipi_dphy_set_pll(1, ClkKhz, 4, 24);
}

static void dw_hdmi_cvitek_encoder_enable(struct drm_encoder *encoder)
{
	struct cvitek_hdmi *hdmi = to_cvitek_hdmi(encoder);
	struct drm_crtc *crtc = encoder->crtc;

	if (WARN_ON(!crtc || !crtc->state))
		return;

	mipipll_clk_set(hdmi->vm.pixelclock / 1000);
}

static int
dw_hdmi_cvitek_encoder_atomic_check(struct drm_encoder *encoder,
				      struct drm_crtc_state *crtc_state,
				      struct drm_connector_state *conn_state)
{
	return 0;
}

static const struct drm_encoder_helper_funcs dw_hdmi_cvitek_encoder_helper_funcs = {
	.mode_set	= dw_hdmi_cvitek_encoder_mode_set,
	.enable     = dw_hdmi_cvitek_encoder_enable,
	.disable    = dw_hdmi_cvitek_encoder_disable,
	.atomic_check = dw_hdmi_cvitek_encoder_atomic_check,
	.commit    = dw_hdmi_cvitek_encoder_commit,
};

static int dw_hdmi_cvitek_bind(struct device *dev, struct device *master,
				 void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_hdmi_plat_data *plat_data;
	struct drm_device *drm = data;
	struct drm_encoder *encoder = NULL;
	struct cvitek_hdmi *hdmi = NULL;
	int ret = 0;

	if (!pdev->dev.of_node)
		return -ENODEV;

	hdmi = platform_get_drvdata(pdev);
	if (!hdmi)
		return -ENOMEM;

	plat_data = hdmi->plat_data;
	if (!plat_data)
		return -ENOMEM;

	hdmi->drm_dev = drm;
	plat_data->phy_data = hdmi;
	encoder = &hdmi->encoder;

	encoder->possible_crtcs = drm_of_find_possible_crtcs(drm, dev->of_node);
	/*
	 * If we failed to find the CRTC(s) which this encoder is
	 * supposed to be connected to, it's because the CRTC has
	 * not been registered yet.  Defer probing, and hope that
	 * the required CRTC is added later.
	 */
	if (encoder->possible_crtcs == 0)
		return -EPROBE_DEFER;

	drm_encoder_helper_add(encoder, &dw_hdmi_cvitek_encoder_helper_funcs);
	ret = drm_simple_encoder_init(drm, encoder, DRM_MODE_ENCODER_TMDS);
	if (ret) {
		DRM_ERROR("failed to init hdmi encoder\n");
		return ret;
	}

	mipipll_clk_set(25200);
	hdmi->hdmi = dw_hdmi_bind(pdev, encoder, plat_data);
	/*
	 * If dw_hdmi_bind() fails we'll never call dw_hdmi_unbind(),
	 * which would have called the encoder cleanup.  Do it manually.
	 */
	if (IS_ERR(hdmi->hdmi)) {
		ret = PTR_ERR(hdmi->hdmi);
		drm_encoder_cleanup(encoder);
	}

	return ret;
}

static void dw_hdmi_cvitek_unbind(struct device *dev, struct device *master,
				    void *data)
{
	struct cvitek_hdmi *hdmi = dev_get_drvdata(dev);

	dw_hdmi_unbind(hdmi->hdmi);
}

static const struct component_ops dw_hdmi_cvitek_ops = {
	.bind	= dw_hdmi_cvitek_bind,
	.unbind	= dw_hdmi_cvitek_unbind,
};

static int dw_hdmi_cvitek_probe(struct platform_device *pdev)
{
	struct cvitek_hdmi *hdmi;
	const struct of_device_id *match;
	struct dw_hdmi_plat_data *plat_data;
	int id;

	hdmi = devm_kzalloc(&pdev->dev, sizeof(*hdmi), GFP_KERNEL);
	if (!hdmi)
		return -ENOMEM;

	id = of_alias_get_id(pdev->dev.of_node, "hdmi");
	if (id < 0)
		id = 0;

	hdmi->id = id;
	hdmi->dev = &pdev->dev;

	match = of_match_node(dw_hdmi_cvitek_dt_ids, pdev->dev.of_node);

	plat_data = devm_kmemdup(&pdev->dev, match->data,
				 sizeof(*plat_data), GFP_KERNEL);
	if (!plat_data)
		return -ENOMEM;

	plat_data->id = hdmi->id;
	hdmi->plat_data = plat_data;

	if (plat_data->phy_data)
		hdmi->chip_data = plat_data->phy_data;

	platform_set_drvdata(pdev, hdmi);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	return component_add(&pdev->dev, &dw_hdmi_cvitek_ops);
}

static int dw_hdmi_cvitek_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &dw_hdmi_cvitek_ops);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static void dw_hdmi_cvitek_shutdown(struct platform_device *pdev)
{
	struct cvitek_hdmi *hdmi = dev_get_drvdata(&pdev->dev);

	if (!hdmi)
		return;

	dw_hdmi_suspend(hdmi->hdmi);
	pm_runtime_put_sync(&pdev->dev);
}

static int dw_hdmi_cvitek_suspend(struct device *dev)
{
	struct cvitek_hdmi *hdmi = dev_get_drvdata(dev);

	dw_hdmi_suspend(hdmi->hdmi);
	pm_runtime_put_sync(dev);

	return 0;
}

static int dw_hdmi_cvitek_resume(struct device *dev)
{
	struct cvitek_hdmi *hdmi = dev_get_drvdata(dev);

	dw_hdmi_resume(hdmi->hdmi);
	pm_runtime_get_sync(dev);

	return 0;
}

static const struct dev_pm_ops dw_hdmi_cvitek_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(dw_hdmi_cvitek_suspend, dw_hdmi_cvitek_resume)
};

struct platform_driver dw_hdmi_cvitek_pltfm_driver = {
	.probe  = dw_hdmi_cvitek_probe,
	.remove = dw_hdmi_cvitek_remove,
	.shutdown = dw_hdmi_cvitek_shutdown,
	.driver = {
		.name = "dw_hdmi_cvitek",
		.pm = &dw_hdmi_cvitek_pm,
		.of_match_table = dw_hdmi_cvitek_dt_ids,
	},
};

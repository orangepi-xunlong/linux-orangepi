#include <linux/component.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_vblank.h>

#include "cvitek_vo_sys_reg.h"
#include "cvitek_drm.h"

#define DRIVER_NAME	"cvitek"
#define DRIVER_DESC	"Cvitek Soc DRM"
#define DRIVER_DATE	"20230531"
#define DRIVER_MAJOR	1
#define DRIVER_MINOR	0

#define MAX_CVITEK_SUB_DRIVERS 16
static struct platform_driver *cvitek_sub_drivers[MAX_CVITEK_SUB_DRIVERS];
static int num_cvitek_sub_drivers;

#define ADD_CVITEK_SUB_DRIVER(drv, cond) { \
	if (IS_ENABLED(cond) && \
	    !WARN_ON(num_cvitek_sub_drivers >= MAX_CVITEK_SUB_DRIVERS)) \
		cvitek_sub_drivers[num_cvitek_sub_drivers++] = &drv; \
}

static const struct drm_mode_config_helper_funcs cvitek_drm_mode_config_helper = {
	.atomic_commit_tail = drm_atomic_helper_commit_tail_rpm,
};

static const struct drm_mode_config_funcs cvitek_drm_mode_config_funcs = {
	.fb_create = drm_gem_fb_create,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static void cvitek_drm_mode_config_init(struct drm_device *dev)
{
	dev->mode_config.min_width = 8;
	dev->mode_config.min_height = 8;
	dev->mode_config.max_width = 6656;
	dev->mode_config.max_height = 2160;

	dev->mode_config.funcs = &cvitek_drm_mode_config_funcs;
	dev->mode_config.helper_private = &cvitek_drm_mode_config_helper;
}

static int drm_cvitek_gem_dumb_create(struct drm_file *file_priv,
				     struct drm_device *drm,
				     struct drm_mode_create_dumb *args)
{
	args->pitch = ALIGN(DIV_ROUND_UP(args->width * args->bpp, 8), 64);

	return drm_gem_cma_dumb_create_internal(file_priv, drm, args);
}

DEFINE_DRM_GEM_CMA_FOPS(cvitek_drm_fops);

static struct drm_driver cvitek_drm_drv = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
	.fops = &cvitek_drm_fops,
	.lastclose = drm_fb_helper_lastclose,
	/* GEM Operations */
	DRM_GEM_CMA_DRIVER_OPS_WITH_DUMB_CREATE(drm_cvitek_gem_dumb_create),

	.name	= DRIVER_NAME,
	.desc	= DRIVER_DESC,
	.date	= DRIVER_DATE,
	.major	= DRIVER_MAJOR,
	.minor	= DRIVER_MINOR,
};

static int cvitek_vo_sys_base_set(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm = dev_get_drvdata(dev);
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "failed to get VO sys MEM resource\n");
		return -EINVAL;
	}

	vo_sys_f_base = devm_ioremap(dev, res->start, resource_size(res));
	if (!vo_sys_f_base) {
		drm_err(drm, "failed to map VO sys registers\n");
		return -ENXIO;
	}
	DRM_DEBUG_DRIVER("res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
		res->start, res->end, vo_sys_f_base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(dev, "failed to get VO sys MEM resource\n");
		return -EINVAL;
	}

	vo_sys_b_base = devm_ioremap(dev, res->start, resource_size(res));
	if (!vo_sys_b_base) {
		drm_err(drm, "failed to map VO sys registers\n");
		return -ENXIO;
	}
	DRM_DEBUG_DRIVER("res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
		res->start, res->end, vo_sys_b_base);

	return 0;
}

static int cvitek_top_pll_set(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm = dev_get_drvdata(dev);
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!res) {
		dev_err(dev, "failed to get top pll MEM resource\n");
		return -EINVAL;
	}

	top_pll_base = devm_ioremap(dev, res->start, resource_size(res));
	if (!top_pll_base) {
		drm_err(drm, "failed to map top pll registers\n");
		return -ENXIO;
	}
	DRM_DEBUG_DRIVER("res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
		res->start, res->end, top_pll_base);

	return 0;
}

static int cvitek_drm_bind(struct device *dev)
{
	struct drm_device *drm;
	struct cvitek_drm *cvitek;
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek drm bind. ----\n");

	cvitek = devm_drm_dev_alloc(dev, &cvitek_drm_drv, struct cvitek_drm, drm);
	if (IS_ERR(cvitek))
		return PTR_ERR(cvitek);

	drm = &cvitek->drm;
	dev_set_drvdata(dev, drm);

	//Athena 2 dma need set to 64bit
	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
	if (ret) {
		drm_err(drm, "cannot set DMA masks to 64-bit.\n");
		return ret;
	}

	ret = cvitek_vo_sys_base_set(dev);
	if (ret)
		return ret;

	ret = cvitek_top_pll_set(dev);
	if (ret)
		return ret;

	/*reset disp0 and disp1*/
	reset_disp();

	ret = drmm_mode_config_init(drm);
	if (ret)
		return ret;

	cvitek_drm_mode_config_init(drm);

	/* bind and init sub drivers */
	ret = component_bind_all(drm->dev, drm);
	if (ret) {
		drm_err(drm, "failed to bind all component.\n");
		return ret;
	}

	/* vblank init */
	drm->irq_enabled = true;
	ret = drm_vblank_init(drm, drm->mode_config.num_crtc);
	if (ret) {
		drm_err(drm, "failed to initialize vblank.\n");
		goto err_unbind_all;
	}

	/* reset all the states of crtc/plane/encoder/connector */
	drm_mode_config_reset(drm);

	/* init kms poll for handling hpd */
	drm_kms_helper_poll_init(drm);

	ret = drm_dev_register(drm, 0);
	if (ret < 0)
		goto err_kms_helper_poll_fini;

	//frame buffer support
	drm_fbdev_generic_setup(drm, 24);
	drm->mode_config.allow_fb_modifiers = true;

	return 0;

err_kms_helper_poll_fini:
	drm_kms_helper_poll_fini(drm);
err_unbind_all:
	component_unbind_all(drm->dev, drm);
	return ret;
}

static void cvitek_drm_unbind(struct device *dev)
{
	struct drm_device *drm = dev_get_drvdata(dev);

	DRM_DEBUG_DRIVER("---- enter cvitek drm unbind. ----\n");

	drm_dev_unregister(drm);
	drm_kms_helper_poll_fini(drm);
	drm_atomic_helper_shutdown(drm);
	drm_mode_config_cleanup(drm);
	component_unbind_all(drm->dev, drm);
}

static const struct component_master_ops drm_component_ops = {
	.bind = cvitek_drm_bind,
	.unbind = cvitek_drm_unbind,
};

static int compare_dev(struct device *dev, void *data)
{
	return dev == (struct device *)data;
}

static struct component_match *cvitek_drm_match_add(struct device *dev)
{
	struct component_match *match = NULL;
	int i;

	DRM_DEBUG_DRIVER("---- enter cvitek drm match add. ----\n");

	for (i = 0; i < num_cvitek_sub_drivers; i++) {
		struct platform_driver *drv = cvitek_sub_drivers[i];
		struct device *p = NULL, *d;

		do {
			d = platform_find_device_by_driver(p, &drv->driver);
			put_device(p);
			p = d;
			if (!d)
				break;
			component_match_add(dev, &match, compare_dev, d);
			DRM_DEBUG_DRIVER("---- after component match add[%d]. ----\n", i);
		} while (true);
	}

	return match ?: ERR_PTR(-ENODEV);
}

static int cvitek_drm_platform_probe(struct platform_device *pdev)
{
	struct component_match *match;

	DRM_DEBUG_DRIVER("---- enter cvitek drm platform probe. ----\n");

	match = cvitek_drm_match_add(&pdev->dev);
	if (IS_ERR(match))
		return PTR_ERR(match);

	return component_master_add_with_match(&pdev->dev, &drm_component_ops,
					       match);
}

static int cvitek_drm_platform_remove(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("---- enter cvitek drm platform remove. ----\n");

	component_master_del(&pdev->dev, &drm_component_ops);

	return 0;
}

static void cvitek_drm_platform_shutdown(struct platform_device *pdev)
{
	struct drm_device *drm = platform_get_drvdata(pdev);

	if (drm)
		drm_atomic_helper_shutdown(drm);
}

#ifdef CONFIG_PM_SLEEP
static int cvitek_drm_suspend(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	return drm_mode_config_helper_suspend(drm_dev);
}

static int cvitek_drm_resume(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	return drm_mode_config_helper_resume(drm_dev);
}
#endif

static SIMPLE_DEV_PM_OPS(cvitek_drm_pm_ops, cvitek_drm_suspend, cvitek_drm_resume);

static const struct of_device_id cvitek_drm_dts_match[] = {
	{ .compatible = "cvitek,drm-subsystem", },
	{ /* end node */ },
};
MODULE_DEVICE_TABLE(of, cvitek_drm_dts_match);

static struct platform_driver cvitek_drm_platform_driver = {
	.probe = cvitek_drm_platform_probe,
	.remove = cvitek_drm_platform_remove,
	.shutdown = cvitek_drm_platform_shutdown,
	.driver = {
		.name = "cvitek-drm",
		.pm = &cvitek_drm_pm_ops,
		.of_match_table = cvitek_drm_dts_match,
	},
};

static int __init cvitek_drm_init(void)
{
	int ret;

	DRM_DEBUG_DRIVER("---- enter cvitek drm init. ----\n");

	num_cvitek_sub_drivers = 0;
	ADD_CVITEK_SUB_DRIVER(cvitek_disp_driver, CONFIG_DRM_CVITEK);
	ADD_CVITEK_SUB_DRIVER(cvitek_dsi_driver, CONFIG_CVITEK_MIPI_DSI);
	ADD_CVITEK_SUB_DRIVER(dw_hdmi_cvitek_pltfm_driver, CONFIG_CVITEK_HDMI);
	ADD_CVITEK_SUB_DRIVER(cvitek_lvds_driver, CONFIG_CVITEK_LVDS);
	ret = platform_register_drivers(cvitek_sub_drivers,
					num_cvitek_sub_drivers);
	if (ret)
		return ret;

	ret = platform_driver_register(&cvitek_drm_platform_driver);
	if (ret)
		goto err_unreg_drivers;

	return 0;

err_unreg_drivers:
	platform_unregister_drivers(cvitek_sub_drivers,
				    num_cvitek_sub_drivers);
	return ret;
}

static void __exit cvitek_drm_exit(void)
{
	DRM_DEBUG_DRIVER("---- enter cvitek drm exit. ----\n");

	platform_driver_unregister(&cvitek_drm_platform_driver);
	platform_unregister_drivers(cvitek_sub_drivers,
				    num_cvitek_sub_drivers);
}

module_init(cvitek_drm_init);
module_exit(cvitek_drm_exit);

MODULE_AUTHOR("Bowen Du <bowen.du@sophgo.com>");
MODULE_DESCRIPTION("cvitek CV186X SoCs' DRM master driver");
MODULE_LICENSE("GPL v2");

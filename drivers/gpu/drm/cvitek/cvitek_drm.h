#ifndef __CVITEK_DRM_H__
#define __CVITEK_DRM_H__

struct cvitek_drm {
	struct drm_device drm;
};

extern struct platform_driver cvitek_disp_driver;
extern struct platform_driver cvitek_dsi_driver;
extern struct platform_driver dw_hdmi_cvitek_pltfm_driver;
extern struct platform_driver cvitek_lvds_driver;

#endif /* __CVITEK_DRM_H__ */

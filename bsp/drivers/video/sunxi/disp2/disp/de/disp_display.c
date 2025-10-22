/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "disp_display.h"
#include "disp_rtwb.h"
#include "../disp_trace.h"

struct disp_dev_t gdisp;
extern struct disp_drv_info g_disp_drv;

int platform_fb_adjust_output_size(int disp, enum disp_output_type, unsigned int output_mode);

s32 bsp_disp_init(struct disp_bsp_init_para *para)
{
	u32 num_screens, disp;

	memset(&gdisp, 0x00, sizeof(struct disp_dev_t));
	memcpy(&gdisp.init_para, para, sizeof(struct disp_bsp_init_para));
	para->shadow_protect = bsp_disp_shadow_protect;
	disp_init_feat(&para->feat_init);

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		spin_lock_init(&gdisp.screen[disp].flag_lock);
		tasklet_init(&gdisp.screen[disp].tasklet, disp_tasklet,
			     (unsigned long)disp);
	}

	disp_init_al(para);
#if defined (DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	disp_al_init_tcon(para);
#endif
	disp_init_lcd(para);
#if defined(SUPPORT_HDMI)
	disp_init_hdmi(para);
#endif
#if defined(SUPPORT_TV)
	disp_init_tv_para(para);
#endif
#if defined(SUPPORT_RTWB)
	disp_init_rtwb(para);
#endif

#if defined(SUPPORT_EDP)
	disp_init_edp(para);
#endif /* endif SUPPORT_EDP */

#if defined (DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	disp_init_irq_util(para->irq_no[DISP_MOD_DE]);
#endif

#if defined(SUPPORT_VDPO)
	disp_init_vdpo(para);
#endif
	disp_init_mgr(para);
#if IS_ENABLED(CONFIG_AW_DISP2_SUPPORT_ENAHNCE)
	disp_init_enhance(para);
#endif
#if IS_ENABLED(CONFIG_AW_DISP2_SUPPORT_SMBL)
	disp_init_smbl(para);
#endif
	disp_init_capture(para);

#if IS_ENABLED(CONFIG_AW_DISP2_FB_ROTATION_SUPPORT)
	disp_init_rotation_sw(para);
#endif

#if defined(SUPPORT_WB)
	disp_init_format_convert_manager(para);
#endif
#if defined SUPPORT_EINK
	disp_init_eink(para);
#endif

	disp_init_connections(para);

	return DIS_SUCCESS;
}

s32 bsp_disp_exit(u32 mode)
{
#if defined SUPPORT_EINK
	disp_exit_eink();
#endif
#if defined(SUPPORT_WB)
	disp_exit_format_convert_manager();
#endif
	disp_exit_capture();
#if IS_ENABLED(CONFIG_AW_DISP2_SUPPORT_SMBL)
	disp_exit_smbl();
#endif
#if IS_ENABLED(CONFIG_AW_DISP2_SUPPORT_ENAHNCE)
	disp_exit_enhance();
#endif
	disp_exit_mgr();
#if defined(SUPPORT_HDMI)
	disp_exit_hdmi();
#endif

#if defined(SUPPORT_VDPO)
	disp_exit_vdpo();
#endif
	disp_exit_lcd();
	disp_exit_al();

	disp_exit_feat();

	return DIS_SUCCESS;
}

s32 bsp_disp_open(void)
{
	return DIS_SUCCESS;
}

s32 bsp_disp_close(void)
{
	return DIS_SUCCESS;
}

s32 disp_device_attached(int disp_mgr, int disp_dev,
			struct disp_device_config *config)
{
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;

	mgr = disp_get_layer_manager(disp_mgr);
	if (!mgr)
		return -1;

	/* no need to attch */
	if (mgr->device && (config->type == mgr->device->type))
		return 0;

	/* detach manager and device first */
	if (mgr->device) {
		dispdev = mgr->device;
		if (dispdev->is_enabled && dispdev->is_enabled(dispdev)
		    && dispdev->disable)
			dispdev->disable(dispdev);
		if (dispdev->unset_manager)
			dispdev->unset_manager(dispdev);
	}

	dispdev = disp_device_get(disp_dev, config->type);
	if (dispdev && dispdev->set_manager) {
		dispdev->set_manager(dispdev, mgr);
		DE_WARN("attached ok, mgr%d<-->device%d, type=%d\n",
				disp_mgr, disp_dev, (u32)config->type);
		if (dispdev->set_static_config)
			dispdev->set_static_config(dispdev, config);
		return 0;
	}

	return -1;
}

s32 disp_device_attached_and_enable(int disp_mgr, int disp_dev,
				    struct disp_device_config *config)
{
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;
	int ret = 0;

	mgr = disp_get_layer_manager(disp_mgr);
	if (!mgr)
		return -1;

	if (mgr->device && mgr->device->type != config->type) {
		if (mgr->device->is_enabled(mgr->device))
			mgr->device->disable(mgr->device);

		if (mgr->device->unset_manager)
			mgr->device->unset_manager(mgr->device);
	}

	if ((!mgr->device) && (config->type != DISP_OUTPUT_TYPE_NONE)) {
		dispdev = disp_device_get(disp_dev, config->type);
		if (dispdev && dispdev->set_manager) {
			dispdev->set_manager(dispdev, mgr);
		} else {
			ret = -1;
			goto exit;
		}
	}

	if (mgr->device) {
		disp_config_update_t update = DISP_NORMAL_UPDATE;

		if (mgr->device->check_config_dirty)
			update = mgr->device->check_config_dirty(mgr->device,
								 config);

		if (update == DISP_NORMAL_UPDATE) {
			static char const *fmt_name[] = {
				"rgb",
				"yuv444",
				"yuv422",
				"yuv420"
			};
			static char const *bits_name[] = {
				"8bits",
				"10bits",
				"12bits",
				"16bits"
			};
			if (mgr->device->is_enabled(mgr->device))
				mgr->device->disable(mgr->device);

			if (mgr->device->set_static_config)
				ret = mgr->device->set_static_config(mgr->device,
							config);
			if (ret != 0)
				goto exit;

			if (config->type == DISP_OUTPUT_TYPE_TV)
				disp_delay_ms(300);

			if (config->type == DISP_OUTPUT_TYPE_HDMI)
				disp_delay_ms(1000);

			if (config->type == DISP_OUTPUT_TYPE_LCD)
				disp_delay_ms(1000);

			platform_fb_adjust_output_size(mgr->disp, config->type, config->mode);
			ret = mgr->device->enable(mgr->device);
			DE_WARN("attached %s, mgr%d<-->dev%d\n",
				(ret == 0) ? "ok" : "fail",
				disp_mgr, disp_dev);
			DE_WARN("type:%d,mode:%d,fmt:%s,bits:%s,eotf:%d,cs:%d dvi_hdmi:%d, range:%d scan:%d ratio:%d\n",
				config->type,
				config->mode,
				(config->format < 4) ?
				    fmt_name[config->format] : "undef",
				(config->bits < 4) ?
				    bits_name[config->bits] : "undef",
				config->eotf,
				config->cs,
				config->dvi_hdmi,
				config->range,
				config->scan,
				config->aspect_ratio);
			if (ret != 0)
				goto exit;

		} else if (update == DISP_SMOOTH_UPDATE) {
			if (mgr->device->set_static_config)
				ret = mgr->device->set_static_config(mgr->device,
							config);
			mgr->device->smooth_enable(mgr->device);
			if (ret != 0)
				goto exit;
		}
	}

	return 0;

exit:
	return ret;
}

s32 disp_device_detach(int disp_mgr, int disp_dev,
		       enum disp_output_type output_type)
{
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;

	mgr = disp_get_layer_manager(disp_mgr);
	if (!mgr) {
		DE_WARN("get mgr%d fail\n", disp_mgr);
		return -1;
	}

	dispdev = disp_device_get(disp_dev, output_type);
	if (!dispdev) {
		DE_WARN("get device%d fail\n", disp_dev);
		return -1;
	}

	if (mgr->device == dispdev) {
		if (dispdev->disable)
			dispdev->disable(dispdev);
		if (dispdev->unset_manager)
			dispdev->unset_manager(dispdev);
	}

	return 0;
}

s32 bsp_disp_device_switch(int disp, enum disp_output_type output_type,
			   enum disp_output_type mode)
{
	int num_screens = 0;
	int disp_dev = disp;
	struct disp_init_para *init_para = &g_disp_drv.disp_init;
	int ret = -1;
	struct disp_device_config config;
	memset(&config, 0, sizeof(struct disp_device_config));

	config.type = output_type;
	config.mode = (enum disp_tv_mode)mode;
	config.format = (output_type == DISP_OUTPUT_TYPE_LCD) ?
			DISP_CSC_TYPE_RGB : DISP_CSC_TYPE_YUV444;
	config.bits = DISP_DATA_8BITS;
	config.eotf = DISP_EOTF_GAMMA22;

	if (config.type == DISP_OUTPUT_TYPE_HDMI) {
		if (config.mode >= DISP_TV_MOD_720P_50HZ)
			config.cs = DISP_BT709;
		else
			config.cs = DISP_BT601;

		config.eotf = DISP_EOTF_GAMMA22;
	}

	config.dvi_hdmi = DISP_HDMI;
	config.range = DISP_COLOR_RANGE_16_235;
	config.scan = DISP_SCANINFO_NO_DATA;
	config.aspect_ratio = 8;

	if (config.type == DISP_OUTPUT_TYPE_LCD
		&& init_para->to_lcd_index[disp] != DE_TO_TCON_NOT_DEFINE) {
		disp_dev = init_para->to_lcd_index[disp];
	}

	ret = disp_device_attached_and_enable(disp, disp_dev, &config);
	if (ret != 0) {
		DE_INFO("DE%d attach TCON_LCD_%d/TCON_TV_%d failed! \n", disp, disp_dev, disp_dev);
		num_screens = bsp_disp_feat_get_num_screens();
		for (disp_dev = 0; disp_dev < num_screens; disp_dev++) {
			DE_INFO("DE%d try to attach TCON_LCD_%d/TCON_TV_%d \n", disp, disp_dev, disp_dev);
			ret = disp_device_attached_and_enable(disp,
							      disp_dev,
							      &config);
			if (ret == 0) {
				DE_INFO("DE%d attach TCON_LCD_%d/TCON_TV_%d OK! \n", disp, disp_dev, disp_dev);
				break;
			}
		}
	}

	return ret;
}

s32 bsp_disp_device_set_config(int disp, struct disp_device_config *config)
{
	int num_screens = 0;
	int disp_dev = disp;
	int ret = -1;
	struct disp_init_para *init_para = &g_disp_drv.disp_init;

	if ((config->dvi_hdmi != DISP_DVI) && (config->dvi_hdmi != DISP_HDMI))
		config->dvi_hdmi = DISP_HDMI;
	if (config->range > 2)
		config->range = 0;
	if (config->scan > 2)
		config->scan = 0;
	if (!config->aspect_ratio)
		config->aspect_ratio = 8;
	if ((config->type == DISP_OUTPUT_TYPE_HDMI)
		&& (!config->cs)) {
		if (config->mode >= DISP_TV_MOD_720P_50HZ)
			config->cs = DISP_BT709;
		else
			config->cs = DISP_BT601;

		if (!config->eotf)
			config->eotf = DISP_EOTF_GAMMA22;
	}

	if (config->type == DISP_OUTPUT_TYPE_LCD
		&& init_para->to_lcd_index[disp] != DE_TO_TCON_NOT_DEFINE) {
		disp_dev = init_para->to_lcd_index[disp];
	}

	ret = disp_device_attached_and_enable(disp, disp_dev, config);
	if (ret != 0) {
		DE_INFO("DE%d attach TCON_LCD_%d/TCON_TV_%d failed! \n", disp, disp_dev, disp_dev);
		num_screens = bsp_disp_feat_get_num_screens();
		for (disp_dev = 0; disp_dev < num_screens; disp_dev++) {
			DE_INFO("DE%d try to attach TCON_LCD_%d/TCON_TV_%d \n", disp, disp_dev, disp_dev);
			ret = disp_device_attached_and_enable(disp,
							      disp_dev,
							      config);
			if (ret == 0) {
				DE_INFO("DE%d attach TCON_LCD_%d/TCON_TV_%d OK! \n", disp, disp_dev, disp_dev);
				break;
			}
		}
	}

	return ret;
}

#if IS_ENABLED(CONFIG_EINK_PANEL_UESD)
s32 bsp_disp_eink_update(struct disp_eink_manager *manager,
			struct disp_layer_config_inner *config,
			unsigned int layer_num,
			enum eink_update_mode mode,
			struct area_info *update_area)
{
	int ret = -1;
	struct area_info area;

	memcpy(&area, update_area, sizeof(struct area_info));

	if (manager)
		ret = manager->eink_update(manager, config, layer_num,
					   mode, area);
	else
		DE_DEBUG("eink manager is NULL\n");

	return ret;
}

s32 bsp_disp_eink_set_temperature(struct disp_eink_manager *manager,
				  unsigned int temp)
{
	s32 ret = -1;

	if (manager)
		ret = manager->set_temperature(manager, temp);
	else
		DE_WARN("eink manager is NULL\n");

	return ret;
}

s32 bsp_disp_eink_get_temperature(struct disp_eink_manager *manager)
{
	s32 ret = -1;

	if (manager)
		ret = manager->get_temperature(manager);
	else
		DE_WARN("eink manager is NULL\n");

	return ret;
}

s32 bsp_disp_eink_op_skip(struct disp_eink_manager *manager, unsigned int skip)
{
	s32 ret = -1;

	if (manager)
		ret = manager->op_skip(manager, skip);
	else
		DE_WARN("eink manager is NULL\n");

	return ret;
}
#endif

s32 disp_init_connections(struct disp_bsp_init_para *para)
{
	u32 disp = 0;
	u32 num_screens = 0;
	u32 num_layers = 0, layer_id = 0;
	u32 i = 0;
	struct disp_init_para *init_para = &g_disp_drv.disp_init;

	DE_INFO("disp_init_connections\n");

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		int dev_index = disp;
		struct disp_manager *mgr;
		struct disp_layer *lyr;
		struct disp_device *dispdev = NULL;
#if IS_ENABLED(CONFIG_AW_DISP2_SUPPORT_ENAHNCE)
		struct disp_enhance *enhance = NULL;
#endif
#if IS_ENABLED(CONFIG_AW_DISP2_SUPPORT_SMBL)
		struct disp_smbl *smbl = NULL;
#endif
		struct disp_capture *cptr = NULL;
#if IS_ENABLED(CONFIG_AW_DISP2_FB_ROTATION_SUPPORT)
		struct disp_rotation_sw *rot_sw = NULL;
#endif

		mgr = disp_get_layer_manager(disp);
		if (!mgr)
			continue;

		/* connect layer & it's manager */
		num_layers = bsp_disp_feat_get_num_layers(disp);
		for (layer_id = 0; layer_id < num_layers; layer_id++) {
			lyr = disp_get_layer_1(disp, layer_id);
			if (lyr != NULL)
				lyr->set_manager(lyr, mgr);
		}

		if ((para->boot_info[disp].sync == 1)
		    && (disp == para->boot_info[disp].disp)
		    && (para->boot_info[disp].type == DISP_OUTPUT_TYPE_LCD)) {
			/* connect device & it's manager */
			if (init_para->to_lcd_index[disp] != DE_TO_TCON_NOT_DEFINE) {
				dev_index = init_para->to_lcd_index[disp];
			}

			dispdev = disp_device_get(dev_index, DISP_OUTPUT_TYPE_LCD);
			if ((dispdev) && (dispdev->set_manager)) {
				dispdev->set_manager(dispdev, mgr);
			} else {
				for (i = 0; i < num_screens; i++) {
					dispdev =
					    disp_device_get(i,
							DISP_OUTPUT_TYPE_LCD);
					if ((dispdev)
					    && (dispdev->set_manager)) {
						dispdev->set_manager(dispdev,
								     mgr);
						break;
					}
				}
			}
		} else if (para->boot_info[0].sync == 0) {
			if (init_para->to_lcd_index[disp] != DE_TO_TCON_NOT_DEFINE) {
				dev_index = init_para->to_lcd_index[disp];
			}
			dispdev = disp_device_get(dev_index, DISP_OUTPUT_TYPE_LCD);
			if ((dispdev) && (dispdev->set_manager))
				dispdev->set_manager(dispdev, mgr);
			mgr->enable_iommu(mgr, true);
		}

#if IS_ENABLED(CONFIG_AW_DISP2_SUPPORT_ENAHNCE)
		enhance = disp_get_enhance(disp);
		if (enhance && (enhance->set_manager))
			enhance->set_manager(enhance, mgr);
#endif

#if IS_ENABLED(CONFIG_AW_DISP2_SUPPORT_SMBL)
		smbl = disp_get_smbl(disp);
		if (smbl && (smbl->set_manager))
			smbl->set_manager(smbl, mgr);
#endif

		cptr = disp_get_capture(disp);
		if (cptr && (cptr->set_manager))
			cptr->set_manager(cptr, mgr);

#if IS_ENABLED(CONFIG_AW_DISP2_FB_ROTATION_SUPPORT)
		rot_sw = disp_get_rotation_sw(disp);
		if (rot_sw && (rot_sw->set_manager))
			rot_sw->set_manager(rot_sw, mgr);
#endif
	}

	return 0;
}

/* s32 bsp_disp_check_device_enabled(struct disp_bsp_init_para *para)
{
	int ret = 0;
	int enabled = 0;

	if ((para->boot_info.sync == 1)
	    && (para->boot_info.type != DISP_OUTPUT_TYPE_NONE)) {
		int num_screens = 0;
		int disp = para->boot_info.disp;
		int disp_dev = disp;
		enum disp_output_type type =
		    (enum disp_output_type)para->boot_info.type;
		enum disp_tv_mode mode =
			(enum disp_tv_mode)para->boot_info.mode;
		enum disp_csc_type format =
			(enum disp_csc_type)para->boot_info.format;
		enum disp_data_bits bits =
			(enum disp_data_bits)para->boot_info.bits;
		enum disp_color_space cs =
			(enum disp_color_space)para->boot_info.cs;
		enum disp_eotf eotf = (enum disp_eotf)para->boot_info.eotf;
		struct disp_manager *mgr = NULL;

		mgr = disp_get_layer_manager(disp);
		if (!mgr) {
			enabled = 0;
			goto exit;
		}
*/

		/* attach manager and display device */
/*		ret = disp_device_attached(disp, disp_dev, type, mode);
		if (ret != 0) {
			num_screens = bsp_disp_feat_get_num_screens();
			for (disp_dev = 0; disp_dev < num_screens; disp_dev++) {
				ret =
				    disp_device_attached(disp, disp_dev, type,
							 mode);
				if (ret == 0)
					break;
			}
		}
*/

		/* enable display device(only software) */
		/* if (ret != 0) { */
			/* attach fail */
/*			DE_WARN("Can't find device(%d) for manager %d\n",
			     (int)type, disp);
			goto exit;
		}

		if (mgr->device->check_if_enabled &&
		    mgr->device->check_if_enabled(mgr->device)) {
			enabled = 1;
		}
	}

exit:
	return enabled;
}
*/

s32 bsp_disp_sync_with_hw(struct disp_bsp_init_para *para, int screen_id)
{
	if ((para->boot_info[screen_id].sync == 1)
	    && (para->boot_info[screen_id].type != DISP_OUTPUT_TYPE_NONE)) {
		int num_screens = 0;
		int disp = para->boot_info[screen_id].disp;
		int disp_dev = disp;
		enum disp_output_type type =
			(enum disp_output_type)para->boot_info[screen_id].type;
		enum disp_tv_mode mode =
			(enum disp_tv_mode)para->boot_info[screen_id].mode;
		enum disp_csc_type format =
			(enum disp_csc_type)para->boot_info[screen_id].format;
		enum disp_data_bits bits =
			(enum disp_data_bits)para->boot_info[screen_id].bits;
		enum disp_color_space cs =
			(enum disp_color_space)para->boot_info[screen_id].cs;
		enum disp_eotf eotf = (enum disp_eotf)para->boot_info[screen_id].eotf;

		int ret = -1;
		struct disp_manager *mgr = NULL;

		struct disp_device_config config;
		memset(&config, 0, sizeof(struct disp_device_config));

		config.type = type;
		config.mode = mode;
		config.format = format;
		config.bits = bits;
		config.eotf = eotf;
		config.cs = cs;
		config.dvi_hdmi = para->boot_info[screen_id].dvi_hdmi;
		config.range = para->boot_info[screen_id].range;
		config.scan = para->boot_info[screen_id].scan;
		config.aspect_ratio = para->boot_info[screen_id].aspect_ratio;

		if ((config.dvi_hdmi != DISP_DVI) && (config.dvi_hdmi != DISP_HDMI))
			config.dvi_hdmi = DISP_HDMI;
		if (config.range > 2)
			config.range = 0;
		if (config.scan > 2)
			config.scan = 0;
		if (!config.aspect_ratio)
			config.aspect_ratio = 8;

		mgr = disp_get_layer_manager(disp);
		if (!mgr)
			return -1;

		/* attach manager and display device */
		ret = disp_device_attached(disp, disp_dev, &config);
		if (ret != 0) {
			num_screens = bsp_disp_feat_get_num_screens();
			for (disp_dev = 0; disp_dev < num_screens; disp_dev++) {
				ret =
				    disp_device_attached(disp, disp_dev, &config);
				if (ret == 0)
					break;
			}
		}

		/* enable display device(only software) */
		if (ret != 0) {
			/* attach fail */
			DE_WARN("Can't find device(%d) for manager %d\n",
			     (int)type, disp);
			return -1;
		}
		if (mgr->device && mgr->device->sw_enable) {
			if (mgr->device->set_mode)
				mgr->device->set_mode(mgr->device, mode);
			else if (mgr->device->set_static_config)
				mgr->device->set_static_config(mgr->device,
							&config);
			return mgr->device->sw_enable(mgr->device);
		}
	}

	return -1;
}

/***********************************************************
 *
 * interrupt proc
 *
 ***********************************************************/
static s32 bsp_disp_cfg_get(u32 disp)
{
	return gdisp.screen[disp].cfg_cnt;
}

s32 bsp_disp_shadow_protect(u32 disp, bool protect)
{
	s32 ret = -1;
	u32 cnt = 0;
	u32 max_cnt = 50;
	u32 delay = 10;		/* us */
	unsigned long flags;

	if (protect) {
		while ((ret != 0) && (cnt < max_cnt)) {
			spin_lock_irqsave(&gdisp.screen[disp].flag_lock, flags);
			cnt++;
			if (gdisp.screen[disp].have_cfg_reg == false) {
				gdisp.screen[disp].cfg_cnt++;
				ret = 0;
			}
			spin_unlock_irqrestore(&gdisp.screen[disp].flag_lock,
					       flags);
			if (ret != 0)
				disp_delay_us(delay);
		}

		if (ret != 0) {
			DE_INFO("wait for reg load finish time out\n");
			spin_lock_irqsave(&gdisp.screen[disp].flag_lock, flags);
			gdisp.screen[disp].cfg_cnt++;
			spin_unlock_irqrestore(&gdisp.screen[disp].flag_lock,
					       flags);
		}
	} else {
		spin_lock_irqsave(&gdisp.screen[disp].flag_lock, flags);
		gdisp.screen[disp].cfg_cnt--;
		spin_unlock_irqrestore(&gdisp.screen[disp].flag_lock, flags);
	}
	DE_INFO("sel=%d, protect:%d,  cnt=%d\n", disp, protect,
	      gdisp.screen[disp].cfg_cnt);
	return DIS_SUCCESS;
}
EXPORT_SYMBOL(bsp_disp_shadow_protect);

s32 bsp_disp_vsync_event_enable(u32 disp, bool enable)
{
	gdisp.screen[disp].vsync_event_en = enable;

	return DIS_SUCCESS;
}

bool bsp_disp_vsync_event_is_enabled(u32 disp)
{
	return gdisp.screen[disp].vsync_event_en;
}

static s32 disp_sync_all(u32 disp, bool sync)
{
	struct disp_manager *mgr;
	struct disp_device *dispdev;

	DISP_TRACE_INT2("frame-skip", disp, sync ? 0 : 1);

	mgr = disp_get_layer_manager(disp);
	if (!mgr) {
		DE_WARN("get mgr%d fail\n", disp);
	} else {
		dispdev = mgr->device;
		if (mgr->sync)
			mgr->sync(mgr, sync);
		if (dispdev && dispdev->get_status) {
			if (dispdev->get_status(dispdev) != 0)
				gdisp.screen[disp].health_info.error_cnt++;
		}
		if (sync == true)
			mgr->enable_iommu(mgr, true);
	}

	return 0;
}

#if defined(__LINUX_PLAT__)
/* return 10fps */
s32 bsp_disp_get_fps(u32 disp)
{
	u32 pre_time_index, cur_time_index;
	u32 pre_time, cur_time;
	u32 fps = 0xff;

	pre_time_index = gdisp.screen[disp].health_info.sync_time_index;
	cur_time_index =
	    (pre_time_index == 0) ?
	    (DEBUG_TIME_SIZE - 1) : (pre_time_index - 1);

	pre_time = gdisp.screen[disp].health_info.sync_time[pre_time_index];
	cur_time = gdisp.screen[disp].health_info.sync_time[cur_time_index];


	 /* HZ:timer interrupt times in 1 sec */
	 /* jiffies:increase 1 when timer interrupt occur. */
	 /* jiffies/HZ:current kernel boot time(second). */
	 /* 1000MS / ((cur_time_jiffies/HZ - pre_time_jiffes/HZ)*1000) */

	if (pre_time != cur_time)
		fps = MSEC_PER_SEC * HZ / (cur_time - pre_time);

	return fps;
}

s32 bsp_disp_get_cur_line(u32 screen_id)
{
	struct disp_manager *mgr = disp_get_layer_manager(screen_id);
	struct disp_device *device;

	if (!mgr || !mgr->device) {
		DE_WARN("Null hdl\n");
		return 0;
	}

	device = mgr->device;
	if (device->type == DISP_OUTPUT_TYPE_LCD) {
		struct disp_panel_para info;

		memset(&info, 0, sizeof(struct disp_panel_para));
		device->get_panel_info(device, &info);
		return disp_al_lcd_get_cur_line(device->hwdev_index, &info);
	}

	return disp_al_device_get_cur_line(device->hwdev_index);
}

static void disp_sync_checkin(u32 disp)
{
	u32 index = gdisp.screen[disp].health_info.sync_time_index;

	gdisp.screen[disp].health_info.sync_time[index] = jiffies;
	index++;
	index = (index >= DEBUG_TIME_SIZE) ? 0 : index;
	gdisp.screen[disp].health_info.sync_time_index = index;
}

s32 bsp_disp_get_health_info(u32 disp, struct disp_health_info *info)
{
	if (info)
		memcpy(info, &gdisp.screen[disp].health_info,
		       sizeof(struct disp_health_info));
	return 0;
}
#endif

void sync_event_proc(u32 disp, bool timeout)
{
	int ret;
	unsigned long flags;

	gdisp.screen[disp].health_info.irq_cnt++;

	DISP_TRACE_INT2("vsync-irq", disp,
			gdisp.screen[disp].health_info.irq_cnt & 0x01);

	DISP_TRACE_INT2("vsync-timeout", disp, timeout ? 1 : 0);
	DISP_TRACE_INT2("vsync-en", disp,
			gdisp.screen[disp].vsync_event_en);

	if (!disp_feat_is_using_rcq(disp)) {
		if (!timeout)
			disp_sync_checkin(disp);
		else
			gdisp.screen[disp].health_info.skip_cnt++;


		spin_lock_irqsave(&gdisp.screen[disp].flag_lock, flags);
		if ((bsp_disp_cfg_get(disp) == 0) && (!timeout)) {
			gdisp.screen[disp].have_cfg_reg = true;
			spin_unlock_irqrestore(&gdisp.screen[disp].flag_lock, flags);

			disp_sync_all(disp, true);
			gdisp.screen[disp].have_cfg_reg = false;
			if (gdisp.init_para.disp_int_process)
				gdisp.init_para.disp_int_process(disp);

		} else {
			spin_unlock_irqrestore(&gdisp.screen[disp].flag_lock, flags);
			disp_sync_all(disp, false);
		}

		if (gdisp.screen[disp].vsync_event_en && gdisp.init_para.vsync_event) {
			ret = gdisp.init_para.vsync_event(disp);
			if (ret == 0)
				gdisp.screen[disp].health_info.vsync_cnt++;
			else
				gdisp.screen[disp].health_info.vsync_skip_cnt++;

		}
		tasklet_schedule(&gdisp.screen[disp].tasklet);
	} else {
		struct disp_manager *mgr;

		mgr = disp_get_layer_manager(disp);
		if (!timeout)
			disp_sync_checkin(disp);

		disp_sync_all(disp, false);
		if (gdisp.init_para.disp_int_process)
			gdisp.init_para.disp_int_process(disp);

		if (gdisp.screen[disp].vsync_event_en && gdisp.init_para.vsync_event) {
			ret = gdisp.init_para.vsync_event(disp);
			if (ret == 0)
				gdisp.screen[disp].health_info.vsync_cnt++;
			else
				gdisp.screen[disp].health_info.vsync_skip_cnt++;

		}
		tasklet_schedule(&gdisp.screen[disp].tasklet);

	}
	DISP_TRACE_INT2("vsync-cnt", disp,
			gdisp.screen[disp].health_info.vsync_cnt & 0x01);
	DISP_TRACE_INT2("vsync-skip-cnt", disp,
			gdisp.screen[disp].health_info.vsync_skip_cnt & 0x01);
}

s32 bsp_disp_get_output_type(u32 disp)
{
	struct disp_manager *mgr = disp_get_layer_manager(disp);

	if (mgr) {
		struct disp_device *dispdev = mgr->device;

		if (dispdev && dispdev->is_enabled
		    && dispdev->is_enabled(dispdev))
			return dispdev->type;
	}

	return DISP_OUTPUT_TYPE_NONE;
}

void disp_tasklet(unsigned long data)
{
	struct disp_manager *mgr;
	u32 disp = (u32)data;

	mgr = disp_get_layer_manager(disp);
	if (!mgr) {
		DE_WARN("get mgr%d fail\n", disp);
	} else {
		if (mgr->tasklet)
			mgr->tasklet(mgr);
	}
}

#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC)
s32 bsp_disp_get_vb_time(void)
{
	u32 num_screens, screen_id;
	struct disp_video_timings tt;
	u32 vb_time = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		if (bsp_disp_get_output_type(screen_id)
		    == DISP_OUTPUT_TYPE_LCD) {
			struct disp_device *lcd;
			u32 time_per_line = 0;
			u32 start_delay = 0;

			lcd = disp_get_lcd(screen_id);
			if (!lcd)
				DE_WARN("get lcd%d fail\n", screen_id);

			if (lcd && lcd->get_timings) {
				u32 fps = 0;

				lcd->get_timings(lcd, &tt);
				if ((tt.ver_total_time != 0)
				    && (tt.hor_total_time != 0))
					fps =
					    tt.pixel_clk * 1000 /
					    (tt.ver_total_time *
					     tt.hor_total_time);
				start_delay = tt.ver_total_time - tt.y_res - 10;
				fps = (fps == 0) ? 60 : fps;
				time_per_line =
				    1000000 / fps / tt.ver_total_time;
				vb_time = (start_delay) * time_per_line;
			}
		}
		/* add hdmi support ? */
	}
	return vb_time;
}

/* returns: us */
s32 bsp_disp_get_next_vb_time(void)
{
	u32 cur_line;
	u32 num_screens, screen_id;
	struct disp_video_timings tt;
	u32 next_time = 16000;

	num_screens = bsp_disp_feat_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		if (bsp_disp_get_output_type(screen_id)
		    == DISP_OUTPUT_TYPE_LCD) {
			struct disp_device *lcd;
			u32 time_per_line = 0;
			struct disp_panel_para info;

			memset(&info, 0, sizeof(struct disp_panel_para));
			lcd = disp_get_lcd(screen_id);
			if (lcd && lcd->get_panel_info)
				lcd->get_panel_info(lcd, &info);
			cur_line = disp_al_lcd_get_cur_line(screen_id, &info);
			if (!lcd)
				DE_WARN("get lcd%d fail\n", screen_id);

			if (info.lcd_if != LCD_IF_EDP) {
				if (lcd && lcd->get_timings) {
					u32 fps = 0;

					lcd->get_timings(lcd, &tt);
					if ((tt.ver_total_time != 0)
					    && (tt.hor_total_time != 0))
						fps =
						    tt.pixel_clk * 1000 /
						    (tt.ver_total_time *
						     tt.hor_total_time);
					fps = (fps == 0) ? 60 : fps;
					time_per_line =
					    1000000 / fps / tt.ver_total_time;
					next_time =
					    (tt.ver_total_time -
					     cur_line) * time_per_line;
				}
			}
		}
		/* add hdmi support ? */
	}
	return next_time;
}

s32 bsp_disp_is_in_vb(void)
{
	u32 num_screens, screen_id;
	s32 ret = 1;

	num_screens = bsp_disp_feat_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		if (bsp_disp_get_output_type(screen_id)
		    == DISP_OUTPUT_TYPE_LCD) {
			struct disp_device *lcd;
			struct disp_panel_para info;

			lcd = disp_get_lcd(screen_id);
			if (!lcd)
				DE_WARN("get lcd%d fail\n", screen_id);

			memset(&info, 0, sizeof(struct disp_panel_para));
			if (lcd && lcd->get_panel_info)
				lcd->get_panel_info(lcd, &info);
			ret =
			    disp_al_lcd_query_irq(screen_id, LCD_IRQ_TCON0_VBLK,
						  &info);
		} else if (bsp_disp_get_output_type(screen_id) ==
			   DISP_OUTPUT_TYPE_HDMI) {
			/* FIXME: add hdmi */
		}
	}
	return ret;
}
#endif


s32 bsp_disp_get_screen_physical_width(u32 disp)
{
	s32 width = 0, height = 0;
	struct disp_manager *mgr = NULL;

	mgr = disp_get_layer_manager(disp);
	if (mgr && mgr->device && mgr->device->get_dimensions)
		mgr->device->get_dimensions(mgr->device, &width, &height);

	return width;
}

s32 bsp_disp_get_screen_physical_height(u32 disp)
{
	s32 width = 0, height = 0;
	struct disp_manager *mgr = NULL;

	mgr = disp_get_layer_manager(disp);
	if (mgr && mgr->device && mgr->device->get_dimensions)
		mgr->device->get_dimensions(mgr->device, &width, &height);

	return height;
}

s32 bsp_disp_get_screen_width(u32 disp)
{
	s32 width = 0;
	/* FIXME */
	return width;
}

s32 bsp_disp_get_screen_height(u32 disp)
{
	s32 height = 0;
	/* FIXME */

	return height;
}

s32 bsp_disp_get_screen_width_from_output_type(u32 disp, u32 output_type,
					       u32 output_mode)
{
	u32 width = 800, height = 480;
	struct disp_device *dispdev;

	if (output_type == DISP_OUTPUT_TYPE_LCD) {
		struct disp_manager *mgr;

		mgr = disp_get_layer_manager(disp);
		if (mgr && mgr->device && mgr->device->get_resolution) {
			mgr->device->get_resolution(mgr->device, &width,
						    &height);
		}
	} else if (output_type == DISP_OUTPUT_TYPE_EDP) {
		dispdev = disp_device_get(disp, DISP_OUTPUT_TYPE_EDP);
		if (dispdev)
			dispdev->get_resolution(dispdev, &width, &height);
	} else if ((output_type == DISP_OUTPUT_TYPE_HDMI)
		   || (output_type == DISP_OUTPUT_TYPE_TV)
		   || (output_type == DISP_OUTPUT_TYPE_VGA)
		   || (output_type == DISP_OUTPUT_TYPE_RTWB)
		   || (output_type == DISP_OUTPUT_TYPE_VDPO)) {
		switch (output_mode) {
		case DISP_TV_MOD_NTSC:
		case DISP_TV_MOD_480I:
		case DISP_TV_MOD_480P:
			width = 720;
			height = 480;
			break;
		case DISP_TV_MOD_PAL:
		case DISP_TV_MOD_576I:
		case DISP_TV_MOD_576P:
			width = 720;
			height = 576;
			break;
		case DISP_TV_MOD_720P_50HZ:
		case DISP_TV_MOD_720P_60HZ:
			width = 1280;
			height = 720;
			break;
		case DISP_TV_MOD_1080P_50HZ:
		case DISP_TV_MOD_1080P_60HZ:
		case DISP_TV_MOD_1080P_30HZ:
		case DISP_TV_MOD_1080P_25HZ:
		case DISP_TV_MOD_1080P_24HZ:
		case DISP_TV_MOD_1080I_50HZ:
		case DISP_TV_MOD_1080I_60HZ:
			width = 1920;
			height = 1080;
			break;
		case DISP_TV_MOD_1080_1920P_60HZ:
			width = 1080;
			height = 1920;
			break;
		case DISP_TV_MOD_3840_2160P_60HZ:
		case DISP_TV_MOD_3840_2160P_30HZ:
		case DISP_TV_MOD_3840_2160P_25HZ:
		case DISP_TV_MOD_3840_2160P_24HZ:
			width = 3840;
			height = 2160;
			break;
		case DISP_TV_MOD_4096_2160P_24HZ:
		case DISP_TV_MOD_4096_2160P_25HZ:
		case DISP_TV_MOD_4096_2160P_30HZ:
		case DISP_TV_MOD_4096_2160P_50HZ:
		case DISP_TV_MOD_4096_2160P_60HZ:
			width = 4096;
			height = 2160;
			break;
		case DISP_VGA_MOD_800_600P_60:
			width = 800;
			height = 600;
			break;
		case DISP_VGA_MOD_1024_768P_60:
			width = 1024;
			height = 768;
			break;
		case DISP_VGA_MOD_1280_768P_60:
			width = 1280;
			height = 768;
			break;
		case DISP_VGA_MOD_1280_800P_60:
			width = 1280;
			height = 800;
			break;
		case DISP_VGA_MOD_1366_768P_60:
			width = 1366;
			height = 768;
			break;
		case DISP_VGA_MOD_1440_900P_60:
			width = 1440;
			height = 900;
			break;
		case DISP_TV_MOD_1440_2560P_70HZ:
			width = 1440;
			height = 2560;
			break;
		case DISP_VGA_MOD_1920_1080P_60:
			width = 1920;
			height = 1080;
			break;
		case DISP_VGA_MOD_1920_1200P_60:
			width = 1920;
			height = 1200;
			break;
		case DISP_VGA_MOD_1280_720P_60:
			width = 1280;
			height = 720;
			break;
		case DISP_VGA_MOD_1600_900P_60:
			width = 1600;
			height = 900;
			break;
		case DISP_TV_MOD_2560_1440P_60HZ:
			width = 2560;
			height = 1440;
			break;
		case DISP_TV_MOD_3840_1080P_30:
			width = 3840;
			height = 1080;
			break;
		}
	}
	/* FIXME: add other output device res */

	return width;
}

s32 bsp_disp_get_screen_height_from_output_type(u32 disp, u32 output_type,
						u32 output_mode)
{
	u32 width = 800, height = 480;
	struct disp_device *dispdev;

	if (output_type == DISP_OUTPUT_TYPE_LCD) {
		struct disp_manager *mgr;

		mgr = disp_get_layer_manager(disp);
		if (mgr && mgr->device && mgr->device->get_resolution) {
			mgr->device->get_resolution(mgr->device, &width,
						    &height);
		}
	} else if (output_type == DISP_OUTPUT_TYPE_EDP) {
		dispdev = disp_device_get(disp, DISP_OUTPUT_TYPE_EDP);
		if (dispdev)
			dispdev->get_resolution(dispdev, &width, &height);
	} else if ((output_type == DISP_OUTPUT_TYPE_HDMI)
		   || (output_type == DISP_OUTPUT_TYPE_TV)
		   || (output_type == DISP_OUTPUT_TYPE_VGA)
		   || (output_type == DISP_OUTPUT_TYPE_RTWB)
		   || (output_type == DISP_OUTPUT_TYPE_VDPO)) {
		switch (output_mode) {
		case DISP_TV_MOD_NTSC:
		case DISP_TV_MOD_480I:
		case DISP_TV_MOD_480P:
			width = 720;
			height = 480;
			break;
		case DISP_TV_MOD_PAL:
		case DISP_TV_MOD_576I:
		case DISP_TV_MOD_576P:
			width = 720;
			height = 576;
			break;
		case DISP_TV_MOD_720P_50HZ:
		case DISP_TV_MOD_720P_60HZ:
			width = 1280;
			height = 720;
			break;
		case DISP_TV_MOD_1080P_50HZ:
		case DISP_TV_MOD_1080P_60HZ:
		case DISP_TV_MOD_1080P_30HZ:
		case DISP_TV_MOD_1080P_25HZ:
		case DISP_TV_MOD_1080P_24HZ:
		case DISP_TV_MOD_1080I_50HZ:
		case DISP_TV_MOD_1080I_60HZ:
			width = 1920;
			height = 1080;
			break;
		case DISP_TV_MOD_1080_1920P_60HZ:
			width = 1080;
			height = 1920;
			break;
		case DISP_TV_MOD_3840_2160P_60HZ:
		case DISP_TV_MOD_3840_2160P_30HZ:
		case DISP_TV_MOD_3840_2160P_25HZ:
		case DISP_TV_MOD_3840_2160P_24HZ:
			width = 3840;
			height = 2160;
			break;
		case DISP_TV_MOD_4096_2160P_24HZ:
		case DISP_TV_MOD_4096_2160P_25HZ:
		case DISP_TV_MOD_4096_2160P_30HZ:
		case DISP_TV_MOD_4096_2160P_50HZ:
		case DISP_TV_MOD_4096_2160P_60HZ:
			width = 4096;
			height = 2160;
			break;
		case DISP_VGA_MOD_800_600P_60:
			width = 800;
			height = 600;
			break;
		case DISP_VGA_MOD_1024_768P_60:
			width = 1024;
			height = 768;
			break;
		case DISP_VGA_MOD_1280_768P_60:
			width = 1280;
			height = 768;
			break;
		case DISP_VGA_MOD_1280_800P_60:
			width = 1280;
			height = 800;
			break;
		case DISP_VGA_MOD_1366_768P_60:
			width = 1366;
			height = 768;
			break;
		case DISP_VGA_MOD_1440_900P_60:
			width = 1440;
			height = 900;
			break;
		case DISP_TV_MOD_1440_2560P_70HZ:
			width = 1440;
			height = 2560;
			break;
		case DISP_VGA_MOD_1920_1080P_60:
			width = 1920;
			height = 1080;
			break;
		case DISP_VGA_MOD_1920_1200P_60:
			width = 1920;
			height = 1200;
			break;
		case DISP_VGA_MOD_1280_720P_60:
			width = 1280;
			height = 720;
			break;
		case DISP_VGA_MOD_1600_900P_60:
			width = 1600;
			height = 900;
			break;
		case DISP_TV_MOD_2560_1440P_60HZ:
			width = 2560;
			height = 1440;
			break;
		case DISP_TV_MOD_3840_1080P_30:
			width = 3840;
			height = 1080;
			break;
		}
	}
	/* FIXME: add other output device res */

	return height;
}

s32 bsp_disp_get_hw_info(struct disp_hw_info __user *hw_info)
{
	unsigned int num_screens, num_channels, size;
	struct disp_hw_info __user *first = hw_info;
	int i, j;
	struct disp_hw_info temp_hw_info;
	struct disp_de_hw_info temp_disp;
	struct disp_channel_hw_info temp_chn;
	struct disp_de_hw_info __user *p1;
	struct disp_channel_hw_info __user *p2;

	num_screens = bsp_disp_feat_get_num_screens();
	num_channels = 0;
	for (i = 0; i < num_screens; i++)
		num_channels += bsp_disp_feat_get_num_channels(i);
	size = sizeof(struct disp_hw_info) +
	       sizeof(struct disp_de_hw_info) * num_screens +
	       sizeof(struct disp_channel_hw_info) * num_channels;

	if (!hw_info)
		return size;

	if (!access_ok(hw_info, size)) {
		DE_WARN("user alloc memory small");
		return -1;
	}

	temp_hw_info.disp_num = num_screens;
	temp_hw_info.disp = (struct disp_de_hw_info __user *)(first + 1);
	if (copy_to_user((void __user *)first, &temp_hw_info, sizeof(temp_hw_info))) {
		DE_WARN("copy_from_user fail\n");
		return -EFAULT;
	}

	p1 = (struct disp_de_hw_info *)(first + 1);
	p2 = (struct disp_channel_hw_info *)((struct disp_de_hw_info *)(first + 1) + num_screens);
	for (i = 0; i < num_screens; i++, p1++) {
		temp_disp.disp_out.deband = bsp_disp_feat_is_support_deband(i);
		temp_disp.disp_out.smbl = bsp_disp_feat_is_support_smbl(i);
		temp_disp.disp_out.gamma = bsp_disp_feat_is_support_gamma(i);
		temp_disp.disp_out.fmt = bsp_disp_feat_is_support_fmt(i);
		temp_disp.disp_out.ksc = bsp_disp_feat_is_support_ksc(i);
		temp_disp.disp_out.dither = bsp_disp_feat_is_support_dither(i);
		temp_disp.disp_out.wb = bsp_disp_feat_is_support_capture(i);
		temp_disp.chn_num = bsp_disp_feat_get_num_channels(i);
		temp_disp.chn = (struct disp_channel_hw_info *)p2;

		if (copy_to_user((void __user *)p1, &temp_disp, sizeof(temp_disp))) {
			DE_WARN("copy_from_user fail\n");
			return -EFAULT;
		}
		for (j = 0; j < temp_disp.chn_num; j++, p2++) {
			temp_chn.type = bsp_disp_feat_get_chn_type(i, j);
			temp_chn.vep = bsp_disp_feat_chn_is_support_vep(i, j);
			temp_chn.fbd = bsp_disp_feat_lyr_is_support_fbd(i, j, 0);
			temp_chn.atw = bsp_disp_feat_lyr_is_support_atw(i, j, 0);
			temp_chn.scale = bsp_disp_feat_chn_is_support_scale(i, j);
			temp_chn.edscale = bsp_disp_feat_chn_is_support_edscale(i, j);
			temp_chn.noise = bsp_disp_feat_chn_is_support_noise(i, j);
			temp_chn.cdc = bsp_disp_feat_chn_is_support_cdc(i, j);
			temp_chn.snr = bsp_disp_feat_chn_is_support_snr(i, j);
			temp_chn.asuscale = bsp_disp_feat_chn_is_support_asuscale(i, j);
			temp_chn.fcm = bsp_disp_feat_chn_is_support_fcm(i, j);
			temp_chn.dci = bsp_disp_feat_chn_is_support_dci(i, j);
			temp_chn.sharp = bsp_disp_feat_chn_is_support_sharp(i, j);
			if (copy_to_user((void __user *)p2, &temp_chn, sizeof(temp_chn))) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}
		}
	}
	return 0;
}

s32 bsp_disp_get_boot_config_list(struct disp_boot_config_list __user *cfg_list)
{
	int i;
	unsigned int device_num, size;
	struct disp_init_para *init = &g_disp_drv.disp_init;
	struct disp_boot_config_list *config_list = NULL;

	if (!g_disp_drv.inited) {
		DE_WARN("disp drv not inited\n");
		return -1;
	}

	device_num = init->disp_device_num;
	if (!cfg_list)
		return device_num;

	size = sizeof(struct disp_boot_config_list) +
	       sizeof(struct disp_boot_config) * device_num;
	if (!access_ok(cfg_list, size)) {
		DE_WARN("user alloc memory small");
		return -1;
	}

	config_list = kmalloc(size, GFP_KERNEL | __GFP_ZERO);
	if (!config_list) {
		DE_WARN("malloc memory err");
		return -1;
	}
	config_list->num = device_num;
	for (i = 0; i < device_num; i++) {
		config_list->cfgs[i].logic_id = init->logic_id[i];
		config_list->cfgs[i].de_id = init->de_id[i];
		memcpy(config_list->cfgs[i].type, init->type[i], sizeof(init->type[i]));
		config_list->cfgs[i].fb.width = init->framebuffer_width[i];
		config_list->cfgs[i].fb.height = init->framebuffer_height[i];
		config_list->cfgs[i].dpix = init->dpix[i];
		config_list->cfgs[i].dpiy = init->dpiy[i];
	}

	if (copy_to_user(cfg_list, config_list, size)) {
		DE_WARN("copy_from_user fail\n");
		kfree(config_list);
		return -EFAULT;
	}
	kfree(config_list);
	return 0;
}

s32 bsp_disp_set_hdmi_func(struct disp_device_func *func)
{
	u32 disp = 0;
	u32 num_screens = 0;
	s32 ret = 0, registered_cnt = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *hdmi;

		hdmi = disp_device_find(disp, DISP_OUTPUT_TYPE_HDMI);
		if (hdmi) {
			if (hdmi->set_func) {
				ret = hdmi->set_func(hdmi, func);
				if (ret == 0)
					registered_cnt++;
			} else {
				DE_WARN("disp hdmi device is NOT registered\n");
				return -1;
			}
		}
	}
	if (registered_cnt != 0) {
		DE_INFO("registered!\n");
		gdisp.hdmi_registered = 1;
		if (gdisp.init_para.start_process)
			gdisp.init_para.start_process();

		return 0;
	} else {
		DE_WARN("NO hdmi funcs to registered!!\n");
	}

	return -1;
}

s32 bsp_disp_set_vdpo_func(struct disp_tv_func *func)
{
	u32 disp = 0;
	u32 num_screens = 0;
	s32 ret = 0, registered_cnt = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *vdpo;

		vdpo = disp_device_find(disp, DISP_OUTPUT_TYPE_VDPO);
		if (vdpo) {
			if (vdpo->set_tv_func)
				ret = vdpo->set_tv_func(vdpo, func);
			if (ret == 0)
				registered_cnt++;
		}
	}

	if (registered_cnt != 0) {
		DE_INFO("registered!\n");
		gdisp.vdpo_registered = 1;
		if (gdisp.init_para.start_process)
			gdisp.init_para.start_process();

		return 0;
	}

	return -1;
}

s32 bsp_disp_set_edp_func(struct disp_tv_func *func)
{
	u32 disp = 0;
	u32 num_screens = 0;
	s32 ret = 0, registered_cnt = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *edp;

		edp = disp_device_find(disp, DISP_OUTPUT_TYPE_EDP);
		if (edp) {
			if (edp->set_tv_func)
				ret = edp->set_tv_func(edp, func);
			if (ret == 0)
				registered_cnt++;
		}
	}

	if (registered_cnt != 0) {
		DE_INFO("edp registered!\n");
		gdisp.edp_registered = 1;
		if (gdisp.init_para.start_process)
			gdisp.init_para.start_process();

		return 0;
	}

	return -1;
}

s32 bsp_disp_deliver_edp_dev(struct device *dev)
{
	u32 disp = 0;
	u32 num_screens = 0;
	s32 ret = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *edp;

		edp = disp_device_find(disp, DISP_OUTPUT_TYPE_EDP);
		if (edp) {
			if (edp->deliver_dev)
				ret = edp->deliver_dev(edp, dev);
		}
	}

	return -1;
}

s32 bsp_disp_edp_drv_init(int dispdev_id)
{
	u32 disp = 0;
	u32 num_screens = 0;
	s32 ret = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *edp;

		edp = disp_device_find(disp, DISP_OUTPUT_TYPE_EDP);
		if (edp) {
			if (edp->drv_init)
				ret = edp->drv_init(edp, dispdev_id);
		}
	}

	return -1;
}

s32 bsp_disp_edp_drv_deinit(int dispdev_id)
{
	u32 disp = 0;
	u32 num_screens = 0;
	s32 ret = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *edp;

		edp = disp_device_find(disp, DISP_OUTPUT_TYPE_EDP);
		if (edp) {
			if (edp->drv_deinit)
				ret = edp->drv_deinit(edp, dispdev_id);
		}
	}

	return -1;
}

s32 bsp_disp_hdmi_check_support_mode(u32 disp, enum disp_output_type mode)
{
	u32 num_screens = 0;
	s32 ret = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *hdmi;

		hdmi = disp_device_find(disp, DISP_OUTPUT_TYPE_HDMI);
		if (hdmi && hdmi->check_support_mode) {
			ret = hdmi->check_support_mode(hdmi, (u32) mode);
			break;
		}
	}

	return ret;
}

s32 bsp_disp_edp_check_support_mode(u32 disp, enum disp_output_type mode)
{
	u32 num_screens = 0;
	s32 ret = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *edp;

		edp = disp_device_find(disp, DISP_OUTPUT_TYPE_EDP);
		if (edp && edp->check_support_mode) {
			ret = edp->check_support_mode(edp, (u32) mode);
			break;
		}
	}

	return ret;
}

s32 bsp_disp_hdmi_set_detect(bool hpd)
{
	u32 num_screens = 0;
	u32 disp;
	s32 ret = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *hdmi;

		hdmi = disp_device_find(disp, DISP_OUTPUT_TYPE_HDMI);
		if (hdmi && hdmi->set_detect) {
			ret = hdmi->set_detect(hdmi, hpd);
			break;
		}
	}

	return ret;
}

s32 bsp_disp_hdmi_cec_standby_request(void)
{
	u32 num_screens = 0;
	s32 ret = 0;
	u32 disp;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *hdmi;

		hdmi = disp_device_find(disp, DISP_OUTPUT_TYPE_HDMI);
		if (hdmi && hdmi->cec_standby_request) {
			ret = hdmi->cec_standby_request(hdmi);
			break;
		}
	}

	return ret;
}

s32 bsp_disp_hdmi_cec_send_one_touch_play(void)
{
	u32 num_screens = 0;
	s32 ret = 0;
	u32 disp;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *hdmi;

		hdmi = disp_device_find(disp, DISP_OUTPUT_TYPE_HDMI);
		if (hdmi && hdmi->cec_send_one_touch_play) {
			ret = hdmi->cec_send_one_touch_play(hdmi);
			break;
		}
	}

	return ret;
}

s32 bsp_disp_hdmi_get_color_format(void)
{
	u32 num_screens = 0;
	s32 ret = 0;
	u32 disp;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		struct disp_device *hdmi;

		hdmi = disp_device_find(disp, DISP_OUTPUT_TYPE_HDMI);
		if (hdmi && hdmi->get_input_csc) {
			ret = hdmi->get_input_csc(hdmi);
			break;
		}
	}

	return ret;

}

s32 bsp_disp_tv_set_hpd(u32 state)
{
#if defined SUPPORT_TV

	u32 disp = 0, num_screens = 0;
	s32 ret = 0;
	struct disp_device *ptv = NULL;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		ptv = disp_device_find(disp, DISP_OUTPUT_TYPE_TV);
		if (ptv) {
			ret = disp_tv_set_hpd(ptv, state);
		} else {
			/* ret &= ret; */
			/* DE_WARN("'ptv is null\n"); */
			continue;
		}
	}

	if (ret != 0) {
		DE_WARN("'tv set hpd is fail\n");
		return -1;
	}
#endif

	return 0;
}

s32 bsp_disp_tv_register(struct disp_tv_func *func)
{
#if defined SUPPORT_TV
	u32 disp = 0;
	u32 num_screens = 0;
	s32 ret = 0, registered_cnt = 0;
	struct disp_device *dispdev = NULL;

	num_screens = bsp_disp_feat_get_num_screens();
	disp_init_tv();
	for (disp = 0; disp < num_screens; disp++) {
		dispdev = disp_device_find(disp, DISP_OUTPUT_TYPE_TV);
		if (dispdev && dispdev->set_tv_func) {
			ret = dispdev->set_tv_func(dispdev, func);
			if (ret == 0)
				registered_cnt++;
		}
	}

#if defined(SUPPORT_VGA)
	disp_init_vga();
	for (disp = 0; disp < num_screens; disp++) {
		dispdev = disp_device_find(disp, DISP_OUTPUT_TYPE_VGA);
		if (dispdev && dispdev->set_tv_func) {
			ret = dispdev->set_tv_func(dispdev, func);
			if (ret == 0)
				registered_cnt++;
		}
	}
#endif
	if (registered_cnt != 0) {
		gdisp.tv_registered = 1;
		if (gdisp.init_para.start_process)
			gdisp.init_para.start_process();
	}
#endif
	return 0;
}

/**
 * All lcd object should do set_panel_func
 */
s32 bsp_disp_lcd_set_panel_funs(char *name, struct disp_lcd_panel_fun *lcd_cfg)
{
	struct disp_device *lcd;
	u32 hwdev_index;
	u32 lcd_devices;
	u32 registered_cnt = 0;

	lcd_devices = disp_lcd_get_device_count();
	for (hwdev_index = 0; hwdev_index < lcd_devices; hwdev_index++) {
		lcd = disp_get_lcd(hwdev_index);
		if (lcd && (lcd->set_panel_func)) {
			if (!lcd->set_panel_func(lcd, name, lcd_cfg)) {
				gdisp.lcd_registered[hwdev_index] = 1;
				registered_cnt++;
				DE_INFO("panel driver %s register in lcd[%d] success\n", name, hwdev_index);
			}
		}
	}

	return 0;
}

s32 bsp_disp_edp_set_panel_funs(char *name, struct disp_lcd_panel_fun *edp_cfg)
{
	struct disp_device *edp;
	u32 num_screens;
	u32 screen_id;
	u32 registered_cnt = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		edp = disp_get_edp(screen_id);
		if (edp && (edp->set_panel_func)) {
			if (!edp->set_panel_func(edp, name, edp_cfg)) {
				registered_cnt++;
				DE_INFO("panel driver %s register\n", name);
			}
		}
	}

	return 0;
}

void LCD_OPEN_FUNC(u32 disp, LCD_FUNC func, u32 delay)
{
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);

	if (lcd && lcd->set_open_func)
		lcd->set_open_func(lcd, func, delay);
}
EXPORT_SYMBOL(LCD_OPEN_FUNC);

void LCD_CLOSE_FUNC(u32 disp, LCD_FUNC func, u32 delay)
{
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);

	if (lcd && lcd->set_close_func)
		lcd->set_close_func(lcd, func, delay);
}
EXPORT_SYMBOL(LCD_CLOSE_FUNC);

void EDP_OPEN_FUNC(u32 disp, EDP_FUNC func, u32 delay)
{
	struct disp_device *edp;

	edp = disp_get_edp(disp);

	if (edp && edp->set_open_func)
		edp->set_open_func(edp, func, delay);
}
EXPORT_SYMBOL(EDP_OPEN_FUNC);

void EDP_CLOSE_FUNC(u32 disp, EDP_FUNC func, u32 delay)
{
	struct disp_device *edp;

	edp = disp_get_edp(disp);

	if (edp && edp->set_close_func)
		edp->set_close_func(edp, func, delay);
}
EXPORT_SYMBOL(EDP_CLOSE_FUNC);

s32 bsp_disp_get_lcd_registered(u32 disp)
{
	return gdisp.lcd_registered[disp];
}

s32 bsp_disp_get_hdmi_registered(void)
{
	return gdisp.hdmi_registered;
}

s32 bsp_disp_get_edp_registered(void)
{
	return gdisp.edp_registered;
}

s32 bsp_disp_get_tv_registered(void)
{
	return gdisp.tv_registered;
}

s32 bsp_disp_lcd_backlight_enable(u32 disp)
{
	s32 ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd == NULL)
		return ret;

	if (lcd->backlight_enable)
		ret = lcd->backlight_enable(lcd);

	return ret;
}

s32 bsp_disp_lcd_backlight_disable(u32 disp)
{
	s32 ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd == NULL)
		return ret;

	if (lcd && lcd->backlight_disable)
		ret = lcd->backlight_disable(lcd);

	return ret;
}

s32 bsp_disp_lcd_pwm_enable(u32 disp)
{
	s32 ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd == NULL)
		return ret;

	if (lcd && lcd->pwm_enable)
		ret = lcd->pwm_enable(lcd);

	return ret;
}

s32 bsp_disp_lcd_pwm_disable(u32 disp)
{
	s32 ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd == NULL)
		return ret;

	if (lcd && lcd->pwm_disable)
		ret = lcd->pwm_disable(lcd);

	return ret;
}

s32 bsp_disp_lcd_power_enable(u32 disp, u32 power_id)
{
	s32 ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd == NULL)
		return ret;

	if (lcd && lcd->power_enable)
		ret = lcd->power_enable(lcd, power_id);

	return ret;
}

s32 bsp_disp_lcd_power_disable(u32 disp, u32 power_id)
{
	s32 ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd == NULL)
		return ret;

	if (lcd && lcd->power_disable)
		ret = lcd->power_disable(lcd, power_id);

	return ret;
}

s32 bsp_disp_lcd_set_bright(u32 disp, u32 bright)
{
	s32 ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd == NULL)
		return ret;

	if (lcd && lcd->set_bright)
		ret = lcd->set_bright(lcd, bright);

	return ret;
}

s32 bsp_disp_lcd_get_bright(u32 disp)
{
	u32 bright = 0;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd && lcd->get_bright)
		bright = lcd->get_bright(lcd);

	return bright;
}


s32 bsp_disp_edp_backlight_enable(u32 disp)
{
	s32 ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp == NULL)
		return ret;

	if (edp->backlight_enable)
		ret = edp->backlight_enable(edp);

	return ret;
}

s32 bsp_disp_edp_backlight_disable(u32 disp)
{
	s32 ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp == NULL)
		return ret;

	if (edp && edp->backlight_disable)
		ret = edp->backlight_disable(edp);

	return ret;
}

s32 bsp_disp_edp_pwm_enable(u32 disp)
{
	s32 ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp == NULL)
		return ret;

	if (edp && edp->pwm_enable)
		ret = edp->pwm_enable(edp);

	return ret;
}

s32 bsp_disp_edp_pwm_disable(u32 disp)
{
	s32 ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp == NULL)
		return ret;

	if (edp && edp->pwm_disable)
		ret = edp->pwm_disable(edp);

	return ret;
}

s32 bsp_disp_edp_power_enable(u32 disp, u32 power_id)
{
	s32 ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp == NULL)
		return ret;

	if (edp && edp->power_enable)
		ret = edp->power_enable(edp, power_id);

	return ret;
}

s32 bsp_disp_edp_power_disable(u32 disp, u32 power_id)
{
	s32 ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp == NULL)
		return ret;

	if (edp && edp->power_disable)
		ret = edp->power_disable(edp, power_id);

	return ret;
}

s32 bsp_disp_edp_set_bright(u32 disp, u32 bright)
{
	s32 ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp == NULL)
		return ret;

	if (edp && edp->set_bright)
		ret = edp->set_bright(edp, bright);

	return ret;
}

s32 bsp_disp_edp_get_bright(u32 disp)
{
	u32 bright = 0;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp && edp->get_bright)
		bright = edp->get_bright(edp);

	return bright;
}

s32 bsp_disp_edp_pin_cfg(u32 disp, u32 en)
{
	int ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp && edp->pin_cfg)
		ret = edp->pin_cfg(edp, en);

	return ret;
}

s32 bsp_disp_edp_gpio_set_value(u32 disp, u32 io_index, u32 value)
{
	int ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp && edp->gpio_set_value)
		ret = edp->gpio_set_value(edp, io_index, value);

	return ret;
}

s32 bsp_disp_edp_gpio_set_direction(u32 disp, unsigned int io_index,
				    u32 direction)
{
	int ret = -1;
	struct disp_device *edp;

	edp = disp_get_edp(disp);
	if (edp && edp->gpio_set_direction)
		ret = edp->gpio_set_direction(edp, io_index, direction);

	return ret;
}

s32 bsp_disp_lcd_tcon_enable(u32 disp)
{
	int ret = -1;
	struct disp_device *lcd;
	struct disp_device *lcd_slave;

	struct disp_panel_para *panel_info =
	    kmalloc(sizeof(struct disp_panel_para), GFP_KERNEL | __GFP_ZERO);

	if (panel_info == NULL)
		goto OUT;

	lcd = disp_get_lcd(disp);
	if (lcd == NULL)
		goto FREE_INFO;

	if (lcd && lcd->get_panel_info)
		ret = lcd->get_panel_info(lcd, panel_info);

	if (ret != 0)
		goto FREE_INFO;

	if (panel_info->lcd_tcon_mode == DISP_TCON_SLAVE_MODE) {
		ret = 0;
		goto FREE_INFO;
	}

	if (lcd && lcd->tcon_enable)
		ret = lcd->tcon_enable(lcd);

	if (panel_info->lcd_tcon_mode <= DISP_TCON_MASTER_SYNC_EVERY_FRAME &&
	    panel_info->lcd_tcon_mode >= DISP_TCON_MASTER_SYNC_AT_FIRST_TIME) {
		lcd_slave = disp_get_lcd(panel_info->lcd_slave_tcon_num);
		if (lcd_slave == NULL)
			goto FREE_INFO;
		if (lcd_slave && lcd_slave->tcon_enable)
			ret = lcd_slave->tcon_enable(lcd_slave);
	}

FREE_INFO:
	if (panel_info != NULL)
		kfree(panel_info);
OUT:
	return ret;
}

s32 bsp_disp_lcd_tcon_disable(u32 disp)
{
	int ret = -1;
	struct disp_device *lcd;
	struct disp_device *lcd_slave;

	struct disp_panel_para *panel_info =
	    kmalloc(sizeof(struct disp_panel_para), GFP_KERNEL | __GFP_ZERO);

	if (panel_info == NULL)
		goto OUT;

	lcd = disp_get_lcd(disp);
	if (lcd == NULL)
		goto FREE_INFO;

	if (lcd && lcd->get_panel_info)
		ret = lcd->get_panel_info(lcd, panel_info);

	if (ret != 0)
		goto FREE_INFO;

	if (lcd && lcd->tcon_disable)
		ret = lcd->tcon_disable(lcd);

	if (panel_info->lcd_tcon_mode <= DISP_TCON_MASTER_SYNC_EVERY_FRAME &&
	    panel_info->lcd_tcon_mode >= DISP_TCON_MASTER_SYNC_AT_FIRST_TIME) {
		lcd_slave = disp_get_lcd(panel_info->lcd_slave_tcon_num);
		if (lcd_slave == NULL)
			goto FREE_INFO;
		if (lcd_slave && lcd_slave->tcon_disable)
			ret = lcd_slave->tcon_disable(lcd_slave);
	}

FREE_INFO:
	if (panel_info != NULL)
		kfree(panel_info);
OUT:
	return ret;
}

s32 bsp_disp_lcd_pin_cfg(u32 disp, u32 en)
{
	int ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd && lcd->pin_cfg)
		ret = lcd->pin_cfg(lcd, en);

	return ret;
}

s32 bsp_disp_lcd_gpio_set_value(u32 disp, u32 io_index, u32 value)
{
	int ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd && lcd->gpio_set_value)
		ret = lcd->gpio_set_value(lcd, io_index, value);

	return ret;
}

s32 bsp_disp_lcd_gpio_get_value(u32 disp, u32 io_index)
{
	int ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd && lcd->gpio_get_value)
		ret = lcd->gpio_get_value(lcd, io_index);

	return ret;
}
s32 bsp_disp_lcd_gpio_set_direction(u32 disp, unsigned int io_index,
				    u32 direction)
{
	int ret = -1;
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (lcd && lcd->gpio_set_direction)
		ret = lcd->gpio_set_direction(lcd, io_index, direction);

	return ret;
}

s32 bsp_disp_get_panel_info(u32 disp, struct disp_panel_para *info)
{
	struct disp_device *lcd;

	lcd = disp_get_lcd(disp);
	if (!lcd)
		DE_WARN("get lcd%d fail\n", disp);

	if (lcd && lcd->get_panel_info)
		return lcd->get_panel_info(lcd, info);

	return DIS_FAIL;
}

int bsp_disp_get_display_size(u32 disp, unsigned int *width,
			      unsigned int *height)
{
	return disp_al_get_display_size(disp, width, height);
}
#if defined(SUPPORT_DSI)
/**
 * @name       :bsp_disp_lcd_dsi_mode_switch
 * @brief      :dsi module mode switch
 * @param[IN]  :cmd_en: command mode enable
 * @param[IN]  :lp_en: lower power mode enable
 * @return     :0 if success
 */
s32 bsp_disp_lcd_dsi_mode_switch(u32 disp, u32 cmd_en, u32 lp_en)
{
	s32 ret = -1;
	struct disp_panel_para *panel_info =
	    kmalloc(sizeof(struct disp_panel_para), GFP_KERNEL | __GFP_ZERO);

	ret = bsp_disp_get_panel_info(disp, panel_info);
	if (ret == DIS_FAIL) {
		DE_WARN("Get panel info failed\n");
		goto OUT;
	}

	if (panel_info->lcd_tcon_mode == DISP_TCON_SLAVE_MODE)
		goto OUT;

	ret = dsi_mode_switch(disp, cmd_en, lp_en);
	if (panel_info->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
	    disp + 1 < DEVICE_DSI_NUM)
		ret = dsi_mode_switch(disp + 1, cmd_en, lp_en);
	else if (panel_info->lcd_tcon_mode != DISP_TCON_NORMAL_MODE &&
		 panel_info->lcd_tcon_mode != DISP_TCON_DUAL_DSI)
		ret = dsi_mode_switch(panel_info->lcd_slave_tcon_num, cmd_en,
				      lp_en);
OUT:
	kfree(panel_info);
	return ret;
}

s32 bsp_disp_lcd_dsi_clk_enable(u32 disp, u32 en)
{
	s32 ret = -1;
	struct disp_panel_para *panel_info =
	    kmalloc(sizeof(struct disp_panel_para), GFP_KERNEL | __GFP_ZERO);

	ret = bsp_disp_get_panel_info(disp, panel_info);
	if (ret == DIS_FAIL) {
		DE_WARN("Get panel info failed\n");
		goto OUT;
	}

	if (panel_info->lcd_tcon_mode == DISP_TCON_SLAVE_MODE)
		goto OUT;

	ret = dsi_clk_enable(disp, en);
	if (panel_info->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
	    disp + 1 < DEVICE_DSI_NUM)
		ret = dsi_clk_enable(disp + 1, en);
	else if (panel_info->lcd_tcon_mode != DISP_TCON_NORMAL_MODE &&
		 panel_info->lcd_tcon_mode != DISP_TCON_DUAL_DSI)
		ret = dsi_clk_enable(panel_info->lcd_slave_tcon_num, en);
OUT:
	kfree(panel_info);
	return ret;
}

s32 bsp_disp_lcd_dsi_dcs_wr(u32 disp, u8 command, u8 *para, u32 para_num)
{
	s32 ret = -1;
	struct disp_panel_para *panel_info =
	    kmalloc(sizeof(struct disp_panel_para), GFP_KERNEL | __GFP_ZERO);

	ret = bsp_disp_get_panel_info(disp, panel_info);
	if (ret == DIS_FAIL) {
		DE_WARN("Get panel info failed\n");
		goto OUT;
	}

	if (panel_info->lcd_tcon_mode == DISP_TCON_SLAVE_MODE)
		goto OUT;

	ret = dsi_dcs_wr(disp, command, para, para_num);
	if (panel_info->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
	    disp + 1 < DEVICE_DSI_NUM &&
	    panel_info->lcd_dsi_port_num == DISP_LCD_DSI_SINGLE_PORT)
		ret = dsi_dcs_wr(disp + 1, command, para, para_num);
	else if (panel_info->lcd_tcon_mode != DISP_TCON_NORMAL_MODE &&
		 panel_info->lcd_tcon_mode != DISP_TCON_DUAL_DSI)
		ret = dsi_dcs_wr(panel_info->lcd_slave_tcon_num, command, para,
				 para_num);
OUT:
	kfree(panel_info);
	return ret;
}

s32 bsp_disp_lcd_dsi_gen_wr(u32 disp, u8 command, u8 *para, u32 para_num)
{
	s32 ret = -1;
	struct disp_panel_para *panel_info =
	    kmalloc(sizeof(struct disp_panel_para), GFP_KERNEL | __GFP_ZERO);

	ret = bsp_disp_get_panel_info(disp, panel_info);
	if (ret == DIS_FAIL) {
		DE_WARN("Get panel info failed\n");
		goto OUT;
	}

	if (panel_info->lcd_tcon_mode == DISP_TCON_SLAVE_MODE)
		goto OUT;

	ret = dsi_gen_wr(disp, command, para, para_num);
	if (panel_info->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
	    disp + 1 < DEVICE_DSI_NUM &&
	    panel_info->lcd_dsi_port_num == DISP_LCD_DSI_SINGLE_PORT)
		ret = dsi_gen_wr(disp + 1, command, para, para_num);
	else if (panel_info->lcd_tcon_mode != DISP_TCON_NORMAL_MODE &&
		 panel_info->lcd_tcon_mode != DISP_TCON_DUAL_DSI)
		ret = dsi_gen_wr(panel_info->lcd_slave_tcon_num, command, para,
				 para_num);
OUT:
	kfree(panel_info);
	return ret;
}

s32 bsp_disp_lcd_dsi_gen_short_read(u32 sel, u8 *para_p, u8 para_num,
				    u8 *result)
{
	s32 ret = -1;

	if (!result || !para_p || para_num > 2) {
		DE_WARN("Wrong para\n");
		goto OUT;
	}
	ret = dsi_gen_short_rd(sel, para_p, para_num, result);
OUT:
	return ret;
}

s32 bsp_disp_lcd_dsi_dcs_read(u32 sel, u8 cmd, u8 *result, u32 *num_p)
{
	s32 ret = -1;

	if (!result || !num_p) {
		DE_WARN("Wrong para\n");
		goto OUT;
	}
	ret = dsi_dcs_rd(sel, cmd, result, num_p);
OUT:
	return ret;
}

s32 bsp_disp_lcd_set_max_ret_size(u32 sel, u32 size)
{
	return dsi_set_max_ret_size(sel, size);
}
#endif /* endif SUPPORT_DSI */

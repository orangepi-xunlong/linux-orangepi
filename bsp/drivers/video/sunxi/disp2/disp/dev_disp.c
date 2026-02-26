/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de/disp_lcd.h"
#include "dev_disp.h"
#include "fb_top.h"
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY)
#include <linux/sunxi_dramfreq.h>
#endif
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/reset.h>
#include <linux/dma-heap.h>
#if IS_ENABLED(CONFIG_AW_IOMMU)
#include "sunxi-iommu.h"
#endif
#ifndef dma_mmap_writecombine
#define dma_mmap_writecombine dma_mmap_wc
#endif

#if IS_ENABLED(CONFIG_PM)
#define SUPPORT_PM_RUNTIME
#endif

#define DISP_MEM_NUM 10
struct disp_drv_info g_disp_drv;
/* alloc based on 4K byte */
#define MY_BYTE_ALIGN(x) (((x + (4*1024-1)) >> 12) << 12)

static u32 suspend_output_type[4] = {0};
/*
 * 0:normal;
 * suspend_status&1 != 0:in early_suspend;
 * suspend_status&2 != 0:in suspend;
 */
u32 suspend_status;
/* 0:after early suspend; 1:after suspend; 2:after resume;3:after late resume */
static u32 suspend_prestep = 3;

/* static unsigned int gbuffer[4096]; */
static struct info_mm g_disp_mm[DISP_MEM_NUM];
static int g_disp_mem_id = -1;

static struct cdev *my_cdev;
static dev_t devid;
static struct class *disp_class;
static struct device *display_dev;

static unsigned int g_disp = 0, g_enhance_mode = 0, g_cvbs_enhance_mode;
static u32 DISP_print = 0xffff;	/* print cmd which eq DISP_print */
static bool g_pm_runtime_enable;

#ifdef SUPPORT_EINK
struct disp_layer_config_inner eink_para[16];
#endif

struct disp_layer_config lyr_cfg[MAX_LAYERS];
struct disp_layer_config2 lyr_cfg2[MAX_LAYERS];
struct disp_layer_config2 lyr_cfg2_1[MAX_LAYERS];
static spinlock_t sync_finish_lock;
unsigned int bright_csc = 50, contrast_csc = 50, satuation_csc = 50;

static atomic_t g_driver_ref_count;
static u8 palette_data[256*4];
#ifndef CONFIG_OF
static struct sunxi_disp_mod disp_mod[] = {
	{DISP_MOD_DE, "de"},
	{DISP_MOD_LCD0, "lcd0"},
	{DISP_MOD_DSI0, "dsi0"},
#ifdef DISP_SCREEN_NUM
#if DISP_SCREEN_NUM == 2
	{DISP_MOD_LCD1, "lcd1"}
#endif
#else
#	error "DISP_SCREEN_NUM undefined!"
#endif
};

static struct resource disp_resource[] = {
};
#endif

#if IS_ENABLED(CONFIG_AW_DISP2_COMPOSER)
int composer_init(struct disp_drv_info *p_disp_drv);
int hwc_dump(char *buf);
#endif

void disp_set_suspend_output_type(u8 disp, u8 output_type)
{
	suspend_output_type[disp] = output_type;
}

static void disp_shutdown(struct platform_device *pdev);
static ssize_t disp_sys_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;
	ssize_t count = 0;
	int num_screens, screen_id;
	int num_layers, layer_id;
	int num_chans, chan_id;
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
	struct disp_lcd_esd_info esd_inf;

	memset(&esd_inf, 0, sizeof(struct disp_lcd_esd_info));
#endif
	/* int hpd; */

	num_screens = bsp_disp_feat_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		u32 width = 0, height = 0;
		int fps = 0;
		struct disp_health_info info;

		mgr = disp_get_layer_manager(screen_id);
		if (mgr == NULL)
			continue;
		dispdev = mgr->device;
		if (dispdev == NULL)
			continue;
		dispdev->get_resolution(dispdev, &width, &height);
		fps = bsp_disp_get_fps(screen_id);
		bsp_disp_get_health_info(screen_id, &info);

		if (!dispdev->is_enabled(dispdev))
			continue;
		count += sprintf(buf + count, "screen[%d] -> dev[%d]\n", screen_id, dispdev->hwdev_index);
		count += sprintf(buf + count, "de_rate %d hz, ref_fps:%d\n",
				 mgr->get_clk_rate(mgr),
				 dispdev->get_fps(dispdev));
		count += mgr->dump(mgr, buf + count);
		/* output */
		if (dispdev->type == DISP_OUTPUT_TYPE_LCD) {
			count += sprintf(buf + count,
				"\tlcd output\tbacklight(%3d)\tfps:%d.%d",
				dispdev->get_bright(dispdev), fps / 10,
				fps % 10);
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
			if (dispdev->get_esd_info) {
				dispdev->get_esd_info(dispdev, &esd_inf);
				count += sprintf(buf + count,
				"\tesd level(%u)\tfreq(%u)\tpos(%u)\treset(%u)",
				esd_inf.level, esd_inf.freq,
				esd_inf.esd_check_func_pos, esd_inf.rst_cnt);
			}
#endif
		} else if (dispdev->type == DISP_OUTPUT_TYPE_HDMI) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\thdmi output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		} else if (dispdev->type == DISP_OUTPUT_TYPE_TV) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\ttv output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		} else if (dispdev->type == DISP_OUTPUT_TYPE_VGA) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\tvga output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		}  else if (dispdev->type == DISP_OUTPUT_TYPE_VDPO) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\tvdpo output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		}  else if (dispdev->type == DISP_OUTPUT_TYPE_RTWB) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\trtwb output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		} else if (dispdev->type == DISP_OUTPUT_TYPE_EDP) {
			count += sprintf(
			    buf + count, "\tEDP output(%s) \tfps:%d.%d",
			    (dispdev->is_enabled(dispdev) == 1) ? "enable"
								: "disable",
			    fps / 10, fps % 10);
		}
		if (dispdev->type != DISP_OUTPUT_TYPE_NONE) {
			if (dispdev->get_real_fps != NULL)
				count += sprintf(buf + count, "\t%4ux%4u@%ufps\n",
					 width, height, dispdev->get_real_fps(dispdev));
			else
				count += sprintf(buf + count, "\t%4ux%4u\n",
					 width, height);
			count += sprintf(buf + count,
					"\terr:%u\tskip:%u\tirq:%llu\tvsync:%u\tvsync_skip:%u\t\n",
					info.error_cnt, info.skip_cnt,
					info.irq_cnt, info.vsync_cnt,
					info.vsync_skip_cnt);
		}

		num_chans = bsp_disp_feat_get_num_channels(screen_id);

		/* layer info */
		for (chan_id = 0; chan_id < num_chans; chan_id++) {
			num_layers =
			    bsp_disp_feat_get_num_layers_by_chn(screen_id,
								chan_id);
			for (layer_id = 0; layer_id < num_layers; layer_id++) {
				struct disp_layer *lyr = NULL;
				struct disp_layer_config config;

				lyr = disp_get_layer(screen_id, chan_id,
						     layer_id);
				config.channel = chan_id;
				config.layer_id = layer_id;
				mgr->get_layer_config(mgr, &config, 1);
				if (lyr && (true == config.enable) && lyr->dump)
					count += lyr->dump(lyr, buf + count);
			}
		}
	}
#if IS_ENABLED(CONFIG_AW_DISP2_COMPOSER)
	count += hwc_dump(buf + count);
#endif

	return count;
}

static ssize_t disp_sys_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(sys, 0660,
		   disp_sys_show, disp_sys_store);

static ssize_t disp_disp_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_disp);
}

static ssize_t disp_disp_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int err;
	unsigned long val;
	unsigned int num_screens;

	err = kstrtoul(buf, 10, &val);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}

	num_screens = bsp_disp_feat_get_num_screens();
	if (val > num_screens)
		DE_WARN("Invalid value, <%d is expected\n", num_screens);
	else
		g_disp = val;

	return count;
}

static DEVICE_ATTR(disp, 0660,
		   disp_disp_show, disp_disp_store);

static ssize_t disp_enhance_mode_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_enhance_mode);
}

static ssize_t disp_enhance_mode_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = kstrtoul(buf, 10, &val);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}

	/*
	 * mode: 0: standard; 1: vivid; 2: soft; 3: demo vivid
	 */
	if (val > 3)
		DE_WARN("Invalid value, 0~3 is expected\n");
	else {
		int num_screens = 2;
		struct disp_manager *mgr = NULL;
		struct disp_enhance *enhance = NULL;

		g_enhance_mode = val;

		num_screens = bsp_disp_feat_get_num_screens();

		if (g_disp < num_screens)
			mgr = g_disp_drv.mgr[g_disp];

		if (mgr) {
			enhance = mgr->enhance;
			if (enhance && enhance->set_mode)
#if IS_ENABLED(CONFIG_ARCH_SUN8IW15) || IS_ENABLED(CONFIG_ARCH_SUN50IW1)
				enhance->set_mode(enhance,
						  (g_enhance_mode == 2) ?
						  1 : g_enhance_mode);
			if (g_enhance_mode == 2)
				g_enhance_mode = 3;
#else
			enhance->set_mode(enhance,
					  (g_enhance_mode == 3) ?
					  1 : g_enhance_mode);
#endif

			if (enhance && enhance->demo_enable
			    && enhance->demo_disable) {
				if (g_enhance_mode == 3)
					enhance->demo_enable(enhance);
				else
					enhance->demo_disable(enhance);
			}
		}
	}

	return count;
}

static DEVICE_ATTR(enhance_mode, 0660,
		   disp_enhance_mode_show, disp_enhance_mode_store);
int __attribute__ ((weak))
_csc_enhance_setting[3][4] = {
	{50, 50, 50, 50},
	{50, 50, 50, 50},
	{50, 40, 50, 50},
};

static ssize_t disp_enhance_bright_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;
	int value = 0;
	int real_mode = (g_enhance_mode == 3) ? 1 : g_enhance_mode;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->get_bright)
			value = enhance->get_bright(enhance);
	}

	return sprintf(buf, "%d %d\n", _csc_enhance_setting[real_mode][0], value);
}

static ssize_t disp_enhance_bright_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	int err;
	unsigned long value;
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;
	int real_mode = (g_enhance_mode == 3) ? 1 : g_enhance_mode;

	err = kstrtoul(buf, 10, &value);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}

	if (g_enhance_mode == 0)
		return count;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->set_bright) {
			_csc_enhance_setting[real_mode][0] = value;
			enhance->set_bright(enhance, value);
		}
#if defined(DE_VERSION_V35X)
#else
		if (enhance && enhance->set_mode) {
			enhance->set_mode(enhance, real_mode ? 0 : 1);
			enhance->set_mode(enhance, real_mode);
		}
#endif
	}

	return count;
}
static DEVICE_ATTR(enhance_bright, 0660,
	disp_enhance_bright_show, disp_enhance_bright_store);

static ssize_t disp_enhance_saturation_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;
	int value = 0;
	int real_mode = (g_enhance_mode == 3) ? 1 : g_enhance_mode;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->get_saturation)
			value = enhance->get_saturation(enhance);
	}

	return sprintf(buf, "%d %d\n", _csc_enhance_setting[real_mode][2], value);
}

static ssize_t disp_enhance_saturation_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	int err;
	unsigned long value;
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;
	int real_mode = (g_enhance_mode == 3) ? 1 : g_enhance_mode;

	err = kstrtoul(buf, 10, &value);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}

	if (g_enhance_mode == 0)
		return count;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->set_saturation) {
			_csc_enhance_setting[real_mode][2] = value;
			enhance->set_saturation(enhance, value);
		}
#if defined(DE_VERSION_V35X)
#else
		if (enhance && enhance->set_mode) {
			enhance->set_mode(enhance, real_mode ? 0 : 1);
			enhance->set_mode(enhance, real_mode);
		}
#endif
	}

	return count;
}
static DEVICE_ATTR(enhance_saturation, 0660,
	disp_enhance_saturation_show, disp_enhance_saturation_store);

static ssize_t disp_enhance_contrast_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;
	int value = 0;
	int real_mode = (g_enhance_mode == 3) ? 1 : g_enhance_mode;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->get_contrast)
			value = enhance->get_contrast(enhance);
	}

	return sprintf(buf, "%d %d\n", _csc_enhance_setting[real_mode][1], value);
}

static ssize_t disp_enhance_contrast_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	int err;
	unsigned long value;
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;
	int real_mode = (g_enhance_mode == 3) ? 1 : g_enhance_mode;

	err = kstrtoul(buf, 10, &value);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}
	if (g_enhance_mode == 0)
		return count;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->set_contrast) {
			_csc_enhance_setting[real_mode][1] = value;
			enhance->set_contrast(enhance, value);
		}
#if defined(DE_VERSION_V35X)
#else
		if (enhance && enhance->set_mode) {
			enhance->set_mode(enhance, real_mode ? 0 : 1);
			enhance->set_mode(enhance, real_mode);
		}
#endif
	}

	return count;
}
static DEVICE_ATTR(enhance_contrast, 0660,
	disp_enhance_contrast_show, disp_enhance_contrast_store);

static ssize_t disp_enhance_edge_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;
	int value = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->get_edge)
			value = enhance->get_edge(enhance);
	}

	return sprintf(buf, "%d\n", value);
}

static ssize_t disp_enhance_edge_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	int err;
	unsigned long value;
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;

	err = kstrtoul(buf, 10, &value);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->set_edge)
			enhance->set_edge(enhance, value);
	}

	return count;
}
static DEVICE_ATTR(enhance_edge, 0660,
	disp_enhance_edge_show, disp_enhance_edge_store);

static ssize_t disp_enhance_detail_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;
	int value = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->get_detail)
			value = enhance->get_detail(enhance);
	}

	return sprintf(buf, "%d\n", value);
}

static ssize_t disp_enhance_detail_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	int err;
	unsigned long value;
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;

	err = kstrtoul(buf, 10, &value);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->set_detail)
			enhance->set_detail(enhance, value);
	}

	return count;
}
static DEVICE_ATTR(enhance_detail, 0660,
	disp_enhance_detail_show, disp_enhance_detail_store);

static ssize_t disp_enhance_denoise_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;
	int value = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->get_denoise)
			value = enhance->get_denoise(enhance);
	}

	return sprintf(buf, "%d\n", value);
}

static ssize_t disp_enhance_denoise_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	int err;
	unsigned long value;
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_enhance *enhance = NULL;

	err = kstrtoul(buf, 10, &value);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr) {
		enhance = mgr->enhance;
		if (enhance && enhance->set_denoise)
			enhance->set_denoise(enhance, value);
	}

	return count;
}
static DEVICE_ATTR(enhance_denoise, 0660,
	disp_enhance_denoise_show, disp_enhance_denoise_store);

static ssize_t disp_cvbs_enhance_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_cvbs_enhance_mode);
}

static ssize_t disp_cvbs_enhance_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int err;
	unsigned long val;
	int num_screens = 0;
	unsigned int disp;
	struct disp_device *ptv = NULL;

	err = kstrtoul(buf, 10, &val);

	g_cvbs_enhance_mode = val;
	num_screens = bsp_disp_feat_get_num_screens();

	for (disp = 0; disp < num_screens; disp++) {
		ptv = disp_device_find(disp, DISP_OUTPUT_TYPE_TV);
		if (ptv && ptv->set_enhance_mode)
			ptv->set_enhance_mode(ptv, g_cvbs_enhance_mode);
	}

	return count;
}

static DEVICE_ATTR(cvbs_enhacne_mode, 0660,
		   disp_cvbs_enhance_show, disp_cvbs_enhance_store);

static ssize_t disp_runtime_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", g_pm_runtime_enable);
}

static ssize_t disp_runtime_enable_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = kstrtoul(buf, 10, &val);
	if (val > 1)
		DE_WARN("Invalid value, 0/1 is expected\n");
	else
		g_pm_runtime_enable = val;

	return count;
}

static DEVICE_ATTR(runtime_enable, 0660,
		   disp_runtime_enable_show, disp_runtime_enable_store);
static ssize_t disp_color_temperature_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;
	int value = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr && mgr->device) {
		dispdev = mgr->device;
		if (dispdev->get_color_temperature)
			value = dispdev->get_color_temperature(dispdev);
	}

	return sprintf(buf, "%d\n", value);
}

static ssize_t disp_color_temperature_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	int err;
	long value;
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;

	err = kstrtol(buf, 10, &value);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}

	if ((value > 256) || (value < -256)) {
		DE_WARN("value shoud in range [-256,256]\n");
		value = (value > 256) ? 256 : value;
		value = (value < -256) ? -256 : value;
	}

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr && mgr->device) {
		dispdev = mgr->device;
		if (dispdev->set_color_temperature)
			value = dispdev->set_color_temperature(dispdev, value);
	}

	return count;
}

static DEVICE_ATTR(color_temperature, 0660,
	disp_color_temperature_show, disp_color_temperature_store);


static ssize_t disp_boot_para_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
#if DISP_SCREEN_NUM > 1
		return sprintf(buf, "disp_para=%x init_disp=%x tv_vdid=%x fb_base=0x%x disp_config0=%d,%u - %d,%d,%d,%d disp_config1=%d,%u - %d,%d,%d,%d\n",
		disp_boot_para_parse("boot_disp"),
		disp_boot_para_parse("init_disp"),
		disp_boot_para_parse("tv_vdid"),
		disp_boot_para_parse("fb_base"),
		g_disp_drv.disp_init.output_type[0], g_disp_drv.disp_init.output_mode[0],
		g_disp_drv.disp_init.output_format[0], g_disp_drv.disp_init.output_bits[0],
		g_disp_drv.disp_init.output_cs[0], g_disp_drv.disp_init.output_eotf[0],
		g_disp_drv.disp_init.output_type[1], g_disp_drv.disp_init.output_mode[1],
		g_disp_drv.disp_init.output_format[1], g_disp_drv.disp_init.output_bits[1],
		g_disp_drv.disp_init.output_cs[1], g_disp_drv.disp_init.output_eotf[1]);
#else
		return sprintf(buf, "disp_para=%x init_disp=%x tv_vdid=%x fb_base=0x%x disp_config0=%d,%u - %d,%d,%d,%d\n",
		disp_boot_para_parse("boot_disp"),
		disp_boot_para_parse("init_disp"),
		disp_boot_para_parse("tv_vdid"),
		disp_boot_para_parse("fb_base"),
		g_disp_drv.disp_init.output_type[0], g_disp_drv.disp_init.output_mode[0],
		g_disp_drv.disp_init.output_format[0], g_disp_drv.disp_init.output_bits[0],
		g_disp_drv.disp_init.output_cs[0], g_disp_drv.disp_init.output_eotf[0]);
#endif
}

static ssize_t disp_xres_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;
	u32 width = 0, height;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr && mgr->device) {
		dispdev = mgr->device;
		if (dispdev->get_resolution)
			dispdev->get_resolution(dispdev, &width, &height);
	}

	return sprintf(buf, "%d\n", width);
}

static ssize_t disp_yres_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;
	u32 width = 0, height = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (mgr && mgr->device) {
		dispdev = mgr->device;
		if (dispdev->get_resolution)
			dispdev->get_resolution(dispdev, &width, &height);
	}

	return sprintf(buf, "%d\n", height);
}

/**
 * @name       :disp_draw_colorbar
 * @brief      :draw colorbar using DE's LAYER MODE
 * @param[IN]  :disp:screen index
 * @return     :0 if success
 */
int disp_draw_colorbar(u32 disp, u8 zorder)
{
	struct disp_manager *mgr = NULL;
	struct disp_layer_config config[4];
	unsigned int i = 0;
	unsigned int width = 0, height = 0, num_screens;
	int ret = -1;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp < num_screens)
		mgr = g_disp_drv.mgr[disp];
	else
		return ret;

	if (mgr && mgr->device && mgr->device->get_resolution)
		mgr->device->get_resolution(mgr->device, &width, &height);
	else
		return ret;

	memset(config, 0, 4 * sizeof(struct disp_layer_config));
	for (i = 0; i < 4; ++i) {
		config[i].channel = 0;
		config[i].layer_id = i;
		config[i].enable = 1;
		config[i].info.zorder = zorder;
		config[i].info.mode = LAYER_MODE_COLOR;
		config[i].info.fb.format = DISP_FORMAT_ARGB_8888;
		config[i].info.screen_win.width = width / 4;
		config[i].info.screen_win.height = height;
		config[i].info.screen_win.x = (width / 4) * i;
		config[i].info.screen_win.y = 0;
		config[i].info.fb.crop.x =
		    ((long long)(config[i].info.screen_win.x) << 32);
		config[i].info.fb.crop.y =
		    ((long long)(config[i].info.screen_win.y) << 32);
		config[i].info.fb.crop.width =
		    ((long long)(config[i].info.screen_win.width) << 32);
		config[i].info.fb.crop.height =
		    ((long long)(config[i].info.screen_win.height) << 32);
	}
	config[0].info.color = 0xffff0000; /* red */
	config[1].info.color = 0xff00ff00; /* green */
	config[2].info.color = 0xff0000ff; /* blue */
	config[3].info.color = 0xffffff00; /* yellow */

	if (mgr->set_layer_config)
		ret = mgr->set_layer_config(mgr, config, 4);

	return ret;
}

static ssize_t disp_colorbar_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	int err;
	unsigned int val;
	unsigned int num_screens;
	struct disp_manager *mgr = NULL;

	err = kstrtou32(buf, 10, &val);
	if (err) {
		DE_WARN("Invalid size\n");
		return err;
	}

	num_screens = bsp_disp_feat_get_num_screens();

	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	/* val: */
	/* 0:DE-->tcon-->other interface */
	/* 1-7:tcon or edp or other device's builtin patten */
	/* for tcon: */
	/* 1:color bar */
	/* 2:grayscale check */
	/* 3:black and white check */
	/* 4:all 0 */
	/* 5:all 1 */
	/* 6:reserve */
	/* 7:Gridding */
	/* for edp: */
	/* 1:colorbar */
	/* 2:mosaic */
	if (val == 8) {
		disp_draw_colorbar(g_disp, 16);
		if (mgr && mgr->device && mgr->device->show_builtin_patten)
			mgr->device->show_builtin_patten(mgr->device, 0);
	} else {
		if (mgr && mgr->device && mgr->device->show_builtin_patten)
			mgr->device->show_builtin_patten(mgr->device, val);
	}

	return count;
}

static ssize_t disp_capture_dump_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
#ifndef MODULE
	struct file *pfile;
	ssize_t bw;
	loff_t pos = 0;
	dma_addr_t phy_addr = 0;
	void *buf_addr_vir = NULL;
	struct disp_capture_info cptr_info;
	unsigned int size = 0, width = 0, height = 0, num_screens = 0;
	struct disp_manager *mgr = NULL;
	char *image_name = NULL;
	int ret = -1, cs = DISP_CSC_TYPE_RGB;
	struct bmp_header bmp_header;

	MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
	num_screens = bsp_disp_feat_get_num_screens();

	if (g_disp < num_screens)
		mgr = g_disp_drv.mgr[g_disp];

	if (!mgr || !mgr->device || !mgr->cptr)
		goto OUT;

	memset(&cptr_info, 0, sizeof(struct disp_capture_info));

	image_name = kmalloc(count, GFP_KERNEL | __GFP_ZERO);
	if (!image_name) {
		DE_WARN("kmalloc image name fail\n");
		goto OUT;
	}
	strncpy(image_name, buf, count);
	image_name[count - 1] = '\0';

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	{
		mm_segment_t old_fs;
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		pfile = filp_open(image_name, O_RDWR | O_CREAT | O_EXCL, 0755);
		set_fs(old_fs);
	}
#else
	pfile = filp_open(image_name, O_RDWR | O_CREAT | O_EXCL, 0755);
#endif

	if (IS_ERR(pfile)) {
		DE_WARN("open %s err\n", image_name);
		goto FREE;
	}

	if (mgr->device->get_resolution)
		ret = mgr->device->get_resolution(mgr->device, &width, &height);
	if (ret) {
		DE_WARN("Get resolution fail\n");
		goto FILE_CLOSE;
	}

	cptr_info.out_frame.size[0].width = width;
	cptr_info.out_frame.size[0].height = height;
	cptr_info.window.width = width;
	cptr_info.window.height = height;
	cptr_info.out_frame.crop.width = width;
	cptr_info.out_frame.crop.height = height;
	if (strstr(image_name, ".bmp"))
		cptr_info.out_frame.format = DISP_FORMAT_ARGB_8888;
	else if (strstr(image_name, ".yuv420_p"))
		cptr_info.out_frame.format = DISP_FORMAT_YUV420_P;
	else if (strstr(image_name, ".yuv420_sp_uvuv"))
		cptr_info.out_frame.format = DISP_FORMAT_YUV420_SP_UVUV;
	else if (strstr(image_name, ".yuv420_sp_vuvu"))
		cptr_info.out_frame.format = DISP_FORMAT_YUV420_SP_VUVU;
	else if (strstr(image_name, ".argb8888"))
		cptr_info.out_frame.format = DISP_FORMAT_ARGB_8888;
	else if (strstr(image_name, ".abgr8888"))
		cptr_info.out_frame.format = DISP_FORMAT_ABGR_8888;
	else if (strstr(image_name, ".rgb888"))
		cptr_info.out_frame.format = DISP_FORMAT_RGB_888;
	else if (strstr(image_name, ".bgr888"))
		cptr_info.out_frame.format = DISP_FORMAT_BGR_888;
	else if (strstr(image_name, ".rgba8888"))
		cptr_info.out_frame.format = DISP_FORMAT_RGBA_8888;
	else if (strstr(image_name, ".bgra8888"))
		cptr_info.out_frame.format = DISP_FORMAT_BGRA_8888;
	else {
		if (mgr->device->get_input_csc)
			cs = mgr->device->get_input_csc(mgr->device);
		if (cs == DISP_CSC_TYPE_RGB)
			cptr_info.out_frame.format = DISP_FORMAT_ARGB_8888;
		else
			cptr_info.out_frame.format = DISP_FORMAT_YUV420_P;
	}

	size = width * height * 4;

	buf_addr_vir = disp_malloc(size, (void *)&phy_addr);
	if (!phy_addr || !buf_addr_vir) {
		DE_WARN("disp_malloc phy_addr err\n");
		goto FILE_CLOSE;
	}

	cptr_info.out_frame.addr[0] = (unsigned long)phy_addr;
	cptr_info.out_frame.addr[1] =
	    cptr_info.out_frame.addr[0] + width * height;
	cptr_info.out_frame.addr[2] =
	    cptr_info.out_frame.addr[1] + width * height / 4;

	ret = mgr->cptr->start(mgr->cptr);
	if (ret) {
		mgr->cptr->stop(mgr->cptr);
		goto FREE_DMA;
	}

	ret = mgr->cptr->commmit(mgr->cptr, &cptr_info);
	if (ret) {
		mgr->cptr->stop(mgr->cptr);
		goto FREE_DMA;
	}
	disp_delay_ms(2000);
	ret = mgr->cptr->stop(mgr->cptr);
	if (ret)
		goto FREE_DMA;

	if (strstr(image_name, ".bmp")) {
		memset(&bmp_header, 0, sizeof(struct bmp_header));
		bmp_header.signature[0] = 'B';
		bmp_header.signature[1] = 'M';
		bmp_header.data_offset = sizeof(struct bmp_header);
		bmp_header.file_size = bmp_header.data_offset + size;
		bmp_header.size = sizeof(struct bmp_header) - 14;
		bmp_header.width = width;
		bmp_header.height = -height;
		bmp_header.planes = 1;
		bmp_header.bit_count = 32;
		bmp_header.image_size = size;
		bw = kernel_write(pfile, (const char *)&bmp_header,
			       sizeof(struct bmp_header), &pos);
		pos = sizeof(struct bmp_header);
	}

	bw = kernel_write(pfile, (char *)buf_addr_vir, size, &pos);
	if (unlikely(bw != size))
		DE_WARN("write %s err at byte offset %llu\n",
		      image_name, pfile->f_pos);

FREE_DMA:
	disp_free((void *)buf_addr_vir, (void *)phy_addr, size);
FILE_CLOSE:
	filp_close(pfile, NULL);
FREE:
	kfree(image_name);
	image_name = NULL;
OUT:
#endif
	return count;
}

static DEVICE_ATTR(boot_para, 0660, disp_boot_para_show, NULL);
static DEVICE_ATTR(xres, 0660, disp_xres_show, NULL);
static DEVICE_ATTR(yres, 0660, disp_yres_show, NULL);
static DEVICE_ATTR(colorbar, 0660, NULL, disp_colorbar_store);
static DEVICE_ATTR(capture_dump, 0660, NULL, disp_capture_dump_store);

static struct attribute *disp_attributes[] = {
	&dev_attr_sys.attr,
	&dev_attr_disp.attr,
	&dev_attr_enhance_mode.attr,
	&dev_attr_cvbs_enhacne_mode.attr,
	&dev_attr_runtime_enable.attr,
	&dev_attr_enhance_bright.attr,
	&dev_attr_enhance_saturation.attr,
	&dev_attr_enhance_contrast.attr,
	&dev_attr_enhance_edge.attr,
	&dev_attr_enhance_detail.attr,
	&dev_attr_enhance_denoise.attr,
	&dev_attr_color_temperature.attr,
	&dev_attr_boot_para.attr,
	&dev_attr_xres.attr,
	&dev_attr_yres.attr,
	&dev_attr_colorbar.attr,
	&dev_attr_capture_dump.attr,
	NULL
};

static struct attribute_group disp_attribute_group = {
	.name = "attr",
	.attrs = disp_attributes
};

unsigned int disp_boot_para_parse(const char *name)
{
	unsigned int value = 0;

	if (!g_disp_drv.dev->of_node) {
	    DE_ERR("disp_boot_para_parse failed, of node is NULL\n");
	    return 0;
	}

	if (of_property_read_u32(g_disp_drv.dev->of_node, name, &value) < 0)
		DE_INFO("of_property_read disp.%s fail\n", name);

	DE_INFO("%s:0x%x\n", name, value);
	return value;
}
EXPORT_SYMBOL(disp_boot_para_parse);

bool disp_init_ready(void)
{
	if (!g_disp_drv.dev)
		return false;

	return true;
}
EXPORT_SYMBOL(disp_init_ready);

unsigned int disp_boot_para_parse_array(const char *name, unsigned int *value,
							  unsigned int count)
{
	unsigned int ret = 0;

	ret = of_property_read_u32_array(g_disp_drv.dev->of_node, name,
							  value, count);
	if (ret)
		DE_WARN("of_property_read_array disp.%s fail\n", name);

	return ret;
}
EXPORT_SYMBOL(disp_boot_para_parse_array);


const char *disp_boot_para_parse_str(const char *name)
{
	const char *str;

	if (!of_property_read_string(g_disp_drv.dev->of_node, name, &str))
		return str;

	DE_INFO("of_property_read_string disp.%s fail\n", name);

	return NULL;
}
EXPORT_SYMBOL(disp_boot_para_parse_str);

static s32 parser_disp_init_para(const struct device_node *np,
				 struct disp_init_para *init_para)
{
	int value, i, tmp[4] = {0};
	char key_name[64] = {0};
	const char *device_type_str = NULL;
	int ret = 0;

	memset(init_para, 0, sizeof(struct disp_init_para));

	if (of_property_read_u32(np, "disp_init_enable", &value) < 0) {
		DE_WARN("of_property_read disp_init.disp_init_enable fail\n");
		return -1;
	}
	init_para->b_init = value;

	if (of_property_read_u32(np, "chn_cfg_mode", &value) < 0)
		value = 0;
	init_para->chn_cfg_mode = value;

	if (of_property_read_u32(np, "disp_mode", &value) < 0) {
		DE_WARN("of_property_read disp_init.disp_mode fail\n");
		return -1;
	}
	init_para->disp_mode = value;

	/* screen0 */
	if (of_property_read_u32(np, "screen0_output_type", &value) < 0) {
		DE_WARN("of_property_read disp_init.screen0_output_type fail\n");
		return -1;
	}
	if (value == 0) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_NONE;
	} else if (value == 1) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_LCD;
	} else if (value == 2) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_TV;
	} else if (value == 3) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_HDMI;
	} else if (value == 4) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_VGA;
	} else if (value == 5) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_VDPO;
	} else if (value == 6) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_EDP;
	} else if (value == 7) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_RTWB;
	} else {
		DE_WARN("invalid screen0_output_type %d\n",
		      init_para->output_type[0]);
		return -1;
	}
	if (of_property_read_u32(np, "screen0_amp_preview", &value) < 0)
		value = 0;
	init_para->amp_preview[0] = value;

	if (init_para->amp_preview[0]) {
		ret |= of_property_read_u32_array(np, "sunxi-iova-premap", tmp, 4);
		init_para->premap_addr = tmp[1];
		ret |= of_property_read_u32(np, "preview_src_width", &value);
		init_para->src_width = value;
		ret |= of_property_read_u32(np, "preview_src_height", &value);
		init_para->src_height = value;
		ret |= of_property_read_u32(np, "preview_aux_line_en", &value);
		init_para->aux_line_en = value;
		ret |= of_property_read_u32(np, "preview_screen_x", &value);
		init_para->screen_x = value;
		ret |= of_property_read_u32(np, "preview_screen_y", &value);
		init_para->screen_y = value;
		ret |= of_property_read_u32(np, "preview_screen_w", &value);
		init_para->screen_w = value;
		ret |= of_property_read_u32(np, "preview_screen_h", &value);
		init_para->screen_h = value;
		ret |= of_property_read_u32(np, "preview_format", &value);
		init_para->preview_format = value;
		if (ret) {
			DE_WARN("parse amp preview error\n");
			return -1;
		}
	}


	if (of_property_read_u32(np, "screen0_output_mode", &value) < 0) {
		DE_WARN("of_property_read disp_init.screen0_output_mode fail\n");
		return -1;
	}

	if (init_para->output_type[0] != DISP_OUTPUT_TYPE_NONE &&
		init_para->output_type[0] != DISP_OUTPUT_TYPE_LCD)
		init_para->output_mode[0] = value;

	if (of_property_read_u32(np, "screen0_output_format", &value) < 0) {
		DE_INFO("of_property_read screen0_output_format fail\n");
	} else {
		init_para->output_format[0] = value;
		init_para->using_device_config[0] = true;
	}

	if (of_property_read_u32(np, "screen0_to_lcd_index", &value) < 0) {
		DE_INFO("of_property_read screen0_to_lcd_index fail\n");
		init_para->to_lcd_index[0] = DE_TO_TCON_NOT_DEFINE;
	} else {
		init_para->to_lcd_index[0] = value;
		init_para->using_device_config[0] = true;
	}

	if (of_property_read_u32(np, "screen0_output_bits", &value) < 0) {
		DE_INFO("of_property_read screen0_output_bits fail\n");
	} else {
		init_para->output_bits[0] = value;
		init_para->using_device_config[0] = true;
	}

	if (of_property_read_u32(np, "screen0_output_eotf", &value) < 0) {
		DE_INFO("of_property_read screen0_output_eotf fail\n");
	} else {
		init_para->output_eotf[0] = value;
		init_para->using_device_config[0] = true;
	}

	if (of_property_read_u32(np, "screen0_output_cs", &value) < 0) {
		DE_INFO("of_property_read screen0_output_cs fail\n");
	} else {
		init_para->output_cs[0] = value;
		init_para->using_device_config[0] = true;
	}

	if (of_property_read_u32(np, "screen0_output_dvi_hdmi", &value) < 0) {
		DE_INFO("of_property_read screen0_output_dvi_hdmi fail\n");
	} else {
		init_para->output_dvi_hdmi[0] = value;
		init_para->using_device_config[0] = true;
	}

	if (of_property_read_u32(np, "screen0_output_range", &value) < 0) {
		DE_INFO("of_property_read screen0_output_range fail\n");
	} else {
		init_para->output_range[0] = value;
		init_para->using_device_config[0] = true;
	}

	if (of_property_read_u32(np, "screen0_output_scan", &value) < 0) {
		DE_INFO("of_property_read screen0_output_scan fail\n");
	} else {
		init_para->output_scan[0] = value;
		init_para->using_device_config[0] = true;
	}

	if (of_property_read_u32(np, "screen0_output_aspect_ratio", &value) < 0) {
		DE_INFO("of_property_read screen0_output_aspect_ratio fail\n");
	} else {
		init_para->output_aspect_ratio[0] = value;
		init_para->using_device_config[0] = true;
	}

	if (of_property_read_u32(np, "display_device_num", &value) < 0) {
		value = 2;
		DE_INFO("of_property_read disp_init.primary_de_id fail\n");
	}
	init_para->disp_device_num = value <= 8 ? value : 8;

	for (i = 0; i < init_para->disp_device_num; i++) {
		if (i == 0) {
			if (of_property_read_u32(np, "primary_de_id", &value) < 0) {
				value = i;
				DE_INFO("of_property_read disp_init.primary_de_id fail\n");
			}
			init_para->logic_id[i] = i;
			if (value >= DISP_SCREEN_NUM) {
				DE_INFO("disp_init.de_id[%d] over range, default 0\n", i);
				init_para->de_id[i] = 0;
			} else
				init_para->de_id[i] = value;

			if (of_property_read_string(np, "primary_display_type", &device_type_str) != 0) {
				memcpy(&init_para->type[i][0], "NONE", sizeof("NONE"));
				DE_INFO("of_property_read disp_init.primary_display_type fail\n");
			} else {
				if (strlen(device_type_str) < 16) {
					memcpy(&init_para->type[i][0], device_type_str,
					       strlen(device_type_str));
				} else {
					memcpy(&init_para->type[i][0], "NONE", sizeof("NONE"));
					DE_INFO("primary_display_type over 16 range\n");
				}
			}

			if (of_property_read_u32(np, "primary_framebuffer_width", &value) < 0) {
				value = 0;
				DE_INFO("of_property_read disp_init.\
				      primary_framebuffer_width fail\n");
			}
			init_para->framebuffer_width[i] = value;

			if (of_property_read_u32(np, "primary_framebuffer_height", &value) < 0) {
				value = 0;
				DE_INFO("of_property_read disp_init.\
				      primary_framebuffer_height fail\n");
			}
			init_para->framebuffer_height[i] = value;

			if (of_property_read_u32(np, "primary_dpix", &value) < 0) {
				value = 0;
				DE_INFO("of_property_read disp_init.primary_dpix fail\n");
			}
			init_para->dpix[i] = value;

			if (of_property_read_u32(np, "primary_dpiy", &value) < 0) {
				value = 0;
				DE_INFO("of_property_read disp_init.primary_dpiy fail\n");
			}
			init_para->dpiy[i] = value;
			continue;
		}

		sprintf(key_name, "extend%d_de_id", i - 1);
		if (of_property_read_u32(np, key_name, &value) < 0) {
			value = i;
			DE_INFO("of_property_read disp_init.%s fail\n", key_name);
		}
		init_para->logic_id[i] = 1;
		if (value >= DISP_SCREEN_NUM) {
			DE_INFO("disp_init.de_id[%d] over range, default 0\n", i);
			init_para->de_id[i] = 0;
		} else
			init_para->de_id[i] = value;

		sprintf(key_name, "extend%d_display_type", i - 1);
		if (of_property_read_string(np, key_name, &device_type_str) != 0) {
			memcpy(&init_para->type[i][0], "NONE", sizeof("NONE"));
			DE_INFO("of_property_read disp_init.%s fail\n", key_name);
		} else {
			if (strlen(device_type_str) < 16) {
				memcpy(&init_para->type[i][0], device_type_str,
				       strlen(device_type_str));
			} else {
				memcpy(&init_para->type[i][0], "NONE", sizeof("NONE"));
				DE_INFO("primary_display_type over 16 range\n");
			}
		}

		sprintf(key_name, "extend%d_framebuffer_width", i - 1);
		if (of_property_read_u32(np, key_name, &value) < 0) {
			value = 0;
			DE_INFO("of_property_read disp_init.%s fail\n", key_name);
		}
		init_para->framebuffer_width[i] = value;

		sprintf(key_name, "extend%d_framebuffer_height", i - 1);
		if (of_property_read_u32(np, key_name, &value) < 0) {
			value = 0;
			DE_INFO("of_property_read disp_init.%s fail\n", key_name);
		}
		init_para->framebuffer_height[i] = value;

		sprintf(key_name, "extend%d_dpix", i - 1);
		if (of_property_read_u32(np, key_name, &value) < 0) {
			value = 0;
			DE_INFO("of_property_read disp_init.%s fail\n", key_name);
		}
		init_para->dpix[i] = value;

		sprintf(key_name, "extend%d_dpiy", i - 1);
		if (of_property_read_u32(np, key_name, &value) < 0) {
			value = 0;
			DE_INFO("of_property_read disp_init.%s fail\n", key_name);
		}
		init_para->dpiy[i] = value;
	}

#if DISP_SCREEN_NUM > 1
		/* screen1 */
		if (of_property_read_u32(np,
					 "screen1_output_type",
					 &value) < 0) {
			DE_WARN("of_property_read screen1_output_type fail\n");
			return -1;
		}
		if (value == 0) {
			init_para->output_type[1] = DISP_OUTPUT_TYPE_NONE;
		} else if (value == 1) {
			init_para->output_type[1] = DISP_OUTPUT_TYPE_LCD;
		} else if (value == 2) {
			init_para->output_type[1] = DISP_OUTPUT_TYPE_TV;
		} else if (value == 3) {
			init_para->output_type[1] = DISP_OUTPUT_TYPE_HDMI;
		} else if (value == 4) {
			init_para->output_type[1] = DISP_OUTPUT_TYPE_VGA;
		} else if (value == 5) {
			init_para->output_type[1] = DISP_OUTPUT_TYPE_VDPO;
		} else if (value == 6) {
			init_para->output_type[1] = DISP_OUTPUT_TYPE_EDP;
		} else if (value == 7) {
			init_para->output_type[1] = DISP_OUTPUT_TYPE_RTWB;
		} else {
			DE_WARN("invalid screen1_output_type %d\n",
			      init_para->output_type[1]);
			return -1;
		}

		if (of_property_read_u32(np, "screen1_output_mode", &value) < 0)
			DE_INFO
			    ("of_property_read screen1_output_mode fail\n");
		if (init_para->output_type[1] != DISP_OUTPUT_TYPE_NONE &&
		    init_para->output_type[1] != DISP_OUTPUT_TYPE_LCD)
			init_para->output_mode[1] = value;

		if (of_property_read_u32(np,
					 "screen1_output_format", &value) < 0) {
			DE_INFO("of_property_read screen1_output_format fail\n");
		} else {
			init_para->output_format[1] = value;
			init_para->using_device_config[1] = true;
		}

		if (of_property_read_u32(np, "screen1_to_lcd_index", &value) < 0) {
			DE_INFO("of_property_read screen1_to_lcd_index fail\n");
			init_para->to_lcd_index[1] = DE_TO_TCON_NOT_DEFINE;
		} else {
			init_para->to_lcd_index[1] = value;
			init_para->using_device_config[1] = true;
		}

		if (of_property_read_u32(np,
					 "screen1_output_bits", &value) < 0) {
			DE_INFO("of_property_read screen1_output_bits fail\n");
		} else {
			init_para->output_bits[1] = value;
			init_para->using_device_config[1] = true;
		}

		if (of_property_read_u32(np,
					 "screen1_output_eotf", &value) < 0) {
			DE_INFO("of_property_read screen1_output_eotf fail\n");
		} else {
			init_para->output_eotf[1] = value;
			init_para->using_device_config[1] = true;
		}

		if (of_property_read_u32(np, "screen1_output_cs", &value) < 0) {
			DE_INFO("of_property_read screen1_output_cs fail\n");
		} else {
			init_para->output_cs[1] = value;
			init_para->using_device_config[1] = true;
		}

		if (of_property_read_u32(np, "screen1_output_dvi_hdmi", &value) < 0) {
			DE_INFO(
			    "of_property_read screen1_output_dvi_hdmi fail\n");
		} else {
			init_para->output_dvi_hdmi[1] = value;
			init_para->using_device_config[1] = true;
		}

		if (of_property_read_u32(np, "screen1_output_range", &value) < 0) {
			DE_INFO("of_property_read screen1_output_range fail\n");
		} else {
			init_para->output_range[1] = value;
			init_para->using_device_config[1] = true;
		}

		if (of_property_read_u32(np, "screen1_output_scan", &value) < 0) {
			DE_INFO("of_property_read screen1_output_scan fail\n");
		} else {
			init_para->output_scan[1] = value;
			init_para->using_device_config[1] = true;
		}

		if (of_property_read_u32(np, "screen1_output_aspect_ratio", &value) < 0) {
			DE_INFO("read screen1_output_aspect_ratio fail\n");
		} else {
			init_para->output_aspect_ratio[1] = value;
			init_para->using_device_config[1] = true;
		}
#endif

	DE_INFO("====display init para begin====\n");
	DE_INFO("b_init:%d\n", init_para->b_init);
	DE_INFO("disp_mode:%d\n\n", init_para->disp_mode);
	for (i = 0; i < DISP_SCREEN_NUM; i++) {
		DE_INFO("output_type[%d]:%d\n", i, init_para->output_type[i]);
		DE_INFO("output_mode[%d]:%d\n", i, init_para->output_mode[i]);
	}
	for (i = 0; i < DISP_SCREEN_NUM; i++) {
		DE_INFO("buffer_num[%d]:%d\n", i, init_para->buffer_num[i]);
		DE_INFO("format[%d]:%d\n", i, init_para->format[i]);
		DE_INFO("fb_width[%d]:%d\n", i, init_para->fb_width[i]);
		DE_INFO("fb_height[%d]:%d\n", i, init_para->fb_height[i]);
	}
	DE_INFO("====display init para end====\n");

	return 0;
}

void *disp_malloc(u32 num_bytes, void *phys_addr)
{
	u32 actual_bytes;
	void *address = NULL;

	if (num_bytes != 0) {
		actual_bytes = MY_BYTE_ALIGN(num_bytes);

		address =
		    dma_alloc_coherent(g_disp_drv.dev, actual_bytes,
				       (dma_addr_t *) phys_addr, GFP_KERNEL);
		if (address) {
			DE_INFO
			    ("dma_alloc_coherent ok, address=0x%p, size=0x%x\n",
			     (void *)(*(unsigned long *)phys_addr), num_bytes);
			return address;
		}

		DE_WARN("dma_alloc_coherent fail, size=0x%x\n", num_bytes);
		return NULL;
	}

	DE_WARN("size is zero\n");

	return NULL;
}

void disp_free(void *virt_addr, void *phys_addr, u32 num_bytes)
{
	u32 actual_bytes;

	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if (phys_addr && virt_addr)
		dma_free_coherent(g_disp_drv.dev, actual_bytes, virt_addr,
				  (dma_addr_t)phys_addr);
}

#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
static int init_disp_ion_mgr(struct disp_ion_mgr *ion_mgr)
{
	if (ion_mgr == NULL) {
		DE_WARN("input param is null\n");
		return -EINVAL;
	}

	mutex_init(&(ion_mgr->mlock));

	mutex_lock(&(ion_mgr->mlock));
	INIT_LIST_HEAD(&(ion_mgr->ion_list));
	mutex_unlock(&(ion_mgr->mlock));

	return 0;
}

static int __disp_dma_heap_alloc_coherent(struct disp_ion_mem *mem)
{
	struct dma_buf *dmabuf;
	struct dma_heap *dmaheap;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) &&\
    LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	int ret;
	struct dma_buf_map map;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	int ret;
	struct iosys_map map;
#endif

#if IS_ENABLED(CONFIG_AW_IOMMU) || IS_ENABLED(CONFIG_ARM_SMMU_V3)
	dmaheap = dma_heap_find("system-uncached");
#else
	dmaheap = dma_heap_find("reserved");
#endif

	if (IS_ERR_OR_NULL(dmaheap)) {
		DE_WARN("failed, size=%u dmaheap=0x%p\n", (unsigned int)mem->size, dmaheap);
		return -2;
	}

	dmabuf = dma_heap_buffer_alloc(dmaheap, mem->size, O_RDWR, 0);

	if (IS_ERR_OR_NULL(dmabuf)) {
		DE_WARN("failed, size=%u dmabuf=0x%p\n", (unsigned int)mem->size, dmabuf);
		return -2;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = dma_buf_vmap(dmabuf, &map);
	mem->vaddr = map.vaddr;
	if (ret) {
		DE_WARN("dma_buf_vmap failed!\n");
		goto err_map_kernel;
	}
#else
	mem->vaddr = dma_buf_vmap(dmabuf);
#endif

	if (IS_ERR_OR_NULL(mem->vaddr)) {
		DE_WARN("ion_map_kernel failed!\n");
		goto err_map_kernel;
	}

	DE_DEBUG("ion map kernel, vaddr=0x%p\n", mem->vaddr);
	mem->p_item = kmalloc(sizeof(struct dmabuf_item), GFP_KERNEL);

	attachment = dma_buf_attach(dmabuf, g_disp_drv.dev);
	if (IS_ERR(attachment)) {
		DE_WARN("dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		DE_WARN("dma_buf_map_attachment failed\n");
		/* FIXME, wait iommu ready */
		return -1;
		/* goto err_buf_detach; */
	}

	mem->p_item->dmabuf = dmabuf;
	mem->p_item->sgt = sgt;
	mem->p_item->attachment = attachment;
	mem->p_item->dma_addr = sg_dma_address(sgt->sgl);

	return 0;
	/* unmap attachment sgt, not sgt_bak, cause it's not alloc yet! */
	dma_buf_unmap_attachment(attachment, sgt, DMA_FROM_DEVICE);
/* err_buf_detach: */
	dma_buf_detach(dmabuf, attachment);
err_buf_put:
	dma_buf_put(dmabuf);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	map.vaddr = mem->vaddr;
	dma_buf_vunmap(mem->p_item->dmabuf, &map);
#else
	dma_buf_vunmap(mem->p_item->dmabuf, mem->vaddr);
#endif
err_map_kernel:
	dma_heap_buffer_free(mem->p_item->dmabuf);
	return -ENOMEM;
}

static void __disp_ion_free_coherent(struct disp_ion_mem *mem)
{
	struct dmabuf_item item;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) &&\
    LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	struct dma_buf_map map = DMA_BUF_MAP_INIT_VADDR(mem->vaddr);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(mem->vaddr);
#endif
	memcpy(&item, mem->p_item, sizeof(struct dmabuf_item));
	disp_dma_unmap(mem->p_item);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	dma_buf_vunmap(item.dmabuf, &map);
#else
	dma_buf_vunmap(item.dmabuf, mem->vaddr);
#endif
	dma_heap_buffer_free(item.dmabuf);
	return;
}



struct disp_ion_mem *disp_ion_malloc(u32 num_bytes, void *phys_addr)
{
	struct disp_ion_mgr *ion_mgr = &(g_disp_drv.ion_mgr);
	struct disp_ion_list_node *ion_node = NULL;
	struct disp_ion_mem *mem = NULL;
	u32 *paddr = NULL;
	int ret = -1;

	if (ion_mgr == NULL) {
		DE_WARN("disp ion manager has not initial yet\n");
		return NULL;
	}

	ion_node = kmalloc(sizeof(struct disp_ion_list_node), GFP_KERNEL);
	if (ion_node == NULL) {
		DE_WARN("fail to alloc ion node, size=%u\n",
		      (unsigned int)sizeof(struct disp_ion_list_node));
		return NULL;
	}
	mutex_lock(&(ion_mgr->mlock));
	mem = &ion_node->mem;
	mem->size = MY_BYTE_ALIGN(num_bytes);

	ret = __disp_dma_heap_alloc_coherent(mem);

	if (ret != 0) {
		DE_WARN("fail to alloc ion, ret=%d\n", ret);
		goto err_hdl;
	}

	paddr = (u32 *)phys_addr;
	*paddr = (u32)mem->p_item->dma_addr;
	list_add_tail(&(ion_node->node), &(ion_mgr->ion_list));

	mutex_unlock(&(ion_mgr->mlock));
	return mem;

err_hdl:
	kfree(ion_node);
	mutex_unlock(&(ion_mgr->mlock));

	return NULL;
}

int disp_get_ion_fd(struct disp_ion_mem *mem)
{
	return dma_buf_fd(mem->p_item->dmabuf, O_CLOEXEC);
}

void *disp_get_phy_addr(struct disp_ion_mem *mem)
{
	return (void *)mem->p_item->dma_addr;
}

void disp_ion_free(void *virt_addr, void *phys_addr, u32 num_bytes)
{
	struct disp_ion_mgr *ion_mgr = &(g_disp_drv.ion_mgr);
	struct disp_ion_list_node *ion_node = NULL, *tmp_ion_node = NULL;
	struct disp_ion_mem *mem = NULL;
	bool found = false;

	if (ion_mgr == NULL) {
		DE_WARN("disp ion manager has not initial yet\n");
		return;
	}


	mutex_lock(&(ion_mgr->mlock));
	list_for_each_entry_safe(ion_node, tmp_ion_node, &ion_mgr->ion_list,
				 node) {
		if (ion_node != NULL) {
			mem = &ion_node->mem;
			if ((((unsigned long)mem->p_item->dma_addr) ==
			     ((unsigned long)phys_addr)) &&
			    (((unsigned long)mem->vaddr) ==
			     ((unsigned long)virt_addr))) {
				__disp_ion_free_coherent(mem);
				__list_del_entry(&(ion_node->node));
				found = true;
				break;
			}
		}
	}
	mutex_unlock(&(ion_mgr->mlock));

	if (false == found) {
		DE_WARN("vaddr=0x%p, paddr=0x%p is not found in ion\n", virt_addr,
		      phys_addr);
	}
}

static void deinit_disp_ion_mgr(struct disp_ion_mgr *ion_mgr)
{
	struct disp_ion_list_node *ion_node = NULL, *tmp_ion_node = NULL;
	struct disp_ion_mem *mem = NULL;

	if (ion_mgr == NULL) {
		DE_WARN("input param is null\n");
		return;
	}

	mutex_lock(&(ion_mgr->mlock));
	list_for_each_entry_safe(ion_node, tmp_ion_node, &ion_mgr->ion_list,
				 node) {
		if (ion_node != NULL) {
			/* free all ion node */
			mem = &ion_node->mem;
			__disp_ion_free_coherent(mem);
			__list_del_entry(&(ion_node->node));
			kfree(ion_node);
		}
	}
	mutex_unlock(&(ion_mgr->mlock));
}
#endif

s32 disp_set_hdmi_func(struct disp_device_func *func)
{
	return bsp_disp_set_hdmi_func(func);
}
EXPORT_SYMBOL(disp_set_hdmi_func);

s32 disp_set_edp_func(struct disp_tv_func *func)
{
	return bsp_disp_set_edp_func(func);
}
EXPORT_SYMBOL(disp_set_edp_func);

s32 disp_deliver_edp_dev(struct device *dev)
{
	return bsp_disp_deliver_edp_dev(dev);
}
EXPORT_SYMBOL(disp_deliver_edp_dev);

s32 disp_edp_drv_init(int sel)
{
	return bsp_disp_edp_drv_init(sel);
}
EXPORT_SYMBOL(disp_edp_drv_init);

s32 disp_edp_drv_deinit(int sel)
{
	return bsp_disp_edp_drv_deinit(sel);
}
EXPORT_SYMBOL(disp_edp_drv_deinit);

s32 disp_set_vdpo_func(struct disp_tv_func *func)
{
	return bsp_disp_set_vdpo_func(func);
}
EXPORT_SYMBOL(disp_set_vdpo_func);

s32 disp_set_hdmi_detect(bool hpd)
{
	return bsp_disp_hdmi_set_detect(hpd);
}
EXPORT_SYMBOL(disp_set_hdmi_detect);

s32 disp_tv_register(struct disp_tv_func *func)
{
	return bsp_disp_tv_register(func);
}
EXPORT_SYMBOL(disp_tv_register);

static void resume_proc(unsigned int disp, struct disp_manager *mgr)
{
	if (!mgr || !mgr->device)
		return;

	if (mgr->device->type == DISP_OUTPUT_TYPE_LCD)
		mgr->device->fake_enable(mgr->device);
}

static void resume_work_0(struct work_struct *work)
{
	resume_proc(0, g_disp_drv.mgr[0]);
}

#if DISP_SCREEN_NUM > 1
static void resume_work_1(struct work_struct *work)
{
	resume_proc(1, g_disp_drv.mgr[1]);
}
#endif

int disp_device_set_config(struct disp_init_para *init,
					unsigned int screen_id)
{
	struct disp_device_config config;

	if (screen_id >= DISP_SCREEN_NUM) {
		DE_WARN("Out of range of screen index\n");
		return -1;
	}

	memset(&config, 0, sizeof(struct disp_device_config));
	config.type = init->output_type[screen_id];
	config.mode = init->output_mode[screen_id];
	config.format = init->output_format[screen_id];
	config.bits = init->output_bits[screen_id];
	config.eotf = init->output_eotf[screen_id];
	config.cs = init->output_cs[screen_id];
	config.dvi_hdmi = init->output_dvi_hdmi[screen_id];
	config.range = init->output_range[screen_id];
	config.scan = init->output_scan[screen_id];
	config.aspect_ratio = init->output_aspect_ratio[screen_id];
	if (!init->using_device_config[screen_id])
		return bsp_disp_device_switch(screen_id, config.type, (enum disp_output_type)config.mode);
	else
		return bsp_disp_device_set_config(screen_id, &config);
}

static void start_work(struct work_struct *work)
{
	int num_screens;
	int screen_id;
	int count = 0;
	int sync = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	while ((g_disp_drv.inited == 0) && (count < 5)) {
		count++;
		msleep(20);
	}
	if (count >= 5)
		DE_WARN("timeout\n");

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		if (g_disp_drv.para.boot_info[screen_id].sync == 1) {
			sync = 1;

			if ((g_disp_drv.para.boot_info[screen_id].type == DISP_OUTPUT_TYPE_HDMI)
					&& !bsp_disp_get_hdmi_registered()) {
				/* double display scene, make sure that DE0 will be enabled first */
				if (screen_id == 0)
					break;
				else
					continue;
			}

			if ((g_disp_drv.para.boot_info[screen_id].type == DISP_OUTPUT_TYPE_EDP)
					&& !bsp_disp_get_edp_registered()) {
				if (screen_id == 0)
					break;
				else
					continue;
			}

			if ((g_disp_drv.para.boot_info[screen_id].type == DISP_OUTPUT_TYPE_TV)
					&& !bsp_disp_get_tv_registered()) {
				if (screen_id == 0)
					break;
				else
					continue;
			}

			if (bsp_disp_get_output_type(g_disp_drv.para.boot_info[screen_id].disp) !=
					g_disp_drv.para.boot_info[screen_id].type) {
				bsp_disp_sync_with_hw(&g_disp_drv.para, screen_id);
				suspend_output_type[g_disp_drv.para.boot_info[screen_id].disp] =
					g_disp_drv.para.boot_info[screen_id].type;
			}

		}
	}

	if (sync == 0) {
		for (screen_id = 0; screen_id < num_screens; screen_id++) {
			int disp_mode = g_disp_drv.disp_init.disp_mode;
			int output_type =
			    g_disp_drv.disp_init.output_type[screen_id%DE_NUM];
			int lcd_registered =
			    bsp_disp_get_lcd_registered(screen_id);
			int hdmi_registered = bsp_disp_get_hdmi_registered();

			DE_INFO("sel=%d, output_type=%d, lcd_reg=%d,hdmi_reg=%d\n",
			     screen_id, output_type, lcd_registered, hdmi_registered);

			if (((disp_mode == DISP_INIT_MODE_SCREEN0)
			     && (screen_id == 0))
			    || ((disp_mode == DISP_INIT_MODE_SCREEN1)
				&& (screen_id == 1))) {
				if (output_type == DISP_OUTPUT_TYPE_LCD) {
					if (lcd_registered &&
					    bsp_disp_get_output_type(screen_id)
					    != DISP_OUTPUT_TYPE_LCD) {
						disp_device_set_config(
								       &g_disp_drv.disp_init, screen_id);
						suspend_output_type[screen_id] =
						    output_type;
					}
				} else if (output_type
				    == DISP_OUTPUT_TYPE_HDMI) {
					if (hdmi_registered &&
					    bsp_disp_get_output_type(screen_id)
					    != DISP_OUTPUT_TYPE_HDMI) {
						msleep(600);
						disp_device_set_config(
								       &g_disp_drv.disp_init, screen_id);
						suspend_output_type[screen_id] =
						    output_type;
					}
				} else {
					disp_device_set_config(
					    &g_disp_drv.disp_init, screen_id);
					suspend_output_type[screen_id] =
					    output_type;
				}
			}
		}
	}
}

static s32 start_process(void)
{
	flush_work(&g_disp_drv.start_work);
#if !IS_ENABLED(CONFIG_EINK_PANEL_USED) && !IS_ENABLED(CONFIG_EINK200_SUNXI)
	schedule_work(&g_disp_drv.start_work);
#endif
	return 0;
}

s32 disp_register_sync_proc(void (*proc) (u32))
{
	struct proc_list *new_proc;

	new_proc =
	    (struct proc_list *)disp_sys_malloc(sizeof(struct proc_list));
	if (new_proc) {
		new_proc->proc = proc;
		list_add_tail(&(new_proc->list),
			      &(g_disp_drv.sync_proc_list.list));
	} else {
		DE_WARN("malloc fail\n");
	}

	return 0;
}

s32 disp_unregister_sync_proc(void (*proc) (u32))
{
	struct proc_list *ptr, *ptrtmp;

	if (proc == NULL) {
		DE_WARN("hdl is NULL\n");
		return -1;
	}
	list_for_each_entry_safe(ptr,
				 ptrtmp,
				 &g_disp_drv.sync_proc_list.list,
				 list) {
		if (ptr->proc == proc) {
			list_del(&ptr->list);
			kfree((void *)ptr);
			return 0;
		}
	}

	return -1;
}

s32 disp_register_sync_finish_proc(void (*proc) (u32))
{
	struct proc_list *new_proc;

	new_proc =
	    (struct proc_list *)disp_sys_malloc(sizeof(struct proc_list));
	if (new_proc) {
		new_proc->proc = proc;
		list_add_tail(&(new_proc->list),
			      &(g_disp_drv.sync_finish_proc_list.list));
	} else {
		DE_WARN("malloc fail\n");
	}

	return 0;
}

s32 disp_unregister_sync_finish_proc(void (*proc) (u32))
{
	struct proc_list *ptr, *ptrtmp;
	unsigned long flags;

	spin_lock_irqsave(&sync_finish_lock, flags);
	if (proc == NULL) {
		DE_WARN("hdl is NULL\n");
		return -1;
	}
	list_for_each_entry_safe(ptr,
				 ptrtmp,
				 &g_disp_drv.sync_finish_proc_list.list,
				 list) {
		if (ptr->proc == proc) {
			list_del(&ptr->list);
			kfree((void *)ptr);
			return 0;
		}
	}
	spin_unlock_irqrestore(&sync_finish_lock, flags);

	return -1;
}

static s32 disp_sync_finish_process(u32 screen_id)
{
	struct proc_list *ptr;
	unsigned long flags;

	spin_lock_irqsave(&sync_finish_lock, flags);
	list_for_each_entry(ptr, &g_disp_drv.sync_finish_proc_list.list, list) {
		if (ptr->proc)
			ptr->proc(screen_id);
	}
	spin_unlock_irqrestore(&sync_finish_lock, flags);

	return 0;
}

s32 disp_register_ioctl_func(unsigned int cmd,
			     int (*proc)(unsigned int cmd, unsigned long arg))
{
	struct ioctl_list *new_proc;

	new_proc =
	    (struct ioctl_list *)disp_sys_malloc(sizeof(struct ioctl_list));
	if (new_proc) {
		new_proc->cmd = cmd;
		new_proc->func = proc;
		list_add_tail(&(new_proc->list),
			      &(g_disp_drv.ioctl_extend_list.list));
	} else {
		DE_WARN("malloc fail\n");
	}

	return 0;
}

s32 disp_unregister_ioctl_func(unsigned int cmd)
{
	struct ioctl_list *ptr;

	list_for_each_entry(ptr, &g_disp_drv.ioctl_extend_list.list, list) {
		if (ptr->cmd == cmd) {
			list_del(&ptr->list);
			kfree((void *)ptr);
			return 0;
		}
	}

	DE_WARN("no ioctl found(cmd:0x%x)\n", cmd);
	return -1;
}

static s32 disp_ioctl_extend(unsigned int cmd, unsigned long arg)
{
	struct ioctl_list *ptr;

	list_for_each_entry(ptr, &g_disp_drv.ioctl_extend_list.list, list) {
		if (cmd == ptr->cmd)
			return ptr->func(cmd, arg);
	}

	return -1;
}

s32 disp_register_compat_ioctl_func(unsigned int cmd,
				    int (*proc)(unsigned int cmd,
						 unsigned long arg))
{
	struct ioctl_list *new_proc;

	new_proc =
	    (struct ioctl_list *)disp_sys_malloc(sizeof(struct ioctl_list));
	if (new_proc) {
		new_proc->cmd = cmd;
		new_proc->func = proc;
		list_add_tail(&(new_proc->list),
			      &(g_disp_drv.compat_ioctl_extend_list.list));
	} else {
		DE_WARN("malloc fail\n");
	}

	return 0;
}

s32 disp_unregister_compat_ioctl_func(unsigned int cmd)
{
	struct ioctl_list *ptr;

	list_for_each_entry(ptr, &g_disp_drv.compat_ioctl_extend_list.list,
			    list) {
		if (ptr->cmd == cmd) {
			list_del(&ptr->list);
			kfree((void *)ptr);
			return 0;
		}
	}

	DE_WARN("no ioctl found(cmd:0x%x)\n", cmd);
	return -1;
}

#if IS_ENABLED(CONFIG_COMPAT)
static __attribute__((unused)) s32 disp_compat_ioctl_extend(unsigned int cmd, unsigned long arg)
{
	struct ioctl_list *ptr;

	list_for_each_entry(ptr, &g_disp_drv.compat_ioctl_extend_list.list,
			    list) {
		if (cmd == ptr->cmd)
			return ptr->func(cmd, arg);
	}

	return -1;
}
#endif

s32 disp_register_standby_func(int (*suspend) (void), int (*resume) (void))
{
	struct standby_cb_list *new_proc;

	new_proc = (struct standby_cb_list *)disp_sys_malloc(
	    sizeof(struct standby_cb_list));
	if (new_proc) {
		new_proc->suspend = suspend;
		new_proc->resume = resume;
		list_add_tail(&(new_proc->list),
			      &(g_disp_drv.stb_cb_list.list));
	} else {
		DE_WARN("malloc fail\n");
	}

	return 0;
}

s32 disp_unregister_standby_func(int (*suspend) (void), int (*resume) (void))
{
	struct standby_cb_list *ptr;

	list_for_each_entry(ptr, &g_disp_drv.stb_cb_list.list, list) {
		if ((ptr->suspend == suspend) && (ptr->resume == resume)) {
			list_del(&ptr->list);
			kfree((void *)ptr);
			return 0;
		}
	}

	return -1;
}

static s32 disp_suspend_cb(void)
{
	struct standby_cb_list *ptr;

	list_for_each_entry(ptr, &g_disp_drv.stb_cb_list.list, list) {
		if (ptr->suspend)
			return ptr->suspend();
	}

	return -1;
}

static s32 disp_resume_cb(void)
{
	struct standby_cb_list *ptr;

	list_for_each_entry(ptr, &g_disp_drv.stb_cb_list.list, list) {
		if (ptr->resume)
			return ptr->resume();
	}

	return -1;
}
/**
 * drv_disp_vsync_event - wakeup vsync thread
 * @sel: the index of display manager
 *
 * Get the current time, push it into the cirular queue,
 * and then wakeup the vsync thread.
 */

s32 drv_disp_vsync_event(u32 sel)
{
	unsigned long flags;
	ktime_t now;
	unsigned int head, tail, next;
	bool full = false;
	int cur_line = -1;
	struct disp_device *dispdev = NULL;
	struct disp_manager *mgr = g_disp_drv.mgr[sel];

	if (mgr)
		dispdev = mgr->device;
	if (dispdev) {
		if (dispdev->type == DISP_OUTPUT_TYPE_LCD) {
			struct disp_panel_para panel;

			if (dispdev->get_panel_info) {
				dispdev->get_panel_info(dispdev, &panel);
				cur_line = disp_al_lcd_get_cur_line(
				    dispdev->hwdev_index, &panel);
			}
		}
#if defined(SUPPORT_EDP) && !defined(EDP_SUPPORT_TCON_UPDATE_SAFE_TIME)
		else if (dispdev->type == DISP_OUTPUT_TYPE_EDP) {
			cur_line = -1;
			cur_line = disp_edp_get_cur_line(dispdev);
		}
#endif
		else {
			cur_line =
			    disp_al_device_get_cur_line(dispdev->hwdev_index);
		}
	}

	now = ktime_get();
	spin_lock_irqsave(&g_disp_drv.disp_vsync.slock[sel], flags);
	head = g_disp_drv.disp_vsync.vsync_timestamp_head[sel];
	tail = g_disp_drv.disp_vsync.vsync_timestamp_tail[sel];
	next = tail + 1;
	next = (next >= VSYNC_NUM) ? 0 : next;
	if (next == head)
		full = true;

	if (!full) {
		g_disp_drv.disp_vsync.vsync_timestamp[sel][tail] = now;
		g_disp_drv.disp_vsync.vsync_cur_line[sel][tail] = cur_line;
		g_disp_drv.disp_vsync.vsync_timestamp_tail[sel] = next;
	}
	g_disp_drv.disp_vsync.vsync_read[sel] = true;
	spin_unlock_irqrestore(&g_disp_drv.disp_vsync.slock[sel], flags);

	if (g_disp_drv.disp_vsync.vsync_task[sel])
		wake_up_process(g_disp_drv.disp_vsync.vsync_task[sel]);
	else
		wake_up_interruptible(&g_disp_drv.disp_vsync.vsync_waitq);

	if (full)
		return -1;
	return 0;
}

/**
 * vsync_proc - sends vsync message
 * @disp: the index of display manager
 *
 * Get the timestamp from the circular queue,
 * And send it widthin vsync message to the userland.
 */

static int vsync_proc(u32 disp)
{
	char buf[64];
	char *envp[2];
	unsigned long flags;
	unsigned int head, tail, next;
	ktime_t time;
	s64 ts;
	int cur_line = -1, start_delay = -1;
	struct disp_device *dispdev = NULL;
	struct disp_manager *mgr = g_disp_drv.mgr[disp];
	u32 total_lines = 0;
	u64 period = 0;

	if (mgr)
		dispdev = mgr->device;
	if (dispdev) {
		start_delay = dispdev->timings.start_delay;
		total_lines = dispdev->timings.ver_total_time;
		period = dispdev->timings.frame_period;
	}

	spin_lock_irqsave(&g_disp_drv.disp_vsync.slock[disp], flags);
	head = g_disp_drv.disp_vsync.vsync_timestamp_head[disp];
	tail = g_disp_drv.disp_vsync.vsync_timestamp_tail[disp];
	while (head != tail) {
		time = g_disp_drv.disp_vsync.vsync_timestamp[disp][head];
		cur_line = g_disp_drv.disp_vsync.vsync_cur_line[disp][head];
		next = head + 1;
		next = (next >= VSYNC_NUM) ? 0 : next;
		g_disp_drv.disp_vsync.vsync_timestamp_head[disp] = next;
		spin_unlock_irqrestore(&g_disp_drv.disp_vsync.slock[disp], flags);

		ts = ktime_to_ns(time);
		if ((cur_line >= 0)
			&& (period > 0)
			&& (start_delay >= 0)
			&& (total_lines > 0)
			&& (cur_line != start_delay)) {
			u64 tmp;

			if (cur_line < start_delay) {
				tmp = (start_delay - cur_line) * period;
				do_div(tmp, total_lines);
				ts += tmp;
			} else {
				tmp = (cur_line - start_delay) * period;
				do_div(tmp, total_lines);
				ts -= tmp;
			}
		}
		snprintf(buf, sizeof(buf), "VSYNC%d=%llu", disp, ts);
		envp[0] = buf;
		envp[1] = NULL;
		kobject_uevent_env(&g_disp_drv.dev->kobj, KOBJ_CHANGE, envp);

		spin_lock_irqsave(&g_disp_drv.disp_vsync.slock[disp], flags);
		head = g_disp_drv.disp_vsync.vsync_timestamp_head[disp];
		tail = g_disp_drv.disp_vsync.vsync_timestamp_tail[disp];
	}

	spin_unlock_irqrestore(&g_disp_drv.disp_vsync.slock[disp], flags);

	return 0;
}
unsigned int vsync_poll(struct file *file, poll_table *wait)
{
	unsigned long flags;
	int ret = 0;
	int disp;
	for (disp = 0; disp < DISP_SCREEN_NUM; disp++) {
		spin_lock_irqsave(&g_disp_drv.disp_vsync.slock[disp], flags);
		if (!bsp_disp_vsync_event_is_enabled(disp))
			g_disp_drv.disp_vsync.vsync_read[disp] = false;

		ret |= g_disp_drv.disp_vsync.vsync_read[disp] == true ? POLLIN : 0;
		spin_unlock_irqrestore(&g_disp_drv.disp_vsync.slock[disp], flags);
	}
	if (ret == 0)
		poll_wait(file, &g_disp_drv.disp_vsync.vsync_waitq, wait);
	return ret;
}

int bsp_disp_get_vsync_timestamp(int disp, int64_t *timestamp)
{
	unsigned long flags;
	unsigned int head, tail, next;
	ktime_t time;
	s64 ts;
	int cur_line = -1, start_delay = -1;
	struct disp_device *dispdev = NULL;
	struct disp_manager *mgr = g_disp_drv.mgr[disp];
	u32 total_lines = 0;
	u64 period = 0;
	u64 tmp;

	if (mgr)
		dispdev = mgr->device;
	if (dispdev) {
		start_delay = dispdev->timings.start_delay;
		total_lines = dispdev->timings.ver_total_time;
		period = dispdev->timings.frame_period;
	}

	spin_lock_irqsave(&g_disp_drv.disp_vsync.slock[disp], flags);
	head = g_disp_drv.disp_vsync.vsync_timestamp_head[disp];
	tail = g_disp_drv.disp_vsync.vsync_timestamp_tail[disp];
	if (head == tail) {
		spin_unlock_irqrestore(&g_disp_drv.disp_vsync.slock[disp], flags);
		return -1;
	}

	time = g_disp_drv.disp_vsync.vsync_timestamp[disp][head];
	cur_line = g_disp_drv.disp_vsync.vsync_cur_line[disp][head];
	next = head + 1;
	next = (next >= VSYNC_NUM) ? 0 : next;
	g_disp_drv.disp_vsync.vsync_timestamp_head[disp] = next;
	head = g_disp_drv.disp_vsync.vsync_timestamp_head[disp];
	tail = g_disp_drv.disp_vsync.vsync_timestamp_tail[disp];
	if (head == tail)
		g_disp_drv.disp_vsync.vsync_read[disp] = false;
	spin_unlock_irqrestore(&g_disp_drv.disp_vsync.slock[disp], flags);

	ts = ktime_to_ns(time);
	if ((cur_line >= 0)
		&& (period > 0)
		&& (start_delay >= 0)
		&& (total_lines > 0)
		&& (cur_line != start_delay)) {

		if (cur_line < start_delay) {
			tmp = (start_delay - cur_line) * period;
			do_div(tmp, total_lines);
			ts += tmp;
		} else {
			tmp = (cur_line - start_delay) * period;
			do_div(tmp, total_lines);
			ts -= tmp;
		}
	}
		*timestamp = ts;

	return 0;
}

int vsync_thread(void *parg)
{
	unsigned long disp = (unsigned long)parg;

	while (1) {

		vsync_proc(disp);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		if (kthread_should_stop())
			break;
		set_current_state(TASK_RUNNING);
	}

	return 0;
}

static inline void disp_init_bsp_init_para(struct disp_bsp_init_para *para, const struct disp_drv_info *drv_info)
{
	int i, j;
	unsigned long output_type, output_mode, output_format;
	unsigned long output_bits, output_eotf, output_cs;
	char boot_para[16] = {0};
	char *boot_disp_str = NULL;
	int ret;
	int len;
	char *tmp;
	char *p;
	void *buf[] = {
		&output_type,
		&output_mode,
		&output_format,
		&output_bits,
		&output_cs,
		&output_eotf,
	};

	memset(para, 0, sizeof(struct disp_bsp_init_para));
	/* clk rst */
	for (i = 0; i < DISP_MOD_NUM; i++) {
		para->reg_base[i] = drv_info->reg_base[i];
		para->irq_no[i] = drv_info->irq_no[i];
		DE_INFO("mod %d, base=0x%lx, irq=%d\n", i, para->reg_base[i], para->irq_no[i]);
	}

	for (i = 0; i < DE_NUM; i++) {
		para->clk_de[i] = drv_info->clk_de[i];
		para->clk_bus_de[i] = drv_info->clk_bus_de[i];
		para->rst_bus_de[i] = drv_info->rst_bus_de[i];
		para->rst_bus_de_sys[i] = drv_info->rst_bus_de_sys[i];
	}

#if defined(HAVE_DEVICE_COMMON_MODULE)
	para->clk_bus_extra = drv_info->clk_bus_extra;
	para->rst_bus_extra = drv_info->rst_bus_extra;
#endif
	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		para->clk_bus_dpss_top[i] = drv_info->clk_bus_dpss_top[i];
		para->clk_tcon[i] = drv_info->clk_tcon[i];
		para->clk_bus_tcon[i] = drv_info->clk_bus_tcon[i];
		para->rst_bus_dpss_top[i] = drv_info->rst_bus_dpss_top[i];
		para->rst_bus_tcon[i] = drv_info->rst_bus_tcon[i];
		para->rst_bus_video_out[i] = drv_info->rst_bus_video_out[i];
	}

#if defined(SUPPORT_DSI)
	for (i = 0; i < CLK_DSI_NUM; i++) {
		para->clk_mipi_dsi[i] = g_disp_drv.clk_mipi_dsi[i];
		para->clk_bus_mipi_dsi[i] = g_disp_drv.clk_bus_mipi_dsi[i];
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
		para->clk_mipi_dsi_combphy[i] = g_disp_drv.clk_mipi_dsi_combphy[i];
#endif
	}

	for (i = 0; i < DEVICE_DSI_NUM; i++)
		para->rst_bus_mipi_dsi[i] = g_disp_drv.rst_bus_mipi_dsi[i];
#endif

#if defined(SUPPORT_LVDS)
	for (i = 0; i < DEVICE_LVDS_NUM; i++)
		para->rst_bus_lvds[i] = drv_info->rst_bus_lvds[i];
#endif

	/* callback */
	para->disp_int_process = disp_sync_finish_process;
	para->vsync_event = drv_disp_vsync_event;
	para->start_process = start_process;

	if (g_disp_drv.disp_init.amp_preview[0]) {
		para->preview.amp_preview[0] = g_disp_drv.disp_init.amp_preview[0];
		para->preview.premap_addr = g_disp_drv.disp_init.premap_addr;
		para->preview.src_width = g_disp_drv.disp_init.src_width;
		para->preview.src_height = g_disp_drv.disp_init.src_height;
		para->preview.aux_line_en = g_disp_drv.disp_init.aux_line_en;
		para->preview.screen_x = g_disp_drv.disp_init.screen_x;
		para->preview.screen_y = g_disp_drv.disp_init.screen_y;
		para->preview.screen_w = g_disp_drv.disp_init.screen_w;
		para->preview.screen_h = g_disp_drv.disp_init.screen_h;
		para->preview.format = g_disp_drv.disp_init.preview_format;
	}

	/* smooth boot info */
	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		para->boot_info[i].sync = 0;

		sprintf(boot_para, "boot_disp%d", i);
		boot_disp_str = (char *)disp_boot_para_parse_str(boot_para);
		if (!boot_disp_str) {
			DE_WARN("not found %s\n", boot_para);
			break;
		}

		len = strlen(boot_disp_str);
		tmp = kzalloc(len + 1, GFP_KERNEL);
		if (!tmp) {
			DE_WARN("kzalloc failed\n");
			break;
		}

		memcpy((void *)tmp, (void *)boot_disp_str, len);
		boot_disp_str = tmp;

		j = 0;
		do {
			p = strstr(boot_disp_str, ",");
			if (p != NULL)
				*p = '\0';
			ret = kstrtoul(boot_disp_str, 16, buf[j]);
			if (ret) {
				DE_WARN("parse %s fail! index = %d\n", boot_para, j);
				break;
			}
			j++;
			boot_disp_str = p + 1;
		} while (j < sizeof(buf) / sizeof(buf[0]) && p != NULL);
		if (ret)
			break;

		kfree(tmp);

		if (output_type != (int)DISP_OUTPUT_TYPE_NONE) {
			para->boot_info[i].sync = 1;
			para->boot_info[i].disp = i;
			para->boot_info[i].type = output_type;
			para->boot_info[i].mode = output_mode;
			para->boot_info[i].format = output_format;
			para->boot_info[i].bits = output_bits;
			para->boot_info[i].cs = output_cs;
			para->boot_info[i].eotf = output_eotf;

			para->boot_info[i].dvi_hdmi =
				drv_info->disp_init.output_dvi_hdmi[para->boot_info[i].disp];
			para->boot_info[i].range =
				drv_info->disp_init.output_range[para->boot_info[i].disp];
			para->boot_info[i].scan =
				drv_info->disp_init.output_scan[para->boot_info[i].disp];
			para->boot_info[i].aspect_ratio =
				drv_info->disp_init.output_aspect_ratio[para->boot_info[i].disp];
		}
	}

	/* common */
	para->feat_init.chn_cfg_mode = drv_info->disp_init.chn_cfg_mode;
}

static inline void disp_init_drv_boot_info(const struct disp_bsp_init_para *para, struct disp_init_para *disp_init)
{
	int i;

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		if (para->boot_info[i].sync == 1) {
			DE_WARN("smooth display screen:%d type:%x mode:%x format:%x bits:%x cs:%x eotf:%x\n",
					para->boot_info[i].disp,
					para->boot_info[i].type, para->boot_info[i].mode,
					para->boot_info[i].format, para->boot_info[i].bits,
					para->boot_info[i].cs, para->boot_info[i].eotf);

			disp_init->disp_mode = para->boot_info[i].disp;
			disp_init->output_type[para->boot_info[i].disp] = para->boot_info[i].type;
			disp_init->output_mode[para->boot_info[i].disp] = para->boot_info[i].mode;
			disp_init->output_format[para->boot_info[i].disp] = para->boot_info[i].format;
			disp_init->output_bits[para->boot_info[i].disp] = para->boot_info[i].bits;
			disp_init->output_cs[para->boot_info[i].disp] = para->boot_info[i].cs;
			disp_init->output_eotf[para->boot_info[i].disp] = para->boot_info[i].eotf;
		}
	}
}

static s32 disp_init(struct platform_device *pdev)
{
	int disp, num_screens;

	DE_INFO("\n");

	INIT_WORK(&g_disp_drv.resume_work[0], resume_work_0);
#if DISP_SCREEN_NUM > 1
	INIT_WORK(&g_disp_drv.resume_work[1], resume_work_1);
#endif
	/* INIT_WORK(&g_disp_drv.resume_work[2], resume_work_2); */
	INIT_WORK(&g_disp_drv.start_work, start_work);
	INIT_LIST_HEAD(&g_disp_drv.sync_proc_list.list);
	INIT_LIST_HEAD(&g_disp_drv.sync_finish_proc_list.list);
	INIT_LIST_HEAD(&g_disp_drv.ioctl_extend_list.list);
	INIT_LIST_HEAD(&g_disp_drv.compat_ioctl_extend_list.list);
	INIT_LIST_HEAD(&g_disp_drv.stb_cb_list.list);
	mutex_init(&g_disp_drv.mlock);
	spin_lock_init(&sync_finish_lock);
	parser_disp_init_para(pdev->dev.of_node, &g_disp_drv.disp_init);
	disp_init_bsp_init_para(&g_disp_drv.para, &g_disp_drv);
	disp_init_drv_boot_info(&g_disp_drv.para, &g_disp_drv.disp_init);
	bsp_disp_init(&g_disp_drv.para);

#if IS_ENABLED(CONFIG_AW_DISP2_PQ)
	pq_init(&g_disp_drv.para);
#endif

	/*if (bsp_disp_check_device_enabled(para) == 0)
		para->boot_info.sync = 0;
	*/
	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		g_disp_drv.mgr[disp] = disp_get_layer_manager(disp);
		spin_lock_init(&g_disp_drv.disp_vsync.slock[disp]);
#ifdef VSYNC_USE_UEVENT
		char task_name[25];
		sprintf(task_name, "vsync proc %d", disp);
		g_disp_drv.disp_vsync.vsync_task[disp] =
		    kthread_create(vsync_thread, (void *)(unsigned long)disp, task_name);
		if (IS_ERR(g_disp_drv.disp_vsync.vsync_task[disp])) {
			s32 err = 0;

			DE_WARN("Unable to start kernel thread %s\n",
			      "hdmi proc");
			err = PTR_ERR(g_disp_drv.disp_vsync.vsync_task[disp]);
			g_disp_drv.disp_vsync.vsync_task[disp] = NULL;
		} else {
			wake_up_process(g_disp_drv.disp_vsync.vsync_task[disp]);
		}
#endif
	}
	init_waitqueue_head(&g_disp_drv.disp_vsync.vsync_waitq);

#if defined(SUPPORT_EINK)
	g_disp_drv.eink_manager[0] = disp_get_eink_manager(0);
#endif
	lcd_init();
	bsp_disp_open();
	fb_init(pdev);
#if IS_ENABLED(CONFIG_AW_DISP2_COMPOSER)
	composer_init(&g_disp_drv);
#endif
	g_disp_drv.inited = true;
	start_process();

	DE_INFO("finish\n");
	return 0;
}

static s32 disp_exit(void)
{
	unsigned int i;
	unsigned int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	for (i = 0; i < num_screens; i++) {
		if (g_disp_drv.disp_vsync.vsync_task[i] && !IS_ERR(g_disp_drv.disp_vsync.vsync_task[i])) {
			kthread_stop(g_disp_drv.disp_vsync.vsync_task[i]);
			g_disp_drv.disp_vsync.vsync_task[i] = NULL;
		}
	}

	fb_exit();
	bsp_disp_close();
	bsp_disp_exit(g_disp_drv.exit_mode);
	return 0;
}

static int disp_mem_request(int sel, u32 size)
{

#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	u32 mem_start;
	struct disp_ion_mem *p_ion_mem;
	if (sel >= DISP_MEM_NUM || sel < 0 || !size) {
		DE_WARN("invalid param\n");
		return -EINVAL;
	}

	p_ion_mem = disp_ion_malloc(size, &mem_start);
	mutex_lock(&g_disp_mm[sel].mlock);
	g_disp_mm[sel].p_ion_mem = p_ion_mem;
	g_disp_mm[sel].mem_start = mem_start;
	if (g_disp_mm[sel].p_ion_mem) {
		g_disp_mm[sel].info_base = (char __iomem *)g_disp_mm[sel].p_ion_mem->vaddr;
		g_disp_mm[sel].mem_len = size;
		g_disp_mem_id = sel;
		mutex_unlock(&g_disp_mm[sel].mlock);
		return 0;
	} else {
		mutex_unlock(&g_disp_mm[sel].mlock);
		return -ENOMEM;
	}
#else

#ifndef FB_RESERVED_MEM
	unsigned int map_size = 0;
	struct page *page;

	if ((sel >= DISP_MEM_NUM) ||
	    (g_disp_mm[sel].info_base != NULL)) {
		DE_WARN("invalid param\n");
		return -EINVAL;
	}

	map_size = PAGE_ALIGN(size);
	page = alloc_pages(GFP_KERNEL, get_order(map_size));
	mutex_lock(&g_disp_mm[sel].mlock);
	if (page != NULL) {
		g_disp_mm[sel].info_base = page_address(page);
		if (g_disp_mm[sel].info_base == NULL) {
			free_pages((unsigned long)(page), get_order(map_size));
			DE_WARN("page_address fail\n");
			mutex_unlock(&g_disp_mm[sel].mlock);
			return -ENOMEM;
		}
		g_disp_mm[sel].mem_start =
		    virt_to_phys(g_disp_mm[sel].info_base);
		g_disp_mm[sel].mem_len = size;
		memset(g_disp_mm[sel].info_base, 0, size);

		DE_INFO("pa=0x%p va=0x%p size:0x%x\n",
		      (void *)g_disp_mm[sel].mem_start,
		      g_disp_mm[sel].info_base, size);
		g_disp_mem_id = sel;
		mutex_unlock(&g_disp_mm[sel].mlock);
		return 0;
	}

	DE_WARN("alloc_pages fail\n");
	mutex_unlock(&g_disp_mm[sel].mlock);
	return -ENOMEM;
#else
	uintptr_t phy_addr;
	void *info_base;

	if ((sel >= DISP_MEM_NUM) ||
	    (g_disp_mm[sel].info_base != NULL)) {
		DE_WARN("invalid param\n");
		return -EINVAL;
	}

	info_base = disp_malloc(size, (void *)&phy_addr);
	mutex_lock(&g_disp_mm[sel].mlock);
	g_disp_mm[sel].info_base = info_base;
	if (g_disp_mm[sel].info_base) {
		g_disp_mm[sel].mem_start = phy_addr;
		g_disp_mm[sel].mem_len = size;
		memset(g_disp_mm[sel].info_base, 0, size);
		DE_INFO("pa=0x%p va=0x%p size:0x%x\n",
		      (void *)g_disp_mm[sel].mem_start,
		      g_disp_mm[sel].info_base, size);
		mutex_unlock(&g_disp_mm[sel].mlock);
		g_disp_mem_id = sel;

		return 0;
	}

	DE_WARN("disp_malloc fail\n");
	mutex_unlock(&g_disp_mm[sel].mlock);
	return -ENOMEM;
#endif
#endif
}

static int disp_mem_release(int sel)
{
	if (sel >= DISP_MEM_NUM || sel < 0) {
		DE_WARN("invalid param\n");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	if (!g_disp_mm[sel].info_base) {
		DE_WARN("invalid param\n");
		return -EINVAL;
	}
	disp_ion_free((void *__force)g_disp_mm[sel].info_base,
		      (void *)g_disp_mm[sel].mem_start, g_disp_mm[sel].mem_len);
#else

#ifndef FB_RESERVED_MEM
	unsigned int map_size;
	unsigned int page_size;

	if (g_disp_mm[sel].info_base == NULL) {
		DE_WARN("invalid param\n");
		return -EINVAL;
	}

	map_size = PAGE_ALIGN(g_disp_mm[sel].mem_len);
	page_size = map_size;

	free_pages((unsigned long)(g_disp_mm[sel].info_base),
		   get_order(page_size));
#else
	if (g_disp_mm[sel].info_base == NULL)
		return -EINVAL;

	DE_INFO("disp_mem_release, mem_id=%d, phy_addr=0x%p\n", sel,
	      (void *)g_disp_mm[sel].mem_start);
	disp_free((void *)g_disp_mm[sel].info_base,
		  (void *)g_disp_mm[sel].mem_start, g_disp_mm[sel].mem_len);
#endif
#endif
	mutex_lock(&g_disp_mm[sel].mlock);
	memset(&g_disp_mm[sel], 0, sizeof(struct info_mm));
	mutex_unlock(&g_disp_mm[sel].mlock);
	g_disp_mem_id = -1;
	return 0;
}

int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops)
{
	memset((void *)src_ops, 0, sizeof(struct sunxi_disp_source_ops));

	src_ops->sunxi_lcd_set_panel_funs = bsp_disp_lcd_set_panel_funs;
	src_ops->sunxi_lcd_delay_ms = disp_delay_ms;
	src_ops->sunxi_lcd_delay_us = disp_delay_us;
	src_ops->sunxi_lcd_backlight_enable = bsp_disp_lcd_backlight_enable;
	src_ops->sunxi_lcd_backlight_disable = bsp_disp_lcd_backlight_disable;
	src_ops->sunxi_lcd_pwm_enable = bsp_disp_lcd_pwm_enable;
	src_ops->sunxi_lcd_pwm_disable = bsp_disp_lcd_pwm_disable;
	src_ops->sunxi_lcd_power_enable = bsp_disp_lcd_power_enable;
	src_ops->sunxi_lcd_power_disable = bsp_disp_lcd_power_disable;
	src_ops->sunxi_lcd_tcon_enable = bsp_disp_lcd_tcon_enable;
	src_ops->sunxi_lcd_tcon_disable = bsp_disp_lcd_tcon_disable;
	src_ops->sunxi_lcd_pin_cfg = bsp_disp_lcd_pin_cfg;
	src_ops->sunxi_lcd_gpio_set_value = bsp_disp_lcd_gpio_set_value;
	src_ops->sunxi_lcd_gpio_get_value = bsp_disp_lcd_gpio_get_value;
	src_ops->sunxi_lcd_gpio_set_direction = bsp_disp_lcd_gpio_set_direction;
#ifdef SUPPORT_DSI
	src_ops->sunxi_lcd_dsi_dcs_write = bsp_disp_lcd_dsi_dcs_wr;
	src_ops->sunxi_lcd_dsi_gen_write = bsp_disp_lcd_dsi_gen_wr;
	src_ops->sunxi_lcd_dsi_clk_enable = bsp_disp_lcd_dsi_clk_enable;
	src_ops->sunxi_lcd_dsi_mode_switch = bsp_disp_lcd_dsi_mode_switch;
	src_ops->sunxi_lcd_dsi_gen_short_read = bsp_disp_lcd_dsi_gen_short_read;
	src_ops->sunxi_lcd_dsi_dcs_read = bsp_disp_lcd_dsi_dcs_read;
	src_ops->sunxi_lcd_dsi_set_max_ret_size = bsp_disp_lcd_set_max_ret_size;
#endif
	src_ops->sunxi_lcd_cpu_write = tcon0_cpu_wr_16b;
	src_ops->sunxi_lcd_cpu_write_data = tcon0_cpu_wr_16b_data;
	src_ops->sunxi_lcd_cpu_write_index = tcon0_cpu_wr_16b_index;
	src_ops->sunxi_lcd_cpu_set_auto_mode = tcon0_cpu_set_auto_mode;

	return 0;
}

int sunxi_disp_get_edp_ops(struct sunxi_disp_edp_ops *edp_ops)
{
	memset((void *)edp_ops, 0, sizeof(struct sunxi_disp_edp_ops));

	edp_ops->sunxi_edp_set_panel_funs = bsp_disp_edp_set_panel_funs;
	edp_ops->sunxi_edp_delay_ms = disp_delay_ms;
	edp_ops->sunxi_edp_delay_us = disp_delay_us;
	edp_ops->sunxi_edp_backlight_enable = bsp_disp_edp_backlight_enable;
	edp_ops->sunxi_edp_backlight_disable = bsp_disp_edp_backlight_disable;
	edp_ops->sunxi_edp_pwm_enable = bsp_disp_edp_pwm_enable;
	edp_ops->sunxi_edp_pwm_disable = bsp_disp_edp_pwm_disable;
	edp_ops->sunxi_edp_power_enable = bsp_disp_edp_power_enable;
	edp_ops->sunxi_edp_power_disable = bsp_disp_edp_power_disable;
	edp_ops->sunxi_edp_pin_cfg = bsp_disp_edp_pin_cfg;
	edp_ops->sunxi_edp_gpio_set_value = bsp_disp_edp_gpio_set_value;
	edp_ops->sunxi_edp_gpio_set_direction = bsp_disp_edp_gpio_set_direction;

	return 0;
}
EXPORT_SYMBOL(sunxi_disp_get_edp_ops);

int disp_mmap(struct file *file, struct vm_area_struct *vma)
{

	unsigned int off = vma->vm_pgoff << PAGE_SHIFT;

	int mem_id = g_disp_mem_id;

	if (mem_id >= DISP_MEM_NUM || mem_id < 0 ||
	    !g_disp_mm[mem_id].info_base) {
		DE_WARN("invalid param\n");
		return -EINVAL;
	}

	if (off < g_disp_mm[mem_id].mem_len) {
#if IS_ENABLED(CONFIG_AW_IOMMU) || IS_ENABLED(CONFIG_ARM_SMMU_V3)
		if (g_disp_mm[mem_id].p_ion_mem)
			return g_disp_mm[mem_id].p_ion_mem->p_item->dmabuf->ops->mmap(g_disp_mm[mem_id].p_ion_mem->p_item->dmabuf, vma);
		else
			return -EINVAL;
#else
		return dma_mmap_writecombine(
		    g_disp_drv.dev, vma, g_disp_mm[mem_id].info_base,
		    g_disp_mm[mem_id].mem_start, g_disp_mm[mem_id].mem_len);

#endif
	}

	return -EINVAL;
}

int disp_open(struct inode *inode, struct file *file)
{
	atomic_inc(&g_driver_ref_count);
	return 0;
}

void disp_device_off(void)
{
	int num_screens = 0, i = 0, j = 0;
	struct disp_manager *mgr = NULL;

	memset(lyr_cfg, 0, MAX_LAYERS * sizeof(struct disp_layer_config));

	for (i = 0; i < MAX_LAYERS / 4; ++i) {
		for (j = 0; j < 4; ++j) {
			lyr_cfg[i + j].enable = false;
			lyr_cfg[i + j].channel = i;
			lyr_cfg[i + j].layer_id = j;
		}
	}
	num_screens = bsp_disp_feat_get_num_screens();
	for (i = 0; i < num_screens; ++i) {
		mgr = g_disp_drv.mgr[i];
		if (mgr && mgr->device) {
			if (mgr->device->disable && mgr->device->is_enabled) {
				if (mgr->device->is_enabled(mgr->device)) {
					mgr->set_layer_config(mgr, lyr_cfg, MAX_LAYERS);
					disp_delay_ms(20);
					mgr->device->disable(mgr->device);
				}
			}
		}
	}
}

int disp_release(struct inode *inode, struct file *file)
{
	if (!atomic_dec_and_test(&g_driver_ref_count)) {
		/* There is any other user, just return. */
		return 0;
	}

#if IS_ENABLED(CONFIG_AW_DISP2_DEVICE_OFF_ON_RELEASE)
	disp_device_off();
#endif
	return 0;
}

ssize_t disp_read(struct file *file, char __user *buf, size_t count,
		  loff_t *ppos)
{
	return 0;
}

ssize_t disp_write(struct file *file, const char __user *buf, size_t count,
		   loff_t *ppos)
{
	return 0;
}

static int disp_clk_get_wrap(struct disp_drv_info *disp_drv)
{
	int i;
	char id[32];
	struct device *dev = disp_drv->dev;

	/* get clocks for de */
	for (i = 0; i < DE_NUM; i++) {
		sprintf(id, "clk_de%d", i);
		disp_drv->clk_de[i] = devm_clk_get(dev, id);
		if (IS_ERR(disp_drv->clk_de[i])) {
			disp_drv->clk_de[i] = NULL;
			DE_DEV_ERR(dev, "failed to get clk for %s\n", id);
			return -EINVAL;
		}

		sprintf(id, "clk_bus_de%d", i);
		disp_drv->clk_bus_de[i] = devm_clk_get(dev, id);
		if (IS_ERR(disp_drv->clk_bus_de[i])) {
			disp_drv->clk_bus_de[i] = NULL;
			DE_DEV_ERR(dev, "failed to get clk for %s\n", id);
			return -EINVAL;
		}
	}

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
#if defined(HAVE_DEVICE_COMMON_MODULE)
		/* get clocks for dpss */
		sprintf(id, "clk_bus_dpss_top%d", i);
		disp_drv->clk_bus_dpss_top[i] = devm_clk_get(dev, id);
		if (IS_ERR(disp_drv->clk_bus_dpss_top[i])) {
			disp_drv->clk_bus_dpss_top[i] = NULL;
			DE_DEV_ERR(dev, "failed to get clk for %s\n", id);
		}

#endif
		/* get clocks for tcon */
		sprintf(id, "clk_tcon%d", i);
		disp_drv->clk_tcon[i] = devm_clk_get(dev, id);
		if (IS_ERR(disp_drv->clk_tcon[i])) {
			disp_drv->clk_tcon[i] = NULL;
			DE_DEV_ERR(dev, "failed to get clk for %s\n", id);
		}

		sprintf(id, "clk_bus_tcon%d", i);
		disp_drv->clk_bus_tcon[i] = devm_clk_get(dev, id);
		if (IS_ERR(disp_drv->clk_bus_tcon[i])) {
			disp_drv->clk_bus_tcon[i] = NULL;
			DE_DEV_ERR(dev, "failed to get clk for %s\n", id);
		}
	}

#if defined(SUPPORT_DSI)
	for (i = 0; i < CLK_DSI_NUM; i++) {
		sprintf(id, "clk_mipi_dsi%d", i);
		disp_drv->clk_mipi_dsi[i] = devm_clk_get(dev, id);
		if (IS_ERR(disp_drv->clk_mipi_dsi[i])) {
			disp_drv->clk_mipi_dsi[i] = NULL;
			DE_DEV_ERR(dev, "failed to get clk for %s\n", id);
			return -EINVAL;
		}

		sprintf(id, "clk_bus_mipi_dsi%d", i);
		disp_drv->clk_bus_mipi_dsi[i] = devm_clk_get(dev, id);
		if (IS_ERR(disp_drv->clk_bus_mipi_dsi[i])) {
			disp_drv->clk_bus_mipi_dsi[i] = NULL;
			DE_DEV_ERR(dev, "failed to get clk for %s\n", id);
			return -EINVAL;
		}

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
		sprintf(id, "clk_mipi_dsi_combphy%d", i);
		disp_drv->clk_mipi_dsi_combphy[i] = devm_clk_get(dev, id);
		if (IS_ERR(disp_drv->clk_mipi_dsi_combphy[i])) {
			disp_drv->clk_mipi_dsi_combphy[i] = NULL;
			DE_DEV_ERR(dev, "failed to get clk for %s\n", id);
			return -EINVAL;
		}
#endif
	}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		disp_drv->clk_bus_extra = devm_clk_get(dev, "clk_pll_com");
		if (IS_ERR(disp_drv->clk_bus_extra)) {
			disp_drv->clk_bus_extra = NULL;
			DE_DEV_ERR(dev, "failed to get clk for display top\n");
			return -EINVAL;
		}
#endif
	return 0;
}

static void disp_clk_put_wrap(struct disp_drv_info *disp_drv)
{
	int i;
	struct device *dev = disp_drv->dev;

	/* put clocks for de */
	for (i = 0; i < DE_NUM; i++) {
		devm_clk_put(dev, disp_drv->clk_de[i]);
		devm_clk_put(dev, disp_drv->clk_bus_de[i]);
	}

#if defined(HAVE_DEVICE_COMMON_MODULE)
	devm_clk_put(dev, disp_drv->clk_bus_extra);
#endif

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		/* put clocks for dpss */
		devm_clk_put(dev, disp_drv->clk_bus_dpss_top[i]);

		/* put clocks for tcon */
		devm_clk_put(dev, disp_drv->clk_tcon[i]);
		devm_clk_put(dev, disp_drv->clk_bus_tcon[i]);
	}

#if defined(SUPPORT_DSI)
	/* put clocks for dsi */
	for (i = 0; i < CLK_DSI_NUM; i++) {
		devm_clk_put(dev, disp_drv->clk_mipi_dsi[i]);
		devm_clk_put(dev, disp_drv->clk_bus_mipi_dsi[i]);
	}
#endif
}

static int disp_reset_control_get_wrap(struct disp_drv_info *disp_drv)
{
	int i;
	char id[32];
	struct device *dev = disp_drv->dev;
	for (i = 0; i < DE_NUM; i++) {
		/* get resets for de */
		sprintf(id, "rst_bus_de%d", i);
		disp_drv->rst_bus_de[i] = devm_reset_control_get_shared(dev, id);
		if (IS_ERR(disp_drv->rst_bus_de[i])) {
			disp_drv->rst_bus_de[i] = NULL;
			DE_DEV_ERR(dev, "failed to get reset for %s\n", id);
			return -EINVAL;
		}
#if IS_ENABLED(CONFIG_ARCH_SUN60IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		sprintf(id, "rst_bus_de_sys%d", i);
		disp_drv->rst_bus_de_sys[i] = devm_reset_control_get_shared(dev, id);
		if (IS_ERR(disp_drv->rst_bus_de_sys[i])) {
			disp_drv->rst_bus_de_sys[i] = NULL;
			DE_DEV_ERR(dev, "failed to get reset for %s\n", id);
			return -EINVAL;
		}
#endif
	}

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		/* get resets for dpss */
#if defined(HAVE_DEVICE_COMMON_MODULE)
		sprintf(id, "rst_bus_dpss_top%d", i);
		disp_drv->rst_bus_dpss_top[i] = devm_reset_control_get_shared(dev, id);
		if (IS_ERR(disp_drv->rst_bus_dpss_top[i])) {
			disp_drv->rst_bus_dpss_top[i] = NULL;
			DE_DEV_ERR(dev, "failed to get reset for %s\n", id);
			return -EINVAL;
		}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN60IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		sprintf(id, "rst_bus_video_out%d", i);
		disp_drv->rst_bus_video_out[i] = devm_reset_control_get_shared(dev, id);
		if (IS_ERR(disp_drv->rst_bus_dpss_top[i])) {
			disp_drv->rst_bus_dpss_top[i] = NULL;
			DE_DEV_ERR(dev, "failed to get reset for %s\n", id);
			return -EINVAL;
		}
#endif

		/* get resets for tcon */
		sprintf(id, "rst_bus_tcon%d", i);
		disp_drv->rst_bus_tcon[i] = devm_reset_control_get_shared(dev, id);
		if (IS_ERR(disp_drv->rst_bus_tcon[i])) {
			disp_drv->rst_bus_tcon[i] = NULL;
			DE_DEV_ERR(dev, "failed to get reset for %s\n", id);
			return -EINVAL;
		}
	}
#if defined(SUPPORT_DSI)
	for (i = 0; i < DEVICE_DSI_NUM; i++) {
		sprintf(id, "rst_bus_mipi_dsi%d", i);
		disp_drv->rst_bus_mipi_dsi[i] = devm_reset_control_get(dev, id);
		if (IS_ERR(disp_drv->rst_bus_mipi_dsi[i])) {
			disp_drv->rst_bus_mipi_dsi[i] = NULL;
			DE_DEV_ERR(dev, "failed to get reset for %s\n", id);
			return -EINVAL;
		}
	}
#endif

#if defined(SUPPORT_LVDS)
	/* get resets for lvds */
	for (i = 0; i < DEVICE_LVDS_NUM; i++) {
		sprintf(id, "rst_bus_lvds%d", i);
		disp_drv->rst_bus_lvds[i] =
			devm_reset_control_get_shared(dev, id);
		if (IS_ERR(disp_drv->rst_bus_lvds[i])) {
			disp_drv->rst_bus_lvds[i] = NULL;
			DE_DEV_ERR(dev, "failed to get reset for %s\n", id);
			return -EINVAL;
		}
	}
#endif
	return 0;
}

static void disp_reset_control_put_wrap(struct disp_drv_info *disp_drv)
{
	int i;

	/* put resets for de */
	for (i = 0; i < DE_NUM; i++)
		reset_control_put(disp_drv->rst_bus_de[i]);

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
#if defined(HAVE_DEVICE_COMMON_MODULE)
		/* put resets for dpss */
		reset_control_put(disp_drv->rst_bus_dpss_top[i]);
#endif

		/* put resets for tcon */
		reset_control_put(disp_drv->rst_bus_tcon[i]);
	}

#if defined(SUPPORT_LVDS)
	for (i = 0; i < DEVICE_LVDS_NUM; i++) {
		/* put resets for lvds */
		reset_control_put(disp_drv->rst_bus_lvds[i]);
	}
#endif
}

#if IS_ENABLED(CONFIG_AW_IOMMU)
static void iommu_handle_func(struct work_struct *work);
static void iommu_error_cb(void);
struct work_struct iommu_wq;
#endif

static int disp_attach_pd(struct device *dev)
{
	struct device_link *link;
	struct device *pd_dev;

	/* Do nothing when in a single power domain */
	if (dev->pm_domain)
		return 0;

	pd_dev = dev_pm_domain_attach_by_name(dev, "pd_de");
	if (IS_ERR(pd_dev))
		return PTR_ERR(pd_dev);
	/* Do nothing when power domain missing */
	if (!pd_dev)
		return 0;
	link = device_link_add(dev, pd_dev,
						   DL_FLAG_STATELESS |
						   DL_FLAG_PM_RUNTIME |
						   DL_FLAG_RPM_ACTIVE);
	if (!link) {
		DE_DEV_ERR(dev, "Failed to add device_link to pd_de\n");
		return -EINVAL;
	}

	pd_dev = dev_pm_domain_attach_by_name(dev, "pd_vo0");
	if (IS_ERR(pd_dev))
		return PTR_ERR(pd_dev);
	/* Do nothing when power domain missing */
	if (!pd_dev)
		return 0;
	link = device_link_add(dev, pd_dev,
						   DL_FLAG_STATELESS |
						   DL_FLAG_PM_RUNTIME |
						   DL_FLAG_RPM_ACTIVE);
	if (!link) {
		DE_DEV_ERR(dev, "Failed to add device_link to pd_vo0\n");
		return -EINVAL;
	}

	pd_dev = dev_pm_domain_attach_by_name(dev, "pd_vo1");
	if (IS_ERR(pd_dev))
		return PTR_ERR(pd_dev);
	if (!pd_dev)
		return 0;
	link = device_link_add(dev, pd_dev,
						   DL_FLAG_STATELESS |
						   DL_FLAG_PM_RUNTIME |
						   DL_FLAG_RPM_ACTIVE);
	if (!link) {
		DE_DEV_ERR(dev, "Failed to add device_link to pd_vo1\n");
		return -EINVAL;
	}

	return 0;
}

static u64 disp_dmamask = DMA_BIT_MASK(32);
static int disp_probe(struct platform_device *pdev)
{
	int i;
	int ret;
	int counter = 0;

#if IS_ENABLED(CONFIG_AW_IOMMU)
#if defined(IOMMU_DE0_MASTOR_ID)
	sunxi_iommu_register_fault_cb(iommu_error_cb, IOMMU_DE0_MASTOR_ID);
#endif
#if defined(IOMMU_DE1_MASTOR_ID)
	sunxi_iommu_register_fault_cb(iommu_error_cb, IOMMU_DE1_MASTOR_ID);
#endif
	INIT_WORK(&iommu_wq, iommu_handle_func);
#endif
	if (g_disp_drv.inited) {
		DE_WARN("disp has probed\n");
		return 0;
	}
	DE_INFO("disp_probe\n");
	memset(&g_disp_drv, 0, sizeof(struct disp_drv_info));

#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)\
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
	/* set ve to normal mode */
	writel((readl(ioremap(0x03000004, 4)) & 0xfeffffff),
	       ioremap(0x03000004, 4));
#endif

	g_disp_drv.dev = &pdev->dev;
	pdev->dev.dma_mask = &disp_dmamask;

	/* iomap */
	/* de - [device(tcon-top)] - lcd0/1/2.. - dsi */
	counter = 0;
	g_disp_drv.reg_base[DISP_MOD_DE] =
	    (uintptr_t __force) of_iomap(pdev->dev.of_node, counter);
	if (!g_disp_drv.reg_base[DISP_MOD_DE]) {
		DE_DEV_ERR(&pdev->dev, "unable to map de registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
	counter++;

#if defined(SUPPORT_INDEPENDENT_DE)
	g_disp_drv.reg_base[DISP_MOD_DE1] =
	    (uintptr_t __force) of_iomap(pdev->dev.of_node, counter);
	if (!g_disp_drv.reg_base[DISP_MOD_DE1]) {
		DE_DEV_ERR(&pdev->dev, "unable to map de registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
	counter++;
#endif

#if defined(HAVE_DEVICE_COMMON_MODULE)
	g_disp_drv.reg_base[DISP_MOD_DEVICE] =
	    (uintptr_t __force) of_iomap(pdev->dev.of_node, counter);
	if (!g_disp_drv.reg_base[DISP_MOD_DEVICE]) {
		DE_DEV_ERR(&pdev->dev,
			"unable to map device common module registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
	counter++;
#if defined(SUPPORT_INDEPENDENT_DE) || defined(SUPPORT_INDEPENDENT_DEVICE_COMMON)
	g_disp_drv.reg_base[DISP_MOD_DEVICE1] =
	    (uintptr_t __force) of_iomap(pdev->dev.of_node, counter);
	if (!g_disp_drv.reg_base[DISP_MOD_DEVICE1]) {
		DE_DEV_ERR(&pdev->dev,
			"unable to map device common module registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
	counter++;
#endif
#endif

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		g_disp_drv.reg_base[DISP_MOD_LCD0 + i] =
		    (uintptr_t __force) of_iomap(pdev->dev.of_node, counter);
		if (!g_disp_drv.reg_base[DISP_MOD_LCD0 + i]) {
			DE_DEV_ERR(&pdev->dev,
				"unable to map timing controller %d registers\n",
				i);
			ret = -EINVAL;
			goto err_iomap;
		}
		counter++;
	}

#if defined(SUPPORT_DSI)
	for (i = 0; i < DEVICE_DSI_NUM; ++i) {
		g_disp_drv.reg_base[DISP_MOD_DSI0 + i] = (uintptr_t __force)
			of_iomap(pdev->dev.of_node, counter);
		if (!g_disp_drv.reg_base[DISP_MOD_DSI0 + i]) {
			DE_DEV_ERR(&pdev->dev, "unable to map dsi registers\n");
			ret = -EINVAL;
			goto err_iomap;
		}
		counter++;
	}
#endif

#if defined(SUPPORT_EINK)
	g_disp_drv.reg_base[DISP_MOD_EINK] =
		   (uintptr_t __force)of_iomap(pdev->dev.of_node, counter);
	if (!g_disp_drv.reg_base[DISP_MOD_EINK]) {
		DE_DEV_ERR(&pdev->dev, "unable to map eink registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
	counter++;
#endif

	/* parse and map irq */
	/* lcd0/1/2.. - dsi */
	/* get de irq for rcq update and eink */
	counter = 0;

#if defined(DE_VERSION_V33X) || defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	g_disp_drv.irq_no[DISP_MOD_DE] =
	    irq_of_parse_and_map(pdev->dev.of_node, counter);
	if (!g_disp_drv.irq_no[DISP_MOD_DE]) {
		DE_DEV_ERR(&pdev->dev, "irq_of_parse_and_map de irq fail\n");
	}
	++counter;

#if defined(SUPPORT_CRC)
	g_disp_drv.irq_no[DISP_MOD_DE_FREEZE] =
		  irq_of_parse_and_map(pdev->dev.of_node, counter);
	if (!g_disp_drv.irq_no[DISP_MOD_DE]) {
		DE_DEV_ERR(&pdev->dev, "irq_of_parse_and_map de freeze irq fail\n");
	}
	++counter;
#endif

#endif

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		g_disp_drv.irq_no[DISP_MOD_LCD0 + i] =
		    irq_of_parse_and_map(pdev->dev.of_node, counter);
		if (!g_disp_drv.irq_no[DISP_MOD_LCD0 + i])
			DE_DEV_ERR(&pdev->dev,
				"get irq %d fail for timing controller%d\n",
				counter, i);

		counter++;
	}
#if defined(SUPPORT_DSI)
	for (i = 0; i < DEVICE_DSI_NUM; ++i) {
		g_disp_drv.irq_no[DISP_MOD_DSI0 + i] = irq_of_parse_and_map(
						 pdev->dev.of_node, counter);
		if (!g_disp_drv.irq_no[DISP_MOD_DSI0 + i])
			DE_DEV_ERR(&pdev->dev,
				"irq_of_parse_and_map irq %d fail for dsi\n",
				i);
		counter++;
	}
#endif

#if defined(SUPPORT_VDPO)
	g_disp_drv.irq_no[DISP_MOD_VDPO] =
	    irq_of_parse_and_map(pdev->dev.of_node, counter);
	if (!g_disp_drv.irq_no[DISP_MOD_DSI0])
		DE_DEV_ERR(&pdev->dev,
			"irq_of_parse_and_map irq fail for vdpo\n");
	++counter;
#endif /* endif SUPPORT_VDPO */

#if defined(SUPPORT_EINK)
	g_disp_drv.irq_no[DISP_MOD_EINK] =
	    irq_of_parse_and_map(pdev->dev.of_node, counter);
	if (!g_disp_drv.irq_no[DISP_MOD_EINK])
		DE_DEV_ERR(&pdev->dev,
			"irq_of_parse_and_map eink irq %d fail for ee\n", i);
	counter++;
#endif

	ret = disp_clk_get_wrap(&g_disp_drv);
	if (ret)
		goto out_dispose_mapping;

	ret = disp_reset_control_get_wrap(&g_disp_drv);
	if (ret)
		goto out_dispose_mapping;

#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	init_disp_ion_mgr(&g_disp_drv.ion_mgr);
#endif

	disp_init(pdev);
	ret = sysfs_create_group(&display_dev->kobj, &disp_attribute_group);
	if (ret)
		DE_ERR("sysfs_create_group fail\n");

	disp_attach_pd(&pdev->dev);
#if defined(SUPPORT_PM_RUNTIME)
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_get_noresume(&pdev->dev);
	/* pm_runtime_set_autosuspend_delay(&pdev->dev, 5000); */
	/* pm_runtime_use_autosuspend(&pdev->dev); */
	pm_runtime_enable(&pdev->dev);
#endif
	device_enable_async_suspend(&pdev->dev);

	atomic_set(&g_driver_ref_count, 0);

	DE_INFO("disp_probe finish\n");

	return ret;

out_dispose_mapping:
	for (i = 0; i < DISP_DEVICE_NUM; i++)
		irq_dispose_mapping(g_disp_drv.irq_no[i]);
err_iomap:
	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		if (g_disp_drv.reg_base[i])
			iounmap((char __iomem *)g_disp_drv.reg_base[i]);
	}

	return ret;
}

static int disp_remove(struct platform_device *pdev)
{
	int i;

	DE_INFO("disp_remove call\n");

	disp_shutdown(pdev);
#if defined(SUPPORT_PM_RUNTIME)
	pm_runtime_set_suspended(&pdev->dev);
	/* pm_runtime_dont_use_autosuspend(&pdev->dev); */
	pm_runtime_disable(&pdev->dev);
#endif
	disp_exit();

#if IS_ENABLED(CONFIG_DMABUF_HEAPS)
	deinit_disp_ion_mgr(&g_disp_drv.ion_mgr);
#endif

	sysfs_remove_group(&display_dev->kobj, &disp_attribute_group);

	disp_clk_put_wrap(&g_disp_drv);

	disp_reset_control_put_wrap(&g_disp_drv);

	for (i = 0; i < DISP_MOD_NUM; i++) {
		irq_dispose_mapping(g_disp_drv.irq_no[i]);
		if (g_disp_drv.reg_base[i])
			iounmap((char __iomem *)g_disp_drv.reg_base[i]);
	}

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static int disp_blank(bool blank)
{
	u32 screen_id = 0;
	int num_screens;
	struct disp_manager *mgr = NULL;
	struct fb_event event;
	int disp_blank;
	int ret = -EINVAL;

	if (blank)
		disp_blank = FB_BLANK_POWERDOWN;
	else
		disp_blank = FB_BLANK_UNBLANK;

	event.info = fb_infos[0];
	event.data = &disp_blank;

#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY)
	/* notify dramfreq module that DE will access DRAM in a short time */
	if (!blank)
		dramfreq_master_access(MASTER_DE, true);
#endif
	num_screens = bsp_disp_feat_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = g_disp_drv.mgr[screen_id];
		/* Currently remove !mgr->device condition,
		 * avoiding problem in the following case:
		 *
		 *   attach manager and device -> disp blank --> blank success
		 *   deattach manager and device -> disp unblank --> fail
		 *   (cause don't satisfy !mgr->device condition)
		 *   attach manager and device --> problem arises
		 *   (manager will be always on unblank state)
		 *
		 * The scenario is: hdmi plug in -> enter standy
		 *  -> hdmi plug out -> exit standby -> hdmi plug in
		 *  -> display blank on hdmi screen
		 */
		if (!mgr)
			continue;

		if (mgr->blank)
			ret = mgr->blank(mgr, blank);
	}

	if (!ret)
		fb_notifier_call_chain(FB_EVENT_BLANK, &event);

#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY)
	/* notify dramfreq module that DE will not access DRAM any more */
	if (blank)
		dramfreq_master_access(MASTER_DE, false);
#endif

	return 0;
}

#if defined(SUPPORT_PM_RUNTIME)
static int disp_runtime_suspend(struct device *dev)
{
	u32 screen_id = 0;
	int num_screens;
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev_suspend = NULL;
	struct list_head *disp_list = NULL;

	DE_INFO("\n");

	if (!g_pm_runtime_enable)
		return 0;

	num_screens = bsp_disp_feat_get_num_screens();

	disp_suspend_cb();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = g_disp_drv.mgr[screen_id];
		if (mgr && mgr->device) {
			struct disp_device *dispdev = mgr->device;

			if (suspend_output_type[screen_id] ==
			    DISP_OUTPUT_TYPE_LCD)
				flush_work(&g_disp_drv.resume_work[screen_id]);

			if (dispdev && dispdev->is_enabled && dispdev->is_enabled(dispdev))
				dispdev->disable(dispdev);
		}
	}

	disp_list = disp_device_get_list_head();
	list_for_each_entry(dispdev_suspend, disp_list, list) {
		if (dispdev_suspend && dispdev_suspend->suspend)
			dispdev_suspend->suspend(dispdev_suspend);
	}

	suspend_status |= DISPLAY_LIGHT_SLEEP;
	suspend_prestep = 0;

	DE_INFO("finish\n");

	return 0;
}

static int disp_runtime_resume(struct device *dev)
{
	u32 screen_id = 0;
	int num_screens;
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;
	struct list_head *disp_list = NULL;
	struct disp_device_config config;

	DE_INFO("\n");

	if (!g_pm_runtime_enable)
		return 0;

	memset(&config, 0, sizeof(struct disp_device_config));
	num_screens = bsp_disp_feat_get_num_screens();

	disp_list = disp_device_get_list_head();
	list_for_each_entry(dispdev, disp_list, list) {
		if (dispdev->resume)
			dispdev->resume(dispdev);
	}

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = g_disp_drv.mgr[screen_id];
		if (!mgr || !mgr->device)
			continue;

		if (suspend_output_type[screen_id] == DISP_OUTPUT_TYPE_LCD) {
			flush_work(&g_disp_drv.resume_work[screen_id]);
			if (!mgr->device->is_enabled(mgr->device)) {
				mgr->device->enable(mgr->device);
			} else {
				mgr->device->pwm_enable(mgr->device);
				mgr->device->backlight_enable(mgr->device);
			}
		} else if (suspend_output_type[screen_id] !=
							DISP_OUTPUT_TYPE_NONE) {
			if (mgr->device->set_static_config &&
					mgr->device->get_static_config) {
				mgr->device->get_static_config(mgr->device,
								&config);

				mgr->device->set_static_config(mgr->device,
								&config);
			}
			if (!mgr->device->is_enabled(mgr->device))
				mgr->device->enable(mgr->device);
		}
	}

	suspend_status &= (~DISPLAY_LIGHT_SLEEP);
	suspend_prestep = 3;

	disp_resume_cb();

	DE_INFO("finish\n");

	return 0;
}

static int disp_runtime_idle(struct device *dev)
{
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	if (g_disp_drv.dev)
		DE_INFO("\n");
	else
		DE_WARN("display device is null\n");

	/* return 0: for framework to request enter suspend.
	 * return non-zero: do susupend for myself;
	 */
	return 0;
}
#endif

int disp_suspend(struct device *dev)
{
	u32 screen_id = 0;
	bool runtime_suspended = false;
	int num_screens;
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev_suspend = NULL;
	struct list_head *disp_list = NULL;
	struct disp_device *dispdev = NULL;

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
	struct disp_eink_manager *eink_manager = NULL;

	eink_manager = g_disp_drv.eink_manager[0];
	if (!eink_manager)
		DE_WARN("eink_manager is NULL\n");
#endif

	if (!g_disp_drv.dev) {
		DE_WARN("display device is null\n");
		return 0;
	}
#if defined(SUPPORT_PM_RUNTIME)
	runtime_suspended = pm_runtime_suspended(g_disp_drv.dev);
#endif

	if (g_pm_runtime_enable && runtime_suspended) {
		DE_INFO("suspend operation has been done in runtime_suspend\n");
	} else {
		num_screens = bsp_disp_feat_get_num_screens();
		disp_suspend_cb();

		DE_INFO("\n");
		for (screen_id = 0; screen_id < num_screens;
		     screen_id++) {
			mgr = g_disp_drv.mgr[screen_id];
			if (!mgr || !mgr->device)
				continue;
			dispdev = mgr->device;
			if (suspend_output_type[screen_id] ==
			    DISP_OUTPUT_TYPE_LCD)
				flush_work(&g_disp_drv.
					   resume_work[screen_id]);
			if (suspend_output_type[screen_id] !=
			    DISP_OUTPUT_TYPE_NONE) {
				if (dispdev->is_enabled(dispdev))
					dispdev->disable(dispdev);
			}
		}

		/* suspend for all display device */
		disp_list = disp_device_get_list_head();
		list_for_each_entry(dispdev_suspend, disp_list, list) {
			if (dispdev_suspend->suspend)
				dispdev_suspend->suspend(dispdev_suspend);
		}
	}
	/* FIXME: hdmi suspend */
	suspend_status |= DISPLAY_DEEP_SLEEP;
	suspend_prestep = 1;
	DE_INFO("finish\n");

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
	eink_manager->suspend(eink_manager);
#endif
	return 0;
}

int disp_resume(struct device *dev)
{
	u32 screen_id = 0;
	bool runtime_suspended = false;
	int num_screens = bsp_disp_feat_get_num_screens();
	struct disp_manager *mgr = NULL;
	struct disp_device_config config;

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
	struct disp_eink_manager *eink_manager = NULL;
#endif

#if defined(SUPPORT_PM_RUNTIME)
	runtime_suspended = pm_runtime_suspended(g_disp_drv.dev);
#endif
	memset(&config, 0, sizeof(struct disp_device_config));
	if (g_pm_runtime_enable && runtime_suspended) {
		DE_INFO("resume operation is put into runtime_resume\n");
	} else {
		struct disp_device *dispdev = NULL;
		struct list_head *disp_list = NULL;

		DE_INFO("\n");
		disp_list = disp_device_get_list_head();
		list_for_each_entry(dispdev, disp_list, list) {
			if (dispdev->resume)
				dispdev->resume(dispdev);
		}
		for (screen_id = 0; screen_id < num_screens; screen_id++) {
			mgr = g_disp_drv.mgr[screen_id];
			if (!mgr || !mgr->device)
				continue;

			if (suspend_output_type[screen_id] !=
						DISP_OUTPUT_TYPE_NONE) {
				if (mgr->device->set_static_config
					&& mgr->device->get_static_config) {
					mgr->device->get_static_config(mgr->device, &config);
					mgr->device->set_static_config(mgr->device, &config);
				}
				if (!mgr->device->is_enabled(mgr->device))
					mgr->device->enable(mgr->device);
			}
		}
		disp_resume_cb();
	}

	suspend_status &= (~DISPLAY_DEEP_SLEEP);
	suspend_prestep = 2;

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
	eink_manager = g_disp_drv.eink_manager[0];
	if (!eink_manager)
		DE_WARN("eink_manager is NULL\n");
	eink_manager->resume(eink_manager);
#endif
	DE_INFO("finish\n");

	return 0;
}

#if IS_ENABLED(CONFIG_AW_IOMMU)
static void iommu_handle_func(struct work_struct *work)
{

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
    unsigned int ic_ver = sunxi_get_soc_ver();
    if (ic_ver >= 2) {
	    /*bring up wb power*/
	    writel(0x11, ioremap(0x05008000, 4));
	    /*soft reset de, reused with wb*/
	    writel(0x10, ioremap(0x05011000, 4));
	    /*soft reset release*/
	    writel(0x0, ioremap(0x05011000, 4));
	    /*bring up rcq update*/
	    writel(0x1, ioremap(0x05008110, 4));
    }
#endif

#ifdef PRINT_SYS_IN_IOMMU_ERR
/*	DE_WARN("in work\n");
#if defined(SUPPORT_PM_RUNTIME)
	disp_runtime_suspend(g_disp_drv.dev);
	disp_runtime_resume(g_disp_drv.dev);
#endif
	DE_WARN("work =%px\n", work);*/
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;
	ssize_t count = 0;
	int num_screens, screen_id;
	int num_layers, layer_id;
	int num_chans, chan_id;
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
	struct disp_lcd_esd_info esd_inf;

	memset(&esd_inf, 0, sizeof(struct disp_lcd_esd_info));
#endif
	/* int hpd; */
	const int MAXSIZE = 4096;
    char *buf = (char *)kmalloc(MAXSIZE, GFP_KERNEL | __GFP_ZERO);


	num_screens = bsp_disp_feat_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		u32 width = 0, height = 0;
		int fps = 0;
		struct disp_health_info info;

		mgr = disp_get_layer_manager(screen_id);
		if (mgr == NULL)
			continue;
		dispdev = mgr->device;
		if (dispdev == NULL)
			continue;
		dispdev->get_resolution(dispdev, &width, &height);
		fps = bsp_disp_get_fps(screen_id);
		bsp_disp_get_health_info(screen_id, &info);

		if (!dispdev->is_enabled(dispdev))
			continue;
		count += sprintf(buf + count, "screen %d:\n", screen_id);
		count += sprintf(buf + count, "de_rate %d hz, ref_fps:%d\n",
				 mgr->get_clk_rate(mgr),
				 dispdev->get_fps(dispdev));
		count += mgr->dump(mgr, buf + count);
		/* output */
		if (dispdev->type == DISP_OUTPUT_TYPE_LCD) {
			count += sprintf(buf + count,
				"\tlcd output\tbacklight(%3d)\tfps:%d.%d",
				dispdev->get_bright(dispdev), fps / 10,
				fps % 10);
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
			if (dispdev->get_esd_info) {
				dispdev->get_esd_info(dispdev, &esd_inf);
				count += sprintf(buf + count,
				"\tesd level(%u)\tfreq(%u)\tpos(%u)\treset(%u)",
				esd_inf.level, esd_inf.freq,
				esd_inf.esd_check_func_pos, esd_inf.rst_cnt);
			}
#endif
		} else if (dispdev->type == DISP_OUTPUT_TYPE_HDMI) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\thdmi output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		} else if (dispdev->type == DISP_OUTPUT_TYPE_TV) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\ttv output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		} else if (dispdev->type == DISP_OUTPUT_TYPE_VGA) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\tvga output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		}  else if (dispdev->type == DISP_OUTPUT_TYPE_VDPO) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\tvdpo output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		}  else if (dispdev->type == DISP_OUTPUT_TYPE_RTWB) {
			int mode = dispdev->get_mode(dispdev);

			count += sprintf(buf + count,
					 "\trtwb output mode(%d)\tfps:%d.%d",
					 mode, fps / 10, fps % 10);
		} else if (dispdev->type == DISP_OUTPUT_TYPE_EDP) {
			count += sprintf(
			    buf + count, "\tEDP output(%s) \tfps:%d.%d",
			    (dispdev->is_enabled(dispdev) == 1) ? "enable"
								: "disable",
			    fps / 10, fps % 10);
		}
		if (dispdev->type != DISP_OUTPUT_TYPE_NONE) {
			count += sprintf(buf + count, "\t%4ux%4u\n",
					 width, height);
			count += sprintf(buf + count,
					"\terr:%u\tskip:%u\tirq:%llu\tvsync:%u\tvsync_skip:%u\t\n",
					info.error_cnt, info.skip_cnt,
					info.irq_cnt, info.vsync_cnt,
					info.vsync_skip_cnt);
		}

		num_chans = bsp_disp_feat_get_num_channels(screen_id);

		/* layer info */
		for (chan_id = 0; chan_id < num_chans; chan_id++) {
			num_layers =
			    bsp_disp_feat_get_num_layers_by_chn(screen_id,
								chan_id);
			for (layer_id = 0; layer_id < num_layers; layer_id++) {
				struct disp_layer *lyr = NULL;
				struct disp_layer_config config;

				lyr = disp_get_layer(screen_id, chan_id,
						     layer_id);
				config.channel = chan_id;
				config.layer_id = layer_id;
				mgr->get_layer_config(mgr, &config, 1);
				if (lyr && (true == config.enable) && lyr->dump)
					count += lyr->dump(lyr, buf + count);
			}
		}
	}
#if IS_ENABLED(CONFIG_AW_DISP2_COMPOSER)
	count += hwc_dump(buf + count);
#endif
	if (MAXSIZE <= count) {
		//DE_WARN("count = %d outof size\n", (int)count);
		count = MAXSIZE - 1;
	}
	buf[count - 1] = '\n';
	DE_WARN("%s\n", buf);
#endif
}

static void iommu_error_cb(void)
{
	int num_screens = 0;
	int screen_id = 0;
	struct disp_manager *mgr = NULL;
	DE_WARN("in iommu error\n");
	schedule_work(&iommu_wq);
	num_screens = bsp_disp_feat_get_num_screens();
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		mgr = g_disp_drv.mgr[screen_id];
		if (mgr && mgr->handle_iommu_error) {
			mgr->handle_iommu_error(mgr);
		}
	}
	DE_WARN("out iommu error\n");
}
#endif


static const struct dev_pm_ops disp_runtime_pm_ops = {
#if defined(SUPPORT_PM_RUNTIME)
	.runtime_suspend = disp_runtime_suspend,
	.runtime_resume = disp_runtime_resume,
	.runtime_idle = disp_runtime_idle,
#endif
	.suspend = disp_suspend,
	.resume = disp_resume,
};

bool disp_is_enable(void)

{
	bool ret = false;
	u32 screen_id = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		struct disp_manager *mgr = g_disp_drv.mgr[screen_id];

		if (mgr && mgr->device && mgr->device->is_enabled &&
		    mgr->device->disable)
			if (mgr->device->is_enabled(mgr->device))
				ret = true;
	}
	return ret;
}
EXPORT_SYMBOL(disp_is_enable);

static void disp_shutdown(struct platform_device *pdev)
{
	u32 screen_id = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		struct disp_manager *mgr = g_disp_drv.mgr[screen_id];

		if (mgr && mgr->device && mgr->device->is_enabled
		    && mgr->device->disable) {
			if (mgr->device->is_enabled(mgr->device))
				mgr->device->disable(mgr->device);
			mgr->enable_iommu(mgr, false);
		}
	}
}

#ifdef EINK_FLUSH_TIME_TEST
struct timeval ioctrl_start_timer;
#endif
long disp_ioctl_inner(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long *ubuffer = (unsigned long *)arg;
	s32 ret = 0;
	int num_screens = 2;
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;
	struct disp_enhance *enhance = NULL;
	struct disp_smbl *smbl = NULL;
	struct disp_capture *cptr = NULL;
#if defined(SUPPORT_EINK)
	struct disp_eink_manager *eink_manager = NULL;
#endif

#ifdef EINK_FLUSH_TIME_TEST
	do_gettimeofday(&ioctrl_start_timer);
#endif				/* test eink time */
	num_screens = bsp_disp_feat_get_num_screens();

	if (cmd == DISP_NODE_LCD_MESSAGE_REQUEST || cmd == DISP_RELOAD_LCD) {
		goto handle_cmd;
	}

	if (ubuffer[0] < num_screens && cmd != DISP_GET_VSYNC_TIMESTAMP &&
	    cmd != DISP_GET_HW_INFO && cmd != DISP_GET_BOOT_CONFIG)
		mgr = g_disp_drv.mgr[ubuffer[0]];
	if (mgr) {
		dispdev = mgr->device;
		enhance = mgr->enhance;
		smbl = mgr->smbl;
		cptr = mgr->cptr;
	}
#if defined(SUPPORT_EINK)
	eink_manager = g_disp_drv.eink_manager[0];

	if (!eink_manager)
		DE_WARN("eink_manager is NULL\n");

#endif

	if (cmd < DISP_FB_REQUEST && cmd != DISP_GET_VSYNC_TIMESTAMP &&
	    cmd != DISP_GET_HW_INFO && cmd != DISP_GET_BOOT_CONFIG) {
		if (ubuffer[0] >= num_screens) {
			DE_WARN("para err, cmd = 0x%x,screen id = %lu\n",
				cmd, ubuffer[0]);
			return -1;
		}
	}
	if (DISPLAY_DEEP_SLEEP & suspend_status) {
		DE_WARN("ioctl:%x fail when in suspend\n", cmd);
		return -1;
	}

	if (cmd == DISP_print)
		DE_WARN("cmd:0x%x,%ld,%ld\n", cmd, ubuffer[0], ubuffer[1]);

handle_cmd:
	switch (cmd) {
		/* ----disp global---- */
	case DISP_SET_BKCOLOR:
		{
			struct disp_color para;

			if (copy_from_user(&para, (void __user *)ubuffer[1],
			     sizeof(struct disp_color))) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}
			if (mgr && (mgr->set_back_color != NULL))
				ret = mgr->set_back_color(mgr, &para);
			break;
		}

	case DISP_GET_OUTPUT_TYPE:
		{
			if (suspend_status != DISPLAY_NORMAL)
				ret = suspend_output_type[ubuffer[0]];
			else
				ret = bsp_disp_get_output_type(ubuffer[0]);

			break;
		}

	case DISP_GET_SCN_WIDTH:
		{
			unsigned int width = 0, height = 0;

			if (mgr && mgr->device && mgr->device->get_resolution)
				mgr->device->get_resolution(mgr->device, &width,
							    &height);
			ret = width;
			break;
		}

	case DISP_GET_SCN_HEIGHT:
		{
			unsigned int width = 0, height = 0;

			if (mgr && mgr->device && mgr->device->get_resolution)
				mgr->device->get_resolution(mgr->device, &width,
							    &height);
			ret = height;
			break;
		}

	case DISP_VSYNC_EVENT_EN:
		{
			ret =
			    bsp_disp_vsync_event_enable(ubuffer[0], ubuffer[1]);
			break;
		}
	case DISP_GET_VSYNC_TIMESTAMP:
		{
			struct disp_vsync_timestame ts;
			for (ts.disp = 0; ts.disp < DISP_SCREEN_NUM; ts.disp++) {
				ret = bsp_disp_get_vsync_timestamp(ts.disp, &ts.timestamp);
				if (ret == 0) {
					if (copy_to_user((void __user *)ubuffer[0], &ts,
						  sizeof(struct disp_vsync_timestame))) {
						DE_WARN("copy_to_user fail\n");
						ret = -EFAULT;
					}
					break;
				}
			}
			break;
		}

	case DISP_GET_HW_INFO:
		{
			ret = bsp_disp_get_hw_info((struct disp_hw_info __user *)ubuffer[0]);
			break;
		}

	case DISP_SHADOW_PROTECT:
		{
			ret = bsp_disp_shadow_protect(ubuffer[0], ubuffer[1]);
			break;
		}

	case DISP_BLANK:
		{
			/* only response main device' blank request */

			if (!g_pm_runtime_enable)
				break;

			if (ubuffer[0] != 0)
				break;

			if (ubuffer[1]) {
#if IS_ENABLED(CONFIG_ARCH_SUN50IW6)
			bsp_disp_hdmi_cec_standby_request();
#endif
#if defined(SUPPORT_PM_RUNTIME)
				if (!g_disp_drv.dev)
					DE_WARN("display device is null\n");
				else if (pm_runtime_active(g_disp_drv.dev))
					pm_runtime_put_sync(g_disp_drv.dev);
#endif
				suspend_status |= DISPLAY_BLANK;
				disp_blank(true);
			} else {

				disp_blank(false);
				suspend_status &= ~DISPLAY_BLANK;
#if defined(SUPPORT_PM_RUNTIME)
				if (!g_disp_drv.dev) {
					DE_WARN("display device is null\n");
				} else if (pm_runtime_suspended(g_disp_drv.dev))
					pm_runtime_get_sync(g_disp_drv.dev);
#endif
			}
			break;
		}

	case DISP_DEVICE_SWITCH:
		{
			/* if the display device has already enter blank status,
			 * DISP_DEVICE_SWITCH request will not be responsed.
			 */
			if (!(suspend_status & DISPLAY_BLANK))
				ret =
				    bsp_disp_device_switch(ubuffer[0],
					   (enum disp_output_type)ubuffer[1],
					   (enum disp_output_type)ubuffer[2]);
			suspend_output_type[ubuffer[0]] = ubuffer[1];
#if defined(SUPPORT_TV) && defined(CONFIG_ARCH_SUN50IW2P1)
			bsp_disp_tv_set_hpd(1);
#endif
		break;
	}

	case DISP_DEVICE_SET_CONFIG:
	{
		struct disp_device_config config;

		if (copy_from_user(&config, (void __user *)ubuffer[1],
			sizeof(struct disp_device_config))) {
			DE_WARN("copy_from_user fail\n");
			return  -EFAULT;
		}
		suspend_output_type[ubuffer[0]] = config.type;
		if (pm_runtime_suspended(g_disp_drv.dev))
			pm_runtime_get_sync(g_disp_drv.dev);

		ret = bsp_disp_device_set_config(ubuffer[0], &config);
		break;
	}

	case DISP_DEVICE_GET_CONFIG:
	{
		struct disp_device_config config;

		if (mgr && dispdev)
			dispdev->get_static_config(dispdev, &config);
		else
			ret = -EFAULT;

		if (ret == 0) {
			if (copy_to_user((void __user *)ubuffer[1], &config,
				sizeof(struct disp_device_config))) {
				DE_WARN("copy_to_user fail\n");
				return  -EFAULT;
			}
		}
		break;
	}

	case DISP_GET_BOOT_CONFIG:
	{
		ret = bsp_disp_get_boot_config_list((struct disp_boot_config_list __user *)
						    ubuffer[0]);
		break;
	}
#if defined(SUPPORT_EINK)

	case DISP_EINK_UPDATE:
		{
			s32 i = 0;
			struct area_info area;
			const unsigned int lyr_cfg_size = ARRAY_SIZE(lyr_cfg);

			if (IS_ERR_OR_NULL((void __user *)ubuffer[3])) {
				DE_WARN("incoming pointer of user is ERR or NULL");
				return -EFAULT;
			}

			if (ubuffer[1] == 0 || ubuffer[1] > lyr_cfg_size) {
				DE_WARN("layer number need to be set from 1 to %d\n", lyr_cfg_size);
				return -EFAULT;
			}

			if (!eink_manager) {
				DE_WARN("there is no eink manager\n");
				break;
			}

			memset(lyr_cfg, 0,
			       16 * sizeof(struct disp_layer_config));
			if (copy_from_user(lyr_cfg, (void __user *)ubuffer[3],
			     sizeof(struct disp_layer_config) * ubuffer[1])) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}

			memset(&area, 0, sizeof(struct area_info));
			if (copy_from_user(&area, (void __user *)ubuffer[0],
					   sizeof(struct area_info))) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}

			for (i = 0; i < ubuffer[1]; i++)
				__disp_config_transfer2inner(&eink_para[i],
								&lyr_cfg[i]);

			ret = bsp_disp_eink_update(eink_manager,
				(struct disp_layer_config_inner *)&eink_para[0],
				(unsigned int)ubuffer[1],
				(enum eink_update_mode)ubuffer[2], &area);
			break;
		}

	case DISP_EINK_UPDATE2:
		{
			s32 i = 0;
			struct area_info area;
			const unsigned int lyr_cfg_size = ARRAY_SIZE(lyr_cfg2);

			if (IS_ERR_OR_NULL((void __user *)ubuffer[3])) {
				DE_WARN("incoming pointer of user is ERR or NULL");
				return -EFAULT;
			}

			if (ubuffer[1] == 0 || ubuffer[1] > lyr_cfg_size) {
				DE_WARN("layer number need to be set from 1 to %d\n", lyr_cfg_size);
				return -EFAULT;
			}

			if (!eink_manager) {
				DE_WARN("there is no eink manager\n");
				break;
			}

			memset(lyr_cfg2, 0,
			       16 * sizeof(struct disp_layer_config2));
			if (copy_from_user(lyr_cfg2, (void __user *)ubuffer[3],
			     sizeof(struct disp_layer_config2) * ubuffer[1])) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}

			memset(&area, 0, sizeof(struct area_info));
			if (copy_from_user(&area, (void __user *)ubuffer[0],
					   sizeof(struct area_info))) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}

			for (i = 0; i < ubuffer[1]; i++)
				__disp_config2_transfer2inner(&eink_para[i],
								&lyr_cfg2[i]);

			ret = bsp_disp_eink_update(eink_manager,
				(struct disp_layer_config_inner *)&eink_para[0],
				(unsigned int)ubuffer[1],
				(enum eink_update_mode)ubuffer[2], &area);
			break;
		}

	case DISP_EINK_SET_TEMP:
		{
			ret =
			    bsp_disp_eink_set_temperature(eink_manager,
							  ubuffer[0]);
			break;
		}
	case DISP_EINK_GET_TEMP:
		{
			ret = bsp_disp_eink_get_temperature(eink_manager);
			break;
		}
	case DISP_EINK_OVERLAP_SKIP:
		{
			ret = bsp_disp_eink_op_skip(eink_manager, ubuffer[0]);
			break;
		}
#endif

	case DISP_GET_OUTPUT:
		{
			struct disp_output para;

			memset(&para, 0, sizeof(struct disp_output));

			if (mgr && mgr->device) {
				para.type =
				    bsp_disp_get_output_type(ubuffer[0]);
				if (mgr->device->get_mode)
					para.mode =
					    mgr->device->get_mode(mgr->device);
			}

			if (copy_to_user((void __user *)ubuffer[1], &para,
			     sizeof(struct disp_output))) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}
			break;
		}

	case DISP_SET_COLOR_RANGE:
		{
			if (mgr && mgr->set_output_color_range)
				ret =
				    mgr->set_output_color_range(mgr,
								ubuffer[1]);

			break;
		}

	case DISP_GET_COLOR_RANGE:
		{
			if (mgr && mgr->get_output_color_range)
				ret = mgr->get_output_color_range(mgr);

			break;
		}

	case DISP_DEVICE_GET_FPS:
		{
			if (mgr && mgr->device && mgr->device->get_fps)
				ret = mgr->device->get_fps(mgr->device);

			break;
		}

		/* ----layer---- */
	case DISP_LAYER_SET_CONFIG:
	{
		unsigned int i = 0;
		const unsigned int lyr_cfg_size = ARRAY_SIZE(lyr_cfg);

		mutex_lock(&g_disp_drv.mlock);

		if (IS_ERR_OR_NULL((void __user *)ubuffer[1])) {
			DE_WARN("incoming pointer of user is ERR or NULL");
			mutex_unlock(&g_disp_drv.mlock);
			return -EFAULT;
		}

		if (ubuffer[2] == 0 || ubuffer[2] > lyr_cfg_size) {
			DE_WARN("layer number need to be set from 1 to %d\n", lyr_cfg_size);
			mutex_unlock(&g_disp_drv.mlock);
			return -EFAULT;
		}

		if (copy_from_user(lyr_cfg,
			(void __user *)ubuffer[1],
			sizeof(struct disp_layer_config) * ubuffer[2]))	{
			DE_WARN("copy_from_user fail\n");
			mutex_unlock(&g_disp_drv.mlock);

			return  -EFAULT;
		}

		for (i = 0; (i < lyr_cfg_size) && (i < ubuffer[2]); ++i) {
			if (lyr_cfg[i].enable == 0) {
				memset(&(lyr_cfg[i].info), 0,
					sizeof(lyr_cfg[i].info));
			}
		}

#if !defined(CONFIG_EINK_PANEL_USED)
		if (mgr && mgr->set_layer_config)
			ret = mgr->set_layer_config(mgr, lyr_cfg, ubuffer[2]);
#endif
		mutex_unlock(&g_disp_drv.mlock);
		break;
	}

	case DISP_LAYER_GET_CONFIG:
	{
		if (copy_from_user(lyr_cfg,
			(void __user *)ubuffer[1],
			sizeof(struct disp_layer_config) * ubuffer[2]))	{
			DE_WARN("copy_from_user fail\n");

			return  -EFAULT;
		}
		if (mgr && mgr->get_layer_config)
			ret = mgr->get_layer_config(mgr, lyr_cfg, ubuffer[2]);
		if (copy_to_user((void __user *)ubuffer[1],
			lyr_cfg,
			sizeof(struct disp_layer_config) * ubuffer[2]))	{
			DE_WARN("copy_to_user fail\n");

			return  -EFAULT;
		}
		break;
	}

	case DISP_LAYER_SET_CONFIG2:
	{
		struct disp_layer_config2 *pLyr_cfg2;
		unsigned int i = 0;
		unsigned int cnt;
		const unsigned int lyr_cfg_size =
			ARRAY_SIZE(lyr_cfg2);
		unsigned int pLyr_cfg2_size;

		/* adapt to multi thread call in case of disp 0 & 1 work together */
		if (ubuffer[0] == 0) {
			pLyr_cfg2 = lyr_cfg2;
			pLyr_cfg2_size = ARRAY_SIZE(lyr_cfg2);
		} else {
			pLyr_cfg2 = lyr_cfg2_1;
			pLyr_cfg2_size = ARRAY_SIZE(lyr_cfg2_1);
		}

		if (IS_ERR_OR_NULL((void __user *)ubuffer[1])) {
			DE_WARN("incoming pointer of user is ERR or NULL");
			return -EFAULT;
		}

		if (ubuffer[2] == 0 || ubuffer[2] > pLyr_cfg2_size) {
			DE_WARN("layer number need to be set from 1 to %d\n", pLyr_cfg2_size);
			return -EFAULT;
		}

		if (copy_from_user(pLyr_cfg2,
		    (void __user *)ubuffer[1],
		    sizeof(struct disp_layer_config2) * ubuffer[2])) {
			DE_WARN("copy_from_user fail\n");

			return  -EFAULT;
		}
		for (i = 0; (i < lyr_cfg_size) && (i < ubuffer[2]); ++i) {
			if (pLyr_cfg2[i].enable == 0) {
				cnt = pLyr_cfg2[i].info.id;
				memset(&(pLyr_cfg2[i].info), 0,
					sizeof(pLyr_cfg2[i].info));
				pLyr_cfg2[i].info.id = cnt;
			}
		}

#if !defined(CONFIG_EINK_PANEL_USED)
		if (mgr && mgr->set_layer_config2)
			ret = mgr->set_layer_config2(mgr, pLyr_cfg2, ubuffer[2]);
#endif
		break;
	}

	case DISP_RTWB_COMMIT:
	{
#if defined(SUPPORT_RTWB)
		struct disp_layer_config2 *pLyr_cfg2;
		struct disp_capture_info2 info2;
		unsigned int i = 0;
		const unsigned int lyr_cfg_size =
			ARRAY_SIZE(lyr_cfg2);
		unsigned int pLyr_cfg2_size;

		/* adapt to multi thread call in case of disp 0 & 1 work together */
		if (ubuffer[0] == 0) {
			pLyr_cfg2 = lyr_cfg2;
			pLyr_cfg2_size = ARRAY_SIZE(lyr_cfg2);
		} else {
			pLyr_cfg2 = lyr_cfg2_1;
			pLyr_cfg2_size = ARRAY_SIZE(lyr_cfg2_1);
		}

		if (IS_ERR_OR_NULL((void __user *)ubuffer[1])) {
			DE_WARN("incoming pointer of user is ERR or NULL");
			return -EFAULT;
		}

		if (ubuffer[2] == 0 || ubuffer[2] > pLyr_cfg2_size) {
			DE_WARN("layer number need to be set from 1 to %d\n", pLyr_cfg2_size);
			return -EFAULT;
		}

		if (copy_from_user(pLyr_cfg2,
		    (void __user *)ubuffer[1],
		    sizeof(struct disp_layer_config2) * ubuffer[2])) {
			DE_WARN("copy_from_user fail\n");

			return  -EFAULT;
		}

		if (copy_from_user(&info2,
		    (void __user *)ubuffer[3],
		    sizeof(struct disp_capture_info2))) {
			DE_WARN("copy_from_user  disp_capture_info2 fail\n");

			return  -EFAULT;
		}


		for (i = 0; (i < lyr_cfg_size) && (i < ubuffer[2]); ++i) {
			if (pLyr_cfg2[i].enable == 0) {
				memset(&(pLyr_cfg2[i].info), 0,
					sizeof(pLyr_cfg2[i].info));
			}
		}

		if (mgr)
			ret = disp_mgr_set_rtwb_layer(mgr, pLyr_cfg2, &info2, ubuffer[2]);
#endif
		break;
	}

	case DISP_LAYER_GET_CONFIG2:
	{
		if (copy_from_user(lyr_cfg2,
		    (void __user *)ubuffer[1],
		    sizeof(struct disp_layer_config2) * ubuffer[2])) {
			DE_WARN("copy_from_user fail\n");

			return  -EFAULT;
		}

		if (mgr && mgr->get_layer_config2)
			ret = mgr->get_layer_config2(mgr, lyr_cfg2, ubuffer[2]);

		if (copy_to_user((void __user *)ubuffer[1],
			lyr_cfg2,
			sizeof(struct disp_layer_config2) * ubuffer[2])) {
			DE_WARN("copy_to_user fail\n");

			return  -EFAULT;
		}
		break;
	}

	/* ----channels---- */
	case DISP_CHN_SET_PALETTE:
	{
		struct disp_palette_config palette;
		if (copy_from_user(&palette,
		    (void __user *)ubuffer[1],
		    sizeof(struct disp_palette_config))) {
			 DE_WARN("copy_from_user fail\n");
			return  -EFAULT;
		}
		if (palette.num <= 0 || palette.num > 256) {
			DE_WARN("palette param err with num:%d\n", palette.num);
			return -EFAULT;
		}
		if (copy_from_user(palette_data, (void __user *)palette.data, palette.num * 4)) {
			DE_WARN("copy palette data from user fail\n");
			return -EFAULT;
		}
		palette.data = palette_data;
		if (mgr && mgr->set_palette)
			ret = mgr->set_palette(mgr, &palette);

		break;
	}

	case DISP_DEVICE_SET_COLOR_MATRIX:
	{
		struct disp_color_matrix matrix;
		if (copy_from_user(&matrix,
		    (void __user *)ubuffer[1],
		    sizeof(matrix))) {
			 DE_WARN("copy_from_user fail\n");
			return  -EFAULT;
		}
		if (mgr && mgr->set_color_matrix)
			ret = mgr->set_color_matrix(mgr, (long long *)&matrix);
		break;
	}
		/* ---- lcd --- */
	case DISP_LCD_SET_BRIGHTNESS:
		{
			if (dispdev && dispdev->set_bright)
				ret = dispdev->set_bright(dispdev, ubuffer[1]);
			break;
		}

	case DISP_LCD_GET_BRIGHTNESS:
		{
			if (dispdev && dispdev->get_bright)
				ret = dispdev->get_bright(dispdev);
			break;
		}

	case DISP_EDP_SET_BRIGHTNESS:
	{
			if (dispdev && dispdev->set_bright)
				ret = dispdev->set_bright(dispdev, ubuffer[1]);
			break;
	}

	case DISP_EDP_GET_BRIGHTNESS:
	{
			if (dispdev && dispdev->get_bright)
				ret = dispdev->get_bright(dispdev);
			break;
	}

	case DISP_TV_SET_GAMMA_TABLE:
	{
		if (IS_ERR_OR_NULL((void __user *)ubuffer[1])) {
			DE_WARN("incoming pointer of user is ERR or NULL");
			return -EFAULT;
		}

		if (dispdev && (dispdev->type == DISP_OUTPUT_TYPE_TV)) {
			u32 *gamma_tbl = kmalloc(LCD_GAMMA_TABLE_SIZE,
						 GFP_KERNEL | __GFP_ZERO);
			u32 size = ubuffer[2];

			if (gamma_tbl == NULL) {
				DE_WARN("kmalloc fail\n");
				ret = -EFAULT;
				break;
			}

			size = (size > LCD_GAMMA_TABLE_SIZE) ?
			    LCD_GAMMA_TABLE_SIZE : size;
			if (copy_from_user(gamma_tbl, (void __user *)ubuffer[1],
					  size)) {
				DE_WARN("copy_from_user fail\n");
				kfree(gamma_tbl);
				ret = -EFAULT;

				break;
			}
			if (dispdev->set_gamma_tbl)
				ret = dispdev->set_gamma_tbl(dispdev, gamma_tbl,
							     size);
			kfree(gamma_tbl);
		}
		break;
	}

	case DISP_LCD_GAMMA_CORRECTION_ENABLE:
		{
			if (dispdev &&
				(dispdev->type == DISP_OUTPUT_TYPE_LCD)) {
				ret = dispdev->enable_gamma(dispdev);
			}
			break;
		}

	case DISP_LCD_GAMMA_CORRECTION_DISABLE:
	{
		if (dispdev && (dispdev->type == DISP_OUTPUT_TYPE_LCD))
			ret = dispdev->disable_gamma(dispdev);
		break;
	}

	case DISP_LCD_SET_GAMMA_TABLE:
	{
		if (dispdev && (dispdev->type == DISP_OUTPUT_TYPE_LCD)) {
			u32 size = ubuffer[2];
			u32 *gamma_tbl;
			if (size <= 256 * sizeof(unsigned int)) {
				gamma_tbl = kmalloc(LCD_GAMMA_TABLE_SIZE,
							GFP_KERNEL | __GFP_ZERO);
			} else {
				gamma_tbl = kmalloc(LCD_GAMMA_TABLE_SIZE_10BIT,
							GFP_KERNEL | __GFP_ZERO);
			}

			if (gamma_tbl == NULL) {
				DE_WARN("kmalloc fail\n");
				ret = -EFAULT;
				break;
			}

			if (size <= 256 * sizeof(unsigned int)) {
				size = (size > LCD_GAMMA_TABLE_SIZE) ?
					LCD_GAMMA_TABLE_SIZE : size;
			} else {
				size = (size > LCD_GAMMA_TABLE_SIZE_10BIT) ?
					LCD_GAMMA_TABLE_SIZE_10BIT : size;
			}

			if (copy_from_user(gamma_tbl, (void __user *)ubuffer[1],
					  size)) {
				DE_WARN("copy_from_user fail\n");
				kfree(gamma_tbl);
				ret = -EFAULT;

				break;
			}
			ret = dispdev->set_gamma_tbl(dispdev, gamma_tbl, size);
			kfree(gamma_tbl);
		}
		break;
	}
	case DISP_LCD_GET_GAMMA_TABLE:
	{
		if (dispdev && (dispdev->type == DISP_OUTPUT_TYPE_LCD)) {
			u32 size = ubuffer[2];
			u32 *gamma_tbl;
			if (size <= 256 * sizeof(unsigned int)) {
				gamma_tbl = kmalloc(LCD_GAMMA_TABLE_SIZE,
							GFP_KERNEL | __GFP_ZERO);
			} else {
				gamma_tbl = kmalloc(LCD_GAMMA_TABLE_SIZE_10BIT,
							GFP_KERNEL | __GFP_ZERO);
			}

			if (gamma_tbl == NULL) {
				DE_WARN("kmalloc fail\n");
				ret = -EFAULT;
				break;
			}

			if (size <= 256 * sizeof(unsigned int)) {
				size = (size > LCD_GAMMA_TABLE_SIZE) ?
					LCD_GAMMA_TABLE_SIZE : size;
			} else {
				size = (size > LCD_GAMMA_TABLE_SIZE_10BIT) ?
					LCD_GAMMA_TABLE_SIZE_10BIT : size;
			}

			if (dispdev->get_gamma_tbl) {
				ret = dispdev->get_gamma_tbl(dispdev, gamma_tbl, size);
				if (copy_to_user((void __user *)ubuffer[1], gamma_tbl,
									  size)) {
					DE_WARN("copy_from_user fail\n");
					kfree(gamma_tbl);
					ret = -EFAULT;
					break;
				}
			}
			kfree(gamma_tbl);
		}
	}
	break;
		/* ---- hdmi --- */
	case DISP_HDMI_SUPPORT_MODE:
		{
			ret =
			    bsp_disp_hdmi_check_support_mode(ubuffer[0],
							     ubuffer[1]);
			break;
		}

	case DISP_DP_SUPPORT_MODE:
		{
			ret =
			    bsp_disp_edp_check_support_mode(ubuffer[0],
							     ubuffer[1]);
			break;
		}

	case DISP_SET_TV_HPD:
		{
			ret = bsp_disp_tv_set_hpd(ubuffer[0]);
			break;
		}
#if IS_ENABLED(CONFIG_ARCH_SUN50IW6)
	case DISP_CEC_ONE_TOUCH_PLAY:
	{
		ret = bsp_disp_hdmi_cec_send_one_touch_play();
		break;
	}
#endif
		/* ----enhance---- */
	case DISP_ENHANCE_ENABLE:
		{
			if (enhance && enhance->enable)
				ret = enhance->enable(enhance);
			break;
		}

	case DISP_ENHANCE_DISABLE:
		{
			if (enhance && enhance->disable)
				ret = enhance->disable(enhance);
			break;
		}

	case DISP_ENHANCE_DEMO_ENABLE:
		{
			if (enhance && enhance->demo_enable)
				ret = enhance->demo_enable(enhance);
			break;
		}

	case DISP_ENHANCE_DEMO_DISABLE:
		{
			if (enhance && enhance->demo_disable)
				ret = enhance->demo_disable(enhance);
			break;
		}

	case DISP_ENHANCE_SET_MODE:
		{
			if (enhance && enhance->set_mode)
				ret = enhance->set_mode(enhance, ubuffer[1]);
			break;
		}

	case DISP_ENHANCE_GET_MODE:
		{
			if (enhance && enhance->get_mode)
				ret = enhance->get_mode(enhance);
			break;
		}

		/* ---smart backlight -- */
	case DISP_SMBL_ENABLE:
		{
			if (smbl && smbl->enable)
				ret = smbl->enable(smbl);
			break;
		}

	case DISP_SMBL_DISABLE:
		{
			if (smbl && smbl->disable)
				ret = smbl->disable(smbl);
			break;
		}

	case DISP_SMBL_SET_WINDOW:
		{
			struct disp_rect rect;

			if (IS_ERR_OR_NULL((void __user *)ubuffer[1])) {
				DE_WARN("incoming pointer of user is ERR or NULL");
				return -EFAULT;
			}

			if (copy_from_user(&rect, (void __user *)ubuffer[1],
			     sizeof(struct disp_rect))) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}
			if (smbl && smbl->set_window)
				ret = smbl->set_window(smbl, &rect);
			break;
		}

		/* ---capture -- */
	case DISP_CAPTURE_START:
		{
			if (cptr && cptr->start)
				ret = cptr->start(cptr);
			break;
		}

	case DISP_CAPTURE_STOP:
		{
			if (cptr && cptr->stop)
				ret = cptr->stop(cptr);
			break;
		}

	case DISP_CAPTURE_COMMIT:
		{
			struct disp_capture_info info;

			if (IS_ERR_OR_NULL((void __user *)ubuffer[1])) {
				DE_WARN("incoming pointer of user is ERR or NULL");
				return -EFAULT;
			}

			if (copy_from_user(&info, (void __user *)ubuffer[1],
			     sizeof(struct disp_capture_info))) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}
			if (cptr && cptr->commmit)
				ret = cptr->commmit(cptr, &info);
			break;
		}
	case DISP_CAPTURE_COMMIT2:
	{
		struct disp_capture_info2 info;

		if (IS_ERR_OR_NULL((void __user *)ubuffer[1])) {
			DE_WARN("incoming pointer of user is ERR or NULL");
			return -EFAULT;
		}

		if (copy_from_user(&info,
				   (void __user *)ubuffer[1],
				   sizeof(struct disp_capture_info2)))	{
			DE_WARN("copy_from_user fail\n");
			return  -EFAULT;
		}
		if (cptr && cptr->commmit2)
			ret = cptr->commmit2(cptr, &info);
		break;
	}

		/* ----for test---- */
	case DISP_MEM_REQUEST:
		ret = disp_mem_request(ubuffer[0], ubuffer[1]);
		break;

	case DISP_MEM_RELEASE:
		ret = disp_mem_release(ubuffer[0]);
		break;

	case DISP_MEM_GETADR:
	{
		if (ubuffer[0] >= DISP_MEM_NUM) {
			DE_WARN("invalid param\n");
			ret = -EINVAL;
			break;
		}

		return g_disp_mm[ubuffer[0]].mem_start;
	}

#if defined(SUPPORT_VDPO)
	case DISP_VDPO_SET_CONFIG:
		{
			struct disp_vdpo_config vdpo_para;

			if (IS_ERR_OR_NULL((void __user *)ubuffer[1])) {
				DE_WARN("incoming pointer of user is ERR or NULL");
				return -EFAULT;
			}

			if (copy_from_user(&vdpo_para, (void __user *)ubuffer[1],
					   sizeof(struct disp_vdpo_config)) {
				DE_WARN("copy_from_user fail\n");
				return -EFAULT;
			}
			if (mgr && mgr->device)
				disp_vdpo_set_config(mgr->device, &vdpo_para);
			break;
		}
#endif /* endif SUPPORT_VDPO */

#if IS_ENABLED(CONFIG_AW_DISP2_FB_ROTATION_SUPPORT)
	case DISP_ROTATION_SW_SET_ROT:
		{
			int num_screens = bsp_disp_feat_get_num_screens();
			u32 degree, chn, lyr_id;

			mutex_lock(&g_disp_drv.mlock);
			if (mgr == NULL) {
				DE_WARN("mgr is null\n");
			}
			if (mgr->rot_sw == NULL) {
				DE_WARN("mgr->rot_sw is null\n");
			}
			if (!mgr || !mgr->rot_sw || num_screens <= ubuffer[0]) {
				ret = -1;
				mutex_unlock(&g_disp_drv.mlock);
				break;
			}
			degree = ubuffer[3];
			switch (degree) {
			case ROTATION_SW_0:
			case ROTATION_SW_90:
			case ROTATION_SW_180:
			case ROTATION_SW_270:
				chn = ubuffer[1];
				lyr_id = ubuffer[2];
				ret = mgr->rot_sw->set_layer_degree(mgr->rot_sw, chn, lyr_id, degree);
				break;
			default:
				ret = -1;
			}
			mutex_unlock(&g_disp_drv.mlock);
			break;
		}

	case DISP_ROTATION_SW_GET_ROT:
		{
			int num_screens = bsp_disp_feat_get_num_screens();
			u32 chn, lyr_id;

			mutex_lock(&g_disp_drv.mlock);
			if (mgr && mgr->rot_sw && num_screens > ubuffer[0]) {
				chn = ubuffer[1];
				lyr_id = ubuffer[2];
				ret = mgr->rot_sw->get_layer_degree(mgr->rot_sw, chn, lyr_id);
			} else {
				ret = -1;
			}
			mutex_unlock(&g_disp_drv.mlock);
			break;
		}
#endif

	case DISP_LCD_CHECK_OPEN_FINISH:
		{
			if (mgr && mgr->device) {
				if (mgr->device->is_enabled)
					return mgr->device->is_enabled(mgr->device);
				else
					return -1;
			} else
					return -1;
		}

	case DISP_LCD_BACKLIGHT_ENABLE:
		{
			if (mgr && mgr->device) {
				if (mgr->device->pwm_enable)
					mgr->device->pwm_enable(mgr->device);
				if (mgr->device->backlight_enable)
					mgr->device->backlight_enable(mgr->device);

				return 0;
			}
			return -1;
			break;
		}
	case DISP_LCD_BACKLIGHT_DISABLE:
		{
			if (mgr && mgr->device) {
				if (mgr->device->pwm_disable)
					mgr->device->pwm_disable(mgr->device);
				if (mgr->device->backlight_disable)
					mgr->device->backlight_disable(mgr->device);
				return 0;
			}
			return -1;
		break;
		}
	case DISP_SET_KSC_PARA:
		{

			struct disp_ksc_info ksc;

			if (IS_ERR_OR_NULL((void __user *)ubuffer[1])) {
				DE_WARN("incoming pointer of user is ERR or NULL");
				return -EFAULT;
			}

			if (copy_from_user(&ksc, (void __user *)ubuffer[1],
					   sizeof(struct disp_ksc_info))) {
				DE_WARN("copy_from_user fail\n");
				return  -EFAULT;
			}
			if (mgr && mgr->set_ksc_para)
				ret = mgr->set_ksc_para(mgr, &ksc);

			break;
		}
	case DISP_NODE_LCD_MESSAGE_REQUEST:
		{
			int ret, lcd_node;
			struct para lcd_debug_para;
			struct para lcd_debug_para_tmp;
			struct dt_property *dt_prop;
			char prop_name[32] = {0};
			struct dt_property *dt_prop_dts;
			char prop_dts_name[32] = {0};
			unsigned char value[100] = {0};
			unsigned char dts_value[100] = {0};

			if  (copy_from_user(&lcd_debug_para, (void *)ubuffer[3], sizeof(struct para))) {
				return -2;
			}
			if (copy_from_user(&lcd_debug_para_tmp, (void *) ubuffer[3], sizeof(struct para))) {
				return -2;
			}
			if (copy_from_user(&lcd_node, (void *) ubuffer[0], sizeof(int))) {
				return -2;
			}

			if (lcd_debug_para.prop_src.length > sizeof(value) ||
			    lcd_debug_para.prop_dts.length > sizeof(dts_value)) {
				DE_WARN("lcd_debug_para length over range\n");
				return -2;
			}

			dt_prop = &lcd_debug_para.prop_src;

			ret = copy_from_user(prop_name, dt_prop->name, 32);

			if (ret)
				return -2;

			ret = copy_from_user(value, dt_prop->value, dt_prop->length);

			if (ret)
				return -2;

			dt_prop->name = prop_name;
			dt_prop->value = (void *)value;

			dt_prop_dts = &lcd_debug_para.prop_dts;

			ret = copy_from_user(prop_dts_name, dt_prop_dts->name, 32);

			if (ret)
				return -2;

			ret = copy_from_user(dts_value, dt_prop_dts->value, dt_prop_dts->length);

			if (ret)
				return -2;

			dt_prop_dts->name = prop_dts_name;
			dt_prop_dts->value = (void *)dts_value;

			ret = handle_request(&lcd_debug_para, lcd_node);

			if (ret)
				return -1;

			if (copy_to_user((void __user *)lcd_debug_para_tmp.prop_dts.name, lcd_debug_para.prop_dts.name, 32))
				return -3;

			if (copy_to_user((void __user *)lcd_debug_para_tmp.prop_dts.value, lcd_debug_para.prop_dts.value, lcd_debug_para.prop_dts.length))
				return -3;

			if (copy_to_user(&((struct para *)ubuffer[3])->prop_dts.length, &lcd_debug_para.prop_dts.length, sizeof(lcd_debug_para.prop_dts.length)))
				return -3;

			return ret;
		}
	case DISP_RELOAD_LCD:
		{
			int lcd_node = 0;

			if (ubuffer[0] == 1) {
				lcd_node = (int)ubuffer[1];
				DE_INFO("==========lcd_node = %d ===========\n", lcd_node);
				if (lcd_node > 2 || lcd_node < 0)
					break;
				ret = set_lcd_timing(lcd_node, (unsigned long)ubuffer);
			} else if (ubuffer[0] == 2) {
				lcd_node = (int)ubuffer[1];
				DE_INFO("==========lcd_node = %d ===========\n", lcd_node);
				if (lcd_node > 2 || lcd_node < 0)
					break;
				ret = get_lcd_timing(lcd_node, (unsigned long)ubuffer);
			} else {
				if (copy_from_user(&lcd_node, (void *)ubuffer[3], sizeof(int))) {
					return -2;
				}
				reload_lcd(lcd_node);
			}
			break;
		}

	default:
		ret = disp_ioctl_extend(cmd, (unsigned long)ubuffer);
		break;
	}

	return ret;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long disp_compat_ioctl(struct file *file, unsigned int cmd,
			      unsigned long arg)
{
	compat_uptr_t arg32[4] = {0};
	unsigned long arg64[4] = {0};

	if (copy_from_user
	    ((void *)arg32, (void __user *)arg, 4 * sizeof(compat_uptr_t))) {
		DE_WARN("copy_from_user fail\n");
		return -EFAULT;
	}

	arg64[0] = (unsigned long)arg32[0];
	arg64[1] = (unsigned long)arg32[1];
	arg64[2] = (unsigned long)arg32[2];
	arg64[3] = (unsigned long)arg32[3];

	return disp_ioctl_inner(file, cmd, (unsigned long)arg64);
}
#endif

static long disp_ioctl(struct file *file, unsigned int cmd,
			      unsigned long arg)
{
	unsigned long arg64[4] = {0};

	if (copy_from_user
	    ((void *)arg64, (void __user *)arg, 4 * sizeof(unsigned long))) {
		DE_WARN("copy_from_user fail\n");
		return -EFAULT;
	}

	return disp_ioctl_inner(file, cmd, (unsigned long)arg64);
}

static unsigned int disp_vsync_poll(struct file *file, poll_table *wait)
{
	return vsync_poll(file, wait);
}

static const struct file_operations disp_fops = {
	.owner          = THIS_MODULE,
	.open           = disp_open,
	.release        = disp_release,
	.write          = disp_write,
	.read           = disp_read,
	.unlocked_ioctl = disp_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl   = disp_compat_ioctl,
#endif
	.mmap           = disp_mmap,
	.poll		= disp_vsync_poll,
};

#ifndef CONFIG_OF
static struct platform_device disp_device = {
	.name = "disp",
	.id = -1,
	.num_resources = ARRAY_SIZE(disp_resource),
	.resource = disp_resource,
	.dev = {
		.power = {
			  .async_suspend = 1,
			}
		}
};
#else
static const struct of_device_id sunxi_disp_match[] = {
	{.compatible = "allwinner,sun8iw10p1-disp",},
	{.compatible = "allwinner,sun50i-disp",},
	{.compatible = "allwinner,sunxi-disp",},
	{},
};
#endif

static struct platform_driver disp_driver = {
	.probe    = disp_probe,
	.remove   = disp_remove,
	.shutdown = disp_shutdown,
	.driver   = {
		  .name           = "disp",
		  .owner          = THIS_MODULE,
		  .pm             = &disp_runtime_pm_ops,
		  .of_match_table = sunxi_disp_match,
	},
};

#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC)
struct dramfreq_vb_time_ops {
	int (*get_vb_time)(void);
	int (*get_next_vb_time)(void);
	int (*is_in_vb)(void);
};
static struct dramfreq_vb_time_ops dramfreq_ops = {
	.get_vb_time = bsp_disp_get_vb_time,
	.get_next_vb_time = bsp_disp_get_next_vb_time,
	.is_in_vb = bsp_disp_is_in_vb,
};
extern int dramfreq_set_vb_time_ops(struct dramfreq_vb_time_ops *ops);
#endif

static int __init disp_module_init(void)
{
	int ret = 0, err;

	DE_INFO("\n");

	alloc_chrdev_region(&devid, 0, 1, "disp");
	if (my_cdev)
		cdev_del(my_cdev);
	my_cdev = cdev_alloc();
	if (!my_cdev) {
		DE_ERR("cdev_alloc err\n");
		return -1;
	}
	cdev_init(my_cdev, &disp_fops);
	my_cdev->owner = THIS_MODULE;
	err = cdev_add(my_cdev, devid, 1);
	if (err) {
		DE_ERR("cdev_add fail\n");
		return -1;
	}

	disp_class = class_create(THIS_MODULE, "disp");
	if (IS_ERR(disp_class)) {
		DE_ERR("class_create fail\n");
		return -1;
	}

	display_dev = device_create(disp_class, NULL, devid, NULL, "disp");

#ifndef CONFIG_OF
	ret = platform_device_register(&disp_device);
#endif
	if (ret == 0)
		ret = platform_driver_register(&disp_driver);
#if IS_ENABLED(CONFIG_AW_DISP2_DEBUG)
	dispdbg_init();
#endif

#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC)
	dramfreq_set_vb_time_ops(&dramfreq_ops);
#endif

	DE_INFO("finish, ret : %d\n", ret);

	return ret;
}

static void __exit disp_module_exit(void)
{
	DE_INFO("disp_module_exit\n");

#if IS_ENABLED(CONFIG_AW_DISP2_DEBUG)
	dispdbg_exit();
#endif

	disp_exit();

	platform_driver_unregister(&disp_driver);
#ifndef CONFIG_OF
	platform_device_unregister(&disp_device);
#endif

	device_destroy(disp_class, devid);
	class_destroy(disp_class);

	cdev_del(my_cdev);
}

module_init(disp_module_init);
module_exit(disp_module_exit);

MODULE_AUTHOR("tan");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_DESCRIPTION("display driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:disp");

/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/*HDMI panel ops*/
#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_edid.h>
#include "../sunxi_drm_drv.h"
#include "../sunxi_drm_core.h"
#include "../sunxi_drm_crtc.h"
#include "../sunxi_drm_encoder.h"
#include "../sunxi_drm_connector.h"
#include "../sunxi_drm_plane.h"
#include "../drm_de/drm_al.h"
#include "../subdev/sunxi_common.h"
#include "../sunxi_drm_panel.h"
#include "sunxi_hdmi.h"
#include "hdmi_bsp.h"
#include "de/include.h"
#include <linux/clk-private.h>
#if defined(CONFIG_ARCH_SUN50IW2P1)
#define HDMI_USING_INNER_BIAS 1
#endif

/* vendor specific mode */
static const struct drm_display_mode edid_vsp_modes[] = {
	/* 0 */
	{ DRM_MODE("1920x1080@3D", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2558,
		2602, 2750, 0, 1080, 1084, 1089, 2160, 0,
		DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 24, },
	/* 1 */
	{ DRM_MODE("1280x720@3D", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1720,
		1760, 1980, 0, 1440, 1445, 1450, 1500, 0,
		DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 50, },
	/* 2 */
	{ DRM_MODE("1280x720@3D", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1390,
		1430, 1650, 0, 1440, 1445, 1450, 1500, 0,
		DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 60, },
	/* 3 */
	{ DRM_MODE("3840x2160@30", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4010,
		4098, 4400, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 30, },
	/* 4 */
	{ DRM_MODE("3840x2160@25", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4896,
		4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
		DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
		.vrefresh = 25, },
};

static const struct disp_video_timings video_timing[] =
{
       //VIC	     tv_mode  PCLK    AVI_PR  X      Y     HT     HBP  HFP   HST   VT    VBP VFP VST h_pol v_pol int  vac  trd
/* 0 */{HDMI1440_480I,      0,13500000,  1,  720,   480,   858,   57,   19,   62,  525,   15,  4,  3,  0,   0,    1,   0,   0},
/* 1 */{HDMI1440_576I,      0,13500000,  1,  720,   576,   864,   69,   12,   63,  625,   19,  2,  3,  0,   0,    1,   0,   0},
/* 2 */{HDMI480P,           0,27000000,  0,  720,   480,   858,   60,   16,   62,  525,   30,  9,  6,  0,   0,    0,   0,   0},
/* 3 */{HDMI576P,           0,27000000,  0,  720,   576,   864,   68,   12,   64,  625,   39,  5,  5,  0,   0,    0,   0,   0},
/* 4 */{HDMI720P_50,        0,74250000,  0,  1280,  720,   1980,  220,  440,  40,  750,   20,  5,  5,  1,   1,    0,   0,   0},
/* 5 */{HDMI720P_60,        0,74250000,  0,  1280,  720,   1650,  220,  110,  40,  750,   20,  5,  5,  1,   1,    0,   0,   0},
/* 6 */{HDMI1080I_50,       0,74250000,  0,  1920,  1080,  2640,  148,  528,  44,  1125,  15,  2,  5,  1,   1,    1,   0,   0},
/* 7 */{HDMI1080I_60,       0,74250000,  0,  1920,  1080,  2200,  148,  88,   44,  1125,  15,  2,  5,  1,   1,    1,   0,   0},
/* 8 */{HDMI1080P_50,       0,148500000, 0,  1920,  1080,  2640,  148,  528,  44,  1125,  36,  4,  5,  1,   1,    0,   0,   0},
/* 9 */{HDMI1080P_60,       0,148500000, 0,  1920,  1080,  2200,  148,  88,   44,  1125,  36,  4,  5,  1,   1,    0,   0,   0},
/* 10*/{HDMI1080P_24,       0,74250000,  0,  1920,  1080,  2750,  148,  638,  44,  1125,  36,  4,  5,  1,   1,    0,   0,   0},
/* 11*/{HDMI1080P_25,       0,74250000,  0,  1920,  1080,  2640,  148,  528,  44,  1125,  36,  4,  5,  0,   0,    0,   0,   0},
/* 12*/{HDMI1080P_30,       0,74250000,  0,  1920,  1080,  2200,  148,  88,   44,  1125,  36,  4,  5,  0,   0,    0,   0,   0},
/* 13*/{HDMI1080P_24_3D_FP, 0,148500000, 0,  1920,  2160,  2750,  148,  638,  44,  1125,  36,  4,  5,  1,   1,    0,   45,  1},
/* 14*/{HDMI720P_50_3D_FP,  0,148500000, 0,  1280,  1440,  1980,  220,  440,  40,  750,   20,  5,  5,  1,   1,    0,   30,  1},
/* 15*/{HDMI720P_60_3D_FP,  0,148500000, 0,  1280,  1440,  1650,  220,  110,  40,  750,   20,  5,  5,  1,   1,    0,   30,  1},
/* 16*/{HDMI3840_2160P_30,  0,297000000, 0,  3840,  2160,  4400,  296,  176,  88,  2250,  72,  8, 10,  1,   1,    0,    0,  0},
/* 17*/{HDMI3840_2160P_25,  0,297000000, 0,  3840,  2160,  5280,  296, 1056,  88,  2250,  72,  8, 10,  1,   1,    0,    0,  0},
};

void hdmi_delay_us(unsigned long us)
{
	udelay(us);
}

void hdmi_delay_ms(unsigned long ms)
{
	sunxi_drm_delayed_ms(ms);
}

enum drm_mode_status sunxi_hdmi_check_mode(struct sunxi_panel *sunxi_panel,
	struct drm_display_mode *mode)
{
	struct disp_video_timings hdmi_timing;
	return sunxi_hdmi_mode_timmings(&hdmi_timing, mode);
}

bool default_hdmi_init(struct sunxi_panel *sunxi_panel)
{
	struct sunxi_hdmi_private *sunxi_hdmi_p;
	struct video_para *glb_video_para;
	struct audio_para * glb_audio_para;
	struct disp_video_timings *hdmi_timing;
	sunxi_hdmi_p = (struct sunxi_hdmi_private *)sunxi_panel->private;
	glb_video_para = &sunxi_hdmi_p->video_para;
	glb_audio_para = &sunxi_hdmi_p->audio_para;
	hdmi_timing = sunxi_hdmi_p->timing;

	memcpy(hdmi_timing, &video_timing[4], sizeof(struct disp_video_timings));

	glb_video_para->vic = HDMI720P_50;
	glb_video_para->csc = BT601;
	glb_video_para->is_hdmi = 1;
	glb_video_para->is_yuv = 0;
	glb_video_para->is_hcts = 0;
	glb_audio_para->type = 1;
	glb_audio_para->sample_rate = 44100;
	glb_audio_para->sample_bit = 16;
	glb_audio_para->ch_num = 2;
	glb_audio_para->ca = 0;
	glb_audio_para->vic = HDMI720P_50;
	return true;
}

static int hdmi_io_request(struct sunxi_hdmi_private *sunxi_hdmi_p)
{
	int i;

	for (i = 0; i < HDMI_IO_NUM; i++) {
		if (sunxi_hdmi_p->hdmi_io_used[i]) {
			disp_gpio_set_t  gpio_info[1];
			memcpy(gpio_info, &(sunxi_hdmi_p->hdmi_io[i]), sizeof(disp_gpio_set_t));
			sunxi_hdmi_p->gpio_handle[i] = sunxi_drm_sys_gpio_request(gpio_info);
		}
	}

	return 0;
}

static int hdmi_io_release(struct sunxi_hdmi_private *sunxi_hdmi_p)
{
	int i;

	for (i = 0; i < HDMI_IO_NUM; i++) {
		if (sunxi_hdmi_p->hdmi_io_used[i] && sunxi_hdmi_p->gpio_handle[i] != 0) {
			disp_gpio_set_t  gpio_info[1];
			memcpy(gpio_info, &(sunxi_hdmi_p->hdmi_io[i]), sizeof(disp_gpio_set_t));
			sunxi_hdmi_p->gpio_handle[i] = sunxi_drm_sys_gpio_request(gpio_info);
			gpio_info->mul_sel = 7;
			sunxi_drm_sys_gpio_release(sunxi_hdmi_p->gpio_handle[i]);
			sunxi_hdmi_p->gpio_handle[i] = 0;
		}
	}
	return 0;
}

bool default_hdmi_open(struct sunxi_panel *sunxi_panel)
{
	struct sunxi_hdmi_private *sunxi_hdmi_p;
	sunxi_hdmi_p = (struct sunxi_hdmi_private *)sunxi_panel->private;
	hdmi_io_request(sunxi_hdmi_p);

	bsp_hdmi_video(&sunxi_hdmi_p->video_para);
	bsp_hdmi_audio(&sunxi_hdmi_p->audio_para);
	bsp_hdmi_set_video_en(1);
	return true;
}

bool default_hdmi_close(struct sunxi_panel *sunxi_panel)
{
	struct sunxi_hdmi_private *sunxi_hdmi_p;
	sunxi_hdmi_p = (struct sunxi_hdmi_private *)sunxi_panel->private;
	bsp_hdmi_set_video_en(0);
	hdmi_io_release(sunxi_hdmi_p);
	return true;
}

enum drm_mode_status sunxi_hdmi_mode_timmings(void *timing, struct drm_display_mode *mode)
{
	struct disp_video_timings *hdmi_timing = (struct disp_video_timings *)timing;
	const struct disp_video_timings *fix_timing = NULL;
	int vrefresh;
	if (!hdmi_timing || !mode)
		return MODE_BAD;
	vrefresh = mode->vrefresh;
	if (mode->vrefresh == 0) {
		vrefresh = drm_mode_vrefresh(mode);
		DRM_DEBUG_KMS("vrefresh:%d id:%d (%d x %d) flag:%x\n",
		vrefresh,mode->base.id,mode->hdisplay,mode->vdisplay,mode->flags);
	}

	if (vrefresh == 60 || vrefresh == 59 || vrefresh == 61) {
		if(mode->flags & DRM_MODE_FLAG_INTERLACE) {
			if (mode->hdisplay == 1920 && mode->vdisplay == 1080) {
				/* HDMI1080I_60 */
				fix_timing = &video_timing[7];
			}
			if (mode->hdisplay == 720 && mode->vdisplay == 480) {
				/* HDMI1440_480I */
				fix_timing = &video_timing[0];
			}
		}else{
			if (mode->vdisplay == 1080 && mode->hdisplay == 1920) {
				/* HDMI1080P_60 */
				fix_timing = &video_timing[9];
			}
			if (mode->vdisplay == 720 && mode->hdisplay == 1280) {
				/* HDMI720P_60 */
				fix_timing = &video_timing[5];
			}
			if (mode->vdisplay == 480 && mode->hdisplay == 720) {
				/* HDMI480P */
				fix_timing = &video_timing[2];
			}
			if (mode->vdisplay == 576 && mode->hdisplay == 720) {
				/* HDMI576P */
				fix_timing = &video_timing[3];
			}
			if (mode->vdisplay == 1440 && mode->hdisplay == 1280) {
				/* HDMI720P_60_3D_FP */
				fix_timing = &video_timing[15];
			}
		}
	}else if (vrefresh == 50 || vrefresh == 49 || vrefresh == 51) {
		if(mode->flags & DRM_MODE_FLAG_INTERLACE) {
			if (mode->hdisplay == 1920 && mode->vdisplay == 1080) {
				fix_timing = &video_timing[6];
				/* HDMI1080I_50 */
			}
			if (mode->hdisplay == 720 && mode->vdisplay == 576) {
				/* HDMI1440_576I */
				fix_timing = &video_timing[1];
			}
		}else{
			if (mode->vdisplay == 1080 && mode->hdisplay == 1920) {
				/* HDMI1080P_50 */
				fix_timing = &video_timing[8];
			}
			if (mode->vdisplay == 720 && mode->hdisplay == 1280) {
				/* HDMI720P_50 */
				fix_timing = &video_timing[4];
			}
			if (mode->vdisplay == 1440 && mode->hdisplay == 1280) {
				/* HDMI720P_50_3D_FP */
				fix_timing = &video_timing[14];
			}
		}
	}else if (vrefresh == 30 || vrefresh == 29 || vrefresh == 31) {
		if (mode->vdisplay == 1080 && mode->hdisplay == 1920) {
			/* HDMI1080P_30 */
			fix_timing = &video_timing[12];
		}
		if (mode->vdisplay == 2160 && mode->hdisplay == 3840) {
			/* HDMI3840_2160P_30 */
			fix_timing = &video_timing[16];
		}
	}else if (vrefresh == 23 || vrefresh == 24 ||
		vrefresh == 25 || vrefresh == 26) {
		if (mode->vdisplay == 2160 && mode->hdisplay == 1920) {
			/* HDMI1080P_24_3D_FP */
			fix_timing = &video_timing[13];
		}
		if (mode->vdisplay == 1080 && mode->hdisplay == 1920 &&
			mode->htotal == 2640) {
			/* HDMI1080P_25 */
			fix_timing = &video_timing[11];
		}
		if (mode->vdisplay == 1080 && mode->hdisplay == 1920 &&
			mode->htotal == 2750) {
			/* HDMI1080P_24 */
			fix_timing = &video_timing[10];
		}
		if (mode->vdisplay == 2160 && mode->hdisplay == 3840) {
			/* HDMI3840_2160P_25 */
			fix_timing = &video_timing[17];
		}
	}

	if (fix_timing == NULL)
		return MODE_ERROR;

	memcpy(hdmi_timing, fix_timing, sizeof(struct disp_video_timings));
	DRM_DEBUG_KMS("[%d] vrefresh:%d id:%d(%d x %d) flag:%x vic:%d  %d\n",__LINE__,
		vrefresh,mode->base.id, mode->hdisplay,mode->vdisplay,
		mode->flags, fix_timing->vic, hdmi_timing->vic);

	return MODE_OK;
}

static void edid_read_data(u8 block, struct edid *buf)
{
	int ret;
	u8 *pbuf = ((u8 *)(buf)) + 128*block;
	u8 offset = (block&0x01)? 128:0;
	ret = bsp_hdmi_ddc_read(Explicit_Offset_Address_E_DDC_Read, block>>1, offset, 128, pbuf);

}
/* drm does not add the vendor specific mode, so we add it */
int sunxi_vendor_specific_mode(struct drm_connector *connector,
            unsigned char *edid, int size)
{
	struct drm_display_mode *newmode;

	u8 index = 8;
	u8 vic_len = 0;
	u8 i;
	int cout = 0;

	/* check if it's HDMI VSDB */
	if (!((edid[0] ==0x03) &&	(edid[1] ==0x0c) &&	(edid[2] ==0x00))) {
		return 0;
	}

	if (size <= 8)
		return 0;


	if ((edid[7]&0x20) == 0 )
		return 0;
	if ((edid[7]&0x40) == 0x40 )
		index = index +2;
	if ((edid[7]&0x80) == 0x80 )
		index = index +2;

	/* mandatary format support */
	if (edid[index]&0x80) {
		i = 3;
		while (i--){
			newmode = drm_mode_duplicate(connector->dev,
					&edid_vsp_modes[i]);
			if (newmode) {
				cout++;
				drm_mode_probed_add(connector, newmode);
			}else {
				DRM_ERROR("kmzalloc edid for sunxi failed.\n");
			}
		}
	}
	DRM_INFO("3D_multi_present   %d.\n",cout);
	if ( ((edid[index]&0x60) ==1) || ((edid[index]&0x60) ==2) )
		DRM_INFO("3D_multi_present.\n");

	vic_len = edid[index+1]>>5;
	for (i = 0; i < vic_len; i++) {
		/* HDMI_VIC for extended resolution transmission */
		if ((edid[index+1+1+i] + 0x100) == HDMI3840_2160P_30)
			newmode = drm_mode_duplicate(connector->dev,
					&edid_vsp_modes[3]);
		if (newmode) {
			cout++;
			drm_mode_probed_add(connector, newmode);
		}
		if (HDMI3840_2160P_25 == (edid[index+1+1+i] + 0x100))
			newmode = drm_mode_duplicate(connector->dev,
						&edid_vsp_modes[4]);
		if (newmode) {
			cout++;
			drm_mode_probed_add(connector, newmode);
		}
		DRM_INFO("edid_parse_vsdb: VIC %d support.\n", edid[index+1+1+i]);
	}
	DRM_INFO("4k  _multi_present   %d.\n",cout);

	index += (edid[index+1]&0xe0) + 2;
	if (index > (size+1) )
		return cout;

	DRM_INFO("3D_multi_present byte(%2.2x,%2.2x).\n",edid[index],edid[index+1]);

	return cout;
}

static int sunxi_load_cea_db(struct drm_connector *connector, struct edid *pedid)
{
	unsigned int offset;
	struct sunxi_hdmi_private *sunxi_hdmi_p;
	struct sunxi_drm_connector *sunxi_connector =
	to_sunxi_connector(connector);
	unsigned char *edid;
	int count = 0;
	sunxi_hdmi_p = sunxi_connector->panel->private;
	edid = (unsigned char *)pedid;
	/* must cea */
	if (edid[0]!= 2) {
		DRM_INFO("edid not cea :%d.\n",edid[0]);

		return 0;
	}
	if (edid[1] >= 1) {
		sunxi_hdmi_p->can_YCbCr444 = !!(edid[3] & 0x20);
		sunxi_hdmi_p->can_YCbCr422 = !!(edid[3] & 0x10);
	}

	offset = edid[2];
	if (offset > 4) {
		u8 bsum = 4;
		while (bsum < offset) {
			u8 tag = edid[bsum]>>5;
			u8 len = edid[bsum]&0x1f;
			if ( (len > 0) && ((bsum + len + 1) > offset) ) {
				goto out;

				if ( tag == 3) {
					count +=
					sunxi_vendor_specific_mode(connector, edid+bsum+1, len);
				}
			}
			bsum += (len +1);
		}
	}

out:
	return count;
}

void swiotlb_sync_single_for_device(struct device *hwdev, dma_addr_t dev_addr,
		size_t size, enum dma_data_direction dir);

unsigned int sunxi_hdmi_get_edid(struct sunxi_panel *panel)
{
	struct drm_connector *connector = panel->drm_connector;
	int i = 0, block = 0;
	unsigned int count = 0;
	struct edid *sunxi_deid;
	struct sunxi_hdmi_private *sunxi_hdmi_p;

	sunxi_hdmi_p = (struct sunxi_hdmi_private *)panel->private;
	sunxi_deid = kzalloc(sizeof(struct edid), GFP_KERNEL);
	if (!sunxi_deid) {
		DRM_ERROR("kzalloc edid for sunxi failed.\n");
		goto out;
	}

	edid_read_data(0, sunxi_deid);
	block = sunxi_deid->extensions;
	if (block > 0 && block < 8) {
		kfree(sunxi_deid);
		sunxi_deid = kzalloc(sizeof(struct edid) * (block+1), GFP_KERNEL);
		if (!sunxi_deid) {
			DRM_ERROR("kzalloc edid for sunxi failed.\n");
			goto out;
		}
		for (i = 0; i < (block+1); i++) {
			edid_read_data(i, sunxi_deid);
			//swiotlb_sync_single_for_device(NULL, virt_to_phys((sunxi_deid+i)), 128, DMA_BIDIRECTIONAL);
			if(i > 0)
			count += sunxi_load_cea_db(connector, sunxi_deid+i);
		}
		sunxi_hdmi_p->nr_sad = drm_edid_to_sad(sunxi_deid, &sunxi_hdmi_p->hdmi_sads);
		drm_edid_to_eld(connector, sunxi_deid);
	}

	count += drm_add_edid_modes(connector, sunxi_deid);
	if (!count) {
		DRM_ERROR("Add edid modes failed %d\n", count);
		kfree(sunxi_deid);
		goto out;
	}

	drm_mode_connector_update_edid_property(connector, sunxi_deid);
	kfree(sunxi_deid);

	DRM_INFO("%s:add %d cout modes.\n",__func__, count);

out:
	return count;
}

enum drm_connector_status sunxi_hdmi_detect(struct sunxi_panel *panel)
{
	enum drm_connector_status status = connector_status_disconnected; 
	int on_sure = 0, off_sure = 0, time = 5;

	DRM_DEBUG_KMS("%s:line:%d  status:  %d\n", __func__,__LINE__, bsp_hdmi_get_hpd());
	panel = panel;

	while (time--) {
		if (bsp_hdmi_get_hpd()) {
			on_sure++;
			off_sure = 0;
		} else {
			off_sure++;
			on_sure = 0;
		}
		if (off_sure == 3) {
			status = connector_status_disconnected;
			break;
		}
		if (on_sure == 3) {
			status = connector_status_connected;
			break;
		}
		hdmi_delay_us(100);
	}
	return status;
}

static struct panel_ops  default_panel = {
	.init = default_hdmi_init,
	.open = default_hdmi_open,
	.close = default_hdmi_close,
	.reset = NULL,
	.bright_light = NULL,
	.detect = sunxi_hdmi_detect,
	.change_mode_to_timming = sunxi_hdmi_mode_timmings,
	.check_valid_mode = sunxi_hdmi_check_mode,
	.get_modes = sunxi_hdmi_get_edid

};

void sunxi_hdmi_get_sys_info(struct sunxi_panel *sunxi_panel)
{
	struct sunxi_hdmi_private *sunxi_hdmi_p;
	struct device_node *node;
	unsigned int value;
	disp_gpio_set_t  *gpio_info;
	int i;
	char io_name[32];

	sunxi_hdmi_p = (struct sunxi_hdmi_private *)sunxi_panel->private;
	node = sunxi_drm_get_name_node("sunxi-hdmi");
	if (!node) {
		DRM_ERROR("get sunxi-hdmi node err.\n");
		return ;
	}

	sunxi_drm_get_sys_item_char(node, "hdmi_power", sunxi_hdmi_p->hdmi_power);
	if (0 == sunxi_drm_get_sys_item_int(node, "hdmi_cts_compatibility", &value)) {
		sunxi_hdmi_p->cts_compat = !!value;
	}
	if (0 == sunxi_drm_get_sys_item_int(node, "hdmi_hdcp_enable", &value)) {
		sunxi_hdmi_p->hdcp_enable = !!value;
	}
	if (0 == sunxi_drm_get_sys_item_int(node, "hdmi_cec_support", &value)) {
		sunxi_hdmi_p->cec_support = !!value;
	}

	for (i = 0; i < HDMI_IO_NUM; i++) {
		gpio_info = &(sunxi_hdmi_p->hdmi_io[i]);
		sprintf(io_name, "hdmi_io_%d", i);
		if (0 == sunxi_drm_get_sys_item_gpio(node, io_name, gpio_info))
			sunxi_hdmi_p->hdmi_io_used[i]= 1;
		else
			sunxi_hdmi_p->hdmi_io_used[i] = 0;
	}
}

int sunxi_hdmi_set_audio_mode(struct sunxi_panel *sunxi_panel)
{
	return 0;
}

unsigned char  sunxi_hdmi_yuv_output(struct sunxi_hdmi_private *sunxi_hdmi_p)
{
	if (!sunxi_hdmi_p->can_YCbCr444)
		return 0;   
	switch (sunxi_hdmi_p->timing->vic) {
	case HDMI1080P_24:
	case HDMI1080P_24_3D_FP:
	case HDMI3840_2160P_24:
	case HDMI3840_2160P_30:
	case HDMI3840_2160P_25:
		return 0;
	}
	/* 1 is can_YCbCr444 */
	return 1;
}

int sunxi_hdmi_set_timing(void *data, struct drm_display_mode *mode)
{
	struct sunxi_drm_connector *sunxi_connector =
				to_sunxi_connector(data);
	struct sunxi_panel *sunxi_panel = sunxi_connector->panel;
	struct sunxi_hdmi_private *sunxi_hdmi_p = (struct sunxi_hdmi_private *)sunxi_panel->private;
	struct sunxi_hardware_res *hw_res = sunxi_connector->hw_res;
	enum drm_mode_status status;

	if (!mode)
		return 0;

	status = sunxi_panel->panel_ops->change_mode_to_timming(sunxi_hdmi_p->timing, mode);
	if (status != MODE_OK) {
		DRM_ERROR("sunxi hdmi set timmings err status:%d.\n",status);
		return  -EINVAL;
	}
	DRM_DEBUG_KMS("[%d] con_id:%d  vic:%d, mode_id:%d\n", __LINE__,
	sunxi_connector->connector_id,sunxi_hdmi_p->timing->vic, mode->base.id);

	sunxi_hdmi_p->video_para.vic = sunxi_hdmi_p->timing->vic;
	sunxi_hdmi_p->video_para.is_yuv = 0;//sunxi_hdmi_yuv_output(sunxi_hdmi_p);
	sunxi_hdmi_p->video_para.is_hdmi = 1;
	sunxi_hdmi_p->video_para.csc = BT601;
	sunxi_hdmi_p->audio_para.vic = sunxi_hdmi_p->timing->vic;
	hw_res->clk_rate = sunxi_hdmi_p->timing->pixel_clk;

	return 0;
}

bool sunxi_hdmi_disable(void *data)
{
	struct sunxi_drm_connector *sunxi_connector =
				to_sunxi_connector(data);
	struct sunxi_hardware_res  *hw_res = sunxi_connector->hw_res;
	struct sunxi_panel *sunxi_panel = sunxi_connector->panel;

	sunxi_panel->panel_ops->close(sunxi_panel);
	sunxi_irq_free(hw_res);
	return true;
}

bool sunxi_hdmi_enable(void *data)
{
	struct sunxi_drm_connector *sunxi_connector =
				to_sunxi_connector(data);
	struct sunxi_panel *sunxi_panel = sunxi_connector->panel;
	sunxi_panel->panel_ops->open(sunxi_panel);
	return true;
}

bool sunxi_hdmi_reset(void *data)
{
	struct drm_connector *connector = (struct drm_connector *)data;
	struct sunxi_drm_connector *sunxi_connector =
					to_sunxi_connector(data);
	struct sunxi_drm_encoder *sunxi_encoder;
	struct sunxi_drm_crtc *sunxi_crtc;
	if (!connector || !connector->encoder || !connector->encoder->crtc) {
		return false;
	}

	sunxi_encoder = to_sunxi_encoder(connector->encoder);
	sunxi_crtc = to_sunxi_crtc(connector->encoder->crtc);
	sunxi_clk_set(sunxi_connector->hw_res);
	sunxi_clk_enable(sunxi_connector->hw_res);
	return true;
}

bool sunxi_hdmi_init(void *data)
{
	struct sunxi_drm_connector *sunxi_connector =
			to_sunxi_connector(data);
	struct sunxi_hdmi_private *sunxi_hdmi_p;
	hdmi_bsp_func hdmi_func;
	struct sunxi_panel *sunxi_panel;
	sunxi_panel = sunxi_connector->panel;
	sunxi_hdmi_p = (struct sunxi_hdmi_private *)sunxi_panel->private;

	hdmi_func.delay_ms = hdmi_delay_ms;
	hdmi_func.delay_us = hdmi_delay_us;
	bsp_hdmi_set_func(&hdmi_func);
	sunxi_hdmi_get_sys_info(sunxi_connector->panel);
	/* hdmi must all open */
	sunxi_connector->hw_res->clk_rate = 
	sunxi_hdmi_p->timing->pixel_clk;
	sunxi_drm_sys_power_enable(sunxi_hdmi_p->hdmi_power);

	sunxi_clk_set(sunxi_connector->hw_res);
	sunxi_clk_enable(sunxi_connector->hw_res);
	clk_prepare_enable(sunxi_panel->clk);
	msleep(200);
	bsp_hdmi_set_version(0);
#if defined(HDMI_USING_INNER_BIAS)
	bsp_hdmi_set_bias_source(HDMI_USING_INNER_BIAS);
#endif
	bsp_hdmi_init();
	bsp_hdmi_hrst();
	bsp_hdmi_standby();
	msleep(200);
	return true;
}

void sunxi_hdmi_delayed_work(void *data)
{
	/* get cec msg */
	unsigned char msg;
	bsp_hdmi_cec_get_simple_msg(&msg);
}

static struct sunxi_hardware_ops hdmi_con_ops = {
	.init = sunxi_hdmi_init,
	.reset = sunxi_hdmi_reset,
	.enable = sunxi_hdmi_enable,
	.disable = sunxi_hdmi_disable,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.irq_query = NULL,
	/* whether check the cec ctl info in vysnc delayed work?
	* or other place better?. 
	*/
	.vsync_delayed_do = sunxi_hdmi_delayed_work,
	.set_timming = sunxi_hdmi_set_timing,

};

static int sunxi_hdmi_hwres_init(struct sunxi_hardware_res *hw_res,
	struct sunxi_panel *sunxi_panel, struct device_node *hdmi_node)
{
	hw_res->reg_base = (uintptr_t __force)of_iomap(hdmi_node, 0);
	if (0 ==  hw_res->reg_base) {
		DRM_ERROR("unable to map hdmi registers\n");
		return -EINVAL;
	}
	bsp_hdmi_set_addr(hw_res->reg_base);
#if defined(CONFIG_COMMON_CLK)
	/* get clk */
	hw_res->clk = of_clk_get(hdmi_node, 0);
	if (IS_ERR(hw_res->clk)) {
		DRM_ERROR("fail to get clk for hdmi\n");
		return -EINVAL;
	}
	/* here we use panel clk for ddc clock*/
	sunxi_panel->clk = of_clk_get(hdmi_node, 1);
	if (IS_ERR(sunxi_panel->clk)) {
		DRM_ERROR("fail to get clk for hdmi ddc\n");
		return -EINVAL;
	}
	hw_res->ops = &hdmi_con_ops;
#endif
	return 0;
}

static int sunxi_hdmi_private_init(struct sunxi_panel *sunxi_panel, int hdmi_id)
{
	struct sunxi_hdmi_private *sunxi_hdmi_p;
	sunxi_hdmi_p = kzalloc(sizeof( struct sunxi_hdmi_private), GFP_KERNEL);
	if (!sunxi_hdmi_p) {
		DRM_ERROR("failed to allocate sunxi_hdmi_private.\n");
		return -EINVAL;
	}
	sunxi_hdmi_p->timing = kzalloc(sizeof( struct disp_video_timings), GFP_KERNEL);
	if (!sunxi_hdmi_p->timing) {
		DRM_ERROR("failed to allocate disp_video_timings.\n");
		kfree(sunxi_hdmi_p);
		return -EINVAL;
	}
	sunxi_panel->private = sunxi_hdmi_p;
	sunxi_hdmi_p->hdmi_id = hdmi_id;

	return 0;
}

void sunxi_hdmi_private_destroy(struct sunxi_hdmi_private *sunxi_hdmi_p)
{
	kfree(sunxi_hdmi_p->timing);
	kfree(sunxi_hdmi_p);
}

static int inline sunxi_hdmi_panel_ops_init(struct sunxi_panel *sunxi_panel)
{
	sunxi_panel->panel_ops = &default_panel;
	if (sunxi_panel->panel_ops->init) {
		if(!sunxi_panel->panel_ops->init(sunxi_panel))
			return -EINVAL;
	}
	return 0;
}

struct sunxi_panel *sunxi_hdmi_pan_init(struct sunxi_hardware_res *hw_res, int pan_id, int hdmi_id)
{
	char primary_key[20];
	int value,ret;
	struct sunxi_panel *sunxi_panel = NULL;
	struct device_node *node;

	sprintf(primary_key, "sunxi-hdmi");
	node = sunxi_drm_get_name_node(primary_key);
	if (!node) {
		DRM_ERROR("get device [%s] node fail.\n", primary_key);
		return NULL;
	}

	sunxi_panel = sunxi_panel_creat(value, pan_id);
	if (!sunxi_panel) {
		DRM_ERROR("creat sunxi panel fail.\n");
		goto err_false;
	}

	ret = sunxi_hdmi_hwres_init(hw_res, sunxi_panel, node);
	if (ret) {
		DRM_ERROR("creat hw_res fail.\n");
		sunxi_panel_destroy(sunxi_panel);
		goto err_false;
	}

	ret = sunxi_hdmi_private_init(sunxi_panel, hdmi_id);
	if (ret) {
		DRM_ERROR("creat hw_res fail.\n");
		sunxi_panel_destroy(sunxi_panel);
		goto err_false;
	} 

	ret = sunxi_hdmi_panel_ops_init(sunxi_panel);
	if (ret) {
		sunxi_panel_destroy(sunxi_panel);
		DRM_ERROR("creat panel_ops fail.\n");
		goto err_false;
	}
	return sunxi_panel;
err_false:
	return NULL;
}

void sunxi_hdmi_pan_destroy(struct sunxi_panel *sunxi_panel,
            struct sunxi_hardware_res *hw_res)
{
	struct sunxi_hdmi_private *sunxi_hdmi_p;
	sunxi_hdmi_p = (struct sunxi_hdmi_private *)sunxi_panel->private;
	sunxi_hdmi_private_destroy(sunxi_hdmi_p);
	sunxi_hwres_destroy(hw_res);
}

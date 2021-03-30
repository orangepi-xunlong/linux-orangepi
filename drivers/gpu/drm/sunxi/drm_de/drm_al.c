#include "de_feat.h"
#include "drm_al.h"
#include "sunxi_drm_core.h"
#include "sunxi_drm_panel.h"
#include "subdev/sunxi_common.h"
#include "sunxi_drm_crtc.h"
#include <drm/drmP.h>


void sunxi_drm_crtc_clk_disable(int nr)
{
	de_clk_disable(DE_CLK_CORE0 + nr);
}

void sunxi_drm_crtc_clk_enable(int nr)
{
	de_clk_enable(DE_CLK_CORE0 + nr);
}

int sunxi_drm_get_max_crtc(void)
{
	/* return the DE count */
	return de_feat_get_num_screens();
}

int sunxi_drm_get_max_connector(void)
{
	return de_feat_get_num_devices();
}

int sunxi_drm_get_max_encoder(void)
{
	/* return the tcon count*/
	return de_feat_get_num_devices();
}

int sunxi_drm_get_vi_pipe_by_crtc(int crtc)
{
	return de_feat_get_num_vi_chns(crtc);
}

int sunxi_drm_get_num_chns_by_crtc(int nr)
{
	return de_feat_get_num_chns(nr);
}

int sunxi_drm_encoder_support(int encoder, unsigned int output_type)
{
	return de_feat_is_supported_output_types(encoder, output_type);
}

void sunxi_drm_crtc_updata_fps(int crtc, int fps)
{
	de_update_device_fps(crtc, fps);
}

int sunxi_drm_get_crtc_pipe_plane(int crtc, int channel)
{
	return de_feat_get_num_layers_by_chn(crtc, channel);
}

int sunxi_drm_get_plane_by_crtc(int crtc)
{
	return de_feat_get_num_layers(crtc);
}

int sunxi_drm_updata_reg(int crtc)
{
	return de_al_mgr_update_regs(crtc);
}

int sunxi_drm_sync_reg(int crtc)
{
	return de_rtmx_set_dbuff_rdy(crtc);
}

int sunxi_drm_apply_cache(int crtc, struct disp_manager_data *data)
{
	return de_al_mgr_apply(crtc, data);
}

void sunxi_drm_updata_crtc(struct sunxi_drm_crtc *sunxi_crtc,
	struct sunxi_drm_connector *sunxi_connector)
{
#ifdef CONFIG_ARCH_SUN8IW11
	sunxi_connector = sunxi_connector;
	de_al_lyr_apply(sunxi_crtc->crtc_id, sunxi_crtc->plane_cfgs,
		sunxi_crtc->plane_of_de, false);
#endif
#ifdef CONFIG_ARCH_SUN50IW1P1
	de_al_lyr_apply(sunxi_crtc->crtc_id, sunxi_crtc->plane_cfgs,
		sunxi_crtc->plane_of_de, sunxi_connector->disp_out_type);
#endif
#ifdef CONFIG_ARCH_SUN50IW2P1
	sunxi_connector = sunxi_connector;
	de_al_lyr_apply(sunxi_crtc->crtc_id, sunxi_crtc->plane_cfgs,
		sunxi_crtc->plane_of_de, false);
#endif
}

void sunxi_updata_crtc_freq(unsigned long rate)
{
#ifdef CONFIG_ARCH_SUN8IW11
	de_update_clk_rate(rate);
#endif
#ifdef CONFIG_ARCH_SUN50IW1P1
	de_update_de_frequency(rate);
#endif
#ifdef CONFIG_ARCH_SUN50IW2P1
	de_update_clk_rate(rate);
#endif
}

bool sunxi_drm_init_al(disp_bsp_init_para *para)
{
	int i;
	de_feat_init(); // h5-sunxi50w2p1 need it to initialize de_cur_features
	de_al_init(para);
	de_enhance_init(para);
	de_ccsc_init(para);
	de_dcsc_init(para);
#ifdef CONFIG_ARCH_SUN8IW11
	wb_ebios_init(para);
#endif
#ifdef CONFIG_ARCH_SUN50IW1P1
	WB_EBIOS_Init(para);
#endif
#ifdef CONFIG_ARCH_SUN50IW2P1
	wb_ebios_init(para);
#endif
	de_clk_set_reg_base(para->reg_base[DISP_MOD_DE]);

	for (i = 0; i < DEVICE_NUM; i++) {
		tcon_set_reg_base(i, para->reg_base[DISP_MOD_LCD0 + i]);//calc lcd1 base
		de_smbl_init(i, para->reg_base[DISP_MOD_DE]);
	}
#if defined(HAVE_DEVICE_COMMON_MODULE)
	tcon_top_set_reg_base(0, para->reg_base[DISP_MOD_DEVICE]);
#endif

#ifdef SUPPORT_DSI
	dsi_set_reg_base(0, para->reg_base[DISP_MOD_DSI0]);
#endif

	return true;
}

int bsp_disp_get_print_level(void)
{
	return 0;
}

int disp_delay_us(u32 us)
{
	udelay(us);
	return 0;
}

s32 disp_delay_ms(u32 ms)
{
	sunxi_drm_delayed_ms(ms);
	return 0;
}

int disp_sys_script_get_item(char *main_name, char *sub_name, int value[], int type)
{
	struct device_node *node;
	char compat[32];
	u32 len = 0;

	len = sprintf(compat, "sunxi-%s", main_name);
	node = sunxi_drm_get_name_node(compat);
	if (!node) {
		DRM_ERROR("get [%s] item err.\n", main_name);
		return -EINVAL;
	}
	switch (type) {
	case 1:
		if(sunxi_drm_get_sys_item_int(node, sub_name, value))
			return 0;
		return type;
	case 2:
		if(sunxi_drm_get_sys_item_char(node, sub_name, (char *)value))
			return 0;
		return type;
	case 3:
		if(sunxi_drm_get_sys_item_gpio(node, sub_name, (disp_gpio_set_t *)value))
			return 0;
		return type;
	default:
		return 0;
	}
}

int disp_checkout_straight(unsigned int disp, struct disp_layer_config_data *data)
{
	unsigned char i,chn,vi_chn,device_type;
	struct disp_layer_config_data * pdata;
	u32 num_layers = sunxi_drm_get_plane_by_crtc(disp);
	u32 num_layers_video_by_chn[4] = {0};
	u32 num_layers_video = 0;
	u32 index = 0;
	chn = sunxi_drm_get_num_chns_by_crtc(disp);

	vi_chn = sunxi_drm_get_vi_pipe_by_crtc(disp);
	for (i = 0; i < vi_chn; i++) {
		num_layers_video_by_chn[i] = sunxi_drm_get_crtc_pipe_plane(disp, i);
	}

	for (i = 0; i < vi_chn; i++) {
		num_layers_video += sunxi_drm_get_crtc_pipe_plane(disp, i);
	}

	pdata = data;

	device_type = get_sunxi_crtc_out_type(disp);


	if(device_type == DISP_OUTPUT_TYPE_TV)
	{
		int j;
		for (j = 0; j < vi_chn; j++) {
			for(i = 0; i < num_layers_video_by_chn[j]; i++) {
				if(pdata->config.enable &&
					pdata->config.info.fb.format >= DISP_FORMAT_YUV444_I_AYUV) {
					pdata = data + num_layers_video_by_chn[j] + index;
					index += num_layers_video_by_chn[j];
					break;
				}
				pdata++;
			}
		}

		if (index < num_layers_video) {
			return -1;
		}
		for (i = index; i < num_layers; i++) {
			if(pdata->config.enable) {
				index++;
				break;
			}
			pdata++;
		}
		if (index > num_layers_video) {
			return -1;
		}
	}else
		return -1;

	return 0;
}


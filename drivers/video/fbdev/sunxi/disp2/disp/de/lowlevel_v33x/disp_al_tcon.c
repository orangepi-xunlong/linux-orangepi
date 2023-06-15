/*
 * drivers/video/fbdev/sunxi/disp2/disp/de/lowlevel_v33x/disp_al_tcon/disp_al_tcon.c
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#include "disp_al_tcon.h"


/*
 * disp_al_private_data - abstract layer private data
 * @output_type: current output type of specified device(eg. LCD/HDMI/TV)
 * @output_mode: current output mode of specified device(eg. 720P,1080P)
 * @output_format: current output color format of specified device
 * @output_color_space: current output color space of specified device
 * @tcon_id: the id of device connect to the specified de
 * @de_id: the id of de connect to the specified tcon
 * @disp_size: the output size of specified de
 * @output_fps: the output fps of specified device
 * @tcon_type: tcon type, 0: tcon0(drive panel), 1: tcon1(general drive HDMI/TV)
 */
struct disp_al_private_data {
	u32 output_type[DEVICE_NUM];
	/* u32 output_mode[DEVICE_NUM]; */
	u32 output_format[DEVICE_NUM];
	/* u32 output_color_space[DEVICE_NUM]; */
	/* u32 output_color_range[DEVICE_NUM]; */
	/* u32 output_eotf[DEVICE_NUM]; */
	/* u32 tcon_id[DE_NUM]; */
	u32 de_id[DEVICE_NUM];
	/* struct disp_rect disp_size[DE_NUM]; */
	/* u32 output_fps[DEVICE_NUM]; */
	u32 tcon_type[DEVICE_NUM];
	/* u32 de_backcolor[DE_NUM]; */
	/* bool direct_show[DE_NUM]; */
	/* bool direct_show_toggle[DE_NUM]; */
	u32 de_use_rcq[DEVICE_NUM];
};

static struct disp_al_private_data al_priv;

/* lcd */
/* lcd_dclk_freq * div -> lcd_clk_freq * div2 -> pll_freq */
/* lcd_dclk_freq * dsi_div -> lcd_dsi_freq */
int disp_al_lcd_get_clk_info(u32 screen_id,
	struct lcd_clk_info *info, struct disp_panel_para *panel)
{
	int tcon_div = 6, lcd_div = 1, dsi_div = 4, dsi_rate = 0, find = 0,
	    i = 0;
	/* tcon_div >= 6 when
	 * lcd_dev[sel]->tcon0_dclk.bits.tcon0_dclk_en = 0xf
	 * */

#if defined(CONFIG_FPGA_V7_PLATFORM) || defined(CONFIG_FPGA_V4_PLATFORM)
	struct lcd_clk_info clk_tbl[] = {
		{LCD_IF_HV, 0x12, 1, 1, 0},
		{LCD_IF_CPU, 12, 1, 1, 0},
		{LCD_IF_LVDS, 7, 1, 1, 0},
#if defined (DSI_VERSION_28)
		{LCD_IF_DSI, 4, 1, 4, 0},
#else
		{LCD_IF_DSI, 4, 1, 4, 148500000},
#endif /*endif DSI_VERSION_28 */
	};
#else
	static struct lcd_clk_info clk_tbl[] = {
		{LCD_IF_HV, 6, 1, 1, 0},
		{LCD_IF_CPU, 12, 1, 1, 0},
		{LCD_IF_LVDS, 7, 1, 1, 0},
#if defined (DSI_VERSION_28)
		{LCD_IF_DSI, 4, 1, 4, 0},
#else
		{LCD_IF_DSI, 4, 1, 4, 148500000},
#endif /*endif DSI_VERSION_28 */
	};
#endif


	if (panel == NULL) {
		__wrn("panel is NULL\n");
		return -1;
	}

	memset(info, 0, sizeof(struct lcd_clk_info));
	for (i = 0; i < sizeof(clk_tbl) / sizeof(struct lcd_clk_info); i++) {
		if (clk_tbl[i].lcd_if == panel->lcd_if) {
			tcon_div = clk_tbl[i].tcon_div;
			lcd_div = clk_tbl[i].lcd_div;
			dsi_div = clk_tbl[i].dsi_div;
			dsi_rate = clk_tbl[i].dsi_rate;
			find = 1;
			break;
		}
	}

#if !defined(DSI_VERSION_28)
	if (panel->lcd_if == LCD_IF_DSI) {
		u32 lane = panel->lcd_dsi_lane;
		u32 bitwidth = 0;

		switch (panel->lcd_dsi_format) {
		case LCD_DSI_FORMAT_RGB888:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB666:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB565:
			bitwidth = 16;
			break;
		case LCD_DSI_FORMAT_RGB666P:
			bitwidth = 18;
			break;
		}

		dsi_div = bitwidth / lane;
		if (panel->lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE) {
			tcon_div = dsi_div;
		}
	}
#endif /*endif DSI_VERSION_28 */

	if (find == 0)
		__wrn("cant find clk info for lcd_if %d\n", panel->lcd_if);
	if (panel->lcd_if == LCD_IF_HV &&
	    panel->lcd_hv_if == LCD_HV_IF_CCIR656_2CYC &&
	    panel->ccir_clk_div > 0)
		tcon_div = panel->ccir_clk_div;
	else if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
		 panel->lcd_if == LCD_IF_DSI) {
		tcon_div = tcon_div / 2;
		dsi_div /= 2;
	}

#if defined(DSI_VERSION_28)
	if (panel->lcd_if == LCD_IF_DSI &&
	    panel->lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE) {
		tcon_div = 6;
		dsi_div = 6;
	}
#endif

	info->tcon_div = tcon_div;
	info->lcd_div = lcd_div;
	info->dsi_div = dsi_div;
	info->dsi_rate = dsi_rate;

	return 0;
}

int disp_al_lcd_cfg(u32 screen_id, struct disp_panel_para *panel,
		    struct panel_extend_para *extend_panel)
{
	struct lcd_clk_info info;

	memset(&info, 0, sizeof(struct lcd_clk_info));

	tcon_init(screen_id);
	disp_al_lcd_get_clk_info(screen_id, &info, panel);
	tcon0_set_dclk_div(screen_id, info.tcon_div);

#if !defined(TCON1_DRIVE_PANEL)
	al_priv.tcon_type[screen_id] = 0;
	if (tcon0_cfg(screen_id, panel, al_priv.de_use_rcq[screen_id]) != 0)
		DE_WRN("lcd cfg fail!\n");
	else
		DE_INF("lcd cfg ok!\n");

	tcon0_cfg_ext(screen_id, extend_panel);
	tcon0_src_select(screen_id, LCD_SRC_DE, al_priv.de_id[screen_id]);

	if (panel->lcd_if == LCD_IF_DSI)	{
#if defined(SUPPORT_DSI)
		if (panel->lcd_if == LCD_IF_DSI) {
			if (0 != dsi_cfg(screen_id, panel))
				DE_WRN("dsi %d cfg fail!\n", screen_id);
			if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
			    screen_id + 1 < DEVICE_DSI_NUM) {
				if (0 != dsi_cfg(screen_id + 1, panel))
					DE_WRN("dsi %d cfg fail!\n",
					       screen_id + 1);
			}
		}
#endif
	}
#else

	/* There is no tcon0 on this platform,
	 * At fpga period, we can use tcon1 to driver lcd pnael,
	 * so, here we need to config tcon1 here.
	 */
	al_priv.tcon_type[screen_id] = 1;
	if (tcon1_cfg_ex(screen_id, panel) != 0)
		DE_WRN("lcd cfg fail!\n");
	else
		DE_INF("lcd cfg ok!\n");
#endif
	return 0;
}

int disp_al_lcd_cfg_ext(u32 screen_id, struct panel_extend_para *extend_panel)
{
	tcon0_cfg_ext(screen_id, extend_panel);

	return 0;
}

int disp_al_lcd_enable(u32 screen_id, struct disp_panel_para *panel)
{
#if !defined(TCON1_DRIVE_PANEL)

	tcon0_open(screen_id, panel);
	if (panel->lcd_if == LCD_IF_LVDS) {
		lvds_open(screen_id, panel);
	} else if (panel->lcd_if == LCD_IF_DSI) {
#if defined(SUPPORT_DSI)
		dsi_open(screen_id, panel);
		if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
		    screen_id + 1 < DEVICE_DSI_NUM)
			dsi_open(screen_id + 1, panel);
#endif
	}

#else
	/* There is no tcon0 on this platform,
	 * At fpga period, we can use tcon1 to driver lcd pnael,
	 * so, here we need to open tcon1 here.
	 */
	tcon1_open(screen_id);
#endif

	return 0;
}

int disp_al_lcd_disable(u32 screen_id, struct disp_panel_para *panel)
{

#if !defined(TCON1_DRIVE_PANEL)

	if (panel->lcd_if == LCD_IF_LVDS) {
		lvds_close(screen_id);
	} else if (panel->lcd_if == LCD_IF_DSI) {
#if defined(SUPPORT_DSI)
		dsi_close(screen_id);
		if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
		    screen_id + 1 < DEVICE_DSI_NUM)
			dsi_close(screen_id + 1);
#endif
	}
	tcon0_close(screen_id);
#else
	/* There is no tcon0 on platform sun50iw2,
	 * on fpga period, we can use tcon1 to driver lcd pnael,
	 * so, here we need to close tcon1.
	 */
	tcon1_close(screen_id);
#endif
	tcon_exit(screen_id);

	return 0;
}

/* query lcd irq, clear it when the irq queried exist
 */
int disp_al_lcd_query_irq(u32 screen_id, enum __lcd_irq_id_t irq_id,
			  struct disp_panel_para *panel)
{
#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI &&
	    panel->lcd_dsi_if != LCD_DSI_IF_COMMAND_MODE) {
		enum __dsi_irq_id_t dsi_irq = (irq_id == LCD_IRQ_TCON0_VBLK)
						  ? DSI_IRQ_VIDEO_VBLK
						  : DSI_IRQ_VIDEO_LINE;

		return dsi_irq_query(screen_id, dsi_irq);
	} else
#endif
	return tcon_irq_query(screen_id,
			      (al_priv.tcon_type[screen_id] == 0) ?
			      irq_id : LCD_IRQ_TCON1_VBLK);
}

int disp_al_lcd_enable_irq(u32 screen_id, enum __lcd_irq_id_t irq_id,
			   struct disp_panel_para *panel)
{
	int ret = 0;

#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI) {
		enum __dsi_irq_id_t dsi_irq =
		    (irq_id == LCD_IRQ_TCON0_VBLK) ?
		    DSI_IRQ_VIDEO_VBLK : DSI_IRQ_VIDEO_LINE;

		ret = dsi_irq_enable(screen_id, dsi_irq);
	}
#endif
	ret =
	    tcon_irq_enable(screen_id,
			    (al_priv.tcon_type[screen_id] == 0) ?
			    irq_id : LCD_IRQ_TCON1_VBLK);

	return ret;
}

int disp_al_lcd_disable_irq(u32 screen_id, enum __lcd_irq_id_t irq_id,
			    struct disp_panel_para *panel)
{
	int ret = 0;

#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI) {
		enum __dsi_irq_id_t dsi_irq =
		    (irq_id == LCD_IRQ_TCON0_VBLK) ?
		    DSI_IRQ_VIDEO_VBLK : DSI_IRQ_VIDEO_LINE;

		ret = dsi_irq_disable(screen_id, dsi_irq);
	}
#endif
	ret =
	    tcon_irq_disable(screen_id,
			     (al_priv.tcon_type[screen_id] ==
			      0) ? irq_id : LCD_IRQ_TCON1_VBLK);

	return ret;
}

int disp_al_lcd_tri_busy(u32 screen_id, struct disp_panel_para *panel)
{
	int busy = 0;
	int ret = 0;

	busy |= tcon0_tri_busy(screen_id);
#if defined(SUPPORT_DSI)
	busy |= dsi_inst_busy(screen_id);
#endif
	ret = (busy == 0) ? 0 : 1;

	return ret;
}

/* take dsi irq s32o account, todo? */
int disp_al_lcd_tri_start(u32 screen_id, struct disp_panel_para *panel)
{
#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI)
		dsi_tri_start(screen_id);
#endif
	return tcon0_tri_start(screen_id);
}

int disp_al_lcd_io_cfg(u32 screen_id, u32 enable, struct disp_panel_para *panel)
{
#if defined(SUPPORT_DSI)
	if (panel->lcd_if == LCD_IF_DSI) {
		if (enable == 1) {
			dsi_io_open(screen_id, panel);
			if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
			    screen_id + 1 < DEVICE_DSI_NUM)
				dsi_io_open(screen_id + 1, panel);
		} else {
			dsi_io_close(screen_id);
			if (panel->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
			    screen_id + 1 < DEVICE_DSI_NUM)
				dsi_io_close(screen_id + 1);
		}
	}
#endif

	return 0;
}

int disp_al_lcd_get_cur_line(u32 screen_id, struct disp_panel_para *panel)
{
#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI)
		return dsi_get_cur_line(screen_id);
#endif
	return tcon_get_cur_line(screen_id,
				 al_priv.tcon_type[screen_id]);
}

int disp_al_lcd_get_start_delay(u32 screen_id, struct disp_panel_para *panel)
{
#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	u32 lcd_start_delay = 0;
	u32 de_clk_rate = de_get_clk_rate() / 1000000;
	if (panel && panel->lcd_if == LCD_IF_DSI) {
		lcd_start_delay =
		    ((tcon0_get_cpu_tri2_start_delay(screen_id) + 1) << 3) *
		    (panel->lcd_dclk_freq) / (panel->lcd_ht * de_clk_rate);
		return dsi_get_start_delay(screen_id) + lcd_start_delay;
	} else
#endif
	return tcon_get_start_delay(screen_id,
				    al_priv.tcon_type[screen_id]);
}

/* hdmi */
int disp_al_hdmi_pad_sel(u32 screen_id, u32 pad)
{
	tcon_pan_sel(screen_id, pad);

	return 0;
}

int disp_al_hdmi_enable(u32 screen_id)
{
	tcon1_hdmi_clk_enable(screen_id, 1);

	tcon1_open(screen_id);
	return 0;
}

int disp_al_hdmi_disable(u32 screen_id)
{
	tcon1_close(screen_id);
	tcon_exit(screen_id);
	tcon1_hdmi_clk_enable(screen_id, 0);

	return 0;
}

int disp_al_hdmi_set_output_format(
	u32 screen_id, u32 output_format)
{
	al_priv.output_format[screen_id] = output_format;
	return 0;
}

int disp_al_hdmi_cfg(u32 screen_id, struct disp_video_timings *video_info)
{
	struct disp_video_timings *timings = NULL;

	al_priv.tcon_type[screen_id] = 1;
	al_priv.output_format[screen_id]
		= bsp_disp_hdmi_get_color_format();

	timings = kmalloc(sizeof(struct disp_video_timings),
			  GFP_KERNEL | __GFP_ZERO);
	if (timings) {
		memcpy(timings, video_info, sizeof(struct disp_video_timings));
		if (al_priv.output_format[screen_id] == DISP_CSC_TYPE_YUV420) {
			timings->x_res /= 2;
			timings->hor_total_time /= 2;
			timings->hor_back_porch /= 2;
			timings->hor_front_porch /= 2;
			timings->hor_sync_time /= 2;
		}
	} else {
		__wrn("malloc memory for timings fail! size=0x%x\n",
		      (unsigned int)sizeof(struct disp_video_timings));
	}
	tcon_init(screen_id);

	if (timings)
		tcon1_set_timming(screen_id, timings);
	else
		tcon1_set_timming(screen_id, video_info);
	tcon1_src_select(screen_id, LCD_SRC_DE, al_priv.de_id[screen_id]);
	tcon1_black_src(screen_id, 0, al_priv.output_format[screen_id]);

	kfree(timings);

	return 0;
}

int disp_al_hdmi_irq_enable(u32 screen_id)
{
	tcon_irq_enable(screen_id, LCD_IRQ_TCON1_VBLK);
	return 0;
}

int disp_al_hdmi_irq_disable(u32 screen_id)
{
	tcon_irq_disable(screen_id, LCD_IRQ_TCON1_VBLK);
	return 0;
}

/* tv */
int disp_al_tv_enable(u32 screen_id)
{
	tcon1_tv_clk_enable(screen_id, 1);
	tcon1_open(screen_id);

	return 0;
}

int disp_al_tv_disable(u32 screen_id)
{
	tcon1_close(screen_id);
	tcon_exit(screen_id);
	tcon1_tv_clk_enable(screen_id, 0);

	return 0;
}

#if defined(SUPPORT_EDP)
int disp_al_edp_cfg(u32 screen_id, u32 fps, u32 edp_index)
{
	al_priv.tcon_type[screen_id] = 0;
	edp_de_attach(edp_index, al_priv.de_id[screen_id]);
	return 0;
}

int disp_al_edp_disable(u32 screen_id)
{
	return 0;
}
#endif

int disp_al_tv_cfg(u32 screen_id, struct disp_video_timings *video_info)
{
	al_priv.tcon_type[screen_id] = 1;

	tcon_init(screen_id);
/*#if defined(HAVE_DEVICE_COMMON_MODULE)*/
	/*rgb_src_sel(screen_id);*/
/*#endif*/
	tcon1_set_timming(screen_id, video_info);
#if defined (CONFIG_ARCH_SUN50IW9)
	tcon1_yuv_range(screen_id, 0);
#else
	tcon1_yuv_range(screen_id, 1);
#endif
	tcon1_src_select(screen_id, LCD_SRC_DE, al_priv.de_id[screen_id]);

	return 0;
}

int disp_al_tv_irq_enable(u32 screen_id)
{
	tcon_irq_enable(screen_id, LCD_IRQ_TCON1_VBLK);

	return 0;
}

int disp_al_tv_irq_disable(u32 screen_id)
{
	tcon_irq_disable(screen_id, LCD_IRQ_TCON1_VBLK);

	return 0;
}

#if defined(SUPPORT_VGA)
/* vga interface
 */
int disp_al_vga_enable(u32 screen_id)
{
	tcon1_open(screen_id);

	return 0;
}

int disp_al_vga_disable(u32 screen_id)
{
	tcon1_close(screen_id);
	tcon_exit(screen_id);

	return 0;
}

int disp_al_vga_cfg(u32 screen_id, struct disp_video_timings *video_info)
{
	al_priv.tcon_type[screen_id] = 1;

	tcon_init(screen_id);
	tcon1_set_timming(screen_id, video_info);
	tcon1_src_select(screen_id, LCD_SRC_DE, al_priv.de_id[screen_id]);

	return 0;
}

int disp_al_vga_irq_enable(u32 screen_id)
{
	tcon_irq_enable(screen_id, LCD_IRQ_TCON1_VBLK);

	return 0;
}

int disp_al_vga_irq_disable(u32 screen_id)
{
	tcon_irq_disable(screen_id, LCD_IRQ_TCON1_VBLK);

	return 0;
}
#endif

int disp_al_vdevice_cfg(u32 screen_id, struct disp_video_timings *video_info,
			struct disp_vdevice_interface_para *para,
			u8 config_tcon_only)
{
	struct lcd_clk_info clk_info;
	struct disp_panel_para info;

	al_priv.tcon_type[screen_id] = 0;

	memset(&info, 0, sizeof(struct disp_panel_para));
	info.lcd_if = para->intf;
	info.lcd_x = video_info->x_res;
	info.lcd_y = video_info->y_res;
	info.lcd_hv_if = (enum disp_lcd_hv_if) para->sub_intf;
	info.lcd_dclk_freq = video_info->pixel_clk;
	info.lcd_ht = video_info->hor_total_time;
	info.lcd_hbp = video_info->hor_back_porch + video_info->hor_sync_time;
	info.lcd_hspw = video_info->hor_sync_time;
	info.lcd_vt = video_info->ver_total_time;
	info.lcd_vbp = video_info->ver_back_porch + video_info->ver_sync_time;
	info.lcd_vspw = video_info->ver_sync_time;
	info.lcd_interlace = video_info->b_interlace;
	info.lcd_hv_syuv_fdly = para->fdelay;
	info.lcd_hv_clk_phase = para->clk_phase;
	info.lcd_hv_sync_polarity = para->sync_polarity;
	info.ccir_clk_div = para->ccir_clk_div;
	info.input_csc = para->input_csc;

	if (info.lcd_hv_if == LCD_HV_IF_CCIR656_2CYC)
		info.lcd_hv_syuv_seq = para->sequence;
	else
		info.lcd_hv_srgb_seq = para->sequence;
	tcon_init(screen_id);
	disp_al_lcd_get_clk_info(screen_id, &clk_info, &info);
	tcon0_set_dclk_div(screen_id, clk_info.tcon_div);

	if (para->sub_intf == LCD_HV_IF_CCIR656_2CYC)
		tcon1_yuv_range(screen_id, 1);
	if (tcon0_cfg(screen_id, &info, al_priv.de_use_rcq[screen_id]) != 0)
		DE_WRN("lcd cfg fail!\n");
	else
		DE_INF("lcd cfg ok!\n");

	if (!config_tcon_only)
		tcon0_src_select(screen_id, LCD_SRC_DE,
				 al_priv.de_id[screen_id]);
	else
		DE_INF("%s:config_tcon_only is %d\n", __func__,
		       config_tcon_only);

	return 0;
}

int disp_al_vdevice_enable(u32 screen_id)
{
	struct disp_panel_para panel;

	memset(&panel, 0, sizeof(struct disp_panel_para));
	panel.lcd_if = LCD_IF_HV;
	tcon0_open(screen_id, &panel);

	return 0;
}

int disp_al_vdevice_disable(u32 screen_id)
{
	tcon0_close(screen_id);
	tcon_exit(screen_id);

	return 0;
}

/* screen_id: used for index of manager */
int disp_al_device_get_cur_line(u32 screen_id)
{
	u32 tcon_type = al_priv.tcon_type[screen_id];

	return tcon_get_cur_line(screen_id, tcon_type);
}

int disp_al_device_get_start_delay(u32 screen_id)
{
	u32 tcon_type = al_priv.tcon_type[screen_id];

	tcon_type = (al_priv.tcon_type[screen_id] == 0) ? 0 : 1;
	return tcon_get_start_delay(screen_id, tcon_type);
}

int disp_al_device_query_irq(u32 screen_id)
{
	int ret = 0;
	int irq_id = 0;

	irq_id = (al_priv.tcon_type[screen_id] == 0) ?
	    LCD_IRQ_TCON0_VBLK : LCD_IRQ_TCON1_VBLK;
	ret = tcon_irq_query(screen_id, irq_id);

	return ret;
}

int disp_al_device_enable_irq(u32 screen_id)
{
	int ret = 0;
	int irq_id = 0;

	irq_id = (al_priv.tcon_type[screen_id] == 0) ?
	    LCD_IRQ_TCON0_VBLK : LCD_IRQ_TCON1_VBLK;
	ret = tcon_irq_enable(screen_id, irq_id);

	return ret;
}

int disp_al_device_disable_irq(u32 screen_id)
{
	int ret = 0;
	int irq_id = 0;

	irq_id = (al_priv.tcon_type[screen_id] == 0) ?
	    LCD_IRQ_TCON0_VBLK : LCD_IRQ_TCON1_VBLK;
	ret = tcon_irq_disable(screen_id, irq_id);

	return ret;
}

int disp_al_lcd_get_status(u32 screen_id, struct disp_panel_para *panel)
{
	int ret = 0;
#if defined(DSI_VERSION_40)
	if (panel->lcd_if == LCD_IF_DSI)
		ret = dsi_get_status(screen_id);
	else
#endif
		ret = tcon_get_status(screen_id, al_priv.tcon_type[screen_id]);

	return ret;
}

int disp_al_device_get_status(u32 screen_id)
{
	int ret = 0;

	ret = tcon_get_status(screen_id, al_priv.tcon_type[screen_id]);

	return ret;
}

int disp_al_device_src_select(u32 screen_id, u32 src)
{
	int ret = 0;

	return ret;
}

int disp_al_device_set_de_id(u32 screen_id, u32 de_id)
{
	al_priv.de_id[screen_id] = de_id;
	return 0;
}

int disp_al_device_set_de_use_rcq(u32 screen_id, u32 use_rcq)
{
	al_priv.de_use_rcq[screen_id] = use_rcq;
	return 0;
}

int disp_al_device_set_output_type(u32 screen_id, u32 output_type)
{
	al_priv.output_type[screen_id] = output_type;

	al_priv.tcon_type[screen_id] = 0;

#if defined(SUPPORT_HDMI)
	if (output_type == DISP_OUTPUT_TYPE_HDMI) {
		al_priv.tcon_type[screen_id] = 1;
	}
#endif

#if defined(SUPPORT_TV)
	al_priv.tcon_type[screen_id] =
		(output_type == DISP_OUTPUT_TYPE_TV) ?
		1 : al_priv.tcon_type[screen_id];
#if defined(CONFIG_DISP2_TV_AC200)
	al_priv.tcon_type[screen_id] =
		(output_type == DISP_OUTPUT_TYPE_TV) ?
		0 : al_priv.tcon_type[screen_id];
#endif /*endif CONFIG_DISP2_TV_AC200 */
#endif /* defined(SUPPORT_TV) */

	return 0;
}



s32 disp_al_init_tcon(struct disp_bsp_init_para *para)
{
	u32 i;

	for (i = 0; i < DEVICE_NUM; i++) {
		tcon_set_reg_base(i, para->reg_base[DISP_MOD_LCD0 + i]);
	}
#if defined(HAVE_DEVICE_COMMON_MODULE)
	tcon_top_set_reg_base(0, para->reg_base[DISP_MOD_DEVICE]);
#endif

	return 0;
}

void disp_al_show_builtin_patten(u32 hwdev_index, u32 patten)
{
	tcon_show_builtin_patten(hwdev_index, patten);
}

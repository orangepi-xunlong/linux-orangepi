#ifndef _DISP_AL_H_
#define _DISP_AL_H_

#include "./ebios_lcdc_tve.h"
#include "./ebios_de.h"
#include "../bsp_display.h"
#include "../disp_private.h"
#include "./edp_type.h"
#include "./iep_drc.h"

#define MAX_NUM_SCREEN 3

#define DISP2_CTRL0 0xf3000030
#define DISP2_CTRL1 0xf3000034
#define DISP2_CTRL2 0xf3000038
#define DISP2_CTRL3 0xf300003c

typedef struct {
	u32 input_source; //0: dram; 1: fe0; 2:fe1; 3: fe2 ..(1-..: if fe exist)
	u32 out_select;//lcd, fe2
	u32 in_csc, out_csc;//0:rgb; 1:yuv
	u32 color_range;
	u32 in_width, in_height;//input size, eq the out_size of the pre_mod
	u32 out_width, out_height;//output size, eq the in_size of the next mod
	u32 b_out_interlace, b_in_interlace;
	u32 b_de_flicker;
	u32 enabled;
}__disp_al_be_info_t;

typedef struct {
	u32 out_select;//0: be0; 1:be1; 2:be2; 0xff: wb
	u32 in_csc, out_csc;//0:rgb; 1:yuv
	u32 color_range;
	u32 in_width, in_height;//input size, eq the out_size of the pre_mod
	u32 out_width, out_height;//output size, eq the in_size of the next mod
	u32 b_out_interlace, b_in_interlace;
	u32 b_de_flicker;
	u32 b_top_filed_first;
	u32 b_de_interlace;
	u32 enabled;
	u32 close_request;
	disp_scaler_info  scaler_info;

/*
	struct {
		u32 clk;
		u32 clk_src;
		u32 h_clk;
		u32 enabled;
	}clk;
*/
	disp_clk_info_t clk;
	disp_clk_info_t extra_clk;
}__disp_al_fe_info_t;

typedef struct {
    u32 in_width, in_height;
    u32 in_csc, out_csc;        //0:RGB output; 1:UV for tv(range 16-235); 2:UV for HDMI(range 16-235)
    u32 color_range;              //0:16~255, 1:0~255, 2:16~235
    u32 tcon_index;                //0:for panel; 1:for hdmi
    u32 support_4k;
    u32 b_out_interlace, b_in_interlace;
}__disp_al_hdmi_info_t;

typedef struct {
	u32 in_width, in_height;//input size, eq the out_size of the pre_mod
	u32 out_width, out_height;//display size, eq the in_size of the next mod
	u32 in_csc, out_csc;//0:rgb; 1:yuv
	u32 b_out_interlace, b_in_interlace;
	u32 backlight;
	u32 enabled;

	u32 lcd_if;
	u32 tcon_index;//0:for panel; 1:for hdmi
	u32 time_per_line;//unit:0.1us

	u32 edp_rate;
}__disp_al_lcd_info_t;

typedef struct {
	u32 in_width, in_height;//input size, eq the out_size of the pre_mod
	u32 out_width, out_height;//display size, eq the in_size of the next mod
	u32 out_select;//0:be0; 1:be1;
	u32 in_csc, out_csc;//0:rgb; 1:yuv
	u32 b_out_interlace, b_in_interlace;
	u32 enabled;

	u32 luma_sharp_level;
	u32 chroma_sharp_level;
	u32 white_exten_level;
	u32 black_exten_level;
	disp_window window;
	u32 enable;
	u32 dirty;
}__disp_al_deu_info_t;

typedef struct {
	u32 in_width, in_height;//input size, eq the out_size of the pre_mod
	u32 out_width, out_height;//display size, eq the in_size of the next mod
	u32 in_csc, out_csc;
	u32 b_de_flicker;
	u32 b_out_interlace, b_in_interlace;
	u32 backlight,backlight_dimming;
	u32 enabled;

	u32 mode;//0:rgb(UI mode); 1:yuv(video mode)
}__disp_al_drc_info_t;

typedef struct {
	u32 in_width, in_height;//input size, eq the out_size of the pre_mod
	u32 out_width, out_height;//display size, eq the in_size of the next mod
	u32 in_csc, out_csc;//0:rgb; 1:yuv
	u32 b_out_interlace, b_in_interlace;
	u32 mode;//0: screen mode;  1:layer mode
	disp_window window;
	u32 enabled;
}__disp_al_cmu_info_t;

//capture
typedef struct {
	u32 fps_counter;
	u32 switch_buffer_counter;
	disp_capture_para capture_para;
}__disp_al_capture_info_t;

typedef struct
{
	__disp_al_be_info_t   be_info;
	__disp_al_fe_info_t   fe_info;
	__disp_al_lcd_info_t  lcd_info;
	__disp_al_hdmi_info_t hdmi_info;
	__disp_al_deu_info_t  deu_info;
	__disp_al_drc_info_t  drc_info;
	__disp_al_cmu_info_t  cmu_info;
	__disp_al_capture_info_t capture_info;
	u32 output_type;
}__disp_al_private_data;

typedef struct
{
    disp_mod_id mod_id;
    struct list_head list;
}__disp_al_mod_list_t;

typedef struct
{
	disp_mod_id mod_id;
	char *name;
	u32 screen_id;
	s32 (*set_reg_base)(u32 screen_id, u32 reg_base);
	int (*notifier_call)(struct disp_notifier_block *, u32 event, u32 sel, void* data);
}__disp_al_mod_init_data;

typedef struct
{
	disp_pixel_format format;//input format

	/* reg config */
	u32    mod;       //0:plannar; 1: interleaved; 2: plannar uv combined; 4: plannar mb; 6: uv combined mb
	u32    fmt;       //0:yuv444; 1: yuv422; 2: yuv420; 3:yuv411; 4: csi rgb; 5:rgb888
	u32    ps;        //
	u32    br_swap;   //swap blue & red
	u32    byte_seq;
}__de_format_t;

/***********************************************************
 *
 * global
 *
 ***********************************************************/
s32 disp_al_check_csc(u32 screen_id);
s32 disp_al_check_display_size(u32 screen_id);
s32 disp_al_format_to_bpp(disp_pixel_format fmt);
s32 disp_al_query_mod(disp_mod_id mod_id);
s32 disp_al_query_be_mod(u32 screen_id);
s32 disp_al_query_fe_mod(u32 screen_id);
s32 disp_al_query_lcd_mod(u32 screen_id);
s32 disp_al_query_smart_color_mod(u32 screen_id);
s32 disp_al_query_drc_mod(u32 screen_id);
s32 disp_al_query_deu_mod(u32 screen_id);
s32 disp_al_query_dsi_mod(u32 screen_id);
s32 disp_al_query_hdmi_mod(u32 screen_id);
s32 disp_al_query_edp_mod(u32 screen_id);
void disp_al_edp_print(const char *c);
void disp_al_edp_print_val(u32 val);
void disp_al_dsi_print(const char *c);
void disp_al_dsi_print_val(u32 val);
s32 disp_al_de_clk_init(__disp_bsp_init_para * para);
s32 disp_al_de_clk_enable(disp_mod_id mod_id, u32 enable);
void disp_al_disp_config(u32 screen_id, u32 width);

/***********************************************************
 *
 * disp_al_lcd
 *
 ***********************************************************/
s32 disp_al_lcd_clk_init(u32 clk_id);
s32 disp_al_lcd_clk_exit(u32 clk_id);
s32 disp_al_lcd_clk_enable(u32 clk_id);
s32 disp_al_lcd_clk_disable(u32 clk_id);
s32 disp_al_lcd_cfg(u32 screen_id, disp_panel_para * panel);//irq enable mv from open to cfg
s32 disp_al_lcd_init(u32 screen_id);
s32 disp_al_lcd_init_sw(u32 screen_id);
s32 disp_al_lcd_exit(u32 screen_id);
s32 disp_al_lcd_enable(u32 screen_id, u32 enable, disp_panel_para * panel);
s32 disp_al_lcd_set_src(u32 screen_id, disp_lcd_src src);
s32 disp_al_lcd_clk_enable(u32 clk_id);
s32 disp_al_lcd_clk_disable(u32 clk_id);
s32 disp_al_edp_init(u32 screen_id, u32 edp_rate);
s32 disp_al_edp_cfg(u32 screen_id, disp_panel_para * panel);
s32 disp_al_edp_disable_cfg(u32 screen_id);
s32 disp_al_edp_int(__edp_irq_id_t edp_irq);
s32 disp_al_lcd_check_time(u32 screen_id, u32 us);
/* query lcd irq, clear it when the irq queried exist
 * take dsi irq s32o account, todo?
 */
s32 disp_al_lcd_query_irq(u32 screen_id, __lcd_irq_id_t irq_id);
/* take dsi irq s32o account, todo? */
s32 disp_al_lcd_tri_busy(u32 screen_id);
/* take dsi irq s32o account, todo? */
s32 disp_al_lcd_tri_start(u32 screen_id);
s32 disp_al_lcd_io_cfg(u32 screen_id, u32 enabled, disp_panel_para * panel);
s32 disp_al_lcd_get_cur_line(u32 screen_id);
s32 disp_al_lcd_get_start_delay(u32 screen_id);
s32 disp_al_lcd_set_clk_div(u32 screen_id, u32 clk_div);
u32 disp_al_lcd_get_clk_div(u32 screen_id);
/***********************************************************
 *
 * disp_al_hdmi
 *
 ***********************************************************/
u32 disp_al_hdmi_clk_enable(u32 clk_id);
u32 disp_al_hdmi_clk_disable(u32 clk_id);
s32 disp_al_hdmi_init(u32 screen_id, u32 hdmi_4k);
s32 disp_al_hdmi_init_sw(u32 screen_id, u32 hdmi_4k);
s32 disp_al_hdmi_exit(u32 screen_id);
s32 disp_al_hdmi_enable(u32 screen_id);
s32 disp_al_hdmi_enable_sw(u32 screen_id);
s32 disp_al_hdmi_disable(u32 screen_id);
s32 disp_al_hdmi_cfg(u32 screen_id, disp_video_timing *video_info);
s32 disp_al_hdmi_cfg_sw(u32 screen_id, disp_video_timing *video_info);
/***********************************************************
 *
 * disp_al_manager
 *
 ***********************************************************/
/* init reg */
s32 disp_al_manager_init(u32 screen_id);
s32 disp_al_manager_init_sw(u32 screen_id);
s32 disp_al_mnanger_exit(u32 screen_id);
s32 disp_al_manager_clk_init(u32 clk_id);
s32 disp_al_manager_clk_exit(u32 clk_id);
s32 disp_al_manager_clk_enable(u32 clk_id);
s32 disp_al_manager_clk_disable(u32 clk_id);
s32 disp_al_cfg_itl(u32 screen_id,u32 b_out_interlace);
/*
 * take irq en/disable & reg_auto_load s32o account
 */
s32 disp_al_manager_enable(u32 screen_id, u32 enabled);
s32 disp_al_manager_enable_sw(u32 screen_id, u32 enabled);
s32 disp_al_manager_set_backcolor(u32 screen_id, disp_color_info *bk_color);
s32 disp_al_manager_set_color_key(u32 screen_id, disp_colorkey *ck_mode);
s32 disp_al_manager_sync(u32 screen_id);
s32 disp_al_manager_query_irq(u32 screen_id, u32 irq_id);
s32 disp_al_manager_set_display_size(u32 screen_id, u32 width, u32 height);
s32 disp_al_manager_get_display_size(u32 screen_id, u32 *width, u32 *height);

/***********************************************************
 *
 * disp_al_capture
 *
 ***********************************************************/
s32 disp_al_capture_init(u32 screen_id);
s32 disp_al_capture_screen_proc(u32 screen_id);
s32 disp_al_capture_screen_switch_buff(u32 screen_id);
s32 disp_al_capture_screen(u32 screen_id,disp_capture_para * para);
s32 disp_al_capture_screen_stop(u32 screen_id);
s32 disp_al_capture_screen_get_buffer_id(u32 screen_id);
s32 disp_al_capture_sync(u32 screen_id);
s32 disp_al_caputure_screen_finished(u32 screen_id);
/***********************************************************
 *
 * disp_al_layer
 *
 ***********************************************************/
s32 disp_al_layer_set_pipe(u32 screen_id, u32 layer_id, u32 pipe);
s32 disp_al_layer_set_zorder(u32 screen_id, u32 layer_id, u32 zorder);
s32 disp_al_layer_set_alpha_mode(u32 screen_id, u32 layer_id, u32 alpha_mode, u32 alpha_value);
s32 disp_al_layer_color_key_enable(u32 screen_id, u32 layer_id, u32 enabled);
s32 disp_al_layer_set_screen_window(u32 screen_id, u32 layer_id, disp_window * window);
s32 disp_al_layer_set_framebuffer(u32 screen_id, u32 layer_id, disp_fb_info *fb);
s32 disp_al_layer_enable(u32 screen_id, u32 layer_id, u32 enabled);
s32 disp_al_layer_set_extra_info(u32 screen_id, u32 layer_id, disp_layer_info *info, __disp_layer_extra_info_t *extra_info);
s32 disp_al_layer_set_extra_info_sw(u32 screen_id, u32 layer_id, disp_layer_info *info, __disp_layer_extra_info_t *extra_info);
s32 disp_al_layer_sync(u32 screen_id, u32 layer_id, __disp_layer_extra_info_t *extra_info);
s32 disp_al_deinterlace_cfg(u32 scaler_id,disp_scaler_info * scaler);
u32 disp_al_layer_get_addr(u32 sel, u32 hid);
u32 disp_al_layer_set_addr(u32 sel, u32 hid, u32 addr);
u32 disp_al_layer_get_inWidth(u32 sel,u32 hid);
u32 disp_al_layer_get_inHeight(u32 sel,u32 hid);

/* scale */
s32 scaler_init(__disp_bsp_init_para * para);
s32 scaler_exit(void);
s32 scaler_sync_ex(u32 sel);
/***********************************************************
 *
 * disp_al_smcl(smart color)
 *
 ***********************************************************/
s32 disp_al_smcl_clk_init(u32 clk_id);
s32 disp_al_smcl_clk_exit(u32 clk_id);
s32 disp_al_smcl_clk_enable(u32 clk_id, u32 enable);
s32 disp_al_smcl_set_para(u32 screen_id, u32 brightness, u32 saturaion, u32 hue, u32 mode);
s32 disp_al_smcl_set_window(u32 screen_id, disp_window *window);
s32 disp_al_smcl_enable(u32 screen_id, u32 enable);
s32 disp_al_smcl_sync(u32 screen_id);

/***********************************************************
 *
 * disp_al_smbl(smart backlight)
 *
 ***********************************************************/
s32 disp_al_smbl_enable(u32 screen_id, u32 enable);
s32 disp_al_smbl_set_window(u32 screen_id, disp_window *window);
s32 disp_al_smbl_sync(u32 screen_id);
s32 disp_al_smbl_update_backlight(u32 screen_id, u32 bl);
s32 disp_al_smbl_get_backlight_dimming(u32 screen_id);
s32 disp_al_smbl_tasklet(u32 screen_id);
/***********************************************************
 *
 * disp_al_cursor(hardware cursor)
 *
 ***********************************************************/
s32 disp_al_hwc_enable(u32 screen_id, bool enable);
s32 disp_al_hwc_set_pos(u32 screen_id, disp_position *pos);
s32 disp_al_hwc_get_pos(u32 screen_id, disp_position *pos);
s32 disp_al_hwc_set_framebuffer(u32 screen_id, disp_cursor_fb *fb);
s32 disp_al_hwc_set_palette(u32 screen_id, void *palette,u32 offset, u32 palette_size);

/***********************************************************
 *
 * disp_al_deu(detail enhance)
 *
 ***********************************************************/
s32 disp_al_deu_set_window(u32 screen_id, disp_window *window);
s32 disp_al_deu_set_luma_sharp_level(u32 screen_id, u32 level);
s32 disp_al_deu_set_chroma_sharp_level(u32 screen_id, u32 level);
s32 disp_al_deu_set_white_exten_level(u32 screen_id, u32 level);
s32 disp_al_deu_set_black_exten_level(u32 screen_id, u32 level);
s32 disp_al_deu_exit(u32 screen_id);
s32 disp_al_deu_init(__disp_bsp_init_para * para);

/* private */
s32 disp_al_notifier_register(struct disp_notifier_block *nb);
s32 disp_al_notifier_unregister(struct disp_notifier_block *nb);

s32 disp_init_al(__disp_bsp_init_para * para);

#endif

#ifndef _DISP_AL_H_
#define _DISP_AL_H_

#include "../include.h"
#include "de_feat.h"
#include "de_hal.h"
#include "de_enhance.h"
//#include "de_wb.h"
//#include "de_wb_type.h"
#include "de_smbl.h"
#include "de_csc.h"
#include "de_lcd.h"
#include "de_clock.h"
#include "de_rtmx.h"
#include "rtmx_eink.h"
#include "disp_waveform.h"
#include "disp_eink_data.h"

struct lcd_clk_info {
	disp_lcd_if lcd_if;
	int tcon_div;
	int lcd_div;
	int dsi_div;
	int dsi_rate;
};

int disp_al_de_clk_enable(unsigned int disp);
int disp_al_de_clk_disable(unsigned int disp);
int disp_al_manager_init(unsigned int disp);
int disp_al_manager_exit(unsigned int disp);
extern int disp_al_manager_apply(unsigned int disp, struct disp_manager_data *data);
extern int disp_al_layer_apply(unsigned int disp, struct disp_layer_config_data *data, unsigned int layer_num);
extern int disp_init_al(disp_bsp_init_para * para);
extern int disp_al_manager_sync(unsigned int disp);
extern int disp_al_manager_update_regs(unsigned int disp);
int disp_al_manager_query_irq(unsigned int disp);
int disp_al_manager_enable_irq(unsigned int disp);
int disp_al_manager_disable_irq(unsigned int disp);

int disp_al_enhance_apply(unsigned int disp, struct disp_enhance_config *config);
int disp_al_enhance_update_regs(unsigned int disp);
int disp_al_enhance_sync(unsigned int disp);
int disp_al_enhance_tasklet(unsigned int disp);

int disp_al_smbl_apply(unsigned int disp, struct disp_smbl_info *info);
int disp_al_smbl_update_regs(unsigned int disp);
int disp_al_smbl_sync(unsigned int disp);
int disp_al_smbl_get_status(unsigned int disp);
int disp_al_smbl_tasklet(unsigned int disp);

int disp_al_write_back_clk_init(unsigned int disp);
int disp_al_write_back_clk_exit(unsigned int disp);

int disp_al_capture_init(unsigned int disp);
int disp_al_capture_exit(unsigned int disp);
int disp_al_capture_sync(u32 disp);
int disp_al_capture_apply(unsigned int disp, struct disp_capture_config *cfg);
int disp_al_capture_get_status(unsigned int disp);

int disp_al_lcd_cfg(u32 screen_id, disp_panel_para * panel, panel_extend_para *extend_panel);
int disp_al_lcd_enable(u32 screen_id, disp_panel_para * panel);
int disp_al_lcd_disable(u32 screen_id, disp_panel_para * panel);
int disp_al_lcd_query_irq(u32 screen_id, __lcd_irq_id_t irq_id, disp_panel_para * panel);
int disp_al_lcd_tri_busy(u32 screen_id, disp_panel_para * panel);
int disp_al_lcd_tri_start(u32 screen_id, disp_panel_para * panel);
int disp_al_lcd_io_cfg(u32 screen_id, u32 enable, disp_panel_para * panel);
int disp_al_lcd_get_cur_line(u32 screen_id, disp_panel_para * panel);
int disp_al_lcd_get_start_delay(u32 screen_id, disp_panel_para * panel);
int disp_al_lcd_get_clk_info(u32 screen_id, struct lcd_clk_info *info, disp_panel_para * panel);
int disp_al_lcd_enable_irq(u32 screen_id, __lcd_irq_id_t irq_id, disp_panel_para * panel);
int disp_al_lcd_disable_irq(u32 screen_id, __lcd_irq_id_t irq_id, disp_panel_para * panel);

int disp_al_hdmi_enable(u32 screen_id);
int disp_al_hdmi_disable(u32 screen_id);
int disp_al_hdmi_cfg(u32 screen_id, struct disp_video_timings *video_info);

int disp_al_tv_enable(u32 screen_id);
int disp_al_tv_disable(u32 screen_id);
int disp_al_tv_cfg(u32 screen_id, struct disp_video_timings *video_info);
int disp_al_vdevice_cfg(u32 screen_id, struct disp_video_timings *video_info, struct disp_vdevice_interface_para *para);
int disp_al_vdevice_enable(u32 screen_id);
int disp_al_vdevice_disable(u32 screen_id);

int disp_al_device_get_cur_line(u32 screen_id);
int disp_al_device_get_start_delay(u32 screen_id);
int disp_al_device_query_irq(u32 screen_id);
int disp_al_device_enable_irq(u32 screen_id);
int disp_al_device_disable_irq(u32 screen_id);
int disp_al_device_get_status(u32 screen_id);

#ifdef SUPPORT_WB
int disp_al_set_rtmx_base(u32 disp, unsigned int base);
int disp_al_rtmx_init(u32 disp,unsigned int addr0,unsigned int addr1,unsigned int addr2,unsigned int w,unsigned int h,\
				  unsigned int outw,unsigned int outh,unsigned int fmt);
int disp_al_rtmx_set_addr(u32 disp, unsigned int addr0);
int disp_al_set_eink_wb_base(u32 disp, unsigned int base);
int disp_al_set_eink_wb_param(u32 disp, unsigned int w, unsigned int h, unsigned int addr);
int disp_al_enable_eink_wb_interrupt(u32 disp);
int disp_al_disable_eink_wb_interrupt(u32 disp);
int disp_al_clear_eink_wb_interrupt(u32 disp);
int disp_al_enable_eink_wb(u32 disp);
int disp_al_disable_eink_wb(u32 disp);
int disp_al_get_eink_wb_status(u32 disp);
#endif

int disp_al_get_fb_info(unsigned int sel, struct disp_layer_info *info);
int disp_al_get_display_size(unsigned int sel, unsigned int *width, unsigned int *height);
int disp_al_set_eink_base(u32 disp, unsigned long base);
int disp_al_eink_irq_enable(u32 disp);
int disp_al_eink_irq_disable(u32 disp);
int disp_al_eink_irq_query(u32 disp);
int disp_al_eink_config(u32 disp, struct eink_init_param* param);
int disp_al_eink_disable(u32 disp);
int disp_al_eink_start_calculate_index(u32 disp,
										unsigned long old_index_data_paddr,
										unsigned long new_index_data_paddr,
										struct eink_8bpp_image *last_image,
										struct eink_8bpp_image *current_image);
int disp_al_is_calculate_index_finish(unsigned int disp);
int disp_al_get_update_area(unsigned int disp, struct area_info* area);
int disp_al_eink_pipe_enable(u32 disp, unsigned int pipe_no);
int disp_al_eink_pipe_disable(u32 disp, unsigned int pipe_no);
int disp_al_eink_pipe_config(u32 disp,  unsigned int pipe_no, struct area_info area);
int disp_al_eink_pipe_config_wavefile(u32 disp, unsigned int wav_file_addr, unsigned int pipe_no);
int disp_al_eink_start_decode(unsigned int disp, unsigned long new_idx_addr,
								unsigned long wav_data_addr, struct eink_init_param *param);
int disp_al_init_waveform(const char *path);
int disp_al_edma_init(unsigned int disp,struct eink_init_param* param);
int disp_al_edma_config(unsigned int disp, unsigned long wave_data_addr, struct eink_init_param *param);
int disp_al_eink_edma_cfg_addr(unsigned int disp, unsigned long wav_addr);
int disp_al_dbuf_rdy(void);
int disp_al_edma_write(unsigned int disp, unsigned char en);
int disp_al_get_waveform_data(unsigned int disp, enum eink_update_mode mode, unsigned int temp, unsigned int *total_frames, unsigned int*wf_buf);
int disp_al_get_eink_panel_bit_num(unsigned int disp, enum  eink_bit_num *bit_num);
void disp_al_free_waveform(void);
int disp_al_init_eink_ctrl_data_8(unsigned int disp, unsigned long wavedata_buf, struct eink_timing_param *eink_timing_info, unsigned int i);
int disp_al_init_eink_ctrl_data_16(unsigned int disp, unsigned int wavedata_buf, struct eink_timing_param *eink_timing_info);


#endif

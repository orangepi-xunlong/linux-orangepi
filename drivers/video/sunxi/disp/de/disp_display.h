
#ifndef __DISP_DISPLAY_H__
#define __DISP_DISPLAY_H__

#include "disp_private.h"

typedef struct
{
	bool                  have_cfg_reg;
	u32                   cache_flag;
	u32                   cfg_cnt;
#ifdef __LINUX_PLAT__
	spinlock_t              flag_lock;
	u32                     wait_count;
	wait_queue_head_t       wait;
#endif
	bool                  vsync_event_en;
	bool                  dvi_enable;
}__disp_screen_t;

typedef struct
{
	__disp_bsp_init_para    init_para;//para from driver
	__disp_screen_t         screen[3];
	u32                   print_level;
	u32                   lcd_registered[3];
	u32                   hdmi_registered;
	u32                   edp_registered;
}__disp_dev_t;

typedef struct
{
	u32 last_time;
	u32 current_time;
	u32 counter;
}disp_fps_data;

extern disp_fps_data fps_data[3];

extern __disp_dev_t gdisp;
extern __disp_al_private_data *disp_al_get_priv(u32 screen_id);
s32 disp_init_connections(void);
s32 bsp_disp_cfg_get(u32 screen_id);
void sync_event_proc(u32 screen_id);
void sync_finish_event_proc(u32 screen_id);
#ifdef CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC
s32 bsp_disp_get_vb_time(void);
s32 bsp_disp_get_next_vb_time(void);
s32 bsp_disp_is_in_vb(void);
#endif
s32 bsp_disp_get_panel_info(u32 screen_id, disp_panel_para *info);
void LCD_OPEN_FUNC(u32 screen_id, LCD_FUNC func, u32 delay);
void LCD_CLOSE_FUNC(u32 screen_id, LCD_FUNC func, u32 delay);

#endif

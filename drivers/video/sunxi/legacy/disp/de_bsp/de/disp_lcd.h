
#ifndef __DISP_LCD_H__
#define __DISP_LCD_H__

#include "disp_display_i.h"

__s32 disp_lcdc_init(__u32 screen_id);
__s32 disp_lcdc_exit(__u32 screen_id);

#ifdef __LINUX_OSAL__
__s32 disp_lcdc_event_proc(__s32 irq, void *parg);
#else
__s32 disp_lcdc_event_proc(void *parg);
#endif
__s32 disp_lcdc_pin_cfg(__u32 screen_id, __disp_output_type_t out_type, __u32 bon);
__u32 disp_get_screen_scan_mode(__disp_tv_mode_t tv_mode);
__s32 TCON_get_cur_line(__u32 screen_id, __u32 tcon_index);
__s32 TCON_get_start_delay(__u32 screen_id, __u32 tcon_index);

__u32 tv_mode_to_width(__disp_tv_mode_t mode);
__u32 tv_mode_to_height(__disp_tv_mode_t mode);
__u32 vga_mode_to_width(__disp_vga_mode_t mode);
__u32 vga_mode_to_height(__disp_vga_mode_t mode);

__s32 bsp_disp_lcd_delay_ms(__u32 ms) ;
__s32 bsp_disp_lcd_delay_us(__u32 ns);

__s32 LCD_GPIO_request(__u32 screen_id, __u32 io_index);
__s32 LCD_GPIO_release(__u32 screen_id,__u32 io_index);
__s32 LCD_GPIO_set_attr(__u32 screen_id,__u32 io_index, __bool b_output);
__s32 LCD_GPIO_read(__u32 screen_id,__u32 io_index);
__s32 LCD_GPIO_write(__u32 screen_id,__u32 io_index, __u32 data);
void LCD_get_reg_bases(__reg_bases_t *para);
void LCD_OPEN_FUNC(__u32 screen_id, LCD_FUNC func, __u32 delay);
void LCD_CLOSE_FUNC(__u32 screen_id, LCD_FUNC func, __u32 delay);
__s32 LCD_PWM_EN(__u32 screen_id, __bool b_en);
__s32 LCD_BL_EN(__u32 screen_id, __bool b_en);
__s32 LCD_POWER_EN(__u32 screen_id, __u32 pwr_id, __bool b_en);
void LCD_CPU_register_irq(__u32 screen_id, void (*Lcd_cpuisr_proc) (void));
__s32 lcd_get_panel_para(__u32 screen_id, __panel_para_t * info);
__s32 LCD_CPU_WR(__u32 screen_id,__u32 index,__u32 data);
__s32 LCD_CPU_WR_INDEX(__u32 screen_id,__u32 index);
__s32 LCD_CPU_WR_DATA(__u32 screen_id,__u32 data);
__s32 disp_lcd_set_fps(__u32 screen_id);
__s32 bsp_disp_lcd_dsi_io_enable(__u32 screen_id);
__s32 disp_lcd_get_support(__u32 screen_id);
__s32 bsp_disp_lcd_get_registered(void);

extern __panel_para_t              gpanel_info[2];

#endif

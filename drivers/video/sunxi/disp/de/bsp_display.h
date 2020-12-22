
#ifndef __EBSP_DISPLAY_H__
#define __EBSP_DISPLAY_H__

#if defined(CONFIG_FPGA_V4_PLATFORM) || defined(CONFIG_FPGA_V7_PLATFORM) || defined(CONFIG_A67_FPGA)
#define __FPGA_DEBUG__
#endif

#define __LINUX_PLAT__
//#define __MELIS_OSAL__
//#define __WINCE_OSAL__
//#define __BOOT_OSAL__

#if defined(__LINUX_PLAT__)
#include <linux/module.h>
#include "linux/kernel.h"
#include "linux/mm.h"
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()¡¢kthread_run()
#include <linux/err.h> //IS_ERR()¡¢PTR_ERR()
#include <linux/delay.h>
#include <linux/platform_device.h>
#include "asm-generic/int-ll64.h"
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pwm.h>
#include <mach/sys_config.h>
#include <mach/irqs.h>
#include <mach/platform.h>
#if defined(CONFIG_ARCH_SUN6I)
#include <linux/sunxi_physmem.h>
#include <mach/clock.h>
#else
#include <linux/ion_sunxi.h>
#endif
#if defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1)
#include <linux/sync.h>
#include <linux/sw_sync.h>
#endif

typedef unsigned int __hdle;

#include <video/drv_display.h>
#include "../OSAL/OSAL.h"

#define DEFAULT_PRINT_LEVLE 0

#define OSAL_PRINTF(msg...) do{printk(KERN_WARNING "[DISP] ");printk(msg);}while(0)
#define __inf(msg...)       do{if(bsp_disp_get_print_level()){printk(KERN_WARNING "[DISP] ");printk(msg);}}while(0)
#define __msg(msg...)       do{if(bsp_disp_get_print_level()){printk(KERN_WARNING "[DISP] %s,line:%d:    ",__func__,__LINE__);printk(msg);}}while(0)
#define __wrn(msg...)       do{{printk(KERN_WARNING "[DISP] %s,line:%d:    ",__func__,__LINE__);printk(msg);}}while(0)
#define __here__            do{if(bsp_disp_get_print_level()==2){printk(KERN_WARNING "[DISP] %s,line:%d\n",__func__,__LINE__);}}while(0)
#define __debug(msg...)     do{if(bsp_disp_get_print_level()==2){printk(KERN_WARNING "[DISP] ");printk(msg);}}while(0)

#endif//end of define __LINUX_PLAT__

typedef struct
{
	u32 disp_rsl;
	u8 closed;
	u8 output_type; //disp_output_type
	u8 output_mode;  //disp_output_type or disp_lcd_if
	u8 output_channel;
	u8 sw_init_flag;
}disp_sw_init_para;

typedef struct
{
	u32                   reg_base[DISP_MOD_NUM];
	u32                   reg_size[DISP_MOD_NUM];
	u32                   irq_no[DISP_MOD_NUM];

	s32 (*disp_int_process)(u32 sel);
	s32 (*vsync_event)(u32 sel);
	s32 (*start_process)(void);
	s32 (*capture_event)(u32 sel);
	s32 (*shadow_protect)(u32 sel, bool protect);
	disp_sw_init_para *sw_init_para;
}__disp_bsp_init_para;

typedef struct
{
	u32 acquire_count;
	u32 release_count;
	u32 display_count;
	u32 video_count;
	u32 video_strict_count;
	u32 vsync_count[3];
	u32 skip_count[3];
	u32 frame_refresh_array[100];
	u32 current_index;
}disp_fcount_data;
extern disp_fcount_data fcount_data;

extern s32 bsp_disp_shadow_protect(u32 screen_id, bool protect);
extern s32 bsp_disp_set_print_level(u32 print_level);
extern s32 bsp_disp_get_print_level(void);
extern s32 bsp_disp_print_reg(bool b_force_on,disp_mod_id id,char * buf);
extern s32 bsp_disp_vsync_event_enable(u32 screen_id, bool enable);
extern s32 bsp_disp_get_lcd_registered(u32 screen_id);
extern s32 bsp_disp_get_hdmi_registered(void);
extern s32 bsp_disp_get_output_type(u32 screen_id);
extern s32 bsp_disp_get_lcd_output_type(u32 screen_id);
extern s32 bsp_disp_get_screen_width(u32 screen_id);
extern s32 bsp_disp_get_screen_height(u32 screen_id);
extern s32 bsp_disp_get_screen_physical_width(u32 screen_id);
extern s32 bsp_disp_get_screen_physical_height(u32 screen_id);
extern s32 bsp_disp_get_screen_width_from_output_type(u32 screen_id, u32 output_type, u32 output_mode);
extern s32 bsp_disp_get_screen_height_from_output_type(u32 screen_id, u32 output_type, u32 output_mode);
extern s32 bsp_disp_open(void);
extern s32 bsp_disp_close(void);
extern s32 bsp_disp_init(__disp_bsp_init_para * para);
extern s32 bsp_disp_exit(u32 mode);
extern s32 bsp_disp_get_timming(u32 screen_id, disp_video_timing * tt);
extern s32 bsp_disp_get_fps(u32 screen_id);
#if defined(CONFIG_ARCH_SUN9IW1P1)
extern s32 bsp_disp_hdmi_enable(u32 screen_id);
extern s32 bsp_disp_hdmi_disable(u32 screen_id);
extern s32 bsp_disp_hdmi_set_mode(u32 screen_id,  disp_tv_mode mode);
extern s32 bsp_disp_hdmi_get_mode(u32 screen_id);
extern s32 bsp_disp_hdmi_check_support_mode(u32 screen_id,  u8 mode);
extern s32 bsp_disp_hdmi_get_input_csc(u32 screen_id);
extern s32 bsp_disp_set_hdmi_func(u32 screen_id, disp_hdmi_func * func);
extern s32 bsp_disp_hdmi_suspend(u32 screen_id);
extern s32 bsp_disp_hdmi_resume(u32 screen_id);
extern s32 bsp_disp_hdmi_get_hpd_status(u32 sel);
#endif
extern s32 bsp_disp_lcd_pre_enable(u32 sel);
extern s32 bsp_disp_lcd_post_enable(u32 sel);
extern disp_lcd_flow * bsp_disp_lcd_get_open_flow(u32 sel);
extern s32 bsp_disp_lcd_pre_disable(u32 sel);
extern s32 bsp_disp_lcd_post_disable(u32 sel);
extern disp_lcd_flow * bsp_disp_lcd_get_close_flow(u32 sel);
extern s32 bsp_disp_lcd_set_bright(u32 sel, u32  bright);
extern s32 bsp_disp_lcd_get_bright(u32 sel);
extern s32 bsp_disp_lcd_power_enable(u32 screen_id, u32 pwr_id);
extern s32 bsp_disp_lcd_power_disable(u32 screen_id, u32 pwr_id);
extern s32 bsp_disp_lcd_backlight_enable(u32 screen_id);
extern s32 bsp_disp_lcd_backlight_disable(u32 screen_id);
extern s32 bsp_disp_lcd_pwm_enable(u32 screen_id);
extern s32 bsp_disp_lcd_pwm_disable(u32 screen_id);
extern s32 bsp_disp_get_timming(u32 sel, disp_video_timing * tt);
extern s32 bsp_disp_lcd_is_used(u32 sel);
extern s32 bsp_disp_lcd_tcon_enable(u32 sel);
extern s32 bsp_disp_lcd_tcon_disable(u32 sel);
extern s32 bsp_disp_lcd_get_registered(void);
extern s32 bsp_disp_lcd_delay_ms(u32 ms);
extern s32 bsp_disp_lcd_delay_us(u32 us);
extern s32 bsp_disp_lcd_set_panel_funs(char *name, disp_lcd_panel_fun * lcd_cfg);
extern s32 bsp_disp_lcd_pin_cfg(u32 screen_id, u32 bon);
extern s32 bsp_disp_lcd_gpio_set_value(u32 screen_id, u32 io_index, u32 value);
extern s32 bsp_disp_lcd_gpio_set_direction(u32 screen_id, u32 io_index, u32 direction);
extern s32 bsp_disp_lcd_get_tv_mode(u32 screen_id);
extern s32 bsp_disp_lcd_set_tv_mode(u32 screen_id, disp_tv_mode tv_mode);

extern s32 bsp_disp_layer_set_info(u32 screen_id, u32 layer_id,disp_layer_info *player);
extern s32 bsp_disp_layer_get_info(u32 screen_id, u32 layer_id,disp_layer_info *player);
extern s32 bsp_disp_layer_enable(u32 screen_id, u32 layer_id);
extern s32 bsp_disp_layer_disable(u32 screen_id, u32 layer_id);
extern s32 bsp_disp_layer_is_enabled(u32 screen_id, u32 layer_id);
extern s32 bsp_disp_layer_get_frame_id(u32 screen_id, u32 layer_id);
extern s32 bsp_disp_layer_deinterlace_cfg(u32 screen_id);
extern u32 bsp_disp_layer_get_addr(u32 sel,u32 hid);
extern u32 bsp_disp_layer_set_addr(u32 sel,u32 hid,u32 addr);
extern u32 bsp_disp_layer_get_size(u32 sel,u32 hid,u32 * width,u32 * height);

//capture
extern s32 bsp_disp_capture_screen(u32 screen_id, disp_capture_para *para);
extern s32 bsp_disp_capture_screen_stop(u32 screen_id);
extern s32 bsp_disp_capture_screen_get_buffer_id(u32 screen_id);
extern s32 bsp_disp_capture_screen_finished(u32 screen_id);

extern s32 bsp_disp_smcl_enable(u32 screen_id);
extern s32 bsp_disp_smcl_disable(u32 screen_id);
extern s32 bsp_disp_smcl_is_enabled(u32 screen_id);
extern s32 bsp_disp_smcl_set_bright(u32 screen_id, u32 val);
extern s32 bsp_disp_smcl_set_saturation(u32 screen_id, u32 val);
extern s32 bsp_disp_smcl_set_contrast(u32 screen_id, u32 val);
extern s32 bsp_disp_smcl_set_hue(u32 screen_id, u32 val);
extern s32 bsp_disp_smcl_set_mode(u32 screen_id, u32 val);
extern s32 bsp_disp_smcl_set_window(u32 screen_id, disp_window *window);
extern s32 bsp_disp_smcl_get_bright(u32 screen_id);
extern s32 bsp_disp_smcl_get_saturation(u32 screen_id);
extern s32 bsp_disp_smcl_get_contrast(u32 screen_id);
extern s32 bsp_disp_smcl_get_hue(u32 screen_id);
extern s32 bsp_disp_smcl_get_mode(u32 screen_id);
extern s32 bsp_disp_smcl_get_window(u32 screen_id, disp_window *window);

extern s32 bsp_disp_smbl_enable(u32 screen_id);
extern s32 bsp_disp_smbl_disable(u32 screen_id);
extern s32 bsp_disp_smbl_is_enabled(u32 screen_id);
extern s32 bsp_disp_smbl_set_window(u32 screen_id, disp_window *window);
extern s32 bsp_disp_smbl_get_window(u32 screen_id, disp_window *window);

#if defined(CONFIG_ARCH_SUN9IW1P1)
extern s32 bsp_disp_cursor_enable(u32 screen_id);
extern s32 bsp_disp_cursor_disable(u32 screen_id);
extern s32 bsp_disp_cursor_is_enabled(u32 screen_id);
extern s32 bsp_disp_cursor_set_pos(u32 screen_id, disp_position *pos);
extern s32 bsp_disp_cursor_get_pos(u32 screen_id, disp_position *pos);
extern s32 bsp_disp_cursor_set_fb(u32 screen_id, disp_cursor_fb *fb);
extern s32 bsp_disp_cursor_set_palette(u32 screen_id, void *palette, u32 offset, u32 palette_size);
#endif
extern s32 bsp_disp_set_back_color(u32 screen_id, disp_color_info *bk_color);
extern s32 bsp_disp_set_color_key(u32 screen_id, disp_colorkey *ck);

int bsp_disp_feat_get_num_screens(void);
int bsp_disp_feat_get_num_layers(u32 screen_id);
int bsp_disp_feat_get_num_scalers(void);
disp_output_type bsp_disp_feat_get_supported_output_types(u32 screen_id);
enum __disp_layer_feat bsp_disp_feat_get_layer_feats(u32 screen_id,
    disp_layer_mode mode, u32 scaler_index);
enum __disp_enhance_feat bsp_disp_feat_get_enhance_feats(u32 screen_id);
int bsp_disp_feat_get_layer_horizontal_resolution_max(u32 screen_id,
    disp_layer_mode mode, u32 scaler_index);
int bsp_disp_feat_get_layer_vertical_resolution_max(u32 screen_id,
    disp_layer_mode mode, u32 scaler_index);
int bsp_disp_feat_get_layer_horizontal_resolution_min(u32 screen_id,
    disp_layer_mode mode, u32 scaler_index);
int bsp_disp_feat_get_layer_vertical_resolution_min(u32 screen_id,
    disp_layer_mode mode, u32 scaler_index);
int bsp_disp_feat_get_de_flicker_support(u32 screen_id);
int bsp_disp_feat_get_smart_backlight_support(u32 screen_id);
int bsp_disp_feat_get_image_detail_enhance_support(u32 screen_id);

#if defined(__LINUX_PLAT__)
s32 Display_set_fb_timming(u32 sel);
#endif

#ifdef CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC
s32 bsp_disp_get_vb_time(void);
s32 bsp_disp_get_next_vb_time(void);
s32 bsp_disp_is_in_vb(void);
#endif

#define BIT0		  0x00000001
#define BIT1		  0x00000002
#define BIT2		  0x00000004
#define BIT3		  0x00000008
#define BIT4		  0x00000010
#define BIT5		  0x00000020
#define BIT6		  0x00000040
#define BIT7		  0x00000080
#define BIT8		  0x00000100
#define BIT9		  0x00000200
#define BIT10		  0x00000400
#define BIT11		  0x00000800
#define BIT12		  0x00001000
#define BIT13		  0x00002000
#define BIT14		  0x00004000
#define BIT15		  0x00008000
#define BIT16		  0x00010000
#define BIT17		  0x00020000
#define BIT18		  0x00040000
#define BIT19		  0x00080000
#define BIT20		  0x00100000
#define BIT21		  0x00200000
#define BIT22		  0x00400000
#define BIT23		  0x00800000
#define BIT24		  0x01000000
#define BIT25		  0x02000000
#define BIT26		  0x04000000
#define BIT27		  0x08000000
#define BIT28		  0x10000000
#define BIT29		  0x20000000
#define BIT30		  0x40000000
#define BIT31		  0x80000000

#define sys_get_wvalue(n)   (*((volatile u32 *)(n)))          /* word input */
#define sys_put_wvalue(n,c) (*((volatile u32 *)(n))  = (c))   /* word output */

#endif

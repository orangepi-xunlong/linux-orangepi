
#ifndef __EBSP_DISPLAY_H__
#define __EBSP_DISPLAY_H__

//#define __FPGA_DEBUG__

#define __LINUX_OSAL__
//#define __MELIS_OSAL__
//#define __WINCE_OSAL__
//#define __BOOT_OSAL__

#ifdef __LINUX_OSAL__
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
//#include <mach/clock.h>
//#include <mach/gpio.h>
//#include <mach/system.h>
#include <mach/irqs.h>
#include <mach/platform.h>
#include <linux/ion_sunxi.h>

//#include <linux/sunxi_physmem.h>

typedef unsigned int __hdle;

#include <video/drv_display.h>
#include "../OSAL/OSAL.h"
#include "iep/iep.h"

#define DEFAULT_PRINT_LEVLE 0
#define OSAL_PRINTF(msg...) do{printk(KERN_WARNING "[DISP] ");printk(msg);}while(0)
#define __inf(msg...)       do{if(bsp_disp_get_print_level()){printk(KERN_WARNING "[DISP] ");printk(msg);}}while(0)
#define __msg(msg...)       do{if(bsp_disp_get_print_level()){printk(KERN_WARNING "[DISP] file:%s,line:%d:    ",__FILE__,__LINE__);printk(msg);}}while(0)
#define __wrn(msg...)       {printk(KERN_WARNING "[DISP WRN] file:%s,line:%d:    ",__FILE__,__LINE__);printk(msg);}
#define __here__            do{if(bsp_disp_get_print_level()==2){printk(KERN_WARNING "[DISP] file:%s,line:%d\n",__FILE__,__LINE__);}}while(0)
#define __debug(msg...)     do{if(bsp_disp_get_print_level()==2){printk(KERN_WARNING "[DISP] ");printk(msg);}}while(0)


#endif//end of define __LINUX_OSAL__

#ifdef __MELIS_OSAL__
#include "string.h"
#include "D:/winners/eBase/eBSP/BSP/sun_20/common_inc.h"
#endif

#ifdef __BOOT_OSAL__
#define OSAL_PRINTF wlibc_uprintf

#include "egon2.h"
#include "string.h"
#include "../OSAL/OSAL_De.h"
#endif


typedef struct
{
	__u32                   reg_base[DISP_MOD_NUM];
	__u32                   reg_size[DISP_MOD_NUM];
	__u32                   irq[DISP_MOD_NUM];

	void (*tve_interrup) (__u32 sel);
	__s32 (*hdmi_set_mode)(__disp_tv_mode_t mode);
	__s32 (*hdmi_open)(void);
	__s32 (*hdmi_close)(void);
	__s32 (*hdmi_mode_support)(__disp_tv_mode_t mode);
	__s32 (*hdmi_get_HPD_status)(void);
	__s32 (*hdmi_set_pll)(__u32 pll, __u32 clk);
	__s32 (*hdmi_dvi_enable)(__u32 mode);
	__s32 (*hdmi_dvi_support)(void);
	__s32 (*hmdi_get_input_csc)(void);
	__s32 (*hdmi_suspend)(void);
	__s32 (*hdmi_resume)(void);
	__s32 (*hdmi_early_suspend)(void);
	__s32 (*hdmi_late_resume)(void);
	__s32 (*disp_int_process)(__u32 sel);
	__s32 (*vsync_event)(__u32 sel);
	__s32 (*take_effect) (__u32 sel);
	__s32 (*start_process)(void);
	__s32 (*capture_event)(__u32 sel);
	__u32 hdmi_cts_compatibility;//0:force hdmi; 1: auto output hdmi/dvi
}__disp_bsp_init_para;

extern __s32 bsp_disp_clk_on(__u32 type);
extern __s32 bsp_disp_clk_off(__u32 type);
extern __s32 bsp_disp_init(__disp_bsp_init_para * para);
extern __s32 bsp_disp_exit(__u32 mode);
extern __s32 bsp_disp_open(void);
extern __s32 bsp_disp_close(void);
extern __s32 bsp_disp_print_reg(__bool b_force_on, __disp_mod_id_t id, char *buf);
extern __s32 bsp_disp_set_print_level(__u32 print_level);
extern __s32 bsp_disp_get_print_level(void);
extern __s32 bsp_disp_set_condition(__u32 condition);
extern __s32 bsp_disp_get_condition(void);

extern __s32 bsp_disp_cmd_cache(__u32 sel);
extern __s32 bsp_disp_cmd_submit(__u32 sel);
extern __s32 bsp_disp_set_bk_color(__u32 sel, __disp_color_t *color);
extern __s32 bsp_disp_get_bk_color(__u32 sel, __disp_color_t *color);
extern __s32 bsp_disp_set_color_key(__u32 sel, __disp_colorkey_t *ck_mode);
extern __s32 bsp_disp_get_color_key(__u32 sel, __disp_colorkey_t *ck_mode);
extern __s32 bsp_disp_set_palette_table(__u32 sel, __u32 *pbuffer, __u32 offset, __u32 size);
extern __s32 bsp_disp_get_palette_table(__u32 sel, __u32 * pbuffer, __u32 offset,__u32 size);
extern __s32 bsp_disp_get_screen_height(__u32 sel);
extern __s32 bsp_disp_get_screen_width(__u32 sel);
extern __s32 bsp_disp_get_screen_width_from_output_type(__u32 screen_id, __disp_output_type_t output_type, __u32 mode);
extern __s32 bsp_disp_get_screen_height_from_output_type(__u32 screen_id, __disp_output_type_t output_type, __u32 mode);
extern __s32 bsp_disp_get_screen_physical_height(__u32 sel);
extern __s32 bsp_disp_get_screen_physical_width(__u32 sel);
extern __s32 bsp_disp_get_output_type(__u32 sel);
extern __s32 bsp_disp_get_frame_rate(__u32 sel);
extern __s32 bsp_disp_gamma_correction_enable(__u32 sel);
extern __s32 bsp_disp_gamma_correction_disable(__u32 sel);
extern __s32 bsp_disp_set_bright(__u32 sel, __u32 bright);
extern __s32 bsp_disp_get_bright(__u32 sel);
extern __s32 bsp_disp_set_contrast(__u32 sel, __u32 contrast);
extern __s32 bsp_disp_get_contrast(__u32 sel);
extern __s32 bsp_disp_set_saturation(__u32 sel, __u32 saturation);
extern __s32 bsp_disp_get_saturation(__u32 sel);
extern __s32 bsp_disp_set_hue(__u32 sel, __u32 hue);
extern __s32 bsp_disp_get_hue(__u32 sel);
extern __s32 bsp_disp_enhance_enable(__u32 sel, __bool enable);
extern __s32 bsp_disp_get_enhance_enable(__u32 sel);
extern __s32 bsp_disp_capture_screen(__u32 sel, __disp_capture_screen_para_t * para);
extern __s32 bsp_disp_capture_screen_stop(__u32 sel);
extern __s32 bsp_disp_capture_screen_get_buffer_id(__u32 sel);
extern __s32 bsp_disp_set_screen_size(__u32 sel, __disp_rectsz_t * size);
extern __s32 bsp_disp_set_output_csc(__u32 sel, __disp_out_csc_type_t type, __u32 drc_csc);
extern __s32 bsp_disp_de_flicker_enable(__u32 sel, __bool b_en);
extern __s32 bsp_disp_store_image_reg(__u32 sel, __u32 addr);
extern __s32 bsp_disp_restore_image_reg(__u32 sel, __u32 addr);
extern __s32 bsp_disp_vsync_event_enable(__u32 sel, __bool enable);
extern __s32 bsp_disp_get_vint_count(__u32 sel);
extern __s32 bsp_disp_get_lint_count(__u32 sel);
extern __s32 bsp_disp_cmd_cache_get(__u32 sel);
extern __s32 bsp_disp_cfg_get(__u32 sel);
extern __s32 bsp_disp_image_resume(__u32 screen_id);

extern __s32 bsp_disp_layer_request(__u32 sel, __disp_layer_work_mode_t mode);
extern __s32 bsp_disp_layer_release(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_open(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_is_open(__u32 screen_id, __u32 hid);
extern __s32 bsp_disp_layer_close(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_set_framebuffer(__u32 sel, __u32 hid,__disp_fb_t *fbinfo);
extern __s32 bsp_disp_layer_get_framebuffer(__u32 sel, __u32 hid,__disp_fb_t*fbinfo);
extern __s32 bsp_disp_layer_set_src_window(__u32 sel, __u32 hid,__disp_rect_t *regn);
extern __s32 bsp_disp_layer_get_src_window(__u32 sel, __u32 hid,__disp_rect_t *regn);
extern __s32 bsp_disp_layer_set_screen_window(__u32 sel, __u32 hid,__disp_rect_t* regn);
extern __s32 bsp_disp_layer_get_screen_window(__u32 sel, __u32 hid,__disp_rect_t *regn);
extern __s32 bsp_disp_layer_set_para(__u32 sel, __u32 hid, __disp_layer_info_t * layer_para);
extern __s32 bsp_disp_layer_get_para(__u32 sel, __u32 hid, __disp_layer_info_t * layer_para);
extern __s32 bsp_disp_layer_set_top(__u32 sel, __u32  handle);
extern __s32 bsp_disp_layer_set_bottom(__u32 sel, __u32  handle);
extern __s32 bsp_disp_layer_set_alpha_value(__u32 sel, __u32 hid,__u8 value);
extern __s32 bsp_disp_layer_get_alpha_value(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_alpha_enable(__u32 sel, __u32 hid, __bool enable);
extern __s32 bsp_disp_layer_get_alpha_enable(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_set_pipe(__u32 sel, __u32 hid,__u8 pipe);
extern __s32 bsp_disp_layer_get_pipe(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_get_piro(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_colorkey_enable(__u32 sel, __u32 hid, __bool enable);
extern __s32 bsp_disp_layer_get_colorkey_enable(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_set_smooth(__u32 sel, __u32 hid, __disp_video_smooth_t  mode);
extern __s32 bsp_disp_layer_get_smooth(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_set_bright(__u32 sel, __u32 hid, __u32 bright);
extern __s32 bsp_disp_layer_set_contrast(__u32 sel, __u32 hid, __u32 contrast);
extern __s32 bsp_disp_layer_set_saturation(__u32 sel, __u32 hid, __u32 saturation);
extern __s32 bsp_disp_layer_set_hue(__u32 sel, __u32 hid, __u32 hue);
extern __s32 bsp_disp_layer_enhance_enable(__u32 sel, __u32 hid, __bool enable);
extern __s32 bsp_disp_layer_get_bright(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_get_contrast(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_get_saturation(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_get_hue(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_get_enhance_enable(__u32 sel, __u32 hid);

extern __s32 bsp_disp_layer_vpp_enable(__u32 sel, __u32 hid, __bool enable);
extern __s32 bsp_disp_layer_get_vpp_enable(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_set_luma_sharp_level(__u32 sel, __u32 hid, __u32 level);
extern __s32 bsp_disp_layer_get_luma_sharp_level(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_set_chroma_sharp_level(__u32 sel, __u32 hid, __u32 level);
extern __s32 bsp_disp_layer_get_chroma_sharp_level(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_set_white_exten_level(__u32 sel, __u32 hid, __u32 level);
extern __s32 bsp_disp_layer_get_white_exten_level(__u32 sel, __u32 hid);
extern __s32 bsp_disp_layer_set_black_exten_level(__u32 sel, __u32 hid, __u32 level);
extern __s32 bsp_disp_layer_get_black_exten_level(__u32 sel, __u32 hid);

extern __s32 bsp_disp_scaler_get_smooth(__u32 sel);
extern __s32 bsp_disp_scaler_set_smooth(__u32 sel, __disp_video_smooth_t  mode);
extern __s32 bsp_disp_scaler_request(void);
extern __s32 bsp_disp_scaler_release(__u32 handle);
extern __s32 bsp_disp_scaler_start(__u32 handle,__disp_scaler_para_t *scl);
extern __s32 bsp_disp_scaler_start_ex(__u32 handle,__disp_scaler_para_t *scl);
extern __s32 bsp_disp_store_scaler_reg(__u32 sel, __u32 addr);
extern __s32 bsp_disp_restore_scaler_reg(__u32 sel, __u32 addr);

extern __s32 bsp_disp_hwc_enable(__u32 sel, __bool enable);
extern __s32 bsp_disp_hwc_set_pos(__u32 sel, __disp_pos_t *pos);
extern __s32 bsp_disp_hwc_get_pos(__u32 sel, __disp_pos_t *pos);
extern __s32 bsp_disp_hwc_set_framebuffer(__u32 sel, __disp_hwc_pattern_t *patmem);
extern __s32 bsp_disp_hwc_set_palette(__u32 sel, void *palette,__u32 offset, __u32 palette_size);

extern __s32 bsp_disp_video_set_fb(__u32 sel, __u32 hid, __disp_video_fb_t *in_addr);
extern __s32 bsp_disp_video_get_fb(__u32 sel, __u32 hid, __disp_video_fb_t *in_addr);
extern __s32 bsp_disp_video_get_frame_id(__u32 sel, __u32 hid);
extern __s32 bsp_disp_video_get_dit_info(__u32 sel, __u32 hid, __disp_dit_info_t * dit_info);
extern __s32 bsp_disp_video_start(__u32 sel, __u32 hid);
extern __s32 bsp_disp_video_stop(__u32 sel, __u32 hid);
extern __s32 bsp_disp_video_get_start(__u32 sel, __u32 hid);
extern __s32 bsp_disp_video_get_fps(__u32 sel, __u32 hid);

extern __s32 bsp_disp_lcd_open_before(__u32 sel);
extern __s32 bsp_disp_lcd_open_after(__u32 sel);
extern __lcd_flow_t * bsp_disp_lcd_get_open_flow(__u32 sel);
extern __s32 bsp_disp_lcd_close_befor(__u32 sel);
extern __s32 bsp_disp_lcd_close_after(__u32 sel);
extern __lcd_flow_t * bsp_disp_lcd_get_close_flow(__u32 sel);
//extern __s32 bsp_disp_lcd_xy_switch(__u32 sel, __s32 mode);
extern __s32 bsp_disp_set_gamma_table(__u32 sel, __u32 *gamtbl_addr,__u32 gamtbl_size);
extern __s32 bsp_disp_lcd_set_bright(__u32 sel, __u32  bright, __u32 from_iep);
extern __s32 bsp_disp_lcd_get_bright(__u32 sel);
extern __s32 bsp_disp_lcd_set_src(__u32 sel, __disp_lcdc_src_t src);
extern __s32 bsp_disp_lcd_get_src(__u32 sel);
extern __s32 bsp_disp_lcd_power_enable(__u32 screen_id, __u32 pwr_id);
extern __s32 bsp_disp_lcd_power_disable(__u32 screen_id, __u32 pwr_id);
extern __s32 bsp_disp_lcd_backlight_enable(__u32 screen_id);
extern __s32 bsp_disp_lcd_backlight_disable(__u32 screen_id);
extern __s32 bsp_disp_lcd_pwm_enable(__u32 screen_id);
extern __s32 bsp_disp_lcd_pwm_disable(__u32 screen_id);
extern __s32 bsp_disp_lcd_user_defined_func(__u32 sel, __u32 para1, __u32 para2, __u32 para3);
extern __s32 bsp_disp_lcd_set_panel_funs(char *drv_name, __lcd_panel_fun_t * lcd_cfg);
extern __s32 bsp_disp_lcd_pin_cfg(__u32 screen_id, __u32 bon);
extern __s32   dsi_clk_enable(__u32 sel, __u32 en);

extern __s32 bsp_disp_get_timming(__u32 sel, __disp_tcon_timing_t * tt);
extern __u32 bsp_disp_get_cur_line(__u32 sel);
extern __s32 bsp_disp_close_lcd_backlight(__u32 sel);
extern __s32 bsp_disp_open_lcd_backlight(__u32 sel);
extern __s32 bsp_disp_lcd_set_bright_dimming(__u32 sel, __u32 bright_dimming);  //for drc
extern __s32 bsp_disp_lcd_used(__u32 sel);
extern __s32 bsp_disp_restore_lcdc_reg(__u32 sel);
extern __s32 bsp_disp_lcd_get_bright_curve_en(__u32 sel);
extern __s32 bsp_disp_lcd_set_bright_curve_en(__u32 sel, __u32 en);
extern __s32 bsp_disp_get_fps(__u32 sel);
extern __s32 bsp_disp_lcd_set_fps(__u32 sel, __u32 fps);
extern __s32 bsp_disp_lcd_get_size(__u32 sel, char *lcd_size);
extern __s32 bsp_disp_lcd_get_model_name(__u32 sel, char *model_name);
extern __s32 bsp_disp_lcd_delay_ms(__u32 ms);
extern __s32 bsp_disp_lcd_delay_us(__u32 us);
extern __s32 bsp_disp_lcd_tcon_open(__u32 sel);
extern __s32 bsp_disp_lcd_tcon_close(__u32 sel);
extern __s32 bsp_disp_lcd_get_driver_name(__u32 screen_id, char* name);
extern __s32 bsp_disp_lcd_get_registered(void);

extern __s32 bsp_disp_tv_open(__u32 sel);
extern __s32 bsp_disp_tv_close(__u32 sel);
extern __s32 bsp_disp_tv_set_mode(__u32 sel, __disp_tv_mode_t tv_mod);
extern __s32 bsp_disp_tv_get_mode(__u32 sel);
extern __s32 bsp_disp_tv_get_interface(__u32 sel);
extern __s32 bsp_disp_tv_auto_check_enable(__u32 sel);
extern __s32 bsp_disp_tv_auto_check_disable(__u32 sel);
extern __s32 bsp_disp_tv_set_src(__u32 sel, __disp_lcdc_src_t src);
extern __s32 bsp_disp_tv_get_dac_status(__u32 sel, __u32 index);
extern __s32 bsp_disp_tv_set_dac_source(__u32 sel, __u32 index, __disp_tv_dac_source source);
extern __s32 bsp_disp_tv_get_dac_source(__u32 sel, __u32 index);
extern __s32 bsp_disp_restore_tvec_reg(__u32 sel);

extern __s32 bsp_disp_hdmi_open(__u32 sel);
extern __s32 bsp_disp_hdmi_close(__u32 sel);
extern __s32 bsp_disp_hdmi_set_mode(__u32 sel, __disp_tv_mode_t  mode);
extern __s32 bsp_disp_hdmi_get_mode(__u32 sel);
extern __s32 bsp_disp_hdmi_check_support_mode(__u32 sel, __u8  mode);
extern __s32 bsp_disp_hdmi_get_hpd_status(__u32 sel);
extern __s32 bsp_disp_hdmi_set_src(__u32 sel, __disp_lcdc_src_t src);
extern __s32 bsp_disp_hdmi_get_src(__u32 sel);
extern __s32 bsp_disp_set_hdmi_func(__disp_hdmi_func * func);
extern __s32 bsp_disp_set_hdmi_hpd(__u32 hpd);
extern __s32 bsp_disp_hdmi_dvi_enable(__u32 sel, __u32 enable);
extern __s32 bsp_disp_hdmi_dvi_support(__u32 sel);
extern __s32 BSP_dsip_hdmi_get_input_csc(__u32 sel);
extern __s32 bsp_disp_hdmi_suspend(void);
extern __s32 bsp_disp_hdmi_resume(void);
extern __s32 bsp_disp_hdmi_early_suspend(void);
extern __s32 bsp_disp_hdmi_late_resume(void);
extern __s32 bsp_disp_hdmi_cts_enable(__u32 en);
extern __s32 bsp_disp_hdmi_get_cts_enable(void);
extern __s32 bsp_disp_hdmi_set_test_mode(__u32 sel, __u32 mode);
extern __s32 bsp_disp_hdmi_get_test_mode(__u32 sel);
extern __s32 bsp_disp_hdmi_get_registered(void);

extern __s32 bsp_disp_vga_open(__u32 sel);
extern __s32 bsp_disp_vga_close(__u32 sel);
extern __s32 bsp_disp_vga_set_mode(__u32 sel, __disp_vga_mode_t  mode);
extern __s32 bsp_disp_vga_get_mode(__u32 sel);
extern __s32 bsp_disp_vga_set_src(__u32 sel, __disp_lcdc_src_t src);

extern __s32 bsp_disp_cmu_layer_enable(__u32 sel,__u32 hid, __bool en);
extern __s32 bsp_disp_cmu_layer_get_enable(__u32 sel,__u32 hid);
extern __s32 bsp_disp_cmu_layer_set_window(__u32 sel, __u32 hid, __disp_rect_t *rect);
extern __s32 bsp_disp_cmu_layer_set_bright(__u32 sel, __u32 hid, __u32 bright);
extern __s32 bsp_disp_cmu_layer_get_bright(__u32 sel, __u32 hid);
extern __s32 bsp_disp_cmu_layer_set_saturation(__u32 sel, __u32 hid, __u32 saturation);
extern __s32 bsp_disp_cmu_layer_get_saturation(__u32 sel, __u32 hid);
extern __s32 bsp_disp_cmu_layer_set_hue(__u32 sel, __u32 hid, __u32 hue);
extern __s32 bsp_disp_cmu_layer_get_hue(__u32 sel, __u32 hid);
extern __s32 bsp_disp_cmu_layer_set_contrast(__u32 sel, __u32 hid, __u32 contrast);
extern __s32 bsp_disp_cmu_layer_get_contrast(__u32 sel, __u32 hid);
extern __s32 bsp_disp_cmu_layer_set_mode(__u32 sel, __u32 hid, __u32 mode);
extern __s32 bsp_disp_cmu_layer_get_mode(__u32 sel, __u32 hid);
extern __s32 bsp_disp_cmu_layer_set_window(__u32 sel, __u32 hid, __disp_rect_t *rect);
extern __s32 bsp_disp_cmu_layer_get_window(__u32 sel, __u32 hid, __disp_rect_t *rect);
extern __s32 bsp_disp_cmu_enable(__u32 sel,__bool en);
extern __s32 bsp_disp_cmu_get_enable(__u32 sel);
extern __s32 bsp_disp_cmu_set_window(__u32 sel,  __disp_rect_t *rect);
extern __s32 bsp_disp_cmu_get_window(__u32 sel,  __disp_rect_t *rect);
extern __s32 bsp_disp_cmu_set_bright(__u32 sel, __u32 bright);
extern __s32 bsp_disp_cmu_set_saturation(__u32 sel,__u32 saturation);
extern __s32 bsp_disp_cmu_set_hue(__u32 sel, __u32 hue);
extern __s32 bsp_disp_cmu_set_contrast(__u32 sel,__u32 contrast);
extern __s32 bsp_disp_cmu_get_bright(__u32 sel);
extern __s32 bsp_disp_cmu_get_saturation(__u32 sel);
extern __s32 bsp_disp_cmu_get_hue(__u32 sel);
extern __s32 bsp_disp_cmu_get_contrast(__u32 sel);
extern __s32 bsp_disp_cmu_set_mode(__u32 sel, __u32 mode);
extern __s32 bsp_disp_cmu_get_mode(__u32 sel);


extern __s32 bsp_disp_drc_get_enable(__u32 sel);
extern __s32 bsp_disp_drc_enable(__u32 sel, __u32 en);
extern __s32 bsp_disp_drc_set_window(__u32 sel, __disp_rect_t *rect);
extern __s32 bsp_disp_drc_get_window(__u32 sel, __disp_rect_t *rect);
extern __s32 bsp_disp_drc_set_mode(__u32 sel, __u32 mode);
extern __s32 bsp_disp_drc_get_mode(__u32 sel);
extern __s32 bsp_disp_iep_suspend(__u32 sel);
extern __s32 bsp_disp_iep_resume(__u32 sel);

extern __u32 disp_get_screen_scan_mode(__disp_tv_mode_t tv_mode);

extern __s32 bsp_disp_deu_enable(__u8 sel, __u32 hid,  __u32 enable);
extern __s32 bsp_disp_deu_get_enable(__u32 sel, __u32 hid);
extern __s32 bsp_disp_deu_set_luma_sharp_level(__u32 sel, __u32 hid,__u32 level);
extern __s32 bsp_disp_deu_get_luma_sharp_level(__u32 sel, __u32 hid);
extern __s32 bsp_disp_deu_set_chroma_sharp_level(__u32 sel, __u32 hid, __u32 level);
extern __s32 bsp_disp_deu_get_chroma_sharp_level(__u32 sel, __u32 hid);
extern __s32 bsp_disp_deu_set_white_exten_level(__u32 sel, __u32 hid, __u32 level);
extern __s32 bsp_disp_deu_get_white_exten_level(__u32 sel, __u32 hid);
extern __s32 bsp_disp_deu_set_black_exten_level(__u32 sel, __u32 hid, __u32 level);
extern __s32 bsp_disp_deu_get_black_exten_level(__u32 sel, __u32 hid);
extern __s32 bsp_disp_deu_set_window(__u32 sel, __u32 hid, __disp_rect_t *rect);
extern __s32 bsp_disp_deu_get_window(__u32 sel, __u32 hid, __disp_rect_t *rect);

int bsp_disp_feat_get_num_screens(void);
int bsp_disp_feat_get_num_layers(__u32 screen_id);
int bsp_disp_feat_get_num_scalers(void);
__disp_output_type_t bsp_disp_feat_get_supported_output_types(__u32 screen_id);
enum __disp_layer_feat bsp_disp_feat_get_layer_feats(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
enum __disp_enhance_feat bsp_disp_feat_get_enhance_feats(__u32 screen_id);
int bsp_disp_feat_get_layer_horizontal_resolution_max(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
int bsp_disp_feat_get_layer_vertical_resolution_max(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
int bsp_disp_feat_get_layer_horizontal_resolution_min(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
int bsp_disp_feat_get_layer_vertical_resolution_min(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
int bsp_disp_feat_get_de_flicker_support(__u32 screen_id);
int bsp_disp_feat_get_smart_backlight_support(__u32 screen_id);
int bsp_disp_feat_get_image_detail_enhance_support(__u32 screen_id);

extern __s32 iep_init(__u32 sel);
extern __s32 iep_exit(__u32 sel);

extern int TCON_get_open_status(__u32 sel);
extern __s32 bsp_disp_cfg_start(__u32 sel);
extern __s32 bsp_disp_cfg_finish(__u32 sel);

extern __s32 bsp_disp_delay_ms(__u32 ms);
extern __s32 bsp_disp_delay_us(__u32 us);

#ifdef __LINUX_OSAL__
__s32 Display_set_fb_timming(__u32 sel);
#endif

#endif

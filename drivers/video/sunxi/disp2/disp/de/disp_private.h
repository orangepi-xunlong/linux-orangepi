#ifndef _DISP_PRIVATE_H_
#define _DISP_PRIVATE_H_

#include "disp_features.h"
#if defined(CONFIG_ARCH_SUN50IW1P1)
#include "./lowlevel_sun50iw1/disp_al.h"
#elif defined(CONFIG_ARCH_SUN50IW2)
#include "./lowlevel_v2x/disp_al.h"
#elif defined(CONFIG_ARCH_SUN8IW10)
#include "./lowlevel_sun8iw10/disp_al.h"
#elif defined(CONFIG_ARCH_SUN8IW11)
#include "./lowlevel_v2x/disp_al.h"
#else
#error "undefined platform!!!"
#endif

extern struct disp_device* disp_get_lcd(u32 disp);

extern struct disp_device* disp_get_hdmi(u32 disp);

extern struct disp_manager* disp_get_layer_manager(u32 disp);

extern struct disp_layer* disp_get_layer(u32 disp, u32 chn, u32 layer_id);
extern struct disp_layer* disp_get_layer_1(u32 disp, u32 layer_id);
extern struct disp_smbl* disp_get_smbl(u32 disp);
extern struct disp_enhance* disp_get_enhance(u32 disp);
extern struct disp_capture* disp_get_capture(u32 disp);

extern s32 disp_delay_ms(u32 ms);
extern s32 disp_delay_us(u32 us);
extern s32 disp_init_lcd(disp_bsp_init_para * para);
extern s32 disp_init_hdmi(disp_bsp_init_para *para);
extern s32 disp_init_tv(void);//(disp_bsp_init_para * para);
extern s32 disp_tv_set_func(struct disp_device*  ptv, struct disp_tv_func * func);
extern s32 disp_init_tv_para(disp_bsp_init_para * para);
extern s32 disp_tv_set_hpd(struct disp_device*  ptv, u32 state);
extern s32 disp_init_vga(void);

extern s32 disp_init_feat(void);
extern s32 disp_init_mgr(disp_bsp_init_para * para);
extern s32 disp_init_enhance(disp_bsp_init_para * para);
extern s32 disp_init_smbl(disp_bsp_init_para * para);
extern s32 disp_init_capture(disp_bsp_init_para *para);

extern s32 disp_init_eink(disp_bsp_init_para * para);
extern s32 write_edma(struct disp_eink_manager*  manager);
extern s32 disp_init_format_convert_manager(disp_bsp_init_para *para);

extern struct disp_eink_manager* disp_get_eink_manager(unsigned int disp);

#include "disp_device.h"

u32 dump_layer_config(struct disp_layer_config_data *data);

#endif


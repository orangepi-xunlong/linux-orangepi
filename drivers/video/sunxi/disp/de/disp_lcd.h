
#ifndef __DISP_LCD_H__
#define __DISP_LCD_H__

#include "disp_private.h"

#define LCD_GPIO_SCL (LCD_GPIO_NUM-2)
#define LCD_GPIO_SDA (LCD_GPIO_NUM-1)
#define LCD_GPIO_NUM 6
#define LCD_POWER_NUM 4
#define LCD_GPIO_REGU_NUM 3
typedef struct
{
	bool                  lcd_used;

	bool                  lcd_bl_en_used;
	disp_gpio_set_t       lcd_bl_en;
	char                  lcd_bl_regulator[25];

	u32                   lcd_power_type[LCD_POWER_NUM];/* 0: invalid, 1: gpio, 2: regulator */
	disp_gpio_set_t       lcd_power[LCD_POWER_NUM];
	char                  lcd_regu[LCD_POWER_NUM][25];

	bool                  lcd_gpio_used[LCD_GPIO_NUM];  //index4: scl;  index5: sda
	disp_gpio_set_t       lcd_gpio[LCD_GPIO_NUM];       //index4: scl; index5: sda
	u32                   gpio_hdl[LCD_GPIO_NUM];
	char                  lcd_gpio_regulator[LCD_GPIO_REGU_NUM][25];

	bool                  lcd_io_used[28];
	disp_gpio_set_t       lcd_io[28];
	char                  lcd_io_regulator[25];

	u32                   backlight_bright;
	u32                   backlight_dimming;//IEP-drc backlight dimming rate: 0 -256 (256: no dimming; 0: the most dimming)
	u32                   backlight_curve_adjust[101];

	u32                   lcd_bright;
	u32                   lcd_contrast;
	u32                   lcd_saturation;
	u32                   lcd_hue;
}__disp_lcd_cfg_t;

s32 disp_init_lcd(__disp_bsp_init_para * para);
s32 disp_lcd_gpio_init(struct disp_lcd* lcd);
s32 disp_lcd_gpio_exit(struct disp_lcd* lcd);
s32 disp_lcd_gpio_get_value(struct disp_lcd* lcd,__u32 io_index);

#endif

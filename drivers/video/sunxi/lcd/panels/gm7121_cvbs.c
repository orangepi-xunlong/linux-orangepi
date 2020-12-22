#include "panels.h"

extern s32 gm7121_tv_power_on(u32 on_off);
extern s32 gm7121_tv_open(void);
extern s32 gm7121_tv_close(void);
extern s32 gm7121_tv_get_mode(void);
extern s32 gm7121_tv_set_mode(disp_tv_mode tv_mode);
extern s32 gm7121_tv_get_mode_support(disp_tv_mode tv_mode);

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

static void LCD_cfg_panel_info(panel_extend_para * info)
{
	u32 i = 0, j=0;
	u32 items;
	u8 lcd_gamma_tbl[][2] =
	{
		//{input value, corrected value}
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	u8 lcd_bright_curve_tbl[][2] =
	{
		//{input value, corrected value}
		{0    ,0  },//0
		{15   ,3  },//0
		{30   ,6  },//0
		{45   ,9  },// 1
		{60   ,12  },// 2
		{75   ,16  },// 5
		{90   ,22  },//9
		{105   ,28 }, //15
		{120  ,36 },//23
		{135  ,44 },//33
		{150  ,54 },
		{165  ,67 },
		{180  ,84 },
		{195  ,108},
		{210  ,137},
		{225 ,171},
		{240 ,210},
		{255 ,255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	{
		{LCD_CMAP_G0,LCD_CMAP_B1,LCD_CMAP_G2,LCD_CMAP_B3},
		{LCD_CMAP_B0,LCD_CMAP_R1,LCD_CMAP_B2,LCD_CMAP_R3},
		{LCD_CMAP_R0,LCD_CMAP_G1,LCD_CMAP_R2,LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3,LCD_CMAP_G2,LCD_CMAP_B1,LCD_CMAP_G0},
		{LCD_CMAP_R3,LCD_CMAP_B2,LCD_CMAP_R1,LCD_CMAP_B0},
		{LCD_CMAP_G3,LCD_CMAP_R2,LCD_CMAP_G1,LCD_CMAP_R0},
		},
	};

	memset(info,0,sizeof(panel_extend_para));

	items = sizeof(lcd_gamma_tbl)/2;
	for(i=0; i<items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for(j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) + (lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];

	items = sizeof(lcd_bright_curve_tbl)/2;
	for(i=0; i<items-1; i++) {
		u32 num = lcd_bright_curve_tbl[i+1][0] - lcd_bright_curve_tbl[i][0];

		for(j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_bright_curve_tbl[i][1] + ((lcd_bright_curve_tbl[i+1][1] - lcd_bright_curve_tbl[i][1]) * j)/num;
			info->lcd_bright_curve_tbl[lcd_bright_curve_tbl[i][0] + j] = value;
		}
	}
	info->lcd_bright_curve_tbl[255] = lcd_bright_curve_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 200);
	LCD_OPEN_FUNC(sel, LCD_panel_init, 100);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);
	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 50);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 50);
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 0);
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);
	return 0;
}

static void LCD_power_on(u32 sel)
{
	sunxi_lcd_gpio_set_value(sel, 0, 0);
	gm7121_tv_power_on(1);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	gm7121_tv_power_on(0);
}

static void LCD_bl_open(u32 sel)
{
	gm7121_tv_open();
}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_gpio_set_value(sel, 0, 0);
}

static void LCD_panel_init(u32 sel)
{
	sunxi_lcd_gpio_set_value(sel, 0, 1);
	return;
}

static void LCD_panel_exit(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	return ;
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	s32 ret = -1;
	switch(para1){
		case 0: //tv_get_mode
			ret = gm7121_tv_get_mode();
			break;
		case 1: //tv_set_mode
			ret = gm7121_tv_set_mode(para2);
			break;
		case 2: //tv_get_mode_support
			ret = gm7121_tv_get_mode_support(para2);
			break;
		default:
			break;
	}
	return ret;
}

__lcd_panel_t gm7121_cvbs = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "gm7121",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};

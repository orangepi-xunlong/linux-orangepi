#include "tg078uw004a0.h"

extern s32 bsp_disp_get_panel_info(u32 screen_id, disp_panel_para *info);
static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_backlight_open(u32 sel);
static void lcd_backlight_close(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

static u8 const mipi_dcs_pixel_format[4] = { 0x77, 0x66, 0x66, 0x55 };

#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 1, val)
#define power_en(val)  sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;

	/* {input value, corrected value} */
	u8 lcd_gamma_tbl[][2] = {
		{  0,   0}, { 15,  15}, { 30,  30}, { 45,  45},
		{ 60,  60}, { 75,  75}, { 90,  90}, {105, 105},
		{120, 120}, {135, 135}, {150, 150}, {165, 165},
		{180, 180}, {195, 195}, {210, 210}, {225, 225},
		{240, 240}, {255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
		{
			{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
			{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
			{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		}, {
			{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
			{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
			{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
				((lcd_gamma_tbl[i + 1][1] - lcd_gamma_tbl[i][1]) * j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
				(value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] =
		(lcd_gamma_tbl[items - 1][1] << 16) +
		(lcd_gamma_tbl[items - 1][1] << 8) +
		lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
}

static s32 lcd_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, lcd_power_on, 100);
	LCD_OPEN_FUNC(sel, lcd_panel_init, 50);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 20);
	LCD_OPEN_FUNC(sel, lcd_backlight_open, 0);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, lcd_backlight_close, 0);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 20);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 10);

	return 0;
}

static void lcd_power_on(u32 sel)
{
#if 0
	sunxi_lcd_gpio_set_value(sel, 0, 1); /* reset = 1 */
#endif
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_power_enable(sel, 1);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_power_enable(sel, 2);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_pin_cfg(sel, 1);
#if 0
	sunxi_lcd_gpio_set_value(sel, 0, 0);	/* reset = 1 */
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_gpio_set_value(sel, 0, 1);	/* reset = 0 */
	sunxi_lcd_delay_ms(150);
#endif
}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_delay_ms(100);

	/*
	sunxi_lcd_gpio_set_value(sel, 0, 0);
	sunxi_lcd_delay_ms(5);
	*/
	sunxi_lcd_power_disable(sel, 2);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_power_disable(sel, 1);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_power_disable(sel, 0);
}

static void lcd_backlight_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);
}

static void lcd_backlight_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
}

static void load_config_to_tg078uw004a0(u32 sel)
{
	int i;
	int size = sizeof(panel_config) / sizeof(*panel_config);

	for (i = 0; i < size; i++) {
		if (panel_config[i].command == 0x1FF)
			sunxi_lcd_delay_ms(panel_config[i].param[0]);

		if (panel_config[i].para_num == 1) {
			sunxi_lcd_dsi_dcs_write_1para(sel,
					panel_config[i].command,
					panel_config[i].param[0]);
		} else if (panel_config[i].para_num > 1) {
			sunxi_lcd_dsi_dcs_write(sel,
					panel_config[i].command,
					(unsigned char *) panel_config[i].param,
					panel_config[i].para_num);
		}
	}
}

static void lcd_panel_init(u32 sel)
{
	disp_panel_para *panel_info = disp_sys_malloc(sizeof(disp_panel_para));

	bsp_disp_get_panel_info(sel, panel_info);
	sunxi_lcd_dsi_clk_enable(sel);

	load_config_to_tg078uw004a0(sel);
	sunxi_lcd_delay_ms(50);
#if 0
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x11);
	sunxi_lcd_delay_ms(150);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x29);
	sunxi_lcd_delay_ms(50);
#endif

	disp_sys_free(panel_info);

	return;
}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SET_DISPLAY_OFF);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_ENTER_SLEEP_MODE);
	sunxi_lcd_delay_ms(20);

	return;
}

/* sel: 0:lcd0; 1:lcd1 */
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

/* sel: 0:lcd0; 1:lcd1 */
static s32 lcd_set_bright(u32 sel, u32 bright)
{
	/* sunxi_lcd_dsi_dcs_write_1para(sel,0x51,bright); */
	return 0;
}

__lcd_panel_t tg078uw004a0_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in
	   sys_config.fex */
	.name = "tg078uw004a0",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
		.cfg_open_flow = lcd_open_flow,
		.cfg_close_flow = lcd_close_flow,
		.lcd_user_defined_func = lcd_user_defined_func,
		.set_bright = lcd_set_bright,
	},
};

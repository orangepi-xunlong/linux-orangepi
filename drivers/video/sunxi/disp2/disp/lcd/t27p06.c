#include "t27p06.h"

#define t27p06_spi_scl_1	sunxi_lcd_gpio_set_value(0,2,1)
#define t27p06_spi_scl_0	sunxi_lcd_gpio_set_value(0,2,0)
#define t27p06_spi_sdi_1	sunxi_lcd_gpio_set_value(0,1,1)
#define t27p06_spi_sdi_0	sunxi_lcd_gpio_set_value(0,1,0)
#define t27p06_spi_cs_1  	sunxi_lcd_gpio_set_value(0,0,1)
#define t27p06_spi_cs_0  	sunxi_lcd_gpio_set_value(0,0,0)

static void t27p06_spi_wr(u32 value);
static void lcd_panel_t27p06_init(void);
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

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 10);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 10);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 150);     //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 50);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	10);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 10);   //close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(u32 sel)
{
	sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power0
	sunxi_lcd_gpio_set_value(sel, 3, 0);//pwr_en, active low
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_gpio_set_value(sel, 3, 1);//pwr_en, active low
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power0
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	sunxi_lcd_pwm_disable(sel);
}

static void LCD_panel_init(u32 sel)
{
	disp_panel_para *info = disp_sys_malloc(sizeof(disp_panel_para));
    	bsp_disp_get_panel_info(sel, info);
  	lcd_panel_t27p06_init();
	disp_sys_free(info);
	return;
}

static void LCD_panel_exit(u32 sel)
{
	disp_panel_para *info = disp_sys_malloc(sizeof(disp_panel_para));
	bsp_disp_get_panel_info(sel, info);
	disp_sys_free(info);
	return ;
}

static void t27p06_spi_wr(u32 value)
{
	u32 i;
	t27p06_spi_cs_1;
	t27p06_spi_sdi_1;
	t27p06_spi_scl_1;
	sunxi_lcd_delay_us(10);
	t27p06_spi_cs_0; 
	sunxi_lcd_delay_us(10);
	for(i=0;i<16;i++)
	{
		sunxi_lcd_delay_us(10);
		t27p06_spi_scl_0;
		if(value & 0x8000)
			t27p06_spi_sdi_1;
		else
			t27p06_spi_sdi_0;
		value <<= 1;
		sunxi_lcd_delay_us(10);
		t27p06_spi_scl_1;
	}
	sunxi_lcd_delay_us(10);
	t27p06_spi_sdi_1;
	t27p06_spi_cs_1;
}
static void lcd_panel_t27p06_init(void)
{

	int need_special_process = 0;
#ifdef CONFIG_LCD_ABT_320X480
		need_special_process = 1;
#endif

	if (!need_special_process) {
		t27p06_spi_wr(0x055f);
		sunxi_lcd_delay_ms(5);
		t27p06_spi_wr(0x051f);//reset
		sunxi_lcd_delay_ms(10);
		t27p06_spi_wr(0x055f);
		sunxi_lcd_delay_ms(50);
		t27p06_spi_wr(0x2b01);//exit standby mode
		t27p06_spi_wr(0x0009);//vcomac
		t27p06_spi_wr(0x019f);//vcomdc
	//	t27p06_spi_wr(0x040b);//8-bit rgb interface
		t27p06_spi_wr(0x040c);//8-bit rgb interface
		t27p06_spi_wr(0x1604);//default gamma setting  2.2
	} else {//asia better lcd 320x480
		t27p06_spi_wr(0x0500);
		t27p06_spi_wr(0x7006);
		t27p06_spi_wr(0x54d0);
		t27p06_spi_wr(0x0177);
		t27p06_spi_wr(0x0614);//04
		t27p06_spi_wr(0x0820);
		//t27p06_spi_wr(0x0b03);//before 180 rotate
		t27p06_spi_wr(0x0b00);//180 rotate

		t27p06_spi_wr(0x1401);
		t27p06_spi_wr(0x031f);
		t27p06_spi_wr(0x041f);
		//t27p06_spi_wr(0x1060);

		t27p06_spi_wr(0x7110);
		t27p06_spi_wr(0x7b82);

		//t27p06_spi_wr(0x0CAA);
		//t27p06_spi_wr(0x0D26);

		t27p06_spi_wr(0x1911);
		t27p06_spi_wr(0x1a0c);
		t27p06_spi_wr(0x1b14);
		t27p06_spi_wr(0x1c0e);
		t27p06_spi_wr(0x1d10);
		t27p06_spi_wr(0x1e0c);
		t27p06_spi_wr(0x1fa8);

		t27p06_spi_wr(0x2085);
		t27p06_spi_wr(0x218b);
		t27p06_spi_wr(0x22bd);
		t27p06_spi_wr(0x230f);
		t27p06_spi_wr(0x2413);
		t27p06_spi_wr(0x2511);
		t27p06_spi_wr(0x2613);
		t27p06_spi_wr(0x270a);
		t27p06_spi_wr(0x2803);
		t27p06_spi_wr(0x2903);

		t27p06_spi_wr(0x2a0a);
		t27p06_spi_wr(0x2b13);
		t27p06_spi_wr(0x2c11);
		t27p06_spi_wr(0x2d13);
		t27p06_spi_wr(0x2e0f);
		t27p06_spi_wr(0x2f0c);

		t27p06_spi_wr(0x3010);
		t27p06_spi_wr(0x310e);
		t27p06_spi_wr(0x3214);
		t27p06_spi_wr(0x330c);
		t27p06_spi_wr(0x3422);
		t27p06_spi_wr(0x0581);
	}
}


//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

__lcd_panel_t t27p06_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "t27p06",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};

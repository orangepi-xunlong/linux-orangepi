#include "st7789s.h"


static void lcd_panel_st7789s_init(void);
static void lcd_cpu_panel_fr(__u32 sel,__u32 w,__u32 h,__u32 x,__u32 y);
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
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 150);     //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 10);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 50);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	10);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);         //close lcd controller, and delay 0ms
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
	lcd_panel_st7789s_init();
	sunxi_lcd_cpu_set_auto_mode(sel);
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

static void lcd_dbi_wr_dcs(__u32 sel,__u8 cmd,__u8* para,__u32 para_num)
{
	__u8 index		= cmd;
	__u8* data_p	= para;
	__u16 i;
	sunxi_lcd_cpu_write_index(sel,index);
	for(i=0;i<para_num;i++)
	{
		sunxi_lcd_cpu_write_data(sel,*(data_p++));
	}
}

static void lcd_cpu_panel_fr(__u32 sel,__u32 w,__u32 h,__u32 x,__u32 y)
{
	__u8 para[4];
	__u32 para_num;
	para[0] = (x>>8)		& 0xff;
	para[1] = (x>>0) 		& 0xff;
	para[2] = ((x+w-1)>>8) 	& 0xff;
	para[3] = ((x+w-1)>>0) 	& 0xff;
	para_num = 4;
	lcd_dbi_wr_dcs(sel,DSI_DCS_SET_COLUMN_ADDRESS,para,para_num);

	para[0] = (y>>8)		& 0xff;
	para[1] = (y>>0) 		& 0xff;
	para[2] = ((y+h-1)>>8) 	& 0xff;
	para[3] = ((y+h-1)>>0) 	& 0xff;
	para_num = 4;
	lcd_dbi_wr_dcs(sel,DSI_DCS_SET_PAGE_ADDRESS,para,para_num);

	para_num = 0;
	lcd_dbi_wr_dcs(sel,DSI_DCS_WRITE_MEMORY_START,para,para_num);
}

static void lcd_panel_st7789s_init(void)
{
	u32 x = 320, y = 240, sel = 0;

	sunxi_lcd_cpu_write_index(0,0x11);
	sunxi_lcd_delay_ms(120); //Delay 120ms
	//-------------Display and color format setting---------------
	sunxi_lcd_cpu_write_index(0,0x36);
	sunxi_lcd_cpu_write_data(0,0x60);


	sunxi_lcd_cpu_write_index(0,0x2A);
	sunxi_lcd_cpu_write_data(0,0x00);
	sunxi_lcd_cpu_write_data(0,0x00);
	sunxi_lcd_cpu_write_data(0,0x01);
	sunxi_lcd_cpu_write_data(0,0x3f);

	sunxi_lcd_cpu_write_index(0,0x2B);
	sunxi_lcd_cpu_write_data(0,0x00);
	sunxi_lcd_cpu_write_data(0,0x00);
	sunxi_lcd_cpu_write_data(0,0x00);
	sunxi_lcd_cpu_write_data(0,0xef);

	sunxi_lcd_cpu_write_index(0,0x3a);	//262k,?????0X66, 65K,?????0X55
	sunxi_lcd_cpu_write_data(0,0x66);
	//----------ST7789S Frame rate setting----------

	sunxi_lcd_cpu_write_index(0,0xb2);
	sunxi_lcd_cpu_write_data(0,0x08);
	sunxi_lcd_cpu_write_data(0,0x08);
	sunxi_lcd_cpu_write_data(0,0x00);
	sunxi_lcd_cpu_write_data(0,0x22);
	sunxi_lcd_cpu_write_data(0,0x22);

	sunxi_lcd_cpu_write_index(0,0xb7);
	sunxi_lcd_cpu_write_data(0,0x35 );
	//----------ST7789S Power setting-----------------

	sunxi_lcd_cpu_write_index(0,0xbb);
	sunxi_lcd_cpu_write_data(0,0x27 );

	//sunxi_lcd_cpu_write_index(0,0xc0);
	//sunxi_lcd_cpu_write_data(0,0x6e ); //0x2c

	sunxi_lcd_cpu_write_index(0,0xc2);
	sunxi_lcd_cpu_write_data(0,0x01 );

	sunxi_lcd_cpu_write_index(0,0xc3);
	sunxi_lcd_cpu_write_data(0,0x0b );

	sunxi_lcd_cpu_write_index(0,0xc4);
	sunxi_lcd_cpu_write_data(0,0x20 );

	sunxi_lcd_cpu_write_index(0,0xc6);
	sunxi_lcd_cpu_write_data(0,0x0f );

	sunxi_lcd_cpu_write_index(0,0xd0);
	sunxi_lcd_cpu_write_data(0,0xa4 );
	sunxi_lcd_cpu_write_data(0,0xa1 );

	//-----------ST7789S gamma setting----------------
	sunxi_lcd_cpu_write_index(0,0xe0);
	sunxi_lcd_cpu_write_data(0,0xd0);
	sunxi_lcd_cpu_write_data(0,0x00);
	sunxi_lcd_cpu_write_data(0,0x02);
	sunxi_lcd_cpu_write_data(0,0x07);
	sunxi_lcd_cpu_write_data(0,0x07);
	sunxi_lcd_cpu_write_data(0,0x19); 
	sunxi_lcd_cpu_write_data(0,0x2e);
	sunxi_lcd_cpu_write_data(0,0x54);
	sunxi_lcd_cpu_write_data(0,0x41);
	sunxi_lcd_cpu_write_data(0,0x2d);
	sunxi_lcd_cpu_write_data(0,0x17);
	sunxi_lcd_cpu_write_data(0,0x18);
	sunxi_lcd_cpu_write_data(0,0x14);
	sunxi_lcd_cpu_write_data(0,0x18);

	sunxi_lcd_cpu_write_index(0,0xe1);
	sunxi_lcd_cpu_write_data(0,0xd0);
	sunxi_lcd_cpu_write_data(0,0x00);
	sunxi_lcd_cpu_write_data(0,0x02);
	sunxi_lcd_cpu_write_data(0,0x07);
	sunxi_lcd_cpu_write_data(0,0x04);
	sunxi_lcd_cpu_write_data(0,0x24);
	sunxi_lcd_cpu_write_data(0,0x2c);
	sunxi_lcd_cpu_write_data(0,0x44);
	sunxi_lcd_cpu_write_data(0,0x42);
	sunxi_lcd_cpu_write_data(0,0x1c);
	sunxi_lcd_cpu_write_data(0,0x1a);
	sunxi_lcd_cpu_write_data(0,0x17);
	sunxi_lcd_cpu_write_data(0,0x15);
	sunxi_lcd_cpu_write_data(0,0x18);

	sunxi_lcd_cpu_write_index(0,0x29);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_cpu_write_index(0,0x2C);
	lcd_cpu_panel_fr(sel, x, y, 0, 0);
}
//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

__lcd_panel_t st7789s_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "st7789s",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};

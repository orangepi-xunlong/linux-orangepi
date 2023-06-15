/*
 * drivers/video/sunxi/disp2/disp/lcd/K080_IM2HYL802R_800X1280.c
 *
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "K080_IM2HYL802R_800X1280.h"
//#include <mach/sys_config.h>
#include "panels.h"


/*
&lcd0 {
	lcd_used            = <1>;
	status              = "okay";
	lcd_driver_name     = "K080_IM2HYL802R_800X1280";
	lcd_backlight       = <50>;
	lcd_if              = <4>;

	lcd_x               = <800>;
	lcd_y               = <1280>;
	lcd_width           = <135>;
	lcd_height          = <216>;
	lcd_dclk_freq       = <75>;

	lcd_pwm_used        = <1>;
	lcd_pwm_ch          = <0>;
	lcd_pwm_freq        = <50000>;
	lcd_pwm_pol         = <1>;
	lcd_pwm_max_limit   = <255>;

	lcd_hbp             = <88>;
	lcd_ht              = <960>;
	lcd_hspw            = <4>;
	lcd_vbp             = <12>;
	lcd_vt              = <1300>;
	lcd_vspw            = <4>;

	lcd_frm             = <0>;
	lcd_gamma_en        = <0>;
	lcd_bright_curve_en = <0>;
	lcd_cmap_en         = <0>;

	deu_mode            = <0>;
	lcdgamma4iep        = <22>;
	smart_color         = <90>;

	lcd_dsi_if          = <0>;
	lcd_dsi_lane        = <4>;
	lcd_dsi_format      = <0>;
	lcd_dsi_te          = <0>;
	lcd_dsi_eotp        = <0>;

	lcd_pin_power = "dcdc1";
	lcd_pin_power1 = "eldo3";

	lcd_power = "eldo3";

	lcd_power1 = "dcdc1";
	lcd_power2 = "dc1sw";

	lcd_gpio_1 = <&pio PD 22 1 0 3 1>;

	pinctrl-0 = <&dsi4lane_pins_a>;
	pinctrl-1 = <&dsi4lane_pins_b>;

	lcd_bl_en = <&pio PB 8 1 0 3 1>;
	lcd_bl_0_percent	= <15>;
	lcd_bl_100_percent  = <100>;
};

*/


extern s32 bsp_disp_get_panel_info(u32 screen_id, struct disp_panel_para *info);
static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

//static u8 const mipi_dcs_pixel_format[4] = {0x77,0x66,0x66,0x55};
#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 1, val)
#define power_en(val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void LCD_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
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
		{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
		{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
		{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		},
		{
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

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i + 1][1] - lcd_gamma_tbl[i][1]) * j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) + (lcd_gamma_tbl[items - 1][1] << 8) + lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 100);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 200);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);     //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 200);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 20);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	10);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);   //close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(u32 sel)
{
	panel_reset(0);
	sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 1);//config lcd_power pin to open lcd power1
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 2);//config lcd_power pin to open lcd power2
	sunxi_lcd_delay_ms(5);
	power_en(1);
	sunxi_lcd_delay_ms(20);

	//panel_reset(1);
	sunxi_lcd_delay_ms(40);
	panel_reset(1);
	//sunxi_lcd_delay_ms(10);
	//panel_reset(0);
	//sunxi_lcd_delay_ms(5);
	//panel_reset(1);
	sunxi_lcd_delay_ms(120);
	//sunxi_lcd_delay_ms(5);

	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	power_en(0);
	sunxi_lcd_delay_ms(20);
	panel_reset(0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 2);//config lcd_power pin to close lcd power2
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 1);//config lcd_power pin to close lcd power1
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_pwm_disable(sel);
}

#define REGFLAG_DELAY             						0XFE
#define REGFLAG_END_OF_TABLE      						0xFD   // END OF REGISTERS MARKER

struct LCM_setting_table {
    u8 cmd;
    u32 count;
    u8 para_list[64];
};

/*add panel initialization below*/


static struct LCM_setting_table lcm_initialization_setting[] = {
  {0xFF,    3,     {0x98, 0x81, 0x03} },

//GIP_1

  {0x01,    1,     {0x00} },
  {0x02,    1,     {0x00} },
  {0x03,    1,     {0x73} },     //STA Width 4H
  {0x04,    1,     {0x00} },
  {0x05,    1,     {0x00} },
  {0x06,    1,     {0x0A} },     //STVA Rise start
  {0x07,    1,     {0x00} },
  {0x08,    1,     {0x00} },
  {0x09,    1,     {0x20} },     //Detail A&B 3.5-4.0us
  {0x0A,    1,     {0x20} },     //Detail A&B 3.5-4.0us
  {0x0B,    1,     {0x00} },
  {0x0C,    1,     {0x00} },
  {0x0D,    1,     {0x00} },
  {0x0E,    1,     {0x00} },
  {0x0F,    1,     {0x20} },     //Detail A&B 3.5-4.0us
  {0x10,    1,     {0x20} },     //Detail A&B 3.5-4.0us
  {0x11,    1,     {0x00} },
  {0x12,    1,     {0x00} },
  {0x13,    1,     {0x00} },
  {0x14,    1,     {0x00} },
  {0x15,    1,     {0x00} },
  {0x16,    1,     {0x00} },
  {0x17,    1,     {0x00} },
  {0x18,    1,     {0x00} },
  {0x19,    1,     {0x00} },
  {0x1A,    1,     {0x00} },
  {0x1B,    1,     {0x00} },
  {0x1C,    1,     {0x00} },
  {0x1D,    1,     {0x00} },
  {0x1E,    1,     {0x40} },
  {0x1F,    1,     {0x80} },
  {0x20,    1,     {0x06} },    //CLKA Rise START
  {0x21,    1,     {0x01} },    //CLKA FALL END
  {0x22,    1,     {0x00} },
  {0x23,    1,     {0x00} },
  {0x24,    1,     {0x00} },
  {0x25,    1,     {0x00} },
  {0x26,    1,     {0x00} },
  {0x27,    1,     {0x00} },
  {0x28,    1,     {0x33} },    //CLK_x_Numb[2:0]  Phase_CLK[2:0]
  {0x29,    1,     {0x03} },    //Overlap_CLK[3:0]
  {0x2A,    1,     {0x00} },
  {0x2B,    1,     {0x00} },
  {0x2C,    1,     {0x00} },
  {0x2D,    1,     {0x00} },
  {0x2E,    1,     {0x00} },
  {0x2F,    1,     {0x00} },
  {0x30,    1,     {0x00} },
  {0x31,    1,     {0x00} },
  {0x32,    1,     {0x00} },
  {0x33,    1,     {0x00} },
  {0x34,    1,     {0x04} },     //04 GPWR1/2 non overlap time 2.62us
  {0x35,    1,     {0x00} },
  {0x36,    1,     {0x00} },
  {0x37,    1,     {0x00} },
  {0x38,    1,     {0x3C} },     //78 FOR GPWR1/2 cycle 2 s
  {0x39,    1,     {0x00} },
  {0x3A,    1,     {0x00} },
  {0x3B,    1,     {0x00} },
  {0x3C,    1,     {0x00} },
  {0x3D,    1,     {0x00} },
  {0x3E,    1,     {0x00} },
  {0x3F,    1,     {0x00} },
  {0x40,    1,     {0x00} },
  {0x41,    1,     {0x00} },
  {0x42,    1,     {0x00} },
  {0x43,    1,     {0x00} },
  {0x44,    1,     {0x00} },


//GIP_2
  {0x50,    1,     {0x10} },
  {0x51,    1,     {0x32} },
  {0x52,    1,     {0x54} },
  {0x53,    1,     {0x76} },
  {0x54,    1,     {0x98} },
  {0x55,    1,     {0xba} },
  {0x56,    1,     {0x10} },
  {0x57,    1,     {0x32} },
  {0x58,    1,     {0x54} },
  {0x59,    1,     {0x76} },
  {0x5A,    1,     {0x98} },
  {0x5B,    1,     {0xba} },
  {0x5C,    1,     {0xdc} },
  {0x5D,    1,     {0xfe} },

//G0xIP_3
  {0x5E,    1,     {0x00} },
  {0x5F,    1,     {0x01} },      //GOUT1_FW
  {0x60,    1,     {0x00} },      //GOUT2_BW
  {0x61,    1,     {0x15} },      //GOUT3_GPWR1
  {0x62,    1,     {0x14} },      //GOUT4_GPWR2
  {0x63,    1,     {0x0E} },      //GOUT5_CLK1_R
  {0x64,    1,     {0x0F} },      //GOUT6_CLK2_R
  {0x65,    1,     {0x0C} },      //GOUT7_CLK3_R
  {0x66,    1,     {0x0D} },      //GOUT8_CLK4_R
  {0x67,    1,     {0x06} },      //GOUT9_STV1_R
  {0x68,    1,     {0x02} },
  {0x69,    1,     {0x02} },
  {0x6A,    1,     {0x02} },
  {0x6B,    1,     {0x02} },
  {0x6C,    1,     {0x02} },
  {0x6D,    1,     {0x02} },
  {0x6E,    1,     {0x07} },       //GOUT16_STV2_R
  {0x6F,    1,     {0x02} },       //GOUT17_VGL
  {0x70,    1,     {0x02} },       //GOUT18_VGL
  {0x71,    1,     {0x02} },       //GOUT19_VGL
  {0x72,    1,     {0x02} },
  {0x73,    1,     {0x02} },
  {0x74,    1,     {0x02} },

  {0x75,    1,     {0x01} },       //FW
  {0x76,    1,     {0x00} },       //BW
  {0x77,    1,     {0x14} },       //GPWR1
  {0x78,    1,     {0x15} },       //GPWR2
  {0x79,    1,     {0x0E} },       //CLK1_R
  {0x7A,    1,     {0x0F} },       //CLK2_R
  {0x7B,    1,     {0x0C} },       //CLK3_R
  {0x7C,    1,     {0x0D} },       //CLK4_R
  {0x7D,    1,     {0x06} },       //STV1_R
  {0x7E,    1,     {0x02} },
  {0x7F,    1,     {0x02} },
  {0x80,    1,     {0x02} },
  {0x81,    1,     {0x02} },
  {0x82,    1,     {0x02} },
  {0x83,    1,     {0x02} },
  {0x84,    1,     {0x07} },       //STV2_R
  {0x85,    1,     {0x02} },       //VGL
  {0x86,    1,     {0x02} },       //VGL
  {0x87,    1,     {0x02} },       //VGL
  {0x88,    1,     {0x02} },
  {0x89,    1,     {0x02} },
  {0x8A,    1,     {0x02} },

//CMD_Page 4
  {0xFF,    3,     {0x98, 0x81, 0x04} },
  {0x6C,    1,     {0x15} },        //Set VCORE voltage =1.5V
  {0x6E,    1,     {0x2A} },        //di_pwr_reg=0 for power mode 2A //VGH clamp 15V
  {0x6F,    1,     {0x33} },        // reg vcl + pumping ratio VGH=3x VGL=-2.5x
  {0x3A,    1,     {0x92} },        //POWER SAVING
  {0x8D,    1,     {0x1A} },        //VGL clamp -11V
  {0x87,    1,     {0xBA} },        //ESD
  {0x26,    1,     {0x76} },
  {0xB2,    1,     {0xD1} },
  {0xB5,    1,     {0x27} },        //GMA BIAS
  {0x31,    1,     {0x75} },        //SRC BIAS
  {0x30,    1,     {0x03} },        //SRC OUTPUT BIAS
  {0x3B,    1,     {0x98} },        //PUMP SHIFT CLK
  {0x35,    1,     {0x17} },        //HZ_opt
  {0x33,    1,     {0x14} },        //Blanking frame 設定為GND
  {0x38,    1,     {0x01} },
  {0x39,    1,     {0x00} },


//CMD_Page 1
  {0xFF,    3,     {0x98, 0x81, 0x01} },
  {0x22,    1,     {0x0A} },		//BGR, SS
  {0x31,    1,     {0x00} },		//column inversion
  {0x53,    1,     {0x3C} },		//VCOM1
  {0x55,    1,     {0x8F} },		//VCOM2
  {0x50,    1,     {0xC0} },		//VREG1OUT=5V
  {0x51,    1,     {0xC0} },		//VREG2OUT=-5V
  {0x60,    1,     {0x14} },               //SDT



  {0xA0,    1,     {0x08} },		//VP255	Gamma P
  {0xA1,    1,     {0x10} },               //VP251
  {0xA2,    1,     {0x25} },               //VP247
  {0xA3,    1,     {0x00} },               //VP243
  {0xA4,    1,     {0x24} },               //VP239
  {0xA5,    1,     {0x19} },               //VP231
  {0xA6,    1,     {0x12} },               //VP219
  {0xA7,    1,     {0x1B} },               //VP203
  {0xA8,    1,     {0x77} },               //VP175
  {0xA9,    1,     {0x19} },               //VP144
  {0xAA,    1,     {0x25} },               //VP111
  {0xAB,    1,     {0x6E} },               //VP80
  {0xAC,    1,     {0x20} },               //VP52
  {0xAD,    1,     {0x17} },               //VP36
  {0xAE,    1,     {0x54} },               //VP24
  {0xAF,    1,     {0x24} },               //VP16
  {0xB0,    1,     {0x27} },               //VP12
  {0xB1,    1,     {0x52} },               //VP8
  {0xB2,    1,     {0x63} },               //VP4
  {0xB3,    1,     {0x39} },               //VP0

  {0xC0,    1,     {0x08} },		//VN255 GAMMA N
  {0xC1,    1,     {0x20} },               //VN251
  {0xC2,    1,     {0x23} },               //VN247
  {0xC3,    1,     {0x22} },               //VN243
  {0xC4,    1,     {0x06} },               //VN239
  {0xC5,    1,     {0x34} },               //VN231
  {0xC6,    1,     {0x25} },               //VN219
  {0xC7,    1,     {0x20} },               //VN203
  {0xC8,    1,     {0x86} },               //VN175
  {0xC9,    1,     {0x1F} },               //VN144
  {0xCA,    1,     {0x2B} },               //VN111
  {0xCB,    1,     {0x74} },               //VN80
  {0xCC,    1,     {0x16} },               //VN52
  {0xCD,    1,     {0x1B} },               //VN36
  {0xCE,    1,     {0x46} },               //VN24
  {0xCF,    1,     {0x21} },               //VN16
  {0xD0,    1,     {0x29} },               //VN12
  {0xD1,    1,     {0x54} },               //VN8
  {0xD2,    1,     {0x65} },               //VN4
  {0xD3,    1,     {0x39} },               //VN0

//CMD_Page 0
  {0xFF,    3,     {0x98, 0x81, 0x00} },

  //SLP OUT
  {0x11,     0,     {0x00} },  	// SLPOUT
  {REGFLAG_DELAY, REGFLAG_DELAY, {120} },

  //DISP ON
  {0x29,     0,     {0x00} },  	// DSPON
  {REGFLAG_DELAY, REGFLAG_DELAY, {5} },

  //TE ON
  {0x35,     0,     {0x00} },  	// TE
  {REGFLAG_DELAY, REGFLAG_DELAY, {5} },

  {REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {} }
};

static void LCD_panel_init(u32 sel)
{
	__u32 i;
	char model_name[25];
	disp_sys_script_get_item("lcd0", "lcd_model_name", (int *)model_name, 25);
	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SOFT_RESET);
	sunxi_lcd_delay_ms(10);

	for (i = 0; ; i++) {
			if (lcm_initialization_setting[i].count == REGFLAG_END_OF_TABLE)
				break;
			else if (lcm_initialization_setting[i].count == REGFLAG_DELAY)
				sunxi_lcd_delay_ms(lcm_initialization_setting[i].para_list[0]);
#ifdef SUPPORT_DSI
			else
				dsi_dcs_wr(sel, lcm_initialization_setting[i].cmd, lcm_initialization_setting[i].para_list, lcm_initialization_setting[i].count);
#endif
		//break;
	}

	return;
}

static void LCD_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_SET_DISPLAY_OFF);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_dsi_dcs_write_0para(sel, DSI_DCS_ENTER_SLEEP_MODE);
	sunxi_lcd_delay_ms(80);

	return ;
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

//sel: 0:lcd0; 1:lcd1
/*static s32 LCD_set_bright(u32 sel, u32 bright)
{
	sunxi_lcd_dsi_dcs_write_1para(sel,0x51,bright);
	return 0;
}*/

struct __lcd_panel K080_IM2HYL802R_800X1280_mipi_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "K080_IM2HYL802R_800X1280",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
		//.set_bright = LCD_set_bright,
	},
};

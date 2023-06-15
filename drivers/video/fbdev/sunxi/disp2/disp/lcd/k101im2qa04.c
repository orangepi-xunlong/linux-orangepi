/* drivers/video/sunxi/disp2/disp/lcd/k101im2qa04.c
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include "he0801a068.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init1(u32 sel);
static void lcd_panel_init2(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
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
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
				((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1])
				* j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
							(value<<16)
							+ (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) +
					(lcd_gamma_tbl[items-1][1]<<8)
					+ lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 lcd_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, lcd_power_on, 120);
	LCD_OPEN_FUNC(sel, lcd_panel_init1, 120);
	LCD_OPEN_FUNC(sel, lcd_panel_init2, 5);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 5);
	LCD_OPEN_FUNC(sel, lcd_bl_open, 0);

	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, lcd_bl_close, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 1);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 0);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 1);
	panel_reset(sel, 0);
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_power_enable(sel, 1);
	sunxi_lcd_delay_ms(40);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(10);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(10);
	panel_reset(sel, 1);

}

static void lcd_power_off(u32 sel)
{
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(1);
	sunxi_lcd_power_disable(sel, 1);
	sunxi_lcd_delay_ms(1);
	sunxi_lcd_power_disable(sel, 0);
	sunxi_lcd_pin_cfg(sel, 0);
}

static void lcd_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);
}

static void lcd_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
}

static void lcd_panel_init2(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x29); /* DSPON */
}

static void lcd_panel_init1(u32 sel)
{
	sunxi_lcd_dsi_clk_enable(sel);
	/*JD9365 initial code */

	/*Page0 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x00);
	/*--- PASSWORD  ----// */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE1, 0x93);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE2, 0x65);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE3, 0xF8);

	/*Lane select by internal reg  4 lanes */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2D, 0x03); /*defult 0x01 */

	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x80,
				      0x03); /*0X03：4-LANE;0x02:3-LANE */

	/*--- Sequence Ctrl  ----// */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x70, 0x02); /*DC0,DC1 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x71, 0x23); /*DC2,DC3 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x72, 0x06); /*DC7 */

	/*--- Page1  ----// */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x01);
	/*Set VCOM */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x00, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x01, 0x66);
	/*Set VCOM_Reverse */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x03, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x04, 0x6D);
	/*Set Gamma Power, VGMP,VGMN,VGSP,VGSN */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x17, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x18, 0xBF); /*4.5V */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x19, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1A, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1B, 0xBF); /*VGMN=-4.5V */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1C, 0x00);

	/*Set Gate Power */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1F, 0x3E); /*VGH_R  = 15V */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x20, 0x28); /*VGL_R  = -11V */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x21, 0x28); /*VGL_R2 = -11V */
	sunxi_lcd_dsi_dcs_write_1para(
	    sel, 0x22, 0x0E); /*PA[6]=0, PA[5]=0, PA[4]=0, PA[0]=0 */

	/*SETPANEL */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x37, 0x09); /*SS=1,BGR=1 */
	/*SET RGBCYC */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x38,
				      0x04); /*JDT=100 column inversion */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x39,
				      0x08); /*RGB_N_EQ1, modify 20140806 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3A,
				      0x12); /*RGB_N_EQ2, modify 20140806 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3C, 0x78); /*SET EQ3 for TE_H */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3D,
				      0xFF); /*SET CHGEN_ON, modify 20140806  */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3E,
				      0xFF); /*SET CHGEN_OFF, modify 20140806 */
	sunxi_lcd_dsi_dcs_write_1para(
	    sel, 0x3F, 0x7F); /*SET CHGEN_OFF2, modify 20140806 */

	/*Set TCON */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x40, 0x06); /*RSO=800 RGB */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x41, 0xA0); /*LN=640->1280 line */
	/*--- power voltage  ----// */
	sunxi_lcd_dsi_dcs_write_1para(
	    sel, 0x55, 0x0F); /*DCDCM=0001, JD5002/JD5001;Other:0x0F */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x56, 0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x57, 0x69);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x58, 0x0A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x59, 0x0A); /*VCL = -2.5V */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5A, 0x45); /*29 VGH = 15.2V */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5B, 0x15); /*VGL = -11.2V */

	/*--- Gamma  ----// */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5D, 0x7C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5E, 0x65);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5F, 0x55);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x60, 0x49);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x61, 0x44);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x62, 0x35);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x63, 0x3A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x64, 0x23);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x65, 0x3D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x66, 0x3C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x67, 0x3D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x68, 0x5D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x69, 0x4D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6A, 0x56);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6B, 0x48);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6C, 0x45);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6D, 0x38);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6E, 0x25);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6F, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x70, 0x7C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x71, 0x65);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x72, 0x55);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x73, 0x49);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x74, 0x44);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x75, 0x35);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x76, 0x3A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x77, 0x23);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x78, 0x3D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x79, 0x3C);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7A, 0x3D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7B, 0x5D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7C, 0x4D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7D, 0x56);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7E, 0x48);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7F, 0x45);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x80, 0x38);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x81, 0x25);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x82, 0x00);

	/*Page2, for GIP                                       */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x02);
	/*GIP_L Pin mapping                                    */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x00, 0x1E); /*1  VDS */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x01, 0x1E); /*2  VDS */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x02, 0x41); /*3  STV2 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x03, 0x41); /*4  STV2 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x04, 0x43); /*5  STV4 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x05, 0x43); /*6  STV4 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x06, 0x1F); /*7  VSD */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x07, 0x1F); /*8  VSD */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x08, 0x1F); /*9  GCL */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x09, 0x1F); /*10 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0A, 0x1E); /*11 GCH */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0B, 0x1E); /*12 GCH */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0C, 0x1F); /*13 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0D, 0x47); /*14 CLK8 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0E, 0x47); /*15 CLK8 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0F, 0x45); /*16 CLK6 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x10, 0x45); /*17 CLK6 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x11, 0x4B); /*18 CLK4 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x12, 0x4B); /*19 CLK4 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x13, 0x49); /*20 CLK2 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x14, 0x49); /*21 CLK2 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x15, 0x1F); /*22 VGL */

	/*GIP_R Pin mapping                                    */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x16, 0x1E); /*1  VDS */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x17, 0x1E); /*2  VDS */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x18, 0x40); /*3  STV1 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x19, 0x40); /*4  STV1 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1A, 0x42); /*5  STV3 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1B, 0x42); /*6  STV3 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1C, 0x1F); /*7  VSD */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1D, 0x1F); /*8  VSD */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1E, 0x1F); /*9  GCL */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x1F, 0x1f); /*10 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x20, 0x1E); /*11 GCH */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x21, 0x1E); /*12 GCH */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x22, 0x1f); /*13 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x23, 0x46); /*14 CLK7 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x24, 0x46); /*15 CLK7 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x25, 0x44); /*16 CLK5 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x26, 0x44); /*17 CLK5 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x27, 0x4A); /*18 CLK3 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x28, 0x4A); /*19 CLK3 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x29, 0x48); /*20 CLK1 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2A, 0x48); /*21 CLK1 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2B, 0x1f); /*22 VGL */

	/*GIP_L_GS Pin mapping */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2C,
				      0x1F); /*1  VDS 		0x1E */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2D,
				      0x1F); /*2  VDS          0x1E */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2E,
				      0x42); /*3  STV2         0x41 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2F,
				      0x42); /*4  STV2         0x41 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x30,
				      0x40); /*5  STV4         0x43 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x31,
				      0x40); /*6  STV4         0x43 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x32,
				      0x1E); /*7  VSD          0x1F */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x33,
				      0x1E); /*8  VSD          0x1F */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x34,
				      0x1F); /*9  GCL          0x1F */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x35,
				      0x1F); /*10              0x1F */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x36,
				      0x1E); /*11 GCH          0x1E */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x37,
				      0x1E); /*12 GCH          0x1E */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x38,
				      0x1F); /*13              0x1F */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x39,
				      0x48); /*14 CLK8         0x47 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3A,
				      0x48); /*15 CLK8         0x47 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3B,
				      0x4A); /*16 CLK6         0x45 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3C,
				      0x4A); /*17 CLK6         0x45 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3D,
				      0x44); /*18 CLK4         0x4B */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3E,
				      0x44); /*19 CLK4         0x4B */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x3F,
				      0x46); /*20 CLK2         0x49 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x40,
				      0x46); /*21 CLK2         0x49 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x41,
				      0x1F); /*22 VGL          0x1F */

	/*GIP_R_GS Pin mapping */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x42,
				      0x1F); /*1  VDS 		0x1E */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x43,
				      0x1F); /*2  VDS          0x1E */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x44,
				      0x43); /*3  STV1         0x40 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x45,
				      0x43); /*4  STV1         0x40 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x46,
				      0x41); /*5  STV3         0x42 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x47,
				      0x41); /*6  STV3         0x42 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x48,
				      0x1E); /*7  VSD          0x1F */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x49,
				      0x1E); /*8  VSD          0x1F */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4A,
				      0x1E); /*9  GCL          0x1F */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4B,
				      0x1F); /*10              0x1f */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4C,
				      0x1E); /*11 GCH          0x1E */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4D,
				      0x1E); /*12 GCH          0x1E */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4E,
				      0x1F); /*13              0x1f */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x4F,
				      0x49); /*14 CLK7         0x46 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x50,
				      0x49); /*15 CLK7         0x46 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x51,
				      0x4B); /*16 CLK5         0x44 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x52,
				      0x4B); /*17 CLK5         0x44 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x53,
				      0x45); /*18 CLK3         0x4A */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x54,
				      0x45); /*19 CLK3         0x4A */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x55,
				      0x47); /*20 CLK1         0x48 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x56,
				      0x47); /*21 CLK1         0x48 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x57,
				      0x1F); /*22 VGL          0x1f */

	/*GIP Timing   */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x58, 0x10);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x59, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5A, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5B, 0x30); /*STV_S0 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5C, 0x02); /*STV_S0 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5D, 0x40); /*STV_W / S1 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5E, 0x01); /*STV_S2 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x5F, 0x02); /*STV_S3 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x60, 0x30); /*ETV_W / S1 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x61, 0x01); /*ETV_S2 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x62, 0x02); /*ETV_S3 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x63, 0x6A); /*SETV_ON   */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x64, 0x6A); /*SETV_OFF  */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x65, 0x05); /*ETV */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x66, 0x12);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x67, 0x74);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x68, 0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x69, 0x6A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6A, 0x6A);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6B, 0x08);

	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6C, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6D, 0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6E, 0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x6F, 0x88);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x70, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x71, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x72, 0x06);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x73, 0x7B);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x74, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x75, 0x07);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x76, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x77, 0x5D);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x78, 0x17);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x79, 0x1F);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7A, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7B, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7C, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7D, 0x03);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x7E, 0x7B);

	/*Page4 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x04);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2B, 0x2B);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x2E, 0x44);

	/*Page1 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x01);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x0E, 0x01); /*LEDON output VCSW2 */

	/*Page3 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x03);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0x98,
				      0x2F); /*From 2E to 2F, LED_VOL */

	/*Page0 */
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE0, 0x00);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE6, 0x02);
	sunxi_lcd_dsi_dcs_write_1para(sel, 0xE7, 0x02);
	/*SLP OUT */
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x11); /* SLPOUT */

}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x10);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x28);
	sunxi_lcd_delay_ms(5);
}

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel k101im2qa04_panel = {
	/* panel driver name, must mach the name of
	 * lcd_drv_name in sys_config.fex
	 */
	.name = "k101im2qa04",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
			.cfg_open_flow = lcd_open_flow,
			.cfg_close_flow = lcd_close_flow,
			.lcd_user_defined_func = lcd_user_defined_func,
	},
};

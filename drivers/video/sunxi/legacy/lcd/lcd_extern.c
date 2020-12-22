/* linux/drivers/video/sunxi/lcd/video_source_interface.c
 *
 * Copyright (c) 2013 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * Video source interface for LCD driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "dev_lcd.h"

extern struct sunxi_lcd_drv g_lcd_drv;

/**
 * sunxi_lcd_parse_panel_para - parse panel para from sys_config file.
 * @screen_id: The index of screen.
 * @info: The pointer to panel para.
 */
__s32 sunxi_lcd_parse_panel_para(struct sunxi_panel *panel, __panel_para_t * info)
{
  __s32 ret = 0;
  char primary_key[25];
  __s32 value = 0;
  __u32 sel = panel->screen_id;

  sprintf(primary_key, "lcd%d_para", sel);

  memset(info, 0, sizeof(__panel_para_t));

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_x", &value, 1);
  if(ret == 0)
  {
    info->lcd_x = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_y", &value, 1);
  if(ret == 0)
  {
    info->lcd_y = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_width", &value, 1);
  if(ret == 0)
  {
    info->lcd_width = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_height", &value, 1);
  if(ret == 0)
  {
    info->lcd_height = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dclk_freq", &value, 1);
  if(ret == 0)
  {
    info->lcd_dclk_freq = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_freq", &value, 1);
  if(ret == 0)
  {
    info->lcd_pwm_freq = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_pol", &value, 1);
  if(ret == 0)
  {
    info->lcd_pwm_pol = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_if", &value, 1);
  if(ret == 0)
  {
    info->lcd_if = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hbp", &value, 1);
  if(ret == 0)
  {
    info->lcd_hbp = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_ht", &value, 1);
  if(ret == 0)
  {
    info->lcd_ht = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_vbp", &value, 1);
  if(ret == 0)
  {
    info->lcd_vbp = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_vt", &value, 1);
  if(ret == 0)
  {
    info->lcd_vt = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_if", &value, 1);
  if(ret == 0)
  {
    info->lcd_hv_if = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_vspw", &value, 1);
  if(ret == 0)
  {
    info->lcd_vspw = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hspw", &value, 1);
  if(ret == 0)
  {
    info->lcd_hspw = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_if", &value, 1);
  if(ret == 0)
  {
    info->lcd_lvds_if = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_mode", &value, 1);
  if(ret == 0)
  {
    info->lcd_lvds_mode = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_colordepth", &value, 1);
  if(ret == 0)
  {
    info->lcd_lvds_colordepth= value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_lvds_io_polarity", &value, 1);
  if(ret == 0)
  {
    info->lcd_lvds_io_polarity = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_cpu_if", &value, 1);
  if(ret == 0)
  {
    info->lcd_cpu_if = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_cpu_te", &value, 1);
  if(ret == 0)
  {
    info->lcd_cpu_te = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_frm", &value, 1);
  if(ret == 0)
  {
    info->lcd_frm = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_if", &value, 1);
  if(ret == 0)
  {
    info->lcd_dsi_if = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_lane", &value, 1);
  if(ret == 0)
  {
    info->lcd_dsi_lane = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_format", &value, 1);
  if(ret == 0)
  {
    info->lcd_dsi_format = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_eotp", &value, 1);
  if(ret == 0)
  {
    info->lcd_dsi_eotp = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_dsi_te", &value, 1);
  if(ret == 0)
  {
    info->lcd_dsi_te = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_tx_ic", &value, 1);
  if(ret == 0)
  {
    info->lcd_edp_tx_ic= value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_tx_rate", &value, 1);
  if(ret == 0)
  {
    info->lcd_edp_tx_rate = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_tx_lane", &value, 1);
  if(ret == 0)
  {
    info->lcd_edp_tx_lane= value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_colordepth", &value, 1);
  if(ret == 0)
  {
    info->lcd_edp_colordepth = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_clk_phase", &value, 1);
  if(ret == 0)
  {
    info->lcd_hv_clk_phase = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_sync_polarity", &value, 1);
  if(ret == 0)
  {
    info->lcd_hv_sync_polarity = value;
  }
  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_gamma_en", &value, 1);
  if(ret == 0)
  {
    info->lcd_gamma_en = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_cmap_en", &value, 1);
  if(ret == 0)
  {
    info->lcd_cmap_en = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_bright_curve_en", &value, 1);
  if(ret == 0)
  {
    info->lcd_bright_curve_en = value;
  }

  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_size", (int*)info->lcd_size, 2);
  ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_model_name", (int*)info->lcd_model_name, 2);

  return 0;
}

void sunxi_lcd_parse_sys_config(struct sunxi_panel *panel, __disp_lcd_cfg_t *lcd_cfg)
{
    static char io_name[28][20] = {"lcdd0", "lcdd1", "lcdd2", "lcdd3", "lcdd4", "lcdd5", "lcdd6", "lcdd7", "lcdd8", "lcdd9", "lcdd10", "lcdd11", 
                         "lcdd12", "lcdd13", "lcdd14", "lcdd15", "lcdd16", "lcdd17", "lcdd18", "lcdd19", "lcdd20", "lcdd21", "lcdd22",
                         "lcdd23", "lcdclk", "lcdde", "lcdhsync", "lcdvsync"};
    disp_gpio_set_t  *gpio_info;
    int  value = 1;
    char primary_key[20], sub_name[25];
    int i = 0;
    int  ret;
    __u32 sel = panel->screen_id;

    sprintf(primary_key, "lcd%d_para", sel);

//lcd_used
    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_used", &value, 1);
    if(ret == 0)
    {
        lcd_cfg->lcd_used = value;
    }

    if(lcd_cfg->lcd_used == 0) //no need to get lcd config if lcd_used eq 0
        return ;

//lcd_bl_en
    lcd_cfg->lcd_bl_en_used = 0;
    gpio_info = &(lcd_cfg->lcd_bl_en);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_bl_en", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
        lcd_cfg->lcd_bl_en_used = 1;
    }

//lcd_power
    lcd_cfg->lcd_power_used= 0;
    gpio_info = &(lcd_cfg->lcd_power);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_power", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
        lcd_cfg->lcd_power_used= 1;
    }

//lcd_pwm
    lcd_cfg->lcd_pwm_used= 0;
    gpio_info = &(lcd_cfg->lcd_pwm);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_pwm", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
#if 0
#ifdef __LINUX_OSAL__
        lcd_cfg->lcd_pwm_used= 1;
        if(gpio_info->gpio == GPIOH(13))//ph13
        {
            lcd_cfg->lcd_pwm_ch = 0;
        }
        else if((gpio_info->gpio == GPIOH(9)) && (gpio_info->gpio == GPIOH(10)))//ph9,ph10
        {
            lcd_cfg->lcd_pwm_ch = 1;
        }
        else if((gpio_info->gpio == GPIOH(11)) && (gpio_info->gpio ==  GPIOH(12)))//ph11,ph12
        {
            lcd_cfg->lcd_pwm_ch = 2;
        }
        else if((gpio_info->gpio == GPIOA(19)) && (gpio_info->gpio == GPIOA(20)))//ph19 pa20
        {
            lcd_cfg->lcd_pwm_ch = 3;
        }
        else
        {
            lcd_cfg->lcd_pwm_used = 0;
        }

        __inf("lcd_pwm_used=%d,lcd_pwm_ch=%d\n", lcd_cfg->lcd_pwm_used, lcd_cfg->lcd_pwm_ch);
#endif
#endif
    }

//lcd_gpio
    for(i=0; i<4; i++)
    {
        sprintf(sub_name, "lcd_gpio_%d", i);
        
        gpio_info = &(lcd_cfg->lcd_gpio[i]);
        ret = OSAL_Script_FetchParser_Data(primary_key,sub_name, (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
        if(ret == 0)
        {
            lcd_cfg->lcd_gpio_used[i]= 1;
        }
    }
    
//lcd_gpio_scl,lcd_gpio_sda
    gpio_info = &(lcd_cfg->lcd_gpio[4]);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_gpio_scl", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
        lcd_cfg->lcd_gpio_used[4]= 1;
    }
    gpio_info = &(lcd_cfg->lcd_gpio[5]);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_gpio_sda", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
        lcd_cfg->lcd_gpio_used[5]= 1;
    }
    
//lcd io
    for(i=0; i<28; i++)
    {
        gpio_info = &(lcd_cfg->lcd_io[i]);
        ret = OSAL_Script_FetchParser_Data(primary_key,io_name[i], (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
        if(ret == 0)
        {
            lcd_cfg->lcd_io_used[i]= 1;
        }
    }

    lcd_cfg->backlight_max_limit = 150;
    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_max_limit", &value, 1);
    if(ret == 0)
    {
        lcd_cfg->backlight_max_limit = (value > 255)? 255:value;
    }

//init_bright
    sprintf(primary_key, "disp_init");
    sprintf(sub_name, "lcd%d_backlight", sel);
    
    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->backlight_bright = 197;
    }
    else
    {
        if(value > 256)
        {
            value = 256;
        }
        lcd_cfg->backlight_bright = value;
    }

//bright,constraction,saturation,hue
    sprintf(primary_key, "disp_init");
    sprintf(sub_name, "lcd%d_bright", sel);
    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->lcd_bright = 50;
    }
    else
    {
        if(value > 100)
        {
            value = 100;
        }
        lcd_cfg->lcd_bright = value;
    }
    
    sprintf(sub_name, "lcd%d_contrast", sel);
    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->lcd_contrast = 50;
    }
    else
    {
        if(value > 100)
        {
            value = 100;
        }
        lcd_cfg->lcd_contrast = value;
    }

    sprintf(sub_name, "lcd%d_saturation", sel);
    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->lcd_saturation = 50;
    }
    else
    {
        if(value > 100)
        {
            value = 100;
        }
        lcd_cfg->lcd_saturation = value;
    }
    
    sprintf(sub_name, "lcd%d_hue", sel);
    ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
    if(ret < 0)
    {
        lcd_cfg->lcd_hue = 50;
    }
    else
    {
        if(value > 100)
        {
            value = 100;
        }
        lcd_cfg->lcd_hue = value;
    }
}

//voltage(7~33): 0.1v,   e.g. 1 voltage = 0.1v
__s32 LCD_POWER_ELDO3_EN(__u32 sel, __bool b_en, __u32 voltage)
{
#if 0
    __u8 data;
    __u32 ret;
    __u8 addr;

    voltage = (voltage < 7)? 7 :voltage;
    voltage = (voltage > 33)?33:voltage;
    data = voltage - 7;
    addr = 0x1b;
    ret = ar100_axp_write_reg(&addr, &data, 1);
    if(ret != 0)
    {
        DE_WRN("set eldo3 to %d.%dv fail\n", voltage/10, voltage%10);
    }
    addr = 0x12;
    ret = ar100_axp_read_reg(&addr, &data, 1);
    if(ret != 0)
    {
        DE_WRN("axp read reg fail\n");
    }
    addr = 0x12;
    data = (b_en)? (data | 0x04):(data & 0xfb);
    ar100_axp_write_reg(&addr, &data, 1);
    if(ret != 0)
    {
        DE_WRN("%s eldo3 fail\n", (b_en)? "enable":"disable");
    }
#endif
    return 0;
}

//voltage(7~33): 0.1v,   e.g. 1 voltage = 0.1v
__s32 LCD_POWER_DLDO1_EN(__u32 sel, __bool b_en, __u32 voltage)
{
#if 0
    __u8 data;
    __u32 ret;
    __u8 addr;

    voltage = (voltage < 7)? 7 :voltage;
    voltage = (voltage > 33)?33:voltage;
    data = voltage - 7;
    addr = 0x15;
    ret = ar100_axp_write_reg(&addr, &data, 1);
    if(ret != 0)
    {
        DE_WRN("set dldo1 to %d.%dv fail\n", voltage/10, voltage%10);
    }
    addr = 0x12;
    ret = ar100_axp_read_reg(&addr, &data, 1);
    if(ret != 0)
    {
        DE_WRN("axp read reg fail\n");
    }
    addr = 0x12;
    data = (b_en)? (data | 0x08):(data & 0xf7);
    ar100_axp_write_reg(&addr, &data, 1);
    if(ret != 0)
    {
        DE_WRN("%s dldo1 fail\n", (b_en)? "enable":"disable");
    }
#endif
    return 0;
}

__s32 LCD_POWER_EN(__u32 sel, __bool b_en)
{
#if 0
    disp_gpio_set_t  gpio_info[1];
    __hdle hdl;

	if(b_en)
	{
		if(gdisp.screen[sel].lcd_cfg.lcd_power_used)
        {
            if(gpanel_info[sel].lcd_if == LCD_IF_EXT_DSI)
	        {
	            LCD_POWER_ELDO3_EN(sel, 1, 12);
	            msleep(10);
	        }

            memcpy(gpio_info, &(gdisp.screen[sel].lcd_cfg.lcd_power), sizeof(disp_gpio_set_t));
		        
	        if(!b_en)
	        {
	            gpio_info->data = (gpio_info->data==0)?1:0;
	        }

	        hdl = OSAL_GPIO_Request(gpio_info, 1);
	        OSAL_GPIO_Release(hdl, 2);
	        udelay(200);
	        
	        if((gpanel_info[sel].lcd_if == LCD_IF_EDP) && (gpanel_info[sel].lcd_edp_tx_ic == 0))
	        {
				__u8 data;
				__u32 ret;
				__u8 addr;
				
				addr = 0x1b;
				data = 0x0b;
				ret = ar100_axp_write_reg(&addr, &data, 1); //set eldo3 to 1.8v
				if(ret != 0)
				{
					DE_WRN("set eldo3 to 1.8v fail\n");	
				}
				addr = 0x12;
				ret = ar100_axp_read_reg(&addr, &data, 1);
				if(ret != 0)
				{
					DE_WRN("axp read reg fail\n");	
				}
				addr = 0x12;
				data |= 0x04;
				ar100_axp_write_reg(&addr, &data, 1); //enable eldo3
				if(ret != 0)
				{
					DE_WRN("enable eldo3 fail\n");	
				}
	        }
            else if((gpanel_info[sel].lcd_if == LCD_IF_EDP) && (gpanel_info[sel].lcd_edp_tx_ic == 1))
	        {
				__u8 data;
				__u32 ret;
				__u8 addr;

				addr = 0x15;
				data = 0x12;
				ret = ar100_axp_write_reg(&addr, &data, 1); //set dldo1 to 2.5v
				if(ret != 0)
				{
					DE_WRN("set dldo1 to 2.5v fail\n");
				}
				addr = 0x12;
				ret = ar100_axp_read_reg(&addr, &data, 1);
				if(ret != 0)
				{
					DE_WRN("axp read reg fail\n");
				}
				addr = 0x12;
				data |= 0x08;
				ar100_axp_write_reg(&addr, &data, 1); //enable dldo1
				if(ret != 0)
				{
					DE_WRN("enable dldo1 fail\n");
				}

                addr = 0x1b;
				data = 0x05;
				ret = ar100_axp_write_reg(&addr, &data, 1); //set eldo3 to 1.2v
				if(ret != 0)
				{
					DE_WRN("set eldo3 to 1.2v fail\n");
				}
				addr = 0x12;
				ret = ar100_axp_read_reg(&addr, &data, 1);
				if(ret != 0)
				{
					DE_WRN("axp read reg fail\n");
				}
				addr = 0x12;
				data |= 0x04;
				ar100_axp_write_reg(&addr, &data, 1); //enable eldo3
				if(ret != 0)
				{
					DE_WRN("enable eldo3 fail\n");
				}
	        }
            msleep(50);
        }
        Disp_lcdc_pin_cfg(sel, DISP_OUTPUT_TYPE_LCD, 1);
        msleep(2);
	}
	else
	{
		Disp_lcdc_pin_cfg(sel, DISP_OUTPUT_TYPE_LCD, 0);
        msleep(2);
        if(gdisp.screen[sel].lcd_cfg.lcd_power_used)
        {
            if((gpanel_info[sel].lcd_if == LCD_IF_EDP) && (gpanel_info[sel].lcd_edp_tx_ic == 0))
            {
    			__u8 data;
    			__u32 ret;
    			__u8 addr;
    		
    			addr = 0x12;
    			ret = ar100_axp_read_reg(&addr, &data, 1);
    			if(ret != 0)
    			{
                    DE_WRN("axp read reg fail\n");
    			}
    			data &= 0xfb;
    			ar100_axp_write_reg(&addr, &data, 1); //enable eldo3
    			if(ret != 0)
    			{
                    DE_WRN("disable eldo3 fail\n");
    			}
           }
           else if((gpanel_info[sel].lcd_if == LCD_IF_EDP) && (gpanel_info[sel].lcd_edp_tx_ic == 1))
           {
                __u8 data;
                __u32 ret;
                __u8 addr;

                addr = 0x12;
                ret = ar100_axp_read_reg(&addr, &data, 1);
                if(ret != 0)
                {
                    DE_WRN("axp read reg fail\n");
                }
                data &= 0xfb;
                ar100_axp_write_reg(&addr, &data, 1); //disable eldo3
                if(ret != 0)
                {
                    DE_WRN("disable eldo3 fail\n");
                }

                addr = 0x12;
                ret = ar100_axp_read_reg(&addr, &data, 1);
                if(ret != 0)
                {
                    DE_WRN("axp read reg fail\n");
                }
                data &= 0xf7;
                ar100_axp_write_reg(&addr, &data, 1); //disable dldo1
                if(ret != 0)
                {
                    DE_WRN("disable dldo1 fail\n");
                }
           }

    		udelay(200);
    		memcpy(gpio_info, &(gdisp.screen[sel].lcd_cfg.lcd_power), sizeof(disp_gpio_set_t));
            
            if(!b_en)
            {
                gpio_info->data = (gpio_info->data==0)?1:0;
            }
            hdl = OSAL_GPIO_Request(gpio_info, 1);
            OSAL_GPIO_Release(hdl, 2);

            if(gpanel_info[sel].lcd_if == LCD_IF_EXT_DSI)
	        {
	            LCD_POWER_ELDO3_EN(sel, 0, 7);
	        }
        }
	}
#endif
    return 0;
}

__s32 LCD_GPIO_set_attr(disp_gpio_set_t  gpio_info[1], __bool b_output)
{
    return  OSAL_GPIO_DevSetONEPIN_IO_STATUS(gpio_info->hdl, b_output, gpio_info->gpio_name);
}

__s32 LCD_GPIO_read(disp_gpio_set_t  gpio_info[1])
{
    return OSAL_GPIO_DevREAD_ONEPIN_DATA(gpio_info->hdl, gpio_info->gpio_name);
}

__s32 LCD_GPIO_write(disp_gpio_set_t  gpio_info[1], __u32 data)
{
    return OSAL_GPIO_DevWRITE_ONEPIN_DATA(gpio_info->hdl, data, gpio_info->gpio_name);
}

__s32 LCD_GPIO_init(disp_gpio_set_t  gpio_info[6], __bool lcd_gpio_used[6])
{
    __u32 i = 0;

    for(i=0; i<6; i++) {
        gpio_info[i].hdl = 0;

        if(lcd_gpio_used[i]) {
            gpio_info[i].hdl = OSAL_GPIO_Request(&gpio_info[i], 1);
        }
    }
    
    return 0;
}

__s32 LCD_GPIO_exit(disp_gpio_set_t  gpio_info[6])
{
  __u32 i = 0;
  
  for(i=0; i<6; i++) {
    if(gpio_info[i].hdl) {
      OSAL_GPIO_Release(gpio_info[i].hdl, 2);
          
      gpio_info[i].mul_sel = 7;
      gpio_info[i].hdl = OSAL_GPIO_Request(&gpio_info[i], 1);
      OSAL_GPIO_Release(gpio_info[i].hdl, 2);
      gpio_info[i].hdl = 0;
    }
  }
  
  return 0;
}

__s32 LCD_GPIO_cfg(disp_gpio_set_t gpio_info[], __bool used[], __u32 num, __u32 bon)
{
  __hdle hdl;
  int i;

  for(i=0; i<num; i++) {
    if(used[i]) {
      if(!bon) {
        gpio_info[i].mul_sel = 7;
        gpio_info[i].data = 0;
      }

      hdl = OSAL_GPIO_Request(&gpio_info[i], 1);
      OSAL_GPIO_Release(hdl, 2);
    }
  }

  return 0;
}

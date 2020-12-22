#include "disp_lcd.h"

struct disp_lcd_private_data
{
	disp_lcd_flow             open_flow;
	disp_lcd_flow             close_flow;
	disp_panel_para           panel_info;
	__disp_lcd_cfg_t          lcd_cfg;
	disp_lcd_panel_fun        lcd_panel_fun;
	bool                      enabling;
	bool                      disabling;
	u32                       irq_no;
	u32                       reg_base;
	u32                       irq_no_dsi;
	u32                       reg_base_dsi;
	u32                       irq_no_edp;
	u32                       enabled;
	struct {
		__hdle                  dev;
		u32                     channel;
		u32                     polarity;
		u32                     period_ns;
		u32                     duty_ns;
		u32                     enabled;
	}pwm_info;
	disp_clk_info_t           lcd_clk;
	disp_clk_info_t           dsi_clk;
	disp_clk_info_t           lvds_clk;
	disp_clk_info_t           edp_clk;
	disp_clk_info_t           extra_clk;
	disp_clk_info_t           merge_clk;
	disp_clk_info_t           sat_clk;
};
#if defined(__LINUX_PLAT__)
static spinlock_t lcd_data_lock;
#endif

static struct disp_lcd *lcds = NULL;
static struct disp_lcd_private_data *lcd_private;

static s32 disp_lcd_set_bright(struct disp_lcd *lcd, u32 bright);
static s32 disp_lcd_get_bright(struct disp_lcd *lcd, u32 *bright);

struct disp_lcd* disp_get_lcd(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens) {
		DE_WRN("screen_id %d out of range\n", screen_id);
		return NULL;
	}

	return &lcds[screen_id];
}
static struct disp_lcd_private_data *disp_lcd_get_priv(struct disp_lcd *lcd)
{
	if(NULL == lcd) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return &lcd_private[lcd->channel_id];
}

static s32 disp_lcd_is_used(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	} else {
		return lcdp->lcd_cfg.lcd_used;
	}
}

static s32 lcd_parse_panel_para(u32 screen_id, disp_panel_para * info)
{
    s32 ret = 0;
    char primary_key[25];
    s32 value = 0;

    sprintf(primary_key, "lcd%d_para", screen_id);

//lcd_used
    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_used", &value, 1);

    if(value == 0) //no need to get panel para if !lcd_used
        return -1;

    sprintf(primary_key, "lcd%d_para", screen_id);

    memset(info, 0, sizeof(disp_panel_para));

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

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_used", &value, 1);
    if(ret == 0)
    {
        info->lcd_pwm_used = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_pwm_ch", &value, 1);
    if(ret == 0)
    {
        info->lcd_pwm_ch = value;
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

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_rate", &value, 1);
    if(ret == 0)
    {
        info->lcd_edp_rate = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_lane", &value, 1);
    if(ret == 0)
    {
        info->lcd_edp_lane= value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_colordepth", &value, 1);
    if(ret == 0)
    {
        info->lcd_edp_colordepth = value;
    }

	ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_fps", &value, 1);
    if(ret == 0)
    {
        info->lcd_edp_fps = value;
    }

	ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_edp_swing_level", &value, 1);
    if(ret == 0)
    {
        info->lcd_edp_swing_level = value;
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

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_syuv_seq", &value, 1);
    if(ret == 0)
    {
        info->lcd_hv_syuv_seq = value;
    }

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_hv_syuv_fdly", &value, 1);
    if(ret == 0)
    {
        info->lcd_hv_syuv_fdly = value;
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

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_xtal_freq", &value, 1);
    if(ret == 0)
    {
        info->lcd_xtal_freq = value;
    }
	//for edp
	if((info->lcd_dclk_freq == 0) && (info->lcd_if == 5))
	{
		info->lcd_dclk_freq = (info->lcd_ht * info->lcd_vt * info->lcd_edp_fps + 500000) / 1000000;
	}

    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_size", (int*)info->lcd_size, 2);
    ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_model_name", (int*)info->lcd_model_name, 2);

    return 0;
}

static void lcd_panel_parameter_check(u32 screen_id, struct disp_lcd* lcd)
{
	disp_panel_para* info;
	u32 cycle_num = 1;
	u32 Lcd_Panel_Err_Flag = 0;
	u32 Lcd_Panel_Wrn_Flag = 0;
	u32 Disp_Driver_Bug_Flag = 0;

	u32 lcd_fclk_frq;
	u32 lcd_clk_div;
	s32 ret = 0;

	char primary_key[20];
	s32 value = 0;

	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return ;
	}

	if(!disp_al_query_lcd_mod(lcd->channel_id))
		return;

	sprintf(primary_key, "lcd%d_para", lcd->channel_id);
	ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_used", &value, 1);

	if(ret != 0 ) {
		DE_INF("get lcd%dpara lcd_used fail\n", lcd->channel_id);
		return;
	} else {
		if(value != 1) {
			DE_INF("lcd%dpara is not used\n", lcd->channel_id);
			return;
		}
	}

	info = &(lcdp->panel_info);
	if(NULL == info) {
		DE_WRN("NULL hdl!\n");
		return;
	}

	if(info->lcd_if == 0 && info->lcd_hv_if == 8)
		cycle_num = 3;
	else if(info->lcd_if == 0 && info->lcd_hv_if == 10)
		cycle_num = 3;
	else if(info->lcd_if == 0 && info->lcd_hv_if == 11)
		cycle_num = 4;
	else if(info->lcd_if == 0 && info->lcd_hv_if == 12)
		cycle_num = 2;
	else if(info->lcd_if == 1 && info->lcd_cpu_if == 2)
		cycle_num = 3;
	else if(info->lcd_if == 1 && info->lcd_cpu_if == 4)
		cycle_num = 2;
	else if(info->lcd_if == 1 && info->lcd_cpu_if == 6)
		cycle_num = 2;
	else if(info->lcd_if == 1 && info->lcd_cpu_if == 10)
		cycle_num = 2;
	else if(info->lcd_if == 1 && info->lcd_cpu_if == 12)
		cycle_num = 3;
	else if(info->lcd_if == 1 && info->lcd_cpu_if == 14)
		cycle_num = 2;
	else
		cycle_num = 1;

	if(info->lcd_hbp > info->lcd_hspw)
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT0;
	}

	if(info->lcd_vbp > info->lcd_vspw)
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT1;
	}

	if(info->lcd_ht >= (info->lcd_hbp+info->lcd_x*cycle_num+4))
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT2;
	}

	if((info->lcd_vt) >= (info->lcd_vbp+info->lcd_y + 2))
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT3;
	}

	lcd_clk_div = disp_al_lcd_get_clk_div(screen_id);

	if(lcd_clk_div >= 6)
	{
		;
	}
	else if(lcd_clk_div >=2)
	{
		if((info->lcd_hv_clk_phase == 1) || (info->lcd_hv_clk_phase == 3))
		{
			Lcd_Panel_Err_Flag |= BIT10;
		}
	}
	else
	{
		Disp_Driver_Bug_Flag |= 1;
	}

	if((info->lcd_if == 1 && info->lcd_cpu_if == 0) ||	(info->lcd_if == 1 && info->lcd_cpu_if == 10)
		|| (info->lcd_if == 1 && info->lcd_cpu_if == 12) ||(info->lcd_if == 3 && info->lcd_lvds_colordepth == 1))
	{
		if(info->lcd_frm != 1)
			Lcd_Panel_Wrn_Flag |= BIT0;
	}
	else if(info->lcd_if == 1 && ((info->lcd_cpu_if == 2) || (info->lcd_cpu_if == 4) || (info->lcd_cpu_if == 6)
		|| (info->lcd_cpu_if == 8) || (info->lcd_cpu_if == 14)))
	{
		if(info->lcd_frm != 2)
			Lcd_Panel_Wrn_Flag |= BIT1;
	}

	lcd_fclk_frq = (info->lcd_dclk_freq * 1000 * 1000) / ((info->lcd_vt) * info->lcd_ht);
	if(lcd_fclk_frq < 50 || lcd_fclk_frq > 70)
	{
		Lcd_Panel_Wrn_Flag |= BIT2;
	}

	if((info->lcd_vt - info->lcd_y) < 30)
	{
		Lcd_Panel_Wrn_Flag |= BIT3;
	}

	if(Lcd_Panel_Err_Flag != 0 || Lcd_Panel_Wrn_Flag != 0)
	{
		if(Lcd_Panel_Err_Flag != 0)
		{
			__u32 i;
			for(i = 0; i < 200; i++)
			{
				//OSAL_PRINTF("*** Lcd in danger...\n");
			}
		}

		OSAL_PRINTF("*****************************************************************\n");
		OSAL_PRINTF("***\n");
		OSAL_PRINTF("*** LCD Panel Parameter Check\n");
		OSAL_PRINTF("***\n");
		OSAL_PRINTF("***             by guozhenjie\n");
		OSAL_PRINTF("***\n");
		OSAL_PRINTF("*****************************************************************\n");

		OSAL_PRINTF("*** \n");
		OSAL_PRINTF("*** Interface:");
		if(info->lcd_if == 0 && info->lcd_hv_if == 0)
			{OSAL_PRINTF("*** Parallel HV Panel\n");}
		else if(info->lcd_if == 0 && info->lcd_hv_if == 8)
			{OSAL_PRINTF("*** Serial HV Panel\n");}
		else if(info->lcd_if == 0 && info->lcd_hv_if == 10)
			{OSAL_PRINTF("*** Dummy RGB HV Panel\n");}
		else if(info->lcd_if == 0 && info->lcd_hv_if == 11)
			{OSAL_PRINTF("*** RGB Dummy HV Panel\n");}
		else if(info->lcd_if == 0 && info->lcd_hv_if == 12)
			{OSAL_PRINTF("*** Serial YUV Panel\n");}
		else if(info->lcd_if == 3 && info->lcd_lvds_colordepth== 0)
			{OSAL_PRINTF("*** 24Bit LVDS Panel\n");}
		else if(info->lcd_if == 3 && info->lcd_lvds_colordepth== 1)
			{OSAL_PRINTF("*** 18Bit LVDS Panel\n");}
		else if((info->lcd_if == 1) && (info->lcd_cpu_if == 0 || info->lcd_cpu_if == 10 || info->lcd_cpu_if == 12))
			{OSAL_PRINTF("*** 18Bit CPU Panel\n");}
		else if((info->lcd_if == 1) && (info->lcd_cpu_if == 2 || info->lcd_cpu_if == 4 ||
				info->lcd_cpu_if == 6 || info->lcd_cpu_if == 8 || info->lcd_cpu_if == 14))
			{OSAL_PRINTF("*** 16Bit CPU Panel\n");}
		else
		{
			OSAL_PRINTF("\n");
			OSAL_PRINTF("*** lcd_if:     %d\n",info->lcd_if);
			OSAL_PRINTF("*** lcd_hv_if:  %d\n",info->lcd_hv_if);
			OSAL_PRINTF("*** lcd_cpu_if: %d\n",info->lcd_cpu_if);
		}
		if(info->lcd_frm == 0)
			{OSAL_PRINTF("*** Lcd Frm Disable\n");}
		else if(info->lcd_frm == 1)
			{OSAL_PRINTF("*** Lcd Frm to RGB666\n");}
		else if(info->lcd_frm == 2)
			{OSAL_PRINTF("*** Lcd Frm to RGB565\n");}

		OSAL_PRINTF("*** \n");
		OSAL_PRINTF("*** Timing:\n");
		OSAL_PRINTF("*** lcd_x:      %d\n", info->lcd_x);
		OSAL_PRINTF("*** lcd_y:      %d\n", info->lcd_y);
		OSAL_PRINTF("*** lcd_ht:     %d\n", info->lcd_ht);
		OSAL_PRINTF("*** lcd_hbp:    %d\n", info->lcd_hbp);
		OSAL_PRINTF("*** lcd_vt:     %d\n", info->lcd_vt);
		OSAL_PRINTF("*** lcd_vbp:    %d\n", info->lcd_vbp);
		OSAL_PRINTF("*** lcd_hspw:   %d\n", info->lcd_hspw);
		OSAL_PRINTF("*** lcd_vspw:   %d\n", info->lcd_vspw);
		OSAL_PRINTF("*** lcd_frame_frq:  %dHz\n", lcd_fclk_frq);

		//�䨰��?�䨪?��������?
		OSAL_PRINTF("*** \n");
		if(Lcd_Panel_Err_Flag & BIT0)
			{OSAL_PRINTF("*** Err01: Violate \"lcd_hbp > lcd_hspw\"\n");}
		if(Lcd_Panel_Err_Flag & BIT1)
			{OSAL_PRINTF("*** Err02: Violate \"lcd_vbp > lcd_vspw\"\n");}
		if(Lcd_Panel_Err_Flag & BIT2)
			{OSAL_PRINTF("*** Err03: Violate \"lcd_ht >= (lcd_hbp+lcd_x*%d+4)\"\n", cycle_num);}
		if(Lcd_Panel_Err_Flag & BIT3)
			{OSAL_PRINTF("*** Err04: Violate \"(lcd_vt) >= (lcd_vbp+lcd_y+2)\"\n");}
		if(Lcd_Panel_Err_Flag & BIT10)
			{OSAL_PRINTF("*** Err10: Violate \"lcd_hv_clk_phase\",use \"0\" or \"2\"");}
		if(Lcd_Panel_Wrn_Flag & BIT0)
			{OSAL_PRINTF("*** WRN01: Recommend \"lcd_frm = 1\"\n");}
		if(Lcd_Panel_Wrn_Flag & BIT1)
			{OSAL_PRINTF("*** WRN02: Recommend \"lcd_frm = 2\"\n");}
		if(Lcd_Panel_Wrn_Flag & BIT2)
			{OSAL_PRINTF("*** WRN03: Recommend \"lcd_dclk_frq = %d\"\n",
				((info->lcd_vt) * info->lcd_ht) * 60 / (1000 * 1000));}
		if(Lcd_Panel_Wrn_Flag & BIT3)
			{OSAL_PRINTF("*** WRN04: Recommend \"lcd_vt - lcd_y >= 30\"\n");}
		OSAL_PRINTF("*** \n");

		if(Lcd_Panel_Err_Flag != 0)
		{
			u32 image_base_addr;
			u32 reg_value = 0;

			image_base_addr = DE_Get_Reg_Base(screen_id);

			sys_put_wvalue(image_base_addr+0x804, 0xffff00ff);//set background color

			reg_value = sys_get_wvalue(image_base_addr + 0x800);
			sys_put_wvalue(image_base_addr+0x800, reg_value & 0xfffff0ff);//close all layer

			mdelay(2000);
			sys_put_wvalue(image_base_addr + 0x804, 0x00000000);//set background color
			sys_put_wvalue(image_base_addr + 0x800, reg_value);//open layer

			OSAL_PRINTF("*** Try new parameters,you can make it pass!\n");
		}
		OSAL_PRINTF("*** LCD Panel Parameter Check End\n");
		OSAL_PRINTF("*****************************************************************\n");
	}
}

static void lcd_get_sys_config(u32 screen_id, __disp_lcd_cfg_t *lcd_cfg)
{
    static char io_name[28][20] = {"lcdd0", "lcdd1", "lcdd2", "lcdd3", "lcdd4", "lcdd5", "lcdd6", "lcdd7", "lcdd8", "lcdd9", "lcdd10", "lcdd11",
                         "lcdd12", "lcdd13", "lcdd14", "lcdd15", "lcdd16", "lcdd17", "lcdd18", "lcdd19", "lcdd20", "lcdd21", "lcdd22",
                         "lcdd23", "lcdclk", "lcdde", "lcdhsync", "lcdvsync"};
    disp_gpio_set_t  *gpio_info;
    int  value = 1;
    char primary_key[20], sub_name[25];
    int i = 0;
    int  ret;

    sprintf(primary_key, "lcd%d_para", screen_id);

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

	sprintf(sub_name, "lcd_bl_regulator");
	ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, (int *)lcd_cfg->lcd_bl_regulator, 2);

//lcd_power0
	for(i=0; i<LCD_POWER_NUM; i++)
	{
		if(i==0)
			sprintf(sub_name, "lcd_power");
		else
			sprintf(sub_name, "lcd_power%d", i);
		lcd_cfg->lcd_power_type[i] = 0; /* invalid */
		ret = OSAL_Script_FetchParser_Data_Ex(primary_key,sub_name, (int *)(lcd_cfg->lcd_regu[i]), 25);
		if(ret == 3) {
			/* gpio */
		  lcd_cfg->lcd_power_type[i] = 1; /* gpio */
		  memcpy(&(lcd_cfg->lcd_power[i]), lcd_cfg->lcd_regu[i], sizeof(disp_gpio_set_t));
		} else if(ret == 2) {
			/* str */
			lcd_cfg->lcd_power_type[i] = 2; /* regulator */
		}
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
    gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SCL]);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_gpio_scl", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
        lcd_cfg->lcd_gpio_used[LCD_GPIO_SCL]= 1;
    }
    gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SDA]);
    ret = OSAL_Script_FetchParser_Data(primary_key,"lcd_gpio_sda", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
    if(ret == 0)
    {
        lcd_cfg->lcd_gpio_used[LCD_GPIO_SDA]= 1;
    }

	for(i = 0; i < LCD_GPIO_REGU_NUM; i++)
	{
		sprintf(sub_name, "lcd_gpio_regulator%d", i);

		ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, (int *)lcd_cfg->lcd_gpio_regulator[i], 2);
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

	sprintf(sub_name, "lcd_io_regulator");
	ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, (int *)lcd_cfg->lcd_io_regulator, 2);

//backlight adjust
	for(i = 0; i < 101; i++) {
		sprintf(sub_name, "lcd_bl_%d_percent", i);
		lcd_cfg->backlight_curve_adjust[i] = 0;

		if(i == 100)
		    lcd_cfg->backlight_curve_adjust[i] = 255;

		ret = OSAL_Script_FetchParser_Data(primary_key, sub_name, &value, 1);
		if(ret == 0) {
			value = (value > 100)? 100:value;
			value = value * 255 / 100;
			lcd_cfg->backlight_curve_adjust[i] = value;
		}
	}


//init_bright
    sprintf(primary_key, "disp_init");
    sprintf(sub_name, "lcd%d_backlight", screen_id);

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
    sprintf(sub_name, "lcd%d_bright", screen_id);
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

    sprintf(sub_name, "lcd%d_contrast", screen_id);
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

    sprintf(sub_name, "lcd%d_saturation", screen_id);
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

    sprintf(sub_name, "lcd%d_hue", screen_id);
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

static s32 disp_lcd_pin_cfg(struct disp_lcd *lcd, u32 bon)
{
	__hdle lcd_pin_hdl;
	int  i;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("lcd %d pin config, state %s, %d\n", lcd->channel_id, (bon)? "on":"off", bon);

	//io-pad
	if(bon == 1) {
		if(!((!strcmp(lcdp->lcd_cfg.lcd_io_regulator, "")) || (!strcmp(lcdp->lcd_cfg.lcd_io_regulator, "none"))))
			OSAL_Power_Enable(lcdp->lcd_cfg.lcd_io_regulator);
	}

#if defined (__FPGA_DEBUG__)
#if defined(CONFIG_ARCH_SUN9IW1P1)
	if(!bon) {
		//pd28--pwm, ph0--pwr,pd29--bl_en
		writel(0x77777777, 0xf6000800+0x6c);
		writel(0x77777777, 0xf6000800+0x70);
		writel(0x77777777, 0xf6000800+0x74);
		writel(0x77727777, 0xf6000800+0x78);
		writel((readl(0xf6000800+0xfc) & (~0x0000000f)) | 0x00000007, 0xf6000800+0xfc);
	} else {
		//pd28--pwm, ph0--pwr,pd29--bl_en
		writel(0x22222222, (void __iomem *)(0xf6000800+0x6c));
		writel(0x22222222, (void __iomem *)(0xf6000800+0x70));
		writel(0x22222222, (void __iomem *)(0xf6000800+0x74));
		writel(0x77722222, (void __iomem *)(0xf6000800+0x78));
		writel(0x20000000, (void __iomem *)(0xf6000800+0x7c));
		writel((readl((void __iomem *)(0xf6000800+0xfc)) & (~0x0000000f)) | 0x00000001, (void __iomem *)(0xf6000800+0xfc));
		writel((readl((void __iomem *)(0xf6000800+0x10c)) & (~0x0000000f)) | 0x00000001, (void __iomem *)(0xf6000800+0x10c));
	}
#elif defined(CONFIG_ARCH_SUN8IW5P1)
	if(!bon) {
		//pd28--pwm, ph0--pwr,pd29--bl_en
		writel(0x77777777, (void __iomem *)(0xf1c20800+0x6c));
		writel(0x77777777, (void __iomem *)(0xf1c20800+0x70));
		writel(0x77777777, (void __iomem *)(0xf1c20800+0x74));
		writel(0x77727777, (void __iomem *)(0xf1c20800+0x78));
		writel((readl((void __iomem *)(0xf1c20800+0xfc)) & (~0x0000000f)) | 0x00000007, (void __iomem *)(0xf1c20800+0xfc));
	} else {
		//pd28--pwm, ph0--pwr,pd29--bl_en
		writel(0x22222222, (void __iomem *)(0xf1c20800+0x6c));
		writel(0x22222222, (void __iomem *)(0xf1c20800+0x70));
		writel(0x22222222, (void __iomem *)(0xf1c20800+0x74));
		writel(0x77122222, (void __iomem *)(0xf1c20800+0x78));
		writel(0x20000000, (void __iomem *)(0xf1c20800+0x7c));
		writel((readl((void __iomem *)(0xf1c20800+0xfc)) & (~0x0000000f)) | 0x00000001, (void __iomem *)(0xf1c20800+0xfc));
		writel((readl((void __iomem *)(0xf1c20800+0x10c)) & (~0x0000000f)) | 0x00000001, (void __iomem *)(0xf1c20800+0x10c));
	}
#endif
#endif
	for(i=0; i<28; i++)	{
		if(lcdp->lcd_cfg.lcd_io_used[i]) {
			disp_gpio_set_t  gpio_info[1];

			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_io[i]), sizeof(disp_gpio_set_t));
			if(!bon) {
				gpio_info->mul_sel = 7;
			}	else {
				if((lcdp->panel_info.lcd_if == 3) && (gpio_info->mul_sel==2))	{
					gpio_info->mul_sel = 3;
				}
			}
			lcd_pin_hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(lcd_pin_hdl, 2);
		}
	}

	disp_al_lcd_io_cfg(lcd->channel_id, bon, &lcdp->panel_info);

	if(bon == 0) {
		if(!((!strcmp(lcdp->lcd_cfg.lcd_io_regulator, "")) || (!strcmp(lcdp->lcd_cfg.lcd_io_regulator, "none"))))
			OSAL_Power_Disable(lcdp->lcd_cfg.lcd_io_regulator);
	}

	return DIS_SUCCESS;
}

static s32 disp_lcd_get_driver_name(struct disp_lcd *lcd, char* name)
{
	char primary_key[20];
	s32 ret;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	sprintf(primary_key, "lcd%d_para", lcd->channel_id);

	ret = OSAL_Script_FetchParser_Data(primary_key, "lcd_driver_name",  (int*)name, 2);
	DE_INF("disp_lcd_get_driver_name, %s\n", name);
	return ret;
}
static s32 lcd_clk_init(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("lcd %d clk init\n", lcd->channel_id);

	if((LCD_IF_LVDS == lcdp->panel_info.lcd_if) && (lcdp->lvds_clk.clk)) {
		lcdp->lvds_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->lvds_clk.clk);
		OSAL_CCMU_MclkOnOff(lcdp->lvds_clk.h_clk, CLK_OFF);
	} else if((LCD_IF_DSI == lcdp->panel_info.lcd_if) && (lcdp->dsi_clk.clk)) {
		lcdp->dsi_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->dsi_clk.clk);
		lcdp->dsi_clk.h_clk_p = OSAL_CCMU_OpenMclk(lcdp->dsi_clk.clk_p);
		OSAL_CCMU_SetMclkSrc(lcdp->dsi_clk.h_clk);
		OSAL_CCMU_SetMclkFreq(lcdp->dsi_clk.h_clk, 0);
		//OSAL_CCMU_SetMclkSrc(lcdp->dsi_clk.h_clk_p);
		//OSAL_CCMU_SetMclkFreq(lcdp->dsi_clk.h_clk_p, 0);

		OSAL_CCMU_MclkOnOff(lcdp->dsi_clk.h_clk, CLK_OFF);
		OSAL_CCMU_MclkOnOff(lcdp->dsi_clk.h_clk_p, CLK_OFF);
	} else if((LCD_IF_EDP == lcdp->panel_info.lcd_if) && (lcdp->edp_clk.clk)) {
		lcdp->edp_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->edp_clk.clk);
		OSAL_CCMU_MclkOnOff(lcdp->edp_clk.h_clk, CLK_OFF);
	}

	if(lcdp->lcd_clk.clk) {
		lcdp->lcd_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->lcd_clk.clk);
		OSAL_CCMU_SetMclkSrc(lcdp->lcd_clk.h_clk);
		OSAL_CCMU_MclkOnOff(lcdp->lcd_clk.h_clk, CLK_OFF);
	}
	bsp_disp_delay_ms(10);

	if(lcdp->sat_clk.clk) {
		lcdp->sat_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->sat_clk.clk);
		OSAL_CCMU_MclkOnOff(lcdp->sat_clk.h_clk, CLK_OFF);
	}

	if(lcdp->extra_clk.clk)
		disp_al_lcd_clk_init(lcdp->extra_clk.clk);

	if(lcdp->merge_clk.clk)
		disp_al_lcd_clk_init(lcdp->merge_clk.clk);

	return DIS_SUCCESS;
}

s32 lcd_clk_init_sw(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("lcd %d clk init\n", lcd->channel_id);

	if((LCD_IF_LVDS == lcdp->panel_info.lcd_if) && (lcdp->lvds_clk.clk)) {
		lcdp->lvds_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->lvds_clk.clk);
	} else if((LCD_IF_DSI == lcdp->panel_info.lcd_if) && (lcdp->dsi_clk.clk)) {
		lcdp->dsi_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->dsi_clk.clk);
		lcdp->dsi_clk.h_clk_p = OSAL_CCMU_OpenMclk(lcdp->dsi_clk.clk_p);
	} else if((LCD_IF_EDP == lcdp->panel_info.lcd_if) && (lcdp->edp_clk.clk)) {
		lcdp->edp_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->edp_clk.clk);
	}

	if(lcdp->lcd_clk.clk) {
		lcdp->lcd_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->lcd_clk.clk);
	}

	if(lcdp->sat_clk.clk) {
		lcdp->sat_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->sat_clk.clk);
	}

	return DIS_SUCCESS;
}

static s32 lcd_clk_exit(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(lcdp->lcd_clk.enabled == 1) {
		if(lcdp->lcd_clk.clk) {
			OSAL_CCMU_MclkOnOff(lcdp->lcd_clk.h_clk, CLK_OFF);
			OSAL_CCMU_CloseMclk(lcdp->lcd_clk.h_clk);
			lcdp->lcd_clk.h_clk = 0;
		}
		if((LCD_IF_LVDS == lcdp->panel_info.lcd_if) && lcdp->lvds_clk.clk) {
				OSAL_CCMU_MclkOnOff(lcdp->lvds_clk.h_clk, CLK_OFF);
				OSAL_CCMU_CloseMclk(lcdp->lvds_clk.h_clk);
				lcdp->lvds_clk.h_clk = 0;
		}
		if(LCD_IF_DSI == lcdp->panel_info.lcd_if) {
			if(lcdp->dsi_clk.h_clk) {
				OSAL_CCMU_MclkOnOff(lcdp->dsi_clk.h_clk, CLK_OFF);
				OSAL_CCMU_MclkOnOff(lcdp->dsi_clk.h_clk_p, CLK_OFF);
				OSAL_CCMU_CloseMclk(lcdp->dsi_clk.h_clk);
				OSAL_CCMU_CloseMclk(lcdp->dsi_clk.h_clk_p);
				lcdp->dsi_clk.h_clk = 0;
				lcdp->dsi_clk.h_clk_p = 0;
			}
		} else if(LCD_IF_EDP == lcdp->panel_info.lcd_if) {
			if(lcdp->edp_clk.h_clk) {
				OSAL_CCMU_MclkOnOff(lcdp->edp_clk.h_clk, CLK_OFF);
				OSAL_CCMU_CloseMclk(lcdp->edp_clk.h_clk);
				lcdp->edp_clk.h_clk = 0;
			}
		}

	if(lcdp->sat_clk.clk) {
		lcdp->sat_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->sat_clk.clk);
		OSAL_CCMU_MclkOnOff(lcdp->sat_clk.h_clk, CLK_OFF);
	}

	if(lcdp->extra_clk.clk)
		disp_al_lcd_clk_exit(lcdp->extra_clk.clk);

	if(lcdp->merge_clk.clk)
		disp_al_lcd_clk_exit(lcdp->merge_clk.clk);

#if defined(__LINUX_PLAT__)
	  {
	  	unsigned long flags;
	  	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
	  	lcdp->lcd_clk.enabled = 0;
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&lcd_data_lock, flags);
		}
#endif
	}

	return DIS_SUCCESS;
}

static s32 lcd_clk_config(struct disp_lcd* lcd)
{
	u32 lcd_dclk_freq;	//Hz,dclk
	u32 lcd_clk_freq;   //HZ,output lcd clk in ccm module
	s32 pll_freq = -1;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

#if defined(CONFIG_ARCH_SUN9IW1P1)
#else
			lcdp->lcd_clk.clk_div2 = 1;
#endif
	lcd_dclk_freq = lcdp->panel_info.lcd_dclk_freq * 1000000;
	if ((lcdp->panel_info.lcd_if == LCD_IF_HV) || (lcdp->panel_info.lcd_if == LCD_IF_CPU)
	    || (lcdp->panel_info.lcd_if == LCD_IF_EDP))	{
		lcdp->lcd_clk.clk_div = 6;
#if defined(CONFIG_ARCH_SUN9IW1P1)
		lcdp->lcd_clk.clk_div2 = 4;
#else
		lcdp->lcd_clk.clk_div2 = 1;
#endif
	}	else if(lcdp->panel_info.lcd_if == LCD_IF_LVDS) {
		lcdp->lcd_clk.clk_div = 7;
		lcdp->lcd_clk.clk_div2 = 1;
	}	else if(lcdp->panel_info.lcd_if == LCD_IF_DSI) {
		u32 lane = lcdp->panel_info.lcd_dsi_lane;
		u32 bitwidth = 0;

		switch(lcdp->panel_info.lcd_dsi_format) {
			case LCD_DSI_FORMAT_RGB888:
			bitwidth = 24;
			break;
			case LCD_DSI_FORMAT_RGB666:
			bitwidth = 24;
			break;
			case LCD_DSI_FORMAT_RGB565:
			bitwidth = 16;
			break;
			case LCD_DSI_FORMAT_RGB666P:
			bitwidth = 18;
			break;
		}
#if defined(CONFIG_ARCH_SUN9IW1P1)
		lcdp->lcd_clk.clk_div = bitwidth/lane;
#else
		lcdp->dsi_clk.clk_div = bitwidth/lane;
#endif
		if((lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_VIDEO_MODE) || (lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_BURST_MODE)) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
			lcdp->lcd_clk.clk_div = 1;
			lcdp->lcd_clk.clk_div2 = 4;

#else
			lcdp->lcd_clk.clk_div = 4;
#endif
		} else if(lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE) {
			//FIXME,sure?
			lcdp->lcd_clk.clk_div = 6;
			lcdp->lcd_clk.clk_div2 = 4;
		}
	}

#if defined(CONFIG_ARCH_SUN9IW1P1)
	lcd_clk_freq = lcd_dclk_freq * (lcdp->lcd_clk.clk_div);
#else
	if(lcdp->panel_info.lcd_if == LCD_IF_DSI)
		lcd_clk_freq = lcd_dclk_freq * (lcdp->dsi_clk.clk_div);
	else
		lcd_clk_freq = lcd_dclk_freq * (lcdp->lcd_clk.clk_div);
#endif
	pll_freq = lcd_clk_freq * (lcdp->lcd_clk.clk_div2);

	if(pll_freq != 0) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
#if 0
		if(lcdp->lcd_clk.clk)
			OSAL_CCMU_SetMclkFreq(lcdp->lcd_clk.h_clk, pll_freq);
#else
		if(lcdp->lcd_clk.clk) {
			u32 pll_freq_set, lcd_clk_freq_set,dclk_freq_set;
			lcdp->lcd_clk.clk_src = OSAL_CCMU_GetMclkSrc(lcdp->lcd_clk.h_clk);
			OSAL_CCMU_SetSrcFreq(lcdp->lcd_clk.clk_src, pll_freq);

			pll_freq_set = OSAL_CCMU_GetSrcFreq(lcdp->lcd_clk.clk_src);
			lcd_clk_freq_set = pll_freq_set / lcdp->lcd_clk.clk_div2;
			dclk_freq_set = lcd_clk_freq_set / lcdp->lcd_clk.clk_div;

			OSAL_CCMU_SetMclkFreq(lcdp->lcd_clk.h_clk, lcd_clk_freq_set);
			lcd_clk_freq_set = OSAL_CCMU_GetMclkFreq(lcdp->lcd_clk.h_clk);

			if((pll_freq_set != pll_freq) || (lcd_clk_freq_set != lcd_clk_freq)
				|| (dclk_freq_set != lcd_dclk_freq)) {
					DE_INF("screen %d, clk: pll(%d),clk(%d),dclk(%d) \n     clk real:pll(%d),clk(%d),dclk(%d)\n",
						lcd->channel_id, pll_freq, lcd_clk_freq, lcd_dclk_freq, pll_freq_set, lcd_clk_freq_set, dclk_freq_set);
			}
		}
#endif
#else
		if(lcdp->lcd_clk.clk) {
			lcdp->lcd_clk.clk_src = OSAL_CCMU_GetMclkSrc(lcdp->lcd_clk.h_clk);
			OSAL_CCMU_SetSrcFreq(lcdp->lcd_clk.clk_src, pll_freq);
		}
#endif

#if defined(CONFIG_ARCH_SUN9IW1P1)
		if(lcdp->panel_info.lcd_if == LCD_IF_DSI) {
			OSAL_CCMU_SetMclkFreq(lcdp->dsi_clk.h_clk, lcd_clk_freq);
		}
#else
		if(lcdp->panel_info.lcd_if == LCD_IF_DSI) {
			OSAL_CCMU_SetMclkFreq(lcdp->dsi_clk.h_clk_p, 0);
		}
#endif
	}

	return 0;
}

static s32 lcd_clk_config_sw(struct disp_lcd* lcd)
{
	u32 lcd_dclk_freq;	//Hz,dclk
	u32 lcd_clk_freq;   //HZ,output lcd clk in ccm module
	s32 pll_freq = -1;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

#if defined(CONFIG_ARCH_SUN9IW1P1)
#else
			lcdp->lcd_clk.clk_div2 = 1;
#endif
	lcd_dclk_freq = lcdp->panel_info.lcd_dclk_freq * 1000000;
	if ((lcdp->panel_info.lcd_if == LCD_IF_HV) || (lcdp->panel_info.lcd_if == LCD_IF_CPU)
	    || (lcdp->panel_info.lcd_if == LCD_IF_EDP))	{
		lcdp->lcd_clk.clk_div = 6;
		lcdp->lcd_clk.clk_div2 = 4;
	}	else if(lcdp->panel_info.lcd_if == LCD_IF_LVDS) {
		lcdp->lcd_clk.clk_div = 7;
		lcdp->lcd_clk.clk_div2 = 1;
	}	else if(lcdp->panel_info.lcd_if == LCD_IF_DSI) {
		u32 lane = lcdp->panel_info.lcd_dsi_lane;
		u32 bitwidth = 0;

		switch(lcdp->panel_info.lcd_dsi_format) {
			case LCD_DSI_FORMAT_RGB888:
			bitwidth = 24;
			break;
			case LCD_DSI_FORMAT_RGB666:
			bitwidth = 24;
			break;
			case LCD_DSI_FORMAT_RGB565:
			bitwidth = 16;
			break;
			case LCD_DSI_FORMAT_RGB666P:
			bitwidth = 18;
			break;
		}

		lcdp->lcd_clk.clk_div = bitwidth/lane;
		if((lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_VIDEO_MODE) || (lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_BURST_MODE)) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
			lcdp->lcd_clk.clk_div = 1;
			lcdp->lcd_clk.clk_div2 = 4;

#else
			lcdp->lcd_clk.clk_div = 4;
#endif
		} else if(lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE) {
			//FIXME,sure?
			lcdp->lcd_clk.clk_div = 6;
			lcdp->lcd_clk.clk_div2 = 4;
		}
	}

	lcd_clk_freq = lcd_dclk_freq * (lcdp->lcd_clk.clk_div);
	pll_freq = lcd_clk_freq * (lcdp->lcd_clk.clk_div2);

	if(pll_freq != 0) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
#if 0
		if(lcdp->lcd_clk.clk)
			OSAL_CCMU_SetMclkFreq(lcdp->lcd_clk.h_clk, pll_freq);
#else
		if(lcdp->lcd_clk.clk) {
			u32 pll_freq_set, lcd_clk_freq_set,dclk_freq_set;
			lcdp->lcd_clk.clk_src = OSAL_CCMU_GetMclkSrc(lcdp->lcd_clk.h_clk);

			pll_freq_set = OSAL_CCMU_GetSrcFreq(lcdp->lcd_clk.clk_src);
			lcd_clk_freq_set = pll_freq_set / lcdp->lcd_clk.clk_div2;
			dclk_freq_set = lcd_clk_freq_set / lcdp->lcd_clk.clk_div;

			lcd_clk_freq_set = OSAL_CCMU_GetMclkFreq(lcdp->lcd_clk.h_clk);

			if((pll_freq_set != pll_freq) || (lcd_clk_freq_set != lcd_clk_freq)
				|| (dclk_freq_set != lcd_dclk_freq)) {
					DE_WRN("screen %d, clk: pll(%d),clk(%d),dclk(%d) \n     clk real:pll(%d),clk(%d),dclk(%d)\n",
						lcd->channel_id, pll_freq, lcd_clk_freq, lcd_dclk_freq, pll_freq_set, lcd_clk_freq_set, dclk_freq_set);
			}
		}
#endif
#else
		if(lcdp->lcd_clk.clk) {
			lcdp->lcd_clk.clk_src = OSAL_CCMU_GetMclkSrc(lcdp->lcd_clk.h_clk);
		}
#endif

	}

	return 0;
}

static s32 lcd_clk_enable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	if(lcdp->extra_clk.clk)
		disp_al_lcd_clk_enable(lcdp->extra_clk.clk);

	if(lcdp->merge_clk.clk)
		disp_al_lcd_clk_enable(lcdp->merge_clk.clk);

	lcd_clk_config(lcd);

	if(lcdp->lcd_clk.clk && (LCD_IF_EDP != lcdp->panel_info.lcd_if))
		OSAL_CCMU_MclkOnOff(lcdp->lcd_clk.h_clk, CLK_ON);
	if((LCD_IF_LVDS == lcdp->panel_info.lcd_if) && lcdp->lvds_clk.clk) {
		OSAL_CCMU_MclkOnOff(lcdp->lvds_clk.h_clk, CLK_ON);
	} else if((LCD_IF_DSI == lcdp->panel_info.lcd_if) && lcdp->dsi_clk.clk) {
		OSAL_CCMU_MclkOnOff(lcdp->dsi_clk.h_clk, CLK_ON);
		OSAL_CCMU_MclkOnOff(lcdp->dsi_clk.h_clk_p, CLK_ON);
	} else if((LCD_IF_EDP == lcdp->panel_info.lcd_if) && lcdp->edp_clk.clk) {
		OSAL_CCMU_MclkOnOff(lcdp->edp_clk.h_clk, CLK_ON);
	}

	if(lcdp->sat_clk.clk) {
		lcdp->sat_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->sat_clk.clk);
		OSAL_CCMU_MclkOnOff(lcdp->sat_clk.h_clk, CLK_ON);
	}



#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
  	lcdp->lcd_clk.enabled = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif
	return	DIS_SUCCESS;
}

s32 lcd_clk_enable_sw(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	lcd_clk_config_sw(lcd);

	if(lcdp->sat_clk.clk) {
		lcdp->sat_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->sat_clk.clk);
	}



#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
	lcdp->lcd_clk.enabled = 1;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif
	return	DIS_SUCCESS;
}

static s32 lcd_clk_disable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(lcdp->lcd_clk.clk && (LCD_IF_EDP != lcdp->panel_info.lcd_if))
		OSAL_CCMU_MclkOnOff(lcdp->lcd_clk.h_clk, CLK_OFF);

	if((LCD_IF_LVDS == lcdp->panel_info.lcd_if) && lcdp->lvds_clk.clk) {
		OSAL_CCMU_MclkOnOff(lcdp->lvds_clk.h_clk, CLK_OFF);
	} else if((LCD_IF_DSI == lcdp->panel_info.lcd_if) && lcdp->dsi_clk.clk) {
		OSAL_CCMU_MclkOnOff(lcdp->dsi_clk.h_clk, CLK_OFF);
		OSAL_CCMU_MclkOnOff(lcdp->dsi_clk.h_clk_p, CLK_OFF);
	}  else if((LCD_IF_EDP == lcdp->panel_info.lcd_if) && lcdp->edp_clk.clk) {
		OSAL_CCMU_MclkOnOff(lcdp->edp_clk.h_clk, CLK_OFF);
	}

	if(lcdp->sat_clk.clk) {
		lcdp->sat_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->sat_clk.clk);
		OSAL_CCMU_MclkOnOff(lcdp->sat_clk.h_clk, CLK_OFF);
	}

	if(lcdp->extra_clk.clk)
		disp_al_lcd_clk_disable(lcdp->extra_clk.clk);

	if(lcdp->merge_clk.clk)
		disp_al_lcd_clk_disable(lcdp->merge_clk.clk);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
  	lcdp->lcd_clk.enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif
	return	DIS_SUCCESS;
}

s32 lcd_clk_disable_sw(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(lcdp->sat_clk.clk) {
		lcdp->sat_clk.h_clk = OSAL_CCMU_OpenMclk(lcdp->sat_clk.clk);
	}

#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
	lcdp->lcd_clk.enabled = 0;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif
	return	DIS_SUCCESS;
}

static s32 disp_lcd_tcon_enable(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return disp_al_lcd_enable(lcd->channel_id, 1, &lcdp->panel_info);
}

static s32 disp_lcd_tcon_disable(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return disp_al_lcd_enable(lcd->channel_id, 0, &lcdp->panel_info);
}

static s32 disp_lcd_set_open_func(struct disp_lcd* lcd, LCD_FUNC func, u32 delay)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(func) {
		lcdp->open_flow.func[lcdp->open_flow.func_num].func = func;
		lcdp->open_flow.func[lcdp->open_flow.func_num].delay = delay;
		lcdp->open_flow.func_num ++;
	}

	return DIS_SUCCESS;
}

static s32 disp_lcd_set_close_func(struct disp_lcd* lcd, LCD_FUNC func, u32 delay)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(func) {
		lcdp->close_flow.func[lcdp->close_flow.func_num].func = func;
		lcdp->close_flow.func[lcdp->close_flow.func_num].delay = delay;
		lcdp->close_flow.func_num ++;
	}

	return DIS_SUCCESS;
}

static s32 disp_lcd_set_panel_funs(struct disp_lcd* lcd, disp_lcd_panel_fun * lcd_cfg)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	memset(&lcdp->lcd_panel_fun, 0, sizeof(disp_lcd_panel_fun));
	lcdp->lcd_panel_fun.cfg_panel_info= lcd_cfg->cfg_panel_info;
	lcdp->lcd_panel_fun.cfg_open_flow = lcd_cfg->cfg_open_flow;
	lcdp->lcd_panel_fun.cfg_close_flow = lcd_cfg->cfg_close_flow;
	lcdp->lcd_panel_fun.lcd_user_defined_func = lcd_cfg->lcd_user_defined_func;
#if 0
	gdisp.lcd_registered = 1;
	if(gdisp.init_para.start_process) {
		gdisp.init_para.start_process();
	}
#endif

	return 0;
}

static disp_lcd_flow * disp_lcd_get_open_flow(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return &(lcdp->open_flow);
}

static disp_lcd_flow * disp_lcd_get_close_flow(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return &(lcdp->close_flow);
}

static s32 disp_lcd_pre_enable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 data[2];

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
		lcdp->enabling = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif
	/* notifier */
	data[0] = 1;/* enable */
	data[1] = (u32)DISP_OUTPUT_TYPE_LCD;
	disp_notifier_call_chain(DISP_EVENT_OUTPUT_ENABLE, lcd->channel_id, (void*)data);
	data[0] = DISP_OUT_CSC_TYPE_LCD;
	data[1] = DISP_COLOR_RANGE_0_255;
	disp_notifier_call_chain(DISP_EVENT_OUTPUT_CSC, lcd->channel_id, (void*)data);

	if(lcdp->lcd_panel_fun.cfg_panel_info)	{
		lcdp->lcd_panel_fun.cfg_panel_info(&lcdp->panel_info.lcd_extend_para);
	} else {
		DE_WRN("lcd_panel_fun[%d].cfg_panel_info is NULL\n", lcd->channel_id);
	}
	/* clk enable */
	if((NULL == lcd->p_sw_init_flag) || (0 == *(lcd->p_sw_init_flag))) {
		lcd_clk_enable(lcd);
		disp_al_lcd_init(lcd->channel_id);
		disp_al_lcd_cfg(lcd->channel_id, &lcdp->panel_info);
		disp_al_lcd_set_clk_div(lcd->channel_id, lcdp->lcd_clk.clk_div);
	} else {
#if defined(CONFIG_HOMLET_PLATFORM)
		lcd_clk_enable_sw(lcd);
		disp_al_lcd_init_sw(lcd->channel_id);
		disp_al_lcd_cfg(lcd->channel_id, &lcdp->panel_info);
		//disp_al_lcd_set_clk_div(lcd->channel_id, lcdp->lcd_clk.clk_div);
#endif
	}

	//gpio init
	disp_lcd_gpio_init(lcd);

	lcdp->open_flow.func_num = 0;
	if(lcdp->lcd_panel_fun.cfg_open_flow)	{
		lcdp->lcd_panel_fun.cfg_open_flow(lcd->channel_id);
	}	else {
		DE_WRN("lcd_panel_fun[%d].cfg_open_flow is NULL\n", lcd->channel_id);
	}
	return 0;
}

static s32 disp_lcd_post_enable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if((lcdp->panel_info.lcd_if == LCD_IF_EDP) && disp_al_query_edp_mod(lcd->channel_id)) {
		disp_al_edp_cfg(lcd->channel_id, &lcdp->panel_info);

		if(lcdp->irq_no_edp)
			OSAL_InterruptEnable(lcdp->irq_no_edp);
	}

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
		lcdp->enabling = 0;
		lcdp->enabled = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_lcd_pre_disable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
		lcdp->disabling = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif

	if((lcdp->panel_info.lcd_if == LCD_IF_EDP) && disp_al_query_edp_mod(lcd->channel_id)) {
		if(lcdp->irq_no_edp)
			OSAL_InterruptDisable(lcdp->irq_no_edp);

		disp_al_edp_disable_cfg(lcd->channel_id);
	}


	lcdp->close_flow.func_num = 0;
	if(lcdp->lcd_panel_fun.cfg_close_flow)	{
		lcdp->lcd_panel_fun.cfg_close_flow(lcd->channel_id);
	}	else {
		DE_WRN("lcd_panel_fun[%d].cfg_close_flow is NULL\n", lcd->channel_id);
	}

	return 0;
}

static s32 disp_lcd_post_disable(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 data[2];

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	lcd_clk_disable(lcd);

	//gpio exit
	disp_lcd_gpio_exit(lcd);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lcd_data_lock, flags);
#endif
		lcdp->disabling = 0;
		lcdp->enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lcd_data_lock, flags);
	}
#endif
	/* notifier */
	data[0] = 0;/* enable */
	data[1] = (u32)DISP_OUTPUT_TYPE_LCD;
	disp_notifier_call_chain(DISP_EVENT_OUTPUT_ENABLE, lcd->channel_id, (void*)data);

	return 0;
}

static s32 disp_lcd_is_enabled(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	return (1 == lcdp->enabled);
}

static s32 disp_lcd_backlight_enable(struct disp_lcd *lcd)
{
	disp_gpio_set_t  gpio_info[1];
	__hdle hdl;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	//io-pad
	if(!((!strcmp(lcdp->lcd_cfg.lcd_bl_regulator, "")) || (!strcmp(lcdp->lcd_cfg.lcd_bl_regulator, "none"))))
		OSAL_Power_Enable(lcdp->lcd_cfg.lcd_bl_regulator);

	if(disp_lcd_is_used(lcd)) {
		if(lcdp->lcd_cfg.lcd_bl_en_used) {
			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_bl_en), sizeof(disp_gpio_set_t));

			hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(hdl, 2);
		}
	} else {
	}

	return 0;
}

static s32 disp_lcd_backlight_disable(struct disp_lcd *lcd)
{
	disp_gpio_set_t  gpio_info[1];
	__hdle hdl;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd)) {
		if(lcdp->lcd_cfg.lcd_bl_en_used) {
			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_bl_en), sizeof(disp_gpio_set_t));
			gpio_info->data = (gpio_info->data==0)?1:0;
			gpio_info->mul_sel = 7;
			hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(hdl, 2);
		}
	}

	//io-pad
	if(!((!strcmp(lcdp->lcd_cfg.lcd_bl_regulator, "")) || (!strcmp(lcdp->lcd_cfg.lcd_bl_regulator, "none"))))
		OSAL_Power_Disable(lcdp->lcd_cfg.lcd_bl_regulator);

	return 0;
}

static s32 disp_lcd_pwm_enable(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		return OSAL_Pwm_Enable(lcdp->pwm_info.dev);
	}
	DE_WRN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}

static s32 disp_lcd_pwm_disable(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		return OSAL_Pwm_Disable(lcdp->pwm_info.dev);
	}
	DE_WRN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}


static s32 disp_lcd_power_enable(struct disp_lcd *lcd, u32 power_id)
{
	disp_gpio_set_t  gpio_info[1];
	__hdle hdl;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd)) {
		if(lcdp->lcd_cfg.lcd_power_type[power_id] == 1) {
			/* gpio type */
			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_power[power_id]), sizeof(disp_gpio_set_t));

			hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(hdl, 2);
		} else if(lcdp->lcd_cfg.lcd_power_type[power_id] == 2) {
			/* regulator type */
			OSAL_Power_Enable(lcdp->lcd_cfg.lcd_regu[power_id]);
		}
	}

	return 0;
}

static s32 disp_lcd_power_disable(struct disp_lcd *lcd, u32 power_id)
{
	disp_gpio_set_t  gpio_info[1];
	__hdle hdl;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(disp_lcd_is_used(lcd)) {
		if(lcdp->lcd_cfg.lcd_power_type[power_id] == 1) {
			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_power[power_id]), sizeof(disp_gpio_set_t));
			gpio_info->data = (gpio_info->data==0)?1:0;
			gpio_info->mul_sel = 7;
			hdl = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(hdl, 2);
		} else if(lcdp->lcd_cfg.lcd_power_type[power_id] == 2) {
			/* regulator type */
			OSAL_Power_Disable(lcdp->lcd_cfg.lcd_regu[power_id]);
		}
	}

	return 0;
}

static s32 disp_lcd_bright_get_adjust_value(struct disp_lcd *lcd, u32 bright)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	bright = (bright > 255)? 255:bright;
	return lcdp->panel_info.lcd_extend_para.lcd_bright_curve_tbl[bright];
}

static s32 disp_lcd_bright_curve_init(struct disp_lcd *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 i = 0, j=0;
	u32 items = 0;
	u32 lcd_bright_curve_tbl[101][2];

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	for(i = 0; i < 101; i++) {
		if(lcdp->lcd_cfg.backlight_curve_adjust[i] == 0) {
			if(i == 0) {
				lcd_bright_curve_tbl[items][0] = 0;
				lcd_bright_curve_tbl[items][1] = 0;
				items++;
			}
		}	else {
			lcd_bright_curve_tbl[items][0] = 255 * i / 100;
			lcd_bright_curve_tbl[items][1] = lcdp->lcd_cfg.backlight_curve_adjust[i];
			items++;
		}
	}

	for(i=0; i<items-1; i++) {
		u32 num = lcd_bright_curve_tbl[i+1][0] - lcd_bright_curve_tbl[i][0];

		for(j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_bright_curve_tbl[i][1] + ((lcd_bright_curve_tbl[i+1][1] - lcd_bright_curve_tbl[i][1]) * j)/num;
			lcdp->panel_info.lcd_extend_para.lcd_bright_curve_tbl[lcd_bright_curve_tbl[i][0] + j] = value;
		}
	}
	lcdp->panel_info.lcd_extend_para.lcd_bright_curve_tbl[255] = lcd_bright_curve_tbl[items-1][1];

	return 0;
}

static s32 disp_lcd_set_bright(struct disp_lcd *lcd, u32 bright)
{
	u32 duty_ns;
	__u64 backlight_bright = bright;
	__u64 backlight_dimming;
	__u64 period_ns;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	bool need_enable_bl = false, need_disable_bl = false;

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if((0 == lcdp->lcd_cfg.backlight_bright) && (0 != bright))
		need_enable_bl = true;
	if((0 != lcdp->lcd_cfg.backlight_bright) && (0 == bright))
		need_disable_bl = true;

	backlight_bright = (backlight_bright > 255)? 255:backlight_bright;
	if(lcdp->lcd_cfg.backlight_bright != backlight_bright) {
		lcdp->lcd_cfg.backlight_bright = backlight_bright;
		disp_notifier_call_chain(DISP_EVENT_BACKLIGHT_UPDATE, lcd->channel_id, (void*)bright);
	}

	if(lcdp->pwm_info.dev) {
		if(backlight_bright != 0)	{
			backlight_bright += 1;
		}
		backlight_bright = disp_lcd_bright_get_adjust_value(lcd, backlight_bright);

		lcdp->lcd_cfg.backlight_dimming = (0 == lcdp->lcd_cfg.backlight_dimming)? 256:lcdp->lcd_cfg.backlight_dimming;
		backlight_dimming = lcdp->lcd_cfg.backlight_dimming;
		period_ns = lcdp->pwm_info.period_ns;
		duty_ns = (backlight_bright * backlight_dimming *  period_ns/256 + 128) / 256;
		lcdp->pwm_info.duty_ns = duty_ns;
#if 0
		DE_DBG("[PWM]bright=%d, bright_modify=%d, backlight_dimming=%d, period_ns=%d, duty_ns=%d\n",
		bright,(u32)backlight_bright, lcdp->lcd_cfg.backlight_dimming,  (u32)period_ns, (u32)duty_ns);
#endif
		OSAL_Pwm_Config(lcdp->pwm_info.dev, duty_ns, period_ns);
	}

	return DIS_SUCCESS;
}

static s32 disp_lcd_get_bright(struct disp_lcd *lcd, u32 *bright)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*bright = lcdp->lcd_cfg.backlight_bright;
	return DIS_SUCCESS;
}

static s32 disp_lcd_update_bright_dimming(struct disp_lcd *lcd, u32 bright_dimming)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 backlight = 0;

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	lcdp->lcd_cfg.backlight_dimming = bright_dimming;
	disp_lcd_get_bright(lcd, &backlight);
	disp_lcd_set_bright(lcd, backlight);

	return DIS_SUCCESS;
}

static s32 disp_lcd_get_resolution(struct disp_lcd *lcd, u32 *xres, u32 *yres)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*xres = lcdp->panel_info.lcd_x;
	*yres = lcdp->panel_info.lcd_y;

	return 0;
}

static s32 disp_lcd_get_physical_size(struct disp_lcd *lcd, u32 *width, u32 *height)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*width = lcdp->panel_info.lcd_width;
	*height = lcdp->panel_info.lcd_height;

	return 0;
}

static s32 disp_lcd_get_input_csc(struct disp_lcd *lcd, disp_out_csc_type *csc_type)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*csc_type = DISP_OUT_CSC_TYPE_LCD;

	return 0;
}

static s32 disp_lcd_get_timing(struct disp_lcd *lcd, disp_video_timing * tt)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	memset(tt, 0, sizeof(disp_video_timing));
	tt->pixel_clk = lcdp->panel_info.lcd_dclk_freq * 1000;
	tt->x_res = lcdp->panel_info.lcd_x;
	tt->y_res = lcdp->panel_info.lcd_y;
	tt->hor_total_time= lcdp->panel_info.lcd_ht;
	tt->hor_sync_time= lcdp->panel_info.lcd_hspw;
	tt->hor_back_porch= lcdp->panel_info.lcd_hbp-lcdp->panel_info.lcd_hspw;
	tt->hor_front_porch= lcdp->panel_info.lcd_ht-lcdp->panel_info.lcd_hbp - lcdp->panel_info.lcd_x;
	tt->ver_total_time= lcdp->panel_info.lcd_vt;
	tt->ver_sync_time= lcdp->panel_info.lcd_vspw;
	tt->ver_back_porch= lcdp->panel_info.lcd_vbp-lcdp->panel_info.lcd_vspw;
	tt->ver_front_porch= lcdp->panel_info.lcd_vt-lcdp->panel_info.lcd_vbp -lcdp->panel_info.lcd_y;

	return 0;
}

static s32 disp_lcd_get_panel_info(struct disp_lcd *lcd, disp_panel_para* info)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	memcpy(info, (disp_panel_para*)(&(lcdp->panel_info)), sizeof(disp_panel_para));
	return 0;
}

s32 disp_lcd_get_tv_mode(struct disp_lcd *lcd)
{
	u32 channel_id;
	s32 tv_mode = -1;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	channel_id = lcd->channel_id;
	if(NULL != lcdp->lcd_panel_fun.lcd_user_defined_func) {
		tv_mode = lcdp->lcd_panel_fun.lcd_user_defined_func(channel_id, 0, 0, 0);
	} else {
		printk("lcd_user_defined_func for get cvbs mode is null!!!\n");
	}

	return tv_mode;
}

s32 disp_lcd_set_tv_mode(struct disp_lcd *lcd, disp_tv_mode tv_mode)
{
	disp_panel_para* info;
	u32 channel_id;
	s32 ret = -1;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	channel_id = lcd->channel_id;
	info = &(lcdp->panel_info);
	if(DISP_TV_MOD_PAL == tv_mode) {
        info->lcd_if = 0;
        info->lcd_x = 720;
        info->lcd_y = 576;
        info->lcd_hv_if = LCD_HV_IF_CCIR656_2CYC;
        info->lcd_dclk_freq = 27;
        info->lcd_ht = 864;
        info->lcd_hbp = 139;
        info->lcd_hspw = 2;
        info->lcd_vt = 625;
        info->lcd_vbp = 22;
        info->lcd_vspw = 2;
        info->lcd_hv_syuv_fdly = LCD_HV_SRGB_FDLY_3LINE;
        info->lcd_hv_syuv_seq = LCD_HV_SYUV_SEQ_UYUV;
		if(NULL != lcdp->lcd_panel_fun.lcd_user_defined_func) {
			lcdp->lcd_panel_fun.lcd_user_defined_func(channel_id, 1, tv_mode, 0);
		} else {
			printk("lcd_user_defined_func for cvbs is null!!!\n");
		}
#if defined(CONFIG_ARCH_SUN9IW1P1)
		disp_al_cfg_itl(channel_id, 1);
#endif
        ret = 0;
	} else if(DISP_TV_MOD_NTSC == tv_mode) {
        info->lcd_if = 0;
        info->lcd_x = 720;
        info->lcd_y = 480;
        info->lcd_hv_if = LCD_HV_IF_CCIR656_2CYC;
        info->lcd_dclk_freq = 27;
        info->lcd_ht = 858;
        info->lcd_hbp = 118;
        info->lcd_hspw = 2;
        info->lcd_vt = 525;
        info->lcd_vbp = 18;
        info->lcd_vspw = 2;
        info->lcd_hv_syuv_fdly = LCD_HV_SRGB_FDLY_2LINE;
        info->lcd_hv_syuv_seq = LCD_HV_SYUV_SEQ_UYUV;
		if(NULL != lcdp->lcd_panel_fun.lcd_user_defined_func) {
			lcdp->lcd_panel_fun.lcd_user_defined_func(channel_id, 1, tv_mode, 0);
		} else {
			printk("lcd_user_defined_func for cvbs is null!!!\n");
		}
#if defined(CONFIG_ARCH_SUN9IW1P1)
		disp_al_cfg_itl(channel_id, 1);
#endif
        ret = 0;
	}
	return ret;
}

extern void sync_event_proc(u32 screen_id);
#if defined(__LINUX_PLAT__)
static s32 disp_lcd_event_proc(int irq, void *parg)
#else
static s32 disp_lcd_event_proc(void *parg)
#endif
{
	u32 screen_id = (u32)parg;
	static u32 cntr=0;
	struct disp_lcd* lcd = NULL;
	struct disp_lcd_private_data *lcdp = NULL;

	if(tcon_irq_query(screen_id,LCD_IRQ_TCON0_VBLK) || tcon_irq_query(screen_id,LCD_IRQ_TCON1_VBLK)
	    || tcon_irq_query(screen_id,LCD_IRQ_TCON0_CNTR) || dsi_irq_query(screen_id,DSI_IRQ_VIDEO_VBLK)) {
		sync_event_proc(screen_id);
	}

	if(tcon_irq_query(screen_id,LCD_IRQ_TCON0_CNTR)) {
		sync_event_proc(screen_id);

		if(disp_al_lcd_tri_busy(screen_id))	{
			if(cntr>=1)	{
				cntr = 0;
			}	else {
				cntr++;
			}
		}	else {
			cntr = 0;
		}

		if(cntr==0)	{
			/* FIXME cpu_isr */
			disp_al_lcd_tri_start(screen_id);
		}
	}

	lcd = disp_get_lcd(screen_id);
	if(lcd) {
		lcdp = disp_lcd_get_priv(lcd);
		if(lcdp && (lcdp->panel_info.lcd_if == LCD_IF_EDP)) {
			if(disp_al_query_edp_mod(screen_id)) {
				if(disp_al_edp_int(EDP_IRQ_VBLK) == 1)
					sync_event_proc(screen_id);
			}
		}
	}

	return OSAL_IRQ_RETURN;
}

static s32 disp_lcd_notifier_callback(struct disp_notifier_block *self,
		 u32 event, u32 sel, void *data)
{
	struct disp_lcd *lcd;
	u32 *ptr = (u32 *)data;
	u32 backlight, backlight_dimming;

	lcd = disp_get_lcd(sel);
	if(!lcd)
		return -1;

	DE_INF("notifier cb: event=0x%x, sel=%d, data=0x%x\n", event, sel, (u32)data);
	switch(event){
	case DISP_EVENT_BACKLIGHT_DIMMING_UPDATE:
		backlight_dimming = (u32)ptr;
		disp_lcd_update_bright_dimming(lcd, backlight_dimming);
		disp_lcd_get_bright(lcd, &backlight);
		disp_lcd_set_bright(lcd, backlight);
		break;

	default:
		break;
	}
	return 0;
}

s32 disp_lcd_gpio_init(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 i = 0;

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(!disp_al_query_lcd_mod(lcd->channel_id)) {
		DE_WRN("lcd %d is not register\n", lcd->channel_id);
		return DIS_FAIL;
	}

	//io-pad
	for(i = 0; i < LCD_GPIO_REGU_NUM; i++)
	{
		if(!((!strcmp(lcdp->lcd_cfg.lcd_gpio_regulator[i], "")) || (!strcmp(lcdp->lcd_cfg.lcd_gpio_regulator[i], "none"))))
			OSAL_Power_Enable(lcdp->lcd_cfg.lcd_gpio_regulator[i]);
	}

	for(i=0; i<LCD_GPIO_NUM; i++) {
		lcdp->lcd_cfg.gpio_hdl[i] = 0;

		if(lcdp->lcd_cfg.lcd_gpio_used[i]) {
			disp_gpio_set_t  gpio_info[1];

			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_gpio[i]), sizeof(disp_gpio_set_t));
			lcdp->lcd_cfg.gpio_hdl[i] = OSAL_GPIO_Request(gpio_info, 1);
		}
	}

	return 0;
}

s32 disp_lcd_gpio_exit(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 i = 0;

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(!disp_al_query_lcd_mod(lcd->channel_id)) {
		DE_WRN("lcd %d is not register\n", lcd->channel_id);
		return DIS_FAIL;
	}

	for(i=0; i<LCD_GPIO_NUM; i++) {
		if(lcdp->lcd_cfg.gpio_hdl[i]) {
			disp_gpio_set_t  gpio_info[1];

			OSAL_GPIO_Release(lcdp->lcd_cfg.gpio_hdl[i], 2);

			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_gpio[i]), sizeof(disp_gpio_set_t));
			gpio_info->mul_sel = 7;
			lcdp->lcd_cfg.gpio_hdl[i] = OSAL_GPIO_Request(gpio_info, 1);
			OSAL_GPIO_Release(lcdp->lcd_cfg.gpio_hdl[i], 2);
			lcdp->lcd_cfg.gpio_hdl[i] = 0;
		}
	}

	//io-pad
	for(i = 0; i < LCD_GPIO_REGU_NUM; i++)
	{
		if(!((!strcmp(lcdp->lcd_cfg.lcd_gpio_regulator[i], "")) || (!strcmp(lcdp->lcd_cfg.lcd_gpio_regulator[i], "none"))))
			OSAL_Power_Disable(lcdp->lcd_cfg.lcd_gpio_regulator[i]);
	}

	return 0;
}

//direction: input(0), output(1)
static s32 disp_lcd_gpio_set_direction(struct disp_lcd* lcd, u32 io_index, u32 direction)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char gpio_name[20];

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(!disp_al_query_lcd_mod(lcd->channel_id)) {
		DE_WRN("lcd %d is not register\n", lcd->channel_id);
		return DIS_FAIL;
	}

	sprintf(gpio_name, "lcd_gpio_%d", io_index);
	return  OSAL_GPIO_DevSetONEPIN_IO_STATUS(lcdp->lcd_cfg.gpio_hdl[io_index], direction, gpio_name);
}

s32 disp_lcd_gpio_get_value(struct disp_lcd* lcd,__u32 io_index)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char gpio_name[20];

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(!disp_al_query_lcd_mod(lcd->channel_id)) {
		DE_WRN("lcd %d is not register\n", lcd->channel_id);
		return DIS_FAIL;
	}

	sprintf(gpio_name, "lcd_gpio_%d", io_index);
	return OSAL_GPIO_DevREAD_ONEPIN_DATA(lcdp->lcd_cfg.gpio_hdl[io_index], gpio_name);
}

static s32 disp_lcd_gpio_set_value(struct disp_lcd* lcd, u32 io_index, u32 data)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char gpio_name[20];

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(!disp_al_query_lcd_mod(lcd->channel_id)) {
		DE_WRN("lcd %d is not register\n", lcd->channel_id);
		return DIS_FAIL;
	}

	sprintf(gpio_name, "lcd_gpio_%d", io_index);
	return OSAL_GPIO_DevWRITE_ONEPIN_DATA(lcdp->lcd_cfg.gpio_hdl[io_index], data, gpio_name);
}

static s32 disp_lcd_init(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_notifier_block *nb;

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	lcd_get_sys_config(lcd->channel_id, &lcdp->lcd_cfg);
	lcd_parse_panel_para(lcd->channel_id, &lcdp->panel_info);

	/* register one notifier for all lcd */
	if(0 == lcd->channel_id) {
		nb = (struct disp_notifier_block *)OSAL_malloc(sizeof(struct disp_notifier_block));
		if(nb) {
			nb->notifier_call = &disp_lcd_notifier_callback;
			disp_notifier_register(nb);
		} else
			DE_WRN("malloc memory fail!\n");
	}

	if(disp_lcd_is_used(lcd)) {
		if(lcdp->panel_info.lcd_pwm_used) {
			lcdp->pwm_info.channel = lcdp->panel_info.lcd_pwm_ch;
			lcdp->pwm_info.polarity = lcdp->panel_info.lcd_pwm_pol;
			lcdp->pwm_info.dev = OSAL_Pwm_request(lcdp->panel_info.lcd_pwm_ch);
		}
		disp_lcd_backlight_disable(lcd);
	}
	disp_lcd_bright_curve_init(lcd);
	lcdp->lcd_cfg.backlight_dimming = 256;

	if((NULL != lcd->p_sw_init_flag) && (0 != *(lcd->p_sw_init_flag))) {
#if defined(CONFIG_HOMLET_PLATFORM)
		lcd_clk_init_sw(lcd);
		lcd_clk_enable_sw(lcd);
		disp_al_lcd_init_sw(lcd->channel_id);
		lcd_clk_disable_sw(lcd);
#endif
	} else {
		lcd_clk_init(lcd);
		lcd_clk_enable(lcd);
		disp_al_lcd_init(lcd->channel_id);
		lcd_clk_disable(lcd);
	}

	disp_al_edp_init(lcd->channel_id, lcdp->panel_info.lcd_edp_rate);

	//lcd_panel_parameter_check(lcd->channel_id, lcd);

	if(disp_al_query_lcd_mod(lcd->channel_id)) {
		OSAL_RegISR(lcdp->irq_no,0,disp_lcd_event_proc,(void*)lcd->channel_id,0,0);
#if !defined(__LINUX_PLAT__)
		OSAL_InterruptEnable(lcdp->irq_no);
#endif
	}

	if(LCD_IF_DSI == lcdp->panel_info.lcd_if)	{
		OSAL_RegISR(lcdp->irq_no_dsi,0,disp_lcd_event_proc,(void*)lcd->channel_id,0,0);
#if !defined(__LINUX_PLAT__)
		OSAL_InterruptEnable(lcdp->irq_no_dsi);
#endif
	} else if(LCD_IF_EDP == lcdp->panel_info.lcd_if) {
		OSAL_RegISR(lcdp->irq_no_edp,0,disp_lcd_event_proc,(void*)lcd->channel_id,0,0);
		OSAL_InterruptDisable(lcdp->irq_no_edp);
#if !defined(__LINUX_PLAT__)
		OSAL_InterruptEnable(lcdp->irq_no_edp);
#endif
	}

	if(lcdp->pwm_info.dev) {
		__u64 backlight_bright;
		__u64 period_ns, duty_ns;

		if(lcdp->panel_info.lcd_pwm_freq != 0) {
			period_ns = 1000*1000*1000 / lcdp->panel_info.lcd_pwm_freq;
		} else {
			DE_WRN("lcd%d.lcd_pwm_freq is ZERO\n", lcd->channel_id);
			period_ns = 1000*1000*1000 / 1000;  //default 1khz
		}

		backlight_bright = lcdp->lcd_cfg.backlight_bright;

		duty_ns = (backlight_bright * period_ns) / 256;
		DE_DBG("[PWM]backlight_bright=%d,period_ns=%d,duty_ns=%d\n",(u32)backlight_bright,(u32)period_ns, (u32)duty_ns);
		OSAL_Pwm_Set_Polarity(lcdp->pwm_info.dev, lcdp->pwm_info.polarity);
		OSAL_Pwm_Config(lcdp->pwm_info.dev, duty_ns, period_ns);
		lcdp->pwm_info.duty_ns = duty_ns;
		lcdp->pwm_info.period_ns = period_ns;
	}

	return 0;
}

static s32 disp_lcd_exit(struct disp_lcd* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if(!disp_al_query_lcd_mod(lcd->channel_id)) {
		DE_WRN("lcd %d is not register\n", lcd->channel_id);
		return DIS_FAIL;
	}

	if(disp_al_query_lcd_mod(lcd->channel_id)) {
		OSAL_InterruptDisable(lcdp->irq_no);
		OSAL_UnRegISR(lcdp->irq_no, disp_lcd_event_proc,(void*)lcd->channel_id);
	}

	if(LCD_IF_DSI == lcdp->panel_info.lcd_if)	{
		OSAL_InterruptDisable(lcdp->irq_no_dsi);
		OSAL_UnRegISR(lcdp->irq_no_dsi, disp_lcd_event_proc,(void*)lcd->channel_id);
	} else if(LCD_IF_EDP == lcdp->panel_info.lcd_if)	{
		/* FIXME unregister edp vint proc */
		OSAL_InterruptDisable(lcdp->irq_no_edp);
		OSAL_UnRegISR(lcdp->irq_no_edp, disp_lcd_event_proc,(void*)lcd->channel_id);
	}

	disp_al_lcd_exit(lcd->channel_id);
	lcd_clk_exit(lcd);

	return 0;
}

s32 disp_init_lcd(__disp_bsp_init_para * para)
{
	u32 num_screens;
	u32 screen_id;
	struct disp_lcd *lcd;
	struct disp_lcd_private_data *lcdp;

	DE_INF("disp_init_lcd\n");

#if defined(__LINUX_PLAT__)
	spin_lock_init(&lcd_data_lock);
#endif
	num_screens = bsp_disp_feat_get_num_screens();
	lcds = (struct disp_lcd *)OSAL_malloc(sizeof(struct disp_lcd) * num_screens);
	if(NULL == lcds) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	lcd_private = (struct disp_lcd_private_data *)OSAL_malloc(sizeof(struct disp_lcd_private_data) * num_screens);
	if(NULL == lcd_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	for(screen_id=0; screen_id<num_screens; screen_id++) {

		lcd = &lcds[screen_id];
		lcdp = &lcd_private[screen_id];

		switch(screen_id) {
		case 0:
			lcd->name = "lcd0";
			lcd->channel_id = 0;
			lcd->type = DISP_OUTPUT_TYPE_LCD;
			lcdp->irq_no = para->irq_no[DISP_MOD_LCD0];
			lcdp->irq_no_dsi = para->irq_no[DISP_MOD_DSI0];
			lcdp->reg_base = para->reg_base[DISP_MOD_LCD0];
			lcdp->reg_base_dsi = para->reg_base[DISP_MOD_DSI0];
			lcdp->lcd_clk.clk = MOD_CLK_LCD0CH0;
			lcdp->lvds_clk.clk = MOD_CLK_LVDS;
			lcdp->dsi_clk.clk = MOD_CLK_MIPIDSIS;
			lcdp->dsi_clk.clk_div = 1;

			lcdp->dsi_clk.clk_p = MOD_CLK_MIPIDSIP;
			lcdp->dsi_clk.clk_div_p = 2;

			lcdp->extra_clk.clk = MOD_CLK_IEPDRC0;
			lcdp->extra_clk.clk_div = 3;

			lcdp->edp_clk.clk = MOD_CLK_EDP;
			lcdp->irq_no_edp = para->irq_no[DISP_MOD_EDP];

			lcdp->merge_clk.clk = MOD_CLK_MERGE;

			lcdp->sat_clk.clk = MOD_CLK_SAT0;

			break;
		case 1:
			lcd->name = "lcd1";
			lcd->channel_id = 1;
			lcd->type = DISP_OUTPUT_TYPE_LCD;
			lcdp->irq_no = para->irq_no[DISP_MOD_LCD1];
			lcdp->reg_base = para->reg_base[DISP_MOD_LCD1];
			lcdp->lcd_clk.clk = MOD_CLK_LCD1CH0;
			lcdp->lcd_clk.clk_src = MOD_CLK_LCD1CH0;

			lcdp->lvds_clk.clk = MOD_CLK_LVDS;
			lcdp->dsi_clk.clk = MOD_CLK_MIPIDSIS;
			lcdp->dsi_clk.clk_div = 1;

			lcdp->dsi_clk.clk_p = MOD_CLK_MIPIDSIP;
			lcdp->dsi_clk.clk_div_p = 2;

			lcdp->extra_clk.clk = MOD_CLK_IEPDRC1;
			lcdp->extra_clk.clk_div = 3;

			break;
		case 2:
			lcd->name = "lcd2";
			lcd->channel_id = 2;
			lcd->type = DISP_OUTPUT_TYPE_LCD;
			//lcdp->reg_base = para->reg_base[DISP_MOD_LCD2];
			//lcdp->lcd_clk.clk = MOD_CLK_LCD2CH0;

			lcdp->lvds_clk.clk = MOD_CLK_LVDS;
			lcdp->dsi_clk.clk = MOD_CLK_MIPIDSIS;
			lcdp->dsi_clk.clk_div = 1;

			lcdp->dsi_clk.clk_p = MOD_CLK_MIPIDSIP;
			lcdp->dsi_clk.clk_div_p = 2;

			lcdp->edp_clk.clk = MOD_CLK_EDP;
			lcdp->irq_no_edp = para->irq_no[DISP_MOD_EDP];

			lcdp->merge_clk.clk = MOD_CLK_MERGE;
			break;
		}
		DE_INF("lcd %d, reg_base=0x%x, irq_no=%d, reg_base_dsi=0x%x, irq_no_dsi=%d\n",
		    screen_id, lcdp->reg_base, lcdp->irq_no, lcdp->reg_base_dsi, lcdp->irq_no_dsi);

		lcd->p_sw_init_flag = ((1 << screen_id) & para->sw_init_para->sw_init_flag) ?
			(&(para->sw_init_para->sw_init_flag)) : NULL;

		lcd->is_enabled = disp_lcd_is_enabled;
		lcd->is_used = disp_lcd_is_used;
		lcd->get_resolution = disp_lcd_get_resolution;
		lcd->get_physical_size = disp_lcd_get_physical_size;
		lcd->get_input_csc = disp_lcd_get_input_csc;

		lcd->init = disp_lcd_init;
		lcd->exit = disp_lcd_exit;

		//lcd->apply

//		lcd->early_suspend
//		lcd->late_resume
//		lcd->suspend
//		lcd->resume

		lcd->backlight_enable = disp_lcd_backlight_enable;
		lcd->backlight_disable = disp_lcd_backlight_disable;
		lcd->pwm_enable = disp_lcd_pwm_enable;
		lcd->pwm_disable = disp_lcd_pwm_disable;
		lcd->power_enable = disp_lcd_power_enable;
		lcd->power_disable = disp_lcd_power_disable;
		lcd->pin_cfg = disp_lcd_pin_cfg;
		lcd->set_bright = disp_lcd_set_bright;
		lcd->get_bright = disp_lcd_get_bright;
		lcd->set_bright_dimming = disp_lcd_update_bright_dimming;
		lcd->get_timing = disp_lcd_get_timing;
		lcd->get_open_flow = disp_lcd_get_open_flow;
		lcd->get_close_flow  = disp_lcd_get_close_flow;
		lcd->pre_enable = disp_lcd_pre_enable;
		lcd->post_enable = disp_lcd_post_enable;;
		lcd->pre_disable = disp_lcd_pre_disable;
		lcd->post_disable  = disp_lcd_post_disable;
		lcd->tcon_enable = disp_lcd_tcon_enable;
		lcd->tcon_disable = disp_lcd_tcon_disable;
		lcd->set_panel_func = disp_lcd_set_panel_funs;
		lcd->set_open_func = disp_lcd_set_open_func;
		lcd->set_close_func = disp_lcd_set_close_func;
		lcd->get_panel_driver_name = disp_lcd_get_driver_name;
		lcd->gpio_set_direction = disp_lcd_gpio_set_direction;
		lcd->gpio_set_value = disp_lcd_gpio_set_value;
		lcd->get_panel_info = disp_lcd_get_panel_info;
		lcd->get_tv_mode = disp_lcd_get_tv_mode;
		lcd->set_tv_mode = disp_lcd_set_tv_mode;

		lcd->init(lcd);
	}

	return 0;
}


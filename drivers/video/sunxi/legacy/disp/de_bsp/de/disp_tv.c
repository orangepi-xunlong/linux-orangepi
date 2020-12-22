#include "disp_tv.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_de.h"
#include "disp_lcd.h"
#include "disp_clk.h"

__s32 Disp_Switch_Dram_Mode(__u32 type, __u8 tv_mod)
{
	return DIS_SUCCESS;
}

__s32 Disp_TVEC_Init(__u32 screen_id)
{
	__s32 ret = 0, value = 0;

	tve_clk_init(screen_id);
	tve_clk_on(screen_id);
	TVE_init(screen_id);
	tve_clk_off(screen_id);

	gdisp.screen[screen_id].dac_source[0] = DISP_TV_DAC_SRC_Y;
	gdisp.screen[screen_id].dac_source[1] = DISP_TV_DAC_SRC_PB;
	gdisp.screen[screen_id].dac_source[2] = DISP_TV_DAC_SRC_PR;
	gdisp.screen[screen_id].dac_source[3] = DISP_TV_DAC_SRC_COMPOSITE;

	ret = OSAL_Script_FetchParser_Data("tv_para", "dac_used", &value, 1);
	if(ret == 0) {
		if(value != 0) {
			__s32 i = 0;
			char sub_key[20];

			for(i=0; i<4; i++) {
				sprintf(sub_key, "dac%d_src", i);

				ret = OSAL_Script_FetchParser_Data("tv_out_dac_para", sub_key, &value, 1);
				if(ret == 0) {
					gdisp.screen[screen_id].dac_source[i] = value;
				}
			}
		}
	}

	gdisp.screen[screen_id].tv_mode = DISP_TV_MOD_720P_50HZ;
	return DIS_SUCCESS;
}


__s32 Disp_TVEC_Exit(__u32 screen_id)
{
	TVE_exit(screen_id);
	tve_clk_exit(screen_id);

	return DIS_SUCCESS;
}

__s32 Disp_TVEC_Open(__u32 screen_id)
{
	TVE_open(screen_id);
	return DIS_SUCCESS;
}

__s32 Disp_TVEC_Close(__u32 screen_id)
{
	TVE_dac_disable(screen_id, 0);
	TVE_dac_disable(screen_id, 1);
	TVE_dac_disable(screen_id, 2);
	TVE_dac_disable(screen_id, 3);

	TVE_close(screen_id);

	return DIS_SUCCESS;
}

static void Disp_TVEC_DacCfg(__u32 screen_id, __u8 mode)
{
	__u32 i = 0;

	TVE_dac_disable(screen_id, 0);
	TVE_dac_disable(screen_id, 1);
	TVE_dac_disable(screen_id, 2);
	TVE_dac_disable(screen_id, 3);

	switch(mode) {
	case DISP_TV_MOD_NTSC:
	case DISP_TV_MOD_PAL:
	case DISP_TV_MOD_PAL_M:
	case DISP_TV_MOD_PAL_NC:
	{
		for(i=0; i<4; i++) {
			if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_COMPOSITE) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_COMPOSITE);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}
		}
	}
	break;

	case DISP_TV_MOD_NTSC_SVIDEO:
	case DISP_TV_MOD_PAL_SVIDEO:
	case DISP_TV_MOD_PAL_M_SVIDEO:
	case DISP_TV_MOD_PAL_NC_SVIDEO:
	{
		for(i=0; i<4; i++) {
			if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_LUMA) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_LUMA);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_CHROMA) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_CHROMA);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}
		}
	}
	break;

	case DISP_TV_MOD_480I:
	case DISP_TV_MOD_576I:
	case DISP_TV_MOD_480P:
	case DISP_TV_MOD_576P:
	case DISP_TV_MOD_720P_50HZ:
	case DISP_TV_MOD_720P_60HZ:
	case DISP_TV_MOD_1080I_50HZ:
	case DISP_TV_MOD_1080I_60HZ:
	case DISP_TV_MOD_1080P_50HZ:
	case DISP_TV_MOD_1080P_60HZ:
	{
		for(i=0; i<4; i++) {
			if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_Y) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_Y);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_PB) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_PB);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_PR) {
				TVE_dac_set_source(screen_id, i, DISP_TV_DAC_SRC_PR);
				TVE_dac_enable(screen_id, i);
				TVE_dac_sel(screen_id, i, i);
			}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_COMPOSITE) {
				  TVE_dac_set_source(1-screen_id, i, DISP_TV_DAC_SRC_COMPOSITE);
				  TVE_dac_sel(1-screen_id, i, i);
			}
		}
	}
		break;

	default:
		break;
	}
}

__s32 bsp_disp_tv_open(__u32 screen_id)
{
	if(!(gdisp.screen[screen_id].status & TV_ON))
	{
	__disp_tv_mode_t     tv_mod;

	tv_mod = gdisp.screen[screen_id].tv_mode;

	image_clk_on(screen_id, 1);
	Image_open(screen_id);//set image normal channel start bit , because every de_clk_off( )will reset this bit

	disp_clk_cfg(screen_id,DISP_OUTPUT_TYPE_TV, tv_mod);
	tve_clk_on(screen_id);
	lcdc_clk_on(screen_id, 1, 0);
	lcdc_clk_on(screen_id, 1, 1);
	tcon_init(screen_id);

	gdisp.screen[screen_id].output_csc_type = DISP_OUT_CSC_TYPE_TV;
	bsp_disp_set_output_csc(screen_id, gdisp.screen[screen_id].output_csc_type, bsp_disp_drc_get_input_csc(screen_id));
	DE_BE_set_display_size(screen_id, tv_mode_to_width(tv_mod), tv_mode_to_height(tv_mod));
	DE_BE_Output_Select(screen_id, screen_id);

	tcon1_set_tv_mode(screen_id,tv_mod);
	TVE_set_tv_mode(screen_id, tv_mod);
	Disp_TVEC_DacCfg(screen_id, tv_mod);

	tcon1_open(screen_id);
	Disp_TVEC_Open(screen_id);

	Disp_Switch_Dram_Mode(DISP_OUTPUT_TYPE_TV, tv_mod);
#ifdef __LINUX_OSAL__
	{
		disp_gpio_set_t  gpio_info[1];
		__hdle gpio_pa_shutdown;
		__s32 ret;

		memset(gpio_info, 0, sizeof(disp_gpio_set_t));
		ret = OSAL_Script_FetchParser_Data("audio_para","audio_pa_ctrl", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
		if(ret == 0) {
			gpio_pa_shutdown = OSAL_GPIO_Request(gpio_info, 1);
			if(!gpio_pa_shutdown) {
				DE_WRN("audio codec_wakeup request gpio fail!\n");
			} else {
				OSAL_GPIO_DevWRITE_ONEPIN_DATA(gpio_pa_shutdown, 0, "audio_pa_ctrl");
			}
		}
	}
#endif
	gdisp.screen[screen_id].b_out_interlace = disp_get_screen_scan_mode(tv_mod);
	gdisp.screen[screen_id].status |= TV_ON;
	gdisp.screen[screen_id].lcdc_status |= LCDC_TCON1_USED;
	gdisp.screen[screen_id].output_type = DISP_OUTPUT_TYPE_TV;

	Disp_set_out_interlace(screen_id);
#ifdef __LINUX_OSAL__
	Display_set_fb_timming(screen_id);
#endif
	}
	return DIS_SUCCESS;
}


__s32 bsp_disp_tv_close(__u32 screen_id)
{
	if(gdisp.screen[screen_id].status & TV_ON)
	{
	Image_close(screen_id);
	tcon1_close(screen_id);
	Disp_TVEC_Close(screen_id);

	tve_clk_off(screen_id);
	image_clk_off(screen_id, 1);
	lcdc_clk_off(screen_id);

#ifdef __LINUX_OSAL__
	{
		disp_gpio_set_t  gpio_info[1];
		__hdle gpio_pa_shutdown;
		__s32 ret;

		memset(gpio_info, 0, sizeof(disp_gpio_set_t));
		ret = OSAL_Script_FetchParser_Data("audio_para","audio_pa_ctrl", (int *)gpio_info, sizeof(disp_gpio_set_t)/sizeof(int));
		if(ret < 0) {
			DE_WRN("fetch script data audio_para.audio_pa_ctrl fail\n");
		}	else {
			gpio_pa_shutdown = OSAL_GPIO_Request(gpio_info, 1);
			if(!gpio_pa_shutdown) {
				DE_WRN("audio codec_wakeup request gpio fail!\n");
			}	else {
				OSAL_GPIO_DevWRITE_ONEPIN_DATA(gpio_pa_shutdown, 1, "audio_pa_ctrl");
			}
		}
	}
#endif
	gdisp.screen[screen_id].b_out_interlace = 0;
	gdisp.screen[screen_id].status &= TV_OFF;
	gdisp.screen[screen_id].lcdc_status &= LCDC_TCON1_USED_MASK;
	gdisp.screen[screen_id].output_type = DISP_OUTPUT_TYPE_NONE;
	gdisp.screen[screen_id].pll_use_status &= ((gdisp.screen[screen_id].pll_use_status == VIDEO_PLL0_USED)? VIDEO_PLL0_USED_MASK : VIDEO_PLL1_USED_MASK);

	Disp_set_out_interlace(screen_id);
	}
	return DIS_SUCCESS;
}

__s32 bsp_disp_tv_set_mode(__u32 screen_id, __disp_tv_mode_t tv_mod)
{
	if(tv_mod >= DISP_TV_MODE_NUM) {
		DE_WRN("unsupported tv mode:%d in bsp_disp_tv_set_mode\n", tv_mod);
		return DIS_FAIL;
	}

	gdisp.screen[screen_id].tv_mode = tv_mod;
	gdisp.screen[screen_id].output_type = DISP_OUTPUT_TYPE_TV;
	return DIS_SUCCESS;
}


__s32 bsp_disp_tv_get_mode(__u32 screen_id)
{
	return gdisp.screen[screen_id].tv_mode;
}


__s32 bsp_disp_tv_get_interface(__u32 screen_id)
{
	__u8 dac[4] = {0};
	__s32 i = 0;
	__u32  ret = DISP_TV_NONE;

	for(i=0; i<4; i++) {
		dac[i] = TVE_get_dac_status(i);
		if(dac[i]>1) {
			DE_WRN("dac %d short to ground\n", i);
			dac[i] = 0;
		}

		if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_COMPOSITE && dac[i] == 1) {
			ret |= DISP_TV_CVBS;
		}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_Y && dac[i] == 1) {
			ret |= DISP_TV_YPBPR;
		}	else if(gdisp.screen[screen_id].dac_source[i] == DISP_TV_DAC_SRC_LUMA && dac[i] == 1)	{
			ret |= DISP_TV_SVIDEO;
		}
	}

	return  ret;
}



__s32 bsp_disp_tv_get_dac_status(__u32 screen_id, __u32 index)
{
	return TVE_get_dac_status(index);
}

__s32 bsp_disp_tv_set_dac_source(__u32 screen_id, __u32 index, __disp_tv_dac_source source)
{
	gdisp.screen[screen_id].dac_source[index] = source;

	if(gdisp.screen[screen_id].status & TV_ON) {
		Disp_TVEC_DacCfg(screen_id, gdisp.screen[screen_id].tv_mode);
	}

	return  0;
}

__s32 bsp_disp_tv_get_dac_source(__u32 screen_id, __u32 index)
{
	return (__s32)gdisp.screen[screen_id].dac_source[index];
}

__s32 bsp_disp_tv_auto_check_enable(__u32 screen_id)
{
	TVE_dac_autocheck_enable(screen_id, 0);
	TVE_dac_autocheck_enable(screen_id, 1);
	TVE_dac_autocheck_enable(screen_id, 2);
	TVE_dac_autocheck_enable(screen_id, 3);

	return DIS_SUCCESS;
}


__s32 bsp_disp_tv_auto_check_disable(__u32 screen_id)
{
	TVE_dac_autocheck_disable(screen_id, 0);
	TVE_dac_autocheck_disable(screen_id, 1);
	TVE_dac_autocheck_disable(screen_id, 2);
	TVE_dac_autocheck_disable(screen_id, 3);

	return DIS_SUCCESS;
}

__s32 bsp_disp_tv_set_src(__u32 screen_id, __disp_lcdc_src_t src)
{
	switch (src) {
	case DISP_LCDC_SRC_DE_CH1:
		tcon1_src_select(screen_id, LCD_SRC_BE0);
		break;

	case DISP_LCDC_SRC_DE_CH2:
		tcon1_src_select(screen_id, LCD_SRC_BE1);
		break;

	case DISP_LCDC_SRC_BLUE:
		tcon1_src_select(screen_id, LCD_SRC_BLUE);
		break;

	default:
		DE_WRN("not supported lcdc src:%d in bsp_disp_tv_set_src\n", src);
		return DIS_NOT_SUPPORT;
	}
	return DIS_SUCCESS;
}


__s32 bsp_disp_restore_tvec_reg(__u32 screen_id)
{
	TVE_init(screen_id);

	return 0;
}

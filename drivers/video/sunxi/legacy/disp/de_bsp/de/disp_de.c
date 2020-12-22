#include "disp_de.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_scaler.h"
#include "disp_clk.h"
#include "disp_lcd.h"

__s32 Image_init(__u32 screen_id)
{
	image_clk_init(screen_id);
	/* when access image registers, must open MODULE CLOCK of image */
	image_clk_on(screen_id, 0);
	DE_BE_Reg_Init(screen_id);

	Image_open(screen_id);
	DE_BE_EnableINT(screen_id, DE_IMG_REG_LOAD_FINISH);
	DE_BE_reg_auto_load_en(screen_id, 0);

	if(screen_id == 0) {
		OSAL_RegISR(gdisp.init_para.irq[DISP_MOD_BE0],0,scaler_event_proc, (void *)screen_id,0,0);
#ifndef __LINUX_OSAL__
		OSAL_InterruptEnable(gdisp.init_para.irq[DISP_MOD_BE0]);
#endif
	}	else if(screen_id == 1) {
		OSAL_RegISR(gdisp.init_para.irq[DISP_MOD_BE1],0,scaler_event_proc, (void *)screen_id,0,0);
#ifndef __LINUX_OSAL__
		OSAL_InterruptEnable(gdisp.init_para.irq[DISP_MOD_BE1]);
#endif
	}
	return DIS_SUCCESS;
}

__s32 Image_exit(__u32 screen_id)
{
	DE_BE_DisableINT(screen_id, DE_IMG_REG_LOAD_FINISH);
	//bsp_disp_sprite_exit(screen_id);
	image_clk_exit(screen_id);

	return DIS_SUCCESS;
}

__s32 Image_open(__u32  screen_id)
{
	DE_BE_Enable(screen_id);

	return DIS_SUCCESS;
}


__s32 Image_close(__u32 screen_id)
{
	DE_BE_Disable(screen_id);

	gdisp.screen[screen_id].status &= IMAGE_USED_MASK;

	return DIS_SUCCESS;
}


__s32 bsp_disp_set_bright(__u32 screen_id, __u32 bright)
{
	gdisp.screen[screen_id].bright = bright;
	bsp_disp_set_output_csc(screen_id, gdisp.screen[screen_id].output_csc_type, bsp_disp_drc_get_input_csc(screen_id));

	return DIS_SUCCESS;
}

__s32 bsp_disp_get_bright(__u32 screen_id)
{
	return gdisp.screen[screen_id].bright;
}

__s32 bsp_disp_set_contrast(__u32 screen_id, __u32 contrast)
{
	gdisp.screen[screen_id].contrast = contrast;
	bsp_disp_set_output_csc(screen_id, gdisp.screen[screen_id].output_csc_type, bsp_disp_drc_get_input_csc(screen_id));

	return DIS_SUCCESS;
}

__s32 bsp_disp_get_contrast(__u32 screen_id)
{
	return gdisp.screen[screen_id].contrast;
}

__s32 bsp_disp_set_saturation(__u32 screen_id, __u32 saturation)
{
	gdisp.screen[screen_id].saturation = saturation;
	bsp_disp_set_output_csc(screen_id, gdisp.screen[screen_id].output_csc_type, bsp_disp_drc_get_input_csc(screen_id));

	return DIS_SUCCESS;
}

__s32 bsp_disp_get_saturation(__u32 screen_id)
{
	return gdisp.screen[screen_id].saturation;
}

__s32 bsp_disp_set_hue(__u32 screen_id, __u32 hue)
{
	gdisp.screen[screen_id].hue = hue;
	bsp_disp_set_output_csc(screen_id, gdisp.screen[screen_id].output_csc_type, bsp_disp_drc_get_input_csc(screen_id));

	return DIS_SUCCESS;
}

__s32 bsp_disp_get_hue(__u32 screen_id)
{
	return gdisp.screen[screen_id].hue;
}

__s32 bsp_disp_enhance_enable(__u32 screen_id, __bool enable)
{
	gdisp.screen[screen_id].enhance_en = enable;
	bsp_disp_set_output_csc(screen_id, gdisp.screen[screen_id].output_csc_type, bsp_disp_drc_get_input_csc(screen_id));

	return DIS_SUCCESS;
}

__s32 bsp_disp_get_enhance_enable(__u32 screen_id)
{
	return gdisp.screen[screen_id].enhance_en;
}


__s32 bsp_disp_set_screen_size(__u32 screen_id, __disp_rectsz_t * size)
{
	DE_BE_set_display_size(screen_id, size->width, size->height);

	gdisp.screen[screen_id].screen_width = size->width;
	gdisp.screen[screen_id].screen_height= size->height;

	return DIS_SUCCESS;
}

__s32 bsp_disp_set_output_csc(__u32 screen_id, __disp_out_csc_type_t out_type, __u32 drc_csc)
{
	__disp_color_range_t out_color_range = DISP_COLOR_RANGE_0_255;
	__u32 out_csc = 0;
	__u32 enhance_en, bright, contrast, saturation, hue;

	enhance_en = gdisp.screen[screen_id].enhance_en;
	bright = gdisp.screen[screen_id].bright;
	contrast = gdisp.screen[screen_id].contrast;
	saturation = gdisp.screen[screen_id].saturation;
	hue = gdisp.screen[screen_id].hue;

	if(out_type == DISP_OUT_CSC_TYPE_HDMI_YUV) {
		__s32 ret = 0;
		__s32 value = 0;

		out_color_range = DISP_COLOR_RANGE_16_255;

		ret = OSAL_Script_FetchParser_Data("disp_init", "screen0_out_color_range", &value, 1);
		if(ret < 0) {
			DE_INF("fetch script data disp_init.screen0_out_color_range fail\n");
		}	else {
			out_color_range = value;
			DE_INF("screen0_out_color_range = %d\n", value);
		}
		out_csc = 2;
	} else if(out_type == DISP_OUT_CSC_TYPE_HDMI_RGB) {
		__s32 ret = 0;
		__s32 value = 0;

		out_color_range = DISP_COLOR_RANGE_16_255;

		ret = OSAL_Script_FetchParser_Data("disp_init", "screen0_out_color_range", &value, 1);
		if(ret < 0)	{
			DE_INF("fetch script data disp_init.screen0_out_color_range fail\n");
		}	else {
			out_color_range = value;
			DE_INF("screen0_out_color_range = %d\n", value);
		}
		out_csc = 0;
	}	else if(out_type == DISP_OUT_CSC_TYPE_TV) {
		out_csc = 1;
	}	else if(out_type == DISP_OUT_CSC_TYPE_LCD) {
		if(enhance_en == 0)
		{
		enhance_en = 1;
		bright = 50;//gdisp.screen[screen_id].lcd_cfg.lcd_bright;
		contrast = 50;//gdisp.screen[screen_id].lcd_cfg.lcd_contrast;
		saturation = 57;//gdisp.screen[screen_id].lcd_cfg.lcd_saturation;
		hue = 50;//gdisp.screen[screen_id].lcd_cfg.lcd_hue;
		}
		out_csc = 0;
	}

	/* if drc csc eq 1, indicate yuv for drc */
	if(drc_csc) {
		out_csc = 3;
	}

	gdisp.screen[screen_id].out_color_range = out_color_range;
	gdisp.screen[screen_id].out_csc = out_csc;

	DE_BE_Set_Enhance_ex(screen_id, gdisp.screen[screen_id].out_csc, gdisp.screen[screen_id].out_color_range, enhance_en, bright, contrast, saturation, hue);

	return DIS_SUCCESS;
}

__s32 bsp_disp_de_flicker_enable(__u32 screen_id, __bool b_en)
{
	if(b_en) {
		gdisp.screen[screen_id].de_flicker_status |= DE_FLICKER_REQUIRED;
	}	else {
		gdisp.screen[screen_id].de_flicker_status &= DE_FLICKER_REQUIRED_MASK;
	}
	Disp_set_out_interlace(screen_id);
	return DIS_SUCCESS;
}

__s32 Disp_set_out_interlace(__u32 screen_id)
{
	__u32 i;
	__bool b_cvbs_out = 0;
	__u32 num_scalers;

	num_scalers = bsp_disp_feat_get_num_scalers();

	if(gdisp.screen[screen_id].output_type==DISP_OUTPUT_TYPE_TV
	    && (gdisp.screen[screen_id].tv_mode==DISP_TV_MOD_PAL || gdisp.screen[screen_id].tv_mode==DISP_TV_MOD_PAL_M
	    ||	gdisp.screen[screen_id].tv_mode==DISP_TV_MOD_PAL_NC || gdisp.screen[screen_id].tv_mode==DISP_TV_MOD_NTSC)) {
		b_cvbs_out = 1;
	}

	gdisp.screen[screen_id].de_flicker_status |= DE_FLICKER_REQUIRED;

	bsp_disp_cfg_start(screen_id);

	/* when output device is cvbs */
	if((gdisp.screen[screen_id].de_flicker_status & DE_FLICKER_REQUIRED) && b_cvbs_out)	{
		DE_BE_deflicker_enable(screen_id, TRUE);
		for(i=0; i<num_scalers; i++) {
			if((gdisp.scaler[i].status & SCALER_USED) && (gdisp.scaler[i].screen_index == screen_id))	{
				Scaler_Set_Outitl(i, FALSE);
				gdisp.scaler[i].b_reg_change = TRUE;
			}
		}
		gdisp.screen[screen_id].de_flicker_status |= DE_FLICKER_USED;
	}	else {
		DE_BE_deflicker_enable(screen_id, FALSE);
		for(i=0; i<num_scalers; i++) {
			if((gdisp.scaler[i].status & SCALER_USED) && (gdisp.scaler[i].screen_index == screen_id))	{
				Scaler_Set_Outitl(i, gdisp.screen[screen_id].b_out_interlace);
				gdisp.scaler[i].b_reg_change = TRUE;
			}
		}
		gdisp.screen[screen_id].de_flicker_status &= DE_FLICKER_USED_MASK;
	}
	DE_BE_Set_Outitl_enable(screen_id, gdisp.screen[screen_id].b_out_interlace);

	bsp_disp_cfg_finish(screen_id);

	return DIS_SUCCESS;
}


__s32 bsp_disp_store_image_reg(__u32 screen_id, __u32 addr)
{
	__u32 i = 0;
	__u32 value = 0;
	__u32 reg_base = 0;

	if(addr == 0) {
		DE_WRN("bsp_disp_store_image_reg, addr is NULL!");
		return -1;
	}

	if(screen_id == 0) {
		reg_base = gdisp.init_para.reg_base[DISP_MOD_BE0];
	}	else {
		reg_base = gdisp.init_para.reg_base[DISP_MOD_BE1];
	}

	for(i=0; i<0xe00 - 0x800; i+=4)	{
		value = sys_get_wvalue(reg_base + 0x800 + i);
		sys_put_wvalue(addr + i, value);
	}

	return 0;
}

__s32 bsp_disp_restore_image_reg(__u32 screen_id, __u32 addr)
{
	__u32 i = 0;
	__u32 value = 0;
	__u32 reg_base = 0;

	if(addr == 0) {
		DE_WRN("bsp_disp_restore_image_reg, addr is NULL!");
		return -1;
	}

	if(screen_id == 0) {
		reg_base = gdisp.init_para.reg_base[DISP_MOD_BE0];
	}	else {
		reg_base = gdisp.init_para.reg_base[DISP_MOD_BE1];
	}

	DE_BE_Reg_Init(screen_id);
	for(i=4; i<0xe00 - 0x800; i+=4) {
		value = sys_get_wvalue(addr + i);
		sys_put_wvalue(reg_base + 0x800 + i,value);
	}

	value = sys_get_wvalue(addr);
	sys_put_wvalue(reg_base + 0x800,value);

	return 0;
}


__s32 bsp_disp_image_resume(__u32 screen_id)
{
	disp_layer_resmue(screen_id);

	return 0;
}

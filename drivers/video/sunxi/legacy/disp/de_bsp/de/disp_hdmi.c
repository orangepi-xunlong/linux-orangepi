#include "disp_hdmi.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_de.h"
#include "disp_tv.h"
#include "disp_lcd.h"
#include "disp_clk.h"


__s32 disp_hdmi_init(void)
{
	__s32 ret;
	__u32 value;
	__u32 num_screens;
	__u32 screen_id;

	num_screens = bsp_disp_feat_get_num_screens();
	ret = OSAL_Script_FetchParser_Data("hdmi_para", "hdmi_used", &value, 1);
	if(ret == 0) {
		if(value) {
			ret = OSAL_Script_FetchParser_Data("hdmi_para", "hdmi_cts_compatibility", &value, 1);
			if(ret < 0) {
				DE_INF("disp_init.hdmi_cts_compatibility not exist\n");
			}	else {
				DE_INF("disp_init.hdmi_cts_compatibility = %d\n", value);
				gdisp.init_para.hdmi_cts_compatibility = value;
			}
			hdmi_clk_init();
			hdmi_clk_on();
		}

		for(screen_id=0; screen_id<num_screens; screen_id++)
		{
			gdisp.screen[screen_id].hdmi_mode = DISP_TV_MOD_720P_50HZ;
			gdisp.screen[screen_id].hdmi_test_mode = 0xff;
			gdisp.screen[screen_id].hdmi_used = value;
		}
	}

	return DIS_SUCCESS;
}

__s32 disp_hdmi_exit(void)
{
	if(gdisp.screen[0].hdmi_used || gdisp.screen[1].hdmi_used) {
		hdmi_clk_exit();
	}

	return DIS_SUCCESS;
}

__s32 bsp_disp_hdmi_open(__u32 screen_id)
{
	if(disp_hdmi_get_support(screen_id) && (gdisp.screen[screen_id].hdmi_used)
	    && (!(gdisp.screen[screen_id].status & HDMI_ON))) {
		__disp_tv_mode_t     tv_mod;

		tv_mod = gdisp.screen[screen_id].hdmi_mode;

		hdmi_clk_on();
		lcdc_clk_on(screen_id, 1, 0);
		lcdc_clk_on(screen_id, 1, 1);
		drc_clk_open(screen_id,0);
		drc_clk_open(screen_id,1);
		tcon_init(screen_id);
		image_clk_on(screen_id, 1);
		/* set image normal channel start bit , because every de_clk_off( )will reset this bit */
		Image_open(screen_id);
		disp_clk_cfg(screen_id,DISP_OUTPUT_TYPE_HDMI, tv_mod);

		//bsp_disp_set_output_csc(screen_id, DISP_OUTPUT_TYPE_HDMI, gdisp.screen[screen_id].iep_status&DRC_USED);
		if(gdisp.init_para.hdmi_cts_compatibility == 0)	{
			DE_INF("bsp_disp_hdmi_open: disable dvi mode\n");
			bsp_disp_hdmi_dvi_enable(screen_id, 0);
		}	else if(gdisp.init_para.hdmi_cts_compatibility == 1) {
			DE_INF("bsp_disp_hdmi_open: enable dvi mode\n");
			bsp_disp_hdmi_dvi_enable(screen_id, 1);
		}	else {
			bsp_disp_hdmi_dvi_enable(screen_id, bsp_disp_hdmi_dvi_support(screen_id));
		}

		if(BSP_dsip_hdmi_get_input_csc(screen_id) == 0) {
			__inf("bsp_disp_hdmi_open:   hdmi output rgb\n");
			gdisp.screen[screen_id].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_RGB;
			bsp_disp_set_output_csc(screen_id, gdisp.screen[screen_id].output_csc_type, bsp_disp_drc_get_input_csc(screen_id));
		} else {
			__inf("bsp_disp_hdmi_open:   hdmi output yuv\n");
			gdisp.screen[screen_id].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_YUV;//default  yuv
			bsp_disp_set_output_csc(screen_id, gdisp.screen[screen_id].output_csc_type, bsp_disp_drc_get_input_csc(screen_id));
		}
		DE_BE_set_display_size(screen_id, tv_mode_to_width(tv_mod), tv_mode_to_height(tv_mod));
		DE_BE_Output_Select(screen_id, screen_id);
		bsp_disp_hdmi_set_src(screen_id, DISP_LCDC_SRC_DE_CH1);

		tcon1_set_hdmi_mode(screen_id,tv_mod);
		tcon1_open(screen_id);
		if(gdisp.init_para.hdmi_open)	{
			gdisp.init_para.hdmi_open();
		}	else {
			DE_WRN("Hdmi_open is NULL\n");
			return -1;
		}

		Disp_Switch_Dram_Mode(DISP_OUTPUT_TYPE_HDMI, tv_mod);

		gdisp.screen[screen_id].b_out_interlace = disp_get_screen_scan_mode(tv_mod);
		gdisp.screen[screen_id].status |= HDMI_ON;
		gdisp.screen[screen_id].lcdc_status |= LCDC_TCON1_USED;
		gdisp.screen[screen_id].output_type = DISP_OUTPUT_TYPE_HDMI;

		if(bsp_disp_cmu_get_enable(screen_id) ==1) {
			IEP_CMU_Set_Imgsize(screen_id, bsp_disp_get_screen_width(screen_id), bsp_disp_get_screen_height(screen_id));
		}

		Disp_set_out_interlace(screen_id);
#ifdef __LINUX_OSAL__
		Display_set_fb_timming(screen_id);
#endif
		return DIS_SUCCESS;
	}

	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_hdmi_close(__u32 screen_id)
{
	if(disp_hdmi_get_support(screen_id) && (gdisp.screen[screen_id].hdmi_used)
	    && (gdisp.screen[screen_id].status & HDMI_ON)) {
		if(gdisp.init_para.hdmi_close) {
			gdisp.init_para.hdmi_close();
		}	else {
			DE_WRN("Hdmi_close is NULL\n");
			return -1;
		}
		Image_close(screen_id);
		tcon1_close(screen_id);

		image_clk_off(screen_id, 1);
		lcdc_clk_off(screen_id);
		drc_clk_close(screen_id,0);
		drc_clk_close(screen_id,1);
		hdmi_clk_off();

		gdisp.screen[screen_id].b_out_interlace = 0;
		gdisp.screen[screen_id].lcdc_status &= LCDC_TCON1_USED_MASK;
		gdisp.screen[screen_id].status &= HDMI_OFF;
		gdisp.screen[screen_id].output_type = DISP_OUTPUT_TYPE_NONE;
		gdisp.screen[screen_id].pll_use_status &= ((gdisp.screen[screen_id].pll_use_status == VIDEO_PLL0_USED)? VIDEO_PLL0_USED_MASK : VIDEO_PLL1_USED_MASK);

		Disp_set_out_interlace(screen_id);

		return DIS_SUCCESS;
	}

	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_hdmi_set_mode(__u32 screen_id, __disp_tv_mode_t  mode)
{
	if(mode >= DISP_TV_MODE_NUM) {
		DE_WRN("unsupported hdmi mode:%d in bsp_disp_hdmi_set_mode\n", mode);
		return DIS_FAIL;
	}

	if(gdisp.screen[screen_id].hdmi_used) {
		if(gdisp.init_para.hdmi_set_mode)	{
			gdisp.init_para.hdmi_set_mode(mode);
		}	else	{
			DE_WRN("hdmi_set_mode is NULL\n");
			return -1;
		}

		gdisp.screen[screen_id].hdmi_mode = mode;
		gdisp.screen[screen_id].output_type = DISP_OUTPUT_TYPE_HDMI;

		return DIS_SUCCESS;
	}

	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_hdmi_get_mode(__u32 screen_id)
{
	return gdisp.screen[screen_id].hdmi_mode;
}

__s32 bsp_disp_hdmi_check_support_mode(__u32 screen_id, __u8  mode)
{
	if(disp_hdmi_get_support(screen_id) && (gdisp.screen[screen_id].hdmi_used)
	    && (gdisp.init_para.hdmi_mode_support)) {
		if(gdisp.screen[screen_id].hdmi_test_mode < DISP_TV_MODE_NUM)	{
			if(mode == gdisp.screen[screen_id].hdmi_test_mode) {
				return 1;
			} else {
				return 0;
			}
		} else {
			return gdisp.init_para.hdmi_mode_support(mode);
		}
	}	else {
		DE_WRN("hdmi_mode_support is NULL\n");
		return DIS_NOT_SUPPORT;
	}
}

__s32 bsp_disp_hdmi_get_hpd_status(__u32 screen_id)
{
	if(disp_hdmi_get_support(screen_id) && (gdisp.screen[screen_id].hdmi_used)
	    && (gdisp.init_para.hdmi_get_HPD_status)) {
		return gdisp.init_para.hdmi_get_HPD_status();
	}	else {
		DE_WRN("hdmi_get_HPD_status is NULL\n");
		return -1;
	}
}

__s32 bsp_disp_hdmi_set_src(__u32 screen_id, __disp_lcdc_src_t src)
{
	if(disp_hdmi_get_support(screen_id) && (gdisp.screen[screen_id].hdmi_used)) {
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
	}
	return DIS_SUCCESS;
}

__s32 bsp_disp_hdmi_get_src(__u32 screen_id)
{
	if(disp_hdmi_get_support(screen_id) && (gdisp.screen[screen_id].hdmi_used)) {
		return tcon1_src_get(screen_id);
	}

	return DIS_SUCCESS;
}

__s32 bsp_disp_hdmi_dvi_enable(__u32 screen_id, __u32 enable)
{
	__s32 ret = -1;

	if(disp_hdmi_get_support(screen_id) && (gdisp.screen[screen_id].hdmi_used)
	    && (gdisp.init_para.hdmi_dvi_enable))	{
		ret = gdisp.init_para.hdmi_dvi_enable(enable);
		gdisp.screen[screen_id].dvi_enable = enable;
	}	else {
		DE_WRN("Hdmi_dvi_enable is NULL\n");
	}

	return ret;
}


__s32 bsp_disp_hdmi_dvi_support(__u32 screen_id)
{
	__s32 ret = -1;

	if(disp_hdmi_get_support(screen_id) && (gdisp.screen[screen_id].hdmi_used)
	    && (gdisp.init_para.hdmi_dvi_support)) {
		ret = gdisp.init_para.hdmi_dvi_support();
	}	else {
		DE_WRN("Hdmi_dvi_support is NULL\n");
	}

	return ret;
}

__s32 BSP_dsip_hdmi_get_input_csc(__u32 screen_id)
{
	__s32 ret = -1;

	if(disp_hdmi_get_support(screen_id) && (gdisp.screen[screen_id].hdmi_used)
	    && (gdisp.init_para.hmdi_get_input_csc)) {
		ret = gdisp.init_para.hmdi_get_input_csc();
	} else {
		DE_WRN("Hdmi_get_input_csc is NULL \n");
	}

	return ret;
}

__s32 bsp_disp_hdmi_suspend(void)
{
	if(gdisp.init_para.hdmi_suspend) {
		return gdisp.init_para.hdmi_suspend();
	}

	return -1;
}

__s32 bsp_disp_hdmi_resume(void)
{
	if(gdisp.init_para.hdmi_resume) {
		return gdisp.init_para.hdmi_resume();
	}

	return -1;
}

__s32 bsp_disp_hdmi_early_suspend(void)
{
	if(gdisp.init_para.hdmi_early_suspend) {
		return gdisp.init_para.hdmi_early_suspend();
	}

	return -1;
}

__s32 bsp_disp_hdmi_late_resume(void)
{
	if(gdisp.init_para.hdmi_late_resume) {
		return gdisp.init_para.hdmi_late_resume();
	}

	return -1;
}

__s32 bsp_disp_set_hdmi_func(__disp_hdmi_func * func)
{
	if((disp_hdmi_get_support(0) || disp_hdmi_get_support(1))
	    && (gdisp.screen[0].hdmi_used || gdisp.screen[1].hdmi_used)) {
		gdisp.init_para.hdmi_open = func->hdmi_open;
		gdisp.init_para.hdmi_close = func->hdmi_close;
		gdisp.init_para.hdmi_set_mode = func->hdmi_set_mode;
		gdisp.init_para.hdmi_mode_support = func->hdmi_mode_support;
		gdisp.init_para.hdmi_get_HPD_status = func->hdmi_get_HPD_status;
		gdisp.init_para.hdmi_set_pll = func->hdmi_set_pll;
		gdisp.init_para.hdmi_dvi_enable = func->hdmi_dvi_enable;
		gdisp.init_para.hdmi_dvi_support = func->hdmi_dvi_support;
		gdisp.init_para.hmdi_get_input_csc = func->hdmi_get_input_csc;
		gdisp.init_para.hdmi_suspend = func->hdmi_suspend;
		gdisp.init_para.hdmi_resume = func->hdmi_resume;
		gdisp.init_para.hdmi_early_suspend = func->hdmi_early_suspend;
		gdisp.init_para.hdmi_late_resume = func->hdmi_late_resume;

		gdisp.hdmi_registered = 1;
		if(gdisp.init_para.start_process) {
			gdisp.init_para.start_process();
		}

		return DIS_SUCCESS;
	}

	return -1;
}
//hpd: 0 plugout;  1 plugin
__s32 bsp_disp_set_hdmi_hpd(__u32 hpd)
{
	if(hpd == 1) {
		gdisp.screen[0].hdmi_hpd = 1;
		gdisp.screen[1].hdmi_hpd = 1;
		if(gdisp.screen[0].status & HDMI_ON) {
			if(BSP_dsip_hdmi_get_input_csc(0) == 0)	{
				__inf("bsp_disp_set_hdmi_hpd:   hdmi output rgb\n");
				gdisp.screen[0].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_RGB;
				bsp_disp_set_output_csc(0, gdisp.screen[0].output_csc_type, bsp_disp_drc_get_input_csc(0));
			} else {
				__inf("bsp_disp_set_hdmi_hpd:   hdmi output yuv\n");//default  yuv
				gdisp.screen[0].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_YUV;
				bsp_disp_set_output_csc(0, gdisp.screen[0].output_csc_type, bsp_disp_drc_get_input_csc(0));
			}
		} else if(gdisp.screen[1].status & HDMI_ON) {
			if(BSP_dsip_hdmi_get_input_csc(1) == 0)	{
				__inf("bsp_disp_set_hdmi_hpd:   hdmi output rgb\n");
				gdisp.screen[1].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_RGB;
				bsp_disp_set_output_csc(1, gdisp.screen[1].output_csc_type, bsp_disp_drc_get_input_csc(1));
			} else {
				__inf("bsp_disp_set_hdmi_hpd:   hdmi output yuv\n");
				gdisp.screen[1].output_csc_type = DISP_OUT_CSC_TYPE_HDMI_YUV;//default  yuv
				bsp_disp_set_output_csc(1, gdisp.screen[1].output_csc_type,  bsp_disp_drc_get_input_csc(1));
			}
		}
	}	else {
		gdisp.screen[0].hdmi_hpd = 0;
		gdisp.screen[1].hdmi_hpd = 0;
	}

	return 0;
}

__s32 bsp_disp_hdmi_cts_enable(__u32 en)
{
	gdisp.init_para.hdmi_cts_compatibility = en;

	return 0;
}


__s32 bsp_disp_hdmi_get_cts_enable(void)
{
	return gdisp.init_para.hdmi_cts_compatibility;
}


__s32 bsp_disp_hdmi_set_test_mode(__u32 screen_id, __u32 mode)
{
	gdisp.screen[screen_id].hdmi_test_mode = mode;

	return 0;
}

__s32 bsp_disp_hdmi_get_test_mode(__u32 screen_id)
{
	return gdisp.screen[screen_id].hdmi_test_mode;
}

__s32 disp_hdmi_get_support(__u32 screen_id)
{
	return (bsp_disp_feat_get_supported_output_types(screen_id) & DISP_OUTPUT_TYPE_HDMI);
}

__s32 bsp_disp_hdmi_get_registered(void)
{
	return gdisp.hdmi_registered;
}


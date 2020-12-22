
#include "disp_scaler.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_layer.h"
#include "disp_clk.h"
#include "disp_lcd.h"
#include "disp_de.h"


// 0:scaler input pixel format
// 1:scaler input yuv mode
// 2:scaler input pixel sequence
// 3:scaler output format
__s32  Scaler_sw_para_to_reg(__u8 type, __u8 mode, __u8 format, __u8 seq)
{
	/* scaler input  pixel format */
	if(type == 0)	{
		if(format == DISP_FORMAT_YUV444) {
			return DE_SCAL_INYUV444;
		}	else if(format == DISP_FORMAT_YUV420)	{
			return DE_SCAL_INYUV420;
		}	else if(format == DISP_FORMAT_YUV422)	{
			return DE_SCAL_INYUV422;
		}	else if(format == DISP_FORMAT_YUV411)	{
			return DE_SCAL_INYUV411;
		}	else if(format == DISP_FORMAT_CSIRGB)	{
			return DE_SCAL_INRGB565;
		}	else if(format == DISP_FORMAT_ARGB8888)	{
			return DE_SCAL_INRGB888;
		}	else if(format == DISP_FORMAT_RGB888)	{
			return DE_SCAL_INRGB888;
		}	else if(format == DISP_FORMAT_RGB565)	{
			return DE_SCAL_INRGB565;
		}	else if(format == DISP_FORMAT_ARGB4444)	{
			return DE_SCAL_INRGB4444;
		}	else if(format == DISP_FORMAT_ARGB1555)	{
			return DE_SCAL_INRGB1555;
		}	else {
			DE_INF("not supported scaler input pixel format:%d in Scaler_sw_para_to_reg\n",format);
		}
	}
	/* scaler input mode */
	else if(type == 1) {
		if(mode == DISP_MOD_INTERLEAVED) {
			return DE_SCAL_INTERLEAVED;
		}	else if(mode == DISP_MOD_MB_PLANAR)	{
			return DE_SCAL_PLANNARMB;
		}	else if(mode == DISP_MOD_NON_MB_PLANAR)	{
			return DE_SCAL_PLANNAR;
		}	else if(mode == DISP_MOD_NON_MB_UV_COMBINED) {
			return DE_SCAL_UVCOMBINED;
		}	else if(mode == DISP_MOD_MB_UV_COMBINED) {
			return DE_SCAL_UVCOMBINEDMB;
		}	else {
			DE_INF("not supported scaler input mode:%d in Scaler_sw_para_to_reg\n",mode);
		}
	}
	/* scaler input pixel sequence */
	else if(type == 2) {
		if(seq == DISP_SEQ_UYVY) {
			return DE_SCAL_UYVY;
		}	else if(seq == DISP_SEQ_YUYV) {
			return DE_SCAL_YUYV;
		}	else if(seq == DISP_SEQ_VYUY) {
			return DE_SCAL_VYUY;
		}	else if(seq == DISP_SEQ_YVYU) {
			return DE_SCAL_YVYU;
		}	else if(seq == DISP_SEQ_AYUV) {
			return DE_SCAL_AYUV;
		}	else if(seq == DISP_SEQ_UVUV) {
			return DE_SCAL_UVUV;
		}	else if(seq == DISP_SEQ_VUVU) {
			return DE_SCAL_VUVU;
		}	else if(seq == DISP_SEQ_ARGB) {
			if(format == DISP_FORMAT_ARGB8888) {
				return DE_SCAL_ARGB;
			}	else if(format == DISP_FORMAT_RGB565) {
				return DE_SCAL_RGB565;
			}	else if(format == DISP_FORMAT_ARGB1555) {
				return DE_SCAL_ARGB1555;
			}	else if(format == DISP_FORMAT_ARGB4444) {
				return DE_SCAL_ARGB4444;
			}
		}	else if(seq == DISP_SEQ_BGRA) {
			if(format == DISP_FORMAT_ARGB8888) {
				return DE_SCAL_BGRA;
			}	else if(format == DISP_FORMAT_RGB565) {
				return DE_SCAL_BGR565;
			}	else if(format == DISP_FORMAT_ARGB1555) {
				return DE_SCAL_BGRA5551;
			}	else if(format == DISP_FORMAT_ARGB4444) {
				return DE_SCAL_BGRA4444;
			}
		}	else if(seq == DISP_SEQ_P3210) {
			return 0;
		}	else if(seq == DISP_SEQ_P0123) {
			return 1;
		}	else {
			DE_INF("not supported scaler input pixel sequence:%d in Scaler_sw_para_to_reg\n",seq);
		}
	}
	/* scaler output value */
	else if(type == 3) {
		if(format == DISP_FORMAT_YUV444) {
			return DE_SCAL_OUTPYUV444;
		}	else if(format == DISP_FORMAT_YUV422) {
			return DE_SCAL_OUTPYUV422;
		}	else if(format == DISP_FORMAT_YUV420) {
			return DE_SCAL_OUTPYUV420;
		}	else if(format == DISP_FORMAT_YUV411) {
			return DE_SCAL_OUTPYUV411;
		}	else if(format == DISP_FORMAT_ARGB8888) {
			return DE_SCAL_OUTI1RGB888;
		}	else if(format == DISP_FORMAT_RGB888) {
			return DE_SCAL_OUTPRGB888;
		}	else {
			DE_INF("not supported scaler output value:%d in Scaler_sw_para_to_reg\n", format);
		}
	}
	DE_INF("not supported type:%d in Scaler_sw_para_to_reg\n", type);
	return DIS_FAIL;
}

// 0: 3d in mode
// 1: 3d out mode
__s32 Scaler_3d_sw_para_to_reg(__u32 type, __u32 mode, __bool b_out_interlace)
{
	if(type == 0) {
		switch (mode) {
		case DISP_3D_SRC_MODE_TB:
			return DE_SCAL_3DIN_TB;

		case DISP_3D_SRC_MODE_FP:
			return DE_SCAL_3DIN_FP;

		case DISP_3D_SRC_MODE_SSF:
			return DE_SCAL_3DIN_SSF;

		case DISP_3D_SRC_MODE_SSH:
			return DE_SCAL_3DIN_SSH;

		case DISP_3D_SRC_MODE_LI:
			return DE_SCAL_3DIN_LI;

		default:
			DE_INF("not supported 3d in mode:%d in Scaler_3d_sw_para_to_reg\n", mode);
			return DIS_FAIL;
		}
	}
	else if(type == 1) {
		switch (mode)	{
		case DISP_3D_OUT_MODE_CI_1:
			return DE_SCAL_3DOUT_CI_1;

		case DISP_3D_OUT_MODE_CI_2:
			return DE_SCAL_3DOUT_CI_2;

		case DISP_3D_OUT_MODE_CI_3:
			return DE_SCAL_3DOUT_CI_3;

		case DISP_3D_OUT_MODE_CI_4:
			return DE_SCAL_3DOUT_CI_4;

		case DISP_3D_OUT_MODE_LIRGB:
			return DE_SCAL_3DOUT_LIRGB;

		case DISP_3D_OUT_MODE_TB:
			return DE_SCAL_3DOUT_HDMI_TB;

		case DISP_3D_OUT_MODE_FP:
		{
			if(b_out_interlace == TRUE)	{
				return DE_SCAL_3DOUT_HDMI_FPI;
			}	else {
				return DE_SCAL_3DOUT_HDMI_FPP;
			}
		}

		case DISP_3D_OUT_MODE_SSF:
			return DE_SCAL_3DOUT_HDMI_SSF;

		case DISP_3D_OUT_MODE_SSH:
			return DE_SCAL_3DOUT_HDMI_SSH;

		case DISP_3D_OUT_MODE_LI:
			return DE_SCAL_3DOUT_HDMI_LI;

		case DISP_3D_OUT_MODE_FA:
			return DE_SCAL_3DOUT_HDMI_FA;

		default:
			DE_INF("not supported 3d output mode:%d in Scaler_3d_sw_para_to_reg\n", mode);
			return DIS_FAIL;
		}
	}

	return DIS_FAIL;
}

#ifdef __LINUX_OSAL__
__s32 scaler_event_proc(__s32 irq, void *parg)
#else
__s32 Scaler_event_proc(void *parg)
#endif
{
	__u8 fe_intflags = 0, be_intflags = 0;
	__u32 scaler_index = (__u32)parg;

	if(gdisp.scaler[scaler_index].status & SCALER_USED) {
		fe_intflags = DE_SCAL_QueryINT(scaler_index);
		DE_SCAL_ClearINT(scaler_index,fe_intflags);
	}
	be_intflags = DE_BE_QueryINT(scaler_index);
	DE_BE_ClearINT(scaler_index,be_intflags);

	//DE_INF("scaler %d interrupt, scal_int_status:0x%x!\n", scaler_index, fe_intflags);

	if(be_intflags & DE_IMG_REG_LOAD_FINISH) {
		LCD_line_event_proc(scaler_index);
	}

	if((gdisp.scaler[scaler_index].status & SCALER_USED) && (fe_intflags & DE_WB_END_IE)) {
		DE_SCAL_DisableINT(scaler_index,DE_FE_INTEN_ALL);
#if 0//def __LINUX_OSAL__
		if(gdisp.scaler[scaler_index].b_scaler_finished == 1 && (&gdisp.scaler[scaler_index].scaler_queue != NULL)) {
			gdisp.scaler[scaler_index].b_scaler_finished = 2;
			wake_up_interruptible(&(gdisp.scaler[scaler_index].scaler_queue));
		}	else {
			__wrn("not scaler %d begin in DRV_scaler_finish\n", scaler_index);
		}
#endif
	}

	return OSAL_IRQ_RETURN;
}

__s32 Scaler_Init(__u32 scaler_index)
{
	scaler_clk_init(scaler_index);
	DE_SCAL_DisableINT(scaler_index,DE_WB_END_IE);

	if(scaler_index == 0) {
		OSAL_RegISR(gdisp.init_para.irq[DISP_MOD_FE0],0,scaler_event_proc, (void *)scaler_index,0,0);
#ifndef __LINUX_OSAL__
		OSAL_InterruptEnable(gdisp.init_para.irq[DISP_MOD_FE0]);
#endif
	}	else if(scaler_index == 1) {
		OSAL_RegISR(gdisp.init_para.irq[DISP_MOD_FE1],0,scaler_event_proc, (void *)scaler_index,0,0);
#ifndef __LINUX_OSAL__
		OSAL_InterruptEnable(gdisp.init_para.irq[DISP_MOD_FE1]);
#endif
	}
	return DIS_SUCCESS;
}

__s32 Scaler_Exit(__u32 scaler_index)
{
	if(scaler_index == 0) {
		OSAL_InterruptDisable(gdisp.init_para.irq[DISP_MOD_FE0]);
		OSAL_UnRegISR(gdisp.init_para.irq[DISP_MOD_FE0],scaler_event_proc,(void*)scaler_index);
	} else if(scaler_index == 1) {
		OSAL_InterruptDisable(gdisp.init_para.irq[DISP_MOD_FE1]);
		OSAL_UnRegISR(gdisp.init_para.irq[DISP_MOD_FE1],scaler_event_proc,(void*)scaler_index);
	}

	DE_SCAL_DisableINT(scaler_index,DE_WB_END_IE);
	DE_SCAL_Reset(scaler_index);
	DE_SCAL_Disable(scaler_index);
	scaler_clk_off(scaler_index);
	return DIS_SUCCESS;
}

__s32 Scaler_open(__u32 scaler_index)
{
	DE_INF("scaler %d open\n", scaler_index);

	scaler_clk_on(scaler_index);
	deu_clk_open(scaler_index, 0);
	DE_SCAL_Reset(scaler_index);
	DE_SCAL_DisableINT(scaler_index,DE_WB_END_IE);
	DE_SCAL_Enable(scaler_index);

	return DIS_SUCCESS;
}

__s32 Scaler_close(__u32 scaler_index)
{
	DE_INF("scaler %d close\n", scaler_index);

	DE_SCAL_Reset(scaler_index);
	DE_SCAL_Disable(scaler_index);
	scaler_clk_off(scaler_index);
	deu_clk_close(scaler_index, 0);

	memset(&gdisp.scaler[scaler_index], 0, sizeof(__disp_scaler_t));
	gdisp.scaler[scaler_index].bright = 32;
	gdisp.scaler[scaler_index].contrast = 32;
	gdisp.scaler[scaler_index].saturation = 32;
	gdisp.scaler[scaler_index].hue = 32;
	gdisp.scaler[scaler_index].status &= SCALER_USED_MASK;

	return DIS_SUCCESS;
}

__s32 Scaler_Request(__u32 scaler_index)
{
	__s32 ret = DIS_NO_RES;
	__u32 num_scalers;
	__u32 i;

	num_scalers = bsp_disp_feat_get_num_scalers();

	DE_INF("Scaler_Request,%d\n", scaler_index);

	for(i=0; i<num_scalers; i++) {
		if(((scaler_index == i) || (scaler_index == 0xff))
		    && (!(gdisp.scaler[i].status & SCALER_USED)))	{
			ret = i;
			Scaler_open(ret);
			gdisp.scaler[ret].b_close = FALSE;
			gdisp.scaler[ret].status |= SCALER_USED;
			break;
		}	else if(((scaler_index == i) || (scaler_index == 0xff))
		    && (gdisp.scaler[i].b_close == TRUE))	{
			ret = i;
			gdisp.scaler[ret].b_close = FALSE;
			gdisp.scaler[ret].status |= SCALER_USED;
			break;
		}
	}

	return ret;
}


__s32 Scaler_Release(__u32 scaler_index, __bool b_display)
{
	__u32 screen_index = 0xff;
	__bool b_display_off = TRUE;

	DE_INF("Scaler_Release:%d\n", scaler_index);

	DE_SCAL_Set_Di_Ctrl(scaler_index, 0, 0, 0, 0);
	screen_index = gdisp.scaler[scaler_index].screen_index;

	if((screen_index != 0xff)
	    && (bsp_disp_get_output_type(screen_index) != DISP_OUTPUT_TYPE_NONE))	{
		b_display_off = FALSE;
	}
	if((b_display == FALSE) || b_display_off) {
		Scaler_close(scaler_index);
	}	else {
		gdisp.scaler[scaler_index].b_close = TRUE;
	}

	return DIS_SUCCESS;
}

__s32 Scaler_Set_Framebuffer(__u32 scaler_index, __disp_fb_t *pfb)//keep the source window
{
	__scal_buf_addr_t scal_addr;
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__scal_src_type_t in_type;
	__scal_out_type_t out_type;
	__scal_scan_mod_t in_scan;
	__scal_scan_mod_t out_scan;
	__disp_scaler_t * scaler;
	__u32 screen_index;
	__u32 cpu_sr;

	scaler = &(gdisp.scaler[scaler_index]);
	screen_index = gdisp.scaler[scaler_index].screen_index;

	OSAL_IrqLock(&cpu_sr);
	memcpy(&scaler->in_fb, pfb, sizeof(__disp_fb_t));
	OSAL_IrqUnLock(cpu_sr);

	in_type.fmt= Scaler_sw_para_to_reg(0,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.mod= Scaler_sw_para_to_reg(1,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.ps= Scaler_sw_para_to_reg(2,scaler->in_fb.mode, scaler->in_fb.format, (__u8)scaler->in_fb.seq);
	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	scal_addr.ch0_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[0]));
	scal_addr.ch1_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[1]));
	scal_addr.ch2_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[2]));

	in_size.src_width = scaler->in_fb.size.width;
	in_size.src_height = scaler->in_fb.size.height;
	in_size.x_off = scaler->src_win.x;
	in_size.y_off = scaler->src_win.y;
	in_size.scal_width = scaler->src_win.width;
	in_size.scal_height = scaler->src_win.height;

	out_type.byte_seq = scaler->out_fb.seq;
	out_type.fmt = scaler->out_fb.format;
	out_type.alpha_en = 1;
	out_type.alpha_coef_type = 0;

	out_size.width = scaler->out_size.width;
	out_size.height = scaler->out_size.height;

	in_scan.field = FALSE;
	in_scan.bottom = FALSE;

	out_scan.field = (gdisp.screen[screen_index].de_flicker_status & DE_FLICKER_USED)?FALSE: gdisp.screen[screen_index].b_out_interlace;

	if(scaler->in_fb.cs_mode > DISP_VXYCC) {
		scaler->in_fb.cs_mode = DISP_BT601;
	}

	if(scaler->in_fb.b_trd_src)	{
		__scal_3d_inmode_t inmode;
		__scal_3d_outmode_t outmode = 0;
		__scal_buf_addr_t scal_addr_right;

		inmode = Scaler_3d_sw_para_to_reg(0, scaler->in_fb.trd_mode, 0);
		outmode = Scaler_3d_sw_para_to_reg(1, scaler->out_trd_mode, gdisp.screen[screen_index].b_out_interlace);

		DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
		if(scaler->b_trd_out)	{
		  DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
		}

		scal_addr_right.ch0_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.trd_right_addr[0]));
		scal_addr_right.ch1_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.trd_right_addr[1]));
		scal_addr_right.ch2_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.trd_right_addr[2]));

		DE_SCAL_Set_3D_Ctrl(scaler_index, scaler->b_trd_out, inmode, outmode);
		DE_SCAL_Config_3D_Src(scaler_index, &scal_addr, &in_size, &in_type, inmode, &scal_addr_right);
		DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, scaler->b_trd_out, outmode);
	}	else {
		DE_SCAL_Config_Src(scaler_index,&scal_addr,&in_size,&in_type,FALSE,FALSE);
		DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, 0, 0);
	}
	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	if(scaler->enhance_en == TRUE) {
		Scaler_Set_Enhance(scaler_index, scaler->bright, scaler->contrast, scaler->saturation, scaler->hue);
	}	else {
		DE_SCAL_Set_CSC_Coef(scaler_index, scaler->in_fb.cs_mode, DISP_BT601, get_fb_type(scaler->in_fb.format), DISP_FB_TYPE_RGB, scaler->in_fb.br_swap, 0);
	}
	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, scaler->smooth_mode);
	return DIS_SUCCESS;
}

__s32 Scaler_Get_Framebuffer(__u32 scaler_index, __disp_fb_t *pfb)
{
	__disp_scaler_t * scaler;

	if(pfb==NULL)	{
		return  DIS_PARA_FAILED;
	}

	scaler = &(gdisp.scaler[scaler_index]);
	if(scaler->status & SCALER_USED) {
		memcpy(pfb,&scaler->in_fb, sizeof(__disp_fb_t));
	}	else {
		return DIS_PARA_FAILED;
	}

	return DIS_SUCCESS;
}

__s32 Scaler_Set_Output_Size(__u32 scaler_index, __disp_rectsz_t *size)
{
	__disp_scaler_t * scaler;
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__scal_src_type_t in_type;
	__scal_out_type_t out_type;
	__scal_scan_mod_t in_scan;
	__scal_scan_mod_t out_scan;
	__u32 screen_index;

	scaler = &(gdisp.scaler[scaler_index]);
	screen_index = gdisp.scaler[scaler_index].screen_index;

	scaler->out_size.height = size->height;
	scaler->out_size.width = size->width;

	in_type.fmt= Scaler_sw_para_to_reg(0,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.mod= Scaler_sw_para_to_reg(1,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.ps= Scaler_sw_para_to_reg(2,scaler->in_fb.mode, scaler->in_fb.format, (__u8)scaler->in_fb.seq);
	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	in_size.src_width = scaler->src_win.width;
	in_size.src_height = scaler->in_fb.size.height;
	in_size.x_off = scaler->src_win.x;
	in_size.y_off = scaler->src_win.y;
	in_size.scal_height= scaler->src_win.height;
	in_size.scal_width= scaler->src_win.width;

	out_type.byte_seq = scaler->out_fb.seq;
	out_type.fmt = scaler->out_fb.format;
	out_type.alpha_en = 1;
	out_type.alpha_coef_type = 0;

	out_size.width = scaler->out_size.width;
	out_size.height = scaler->out_size.height;

	in_scan.field = FALSE;
	in_scan.bottom = FALSE;

	out_scan.field = (gdisp.screen[screen_index].de_flicker_status
	    & DE_FLICKER_USED)?FALSE: gdisp.screen[screen_index].b_out_interlace;

	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	if(scaler->enhance_en == TRUE) {
		Scaler_Set_Enhance(scaler_index, scaler->bright, scaler->contrast, scaler->saturation, scaler->hue);
	}	else {
		DE_SCAL_Set_CSC_Coef(scaler_index, scaler->in_fb.cs_mode, DISP_BT601, get_fb_type(scaler->in_fb.format), DISP_FB_TYPE_RGB, scaler->in_fb.br_swap, 0);
	}
	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, scaler->smooth_mode);
	DE_SCAL_Set_Out_Size(scaler_index, &out_scan, &out_type, &out_size);

	return DIS_SUCCESS;
}

__s32 Scaler_Set_SclRegn(__u32 scaler_index, __disp_rect_t *scl_rect)
{
	__disp_scaler_t * scaler;
	__scal_buf_addr_t scal_addr;
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__scal_src_type_t in_type;
	__scal_out_type_t out_type;
	__scal_scan_mod_t in_scan;
	__scal_scan_mod_t out_scan;
	__u32 screen_index;

	scaler = &(gdisp.scaler[scaler_index]);
	screen_index = gdisp.scaler[scaler_index].screen_index;

	scaler->src_win.x         = scl_rect->x;
	scaler->src_win.y         = scl_rect->y;
	scaler->src_win.height    = scl_rect->height;
	scaler->src_win.width     = scl_rect->width;

	in_type.fmt= Scaler_sw_para_to_reg(0,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.mod= Scaler_sw_para_to_reg(1,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.ps= Scaler_sw_para_to_reg(2,scaler->in_fb.mode, scaler->in_fb.format, (__u8)scaler->in_fb.seq);
	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	scal_addr.ch0_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[0]));
	scal_addr.ch1_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[1]));
	scal_addr.ch2_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[2]));

	in_size.src_width = scaler->in_fb.size.width;
	in_size.src_height = scaler->in_fb.size.height;
	in_size.x_off = scaler->src_win.x;
	in_size.y_off = scaler->src_win.y;
	in_size.scal_width = scaler->src_win.width;
	in_size.scal_height = scaler->src_win.height;

	out_type.byte_seq = scaler->out_fb.seq;
	out_type.fmt = scaler->out_fb.format;
	out_type.alpha_en = 1;
	out_type.alpha_coef_type = 0;

	out_size.width = scaler->out_size.width;
	out_size.height = scaler->out_size.height;

	in_scan.field = FALSE;
	in_scan.bottom = FALSE;

	out_scan.field = (gdisp.screen[screen_index].de_flicker_status & DE_FLICKER_USED)?FALSE: gdisp.screen[screen_index].b_out_interlace;

	if(scaler->in_fb.cs_mode > DISP_VXYCC) {
		scaler->in_fb.cs_mode = DISP_BT601;
	}

	if(scaler->in_fb.b_trd_src) {
		__scal_3d_inmode_t inmode;
		__scal_3d_outmode_t outmode = 0;
		__scal_buf_addr_t scal_addr_right;

		inmode = Scaler_3d_sw_para_to_reg(0, scaler->in_fb.trd_mode, 0);
		outmode = Scaler_3d_sw_para_to_reg(1, scaler->out_trd_mode, gdisp.screen[screen_index].b_out_interlace);

		DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
		if(scaler->b_trd_out)	{
			DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
		}

		scal_addr_right.ch0_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.trd_right_addr[0]));
		scal_addr_right.ch1_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.trd_right_addr[1]));
		scal_addr_right.ch2_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.trd_right_addr[2]));

		DE_SCAL_Set_3D_Ctrl(scaler_index, scaler->b_trd_out, inmode, outmode);
		DE_SCAL_Config_3D_Src(scaler_index, &scal_addr, &in_size, &in_type, inmode, &scal_addr_right);
		DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, scaler->b_trd_out, outmode);
	}	else {
		DE_SCAL_Config_Src(scaler_index,&scal_addr,&in_size,&in_type,FALSE,FALSE);
		DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, 0, 0);
	}
	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, scaler->smooth_mode);

	return DIS_SUCCESS;
}


__s32 Scaler_Get_SclRegn(__u32 scaler_index, __disp_rect_t *scl_rect)
{
	__disp_scaler_t * scaler;

	if(scl_rect == NULL) {
		return  DIS_PARA_FAILED;
	}

	scaler = &(gdisp.scaler[scaler_index]);
	if(scaler->status & SCALER_USED) {
		scl_rect->x = scaler->src_win.x;
		scl_rect->y = scaler->src_win.y;
		scl_rect->width = scaler->src_win.width;
		scl_rect->height = scaler->src_win.height;
	}	else {
		return DIS_PARA_FAILED;
	}

	return DIS_SUCCESS;
}

__s32 Scaler_Set_Para(__u32 scaler_index, __disp_scaler_t *scl)
{
	__disp_scaler_t * scaler;
	__scal_buf_addr_t scal_addr;
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__scal_src_type_t in_type;
	__scal_out_type_t out_type;
	__scal_scan_mod_t in_scan;
	__scal_scan_mod_t out_scan;
	__u32 screen_index;


	scaler = &(gdisp.scaler[scaler_index]);
	screen_index = gdisp.scaler[scaler_index].screen_index;

	memcpy(&(scaler->in_fb), &(scl->in_fb), sizeof(__disp_fb_t));
	memcpy(&(scaler->src_win), &(scl->src_win), sizeof(__disp_rect_t));
	memcpy(&(scaler->out_size), &(scl->out_size), sizeof(__disp_rectsz_t));

	in_type.fmt= Scaler_sw_para_to_reg(0,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.mod= Scaler_sw_para_to_reg(1,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.ps= Scaler_sw_para_to_reg(2,scaler->in_fb.mode, scaler->in_fb.format, (__u8)scaler->in_fb.seq);
	if(((__s32)in_type.fmt == DIS_FAIL) || ((__s32)in_type.mod== DIS_FAIL)
	    || ((__s32)in_type.ps == DIS_FAIL))	{
		DE_WRN("not supported scaler input pixel format: mode=%d,fmt=%d,ps=%d\n", scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	}

	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	scal_addr.ch0_addr = (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[0]));
	scal_addr.ch1_addr = (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[1]));
	scal_addr.ch2_addr = (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.addr[2]));

	in_size.src_width = scaler->in_fb.size.width;
	in_size.src_height = scaler->in_fb.size.height;
	in_size.x_off = scaler->src_win.x;
	in_size.y_off = scaler->src_win.y;
	in_size.scal_height= scaler->src_win.height;
	in_size.scal_width= scaler->src_win.width;

	//	out_type.byte_seq = Scaler_sw_para_to_reg(2,scaler->out_fb.seq);
	//	out_type.fmt = Scaler_sw_para_to_reg(3, scaler->out_fb.format);

	if(get_fb_type(scaler->out_fb.format) == DISP_FB_TYPE_YUV) {
		if(scaler->out_fb.mode == DISP_MOD_NON_MB_PLANAR)	{
			out_type.fmt = Scaler_sw_para_to_reg(3, scaler->out_fb.mode, scaler->out_fb.format, scaler->out_fb.seq);
		}	else {
			DE_WRN("output mode:%d invalid in Scaler_Set_Para\n",scaler->out_fb.mode);
			return DIS_FAIL;
		}
	} else {
		if(scaler->out_fb.mode == DISP_MOD_NON_MB_PLANAR
		    && (scaler->out_fb.format == DISP_FORMAT_RGB888 || scaler->out_fb.format == DISP_FORMAT_ARGB8888)) {
			out_type.fmt = DE_SCAL_OUTPRGB888;
		}	else if(scaler->out_fb.mode == DISP_MOD_INTERLEAVED
		    && scaler->out_fb.format == DISP_FORMAT_ARGB8888 && scaler->out_fb.seq == DISP_SEQ_ARGB) {
			out_type.fmt = DE_SCAL_OUTI1RGB888;
		}else if(scaler->out_fb.mode == DISP_MOD_INTERLEAVED && scaler->out_fb.format == DISP_FORMAT_ARGB8888
		    && scaler->out_fb.seq == DISP_SEQ_BGRA) {
			out_type.fmt = DE_SCAL_OUTI0RGB888;
		}	else {
			DE_WRN("output para invalid in Scaler_Set_Para,mode:%d,format:%d\n",scaler->out_fb.mode, scaler->out_fb.format);
			return DIS_FAIL;
		}
	}
	out_type.byte_seq = Scaler_sw_para_to_reg(2,scaler->out_fb.mode, scaler->out_fb.format, scaler->out_fb.seq);
	out_type.alpha_en = 1;
	out_type.alpha_coef_type = 0;

	out_size.width = scaler->out_size.width;
	out_size.height = scaler->out_size.height;

	in_scan.field = FALSE;
	in_scan.bottom = FALSE;

	out_scan.field = (gdisp.screen[screen_index].de_flicker_status & DE_FLICKER_USED)?FALSE: gdisp.screen[screen_index].b_out_interlace;

	if(scaler->in_fb.cs_mode > DISP_VXYCC) {
		scaler->in_fb.cs_mode = DISP_BT601;
	}

	if(scaler->in_fb.b_trd_src)	{
		__scal_3d_inmode_t inmode;
		__scal_3d_outmode_t outmode = 0;
		__scal_buf_addr_t scal_addr_right;

		if(bsp_disp_feat_get_layer_feats(scaler->screen_index, DISP_LAYER_WORK_MODE_SCALER, scaler_index) & DISP_LAYER_FEAT_3D) {
			inmode = Scaler_3d_sw_para_to_reg(0, scaler->in_fb.trd_mode, 0);
			outmode = Scaler_3d_sw_para_to_reg(1, scaler->out_trd_mode, gdisp.screen[screen_index].b_out_interlace);

			if(((__s32)inmode == DIS_FAIL) || ((__s32)outmode == DIS_FAIL))	{
				DE_WRN("input 3d para invalid in Scaler_Set_Para,trd_mode:%d,out_trd_mode:%d\n", scaler->in_fb.trd_mode, scaler->out_trd_mode);
			}

			DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
			if(scaler->b_trd_out)	{
				DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
			}

			scal_addr_right.ch0_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.trd_right_addr[0]));
			scal_addr_right.ch1_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.trd_right_addr[1]));
			scal_addr_right.ch2_addr= (__u32)OSAL_VAtoPA((void*)(scaler->in_fb.trd_right_addr[2]));

			DE_SCAL_Set_3D_Ctrl(scaler_index, scaler->b_trd_out, inmode, outmode);
			DE_SCAL_Config_3D_Src(scaler_index, &scal_addr, &in_size, &in_type, inmode, &scal_addr_right);
			DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, scaler->b_trd_out, outmode);
		}	else {
			DE_WRN("This platform not support 3d output!\n");
			DE_SCAL_Config_Src(scaler_index,&scal_addr,&in_size,&in_type,FALSE,FALSE);
			DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, 0, 0);
		}
	}	else {
		DE_SCAL_Config_Src(scaler_index,&scal_addr,&in_size,&in_type,FALSE,FALSE);
		DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, 0, 0);
	}
	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	DE_SCAL_Set_Init_Phase(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, FALSE);
	if(scaler->enhance_en == TRUE) {
		Scaler_Set_Enhance(scaler_index, scaler->bright, scaler->contrast, scaler->saturation, scaler->hue);
	}	else {
		DE_SCAL_Set_CSC_Coef(scaler_index, scaler->in_fb.cs_mode, DISP_BT601, get_fb_type(scaler->in_fb.format), get_fb_type(scaler->out_fb.format), scaler->in_fb.br_swap, 0);
	}
	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, scaler->smooth_mode);
	DE_SCAL_Set_Out_Format(scaler_index, &out_type);
	DE_SCAL_Set_Out_Size(scaler_index, &out_scan,&out_type, &out_size);

	return DIS_NULL;
}

__s32 Scaler_Set_Outitl(__u32 scaler_index,  __bool enable)
{
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__scal_src_type_t in_type;
	__scal_out_type_t out_type;
	__scal_scan_mod_t in_scan;
	__scal_scan_mod_t out_scan;
	__disp_scaler_t * scaler;

	scaler = &(gdisp.scaler[scaler_index]);

	in_type.fmt= Scaler_sw_para_to_reg(0,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.mod= Scaler_sw_para_to_reg(1,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.ps= Scaler_sw_para_to_reg(2,scaler->in_fb.mode, scaler->in_fb.format, (__u8)scaler->in_fb.seq);
	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	in_size.src_width = scaler->in_fb.size.width;
	in_size.src_height = scaler->in_fb.size.height;
	in_size.x_off =  scaler->src_win.x;
	in_size.y_off =  scaler->src_win.y;
	in_size.scal_height=  scaler->src_win.height;
	in_size.scal_width=  scaler->src_win.width;

	out_type.byte_seq =  scaler->out_fb.seq;
	out_type.fmt =  scaler->out_fb.format;

	out_size.width =  scaler->out_size.width;
	out_size.height =  scaler->out_size.height;

	in_scan.field = FALSE;
	in_scan.bottom = FALSE;

	out_scan.field = enable;

	DE_SCAL_Set_Init_Phase(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, FALSE);
	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type,  scaler->smooth_mode);
	DE_SCAL_Set_Out_Size(scaler_index, &out_scan,&out_type, &out_size);

	return DIS_SUCCESS;
}

__s32 bsp_disp_scaler_set_smooth(__u32 scaler_index, __disp_video_smooth_t  mode)
{
	__disp_scaler_t * scaler;
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__scal_src_type_t in_type;
	__scal_out_type_t out_type;
	__scal_scan_mod_t in_scan;
	__scal_scan_mod_t out_scan;
	__u32 screen_index;

	scaler = &(gdisp.scaler[scaler_index]);
	screen_index = gdisp.scaler[scaler_index].screen_index;
	scaler->smooth_mode = mode;

	in_type.fmt= Scaler_sw_para_to_reg(0,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.mod= Scaler_sw_para_to_reg(1,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
	in_type.ps= Scaler_sw_para_to_reg(2,scaler->in_fb.mode, scaler->in_fb.format, (__u8)scaler->in_fb.seq);
	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	in_size.src_width = scaler->in_fb.size.width;
	in_size.src_height = scaler->in_fb.size.height;
	in_size.x_off = scaler->src_win.x;
	in_size.y_off = scaler->src_win.y;
	in_size.scal_height= scaler->src_win.height;
	in_size.scal_width= scaler->src_win.width;

	out_type.byte_seq = scaler->out_fb.seq;
	out_type.fmt = scaler->out_fb.format;

	out_size.width = scaler->out_size.width;
	out_size.height = scaler->out_size.height;

	in_scan.field = FALSE;
	in_scan.bottom = FALSE;

	out_scan.field = (gdisp.screen[screen_index].de_flicker_status & DE_FLICKER_USED)?FALSE: gdisp.screen[screen_index].b_out_interlace;

	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, scaler->smooth_mode);
	scaler->b_reg_change = TRUE;

	return DIS_SUCCESS;
}

__s32 bsp_disp_scaler_get_smooth(__u32 scaler_index)
{
	return gdisp.scaler[scaler_index].smooth_mode;
}


__s32 bsp_disp_scaler_request(void)
{
	__s32 scaler_index = 0;
	scaler_index =  Scaler_Request(0xff);
	if(scaler_index < 0)
		return scaler_index;
	else
		gdisp.scaler[scaler_index].screen_index = 0xff;
	return SCALER_IDTOHAND(scaler_index);
}

__s32 bsp_disp_scaler_release(__u32 handle)
{
	__u32 scaler_index = 0;

	scaler_index = SCALER_HANDTOID(handle);
	if(gdisp.scaler[scaler_index].screen_index == 0xff) {
		return Scaler_Release(scaler_index, FALSE);
	}
	DE_INF("bsp_disp_scaler_release, scaler %d not a independent scaler\n", scaler_index);
	return 0;
}

__s32 bsp_disp_scaler_start_ex(__u32 handle,__disp_scaler_para_t *para)
{
	__scal_buf_addr_t in_addr;
	__scal_buf_addr_t out_addr;
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__scal_src_type_t in_type;
	__scal_out_type_t out_type;
	__scal_scan_mod_t in_scan;
	__scal_scan_mod_t out_scan;
	__u32 size = 0;
	__u32 scaler_index = 0;
	__s32 ret = 0;

	if(para==NULL) {
		DE_WRN("input parameter can't be null!\n");
		return DIS_FAIL;
	}

	scaler_index = SCALER_HANDTOID(handle);

	in_type.fmt= Scaler_sw_para_to_reg(0,para->input_fb.mode, para->input_fb.format, para->input_fb.seq);
	in_type.mod= Scaler_sw_para_to_reg(1,para->input_fb.mode, para->input_fb.format, para->input_fb.seq);
	in_type.ps= Scaler_sw_para_to_reg(2,para->input_fb.mode, para->input_fb.format, (__u8)para->input_fb.seq);
	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	if(get_fb_type(para->output_fb.format) == DISP_FB_TYPE_YUV)	{
		if(para->output_fb.mode == DISP_MOD_NON_MB_PLANAR) {
			out_type.fmt = Scaler_sw_para_to_reg(3, para->output_fb.mode, para->output_fb.format, para->output_fb.seq);
		}	else {
			DE_WRN("output mode:%d invalid in Display_Scaler_Start\n",para->output_fb.mode);
			return DIS_FAIL;
		}
	}	else {
		if(para->output_fb.mode == DISP_MOD_NON_MB_PLANAR
		    && (para->output_fb.format == DISP_FORMAT_RGB888 || para->output_fb.format == DISP_FORMAT_ARGB8888)) {
			out_type.fmt = DE_SCAL_OUTPRGB888;
		}	else if(para->output_fb.mode == DISP_MOD_INTERLEAVED && para->output_fb.format == DISP_FORMAT_ARGB8888)	{
			out_type.fmt = DE_SCAL_OUTI0RGB888;
		}	else {
			DE_WRN("output para invalid in Display_Scaler_Start,mode:%d,format:%d\n",para->output_fb.mode, para->output_fb.format);
			return DIS_FAIL;
		}
	}
	out_type.byte_seq = Scaler_sw_para_to_reg(2,para->output_fb.mode, para->output_fb.format, para->output_fb.seq);
	out_type.alpha_en = 1;
	out_type.alpha_coef_type = 0;

	out_size.width  = para->out_regn.width;
	out_size.height = para->out_regn.height;
	out_size.x_off = para->out_regn.x;
	out_size.y_off = para->out_regn.y;
	out_size.fb_width = para->output_fb.size.width;
	out_size.fb_height = para->output_fb.size.height;

	in_addr.ch0_addr = (__u32)OSAL_VAtoPA((void*)(para->input_fb.addr[0]));
	in_addr.ch1_addr = (__u32)OSAL_VAtoPA((void*)(para->input_fb.addr[1]));
	in_addr.ch2_addr = (__u32)OSAL_VAtoPA((void*)(para->input_fb.addr[2]));

	in_size.src_width = para->input_fb.size.width;
	in_size.src_height = para->input_fb.size.height;
	in_size.x_off = para->source_regn.x;
	in_size.y_off = para->source_regn.y;
	in_size.scal_width= para->source_regn.width;
	in_size.scal_height= para->source_regn.height;

	in_scan.field = FALSE;
	in_scan.bottom = FALSE;

	out_scan.field = FALSE;	//when use scaler as writeback, won't be outinterlaced for any display device
	out_scan.bottom = FALSE;

	out_addr.ch0_addr = (__u32)OSAL_VAtoPA((void*)(para->output_fb.addr[0]));
	out_addr.ch1_addr = (__u32)OSAL_VAtoPA((void*)(para->output_fb.addr[1]));
	out_addr.ch2_addr = (__u32)OSAL_VAtoPA((void*)(para->output_fb.addr[2]));

	size = (para->input_fb.size.width * para->input_fb.size.height * de_format_to_bpp(para->input_fb.format) + 7)/8;
	OSAL_CacheRangeFlush((void *)para->input_fb.addr[0],size ,CACHE_CLEAN_FLUSH_D_CACHE_REGION);

	size = (para->output_fb.size.width * para->output_fb.size.height * de_format_to_bpp(para->output_fb.format) + 7)/8;
	OSAL_CacheRangeFlush((void *)para->output_fb.addr[0],size ,CACHE_FLUSH_D_CACHE_REGION);
	if(para->input_fb.b_trd_src) {
		__scal_3d_inmode_t inmode;
		__scal_3d_outmode_t outmode = 0;
		__scal_buf_addr_t scal_addr_right;

		inmode = Scaler_3d_sw_para_to_reg(0, para->input_fb.trd_mode, FALSE);
		outmode = Scaler_3d_sw_para_to_reg(1, para->output_fb.trd_mode, FALSE);

		DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
		if(para->output_fb.b_trd_src)	{
			DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
		}

		scal_addr_right.ch0_addr= (__u32)OSAL_VAtoPA((void*)(para->input_fb.trd_right_addr[0]));
		scal_addr_right.ch1_addr= (__u32)OSAL_VAtoPA((void*)(para->input_fb.trd_right_addr[1]));
		scal_addr_right.ch2_addr= (__u32)OSAL_VAtoPA((void*)(para->input_fb.trd_right_addr[2]));

		DE_SCAL_Set_3D_Ctrl(scaler_index, para->output_fb.b_trd_src, inmode, outmode);
		DE_SCAL_Config_3D_Src(scaler_index, &in_addr, &in_size, &in_type, inmode, &scal_addr_right);
		DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, para->output_fb.b_trd_src, outmode);
	}	else {
		DE_SCAL_Config_Src(scaler_index,&in_addr,&in_size,&in_type,FALSE,FALSE);
		DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, 0, 0);
	}
	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	DE_SCAL_Set_Init_Phase(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, FALSE);
	DE_SCAL_Set_CSC_Coef(scaler_index, para->input_fb.cs_mode, para->output_fb.cs_mode, get_fb_type(para->input_fb.format), get_fb_type(para->output_fb.format),  para->input_fb.br_swap, para->output_fb.br_swap);
	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, DISP_VIDEO_NATUAL);
	DE_SCAL_Set_Out_Format(scaler_index, &out_type);
	DE_SCAL_Set_Out_Size(scaler_index, &out_scan,&out_type, &out_size);
	//DE_SCAL_Set_Writeback_Addr(scaler_index,&out_addr);
	DE_SCAL_Set_Writeback_Addr_ex(scaler_index,&out_addr,&out_size,&out_type);
	DE_SCAL_Writeback_Linestride_Enable(scaler_index);
	DE_SCAL_Output_Select(scaler_index, 3);
	DE_SCAL_Input_Select(scaler_index, 0);

	DE_SCAL_EnableINT(scaler_index,DE_WB_END_IE);
	DE_SCAL_Start(scaler_index);
	DE_SCAL_Set_Reg_Rdy(scaler_index);

	DE_INF("scaler write back begin\n");

#ifndef __LINUX_OSAL__
	DE_SCAL_Writeback_Enable(scaler_index);
	while(!(DE_SCAL_QueryINT(scaler_index) & DE_WB_END_IE)) {
	}
#else
	{
		long timeout = (100 * HZ)/1000;//100ms

		init_waitqueue_head(&(gdisp.scaler[scaler_index].scaler_queue));
		gdisp.scaler[scaler_index].b_scaler_finished = 1;
		DE_SCAL_Writeback_Enable(scaler_index);

		timeout = wait_event_interruptible_timeout(gdisp.scaler[scaler_index].scaler_queue, gdisp.scaler[scaler_index].b_scaler_finished == 2, timeout);
		gdisp.scaler[scaler_index].b_scaler_finished = 0;
		if(timeout == 0)	{
			__wrn("wait scaler %d finished timeout\n", scaler_index);
			return -1;
		}
	}
#endif
	DE_SCAL_Reset(scaler_index);
	DE_SCAL_Writeback_Disable(scaler_index);
	DE_SCAL_Writeback_Linestride_Disable(scaler_index);
	DE_SCAL_ClearINT(scaler_index,DE_WB_END_IE);
	DE_SCAL_DisableINT(scaler_index,DE_WB_END_IE);

	return ret;
}


__s32 bsp_disp_scaler_start(__u32 handle,__disp_scaler_para_t *para)
{
	__scal_buf_addr_t in_addr;
	__scal_buf_addr_t out_addr;
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__scal_src_type_t in_type;
	__scal_out_type_t out_type;
	__scal_scan_mod_t in_scan;
	__scal_scan_mod_t out_scan;
	__u32 size = 0;
	__u32 scaler_index = 0;
	__s32 ret = 0;

	if(para==NULL) {
		DE_WRN("input parameter can't be null!\n");
		return DIS_FAIL;
	}

	scaler_index = SCALER_HANDTOID(handle);

	in_type.fmt= Scaler_sw_para_to_reg(0,para->input_fb.mode, para->input_fb.format, para->input_fb.seq);
	in_type.mod= Scaler_sw_para_to_reg(1,para->input_fb.mode, para->input_fb.format, para->input_fb.seq);
	in_type.ps= Scaler_sw_para_to_reg(2,para->input_fb.mode, para->input_fb.format, (__u8)para->input_fb.seq);
	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	if(get_fb_type(para->output_fb.format) == DISP_FB_TYPE_YUV)	{
		if(para->output_fb.mode == DISP_MOD_NON_MB_PLANAR) {
			out_type.fmt = Scaler_sw_para_to_reg(3, para->output_fb.mode, para->output_fb.format, para->output_fb.seq);
		}	else {
			DE_WRN("output mode:%d invalid in Display_Scaler_Start\n",para->output_fb.mode);
			return DIS_FAIL;
		}
	}	else {
		if(para->output_fb.mode == DISP_MOD_NON_MB_PLANAR
		    && (para->output_fb.format == DISP_FORMAT_RGB888 || para->output_fb.format == DISP_FORMAT_ARGB8888)) {
			out_type.fmt = DE_SCAL_OUTPRGB888;
		}	else if(para->output_fb.mode == DISP_MOD_INTERLEAVED && para->output_fb.format == DISP_FORMAT_ARGB8888)	{
			out_type.fmt = DE_SCAL_OUTI0RGB888;
		}	else {
			DE_WRN("output para invalid in Display_Scaler_Start,mode:%d,format:%d\n",para->output_fb.mode, para->output_fb.format);
			return DIS_FAIL;
		}
	}
	out_type.byte_seq = Scaler_sw_para_to_reg(2,para->output_fb.mode, para->output_fb.format, para->output_fb.seq);
	out_type.alpha_en = 1;
	out_type.alpha_coef_type = 0;

	out_size.width     = para->output_fb.size.width;
	out_size.height = para->output_fb.size.height;

	in_addr.ch0_addr = (__u32)OSAL_VAtoPA((void*)(para->input_fb.addr[0]));
	in_addr.ch1_addr = (__u32)OSAL_VAtoPA((void*)(para->input_fb.addr[1]));
	in_addr.ch2_addr = (__u32)OSAL_VAtoPA((void*)(para->input_fb.addr[2]));

	in_size.src_width = para->input_fb.size.width;
	in_size.src_height = para->input_fb.size.height;
	in_size.x_off = para->source_regn.x;
	in_size.y_off = para->source_regn.y;
	in_size.scal_width= para->source_regn.width;
	in_size.scal_height= para->source_regn.height;

	in_scan.field = FALSE;
	in_scan.bottom = FALSE;

	out_scan.field = FALSE;	//when use scaler as writeback, won't be outinterlaced for any display device
	out_scan.bottom = FALSE;

	out_addr.ch0_addr = (__u32)OSAL_VAtoPA((void*)(para->output_fb.addr[0]));
	out_addr.ch1_addr = (__u32)OSAL_VAtoPA((void*)(para->output_fb.addr[1]));
	out_addr.ch2_addr = (__u32)OSAL_VAtoPA((void*)(para->output_fb.addr[2]));

	size = (para->input_fb.size.width * para->input_fb.size.height * de_format_to_bpp(para->input_fb.format) + 7)/8;
	OSAL_CacheRangeFlush((void *)para->input_fb.addr[0],size ,CACHE_CLEAN_FLUSH_D_CACHE_REGION);

	size = (para->output_fb.size.width * para->output_fb.size.height * de_format_to_bpp(para->output_fb.format) + 7)/8;
	OSAL_CacheRangeFlush((void *)para->output_fb.addr[0],size ,CACHE_FLUSH_D_CACHE_REGION);
	if(para->input_fb.b_trd_src) {
		__scal_3d_inmode_t inmode;
		__scal_3d_outmode_t outmode = 0;
		__scal_buf_addr_t scal_addr_right;

		inmode = Scaler_3d_sw_para_to_reg(0, para->input_fb.trd_mode, FALSE);
		outmode = Scaler_3d_sw_para_to_reg(1, para->output_fb.trd_mode, FALSE);

		DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
		if(para->output_fb.b_trd_src)	{
			DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
		}

		scal_addr_right.ch0_addr= (__u32)OSAL_VAtoPA((void*)(para->input_fb.trd_right_addr[0]));
		scal_addr_right.ch1_addr= (__u32)OSAL_VAtoPA((void*)(para->input_fb.trd_right_addr[1]));
		scal_addr_right.ch2_addr= (__u32)OSAL_VAtoPA((void*)(para->input_fb.trd_right_addr[2]));

		DE_SCAL_Set_3D_Ctrl(scaler_index, para->output_fb.b_trd_src, inmode, outmode);
		DE_SCAL_Config_3D_Src(scaler_index, &in_addr, &in_size, &in_type, inmode, &scal_addr_right);
		DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, para->output_fb.b_trd_src, outmode);
	}	else {
		DE_SCAL_Config_Src(scaler_index,&in_addr,&in_size,&in_type,FALSE,FALSE);
		DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, 0, 0);
	}
	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	DE_SCAL_Set_Init_Phase(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, FALSE);
	DE_SCAL_Set_CSC_Coef(scaler_index, para->input_fb.cs_mode, para->output_fb.cs_mode, get_fb_type(para->input_fb.format), get_fb_type(para->output_fb.format),  para->input_fb.br_swap, para->output_fb.br_swap);
	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, DISP_VIDEO_NATUAL);
	DE_SCAL_Set_Out_Format(scaler_index, &out_type);
	DE_SCAL_Set_Out_Size(scaler_index, &out_scan,&out_type, &out_size);
	DE_SCAL_Set_Writeback_Addr(scaler_index,&out_addr);
	DE_SCAL_Output_Select(scaler_index, 3);
	DE_SCAL_Input_Select(scaler_index, 0);

	DE_SCAL_EnableINT(scaler_index,DE_WB_END_IE);
	DE_SCAL_Start(scaler_index);
	DE_SCAL_Set_Reg_Rdy(scaler_index);

	DE_INF("scaler write back begin\n");

#ifndef __LINUX_OSAL__
	DE_SCAL_Writeback_Enable(scaler_index);
	while(!(DE_SCAL_QueryINT(scaler_index) & DE_WB_END_IE)) {
	}
#else
	{
		long timeout = (100 * HZ)/1000;//100ms

		init_waitqueue_head(&(gdisp.scaler[scaler_index].scaler_queue));
		gdisp.scaler[scaler_index].b_scaler_finished = 1;
		DE_SCAL_Writeback_Enable(scaler_index);

		timeout = wait_event_interruptible_timeout(gdisp.scaler[scaler_index].scaler_queue, gdisp.scaler[scaler_index].b_scaler_finished == 2, timeout);
		gdisp.scaler[scaler_index].b_scaler_finished = 0;
		if(timeout == 0) {
			__wrn("wait scaler %d finished timeout\n", scaler_index);
			return -1;
		}
	}
#endif
	DE_SCAL_Reset(scaler_index);
	DE_SCAL_Writeback_Disable(scaler_index);
	DE_SCAL_ClearINT(scaler_index,DE_WB_END_IE);
	DE_SCAL_DisableINT(scaler_index,DE_WB_END_IE);

	return ret;
}

__s32 Scaler_Set_Enhance(__u32 scaler_index, __u32 bright, __u32 contrast, __u32 saturation, __u32 hue)
{
	__u32 b_yuv_in,b_yuv_out;
	__disp_scaler_t * scaler;

	scaler = &(gdisp.scaler[scaler_index]);

	b_yuv_in = (get_fb_type(scaler->in_fb.format) == DISP_FB_TYPE_YUV)?1:0;
	b_yuv_out = (get_fb_type(scaler->out_fb.format) == DISP_FB_TYPE_YUV)?1:0;
	DE_SCAL_Set_CSC_Coef_Enhance(scaler_index, scaler->in_fb.cs_mode, scaler->out_fb.cs_mode, b_yuv_in, b_yuv_out, bright, contrast, saturation, hue, scaler->in_fb.br_swap, 0);
	scaler->b_reg_change = TRUE;

	return DIS_SUCCESS;
}


__s32 bsp_disp_store_scaler_reg(__u32 scaler_index, __u32 addr)
{
	__u32 i = 0;
	__u32 value = 0;
	__u32 reg_base = 0;

	if(addr == 0) {
		DE_WRN("bsp_disp_store_scaler_reg, addr is NULL!");
		return -1;
	}

	if(scaler_index == 0) {
		reg_base = gdisp.init_para.reg_base[DISP_MOD_FE0];
	}	else {
		reg_base = gdisp.init_para.reg_base[DISP_MOD_FE1];
	}

	for(i=0; i<0xa18; i+=4)	{
		value = sys_get_wvalue(reg_base + i);
		sys_put_wvalue(addr + i, value);
	}

	return 0;
}

__s32 bsp_disp_restore_scaler_reg(__u32 scaler_index, __u32 addr)
{
	__u32 i = 0;
	__u32 value = 0;
	__u32 reg_base = 0;

	if(addr == 0) {
		DE_WRN("bsp_disp_restore_scaler_reg, addr is NULL!");
		return -1;
	}

	if(scaler_index == 0) {
		reg_base = gdisp.init_para.reg_base[DISP_MOD_FE0];
	}	else {
		reg_base = gdisp.init_para.reg_base[DISP_MOD_FE1];
	}

	for(i=8; i<0xa18; i+=4)	{
		value = sys_get_wvalue(addr + i);
		sys_put_wvalue(reg_base + i,value);
	}

	value = sys_get_wvalue(addr);
	sys_put_wvalue(reg_base,value);

	value = sys_get_wvalue(addr + 4);
	sys_put_wvalue(reg_base + 4,value);

	return 0;
}

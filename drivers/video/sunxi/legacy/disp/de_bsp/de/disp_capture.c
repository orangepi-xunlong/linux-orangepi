#include "disp_capture.h"
#include "disp_scaler.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_layer.h"
#include "disp_clk.h"
#include "disp_lcd.h"
#include "disp_de.h"

#if defined CONFIG_ARCH_SUN8IW1P1
__s32 bsp_disp_capture_screen(__u32 screen_id, __disp_capture_screen_para_t * para)
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
	__s32 scaler_index = 0;
	__s32 ret = 0;

	if(para==NULL) {
		DE_WRN("input parameter can't be null!\n");
		return DIS_FAIL;
	}

	scaler_index =  Scaler_Request(0xff);
	if(scaler_index < 0) {
		DE_WRN("request scaler fail in bsp_disp_capture_screen\n");
		return DIS_FAIL;
	} else {
		gdisp.scaler[screen_id].screen_index = 0xff;
	}

	memset(&gdisp.capture_para[screen_id], 0, sizeof(__disp_capture_screen_para_t));
	memcpy(&gdisp.capture_para[screen_id], para,sizeof(__disp_capture_screen_para_t));
	gdisp.capture_para[screen_id].capture_request = 1;
	gdisp.capture_para[screen_id].scaler_id = scaler_index;
	gdisp.capture_para[screen_id].cur_buffer_id= 0;

	in_type.fmt= Scaler_sw_para_to_reg(0,DISP_MOD_INTERLEAVED, DISP_FORMAT_ARGB8888, DISP_SEQ_ARGB);
	in_type.mod= Scaler_sw_para_to_reg(1,DISP_MOD_INTERLEAVED, DISP_FORMAT_ARGB8888, DISP_SEQ_ARGB);
	in_type.ps= Scaler_sw_para_to_reg(2,DISP_MOD_INTERLEAVED, DISP_FORMAT_ARGB8888, (__u8)DISP_SEQ_ARGB);
	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	if(get_fb_type(para->output_fb[0].format) == DISP_FB_TYPE_YUV) {
		if(para->output_fb[0].mode == DISP_MOD_NON_MB_PLANAR) {
			out_type.fmt = Scaler_sw_para_to_reg(3, para->output_fb[0].mode, para->output_fb[0].format, para->output_fb[0].seq);
		} else {
			DE_WRN("output mode:%d invalid in Display_Scaler_Start\n",para->output_fb[0].mode);
			return DIS_FAIL;
		}
	} else {
		if(para->output_fb[0].mode == DISP_MOD_NON_MB_PLANAR &&
		    (para->output_fb[0].format == DISP_FORMAT_RGB888 || para->output_fb[0].format == DISP_FORMAT_ARGB8888)) {
			out_type.fmt = DE_SCAL_OUTPRGB888;
		} else if(para->output_fb[0].mode == DISP_MOD_INTERLEAVED && para->output_fb[0].format == DISP_FORMAT_ARGB8888) {
			out_type.fmt = DE_SCAL_OUTI0RGB888;
		} else {
			DE_WRN("output para invalid in Display_Scaler_Start,mode:%d,format:%d\n",para->output_fb[0].mode, para->output_fb[0].format);
			return DIS_FAIL;
		}
		para->output_fb[0].br_swap= FALSE;
	}
	out_type.byte_seq = Scaler_sw_para_to_reg(2,para->output_fb[0].mode, para->output_fb[0].format, para->output_fb[0].seq);
	out_type.alpha_en = 1;
	out_type.alpha_coef_type = 0;

	out_size.width     = para->output_fb[0].size.width;
	out_size.height = para->output_fb[0].size.height;

	if(bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_NONE) {
		in_size.src_width = bsp_disp_get_screen_width(screen_id);
		in_size.src_height = bsp_disp_get_screen_height(screen_id);
		in_size.x_off = 0;
		in_size.y_off = 0;
		in_size.scal_width= bsp_disp_get_screen_width(screen_id);
		in_size.scal_height= bsp_disp_get_screen_height(screen_id);
	} else {
		in_size.src_width = para->screen_size.width;
		in_size.src_height= para->screen_size.height;
		in_size.x_off = 0;
		in_size.y_off = 0;
		in_size.scal_width= para->screen_size.width;
		in_size.scal_height= para->screen_size.height;
	}

	in_scan.field = FALSE;
	in_scan.bottom = FALSE;

	out_scan.field = FALSE;	//when use scaler as writeback, won't be outinterlaced for any display device
	out_scan.bottom = FALSE;

	in_addr.ch0_addr = 0;
	in_addr.ch1_addr = 0;
	in_addr.ch2_addr = 0;

	out_addr.ch0_addr = (__u32)OSAL_VAtoPA((void*)(para->output_fb[0].addr[0]));
	out_addr.ch1_addr = (__u32)OSAL_VAtoPA((void*)(para->output_fb[0].addr[1]));
	out_addr.ch2_addr = (__u32)OSAL_VAtoPA((void*)(para->output_fb[0].addr[2]));

	size = (para->output_fb[0].size.width * para->output_fb[0].size.height * de_format_to_bpp(para->output_fb[0].format) + 7)/8;
	OSAL_CacheRangeFlush((void *)para->output_fb[0].addr[0],size ,CACHE_FLUSH_D_CACHE_REGION);

	if(bsp_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_NONE) {
		DE_SCAL_Input_Select(scaler_index, 6 + screen_id);
		DE_BE_set_display_size(screen_id, para->screen_size.width, para->screen_size.height);
		DE_BE_Output_Select(screen_id, 6 + scaler_index);
		image_clk_on(screen_id, 1);
		Image_open(screen_id);
		DE_BE_Cfg_Ready(screen_id);
	} else {
		DE_SCAL_Input_Select(scaler_index, 4 + screen_id);
		DE_BE_Output_Select(screen_id, 2 + (scaler_index * 2) + screen_id);
	}
	DE_SCAL_Config_Src(scaler_index,&in_addr,&in_size,&in_type,FALSE,FALSE);
	DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	DE_SCAL_Set_Init_Phase(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, FALSE);
	DE_SCAL_Set_CSC_Coef(scaler_index, DISP_BT601, para->output_fb[0].cs_mode, DISP_FB_TYPE_RGB, get_fb_type(para->output_fb[0].format), 0, 0);
	DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, DISP_VIDEO_NATUAL);
	DE_SCAL_Set_Out_Format(scaler_index, &out_type);
	DE_SCAL_Set_Out_Size(scaler_index, &out_scan,&out_type, &out_size);
	DE_SCAL_Set_Writeback_Addr(scaler_index,&out_addr);
	DE_SCAL_Output_Select(scaler_index, 3);
	DE_SCAL_ClearINT(scaler_index,DE_WB_END_IE);
	//DE_SCAL_EnableINT(scaler_index,DE_WB_END_IE);
	DE_SCAL_DisableINT(scaler_index,DE_WB_END_IE);
	DE_SCAL_Set_Reg_Rdy(scaler_index);
	DE_SCAL_Writeback_Enable(scaler_index);
	DE_SCAL_Start(scaler_index);

	DE_INF("capture begin\n");
#ifndef __LINUX_OSAL__
	while(!(DE_SCAL_QueryINT(scaler_index) & DE_WB_END_IE)){
	}
#endif
	return ret;
}

__s32 bsp_disp_capture_screen_stop(__u32 screen_id)
{
	__s32 ret = 0;
	__u32 scaler_index;

	if(gdisp.capture_para[screen_id].capture_request == 1) {
	gdisp.capture_para[screen_id].capture_request = 0;
		if(gdisp.capture_para[screen_id].scaler_id >= 0) {
			scaler_index = gdisp.capture_para[screen_id].scaler_id;
			DE_SCAL_Reset(scaler_index);
			Scaler_Release(scaler_index, FALSE);
			if(bsp_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_NONE) {
				Image_close(screen_id);
				image_clk_off(screen_id, 1);
			}
			DE_BE_Output_Select(screen_id, screen_id);
			DE_SCAL_ClearINT(scaler_index,DE_WB_END_IE);
			DE_SCAL_DisableINT(scaler_index,DE_WB_END_IE);
		}

		memset(&gdisp.capture_para[screen_id], 0,sizeof(__disp_capture_screen_para_t));
	}

	return ret;
}

static __s32 disp_capture_screen_switch_buffer(__u32 screen_id)
{
	__s32 scaler_index = 0;
	__s32 ret = 0;
	__scal_buf_addr_t out_addr;
	__u32 cur_buffer_id;
	__u32 buffer_num;

	if(gdisp.capture_para[screen_id].capture_request == 1) {
		if(gdisp.capture_para[screen_id].mode == 1) {
			__disp_rect_t regn;
			scaler_index = gdisp.capture_para[screen_id].scaler_id;
			cur_buffer_id = gdisp.capture_para[screen_id].cur_buffer_id;
			buffer_num = gdisp.capture_para[screen_id].buffer_num;
			//for_test, tyle
			if(0) {
				out_addr.ch0_addr = (__u32)OSAL_VAtoPA((void*)(gdisp.capture_para[screen_id].output_fb[cur_buffer_id].addr[0]));
				out_addr.ch1_addr = (__u32)OSAL_VAtoPA((void*)(gdisp.capture_para[screen_id].output_fb[cur_buffer_id].addr[1]));
				out_addr.ch2_addr = (__u32)OSAL_VAtoPA((void*)(gdisp.capture_para[screen_id].output_fb[cur_buffer_id].addr[2]));
				if(bsp_disp_layer_get_src_window(1,100,&regn) == 0) {
					regn.y = (regn.y == 0)?regn.height:0;
					bsp_disp_layer_set_src_window(1,100,&regn);
				}
			}

			cur_buffer_id ++;
			cur_buffer_id = (cur_buffer_id ==  buffer_num)? 0:cur_buffer_id;
			gdisp.capture_para[screen_id].cur_buffer_id = cur_buffer_id;
			out_addr.ch0_addr = (__u32)OSAL_VAtoPA((void*)(gdisp.capture_para[screen_id].output_fb[cur_buffer_id].addr[0]));
			out_addr.ch1_addr = (__u32)OSAL_VAtoPA((void*)(gdisp.capture_para[screen_id].output_fb[cur_buffer_id].addr[1]));
			out_addr.ch2_addr = (__u32)OSAL_VAtoPA((void*)(gdisp.capture_para[screen_id].output_fb[cur_buffer_id].addr[2]));
			DE_SCAL_Writeback_Disable(scaler_index);
			DE_SCAL_Set_Writeback_Addr(scaler_index,&out_addr);
			DE_SCAL_Output_Select(scaler_index, 3);
			DE_SCAL_Set_Reg_Rdy(scaler_index);
			DE_SCAL_Writeback_Enable(scaler_index);
			DE_SCAL_Reset(scaler_index);
			DE_SCAL_Start(scaler_index);
		} else {
			bsp_disp_capture_screen_stop(screen_id);
		}

		if(gdisp.init_para.capture_event) {
			gdisp.init_para.capture_event(screen_id);
		}
	}

	return ret;
}

#elif defined CONFIG_ARCH_SUN8IW3P1
__s32 bsp_disp_capture_screen(__u32 screen_id, __disp_capture_screen_para_t * para)
{
	__disp_rectsz_t in_size;
	__s32 ret = 0;

	if(para==NULL) {
		DE_WRN("input parameter can't be null!\n");
		return DIS_FAIL;
	}

	memset(&gdisp.capture_para[screen_id], 0, sizeof(__disp_capture_screen_para_t));
	memcpy(&gdisp.capture_para[screen_id], para,sizeof(__disp_capture_screen_para_t));
	gdisp.capture_para[screen_id].capture_request = 1;
	gdisp.capture_para[screen_id].cur_buffer_id= 0;

	if(bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_NONE) {
		in_size.width = bsp_disp_get_screen_width(screen_id);
		in_size.height = bsp_disp_get_screen_height(screen_id);
	} else {
		in_size.width = para->screen_size.width;
		in_size.height= para->screen_size.height;
	}

	WB_EBIOS_Enable(screen_id);
	ret = WB_EBIOS_Set_Para(screen_id, in_size, para->capture_window, para->output_window, para->output_fb[0]);
	if(ret != 0) {
		DE_WRN("WB_EBIOS_Set_Para fail, ret=%d\n", ret);
		WB_EBIOS_Reset(screen_id);
		return -1;
	}
	WB_EBIOS_EnableINT(screen_id);
	WB_EBIOS_Writeback_Enable(screen_id);
	WB_EBIOS_Set_Reg_Rdy(screen_id);

	DE_INF("capture begin\n");
#ifndef __LINUX_OSAL__
	while(!(DE_SCAL_QueryINT(scaler_index) & DE_WB_END_IE)){
	}
#else

#endif
	return ret;
}

__s32 bsp_disp_capture_screen_stop(__u32 screen_id)
{
	__s32 ret = 0;
	DE_INF("bsp_disp_capture_screen_stop\n");

	if(gdisp.capture_para[screen_id].capture_request == 1) {
		gdisp.capture_para[screen_id].capture_request = 0;
		//if(gdisp.capture_para[screen_id].scaler_id >= 0) {
		if(1) {
			//scaler_index = gdisp.capture_para[screen_id].scaler_id;
			WB_EBIOS_Reset(screen_id);
			WB_EBIOS_Writeback_Disable(screen_id);
			WB_EBIOS_DisableINT(screen_id);
			WB_EBIOS_ClearINT(screen_id);

			if(bsp_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_NONE) {
				Image_close(screen_id);
				image_clk_off(screen_id, 1);
			}
			DE_BE_Output_Select(screen_id, screen_id);
		}

		memset(&gdisp.capture_para[screen_id], 0,sizeof(__disp_capture_screen_para_t));
	}

	return ret;
}

static __s32 disp_capture_screen_switch_buffer(__u32 screen_id)
{
	__s32 ret = 0;
	__u32 addr[3];
	__u32 cur_buffer_id;
	__u32 buffer_num;

	if(gdisp.capture_para[screen_id].capture_request == 1) {

		if(gdisp.capture_para[screen_id].got_frame == 1) {
			if(gdisp.init_para.capture_event) {
				gdisp.init_para.capture_event(screen_id);
			}
		}
		gdisp.capture_para[screen_id].got_frame = 1;
		DE_INF("drc wb st=0x%x\n", WB_EBIOS_Get_Status(screen_id));

		if((gdisp.capture_para[screen_id].mode == 1)) {
			if((WB_EBIOS_Get_Status(screen_id) == 0)) {
				//scaler_index = gdisp.capture_para[screen_id].scaler_id;
				cur_buffer_id = gdisp.capture_para[screen_id].cur_buffer_id;
				buffer_num = gdisp.capture_para[screen_id].buffer_num;

				cur_buffer_id ++;
				cur_buffer_id = (cur_buffer_id ==  buffer_num)? 0:cur_buffer_id;
				gdisp.capture_para[screen_id].cur_buffer_id = cur_buffer_id;
				addr[0] = (__u32)OSAL_VAtoPA((void*)(gdisp.capture_para[screen_id].output_fb[cur_buffer_id].addr[0]));
				addr[1] = (__u32)OSAL_VAtoPA((void*)(gdisp.capture_para[screen_id].output_fb[cur_buffer_id].addr[1]));
				addr[2] = (__u32)OSAL_VAtoPA((void*)(gdisp.capture_para[screen_id].output_fb[cur_buffer_id].addr[2]));
				WB_EBIOS_Set_Addr(screen_id, addr);
			}else {
				WB_EBIOS_Enable(screen_id);
			}
				WB_EBIOS_Writeback_Enable(screen_id);
				WB_EBIOS_Set_Reg_Rdy(screen_id);
		}		else if(gdisp.capture_para[screen_id].mode == 0) {
			bsp_disp_capture_screen_stop(screen_id);
		}
	}

	return ret;
}
#else
__s32 bsp_disp_capture_screen(__u32 screen_id, __disp_capture_screen_para_t * para)
{
	__s32 ret = 0;
	return ret;
}

__s32 bsp_disp_capture_screen_stop(__u32 screen_id)
{
	__s32 ret = 0;

	return ret;
}

__s32 disp_capture_screen_switch_buffer(__u32 screen_id)
{
	__s32 ret = 0;

	return ret;
}

#endif

__s32 bsp_disp_capture_init(__u32 screen_id)
{
#if defined CONFIG_ARCH_SUN8IW3P1
	WB_EBIOS_Init(screen_id);
#endif

	return 0;
}

static __s32 disp_capture_screen_get_request(__u32 screen_id)
{
	return gdisp.capture_para[screen_id].capture_request;
}

__s32 bsp_disp_capture_screen_get_buffer_id(__u32 screen_id)
{
	__u32 buffer_id = 0;

	if(gdisp.capture_para[screen_id].capture_request == 1) {
		buffer_id = gdisp.capture_para[screen_id].cur_buffer_id;
		buffer_id = (buffer_id == 0)? (gdisp.capture_para[screen_id].buffer_num-1): (buffer_id -1);
	}

	return buffer_id;
}

__s32 disp_capture_screen_proc(__u32 screen_id)
{
	//__u32 scaler_id;
	static __u32 irq_count[2] = {0, 0};

	if(disp_capture_screen_get_request(screen_id)) {
		irq_count[screen_id] ++;
		//scaler_id = gdisp.capture_para[screen_id].scaler_id;
		//if(gdisp.scaler[scaler_id].screen_index == 0xff) {
		if(1) {
			if((gdisp.capture_para[screen_id].fps == 0) ||
			    (((irq_count[screen_id]&0x1) == 0x0))) {
				disp_capture_screen_switch_buffer(screen_id);
			}
		}
	}

	return 0;
}


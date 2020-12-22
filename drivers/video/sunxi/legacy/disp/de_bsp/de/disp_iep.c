#include "disp_iep.h"
#include "disp_event.h"

extern __disp_dev_t         gdisp;

__s32 iep_init(__u32 screen_id)
{
	__iep_cmu_t *cmu;
	__iep_drc_t *drc;
	__iep_deu_t *deu;

	if(disp_deu_get_support(screen_id)) {
		IEP_Deu_Init(screen_id);
	}
	if(disp_drc_get_support(screen_id)) {
		IEP_Drc_Init(screen_id);
	}
	//IEP_CMU_Init(screen_id);

	cmu = &gdisp.screen[screen_id].cmu;
	drc = &gdisp.screen[screen_id].drc;
	deu = &gdisp.scaler[screen_id].deu;

	memset(cmu, 0, sizeof(__iep_cmu_t));
	cmu->status = 0;
	cmu->layer_mode = DISP_ENHANCE_MODE_VIVID;
	cmu->layer_rect.x = 0;
	cmu->layer_rect.y = 0;
	cmu->layer_rect.width = bsp_disp_get_screen_width(screen_id);
	cmu->layer_rect.height = bsp_disp_get_screen_height(screen_id);
	cmu->layer_bright = 50;
	cmu->layer_saturation = 50;
	cmu->layer_contrast = 50;
	cmu->layer_hue = 50;

	cmu->screen_mode = DISP_ENHANCE_MODE_VIVID;
	cmu->screen_rect.x = 0;
	cmu->screen_rect.y = 0;
	cmu->screen_rect.width = bsp_disp_get_screen_width(screen_id);
	cmu->screen_rect.height = bsp_disp_get_screen_height(screen_id);
	cmu->screen_bright = 50;
	cmu->screen_saturation = 50;
	cmu->screen_contrast = 50;
	cmu->screen_hue = 50;

	memset(drc, 0, sizeof(__iep_drc_t));
	drc->enable = 0;
	drc->mode = IEP_DRC_MODE_UI;
	drc->rect.x = 0;
	drc->rect.y = 0;
	drc->rect.width = bsp_disp_get_screen_width(screen_id);
	drc->rect.height = bsp_disp_get_screen_height(screen_id);

	memset(deu, 0, sizeof(__iep_deu_t));
	deu->luma_sharpe_level = 2;

	return DIS_SUCCESS;
}

__s32 iep_exit(__u32 screen_id)
{
	//IEP_Deu_Exit(screen_id);
	//IEP_Drc_Exit(screen_id);
	//IEP_CMU_Exit(screen_id);
	return DIS_SUCCESS;
}

__s32 bsp_disp_iep_suspend(__u32 screen_id)
{
	iep_cmu_suspend(screen_id);
	if(disp_deu_get_support(screen_id)) {
		iep_deu_suspend(screen_id);
	}
	if(disp_drc_get_support(screen_id)) {
		iep_drc_suspend(screen_id);
	}

	return DIS_SUCCESS;
}

__s32 bsp_disp_iep_resume(__u32 screen_id)
{
	iep_cmu_resume(screen_id);
	if(disp_deu_get_support(screen_id)) {
		iep_deu_resume(screen_id);
	}
	if(disp_drc_get_support(screen_id)) {
		iep_drc_resume(screen_id);
	}

	return DIS_SUCCESS;
}

#define ____SEPARATOR_DRC____
//todo : csc->set_mode   or set_mode->csc?
__s32 bsp_disp_drc_enable(__u32 screen_id, __u32 en)
{
	if((DISP_OUTPUT_TYPE_LCD == bsp_disp_get_output_type(screen_id)) && disp_drc_get_support(screen_id))	{
		__iep_drc_t *drc;

		drc = &gdisp.screen[screen_id].drc;

		if((1 == en) && (0 == drc->enable))	{
			IEP_Drc_Set_Imgsize(screen_id, bsp_disp_get_screen_width(screen_id), bsp_disp_get_screen_height(screen_id));
			IEP_Drc_Set_Winodw(screen_id, drc->rect);
			IEP_Drc_Set_Mode(screen_id,drc->mode);
			bsp_disp_cfg_start(screen_id);
			/* video mode */
			if(drc->mode == IEP_DRC_MODE_VIDEO) {
			//todo?  yuv output
			DE_BE_Set_Enhance_ex(screen_id, 3, DISP_COLOR_RANGE_0_255, 0,50, 50, 50,50);
			}
			/* UI mode */
			else {
				DE_BE_Set_Enhance_ex(screen_id, 0,DISP_COLOR_RANGE_0_255, 0,50, 50, 50,50);
			}
			IEP_Drc_Enable(screen_id,TRUE);
			bsp_disp_cfg_finish(screen_id);
			gdisp.screen[screen_id].drc.enable = 1;
		} else if((0 == en) && (1 == drc->enable)) {
			bsp_disp_cfg_start(screen_id);
			IEP_Drc_Enable(screen_id,en);
			/* video mode */
			if(drc->mode == IEP_DRC_MODE_VIDEO) {
				//todo? change to RGB output
				DE_BE_Set_Enhance_ex(screen_id, 0,DISP_COLOR_RANGE_0_255, 0,50, 50, 50,50);
			}
			bsp_disp_cfg_finish(screen_id);
			gdisp.screen[screen_id].drc.enable = 0;
		}

		return DIS_SUCCESS;
	}

	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_drc_get_enable(__u32 screen_id)
{
	return gdisp.screen[screen_id].drc.enable;
}

__s32 bsp_disp_drc_set_window(__u32 screen_id,__disp_rect_t *regn)
{
	memcpy(&gdisp.screen[screen_id].drc.rect, regn, sizeof(__disp_rect_t));
	if(bsp_disp_drc_get_enable(screen_id) == 1) {
		IEP_Drc_Set_Winodw(screen_id,*regn);
	}
	return DIS_SUCCESS;
}

__s32 bsp_disp_drc_get_window(__u32 screen_id,__disp_rect_t *regn)
{
	if(DISP_OUTPUT_TYPE_LCD == bsp_disp_get_output_type(screen_id))	{
		memcpy(regn, &gdisp.screen[screen_id].drc.rect, sizeof(__disp_rect_t));
		return DIS_SUCCESS;
	}

	return DIS_NOT_SUPPORT;
}


__s32 bsp_disp_drc_set_mode(__u32 screen_id,__u32 mode)
{
	gdisp.screen[screen_id].drc.mode = mode;
	if(bsp_disp_drc_get_enable(screen_id) == 1) {
		IEP_Drc_Set_Mode(screen_id,mode);
	}
	return DIS_SUCCESS;
}
__s32 bsp_disp_drc_get_mode(__u32 screen_id)
{
	return gdisp.screen[screen_id].drc.mode;
}

//csc: 0(rgb);  1(yuv)
__s32 bsp_disp_drc_get_input_csc(__u32 screen_id)
{
	if(bsp_disp_drc_get_enable(screen_id) && (gdisp.screen[screen_id].drc.mode == IEP_DRC_MODE_VIDEO)) {
		return 1;
	}

	return 0;
}


__s32 disp_drc_start_video_mode(__u32 screen_id)
{
	gdisp.screen[screen_id].drc.mode = IEP_DRC_MODE_VIDEO;
	if(bsp_disp_drc_get_enable(screen_id) == 1)	{
		bsp_disp_cfg_start(screen_id);
		DE_BE_Set_Enhance_ex(screen_id, 3, DISP_COLOR_RANGE_0_255, 0,50, 50, 50,50);
		IEP_Drc_Set_Mode(screen_id,gdisp.screen[screen_id].drc.mode);
		bsp_disp_cfg_finish(screen_id);
	}
	return DIS_SUCCESS;
}

__s32 disp_drc_start_ui_mode(__u32 screen_id)
{
	gdisp.screen[screen_id].drc.mode = IEP_DRC_MODE_UI;
	if(bsp_disp_drc_get_enable(screen_id) == 1) {
		bsp_disp_cfg_start(screen_id);
		DE_BE_Set_Enhance_ex(screen_id, 0,DISP_COLOR_RANGE_0_255, 0,50, 50, 50,50);
		IEP_Drc_Set_Mode(screen_id,gdisp.screen[screen_id].drc.mode);
		bsp_disp_cfg_finish(screen_id);
	}

	return DIS_SUCCESS;
}

__s32 disp_drc_get_support(__u32 screen_id)
{
	return bsp_disp_feat_get_smart_backlight_support(screen_id);
}

#define ____SEPARATOR_DEU____
__s32 bsp_disp_deu_enable(__u8 screen_id, __u32 hid,  __u32 enable)
{
	__layer_man_t * layer_man;
	__scal_out_type_t out_type;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))	{
		if((bsp_disp_feat_get_layer_feats(screen_id, DISP_LAYER_WORK_MODE_SCALER, layer_man->scaler_index) & DISP_LAYER_FEAT_DETAIL_ENHANCE)) {
			__disp_scaler_t * scaler;

			scaler = &(gdisp.scaler[layer_man->scaler_index]);

			if((enable == 1) && (!gdisp.scaler[layer_man->scaler_index].deu.enable)) {
				disp_deu_set_frame_info(screen_id, IDTOHAND(hid));
				IEP_Deu_Set_Luma_Sharpness_Level(layer_man->scaler_index, scaler->deu.luma_sharpe_level);
				IEP_Deu_Set_Chroma_Sharpness_Level(layer_man->scaler_index, scaler->deu.chroma_sharpe_level);
				IEP_Deu_Set_Black_Level_Extension(layer_man->scaler_index, scaler->deu.black_exten_level);
				IEP_Deu_Set_White_Level_Extension(layer_man->scaler_index, scaler->deu.while_exten_level);

				bsp_disp_cfg_start(screen_id);
				//Scaler_Set_Para(layer_man->scaler_index,scaler);
				{
					scaler->out_fb.mode = DISP_MOD_NON_MB_PLANAR;
					scaler->out_fb.seq = DISP_SEQ_P3210;
					scaler->out_fb.format= DISP_FORMAT_YUV444;
					if(get_fb_type(scaler->out_fb.format) == DISP_FB_TYPE_YUV) {
						if(scaler->out_fb.mode == DISP_MOD_NON_MB_PLANAR)	{
							out_type.fmt = Scaler_sw_para_to_reg(3, scaler->out_fb.mode, scaler->out_fb.format, scaler->out_fb.seq);
						}	else {
							DE_WRN("output mode:%d invalid in bsp_disp_deu_enable\n",scaler->out_fb.mode);
							return DIS_FAIL;
						}
					}	else {
						if(scaler->out_fb.mode == DISP_MOD_NON_MB_PLANAR
						    && (scaler->out_fb.format == DISP_FORMAT_RGB888 || scaler->out_fb.format == DISP_FORMAT_ARGB8888)) {
							out_type.fmt = DE_SCAL_OUTPRGB888;
						}	else if(scaler->out_fb.mode == DISP_MOD_INTERLEAVED && scaler->out_fb.format == DISP_FORMAT_ARGB8888
						    && scaler->out_fb.seq == DISP_SEQ_ARGB)	{
							out_type.fmt = DE_SCAL_OUTI1RGB888;
						} else if(scaler->out_fb.mode == DISP_MOD_INTERLEAVED
						    && scaler->out_fb.format == DISP_FORMAT_ARGB8888 && scaler->out_fb.seq == DISP_SEQ_BGRA) {
							out_type.fmt = DE_SCAL_OUTI0RGB888;
						}	else {
							DE_WRN("output para invalid in bsp_disp_deu_enable,mode:%d,format:%d\n",scaler->out_fb.mode, scaler->out_fb.format);
							return DIS_FAIL;
						}
					}
					out_type.byte_seq = Scaler_sw_para_to_reg(2,scaler->out_fb.mode, scaler->out_fb.format, scaler->out_fb.seq);
					out_type.alpha_en = 1;
					out_type.alpha_coef_type = 0;

					DE_SCAL_Set_CSC_Coef(layer_man->scaler_index, scaler->in_fb.cs_mode, DISP_BT601, get_fb_type(scaler->in_fb.format), get_fb_type(scaler->out_fb.format), scaler->in_fb.br_swap, 0);
					DE_SCAL_Set_Out_Format(layer_man->scaler_index, &out_type);
				}
				IEP_Deu_Enable(layer_man->scaler_index, enable);
				bsp_disp_cfg_finish(screen_id);
			}	else if((enable == 0) && (gdisp.scaler[layer_man->scaler_index].deu.enable)) {
				bsp_disp_cfg_start(screen_id);
				//Scaler_Set_Para(layer_man->scaler_index,scaler);
				{
					scaler->out_fb.mode = DISP_MOD_INTERLEAVED;
					scaler->out_fb.seq= DISP_SEQ_ARGB;
					scaler->out_fb.format= DISP_FORMAT_ARGB8888;
					if(get_fb_type(scaler->out_fb.format) == DISP_FB_TYPE_YUV) {
						if(scaler->out_fb.mode == DISP_MOD_NON_MB_PLANAR) {
							out_type.fmt = Scaler_sw_para_to_reg(3, scaler->out_fb.mode, scaler->out_fb.format, scaler->out_fb.seq);
						}	else {
							DE_WRN("output mode:%d invalid in bsp_disp_deu_enable\n",scaler->out_fb.mode);
							return DIS_FAIL;
						}
					}	else {
						if(scaler->out_fb.mode == DISP_MOD_NON_MB_PLANAR
						    && (scaler->out_fb.format == DISP_FORMAT_RGB888 || scaler->out_fb.format == DISP_FORMAT_ARGB8888)) {
							out_type.fmt = DE_SCAL_OUTPRGB888;
						}	else if(scaler->out_fb.mode == DISP_MOD_INTERLEAVED
						    && scaler->out_fb.format == DISP_FORMAT_ARGB8888 && scaler->out_fb.seq == DISP_SEQ_ARGB) {
							out_type.fmt = DE_SCAL_OUTI1RGB888;
						} else if(scaler->out_fb.mode == DISP_MOD_INTERLEAVED && scaler->out_fb.format == DISP_FORMAT_ARGB8888
						    && scaler->out_fb.seq == DISP_SEQ_BGRA)	{
							out_type.fmt = DE_SCAL_OUTI0RGB888;
						}	else {
							DE_WRN("output para invalid in bsp_disp_deu_enable,mode:%d,format:%d\n",scaler->out_fb.mode, scaler->out_fb.format);
							return DIS_FAIL;
						}
					}
					out_type.byte_seq = Scaler_sw_para_to_reg(2,scaler->out_fb.mode, scaler->out_fb.format, scaler->out_fb.seq);
					out_type.alpha_en = 1;
					out_type.alpha_coef_type = 0;

					DE_SCAL_Set_CSC_Coef(layer_man->scaler_index, scaler->in_fb.cs_mode, DISP_BT601, get_fb_type(scaler->in_fb.format),
					    get_fb_type(scaler->out_fb.format), scaler->in_fb.br_swap, 0);
					DE_SCAL_Set_Out_Format(layer_man->scaler_index, &out_type);
				}
				enable = (bsp_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_NONE)? 2:0;
				IEP_Deu_Enable(layer_man->scaler_index, enable);
				bsp_disp_cfg_finish(screen_id);
			}

			gdisp.scaler[layer_man->scaler_index].deu.enable = enable;
			return DIS_SUCCESS;
		}
	}
	return DIS_NOT_SUPPORT;
}


__s32 bsp_disp_deu_get_enable(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		return gdisp.scaler[layer_man->scaler_index].deu.enable;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_deu_set_luma_sharp_level(__u32 screen_id, __u32 hid,__u32 level)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		if((bsp_disp_feat_get_layer_feats(screen_id, DISP_LAYER_WORK_MODE_SCALER, layer_man->scaler_index) & DISP_LAYER_FEAT_DETAIL_ENHANCE)) {
			__disp_scaler_t * scaler;

			scaler = &(gdisp.scaler[layer_man->scaler_index]);

			scaler->deu.luma_sharpe_level = level;
			if(scaler->deu.enable) {
				disp_deu_set_frame_info(screen_id, IDTOHAND(hid));
				IEP_Deu_Set_Luma_Sharpness_Level(layer_man->scaler_index, scaler->deu.luma_sharpe_level);
			}
			return DIS_SUCCESS;
		}
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_deu_get_luma_sharp_level(__u32 screen_id, __u32 hid)

{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		return gdisp.scaler[layer_man->scaler_index].deu.luma_sharpe_level;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_deu_set_chroma_sharp_level(__u32 screen_id, __u32 hid, __u32 level)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		if((bsp_disp_feat_get_layer_feats(screen_id, DISP_LAYER_WORK_MODE_SCALER, layer_man->scaler_index) & DISP_LAYER_FEAT_DETAIL_ENHANCE)) {
			gdisp.scaler[layer_man->scaler_index].deu.chroma_sharpe_level = level;
			if(gdisp.scaler[layer_man->scaler_index].deu.enable) {
				IEP_Deu_Set_Chroma_Sharpness_Level(layer_man->scaler_index,level);
			}
			return DIS_SUCCESS;
		}
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_deu_get_chroma_sharp_level(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		return gdisp.scaler[layer_man->scaler_index].deu.chroma_sharpe_level;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_deu_set_white_exten_level(__u32 screen_id, __u32 hid, __u32 level)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		if((bsp_disp_feat_get_layer_feats(screen_id, DISP_LAYER_WORK_MODE_SCALER, layer_man->scaler_index) & DISP_LAYER_FEAT_DETAIL_ENHANCE)) {
			gdisp.scaler[layer_man->scaler_index].deu.while_exten_level = level;
			if(gdisp.scaler[layer_man->scaler_index].deu.enable) {
				IEP_Deu_Set_White_Level_Extension(layer_man->scaler_index,level);
			}

			return DIS_SUCCESS;
		}
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_deu_get_white_exten_level(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		return gdisp.scaler[layer_man->scaler_index].deu.while_exten_level;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_deu_set_black_exten_level(__u32 screen_id, __u32 hid, __u32 level)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		if((bsp_disp_feat_get_layer_feats(screen_id, DISP_LAYER_WORK_MODE_SCALER, layer_man->scaler_index) & DISP_LAYER_FEAT_DETAIL_ENHANCE)) {
			gdisp.scaler[layer_man->scaler_index].deu.black_exten_level = level;
			if(gdisp.scaler[layer_man->scaler_index].deu.enable) {
				IEP_Deu_Set_Black_Level_Extension(layer_man->scaler_index,level);
			}
			return DIS_SUCCESS;
		}
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_deu_get_black_exten_level(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		return gdisp.scaler[layer_man->scaler_index].deu.black_exten_level;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_deu_set_window(__u32 screen_id, __u32 hid, __disp_rect_t *rect)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((bsp_disp_feat_get_layer_feats(screen_id, DISP_LAYER_WORK_MODE_SCALER, layer_man->scaler_index) & DISP_LAYER_FEAT_DETAIL_ENHANCE)) {
		if((rect->width == 0) || (rect->height == 0)) {
			bsp_disp_layer_get_screen_window(screen_id,IDTOHAND(hid),rect);
		}
		memcpy(&gdisp.scaler[layer_man->scaler_index].deu.rect, rect, sizeof(__disp_rect_t));
		if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))	{
			IEP_Deu_Set_Winodw(layer_man->scaler_index,rect);

			return DIS_SUCCESS;
		}
	}
	return DIS_NOT_SUPPORT;
}


__s32 bsp_disp_deu_get_window(__u32 screen_id, __u32 hid, __disp_rect_t *rect)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((bsp_disp_feat_get_layer_feats(screen_id, DISP_LAYER_WORK_MODE_SCALER, layer_man->scaler_index) & DISP_LAYER_FEAT_DETAIL_ENHANCE)) {
		if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
			memcpy(rect, &gdisp.scaler[layer_man->scaler_index].deu.rect, sizeof(__disp_rect_t));
			return DIS_SUCCESS;
		}
	}
	return DIS_NOT_SUPPORT;
}

__s32 disp_deu_set_frame_info(__u32 screen_id, __u32 hid)
{
	static __disp_frame_info_t frame_info;
	__disp_scaler_t * scaler;
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__u32 scaler_index;
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	scaler_index = gdisp.screen[screen_id].layer_manage[hid].scaler_index;
	scaler = &(gdisp.scaler[scaler_index]);

	if(!(bsp_disp_feat_get_layer_feats(screen_id, DISP_LAYER_WORK_MODE_SCALER,
	    layer_man->scaler_index) & DISP_LAYER_FEAT_DETAIL_ENHANCE)) {
	    return DIS_NOT_SUPPORT;

	}

	if(scaler->deu.enable) {
		in_size.src_width = scaler->in_fb.size.width;
		in_size.src_height = scaler->in_fb.size.height;
		in_size.x_off = scaler->src_win.x;
		in_size.y_off = scaler->src_win.y;
		in_size.scal_width = scaler->src_win.width;
		in_size.scal_height = scaler->src_win.height;

		out_size.width = scaler->out_size.width;
		out_size.height = scaler->out_size.height;

		frame_info.b_interlace_out = 0;
		frame_info.b_trd_out = scaler->b_trd_out;
		frame_info.trd_out_mode = scaler->out_trd_mode;
		frame_info.csc_mode =  scaler->in_fb.cs_mode;

		frame_info.disp_size.width = out_size.width;
		frame_info.disp_size.height = out_size.height;

		if(scaler->in_fb.b_trd_src) {
			__scal_3d_inmode_t inmode;
			__scal_3d_outmode_t outmode = 0;

			inmode = Scaler_3d_sw_para_to_reg(0, scaler->in_fb.trd_mode, 0);
			outmode = Scaler_3d_sw_para_to_reg(1, scaler->out_trd_mode, frame_info.b_interlace_out);

			DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
			if(scaler->b_trd_out)
			{
			  DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
			}
		}

		memcpy(&frame_info.disp_size, &scaler->out_size, sizeof(__disp_rectsz_t));
		frame_info.in_size.width = in_size.scal_width;
		frame_info.in_size.height = in_size.scal_height;
		if((frame_info.out_size.width != out_size.width) || (frame_info.out_size.height != out_size.height)) {
			//pr_warn("out_size=<%dx%d>\n", out_size.width, out_size.height);
		}
		frame_info.out_size.width = out_size.width;
		frame_info.out_size.height = out_size.height;

		IEP_Deu_Set_frameinfo(scaler_index,frame_info);

		if((scaler->deu.rect.width == 0) || (scaler->deu.rect.height == 0)
		    || (scaler->deu.rect.width == 1) || (scaler->deu.rect.height == 1))	{
			//pr_warn("deu.rest.size=<%dx%d>\n", scaler->deu.rect.width, scaler->deu.rect.height);
		}

		if((scaler->out_size.width == 0) || (scaler->out_size.height == 0)
		    || (scaler->out_size.width == 1) || (scaler->out_size.height == 1))	{
			//pr_warn("scaler.outsize=<%dx%d>\n", scaler->out_size.width, scaler->out_size.height);
		}

		if((scaler->deu.rect.width == 0) || (scaler->deu.rect.height == 0)
		    || (scaler->deu.rect.width == 1) || (scaler->deu.rect.height == 1))	{
			scaler->deu.rect.x = 0;
			scaler->deu.rect.y = 0;
			scaler->deu.rect.width = scaler->out_size.width;
			scaler->deu.rect.height = scaler->out_size.height;
		}

		IEP_Deu_Set_Winodw(scaler_index,&scaler->deu.rect);
	}

	return DIS_SUCCESS;
}


__s32 disp_deu_clear(__u32 screen_id, __u32 hid)
{
	__u32 scaler_index;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	scaler_index = gdisp.screen[screen_id].layer_manage[hid].scaler_index;

	memset(&gdisp.scaler[scaler_index].deu, 0, sizeof(__iep_deu_t));
	gdisp.scaler[scaler_index].deu.luma_sharpe_level = 2;

	return DIS_SUCCESS;
}

__s32 disp_deu_output_select(__u32 screen_id, __u32 hid, __u32 ch)
{
	__u32 scaler_index;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	scaler_index = gdisp.screen[screen_id].layer_manage[hid].scaler_index;
	if((bsp_disp_feat_get_layer_feats(screen_id, DISP_LAYER_WORK_MODE_SCALER, scaler_index) & DISP_LAYER_FEAT_DETAIL_ENHANCE)) {
	  IEP_Deu_Output_Select(scaler_index, ch);
	}

	return DIS_SUCCESS;
}

__s32 disp_deu_get_support(__u32 screen_id)
{
	return bsp_disp_feat_get_image_detail_enhance_support(screen_id);
}


#define ____SEPARATOR_CMU____


static __s32 __disp_cmu_get_adjust_value(__disp_enhance_mode_t mode, __u32 value)
{
	/* adjust to 30percent */
	if(mode != DISP_ENHANCE_MODE_STANDARD) {
		if(value > 50) {
			value = 50 + (value-50)*10/50;
		}	else {
			value = 50 - (50-value)*10/50;
		}
	}

	return value;
}
__s32 bsp_disp_cmu_layer_enable(__u32 screen_id,__u32 hid, __bool en)
{
	__layer_man_t * layer_man;
	__u32 layer_bright, layer_saturation, layer_hue, layer_mode;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		//pr_warn("bsp_disp_cmu_layer_enable, screen_id=%d,hid=%d,en=%d\n", screen_id, hid, en);
		if(en && (!(gdisp.screen[screen_id].cmu.status & CMU_LAYER_EN))) {
			layer_mode = gdisp.screen[screen_id].cmu.layer_mode;
			layer_bright = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_bright);
			layer_saturation = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_saturation);
			layer_hue = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_hue);
			bsp_disp_cfg_start(screen_id);
			IEP_CMU_Set_Imgsize(screen_id,layer_man->para.scn_win.width, layer_man->para.scn_win.height);
			IEP_CMU_Set_Par(screen_id, layer_hue, layer_saturation, layer_bright, layer_mode);
			IEP_CMU_Set_Data_Flow(screen_id,layer_man->scaler_index+1);//fe0 : 1, fe1 :2
			IEP_CMU_Set_Window(screen_id,&gdisp.screen[screen_id].cmu.layer_rect);
			IEP_CMU_Enable(screen_id, TRUE);
			bsp_disp_cfg_finish(screen_id);
			gdisp.screen[screen_id].cmu.status |= CMU_LAYER_EN;
		} else if((!en) && (gdisp.screen[screen_id].cmu.status & CMU_LAYER_EN))	{
			IEP_CMU_Enable(screen_id, FALSE);
			gdisp.screen[screen_id].cmu.status &= CMU_LAYER_EN_MASK;
		}
		return DIS_SUCCESS;
	}

	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_layer_get_enable(__u32 screen_id,__u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		__s32 ret;

		ret = (gdisp.screen[screen_id].cmu.status & CMU_LAYER_EN)? 1:0;
		return ret;
	}

	return DIS_NOT_SUPPORT;
}

__s32 disp_cmu_layer_clear(__u32 screen_id)
{
	gdisp.screen[screen_id].cmu.layer_mode = DISP_ENHANCE_MODE_STANDARD;
	memset(&gdisp.screen[screen_id].cmu.layer_rect, 0,sizeof(__disp_rect_t));
	gdisp.screen[screen_id].cmu.layer_bright = 50;
	gdisp.screen[screen_id].cmu.layer_saturation = 50;
	gdisp.screen[screen_id].cmu.layer_contrast = 50;
	gdisp.screen[screen_id].cmu.layer_hue = 50;

	return DIS_SUCCESS;
}

__s32 bsp_disp_cmu_layer_set_window(__u32 screen_id, __u32 hid, __disp_rect_t *rect)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
		if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		memcpy(&gdisp.screen[screen_id].cmu.layer_rect, rect, sizeof(__disp_rect_t));
		if(gdisp.screen[screen_id].cmu.status & CMU_LAYER_EN)	{
			IEP_CMU_Set_Window(screen_id,&gdisp.screen[screen_id].cmu.layer_rect);
		}
		return DIS_SUCCESS;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_layer_get_window(__u32 screen_id, __u32 hid, __disp_rect_t *rect)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		memcpy(rect, &gdisp.screen[screen_id].cmu.layer_rect, sizeof(__disp_rect_t));
		return DIS_SUCCESS;
	}
	return DIS_NOT_SUPPORT;
}



__s32 bsp_disp_cmu_layer_set_bright(__u32 screen_id, __u32 hid, __u32 bright)
{
	__layer_man_t * layer_man;
	__u32 layer_bright, layer_saturation, layer_hue, layer_mode;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))	{
		gdisp.screen[screen_id].cmu.layer_bright = bright;
		layer_mode = gdisp.screen[screen_id].cmu.layer_mode;
		layer_bright = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_bright);
		layer_saturation = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_saturation);
		layer_hue = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_hue);
		if(gdisp.screen[screen_id].cmu.status & CMU_LAYER_EN)	{
			IEP_CMU_Set_Par(screen_id, layer_hue, layer_saturation, layer_bright, layer_mode);
		}
		return DIS_SUCCESS;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_layer_get_bright(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		return gdisp.screen[screen_id].cmu.layer_bright;
	}
	return DIS_NOT_SUPPORT;
}


__s32 bsp_disp_cmu_layer_set_saturation(__u32 screen_id, __u32 hid, __u32 saturation)
{
    __layer_man_t * layer_man;
	__u32 layer_bright, layer_saturation, layer_hue,layer_mode;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		gdisp.screen[screen_id].cmu.layer_saturation= saturation;
		layer_mode = gdisp.screen[screen_id].cmu.layer_mode;
		layer_bright = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_bright);
		layer_saturation = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_saturation);
		layer_hue = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_hue);
		if(gdisp.screen[screen_id].cmu.status & CMU_LAYER_EN)	{
			IEP_CMU_Set_Par(screen_id, layer_hue, layer_saturation, layer_bright,layer_mode);
		}
		return DIS_SUCCESS;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_layer_get_saturation(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))	{
		return gdisp.screen[screen_id].cmu.layer_saturation;
	}
	return DIS_NOT_SUPPORT;
}


__s32 bsp_disp_cmu_layer_set_hue(__u32 screen_id, __u32 hid, __u32 hue)
{
	__layer_man_t * layer_man;
	__u32 layer_bright, layer_saturation, layer_hue,layer_mode;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		gdisp.screen[screen_id].cmu.layer_hue = hue;
		layer_mode = gdisp.screen[screen_id].cmu.layer_mode;
		layer_bright = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_bright);
		layer_saturation = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_saturation);
		layer_hue = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_hue);
		if(gdisp.screen[screen_id].cmu.status & CMU_LAYER_EN)	{
			IEP_CMU_Set_Par(screen_id, layer_hue, layer_saturation, layer_bright, layer_mode);
		}
		return DIS_SUCCESS;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_layer_get_hue(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))	{
		return gdisp.screen[screen_id].cmu.layer_hue;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_layer_set_contrast(__u32 screen_id, __u32 hid, __u32 contrast)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER)) {
		gdisp.screen[screen_id].cmu.layer_contrast = contrast;
		return DIS_SUCCESS;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_layer_get_contrast(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))	{
		return gdisp.screen[screen_id].cmu.layer_contrast;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_layer_set_mode(__u32 screen_id, __u32 hid, __u32 mode)
{
	__layer_man_t * layer_man;
	__u32 layer_bright, layer_saturation, layer_hue,layer_mode;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))	{
		gdisp.screen[screen_id].cmu.layer_mode = mode;
		layer_mode = gdisp.screen[screen_id].cmu.layer_mode;
		layer_bright = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_bright);
		layer_saturation = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_saturation);
		layer_hue = __disp_cmu_get_adjust_value(layer_mode, gdisp.screen[screen_id].cmu.layer_hue);
		if(gdisp.screen[screen_id].cmu.status & CMU_LAYER_EN)	{
			IEP_CMU_Set_Par(screen_id, layer_hue, layer_saturation, layer_bright, layer_mode);
		}
		return DIS_SUCCESS;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_layer_get_mode(__u32 screen_id, __u32 hid)
{
	__layer_man_t * layer_man;

	hid= HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER))	{
		return gdisp.screen[screen_id].cmu.layer_mode;
	}
	return DIS_NOT_SUPPORT;
}

__s32 bsp_disp_cmu_enable(__u32 screen_id,__bool en)
{
	__u32 screen_bright, screen_saturation, screen_hue, screen_mode;

	//pr_warn("bsp_disp_cmu_enable, screen_id=%d,en=%d\n", screen_id, en);

	if((en) && (!(gdisp.screen[screen_id].cmu.status & CMU_SCREEN_EN))) {
		screen_mode = gdisp.screen[screen_id].cmu.screen_mode;
		screen_bright = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_bright);
		screen_saturation = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_saturation);
		screen_hue = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_hue);
		IEP_CMU_Set_Imgsize(screen_id,bsp_disp_get_screen_width(screen_id), bsp_disp_get_screen_height(screen_id));
		IEP_CMU_Set_Par(screen_id, screen_hue, screen_saturation, screen_bright, screen_mode);
		IEP_CMU_Set_Data_Flow(screen_id,0);
		IEP_CMU_Set_Window(screen_id,&gdisp.screen[screen_id].cmu.screen_rect);
		IEP_CMU_Enable(screen_id, TRUE);
		gdisp.screen[screen_id].cmu.status |= CMU_SCREEN_EN;
	}	else if((!en) && (gdisp.screen[screen_id].cmu.status & CMU_SCREEN_EN)) {
		IEP_CMU_Enable(screen_id, FALSE);
		gdisp.screen[screen_id].cmu.status &= CMU_SCREEN_EN_MASK;
	}

	return DIS_SUCCESS;
}

__s32 bsp_disp_cmu_get_enable(__u32 screen_id)
{
	__u32 ret;

	ret = (gdisp.screen[screen_id].cmu.status & CMU_SCREEN_EN)? 1:0;
	return ret;
}

__s32 bsp_disp_cmu_set_window(__u32 screen_id, __disp_rect_t *rect)
{
	memcpy(&gdisp.screen[screen_id].cmu.screen_rect, rect, sizeof(__disp_rect_t));
	if(gdisp.screen[screen_id].cmu.status & CMU_SCREEN_EN) {
		IEP_CMU_Set_Window(screen_id,&gdisp.screen[screen_id].cmu.screen_rect);
	}
	return DIS_SUCCESS;
}

__s32 bsp_disp_cmu_get_window(__u32 screen_id, __disp_rect_t *rect)
{
	memcpy(rect, &gdisp.screen[screen_id].cmu.screen_rect, sizeof(__disp_rect_t));
	return DIS_SUCCESS;
}


__s32 bsp_disp_cmu_set_bright(__u32 screen_id, __u32 bright)
{
	__u32 screen_bright, screen_saturation, screen_hue, screen_mode;

	gdisp.screen[screen_id].cmu.screen_bright = bright;
	if(gdisp.screen[screen_id].cmu.status & CMU_SCREEN_EN) {
		screen_mode = gdisp.screen[screen_id].cmu.screen_mode;
		screen_bright = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_bright);
		screen_saturation = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_saturation);
		screen_hue = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_hue);
		IEP_CMU_Set_Par(screen_id, screen_hue, screen_saturation, screen_bright, screen_mode);
	}
	return DIS_SUCCESS;
}

__s32 bsp_disp_cmu_get_bright(__u32 screen_id)
{
	return gdisp.screen[screen_id].cmu.screen_bright;
}


__s32 bsp_disp_cmu_set_saturation(__u32 screen_id,__u32 saturation)
{
	__u32 screen_bright, screen_saturation, screen_hue, screen_mode;

	gdisp.screen[screen_id].cmu.screen_saturation= saturation;
	if(gdisp.screen[screen_id].cmu.status & CMU_SCREEN_EN) {
		screen_mode = gdisp.screen[screen_id].cmu.screen_mode;
		screen_bright = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_bright);
		screen_saturation = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_saturation);
		screen_hue = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_hue);
		IEP_CMU_Set_Par(screen_id, screen_hue, screen_saturation, screen_bright, screen_mode);
	}
	return DIS_SUCCESS;
}

__s32 bsp_disp_cmu_get_saturation(__u32 screen_id)
{
	return gdisp.screen[screen_id].cmu.screen_saturation;
}

__s32 bsp_disp_cmu_set_hue(__u32 screen_id, __u32 hue)
{
	__u32 screen_bright, screen_saturation, screen_hue, screen_mode;

	gdisp.screen[screen_id].cmu.screen_hue = hue;
	if(gdisp.screen[screen_id].cmu.status & CMU_SCREEN_EN) {
		screen_mode = gdisp.screen[screen_id].cmu.screen_mode;
		screen_bright = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_bright);
		screen_saturation = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_saturation);
		screen_hue = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_hue);
		IEP_CMU_Set_Par(screen_id, screen_hue, screen_saturation, screen_bright, screen_mode);
	}
	return DIS_SUCCESS;
}

__s32 bsp_disp_cmu_get_hue(__u32 screen_id)
{
	return gdisp.screen[screen_id].cmu.screen_hue;
}



__s32 bsp_disp_cmu_set_contrast(__u32 screen_id,__u32 contrast)
{
	gdisp.screen[screen_id].cmu.screen_contrast = contrast;
	return DIS_SUCCESS;
}

__s32 bsp_disp_cmu_get_contrast(__u32 screen_id)
{
	return gdisp.screen[screen_id].cmu.screen_contrast;
}

__s32 bsp_disp_cmu_set_mode(__u32 screen_id,__u32 mode)
{
	__u32 screen_bright, screen_saturation, screen_hue, screen_mode;

	gdisp.screen[screen_id].cmu.screen_mode= mode;
	if(gdisp.screen[screen_id].cmu.status & CMU_SCREEN_EN) {
		screen_mode = gdisp.screen[screen_id].cmu.screen_mode;
		screen_bright = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_bright);
		screen_saturation = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_saturation);
		screen_hue = __disp_cmu_get_adjust_value(screen_mode, gdisp.screen[screen_id].cmu.screen_hue);
		IEP_CMU_Set_Par(screen_id, screen_hue, screen_saturation, screen_bright, screen_mode);
	}
	return DIS_SUCCESS;
}

__s32 bsp_disp_cmu_get_mode(__u32 screen_id)
{
	return gdisp.screen[screen_id].cmu.screen_mode;
}



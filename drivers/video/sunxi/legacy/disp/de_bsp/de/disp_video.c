
#include "disp_video.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_scaler.h"
#include "disp_de.h"
#include "disp_clk.h"

frame_para_t g_video[2][4];
static __u32 maf_flag_mem_len = 2048*2/8*544*2;
static void *maf_flag_mem[2][2];//2048*2/8 *544   * 2
static dit_mode_t dit_mode_default[2];

#define VIDEO_TIME_LEN 100
static unsigned long video_time_index[2][4] = {{0,0}};
static unsigned long video_time[2][4][VIDEO_TIME_LEN];//jiffies

__s32 disp_video_checkin(__u32 screen_id, __u32 id);

#if 1
static __s32 video_enhancement_start(__u32 screen_id, __u32 id)
{
	/* assume open HDMI before video start */
	if(gdisp.screen[screen_id].output_type == DISP_OUTPUT_TYPE_LCD) {
		bsp_disp_deu_enable(screen_id,IDTOHAND(id),TRUE);
		gdisp.screen[screen_id].layer_manage[id].video_enhancement_en = 1;
	}

	return 0;
}

static __s32 video_enhancement_stop(__u32 screen_id, __u32 id)
{
	if(gdisp.screen[screen_id].layer_manage[id].video_enhancement_en) {
		bsp_disp_deu_enable(screen_id,IDTOHAND(id),FALSE);
		gdisp.screen[screen_id].layer_manage[id].video_enhancement_en = 0;
	}

	return 0;
}
#endif

static __inline __s32 Hal_Set_Frame(__u32 screen_id, __u32 tcon_index, __u32 id)
{
	__u32 cur_line = 0, start_delay = 0;
#ifdef __LINUX_OSAL__
	unsigned long flags;
#endif

	cur_line = tcon_get_cur_line(screen_id, tcon_index);
	start_delay = tcon_get_start_delay(screen_id, tcon_index);
	if(cur_line > start_delay-5) {
		//DE_INF("cur_line(%d) >= start_delay(%d)-3 in Hal_Set_Frame\n", cur_line, start_delay);
		return DIS_FAIL;
	}
#ifdef __LINUX_OSAL__
	spin_lock_irqsave(&g_video[screen_id][id].flag_lock, flags);
#endif
	if(g_video[screen_id][id].display_cnt == 0) {
		g_video[screen_id][id].pre_frame_addr_luma = g_video[screen_id][id].video_cur.addr[0];
		g_video[screen_id][id].pre_frame_addr_chroma= g_video[screen_id][id].video_cur.addr[1];
		memcpy(&g_video[screen_id][id].video_cur, &g_video[screen_id][id].video_new, sizeof(__disp_video_fb_t));
		g_video[screen_id][id].cur_maf_flag_addr ^=  g_video[screen_id][id].pre_maf_flag_addr;
		g_video[screen_id][id].pre_maf_flag_addr ^=  g_video[screen_id][id].cur_maf_flag_addr;
		g_video[screen_id][id].cur_maf_flag_addr ^=  g_video[screen_id][id].pre_maf_flag_addr;
		disp_video_checkin(screen_id, id);
	}
	g_video[screen_id][id].display_cnt++;
#ifdef __LINUX_OSAL__
	spin_unlock_irqrestore(&g_video[screen_id][id].flag_lock, flags);
#endif

	/* scaler layer */
	if(gdisp.screen[screen_id].layer_manage[id].para.mode == DISP_LAYER_WORK_MODE_SCALER) {
		__u32 scaler_index;
		__scal_buf_addr_t scal_addr;
		__scal_src_size_t in_size;
		__scal_out_size_t out_size;
		__scal_src_type_t in_type;
		__scal_out_type_t out_type;
		__scal_scan_mod_t in_scan;
		__scal_scan_mod_t out_scan;
		__disp_scaler_t * scaler;
		__u32 pre_frame_addr_luma = 0, pre_frame_addr_chroma = 0;
		__u32 maf_linestride = 0;
		__u32 size;

		scaler_index = gdisp.screen[screen_id].layer_manage[id].scaler_index;

		scaler = &(gdisp.scaler[scaler_index]);

		if(g_video[screen_id][id].video_cur.interlace == TRUE) {
			/* todo , full size of 3d mode < 1920 */
			if((!(gdisp.screen[screen_id].de_flicker_status & DE_FLICKER_USED)) &&
			    (scaler->in_fb.format == DISP_FORMAT_YUV420 && scaler->in_fb.mode == DISP_MOD_MB_UV_COMBINED)
			    && (dit_mode_default[scaler_index] != 0xff) && (scaler->in_fb.size.width < 1920)) {
				g_video[screen_id][id].dit_enable = TRUE;
				if(!(bsp_disp_feat_get_layer_feats(scaler->screen_index, DISP_LAYER_WORK_MODE_SCALER, scaler_index)
				    & DISP_LAYER_FEAT_DE_INTERLACE)) {
					g_video[screen_id][id].dit_enable = FALSE;
				}
			} else {
				g_video[screen_id][id].dit_enable = FALSE;
			}

			g_video[screen_id][id].fetch_field = FALSE;
			if(g_video[screen_id][id].display_cnt == 0) {
				g_video[screen_id][id].fetch_bot = (g_video[screen_id][id].video_cur.top_field_first)?0:1;
			}	else {
				g_video[screen_id][id].fetch_bot = (g_video[screen_id][id].video_cur.top_field_first)?1:0;
			}

			if(g_video[screen_id][id].dit_enable == TRUE) {
				g_video[screen_id][id].dit_mode = dit_mode_default[scaler_index];
				maf_linestride = (((scaler->src_win.width + 31) & 	0xffffffe0)*2/8 + 31) & 0xffffffe0;
				// //g_video[screen_id][id].video_cur.flag_stride;//todo? ( (£¨720 + 31£©&0xffffffe0 ) * 2/8  + 31) & 0xffffffe0

				if(g_video[screen_id][id].video_cur.pre_frame_valid == TRUE) {
					g_video[screen_id][id].tempdiff_en = TRUE;
					pre_frame_addr_luma= (__u32)OSAL_VAtoPA((void*)g_video[screen_id][id].pre_frame_addr_luma);
					pre_frame_addr_chroma= (__u32)OSAL_VAtoPA((void*)g_video[screen_id][id].pre_frame_addr_chroma);
				}	else {
					g_video[screen_id][id].tempdiff_en = FALSE;
				}
				g_video[screen_id][id].diagintp_en = TRUE;
			}
			else
			{
				g_video[screen_id][id].dit_mode = DIT_MODE_WEAVE;
				g_video[screen_id][id].tempdiff_en = FALSE;
				g_video[screen_id][id].diagintp_en = FALSE;
			}
		}	else {
			g_video[screen_id][id].dit_enable = FALSE;
			g_video[screen_id][id].fetch_field = FALSE;
			g_video[screen_id][id].fetch_bot = FALSE;
			g_video[screen_id][id].dit_mode = DIT_MODE_WEAVE;
			g_video[screen_id][id].tempdiff_en = FALSE;
			g_video[screen_id][id].diagintp_en = FALSE;
		}

		in_type.fmt= Scaler_sw_para_to_reg(0,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
		in_type.mod= Scaler_sw_para_to_reg(1,scaler->in_fb.mode, scaler->in_fb.format, scaler->in_fb.seq);
		in_type.ps= Scaler_sw_para_to_reg(2,scaler->in_fb.mode, scaler->in_fb.format, (__u8)scaler->in_fb.seq);
		in_type.byte_seq = 0;
		in_type.sample_method = 0;

		scal_addr.ch0_addr= (__u32)OSAL_VAtoPA((void*)(g_video[screen_id][id].video_cur.addr[0]));
		scal_addr.ch1_addr= (__u32)OSAL_VAtoPA((void*)(g_video[screen_id][id].video_cur.addr[1]));
		scal_addr.ch2_addr= (__u32)OSAL_VAtoPA((void*)(g_video[screen_id][id].video_cur.addr[2]));

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

		in_scan.field = g_video[screen_id][id].fetch_field;
		in_scan.bottom = g_video[screen_id][id].fetch_bot;

		out_scan.field = (gdisp.screen[screen_id].de_flicker_status & DE_FLICKER_USED)?0: gdisp.screen[screen_id].b_out_interlace;

		if(scaler->out_fb.cs_mode > DISP_VXYCC)	{
			scaler->out_fb.cs_mode = DISP_BT601;
		}

		/* 3d source */
		if(scaler->in_fb.b_trd_src)	{
			__scal_3d_inmode_t inmode;
			__scal_3d_outmode_t outmode = 0;
			__scal_buf_addr_t scal_addr_right;

			/* if supported 3d display */
			if(bsp_disp_feat_get_layer_feats(scaler->screen_index, DISP_LAYER_WORK_MODE_SCALER, scaler_index) & DISP_LAYER_FEAT_3D) {
				inmode = Scaler_3d_sw_para_to_reg(0, scaler->in_fb.trd_mode, 0);
				outmode = Scaler_3d_sw_para_to_reg(1, scaler->out_trd_mode, gdisp.screen[screen_id].b_out_interlace);

				DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
				if(scaler->b_trd_out)	{
					DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
				}

				scal_addr_right.ch0_addr= (__u32)OSAL_VAtoPA((void*)(g_video[screen_id][id].video_cur.addr_right[0]));
				scal_addr_right.ch1_addr= (__u32)OSAL_VAtoPA((void*)(g_video[screen_id][id].video_cur.addr_right[1]));
				scal_addr_right.ch2_addr= (__u32)OSAL_VAtoPA((void*)(g_video[screen_id][id].video_cur.addr_right[2]));

				DE_SCAL_Set_3D_Ctrl(scaler_index, scaler->b_trd_out, inmode, outmode);
				DE_SCAL_Config_3D_Src(scaler_index, &scal_addr, &in_size, &in_type, inmode, &scal_addr_right);
				DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, scaler->b_trd_out, outmode);
			}	else {
				DE_WRN("This platform not support 3d output!\n");
				DE_SCAL_Config_Src(scaler_index,&scal_addr,&in_size,&in_type,FALSE,FALSE);
				DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, 0, 0);
			}
		}	else {
			/* 2d source */
			DE_SCAL_Config_Src(scaler_index,&scal_addr,&in_size,&in_type,FALSE,FALSE);
			DE_SCAL_Agth_Config(scaler_index, &in_type, &in_size, &out_size, 0, 0, 0);
		}

		DE_SCAL_Set_Init_Phase(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, g_video[screen_id][id].dit_enable);
		DE_SCAL_Set_Scaling_Factor(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
		//DE_SCAL_Set_Scaling_Coef_for_video(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, 0x00000101);
		DE_SCAL_Set_Scaling_Coef(scaler_index, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, scaler->smooth_mode);
		DE_SCAL_Set_Out_Size(scaler_index, &out_scan,&out_type, &out_size);
		DE_SCAL_Set_Di_Ctrl(scaler_index,g_video[screen_id][id].dit_enable,g_video[screen_id][id].dit_mode,g_video[screen_id][id].diagintp_en,g_video[screen_id][id].tempdiff_en);
		DE_SCAL_Set_Di_PreFrame_Addr(scaler_index, pre_frame_addr_luma, pre_frame_addr_chroma);
		DE_SCAL_Set_Di_MafFlag_Src(scaler_index, 
		    OSAL_VAtoPA((void*)g_video[screen_id][id].cur_maf_flag_addr),
		    OSAL_VAtoPA((void*)g_video[screen_id][id].pre_maf_flag_addr), maf_linestride);

		if(g_video[screen_id][id].display_cnt == 0) {
			size = (scaler->in_fb.size.width * scaler->src_win.height * de_format_to_bpp(scaler->in_fb.format) + 7)/8;
			//OSAL_CacheRangeFlush((void *)scal_addr.ch0_addr,size ,CACHE_CLEAN_FLUSH_D_CACHE_REGION);
		}

		DE_SCAL_Set_Reg_Rdy(scaler_index);
	}
	/* normal layer */
	else {
		__layer_man_t * layer_man;
		__disp_fb_t fb;
		layer_src_t layer_fb;

		memset(&layer_fb, 0, sizeof(layer_src_t));
		layer_man = &gdisp.screen[screen_id].layer_manage[id];

		bsp_disp_layer_get_framebuffer(screen_id, id, &fb);
		fb.addr[0] = (__u32)OSAL_VAtoPA((void*)(g_video[screen_id][id].video_cur.addr[0]));
		fb.addr[1] = (__u32)OSAL_VAtoPA((void*)(g_video[screen_id][id].video_cur.addr[1]));
		fb.addr[2] = (__u32)OSAL_VAtoPA((void*)(g_video[screen_id][id].video_cur.addr[2]));

		if(get_fb_type(fb.format) == DISP_FB_TYPE_YUV) {
			Yuv_Channel_adjusting(screen_id , fb.mode, fb.format, &layer_man->para.src_win.x, &layer_man->para.scn_win.width);
			Yuv_Channel_Set_framebuffer(screen_id, &fb, layer_man->para.src_win.x, layer_man->para.src_win.y);
		}	else {
			layer_fb.fb_addr    = (__u32)OSAL_VAtoPA((void*)fb.addr[0]);
			layer_fb.pixseq     = img_sw_para_to_reg(3,0,fb.seq);
			layer_fb.br_swap    = fb.br_swap;
			layer_fb.fb_width   = fb.size.width;
			layer_fb.offset_x   = layer_man->para.src_win.x;
			layer_fb.offset_y   = layer_man->para.src_win.y;
			layer_fb.format = fb.format;
			DE_BE_Layer_Set_Framebuffer(screen_id, id,&layer_fb);
		}
	}

	gdisp.screen[screen_id].layer_manage[id].para.fb.addr[0] = g_video[screen_id][id].video_cur.addr[0];
	gdisp.screen[screen_id].layer_manage[id].para.fb.addr[1] = g_video[screen_id][id].video_cur.addr[1];
	gdisp.screen[screen_id].layer_manage[id].para.fb.addr[2] = g_video[screen_id][id].video_cur.addr[2];
	gdisp.screen[screen_id].layer_manage[id].para.fb.trd_right_addr[0] = g_video[screen_id][id].video_cur.addr_right[0];
	gdisp.screen[screen_id].layer_manage[id].para.fb.trd_right_addr[1] = g_video[screen_id][id].video_cur.addr_right[1];
	gdisp.screen[screen_id].layer_manage[id].para.fb.trd_right_addr[2] = g_video[screen_id][id].video_cur.addr_right[2];

	return DIS_SUCCESS;
}


__s32 Video_Operation_In_Vblanking(__u32 screen_id, __u32 tcon_index)
{
	__u32 id=0;
	__u32 num_layers;

	num_layers = bsp_disp_feat_get_num_layers(screen_id);

	for(id = 0; id<num_layers; id++) {
		if((g_video[screen_id][id].enable == TRUE) && (g_video[screen_id][id].have_got_frame == TRUE)) {
			Hal_Set_Frame(screen_id, tcon_index, id);
		}
	}

	return DIS_SUCCESS;
}

__s32 bsp_disp_video_set_fb(__u32 screen_id, __u32 hid, __disp_video_fb_t *in_addr)
{
#ifdef __LINUX_OSAL__
	unsigned long flags;
#endif
	hid = HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	if(g_video[screen_id][hid].enable) {
		memcpy(&g_video[screen_id][hid].video_new, in_addr, sizeof(__disp_video_fb_t));
		g_video[screen_id][hid].have_got_frame = TRUE;
#ifdef __LINUX_OSAL__
			spin_lock_irqsave(&g_video[screen_id][hid].flag_lock, flags);
#endif
			g_video[screen_id][hid].display_cnt = 0;
#ifdef __LINUX_OSAL__
			spin_unlock_irqrestore(&g_video[screen_id][hid].flag_lock, flags);
#endif

		return DIS_SUCCESS;
	}	else {
		return DIS_FAIL;
	}
}


__s32 bsp_disp_video_get_frame_id(__u32 screen_id, __u32 hid)//get the current displaying frame id
{
	hid = HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	if(g_video[screen_id][hid].enable) {
		if(g_video[screen_id][hid].have_got_frame == TRUE) {
			return g_video[screen_id][hid].video_cur.id;
		}	else {
			return DIS_FAIL;
		}
	}	else {
		return DIS_FAIL;
	}
}

__s32 bsp_disp_video_get_dit_info(__u32 screen_id, __u32 hid, __disp_dit_info_t * dit_info)
{
	hid = HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	if(g_video[screen_id][hid].enable) {
		dit_info->maf_enable = FALSE;
		dit_info->pre_frame_enable = FALSE;

		if(g_video[screen_id][hid].dit_enable) {
			if(g_video[screen_id][hid].dit_mode == DIT_MODE_MAF) {
				dit_info->maf_enable = TRUE;
			}
			if(g_video[screen_id][hid].tempdiff_en) {
				dit_info->pre_frame_enable = TRUE;
			}
		}
		return DIS_SUCCESS;
	}	else {
		return DIS_FAIL;
	}
}

__s32 bsp_disp_video_start(__u32 screen_id, __u32 hid)
{
	__layer_man_t *layer_man;

	hid = HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];
	if((layer_man->status & LAYER_USED) && (!g_video[screen_id][hid].enable)) {
		memset(&g_video[screen_id][hid], 0, sizeof(frame_para_t));
		g_video[screen_id][hid].video_cur.id = -1;
		g_video[screen_id][hid].enable = TRUE;

		if(layer_man->para.mode == DISP_LAYER_WORK_MODE_SCALER) {
			g_video[screen_id][hid].cur_maf_flag_addr = (__u32)maf_flag_mem[layer_man->scaler_index][0];
			g_video[screen_id][hid].pre_maf_flag_addr = (__u32)maf_flag_mem[layer_man->scaler_index][1];
		}
		disp_drc_start_video_mode(screen_id);
		video_enhancement_start(screen_id,hid);

		return DIS_SUCCESS;
	}	else {
		return DIS_FAIL;
	}
}

__s32 bsp_disp_video_stop(__u32 screen_id, __u32 hid)
{
	hid = HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	if(g_video[screen_id][hid].enable) {
		memset(&g_video[screen_id][hid], 0, sizeof(frame_para_t));

		disp_drc_start_ui_mode(screen_id);
		video_enhancement_stop(screen_id,hid);
		return DIS_SUCCESS;
	}	else {
		return DIS_FAIL;
	}
}

__s32 bsp_disp_video_get_start(__u32 screen_id, __u32 hid)
{
	__layer_man_t *layer_man;

	hid = HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	layer_man = &gdisp.screen[screen_id].layer_manage[hid];

	return ((layer_man->status & LAYER_USED) && (g_video[screen_id][hid].enable));
}

__s32 disp_video_init(void)
{
	__u32 num_screens;
	__u32 num_scalers;
	__u32 scaler_id;

	num_screens = bsp_disp_feat_get_num_screens();
	num_scalers = bsp_disp_feat_get_num_scalers();
	memset(g_video,0,sizeof(g_video));
#ifdef __LINUX_OSAL__
	for(scaler_id=0; scaler_id<num_scalers; scaler_id++) {
		if((bsp_disp_feat_get_layer_feats(scaler_id, DISP_LAYER_WORK_MODE_SCALER, scaler_id)
				    & DISP_LAYER_FEAT_DE_INTERLACE))
			maf_flag_mem[scaler_id][0] = (void*)__pa((char __iomem *)kmalloc(maf_flag_mem_len, GFP_KERNEL |  __GFP_ZERO));
			maf_flag_mem[scaler_id][1] = (void*)__pa((char __iomem *)kmalloc(maf_flag_mem_len, GFP_KERNEL |  __GFP_ZERO));
			DE_INF("maf_flag_mem[%d][%d]=0x%8x, maf_flag_mem[%d][%d]=0x%8x\n",scaler_id, 0,
				(unsigned int)maf_flag_mem[scaler_id][0], scaler_id, 1, (unsigned int)maf_flag_mem[scaler_id][1]);
			if((maf_flag_mem[scaler_id][0] == NULL) || (maf_flag_mem[scaler_id][1] == NULL)) {
				DE_WRN("maf memory[%d] request fail\n", scaler_id);
			}
	}
#else
	for(scaler_id=0; scaler_id<num_scalers; scaler_id++) {
		if((bsp_disp_feat_get_layer_feats(scaler_id, DISP_LAYER_WORK_MODE_SCALER, scaler_id)
			    & DISP_LAYER_FEAT_DE_INTERLACE))
			maf_flag_mem[scaler_id][0] = OSAL_PhyAlloc(maf_flag_mem_len);
			maf_flag_mem[scaler_id][1] = OSAL_PhyAlloc(maf_flag_mem_len);
			DE_INF("maf_flag_mem[%d][%d]=0x%8x, maf_flag_mem[%d][%d]=0x%8x\n",scaler_id, 0,
				(unsigned int)maf_flag_mem[scaler_id][0], scaler_id, 1, (unsigned int)maf_flag_mem[scaler_id][1]);
			if((maf_flag_mem[scaler_id][0] == NULL) || (maf_flag_mem[scaler_id][1] == NULL)) {
				DE_WRN("maf memory[%d] request fail\n", scaler_id);
			}
	}
#endif
	for(scaler_id=0; scaler_id<num_scalers; scaler_id++) {
		dit_mode_default[scaler_id] = DIT_MODE_MAF;
	}

	return DIS_SUCCESS;
}

__s32 disp_video_exit(void)
{
	__u32 num_screen;
	__u32 num_scalers;
	__u32 scaler_id;

	num_screen = bsp_disp_feat_get_num_screens();
	num_scalers = bsp_disp_feat_get_num_scalers();
#ifdef __LINUX_OSAL__
	for(scaler_id=0; scaler_id<num_scalers; scaler_id++) {
		if(maf_flag_mem[scaler_id][0]) {
			kfree(maf_flag_mem[scaler_id][0]);
		}
		if(maf_flag_mem[scaler_id][1]) {
			kfree(maf_flag_mem[scaler_id][1]);
		}
	}

#else
	for(scaler_id=0; scaler_id<num_scalers; scaler_id++) {
		if(maf_flag_mem[scaler_id][0]) {
			OSAL_PhyFree(maf_flag_mem[scaler_id][0], maf_flag_mem_len);
		}
		if(maf_flag_mem[scaler_id][1]) {
			OSAL_PhyFree(maf_flag_mem[scaler_id][1], maf_flag_mem_len);
		}
	}

#endif
	memset(g_video,0,sizeof(g_video));

	return DIS_SUCCESS;
}

__s32 disp_video_set_dit_mode(__u32 scaler_index, __u32 mode)
{
	dit_mode_default[scaler_index] = mode;
	return DIS_SUCCESS;
}

__s32 disp_video_get_dit_mode(__u32 scaler_index)
{
	return dit_mode_default[scaler_index];
}

__s32 bsp_disp_video_get_fb(__u32 screen_id, __u32 hid, __disp_video_fb_t *in_addr)
{
	hid = HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	if(g_video[screen_id][hid].enable) {
		memcpy(in_addr, &g_video[screen_id][hid].video_new, sizeof(__disp_video_fb_t));

		return DIS_SUCCESS;
	}	else {
		return DIS_FAIL;
	}
}

//return 10fps
__s32 bsp_disp_video_get_fps(__u32 screen_id, __u32 hid)
{
	__u32 pre_time_index, cur_time_index;
	__u32 pre_time, cur_time;
	__u32 fps = 0xff;

	hid = HANDTOID(hid);
	HLID_ASSERT(hid, gdisp.screen[screen_id].max_layers);

	pre_time_index = video_time_index[screen_id][hid];
	cur_time_index = (pre_time_index == 0)? (VIDEO_TIME_LEN -1):(pre_time_index-1);

	pre_time = video_time[screen_id][hid][pre_time_index];
	cur_time = video_time[screen_id][hid][cur_time_index];

	if(pre_time != cur_time) {
		fps = 1000 * 100 / (cur_time - pre_time);
	}

	return fps;
}

__s32 disp_video_checkin(__u32 screen_id, __u32 id)
{
#if defined __LINUX_OSAL__
	video_time[screen_id][id][video_time_index[screen_id][id]] = jiffies;
#endif
	video_time_index[screen_id][id] ++;

	video_time_index[screen_id][id] = (video_time_index[screen_id][id] >= VIDEO_TIME_LEN)? 0:video_time_index[screen_id][id];

	return 0;
}


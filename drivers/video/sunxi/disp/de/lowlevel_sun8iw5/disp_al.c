#include "disp_al.h"

static __disp_al_mod_list_t mod_list;
static __disp_al_private_data *al_private;
#if defined(__LINUX_PLAT__)
static spinlock_t al_data_lock;
#endif

static s32 scaler_set_para(u32 scaler_id, disp_scaler_info *scaler_info);
s32 scaler_sync(u32 scaler_id);
s32 scaler_close(u32 scaler_id);

static __de_format_t scal_input_format_array[] =
{
	/*  format                                     mod,                         fmt                seq          br_swap */
	{DISP_FORMAT_ARGB_8888                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_ARGB    ,0 ,},
	{DISP_FORMAT_ABGR_8888                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_ARGB    ,1 ,},
	{DISP_FORMAT_RGBA_8888                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_BGRA    ,1 ,},
	{DISP_FORMAT_BGRA_8888                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_BGRA    ,0 ,},
	{DISP_FORMAT_XRGB_8888                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_ARGB    ,0 ,},
	{DISP_FORMAT_XBGR_8888                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_ARGB    ,1 ,},
	{DISP_FORMAT_RGBX_8888                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_ARGB    ,1 ,},
	{DISP_FORMAT_BGRX_8888                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_BGRA    ,0 ,},
	{DISP_FORMAT_RGB_888                    ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_ARGB    ,0 ,},
	{DISP_FORMAT_BGR_888                    ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB888      ,DE_SCAL_BGRA    ,0 ,},
	{DISP_FORMAT_RGB_565                    ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB565      ,DE_SCAL_RGB565  ,0 ,},
	{DISP_FORMAT_BGR_565                    ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB565      ,DE_SCAL_BGR565  ,0 ,},
	{DISP_FORMAT_ARGB_4444                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB4444     ,DE_SCAL_ARGB4444,0 ,},
	{DISP_FORMAT_ABGR_4444                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB4444     ,DE_SCAL_ARGB4444,1 ,},
	{DISP_FORMAT_RGBA_4444                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB4444     ,DE_SCAL_BGRA4444,1 ,},
	{DISP_FORMAT_BGRA_4444                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB4444     ,DE_SCAL_BGRA4444,0 ,},
	{DISP_FORMAT_ARGB_1555                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB1555     ,DE_SCAL_ARGB1555,0 ,},
	{DISP_FORMAT_ABGR_1555                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB1555     ,DE_SCAL_ARGB1555,1 ,},
	{DISP_FORMAT_RGBA_5551                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB1555     ,DE_SCAL_BGRA5551,1 ,},
	{DISP_FORMAT_BGRA_5551                  ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INRGB1555     ,DE_SCAL_BGRA5551,0 ,},
	{DISP_FORMAT_YUV444_I_AYUV              ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INYUV444      ,DE_SCAL_AYUV    ,0 ,},
	{DISP_FORMAT_YUV444_I_VUYA              ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INYUV444      ,DE_SCAL_VUYA    ,0 ,},
	{DISP_FORMAT_YUV422_I_YVYU              ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INYUV444      ,DE_SCAL_YVYU    ,0 ,},
	{DISP_FORMAT_YUV422_I_YUYV              ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INYUV422      ,DE_SCAL_YUYV    ,0 ,},
	{DISP_FORMAT_YUV422_I_UYVY              ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INYUV422      ,DE_SCAL_UYVY    ,0 ,},
	{DISP_FORMAT_YUV422_I_VYUY              ,DE_SCAL_INTERLEAVED         ,DE_SCAL_INYUV422      ,DE_SCAL_VYUY    ,0 ,},
	{DISP_FORMAT_YUV444_P                   ,DE_SCAL_PLANNAR             ,DE_SCAL_INYUV444      ,0/*P3210*/      ,0 ,},
	{DISP_FORMAT_YUV422_P                   ,DE_SCAL_PLANNAR             ,DE_SCAL_INYUV422      ,0               ,0 ,},
	{DISP_FORMAT_YUV420_P                   ,DE_SCAL_PLANNAR             ,DE_SCAL_INYUV420      ,0               ,0 ,},
	{DISP_FORMAT_YUV411_P                   ,DE_SCAL_PLANNAR             ,DE_SCAL_INYUV411      ,0               ,0 ,},
	{DISP_FORMAT_YUV422_SP_UVUV             ,DE_SCAL_UVCOMBINED          ,DE_SCAL_INYUV422      ,DE_SCAL_UVUV    ,0 ,},
	{DISP_FORMAT_YUV422_SP_VUVU             ,DE_SCAL_UVCOMBINED          ,DE_SCAL_INYUV422      ,DE_SCAL_VUVU    ,0 ,},
	{DISP_FORMAT_YUV420_SP_UVUV             ,DE_SCAL_UVCOMBINED          ,DE_SCAL_INYUV420      ,DE_SCAL_UVUV    ,0 ,},
	{DISP_FORMAT_YUV420_SP_VUVU             ,DE_SCAL_UVCOMBINED          ,DE_SCAL_INYUV420      ,DE_SCAL_VUVU    ,0 ,},
	{DISP_FORMAT_YUV411_SP_UVUV             ,DE_SCAL_UVCOMBINED          ,DE_SCAL_INYUV411      ,DE_SCAL_UVUV    ,0 ,},
	{DISP_FORMAT_YUV411_SP_VUVU             ,DE_SCAL_UVCOMBINED          ,DE_SCAL_INYUV411      ,DE_SCAL_VUVU    ,0 ,},
	{DISP_FORMAT_YUV422_SP_TILE_UVUV       ,DE_SCAL_UVCOMBINEDMB        ,DE_SCAL_INYUV422      ,DE_SCAL_UVUV    ,0 ,},
	{DISP_FORMAT_YUV422_SP_TILE_VUVU       ,DE_SCAL_UVCOMBINEDMB        ,DE_SCAL_INYUV422      ,DE_SCAL_VUVU    ,0 ,},
	{DISP_FORMAT_YUV420_SP_TILE_UVUV       ,DE_SCAL_UVCOMBINEDMB        ,DE_SCAL_INYUV420      ,DE_SCAL_UVUV    ,0 ,},
	{DISP_FORMAT_YUV420_SP_TILE_VUVU       ,DE_SCAL_UVCOMBINEDMB        ,DE_SCAL_INYUV420      ,DE_SCAL_VUVU    ,0 ,},
	{DISP_FORMAT_YUV411_SP_TILE_UVUV       ,DE_SCAL_UVCOMBINEDMB        ,DE_SCAL_INYUV411      ,DE_SCAL_UVUV    ,0 ,},
	{DISP_FORMAT_YUV411_SP_TILE_VUVU       ,DE_SCAL_UVCOMBINEDMB        ,DE_SCAL_INYUV411      ,DE_SCAL_VUVU    ,0 ,},
	//{DISP_FORMAT_YUV422_SP_TILE_128X32_UVUV,DE_SCAL_UVCOMBINEDMB_128X32 ,DE_SCAL_INYUV422      ,DE_SCAL_UVUV    ,0 ,},
	//{DISP_FORMAT_YUV422_SP_TILE_128X32_VUVU,DE_SCAL_UVCOMBINEDMB_128X32 ,DE_SCAL_INYUV422      ,DE_SCAL_VUVU    ,0 ,},
	//{DISP_FORMAT_YUV420_SP_TILE_128X32_UVUV,DE_SCAL_UVCOMBINEDMB_128X32 ,DE_SCAL_INYUV420      ,DE_SCAL_UVUV    ,0 ,},
	//{DISP_FORMAT_YUV420_SP_TILE_128X32_VUVU,DE_SCAL_UVCOMBINEDMB_128X32 ,DE_SCAL_INYUV420      ,DE_SCAL_VUVU    ,0 ,},
	//{DISP_FORMAT_YUV411_SP_TILE_128X32_UVUV,DE_SCAL_UVCOMBINEDMB_128X32 ,DE_SCAL_INYUV411      ,DE_SCAL_UVUV    ,0 ,},
	//{DISP_FORMAT_YUV411_SP_TILE_128X32_VUVU,DE_SCAL_UVCOMBINEDMB_128X32 ,DE_SCAL_INYUV411      ,DE_SCAL_VUVU    ,0 ,},
};

static __de_format_t scal_output_format_array[] =
{
	/*  format                              mod,   fmt               seq br_swap */
	{DISP_FORMAT_ARGB_8888                  ,0 ,DE_SCAL_OUTI1RGB888   ,0 ,0 ,},
	{DISP_FORMAT_ABGR_8888                  ,0 ,DE_SCAL_OUTI1RGB888   ,0 ,1 ,},
	{DISP_FORMAT_RGBA_8888                  ,0 ,DE_SCAL_OUTI0RGB888   ,0 ,1 ,},
	{DISP_FORMAT_BGRA_8888                  ,0 ,DE_SCAL_OUTI0RGB888   ,0 ,0 ,},
	{DISP_FORMAT_XRGB_8888                  ,0 ,DE_SCAL_OUTI1RGB888   ,0 ,0 ,},
	{DISP_FORMAT_XBGR_8888                  ,0 ,DE_SCAL_OUTI1RGB888   ,0 ,1 ,},
	{DISP_FORMAT_RGBX_8888                  ,0 ,DE_SCAL_OUTI0RGB888   ,0 ,1 ,},
	{DISP_FORMAT_BGRX_8888                  ,0 ,DE_SCAL_OUTI0RGB888   ,0 ,0 ,},
	{DISP_FORMAT_YUV444_P                   ,0 ,DE_SCAL_OUTPYUV444    ,0 ,0 ,},
	{DISP_FORMAT_YUV422_P                   ,0 ,DE_SCAL_OUTPYUV422    ,0 ,0 ,},
	{DISP_FORMAT_YUV420_P                   ,0 ,DE_SCAL_OUTPYUV420    ,0 ,0 ,},
	{DISP_FORMAT_YUV411_P                   ,0 ,DE_SCAL_OUTPYUV411    ,0 ,0 ,},
};

static __de_format_t be_input_format_array[] =
{
	/*  format                             mod,    fmt                seq  br_swap */
	{DISP_FORMAT_ARGB_8888                  ,0 ,DE_COLOR_ARGB8888    ,0    ,0 ,},
	{DISP_FORMAT_ABGR_8888                  ,0 ,DE_COLOR_ARGB8888    ,0    ,1 ,},
	{DISP_FORMAT_RGBA_8888                  ,0 ,DE_COLOR_ARGB8888    ,2    ,1 ,},
	{DISP_FORMAT_BGRA_8888                  ,0 ,DE_COLOR_ARGB8888    ,2    ,0 ,},
	{DISP_FORMAT_XRGB_8888                  ,0 ,DE_COLOR_ARGB8888    ,0    ,0 ,},
	{DISP_FORMAT_XBGR_8888                  ,0 ,DE_COLOR_ARGB8888    ,0    ,1 ,},
	{DISP_FORMAT_RGBX_8888                  ,0 ,DE_COLOR_ARGB8888    ,2    ,1 ,},
	{DISP_FORMAT_BGRX_8888                  ,0 ,DE_COLOR_ARGB8888    ,2    ,0 ,},
	{DISP_FORMAT_RGB_888                    ,0 ,DE_COLOR_RGB888      ,0    ,0 ,},
	{DISP_FORMAT_BGR_888                    ,0 ,DE_COLOR_RGB888      ,2    ,0 ,},
	{DISP_FORMAT_RGB_565                    ,0 ,DE_COLOR_RGB565      ,0    ,0 ,},
	{DISP_FORMAT_BGR_565                    ,0 ,DE_COLOR_RGB565      ,1    ,0 ,},
	{DISP_FORMAT_ARGB_4444                  ,0 ,DE_COLOR_ARGB4444    ,0    ,0 ,},
	{DISP_FORMAT_ABGR_4444                  ,0 ,DE_COLOR_ARGB4444    ,0    ,1 ,},
	{DISP_FORMAT_RGBA_4444                  ,0 ,DE_COLOR_ARGB4444    ,1    ,1 ,},
	{DISP_FORMAT_BGRA_4444                  ,0 ,DE_COLOR_ARGB4444    ,2    ,0 ,},
	{DISP_FORMAT_ARGB_1555                  ,0 ,DE_COLOR_ARGB1555    ,0    ,0 ,},
	{DISP_FORMAT_ABGR_1555                  ,0 ,DE_COLOR_ARGB1555    ,1    ,1 ,},
	{DISP_FORMAT_RGBA_5551                  ,0 ,DE_COLOR_ARGB1555    ,1    ,1 ,},
	{DISP_FORMAT_BGRA_5551                  ,0 ,DE_COLOR_ARGB1555    ,0    ,0 ,},
};
/***********************************************************
 *
 * basic
 *
 ***********************************************************/

static __disp_al_private_data *disp_al_get_priv(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id > num_screens)
		return NULL;

	return &al_private[screen_id];
}

/***********************************************************
 *
 * notifier
 *
 ***********************************************************/
static struct disp_notifier_block al_notifier_list;

static s32 disp_al_notifier_init(void)
{
	INIT_LIST_HEAD(&al_notifier_list.list);
	return 0;
}

s32 disp_al_notifier_register(struct disp_notifier_block *nb)
{
	if((NULL == nb)) {
		DE_WRN("hdl is NULL\n");
		return -1;
	}
	list_add_tail(&(nb->list), &(al_notifier_list.list));
	return 0;
}

s32 disp_al_notifier_unregister(struct disp_notifier_block *nb)
{
	struct disp_notifier_block *ptr;
	if((NULL == nb)) {
		DE_WRN("hdl is NULL\n");
		return -1;
	}
	list_for_each_entry(ptr, &al_notifier_list.list, list) {
		if(ptr == nb) {
			list_del(&ptr->list);
			return 0;
		}
	}
	return -1;
}

static s32 disp_al_notifier_call_chain(u32 event, u32 sel, void *v)
{
	struct disp_notifier_block *ptr;

	//DE_INF("notifier_call_chain: event=0x%x, sel=%d, data=0x%x\n", event, sel, (u32)v);

	list_for_each_entry(ptr, &al_notifier_list.list, list) {
		if(ptr->notifier_call)
			ptr->notifier_call(ptr, event, sel, v);
	}

	return 0;
}

static s32 disp_al_be_notifier_callback(struct disp_notifier_block *self,
		 u32 event, u32 sel, void *data)
{
	__disp_al_private_data *al_priv;
	disp_size *rect;
	u32 *ptr;

	al_priv = disp_al_get_priv(sel);
	if(NULL == al_priv)
		return -1;

	//DE_INF("notifier cb: event=0x%x, sel=%d, data=0x%x\n", event, sel, (u32)data);
	switch(event){
	case DISP_EVENT_OUTPUT_SIZE:
		rect = (disp_size *)data;
		al_priv->be_info.out_width = rect->width;
		al_priv->be_info.out_height = rect->height;
		if(al_priv->be_info.enabled)
			DE_BE_set_display_size(sel, al_priv->be_info.out_width, al_priv->be_info.out_height);
		break;
	case DISP_EVENT_OUTPUT_CSC:
	case DISP_EVENT_SAT_CSC:
		ptr = (u32*)data;
		al_priv->be_info.out_csc = ptr[0];
		al_priv->be_info.color_range = ptr[1];

		if(al_priv->be_info.enabled)
			DE_BE_Set_Enhance_ex(sel, al_priv->be_info.out_csc, al_priv->be_info.color_range, 0, 50, 50, 50, 50);
		break;
	default:
		break;
	}
	return 0;
}

static s32 disp_al_drc_notifier_callback(struct disp_notifier_block *self,
		 u32 event, u32 sel, void *data)
{
	__disp_al_private_data *al_priv;
	disp_size *rect;

	al_priv = disp_al_get_priv(sel);
	if(NULL == al_priv)
		return -1;

	switch(event){
	case DISP_EVENT_OUTPUT_SIZE:
		rect = (disp_size *)data;
		al_priv->drc_info.out_width = rect->width;
		al_priv->drc_info.out_height = rect->height;
		al_priv->drc_info.in_width = al_priv->drc_info.out_width;
		al_priv->drc_info.in_height = al_priv->drc_info.out_height;
		IEP_Drc_Set_Imgsize(sel, al_priv->drc_info.out_width, al_priv->drc_info.out_height);
		disp_al_smbl_sync(sel);
		break;
	default:
		break;
	}
	return 0;
}

static s32 disp_al_fe_notifier_callback(struct disp_notifier_block *self,
		 u32 event, u32 sel, void *data)
{
	__disp_al_private_data *al_priv;
	u32 *ptr = (u32*)data;
	s32 ret = 0;

	al_priv = disp_al_get_priv(sel);
	if(NULL == al_priv)
		return -1;

	switch(event){
	case DISP_EVENT_DEU_CSC:
	{
		/* data[0]:csc,  data[1]: color range */
		u32 csc = ptr[0];

		al_priv->fe_info.out_csc = csc;
	}
		break;

	case DISP_EVENT_MANAGER_SYNC:
	{
		u32 num_scalers;
		u32 scaler_id;

		num_scalers = bsp_disp_feat_get_num_scalers();
		for(scaler_id=0; scaler_id<num_scalers; scaler_id++) {
			u32 screen_id;

			al_priv = disp_al_get_priv(scaler_id);
			if(NULL == al_priv)
				continue;

			screen_id = al_priv->fe_info.out_select;
			if((sel == screen_id) && (1 == al_priv->fe_info.enabled)) {
				scaler_sync(scaler_id);
			}
		}
	}
		break;

	default:
		break;
	}
	return ret;
}

static s32 disp_al_sat_notifier_callback(struct disp_notifier_block *self,
		 u32 event, u32 sel, void *data)
{
	__disp_al_private_data *al_priv;
	disp_size *rect;

	al_priv = disp_al_get_priv(sel);
	if(NULL == al_priv)
		return -1;

	switch(event){
	case DISP_EVENT_OUTPUT_SIZE:
		rect = (disp_size *)data;
		al_priv->sat_info.size.width = rect->width;
		al_priv->sat_info.size.height = rect->height;
		SAT_Set_Display_Size(sel, al_priv->sat_info.size.width, al_priv->sat_info.size.height);
		break;
	default:
		break;
	}
	return 0;
}

/* query mod if exist in current plat
 * @mod_id: module id
 *
 * returns exist(1), not exist(0)
 */
s32 disp_al_query_mod(disp_mod_id mod_id)
{
	__disp_al_mod_list_t *mod;
	struct list_head *pos;

	list_for_each(pos, &mod_list.list)
	{
		mod = list_entry(pos, __disp_al_mod_list_t, list);
		if(mod->mod_id == mod_id)
			return 1;
	}

	return 0;
}

s32 disp_al_query_be_mod(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens)
		return 0;

	return disp_al_query_mod(DISP_MOD_BE0+screen_id);
}

s32 disp_al_query_fe_mod(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens)
		return 0;

	return disp_al_query_mod(DISP_MOD_FE0+screen_id);
}

s32 disp_al_query_lcd_mod(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens)
		return 0;

	return disp_al_query_mod(DISP_MOD_LCD0+screen_id);
}

s32 disp_al_query_smart_color_mod(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens)
		return 0;

	return disp_al_query_mod(DISP_MOD_SAT0+screen_id);
}

s32 disp_al_query_drc_mod(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if((screen_id >= num_screens) || (!bsp_disp_feat_get_smart_backlight_support(screen_id)))
		return 0;

	return disp_al_query_mod(DISP_MOD_DRC0+screen_id);
}

s32 disp_al_query_dsi_mod(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens)
		return 0;

	return disp_al_query_mod(DISP_MOD_DSI0+screen_id);
}

s32 disp_al_query_edp_mod(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens)
		return 0;

	return disp_al_query_mod(DISP_MOD_EDP);
}

/***********************************************************
 *
 * global
 *
 ***********************************************************/
s32 disp_al_check_csc(u32 screen_id)
{
	return 0;
}
s32 disp_al_check_display_size(u32 screen_id)
{
	return 0;
}

/***********************************************************
 *
 * disp_al_lcd
 *
 ***********************************************************/
s32 disp_al_lcd_clk_init(u32 clk_id)
{
	__hdle hdl;

	hdl = OSAL_CCMU_OpenMclk(clk_id);
	//OSAL_CCMU_MclkOnOff(hdl, CLK_ON);
	OSAL_CCMU_SetMclkSrc(hdl);
	OSAL_CCMU_SetMclkFreq(hdl, 0);

	OSAL_CCMU_MclkOnOff(hdl, CLK_OFF);

	OSAL_CCMU_CloseMclk(hdl);

	return 0;
}

s32 disp_al_lcd_clk_exit(u32 clk_id)
{
	__hdle hdl;

	hdl = OSAL_CCMU_OpenMclk(clk_id);
	OSAL_CCMU_SetMclkSrc(hdl);
	OSAL_CCMU_SetMclkFreq(hdl, 0);

	OSAL_CCMU_MclkOnOff(hdl, CLK_OFF);

	OSAL_CCMU_CloseMclk(hdl);

	return 0;
}

s32 disp_al_lcd_clk_enable(u32 clk_id)
{
	__hdle hdl;
	hdl = OSAL_CCMU_OpenMclk(clk_id);
	OSAL_CCMU_MclkOnOff(hdl, CLK_ON);
	OSAL_CCMU_CloseMclk(hdl);
	return 0;
}

s32 disp_al_lcd_clk_disable(u32 clk_id)
{
	__hdle hdl;
	hdl = OSAL_CCMU_OpenMclk(clk_id);
	OSAL_CCMU_MclkOnOff(hdl, CLK_OFF);
	OSAL_CCMU_CloseMclk(hdl);
	return 0;
}


s32 disp_al_lcd_cfg(u32 screen_id, disp_panel_para * panel)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;
#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&al_data_lock, flags);
#endif
		/*tcon0 for panel */
		al_priv->lcd_info.tcon_index = 0;
		al_priv->lcd_info.lcd_if = panel->lcd_if;
		al_priv->lcd_info.out_width = panel->lcd_x;
		al_priv->lcd_info.out_height = panel->lcd_y;
		al_priv->lcd_info.out_csc = ((LCD_IF_HV == panel->lcd_if) && (LCD_HV_IF_CCIR656_2CYC == panel->lcd_hv_if))? 1:0;
		al_priv->lcd_info.b_out_interlace = ((LCD_IF_HV == panel->lcd_if) && (LCD_HV_IF_CCIR656_2CYC == panel->lcd_hv_if))? 1:0;
		al_priv->lcd_info.in_width = al_priv->lcd_info.out_width;
		al_priv->lcd_info.in_height = al_priv->lcd_info.out_height;
		al_priv->lcd_info.in_csc = al_priv->lcd_info.out_csc;
		al_priv->lcd_info.b_in_interlace = al_priv->lcd_info.b_out_interlace;
		al_priv->lcd_info.time_per_line = panel->lcd_ht * 10 / panel->lcd_dclk_freq;//unit:0.1us

#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&al_data_lock, flags);
	}
#endif
	if(disp_al_query_lcd_mod(screen_id)) {
		if(0 != tcon0_cfg(screen_id, panel))
			DE_WRN("lcd cfg fail!\n");
		else
			DE_INF("lcd cfg ok!\n");

		if(LCD_IF_DSI == panel->lcd_if)	{
			if(0 != dsi_cfg(screen_id, panel)) {
				DE_WRN("dsi cfg fail!\n");
			}
		}
		tcon0_src_select(screen_id, LCD_SRC_BE0);
	}
	/* todo? channel 2 output edp */

	return 0;
}

s32 disp_al_edp_init(u32 screen_id, u32 edp_rate)
{
	return 0;
}

s32 disp_al_edp_disable_cfg(u32 screen_id)
{
	return 0;
}

s32 disp_al_edp_cfg(u32 screen_id, disp_panel_para * panel)
{
	return 0;
}

s32 disp_al_edp_int(__edp_irq_id_t edp_irq)
{
	return 0;
}

s32 disp_al_lcd_init(u32 screen_id)
{
	return tcon_init(screen_id);
}
s32 disp_al_lcd_exit(u32 screen_id)
{
	return tcon_exit(screen_id);
}
s32 disp_al_lcd_enable(u32 screen_id, u32 enable, disp_panel_para * panel)
{
	__disp_al_private_data *al_priv;
	disp_size size;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	if(disp_al_query_lcd_mod(screen_id)) {
		if(1 == enable) {
			tcon0_open(screen_id, panel);
			if(LCD_IF_LVDS == panel->lcd_if) {
				lvds_open(screen_id, panel);
			} else if(LCD_IF_DSI == panel->lcd_if) {
				dsi_open(screen_id, panel);
			}

#if defined(__LINUX_PLAT__)
	  {
		unsigned long flags;
		spin_lock_irqsave(&al_data_lock, flags);
#endif
		al_priv->lcd_info.enabled = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&al_data_lock, flags);
		}
#endif
		} else {
			if(LCD_IF_LVDS == panel->lcd_if) {
				lvds_close(screen_id);
			} else if(LCD_IF_DSI == panel->lcd_if) {
				dsi_close(screen_id);
			}
			tcon0_close(screen_id);

#if defined(__LINUX_PLAT__)
	  {
	  	unsigned long flags;
	  	spin_lock_irqsave(&al_data_lock, flags);
#endif
	  	al_priv->lcd_info.enabled = 0;
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&al_data_lock, flags);
		}
#endif
		}
	}
	/* todo? channel 2 output edp */
	size.width = al_priv->lcd_info.in_width;
	size.height = al_priv->lcd_info.in_height;
	disp_al_notifier_call_chain(DISP_EVENT_OUTPUT_SIZE, screen_id, &size);
	al_priv->output_type = DISP_OUTPUT_TYPE_LCD;

	return 0;
}
s32 disp_al_lcd_set_src(u32 screen_id, disp_lcd_src src)
{
	switch (src) {
	case DISP_LCDC_SRC_DE_CH1:
		tcon0_src_select(screen_id, LCD_SRC_BE0);
		break;

	case DISP_LCDC_SRC_DE_CH2:
		tcon0_src_select(screen_id, LCD_SRC_BE1);
		break;

	case DISP_LCDC_SRC_DMA888:
		tcon0_src_select(screen_id, LCD_SRC_DMA888);
		break;

	case DISP_LCDC_SRC_DMA565:
		tcon0_src_select(screen_id, LCD_SRC_DMA565);
		break;

	case DISP_LCDC_SRC_WHITE:
		tcon0_src_select(screen_id, LCD_SRC_WHITE);
		break;

	case DISP_LCDC_SRC_BLACK:
		tcon0_src_select(screen_id, LCD_SRC_BLACK);
		break;

	default:
		DE_WRN("not supported lcd src:%d in disp_al_lcd_set_src\n", src);
		return -1;
	}
	return 0;
}
/* query lcd irq, clear it when the irq queried exist
 * take dsi irq s32o account, todo?
 */
s32 disp_al_lcd_query_irq(u32 screen_id, __lcd_irq_id_t irq_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	if(LCD_IF_DSI == al_priv->lcd_info.lcd_if)
		return dsi_irq_query(screen_id, (__dsi_irq_id_t)irq_id);
	else
		return tcon_irq_query(screen_id, irq_id);
}
/* take dsi irq s32o account, todo? */
s32 disp_al_lcd_tri_busy(u32 screen_id)
{
	return (tcon0_tri_busy(screen_id) | (dsi_inst_busy(screen_id)));
}
/* take dsi irq s32o account, todo? */
s32 disp_al_lcd_tri_start(u32 screen_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	if(LCD_IF_DSI == al_priv->lcd_info.lcd_if)
		dsi_tri_start(screen_id);
	return tcon0_tri_start(screen_id);
}

s32 disp_al_lcd_io_cfg(u32 screen_id, u32 enable, disp_panel_para * panel)
{
	if(LCD_IF_DSI ==  panel->lcd_if) {
		if(enable) {
			dsi_io_open(screen_id, panel);
		} else {
			dsi_io_close(screen_id);
		}
	}

	return 0;
}

s32 disp_al_lcd_set_clk_div(u32 screen_id, u32 clk_div)
{
	return tcon0_set_dclk_div(screen_id, clk_div);
}

u32 disp_al_lcd_get_clk_div(u32 screen_id)
{
	return tcon0_get_dclk_div(screen_id);
}

s32 disp_al_lcd_get_cur_line(u32 screen_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	if(1 == al_priv->lcd_info.enabled) {
		if(LCD_IF_DSI == al_priv->lcd_info.lcd_if) {
			return dsi_get_cur_line(screen_id);
		} else {
			return tcon_get_cur_line(screen_id, al_priv->lcd_info.tcon_index);
		}
	}

	return 0;
}

s32 disp_al_lcd_get_start_delay(u32 screen_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	if(LCD_IF_DSI == al_priv->lcd_info.lcd_if) {
		return dsi_get_start_delay(screen_id);
	} else	{
		return tcon_get_start_delay(screen_id, al_priv->lcd_info.tcon_index);
	}

	return 0;
}

s32 disp_al_lcd_check_time(u32 screen_id, u32 us)
{
	__disp_al_private_data *al_priv;
	u32 cur_line = 0, start_delay = 0;
	u32 avail_time = 0;//us
	s32 ret = 0;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	if(1 == al_priv->lcd_info.enabled) {
		if(LCD_IF_DSI == al_priv->lcd_info.lcd_if) {
			cur_line = dsi_get_cur_line(screen_id);
			start_delay = dsi_get_start_delay(screen_id);
		} else {
			cur_line = tcon_get_cur_line(screen_id, al_priv->lcd_info.tcon_index);
			start_delay = tcon_get_start_delay(screen_id, al_priv->lcd_info.tcon_index);
		}
		if(cur_line >= start_delay) {
			ret = 1;
		} else {
			avail_time = (start_delay - cur_line) * al_priv->lcd_info.time_per_line / 10 - 10;//us
			if(avail_time > us)
				ret = 0;
			else
				ret = 1;

		}
		return ret;
	}

	return -1;
}
/***********************************************************
 *
 * disp_al_manager
 *
 ***********************************************************/
/* init reg */
s32 disp_al_manager_clk_init(u32 clk_id)
{
	__hdle hdl;

	hdl = OSAL_CCMU_OpenMclk(clk_id);
	OSAL_CCMU_SetMclkSrc(hdl);

	OSAL_CCMU_SetMclkFreq(hdl, 0);
	//OSAL_CCMU_MclkOnOff(hdl, CLK_ON);
	OSAL_CCMU_MclkOnOff(hdl, CLK_OFF);
	OSAL_CCMU_CloseMclk(hdl);

	return 0;
}

s32 disp_al_manager_clk_exit(u32 clk_id)
{
	__hdle hdl;

	hdl = OSAL_CCMU_OpenMclk(clk_id);
	OSAL_CCMU_MclkOnOff(hdl, CLK_OFF);
	OSAL_CCMU_CloseMclk(hdl);

	return 0;
}

s32 disp_al_manager_clk_enable(u32 clk_id)
{
	__hdle hdl;

	hdl = OSAL_CCMU_OpenMclk(clk_id);
	OSAL_CCMU_MclkOnOff(hdl, CLK_ON);
	OSAL_CCMU_CloseMclk(hdl);

	return 0;
}

s32 disp_al_manager_clk_disable(u32 clk_id)
{
	__hdle hdl;

	hdl = OSAL_CCMU_OpenMclk(clk_id);
	OSAL_CCMU_MclkOnOff(hdl, CLK_OFF);
	OSAL_CCMU_CloseMclk(hdl);

	return 0;
}


s32 disp_al_manager_init(u32 screen_id)
{
	return DE_BE_Reg_Init(screen_id);
}
s32 disp_al_mnanger_exit(u32 screen_id)
{
	return 0;
}
/*
 * take irq en/disable & reg_auto_load s32o account
 */
s32 disp_al_manager_enable(u32 screen_id, u32 enable)
{
	__disp_al_private_data *al_priv;
	u32 scaler_id;
	u32 scaler_num;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	if(1 ==  enable) {
		/* if using internal clk, enable clk */
		//enable_clk
		DE_BE_Reg_Init(screen_id);
		DE_BE_reg_auto_load_en(screen_id, 0);

		DE_BE_EnableINT(screen_id, DE_IMG_REG_LOAD_FINISH);
		DE_BE_Enable(screen_id);
		/* todo? if capture in processing, output_select will result in capture err */
		DE_BE_Output_Select(screen_id, screen_id);
		DE_BE_Set_Enhance_ex(screen_id, al_priv->be_info.out_csc, al_priv->be_info.color_range, 0, 50, 50, 50, 50);
		DE_BE_set_display_size(screen_id, al_priv->be_info.out_width, al_priv->be_info.out_height);
#if defined(__LINUX_PLAT__)
	  {
		unsigned long flags;
		spin_lock_irqsave(&al_data_lock, flags);
#endif
		al_priv->be_info.enabled = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&al_data_lock, flags);
		}
#endif
	} else {
		u32 data[2];
		DE_BE_Disable(screen_id);
		DE_BE_DisableINT(screen_id, DE_IMG_REG_LOAD_FINISH);
		/* if using internal clk, enable clk */
		//disable_clk
		scaler_num = bsp_disp_feat_get_num_scalers();
		for(scaler_id = 0; scaler_id < scaler_num; scaler_id++) {
			__disp_al_private_data *al_priv_temp;
			al_priv_temp = disp_al_get_priv(scaler_id);

			if(al_priv_temp->fe_info.out_select == screen_id) {
				scaler_close(scaler_id);
				break;
			}
		}

#if defined(__LINUX_PLAT__)
	  {
	  	unsigned long flags;
	  	spin_lock_irqsave(&al_data_lock, flags);
#endif
	  	al_priv->be_info.enabled = 0;
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&al_data_lock, flags);
		}
#endif
		//call last sync for resource release
		disp_al_notifier_call_chain(DISP_EVENT_MANAGER_SYNC, screen_id, (void*)&data);
	}

	return 0;
}

s32 disp_al_manager_set_display_size(u32 screen_id, u32 width, u32 height)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

  DE_BE_set_display_size(screen_id, width, height);

	return 0;
}

s32 disp_al_manager_get_display_size(u32 screen_id, u32 *width, u32 *height)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

  *width = DE_BE_get_display_width(screen_id);
  *height = DE_BE_get_display_height(screen_id);

	return 0;
}

s32 disp_al_manager_set_backcolor(u32 screen_id, disp_color_info *bk_color)
{
	return DE_BE_Set_BkColor(screen_id, *bk_color);;
}

s32 disp_al_manager_set_color_key(u32 screen_id, disp_colorkey *ck_mode)
{
	return DE_BE_Set_ColorKey(screen_id, ck_mode->ck_max, ck_mode->ck_min,
	    ck_mode->red_match_rule, ck_mode->green_match_rule, ck_mode->blue_match_rule);
}

s32 disp_al_manager_sync(u32 screen_id)
{
	/* sync the manager first, since it's the most important */
	DE_BE_Cfg_Ready(screen_id);
	scaler_sync_ex(screen_id);

	return 0;
}

s32 disp_al_manager_query_irq(u32 screen_id, u32 irq_id)
{
	u32 intflag;

	intflag = DE_BE_QueryINT(screen_id);
	DE_BE_ClearINT(screen_id, intflag);

	return (intflag & irq_id);
}

/***********************************************************
 *
 * disp_al_scaler
 *
 ***********************************************************/
static __disp_pixel_type_t disp_al_get_pixel_type(disp_pixel_format  format)
{
	if(format >= DISP_FORMAT_YUV444_I_AYUV) {
		return DISP_PIXEL_TYPE_YUV;
	}	else {
		return DISP_PIXEL_TYPE_RGB;
	}
}

// 0:scaler input pixel format
// 1:scaler output format
static s32  scaler_sw_para_to_reg(u32 type, disp_pixel_format format, __de_format_t *scal_fmt)
{
	u32 array_size = ARRAY_SIZE(scal_input_format_array);
	u32 i;

	if((NULL == scal_fmt) || (0 == array_size))
		return -1;

	memset(scal_fmt, 0, sizeof(__de_format_t));
	/* scaler input  pixel format */
	if(type == 0)	{
		memcpy(scal_fmt, &scal_input_format_array[0], sizeof(__de_format_t));
		for(i=0; i<array_size; i++) {
			if(scal_input_format_array[i].format == format) {
				memcpy(scal_fmt, &scal_input_format_array[i], sizeof(__de_format_t));
				return 0;
			}
		}
	}
	/* scaler output format */
	else if(type == 1) {
		memcpy(scal_fmt, &scal_output_format_array[0], sizeof(__de_format_t));
		for(i=0; i<ARRAY_SIZE(scal_output_format_array); i++) {
			if(scal_output_format_array[i].format == format) {
				memcpy(scal_fmt, &scal_output_format_array[i], sizeof(__de_format_t));
				return 0;
			}
		}
	}
	DE_INF("not supported type:%d format: %d in scaler_sw_para_to_reg\n", type, format);
	return DIS_FAIL;
}

// 0: 3d in mode
// 1: 3d out mode
static s32 scaler_3d_sw_para_to_reg(u32 type, u32 mode, bool b_out_interlace)
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
			DE_INF("not supported 3d in mode:%d in scaler_3d_sw_para_to_reg\n", mode);
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
			if(b_out_interlace == true)	{
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
			DE_INF("not supported 3d output mode:%d in scaler_3d_sw_para_to_reg\n", mode);
			return DIS_FAIL;
		}
	}

	return DIS_FAIL;
}

static s32 scaler_clk_init(u32 scaler_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

	DE_INF("scaler %d clk init\n", scaler_id);

	al_priv->fe_info.clk.h_clk = OSAL_CCMU_OpenMclk(al_priv->fe_info.clk.clk);

	//OSAL_CCMU_MclkOnOff(al_priv->fe_info.clk.h_clk, CLK_ON);
	OSAL_CCMU_SetMclkSrc(al_priv->fe_info.clk.h_clk);

	OSAL_CCMU_SetMclkFreq(al_priv->fe_info.clk.h_clk, 0);

	OSAL_CCMU_MclkOnOff(al_priv->fe_info.clk.h_clk, CLK_OFF);

	return DIS_SUCCESS;
}

static s32 scaler_clk_exit(u32 scaler_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

	DE_INF("scaler %d clk exit\n", scaler_id);

	OSAL_CCMU_MclkOnOff(al_priv->fe_info.clk.h_clk, CLK_OFF);
	OSAL_CCMU_CloseMclk(al_priv->fe_info.clk.h_clk);

#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&al_data_lock, flags);
#endif
		al_priv->fe_info.clk.enabled = 0;
		al_priv->fe_info.clk.h_clk = 0;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&al_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 scaler_clk_on(u32 scaler_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

	DE_INF("scaler %d clk on\n", scaler_id);

	OSAL_CCMU_MclkOnOff(al_priv->fe_info.clk.h_clk, CLK_ON);

#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&al_data_lock, flags);
#endif
		al_priv->fe_info.clk.enabled = 1;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&al_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 scaler_clk_off(u32 scaler_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

	DE_INF("scaler %d clk off\n", scaler_id);

	OSAL_CCMU_MclkOnOff(al_priv->fe_info.clk.h_clk, CLK_OFF);

#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&al_data_lock, flags);
#endif
		al_priv->fe_info.clk.enabled = 0;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&al_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

s32 scaler_init(__disp_bsp_init_para * para)
{
	u32 num_screens;
	u32 scaler_id;

	DE_INF("scaler_init\n");

	num_screens = bsp_disp_feat_get_num_screens();
	for(scaler_id=0; scaler_id<num_screens; scaler_id++) {
		__disp_al_private_data *al_priv;

		al_priv = disp_al_get_priv(scaler_id);
		if(NULL == al_priv)
			continue;

		switch(scaler_id) {
		case 0:
			al_priv->fe_info.clk.clk = MOD_CLK_DEFE0;
			al_priv->fe_info.extra_clk.clk = MOD_CLK_IEPDEU0;

			break;
		case 1:
			al_priv->fe_info.clk.clk = MOD_CLK_DEFE1;
			al_priv->fe_info.extra_clk.clk = MOD_CLK_IEPDEU1;

			break;
		case 2:
			al_priv->fe_info.extra_clk.clk = MOD_CLK_DEFE2;

			break;
		default:
			DE_WRN("scaler_init err, id %d outof range\n", scaler_id);
		}

		scaler_clk_init(scaler_id);
	}
	return 0;
}

s32 scaler_exit(void)
{
	u32 num_screens;
	u32 scaler_id;

	DE_INF("%s\n", __func__);

	num_screens = bsp_disp_feat_get_num_screens();
	for(scaler_id=0; scaler_id<num_screens; scaler_id++) {
		scaler_clk_exit(scaler_id);
	}
	return 0;
}

static s32 scaler_open(u32 scaler_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

	DE_INF("scaler %d open\n", scaler_id);

	if(0 == al_priv->fe_info.clk.enabled) {
		scaler_clk_on(scaler_id);
		/* todo? register notifier */
		// deu
		DE_INF("%s %d\n", __func__, scaler_id);
		DE_SCAL_Reset(scaler_id);
		DE_SCAL_Enable(scaler_id);
		DE_SCAL_Start(scaler_id);
		DE_SCAL_Input_Select(scaler_id, 0);
	}
#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&al_data_lock, flags);
#endif
		al_priv->fe_info.clk.enabled = 1;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&al_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

s32 scaler_close(u32 scaler_id)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

	if((1 == al_priv->fe_info.enabled)) {
		DE_SCAL_Reset(scaler_id);
		DE_SCAL_Disable(scaler_id);
		scaler_clk_off(scaler_id);
#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&al_data_lock, flags);
#endif
		al_priv->fe_info.close_request = 0;
		al_priv->fe_info.enabled = 0;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&al_data_lock, flags);
	}
#endif
	}

	return DIS_SUCCESS;
}

static s32 scaler_request(u32 requst_index)
{
	s32 ret = DIS_NO_RES;
	u32 num_scalers;
	s32 scaler_id = -1;
	unsigned long flags;

	num_scalers = bsp_disp_feat_get_num_scalers();

	DE_INF("scaler_Request,%d\n", requst_index);

	for(scaler_id=0; scaler_id<num_scalers; scaler_id++) {
		__disp_al_private_data *al_priv;
		al_priv = disp_al_get_priv(scaler_id);
		if(NULL == al_priv)
			continue;

#if defined(__LINUX_PLAT__)
	spin_lock_irqsave(&al_data_lock, flags);
#endif
		if(((requst_index == scaler_id) || (requst_index == 0xff))
			&& ((0 == al_priv->fe_info.enabled) || (1 == al_priv->fe_info.close_request)))	{
			ret = scaler_id;
			al_priv->fe_info.enabled = 1;
			al_priv->fe_info.close_request = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&al_data_lock, flags);
#endif
			scaler_open(ret);
			break;
		} else {
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&al_data_lock, flags);
#endif
		}
	}

	return ret;
}

static s32 scaler_release(u32 scaler_id, bool b_layer)
{
	u32 screen_index = 0xff;
	bool b_display = false;
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

	DE_INF("scaler_Release:%d\n", scaler_id);

	DE_SCAL_Set_Di_Ctrl(scaler_id, 0, 0, 0, 0);
	screen_index = al_priv->fe_info.out_select;

	if(screen_index != 0xff) {
		__disp_al_private_data *al_priv_tmp; //for be

		al_priv_tmp = disp_al_get_priv(screen_index);
		if(1 == al_priv_tmp->be_info.enabled)
			b_display = true;
	}
	/* is not for a layer or display is off, then close it immediately */
	if((!b_layer) || !b_display) {
		//DE_INF("scaler close direct\n");
		scaler_close(scaler_id);
	}	else {
	/* when it is for a layer and display is on, set a flag,
	     it will close at the next vblank zone */
#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&al_data_lock, flags);
#endif
		al_priv->fe_info.close_request = 1;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&al_data_lock, flags);
	}
#endif
	}

	return DIS_SUCCESS;
}

s32 scaler_sync(u32 scaler_id)
{
	unsigned long flags;
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

#if defined(__LINUX_PLAT__)
	spin_lock_irqsave(&al_data_lock, flags);
#endif
	if((1 == al_priv->fe_info.close_request)) {
		al_priv->fe_info.close_request = 0;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&al_data_lock, flags);
#endif
		scaler_close(scaler_id);
	} else {
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&al_data_lock, flags);
#endif
		DE_SCAL_Set_Reg_Rdy(scaler_id);
	}

	return 0;
}

s32 scaler_sync_ex(u32 sel)
{
	u32 num_scalers;
	u32 scaler_id;
	__disp_al_private_data *al_priv;

	num_scalers = bsp_disp_feat_get_num_scalers();
	for(scaler_id=0; scaler_id<num_scalers; scaler_id++) {
		u32 screen_id;

		al_priv = disp_al_get_priv(scaler_id);
		if(NULL == al_priv)
			continue;

		screen_id = al_priv->fe_info.out_select;
		if((sel == screen_id) && (1 == al_priv->fe_info.enabled)) {
			scaler_sync(scaler_id);
		}
	}
	return 0;
}
/*
 * @out_select: dram(0xff); be(0..2)
 */
static s32 scaler_output_select(u32 scaler_id, u32 out_select)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

	DE_INF("scaler %d --> %d\n", scaler_id, out_select);
	if(0xff != out_select) {
		DE_SCAL_Output_Select(scaler_id, out_select);
	} else {
		DE_SCAL_Output_Select(scaler_id, 3);
	}
#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&al_data_lock, flags);
#endif
		al_priv->fe_info.out_select = out_select;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&al_data_lock, flags);
	}
#endif
	return 0;
}

static s32 scaler_alpha_is_support(disp_pixel_format format)
{
	switch(format) {
	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_BGRA_8888:
	case DISP_FORMAT_XRGB_8888:
	case DISP_FORMAT_XBGR_8888:
	case DISP_FORMAT_RGBX_8888:
	case DISP_FORMAT_BGRX_8888:
	case DISP_FORMAT_ARGB_4444:
	case DISP_FORMAT_ABGR_4444:
	case DISP_FORMAT_RGBA_4444:
	case DISP_FORMAT_BGRA_4444:
	case DISP_FORMAT_ARGB_1555:
	case DISP_FORMAT_ABGR_1555:
	case DISP_FORMAT_RGBA_5551:
	case DISP_FORMAT_BGRA_5551:
		return 1;
	default :
		return 0;
	}
}

static s32 scaler_set_para(u32 scaler_id, disp_scaler_info *scaler_info)
{
	__scal_buf_addr_t scal_addr;
	__scal_src_size_t in_size;
	__scal_out_size_t out_size;
	__scal_src_type_t in_type;
	__scal_out_type_t out_type;
	__scal_scan_mod_t in_scan;
	__scal_scan_mod_t out_scan;
	u32 screen_index;
	__de_format_t scal_fmt;
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(scaler_id);
	if(NULL == al_priv)
		return -1;

//	DE_INF("scaler %d set para, addr=[0x%x,%x0x,%x0x]\n", scaler_id, (u32)scaler_info->in_fb.addr[0], (u32)scaler_info->in_fb.addr[1], (u32)scaler_info->in_fb.addr[2]);
	screen_index = al_priv->fe_info.out_select;
	scaler_info->out_fb.format = (al_priv->fe_info.out_csc == 1)? DISP_FORMAT_YUV444_P:DISP_FORMAT_ARGB_8888;
	memcpy(&al_priv->fe_info.scaler_info, scaler_info, sizeof(disp_scaler_info));

	scaler_sw_para_to_reg(0, scaler_info->in_fb.format, &scal_fmt);

	in_type.fmt= scal_fmt.fmt;
	in_type.mod= scal_fmt.mod;
	in_type.ps= scal_fmt.ps;
	in_type.br_swap = scal_fmt.br_swap;
	if(((s32)in_type.fmt == DIS_FAIL) || ((s32)in_type.mod== DIS_FAIL)
	    || ((s32)in_type.ps == DIS_FAIL))	{
		DE_WRN("not supported scaler input pixel format: %d\n", scaler_info->in_fb.format);
	}

	in_type.byte_seq = 0;
	in_type.sample_method = 0;

	scal_addr.ch0_addr = (u32)OSAL_VAtoPA((void*)(scaler_info->in_fb.addr[0]));
	scal_addr.ch1_addr = (u32)OSAL_VAtoPA((void*)(scaler_info->in_fb.addr[1]));
	scal_addr.ch2_addr = (u32)OSAL_VAtoPA((void*)(scaler_info->in_fb.addr[2]));

	in_size.src_width = scaler_info->in_fb.size.width;
	in_size.src_height = scaler_info->in_fb.size.height;
	in_size.x_off = scaler_info->in_fb.src_win.x;
	in_size.y_off = scaler_info->in_fb.src_win.y;
	in_size.scal_height= scaler_info->in_fb.src_win.height;
	in_size.scal_width= scaler_info->in_fb.src_win.width;

	scaler_sw_para_to_reg(1, scaler_info->out_fb.format, &scal_fmt);
	out_type.fmt = scal_fmt.fmt;
	out_type.byte_seq = scal_fmt.ps;
	if(scaler_alpha_is_support(scaler_info->in_fb.format))
		out_type.alpha_en = 1;
	else
		out_type.alpha_en = 0;
	out_type.alpha_coef_type = 0;

	out_size.width = scaler_info->out_fb.size.width;
	out_size.height = scaler_info->out_fb.size.height;

	in_scan.field = false;
	in_scan.bottom = false;

	/* todo?  get out scan field */
	out_scan.field = 0;//(gdisp.screen[screen_index].de_flicker_status & DE_FLICKER_USED)?false: gdisp.screen[screen_index].b_out_interlace;

	if(scaler_info->in_fb.cs_mode > DISP_VXYCC)
		scaler_info->in_fb.cs_mode = DISP_BT601;

	if(scaler_info->in_fb.b_trd_src)	{
		__scal_3d_inmode_t inmode;
		__scal_3d_outmode_t outmode = 0;
		__scal_buf_addr_t scal_addr_right;

		if(bsp_disp_feat_get_layer_feats(screen_index, DISP_LAYER_WORK_MODE_SCALER, scaler_id) & DISP_LAYER_FEAT_3D) {
			inmode = scaler_3d_sw_para_to_reg(0, scaler_info->in_fb.trd_mode, 0);
			//todo? FPI
			outmode = scaler_3d_sw_para_to_reg(1, scaler_info->out_fb.trd_mode, 0);

			if(((s32)inmode == DIS_FAIL) || ((s32)outmode == DIS_FAIL))	{
				DE_WRN("input 3d para invalid in Scaler_Set_Para,trd_mode:%d,out_trd_mode:%d\n", scaler_info->in_fb.trd_mode, scaler_info->out_fb.trd_mode);
			}

			DE_SCAL_Get_3D_In_Single_Size(inmode, &in_size, &in_size);
			if(scaler_info->out_fb.b_trd_src)	{
				DE_SCAL_Get_3D_Out_Single_Size(outmode, &out_size, &out_size);
			}

			scal_addr_right.ch0_addr= (u32)OSAL_VAtoPA((void*)(scaler_info->in_fb.trd_right_addr[0]));
			scal_addr_right.ch1_addr= (u32)OSAL_VAtoPA((void*)(scaler_info->in_fb.trd_right_addr[1]));
			scal_addr_right.ch2_addr= (u32)OSAL_VAtoPA((void*)(scaler_info->in_fb.trd_right_addr[2]));

			DE_SCAL_Set_3D_Ctrl(scaler_id, scaler_info->out_fb.b_trd_src, inmode, outmode);
			DE_SCAL_Config_3D_Src(scaler_id, &scal_addr, &in_size, &in_type, inmode, &scal_addr_right);
			DE_SCAL_Agth_Config(scaler_id, &in_type, &in_size, &out_size, 0, scaler_info->out_fb.b_trd_src, outmode);
		}	else {
			DE_WRN("This platform not support 3d output!\n");
			DE_SCAL_Config_Src(scaler_id,&scal_addr,&in_size,&in_type,false,false);
			DE_SCAL_Agth_Config(scaler_id, &in_type, &in_size, &out_size, 0, 0, 0);
		}
	}	else {
		DE_SCAL_Config_Src(scaler_id,&scal_addr,&in_size,&in_type,false,false);
		DE_SCAL_Agth_Config(scaler_id, &in_type, &in_size, &out_size, 0, 0, 0);
	}

	DE_SCAL_Set_Scaling_Factor(scaler_id, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type);
	DE_SCAL_Set_Init_Phase(scaler_id, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, false);
	DE_SCAL_Set_CSC_Coef(scaler_id, scaler_info->in_fb.cs_mode, DISP_BT601,disp_al_get_pixel_type(scaler_info->in_fb.format),
	    disp_al_get_pixel_type(scaler_info->out_fb.format), in_type.br_swap, 0);
	/* todo? smooth support */
	DE_SCAL_Set_Scaling_Coef(scaler_id, &in_scan, &in_size, &in_type, &out_scan, &out_size, &out_type, 0);
	DE_SCAL_Set_Out_Format(scaler_id, &out_type);
	DE_SCAL_Set_Out_Size(scaler_id, &out_scan,&out_type, &out_size);

	return -1;
}

/***********************************************************
 *
 * disp_al_layer
 *
 ***********************************************************/
static s32 disp_al_be_sw_para_to_reg(u32 format, __de_format_t *layer_fmt)
{
	u32 array_size = ARRAY_SIZE(be_input_format_array);
	u32 i;

	if((NULL == layer_fmt) || (0 == array_size))
		return -1;

	memcpy(layer_fmt, &be_input_format_array[0], sizeof(__de_format_t));

	/* scaler input  pixel format */
	for(i=0; i<array_size; i++) {
		if(be_input_format_array[i].format == format) {
			memcpy(layer_fmt, &be_input_format_array[i], sizeof(__de_format_t));
			return 0;
		}
	}

	return -1;
}

s32 disp_al_format_to_bpp(disp_pixel_format fmt)
{
	switch(fmt) {
	case DISP_FORMAT_RGB_565:
	case DISP_FORMAT_BGR_565:
	case DISP_FORMAT_ARGB_1555:
	case DISP_FORMAT_ABGR_1555:
	case DISP_FORMAT_RGBA_5551:
	case DISP_FORMAT_BGRA_5551:
	case DISP_FORMAT_ARGB_4444:
	case DISP_FORMAT_ABGR_4444:
	case DISP_FORMAT_RGBA_4444:
	case DISP_FORMAT_BGRA_4444:
		return 16;

	case DISP_FORMAT_RGB_888:
	case DISP_FORMAT_BGR_888:
		return 24;

	case DISP_FORMAT_ARGB_8888:
	case DISP_FORMAT_ABGR_8888:
	case DISP_FORMAT_RGBA_8888:
	case DISP_FORMAT_BGRA_8888:
	case DISP_FORMAT_XRGB_8888:
	case DISP_FORMAT_XBGR_8888:
	case DISP_FORMAT_RGBX_8888:
	case DISP_FORMAT_BGRX_8888:
		return 32;

	case DISP_FORMAT_YUV444_I_AYUV:
	case DISP_FORMAT_YUV444_I_VUYA:
	case DISP_FORMAT_YUV444_P:
		return 24;

	case DISP_FORMAT_YUV422_I_YVYU:
	case DISP_FORMAT_YUV422_I_YUYV:
	case DISP_FORMAT_YUV422_I_UYVY:
	case DISP_FORMAT_YUV422_I_VYUY:
	case DISP_FORMAT_YUV422_SP_UVUV:
	case DISP_FORMAT_YUV422_SP_VUVU:
	case DISP_FORMAT_YUV422_SP_TILE_UVUV:
	case DISP_FORMAT_YUV422_SP_TILE_VUVU:
	case DISP_FORMAT_YUV422_SP_TILE_128X32_UVUV:
	case DISP_FORMAT_YUV422_SP_TILE_128X32_VUVU:
		return 16;

	case DISP_FORMAT_YUV420_SP_UVUV:
	case DISP_FORMAT_YUV420_SP_VUVU:
	case DISP_FORMAT_YUV420_SP_TILE_UVUV:
	case DISP_FORMAT_YUV420_SP_TILE_VUVU:
	case DISP_FORMAT_YUV420_SP_TILE_128X32_UVUV:
	case DISP_FORMAT_YUV420_SP_TILE_128X32_VUVU:
	case DISP_FORMAT_YUV420_P:
	case DISP_FORMAT_YUV411_P:
	case DISP_FORMAT_YUV411_SP_UVUV:
	case DISP_FORMAT_YUV411_SP_VUVU:
	case DISP_FORMAT_YUV411_SP_TILE_UVUV:
	case DISP_FORMAT_YUV411_SP_TILE_VUVU:
	case DISP_FORMAT_YUV411_SP_TILE_128X32_UVUV:
	case DISP_FORMAT_YUV411_SP_TILE_128X32_VUVU:
		return 12;

	default:
		return 0;
	}
}

s32 disp_al_layer_set_pipe(u32 screen_id, u32 layer_id, u32 pipe)
{
	return DE_BE_Layer_Set_Pipe(screen_id, layer_id, pipe);
}

s32 disp_al_layer_set_zorder(u32 screen_id, u32 layer_id, u32 zorder)
{
	if(zorder > 3) {
		/* support 0~3 zorder, according to the feature of cur plat */
		DE_WRN("not suppot zorder(%d)\n", zorder);
		return -1;
	}
	return DE_BE_Layer_Set_Prio(screen_id, layer_id, zorder);
}

s32 disp_al_layer_set_alpha_mode(u32 screen_id, u32 layer_id, u32 alpha_mode, u32 alpha_value)
{
	s32 ret = 0;
	/*  AW1667 has no mode choose*/
	/* alpha_mode: 0:pixel alpha; 1:global alpha;  2:global_pixel alpha */
	if(1 == alpha_mode) {
		DE_BE_Layer_Set_Alpha_Value(screen_id, layer_id, alpha_value);
		DE_BE_Layer_Alpha_Enable(screen_id, layer_id, 1);
		//DE_BE_Layer_Global_Pixel_Alpha_Enable(screen_id, layer_id, 0);
	} else if(0 == alpha_mode) {
		DE_BE_Layer_Alpha_Enable(screen_id, layer_id, 0);
		//DE_BE_Layer_Global_Pixel_Alpha_Enable(screen_id, layer_id, 0);
	} else if(2== alpha_mode) {
		/* FIXME global pixel alpha */
		DE_BE_Layer_Set_Alpha_Value(screen_id, layer_id, alpha_value);
		DE_BE_Layer_Alpha_Enable(screen_id, layer_id, 1);
		//DE_BE_Layer_Global_Pixel_Alpha_Enable(screen_id, layer_id, 1);
	}else {
		DE_WRN("not support alpha_mode(%d, %d), set to global alpha mode\n", alpha_mode, alpha_value);
		DE_BE_Layer_Set_Alpha_Value(screen_id, layer_id, alpha_value);
		DE_BE_Layer_Alpha_Enable(screen_id, layer_id, 1);
		//DE_BE_Layer_Global_Pixel_Alpha_Enable(screen_id, layer_id, 0);
		ret = -1;
	}

	return ret;
}

s32 disp_al_layer_color_key_enable(u32 screen_id, u32 layer_id, u32 enable)
{
	return DE_BE_Layer_ColorKey_Enable(screen_id, layer_id, enable);
}

s32 disp_al_layer_set_screen_window(u32 screen_id, u32 layer_id, disp_window * window)
{
	return DE_BE_Layer_Set_Screen_Win(screen_id, layer_id, window);
}

s32 disp_al_layer_set_framebuffer(u32 screen_id, u32 layer_id, disp_fb_info *fb)
{
	layer_src_t layer_fb;
	__de_format_t layer_fmt;

	memset(&layer_fb, 0, sizeof(layer_src_t));
	layer_fb.fb_addr    = (u32)OSAL_VAtoPA((void*)fb->addr[0]);
	if(0 != disp_al_be_sw_para_to_reg(fb->format, &layer_fmt)) {
		DE_WRN("not support fmt!\n");
	}
	layer_fb.format     = layer_fmt.fmt;
	layer_fb.pixseq     = layer_fmt.ps;
	layer_fb.br_swap    = layer_fmt.br_swap;
	layer_fb.fb_width   = fb->size.width;
	layer_fb.offset_x   = fb->src_win.x;
	layer_fb.offset_y   = fb->src_win.y;
	layer_fb.pre_multiply = fb->pre_multiply;
	return DE_BE_Layer_Set_Framebuffer(screen_id, layer_id, &layer_fb);
}

s32 disp_al_layer_enable(u32 screen_id, u32 layer_id, u32 enable)
{
	s32 ret = 0;
	if(1 == enable)
		ret = DE_BE_Layer_Enable(screen_id, layer_id, 1);
	else
		ret = DE_BE_Layer_Enable(screen_id, layer_id, 0);
	DE_BE_Layer_Set_Work_Mode(screen_id, layer_id, 0);

	return ret;
}

s32 disp_al_layer_set_extra_info(u32 screen_id, u32 layer_id, disp_layer_info *info, __disp_layer_extra_info_t *extra_info)
{
	disp_scaler_info scaler_info;
	s32 scaler_id = extra_info->scaler_id;
	__disp_al_private_data *al_priv;

	if(DISP_LAYER_WORK_MODE_SCALER == info->mode) {
		if(extra_info->b_scaler_mode) {
			/* scaler --> scaler */
			al_priv = disp_al_get_priv(scaler_id);
			if(NULL == al_priv)
				return -1;
		} else {
			/* !scaler --> scaler */
			scaler_id = scaler_request(0xff);
			if(scaler_id < 0) {
				DE_WRN("no source!\n");
				return -1;
			}

			al_priv = disp_al_get_priv(scaler_id);
			if(NULL == al_priv)
				return -1;

			extra_info->b_scaler_mode = 1;
			extra_info->scaler_id = scaler_id;
		}
		DE_BE_Layer_Video_Enable(screen_id, layer_id, true);
		DE_BE_Layer_Video_Ch_Sel(screen_id, layer_id, scaler_id);
		scaler_output_select(scaler_id, screen_id);

		if(1 != extra_info->b_scaler_mode)
			DE_WRN("something error\n");

		memset(&scaler_info, 0, sizeof(disp_scaler_info));
		memcpy(&scaler_info.in_fb, &info->fb, sizeof(disp_fb_info));
		memcpy(&scaler_info.out_fb.src_win, &info->screen_win, sizeof(disp_window));
		scaler_info.out_fb.size.width = info->screen_win.width;
		scaler_info.out_fb.size.height = info->screen_win.height;
		scaler_info.out_fb.addr[0] = 0;
		scaler_info.out_fb.format = DISP_FORMAT_ARGB_8888;
		scaler_info.out_fb.b_trd_src = info->b_trd_out;
		scaler_info.out_fb.trd_mode = (disp_3d_src_mode)info->out_trd_mode;
		scaler_set_para(scaler_id, &scaler_info);
	} else {
		if(extra_info->b_scaler_mode) {
			/* scaler --> !scaler */
			scaler_release(extra_info->scaler_id, true);
			extra_info->b_scaler_mode = 0;
			DE_BE_Layer_Video_Enable(screen_id, layer_id, false);
			DE_BE_Layer_Video_Ch_Sel(screen_id, layer_id, 0);
		} else {
			/* !scaler --> !scaler */
		}
	}

	return 0;
}

s32 disp_al_layer_sync(u32 screen_id, u32 layer_id, __disp_layer_extra_info_t *extra_info)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;
//
//	if(extra_info->b_scaler_mode) {
//		scaler_sync(extra_info->scaler_id);
//	}

	return 0;
}

//capture
s32 disp_al_capture_init(u32 screen_id)
{
	WB_EBIOS_Init(screen_id);
	return 0;
}

s32 disp_al_capture_screen_proc(u32 screen_id)
{
	s32 ret = 0;
	__disp_al_private_data *al_priv;
	al_priv = disp_al_get_priv(screen_id);

	if((al_priv->capture_info.capture_para.capture_request == 1)) {
		if(al_priv->capture_info.switch_buffer_counter == 0) {
			al_priv->capture_info.switch_buffer_counter = 1;
		}
		else {
			if(al_priv->capture_info.capture_para.fps == 1) {
				al_priv->capture_info.fps_counter++;
			}
			if((al_priv->capture_info.capture_para.fps == 0) || ((al_priv->capture_info.capture_para.fps == 1) &&
				(al_priv->capture_info.fps_counter & 0x1)))
				disp_al_capture_screen_switch_buff(screen_id);
		}
	}

	return ret;
}

s32 disp_al_capture_screen_switch_buff(u32 screen_id)
{
	s32 ret = 0;
	u32 buffer_num;
	u32 cur_buffer_id;
	disp_size in_size;
	disp_window capture_window, output_window;

	__disp_al_private_data *al_priv;
	al_priv = disp_al_get_priv(screen_id);

	if(al_priv->capture_info.capture_para.capture_request == 1) {
		if((al_priv->capture_info.capture_para.mode == 1)) {
			cur_buffer_id = al_priv->capture_info.capture_para.cur_buffer_id;
			buffer_num = al_priv->capture_info.capture_para.buffer_num;

			if(buffer_num > 1) {
				cur_buffer_id++;
				cur_buffer_id = (cur_buffer_id ==  buffer_num) ? 0 : cur_buffer_id;
			}

			al_priv->capture_info.capture_para.cur_buffer_id = cur_buffer_id;

			if(bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_NONE) {
				in_size.width = bsp_disp_get_screen_width(screen_id);
				in_size.height = bsp_disp_get_screen_height(screen_id);
			} else {
				in_size.width = al_priv->capture_info.capture_para.screen_size.width;
				in_size.height = al_priv->capture_info.capture_para.screen_size.height;
			}

			capture_window.x = 0;
			capture_window.y = 0;
			capture_window.width = al_priv->capture_info.capture_para.screen_size.width;
			capture_window.height = al_priv->capture_info.capture_para.screen_size.height;

			output_window.x = 0;
			output_window.y = 0;
			output_window.width = al_priv->capture_info.capture_para.output_fb[cur_buffer_id].size.width;
			output_window.height = al_priv->capture_info.capture_para.output_fb[cur_buffer_id].size.height;

			WB_EBIOS_Writeback_Disable(screen_id);
			ret = WB_EBIOS_Set_Para(screen_id, in_size, capture_window, output_window,
									al_priv->capture_info.capture_para.output_fb[cur_buffer_id]);
			WB_EBIOS_Writeback_Enable(screen_id);

		}
		else {
			disp_al_capture_screen_stop(screen_id);
		}
	}
	return ret;
}

s32 disp_al_capture_screen(u32 screen_id, disp_capture_para *para)
{
	disp_size in_size;
	disp_window capture_window, output_window;
	s32 ret = 0;

	__disp_al_private_data *al_priv;

	if(para==NULL) {
		DE_WRN("input parameter can't be null!\n");
		return DIS_FAIL;
	}

	al_priv = disp_al_get_priv(screen_id);
	if(al_priv == NULL) {
		DE_WRN("disp al get private data is NULL!\n");
		return -1;
	}

	if(al_priv->capture_info.capture_para.capture_request == 1) {
		DE_WRN("capture is working!\n");
		return -1;
	}

	memset(&al_priv->capture_info.capture_para, 0, sizeof(disp_capture_para));
	memcpy(&al_priv->capture_info.capture_para, para, sizeof(disp_capture_para));

	al_priv->capture_info.capture_para.cur_buffer_id = 0;

	//init fps
	if((al_priv->capture_info.capture_para.mode == 1) && (al_priv->capture_info.capture_para.fps == 1))
		al_priv->capture_info.fps_counter = 0;

	if(bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_NONE) {
		in_size.width = bsp_disp_get_screen_width(screen_id);
		in_size.height = bsp_disp_get_screen_height(screen_id);
	} else {
		in_size.width = para->screen_size.width;
		in_size.height = para->screen_size.height;
	}

	//capture hole data
	capture_window.x = 0;
	capture_window.y = 0;
	capture_window.width = para->screen_size.width;
	capture_window.height = para->screen_size.height;

	output_window.x = 0;
	output_window.y = 0;
	output_window.width = para->output_fb[0].size.width;
	output_window.height = para->output_fb[0].size.height;

	WB_EBIOS_Enable(screen_id);
	ret = WB_EBIOS_Set_Para(screen_id, in_size, capture_window, output_window, para->output_fb[0]);
	if(ret != 0) {
		DE_WRN("WB_EBIOS_Set_Para fail, ret=%d\n", ret);
		WB_EBIOS_Reset(screen_id);
		return -1;
	}
	WB_EBIOS_Writeback_Enable(screen_id);

#if defined(__LINUX_PLAT__)
				{
					unsigned long flags;
					spin_lock_irqsave(&al_data_lock, flags);
#endif
			al_priv->capture_info.capture_para.capture_request = 1;
			al_priv->capture_info.switch_buffer_counter = 0;
#if defined(__LINUX_PLAT__)
					spin_unlock_irqrestore(&al_data_lock, flags);
				}
#endif


	return 0;
}

s32 disp_al_capture_screen_stop(u32 screen_id)
{
	s32 ret = 0;

	__disp_al_private_data *al_priv;
	al_priv = disp_al_get_priv(screen_id);

	if(al_priv->capture_info.capture_para.capture_request == 1) {
#if defined(__LINUX_PLAT__)
		{
			unsigned long flags;
			spin_lock_irqsave(&al_data_lock, flags);
#endif
			al_priv->capture_info.capture_para.capture_request = 0;
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&al_data_lock, flags);
		}
#endif
		WB_EBIOS_Reset(screen_id);
		WB_EBIOS_Writeback_Disable(screen_id);
	}
	return ret;
}

s32 disp_al_capture_screen_get_buffer_id(u32 screen_id)
{
	__disp_al_private_data *al_priv;
	al_priv = disp_al_get_priv(screen_id);

	return al_priv->capture_info.capture_para.cur_buffer_id;
}

s32 disp_al_capture_sync(u32 screen_id)
{
	__disp_al_private_data *al_priv;
	al_priv = disp_al_get_priv(screen_id);

	if(al_priv == NULL) {
		DE_WRN("get capture priv fail\n");
		return -1;
	}
/*
	//capture event
	if(WB_EBIOS_Get_Status(screen_id) == 0) {
		WB_EBIOS_ClearINT(screen_id);
		gdisp.init_para.capture_event(screen_id);
	}
*/
	if(al_priv->capture_info.capture_para.capture_request == 1) {
		disp_al_capture_screen_proc(screen_id);
		WB_EBIOS_Set_Reg_Rdy(screen_id);
	}

	return 0;
}

s32 disp_al_caputure_screen_finished(u32 screen_id)
{
	__disp_al_private_data *al_priv;
	al_priv = disp_al_get_priv(screen_id);

	if(al_priv == NULL) {
		DE_WRN("get capture priv fail\n");
		return -1;
	}

	if(al_priv->capture_info.capture_para.capture_request == 1) {
		if(WB_EBIOS_Get_Status(screen_id) == 0) {
			WB_EBIOS_ClearINT(screen_id);
			return 1;
		}
	}
	return 0;
}

s32 disp_al_smbl_enable(u32 screen_id, u32 enable)
{
	return IEP_Drc_Enable(screen_id, enable);
}

s32 disp_al_smbl_set_window(u32 screen_id, disp_window *window)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	if((0 == window->width) || (0 == window->height)) {
		window->width = al_priv->drc_info.in_width;
		window->height = al_priv->drc_info.in_height;
	}
	return IEP_Drc_Set_Winodw(screen_id, *window);
}

s32 disp_al_smbl_sync(u32 screen_id)
{
	return IEP_Drc_Operation_In_Vblanking(screen_id);
}

s32 disp_al_smbl_tasklet(u32 screen_id)
{
	IEP_Drc_Tasklet(screen_id);

	return 0;
}

s32 disp_al_smbl_update_backlight(u32 screen_id, u32 bl)
{
	return IEP_Drc_Update_Backlight(screen_id, bl);
}

s32 disp_al_smbl_get_backlight_dimming(u32 screen_id)
{
	return IEP_Drc_Get_Backlight_Dimming(screen_id);
}

s32 disp_al_smcl_clk_init(u32 clk_id)
{
	return 0;
}

s32 disp_al_smcl_clk_exit(u32 clk_id)
{
	return 0;
}

s32 disp_al_smcl_clk_enable(u32 clk_id, u32 enable)
{
	return 0;
}

s32 disp_al_smcl_set_para(u32 screen_id, u32 brightness, u32 saturation, u32 hue, u32 mode)
{
	return 0;
}

s32 disp_al_smcl_set_window(u32 screen_id, disp_window *window)
{
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	memcpy(&al_priv->sat_info.window, window, sizeof(disp_window));

	return SAT_Set_Window(screen_id, window);
}

s32 disp_al_smcl_enable(u32 screen_id, u32 enable)
{
	u32 data[2];
	__disp_al_private_data *al_priv;

	al_priv = disp_al_get_priv(screen_id);
	if(NULL == al_priv)
		return -1;

	enable = enable ? 1 : 0;

	if(enable)
		data[0] = DISP_OUT_CSC_TYPE_SAT;
	else
		data[0] = DISP_OUT_CSC_TYPE_LCD;

	data[1] = DISP_COLOR_RANGE_0_255;
	disp_al_notifier_call_chain(DISP_EVENT_SAT_CSC, screen_id, (void*)data);

	return SAT_Enable_Disable(screen_id, enable);
}

s32 disp_al_smcl_sync(u32 screen_id)
{
	return SAT_Set_Sync_Para(screen_id);
}

#if defined(CONFIG_ARCH_SUN6I) || defined(CONFIG_ARCH_SUN8IW1P1) || defined(CONFIG_ARCH_SUN9IW1P1)
static __disp_al_mod_init_data mod_init_data[] =
{
	{DISP_MOD_FE0      , "fe0"  ,0, &DE_SCAL_Set_Reg_Base             ,&disp_al_fe_notifier_callback ,},
	{DISP_MOD_FE1      , "fe1"  ,1, &DE_SCAL_Set_Reg_Base             ,NULL                          ,},
	//{DISP_MOD_FE2      , "fe2" , 2, &DE_SCAL_Set_Reg_Base           ,NULL                          ,},
	{DISP_MOD_BE0      , "be0"  ,0, &DE_Set_Reg_Base                  ,&disp_al_be_notifier_callback ,},
	{DISP_MOD_BE1      , "be1"  ,1, &DE_Set_Reg_Base                  ,NULL                          ,},
	//{DISP_MOD_BE2      , be2" ,2, &DE_Set_Reg_Base                ,NULL                         ,},
	{DISP_MOD_LCD0     , "lcd0" ,0, &tcon_set_reg_base                ,NULL                          ,},
	{DISP_MOD_LCD1     , "lcd1" ,1, &tcon_set_reg_base                ,NULL                          ,},
	{DISP_MOD_DEU0     , "deu0" ,0, &IEP_Deu_Set_Reg_base             ,&disp_al_deu_notifier_callback,},
	{DISP_MOD_DEU1     , "deu1" ,1, &IEP_Deu_Set_Reg_base             ,NULL                          ,},
	{DISP_MOD_CMU0     , "cmu0" ,0, &IEP_CMU_Set_Reg_Base             ,&disp_al_cmu_notifier_callback,},
	{DISP_MOD_CMU1     , "cmu1" ,1, &IEP_CMU_Set_Reg_Base             ,NULL                          ,},
	{DISP_MOD_DRC0     , "drc0" ,0, &IEP_Drc_Set_Reg_Base             ,&disp_al_drc_notifier_callback,},
	{DISP_MOD_DRC1     , "drc1" ,1, &IEP_Drc_Set_Reg_Base             ,NULL                          ,},
	{DISP_MOD_DSI0     , "dsi0" ,0, &dsi_set_reg_base                 ,NULL                          ,},
	{DISP_MOD_HDMI     , "hdmi" ,0, NULL                              ,NULL                          ,},
	{DISP_MOD_EDP      , "edp"  ,0, NULL                              ,NULL                          ,},
	{DISP_MOD_TOP      , "top"  ,0, &de_top_set_reg_base              ,NULL                          ,},
};
#elif defined(CONFIG_ARCH_SUN8IW3P1)
static __disp_al_mod_init_data mod_init_data[] =
{
	{DISP_MOD_FE0      , "fe0"  ,0, &DE_SCAL_Set_Reg_Base             ,&disp_al_fe_notifier_callback ,},
	{DISP_MOD_BE0      , "be0"  ,0, &DE_Set_Reg_Base                  ,&disp_al_be_notifier_callback ,},
	{DISP_MOD_LCD0     , "lcd0" ,0, &tcon_set_reg_base                ,NULL                          ,},
	{DISP_MOD_CMU0     , "cmu0" ,0, &IEP_CMU_Set_Reg_Base             ,&disp_al_cmu_notifier_callback,},
	{DISP_MOD_DRC0     , "drc0" ,0, &IEP_Drc_Set_Reg_Base             ,&disp_al_drc_notifier_callback,},
	{DISP_MOD_WB0      , "wb0"  ,0, &WB_EBIOS_Set_Reg_Base            ,NULL                          ,},
	{DISP_MOD_DSI0     , "dsi0" ,0, &dsi_set_reg_base                 ,NULL                          ,},
	{DISP_MOD_HDMI     , "hdmi" ,0, NULL                              ,NULL                          ,},
};
#elif defined(CONFIG_ARCH_SUN8IW5P1)
static __disp_al_mod_init_data mod_init_data[] =
{
	{DISP_MOD_FE0      , "fe0"  ,0, &DE_SCAL_Set_Reg_Base             ,&disp_al_fe_notifier_callback ,},
	{DISP_MOD_BE0      , "be0"  ,0, &DE_Set_Reg_Base                  ,&disp_al_be_notifier_callback ,},
	{DISP_MOD_LCD0     , "lcd0" ,0, &tcon_set_reg_base                ,NULL                          ,},
	{DISP_MOD_DRC0     , "drc0" ,0, &IEP_Drc_Set_Reg_Base             ,&disp_al_drc_notifier_callback,},
	{DISP_MOD_WB0      , "wb0"  ,0, &WB_EBIOS_Set_Reg_Base            ,NULL                          ,},
	{DISP_MOD_DSI0     , "dsi0" ,0, &dsi_set_reg_base                 ,NULL                          ,},
	{DISP_MOD_SAT0     , "sat0" ,0, &SAT_Set_Reg_Base                 ,&disp_al_sat_notifier_callback,},
};

#endif

s32 disp_init_al(__disp_bsp_init_para * para)
{
	u32 num_screens;
	u32 i;
	__disp_al_mod_list_t *new_mod;
	//struct dsi_init_para dsi_para;

	DE_INF("disp_init_al\n");

	num_screens = bsp_disp_feat_get_num_screens();
	al_private = (__disp_al_private_data *)OSAL_malloc(sizeof(__disp_al_private_data) * num_screens);
	if(NULL == al_private) {
		DE_WRN("malloc memory fail!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
	spin_lock_init(&al_data_lock);
#endif
	disp_al_notifier_init();

	/* add all disp mod exist in this plat into mod_list */
	INIT_LIST_HEAD(&mod_list.list);
	for(i=0; i<ARRAY_SIZE(mod_init_data); i++) {
		new_mod = (__disp_al_mod_list_t*)OSAL_malloc(sizeof(__disp_al_mod_list_t));
		new_mod->mod_id = mod_init_data[i].mod_id;
		list_add_tail(&(new_mod->list), &(mod_list.list));
		/* set reg base of all mod at current plat */
		if(mod_init_data[i].set_reg_base)
			mod_init_data[i].set_reg_base(mod_init_data[i].screen_id, para->reg_base[mod_init_data[i].mod_id]);
			DE_INF("%s(%d), reg_base=0x%x\n", mod_init_data[i].name, mod_init_data[i].mod_id, para->reg_base[mod_init_data[i].mod_id]);
		if(mod_init_data[i].notifier_call) {
			struct disp_notifier_block *nb = (struct disp_notifier_block *)OSAL_malloc(sizeof(struct disp_notifier_block));
			if(nb) {
				nb->notifier_call = mod_init_data[i].notifier_call;
				disp_al_notifier_register(nb);
			} else
				DE_WRN("malloc memory fail!\n");
		}
	}

	/* do some special init for modules needed */
	//init pll
	osal_init_clk_pll();

	scaler_init(para);

	//dsi_para.delay_ms = bsp_disp_delay_ms;
	//dsi_para.delay_us = bsp_disp_delay_us;
	//dsi_init(&dsi_para);

	return 0;
}

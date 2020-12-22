#include "disp_layer.h"

struct disp_layer_private_data {
	bool user_info_dirty;
	disp_layer_info user_info;

	bool info_dirty;
	disp_layer_info info;

	bool shadow_info_dirty;

	__disp_layer_extra_info_t extra_info;

	bool enabled;
	u32  frame_shadow_id;//the id of frame at the shadow register now
	u32  frame_id;//the id of frame display now

	s32 (*shadow_protect)(u32 sel, bool protect);
};

#if defined(__LINUX_PLAT__)
static spinlock_t lyr_data_lock;
#endif

static struct disp_layer *lyrs = NULL;
static struct disp_layer_private_data *lyr_private;

struct disp_layer* disp_get_layer(u32 screen_id, u32 layer_id)
{
	u32 num_screens, max_num_layers, num_layers;

	num_screens = bsp_disp_feat_get_num_screens();
	max_num_layers = bsp_disp_feat_get_num_layers(0);
	num_layers = bsp_disp_feat_get_num_layers(screen_id);
	if((screen_id > num_screens) || (layer_id > num_layers)) {
		DE_WRN("screen_id %d or layer_id %d is out of range\n", screen_id, layer_id);
		return NULL;
	}

	if(!disp_al_query_be_mod(screen_id)) {
		DE_WRN("be %d is not registered\n", screen_id);
		return NULL;
	}

	return &lyrs[screen_id * max_num_layers + layer_id];
}
static struct disp_layer_private_data *disp_lyr_get_priv(struct disp_layer *lyr)
{
	u32 num_screens, max_num_layers;

	if(NULL == lyr) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	if(!disp_al_query_be_mod(lyr->channel_id)) {
		DE_WRN("be %d is not registered\n", lyr->channel_id);
		return NULL;
	}

	num_screens = bsp_disp_feat_get_num_screens();
	max_num_layers = bsp_disp_feat_get_num_layers(0);

	return &lyr_private[lyr->channel_id * max_num_layers + lyr->layer_id];
}


static s32 disp_lyr_init(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return 0;
}

static s32 disp_lyr_exit(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return 0;
}

static s32 disp_lyr_enable(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(lyr->manager && lyr->manager->is_enabled && lyr->manager->is_enabled(lyr->manager)) {
		disp_al_layer_enable(lyr->channel_id, lyr->layer_id, 1);
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		lyrp->enabled = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif

	return 0;
}

/* need? */
static s32 disp_lyr_disable(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(lyr->manager && lyr->manager->is_enabled && lyr->manager->is_enabled(lyr->manager)) {
		disp_al_layer_enable(lyr->channel_id, lyr->layer_id, 0);
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		lyrp->enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif

	return 0;
}

static bool disp_lyr_is_enabled(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return false;
	}

	return lyrp->enabled;
}

static s32 disp_lyr_sync(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);
	bool swap = false;

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	disp_al_layer_sync(lyr->channel_id, lyr->layer_id, &lyrp->extra_info);
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		if(lyrp->frame_id != lyrp->frame_shadow_id)
			swap = true;
		lyrp->frame_id = lyrp->frame_shadow_id;
		lyrp->shadow_info_dirty = false;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 disp_lyr_update_regs(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);
	disp_layer_info layer_info;
	u32 size, bpp;

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("disp_lyr_update_regs, mgr%d lyr%d\n", lyr->channel_id, lyr->layer_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		if(!lyr->manager || !lyr->manager->is_enabled || !lyr->manager->is_enabled(lyr->manager) || !lyrp->info_dirty) {
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&lyr_data_lock, flags);
#endif
			return DIS_SUCCESS;
		}
		memcpy(&layer_info, &lyrp->info, sizeof(disp_layer_info));
		DE_INF("update_regs ok, frame_id=%d!\n", layer_info.id);
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif
	bpp = disp_al_format_to_bpp(layer_info.fb.format);
	size = (layer_info.fb.size.width * layer_info.screen_win.height * bpp + 7)/8;
	disp_lyr_shadow_protect(lyr, 1);
	OSAL_CacheRangeFlush((void *)layer_info.fb.addr[0], size,CACHE_CLEAN_FLUSH_D_CACHE_REGION);
	//FIXME zorder
	disp_al_layer_set_zorder(lyr->channel_id, lyr->layer_id, lyr->layer_id);
	disp_al_layer_set_pipe(lyr->channel_id, lyr->layer_id, layer_info.pipe);
	disp_al_layer_set_alpha_mode(lyr->channel_id, lyr->layer_id, layer_info.alpha_mode, layer_info.alpha_value);
	disp_al_layer_color_key_enable(lyr->channel_id, lyr->layer_id, layer_info.ck_enable);
	disp_al_layer_set_screen_window(lyr->channel_id,lyr->layer_id, &layer_info.screen_win);
	disp_al_layer_enable(lyr->channel_id, lyr->layer_id, lyrp->enabled);
	if((NULL == lyr->p_sw_init_flag) || (0 == *(lyr->p_sw_init_flag))) {
		disp_al_layer_set_extra_info(lyr->channel_id, lyr->layer_id, &layer_info, &lyrp->extra_info);
	} else {
#if defined(CONFIG_HOMLET_PLATFORM)
		disp_al_layer_set_extra_info_sw(lyr->channel_id, lyr->layer_id, &layer_info, &lyrp->extra_info);
#endif
	}
	if(layer_info.mode != 0)
		layer_info.fb.format = DISP_FORMAT_ARGB_8888;
	disp_al_layer_set_framebuffer(lyr->channel_id, lyr->layer_id,&layer_info.fb);
	disp_lyr_shadow_protect(lyr, 0);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		lyrp->info_dirty = false;
		lyrp->shadow_info_dirty = true;
		lyrp->frame_shadow_id = layer_info.id;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 disp_lyr_apply(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	__debug("disp_lyr_apply, mgr%d lyr%d\n", lyr->channel_id, lyr->layer_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
	if(lyrp->user_info_dirty) {
		memcpy(&lyrp->info, &lyrp->user_info, sizeof(disp_layer_info));
		lyrp->user_info_dirty = false;
		lyrp->info_dirty = true;
	}
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif
	disp_lyr_update_regs(lyr);

	return DIS_SUCCESS;
}

static s32 disp_lyr_force_update_regs(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	__inf("disp_lyr_force_update_regs, mgr%d lyr%d\n", lyr->channel_id, lyr->layer_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		lyrp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif
		return disp_lyr_apply(lyr);
}

static s32 disp_lyr_clear_regs(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	__inf("disp_lyr_clear_regs, mgr%d lyr%d\n", lyr->channel_id, lyr->layer_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		lyrp->info_dirty = true;
		memset(&lyrp->info, 0, sizeof(disp_layer_info));
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif
		return disp_lyr_update_regs(lyr);
}

static s32 disp_lyr_set_info(struct disp_layer *lyr, disp_layer_info *player_info)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("mgr %d lyr %d, set_info, frame_id %d\n", lyr->channel_id, lyr->layer_id, player_info->id);
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		memcpy(&lyrp->user_info, player_info, sizeof(disp_layer_info));
		lyrp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif
	disp_lyr_apply(lyr);

	return 0;
}

static s32 disp_lyr_get_info(struct disp_layer *lyr, disp_layer_info *player_info)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		memcpy(player_info, &lyrp->user_info, sizeof(disp_layer_info));
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif

  return 0;
}

static s32 disp_lyr_get_frame_id(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);
	u32 id;

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		id = lyrp->frame_id;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif

  return id;
}

static s32 disp_lyr_set_manager(struct disp_layer *lyr, struct disp_manager *mgr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp) || (NULL == mgr)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&lyr_data_lock, flags);
#endif
		lyr->manager = mgr;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&lyr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

s32 disp_lyr_shadow_protect(struct disp_layer *lyr, bool protect)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(lyrp->shadow_protect)
		return lyrp->shadow_protect(lyr->channel_id, protect);

	return -1;
}

s32 disp_lyr_deinterlace_cfg(struct disp_layer *lyr)
{
	u32 scaler_id;
	disp_layer_info *info;
	disp_scaler_info scaler_info;
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if((NULL == lyr) || (NULL == lyrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(0 != lyrp->extra_info.b_scaler_mode) {
		scaler_id = lyrp->extra_info.scaler_id;
		info = &(lyrp->info);
		memset(&scaler_info, 0, sizeof(disp_scaler_info));
		memcpy(&scaler_info.in_fb, &info->fb, sizeof(disp_fb_info));
		memcpy(&scaler_info.out_fb.src_win, &info->screen_win, sizeof(disp_window));
		scaler_info.out_fb.size.width = info->screen_win.width;
		scaler_info.out_fb.size.height = info->screen_win.height;
		scaler_info.out_fb.addr[0] = 0;
		scaler_info.out_fb.format = DISP_FORMAT_ARGB_8888;
		scaler_info.out_fb.b_trd_src = info->b_trd_out;
		scaler_info.out_fb.trd_mode = (disp_3d_src_mode)info->out_trd_mode;
#if defined(CONFIG_HOMLET_PLATFORM)
		disp_al_deinterlace_cfg(scaler_id, &scaler_info);
#endif
		lyrp->shadow_info_dirty = true;
	}

	return 0;
}

s32 disp_init_lyr(__disp_bsp_init_para * para)
{
	u32 num_screens, num_layers;
	u32 max_num_layers;
	u32 screen_id, layer_id;

	DE_INF("disp_init_lyr\n");

#if defined(__LINUX_PLAT__)
	spin_lock_init(&lyr_data_lock);
#endif
	num_screens = bsp_disp_feat_get_num_screens();
	max_num_layers = bsp_disp_feat_get_num_layers(0);
	lyrs = (struct disp_layer *)OSAL_malloc(sizeof(struct disp_layer) * num_screens * max_num_layers);
	if(NULL == lyrs) {
		DE_WRN("malloc memory fail! size=0x%x x %d\n", sizeof(struct disp_layer), max_num_layers);
		return DIS_FAIL;
	}
	DE_INF("malloc memory ok! size=0x%x x %d\n", sizeof(struct disp_layer), max_num_layers);

	lyr_private = (struct disp_layer_private_data *)OSAL_malloc(sizeof(struct disp_layer_private_data) * num_screens * max_num_layers);
	if(NULL == lyr_private) {
		DE_WRN("malloc memory fail! size=0x%x x %d\n", sizeof(struct disp_layer_private_data), max_num_layers);
		return DIS_FAIL;
	}
	DE_INF("malloc memory ok! size=0x%x x %d\n", sizeof(struct disp_layer_private_data), max_num_layers);

	for(screen_id=0; screen_id<num_screens; screen_id++) {
		if(!disp_al_query_be_mod(screen_id))
			continue;

		num_layers = bsp_disp_feat_get_num_layers(screen_id);
		for(layer_id=0; layer_id<num_layers; layer_id++) {
			struct disp_layer *lyr = &lyrs[screen_id * max_num_layers + layer_id];
			struct disp_layer_private_data *lyrp = &lyr_private[screen_id * max_num_layers + layer_id];

			lyrp->shadow_protect = para->shadow_protect;

			sprintf(lyr->name, "mgr%d lyr%d", screen_id, layer_id);
			lyr->channel_id = screen_id;
			lyr->layer_id = layer_id;
			lyr->p_sw_init_flag = ((1 << screen_id) & para->sw_init_para->sw_init_flag) ?
				(&(para->sw_init_para->sw_init_flag)) : NULL;

			lyr->enable = disp_lyr_enable;
			lyr->disable = disp_lyr_disable;
			lyr->is_enabled = disp_lyr_is_enabled;
			lyr->set_info = disp_lyr_set_info;
			lyr->get_info = disp_lyr_get_info;
			lyr->set_manager = disp_lyr_set_manager;
			lyr->init = disp_lyr_init;
			lyr->exit = disp_lyr_exit;

			lyr->apply = disp_lyr_apply;
			lyr->update_regs = disp_lyr_update_regs;
			lyr->force_update_regs = disp_lyr_force_update_regs;
			lyr->clear_regs = disp_lyr_clear_regs;
			lyr->sync = disp_lyr_sync;
			lyr->get_frame_id = disp_lyr_get_frame_id;
			lyr->deinterlace_cfg = disp_lyr_deinterlace_cfg;

			lyr->init(lyr);
		}
	}

	return 0;
}

//for coping bootlogo to fb0
u32 disp_layer_get_addr(u32 sel, u32 hid)
{
#if defined(CONFIG_HOMLET_PLATFORM)
    return disp_al_layer_get_addr(sel, hid);
#else
	return 0;
#endif
}

u32 disp_layer_set_addr(u32 sel, u32 hid, u32 addr)
{
#if defined(CONFIG_HOMLET_PLATFORM)
	return disp_al_layer_set_addr(sel, hid, addr);
#else
	return 0;
#endif
}

u32 disp_layer_get_in_width(u32 sel, u32 hid)
{
#if defined(CONFIG_HOMLET_PLATFORM)
    return disp_al_layer_get_inWidth(sel, hid);
#else
	return 0;
#endif
}

u32 disp_layer_get_in_height(u32 sel, u32 hid)
{
#if defined(CONFIG_HOMLET_PLATFORM)
    return disp_al_layer_get_inHeight(sel, hid);
#else
	return 0;
#endif
}


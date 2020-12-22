#include "disp_manager.h"

struct disp_manager_info {
	disp_color_info back_color;
	disp_colorkey ck;
};

struct disp_manager_private_data {
	bool user_info_dirty;
	struct disp_manager_info user_info;

	bool info_dirty;
	struct disp_manager_info info;

	bool shadow_info_dirty;

	/* If true, a display is enabled using this manager */
	bool enabled;

	bool extra_info_dirty;//??
	bool shadow_extra_info_dirty;//??

	disp_output_type output;
	u32 width, height;

	u32 de_flicker_status;
	bool b_outinterlerlace;
	disp_out_csc_type output_csc_type;

	s32 (*shadow_protect)(u32 sel, bool protect);

	u32 reg_base;
	u32 irq_no;
	disp_clk_info_t  clk;
};

#if defined(__LINUX_PLAT__)
static spinlock_t mgr_data_lock;
#endif

static struct disp_manager *mgrs = NULL;
static struct disp_manager_private_data *mgr_private;

struct disp_manager* disp_get_layer_manager(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens) {
		DE_WRN("screen_id %d out of range\n", screen_id);
		return NULL;
	}

	if(!disp_al_query_be_mod(screen_id)) {
		DE_WRN("manager %d is not registered\n", screen_id);
		return NULL;
	}

	return &mgrs[screen_id];
}
static struct disp_manager_private_data *disp_mgr_get_priv(struct disp_manager *mgr)
{
	if(NULL == mgr) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	if(!disp_al_query_be_mod(mgr->channel_id)) {
		DE_WRN("manager %d is not registered\n", mgr->channel_id);
		return NULL;
	}

	return &mgr_private[mgr->channel_id];
}

static s32 disp_mgr_clk_init(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	disp_al_manager_clk_init(mgrp->clk.clk);

	return 0;
}

static s32 disp_mgr_clk_exit(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("mgr %d clk exit\n", mgr->channel_id);

	disp_al_manager_clk_exit(mgrp->clk.clk);

	mgrp->clk.h_clk = 0;
#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&mgr_data_lock, flags);
#endif
			mgrp->clk.enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_mgr_clk_enable(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("mgr %d clk enable\n", mgr->channel_id);

	disp_al_manager_clk_enable(mgrp->clk.clk);

#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&mgr_data_lock, flags);
#endif
			mgrp->clk.enabled = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif
	return 0;
}

s32 disp_mgr_clk_enable_sw(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("mgr %d clk enable\n", mgr->channel_id);

#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&mgr_data_lock, flags);
#endif
			mgrp->clk.enabled = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif
	return 0;
}

static s32 disp_mgr_clk_disable(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("mgr %d clk disable\n", mgr->channel_id);

	disp_al_manager_clk_disable(mgrp->clk.clk);

#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&mgr_data_lock, flags);
#endif
			mgrp->clk.enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return 0;
}

s32 disp_mgr_clk_disable_sw(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("mgr %d clk disable\n", mgr->channel_id);

#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&mgr_data_lock, flags);
#endif
			mgrp->clk.enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return 0;
}

extern void sync_finish_event_proc(u32 screen_id);
#if defined(__LINUX_PLAT__)
static s32 manager_event_proc(s32 irq, void *parg)
#else
static s32 manager_event_proc(void *parg)
#endif
{
	u32 screen_id = (u32)parg;

	if(disp_al_manager_query_irq(screen_id, DE_IMG_REG_LOAD_FINISH)) {
		sync_finish_event_proc(screen_id);
	}

	return OSAL_IRQ_RETURN;
}

static s32 disp_mgr_init(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(!disp_al_query_be_mod(mgr->channel_id)) {
		DE_WRN("manager %d is not register\n", mgr->channel_id);
		return -1;
	}

	if((NULL != mgr->p_sw_init_flag) && (0 != *(mgr->p_sw_init_flag))) {
#if defined(CONFIG_HOMLET_PLATFORM)
		disp_mgr_clk_enable_sw(mgr);
		disp_al_manager_init_sw(mgr->channel_id); //disable irq of imageloadfinish
		disp_mgr_clk_disable_sw(mgr);
#endif
	} else {
		disp_mgr_clk_init(mgr);
		disp_mgr_clk_enable(mgr);
		disp_al_manager_init(mgr->channel_id);
		disp_mgr_clk_disable(mgr);
	}

	OSAL_RegISR(mgrp->irq_no, 0, manager_event_proc, (void *)mgr->channel_id, 0, 0);
#if !defined(__LINUX_PLAT__)
	OSAL_InterruptEnable(mgrp->irq_no);
#endif
	return 0;
}

static s32 disp_mgr_exit(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(!disp_al_query_be_mod(mgr->channel_id)) {
		DE_WRN("manager %d is not register\n", mgr->channel_id);
		return -1;
	}

	disp_al_manager_enable(mgr->channel_id, 0);
	disp_mgr_clk_exit(mgr);
#if defined(__LINUX_PLAT__)
	OSAL_InterruptDisable(mgrp->irq_no);
#endif
	OSAL_UnRegISR(mgrp->irq_no, manager_event_proc,(void*)mgr->channel_id);

	return 0;
}

static s32 disp_mgr_set_back_color(struct disp_manager *mgr, disp_color_info *back_color)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&mgr_data_lock, flags);
#endif
			memcpy(&mgrp->user_info.back_color, back_color, sizeof(disp_color_info));
			mgrp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif
	mgr->apply(mgr);

	return DIS_SUCCESS;
}

static s32 disp_mgr_get_back_color(struct disp_manager *mgr, disp_color_info *back_color)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		memcpy(back_color, &mgrp->user_info.back_color, sizeof(disp_color_info));
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 disp_mgr_set_color_key(struct disp_manager *mgr, disp_colorkey *ck)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		memcpy(&mgrp->user_info.ck, ck, sizeof(disp_colorkey));
		mgrp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif
	mgr->apply(mgr);

	return DIS_SUCCESS;
}

static s32 disp_mgr_get_color_key(struct disp_manager *mgr, disp_colorkey *ck)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		memcpy(ck, &mgrp->user_info.ck, sizeof(disp_colorkey));
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 disp_mgr_set_output_type(struct disp_manager *mgr, disp_output_type output_type)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		mgrp->output = output_type;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 disp_mgr_get_output_type(struct disp_manager *mgr, disp_output_type *output_type)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	*output_type = DISP_OUTPUT_TYPE_NONE;
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		if(mgrp->enabled)
			*output_type = mgrp->output;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 disp_mgr_sync(struct disp_manager *mgr)
{

		disp_al_manager_sync(mgr->channel_id);


	return DIS_SUCCESS;
}

static s32 disp_mgr_update_regs(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
#if defined(__LINUX_PLAT__)
  unsigned long flags;
#endif

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	__inf("disp_mgr_update_regs, mgr%d\n", mgr->channel_id);

#if defined(__LINUX_PLAT__)
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		if(!mgrp->enabled || !mgrp->info_dirty) {
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&mgr_data_lock, flags);
#endif
      return -1;
    }
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&mgr_data_lock, flags);
#endif
	disp_mgr_shadow_protect(mgr, 1);
	disp_al_manager_set_backcolor(mgr->channel_id, &mgrp->info.back_color);
	disp_al_manager_set_color_key(mgr->channel_id, &mgrp->info.ck);
	disp_mgr_shadow_protect(mgr, 0);
#if defined(__LINUX_PLAT__)
  spin_lock_irqsave(&mgr_data_lock, flags);
#endif
	mgrp->info_dirty = false;
	mgrp->shadow_info_dirty = true;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&mgr_data_lock, flags);
#endif

	return DIS_SUCCESS;
}

static s32 disp_mgr_apply(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("mgr %d apply\n", mgr->channel_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		if(mgrp->user_info_dirty) {
			memcpy(&mgrp->info, &mgrp->user_info, sizeof(struct disp_manager_info));
			mgrp->user_info_dirty = false;
			mgrp->info_dirty = true;
		}
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif
	disp_mgr_update_regs(mgr);

	return DIS_SUCCESS;
}

static s32 disp_mgr_force_update_regs(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	struct disp_layer *lyr;

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	__inf("disp_mgr_force_update_regs, mgr%d\n", mgr->channel_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		mgrp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	disp_mgr_apply(mgr);
	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		DE_INF("force_update_regs, mgr%d lyr%d\n", lyr->channel_id, lyr->layer_id);
		if(lyr->force_update_regs)
			lyr->force_update_regs(lyr);
	}
    disp_mgr_sync(mgr);
	return 0;
}

static s32 disp_mgr_clear_regs(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	struct disp_layer *lyr;

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	__inf("disp_mgr_clear_regs, mgr%d\n", mgr->channel_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		memset(&mgrp->info, 0, sizeof(struct disp_manager_info));
		mgrp->info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	disp_mgr_update_regs(mgr);
	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		DE_INF("clear_regs, mgr%d lyr%d\n", lyr->channel_id, lyr->layer_id);
		if(lyr->clear_regs)
			lyr->clear_regs(lyr);
	}
	return 0;
}

static s32 disp_mgr_set_screen_size(struct disp_manager *mgr, u32 width, u32 height)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		mgrp->width = width;
		mgrp->height = height;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 disp_mgr_get_screen_size(struct disp_manager *mgr, u32 *width, u32 *height)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
	disp_al_manager_get_display_size(mgr->channel_id, width, height);
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 disp_mgr_add_layer(struct disp_manager *mgr, struct disp_layer* lyr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp) || (NULL == lyr)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	__inf("disp_mgr_add_layer, mgr%d <--- mgr%d lyr%d\n", mgr->channel_id, lyr->channel_id, lyr->layer_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		list_add_tail(&(lyr->list), &(mgr->lyr_list));
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif

	return DIS_SUCCESS;
}

static s32 disp_mgr_enable(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("mgr %d enable\n", mgr->channel_id);

	if((NULL != mgr->p_sw_init_flag) && (0 != *(mgr->p_sw_init_flag))) {
#if defined(CONFIG_HOMLET_PLATFORM)
		disp_mgr_clk_enable_sw(mgr);
		disp_al_manager_enable_sw(mgr->channel_id, 1);
#endif
	} else {
		disp_mgr_clk_enable(mgr);
		disp_al_manager_enable(mgr->channel_id, 1);
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		mgrp->enabled = 1;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif
	disp_mgr_force_update_regs(mgr);

	return 0;
}

static s32 disp_mgr_disable(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("mgr %d disable\n", mgr->channel_id);

	disp_mgr_clear_regs(mgr);
	disp_al_manager_enable(mgr->channel_id, 0);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&mgr_data_lock, flags);
#endif
		mgrp->enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&mgr_data_lock, flags);
	}
#endif
	bsp_disp_delay_ms(5);

	disp_mgr_clk_disable(mgr);

	return 0;
}

static s32 disp_mgr_is_enabled(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return mgrp->enabled;

}

s32 disp_mgr_shadow_protect(struct disp_manager *mgr, bool protect)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if((NULL == mgr) || (NULL == mgrp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(mgrp->shadow_protect)
		return mgrp->shadow_protect(mgr->channel_id, protect);

	return -1;
}

s32 disp_init_mgr(__disp_bsp_init_para * para)
{
	u32 num_screens;
	u32 screen_id;
	struct disp_manager *mgr;
	struct disp_manager_private_data *mgrp;

	DE_INF("disp_init_mgr\n");

#if defined(__LINUX_PLAT__)
	spin_lock_init(&mgr_data_lock);
#endif
	num_screens = bsp_disp_feat_get_num_screens();
	mgrs = (struct disp_manager *)OSAL_malloc(sizeof(struct disp_manager) * num_screens);
	if(NULL == mgrs) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	mgr_private = (struct disp_manager_private_data *)OSAL_malloc(sizeof(struct disp_manager_private_data) * num_screens);
	if(NULL == mgr_private) {
		DE_WRN("malloc memory fail! size=0x%x x %d\n", sizeof(struct disp_manager_private_data), num_screens);
		return DIS_FAIL;
	}

	for(screen_id=0; screen_id<num_screens; screen_id++) {
		if(!disp_al_query_be_mod(screen_id))
			continue;

		mgr = &mgrs[screen_id];
		mgrp = &mgr_private[screen_id];

		switch(screen_id) {
		case 0:
			mgr->name = "mgr0";
			mgr->channel_id = 0;
			mgr->output_type = DISP_OUTPUT_TYPE_NONE;
			mgrp->irq_no = para->irq_no[DISP_MOD_BE0];
			mgrp->reg_base = para->reg_base[DISP_MOD_BE0];
			mgrp->clk.clk = MOD_CLK_DEBE0;
			mgrp->shadow_protect = para->shadow_protect;

			break;
		case 1:
			mgr->name = "mgr1";
			mgr->channel_id = 1;
			mgr->output_type = DISP_OUTPUT_TYPE_NONE;
			mgrp->irq_no = para->irq_no[DISP_MOD_BE1];
			mgrp->reg_base = para->reg_base[DISP_MOD_BE1];
			mgrp->clk.clk = MOD_CLK_DEBE1;
			mgrp->shadow_protect = para->shadow_protect;

			break;
		case 2:
			mgr->name = "mgr2";
			mgr->channel_id = 2;
			mgr->output_type = DISP_OUTPUT_TYPE_NONE;
			mgrp->irq_no = para->irq_no[DISP_MOD_BE2];
			mgrp->reg_base = para->reg_base[DISP_MOD_BE2];
			mgrp->clk.clk = MOD_CLK_DEBE2;
			mgrp->shadow_protect = para->shadow_protect;

			break;
		}
		mgr->p_sw_init_flag = ((1 << screen_id) & para->sw_init_para->sw_init_flag) ?
			(&(para->sw_init_para->sw_init_flag)) : NULL;

		mgr->enable = disp_mgr_enable;
		mgr->disable = disp_mgr_disable;
		mgr->is_enabled = disp_mgr_is_enabled;
		mgr->set_screen_size = disp_mgr_set_screen_size;
		mgr->get_screen_size = disp_mgr_get_screen_size;
		mgr->set_color_key = disp_mgr_set_color_key;
		mgr->get_color_key = disp_mgr_get_color_key;
		mgr->set_back_color = disp_mgr_set_back_color;
		mgr->get_back_color = disp_mgr_get_back_color;
		mgr->set_output_type = disp_mgr_set_output_type;
		mgr->get_output_type = disp_mgr_get_output_type;
		mgr->add_layer = disp_mgr_add_layer;

		mgr->init = disp_mgr_init;
		mgr->exit = disp_mgr_exit;

		mgr->apply = disp_mgr_apply;
		mgr->update_regs = disp_mgr_update_regs;
		mgr->force_update_regs = disp_mgr_force_update_regs;
		mgr->sync = disp_mgr_sync;

		INIT_LIST_HEAD(&mgr->lyr_list);

		mgr->init(mgr);
	}

	return 0;
}

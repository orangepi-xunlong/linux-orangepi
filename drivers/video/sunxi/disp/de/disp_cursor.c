#include "disp_cursor.h"

/* FIXME the fb will be clear when BE's clock reset
 * so save the fb is needed
 */
struct disp_cursor_info
{
	u32                      palette[CURSOR_MAX_PALETTE_SIZE];
	disp_position            pos;
	disp_cursor_fb           fb;

	u32                      enable;
};

struct disp_cursor_private_data
{
	u32                       reg_base;
	u32                       enabled;

	struct disp_cursor_info   info;
	bool                      info_dirty;
	bool                      info_dirty_p;

	bool                      shadow_info_dirty;

	s32 (*shadow_protect)(u32 sel, bool protect);
};
#if defined(__LINUX_PLAT__)
static spinlock_t cursor_data_lock;
#endif

static struct disp_cursor *cursors = NULL;
static struct disp_cursor_private_data *cursor_private;

struct disp_cursor* disp_get_cursor(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens) {
		DE_WRN("screen_id %d out of range\n", screen_id);
		return NULL;
	}

	if(!disp_al_query_be_mod(screen_id)) {
		//DE_WRN("be %d is not registered\n", screen_id);
		return NULL;
	}

	return &cursors[screen_id];
}

static struct disp_cursor_private_data *disp_cursor_get_priv(struct disp_cursor *cursor)
{
	if(NULL == cursor) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	if(!disp_al_query_be_mod(cursor->channel_id)) {
		DE_WRN("be %d is not registered\n", cursor->channel_id);
		return NULL;
	}

	return &cursor_private[cursor->channel_id];
}

static s32 disp_cursor_sync(struct disp_cursor* cursor)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		cursorp->shadow_info_dirty = false;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif
	return 0;
}

static s32 disp_cursor_update_regs(struct disp_cursor* cursor)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);
	bool info_dirty = false, info_dirty_p = false;

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("disp_cursor_update_regs, screen %d, enable=%d\n", cursor->channel_id, cursorp->info.enable);

  {
#if defined(__LINUX_PLAT__)
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		if((!cursor->manager) || (!cursor->manager->is_enabled) ||
			(!cursor->manager->is_enabled(cursor->manager)) || (!(cursorp->info_dirty || cursorp->info_dirty_p))) {
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&cursor_data_lock, flags);
#endif
		} else {
			info_dirty = cursorp->info_dirty;
			info_dirty_p = cursorp->info_dirty_p;
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&cursor_data_lock, flags);
#endif
		}
	}

	disp_cursor_shadow_protect(cursor, 1);
	if(info_dirty_p)
		disp_al_hwc_set_palette(cursor->channel_id, (void *)cursorp->info.palette, 0, CURSOR_MAX_PALETTE_SIZE);
	if(info_dirty) {
		disp_al_hwc_set_pos(cursor->channel_id, &cursorp->info.pos);
		if(cursorp->info.fb.addr)
			disp_al_hwc_set_framebuffer(cursor->channel_id, &cursorp->info.fb);
		disp_al_hwc_enable(cursor->channel_id, cursorp->info.enable);
	}
	disp_cursor_shadow_protect(cursor, 0);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		cursorp->info_dirty = false;
		cursorp->info_dirty_p = false;
		cursorp->shadow_info_dirty = true;
		cursorp->enabled = cursorp->info.enable;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_cursor_apply(struct disp_cursor* cursor)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("disp_cursor_apply, screen %d\n", cursor->channel_id);
	disp_cursor_update_regs(cursor);
	return 0;
}

static s32 disp_cursor_force_update_regs(struct disp_cursor* cursor)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		cursorp->info_dirty = true;
		cursorp->info_dirty_p = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif
	disp_cursor_apply(cursor);

	return 0;
}

/* seem no meaning */
static bool disp_cursor_is_enabled(struct disp_cursor* cursor)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return false;
	}

	return cursorp->enabled;
}

static s32 disp_cursor_enable(struct disp_cursor* cursor)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		cursorp->info.enable = 1;
		cursorp->info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif
	disp_cursor_apply(cursor);
	return 0;
}

static s32 disp_cursor_disable(struct disp_cursor* cursor)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		cursorp->info.enable = 0;
		cursorp->info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif
	disp_cursor_apply(cursor);
	return 0;
}

static s32 disp_cursor_set_palette(struct disp_cursor* cursor, void *palette, u32 offset, u32 palette_size)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if((offset + palette_size) > CURSOR_MAX_PALETTE_SIZE) {
		DE_WRN("para err, offset(%d) > MAX_SIZE(%d), or palette_size(%d) > MAX_SIZE(%d)\n", offset, CURSOR_MAX_PALETTE_SIZE, palette_size, CURSOR_MAX_PALETTE_SIZE);
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		memcpy((void*)&cursorp->info.palette[offset], palette, palette_size);
		cursorp->info_dirty_p = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif
  disp_cursor_apply(cursor);

	return 0;
}

static s32 disp_cursor_set_fb(struct disp_cursor* cursor, disp_cursor_fb *fb)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		memcpy(&cursorp->info.fb, fb, sizeof(disp_cursor_fb));
		cursorp->info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif
  disp_cursor_apply(cursor);

	return 0;
}

static s32 disp_cursor_set_pos(struct disp_cursor* cursor, disp_position *pos)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		memcpy(&cursorp->info.pos, pos, sizeof(disp_position));
		cursorp->info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif
  disp_cursor_apply(cursor);

	return 0;
}

static s32 disp_cursor_get_pos(struct disp_cursor* cursor, disp_position *pos)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		memcpy(pos, &cursorp->info.pos, sizeof(disp_position));
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_cursor_set_manager(struct disp_cursor* cursor, struct disp_manager *mgr)
{
	if((NULL == cursor) || (NULL == mgr)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&cursor_data_lock, flags);
#endif
		cursor->manager = mgr;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&cursor_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_cursor_notifier_callback(struct disp_notifier_block *self,
		 u32 event, u32 sel, void *data)
{
	struct disp_cursor *cursor;
	u32 *ptr = (u32 *)data;
	u32 enable;
	disp_output_type output_type;

	cursor = disp_get_cursor(sel);
	if(!cursor || !ptr)
		return -1;

	DE_INF("notifier cb: event=0x%x, sel=%d, data=0x%x\n", event, sel, (u32)data);
	switch(event){
	case DISP_EVENT_OUTPUT_ENABLE:
		enable = ptr[0];
		output_type = (disp_output_type)(ptr[1]);

//FIXME
//		if(enable)
//			disp_cursor_force_update_regs(cursor);

		break;

	default:

		break;
	}
	return 0;
}

s32 disp_cursor_shadow_protect(struct disp_cursor *cursor, bool protect)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(cursorp->shadow_protect)
		return cursorp->shadow_protect(cursor->channel_id, protect);

	return -1;
}

static s32 disp_cursor_init(struct disp_cursor *cursor)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);
	struct disp_notifier_block *nb;

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(!disp_al_query_be_mod(cursor->channel_id)) {
		DE_WRN("curosr %d is not register\n", cursor->channel_id);
		return -1;
	}

	/* register one notifier for all cursor */
	if(0 == cursor->channel_id) {
		nb = (struct disp_notifier_block *)OSAL_malloc(sizeof(struct disp_notifier_block));
		if(nb) {
			nb->notifier_call = &disp_cursor_notifier_callback;
			disp_notifier_register(nb);
		} else
			DE_WRN("malloc memory fail!\n");
	}

	return 0;
}

static s32 disp_cursor_exit(struct disp_cursor *cursor)
{
	struct disp_cursor_private_data *cursorp = disp_cursor_get_priv(cursor);

	if((NULL == cursor) || (NULL == cursorp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(!disp_al_query_be_mod(cursor->channel_id)) {
		DE_WRN("curosr %d is not register\n", cursor->channel_id);
		return -1;
	}

	return 0;
}

s32 disp_init_cursor(__disp_bsp_init_para * para)
{
	u32 num_screens;
	u32 screen_id;
	struct disp_cursor *cursor;
	struct disp_cursor_private_data *cursorp;

	DE_INF("disp_init_cursor\n");

#if defined(__LINUX_PLAT__)
	spin_lock_init(&cursor_data_lock);
#endif
	num_screens = bsp_disp_feat_get_num_screens();
	cursors = (struct disp_cursor *)OSAL_malloc(sizeof(struct disp_cursor) * num_screens);
	if(NULL == cursors) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	cursor_private = (struct disp_cursor_private_data *)OSAL_malloc(sizeof(struct disp_cursor_private_data) * num_screens);
	if(NULL == cursor_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	for(screen_id=0; screen_id<num_screens; screen_id++) {
		if(!disp_al_query_be_mod(screen_id))
			continue;

		cursor = &cursors[screen_id];
		cursorp = &cursor_private[screen_id];

		switch(screen_id) {
		case 0:
			cursor->name = "cursor0";
			cursor->channel_id = 0;

			break;
		case 1:
			cursor->name = "cursor1";
			cursor->channel_id = 1;

			break;
		case 2:
			cursor->name = "cursor2";
			cursor->channel_id = 2;

			break;
		}
		cursorp->shadow_protect = para->shadow_protect;

		cursor->enable = disp_cursor_enable;
		cursor->disable = disp_cursor_disable;
		cursor->is_enabled = disp_cursor_is_enabled;
		cursor->init = disp_cursor_init;
		cursor->exit = disp_cursor_exit;
		cursor->apply = disp_cursor_apply;
		cursor->update_regs = disp_cursor_update_regs;
		cursor->force_update_regs = disp_cursor_force_update_regs;
		cursor->sync = disp_cursor_sync;
		cursor->set_manager = disp_cursor_set_manager;
		cursor->set_pos = disp_cursor_set_pos;
		cursor->get_pos = disp_cursor_get_pos;
		cursor->set_fb = disp_cursor_set_fb;
		cursor->set_palette = disp_cursor_set_palette;

		cursor->init(cursor);
	}

	return 0;
}


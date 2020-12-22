#include "disp_smart_backlight.h"

struct disp_smbl_info
{
	u32                      mode;
	disp_window              window;
	u32                      enable;
};
struct disp_smbl_private_data
{
	u32                       irq_no;
	u32                       reg_base;
	bool                      user_info_dirty;
	struct disp_smbl_info     user_info;

	bool                      info_dirty;
	struct disp_smbl_info     info;

	u32                       dimming;

	bool                      shadow_info_dirty;
	s32 (*shadow_protect)(u32 sel, bool protect);

	u32                       enabled;
#if defined(__LINUX_PLAT__)
	struct tasklet_struct     tasklet;
#endif
};
#if defined(__LINUX_PLAT__)
static spinlock_t smbl_data_lock;
#endif

static struct disp_smbl *smbls = NULL;
static struct disp_smbl_private_data *smbl_private;

struct disp_smbl* disp_get_smbl(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens) {
		DE_WRN("screen_id %d out of range\n", screen_id);
		return NULL;
	}

	if(!disp_al_query_drc_mod(screen_id)) {
		//DE_WRN("drc %d is not registered\n", screen_id);
		return NULL;
	}

	return &smbls[screen_id];
}
static struct disp_smbl_private_data *disp_smbl_get_priv(struct disp_smbl *smbl)
{
	if(NULL == smbl) {
		DE_INF("NULL hdl!\n");
		return NULL;
	}

	if(!disp_al_query_drc_mod(smbl->channel_id)) {
		DE_WRN("drc %d is not registered\n", smbl->channel_id);
		return NULL;
	}

	return &smbl_private[smbl->channel_id];
}

static s32 disp_smbl_update_regs(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);
	struct disp_smbl_info smbl_info;

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		if((!smbl->manager) || (!smbl->manager->is_enabled) ||
			(!smbl->manager->is_enabled(smbl->manager)) || (!smblp->info_dirty)) {
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&smbl_data_lock, flags);
#endif
			return DIS_SUCCESS;
		}
		memcpy(&smbl_info, &smblp->info, sizeof(struct disp_smbl_info));
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	/* if this func called by isr, can't call shadow protect
	 *    if called by non_isr, shadow protect is must
	 */
	//disp_smbl_shadow_protect(smbl, 1);
	//DE_INF("smbl %d update_regs ok, enable=%d\n", smbl->channel_id, smbl_info.enable);
	disp_al_smbl_enable(smbl->channel_id, smbl_info.enable);
	disp_al_smbl_set_window(smbl->channel_id, &smbl_info.window);
	//disp_smbl_shadow_protect(smbl, 0);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->info_dirty = false;
		smblp->shadow_info_dirty = true;
		smblp->enabled = smbl_info.enable;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	return 0;
}

static s32 disp_smbl_apply(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("smbl %d apply\n", smbl->channel_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		if(smblp->user_info_dirty) {
			memcpy(&smblp->info, &smblp->user_info, sizeof(struct disp_smbl_info));
			smblp->user_info_dirty = false;
			smblp->info_dirty = true;
		}
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	/* can't call update_regs at apply at single register buffer plat(e.g., a23,a31)
	 *   but it's recommended at double register buffer plat
	 */
	//disp_smbl_update_regs(smbl);
	return 0;
}

static s32 disp_smbl_force_update_regs(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("disp_smbl_force_update_regs, smbl %d\n", smbl->channel_id);
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	disp_smbl_apply(smbl);

	return 0;
}

#if defined(__LINUX_PLAT__)
static void disp_smbl_tasklet(unsigned long data)
{
	struct disp_smbl *smbl = (struct disp_smbl *)data;
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);
	struct disp_lcd *lcd = NULL;

	if((NULL == smbl) || (NULL == smblp))
		return;

	lcd = disp_get_lcd(smbl->channel_id);

	/* update bright dimming */
	if(lcd && lcd->set_bright_dimming)
		lcd->set_bright_dimming(lcd, smblp->dimming);

	/* excute smbl tasklet */
	disp_al_smbl_tasklet(smbl->channel_id);

	return ;
}
#endif
static s32 disp_smbl_sync(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	disp_smbl_update_regs(smbl);
	if(smblp->shadow_info_dirty || smblp->enabled) {
		u32 backlight_dimming;

		disp_al_smbl_sync(smbl->channel_id);
		backlight_dimming = disp_al_smbl_get_backlight_dimming(smbl->channel_id);
		smblp->dimming = backlight_dimming;
#if defined(__LINUX_PLAT__)
			tasklet_schedule(&smblp->tasklet);
#endif
		//disp_notifier_call_chain(DISP_EVENT_BACKLIGHT_DIMMING_UPDATE, smbl->channel_id, (void*)backlight_dimming);
	}

	if(smblp->shadow_info_dirty && (smblp->enabled == 0)) {
		disp_notifier_call_chain(DISP_EVENT_BACKLIGHT_DIMMING_UPDATE, smbl->channel_id, (void*)256);
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->shadow_info_dirty = false;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	return 0;
}

static bool disp_smbl_is_enabled(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return false;
	}

	return (smblp->user_info.enable == 1);
}

static s32 disp_smbl_enable(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("smbl %d enable\n", smbl->channel_id);
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->user_info.enable = 1;
		smblp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	disp_smbl_apply(smbl);

	return 0;
}

static s32 disp_smbl_disable(struct disp_smbl* smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("smbl %d disable\n", smbl->channel_id);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smblp->user_info.enable = 0;
		smblp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
	disp_smbl_apply(smbl);
	return 0;
}

s32 disp_smbl_shadow_protect(struct disp_smbl *smbl, bool protect)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	if(smblp->shadow_protect)
		return smblp->shadow_protect(smbl->channel_id, protect);

	return -1;
}

static s32 disp_smbl_set_window(struct disp_smbl* smbl, disp_window *window)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		memcpy(&smblp->user_info.window, window, sizeof(disp_window));
		smblp->user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif
  disp_smbl_apply(smbl);

	return 0;
}

static s32 disp_smbl_set_manager(struct disp_smbl* smbl, struct disp_manager *mgr)
{
	if((NULL == smbl) || (NULL == mgr)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smbl_data_lock, flags);
#endif
		smbl->manager = mgr;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smbl_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_smbl_notifier_callback(struct disp_notifier_block *self,
		 u32 event, u32 sel, void *data)
{
	struct disp_smbl *smbl;
	u32 *ptr = (u32 *)data;
	u32 enable;
	disp_output_type output_type;
	u32 backlight;

	smbl = disp_get_smbl(sel);
	if(!smbl || !ptr)
		return -1;

	DE_INF("notifier cb: event=0x%x, sel=%d, data=0x%x\n", event, sel, (u32)data);
	switch(event){
	case DISP_EVENT_OUTPUT_ENABLE:
		enable = ptr[0];
		output_type = (disp_output_type)(ptr[1]);

		if(enable && (DISP_OUTPUT_TYPE_LCD == output_type)) {
			disp_smbl_force_update_regs(smbl);
		} else if(!enable) {
		}

		break;
	case DISP_EVENT_BACKLIGHT_UPDATE:
		backlight = (u32)ptr;
		disp_al_smbl_update_backlight(sel, backlight);

		break;
	default:

		break;
	}
	return 0;
}

static s32 disp_smbl_init(struct disp_smbl *smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);
	struct disp_notifier_block *nb;

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

#if defined(__LINUX_PLAT__)
	tasklet_init(&smblp->tasklet, disp_smbl_tasklet, (unsigned long)smbl);
#endif
	IEP_Drc_Init(smbl->channel_id);

	/* register one notifier for all smbl */
	if(0 == smbl->channel_id) {
		nb = (struct disp_notifier_block *)OSAL_malloc(sizeof(struct disp_notifier_block));
		if(nb) {
			nb->notifier_call = &disp_smbl_notifier_callback;
			disp_notifier_register(nb);
		} else
			DE_WRN("malloc memory fail!\n");
	}

	return 0;
}

static s32 disp_smbl_exit(struct disp_smbl *smbl)
{
	struct disp_smbl_private_data *smblp = disp_smbl_get_priv(smbl);

	if((NULL == smbl) || (NULL == smblp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	return 0;
}

s32 disp_init_smbl(__disp_bsp_init_para * para)
{
	u32 num_smbls;
	u32 screen_id;
	struct disp_smbl *smbl;
	struct disp_smbl_private_data *smblp;

	DE_INF("disp_init_smbl\n");

#if defined(__LINUX_PLAT__)
	spin_lock_init(&smbl_data_lock);
#endif
	num_smbls = bsp_disp_feat_get_num_smart_backlights();
	smbls = (struct disp_smbl *)OSAL_malloc(sizeof(struct disp_smbl) * num_smbls);
	if(NULL == smbls) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	smbl_private = (struct disp_smbl_private_data *)OSAL_malloc(sizeof(struct disp_smbl_private_data) * num_smbls);
	if(NULL == smbl_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	for(screen_id=0; screen_id<num_smbls; screen_id++) {
		if(!disp_al_query_drc_mod(screen_id))
			continue;

		smbl = &smbls[screen_id];
		smblp = &smbl_private[screen_id];

		switch(screen_id) {
		case 0:
			smbl->name = "smbl0";
			smbl->channel_id = 0;
			smblp->irq_no = para->irq_no[DISP_MOD_DRC0];
			smblp->reg_base = para->reg_base[DISP_MOD_DRC0];

			break;
		case 1:
			smbl->name = "smbl1";
			smbl->channel_id = 1;
			smblp->irq_no = para->irq_no[DISP_MOD_DRC1];
			smblp->reg_base = para->reg_base[DISP_MOD_DRC1];

			break;
		case 2:
			smbl->name = "smbl2";
			smbl->channel_id = 2;
			smblp->irq_no = para->irq_no[DISP_MOD_DRC2];
			smblp->reg_base = para->reg_base[DISP_MOD_DRC2];

			break;
		}
		smblp->shadow_protect = para->shadow_protect;

		smbl->enable = disp_smbl_enable;
		smbl->disable = disp_smbl_disable;
		smbl->is_enabled = disp_smbl_is_enabled;
		smbl->init = disp_smbl_init;
		smbl->exit = disp_smbl_exit;
		smbl->apply = disp_smbl_apply;
		smbl->update_regs = disp_smbl_update_regs;
		smbl->force_update_regs = disp_smbl_force_update_regs;
		smbl->sync = disp_smbl_sync;
		smbl->set_manager = disp_smbl_set_manager;
		smbl->set_window = disp_smbl_set_window;

		smbl->init(smbl);
	}

	return 0;
}


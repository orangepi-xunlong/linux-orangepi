#include "disp_smart_color.h"

struct disp_smcl_info
{
	u32                      bright;
	u32                      saturation;
	u32                      contrast;
	u32                      hue;
	u32                      mode;
	disp_window              window;
	u32                      enable;
	disp_size                size;
};

struct disp_smcl_private_data
{
	u32                       reg_base;
	u32                       enabled;

	struct {
		bool                      user_info_dirty;
		struct disp_smcl_info     user_info;

		bool                      info_dirty;
		struct disp_smcl_info     info;
	}screen_info;

	bool                      shadow_info_dirty;
	s32 (*shadow_protect)(u32 sel, bool protect);

	disp_clk_info_t  clk;
	u32                       clk_close_request;
};
#if defined(__LINUX_PLAT__)
static spinlock_t smcl_data_lock;
#endif

static struct disp_smcl *smcls = NULL;
static struct disp_smcl_private_data *smcl_private;

struct disp_smcl* disp_get_smcl(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens) {
		DE_WRN("screen_id %d out of range\n", screen_id);
		return NULL;
	}

	if(!disp_al_query_smart_color_mod(screen_id)) {
		//DE_WRN("cmu %d is not registered\n", screen_id);
		return NULL;
	}

	return &smcls[screen_id];
}
static struct disp_smcl_private_data *disp_smcl_get_priv(struct disp_smcl *smcl)
{
	if(NULL == smcl) {
		DE_INF("NULL hdl!\n");
		return NULL;
	}

	if(!disp_al_query_smart_color_mod(smcl->channel_id)) {
		DE_WRN("cmu %d is not registered\n", smcl->channel_id);
		return NULL;
	}

	return &smcl_private[smcl->channel_id];
}

//smcl clk
static s32 disp_smcl_clk_init(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);
	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	return 0;
}

static s32 disp_smcl_clk_exit(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);
	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&smcl_data_lock, flags);
#endif
			smclp->clk.enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_smcl_clk_enable(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);
	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	if(smclp->clk.enabled == 0) {
#if defined(__LINUX_PLAT__)
		{
			unsigned long flags;
			spin_lock_irqsave(&smcl_data_lock, flags);
#endif
				smclp->clk.enabled = 1;
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&smcl_data_lock, flags);
		}
#endif
	}

	return 0;
}

static s32 disp_smcl_clk_disable(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);
	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	if(smclp->clk.enabled) {
#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&smcl_data_lock, flags);
#endif
			smclp->clk.enabled = 0;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
	}

	return 0;
}

static s32 disp_smcl_sync(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	//FIXME: display size may be set atfer sync(widthout shadow_info_dirty)
	if(smclp->shadow_info_dirty || smclp->enabled)
	{
		disp_al_smcl_sync(smcl->channel_id);
	}

	if((smclp->enabled == 0) && (smclp->clk_close_request == 1) && smclp->shadow_info_dirty) {
		disp_smcl_clk_disable(smcl);
		smclp->clk_close_request = 0;
	}

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smclp->shadow_info_dirty = false;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_smcl_update_regs(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);
	struct disp_smcl_info smcl_info;

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("disp_smcl_update_regs, screen %d\n", smcl->channel_id);
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		if((!smcl->manager) || (!smcl->manager->is_enabled) ||
			(!smcl->manager->is_enabled(smcl->manager)) || (!smclp->screen_info.info_dirty)) {
#if defined(__LINUX_PLAT__)
			spin_unlock_irqrestore(&smcl_data_lock, flags);
#endif
			return DIS_SUCCESS;
		}
		memcpy(&smcl_info, &smclp->screen_info.info, sizeof(struct disp_smcl_info));
		DE_INF("smcl_update_regs ok, enable=%d, <%d,%d,%d>,mode=%d, win[%d,%d,%d,%d]\n", smcl_info.enable, smcl_info.bright, smcl_info.saturation, smcl_info.hue,
			smcl_info.mode, smcl_info.window.x, smcl_info.window.y, smcl_info.window.width, smcl_info.window.height);
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
	disp_smcl_shadow_protect(smcl, 1);
	disp_al_smcl_set_para(smcl->channel_id, smcl_info.bright, smcl_info.saturation, smcl_info.hue, smcl_info.mode);
	disp_al_smcl_set_window(smcl->channel_id, &smcl_info.window);
	disp_al_smcl_enable(smcl->channel_id, smcl_info.enable);
	disp_smcl_shadow_protect(smcl, 0);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smclp->screen_info.info_dirty = false;
		smclp->shadow_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_smcl_apply(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("disp_smcl_apply, screen %d\n", smcl->channel_id);
#if defined(__LINUX_PLAT__)
	{
	unsigned long flags;
	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		if(smclp->screen_info.user_info_dirty) {
			memcpy(&smclp->screen_info.info, &smclp->screen_info.user_info, sizeof(struct disp_smcl_info));
			smclp->screen_info.user_info_dirty = false;
			smclp->screen_info.info_dirty = true;
		}
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
	disp_smcl_update_regs(smcl);
	return 0;
}

static s32 disp_smcl_force_update_regs(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smclp->screen_info.user_info_dirty = true;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
	disp_smcl_apply(smcl);

	return 0;
}

/* seem no meaning */
static bool disp_smcl_is_enabled(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return false;
	}

	return smclp->enabled;
}

static s32 disp_smcl_enable(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	disp_smcl_clk_enable(smcl);

#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smclp->screen_info.user_info.enable = 1;
		smclp->enabled = 1;
		smclp->screen_info.user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif

	disp_smcl_apply(smcl);
	return 0;
}

static s32 disp_smcl_disable(struct disp_smcl* smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smclp->enabled = 0;
		smclp->clk_close_request = 1;
		smclp->screen_info.user_info.enable = 0;
		smclp->screen_info.user_info_dirty = true;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif

	disp_smcl_apply(smcl);

	return 0;
}

static s32 disp_smcl_set_bright(struct disp_smcl* smcl, u32 val)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("disp_smcl_set_bright, screen %d, bright=%d\n", smcl->channel_id, val);
#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smclp->screen_info.user_info.bright = val;
		smclp->screen_info.user_info_dirty = true;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
	disp_smcl_apply(smcl);

	return 0;
}

static s32 disp_smcl_set_saturation(struct disp_smcl* smcl, u32 val)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smclp->screen_info.user_info.saturation = val;
		smclp->screen_info.user_info_dirty = true;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
	disp_smcl_apply(smcl);

	return 0;
}

static s32 disp_smcl_set_hue(struct disp_smcl* smcl, u32 val)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smclp->screen_info.user_info.hue = val;
		smclp->screen_info.user_info_dirty = true;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
	disp_smcl_apply(smcl);

	return 0;
}

static s32 disp_smcl_set_mode(struct disp_smcl* smcl, u32 val)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smclp->screen_info.user_info.mode = val;
		smclp->screen_info.user_info_dirty = true;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
	disp_smcl_apply(smcl);

	return 0;
}

static s32 disp_smcl_set_window(struct disp_smcl* smcl, disp_window *window)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		memcpy(&smclp->screen_info.user_info.window, window, sizeof(disp_window));
		smclp->screen_info.user_info_dirty = true;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
  disp_smcl_apply(smcl);

	return 0;
}

s32 disp_smcl_set_size(struct disp_smcl* smcl, disp_size *size)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
	unsigned long flags;
	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		memcpy(&smclp->screen_info.user_info.size, size, sizeof(disp_size));
		smclp->screen_info.user_info_dirty = true;
#if defined(__LINUX_PLAT__)
	spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif
	disp_smcl_apply(smcl);

	return 0;
}

static s32 disp_smcl_set_manager(struct disp_smcl* smcl, struct disp_manager *mgr)
{
	if((NULL == smcl) || (NULL == mgr)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
  {
  	unsigned long flags;
  	spin_lock_irqsave(&smcl_data_lock, flags);
#endif
		smcl->manager = mgr;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&smcl_data_lock, flags);
	}
#endif

	return 0;
}

s32 disp_smcl_shadow_protect(struct disp_smcl *smcl, bool protect)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	if(smclp->shadow_protect)
		return smclp->shadow_protect(smcl->channel_id, protect);

	return -1;
}

static s32 disp_smcl_notifier_callback(struct disp_notifier_block *self,
		 u32 event, u32 sel, void *data)
{
	struct disp_smcl *smcl;
	struct disp_smcl_private_data *smclp;
	u32 *ptr = (u32 *)data;
	u32 enable;
	disp_output_type output_type;

	smcl = disp_get_smcl(sel);
	smclp = disp_smcl_get_priv(smcl);
	if(!smcl || !ptr)
		return -1;

	DE_INF("notifier cb: event=0x%x, sel=%d, data=0x%x\n", event, sel, (u32)data);
	switch(event){
	case DISP_EVENT_OUTPUT_ENABLE:
		enable = ptr[0];
		output_type = (disp_output_type)(ptr[1]);

		if(enable) {
			if(smclp->enabled)
				disp_smcl_clk_enable(smcl);

				disp_smcl_force_update_regs(smcl);
		} else if(!enable) {
			disp_smcl_clk_disable(smcl);
		}

		break;
	default:

		break;
	}
	return 0;
}

static s32 disp_smcl_init(struct disp_smcl *smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);
	struct disp_notifier_block *nb;

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	disp_smcl_clk_init(smcl);

	/* register one notifier for all smcl */
	if(0 == smcl->channel_id) {
		nb = (struct disp_notifier_block *)OSAL_malloc(sizeof(struct disp_notifier_block));
		if(nb) {
			nb->notifier_call = &disp_smcl_notifier_callback;
			disp_notifier_register(nb);
		} else
			DE_WRN("malloc memory fail!\n");
	}

	return 0;
}

static s32 disp_smcl_exit(struct disp_smcl *smcl)
{
	struct disp_smcl_private_data *smclp = disp_smcl_get_priv(smcl);

	if((NULL == smcl) || (NULL == smclp)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	disp_smcl_clk_exit(smcl);

	return 0;
}

s32 disp_init_smcl(__disp_bsp_init_para * para)
{
	u32 num_smcls;
	u32 screen_id;
	struct disp_smcl *smcl;
	struct disp_smcl_private_data *smclp;

	DE_INF("disp_init_smcl\n");

#if defined(__LINUX_PLAT__)
	spin_lock_init(&smcl_data_lock);
#endif
	num_smcls = bsp_disp_feat_get_num_smart_backlights();
	smcls = (struct disp_smcl *)OSAL_malloc(sizeof(struct disp_smcl) * num_smcls);
	if(NULL == smcls) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	smcl_private = (struct disp_smcl_private_data *)OSAL_malloc(sizeof(struct disp_smcl_private_data) * num_smcls);
	if(NULL == smcl_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	for(screen_id=0; screen_id<num_smcls; screen_id++) {
		if(!disp_al_query_smart_color_mod(screen_id))
			continue;

		smcl = &smcls[screen_id];
		smclp = &smcl_private[screen_id];

		switch(screen_id) {
		case 0:
			smcl->name = "smcl0";
			smcl->channel_id = 0;
			smclp->reg_base = para->reg_base[DISP_MOD_CMU0];
			break;
		case 1:
			smcl->name = "smcl1";
			smcl->channel_id = 1;
			smclp->reg_base = para->reg_base[DISP_MOD_CMU1];
			break;
		case 2:
			smcl->name = "smcl2";
			smcl->channel_id = 2;
			smclp->reg_base = para->reg_base[DISP_MOD_CMU2];

			break;
		}
		smclp->shadow_protect = para->shadow_protect;
#if defined(CONFIG_HOMLET_PLATFORM)
		smclp->screen_info.user_info.mode = DISP_ENHANCE_MODE_STANDARD;
		smclp->screen_info.user_info.bright = 50;
		smclp->screen_info.user_info.saturation = 50;
		smclp->screen_info.user_info.hue = 50;
		smclp->screen_info.user_info.enable = 1;
#else
		smclp->screen_info.user_info.mode = DISP_ENHANCE_MODE_VIVID;
#endif

		smcl->enable = disp_smcl_enable;
		smcl->disable = disp_smcl_disable;
		smcl->is_enabled = disp_smcl_is_enabled;
		smcl->init = disp_smcl_init;
		smcl->exit = disp_smcl_exit;
		smcl->apply = disp_smcl_apply;
		smcl->update_regs = disp_smcl_update_regs;
		smcl->force_update_regs = disp_smcl_force_update_regs;
		smcl->sync = disp_smcl_sync;
		smcl->set_manager = disp_smcl_set_manager;
		smcl->set_bright = disp_smcl_set_bright;
		smcl->set_saturation = disp_smcl_set_saturation;
		smcl->set_hue = disp_smcl_set_hue;
		//smcl->set_contrast = disp_smcl_set_contrast;
		smcl->set_mode = disp_smcl_set_mode;
		smcl->set_window = disp_smcl_set_window;

		smcl->init(smcl);
	}

	return 0;
}

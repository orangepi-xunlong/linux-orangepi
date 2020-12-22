#include "disp_capture.h"
struct disp_capture_private_data
{
	s32 (*shadow_protect)(u32 sel, bool protect);
};

#if defined(__LINUX_PLAT__)
static spinlock_t capture_data_lock;
#endif

static struct disp_capture *captures = NULL;
static struct disp_capture_private_data *capture_private;

struct disp_capture* disp_get_capture(u32 screen_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if(screen_id >= num_screens) {
		DE_WRN("screen_id %d out of range\n", screen_id);
		return NULL;
	}

	if((!disp_al_query_be_mod(screen_id)) || (!bsp_disp_feat_get_capture_support(screen_id))) {
		//DE_WRN("be %d mod is not registered or not support capture\n", screen_id);
		return NULL;
	}

	return &captures[screen_id];
}

static struct disp_capture_private_data *disp_capture_get_priv(struct disp_capture *capture)
{
	if(NULL == capture) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	if((!disp_al_query_be_mod(capture->channel_id)) || (!bsp_disp_feat_get_capture_support(capture->channel_id))) {
		DE_WRN("be %d mod is not registered or not support capture\n", capture->channel_id);
		return NULL;
	}

	return &capture_private[capture->channel_id];
}

static s32 disp_capture_shadow_protect(struct disp_capture *capture, bool protect)
{
	struct disp_capture_private_data *capturep = disp_capture_get_priv(capture);

	if((NULL == capture) || (NULL == capturep)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if(capturep->shadow_protect)
		return capturep->shadow_protect(capture->channel_id, protect);

	return -1;
}

static s32 disp_capture_screen(struct disp_capture *capture, disp_capture_para *para)
{
	struct disp_capture_private_data *capturep = disp_capture_get_priv(capture);

	if(capture == NULL || capturep == NULL) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	disp_capture_shadow_protect(capture, 1);
	if(bsp_disp_get_output_type(capture->channel_id) == DISP_OUTPUT_TYPE_NONE) {
		capture->manager->enable(capture->manager);
	}
	disp_al_capture_screen(capture->channel_id, para);
	disp_capture_shadow_protect(capture, 0);
	return 0;
}

static s32 disp_capture_screen_stop(struct disp_capture *capture)
{
	struct disp_capture_private_data *capturep = disp_capture_get_priv(capture);

	if(capture == NULL || capturep == NULL) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	disp_capture_shadow_protect(capture, 1);
	disp_al_capture_screen_stop(capture->channel_id);

	if(bsp_disp_get_output_type(capture->channel_id) == DISP_OUTPUT_TYPE_NONE) {
		capture->manager->disable(capture->manager);
	}
	disp_capture_shadow_protect(capture, 0);
	return 0;
}

static s32 disp_capture_screen_get_buffer_id(struct disp_capture *capture)
{
	return disp_al_capture_screen_get_buffer_id(capture->channel_id);
}

static s32 disp_capture_sync(struct disp_capture *capture)
{
	return disp_al_capture_sync(capture->channel_id);
}

static s32 disp_capture_screen_finished(struct disp_capture *capture)
{
	return disp_al_caputure_screen_finished(capture->channel_id);
}

static s32 disp_set_manager(struct disp_capture* cptr, struct disp_manager *mgr)
{
	if((NULL == cptr) || (NULL == mgr)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
#if defined(__LINUX_PLAT__)
	{
		unsigned long flags;
		spin_lock_irqsave(&capture_data_lock, flags);
#endif
		cptr->manager = mgr;
#if defined(__LINUX_PLAT__)
		spin_unlock_irqrestore(&capture_data_lock, flags);
	}
#endif

	return 0;
}

static s32 disp_capture_init(struct disp_capture *capture)
{
	struct disp_capture_private_data* capturep = disp_capture_get_priv(capture);

	if((NULL == capture) || (NULL == capturep)) {
		DE_WRN("capture NULL hdl!\n");
		return -1;
	}

	if((!disp_al_query_be_mod(capture->channel_id)) || (!bsp_disp_feat_get_capture_support(capture->channel_id))) {
		DE_WRN("be %d is not register or capture %d is not support\n", capture->channel_id, capture->channel_id);
		return -1;
	}

	return disp_al_capture_init(capture->channel_id);
}

static s32 disp_capture_exit(struct disp_capture *capture)
{
	if((!disp_al_query_be_mod(capture->channel_id)) || (!bsp_disp_feat_get_capture_support(capture->channel_id))) {
		DE_WRN("be %d is not register or capture %d is not support\n", capture->channel_id, capture->channel_id);
		return -1;
	}

	return 0;
}

s32 disp_init_capture(__disp_bsp_init_para *para)
{
	u32 num_screens;
	u32 screen_id;
	struct disp_capture *capture;
	struct disp_capture_private_data *capturep;

#if defined(__LINUX_PLAT__)
	spin_lock_init(&capture_data_lock);
#endif

	num_screens = bsp_disp_feat_get_num_screens();
	captures = (struct disp_capture *)OSAL_malloc(sizeof(struct disp_capture) * num_screens);
	if(NULL == captures) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	capture_private = (struct disp_capture_private_data *)OSAL_malloc(sizeof(struct disp_capture_private_data) * num_screens);
	if(NULL == capture_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	for(screen_id=0; screen_id < num_screens; screen_id++) {
		if((!disp_al_query_be_mod(screen_id)) || (!bsp_disp_feat_get_capture_support(screen_id)))
			continue;

		capture = &captures[screen_id];
		capturep = &capture_private[screen_id];

		switch(screen_id) {
		case 0:
			capture->channel_id = 0;
			capture->name = "capture0";
			break;

		case 1:
			capture->channel_id = 1;
			capture->name = "capture1";
			break;

		case 2:
			capture->channel_id = 2;
			capture->name = "capture2";
			break;
		}

		capturep->shadow_protect = para->shadow_protect;

		capture->set_manager = disp_set_manager;

		capture->capture_screen = disp_capture_screen;
		capture->capture_screen_stop = disp_capture_screen_stop;
		capture->capture_screen_get_buffer_id = disp_capture_screen_get_buffer_id;
		capture->sync = disp_capture_sync;
		capture->capture_screen_finished = disp_capture_screen_finished;
		capture->init = disp_capture_init;
		capture->exit = disp_capture_exit;

		disp_capture_init(capture);
	}
	return 0;
}

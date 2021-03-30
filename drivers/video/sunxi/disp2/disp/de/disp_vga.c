#include "disp_vga.h"

#if defined(SUPPORT_VGA)

static struct disp_device *vgas = NULL;
static struct disp_vga_private_data *vga_private = NULL;

static s32 disp_vga_set_hpd(struct disp_device*  vga, u32 state);
static struct disp_vga_private_data *disp_vga_get_priv(struct disp_device *vga)
{
	if (NULL == vga) {
		DE_WRN("device hdl is NULL!\n");
		return NULL;
	}

	return (struct disp_vga_private_data *)vga->priv_data;
}

extern void sync_event_proc(u32 disp, bool timeout);

#if defined(__LINUX_PLAT__)
static s32 disp_vga_event_proc(int irq, void *parg)
#else
static s32 disp_vga_event_proc(void *parg)
#endif
{
	struct disp_device *vga = (struct disp_device *)parg;
	struct disp_manager *mgr = NULL;
	u32 hwdev_index;

	if (vga==NULL) {
		DE_WRN("device hdl is NULL!\n");
		return DISP_IRQ_RETURN;
	}

	hwdev_index = vga->hwdev_index;
	if (disp_al_device_query_irq(hwdev_index)) {
		int cur_line = disp_al_device_get_cur_line(hwdev_index);
		int start_delay = disp_al_device_get_start_delay(hwdev_index);

		mgr = vga->manager;
		if (NULL == mgr)
			return DISP_IRQ_RETURN;

		if (cur_line <= (start_delay-4)) {
			sync_event_proc(mgr->disp, false);
		} else {
			sync_event_proc(mgr->disp, true);
		}
	}

	return DISP_IRQ_RETURN;
}

static s32 vga_clk_init(struct disp_device*  vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if (!vga || !vgap) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	vgap->clk_parent = clk_get_parent(vgap->clk);

	return 0;
}

static s32 vga_clk_exit(struct disp_device*  vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if (!vga || !vgap) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	if (vgap->clk_parent)
		clk_put(vgap->clk_parent);

	return 0;
}

static s32 vga_clk_config(struct disp_device*  vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if (!vga || !vgap) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	return clk_set_rate(vgap->clk, vga->timings.pixel_clk);
}

static s32 vga_clk_enable(struct disp_device*  vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if (!vga || !vgap) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	return clk_prepare_enable(vgap->clk);
}

static s32 vga_clk_disable(struct disp_device*  vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if (!vga || !vgap) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	clk_disable(vgap->clk);

	return 0;
}

static s32 vga_calc_judge_line(struct disp_device *vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);
	int start_delay, usec_start_delay;
	int usec_judge_point;

	if (!vga || !vgap) {
		DE_WRN("VGA init null hdl!\n");
		return DIS_FAIL;
	}

	/*
	 * usec_per_line = 1 / fps / vt * 1000000
	 *               = 1 / (pixel_clk / vt / ht) / vt * 1000000
	 *               = ht / pixel_clk * 1000000
	 */
	vgap->frame_per_sec = vgap->video_info->pixel_clk
	    / vgap->video_info->hor_total_time
	    / vgap->video_info->ver_total_time
	    * (vgap->video_info->b_interlace + 1)
	    / (vgap->video_info->trd_mode + 1);
	vgap->usec_per_line = vgap->video_info->hor_total_time
	    * 1000000 / vgap->video_info->pixel_clk;

	start_delay =
	    disp_al_device_get_start_delay(vga->hwdev_index);
	usec_start_delay = start_delay * vgap->usec_per_line;

	if (usec_start_delay <= 200)
		usec_judge_point = usec_start_delay * 3 / 7;
	else if (usec_start_delay <= 400)
		usec_judge_point = usec_start_delay / 2;
	else
		usec_judge_point = 200;
	if (vgap->usec_per_line)
		vgap->judge_line = usec_judge_point / vgap->usec_per_line;
	else
		DE_WRN("usec_per_line is Null,someting is wrong!\n");

	return 0;
}

static s32 disp_vga_enable( struct disp_device* vga)
{
	int ret;
	struct disp_manager *mgr = NULL;
	unsigned long flags;
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	mgr = vga->manager;
	if (!mgr) {
		DE_WRN("device%d's mgr is NULL!\n", vga->disp);
		return DIS_FAIL;
	}

	if (vgap->enabled) {
		DE_WRN("device%d is already enabled!\n", vga->disp);
		return DIS_FAIL;
	}
	if (vgap->tv_func.tv_get_video_timing_info == NULL) {
		DE_WRN("get_video_timing_info func is null\n");
		return DIS_FAIL;
	}

	vgap->tv_func.tv_get_video_timing_info(vga->disp, &(vgap->video_info));

	if (vgap->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}
	memcpy(&vga->timings, vgap->video_info, sizeof(struct disp_video_timings));
	vga_calc_judge_line(vga);
	mutex_lock(&vgap->mlock);
	if (vgap->enabled)
		goto exit;
	if (mgr->enable)
		mgr->enable(mgr);

	if (NULL == vgap->tv_func.tv_enable) {
		DE_WRN("enable is NULL\n");
		goto exit;
	}
	vga_clk_config(vga);
	ret = vga_clk_enable(vga);
	if (0 != ret) {
		DE_WRN("fail to enable clock\n");
		goto exit;
	}

	vgap->tv_func.tv_enable(vga->disp);
	disp_al_vga_cfg(vga->hwdev_index, vgap->video_info);
	disp_al_vga_enable(vga->hwdev_index);

	ret = disp_sys_register_irq(vgap->irq_no, 0,
					disp_vga_event_proc,
					(void *)vga, 0, 0);
	if (ret != 0)
		DE_WRN("request irq failed!\n");

	disp_sys_enable_irq(vgap->irq_no);
	spin_lock_irqsave(&vgap->data_lock, flags);
	vgap->enabled = true;
	spin_unlock_irqrestore(&vgap->data_lock, flags);

exit:
	mutex_unlock(&vgap->mlock);

	return 0;
}

static s32 disp_vga_sw_enable( struct disp_device* vga)
{
	struct disp_manager *mgr = NULL;
	unsigned long flags;
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if (!vga || !vgap) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	mgr = vga->manager;
	if (!mgr) {
		DE_WRN("device%d's mgr is NULL\n", vga->disp);
		return DIS_FAIL;
	}

	if (vgap->enabled) {
		DE_WRN("device%d is already enabled\n", vga->disp);
		return DIS_FAIL;
	}
	if (vgap->tv_func.tv_get_video_timing_info == NULL) {
		DE_WRN("get_video_timing_info func is null\n");
		return DIS_FAIL;
	}

	vgap->tv_func.tv_get_video_timing_info(vga->disp, &(vgap->video_info));

	if (vgap->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}
	memcpy(&vga->timings, vgap->video_info, sizeof(struct disp_video_timings));
	vga_calc_judge_line(vga);
	mutex_lock(&vgap->mlock);
	if (mgr->sw_enable)
		mgr->sw_enable(mgr);
	if (NULL == vgap->tv_func.tv_enable) {
		DE_WRN("enable func is NULL\n");
		goto exit;
	}

	disp_sys_register_irq(vgap->irq_no,0,disp_vga_event_proc,(void*)vga,0,0);
	disp_sys_enable_irq(vgap->irq_no);

#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
	if (0 != vga_clk_enable(vga))
		goto exit;
#endif

	spin_lock_irqsave(&vgap->data_lock, flags);
	vgap->enabled = true;
	spin_unlock_irqrestore(&vgap->data_lock, flags);

exit:
	mutex_unlock(&vgap->mlock);

	return 0;
}

s32 disp_vga_disable(struct disp_device* vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);
	unsigned long flags;
	struct disp_manager *mgr = NULL;

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	mgr = vga->manager;
	if (!mgr) {
		DE_WRN("device%d's mgr is NULL\n", vga->disp);
		return DIS_FAIL;
	}

	if (!vgap->enabled) {
		DE_WRN("device%d is already disabled\n", vga->disp);
		return DIS_FAIL;
	}
	if (vgap->tv_func.tv_disable== NULL) {
		DE_WRN("disable func is NULL\n");
		return -1;
	}

	mutex_lock(&vgap->mlock);

	spin_lock_irqsave(&vgap->data_lock, flags);
	vgap->enabled = false;
	spin_unlock_irqrestore(&vgap->data_lock, flags);

	disp_vga_set_hpd(vga, 0);
	vgap->tv_func.tv_disable(vga->disp);
	disp_al_vga_disable(vga->hwdev_index);
	if (mgr->disable)
		mgr->disable(mgr);
	vga_clk_disable(vga);
	vgap->video_info = NULL;

	disp_sys_disable_irq(vgap->irq_no);
	disp_sys_unregister_irq(vgap->irq_no, disp_vga_event_proc,(void*)vga);

	mutex_unlock(&vgap->mlock);

	return 0;
}

static s32 disp_vga_init(struct disp_device*  vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if (!vga || !vgap) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	vga_clk_init(vga);

	return 0;
}

static s32 disp_vga_exit(struct disp_device* vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if (!vga || !vgap) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}
	disp_vga_disable(vga);

	vga_clk_exit(vga);

	kfree(vga);
	kfree(vgap);
	vga = NULL;
	vgap = NULL;
	return 0;
}

static s32 disp_vga_is_enabled(struct disp_device* vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	return vgap->enabled?1:0;
}

static s32 disp_vga_suspend(struct disp_device* vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	mutex_lock(&vgap->mlock);
	if (!vgap->suspended) {
		vgap->suspended = true;
		if (vgap->tv_func.tv_suspend != NULL) {
			vgap->tv_func.tv_suspend(vga->disp);
		}
	}
	mutex_unlock(&vgap->mlock);

	return 0;
}

static s32 disp_vga_resume(struct disp_device* vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	mutex_lock(&vgap->mlock);
	if (vgap->suspended) {
		if (vgap->tv_func.tv_resume != NULL) {
			vgap->tv_func.tv_resume(vga->disp);
		}
		vgap->suspended = false;
	}
	mutex_unlock(&vgap->mlock);

	return 0;
}

static s32 disp_vga_set_mode(struct disp_device* vga, u32 vga_mode)
{
	s32 ret = 0;
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	if (vgap->tv_func.tv_set_mode == NULL) {
		DE_WRN("set_mode func is null!\n");
		return DIS_FAIL;
	}

	ret = vgap->tv_func.tv_set_mode(vga->disp, vga_mode);
	if (ret == 0)
		vgap->vga_mode = vga_mode;

	return ret;
}

static s32 disp_vga_get_mode(struct disp_device* vga)
{
	enum disp_tv_mode  vga_mode;
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	if (vgap->tv_func.tv_get_mode == NULL) {
		DE_WRN("set_mode is null!\n");
		return -1;
	}

	vga_mode = vgap->tv_func.tv_get_mode(vga->disp);

	if (vga_mode != vgap->vga_mode)
		vgap->vga_mode = vga_mode;

	return vgap->vga_mode;
}

static s32 disp_vga_get_input_csc(struct disp_device* vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	if (vgap->tv_func.tv_get_input_csc == NULL)
		return DIS_FAIL;

	return vgap->tv_func.tv_get_input_csc(vga->disp);			//0 or 1.
}

static s32 disp_vga_set_func(struct disp_device*  vga, struct disp_tv_func * func)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	vgap->tv_func.tv_enable = func->tv_enable;
	vgap->tv_func.tv_disable = func->tv_disable;
	vgap->tv_func.tv_suspend = func->tv_suspend;
	vgap->tv_func.tv_resume = func->tv_resume;
	vgap->tv_func.tv_get_mode = func->tv_get_mode;
	vgap->tv_func.tv_set_mode = func->tv_set_mode;
	vgap->tv_func.tv_get_input_csc = func->tv_get_input_csc;
	vgap->tv_func.tv_get_video_timing_info = func->tv_get_video_timing_info;
	vgap->tv_func.tv_mode_support = func->tv_mode_support;
	vgap->tv_func.tv_hot_plugging_detect = func->tv_hot_plugging_detect;
	vgap->tv_func.tv_set_enhance_mode = func->tv_set_enhance_mode;

	return 0;
}

static s32 disp_vga_check_support_mode(struct disp_device*  vga, enum disp_tv_mode vga_mode)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	if (vgap->tv_func.tv_get_input_csc == NULL)
		return DIS_FAIL;
	return vgap->tv_func.tv_mode_support(vga->disp, vga_mode);
}

static s32 disp_vga_set_hpd(struct disp_device*  vga, u32 state)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	if (vgap->tv_func.tv_hot_plugging_detect== NULL)
		return DIS_FAIL;

	return vgap->tv_func.tv_hot_plugging_detect(state);
}

static s32 disp_set_enhance_mode(struct disp_device *vga, u32 mode)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("device hdl is NULL!\n");
		return DIS_FAIL;
	}

	if (vgap->tv_func.tv_hot_plugging_detect== NULL) {
		DE_WRN("set_enhance_mode is null!\n");
		return DIS_FAIL;
	}

	return vgap->tv_func.tv_set_enhance_mode(vga->disp, mode);
}

static s32 disp_vga_get_fps(struct disp_device *vga)
{
	struct disp_vga_private_data *vgap = disp_vga_get_priv(vga);

	if ((NULL == vga) || (NULL == vgap)) {
		DE_WRN("vga set func null  hdl!\n");
		return 0;
	}

	return vgap->frame_per_sec;
}

s32 disp_init_vga(void)
{
	u32 value = 0;
	u32 ret = 0;
	char type_name[32];
	char compat[32];
	char status[10] = {0};
	const char *str;
	struct device_node *node;
	u32 num_devices;
	u32 disp = 0;
	struct disp_device *vga;
	struct disp_vga_private_data *vgap;
	u32 hwdev_index = 0;
	u32 num_devices_support_vga = 0;

	sprintf(compat, "allwinner,sunxi-tv");
	num_devices = bsp_disp_feat_get_num_devices();
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (bsp_disp_feat_is_supported_output_types(hwdev_index,
							DISP_OUTPUT_TYPE_VGA))
			num_devices_support_vga++;
	}
	vgas = kmalloc(sizeof(*vga) * num_devices_support_vga,
			GFP_KERNEL | __GFP_ZERO);
	if (NULL == vgas) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	vga_private = kmalloc(sizeof(*vgap) * num_devices_support_vga,
				GFP_KERNEL | __GFP_ZERO);
	if (NULL == vga_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	disp = 0;
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (!bsp_disp_feat_is_supported_output_types(hwdev_index,
							DISP_OUTPUT_TYPE_VGA)) {
			DE_WRN("screen %d do not support VGA TYPE!\n",
				hwdev_index);
			continue;
		}
		sprintf(type_name, "tv%d", disp);
		node = of_find_compatible_node(NULL, type_name, compat);

		ret = of_property_read_string(node, "status", &str);
		memcpy((void *)status, str, strlen(str)+1);
		if (ret || strcmp(status, "okay")) {
			DE_WRN("disp%d not support vga\n", disp);
			disp++;
			continue;
		}

		ret = of_property_read_u32_array(node, "interface", &value, 1);
		if (ret || value != DISP_VGA) {
			DE_WRN("disp%d not support vga\n", disp);
			disp++;
			continue;
		}

		vga = &vgas[disp];
		vgap = &vga_private[disp];
		vga->priv_data = (void *)vgap;

		spin_lock_init(&vgap->data_lock);
		mutex_init(&vgap->mlock);
		vga->disp = disp;
		vga->hwdev_index = hwdev_index;
		sprintf(vga->name, "vga%d", disp);
		vga->type = DISP_OUTPUT_TYPE_VGA;
		vgap->vga_mode = DISP_TV_MOD_PAL;
		vgap->irq_no = gdisp.init_para.irq_no[DISP_MOD_LCD0 +
							hwdev_index];
		vgap->clk = gdisp.init_para.mclk[DISP_MOD_LCD0 + hwdev_index];

		vga->set_manager = disp_device_set_manager;
		vga->unset_manager = disp_device_unset_manager;
		vga->get_resolution = disp_device_get_resolution;
		vga->get_timings = disp_device_get_timings;

		vga->init =  disp_vga_init;
		vga->exit =  disp_vga_exit;
		vga->set_tv_func = disp_vga_set_func;
		vga->enable = disp_vga_enable;
		vga->sw_enable = disp_vga_sw_enable;
		vga->disable = disp_vga_disable;
		vga->is_enabled = disp_vga_is_enabled;
		vga->set_mode = disp_vga_set_mode;
		vga->get_mode = disp_vga_get_mode;
		vga->check_support_mode = disp_vga_check_support_mode;
		vga->get_input_csc = disp_vga_get_input_csc;
		vga->suspend = disp_vga_suspend;
		vga->resume = disp_vga_resume;
		vga->set_enhance_mode = disp_set_enhance_mode;
		vga->get_fps = disp_vga_get_fps;
		vga->init(vga);
		disp_device_register(vga);
		disp++;
	}

	return 0;
}

#endif

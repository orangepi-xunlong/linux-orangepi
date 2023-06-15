/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/reset.h>
#include "disp_capture.h"

struct disp_capture_info_list {
	struct disp_capture_info_inner info;
	struct list_head list;
	bool done;
	struct dmabuf_item *item;
};

struct disp_capture_private_data {
	u32 reg_base;
	u32 enabled;
	u32 applied;
	s32 status; /* 0: finish; other: fail&err */

	u32 done_sum;
	struct list_head req_list;
	u32 req_cnt;
	struct list_head runing_list;
	u32 runing_cnt;
	struct list_head done_list;
	u32 done_cnt;
	s32 (*shadow_protect)(u32 sel, bool protect);

	struct clk *clk;
	struct clk *clk_bus;
	struct reset_control *rst;

#if defined(__LINUX_PLAT__)
	struct mutex mlock;
	spinlock_t data_lock;
#else
	int mlock;
	int data_lock;
#endif
	struct disp_capture_info_list *cur_info;
	struct disp_irq_info irq_info;
	spinlock_t reg_lock;
	u32 reg_protect_cnt; /* protect reg for user's writing */
	u8 reg_access; /* access reg for clearing */
};

static struct disp_capture *captures;
static struct disp_capture_private_data *capture_private;
struct cptr_wq_t {
	struct work_struct wq;
	struct disp_capture *cptr;
	u32 rcq_state;
};

static struct cptr_wq_t rcq_cptr_wq;

struct disp_capture *disp_get_capture(u32 disp)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp >= num_screens) {
		DE_WRN("disp %d out of range\n", disp);
		return NULL;
	}

	if (!bsp_disp_feat_is_support_capture(disp)) {
		DE_INF("screen %d not support capture\n", disp);
		return NULL;
	}

	return &captures[disp];
}

static struct disp_capture_private_data *disp_capture_get_priv(struct
							       disp_capture
							       *cptr)
{
	if (cptr == NULL) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	if (!bsp_disp_feat_is_support_capture(cptr->disp)) {
		DE_WRN("screen %d not support capture\n", cptr->disp);
		return NULL;
	}

	return &capture_private[cptr->disp];
}

s32 disp_capture_shadow_protect(struct disp_capture *capture, bool protect)
{
	struct disp_capture_private_data *capturep =
	    disp_capture_get_priv(capture);

	if ((capture == NULL) || (capturep == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (capturep->shadow_protect)
		return capturep->shadow_protect(capture->disp, protect);

	return -1;
}

static s32 disp_capture_protect_reg_for_rcq(
	struct disp_capture *capture, bool protect)
{
	struct disp_capture_private_data *capturep = disp_capture_get_priv(capture);
	unsigned long flags;

	if ((NULL == capture) || (NULL == capturep)) {
		__wrn("NULL hdl!\n");
		return -1;
	}

	if (protect) {
		u32 irq_state = 0;
		u32 cnt = 0;
		u32 max_cnt = 50;
		u32 delay = 10;

		do {
			spin_lock_irqsave(&capturep->reg_lock, flags);
			if (capturep->reg_access == 0) {
				capturep->reg_protect_cnt++;
				disp_al_capture_set_rcq_update(capture->disp, 0);
				irq_state = disp_al_capture_query_irq_state(capture->disp,
					DISP_AL_CAPTURE_IRQ_STATE_RCQ_ACCEPT
					| DISP_AL_CAPTURE_IRQ_STATE_RCQ_FINISH);
				spin_unlock_irqrestore(&capturep->reg_lock, flags);
				break;
			}
			spin_unlock_irqrestore(&capturep->reg_lock, flags);

			cnt++;
			disp_delay_us(delay);
		} while (cnt < max_cnt);

		if (capturep->reg_access != 0) {
			spin_lock_irqsave(&capturep->reg_lock, flags);
			capturep->reg_protect_cnt++;
			disp_al_capture_set_rcq_update(capture->disp, 0);
			irq_state = disp_al_capture_query_irq_state(capture->disp,
				DISP_AL_CAPTURE_IRQ_STATE_RCQ_ACCEPT
				| DISP_AL_CAPTURE_IRQ_STATE_RCQ_FINISH);
			spin_unlock_irqrestore(&capturep->reg_lock, flags);
		}

		if (irq_state & DISP_AL_CAPTURE_IRQ_STATE_RCQ_ACCEPT) {
			u32 cnt = 0;
			u32 max_cnt = 500;
			u32 delay = 1;

			while (!(irq_state & DISP_AL_CAPTURE_IRQ_STATE_RCQ_FINISH)
				&& (cnt < max_cnt)) {
				cnt++;
				disp_delay_us(delay);
				irq_state = disp_al_capture_query_irq_state(capture->disp,
					DISP_AL_CAPTURE_IRQ_STATE_RCQ_FINISH);
			}
			disp_al_capture_set_all_rcq_head_dirty(capture->disp, 0);
		}

	} else {
		spin_lock_irqsave(&capturep->reg_lock, flags);
		if (capturep->reg_protect_cnt > 0)
			capturep->reg_protect_cnt--;
		if (capturep->reg_protect_cnt == 0)
			disp_al_capture_set_rcq_update(capture->disp, 1);
		spin_unlock_irqrestore(&capturep->reg_lock, flags);
	}

	return 0;
}

static void rcq_cptr_sync(struct work_struct *work)
{
	struct cptr_wq_t *cpwq = NULL;

	if (!work)
		return;
	cpwq = container_of(work, struct cptr_wq_t, wq);
	if (!cpwq || !cpwq->cptr)
		return;
	disp_capture_sync(cpwq->cptr);
}

static s32 disp_capture_rcq_finish_irq_handler(
	struct disp_capture *capture)
{
	struct disp_capture_private_data *capturep =
		disp_capture_get_priv(capture);
	unsigned long flags;
	u32 irq_state = 0;

	if ((capture == NULL) || (capturep == NULL)) {
		__wrn("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&capturep->reg_lock, flags);
	if (capturep->reg_protect_cnt > 0) {
		spin_unlock_irqrestore(&capturep->reg_lock, flags);
		return 0;
	}
	capturep->reg_access = 1;
	irq_state = disp_al_capture_query_irq_state(capture->disp,
		DISP_AL_CAPTURE_IRQ_STATE_RCQ_ACCEPT
		| DISP_AL_CAPTURE_IRQ_STATE_RCQ_FINISH);
	capturep->reg_access = 0;
	spin_unlock_irqrestore(&capturep->reg_lock, flags);

	if (irq_state & DISP_AL_CAPTURE_IRQ_STATE_RCQ_FINISH) {
		if (rcq_cptr_wq.rcq_state == 1) {
			rcq_cptr_wq.cptr = capture;
			schedule_work(&rcq_cptr_wq.wq);
		} else if (rcq_cptr_wq.rcq_state == 2) {
			disp_al_capture_set_all_rcq_head_dirty(capture->disp,
					0);
			disp_al_capture_sync(capture->disp);
			rcq_cptr_wq.rcq_state = 0;
		}
	}
	if (rcq_cptr_wq.rcq_state < 1) {
		rcq_cptr_wq.rcq_state = 1;
		rcq_cptr_wq.cptr = capture;
		schedule_work(&rcq_cptr_wq.wq);
	}

	return 0;
}


s32 disp_capture_irq_handler(u32 sel, u32 irq_flag, void *ptr)
{
	if (irq_flag & DISP_AL_CAPTURE_IRQ_FLAG_RCQ_FINISH)
		disp_capture_rcq_finish_irq_handler((struct disp_capture *)ptr);

	return 0;
}

static s32 disp_capture_clk_init(struct disp_capture *cptr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (cptrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}
	/* todo: int the clock */

	return 0;
}

static s32 disp_capture_clk_exit(struct disp_capture *cptr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (cptrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	/* todo: colse the clock */

	return 0;
}

static s32 disp_capture_clk_enable(struct disp_capture *cptr)
{
	int ret;
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (cptrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	ret = reset_control_deassert(cptrp->rst);
	if (ret) {
		DE_WRN("%s: reset_control_deassert for rst failed\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(cptrp->clk);
	if (ret) {
		DE_WRN("%s: clk_prepare_enable for clk failed\n", __func__);
		return ret;
	}

	ret = clk_prepare_enable(cptrp->clk_bus);
	if (ret) {
		DE_WRN("%s: clk_prepare_enable for clk_bus failed\n", __func__);
		return ret;;
	}

	return 0;
}

static s32 disp_capture_clk_disable(struct disp_capture *cptr)
{
	int ret;
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (cptrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	clk_disable_unprepare(cptrp->clk_bus);
	clk_disable_unprepare(cptrp->clk);

	ret = reset_control_assert(cptrp->rst);
	if (ret) {
		DE_WRN("%s: reset_control_assert for rst failed\n", __func__);
		return ret;
	}

	return 0;
}

s32 disp_capture_apply(struct disp_capture *cptr)
{
	return 0;
}

s32 disp_capture_force_apply(struct disp_capture *cptr)
{
	return 0;
}

s32 disp_capture_start(struct disp_capture *cptr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (cptrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	rcq_cptr_wq.rcq_state = 0;
	DE_INF("cap %d\n", cptr->disp);

	mutex_lock(&cptrp->mlock);
	if (cptrp->enabled == 1) {
		DE_WRN("capture %d already started!\n", cptr->disp);
		mutex_unlock(&cptrp->mlock);
		return -1;
	}
	if (cptrp->irq_info.irq_flag)
		disp_register_irq(cptr->disp + DISP_SCREEN_NUM,
				  &cptrp->irq_info);
	disp_capture_clk_enable(cptr);
	disp_al_capture_init(cptr->disp);
	//TODO:move this function to disp_al_capture_init
	disp_al_capture_set_irq_enable(cptr->disp,
		cptrp->irq_info.irq_flag, 1);
	cptrp->enabled = 1;
	mutex_unlock(&cptrp->mlock);

	return 0;
}

s32 disp_capture_stop(struct disp_capture *cptr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);
	unsigned long flags;
	struct disp_capture_info_list *info_list = NULL, *temp = NULL;
	struct list_head drop_list;

	if ((cptr == NULL) || (cptrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("cap %d\n", cptr->disp);

	INIT_LIST_HEAD(&drop_list);

	mutex_lock(&cptrp->mlock);
	if (cptrp->enabled == 1) {
		disp_al_capture_set_irq_enable(cptr->disp,
			cptrp->irq_info.irq_flag, 0);
		disp_al_capture_query_irq_state(cptr->disp,
			DISP_AL_CAPTURE_IRQ_STATE_MASK);
		disp_al_capture_exit(cptr->disp);
		disp_capture_clk_disable(cptr);
		if (cptrp->irq_info.irq_flag)
			disp_unregister_irq(cptr->disp + DISP_SCREEN_NUM,
				&cptrp->irq_info);
		cptrp->enabled = 0;
	}
	spin_lock_irqsave(&cptrp->data_lock, flags);
	list_for_each_entry_safe(info_list, temp, &cptrp->runing_list, list) {
		list_del(&info_list->list);
		list_add_tail(&info_list->list, &drop_list);
		cptrp->runing_cnt--;
	}
	list_for_each_entry_safe(info_list, temp, &cptrp->done_list, list) {
		list_del(&info_list->list);
		list_add_tail(&info_list->list, &drop_list);
		cptrp->done_cnt--;
	}
	list_for_each_entry_safe(info_list, temp, &cptrp->req_list, list) {
		list_del(&info_list->list);
		list_add_tail(&info_list->list, &drop_list);
		cptrp->req_cnt--;
	}
	spin_unlock_irqrestore(&cptrp->data_lock, flags);
	list_for_each_entry_safe(info_list, temp, &drop_list, list) {
		list_del(&info_list->list);
		if (info_list->item)
			disp_dma_unmap(info_list->item);
		kfree(info_list);
	}
	mutex_unlock(&cptrp->mlock);

	return 0;
}

static void
__disp_capture_info_transfer(struct disp_capture_info_inner *info_inner,
			    struct disp_capture_info *info)
{
	memcpy(&info_inner->window, &info->window, sizeof(struct disp_rect));
	info_inner->out_frame.format = info->out_frame.format;
	memcpy(info_inner->out_frame.size,
	       info->out_frame.size,
	       sizeof(struct disp_rectsz) * 3);
	memcpy(&info_inner->out_frame.crop,
	       &info->out_frame.crop,
	       sizeof(struct disp_rect));
	memcpy(info_inner->out_frame.addr,
	       info->out_frame.addr,
	       sizeof(long long) * 3);
}

static void
__disp_capture_info2_transfer(struct disp_capture_info_inner *info_inner,
			    struct disp_capture_info2 *info)
{
	memcpy(&info_inner->window, &info->window, sizeof(struct disp_rect));
	info_inner->out_frame.format = info->out_frame.format;
	memcpy(info_inner->out_frame.size,
	       info->out_frame.size,
	       sizeof(struct disp_rectsz) * 3);
	memcpy(&info_inner->out_frame.crop,
	       &info->out_frame.crop,
	       sizeof(struct disp_rect));
	info_inner->out_frame.fd = info->out_frame.fd;
}

s32 disp_capture_commit(struct disp_capture *cptr,
			struct disp_capture_info *info)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);
	struct disp_manager *mgr;
	struct disp_device *dispdev = NULL;
	struct disp_capture_info_list *info_list, *temp;
	unsigned long flags;
	enum disp_csc_type cs = DISP_CSC_TYPE_RGB;
	struct list_head done_list;
	int ret = -1;

	if (NULL == cptr || NULL == cptrp) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	INIT_LIST_HEAD(&done_list);
	DE_INF("cap %d\n", cptr->disp);

	mgr = cptr->manager;
	if ((mgr == NULL) || (mgr->is_enabled(mgr) == 0)) {
		DE_WRN("manager disable!\n");
		return -1;
	}

	dispdev = mgr->device;
	if (NULL == dispdev) {
		DE_WRN("disp device is NULL!\n");
		return -1;
	}

	if (dispdev->get_input_csc)
		cs = dispdev->get_input_csc(dispdev);
#ifdef SUPPORT_YUV_BLEND
	if ((DISP_CSC_TYPE_RGB != cs) &&
	    ((info->out_frame.format == DISP_FORMAT_ARGB_8888) ||
	     (info->out_frame.format == DISP_FORMAT_ABGR_8888) ||
	     (info->out_frame.format == DISP_FORMAT_RGBA_8888) ||
	     (info->out_frame.format == DISP_FORMAT_BGRA_8888) ||
	     (info->out_frame.format == DISP_FORMAT_RGB_888) ||
	     (info->out_frame.format == DISP_FORMAT_BGR_888))) {
		DE_WRN("in_fmt and out_fmt not match!\n");
		return -1;
	}
#endif

#ifndef WB_HAS_CSC
	if ((DISP_CSC_TYPE_YUV444 == cs) || (DISP_CSC_TYPE_YUV422 == cs) ||
	    (DISP_CSC_TYPE_YUV420 == cs)) {
		if ((info->out_frame.format != DISP_FORMAT_YUV420_P) &&
		    (info->out_frame.format != DISP_FORMAT_YUV420_SP_UVUV) &&
		    (info->out_frame.format != DISP_FORMAT_YUV420_SP_VUVU) &&
		    (info->out_frame.format != DISP_FORMAT_YUV444_I_AYUV)) {
			DE_WRN("out_format is not support!\n");
			return -1;
		}
	}
#endif

	DE_INF
	    ("disp%d,fmt %d,pitch<%d,%d,%d>,crop<%d,%d,%d,%d>,addr<0x%llx,0x%llx,0x%llx>\n",
	     cptr->disp, info->out_frame.format, info->out_frame.size[0].width,
	     info->out_frame.size[1].width, info->out_frame.size[2].width,
	     info->out_frame.crop.x, info->out_frame.crop.y,
	     info->out_frame.crop.width, info->out_frame.crop.height,
	     info->out_frame.addr[0], info->out_frame.addr[1],
	     info->out_frame.addr[2]);

	mutex_lock(&cptrp->mlock);
	if (cptrp->enabled == 0) {
		DE_WRN("capture %d is disabled!\n", cptr->disp);
		goto EXIT;
	}
	if (disp_feat_is_using_wb_rcq(cptr->disp)) {
		struct disp_capture_config config;
		u32 width = 0, height = 0;

		memset(&config, 0, sizeof(struct disp_capture_config));

		config.disp = cptr->disp;

		config.out_frame.format = info->out_frame.format;
		memcpy(config.out_frame.size,
			   info->out_frame.size,
			   sizeof(struct disp_rectsz) * 3);
		memcpy(&config.out_frame.crop,
			   &info->out_frame.crop,
			   sizeof(struct disp_rect));
		memcpy(config.out_frame.addr,
			   info->out_frame.addr,
			   sizeof(long long) * 3);

		memcpy(&config.in_frame.crop,
			&info->window,
			sizeof(struct disp_rect));
		if (DISP_CSC_TYPE_RGB == cs)
			config.in_frame.format = DISP_FORMAT_ARGB_8888;
		else if (DISP_CSC_TYPE_YUV444  == cs)
			config.in_frame.format = DISP_FORMAT_YUV444_P;
		else if (DISP_CSC_TYPE_YUV422 == cs)
			config.in_frame.format = DISP_FORMAT_YUV422_P;
		else
			config.in_frame.format = DISP_FORMAT_YUV420_P;

		if (dispdev->get_resolution) {
			dispdev->get_resolution(dispdev, &width, &height);
		}
		config.in_frame.size[0].width = width;
		config.in_frame.size[1].width = width;
		config.in_frame.size[2].width = width;
		config.in_frame.size[0].height = height;
		config.in_frame.size[1].height = height;
		config.in_frame.size[2].height = height;
		if ((0 == config.in_frame.crop.width)
			|| (0 == config.in_frame.crop.height)) {
			config.in_frame.crop.width = width;
			config.in_frame.crop.height = height;
		}

		DE_INF(
		    "disp:%d flag:%d in_fmt:%d in_fd:%d in_crop[%d %d %d "
		    "%d],in_size[%u %u, %u %u, %u "
		    "%u],in_addr[0x%llx,0x%llx,0x%llx],out_fmt:%d out_fd:%d "
		    "out_crop[%d %d %d %d],out_size[%u %u, %u %u, %u "
		    "%u],out_addr[0x%llx,0x%llx,0x%llx]\n",
		    config.disp, config.flags, config.in_frame.format,
		    config.in_frame.fd, config.in_frame.crop.x,
		    config.in_frame.crop.y, config.in_frame.crop.width,
		    config.in_frame.crop.height, config.in_frame.size[0].width,
		    config.in_frame.size[0].height,
		    config.in_frame.size[1].width,
		    config.in_frame.size[1].height,
		    config.in_frame.size[2].width,
		    config.in_frame.size[2].height, config.in_frame.addr[0],
		    config.in_frame.addr[1], config.in_frame.addr[2],
		    config.out_frame.format, config.out_frame.fd,
		    config.out_frame.crop.x, config.out_frame.crop.y,
		    config.out_frame.crop.width, config.out_frame.crop.height,
		    config.out_frame.size[0].width,
		    config.out_frame.size[0].height,
		    config.out_frame.size[1].width,
		    config.out_frame.size[1].height,
		    config.out_frame.size[2].width,
		    config.out_frame.size[2].height, config.out_frame.addr[0],
		    config.out_frame.addr[1], config.out_frame.addr[2]);

		rcq_cptr_wq.rcq_state = 2;
		disp_capture_protect_reg_for_rcq(cptr, 1);
		disp_al_capture_apply(cptr->disp, &config);
		disp_capture_protect_reg_for_rcq(cptr, 0);

		ret = 0;
		goto EXIT;
	}
	info_list =
	    kmalloc(sizeof(struct disp_capture_info_list),
		    GFP_KERNEL | __GFP_ZERO);
	if (info_list == NULL) {
		DE_WRN("malloc fail!\n");
		goto EXIT;
	}
	__disp_capture_info_transfer(&info_list->info, info);
	spin_lock_irqsave(&cptrp->data_lock, flags);
	list_add_tail(&info_list->list, &cptrp->req_list);
	cptrp->req_cnt++;

	list_for_each_entry_safe(info_list, temp, &cptrp->done_list, list) {
		list_del(&info_list->list);
		list_add_tail(&info_list->list, &done_list);
		cptrp->done_cnt--;
	}
	spin_unlock_irqrestore(&cptrp->data_lock, flags);
	list_for_each_entry_safe(info_list, temp, &done_list, list) {
		list_del(&info_list->list);
		if (info_list->item)
			disp_dma_unmap(info_list->item);
		kfree(info_list);
	}
	ret = 0;
EXIT:
	mutex_unlock(&cptrp->mlock);

	return ret;
}

static s32 disp_capture_commit2(struct disp_capture *cptr,
				struct disp_capture_info2 *info)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);
	struct disp_manager *mgr;
	struct disp_capture_info_list *info_list, *temp;
	unsigned long flags;
	struct dmabuf_item *item;
	struct disp_device *dispdev = NULL;
	int ret = -1;
	struct list_head done_list;
	enum disp_csc_type cs = DISP_CSC_TYPE_RGB;
	struct fb_address_transfer fb;

	INIT_LIST_HEAD(&done_list);
	if (NULL == cptr || NULL == cptrp) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("cap %d\n", cptr->disp);

	mgr = cptr->manager;
	if ((NULL == mgr) || (0 == mgr->is_enabled(mgr))) {
		DE_WRN("manager disable!\n");
		return -1;
	}

	mutex_lock(&cptrp->mlock);
	if (0 == cptrp->enabled) {
		DE_WRN("capture %d is disabled!\n", cptr->disp);
		goto exit;
	}
	info_list = kmalloc(sizeof(struct disp_capture_info_list),
			    GFP_KERNEL | __GFP_ZERO);
	if (NULL == info_list) {
		DE_WRN("malloc fail!\n");
		goto exit;
	}
	__disp_capture_info2_transfer(&info_list->info, info);
	item = disp_dma_map(info_list->info.out_frame.fd);
	if (item == NULL) {
		DE_WRN("disp dma map fail!\n");
		kfree(info_list);
		goto exit;
	}
	fb.format = info_list->info.out_frame.format;
	memcpy(fb.size, info_list->info.out_frame.size,
	       sizeof(struct disp_rectsz) * 3);
	fb.dma_addr = item->dma_addr;
	fb.align[0] = 0;
	fb.align[1] = 0;
	fb.align[2] = 0;
	disp_set_fb_info(&fb, true);
	memcpy(info_list->info.out_frame.addr, fb.addr, sizeof(long long) * 3);

	info_list->item = item;
	/*
	info_list->info.out_frame.addr[0] = item->dma_addr;
	info_list->info.out_frame.addr[1] =
	    item->dma_addr +
	    info_list->info.out_frame.size[0].width *
		info_list->info.out_frame.size[0].height;
	info_list->info.out_frame.addr[2] =
	    info_list->info.out_frame.addr[1] +
	    info_list->info.out_frame.size[0].width *
		info_list->info.out_frame.size[0].height / 4;
	*/

	if (disp_feat_is_using_wb_rcq(cptr->disp)) {
		struct disp_capture_config config;
		u32 width = 0, height = 0;

		memset(&config, 0, sizeof(struct disp_capture_config));

		config.disp = cptr->disp;

		memcpy(&config.out_frame,
		       &info_list->info.out_frame,
		       sizeof(struct disp_s_frame_inner));

		memcpy(&config.in_frame.crop,
			&info_list->info.window,
			sizeof(struct disp_rect));
		dispdev = mgr->device;
		if (!dispdev) {
			DE_WRN("disp device is NULL!\n");
			return -1;
		}
		if (dispdev->get_input_csc)
			cs = dispdev->get_input_csc(dispdev);
		if (DISP_CSC_TYPE_RGB == cs)
			config.in_frame.format = DISP_FORMAT_ARGB_8888;
		else if (DISP_CSC_TYPE_YUV444  == cs)
			config.in_frame.format = DISP_FORMAT_YUV444_P;
		else if (DISP_CSC_TYPE_YUV422 == cs)
			config.in_frame.format = DISP_FORMAT_YUV422_P;
		else
			config.in_frame.format = DISP_FORMAT_YUV420_P;

		if (dispdev->get_resolution)
			dispdev->get_resolution(dispdev, &width, &height);
		config.in_frame.size[0].width = width;
		config.in_frame.size[1].width = width;
		config.in_frame.size[2].width = width;
		config.in_frame.size[0].height = height;
		config.in_frame.size[1].height = height;
		config.in_frame.size[2].height = height;
		if ((0 == config.in_frame.crop.width)
			|| (0 == config.in_frame.crop.height)) {
			config.in_frame.crop.width = width;
			config.in_frame.crop.height = height;
		}

		DE_INF(
		    "disp:%d flag:%d in_fmt:%d in_fd:%d in_crop[%d %d %d "
		    "%d],in_size[%u %u, %u %u, %u "
		    "%u],in_addr[0x%llx,0x%llx,0x%llx],out_fmt:%d out_fd:%d "
		    "out_crop[%d %d %d %d],out_size[%u %u, %u %u, %u "
		    "%u],out_addr[0x%llx,0x%llx,0x%llx]\n",
		    config.disp, config.flags, config.in_frame.format,
		    config.in_frame.fd, config.in_frame.crop.x,
		    config.in_frame.crop.y, config.in_frame.crop.width,
		    config.in_frame.crop.height, config.in_frame.size[0].width,
		    config.in_frame.size[0].height,
		    config.in_frame.size[1].width,
		    config.in_frame.size[1].height,
		    config.in_frame.size[2].width,
		    config.in_frame.size[2].height, config.in_frame.addr[0],
		    config.in_frame.addr[1], config.in_frame.addr[2],
		    config.out_frame.format, config.out_frame.fd,
		    config.out_frame.crop.x, config.out_frame.crop.y,
		    config.out_frame.crop.width, config.out_frame.crop.height,
		    config.out_frame.size[0].width,
		    config.out_frame.size[0].height,
		    config.out_frame.size[1].width,
		    config.out_frame.size[1].height,
		    config.out_frame.size[2].width,
		    config.out_frame.size[2].height, config.out_frame.addr[0],
		    config.out_frame.addr[1], config.out_frame.addr[2]);

		/*
		disp_capture_protect_reg_for_rcq(cptr, 1);
		disp_al_capture_apply(cptr->disp, &config);
		disp_capture_protect_reg_for_rcq(cptr, 0);
		list_add_tail(&info_list->list, &cptrp->done_list);

		cptrp->done_sum++;

		ret = 0;
		goto exit;
		*/
	}

	spin_lock_irqsave(&cptrp->data_lock, flags);
	list_add_tail(&info_list->list, &cptrp->req_list);
	cptrp->req_cnt++;

	list_for_each_entry_safe(info_list, temp, &cptrp->done_list, list) {
		if (cptrp->done_cnt >= 2) {
			list_del(&info_list->list);
			list_add_tail(&info_list->list, &done_list);
			cptrp->done_cnt--;
		}
	}
	spin_unlock_irqrestore(&cptrp->data_lock, flags);

	list_for_each_entry_safe(info_list, temp, &done_list, list) {
		list_del(&info_list->list);
		if (info_list->item)
			disp_dma_unmap(info_list->item);
		kfree(info_list);
	}
	ret = 0;
exit:
	mutex_unlock(&cptrp->mlock);
	return ret;
}

s32 disp_capture_query(struct disp_capture *cptr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (cptrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	return cptrp->status;
}

s32 disp_capture_sync(struct disp_capture *cptr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);
	struct disp_manager *mgr = NULL;
	struct disp_device *dispdev = NULL;
	s32 ret = 0;
	unsigned long flags;

	if ((NULL == cptr) || (NULL == cptrp)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	/*
	if (disp_feat_is_using_wb_rcq(cptr->disp))
		disp_delay_us(2 * 50);
	*/

	mgr = cptr->manager;
	if ((NULL == mgr) || (0 == mgr->is_enabled(mgr))) {
		rcq_cptr_wq.rcq_state = 0;
		/*DE_WRN("mgr is disable!\n");*/
		return 0;
	}
	dispdev = mgr->device;
	if (NULL == dispdev) {
		DE_WRN("disp device is NULL!\n");
		return 0;
	}

	if (1 == cptrp->enabled) {
		struct disp_capture_info_list *info_list = NULL, *temp;
		struct disp_capture_info_list *running = NULL;
		bool find = false;
		bool run = false;

		spin_lock_irqsave(&cptrp->data_lock, flags);
		list_for_each_entry_safe(running, temp, &cptrp->runing_list,
					 list) {
			list_del(&running->list);
			cptrp->runing_cnt--;

			list_add_tail(&running->list, &cptrp->done_list);
			cptrp->done_cnt++;
			cptrp->done_sum++;
			run = true;
			break;
		}

		list_for_each_entry_safe(info_list, temp, &cptrp->req_list,
					 list) {
			list_del(&info_list->list);
			cptrp->req_cnt--;

			list_add_tail(&info_list->list, &cptrp->runing_list);
			cptrp->runing_cnt++;
			find = true;
			break;
		}
		spin_unlock_irqrestore(&cptrp->data_lock, flags);

		ret = disp_al_capture_get_status(cptr->disp);
		cptrp->status = ret;
		if (run)
			running->done = (ret == 0) ? true : false;

		if (find) {
			struct disp_capture_config config;
			enum disp_csc_type cs = DISP_CSC_TYPE_RGB;
			u32 width = 0, height = 0;

			memset(&config, 0, sizeof(struct disp_capture_config));
			memcpy(&config.out_frame, &info_list->info.out_frame,
			       sizeof(struct disp_s_frame_inner));
			config.disp = cptr->disp;
			memcpy(&config.in_frame.crop, &info_list->info.window,
			       sizeof(struct disp_rect));
			if (dispdev->get_input_csc) {
				cs = dispdev->get_input_csc(dispdev);
			}
			if (DISP_CSC_TYPE_RGB == cs)
				config.in_frame.format = DISP_FORMAT_ARGB_8888;
			else if (DISP_CSC_TYPE_YUV444 == cs)
				config.in_frame.format = DISP_FORMAT_YUV444_P;
			else if (DISP_CSC_TYPE_YUV422 == cs)
				config.in_frame.format = DISP_FORMAT_YUV422_P;
			else
				config.in_frame.format = DISP_FORMAT_YUV420_P;
			if (dispdev->get_resolution) {
				dispdev->get_resolution(dispdev, &width,
							&height);
			}
			config.in_frame.size[0].width = width;
			config.in_frame.size[1].width = width;
			config.in_frame.size[2].width = width;
			config.in_frame.size[0].height = height;
			config.in_frame.size[1].height = height;
			config.in_frame.size[2].height = height;
			if ((0 == config.in_frame.crop.width) ||
			    (0 == config.in_frame.crop.height)) {
				config.in_frame.crop.width = width;
				config.in_frame.crop.height = height;
			}

			if (disp_feat_is_using_wb_rcq(cptr->disp)) {
				disp_capture_protect_reg_for_rcq(cptr, 1);
				disp_al_capture_apply(cptr->disp, &config);
				disp_al_capture_sync(cptr->disp);
				//disp_al_capture_set_rcq_update(cptr->disp, 1);
				disp_capture_protect_reg_for_rcq(cptr, 0);
			} else {
				disp_al_capture_apply(cptr->disp, &config);
				disp_al_capture_sync(cptr->disp);
			}
		} else {
			rcq_cptr_wq.rcq_state = 0;
		}
	}

	return 0;
}

s32 disp_capture_set_manager(struct disp_capture *cptr,
			     struct disp_manager *mgr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (mgr == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	mutex_lock(&cptrp->mlock);
	cptr->manager = mgr;
	if (mgr)
		mgr->cptr = cptr;
	mutex_unlock(&cptrp->mlock);
	return 0;
}

s32 disp_capture_unset_manager(struct disp_capture *cptr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if (cptr == NULL) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	mutex_lock(&cptrp->mlock);
	if (cptr->manager)
		cptr->manager->cptr = NULL;
	cptr->manager = NULL;
	mutex_unlock(&cptrp->mlock);
	return 0;
}

s32 disp_capture_suspend(struct disp_capture *cptr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (cptrp == NULL)) {
		DE_WRN("capture NULL hdl!\n");
		return -1;
	}

	return 0;
}

s32 disp_capture_resume(struct disp_capture *cptr)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (cptrp == NULL)) {
		DE_WRN("capture NULL hdl!\n");
		return -1;
	}

	return 0;

}

static s32 disp_capture_dump(struct disp_capture *cptr, char *buf)
{
	struct disp_capture_private_data *cptrp = disp_capture_get_priv(cptr);
	unsigned int count = 0;

	if ((NULL == cptr) || (NULL == cptrp)) {
		DE_WRN("capture NULL hdl!\n");
		return -1;
	}

	count += sprintf(buf + count,
			 "capture: %3s req[%u] runing[%u] done[%d,%u]\n",
			 (cptrp->enabled == 1) ? "en" : "dis", cptrp->req_cnt,
			 cptrp->runing_cnt, cptrp->done_cnt, cptrp->done_sum);

	return count;
}

s32 disp_capture_init(struct disp_capture *cptr)
{
	struct disp_capture_private_data *capturep =
	    disp_capture_get_priv(cptr);

	if ((cptr == NULL) || (capturep == NULL)) {
		DE_WRN("capture NULL hdl!\n");
		return -1;
	}

	if (!bsp_disp_feat_is_support_capture(cptr->disp)) {
		DE_WRN("capture %d is not support\n", cptr->disp);
		return -1;
	}

	disp_capture_clk_init(cptr);
	return 0;
}

s32 disp_capture_exit(struct disp_capture *cptr)
{
	if (!bsp_disp_feat_is_support_capture(cptr->disp)) {
		DE_WRN("capture %d is not support\n", cptr->disp);
		return -1;
	}
	disp_capture_clk_exit(cptr);

	return 0;
}

s32 disp_init_capture(struct disp_bsp_init_para *para)
{
	u32 num_screens;
	u32 disp;
	struct disp_capture *capture;
	struct disp_capture_private_data *capturep;

	num_screens = bsp_disp_feat_get_num_screens();
	captures =
	    kmalloc_array(num_screens, sizeof(struct disp_capture),
			  GFP_KERNEL | __GFP_ZERO);
	if (!captures) {
		DE_WRN("malloc memory fail!\n");
		goto malloc_err;
	}
	capture_private =
	    kmalloc(sizeof(struct disp_capture_private_data)
		    * num_screens, GFP_KERNEL | __GFP_ZERO);
	if (!capture_private) {
		DE_WRN("malloc memory fail!\n");
		goto malloc_err;
	}

	INIT_WORK(&rcq_cptr_wq.wq, rcq_cptr_sync);
	for (disp = 0; disp < num_screens; disp++) {
		if (!bsp_disp_feat_is_support_capture(disp))
			continue;

		capture = &captures[disp];
		capturep = &capture_private[disp];
		mutex_init(&capturep->mlock);
		spin_lock_init(&(capturep->data_lock));
		spin_lock_init(&(capturep->reg_lock));

		capturep->clk = para->clk_de[disp];
		capturep->clk_bus = para->clk_bus_de[disp];
		capturep->rst = para->rst_bus_de[disp];

		capture->disp = disp;
		sprintf(capture->name, "capture%d", disp);

		capturep->irq_info.sel = disp;
		capturep->irq_info.irq_flag =
			disp_feat_is_using_wb_rcq(disp) ?
				DISP_AL_CAPTURE_IRQ_FLAG_RCQ_FINISH : 0;
		capturep->irq_info.ptr = (void *)capture;
		capturep->irq_info.irq_handler = disp_capture_irq_handler;

		capturep->shadow_protect = para->shadow_protect;
		capture->set_manager = disp_capture_set_manager;
		capture->unset_manager = disp_capture_unset_manager;
		capture->start = disp_capture_start;
		capture->stop = disp_capture_stop;
		capture->sync = disp_capture_sync;
		capture->init = disp_capture_init;
		capture->exit = disp_capture_exit;
		capture->commmit = disp_capture_commit;
		capture->commmit2 = disp_capture_commit2;
		capture->query = disp_capture_query;
		capture->dump = disp_capture_dump;
		INIT_LIST_HEAD(&capturep->req_list);
		INIT_LIST_HEAD(&capturep->runing_list);
		INIT_LIST_HEAD(&capturep->done_list);

		capture->init(capture);
	}
	return 0;

malloc_err:
	kfree(capture_private);
	kfree(captures);
	capture_private = NULL;
	captures = NULL;

	return -1;
}

s32 disp_exit_capture(void)
{
	u32 num_screens;
	struct disp_capture *capture;
	u32 disp;

	if (!captures)
		return 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		if (!bsp_disp_feat_is_support_capture(disp))
			continue;

		capture = &captures[disp];
		capture->exit(capture);
	}

	kfree(capture_private);
	kfree(captures);
	capture_private = NULL;
	captures = NULL;

	return 0;
}

/*
 * Allwinner SoCs display driver.
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "disp_rtwb.h"

#if defined(SUPPORT_RTWB)

struct disp_device_private_data {
	u32 enabled;
	bool suspended;

	enum disp_tv_mode tv_mode;
	struct disp_device_config config;

	struct disp_irq_info irq_info;
	struct disp_rtwb_timings *video_info;
	wait_queue_head_t wait_wb_finish_queue;
	atomic_t wati_wb_finish_flag;
	struct clk *clk;
	struct clk *clk_bus;
	struct reset_control *rst;

};

struct disp_rtwb_timings {
	unsigned int tv_mode;	/* video information code */
	unsigned int pixel_repeat; /* pixel repeat (pixel_repeat+1) times */
	unsigned int x_res;
	unsigned int y_res;
	bool b_interlace;
	unsigned int trd_mode;
	unsigned int fps;
};

static struct disp_rtwb_timings video_timing[] = {
    {
	.tv_mode = DISP_TV_MOD_720P_60HZ,
	.pixel_repeat = 0,
	.x_res = 1280,
	.y_res = 720,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_720P_50HZ,
	.pixel_repeat = 0,
	.x_res = 1280,
	.y_res = 720,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 50,
    },
    {
	.tv_mode = DISP_TV_MOD_1080I_60HZ,
	.pixel_repeat = 0,
	.x_res = 1920,
	.y_res = 1080,
	.b_interlace = 1,
	.trd_mode = 0,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_1080I_50HZ,
	.pixel_repeat = 0,
	.x_res = 1920,
	.y_res = 1080,
	.b_interlace = 1,
	.trd_mode = 0,
	.fps = 50,
    },
    {
	.tv_mode = DISP_TV_MOD_1080P_60HZ,
	.pixel_repeat = 0,
	.x_res = 1920,
	.y_res = 1080,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_1080P_50HZ,
	.pixel_repeat = 0,
	.x_res = 1920,
	.y_res = 1080,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 50,
    },
    {
	.tv_mode = DISP_TV_MOD_1080P_24HZ_3D_FP,
	.pixel_repeat = 0,
	.x_res = 1920,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 1,
	.fps = 24,
    },
    {
	.tv_mode = DISP_TV_MOD_1080P_30HZ,
	.pixel_repeat = 0,
	.x_res = 1920,
	.y_res = 1080,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 30,
    },
    {
	.tv_mode = DISP_TV_MOD_1080P_25HZ,
	.pixel_repeat = 0,
	.x_res = 1920,
	.y_res = 1080,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 25,
    },
    {
	.tv_mode = DISP_TV_MOD_1080P_24HZ,
	.pixel_repeat = 0,
	.x_res = 1920,
	.y_res = 1080,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 24,
    },
    {
	.tv_mode = DISP_TV_MOD_720P_50HZ_3D_FP,
	.pixel_repeat = 0,
	.x_res = 1280,
	.y_res = 1440,
	.b_interlace = 0,
	.trd_mode = 1,
	.fps = 50,
    },
    {
	.tv_mode = DISP_TV_MOD_720P_60HZ_3D_FP,
	.pixel_repeat = 0,
	.x_res = 1280,
	.y_res = 1440,
	.b_interlace = 0,
	.trd_mode = 1,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_3840_2160P_30HZ,
	.pixel_repeat = 0,
	.x_res = 3840,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 30,
    },
    {
	.tv_mode = DISP_TV_MOD_3840_2160P_25HZ,
	.pixel_repeat = 0,
	.x_res = 3840,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 25,
    },
    {
	.tv_mode = DISP_TV_MOD_3840_2160P_24HZ,
	.pixel_repeat = 0,
	.x_res = 3840,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 24,
    },
    {
	.tv_mode = DISP_TV_MOD_4096_2160P_24HZ,
	.pixel_repeat = 0,
	.x_res = 4096,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 24,
    },
    {
	.tv_mode = DISP_TV_MOD_4096_2160P_25HZ,
	.pixel_repeat = 0,
	.x_res = 4096,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 25
    },
    {
	.tv_mode = DISP_TV_MOD_4096_2160P_30HZ,
	.pixel_repeat = 0,
	.x_res = 4096,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 30,
    },
    {
	.tv_mode = DISP_TV_MOD_3840_2160P_60HZ,
	.pixel_repeat = 0,
	.x_res = 3840,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_4096_2160P_60HZ,
	.pixel_repeat = 0,
	.x_res = 4096,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_3840_2160P_50HZ,
	.pixel_repeat = 0,
	.x_res = 3840,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 50,
    },
    {
	.tv_mode = DISP_TV_MOD_4096_2160P_50HZ,
	.pixel_repeat = 0,
	.x_res = 4096,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 50,
    },
    {
	.tv_mode = DISP_TV_MOD_2560_1440P_60HZ,
	.pixel_repeat = 0,
	.x_res = 3840,
	.y_res = 2160,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_1440_2560P_70HZ,
	.pixel_repeat = 0,
	.x_res = 1440,
	.y_res = 2560,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 70,
    },
    {
	.tv_mode = DISP_TV_MOD_1080_1920P_60HZ,
	.pixel_repeat = 0,
	.x_res = 1080,
	.y_res = 1920,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_1280_1024P_60HZ,
	.pixel_repeat = 0,
	.x_res = 1280,
	.y_res = 1024,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_1024_768P_60HZ,
	.pixel_repeat = 0,
	.x_res = 1024,
	.y_res = 768,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 60,
    },
    {
	.tv_mode = DISP_TV_MOD_900_540P_60HZ,
	.pixel_repeat = 0,
	.x_res = 900,
	.y_res = 540,
	.b_interlace = 0,
	.trd_mode = 0,
	.fps = 60,
    },
};

static spinlock_t g_rtwb_data_lock;
static struct disp_device *g_rtwb_devices;
static struct disp_device_private_data *g_rtwb_private;

static struct disp_device_private_data *disp_rtwb_get_priv(struct disp_device
							 *p_rtwb)
{
	if (p_rtwb == NULL) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return (struct disp_device_private_data *)p_rtwb->priv_data;
}

static s32 disp_rtwb_irq_handler(u32 sel, u32 irq_flag, void *ptr)
{
	struct disp_device *p_rtwb = (struct disp_device *)ptr;
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if (irq_flag & DISP_AL_CAPTURE_IRQ_FLAG_FRAME_END) {
		if (de_wb_irq_query_and_clear(0, WB_IRQ_STATE_PROC_END |
					      WB_IRQ_STATE_FINISH)) {
			sync_event_proc(p_rtwb->disp, true);
			atomic_set(&p_rtwbp->wati_wb_finish_flag, 1);
			wake_up(&p_rtwbp->wait_wb_finish_queue);
		}
	}
	return DISP_IRQ_RETURN;
}


static s32 rtwb_get_video_timing_info(struct disp_device *p_rtwb, struct disp_video_timings *video_info)
{
	struct disp_rtwb_timings *info;
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);
	int ret = -1;
	int i, list_num;

	if (!p_rtwb || !video_info || !p_rtwbp) {
		DE_WRN("tv init null hdl!\n");
		return DIS_FAIL;
	}

	info = video_timing;
	list_num = sizeof(video_timing)/sizeof(struct disp_rtwb_timings);
	for (i = 0; i < list_num; i++) {
		if (info->tv_mode == p_rtwbp->tv_mode) {
			video_info->tv_mode = info->tv_mode;
			video_info->pixel_repeat = info->pixel_repeat;
			video_info->x_res = info->x_res;
			video_info->y_res = info->y_res;
			video_info->b_interlace = info->b_interlace;
			video_info->trd_mode = info->trd_mode;
			p_rtwbp->video_info = info;
			ret = 0;
			break;
		}
		info++;
	}
	return ret;
}

s32 disp_rtwb_enable(struct disp_device *p_rtwb)
{
	int ret;
	struct disp_manager *mgr = NULL;
	unsigned long flags;
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if (!p_rtwb || !p_rtwbp) {
		DE_WRN(" p_rtwb | p_rtwbp is null\n");
		return DIS_FAIL;
	}

	if ((p_rtwb == NULL) || (p_rtwbp == NULL)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}
	mgr = p_rtwb->manager;
	if (!mgr) {
		DE_WRN("tv%d's mgr is NULL\n", p_rtwb->disp);
		return DIS_FAIL;
	}

	if (p_rtwbp->enabled == 1) {
		DE_WRN("tv%d is already open\n", p_rtwb->disp);
		return DIS_FAIL;
	}

	rtwb_get_video_timing_info(p_rtwb, &p_rtwb->timings);

	ret = reset_control_deassert(p_rtwbp->rst);
	if (ret) {
		DE_WRN("%s(%d): reset_control_deassert for rst failed\n", __func__, __LINE__);
		return ret;
	}

	ret = clk_prepare_enable(p_rtwbp->clk);
	if (ret) {
		DE_WRN("%s(%d): clk_prepare_enable for clk failed\n", __func__, __LINE__);
		return ret;
	}

	ret = clk_prepare_enable(p_rtwbp->clk_bus);
	if (ret) {
		DE_WRN("%s(%d): clk_prepare_enable for clk_bus failed\n", __func__, __LINE__);
		return ret;
	}

	disp_al_capture_init(p_rtwb->disp);
	disp_al_capture_set_irq_enable(p_rtwb->disp,
				       DISP_AL_CAPTURE_IRQ_FLAG_FRAME_END, 1);
	disp_al_capture_set_mode(p_rtwb->disp, SELF_GENERATED_TIMING);

	if (mgr->enable)
		mgr->enable(mgr);

#if defined(HAVE_DEVICE_COMMON_MODULE)
	tcon_de_attach(0xf, mgr->disp);
#endif


	disp_register_irq(p_rtwb->disp + DISP_SCREEN_NUM + DISP_WB_NUM,
			  &p_rtwbp->irq_info);

	spin_lock_irqsave(&g_rtwb_data_lock, flags);
	p_rtwbp->enabled = 1;
	spin_unlock_irqrestore(&g_rtwb_data_lock, flags);

	return 0;
}


s32 disp_rtwb_disable(struct disp_device *p_rtwb)
{
	int ret;
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);
	unsigned long flags;
	struct disp_manager *mgr = NULL;

	if ((p_rtwb == NULL) || (p_rtwbp == NULL)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	mgr = p_rtwb->manager;
	if (!mgr) {
		DE_WRN("tv%d's mgr is NULL\n", p_rtwb->disp);
		return DIS_FAIL;
	}

	if (p_rtwbp->enabled == 0) {
		DE_WRN("tv%d is already closed\n", p_rtwb->disp);
		return DIS_FAIL;
	}

	spin_lock_irqsave(&g_rtwb_data_lock, flags);
	p_rtwbp->enabled = 0;
	spin_unlock_irqrestore(&g_rtwb_data_lock, flags);

	if (mgr->disable)
		mgr->disable(mgr);

	disp_al_capture_set_irq_enable(p_rtwb->disp,
				       DISP_AL_CAPTURE_IRQ_FLAG_FRAME_END, 0);
	disp_al_capture_set_mode(p_rtwb->disp, TIMING_FROM_TCON);
	disp_al_capture_exit(p_rtwb->disp);

	clk_disable_unprepare(p_rtwbp->clk);
	clk_disable_unprepare(p_rtwbp->clk_bus);
	ret = reset_control_assert(p_rtwbp->rst);
	if (ret) {
		DE_WRN("%s(%d): reset_control_assert for rst failed\n", __func__, __LINE__);
		return ret;
	}

	disp_unregister_irq(p_rtwb->disp + DISP_SCREEN_NUM + DISP_WB_NUM,
			    &p_rtwbp->irq_info);
	return 0;
}

s32 disp_rtwb_is_enabled(struct disp_device *p_rtwb)
{
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if ((p_rtwb == NULL) || (p_rtwbp == NULL)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	return p_rtwbp->enabled;

}


static s32 disp_rtwb_init(struct disp_device *p_rtwb)
{
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if (!p_rtwb || !p_rtwbp) {
		DE_WRN("tv init null hdl!\n");
		return DIS_FAIL;
	}

	init_waitqueue_head(&p_rtwbp->wait_wb_finish_queue);
	atomic_set(&p_rtwbp->wati_wb_finish_flag, 0);
	return 0;
}

s32 disp_rtwb_exit(struct disp_device *p_rtwb)
{
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if (!p_rtwb || !p_rtwbp) {
		DE_WRN("tv init null hdl!\n");
		return DIS_FAIL;
	}
	disp_rtwb_disable(p_rtwb);
	kfree(p_rtwb);
	kfree(p_rtwbp);
	return 0;
}


s32 disp_rtwb_set_mode(struct disp_device *p_rtwb, enum disp_tv_mode tv_mode)
{
	s32 ret = 0;
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if ((p_rtwb == NULL) || (p_rtwbp == NULL)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	p_rtwbp->tv_mode = tv_mode;

	return ret;
}

s32 disp_rtwb_get_mode(struct disp_device *p_rtwb)
{

	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if ((p_rtwb == NULL) || (p_rtwbp == NULL)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}


	return p_rtwbp->tv_mode;

}

s32 disp_rtwb_get_input_csc(struct disp_device *p_rtwb)
{
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if ((p_rtwb == NULL) || (p_rtwbp == NULL)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}


	return 1; /*FIXME*/
}


s32 disp_rtwb_check_support_mode(struct disp_device *p_rtwb, enum disp_output_type tv_mode)
{
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);
	u32 i, list_num;
	struct disp_rtwb_timings *info;

	if ((NULL == p_rtwb) || (NULL == p_rtwbp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	info = video_timing;
	list_num = sizeof(video_timing)/sizeof(struct disp_rtwb_timings);
	for (i = 0; i < list_num; i++) {
		if (info->tv_mode == tv_mode)
			return 1;
		info++;
	}
	return 0;
}


static s32 disp_rtwb_get_fps(struct disp_device *p_rtwb)
{
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if (!p_rtwb || !p_rtwbp) {
		DE_WRN("tv set func null  hdl!\n");
		return 0;
	}

	if (p_rtwbp->video_info) {
		return p_rtwbp->video_info->fps;
	} else
		return 0;
}


static disp_config_update_t disp_rtwb_check_config_dirty(struct disp_device *p_rtwb,
					struct disp_device_config *config)
{
	disp_config_update_t ret = DISP_NOT_UPDATE;
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if ((p_rtwb == NULL) || (p_rtwbp == NULL)) {
		DE_WRN("NULL hdl!\n");
		ret = DISP_NOT_UPDATE;
		goto exit;
	}

	if ((p_rtwbp->enabled == 0) ||
	    (config->mode != p_rtwbp->tv_mode))
		ret = DISP_NORMAL_UPDATE;

exit:
	return ret;
}

static s32 disp_rtwb_set_static_config(struct disp_device *p_rtwb,
			       struct disp_device_config *config)
{
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if (!p_rtwb || !p_rtwbp) {
		DE_WRN("Null pointer!\n");
		return -1;
	}

	memcpy(&p_rtwbp->config, config, sizeof(struct disp_device_config));

	p_rtwbp->config.type = p_rtwb->type;
	p_rtwbp->config.mode = config->mode;
	return disp_rtwb_set_mode(p_rtwb, config->mode);
}

static s32 disp_rtwb_get_static_config(struct disp_device *p_rtwb,
			      struct disp_device_config *config)
{
	int ret = 0;
	struct disp_device_private_data *p_rtwbp = disp_rtwb_get_priv(p_rtwb);

	if ((p_rtwb == NULL) || (p_rtwbp == NULL)) {
		DE_WRN("NULL hdl!\n");
		ret = -1;
		goto exit;
	}

	memcpy(config,
	       &p_rtwbp->config,
	       sizeof(struct disp_device_config));
	config->type = p_rtwb->type;
	config->mode = p_rtwbp->tv_mode;

exit:
	return ret;
}



/**
 * @name       :disp_init_rtwb
 * @brief      :register rtwb display device
 * @param[IN]  :para:pointer of hardware resource
 * @return     :0 if success
 */
s32 disp_init_rtwb(struct disp_bsp_init_para *para)
{
	u32 disp = 0;
	struct disp_device *p_rtwb;
	struct disp_device_private_data *p_rtwbp;
	u32 hwdev_index = 0;
	u32 num_devices_support_rtwb = 0;

	spin_lock_init(&g_rtwb_data_lock);


	num_devices_support_rtwb = bsp_disp_feat_get_num_screens();
	g_rtwb_devices = kmalloc(sizeof(struct disp_device)
				* num_devices_support_rtwb,
				GFP_KERNEL | __GFP_ZERO);
	if (g_rtwb_devices == NULL) {
		DE_WRN("malloc memory fail!\n");
		goto malloc_err;
	}

	g_rtwb_private = kmalloc_array(num_devices_support_rtwb, sizeof(*p_rtwbp),
				      GFP_KERNEL | __GFP_ZERO);
	if (g_rtwb_private == NULL) {
		DE_WRN("malloc memory fail!\n");
		goto malloc_err;
	}

	disp = 0;
	for (hwdev_index = 0; hwdev_index < num_devices_support_rtwb; hwdev_index++) {
		p_rtwb = &g_rtwb_devices[disp];
		p_rtwbp = &g_rtwb_private[disp];
		p_rtwb->priv_data = (void *)p_rtwbp;
		p_rtwb->disp = disp;
		p_rtwb->hwdev_index = hwdev_index;
		snprintf(p_rtwb->name, sizeof(p_rtwb->name), "rtwb%d", disp);
		p_rtwb->type = DISP_OUTPUT_TYPE_RTWB;
		p_rtwbp->tv_mode = DISP_TV_MOD_1080P_60HZ;

		p_rtwbp->clk = para->clk_de[hwdev_index];
		p_rtwbp->clk_bus = para->clk_bus_de[hwdev_index];
		p_rtwbp->rst = para->rst_bus_de[hwdev_index];

		p_rtwbp->irq_info.sel = disp;
		p_rtwbp->irq_info.irq_flag =
		    DISP_AL_CAPTURE_IRQ_FLAG_FRAME_END;
		p_rtwbp->irq_info.ptr = (void *)p_rtwb;
		p_rtwbp->irq_info.irq_handler = disp_rtwb_irq_handler;

		p_rtwb->set_manager = disp_device_set_manager;
		p_rtwb->unset_manager = disp_device_unset_manager;
		p_rtwb->get_resolution = disp_device_get_resolution;
		p_rtwb->get_timings = disp_device_get_timings;
		p_rtwb->is_interlace = disp_device_is_interlace;
		p_rtwb->init = disp_rtwb_init;
		p_rtwb->exit = disp_rtwb_exit;
		p_rtwb->enable = disp_rtwb_enable;
		p_rtwb->disable = disp_rtwb_disable;
		p_rtwb->is_enabled = disp_rtwb_is_enabled;
		p_rtwb->set_mode = disp_rtwb_set_mode;
		p_rtwb->get_mode = disp_rtwb_get_mode;
		p_rtwb->set_static_config = disp_rtwb_set_static_config;
		p_rtwb->get_static_config = disp_rtwb_get_static_config;
		p_rtwb->check_config_dirty = disp_rtwb_check_config_dirty;
		p_rtwb->check_support_mode = disp_rtwb_check_support_mode;
		p_rtwb->get_input_csc = disp_rtwb_get_input_csc;
		p_rtwb->get_fps = disp_rtwb_get_fps;
		p_rtwb->init(p_rtwb);

		disp_device_register(p_rtwb);
		disp++;
	}
	return 0;

malloc_err:
	kfree(g_rtwb_devices);
	kfree(g_rtwb_private);
	g_rtwb_devices = NULL;
	g_rtwb_private = NULL;

	return -1;
}

/**
 * @name       :disp_exit_rtwb
 * @brief      :unregister rtwb display device
 * @return     :0 if success
 */
s32 disp_exit_rtwb(void)
{
	u32 num_devices;
	u32 disp = 0;
	struct disp_device *p_rtwb;
	u32 hwdev_index = 0;

	if (!g_rtwb_devices)
		return 0;

	num_devices = bsp_disp_feat_get_num_screens();
	disp = 0;
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {

		p_rtwb = &g_rtwb_devices[disp];
		disp_device_unregister(p_rtwb);

		p_rtwb->exit(p_rtwb);
		disp++;
	}

	kfree(g_rtwb_devices);
	kfree(g_rtwb_private);
	g_rtwb_devices = NULL;
	g_rtwb_private = NULL;

	return 0;
}

/**
 * @name       :disp_rtwb_config
 * @brief      :config rtwb in out param
 * @param[IN]  :mgr:pointer of display manager
 * @param[IN]  :info:pointer of capture info
 * @return     :0 if success
 */
struct dmabuf_item *disp_rtwb_config(struct disp_manager *mgr, struct disp_capture_info2 *info)
{
	struct disp_capture_info_inner info_inner;
	struct dmabuf_item *item = NULL;
	struct disp_device *dispdev = NULL;
	enum disp_csc_type cs = DISP_CSC_TYPE_RGB;
	struct disp_device_private_data *p_rtwbp = NULL;
	struct disp_capture_config config;
	u32 width = 0, height = 0;
	struct fb_address_transfer fb;

	if ((NULL == mgr) || (0 == mgr->is_enabled(mgr))) {
		DE_WRN("manager disable!\n");
		goto exit;
	}
	dispdev = mgr->device;
	if (!dispdev || dispdev->type != DISP_OUTPUT_TYPE_RTWB) {
		DE_WRN("disp device is NULL! or device type is not rtwb\n");
		goto exit;
	}

	p_rtwbp = disp_rtwb_get_priv(dispdev);
	if (!p_rtwbp) {
		DE_WRN("Null p_rtwbp!\n");
		return NULL;
	}

	/*after de enable*/

	atomic_set(&p_rtwbp->wati_wb_finish_flag, 0);

	memset(&info_inner, 0, sizeof(struct disp_s_frame_inner));
	memcpy(&info_inner.window, &info->window, sizeof(struct disp_rect));
	info_inner.out_frame.format = info->out_frame.format;
	memcpy(info_inner.out_frame.size, info->out_frame.size,
	       sizeof(struct disp_rectsz) * 3);
	memcpy(&info_inner.out_frame.crop, &info->out_frame.crop,
	       sizeof(struct disp_rect));
	info_inner.out_frame.fd = info->out_frame.fd;

	item = disp_dma_map(info_inner.out_frame.fd);
	if (item == NULL) {
		DE_WRN("disp dma map fail!\n");
		goto exit;
	}

	memset(&fb, 0, sizeof(struct fb_address_transfer));
	fb.format = info_inner.out_frame.format;
	memcpy(fb.size, info_inner.out_frame.size,
	       sizeof(struct disp_rectsz) * 3);
	fb.dma_addr = item->dma_addr;
	disp_set_fb_info(&fb, true);
	memcpy(info_inner.out_frame.addr, fb.addr, sizeof(long long) * 3);

	memset(&config, 0, sizeof(struct disp_capture_config));

	config.disp = mgr->disp;

	memcpy(&config.out_frame, &info_inner.out_frame,
	       sizeof(struct disp_s_frame_inner));

	memcpy(&config.in_frame.crop, &info_inner.window,
	       sizeof(struct disp_rect));
	if (dispdev->get_input_csc)
		cs = dispdev->get_input_csc(dispdev);
	if (DISP_CSC_TYPE_RGB == cs)
		config.in_frame.format = DISP_FORMAT_ARGB_8888;
	else if (DISP_CSC_TYPE_YUV444 == cs)
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
	if ((0 == config.in_frame.crop.width) ||
	    (0 == config.in_frame.crop.height)) {
		config.in_frame.crop.width = width;
		config.in_frame.crop.height = height;
	}

	DE_INF("disp:%d flag:%d in_fmt:%d in_fd:%d in_crop[%d %d %d "
	       "%d],in_size[%u %u, %u %u, %u "
	       "%u],in_addr[0x%llx,0x%llx,0x%llx],out_fmt:%d out_fd:%d "
	       "out_crop[%d %d %d %d],out_size[%u %u, %u %u, %u "
	       "%u],out_addr[0x%llx,0x%llx,0x%llx]\n",
	       config.disp, config.flags, config.in_frame.format,
	       config.in_frame.fd, config.in_frame.crop.x,
	       config.in_frame.crop.y, config.in_frame.crop.width,
	       config.in_frame.crop.height, config.in_frame.size[0].width,
	       config.in_frame.size[0].height, config.in_frame.size[1].width,
	       config.in_frame.size[1].height, config.in_frame.size[2].width,
	       config.in_frame.size[2].height, config.in_frame.addr[0],
	       config.in_frame.addr[1], config.in_frame.addr[2],
	       config.out_frame.format, config.out_frame.fd,
	       config.out_frame.crop.x, config.out_frame.crop.y,
	       config.out_frame.crop.width, config.out_frame.crop.height,
	       config.out_frame.size[0].width, config.out_frame.size[0].height,
	       config.out_frame.size[1].width, config.out_frame.size[1].height,
	       config.out_frame.size[2].width, config.out_frame.size[2].height,
	       config.out_frame.addr[0], config.out_frame.addr[1],
	       config.out_frame.addr[2]);

	if (disp_feat_is_using_wb_rcq(mgr->disp)) {
		disp_al_capture_set_all_rcq_head_dirty(mgr->disp, 0);
		disp_al_capture_set_rcq_update(mgr->disp, 0);
	}
	disp_al_capture_apply(mgr->disp, &config);
	if (disp_feat_is_using_wb_rcq(mgr->disp))
		disp_al_capture_set_rcq_update(mgr->disp, 1);
	else
		disp_al_capture_sync(mgr->disp);

exit:
	return item;
}

s32 disp_rtwb_wait_finish(struct disp_manager *mgr)
{
	struct disp_device *dispdev = NULL;
	struct disp_device_private_data *p_rtwbp = NULL;
	int ret = 0;

	if ((NULL == mgr) || (0 == mgr->is_enabled(mgr))) {
		DE_WRN("manager disable!\n");
		return -1;
	}

	dispdev = mgr->device;

	if (!dispdev ||  dispdev->type != DISP_OUTPUT_TYPE_RTWB) {
		DE_WRN("disp device is NULL! or device type is not rtwb\n");
		return -1;
	}
	p_rtwbp = disp_rtwb_get_priv(dispdev);
	if (!p_rtwbp) {
		DE_WRN("Null p_rtwbp!\n");
		return -1;
	}

	de_top_start_rtwb(dispdev->disp, 1);

	ret = wait_event_timeout(
				 p_rtwbp->wait_wb_finish_queue,
				 atomic_read(&p_rtwbp->wati_wb_finish_flag) == 1,
				 msecs_to_jiffies(100));
	if (ret <= 0) {
		DE_WRN("rtwb pending timout!\n");
		atomic_set(&p_rtwbp->wati_wb_finish_flag, 1);
		wake_up(&p_rtwbp->wait_wb_finish_queue);
		ret = -1;
	} else
		ret = 0;


	de_top_start_rtwb(dispdev->disp, 0);
	return ret;
}

#endif

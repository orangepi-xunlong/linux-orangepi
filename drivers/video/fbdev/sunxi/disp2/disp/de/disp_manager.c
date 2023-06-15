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
#include "disp_manager.h"
#include "disp_display.h"
#include "../disp_trace.h"
#include "disp_rtwb.h"

#define MAX_LAYERS 16
#define FORCE_SYNC_THRESHOLD 4

struct disp_manager_private_data {
	bool applied;
	bool enabled;
	bool unmap_dmabuf;
	bool setting;
	bool color_range_modified;
	struct disp_manager_data *cfg;

	 s32 (*shadow_protect)(u32 disp, bool protect);

	u32 reg_base;
	u32 irq_no;
	struct clk *clk;
	struct clk *clk_bus;
	struct clk *clk_parent;
	struct clk *clk_extra;
	struct clk *clk_dpss;

	struct reset_control *rst;
#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct reset_control *rst_extra;
#endif
	struct reset_control *rst_dpss;

	unsigned int layers_using;
	bool sync;
	bool force_sync;
	unsigned int nosync_cnt;
	unsigned int force_sync_cnt;
	bool err;
	unsigned int err_cnt;
	unsigned int dmabuf_unmap_skip_cnt;
	unsigned int dmabuf_unmap_skip_cnt_max;
	struct list_head dmabuf_list;
	unsigned int dmabuf_cnt;
	unsigned int dmabuf_cnt_max;

	struct disp_irq_info irq_info;
	wait_queue_head_t wait_rcq_finish_queue;
	atomic_t wati_rcq_finish_flag;
	bool iommu_en_flag;
	unsigned int iommu_master_id;
};

static spinlock_t mgr_data_lock;
static struct mutex mgr_mlock;

static struct disp_manager *mgrs;
static struct disp_manager_private_data *mgr_private;
static struct disp_manager_data *mgr_cfgs;

static struct disp_layer_config_data *lyr_cfgs;

static int rcq_init_finished;

/*
 * layer unit
 */
struct disp_layer_private_data {
	struct disp_layer_config_data *cfg;
	 s32 (*shadow_protect)(u32 sel, bool protect);
};

static struct disp_layer *lyrs;
static struct disp_layer_private_data *lyr_private;
struct disp_layer *disp_get_layer(u32 disp, u32 chn, u32 layer_id)
{
	u32 num_screens, max_num_layers = 0;
	struct disp_layer *lyr = lyrs;
	int i;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp >= num_screens) {
		DE_WRN("disp %d is out of range %d\n", disp, num_screens);
		return NULL;
	}

	for (i = 0; i < num_screens; i++)
		max_num_layers += bsp_disp_feat_get_num_layers(i);

	for (i = 0; i < max_num_layers; i++) {
		if ((lyr->disp == disp) && (lyr->chn == chn)
		    && (lyr->id == layer_id)) {
			DE_INF("%d,%d,%d, name=%s\n", disp, chn, layer_id,
			       lyr->name);
			return lyr;
		}
		lyr++;
	}

	DE_WRN("%s (%d,%d,%d) fail\n", __func__, disp, chn, layer_id);
	return NULL;

}

struct disp_layer *disp_get_layer_1(u32 disp, u32 layer_id)
{
	u32 num_screens, num_layers;
	u32 i, k;
	u32 layer_index = 0, start_index = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp >= num_screens) {
		DE_WRN("disp %d is out of range %d\n", disp, num_screens);
		return NULL;
	}

	for (i = 0; i < disp; i++)
		start_index += bsp_disp_feat_get_num_layers(i);

	layer_id += start_index;

	for (i = 0; i < num_screens; i++) {
		num_layers = bsp_disp_feat_get_num_layers(i);
		for (k = 0; k < num_layers; k++) {
			if (layer_index == layer_id) {
				DE_INF("disp%d layer%d: %d,%d,%d\n",
				       disp, layer_id,
				       lyrs[layer_index].disp,
				       lyrs[layer_index].chn,
				       lyrs[layer_index].id);
				return &lyrs[layer_index];
			}
			layer_index++;
		}
	}

	DE_WRN("%s fail\n", __func__);
	return NULL;
}

static struct disp_layer_private_data *disp_lyr_get_priv(struct disp_layer *lyr)
{
	if (lyr == NULL) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return (struct disp_layer_private_data *)lyr->data;
}


/** __disp_config_transfer2inner - transfer disp_layer_config to inner one
 */
s32 __disp_config_transfer2inner(
	struct disp_layer_config_inner *config_inner,
	struct disp_layer_config *config)
{
	config_inner->enable = config->enable;
	config_inner->channel = config->channel;
	config_inner->layer_id = config->layer_id;

	if (0 == config->enable) {
		memset(&(config->info), 0, sizeof(config->info));
	}

	/* layer info */
	config_inner->info.mode = config->info.mode;
	config_inner->info.zorder = config->info.zorder;
	config_inner->info.alpha_mode = config->info.alpha_mode;
	config_inner->info.alpha_value = config->info.alpha_value;
	memcpy(&config_inner->info.screen_win,
	       &config->info.screen_win,
	       sizeof(struct disp_rect));
	config_inner->info.b_trd_out = config->info.b_trd_out;
	config_inner->info.out_trd_mode = config->info.out_trd_mode;
	config_inner->info.id = config->info.id;
	/* fb info */
	memcpy(config_inner->info.fb.addr,
	       config->info.fb.addr,
	       sizeof(long long) * 3);
	memcpy(config_inner->info.fb.size,
	       config->info.fb.size,
	       sizeof(struct disp_rectsz) * 3);
	memcpy(config_inner->info.fb.align,
	    config->info.fb.align, sizeof(int) * 3);
	config_inner->info.fb.format = config->info.fb.format;
	config_inner->info.fb.color_space = config->info.fb.color_space;
	memcpy(config_inner->info.fb.trd_right_addr,
	       config->info.fb.trd_right_addr,
	       sizeof(int) * 3);
	config_inner->info.fb.pre_multiply = config->info.fb.pre_multiply;
	memcpy(&config_inner->info.fb.crop,
	       &config->info.fb.crop,
	       sizeof(struct disp_rect64));
	config_inner->info.fb.flags = config->info.fb.flags;
	config_inner->info.fb.scan = config->info.fb.scan;
	config_inner->info.fb.eotf = DISP_EOTF_UNDEF;
	config_inner->info.fb.fbd_en = 0;
	config_inner->info.fb.lbc_en = config->info.fb.lbc_en;
	memcpy(&config_inner->info.fb.lbc_info,
	       &config->info.fb.lbc_info,
	       sizeof(struct disp_lbc_info));
	config_inner->info.fb.metadata_buf = 0;
	config_inner->info.fb.fd = -911;
	config_inner->info.fb.metadata_size = 0;
	config_inner->info.fb.metadata_flag = 0;
	config_inner->info.atw.used = 0;

	if (config_inner->info.mode == LAYER_MODE_COLOR)
		config_inner->info.color = config->info.color;

	return 0;
}


/** __disp_config2_transfer2inner -transfer disp_layer_config2 to inner one
 */
s32 __disp_config2_transfer2inner(struct disp_layer_config_inner *config_inner,
				      struct disp_layer_config2 *config2)
{
	config_inner->enable = config2->enable;
	config_inner->channel = config2->channel;
	config_inner->layer_id = config2->layer_id;

	if (0 == config2->enable) {
		memset(&(config2->info), 0, sizeof(config2->info));
	}

	/* layer info */
	config_inner->info.mode = config2->info.mode;
	config_inner->info.zorder = config2->info.zorder;
	config_inner->info.alpha_mode = config2->info.alpha_mode;
	config_inner->info.alpha_value = config2->info.alpha_value;
	memcpy(&config_inner->info.screen_win,
	       &config2->info.screen_win,
	       sizeof(struct disp_rect));
	config_inner->info.b_trd_out = config2->info.b_trd_out;
	config_inner->info.out_trd_mode = config2->info.out_trd_mode;
	/* fb info */
	config_inner->info.fb.fd = config2->info.fb.fd;
	memcpy(config_inner->info.fb.size,
	       config2->info.fb.size,
	       sizeof(struct disp_rectsz) * 3);
	memcpy(config_inner->info.fb.align,
	    config2->info.fb.align, sizeof(int) * 3);
	config_inner->info.fb.format = config2->info.fb.format;
	config_inner->info.fb.color_space = config2->info.fb.color_space;
	config_inner->info.fb.trd_right_fd = config2->info.fb.trd_right_fd;
	config_inner->info.fb.pre_multiply = config2->info.fb.pre_multiply;
	memcpy(&config_inner->info.fb.crop,
	       &config2->info.fb.crop,
	       sizeof(struct disp_rect64));
	config_inner->info.fb.flags = config2->info.fb.flags;
	config_inner->info.fb.scan = config2->info.fb.scan;
	config_inner->info.fb.depth = config2->info.fb.depth;
	/* hdr related */
	config_inner->info.fb.eotf = config2->info.fb.eotf;
	config_inner->info.fb.fbd_en = config2->info.fb.fbd_en;
	config_inner->info.fb.lbc_en = config2->info.fb.lbc_en;
	memcpy(&config_inner->info.fb.lbc_info,
	       &config2->info.fb.lbc_info,
	       sizeof(struct disp_lbc_info));
	config_inner->info.fb.metadata_fd = config2->info.fb.metadata_fd;
	config_inner->info.fb.metadata_size = config2->info.fb.metadata_size;
	config_inner->info.fb.metadata_flag =
	    config2->info.fb.metadata_flag;

	config_inner->info.id = config2->info.id;
	/* atw related */
	config_inner->info.atw.used = config2->info.atw.used;
	config_inner->info.atw.mode = config2->info.atw.mode;
	config_inner->info.atw.b_row = config2->info.atw.b_row;
	config_inner->info.atw.b_col = config2->info.atw.b_col;
	config_inner->info.atw.cof_fd = config2->info.atw.cof_fd;
	if (config_inner->info.mode == LAYER_MODE_COLOR)
		config_inner->info.color = config2->info.color;


#if defined(DE_VERSION_V33X)
	//TODO:move transform memory to atw
	config_inner->info.transform = config2->info.transform;
	memcpy(&config_inner->info.snr, &config2->info.snr,
	       sizeof(struct disp_snr_info));

#endif
	return 0;
}
EXPORT_SYMBOL(__disp_config2_transfer2inner);

/** __disp_inner_transfer2config - transfer inner to disp_layer_config
 */
s32 __disp_inner_transfer2config(struct disp_layer_config *config,
				 struct disp_layer_config_inner *config_inner)
{
	config->enable = config_inner->enable;
	config->channel = config_inner->channel;
	config->layer_id = config_inner->layer_id;
	/* layer info */
	config->info.mode = config_inner->info.mode;
	config->info.zorder = config_inner->info.zorder;
	config->info.alpha_mode = config_inner->info.alpha_mode;
	config->info.alpha_value = config_inner->info.alpha_value;
	memcpy(&config->info.screen_win,
	       &config_inner->info.screen_win,
	       sizeof(struct disp_rect));
	config->info.b_trd_out = config_inner->info.b_trd_out;
	config->info.out_trd_mode = config_inner->info.out_trd_mode;
	config->info.id = config_inner->info.id;
	/* fb info */
	memcpy(config->info.fb.addr,
	       config_inner->info.fb.addr,
	       sizeof(long long) * 3);
	memcpy(config->info.fb.size,
	       config_inner->info.fb.size,
	       sizeof(struct disp_rectsz) * 3);
	memcpy(config->info.fb.align,
	    config_inner->info.fb.align, sizeof(int) * 3);
	config->info.fb.format = config_inner->info.fb.format;
	config->info.fb.color_space = config_inner->info.fb.color_space;
	memcpy(config->info.fb.trd_right_addr,
	       config_inner->info.fb.trd_right_addr,
	       sizeof(int) * 3);
	config->info.fb.pre_multiply = config_inner->info.fb.pre_multiply;
	memcpy(&config->info.fb.crop,
	       &config_inner->info.fb.crop,
	       sizeof(struct disp_rect64));
	config->info.fb.flags = config_inner->info.fb.flags;
	config->info.fb.scan = config_inner->info.fb.scan;

	config->info.fb.lbc_en = config_inner->info.fb.lbc_en;
	memcpy(&config->info.fb.lbc_info,
	       &config_inner->info.fb.lbc_info,
	       sizeof(struct disp_lbc_info));

	if (config->info.mode == LAYER_MODE_COLOR)
		config->info.color = config_inner->info.color;

	return 0;
}


/** __disp_inner_transfer2config2 - transfer inner to disp_layer_config2
 */
s32 __disp_inner_transfer2config2(struct disp_layer_config2 *config2,
				  struct disp_layer_config_inner *config_inner)
{
	config2->enable = config_inner->enable;
	config2->channel = config_inner->channel;
	config2->layer_id = config_inner->layer_id;
	/* layer info */
	config2->info.mode = config_inner->info.mode;
	config2->info.zorder = config_inner->info.zorder;
	config2->info.alpha_mode = config_inner->info.alpha_mode;
	config2->info.alpha_value = config_inner->info.alpha_value;
	memcpy(&config2->info.screen_win,
	       &config_inner->info.screen_win,
	       sizeof(struct disp_rect));
	config2->info.b_trd_out = config_inner->info.b_trd_out;
	config2->info.out_trd_mode = config_inner->info.out_trd_mode;
	/* fb info */
	config2->info.fb.fd = config_inner->info.fb.fd;
	memcpy(config2->info.fb.size,
	       config_inner->info.fb.size,
	       sizeof(struct disp_rectsz) * 3);
	memcpy(config2->info.fb.align,
	    config_inner->info.fb.align, sizeof(int) * 3);
	config2->info.fb.format = config_inner->info.fb.format;
	config2->info.fb.color_space = config_inner->info.fb.color_space;
	config2->info.fb.trd_right_fd = config_inner->info.fb.trd_right_fd;
	config2->info.fb.pre_multiply = config_inner->info.fb.pre_multiply;
	memcpy(&config2->info.fb.crop,
	       &config_inner->info.fb.crop,
	       sizeof(struct disp_rect64));
	config2->info.fb.flags = config_inner->info.fb.flags;
	config2->info.fb.scan = config_inner->info.fb.scan;
	config2->info.fb.depth = config_inner->info.fb.depth;
	/* hdr related */
	config2->info.fb.eotf = config_inner->info.fb.eotf;
	config2->info.fb.fbd_en = config_inner->info.fb.fbd_en;

	config2->info.fb.lbc_en = config_inner->info.fb.lbc_en;
	memcpy(&config2->info.fb.lbc_info,
	       &config_inner->info.fb.lbc_info,
	       sizeof(struct disp_lbc_info));

	config2->info.fb.metadata_fd = config_inner->info.fb.metadata_fd;
	config2->info.fb.metadata_size = config_inner->info.fb.metadata_size;
	config2->info.fb.metadata_flag =
	    config_inner->info.fb.metadata_flag;

	config2->info.id = config_inner->info.id;
	/* atw related */
	config2->info.atw.used = config_inner->info.atw.used;
	config2->info.atw.mode = config_inner->info.atw.mode;
	config2->info.atw.b_row = config_inner->info.atw.b_row;
	config2->info.atw.b_col = config_inner->info.atw.b_col;
	config2->info.atw.cof_fd = config_inner->info.atw.cof_fd;
	if (config2->info.mode == LAYER_MODE_COLOR)
		config2->info.color = config_inner->info.color;

#if defined(DE_VERSION_V33X)
	config2->info.transform = config_inner->info.transform;
	memcpy(&config2->info.snr, &config_inner->info.snr,
	       sizeof(struct disp_snr_info));
#endif

	return 0;
}


static s32
disp_lyr_set_manager(struct disp_layer *lyr, struct disp_manager *mgr)
{
	unsigned long flags;
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL) || (mgr == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	lyr->manager = mgr;
	list_add_tail(&lyr->list, &mgr->lyr_list);
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	return DIS_SUCCESS;
}

static s32 disp_lyr_unset_manager(struct disp_layer *lyr)
{
	unsigned long flags;
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	lyr->manager = NULL;
	list_del(&lyr->list);
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	return DIS_SUCCESS;
}

static s32
disp_lyr_check(struct disp_layer *lyr, struct disp_layer_config *config)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return DIS_SUCCESS;
}

static s32 disp_lyr_check2(struct disp_layer *lyr,
			   struct disp_layer_config2 *config)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return DIS_SUCCESS;
}

static s32
disp_lyr_save_and_dirty_check(struct disp_layer *lyr,
			      struct disp_layer_config *config)
{
	unsigned long flags;
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	if (lyrp->cfg) {
		struct disp_layer_config_inner *pre_config = &lyrp->cfg->config;

		if ((pre_config->enable != config->enable) ||
		    (pre_config->info.fb.addr[0] != config->info.fb.addr[0])
		    || (pre_config->info.fb.format != config->info.fb.format)
		    || (pre_config->info.fb.flags != config->info.fb.flags))
			lyrp->cfg->flag |= LAYER_ATTR_DIRTY;
		if ((pre_config->info.fb.size[0].width !=
		     config->info.fb.size[0].width)
		    || (pre_config->info.fb.size[0].height !=
			config->info.fb.size[0].height)
		    || (pre_config->info.fb.crop.width !=
			config->info.fb.crop.width)
		    || (pre_config->info.fb.crop.height !=
			config->info.fb.crop.height)
		    || (pre_config->info.screen_win.width !=
			config->info.screen_win.width)
		    || (pre_config->info.screen_win.height !=
			config->info.screen_win.height))
			lyrp->cfg->flag |= LAYER_SIZE_DIRTY;
		lyrp->cfg->flag = LAYER_ALL_DIRTY;

		if ((pre_config->enable == config->enable) &&
			(config->enable == 0))
			lyrp->cfg->flag = 0;
	__disp_config_transfer2inner(&lyrp->cfg->config, config);
	} else {
		DE_INF("cfg is NULL\n");
	}
	spin_unlock_irqrestore(&mgr_data_lock, flags);
	DE_INF
	    ("layer:ch%d, layer%d, format=%d, size=<%d,%d>, crop=<%lld,%lld,%lld,%lld>,frame=<%d,%d>, en=%d addr[0x%llx,0x%llx,0x%llx>\n",
	     config->channel, config->layer_id, config->info.fb.format,
	     config->info.fb.size[0].width, config->info.fb.size[0].height,
	     config->info.fb.crop.x >> 32, config->info.fb.crop.y >> 32,
	     config->info.fb.crop.width >> 32,
	     config->info.fb.crop.height >> 32, config->info.screen_win.width,
	     config->info.screen_win.height, config->enable,
	     config->info.fb.addr[0], config->info.fb.addr[1],
	     config->info.fb.addr[2]);

	return DIS_SUCCESS;
}

static s32 disp_lyr_save_and_dirty_check2(struct disp_layer *lyr,
					 struct disp_layer_config2 *config)
{
	unsigned long flags;
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	if (lyrp->cfg) {
		struct disp_layer_config_inner *pre_cfg = &lyrp->cfg->config;
		struct disp_layer_info_inner *pre_info = &pre_cfg->info;
		struct disp_layer_info2 *info = &config->info;
		struct disp_fb_info_inner *pre_fb = &pre_info->fb;
		struct disp_fb_info2 *fb = &info->fb;

		if ((pre_cfg->enable != config->enable) ||
		    (pre_fb->fd != fb->fd) ||
		    (pre_fb->format != fb->format) ||
		    (pre_fb->flags != fb->flags))
			lyrp->cfg->flag |= LAYER_ATTR_DIRTY;
		if ((pre_fb->size[0].width != fb->size[0].width) ||
		    (pre_fb->size[0].height != info->fb.size[0].height) ||
		    (pre_fb->crop.width != info->fb.crop.width) ||
		    (pre_fb->crop.height != info->fb.crop.height) ||
		    (pre_info->screen_win.width != info->screen_win.width) ||
		    (pre_info->screen_win.height != info->screen_win.height))
			lyrp->cfg->flag |= LAYER_SIZE_DIRTY;

		lyrp->cfg->flag = LAYER_ALL_DIRTY;

		if ((pre_cfg->enable == config->enable) &&
			(config->enable == 0))
			lyrp->cfg->flag = 0;
		__disp_config2_transfer2inner(&lyrp->cfg->config,
						      config);
	} else {
		DE_INF("cfg is NULL\n");
	}
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	return DIS_SUCCESS;
}


static s32 disp_lyr_is_dirty(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (lyrp->cfg)
		return (lyrp->cfg->flag & LAYER_ALL_DIRTY);

	return 0;
}

static s32 disp_lyr_dirty_clear(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	lyrp->cfg->flag = 0;

	return 0;
}

static s32
disp_lyr_get_config(struct disp_layer *lyr, struct disp_layer_config *config)
{
	unsigned long flags;
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	if (lyrp->cfg) {
		/* memcpy(config, &lyrp->cfg->config, */
		/* sizeof(struct disp_layer_config)); */
	__disp_inner_transfer2config(config, &lyrp->cfg->config);
	}
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	return DIS_SUCCESS;
}

static s32 disp_lyr_get_config2(struct disp_layer *lyr,
				struct disp_layer_config2 *config)
{
	unsigned long flags;
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	if (lyrp->cfg)
		__disp_inner_transfer2config2(config, &lyrp->cfg->config);
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	return DIS_SUCCESS;
}

static s32 disp_lyr_apply(struct disp_layer *lyr)
{
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return DIS_SUCCESS;
}

static s32 disp_lyr_force_apply(struct disp_layer *lyr)
{
	unsigned long flags;
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	lyrp->cfg->flag |= LAYER_ALL_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);
	disp_lyr_apply(lyr);

	return DIS_SUCCESS;
}

static s32 disp_lyr_dump(struct disp_layer *lyr, char *buf)
{
	unsigned long flags;
	struct disp_layer_config_data data;
	struct disp_layer_private_data *lyrp = disp_lyr_get_priv(lyr);
	u32 count = 0;

	if ((lyr == NULL) || (lyrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	memcpy(&data, lyrp->cfg, sizeof(struct disp_layer_config_data));
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	count += sprintf(buf + count, " %5s ",
			 (data.config.info.mode == LAYER_MODE_BUFFER) ?
			 "BUF" : "COLOR");
	count += sprintf(buf + count, " %8s ",
			 (data.config.enable == 1) ? "enable" : "disable");
	count += sprintf(buf + count, "ch[%1u] ", data.config.channel);
	count += sprintf(buf + count, "lyr[%1u] ", data.config.layer_id);
	count += sprintf(buf + count, "z[%1u] ", data.config.info.zorder);
	count += sprintf(buf + count, "prem[%1s] ",
			 (data.config.info.fb.pre_multiply) ? "Y" : "N");
	count += sprintf(buf + count, "a[%5s %3u] ",
			 (data.config.info.alpha_mode) ? "globl" : "pixel",
			 data.config.info.alpha_value);
	count += sprintf(buf + count, "fmt[%3d] ", data.config.info.fb.format);
	count += sprintf(buf + count, "fb[%4u,%4u;%4u,%4u;%4u,%4u] ",
			 data.config.info.fb.size[0].width,
			 data.config.info.fb.size[0].height,
			 data.config.info.fb.size[1].width,
			 data.config.info.fb.size[1].height,
			 data.config.info.fb.size[2].width,
			 data.config.info.fb.size[2].height);
	count += sprintf(buf + count, "align[%4u,%4u,%4u] ",
				      data.config.info.fb.align[0],
				      data.config.info.fb.align[1],
				      data.config.info.fb.align[2]);

	count += sprintf(buf + count, "crop[%4u,%4u,%4u,%4u] ",
			 (unsigned int)(data.config.info.fb.crop.x >> 32),
			 (unsigned int)(data.config.info.fb.crop.y >> 32),
			 (unsigned int)(data.config.info.fb.crop.width >> 32),
			 (unsigned int)(data.config.info.fb.crop.height >> 32));
	count += sprintf(buf + count, "frame[%4d,%4d,%4u,%4u] ",
			 data.config.info.screen_win.x,
			 data.config.info.screen_win.y,
			 data.config.info.screen_win.width,
			 data.config.info.screen_win.height);
	count += sprintf(buf + count, "addr[%8llx,%8llx,%8llx] ",
			 data.config.info.fb.addr[0],
			 data.config.info.fb.addr[1],
			 data.config.info.fb.addr[2]);
	count += sprintf(buf + count, "flags[0x%8x] trd[%1d,%1d]\n",
			 data.config.info.fb.flags, data.config.info.b_trd_out,
			 data.config.info.out_trd_mode);
	count += sprintf(buf + count, "depth[%2d] ", data.config.info.fb.depth);
#if defined(DE_VERSION_V33X)
	count += sprintf(buf + count, "transf[%d]\n", data.config.info.transform);
#endif

	return count;
}

static s32 disp_init_lyr(struct disp_bsp_init_para *para)
{
	u32 num_screens = 0, num_channels = 0, num_layers = 0;
	u32 max_num_layers = 0;
	u32 disp, chn, layer_id, layer_index = 0;

	DE_INF("disp_init_lyr\n");

	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++)
		max_num_layers += bsp_disp_feat_get_num_layers(disp);

	lyrs = kmalloc_array(max_num_layers, sizeof(struct disp_layer),
			     GFP_KERNEL | __GFP_ZERO);
	if (lyrs == NULL) {
		DE_WRN("malloc memory fail! size=0x%x\n",
		       (unsigned int)sizeof(struct disp_layer) *
		       max_num_layers);
		return DIS_FAIL;
	}

	lyr_private = (struct disp_layer_private_data *)
	    kmalloc(sizeof(struct disp_layer_private_data) *
		    max_num_layers, GFP_KERNEL | __GFP_ZERO);
	if (lyr_private == NULL) {
		DE_WRN("malloc memory fail! size=0x%x\n",
		       (unsigned int)sizeof(struct disp_layer_private_data)
		       * max_num_layers);
		return DIS_FAIL;
	}

	lyr_cfgs = (struct disp_layer_config_data *)
	    kmalloc(sizeof(struct disp_layer_config_data) *
		    max_num_layers, GFP_KERNEL | __GFP_ZERO);
	if (lyr_cfgs == NULL) {
		DE_WRN("malloc memory fail! size=0x%x\n",
		       (unsigned int)sizeof(struct disp_layer_private_data)
		       * max_num_layers);
		return DIS_FAIL;
	}

	for (disp = 0; disp < num_screens; disp++) {
		num_channels = bsp_disp_feat_get_num_channels(disp);
		for (chn = 0; chn < num_channels; chn++) {
			num_layers =
			    bsp_disp_feat_get_num_layers_by_chn(disp, chn);
			for (layer_id = 0; layer_id < num_layers;
			     layer_id++, layer_index++) {
				struct disp_layer *lyr = &lyrs[layer_index];
				struct disp_layer_config_data *lyr_cfg =
				    &lyr_cfgs[layer_index];
				struct disp_layer_private_data *lyrp =
				    &lyr_private[layer_index];

				lyrp->shadow_protect = para->shadow_protect;
				lyrp->cfg = lyr_cfg;

				lyr_cfg->ops.vmap = disp_vmap;
				lyr_cfg->ops.vunmap = disp_vunmap;
				sprintf(lyr->name, "mgr%d chn%d lyr%d",
					disp, chn, layer_id);
				lyr->disp = disp;
				lyr->chn = chn;
				lyr->id = layer_id;
				lyr->data = (void *)lyrp;

				lyr->set_manager = disp_lyr_set_manager;
				lyr->unset_manager = disp_lyr_unset_manager;
				lyr->apply = disp_lyr_apply;
				lyr->force_apply = disp_lyr_force_apply;
				lyr->check = disp_lyr_check;
				lyr->check2 = disp_lyr_check2;
				lyr->save_and_dirty_check =
				    disp_lyr_save_and_dirty_check;
				lyr->save_and_dirty_check2 =
				    disp_lyr_save_and_dirty_check2;
				lyr->get_config = disp_lyr_get_config;
				lyr->get_config2 = disp_lyr_get_config2;
				lyr->dump = disp_lyr_dump;
				lyr->is_dirty = disp_lyr_is_dirty;
				lyr->dirty_clear = disp_lyr_dirty_clear;
			}
		}
	}

	return 0;
}

static s32 disp_exit_lyr(void)
{
	kfree(lyr_cfgs);
	kfree(lyr_private);
	kfree(lyrs);

	return 0;
}

struct disp_manager *disp_get_layer_manager(u32 disp)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp >= num_screens) {
		DE_WRN("disp %d out of range\n", disp);
		return NULL;
	}

	return &mgrs[disp];
}
EXPORT_SYMBOL(disp_get_layer_manager);

int disp_get_num_screens(void)
{
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	return num_screens;
}
EXPORT_SYMBOL(disp_get_num_screens);
static struct disp_manager_private_data *disp_mgr_get_priv(struct disp_manager
							   *mgr)
{
	if (mgr == NULL) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return &mgr_private[mgr->disp];
}

static struct disp_layer_config_data *disp_mgr_get_layer_cfg_head(struct
								  disp_manager
								  *mgr)
{
	int layer_index = 0, disp;
	int num_screens = bsp_disp_feat_get_num_screens();

	for (disp = 0; disp < num_screens && disp < mgr->disp; disp++)
		layer_index += bsp_disp_feat_get_num_layers(disp);

	return &lyr_cfgs[layer_index];

}
static struct disp_layer_config_data *
disp_mgr_get_layer_cfg(struct disp_manager *mgr,
		       struct disp_layer_config2 *config)
{
	int layer_index = 0, disp;
	int num_screens = bsp_disp_feat_get_num_screens();

	for (disp = 0; disp < num_screens && disp < mgr->disp; disp++)
		layer_index += bsp_disp_feat_get_num_layers(disp);

	layer_index += config->channel * bsp_disp_feat_get_num_layers_by_chn(
					     mgr->disp, config->channel) +
		       config->layer_id;

	return &lyr_cfgs[layer_index];
}

static s32 disp_mgr_protect_reg_for_rcq(
	struct disp_manager *mgr, bool protect)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	long ret = 0;

	if ((mgr == NULL) || (mgrp == NULL)) {
		__wrn("NULL hdl!\n");
		return -1;
	}

	if (!rcq_init_finished) {
		return -1;
	}

	if (protect) {
		if (atomic_read(&mgrp->wati_rcq_finish_flag) == 1) {
			ret = wait_event_timeout(
						 mgrp->wait_rcq_finish_queue,
						 atomic_read(&mgrp->wati_rcq_finish_flag) == 2,
						 msecs_to_jiffies(1000/mgrp->cfg->config.device_fps + 1));
			if (ret <= 0) {
				atomic_set(&mgrp->wati_rcq_finish_flag, 2);
				disp_al_manager_set_rcq_update(mgr->disp, 0);
				wake_up(&mgrp->wait_rcq_finish_queue);
				gdisp.screen[mgr->disp].health_info.skip_cnt++;
			} else
				disp_al_manager_set_all_rcq_head_dirty(mgr->disp, 0);
		}
	} else {
		DISP_TRACE_BEGIN("set_rcq_update");
		disp_al_manager_set_rcq_update(mgr->disp, 1);
		/*don't wait rcq finish if output type is rtwb*/
		if (mgr->device && mgr->device->type == DISP_OUTPUT_TYPE_RTWB)
			atomic_set(&mgrp->wati_rcq_finish_flag, 0);
		else
			atomic_set(&mgrp->wati_rcq_finish_flag, 1);
		DISP_TRACE_END("set_rcq_update");
	}

	return 0;
}

static s32 disp_mgr_shadow_protect(struct disp_manager *mgr, bool protect)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (mgrp->shadow_protect)
		return mgrp->shadow_protect(mgr->disp, protect);

	return -1;
}

static s32 disp_mgr_rcq_finish_irq_handler(
	struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	u32 irq_state = 0;
	unsigned long flags;

	if ((mgr == NULL) || (mgrp == NULL)) {
		__wrn("NULL hdl!\n");
		return -1;
	}

	irq_state = disp_al_manager_query_irq_state(mgr->disp,
		DISP_AL_IRQ_STATE_RCQ_ACCEPT
		| DISP_AL_IRQ_STATE_RCQ_FINISH);

	if (irq_state & DISP_AL_IRQ_STATE_RCQ_FINISH) {
		if (mgr->device && mgr->device->get_status)
			if (mgr->device->get_status(mgr->device) != 0)
				gdisp.screen[mgr->disp].health_info.error_cnt++;
		disp_al_manager_set_all_rcq_head_dirty(mgr->disp, 0);
		spin_lock_irqsave(&mgr_data_lock, flags);
		/*rcq update ok, we know exactly what address is using, so just unmap dmabuf*/
		mgrp->unmap_dmabuf = true;
		spin_unlock_irqrestore(&mgr_data_lock, flags);
		atomic_set(&mgrp->wati_rcq_finish_flag, 2);
		wake_up(&mgrp->wait_rcq_finish_queue);
	}
	DISP_TRACE_INT_F("rcq_irq_state", irq_state);

	return 0;
}

s32 disp_mgr_irq_handler(u32 disp, u32 irq_flag, void *ptr)
{
	if (irq_flag & DISP_AL_IRQ_FLAG_RCQ_FINISH)
		disp_mgr_rcq_finish_irq_handler((struct disp_manager *)ptr);

	return 0;
}

static s32 disp_mgr_clk_init(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	mgrp->clk_parent = clk_get_parent(mgrp->clk);
	mgrp->cfg->config.de_freq = clk_get_rate(mgrp->clk);

#if defined(DE_VERSION_V2X)
	disp_al_update_de_clk_rate(mgrp->cfg->config.de_freq);
#endif

	return 0;
}

static s32 disp_mgr_clk_exit(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return 0;
}

static s32 disp_mgr_clk_enable(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	int ret = 0;
	unsigned long de_freq = 0;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (mgr->get_clk_rate && mgrp->clk) {
		DE_INF("set DE rate to %u\n", mgr->get_clk_rate(mgr));
		de_freq = mgr->get_clk_rate(mgr);
		clk_set_rate(mgrp->clk, de_freq);
		if (de_freq != clk_get_rate(mgrp->clk)) {
			if (mgrp->clk_parent)
				clk_set_rate(mgrp->clk_parent, de_freq);
			clk_set_rate(mgrp->clk, de_freq);
			if (de_freq != clk_get_rate(mgrp->clk)) {
				DE_WRN("Set DE clk fail\n");
				return -1;
			}
		}
	}

	clk_set_parent(mgrp->clk, mgrp->clk_parent);

	DE_INF("mgr %d clk enable\n", mgr->disp);

	if (mgrp->rst_extra) {
		ret = reset_control_deassert(mgrp->rst_extra);
		if (ret) {
			DE_WRN("%s(%d): reset_control_deassert for rst_display top failed, ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}
	}

	if (mgrp->clk_extra) {
		ret = clk_prepare_enable(mgrp->clk_extra);
		if (ret != 0)
			DE_WRN("fail enable mgr's clk_extra!\n");
	}

	ret = reset_control_deassert(mgrp->rst);
	if (ret) {
		DE_WRN("%s(%d): reset_control_deassert for rst failed\n", __func__, __LINE__);
		return ret;
	}

	ret = clk_prepare_enable(mgrp->clk);
	if (ret) {
		DE_WRN("%s(%d): clk_prepare_enable for clk failed\n", __func__, __LINE__);
		return ret;
	}

	ret = clk_prepare_enable(mgrp->clk_bus);
	if (ret) {
		DE_WRN("%s(%d): clk_prepare_enable for clk_bus failed\n", __func__, __LINE__);
		return ret;
	}

	if (mgrp->rst_dpss) {
		ret = reset_control_deassert(mgrp->rst_dpss);
		if (ret) {
			DE_WRN("%s(%d): reset_control_deassert for rst_dpss failed, ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}
	}
	if (mgrp->clk_dpss) {
		ret = clk_prepare_enable(mgrp->clk_dpss);
		if (ret) {
			DE_WRN("%s(%d): clk_prepare_enable for clk_dpss failed, ret=%d\n", __func__, __LINE__, ret);
			return ret;
		}
	}

	return ret;
}

static s32 disp_mgr_clk_disable(struct disp_manager *mgr)
{
	int ret;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (mgrp->clk_extra)
		clk_disable_unprepare(mgrp->clk_extra);
	if (mgrp->rst_extra)
		reset_control_assert(mgrp->rst_extra);

	clk_disable_unprepare(mgrp->clk);
	clk_disable_unprepare(mgrp->clk_bus);

	ret = reset_control_assert(mgrp->rst);
	if (ret) {
		DE_WRN("%s(%d): reset_control_assert for rst failed\n", __func__, __LINE__);
		return ret;
	}

	if (mgrp->clk_dpss)
		clk_disable(mgrp->clk_dpss);
	if (mgrp->rst_dpss)
		reset_control_assert(mgrp->rst_dpss);

	return 0;
}

/* Return: unit(hz) */
static s32 disp_mgr_get_clk_rate(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

#if defined(CONFIG_ARCH_SUN8IW16)
	if (mgr->device && mgr->device->type != DISP_OUTPUT_TYPE_HDMI)
		mgrp->cfg->config.de_freq = 216000000;
	else
		mgrp->cfg->config.de_freq = 432000000;
#endif

	return mgrp->cfg->config.de_freq;
}

static s32 disp_mgr_init(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	disp_mgr_clk_init(mgr);
	return 0;
}

static s32 disp_mgr_exit(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	/* FIXME, disable manager */
	disp_mgr_clk_exit(mgr);

	return 0;
}

static s32 disp_mgr_set_palette(struct disp_manager *mgr, struct disp_palette_config *config)
{
	unsigned long flags;
	unsigned int num_chns = 0;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	num_chns = bsp_disp_feat_get_num_channels(mgr->disp);
	if ((config == NULL) || (num_chns == 0) || (config->channel > num_chns)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	spin_lock_irqsave(&mgr_data_lock, flags);
	memcpy(&mgrp->cfg->config.palette, config,
	    sizeof(struct disp_palette_config));
	mgrp->cfg->flag |= MANAGER_PALETTE_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	mgr->apply(mgr);

	return DIS_SUCCESS;
}

static s32
disp_mgr_set_back_color(struct disp_manager *mgr, struct disp_color *back_color)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	memcpy(&mgrp->cfg->config.back_color, back_color,
	       sizeof(struct disp_color));
	mgrp->cfg->flag |= MANAGER_BACK_COLOR_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	mgr->apply(mgr);

	return DIS_SUCCESS;
}

static s32
disp_mgr_get_back_color(struct disp_manager *mgr, struct disp_color *back_color)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	memcpy(back_color, &mgrp->cfg->config.back_color,
	       sizeof(struct disp_color));
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	return DIS_SUCCESS;
}

static s32
disp_mgr_set_color_key(struct disp_manager *mgr, struct disp_colorkey *ck)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	memcpy(&mgrp->cfg->config.ck, ck, sizeof(struct disp_colorkey));
	mgrp->cfg->flag |= MANAGER_CK_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	mgr->apply(mgr);

	return DIS_SUCCESS;
}

static s32
disp_mgr_get_color_key(struct disp_manager *mgr, struct disp_colorkey *ck)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	memcpy(ck, &mgrp->cfg->config.ck, sizeof(struct disp_colorkey));
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	return DIS_SUCCESS;
}

static s32
disp_mgr_set_output_color_range(struct disp_manager *mgr, u32 color_range)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->cfg->config.color_range = color_range;
	mgrp->cfg->flag |= MANAGER_COLOR_RANGE_DIRTY;
	mgrp->color_range_modified = true;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	mgr->apply(mgr);

	return DIS_SUCCESS;
}

static s32 disp_mgr_get_output_color_range(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	return mgrp->cfg->config.color_range;
}

static s32 disp_mgr_update_color_space(struct disp_manager *mgr)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	unsigned int cs = 0, color_range = 0;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (mgr->device) {
		if (mgr->device->get_input_csc)
			cs = mgr->device->get_input_csc(mgr->device);
		if (mgr->device && mgr->device->get_input_color_range)
			color_range =
			    mgr->device->get_input_color_range(mgr->device);
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->cfg->config.cs = cs;
	if (!mgrp->color_range_modified)
		mgrp->cfg->config.color_range = color_range;
	mgrp->cfg->flag |= MANAGER_COLOR_SPACE_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	mgr->apply(mgr);

	return DIS_SUCCESS;
}

static s32 disp_mgr_smooth_switch(struct disp_manager *mgr)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	struct disp_device_config dev_config;

	if ((NULL == mgr) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	if (mgr->device) {

		if (mgr->device->get_static_config)
			mgr->device->get_static_config(mgr->device,
								&dev_config);
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->cfg->config.color_space = dev_config.cs;
	mgrp->cfg->config.eotf = dev_config.eotf;
	mgrp->cfg->config.hdr_type = dev_config.hdr_type;
	mgrp->cfg->flag |= MANAGER_COLOR_SPACE_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	mgr->apply(mgr);

	return DIS_SUCCESS;
}

static int _force_layer_en;
static int _force_layer2;
static struct disp_layer_config backup_layer[2][16];
static struct disp_layer_config2 backup_layer2[2][16];
struct dmabuf_item back_dma_item[2][4];

static int backup_layer_num;

static void layer_mask_init(unsigned int *mask, unsigned int total)
{
	*mask = (0x00000001 << total) - 1;
}
static void layer_mask_clear(unsigned int *mask, unsigned int channel, unsigned int id)
{
	unsigned int bit = (0x00000001 << id) << (channel * 4);

	(*mask) = (*mask) & (~bit);
}
static int layer_mask_test(unsigned int *mask, unsigned int channel, unsigned int id)
{
	unsigned int bit = (0x00000001 << id) << (channel * 4);

	return (*mask) & bit;
}

static s32 disp_mgr_force_set_layer_config(struct disp_manager *mgr, struct disp_layer_config *config, unsigned int layer_num)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	unsigned int num_layers = 0, layer_index = 0;
	struct disp_layer *lyr = NULL;
	unsigned int mask = 0;
	struct disp_layer_config dummy;
	struct disp_layer *pre_lyr = NULL;
	int channel, id;
	int layers_cnt = 0;
	int channels_cnt = 0;

	if ((NULL == mgr) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	mutex_lock(&mgr_mlock);
	_force_layer_en = 1;

	num_layers = bsp_disp_feat_get_num_layers(mgr->disp);
	if ((config == NULL) || (layer_num == 0) || (layer_num > num_layers)) {
		DE_WRN("NULL hdl!\n");
		mutex_unlock(&mgr_mlock);
		return -1;
	}

	layer_mask_init(&mask, num_layers);
	for (layer_index = 0; layer_index < layer_num; layer_index++) {

		lyr = disp_get_layer(mgr->disp, config->channel, config->layer_id);
		if (lyr == NULL)
			continue;
		if (!lyr->check(lyr, config)) {
			lyr->save_and_dirty_check(lyr, config);
		}
		layer_mask_clear(&mask, config->channel, config->layer_id);
		config++;
	}

	channels_cnt = bsp_disp_feat_get_num_channels(mgr->disp);
	memset(&dummy, 0, sizeof(dummy));
	for (channel = 0; channel < channels_cnt; channel++) {
		layers_cnt = bsp_disp_feat_get_num_layers_by_chn(mgr->disp, channel);
		for (id = 0; id < layers_cnt; id++) {
			if (layer_mask_test(&mask, channel, id) == 0)
				continue;

			layer_mask_clear(&mask, channel, id);
			pre_lyr = disp_get_layer(mgr->disp, channel, id);
			if (pre_lyr == NULL)
				continue;

			dummy.channel = channel;
			dummy.layer_id = id;
			if (!pre_lyr->check(pre_lyr, &dummy))
				pre_lyr->save_and_dirty_check(pre_lyr, &dummy);

		}
	}

	if (mgr->apply)
		mgr->apply(mgr);

	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		lyr->dirty_clear(lyr);
	}

	mutex_unlock(&mgr_mlock);
	return DIS_SUCCESS;
}

s32 disp_mgr_set_layer_config2_restore(struct disp_manager *mgr,
			  struct disp_layer_config2 *config,
			  unsigned int layer_num);

static s32 disp_mgr_force_set_layer_config_exit(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	unsigned int num_layers = 16, layer_index = 0;
	unsigned int layer_num;
	struct disp_layer *lyr = NULL;
	struct disp_layer_config *backup;
	struct disp_layer_config2 *backup2;
	struct disp_layer_config dummy;
	int channel = 0;
	int id = 0;
	int layers_cnt = 0;
	int channels_cnt;

	if ((NULL == mgr) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	num_layers = bsp_disp_feat_get_num_layers(mgr->disp);

	mutex_lock(&mgr_mlock);
	layer_num = backup_layer_num;
	backup = &(backup_layer[mgr->disp][0]);
	backup2 = &(backup_layer2[mgr->disp][0]);

	if (layer_num <= 0 || layer_num > num_layers) {
		DE_WRN("invalid input params!\n");
		mutex_unlock(&mgr_mlock);
		return -1;
	}
	if (_force_layer2) {
		disp_mgr_set_layer_config2_restore(mgr, backup2, layer_num);
		_force_layer_en = 0;
		mutex_unlock(&mgr_mlock);
		return DIS_SUCCESS;
	}

	/* disable all layer first */
	channels_cnt = bsp_disp_feat_get_num_channels(mgr->disp);

	memset(&dummy, 0, sizeof(dummy));
	for (channel = 0; channel < channels_cnt; channel++) {
		layers_cnt = bsp_disp_feat_get_num_layers_by_chn(mgr->disp, channel);
		for (id = 0; id < layers_cnt; id++) {

			lyr = disp_get_layer(mgr->disp, channel, id);
			if (lyr == NULL)
				continue;

			dummy.channel = channel;
			dummy.layer_id = id;
			if (!lyr->check(lyr, &dummy)) {
				lyr->save_and_dirty_check(lyr, &dummy);
			}
		}
	}

	for (layer_index = 0; layer_index < layer_num; layer_index++) {

		lyr = disp_get_layer(mgr->disp, backup->channel, backup->layer_id);
		if (lyr == NULL)
			continue;
		if (!lyr->check(lyr, backup))
			lyr->save_and_dirty_check(lyr, backup);

		backup++;
	}

	if (mgr->apply)
		mgr->apply(mgr);

	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		lyr->dirty_clear(lyr);
	}
	_force_layer_en = 0;
	mutex_unlock(&mgr_mlock);

	return DIS_SUCCESS;
}

static s32
disp_mgr_set_layer_config(struct disp_manager *mgr,
			  struct disp_layer_config *config,
			  unsigned int layer_num)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	unsigned int num_layers = 0, layer_index = 0;
	struct disp_layer *lyr = NULL;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("mgr%d, config %d layers\n", mgr->disp, layer_num);

	num_layers = bsp_disp_feat_get_num_layers(mgr->disp);
	if ((config == NULL) || (layer_num == 0) || (layer_num > num_layers)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF
	    ("layer:ch%d, layer%d, format=%d, size=<%d,%d>, crop=<%lld,%lld,%lld,%lld>,frame=<%d,%d>, en=%d addr[0x%llx,0x%llx,0x%llx> alpha=<%d,%d>\n",
	     config->channel, config->layer_id, config->info.fb.format,
	     config->info.fb.size[0].width, config->info.fb.size[0].height,
	     config->info.fb.crop.x >> 32, config->info.fb.crop.y >> 32,
	     config->info.fb.crop.width >> 32,
	     config->info.fb.crop.height >> 32, config->info.screen_win.width,
	     config->info.screen_win.height, config->enable,
	     config->info.fb.addr[0], config->info.fb.addr[1],
	     config->info.fb.addr[2], config->info.alpha_mode,
	     config->info.alpha_value);

	mutex_lock(&mgr_mlock);
	{
		struct disp_layer_config *src = config;
		struct disp_layer_config *backup = &(backup_layer[mgr->disp][0]);
		backup_layer_num = layer_num;
		memset(backup, 0, sizeof(struct disp_layer_config)*16);
		for (layer_index = 0; layer_index < layer_num; layer_index++) {
			memcpy(backup, src, sizeof(struct disp_layer_config));
			backup++;
			src++;
		}
		if (_force_layer_en) {
			_force_layer2 = 0;
			mutex_unlock(&mgr_mlock);
			return DIS_SUCCESS;
		}
	}

	for (layer_index = 0; layer_index < layer_num; layer_index++) {
		struct disp_layer *lyr = NULL;

		lyr = disp_get_layer(mgr->disp, config->channel,
				     config->layer_id);
		if (lyr == NULL)
			continue;
		if (!lyr->check(lyr, config))
			lyr->save_and_dirty_check(lyr, config);
		config++;
	}

	if (mgr->apply)
		mgr->apply(mgr);

	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		lyr->dirty_clear(lyr);
	}
	mutex_unlock(&mgr_mlock);

	return DIS_SUCCESS;
}

static void disp_mgr_dmabuf_list_add(struct dmabuf_item *item,
			struct disp_manager_private_data *mgrp,
			unsigned long long ref)
{
	item->id = ref;
	list_add_tail(&item->list, &mgrp->dmabuf_list);
	mgrp->dmabuf_cnt++;
}

s32 disp_mgr_set_layer_config2_restore(struct disp_manager *mgr,
			  struct disp_layer_config2 *config,
			  unsigned int layer_num)
{
		unsigned int num_layers = 0, i = 0;
		struct disp_layer *lyr = NULL;
		struct disp_layer_config_data *lyr_cfg;
		struct dmabuf_item *item;
		struct disp_layer_config2 *config1 = config;
		struct fb_address_transfer fb;
		struct disp_device_dynamic_config dconf;


		num_layers = bsp_disp_feat_get_num_layers(mgr->disp);

		memset(&dconf, 0, sizeof(struct disp_device_dynamic_config));

		for (i = 0; i < layer_num; i++) {
			struct disp_layer *lyr = NULL;

			lyr = disp_get_layer(mgr->disp, config1->channel,
					     config1->layer_id);

			if (lyr) {
				lyr->save_and_dirty_check2(lyr, config1);
				if (lyr->is_dirty(lyr) &&
				   (config1->info.fb.metadata_flag & 0x3)) {
					dconf.metadata_fd =
							config1->info.fb.metadata_fd;
					dconf.metadata_size =
							config1->info.fb.metadata_size;
					dconf.metadata_flag =
							config1->info.fb.metadata_flag;
				}
			}

			config1++;
		}

		lyr_cfg = disp_mgr_get_layer_cfg_head(mgr);
		for (i = 0; i < layer_num; i++, lyr_cfg++) {

			if (lyr_cfg->config.enable == 0)
				continue;

			if (lyr_cfg->config.info.mode == LAYER_MODE_COLOR)
				continue;

			item = &back_dma_item[mgr->disp][0];

			fb.format = lyr_cfg->config.info.fb.format;
			memcpy(fb.size, lyr_cfg->config.info.fb.size,
		       sizeof(struct disp_rectsz) * 3);
			memcpy(fb.align, lyr_cfg->config.info.fb.align,
			       sizeof(int) * 3);
			fb.depth = lyr_cfg->config.info.fb.depth;
			fb.dma_addr = item->dma_addr;
			disp_set_fb_info(&fb, true);
			memcpy(lyr_cfg->config.info.fb.addr,
			       fb.addr,
			       sizeof(long long) * 3);

			lyr_cfg->config.info.fb.trd_right_addr[0] =
					(unsigned int)fb.trd_right_addr[0];
			lyr_cfg->config.info.fb.trd_right_addr[1] =
					(unsigned int)fb.trd_right_addr[1];
			lyr_cfg->config.info.fb.trd_right_addr[2] =
					(unsigned int)fb.trd_right_addr[2];

			/* get dma_buf for right image buffer */
			if (lyr_cfg->config.info.fb.flags == DISP_BF_STEREO_FP) {
				item = &back_dma_item[mgr->disp][1];
				fb.dma_addr = item->dma_addr;
				disp_set_fb_info(&fb, false);
				lyr_cfg->config.info.fb.trd_right_addr[0] =
						(unsigned int)fb.trd_right_addr[0];
				lyr_cfg->config.info.fb.trd_right_addr[1] =
						(unsigned int)fb.trd_right_addr[1];
				lyr_cfg->config.info.fb.trd_right_addr[2] =
						(unsigned int)fb.trd_right_addr[2];
			}

			/* process 2d plus depth stereo mode */
			if (lyr_cfg->config.info.fb.flags == DISP_BF_STEREO_2D_DEPTH)  {
				lyr_cfg->config.info.fb.flags = DISP_BF_STEREO_FP;
				/* process depth, only support rgb format */
				if ((lyr_cfg->config.info.fb.depth != 0) &&
				    (lyr_cfg->config.info.fb.format
				      < DISP_FORMAT_YUV444_I_AYUV)) {
					int depth = lyr_cfg->config.info.fb.depth;
					unsigned long long abs_depth =
						(depth > 0) ? depth : (-depth);

					memcpy(fb.addr,
					       lyr_cfg->config.info.fb.addr,
					       sizeof(long long) * 3);
					fb.trd_right_addr[0] =
					    lyr_cfg->config.info.fb.trd_right_addr[0];
					fb.trd_right_addr[1] =
					    lyr_cfg->config.info.fb.trd_right_addr[1];
					fb.trd_right_addr[2] =
				    lyr_cfg->config.info.fb.trd_right_addr[2];
					if (disp_set_fb_base_on_depth(&fb) == 0) {
						memcpy(lyr_cfg->config.info.fb.addr,
						       fb.addr,
						       sizeof(long long) * 3);
			    lyr_cfg->config.info.fb.trd_right_addr[0] =
				(unsigned int)fb.trd_right_addr[0];
			    lyr_cfg->config.info.fb.trd_right_addr[1] =
				(unsigned int)fb.trd_right_addr[1];
			    lyr_cfg->config.info.fb.trd_right_addr[2] =
			(unsigned int)fb.trd_right_addr[2];

						lyr_cfg->config.info.fb.crop.width -=
						    (abs_depth << 32);
				}
				}

			}

			/* get dma_buf for atw coef buffer */
			if (!lyr_cfg->config.info.atw.used)
				continue;

			item = &back_dma_item[mgr->disp][2];
			lyr_cfg->config.info.atw.cof_addr = item->dma_addr;


		}

		if (mgr->apply)
			mgr->apply(mgr);

		list_for_each_entry(lyr, &mgr->lyr_list, list) {
				lyr->dirty_clear(lyr);
		}
		return 0;
}


static s32 disp_mgr_is_address_using(u32 layer_num, struct dmabuf_item *item,
				     struct disp_layer_address *lyr_addr)
{
	u32 i = 0;

	if (!item || !lyr_addr || layer_num <= 0)
		return 0;

	for (i = 0; i < layer_num; ++i) {
		if (lyr_addr[i].lyr_id.disp == item->lyr_id.disp &&
		    lyr_addr[i].lyr_id.channel == item->lyr_id.channel &&
		    lyr_addr[i].lyr_id.layer_id == item->lyr_id.layer_id) {
			if (item->lyr_id.type & 0x1)
				if (lyr_addr[i].dma_addr ==
				    item->dma_addr)
					return 1;
			if (item->lyr_id.type & 0x2)
				if (lyr_addr[i].trd_addr ==
				    item->dma_addr)
					return 1;
			if (item->lyr_id.type & 0x4)
				if (lyr_addr[i].atw_addr ==
				    item->dma_addr)
					return 1;
		}
	}

	return 0;
}

static s32 disp_unmap_afbc_header(struct disp_fb_info_inner *fb)
{
	if ((fb->p_afbc_header == NULL)
		|| (fb->p_metadata == NULL)) {
		DE_WRN("null buf: %p, %p\n",
			fb->p_afbc_header, fb->p_metadata);
		return -1;
	}
	if (IS_ERR(fb->metadata_dmabuf)) {
		DE_WRN("bad metadata_dmabuf(%p)\n", fb->metadata_dmabuf);
		return -1;
	}

	DE_INF("\n");
	dma_buf_kunmap(fb->metadata_dmabuf, 0, fb->p_metadata);
	dma_buf_end_cpu_access(fb->metadata_dmabuf, DMA_FROM_DEVICE);
	dma_buf_put(fb->metadata_dmabuf);

	fb->p_afbc_header = NULL;
	fb->p_metadata = NULL;
	fb->metadata_dmabuf = NULL;

	return 0;
}

static void disp_unmap_dmabuf(struct disp_manager *mgr, struct disp_manager_private_data *mgrp, unsigned int num_layers)
{
	struct disp_layer_config_data *lyr_cfg;
	struct disp_layer_address *lyr_addr = NULL;
	struct dmabuf_item *item, *tmp;
	int i;
	lyr_addr = kmalloc_array(num_layers, sizeof(struct disp_manager),
			     GFP_KERNEL | __GFP_ZERO);

	lyr_cfg = disp_mgr_get_layer_cfg_head(mgr);
	for (i = 0; i < num_layers; ++i, ++lyr_cfg) {

		if (lyr_cfg->config.enable == 0)
			continue;
		/*color mode and set_layer_config do no need to dma map*/
		if (lyr_cfg->config.info.mode == LAYER_MODE_COLOR ||
		    lyr_cfg->config.info.fb.fd == -911)
			continue;
		lyr_addr[i].lyr_id.disp = mgr->disp;
		lyr_addr[i].lyr_id.channel = lyr_cfg->config.channel;
		lyr_addr[i].lyr_id.layer_id = lyr_cfg->config.layer_id;
		lyr_addr[i].dma_addr = lyr_cfg->config.info.fb.addr[0];
		lyr_addr[i].lyr_id.type |= 0x1;
		if (lyr_cfg->config.info.fb.flags == DISP_BF_STEREO_FP) {
			lyr_addr[i].trd_addr =
			    lyr_cfg->config.info.fb.trd_right_addr[0];
			lyr_addr[i].lyr_id.type |= 0x2;
		}
		/* get dma_buf for atw coef buffer */
		if (!lyr_cfg->config.info.atw.used)
			continue;
		lyr_addr[i].atw_addr = lyr_cfg->config.info.atw.cof_addr;
		lyr_addr[i].lyr_id.type |= 0x4;
	}

	list_for_each_entry_safe(item, tmp, &mgrp->dmabuf_list, list) {
		if (!disp_mgr_is_address_using(num_layers, item, lyr_addr)) {
			list_del(&item->list);
			disp_dma_unmap(item);
			mgrp->dmabuf_cnt--;
		}
	}
	kfree(lyr_addr);
}

static s32 disp_map_afbc_header(struct disp_fb_info_inner *fb)
{
	int ret;
	if ((fb->metadata_fd < 0)
		|| !(fb->metadata_flag & SUNXI_METADATA_FLAG_AFBC_HEADER
		|| (fb->metadata_flag & SUNXI_METADATA_FLAG_HDR10P_METADATA))
		|| (fb->metadata_size == 0)) {
		DE_WRN("invalid value\n");
		return -1;
	}

	fb->metadata_dmabuf = dma_buf_get(fb->metadata_fd);

	if (IS_ERR(fb->metadata_dmabuf)) {
		DE_WRN("dma_buf_get, fd(%d)\n", fb->metadata_fd);
		return -1;
	}

	ret = dma_buf_begin_cpu_access(fb->metadata_dmabuf, DMA_FROM_DEVICE);
	if (ret) {
		dma_buf_put(fb->metadata_dmabuf);
		fb->metadata_dmabuf = NULL;
		DE_WRN("dma_buf_begin_cpu_access failed\n");
		return -1;
	}
	fb->p_metadata = dma_buf_kmap(fb->metadata_dmabuf, 0);
	if (!fb->p_metadata) {
		dma_buf_end_cpu_access(fb->metadata_dmabuf, DMA_FROM_DEVICE);
		dma_buf_put(fb->metadata_dmabuf);
		fb->metadata_dmabuf = NULL;
		DE_WRN("dma_buf_kmap failed\n");
		return -1;
	}

	fb->p_afbc_header = &(fb->p_metadata->afbc_head);

	return 0;
}

s32 disp_mgr_set_layer_config2(struct disp_manager *mgr,
			  struct disp_layer_config2 *config,
			  unsigned int layer_num)
{
	struct disp_manager_private_data *mgrp;
	unsigned int num_layers = 0, i = 0;
	struct disp_layer *lyr = NULL;
	struct disp_layer_config_data *lyr_cfg_using[MAX_LAYERS];
	struct disp_layer_config_data *lyr_cfg;
	struct dmabuf_item *item;
	struct disp_layer_config2 *config1 = config;
	unsigned long long ref = 0;
	struct fb_address_transfer fb;
	bool pre_force_sync;
	struct disp_device_dynamic_config dconf;
	struct disp_device *dispdev = NULL;
	unsigned int map_err_cnt = 0;
	unsigned long flags;
	bool unmap = false;
	bool send_vsif = false;

	if (mgr == NULL) {
		DE_WRN("NULL hdl!\n");
		goto err;
	}
	mgrp = disp_mgr_get_priv(mgr);
	if (mgrp == NULL) {
		DE_WRN("NULL hdl!\n");
		goto err;
	}
	pre_force_sync = mgrp->force_sync;

	DE_INF("mgr%d, config %d layers\n", mgr->disp, layer_num);

	num_layers = bsp_disp_feat_get_num_layers(mgr->disp);
	if ((config == NULL) || (layer_num == 0) || (layer_num > num_layers)) {
		DE_WRN("NULL hdl!\n");
		goto err;
	}

	memset(&dconf, 0, sizeof(struct disp_device_dynamic_config));
	mutex_lock(&mgr_mlock);

	{
		unsigned int layer_index = 0;
		struct disp_layer_config2 *src = config;
		struct disp_layer_config2 *backup = &(backup_layer2[mgr->disp][0]);
		backup_layer_num = layer_num;
		memset(backup, 0, sizeof(struct disp_layer_config2)*16);
		for (layer_index = 0; layer_index < layer_num; layer_index++) {
			memcpy(backup, src, sizeof(struct disp_layer_config2));
			backup++;
			src++;
		}
		_force_layer2 = 1;
	}

	spin_lock_irqsave(&mgr_data_lock, flags);
	if (mgrp->unmap_dmabuf) {
		mgrp->unmap_dmabuf = false;
		unmap = true;
	}
	/*if vysnc comes before finish modifying lyr_cfgs, we should not unmap after vsync, or we will get uncorrect using address*/
	mgrp->setting = true;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	if (unmap)
		disp_unmap_dmabuf(mgr, mgrp, num_layers);


	for (i = 0; i < layer_num; i++) {
		struct disp_layer *lyr = NULL;

		lyr = disp_get_layer(mgr->disp, config1->channel,
				     config1->layer_id);

		if (lyr) {
			lyr->save_and_dirty_check2(lyr, config1);
			if (lyr->is_dirty(lyr) &&
			   (config1->info.fb.metadata_flag & 0x3)) {
				dconf.metadata_fd =
						config1->info.fb.metadata_fd;
				dconf.metadata_size =
						config1->info.fb.metadata_size;
				dconf.metadata_flag =
						config1->info.fb.metadata_flag;
			}
		}
		lyr_cfg_using[i] = disp_mgr_get_layer_cfg(mgr, config1);

		config1++;
	}

	ref = gdisp.screen[mgr->disp].health_info.irq_cnt;

	for (i = 0, lyr_cfg = lyr_cfg_using[0]; i < layer_num; i++, lyr_cfg++) {

		if (lyr_cfg->config.enable == 0)
			continue;
		/*color mode and set_layer_config do no need to dma map*/
		if (lyr_cfg->config.info.mode == LAYER_MODE_COLOR)
			continue;

		item = disp_dma_map(lyr_cfg->config.info.fb.fd);
		if (item == NULL) {
			pr_info("disp dma map fail!\n");
			lyr_cfg->config.enable = 0;
			map_err_cnt++;
			continue;
		}
		memcpy(&back_dma_item[mgr->disp][0], item, sizeof(struct dmabuf_item));
		item->lyr_id.disp = mgr->disp;
		item->lyr_id.channel = lyr_cfg->config.channel;
		item->lyr_id.layer_id = lyr_cfg->config.layer_id;
		item->lyr_id.type |= 0x1;
		fb.format = lyr_cfg->config.info.fb.format;
		memcpy(fb.size, lyr_cfg->config.info.fb.size,
		       sizeof(struct disp_rectsz) * 3);
		memcpy(fb.align, lyr_cfg->config.info.fb.align,
		       sizeof(int) * 3);
		fb.depth = lyr_cfg->config.info.fb.depth;
		fb.dma_addr = item->dma_addr;
		disp_set_fb_info(&fb, true);
		memcpy(lyr_cfg->config.info.fb.addr,
		       fb.addr,
		       sizeof(long long) * 3);

		lyr_cfg->config.info.fb.trd_right_addr[0] =
				(unsigned int)fb.trd_right_addr[0];
		lyr_cfg->config.info.fb.trd_right_addr[1] =
				(unsigned int)fb.trd_right_addr[1];
		lyr_cfg->config.info.fb.trd_right_addr[2] =
				(unsigned int)fb.trd_right_addr[2];
		disp_mgr_dmabuf_list_add(item, mgrp, ref);

		if (lyr_cfg->config.info.fb.fbd_en
			|| (lyr_cfg->config.info.fb.metadata_flag
			& SUNXI_METADATA_FLAG_HDR10P_METADATA)) {
			disp_map_afbc_header(&(lyr_cfg->config.info.fb));
			if ((lyr_cfg->config.info.fb.metadata_flag
				& SUNXI_METADATA_FLAG_HDR10P_METADATA)
				&& mgr->device && mgr->device->set_vsif_config
				&& mgrp->cfg->config.hdr_type == HDR10P) {
				mgr->device->set_vsif_config(mgr->device,
				(void *) lyr_cfg->config.info.fb.p_metadata,
				&dconf);
				send_vsif = true;
			}
		} else if (mgr->device && mgr->device->set_vsif_config
			&& send_vsif == false) {
			mgr->device->set_vsif_config(mgr->device, NULL, &dconf);
		}

		/* get dma_buf for right image buffer */
		if (lyr_cfg->config.info.fb.flags == DISP_BF_STEREO_FP) {
			item = disp_dma_map(lyr_cfg->config.info.atw.cof_fd);
			if (item == NULL) {
				DE_WRN("disp dma map for right buffer fail!\n");
				lyr_cfg->config.info.fb.flags = DISP_BF_NORMAL;
				continue;
			}
			memcpy(&back_dma_item[mgr->disp][1], item, sizeof(struct dmabuf_item));
			item->lyr_id.disp = mgr->disp;
			item->lyr_id.channel = lyr_cfg->config.channel;
			item->lyr_id.layer_id = lyr_cfg->config.layer_id;
			item->lyr_id.type |= 0x2;
			fb.dma_addr = item->dma_addr;
			disp_set_fb_info(&fb, false);
			lyr_cfg->config.info.fb.trd_right_addr[0] =
					(unsigned int)fb.trd_right_addr[0];
			lyr_cfg->config.info.fb.trd_right_addr[1] =
					(unsigned int)fb.trd_right_addr[1];
			lyr_cfg->config.info.fb.trd_right_addr[2] =
					(unsigned int)fb.trd_right_addr[2];
			disp_mgr_dmabuf_list_add(item, mgrp, ref);
		}

		/* process 2d plus depth stereo mode */
		if (lyr_cfg->config.info.fb.flags == DISP_BF_STEREO_2D_DEPTH)  {
			lyr_cfg->config.info.fb.flags = DISP_BF_STEREO_FP;
			/* process depth, only support rgb format */
			if ((lyr_cfg->config.info.fb.depth != 0) &&
			    (lyr_cfg->config.info.fb.format
			      < DISP_FORMAT_YUV444_I_AYUV)) {
				int depth = lyr_cfg->config.info.fb.depth;
				unsigned long long abs_depth =
					(depth > 0) ? depth : (-depth);

				memcpy(fb.addr,
				       lyr_cfg->config.info.fb.addr,
				       sizeof(long long) * 3);
				fb.trd_right_addr[0] =
				    lyr_cfg->config.info.fb.trd_right_addr[0];
				fb.trd_right_addr[1] =
				    lyr_cfg->config.info.fb.trd_right_addr[1];
				fb.trd_right_addr[2] =
				    lyr_cfg->config.info.fb.trd_right_addr[2];
				if (disp_set_fb_base_on_depth(&fb) == 0) {
					memcpy(lyr_cfg->config.info.fb.addr,
					       fb.addr,
					       sizeof(long long) * 3);
		    lyr_cfg->config.info.fb.trd_right_addr[0] =
			(unsigned int)fb.trd_right_addr[0];
		    lyr_cfg->config.info.fb.trd_right_addr[1] =
			(unsigned int)fb.trd_right_addr[1];
		    lyr_cfg->config.info.fb.trd_right_addr[2] =
			(unsigned int)fb.trd_right_addr[2];

					lyr_cfg->config.info.fb.crop.width -=
					    (abs_depth << 32);
				}
			}

		}

		/* get dma_buf for atw coef buffer */
		if (!lyr_cfg->config.info.atw.used)
			continue;

		item = disp_dma_map(lyr_cfg->config.info.atw.cof_fd);
		if (item == NULL) {
			DE_WRN("disp dma map for atw coef fail!\n");
			lyr_cfg->config.info.atw.used = 0;
			continue;
		}
		memcpy(&back_dma_item[mgr->disp][2], item, sizeof(struct dmabuf_item));

		item->lyr_id.disp = mgr->disp;
		item->lyr_id.channel = lyr_cfg->config.channel;
		item->lyr_id.layer_id = lyr_cfg->config.layer_id;
		item->lyr_id.type |= 0x4;

		lyr_cfg->config.info.atw.cof_addr = item->dma_addr;
		disp_mgr_dmabuf_list_add(item, mgrp, ref);


	}

	mgrp->dmabuf_cnt_max = (mgrp->dmabuf_cnt > mgrp->dmabuf_cnt_max) ?
			mgrp->dmabuf_cnt : mgrp->dmabuf_cnt_max;
	if (!_force_layer_en) {
		if (mgr->apply)
			mgr->apply(mgr);

		lyr_cfg = disp_mgr_get_layer_cfg_head(mgr);
		for (i = 0; i < num_layers; i++, lyr_cfg++)
			if (lyr_cfg->config.info.fb.fbd_en
				|| (lyr_cfg->config.info.fb.metadata_flag
				& SUNXI_METADATA_FLAG_HDR10P_METADATA))
				disp_unmap_afbc_header(&(lyr_cfg->config.info.fb));

		list_for_each_entry(lyr, &mgr->lyr_list, list) {
			lyr->dirty_clear(lyr);
		}
	}

	/* finish modifying lyr_cfgs, vsync update register will make sw_reg == hw_reg == lyr_cfgs*/
	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->setting = false;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	/* will force sync the manager when continue nosync appear */
	mgrp->force_sync = false;
	if (!mgrp->sync) {
		mgrp->nosync_cnt++;
		if (mgrp->nosync_cnt >= FORCE_SYNC_THRESHOLD) {
			mgrp->nosync_cnt = 0;
			mgrp->force_sync_cnt++;
			mgrp->force_sync = true;
			mgr->sync(mgr, true);
		}
	} else {
		mgrp->nosync_cnt = 0;
	}

	mutex_unlock(&mgr_mlock);
	dispdev = mgr->device;
	if ((dconf.metadata_flag & 0x3) &&
		dispdev && dispdev->set_dynamic_config
		&& send_vsif == false)
		dispdev->set_dynamic_config(dispdev, &dconf);

	return DIS_SUCCESS;
err:
	return -1;
}

static s32
disp_mgr_get_layer_config(struct disp_manager *mgr,
			  struct disp_layer_config *config,
			  unsigned int layer_num)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	struct disp_layer *lyr;
	unsigned int num_layers = 0, layer_index = 0;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	num_layers = bsp_disp_feat_get_num_layers(mgr->disp);
	if ((config == NULL) || (layer_num == 0) || (layer_num > num_layers)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	for (layer_index = 0; layer_index < layer_num; layer_index++) {
		lyr = disp_get_layer(mgr->disp, config->channel,
				     config->layer_id);
		if (lyr == NULL) {
			DE_WRN("get layer(%d,%d,%d) fail\n", mgr->disp,
			       config->channel, config->layer_id);
			continue;
		}
		if (lyr->get_config)
			lyr->get_config(lyr, config);
		config++;
	}

	return DIS_SUCCESS;
}

static s32
disp_mgr_get_layer_config2(struct disp_manager *mgr,
			  struct disp_layer_config2 *config,
			  unsigned int layer_num)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	struct disp_layer *lyr;
	unsigned int num_layers = 0, layer_index = 0;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		goto err;
	}

	num_layers = bsp_disp_feat_get_num_layers(mgr->disp);
	if ((config == NULL) || (layer_num == 0) || (layer_num > num_layers)) {
		DE_WRN("NULL hdl!\n");
		goto err;
	}
	for (layer_index = 0; layer_index < layer_num; layer_index++) {
		lyr = disp_get_layer(mgr->disp, config->channel,
				     config->layer_id);
		if (lyr != NULL)
			lyr->get_config2(lyr, config);
		else
			DE_WRN("get layer(%d,%d,%d) fail\n", mgr->disp,
			       config->channel, config->layer_id);
		config++;
	}

	return DIS_SUCCESS;

err:
	return -1;
}

static s32 disp_mgr_sync(struct disp_manager *mgr, bool sync)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = NULL;
	struct disp_enhance *enhance = NULL;
	struct disp_smbl *smbl = NULL;
	struct disp_capture *cptr = NULL;

	if (mgr == NULL) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (disp_feat_is_using_rcq(mgr->disp))
		return 0;

	mgrp = disp_mgr_get_priv(mgr);
	if (mgrp == NULL) {
		DE_WRN("get mgr %d's priv fail!!\n", mgr->disp);
		return -1;
	}

	mgrp->sync = sync;
	if (!mgrp->sync) {
		mgrp->dmabuf_unmap_skip_cnt_max = mgrp->dmabuf_unmap_skip_cnt++ >
							mgrp->dmabuf_unmap_skip_cnt_max ?
							mgrp->dmabuf_unmap_skip_cnt :
							mgrp->dmabuf_unmap_skip_cnt_max;
		return 0;
	}

	mgrp->nosync_cnt = 0;

	enhance = mgr->enhance;
	smbl = mgr->smbl;
	cptr = mgr->cptr;

	spin_lock_irqsave(&mgr_data_lock, flags);
	if (!mgrp->enabled) {
		spin_unlock_irqrestore(&mgr_data_lock, flags);
		return -1;
	}
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	/* mgr->update_regs(mgr); */
	disp_al_manager_sync(mgr->disp);
	mgr->update_regs(mgr);
	if (mgr->device && mgr->device->is_in_safe_period) {
		if (!mgr->device->is_in_safe_period(mgr->device)) {
			mgrp->err = true;
			mgrp->err_cnt++;
		} else {
			mgrp->err = false;
		}
	}
	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->applied = false;
	if (!mgrp->err && !mgrp->setting) {
		mgrp->dmabuf_unmap_skip_cnt = 0;
		/*update_regs ok, we know exactly what address is using, so just unmap dmabuf*/
		mgrp->unmap_dmabuf = true;
	} else {
		mgrp->dmabuf_unmap_skip_cnt_max = mgrp->dmabuf_unmap_skip_cnt++ >
							mgrp->dmabuf_unmap_skip_cnt_max ?
							mgrp->dmabuf_unmap_skip_cnt :
							mgrp->dmabuf_unmap_skip_cnt_max;
	}
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	/* enhance */
	if (enhance && enhance->sync)
		enhance->sync(enhance);

	/* smbl */
	if (smbl && smbl->sync)
		smbl->sync(smbl);

	/* capture */
	if (cptr && cptr->sync)
		cptr->sync(cptr);

	return DIS_SUCCESS;
}

static s32 disp_mgr_tasklet(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = NULL;
	struct disp_enhance *enhance = NULL;
	struct disp_smbl *smbl = NULL;
	struct disp_capture *cptr = NULL;

	if (mgr == NULL) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	mgrp = disp_mgr_get_priv(mgr);
	if (mgrp == NULL) {
		DE_WRN("get mgr %d's priv fail!!\n", mgr->disp);
		return -1;
	}
	enhance = mgr->enhance;
	smbl = mgr->smbl;
	cptr = mgr->cptr;

	if (!mgrp->enabled)
		return -1;

	/* enhance */
	if (enhance && enhance->tasklet)
		enhance->tasklet(enhance);

	/* smbl */
	if (smbl && smbl->tasklet)
		smbl->tasklet(smbl);

	/* capture */
	if (cptr && cptr->tasklet)
		cptr->tasklet(cptr);

	return DIS_SUCCESS;
}

static s32 disp_mgr_update_regs(struct disp_manager *mgr)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	/* FIXME,at sometimes,other module may need to sync while mgr don't */
	/* if (true == mgrp->applied) */
	disp_al_manager_update_regs(mgr->disp);

	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->applied = false;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	return DIS_SUCCESS;
}

static s32 disp_mgr_apply(struct disp_manager *mgr)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	struct disp_manager_data data;
	bool mgr_dirty = false;
	bool lyr_drity = false;
	struct disp_layer *lyr = NULL;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("mgr %d apply\n", mgr->disp);

	DISP_TRACE_BEGIN(__func__);
	spin_lock_irqsave(&mgr_data_lock, flags);
	if ((mgrp->enabled) && (mgrp->cfg->flag & MANAGER_ALL_DIRTY)) {
		mgr_dirty = true;
		memcpy(&data, mgrp->cfg, sizeof(struct disp_manager_data));
		mgrp->cfg->flag &= ~MANAGER_ALL_DIRTY;
	}

	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		if (lyr->is_dirty && lyr->is_dirty(lyr)) {
			lyr_drity = true;
			break;
		}
	}

	mgrp->applied = true;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	if (mgr->reg_protect)
		mgr->reg_protect(mgr, true);
	if (mgr_dirty)
		disp_al_manager_apply(mgr->disp, &data);

	if (lyr_drity) {
		u32 num_layers = bsp_disp_feat_get_num_layers(mgr->disp);
		struct disp_layer_config_data *lyr_cfg =
		    disp_mgr_get_layer_cfg_head(mgr);

		disp_al_layer_apply(mgr->disp, lyr_cfg, num_layers);
	}
	if (mgr->reg_protect)
		mgr->reg_protect(mgr, false);

	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->applied = true;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	DISP_TRACE_END(__func__);
	return DIS_SUCCESS;
}

static s32 disp_mgr_force_apply(struct disp_manager *mgr)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	struct disp_layer *lyr = NULL;
	struct disp_enhance *enhance = NULL;
	struct disp_smbl *smbl = NULL;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	enhance = mgr->enhance;
	smbl = mgr->smbl;

	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->cfg->flag |= MANAGER_ALL_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);
	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		lyr->force_apply(lyr);
	}

	disp_mgr_apply(mgr);
	disp_mgr_update_regs(mgr);

	/* enhance */
	if (enhance && enhance->force_apply)
		enhance->force_apply(enhance);

	/* smbl */
	if (smbl && smbl->force_apply)
		smbl->force_apply(smbl);

	return 0;
}

static s32 disp_mgr_enable(struct disp_manager *mgr)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	unsigned int width = 0, height = 0;
	unsigned int color_range = DISP_COLOR_RANGE_0_255;
	int ret;
	struct disp_device_config dev_config;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("mgr %d enable\n", mgr->disp);

	dev_config.bits = DISP_DATA_8BITS;
	dev_config.eotf = DISP_EOTF_GAMMA22;
	dev_config.cs = DISP_BT601_F;
	dev_config.dvi_hdmi = DISP_HDMI;
	dev_config.range = DISP_COLOR_RANGE_16_235;
	dev_config.scan = DISP_SCANINFO_NO_DATA;
	dev_config.aspect_ratio = 8;
	ret = disp_mgr_clk_enable(mgr);
	if (ret != 0)
		return ret;

	if (mgrp->irq_info.irq_flag)
		disp_register_irq(mgr->disp, &mgrp->irq_info);

	disp_al_manager_init(mgr->disp);

	if (mgr->device && mgr->device->type == DISP_OUTPUT_TYPE_RTWB)
		disp_al_manager_set_irq_enable(mgr->disp,
					       mgrp->irq_info.irq_flag, 0);
	else
		disp_al_manager_set_irq_enable(mgr->disp,
					       mgrp->irq_info.irq_flag, 1);
	rcq_init_finished = 1;

	if (mgr->device) {
		disp_al_device_set_de_id(mgr->device->hwdev_index, mgr->disp);
		disp_al_device_set_de_use_rcq(mgr->device->hwdev_index,
			disp_feat_is_using_rcq(mgr->disp));
		disp_al_device_set_output_type(mgr->device->hwdev_index,
			mgr->device->type);
		if (mgr->device->get_resolution)
			mgr->device->get_resolution(mgr->device, &width,
						    &height);
		if (mgr->device->get_static_config)
			mgr->device->get_static_config(mgr->device,
						       &dev_config);
		if (mgr->device && mgr->device->get_input_color_range)
			color_range =
			    mgr->device->get_input_color_range(mgr->device);
		mgrp->cfg->config.disp_device = mgr->device->disp;
		mgrp->cfg->config.hwdev_index = mgr->device->hwdev_index;
		if (mgr->device && mgr->device->is_interlace)
			mgrp->cfg->config.interlace =
			    mgr->device->is_interlace(mgr->device);
		else
			mgrp->cfg->config.interlace = 0;
		if (mgr->device && mgr->device->get_fps)
			mgrp->cfg->config.device_fps =
			    mgr->device->get_fps(mgr->device);
		else
			mgrp->cfg->config.device_fps = 60;
	}

	DE_INF("output res: %d x %d, cs=%d, range=%d, interlace=%d\n",
	       width, height, dev_config.cs, color_range, mgrp->cfg->config.interlace);

	mutex_lock(&mgr_mlock);
	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->enabled = 1;
	mgrp->cfg->config.enable = 1;
	mgrp->cfg->flag |= MANAGER_ENABLE_DIRTY;

	mgrp->cfg->config.size.width = width;
	mgrp->cfg->config.size.height = height;
	mgrp->cfg->config.cs = dev_config.format;
	mgrp->cfg->config.color_space = dev_config.cs;
	mgrp->cfg->config.eotf = dev_config.eotf;
	mgrp->cfg->config.data_bits = dev_config.bits;
	mgrp->cfg->config.hdr_type = dev_config.hdr_type;
	if (!mgrp->color_range_modified)
		mgrp->cfg->config.color_range = color_range;
	mgrp->cfg->flag |= MANAGER_SIZE_DIRTY;
	mgrp->cfg->flag |= MANAGER_COLOR_SPACE_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	disp_mgr_force_apply(mgr);

	if (mgr->enhance && mgr->enhance->enable)
		mgr->enhance->enable(mgr->enhance);
	mutex_unlock(&mgr_mlock);

	return 0;
}

#ifndef RTMX_USE_RCQ
#define RTMX_USE_RCQ (0)
#endif

static s32 disp_mgr_sw_enable(struct disp_manager *mgr)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	unsigned int width = 0, height = 0;
	unsigned int cs = 0, color_range = DISP_COLOR_RANGE_0_255;
	struct disp_device_config dev_config;
	struct disp_enhance *enhance = NULL;
	struct disp_smbl *smbl = NULL;
	struct disp_layer *lyr = NULL;
#if RTMX_USE_RCQ
	struct disp_device *dispdev;
	unsigned long curtime;
	unsigned int curline0, curline1;
	unsigned long cnt = 0;
#endif
	memset(&dev_config, 0, sizeof(struct disp_device_config));
	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	DE_INF("mgr %d enable\n", mgr->disp);

	dev_config.bits = DISP_DATA_8BITS;
	dev_config.eotf = DISP_EOTF_GAMMA22;
	dev_config.cs = DISP_BT601_F;
#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
	if (disp_mgr_clk_enable(mgr) != 0)
		return -1;
#endif

	if (mgrp->irq_info.irq_flag)
		disp_register_irq(mgr->disp, &mgrp->irq_info);
#if defined(DE_VERSION_V33X)
	disp_al_manager_init(mgr->disp);
#endif
	disp_al_manager_set_irq_enable(mgr->disp, mgrp->irq_info.irq_flag, 1);
	rcq_init_finished = 1;

	if (mgr->device) {
		disp_al_device_set_de_id(mgr->device->hwdev_index, mgr->disp);
		disp_al_device_set_de_use_rcq(mgr->device->hwdev_index,
			disp_feat_is_using_rcq(mgr->disp));
		disp_al_device_set_output_type(mgr->device->hwdev_index,
			mgr->device->type);
		if (mgr->device->get_resolution)
			mgr->device->get_resolution(mgr->device, &width,
						    &height);
		if (mgr->device->get_static_config)
			mgr->device->get_static_config(mgr->device,
						       &dev_config);
		if (mgr->device->get_input_csc)
			cs = mgr->device->get_input_csc(mgr->device);
		if (mgr->device && mgr->device->get_input_color_range)
			color_range =
			    mgr->device->get_input_color_range(mgr->device);
		mgrp->cfg->config.disp_device = mgr->device->disp;
		mgrp->cfg->config.hwdev_index = mgr->device->hwdev_index;
		if (mgr->device && mgr->device->is_interlace)
			mgrp->cfg->config.interlace =
			    mgr->device->is_interlace(mgr->device);
		else
			mgrp->cfg->config.interlace = 0;
		if (mgr->device && mgr->device->get_fps)
			mgrp->cfg->config.device_fps =
			    mgr->device->get_fps(mgr->device);
		else
			mgrp->cfg->config.device_fps = 60;
	}
	DE_INF("output res: %d x %d, cs=%d, range=%d, interlace=%d\n",
	       width, height, cs, color_range, mgrp->cfg->config.interlace);

	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->enabled = 1;
	mgrp->cfg->config.enable = 1;

	mgrp->cfg->config.size.width = width;
	mgrp->cfg->config.size.height = height;
	mgrp->cfg->config.cs = dev_config.format;
	mgrp->cfg->config.color_space = dev_config.cs;
	mgrp->cfg->config.eotf = dev_config.eotf;
	mgrp->cfg->config.data_bits = dev_config.bits;
	mgrp->cfg->config.color_range = color_range;
	mgrp->cfg->config.hdr_type = dev_config.hdr_type;
	mgrp->cfg->flag |= MANAGER_ALL_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);
	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		lyr->force_apply(lyr);
	}

#if RTMX_USE_RCQ
	/* wait for vertical blank period */
	dispdev = mgr->device;
	if (dispdev && dispdev->usec_before_vblank) {
		curtime = jiffies;
		while (dispdev->usec_before_vblank(dispdev) != 0) {
			if (time_after(jiffies, curtime + msecs_to_jiffies(50)))
				break;
			cnt++;
			if (cnt >= 1000 * 1000)
				break;
		}
	}

	if (dispdev) {
		curline0 = disp_al_device_get_cur_line(dispdev->hwdev_index);
		disp_mgr_sync(mgr, true);
#if IS_ENABLED(CONFIG_SUNXI_IOMMU)
		DISP_TRACE_BEGIN("enable_iommu");
		mgr->enable_iommu(mgr, true);
		DISP_TRACE_END("enable_iommu");
#endif
		curline1 = disp_al_device_get_cur_line(dispdev->hwdev_index);
		if (dispdev->is_in_safe_period) {
			if (!dispdev->is_in_safe_period(dispdev)) {
				DE_WRN("sync at non-safe "
						"period,start=%d,end=%d line\n",
						curline0, curline1);
			}
		}
	}

	DISP_TRACE_BEGIN("flush_layer_address");
	disp_al_flush_layer_address(mgr->disp, 1, 0);
	DISP_TRACE_END("flush_layer_address");

	// wait enough time to ensure the layer address
	// has been replace with the new framebuffer buffer address.
	msleep(50);

	DISP_TRACE_BEGIN("sw_apply");
	disp_mgr_apply(mgr);
	disp_mgr_update_regs(mgr);
	DISP_TRACE_END("sw_apply");
#else
	disp_mgr_apply(mgr);
#endif

	enhance = mgr->enhance;
	smbl = mgr->smbl;

	/* enhance */
	if (enhance && enhance->force_apply)
		enhance->force_apply(enhance);

	/* smbl */
	if (smbl && smbl->force_apply)
		smbl->force_apply(smbl);

	if (mgr->enhance && mgr->enhance->enable)
		mgr->enhance->enable(mgr->enhance);

	return 0;
}

static s32 disp_mgr_disable(struct disp_manager *mgr)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("mgr %d disable\n", mgr->disp);

	mutex_lock(&mgr_mlock);
	if (mgr->enhance && mgr->enhance->disable)
		mgr->enhance->disable(mgr->enhance);

	spin_lock_irqsave(&mgr_data_lock, flags);
	mgrp->enabled = 0;
	mgrp->cfg->flag |= MANAGER_ENABLE_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);
	disp_mgr_force_apply(mgr);
	disp_delay_ms(5);

	disp_al_manager_exit(mgr->disp);
	disp_mgr_clk_disable(mgr);
	atomic_set(&mgrp->wati_rcq_finish_flag, 0);
	mutex_unlock(&mgr_mlock);

	return 0;
}

static s32 disp_mgr_is_enabled(struct disp_manager *mgr)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return mgrp->enabled;

}

static s32 disp_mgr_dump(struct disp_manager *mgr, char *buf)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	unsigned int count = 0;
	char const *fmt_name[] = {"rgb", "yuv444", "yuv422", "yuv420"};
	char const *bits_name[] = {
		"8bits",
		"10bits",
		"12bits",
		"16bits"
	};
	bool direct_show = false;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if ((NULL == mgr) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	direct_show = disp_al_get_direct_show_state(mgr->disp);

	count += sprintf(
	    buf + count,
	    "mgr%d: %dx%d fmt[%s] cs[0x%x] range[%s] eotf[0x%x] bits[%s] err[%u] force_sync[%u] %s direct_show[%s] iommu[%d]\n",
	    mgr->disp, mgrp->cfg->config.size.width, mgrp->cfg->config.size.height,
	    (mgrp->cfg->config.cs < 4) ? fmt_name[mgrp->cfg->config.cs]
				       : "undef",
	    mgrp->cfg->config.color_space,
	    (mgrp->cfg->config.color_range == DISP_COLOR_RANGE_0_255) ? "full"
								      : "limit",
	    mgrp->cfg->config.eotf,
	    (mgrp->cfg->config.data_bits < 4) ?
		bits_name[mgrp->cfg->config.data_bits] : "undef",
		mgrp->err_cnt,
		 mgrp->force_sync_cnt,
		 (mgrp->cfg->config.blank) ? "blank" : "unblank",
	    (direct_show) ? "true" : "false", mgrp->iommu_en_flag);

	count += sprintf(
	    buf + count,
	    "dmabuf: cache[%d] cache max[%d] umap skip[%d] umap skip max[%d]\n",
	    mgrp->dmabuf_cnt, mgrp->dmabuf_cnt_max, mgrp->dmabuf_unmap_skip_cnt,
	    mgrp->dmabuf_unmap_skip_cnt_max);

	return count;

	return 0;
}

static s32 disp_mgr_blank(struct disp_manager *mgr, bool blank)
{
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);
	struct disp_layer *lyr = NULL;

	if ((mgr == NULL) || (mgrp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	mutex_lock(&mgr_mlock);
	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		lyr->force_apply(lyr);
	}
	mgrp->cfg->config.blank = blank;
	mgrp->cfg->flag |= MANAGER_BLANK_DIRTY;
	mgr->apply(mgr);
	mutex_unlock(&mgr_mlock);


	return 0;
}

s32 disp_mgr_set_ksc_para(struct disp_manager *mgr,
		    struct disp_ksc_info *pinfo)
{
	unsigned long flags;
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	spin_lock_irqsave(&mgr_data_lock, flags);
	memcpy(&mgrp->cfg->config.ksc, pinfo, sizeof(struct disp_colorkey));
	mgrp->cfg->flag |= MANAGER_KSC_DIRTY;
	spin_unlock_irqrestore(&mgr_data_lock, flags);

	return mgr->apply(mgr);
}

extern void sunxi_enable_device_iommu(unsigned int mastor_id, bool flag);

static s32 disp_mgr_enable_iommu(struct disp_manager *mgr, bool en)
{
#if IS_ENABLED(CONFIG_SUNXI_IOMMU)
	struct disp_manager_private_data *mgrp = disp_mgr_get_priv(mgr);

	if (mgrp->iommu_en_flag != en)
		sunxi_enable_device_iommu(mgrp->iommu_master_id, en);
	mgrp->iommu_en_flag = en;
#endif
	return 0;
}

s32 disp_init_mgr(struct disp_bsp_init_para *para)
{
	u32 num_screens;
	u32 disp;
	struct disp_manager *mgr;
	struct disp_manager_private_data *mgrp;

	DE_INF("%s\n", __func__);

	spin_lock_init(&mgr_data_lock);
	mutex_init(&mgr_mlock);

	num_screens = bsp_disp_feat_get_num_screens();
	mgrs = kmalloc_array(num_screens, sizeof(struct disp_manager),
			     GFP_KERNEL | __GFP_ZERO);
	if (mgrs == NULL) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	mgr_private = (struct disp_manager_private_data *)
	    kmalloc(sizeof(struct disp_manager_private_data) *
		    num_screens, GFP_KERNEL | __GFP_ZERO);
	if (mgr_private == NULL) {
		DE_WRN("malloc memory fail! size=0x%x x %d\n",
		       (unsigned int)sizeof(struct disp_manager_private_data),
		       num_screens);
		goto malloc_err;
	}
	mgr_cfgs = (struct disp_manager_data *)
	    kmalloc(sizeof(struct disp_manager_data) * num_screens,
		    GFP_KERNEL | __GFP_ZERO);
	if (mgr_private == NULL) {
		DE_WRN("malloc memory fail! size=0x%x x %d\n",
		       (unsigned int)sizeof(struct disp_manager_private_data),
		       num_screens);
		goto malloc_err;
	}

	for (disp = 0; disp < num_screens; disp++) {
		mgr = &mgrs[disp];
		mgrp = &mgr_private[disp];

		DE_INF("mgr %d, 0x%p\n", disp, mgr);

		sprintf(mgr->name, "mgr%d", disp);
		mgr->disp = disp;
		mgrp->cfg = &mgr_cfgs[disp];
		mgrp->irq_no = para->irq_no[DISP_MOD_DE];
		mgrp->shadow_protect = para->shadow_protect;
		mgrp->clk = para->clk_de[disp];
		mgrp->clk_bus = para->clk_bus_de[disp];
		if (para->rst_bus_de[disp])
			mgrp->rst = para->rst_bus_de[disp];
		mgrp->clk_dpss = para->clk_bus_dpss_top[disp];
		if (para->rst_bus_dpss_top[disp])
			mgrp->rst_dpss = para->rst_bus_dpss_top[disp];
		mgrp->clk_extra = para->clk_bus_extra;
		mgrp->rst_extra = para->rst_bus_extra;
		mgrp->irq_info.sel = disp;
		mgrp->irq_info.irq_flag = disp_feat_is_using_rcq(disp) ?
			DISP_AL_IRQ_FLAG_RCQ_FINISH : 0;
		mgrp->irq_info.ptr = (void *)mgr;
		mgrp->irq_info.irq_handler = disp_mgr_irq_handler;
		if (disp_feat_is_using_rcq(disp)) {
			init_waitqueue_head(&mgrp->wait_rcq_finish_queue);
			atomic_set(&mgrp->wati_rcq_finish_flag, 0);
		}

		mgrp->iommu_master_id =
		    (disp == 0) ? IOMMU_DE0_MASTOR_ID : IOMMU_DE1_MASTOR_ID;


		mgr->enable = disp_mgr_enable;
		mgr->sw_enable = disp_mgr_sw_enable;
		mgr->disable = disp_mgr_disable;
		mgr->is_enabled = disp_mgr_is_enabled;
		mgr->set_color_key = disp_mgr_set_color_key;
		mgr->get_color_key = disp_mgr_get_color_key;
		mgr->set_back_color = disp_mgr_set_back_color;
		mgr->get_back_color = disp_mgr_get_back_color;
		mgr->set_layer_config = disp_mgr_set_layer_config;
		mgr->force_set_layer_config = disp_mgr_force_set_layer_config;
		mgr->force_set_layer_config_exit = disp_mgr_force_set_layer_config_exit;
		mgr->get_layer_config = disp_mgr_get_layer_config;
		mgr->set_layer_config2 = disp_mgr_set_layer_config2;
		mgr->get_layer_config2 = disp_mgr_get_layer_config2;
		mgr->set_output_color_range = disp_mgr_set_output_color_range;
		mgr->get_output_color_range = disp_mgr_get_output_color_range;
		mgr->update_color_space = disp_mgr_update_color_space;
		mgr->smooth_switch = disp_mgr_smooth_switch;
		mgr->set_ksc_para = disp_mgr_set_ksc_para;
		mgr->dump = disp_mgr_dump;
		mgr->blank = disp_mgr_blank;
		mgr->get_clk_rate = disp_mgr_get_clk_rate;
		mgr->set_palette = disp_mgr_set_palette;

		mgr->init = disp_mgr_init;
		mgr->exit = disp_mgr_exit;

		mgr->apply = disp_mgr_apply;
		mgr->update_regs = disp_mgr_update_regs;
		mgr->force_apply = disp_mgr_force_apply;
		mgr->sync = disp_mgr_sync;
		mgr->tasklet = disp_mgr_tasklet;
		mgr->enable_iommu = disp_mgr_enable_iommu;

		if (disp_feat_is_using_rcq(disp))
			mgr->reg_protect = disp_mgr_protect_reg_for_rcq;
		else
			mgr->reg_protect = disp_mgr_shadow_protect;

		INIT_LIST_HEAD(&mgr->lyr_list);
		INIT_LIST_HEAD(&mgrp->dmabuf_list);

		mgr->init(mgr);
	}

	disp_init_lyr(para);

	return 0;

malloc_err:
	kfree(mgr_private);
	kfree(mgrs);

	return -1;
}

s32 disp_exit_mgr(void)
{
	u32 num_screens;
	u32 disp;
	struct disp_manager *mgr;

	if (!mgrs)
		return 0;

	disp_exit_lyr();
	num_screens = bsp_disp_feat_get_num_screens();
	for (disp = 0; disp < num_screens; disp++) {
		mgr = &mgrs[disp];
		mgr->exit(mgr);
	}

	kfree(mgr_private);
	kfree(mgrs);

	return 0;
}

#if defined(SUPPORT_RTWB)
s32
disp_mgr_set_rtwb_layer(struct disp_manager *mgr,
			  struct disp_layer_config2 *config,
			  struct disp_capture_info2 *p_cptr_info,
			  unsigned int layer_num)
{
#define LAYER_NUM_MAX 16
	struct disp_manager_private_data *mgrp;
	unsigned int num_layers = 0, i = 0;
	struct disp_layer *lyr = NULL;
	struct disp_layer_config_data *lyr_cfg_using[LAYER_NUM_MAX];
	struct disp_layer_config_data *lyr_cfg;
	struct dmabuf_item *item, *wb_item;
	struct disp_layer_config2 *config1 = config;
	unsigned long long ref = 0;
	struct fb_address_transfer fb;
	bool pre_force_sync;
	struct disp_device_dynamic_config dconf;
	unsigned int map_err_cnt = 0;
	int ret = -1;

	if (!mgr || !p_cptr_info) {
		DE_WRN("NULL hdl!\n");
		goto err;
	}
	mgrp = disp_mgr_get_priv(mgr);
	if (mgrp == NULL) {
		DE_WRN("NULL hdl!\n");
		goto err;
	}
	pre_force_sync = mgrp->force_sync;
	wb_item = disp_rtwb_config(mgr, p_cptr_info);
	if (!wb_item) {
		DE_WRN("rtwb config failed!\n");
		goto err;
	}

	DE_INF("mgr%d, config %d layers\n", mgr->disp, layer_num);

	num_layers = bsp_disp_feat_get_num_layers(mgr->disp);
	if ((config == NULL) || (layer_num == 0) || (layer_num > num_layers) || (layer_num > LAYER_NUM_MAX)) {
		DE_WRN("NULL hdl!\n");
		goto err;
	}

	memset(&dconf, 0, sizeof(struct disp_device_dynamic_config));
	mutex_lock(&mgr_mlock);

	/*every call of set_rtwb_layer should update hw_reg/sw_reg/lyr_cfgs successfully, it is ok to unmap dmabuf*/
	disp_unmap_dmabuf(mgr, mgrp, num_layers);
	for (i = 0; i < layer_num; i++) {
		struct disp_layer *lyr = NULL;

		lyr = disp_get_layer(mgr->disp, config1->channel,
				     config1->layer_id);

		if (lyr) {
			lyr->save_and_dirty_check2(lyr, config1);
			if (lyr->is_dirty(lyr) &&
			   (config1->info.fb.metadata_flag & 0x3)) {
				dconf.metadata_fd =
						config1->info.fb.metadata_fd;
				dconf.metadata_size =
						config1->info.fb.metadata_size;
				dconf.metadata_flag =
						config1->info.fb.metadata_flag;
			}
		}
		lyr_cfg_using[i] = disp_mgr_get_layer_cfg(mgr, config1);

		config1++;
	}


	lyr_cfg = disp_mgr_get_layer_cfg_head(mgr);
	ref = gdisp.screen[mgr->disp].health_info.irq_cnt;

	for (i = 0, lyr_cfg = lyr_cfg_using[0]; i < layer_num; i++, lyr_cfg++) {

		if (lyr_cfg->config.enable == 0)
			continue;
		/*color mode and set_layer_config do no need to dma map*/
		if (lyr_cfg->config.info.mode == LAYER_MODE_COLOR)
			continue;

		item = disp_dma_map(lyr_cfg->config.info.fb.fd);
		if (item == NULL) {
			pr_info("disp dma map fail!\n");
			lyr_cfg->config.enable = 0;
			map_err_cnt++;
			continue;
		}
		memcpy(&back_dma_item[mgr->disp][0], item, sizeof(struct dmabuf_item));
		item->lyr_id.disp = mgr->disp;
		item->lyr_id.channel = lyr_cfg->config.channel;
		item->lyr_id.layer_id = lyr_cfg->config.layer_id;
		item->lyr_id.type |= 0x1;
		fb.format = lyr_cfg->config.info.fb.format;
		memcpy(fb.size, lyr_cfg->config.info.fb.size,
		       sizeof(struct disp_rectsz) * 3);
		memcpy(fb.align, lyr_cfg->config.info.fb.align,
		       sizeof(int) * 3);
		fb.depth = lyr_cfg->config.info.fb.depth;
		fb.dma_addr = item->dma_addr;
		disp_set_fb_info(&fb, true);
		memcpy(lyr_cfg->config.info.fb.addr,
		       fb.addr,
		       sizeof(long long) * 3);

		lyr_cfg->config.info.fb.trd_right_addr[0] =
				(unsigned int)fb.trd_right_addr[0];
		lyr_cfg->config.info.fb.trd_right_addr[1] =
				(unsigned int)fb.trd_right_addr[1];
		lyr_cfg->config.info.fb.trd_right_addr[2] =
				(unsigned int)fb.trd_right_addr[2];
		disp_mgr_dmabuf_list_add(item, mgrp, ref);

		if (lyr_cfg->config.info.fb.fbd_en
			|| (lyr_cfg->config.info.fb.metadata_flag
			& SUNXI_METADATA_FLAG_HDR10P_METADATA)) {
			disp_map_afbc_header(&(lyr_cfg->config.info.fb));
		}

		/* get dma_buf for right image buffer */
		if (lyr_cfg->config.info.fb.flags == DISP_BF_STEREO_FP) {
			item = disp_dma_map(lyr_cfg->config.info.atw.cof_fd);
			if (item == NULL) {
				DE_WRN("disp dma map for right buffer fail!\n");
				lyr_cfg->config.info.fb.flags = DISP_BF_NORMAL;
				continue;
			}
			memcpy(&back_dma_item[mgr->disp][1], item, sizeof(struct dmabuf_item));
			item->lyr_id.disp = mgr->disp;
			item->lyr_id.channel = lyr_cfg->config.channel;
			item->lyr_id.layer_id = lyr_cfg->config.layer_id;
			item->lyr_id.type |= 0x2;
			fb.dma_addr = item->dma_addr;
			disp_set_fb_info(&fb, false);
			lyr_cfg->config.info.fb.trd_right_addr[0] =
					(unsigned int)fb.trd_right_addr[0];
			lyr_cfg->config.info.fb.trd_right_addr[1] =
					(unsigned int)fb.trd_right_addr[1];
			lyr_cfg->config.info.fb.trd_right_addr[2] =
					(unsigned int)fb.trd_right_addr[2];
			disp_mgr_dmabuf_list_add(item, mgrp, ref);
		}

		/* process 2d plus depth stereo mode */
		if (lyr_cfg->config.info.fb.flags == DISP_BF_STEREO_2D_DEPTH)  {
			lyr_cfg->config.info.fb.flags = DISP_BF_STEREO_FP;
			/* process depth, only support rgb format */
			if ((lyr_cfg->config.info.fb.depth != 0) &&
			    (lyr_cfg->config.info.fb.format
			      < DISP_FORMAT_YUV444_I_AYUV)) {
				int depth = lyr_cfg->config.info.fb.depth;
				unsigned long long abs_depth =
					(depth > 0) ? depth : (-depth);

				memcpy(fb.addr,
				       lyr_cfg->config.info.fb.addr,
				       sizeof(long long) * 3);
				fb.trd_right_addr[0] =
				    lyr_cfg->config.info.fb.trd_right_addr[0];
				fb.trd_right_addr[1] =
				    lyr_cfg->config.info.fb.trd_right_addr[1];
				fb.trd_right_addr[2] =
				    lyr_cfg->config.info.fb.trd_right_addr[2];
				if (disp_set_fb_base_on_depth(&fb) == 0) {
					memcpy(lyr_cfg->config.info.fb.addr,
					       fb.addr,
					       sizeof(long long) * 3);
		    lyr_cfg->config.info.fb.trd_right_addr[0] =
			(unsigned int)fb.trd_right_addr[0];
		    lyr_cfg->config.info.fb.trd_right_addr[1] =
			(unsigned int)fb.trd_right_addr[1];
		    lyr_cfg->config.info.fb.trd_right_addr[2] =
			(unsigned int)fb.trd_right_addr[2];

					lyr_cfg->config.info.fb.crop.width -=
					    (abs_depth << 32);
				}
			}

		}

		/* get dma_buf for atw coef buffer */
		if (!lyr_cfg->config.info.atw.used)
			continue;

		item = disp_dma_map(lyr_cfg->config.info.atw.cof_fd);
		if (item == NULL) {
			DE_WRN("disp dma map for atw coef fail!\n");
			lyr_cfg->config.info.atw.used = 0;
			continue;
		}
		memcpy(&back_dma_item[mgr->disp][2], item, sizeof(struct dmabuf_item));

		item->lyr_id.disp = mgr->disp;
		item->lyr_id.channel = lyr_cfg->config.channel;
		item->lyr_id.layer_id = lyr_cfg->config.layer_id;
		item->lyr_id.type |= 0x4;

		lyr_cfg->config.info.atw.cof_addr = item->dma_addr;
		disp_mgr_dmabuf_list_add(item, mgrp, ref);


	}

	mgrp->dmabuf_cnt_max = (mgrp->dmabuf_cnt > mgrp->dmabuf_cnt_max) ?
				mgrp->dmabuf_cnt : mgrp->dmabuf_cnt_max;
	if (mgr->apply)
		mgr->apply(mgr);

	lyr_cfg = disp_mgr_get_layer_cfg_head(mgr);
	for (i = 0; i < num_layers; i++, lyr_cfg++)
		if (lyr_cfg->config.info.fb.fbd_en
			|| (lyr_cfg->config.info.fb.metadata_flag
			& SUNXI_METADATA_FLAG_HDR10P_METADATA))
			disp_unmap_afbc_header(&(lyr_cfg->config.info.fb));

	list_for_each_entry(lyr, &mgr->lyr_list, list) {
		lyr->dirty_clear(lyr);
	}

	if (!disp_feat_is_using_rcq(mgr->disp))
		mgr->sync(mgr, true);

	ret = disp_rtwb_wait_finish(mgr);

	disp_dma_unmap(wb_item);

	mutex_unlock(&mgr_mlock);

err:
	return ret;
}
#endif

void disp_pq_force_flush(int disp)
{
	if (mgrs != NULL && disp < bsp_disp_feat_get_num_screens()) {
		disp_mgr_force_apply(&mgrs[disp]);
	}
}

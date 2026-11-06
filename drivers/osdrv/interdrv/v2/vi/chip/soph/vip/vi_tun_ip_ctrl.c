#include <linux/slab.h>
#include <vip/vi_drv.h>

#define BE_RUNTIME_TUN(_name) \
	{\
		struct sop_vip_isp_##_name##_config *cfg;\
		cfg = &be_tun->_name##_cfg;\
		ispblk_##_name##_tun_cfg(ctx, cfg, raw_num);\
	}

#define POST_RUNTIME_TUN(_name) \
	{\
		struct sop_vip_isp_##_name##_config *cfg;\
		cfg = &post_tun->_name##_cfg;\
		ispblk_##_name##_tun_cfg(ctx, cfg, raw_num);\
	}

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern int tuning_dis[4];

struct isp_tuning_cfg tuning_buf_addr;
static void *vi_tuning_ptr[ISP_PRERAW_MAX];

struct vi_clut_idx {
	u32		clut_tbl_idx[TUNING_NODE_NUM];
	spinlock_t	clut_idx_lock;
};

struct vi_clut_idx g_clut_idx[ISP_PRERAW_MAX];

/*******************************************************
 *  Internal APIs
 ******************************************************/
static int _blc_dev_mapping(
	struct isp_ctx *ctx,
	struct sop_vip_isp_blc_config *cfg,
	const enum sop_isp_raw raw_num)
{
	int id = cfg->inst;
	int raw_path = 0;

	if (id >= ISP_BLC_ID_FE0_LE && id <= ISP_BLC_ID_FE5_SE) {
		raw_path = (id % 2 == 0) ? ISP_RAW_PATH_LE : ISP_RAW_PATH_SE;
		id = raw_num * 2 + raw_path;
	}

	return id;
}

static int _wbg_dev_mapping(
	struct isp_ctx *ctx,
	struct sop_vip_isp_wbg_config *cfg,
	const enum sop_isp_raw raw_num)
{
	int id = cfg->inst;
	int raw_path = 0;

	if (id >= ISP_WBG_ID_FE0_LE && id <= ISP_WBG_ID_FE5_SE) {
		raw_path = (id % 2 == 0) ? ISP_RAW_PATH_LE : ISP_RAW_PATH_SE;
		id = raw_num * 2 + raw_path;
	}

	return id;
}

/*******************************************************************************
 *	Tuning modules update
 ******************************************************************************/
void vi_tuning_gamma_ips_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num)
{
	u8 tun_idx = 0;
	u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
	static int stop_update_gamma_ip = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;

	struct sop_vip_isp_ygamma_config  *ygamma_cfg;
	struct sop_vip_isp_gamma_config   *gamma_cfg;
	struct sop_vip_isp_ycur_config    *ycur_cfg;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[dev_num];
	tun_idx  = post_cfg->tun_idx;

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (stop_update_gamma_ip > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_gamma_ip = 1;
			return;
		} else if ((tuning_dis[0] - 1) == dev_num)
			stop_update_gamma_ip = 1; // stop on next
	} else
		stop_update_gamma_ip = 0;

	ygamma_cfg = &post_cfg->tun_cfg[tun_idx].ygamma_cfg;
	ispblk_ygamma_tun_cfg(ctx, ygamma_cfg, raw_num);

	gamma_cfg = &post_cfg->tun_cfg[tun_idx].gamma_cfg;
	ispblk_gamma_tun_cfg(ctx, gamma_cfg, raw_num);

	ycur_cfg = &post_cfg->tun_cfg[tun_idx].ycur_cfg;
	ispblk_ycur_tun_cfg(ctx, ycur_cfg, raw_num);
}

void vi_tuning_dci_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num)
{
	u8 tun_idx = 0;
	u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
	static int stop_update_dci = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;
	struct sop_vip_isp_dci_config   *dci_cfg;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[dev_num];
	tun_idx  = post_cfg->tun_idx;

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (stop_update_dci > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_dci = 1;
			return;
		} else if ((tuning_dis[0] - 1) == dev_num)
			stop_update_dci = 1; // stop on next
	} else
		stop_update_dci = 0;

	dci_cfg = &post_cfg->tun_cfg[tun_idx].dci_cfg;
	ispblk_dci_tun_cfg(ctx, dci_cfg, raw_num);
}

void vi_tuning_drc_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num)
{
	u8 tun_idx = 0;
	u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
	static int stop_update_drc = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;
	struct sop_vip_isp_drc_config   *drc_cfg;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[dev_num];
	tun_idx  = post_cfg->tun_idx;

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (stop_update_drc > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_drc = 1;
			return;
		} else if ((tuning_dis[0] - 1) == dev_num)
			stop_update_drc = 1; // stop on next
	} else
		stop_update_drc = 0;

	drc_cfg = &post_cfg->tun_cfg[tun_idx].drc_cfg;
	ispblk_drc_tun_cfg(ctx, drc_cfg, raw_num);
}

void vi_tuning_clut_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num)
{
	int ret = 0;
	u8 tun_idx = 0;
	u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
	static int stop_update_clut = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;
	struct sop_vip_isp_clut_config  *clut_cfg;
	unsigned long flags;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[dev_num];
	tun_idx  = post_cfg->tun_idx;

	vi_pr(VI_DBG, "Postraw_%d tuning update(%d):idx(%d)\n",
			raw_num, post_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (stop_update_clut > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_clut = 1;
			return;
		} else if ((tuning_dis[0] - 1) == dev_num)
			stop_update_clut = 1; // stop on next
	} else
		stop_update_clut = 0;

	clut_cfg = &post_cfg->tun_cfg[tun_idx].clut_cfg;
	ret = ispblk_clut_tun_cfg(ctx, clut_cfg, raw_num);

	//Record the clut tbl idx written into HW for ISP MW.
	spin_lock_irqsave(&g_clut_idx[raw_num].clut_idx_lock, flags);
	if (ret)
		g_clut_idx[raw_num].clut_tbl_idx[tun_idx] = clut_cfg->tbl_idx;
	else
		g_clut_idx[raw_num].clut_tbl_idx[tun_idx] = 0;
	spin_unlock_irqrestore(&g_clut_idx[raw_num].clut_idx_lock, flags);
}

int vi_tuning_get_clut_tbl_idx(enum sop_isp_raw raw_num, int tun_idx)
{
	unsigned long flags;
	u32 tbl_idx = 0;

	if (tun_idx >= TUNING_NODE_NUM || tun_idx < 0) {
		vi_pr(VI_WARN, "illegal tun_idx(%d)\n", tun_idx);
		return -1;
	}

	//Return clut tbl idx to ISP MW that currently is written into ISP HW.
	spin_lock_irqsave(&g_clut_idx[raw_num].clut_idx_lock, flags);
	tbl_idx = g_clut_idx[raw_num].clut_tbl_idx[tun_idx];
	spin_unlock_irqrestore(&g_clut_idx[raw_num].clut_idx_lock, flags);

	return tbl_idx;
}

int vi_tuning_sw_init(void)
{
	enum sop_isp_raw raw_num;
	u8 i = 0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		for (i = 0; i < TUNING_NODE_NUM; i++) {
			g_clut_idx[raw_num].clut_tbl_idx[i] = 0;
		}
		spin_lock_init(&g_clut_idx[raw_num].clut_idx_lock);
	}

	return 0;
}

int vi_tuning_buf_setup(struct isp_ctx *ctx)
{
	u64 post_paddr = 0, be_paddr = 0, fe_paddr = 0;
	u64 phyAddr = 0;
	u32 size = 0;
	u8 i = 0;
	u8 dev_num = 0;

	size = (VI_ALIGN(sizeof(struct sop_vip_isp_post_cfg)) +
		VI_ALIGN(sizeof(struct sop_vip_isp_be_cfg)) +
		VI_ALIGN(sizeof(struct sop_vip_isp_fe_cfg)));

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		if (!ctx->isp_pipe_enable[i])
			continue;

		if (_is_right_tile(ctx, i))
			continue;

		if (vi_tuning_ptr[i] != NULL)
			continue;

		vi_tuning_ptr[i] = kzalloc(size, GFP_KERNEL | __GFP_RETRY_MAYFAIL);
		if (vi_tuning_ptr[i] == NULL) {
			vi_pr(VI_ERR, "tuning_buf ptr[%d] kmalloc size(%u) fail\n", i, size);
			return -ENOMEM;
		}

		phyAddr = virt_to_phys(vi_tuning_ptr[i]);
		dev_num = vi_get_dev_num_by_raw(ctx, i);

		post_paddr = phyAddr;
		tuning_buf_addr.post_addr[dev_num] = post_paddr;
		tuning_buf_addr.post_vir[dev_num] = phys_to_virt(post_paddr);

		be_paddr = phyAddr + VI_ALIGN(sizeof(struct sop_vip_isp_post_cfg));
		tuning_buf_addr.be_addr[dev_num] = be_paddr;
		tuning_buf_addr.be_vir[dev_num] = phys_to_virt(be_paddr);

		fe_paddr = phyAddr + VI_ALIGN(sizeof(struct sop_vip_isp_post_cfg))
				+ VI_ALIGN(sizeof(struct sop_vip_isp_be_cfg));
		tuning_buf_addr.fe_addr[dev_num] = fe_paddr;
		tuning_buf_addr.fe_vir[dev_num] = phys_to_virt(fe_paddr);

		vi_pr(VI_INFO, "dev_num_%d tuning fe_addr[%d]=0x%llx, be_addr[%d]=0x%llx, post_addr[%d]=0x%llx\n",
				dev_num, i, tuning_buf_addr.fe_addr[dev_num],
				i, tuning_buf_addr.be_addr[dev_num],
				i, tuning_buf_addr.post_addr[dev_num]);
	}

	return 0;
}

void vi_tuning_buf_release(void)
{
	u8 i;

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		if (vi_tuning_ptr[i]) {
			kfree(vi_tuning_ptr[i]);
			vi_tuning_ptr[i] = NULL;
		}
	}
}

void *vi_get_tuning_buf_addr(u32 *size)
{
	*size = sizeof(struct isp_tuning_cfg);

	return (void *)&tuning_buf_addr;
}

void vi_tuning_buf_clear(void)
{
	struct sop_vip_isp_post_cfg *post_cfg;
	struct sop_vip_isp_be_cfg   *be_cfg;
	struct sop_vip_isp_fe_cfg   *fe_cfg;
	u8 i = 0, tun_idx = 0;

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[i];
		be_cfg   = (struct sop_vip_isp_be_cfg *)tuning_buf_addr.be_vir[i];
		fe_cfg   = (struct sop_vip_isp_fe_cfg *)tuning_buf_addr.fe_vir[i];

		if (tuning_buf_addr.post_vir[i] != NULL) {
			memset((void *)tuning_buf_addr.post_vir[i], 0x0, sizeof(struct sop_vip_isp_post_cfg));
			tun_idx = post_cfg->tun_idx;
			vi_pr(VI_INFO, "Clear post tuning tun_update(%d), tun_idx(%d)\n",
					post_cfg->tun_update[tun_idx], tun_idx);
		}

		if (tuning_buf_addr.be_vir[i] != NULL) {
			memset((void *)tuning_buf_addr.be_vir[i], 0x0, sizeof(struct sop_vip_isp_be_cfg));
			tun_idx = be_cfg->tun_idx;
			vi_pr(VI_INFO, "Clear be tuning tun_update(%d), tun_idx(%d)\n",
					be_cfg->tun_update[tun_idx], tun_idx);
		}

		if (tuning_buf_addr.fe_vir[i] != NULL) {
			memset((void *)tuning_buf_addr.fe_vir[i], 0x0, sizeof(struct sop_vip_isp_fe_cfg));
			tun_idx = fe_cfg->tun_idx;
			vi_pr(VI_INFO, "Clear fe tuning tun_update(%d), tun_idx(%d)\n",
					fe_cfg->tun_update[tun_idx], tun_idx);
		}
	}
}

void pre_fe_tuning_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num)
{
	u8 idx = 0, tun_idx = 0;
	u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
	static int stop_update = -1;
	struct sop_vip_isp_fe_cfg *fe_cfg;
	struct sop_vip_isp_fe_tun_cfg *fe_tun;

	fe_cfg = (struct sop_vip_isp_fe_cfg *)tuning_buf_addr.fe_vir[dev_num];
	tun_idx = fe_cfg->tun_idx;

	vi_pr(VI_DBG, "Pre_fe_%d tuning update(%d):idx(%d)\n",
			raw_num, fe_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (fe_cfg->tun_update[tun_idx] == 0))
		return;

	fe_tun = &fe_cfg->tun_cfg[tun_idx];

	if (tuning_dis[1]) {
		if (stop_update > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update = 1;
			return;
		} else if ((tuning_dis[0] - 1) == dev_num)
			stop_update = 1; // stop on next
	} else
		stop_update = 0;

	for (idx = 0; idx < 2; idx++) {
		struct sop_vip_isp_blc_config	*blc_cfg;
		//struct sop_vip_isp_lscr_config	*lscr_cfg;
		struct sop_vip_isp_wbg_config	*wbg_cfg;

		blc_cfg  = &fe_tun->blc_cfg[idx];
		ispblk_blc_tun_cfg(ctx, blc_cfg, raw_num);

		//lscr_cfg = &fe_tun->lscr_cfg[idx];
		//ispblk_lscr_tun_cfg(ctx, lscr_cfg, raw_num);

		wbg_cfg = &fe_tun->wbg_cfg[idx];
		ispblk_wbg_tun_cfg(ctx, wbg_cfg, raw_num);

	}
}

void pre_be_tuning_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num)
{
	u8 idx = 0, tun_idx = 0;
	u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
	static int stop_update = -1;
	struct sop_vip_isp_be_cfg *be_cfg;
	struct sop_vip_isp_be_tun_cfg *be_tun;

	be_cfg	= (struct sop_vip_isp_be_cfg *)tuning_buf_addr.be_vir[dev_num];
	tun_idx = be_cfg->tun_idx;

	if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile)
		return;

	vi_pr(VI_DBG, "Pre_be_%d tuning update(%d):idx(%d)\n",
			raw_num, be_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (be_cfg->tun_update[tun_idx] == 0))
		return;

	be_tun = &be_cfg->tun_cfg[tun_idx];

	if (tuning_dis[2]) {
		if (tuning_dis[0] == 0) {
			vi_pr(VI_DBG, "raw_%d stop tuning_update immediately\n", raw_num);
			return;
		} else if ((tuning_dis[0] - 1) == dev_num) {//stop on next
			if (stop_update > 0) {
				vi_pr(VI_DBG, "raw_%d stop tuning_update\n", raw_num);
				return;
			}
			stop_update = 1;
		} else {//must update tuning buf for sensor, it's will be not trrigered
			stop_update = 0;
		}
	} else
		stop_update = 0;

	for (idx = 0; idx < 2; idx++) {
		struct sop_vip_isp_blc_config	*blc_cfg;
		struct sop_vip_isp_rgbir_config	*rgbir_cfg;
		struct sop_vip_isp_dpc_config	*dpc_cfg;
		struct sop_vip_isp_ge_config	*ge_cfg;

		blc_cfg = &be_tun->blc_cfg[idx];
		ispblk_blc_tun_cfg(ctx, blc_cfg, raw_num);

		rgbir_cfg = &be_tun->rgbir_cfg[idx];
		ispblk_rgbir_tun_cfg(ctx, rgbir_cfg, raw_num);

		dpc_cfg = &be_tun->dpc_cfg[idx];
		ispblk_dpc_tun_cfg(ctx, dpc_cfg, raw_num);

		ge_cfg = &be_tun->ge_cfg[idx];
		ispblk_ge_tun_cfg(ctx, ge_cfg, raw_num);

	}

	BE_RUNTIME_TUN(af);
}

void postraw_tuning_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num)
{
	int ret = 0;
	u8 idx = 0, tun_idx = 0;
	u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
	static int stop_update = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;
	struct sop_vip_isp_clut_config  *clut_cfg;
	unsigned long flags;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[dev_num];
	tun_idx  = post_cfg->tun_idx;

	if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile)
		return;

	vi_pr(VI_DBG, "Postraw_%d tuning update(%d):idx(%d)\n",
			raw_num, post_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[3]) {
		if (tuning_dis[0] == 0) {
			vi_pr(VI_DBG, "raw_%d stop tuning_update immediately\n", raw_num);
			return;
		} else if ((tuning_dis[0] - 1) == dev_num) {//stop on next
			if (stop_update > 0) {
				vi_pr(VI_DBG, "raw_%d stop tuning_update\n", raw_num);
				return;
			}
			stop_update = 1;
		} else {//must update tuning buf for sensor, it's will be not trrigered
			stop_update = 0;
		}
	} else
		stop_update = 0;

	for (idx = 0; idx < (ctx->isp_pipe_cfg[raw_num].is_hdr_on + 1); idx++) {
		struct sop_vip_isp_bnr_config		*bnr_cfg;
		struct sop_vip_isp_lsc_config		*lsc_cfg;
		struct sop_vip_isp_ae_config		*ae_cfg;
		struct sop_vip_isp_wbg_config		*wbg_cfg;
		struct sop_vip_isp_demosiac_config	*demosiac_cfg;
		struct sop_vip_isp_rgbcac_config	*rgbcac_cfg;
		struct sop_vip_isp_lcac_config		*lcac_cfg;
		struct sop_vip_isp_ccm_config		*ccm_cfg;

		bnr_cfg = &post_tun->bnr_cfg[idx];
		ispblk_bnr_tun_cfg(ctx, bnr_cfg, raw_num);

		lsc_cfg = &post_tun->lsc_cfg[idx];
		ispblk_lsc_tun_cfg(ctx, lsc_cfg, raw_num);

		ae_cfg = &post_tun->ae_cfg[idx];
		ispblk_ae_tun_cfg(ctx, ae_cfg, raw_num);

		wbg_cfg = &post_tun->wbg_cfg[idx];
		ispblk_wbg_tun_cfg(ctx, wbg_cfg, raw_num);

		demosiac_cfg = &post_tun->demosiac_cfg[idx];
		ispblk_demosiac_tun_cfg(ctx, demosiac_cfg, raw_num);

		rgbcac_cfg = &post_tun->rgbcac_cfg[idx];
		ispblk_rgbcac_tun_cfg(ctx, rgbcac_cfg, raw_num);

		lcac_cfg = &post_tun->lcac_cfg[idx];
		ispblk_lcac_tun_cfg(ctx, lcac_cfg, raw_num);

		ccm_cfg = &post_tun->ccm_cfg[idx];
		ispblk_ccm_tun_cfg(ctx, ccm_cfg, raw_num);

	}


	POST_RUNTIME_TUN(gms);

	POST_RUNTIME_TUN(fswdr);
	// POST_RUNTIME_TUN(drc);
	POST_RUNTIME_TUN(hist_v);
	//HW limit
	//Need to update gamma ips in postraw done
	//POST_RUNTIME_TUN(ygamma);
	//POST_RUNTIME_TUN(gamma);
	POST_RUNTIME_TUN(dhz);

	//Clut can't be writen when streaming
	//HW limit
	//Need to update gamma ips in postraw done
	if (_is_be_post_online(ctx) || (!ctx->is_slice_buf_on)) {// _be_post_online or not slice buffer mode
		if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {//timing of raw replay is before trigger
			POST_RUNTIME_TUN(drc);
			POST_RUNTIME_TUN(ygamma);
			POST_RUNTIME_TUN(gamma);
			POST_RUNTIME_TUN(dci);
			POST_RUNTIME_TUN(ycur);

			clut_cfg = &post_tun->clut_cfg;
			ret = ispblk_clut_tun_cfg(ctx, clut_cfg, raw_num);

			//Record the clut tbl idx written into HW for ISP MW.
			spin_lock_irqsave(&g_clut_idx[raw_num].clut_idx_lock, flags);
			if (ret)
				g_clut_idx[raw_num].clut_tbl_idx[tun_idx] = clut_cfg->tbl_idx;
			else
				g_clut_idx[raw_num].clut_tbl_idx[tun_idx] = 0;
			spin_unlock_irqrestore(&g_clut_idx[raw_num].clut_idx_lock, flags);
		}
	}

	POST_RUNTIME_TUN(csc);
	//HW limit
	//To update dci tuning at postraw done because josh's ping pong sram has bug
	//POST_RUNTIME_TUN(dci);
	POST_RUNTIME_TUN(ldci);
	POST_RUNTIME_TUN(pre_ee);
	POST_RUNTIME_TUN(tnr);
	POST_RUNTIME_TUN(cnr);
	POST_RUNTIME_TUN(cac);
	POST_RUNTIME_TUN(ynr);
	POST_RUNTIME_TUN(ee);
	POST_RUNTIME_TUN(cacp);
	POST_RUNTIME_TUN(ca2);
	//HW limit
	//Need to update gamma ips in postraw done
	//POST_RUNTIME_TUN(ycur);
	{
		struct sop_vip_isp_mono_config *cfg;
		static u8 mono_mode_sts[ISP_PRERAW_MAX];

		cfg = &post_tun->mono_cfg;
		ispblk_mono_tun_cfg(ctx, cfg, raw_num);

		if (_is_be_post_online(ctx) ||
			(_is_fe_be_online(ctx) && !ctx->is_slice_buf_on)) {//Not single sensor with slice mode
			if (cfg->force_mono_enable != mono_mode_sts[raw_num]) {
				isp_first_frm_reset(ctx, 1);
			} else {
				isp_first_frm_reset(ctx, 0);
			}
		}
		mono_mode_sts[raw_num] = cfg->force_mono_enable;
	}
}

/****************************************************************************
 *	Tuning Config
 ****************************************************************************/
void ispblk_blc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_blc_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t blc;
	int dev_id, id;

	if (!cfg->update)
		return;

	dev_id = _blc_dev_mapping(ctx, cfg, raw_num);
	id = blc_find_hwid(dev_id);
	if (id < 0)
		return;

	blc = ctx->phys_regs[id];

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_0, blc_bypass, cfg->bypass);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_2, blc_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_3, blc_offset_r, cfg->roffset);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_3, blc_offset_gr, cfg->groffset);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_4, blc_offset_gb, cfg->gboffset);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_4, blc_offset_b, cfg->boffset);

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_9, blc_2ndoffset_r, cfg->roffset_2nd);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_9, blc_2ndoffset_gr, cfg->groffset_2nd);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_a, blc_2ndoffset_gb, cfg->gboffset_2nd);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_a, blc_2ndoffset_b, cfg->boffset_2nd);

	ISP_WR_BITS(blc, reg_isp_blc_t, blc_5, blc_gain_r, cfg->rgain);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_5, blc_gain_gr, cfg->grgain);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_6, blc_gain_gb, cfg->gbgain);
	ISP_WR_BITS(blc, reg_isp_blc_t, blc_6, blc_gain_b, cfg->bgain);
}

void ispblk_wbg_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_wbg_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ba;
	int dev_id, id;

	if (!cfg->update)
		return;

	dev_id = _wbg_dev_mapping(ctx, cfg, raw_num);
	id = wbg_find_hwid(dev_id);
	if (id < 0) {
		return;
	}

	ba = ctx->phys_regs[id];

	ISP_WR_BITS(ba, reg_isp_wbg_t, wbg_0, wbg_bypass, cfg->bypass);
	ISP_WR_BITS(ba, reg_isp_wbg_t, wbg_2, wbg_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ba, reg_isp_wbg_t, wbg_4, wbg_rgain, cfg->rgain);
	ISP_WR_BITS(ba, reg_isp_wbg_t, wbg_4, wbg_ggain, cfg->ggain);
	ISP_WR_BITS(ba, reg_isp_wbg_t, wbg_5, wbg_bgain, cfg->bgain);
	ISP_WR_REG(ba, reg_isp_wbg_t, wbg_34, cfg->rgain_fraction);
	ISP_WR_REG(ba, reg_isp_wbg_t, wbg_38, cfg->ggain_fraction);
	ISP_WR_REG(ba, reg_isp_wbg_t, wbg_3c, cfg->bgain_fraction);
}

/****************************************************************************
 *	Postraw Tuning Config
 ****************************************************************************/
void ispblk_ccm_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ccm_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ccm = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_CCM0] : ctx->phys_regs[ISP_BLK_ID_CCM1];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ccm, reg_isp_ccm_t, ccm_ctrl, ccm_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_00, cfg->coef[0][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_01, cfg->coef[0][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_02, cfg->coef[0][2]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_10, cfg->coef[1][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_11, cfg->coef[1][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_12, cfg->coef[1][2]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_20, cfg->coef[2][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_21, cfg->coef[2][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_22, cfg->coef[2][2]);
}

void ispblk_cacp_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cacp_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_CA];
	u16 i;
	union reg_ca_00 ca_00;
	union reg_ca_04 wdata;

	if (!cfg->update)
		return;

	ca_00.raw = ISP_RD_REG(cacp, reg_ca_t, reg_00);
	ca_00.bits.cacp_enable		= cfg->enable;
	ca_00.bits.cacp_mode		= cfg->mode; // 0 CA mode, 1 CP mode
	ca_00.bits.cacp_iso_ratio	= cfg->iso_ratio;
	ISP_WR_REG(cacp, reg_ca_t, reg_00, ca_00.raw);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mem_sw_mode, 1);
	if (cfg->mode == 0) {
		for (i = 0; i < 256; i++) {
			wdata.raw = 0;
			wdata.bits.cacp_mem_d = cfg->ca_y_ratio_lut[i];
			wdata.bits.cacp_mem_w = 1;
			ISP_WR_REG(cacp, reg_ca_t, reg_04, wdata.raw);
		}
	} else { //cp mode
		for (i = 0; i < 256; i++) {
			wdata.raw = 0;
			wdata.bits.cacp_mem_d = ((cfg->cp_y_lut[i] << 16) |
					(cfg->cp_u_lut[i] << 8) | (cfg->cp_v_lut[i]));
			wdata.bits.cacp_mem_w = 1;
			ISP_WR_REG(cacp, reg_ca_t, reg_04, wdata.raw);
		}
	}

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mem_sw_mode, 0);
}

void ispblk_ca2_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ca2_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_CA_LITE];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_00, ca_lite_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_04, ca_lite_lut_in_0, cfg->lut_in[0]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_04, ca_lite_lut_in_1, cfg->lut_in[1]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_08, ca_lite_lut_in_2, cfg->lut_in[2]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_08, ca_lite_lut_in_3, cfg->lut_in[3]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_0c, ca_lite_lut_in_4, cfg->lut_in[4]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_0c, ca_lite_lut_in_5, cfg->lut_in[5]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_10, ca_lite_lut_out_0, cfg->lut_out[0]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_10, ca_lite_lut_out_1, cfg->lut_out[1]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_14, ca_lite_lut_out_2, cfg->lut_out[2]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_14, ca_lite_lut_out_3, cfg->lut_out[3]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_18, ca_lite_lut_out_4, cfg->lut_out[4]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_18, ca_lite_lut_out_5, cfg->lut_out[5]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_1c, ca_lite_lut_slp_0, cfg->lut_slp[0]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_1c, ca_lite_lut_slp_1, cfg->lut_slp[1]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_20, ca_lite_lut_slp_2, cfg->lut_slp[2]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_20, ca_lite_lut_slp_3, cfg->lut_slp[3]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_24, ca_lite_lut_slp_4, cfg->lut_slp[4]);
}

void ispblk_ygamma_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ygamma_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_YGAMMA];
	int16_t i;

	union reg_ygamma_gamma_prog_data reg_data;
	union reg_ygamma_gamma_prog_ctrl prog_ctrl;

	if (!cfg->update)
		return;

	ISP_WR_BITS(gamma, reg_ygamma_t, gamma_ctrl, ygamma_enable, cfg->enable);

	if (!cfg->enable)
		return;

	prog_ctrl.raw = ISP_RD_REG(gamma, reg_ygamma_t, gamma_prog_ctrl);
	prog_ctrl.bits.gamma_wsel		= prog_ctrl.bits.gamma_wsel ^ 1;

	prog_ctrl.bits.gamma_prog_en		= 1;
	prog_ctrl.bits.gamma_prog_1to3_en	= 1;
	ISP_WR_REG(gamma, reg_ygamma_t, gamma_prog_ctrl, prog_ctrl.raw);

	ISP_WR_BITS(gamma, reg_ygamma_t, gamma_prog_st_addr, gamma_st_addr, 0);
	ISP_WR_BITS(gamma, reg_ygamma_t, gamma_prog_st_addr, gamma_st_w, 1);
	ISP_WR_REG(gamma, reg_ygamma_t, gamma_prog_max, cfg->max);

	for (i = 0; i < 256; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.gamma_data_e = cfg->lut[i];
		reg_data.bits.gamma_data_o = cfg->lut[i + 1];
		ISP_WR_REG(gamma, reg_ygamma_t, gamma_prog_data, reg_data.raw);
		ISP_WR_BITS(gamma, reg_ygamma_t, gamma_prog_ctrl, gamma_w, 1);
	}

	ISP_WR_BITS(gamma, reg_ygamma_t, gamma_prog_ctrl, gamma_rsel, prog_ctrl.bits.gamma_wsel);
	ISP_WR_BITS(gamma, reg_ygamma_t, gamma_prog_ctrl, gamma_prog_en, 0);
}

void ispblk_gamma_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_gamma_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];
	int16_t i;

	union reg_isp_gamma_prog_data reg_data;
	union reg_isp_gamma_prog_ctrl prog_ctrl;

	if (!cfg->update)
		return;

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_ctrl, gamma_enable, cfg->enable);

	if (!cfg->enable)
		return;

	prog_ctrl.raw = ISP_RD_REG(gamma, reg_isp_gamma_t, gamma_prog_ctrl);
	prog_ctrl.bits.gamma_wsel		= prog_ctrl.bits.gamma_wsel ^ 1;

	prog_ctrl.bits.gamma_prog_en		= 1;
	prog_ctrl.bits.gamma_prog_1to3_en	= 1;
	ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_ctrl, prog_ctrl.raw);

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_st_addr, gamma_st_addr, 0);
	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_st_addr, gamma_st_w, 1);
	ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_max, cfg->max);

	for (i = 0; i < 256; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.gamma_data_e = cfg->lut[i];
		reg_data.bits.gamma_data_o = cfg->lut[i + 1];
		reg_data.bits.gamma_w = 1;
		ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_data, reg_data.raw);
	}

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_rsel, prog_ctrl.bits.gamma_wsel);
	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_prog_en, 0);
}

void ispblk_demosiac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_demosiac_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t cfa = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_CFA0] : ctx->phys_regs[ISP_BLK_ID_CFA1];
	uintptr_t rgbcac = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_RGBCAC0] : ctx->phys_regs[ISP_BLK_ID_RGBCAC1];
	union reg_isp_cfa_00 reg_00;
	union reg_isp_cfa_04 reg_04;
	union reg_isp_cfa_110 reg_110;

	if (!cfg->update)
		return;

	if (ISP_RD_BITS(rgbcac, reg_isp_rgbcac_t, rgbcac_ctrl, rgbcac_enable) && !cfg->cfa_enable) {
		vi_pr(VI_WARN, "[WARN] not support cfa disable && rgbcac enable\n");
		return;
	}

	reg_00.raw = ISP_RD_REG(cfa, reg_isp_cfa_t, reg_00);
	reg_00.bits.cfa_enable			= cfg->cfa_enable;
	reg_00.bits.cfa_force_dir_enable	= cfg->cfa_force_dir_enable;
	reg_00.bits.cfa_force_dir_sel		= cfg->cfa_force_dir_sel;
	reg_00.bits.cfa_ymoire_enable		= cfg->cfa_ymoire_enable;
	ISP_WR_REG(cfa, reg_isp_cfa_t, reg_00, reg_00.raw);

	if (!cfg->cfa_enable)
		return;

	reg_04.raw = ISP_RD_REG(cfa, reg_isp_cfa_t, reg_04);
	reg_04.bits.cfa_out_sel		= cfg->cfa_out_sel;
	reg_04.bits.cfa_edgee_thd2	= cfg->cfa_edgee_thd2;
	ISP_WR_REG(cfa, reg_isp_cfa_t, reg_04, reg_04.raw);

	ISP_WR_BITS(cfa, reg_isp_cfa_t, reg_20, cfa_rbsig_luma_thd, cfg->cfa_rbsig_luma_thd);

	reg_110.raw = ISP_RD_REG(cfa, reg_isp_cfa_t, reg_110);
	reg_110.bits.cfa_ymoire_lpf_w	= cfg->cfa_ymoire_lpf_w;
	reg_110.bits.cfa_ymoire_dc_w	= cfg->cfa_ymoire_dc_w;
	ISP_WR_REG(cfa, reg_isp_cfa_t, reg_110, reg_110.raw);

	ISP_WR_REG_LOOP_SHFT(cfa, reg_isp_cfa_t, reg_30, 32, 4, cfg->cfa_ghp_lut, 8);

	ISP_WR_REGS_BURST(cfa, reg_isp_cfa_t, reg_0c, cfg->demosiac_cfg, cfg->demosiac_cfg.reg_0c);

	ISP_WR_REGS_BURST(cfa, reg_isp_cfa_t, reg_120, cfg->demosiac_1_cfg, cfg->demosiac_1_cfg.reg_120);

	ISP_WR_REGS_BURST(cfa, reg_isp_cfa_t, reg_90, cfg->demosiac_2_cfg, cfg->demosiac_2_cfg.reg_90);
}

void ispblk_lsc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_lsc_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t lsc = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_LSC0] : ctx->phys_regs[ISP_BLK_ID_LSC1];
	union reg_isp_lsc_enable lsc_enable;
	union reg_isp_lsc_interpolation inter_p;
	union reg_isp_lsc_bld lsc_bld;

	if (!cfg->update)
		return;

	ISP_WR_BITS(lsc, reg_isp_lsc_t, dmi_enable, dmi_enable, cfg->enable);
	ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_enable, lsc_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_strength, cfg->strength);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_gain_base, cfg->gain_base);
	ISP_WR_BITS(lsc, reg_isp_lsc_t, lsc_dummy, lsc_debug, cfg->debug);

	inter_p.raw = ISP_RD_REG(lsc, reg_isp_lsc_t, interpolation);
	inter_p.bits.lsc_boundary_interpolation_lf_range = cfg->boundary_interpolation_lf_range;
	inter_p.bits.lsc_boundary_interpolation_up_range = cfg->boundary_interpolation_up_range;
	inter_p.bits.lsc_boundary_interpolation_rt_range = cfg->boundary_interpolation_rt_range;
	inter_p.bits.lsc_boundary_interpolation_dn_range = cfg->boundary_interpolation_dn_range;
	ISP_WR_REG(lsc, reg_isp_lsc_t, interpolation, inter_p.raw);

	lsc_enable.raw = ISP_RD_REG(lsc, reg_isp_lsc_t, lsc_enable);
	lsc_enable.bits.lsc_gain_3p9_0_4p8_1 = cfg->gain_3p9_0_4p8_1;
	lsc_enable.bits.lsc_gain_bicubic_0_bilinear_1 = cfg->gain_bicubic_0_bilinear_1;
	lsc_enable.bits.lsc_boundary_interpolation_mode = cfg->boundary_interpolation_mode;
	lsc_enable.bits.lsc_renormalize_enable = cfg->renormalize_enable;
	lsc_enable.bits.lsc_hdr_enable = ctx->isp_pipe_cfg[raw_num].is_hdr_on ? 1 : 0;
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_enable, lsc_enable.raw);

	lsc_bld.raw = ISP_RD_REG(lsc, reg_isp_lsc_t, lsc_bld);
	lsc_bld.bits.lsc_bldratio_enable = cfg->bldratio_enable;
	lsc_bld.bits.lsc_bldratio = cfg->bldratio;
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_bld, lsc_bld.raw);

	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_intp_gain_max, cfg->intp_gain_max);
	ISP_WR_REG(lsc, reg_isp_lsc_t, lsc_intp_gain_min, cfg->intp_gain_min);
}

void ispblk_bnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_bnr_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t bnr = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_BNR0] : ctx->phys_regs[ISP_BLK_ID_BNR1];
	u16 i = 0;

	if (!cfg->update)
		return;

	if (cfg->enable) {
		if ((cfg->out_sel == 8) || ((cfg->out_sel >= 11) && (cfg->out_sel <= 15)))
			ISP_WO_BITS(bnr, reg_isp_bnr_t, out_sel, bnr_out_sel, cfg->out_sel);
		else
			vi_pr(VI_ERR, "[ERR] BNR out_sel(%d) should be 8 and 11~15\n", cfg->out_sel);

		ISP_WR_REG(bnr, reg_isp_bnr_t, weight_intra_0, cfg->weight_intra_0);
		ISP_WR_REG(bnr, reg_isp_bnr_t, weight_intra_1, cfg->weight_intra_1);
		ISP_WR_REG(bnr, reg_isp_bnr_t, weight_intra_2, cfg->weight_intra_2);
		ISP_WR_REG(bnr, reg_isp_bnr_t, weight_norm_1, cfg->weight_norm_1);
		ISP_WR_REG(bnr, reg_isp_bnr_t, weight_norm_2, cfg->weight_norm_2);
		ISP_WR_REG(bnr, reg_isp_bnr_t, res_k_smooth, cfg->k_smooth);
		ISP_WR_REG(bnr, reg_isp_bnr_t, res_k_texture, cfg->k_texture);

		ISP_WR_REGS_BURST(bnr, reg_isp_bnr_t, ns_luma_th_r,
					cfg->bnr_1_cfg, cfg->bnr_1_cfg.ns_luma_th_r);

		ISP_WR_REGS_BURST(bnr, reg_isp_bnr_t, var_th,
					cfg->bnr_2_cfg, cfg->bnr_2_cfg.var_th);

		ISP_WO_BITS(bnr, reg_isp_bnr_t, index_clr, bnr_index_clr, 1);
		for (i = 0; i < 8; i++)
			ISP_WR_REG(bnr, reg_isp_bnr_t, intensity_sel, cfg->intensity_sel[i]);

		for (i = 0; i < 256; i++)
			ISP_WR_REG(bnr, reg_isp_bnr_t, weight_lut, cfg->weight_lut[i]);
	} else {
		ISP_WO_BITS(bnr, reg_isp_bnr_t, out_sel, bnr_out_sel, 1);
	}

	ISP_WO_BITS(bnr, reg_isp_bnr_t, shadow_rd_sel, shadow_rd_sel, 1);
}

void ispblk_clut_partial_update(
	struct isp_ctx *ctx,
	struct sop_vip_isp_clut_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	union reg_isp_clut_ctrl      ctrl;
	union reg_isp_clut_prog_data prog_data;
	u32 i = 0;

	if ((_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)) && cfg->update_length >= 256)
		cfg->update_length = 256;
	else if (cfg->update_length >= 1024)
		cfg->update_length = 1024;

	ctrl.raw = ISP_RD_REG(clut, reg_isp_clut_t, clut_ctrl);
	ctrl.bits.prog_en = 1;
	ISP_WR_REG(clut, reg_isp_clut_t, clut_ctrl, ctrl.raw);

	for (; i < cfg->update_length; i++) {
		ISP_WR_REG(clut, reg_isp_clut_t, clut_prog_addr, cfg->lut[i][0]);

		prog_data.raw			= 0;
		prog_data.bits.sram_wdata	= cfg->lut[i][1];
		prog_data.bits.sram_wr		= 1;
		ISP_WR_REG(clut, reg_isp_clut_t, clut_prog_data, prog_data.raw);
	}

	ctrl.bits.clut_enable = cfg->enable;
	ctrl.bits.prog_en = 0;
	ISP_WR_REG(clut, reg_isp_clut_t, clut_ctrl, ctrl.raw);
}

int ispblk_clut_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_clut_config *cfg,
	const enum sop_isp_raw raw_num)
{
	if (!cfg->update)
		return 0;

	if (cfg->is_update_partial) { //partail update table
		ispblk_clut_partial_update(ctx, cfg, raw_num);
	} else if (!(_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on))) {
		//ispblk_clut_config(ctx, cfg->enable, cfg->r_lut, cfg->g_lut, cfg->b_lut);
		ispblk_clut_cmdq_config(ctx, raw_num, cfg->enable, cfg->r_lut, cfg->g_lut, cfg->b_lut);
	}

	return 1;
}

void ispblk_drc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_drc_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_LTM];
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	uintptr_t lmap0 = ctx->phys_regs[ISP_BLK_ID_LMAP0];
	uintptr_t lmap1 = ctx->phys_regs[ISP_BLK_ID_LMAP1];
	union reg_ltm_h00 reg_00;
	union reg_ltm_h08 reg_08;
	union reg_ltm_h0c reg_0c;
	union reg_isp_lmap_lmp_0 lmp_0;
	union reg_ltm_h3c reg_3c;
	u8 sel = 0;

	if (!cfg->update)
		return;

	reg_00.raw = ISP_RD_REG(ba, reg_ltm_t, reg_h00);
	reg_00.bits.ltm_enable			= cfg->ltm_enable;
	reg_00.bits.ltm_dark_enh_enable		= cfg->dark_enh_en;
	reg_00.bits.ltm_brit_enh_enable		= cfg->brit_enh_en;
	reg_00.bits.ltm_dbg_mode		= cfg->dbg_mode;
	reg_00.bits.force_dma_disable		= ((!cfg->dark_enh_en) | (!cfg->brit_enh_en << 1));
	reg_00.bits.dark_tone_wgt_refine_enable	= cfg->dark_tone_wgt_refine_en;
	reg_00.bits.brit_tone_wgt_refine_enable	= cfg->brit_tone_wgt_refine_en;
	ISP_WR_REG(ba, reg_ltm_t, reg_h00, reg_00.raw);

	if (!cfg->ltm_enable)
		return;

	lmp_0.raw = ISP_RD_REG(lmap0, reg_isp_lmap_t, lmp_0);
	lmp_0.bits.lmap_enable	= cfg->lmap_enable;
	lmp_0.bits.lmap_y_mode	= cfg->lmap_y_mode;
	lmp_0.bits.lmap_thd_l	= cfg->lmap_thd_l;
	lmp_0.bits.lmap_thd_h	= cfg->lmap_thd_h;
	ISP_WR_REG(lmap0, reg_isp_lmap_t, lmp_0, lmp_0.raw);
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
		ISP_WR_REG(lmap1, reg_isp_lmap_t, lmp_0, lmp_0.raw);
	else {
		lmp_0.bits.lmap_enable = 0;
		ISP_WR_REG(lmap1, reg_isp_lmap_t, lmp_0, lmp_0.raw);
	}

	reg_08.raw = ISP_RD_REG(ba, reg_ltm_t, reg_h08);
	reg_08.bits.ltm_be_strth_dshft	= cfg->be_strth_dshft;
	reg_08.bits.ltm_be_strth_gain	= cfg->be_strth_gain;
	ISP_WR_REG(ba, reg_ltm_t, reg_h08, reg_08.raw);

	reg_0c.raw = ISP_RD_REG(ba, reg_ltm_t, reg_h0c);
	reg_0c.bits.ltm_de_strth_dshft	= cfg->de_strth_dshft;
	reg_0c.bits.ltm_de_strth_gain	= cfg->de_strth_gain;
	ISP_WR_REG(ba, reg_ltm_t, reg_h0c, reg_0c.raw);

	//Update ltm w_h_bit and lmap w_h_bit in rawtop
	if ((g_lmp_cfg[raw_num].post_w_bit != cfg->lmap_w_bit ||
		g_lmp_cfg[raw_num].post_h_bit != cfg->lmap_h_bit) &&
		((cfg->lmap_w_bit > 2) && (cfg->lmap_h_bit > 2))) {
		union reg_ltm_h8c reg_8c;
		union reg_raw_top_le_lmap_grid_number	le_lmap_size;
		union reg_raw_top_se_lmap_grid_number	se_lmap_size;

		g_lmp_cfg[raw_num].post_w_bit = cfg->lmap_w_bit;
		g_lmp_cfg[raw_num].post_h_bit = cfg->lmap_h_bit;

		reg_8c.raw = ISP_RD_REG(ba, reg_ltm_t, reg_h8c);
		reg_8c.bits.lmap_w_bit = g_lmp_cfg[raw_num].post_w_bit;
		reg_8c.bits.lmap_h_bit = g_lmp_cfg[raw_num].post_h_bit;
		ISP_WR_REG(ba, reg_ltm_t, reg_h8c, reg_8c.raw);

		le_lmap_size.raw = ISP_RD_REG(rawtop, reg_raw_top_t, le_lmap_grid_number);
		le_lmap_size.bits.le_lmp_h_grid_size = g_lmp_cfg[raw_num].post_w_bit;
		le_lmap_size.bits.le_lmp_v_grid_size = g_lmp_cfg[raw_num].post_h_bit;
		ISP_WR_REG(rawtop, reg_raw_top_t, le_lmap_grid_number, le_lmap_size.raw);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			se_lmap_size.raw = ISP_RD_REG(rawtop, reg_raw_top_t, se_lmap_grid_number);
			se_lmap_size.bits.se_lmp_h_grid_size = g_lmp_cfg[raw_num].post_w_bit;
			se_lmap_size.bits.se_lmp_v_grid_size = g_lmp_cfg[raw_num].post_h_bit;
			ISP_WR_REG(rawtop, reg_raw_top_t, se_lmap_grid_number, se_lmap_size.raw);
		}
	}

	reg_3c.raw = ISP_RD_REG(ba, reg_ltm_t, reg_h3c);
	sel = reg_3c.bits.lut_wsel ^ 1;

	ispblk_ltm_g_lut(ctx, sel, cfg->global_lut);
	ispblk_ltm_b_lut(ctx, sel, cfg->brit_lut);
	ispblk_ltm_d_lut(ctx, sel, cfg->dark_lut);
	ISP_WR_BITS(ba, reg_ltm_t, reg_h60, hw_mem_sel, sel);

	ISP_WR_REGS_BURST(ba, reg_ltm_t, reg_h90, cfg->drc_1_cfg, cfg->drc_1_cfg.reg_h90);
	ISP_WR_REGS_BURST(ba, reg_ltm_t, reg_h14, cfg->drc_2_cfg, cfg->drc_2_cfg.reg_h14);
	ISP_WR_REGS_BURST(ba, reg_ltm_t, reg_h64, cfg->drc_3_cfg, cfg->drc_3_cfg.reg_h64);
}

void ispblk_ynr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ynr_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ynr = ctx->phys_regs[ISP_BLK_ID_YNR];
	u8 i = 0;

	if (!cfg->update)
		return;

	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_intra_0, cfg->weight_intra_0);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_intra_1, cfg->weight_intra_1);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_intra_2, cfg->weight_intra_2);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_norm_1, cfg->weight_norm_1);
	ISP_WR_REG(ynr, reg_isp_ynr_t, weight_norm_2, cfg->weight_norm_2);

	if (cfg->enable) {
		if ((cfg->out_sel == 8) || ((cfg->out_sel >= 11) && (cfg->out_sel <= 15)))
			ISP_WR_REG(ynr, reg_isp_ynr_t, out_sel, cfg->out_sel);
		else
			vi_pr(VI_ERR, "[ERR] YNR out_sel(%d) should be 8 and 11~15\n", cfg->out_sel);

		ISP_WR_REG(ynr, reg_isp_ynr_t, motion_ns_clip_max, cfg->motion_ns_clip_max);
		ISP_WR_REG(ynr, reg_isp_ynr_t, res_max, cfg->res_max);
		ISP_WR_REG(ynr, reg_isp_ynr_t, res_motion_max, cfg->res_motion_max);

		ISP_WR_REGS_BURST(ynr, reg_isp_ynr_t, ns0_luma_th_00,
					cfg->ynr_1_cfg, cfg->ynr_1_cfg.ns0_luma_th_00);

		ISP_WR_REGS_BURST(ynr, reg_isp_ynr_t, motion_lut_00,
					cfg->ynr_2_cfg, cfg->ynr_2_cfg.motion_lut_00);

		ISP_WR_REGS_BURST(ynr, reg_isp_ynr_t, alpha_gain,
					cfg->ynr_3_cfg, cfg->ynr_3_cfg.alpha_gain);

		ISP_WR_REGS_BURST(ynr, reg_isp_ynr_t, res_mot_lut_00,
					cfg->ynr_4_cfg, cfg->ynr_4_cfg.res_mot_lut_00);

		ISP_WO_BITS(ynr, reg_isp_ynr_t, index_clr, ynr_index_clr, 1);
		for (i = 0; i < 64; i++)
			ISP_WR_REG(ynr, reg_isp_ynr_t, weight_lut, cfg->weight_lut_h[i]);

	} else {
		ISP_WR_REG(ynr, reg_isp_ynr_t, out_sel, 1);
	}
}

void ispblk_cnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cnr_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];

	union reg_isp_cnr_enable reg_enable;
	union reg_isp_cnr_strength_mode reg_strength_mode;
	union reg_isp_cnr_weight_lut_inter_cnr_00 reg_weight_lut_00;
	union reg_isp_cnr_weight_lut_inter_cnr_04 reg_weight_lut_04;
	union reg_isp_cnr_weight_lut_inter_cnr_08 reg_weight_lut_08;
	union reg_isp_cnr_weight_lut_inter_cnr_12 reg_weight_lut_12;
	union reg_isp_cnr_coring_motion_lut_0 reg_coring_motion_lut_00;
	union reg_isp_cnr_coring_motion_lut_4 reg_coring_motion_lut_04;
	union reg_isp_cnr_coring_motion_lut_8 reg_coring_motion_lut_08;
	union reg_isp_cnr_coring_motion_lut_12 reg_coring_motion_lut_12;
	union reg_isp_cnr_motion_lut_0 reg_motion_lut_00;
	union reg_isp_cnr_motion_lut_4 reg_motion_lut_04;
	union reg_isp_cnr_motion_lut_8 reg_motion_lut_08;
	union reg_isp_cnr_motion_lut_12 reg_motion_lut_12;

	if (!cfg->update)
		return;

	ISP_WR_BITS(cnr, reg_isp_cnr_t, cnr_enable, cnr_enable, cfg->enable);

	if (!cfg->enable)
		return;

	reg_enable.raw = ISP_RD_REG(cnr, reg_isp_cnr_t, cnr_enable);
	reg_enable.bits.cnr_enable = cfg->enable;
	reg_enable.bits.cnr_diff_shift_val = cfg->diff_shift_val;
	reg_enable.bits.cnr_ratio = cfg->ratio;
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_enable, reg_enable.raw);

	reg_strength_mode.raw = ISP_RD_REG(cnr, reg_isp_cnr_t, cnr_strength_mode);
	reg_strength_mode.bits.cnr_strength_mode = cfg->strength_mode;
	reg_strength_mode.bits.cnr_fusion_intensity_weight = cfg->fusion_intensity_weight;
	reg_strength_mode.bits.cnr_flag_neighbor_max_weight = cfg->flag_neighbor_max_weight;
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_strength_mode, reg_strength_mode.raw);

	ISP_WR_BITS(cnr, reg_isp_cnr_t, cnr_purple_th, cnr_diff_gain, cfg->diff_gain);
	ISP_WR_BITS(cnr, reg_isp_cnr_t, cnr_purple_th, cnr_motion_enable, cfg->motion_enable);

	reg_weight_lut_00.raw = (u32)((cfg->weight_lut_inter[0] & 0x1F) |
				((cfg->weight_lut_inter[1] & 0x1F) << 8) |
				((cfg->weight_lut_inter[2] & 0x1F) << 16) |
				((cfg->weight_lut_inter[3] & 0x1F) << 24));
	reg_weight_lut_04.raw = (u32)((cfg->weight_lut_inter[4] & 0x1F) |
				((cfg->weight_lut_inter[5] & 0x1F) << 8) |
				((cfg->weight_lut_inter[6] & 0x1F) << 16) |
				((cfg->weight_lut_inter[7] & 0x1F) << 24));
	reg_weight_lut_08.raw = (u32)((cfg->weight_lut_inter[8] & 0x1F) |
				((cfg->weight_lut_inter[9] & 0x1F) << 8) |
				((cfg->weight_lut_inter[10] & 0x1F) << 16) |
				((cfg->weight_lut_inter[11] & 0x1F) << 24));
	reg_weight_lut_12.raw = (u32)((cfg->weight_lut_inter[12] & 0x1F) |
				((cfg->weight_lut_inter[13] & 0x1F) << 8) |
				((cfg->weight_lut_inter[14] & 0x1F) << 16) |
				((cfg->weight_lut_inter[15] & 0x1F) << 24));
	ISP_WR_REG(cnr, reg_isp_cnr_t, weight_lut_inter_cnr_00, reg_weight_lut_00.raw);
	ISP_WR_REG(cnr, reg_isp_cnr_t, weight_lut_inter_cnr_04, reg_weight_lut_04.raw);
	ISP_WR_REG(cnr, reg_isp_cnr_t, weight_lut_inter_cnr_08, reg_weight_lut_08.raw);
	ISP_WR_REG(cnr, reg_isp_cnr_t, weight_lut_inter_cnr_12, reg_weight_lut_12.raw);

	reg_coring_motion_lut_00.raw = (u32)((cfg->coring_motion_lut[0] & 0xFF) |
				((cfg->coring_motion_lut[1] & 0xFF) << 8) |
				((cfg->coring_motion_lut[2] & 0xFF) << 16) |
				((cfg->coring_motion_lut[3] & 0xFF) << 24));
	reg_coring_motion_lut_04.raw = (u32)((cfg->coring_motion_lut[4] & 0xFF) |
				((cfg->coring_motion_lut[5] & 0xFF) << 8) |
				((cfg->coring_motion_lut[6] & 0xFF) << 16) |
				((cfg->coring_motion_lut[7] & 0xFF) << 24));
	reg_coring_motion_lut_08.raw = (u32)((cfg->coring_motion_lut[8] & 0xFF) |
				((cfg->coring_motion_lut[9] & 0xFF) << 8) |
				((cfg->coring_motion_lut[10] & 0xFF) << 16) |
				((cfg->coring_motion_lut[11] & 0xFF) << 24));
	reg_coring_motion_lut_12.raw = (u32)((cfg->coring_motion_lut[12] & 0xFF) |
				((cfg->coring_motion_lut[13] & 0xFF) << 8) |
				((cfg->coring_motion_lut[14] & 0xFF) << 16) |
				((cfg->coring_motion_lut[15] & 0xFF) << 24));
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_coring_motion_lut_0, reg_coring_motion_lut_00.raw);
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_coring_motion_lut_4, reg_coring_motion_lut_04.raw);
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_coring_motion_lut_8, reg_coring_motion_lut_08.raw);
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_coring_motion_lut_12, reg_coring_motion_lut_12.raw);

	reg_motion_lut_00.raw = (u32)((cfg->motion_lut[0] & 0xFF) |
				((cfg->motion_lut[1] & 0xFF) << 8) |
				((cfg->motion_lut[2] & 0xFF) << 16) |
				((cfg->motion_lut[3] & 0xFF) << 24));
	reg_motion_lut_04.raw = (u32)((cfg->motion_lut[4] & 0xFF) |
				((cfg->motion_lut[5] & 0xFF) << 8) |
				((cfg->motion_lut[6] & 0xFF) << 16) |
				((cfg->motion_lut[7] & 0xFF) << 24));
	reg_motion_lut_08.raw = (u32)((cfg->motion_lut[8] & 0xFF) |
				((cfg->motion_lut[9] & 0xFF) << 8) |
				((cfg->motion_lut[10] & 0xFF) << 16) |
				((cfg->motion_lut[11] & 0xFF) << 24));
	reg_motion_lut_12.raw = (u32)((cfg->motion_lut[12] & 0xFF) |
				((cfg->motion_lut[13] & 0xFF) << 8) |
				((cfg->motion_lut[14] & 0xFF) << 16) |
				((cfg->motion_lut[15] & 0xFF) << 24));
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_motion_lut_0, reg_motion_lut_00.raw);
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_motion_lut_4, reg_motion_lut_04.raw);
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_motion_lut_8, reg_motion_lut_08.raw);
	ISP_WR_REG(cnr, reg_isp_cnr_t, cnr_motion_lut_12, reg_motion_lut_12.raw);
}

void ispblk_tnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_tnr_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t manr = ctx->phys_regs[ISP_BLK_ID_MMAP];
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];

	//union reg_isp_mmap_00 mm_00;
	union reg_isp_mmap_04 mm_04;
	union reg_isp_mmap_08 mm_08;
	union reg_isp_mmap_44 mm_44;
	union reg_isp_444_422_8 reg_8;
	union reg_isp_444_422_9 reg_9;

	if (!ctx->is_3dnr_on || !cfg->update)
		return;
#if 0
	mm_00.raw = ISP_RD_REG(manr, reg_isp_mmap_t, reg_00);
	if (!cfg->manr_enable) {
		mm_00.bits.MMAP_0_ENABLE = 0;
		mm_00.bits.MMAP_1_ENABLE = 0;
		mm_00.bits.BYPASS = 1;
	} else {
		mm_00.bits.MMAP_0_ENABLE = 1;
		mm_00.bits.MMAP_1_ENABLE = (ctx->isp_pipe_cfg[raw_num].is_hdr_on) ? 1 : 0;
		mm_00.bits.BYPASS = (ctx->isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_be) ? 1 : 0;
	}
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_00, mm_00.raw);
#endif
	mm_04.raw = ISP_RD_REG(manr, reg_isp_mmap_t, reg_04);
	mm_04.bits.mmap_0_lpf_00 = cfg->lpf[0][0];
	mm_04.bits.mmap_0_lpf_01 = cfg->lpf[0][1];
	mm_04.bits.mmap_0_lpf_02 = cfg->lpf[0][2];
	mm_04.bits.mmap_0_lpf_10 = cfg->lpf[1][0];
	mm_04.bits.mmap_0_lpf_11 = cfg->lpf[1][1];
	mm_04.bits.mmap_0_lpf_12 = cfg->lpf[1][2];
	mm_04.bits.mmap_0_lpf_20 = cfg->lpf[2][0];
	mm_04.bits.mmap_0_lpf_21 = cfg->lpf[2][1];
	mm_04.bits.mmap_0_lpf_22 = cfg->lpf[2][2];
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_04, mm_04.raw);

	mm_08.raw = ISP_RD_REG(manr, reg_isp_mmap_t, reg_08);
	mm_08.bits.mmap_0_map_gain  = cfg->map_gain;
	mm_08.bits.mmap_0_map_thd_l = cfg->map_thd_l;
	mm_08.bits.mmap_0_map_thd_h = cfg->map_thd_h;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_08, mm_08.raw);

	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_1c, mmap_0_luma_adapt_lut_slope_2, cfg->luma_adapt_lut_slope_2);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_f8, history_sel_0, cfg->history_sel_0);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_f8, history_sel_1, cfg->history_sel_1);
	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_f8, history_sel_3, cfg->history_sel_3);

	if (_is_all_online(ctx) && cfg->rgbmap_w_bit > 3) {
		vi_pr(VI_WARN, "rgbmap_w_bit(%d) need <= 3 under on the fly mode\n", cfg->rgbmap_w_bit);
		cfg->rgbmap_w_bit = cfg->rgbmap_h_bit = 3;
	} else if (cfg->rgbmap_w_bit > 5) {
		vi_pr(VI_WARN, "rgbmap_w_bit(%d) need <= 5\n", cfg->rgbmap_w_bit);
		cfg->rgbmap_w_bit = cfg->rgbmap_h_bit = 5;
	} else if (cfg->rgbmap_w_bit < 3) {
		vi_pr(VI_WARN, "rgbmap_w_bit(%d) need >= 3\n", cfg->rgbmap_w_bit);
		cfg->rgbmap_w_bit = cfg->rgbmap_h_bit = 3;
	}

	reg_8.raw = ISP_RD_REG(tnr, reg_isp_444_422_t, reg_8);
	reg_8.bits.force_dma_disable = (cfg->manr_enable == ISP_TNR_TYPE_NEW_MODE) ? 0x0 :
					((cfg->manr_enable == ISP_TNR_TYPE_OLD_MODE) ? 0x24 : 0x3f);
	reg_8.bits.uv_rounding_type_sel = cfg->uv_rounding_type_sel;
	reg_8.bits.tdnr_pixel_lp = cfg->tdnr_pixel_lp;
	reg_8.bits.tdnr_debug_sel = cfg->tdnr_debug_sel;
	ISP_WR_REG(tnr, reg_isp_444_422_t, reg_8, reg_8.raw);

	// 0: disbale, 1:old mode, 2: new mode
	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_5, tdnr_enable, cfg->manr_enable);

	ctx->is_3dnr_old2new = (ctx->isp_pipe_cfg[raw_num].tnr_mode == ISP_TNR_TYPE_OLD_MODE &&
				cfg->manr_enable == ISP_TNR_TYPE_NEW_MODE) ? true : false;
	ctx->isp_pipe_cfg[raw_num].tnr_mode = cfg->manr_enable;

	if (!cfg->manr_enable)
		return;

	reg_9.raw = ISP_RD_REG(tnr, reg_isp_444_422_t, reg_9);
	reg_9.bits.avg_mode_write  = cfg->avg_mode_write;
	reg_9.bits.drop_mode_write = cfg->drop_mode_write;
	ISP_WR_REG(tnr, reg_isp_444_422_t, reg_9, reg_9.raw);

	mm_44.raw = 0;
	mm_44.bits.mmap_med_enable = cfg->med_enable;
	mm_44.bits.mmap_med_wgt    = cfg->med_wgt;
	ISP_WR_REG(manr, reg_isp_mmap_t, reg_44, mm_44.raw);

	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_3c, motion_yv_ls_mode, cfg->mtluma_mode);
	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_5, reg_3dnr_comp_gain_enable, cfg->tdnr_comp_gain_enable);
	ISP_WR_REG(tnr, reg_isp_444_422_t, reg_80, cfg->tdnr_ee_comp_gain);

	ISP_WR_REGS_BURST(manr, reg_isp_mmap_t, reg_0c, cfg->tnr_cfg, cfg->tnr_cfg.reg_0c);
	ISP_WR_REGS_BURST(manr, reg_isp_mmap_t, reg_20, cfg->tnr_5_cfg, cfg->tnr_5_cfg.reg_20);
	ISP_WR_REGS_BURST(manr, reg_isp_mmap_t, reg_4c, cfg->tnr_1_cfg, cfg->tnr_1_cfg.reg_4c);
	ISP_WR_REGS_BURST(manr, reg_isp_mmap_t, reg_70, cfg->tnr_2_cfg, cfg->tnr_2_cfg.reg_70);
	ISP_WR_REGS_BURST(manr, reg_isp_mmap_t, reg_a0, cfg->tnr_3_cfg, cfg->tnr_3_cfg.reg_a0);
	ISP_WR_REGS_BURST(tnr, reg_isp_444_422_t, reg_13, cfg->tnr_4_cfg, cfg->tnr_4_cfg.reg_13);
	ISP_WR_REGS_BURST(tnr, reg_isp_444_422_t, reg_84, cfg->tnr_6_cfg, cfg->tnr_6_cfg.reg_84);
	ISP_WR_REGS_BURST(manr, reg_isp_mmap_t, reg_100, cfg->tnr_7_cfg, cfg->tnr_7_cfg.reg_100);

	ISP_WR_BITS(manr, reg_isp_mmap_t, reg_2c, mmap_0_mh_wgt, cfg->mh_wgt);

	if (g_w_bit[raw_num] != cfg->rgbmap_w_bit) {
		g_w_bit[raw_num] = cfg->rgbmap_w_bit;
		g_h_bit[raw_num] = cfg->rgbmap_h_bit;

		g_rgbmap_chg_pre[raw_num][0] = true;
		g_rgbmap_chg_pre[raw_num][1] = true;

		if (_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)) {
			ispblk_tnr_rgbmap_chg(ctx, raw_num, ISP_FE_CH0);
			if (ctx->is_hdr_on)
				ispblk_tnr_rgbmap_chg(ctx, raw_num, ISP_FE_CH1);

			ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit = g_w_bit[raw_num];
			ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit = g_h_bit[raw_num];
			ispblk_tnr_post_chg(ctx, raw_num);
		}
	}
}

void ispblk_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ee_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_EE_POST];
	union reg_isp_ee_00  reg_0;
	union reg_isp_ee_04  reg_4;
	union reg_isp_ee_0c  reg_c;
	union reg_isp_ee_10  reg_10;
	union reg_isp_ee_1fc reg_1fc;
	u32 i, raw;

	if (!cfg->update)
		return;

	reg_0.raw = ISP_RD_REG(ba, reg_isp_ee_t, reg_00);
	reg_0.bits.ee_enable = cfg->enable;
	reg_0.bits.ee_debug_mode = cfg->dbg_mode;
	reg_0.bits.ee_total_coring = cfg->total_coring;
	reg_0.bits.ee_total_motion_coring = cfg->total_motion_coring;
	reg_0.bits.ee_total_gain = cfg->total_gain;
	ISP_WR_REG(ba, reg_isp_ee_t, reg_00, reg_0.raw);

	if (!cfg->enable)
		return;

	reg_4.raw = ISP_RD_REG(ba, reg_isp_ee_t, reg_04);
	reg_4.bits.ee_total_oshtthrd = cfg->total_oshtthrd;
	reg_4.bits.ee_total_ushtthrd = cfg->total_ushtthrd;
	reg_4.bits.ee_pre_proc_enable = cfg->pre_proc_enable;
	ISP_WR_REG(ba, reg_isp_ee_t, reg_04, reg_4.raw);

	reg_c.raw = ISP_RD_REG(ba, reg_isp_ee_t, reg_0c);
	reg_c.bits.ee_lumaref_lpf_en = cfg->lumaref_lpf_en;
	reg_c.bits.ee_luma_coring_en = cfg->luma_coring_en;
	reg_c.bits.ee_luma_adptctrl_en = cfg->luma_adptctrl_en;
	reg_c.bits.ee_delta_adptctrl_en = cfg->delta_adptctrl_en;
	reg_c.bits.ee_delta_adptctrl_shift = cfg->delta_adptctrl_shift;
	reg_c.bits.ee_chromaref_lpf_en = cfg->chromaref_lpf_en;
	reg_c.bits.ee_chroma_adptctrl_en = cfg->chroma_adptctrl_en;
	reg_c.bits.ee_mf_core_gain = cfg->mf_core_gain;
	ISP_WR_REG(ba, reg_isp_ee_t, reg_0c, reg_c.raw);

	reg_10.raw = ISP_RD_REG(ba, reg_isp_ee_t, reg_10);
	reg_10.bits.hf_blend_wgt = cfg->hf_blend_wgt;
	reg_10.bits.mf_blend_wgt = cfg->mf_blend_wgt;
	ISP_WR_REG(ba, reg_isp_ee_t, reg_10, reg_10.raw);

	reg_1fc.raw = ISP_RD_REG(ba, reg_isp_ee_t, reg_1fc);
	reg_1fc.bits.ee_soft_clamp_enable = cfg->soft_clamp_enable;
	reg_1fc.bits.ee_upper_bound_left_diff = cfg->upper_bound_left_diff;
	reg_1fc.bits.ee_lower_bound_right_diff = cfg->lower_bound_right_diff;
	ISP_WR_REG(ba, reg_isp_ee_t, reg_1fc, reg_1fc.raw);

	for (i = 0; i < 32; i += 4) {
		raw = cfg->luma_adptctrl_lut[i] + (cfg->luma_adptctrl_lut[i + 1] << 8) +
			(cfg->luma_adptctrl_lut[i + 2] << 16) + (cfg->luma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, reg_isp_ee_t, reg_130, i, raw);

		raw = cfg->delta_adptctrl_lut[i] + (cfg->delta_adptctrl_lut[i + 1] << 8) +
			(cfg->delta_adptctrl_lut[i + 2] << 16) + (cfg->delta_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, reg_isp_ee_t, reg_154, i, raw);

		raw = cfg->chroma_adptctrl_lut[i] + (cfg->chroma_adptctrl_lut[i + 1] << 8) +
			(cfg->chroma_adptctrl_lut[i + 2] << 16) + (cfg->chroma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, reg_isp_ee_t, reg_178, i, raw);
	}

	ISP_WR_REG(ba, reg_isp_ee_t, reg_150, cfg->luma_adptctrl_lut[32]);
	ISP_WR_REG(ba, reg_isp_ee_t, reg_174, cfg->delta_adptctrl_lut[32]);
	ISP_WR_REG(ba, reg_isp_ee_t, reg_198, cfg->chroma_adptctrl_lut[32]);

	ISP_WR_REGS_BURST(ba, reg_isp_ee_t, reg_a4, cfg->ee_1_cfg, cfg->ee_1_cfg.reg_a4);
	ISP_WR_REGS_BURST(ba, reg_isp_ee_t, reg_19c, cfg->ee_2_cfg, cfg->ee_2_cfg.reg_19c);
	ISP_WR_REGS_BURST(ba, reg_isp_ee_t, reg_1c4, cfg->ee_3_cfg, cfg->ee_3_cfg.reg_1c4);
	//update this filter coffee will cause abnormal luma
	//isp no need to update
	//ISP_WR_REGS_BURST(ba, reg_isp_ee_t, REG_1DC, cfg->ee_4_cfg, cfg->ee_4_cfg.REG_1DC);
}

void ispblk_pre_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_pre_ee_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_EE_PRE];
	union reg_isp_preyee_00  reg_0;
	union reg_isp_preyee_04  reg_4;
	union reg_isp_preyee_0c  reg_c;
	union reg_isp_preyee_10  reg_10;
	union reg_isp_preyee_1fc reg_1fc;
	u32 i, raw;

	if (!cfg->update)
		return;

	reg_0.raw = ISP_RD_REG(ba, reg_isp_preyee_t, reg_00);
	reg_0.bits.ee_enable = cfg->enable;
	reg_0.bits.ee_debug_mode = cfg->dbg_mode;
	reg_0.bits.ee_total_coring = cfg->total_coring;
	reg_0.bits.ee_total_motion_coring = cfg->total_motion_coring;
	reg_0.bits.ee_total_gain = cfg->total_gain;
	ISP_WR_REG(ba, reg_isp_preyee_t, reg_00, reg_0.raw);

	if (!cfg->enable)
		return;

	reg_4.raw = ISP_RD_REG(ba, reg_isp_preyee_t, reg_04);
	reg_4.bits.ee_total_oshtthrd = cfg->total_oshtthrd;
	reg_4.bits.ee_total_ushtthrd = cfg->total_ushtthrd;
	reg_4.bits.ee_pre_proc_enable = cfg->pre_proc_enable;
	ISP_WR_REG(ba, reg_isp_preyee_t, reg_04, reg_4.raw);

	reg_c.raw = ISP_RD_REG(ba, reg_isp_preyee_t, reg_0c);
	reg_c.bits.ee_lumaref_lpf_en = cfg->lumaref_lpf_en;
	reg_c.bits.ee_luma_coring_en = cfg->luma_coring_en;
	reg_c.bits.ee_luma_adptctrl_en = cfg->luma_adptctrl_en;
	reg_c.bits.ee_delta_adptctrl_en = cfg->delta_adptctrl_en;
	reg_c.bits.ee_delta_adptctrl_shift = cfg->delta_adptctrl_shift;
	reg_c.bits.ee_chromaref_lpf_en = cfg->chromaref_lpf_en;
	reg_c.bits.ee_chroma_adptctrl_en = cfg->chroma_adptctrl_en;
	reg_c.bits.ee_mf_core_gain = cfg->mf_core_gain;
	ISP_WR_REG(ba, reg_isp_preyee_t, reg_0c, reg_c.raw);

	reg_10.raw = ISP_RD_REG(ba, reg_isp_preyee_t, reg_10);
	reg_10.bits.hf_blend_wgt = cfg->hf_blend_wgt;
	reg_10.bits.mf_blend_wgt = cfg->mf_blend_wgt;
	ISP_WR_REG(ba, reg_isp_preyee_t, reg_10, reg_10.raw);

	reg_1fc.raw = ISP_RD_REG(ba, reg_isp_preyee_t, reg_1fc);
	reg_1fc.bits.ee_soft_clamp_enable = cfg->soft_clamp_enable;
	reg_1fc.bits.ee_upper_bound_left_diff = cfg->upper_bound_left_diff;
	reg_1fc.bits.ee_lower_bound_right_diff = cfg->lower_bound_right_diff;
	ISP_WR_REG(ba, reg_isp_preyee_t, reg_1fc, reg_1fc.raw);

	for (i = 0; i < 32; i += 4) {
		raw = cfg->luma_adptctrl_lut[i] + (cfg->luma_adptctrl_lut[i + 1] << 8) +
			(cfg->luma_adptctrl_lut[i + 2] << 16) + (cfg->luma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, reg_isp_preyee_t, reg_130, i, raw);

		raw = cfg->delta_adptctrl_lut[i] + (cfg->delta_adptctrl_lut[i + 1] << 8) +
			(cfg->delta_adptctrl_lut[i + 2] << 16) + (cfg->delta_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, reg_isp_preyee_t, reg_154, i, raw);

		raw = cfg->chroma_adptctrl_lut[i] + (cfg->chroma_adptctrl_lut[i + 1] << 8) +
			(cfg->chroma_adptctrl_lut[i + 2] << 16) + (cfg->chroma_adptctrl_lut[i + 3] << 24);
		ISP_WR_REG_OFT(ba, reg_isp_preyee_t, reg_178, i, raw);
	}

	ISP_WR_REG(ba, reg_isp_preyee_t, reg_150, cfg->luma_adptctrl_lut[32]);
	ISP_WR_REG(ba, reg_isp_preyee_t, reg_174, cfg->delta_adptctrl_lut[32]);
	ISP_WR_REG(ba, reg_isp_preyee_t, reg_198, cfg->chroma_adptctrl_lut[32]);

	ISP_WR_REGS_BURST(ba, reg_isp_preyee_t, reg_a4, cfg->pre_ee_1_cfg, cfg->pre_ee_1_cfg.reg_a4);
	ISP_WR_REGS_BURST(ba, reg_isp_preyee_t, reg_19c, cfg->pre_ee_2_cfg, cfg->pre_ee_2_cfg.reg_19c);
	ISP_WR_REGS_BURST(ba, reg_isp_preyee_t, reg_1c4, cfg->pre_ee_3_cfg, cfg->pre_ee_3_cfg.reg_1c4);
	//update this filter coffee will cause abnormal luma
        //isp no need to update
	//ISP_WR_REGS_BURST(ba, reg_isp_preyee_t, reg_1dc, cfg->pre_ee_4_cfg, cfg->pre_ee_4_cfg.reg_1dc);
	ISP_WR_REGS_BURST(ba, reg_isp_preyee_t, reg_200, cfg->pre_ee_5_cfg, cfg->pre_ee_5_cfg.reg_200);
}

void ispblk_fswdr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_fswdr_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_FUSION];
	uintptr_t manr_ba = ctx->phys_regs[ISP_BLK_ID_MMAP];
	union reg_fusion_fs_ctrl_0 fs_ctrl;
	union reg_fusion_fs_ctrl_1 fs_ctrl_1;
	union reg_isp_mmap_00 mm_00;
	union reg_isp_mmap_34 mm_34;
	union reg_isp_mmap_3c mm_3c;
	union reg_isp_mmap_40 mm_40;

	if (!cfg->update)
		return;

	fs_ctrl.raw = ISP_RD_REG(ba, reg_fusion_t, fs_ctrl_0);
	fs_ctrl.bits.fs_enable			= (_is_all_online(ctx)) ? 0 : cfg->enable;
	fs_ctrl.bits.se_in_sel			= cfg->se_in_sel;
	fs_ctrl.bits.fs_mc_enable		= cfg->mc_enable;
	fs_ctrl.bits.fs_dc_mode			= cfg->dc_mode;
	fs_ctrl.bits.fs_luma_mode		= cfg->luma_mode;
	fs_ctrl.bits.fs_lmap_guide_dc_mode	= cfg->lmap_guide_dc_mode;
	fs_ctrl.bits.fs_lmap_guide_luma_mode	= cfg->lmap_guide_luma_mode;
	fs_ctrl.bits.fs_s_max			= cfg->s_max;
	ISP_WR_REG(ba, reg_fusion_t, fs_ctrl_0, fs_ctrl.raw);

	if (!cfg->enable)
		return;

	fs_ctrl_1.raw = ISP_RD_REG(ba, reg_fusion_t, fs_ctrl_1);
	fs_ctrl_1.bits.le_in_sel      = cfg->le_in_sel;
	fs_ctrl_1.bits.fs_fusion_type = cfg->fusion_type;
	fs_ctrl_1.bits.fs_fusion_lwgt = cfg->fusion_lwgt;
	ISP_WR_REG(ba, reg_fusion_t, fs_ctrl_1, fs_ctrl_1.raw);

	mm_00.raw = ISP_RD_REG(manr_ba, reg_isp_mmap_t, reg_00);
	mm_00.bits.mmap_1_enable = cfg->mmap_1_enable;
	mm_00.bits.mmap_mrg_mode = cfg->mmap_mrg_mode;
	mm_00.bits.mmap_mrg_alph = cfg->mmap_mrg_alph;
	ISP_WR_REG(manr_ba, reg_isp_mmap_t, reg_00, mm_00.raw);

	mm_34.raw = 0;
	mm_34.bits.v_thd_l = cfg->mmap_v_thd_l;
	mm_34.bits.v_thd_h = cfg->mmap_v_thd_h;
	ISP_WR_REG(manr_ba, reg_isp_mmap_t, reg_34, mm_34.raw);

	mm_40.raw = 0;
	mm_40.bits.v_wgt_min = cfg->mmap_v_wgt_min;
	mm_40.bits.v_wgt_max = cfg->mmap_v_wgt_max;
	ISP_WR_REG(manr_ba, reg_isp_mmap_t, reg_40, mm_40.raw);

	mm_3c.raw = ISP_RD_REG(manr_ba, reg_isp_mmap_t, reg_3c);
	mm_3c.bits.v_wgt_slp		= (cfg->mmap_v_wgt_slp & 0x7ffff);
	mm_3c.bits.motion_ls_mode	= cfg->motion_ls_mode;
	mm_3c.bits.motion_ls_sel	= cfg->motion_ls_sel;
	ISP_WR_REG(manr_ba, reg_isp_mmap_t, reg_3c, mm_3c.raw);

	ISP_WR_BITS(manr_ba, reg_isp_mmap_t, reg_f8, history_sel_2, cfg->history_sel_2);

	ISP_WR_REGS_BURST(ba, reg_fusion_t, fs_se_gain, cfg->fswdr_cfg, cfg->fswdr_cfg.fs_se_gain);
	ISP_WR_REGS_BURST(ba, reg_fusion_t, fs_motion_lut_in, cfg->fswdr_2_cfg, cfg->fswdr_2_cfg.fs_motion_lut_in);
	ISP_WR_REGS_BURST(ba, reg_fusion_t, fs_calib_ctrl_0, cfg->fswdr_3_cfg, cfg->fswdr_3_cfg.fs_calib_ctrl_0);
}

void ispblk_fswdr_update_rpt(
	struct isp_ctx *ctx,
	struct sop_vip_isp_fswdr_report *cfg)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_FUSION];

	cfg->cal_pix_num = ISP_RD_REG(ba, reg_fusion_t, fs_calib_out_0);
	cfg->diff_sum_r = ISP_RD_REG(ba, reg_fusion_t, fs_calib_out_1);
	cfg->diff_sum_g = ISP_RD_REG(ba, reg_fusion_t, fs_calib_out_2);
	cfg->diff_sum_b = ISP_RD_REG(ba, reg_fusion_t, fs_calib_out_3);
}

void ispblk_ldci_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ldci_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_LDCI];
	union reg_isp_ldci_enable reg_enable;

	if (!cfg->update)
		return;

	reg_enable.raw = ISP_RD_REG(ba, reg_isp_ldci_t, ldci_enable);
	reg_enable.bits.ldci_enable = cfg->enable;
	reg_enable.bits.ldci_stats_enable = cfg->stats_enable;
	reg_enable.bits.ldci_map_enable = cfg->map_enable;
	reg_enable.bits.ldci_uv_gain_enable = cfg->uv_gain_enable;
	reg_enable.bits.ldci_first_frame_enable = cfg->first_frame_enable;
	reg_enable.bits.ldci_image_size_div_by_16x12 = cfg->image_size_div_by_16x12;
	ISP_WR_REG(ba, reg_isp_ldci_t, ldci_enable, reg_enable.raw);
	ISP_WR_BITS(ba, reg_isp_ldci_t, dmi_enable, dmi_enable, cfg->enable ? 3 : 0);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ba, reg_isp_ldci_t, ldci_strength, ldci_strength, cfg->strength);

	ISP_WR_REGS_BURST(ba, reg_isp_ldci_t, ldci_luma_wgt_max,
		cfg->ldci_1_cfg, cfg->ldci_1_cfg.ldci_luma_wgt_max);
	ISP_WR_REGS_BURST(ba, reg_isp_ldci_t, ldci_blk_size_x,
		cfg->ldci_2_cfg, cfg->ldci_2_cfg.ldci_blk_size_x);
	ISP_WR_REGS_BURST(ba, reg_isp_ldci_t, ldci_idx_filter_lut_00,
		cfg->ldci_3_cfg, cfg->ldci_3_cfg.ldci_idx_filter_lut_00);
	ISP_WR_REGS_BURST(ba, reg_isp_ldci_t, ldci_luma_wgt_lut_00,
		cfg->ldci_4_cfg, cfg->ldci_4_cfg.ldci_luma_wgt_lut_00);
	ISP_WR_REGS_BURST(ba, reg_isp_ldci_t, ldci_var_filter_lut_00,
		cfg->ldci_5_cfg, cfg->ldci_5_cfg.ldci_var_filter_lut_00);
}

void ispblk_ycur_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ycur_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	u16 i = 0;

	union reg_isp_ycurv_ycur_prog_data reg_data;
	union reg_isp_ycurv_ycur_prog_ctrl prog_ctrl;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_ctrl, ycur_enable, cfg->enable);

	if (cfg->enable) {
		prog_ctrl.raw = ISP_RD_REG(ycur, reg_isp_ycurv_t, ycur_prog_ctrl);
		prog_ctrl.bits.ycur_wsel = prog_ctrl.bits.ycur_wsel ^ 1;
		prog_ctrl.bits.ycur_prog_en = 1;
		ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, prog_ctrl.raw);

		ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_st_addr, ycur_st_addr, 0);
		ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_st_addr, ycur_st_w, 1);
		ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_max, cfg->lut_256);

		for (i = 0; i < 64; i += 2) {
			reg_data.raw = 0;
			reg_data.bits.ycur_data_e = cfg->lut[i];
			reg_data.bits.ycur_data_o = cfg->lut[i + 1];
			reg_data.bits.ycur_w = 1;
			ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_data, reg_data.raw);
		}

		ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_rsel, prog_ctrl.bits.ycur_wsel);
		ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_prog_en, 0);
	}
}

void ispblk_dci_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dci_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_DCI];
	uintptr_t dci_gamma = ctx->phys_regs[ISP_BLK_ID_DCI_GAMMA];
	union reg_isp_gamma_prog_ctrl dci_gamma_ctrl;
	union reg_isp_gamma_prog_data dci_gamma_data;
	u16 i = 0;

	if (!cfg->update)
		return;

	ISP_WR_BITS(dci, reg_isp_dci_t, dci_enable, dci_enable, cfg->enable);
	ISP_WR_BITS(dci, reg_isp_dci_t, dmi_enable, dmi_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_map_enable, cfg->map_enable);
	ISP_WR_BITS(dci, reg_isp_dci_t, dci_enable, dci_hist_enable, cfg->hist_enable);
	ISP_WR_BITS(dci, reg_isp_dci_t, dci_map_enable, dci_per1sample_enable, cfg->per1sample_enable);
	ISP_WR_REG(dci, reg_isp_dci_t, dci_demo_mode, cfg->demo_mode);

	dci_gamma_ctrl.raw = ISP_RD_REG(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl);
	dci_gamma_ctrl.bits.gamma_wsel = dci_gamma_ctrl.bits.gamma_wsel ^ 1;
	dci_gamma_ctrl.bits.gamma_prog_en = 1;
	dci_gamma_ctrl.bits.gamma_prog_1to3_en = 1;
	ISP_WR_REG(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl, dci_gamma_ctrl.raw);

	for (i = 0; i < 256; i += 2) {
		dci_gamma_data.raw = 0;
		dci_gamma_data.bits.gamma_data_e = cfg->map_lut[i];
		dci_gamma_data.bits.gamma_data_o = cfg->map_lut[i + 1];
		dci_gamma_data.bits.gamma_w = 1;
		ISP_WR_REG(dci_gamma, reg_isp_gamma_t, gamma_prog_data, dci_gamma_data.raw);
	}

	ISP_WR_BITS(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_rsel, dci_gamma_ctrl.bits.gamma_wsel);
	ISP_WR_BITS(dci_gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_prog_en, 0);
}

void ispblk_dhz_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dhz_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t dhz = ctx->phys_regs[ISP_BLK_ID_DEHAZE];
	union reg_isp_dehaze_dhz_wgt dhz_wgt;

	if (!cfg->update)
		return;

	ISP_WR_BITS(dhz, reg_isp_dehaze_t, dhz_bypass, dehaze_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(dhz, reg_isp_dehaze_t, dhz_smooth, dehaze_w, cfg->strength);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, dhz_smooth, dehaze_th_smooth, cfg->th_smooth);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_1, dehaze_cum_th, cfg->cum_th);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_1, dehaze_hist_th, cfg->hist_th);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_3, dehaze_tmap_min, cfg->tmap_min);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_3, dehaze_tmap_max, cfg->tmap_max);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, dhz_bypass, dehaze_luma_lut_enable, cfg->luma_lut_enable);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, dhz_bypass, dehaze_skin_lut_enable, cfg->skin_lut_enable);

	dhz_wgt.raw = ISP_RD_REG(dhz, reg_isp_dehaze_t, dhz_wgt);
	dhz_wgt.bits.dehaze_a_luma_wgt = cfg->a_luma_wgt;
	dhz_wgt.bits.dehaze_blend_wgt = cfg->blend_wgt;
	dhz_wgt.bits.dehaze_tmap_scale = cfg->tmap_scale;
	dhz_wgt.bits.dehaze_d_wgt = cfg->d_wgt;
	ISP_WR_REG(dhz, reg_isp_dehaze_t, dhz_wgt, dhz_wgt.raw);

	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_2, dehaze_sw_dc_th, cfg->sw_dc_th);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_2, dehaze_sw_aglobal_r, cfg->sw_aglobal_r);

	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_28, dehaze_sw_aglobal_g, cfg->sw_aglobal_g);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_28, dehaze_sw_aglobal_b, cfg->sw_aglobal_b);

	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_2c, dehaze_aglobal_max, cfg->aglobal_max);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, reg_2c, dehaze_aglobal_min, cfg->aglobal_min);

	ISP_WR_BITS(dhz, reg_isp_dehaze_t, dhz_skin, dehaze_skin_cb, cfg->skin_cb);
	ISP_WR_BITS(dhz, reg_isp_dehaze_t, dhz_skin, dehaze_skin_cr, cfg->skin_cr);

	ISP_WR_REGS_BURST(dhz, reg_isp_dehaze_t, reg_9, cfg->luma_cfg, cfg->luma_cfg.luma_00);
	ISP_WR_REGS_BURST(dhz, reg_isp_dehaze_t, reg_17, cfg->skin_cfg, cfg->skin_cfg.skin_00);
	ISP_WR_REGS_BURST(dhz, reg_isp_dehaze_t, tmap_00, cfg->tmap_cfg, cfg->tmap_cfg.tmap_00);
}

void ispblk_rgbcac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_rgbcac_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t cfa = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_CFA0] : ctx->phys_regs[ISP_BLK_ID_CFA1];
	uintptr_t rgbcac = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_RGBCAC0] : ctx->phys_regs[ISP_BLK_ID_RGBCAC1];

	if (!cfg->update)
		return;

	if (!ISP_RD_BITS(cfa, reg_isp_cfa_t, reg_00, cfa_enable) && cfg->enable) {
		vi_pr(VI_WARN, "[WARN] not support cfa disable && rgbcac enable\n");
		return;
	}

	ISP_WR_BITS(rgbcac, reg_isp_rgbcac_t, rgbcac_ctrl, rgbcac_enable, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(rgbcac, reg_isp_rgbcac_t, rgbcac_ctrl, rgbcac_out_sel, cfg->out_sel);
	ISP_WR_REGS_BURST(rgbcac, reg_isp_rgbcac_t, rgbcac_purple_th,
		cfg->rgbcac_cfg, cfg->rgbcac_cfg.rgbcac_purple_th);
}

void ispblk_cac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cac_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t cac = ctx->phys_regs[ISP_BLK_ID_CNR];
	union reg_isp_cnr_purple_cb2 cnr_purple_cb2;

	if (!cfg->update)
		return;

	ISP_WR_BITS(cac, reg_isp_cnr_t, cnr_enable, pfc_enable, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(cac, reg_isp_cnr_t, cnr_enable, cnr_out_sel, cfg->out_sel);
	ISP_WR_BITS(cac, reg_isp_cnr_t, cnr_purple_th, cnr_purple_th, cfg->purple_th);
	ISP_WR_BITS(cac, reg_isp_cnr_t, cnr_purple_th, cnr_correct_strength, cfg->correct_strength);

	cnr_purple_cb2.raw = ISP_RD_REG(cac, reg_isp_cnr_t, cnr_purple_cb2);
	cnr_purple_cb2.bits.cnr_purple_cb2 = cfg->purple_cb2;
	cnr_purple_cb2.bits.cnr_purple_cr2 = cfg->purple_cr2;
	cnr_purple_cb2.bits.cnr_purple_cb3 = cfg->purple_cb3;
	cnr_purple_cb2.bits.cnr_purple_cr3 = cfg->purple_cr3;
	ISP_WR_REG(cac, reg_isp_cnr_t, cnr_purple_cb2, cnr_purple_cb2.raw);

	ISP_WR_REGS_BURST(cac, reg_isp_cnr_t, cnr_purple_cb, cfg->cac_cfg,
		cfg->cac_cfg.cnr_purple_cb);
	ISP_WR_REGS_BURST(cac, reg_isp_cnr_t, cnr_edge_scale, cfg->cac_2_cfg,
		cfg->cac_2_cfg.cnr_edge_scale);
	ISP_WR_REGS_BURST(cac, reg_isp_cnr_t, cnr_edge_scale_lut_0, cfg->cac_3_cfg,
		cfg->cac_3_cfg.cnr_edge_scale_lut_0);
}

void ispblk_lcac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_lcac_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t lcac = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_LCAC0] : ctx->phys_regs[ISP_BLK_ID_LCAC1];

	if (!cfg->update)
		return;

	ISP_WR_BITS(lcac, reg_isp_lcac_t, reg00, lcac_enable, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(lcac, reg_isp_lcac_t, reg00, lcac_out_sel, cfg->out_sel);
	ISP_WR_BITS(lcac, reg_isp_lcac_t, reg90, lcac_lti_luma_lut_32, cfg->lti_luma_lut_32);
	ISP_WR_BITS(lcac, reg_isp_lcac_t, reg90, lcac_fcf_luma_lut_32, cfg->fcf_luma_lut_32);

	ISP_WR_REGS_BURST(lcac, reg_isp_lcac_t, reg04, cfg->lcac_cfg, cfg->lcac_cfg.reg04);
	ISP_WR_REGS_BURST(lcac, reg_isp_lcac_t, reg50, cfg->lcac_2_cfg, cfg->lcac_2_cfg.reg50);
	ISP_WR_REGS_BURST(lcac, reg_isp_lcac_t, reg70, cfg->lcac_3_cfg, cfg->lcac_3_cfg.reg70);
}

void ispblk_csc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_csc_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t csc = ctx->phys_regs[ISP_BLK_ID_CSC];
	union reg_isp_csc_4 csc_4;
	union reg_isp_csc_5 csc_5;
	union reg_isp_csc_6 csc_6;
	union reg_isp_csc_7 csc_7;
	union reg_isp_csc_8 csc_8;
	union reg_isp_csc_9 csc_9;

	if (!cfg->update)
		return;

	ISP_WR_BITS(csc, reg_isp_csc_t, reg_0, csc_enable, cfg->enable);
	if (!cfg->enable)
		return;

	csc_4.raw = 0;
	csc_4.bits.coeff_00 = cfg->coeff[0] & 0x3fff;
	csc_4.bits.coeff_01 = cfg->coeff[1] & 0x3fff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_4, csc_4.raw);

	csc_5.raw = 0;
	csc_5.bits.coeff_02 = cfg->coeff[2] & 0x3fff;
	csc_5.bits.coeff_10 = cfg->coeff[3] & 0x3fff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_5, csc_5.raw);

	csc_6.raw = 0;
	csc_6.bits.coeff_11 = cfg->coeff[4] & 0x3fff;
	csc_6.bits.coeff_12 = cfg->coeff[5] & 0x3fff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_6, csc_6.raw);

	csc_7.raw = 0;
	csc_7.bits.coeff_20 = cfg->coeff[6] & 0x3fff;
	csc_7.bits.coeff_21 = cfg->coeff[7] & 0x3fff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_7, csc_7.raw);

	csc_8.raw = 0;
	csc_8.bits.coeff_22 = cfg->coeff[8] & 0x3fff;
	csc_8.bits.offset_0 = cfg->offset[0] & 0x7ff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_8, csc_8.raw);

	csc_9.raw = 0;
	csc_9.bits.offset_1 = cfg->offset[1] & 0x7ff;
	csc_9.bits.offset_2 = cfg->offset[2] & 0x7ff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_9, csc_9.raw);
}

void ispblk_rgbir_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_rgbir_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t rgbir = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_RGBIR0] : ctx->phys_regs[ISP_BLK_ID_RGBIR1];

	if (!cfg->update)
		return;

	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_ctrl, rgbir2rggb_enable, cfg->enable);
	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rbgir_wdma_ctl, rgbir2rggb_dma_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(rgbir, reg_isp_rgbir_t, rgbir_ctrl, rgbir2rggb_comp_enable, cfg->comp_enable);
	ISP_WR_REGS_BURST(rgbir, reg_isp_rgbir_t, rgbir_gain_offset_1, cfg->rgbir_cfg, cfg->rgbir_cfg.gain_offset_1);
}

void ispblk_dpc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dpc_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t dpc = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_DPC0] : ctx->phys_regs[ISP_BLK_ID_DPC1];
	union reg_isp_dpc_2 dpc_2;
	u16 i;

	if (!cfg->update)
		return;

	ISP_WR_BITS(dpc, reg_isp_dpc_t, dpc_2, dpc_enable, cfg->enable);
	if (!cfg->enable)
		return;

	dpc_2.raw = ISP_RD_REG(dpc, reg_isp_dpc_t, dpc_2);
	dpc_2.bits.dpc_enable = cfg->enable;
	dpc_2.bits.dpc_dynamicbpc_enable = cfg->enable ? cfg->dynamicbpc_enable : 0;
	dpc_2.bits.dpc_staticbpc_enable = cfg->enable ? cfg->staticbpc_enable : 0;
	dpc_2.bits.dpc_cluster_size = cfg->cluster_size;
	ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_2, dpc_2.raw);

	if (cfg->staticbpc_enable && (cfg->bp_cnt > 0) && (cfg->bp_cnt < 4096)) {
		ISP_WR_BITS(dpc, reg_isp_dpc_t, dpc_17, dpc_mem_prog_mode, 1);
		ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_mem_st_addr, 0x80000000);

		for (i = 0; i < cfg->bp_cnt; i++)
			ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_mem_w0, 0x80000000 | cfg->bp_tbl[i]);

		// write 1 3-fff-fff to end
		ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_mem_w0, 0x83ffffff);
		ISP_WR_BITS(dpc, reg_isp_dpc_t, dpc_17, dpc_mem_prog_mode, 0);
	}
	ISP_WR_REGS_BURST(dpc, reg_isp_dpc_t, dpc_3, cfg->dpc_cfg, cfg->dpc_cfg.dpc_3);
}

void ispblk_ae_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ae_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ba = 0;
	u32 raw;

	if (!cfg->update)
		return;

	switch (cfg->inst) {
	case 0: // LE
		ba = ctx->phys_regs[ISP_BLK_ID_AE_HIST0];
		break;
	case 1: // SE
		ba = ctx->phys_regs[ISP_BLK_ID_AE_HIST1];
		break;
	default:
		vi_pr(VI_ERR, "Wrong ae inst\n");
		return;
	}

	ISP_WR_BITS(ba, reg_isp_ae_hist_t, sts_ae0_hist_enable, sts_ae0_hist_enable, cfg->ae_enable);
	ISP_WR_BITS(ba, reg_isp_ae_hist_t, dmi_enable, dmi_enable, cfg->ae_enable);

	if (!cfg->ae_enable)
		return;

	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_offsetx, cfg->ae_offsetx);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_offsety, cfg->ae_offsety);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_numxm1, cfg->ae_numx - 1);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_numym1, cfg->ae_numy - 1);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_width, cfg->ae_width);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_height, cfg->ae_height);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_sts_div, cfg->ae_sts_div);

	raw = cfg->ae_face_offset_x[0] + (cfg->ae_face_offset_y[0] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face0_location, raw);

	raw = cfg->ae_face_size_minus1_x[0] + (cfg->ae_face_size_minus1_y[0] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face0_size, raw);

	raw = cfg->ae_face_offset_x[1] + (cfg->ae_face_offset_y[1] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face1_location, raw);

	raw = cfg->ae_face_size_minus1_x[1] + (cfg->ae_face_size_minus1_y[1] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face1_size, raw);

	raw = cfg->ae_face_offset_x[2] + (cfg->ae_face_offset_y[2] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face2_location, raw);

	raw = cfg->ae_face_size_minus1_x[2] + (cfg->ae_face_size_minus1_y[2] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face2_size, raw);

	raw = cfg->ae_face_offset_x[3] + (cfg->ae_face_offset_y[3] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face3_location, raw);

	raw = cfg->ae_face_size_minus1_x[3] + (cfg->ae_face_size_minus1_y[3] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face3_size, raw);

	ISP_WR_REGS_BURST(ba, reg_isp_ae_hist_t, ae_face0_enable, cfg->ae_cfg, cfg->ae_cfg.ae_face0_enable);
	ISP_WR_REGS_BURST(ba, reg_isp_ae_hist_t, ae_wgt_00, cfg->ae_2_cfg, cfg->ae_2_cfg.ae_wgt_00);
}

void ispblk_ge_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ge_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t dpc = (cfg->inst == 0) ?
			ctx->phys_regs[ISP_BLK_ID_DPC0] : ctx->phys_regs[ISP_BLK_ID_DPC1];
	if (!cfg->update)
		return;

	ISP_WR_BITS(dpc, reg_isp_dpc_t, dpc_2, ge_enable, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_REGS_BURST(dpc, reg_isp_dpc_t, dpc_10, cfg->ge_cfg, cfg->ge_cfg.dpc_10);
}

void ispblk_af_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_af_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_AF];

	union reg_isp_af_enables af_enables;
	union reg_isp_af_low_pass_horizon low_pass_horizon;
	union reg_isp_af_high_pass_horizon_0 high_pass_horizon_0;
	union reg_isp_af_high_pass_horizon_1 high_pass_horizon_1;
	union reg_isp_af_high_pass_vertical_0 high_pass_vertical_0;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ba, reg_isp_af_t, kickoff, af_enable, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(ba, reg_isp_af_t, dmi_enable, dmi_enable, cfg->enable);
	ISP_WR_REG(ba, reg_isp_af_t, bypass, !cfg->enable);

	af_enables.raw = ISP_RD_REG(ba, reg_isp_af_t, enables);
	af_enables.bits.af_dpc_enable = cfg->dpc_enable;
	af_enables.bits.af_hlc_enable = cfg->hlc_enable;
	ISP_WR_REG(ba, reg_isp_af_t, enables, af_enables.raw);

	ISP_WR_BITS(ba, reg_isp_af_t, square_enable, af_square_enable, cfg->square_enable);
	ISP_WR_BITS(ba, reg_isp_af_t, outshift, af_outshift, cfg->outshift);
	ISP_WR_BITS(ba, reg_isp_af_t, num_gapline, af_num_gapline, cfg->num_gapline);

	// 8 <= offset_x <= img_width - 8
	ISP_WR_BITS(ba, reg_isp_af_t, offset_x, af_offset_x, cfg->offsetx);
	// 2 <= offset_y <= img_height - 2
	ISP_WR_BITS(ba, reg_isp_af_t, offset_x, af_offset_y, cfg->offsety);

	ISP_WR_REG(ba, reg_isp_af_t, block_width, cfg->block_width);	// (ctx->img_width - 16) / numx
	ISP_WR_REG(ba, reg_isp_af_t, block_height, cfg->block_height);	// (ctx->img_height - 4) / numy
	ISP_WR_REG(ba, reg_isp_af_t, block_num_x, cfg->block_numx);		// should fixed to 17
	ISP_WR_REG(ba, reg_isp_af_t, block_num_y, cfg->block_numy);		// should fixed to 15

	ISP_WR_REG(ba, reg_isp_af_t, hor_low_pass_value_shift, cfg->h_low_pass_value_shift);
	ISP_WR_REG(ba, reg_isp_af_t, offset_horizontal_0, cfg->h_corning_offset_0);
	ISP_WR_REG(ba, reg_isp_af_t, offset_horizontal_1, cfg->h_corning_offset_1);
	ISP_WR_REG(ba, reg_isp_af_t, offset_vertical, cfg->v_corning_offset);
	ISP_WR_REG(ba, reg_isp_af_t, high_y_thre, cfg->high_luma_threshold);

	low_pass_horizon.raw = 0;
	low_pass_horizon.bits.af_low_pass_horizon_0 = cfg->h_low_pass_coef[0];
	low_pass_horizon.bits.af_low_pass_horizon_1 = cfg->h_low_pass_coef[1];
	low_pass_horizon.bits.af_low_pass_horizon_2 = cfg->h_low_pass_coef[2];
	low_pass_horizon.bits.af_low_pass_horizon_3 = cfg->h_low_pass_coef[3];
	low_pass_horizon.bits.af_low_pass_horizon_4 = cfg->h_low_pass_coef[4];
	ISP_WR_REG(ba, reg_isp_af_t, low_pass_horizon, low_pass_horizon.raw);

	high_pass_horizon_0.raw = 0;
	high_pass_horizon_0.bits.af_high_pass_horizon_0_0 = cfg->h_high_pass_coef_0[0];
	high_pass_horizon_0.bits.af_high_pass_horizon_0_1 = cfg->h_high_pass_coef_0[1];
	high_pass_horizon_0.bits.af_high_pass_horizon_0_2 = cfg->h_high_pass_coef_0[2];
	high_pass_horizon_0.bits.af_high_pass_horizon_0_3 = cfg->h_high_pass_coef_0[3];
	high_pass_horizon_0.bits.af_high_pass_horizon_0_4 = cfg->h_high_pass_coef_0[4];
	ISP_WR_REG(ba, reg_isp_af_t, high_pass_horizon_0, high_pass_horizon_0.raw);

	high_pass_horizon_1.raw = 0;
	high_pass_horizon_1.bits.af_high_pass_horizon_1_0 = cfg->h_high_pass_coef_1[0];
	high_pass_horizon_1.bits.af_high_pass_horizon_1_1 = cfg->h_high_pass_coef_1[1];
	high_pass_horizon_1.bits.af_high_pass_horizon_1_2 = cfg->h_high_pass_coef_1[2];
	high_pass_horizon_1.bits.af_high_pass_horizon_1_3 = cfg->h_high_pass_coef_1[3];
	high_pass_horizon_1.bits.af_high_pass_horizon_1_4 = cfg->h_high_pass_coef_1[4];
	ISP_WR_REG(ba, reg_isp_af_t, high_pass_horizon_1, high_pass_horizon_1.raw);

	high_pass_vertical_0.raw = 0;
	high_pass_vertical_0.bits.af_high_pass_vertical_0_0 = cfg->v_high_pass_coef[0];
	high_pass_vertical_0.bits.af_high_pass_vertical_0_1 = cfg->v_high_pass_coef[1];
	high_pass_vertical_0.bits.af_high_pass_vertical_0_2 = cfg->v_high_pass_coef[2];
	ISP_WR_REG(ba, reg_isp_af_t, high_pass_vertical_0, high_pass_vertical_0.raw);

	ISP_WR_BITS(ba, reg_isp_af_t, th_low, af_th_low, cfg->th_low);
	ISP_WR_BITS(ba, reg_isp_af_t, th_low, af_th_high, cfg->th_high);
	ISP_WR_BITS(ba, reg_isp_af_t, gain_low, af_gain_low, cfg->gain_low);
	ISP_WR_BITS(ba, reg_isp_af_t, gain_low, af_gain_high, cfg->gain_high);
	ISP_WR_BITS(ba, reg_isp_af_t, slop_low, af_slop_low, cfg->slop_low);
	ISP_WR_BITS(ba, reg_isp_af_t, slop_low, af_slop_high, cfg->slop_high);
}

void ispblk_hist_v_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_hist_v_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t hist_v = ctx->phys_regs[ISP_BLK_ID_HIST_EDGE_V];

	if (!cfg->update)
		return;

	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, ip_config, hist_edge_v_enable, cfg->enable);
	if (!cfg->enable)
		return;

	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, ip_config, hist_edge_v_luma_mode, cfg->luma_mode);
	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, dmi_enable, dmi_enable, cfg->enable);
	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, hist_edge_v_offsetx, hist_edge_v_offsetx, cfg->offset_x);
	ISP_WR_BITS(hist_v, reg_isp_hist_edge_v_t, hist_edge_v_offsety, hist_edge_v_offsety, cfg->offset_y);
}

void ispblk_gms_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_gms_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_GMS];
	u32 img_width = ctx->isp_pipe_cfg[raw_num].crop.w;
	u32 img_height = ctx->isp_pipe_cfg[raw_num].crop.h;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ba, reg_isp_gms_t, gms_enable, gms_enable, cfg->enable);
	ISP_WR_BITS(ba, reg_isp_gms_t, dmi_enable, dmi_enable, cfg->enable);

	if (!cfg->enable)
		return;

	if (cfg->x_section_size > 1024 || cfg->y_section_size > 512 ||
		cfg->x_gap < 10 || cfg->y_gap < 10) {
		vi_pr(VI_WARN, "[WARN] GMS tuning x_gap(%d), y_gap(%d), x_sec_size(%d), y_sec_size(%d)\n",
			cfg->x_gap, cfg->y_gap, cfg->x_section_size, cfg->y_section_size);
		return;
	}

	if (((cfg->x_section_size & 1) != 0) || ((cfg->y_section_size & 1) != 0) ||
		((cfg->x_section_size & 3) != 2) || ((cfg->y_section_size & 3) != 2)) {
		vi_pr(VI_WARN, "[WARN] GMS tuning x_sec_size(%d) and y_sec_size(%d) should be even and 4n+2\n",
			cfg->x_section_size, cfg->y_section_size);
		return;
	}

	if (((cfg->x_section_size + 1) * 3 + cfg->offset_x + cfg->x_gap * 2 + 4) >= img_width) {
		vi_pr(VI_WARN, "[WARN] GMS tuning x_sec_size(%d), ofst_x(%d), x_gap(%d), img_size(%d)\n",
				cfg->x_section_size, cfg->offset_x, cfg->x_gap, img_width);
		return;
	}

	if (((cfg->y_section_size + 1) * 3 + cfg->offset_y + cfg->y_gap * 2) > img_height) {
		vi_pr(VI_WARN, "[WARN] GMS tuning y_sec_size(%d), ofst_y(%d), y_gap(%d), img_size(%d)\n",
				cfg->y_section_size, cfg->offset_y, cfg->y_gap, img_height);
		return;
	}

	ISP_WR_REG(ba, reg_isp_gms_t, gms_start_x, cfg->offset_x);
	ISP_WR_REG(ba, reg_isp_gms_t, gms_start_y, cfg->offset_y);

	ISP_WR_REG(ba, reg_isp_gms_t, gms_x_sizem1, cfg->x_section_size - 1);
	ISP_WR_REG(ba, reg_isp_gms_t, gms_y_sizem1, cfg->y_section_size - 1);

	ISP_WR_REG(ba, reg_isp_gms_t, gms_x_gap, cfg->x_gap);
	ISP_WR_REG(ba, reg_isp_gms_t, gms_y_gap, cfg->y_gap);

	ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_GMS, 0);
}

void ispblk_mono_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_mono_config *cfg,
	const enum sop_isp_raw raw_num)
{
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];

	if (!cfg->update)
		return;

	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_5, force_mono_enable, cfg->force_mono_enable);
}

#if 0
void ispblk_lscr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_lscr_config *cfg,
	const enum sop_isp_raw raw_num)
{
}
void ispblk_preproc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_preproc_config *cfg,
	const enum sop_isp_raw raw_num)
{
}
#endif

#include <vi_raw_dump.h>
#include <vip/vi_drv.h>
#include <linux/comm_errno.h>
#include <vb.h>

struct isp_buffer *isp_byr[ISP_PRERAW_MAX], *isp_byr_se[ISP_PRERAW_MAX];

struct isp_queue raw_dump_b_q[ISP_PRERAW_MAX], raw_dump_b_se_q[ISP_PRERAW_MAX],
	raw_dump_b_dq[ISP_PRERAW_MAX], raw_dump_b_se_dq[ISP_PRERAW_MAX];

static struct raw_dump_work g_raw_dump_work;

void _isp_fe_be_raw_dump_cfg(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u8 chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u8 trigger = false;

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		trigger = (vdev->pre_be_frm_num[raw_num][ISP_BE_CH0] ==
				vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);
	} else
		trigger = true;

	if (trigger) {
		struct isp_buffer *b = NULL, *b_se = NULL;
		struct isp_queue *fe_out_q = &raw_dump_b_q[raw_num];
		struct isp_queue *fe_out_q_se = &raw_dump_b_se_q[raw_num];
		u32 dmaid_le, dmaid_se;

		dmaid_le = csibdg_dma_find_hwid(raw_num, ISP_FE_CH0);
		dmaid_se = csibdg_dma_find_hwid(raw_num, ISP_FE_CH1);

		vi_pr(VI_DBG, "fe_be raw_dump cfg start\n");

		b = isp_buf_next(fe_out_q);
		if (b == NULL) {
			vi_pr(VI_ERR, "Pre_fe_%d LE raw_dump outbuf is empty\n", raw_num);
			return;
		}

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			b_se = isp_buf_next(fe_out_q_se);
			if (b_se == NULL) {
				vi_pr(VI_ERR, "Pre_fe_%d SE raw_dump outbuf is empty\n", raw_num);
				return;
			}
		}

		if (ctx->isp_pipe_cfg[raw_num].rawdump_crop.w &&
			ctx->isp_pipe_cfg[raw_num].rawdump_crop.h) {
			vi_pr(VI_DBG, "rawdump_crop w(%d), h(%d)\n",
				ctx->isp_pipe_cfg[raw_num].rawdump_crop.w,
				ctx->isp_pipe_cfg[raw_num].rawdump_crop.h);
			vi_pr(VI_DBG, "b->crop_le x(%d), y(%d), w(%d), h(%d)\n",
				b->crop_le.x, b->crop_le.y, b->crop_le.w, b->crop_le.h);
			ispblk_csibdg_wdma_crop_config(ctx, raw_num, b->crop_le, 1);
		} else {
			ispblk_csibdg_wdma_crop_config(ctx, raw_num, b->crop_le, 0);
		}

		ispblk_dma_setaddr(ctx, raw_num, dmaid_le, b->addr);
		ispblk_csidbg_dma_wr_en(ctx, raw_num, ISP_FE_CH0, 1);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			ispblk_dma_setaddr(ctx, raw_num, dmaid_se, b_se->addr);
			ispblk_csidbg_dma_wr_en(ctx, raw_num, ISP_FE_CH1, 1);
		}

		atomic_set(&vdev->isp_raw_dump_en[raw_num], 2);
	}
}

int isp_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_raw_blk *dump)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b;
	int ret = 0;
	u8 raw_num = dump[0].raw_dump.raw_num;

	if (ctx->isp_pipe_cfg[raw_num].is_tile && raw_num != ISP_PRERAW0) {
		vi_pr(VI_ERR, "tile mode raw_num must be ISP_PRERAW0\n");
		return -EINVAL;
	}

	if (isp_byr[raw_num] != 0 || isp_byr_se[raw_num] != 0) {
		vi_pr(VI_ERR, "Release buffer first, call put pipe dump\n");
		dump[0].is_b_not_rls = true;
		ret = -EINVAL;
		return ret;
	}

	b = vmalloc(sizeof(*b));
	memset(b, 0, sizeof(*b));
	b->addr = dump[0].raw_dump.phy_addr;
	b->raw_num = raw_num;
	b->crop_le = ctx->isp_pipe_cfg[raw_num].rawdump_crop;
	vi_pr(VI_DBG, "raw_num=%d enque raw_dump le\n", raw_num);
	vi_pr(VI_DBG, "b->crop_le x(%d), y(%d), w(%d), h(%d)\n",
		b->crop_le.x, b->crop_le.y, b->crop_le.w, b->crop_le.h);
	isp_buf_queue(&raw_dump_b_q[b->raw_num], b);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		b = vmalloc(sizeof(*b));
		memset(b, 0, sizeof(*b));
		b->addr = dump[1].raw_dump.phy_addr;
		b->crop_se = ctx->isp_pipe_cfg[raw_num].rawdump_crop_se;
		b->raw_num = raw_num;
		vi_pr(VI_DBG, "enque raw_dump se\n");
		vi_pr(VI_DBG, "b->crop_se x(%d), y(%d), w(%d), h(%d)\n",
			b->crop_se.x, b->crop_se.y, b->crop_se.w, b->crop_se.h);
		isp_buf_queue(&raw_dump_b_se_q[b->raw_num], b);
	}

	atomic_set(&vdev->isp_raw_dump_en[raw_num], 1);
	if (ctx->isp_pipe_cfg[raw_num].is_tile)
		atomic_set(&vdev->isp_raw_dump_en[raw_num + 1], 1);

	ret = wait_event_interruptible_timeout(
		vdev->isp_int_wait_q[raw_num], vdev->isp_int_flag[raw_num] != 0,
		msecs_to_jiffies(dump->time_out));

	vdev->isp_int_flag[raw_num] = 0;
	if (!ret) {
		vi_pr(VI_ERR, "vi get raw timeout(%d)\n", dump[0].time_out);
		dump[0].is_timeout = true;
		isp_byr[raw_num] = isp_byr_se[raw_num] = NULL;
		ret = -ETIME;
		return ret;
	}

	if (signal_pending(current)) {
		dump[0].is_sig_int = true;
		isp_byr[raw_num] = isp_byr_se[raw_num] = NULL;
		ret = -ERESTARTSYS;
		return ret;
	}

	isp_byr[raw_num] = isp_buf_remove(&raw_dump_b_dq[raw_num]);
	if (isp_byr[raw_num] == NULL) {
		vi_pr(VI_ERR, "Get raw_le dump buffer time_out(%d)\n", dump[0].time_out);
		dump[0].is_timeout = true;
		isp_byr[raw_num] = isp_byr_se[raw_num] = NULL;
		ret = -ETIME;
		return ret;
	}

	dump[0].src_w		= isp_byr[raw_num]->crop_le.w;
	dump[0].src_h		= isp_byr[raw_num]->crop_le.h;
	dump[0].crop_x		= isp_byr[raw_num]->crop_le.x;
	dump[0].crop_y		= isp_byr[raw_num]->crop_le.y;
	dump[0].frm_num		= isp_byr[raw_num]->frm_num;

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		isp_byr_se[raw_num] = isp_buf_remove(&raw_dump_b_se_dq[raw_num]);
		if (isp_byr_se[raw_num] == NULL) {
			vi_pr(VI_ERR, "Get raw_se dump buffer time_out(%d)\n", dump[1].time_out);
			dump[0].is_timeout = true;
			isp_byr[raw_num] = isp_byr_se[raw_num] = NULL;
			ret = -ETIME;
			return ret;
		}

		dump[1].src_w		= isp_byr_se[raw_num]->crop_se.w;
		dump[1].src_h		= isp_byr_se[raw_num]->crop_se.h;
		dump[1].crop_x		= isp_byr_se[raw_num]->crop_se.x;
		dump[1].crop_y		= isp_byr_se[raw_num]->crop_se.y;
		dump[1].frm_num		= isp_byr_se[raw_num]->frm_num;
	}

	return 0;
}

void free_isp_byr(u8 raw_num)
{
	if (isp_byr[raw_num]) {
		vfree(isp_byr[raw_num]);
		isp_byr[raw_num] = NULL;
	}

	if (isp_byr_se[raw_num]) {
		vfree(isp_byr_se[raw_num]);
		isp_byr_se[raw_num] = NULL;
	}
}

int isp_start_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_smooth_raw_param *pstSmoothRawParam)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL, *b_se = NULL;
	int ret = 0;
	u8 j, raw_num, frm_num;
	struct vi_rect rawdump_crop;

	raw_num = pstSmoothRawParam->raw_num;
	frm_num = pstSmoothRawParam->frm_num;

	if (raw_num > ISP_PRERAW_MAX - 1)
		return ERR_VI_INVALID_PIPEID;

	rawdump_crop.x = pstSmoothRawParam->raw_blk->crop_x;
	rawdump_crop.y = pstSmoothRawParam->raw_blk->crop_y;
	rawdump_crop.w = pstSmoothRawParam->raw_blk->src_w;
	rawdump_crop.h = pstSmoothRawParam->raw_blk->src_h;
	ctx->isp_pipe_cfg[raw_num].rawdump_crop = rawdump_crop;
	ctx->isp_pipe_cfg[raw_num].rawdump_crop_se = rawdump_crop;

	for (j = 0; j < frm_num; j++) {
		b = vzalloc(sizeof(*b));
		if (b == NULL) {
			vi_pr(VI_ERR, "roi_le_%d vmalloc size(%zu) fail\n", j, sizeof(*b));
			ret = -1;
			goto err;
		}
		b->addr = (pstSmoothRawParam->raw_blk + j)->raw_dump.phy_addr;
		b->raw_num = raw_num;
		b->crop_le = ctx->isp_pipe_cfg[raw_num].rawdump_crop;
		vi_pr(VI_DBG, "raw_num=%d enque raw_dump le 0x%llx, crop x(%d), y(%d), w(%d), h(%d)\n",
			raw_num, b->addr,
			b->crop_le.x, b->crop_le.y, b->crop_le.w, b->crop_le.h);
		isp_buf_queue(&raw_dump_b_q[raw_num], b);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			b_se = vzalloc(sizeof(*b_se));
			if (b_se == NULL) {
				vi_pr(VI_ERR, "roi_se_%d vmalloc size(%zu) fail\n", j, sizeof(*b_se));
				ret = -1;
				goto err;
			}
			j++;
			b_se->addr = (pstSmoothRawParam->raw_blk + j)->raw_dump.phy_addr;
			b_se->raw_num = raw_num;
			b_se->crop_se = ctx->isp_pipe_cfg[raw_num].rawdump_crop_se;
			vi_pr(VI_DBG, "raw_num=%d enque raw_dump se 0x%llx, crop x(%d), y(%d), w(%d), h(%d)\n",
				raw_num, b_se->addr,
				b_se->crop_se.x, b_se->crop_se.y, b_se->crop_se.w, b_se->crop_se.h);
			isp_buf_queue(&raw_dump_b_se_q[raw_num], b_se);
		}
	}

	atomic_set(&vdev->isp_smooth_raw_dump_en[raw_num], 1);
	atomic_set(&vdev->isp_raw_dump_en[raw_num], 1);
	if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		atomic_set(&vdev->isp_smooth_raw_dump_en[raw_num + 1], 1);
		atomic_set(&vdev->isp_raw_dump_en[raw_num + 1], 1);
	}

err:
	if (ret == -1) {
		while ((b = isp_buf_remove(&raw_dump_b_q[raw_num])) != NULL)
			vfree(b);
		while ((b_se = isp_buf_remove(&raw_dump_b_se_q[raw_num])) != NULL)
			vfree(b_se);
	}

	return ret;
}

int isp_stop_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_smooth_raw_param *pstSmoothRawParam)
{
	u8 raw_num;

	raw_num = pstSmoothRawParam->raw_num;

	if (raw_num > ISP_PRERAW_MAX - 1)
		return ERR_VI_INVALID_PIPEID;

	atomic_set(&vdev->isp_smooth_raw_dump_en[raw_num], 2);

	return 0;
}

int isp_get_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_raw_blk *dump)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL, *b_se = NULL;
	int ret = 0;
	u8 raw_num = dump[0].raw_dump.raw_num;

	ret = wait_event_interruptible_timeout(
		vdev->isp_int_wait_q[raw_num], vdev->isp_int_flag[raw_num] != 0,
		msecs_to_jiffies(dump->time_out));

	vdev->isp_int_flag[raw_num] = 0;
	if (!ret) {
		vi_pr(VI_ERR, "vi get raw timeout(%d)\n", dump[0].time_out);
		dump[0].is_timeout = true;
		ret = -ETIME;
		return ret;
	}

	if (signal_pending(current)) {
		dump[0].is_sig_int = true;
		ret = -ERESTARTSYS;
		return ret;
	}

	b = isp_buf_remove(&raw_dump_b_dq[raw_num]);
	if (b == NULL) {
		vi_pr(VI_ERR, "Get raw_le dump buffer time_out(%d)\n", dump[0].time_out);
		vfree(b);
		dump[0].is_timeout = true;
		ret = -ETIME;
		return ret;
	}

	memset(&dump[0], 0, sizeof(struct sop_vip_isp_raw_blk));
	vi_pr(VI_DBG, "raw_le phy_addr=0x%llx byr_size=%d frm_num=%d\n",
		b->addr, b->byr_size, b->frm_num);

	dump[0].src_w             = b->crop_le.w;
	dump[0].src_h             = b->crop_le.h;
	dump[0].crop_x            = b->crop_le.x;
	dump[0].crop_y            = b->crop_le.y;
	dump[0].frm_num           = b->frm_num;
	dump[0].raw_dump.size     = b->byr_size;
	dump[0].raw_dump.phy_addr = b->addr;
	vfree(b);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		b_se = isp_buf_remove(&raw_dump_b_se_dq[raw_num]);
		if (b_se == NULL) {
			vi_pr(VI_ERR, "Get raw_se dump buffer time_out(%d)\n", dump[1].time_out);
			vfree(b_se);
			dump[1].is_timeout = true;
			ret = -ETIME;
			return ret;
		}

		memset(&dump[1], 0, sizeof(struct sop_vip_isp_raw_blk));
		vi_pr(VI_DBG, "raw_se phy_addr=0x%llx byr_size=%d frm_num=%d\n",
			b_se->addr, b_se->byr_size, b_se->frm_num);

		dump[1].src_w				= b_se->crop_se.w;
		dump[1].src_h				= b_se->crop_se.h;
		dump[1].crop_x				= b_se->crop_se.x;
		dump[1].crop_y				= b_se->crop_se.y;
		dump[1].frm_num				= b_se->frm_num;
		dump[1].raw_dump.size		= b_se->byr_size;
		dump[1].raw_dump.phy_addr	= b_se->addr;
		vfree(b_se);
	}

	return 0;
}

int isp_put_smooth_raw_dump(struct sop_vi_dev *vdev, struct sop_vip_isp_raw_blk *dump)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL, *b_se = NULL;
	u8 raw_num;

	raw_num = dump[0].raw_dump.raw_num;

	b = vzalloc(sizeof(*b));
	if (b == NULL) {
		vi_pr(VI_ERR, "le vmalloc size(%zu) fail\n", sizeof(*b));
		vfree(b);
		return -1;
	}
	b->addr = dump[0].raw_dump.phy_addr;
	b->raw_num = raw_num;
	b->crop_le = ctx->isp_pipe_cfg[raw_num].rawdump_crop;
	vi_pr(VI_DBG, "raw_num=%d enque raw_dump le 0x%llx, crop x(%d), y(%d), w(%d), h(%d)\n",
		raw_num, b->addr,
		b->crop_le.x, b->crop_le.y, b->crop_le.w, b->crop_le.h);
	isp_buf_queue(&raw_dump_b_q[b->raw_num], b);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		b_se = vzalloc(sizeof(*b_se));
		if (b_se == NULL) {
			vi_pr(VI_ERR, "se vmalloc size(%zu) fail\n", sizeof(*b_se));
			vfree(b_se);
			return -1;
		}
		b_se->addr = dump[1].raw_dump.phy_addr;
		b_se->raw_num = raw_num;
		b_se->crop_se = ctx->isp_pipe_cfg[raw_num].rawdump_crop_se;
		vi_pr(VI_DBG, "raw_num=%d enque raw_dump se 0x%llx, crop x(%d), y(%d), w(%d), h(%d)\n",
			raw_num, b_se->addr,
			b_se->crop_se.x, b_se->crop_se.y, b_se->crop_se.w, b_se->crop_se.h);
		isp_buf_queue(&raw_dump_b_se_q[b->raw_num], b_se);
	}

	return 0;
}

void _isp_raw_dump_chk(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u32 frm_num)
{
	switch (atomic_read(&vdev->isp_smooth_raw_dump_en[raw_num])) {
	default:
	case 0:
	{
		vi_pr(VI_DBG, "wake up wait_q\n");

		vdev->isp_int_flag[raw_num] = 1;
		wake_up_interruptible(&vdev->isp_int_wait_q[raw_num]);

		atomic_set(&vdev->isp_raw_dump_en[raw_num], 0);
		if (vdev->ctx.isp_pipe_cfg[raw_num].is_tile)
			atomic_set(&vdev->isp_raw_dump_en[raw_num + 1], 0);
		return;
	}
	case 1:
	{
		vi_pr(VI_DBG, "wake up wait_q smooth frm=%d\n", frm_num);

		vdev->isp_int_flag[raw_num] = 1;
		wake_up_interruptible(&vdev->isp_int_wait_q[raw_num]);

		atomic_set(&vdev->isp_raw_dump_en[raw_num], 1);
		if (vdev->ctx.isp_pipe_cfg[raw_num].is_tile)
			atomic_set(&vdev->isp_raw_dump_en[raw_num + 1], 1);
		return;
	}
	case 2:
	{
		struct isp_buffer *b = NULL;

		vi_pr(VI_DBG, "stop dump smooth\n");

		while ((b = isp_buf_remove(&raw_dump_b_dq[raw_num])) != NULL)
			vfree(b);
		while ((b = isp_buf_remove(&raw_dump_b_se_dq[raw_num])) != NULL)
			vfree(b);
		while ((b = isp_buf_remove(&raw_dump_b_q[raw_num])) != NULL)
			vfree(b);
		while ((b = isp_buf_remove(&raw_dump_b_se_q[raw_num])) != NULL)
			vfree(b);

		atomic_set(&vdev->isp_raw_dump_en[raw_num], 0);
		atomic_set(&vdev->isp_smooth_raw_dump_en[raw_num], 0);
		if (vdev->ctx.isp_pipe_cfg[raw_num].is_tile) {
			atomic_set(&vdev->isp_raw_dump_en[raw_num + 1], 0);
			atomic_set(&vdev->isp_smooth_raw_dump_en[raw_num + 1], 0);
		}
		return;
	}
	}
}

static void raw_dump_wq_handler(struct work_struct *worker)
{
	struct raw_dump_work *dump_work = container_of((void *)worker, struct raw_dump_work, worker);
	struct isp_buffer *buf = NULL;
	vb_blk blk = 0;

	do {
		buf = isp_buf_remove(&dump_work->raw_dump_vb_q);
		if (buf) {
			if (buf->is_ext == INTERNAL_BUFFER) {
				vi_pr(VI_ERR, "buf is invalid\n");
				continue;
			}

			//for ai bnr buf->addr is invalid
			blk = vb_phys_addr2handle(buf->addr);
			if (blk != VB_INVALID_HANDLE)
				vb_release_block(blk);
			kfree(buf);
		}

	} while (buf);
}

void isp_raw_dump_vb_queue(struct isp_buffer *buf)
{
	isp_buf_queue(&g_raw_dump_work.raw_dump_vb_q, buf);

	schedule_work(&g_raw_dump_work.worker);
}

void isp_raw_dump_init(void)
{
	INIT_LIST_HEAD(&g_raw_dump_work.raw_dump_vb_q.rdy_queue);
	g_raw_dump_work.raw_dump_vb_q.num_rdy = 0;
	g_raw_dump_work.raw_dump_vb_q.raw_num = 0;
	INIT_WORK(&g_raw_dump_work.worker, raw_dump_wq_handler);
}

void isp_raw_dump_deinit(void)
{
	raw_dump_wq_handler(&g_raw_dump_work.worker);
	cancel_work_sync(&g_raw_dump_work.worker);
}

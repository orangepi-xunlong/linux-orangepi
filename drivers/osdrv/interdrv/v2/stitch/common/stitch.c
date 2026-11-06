#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/timer.h>

#include <asm/div64.h>

#include <base_ctx.h>
#include <common.h>
#include <comm_buffer.h>
#include <linux/delay.h>

#include <base_cb.h>
#include <base_common.h>
#include <vb.h>
#include <vbq.h>
#include "stitch.h"
#include "stitch_core.h"
#include "stitch_reg_cfg.h"
#include "sys.h"
#include "vb.h"
#include "stitch_proc.h"

#define STITCH_SIZE_ZERO 0
#undef BIT
#undef BITMASK
#define BIT(nr)               (1U << (nr))
#define BITMASK(msb, lsb)     ((2U << (msb))-(1U << (lsb)))

#define SRC_NUM_2WAY 2
#define SRC_NUM_4WAY 4

#define IDLE_TIMEOUT_MS      10000
#define EOF_WAIT_TIMEOUT_MS  1000

//stitch evt
#define CTX_EVENT_WKUP       0x01

extern int gStitchDumpReg;

struct stitch_jobs_ctx {
	struct vb_jobs_t ins[STITCH_MAX_SRC_NUM];
	struct vb_jobs_t out;
	bool init;
};

static struct stitch_jobs_ctx stitch_jobs[STITCH_MAX_GRP_NUM];
static struct vb_s *stitch_chn_vbq[STITCH_MAX_GRP_NUM];

struct __stitch_ctx *stitch_ctx[STITCH_MAX_GRP_NUM] = { [0 ... STITCH_MAX_GRP_NUM-1] = NULL};
struct stitch_handler_ctx evt_hdl_ctx;

//Get Available Grp lock
static struct mutex stitch_get_grp_lock;
static bool stitch_grp_used[STITCH_MAX_GRP_NUM] = {[0 ... STITCH_MAX_GRP_NUM-1] = false};
stitch_grp grp_rec[STITCH_MAX_GRP_NUM] = {[0 ... STITCH_MAX_GRP_NUM-1] = STITCH_INVALID_GRP};
static int stitch_grp_num = 0;

struct stitch_hw_cfg_param {
	enum stitch_wgt_mode wgt_mode;
	enum stitch_data_src data_src;
	struct stitch_bj_size_param bj_size[STITCH_MAX_SRC_NUM -1];//crop2bj12, crop2bj23,crop2bj34
	struct stitch_rdma_size_param rdma_size[STITCH_MAX_SRC_NUM -1][3];
	struct stitch_wdma_nbld_size_param wdma_nbld_size[STITCH_MAX_SRC_NUM -1][2];
	struct stitch_wdma_bld_size_param wdma_bld_size[STITCH_MAX_SRC_NUM -1][2];
	struct stitch_crop2bj_param crop2bj[STITCH_MAX_SRC_NUM -1];
	struct stitch_dma_ctl rdma_l[STITCH_MAX_SRC_NUM -1][3];
	struct stitch_dma_ctl rdma_r[STITCH_MAX_SRC_NUM -1][3];
	struct stitch_dma_ctl rdma_alpha[STITCH_MAX_SRC_NUM -1];
	struct stitch_dma_ctl rdma_beta[STITCH_MAX_SRC_NUM -1];
	struct stitch_dma_ctl wdma_l[STITCH_MAX_SRC_NUM -1][3];//img12/img23/img34 combine
	struct stitch_dma_ctl wdma_r[STITCH_MAX_SRC_NUM -1][3];
	struct stitch_dma_ctl wdma_bld[STITCH_MAX_SRC_NUM -1][3];
};

static struct stitch_hw_cfg_param *stitch_hw_cfg_obj;
static struct timespec64 hw_ts[2];
static struct timespec64 sw_ts[2];

static void stitch_record_hw_time_start(stitch_grp grp_id)
{
	ktime_get_ts64(&hw_ts[0]);
	stitch_ctx[grp_id]->job.ts_start = hw_ts[0];
}

static void stitch_record_hw_time_end(stitch_grp grp_id)
{
	ktime_get_ts64(&hw_ts[1]);
	stitch_ctx[grp_id]->job.ts_end = hw_ts[1];
}

static unsigned int stitch_record_hw_time(stitch_grp grp_id)
{
	TRACE_STITCH(DBG_INFO, "start time(%llu)\n", (unsigned long long)(stitch_ctx[grp_id]->job.ts_start.tv_sec * USEC_PER_SEC + stitch_ctx[grp_id]->job.ts_start.tv_nsec / NSEC_PER_USEC));
	TRACE_STITCH(DBG_INFO, "end time(%llu)\n", (unsigned long long)(stitch_ctx[grp_id]->job.ts_end.tv_sec * USEC_PER_SEC + stitch_ctx[grp_id]->job.ts_end.tv_nsec / NSEC_PER_USEC));
	return get_diff_in_us(stitch_ctx[grp_id]->job.ts_start, stitch_ctx[grp_id]->job.ts_end);
}

static inline unsigned int stitch_modified_bits_by_value(unsigned int orig, unsigned int value, unsigned int msb, unsigned int lsb)
{
	unsigned int bitmask = BITMASK(msb, lsb);

	orig &= ~bitmask;
	return (orig | ((value << lsb) & bitmask));
}

static inline unsigned int stitch_get_bits_from_value(unsigned int value, unsigned int msb, unsigned int lsb)
{
	return ((value & BITMASK(msb, lsb)) >> lsb);
}

static unsigned char stitch_handler_is_idle(void)
{
	int i;

	for (i = 0; i < STITCH_MAX_GRP_NUM; i++)
		if (stitch_ctx[i] && stitch_ctx[i]->is_created && stitch_ctx[i]->is_started)
			return false;

	return true;
}

static void release_stitch_waitq(mmf_chn_s chn, enum chn_type_e chn_type)
{
	vb_blk blk;

	if (chn_type == CHN_TYPE_OUT)
		blk = base_mod_jobs_waitq_pop(&stitch_jobs[chn.dev_id].out);
	else
		blk = base_mod_jobs_waitq_pop(&stitch_jobs[chn.dev_id].ins[chn.chn_id]);

	if (blk != VB_INVALID_HANDLE)
		vb_release_block(blk);
}

static void release_stitch_jobq_in(stitch_grp grp_id)
{
	int i;
	vb_blk blk_src;

	if (stitch_ctx[grp_id]->src_num) {
		for (i = stitch_ctx[grp_id]->src_num - 1; i >= 0; i--) {
			while (!base_mod_jobs_waitq_empty(&stitch_jobs[grp_id].ins[i])) {
				blk_src = base_mod_jobs_waitq_pop(&stitch_jobs[grp_id].ins[i]);
				TRACE_STITCH(DBG_WARN, "grp[%d]: in [%d] waitq pop\n", grp_id, i);
				if (blk_src != VB_INVALID_HANDLE)
					vb_release_block(blk_src);
			}
			while (!base_mod_jobs_workq_empty(&stitch_jobs[grp_id].ins[i])) {
				blk_src = base_mod_jobs_workq_pop(&stitch_jobs[grp_id].ins[i]);
				TRACE_STITCH(DBG_WARN, "grp[%d]: in [%d] workq pop\n", grp_id, i);
				if (blk_src != VB_INVALID_HANDLE)
					vb_release_block(blk_src);
			}
		}
	}
}

static void release_stitch_jobq_out(stitch_grp grp_id)
{
	vb_blk blk_out;

	while (!base_mod_jobs_waitq_empty(&stitch_jobs[grp_id].out)) {
		blk_out = base_mod_jobs_waitq_pop(&stitch_jobs[grp_id].out);
		TRACE_STITCH(DBG_WARN, "grp[%d]: out waitq pop\n", grp_id);
		if (blk_out != VB_INVALID_HANDLE)
			vb_release_block(blk_out);
	}
	while (!base_mod_jobs_workq_empty(&stitch_jobs[grp_id].out)) {
		blk_out = base_mod_jobs_workq_pop(&stitch_jobs[grp_id].out);
		TRACE_STITCH(DBG_WARN, "grp[%d]: out workq pop\n", grp_id);
		if (blk_out != VB_INVALID_HANDLE)
			vb_release_block(blk_out);
	}
}

static void release_stitch_jobq(stitch_grp grp_id)
{
	if (stitch_jobs[grp_id].init) {
		release_stitch_jobq_in(grp_id);
		release_stitch_jobq_out(grp_id);
	}
}

static int trans_hw_crop2bj(stitch_grp grp_id, stitch_src_idx src_id)
{
	int bj_id;
	unsigned int bld_size;
	TRACE_STITCH(DBG_DEBUG, "evt_ctx->src_id(%d)\n", src_id);

	if (src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = src_id - 1;
	bld_size = stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[bj_id] + 1;

	switch (src_id) {
	case STITCH_SRC_ID_1:
	case STITCH_SRC_ID_3:
		stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height  = stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1;
		if (src_id == STITCH_SRC_ID_3) {
			stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width =
				stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[bj_id-1]-1;
		} else {
			stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width   = stitch_ctx[grp_id]->src_attr.size[bj_id].width - 1;
		}
		stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height = stitch_ctx[grp_id]->src_attr.size[bj_id + 1].height - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width  = stitch_ctx[grp_id]->src_attr.size[bj_id + 1].width - 1;

		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_str  = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_str = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_end  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_end = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height;

		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_str  = stitch_ctx[grp_id]->src_attr.bd_attr.bd_lx[bj_id];
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_str = stitch_ctx[grp_id]->src_attr.bd_attr.bd_lx[bj_id + 1];
		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width - stitch_ctx[grp_id]->src_attr.bd_attr.bd_rx[bj_id];
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width - stitch_ctx[grp_id]->src_attr.bd_attr.bd_rx[bj_id + 1];

		TRACE_STITCH(DBG_DEBUG, "crop2bj lft w h:%d %d, rht w h:%d %d\n"
			, stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width, stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height
			, stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width, stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height);
		TRACE_STITCH(DBG_DEBUG, "crop2bj crop info lft[%d %d], rht[%d %d]\n"
			, stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_str, stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end
			, stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_str, stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end);
		break;
	case STITCH_SRC_ID_2:// use top1 full ovlap blending
		stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height  = stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width   = bld_size - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height = stitch_ctx[grp_id]->src_attr.size[bj_id + 1].height - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width  = bld_size - 1;

		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_str  = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_str = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_end  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_end = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height;

		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_str  = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_str = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width;

		TRACE_STITCH(DBG_DEBUG, "crop2bj lft w h:%d %d, rht w h:%d %d\n"
			, stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width, stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height
			, stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width, stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height);
		TRACE_STITCH(DBG_DEBUG, "crop2bj crop info lft[%d %d], rht[%d %d]\n"
			, stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_str, stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end
			, stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_str, stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end);
		break;
	default:
		TRACE_STITCH(DBG_DEBUG, "evt_ctx->src_id(%d)\n", src_id);
		break;
	}

	return 0;
}

static int trans_hw_bj_size_sel(stitch_grp grp_id, enum stitch_src_id src_id, int bj_id, unsigned int *l_nbldsize, unsigned int *bld_size, unsigned int *r_nbldsize)
{
	int ret = 0;

	switch (src_id) {
	case STITCH_SRC_ID_1:
		*l_nbldsize = stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[bj_id] - stitch_ctx[grp_id]->src_attr.bd_attr.bd_lx[bj_id];
		*r_nbldsize = stitch_ctx[grp_id]->tmp_chn_size[bj_id].width - *bld_size - *l_nbldsize;
		break;
	case STITCH_SRC_ID_2:
		*l_nbldsize = *r_nbldsize = 0;
		break;
	case STITCH_SRC_ID_3:
		*l_nbldsize = stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[bj_id] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[bj_id - 1] - 1;
		*r_nbldsize = stitch_ctx[grp_id]->chn_attr.size.width - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[bj_id] - 1;
		break;
	default:
		TRACE_STITCH(DBG_DEBUG, "src_id(%d)\n", src_id);
		ret = -1;
		break;
	}
	return ret;
}

static int trans_hw_bj_size(stitch_grp grp_id, stitch_src_idx src_id)
{
	int bj_id;
	unsigned int bld_size = 0;
	unsigned int l_nbldsize = 0, r_nbldsize = 0;
	TRACE_STITCH(DBG_DEBUG, "grp_id(%d) src_id(%d)\n", grp_id, src_id);

	if (src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = src_id - 1;
	bld_size = stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[bj_id] + 1;

	trans_hw_bj_size_sel(grp_id, src_id, bj_id, &l_nbldsize, &bld_size, &r_nbldsize);

	if (l_nbldsize > 0) {
		if (src_id == STITCH_SRC_ID_2)
			stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_left = 0;
		else
			stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_left = stitch_ctx[grp_id]->src_attr.bd_attr.bd_lx[bj_id];
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_left = l_nbldsize - 1;
	} else {
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_left = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_left = STITCH_MAGIC_SIZE;
	}

	if (bld_size > 0) {
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_left = l_nbldsize;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_left = bld_size + l_nbldsize -1;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_right = 0;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_right =  bld_size -1;
	} else {
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_left = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_left = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_right = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_right = STITCH_MAGIC_SIZE;
	}

	if (r_nbldsize > 0) {
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_right = bld_size;
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_right = bld_size + r_nbldsize -1;
	} else {
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_right = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_right = STITCH_MAGIC_SIZE;
	}

	TRACE_STITCH(DBG_DEBUG, "bj_id(%d) lnb-b-rnb[%d-%d-%d] [%d-%d] [%d-%d] [%d-%d] [%d-%d]\n", bj_id, l_nbldsize, bld_size, r_nbldsize
		, stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_left, stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_left
		, stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_left, stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_left
		, stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_right, stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_right
		, stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_right, stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_right);

	return 0;
}

static int trans_hw_rdma_size(stitch_grp grp_id, stitch_src_idx src_id)
{
	int i;
	int bj_id;
	unsigned int bld_size;
	TRACE_STITCH(DBG_DEBUG, "src_id(%d)\n", src_id);

	if (src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = src_id - 1;
	bld_size = stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[bj_id] + 1;

	for (i = 0; i < 2; i++) {
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_h_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_h_str_r_right = 0;

		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_str_r_right = 0;

		if (stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444
			|| stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx[grp_id]->src_attr.fmt_in == PIXEL_FORMAT_BGR_888_PLANAR
			|| stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_422) {
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_left  = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1);
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_right = (stitch_ctx[grp_id]->src_attr.size[bj_id + 1].height - 1);
		} else {
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_left  = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1) / (i+1);
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_right = (stitch_ctx[grp_id]->src_attr.size[bj_id + 1].height - 1) / (i+1);
		}

		if (stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444 || stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_RGB_888_PLANAR  || stitch_ctx[grp_id]->src_attr.fmt_in == PIXEL_FORMAT_BGR_888_PLANAR) {
			if (bj_id == 1) {
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left   = bld_size -1;
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right  = bld_size -1;
			} else if (bj_id == 2) {
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left   = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width;
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right  = (stitch_ctx[grp_id]->src_attr.size[bj_id + 1].width - 1);
			} else {
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left   = (stitch_ctx[grp_id]->src_attr.size[bj_id].width - 1);
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right  = (stitch_ctx[grp_id]->src_attr.size[bj_id + 1].width - 1);
			}
			//stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end;
			//stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end;
		} else {
			if (bj_id == 1) {
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left   = (bld_size -1) / (i+1);
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right  = (bld_size -1) / (i+1);
			} else if (bj_id == 2) {
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left   = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width / (i+1);
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right  = (stitch_ctx[grp_id]->src_attr.size[bj_id + 1].width - 1) / (i+1);
			} else {
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left   = (stitch_ctx[grp_id]->src_attr.size[bj_id].width - 1) / (i+1);
				stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right  = (stitch_ctx[grp_id]->src_attr.size[bj_id + 1].width - 1) / (i+1);
			}
			//stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end  / (i+1);
			//stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end / (i+1);
		}

		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_left  = stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left;
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_right = stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right;

		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_h_end_r_left  = stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_left;
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_h_end_r_right = stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_right;
	}

	if (stitch_ctx[grp_id]->op_attr.wgt_mode == STITCH_WGT_YUV_SHARE) {//mode 0, wgt
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_end_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_end;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_str_r_right = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_end_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_end;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_end_r_left  = (bld_size - 1) >> 1;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_str_r_right = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_end_r_right =  (bld_size - 1) >> 1;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_height_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_width_r_left   =  (bld_size - 1) >> 1;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_height_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_width_r_right  =  (bld_size - 1) >> 1;
	} else if (stitch_ctx[grp_id]->op_attr.wgt_mode == STITCH_WGT_UV_SHARE) {//mode 1, wgt
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_end_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_end;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_str_r_right = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_end_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_end;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_end_r_left  = bld_size - 1;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_str_r_right = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_end_r_right = bld_size - 1;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_height_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_width_r_left   = bld_size - 1;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_height_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_width_r_right  = bld_size - 1;
	} else {
		TRACE_STITCH(DBG_ERR, "do not support this wgt_mode(%d).\n\n", stitch_ctx[grp_id]->op_attr.wgt_mode);
		return -1;
	}

	return 0;
}

static int trans_hw_wdma_size(stitch_grp grp_id, stitch_src_idx src_id)
{
	int i;
	int bj_id;
	unsigned int bld_size = 0;
	unsigned int l_nbldsize = 0, r_nbldsize = 0;
	TRACE_STITCH(DBG_DEBUG, "evt_ctx->src_id(%d)\n", src_id);

	if (src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = src_id - 1;
	bld_size = stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[bj_id] + 1;

	trans_hw_bj_size_sel(grp_id, src_id, bj_id, &l_nbldsize, &bld_size, &r_nbldsize);

	for (i = 0; i < 2; i++) {
		stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_h_str_w_bld = 0;
		stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_w_str_w_bld = 0;

		stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_str_w_left  = 0;
		stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_str_w_right = 0;

		stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_str_w_left  = 0;
		stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_str_w_right = 0;

		if (stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444
			|| stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_BGR_888_PLANAR
			|| stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_422) {
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_h_end_w_bld = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1);
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_height_w_bld = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_end_w_left  = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_end_w_right = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_height_w_left  = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_height_w_right = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1);
		} else {
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_h_end_w_bld = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1) / (i+1);
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_height_w_bld = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1) / (i+1);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_end_w_left  = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1) / (i+1);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_end_w_right = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1) / (i+1);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_height_w_left  = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1) / (i+1);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_height_w_right = (stitch_ctx[grp_id]->src_attr.size[bj_id].height - 1) / (i+1);
		}

		if (stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444 || stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_BGR_888_PLANAR) {
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_w_end_w_bld = (bld_size) ? (bld_size - 1) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_width_w_bld  = (bld_size) ? (bld_size - 1) : (STITCH_SIZE_ZERO);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_left    = (l_nbldsize) ? (l_nbldsize - 1) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_end_w_left   = (l_nbldsize) ? (l_nbldsize - 1) : (STITCH_SIZE_ZERO);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_right   = (r_nbldsize) ? (r_nbldsize - 1) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_end_w_right  = (r_nbldsize) ? (r_nbldsize - 1) : (STITCH_SIZE_ZERO);
		} else {
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_w_end_w_bld = (bld_size) ? ((bld_size - 1) / (i+1)) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_width_w_bld  = (bld_size) ? ((bld_size - 1) / (i+1)) : (STITCH_SIZE_ZERO);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_left    = (l_nbldsize) ? ((l_nbldsize - 1) / (i+1)) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_end_w_left   = (l_nbldsize) ? ((l_nbldsize - 1) / (i+1)) : (STITCH_SIZE_ZERO);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_right   = (r_nbldsize) ? ((r_nbldsize - 1) / (i+1)) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_end_w_right  = (r_nbldsize) ? ((r_nbldsize - 1) / (i+1)) : (STITCH_SIZE_ZERO);
		}
	}

	return 0;
}

static int trans_hw_op(stitch_grp grp_id, stitch_src_idx src_id)
{
	if (src_id == STITCH_SRC_ID_1 || src_id == STITCH_SRC_ID_3) {
		stitch_hw_cfg_obj->wgt_mode = stitch_ctx[grp_id]->op_attr.wgt_mode;
		stitch_hw_cfg_obj->data_src = stitch_ctx[grp_id]->op_attr.data_src;
	}

	return 0;
}

static int trans_hw_dma_id_cfg(stitch_grp grp_id, stitch_src_idx src_id)
{
	int i;
	int bj_id;
	TRACE_STITCH(DBG_DEBUG, "src_id(%d)\n", src_id);

	if (src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = src_id - 1;

	switch (src_id) {
	case STITCH_SRC_ID_1:
	case STITCH_SRC_ID_3:
		for (i = 0; i < 3; i++) {
			stitch_hw_cfg_obj->rdma_l[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_RDMA_STCH_LEFTY0);
			stitch_hw_cfg_obj->rdma_r[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_RDMA_STCH_RIGHTY0);

			stitch_hw_cfg_obj->wdma_l[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_WDMA_STCH_LEFTY0);
			stitch_hw_cfg_obj->wdma_r[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_WDMA_STCH_RIGHTY0);
			stitch_hw_cfg_obj->wdma_bld[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_WDMA_STCH_BLDY0);
		}
		stitch_hw_cfg_obj->rdma_alpha[bj_id].dma_id = ID_RDMA_STCH_ALPHA0;
		stitch_hw_cfg_obj->rdma_beta[bj_id].dma_id = ID_RDMA_STCH_BETA0;
		break;
	case STITCH_SRC_ID_2:
		for (i = 0; i < 3; i++) {
			stitch_hw_cfg_obj->rdma_l[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_RDMA_STCH_LEFTY1);
			stitch_hw_cfg_obj->rdma_r[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_RDMA_STCH_RIGHTY1);

			stitch_hw_cfg_obj->wdma_bld[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_WDMA_STCH_BLDY1);
		}
		stitch_hw_cfg_obj->rdma_alpha[bj_id].dma_id = ID_RDMA_STCH_ALPHA1;
		stitch_hw_cfg_obj->rdma_beta[bj_id].dma_id = ID_RDMA_STCH_BETA1;
		break;
	default:
		TRACE_STITCH(DBG_DEBUG, "src_id(%d)\n", src_id);
		break;
	}

	return 0;
}

static int trans_hw_wgt_cfg(stitch_grp grp_id, stitch_src_idx src_id)
{
	int bj_id;
	struct stitch_dma_ctl *rdma_alpha, *rdma_beta;
	TRACE_STITCH(DBG_DEBUG, "evt_ctx->src_id(%d)\n", src_id);

	if (src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = src_id - 1;
	rdma_alpha = &stitch_hw_cfg_obj->rdma_alpha[bj_id];
	rdma_beta  = &stitch_hw_cfg_obj->rdma_beta[bj_id];

	rdma_alpha->addr = stitch_ctx[grp_id]->wgt_attr.phy_addr_wgt[bj_id][0];
	rdma_beta->addr  = stitch_ctx[grp_id]->wgt_attr.phy_addr_wgt[bj_id][1];

	rdma_alpha->height = stitch_ctx[grp_id]->wgt_attr.size_wgt[bj_id].height;

	rdma_alpha->width  = (stitch_ctx[grp_id]->op_attr.wgt_mode == STITCH_WGT_UV_SHARE)
		? stitch_ctx[grp_id]->wgt_attr.size_wgt[bj_id].width << 1
		: stitch_ctx[grp_id]->wgt_attr.size_wgt[bj_id].width;

	rdma_beta->height  = rdma_alpha->height;
	rdma_beta->width   = rdma_alpha->width;

	rdma_alpha->stride = ALIGN(rdma_alpha->width, STITCH_STRIDE_ALIGN);
	rdma_beta->stride  = ALIGN(rdma_beta->width,  STITCH_STRIDE_ALIGN);

	return 0;
}


static int stitch_trans_to_hw_setting(stitch_grp grp_id, stitch_src_idx src_id)
{
	if (trans_hw_crop2bj(grp_id, src_id))
		return -1;
	if (trans_hw_bj_size(grp_id, src_id))
		return -1;
	if (trans_hw_rdma_size(grp_id, src_id))
		return -1;
	if (trans_hw_wdma_size(grp_id, src_id))
		return -1;
	if (trans_hw_op(grp_id, src_id))
		return -1;
	if (trans_hw_dma_id_cfg(grp_id, src_id))
		return -1;
	if (trans_hw_wgt_cfg(grp_id, src_id))
		return -1;

	return 0;
}

static void stitch_hw_trig(void)
{
	if (gStitchDumpReg > 0) {
		stitch_dump_register();
		gStitchDumpReg--;
	}
	stitch_enable();
}

static void stitch_hw_cfg_reg(unsigned char top_id, stitch_grp grp_id, stitch_src_idx src_id)
{
	int i;
	unsigned char bj_id = 0;
	unsigned char disable_wdma_l = false, disable_wdma_r = false, disable_bld = false;
	unsigned char disable_rdma_l = false, disable_rdma_r = false, disable_rdma_l_r = false, disable_rdma_wgt = false;

	if (top_id)
		bj_id = 1;
	else {
		if (src_id)
			bj_id = src_id - 1;
	}

	if (stitch_ctx[grp_id]->param_update) {
		stitch_bj_image_size_cfg(top_id, &stitch_hw_cfg_obj->bj_size[bj_id]);
		stitch_rdma_image_size_cfg(top_id, stitch_hw_cfg_obj->rdma_size[bj_id]);
		stitch_wdma_image_size_cfg(top_id, stitch_hw_cfg_obj->wdma_nbld_size[bj_id]
			, stitch_hw_cfg_obj->wdma_bld_size[bj_id]);
		stitch_crop2bj_cfg(top_id, &stitch_hw_cfg_obj->crop2bj[bj_id]);

		stitch_mode_sel((unsigned char)stitch_hw_cfg_obj->wgt_mode);

		if (stitch_hw_cfg_obj->rdma_alpha[bj_id].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->rdma_alpha[bj_id]);

		if (stitch_hw_cfg_obj->rdma_beta[bj_id].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->rdma_beta[bj_id]);

		if (!stitch_hw_cfg_obj->rdma_alpha[bj_id].width && !stitch_hw_cfg_obj->rdma_beta[bj_id].width)
			disable_rdma_wgt = true;
	}

	for (i = 0; i < NUM_OF_PLANES; i++) {
		if (!stitch_hw_cfg_obj->rdma_l[bj_id][i].width)
			disable_rdma_l = true;
		if (!stitch_hw_cfg_obj->rdma_r[bj_id][i].width)
			disable_rdma_r = true;
		if (!stitch_hw_cfg_obj->rdma_l[bj_id][i].width && !stitch_hw_cfg_obj->rdma_r[bj_id][i].width)// src img from vpss
			disable_rdma_l_r = true;
		else {
			stitch_dma_cfg(&stitch_hw_cfg_obj->rdma_l[bj_id][i]);
			stitch_dma_cfg(&stitch_hw_cfg_obj->rdma_r[bj_id][i]);
		}
#if 0 // as bmtest
		stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_l[0][i]);
		if (stitch_hw_cfg_obj->wdma_l[0][i].width == 0)
			disable_wdma_l = true;

		stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_r[0][i]);
		if (stitch_hw_cfg_obj->wdma_r[0][i].width == 0)
			disable_wdma_r = true;

		stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_bld[0][i]);
		if (stitch_hw_cfg_obj->wdma_bld[0][i].width == 0)
			disable_bld = true;
#else
		if (stitch_hw_cfg_obj->wdma_l[bj_id][i].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_l[bj_id][i]);
		else
			disable_wdma_l = true;

		if (stitch_hw_cfg_obj->wdma_r[bj_id][i].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_r[bj_id][i]);
		else
			disable_wdma_r = true;

		if (stitch_hw_cfg_obj->wdma_bld[bj_id][i].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_bld[bj_id][i]);
		else
			disable_bld = true;
#endif
	}

	stitch_valid_param(top_id);

	if (disable_rdma_wgt)
		stitch_disable_wgt(top_id);

	if (disable_rdma_l)
		stitch_disable_left_rdma(top_id);

	if (disable_rdma_r)
		stitch_disable_right_rdma(top_id);

	if (disable_wdma_l)
		stitch_disable_left_wdma(top_id);

	if (disable_wdma_r)
		stitch_disable_right_wdma(top_id);

	if (disable_bld)
		stitch_disable_bld(top_id);

	if (stitch_ctx[grp_id]->src_attr.fmt_in == PIXEL_FORMAT_YUV_PLANAR_444 || stitch_ctx[grp_id]->src_attr.fmt_in == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx[grp_id]->src_attr.fmt_in == PIXEL_FORMAT_BGR_888_PLANAR)
		stitch_r_uv_bypass(top_id);
	else if (stitch_ctx[grp_id]->src_attr.fmt_in == PIXEL_FORMAT_YUV_PLANAR_422)
		stitch_r_uv_half_bypass(top_id);

	if (stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444 || stitch_ctx[grp_id]->src_attr.fmt_in == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx[grp_id]->src_attr.fmt_in == PIXEL_FORMAT_BGR_888_PLANAR)
		stitch_w_uv_bypass(top_id);
	else if (stitch_ctx[grp_id]->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_422)
		stitch_w_uv_half_bypass(top_id);
}

static void stitch_hw_cfg_reg_stage1(stitch_grp grp_id, stitch_src_idx src_id)
{
	TRACE_STITCH(DBG_DEBUG, "src_id(%d) hw_trig stage1\n", src_id);
	stitch_record_hw_time_start(grp_id);
	stitch_enable_dev_clk(true);
	stitch_hw_cfg_reg(0, grp_id, src_id);

	if (src_id <= STITCH_SRC_ID_1) {
		atomic_set(&stitch_ctx[grp_id]->hdl_state, STITCH_HANDLER_STATE_RUN);
		atomic_set(&stitch_ctx[grp_id]->job.job_state, STITCH_JOB_WORKING);
		atomic_set(&evt_hdl_ctx.evt_state, STITCH_EVT_STATE_RUNNING);
		stitch_hw_trig();
	}
}

static void stitch_hw_cfg_reg_stage2(stitch_grp grp_id, stitch_src_idx src_id)
{
	TRACE_STITCH(DBG_DEBUG, "src_id(%d) hw_trig stage2\n", src_id);
	atomic_set(&evt_hdl_ctx.evt_state, STITCH_EVT_STATE_RUNNING_STAGE2);
	atomic_set(&stitch_ctx[grp_id]->hdl_state, STITCH_HANDLER_STATE_RUN_STAGE2);

	stitch_hw_cfg_reg(1, grp_id, src_id);
	stitch_hw_trig();
}

static unsigned char is_hw_buf_prepare_done(struct stitch_handler_ctx * evt_ctx)
{
	stitch_grp grp_id;
	unsigned char prepare_done = false;

	grp_id = evt_ctx->working_grp;
	if (stitch_get_bits_from_value(evt_ctx->prepared_flag, evt_ctx->src_id, evt_ctx->src_id -1) == 0x3)
		prepare_done = true;
	else {
		TRACE_STITCH(DBG_NOTICE, "src buf [%d] or [%d] not ready, prepared_flag[%#x]\n", evt_ctx->src_id, evt_ctx->src_id -1, evt_ctx->prepared_flag);
		if (!base_mod_jobs_workq_empty(&stitch_jobs[grp_id].ins[evt_ctx->src_id]) && !base_mod_jobs_workq_empty(&stitch_jobs[grp_id].ins[evt_ctx->src_id -1]))
			prepare_done = true;
	}
	return prepare_done;
}

static unsigned char stitch_hw_buf_prepare_done(struct stitch_handler_ctx * evt_ctx)
{
	stitch_grp grp_id;
	unsigned char prepare_done = false;

	if (!evt_ctx)
		return prepare_done;

	grp_id = evt_ctx->working_grp;
	switch (stitch_ctx[grp_id]->src_num) {
	case SRC_NUM_2WAY:
		if (evt_ctx->src_id == STITCH_SRC_ID_1)
			prepare_done = is_hw_buf_prepare_done(evt_ctx);
		else {
			if (!base_mod_jobs_workq_empty(&stitch_jobs[grp_id].ins[evt_ctx->src_id])
				&& !base_mod_jobs_workq_empty(&stitch_jobs[grp_id].out))
				prepare_done = true;
		}
		break;
	case SRC_NUM_4WAY:
		if (evt_ctx->src_id == STITCH_SRC_ID_1 || evt_ctx->src_id == STITCH_SRC_ID_3)
			prepare_done = is_hw_buf_prepare_done(evt_ctx);
		break;
	default:
		TRACE_STITCH(DBG_ERR, "not support grp[%d]: stitch_ctx->src_num(%d)\n", grp_id, stitch_ctx[grp_id]->src_num);
		break;
	}

	return prepare_done;
}

static void stitch_hw_enable_param(struct stitch_handler_ctx * evt_ctx)
{
	stitch_grp grp_id;
	unsigned long flags;
	int ret;

	grp_id = evt_ctx->working_grp;
	if (stitch_hw_buf_prepare_done(evt_ctx)) {
		if (evt_ctx->src_id == STITCH_SRC_ID_3) {
			ret = down_timeout(&evt_ctx->sem_2nd, msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS));
			if (ret == -ETIME)
				TRACE_STITCH(DBG_ERR, "get sem_2nd timeout, stitch hw not respond\n");
			stitch_hw_cfg_reg_stage1(evt_ctx->working_grp, evt_ctx->src_id);
			stitch_hw_cfg_reg_stage2(evt_ctx->working_grp, evt_ctx->src_id);
		} else
			stitch_hw_cfg_reg_stage1(evt_ctx->working_grp, evt_ctx->src_id);

		spin_lock_irqsave(&evt_ctx->lock, flags);
		evt_ctx->prepared_flag = stitch_modified_bits_by_value(evt_ctx->prepared_flag, 0, evt_ctx->src_id, evt_ctx->src_id -1);
		spin_unlock_irqrestore(&evt_ctx->lock, flags);
	}
}

static void job_fill_buf_in(struct video_buffer *buf, stitch_grp grp_id, stitch_src_idx src_id)
{
	unsigned char i;
	unsigned int stride;
	unsigned int width = buf->size.width;
	unsigned int height = buf->size.height;
	unsigned int bld23width;
	unsigned int offset = 0;

	bld23width = stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[1] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[1] + 1;
	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (buf->pixel_format == PIXEL_FORMAT_YUV_PLANAR_420 || buf->pixel_format == PIXEL_FORMAT_YUV_PLANAR_422) {
			width  = ((i == 0) ? (buf->size.width)  : (buf->size.width  >> 1));

			if (buf->pixel_format == PIXEL_FORMAT_YUV_PLANAR_420)
				height = ((i == 0) ? (buf->size.height) : (buf->size.height >> 1));
		}

		stride = ALIGN(width, STITCH_STRIDE_ALIGN);

		switch (src_id) {
		case STITCH_SRC_ID_0:
			stitch_hw_cfg_obj->rdma_l[0][i].addr = buf->phy_addr[i];
			stitch_hw_cfg_obj->rdma_l[0][i].height = height;
			stitch_hw_cfg_obj->rdma_l[0][i].width = width;
			stitch_hw_cfg_obj->rdma_l[0][i].stride = stride;
			break;
		case STITCH_SRC_ID_1:
			stitch_hw_cfg_obj->rdma_r[0][i].addr = buf->phy_addr[i];
			stitch_hw_cfg_obj->rdma_r[0][i].height = height;
			stitch_hw_cfg_obj->rdma_r[0][i].width = width;
			stitch_hw_cfg_obj->rdma_r[0][i].stride = stride;
			break;
		case STITCH_SRC_ID_2:
			offset = (i == 0) ? (stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[1] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[1] +1) :
				((stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[1] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[1] +1)>>1);

			stitch_hw_cfg_obj->rdma_l[2][i].addr = buf->phy_addr[i] + offset;
			stitch_hw_cfg_obj->rdma_l[2][i].height = height;
			stitch_hw_cfg_obj->rdma_l[2][i].stride = stride;

			//todo:
			//need calulate rdma for img23
			offset = (i == 0) ? (stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[1] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[0])
				: (stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[1] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[0]) >> 1;
			stitch_hw_cfg_obj->rdma_l[1][i].addr = stitch_hw_cfg_obj->rdma_r[0][i].addr + offset; //img2 + offset
			stitch_hw_cfg_obj->rdma_l[1][i].height = height;
			stitch_hw_cfg_obj->rdma_l[1][i].width = ((i == 0) ? bld23width : (bld23width >> 1));
			stitch_hw_cfg_obj->rdma_l[1][i].stride = stitch_hw_cfg_obj->rdma_r[0][i].stride;

			stitch_hw_cfg_obj->rdma_r[1][i].addr = buf->phy_addr[i];//img3
			stitch_hw_cfg_obj->rdma_r[1][i].height = height;
			stitch_hw_cfg_obj->rdma_r[1][i].width = ((i == 0) ? bld23width : (bld23width >> 1));
			stitch_hw_cfg_obj->rdma_r[1][i].stride = stitch_hw_cfg_obj->rdma_l[2][i].stride;;
			break;
		case STITCH_SRC_ID_3:
			stitch_hw_cfg_obj->rdma_l[2][i].width = (i==0) ? stitch_hw_cfg_obj->crop2bj[2].left_img_width : stitch_hw_cfg_obj->crop2bj[2].left_img_width>>1;
			//stitch_hw_cfg_obj->rdma_l[2][i].width = (i==0) ? 0x200 : 0x200>>1;

			stitch_hw_cfg_obj->rdma_r[2][i].addr = buf->phy_addr[i];
			stitch_hw_cfg_obj->rdma_r[2][i].height = height;
			stitch_hw_cfg_obj->rdma_r[2][i].width = width;
			stitch_hw_cfg_obj->rdma_r[2][i].stride = stride;
			break;
		default:
			TRACE_STITCH(DBG_DEBUG, "src_id(%d)\n", src_id);
			break;
		}
	}
}

static void job_fill_buf_out(struct video_buffer *buf)
{
	int i = 0;
	unsigned int l_nbldwidth;
	unsigned int bldwidth;
	unsigned int r_nbldwidth;
	unsigned int height;
	unsigned int stride = ALIGN(buf->size.width, STITCH_STRIDE_ALIGN);

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (buf->pixel_format == PIXEL_FORMAT_YUV_PLANAR_420 || buf->pixel_format == PIXEL_FORMAT_YUV_PLANAR_422) {
			stride = (i == 0
				? ALIGN(buf->size.width, STITCH_STRIDE_ALIGN)
				: ALIGN(buf->size.width, STITCH_STRIDE_ALIGN) >> 1);
		}

		if (i < 2) {
			l_nbldwidth = (stitch_hw_cfg_obj->wdma_nbld_size[0][i].img_width_w_left) ?
				(stitch_hw_cfg_obj->wdma_nbld_size[0][i].img_width_w_left + 1) : (STITCH_SIZE_ZERO);

			bldwidth = (stitch_hw_cfg_obj->wdma_bld_size[0][i].img_width_w_bld) ?
				(stitch_hw_cfg_obj->wdma_bld_size[0][i].img_width_w_bld + 1) : (STITCH_SIZE_ZERO);

			r_nbldwidth = (stitch_hw_cfg_obj->wdma_nbld_size[0][i].img_width_w_right) ?
				(stitch_hw_cfg_obj->wdma_nbld_size[0][i].img_width_w_right + 1) : (STITCH_SIZE_ZERO);

			height = stitch_hw_cfg_obj->wdma_bld_size[0][i].img_height_w_bld + 1;
		}
		stitch_hw_cfg_obj->wdma_l[0][i].height = height;
		stitch_hw_cfg_obj->wdma_r[0][i].height = height;
		stitch_hw_cfg_obj->wdma_bld[0][i].height = height;

		stitch_hw_cfg_obj->wdma_l[0][i].width = l_nbldwidth;
		stitch_hw_cfg_obj->wdma_r[0][i].width = r_nbldwidth;
		stitch_hw_cfg_obj->wdma_bld[0][i].width = bldwidth;

		stitch_hw_cfg_obj->wdma_l[0][i].stride = stride;
		stitch_hw_cfg_obj->wdma_r[0][i].stride = stride;
		stitch_hw_cfg_obj->wdma_bld[0][i].stride = stride;

		stitch_hw_cfg_obj->wdma_l[0][i].addr = buf->phy_addr[i];
		stitch_hw_cfg_obj->wdma_r[0][i].addr = buf->phy_addr[i] + l_nbldwidth + bldwidth;
		stitch_hw_cfg_obj->wdma_bld[0][i].addr = buf->phy_addr[i] + l_nbldwidth;

		TRACE_STITCH(DBG_DEBUG, "wdma_l:w[%d]-wdma_bld:w[%d]-wdma_r:w[%d]\n"
			, stitch_hw_cfg_obj->wdma_l[0][i].width, stitch_hw_cfg_obj->wdma_bld[0][i].width, stitch_hw_cfg_obj->wdma_r[0][i].width);
	}
}

static void job_fill_buf_out34(struct video_buffer *buf, stitch_grp grp_id, stitch_src_idx src_id)
{
	int i = 0;

	unsigned int l_nbldwidth;
	unsigned int bldwidth;
	unsigned int r_nbldwidth;
	unsigned int height;
	unsigned int stride = ALIGN(buf->size.width, STITCH_STRIDE_ALIGN);
	unsigned char bj_id = src_id - 1;
	unsigned long long addr_offset;

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (buf->pixel_format == PIXEL_FORMAT_YUV_PLANAR_420 || buf->pixel_format == PIXEL_FORMAT_YUV_PLANAR_422) {
			stride = (i == 0
				? ALIGN(buf->size.width, STITCH_STRIDE_ALIGN)
				: ALIGN(buf->size.width, STITCH_STRIDE_ALIGN) >> 1);
		}

		if (i < 2) {
			l_nbldwidth = (stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_left) ?
				(stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_left + 1) : (STITCH_SIZE_ZERO);

			bldwidth = (stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_width_w_bld) ?
				(stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_width_w_bld + 1) : (STITCH_SIZE_ZERO);

			r_nbldwidth = (stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_right) ?
				(stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_right + 1) : (STITCH_SIZE_ZERO);

			height = stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_height_w_bld + 1;
		}
		stitch_hw_cfg_obj->wdma_l[bj_id][i].height = height;
		stitch_hw_cfg_obj->wdma_r[bj_id][i].height = height;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].height = height;

		stitch_hw_cfg_obj->wdma_l[bj_id][i].width = l_nbldwidth;
		stitch_hw_cfg_obj->wdma_r[bj_id][i].width = r_nbldwidth;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].width = bldwidth;

		stitch_hw_cfg_obj->wdma_l[bj_id][i].stride = stride;
		stitch_hw_cfg_obj->wdma_r[bj_id][i].stride = stride;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].stride = stride;

		addr_offset = (i==0 ? stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[1] : stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[1]>>1);

		stitch_hw_cfg_obj->wdma_l[bj_id][i].addr   = buf->phy_addr[i] + addr_offset;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].addr = stitch_hw_cfg_obj->wdma_l[bj_id][i].addr + l_nbldwidth;
		stitch_hw_cfg_obj->wdma_r[bj_id][i].addr   = stitch_hw_cfg_obj->wdma_bld[bj_id][i].addr + bldwidth;

		TRACE_STITCH(DBG_DEBUG, "wdma_l:w[%d]-wdma_bld:w[%d]-wdma_r:w[%d]\n"
			, stitch_hw_cfg_obj->wdma_l[bj_id][i].width, stitch_hw_cfg_obj->wdma_bld[bj_id][i].width, stitch_hw_cfg_obj->wdma_r[bj_id][i].width);
	}
}

static void job_fill_buf_out23(struct video_buffer *buf, stitch_grp grp_id)
{
	int i = 0;
	unsigned int bldwidth = 0;
	unsigned int height = 0;
	unsigned int stride = ALIGN(buf->size.width, STITCH_STRIDE_ALIGN);
	unsigned char bj_id = 1;
	unsigned long long addr_offset;
	unsigned int bld23width;

	bld23width = stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[bj_id] + 1;

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (buf->pixel_format == PIXEL_FORMAT_YUV_PLANAR_420 || buf->pixel_format == PIXEL_FORMAT_YUV_PLANAR_422) {
			stride = (i == 0
				? ALIGN(buf->size.width, STITCH_STRIDE_ALIGN)
				: ALIGN(buf->size.width, STITCH_STRIDE_ALIGN) >> 1);
		}

		bldwidth = (i == 0 ? bld23width : (bld23width >> 1));
		height = (i == 0 ? buf->size.height: (buf->size.height >> 1));

		stitch_hw_cfg_obj->wdma_bld[bj_id][i].height = height;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].width = bldwidth;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].stride = stride;

		addr_offset = (i==0 ? stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[1] : stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[1]>>1);
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].addr   = buf->phy_addr[i] + addr_offset;

		TRACE_STITCH(DBG_DEBUG, "wdma_bld:w[%d]\n", stitch_hw_cfg_obj->wdma_bld[bj_id][i].width);
	}
}

static void stitch_fill_video_buffer(stitch_grp grp_id, struct video_buffer *src_buf, unsigned long long phy_addr, struct video_buffer *buf)
{
	unsigned char align = (stitch_ctx[grp_id]->chn_attr.size.width % (STITCH_ALIGN << 1))
		? (STITCH_ALIGN >> 1)
		: (STITCH_ALIGN);

	base_get_frame_info(stitch_ctx[grp_id]->chn_attr.fmt_out, stitch_ctx[grp_id]->chn_attr.size, buf, phy_addr, align);
	buf->offset_top = 0;
	buf->offset_bottom = 0;
	buf->offset_left = 0;
	buf->offset_right = 0;

	if (src_buf) {
		buf->pts = src_buf->pts;
		buf->frm_num = src_buf->frm_num;
		//buf->motion_lv = grp_buf->motion_lv;
		//memcpy(buf->motion_table, grp_buf->motion_table, MO_TBL_SIZE);
	}
}

static int stitch_chn_qbuf(struct video_buffer *src_buf, vb_blk blk, stitch_grp grp_id)
{
	mmf_chn_s chn = {.mod_id = ID_STITCH, .dev_id = grp_id};
	struct vb_s *vb = (struct vb_s *)blk;
	int ret = 0;

	stitch_fill_video_buffer(grp_id, src_buf, vb_handle2phys_addr(blk), &vb->buf);//convert chn blk to buf, and copy pts info

	TRACE_STITCH(DBG_DEBUG, "frm_num(%d) size[%d-%d] Chn buf[0x%lx-0x%lx-0x%lx] stride[%d-%d-%d] len[%zu-%zu-%zu]\n"
			, vb->buf.frm_num
			, vb->buf.size.width, vb->buf.size.height
			, (unsigned long)vb->buf.phy_addr[0]
			, (unsigned long)vb->buf.phy_addr[1]
			, (unsigned long)vb->buf.phy_addr[2]
			, vb->buf.stride[0], vb->buf.stride[1], vb->buf.stride[2]
			, vb->buf.length[0], vb->buf.length[1], vb->buf.length[2]);

	ret = vb_qbuf(chn, CHN_TYPE_OUT, &stitch_jobs[grp_id].out, blk);
	if (ret != 0)
		TRACE_STITCH(DBG_ERR, "Chn qbuf failed\n");
	else
		job_fill_buf_out(&vb->buf);

	vb_release_block(blk);

	return ret;
}

static int fill_buffers(stitch_grp grp_id, stitch_src_idx src_id)
{
	struct video_buffer *buf_in = NULL, *buf_out = NULL;
	vb_blk blk_out = VB_INVALID_HANDLE;
	int ret = 0;
	struct vb_s *vb = NULL;

	if (base_mod_jobs_waitq_empty(&stitch_jobs[grp_id].ins[src_id])) {
		TRACE_STITCH(DBG_ERR, "src_id(%d) for grp_id(%d) waitq_empty, buf not ready\n", src_id, grp_id);
		ret = ERR_STITCH_BUF_EMPTY;
		goto ERR_FILL_QBUF;
	}

	buf_in = base_mod_jobs_enque_work(&stitch_jobs[grp_id].ins[src_id]);
	if (buf_in == NULL) {
		TRACE_STITCH(DBG_ERR, "src_id(%d) for grp_id(%d) qbuf failed.\n", src_id, grp_id);
		ret = ERR_STITCH_NOTREADY;
		goto ERR_FILL_QBUF;
	}

	TRACE_STITCH(DBG_DEBUG, "src_id(%d) for grp_id(%d) buf: 0x%lx-0x%lx-0x%lx\n"
		, src_id
		, grp_id
		, (unsigned long)buf_in->phy_addr[0]
		, (unsigned long)buf_in->phy_addr[1]
		, (unsigned long)buf_in->phy_addr[2]);

	job_fill_buf_in(buf_in, grp_id, src_id);

	if (src_id == STITCH_SRC_ID_1) {
		if (!base_mod_jobs_waitq_empty(&stitch_jobs[grp_id].out)) {// chn buffer from user
			TRACE_STITCH(DBG_DEBUG, "chn buffer from user.\n");

			buf_out = base_mod_jobs_enque_work(&stitch_jobs[grp_id].out);
			if (!buf_out) {
				TRACE_STITCH(DBG_ERR, "chn qbuf failed.\n");
				ret = ERR_STITCH_NOTREADY;
				goto ERR_FILL_QBUF;
			}

			memcpy(&stitch_chn_vbq[grp_id]->buf, buf_out, sizeof(*buf_out));
			blk_out = (vb_blk)stitch_chn_vbq[grp_id];
			job_fill_buf_out(&stitch_chn_vbq[grp_id]->buf);
		} else {// chn buffer from pool
			blk_out = vb_get_block_with_id(stitch_ctx[grp_id]->vb_pool, stitch_ctx[grp_id]->vb_size, ID_STITCH);

				if (blk_out == VB_INVALID_HANDLE) {
				TRACE_STITCH(DBG_ERR, "Can't acquire VB BLK for STITCH chn\n");
				ret = ERR_STITCH_NOBUF;
				goto ERR_FILL_QBUF;
			}

			vb =(struct vb_s *)blk_out;
			TRACE_STITCH(DBG_DEBUG, "vb_pool: %d, blk_out: vb_id: %llu, vb->buf.size.width: %d vb->buf.size.height: %d stitch_ctx[grp_id]->vb_size: %d buf: buf:(%llx-%llx-%llx)\n",
				stitch_ctx[grp_id]->vb_pool,
				blk_out,
				vb->buf.size.width,
				vb->buf.size.height,
				stitch_ctx[grp_id]->vb_size,
				vb->buf.phy_addr[0],
				vb->buf.phy_addr[1],
				vb->buf.phy_addr[2]);


			if (stitch_chn_qbuf(buf_in, blk_out, grp_id)) {
				TRACE_STITCH(DBG_ERR, "Chn qbuf failed.\n");
				ret = ERR_STITCH_NOTREADY;
				goto ERR_FILL_QBUF;
			}
		}

		stitch_ctx[grp_id]->vb_out = (struct vb_s *)blk_out;
	} else {
		if (src_id == STITCH_SRC_ID_3) {
			vb = stitch_ctx[grp_id]->vb_out;
			job_fill_buf_out34(&vb->buf, grp_id, src_id);
			job_fill_buf_out23(&vb->buf, grp_id);
		}
	}
	return ret;

ERR_FILL_QBUF:
	release_stitch_jobq(grp_id);

	return ret;
}

static void stitch_notify_wkup_evt_th(int core_id)
{
	unsigned long flags;
	(void)core_id;

	spin_lock_irqsave(&evt_hdl_ctx.lock, flags);
	evt_hdl_ctx.events |= CTX_EVENT_WKUP;
	spin_unlock_irqrestore(&evt_hdl_ctx.lock, flags);

	wake_up_interruptible(&evt_hdl_ctx.wait);
}

static int stitch_src_qbuf(mmf_chn_s chn, vb_blk blk)
{
	int ret;
	int src_id = chn.chn_id;
	int grp = chn.dev_id;

	if(!stitch_ctx_is_enbale(chn.dev_id))
		return -1;

	ret = vb_qbuf(chn, CHN_TYPE_IN, &stitch_jobs[grp].ins[src_id], blk);
	if (ret) {
		stitch_ctx[grp]->work_status.lost_cnt++;
		return ret;
	}
	stitch_ctx[grp]->work_status.recv_cnt++;

	return 0;
}

static void stitch_handle_frm_done(struct __stitch_ctx *p_stitch_ctx)
{
	mmf_chn_s chn = {.mod_id = ID_STITCH, .dev_id = p_stitch_ctx->grp_id};
	vb_blk blk;
	int i;
	stitch_grp grp_id = p_stitch_ctx->grp_id;

	atomic_set(&p_stitch_ctx->job.job_state, STITCH_JOB_END);
	atomic_set(&p_stitch_ctx->hdl_state, STITCH_HANDLER_STATE_DONE);
	atomic_set(&evt_hdl_ctx.evt_state, STITCH_EVT_STATE_END);

	for (i = 0; i < p_stitch_ctx->src_num; i++) {
		vb_dqbuf(chn, &stitch_jobs[grp_id].ins[i], &blk);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_STITCH(DBG_ERR, "Mod(%d) src_id(%d) can't get vb-blk\n", chn.mod_id, i);
		} else {
			vb_done_handler(chn, CHN_TYPE_IN, &stitch_jobs[grp_id].ins[i], blk);
		}
	}

	vb_dqbuf(chn, &stitch_jobs[grp_id].out, &blk);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_STITCH(DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.mod_id);
	} else {
		struct vb_s * vb = (struct vb_s *)blk;

		TRACE_STITCH(DBG_DEBUG, "done buf:0x%llx\n", vb->buf.phy_addr[0]);
		vb_done_handler(chn, CHN_TYPE_OUT, &stitch_jobs[grp_id].out, blk);
	}
	p_stitch_ctx->work_status.done_cnt++;

	TRACE_STITCH(DBG_DEBUG, "get eof cnt(%d)\n", p_stitch_ctx->work_status.done_cnt);
}

static void stitch_work_handle_frm_done(struct __stitch_ctx *p_stitch_ctx)
{
	unsigned long flags;
	enum stitch_handler_state hdlState;

	spin_lock_irqsave(&p_stitch_ctx->lock, flags);
	hdlState = (enum stitch_handler_state)atomic_read(&p_stitch_ctx->hdl_state);
	spin_unlock_irqrestore(&p_stitch_ctx->lock, flags);

	TRACE_STITCH(DBG_INFO, "grp[%d], src_num[%d], stt(%d)\n"
		, p_stitch_ctx->grp_id, p_stitch_ctx->src_num, hdlState);

	if (hdlState == STITCH_HANDLER_STATE_RUN || hdlState == STITCH_HANDLER_STATE_DONE) {
		if (p_stitch_ctx->src_num == 2) {
		//if (p_stitch_ctx->src_num == 2 || p_stitch_ctx->src_num == 4) {//this is debug only bld img23
			stitch_handle_frm_done(p_stitch_ctx);
			spin_lock_irqsave(&p_stitch_ctx->lock, flags);
			if (p_stitch_ctx->param_update)
				p_stitch_ctx->param_update = false;
			spin_unlock_irqrestore(&p_stitch_ctx->lock, flags);
		} else {
			up(&evt_hdl_ctx.sem_2nd);
#if 0
			atomic_set(&evt_hdl_ctx.evt_state, STITCH_EVT_STATE_RUNNING_STAGE2);
			atomic_set(&p_stitch_ctx->hdl_state, STITCH_HANDLER_STATE_RUN_STAGE2);
#endif
			spin_lock_irqsave(&p_stitch_ctx->lock, flags);
			p_stitch_ctx->param_update = true;
			spin_unlock_irqrestore(&p_stitch_ctx->lock, flags);
		}
	} else if (hdlState == STITCH_HANDLER_STATE_RUN_STAGE2) {
		stitch_handle_frm_done(p_stitch_ctx);
		spin_lock_irqsave(&p_stitch_ctx->lock, flags);
		p_stitch_ctx->param_update = true;
		spin_unlock_irqrestore(&p_stitch_ctx->lock, flags);
	} else
		TRACE_STITCH(DBG_ERR, "invalid handler state(%d)\n", hdlState);
}

static void stitch_work_frm_done(struct work_struct *work)
{
	struct __stitch_ctx *p_stitch_ctx = container_of(work, struct __stitch_ctx, work_frm_done);

	if (unlikely(!stitch_ctx_is_valid(p_stitch_ctx)))
		return;

	stitch_work_handle_frm_done(p_stitch_ctx);

	stitch_notify_wkup_evt_th(p_stitch_ctx->core_id);
}

static void stitch_wkup_frm_done_work(void *data)
{
	struct __stitch_ctx *p_stitch_ctx = (struct __stitch_ctx *)data;
	unsigned long flags;

	if (!p_stitch_ctx) {
		TRACE_STITCH(DBG_ERR, "dev isn't created yet.\n");
		return;
	}

	if (p_stitch_ctx != stitch_ctx[p_stitch_ctx->grp_id]) {
		TRACE_STITCH(DBG_ERR, "dev ctx[%d] is invalid.\n", p_stitch_ctx->grp_id);
		return;
	}

	stitch_record_hw_time_end(p_stitch_ctx->grp_id);
	p_stitch_ctx->work_status.hw_cost_time = stitch_record_hw_time(p_stitch_ctx->grp_id);
	if (p_stitch_ctx->work_status.hw_cost_time > p_stitch_ctx->work_status.hw_max_cost_time) {
		p_stitch_ctx->work_status.hw_max_cost_time = p_stitch_ctx->work_status.hw_cost_time;
	}
	spin_lock_irqsave(&evt_hdl_ctx.lock, flags);
	evt_hdl_ctx.time.hw_duration += p_stitch_ctx->work_status.hw_cost_time;
	evt_hdl_ctx.time.grp_fps[p_stitch_ctx->grp_id]++;
	spin_unlock_irqrestore(&evt_hdl_ctx.lock, flags);
	evt_hdl_ctx.time.all_hw_duration += p_stitch_ctx->work_status.hw_cost_time;

	TRACE_STITCH(DBG_INFO, "grp[%d] hardware[%d] time cost:[%u] us\n", p_stitch_ctx->grp_id, p_stitch_ctx->core_id, p_stitch_ctx->work_status.hw_cost_time);

	queue_work(evt_hdl_ctx.workqueue, &stitch_ctx[evt_hdl_ctx.working_grp]->work_frm_done);
}

static int stitch_is_grp_waitq_ready(stitch_grp grp_id)
{
	int i;
	int flag = 1;

	for(i = 0; i < stitch_ctx[grp_id]->src_num; i++)
		flag &= ~ (int)base_mod_jobs_waitq_empty(&stitch_jobs[grp_id].ins[i]);
	return flag;
}
static int stitch_is_grp_workq_ready(stitch_grp grp_id)
{
	int i;
	int flag = 1;

	for(i = 0; i < stitch_ctx[grp_id]->src_num; i++)
		flag &= ~ (int)base_mod_jobs_workq_empty(&stitch_jobs[grp_id].ins[i]);
	return flag;
}

static int stitch_try_schedule(struct stitch_handler_ctx * evt_ctx)
{
	mmf_chn_s chn = {.mod_id = ID_STITCH, .chn_id = evt_ctx->src_id, .dev_id = evt_ctx->working_grp};
	unsigned long flags;
	stitch_grp grp_id;

	grp_id = evt_ctx->working_grp;
	ktime_get_ts64(&sw_ts[0]);

	TRACE_STITCH(DBG_DEBUG, "cur src_id[%d] for grp_id[%d]\n", evt_ctx->src_id, grp_id);

	if(unlikely(!stitch_ctx_is_enbale(grp_id)))
		goto stitch_ctx_not_enable;

	if (stitch_trans_to_hw_setting(grp_id, evt_ctx->src_id)) {
		TRACE_STITCH(DBG_ERR, "trans to hw settings NG for grp_id[%d].\n", grp_id);
		release_stitch_waitq(chn, CHN_TYPE_IN);
		stitch_ctx[grp_id]->work_status.fail_recv_cnt++;
		goto stitch_next_job;
	}

	if (fill_buffers(grp_id, evt_ctx->src_id)) {
		TRACE_STITCH(DBG_ERR, "fill buffer NG for grp_id[%d].\n", grp_id);
		stitch_ctx[grp_id]->work_status.fail_recv_cnt++;
		goto stitch_next_job;
	}

	spin_lock_irqsave(&evt_ctx->lock, flags);
	evt_ctx->prepared_flag |= BIT(evt_ctx->src_id);
	spin_unlock_irqrestore(&evt_ctx->lock, flags);

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	stitch_ctx[grp_id]->grp_id = grp_id;
	//stitch_ctx[grp_id]->core_id = evt_ctx->core_id;
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

#if 0
	atomic_set(&stitch_ctx[grp_id]->hdl_state, STITCH_HANDLER_STATE_RUN);
	atomic_set(&stitch_ctx[grp_id]->job.job_state, STITCH_JOB_WORKING);
	atomic_set(&evt_hdl_ctx.evt_state, STITCH_EVT_STATE_RUNNING);
#endif
	stitch_hw_enable_param(evt_ctx);
	return 0;

stitch_next_job:
stitch_ctx_not_enable:
	return -1;
}
static stitch_grp get_schedule_grp_id(void)
{
	struct grp_work *work;
	stitch_grp grp_id;

	if (unlikely(FIFO_EMPTY(&evt_hdl_ctx.grp_workq)))
		return STITCH_INVALID_GRP;

	mutex_lock(&stitch_get_grp_lock);
	FIFO_POP(&evt_hdl_ctx.grp_workq, &work);
	mutex_unlock(&stitch_get_grp_lock);

	grp_id = work->grp;
	kfree(work);

	return grp_id;
}

static int stitch_event_handler_th(void *arg)
{
	struct stitch_handler_ctx *evt_ctx = (struct stitch_handler_ctx *)arg;
	unsigned long idle_timeout = msecs_to_jiffies(IDLE_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	unsigned long flags;
	int ret, src_id;
	unsigned char i;
	unsigned char j;
	enum stitch_handler_state hdl_state;
	enum stitch_evt_state evt_state;
	struct vb_jobs_t *jobs;
	stitch_grp grp_id;

	if (evt_ctx != &evt_hdl_ctx) {
		TRACE_STITCH(DBG_ERR, "invalid thread(%s) param\n", __func__);
		return -1;
	}

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(evt_ctx->wait,
			evt_ctx->events || kthread_should_stop(), timeout);

		/* -ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret && stitch_handler_is_idle()) {
			atomic_set(&evt_ctx->evt_state, STITCH_EVT_STATE_IDLE);
			stitch_enable_dev_clk(false);
			timeout = idle_timeout;
			continue;
		}

		//suspend
		if (is_stitch_suspended())
			continue;

		spin_lock_irqsave(&evt_ctx->lock, flags);
		evt_ctx->events &= ~CTX_EVENT_WKUP;
		spin_unlock_irqrestore(&evt_ctx->lock, flags);

		for(j = 0; j < stitch_grp_num; j++) {
			evt_state = (enum stitch_evt_state)atomic_read(&evt_ctx->evt_state);
			if (evt_state == STITCH_EVT_STATE_RUNNING ||
				evt_state == STITCH_EVT_STATE_RUNNING_STAGE2)
				break;

#if 0
			grp_id = grp_rec[j];
#endif
			grp_id = get_schedule_grp_id();
			if (GRP_ID_INVALID(grp_id))
				break;

			if (!stitch_ctx_is_enbale2(grp_id))
				continue;

			hdl_state = (enum stitch_handler_state)atomic_read(&stitch_ctx[grp_id]->hdl_state);
			if (unlikely(hdl_state == STITCH_HANDLER_STATE_RUN ||
				hdl_state == STITCH_HANDLER_STATE_RUN_STAGE2 ||
				hdl_state == STITCH_HANDLER_STATE_SUSPEND))
				continue;

			if (!(stitch_is_grp_waitq_ready(grp_id)))
				continue;

			stitch_ctx[grp_id]->param_update = true;
			for (i = 0; i < stitch_ctx[grp_id]->src_num; i++) {
				src_id = i;
				jobs = &stitch_jobs[grp_id].ins[src_id];
				if (!jobs) {
					TRACE_STITCH(DBG_DEBUG, "get (%d) jobs failed, src num(%d)\n", i, stitch_ctx[grp_id]->src_num);
					continue;
				}

				if (!down_trylock(&jobs->sem)) {
					evt_ctx->src_id = src_id;
					evt_ctx->working_grp = grp_id;
					stitch_try_schedule(evt_ctx);
				}
			}
			/* Adjust timeout */
			timeout = eof_timeout;
		}
	}
	return 0;
}

static int stitch_creat_hw_cfg_obj(void)
{
	if (!stitch_hw_cfg_obj) {
		stitch_hw_cfg_obj = kzalloc(sizeof(struct stitch_hw_cfg_param), GFP_ATOMIC);//auto memset
		if (!stitch_hw_cfg_obj) {
			TRACE_STITCH(DBG_ERR, "kzalloc fail.\n");
			return ERR_STITCH_NOMEM;
		}
	}

	return 0;
}

static int stitch_destrory_hw_cfg_obj(void)
{
	if (stitch_hw_cfg_obj) {
		kfree(stitch_hw_cfg_obj);
		stitch_hw_cfg_obj = NULL;
	}

	return 0;
}

static void stitch_init_sw_param_ctx(stitch_grp grp_id)
{
	spin_lock_init(&stitch_ctx[grp_id]->lock);
	atomic_set(&stitch_ctx[grp_id]->enable_count, 0);
	stitch_ctx[grp_id]->update_status = 0;
	stitch_ctx[grp_id]->is_created = true;
}

static int stitch_creat_ctx(stitch_grp grp_id)
{
	stitch_ctx[grp_id] = kzalloc(sizeof(struct __stitch_ctx), GFP_ATOMIC);//auto memset
	if (!stitch_ctx[grp_id]) {
		TRACE_STITCH(DBG_ERR, "kzalloc fail.\n");
		return ERR_STITCH_NOMEM;
	}
	stitch_init_sw_param_ctx(grp_id);

	return 0;
}

static void stitch_destroy_ctx(stitch_grp grp_id)
{
	if (stitch_ctx[grp_id]) {
		stitch_ctx[grp_id]->is_created = false;
		kfree(stitch_ctx[grp_id]);
		stitch_ctx[grp_id] = NULL;
	}
}



static void stitch_init_grp_sw_param_ctx(stitch_grp grp_id)
{
	if (stitch_ctx[grp_id]) {
		spin_lock_init(&stitch_ctx[grp_id]->lock);
		atomic_set(&stitch_ctx[grp_id]->enable_count, 0);
	}
}

struct __stitch_ctx *stitch_get_ctx_by_id(stitch_grp grp_id)
{
	return stitch_ctx[grp_id];
}

struct __stitch_ctx **stitch_get_ctx(void)
{
	return stitch_ctx;
}

struct stitch_handler_ctx *stitch_get_evt_hdl_ctx(void)
{
	return &evt_hdl_ctx;
}

stitch_grp stitch_get_available_grp(void)
{
	stitch_grp i = 0;
	stitch_grp grp = STITCH_INVALID_GRP;

	mutex_lock(&stitch_get_grp_lock);
	for (i = 0; i < STITCH_MAX_GRP_NUM; i++) {
		if(!stitch_grp_used[i]) {
			stitch_grp_used[i] = true;
			grp = i;
			break;
		}
	}
	mutex_unlock(&stitch_get_grp_lock);

#if 0
	if (stitch_ctx[grp]) {
		stitch_disable_grp(grp);
		stitch_deinit_grp(grp);
	}
	stitch_init_grp(grp);
	stitch_enable_grp(grp);
#endif

	return grp;
}

int stitch_get_grp_num(void)
{
	return stitch_grp_num;
}

bool *stitch_get_grp_used(void)
{
	return stitch_grp_used;
}

static unsigned char stitch_creat_chn_vbq(stitch_grp grp_id)
{
	if (!stitch_chn_vbq[grp_id]) {
		stitch_chn_vbq[grp_id] = kzalloc(sizeof(*stitch_chn_vbq[grp_id]), GFP_ATOMIC);
		if (!stitch_chn_vbq[grp_id])
			return 1;
	}

	return 0;
}

static unsigned char stitch_destroy_chn_vbq(stitch_grp grp_id)
{
	if (stitch_chn_vbq[grp_id]) {
		kfree(stitch_chn_vbq[grp_id]);
		stitch_chn_vbq[grp_id] = NULL;
	}
	return 0;
}

int stitch_grp_qbuf(mmf_chn_s chn, vb_blk blk) {
	stitch_grp grp_id;
	stitch_src_idx src_idx;
	struct grp_work *work;

	if (is_stitch_suspended()) {
		TRACE_STITCH(DBG_ERR, "stitch dev suspend\n");
		return ERR_STITCH_NOT_PERM;
	}

	//todo: 更换chn_id 和dev_id位置
	grp_id = chn.dev_id;
	src_idx = chn.chn_id;

	//  chn.dev_id = grp_id;
	//  chn.chn_id = src_idx;

	if (stitch_src_qbuf(chn, blk) != 0) {
		TRACE_STITCH(DBG_ERR, " grp_idx[%d] src_idx(%d) buf failed\n", grp_id, src_idx);
		return ERR_STITCH_BUSY;
	}

	if (FIFO_FULL(&evt_hdl_ctx.grp_workq)) {
		TRACE_STITCH(DBG_ERR, "grp_workq full, out of range %d\n", FIFO_CAPACITY(&evt_hdl_ctx.grp_workq));
		return ERR_STITCH_NOT_PERM;
	}
	if (stitch_is_grp_waitq_ready(grp_id)) {
		work = (struct grp_work *)kmalloc(sizeof(*work), GFP_ATOMIC);
		work->grp = grp_id;
		work->private = stitch_ctx[grp_id];
		mutex_lock(&stitch_get_grp_lock);
		FIFO_PUSH(&evt_hdl_ctx.grp_workq, work);
		mutex_unlock(&stitch_get_grp_lock);
	}

	stitch_notify_wkup_evt_th(stitch_ctx[grp_id]->core_id);
	return 0;
}

int stitch_init_grp(stitch_grp grp_id)
{
	if (stitch_ctx_is_creat2(grp_id))
		return 0;

	if (stitch_creat_ctx(grp_id))
		return -1;

	mutex_lock(&stitch_get_grp_lock);
	stitch_grp_used[grp_id] = true;
	grp_rec[stitch_grp_num] = grp_id;
	stitch_grp_num++;
	mutex_unlock(&stitch_get_grp_lock);

	stitch_init_grp_sw_param_ctx(grp_id);
	stitch_ctx[grp_id]->grp_id = grp_id;
	stitch_ctx[grp_id]->update_status = 0;
	stitch_ctx[grp_id]->core_id = 0;
	TRACE_STITCH(DBG_DEBUG, "grp[%d] init+, stitch_grp_num[%d]\n", grp_id, stitch_grp_num);
	return 0;
}

int stitch_deinit_grp(stitch_grp grp_id)
{
	bool grp_used;

	mutex_lock(&stitch_get_grp_lock);
	grp_used = stitch_grp_used[grp_id];
	mutex_unlock(&stitch_get_grp_lock);
	if (!grp_used) {
		TRACE_STITCH(DBG_DEBUG, "grp[%d] del repeat\n", grp_id);
		return 0;
	}

	if (stitch_ctx_is_enbale2(grp_id))
		stitch_disable_grp(grp_id);

	stitch_destroy_ctx(grp_id);

	mutex_lock(&stitch_get_grp_lock);
	stitch_grp_used[grp_id] = false;
	stitch_grp_num--;
	mutex_unlock(&stitch_get_grp_lock);

	TRACE_STITCH(DBG_DEBUG, "grp[%d] deinit+, stitch_grp_num[%d]\n", grp_id, stitch_grp_num);
	return 0;
}

// todo: done ko加载时候做
int stitch_init(void)
{
	mutex_init(&stitch_get_grp_lock);
	//todo: 创建多个，移动到init_grp那里
	if (stitch_creat_hw_cfg_obj()) {
		TRACE_STITCH(DBG_ERR, "creat_hw_cfg_obj fail.\n");
		return ERR_STITCH_NOMEM;
	}

	base_register_recv_cb(ID_STITCH, stitch_grp_qbuf);

	TRACE_STITCH(DBG_DEBUG, "stitch init+\n");
	return 0;
}

int stitch_deinit(void) {
	int i;

	mutex_lock(&stitch_get_grp_lock);
	for(i = 0; i < stitch_grp_num; i++)
		stitch_grp_used[i] = false;
	stitch_grp_num = 0;
	mutex_unlock(&stitch_get_grp_lock);

	mutex_destroy(&stitch_get_grp_lock);
	stitch_destrory_hw_cfg_obj();

	base_unregister_recv_cb(ID_STITCH);

	TRACE_STITCH(DBG_DEBUG, "stitch deinit+\n");
	return 0;
}

int stitch_reset(void)
{
	if (atomic_read(stitch_get_open_count()) > 1)
		return 0;

	stitch_disable();
	stitch_invalid_param(0);
	stitch_invalid_param(1);
	stitch_reset_init();

	TRACE_STITCH(DBG_DEBUG, "stitch_reset+\n");
	return 0;
}

int stitch_send_frame(stitch_grp grp_id, stitch_src_idx src_idx, video_frame_info_s *pstvideo_frame, int s32milli_sec)
{
	mmf_chn_s chn = {.mod_id = ID_STITCH, .chn_id = src_idx, .dev_id = grp_id};
	vb_blk blk;
	int ret;
	struct grp_work *work;

	if (is_stitch_suspended()) {
		TRACE_STITCH(DBG_ERR, "stitch dev suspend\n");
		return ERR_STITCH_NOT_PERM;
	}

	ret = stitch_check_input_video_param(grp_id, src_idx, pstvideo_frame);
	if (ret != 0)
		return ret;

	pstvideo_frame->video_frame.align = STITCH_ALIGN;

	blk = vb_phys_addr2handle(pstvideo_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(pstvideo_frame->video_frame.phyaddr[0], NULL, VB_EXTERNAL_POOLID, true);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_STITCH(DBG_ERR, "src_idx(%d) for grp_idx[%d] no space for malloc.\n", src_idx, grp_id);
			return ERR_STITCH_NOMEM;
		}
	}

	if (base_fill_videoframe2buffer(chn, pstvideo_frame, &((struct vb_s *)blk)->buf) != 0) {
		TRACE_STITCH(DBG_ERR, "src_idx(%d) for grp_idx[%d] base_fill_videoframe2buffer fail\n", src_idx, grp_id);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	if (stitch_src_qbuf(chn, blk) != 0) {
		TRACE_STITCH(DBG_ERR, "src_idx(%d) grp_idx[%d] buf failed\n", src_idx, grp_id);
		return ERR_STITCH_BUSY;
	}

#if 0
	if (src_idx == STITCH_SRC_ID_1)
		atomic_set(&stitch_ctx[grp_id]->hdl_state, STITCH_HANDLER_STATE_RUN);
#endif
	if (FIFO_FULL(&evt_hdl_ctx.grp_workq)) {
		TRACE_STITCH(DBG_ERR, "grp_workq full, out of range %d\n", FIFO_CAPACITY(&evt_hdl_ctx.grp_workq));
		return ERR_STITCH_NOT_PERM;
	}
	if (stitch_is_grp_waitq_ready(grp_id)) {
		work = (struct grp_work *)kmalloc(sizeof(*work), GFP_ATOMIC);
		work->grp = grp_id;
		work->private = stitch_ctx[grp_id];
		mutex_lock(&stitch_get_grp_lock);
		FIFO_PUSH(&evt_hdl_ctx.grp_workq, work);
		mutex_unlock(&stitch_get_grp_lock);
	}
	stitch_notify_wkup_evt_th(stitch_ctx[grp_id]->core_id);

	TRACE_STITCH(DBG_INFO, "grp[%d] src_id[%d] phy_addr(%llx-%llx-%llx) fmt[%d]\n"
		, grp_id, src_idx
		, pstvideo_frame->video_frame.phyaddr[0]
		, pstvideo_frame->video_frame.phyaddr[1]
		, pstvideo_frame->video_frame.phyaddr[2]
		, pstvideo_frame->video_frame.pixel_format);
	return 0;
}

int stitch_send_chn_frame(stitch_grp grp_id, video_frame_info_s *pstvideo_frame, int s32milli_sec)
{
	mmf_chn_s chn = {.mod_id = ID_STITCH, .dev_id = grp_id};
	vb_blk blk;
	struct vb_s *vb;
	struct vb_jobs_t *jobs;
	int ret;

	if (is_stitch_suspended()) {
		TRACE_STITCH(DBG_ERR, "stitch dev suspend\n");
		return ERR_STITCH_NOT_PERM;
	}

	ret = stitch_check_output_video_param(grp_id, pstvideo_frame);
	if (ret != 0)
		return ret;

	pstvideo_frame->video_frame.align = STITCH_ALIGN;

	blk = vb_phys_addr2handle(pstvideo_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(pstvideo_frame->video_frame.phyaddr[0], NULL, VB_EXTERNAL_POOLID, true);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_STITCH(DBG_ERR, "chn no space for malloc.\n");
			return ERR_STITCH_NOMEM;
		}
	}

	if (base_fill_videoframe2buffer(chn, pstvideo_frame, &((struct vb_s *)blk)->buf) != 0) {
		TRACE_STITCH(DBG_ERR, "chn base_fill_videoframe2buffer fail\n");
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	jobs = &stitch_jobs[grp_id].out;
	if (!jobs) {
		TRACE_STITCH(DBG_ERR, "chn get job failed\n");
		return -1;
	}

	vb = (struct vb_s *)blk;
	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		TRACE_STITCH(DBG_ERR, "chn waitq is full\n");
		mutex_unlock(&jobs->lock);
		return -1;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	atomic_fetch_add(1, &vb->usr_cnt);
	mutex_unlock(&jobs->lock);

	TRACE_STITCH(DBG_INFO, "grp[%d] chn phy_addr(%llx-%llx-%llx) fmt[%d]\n"
		, grp_id, pstvideo_frame->video_frame.phyaddr[0]
		, pstvideo_frame->video_frame.phyaddr[1]
		, pstvideo_frame->video_frame.phyaddr[2]
		, pstvideo_frame->video_frame.pixel_format);
	return 0;
}

int stitch_get_chn_frame(stitch_grp grp_id, video_frame_info_s *pstvideo_frame, int s32milli_sec)
{
	int ret, i;
	vb_blk blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	mmf_chn_s chn = {.mod_id = ID_STITCH, .dev_id = grp_id};

	ret = stitch_check_null_ptr(pstvideo_frame);
	if (ret != 0)
		return ret;

	if (!stitch_ctx_is_enbale(grp_id)) {
		ret = ERR_STITCH_NOTREADY;
		return ret;
	}

	memset(pstvideo_frame, 0, sizeof(*pstvideo_frame));
	ret = base_get_chn_buffer(chn, &stitch_jobs[grp_id].out, &blk, s32milli_sec);
	if (ret != 0 || blk == VB_INVALID_HANDLE) {
		TRACE_STITCH(DBG_ERR, "get chn frame from grp[%d] fail, s32milli_sec=%d, ret=%d\n", grp_id, s32milli_sec, ret);
		return ERR_STITCH_BUF_EMPTY;
	}

	vb = (struct vb_s *)blk;
	if (!vb->buf.phy_addr[0] || !vb->buf.size.width) {
		TRACE_STITCH(DBG_ERR, "buf already released\n");
		return ERR_STITCH_BUF_EMPTY;
	}

	pstvideo_frame->video_frame.pixel_format = stitch_ctx[grp_id]->chn_attr.fmt_out;
	pstvideo_frame->video_frame.width = vb->buf.size.width;
	pstvideo_frame->video_frame.height = vb->buf.size.height;
	pstvideo_frame->video_frame.time_ref = vb->buf.frm_num;
	pstvideo_frame->video_frame.pts = vb->buf.pts;
	for (i = 0; i < 3; ++i) {
		pstvideo_frame->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
		pstvideo_frame->video_frame.length[i] = vb->buf.length[i];
		pstvideo_frame->video_frame.stride[i] = vb->buf.stride[i];
	}

	pstvideo_frame->video_frame.offset_top = vb->buf.offset_top;
	pstvideo_frame->video_frame.offset_bottom = vb->buf.offset_bottom;
	pstvideo_frame->video_frame.offset_left = vb->buf.offset_left;
	pstvideo_frame->video_frame.offset_right = vb->buf.offset_right;
	pstvideo_frame->video_frame.private_data = vb;
	pstvideo_frame->video_frame.align = STITCH_ALIGN;

	ktime_get_ts64(&sw_ts[1]);
	stitch_ctx[grp_id]->work_status.cost_time = get_diff_in_us(sw_ts[0], sw_ts[1]);
	if (stitch_ctx[grp_id]->work_status.cost_time > stitch_ctx[grp_id]->work_status.max_cost_time) {
		stitch_ctx[grp_id]->work_status.max_cost_time = stitch_ctx[grp_id]->work_status.cost_time;
	}

	TRACE_STITCH(DBG_INFO, "grp[%d] end to get pstvideo_frame width:%d height:%d buf:0x%llx, software time cost:[%u] us\n"
		, grp_id
		, pstvideo_frame->video_frame.width
		, pstvideo_frame->video_frame.height
		, pstvideo_frame->video_frame.phyaddr[0]
		, stitch_ctx[grp_id]->work_status.cost_time);
	return 0;
}

int stitch_release_chn_frame(stitch_grp grp_id, const video_frame_info_s *pstvideo_frame)
{
	vb_blk blk;
	int ret;

	ret = stitch_check_null_ptr(pstvideo_frame);
	if (ret != 0)
		return ret;

	ret = stitch_support_fmt(pstvideo_frame->video_frame.pixel_format);
	if (ret != 0) {
		TRACE_STITCH(DBG_DEBUG, "fmt[%d]\n", pstvideo_frame->video_frame.pixel_format);
		return ret;
	}

	blk = vb_phys_addr2handle(pstvideo_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		if (pstvideo_frame->video_frame.private_data == 0) {
			TRACE_STITCH(DBG_ERR, "Grp(%d) phy-address(0x%llx) invalid to locate.\n"
				, grp_id
				, (unsigned long long)pstvideo_frame->video_frame.phyaddr[0]);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		blk = (vb_blk)pstvideo_frame->video_frame.private_data;
	}

	if (vb_release_block(blk) != 0)
		return -1;

	TRACE_STITCH(DBG_DEBUG, "Grp(%d) buf:0x%llx\n", grp_id, pstvideo_frame->video_frame.phyaddr[0]);
	return 0;
}

int stitch_set_src_attr(stitch_grp grp_id, const stitch_src_attr *src_attr)
{
	int ret;
	unsigned char i;
	unsigned char src_num;
	unsigned long flags;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	ret = stitch_check_input_param(src_attr);
	if (ret != 0)
		return ret;

	switch (src_attr->way_num) {
	case STITCH_2_WAY:
		src_num = SRC_NUM_2WAY;
		break;
	case STITCH_4_WAY:
		src_num = SRC_NUM_4WAY;
		break;
	default:
		TRACE_STITCH(DBG_ERR, "not support this waynum:%d\n", src_attr->way_num);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	for (i = 0; i < src_num; i++) {
		ret = stitch_check_yuv_param(src_attr->fmt_in, src_attr->size[i].width, src_attr->size[i].height);
		if (ret != 0)
			return ret;
	}

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	stitch_ctx[grp_id]->src_num = src_num;
	stitch_ctx[grp_id]->param_update = true;
	stitch_ctx[grp_id]->update_status |= STITCH_UPDATE_SRC;
	memcpy(&stitch_ctx[grp_id]->src_attr, src_attr, sizeof(*src_attr));
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	TRACE_STITCH(DBG_DEBUG, "grp[%d]set src_attr+\n", grp_id);
	return 0;
}

int stitch_get_src_attr(stitch_grp grp_id, stitch_src_attr *src_attr)
{
	int ret;
	unsigned long flags;

	ret = stitch_check_null_ptr(src_attr);
	if (ret != 0)
		return ret;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	memcpy(src_attr, &stitch_ctx[grp_id]->src_attr, sizeof(*src_attr));
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	TRACE_STITCH(DBG_DEBUG, "grp[%d] get src_attr+\n", grp_id);
	return 0;
}

static void stitch_cfg_tmp_chn_size(stitch_grp grp_id)
{
	if ((stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[0] == 0)
		&& (stitch_ctx[grp_id]->src_attr.size[0].width == stitch_ctx[grp_id]->src_attr.size[1].width)
		&& (stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[0] + 1) == stitch_ctx[grp_id]->src_attr.size[0].width) //full ovlp
		stitch_ctx[grp_id]->tmp_chn_size[0].width = stitch_ctx[grp_id]->src_attr.size[0].width;
	else {
		if (stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[0] > stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[0]) //none
			stitch_ctx[grp_id]->tmp_chn_size[0].width = stitch_ctx[grp_id]->chn_attr.size.width;
		else if (stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[0] == 0) //lft in rht
			stitch_ctx[grp_id]->tmp_chn_size[0].width = stitch_ctx[grp_id]->src_attr.size[1].width;
		else if ((stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[0] + 1) == stitch_ctx[grp_id]->chn_attr.size.width
			&& stitch_ctx[grp_id]->src_attr.size[0].width == stitch_ctx[grp_id]->chn_attr.size.width) //rht in lft
			stitch_ctx[grp_id]->tmp_chn_size[0].width = stitch_ctx[grp_id]->src_attr.size[0].width;
		else
			stitch_ctx[grp_id]->tmp_chn_size[0].width = stitch_ctx[grp_id]->src_attr.size[0].width - stitch_ctx[grp_id]->src_attr.bd_attr.bd_rx[0]
			+ (stitch_ctx[grp_id]->src_attr.size[1].width - (stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[0] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[0] + 1));
	}

	if (stitch_ctx[grp_id]->src_num == STITCH_MAX_SRC_NUM) {
			stitch_ctx[grp_id]->tmp_chn_size[2].width = stitch_ctx[grp_id]->src_attr.size[2].width
			+ (stitch_ctx[grp_id]->src_attr.size[3].width - (stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_rx[2] - stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[2] + 1));
	}
}

static bool stitch_cfg_depend_done(stitch_grp grp_id, enum stitch_update_status status)
{
	if (unlikely(!stitch_ctx[grp_id])) {
		TRACE_STITCH(DBG_ERR, "ctx grp[%d] not yet creat.\n", grp_id);
		return false;
	}

	if (!(stitch_ctx[grp_id]->update_status & status))
		return false;
	return true;
}

int stitch_set_chn_attr(stitch_grp grp_id, stitch_chn_attr *dst_attr)
{
	int ret;
	unsigned long flags;
	vb_cal_config_s stVbCalConfig;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	if (!stitch_cfg_depend_done(grp_id, STITCH_UPDATE_SRC)) {
		TRACE_STITCH(DBG_ERR, "grp[%d] need cfg src attr before chn attr\n", grp_id);
		return ERR_STITCH_NOT_PERM;
	}

	ret = stitch_check_output_param(grp_id, dst_attr);
	if (ret != 0)
		return ret;

	if (stitch_ctx[grp_id]->src_num == SRC_NUM_2WAY) {
		if (stitch_ctx[grp_id]->src_attr.size[1].width != (dst_attr->size.width
			- stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[0] - stitch_ctx[grp_id]->src_attr.bd_attr.bd_rx[1])) {
			TRACE_STITCH(DBG_ERR, "stitch size[1] with ovlp12 param not match\n");
			return ERR_STITCH_ILLEGAL_PARAM;
		}
	} else if (stitch_ctx[grp_id]->src_num == SRC_NUM_4WAY) {
		if (stitch_ctx[grp_id]->src_attr.size[3].width != (dst_attr->size.width
			- stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[2] + stitch_ctx[grp_id]->src_attr.bd_attr.bd_lx[3]
			+ stitch_ctx[grp_id]->src_attr.bd_attr.bd_rx[3])) {
			TRACE_STITCH(DBG_ERR, "stitch size[3][%d] with ovlp34[%d] param not match\n"
				, stitch_ctx[grp_id]->src_attr.size[3].width, stitch_ctx[grp_id]->src_attr.ovlap_attr.ovlp_lx[2]);
			return ERR_STITCH_ILLEGAL_PARAM;
		}
	} else {
		TRACE_STITCH(DBG_ERR, "invalid src num (%d)\n", stitch_ctx[grp_id]->src_num);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	memcpy(&stitch_ctx[grp_id]->chn_attr, dst_attr, sizeof(*dst_attr));
	common_getpicbufferconfig(stitch_ctx[grp_id]->chn_attr.size.width, stitch_ctx[grp_id]->chn_attr.size.height
				, stitch_ctx[grp_id]->chn_attr.fmt_out, DATA_BITWIDTH_8
				, COMPRESS_MODE_NONE, STITCH_ALIGN, &stVbCalConfig);

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	stitch_ctx[grp_id]->param_update = true;
	stitch_ctx[grp_id]->update_status |= STITCH_UPDATE_CHN;
	stitch_ctx[grp_id]->vb_pool = VB_INVALID_POOLID;
	stitch_ctx[grp_id]->vb_size = stVbCalConfig.vb_size;
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	stitch_cfg_tmp_chn_size(grp_id);

	TRACE_STITCH(DBG_DEBUG, "grp[%d] set chn_attr+ width[%d] height[%d] [fmt_out:%d] [vb_size: %d]\n",
		grp_id,
		stitch_ctx[grp_id]->chn_attr.size.width,
		stitch_ctx[grp_id]->chn_attr.size.height,
		stitch_ctx[grp_id]->chn_attr.fmt_out,
		stitch_ctx[grp_id]->vb_size);
	return 0;
}

int stitch_get_chn_attr(stitch_grp grp_id, stitch_chn_attr *dst_attr)
{
	int ret;
	unsigned long flags;

	ret = stitch_check_null_ptr(dst_attr);
	if (ret != 0)
		return ret;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	memcpy(dst_attr, &stitch_ctx[grp_id]->chn_attr, sizeof(*dst_attr));
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	TRACE_STITCH(DBG_DEBUG, "grp[%d] get chn_attr+\n", grp_id);
	return 0;
}

int stitch_set_op_attr(stitch_grp grp_id, stitch_op_attr *op_attr)
{
	int ret;
	unsigned long flags;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	ret = stitch_check_op_param(op_attr);
	if (ret != 0)
		return ret;

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	stitch_ctx[grp_id]->param_update = true;
	stitch_ctx[grp_id]->update_status |= STITCH_UPDATE_OP;
	memcpy(&stitch_ctx[grp_id]->op_attr, op_attr, sizeof(*op_attr));
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	TRACE_STITCH(DBG_DEBUG, "grp[%d] set op_attr+\n", grp_id);
	return 0;
}

int stitch_get_op_attr(stitch_grp grp_id, stitch_op_attr *op_attr)
{
	int ret;
	unsigned long flags;

	ret = stitch_check_null_ptr(op_attr);
	if (ret != 0)
		return ret;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	memcpy(op_attr, &stitch_ctx[grp_id]->op_attr, sizeof(*op_attr));
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	TRACE_STITCH(DBG_DEBUG, "grp[%d] get op_attr+\n", grp_id);
	return 0;
}

int stitch_set_wgt_attr(stitch_grp grp_id, stitch_bld_wgt_attr *wgt_attr)
{
	int ret;
	unsigned long flags;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	ret = stitch_check_wgt_param(grp_id, wgt_attr);
	if (ret != 0)
		return ret;

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	stitch_ctx[grp_id]->param_update = true;
	stitch_ctx[grp_id]->update_status |= STITCH_UPDATE_WGT;
	memcpy(&stitch_ctx[grp_id]->wgt_attr, wgt_attr, sizeof(*wgt_attr));
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	return 0;
}

int stitch_get_wgt_attr(stitch_grp grp_id, stitch_bld_wgt_attr *wgt_attr)
{
	int ret;
	unsigned long flags;

	ret = stitch_check_null_ptr(wgt_attr);
	if (ret != 0)
		return ret;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	memcpy(wgt_attr, &stitch_ctx[grp_id]->wgt_attr, sizeof(*wgt_attr));
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	TRACE_STITCH(DBG_DEBUG, "grp[%d] get wgt_attr+\n", grp_id);
	return 0;
}

int stitch_set_reg_x(unsigned char regx)
{
	stitch_set_regx(0, regx);
	stitch_set_regx(1, regx);

	TRACE_STITCH(DBG_DEBUG, "set reg_x(%d)\n", regx);
	return 0;
}

int stitch_dump_reg_info(void)
{
	stitch_dump_register();

	TRACE_STITCH(DBG_DEBUG, "dump regs+\n");
	return 0;
}

int stitch_enable_grp(stitch_grp grp_id)
{
	int i;
	unsigned long flags;
	int enable_count;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	if (is_stitch_suspended()) {
		TRACE_STITCH(DBG_ERR, "stitch is suspend.\n");
		return ERR_STITCH_NOT_PERM;
	}

	enable_count = atomic_inc_return(&stitch_ctx[grp_id]->enable_count);
	if (enable_count > 1) {
		TRACE_STITCH(DBG_WARN, "enable grp[%d] enable %d times\n", grp_id, enable_count);
		return 0;
	}

	if (stitch_ctx[grp_id]->is_started) {
		TRACE_STITCH(DBG_WARN, "stitch ctx is already start.\n");
		return 0;
	}

	if (!stitch_param_is_cfg_done(grp_id)) {
		TRACE_STITCH(DBG_ERR, "cfg param is not ready.\n");
		return ERR_STITCH_NOTREADY;
	}

	if (stitch_creat_hw_cfg_obj()) {
		TRACE_STITCH(DBG_ERR, "creat_hw_cfg_obj fail.\n");
		return ERR_STITCH_NOMEM;
	}

	if (stitch_creat_chn_vbq(grp_id))
	{
		TRACE_STITCH(DBG_ERR, "creat chn vbq fail.\n");
		return ERR_STITCH_NOMEM;
	}

	INIT_WORK(&stitch_ctx[grp_id]->work_frm_done, stitch_work_frm_done);

	for (i = 0; i < stitch_ctx[grp_id]->src_num; i++) {
		base_mod_jobs_init(&stitch_jobs[grp_id].ins[i], 2, 2, 0);
	}
	base_mod_jobs_init(&stitch_jobs[grp_id].out, 2, 2, 2);
	stitch_jobs[grp_id].init = true;

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	atomic_set(&stitch_ctx[grp_id]->hdl_state, STITCH_HANDLER_STATE_START);
	atomic_set(&stitch_ctx[grp_id]->job.job_state, STITCH_JOB_WAIT);
	stitch_ctx[grp_id]->is_started = true;
	stitch_ctx[grp_id]->param_update = true;
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);
	stitch_ctx[grp_id]->job.data = (void *)stitch_ctx[grp_id];
	stitch_ctx[grp_id]->job.fn_job_cb = stitch_wkup_frm_done_work;

	TRACE_STITCH(DBG_DEBUG, "enable grp[%d]+ count:%d\n", grp_id, enable_count);
	return 0;
}

int stitch_disable_grp(stitch_grp grp_id)
{
	int i, ret = 0, count = 1000;
	enum stitch_handler_state hdl_state;
	int enable_count;

	if (!stitch_ctx_is_enbale(grp_id)) {
		return 0;
	}

	enable_count = atomic_dec_return(&stitch_ctx[grp_id]->enable_count);

	if (enable_count) {
		TRACE_STITCH(DBG_WARN, "disable grp[%d] %d times\n", grp_id, enable_count);
		return 0;
	}

	while (--count > 0) {
		hdl_state = (enum stitch_handler_state)atomic_read(&stitch_ctx[grp_id]->hdl_state);
		if (hdl_state != STITCH_HANDLER_STATE_RUN && hdl_state != STITCH_HANDLER_STATE_RUN_STAGE2)
			break;
		usleep_range(1000, 2000);
	}
	if (count == 0) {
		TRACE_STITCH(DBG_WARN, "stitch_disable_grp[%d] timeout\n", grp_id);
		return -1;
	}

	atomic_set(&stitch_ctx[grp_id]->hdl_state, STITCH_HANDLER_STATE_STOP);
	atomic_set(&stitch_ctx[grp_id]->job.job_state, STITCH_JOB_INVALID);

	spin_lock(&stitch_ctx[grp_id]->lock);
	stitch_ctx[grp_id]->is_started = false;
	stitch_ctx[grp_id]->param_update = false;
	stitch_ctx[grp_id]->update_status = STITCH_UPDATE_SRC | STITCH_UPDATE_CHN | STITCH_UPDATE_OP | STITCH_UPDATE_WGT;
	spin_unlock(&stitch_ctx[grp_id]->lock);

	stitch_jobs[grp_id].init = false;
	base_mod_jobs_exit(&stitch_jobs[grp_id].out);
	for (i = 0; i < stitch_ctx[grp_id]->src_num; i++) {
		base_mod_jobs_exit(&stitch_jobs[grp_id].ins[i]);
	}

	cancel_work_sync(&stitch_ctx[grp_id]->work_frm_done);

	stitch_destroy_chn_vbq(grp_id);

	TRACE_STITCH(DBG_DEBUG, "disable grp_id[%d] count:%d\n", grp_id, enable_count);
	return ret;
}

int stitch_attach_vb_pool(stitch_grp grp_id, vb_pool vb_pool)
{
	unsigned long flags;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	stitch_ctx[grp_id]->vb_pool = vb_pool;
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	TRACE_STITCH(DBG_DEBUG, "grp[%d] attach vb pool(%d)+\n", grp_id, vb_pool);
	return 0;
}

int stitch_detach_vb_pool(stitch_grp grp_id)
{
	unsigned long flags;

	if (!stitch_ctx_is_creat(grp_id))
		return ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx[grp_id]->lock, flags);
	stitch_ctx[grp_id]->vb_pool = VB_INVALID_POOLID;
	spin_unlock_irqrestore(&stitch_ctx[grp_id]->lock, flags);

	TRACE_STITCH(DBG_DEBUG, "grp[%d] dettach vb pool+\n", grp_id);
	return 0;
}

int stitch_thread_init(void)
{
	int ret;
	struct sched_param tsk;

	stitch_init();
	init_waitqueue_head(&evt_hdl_ctx.wait);
	evt_hdl_ctx.events = 0;
	evt_hdl_ctx.prepared_flag = 0;
	evt_hdl_ctx.core_id = 0;
	evt_hdl_ctx.working_grp = -1;
	FIFO_INIT(&evt_hdl_ctx.grp_workq, STITCH_MAX_GRP_NUM << 1);
	atomic_set(&evt_hdl_ctx.evt_state, STITCH_EVT_STATE_IDLE);
	spin_lock_init(&evt_hdl_ctx.lock);
	sema_init(&evt_hdl_ctx.sem_2nd, 0);

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;

	evt_hdl_ctx.thread = kthread_run(stitch_event_handler_th, &evt_hdl_ctx, "stitch_event_handler_th");
	if (IS_ERR(evt_hdl_ctx.thread)) {
		pr_err("failed to create stitch kthread\n");
		ret = -1;
		goto kthread_fail;
	}

	ret = sched_setscheduler(evt_hdl_ctx.thread, SCHED_FIFO, &tsk);
	if (ret)
		pr_warn("stitch thread priority update failed: %d\n", ret);

	evt_hdl_ctx.workqueue = create_singlethread_workqueue("stitch_workqueue");
	if (!evt_hdl_ctx.workqueue) {
		TRACE_STITCH(DBG_ERR, "stitch evt_hdl_ctx create_workqueue failed.\n");
		ret = -2;
		goto workqueue_creat_fail;
	}

kthread_fail:
workqueue_creat_fail:
	return ret;
}

void stitch_thread_deinit(void)
{
	int ret;

	if (evt_hdl_ctx.workqueue)
		destroy_workqueue(evt_hdl_ctx.workqueue);

	if (!evt_hdl_ctx.thread) {
		pr_err("stitch thread not initialized yet\n");
		return;
	}

	ret = kthread_stop(evt_hdl_ctx.thread);
	if (ret)
		pr_err("fail to stop stitch thread, err=%d\n", ret);

	FIFO_EXIT(&evt_hdl_ctx.grp_workq);
	memset(&evt_hdl_ctx, 0, sizeof(evt_hdl_ctx));
	stitch_deinit();
}

int stitch_suspend_handler(void)
{
	int i;
	enum stitch_handler_state hdlState;
	int count = 100;

	if (!evt_hdl_ctx.thread) {
		TRACE_STITCH(DBG_ERR, "stitch thread not initialized yet\n");
		return ERR_STITCH_NOTREADY;
	}
	stitch_proc_set_timer(false);

	for(i = 0; i< stitch_grp_num; i++) {
		if (!stitch_ctx[i])
			continue;

		while (--count > 0) {
			hdlState = (enum stitch_handler_state)atomic_read(&stitch_ctx[i]->hdl_state);
			if (hdlState == STITCH_HANDLER_STATE_RUN
				|| hdlState == STITCH_HANDLER_STATE_RUN_STAGE2
				|| stitch_is_grp_waitq_ready(i)
				|| stitch_is_grp_workq_ready(i))
				usleep_range(1000, 2000);
			else
				break;
		}
		if (count == 0) {
			TRACE_STITCH(DBG_ERR, "dev Wait timeout, HW hang.\n");
		}

		//release_stitch_jobq(i);

		atomic_set(&stitch_ctx[i]->hdl_state, STITCH_HANDLER_STATE_SUSPEND);
	}

	TRACE_STITCH(DBG_WARN, "suspend handler+\n");
	return 0;
}

int stitch_resume_handler(void)
{
	int i;
	unsigned long flags;

	if (!evt_hdl_ctx.thread) {
		TRACE_STITCH(DBG_ERR, "stitch thread not initialized yet\n");
		return ERR_STITCH_NOTREADY;
	}

	for(i = 0; i< stitch_grp_num; i++) {
		//release_stitch_jobq(i);

		spin_lock_irqsave(&stitch_ctx[i]->lock, flags);
		atomic_set(&stitch_ctx[i]->hdl_state, STITCH_HANDLER_STATE_RESUME);
		spin_unlock_irqrestore(&stitch_ctx[i]->lock, flags);

		TRACE_STITCH(DBG_DEBUG, "resume handler+\n");
	}
	stitch_proc_set_timer(true);

	TRACE_STITCH(DBG_WARN, "resume handler+\n");
	return 0;
}


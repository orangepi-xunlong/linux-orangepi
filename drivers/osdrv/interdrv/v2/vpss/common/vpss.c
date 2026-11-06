#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <asm/div64.h>

#include <base_ctx.h>
#include <linux/defines.h>
#include <linux/common.h>
#include <linux/comm_buffer.h>

#include "base_common.h"
#include "ion.h"
#include "sys.h"
#include "bind.h"
#include <vb.h>
#include "vbq.h"
#include <base_cb.h>
#include <vpss_cb.h>
#include <ldc_cb.h>
#include <vcodec_cb.h>
#include "vi_sys.h"
#include "vpss.h"
#include "vpss_common.h"
#include "vpss_hal.h"
#include "vpss_rgn_ctrl.h"

/*******************************************************
 *  MACRO definition
 ******************************************************/
#ifndef FPGA_PORTING
#define IDLE_TIMEOUT_MS      10000
#define EOF_WAIT_TIMEOUT_MS  1000
#else
#define IDLE_TIMEOUT_MS      60000
#define EOF_WAIT_TIMEOUT_MS  60000
#endif

#define CTX_EVENT_WKUP       0x0001
#define CTX_EVENT_EOF        0x0002
#define CTX_EVENT_VI_ERR     0x0004

struct vpss_jobs_ctx {
	struct vb_jobs_t ins;
	struct vb_jobs_t outs[VPSS_MAX_CHN_NUM];
};

struct vpss_handler_ctx {
	wait_queue_head_t wait;
	struct task_struct *thread;
	spinlock_t hdl_lock;
	atomic_t active_cnt;
	unsigned char events;
};

struct _vpss_gdc_cb_param {
	mmf_chn_s chn;
	enum gdc_usage usage;
};

struct vpss_ext_ctx {
	struct csc_cfg csc_cfg;
	struct csc_cfg chn_csc_cfg[VPSS_MAX_CHN_NUM];
	signed int proc_amp[PROC_AMP_MAX];
};

struct vpss_stitch_data {
	wait_queue_head_t wait;
	unsigned char flag;
};

static struct vpss_ctx *g_vpss_ctx[VPSS_MAX_GRP_NUM] = { [0 ... VPSS_MAX_GRP_NUM - 1] = NULL };

// vpss_ctx in uapi, internal extension version.
static struct vpss_ext_ctx g_vpss_ext_ctx[VPSS_MAX_GRP_NUM];

static struct vpss_jobs_ctx g_vpss_vb_jobs[VPSS_MAX_GRP_NUM];

static unsigned char g_is_bm_scene = false;

// Motion level for vcodec
static struct mlv_i_s g_mlv_i[VI_MAX_DEV_NUM];

static struct workqueue_struct *g_vpss_workqueue;

//update proc info
static void _update_vpss_chn_real_frame_rate(struct timer_list *timer);
DEFINE_TIMER(timer_proc, _update_vpss_chn_real_frame_rate);
static atomic_t g_timer_added = ATOMIC_INIT(0);

//vpss job prepare
static struct vpss_handler_ctx g_vpss_hdl_ctx;

//global lock
static struct mutex g_vpss_lock;

//Get Available Grp lock
static unsigned char g_vpss_grp_used[VPSS_MAX_GRP_NUM];

//timer callback
static vpss_timer_cb g_core_cb;
static void *g_core_data;

static struct gdc_mesh g_vpss_mesh[VPSS_MAX_GRP_NUM][VPSS_MAX_CHN_NUM];

static proc_amp_ctrl_s g_procamp_ctrls[PROC_AMP_MAX] = {
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
};

static vi_vpss_mode_s g_vi_vpss_mode;
static vpss_mod_param_s g_vpss_mod_param;

static struct semaphore g_vpss_core_sem;

static inline signed int check_vpss_grp_created(vpss_grp grp)
{
	if (!g_vpss_ctx[grp] || !g_vpss_ctx[grp]->is_created) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) isn't created yet.\n", grp);
		return ERR_VPSS_UNEXIST;
	}
	return 0;
}

static inline signed int check_vpss_chn_valid(vpss_grp grp_id, vpss_chn chn_id)
{

	if ((chn_id >= VPSS_MAX_CHN_NUM) || (chn_id < 0)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) invalid channel ID.\n", grp_id, chn_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	return 0;
}

signed int check_vpss_id(vpss_grp grp_id, vpss_chn chn_id)
{
	signed int ret = 0;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;
	return ret;
}

void vpss_notify_wkup_evt(void)
{
	spin_lock(&g_vpss_hdl_ctx.hdl_lock);
	g_vpss_hdl_ctx.events |= CTX_EVENT_WKUP;
	spin_unlock(&g_vpss_hdl_ctx.hdl_lock);
	wake_up_interruptible(&g_vpss_hdl_ctx.wait);
}

void vpss_wkup_frame_done_handle(void *pdata)
{
	struct vpss_job *job = container_of(pdata, struct vpss_job, data);

	if(!g_is_bm_scene)
		queue_work(g_vpss_workqueue, &job->job_work);
	else {
		struct vpss_stitch_data *data = (struct vpss_stitch_data *)job->data;
		data->flag = 1;
		wake_up(&data->wait);
	}
}

struct vpss_ctx **vpss_get_ctx(void)
{
	return g_vpss_ctx;
}

static unsigned char vpss_online_is_idle(void)
{
	int i;

	for (i = 0; i < VPSS_ONLINE_NUM; i++)
		if (g_vpss_ctx[i] && g_vpss_ctx[i]->is_created && g_vpss_ctx[i]->is_started)
			return true;

	return false;
}

static signed int _mesh_gdc_do_op_cb(enum gdc_usage usage, const void *usage_param,
				struct vb_s *vb_in, pixel_format_e pixel_format, unsigned long long mesh_addr,
				unsigned char sync_io, void *pcb_param, unsigned int param_size,
				mod_id_e mod_id, rotation_e rotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	TRACE_VPSS(DBG_DEBUG, "push jobs(%d) for gdc\n", usage);

	memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.usage_param = usage_param;
	cfg.vb_in = vb_in;
	cfg.pix_format = pixel_format;
	cfg.mesh_addr = mesh_addr;
	cfg.sync_io = sync_io;
	cfg.cb_param = pcb_param;
	cfg.cb_param_size = param_size;
	cfg.rotation = rotation;

	exe_cb.callee = E_MODULE_LDC;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = LDC_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;

	return base_exe_module_cb(&exe_cb);
}

/* aspect_ratio_resize: calculate the new rect to keep aspect ratio
 *   according to given in/out size.
 *
 * @param in: input video size.
 * @param out: output display size.
 *
 * @return: the rect which describe the video on output display.
 */
static rect_s aspect_ratio_resize(size_s in, size_s out)
{
	rect_s rect;
	unsigned int scale = in.height * in.width;
	unsigned int ratio_int = MIN2(out.width * in.height, out.height * in.width);
	unsigned long long height, width;

	//float ratio = MIN2((float)out.width / in.width, (float)out.height / in.height);
	//rect.height = (float)in.height * ratio + 0.5;
	//rect.width = (float)in.width * ratio + 0.5;
	//rect.x = (out.width - rect.width) >> 1;
	//rect.y = (out.height - rect.height) >> 1;

	height = (unsigned long long)in.height * ratio_int + scale/2;
	do_div(height, scale);
	rect.height = (unsigned int)height;

	width = (unsigned long long)in.width * ratio_int + scale/2;
	do_div(width, scale);
	rect.width = (unsigned int)width;

	rect.x = (out.width - rect.width) >> 1;
	rect.y = (out.height - rect.height) >> 1;

	return rect;
}

/**************************************************************************
 *   Job related APIs.
 **************************************************************************/
void vpss_gdc_callback(void *param, vb_blk blk)
{
	struct _vpss_gdc_cb_param *gdc_cb_param = param;

	if (!param)
		return;

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) usage(%d)\n", gdc_cb_param->chn.dev_id,
		       gdc_cb_param->chn.chn_id, gdc_cb_param->usage);
	mutex_unlock(&g_vpss_mesh[gdc_cb_param->chn.dev_id][gdc_cb_param->chn.chn_id].lock);
	if (blk != VB_INVALID_HANDLE) {
		if (!vpss_check_suspend()) {
			vb_done_handler(gdc_cb_param->chn, CHN_TYPE_OUT,
				&g_vpss_vb_jobs[gdc_cb_param->chn.dev_id].outs[gdc_cb_param->chn.chn_id], blk);
		} else {
			vb_release_block(blk);
		}
	}
	vfree(param);
}

static unsigned char get_work_mask(struct vpss_ctx *ctx)
{
	unsigned char mask = 0;
	vpss_chn chn_id;

	if (!ctx->is_created || !ctx->is_started)
		return 0;

	for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
		if (!ctx->chn_cfgs[chn_id].is_enabled)
			continue;
		mask |= BIT(chn_id);
	}
	if (mask == 0)
		return 0;

	// img's mask
	mask |= BIT(7);

	return mask;
}

static void _vpss_fill_buffer(vpss_chn chn_id, struct video_buffer *grp_buf,
		uint64_t phy_addr, struct vpss_ctx *ctx, struct video_buffer *buf)
{
	size_s size;
	unsigned char ldc_wa = false;

	//workaround for ldc 64-align for width/height.
	if (ctx->chn_cfgs[chn_id].rotation != ROTATION_0)
		ldc_wa = true;

	if (ldc_wa) {
		size.width = ALIGN(ctx->chn_cfgs[chn_id].chn_attr.width, LDC_ALIGN);
		size.height = ALIGN(ctx->chn_cfgs[chn_id].chn_attr.height, LDC_ALIGN);
	} else {
		size.width = ctx->chn_cfgs[chn_id].chn_attr.width;
		size.height = ctx->chn_cfgs[chn_id].chn_attr.height;
	}
	base_get_frame_info(ctx->chn_cfgs[chn_id].chn_attr.pixel_format
			   , size
			   , buf
			   , phy_addr
			   , ctx->chn_cfgs[chn_id].align);
	buf->offset_top = 0;
	buf->offset_bottom =
		size.height - ctx->chn_cfgs[chn_id].chn_attr.height;
	buf->offset_left = 0;
	buf->offset_right =
		size.width - ctx->chn_cfgs[chn_id].chn_attr.width;

	if (grp_buf) {
		buf->pts = grp_buf->pts;
		buf->frm_num = grp_buf->frm_num;
		buf->motion_lv = grp_buf->motion_lv;
		memcpy(buf->motion_table, grp_buf->motion_table, MO_TBL_SIZE);
	}
}

void job_fill_buf(struct video_buffer *buf, unsigned long long *addr)
{
	unsigned char i;

	for (i = 0; i < NUM_OF_PLANES; ++i)
		addr[i] = buf->phy_addr[i];

	if (buf->pixel_format == PIXEL_FORMAT_BGR_888_PLANAR) {
		addr[0] = buf->phy_addr[2];
		addr[2] = buf->phy_addr[0];
	}

	if (buf->compress_mode == COMPRESS_MODE_FRAME)
		addr[3] = buf->compress_expand_addr;
}

static signed int vpss_qbuf(mmf_chn_s chn, struct video_buffer *grp_buf,
	vb_blk blk, struct vpss_ctx *ctx, struct vpss_job *job)
{
	signed int ret;
	struct vb_s *vb = (struct vb_s *)blk;

	_vpss_fill_buffer(chn.chn_id, grp_buf, vb_handle2phys_addr(blk), ctx, &vb->buf);

	ret = vb_qbuf(chn, CHN_TYPE_OUT, &g_vpss_vb_jobs[chn.dev_id].outs[chn.chn_id], blk);
	if (ret != 0)
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) qbuf failed\n", chn.dev_id, chn.chn_id);
	else
		job_fill_buf(&vb->buf, job->cfg.chn_cfg[chn.chn_id].addr);

	vb_release_block(blk);

	return ret;
}

static signed int vpss_online_qbuf(mmf_chn_s chn, void *data)
{
	struct vpss_ctx *ctx;
	struct vpss_job *job = (struct vpss_job *)data;
	vb_blk blk;

	if (check_vpss_grp_valid(chn.dev_id))
		return 0;
	if (!g_vpss_ctx[chn.dev_id])
		return 0;
	if (!g_vpss_ctx[chn.dev_id]->chn_cfgs[chn.chn_id].is_enabled)
		return 0;

	ctx = g_vpss_ctx[chn.dev_id];
	blk = vb_get_block_with_id(ctx->chn_cfgs[chn.chn_id].vb_pool,
		ctx->chn_cfgs[chn.chn_id].blk_size, ID_VPSS);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for VPSS\n"
			, chn.dev_id, chn.chn_id);
		return -1;
	}

	TRACE_VPSS(DBG_NOTICE, "Grp(%d) Chn(%d) acquire VB BLK\n", chn.dev_id, chn.chn_id);
	return vpss_qbuf(chn, NULL, blk, ctx, job);
}

static signed int fill_buffers(struct vpss_ctx *ctx, struct vpss_job *job)
{
	signed int ret = 0;
	vb_blk blk[VPSS_MAX_CHN_NUM] = { [0 ... VPSS_MAX_CHN_NUM - 1] = VB_INVALID_HANDLE };
	vb_blk blk_grp;
	vpss_grp grp_id = ctx->vpss_grp;
	unsigned char online_from_isp = ctx->online_from_isp;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = 0};
	vpss_chn chn_id = 0;
	struct vpss_chn_cfg *chn_cfg;
	struct video_buffer *buf;
	struct video_buffer *buf_in = NULL;
	vb_pool pool_id = VB_INVALID_POOLID;

	if (!online_from_isp && base_mod_jobs_waitq_empty(&g_vpss_vb_jobs[grp_id].ins))
		return ERR_VPSS_BUF_EMPTY;

	// get buffers.
	for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
		chn_cfg = &ctx->chn_cfgs[chn_id];
		if (!chn_cfg->is_enabled)
			continue;
		if (chn_cfg->is_drop)
			continue;

		chn.chn_id = chn_id;
		job->cfg.chn_cfg[chn_id].addr[0] = 0;

		// chn buffer from user
		if (!base_mod_jobs_waitq_empty(&g_vpss_vb_jobs[grp_id].outs[chn_id])) {
			TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) chn buffer from user.\n", grp_id, chn_id);

			buf = base_mod_jobs_enque_work(&g_vpss_vb_jobs[grp_id].outs[chn_id]);
			if (!buf) {
				TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) qbuf failed.\n", grp_id, chn_id);
				ret = ERR_VPSS_NOTREADY;
				break;
			}

			// Implement qbuf in user space
			job_fill_buf(buf, job->cfg.chn_cfg[chn_id].addr);
			job->cfg.chn_cfg[chn_id].bytesperline[0] = buf->stride[0];
			job->cfg.chn_cfg[chn_id].bytesperline[1] = buf->stride[1];
			continue;
		}

		// chn buffer from pool
		blk[chn_id] = vb_get_block_with_id(ctx->chn_cfgs[chn_id].vb_pool,
						ctx->chn_cfgs[chn_id].blk_size, ID_VPSS);
		if (blk[chn_id] == VB_INVALID_HANDLE) {
			if (online_from_isp) {
				if (ctx->chn_cfgs[chn_id].vb_pool == VB_INVALID_POOLID) {
					pool_id = find_vb_pool(ctx->chn_cfgs[chn_id].blk_size);
					ctx->chn_cfgs[chn_id].chn_work_status.pool_id = pool_id;
				} else {
					pool_id = ctx->chn_cfgs[chn_id].vb_pool;
				}
				vb_acquire_block(vpss_online_qbuf, chn, pool_id, job);
				TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d) acquire VB BLK later\n"
					, grp_id, chn_id);
			} else {
				TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for VPSS\n"
					, grp_id, chn_id);
				ret = ERR_VPSS_NOBUF;
				break;
			}
		}
	}
	if (ret != 0)
		goto ERR_FILL_BUF;

	if (!online_from_isp) {
		buf_in = base_mod_jobs_enque_work(&g_vpss_vb_jobs[grp_id].ins);
		if (buf_in == NULL) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) qbuf failed.\n", grp_id);
			ret = ERR_VPSS_NOTREADY;
			goto ERR_FILL_BUF;
		}

		TRACE_VPSS(DBG_DEBUG, "grp(%d) buf: 0x%lx-0x%lx-0x%lx\n", grp_id,
			(unsigned long)buf_in->phy_addr[0], (unsigned long)buf_in->phy_addr[1],
			(unsigned long)buf_in->phy_addr[2]);

		job_fill_buf(buf_in, job->cfg.grp_cfg.addr);
	}

	for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
		if (blk[chn_id] == VB_INVALID_HANDLE)
			continue;
		chn.chn_id = chn_id;

		vpss_qbuf(chn, buf_in, blk[chn_id], ctx, job);
	}

	return ret;
ERR_FILL_BUF:
	while ((chn_id > 0) && (--chn_id < VPSS_MAX_CHN_NUM)) {
		if (blk[chn_id] != VB_INVALID_HANDLE)
			vb_release_block(blk[chn_id]);
	}
	blk_grp = base_mod_jobs_waitq_pop(&g_vpss_vb_jobs[grp_id].ins);
	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);

	return ret;
}

static void release_buffers(struct vpss_ctx *ctx)
{
	vpss_grp grp_id = ctx->vpss_grp;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = 0};
	vb_blk blk;
	vpss_chn chn_id;
	vb_pool pool_id = VB_INVALID_POOLID;

	if (!ctx->online_from_isp) {
		chn.chn_id = 0;
		if (!base_mod_jobs_workq_empty(&g_vpss_vb_jobs[grp_id].ins)) {
			vb_dqbuf(chn, &g_vpss_vb_jobs[grp_id].ins, &blk);
			if (blk != VB_INVALID_HANDLE)
				vb_release_block(blk);
		}
	}

	for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
		struct vpss_chn_cfg *chn_cfg = &ctx->chn_cfgs[chn_id];

		if (!chn_cfg->is_enabled)
			continue;

		chn.chn_id = chn_id;

		if (ctx->chn_cfgs[chn_id].vb_pool == VB_INVALID_POOLID)
			pool_id = find_vb_pool(ctx->chn_cfgs[chn_id].blk_size);
		else
			pool_id = ctx->chn_cfgs[chn_id].vb_pool;
		vb_cancel_block(chn, pool_id);

		while (!base_mod_jobs_workq_empty(&g_vpss_vb_jobs[grp_id].outs[chn_id])) {
			vb_dqbuf(chn, &g_vpss_vb_jobs[grp_id].outs[chn_id], &blk);
			if (blk != VB_INVALID_HANDLE)
				vb_release_block(blk);
		}
	}
}

static void _release_vpss_waitq(mmf_chn_s chn, enum chn_type_e chn_type)
{
	vb_blk blk_grp;

	if (chn_type == CHN_TYPE_OUT)
		blk_grp = base_mod_jobs_waitq_pop(&g_vpss_vb_jobs[chn.dev_id].outs[chn.chn_id]);
	else
		blk_grp = base_mod_jobs_waitq_pop(&g_vpss_vb_jobs[chn.dev_id].ins);

	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);
}

static void _vpss_online_set_mlv_info(struct vb_s *vb)
{
	unsigned char snr_num = vb->buf.dev_num;

	vb->buf.motion_lv = g_mlv_i[snr_num].mlv_i_level;
	memcpy(vb->buf.motion_table, g_mlv_i[snr_num].mlv_i_table, sizeof(vb->buf.motion_table));
}

static signed int _vpss_online_get_dpcm_wr_crop(unsigned char snr_num,
	rect_s *dpcm_wr_crop, size_s src_size)
{
#if 0
	struct crop_size crop = g_dpcm_wr_i.dpcm_wr_i_crop[snr_num];
	signed int ret = 0;

	if (g_dpcm_wr_i.dpcm_wr_i_dpcmon) {
		// check if dpcm_wr crop valid
		if (crop.end_x <= crop.start_x ||
			crop.end_y <= crop.start_y ||
			crop.end_x > src_size.width ||
			crop.end_y > src_size.height ||
			((unsigned int)(crop.end_x - crop.start_x) == src_size.width &&
			(unsigned int)(crop.end_y - crop.start_y) == src_size.height))
			ret = ERR_VPSS_ILLEGAL_PARAM;
		else {
			dpcm_wr_crop->x = (signed int)crop.start_x;
			dpcm_wr_crop->y = (signed int)crop.start_y;
			dpcm_wr_crop->width = (unsigned int)(crop.end_x - crop.start_x);
			dpcm_wr_crop->height = (unsigned int)(crop.end_y - crop.start_y);
		}
	} else
		ret = ERR_VPSS_NOT_PERM;

	return ret;
#else
	if (snr_num >= VPSS_MAX_GRP_NUM || !dpcm_wr_crop || !src_size.width)
		return -1;

	return ERR_VPSS_NOT_PERM;
#endif
}

/*
 * _vpss_get_union_crop() - get union crop area of crop_a & crop_b.
 * If two crop area has no union, return crop_a
 */
static rect_s _vpss_get_union_crop(rect_s crop_a, rect_s crop_b)
{
	rect_s union_crop;

	// check if no union
	if ((crop_a.x >= crop_b.x + (signed int)crop_b.width) ||
		(crop_a.y >= crop_b.y + (signed int)crop_b.height) ||
		(crop_b.x >= crop_a.x + (signed int)crop_a.width) ||
		(crop_b.y >= crop_a.y + (signed int)crop_a.height))
		return crop_a;

	union_crop.x = (crop_a.x > crop_b.x) ? crop_a.x : crop_b.x;
	union_crop.y = (crop_a.y > crop_b.y) ? crop_a.y : crop_b.y;
	union_crop.width =
		((crop_a.x + (signed int)crop_a.width) < (crop_b.x + (signed int)crop_b.width)) ?
		(unsigned int)(crop_a.x + (signed int)crop_a.width - union_crop.x) :
		(unsigned int)(crop_b.x + (signed int)crop_b.width - union_crop.x);
	union_crop.height =
		((crop_a.y + (signed int)crop_a.height) < (crop_b.y + (signed int)crop_b.height)) ?
		(unsigned int)(crop_a.y + (signed int)crop_a.height - union_crop.y) :
		(unsigned int)(crop_b.y + (signed int)crop_b.height - union_crop.y);

	return union_crop;
}

/* _is_frame_crop_changed() - to see if frame's crop info changed
 */
static unsigned char _is_frame_crop_changed(vpss_grp grp_id, struct vpss_ctx *ctx)
{
	struct vb_s *vb_in = NULL;
	unsigned char ret = false;
	struct vb_jobs_t *jobs;

	if (base_mod_jobs_waitq_empty(&g_vpss_vb_jobs[grp_id].ins))
		return false;

	jobs = &g_vpss_vb_jobs[grp_id].ins;
	FIFO_GET_FRONT(&jobs->waitq, &vb_in);

	if (vb_in == NULL) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) unexpected empty waitq\n", grp_id);
		return false;
	}

	//GDC 64 align case
	if ((ctx->offset_left != vb_in->buf.offset_left) ||
		(ctx->offset_top != vb_in->buf.offset_top) ||
		(ctx->offset_right != vb_in->buf.offset_right) ||
		(ctx->offset_bottom != vb_in->buf.offset_bottom)) {
		ctx->offset_left = vb_in->buf.offset_left;
		ctx->offset_top = vb_in->buf.offset_top;
		ctx->offset_right = vb_in->buf.offset_right;
		ctx->offset_bottom = vb_in->buf.offset_bottom;
		ret = true;
	}

	//dis case
	if (memcmp(&vb_in->buf.frame_crop, &ctx->frame_crop, sizeof(vb_in->buf.frame_crop))) {
		unsigned char chk_width_even = IS_FMT_YUV420(ctx->grp_attr.pixel_format) ||
				      IS_FMT_YUV422(ctx->grp_attr.pixel_format);
		unsigned char chk_height_even = IS_FMT_YUV420(ctx->grp_attr.pixel_format);

		if (chk_width_even && ((vb_in->buf.frame_crop.end_x - vb_in->buf.frame_crop.start_x) & 0x01)) {
			TRACE_VPSS(DBG_WARN, "grp_id(%d) frame-crop invalid - start_x(%d) end_x(%d)\n",
				       grp_id, vb_in->buf.frame_crop.start_x, vb_in->buf.frame_crop.end_x);
			TRACE_VPSS(DBG_WARN, "frame-crop's width should be even for yuv format\n");
			return ret;
		}
		if (chk_height_even && ((vb_in->buf.frame_crop.end_y - vb_in->buf.frame_crop.start_y) & 0x01)) {
			TRACE_VPSS(DBG_WARN, "grp_id(%d) frame-crop invalid - start_y(%d) end_y(%d)\n",
				       grp_id, vb_in->buf.frame_crop.start_y, vb_in->buf.frame_crop.end_y);
			TRACE_VPSS(DBG_WARN, "frame-crop's height should be even for yuv format\n");
			return ret;
		}

		ctx->frame_crop = vb_in->buf.frame_crop;
		ret = true;
	}
	return ret;
}

/* _is_frame_crop_valid() - to see if frame's crop info valid/enabled
 */
static bool _is_frame_crop_valid(struct vpss_ctx *ctx)
{
	return (ctx->frame_crop.end_x > ctx->frame_crop.start_x &&
		ctx->frame_crop.end_y > ctx->frame_crop.start_y &&
		ctx->frame_crop.end_x <= ctx->grp_attr.w &&
		ctx->frame_crop.end_y <= ctx->grp_attr.h &&
		!((unsigned int)(ctx->frame_crop.end_x - ctx->frame_crop.start_x)
			== ctx->grp_attr.w &&
		  (unsigned int)(ctx->frame_crop.end_y - ctx->frame_crop.start_y)
			== ctx->grp_attr.h));
}

static void _vpss_over_crop_resize
	(struct vpss_hal_grp_cfg *grp_hw_cfg, rect_s crop_rect, rect_s *resize_rect)
{
	unsigned int scale = crop_rect.width * crop_rect.height;
	unsigned int ratio;

	TRACE_VPSS(DBG_DEBUG, "rect_crop (l=%d, t=%d, w=%d, h=%d)\n",
			grp_hw_cfg->crop.left,
			grp_hw_cfg->crop.top,
			grp_hw_cfg->crop.width,
			grp_hw_cfg->crop.height);
	TRACE_VPSS(DBG_DEBUG, "rect before resize(%d, %d, %d, %d)\n"
				, resize_rect->x, resize_rect->y
				, resize_rect->width, resize_rect->height);

	if (crop_rect.x < 0) {
		//ratio = (float)ABS(crop_rect.x) / crop_rect.width;
		//resize_rect->x += (signed int)(resize_rect->width * ratio + 0.5);
		//resize_rect->width -= (unsigned int)(resize_rect->width * ratio + 0.5);
		ratio = ABS(crop_rect.x) * crop_rect.height;
		resize_rect->x += (signed int)(resize_rect->width * ratio + scale / 2) / scale;
		resize_rect->width -= (unsigned int)(resize_rect->width * ratio + scale / 2) / scale;
	}

	if (crop_rect.x + crop_rect.width > grp_hw_cfg->crop.width) {
		//ratio = (float)(crop_rect.x + crop_rect.width - grp_hw_cfg->rect_crop.width)
		//	/ (crop_rect.width);
		//resize_rect->width -= (unsigned int)(resize_rect->width * ratio + 0.5);
		ratio = (crop_rect.x + crop_rect.width - grp_hw_cfg->crop.width)
			* (crop_rect.height);
		resize_rect->width -= (unsigned int)(resize_rect->width * ratio + scale / 2) / scale;
	}

	if (crop_rect.y < 0) {
		//ratio = (float)ABS(crop_rect.y) / crop_rect.height;
		//resize_rect->y += (signed int)(resize_rect->height * ratio + 0.5);
		//resize_rect->height -= (unsigned int)(resize_rect->height * ratio + 0.5);
		ratio = ABS(crop_rect.y) * crop_rect.width;
		resize_rect->y += (signed int)(resize_rect->height * ratio + scale / 2) / scale;
		resize_rect->height -= (unsigned int)(resize_rect->height * ratio + scale / 2) / scale;
	}

	if (crop_rect.y + crop_rect.height > grp_hw_cfg->crop.height) {
		//ratio = (float)(crop_rect.y + crop_rect.height - grp_hw_cfg->rect_crop.height)
		//	/ (crop_rect.height);
		//resize_rect->height -= (unsigned int)(resize_rect->height * ratio + 0.5);
		ratio = (crop_rect.y + crop_rect.height - grp_hw_cfg->crop.height)
			* (crop_rect.width);
		resize_rect->height -= (unsigned int)(resize_rect->height * ratio + scale / 2) / scale;
	}

	TRACE_VPSS(DBG_INFO, "rect after resize(%d, %d, %d, %d)\n"
			, resize_rect->x, resize_rect->y
			, resize_rect->width, resize_rect->height);
}

/*
 * @param chn_id: VPSS Chn to update cfg
 * @param ctx: VPSS ctx which records settings of this grp
 */
void _vpss_chn_hw_cfg_update(vpss_chn chn_id, struct vpss_ctx *ctx)
{
	unsigned char i;
	vpss_grp grp_id = ctx->vpss_grp;
	unsigned char online_from_isp = ctx->online_from_isp;
	struct vpss_hal_grp_cfg *grp_hw_cfg = &ctx->hw_cfg.grp_cfg;
	struct vpss_chn_cfg *chn_cfg = &ctx->chn_cfgs[chn_id];
	struct vpss_hal_chn_cfg *hw_chn_cfg = &ctx->hw_cfg.chn_cfg[chn_id];
	vb_cal_config_s vb_cal_config;
	rect_s chn_crop = chn_cfg->crop_info.crop_rect;
	unsigned char crop_over_src_range = false;

	common_getpicbufferconfig(chn_cfg->chn_attr.width, chn_cfg->chn_attr.height,
		chn_cfg->chn_attr.pixel_format, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, chn_cfg->align, &vb_cal_config);
	hw_chn_cfg->bytesperline[0] = vb_cal_config.main_stride;
	hw_chn_cfg->bytesperline[1] = vb_cal_config.c_stride;

	if (chn_cfg->crop_info.enable) {
		//FLOAT h_ratio = 1.0f, v_ratio = 1.0f;
		unsigned int scale = ctx->grp_attr.w * ctx->grp_attr.h;
		unsigned int h_ratio = scale, v_ratio = scale;
		unsigned long long left, right, height, width;

		if (!online_from_isp) {
			// use ratio-coordinate if dis enabled.
			if (_is_frame_crop_valid(ctx)) {
				//h_ratio = (FLOAT)grp_hw_cfg->rect_crop.width / ctx->grp_attr.w;
				//v_ratio = (FLOAT)grp_hw_cfg->rect_crop.height / ctx->grp_attr.h;
				h_ratio = grp_hw_cfg->crop.width * ctx->grp_attr.h;
				v_ratio = grp_hw_cfg->crop.height * ctx->grp_attr.w;
			}
		} else {
			rect_s dpcm_wr_crop;
			size_s chn_src_size;
			signed int ret;

			chn_src_size.width = grp_hw_cfg->crop.width;
			chn_src_size.height = grp_hw_cfg->crop.height;
			ret = _vpss_online_get_dpcm_wr_crop(grp_id, &dpcm_wr_crop, chn_src_size);
			if (ret == 0)
				chn_crop = _vpss_get_union_crop(dpcm_wr_crop, chn_crop);
		}
		//hw_chn_cfg->rect_crop.left = chn_crop.x = chn_crop.x * h_ratio;
		//hw_chn_cfg->rect_crop.top = chn_crop.y = chn_crop.y * v_ratio;
		//hw_chn_cfg->rect_crop.width = chn_crop.width = chn_crop.width * h_ratio;
		//hw_chn_cfg->rect_crop.height = chn_crop.height = chn_crop.height * v_ratio;
		left = chn_crop.x * (s64)h_ratio;
		do_div(left, scale);
		hw_chn_cfg->crop.left = chn_crop.x = left;

		right = chn_crop.y * (s64)v_ratio;
		do_div(right, scale);
		hw_chn_cfg->crop.top = chn_crop.y = right;

		width = chn_crop.width * (s64)h_ratio;
		do_div(width, scale);
		hw_chn_cfg->crop.width = chn_crop.width = (unsigned int)width;

		height = chn_crop.height * (s64)v_ratio;
		do_div(height, scale);
		hw_chn_cfg->crop.height = chn_crop.height = (unsigned int)height;

		// check if crop rect contains the region outside input src
		if (chn_crop.x < 0) {
			hw_chn_cfg->crop.left = 0;
			hw_chn_cfg->crop.width = (chn_crop.width - ABS(chn_crop.x));
			crop_over_src_range = true;
		}
		if (chn_crop.x + chn_crop.width > grp_hw_cfg->crop.width) {
			hw_chn_cfg->crop.width = grp_hw_cfg->crop.width - hw_chn_cfg->crop.left;
			crop_over_src_range = true;
		}

		if (chn_crop.y < 0) {
			hw_chn_cfg->crop.top = 0;
			hw_chn_cfg->crop.height = (chn_crop.height - ABS(chn_crop.y));
			crop_over_src_range = true;
		}
		if (chn_crop.y + chn_crop.height > grp_hw_cfg->crop.height) {
			hw_chn_cfg->crop.height = grp_hw_cfg->crop.height - hw_chn_cfg->crop.top;
			crop_over_src_range = true;
		}
	} else {
		hw_chn_cfg->crop.left = hw_chn_cfg->crop.top
			= chn_crop.x = chn_crop.y = 0;
		hw_chn_cfg->crop.width = chn_crop.width = grp_hw_cfg->crop.width;
		hw_chn_cfg->crop.height = chn_crop.height = grp_hw_cfg->crop.height;
		if (online_from_isp) {
			rect_s dpcm_wr_crop;
			size_s chn_src_size;
			signed int ret;

			chn_src_size.width = grp_hw_cfg->crop.width;
			chn_src_size.height = grp_hw_cfg->crop.height;
			ret = _vpss_online_get_dpcm_wr_crop(grp_id, &dpcm_wr_crop, chn_src_size);
			if (ret == 0) {
				hw_chn_cfg->crop.left = chn_crop.x = dpcm_wr_crop.x;
				hw_chn_cfg->crop.top = chn_crop.y = dpcm_wr_crop.y;
				hw_chn_cfg->crop.width = chn_crop.width = dpcm_wr_crop.width;
				hw_chn_cfg->crop.height = chn_crop.height = dpcm_wr_crop.height;
			}
		}
	}
	TRACE_VPSS(DBG_DEBUG, "grp(%d) chn(%d) rect(%d %d %d %d)\n", grp_id, chn_id
			, hw_chn_cfg->crop.left, hw_chn_cfg->crop.top
			, hw_chn_cfg->crop.width, hw_chn_cfg->crop.height);

	if (chn_cfg->chn_attr.aspect_ratio.mode == ASPECT_RATIO_AUTO) {
		size_s in, out;
		rect_s rect;
		unsigned char is_border_enabled = false;

		in.width = chn_crop.width;
		in.height = chn_crop.height;
		out.width = chn_cfg->chn_attr.width;
		out.height = chn_cfg->chn_attr.height;
		rect = aspect_ratio_resize(in, out);

		if (crop_over_src_range)
			_vpss_over_crop_resize(grp_hw_cfg, chn_crop, &rect);

		is_border_enabled = chn_cfg->chn_attr.aspect_ratio.enable_bgcolor
			&& ((rect.width != chn_cfg->chn_attr.width)
			 || (rect.height != chn_cfg->chn_attr.height));

		TRACE_VPSS(DBG_INFO, "input(%d %d) output(%d %d)\n"
				, in.width, in.height, out.width, out.height);
		TRACE_VPSS(DBG_INFO, "ratio (%d %d %d %d) border_enabled(%d)\n"
				, rect.x, rect.y, rect.width, rect.height, is_border_enabled);

		hw_chn_cfg->border_cfg.enable = is_border_enabled;
		hw_chn_cfg->border_cfg.offset_x = rect.x;
		hw_chn_cfg->border_cfg.offset_y = rect.y;
		hw_chn_cfg->border_cfg.bg_color[2] = chn_cfg->chn_attr.aspect_ratio.bgcolor & 0xff;
		hw_chn_cfg->border_cfg.bg_color[1] = (chn_cfg->chn_attr.aspect_ratio.bgcolor >> 8) & 0xff;
		hw_chn_cfg->border_cfg.bg_color[0] = (chn_cfg->chn_attr.aspect_ratio.bgcolor >> 16) & 0xff;

		if (is_border_enabled) {
			hw_chn_cfg->dst_rect.left = hw_chn_cfg->dst_rect.top = 0;
		} else {
			hw_chn_cfg->dst_rect.left = rect.x;
			hw_chn_cfg->dst_rect.top = rect.y;
		}


		if (IS_FMT_YUV420(chn_cfg->chn_attr.pixel_format)
			|| IS_FMT_YUV422(chn_cfg->chn_attr.pixel_format)) {
			hw_chn_cfg->dst_rect.width = rect.width & ~0x01;
			hw_chn_cfg->dst_rect.left &= ~0x01;
		} else
			hw_chn_cfg->dst_rect.width = rect.width;

		if (IS_FMT_YUV420(chn_cfg->chn_attr.pixel_format))
			hw_chn_cfg->dst_rect.height = rect.height & ~0x01;
		else
			hw_chn_cfg->dst_rect.height = rect.height;
	} else if (chn_cfg->chn_attr.aspect_ratio.mode == ASPECT_RATIO_MANUAL) {
		rect_s rect = chn_cfg->chn_attr.aspect_ratio.video_rect;
		unsigned char is_border_enabled = false;

		if (crop_over_src_range)
			_vpss_over_crop_resize(grp_hw_cfg, chn_crop, &rect);

		is_border_enabled = chn_cfg->chn_attr.aspect_ratio.enable_bgcolor
			&& ((rect.width != chn_cfg->chn_attr.width)
			 || (rect.height != chn_cfg->chn_attr.height));

		TRACE_VPSS(DBG_INFO, "rect(%d %d %d %d) border_enabled(%d)\n"
				, rect.x, rect.y, rect.width, rect.height, is_border_enabled);

		if (is_border_enabled) {
			hw_chn_cfg->dst_rect.left = hw_chn_cfg->dst_rect.top = 0;
		} else {
			hw_chn_cfg->dst_rect.left = rect.x;
			hw_chn_cfg->dst_rect.top = rect.y;
		}
		hw_chn_cfg->dst_rect.width = rect.width;
		hw_chn_cfg->dst_rect.height = rect.height;

		hw_chn_cfg->border_cfg.enable = is_border_enabled;
		hw_chn_cfg->border_cfg.offset_x = rect.x;
		hw_chn_cfg->border_cfg.offset_y = rect.y;
		hw_chn_cfg->border_cfg.bg_color[2] = chn_cfg->chn_attr.aspect_ratio.bgcolor & 0xff;
		hw_chn_cfg->border_cfg.bg_color[1] = (chn_cfg->chn_attr.aspect_ratio.bgcolor >> 8) & 0xff;
		hw_chn_cfg->border_cfg.bg_color[0] = (chn_cfg->chn_attr.aspect_ratio.bgcolor >> 16) & 0xff;
	} else {
		rect_s rect;

		rect.x = rect.y = 0;
		rect.width = chn_cfg->chn_attr.width;
		rect.height = chn_cfg->chn_attr.height;
		if (crop_over_src_range)
			_vpss_over_crop_resize(grp_hw_cfg, chn_crop, &rect);

		hw_chn_cfg->dst_rect.left = hw_chn_cfg->dst_rect.top = 0;
		hw_chn_cfg->dst_rect.width = rect.width;
		hw_chn_cfg->dst_rect.height = rect.height;
		if (crop_over_src_range) {
			hw_chn_cfg->border_cfg.enable = true;
			hw_chn_cfg->border_cfg.offset_x = rect.x;
			hw_chn_cfg->border_cfg.offset_y = rect.y;
			hw_chn_cfg->border_cfg.bg_color[2] = 0;
			hw_chn_cfg->border_cfg.bg_color[1] = 0;
			hw_chn_cfg->border_cfg.bg_color[0] = 0;
		} else {
			hw_chn_cfg->border_cfg.enable = false;
		}
	}

	if (hw_chn_cfg->dst_rect.width * VPSS_MAX_ZOOMOUT  < hw_chn_cfg->crop.width
		|| hw_chn_cfg->dst_rect.height * VPSS_MAX_ZOOMOUT  < hw_chn_cfg->crop.height) {
		TRACE_VPSS(DBG_ERR, "zoom out over %d times, sc in(w:%d, h:%d), sc out(w:%d, h:%d)\n"
			, VPSS_MAX_ZOOMOUT, hw_chn_cfg->crop.width, hw_chn_cfg->crop.height
			, hw_chn_cfg->dst_rect.width, hw_chn_cfg->dst_rect.height);

		hw_chn_cfg->crop.width = hw_chn_cfg->dst_rect.width * VPSS_MAX_ZOOMOUT;
		hw_chn_cfg->crop.height = hw_chn_cfg->dst_rect.height * VPSS_MAX_ZOOMOUT;
		TRACE_VPSS(DBG_ERR, "Modify to sc in(w:%d, h:%d), sc out(w:%d, h:%d)\n"
			, hw_chn_cfg->crop.width, hw_chn_cfg->crop.height
			, hw_chn_cfg->dst_rect.width, hw_chn_cfg->dst_rect.height);
	}

	hw_chn_cfg->quant_cfg.enable = chn_cfg->chn_attr.normalize.enable;
	if (chn_cfg->chn_attr.normalize.enable) {
		struct vpss_int_normalize *int_norm =
			(struct vpss_int_normalize *)&ctx->chn_cfgs[chn_id].chn_attr.normalize;

		for (i = 0; i < 3; i++) {
			hw_chn_cfg->quant_cfg.sc_frac[i] = int_norm->sc_frac[i];
			hw_chn_cfg->quant_cfg.sub[i] = int_norm->sub[i];
			hw_chn_cfg->quant_cfg.sub_frac[i] = int_norm->sub_frac[i];
		}

		TRACE_VPSS(DBG_DEBUG, "sc_frac(0x%x, 0x%x, 0x%x)\n",
			hw_chn_cfg->quant_cfg.sc_frac[0],
			hw_chn_cfg->quant_cfg.sc_frac[1],
			hw_chn_cfg->quant_cfg.sc_frac[2]);

		TRACE_VPSS(DBG_DEBUG, "sub(0x%x, 0x%x, 0x%x), sub_frac(0x%x, 0x%x, 0x%x)\n",
			hw_chn_cfg->quant_cfg.sub[0],
			hw_chn_cfg->quant_cfg.sub[1],
			hw_chn_cfg->quant_cfg.sub[2],
			hw_chn_cfg->quant_cfg.sub_frac[0],
			hw_chn_cfg->quant_cfg.sub_frac[1],
			hw_chn_cfg->quant_cfg.sub_frac[2]);

		hw_chn_cfg->quant_cfg.rounding = (enum sc_quant_rounding)int_norm->rounding;
	}else {
		hw_chn_cfg->y_ratio = chn_cfg->y_ratio;
	}

	switch (chn_cfg->coef) {
	default:
	case VPSS_SCALE_COEF_BICUBIC:
		hw_chn_cfg->sc_coef = SC_SCALING_COEF_BICUBIC;
		break;
	case VPSS_SCALE_COEF_BILINEAR:
		hw_chn_cfg->sc_coef = SC_SCALING_COEF_BILINEAR;
		break;
	case VPSS_SCALE_COEF_NEAREST:
		hw_chn_cfg->sc_coef = SC_SCALING_COEF_NEAREST;
		break;
	case VPSS_SCALE_COEF_BICUBIC_OPENCV:
		hw_chn_cfg->sc_coef = SC_SCALING_COEF_BICUBIC_OPENCV;
		break;
	}

	for (i = 0; i < VPSS_RECT_NUM; i++) {
		rect_s rect;
		unsigned short thick;

		hw_chn_cfg->border_vpp_cfg[i].enable = chn_cfg->draw_rect.rects[i].enable;
		if (hw_chn_cfg->border_vpp_cfg[i].enable) {
			hw_chn_cfg->border_vpp_cfg[i].bg_color[0] = (chn_cfg->draw_rect.rects[i].bg_color >> 16) & 0xff;
			hw_chn_cfg->border_vpp_cfg[i].bg_color[1] = (chn_cfg->draw_rect.rects[i].bg_color >> 8) & 0xff;
			hw_chn_cfg->border_vpp_cfg[i].bg_color[2] = chn_cfg->draw_rect.rects[i].bg_color & 0xff;

			rect = chn_cfg->draw_rect.rects[i].rect;
			thick = chn_cfg->draw_rect.rects[i].thick;
			if ((rect.x + rect.width) > chn_cfg->chn_attr.width)
				rect.width = chn_cfg->chn_attr.width - rect.x;
			if ((rect.y + rect.height) > chn_cfg->chn_attr.height)
				rect.height = chn_cfg->chn_attr.height - rect.y;

			hw_chn_cfg->border_vpp_cfg[i].outside.start_x = rect.x;
			hw_chn_cfg->border_vpp_cfg[i].outside.start_y = rect.y;
			hw_chn_cfg->border_vpp_cfg[i].outside.end_x = rect.x + rect.width;
			hw_chn_cfg->border_vpp_cfg[i].outside.end_y = rect.y + rect.height;
			hw_chn_cfg->border_vpp_cfg[i].inside.start_x = rect.x + thick;
			hw_chn_cfg->border_vpp_cfg[i].inside.start_y = rect.y + thick;
			hw_chn_cfg->border_vpp_cfg[i].inside.end_x =
				hw_chn_cfg->border_vpp_cfg[i].outside.end_x - thick;
			hw_chn_cfg->border_vpp_cfg[i].inside.end_y =
				hw_chn_cfg->border_vpp_cfg[i].outside.end_y - thick;
		}
	}

	hw_chn_cfg->convert_to_cfg.enable = chn_cfg->convert.enable;
	if (hw_chn_cfg->convert_to_cfg.enable) {
		hw_chn_cfg->convert_to_cfg.a_frac[0] = chn_cfg->convert.a_factor[0];
		hw_chn_cfg->convert_to_cfg.a_frac[1] = chn_cfg->convert.a_factor[1];
		hw_chn_cfg->convert_to_cfg.a_frac[2] = chn_cfg->convert.a_factor[2];
		hw_chn_cfg->convert_to_cfg.b_frac[0] = chn_cfg->convert.b_factor[0];
		hw_chn_cfg->convert_to_cfg.b_frac[1] = chn_cfg->convert.b_factor[1];
		hw_chn_cfg->convert_to_cfg.b_frac[2] = chn_cfg->convert.b_factor[2];
	}

	memcpy(&hw_chn_cfg->csc_cfg, &g_vpss_ext_ctx[grp_id].chn_csc_cfg[chn_id], sizeof(hw_chn_cfg->csc_cfg));

	TRACE_VPSS(DBG_DEBUG, "hw_chn_cfg coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
		, hw_chn_cfg->csc_cfg.coef[0][0]
		, hw_chn_cfg->csc_cfg.coef[0][1]
		, hw_chn_cfg->csc_cfg.coef[0][2]);
	TRACE_VPSS(DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
		, hw_chn_cfg->csc_cfg.coef[1][0]
		, hw_chn_cfg->csc_cfg.coef[1][1]
		, hw_chn_cfg->csc_cfg.coef[1][2]);
	TRACE_VPSS(DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
		, hw_chn_cfg->csc_cfg.coef[2][0]
		, hw_chn_cfg->csc_cfg.coef[2][1]
		, hw_chn_cfg->csc_cfg.coef[2][2]);
	TRACE_VPSS(DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
		, hw_chn_cfg->csc_cfg.sub[0]
		, hw_chn_cfg->csc_cfg.sub[1]
		, hw_chn_cfg->csc_cfg.sub[2]);
	TRACE_VPSS(DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
		, hw_chn_cfg->csc_cfg.add[0]
		, hw_chn_cfg->csc_cfg.add[1]
		, hw_chn_cfg->csc_cfg.add[2]);
}

/*
 * @param ctx: VPSS ctx which records settings of this grp
 * @param grp_hw_cfg: cfg to be updated
 */
void _vpss_grp_hw_cfg_update(struct vpss_ctx *ctx)
{
	vpss_grp grp_id = ctx->vpss_grp;
	vb_cal_config_s vb_cal_config;
	//struct sclr_csc_matrix *mtrx;
	struct vpss_hal_grp_cfg *grp_hw_cfg = &ctx->hw_cfg.grp_cfg;

	common_getpicbufferconfig(ctx->grp_attr.w, ctx->grp_attr.h,
		ctx->grp_attr.pixel_format, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &vb_cal_config);
	grp_hw_cfg->bytesperline[0] = vb_cal_config.main_stride;
	grp_hw_cfg->bytesperline[1] = vb_cal_config.c_stride;

	// frame_crop applied if valid
	if (_is_frame_crop_valid(ctx)) {
		// for frame crop.
		rect_s grp_crop;

		grp_crop.x = ctx->frame_crop.start_x;
		grp_crop.y = ctx->frame_crop.start_y;
		grp_crop.width = ctx->frame_crop.end_x - ctx->frame_crop.start_x;
		grp_crop.height = ctx->frame_crop.end_y - ctx->frame_crop.start_y;
		if (ctx->grp_crop_info.enable)
			grp_crop = _vpss_get_union_crop(grp_crop, ctx->grp_crop_info.crop_rect);

		grp_hw_cfg->crop.left = grp_crop.x + ctx->offset_left;
		grp_hw_cfg->crop.top = grp_crop.y + ctx->offset_top;
		grp_hw_cfg->crop.width = grp_crop.width;
		grp_hw_cfg->crop.height = grp_crop.height;
		TRACE_VPSS(DBG_DEBUG, "grp(%d) use frame crop.\n", grp_id);
	} else {
		// for grp crop.
		if (ctx->grp_crop_info.enable) {
			grp_hw_cfg->crop.width = ctx->grp_crop_info.crop_rect.width;
			grp_hw_cfg->crop.height = ctx->grp_crop_info.crop_rect.height;
			grp_hw_cfg->crop.left = ctx->grp_crop_info.crop_rect.x +
				ctx->offset_left;
			grp_hw_cfg->crop.top = ctx->grp_crop_info.crop_rect.y +
				ctx->offset_top;
			TRACE_VPSS(DBG_DEBUG, "grp(%d) use GrpCrop.\n", grp_id);
		} else {
			grp_hw_cfg->crop.left = ctx->offset_left;
			grp_hw_cfg->crop.top = ctx->offset_top;
			grp_hw_cfg->crop.width = ctx->grp_attr.w;
			grp_hw_cfg->crop.height = ctx->grp_attr.h;
		}
	}
	memcpy(&grp_hw_cfg->csc_cfg, &g_vpss_ext_ctx[grp_id].csc_cfg, sizeof(grp_hw_cfg->csc_cfg));

	TRACE_VPSS(DBG_INFO, "grp(%d) Offset(left:%d top:%d right:%d bottom:%d) rect(%d %d %d %d)\n"
			, grp_id, ctx->offset_left, ctx->offset_top, ctx->offset_right, ctx->offset_bottom
			, grp_hw_cfg->crop.left, grp_hw_cfg->crop.top
			, grp_hw_cfg->crop.width, grp_hw_cfg->crop.height);

	TRACE_VPSS(DBG_DEBUG, "coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
		, grp_hw_cfg->csc_cfg.coef[0][0]
		, grp_hw_cfg->csc_cfg.coef[0][1]
		, grp_hw_cfg->csc_cfg.coef[0][2]);
	TRACE_VPSS(DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
		, grp_hw_cfg->csc_cfg.coef[1][0]
		, grp_hw_cfg->csc_cfg.coef[1][1]
		, grp_hw_cfg->csc_cfg.coef[1][2]);
	TRACE_VPSS(DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
		, grp_hw_cfg->csc_cfg.coef[2][0]
		, grp_hw_cfg->csc_cfg.coef[2][1]
		, grp_hw_cfg->csc_cfg.coef[2][2]);
	TRACE_VPSS(DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
		, grp_hw_cfg->csc_cfg.sub[0]
		, grp_hw_cfg->csc_cfg.sub[1]
		, grp_hw_cfg->csc_cfg.sub[2]);
	TRACE_VPSS(DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
		, grp_hw_cfg->csc_cfg.add[0]
		, grp_hw_cfg->csc_cfg.add[1]
		, grp_hw_cfg->csc_cfg.add[2]);

}

static signed int commit_hw_settings(struct vpss_ctx *ctx)
{
	vpss_grp grp_id = ctx->vpss_grp;
	unsigned char online_from_isp = ctx->online_from_isp;
	vpss_grp_attr_s *grp_attr;
	struct vpss_chn_cfg *chn_cfg;
	struct vpss_hal_grp_cfg *hw_grp_cfg = &ctx->hw_cfg.grp_cfg;
	struct vpss_hal_chn_cfg *hw_chn_cfg;
	struct vb_s *vb_in = NULL;
	vpss_chn chn_id;
	struct vb_jobs_t *jobs;
	unsigned char is_grp_changed = false;
	unsigned char chn_num = 0, i;
	struct mutex *rgn_mutex;
	struct rgn_canvas_ctx *rgn_canvas;
	struct rgn_canvas_q *rgn_canvas_waitq;
	struct rgn_canvas_q *rgn_canvas_doneq;

	grp_attr = &ctx->grp_attr;

	if (!online_from_isp && _is_frame_crop_changed(grp_id, ctx))
		ctx->is_cfg_changed = true; //DIS

	if (ctx->is_cfg_changed) {
		_vpss_grp_hw_cfg_update(ctx);
		ctx->is_cfg_changed = false;
		is_grp_changed = true;
	}

	if (!online_from_isp) {
		if (!base_mod_jobs_waitq_empty(&g_vpss_vb_jobs[grp_id].ins)) {
			jobs = &g_vpss_vb_jobs[grp_id].ins;
			FIFO_GET_FRONT(&jobs->waitq, &vb_in);
		}
	}

	hw_grp_cfg->online_from_isp = online_from_isp;
	hw_grp_cfg->src_size.width = grp_attr->w;
	hw_grp_cfg->src_size.height = grp_attr->h;
	hw_grp_cfg->pixelformat = grp_attr->pixel_format;
	hw_grp_cfg->bytesperline[0] = (vb_in != NULL)
				? vb_in->buf.stride[0] : hw_grp_cfg->bytesperline[0];
	hw_grp_cfg->bytesperline[1] = (vb_in != NULL)
				? vb_in->buf.stride[1] : hw_grp_cfg->bytesperline[1];
	hw_grp_cfg->fbd_enable = (vb_in != NULL)
				? (vb_in->buf.compress_mode == COMPRESS_MODE_FRAME) : false;
	hw_grp_cfg->upsample = false;

	for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
		chn_cfg = &ctx->chn_cfgs[chn_id];
		hw_chn_cfg = &ctx->hw_cfg.chn_cfg[chn_id];
		ctx->hw_cfg.chn_enable[chn_id] = chn_cfg->is_enabled && (!chn_cfg->is_drop);

		if (!chn_cfg->is_enabled)
			continue;
		if (chn_cfg->is_drop)
			continue;
		chn_num++;

		if (is_grp_changed || chn_cfg->is_cfg_changed) {
			_vpss_chn_hw_cfg_update(chn_id, ctx);
			chn_cfg->is_cfg_changed = false;
		}

		TRACE_VPSS(DBG_DEBUG, "grp(%d) chn(%d) size(%d %d) rect(%d %d %d %d)\n", grp_id, chn_id
				, chn_cfg->chn_attr.width, chn_cfg->chn_attr.height
				, hw_chn_cfg->crop.left, hw_chn_cfg->crop.top
				, hw_chn_cfg->crop.width, hw_chn_cfg->crop.height);

		hw_chn_cfg->pixelformat = chn_cfg->chn_attr.pixel_format;
		hw_chn_cfg->src_size.width = hw_grp_cfg->crop.width;
		hw_chn_cfg->src_size.height = hw_grp_cfg->crop.height;
		hw_chn_cfg->dst_size.width = chn_cfg->chn_attr.width;
		hw_chn_cfg->dst_size.height = chn_cfg->chn_attr.height;
		memcpy(hw_chn_cfg->rgn_cfg, chn_cfg->rgn_cfg, sizeof(chn_cfg->rgn_cfg));
		for (i = 0; i < RGN_MAX_LAYER_VPSS; ++i) {
			if (chn_cfg->rgn_cfg[i].rgn_lut_cfg.is_updated) {
				chn_cfg->rgn_cfg[i].rgn_lut_cfg.is_updated = false;
			}
			if (chn_cfg->rgn_cfg[i].odec.enable && chn_cfg->rgn_cfg[i].odec.canvas_updated) {
				rgn_canvas_waitq =
					(struct rgn_canvas_q *)chn_cfg->rgn_cfg[i].odec.rgn_canvas_waitq;
				rgn_canvas_doneq =
					(struct rgn_canvas_q *)chn_cfg->rgn_cfg[i].odec.rgn_canvas_doneq;
				rgn_mutex = (struct mutex *)chn_cfg->rgn_cfg[i].odec.canvas_mutex_lock;
				mutex_lock(rgn_mutex);
				if (FIFO_SIZE(rgn_canvas_doneq) != 2) {
					TRACE_VPSS(DBG_ERR
						, "grp(%d) chn(%d) rgn layer(%d) doneq size isn't right.\n"
						, grp_id, chn_id, i);
					chn_cfg->rgn_cfg[i].odec.canvas_updated = false;
					mutex_unlock(rgn_mutex);
				} else {
					FIFO_POP(rgn_canvas_doneq, &rgn_canvas);
					FIFO_PUSH(rgn_canvas_waitq, rgn_canvas);
					chn_cfg->rgn_cfg[i].odec.canvas_updated = false;
					mutex_unlock(rgn_mutex);
				}
			}
		}


		hw_chn_cfg->rgn_coverex_cfg = chn_cfg->rgn_coverex_cfg;
		hw_chn_cfg->rgn_mosaic_cfg = chn_cfg->rgn_mosaic_cfg;

		if (chn_cfg->chn_attr.flip && chn_cfg->chn_attr.mirror)
			hw_chn_cfg->flip = SC_FLIP_HVFLIP;
		else if (chn_cfg->chn_attr.flip)
			hw_chn_cfg->flip = SC_FLIP_VFLIP;
		else if (chn_cfg->chn_attr.mirror)
			hw_chn_cfg->flip = SC_FLIP_HFLIP;
		else
			hw_chn_cfg->flip = SC_FLIP_NO;
		hw_chn_cfg->mute_cfg.enable = chn_cfg->is_muted;
		hw_chn_cfg->mute_cfg.color[0] = 0;
		hw_chn_cfg->mute_cfg.color[1] = 0;
		hw_chn_cfg->mute_cfg.color[2] = 0;

		if (VPSS_UPSAMPLE(grp_attr->pixel_format, chn_cfg->chn_attr.pixel_format) && (!ctx->is_copy_upsample))
			hw_grp_cfg->upsample = true;
	}

	ctx->hw_cfg.chn_num = chn_num;

	return 0;
}

static void _update_vpss_chn_real_frame_rate(struct timer_list *timer)
{
	int i, j;
	unsigned long long duration, cur_time_us;
	struct timespec64 cur_time;

	UNUSED(timer);
	ktime_get_ts64(&cur_time);
	cur_time_us = (unsigned long long)cur_time.tv_sec * USEC_PER_SEC + cur_time.tv_nsec / NSEC_PER_USEC;

	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (g_vpss_ctx[i] && g_vpss_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				if (g_vpss_ctx[i]->chn_cfgs[j].is_enabled) {
					duration = cur_time_us - g_vpss_ctx[i]->chn_cfgs[j].chn_work_status.prev_time;
					if (duration >= 1000000) {
						g_vpss_ctx[i]->chn_cfgs[j].chn_work_status.real_frame_rate
							= g_vpss_ctx[i]->chn_cfgs[j].chn_work_status.frame_num;
						g_vpss_ctx[i]->chn_cfgs[j].chn_work_status.frame_num = 0;
						g_vpss_ctx[i]->chn_cfgs[j].chn_work_status.prev_time = cur_time_us;
					}
				}
			}
		}
	}
	g_core_cb(g_core_data);

	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
}

/* _vpss_chl_frame_rate_ctrl: dynamically disabled chn per frame-rate-ctrl
 *
 * @param proc_ctx: the frame statics for reference
 * @param ctx: the working settings
 */
static signed int simplify_rate(unsigned int dst_in, unsigned int src_in, unsigned int *dst_out, unsigned int *src_out)
{
	unsigned int i = 1;
	unsigned int a, b;

	while (i < dst_in + 1) {
		a = dst_in % i;
		b = src_in % i;
		if (a == 0 && b == 0) {
			dst_in = dst_in / i;
			src_in = src_in / i;
			i = 1;
		}
		i++;
	}
	*dst_out = dst_in;
	*src_out = src_in;
	return 0;
}

static unsigned char vpss_frame_ctrl(unsigned long long frame_index, frame_rate_ctrl_s *frame_rate)
{
	unsigned int src_simp;
	unsigned int dst_simp;
	unsigned int index;
	unsigned int src_duration, dst_duration;
	unsigned int cur_index, next_index;

	simplify_rate(frame_rate->dst_frame_rate, frame_rate->src_frame_rate,
		&dst_simp, &src_simp);

	index = frame_index % src_simp;
	if (index == 0) {
		return true;
	}
	src_duration = 100;
	dst_duration = (src_duration * src_simp) / dst_simp;
	cur_index = (index - 1) * src_duration / dst_duration;
	next_index = index * src_duration / dst_duration;

	if (next_index == cur_index)
		return false;

	return true;
}

static unsigned char _vpss_chl_frame_rate_ctrl(struct vpss_ctx *ctx, unsigned char working_mask)
{
	vpss_chn chn_id;

	if (!ctx)
		return 0;
	if (!ctx->is_created || !ctx->is_started)
		return 0;

	for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
		if (!ctx->chn_cfgs[chn_id].is_enabled || !ctx->chn_cfgs[chn_id].chn_work_status.send_ok)
			continue;
		ctx->chn_cfgs[chn_id].is_drop = false;
		if (FRC_INVALID(ctx->chn_cfgs[chn_id].chn_attr.frame_rate))
			continue;
		if (!vpss_frame_ctrl(ctx->grp_work_status.frc_recv_cnt - 1,
			&ctx->chn_cfgs[chn_id].chn_attr.frame_rate)) {
			ctx->chn_cfgs[chn_id].is_drop = true;
			working_mask &= ~BIT(chn_id);
			TRACE_VPSS(DBG_DEBUG, "chn[%d] frame index(%d) drop\n", chn_id,
				ctx->grp_work_status.frc_recv_cnt);
		}
	}

	if (!(working_mask & 0xf))
		working_mask = 0;

	return working_mask;
}

static void _update_vpss_grp_proc(vpss_grp grp_id, unsigned int duration, unsigned int hw_duration)
{
	struct vpss_ctx *ctx;

	if (!g_vpss_ctx[grp_id])
		return;

	ctx = g_vpss_ctx[grp_id];

	ctx->grp_work_status.cost_time = duration;
	if (ctx->grp_work_status.max_cost_time <
		ctx->grp_work_status.cost_time) {
		ctx->grp_work_status.max_cost_time
			= ctx->grp_work_status.cost_time;
	}
	ctx->grp_work_status.hw_cost_time = hw_duration;
	if (ctx->grp_work_status.hw_max_cost_time <
		ctx->grp_work_status.hw_cost_time) {
		ctx->grp_work_status.hw_max_cost_time
			= ctx->grp_work_status.hw_cost_time;
	}
}

static void _update_vpss_chn_proc(vpss_grp grp_id, vpss_chn chn_id)
{
	struct vpss_chn_work_status_s *chn_status;

	if (!g_vpss_ctx[grp_id])
		return;

	chn_status = &g_vpss_ctx[grp_id]->chn_cfgs[chn_id].chn_work_status;
	chn_status->send_ok++;
	chn_status->frame_num++;
}

static unsigned char _vpss_check_gdc_job(mmf_chn_s chn, vb_blk blk, struct vpss_ctx *ctx)
{
	struct gdc_mesh *pmesh;

	pmesh = &g_vpss_mesh[chn.dev_id][chn.chn_id];
	if (mutex_trylock(&pmesh->lock)) {
		if (ctx->chn_cfgs[chn.chn_id].ldc_attr.enable) {
			struct vb_s *vb = (struct vb_s *)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};

			if (_mesh_gdc_do_op_cb(GDC_USAGE_LDC
				, &ctx->chn_cfgs[chn.chn_id].ldc_attr.attr
				, vb
				, ctx->chn_cfgs[chn.chn_id].chn_attr.pixel_format
				, pmesh->paddr
				, false, &cb_param
				, sizeof(cb_param)
				, ID_VPSS
				, ctx->chn_cfgs[chn.chn_id].ldc_attr.attr.rotation) != 0) {
				mutex_unlock(&pmesh->lock);
				TRACE_VPSS(DBG_ERR, "gdc LDC failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return false;
			}
			return true;
		} else if (ctx->chn_cfgs[chn.chn_id].fisheye_attr.enable) {
			struct vb_s *vb = (struct vb_s *)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_FISHEYE};
			if (_mesh_gdc_do_op_cb(GDC_USAGE_FISHEYE
				, &ctx->chn_cfgs[chn.chn_id].fisheye_attr
				, vb
				, ctx->chn_cfgs[chn.chn_id].chn_attr.pixel_format
				, pmesh->paddr
				, false, &cb_param
				, sizeof(cb_param)
				, ID_VPSS
				, ctx->chn_cfgs[chn.chn_id].rotation) != 0) {
				mutex_unlock(&pmesh->lock);
				TRACE_VPSS(DBG_ERR, "gdc FishEye failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return false;
			}
			return true;
		} else if (ctx->chn_cfgs[chn.chn_id].rotation != ROTATION_0) {
			struct vb_s *vb = (struct vb_s *)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn,
				.usage = GDC_USAGE_ROTATION };

			if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
				, NULL
				, vb
				, ctx->chn_cfgs[chn.chn_id].chn_attr.pixel_format
				, pmesh->paddr
				, false, &cb_param
				, sizeof(cb_param)
				, ID_VPSS
				, ctx->chn_cfgs[chn.chn_id].rotation) != 0) {
				mutex_unlock(&pmesh->lock);
				TRACE_VPSS(DBG_ERR, "gdc rotation failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return false;
			}
			return true;
		}
		mutex_unlock(&pmesh->lock);
	} else {
		TRACE_VPSS(DBG_WARN, "grp(%d) chn(%d) drop frame due to gdc op blocked.\n",
			chn.dev_id, chn.chn_id);
		// release blk if gdc not done yet
		vb_release_block(blk);
		return true;
	}

	return false;
}

static void vpss_online_unprepare(void)
{
	vpss_hal_online_release_dev();
}

static signed int vpss_online_prepare(vpss_grp working_grp)
{
	unsigned char i, working_mask = 0;
	struct vpss_ctx *vpss_ctx_local = NULL;
	struct vpss_job *past_job[VPSS_ONLINE_JOB_NUM] = {[0 ... VPSS_ONLINE_JOB_NUM - 1] = NULL};

	vpss_ctx_local = g_vpss_ctx[working_grp];
	mutex_lock(&vpss_ctx_local->lock);

	// sc's mask
	working_mask = get_work_mask(vpss_ctx_local);
	if (working_mask)
		working_mask = _vpss_chl_frame_rate_ctrl(vpss_ctx_local, working_mask);
	if (working_mask == 0) {
		TRACE_VPSS(DBG_NOTICE, "grp(%d) working_mask zero.\n", working_grp);
		goto err;
	}

	// commit hw settings of this vpss-grp.
	if (commit_hw_settings(vpss_ctx_local) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) apply hw settings NG.\n", working_grp);
		vpss_ctx_local->grp_work_status.start_fail_cnt++;
		goto err;
	}

	for (i = 0; i < VPSS_ONLINE_JOB_NUM; i++) {
		if (FIFO_EMPTY(&vpss_ctx_local->jobq)) {
			TRACE_VPSS(DBG_ERR, "vpss(%d) jobq empty.\n", working_grp);
			goto err;
		}
		FIFO_POP(&vpss_ctx_local->jobq, &past_job[i]);
		memcpy(&past_job[i]->cfg, &vpss_ctx_local->hw_cfg, sizeof(vpss_ctx_local->hw_cfg));
		past_job[i]->working_mask = working_mask;

		if (fill_buffers(vpss_ctx_local, past_job[i]) != 0) {
			TRACE_VPSS(DBG_ERR, "grp(%d) fill buffer NG.\n", working_grp);
			vpss_ctx_local->grp_work_status.start_fail_cnt++;
			goto err;
		}
	}

	ktime_get_ts64(&vpss_ctx_local->time);
	atomic_set(&vpss_ctx_local->hdl_state, HANDLER_STATE_RUN);
	for (i = 0; i < VPSS_ONLINE_JOB_NUM; i++)
		vpss_hal_push_job(past_job[i]);
	mutex_unlock(&vpss_ctx_local->lock);

	TRACE_VPSS(DBG_DEBUG, "Online Grp(%d) post job.\n", working_grp);
	vpss_hal_try_schedule();

	return 0;

err:
	for (i = 0; i < VPSS_ONLINE_JOB_NUM; i++)
		if (past_job[i])
			FIFO_PUSH(&vpss_ctx_local->jobq, past_job[i]);
	mutex_unlock(&vpss_ctx_local->lock);

	return -1;
}

static signed int vpss_online_full_job(vpss_grp working_grp, struct vpss_job *job)
{
	unsigned char working_mask = 0;
	struct vpss_ctx *vpss_ctx_local = NULL;

	vpss_ctx_local = g_vpss_ctx[working_grp];

	// sc's mask
	working_mask = get_work_mask(vpss_ctx_local);
	if (working_mask)
		working_mask = _vpss_chl_frame_rate_ctrl(vpss_ctx_local, working_mask);
	if (working_mask == 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) working_mask zero.\n", working_grp);
		goto err;
	}

	// commit hw settings of this vpss-grp.
	if (commit_hw_settings(vpss_ctx_local) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) apply hw settings NG.\n", working_grp);
		vpss_ctx_local->grp_work_status.start_fail_cnt++;
		goto err;
	}

	memcpy(&job->cfg, &vpss_ctx_local->hw_cfg, sizeof(vpss_ctx_local->hw_cfg));
	job->working_mask = working_mask;

	if (fill_buffers(vpss_ctx_local, job) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) fill buffer NG.\n", working_grp);
		vpss_ctx_local->grp_work_status.start_fail_cnt++;
		goto err;
	}

	ktime_get_ts64(&vpss_ctx_local->time);
	atomic_set(&vpss_ctx_local->hdl_state, HANDLER_STATE_RUN);

	TRACE_VPSS(DBG_DEBUG, "Online Grp(%d) post job.\n", working_grp);

	return 0;

err:
	return -1;
}

void vpss_handle_online_frame_done(struct vpss_job *job)
{
	struct vpss_ctx *ctx = (struct vpss_ctx *)job->data;
	vpss_grp working_grp = job->grp_id;
	unsigned char working_mask = job->working_mask;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = working_grp, .chn_id = 0};
	vb_blk blk;
	struct vb_s *vb;
	vpss_chn chn_id;
	unsigned int duration;
	struct timespec64 time;
	unsigned long long pts;

	if (working_grp >= VPSS_ONLINE_NUM) {
		TRACE_VPSS(DBG_ERR, "Online Grp(%d) invalid\n", working_grp);
		return;
	}
	TRACE_VPSS(DBG_INFO, "grp(%d) eof\n", working_grp);
	ctx->grp_work_status.recv_cnt++;
	ctx->grp_work_status.frc_recv_cnt++;
	chn_id = 0;
	ktime_get_ts64(&time);
	duration = get_diff_in_us(ctx->time, time);

	do {
		if (!(working_mask & BIT(chn_id)))
			continue;

		if (!ctx->chn_cfgs[chn_id].is_enabled)
			continue;

		chn.chn_id = chn_id;

		vb_dqbuf(chn, &g_vpss_vb_jobs[working_grp].outs[chn_id], &blk);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_VPSS(DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.mod_id);
			continue;
		}
		//if (vi_ctx.bypass_frm[working_grp] >= vpssPrcCtx[working_grp].chn_cfgs[chn_id].send_ok) {
		//	TRACE_VPSS(DBG_DEBUG, "grp(%d) chn(%d) drop frame for vi-bypass(%d).\n",
		//		       working_grp, chn_id, vi_ctx.bypass_frm[working_grp]);
		//	vb_release_block(blk);
		//	_update_vpss_chn_proc(working_grp, chn_id);
		//	continue;
		//}

		// update pts & frm_num info to vb
		pts = timespec64_to_ns(&job->online_param.ts);
		do_div(pts, 1000);
		vb = (struct vb_s *)blk;
		vb->buf.pts = pts;
		vb->buf.dev_num = working_grp;
		vb->buf.frame_flag = job->checksum[chn_id];
		vb->buf.frm_num = ctx->grp_work_status.recv_cnt;
		_vpss_online_set_mlv_info(vb);

		if (_vpss_check_gdc_job(chn, blk, ctx) != true)
			vb_done_handler(chn, CHN_TYPE_OUT, &g_vpss_vb_jobs[working_grp].outs[chn_id], blk);

		TRACE_VPSS(DBG_INFO, "grp(%d) chn(%d) end\n", working_grp, chn_id);
		_update_vpss_chn_proc(working_grp, chn_id);
	} while (++chn_id < VPSS_MAX_CHN_NUM);

	// Update vpss grp proc info
	_update_vpss_grp_proc(working_grp, duration, job->hw_duration);

	vpss_online_full_job(working_grp, job);
	vpss_hal_push_online_job(job);
}


static void vpss_handle_offline_frame_done(struct vpss_job *job)
{
	struct vpss_ctx *ctx = (struct vpss_ctx *)job->data;
	vpss_grp working_grp = job->grp_id;
	unsigned char working_mask = job->working_mask;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = working_grp, .chn_id = 0};
	vb_blk blk;
	struct vb_s *vb;
	vpss_chn chn_id;
	unsigned int duration;
	struct timespec64 time;

	TRACE_VPSS(DBG_INFO, "grp(%d) eof\n", working_grp);

	//Todo: spin_lock, vpss destroy?
	vb_dqbuf(chn, &g_vpss_vb_jobs[working_grp].ins, &blk);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VPSS(DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.mod_id);
	} else {
		vb_done_handler(chn, CHN_TYPE_IN, &g_vpss_vb_jobs[working_grp].ins, blk);
	}

	chn_id = 0;
	do {
		if (!(working_mask & BIT(chn_id)))
			continue;

		if (!ctx->chn_cfgs[chn_id].is_enabled)
			continue;

		chn.chn_id = chn_id;

		vb_dqbuf(chn, &g_vpss_vb_jobs[working_grp].outs[chn_id], &blk);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_VPSS(DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.mod_id);
			continue;
		}
		vb = (struct vb_s *)blk;
		vb->buf.frame_flag = job->checksum[chn_id];
		if (_vpss_check_gdc_job(chn, blk, ctx) != true)
			vb_done_handler(chn, CHN_TYPE_OUT, &g_vpss_vb_jobs[working_grp].outs[chn_id], blk);

		TRACE_VPSS(DBG_INFO, "grp(%d) chn(%d) end\n", working_grp, chn_id);
		_update_vpss_chn_proc(working_grp, chn_id);
	} while (++chn_id < VPSS_MAX_CHN_NUM);

	ktime_get_ts64(&time);
	duration = get_diff_in_us(ctx->time, time);

	// Update vpss grp proc info
	_update_vpss_grp_proc(working_grp, duration, job->hw_duration);
	FIFO_PUSH(&ctx->jobq, job);
	atomic_set(&ctx->hdl_state, HANDLER_STATE_STOP);
}

static void vpss_handle_frame_done(struct work_struct *work)
{
	struct vpss_job *job = container_of(work, struct vpss_job, job_work);
	vpss_grp working_grp = job->grp_id;

	if (!g_vpss_ctx[working_grp] || !g_vpss_ctx[working_grp]->is_started) {
		TRACE_VPSS(DBG_NOTICE, "Grp(%d) isn't start yet.\n", working_grp);
		return;
	}
	mutex_lock(&g_vpss_ctx[working_grp]->lock);

	if (job->is_online)
		vpss_handle_online_frame_done(job);
	else
		vpss_handle_offline_frame_done(job);

	mutex_unlock(&g_vpss_ctx[working_grp]->lock);
	vpss_notify_wkup_evt();
}

/**
 * @return: 0 if ready
 */
static signed int vpss_try_schedule(unsigned char working_grp)
{
	unsigned char working_mask = 0;
	struct vpss_ctx *vpss_ctx_local = NULL;
	struct vpss_job *job = NULL;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = working_grp, .chn_id = 0};

	if (!g_vpss_ctx[working_grp]) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) isn't created yet.\n", working_grp);
		return -1;
	}

	vpss_ctx_local = g_vpss_ctx[working_grp];
	mutex_lock(&vpss_ctx_local->lock);
	if (!vpss_ctx_local->is_started)
		goto vpss_next_job;

	if (FIFO_EMPTY(&vpss_ctx_local->jobq)) {
		TRACE_VPSS(DBG_ERR, "vpss(%d) jobq empty.\n", working_grp);
		goto vpss_next_job;
	}

	// sc's mask
	working_mask = get_work_mask(vpss_ctx_local);
	if (working_mask)
		working_mask = _vpss_chl_frame_rate_ctrl(vpss_ctx_local, working_mask);
	if (working_mask == 0) {
		TRACE_VPSS(DBG_NOTICE, "grp(%d) working_mask zero.\n", working_grp);
		_release_vpss_waitq(chn, CHN_TYPE_IN);
		goto vpss_next_job;
	}

	// commit hw settings of this vpss-grp.
	if (commit_hw_settings(vpss_ctx_local) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) apply hw settings NG.\n", working_grp);
		_release_vpss_waitq(chn, CHN_TYPE_IN);
		vpss_ctx_local->grp_work_status.start_fail_cnt++;
		goto vpss_next_job;
	}
	FIFO_POP(&vpss_ctx_local->jobq, &job);
	memcpy(&job->cfg, &vpss_ctx_local->hw_cfg, sizeof(vpss_ctx_local->hw_cfg));
	job->working_mask = working_mask;

	if (fill_buffers(vpss_ctx_local, job) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) fill buffer NG.\n", working_grp);
		vpss_ctx_local->grp_work_status.start_fail_cnt++;
		FIFO_PUSH(&vpss_ctx_local->jobq, job);
		goto vpss_next_job;
	}

	ktime_get_ts64(&vpss_ctx_local->time);
	atomic_set(&vpss_ctx_local->hdl_state, HANDLER_STATE_RUN);
	vpss_hal_push_job(job);
	mutex_unlock(&vpss_ctx_local->lock);

	TRACE_VPSS(DBG_INFO, "Offline Grp(%d) post job.\n", working_grp);
	vpss_hal_try_schedule();

	// wait for h/w done
	return 0;

vpss_next_job:
	mutex_unlock(&vpss_ctx_local->lock);

	return -1;
}

// static void vpss_timeout(struct vpss_ctx *ctx)
// {
// 	struct vpss_job *job = (struct vpss_job *)ctx->job_buffer;

// 	TRACE_VPSS(DBG_INFO, "vpss grp(%d) timeout...\n", ctx->vpss_grp);

// 	mutex_lock(&ctx->lock);
// 	vpss_hal_remove_job(job);
// 	FIFO_PUSH(&ctx->jobq, job);
// 	release_buffers(ctx);
// 	ctx->hdl_state = HANDLER_STATE_STOP;
// 	mutex_unlock(&ctx->lock);
// }

static unsigned char vpss_handler_is_idle(void)
{
	int i;

	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		if (g_vpss_ctx[i] && g_vpss_ctx[i]->is_created && g_vpss_ctx[i]->is_started)
			return false;

	return true;
}

static int vpss_event_handler(void *arg)
{
	struct vpss_handler_ctx *ctx = (struct vpss_handler_ctx *)arg;
	unsigned long idle_timeout = msecs_to_jiffies(IDLE_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	int i, ret;
	int grp = 0, prev_grp = -1;
	struct vb_jobs_t *jobs;
	struct timespec64 time;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(ctx->wait,
			ctx->events || kthread_should_stop(), timeout);

		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret && vpss_handler_is_idle()) {
			timeout = idle_timeout;
			continue;
		}

		//TRACE_VPSS(DBG_DEBUG, "vpss thread, events:%d\n", ctx->events);
		spin_lock(&g_vpss_hdl_ctx.hdl_lock);
		ctx->events &= ~CTX_EVENT_WKUP;
		spin_unlock(&g_vpss_hdl_ctx.hdl_lock);

		if (atomic_read(&ctx->active_cnt) == 0)
			continue;

		ktime_get_ts64(&time);

		grp = prev_grp;
		for (i = 0; i < VPSS_MAX_GRP_NUM; i++) {
			if (++grp >= VPSS_MAX_GRP_NUM)
				grp = 0;
			if (!g_vpss_ctx[grp] || !g_vpss_ctx[grp]->is_started)
				continue;
			if (atomic_read(&g_vpss_ctx[grp]->hdl_state) == HANDLER_STATE_RUN) {
				// if (get_diff_in_us(vpss_ctx[grp]->time, time) > (eof_timeout * 1000))
				// 	vpss_timeout(vpss_ctx[grp]);
				continue;
			}
			jobs = &g_vpss_vb_jobs[grp].ins;
			if (!jobs) {
				TRACE_VPSS(DBG_INFO, "get jobs failed\n");
				continue;
			}

			if (!down_trylock(&jobs->sem)) {
				vpss_try_schedule(grp);
				timeout = eof_timeout;
				prev_grp = grp;
			}
		}
	}

	return 0;
}


void _vpss_grp_raram_init(vpss_grp grp_id)
{
	unsigned char i, j, k;
	proc_amp_ctrl_s ctrl;
	struct sclr_csc_matrix *mtrx;
	struct vpss_ctx *ctx = g_vpss_ctx[grp_id];

	memset(&ctx->grp_crop_info, 0, sizeof(ctx->grp_crop_info));
	memset(&ctx->frame_crop, 0, sizeof(ctx->frame_crop));
	memset(&ctx->grp_work_status, 0, sizeof(ctx->grp_work_status));

	for (i = 0; i < VPSS_MAX_CHN_NUM; ++i) {
		memset(&ctx->chn_cfgs[i], 0, sizeof(ctx->chn_cfgs[i]));
		ctx->chn_cfgs[i].coef = VPSS_SCALE_COEF_BICUBIC;
		ctx->chn_cfgs[i].align = DEFAULT_ALIGN;
		ctx->chn_cfgs[i].y_ratio = YRATIO_SCALE;
		ctx->chn_cfgs[i].vb_pool = VB_INVALID_POOLID;
		mutex_init(&g_vpss_mesh[grp_id][i].lock);

		for (j = 0; j < RGN_MAX_LAYER_VPSS; ++j)
			for (k = 0; k < RGN_MAX_NUM_VPSS; ++k)
				ctx->chn_cfgs[i].rgn_handle[j][k] = RGN_INVALID_HANDLE;
		for (j = 0; j < RGN_COVEREX_MAX_NUM; ++j)
			ctx->chn_cfgs[i].cover_ex_handle[j] = RGN_INVALID_HANDLE;
		for (j = 0; j < RGN_MOSAIC_MAX_NUM; ++j)
			ctx->chn_cfgs[i].mosaic_handle[j] = RGN_INVALID_HANDLE;
	}

	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i) {
		vpss_get_proc_amp_ctrl(i, &ctrl);
		g_vpss_ext_ctx[grp_id].proc_amp[i] = ctrl.default_value;
	}

	// use designer provided table
	mtrx = sclr_get_csc_mtrx(SCL_CSC_601_LIMIT_YUV2RGB);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			g_vpss_ext_ctx[grp_id].csc_cfg.coef[i][j] = mtrx->coef[i][j];

		g_vpss_ext_ctx[grp_id].csc_cfg.add[i] = mtrx->add[i];
		g_vpss_ext_ctx[grp_id].csc_cfg.sub[i] = mtrx->sub[i];
	}
	ctx->is_copy_upsample = false;
}

static signed int _vpss_update_rotation_mesh(vpss_grp grp_id, vpss_chn chn_id, rotation_e rotation)
{
	struct gdc_mesh *pmesh = &g_vpss_mesh[grp_id][chn_id];
	struct vpss_ctx *ctx = g_vpss_ctx[grp_id];

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) rotation(%d).\n",
			grp_id, chn_id, rotation);

	pmesh->paddr = DEFAULT_MESH_PADDR;

	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].rotation = rotation;
	mutex_unlock(&ctx->lock);
	return 0;
}

signed int _vpss_update_ldc_mesh(vpss_grp grp_id, vpss_chn chn_id,
	const vpss_ldc_attr_s *ldc_attr, unsigned long long paddr)
{
	unsigned long long paddr_old;
	struct gdc_mesh *pmesh = &g_vpss_mesh[grp_id][chn_id];
	struct vpss_ctx *ctx = g_vpss_ctx[grp_id];

	mutex_lock(&pmesh->lock);
	if (pmesh->paddr) {
		paddr_old = pmesh->paddr;
	} else {
		paddr_old = 0;
	}
	pmesh->paddr = paddr;
	pmesh->vaddr = NULL;
	mutex_unlock(&pmesh->lock);

	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].ldc_attr = *ldc_attr;
	mutex_unlock(&ctx->lock);
	//mutex_unlock(&pmesh->lock);

	//if (paddr_old)
	//	SYS_IonFree(paddr_old, vaddr_old);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) mesh base(0x%llx)\n"
		      , grp_id, chn_id, (unsigned long long)paddr);
	TRACE_VPSS(DBG_DEBUG, "enable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d, rotation=%d\n",
			ldc_attr->enable, ldc_attr->attr.aspect,
			ldc_attr->attr.xy_ratio, ldc_attr->attr.center_x_offset,
			ldc_attr->attr.center_y_offset, ldc_attr->attr.distortion_ratio,
			ldc_attr->attr.rotation);
	return 0;
}

signed int _vpss_update_fisheye_mesh(vpss_grp grp_id, vpss_chn chn_id,
	const fisheye_attr_s *fish_eye_attr, rotation_e rotation, unsigned long long paddr)
{
	unsigned long long paddr_old;
	struct gdc_mesh *pmesh = &g_vpss_mesh[grp_id][chn_id];
	struct vpss_ctx *ctx = g_vpss_ctx[grp_id];

	mutex_lock(&pmesh->lock);
	if (pmesh->paddr) {
		paddr_old = pmesh->paddr;
	} else {
		paddr_old = 0;
	}
	pmesh->paddr = paddr;
	pmesh->vaddr = NULL;
	mutex_unlock(&pmesh->lock);

	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].fisheye_attr = *fish_eye_attr;
	ctx->chn_cfgs[chn_id].rotation = rotation;
	mutex_unlock(&ctx->lock);
	//mutex_unlock(&pmesh->lock);

	//if (paddr_old)
	//	SYS_IonFree(paddr_old, vaddr_old);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) mesh base(0x%llx)\n"
		      , grp_id, chn_id, (unsigned long long)paddr);
	return 0;
}

static int vpss_grp_qbuf(mmf_chn_s chn, vb_blk blk)
{
	signed int ret;
	vpss_grp grp_id = chn.dev_id;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	if (!ctx->is_started) {
		TRACE_VPSS(DBG_NOTICE, "Grp(%d) not started yet.\n", grp_id);
		return -1;
	}
	if (ctx->online_from_isp) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) online, No need to receive buffer.\n", grp_id);
		return -1;
	}
	ctx->grp_work_status.recv_cnt++;

	if (!FRC_INVALID(ctx->grp_attr.frame_rate) &&
		!vpss_frame_ctrl(ctx->grp_work_status.recv_cnt - 1,
		&ctx->grp_attr.frame_rate)) {
		TRACE_VPSS(DBG_DEBUG, "grp[%d] frame index(%d) drop\n", grp_id,
			ctx->grp_work_status.recv_cnt);
		return 0;
	}
	ctx->grp_work_status.frc_recv_cnt++;

	TRACE_VPSS(DBG_INFO, "Grp(%d) qbuf, blk(0x%llx)\n", grp_id, blk);

	ret = vb_qbuf(chn, CHN_TYPE_IN, &g_vpss_vb_jobs[grp_id].ins, blk);
	if (ret != 0) {
		ctx->grp_work_status.lost_cnt++;
		return ret;
	}

	vpss_notify_wkup_evt();

	return 0;
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
signed int vpss_set_mod_param(const vpss_mod_param_s *mod_param_info)
{
	signed int ret, i;

	ret = mod_check_null_ptr(ID_VPSS, mod_param_info);
	if (ret != 0)
		return ret;
	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		if (g_vpss_ctx[i]) {
			TRACE_VPSS(DBG_ERR, "Setting module param must be the first step of VPSS.\n");
			return ERR_VPSS_NOT_PERM;
		}

	g_vpss_mod_param = *mod_param_info;

	return 0;
}

signed int vpss_get_mod_param(vpss_mod_param_s *mod_param_info)
{
	signed int ret;

	ret = mod_check_null_ptr(ID_VPSS, mod_param_info);
	if (ret != 0)
		return ret;

	*mod_param_info = g_vpss_mod_param;

	return 0;
}

signed int vpss_create_grp(vpss_grp grp_id, const vpss_grp_attr_s *grp_attr)
{
	unsigned int job_num = 1;
	unsigned char online_from_isp = false;
	struct vpss_job *job;
	signed int ret, i;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, grp_attr);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_fmt(grp_id, grp_attr->pixel_format);
	if (ret != 0)
		return ret;

	ret = check_yuv_param(grp_attr->pixel_format, grp_attr->w, grp_attr->h);
	if (ret != 0)
		return ret;

	if ((grp_attr->w < VPSS_MIN_IMAGE_WIDTH) || (grp_attr->h < VPSS_MIN_IMAGE_HEIGHT)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) max_w(%d) or max_h(%d) too small\n"
			, grp_id, grp_attr->w, grp_attr->h);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (grp_attr->frame_rate.src_frame_rate < grp_attr->frame_rate.dst_frame_rate) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, grp_id, grp_attr->frame_rate.src_frame_rate
				, grp_attr->frame_rate.dst_frame_rate);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (g_vpss_ctx[grp_id]) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) is occupied\n", grp_id);
		return ERR_VPSS_EXIST;
	}

	if ((grp_id < VPSS_ONLINE_NUM) &&
		((g_vi_vpss_mode.mode[grp_id] == VI_OFFLINE_VPSS_ONLINE) ||
		(g_vi_vpss_mode.mode[grp_id] == VI_ONLINE_VPSS_ONLINE))) {
		online_from_isp = true;
		job_num = VPSS_ONLINE_JOB_NUM;
	}

	g_vpss_ctx[grp_id] = kzalloc(sizeof(struct vpss_ctx), GFP_ATOMIC);
	if (!g_vpss_ctx[grp_id]) {
		TRACE_VPSS(DBG_ERR, "vpss_ctx kzalloc fail.\n");
		return ERR_VPSS_NOMEM;
	}

	g_vpss_ctx[grp_id]->job_buffer = kzalloc(sizeof(struct vpss_job) * job_num, GFP_ATOMIC);
	if (!g_vpss_ctx[grp_id]->job_buffer) {
		TRACE_VPSS(DBG_ERR, "job kzalloc fail.\n");
		kfree(g_vpss_ctx[grp_id]);
		return ERR_VPSS_NOMEM;
	}
	ctx = g_vpss_ctx[grp_id];

	FIFO_INIT(&ctx->jobq, job_num);

	job = (struct vpss_job *)ctx->job_buffer;
	for (i = 0; i < job_num; i++) {
		job[i].grp_id = grp_id;
		job[i].is_online = online_from_isp;
		job[i].data = (void *)ctx;
		job[i].job_cb = vpss_wkup_frame_done_handle;
		INIT_WORK(&job[i].job_work, vpss_handle_frame_done);
		spin_lock_init(&job[i].lock);
		atomic_set(&job[i].job_state, JOB_INVALID);
		FIFO_PUSH(&ctx->jobq, job + i);
	}

	if (online_from_isp)
		base_mod_jobs_init(&g_vpss_vb_jobs[grp_id].ins, 0, 0, 0);
	else
		base_mod_jobs_init(&g_vpss_vb_jobs[grp_id].ins, 4, 1, 0);

	ctx->vpss_grp = grp_id;
	ctx->is_created = true;
	ctx->online_from_isp = online_from_isp;
	atomic_set(&ctx->hdl_state, HANDLER_STATE_STOP);
	mutex_init(&ctx->lock);
	memcpy(&ctx->grp_attr, grp_attr, sizeof(*grp_attr));
	_vpss_grp_raram_init(grp_id);
	mutex_lock(&g_vpss_lock);
	g_vpss_grp_used[grp_id] = true;
	if(g_is_bm_scene) g_is_bm_scene = false;
	mutex_unlock(&g_vpss_lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) max_w(%d) max_h(%d) PixelFmt(%d) online_from_isp(%d)\n",
		grp_id, grp_attr->w, grp_attr->h,
		grp_attr->pixel_format, ctx->online_from_isp);
	ctx->is_cfg_changed = true;
	return 0;
}

signed int vpss_destroy_grp(vpss_grp grp_id)
{
	vpss_chn chn_id;
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	if (!ctx)
		return 0;

	// FIXME: free ion until dwa hardware stops
	if (ctx->is_created) {
		mutex_lock(&ctx->lock);
		ctx->is_created = false;
		base_mod_jobs_exit(&g_vpss_vb_jobs[grp_id].ins);
		for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
			ctx->chn_cfgs[chn_id].rotation = ROTATION_0;
			ctx->chn_cfgs[chn_id].ldc_attr.enable = false;
			ctx->chn_cfgs[chn_id].fisheye_attr.enable = false;

			if (g_vpss_mesh[grp_id][chn_id].paddr) {
#if 0
				if (mesh[grp_id][chn_id].paddr && mesh[grp_id][chn_id].paddr != DEFAULT_MESH_PADDR) {
					base_ion_free(mesh[grp_id][chn_id].paddr);
				}
#endif
				g_vpss_mesh[grp_id][chn_id].paddr = 0;
				g_vpss_mesh[grp_id][chn_id].vaddr = 0;
			}
		}
		FIFO_EXIT(&ctx->jobq);
		mutex_unlock(&ctx->lock);
		mutex_destroy(&ctx->lock);
	}

	kfree(ctx->job_buffer);
	kfree(ctx);
	g_vpss_ctx[grp_id] = NULL;

	mutex_lock(&g_vpss_lock);
	g_vpss_grp_used[grp_id] = false;
	mutex_unlock(&g_vpss_lock);
	TRACE_VPSS(DBG_INFO, "Grp(%d)\n", grp_id);
	return 0;
}

vpss_grp vpss_get_available_grp(void)
{
	vpss_grp grp = 0;
	vpss_grp ret = VPSS_INVALID_GRP;
	unsigned char i;

	for (i = 0; i < VPSS_ONLINE_NUM; i++) {
		if ((g_vi_vpss_mode.mode[i] == VI_ONLINE_VPSS_ONLINE) ||
			(g_vi_vpss_mode.mode[i] == VI_OFFLINE_VPSS_ONLINE))
			grp = VPSS_ONLINE_NUM;
	}

	mutex_lock(&g_vpss_lock);
	for (; grp < VPSS_MAX_GRP_NUM; ++grp)
		if (!g_vpss_grp_used[grp]) {
			g_vpss_grp_used[grp] = true;
			ret = grp;
			break;
		}
	mutex_unlock(&g_vpss_lock);

	if ((VPSS_INVALID_GRP != ret) && g_vpss_ctx[ret]){
		vpss_disable_chn(ret, 0);
		vpss_stop_grp(ret);
		vpss_destroy_grp(ret);
		mutex_lock(&g_vpss_lock);
		g_vpss_grp_used[ret] = true;
		mutex_unlock(&g_vpss_lock);
	}

	return ret;
}

signed int vpss_start_grp(vpss_grp grp_id)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	if (ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) already started.\n", grp_id);
		return 0;
	}

	mutex_lock(&ctx->lock);
	ctx->is_started = true;
	atomic_set(&ctx->hdl_state, HANDLER_STATE_STOP);
	mutex_unlock(&ctx->lock);

	if (ctx->online_from_isp) {
		ret = vpss_online_prepare(grp_id);
		if (ret != 0) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) vpss_online_prepare failed.\n", grp_id);
			ctx->is_started = false;
			return ret;
		}
	} else {
		atomic_add(1, &g_vpss_hdl_ctx.active_cnt);
	}
	TRACE_VPSS(DBG_INFO, "Grp(%d)\n", grp_id);

	return 0;
}

signed int vpss_stop_grp(vpss_grp grp_id)
{
	signed int ret, i; //Todo: online ???
	struct vpss_job *job;
	unsigned int job_num;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	if (!ctx)
		return 0;
	if (!ctx->is_started)
		return 0;

	if (!ctx->online_from_isp)
		atomic_sub(1, &g_vpss_hdl_ctx.active_cnt);

	mutex_lock(&ctx->lock);
	ctx->is_started = false;

	job_num = ctx->online_from_isp ? VPSS_ONLINE_JOB_NUM : 1;
	job = (struct vpss_job *)ctx->job_buffer;

	for (i = 0; i < job_num; i++) {
		if ((atomic_read(&job[i].job_state) == JOB_WAIT) ||
			(atomic_read(&job[i].job_state) == JOB_WORKING)) {
			vpss_hal_remove_job(job + i);
			FIFO_PUSH(&ctx->jobq, job + i);
			release_buffers(ctx);
		}
		atomic_set(&job[i].job_state, JOB_INVALID);
		cancel_work_sync(&job[i].job_work);
	}

	atomic_set(&ctx->hdl_state, HANDLER_STATE_STOP);
	mutex_unlock(&ctx->lock);

	if (ctx->online_from_isp && !vpss_online_is_idle())
		vpss_online_unprepare();

	TRACE_VPSS(DBG_INFO, "Grp(%d)\n", grp_id);

	return 0;
}

signed int vpss_reset_grp(vpss_grp grp_id)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	vpss_stop_grp(grp_id);
	mutex_lock(&ctx->lock);
	memset(ctx->chn_cfgs, 0, sizeof(struct vpss_chn_cfg) * VPSS_MAX_CHN_NUM);
	memset(&g_vpss_ext_ctx[grp_id], 0, sizeof(g_vpss_ext_ctx[grp_id]));
	_vpss_grp_raram_init(grp_id);
	ctx->is_cfg_changed = true;
	mutex_unlock(&ctx->lock);
	TRACE_VPSS(DBG_INFO, "Grp(%d)\n", grp_id);

	return 0;
}

signed int vpss_get_grp_attr(vpss_grp grp_id, vpss_grp_attr_s *grp_attr)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, grp_attr);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	mutex_lock(&ctx->lock);
	*grp_attr = ctx->grp_attr;
	mutex_unlock(&ctx->lock);

	return 0;
}

signed int vpss_set_grp_attr(vpss_grp grp_id, const vpss_grp_attr_s *grp_attr)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, grp_attr);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	ret = check_vpss_grp_fmt(grp_id, grp_attr->pixel_format);
	if (ret != 0)
		return ret;

	ret = check_yuv_param(grp_attr->pixel_format, grp_attr->w, grp_attr->h);
	if (ret != 0)
		return ret;

	if ((grp_attr->w < VPSS_MIN_IMAGE_WIDTH) || (grp_attr->h < VPSS_MIN_IMAGE_HEIGHT)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) max_w(%d) or max_h(%d) too small\n"
			, grp_id, grp_attr->w, grp_attr->h);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (ctx->online_from_isp && ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) operation not allowed if vi-2-vpss online\n", grp_id);
		return ERR_VPSS_NOT_PERM;
	}
	if (grp_attr->frame_rate.src_frame_rate < grp_attr->frame_rate.dst_frame_rate) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, grp_id, grp_attr->frame_rate.src_frame_rate
				, grp_attr->frame_rate.dst_frame_rate);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&ctx->lock);
	ctx->grp_attr = *grp_attr;
	ctx->is_cfg_changed = true;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) max_w(%d) max_h(%d) PixelFmt(%d) online_from_isp(%d)\n",
		grp_id, grp_attr->w, grp_attr->h,
		grp_attr->pixel_format, ctx->online_from_isp);
	return 0;
}

signed int vpss_set_grp_csc(struct vpss_grp_csc_cfg *cfg)
{
	signed int ret;
	vpss_grp grp_id = cfg->vpss_grp;
	unsigned char i, j;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	mutex_lock(&ctx->lock);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			g_vpss_ext_ctx[grp_id].csc_cfg.coef[i][j] = cfg->coef[i][j];

		g_vpss_ext_ctx[grp_id].csc_cfg.add[i] = cfg->add[i];
		g_vpss_ext_ctx[grp_id].csc_cfg.sub[i] = cfg->sub[i];
	}
	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i)
		g_vpss_ext_ctx[grp_id].proc_amp[i] = cfg->proc_amp[i];
	ctx->is_copy_upsample = cfg->is_copy_upsample;
	ctx->is_cfg_changed = true;
	mutex_unlock(&ctx->lock);
	TRACE_VPSS(DBG_DEBUG, "Grp(%d)\n", grp_id);

	return 0;
}

signed int vpss_set_chn_csc(struct vpss_chn_csc_cfg *cfg)
{
	signed int ret;
	vpss_grp grp_id = cfg->vpss_grp;
	vpss_chn chn_id = cfg->vpss_chn;
	unsigned char i, j;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			g_vpss_ext_ctx[grp_id].chn_csc_cfg[chn_id].coef[i][j] = cfg->coef[i][j];

		g_vpss_ext_ctx[grp_id].chn_csc_cfg[chn_id].add[i] = cfg->add[i];
		g_vpss_ext_ctx[grp_id].chn_csc_cfg[chn_id].sub[i] = cfg->sub[i];
	}
	ctx->chn_cfgs[chn_id].is_cfg_changed = true;
	mutex_unlock(&ctx->lock);
	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);

	return 0;
}

signed int vpss_get_proc_amp_ctrl(proc_amp_e type, proc_amp_ctrl_s *ctrl)
{
	mod_check_null_ptr(ID_VPSS, ctrl);

	if (type >= PROC_AMP_MAX) {
		TRACE_VPSS(DBG_ERR, "ProcAmp type(%d) invalid.\n", type);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	*ctrl = g_procamp_ctrls[type];
	return 0;
}

signed int vpss_get_proc_amp(vpss_grp grp_id, signed int *proc_amp)
{
	signed int ret;
	unsigned char i;

	mod_check_null_ptr(ID_VPSS, proc_amp);

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i)
		proc_amp[i] = g_vpss_ext_ctx[grp_id].proc_amp[i];

	return 0;
}

signed int vpss_get_all_proc_amp(struct vpss_all_proc_amp_cfg *cfg)
{
	unsigned char i, j;

	mod_check_null_ptr(ID_VPSS, cfg);

	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		for (j = PROC_AMP_BRIGHTNESS; j < PROC_AMP_MAX; ++j)
			cfg->proc_amp[i][j] = g_vpss_ext_ctx[i].proc_amp[j];

	return 0;
}

signed int vpss_set_chn_attr(vpss_grp grp_id, vpss_chn chn_id, const vpss_chn_attr_s *chn_attr)
{
	vb_cal_config_s vb_cal_config;
	struct sclr_csc_matrix *mtrx;
	signed int ret;
	unsigned char i, j;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, chn_attr);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_fmt(grp_id, chn_id, chn_attr->pixel_format);
	if (ret != 0)
		return ret;

	ret = check_yuv_param(chn_attr->pixel_format, chn_attr->width, chn_attr->height);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	if ((chn_attr->width < VPSS_MIN_IMAGE_WIDTH) || (chn_attr->height < VPSS_MIN_IMAGE_HEIGHT)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) width(%d) or height(%d) too small\n"
			, grp_id, chn_id, chn_attr->width, chn_attr->height);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (ctx->online_from_isp &&ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "You must stop the group before changing the channel attributes,Grp(%d)\n",
			grp_id);
		return ERR_VPSS_NOT_PERM;
	}

	if (chn_attr->aspect_ratio.mode == ASPECT_RATIO_MANUAL) {
		const rect_s *rect = &chn_attr->aspect_ratio.video_rect;

		if (!chn_attr->aspect_ratio.enable_bgcolor) {
			ret = check_yuv_param(chn_attr->pixel_format, rect->width, rect->height);
			if (ret != 0)
				return ret;
			if ((IS_FMT_YUV420(chn_attr->pixel_format) || IS_FMT_YUV422(chn_attr->pixel_format))
				&& (rect->x & 0x01)) {
				TRACE_VPSS(DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
				TRACE_VPSS(DBG_ERR, "YUV_420/YUV_422 rect x(%d) should be even.\n",
					rect->x);
				return ERR_VPSS_ILLEGAL_PARAM;
			}
		}

		if ((rect->x < 0) || (rect->y < 0)) {
			TRACE_VPSS(DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) rect pos(%d %d) can't be negative.\n"
				, grp_id, chn_id, rect->x, rect->y);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((rect->width < VPSS_MIN_IMAGE_WIDTH) || (rect->height < VPSS_MIN_IMAGE_HEIGHT)) {
			TRACE_VPSS(DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) rect size(%d %d) can't smaller than %dx%d\n"
				, grp_id, chn_id, rect->width, rect->height
				, VPSS_MIN_IMAGE_WIDTH, VPSS_MIN_IMAGE_HEIGHT);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((rect->x + rect->width > chn_attr->width)
		|| (rect->y + rect->height > chn_attr->height)) {
			TRACE_VPSS(DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) rect(%d %d %d %d) output-size(%d %d).\n"
					, grp_id, chn_id
					, rect->x, rect->y, rect->width, rect->height
					, chn_attr->width, chn_attr->height);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	if (chn_attr->frame_rate.src_frame_rate < chn_attr->frame_rate.dst_frame_rate) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) Chn(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, grp_id, chn_id, chn_attr->frame_rate.src_frame_rate
				, chn_attr->frame_rate.dst_frame_rate);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	common_getpicbufferconfig(chn_attr->width, chn_attr->height,
		chn_attr->pixel_format, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &vb_cal_config);

	mutex_lock(&ctx->lock);
	memcpy(&ctx->chn_cfgs[chn_id].chn_attr, chn_attr,
		sizeof(ctx->chn_cfgs[chn_id].chn_attr));
	ctx->chn_cfgs[chn_id].blk_size = vb_cal_config.vb_size;
	ctx->chn_cfgs[chn_id].align = DEFAULT_ALIGN;
	if (ctx->chn_cfgs[chn_id].vb_pool == VB_INVALID_POOLID) {
		ctx->chn_cfgs[chn_id].chn_work_status.pool_id =
		find_vb_pool(ctx->chn_cfgs[chn_id].blk_size);
	}
	ctx->chn_cfgs[chn_id].is_cfg_changed = true;
	mutex_unlock(&ctx->lock);

	// use designer provided table
	if(chn_attr->pixel_format == PIXEL_FORMAT_YUV_400)
		mtrx = sclr_get_csc_mtrx(SCL_CSC_NONE);
	else
		mtrx = sclr_get_csc_mtrx(SCL_CSC_601_LIMIT_RGB2YUV);

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			g_vpss_ext_ctx[grp_id].chn_csc_cfg[chn_id].coef[i][j] = mtrx->coef[i][j];

		g_vpss_ext_ctx[grp_id].chn_csc_cfg[chn_id].add[i] = mtrx->add[i];
		g_vpss_ext_ctx[grp_id].chn_csc_cfg[chn_id].sub[i] = mtrx->sub[i];
	}

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d) width(%d), height(%d)\n"
		, grp_id, chn_id, chn_attr->width, chn_attr->height);

	return 0;
}

signed int vpss_get_chn_attr(vpss_grp grp_id, vpss_chn chn_id, vpss_chn_attr_s *chn_attr)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, chn_attr);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	memcpy(chn_attr, &ctx->chn_cfgs[chn_id].chn_attr, sizeof(*chn_attr));
	mutex_unlock(&ctx->lock);

	return 0;
}

signed int vpss_enable_chn(vpss_grp grp_id, vpss_chn chn_id)
{
	struct vpss_chn_cfg *chn_cfg;
	unsigned int ret;
	int i, j;
	unsigned char chn_num;
	unsigned char max_chn_num = 0;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	chn_cfg = &ctx->chn_cfgs[chn_id];

	if (chn_cfg->is_enabled) {
		TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) already enabled\n", grp_id, chn_id);
		return 0;
	}

	if (!ctx->online_from_isp && chn_id >= 2) {
		for (i = 0; i < VPSS_ONLINE_NUM; i++) {
			if (g_vpss_ctx[i] && g_vpss_ctx[i]->is_created && g_vpss_ctx[i]->online_from_isp) {
				chn_num = 0;
				for (j = 0; j < VPSS_MAX_CHN_NUM; j++) {
					if (g_vpss_ctx[i]->chn_cfgs[j].is_enabled)
						chn_num++;
				}
				max_chn_num = max_chn_num > chn_num ? max_chn_num : chn_num;
			}
		}

		if (chn_id + max_chn_num >= VPSS_MAX_CHN_NUM) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) invalid channel ID. Job maximum chn ID(%d)\n",
				grp_id, chn_id, VPSS_MAX_CHN_NUM - max_chn_num - 1);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	mutex_lock(&ctx->lock);
	if (ctx->online_from_isp)
		base_mod_jobs_init(&g_vpss_vb_jobs[grp_id].outs[chn_id], 1, VPSS_ONLINE_JOB_NUM, chn_cfg->chn_attr.depth);
	else
		base_mod_jobs_init(&g_vpss_vb_jobs[grp_id].outs[chn_id], 1, 1, chn_cfg->chn_attr.depth);
	chn_cfg->is_enabled = true;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d)\n", grp_id, chn_id);

	return 0;
}

signed int vpss_disable_chn(vpss_grp grp_id, vpss_chn chn_id)
{
	signed int ret;
	struct vpss_chn_work_status_s *chn_status;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	if (!ctx->chn_cfgs[chn_id].is_enabled) {
		TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) not enabled yet\n", grp_id, chn_id);
		return 0;
	}

	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].is_enabled = false;
	chn_status = &ctx->chn_cfgs[chn_id].chn_work_status;
	chn_status->send_ok = 0;
	chn_status->prev_time = 0;
	chn_status->frame_num = 0;
	chn_status->real_frame_rate = 0;
	base_mod_jobs_exit(&g_vpss_vb_jobs[grp_id].outs[chn_id]);
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d)\n", grp_id, chn_id);

	return 0;
}

signed int vpss_set_chn_crop(vpss_grp grp_id, vpss_chn chn_id, const vpss_crop_info_s *crop_info)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, crop_info);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	if (crop_info->enable) {
		if ((crop_info->crop_rect.width < 4) || (crop_info->crop_rect.height < 1)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) crop size(%d %d) can't smaller than 4x1\n"
				, grp_id, chn_id, crop_info->crop_rect.width
				, crop_info->crop_rect.height);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if (crop_info->crop_rect.x + (signed int)crop_info->crop_rect.width < 4
			|| crop_info->crop_rect.y + (signed int)crop_info->crop_rect.height < 1) {
			TRACE_VPSS(DBG_ERR
				, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
				, grp_id, chn_id, crop_info->crop_rect.x, crop_info->crop_rect.y
				, crop_info->crop_rect.width, crop_info->crop_rect.height);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if (ctx->grp_crop_info.enable) {
			if ((signed int)ctx->grp_crop_info.crop_rect.height
				- crop_info->crop_rect.y < 1
				|| (signed int)ctx->grp_crop_info.crop_rect.width
				- crop_info->crop_rect.x < 4) {
				TRACE_VPSS(DBG_ERR
					, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
					, grp_id, chn_id, crop_info->crop_rect.x, crop_info->crop_rect.y
					, crop_info->crop_rect.width, crop_info->crop_rect.height);
				TRACE_VPSS(DBG_ERR, "grp crop size(%d %d)\n"
					, ctx->grp_crop_info.crop_rect.width
					, ctx->grp_crop_info.crop_rect.height);
				return ERR_VPSS_ILLEGAL_PARAM;
			}
		} else {
			if ((signed int)ctx->grp_attr.h - crop_info->crop_rect.y < 1
				|| (signed int)ctx->grp_attr.w - crop_info->crop_rect.x < 4) {
				TRACE_VPSS(DBG_ERR
					, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
					, grp_id, chn_id, crop_info->crop_rect.x, crop_info->crop_rect.y
					, crop_info->crop_rect.width, crop_info->crop_rect.height);
				TRACE_VPSS(DBG_ERR, "out of grp size(%d %d)\n"
					, ctx->grp_attr.w, ctx->grp_attr.h);
				return ERR_VPSS_ILLEGAL_PARAM;
			}
		}
	}

	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].crop_info = *crop_info;
	ctx->chn_cfgs[chn_id].is_cfg_changed = true;
	mutex_unlock(&ctx->lock);
	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d), enable=%d, rect(%d %d %d %d)\n",
		grp_id, chn_id, crop_info->enable,
		crop_info->crop_rect.x, crop_info->crop_rect.y,
		crop_info->crop_rect.width, crop_info->crop_rect.height);

	return 0;
}

signed int vpss_get_chn_crop(vpss_grp grp_id, vpss_chn chn_id, vpss_crop_info_s *crop_info)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, crop_info);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	*crop_info = ctx->chn_cfgs[chn_id].crop_info;
	mutex_unlock(&ctx->lock);

	return 0;
}

signed int vpss_show_chn(vpss_grp grp_id, vpss_chn chn_id)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].is_muted = false;
	ctx->chn_cfgs[chn_id].is_cfg_changed = true;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

signed int vpss_hide_chn(vpss_grp grp_id, vpss_chn chn_id)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].is_muted = true;
	ctx->chn_cfgs[chn_id].is_cfg_changed = true;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

signed int vpss_get_grp_crop(vpss_grp grp_id, vpss_crop_info_s *crop_info)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, crop_info);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	*crop_info = ctx->grp_crop_info;
	mutex_unlock(&ctx->lock);
	return 0;
}

signed int vpss_set_grp_crop(vpss_grp grp_id, const vpss_crop_info_s *crop_info)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, crop_info);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	if (ctx->online_from_isp) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not support crop if online\n", grp_id);
		return ERR_VPSS_NOT_PERM;
	}

	if (crop_info->enable) {
		if ((crop_info->crop_rect.x < 0) || (crop_info->crop_rect.y < 0)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) crop start-point(%d %d) illegal\n"
				, grp_id, crop_info->crop_rect.x, crop_info->crop_rect.y);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((crop_info->crop_rect.width < VPSS_MIN_IMAGE_WIDTH) ||
			(crop_info->crop_rect.height < VPSS_MIN_IMAGE_HEIGHT)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) crop size(%d %d) can't smaller than %dx%d\n"
				, grp_id, crop_info->crop_rect.width, crop_info->crop_rect.height
				, VPSS_MIN_IMAGE_WIDTH, VPSS_MIN_IMAGE_HEIGHT);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((crop_info->crop_rect.y + crop_info->crop_rect.height)
			> ctx->grp_attr.h
		 || (crop_info->crop_rect.x + crop_info->crop_rect.width)
			 > ctx->grp_attr.w) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) crop rect(%d %d %d %d) out of grp size(%d %d)\n"
				, grp_id, crop_info->crop_rect.x, crop_info->crop_rect.y
				, crop_info->crop_rect.width, crop_info->crop_rect.height
				, ctx->grp_attr.w, ctx->grp_attr.h);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	mutex_lock(&ctx->lock);
	ctx->grp_crop_info = *crop_info;
	if (crop_info->enable) {
		bool chk_width_even = IS_FMT_YUV420(ctx->grp_attr.pixel_format) ||
				      IS_FMT_YUV422(ctx->grp_attr.pixel_format);
		bool chk_height_even = IS_FMT_YUV420(ctx->grp_attr.pixel_format);

		if (chk_width_even && (crop_info->crop_rect.width & 0x01)) {
			ctx->grp_crop_info.crop_rect.width &= ~(0x0001);
			TRACE_VPSS(DBG_WARN, "Grp(%d) crop_rect.width(%d) to even(%d) due to YUV\n",
				       grp_id, crop_info->crop_rect.width,
				       ctx->grp_crop_info.crop_rect.width);
		}
		if (chk_height_even && (crop_info->crop_rect.height & 0x01)) {
			ctx->grp_crop_info.crop_rect.height &= ~(0x0001);
			TRACE_VPSS(DBG_WARN, "Grp(%d) crop_rect.height(%d) to even(%d) due to YUV\n",
				       grp_id, crop_info->crop_rect.height,
				       ctx->grp_crop_info.crop_rect.height);
		}
	}

	ctx->is_cfg_changed = true;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d), enable=%d, rect(%d %d %d %d)\n",
		grp_id, crop_info->enable,
		crop_info->crop_rect.x, crop_info->crop_rect.y,
		crop_info->crop_rect.width, crop_info->crop_rect.height);

	return 0;
}

//TBD
signed int vpss_get_grp_frame(vpss_grp grp_id, video_frame_info_s *video_frame)
{
	signed int ret;

	TRACE_VPSS(DBG_DEBUG, "Grp(%d)\n", grp_id);

	ret = mod_check_null_ptr(ID_VPSS, video_frame);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	return 0;
}

signed int vpss_release_grp_frame(vpss_grp grp_id, const video_frame_info_s *video_frame)
{
	signed int ret;

	TRACE_VPSS(DBG_DEBUG, "Grp(%d)\n", grp_id);

	ret = mod_check_null_ptr(ID_VPSS, video_frame);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	return 0;
}

vb_blk vb_create_block_vpss(struct vb_s *p, uint64_t phy_addr, void *vir_addr, vb_pool vb_pool, bool is_external)
{
	p->phy_addr = phy_addr;
	p->vir_addr = vir_addr;
	p->poolid = vb_pool;
	atomic_set(&p->usr_cnt, 10);
	p->magic = VB_MAGIC;
	atomic_long_set(&p->mod_ids, 0);
	p->external = is_external;

	return (vb_blk)p;
}

signed int vpss_send_frame(vpss_grp grp_id, const video_frame_info_s *video_frame, signed int milli_sec)
{
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = 0};
	vb_blk blk;
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, video_frame);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	if (ctx->online_from_isp) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not support if online\n", grp_id);
		return ERR_VPSS_NOT_PERM;
	}

	if (!ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not yet started.\n", grp_id);
		return ERR_VPSS_NOTREADY;
	}
	if (ctx->grp_attr.pixel_format != video_frame->video_frame.pixel_format) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) PixelFormat(%d) mismatch.\n"
			, grp_id, video_frame->video_frame.pixel_format);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	if ((ctx->grp_attr.w != video_frame->video_frame.width)
	 || (ctx->grp_attr.h != video_frame->video_frame.height)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Size(%d * %d) mismatch.\n"
			, grp_id, video_frame->video_frame.width, video_frame->video_frame.height);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	if (IS_FRAME_OFFSET_INVALID(video_frame->video_frame)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) frame offset (%d %d %d %d) invalid\n",
			grp_id, video_frame->video_frame.offset_left, video_frame->video_frame.offset_right,
			video_frame->video_frame.offset_top, video_frame->video_frame.offset_bottom);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	if (IS_FMT_YUV420(ctx->grp_attr.pixel_format)) {
		if ((video_frame->video_frame.width - video_frame->video_frame.offset_left -
		     video_frame->video_frame.offset_right) & 0x01) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) YUV420 can't accept odd frame valid width\n", grp_id);
			TRACE_VPSS(DBG_ERR, "width(%d) offset_left(%d) offset_right(%d)\n",
				video_frame->video_frame.width, video_frame->video_frame.offset_left,
				video_frame->video_frame.offset_right);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		if ((video_frame->video_frame.height - video_frame->video_frame.offset_top -
		     video_frame->video_frame.offset_bottom) & 0x01) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) YUV420 can't accept odd frame valid height\n", grp_id);
			TRACE_VPSS(DBG_ERR, "height(%d) offset_top(%d) offset_bottom(%d)\n",
				video_frame->video_frame.height, video_frame->video_frame.offset_top,
				video_frame->video_frame.offset_bottom);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}
	if (IS_FMT_YUV422(ctx->grp_attr.pixel_format)) {
		if ((video_frame->video_frame.width - video_frame->video_frame.offset_left -
		     video_frame->video_frame.offset_right) & 0x01) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) YUV422 can't accept odd frame valid width\n", grp_id);
			TRACE_VPSS(DBG_ERR, "width(%d) offset_left(%d) offset_right(%d)\n",
				video_frame->video_frame.width, video_frame->video_frame.offset_left,
				video_frame->video_frame.offset_right);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(video_frame->video_frame.phyaddr[0], NULL, VB_EXTERNAL_POOLID, true);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) no space for malloc.\n", grp_id);
			return ERR_VPSS_NOMEM;
		}
	}

	if (base_fill_videoframe2buffer(chn, video_frame, &((struct vb_s *)blk)->buf) != 0) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Invalid parameter\n", grp_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vpss_grp_qbuf(chn, blk) != 0) {
		TRACE_VPSS(DBG_ERR, "vpss_grp_qbuf(%d) failed\n", grp_id);
		return ERR_VPSS_BUSY;
	}

	TRACE_VPSS(DBG_DEBUG, "Grp(%d), phy-address(0x%llx)\n",
			grp_id, video_frame->video_frame.phyaddr[0]);

	return 0;
}

signed int vpss_send_chn_frame(vpss_grp grp_id, vpss_chn chn_id
	, const video_frame_info_s *video_frame, signed int milli_sec)
{
	signed int ret;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = chn_id};
	vb_blk blk;
	struct vb_s *vb;
	struct vb_jobs_t *jobs;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, video_frame);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	if (!ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not yet started.\n", grp_id);
		return ERR_VPSS_NOTREADY;
	}
	if (!ctx->chn_cfgs[chn_id].is_enabled) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", grp_id, chn_id);
		return ERR_VPSS_NOTREADY;
	}
	if (ctx->chn_cfgs[chn_id].chn_attr.pixel_format != video_frame->video_frame.pixel_format) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) PixelFormat(%d) mismatch.\n"
			, grp_id, chn_id, video_frame->video_frame.pixel_format);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	if ((ctx->chn_cfgs[chn_id].chn_attr.width != video_frame->video_frame.width)
	 || (ctx->chn_cfgs[chn_id].chn_attr.height != video_frame->video_frame.height)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Size(%d * %d) mismatch.\n"
			, grp_id, chn_id, video_frame->video_frame.width, video_frame->video_frame.height);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	UNUSED(milli_sec);

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(video_frame->video_frame.phyaddr[0], NULL, VB_EXTERNAL_POOLID, true);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) no space for malloc.\n", grp_id);
			return ERR_VPSS_NOMEM;
		}
	}

	if (base_fill_videoframe2buffer(chn, video_frame, &((struct vb_s *)blk)->buf) != 0) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Invalid parameter\n", grp_id, chn_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	jobs = &g_vpss_vb_jobs[grp_id].outs[chn_id];
	if (!jobs) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) get job failed\n",
				grp_id, chn_id);
		return -1;
	}

	vb = (struct vb_s *)blk;
	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) waitq is full\n", grp_id, chn_id);
		mutex_unlock(&jobs->lock);
		return -1;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	atomic_fetch_add(1, &vb->usr_cnt);
	mutex_unlock(&jobs->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return ret;
}

signed int vpss_get_chn_frame(vpss_grp grp_id, vpss_chn chn_id, video_frame_info_s *frame_info,
			   signed int milli_sec)
{
	signed int ret, i;
	vb_blk blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = chn_id};
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, frame_info);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	if (!ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not yet started.\n", grp_id);
		return ERR_VPSS_NOTREADY;
	}
	if (!ctx->chn_cfgs[chn_id].is_enabled) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", grp_id, chn_id);
		return ERR_VPSS_NOTREADY;
	}

	memset(frame_info, 0, sizeof(*frame_info));
	ret = base_get_chn_buffer(chn, &g_vpss_vb_jobs[grp_id].outs[chn_id], &blk, milli_sec);
	if (ret != 0 || blk == VB_INVALID_HANDLE) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) Chn(%d) get chn frame null, milli_sec=%d, ret=%d\n",
				grp_id, chn_id, milli_sec, ret);
		return ERR_VPSS_BUF_EMPTY;
	}

	vb = (struct vb_s *)blk;
	if (!vb->buf.phy_addr[0] || !vb->buf.size.width) {
		TRACE_VPSS(DBG_ERR, "buf already released\n");
		return ERR_VPSS_BUF_EMPTY;
	}

	frame_info->video_frame.pixel_format = ctx->chn_cfgs[chn_id].chn_attr.pixel_format;
	frame_info->video_frame.width = vb->buf.size.width;
	frame_info->video_frame.height = vb->buf.size.height;
	frame_info->video_frame.time_ref = vb->buf.frm_num;
	frame_info->video_frame.pts = vb->buf.pts;
	frame_info->video_frame.frame_flag = vb->buf.frame_flag;
	for (i = 0; i < 3; ++i) {
		frame_info->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
		frame_info->video_frame.length[i] = vb->buf.length[i];
		frame_info->video_frame.stride[i] = vb->buf.stride[i];
	}

	frame_info->video_frame.offset_top = vb->buf.offset_top;
	frame_info->video_frame.offset_bottom = vb->buf.offset_bottom;
	frame_info->video_frame.offset_left = vb->buf.offset_left;
	frame_info->video_frame.offset_right = vb->buf.offset_right;
	frame_info->video_frame.private_data = vb;

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) end to set frame_info width:%d height:%d buf:0x%llx\n"
			, grp_id, chn_id, frame_info->video_frame.width, frame_info->video_frame.height,
			frame_info->video_frame.phyaddr[0]);
	return 0;
}

signed int vpss_release_chn_frame(vpss_grp grp_id, vpss_chn chn_id, const video_frame_info_s *video_frame)
{
	vb_blk blk;
	signed int ret;

	ret = mod_check_null_ptr(ID_VPSS, video_frame);
	if (ret != 0)
		return ret;

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		if (video_frame->video_frame.private_data == 0) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) phy-address(0x%llx) invalid to locate.\n"
				      , grp_id, chn_id, (unsigned long long)video_frame->video_frame.phyaddr[0]);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		blk = (vb_blk)video_frame->video_frame.private_data;
	}

	if (vb_release_block(blk) != 0)
		return -1;

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) buf:0x%llx\n",
			grp_id, chn_id, video_frame->video_frame.phyaddr[0]);
	return 0;
}

signed int vpss_set_chn_rotation(vpss_grp grp_id, vpss_chn chn_id, rotation_e rotation)
{
	signed int ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_gdc_fmt(grp_id, chn_id, g_vpss_ctx[grp_id]->chn_cfgs[chn_id].chn_attr.pixel_format);
	if (ret != 0)
		return ret;

	return _vpss_update_rotation_mesh(grp_id, chn_id, rotation);
}

signed int vpss_get_chn_rotation(vpss_grp grp_id, vpss_chn chn_id, rotation_e *rotation)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, rotation);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	*rotation = ctx->chn_cfgs[chn_id].rotation;
	mutex_unlock(&ctx->lock);

	return 0;
}

signed int vpss_set_chn_align(vpss_grp grp_id, vpss_chn chn_id, unsigned int align)
{
	signed int ret;
	vb_cal_config_s vb_cal_config;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	if (align > MAX_ALIGN) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) alignment(%d) exceeding the maximum value %d\n",
			grp_id, chn_id, align, MAX_ALIGN);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	ctx = g_vpss_ctx[grp_id];

	common_getpicbufferconfig(ctx->chn_cfgs[chn_id].chn_attr.width,
		ctx->chn_cfgs[chn_id].chn_attr.height,
		ctx->chn_cfgs[chn_id].chn_attr.pixel_format,
		DATA_BITWIDTH_8, COMPRESS_MODE_NONE, align, &vb_cal_config);

	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].blk_size = vb_cal_config.vb_size;
	ctx->chn_cfgs[chn_id].align = align;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) align:%d\n", grp_id, chn_id, align);
	return 0;
}

signed int vpss_get_chn_align(vpss_grp grp_id, vpss_chn chn_id, unsigned int *palign)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, palign);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	*palign = ctx->chn_cfgs[chn_id].align;
	mutex_unlock(&ctx->lock);

	return 0;
}

signed int vpss_set_chn_scale_coef_level(vpss_grp grp_id, vpss_chn chn_id, vpss_scale_coef_e coef)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	if (coef >= VPSS_SCALE_COEF_MAX) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) undefined scale_coef type(%d)\n"
			, grp_id, chn_id, coef);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].coef = coef;
	ctx->chn_cfgs[chn_id].is_cfg_changed = true;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

signed int vpss_get_chn_scale_coef_level(vpss_grp grp_id, vpss_chn chn_id, vpss_scale_coef_e *coef)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = mod_check_null_ptr(ID_VPSS, coef);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	*coef = ctx->chn_cfgs[chn_id].coef;
	mutex_unlock(&ctx->lock);

	return 0;
}

signed int vpss_set_chn_draw_rect(vpss_grp grp_id, vpss_chn chn_id, const vpss_draw_rect_s *draw_rect)
{
	signed int i, ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	for (i = 0; i < VPSS_RECT_NUM; i++) {
		if (!draw_rect->rects[i].enable)
			continue;
		if (draw_rect->rects[i].rect.width < (2 * draw_rect->rects[i].thick)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d), Width less than 2 times thickness.\n",
				grp_id, chn_id);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		if (draw_rect->rects[i].rect.height < (2 * draw_rect->rects[i].thick)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d), Height less than 2 times thickness.\n",
				grp_id, chn_id);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}
	ctx = g_vpss_ctx[grp_id];

	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].draw_rect = *draw_rect;
	ctx->chn_cfgs[chn_id].is_cfg_changed = true;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

signed int vpss_get_chn_draw_rect(vpss_grp grp_id, vpss_chn chn_id, vpss_draw_rect_s *draw_rect)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx ->lock);
	*draw_rect =ctx ->chn_cfgs[chn_id].draw_rect;
	mutex_unlock(&ctx ->lock);

	return 0;
}

signed int vpss_set_chn_convert(vpss_grp grp_id, vpss_chn chn_id, const vpss_convert_s *convert)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].convert = *convert;
	ctx->chn_cfgs[chn_id].is_cfg_changed = true;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

signed int vpss_get_chn_convert(vpss_grp grp_id, vpss_chn chn_id, vpss_convert_s *convert)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	*convert = ctx->chn_cfgs[chn_id].convert;
	mutex_unlock(&ctx->lock);

	return 0;
}

/* VPSS_SetChnYRatio: Modify the y ratio of chn output. Only work for yuv format.
 *
 * @param grp_id: The Vpss Grp to work.
 * @param chn_id: The Vpss Chn to work.
 * @param y_ratio: Output's Y will be sacled by this ratio.
 * @return: 0 if OK.
 */
signed int vpss_set_chn_yratio(vpss_grp grp_id, vpss_chn chn_id, unsigned int y_ratio)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];

	if (!IS_FMT_YUV(ctx->chn_cfgs[chn_id].chn_attr.pixel_format)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) isn't YUV format. Can't apply this setting.\n"
			, grp_id, chn_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (ctx->chn_cfgs[chn_id].chn_attr.normalize.enable) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Y-ratio adjustment can't work with normalize.\n"
			, grp_id, chn_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (y_ratio > YRATIO_SCALE) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) y_ratio(%d) out of range(0-%d).\n"
			, grp_id, chn_id, y_ratio, YRATIO_SCALE);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].y_ratio = y_ratio;
	ctx->chn_cfgs[chn_id].is_cfg_changed = true;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

signed int vpss_get_chn_yratio(vpss_grp grp_id, vpss_chn chn_id, unsigned int *y_ratio)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	*y_ratio = ctx->chn_cfgs[chn_id].y_ratio;
	mutex_unlock(&ctx->lock);

	return 0;
}

signed int vpss_set_chn_ldc_attr(vpss_grp grp_id, vpss_chn chn_id,
				const vpss_ldc_attr_s *ldc_attr, unsigned long long mesh_addr)
{
	signed int ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_dwa_fmt(grp_id, chn_id, g_vpss_ctx[grp_id]->chn_cfgs[chn_id].chn_attr.pixel_format);
	if (ret != 0)
		return ret;

	return _vpss_update_ldc_mesh(grp_id, chn_id, ldc_attr, mesh_addr);
}

signed int vpss_get_chn_ldc_attr(vpss_grp grp_id, vpss_chn chn_id, vpss_ldc_attr_s *ldc_attr)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	memcpy(ldc_attr, &ctx->chn_cfgs[chn_id].ldc_attr, sizeof(*ldc_attr));
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "enable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d\n",
			ldc_attr->enable, ldc_attr->attr.aspect,
			ldc_attr->attr.xy_ratio, ldc_attr->attr.center_x_offset,
			ldc_attr->attr.center_y_offset, ldc_attr->attr.distortion_ratio);

	return 0;
}

signed int vpss_set_chn_fisheye_attr(vpss_grp grp_id, vpss_chn chn_id, rotation_e rotation,
				const fisheye_attr_s *fish_eye_attr, unsigned long long mesh_addr)
{
	signed int ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	if (fish_eye_attr->enable_bg_color)
		ret = check_vpss_dwa_fmt(grp_id, chn_id, g_vpss_ctx[grp_id]->chn_cfgs[chn_id].chn_attr.pixel_format);
	if (ret != 0)
		return ret;

	return _vpss_update_fisheye_mesh(grp_id, chn_id, fish_eye_attr, rotation, mesh_addr);
}

signed int vpss_get_chn_fisheye_attr(vpss_grp grp_id, vpss_chn chn_id, fisheye_attr_s *fish_eye_attr)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	memcpy(fish_eye_attr, &ctx->chn_cfgs[chn_id].fisheye_attr, sizeof(*fish_eye_attr));
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "enable=%d, enable_bgcolor=%d, bgcolor=%d,\
			hor_offset=%d, ver_offset=%d, trapezoid_coef=%d, fan_strength=%d,\
			mount_mode=%d, use_mode=%d, region_num=%d\n",
			fish_eye_attr->enable_bg_color, fish_eye_attr->enable_bg_color,
			fish_eye_attr->bg_color, fish_eye_attr->hor_offset,
			fish_eye_attr->ver_offset, fish_eye_attr->trapezoid_coef,
			fish_eye_attr->fan_strength, fish_eye_attr->mount_mode,
			fish_eye_attr->use_mode, fish_eye_attr->region_num);

	return 0;
}

signed int vpss_attach_vb_pool(vpss_grp grp_id, vpss_chn chn_id, vb_pool vb_pool)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].vb_pool = vb_pool;
	ctx->chn_cfgs[chn_id].chn_work_status.pool_id = vb_pool;
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) attach vb pool(%d)\n",
		grp_id, chn_id, vb_pool);
	return 0;
}

signed int vpss_detach_vb_pool(vpss_grp grp_id, vpss_chn chn_id)
{
	signed int ret;
	struct vpss_ctx *ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	ctx = g_vpss_ctx[grp_id];
	mutex_lock(&ctx->lock);
	ctx->chn_cfgs[chn_id].vb_pool = VB_INVALID_POOLID;
	ctx->chn_cfgs[chn_id].chn_work_status.pool_id =
		find_vb_pool(ctx->chn_cfgs[chn_id].blk_size);
	mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

signed int vpss_trigger_snap_frame(vpss_grp grp_id, vpss_chn chn_id, unsigned int frame_cnt)
{
	signed int ret;
	mmf_bind_dest_s bind_dest;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = chn_id};
	struct base_exe_m_cb exe_cb;
	struct venc_snap_frm_info sanp_info;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id);
	if (ret != 0)
		return ret;

	if (bind_get_dst(&chn, &bind_dest) != 0) {
		TRACE_VPSS(DBG_WARN, "sys_get_bindbysrc fails\n");
		return ERR_VPSS_NOT_PERM;
	}
	if (bind_dest.mmf_chn[0].mod_id != ID_VENC) {
		TRACE_VPSS(DBG_INFO, "next Mod(%d) is not vcodec\n",
				bind_dest.mmf_chn[0].mod_id);
		return ERR_VPSS_NOT_PERM;
	}
	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);

	sanp_info.vpss_grp = grp_id;
	sanp_info.vpss_chn = chn_id;
	sanp_info.skip_frm_cnt = frame_cnt;

	exe_cb.callee = E_MODULE_VCODEC;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VCODEC_CB_SNAP_JPG_FRM;
	exe_cb.data   = &sanp_info;
	ret = base_exe_module_cb(&exe_cb);

	return ret;
}

void stitch_wakeup(void *data)
{
	struct vpss_stitch_data *stitch_data = (struct vpss_stitch_data *)data;

	stitch_data->flag = 1;
	wake_up(&stitch_data->wait);
}

signed int vpss_stitch(unsigned int chn_num, vpss_stitch_chn_attr_s *input,
			vpss_stitch_output_attr_s *output, video_frame_info_s *video_frame)
{
	signed int ret = 0;
	struct vpss_stitch_cfg cfg;
	struct stitch_dst_cfg dst_cfg;
	struct stitch_chn_cfg *chn_cfg = NULL;
	unsigned long timeout = msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS);
	unsigned char i;
	struct vpss_stitch_data *stitch_data = NULL;
	vb_cal_config_s vb_cal_config;
	vb_blk blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	size_s out_size = {output->width, output->height};

	chn_cfg = (struct stitch_chn_cfg *)vzalloc(sizeof(struct stitch_chn_cfg) * chn_num);
	if (!chn_cfg) {
		return ERR_VPSS_NOMEM;
	}

	for (i = 0; i < chn_num; i++) {
		chn_cfg[i].pixelformat = input[i].video_frame.video_frame.pixel_format;
		chn_cfg[i].bytesperline[0] = input[i].video_frame.video_frame.stride[0];
		chn_cfg[i].bytesperline[1] = input[i].video_frame.video_frame.stride[1];
		chn_cfg[i].addr[0] = input[i].video_frame.video_frame.phyaddr[0];
		chn_cfg[i].addr[1] = input[i].video_frame.video_frame.phyaddr[1];
		chn_cfg[i].addr[2] = input[i].video_frame.video_frame.phyaddr[2];
		chn_cfg[i].src_size.width = input[i].video_frame.video_frame.width;
		chn_cfg[i].src_size.height = input[i].video_frame.video_frame.height;
		chn_cfg[i].rect_crop.left = 0;
		chn_cfg[i].rect_crop.top = 0;
		chn_cfg[i].rect_crop.width = input[i].video_frame.video_frame.width;
		chn_cfg[i].rect_crop.height = input[i].video_frame.video_frame.height;

		chn_cfg[i].window.rect_out.left = input[i].dst_rect.x;
		chn_cfg[i].window.rect_out.top = input[i].dst_rect.y;
		chn_cfg[i].window.rect_out.width = input[i].dst_rect.width;
		chn_cfg[i].window.rect_out.height = input[i].dst_rect.height;
		chn_cfg[i].window.rect_in.left = input[i].dst_rect.x;
		chn_cfg[i].window.rect_in.top = input[i].dst_rect.y;
		chn_cfg[i].window.rect_in.width = input[i].dst_rect.width;
		chn_cfg[i].window.rect_in.height = input[i].dst_rect.height;
	}

	common_getpicbufferconfig(output->width, output->height,
		output->pixel_format, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &vb_cal_config);

	blk = vb_get_block_with_id(VB_INVALID_POOLID, vb_cal_config.vb_size, ID_VPSS);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VPSS(DBG_ERR, "Can't acquire VB BLK for vpss stitch.\n");
		ret = ERR_VPSS_NOBUF;
		goto EXIT;
	}

	vb = (struct vb_s *)blk;
	base_get_frame_info(output->pixel_format
			   , out_size
			   , &vb->buf
			   , vb_handle2phys_addr(blk)
			   , DEFAULT_ALIGN);

	dst_cfg.bytesperline[0] = vb->buf.stride[0];
	dst_cfg.bytesperline[1] = vb->buf.stride[1];
	dst_cfg.addr[0] = vb->buf.phy_addr[0];
	dst_cfg.addr[1] = vb->buf.phy_addr[1];
	dst_cfg.addr[2] = vb->buf.phy_addr[2];
	dst_cfg.pixelformat = output->pixel_format;
	dst_cfg.color[0] = (output->color & 0xFF0000) >> 16;
	dst_cfg.color[1] = (output->color & 0xFF00) >> 8;
	dst_cfg.color[2] = output->color & 0xFF;
	dst_cfg.dst_size.width = output->width;
	dst_cfg.dst_size.height = output->height;

	stitch_data = (struct vpss_stitch_data *)vmalloc(sizeof(struct vpss_stitch_data));
	if (!stitch_data) {
		vb_release_block(blk);
		goto EXIT;
	}

	init_waitqueue_head(&stitch_data->wait);
	stitch_data->flag = 0;

	cfg.num = chn_num;
	cfg.chn_cfg = chn_cfg;
	cfg.dst_cfg = dst_cfg;
	cfg.data = (void *)stitch_data;
	cfg.job_cb = stitch_wakeup;

	ret = vpss_hal_stitch_schedule(&cfg);
	if (ret) {
		TRACE_VPSS(DBG_ERR, "vpss_hal_stitch_schedule fail!\n");
		vb_release_block(blk);
		goto EXIT;
	}

	ret = wait_event_timeout(stitch_data->wait, stitch_data->flag, timeout);
	stitch_data->flag = 0;

	if (ret < 0) {
		TRACE_VPSS(DBG_ERR, "-ERESTARTSYS!\n");
		vb_release_block(blk);
		ret = -1;
	} else if (ret == 0) {
		TRACE_VPSS(DBG_ERR, "timeout!\n");
		vb_release_block(blk);
		ret = -1;
	} else {
		memset(video_frame, 0, sizeof(*video_frame));
		video_frame->video_frame.pixel_format = output->pixel_format;
		video_frame->video_frame.width = vb->buf.size.width;
		video_frame->video_frame.height = vb->buf.size.height;
		for (i = 0; i < 3; ++i) {
			video_frame->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
			video_frame->video_frame.length[i] = vb->buf.length[i];
			video_frame->video_frame.stride[i] = vb->buf.stride[i];
		}
		ret = 0;
	}

EXIT:
	if (chn_cfg)
		vfree(chn_cfg);

	if (stitch_data)
		vfree(stitch_data);

	return ret;
}

signed int video_frame_dmabuf_fd_to_paddr(video_frame_s *video_frame){
	int dmabuf_fd = video_frame->phyaddr[0];
	int height = video_frame->height;
	pixel_format_e pixel_format = video_frame->pixel_format;
	int uv_height = (pixel_format == PIXEL_FORMAT_YUV_PLANAR_420 || pixel_format == PIXEL_FORMAT_NV21 ||
		pixel_format == PIXEL_FORMAT_NV12) ? (height / 2) : height;
	TRACE_VPSS(DBG_DEBUG, "dmabuf_fd%d\n", dmabuf_fd);
	video_frame->phyaddr[0] = vpss_dmabuf_fd_to_paddr(dmabuf_fd);
	video_frame->phyaddr[1] = video_frame->phyaddr[0]
		+ video_frame->stride[0] * height;
	video_frame->phyaddr[2] = video_frame->phyaddr[1]
		+ video_frame->stride[1] * uv_height;
	TRACE_VPSS(DBG_DEBUG, "phyaddr%lx, %lx, %lx\n", (long unsigned int)video_frame->phyaddr[0],
		(long unsigned int)video_frame->phyaddr[1], (long unsigned int)video_frame->phyaddr[2]);
	if(!video_frame->phyaddr[0]){
		TRACE_VPSS(DBG_ERR, "vpss_dmabuf_fd_to_paddr fail\n");
		return -1;
	}
	return 0;
}

signed int vpss_bm_send_frame(bm_vpss_cfg *vpss_cfg){
	signed int ret = 0, i = 0, j = 0;
	struct vpss_job *job = kzalloc(sizeof(struct vpss_job), GFP_ATOMIC);
	struct vpss_hal_grp_cfg *grp_hw_cfg = &job->cfg.grp_cfg;
	struct vpss_hal_chn_cfg *chn_hw_cfg = &job->cfg.chn_cfg[0];
	struct vpss_stitch_data data;

	if(!g_is_bm_scene) g_is_bm_scene = true;

	init_waitqueue_head(&data.wait);
	data.flag = 0;

	job->grp_id = 0;
	job->is_online = false;
	job->data = (void *)&data;
	job->job_cb = vpss_wkup_frame_done_handle;
	atomic_set(&job->job_state, JOB_INVALID);
	spin_lock_init(&job->lock);

	job->cfg.chn_num = 1;
	job->cfg.chn_enable[0] = true;

	if(vpss_cfg->grp_csc_cfg.enable){
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++)
				grp_hw_cfg->csc_cfg.coef[i][j] = vpss_cfg->grp_csc_cfg.coef[i][j];
			grp_hw_cfg->csc_cfg.add[i] = vpss_cfg->grp_csc_cfg.add[i];
			grp_hw_cfg->csc_cfg.sub[i] = vpss_cfg->grp_csc_cfg.sub[i];
		}
		grp_hw_cfg->upsample = vpss_cfg->grp_csc_cfg.is_copy_upsample ? false : true;
	}

	if(vpss_cfg->chn_csc_cfg.enable){
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++)
				chn_hw_cfg->csc_cfg.coef[i][j] = vpss_cfg->chn_csc_cfg.coef[i][j];

			chn_hw_cfg->csc_cfg.add[i] = vpss_cfg->chn_csc_cfg.add[i];
			chn_hw_cfg->csc_cfg.sub[i] = vpss_cfg->chn_csc_cfg.sub[i];
		}
	}
	chn_hw_cfg->y_ratio = YRATIO_SCALE;

	if(vpss_cfg->grp_crop_cfg.crop_info.enable){
		grp_hw_cfg->crop.width = vpss_cfg->grp_crop_cfg.crop_info.crop_rect.width;
		grp_hw_cfg->crop.height = vpss_cfg->grp_crop_cfg.crop_info.crop_rect.height;
		grp_hw_cfg->crop.left = vpss_cfg->grp_crop_cfg.crop_info.crop_rect.x;
		grp_hw_cfg->crop.top = vpss_cfg->grp_crop_cfg.crop_info.crop_rect.y;
	} else {
		grp_hw_cfg->crop.left = 0;
		grp_hw_cfg->crop.top = 0;
		grp_hw_cfg->crop.width = vpss_cfg->snd_frm_cfg.video_frame.video_frame.width;
		grp_hw_cfg->crop.height = vpss_cfg->snd_frm_cfg.video_frame.video_frame.height;
	}

	if(vpss_cfg->chn_crop_cfg.crop_info.enable){
		chn_hw_cfg->crop.left = vpss_cfg->chn_crop_cfg.crop_info.crop_rect.x;
		chn_hw_cfg->crop.top = vpss_cfg->chn_crop_cfg.crop_info.crop_rect.y;
		chn_hw_cfg->crop.width = vpss_cfg->chn_crop_cfg.crop_info.crop_rect.width;
		chn_hw_cfg->crop.height = vpss_cfg->chn_crop_cfg.crop_info.crop_rect.height;
	} else {
		chn_hw_cfg->crop.left = 0;
		chn_hw_cfg->crop.top = 0;
		chn_hw_cfg->crop.width = grp_hw_cfg->crop.width;
		chn_hw_cfg->crop.height = grp_hw_cfg->crop.height;
	}

	if(vpss_cfg->chn_convert_cfg.convert.enable){
		chn_hw_cfg->convert_to_cfg.enable = vpss_cfg->chn_convert_cfg.convert.enable;
		chn_hw_cfg->convert_to_cfg.a_frac[0] = vpss_cfg->chn_convert_cfg.convert.a_factor[0];
		chn_hw_cfg->convert_to_cfg.a_frac[1] = vpss_cfg->chn_convert_cfg.convert.a_factor[1];
		chn_hw_cfg->convert_to_cfg.a_frac[2] = vpss_cfg->chn_convert_cfg.convert.a_factor[2];
		chn_hw_cfg->convert_to_cfg.b_frac[0] = vpss_cfg->chn_convert_cfg.convert.b_factor[0];
		chn_hw_cfg->convert_to_cfg.b_frac[1] = vpss_cfg->chn_convert_cfg.convert.b_factor[1];
		chn_hw_cfg->convert_to_cfg.b_frac[2] = vpss_cfg->chn_convert_cfg.convert.b_factor[2];
	}

	if(vpss_cfg->chn_draw_rect_cfg.draw_rect.rects[0].enable){
		for (i = 0; i < VPSS_RECT_NUM; i++) {
			rect_s rect;
			unsigned short thick;

			chn_hw_cfg->border_vpp_cfg[i].enable = vpss_cfg->chn_draw_rect_cfg.draw_rect.rects[i].enable;
			if (chn_hw_cfg->border_vpp_cfg[i].enable) {
				chn_hw_cfg->border_vpp_cfg[i].bg_color[0] = (vpss_cfg->chn_draw_rect_cfg.draw_rect.rects[i].bg_color >> 16) & 0xff;
				chn_hw_cfg->border_vpp_cfg[i].bg_color[1] = (vpss_cfg->chn_draw_rect_cfg.draw_rect.rects[i].bg_color >> 8) & 0xff;
				chn_hw_cfg->border_vpp_cfg[i].bg_color[2] = vpss_cfg->chn_draw_rect_cfg.draw_rect.rects[i].bg_color & 0xff;

				rect = vpss_cfg->chn_draw_rect_cfg.draw_rect.rects[i].rect;
				thick = vpss_cfg->chn_draw_rect_cfg.draw_rect.rects[i].thick;

				chn_hw_cfg->border_vpp_cfg[i].outside.start_x = rect.x;
				chn_hw_cfg->border_vpp_cfg[i].outside.start_y = rect.y;
				chn_hw_cfg->border_vpp_cfg[i].outside.end_x = rect.x + rect.width;
				chn_hw_cfg->border_vpp_cfg[i].outside.end_y = rect.y + rect.height;
				chn_hw_cfg->border_vpp_cfg[i].inside.start_x = rect.x + thick;
				chn_hw_cfg->border_vpp_cfg[i].inside.start_y = rect.y + thick;
				chn_hw_cfg->border_vpp_cfg[i].inside.end_x =
					chn_hw_cfg->border_vpp_cfg[i].outside.end_x - thick;
				chn_hw_cfg->border_vpp_cfg[i].inside.end_y =
					chn_hw_cfg->border_vpp_cfg[i].outside.end_y - thick;
			}
		}
	}

	if(vpss_cfg->coverex_cfg.rgn_coverex_cfg.rgn_coverex_param[0].enable){
		chn_hw_cfg->rgn_coverex_cfg = vpss_cfg->coverex_cfg.rgn_coverex_cfg;
	}

	for(i = 0; i < RGN_MAX_LAYER_VPSS; i++)
		if(vpss_cfg->rgn_cfg[i].num_of_rgn > 0)
			chn_hw_cfg->rgn_cfg[i] = vpss_cfg->rgn_cfg[i];

	grp_hw_cfg->src_size.width = vpss_cfg->snd_frm_cfg.video_frame.video_frame.width;
	grp_hw_cfg->src_size.height = vpss_cfg->snd_frm_cfg.video_frame.video_frame.height;
	grp_hw_cfg->pixelformat = vpss_cfg->snd_frm_cfg.video_frame.video_frame.pixel_format;
	for (i = 0; i < NUM_OF_PLANES; ++i)
		grp_hw_cfg->addr[i] = vpss_cfg->snd_frm_cfg.video_frame.video_frame.phyaddr[i];
	if (vpss_cfg->snd_frm_cfg.video_frame.video_frame.pixel_format == PIXEL_FORMAT_BGR_888_PLANAR) {
		grp_hw_cfg->addr[0] = vpss_cfg->snd_frm_cfg.video_frame.video_frame.phyaddr[2];
		grp_hw_cfg->addr[2] = vpss_cfg->snd_frm_cfg.video_frame.video_frame.phyaddr[0];
	}
	if (vpss_cfg->snd_frm_cfg.video_frame.video_frame.compress_mode == COMPRESS_MODE_FRAME){
		grp_hw_cfg->addr[3] = vpss_cfg->snd_frm_cfg.video_frame.video_frame.ext_phy_addr;
		grp_hw_cfg->fbd_enable = true;
	}
	grp_hw_cfg->bytesperline[0] = vpss_cfg->snd_frm_cfg.video_frame.video_frame.stride[0];
	grp_hw_cfg->bytesperline[1] = vpss_cfg->snd_frm_cfg.video_frame.video_frame.stride[1];
	grp_hw_cfg->bm_scene = true;

	chn_hw_cfg->pixelformat = vpss_cfg->chn_frm_cfg.video_frame.video_frame.pixel_format;
	chn_hw_cfg->bytesperline[0] = vpss_cfg->chn_frm_cfg.video_frame.video_frame.stride[0];
	chn_hw_cfg->bytesperline[1] = vpss_cfg->chn_frm_cfg.video_frame.video_frame.stride[1];
	if(vpss_cfg->chn_frm_cfg.video_frame.video_frame.phyaddr[0] < 0x1000){ // dmabuf_fd
		ret = video_frame_dmabuf_fd_to_paddr(&vpss_cfg->chn_frm_cfg.video_frame.video_frame);
		if(ret != 0){
			kfree(job);
			return ret;
		}
	}
	for (i = 0; i < NUM_OF_PLANES; ++i)
		chn_hw_cfg->addr[i] = vpss_cfg->chn_frm_cfg.video_frame.video_frame.phyaddr[i];
	if (vpss_cfg->chn_frm_cfg.video_frame.video_frame.pixel_format == PIXEL_FORMAT_BGR_888_PLANAR) {
		chn_hw_cfg->addr[0] = vpss_cfg->chn_frm_cfg.video_frame.video_frame.phyaddr[2];
		chn_hw_cfg->addr[2] = vpss_cfg->chn_frm_cfg.video_frame.video_frame.phyaddr[0];
	}
	chn_hw_cfg->src_size.width = grp_hw_cfg->crop.width;
	chn_hw_cfg->src_size.height = grp_hw_cfg->crop.height;
	chn_hw_cfg->dst_size.width = vpss_cfg->chn_frm_cfg.video_frame.video_frame.width;
	chn_hw_cfg->dst_size.height = vpss_cfg->chn_frm_cfg.video_frame.video_frame.height;

	if (vpss_cfg->chn_attr.chn_attr.flip && vpss_cfg->chn_attr.chn_attr.mirror)
		chn_hw_cfg->flip = SC_FLIP_HVFLIP;
	else if (vpss_cfg->chn_attr.chn_attr.flip)
		chn_hw_cfg->flip = SC_FLIP_VFLIP;
	else if (vpss_cfg->chn_attr.chn_attr.mirror)
		chn_hw_cfg->flip = SC_FLIP_HFLIP;
	else
		chn_hw_cfg->flip = SC_FLIP_NO;

	chn_hw_cfg->dst_rect.left = vpss_cfg->chn_attr.chn_attr.aspect_ratio.video_rect.x;
	chn_hw_cfg->dst_rect.top = vpss_cfg->chn_attr.chn_attr.aspect_ratio.video_rect.y;
	chn_hw_cfg->dst_rect.width = vpss_cfg->chn_attr.chn_attr.aspect_ratio.video_rect.width;
	chn_hw_cfg->dst_rect.height = vpss_cfg->chn_attr.chn_attr.aspect_ratio.video_rect.height;

	chn_hw_cfg->border_cfg.enable = vpss_cfg->chn_attr.chn_attr.aspect_ratio.enable_bgcolor
			&& ((chn_hw_cfg->dst_rect.width != chn_hw_cfg->dst_size.width)
			 || (chn_hw_cfg->dst_rect.height != chn_hw_cfg->dst_size.height));
	chn_hw_cfg->border_cfg.offset_x = chn_hw_cfg->dst_rect.left;
	chn_hw_cfg->border_cfg.offset_y = chn_hw_cfg->dst_rect.top;
	chn_hw_cfg->border_cfg.bg_color[2] = vpss_cfg->chn_attr.chn_attr.aspect_ratio.bgcolor & 0xff;
	chn_hw_cfg->border_cfg.bg_color[1] = (vpss_cfg->chn_attr.chn_attr.aspect_ratio.bgcolor >> 8) & 0xff;
	chn_hw_cfg->border_cfg.bg_color[0] = (vpss_cfg->chn_attr.chn_attr.aspect_ratio.bgcolor >> 16) & 0xff;

	switch (vpss_cfg->chn_coef_level_cfg.coef) {
	default:
	case VPSS_SCALE_COEF_BILINEAR:
		chn_hw_cfg->sc_coef = SC_SCALING_COEF_BILINEAR;
		break;
	case VPSS_SCALE_COEF_NEAREST:
		chn_hw_cfg->sc_coef = SC_SCALING_COEF_NEAREST;
		break;
	case VPSS_SCALE_COEF_BICUBIC_OPENCV:
		chn_hw_cfg->sc_coef = SC_SCALING_COEF_BICUBIC_OPENCV;
		break;
	}

	down(&g_vpss_core_sem);

	if (vpss_hal_direct_schedule(job))
		vpss_hal_push_job(job);

	wait_event_timeout(data.wait, data.flag, msecs_to_jiffies(vpss_cfg->chn_frm_cfg.milli_sec));
	if (data.flag){
		ret = 0;
		data.flag = 0;
	} else
		ret = -1;

	if ((atomic_read(&job->job_state) == JOB_WAIT) ||
		(atomic_read(&job->job_state) == JOB_WORKING)){
		vpss_hal_remove_job(job);
		for (i = 0; i < VPSS_MAX; i++) {
			if (!(job->vpss_dev_mask & BIT(i)))
				continue;
			vpss_hal_down_reg(i);
		}
	}
	up(&g_vpss_core_sem);
	kfree(job);

	return ret;
}

signed int vpss_set_vivpss_mode(const vi_vpss_mode_s *mode)
{
	memcpy(&g_vi_vpss_mode, mode, sizeof(g_vi_vpss_mode));

	return 0;
}

void vpss_set_mlv_info(unsigned char snr_num, struct mlv_i_s *p_m_lv_i)
{
	if (snr_num >= ARRAY_SIZE(g_mlv_i)) {
		TRACE_VPSS(DBG_INFO, "snr_num(%d) out of range\n", snr_num);
		return;
	}

	g_mlv_i[snr_num].mlv_i_level = p_m_lv_i->mlv_i_level;
	memcpy(g_mlv_i[snr_num].mlv_i_table, p_m_lv_i->mlv_i_table, sizeof(g_mlv_i[snr_num].mlv_i_table));
}

void vpss_get_mlv_info(unsigned char snr_num, struct mlv_i_s *p_m_lv_i)
{
	if (snr_num >= ARRAY_SIZE(g_mlv_i)) {
		TRACE_VPSS(DBG_ERR, "snr_num(%d) out of range\n", snr_num);
		return;
	}
	p_m_lv_i->mlv_i_level = g_mlv_i[snr_num].mlv_i_level;
	memcpy(p_m_lv_i->mlv_i_table, g_mlv_i[snr_num].mlv_i_table, sizeof(g_mlv_i[snr_num].mlv_i_table));
}

void vpss_mode_init(void)
{
	unsigned char i, j;
	proc_amp_ctrl_s ctrl;

	mutex_lock(&g_vpss_lock);
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		for (j = PROC_AMP_BRIGHTNESS; j < PROC_AMP_MAX; ++j) {
			vpss_get_proc_amp_ctrl(j, &ctrl);
			g_vpss_ext_ctx[i].proc_amp[j] = ctrl.default_value;
		}
		g_vpss_grp_used[i] = false;
	}

	//init_timer(&timer_proc);
	//timer_proc.function = _update_vpss_chn_real_frame_rate;
	//timer_proc.expires = jiffies + msecs_to_jiffies(1000);
	//timer_proc.data = 0;
	if (atomic_cmpxchg(&g_timer_added,0, 1) == 0)
		add_timer(&timer_proc);
	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
	mutex_unlock(&g_vpss_lock);
}

void vpss_mode_deinit(void)
{
	mutex_lock(&g_vpss_lock);
	if (atomic_cmpxchg(&g_timer_added,1, 0) == 1)
		del_timer_sync(&timer_proc);
	mutex_unlock(&g_vpss_lock);
}

void register_timer_fun(vpss_timer_cb cb, void *data)
{
	g_core_cb = cb;
	g_core_data = data;
}

void vpss_init(void)
{
	int ret;
	struct sched_param tsk;

	base_register_recv_cb(ID_VPSS, vpss_grp_qbuf);

	mutex_init(&g_vpss_lock);
	init_waitqueue_head(&g_vpss_hdl_ctx.wait);
	spin_lock_init(&g_vpss_hdl_ctx.hdl_lock);
	atomic_set(&g_vpss_hdl_ctx.active_cnt, 0);
	g_vpss_hdl_ctx.events = 0;

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 4;

	g_vpss_hdl_ctx.thread = kthread_run(vpss_event_handler, &g_vpss_hdl_ctx,
		"task_vpss_hdl");
	if (IS_ERR(g_vpss_hdl_ctx.thread)) {
		TRACE_VPSS(DBG_ERR, "failed to create vpss kthread\n");
	}

	ret = sched_setscheduler(g_vpss_hdl_ctx.thread, SCHED_FIFO, &tsk);
	if (ret)
		TRACE_VPSS(DBG_WARN, "vpss thread priority update failed: %d\n", ret);

	g_vpss_workqueue = alloc_workqueue("vpss_workqueue", WQ_HIGHPRI | WQ_UNBOUND |
						__WQ_LEGACY | WQ_MEM_RECLAIM, 8);
	//g_vpss_workqueue = create_workqueue("vpss_workqueue");
	// g_vpss_workqueue = create_singlethread_workqueue("vpss_workqueue");
	if (!g_vpss_workqueue)
		TRACE_VPSS(DBG_ERR, "vpss create_workqueue failed.\n");
	sema_init(&g_vpss_core_sem, VPSS_WORK_MAX);
}

void vpss_deinit(void)
{
	int ret;

	if (!g_vpss_hdl_ctx.thread) {
		TRACE_VPSS(DBG_ERR, "vpss thread not initialized yet\n");
		return;
	}
	base_unregister_recv_cb(ID_VPSS);

	ret = kthread_stop(g_vpss_hdl_ctx.thread);
	if (ret)
		TRACE_VPSS(DBG_ERR, "fail to stop vpss thread, err=%d\n", ret);

	g_vpss_hdl_ctx.thread = NULL;
	destroy_workqueue(g_vpss_workqueue);
	mutex_destroy(&g_vpss_lock);
}

void vpss_release_grp(void)
{
	vpss_grp grp;
	vpss_chn chn;

	for (grp = 0; grp < VPSS_MAX_GRP_NUM; grp++) {
		if (g_vpss_ctx[grp] && g_vpss_ctx[grp]->is_created) {
			for (chn = 0; chn < VPSS_MAX_CHN_NUM; chn++) {
				if (g_vpss_ctx[grp]->chn_cfgs[chn].is_enabled) {
					vpss_disable_chn(grp, chn);
				}
			}

			vpss_stop_grp(grp);
			vpss_destroy_grp(grp);
		}
	}
}

signed int vpss_suspend_handler(void)
{
	int ret, count;
	vpss_grp grp_id;
	struct vpss_ctx *ctx;

	if (!g_vpss_hdl_ctx.thread) {
		TRACE_VPSS(DBG_ERR, "vpss thread not initialized yet\n");
		return -1;
	}

	ret = kthread_stop(g_vpss_hdl_ctx.thread);
	if (ret) {
		TRACE_VPSS(DBG_ERR, "fail to stop vpss thread, err=%d\n", ret);
		return -1;
	}
	g_vpss_hdl_ctx.thread = NULL;

	mutex_lock(&g_vpss_lock);
	if (atomic_cmpxchg(&g_timer_added,1, 0) == 1)
		del_timer(&timer_proc);
	mutex_unlock(&g_vpss_lock);

	for (grp_id = 0; grp_id < VPSS_MAX_GRP_NUM; ++grp_id) {
		if (!g_vpss_ctx[grp_id])
			continue;

		ctx = g_vpss_ctx[grp_id];

		//wait frame done
		count = 20;
		while (!ctx->online_from_isp && --count > 0) {
			if (atomic_read(&ctx->hdl_state) == HANDLER_STATE_STOP)
				break;
			usleep_range(1000, 2000);
		}
		if (count == 0) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Wait timeout, HW hang.\n", grp_id);
			//TODO: reset dev and job
		}
	}

	return 0;
}

signed int vpss_resume_handler(void)
{
	int ret;
	struct sched_param tsk;

	if (g_vpss_hdl_ctx.thread) {
		TRACE_VPSS(DBG_WARN, "resumed\n");
		return 0;
	}

	g_vpss_hdl_ctx.thread = kthread_run(vpss_event_handler, &g_vpss_hdl_ctx,
		"task_vpss_hdl");
	if (IS_ERR(g_vpss_hdl_ctx.thread)) {
		TRACE_VPSS(DBG_ERR, "failed to create vpss kthread\n");
		return -1;
	}

	tsk.sched_priority = MAX_USER_RT_PRIO - 4;
	ret = sched_setscheduler(g_vpss_hdl_ctx.thread, SCHED_FIFO, &tsk);
	if (ret) {
		TRACE_VPSS(DBG_WARN, "vpss thread priority update failed: %d\n", ret);
		return -1;
	}

	mutex_lock(&g_vpss_lock);
	if (atomic_cmpxchg(&g_timer_added,0, 1) == 0)
		add_timer(&timer_proc);
	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
	mutex_unlock(&g_vpss_lock);

	return 0;
}

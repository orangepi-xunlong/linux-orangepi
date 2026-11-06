
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <uapi/linux/sched/types.h>
#include <linux/delay.h>

#include <linux/comm_video.h>
#include <linux/comm_gdc.h>
#include <linux/ldc_uapi.h>

#include <base_cb.h>
#include <vb.h>
#include <sys.h>
#include "ldc_debug.h"
#include "ldc_core.h"
#include "ldc_sdk.h"
#include "ldc_common.h"
#include "ldc.h"
#include "cmdq.h"
#include "ion.h"
#include "vbq.h"
#include "ldc_proc.h"
#include "mesh.h"
#include "base_cb.h"

#define YUV_8BIT(y, u, v) ((((y)&0xff) << 16) | (((u)&0xff) << 8) | ((v)&0xff))
#define BGCOLOR_GRAY  (0x808080)
#define BGCOLOR_GREEN  (YUV_8BIT(0, 128, 128))
#define LDC_INTR_EN_ALL (0x01)
#define DWA_INTR_EN_ALL (0x07)
#define LDC_INTR_EN_NULL (0x0)
#define LDC_INVALID_CORE_ID (LDC_DEV_MAX_CNT + 1)

#define USE_EXCEPTION_HDL 0
#define USE_REAL_CMDQ 0

static struct mutex g_mesh_lock;

static enum enum_modules_id convert_cb_id(mod_id_e mod_id)
{
	if (mod_id == ID_VI)
		return E_MODULE_VI;
	else if (mod_id == ID_VO)
		return E_MODULE_VO;
	else if (mod_id == ID_VPSS)
		return E_MODULE_VPSS;

	return E_MODULE_BUTT;
}

static mod_id_e convert_mod_id(enum enum_modules_id cbModId)
{
	if (cbModId == E_MODULE_VI)
		return ID_VI;
	else if (cbModId == E_MODULE_VO)
		return ID_VO;
	else if (cbModId == E_MODULE_VPSS)
		return ID_VPSS;

	return ID_BUTT;
}

int ldc_exec_cb(void *dev, enum enum_modules_id caller, unsigned int cmd, void *arg)
{
	struct ldc_vdev *wdev = (struct ldc_vdev *)dev;
	struct mesh_gdc_cfg *cfg;
	mod_id_e mod_id;
	int rc = -1;

	switch (cmd) {
		case LDC_CB_MESH_GDC_OP: {
			mutex_lock(&g_mesh_lock);
			cfg = (struct mesh_gdc_cfg *)arg;
			mod_id = convert_mod_id(caller);
			mutex_unlock(&g_mesh_lock);

			if (mod_id == ID_BUTT) {
				TRACE_LDC(DBG_WARN, "invalid mod do GDC_OP\n");
				return -1;
			}
			rc = mesh_gdc_do_op(wdev, cfg->usage, cfg->usage_param
				, cfg->vb_in, cfg->pix_format, cfg->mesh_addr
				,cfg->sync_io, cfg->cb_param, cfg->cb_param_size
				, mod_id, cfg->rotation);
			break;
		}
		default: {
			TRACE_LDC(DBG_WARN, "invalid cb CMD\n");
			break;
		}
	}

	return rc;
}

int ldc_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_LDC);
}

int ldc_reg_cb(struct ldc_vdev *wdev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_LDC;
	reg_cb.dev		= (void *)wdev;
	reg_cb.cb		= ldc_exec_cb;

	return base_reg_module_cb(&reg_cb);
}

static void ldc_op_done_cb(mod_id_e mod_id, void *param, vb_blk blk)
{
	struct ldc_op_done_cfg cfg;
	struct base_exe_m_cb exe_cb;
	enum enum_modules_id callee = convert_cb_id(mod_id);

	TRACE_LDC(DBG_DEBUG, "callee=%d, mod_id(%d)\n", callee, mod_id);
	cfg.param = param;
	cfg.blk = blk;

	exe_cb.callee = callee;
	exe_cb.caller = E_MODULE_LDC;
	exe_cb.cmd_id = LDC_CB_GDC_OP_DONE;
	exe_cb.data   = &cfg;
	base_exe_module_cb(&exe_cb);
}

static void ldc_hdl_hw_tsk_cb(struct ldc_vdev *wdev, struct ldc_job *job
	, struct ldc_task *tsk, bool is_last)
{
	bool is_internal;
	vb_blk blk_in = VB_INVALID_HANDLE, blk_out = VB_INVALID_HANDLE;
	mod_id_e mod_id;
	unsigned char isLatask;
	struct ldc_vb_done *vb_done = NULL;
	unsigned long flags;

	if (unlikely(!wdev || !job || !tsk)) {
		TRACE_LDC(DBG_ERR, "invalid param\n");
		return;
	}
	is_internal = (tsk->attr.reserved == GDC_MAGIC);
	mod_id = job->identity.mod_id;

	TRACE_LDC(DBG_DEBUG, "is_internal=%d\n", is_internal);

	/* []internal module]:
	 *  sync_io or async_io
	 *  release imgin vb_blk at task done.
	 *  mesh prepared by module.
	 *
	 * []user case]:
	 *  always sync_io.
	 *  Don't care imgin/imgout vb_blk. User release by themselves.
	 *  mesh prepared by ldc at AddXXXTask.
	 *
	 */
	if (is_internal) {
		// for internal module handshaking. such as vi/vpss rotation.
		isLatask = (u8)tsk->attr.private_data[1];

		TRACE_LDC(DBG_DEBUG, "isLatask=%d, blk_in(pa=0x%llx), blk_out(pa=0x%llx)\n",
				isLatask,
				(unsigned long long)tsk->attr.img_in.video_frame.phyaddr[0],
				(unsigned long long)tsk->attr.img_out.video_frame.phyaddr[0]);

		blk_in = vb_phys_addr2handle(tsk->attr.img_in.video_frame.phyaddr[0]);
		blk_out = vb_phys_addr2handle(tsk->attr.img_out.video_frame.phyaddr[0]);
		if (blk_out == VB_INVALID_HANDLE)
			TRACE_LDC(DBG_ERR, "blk_out is invalid vb_blk, no callback to(%d)\n", mod_id);
		else {
			atomic_long_fetch_and(~BIT(ID_GDC), &((struct vb_s *)blk_out)->mod_ids);
			if (isLatask)
				ldc_op_done_cb(mod_id, (void *)(uintptr_t)tsk->attr.private_data[2], blk_out);
		}

		// User space:
		//   Caller always assign callback.
		//   Null callback used for internal ldc sub job.
		// Kernel space:
		//  !isLatask used for internal ldc sub job.
		if (isLatask && blk_in != VB_INVALID_HANDLE) {
			atomic_long_fetch_and(~BIT(ID_GDC), &((struct vb_s *)blk_in)->mod_ids);
			vb_release_block(blk_in);
		} else if (!isLatask) {
			vfree((void *)(uintptr_t)tsk->attr.private_data[2]);
		}
	} else {
		if (!job->identity.sync_io && is_last) {
			vb_done = kzalloc(sizeof(*vb_done), GFP_ATOMIC);

			memcpy(&vb_done->img_out, &tsk->attr.img_out, sizeof(vb_done->img_out));
			memcpy(&vb_done->job, job, sizeof(*job));

			spin_lock_irqsave(&wdev->vb_doneq.lock, flags);
			list_add_tail(&vb_done->node, &wdev->vb_doneq.doneq);
			spin_unlock_irqrestore(&wdev->vb_doneq.lock, flags);

			TRACE_LDC(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
			up(&wdev->vb_doneq.sem);
			TRACE_LDC(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);

			TRACE_LDC(DBG_DEBUG, "vb_done->img_out[%llx-%d]\n"
				, vb_done->img_out.video_frame.phyaddr[0], vb_done->img_out.video_frame.width);
			TRACE_LDC(DBG_DEBUG, "vb_doneq identity[%d-%d-%s]\n"
				, vb_done->job.identity.mod_id, vb_done->job.identity.id, vb_done->job.identity.name);
		}
	}
}

static void ldc_wkup_cmdq_tsk(struct ldc_core *core) {
	unsigned long flags;

#if USE_REAL_CMDQ
	(void)(core);
#else
	spin_lock_irqsave(&core->core_lock, flags);

	core->cmdq_evt = true;
	spin_unlock_irqrestore(&core->core_lock, flags);

	wake_up_interruptible(&core->cmdq_wq);
#endif
	TRACE_LDC(DBG_DEBUG, "ldc_wkup_cmdq_tsk\n");
}

static void ldc_notify_wkup_evt_kth(void *data, enum ldc_wait_evt evt)
{
	struct ldc_vdev *dev = (struct ldc_vdev *)data;
	unsigned long flags;

	if(!dev) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	spin_lock_irqsave(&dev->wdev_lock, flags);
	dev->evt |= evt;
	spin_unlock_irqrestore(&dev->wdev_lock, flags);

	wake_up_interruptible(&dev->wait);

	TRACE_LDC(DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
}

static void ldc_clr_evt_kth(void *data)
{
	struct ldc_vdev *dev = (struct ldc_vdev *)data;
	unsigned long flags;
	enum ldc_wait_evt evt;

	if(!dev) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	evt = dev->evt;
	spin_lock_irqsave(&dev->wdev_lock, flags);
	dev->evt &= ~evt;
	spin_unlock_irqrestore(&dev->wdev_lock, flags);
	TRACE_LDC(DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
}

static void ldc_work_handle_job_done(struct ldc_vdev *dev, struct ldc_job *job)
{
	unsigned long flags;
	struct fasync_struct *fasync;
	struct ldc_core *core;
	int coreid;

	coreid = job->coreid;
	core = &dev->core[coreid];

	atomic_set(&job->job_state, LDC_JOB_END);

	ldc_proc_record_job_done(job);

	spin_lock_irqsave(&job->lock, flags);
	list_del(&job->node);
	job->job_done_evt = true;
	spin_unlock_irqrestore(&job->lock, flags);

	TRACE_LDC(DBG_INFO, "job [%px] done\n", job);

	atomic_set(&core->state, LDC_CORE_STATE_IDLE);

	if (job->identity.sync_io) {
		TRACE_LDC(DBG_INFO, "job[%px] wake endjob\n", job);
		wake_up(&job->job_done_wq);
	} else {
		kfree(job);
		fasync = ldc_get_dev_fasync();
		kill_fasync(&fasync, SIGIO, POLL_IN);
	}

	ldc_notify_wkup_evt_kth(dev, LDC_EVENT_EOF);
}

static void ldc_work_handle_tsk_done(struct ldc_vdev *dev
	, struct ldc_task *tsk, struct ldc_job *job, bool is_last_tsk)
{
	unsigned long flags;

	if (unlikely(!tsk)) {
		TRACE_LDC(DBG_ERR, "null tsk\n");
		return;
	}

	if (atomic_read(&tsk->state) == LDC_TASK_STATE_RUNNING) {
		atomic_set(&tsk->state, LDC_TASK_STATE_DONE);
		atomic_dec(&job->task_num);

		ldc_hdl_hw_tsk_cb(dev, job, tsk, is_last_tsk);

		ldc_proc_record_hw_tsk_done(job, tsk);

		TRACE_LDC(DBG_INFO, "tsk[%px] done, is last tsk[%d]\n", tsk, is_last_tsk);

		spin_lock_irqsave(&job->lock, flags);
		list_del(&tsk->node);
		spin_unlock_irqrestore(&job->lock, flags);
		kfree(tsk);

		if (is_last_tsk) {
			ldc_work_handle_job_done(dev, job);
		}
	} else {
		TRACE_LDC(DBG_ERR, "invalid tsk state(%d).\n"
			, (enum ldc_task_state)atomic_read(&tsk->state));
	}
}

static void ldc_work_handle_frm_done(int coreid)
{
	struct ldc_vdev *dev = ldc_get_dev();
	struct ldc_core *core = &dev->core[coreid];
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long flags;
	bool is_last_tsk;

	spin_lock_irqsave(&core->core_lock, flags);
	job = list_first_entry_or_null(&core->list, struct ldc_job, node);
	spin_unlock_irqrestore(&core->core_lock, flags);

	if (unlikely(!job)) {
		TRACE_LDC(DBG_ERR, "core[%d] null job", coreid);
		return;
	}

	if (atomic_read(&job->job_state) == LDC_JOB_WORKING) {
		spin_lock_irqsave(&job->lock, flags);
		tsk = list_first_entry_or_null(&job->task_list, struct ldc_task, node);
		spin_unlock_irqrestore(&job->lock, flags);

		is_last_tsk = ((atomic_read(&job->task_num) > 1) ? false : true);

		ldc_work_handle_tsk_done(dev, tsk, job, is_last_tsk);

		if (job && job->use_cmdq && !is_last_tsk)
			ldc_wkup_cmdq_tsk(core);
	} else {
		TRACE_LDC(DBG_ERR, "invalid job(%px) state(%d).\n"
			, job, (enum ldc_job_state)atomic_read(&job->job_state));
	}
}

#if LDC_USE_WORKQUEUE
static void ldc_work_frm_done(struct work_struct *work)//intr post handle
{
	struct ldc_core *core = container_of(work, struct ldc_core, work_frm_done);

	ldc_work_handle_frm_done((int)core->dev_type);
}
#endif

//wakeup post handle
void ldc_wkup_frm_done_work(void *data)
{
	struct ldc_core *core = (struct ldc_core *)data;

#if LDC_USE_WORKQUEUE
	//queue_work(dev->workqueue, &dev->work_frm_done);
	schedule_work(&core->work_frm_done);
#else
	ldc_work_handle_frm_done((int)core->dev_type);
#endif
}


static void ldc_set_tsk_run_status(struct ldc_vdev *wdev, int top_id
	, struct ldc_job *job, struct ldc_task *tsk)
{
	atomic_set(&wdev->core[top_id].state, LDC_CORE_STATE_RUNNING);

	atomic_set(&tsk->state, LDC_TASK_STATE_RUNNING);

	ldc_proc_record_hw_tsk_start(job, tsk, top_id);
}

static void ldc_submit_hw(struct ldc_vdev *wdev, int top_id
	, struct ldc_job *job, struct ldc_task *tsk)
{
	struct ldc_cfg cfg;
	video_frame_s *in_frame, *out_frame;
	pixel_format_e pix_format;
	rotation_e rotation;
	unsigned long long mesh_addr;
	unsigned char num_of_plane, extend_haddr, i;
	unsigned long flags;
	unsigned int bg_color;

	ldc_set_tsk_run_status(wdev, top_id, job, tsk);

	bg_color = tsk->attr.private_data[3];
	mesh_addr = (tsk->attr.private_data[0] != DEFAULT_MESH_PADDR)
		     ? tsk->attr.private_data[0] : 0;
	in_frame = &tsk->attr.img_in.video_frame;
	out_frame = &tsk->attr.img_out.video_frame;
	pix_format = in_frame->pixel_format;
	rotation = tsk->rotation;

	memset(&cfg, 0, sizeof(cfg));

	if (!job->devs_type)	{
		switch (out_frame->pixel_format) {
		case PIXEL_FORMAT_YUV_400:
			cfg.pix_fmt = YUV400;
			num_of_plane = 1;
			break;
		case PIXEL_FORMAT_NV12:
		case PIXEL_FORMAT_NV21:
		default:
			cfg.pix_fmt = NV21;
			num_of_plane = 2;
			break;
		};

		switch (rotation) {
		case ROTATION_90:
			cfg.dst_mode = LDC_DST_ROT_270;
			break;
		case ROTATION_270:
			cfg.dst_mode = LDC_DST_ROT_90;
			break;
		case ROTATION_XY_FLIP:
			cfg.dst_mode = LDC_DST_XY_FLIP;
			break;
		default:
			cfg.dst_mode = LDC_DST_FLAT;
			break;
		}

		cfg.map_base = mesh_addr;
		cfg.bgcolor = LDC_YUV_BLACK /*ctx->bgcolor*/;
		cfg.src_width = ALIGN(in_frame->width, 64);
		cfg.src_height = ALIGN(in_frame->height, 64);
		cfg.ras_width = cfg.src_width;
		cfg.ras_height = cfg.src_height;

		if (cfg.map_base == 0)
			cfg.map_bypass = true;
		else
			cfg.map_bypass = false;

		cfg.src_xstart = 0;
		cfg.src_xend = in_frame->width - 1;

		extend_haddr = in_frame->phyaddr[0] >> 33;
		cfg.extend_haddr = ((extend_haddr << 28) | (extend_haddr << 13) | extend_haddr);

		cfg.src_y_base = in_frame->phyaddr[0];
		cfg.dst_y_base = out_frame->phyaddr[0];
		if (num_of_plane == 2) {
			cfg.src_c_base = in_frame->phyaddr[1];
			cfg.dst_c_base = out_frame->phyaddr[1];
		}
	} else {
		switch (out_frame->pixel_format) {
		case PIXEL_FORMAT_YUV_PLANAR_420:
			cfg.pix_fmt = YUV420p;
			num_of_plane = 3;
		break;
		case PIXEL_FORMAT_YUV_400:
			cfg.pix_fmt = 2;
			num_of_plane = 1;
		break;
		case PIXEL_FORMAT_RGB_888_PLANAR:
		case PIXEL_FORMAT_YUV_PLANAR_444:
		default:
			cfg.pix_fmt = RGB888p;
			num_of_plane = 3;
		break;
		};

		cfg.mesh_id = mesh_addr;
		cfg.map_base = mesh_addr;
		cfg.output_target = 1;//dram or sclr
		cfg.bgcolor = bg_color;;
		cfg.bdcolor = (u32)BGCOLOR_GREEN;
		//cfg.bgcolor_dwa = tsk->bgcolor;
		//cfg.bdcolor_dwa = tsk->bdcolor;
		cfg.src_width  = in_frame->width;
		cfg.src_height = in_frame->height;
		cfg.dst_width  = out_frame->width;
		cfg.dst_height = out_frame->height;

		for (i = 0; i < num_of_plane; ++i) {
			unsigned long long addr = in_frame->phyaddr[i];

			cfg.src_buf[i].addrl    = addr;
			cfg.src_buf[i].addrh    = addr >> 32;
			cfg.src_buf[i].pitch    = in_frame->stride[i];
			cfg.src_buf[i].offset_x = cfg.src_buf[i].offset_y = 0;

			addr = out_frame->phyaddr[i];

			cfg.dst_buf[i].addrl    = addr;
			cfg.dst_buf[i].addrh    = addr >> 32;
			cfg.dst_buf[i].pitch    = out_frame->stride[i];
			cfg.dst_buf[i].offset_x = cfg.src_buf[i].offset_y = 0;
		}
	}

	spin_lock_irqsave(&job->lock, flags);
	job->coreid = top_id;
	spin_unlock_irqrestore(&job->lock, flags);

	spin_lock_irqsave(&wdev->core[top_id].core_lock, flags);
	list_add_tail(&job->node, &wdev->core[top_id].list);
	spin_unlock_irqrestore(&wdev->core[top_id].core_lock, flags);

	spin_lock_irqsave(&wdev->wdev_lock, flags);
	wdev->job_cnt--;
	spin_unlock_irqrestore(&wdev->wdev_lock, flags);

	TRACE_LDC(DBG_INFO, "job[%px]\n", job);
	TRACE_LDC(DBG_INFO, "core_id:%d\n", top_id);
	TRACE_LDC(DBG_DEBUG, "update size src(%d %d)\n", cfg.src_width, cfg.src_height);
	TRACE_LDC(DBG_DEBUG, "update src-buf: %#llx-%#llx-%#llx\n",
		in_frame->phyaddr[0], in_frame->phyaddr[1], in_frame->phyaddr[2]);
	TRACE_LDC(DBG_DEBUG, "update dst-buf: %#llx-%#llx-%#llx\n",
		out_frame->phyaddr[0], out_frame->phyaddr[1], out_frame->phyaddr[2]);
	TRACE_LDC(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg.mesh_id);
	TRACE_LDC(DBG_DEBUG, "update map_base(%#llx)\n", cfg.map_base);
	TRACE_LDC(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n", cfg.bgcolor, cfg.pix_fmt);

	ldc_reset(top_id);
	ldc_init(top_id);
	if (top_id >= DEV_DWA_0)
		ldc_intr_ctrl(DWA_INTR_EN_ALL, top_id);
	else
		ldc_intr_ctrl(LDC_INTR_EN_ALL, top_id);
	ldc_engine(&cfg, top_id);
#if 0
	ldc_dump_register(top_id);
#endif
}

static void ldc_submit_hw_cmdq(struct ldc_vdev *wdev, int top_id
	, struct ldc_job *job, struct ldc_task *last_tsk, struct ldc_task **tskq)
{
	struct ldc_cfg *cfg_q[LDC_JOB_MAX_TSK_NUM] = {NULL};
	union cmdq_set *cmdq_addr = NULL;
	video_frame_s *in_frame, *out_frame;
	pixel_format_e pix_format;
	rotation_e rotation;
	unsigned long long mesh_addr;
	unsigned char num_of_plane, i, tsk_idx, extend_haddr;
	int tsk_num, cmdq_wq_ret;
	unsigned long flags;
	unsigned int bg_color;

	if (unlikely(!wdev || !job || !last_tsk || !tskq))
		return;

	tsk_num = atomic_read(&job->task_num);
	if (unlikely(last_tsk != tskq[tsk_num - 1])) {
		TRACE_LDC(DBG_ERR, "invalid last_tsk, not match with tskq\n");
		return;
	}

	cmdq_addr = kzalloc(sizeof(*cmdq_addr) * tsk_num * LDC_CMDQ_MAX_REG_CNT, GFP_ATOMIC);

	spin_lock_irqsave(&job->lock, flags);
	job->coreid = top_id;
	spin_unlock_irqrestore(&job->lock, flags);

	spin_lock_irqsave(&wdev->core[top_id].core_lock, flags);
	list_add_tail(&job->node, &wdev->core[top_id].list);
	wdev->core[top_id].cmdq_evt = false;
	spin_unlock_irqrestore(&wdev->core[top_id].core_lock, flags);

	spin_lock_irqsave(&wdev->wdev_lock, flags);
	wdev->job_cnt--;
	spin_unlock_irqrestore(&wdev->wdev_lock, flags);

	for (tsk_idx = 0; tsk_idx < tsk_num; tsk_idx++) {

		ldc_set_tsk_run_status(wdev, top_id, job, tskq[tsk_idx]);

		cfg_q[tsk_idx] = kzalloc(sizeof(struct ldc_cfg), GFP_ATOMIC);

		bg_color = tskq[tsk_idx]->attr.private_data[3];
		mesh_addr = (tskq[tsk_idx]->attr.private_data[0] != DEFAULT_MESH_PADDR)
			     ? tskq[tsk_idx]->attr.private_data[0] : 0;
		in_frame = &tskq[tsk_idx]->attr.img_in.video_frame;
		out_frame = &tskq[tsk_idx]->attr.img_out.video_frame;
		pix_format = in_frame->pixel_format;
		rotation = tskq[tsk_idx]->rotation;

		if (!job->devs_type) {
			switch (out_frame->pixel_format) {
			case PIXEL_FORMAT_YUV_400:
				cfg_q[tsk_idx]->pix_fmt = YUV400;
				num_of_plane = 1;
				break;
			case PIXEL_FORMAT_NV12:
			case PIXEL_FORMAT_NV21:
			default:
				cfg_q[tsk_idx]->pix_fmt = NV21;
				num_of_plane = 2;
				break;
			};

			switch (rotation) {
			case ROTATION_90:
				cfg_q[tsk_idx]->dst_mode = LDC_DST_ROT_270;
				break;
			case ROTATION_270:
				cfg_q[tsk_idx]->dst_mode = LDC_DST_ROT_90;
				break;
			case ROTATION_XY_FLIP:
				cfg_q[tsk_idx]->dst_mode = LDC_DST_XY_FLIP;
				break;
			default:
				cfg_q[tsk_idx]->dst_mode = LDC_DST_FLAT;
				break;
			}

			cfg_q[tsk_idx]->map_base = mesh_addr;
			cfg_q[tsk_idx]->bgcolor = LDC_YUV_BLACK /*ctx->bgcolor*/;
			cfg_q[tsk_idx]->src_width = ALIGN(in_frame->width, LDC_SIZE_ALIGN);
			cfg_q[tsk_idx]->src_height = ALIGN(in_frame->height, LDC_SIZE_ALIGN);
			cfg_q[tsk_idx]->ras_width = cfg_q[tsk_idx]->src_width;
			cfg_q[tsk_idx]->ras_height = cfg_q[tsk_idx]->src_height;

			if (cfg_q[tsk_idx]->map_base == 0)
				cfg_q[tsk_idx]->map_bypass = true;
			else
				cfg_q[tsk_idx]->map_bypass = false;

			cfg_q[tsk_idx]->src_xstart = 0;
			cfg_q[tsk_idx]->src_xend = in_frame->width - 1;

			extend_haddr = in_frame->phyaddr[0] >> 33;
			cfg_q[tsk_idx]->extend_haddr = ((extend_haddr << 28) | (extend_haddr << 13) | extend_haddr);

			cfg_q[tsk_idx]->src_y_base = in_frame->phyaddr[0];
			cfg_q[tsk_idx]->dst_y_base = out_frame->phyaddr[0];
			if (num_of_plane == 2) {
				cfg_q[tsk_idx]->src_c_base = in_frame->phyaddr[1];
				cfg_q[tsk_idx]->dst_c_base = out_frame->phyaddr[1];
			}
		} else {
			switch (out_frame->pixel_format) {
			case PIXEL_FORMAT_YUV_PLANAR_420:
				cfg_q[tsk_idx]->pix_fmt = YUV420p;
				num_of_plane = 3;
			break;
			case PIXEL_FORMAT_YUV_400:
				cfg_q[tsk_idx]->pix_fmt = 2;
				num_of_plane = 1;
			break;
			case PIXEL_FORMAT_RGB_888_PLANAR:
			case PIXEL_FORMAT_YUV_PLANAR_444:
			default:
				cfg_q[tsk_idx]->pix_fmt = RGB888p;
				num_of_plane = 3;
			break;
			};

			cfg_q[tsk_idx]->mesh_id = mesh_addr;
			cfg_q[tsk_idx]->output_target = 1;//dram or sclr
			cfg_q[tsk_idx]->bgcolor = bg_color;
			cfg_q[tsk_idx]->bdcolor = (u32)BGCOLOR_GREEN;
			//cfg_q[tsk_idx]->bgcolor = tsk->bgcolor;
			//cfg_q[tsk_idx]->bdcolor = tsk->bdcolor;
			cfg_q[tsk_idx]->src_width  = in_frame->width;
			cfg_q[tsk_idx]->src_height = in_frame->height;
			cfg_q[tsk_idx]->dst_width  = out_frame->width;
			cfg_q[tsk_idx]->dst_height = out_frame->height;

			for (i = 0; i < num_of_plane; ++i) {
				unsigned long long addr = in_frame->phyaddr[i];

				cfg_q[tsk_idx]->src_buf[i].addrl    = addr;
				cfg_q[tsk_idx]->src_buf[i].addrh    = addr >> 32;
				cfg_q[tsk_idx]->src_buf[i].pitch    = in_frame->stride[i];
				cfg_q[tsk_idx]->src_buf[i].offset_x = cfg_q[tsk_idx]->src_buf[i].offset_y = 0;

				addr = out_frame->phyaddr[i];;

				cfg_q[tsk_idx]->dst_buf[i].addrl    = addr;
				cfg_q[tsk_idx]->dst_buf[i].addrh    = addr >> 32;
				cfg_q[tsk_idx]->dst_buf[i].pitch    = out_frame->stride[i];
				cfg_q[tsk_idx]->dst_buf[i].offset_x = cfg_q[tsk_idx]->src_buf[i].offset_y = 0;
			}
		}

#if !USE_REAL_CMDQ //fake cmdq
		ldc_reset(top_id);
		ldc_init(top_id);
		if (top_id >= DEV_DWA_0)
			ldc_intr_ctrl(DWA_INTR_EN_ALL, top_id);
		else
			ldc_intr_ctrl(LDC_INTR_EN_ALL, top_id);
		ldc_engine(cfg_q[tsk_idx], top_id);

		if (tsk_idx != (tsk_num - 1)) {
			cmdq_wq_ret = wait_event_interruptible_timeout(wdev->core[top_id].cmdq_wq
				, wdev->core[top_id].cmdq_evt, LDC_EOF_WAIT_TIMEOUT_MS);
			if (cmdq_wq_ret <= 0) {
				TRACE_LDC(DBG_WARN, "ldc cdmq wait timeout, ret(%d)\n", cmdq_wq_ret);
				goto FREE_CMDQ_RES;
			}
			TRACE_LDC(DBG_DEBUG, "ldc cdmq wait done, ret(%d)\n", cmdq_wq_ret);
		}
#endif
	}

#if USE_REAL_CMDQ
	(void)cmdq_wq_ret;
	ldc_reset(top_id);
	ldc_init(top_id);
	if (top_id >= DEV_DWA_0)
		ldc_intr_ctrl(DWA_INTR_EN_ALL, top_id);
	else
		ldc_intr_ctrl(LDC_INTR_EN_ALL, top_id);
	ldc_engine_cmdq(top_id, (void *)cmdq_addr, cfg_q, tsk_num);
	goto FREE_CMDQ_RES;
#endif

FREE_CMDQ_RES:
	TRACE_LDC(DBG_DEBUG, "cmd_start(%px)\n",cmdq_addr);
	for (i = 0; i < tsk_idx; i++) {
		TRACE_LDC(DBG_DEBUG, "tsk_id[%d]-core_id[%d]\n", i,  top_id);
		if (top_id >= DEV_DWA_0) {
			TRACE_LDC(DBG_DEBUG, "update size src(%d %d) dst(%d %d)\n",
				cfg_q[i]->src_width, cfg_q[i]->src_height
				, cfg_q[i]->dst_width, cfg_q[i]->dst_height);
			TRACE_LDC(DBG_DEBUG, "update src-buf: %#llx-%#llx-%#llx\n"
				, tskq[i]->attr.img_in.video_frame.phyaddr[0]
				, tskq[i]->attr.img_in.video_frame.phyaddr[1]
				, tskq[i]->attr.img_in.video_frame.phyaddr[2]);
			TRACE_LDC(DBG_DEBUG, "update dst-buf: %#llx-%#llx-%#llx\n"
				, tskq[i]->attr.img_out.video_frame.phyaddr[0]
				, tskq[i]->attr.img_out.video_frame.phyaddr[1]
				, tskq[i]->attr.img_out.video_frame.phyaddr[2]);
			TRACE_LDC(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg_q[i]->mesh_id);
			TRACE_LDC(DBG_DEBUG, "update output_target(%d)\n", cfg_q[i]->output_target);
			TRACE_LDC(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n"
				, cfg_q[i]->bgcolor, cfg_q[i]->pix_fmt);
			TRACE_LDC(DBG_DEBUG, "update src pitch(%d %d %d)\n"
				, cfg_q[i]->src_buf[0].pitch, cfg_q[i]->src_buf[1].pitch, cfg_q[i]->src_buf[2].pitch);
			TRACE_LDC(DBG_DEBUG, "update dst pitch(%d %d %d)\n"
				, cfg_q[i]->dst_buf[0].pitch, cfg_q[i]->dst_buf[1].pitch, cfg_q[i]->dst_buf[2].pitch);
		} else {
			TRACE_LDC(DBG_DEBUG, "update size src(%d %d)\n", cfg_q[i]->src_width, cfg_q[i]->src_height);
			TRACE_LDC(DBG_DEBUG, "update src-buf: %#llx-%#llx\n",
				cfg_q[i]->src_y_base, cfg_q[i]->src_c_base);
			TRACE_LDC(DBG_DEBUG, "update dst-buf: %#llx-%#llx\n",
				cfg_q[i]->dst_y_base, cfg_q[i]->dst_c_base);
			TRACE_LDC(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg_q[i]->map_base);
			TRACE_LDC(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n"
				, cfg_q[i]->bgcolor, cfg_q[i]->pix_fmt);
		}

		if (cfg_q[i])
			kfree(cfg_q[i]);
	}

if (cmdq_addr)
	kfree(cmdq_addr);
}

static int ldc_try_submit_hw(struct ldc_vdev *wdev, struct ldc_job *job
	, struct ldc_task *tsk, unsigned char use_cmdq, struct ldc_task **tskq)
{
	int coreid, top_id, ret = -1;
	enum ldc_core_state state;
	unsigned char core_num;

	if (!job->devs_type) {
		coreid = DEV_LDC_0;
		core_num = wdev->core_num - 2;
	} else {
		coreid = DEV_DWA_0;
		core_num = wdev->core_num;
	}

	for (; coreid < core_num; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);

		if ((state == LDC_CORE_STATE_IDLE)) {
			top_id = coreid;
			//ldc_enable_dev_clk(top_id, true);
			if (use_cmdq)
				ldc_submit_hw_cmdq(wdev, top_id, job, tsk, tskq);
			else
				ldc_submit_hw(wdev, top_id, job, tsk);
			ret = 0;
			break;
		}
	}

	if (ret)
		TRACE_LDC(DBG_NOTICE, "ldc_submit_hw fail,hw busy\n");

	return ret;
}

static int ldc_try_submit_hw_cmdq(struct ldc_vdev *wdev, struct ldc_job *job
	, struct ldc_task **tskq, struct ldc_task *last_tsk)
{
	return ldc_try_submit_hw(wdev, job, last_tsk, true, tskq);
}

static unsigned char ldc_is_tsk_ready(struct ldc_task *tsk)
{
	if (unlikely(!tsk)) {
		TRACE_LDC(DBG_ERR, "cur tsk is null\n");
		return false;
	}

	if (unlikely(tsk->tsk_id < 0 || tsk->tsk_id >= LDC_JOB_MAX_TSK_NUM)) {
		TRACE_LDC(DBG_ERR, "cur tsk id(%d) invalid\n", tsk->tsk_id);
		return false;
	}

	return true;
}

/*
static unsigned char ldc_get_idle_coreid(struct ldc_vdev *wdev, enum ldc_job_devs_type devs_type)
{
	unsigned char coreid;
	unsigned char core_num;
	enum ldc_core_state state;

	if (devs_type == DEVS_LDC) {
		coreid = DEV_LDC_0;
		core_num = wdev->core_num - 2;
	} else {
		coreid = DEV_DWA_0;
		core_num = wdev->core_num;
	}

	for (; coreid < core_num; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);
		if (state == LDC_CORE_STATE_IDLE)
			return coreid;
	}
	return wdev->core_num;
}
*/

static bool ldc_have_idle_core(struct ldc_vdev *wdev, enum ldc_job_devs_type devs_type)
{
	unsigned char coreid;
	unsigned char core_num;
	enum ldc_core_state state;

	if (devs_type == DEVS_LDC) {
		coreid = DEV_LDC_0;
		core_num = wdev->core_num - 2;
	} else {
		coreid = DEV_DWA_0;
		core_num = wdev->core_num;
	}

	for (; coreid < core_num; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);
		if (state == LDC_CORE_STATE_IDLE)
			return true;
	}
	return false;
}

static bool ldc_vdev_idle(struct ldc_vdev *wdev)
{
	unsigned char coreid;
	enum ldc_core_state state;

	for (coreid = 0; coreid < LDC_DEV_MAX_CNT; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);
		if (state != LDC_CORE_STATE_IDLE)
			return false;
	}

	return true;
}

static void ldc_set_vdev_stt(struct ldc_vdev *wdev)
{
	if (ldc_vdev_idle(wdev)) {
		atomic_set(&wdev->state, LDC_DEV_STATE_STOP);
		ldc_clk_deinit(ldc_get_dev());
		up(&wdev->sem);
	}
}

static void ldc_try_commit_job(struct ldc_vdev *wdev, struct ldc_job *job)
{
	unsigned long flags;
	unsigned char use_cmdq = false, is_ready = true;
	unsigned char i = 0, tsk_num;
	struct ldc_task *tsk = NULL, *tmp_tsk = NULL, *tskq[LDC_JOB_MAX_TSK_NUM], *last_tsk = NULL;

	if (!wdev || !job)
		return;

	if (atomic_read(&job->job_state) != LDC_JOB_WORKING)
		return;

	tsk_num = atomic_read(&job->task_num);

	if (tsk_num == 1) {
		use_cmdq = false;
		spin_lock_irqsave(&job->lock, flags);
		tsk = list_first_entry_or_null(&job->task_list, struct ldc_task, node);
		spin_unlock_irqrestore(&job->lock, flags);
		is_ready = ldc_is_tsk_ready(tsk);
	} else {
		use_cmdq = true;
		i = 0;
		spin_lock_irqsave(&job->lock, flags);
		list_for_each_entry_safe(tskq[i], tmp_tsk, &job->task_list, node) {
			is_ready &= ldc_is_tsk_ready(tskq[i]);
			last_tsk = tskq[i];
			i++;
		}
		spin_unlock_irqrestore(&job->lock, flags);
	}

	if (!is_ready) {
		TRACE_LDC(DBG_ERR, "cur job(%px) cur tsk(%px) is not ready\n", job, tsk);
		return;
	}

	job->use_cmdq = use_cmdq;
	if (use_cmdq) {
		if (unlikely(!last_tsk)) {
			TRACE_LDC(DBG_ERR, "invalid last_tsk,is null\n");
			return;
		}
		if (unlikely(!list_is_last(&last_tsk->node, &job->task_list))) {
			TRACE_LDC(DBG_ERR, "invalid last_tsk,is not last node\n");
			return;
		}
		ldc_try_submit_hw_cmdq(wdev, job, tskq, last_tsk);
	} else {
		ldc_try_submit_hw(wdev, job, tsk, false, NULL);
	}
}

static int ldc_event_handler_th(void *data)
{
	struct ldc_vdev *wdev = (struct ldc_vdev *)data;
	unsigned long flags;
	struct ldc_job *job;
	int ret;
	unsigned long idle_timeout = msecs_to_jiffies(LDC_IDLE_WAIT_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(LDC_EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;

	if (!wdev) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return -1;
	}

	while (!kthread_should_stop()) {
		if (!wdev)
			break;

		ret = wait_event_interruptible_timeout(wdev->wait,
			wdev->evt || kthread_should_stop(), timeout);

		/* -ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		//suspend
		if (is_ldc_suspended())
			continue;

		/* timeout */
		if (!ret) {
			if (list_empty(&wdev->job_list)) {
				ldc_set_vdev_stt(wdev);
				timeout = idle_timeout;
				continue;
			} else
				TRACE_LDC(DBG_NOTICE, "timeout but job list not empty\n");
		}

		if (list_empty(&wdev->job_list)) {
			TRACE_LDC(DBG_DEBUG, "job list empty\n");
			ldc_set_vdev_stt(wdev);
			goto continue_th;
		}

		if (!ldc_have_idle_core(wdev, job->devs_type)) {
			TRACE_LDC(DBG_INFO, "core busy, not have idle core\n");
			goto continue_th;
		}

		spin_lock_irqsave(&wdev->wdev_lock, flags);
		job = list_first_entry_or_null(&wdev->job_list, struct ldc_job, node);
		list_del(&job->node);
		spin_unlock_irqrestore(&wdev->wdev_lock, flags);

		TRACE_LDC(DBG_INFO, "send job[[%px]]\n", job);

		atomic_set(&job->job_state, LDC_JOB_WORKING);
		atomic_set(&wdev->state, LDC_DEV_STATE_RUNNING);

		ldc_proc_record_job_start(job);
		ldc_clk_init(ldc_get_dev());
		ldc_try_commit_job(wdev, job);
continue_th:
		ldc_clr_evt_kth(wdev);

		/* Adjust timeout */
		timeout = list_empty(&wdev->job_list) ? idle_timeout : eof_timeout;
	}

	return 0;
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
int ldc_begin_job(struct ldc_vdev *wdev, struct gdc_handle_data *data)
{
	struct ldc_job *job;
	int ret = 0;

	ret = ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(data);
	if (ret)
		return ret;

	if (is_ldc_suspended()) {
		TRACE_LDC(DBG_ERR, "ldc dev suspend\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	job = kzalloc(sizeof(struct ldc_job), GFP_ATOMIC);
	if (job == NULL) {
		TRACE_LDC(DBG_ERR, "malloc failed.\n");
		return ERR_GDC_NOBUF;
	}

	INIT_LIST_HEAD(&job->task_list);
	spin_lock_init(&job->lock);
	atomic_set(&job->job_state, LDC_JOB_CREAT);
	atomic_set(&job->task_num, 0);
	job->identity.sync_io = true;
	job->devs_type = DEVS_MAX;

	data->handle = (u64)(uintptr_t)job;

	TRACE_LDC(DBG_DEBUG, "job[%px]++\n", job);

	return ret;
}

int ldc_end_job(struct ldc_vdev *wdev, unsigned long long handle)
{
	int ret = 0, sync_io_ret;
	struct ldc_job *job = (struct ldc_job *)(uintptr_t)handle;
	unsigned long flags;
	int tsk_num = 0;
	unsigned long timeout = msecs_to_jiffies(LDC_SYNC_IO_WAIT_TIMEOUT_MS);
	struct ldc_task *tsk, *tmp_tsk;

	ret = ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(job);
	if (ret)
		return ret;

	if (list_empty(&job->task_list)) {
		TRACE_LDC(DBG_DEBUG, "no task in job.\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	if (wdev->job_cnt >= END_JOB_MAX_LEN) {
		TRACE_LDC(DBG_ERR, "job_cnt is full.\n");
		kfree(job);
		return ERR_GDC_BUF_FULL;
	}

	//mutex_lock(&g_io_lock);
	spin_lock_irqsave(&job->lock, flags);

	list_for_each_entry_safe(tsk, tmp_tsk, &job->task_list, node) {
		tsk_num++;
		tsk->tsk_id = tsk_num - 1;
	}

	job->coreid = LDC_INVALID_CORE_ID;
	atomic_set(&job->job_state, LDC_JOB_WAIT);
	atomic_set(&job->task_num, tsk_num);
	spin_unlock_irqrestore(&job->lock, flags);

	ldc_proc_commit_job(job);

	spin_lock_irqsave(&wdev->wdev_lock, flags);
	list_add_tail(&job->node, &wdev->job_list);
	wdev->job_cnt++;
	spin_unlock_irqrestore(&wdev->wdev_lock, flags);

	ldc_notify_wkup_evt_kth(wdev, LDC_EVENT_WKUP);
	//mutex_unlock(&g_io_lock);

	TRACE_LDC(DBG_INFO, "job[%px] name[%s] sync_io=%d\n", job, job->identity.name, job->identity.sync_io);

	if (job->identity.sync_io) {
		spin_lock_irqsave(&job->lock, flags);
		init_waitqueue_head(&job->job_done_wq);
		job->job_done_evt = false;
		spin_unlock_irqrestore(&job->lock, flags);

		sync_io_ret = wait_event_timeout(job->job_done_wq, job->job_done_evt, timeout);
		if (sync_io_ret <= 0) {
			TRACE_LDC(DBG_WARN, "end job[%px] fail,timeout, ret(%d)\n", job, sync_io_ret);
			return -1;
		}

		kfree(job);
	}

	return ret;
}

static int ldc_cancel_wait_job(struct ldc_vdev *wdev, struct ldc_job *job_handle)
{
	struct ldc_job* tmp, *job;
	bool wait_flag = false;
	unsigned long flags;

	spin_lock_irqsave(&wdev->wdev_lock, flags);
	list_for_each_entry_safe(job, tmp, &wdev->job_list, node) {
		if (job == job_handle) {
			wait_flag = true;
			break;
		}
	}
	spin_unlock_irqrestore(&wdev->wdev_lock, flags);

	if (wait_flag) {
		spin_lock_irqsave(&wdev->wdev_lock, flags);
		list_del(&job_handle->node);
		wdev->job_cnt--;
		spin_unlock_irqrestore(&wdev->wdev_lock, flags);

		kfree(job_handle);
		TRACE_LDC(DBG_NOTICE, "cancel wdev job_list job[%px]\n", job_handle);
		return 0;
	}

	return -1;
}

static int ldc_cancel_work_job(struct ldc_vdev *wdev, struct ldc_job *job_handle)
{
	unsigned long flags;
	bool work_flag = false;
	struct ldc_core *core;
	struct ldc_job* tmp, *job;
	int coreid;
	int state;
	int count = 5000;

	for (coreid = 0; coreid < LDC_DEV_MAX_CNT; coreid++) {
		core = &wdev->core[coreid];
		if (list_empty(&core->list))
			continue;

		spin_lock_irqsave(&core->core_lock, flags);
		list_for_each_entry_safe(job, tmp, &core->list, node) {
			if (job == job_handle) {
				work_flag = true;
				break;
			}
		}
		spin_unlock_irqrestore(&core->core_lock, flags);

		if (work_flag)
			break;
	}

	if (work_flag) {
		while (--count > 0) {
			state = atomic_read(&core->state);
			if (state == LDC_CORE_STATE_RUNNING)
				usleep_range(100, 200);
			else
				break;
		}

		if (state == LDC_CORE_STATE_RUNNING) {
			atomic_set(&core->state, LDC_CORE_STATE_IDLE);

			spin_lock_irqsave(&core->core_lock, flags);
			list_del(&job_handle->node);
			spin_unlock_irqrestore(&core->core_lock, flags);

			kfree(job_handle);
			ldc_core_deinit(coreid);
			TRACE_LDC(DBG_NOTICE, "cancel core[%d] workjob[%px]\n", job_handle->coreid, job_handle);
			TRACE_LDC(DBG_NOTICE, "cur core if timeout, need reset\n");
		} else
			TRACE_LDC(DBG_NOTICE, "core[%d] irq have cancel this job[%px] done\n", job_handle->coreid, job_handle);
	}

	return 0;
}

int ldc_cancel_job(struct ldc_vdev *wdev, unsigned long long handle)
{
	int ret = 0;
	struct ldc_job *job_handle = (struct ldc_job *)(uintptr_t)handle;

	ret = ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(job_handle);
	if (ret)
		return ret;

	if (ldc_cancel_wait_job(wdev, job_handle) == 0)
		return ret;

	ret = ldc_cancel_work_job(wdev, job_handle);
	return ret;
}

int ldc_get_work_job(struct ldc_vdev *wdev, struct gdc_handle_data *data)
{
	int ret = 0;
	struct ldc_job *job = NULL;
	unsigned long flags;
	unsigned char coreid;

	ret = ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(data);
	if (ret)
		return ret;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		spin_lock_irqsave(&wdev->core[coreid].core_lock, flags);
		job = list_first_entry_or_null(&wdev->core[coreid].list, struct ldc_job, node);
		spin_unlock_irqrestore(&wdev->core[coreid].core_lock, flags);

		if (job)
			break;
	}

	data->handle = (u64)(uintptr_t)job;

	TRACE_LDC(DBG_DEBUG, "job[%px]\n", job);

	return ret;
}

int ldc_set_identity(struct ldc_vdev *wdev,
			  struct gdc_identity_attr *identity)
{
	struct ldc_job *job;
	unsigned long flags;
	unsigned long long handle;

	if( ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(identity)) {
		TRACE_LDC(DBG_ERR, "null dev or identity_attr\n");
		return -1;
	}
	handle = identity->handle;

	job = (struct ldc_job *)(uintptr_t)handle;
	if (!job) {
		TRACE_LDC(DBG_ERR, "null job handle\n");
		return -1;
	}

	spin_lock_irqsave(&job->lock, flags);
	memcpy(&job->identity, &identity->attr, sizeof(identity->attr));
	spin_unlock_irqrestore(&job->lock, flags);

	TRACE_LDC(DBG_DEBUG, "sync_io:%d, name:%s, id:%d\n"
		, job->identity.sync_io, job->identity.name, job->identity.id);
	return 0;
}

static inline int ldc_cor_check_param(struct gdc_task_attr *ptask)
{
	const fisheye_attr_s *pfisheye_attr = &ptask->fisheye_attr;
	int ret = 0;
	int i;

	if (pfisheye_attr->enable) {
		if(!pfisheye_attr->grid_info_attr.enable) {
			if (pfisheye_attr->region_num == 0) {
				TRACE_LDC(DBG_ERR, "RegionNum(%d) can't be 0 if enable fisheye.\n",
						pfisheye_attr->region_num);
				//gdc_proc_ctx->fisheye_status.add_task_fail++;
				return ERR_GDC_ILLEGAL_PARAM;
			}
			if (((u32)pfisheye_attr->hor_offset > ptask->img_in.video_frame.width) ||
				((u32)pfisheye_attr->mount_mode > ptask->img_in.video_frame.height)) {
				TRACE_LDC(DBG_ERR, "center pos(%d %d) out of frame size(%d %d).\n",
						pfisheye_attr->hor_offset, pfisheye_attr->mount_mode,
						ptask->img_in.video_frame.width, ptask->img_in.video_frame.height);
				//gdc_proc_ctx->fisheye_status.add_task_fail++;
				return ERR_GDC_ILLEGAL_PARAM;
			}
			for (i = 0; i < pfisheye_attr->region_num; ++i) {
				if ((pfisheye_attr->mount_mode == FISHEYE_WALL_MOUNT) &&
					(pfisheye_attr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_360_PANORAMA)) {
					TRACE_LDC(DBG_ERR, "Rgn(%d): WALL_MOUNT not support Panorama_360.\n", i);
					//gdc_proc_ctx->fisheye_status.add_task_fail++;
					return ERR_GDC_ILLEGAL_PARAM;
				}
				if ((pfisheye_attr->mount_mode == FISHEYE_CEILING_MOUNT) &&
					(pfisheye_attr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_180_PANORAMA)) {
					TRACE_LDC(DBG_ERR, "Rgn(%d): CEILING_MOUNT not support Panorama_180.\n", i);
					//gdc_proc_ctx->fisheye_status.add_task_fail++;
					return ERR_GDC_ILLEGAL_PARAM;
				}
				if ((pfisheye_attr->mount_mode == FISHEYE_DESKTOP_MOUNT) &&
					(pfisheye_attr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_180_PANORAMA)) {
					TRACE_LDC(DBG_ERR, "Rgn(%d): DESKTOP_MOUNT not support Panorama_180.\n", i);
					//gdc_proc_ctx->fisheye_status.add_task_fail++;
					return ERR_GDC_ILLEGAL_PARAM;
				}
			}
		}
	}

	return ret;
}

static inline int ldc_affine_check_param(struct gdc_task_attr *ptask)
{
	const affine_attr_s *paffine_attr = &ptask->affine_attr;
	int ret = 0;

	if (paffine_attr->region_num == 0) {
		TRACE_LDC(DBG_ERR, "region_num(%d) can't be zero.\n", paffine_attr->region_num);
		return ERR_GDC_ILLEGAL_PARAM;
	}

	if (paffine_attr->dest_size.width > ptask->img_out.video_frame.width) {
		TRACE_LDC(DBG_ERR, "dest's width(%d) can't be larger than frame's width(%d)\n",
			paffine_attr->dest_size.width, ptask->img_out.video_frame.width);
		return ERR_GDC_ILLEGAL_PARAM;
	}
#if 0 //not float type(cflag:-mgeneral-regs-only)
	for (i = 0; i < paffine_attr->region_num; ++i) {
		TRACE_LDC(DBG_INFO, "region_num(%d) (%f, %f) (%f, %f) (%f, %f) (%f, %f)\n", i,
			paffine_attr->astRegionAttr[i][0].x, paffine_attr->astRegionAttr[i][0].y,
			paffine_attr->astRegionAttr[i][1].x, paffine_attr->astRegionAttr[i][1].y,
			paffine_attr->astRegionAttr[i][2].x, paffine_attr->astRegionAttr[i][2].y,
			paffine_attr->astRegionAttr[i][3].x, paffine_attr->astRegionAttr[i][3].y);
		if ((paffine_attr->astRegionAttr[i][0].x < 0) || (paffine_attr->astRegionAttr[i][0].y < 0) ||
			(paffine_attr->astRegionAttr[i][1].x < 0) || (paffine_attr->astRegionAttr[i][1].y < 0) ||
			(paffine_attr->astRegionAttr[i][2].x < 0) || (paffine_attr->astRegionAttr[i][2].y < 0) ||
			(paffine_attr->astRegionAttr[i][3].x < 0) || (paffine_attr->astRegionAttr[i][3].y < 0)) {
			TRACE_LDC(DBG_ERR, "region_num(%d) affine point can't be negative\n", i);
			return ERR_GDC_ILLEGAL_PARAM;
		}
		if ((paffine_attr->astRegionAttr[i][1].x < paffine_attr->astRegionAttr[i][0].x) ||
			(paffine_attr->astRegionAttr[i][3].x < paffine_attr->astRegionAttr[i][2].x)) {
			TRACE_LDC(DBG_ERR, "region_num(%d) point1/3's x should be bigger thant 0/2's\n", i);
			return ERR_GDC_ILLEGAL_PARAM;
		}
		if ((paffine_attr->astRegionAttr[i][2].y < paffine_attr->astRegionAttr[i][0].y) ||
			(paffine_attr->astRegionAttr[i][3].y < paffine_attr->astRegionAttr[i][1].y)) {
			TRACE_LDC(DBG_ERR, "region_num(%d) point2/3's y should be bigger thant 0/1's\n", i);
			return ERR_GDC_ILLEGAL_PARAM;
		}
	}
#endif
	return ret;
}

int ldc_add_rotation_task(struct ldc_vdev *wdev,
			  struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

	ret = ldc_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;

	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);

	if (job->devs_type == DEVS_DWA) {
		TRACE_LDC(DBG_ERR, "A job is not allowed to use different types of hardware\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	job->devs_type = DEVS_LDC;
	TRACE_LDC(DBG_INFO, "job[%px], job->devs_type is %d\n", job, job->devs_type);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_ROT;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);

	spin_lock_irqsave(&job->lock, flags);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&job->lock, flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_add_ldc_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

#if 0
	ret = ldc_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;
#endif

	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);

	if (job->devs_type == DEVS_LDC) {
		TRACE_LDC(DBG_ERR, "A job is not allowed to use different types of hardware\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	job->devs_type = DEVS_DWA;
	TRACE_LDC(DBG_INFO, "job[%px], job->devs_type is %d\n", job, job->devs_type);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_LDC;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);

	spin_lock_irqsave(&job->lock, flags);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&job->lock, flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_add_cor_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(wdev, attr))
		return -1;

	handle = attr->handle;

	ret = ldc_cor_check_param(attr);
	if (ret)
		return ret;

	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);

	if (job->devs_type == DEVS_LDC) {
		TRACE_LDC(DBG_ERR, "A job is not allowed to use different types of hardware\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	job->devs_type = DEVS_DWA;
	TRACE_LDC(DBG_INFO, "job[%px], job->devs_type is %d\n", job, job->devs_type);


	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_FISHEYE;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);

	spin_lock_irqsave(&job->lock, flags);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&job->lock, flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_add_warp_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(wdev, attr))
		return -1;

	handle = attr->handle;

	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);

	if (job->devs_type == DEVS_LDC) {
		TRACE_LDC(DBG_ERR, "A job is not allowed to use different types of hardware\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	job->devs_type = DEVS_DWA;
	TRACE_LDC(DBG_INFO, "job[%px], job->devs_type is %d\n", job, job->devs_type);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_WARP;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);

	spin_lock_irqsave(&job->lock, flags);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&job->lock, flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_add_affine_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

	ret = ldc_affine_check_param(attr);
	if (ret)
		return ret;

	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);

	if (job->devs_type == DEVS_LDC) {
		TRACE_LDC(DBG_ERR, "A job is not allowed to use different types of hardware\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	job->devs_type = DEVS_DWA;
	TRACE_LDC(DBG_INFO, "job[%px], job->devs_type is %d\n", job, job->devs_type);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_AFFINE;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);

	spin_lock_irqsave(&job->lock, flags);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&job->lock, flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_add_ldc_ldc_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

#if 0
	ret = ldc_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;
#endif

	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);

	if (job->devs_type == DEVS_DWA) {
		TRACE_LDC(DBG_ERR, "A job is not allowed to use different types of hardware\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	job->devs_type = DEVS_LDC;
	TRACE_LDC(DBG_INFO, "job[%px], job->devs_type is %d\n", job, job->devs_type);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_LDC;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);

	spin_lock_irqsave(&job->lock, flags);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&job->lock, flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_add_dwa_rot_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

	ret = ldc_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;

	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);

	if (job->devs_type == DEVS_LDC) {
		TRACE_LDC(DBG_ERR, "A job is not allowed to use different types of hardware\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	job->devs_type = DEVS_DWA;
	TRACE_LDC(DBG_INFO, "job[%px], job->devs_type is %d\n", job, job->devs_type);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_ROT;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);

	spin_lock_irqsave(&job->lock, flags);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&job->lock, flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_get_chn_frame(struct ldc_vdev *wdev, struct gdc_identity_attr *identity
	, video_frame_info_s *pstvideo_frame, int s32milli_sec)
{
	int ret;
	unsigned long flags;
	struct ldc_vb_done *vb_done = NULL, *vb_done_tmp = NULL;
	bool ismatch = false;

	TRACE_LDC(DBG_DEBUG, "++\n");
	ret = ldc_check_null_ptr(pstvideo_frame)
		|| ldc_check_null_ptr(wdev)
		|| ldc_check_null_ptr(identity);
	if (ret)
		return ret;

	memset(pstvideo_frame, 0, sizeof(*pstvideo_frame));
	if (s32milli_sec <= 0) {
		if (down_trylock(&wdev->vb_doneq.sem)) {
			TRACE_LDC(DBG_ERR, "cannot get sem, doneq not ready\n");
			return ERR_GDC_SYS_NOTREADY;
		}
	} else {
		TRACE_LDC(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
		ret = down_timeout(&wdev->vb_doneq.sem, msecs_to_jiffies(s32milli_sec));
		if (ret == -ETIME) {
			TRACE_LDC(DBG_ERR, "get sem timeout, doneq not ready\n");
			return ret;
		}
		TRACE_LDC(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
	}

	if (list_empty(&wdev->vb_doneq.doneq)) {
		TRACE_LDC(DBG_ERR, "vb_doneq is empty\n");
		return ERR_GDC_NOBUF;
	}

	spin_lock_irqsave(&wdev->vb_doneq.lock, flags);
	list_for_each_entry_safe(vb_done, vb_done_tmp, &wdev->vb_doneq.doneq, node) {
		if (ldc_identity_is_match(&vb_done->job.identity, &identity->attr)) {
			TRACE_LDC(DBG_DEBUG, "vb_doneq identity[%d-%d-%s] is match [%d-%d-%s]\n"
				, vb_done->job.identity.mod_id, vb_done->job.identity.id, vb_done->job.identity.name
				, identity->attr.mod_id, identity->attr.id, identity->attr.name);
			ismatch = true;
			break;
		}
	}
	if (ismatch)
		list_del(&vb_done->node);
	spin_unlock_irqrestore(&wdev->vb_doneq.lock, flags);

	if (!ismatch) {
		TRACE_LDC(DBG_DEBUG, "vb_doneq[%px] identity[%d-%d-%s] not match [%d-%d-%s]\n"
			, vb_done_tmp, vb_done_tmp->job.identity.mod_id, vb_done_tmp->job.identity.id, vb_done_tmp->job.identity.name
			, identity->attr.mod_id, identity->attr.id, identity->attr.name);
		return ERR_GDC_NOBUF;
	}

	memcpy(pstvideo_frame, &vb_done->img_out, sizeof(*pstvideo_frame));
	kfree(vb_done);

	TRACE_LDC(DBG_DEBUG, "end to get pstvideo_frame width:%d height:%d buf:0x%llx\n"
		, pstvideo_frame->video_frame.width
		, pstvideo_frame->video_frame.height
		, pstvideo_frame->video_frame.phyaddr[0]);
	TRACE_LDC(DBG_DEBUG, "--\n");

	return 0;
}

int ldc_attach_vb_pool(vb_pool vb_pool)
{
	unsigned long flags;
	struct ldc_vdev *wdev = ldc_get_dev();

	if (!wdev)
		return -1;

	spin_lock_irqsave(&wdev->wdev_lock, flags);
	wdev->vb_pool = vb_pool;
	spin_unlock_irqrestore(&wdev->wdev_lock, flags);

	TRACE_LDC(DBG_DEBUG, "attach vb pool(%d)\n", vb_pool);
	return 0;
}

int ldc_detach_vb_pool(void)
{
	unsigned long flags;
	struct ldc_vdev *wdev = ldc_get_dev();

	if (!wdev)
		return -1;

	spin_lock_irqsave(&wdev->wdev_lock, flags);
	wdev->vb_pool = VB_INVALID_POOLID;
	spin_unlock_irqrestore(&wdev->wdev_lock, flags);

	TRACE_LDC(DBG_DEBUG, "dettach vb pool\n");
	return 0;
}

/**************************************************************************
 *   internal APIs.
 **************************************************************************/
int ldc_sw_init(struct ldc_vdev *wdev)
{
	struct sched_param tsk;
	int ret = 0;
	unsigned char coreid;

	ret = ldc_check_null_ptr(wdev);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&wdev->vb_doneq.doneq);
	sema_init(&wdev->vb_doneq.sem, 0);
	spin_lock_init(&wdev->vb_doneq.lock);
	sema_init(&wdev->sem, 0);
	init_waitqueue_head(&wdev->wait);
	spin_lock_init(&wdev->wdev_lock);
	INIT_LIST_HEAD(&wdev->job_list);
	wdev->core_num = LDC_DEV_MAX_CNT;
	wdev->evt = LDC_EVENT_BUSY_OR_NOT_STAT;
	wdev->vb_pool = VB_INVALID_POOLID;
	wdev->job_cnt = 0;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
#if LDC_USE_WORKQUEUE
		INIT_WORK(&wdev->core[coreid].work_frm_done, ldc_work_frm_done);
#endif
		INIT_LIST_HEAD(&wdev->core[coreid].list);
#if !USE_REAL_CMDQ
		init_waitqueue_head(&wdev->core[coreid].cmdq_wq);
#endif
		spin_lock_init(&wdev->core[coreid].core_lock);
	}

	wdev->thread = kthread_run(ldc_event_handler_th, (void *)wdev, "ldc_event_handler_th");
	if (IS_ERR(wdev->thread)) {
		TRACE_LDC(DBG_ERR, "failed to create ldc kthread\n");
		return -1;
	}

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;
	ret = sched_setscheduler(wdev->thread, SCHED_FIFO, &tsk);
	if (ret)
		TRACE_LDC(DBG_WARN, "ldc thread priority update failed: %d\n", ret);

	/*wdev->workqueue = create_singlethread_workqueue("ldc_workqueue");
	if (!wdev->workqueue) {
		TRACE_LDC(DBG_ERR, "ldc dev create_workqueue failed.\n");
		return -2;
	}

	INIT_WORK(&wdev->work_frm_done, ldc_work_frm_done);*/

	return ret;
}

void ldc_sw_deinit(struct ldc_vdev *wdev)
{
	unsigned char coreid;
	if (ldc_check_null_ptr(wdev) != 0)
		return;

	/*if (wdev->workqueue)
		destroy_workqueue(wdev->workqueue);*/

#if LDC_USE_WORKQUEUE
	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		flush_work(&wdev->core[coreid].work_frm_done);
		cancel_work_sync(&wdev->core[coreid].work_frm_done);
		list_del_init(&wdev->core[coreid].list);
	}
#endif
	if (!IS_ERR(wdev->thread)) {
		if (kthread_stop(wdev->thread))
			TRACE_LDC(DBG_ERR, "fail to stop ldc kthread\n");
	}

	list_del_init(&wdev->job_list);
	list_del_init(&wdev->vb_doneq.doneq);
}

int ldc_suspend_handler(void)
{
	int ret;
	struct ldc_vdev * dev = ldc_get_dev();
	int job_cnt;
	unsigned long flags;

	if (unlikely(!dev)) {
		TRACE_LDC(DBG_ERR, "ldc_dev is null\n");
		return ERR_GDC_NULL_PTR;
	}

	if (!dev->thread) {
		TRACE_LDC(DBG_ERR, "ldc thread not initialized yet\n");
		return ERR_GDC_SYS_NOTREADY;
	}

	spin_lock_irqsave(&dev->wdev_lock, flags);
	job_cnt = dev->job_cnt;
	spin_unlock_irqrestore(&dev->wdev_lock, flags);

	if (job_cnt > 0 || atomic_read(&dev->state) == LDC_DEV_STATE_RUNNING) {
		sema_init(&dev->sem, 0);
		ret = down_timeout(&dev->sem, msecs_to_jiffies(LDC_IDLE_WAIT_TIMEOUT_MS));
		if (ret == -ETIME) {
			TRACE_LDC(DBG_ERR, "get sem timeout, dev not idle yet\n");
			return ret;
		}
	}
	atomic_set(&dev->state, LDC_DEV_STATE_STOP);

	ldc_proc_update_timer_proc(true);
	TRACE_LDC(DBG_WARN, "suspend handler+\n");
	return 0;
}

int ldc_resume_handler(void)
{
	struct ldc_vdev * dev = ldc_get_dev();

	if (unlikely(!dev)) {
		TRACE_LDC(DBG_ERR, "ldc_dev is null\n");
		return ERR_GDC_NULL_PTR;
	}

	if (!dev->thread) {
		TRACE_LDC(DBG_ERR, "ldc thread not initialized yet\n");
		return ERR_GDC_SYS_NOTREADY;
	}

	atomic_set(&dev->state, LDC_DEV_STATE_RUNNING);

	ldc_proc_update_timer_proc(false);

	TRACE_LDC(DBG_WARN, "resume handler+\n");
	return 0;
}


#include <linux/types.h>
#include <linux/mm.h>
#include <linux/comm_buffer.h>
#include <linux/slab.h>

#include "vbq.h"
#include "vb.h"
#include "bind.h"
#include "base_common.h"
#include "base_debug.h"


static vbq_recv_cb g_base_recv_cb[ID_BUTT];

void base_register_recv_cb(mod_id_e mod_id, vbq_recv_cb cb)
{
	if (mod_id < 0 || mod_id >= ID_BUTT)
		TRACE_BASE(DBG_ERR, "enmodid error.\n");

	g_base_recv_cb[mod_id] = cb;
}
EXPORT_SYMBOL_GPL(base_register_recv_cb);


void base_unregister_recv_cb(mod_id_e mod_id)
{
	if (mod_id < 0 || mod_id >= ID_BUTT)
		TRACE_BASE(DBG_ERR, "mod_id error.\n");

	g_base_recv_cb[mod_id] = NULL;
}
EXPORT_SYMBOL_GPL(base_unregister_recv_cb);


s32 base_fill_videoframe2buffer(mmf_chn_s chn, const video_frame_info_s *frame,
	struct video_buffer *buf)
{
	u32 plane_size;
	vb_cal_config_s vbcalconfig;
	u8 i = 0;
	u32 align = frame->video_frame.align ? frame->video_frame.align : DEFAULT_ALIGN;

	common_getpicbufferconfig(frame->video_frame.width, frame->video_frame.height,
		frame->video_frame.pixel_format, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
		align, &vbcalconfig);

	buf->size.width = frame->video_frame.width;
	buf->size.height = frame->video_frame.height;
	buf->pixel_format = frame->video_frame.pixel_format;
	buf->offset_left = frame->video_frame.offset_left;
	buf->offset_top = frame->video_frame.offset_top;
	buf->offset_right = frame->video_frame.offset_right;
	buf->offset_bottom = frame->video_frame.offset_bottom;
	buf->frm_num = frame->video_frame.time_ref;
	buf->pts = frame->video_frame.pts;
	buf->compress_mode = frame->video_frame.compress_mode;
	buf->compress_expand_addr = frame->video_frame.ext_phy_addr;
	memset(&buf->frame_crop, 0, sizeof(buf->frame_crop));

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (i >= vbcalconfig.plane_num) {
			buf->phy_addr[i] = 0;
			buf->length[i] = 0;
			buf->stride[i] = 0;
			continue;
		}

		plane_size = (i == 0) ? vbcalconfig.main_y_size : vbcalconfig.main_c_size;
		buf->phy_addr[i] = frame->video_frame.phyaddr[i];
		buf->length[i] = frame->video_frame.length[i];
		buf->stride[i] = frame->video_frame.stride[i];
		if (buf->length[i] < plane_size && (frame->video_frame.compress_mode != COMPRESS_MODE_FRAME)) {
			TRACE_BASE(DBG_ERR, "Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id, i);
			TRACE_BASE(DBG_ERR, " length(%zu) less than expected(%d).\n"
				, buf->length[i], plane_size);
			return -1;
		}

		if (buf->stride[i] % align && (frame->video_frame.compress_mode != COMPRESS_MODE_FRAME)) {
			TRACE_BASE(DBG_ERR, "Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id, i);
			TRACE_BASE(DBG_ERR, " stride(%d) not aligned(%d).\n"
				, buf->stride[i], align);
			return -1;
		}
		if (buf->phy_addr[i] % align) {
			TRACE_BASE(DBG_ERR, "Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id, i);
			TRACE_BASE(DBG_ERR, " address(%llx) not aligned(%d).\n"
				, buf->phy_addr[i], align);
			return -1;
		}
	}

	// [WA-01]
	if (vbcalconfig.plane_num > 1) {
		if (((buf->phy_addr[0] & (vbcalconfig.addr_align - 1))
			!= (buf->phy_addr[1] & (vbcalconfig.addr_align - 1)))
		|| ((buf->phy_addr[0] & (vbcalconfig.addr_align - 1))
			!= (buf->phy_addr[2] & (vbcalconfig.addr_align - 1)))) {
			TRACE_BASE(DBG_ERR, "Mod(%s) Dev(%d) Chn(%d)\n"
				, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id);
			TRACE_BASE(DBG_ERR, "plane address offset (%llx-%llx-%llx)"
				, buf->phy_addr[0], buf->phy_addr[1], buf->phy_addr[2]);
			TRACE_BASE(DBG_ERR, "not aligned to %#x.\n", vbcalconfig.addr_align);
			return -1;
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(base_fill_videoframe2buffer);

s32 base_get_chn_buffer(mmf_chn_s chn, struct vb_jobs_t *jobs, vb_blk *blk, s32 timeout_ms)
{
	s32 ret = -1;
	struct vb_s *vb;
	struct vbq *doneq;
	struct snap_s *s;

	if (!jobs) {
		TRACE_BASE(DBG_ERR, "mod(%s), jobs Null.\n", sys_get_modname(chn.mod_id));
		return -1;
	}

	if (!jobs->inited) {
		TRACE_BASE(DBG_ERR, "mod(%s) get chn buf fail, not inited yet\n", sys_get_modname(chn.mod_id));
		return -1;
	}
	doneq = &jobs->doneq;

	mutex_lock(&jobs->dlock);
	if (!FIFO_EMPTY(doneq)) {
		FIFO_POP(doneq, &vb);
		atomic_long_fetch_and(~BIT(chn.mod_id), &vb->mod_ids);
		atomic_long_fetch_or(BIT(ID_USER), &vb->mod_ids);
		mutex_unlock(&jobs->dlock);
		*blk = (vb_blk)vb;
		return 0;
	}

	s = kmalloc(sizeof(*s), GFP_ATOMIC);
	if (!s) {
		mutex_unlock(&jobs->dlock);
		return -1;
	}

	init_waitqueue_head(&s->cond_queue);

	s->chn = chn;
	s->blk = VB_INVALID_HANDLE;
	s->avail = 0;

	if (timeout_ms < 0) {
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		wait_event(s->cond_queue, s->avail);
	} else {
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		ret = wait_event_timeout(s->cond_queue, s->avail, msecs_to_jiffies(timeout_ms));
		// ret < 0, error
		// ret = 0, timeout
		// ret = 1, condition true
	}

	if (s->avail)
		ret = 0;
	else
		ret = -1;

	if (!ret) {
		*blk = s->blk;
	} else {
		mutex_lock(&jobs->dlock);
		if (s->blk != VB_INVALID_HANDLE)
			vb_release_block(s->blk);
		TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);

		TRACE_BASE(DBG_WARN, "Mod(%s) Grp(%d) Chn(%d), jobs wait(%d) work(%d) done(%d)\n"
			, sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id
			, FIFO_SIZE(&jobs->waitq), FIFO_SIZE(&jobs->workq), FIFO_SIZE(&jobs->doneq));
	}

	kfree(s);
	return ret;
}
EXPORT_SYMBOL_GPL(base_get_chn_buffer);

/* base_mod_jobs_init: initialize the jobs.
 *
 * @param jobs: vb jobs.
 * @param waitq_depth: the depth for waitq.
 * @param workq_depth: the depth for workq.
 * @param doneq_depth: the depth for doneq.
 */
void base_mod_jobs_init(struct vb_jobs_t *jobs, uint8_t waitq_depth, uint8_t workq_depth, uint8_t doneq_depth)
{
	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "[%p] job init fail, Null parameter\n", __builtin_return_address(0));
		return;
	}

	if (jobs->inited) {
		TRACE_BASE(DBG_ERR, "[%p] job init fail, already inited\n", __builtin_return_address(0));
		return;
	}

	mutex_init(&jobs->lock);
	mutex_init(&jobs->dlock);
	sema_init(&jobs->sem, 0);
	FIFO_INIT(&jobs->waitq, waitq_depth);
	FIFO_INIT(&jobs->workq, workq_depth);
	FIFO_INIT(&jobs->doneq, doneq_depth);
	TAILQ_INIT(&jobs->snap_jobs);
	jobs->inited = true;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_init);

/* base_mod_jobs_clear: clear the jobs.
 *
 * @param jobs: vb jobs.
 */
void base_mod_jobs_clear(struct vb_jobs_t *jobs)
{
	struct vb_s *vb;
	struct snap_s *s, *s_tmp;

	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "[%p] job reinit fail, Null parameter\n", __builtin_return_address(0));
		return;
	}

	if (!(jobs->inited)) {
		TRACE_BASE(DBG_ERR, "[%p] job reinit fail, not inited\n", __builtin_return_address(0));
		return;
	}
	mutex_lock(&jobs->lock);
	while (!FIFO_EMPTY(&jobs->waitq)) {
		FIFO_POP(&jobs->waitq, &vb);
		vb_release_block((vb_blk)vb);
	}

	while (!FIFO_EMPTY(&jobs->workq)) {
		FIFO_POP(&jobs->workq, &vb);
		vb_release_block((vb_blk)vb);
	}
	mutex_unlock(&jobs->lock);

	mutex_lock(&jobs->dlock);
	while (!FIFO_EMPTY(&jobs->doneq)) {
		FIFO_POP(&jobs->doneq, &vb);
		vb_release_block((vb_blk)vb);
	}

	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp)
	TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
	mutex_unlock(&jobs->dlock);
}
EXPORT_SYMBOL_GPL(base_mod_jobs_clear);

/* mod_jobs_exit: end the jobs and release all resources.
 *
 * @param jobs: vb jobs.
 */
void base_mod_jobs_exit(struct vb_jobs_t *jobs)
{
	struct vb_s *vb;
	struct snap_s *s, *s_tmp;

	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "[%p] job exit fail, Null parameter\n", __builtin_return_address(0));
		return;
	}

	if (!jobs->inited) {
		TRACE_BASE(DBG_ERR, "[%p] job exit fail, not inited yet\n", __builtin_return_address(0));
		return;
	}

	mutex_lock(&jobs->lock);
	jobs->inited = false;
	while (!FIFO_EMPTY(&jobs->waitq)) {
		FIFO_POP(&jobs->waitq, &vb);
		vb_release_block((vb_blk)vb);
	}
	FIFO_EXIT(&jobs->waitq);
	while (!FIFO_EMPTY(&jobs->workq)) {
		FIFO_POP(&jobs->workq, &vb);
		vb_release_block((vb_blk)vb);
	}
	FIFO_EXIT(&jobs->workq);
	mutex_unlock(&jobs->lock);
	mutex_destroy(&jobs->lock);

	mutex_lock(&jobs->dlock);
	while (!FIFO_EMPTY(&jobs->doneq)) {
		FIFO_POP(&jobs->doneq, &vb);
		vb_release_block((vb_blk)vb);
	}
	FIFO_EXIT(&jobs->doneq);

	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp)
	TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
	mutex_unlock(&jobs->dlock);
	mutex_destroy(&jobs->dlock);
}
EXPORT_SYMBOL_GPL(base_mod_jobs_exit);

/* mod_jobs_enque_work: Put job into work.
 *     Move vb from waitq into workq and put into driver.
 *
 * @param jobs: vb jobs.
 * @return: 0 if OK.
 */
struct video_buffer *base_mod_jobs_enque_work(struct vb_jobs_t *jobs)
{
	struct vb_s *vb;
	int32_t ret = 0;

	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "[%p] job is NULL.\n", __builtin_return_address(0));
		return NULL;
	}

	mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->waitq)) {
		mutex_unlock(&jobs->lock);
		TRACE_BASE(DBG_ERR, "waitq is empty.\n");
		return NULL;
	}
	if (FIFO_FULL(&jobs->workq)) {
		mutex_unlock(&jobs->lock);
		TRACE_BASE(DBG_ERR, "workq is full.\n");
		return NULL;
	}

	FIFO_POP(&jobs->waitq, &vb);
	FIFO_PUSH(&jobs->workq, vb);
	mutex_unlock(&jobs->lock);

	TRACE_BASE(DBG_DEBUG, "phy-addr(%llx).\n", vb->phy_addr);

	if (ret != 0) {
		TRACE_BASE(DBG_ERR, "qbuf error\n");
		return NULL;
	}
	return &vb->buf;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_enque_work);

/* mod_jobs_waitq_empty: if waitq is empty
 *
 * @param jobs: vb jobs.
 * @return: TRUE if empty.
 */
bool base_mod_jobs_waitq_empty(struct vb_jobs_t *jobs)
{
	bool is_empty;

	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "[%p] job is NULL.\n", __builtin_return_address(0));
		return false;
	}

	mutex_lock(&jobs->lock);
	is_empty = FIFO_EMPTY(&jobs->waitq);
	mutex_unlock(&jobs->lock);

	return is_empty;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_waitq_empty);

/* mod_jobs_workq_empty: if workq is empty
 *
 * @param jobs: vb jobs.
 * @return: TRUE if empty.
 */
bool base_mod_jobs_workq_empty(struct vb_jobs_t *jobs)
{
	bool is_empty;

	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "[%p] job is NULL.\n", __builtin_return_address(0));
		return false;
	}

	mutex_lock(&jobs->lock);
	is_empty = FIFO_EMPTY(&jobs->workq);
	mutex_unlock(&jobs->lock);

	return is_empty;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_workq_empty);

/* mod_jobs_waitq_pop: pop out from waitq.
 *
 * @param jobs: vb jobs.
 * @return: VB_INVALID_HANDLE is not available; o/w, the vb_blk.
 */
vb_blk base_mod_jobs_waitq_pop(struct vb_jobs_t *jobs)
{
	struct vb_s *p;

	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "[%p] job is NULL.\n", __builtin_return_address(0));
		return VB_INVALID_HANDLE;
	}

	mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->waitq)) {
		mutex_unlock(&jobs->lock);
		TRACE_BASE(DBG_ERR, "No more vb in waitq for dequeue.\n");
		return VB_INVALID_HANDLE;
	}
	FIFO_POP(&jobs->waitq, &p);
	mutex_unlock(&jobs->lock);
	return (vb_blk)p;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_waitq_pop);

/* mod_jobs_workq_pop: pop out from workq.
 *
 * @param jobs: vb jobs.
 * @return: VB_INVALID_HANDLE is not available; o/w, the vb_blk.
 */
vb_blk base_mod_jobs_workq_pop(struct vb_jobs_t *jobs)
{
	struct vb_s *p;

	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "Null parameter\n");
		return VB_INVALID_HANDLE;
	}

	mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->workq)) {
		mutex_unlock(&jobs->lock);
		TRACE_BASE(DBG_ERR, "No more vb in workq for dequeue.\n");
		return VB_INVALID_HANDLE;
	}
	FIFO_POP(&jobs->workq, &p);
	mutex_unlock(&jobs->lock);
	return (vb_blk)p;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_workq_pop);

int32_t base_get_frame_info(pixel_format_e fmt, size_s size, struct video_buffer *buf, u64 mem_base, u8 align)
{
	vb_cal_config_s vbcalconfig;
	u8 i = 0;

	common_getpicbufferconfig(size.width, size.height, fmt, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, align, &vbcalconfig);

	memset(buf, 0, sizeof(*buf));
	buf->size = size;
	buf->pixel_format = fmt;
	for (i = 0; i < vbcalconfig.plane_num; ++i) {
		buf->phy_addr[i] = mem_base;
		buf->length[i] = ALIGN((i == 0) ? vbcalconfig.main_y_size : vbcalconfig.main_c_size,
					vbcalconfig.addr_align);
		buf->stride[i] = (i == 0) ? vbcalconfig.main_stride : vbcalconfig.c_stride;
		mem_base += buf->length[i];

		TRACE_BASE(DBG_DEBUG, "(%llx-%zu-%d)\n", buf->phy_addr[i], buf->length[i], buf->stride[i]);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(base_get_frame_info);

/* _handle_snap: if there is get-frame request, hanlde it.
 *
 * @param chn: the channel where the blk is dequeued.
 * @param jobs: vb jobs.
 * @param blk: the vb_blk to handle.
 */
static void _handle_snap(mmf_chn_s chn, struct vb_jobs_t *jobs, vb_blk blk)
{
	struct vb_s *p = (struct vb_s *)blk;
	struct vbq *doneq;
	struct snap_s *s, *s_tmp;

	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "handle snap fail, Null parameter\n");
		return;
	}

	if (!jobs->inited) {
		TRACE_BASE(DBG_ERR, "handle snap fail, job not inited yet\n");
		return;
	}

	mutex_lock(&jobs->dlock);
	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp) {
		if (CHN_MATCH(&s->chn, &chn)) {
			TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
			s->blk = blk;
			atomic_fetch_add(1, &p->usr_cnt);
			atomic_long_fetch_or(BIT(ID_USER), &p->mod_ids);
			s->avail = 1;
			wake_up(&s->cond_queue);
			mutex_unlock(&jobs->dlock);
			return;
		}
	}

	doneq = &jobs->doneq;
	// check if there is a snap-queue
	if (FIFO_CAPACITY(doneq)) {
		if (FIFO_FULL(doneq)) {
			struct vb_s *vb = NULL;

			FIFO_POP(doneq, &vb);
			atomic_long_fetch_and(~BIT(chn.mod_id), &vb->mod_ids);
			vb_release_block((vb_blk)vb);
		}
		atomic_fetch_add(1, &p->usr_cnt);
		atomic_long_fetch_or(BIT(chn.mod_id), &p->mod_ids);
		FIFO_PUSH(doneq, p);
	}
	mutex_unlock(&jobs->dlock);
}

/* vb_qbuf: queue vb into the specified channel.
 *     (src) Put into workq and driver.
 *     (dst) Put into waitq and sem_post
 *
 * @param chn: the channel to be queued.
 * @param chn_type: the chn is input(read) or output(write)
 * @param jobs: vb jobs.
 * @param blk: vb_blk to be queued.
 */
int32_t vb_qbuf(mmf_chn_s chn, enum chn_type_e chn_type, struct vb_jobs_t *jobs, vb_blk blk)
{
	struct vb_s *vb = (struct vb_s *)blk;
	s32 ret = 0;

	TRACE_BASE(DBG_DEBUG, "%s dev(%d) chn(%d) chnType(%d): phy-addr(%lld) cnt(%d)\n",
		     sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id, chn_type,
		     vb->phy_addr, vb->usr_cnt.counter);

	if (!jobs) {
		TRACE_BASE(DBG_ERR, "mod(%s), vb_qbuf fail, error, empty jobs\n", sys_get_modname(chn.mod_id));
		return -1;
	}
	if (!jobs->inited) {
		TRACE_BASE(DBG_ERR, "mod(%s), vb_qbuf fail, jobs not initialized yet\n", sys_get_modname(chn.mod_id));
		return -1;
	}

	mutex_lock(&jobs->lock);
	if (chn_type == CHN_TYPE_OUT) {
		if (FIFO_FULL(&jobs->workq)) {
			mutex_unlock(&jobs->lock);
			TRACE_BASE(DBG_ERR, "%s workq is full. drop new one.\n"
				     , sys_get_modname(chn.mod_id));
			return -ENOBUFS;
		}
		vb->buf.dev_num = chn.chn_id;
		FIFO_PUSH(&jobs->workq, vb);
	} else {
		if (FIFO_FULL(&jobs->waitq)) {
			mutex_unlock(&jobs->lock);
			TRACE_BASE(DBG_ERR, "%s dev(%d) chn(%d) waitq is full. drop new one.\n"
				     , sys_get_modname(chn.mod_id), chn.dev_id, chn.chn_id);
			return -ENOBUFS;
		}
		FIFO_PUSH(&jobs->waitq, vb);
		up(&jobs->sem);
	}
	mutex_unlock(&jobs->lock);

	atomic_fetch_add(1, &vb->usr_cnt);
	atomic_long_fetch_or(BIT(chn.mod_id), &vb->mod_ids);
	return ret;
}
EXPORT_SYMBOL_GPL(vb_qbuf);

/* vb_dqbuf: dequeue vb from the specified channel(driver).
 *
 * @param chn: the channel to be dequeued.
 * @param jobs: vb jobs.
 * @param blk: the vb_blk dequeued.
 * @return: status of operation. 0 if OK.
 */
int32_t vb_dqbuf(mmf_chn_s chn, struct vb_jobs_t *jobs, vb_blk *blk)
{
	struct vb_s *p;

	if (blk == NULL) {
		TRACE_BASE(DBG_ERR, "[%p] blk is NULL.\n", __builtin_return_address(0));
		return -1;
	}
	*blk = VB_INVALID_HANDLE;
	if (jobs == NULL) {
		TRACE_BASE(DBG_ERR, "[%p] job is NULL.\n", __builtin_return_address(0));
		return -1;
	}
	if (!jobs->inited) {
		TRACE_BASE(DBG_ERR, "mod(%s), vb_qbuf fail, jobs not initialized yet\n", sys_get_modname(chn.mod_id));
		return -1;
	}

	mutex_lock(&jobs->lock);
	// get vb from workq which is done.
	if (FIFO_EMPTY(&jobs->workq)) {
		mutex_unlock(&jobs->lock);
		TRACE_BASE(DBG_ERR, "%s ChnId(%d) No more vb for dequeue.\n",
			     sys_get_modname(chn.mod_id), chn.chn_id);
		return -1;
	}
	FIFO_POP(&jobs->workq, &p);
	mutex_unlock(&jobs->lock);
	*blk = (vb_blk)p;
	atomic_long_fetch_and(~BIT(chn.mod_id), &p->mod_ids);

	return 0;
}
EXPORT_SYMBOL_GPL(vb_dqbuf);

/* vb_done_handler: called when vb on specified chn is ready for delivery.
 *    Get vb from chn and deliver to its binding dsts if available;
 *    O/W, release back to vb_pool.
 *
 * @param chn: the chn which has vb to be released
 * @param chn_type: for modules which has both in/out.
 *                True: module generates(output) vb.
 *                False: module take(input) vb.
 * @param jobs: vb jobs.
 * @param blk: vb_blk.
 * @return: status of operation. 0 if OK.
 */
int32_t vb_done_handler(mmf_chn_s chn, enum chn_type_e chn_type, struct vb_jobs_t *jobs, vb_blk blk)
{
	mmf_bind_dest_s bind_dest;
	s32 ret;
	u8 i;
	mod_id_e id;

	if (chn_type == CHN_TYPE_OUT) {
		_handle_snap(chn, jobs, blk);
		if (bind_get_dst(&chn, &bind_dest) == 0) {
			for (i = 0; i < bind_dest.num; ++i) {
				id = bind_dest.mmf_chn[i].mod_id;
				if (g_base_recv_cb[id]) {
					ret = (*g_base_recv_cb[id])(bind_dest.mmf_chn[i], blk);
					if (ret != 0) {
						TRACE_BASE(DBG_INFO, "%s g_base_recv_cb fail.",
							sys_get_modname(bind_dest.mmf_chn[i].mod_id));
						//return ret;
					}
				}
				//vb_qbuf(stBindDest.mmf_chn[i], CHN_TYPE_IN, jobs, blk);
				TRACE_BASE(DBG_DEBUG, " Mod(%s) chn(%d) dev(%d) -> Mod(%s) chn(%d) dev(%d)\n"
					     , sys_get_modname(chn.mod_id), chn.chn_id, chn.dev_id
					     , sys_get_modname(bind_dest.mmf_chn[i].mod_id)
					     , bind_dest.mmf_chn[i].chn_id
					     , bind_dest.mmf_chn[i].dev_id);
			}
		} else {
			// release if not found
			TRACE_BASE(DBG_DEBUG, "Mod(%s) chn(%d) dev(%d) src no dst release\n"
				     , sys_get_modname(chn.mod_id), chn.chn_id, chn.dev_id);
		}
	} else {
		TRACE_BASE(DBG_DEBUG, "Mod(%s) chn(%d) dev(%d) dst out release\n"
			     , sys_get_modname(chn.mod_id), chn.chn_id, chn.dev_id);
	}
	ret = vb_release_block(blk);

	return ret;
}
EXPORT_SYMBOL_GPL(vb_done_handler);


#ifndef __VBQ_H__
#define __VBQ_H__

#include "vb.h"

struct snap_s {
	TAILQ_ENTRY(snap_s) tailq;

	//pthread_cond_t cond;
	wait_queue_head_t cond_queue;
	mmf_chn_s chn;
	vb_blk blk;
	u8 avail;
};

/*
 * lock: lock for waitq/workq.
 * waitq: the queue of vb_blk waiting to be done. For TDM modules, such as VPSS.
 * workq: the queue of vb_blk module is working on.
 * dlock: lock for doneq.
 * doneq: the queue of vb_blk to be taken. Size decided by u32Depth.
 * sem: sem to notify waitq is updated.
 * snap_jobs: the req to get frame for this chn.
 */
struct vb_jobs_t {
	struct mutex lock;
	struct vbq waitq;
	struct vbq workq;
	struct mutex dlock;
	struct vbq doneq;
	struct semaphore sem;
	TAILQ_HEAD(snap_q, snap_s) snap_jobs;
	uint8_t inited;
};

typedef int32_t (*vbq_recv_cb)(mmf_chn_s chn, vb_blk blk);

void base_register_recv_cb(mod_id_e mod_id, vbq_recv_cb cb);
void base_unregister_recv_cb(mod_id_e mod_id);

s32 base_fill_videoframe2buffer(mmf_chn_s chn, const video_frame_info_s *frame,
	struct video_buffer *buf);
s32 base_get_chn_buffer(mmf_chn_s chn, struct vb_jobs_t *jobs, vb_blk *blk, s32 timeout_ms);
void base_mod_jobs_init(struct vb_jobs_t *jobs, uint8_t waitq_depth, uint8_t workq_depth, uint8_t doneq_depth);
void base_mod_jobs_clear(struct vb_jobs_t *jobs);
void base_mod_jobs_exit(struct vb_jobs_t *jobs);
struct video_buffer *base_mod_jobs_enque_work(struct vb_jobs_t *jobs);
bool base_mod_jobs_waitq_empty(struct vb_jobs_t *jobs);
bool base_mod_jobs_workq_empty(struct vb_jobs_t *jobs);
vb_blk base_mod_jobs_waitq_pop(struct vb_jobs_t *jobs);
vb_blk base_mod_jobs_workq_pop(struct vb_jobs_t *jobs);
int32_t base_get_frame_info(pixel_format_e fmt, size_s size, struct video_buffer *buf, u64 mem_base, u8 align);
int32_t vb_qbuf(mmf_chn_s chn, enum chn_type_e chn_type, struct vb_jobs_t *jobs, vb_blk blk);
int32_t vb_dqbuf(mmf_chn_s chn, struct vb_jobs_t *jobs, vb_blk *blk);
int32_t vb_done_handler(mmf_chn_s chn, enum chn_type_e chn_type, struct vb_jobs_t *jobs, vb_blk blk);


#endif  /* __CVI_BASE_CTX_H__ */

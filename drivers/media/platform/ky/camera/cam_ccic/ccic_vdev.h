// SPDX-License-Identifier: GPL-2.0
/*
 * vdev.h - video divece functions
 *
 * Copyright(C) 2019 SPM Micro Limited
 */

#ifndef _SPM_VDEV_H_
#define _SPM_VDEV_H_
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include <linux/notifier.h>
#include "ccic_drv.h"

#define CCIC_DMA_WORK_MAX_CNT    (16)

struct spm_ccic_vbuffer;
struct spm_ccic_vnode;

struct ccic_dma_context
{
	struct list_head dma_work_idle_list;
	struct list_head dma_work_busy_list;
	spinlock_t slock;
	struct spm_ccic_vnode *ac_vnode;
};

struct spm_ccic_vnode {
	struct video_device vnode;
	char name[32];
	struct vb2_queue buf_queue;
	struct list_head queued_list;
	struct list_head busy_list;
	struct ccic_dma_context dma_ctx;
	atomic_t queued_buf_cnt;
	atomic_t busy_buf_cnt;
	atomic_t ref_cnt;
	spinlock_t slock;
	struct mutex mlock;
	struct v4l2_format cur_fmt;
	struct wait_queue_head waitq_head;
	int in_streamoff;
	int in_tasklet;
	int in_irq;
	int is_streaming;
	unsigned int idx;
	unsigned int total_frm;
	unsigned int sw_err_frm;
	unsigned int hw_err_frm;
	unsigned int ok_frm;
	unsigned int planes_offset[VB2_MAX_FRAME][VB2_MAX_PLANES];
	unsigned int v4l2_buf_flags[VB2_MAX_FRAME];
	struct ccic_dev *ccic_dev;
	int csi2vc;
	int src_sel;
	int lane_num;
	int ccic_mode;
	int ch_mode;
	unsigned int main_ccic_id;
	unsigned int main_vc;
	unsigned int sub_vc;
	unsigned int main_dt;
	unsigned int sub_dt;
	uint64_t frame_id;
	void *usr_data;
};

struct ccic_dma_work_struct {
	struct tasklet_struct dma_tasklet;
	struct list_head idle_list_entry;
	struct list_head busy_list_entry;
	unsigned int irq_status;
	struct spm_ccic_vnode *ac_vnode;
};

#define AC_BUF_FLAG_SOF_TOUCH			(1 << 0)
#define AC_BUF_FLAG_DONE_TOUCH			(1 << 1)
#define AC_BUF_FLAG_HW_ERR				(1 << 2)
#define AC_BUF_FLAG_SW_ERR				(1 << 3)
#define AC_BUF_FLAG_TIMESTAMPED			(1 << 4)
#define AC_BUF_FLAG_CCIC_TOUCH			(1 << 5)

#define AC_BUF_RESERVED_DATA_LEN		(32)
struct spm_ccic_vbuffer {
	struct vb2_v4l2_buffer vb2_v4l2_buf;
	struct list_head list_entry;
	unsigned int reset_flag;
	unsigned int flags;
	struct spm_ccic_vnode *ac_vnode;
	unsigned char reserved[AC_BUF_RESERVED_DATA_LEN];
};

#define vb2_buffer_to_spm_ccic_vbuffer(vb)	((struct spm_ccic_vbuffer*)(vb))

#define CAM_ALIGN(a, b)		({ \
								unsigned int ___tmp1 = (a); \
								unsigned int ___tmp2 = (b); \
								unsigned int ___tmp3 = ___tmp1 % ___tmp2; \
								___tmp1 /= ___tmp2; \
								if (___tmp3) \
									___tmp1++; \
								___tmp1 *= ___tmp2; \
								___tmp1; \
							})

#define is_vnode_streaming(vnode)	((vnode)->buf_queue.streaming)

static inline dma_addr_t spm_vb2_buf_paddr(struct vb2_buffer *vb, unsigned int plane_no)
{
	unsigned int offset = 0;
	dma_addr_t paddr = 0;
	struct spm_ccic_vbuffer *ac_vb = vb2_buffer_to_spm_ccic_vbuffer(vb);
	struct spm_ccic_vnode *ac_vnode = ac_vb->ac_vnode;
	dma_addr_t *dma_addr = (dma_addr_t*)vb2_plane_cookie(vb, plane_no);

	BUG_ON(!ac_vnode);
	offset = ac_vnode->planes_offset[vb->index][plane_no];
	paddr = *dma_addr + offset;
	return paddr;
}

static inline void ccic_update_dma_addr(struct spm_ccic_vnode *ac_vnode,
								struct spm_ccic_vbuffer *ac_vbuf, unsigned int offset)
{
	dma_addr_t p0 = 0;
	struct ccic_dma *ccic_dma = ac_vnode->ccic_dev->dma;
	struct vb2_buffer *vb2_buf = &(ac_vbuf->vb2_v4l2_buf.vb2_buf);

	p0 = spm_vb2_buf_paddr(vb2_buf, 0) + offset;
	ccic_dma->ops->set_addr(ccic_dma, p0, 0, 0);
}

static inline void* ac_vnode_get_usrdata(struct spm_ccic_vnode *ac_vnode)
{
	return ac_vnode->usr_data;
}

static inline struct spm_ccic_vbuffer* to_ccic_vbuffer(struct vb2_buffer *vb2)
{
	struct vb2_v4l2_buffer *vb2_v4l2_buf = to_vb2_v4l2_buffer(vb2);
	return container_of(vb2_v4l2_buf, struct spm_ccic_vbuffer, vb2_v4l2_buf);
}

struct spm_ccic_vnode* spm_cvdev_create_vnode(const char *name,
											unsigned int idx,
											struct v4l2_device *v4l2_dev,
											struct device *alloc_dev,
											struct ccic_dev *ccic_dev,
											void (*dma_tasklet_handler)(unsigned long),
											unsigned int min_buffers_needed);
void spm_cvdev_destroy_vnode(struct spm_ccic_vnode *ac_vnode);


int spm_cvdev_busy_list_empty(struct spm_ccic_vnode *ac_vnode);
int __spm_cvdev_busy_list_empty(struct spm_ccic_vnode *ac_vnode);
int spm_cvdev_idle_list_empty(struct spm_ccic_vnode *ac_vnode);
int __spm_cvdev_idle_list_empty(struct spm_ccic_vnode *ac_vnode);
int spm_cvdev_dq_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb);
int spm_cvdev_pick_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb);
int __spm_cvdev_pick_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb);
int spm_cvdev_q_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer *ac_vb);
int __spm_cvdev_dq_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb);
int __spm_cvdev_q_idle_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer *ac_vb);
int spm_cvdev_dq_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb);
int spm_cvdev_pick_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb);
int __spm_cvdev_pick_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb);
int spm_cvdev_q_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer *ac_vb);
int __spm_cvdev_dq_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer **ac_vb);
int __spm_cvdev_q_busy_vbuffer(struct spm_ccic_vnode *ac_vnode, struct spm_ccic_vbuffer *ac_vb);
int spm_cvdev_export_ccic_vbuffer(struct spm_ccic_vbuffer *ac_vb, int with_error);
void spm_cvdev_fill_v4l2_format(struct v4l2_format *f);
#endif

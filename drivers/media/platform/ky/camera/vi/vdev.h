// SPDX-License-Identifier: GPL-2.0
/*
 * vdev.h - video divece functions
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _KY_VDEV_H_
#define _KY_VDEV_H_
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include <media/x1/x1_videodev2.h>
#include <linux/notifier.h>
#include "cam_block.h"
#include "mlink.h"

struct spm_camera_vbuffer;
struct spm_camera_vnode {
	struct video_device vnode;
	char name[KY_VI_ENTITY_NAME_LEN];
	struct spm_camera_block sc_block;
	struct vb2_queue buf_queue;
	struct media_pad pad;
	struct list_head queued_list;
	struct list_head busy_list;
	atomic_t queued_buf_cnt;
	atomic_t busy_buf_cnt;
	atomic_t ref_cnt;
	spinlock_t slock;
	struct mutex mlock;
	struct blocking_notifier_head notify_chain;
	struct notifier_block pipeline_notify_block;
	struct v4l2_format cur_fmt;
	struct wait_queue_head waitq_head;
	int in_streamoff;
	int in_tasklet;
	int direction;
	unsigned int idx;
	unsigned int total_frm;
	unsigned int sw_err_frm;
	unsigned int hw_err_frm;
	unsigned int ok_frm;
	unsigned int planes_offset[VB2_MAX_FRAME][VB2_MAX_PLANES];
	unsigned int v4l2_buf_flags[VB2_MAX_FRAME];
	struct spm_camera_vbuffer *sc_vb;
	void *usr_data;
};

#define SC_BUF_FLAG_SOF_TOUCH			(1 << 0)
#define SC_BUF_FLAG_DONE_TOUCH			(1 << 1)
#define SC_BUF_FLAG_HW_ERR			(1 << 2)
#define SC_BUF_FLAG_SW_ERR			(1 << 3)
#define SC_BUF_FLAG_TIMESTAMPED			(1 << 4)
#define SC_BUF_FLAG_CCIC_TOUCH			(1 << 5)
#define SC_BUF_FLAG_SPECIAL_USE			(1 << 6)
#define SC_BUF_FLAG_CONTINOUS			(1 << 7)
#define SC_BUF_FLAG_GEN_EOF			(1 << 8)
#define SC_BUF_FLAG_RSVD_Z1			(1 << 9)
#define SC_BUF_FLAG_FORCE_SHADOW		(1 << 10)

#define SC_BUF_RESERVED_DATA_LEN		(32)
struct spm_camera_vbuffer {
	struct vb2_v4l2_buffer vb2_v4l2_buf;
	struct list_head list_entry;
	unsigned int reset_flag;
	unsigned int flags;
	__u64 timestamp_eof;
	__u64 timestamp_qbuf;
	struct spm_camera_vnode *sc_vnode;
	unsigned char reserved[SC_BUF_RESERVED_DATA_LEN];
};

#define vb2_buffer_to_spm_camera_vbuffer(vb)	((struct spm_camera_vbuffer*)(vb))

enum {
	KY_VNODE_NOTIFY_STREAM_ON = 0,
	KY_VNODE_NOTIFY_STREAM_OFF,
	KY_VNODE_NOTIFY_BUF_QUEUED,
	KY_VNODE_NOTIFY_BUF_PREPARE,
};

enum {
	KY_VNODE_DIR_IN = 0,
	KY_VNODE_DIR_OUT,
};

#define CAM_ALIGN(a, b)		({		\
		unsigned int ___tmp1 = (a);	\
		unsigned int ___tmp2 = (b);	\
		unsigned int ___tmp3 = ___tmp1 % ___tmp2;	\
		___tmp1 /= ___tmp2;		\
		if (___tmp3)			\
			___tmp1++;		\
		___tmp1 *= ___tmp2;		\
		___tmp1;			\
	})

#define is_vnode_streaming(vnode)	((vnode)->buf_queue.streaming)

static inline void *sc_vnode_get_usrdata(struct spm_camera_vnode *sc_vnode)
{
	return sc_vnode->usr_data;
}

static inline struct spm_camera_vnode *media_entity_to_sc_vnode(struct media_entity *me)
{
	if (is_subdev(me))
		return NULL;
	return (struct spm_camera_vnode *)me;
}

static inline struct spm_camera_vbuffer *to_camera_vbuffer(struct vb2_buffer *vb2)
{
	struct vb2_v4l2_buffer *vb2_v4l2_buf = to_vb2_v4l2_buffer(vb2);
	return container_of(vb2_v4l2_buf, struct spm_camera_vbuffer, vb2_v4l2_buf);
}

struct spm_camera_vnode *spm_vdev_create_vnode(const char *name,
					       int direction,
					       unsigned int idx,
					       struct v4l2_device *v4l2_dev,
					       struct device *alloc_dev,
					       unsigned int min_buffers_needed);

int spm_vdev_register_vnode_notify(struct spm_camera_vnode *sc_vnode,
				   struct notifier_block *notifier_block);
int spm_vdev_unregister_vnode_notify(struct spm_camera_vnode *sc_vnode,
				     struct notifier_block *notifier_block);

int spm_vdev_busy_list_empty(struct spm_camera_vnode *sc_vnode);
int __spm_vdev_busy_list_empty(struct spm_camera_vnode *sc_vnode);
int spm_vdev_idle_list_empty(struct spm_camera_vnode *sc_vnode);
int __spm_vdev_idle_list_empty(struct spm_camera_vnode *sc_vnode);
int spm_vdev_dq_idle_vbuffer(struct spm_camera_vnode *sc_vnode,
			     struct spm_camera_vbuffer **sc_vb);
int spm_vdev_pick_idle_vbuffer(struct spm_camera_vnode *sc_vnode,
			       struct spm_camera_vbuffer **sc_vb);
int __spm_vdev_pick_idle_vbuffer(struct spm_camera_vnode *sc_vnode,
				 struct spm_camera_vbuffer **sc_vb);
int spm_vdev_q_idle_vbuffer(struct spm_camera_vnode *sc_vnode,
			    struct spm_camera_vbuffer *sc_vb);
int __spm_vdev_dq_idle_vbuffer(struct spm_camera_vnode *sc_vnode,
			       struct spm_camera_vbuffer **sc_vb);
int __spm_vdev_q_idle_vbuffer(struct spm_camera_vnode *sc_vnode,
			      struct spm_camera_vbuffer *sc_vb);
int spm_vdev_dq_busy_vbuffer(struct spm_camera_vnode *sc_vnode,
			     struct spm_camera_vbuffer **sc_vb);
int spm_vdev_pick_busy_vbuffer(struct spm_camera_vnode *sc_vnode,
			       struct spm_camera_vbuffer **sc_vb);
int __spm_vdev_pick_busy_vbuffer(struct spm_camera_vnode *sc_vnode, struct spm_camera_vbuffer **sc_vb);
//int spm_vdev_dq_busy_vbuffer_by_paddr(struct spm_camera_vnode *sc_vnode, int plane_id, unsigned long plane_paddr, struct spm_camera_vbuffer **sc_vb);
int spm_vdev_q_busy_vbuffer(struct spm_camera_vnode *sc_vnode,
			    struct spm_camera_vbuffer *sc_vb);
int __spm_vdev_dq_busy_vbuffer(struct spm_camera_vnode *sc_vnode,
			       struct spm_camera_vbuffer **sc_vb);
//int __spm_vdev_dq_busy_vbuffer_by_paddr(struct spm_camera_vnode *sc_vnode, int plane_id, unsigned long plane_paddr, struct spm_camera_vbuffer **sc_vb);
int __spm_vdev_q_busy_vbuffer(struct spm_camera_vnode *sc_vnode,
			      struct spm_camera_vbuffer *sc_vb);
int spm_vdev_export_camera_vbuffer(struct spm_camera_vbuffer *sc_vb, int with_error);
struct spm_camera_vnode *spm_vdev_remote_vnode(struct media_pad *pad);
void spm_vdev_fill_v4l2_format(struct v4l2_subdev_format *sub_f, struct v4l2_format *f);
void spm_vdev_fill_subdev_format(struct v4l2_format *f,
				 struct v4l2_subdev_format *sub_f);
int spm_vdev_is_raw_vnode(struct spm_camera_vnode *sc_vnode);
#ifdef MODULE
void __spm_media_entity_remove_links(struct media_entity *entity);
#endif
#endif

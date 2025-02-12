/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/debugfs.h>
#include <linux/sched.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-sg.h>
#include "mvx_ext_if.h"
#include "mvx_seq.h"
#include "mvx_v4l2_buffer.h"
#include "mvx_v4l2_session.h"
#include "mvx_log_group.h"
#include "mvx-v4l2-controls.h"

/****************************************************************************
 * Exported and static functions
 ****************************************************************************/

static void set_format(struct v4l2_pix_format_mplane *pix_mp,
		       unsigned int width,
		       unsigned int height,
		       unsigned int num_planes,
		       unsigned int *sizeimage,
		       unsigned int *bytesperline)
{
	int i;

	pix_mp->width = width;
	pix_mp->height = height;
	pix_mp->num_planes = num_planes;

	for (i = 0; i < num_planes; ++i) {
		pix_mp->plane_fmt[i].sizeimage = sizeimage[i];
		pix_mp->plane_fmt[i].bytesperline = bytesperline[i];
	}
}

static void v4l2_port_show(struct mvx_v4l2_port *port,
			   struct seq_file *s)
{
	mvx_seq_printf(s, "mvx_v4l2_port", 0, "%p\n", port);
	mvx_seq_printf(s, "pixelformat", 1, "0x%x\n",
		       port->pix_mp.pixelformat);
	mvx_seq_printf(s, "vb2_queue", 1, "\n");
	mvx_seq_printf(s, "memory", 2, "%u\n",
		       port->vb2_queue.memory);
	mvx_seq_printf(s, "min_buffers_needed", 2, "%u\n",
		       port->vb2_queue.min_buffers_needed);
	mvx_seq_printf(s, "num_buffers", 2, "%u\n",
		       port->vb2_queue.num_buffers);
	mvx_seq_printf(s, "queued_count", 2, "%u\n",
		       port->vb2_queue.queued_count);
	mvx_seq_printf(s, "streaming", 2, "%u\n",
		       port->vb2_queue.streaming);
	mvx_seq_printf(s, "error", 2, "%u\n",
		       port->vb2_queue.error);
	mvx_seq_printf(s, "last_buffer_dequeued", 2, "%u\n",
		       port->vb2_queue.last_buffer_dequeued);
}

static int port_stat_show(struct seq_file *s,
			  void *v)
{
	struct mvx_v4l2_port *vport = s->private;
	struct mvx_session_port *sport = vport->port;

	mvx_session_port_show(sport, s);
	seq_puts(s, "\n");
	v4l2_port_show(vport, s);

	return 0;
}

static int port_stat_open(struct inode *inode,
			  struct file *file)
{
	return single_open(file, port_stat_show, inode->i_private);
}

static const struct file_operations port_stat_fops = {
	.open    = port_stat_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int port_debugfs_init(struct device *dev,
			     unsigned int i,
			     struct mvx_v4l2_port *vport,
			     struct mvx_session_port *sport,
			     struct dentry *parent)
{
	char name[20];
	struct dentry *dentry;

	scnprintf(name, sizeof(name), "port%u", i);
	vport->dentry = debugfs_create_dir(name, parent);
	if (IS_ERR_OR_NULL(vport->dentry))
		return -ENOMEM;

	dentry = debugfs_create_file("stat", 0400, vport->dentry, vport,
				     &port_stat_fops);
	if (IS_ERR_OR_NULL(dentry))
		return -ENOMEM;

	return 0;
}

static int session_debugfs_init(struct mvx_v4l2_session *session,
				struct dentry *parent)
{
	int ret;
	char name[20];
	int i;

	scnprintf(name, sizeof(name), "%lx", (unsigned long)(&session->session));
	session->dentry = debugfs_create_dir(name, parent);
	if (IS_ERR_OR_NULL(session->dentry))
		return -ENOMEM;

	for (i = 0; i < MVX_DIR_MAX; i++) {
		struct mvx_v4l2_port *vport = &session->port[i];
		struct mvx_session_port *mport = &session->session.port[i];

		ret = port_debugfs_init(session->ext->dev, i, vport, mport,
					session->dentry);
		if (ret != 0)
			goto remove_dentry;
	}

	return 0;

remove_dentry:
	debugfs_remove_recursive(session->dentry);
	return ret;
}

static struct mvx_v4l2_session *mvx_session_to_v4l2_session(
	struct mvx_session *session)
{
	return container_of(session, struct mvx_v4l2_session, session);
}

static void free_session(struct mvx_session *session)
{
	struct mvx_v4l2_session *s = mvx_session_to_v4l2_session(session);

	MVX_SESSION_INFO(session, "v4l2: Destroy session.");

	mvx_session_destruct(session);

	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(s->dentry);

	v4l2_fh_del(&s->fh);
	v4l2_fh_exit(&s->fh);
    if (mutex_is_locked(&s->mutex)) {
        mutex_unlock(&s->mutex);
    }
	devm_kfree(s->ext->dev, s);
}
#ifdef MODULE
static unsigned sev_pos(const struct v4l2_subscribed_event *sev, unsigned idx)
{
	idx += sev->first;
	return idx >= sev->elems ? idx - sev->elems : idx;
}

/* Caller must hold fh->vdev->fh_lock! */
static struct v4l2_subscribed_event *v4l2_event_subscribed(
		struct v4l2_fh *fh, u32 type, u32 id)
{
	struct v4l2_subscribed_event *sev;

	assert_spin_locked(&fh->vdev->fh_lock);

	list_for_each_entry(sev, &fh->subscribed, list)
		if (sev->type == type && sev->id == id)
			return sev;

	return NULL;
}

static void __v4l2_event_queue_fh(struct v4l2_fh *fh,
				  const struct v4l2_event *ev, u64 ts)
{
	struct v4l2_subscribed_event *sev;
	struct v4l2_kevent *kev;
	bool copy_payload = true;

	/* Are we subscribed? */
	sev = v4l2_event_subscribed(fh, ev->type, ev->id);
	if (sev == NULL)
		return;

	/* Increase event sequence number on fh. */
	fh->sequence++;

	/* Do we have any free events? */
	if (sev->in_use == sev->elems) {
		/* no, remove the oldest one */
		kev = sev->events + sev_pos(sev, 0);
		list_del(&kev->list);
		sev->in_use--;
		sev->first = sev_pos(sev, 1);
		fh->navailable--;
		if (sev->elems == 1) {
			if (sev->ops && sev->ops->replace) {
				sev->ops->replace(&kev->event, ev);
				copy_payload = false;
			}
		} else if (sev->ops && sev->ops->merge) {
			struct v4l2_kevent *second_oldest =
				sev->events + sev_pos(sev, 0);
			sev->ops->merge(&kev->event, &second_oldest->event);
		}
	}

	/* Take one and fill it. */
	kev = sev->events + sev_pos(sev, sev->in_use);
	kev->event.type = ev->type;
	if (copy_payload)
		kev->event.u = ev->u;
	kev->event.id = ev->id;
	kev->ts = ts;
	kev->event.sequence = fh->sequence;
	sev->in_use++;
	list_add_tail(&kev->list, &fh->available);

	fh->navailable++;

	wake_up_all(&fh->wait);
}

void ky_v4l2_event_queue_fh(struct v4l2_fh *fh, const struct v4l2_event *ev)
{
	unsigned long flags;
	u64 ts = ktime_get_ns();

	spin_lock_irqsave(&fh->vdev->fh_lock, flags);
	__v4l2_event_queue_fh(fh, ev, ts);
	spin_unlock_irqrestore(&fh->vdev->fh_lock, flags);
}
void ky_vb2_queue_error(struct vb2_queue *q)
{
	q->error = 1;

	wake_up_all(&q->done_wq);
}

#endif
static void handle_event(struct mvx_session *session,
			 enum mvx_session_event event,
			 void *arg)
{
	struct mvx_v4l2_session *vsession =
		mvx_session_to_v4l2_session(session);

	MVX_SESSION_INFO(&vsession->session,
			 "Event. event=%d, arg=%p.", event, arg);

	switch (event) {
	case MVX_SESSION_EVENT_BUFFER: {
		struct mvx_v4l2_buffer *vbuf = mvx_buffer_to_v4l2_buffer(arg);
		struct vb2_buffer *vb = &vbuf->vb2_v4l2_buffer.vb2_buf;

		/*
		 * When streaming is stopped we don't always receive all
		 * buffers from FW back. So we just return them all to Vb2.
		 * If the FW later returns a buffer to us, we could silently
		 * skip it.
		 */
		if (vb->state != VB2_BUF_STATE_DEQUEUED && vb->state != VB2_BUF_STATE_DONE) {
			enum vb2_buffer_state state =
				mvx_v4l2_buffer_update(vbuf);

			vb2_buffer_done(vb, state);
		}

		if (vbuf->buf.dir == MVX_DIR_OUTPUT && vb->state != VB2_BUF_STATE_QUEUED
				&& !waitqueue_active(&vb->vb2_queue->done_wq)
                && vsession->port[MVX_DIR_INPUT].q_set
				&& waitqueue_active(&vsession->port[MVX_DIR_INPUT].vb2_queue.done_wq)) {
				/*wake up waiters of input queue, because output queue may reallocated and not registered in waiters list */
				wake_up(&vsession->port[MVX_DIR_INPUT].vb2_queue.done_wq);
		}
		break;
	}
	case MVX_SESSION_EVENT_PORT_CHANGED: {
		enum mvx_direction dir = (enum mvx_direction)arg;
		struct mvx_v4l2_port *vport = &vsession->port[dir];
		struct mvx_session_port *port = &session->port[dir];
		const struct v4l2_event event = {
			.type                 = V4L2_EVENT_SOURCE_CHANGE,
			.u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION
		};
		struct v4l2_pix_format_mplane *p = &vport->pix_mp;
		p->field = port->interlaced ? V4L2_FIELD_SEQ_TB : V4L2_FIELD_NONE;
		set_format(&vport->pix_mp, port->width, port->height,
			   port->nplanes, port->size, port->stride);
#ifndef MODULE
		v4l2_event_queue_fh(&vsession->fh, &event);
#else
		ky_v4l2_event_queue_fh(&vsession->fh, &event);
#endif
		break;
	}
	case MVX_SESSION_EVENT_COLOR_DESC: {
		const struct v4l2_event event = {
			.type = V4L2_EVENT_MVX_COLOR_DESC,
		};
#ifndef MODULE
		v4l2_event_queue_fh(&vsession->fh, &event);
#else
		ky_v4l2_event_queue_fh(&vsession->fh, &event);
#endif
		break;
	}
	case MVX_SESSION_EVENT_ERROR: {
		int i;

		for (i = 0; i < MVX_DIR_MAX; ++i) {
			struct vb2_queue *q = &vsession->port[i].vb2_queue;
#ifndef MODULE
			vb2_queue_error(q);
#else
			ky_vb2_queue_error(q);
#endif
		}

		break;
	}
	default:
		MVX_SESSION_WARN(&vsession->session,
				 "Unsupported session event. event=%d", event);
	}
}

int mvx_v4l2_session_construct(struct mvx_v4l2_session *vsession,
			       struct mvx_ext_if *ctx)
{
	int i;
	int ret;

	vsession->ext = ctx;
	mutex_init(&vsession->mutex);

	for (i = 0; i < MVX_DIR_MAX; i++) {
		struct mvx_v4l2_port *vport = &vsession->port[i];

		vport->port = &vsession->session.port[i];
		vport->vsession = vsession;
		vport->dir = i;
		vport->q_set = false;
	}

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		ret = session_debugfs_init(vsession, ctx->dsessions);
		if (ret != 0)
			return ret;
	}

	ret = mvx_session_construct(&vsession->session, ctx->dev,
				    ctx->client_ops, ctx->cache,
				    &vsession->mutex,
				    free_session, handle_event,
				    vsession->dentry);
	if (ret != 0)
		goto remove_dentry;

	return 0;

remove_dentry:
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(vsession->dentry);

	return ret;
}

struct mvx_v4l2_session *v4l2_fh_to_session(struct v4l2_fh *fh)
{
	return container_of(fh, struct mvx_v4l2_session, fh);
}

struct mvx_v4l2_session *file_to_session(struct file *file)
{
	return v4l2_fh_to_session(file->private_data);
}

int mvx_v4l2_session_get_color_desc(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_color_desc *c)
{
	int ret;
	struct mvx_fw_color_desc cd;

	ret = mvx_session_get_color_desc(&vsession->session, &cd);
	if (ret != 0)
		return ret;

	c->flags = 0;

	/* Convert between generic fw_color_desc and V4L2 color desc. */
	c->range = cd.range;
	c->primaries = cd.primaries;
	c->transfer = cd.transfer;
	c->matrix = cd.matrix;

	if (cd.flags & MVX_FW_COLOR_DESC_DISPLAY_VALID) {
		c->flags |= V4L2_MVX_COLOR_DESC_DISPLAY_VALID;
		c->display.r.x = cd.display.r.x;
		c->display.r.y = cd.display.r.y;
		c->display.g.x = cd.display.g.x;
		c->display.g.y = cd.display.g.y;
		c->display.b.x = cd.display.b.x;
		c->display.b.y = cd.display.b.y;
		c->display.w.x = cd.display.w.x;
		c->display.w.y = cd.display.w.y;
		c->display.luminance_min = cd.display.luminance_min;
		c->display.luminance_max = cd.display.luminance_max;
	}

	if (cd.flags & MVX_FW_COLOR_DESC_CONTENT_VALID) {
		c->flags |= V4L2_MVX_COLOR_DESC_CONTENT_VALID;
		c->content.luminance_max = cd.content.luminance_max;
		c->content.luminance_average = cd.content.luminance_average;
	}

	return 0;
}

int mvx_v4l2_session_set_color_desc(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_color_desc *c){
    int ret;
    struct mvx_fw_color_desc cd;
    memset(&cd, 0, sizeof(cd));
    cd.flags = c->flags;
    cd.range = c->range;
    cd.matrix = c->matrix;
    cd.primaries = c->primaries;
    cd.transfer = c->transfer;
    cd.aspect_ratio_idc = c->aspect_ratio_idc;
    cd.num_units_in_tick = c->num_units_in_tick;
    cd.sar_height = c->sar_height;
    cd.sar_width = c->sar_width;
    cd.video_format = c->video_format;
    cd.time_scale = c->time_scale;
    cd.content.luminance_average = c->content.luminance_average;
    cd.content.luminance_max = c->content.luminance_max;
    //memcpy(&cd.display, &c->display, sizeof(c->display));
    cd.display.r.x = c->display.r.x;
    cd.display.r.y = c->display.r.y;
    cd.display.g.x = c->display.g.x;
    cd.display.g.y = c->display.g.y;
    cd.display.b.x = c->display.b.x;
    cd.display.b.y = c->display.b.y;
    cd.display.w.x = c->display.w.x;
    cd.display.w.y = c->display.w.y;
    cd.display.luminance_min = c->display.luminance_min;
    cd.display.luminance_max = c->display.luminance_max;
    ret = mvx_session_set_color_desc(&vsession->session, &cd);
    return ret;
}

int mvx_v4l2_session_set_roi_regions(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_roi_regions *roi)
{
    int ret;
    struct mvx_roi_config roi_regions;
    roi_regions.pic_index = roi->pic_index;
    roi_regions.num_roi = roi->num_roi;
    roi_regions.qp_present = roi->qp_present;
    roi_regions.roi_present = roi->roi_present;
    roi_regions.qp = roi->qp;

    if (roi_regions.roi_present && roi_regions.num_roi > 0) {
        int i = 0;
        for (;i < roi_regions.num_roi; i++) {
            roi_regions.roi[i].mbx_left = roi->roi[i].mbx_left;
            roi_regions.roi[i].mbx_right = roi->roi[i].mbx_right;
            roi_regions.roi[i].mby_top = roi->roi[i].mby_top;
            roi_regions.roi[i].mby_bottom = roi->roi[i].mby_bottom;
            roi_regions.roi[i].qp_delta = roi->roi[i].qp_delta;
        }
    }
    ret = mvx_session_set_roi_regions(&vsession->session, &roi_regions);
    if (ret != 0)
        return ret;

    return 0;
}

int mvx_v4l2_session_set_qp_epr(struct mvx_v4l2_session *vsession,
				    int *qp)
{
    int ret;
    ret = mvx_session_set_qp_epr(&vsession->session, qp);
    if (ret != 0)
        return ret;

    return 0;
}

int mvx_v4l2_session_set_sei_userdata(struct mvx_v4l2_session *vsession,
				    struct v4l2_sei_user_data *sei_userdata)
{
    int ret;
    struct mvx_sei_userdata userdata;
    userdata.flags = sei_userdata->flags;
    userdata.user_data_len = sei_userdata->user_data_len;
    memcpy(&userdata.user_data, &sei_userdata->user_data, sizeof(userdata.user_data));
    memcpy(&userdata.uuid, &sei_userdata->uuid, sizeof(userdata.uuid));
    ret = mvx_session_set_sei_userdata(&vsession->session, &userdata);
    if (ret != 0)
        return ret;

    return 0;
}

int mvx_v4l2_session_set_rate_control(struct mvx_v4l2_session *vsession,
				    struct v4l2_rate_control *rc)
{
    int ret;
    struct mvx_buffer_param_rate_control mvx_rc;
    mvx_rc.rate_control_mode = rc->rc_type;
    mvx_rc.target_bitrate = rc->target_bitrate;
    mvx_rc.maximum_bitrate = rc->maximum_bitrate;
    ret = mvx_session_set_bitrate_control(&vsession->session, &mvx_rc);
    if (ret != 0)
        return ret;

    return 0;
}

int mvx_v4l2_session_set_dsl_frame(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_dsl_frame *dsl)
{
    int ret;
    struct mvx_dsl_frame dsl_frame;
    dsl_frame.width = dsl->width;
    dsl_frame.height = dsl->height;
    ret = mvx_session_set_dsl_frame(&vsession->session, &dsl_frame);
    if (ret != 0)
        return ret;

    return 0;
}

int mvx_v4l2_session_set_dsl_ratio(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_dsl_ratio *dsl)
{
    int ret;
    struct mvx_dsl_ratio dsl_ratio;
    dsl_ratio.hor = dsl->hor;
    dsl_ratio.ver = dsl->ver;

    ret = mvx_session_set_dsl_ratio(&vsession->session, &dsl_ratio);
    if (ret != 0)
        return ret;

    return 0;
}

int mvx_v4l2_session_set_long_term_ref(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_long_term_ref *ltr)
{
    int ret;
    struct mvx_long_term_ref mvx_ltr;
    mvx_ltr.mode = ltr->mode;
    mvx_ltr.period = ltr->period;
    ret = mvx_session_set_long_term_ref(&vsession->session, &mvx_ltr);
    if (ret != 0)
        return ret;

    return 0;

}

int mvx_v4l2_session_set_dsl_mode(struct mvx_v4l2_session *vsession,
				    int *mode)
{
    int ret;
    ret = mvx_session_set_dsl_mode(&vsession->session, mode);
    if (ret != 0)
        return ret;

    return 0;
}

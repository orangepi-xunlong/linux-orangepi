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

#include <linux/device.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-sg.h>
#include "mvx_bitops.h"
#include "mvx_ext_if.h"
#include "mvx_if.h"
#include "mvx_v4l2_buffer.h"
#include "mvx_v4l2_session.h"
#include "mvx_v4l2_vidioc.h"
#include "mvx-v4l2-controls.h"

/****************************************************************************
 * Types
 ****************************************************************************/

struct mvx_format_map {
	enum mvx_format format;
	uint32_t flags;
	uint32_t pixelformat;
	const char *description;
};

/****************************************************************************
 * Static functions and variables
 ****************************************************************************/

struct mvx_format_map mvx_fmts[] = {
    { MVX_FORMAT_AVS,
      V4L2_FMT_FLAG_COMPRESSED,
      V4L2_PIX_FMT_AVS,
      "AVS" },
	{ MVX_FORMAT_AVS2,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_AVS2,
	  "AVS2" },
	{ MVX_FORMAT_H263,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_H263,
	  "H.263" },
	{ MVX_FORMAT_H264,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_H264,
	  "H.264" },
	{ MVX_FORMAT_H264,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_H264_MVC,
	  "H.264 MVC" },
	{ MVX_FORMAT_H264,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_H264_NO_SC,
	  "H.264 (No Start Codes)" },
	{ MVX_FORMAT_HEVC,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_HEVC,
	  "HEVC" },
	{ MVX_FORMAT_MPEG2,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_MPEG2,
	  "MPEG-2 ES" },
	{ MVX_FORMAT_MPEG4,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_MPEG4,
	  "MPEG-4 part 2 ES" },
	{ MVX_FORMAT_RV,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_RV,
	  "Real Video" },
	{ MVX_FORMAT_VC1,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_VC1_ANNEX_G,
	  "VC-1 (SMPTE 412M Annex G)" },
	{ MVX_FORMAT_VC1,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_VC1_ANNEX_L,
	  "VC-1 (SMPTE 412M Annex L)" },
	{ MVX_FORMAT_VP8,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_VP8,
	  "VP8" },
	{ MVX_FORMAT_VP9,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_VP9,
	  "VP9" },
	{ MVX_FORMAT_JPEG,
	  V4L2_FMT_FLAG_COMPRESSED,
	  V4L2_PIX_FMT_JPEG,
	  "JPEG" },
	{ MVX_FORMAT_YUV420_AFBC_8,
	  0,
	  V4L2_PIX_FMT_YUV420_AFBC_8,
	  "YUV420 AFBC 8 bit" },
	{ MVX_FORMAT_YUV420_AFBC_10,
	  0,
	  V4L2_PIX_FMT_YUV420_AFBC_10,
	  "YUV420 AFBC 10 bit" },
	{ MVX_FORMAT_YUV422_AFBC_8,
	  0,
	  V4L2_PIX_FMT_YUV422_AFBC_8,
	  "YUV422 AFBC 8 bit" },
	{ MVX_FORMAT_YUV422_AFBC_10,
	  0,
	  V4L2_PIX_FMT_YUV422_AFBC_10,
	  "YUV422 AFBC 10 bit" },
	{ MVX_FORMAT_YUV420_I420,
	  0,
	  V4L2_PIX_FMT_YUV420M,
	  "Planar YUV 4:2:0 (N-C)" },
	{ MVX_FORMAT_YUV420_NV12,
	  0,
	  V4L2_PIX_FMT_NV12,
	  "Y/CbCr 4:2:0" },
	{ MVX_FORMAT_YUV420_NV21,
	  0,
	  V4L2_PIX_FMT_NV21,
	  "Y/CrCb 4:2:0 (N-C)" },
	{ MVX_FORMAT_YUV420_NV12,
	  0,
	  V4L2_PIX_FMT_NV12M,
	  "Y/CbCr 4:2:0" },
	{ MVX_FORMAT_YUV420_P010,
	  0,
	  V4L2_PIX_FMT_P010,
	  "YUV 4:2:0 P010 (Microsoft format)" },
	{ MVX_FORMAT_YUV420_Y0L2,
	  0,
	  V4L2_PIX_FMT_Y0L2,
	  "YUV 4:2:0 Y0L2 (ARM format)" },
	{ MVX_FORMAT_YUV420_AQB1,
	  0,
	  v4l2_fourcc('Y', '0', 'A', 'B'),
	  "YUV 4:2:0 AQB1 (ARM format)" },
	{ MVX_FORMAT_YUV422_YUY2,
	  0,
	  V4L2_PIX_FMT_YUYV,
	  "YYUV 4:2:2" },
	{ MVX_FORMAT_YUV422_UYVY,
	  0,
	  V4L2_PIX_FMT_UYVY,
	  "UYVY 4:2:2" },
	{ MVX_FORMAT_YUV422_Y210,
	  0,
	  V4L2_PIX_FMT_Y210,
	  "YUV 4:2:2 Y210 (Microsoft format)" },

	/* ARGB */
	{ MVX_FORMAT_ARGB_8888,
	  0,
	  DRM_FORMAT_BGRA8888, /* Equal to V4L2_PIX_FMT_ARGB32. */
	  "32-bit ARGB 8-8-8-8" },
	{ MVX_FORMAT_ARGB_8888,
	  0,
	  V4L2_PIX_FMT_RGB32,
	  "32-bit ARGB 8-8-8-8" },

	/* ABGR */
	{ MVX_FORMAT_ABGR_8888,
	  0,
	  DRM_FORMAT_RGBA8888,
	  "32-bit ABGR-8-8-8-8" },

	/* RGBA */
	{ MVX_FORMAT_RGBA_8888,
	  0,
	  DRM_FORMAT_ABGR8888,
	  "32-bit RGBA 8-8-8-8" },

	/* BGRA (new and legacy format) */
	{ MVX_FORMAT_BGRA_8888,
	  0,
	  DRM_FORMAT_ARGB8888, /* Equal to V4L2_PIX_FMT_ABGR32. */
	  "32-bit BGRA 8-8-8-8" },
	{ MVX_FORMAT_BGRA_8888,
	  0,
	  V4L2_PIX_FMT_BGR32,
	  "32-bit BGRA 8-8-8-8" }
};

/*
 * Search for format map that matches given pixel format.
 */
static struct mvx_format_map *mvx_find_format(uint32_t pixelformat)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mvx_fmts); i++)
		if (mvx_fmts[i].pixelformat == pixelformat)
			return &mvx_fmts[i];

	return ERR_PTR(-EINVAL);
}

static int to_v4l2_format(struct v4l2_format *f,
			  enum v4l2_buf_type type,
			  struct v4l2_pix_format_mplane *pix,
			  unsigned int *stride,
			  unsigned int *size,
			  bool interlaced)
{
	f->type = type;

	switch (f->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE: {
		struct v4l2_pix_format *p = &f->fmt.pix;
		uint32_t i;

		p->width = pix->width;
		p->height = pix->height;
		p->pixelformat = pix->pixelformat;
		p->field = interlaced ? V4L2_FIELD_SEQ_TB : V4L2_FIELD_NONE;
		p->colorspace = pix->colorspace;
		p->flags = pix->flags;
		p->ycbcr_enc = pix->ycbcr_enc;
		p->quantization = pix->quantization;
		p->xfer_func = pix->xfer_func;

		p->sizeimage = 0;
		p->bytesperline = stride[0];
		for (i = 0; i < pix->num_planes; ++i)
			p->sizeimage += size[i];

		break;
	}
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE: {
		struct v4l2_pix_format_mplane *p = &f->fmt.pix_mp;
		int i;

		memcpy(p, pix, sizeof(*p));
		memset(p->reserved, 0, sizeof(p->reserved));
		p->field = interlaced ? V4L2_FIELD_SEQ_TB : V4L2_FIELD_NONE;

		for (i = 0; i < pix->num_planes; i++) {
			p->plane_fmt[i].bytesperline = stride[i];
			p->plane_fmt[i].sizeimage = size[i];
			memset(p->plane_fmt[i].reserved, 0,
			       sizeof(p->plane_fmt[i].reserved));
		}

		break;
	}
	default:
		return -EINVAL;
	}

	return 0;
}

static int from_v4l2_format(struct mvx_v4l2_session *vsession,
			    struct v4l2_format *f,
			    struct v4l2_pix_format_mplane *pix,
			    enum mvx_format *format,
			    unsigned int *stride,
			    unsigned int *size,
			    bool *interlaced)
{
	struct mvx_format_map *map;

	switch (f->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE: {
		struct v4l2_pix_format *p = &f->fmt.pix;

		memset(pix, 0, sizeof(*pix));

		pix->width = p->width;
		pix->height = p->height;
		pix->pixelformat = p->pixelformat;
		pix->field = p->field;
		pix->colorspace = p->colorspace;
		pix->flags = p->flags;

		if (p->priv != V4L2_PIX_FMT_PRIV_MAGIC) {
			pix->ycbcr_enc = V4L2_COLORSPACE_DEFAULT;
			pix->quantization = V4L2_QUANTIZATION_DEFAULT;
			pix->xfer_func = V4L2_XFER_FUNC_DEFAULT;
		}

		pix->num_planes = 1;
		pix->plane_fmt[0].sizeimage = p->sizeimage;
		pix->plane_fmt[0].bytesperline = p->bytesperline;

		size[0] = p->sizeimage;
		stride[0] = p->bytesperline;

		break;
	}
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE: {
		struct v4l2_pix_format_mplane *p = &f->fmt.pix_mp;
		unsigned int i;

		if (p->num_planes > MVX_BUFFER_NPLANES)
			MVX_SESSION_WARN(&vsession->session,
					 "Too many planes for format. format=0x%08x, num_planes=%u.",
					 pix->pixelformat, p->num_planes);

		memcpy(pix, p, sizeof(*pix));

		for (i = 0;
		     i < min_t(unsigned int, MVX_BUFFER_NPLANES, p->num_planes);
		     i++) {
			size[i] = p->plane_fmt[i].sizeimage;
			stride[i] = p->plane_fmt[i].bytesperline;
		}

		break;
	}
	default:
		return -EINVAL;
	}

	/* Adjust default field and color spaces. */

	if (pix->field == V4L2_FIELD_SEQ_TB) {
		*interlaced = true;
	} else {
		pix->field = V4L2_FIELD_NONE;
		*interlaced = false;
	}

	if (pix->colorspace == V4L2_COLORSPACE_DEFAULT)
		pix->colorspace = V4L2_COLORSPACE_REC709;

	if (pix->ycbcr_enc == V4L2_YCBCR_ENC_DEFAULT)
		pix->ycbcr_enc = V4L2_YCBCR_ENC_709;

	if (pix->quantization == V4L2_QUANTIZATION_DEFAULT)
		pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;

	if (pix->xfer_func == V4L2_XFER_FUNC_DEFAULT)
		pix->xfer_func = V4L2_XFER_FUNC_709;

	/* Find mapping between pixel format and mvx format. */
	map = mvx_find_format(pix->pixelformat);
	if (IS_ERR(map)) {
		MVX_SESSION_WARN(&vsession->session,
				 "Unsupported V4L2 pixel format. format=0x%08x.",
				 pix->pixelformat);
		return PTR_ERR(map);
	}

	*format = map->format;

	return 0;
}

/**
 * print_format() - Print V4L2 format.
 * @session:	Pointer to MVX session.
 * @f:		V4L2 format.
 * @prefix:	Prefix string.
 */
static void print_format(struct mvx_session *session,
			 struct v4l2_format *f,
			 const char *prefix)
{
	if (V4L2_TYPE_IS_MULTIPLANAR(f->type) != false) {
		struct v4l2_pix_format_mplane *p = &f->fmt.pix_mp;

		MVX_SESSION_INFO(session,
				 "v4l2: %s. type=%u, pixelformat=0x%08x, width=%u, height=%u, num_planes=%u.",
				 prefix,
				 f->type, p->pixelformat,
				 p->width, p->height,
				 p->num_planes);
	} else {
		struct v4l2_pix_format *p = &f->fmt.pix;

		MVX_SESSION_INFO(session,
				 "v4l2: %s. type=%u, pixelformat=0x%08x, width=%u, height=%u.",
				 prefix,
				 f->type, p->pixelformat,
				 p->width, p->height);
	}
}

/**
 * queue_setup() - Initialize or verify queue parameters.
 * @q:		Videobuf2 queue.
 * @buf_cnt:	Requested/requered buffers count.
 * @plane_cnt:	Required number of planes.
 * @plane_size:	Required size of each plane.
 * @alloc_devs:	Device to allocate memory from.
 *
 * This callback is used to query parameters of a queue from the driver.
 * Vb2 sets buf_cnt to requested amount of buffers, but a driver is free to
 * choose another value and return it. Vb2 will then call queue_setup() again
 * to verify that the new value is accepted by a driver.
 *
 * Vb2 also uses plane_cnt parameter to signal if queue_setup() was called
 * from create_bufs() of reqbufs().
 *
 * No locking is required in this function. The reason is that will be called
 * from within vb2_reqbufs() or vb2_create_bufs() which are executed from our
 * code with session mutex already taken.
 *
 * Return: 0 on success, else error code.
 */
#if KERNEL_VERSION(4, 5, 0) <= LINUX_VERSION_CODE
static int queue_setup(struct vb2_queue *q,
		       unsigned int *buf_cnt,
		       unsigned int *plane_cnt,
		       unsigned int plane_size[],
		       struct device *alloc_devs[])
#else
static int queue_setup(struct vb2_queue *q,
		       const void *unused,
		       unsigned int *buf_cnt,
		       unsigned int *plane_cnt,
		       unsigned int plane_size[],
		       void *alloc_devs[])
#endif
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_session_port *port = vport->port;
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	unsigned int i;

	/*
	 * If the output frame resolution is not known, then there is no need
	 * to allocate buffers yet. But 1 buffer will be needed to carry
	 * information about 'resolution change' and 'end of stream'.
	 */
	if (vport->dir == MVX_DIR_OUTPUT &&
	    mvx_is_frame(port->format) != false &&
	    (port->width == 0 || port->height == 0))
		*buf_cnt = 1;

	memset(plane_size, 0, sizeof(plane_size[0]) * VB2_MAX_PLANES);
	*plane_cnt = port->nplanes;
	for (i = 0; i < port->nplanes; ++i) {
		/*  Vb2 allocator does not handle well buffers of zero size. */
		plane_size[i] = max_t(unsigned int, port->size[i], 1);
		alloc_devs[i] = session->dev;
	}

	MVX_SESSION_VERBOSE(session,
			    "queue_setup. vsession=%p, vport=%p, vb2_queue=%p, dir=%d, format=0x%x, width=%u, height=%u, nplanes=%u, plane_size=[%u, %u, %u]",
			    vsession, vport, q, vport->dir, port->format,
			    port->width, port->height, port->nplanes,
			    plane_size[0], plane_size[1], plane_size[2]);

	return 0;
}

/**
 * buf_init() - Perform initilization for Vb2 buffer.
 * @b:		Pointer to Vb2 buffer.
 *
 * Vb2 framework calls this function once for every allocated buffer.
 * A driver fetches a list of memory pages and constructs MVX V4L2 buffers.
 *
 * No locking is required in this function. The reason is that will be called
 * from within vb2_reqbufs() or vb2_create_bufs() which are executed from our
 * code with session mutex already taken.
 *
 * Return: 0 in case of success, error code otherwise.
 */
static int buf_init(struct vb2_buffer *b)
{
	struct mvx_v4l2_buffer *vbuf = vb2_to_mvx_v4l2_buffer(b);

	int ret;
	unsigned int i;
	struct sg_table *sgt[MVX_BUFFER_NPLANES] = { 0 };
	struct vb2_queue *q = b->vb2_queue;
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	unsigned int flags = vbuf->buf.flags;

	MVX_SESSION_VERBOSE(session,
			    "v4l2: Initialize buffer. vb=%p, type=%u, index=%u, num_planes=%u.",
			    b, b->type, b->index, b->num_planes);

	if (b->num_planes > MVX_BUFFER_NPLANES) {
		MVX_SESSION_WARN(session,
				 "Failed to initialize buffer. Too many planes. vb=%p, num_planes=%u.",
				 b, b->num_planes);
		return -EINVAL;
	}

	for (i = 0; i < b->num_planes; ++i) {
		sgt[i] = vb2_dma_sg_plane_desc(b, i);
		if (sgt[i] == NULL) {
			MVX_SESSION_WARN(session,
					 "Cannot fetch SG descriptor. vb=%p, plane=%u.",
					 b, i);
			return -ENOMEM;
		}
	}

	ret = mvx_v4l2_buffer_construct(vbuf, vsession, vport->dir,
					b->num_planes, sgt);

	vbuf->buf.flags = flags;

	return ret;
}

/**
 * buf_cleanup() - Destroy data associated to Vb2 buffer.
 * @b:		Pointer to Vb2 buffer.
 *
 * Vb2 framework calls this function while destroying a buffer.
 */
static void buf_cleanup(struct vb2_buffer *b)
{
	struct vb2_queue *q = b->vb2_queue;
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	struct mvx_v4l2_buffer *vbuf = vb2_to_mvx_v4l2_buffer(b);

       if (session->port[vport->dir].stream_on) {
            MVX_SESSION_VERBOSE(session,
                "v4l2: Cleanup buffer in the coding process. Will remap mve va and pa address. dir=%d, type=%u, index=%u, vb=%p, vbuf=%p.",
                vport->dir, b->type, b->index, b, vbuf);
       } else {
            MVX_SESSION_VERBOSE(session,
                "v4l2: Cleanup buffer. type=%u, index=%u, vb=%p, vbuf=%p.",
                b->type, b->index, b, vbuf);
       }

	mvx_v4l2_buffer_destruct(vbuf);
}

/**
 * start_streaming() - Start streaming for queue.
 * @q:		Pointer to a queue.
 * @cnt:	Amount of buffers already owned by a driver.
 *
 * Vb2 calls this function when it is ready to start streaming for a queue.
 * Vb2 ensures that minimum required amount of buffers were enqueued to the
 * driver before calling this function.
 *
 * Return: 0 in case of success, error code otherwise.
 */
static int start_streaming(struct vb2_queue *q,
			   unsigned int cnt)
{
	/*
	 * Parameter cnt is not used so far.
	 */
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	int ret;

	MVX_SESSION_VERBOSE(session,
			    "v4l2: Start streaming. queue=%p, type=%u, cnt=%u.",
			    q, q->type, cnt);

	ret = mvx_session_streamon(&vsession->session, vport->dir);

	/*
	 * If attempt was not successful, we should return all owned buffers
	 * to Vb2 with vb2_buffer_done() with state VB2_BUF_STATE_QUEUED.
	 */
	if (ret != 0 && atomic_read(&q->owned_by_drv_count) > 0) {
		int i;

		for (i = 0; i < q->num_buffers; ++i)
			if (q->bufs[i]->state == VB2_BUF_STATE_ACTIVE)
				vb2_buffer_done(q->bufs[i],
						VB2_BUF_STATE_QUEUED);

		WARN_ON(atomic_read(&q->owned_by_drv_count));
	}

	return ret;
}

/**
 * stop_streaming() - Stop streaming for a queue.
 * @q:		Pointer to a queue.
 *
 * Vb2 calls this function when streaming should be terminated.
 * The driver must ensure that no DMA transfers are ongoing and
 * return all buffers to Vb2 with vb2_buffer_done().
 */
static void stop_streaming(struct vb2_queue *q)
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;

	MVX_SESSION_VERBOSE(session,
			    "v4l2: Stop streaming. queue=%p, type=%u.",
			    q, q->type);

	mvx_session_streamoff(&vsession->session, vport->dir);

	/*
	 * We have to return all owned buffers to Vb2 before exiting from
	 * this callback.
	 *
	 * Note: there must be no access to buffers after they are returned.
	 */
	if (atomic_read(&q->owned_by_drv_count) > 0) {
		int i;

		for (i = 0; i < q->num_buffers; ++i)
			if (q->bufs[i]->state == VB2_BUF_STATE_ACTIVE)
				vb2_buffer_done(q->bufs[i],
						VB2_BUF_STATE_ERROR);

		WARN_ON(atomic_read(&q->owned_by_drv_count));
	}
}

/**
 * buf_queue() - Enqueue buffer to a driver.
 * @b:		Pointer to Vb2 buffer structure.
 *
 * Vb2 calls this function to enqueue a buffer to a driver.
 * A driver should later return a buffer to Vb2 with vb2_buffer_done().
 *
 * Return: 0 in case of success, error code otherwise.
 */
static void buf_queue(struct vb2_buffer *b)
{
	struct vb2_queue *q = b->vb2_queue;
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
    struct mvx_session_port *port = vport->port;
	enum mvx_direction dir = vport->dir;
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	struct mvx_v4l2_buffer *vbuf = vb2_to_mvx_v4l2_buffer(b);

	int ret;

	MVX_SESSION_VERBOSE(session,
			    "v4l2: Queue buffer. b=%p, type=%u, index=%u.",
			    b, b->type, b->index);
    vbuf->buf.format = vport->port->format;
	ret = mvx_v4l2_buffer_set(vbuf, b);
	if (ret != 0) {
		goto failed;
    }
	ret = mvx_session_qbuf(&vsession->session, dir, &vbuf->buf);
	if (ret != 0) {
		goto failed;
    }
	return;

failed:
    if (vbuf->buf.flags & MVX_BUFFER_FRAME_NEED_REALLOC) {
        vbuf->vb2_v4l2_buffer.flags |= V4L2_BUF_FLAG_MVX_BUFFER_NEED_REALLOC;
        port->isreallocting = true;
        vb2_buffer_done(b, VB2_BUF_STATE_DONE);
        return;
    }
	vb2_buffer_done(b, VB2_BUF_STATE_ERROR);
}

/**
 * buf_finish() - Finish buffer before it is returned to user space.
 * @vb:		Pointer to Vb2 buffer structure.
 */
static void buf_finish(struct vb2_buffer *vb)
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(vb->vb2_queue);
	struct mvx_v4l2_buffer *vbuf = vb2_to_mvx_v4l2_buffer(vb);

	vport->crop.left = vbuf->buf.crop_left;
	vport->crop.top = vbuf->buf.crop_top;
}

/**
 * wait_prepare() - Prepare driver for waiting
 * @q:		Pointer to Vb2 queue.
 *
 * Vb2 calls this function when it is about to wait for more buffers to
 * be received. A driver should release any locks taken while calling Vb2
 * functions.
 * This is required to avoid a deadlock.
 *
 * This is unused for now and will be called from Vb2.
 */
static void wait_prepare(struct vb2_queue *q)
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;

	MVX_SESSION_VERBOSE(session, "v4l2: Wait prepare. queue=%p.", q);

	mutex_unlock(&vsession->mutex);
}

/**
 * wait_finish() - Wake up after sleep.
 * @q:		Pointer to Vb2 queue.
 *
 * Require mutexes release before.
 *
 * This is unused for now and will be called from Vb2.
 */
static void wait_finish(struct vb2_queue *q)
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	int ignore;

	MVX_SESSION_VERBOSE(session, "v4l2: Wait finish. queue=%p.", q);

	/*
	 * mutex_lock_interruptible is declared with attribute
	 * warn_unused_result, but we have no way to return a status
	 * from wait_finish().
	 */
	ignore = mutex_lock_interruptible(&vsession->mutex);
}

/**
 * mvx_vb2_ops - Callbacks for Vb2 framework
 * Not all possible callbacks are implemented as some of them are optional.
 */
const struct vb2_ops mvx_vb2_ops = {
	.queue_setup     = queue_setup,
	.buf_init        = buf_init,
	.buf_finish      = buf_finish,
	.buf_cleanup     = buf_cleanup,
	.start_streaming = start_streaming,
	.stop_streaming  = stop_streaming,
	.buf_queue       = buf_queue,
	.wait_prepare    = wait_prepare,
	.wait_finish     = wait_finish
};

/**
 * setup_vb2_queue() - Initialize vb2_queue before it can be used by Vb2.
 */
static int setup_vb2_queue(struct mvx_v4l2_port *vport)
{
	struct vb2_queue *q = &vport->vb2_queue;
#if KERNEL_VERSION(4, 5, 0) <= LINUX_VERSION_CODE
	struct device *dev = vport->vsession->ext->dev;
#endif
	int ret;

	q->drv_priv = vport;
	q->type = vport->type;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
#if KERNEL_VERSION(4, 5, 0) <= LINUX_VERSION_CODE
	q->dev = dev;
#endif
	q->ops = &mvx_vb2_ops;
	q->mem_ops = &vb2_dma_sg_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	q->allow_zero_bytesused = true;

	/* Let Vb2 handle mvx_v4l2_buffer allocations. */
	q->buf_struct_size = sizeof(struct mvx_v4l2_buffer);

	ret = vb2_queue_init(q);

	return ret;
}

/****************************************************************************
 * Exported functions and variables
 ****************************************************************************/

int mvx_v4l2_vidioc_querycap(struct file *file,
			     void *fh,
			     struct v4l2_capability *cap)
{
	struct mvx_v4l2_session *session = file_to_session(file);

	MVX_SESSION_INFO(&session->session, "v4l2: Query capabilities.");

	strlcpy(cap->driver, "mvx", sizeof(cap->driver));
	strlcpy(cap->card, "Linlon Video device", sizeof(cap->card));
	strlcpy(cap->bus_info, "platform:mvx", sizeof(cap->bus_info));

	cap->capabilities = V4L2_CAP_DEVICE_CAPS |
			    V4L2_CAP_VIDEO_M2M |
			    V4L2_CAP_VIDEO_M2M_MPLANE |
			    V4L2_CAP_EXT_PIX_FORMAT |
			    V4L2_CAP_STREAMING;
	cap->device_caps = cap->capabilities & ~V4L2_CAP_DEVICE_CAPS;

	return 0;
}

/*
 * Loop over the mvx_fmts searching for pixelformat at offset f->index.
 *
 * Formats that are not present in the 'formats' bitmask will be skipped.
 * Which pixelformat that is mapped to which index will consequently depend
 * on which mvx_formats that are enabled.
 */
static int mvx_v4l2_vidioc_enum_fmt_vid(struct mvx_v4l2_session *session,
					struct v4l2_fmtdesc *f,
					enum mvx_direction dir)
{
	uint64_t formats;
	int index;
	int i;

	mvx_session_get_formats(&session->session, dir, &formats);

	for (i = 0, index = 0; i < ARRAY_SIZE(mvx_fmts); i++)
		if (mvx_test_bit(mvx_fmts[i].format, &formats)) {
			if (f->index == index) {
				f->flags = mvx_fmts[i].flags;
				f->pixelformat = mvx_fmts[i].pixelformat;
				strlcpy(f->description, mvx_fmts[i].description,
					sizeof(f->description));
				break;
			}

			index++;
		}

	if (i >= ARRAY_SIZE(mvx_fmts))
		return -EINVAL;

	return 0;
}

int mvx_v4l2_vidioc_enum_fmt_vid_cap(struct file *file,
				     void *fh,
				     struct v4l2_fmtdesc *f)
{
	struct mvx_v4l2_session *session = file_to_session(file);
	int ret;

	ret = mvx_v4l2_vidioc_enum_fmt_vid(session, f, MVX_DIR_OUTPUT);

	return ret;
}

int mvx_v4l2_vidioc_enum_fmt_vid_out(struct file *file,
				     void *fh,
				     struct v4l2_fmtdesc *f)
{
	struct mvx_v4l2_session *session = file_to_session(file);
	int ret;

	ret = mvx_v4l2_vidioc_enum_fmt_vid(session, f, MVX_DIR_INPUT);

	return ret;
}

int mvx_v4l2_vidioc_enum_framesizes(struct file *file,
				    void *fh,
				    struct v4l2_frmsizeenum *fsize)
{
	struct mvx_format_map *format;

	/* Verify that format is supported. */
	format = mvx_find_format(fsize->pixel_format);
	if (IS_ERR(format))
		return PTR_ERR(format);

	/* For stepwise/continuous frame size the index must be 0. */
	if (fsize->index != 0)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise.min_width = 2;
	fsize->stepwise.max_width = 8192;
	fsize->stepwise.step_width = 2;
	fsize->stepwise.min_height = 2;
	fsize->stepwise.max_height = 8192;
	fsize->stepwise.step_height = 2;

	return 0;
}

static int mvx_v4l2_vidioc_g_fmt_vid(struct file *file,
				     struct v4l2_format *f,
				     enum mvx_direction dir)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	struct mvx_session_port *port = &vsession->session.port[dir];
	int ret;

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	to_v4l2_format(f, f->type, &vport->pix_mp, port->stride, port->size,
		       port->interlaced);

	mutex_unlock(&vsession->mutex);

	print_format(&vsession->session, f, "Get format");

	return 0;
}

int mvx_v4l2_vidioc_g_fmt_vid_cap(struct file *file,
				  void *fh,
				  struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_g_fmt_vid(file, f, MVX_DIR_OUTPUT);
}

int mvx_v4l2_vidioc_g_fmt_vid_out(struct file *file,
				  void *fh,
				  struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_g_fmt_vid(file, f, MVX_DIR_INPUT);
}

static int mvx_v4l2_vidioc_s_fmt_vid(struct file *file,
				     struct v4l2_format *f,
				     enum mvx_direction dir)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	struct v4l2_pix_format_mplane pix_mp;
	enum mvx_format format;
	unsigned int stride[MVX_BUFFER_NPLANES];
	unsigned int size[MVX_BUFFER_NPLANES];
	bool interlaced = false;
	int ret;

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	if (vport->q_set != false && vb2_is_busy(&vport->vb2_queue) != false) {
		MVX_SESSION_WARN(&vsession->session,
				 "Can't set format when there there buffers allocated to the port.");
		ret = -EBUSY;
		goto unlock_mutex;
	}

	/* Convert V4L2 format to V4L2 multi planar pixel format. */
	ret = from_v4l2_format(vsession, f, &pix_mp, &format, stride, size,
			       &interlaced);
	if (ret != 0)
		goto unlock_mutex;

	/* Validate and adjust settings. */
	ret = mvx_session_set_format(&vsession->session, dir, format,
				     &pix_mp.width, &pix_mp.height,
				     &pix_mp.num_planes,
				     stride, size, &interlaced);
	if (ret != 0)
		goto unlock_mutex;

	/* Convert V4L2 multi planar pixel format to format. */
	ret = to_v4l2_format(f, f->type, &pix_mp, stride, size, interlaced);
	if (ret != 0)
		goto unlock_mutex;

	vport->type = f->type;
	vport->pix_mp = pix_mp;

unlock_mutex:
	mutex_unlock(&vsession->mutex);

	print_format(&vsession->session, f, "Set format");

	return ret;
}

int mvx_v4l2_vidioc_s_fmt_vid_cap(struct file *file,
				  void *fh,
				  struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_s_fmt_vid(file, f, MVX_DIR_OUTPUT);
}

int mvx_v4l2_vidioc_s_fmt_vid_out(struct file *file,
				  void *fh,
				  struct v4l2_format *f)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct v4l2_pix_format_mplane *in =
		&vsession->port[MVX_DIR_INPUT].pix_mp;
	struct v4l2_pix_format_mplane *out =
		&vsession->port[MVX_DIR_OUTPUT].pix_mp;
	int ret;

	ret = mvx_v4l2_vidioc_s_fmt_vid(file, f, MVX_DIR_INPUT);
	if (ret != 0)
		return ret;

	/* Copy input formats to output port. */
	out->colorspace = in->colorspace;
	out->ycbcr_enc = in->ycbcr_enc;
	out->quantization = in->quantization;
	out->xfer_func = in->xfer_func;

	return 0;
}

static int mvx_v4l2_vidioc_try_fmt_vid(struct file *file,
				       struct v4l2_format *f,
				       enum mvx_direction dir)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct v4l2_pix_format_mplane pix;
	enum mvx_format format;
	unsigned int stride[MVX_BUFFER_NPLANES];
	unsigned int size[MVX_BUFFER_NPLANES];
	bool interlaced = false;
	int ret;

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	ret = from_v4l2_format(vsession, f, &pix, &format, stride, size,
			       &interlaced);
	if (ret != 0)
		goto unlock_mutex;

	ret = mvx_session_try_format(&vsession->session, dir, format,
				     &pix.width, &pix.height, &pix.num_planes,
				     stride, size, &interlaced);
	if (ret != 0)
		goto unlock_mutex;

	ret = to_v4l2_format(f, f->type, &pix, stride, size, interlaced);
	if (ret != 0)
		goto unlock_mutex;

unlock_mutex:
	mutex_unlock(&vsession->mutex);

	print_format(&vsession->session, f, "Try format");

	return ret;
}

int mvx_v4l2_vidioc_try_fmt_vid_cap(struct file *file,
				    void *fh,
				    struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_try_fmt_vid(file, f, MVX_DIR_OUTPUT);
}

int mvx_v4l2_vidioc_try_fmt_vid_out(struct file *file,
				    void *fh,
				    struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_try_fmt_vid(file, f, MVX_DIR_INPUT);
}
int mvx_v4l2_vidioc_g_crop(struct file *file,
			   void *fh,
			   struct v4l2_crop *a)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(a->type) ?
				 MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	struct mvx_session_port *port = &vsession->session.port[dir];

	mutex_lock(&vsession->mutex);

	a->c.left = vport->crop.left;
	a->c.top = vport->crop.top;
	a->c.width = port->width - vport->crop.left;
	a->c.height = port->height - vport->crop.top;

	mutex_unlock(&vsession->mutex);

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Get crop. dir=%u, crop={left=%u, top=%u, width=%u, height=%u.",
			 dir, a->c.left, a->c.top, a->c.width, a->c.height);

	return 0;
}
int mvx_v4l2_vidioc_g_selection(struct file *file, void *fh,
				  struct v4l2_selection *s)
{
    struct v4l2_crop crop = { .type = s->type };
    int ret;
    ret = mvx_v4l2_vidioc_g_crop(file, fh, &crop);
    if (ret == 0)
    {
        s->r = crop.c;
    }
    return ret;
}
int mvx_v4l2_vidioc_s_selection(struct file *file, void *fh,
				  struct v4l2_selection *s)
{
    struct mvx_v4l2_session *vsession = file_to_session(file);
    int ret = 0;
    ret = mutex_lock_interruptible(&vsession->mutex);
    if (ret != 0)
        return ret;
    ret = mvx_session_set_crop_left(&vsession->session, s->r.left);
    ret = mvx_session_set_crop_top(&vsession->session, s->r.top);
    mutex_unlock(&vsession->mutex);
    return ret;
}

int mvx_v4l2_vidioc_s_parm(struct file *file, void *fh,
			     struct v4l2_streamparm *a)
{
    struct mvx_v4l2_session *vsession = file_to_session(file);
    int ret = 0;
    ret = mutex_lock_interruptible(&vsession->mutex);
    if (ret != 0)
        return ret;
    if (V4L2_TYPE_IS_OUTPUT(a->type)) {
        int64_t framerate = ((int64_t)a->parm.output.timeperframe.denominator << 16)/a->parm.output.timeperframe.numerator;
        ret = mvx_session_set_frame_rate(&vsession->session, framerate);
    }
    mutex_unlock(&vsession->mutex);
    return ret;
}
int mvx_v4l2_vidioc_g_parm(struct file *file, void *fh,
			     struct v4l2_streamparm *a)
{
    struct mvx_v4l2_session *vsession = file_to_session(file);
    int ret = 0;
    ret = mutex_lock_interruptible(&vsession->mutex);
    if (ret != 0)
       return ret;
    if (V4L2_TYPE_IS_OUTPUT(a->type)) {
        a->parm.output.timeperframe.numerator = 1 << 16;
        a->parm.output.timeperframe.denominator = (int32_t)vsession->session.frame_rate;
    }
    mutex_unlock(&vsession->mutex);
    return ret;
}

int mvx_v4l2_vidioc_streamon(struct file *file,
			     void *priv,
			     enum v4l2_buf_type type)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(type) ?
				 MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	int ret;

	MVX_SESSION_INFO(&vsession->session, "v4l2: Stream on. dir=%u.", dir);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	ret = vb2_streamon(&vsession->port[dir].vb2_queue, type);
	if (ret != 0)
		MVX_SESSION_WARN(&vsession->session,
				 "v4l2: Failed to stream on. dir=%u.", dir);

	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_streamoff(struct file *file,
			      void *priv,
			      enum v4l2_buf_type type)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(type) ?
				 MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	int ret;

	MVX_SESSION_INFO(&vsession->session, "v4l2: Stream off. dir=%u.", dir);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	ret = vb2_streamoff(&vsession->port[dir].vb2_queue, type);
	if (ret != 0)
		MVX_SESSION_WARN(&vsession->session,
				 "v4l2: Failed to stream off. dir=%u.", dir);

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Stream off exit. dir=%u, ret=%d.",
			 dir, ret);

	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_encoder_cmd(struct file *file,
				void *priv,
				struct v4l2_encoder_cmd *cmd)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	int ret;

	MVX_SESSION_INFO(&vsession->session, "v4l2: encoder cmd: %u.",
			 cmd->cmd);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	switch (cmd->cmd) {
	case V4L2_ENC_CMD_STOP:
		ret = mvx_session_send_eos(&vsession->session);
		break;
      case V4L2_ENC_CMD_START:
             /*reset flag for v4l2 core, so that buffers can be queued normal.*/
             vsession->port[1].vb2_queue.last_buffer_dequeued = false;
             ret = 0;
            break;
	default:
		MVX_SESSION_WARN(&vsession->session,
				 "Unsupported command. cmd: %u.", cmd->cmd);
		ret = -EINVAL;
	}

	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_try_encoder_cmd(struct file *file,
				    void *priv,
				    struct v4l2_encoder_cmd *cmd)
{
	switch (cmd->cmd) {
	case V4L2_ENC_CMD_STOP:
       case V4L2_ENC_CMD_START:
		return 0;
	default:
		return -EINVAL;
	}
}

int mvx_v4l2_vidioc_decoder_cmd(struct file *file,
				void *priv,
				struct v4l2_decoder_cmd *cmd)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	int ret;

	MVX_SESSION_INFO(&vsession->session, "v4l2: decoder cmd: %u.",
			 cmd->cmd);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
		ret = mvx_session_send_eos(&vsession->session);
		break;
       case V4L2_DEC_CMD_START:
             /*reset flag for v4l2 core, so that buffers can be queued normal.*/
             vsession->port[1].vb2_queue.last_buffer_dequeued = false;
             ret = 0;
            break;
	default:
		MVX_SESSION_WARN(&vsession->session,
				 "Unsupported command. cmd: %u.", cmd->cmd);
		ret = -EINVAL;
	}

	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_try_decoder_cmd(struct file *file,
				    void *priv,
				    struct v4l2_decoder_cmd *cmd)
{
	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
       case V4L2_DEC_CMD_START:
		return 0;
	default:
		return -EINVAL;
	}
}

int mvx_v4l2_vidioc_reqbufs(struct file *file,
			    void *fh,
			    struct v4l2_requestbuffers *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->type) ?
				 MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Request buffers. dir=%d, type=%u, memory=%u, count=%u.",
			 dir, b->type, b->memory, b->count);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	if (b->count == 0) {
		if (vport->q_set != false) {
			vb2_queue_release(&vport->vb2_queue);
			vport->q_set = false;
		}
	} else {
		if (vport->q_set == false) {
			ret = setup_vb2_queue(vport);
			if (ret != 0)
				goto unlock_mutex;

			vport->q_set = true;
		}

		ret = vb2_reqbufs(&vport->vb2_queue, b);
	}
    vport->port->buffer_allocated = b->count;
unlock_mutex:
	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_create_bufs(struct file *file,
				void *fh,
				struct v4l2_create_buffers *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->format.type) ?
				 MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Create buffers. dir=%d, type=%u, memory=%u, count=%u.",
			 dir, b->format.type, b->memory, b->count);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	if (vport->q_set == false)
		ret = setup_vb2_queue(vport);

	if (ret != 0)
		goto unlock_mutex;

	vport->q_set = true;
#ifndef MODULE
	ret = vb2_create_bufs(&vport->vb2_queue, b);
#endif
    vport->port->buffer_allocated = b->count;
unlock_mutex:
	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_querybuf(struct file *file,
			     void *fh,
			     struct v4l2_buffer *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->type) ?
				 MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Query buffer. dir=%d, type=%u, memory=%u, index=%u.",
			 dir, b->type, b->memory, b->index);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	ret = vb2_querybuf(&vport->vb2_queue, b);
	if (ret != 0)
		goto unlock_mutex;

	/*
	 * When user space wants to mmap() a buffer, we have to be able to
	 * determine a direction of coresponding port. To make it easier we
	 * adjust mem_offset on output port by DST_QUEUE_OFF_BASE for all
	 * buffers.
	 */
	if (dir == MVX_DIR_OUTPUT) {
		if (V4L2_TYPE_IS_MULTIPLANAR(b->type)) {
			int i;

			for (i = 0; i < b->length; ++i)
				b->m.planes[i].m.mem_offset +=
					DST_QUEUE_OFF_BASE;
		} else {
			b->m.offset += DST_QUEUE_OFF_BASE;
		}
	}

unlock_mutex:
	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_qbuf(struct file *file,
			 void *fh,
			 struct v4l2_buffer *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->type) ?
				 MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
    struct mvx_v4l2_buffer *vbuf;
    struct mvx_buffer *buf;
    struct vb2_buffer *vb;
    struct v4l2_core_buffer_header_general *v4l2_general;
	int ret;

	MVX_SESSION_INFO(&vsession->session,
            "v4l2: Queue buffer. dir=%d, type=%u, index=%u, flags=0x%x.",
            dir, b->type, b->index, b->flags);

	mutex_lock(&vsession->mutex);

    if ((b->flags & V4L2_BUF_FLAG_MVX_BUFFER_EPR) == V4L2_BUF_FLAG_MVX_BUFFER_EPR ){
        vb = vport->vb2_queue.bufs[b->index];
        vbuf = vb2_to_mvx_v4l2_buffer(vb);
        buf = &vbuf->buf;
        v4l2_general = (struct v4l2_core_buffer_header_general *)&b->m.planes[0].reserved[0];
        buf->general.header.buffer_size = v4l2_general->buffer_size;
        buf->general.header.config_size = v4l2_general->config_size;
        buf->general.header.type = v4l2_general->type;

        memcpy(&buf->general.config.block_configs, &v4l2_general->config, sizeof(v4l2_general->config));
        MVX_SESSION_INFO(&vsession->session,
            "v4l2: Queue buffer. type:%d, config size:%d, buffer size:%d, cfg_type:0x%x, cols and rows:%d, %d",
                v4l2_general->type ,v4l2_general->config_size, v4l2_general->buffer_size,
                v4l2_general->config.blk_cfg_type,v4l2_general->config.blk_cfgs.rows_uncomp.n_cols_minus1,
                v4l2_general->config.blk_cfgs.rows_uncomp.n_rows_minus1);
    }

    if (dir == MVX_DIR_INPUT && V4L2_TYPE_IS_MULTIPLANAR(b->type)) {
        vb = vport->vb2_queue.bufs[b->index];
        vbuf = vb2_to_mvx_v4l2_buffer(vb);
        buf = &vbuf->buf;

        buf->flags = 0;
        if ((b->reserved2 & V4L2_BUF_FRAME_FLAG_ROTATION_90) == V4L2_BUF_FRAME_FLAG_ROTATION_90) {
            buf->flags |= MVX_BUFFER_FRAME_FLAG_ROTATION_90;
        }
        if ((b->reserved2 & V4L2_BUF_FRAME_FLAG_ROTATION_180) == V4L2_BUF_FRAME_FLAG_ROTATION_180) {
           buf->flags |= MVX_BUFFER_FRAME_FLAG_ROTATION_180;
        }
        if ((b->reserved2 & V4L2_BUF_FRAME_FLAG_ROTATION_270) == V4L2_BUF_FRAME_FLAG_ROTATION_270) {
            buf->flags |= MVX_BUFFER_FRAME_FLAG_ROTATION_270;
        }
    }
	ret = vb2_qbuf(&vport->vb2_queue, NULL, b);
	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_dqbuf(struct file *file,
			  void *fh,
			  struct v4l2_buffer *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct mvx_ext_if *ctx = vsession->ext;
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->type) ?
				 MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	struct vb2_buffer *vb;
	struct mvx_v4l2_buffer *vbuf;
       struct mvx_buffer *buf;
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Dequeue buffer. dir=%d, type=%u.",
			 dir, b->type);

	mutex_lock(&vsession->mutex);

	ret = vb2_dqbuf(&vport->vb2_queue, b, file->f_flags & O_NONBLOCK);
	if (ret != 0)
		goto unlock_mutex;

	if ((dir == MVX_DIR_OUTPUT) && (b->flags & V4L2_BUF_FLAG_LAST)) {
		const struct v4l2_event event = {
			.type = V4L2_EVENT_EOS
		};
		v4l2_event_queue(&ctx->vdev, &event);
	}

	/*
	 * For single planar buffers there is no data offset. Instead the
	 * offset is added to the memory pointer and subtraced from the
	 * bytesused.
	 */
	vb = vport->vb2_queue.bufs[b->index];
	if (V4L2_TYPE_IS_MULTIPLANAR(vb->type) == false) {
		b->bytesused -= vb->planes[0].data_offset;

		switch (vb->type) {
		case V4L2_MEMORY_MMAP:
			b->m.offset += vb->planes[0].data_offset;
			break;
		case V4L2_MEMORY_USERPTR:
			b->m.userptr += vb->planes[0].data_offset;
			break;
		default:
			break;
		}
	}

    if (vsession->port[MVX_DIR_INPUT].port->format <= MVX_FORMAT_BITSTREAM_LAST &&
            dir == MVX_DIR_OUTPUT && V4L2_TYPE_IS_MULTIPLANAR(b->type)) {
        vbuf = vb2_to_mvx_v4l2_buffer(vb);
        buf = &vbuf->buf;
        b->reserved2 = (buf->width << 16) | (buf->height);
    }

unlock_mutex:
	mutex_unlock(&vsession->mutex);

#ifndef MODULE
	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Dequeued buffer. dir=%d, type=%u, index=%u, flags=0x%x, nevents=%u, fh=%p.",
			 dir, b->type, b->index, b->flags,
			 v4l2_event_pending(&vsession->fh), fh);
#else
	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Dequeued buffer. dir=%d, type=%u, index=%u, flags=0x%x, fh=%p.",
			 dir, b->type, b->index, b->flags,
			 fh);
#endif

	return ret;
}

int mvx_v4l2_vidioc_subscribe_event(struct v4l2_fh *fh,
				    const struct v4l2_event_subscription *sub)
{
	struct mvx_v4l2_session *session = v4l2_fh_to_session(fh);

	MVX_SESSION_INFO(&session->session,
			 "v4l2: Subscribe event. fh=%p, type=%u.", fh,
			 sub->type);

	switch (sub->type) {
	case V4L2_EVENT_CTRL:
		return v4l2_ctrl_subscribe_event(fh, sub);
	case V4L2_EVENT_EOS:
	case V4L2_EVENT_SOURCE_CHANGE:
	case V4L2_EVENT_MVX_COLOR_DESC:
		return v4l2_event_subscribe(fh, sub, 2, NULL);
	default:
		MVX_SESSION_WARN(&session->session,
				 "Can't register for unsupported event. type=%u.",
				 sub->type);
		return -EINVAL;
	}

	return 0;
}

long mvx_v4l2_vidioc_default(struct file *file,
			     void *fh,
			     bool valid_prio,
			     unsigned int cmd,
			     void *arg)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	int ret;
	MVX_SESSION_INFO(&vsession->session,
			 "Custom ioctl. cmd=0x%x, arg=0x%p.", cmd, arg);

	switch (cmd) {
	case VIDIOC_G_MVX_COLORDESC: {
		ret = mvx_v4l2_session_get_color_desc(vsession, arg);
		break;
	}
    case VIDIOC_S_MVX_ROI_REGIONS: {
        ret = mvx_v4l2_session_set_roi_regions(vsession, arg);
        break;
    }
    case VIDIOC_S_MVX_QP_EPR: {
        ret = mvx_v4l2_session_set_qp_epr(vsession, arg);
        break;
    }
    case VIDIOC_S_MVX_COLORDESC: {
        ret = mvx_v4l2_session_set_color_desc(vsession, arg);
        break;
    }
    case VIDIOC_S_MVX_SEI_USERDATA: {
        ret = mvx_v4l2_session_set_sei_userdata(vsession, arg);
        break;
    }
    case VIDIOC_S_MVX_RATE_CONTROL: {
        ret = mvx_v4l2_session_set_rate_control(vsession, arg);
        break;
    }
    case VIDIOC_S_MVX_DSL_FRAME: {
        ret = mvx_v4l2_session_set_dsl_frame(vsession, arg);
        break;
    }
    case VIDIOC_S_MVX_DSL_RATIO: {
        ret = mvx_v4l2_session_set_dsl_ratio(vsession, arg);
        break;
    }
    case VIDIOC_S_MVX_LONG_TERM_REF: {
        ret = mvx_v4l2_session_set_long_term_ref(vsession, arg);
        break;
    }
    case VIDIOC_S_MVX_DSL_MODE: {
        ret = mvx_v4l2_session_set_dsl_mode(vsession, arg);
        break;
    }
	default:
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Unsupported IOCTL. cmd=0x%x", cmd);
		return -ENOTTY;
	}

	return ret;
}

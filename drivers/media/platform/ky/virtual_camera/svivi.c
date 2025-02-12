#include <linux/init.h>
#include <linux/module.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <linux/videodev2.h>
#include <media/videobuf2-vmalloc.h>
#include <media/videobuf2-v4l2.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/kmod.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/netlink.h>

#include "vcam_dbg.h"

/* The minimum image width/height */
#define MIN_WIDTH  48
#define MIN_HEIGHT 32

#define MAX_WIDTH 1920
#define MAX_HEIGHT 1200
#define SVIVI_MAX_PLANES			3
#define SVIVI_MIN_WIDTH				480U
#define SVIVI_MIN_HEIGHT			288U
#define SVIVI_DEF_COLOR_SPACE		V4L2_COLORSPACE_SRGB
#define SVIVI_DEF_YCBCR_ENC		V4L2_YCBCR_ENC_601
#define SVIVI_DEF_QUANTIZATION	V4L2_QUANTIZATION_LIM_RANGE
#define SVIVI_DEF_XFER_FUNC		V4L2_XFER_FUNC_SRGB

#define SVIVI_NETLINK    17
enum V4L2_PIPE_SEQ_ID {
    START_INIT = 0,
    FINISH_INIT,
    START_OPEN,
    FINISH_OPEN,
    START_QUERYCAP,
    FINISH_QUERYCAP,
    START_S_FMT,
    FINISH_S_FMT,
    START_S_PARM,
    FINISH_S_PARM,
    START_REQBUFS,
    FINISH_REQBUFS,
    START_QUERYBUF,
    FINISH_QUERYBUF,
    START_MMAP,
    FINISH_MMAP,
    START_QBUF,
    FINISH_QBUF,
    START_STREAMON,
    FINISH_STREAMON,
    START_POLL,
    FINISH_POLL,
    START_DQBUF,
    FINISH_DQBUF,
    START_STREAMOFF,
    FINISH_STREAMOFF,
    START_CLOSE,
    FINISH_CLOSE,
    MAX_SEQ_SIZE,
};
struct v4l2_vinit {
	int 			ret;
};
struct v4l2_vformat {
	int 			ret;
	int 			v4l2_buf_type;
	unsigned int	width;
	unsigned int	height;
	unsigned int	pixelformat;
};
struct v4l2_vrequestbuffers {
	int 			ret;
	unsigned int	count;
};
struct v4l2_vbuffer {
	int 			ret;
	int             index;
	unsigned int	length;
	unsigned int	m_offset;
	int 			m_fd;
};
struct v4l2_vstream {
	int 			ret;
};
struct v4l2_vpoll {
	int 			ret;
};
struct vcam_header {
	unsigned int	kpos;
	unsigned int	upos;
	unsigned int	user_pid;

	struct v4l2_vinit vinit;
	struct v4l2_vformat vfmt;
	struct v4l2_vrequestbuffers vreq_buf;
	struct v4l2_vbuffer vbuf;
	struct v4l2_vstream vstream;
	struct v4l2_vpoll vpoll;

	char pbuf[64];
};

struct recevice_message {
	struct completion 		complete;
	struct vcam_header 		rcv_vheader;
};
static struct recevice_message recv_msg;
static int in_use = false;

struct send_message {
	struct vcam_header  	snd_vheader;
};
static struct send_message   snd_msg;

struct svivi_fmt_info {
	u32	mbus_code;
	u32	fourcc;
	// enum mxc_isi_video_type type;
	// u32	isi_in_format;
	// u32	isi_out_format;
	u8	mem_planes;
	u8	color_planes;
	u8	depth[SVIVI_MAX_PLANES];
	u8	hsub;
	u8	vsub;
};

struct svivi_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head	list;
};

struct vivi {
	struct v4l2_device 		v4l2_dev;
	struct video_device 	vid_cap_dev;
	struct vb2_queue 		vb_vid_cap_q;

	struct v4l2_pix_format_mplane	pix;
	u32 					vid_cap_caps;
	struct mutex 			mutex;

	struct v4l2_rect		fmt_cap_rect;

	int 					streaming;
	struct sock 			*netlinkfd;
	u32 					memory;
};

static const struct svivi_fmt_info svivi_formats[] = {
	/* YUV formats */
	{
		.mbus_code	= MEDIA_BUS_FMT_YUV8_1X24,
		.fourcc		= V4L2_PIX_FMT_NV12,
		.color_planes	= 2,
		.mem_planes	= 1,
		.depth		= { 8, 16 },
		.hsub		= 2,
		.vsub		= 2,
	},
	/* RGB formats */
	{
		.mbus_code	= MEDIA_BUS_FMT_RGB888_1X24,
		.fourcc		= V4L2_PIX_FMT_RGB565,
		.mem_planes	= 1,
		.color_planes	= 1,
		.depth		= { 16 },
	}, {
		.mbus_code	= MEDIA_BUS_FMT_RGB888_1X24,
		.fourcc		= V4L2_PIX_FMT_BGR24,
		.mem_planes	= 1,
		.color_planes	= 1,
		.depth		= { 24 },
	},

};

int send_to_user(struct sock *netlinkfd, void *data, unsigned int len, unsigned int pid, unsigned int seq)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;
    int ret;

    nl_skb = nlmsg_new(len, GFP_ATOMIC);
    if (!nl_skb) {
        vcam_err("netlink alloc failure\n");
        return -1;
    }

    nlh = nlmsg_put(nl_skb, 0, seq, SVIVI_NETLINK, len, 0);
    if (nlh == NULL) {
        vcam_err("nlmsg_put failaure \n");
        nlmsg_free(nl_skb);
        return -1;
    }

    memcpy(nlmsg_data(nlh), data, len);
	vcam_info("send to user len:%d, seq:%d\n", nlh->nlmsg_len, nlh->nlmsg_seq);
	ret = netlink_unicast(netlinkfd, nl_skb, pid, MSG_DONTWAIT);
	if (ret < 0) {
		vcam_err("send to user error:%d, len:%d, seq:%d\n", ret, nlh->nlmsg_len, nlh->nlmsg_seq);
	} else {
		// vcam_info("send to user ret:%d, len:%d, seq:%d\n", ret, nlh->nlmsg_len, nlh->nlmsg_seq);
	}

    return ret;
}
int fill_recv_msg_by_nlmsg_data (struct nlmsghdr *nlh)
{
    char *data = NULL;

	if (in_use == false) {
		vcam_warn("other pid:%d != %d, don't respond netlink:%d\n",
			 recv_msg.rcv_vheader.user_pid, nlh->nlmsg_pid, SVIVI_NETLINK);
		return -1;
	}

	if (recv_msg.rcv_vheader.user_pid != 0) {
		if (recv_msg.rcv_vheader.user_pid != nlh->nlmsg_pid) {
			vcam_warn("recv other pid:%d != %d!!\n", recv_msg.rcv_vheader.user_pid, nlh->nlmsg_pid);
			return -1;
		}
	}

	data = NLMSG_DATA(nlh);
	if (data) {
		if (nlh->nlmsg_seq - 1 != snd_msg.snd_vheader.kpos) {
			vcam_warn("user seq %d - 1 != kernel seq %d\n", nlh->nlmsg_seq, snd_msg.snd_vheader.kpos);
			return -1;
		}

		recv_msg.rcv_vheader.user_pid = nlh->nlmsg_pid;
		recv_msg.rcv_vheader.upos = nlh->nlmsg_seq;
		switch (recv_msg.rcv_vheader.upos) {
			case FINISH_INIT:
				memcpy(&recv_msg.rcv_vheader.vinit, data, sizeof(struct v4l2_vinit));
			break;
			case FINISH_S_FMT:
				memcpy(&recv_msg.rcv_vheader.vfmt, data, sizeof(struct v4l2_vformat));
			break;
			case FINISH_REQBUFS:
				memcpy(&recv_msg.rcv_vheader.vreq_buf, data, sizeof(struct v4l2_vrequestbuffers));
			break;
			case FINISH_QUERYBUF:
				memcpy(&recv_msg.rcv_vheader.vbuf, data, sizeof(struct v4l2_vbuffer));
			break;
			case FINISH_QBUF:
				memcpy(&recv_msg.rcv_vheader.vbuf, data, sizeof(struct v4l2_vbuffer));
			break;
			case FINISH_DQBUF:
				memcpy(&recv_msg.rcv_vheader.vbuf, data, sizeof(struct v4l2_vbuffer));
			break;
			case FINISH_STREAMON:
				memcpy(&recv_msg.rcv_vheader.vstream, data, sizeof(struct v4l2_vstream));
			break;
			case FINISH_POLL:
				memcpy(&recv_msg.rcv_vheader.vpoll, data, sizeof(struct v4l2_vpoll));
			break;
			case FINISH_STREAMOFF:
				memcpy(&recv_msg.rcv_vheader.vstream, data, sizeof(struct v4l2_vstream));
			break;
			default:
				vcam_warn("user seq %d - 1 != kernel seq %d\n", nlh->nlmsg_seq, snd_msg.snd_vheader.kpos);
				return -1;
		}
		vcam_info("recv from user pid %d, upos %d\n", recv_msg.rcv_vheader.user_pid, recv_msg.rcv_vheader.upos);
		return 0;
	}

	return -1;
}
static void netlink_rcv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh = NULL;
    char *data = NULL;
	int ret;

    nlh = nlmsg_hdr(skb);

    if(skb->len >= NLMSG_SPACE(0)) {
        data = NLMSG_DATA(nlh);
        if (data) {
			ret = fill_recv_msg_by_nlmsg_data (nlh);
			if (ret < 0) {
				vcam_warn("unexpected resv happen!\n");
				return;
			}
			complete(&recv_msg.complete);
        }
    } else {
        vcam_err("error skb, length:%d\n", skb->len);
    }
}

static int wait_for_recv_complete(int seq, int timeout) {
	int ret = 0;

	if (!wait_for_completion_timeout(&recv_msg.complete, msecs_to_jiffies(timeout))) {
		vcam_err("timeout!! recv user ack failed, seq:%d", seq);
		return -EAGAIN;
	}
	if (recv_msg.rcv_vheader.upos != seq) {
		vcam_err("recv user ack state error, seq %d != %d", recv_msg.rcv_vheader.upos, seq);
		return -EINVAL;
	}

	switch (recv_msg.rcv_vheader.upos) {
		case FINISH_INIT:
			ret = recv_msg.rcv_vheader.vinit.ret;
		break;
		case FINISH_S_FMT:
			ret = recv_msg.rcv_vheader.vfmt.ret;
		break;
		case FINISH_REQBUFS:
			ret = recv_msg.rcv_vheader.vreq_buf.ret;
		break;
		case FINISH_QUERYBUF:
			ret = recv_msg.rcv_vheader.vbuf.ret;
		break;
		case FINISH_QBUF:
			ret = recv_msg.rcv_vheader.vbuf.ret;
		break;
		case FINISH_DQBUF:
			ret = recv_msg.rcv_vheader.vbuf.ret;
		break;
		case FINISH_STREAMON:
			ret = recv_msg.rcv_vheader.vstream.ret;
		break;
		case FINISH_POLL:
			ret = recv_msg.rcv_vheader.vpoll.ret;
		break;
		case FINISH_STREAMOFF:
			ret = recv_msg.rcv_vheader.vstream.ret;
		break;
		default:
			vcam_warn("unexpected resv happen!\n");
			return -1;
	}

	if (ret)
		vcam_err("user ack but run failed, ret:%d, seq:%d\n", ret, recv_msg.rcv_vheader.upos);
	reinit_completion(&recv_msg.complete);

	return ret;
}

static struct netlink_kernel_cfg cfg = {
    .input  = netlink_rcv_msg,
    .groups = 0,
    .flags = 0,
    .cb_mutex = NULL,
    .bind = NULL,
};

static int call_cam_script(void)
{
	int ret = 0;
	static char cmd_path[] = "/bin/bash";
	static char *cmd_argv[] = {
		cmd_path,
		"-c",
		"/usr/bin/cam-test /root/svivi_cam.json > /tmp/svivi_cam.log 2>&1",
		NULL,
	};
	static char *cmd_envp[] = {
		"HOME=/root",
		"PATH=/sbin:/bin:/usr/sbin:/usr/bin",
		NULL,
	};

	ret = call_usermodehelper(cmd_path, cmd_argv, cmd_envp, UMH_NO_WAIT);

	vcam_info("call cam-test! call_usermodehelper ret: %d\n", ret);

	return ret;
}

static int svivi_querycap(struct file *file,void *priv,
					struct v4l2_capability *cap) {
	struct vivi *vind = video_drvdata(file);

	strcpy(cap->driver, "ky vivi");
	strcpy(cap->card, "ky vivi");
	snprintf(cap->bus_info, sizeof(cap->bus_info),
			"platform:%s", vind->v4l2_dev.name);

	cap->capabilities = vind->vid_cap_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int svivi_enum_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_fmtdesc *f){
	const struct svivi_fmt_info *fmt;

	vcam_info("f->index=%d, pid:%d", f->index, recv_msg.rcv_vheader.user_pid);

	if (f->index >= ARRAY_SIZE(svivi_formats))
		return -EINVAL;

	fmt = &svivi_formats[f->index];

	f->pixelformat = fmt->fourcc;
	return 0;
}

static int svivi_g_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f)
{
	struct vivi *vind = video_drvdata(file);

	vcam_info("----------- in");

	f->fmt.pix_mp = vind->pix;

	// vcam_info("width=%d,height=%d,pixelformat=%c%c%c%c,field=%d,colorspace=%d,bytesperline=%d,sizeimage=%d\n",
	// 		pix->width,pix->height,pix->pixelformat&0xff,(pix->pixelformat>>8)&0xff,
	// 		(pix->pixelformat>>16)&0xff,(pix->pixelformat>>24)&0xff, pix->field, pix->colorspace, pix->bytesperline, pix->sizeimage
	// );

	return 0;
}

static const struct svivi_fmt_info *svivi_format_by_fourcc(u32 fourcc)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(svivi_formats); i++) {
		const struct svivi_fmt_info *fmt = &svivi_formats[i];

		if (fmt->fourcc == fourcc)
			return fmt;
	}

	return NULL;
}

const struct svivi_fmt_info *svivi_format_try(struct v4l2_pix_format_mplane *pix)
{
	const struct svivi_fmt_info *fmt;
	unsigned int i;

	fmt = svivi_format_by_fourcc(pix->pixelformat);
	if (!fmt)
		fmt = &svivi_formats[0];

	pix->width = clamp(pix->width, SVIVI_MIN_WIDTH, 1920);
	pix->height = clamp(pix->height, SVIVI_MIN_HEIGHT, 1080);
	pix->pixelformat = fmt->fourcc;
	pix->field = V4L2_FIELD_NONE;

	if (pix->colorspace == V4L2_COLORSPACE_DEFAULT) {
		pix->colorspace = SVIVI_DEF_COLOR_SPACE;
		pix->ycbcr_enc = SVIVI_DEF_YCBCR_ENC;
		pix->quantization = SVIVI_DEF_QUANTIZATION;
		pix->xfer_func = SVIVI_DEF_XFER_FUNC;
	}
	if (pix->ycbcr_enc == V4L2_YCBCR_ENC_DEFAULT)
		pix->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(pix->colorspace);
	if (pix->quantization == V4L2_QUANTIZATION_DEFAULT) {
		bool is_rgb = 0;

		pix->quantization =
			V4L2_MAP_QUANTIZATION_DEFAULT(is_rgb, pix->colorspace,
						      pix->ycbcr_enc);
	}
	if (pix->xfer_func == V4L2_XFER_FUNC_DEFAULT)
		pix->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(pix->colorspace);
	if (pix->quantization == V4L2_QUANTIZATION_DEFAULT) {
		bool is_rgb = 0;

		pix->quantization =
			V4L2_MAP_QUANTIZATION_DEFAULT(is_rgb, pix->colorspace,
						      pix->ycbcr_enc);
	}
	if (pix->xfer_func == V4L2_XFER_FUNC_DEFAULT)
		pix->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(pix->colorspace);

	pix->num_planes = fmt->color_planes;
	for (i = 0; i < fmt->color_planes; ++i) {
		struct v4l2_plane_pix_format *plane = &pix->plane_fmt[i];
		unsigned int bpl;

		/* The pitch must be identical for all planes. */
		if (i == 0)
			bpl = clamp(plane->bytesperline,
				    pix->width * fmt->depth[0] / 8,
				    65535U);
		else
			bpl = pix->plane_fmt[0].bytesperline;

		plane->bytesperline = bpl;

		plane->sizeimage = plane->bytesperline * pix->height;
		if (i >= 1)
			plane->sizeimage /= fmt->vsub;
	}

	return fmt;
}

static int svivi_try_fmt_vid_cap(struct file *file,void *priv,
			struct v4l2_format *f) {
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;

	vcam_info("----- in");

	svivi_format_try(pix);

	return 0;
}

//refer to mxc_isi_video_s_fmt
static int svivi_s_fmt_vid_cap(struct file *file, void *priv,
					struct v4l2_format *f) {
	struct v4l2_pix_format_mplane *pix = &f->fmt.pix_mp;
	struct vivi *vind = video_drvdata(file);
	struct vcam_header *snd_vheader = &snd_msg.snd_vheader;
	int ret = 0;

	vcam_info("-------------- in\n");

	//todo: check streaming capture is active?

	call_cam_script();

	ret = wait_for_recv_complete(FINISH_INIT, 1000);
	if (ret)
		return ret;

	svivi_format_try(pix);

	// char *kmsg = "start to set fmt !!!";

	// memcpy(snd_vheader->pbuf, kmsg, strlen(kmsg) + 1);
	snd_vheader->kpos = START_S_FMT;
	snd_vheader->vfmt.width = pix->width;
	snd_vheader->vfmt.height = pix->height;
	snd_vheader->user_pid = recv_msg.rcv_vheader.user_pid;

	send_to_user(vind->netlinkfd, &snd_vheader->vfmt, sizeof(struct v4l2_vformat), snd_vheader->user_pid, snd_vheader->kpos);

	ret = wait_for_recv_complete(FINISH_S_FMT, 1000);
	if (ret)
		return ret;

	vind->pix = *pix;


	vind->fmt_cap_rect.width = pix->width;
	vind->fmt_cap_rect.height = pix->height;

	return 0;
}

static int svivi_g_fbuf(struct file *file, void *fh, struct v4l2_framebuffer *a)
{
	return 0;
}

static int svivi_s_fbuf(struct file *file, void *fh, const struct v4l2_framebuffer *a)
{
	return 0;
}
static int svivi_reqbufs(struct file *file, void *priv,
                          struct v4l2_requestbuffers *p){
	struct vivi *vind = video_drvdata(file);
	struct vcam_header *snd_vheader = &snd_msg.snd_vheader;
	int ret;

	vcam_info("------------ in");

	if (p->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		return -EINVAL;
	}

	if (p->memory == V4L2_MEMORY_MMAP || p->memory == V4L2_MEMORY_DMABUF)
		vind->memory = p->memory;
	else
		return -EINVAL;

	if (p->count != 0) {
		vcam_info("reqbufs buf type: %d, memory: %d", p->type, p->memory);

		// char *kmsg = "start to reqbufs !!!";

		// //need to get user ready msg and send kernel msg
		// memcpy(snd_vheader->pbuf, kmsg, strlen(kmsg) + 1);
		snd_vheader->kpos = START_REQBUFS;
		snd_vheader->user_pid = recv_msg.rcv_vheader.user_pid;
		snd_vheader->vreq_buf.count = 4;
		send_to_user(vind->netlinkfd, &snd_vheader->vreq_buf, sizeof(struct v4l2_vrequestbuffers), snd_vheader->user_pid, snd_vheader->kpos);

		ret = wait_for_recv_complete(FINISH_REQBUFS, 1000);
		if (ret)
			return ret;
	}

	return 0;
	// return vb2_ioctl_reqbufs(file, priv, p);
}
static int svivi_expbuf(struct file *file, void *fh, struct v4l2_exportbuffer *e)
{
	int ret = 0;
	vcam_info(" ------------ in");

	return ret;
}
//refer to __fill_v4l2_buffer
static int svivi_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct vivi *vind = video_drvdata(file);
	struct vcam_header *snd_vheader = &snd_msg.snd_vheader;
	int ret, plane = 0;

	vcam_info("dequeue buf type: %d, memory: %d, %d", p->type, p->memory, vind->pix.plane_fmt[plane].sizeimage);

	// char *kmsg = "start to dequeue !!!";

	// memcpy(snd_vheader->pbuf, kmsg, strlen(kmsg) + 1);
	snd_vheader->kpos = START_DQBUF;
	snd_vheader->user_pid = recv_msg.rcv_vheader.user_pid;
	send_to_user(vind->netlinkfd, &snd_vheader->vbuf, sizeof(struct v4l2_vbuffer), snd_vheader->user_pid, snd_vheader->kpos);

	ret = wait_for_recv_complete(FINISH_DQBUF, 2000);
	if (ret)
		return ret;

	//fill v4l2 buffer
	p->index = recv_msg.rcv_vheader.vbuf.index;
	p->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	p->memory = vind->memory;
	p->flags = V4L2_BUF_FLAG_DONE;;	//TODO
	p->length = vind->pix.num_planes;
	for (plane = 0; plane < vind->pix.num_planes; ++plane) {
		struct v4l2_plane *pdst = &p->m.planes[plane];
		// struct vb2_plane *psrc = &vb->planes[plane];

		pdst->bytesused = vind->pix.plane_fmt[plane].sizeimage;
		pdst->length = vind->pix.plane_fmt[plane].sizeimage;
		if (p->memory == V4L2_MEMORY_MMAP)
			pdst->m.mem_offset = 0;	//allocBuffer 
		else if (p->memory == V4L2_MEMORY_DMABUF)
			pdst->m.fd = recv_msg.rcv_vheader.vbuf.m_fd;
		pdst->data_offset = 0; //todo
		if (plane == 1)
			pdst->data_offset = vind->pix.plane_fmt[0].sizeimage;
		memset(pdst->reserved, 0, sizeof(pdst->reserved));
	}

	return 0;
}
static int svivi_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct vivi *vind = video_drvdata(file);
	struct vcam_header *snd_vheader = &snd_msg.snd_vheader;
	int ret;

	vcam_info("queue buf%d type:%d, memory:%d, fd:%d,%d", p->index, p->type, p->memory, p->m.planes[0].m.fd, p->m.planes[1].m.fd);

	// char *kmsg = "start to queue !!!";

	// memcpy(snd_vheader->pbuf, kmsg, strlen(kmsg) + 1);
	snd_vheader->kpos = START_QBUF;
	snd_vheader->user_pid = recv_msg.rcv_vheader.user_pid;
	send_to_user(vind->netlinkfd, &snd_vheader->vbuf, sizeof(struct v4l2_vbuffer), snd_vheader->user_pid, snd_vheader->kpos);

	ret = wait_for_recv_complete(FINISH_QBUF, 1000);
	if (ret)
		return ret;

	return 0;
	// return vb2_ioctl_qbuf(file, priv, p);
}

//refer to __fill_v4l2_buffer and _fill_dmx_buffer
static int svivi_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
	struct vivi *vind = video_drvdata(file);
	struct vcam_header *snd_vheader = &snd_msg.snd_vheader;
	int ret;
    // char *kmsg = "start to querybuf !!!";

	vcam_info("------------ in");

    // memcpy(snd_vheader->pbuf, kmsg, strlen(kmsg) + 1);
	snd_vheader->kpos = START_QUERYBUF;
	snd_vheader->user_pid = recv_msg.rcv_vheader.user_pid;
	send_to_user(vind->netlinkfd, &snd_vheader->vbuf, sizeof(struct v4l2_vbuffer), snd_vheader->user_pid, snd_vheader->kpos);

	ret = wait_for_recv_complete(FINISH_QUERYBUF, 1000);
	if (ret)
		return ret;

	//todo: check buffer index out of range and check __verify_planes_array

	// struct v4l2_buffer *b = pb;
	// struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	// struct vb2_queue *q = vb->vb2_queue;
	unsigned int plane;

	/* Copy back data such as timestamp, flags, etc. */
	// b->index = vb->index;
	p->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	p->memory = vind->memory;
	p->bytesused = 0;

	p->flags = 0; 	// refer to print
	p->field = vind->pix.field;
	// v4l2_buffer_set_timestamp(b, vb->timestamp);
	// p->timecode = vbuf->timecode;
	// p->sequence = vbuf->sequence;
	p->reserved2 = 0;
	p->request_fd = 0;

	/*
	* Fill in plane-related data if userspace provided an array
	* for it. The caller has already verified memory and size.
	*/
	p->length = vind->pix.num_planes;
	for (plane = 0; plane < vind->pix.num_planes; ++plane) {
		struct v4l2_plane *pdst = &p->m.planes[plane];
		// struct vb2_plane *psrc = &vb->planes[plane];

		pdst->bytesused = vind->pix.plane_fmt[plane].sizeimage;	//__prepare_mmap -> mxc_isi_video_buffer_prepare -> vb2_set_plane_payload
		pdst->length = vind->pix.plane_fmt[plane].sizeimage;	//vb2_core_reqbufs -> __vb2_queue_alloc -> mxc_isi_video_queue_setup
		if (p->memory == V4L2_MEMORY_MMAP)
			pdst->m.mem_offset = 0;	//allocBuffer 
		else if (p->memory == V4L2_MEMORY_DMABUF)
			pdst->m.fd = recv_msg.rcv_vheader.vbuf.m_fd;
		pdst->data_offset = 0; //todo
		if (plane == 1)
			pdst->data_offset = vind->pix.plane_fmt[0].sizeimage;
		memset(pdst->reserved, 0, sizeof(pdst->reserved));
	}

	vcam_info("querybuff exit, index:%d %d fd:%d", p->index, recv_msg.rcv_vheader.vbuf.index, recv_msg.rcv_vheader.vbuf.m_fd);

	// return vb2_ioctl_querybuf(file, priv, p);
	return 0;
}
static int svivi_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct vivi *vind = video_drvdata(file);
	struct vcam_header *snd_vheader = &snd_msg.snd_vheader;
	int ret;
    // char *kmsg = "start to stream on !!!";

    // memcpy(snd_vheader->pbuf, kmsg, strlen(kmsg) + 1);
	snd_vheader->kpos = START_STREAMON;
	snd_vheader->user_pid = recv_msg.rcv_vheader.user_pid;

	send_to_user(vind->netlinkfd, &snd_vheader->vstream, sizeof(struct v4l2_vstream), snd_vheader->user_pid, snd_vheader->kpos);

	ret = wait_for_recv_complete(FINISH_STREAMON, 1000);
	if (ret)
		return ret;

	vind->streaming = 1;

	return 0;
}
static int svivi_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct vivi *vind = video_drvdata(file);
	struct vcam_header *snd_vheader = &snd_msg.snd_vheader;
	int ret = 0;
    // char *kmsg = "start to stream off !!!";
    // memcpy(snd_vheader->pbuf, kmsg, strlen(kmsg) + 1);
	snd_vheader->kpos = START_STREAMOFF;
	snd_vheader->user_pid = recv_msg.rcv_vheader.user_pid;

	send_to_user(vind->netlinkfd, &snd_vheader->vstream, sizeof(struct v4l2_vstream), snd_vheader->user_pid, snd_vheader->kpos);

	ret = wait_for_recv_complete(FINISH_STREAMOFF, 1000);

	vind->streaming = 0;

	return 0;
}

static const struct v4l2_ioctl_ops svivi_ioctl_ops = {
	.vidioc_querycap = svivi_querycap,

	.vidioc_enum_fmt_vid_cap 	= svivi_enum_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap_mplane	= svivi_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap_mplane	= svivi_s_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap_mplane	= svivi_g_fmt_vid_cap,
	// .vidioc_g_fmt_vid_cap 		= svivi_g_fmt_vid_cap,
	// .vidioc_try_fmt_vid_cap 	= svivi_try_fmt_vid_cap,
	// .vidioc_s_fmt_vid_cap 		= svivi_s_fmt_vid_cap,

	//.vidioc_enum_framesizes		= vidioc_enum_framesizes,
	.vidioc_g_fbuf			= svivi_g_fbuf,
	.vidioc_s_fbuf			= svivi_s_fbuf,

	.vidioc_reqbufs 			= svivi_reqbufs,
	.vidioc_querybuf 			= svivi_querybuf,
	.vidioc_expbuf				= svivi_expbuf,
	.vidioc_qbuf 				= svivi_qbuf,
	.vidioc_dqbuf 				= svivi_dqbuf,

	.vidioc_streamon 			= svivi_streamon,
	.vidioc_streamoff 			= svivi_streamoff,
};

static int svivi_release(struct file *file)
{
	struct video_device *vdev = video_devdata(file);

	in_use = false;

	memset(&recv_msg, 0, sizeof(recv_msg));
	memset(&snd_msg, 0, sizeof(snd_msg));
	init_completion(&recv_msg.complete);

	if (vdev->queue)
		return vb2_fop_release(file);
	return v4l2_fh_release(file);
}
static int svivi_open(struct file *filp)
{
	in_use = true;

	return v4l2_fh_open(filp);
}
static __poll_t svivi_poll(struct file *file, poll_table *wait)
{
	int ret;
	struct vivi *vind = video_drvdata(file);
	struct vcam_header *snd_vheader = &snd_msg.snd_vheader;
	// char *kmsg = "start to poll !!!";

	vcam_info("start to poll ...");

	// memcpy(snd_vheader->pbuf, kmsg, strlen(kmsg) + 1);
	snd_vheader->kpos = START_POLL;
	snd_vheader->user_pid = recv_msg.rcv_vheader.user_pid;
	send_to_user(vind->netlinkfd, &snd_vheader->vpoll, sizeof(struct v4l2_vpoll), snd_vheader->user_pid, snd_vheader->kpos);

	ret = wait_for_recv_complete(FINISH_POLL, 2000);

	return POLLIN | POLLRDNORM;
	// return vb2_fop_poll(file, wait);
}
static int svivi_mmap(struct file *file, struct vm_area_struct *vma)
{
	return 0;
	// return vb2_fop_mmap(file, vma);
}

static const struct v4l2_file_operations svivi_fops = {
	.owner			= THIS_MODULE,
	.open           = svivi_open,
	.release        = svivi_release,
	.poll			= svivi_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap           = svivi_mmap,
};

static int vid_cap_queue_setup(struct vb2_queue *vq,
		       unsigned *nbuffers, unsigned *nplanes,
		       unsigned sizes[], struct device *alloc_devs[]){

	return 0;
}

static int vid_cap_buf_prepare(struct vb2_buffer *vb){

	return 0;
}

static void vid_cap_buf_finish(struct vb2_buffer *vb) {

}

static void vid_cap_buf_queue(struct vb2_buffer *vb) {

}

static int vid_cap_start_streaming(struct vb2_queue *vq, unsigned count) {

	return 0;
}

static void vid_cap_stop_streaming(struct vb2_queue *vq) {

}

const struct vb2_ops svivi_vid_cap_qops = {
	.queue_setup		= vid_cap_queue_setup,
	.buf_prepare		= vid_cap_buf_prepare,
	.buf_finish			= vid_cap_buf_finish,
	.buf_queue			= vid_cap_buf_queue,
	.start_streaming	= vid_cap_start_streaming,
	.stop_streaming		= vid_cap_stop_streaming,
};

static void svivi_dev_release(struct v4l2_device *v4l2_dev)
{
	struct vivi *vind = container_of(v4l2_dev, struct vivi, v4l2_dev);

	v4l2_device_unregister(&vind->v4l2_dev);
	kfree(vind);
}

void svivi_video_device_release_empty(struct video_device *vdev) {

}

static int svivi_probe(struct platform_device *pdev) {
	int ret = -1;
	struct vb2_queue *q;
	struct video_device *vfd;
	struct vivi *svivi;

	svivi = kzalloc(sizeof(*svivi), GFP_KERNEL);
	if (!svivi)
		return -ENOMEM;

	platform_set_drvdata(pdev, svivi);

	snprintf(svivi->v4l2_dev.name, sizeof(svivi->v4l2_dev.name),
			"%s-00", "ky vivi");
	ret = v4l2_device_register(&pdev->dev,&svivi->v4l2_dev);
	if (ret < 0) {
		vcam_err("Failed to register v4l2_device: %d", ret);
		goto v4l2_dev_err;
	}
	svivi->v4l2_dev.release = svivi_dev_release;

	svivi->vid_cap_caps = 	V4L2_CAP_VIDEO_CAPTURE_MPLANE | \
							V4L2_CAP_STREAMING;

	mutex_init(&svivi->mutex);

	/* initialize vid_cap queue */
	q = &svivi->vb_vid_cap_q;
	q->type = V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	q->drv_priv = svivi;
	q->buf_struct_size = sizeof(struct svivi_buffer);
	q->ops = &svivi_vid_cap_qops;
	q->mem_ops = &vb2_vmalloc_memops;
	q->lock = &svivi->mutex;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->dev = svivi->v4l2_dev.dev;
	ret = vb2_queue_init(q);
	if (ret)
		goto unreg_dev;

	vfd = &svivi->vid_cap_dev;
	snprintf(vfd->name, sizeof(vfd->name), "svivi-00-vid-cap");
	vfd->fops = &svivi_fops;
	vfd->ioctl_ops = &svivi_ioctl_ops;
	vfd->device_caps = svivi->vid_cap_caps;
	vfd->release = svivi_video_device_release_empty;
	vfd->v4l2_dev = &svivi->v4l2_dev;
	vfd->queue = &svivi->vb_vid_cap_q;
	vfd->lock = &svivi->mutex;
	video_set_drvdata(vfd, svivi);
	ret = video_register_device(vfd, VFL_TYPE_VIDEO, 50);
	if (ret < 0)
		goto unreg_dev;

	memset(&recv_msg, 0, sizeof(recv_msg));
	memset(&snd_msg, 0, sizeof(snd_msg));
	init_completion(&recv_msg.complete);

    svivi->netlinkfd = (struct sock *)netlink_kernel_create(&init_net, SVIVI_NETLINK, &cfg);
    if (svivi->netlinkfd == NULL) {
        vcam_err("can not create a netlink socket");
        goto unreg_dev;
    }

	svivi->streaming = 0;
	vcam_info("svivi probe ok.");

	return ret;

unreg_dev:
	v4l2_device_put(&svivi->v4l2_dev);
v4l2_dev_err:
	kfree(svivi);
	vcam_info("svivi probe fail!");

	return -1;
}

static int svivi_remove(struct platform_device *pdev){
	struct vivi *vind;

	vind = platform_get_drvdata(pdev);
	if (!vind) {
		dev_err(&pdev->dev, "vind is NULL");
		return 0;
	}
	vcam_info("--------- in");

	video_unregister_device(&vind->vid_cap_dev);
	v4l2_device_put(&vind->v4l2_dev);
	//kfree(svivi);

    if (vind->netlinkfd) {
        netlink_kernel_release(vind->netlinkfd);
        vind->netlinkfd = NULL;
    }
    vcam_info("test_netlink_exit!!");

	return 0;
}

static void svivi_pdev_release(struct device *dev)
{
}

static struct platform_device svivi_pdev = {
	.name			= "ky vivi",
	.dev.release	= svivi_pdev_release,
};

static struct platform_driver svivi_pdrv = {
	.probe		= svivi_probe,
	.remove		= svivi_remove,
	.driver		= {
		.name	= "ky vivi",
	},
};

static int __init svivi_init(void)
{
	int ret;

	ret = platform_device_register(&svivi_pdev);
	if (ret)
		return ret;

	ret = platform_driver_register(&svivi_pdrv);
	if (ret)
		platform_device_unregister(&svivi_pdev);

	return ret;
}

static void __exit svivi_exit(void)
{
	platform_driver_unregister(&svivi_pdrv);
	platform_device_unregister(&svivi_pdev);
}

module_init(svivi_init);
module_exit(svivi_exit);
MODULE_LICENSE("GPL");




#ifndef __sunxi_tvd_h__
#define __sunxi_tvd_h__

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-ctrls.h>

#include "../sunxi-tvd/tvd.h"

/* tvd interface */
#define CVBS_INTERFACE   0
#define YPBPRI_INTERFACE 1
#define YPBPRP_INTERFACE 2

/* output resolution */
#define NTSC             0
#define PAL              1

extern int tvd_open_special(int tvd_fd);
extern int tvd_close_special(int tvd_fd);

extern int vidioc_s_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);
extern int vidioc_g_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);

extern int dqbuffer_special(int tvd_fd, struct tvd_buffer **buf);
extern int qbuffer_special(int tvd_fd, struct tvd_buffer *buf);

extern int vidioc_streamon_special(int tvd_fd, enum v4l2_buf_type i);
extern int vidioc_streamoff_special(int tvd_fd, enum v4l2_buf_type i);

extern void tvd_register_buffer_done_callback(void *func);

#if 0
/* tvd interface */
#define CVBS_INTERFACE   0
#define YPBPRI_INTERFACE 1
#define YPBPRP_INTERFACE 2

/* output resolution */
#define NTSC             0
#define PAL              1

typedef enum {
	TVD_PL_YUV420 = 0,
	TVD_MB_YUV420 = 1,
	TVD_PL_YUV422 = 2,
} TVD_FMT_T;

struct tvd_fmt {
	u8              name[32];
	u32           	fourcc;          /* v4l2 format id */
	TVD_FMT_T    	output_fmt;
	int   	        depth;
	u32           	width;
	u32           	height;
	enum v4l2_field field;
	enum v4l2_mbus_pixelcode    bus_pix_code;
	unsigned char   planes_cnt;
};

struct tvd_buffer {
	struct vb2_buffer vb;
	struct list_head list;
	struct tvd_fmt *fmt;
	enum vb2_buffer_state state;
	void *paddr;
	void *vaddr;
};

int tvd_open_special(int tvd_fd);
int tvd_close_special(int tvd_fd);

int vidioc_s_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);
int vidioc_g_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);

int dqbuffer_special(int tvd_fd, struct tvd_buffer **buf);
int qbuffer_special(int tvd_fd, struct tvd_buffer *buf);

#if 1
int vidioc_streamon_special(int tvd_fd, enum v4l2_buf_type i);
int vidioc_streamoff_special(int tvd_fd, enum v4l2_buf_type i);
#else
int vidioc_streamon_special(int tvd_fd, int flag);
int vidioc_streamoff_special(int tvd_fd, int flag);
#endif

void virtual_tvd_register_buffer_done_callback(void *func);
#endif

#endif

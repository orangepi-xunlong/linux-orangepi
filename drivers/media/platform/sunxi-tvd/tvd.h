/*
 * sunxi tvd core header file
 * Author:zengqi
 */
#ifndef __TVD__H__
#define __TVD__H__

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/pm_runtime.h>

#include <media/sunxi_camera.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-ctrls.h>
#include "bsp_tvd.h"

static LIST_HEAD(devlist);

enum hpd_status {
	STATUE_CLOSE = 0,
	STATUE_OPEN  = 1,
};

struct tvd_fmt {
	u8              name[32];
	u32           	fourcc;          /* v4l2 format id */
	TVD_FMT_T    	output_fmt;
	int   	        depth;
	enum v4l2_field field;
	enum v4l2_mbus_pixelcode    bus_pix_code;
	unsigned char   planes_cnt;
};

enum tvd_interface {
	YPBPRI,
	YPBPRP,
};

/* buffer for one video frame */
struct tvd_buffer {
	struct vb2_buffer	vb;
	struct list_head	list;
	struct tvd_fmt		*fmt;
	enum vb2_buffer_state	state;
	void *paddr;
};

struct tvd_dmaqueue {
	struct list_head  active;
	/* Counters to control fps rate */
	int               frame;
	int               ini_jiffies;
};

struct writeback_addr {
	dma_addr_t	y;
	dma_addr_t	c;
};

struct tvd_status {
	int tvd_used;
	int tvd_if;
	int tvd_3d_used;
};

struct tvd_3d_fliter {
	int used;
	int size;
	void *vir_address;
	void *phy_address;
};

struct tvd_dev {
	struct list_head       	devlist;
	struct v4l2_device      v4l2_dev;
	struct v4l2_subdev	*sd;
	struct platform_device	*pdev;
	int			sel;
	int			id;
	int			first_flag;
	spinlock_t              slock;

	/* various device info */
	struct video_device     *vfd;
	struct tvd_dmaqueue     vidq;

	/* Several counters */
	unsigned     		ms;
	unsigned long           jiffies;

	/* Input Number */
	int	                input;

	/* video capture */
	struct tvd_fmt          *fmt;
	unsigned int            width;
	unsigned int            height;
	unsigned int		frame_size;
	unsigned int            capture_mode;
	struct vb2_alloc_ctx 	*alloc_ctx;
	struct vb2_queue   	vb_vidq;

	unsigned int            interface; /*0 cvbs,1 ypbprI,2 ypbprP*/
	unsigned int            system;	/*0 ntsc, 1 pal*/

	unsigned int            locked;	/* signal is stable or not */
	unsigned int            format;
	unsigned int            channel_index[4];
	int			irq;

	/* working state */
	unsigned long 	        generating;
	unsigned long		opened;

	/* clock */
	struct clk		*parent;
	struct clk		*clk;
	struct clk		*clk_top;
	void __iomem		*regs_top;
	void __iomem		*regs_tvd;

	struct writeback_addr	wb_addr;

	/* tvd revelent para */
	unsigned int            luma;
	unsigned int            contrast;
	unsigned int            saturation;
	unsigned int            hue;

	unsigned int            uv_swap;
	struct v4l2_fract       fps;
	struct tvd_3d_fliter fliter;
	//struct early_suspend early_suspend;
	unsigned int 		para_from;
	struct v4l2_ctrl_handler  ctrl_handler;
	struct mutex              buf_lock;
	struct mutex              stream_lock;
	struct mutex		  opened_lock;
	struct tvd_dmaqueue       vidq_special;
	struct tvd_dmaqueue       done_special;
	int special_active;
};

#endif /* __TVD__H__ */


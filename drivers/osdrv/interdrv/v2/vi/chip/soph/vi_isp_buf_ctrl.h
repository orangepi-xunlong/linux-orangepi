#ifndef __VI_ISP_BUF_CTRL_H__
#define __VI_ISP_BUF_CTRL_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#include <uapi/linux/sched/types.h>
#include <vi_tun_cfg.h>
#include <vi_isp.h>
#include <vi_common.h>
#include <vip/vi_drv.h>
#include <vi_defines.h>

enum cvi_isp_buf_status {
	INTERNAL_BUFFER,
	EXTERNAL_BUFFER,
};

struct isp_buffer {
	enum sop_isp_raw	raw_num;
	uint8_t	          is_ext;
	enum sop_isp_fe_chn_num chn_num;
	struct vi_rect		crop_le;
	struct vi_rect		crop_se;
	struct isp_grid_s_info	rgbmap_i;
	struct isp_grid_s_info	lmap_i;
	struct list_head	list;
	u64			addr;
	u32			byr_size;
	u32			frm_num;
	u32			ir_idx;
	struct timespec64	timestamp;
};

struct isp_queue {
	struct list_head	rdy_queue;
	u32			num_rdy;
	enum sop_isp_raw	raw_num;
};

extern spinlock_t buf_lock;

struct isp_buffer *isp_buf_next(struct isp_queue *q);
struct isp_buffer *isp_buf_last(struct isp_queue *q);
void isp_buf_queue(struct isp_queue *q, struct isp_buffer *b);
struct isp_buffer *isp_buf_remove(struct isp_queue *q);
int isp_buf_empty(struct isp_queue *q);

#ifdef __cplusplus
}
#endif

#endif //__VI_ISP_BUF_CTRL_H__


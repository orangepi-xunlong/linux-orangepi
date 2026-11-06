#ifndef __VO_H__
#define __VO_H__

#ifdef __cplusplus
	extern "C" {
#endif
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
#endif

#include <linux/vo_uapi.h>
#include "vo_disp.h"
#include "vo_common.h"
#include "vo_ctx.h"
#include "vo_sdk_layer.h"
#include "disp.h"

#define CHECK_STRUCT_SIZE(size, type) \
	do { \
		if ((size) != sizeof(type)) { \
			TRACE_VO(DBG_ERR, "data size error!\n"); \
			return -EINVAL; \
		} \
	} while (0)


#define VO_DV_BT_BLANKING_WIDTH(bt) \
	(bt->hfrontporch + bt->hsync + bt->hbackporch)
#define VO_DV_BT_FRAME_WIDTH(bt) \
	(bt->width + VO_DV_BT_BLANKING_WIDTH(bt))
#define VO_DV_BT_BLANKING_HEIGHT(bt) \
	(bt->vfrontporch + bt->vsync + bt->vbackporch + \
	 bt->il_vfrontporch + bt->il_vsync + bt->il_vbackporch)
#define VO_DV_BT_FRAME_HEIGHT(bt) \
	(bt->height + VO_DV_BT_BLANKING_HEIGHT(bt))

struct disp_buffer {
	struct vo_buffer buf;
	vb_blk blk;
	struct list_head list;
	u32 sequence;
};

struct wbc_buffer {
	struct vo_wbc_buffer buf;
	struct list_head list;
};

struct disp_pattern {
	enum disp_pat_type type;
	enum disp_pat_color color;
	u16 rgb[3];
};

int vo_set_interface(vo_dev dev, struct vo_disp_intf_cfg *cfg);
void vo_show_panttern(vo_dev dev);
int vo_start_streaming(vo_dev dev);
int vo_stop_streaming(vo_dev dev);
int vo_create_thread(vo_layer layer);
int vo_destroy_thread(vo_layer layer);
int vo_wbc_create_thread(vo_wbc wbc_dev);
int vo_wbc_destroy_thread(vo_wbc wbc_dev);
void vo_fill_disp_timing(struct disp_timing *timing, struct vo_bt_timings *bt_timing);
int vo_recv_frame(mmf_chn_s chn, vb_blk blk);

void vo_wbc_rdy_buf_queue(struct vo_wbc_ctx *wbc_ctx, struct wbc_buffer *qbuf);
struct wbc_buffer *vo_wbc_rdy_buf_next(struct vo_wbc_ctx *wbc_ctx);
int vo_wbc_rdy_buf_empty(struct vo_wbc_ctx *wbc_ctx);
void vo_wbc_rdy_buf_pop(struct vo_wbc_ctx *wbc_ctx);
void vo_wbc_rdy_buf_remove(struct vo_wbc_ctx *wbc_ctx);
void vo_wbc_dqbuf_list(struct vo_wbc_ctx *wbc_ctx);
int vo_wbc_dqbuf(struct vo_wbc_ctx *wbc_ctx);

#ifdef __cplusplus
}
#endif

#endif /* __VO_H__ */

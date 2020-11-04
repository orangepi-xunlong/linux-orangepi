/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/sunxi-gpio.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/slab.h>
#include <linux/delay.h>

#if (defined CONFIG_SUNXI_DI && defined CONFIG_ARCH_SUN8IW17)

#define USE_SUNXI_DI_MODULE

#define ALIGN_16B(x) (((x) + (15)) & ~(15))

enum __di_pixel_fmt_t {
	DI_FORMAT_NV12 = 0x00,	/* 2-plane */
	DI_FORMAT_NV21 = 0x01,	/* 2-plane */
	DI_FORMAT_MB32_12 = 0x02, /* NOT SUPPORTED, UV mapping like NV12 */
	DI_FORMAT_MB32_21 = 0x03, /* NOT SUPPORTED, UV mapping like NV21 */
	DI_FORMAT_YV12 = 0x04,	/* 3-plane */
	DI_FORMAT_YUV422_SP_UVUV = 0x08, /* 2-plane, New in DI_V2.2 */
	DI_FORMAT_YUV422_SP_VUVU = 0x09, /* 2-plane, New in DI_V2.2 */
	DI_FORMAT_YUV422P = 0x0c,	/* 3-plane, New in DI_V2.2 */
	DI_FORMAT_MAX,
};

enum __di_intp_mode_t {
	DI_MODE_WEAVE = 0x0, /* Copy source to destination */
	DI_MODE_INTP = 0x1, /* Use current field to interpolate another field */
	DI_MODE_MOTION = 0x2, /* Use 4-field to interpolate another field */
};

enum __di_updmode_t {
	DI_UPDMODE_FIELD = 0x0, /* Output 2 frames when updated 1 input frame */
	DI_UPDMODE_FRAME = 0x1, /* Output 1 frame when updated 1 input frame */
};

struct __di_rectsz_t {
	unsigned int width;
	unsigned int height;
};

struct __di_fb_t {
	void *addr[2];		/* frame buffer address */
	struct __di_rectsz_t size; /* size pixel */
	enum __di_pixel_fmt_t format;
};

struct __di_para_t {
	struct __di_fb_t input_fb;	/* current frame fb */
	struct __di_fb_t pre_fb;	/* previous frame fb */
	struct __di_rectsz_t source_regn; /* current frame and
					* previous frame process region
					*/
	struct __di_fb_t output_fb;/* output frame fb */
	struct __di_rectsz_t out_regn;	/* output frame region */
	__u32 field;			/* process field <0-top field ;
					*1-bottom field>
					*/
	__u32 top_field_first;		/*video information <0-is not
					*top_field_first;1-is top_
					*field_first>
					*/
};

/* di_format_attr - display format attribute
 *
 * @format: pixel format
 * @bits: bits of each component
 * @hor_rsample_u: reciprocal of horizontal sample rate
 * @hor_rsample_v: reciprocal of horizontal sample rate
 * @ver_rsample_u: reciprocal of vertical sample rate
 * @hor_rsample_v: reciprocal of vertical sample rate
 * @uvc: 1: u & v component combined
 * @interleave: 0: progressive, 1: interleave
 * @factor & div: bytes of pixel = factor / div (bytes)
 *
 * @addr[out]: address for each plane
 * @trd_addr[out]: address for each plane of right eye buffer
 */
struct di_format_attr {
	enum __di_pixel_fmt_t format;
	unsigned int bits;
	unsigned int hor_rsample_u;
	unsigned int hor_rsample_v;
	unsigned int ver_rsample_u;
	unsigned int ver_rsample_v;
	unsigned int uvc;
	unsigned int interleave;
	unsigned int factor;
	unsigned int div;
};

struct __di_fb_t2 {
	int fd;
	unsigned long long addr[3]; /* frame buffer address */
	struct __di_rectsz_t size;  /* size (in pixel) */
	enum __di_pixel_fmt_t format;
};

struct __di_para_t2 {
	struct __di_fb_t2 input_fb;/* current frame fb */
	struct __di_fb_t2 pre_fb;	 /* previous frame fb */
	struct __di_fb_t2 next_fb;	/* next frame fb */
	struct __di_rectsz_t source_regn; /* current frame /previous frame and
						next frame process region */
	struct __di_fb_t2 output_fb;	/* output frame fb */
	struct __di_rectsz_t out_regn;	/* output frame region */
	unsigned int field; /* process field <0-first field ; 1-second field> */
	unsigned int top_field_first; /* video infomation <0-is not
				top_field_first; 1-is top_field_first> */
	/* unsigned int update_mode; */
	/* update buffer mode <0-update 1 frame,
	output 2 frames; 1-update 1 frame, output 1 frame> */
	int id;
	int dma_if;
};

/* New in DI_2.X */
struct __di_mode_t {
	enum __di_intp_mode_t di_mode;
	enum __di_updmode_t update_mode;
};

struct __di_mem_t {
	unsigned int size;
	void *v_addr;
	unsigned long p_addr;
};

extern unsigned int sunxi_di_request(void);
extern int sunxi_di_commit(struct __di_para_t2 *di_para, struct file *filp);
extern int sunxi_di_close(unsigned int id);
extern void sunxi_di_setmode(struct __di_mode_t *di_mode);

static int di_id;
static struct __di_mode_t car_dimode;

#else
#undef USE_SUNXI_DI_MODULE
#endif
static struct buffer_node *new_frame, *old_frame, *oold_frame;

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
#include <linux/switch.h>
#endif

#include "car_reverse.h"
#include "include.h"

#define MODULE_NAME "car-reverse"

#define SWAP_BUFFER_CNT (5)
#define SWAP_BUFFER_CNT_VIN (5)

#define THREAD_NEED_STOP (1 << 0)
#define THREAD_RUN (1 << 1)
#define CAR_REVSER_GPIO_LEVEL 1
/*#define USE_YUV422*/
#undef USE_YUV422
struct car_reverse_private_data {
	struct preview_params config;
	int reverse_gpio;

	struct buffer_pool *buffer_pool;
	struct buffer_pool *bufferOv_pool[CAR_MAX_CH];
	struct buffer_pool *bufferOv_preview[CAR_MAX_CH];
	struct buffer_node *buffer_disp[2];

	struct work_struct status_detect;
	struct workqueue_struct *preview_workqueue;

	struct task_struct *display_update_task;
	struct task_struct *display_frame_task;
	struct task_struct *display_view_task;
	struct list_head pending_frame;
	struct list_head pending_frameOv[CAR_MAX_CH];
	spinlock_t display_lock;

	int needexit;
	int needfree;
	int status;
	int disp_index;
	int debug;
	int format;
	int used_oview;
	int thread_mask;
	int discard_frame;
	int ov_sync;
	int ov_sync_algo;
	int ov_sync_frame;
	int sync_w;
	int sync_r;
	int standby;
	int view_thread_start;
	int algo_thread_start;
	int view_mask;
	int algo_mask;
	spinlock_t thread_lock;
	spinlock_t free_lock;
};

static int rotate;

static char car_irq_pin_name[16];

module_param(rotate, int, 0644);

static struct car_reverse_private_data *car_reverse;

#define UPDATE_STATE 1
#if defined(UPDATE_STATE) &&		\
	(defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH))
static ssize_t print_dev_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", sdev->name);
}

static struct switch_dev car_reverse_switch = {
	.name = "parking-switch", .state = 0, .print_name = print_dev_name,
};

static void car_reverse_switch_register(void)
{
	switch_dev_register(&car_reverse_switch);
}

static void car_reverse_switch_unregister(void)
{
	switch_dev_unregister(&car_reverse_switch);
}

static void car_reverse_switch_update(int flag)
{
	switch_set_state(&car_reverse_switch, flag);
}
#else
static void car_reverse_switch_register(void)
{
}
static void car_reverse_switch_update(int flag)
{
}

static void car_reverse_switch_unregister(void)
{
}

#endif

static void of_get_value_by_name(struct platform_device *pdev, const char *name,
				 int *retval, unsigned int defval)
{
	if (of_property_read_u32(pdev->dev.of_node, name, retval) != 0) {
		dev_err(&pdev->dev, "missing property '%s', default value %d\n",
			name, defval);
		*retval = defval;
	}
}

static void of_get_gpio_by_name(struct platform_device *pdev, const char *name,
				int *retval)
{
	int gpio_index;
	struct gpio_config config;

	gpio_index = of_get_named_gpio_flags(pdev->dev.of_node, name, 0,
						 (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(gpio_index)) {
		dev_err(&pdev->dev, "failed to get gpio '%s'\n", name);
		*retval = 0;
		return;
	}
	*retval = gpio_index;

	dev_info(&pdev->dev,
		 "%s: gpio=%d mul-sel=%d pull=%d drv_level=%d data=%d\n", name,
		 config.gpio, config.mul_sel, config.pull, config.drv_level,
		 config.data);
}

static void parse_config(struct platform_device *pdev,
			 struct car_reverse_private_data *priv)
{
	of_get_value_by_name(pdev, "tvd_id", &priv->config.tvd_id, 0);
	of_get_value_by_name(pdev, "screen_width", &priv->config.screen_width,
				0);
	of_get_value_by_name(pdev, "screen_height", &priv->config.screen_height,
				0);
	of_get_value_by_name(pdev, "rotation", &priv->config.rotation, 0);
	of_get_value_by_name(pdev, "source", &priv->config.input_src, 0);
	of_get_value_by_name(pdev, "oview", &priv->used_oview, 0);
	of_get_gpio_by_name(pdev, "reverse_pin", &priv->reverse_gpio);
}

void car_reverse_display_update(int tvd_fd)
{
	int run_thread = 0;
	int n = 0;
	int tmp = 0;
	struct buffer_node *node;
	struct list_head *pending_frame = &car_reverse->pending_frame;

	spin_lock(&car_reverse->display_lock);

	if (car_reverse->used_oview) {
		tmp = car_reverse->sync_w - car_reverse->sync_r;
		if ((car_reverse->ov_sync & (1 << tvd_fd)) || tmp > 3) {
			for (n = 0; n < CAR_MAX_CH; n++) {
				if (car_reverse->config.input_src)
					node =
				video_source_dequeue_buffer_vin(n);
				else {
				#ifdef VIDEO_SUNXI_TVD_SPECIAL
				node = video_source_dequeue_buffer(n);
				#endif
				}
				if (node) {
					if (car_reverse->config.input_src)
						video_source_queue_buffer_vin(
							node, n);
					else {
						#ifdef VIDEO_SUNXI_TVD_SPECIAL
						video_source_queue_buffer(node,
						n);
						#endif
						}
				}
			}
			car_reverse->ov_sync = 0;
		} else {
			car_reverse->ov_sync |= (1 << tvd_fd);
			if ((car_reverse->ov_sync & 0xf) == 0xf) {
				run_thread = 1;
				car_reverse->ov_sync = 0;
			}
		}
	}
	if (!car_reverse->used_oview) {
		while (!list_empty(pending_frame)) {
			node = list_entry(pending_frame->next,
			struct buffer_node, list);
			list_del(&node->list);
			if (car_reverse->config.input_src)
			video_source_queue_buffer_vin(node, tvd_fd);
			else {
				#ifdef VIDEO_SUNXI_TVD_SPECIAL
				video_source_queue_buffer(node, tvd_fd);
				#endif
				}
		}
		if (car_reverse->config.input_src)
			node = video_source_dequeue_buffer_vin(tvd_fd);
		else {
			#ifdef VIDEO_SUNXI_TVD_SPECIAL
			node = video_source_dequeue_buffer(tvd_fd);
			#endif
			}
		if (node) {
			list_add(&node->list, pending_frame);
		}
	} else {
		if (run_thread) {
			for (n = 0; n < CAR_MAX_CH; n++) {
				pending_frame =
				&car_reverse->pending_frameOv[n];
				if (car_reverse->config.input_src)
				node =
				video_source_dequeue_buffer_vin(n);
				else {
				#ifdef VIDEO_SUNXI_TVD_SPECIAL
				node = video_source_dequeue_buffer(n);
				#endif
				}
				if (node) {
				list_add(&node->list, pending_frame);
				}
			}
		}
	}
	spin_unlock(&car_reverse->display_lock);

	if (car_reverse->used_oview) {
		if (run_thread) {
			if (car_reverse->display_update_task)
			wake_up_process(
			car_reverse->display_update_task);
			car_reverse->thread_mask |= THREAD_RUN;
			car_reverse->sync_w++;
			run_thread = 0;
		}
	} else {

		spin_lock(&car_reverse->thread_lock);
		if (car_reverse->thread_mask & THREAD_NEED_STOP) {
			spin_unlock(&car_reverse->thread_lock);
			return;
		}
		if (car_reverse->display_update_task)
			wake_up_process(car_reverse->display_update_task);
		car_reverse->thread_mask |= THREAD_RUN;
		spin_unlock(&car_reverse->thread_lock);
	}
}

void car_do_freemem(struct work_struct *work)
{
	int i = 0;
	struct buffer_pool *bp = 0;
	spin_lock(&car_reverse->free_lock);
	if (car_reverse->needfree) {
		if (car_reverse->buffer_pool) {
			free_buffer_pool(car_reverse->config.dev,
				car_reverse->buffer_pool);
			car_reverse->buffer_pool = 0;
		}
		for (i = 0; i < CAR_MAX_CH; i++) {
			bp = car_reverse->bufferOv_pool[i];
			if (bp) {
				free_buffer_pool(car_reverse->config.dev, bp);
				car_reverse->bufferOv_pool[i] = 0;
			}
			bp = car_reverse->bufferOv_preview[i];
			if (bp) {
				free_buffer_pool(car_reverse->config.dev, bp);
				car_reverse->bufferOv_preview[i] = 0;
			}
		}
		if (car_reverse->config.input_src == 0) {
			if (car_reverse->buffer_disp[0]) {
				__buffer_node_free(car_reverse->config.dev,
				car_reverse->buffer_disp[0]);
				car_reverse->buffer_disp[0] = 0;
			}
			if (car_reverse->buffer_disp[1]) {
				__buffer_node_free(car_reverse->config.dev,
				car_reverse->buffer_disp[1]);
				car_reverse->buffer_disp[1] = 0;
			}
		}
		logerror("car_reverse free buffer\n");
	} else
		logdebug("no need free buffer! \n");
	spin_unlock(&car_reverse->free_lock);
}

static DECLARE_DELAYED_WORK(car_freework, car_do_freemem);

int algo_frame_work(void *data)
{
	struct buffer_pool *bp = 0;
	struct buffer_pool *bp_preview = 0;
	int i = 0;
	int tmp = 0;
	int count = 0;
	struct buffer_node *new_frameOv[CAR_MAX_CH];
	while (!kthread_should_stop()) {
		car_reverse->algo_mask = 0;
		if (car_reverse->algo_thread_start) {
			tmp = car_reverse->ov_sync_frame -
			car_reverse->ov_sync_algo;
			if (car_reverse->ov_sync_frame >
				car_reverse->ov_sync_algo) {
				for (i = 0; i < CAR_MAX_CH; i++) {
				new_frameOv[i] = 0;
				bp = car_reverse->bufferOv_pool[i];
					if (bp)
				new_frameOv[i] =
				bp->dequeue_buffer(bp);
				}
				if (tmp <= 4) {
					if (new_frameOv[0] && new_frameOv[1] &&
					new_frameOv[2] && new_frameOv[3] &&
					count) {
					preview_update_Ov(
					new_frameOv,
					car_reverse->config
					.car_direct,
					car_reverse->config
					.lr_direct);
					}

					if (count >= 3)
					count = 0;
					else
					count++;

					for (i = 0; i < CAR_MAX_CH; i++) {
					bp_preview =
					car_reverse
					->bufferOv_preview[i];
					if (new_frameOv[i]) {
					if (car_reverse
					->display_view_task)
					bp_preview
					->queue_buffer(
					bp_preview,
					new_frameOv
					[i]);
					else {
					video_source_queue_buffer_vin(
					new_frameOv
					[i],
					i);
					}
					}
					}
					car_reverse->ov_sync_algo++;
				} else {
				while (tmp > 0 &&
					!kthread_should_stop()) {

			for (i = 0; i < CAR_MAX_CH;
				i++) {
				new_frameOv[i] = 0;
				bp = car_reverse
				->bufferOv_pool
				[i];
				if (bp)
				new_frameOv[i] =
				bp->dequeue_buffer(
				bp);
				if (new_frameOv[i]) {
				video_source_queue_buffer_vin(
				new_frameOv
				[i],
				i);
				}
				}
				car_reverse->ov_sync_algo++;
				tmp =
				car_reverse->ov_sync_frame -
				car_reverse->ov_sync_algo;
				schedule_timeout(HZ / 10000);
				}
			}
		}
		}
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			set_current_state(TASK_RUNNING);
		if (!car_reverse->algo_thread_start) {
			car_reverse->algo_mask = 1;
			schedule();
		} else
			schedule_timeout(HZ / 10000);
	}
	return 0;
}

int display_view_work(void *data)
{
	struct buffer_pool *bp_preview = 0;
	struct buffer_node *frameOv[4];
	int i = 0;
	while (!kthread_should_stop()) {
		if (car_reverse->view_thread_start) {
			car_reverse->view_mask = 0;
			display_frame_work();
			for (i = 0; i < CAR_MAX_CH; i++) {
			bp_preview = car_reverse->bufferOv_preview[i];
			frameOv[i] =
			bp_preview->dequeue_buffer(bp_preview);
				if (frameOv[i]) {
			video_source_queue_buffer_vin(
			frameOv[i], i);
				}
			}
		}
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			set_current_state(TASK_RUNNING);
		if (!car_reverse->view_thread_start) {
			car_reverse->view_mask = 1;
			schedule();
		} else
			schedule_timeout(HZ / 10000);
	}
	return 0;
}

static int display_update_thread(void *data)
{
	struct list_head *pending_frame = &car_reverse->pending_frame;
	struct buffer_pool *bp = car_reverse->buffer_pool;
	int ret = 0;
	int i = 0;
	struct buffer_node *new_frameOv[4];
#ifdef USE_SUNXI_DI_MODULE
	struct file car_file;
	unsigned int src_width, src_height;
	unsigned int dst_width, dst_height;
	struct __di_para_t2 di_para;
#endif
	new_frame = NULL;
	old_frame = NULL;
	oold_frame = NULL;
	while (!kthread_should_stop()) {
		bp = car_reverse->buffer_pool;
		if (!car_reverse->used_oview) {
			ret = spin_is_locked(&car_reverse->display_lock);
			if (ret) {
				goto disp_loop;
			}
		}
		if (!car_reverse->used_oview)
			spin_lock(&car_reverse->display_lock);
		if (car_reverse->config.input_src && car_reverse->used_oview) {
			if (car_reverse->sync_w != car_reverse->sync_r) {
				car_reverse->sync_r++;
				for (i = 0; i < CAR_MAX_CH; i++) {
			new_frameOv[i] = NULL;
			pending_frame =
			&car_reverse->pending_frameOv[i];
			bp = car_reverse->bufferOv_pool[i];
			if (pending_frame->next !=
			pending_frame) {
			new_frameOv[i] = list_entry(
				pending_frame->next,
				struct buffer_node, list);
				list_del(&new_frameOv[i]->list);
				bp->queue_buffer(
				bp, new_frameOv[i]);
				}
				}
				car_reverse->ov_sync_frame++;
			}
		} else {
			if (pending_frame->next != pending_frame) {
				new_frame =
					list_entry(pending_frame->next,
					struct buffer_node, list);
				list_del(&new_frame->list);
				bp->queue_buffer(bp, new_frame);
			}
		}
#ifdef USE_SUNXI_DI_MODULE
		if (car_reverse->config.input_src) {
			if (!car_reverse->used_oview) {
				old_frame = bp->dequeue_buffer(bp);
				list_add(&old_frame->list, pending_frame);
				spin_unlock(&car_reverse->display_lock);
				preview_update(new_frame,
				car_reverse->config.car_direct,
				car_reverse->config.lr_direct);
			}
		} else {
			if (old_frame) {
				oold_frame = old_frame;
				old_frame = bp->dequeue_buffer(bp);
			} else {
				old_frame = bp->dequeue_buffer(bp);
			}
			spin_unlock(&car_reverse->display_lock);
			if (oold_frame && old_frame && new_frame) {
				src_height = car_reverse->config.src_height;
				src_width = car_reverse->config.src_width;
				dst_width = src_width;
				dst_height = src_height;
				di_para.pre_fb.addr[0] =
					*(unsigned long long *)(&(
					oold_frame->phy_address));
				di_para.pre_fb.addr[1] =
					*(unsigned long long *)(&(
					oold_frame->phy_address)) +
					ALIGN_16B(src_width) * src_height;
				di_para.pre_fb.addr[2] = 0x0;
				di_para.pre_fb.size.width = src_width;
				di_para.pre_fb.size.height = src_height;
				if (car_reverse->config.format ==
					V4L2_PIX_FMT_NV61)
					di_para.pre_fb.format =
						DI_FORMAT_YUV422_SP_VUVU;
				else
					di_para.pre_fb.format = DI_FORMAT_NV21;

				di_para.input_fb.addr[0] =
					*(unsigned long long *)(&(
					old_frame->phy_address));
				di_para.input_fb.addr[1] =
					*(unsigned long long *)(&(
					old_frame->phy_address)) +
					ALIGN_16B(src_width) * src_height;
				di_para.input_fb.addr[2] = 0x0;
				di_para.input_fb.size.width = src_width;
				di_para.input_fb.size.height = src_height;
				if (car_reverse->config.format ==
					V4L2_PIX_FMT_NV61)
					di_para.input_fb.format =
						DI_FORMAT_YUV422_SP_VUVU;
				else
					di_para.input_fb.format =
						DI_FORMAT_NV21;

				di_para.next_fb.addr[0] =
					*(unsigned long long *)(&(
					new_frame->phy_address));
				di_para.next_fb.addr[1] =
					*(unsigned long long *)(&(
					new_frame->phy_address)) +
					ALIGN_16B(src_width) * src_height;
				di_para.next_fb.addr[2] = 0x0;
				di_para.next_fb.size.width = src_width;
				di_para.next_fb.size.height = src_height;
				if (car_reverse->config.format ==
					V4L2_PIX_FMT_NV61)
					di_para.next_fb.format =
						DI_FORMAT_YUV422_SP_VUVU;
				else
					di_para.next_fb.format = DI_FORMAT_NV21;

				di_para.source_regn.width = src_width;
				di_para.source_regn.height = src_height;

				di_para.output_fb.addr[0] = *(
					unsigned long long *)(&(
					car_reverse
					->buffer_disp[car_reverse->disp_index]
					->phy_address));
				di_para.output_fb.addr[1] =
					*(unsigned long long *)(&(
					car_reverse
						->buffer_disp[car_reverse
								  ->disp_index]
						->phy_address)) +
					ALIGN_16B(dst_width) * dst_height;
				di_para.output_fb.addr[2] = 0x0;
				di_para.output_fb.size.width = dst_width;
				di_para.output_fb.size.height = dst_height;
				if (car_reverse->config.format ==
					V4L2_PIX_FMT_NV61)
					di_para.output_fb.format =
						DI_FORMAT_YUV422_SP_VUVU;
				else
					di_para.output_fb.format =
						DI_FORMAT_NV21;

				di_para.out_regn.width = dst_width;
				di_para.out_regn.height = dst_height;

				di_para.field = 0;
				di_para.top_field_first = 1;
				di_para.id = di_id;
				di_para.dma_if = 1;
				sunxi_di_commit(&di_para, &car_file);
				di_para.field = 1;
				di_para.top_field_first = 1;
				di_para.id = di_id;
				di_para.dma_if = 1;
				sunxi_di_commit(&di_para, &car_file);
				if (oold_frame) {
					spin_lock(&car_reverse->display_lock);
					list_add(&oold_frame->list,
						 pending_frame);
					spin_unlock(&car_reverse->display_lock);
				}
				if (car_reverse->discard_frame != 0) {
					car_reverse->discard_frame--;
				} else
					car_reverse->discard_frame = 0;
				if (car_reverse->discard_frame == 0) {
					preview_update(
						car_reverse->buffer_disp
						[car_reverse->disp_index],
						car_reverse->config.car_direct,
						car_reverse->config.lr_direct);
				}
				if (car_reverse->disp_index)
					car_reverse->disp_index = 0;
				else
					car_reverse->disp_index = 1;

			} else {
				preview_update(new_frame,
					car_reverse->config.car_direct,
					car_reverse->config.lr_direct);
			}
		}
#else
		old_frame = bp->dequeue_buffer(bp);
		list_add(&old_frame->list, pending_frame);
		spin_unlock(&car_reverse->display_lock);

		if (car_reverse->discard_frame != 0) {
					car_reverse->discard_frame--;
				} else
					car_reverse->discard_frame = 0;
				if (car_reverse->discard_frame == 0) {
					preview_update(
						new_frame,
						car_reverse->config.car_direct,
						car_reverse->config.lr_direct);
				}
/*		preview_update(new_frame, car_reverse->config.car_direct,
				car_reverse->config.lr_direct);*/
#endif

	disp_loop:

		car_reverse->thread_mask &= (~THREAD_RUN);
		if (car_reverse->config.input_src && car_reverse->used_oview) {
			if (car_reverse->sync_w == car_reverse->sync_r)
				schedule();
			else
				schedule_timeout(HZ / 10000);
			if (kthread_should_stop()) {
				break;
			}
		} else {
			set_current_state(TASK_INTERRUPTIBLE);
			if (kthread_should_stop()) {
				break;
			}
			schedule();
		}
	}
	logerror("%s stop\n", __func__);
	return 0;
}

static int car_reverse_preview_start(void)
{
	int retval = 0;
	int i, n;
	struct buffer_node *node;
	struct buffer_pool *bp = 0;
	unsigned int buf_cnt = 0;
	car_reverse->disp_index = 0;
	car_reverse->discard_frame = 8;
	car_reverse->needfree = 0;
	car_reverse->config.car_oview_mode = car_reverse->used_oview;
	cancel_delayed_work(&car_freework);
	car_reverse->display_update_task =
		kthread_create(display_update_thread, NULL, "sunxi-preview");
	if (!car_reverse->display_update_task) {
		printk(KERN_ERR "failed to create kthread\n");
		return -1;
	}
	/* FIXME: Calculate buffer size by preview info */
	//spin_lock(&car_reverse->free_lock);
	if (car_reverse->config.input_src) {
		if (car_reverse->buffer_pool == 0 && !car_reverse->used_oview) {
			car_reverse->buffer_pool = alloc_buffer_pool(
				car_reverse->config.dev, SWAP_BUFFER_CNT_VIN,
				1280 * 720 * 2);
		}
		if (car_reverse->used_oview) {
			for (i = 0; i < CAR_MAX_CH; i++) {
				if (car_reverse->bufferOv_pool[i] == 0 &&
					car_reverse->used_oview) {
					car_reverse->bufferOv_pool[i] =
						alloc_buffer_pool(
						car_reverse->config.dev,
						SWAP_BUFFER_CNT_VIN,
						1280 * 720 * 2);
				}

				if (car_reverse->bufferOv_preview[i] == 0 &&
					car_reverse->used_oview) {
					car_reverse->bufferOv_preview[i] =
					alloc_buffer_pool(
					car_reverse->config.dev,
					SWAP_BUFFER_CNT_VIN, 0);
				}
			}
		}
		car_reverse->buffer_disp[0] = 0;
		car_reverse->buffer_disp[1] = 0;
		buf_cnt = SWAP_BUFFER_CNT_VIN;
	} else {
		if (car_reverse->buffer_pool == 0)
			car_reverse->buffer_pool =
				alloc_buffer_pool(car_reverse->config.dev,
						SWAP_BUFFER_CNT, 720 * 576 * 2);
		if (car_reverse->buffer_disp[0] == 0)
			car_reverse->buffer_disp[0] = __buffer_node_alloc(
				car_reverse->config.dev, 720 * 576 * 2, 0);
		if (car_reverse->buffer_disp[1] == 0)
			car_reverse->buffer_disp[1] = __buffer_node_alloc(
				car_reverse->config.dev, 720 * 576 * 2, 0);
		buf_cnt = SWAP_BUFFER_CNT;
	}
	//spin_unlock(&car_reverse->free_lock);
	if (!car_reverse->buffer_pool && !car_reverse->used_oview) {
		dev_err(car_reverse->config.dev,
			"alloc buffer memory failed\n");
		goto gc;
	}
	if (car_reverse->config.input_src && car_reverse->used_oview) {
		if (!car_reverse->bufferOv_pool[0] ||
			!car_reverse->bufferOv_pool[1] ||
			!car_reverse->bufferOv_pool[2] ||
			!car_reverse->bufferOv_pool[3]) {
			dev_err(car_reverse->config.dev,
				"alloc buffer memory oview failed\n");
			goto gc;
		}

		if (!car_reverse->bufferOv_preview[0] ||
			!car_reverse->bufferOv_preview[1] ||
			!car_reverse->bufferOv_preview[2] ||
			!car_reverse->bufferOv_preview[3]) {
			dev_err(car_reverse->config.dev,
				"alloc buffer memory oview failed\n");
			goto gc;
		}
	}

	if (car_reverse->config.input_src == 0) {
		if (!car_reverse->buffer_disp[0] ||
			!car_reverse->buffer_disp[1]) {
			dev_err(car_reverse->config.dev,
				"alloc buffer memory failed\n");
			goto gc;
		}
	}
	if (car_reverse->config.input_src) {
		car_reverse->config.format = V4L2_PIX_FMT_NV21;
	} else {
		if (car_reverse->format)
			car_reverse->config.format = V4L2_PIX_FMT_NV61;
		else
			car_reverse->config.format = V4L2_PIX_FMT_NV21;
	}
	if (car_reverse->used_oview && car_reverse->config.input_src) {

		for (n = 0; n < CAR_MAX_CH; n++) {
			bp = car_reverse->bufferOv_pool[n];

			INIT_LIST_HEAD(&car_reverse->pending_frameOv[n]);
			retval = video_source_connect(&car_reverse->config, n);
			if (retval != 0) {
				logerror("can't connect to video source!\n");
				goto gc;
			}

			for (i = 0; i < buf_cnt; i++) {
				node = bp->dequeue_buffer(bp);
				if (car_reverse->config.input_src)
					video_source_queue_buffer_vin(node, n);
				else {
					#ifdef VIDEO_SUNXI_TVD_SPECIAL
					video_source_queue_buffer(node, n);
					#endif
					}
			}
		}
		car_reverse->ov_sync_frame = 0;
		car_reverse->ov_sync_algo = 0;
		car_reverse->sync_r = 0;
		car_reverse->sync_w = 0;

		car_reverse->display_frame_task =
			kthread_create(algo_frame_work, NULL, "algo-preview");
		if (!car_reverse->display_frame_task) {
			printk(KERN_ERR "failed to create kthread\n");
			goto gc;
		}

		printk(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);
#if 0
		car_reverse->display_view_task =
			kthread_create(display_view_work, NULL, "view-preview");
		if (!car_reverse->display_view_task) {
			printk(KERN_ERR "failed to create kthread\n");
			goto gc;
		}
#endif
		if (car_reverse->display_frame_task) {
			struct sched_param param = {.sched_priority =
				MAX_RT_PRIO - 1};
			set_user_nice(car_reverse->display_frame_task, -20);
			sched_setscheduler(car_reverse->display_frame_task,
				SCHED_FIFO, &param);
		}
		if (car_reverse->display_view_task) {
			struct sched_param param = {.sched_priority =
				MAX_RT_PRIO - 1};
			set_user_nice(car_reverse->display_view_task, -20);
			sched_setscheduler(car_reverse->display_view_task,
					SCHED_FIFO, &param);
			car_reverse->config.viewthread = 1;

		} else {
			car_reverse->config.viewthread = 0;
		}
		car_reverse->view_thread_start = 1;
		car_reverse->algo_thread_start = 1;
		msleep(1);
		if (car_reverse->display_frame_task)
			wake_up_process(car_reverse->display_frame_task);
		if (car_reverse->display_view_task)
			wake_up_process(car_reverse->display_view_task);

		preview_output_start(&car_reverse->config);
		for (n = 0; n < CAR_MAX_CH; n++) {
			video_source_streamon_vin(n);
		}
	} else {
		bp = car_reverse->buffer_pool;

		INIT_LIST_HEAD(&car_reverse->pending_frame);
		retval = video_source_connect(&car_reverse->config,
			car_reverse->config.tvd_id);
		if (retval != 0) {
			logerror("can't connect to video source!\n");
			goto gc;
		}
		preview_output_start(&car_reverse->config);
	//preview_update(car_reverse->buffer_disp[0],
		//car_reverse->config.car_direct,
				 //car_reverse->config.lr_direct);

		for (i = 0; i < buf_cnt; i++) {
			node = bp->dequeue_buffer(bp);
			if (car_reverse->config.input_src)
				video_source_queue_buffer_vin(
					node, car_reverse->config.tvd_id);
			else {
				#ifdef VIDEO_SUNXI_TVD_SPECIAL
				video_source_queue_buffer(
					node, car_reverse->config.tvd_id);
				#endif
				}
		}
		if (car_reverse->config.input_src)
			video_source_streamon_vin(car_reverse->config.tvd_id);
		else {
			#ifdef VIDEO_SUNXI_TVD_SPECIAL
			video_source_streamon(car_reverse->config.tvd_id);
			#endif
			}
	}
	car_reverse->status = CAR_REVERSE_START;
	car_reverse->thread_mask = 0;
#ifdef USE_SUNXI_DI_MODULE
	di_id = sunxi_di_request();
	car_dimode.di_mode = DI_MODE_MOTION;
	car_dimode.update_mode = DI_UPDMODE_FIELD;
	sunxi_di_setmode(&car_dimode);
#endif
	return 0;
gc:
	car_reverse->needfree = 1;
	schedule_delayed_work(&car_freework, 2 * HZ);
	return -1;
}

static int car_reverse_preview_stop(void)
{
	struct buffer_node *node;
	int i;
	struct buffer_pool *bp = car_reverse->buffer_pool;
	struct buffer_pool *preview_bp = 0;
	struct list_head *pending_frame = &car_reverse->pending_frame;
	printk(KERN_ERR "car_reverse_preview_stop start\n");

	car_reverse->status = CAR_REVERSE_STOP;
	if (car_reverse->config.input_src && car_reverse->used_oview) {
		for (i = 0; i < CAR_MAX_CH; i++)
			video_source_streamoff_vin(i);
	} else {
		if (car_reverse->config.input_src)
			video_source_streamoff_vin(car_reverse->config.tvd_id);
		else {
			#ifdef VIDEO_SUNXI_TVD_SPECIAL
			video_source_streamoff(car_reverse->config.tvd_id);
			#endif
			}
	}
	spin_lock(&car_reverse->thread_lock);
	car_reverse->thread_mask |= THREAD_NEED_STOP;
	spin_unlock(&car_reverse->thread_lock);
	if (car_reverse->config.input_src && car_reverse->used_oview) {
		struct sched_param param = {.sched_priority = MAX_RT_PRIO - 40};
		set_user_nice(car_reverse->display_frame_task, 0);
		sched_setscheduler(car_reverse->display_frame_task,
				SCHED_NORMAL, &param);
		car_reverse->view_thread_start = 0;
		car_reverse->algo_thread_start = 0;
		car_reverse->ov_sync_frame = 0;
		car_reverse->ov_sync_algo = 0;
		car_reverse->sync_r = 0;
		car_reverse->sync_w = 0;
		msleep(10);
		while (car_reverse->thread_mask & THREAD_RUN)
			msleep(1);
		while (!car_reverse->algo_mask &&
			car_reverse->display_frame_task)
			msleep(1);
		while (!car_reverse->view_mask &&
			 car_reverse->display_view_task)
			msleep(1);
		if (car_reverse->display_frame_task)
			kthread_stop(car_reverse->display_frame_task);
		if (car_reverse->display_view_task)
			kthread_stop(car_reverse->display_view_task);
		while (car_reverse->thread_mask & THREAD_RUN)
			msleep(1);
		kthread_stop(car_reverse->display_update_task);
		car_reverse->display_frame_task = 0;
		car_reverse->display_view_task = 0;
		car_reverse->display_update_task = 0;
	} else {
		while (car_reverse->thread_mask & THREAD_RUN)
			msleep(1);
		kthread_stop(car_reverse->display_update_task);
		car_reverse->display_update_task = 0;
	}
	preview_output_stop(&car_reverse->config);

__buffer_gc:
	if (car_reverse->config.input_src && car_reverse->used_oview) {
		for (i = 0; i < CAR_MAX_CH; i++) {
			bp = car_reverse->bufferOv_pool[i];
			while (1) {
				node = video_source_dequeue_buffer_vin(i);
				if (node) {
				bp->queue_buffer(bp, node);
				logdebug("%s: collect %p\n", __func__,
				node->phy_address);
				} else {
				video_source_disconnect(
				&car_reverse->config, i);
				break;
				}
			}
		}
		for (i = 0; i < CAR_MAX_CH; i++) {
			bp = car_reverse->bufferOv_pool[i];
			preview_bp = car_reverse->bufferOv_preview[i];
			if (preview_bp) {
				while (1) {
				node = preview_bp->dequeue_buffer(
				preview_bp);
			if (node) {
				bp->queue_buffer(bp, node);
				logdebug("%s: collect %p\n",
				__func__,
				node->phy_address);
				} else {
				break;
				}
				}
				rest_buffer_pool(NULL, preview_bp);
				dump_buffer_pool(NULL, preview_bp);
			}
		}

		for (i = 0; i < CAR_MAX_CH; i++) {
			spin_lock(&car_reverse->display_lock);
			pending_frame = &car_reverse->pending_frameOv[i];
			bp = car_reverse->bufferOv_pool[i];
			while (!list_empty(pending_frame)) {
				node = list_entry(pending_frame->next,
						struct buffer_node, list);
				list_del(&node->list);
				bp->queue_buffer(bp, node);
			}
			spin_unlock(&car_reverse->display_lock);
			rest_buffer_pool(NULL, bp);
			dump_buffer_pool(NULL, bp);
		}
	} else {
		if (car_reverse->config.input_src)
			node = video_source_dequeue_buffer_vin(
				car_reverse->config.tvd_id);
		else {
			#ifdef VIDEO_SUNXI_TVD_SPECIAL
			node = video_source_dequeue_buffer(
				car_reverse->config.tvd_id);
			#endif
			}
		if (node) {
			bp->queue_buffer(bp, node);
			logdebug("%s: collect %p\n", __func__,
				 node->phy_address);
			goto __buffer_gc;
		}
		spin_lock(&car_reverse->display_lock);
		while (!list_empty(pending_frame)) {
			node = list_entry(pending_frame->next,
					struct buffer_node, list);
			list_del(&node->list);
			bp->queue_buffer(bp, node);
		}
		spin_unlock(&car_reverse->display_lock);
		rest_buffer_pool(NULL, bp);
		dump_buffer_pool(NULL, bp);
		video_source_disconnect(&car_reverse->config,
					car_reverse->config.tvd_id);
	}
#ifdef USE_SUNXI_DI_MODULE
	sunxi_di_close(di_id);
	di_id = -1;
#endif
	car_reverse->needfree = 1;
	schedule_delayed_work(&car_freework, 2 * HZ);
	new_frame = NULL;
	old_frame = NULL;
	oold_frame = NULL;
	printk(KERN_ERR "car_reverse_preview_stop finish\n");
	return 0;
}

void car_reverse_set_int_ioin(void)
{
	long unsigned int config;
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, car_irq_pin_name, &config);
	if (0 != SUNXI_PINCFG_UNPACK_VALUE(config)) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0);
		pin_config_set(SUNXI_PINCTRL, car_irq_pin_name, config);
	}
}

void car_reverse_set_io_int(void)
{
	long unsigned int config;
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 0xFFFF);
	pin_config_get(SUNXI_PINCTRL, car_irq_pin_name, &config);
	if (6 != SUNXI_PINCFG_UNPACK_VALUE(config)) {
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 6);
		pin_config_set(SUNXI_PINCTRL, car_irq_pin_name, config);
	}
}

static int car_reverse_gpio_status(void)
{
#ifdef _REVERSE_DEBUG_
	return (car_reverse->debug == CAR_REVERSE_START ? CAR_REVERSE_START
							: CAR_REVERSE_STOP);
#else
	int value = 1;
	car_reverse_set_int_ioin();
	value = gpio_get_value(car_reverse->reverse_gpio);
	car_reverse_set_io_int();
	return (value == 0) ? CAR_REVERSE_START : CAR_REVERSE_STOP;
#endif
}

/*
 *  current status | gpio status | next status
 *  ---------------+-------------+------------
 *		STOP	 |	STOP	 |	HOLD
 *  ---------------+-------------+------------
 *		STOP	 |	START	|	START
 *  ---------------+-------------+------------
 *		START	|	STOP	 |	STOP
 *  ---------------+-------------+------------
 *		START	|	START	|	HOLD
 *  ---------------+-------------+------------
 */
const int _transfer_table[3][3] = {
	[0] = {0, 0, 0},
	[CAR_REVERSE_START] = {0, CAR_REVERSE_HOLD, CAR_REVERSE_STOP},
	[CAR_REVERSE_STOP] = {0, CAR_REVERSE_START, CAR_REVERSE_HOLD},
};

static int car_reverse_get_next_status(void)
{
	int next_status;
	int gpio_status = car_reverse_gpio_status();
	int curr_status = car_reverse->status;
	car_reverse_switch_update(gpio_status == CAR_REVERSE_START ? 1 : 0);
	next_status = _transfer_table[curr_status][gpio_status];
	return next_status;
}

static void status_detect_func(struct work_struct *work)
{
	int retval;
	int status = car_reverse_get_next_status();

	if (car_reverse->standby)
		status = CAR_REVERSE_STOP;

	switch (status) {
	case CAR_REVERSE_START:
		if (!car_reverse->needexit) {
			retval = car_reverse_preview_start();
			logdebug("start car reverse, return %d\n", retval);
		}
		break;
	case CAR_REVERSE_STOP:
		retval = car_reverse_preview_stop();
		logdebug("stop car reverse, return %d\n", retval);
		break;
	case CAR_REVERSE_HOLD:
	default:
		break;
	}
	return;
}

static irqreturn_t reverse_irq_handle(int irqnum, void *data)
{
	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	return IRQ_HANDLED;
}

static ssize_t car_reverse_status_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int count = 0;

	if (car_reverse->status == CAR_REVERSE_STOP)
		count += sprintf(buf, "%s\n", "stop");
	else if (car_reverse->status == CAR_REVERSE_START)
		count += sprintf(buf, "%s\n", "start");
	else
		count += sprintf(buf, "%s\n", "unknow");
	return count;
}

static ssize_t car_reverse_format_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1))
		car_reverse->format = 1;
	else
		car_reverse->format = 0;
	return count;
}

static ssize_t car_reverse_format_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int count = 0;
	count += sprintf(buf, "%d\n", car_reverse->format);
	return count;
}

static ssize_t car_reverse_oview_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1)) {
		car_reverse->used_oview = 1;
	} else {
		car_reverse->used_oview = 0;
	}
	return count;
}

static ssize_t car_reverse_oview_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int count = 0;
	count += sprintf(buf, "%d\n", car_reverse->used_oview);
	return count;
}

static ssize_t car_reverse_src_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1)) {
		car_reverse->config.input_src = 1;
	} else {
		car_reverse->config.input_src = 0;
	}
	return count;
}

static ssize_t car_reverse_src_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int count = 0;
	count += sprintf(buf, "%d\n", car_reverse->config.input_src);
	return count;
}

static ssize_t car_reverse_rotation_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int err;
	unsigned long val;
	err = kstrtoul(buf, 10, &val); /* strict_strtoul */
	if (err) {
		return err;
	}
	car_reverse->config.rotation = (unsigned int)val;
	return count;
}

static ssize_t car_reverse_rotation_show(struct class *class,
					struct class_attribute *attr,
					char *buf)
{
	int count = 0;
	count += sprintf(buf, "%d\n", car_reverse->config.rotation);
	return count;
}

static ssize_t car_reverse_needexit_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1))
		car_reverse->needexit = 1;
	else
		car_reverse->needexit = 0;
	return count;
}

static ssize_t car_reverse_needexit_show(struct class *class,
					struct class_attribute *attr,
					char *buf)
{
	int count = 0;
	if (car_reverse->needexit == 1)
		count += sprintf(buf, "needexit = %d\n", 1);
	else
		count += sprintf(buf, "needexit = %d\n", 0);
	return count;
}
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
static ssize_t car_reverse_orientation_store(struct class *class,
						struct class_attribute *attr,
						const char *buf, size_t count)
{
	int err;
	unsigned long val;
	err = kstrtoul(buf, 10, &val); /* strict_strtoul */
	if (err) {
		return err;
	}
	car_reverse->config.car_direct = (unsigned int)val;
	return count;
}

static ssize_t car_reverse_orientation_show(struct class *class,
						struct class_attribute *attr,
						char *buf)
{
	int count = 0;
	count += sprintf(buf, "%d\n", car_reverse->config.car_direct);
	return count;
}

static ssize_t car_reverse_lrdirect_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int err;
	unsigned long val;
	err = kstrtoul(buf, 10, &val); /* strict_strtoul */
	if (err) {
		return err;
	}
	car_reverse->config.lr_direct = (unsigned int)val;
	return count;
}

static ssize_t car_reverse_lrdirect_show(struct class *class,
					struct class_attribute *attr,
					char *buf)
{
	int count = 0;
	count += sprintf(buf, "%d\n", car_reverse->config.lr_direct);
	return count;
}

static ssize_t car_reverse_mirror_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int err;
	unsigned long val;
	err = kstrtoul(buf, 10, &val); /* strict_strtoul */
	if (err) {
		return err;
	}
	car_reverse->config.pr_mirror = (unsigned int)val;
	return count;
}

static ssize_t car_reverse_mirror_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int count = 0;
	count += sprintf(buf, "%d\n", car_reverse->config.pr_mirror);
	return count;
}

#endif
#ifdef _REVERSE_DEBUG_
static ssize_t car_reverse_debug_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	if (!strncmp(buf, "stop", 4))
		car_reverse->debug = CAR_REVERSE_STOP;
	else if (!strncmp(buf, "start", 5))
		car_reverse->debug = CAR_REVERSE_START;
	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	return count;
}
#endif

static struct class_attribute car_reverse_attrs[] = {
	__ATTR(status, 0775, car_reverse_status_show, NULL),
	__ATTR(needexit, 0775, car_reverse_needexit_show,
	car_reverse_needexit_store),
	__ATTR(format, 0775, car_reverse_format_show,
	car_reverse_format_store),
	__ATTR(rotation, 0775, car_reverse_rotation_show,
	car_reverse_rotation_store),
	__ATTR(src, 0775, car_reverse_src_show, car_reverse_src_store),
	__ATTR(oview, 0775, car_reverse_oview_show, car_reverse_oview_store),
#ifdef CONFIG_SUPPORT_AUXILIARY_LINE
	__ATTR(car_mirror, 0775, car_reverse_mirror_show,
	car_reverse_mirror_store),
	__ATTR(car_direct, 0775, car_reverse_orientation_show,
	car_reverse_orientation_store),
	__ATTR(car_lr, 0775, car_reverse_lrdirect_show,
	car_reverse_lrdirect_store),
#endif
#ifdef _REVERSE_DEBUG_
	__ATTR(debug, S_IRUGO | S_IWUSR, NULL, car_reverse_debug_store),
#endif
	__ATTR_NULL};

static struct class car_reverse_class = {
	.name = "car_reverse", .class_attrs = car_reverse_attrs,
};

static int car_reverse_probe(struct platform_device *pdev)
{
	int retval = 0;
	long reverse_pin_irqnum;
#ifdef USE_SUNXI_DI_MODULE
	di_id = -1;
#endif
	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "of_node is missing\n");
		retval = -EINVAL;
		goto _err_out;
	}
	car_reverse = devm_kzalloc(
		&pdev->dev, sizeof(struct car_reverse_private_data), GFP_KERNEL);
	if (!car_reverse) {
		dev_err(&pdev->dev, "kzalloc for private data failed\n");
		retval = -ENOMEM;
		goto _err_out;
	}
	parse_config(pdev, car_reverse);
	car_reverse->config.pdev = pdev;
	car_reverse->config.dev = vin_get_dev(car_reverse->config.tvd_id);
	platform_set_drvdata(pdev, car_reverse);
	INIT_LIST_HEAD(&car_reverse->pending_frame);
	spin_lock_init(&car_reverse->display_lock);
	spin_lock_init(&car_reverse->thread_lock);
	spin_lock_init(&car_reverse->free_lock);
	car_reverse->needexit = 0;
	car_reverse->status = CAR_REVERSE_STOP;
	sunxi_gpio_to_name(car_reverse->reverse_gpio, car_irq_pin_name);
	reverse_pin_irqnum = gpio_to_irq(car_reverse->reverse_gpio);
	if (IS_ERR_VALUE(reverse_pin_irqnum)) {
		dev_err(&pdev->dev,
			"map gpio [%d] to virq failed, errno = %ld\n",
			car_reverse->reverse_gpio, reverse_pin_irqnum);
		retval = -EINVAL;
		goto _err_out;
	}
	car_reverse->preview_workqueue =
		create_singlethread_workqueue("car-reverse-wq");
	if (!car_reverse->preview_workqueue) {
		dev_err(&pdev->dev, "create workqueue failed\n");
		retval = -EINVAL;
		goto _err_out;
	}
	INIT_WORK(&car_reverse->status_detect, status_detect_func);
	class_register(&car_reverse_class);
	car_reverse_switch_register();
	car_reverse->format = 0;
	car_reverse->standby = 0;
	car_reverse->config.car_oview_mode = car_reverse->used_oview;
	car_reverse->config.format = V4L2_PIX_FMT_NV21;
#ifdef USE_YUV422
	car_reverse->config.format = V4L2_PIX_FMT_NV61;
	car_reverse->format = 1;
#endif
	car_reverse->needfree = 0;
	printk(KERN_ERR "%s:%d\n", __FUNCTION__, __LINE__);

	if (request_irq(reverse_pin_irqnum, reverse_irq_handle,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"car-reverse", pdev)) {
		dev_err(&pdev->dev, "request irq %ld failed\n",
			reverse_pin_irqnum);
		retval = -EBUSY;
		goto _err_free_buffer;
	}
	car_reverse->debug = CAR_REVERSE_STOP;
	queue_work(car_reverse->preview_workqueue, &car_reverse->status_detect);
	dev_info(&pdev->dev, "car reverse module probe ok\n");
	return 0;
_err_free_buffer:
/*
	free_buffer_pool(car_reverse->config.dev, car_reverse->buffer_pool);
	if (car_reverse->config.input_src == 0) {
		__buffer_node_free(car_reverse->config.dev,
				car_reverse->buffer_disp[0]);
		__buffer_node_free(car_reverse->config.dev,
				car_reverse->buffer_disp[1]);
	}
*/
_err_out:
	dev_err(&pdev->dev, "car reverse module exit, errno %d!\n", retval);
	return retval;
}

static int car_reverse_remove(struct platform_device *pdev)
{
	struct car_reverse_private_data *priv = car_reverse;
	/*
  free_buffer_pool(car_reverse->config.dev, car_reverse->buffer_pool);
	if (car_reverse->config.input_src == 0) {
		__buffer_node_free(car_reverse->config.dev,
				car_reverse->buffer_disp[0]);
		__buffer_node_free(car_reverse->config.dev,
				car_reverse->buffer_disp[1]);
	}
	*/
	car_reverse_switch_unregister();
	class_unregister(&car_reverse_class);
	free_irq(gpio_to_irq(priv->reverse_gpio), pdev);
	cancel_work_sync(&priv->status_detect);
	if (priv->preview_workqueue != NULL) {
		flush_workqueue(priv->preview_workqueue);
		destroy_workqueue(priv->preview_workqueue);
		priv->preview_workqueue = NULL;
	}
	dev_info(&pdev->dev, "car reverse module exit\n");
	return 0;
}

static int car_reverse_suspend(struct device *dev)
{
	int retval;
	if (car_reverse->status == CAR_REVERSE_START) {
		car_reverse->standby = 1;
		retval = car_reverse_preview_stop();
		flush_workqueue(car_reverse->preview_workqueue);
	}
	return 0;
}

static int car_reverse_resume(struct device *dev)
{

	if (car_reverse->standby) {
		car_reverse->standby = 0;
		queue_work(car_reverse->preview_workqueue,
			&car_reverse->status_detect);
	}
	return 0;
}

static const struct dev_pm_ops car_reverse_pm_ops = {
	.suspend = car_reverse_suspend, .resume = car_reverse_resume,
};

static const struct of_device_id car_reverse_dt_ids[] = {
	{.compatible = "allwinner,sunxi-car-reverse"}, {},
};

static struct platform_driver car_reverse_driver = {
	.probe = car_reverse_probe,
	.remove = car_reverse_remove,
	.driver = {
		.name = MODULE_NAME,
		.pm = &car_reverse_pm_ops,
		.of_match_table = car_reverse_dt_ids,
	},
};

static int __init car_reverse_module_init(void)
{
	int ret;
	ret = platform_driver_register(&car_reverse_driver);
	if (ret) {
		pr_err("platform driver register failed\n");
		return ret;
	}
	return 0;
}

static void __exit car_reverse_module_exit(void)
{
	platform_driver_unregister(&car_reverse_driver);
}

#ifdef CONFIG_ARCH_SUN50IW9P1
subsys_initcall_sync(car_reverse_module_init);
#else
module_init(car_reverse_module_init);
#endif
module_exit(car_reverse_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zeng.Yajian <zengyajian@allwinnertech.com>");
MODULE_DESCRIPTION("Sunxi fast car reverse image preview");

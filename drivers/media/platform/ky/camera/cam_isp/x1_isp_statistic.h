/* SPDX-License-Identifier: GPL-2.0 */
#ifndef X1_ISP_STATISTIC_H
#define X1_ISP_STATISTIC_H

#include "x1_isp_drv.h"

int x1isp_stat_node_init(struct x1isp_stats_node *stats_node, u32 hw_pipe_id);
int x1isp_stat_job_flags_init(struct x1isp_stats_node *stats_node);

/**
 * x1isp_stat_node_streamon_dma_port - config the stats node when streamon
 * @stats_node:	pointer to &struct x1isp_stats_node.
 *
 * Should be called after x1isp_stat_node_init.
 *
 * This function:
 *
 * 1) config dma port when streamon isp.
 *
 * The return values:
 * 0  : success.
 * <0 : failed.
 */
int x1isp_stat_node_streamon_dma_port(struct x1isp_stats_node *stats_node);
int x1isp_stat_node_streamoff_dma_port(struct x1isp_stats_node *stats_node);

int x1isp_stat_node_cfg_dma_irqmask(struct x1isp_stats_node *stats_node);
int x1isp_stat_node_clear_dma_irqmask(struct x1isp_stats_node *stats_node);

/**
 * x1isp_stat_reqbuffer - the function of request buffer for user space.
 * @stats_node:	pointer to &struct x1isp_stats_node.
 * @req_info: pointer to &struct isp_buffer_request_info, filled by user space.
 *
 * The return values:
 * 0  : success.
 * <0 : failed.
 */
int x1isp_stat_reqbuffer(struct x1isp_stats_node *stats_node,
			  struct isp_buffer_request_info *req_info);

int x1isp_stat_qbuffer(struct x1isp_stats_node *stats_node,
			struct isp_buffer_enqueue_info *qbuf_info);

int x1isp_stat_flush_buffer(struct x1isp_stats_node *stats_node);
int x1isp_stat_try_flush_buffer(struct x1isp_stats_node *stats_node);

/**
 * x1isp_stat_dma_irq_handler - the upper half of dma irq.
 * @stats_node:	pointer to &struct x1isp_stats_node.
 *
 * This function: record some dma irq flag when them happen.
 *
 * The return values:
 * <= 0 : no dma irq happens or some error.
 * > 0  : some dma irq happens, at least one.
 */
int x1isp_stat_dma_irq_handler(struct x1isp_stats_node *stats_node, void *irq_data);

/**
 * x1isp_stat_dma_lower_half_irq - the lower half of dma irq realized by tasklet.
 * @stats_node:	pointer to &struct x1isp_stats_node.
 * @frame_id: the current frame number.
 *
 * This function: handle some dma irq.
 *
 * The return values:
 * < 0 : some error happen.
 */
int x1isp_stat_dma_lower_half_irq(struct x1isp_stats_node *stats_node, u32 frame_id);

/**
 * x1isp_stat_get_donebuf_by_frameid - get done buffer by frameid from done queue.
 *
 * @stats_node:	pointer to &struct x1isp_stats_node.
 * @stat_id: the stat id, defined by &enum isp_stat_id
 * @frame_num: the frame number.
 * @return_idle: find the buffer return to idle queue if ture.
 *
 * This function:
 * 1. get the buffer whose frameid is equal to frame_num from done queue.
 * 2. put the buffer whose frameid is less than frame_num to idle queue.
 *
 * The return values:
 * NULL : have not found the buffer.
 * pointer to the done buffer.
 */
struct isp_kbuffer_info* x1isp_stat_get_donebuf_by_frameid(struct x1isp_stats_node *stats_node,
							    u32 stat_id, u32 frame_num, u32 return_idle);

/**
 * x1isp_stat_read_from_mem - read stat's result registers to fill the buffer.
 *
 * @stats_node:	pointer to &struct x1isp_stats_node.
 * @stat_id: the stat id, defined by &enum isp_stat_id
 * @frame_num: the frame number.
 *
 * This function likes dma sof and eof, but we do not need to busy queue:
 * 1. get the buffer from idle list.
 * 2. fill the stat result to this buffer.
 *
 * The return values:
 *   the buffer or NULL.
 */
struct isp_kbuffer_info *x1isp_stat_read_from_mem(struct x1isp_stats_node *stats_node,
						   u32 stat_id, u32 frame_num);

/**
 * x1isp_stat_mem_set_irq_flag - set start read for this stat.
 *
 * @stats_node:	pointer to &struct x1isp_stats_node.
 * @stat_id: the stat id, defined by &enum isp_stat_id.
 * @hw_pipe_id: hardware pipeline id.
 *
 * The return values:
 * < 0 : some error happen.
 */
int x1isp_stat_mem_set_irq_flag(struct x1isp_stats_node *stats_node, u32 stat_id,
				 u32 hw_pipe_id);

/**
 * x1isp_stat_mem_lower_half_irq - the lower half irq realized by tasklet for reading mem regs.
 *
 * @stats_node:	pointer to &struct x1isp_stats_node.
 * @stat_id: the stat id, defined by &enum isp_stat_id.
 * @frame_num: the current frame num.
 *
 */
void x1isp_stat_mem_lower_half_irq(struct x1isp_stats_node *stats_node, u32 stat_id,
				    u32 frame_num);

int x1isp_stat_dma_dynamic_enable(struct x1isp_stats_node *stats_node, u32 stat_id,
				   u32 enable);

/**
 * x1isp_stat_reset_dma_busybuf_frameid - reset frameid of busy buffer to zero.
 *
 * @stats_node:	pointer to &struct x1isp_stats_node.
 * 
 * This function calls when stream stop and start by frameid comes from zero.
 */
int x1isp_stat_reset_dma_busybuf_frameid(struct x1isp_stats_node *stats_node);

#endif

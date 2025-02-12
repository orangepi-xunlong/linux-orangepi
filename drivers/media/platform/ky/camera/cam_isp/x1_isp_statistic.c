// SPDX-License-Identifier: GPL-2.0
/*
 * Description on this file
 *
 * Copyright (C) 2023 KY Micro Limited
 */
#include "x1_isp_drv.h"
#include "x1_isp_reg.h"
#include "x1_isp_statistic.h"
#include <cam_plat.h>

#include <linux/dma-buf.h>

#define PIPE0_EIS_DMA_CH_ID 9
#define PIPE1_EIS_DMA_CH_ID 10

#define PIPE0_PDC_DMA_CH_ID 14
#define PIPE1_PDC_DMA_CH_ID 15

static struct stat_dma_irq_bits g_dma_irq_bits_p0[ISP_STAT_THROUGH_DMA_COUNT] = {
	{ ISP_STAT_ID_PDC, PIPE0_PDC_DMA_CH_ID, { 44, 43, 42} },	//> 32 means locate at another dma status register.
	{ ISP_STAT_ID_EIS, PIPE0_EIS_DMA_CH_ID, { 29, 28, 27} },
};

static struct stat_dma_irq_bits g_dma_irq_bits_p1[ISP_STAT_THROUGH_DMA_COUNT] = {
	{ ISP_STAT_ID_PDC, PIPE1_PDC_DMA_CH_ID, { 47, 46, 45} },	//> 32 means locate at another dma status register.
	{ ISP_STAT_ID_EIS, PIPE1_EIS_DMA_CH_ID, { 32, 31, 30} },	//> 32 means locate at another dma status register.
};

static int g_mem_stat_ids[ISP_STAT_THROUGH_MEM_COUNT] = {
	ISP_STAT_ID_AE,
	ISP_STAT_ID_AWB,
	ISP_STAT_ID_AF,
	ISP_STAT_ID_LTM,
};

int isp_stat_dma_sof_handler(struct x1isp_stats_node *stats_node, u32 stat_id,
			     u32 dma_ch_id, u32 frame_id);

static void x1isp_stat_bufqueue_init(struct x1isp_stats_node *isp_stats_node)
{
	int i = 0;
	struct isp_stat_buffer_queue *stat_bufqueue = NULL;

	for (i = 0; i < ISP_STAT_ID_MAX; i++) {
		/* array of bufferque is  AE,AWB,EIS,AF */
		stat_bufqueue = &isp_stats_node->stat_bufqueue[i];
		stat_bufqueue->stat_id = i;
		stat_bufqueue->buf_count = 0;
		stat_bufqueue->busy_bufcnt = 0;
		stat_bufqueue->idle_bufcnt = 0;
		stat_bufqueue->fd_memory = true;
		if (i != ISP_STAT_ID_PDC && i != ISP_STAT_ID_EIS)
			stat_bufqueue->fill_by_cpu = true;
		else
			stat_bufqueue->fill_by_cpu = false;
		spin_lock_init(&stat_bufqueue->queue_lock);
		INIT_LIST_HEAD(&stat_bufqueue->busy_buflist);
		INIT_LIST_HEAD(&stat_bufqueue->idle_buflist);
		memset((void *)stat_bufqueue->buf_info, 0, sizeof(stat_bufqueue->buf_info));

		//done list init.
		isp_stats_node->stat_done_info[i].done_cnt = 0;
		INIT_LIST_HEAD(&isp_stats_node->stat_done_info[i].done_list);
		spin_lock_init(&isp_stats_node->stat_done_info[i].done_lock);
	}
}

int x1isp_stat_job_flags_init(struct x1isp_stats_node *stats_node)
{
	int i, j;

	ISP_DRV_CHECK_POINTER(stats_node);
	//dma irq flag init
	for (i = 0; i < ISP_STAT_THROUGH_DMA_COUNT; i++) {
		for (j = 0; j < ISP_DMA_IRQ_TYPE_NUM; j++) {
			stats_node->dma_irq_info[i].irq_flag[j] = false;
			stats_node->dma_irq_info[i].dynamic_switch = 0;
			stats_node->dma_irq_info[i].dynamic_trigger = 0;
		}
	}

	//mem stat irq flag init.
	for (i = 0; i < ISP_STAT_THROUGH_MEM_COUNT; i++)
		stats_node->mem_irq_info[i].start_read = false;

	return 0;
}

int x1isp_stat_node_init(struct x1isp_stats_node *stats_node, u32 hw_pipe_id)
{
	int i = 0, stat_id = 0;

	ISP_DRV_CHECK_POINTER(stats_node);

	stats_node->hw_pipe_id = hw_pipe_id;
	for (i = 0; i < ISP_STAT_ID_MAX; i++) {
		stats_node->stat_active[i] = true;
		stats_node->mem_irq_index[i] = -1;
	}

	x1isp_stat_bufqueue_init(stats_node);

	//irq bit init.
	if (ISP_HW_PIPELINE_ID_0 == hw_pipe_id)
		memcpy(stats_node->stat_dma_irq_bitmap, g_dma_irq_bits_p0, sizeof(g_dma_irq_bits_p0));
	else if (ISP_HW_PIPELINE_ID_1 == hw_pipe_id)
		memcpy(stats_node->stat_dma_irq_bitmap, g_dma_irq_bits_p1, sizeof(g_dma_irq_bits_p1));
	//dma irq info init.
	memset(&stats_node->dma_irq_info, 0, sizeof(stats_node->dma_irq_info));
	for (i = 0; i < ISP_STAT_THROUGH_DMA_COUNT; i++) {
		stats_node->dma_irq_info[i].stat_id =
		    stats_node->stat_dma_irq_bitmap[i].stat_id;
		spin_lock_init(&stats_node->dma_irq_info[i].flag_lock);
	}

	//mem stat irq info init.
	for (i = 0; i < ISP_STAT_THROUGH_MEM_COUNT; i++) {
		stat_id = g_mem_stat_ids[i];
		stats_node->mem_irq_info[i].stat_id = stat_id;
		stats_node->mem_irq_index[stat_id] = i;
		spin_lock_init(&stats_node->mem_irq_info[i].mem_flag_lock);
	}

	return 0;
}

int x1isp_stat_node_streamon_dma_port(struct x1isp_stats_node *stats_node)
{
	int ret = 0, array_index = 0, dma_cnt, dma_ch_id;
	struct isp_reg_unit reg_unit[10] = { 0 };
	struct isp_stat_buffer_queue *stat_bufqueue = NULL;
	u32 reg_tmp_value = 0;

	ISP_DRV_CHECK_POINTER(stats_node);
	for (dma_cnt = 0; dma_cnt < ISP_STAT_THROUGH_DMA_COUNT; dma_cnt++) { // normal is 2.
		dma_ch_id = stats_node->stat_dma_irq_bitmap[dma_cnt].dma_ch_id;
		if (PIPE0_EIS_DMA_CH_ID == dma_ch_id || PIPE1_EIS_DMA_CH_ID == dma_ch_id) {
			//dma mux only for eis
			reg_unit[array_index].reg_addr = REG_ISP_OFFSET_DMA_MUX_CTRL;
			reg_unit[array_index].reg_value = 3 << (6 + ((dma_ch_id - PIPE0_EIS_DMA_CH_ID) * 2));
			reg_unit[array_index].reg_mask = reg_unit[array_index].reg_value;
			array_index++;

			//pitch
			stat_bufqueue = &stats_node->stat_bufqueue[ISP_STAT_ID_EIS];
			if (0 == stat_bufqueue->buf_count) {
				isp_log_err("no buffer for eis stat queue on pipe%d!", stats_node->hw_pipe_id);
				return -EPERM;
			}
			reg_tmp_value = stat_bufqueue->buf_info[0].buf_planes[0].pitch & 0xffff;
			reg_tmp_value |= (stat_bufqueue->buf_info[0].buf_planes[1].pitch << 16) & 0xffff0000;
			reg_unit[array_index].reg_addr = REG_ISP_DMA_CHANNEL_WR_PITCH(dma_ch_id);
			reg_unit[array_index].reg_value = reg_tmp_value;
			reg_unit[array_index].reg_mask = 0xffffffff;
			array_index++;
		}

		//for the first frame addr and ready.
		ret = isp_stat_dma_sof_handler(stats_node, stats_node->stat_dma_irq_bitmap[dma_cnt].stat_id, dma_ch_id, 0);
		if (ret)
			return ret;
	}

	if (array_index > 0)
		x1isp_reg_write_brust(reg_unit, array_index, false, NULL);

	return ret;
}

static int isp_stat_node_clear_dma_ready(u32 dma_ch_id)
{
	int ret = 0, i, array_index = 0;
	u32 reg_base_addr = 0, plane_count = 0;
	struct isp_reg_unit reg_unit[10] = { 0 };

	if (dma_ch_id < PIPE0_PDC_DMA_CH_ID) {
		//dma channel y and uv addr.
		reg_base_addr = REG_ISP_DMA_Y_ADDR(dma_ch_id);
		plane_count = 2;
	} else if (dma_ch_id <= PIPE1_PDC_DMA_CH_ID) {
		//pdc dma
		reg_base_addr = REG_ISP_PDC_DMA_BASE_ADDR(dma_ch_id - PIPE0_PDC_DMA_CH_ID);
		plane_count = 4;
	} else {
		isp_log_err("unknown this dma ch ID:%d!", dma_ch_id);
		return -EINVAL;
	}

	for (i = 0; i < plane_count; i++) {
		reg_unit[array_index].reg_addr = reg_base_addr + (i * 0x4);
		reg_unit[array_index].reg_value = 0;
		reg_unit[array_index].reg_mask = 0xffffffff;
		array_index++;
	}

	//dma channel ready.
	if (dma_ch_id < PIPE0_PDC_DMA_CH_ID) {
		reg_unit[array_index].reg_addr = REG_ISP_DMA_CHANNEL_MASTER(dma_ch_id);
		reg_unit[array_index].reg_value = 0;
		reg_unit[array_index].reg_mask = BIT(31);
		array_index++;
	} else if (dma_ch_id <= PIPE1_PDC_DMA_CH_ID) {
		reg_unit[array_index].reg_addr = REG_ISP_PDC_DMA_MASTER;
		if (PIPE0_PDC_DMA_CH_ID == dma_ch_id)
			reg_unit[array_index].reg_mask = BIT(30);
		else
			reg_unit[array_index].reg_mask = BIT(31);
		reg_unit[array_index].reg_value = 0;
		array_index++;
	}

	x1isp_reg_write_brust(reg_unit, array_index, false, NULL);

	return ret;
}

int x1isp_stat_node_streamoff_dma_port(struct x1isp_stats_node *stats_node)
{
	int ret = 0, array_index = 0, dma_cnt, dma_ch_id;
	struct isp_reg_unit reg_unit[10] = { 0 };

	ISP_DRV_CHECK_POINTER(stats_node);
	for (dma_cnt = 0; dma_cnt < ISP_STAT_THROUGH_DMA_COUNT; dma_cnt++) {	// normal is 2.
		dma_ch_id = stats_node->stat_dma_irq_bitmap[dma_cnt].dma_ch_id;
		if (PIPE0_EIS_DMA_CH_ID == dma_ch_id
		    || PIPE1_EIS_DMA_CH_ID == dma_ch_id) {
			//dma mux only for eis
			reg_unit[array_index].reg_addr = REG_ISP_OFFSET_DMA_MUX_CTRL;
			reg_unit[array_index].reg_value = 0;
			reg_unit[array_index].reg_mask =
			    3 << (6 + ((dma_ch_id - PIPE0_EIS_DMA_CH_ID) * 2));
			array_index++;

			//pitch
			reg_unit[array_index].reg_addr =
			    REG_ISP_DMA_CHANNEL_WR_PITCH(dma_ch_id);
			reg_unit[array_index].reg_value = 0;
			reg_unit[array_index].reg_mask = 0xffffffff;
			array_index++;
		}
		//clear  addr and ready.
		isp_stat_node_clear_dma_ready(dma_ch_id);
	}

	if (array_index > 0)
		x1isp_reg_write_brust(reg_unit, array_index, false, NULL);

	return ret;
}

int x1isp_stat_node_cfg_dma_irqmask(struct x1isp_stats_node *stats_node)
{
	int ret = 0, array_index = 0, dma_cnt, dma_ch_id;
	struct isp_reg_unit reg_unit[10] = { 0 };

	ISP_DRV_CHECK_POINTER(stats_node);
	for (dma_cnt = 0; dma_cnt < ISP_STAT_THROUGH_DMA_COUNT; dma_cnt++) {	// normal is 2.
		dma_ch_id = stats_node->stat_dma_irq_bitmap[dma_cnt].dma_ch_id;
		if (PIPE0_EIS_DMA_CH_ID == dma_ch_id) {
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK1;
			reg_unit[array_index].reg_value = BIT(27) | BIT(28) | BIT(29);
			reg_unit[array_index].reg_mask =
			    reg_unit[array_index].reg_value;
		} else if (PIPE1_EIS_DMA_CH_ID == dma_ch_id) {
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK1;
			reg_unit[array_index].reg_value = BIT(30) | BIT(31);
			reg_unit[array_index].reg_mask =
			    reg_unit[array_index].reg_value;
			array_index++;
			//error
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK2;
			reg_unit[array_index].reg_value = BIT(0);
			reg_unit[array_index].reg_mask =
			    reg_unit[array_index].reg_value;
		} else if (PIPE0_PDC_DMA_CH_ID == dma_ch_id) {
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK2;
			reg_unit[array_index].reg_value = BIT(10) | BIT(11) | BIT(12);
			reg_unit[array_index].reg_mask =
			    reg_unit[array_index].reg_value;
		} else if (PIPE1_PDC_DMA_CH_ID == dma_ch_id) {
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK2;
			reg_unit[array_index].reg_value = BIT(13) | BIT(14) | BIT(15);
			reg_unit[array_index].reg_mask =
			    reg_unit[array_index].reg_value;
		}

		array_index++;
	}

	x1isp_reg_write_brust(reg_unit, array_index, false, NULL);

	return ret;
}

int x1isp_stat_node_clear_dma_irqmask(struct x1isp_stats_node *stats_node)
{
	int ret = 0, array_index = 0, dma_cnt, dma_ch_id;
	struct isp_reg_unit reg_unit[10] = { 0 };

	ISP_DRV_CHECK_POINTER(stats_node);
	for (dma_cnt = 0; dma_cnt < ISP_STAT_THROUGH_DMA_COUNT; dma_cnt++) {	// normal is 2.
		dma_ch_id = stats_node->stat_dma_irq_bitmap[dma_cnt].dma_ch_id;
		if (PIPE0_EIS_DMA_CH_ID == dma_ch_id) {
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK1;
			reg_unit[array_index].reg_mask = BIT(27) | BIT(28) | BIT(29);
		} else if (PIPE1_EIS_DMA_CH_ID == dma_ch_id) {
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK1;
			reg_unit[array_index].reg_mask = BIT(30) | BIT(31);
			array_index++;
			//error
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK2;
			reg_unit[array_index].reg_mask = BIT(0);
		} else if (PIPE0_PDC_DMA_CH_ID == dma_ch_id) {
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK2;
			reg_unit[array_index].reg_mask = BIT(10) | BIT(11) | BIT(12);
		} else if (PIPE1_PDC_DMA_CH_ID == dma_ch_id) {
			reg_unit[array_index].reg_addr = REG_ISP_DMA_IRQ_MASK2;
			reg_unit[array_index].reg_mask = BIT(13) | BIT(14) | BIT(15);
		}

		reg_unit[array_index].reg_value = 0;
		array_index++;
	}

	x1isp_reg_write_brust(reg_unit, array_index, false, NULL);

	return ret;
}

//thread context
int x1isp_stat_dma_dynamic_enable(struct x1isp_stats_node *stats_node, u32 stat_id,
				   u32 enable)
{
	int ret = 0, i;

	ISP_DRV_CHECK_POINTER(stats_node);
	ISP_DRV_CHECK_PARAMETERS(stat_id, ISP_STAT_ID_PDC, ISP_STAT_ID_EIS,
				 "dma stat id");

	for (i = 0; i < ISP_STAT_THROUGH_DMA_COUNT; i++) {
		if (stat_id == stats_node->dma_irq_info[i].stat_id) {
			if (enable)
				stats_node->dma_irq_info[i].dynamic_switch =
				    STAT_DMA_SWITCH_DYNAMIC_ON;
			else
				stats_node->dma_irq_info[i].dynamic_switch =
				    STAT_DMA_SWITCH_DYNAMIC_OFF;
			break;
		}
	}

	return ret;
}

static int x1isp_stat_put_idlebuffer(struct isp_stat_buffer_queue *stat_bufqueue,
			       struct isp_kbuffer_info *kbuf_info)
{
	int ret = 0;

	ISP_DRV_CHECK_POINTER(stat_bufqueue);
	ISP_DRV_CHECK_POINTER(kbuf_info);

	//lock used only between thread and soft irq.
	spin_lock_bh(&stat_bufqueue->queue_lock);
	kbuf_info->frame_id = -1;
	kbuf_info->buf_status = ISP_BUFFER_STATUS_IDLE;
	list_add_tail(&kbuf_info->hook, &stat_bufqueue->idle_buflist);
	stat_bufqueue->idle_bufcnt++;
	spin_unlock_bh(&stat_bufqueue->queue_lock);

	return ret;
}

int x1isp_stat_reqbuffer(struct x1isp_stats_node *stats_node,
			  struct isp_buffer_request_info *req_info)
{
	int i = 0, buf_index = 0;
	struct isp_stat_buffer_queue *stat_bufqueue = NULL;

	ISP_DRV_CHECK_POINTER(stats_node);
	ISP_DRV_CHECK_POINTER(req_info);

	stat_bufqueue = stats_node->stat_bufqueue;
	for (i = 0; i < ISP_STAT_ID_MAX; i++) {
		if (req_info->stat_buf_count[i] > X1_ISP_MAX_BUFFER_NUM)
			req_info->stat_buf_count[i] = X1_ISP_MAX_BUFFER_NUM;

		stat_bufqueue[i].buf_count = req_info->stat_buf_count[i];
		if (req_info->stat_buf_count[i] == 0 && stats_node->stat_active[i]) {
			isp_log_err("zero buffer count, but we need this stat(%d)!", i);
			return -EPERM;
		}

		for (buf_index = 0; buf_index < stat_bufqueue[i].buf_count; buf_index++) {
			stat_bufqueue[i].buf_info[buf_index].buf_status = ISP_BUFFER_STATUS_DONE;
			INIT_LIST_HEAD(&stat_bufqueue[i].buf_info[buf_index].hook);
			stat_bufqueue[i].buf_info[buf_index].buf_index = buf_index;
		}
	}

	return 0;
}

static int isp_stat_buffer_verfied(struct isp_kbuffer_info *kbuf_info, struct isp_ubuf_uint *ubuf_uint, u8 fd_memory)
{
	int verified = 1, i = 0;

	if (kbuf_info->plane_count != ubuf_uint->plane_count)
		return 0;

	if (fd_memory) {
		for (i = 0; i < ubuf_uint->plane_count; i++) {
			if ((kbuf_info->buf_planes[i].m.fd != ubuf_uint->buf_planes[i].m.fd)
				|| (kbuf_info->buf_planes[i].length != ubuf_uint->buf_planes[i].length)) {
				verified = 0;
				break;
			}
		}
	} else {
		for (i = 0; i < ubuf_uint->plane_count; i++) {
			if ((kbuf_info->buf_planes[i].m.phy_addr != ubuf_uint->buf_planes[i].m.phy_addr)
				|| (kbuf_info->buf_planes[i].length != ubuf_uint->buf_planes[i].length)) {
				verified = 0;
				break;
			}
		}
	}

	return verified;
}

static void _isp_stat_put_kvir_addr(u8 fd_memory, struct isp_kbuffer_info *kbuf_info)
{
	if (kbuf_info->kvir_addr) {
		if (fd_memory) {
			x1isp_dev_put_viraddr_to_dma_buf(kbuf_info->dma_buffer,
				(char*)kbuf_info->kvir_addr - kbuf_info->buf_planes[0].offset);
			kbuf_info->kvir_addr = NULL;
			dma_buf_put(kbuf_info->dma_buffer);
			kbuf_info->dma_buffer = NULL;
		}
	}
}

static int _isp_stat_get_kvir_addr(u8 fd_memory, struct isp_kbuffer_info *kbuf_info)
{
	int ret = 0;
	void *vir_addr = NULL;
	struct dma_buf *dma_buffer = NULL;

	if (fd_memory) {
		if (kbuf_info->kvir_addr)
			_isp_stat_put_kvir_addr(fd_memory, kbuf_info);

		dma_buffer = dma_buf_get(kbuf_info->buf_planes[0].m.fd);
		if (IS_ERR(dma_buffer)) {
			isp_log_err("%s: get dma buffer failed!", __func__);
			return -EBADF;
		}

		kbuf_info->dma_buffer = dma_buffer;
		ret = x1isp_dev_get_viraddr_from_dma_buf(dma_buffer, &vir_addr);
		if (ret)
			kbuf_info->kvir_addr = NULL;
		else
			kbuf_info->kvir_addr = (void*)((char*)vir_addr + kbuf_info->buf_planes[0].offset);
	} else {
		isp_log_err("%s:we get viraddr just only support fd memory!", __func__);
		ret = -1;
	}

	return ret;
}

static int _isp_stat_get_phy_addr(u8 fd_memory, struct isp_kbuffer_info *kbuf_info)
{
	int ret = 0;
	u64 phy_addr = 0;

	if (fd_memory) {
		ret = x1isp_dev_get_phyaddr_from_dma_buf(kbuf_info->buf_planes[0].m.fd, &phy_addr);
		if (ret)
			kbuf_info->phy_addr = 0;
		else
			//multi stats alloc a whole buffer together, so we need offset.x
			kbuf_info->phy_addr = phy_addr + kbuf_info->buf_planes[0].offset;
	} else {
		isp_log_err("%s:we get phyaddr just only support fd memory!", __func__);
		ret = -1;
	}

	return ret;
}

int x1isp_stat_qbuffer(struct x1isp_stats_node *stats_node,
			struct isp_buffer_enqueue_info *qbuf_info)
{
	struct isp_stat_buffer_queue *buf_queue = NULL;
	struct isp_kbuffer_info *kbuf_info = NULL;
	int i = 0, buf_index = -1, ret = 0, plane_size = 0, verified = 0;
	struct isp_ubuf_uint *ubuf_uint = NULL;

	ISP_DRV_CHECK_POINTER(stats_node);
	ISP_DRV_CHECK_POINTER(qbuf_info);

	for (i = 0; i < ISP_STAT_ID_MAX; i++) {
		buf_queue = &stats_node->stat_bufqueue[i];
		ubuf_uint = &qbuf_info->ubuf_uint[i];
		buf_index = ubuf_uint->buf_index;

		//this stat may have no buffer this time.
		if (ubuf_uint->plane_count == 0)
			continue;

		//this stat may have no buffer this time.
		if (buf_index >= buf_queue->buf_count || buf_index < 0)
			continue;

		if (ubuf_uint->plane_count > X1_ISP_MAX_PLANE_NUM) {
			isp_log_err
			    ("the planeNum(%d) is valid in stat(%d) for enqueue on pipe%d!",
			     ubuf_uint->plane_count, i, stats_node->hw_pipe_id);
			return -EINVAL;
		}

		if (ubuf_uint->buf_planes[0].m.fd == 0) {
			isp_log_err
			    ("the buffer(%d) fd is zeor in stat(%d) for enqueue on pipe%d!",
			     buf_index, i, stats_node->hw_pipe_id);
			return -EINVAL;
		}

		kbuf_info = &buf_queue->buf_info[buf_index];
		if (kbuf_info->buf_status != ISP_BUFFER_STATUS_DONE) {
			isp_log_err
			    ("the status(%d) of buffer(%d) isn't dequeued on bufque(%d)!",
			     kbuf_info->buf_status, buf_index, i);
			return -EPERM;
		}

		verified = isp_stat_buffer_verfied(kbuf_info, ubuf_uint, buf_queue->fd_memory);
		if (!verified) {
			//buffer has not been already verified
			// kbuf_info->buf_index = buf_index;
			kbuf_info->plane_count = ubuf_uint->plane_count;
			plane_size = sizeof(struct isp_buffer_plane) * ubuf_uint->plane_count;
			memcpy(kbuf_info->buf_planes, ubuf_uint->buf_planes, plane_size);

			if (buf_queue->fill_by_cpu) {
				//some stat result read from regs,so we need kernel virtual addr.
				ret = _isp_stat_get_kvir_addr(buf_queue->fd_memory, kbuf_info);
			} else {
				//dma fill, need phy addr
				ret = _isp_stat_get_phy_addr(buf_queue->fd_memory, kbuf_info);
			}

			if (ret) {
				isp_log_err
				    ("get kaddr or phyaddr failed in stat(%d) for pipe%d!",
				     i, stats_node->hw_pipe_id);
				ret = -EPERM;
				goto Error_Exit;
			}
		}

		x1isp_stat_put_idlebuffer(buf_queue, kbuf_info);
		isp_log_dbg
		    ("the %d stat has index(%d) buffer,fd=%d, kaddr=0x%lx, phy=0x%llx!\n",
		     i, buf_index, kbuf_info->buf_planes[0].m.fd,
		     (unsigned long)kbuf_info->kvir_addr, kbuf_info->phy_addr);
	}

	return ret;

Error_Exit:
	for (i = 0; i < ISP_STAT_ID_MAX; i++) {
		buf_queue = &stats_node->stat_bufqueue[i];
		ubuf_uint = &qbuf_info->ubuf_uint[i];
		if (ubuf_uint->plane_count > 0 && buf_queue->fill_by_cpu) {
			kbuf_info = &buf_queue->buf_info[ubuf_uint->buf_index];
			_isp_stat_put_kvir_addr(buf_queue->fd_memory, kbuf_info);
		}
	}

	return ret;
}

int x1isp_stat_flush_buffer(struct x1isp_stats_node *stats_node)
{
	int i = 0, j = 0;
	struct isp_stat_buffer_queue *buf_queue = NULL;
	struct isp_kbuffer_info *kbuf_info = NULL;

	ISP_DRV_CHECK_POINTER(stats_node);

	for (i = 0; i < ISP_STAT_ID_MAX; i++) {
		buf_queue = &stats_node->stat_bufqueue[i];
		if (buf_queue->fill_by_cpu) {
			for (j = 0; j < buf_queue->buf_count; j++) {
				kbuf_info = &buf_queue->buf_info[j];
				_isp_stat_put_kvir_addr(buf_queue->fd_memory, kbuf_info);
				kbuf_info->buf_status = ISP_BUFFER_STATUS_INVALID;
			}
		}
		isp_log_dbg("flush the %d stat bufferque!", i);
	}

	x1isp_stat_bufqueue_init(stats_node);
	return 0;
}

//call at close pipe dev for abnormal
int x1isp_stat_try_flush_buffer(struct x1isp_stats_node *stats_node)
{
	int i, j, reinit = 0;
	struct isp_stat_buffer_queue *buf_queue = NULL;
	struct isp_kbuffer_info *kbuf_info = NULL;

	ISP_DRV_CHECK_POINTER(stats_node);
	for (i = 0; i < ISP_STAT_ID_MAX; i++) {
		buf_queue = &stats_node->stat_bufqueue[i];
		if (buf_queue->buf_count > 0 && buf_queue->fill_by_cpu) {
			for (j = 0; j < buf_queue->buf_count; j++) {
				kbuf_info = &buf_queue->buf_info[j];
				_isp_stat_put_kvir_addr(buf_queue->fd_memory, kbuf_info);
				kbuf_info->buf_status = ISP_BUFFER_STATUS_INVALID;
			}
			reinit = 1;
		}
	}

	if (reinit) {
		x1isp_stat_bufqueue_init(stats_node);
		isp_log_info("flush stat bufferque for abnormal situation!");
	}

	return 0;
}

/*get idle buffer and put into busy queue.*/
static struct isp_kbuffer_info *x1isp_stat_get_idlebuffer(struct isp_stat_buffer_queue
						    *buffer_queue, u32 hw_pipe_id)
{
	struct isp_kbuffer_info *kbuf_info = NULL;

	//lock used only between thread and soft irq.
	spin_lock_bh(&buffer_queue->queue_lock);

	if (buffer_queue->busy_bufcnt > 1) {
		/* the previous eof may lost, use this busy buffer. */
		kbuf_info =
		    list_first_entry(&buffer_queue->busy_buflist, struct isp_kbuffer_info, hook);
		list_del_init(&kbuf_info->hook);
		buffer_queue->busy_bufcnt--;
		isp_log_warn
		    ("pre eof lost in stat(%d) on pipeid: %d, bufindex:%d,status:%d,fn:%d,cnt=%d!",
		     buffer_queue->stat_id, hw_pipe_id, kbuf_info->buf_index,
		     kbuf_info->buf_status, kbuf_info->frame_id,
		     buffer_queue->busy_bufcnt);
	} else {
		if (buffer_queue->idle_bufcnt) {
			kbuf_info =
			    list_first_entry(&buffer_queue->idle_buflist, struct isp_kbuffer_info, hook);
			list_del_init(&kbuf_info->hook);
			buffer_queue->idle_bufcnt--;
		}
	}

	spin_unlock_bh(&buffer_queue->queue_lock);
	return kbuf_info;
}

static int x1isp_stat_put_busybuffer(struct isp_stat_buffer_queue *stat_bufqueue,
			       struct isp_kbuffer_info *kbuf_info)
{
	int ret = 0;

	ISP_DRV_CHECK_POINTER(stat_bufqueue);
	ISP_DRV_CHECK_POINTER(kbuf_info);

	//lock used only between thread and soft irq.
	spin_lock_bh(&stat_bufqueue->queue_lock);
	kbuf_info->buf_status = ISP_BUFFER_STATUS_BUSY;
	list_add_tail(&kbuf_info->hook, &stat_bufqueue->busy_buflist);
	stat_bufqueue->busy_bufcnt++;
	spin_unlock_bh(&stat_bufqueue->queue_lock);

	return ret;
}

static struct isp_kbuffer_info* x1isp_stat_get_busybuffer(struct isp_stat_buffer_queue *stat_bufqueue, u32 wr_err, u32 hw_pipe_id)
{
	struct isp_kbuffer_info *kbuf_info = NULL;

	if (!stat_bufqueue) {
		isp_log_err("%s: Invalid pointer!", __func__);
		return NULL;
	}
	//lock used only between thread and soft irq.
	spin_lock_bh(&stat_bufqueue->queue_lock);

	if (stat_bufqueue->busy_bufcnt) {
		kbuf_info = list_first_entry(&stat_bufqueue->busy_buflist, struct isp_kbuffer_info, hook);
		if (!wr_err) {
			//eof: just take out from list.
			list_del_init(&kbuf_info->hook);
			stat_bufqueue->busy_bufcnt--;
			if (kbuf_info->buf_status != ISP_BUFFER_STATUS_BUSY && kbuf_info->buf_status != ISP_BUFFER_STATUS_ERROR) {
				isp_log_err("buffer(%d,%d) busy status can't match(%d) on stat%d!", kbuf_info->buf_index,
					    kbuf_info->frame_id, kbuf_info->buf_status, stat_bufqueue->stat_id);
				kbuf_info->buf_status = ISP_BUFFER_STATUS_ERROR;
			}
		} else {
			/* dma write err: keep the buffer on the list, because eof may come out after err.
			 * If no eof come out, we should find lost pre eof at the next sof.
			 */
			kbuf_info->buf_status = ISP_BUFFER_STATUS_ERROR;
		}
	} else
		isp_log_err("there is no buffer in stat(%d) busy queue on pipe%d!", stat_bufqueue->stat_id, hw_pipe_id);

	spin_unlock_bh(&stat_bufqueue->queue_lock);

	return kbuf_info;
}

static int x1isp_stat_put_donebuffer(u32 stat_id, struct isp_stat_done_info *stat_done_info,
			       struct isp_kbuffer_info *kbuf_info)
{
	int ret = 0;

	ISP_DRV_CHECK_POINTER(stat_done_info);
	ISP_DRV_CHECK_POINTER(kbuf_info);

	//lock used only between thread and soft irq.
	spin_lock_bh(&stat_done_info->done_lock);
	kbuf_info->buf_status = ISP_BUFFER_STATUS_DONE;
	list_add_tail(&kbuf_info->hook, &stat_done_info->done_list);
	stat_done_info->done_cnt++;
	spin_unlock_bh(&stat_done_info->done_lock);

	if (stat_done_info->done_cnt > 1) {
		isp_log_dbg("previous stat(%d) buffer may not deal on time,count=%d!",
			    stat_id, stat_done_info->done_cnt);
	}

	return ret;
}

struct isp_kbuffer_info* x1isp_stat_get_donebuf_by_frameid(struct x1isp_stats_node *stats_node,
							u32 stat_id, u32 frame_num, u32 return_idle)
{
	struct isp_stat_done_info *stat_done_info = NULL;
	struct isp_kbuffer_info *kbuf_info = NULL;
	struct isp_kbuffer_info *kbuf_idle[X1_ISP_MAX_BUFFER_NUM] = {NULL, NULL, NULL, NULL};
	struct list_head *pos, *n;
	int find_item = 0, idle_buf_cnt = 0, i = 0;

	if (!stats_node) {
		return NULL;
	}

	stat_done_info = &stats_node->stat_done_info[stat_id];
	//lock used only between thread and soft irq.
	spin_lock_bh(&stat_done_info->done_lock);

	list_for_each_safe(pos, n, &stat_done_info->done_list) {
		kbuf_info = list_entry(pos, struct isp_kbuffer_info, hook);
		if (kbuf_info->frame_id <= frame_num) {
			list_del_init(&kbuf_info->hook);
			stat_done_info->done_cnt--;
			if (kbuf_info->frame_id == frame_num) {
				if (!return_idle) {
					find_item = 1;
				} else {
					kbuf_idle[idle_buf_cnt] = kbuf_info;
					idle_buf_cnt++;
				}

				break;
			} else {
				// we have to put the buffer whose frameid is less than frame_num to idle list.
				isp_log_dbg
				    ("find stat%d buf(frame%d,index%d,cnt=%d), but we need frame%d!",
				     stat_id, kbuf_info->frame_id, kbuf_info->buf_index,
				     stat_done_info->done_cnt, frame_num);
				kbuf_idle[idle_buf_cnt] = kbuf_info;
				idle_buf_cnt++;
			}
		}
	}

	if (!find_item) {
		kbuf_info = NULL;
		isp_log_dbg("there is no buffer in stat(%d) done queue on pipe%d!",
			    stat_id, stats_node->hw_pipe_id);
	}

	spin_unlock_bh(&stat_done_info->done_lock);

	for (i = 0; i < idle_buf_cnt; i ++)
		x1isp_stat_put_idlebuffer(&stats_node->stat_bufqueue[stat_id], kbuf_idle[i]);

	return kbuf_info;
}

int x1isp_stat_reset_dma_busybuf_frameid(struct x1isp_stats_node *stats_node)
{
	struct isp_kbuffer_info *kbuf_info = NULL;
	struct list_head *pos, *n;
	u32 stat_id, dma_cnt;
	struct isp_stat_buffer_queue *buffer_queue = NULL;

	ISP_DRV_CHECK_POINTER(stats_node);

	for (dma_cnt = 0; dma_cnt < ISP_STAT_THROUGH_DMA_COUNT; dma_cnt++) {	// normal is 2.
		stat_id = stats_node->stat_dma_irq_bitmap[dma_cnt].stat_id;
		buffer_queue = &stats_node->stat_bufqueue[stat_id];
		//lock used only between thread and soft irq.
		spin_lock_bh(&buffer_queue->queue_lock);
		list_for_each_safe(pos, n, &buffer_queue->busy_buflist) {
			kbuf_info = list_entry(pos, struct isp_kbuffer_info, hook);
			kbuf_info->frame_id = 0;
		}
		spin_unlock_bh(&buffer_queue->queue_lock);
	}

	return 0;
}

// the upper half irq context
int x1isp_stat_dma_irq_handler(struct x1isp_stats_node *stats_node, void *irq_data)
{
	struct stat_dma_irq_bits *dma_irq_bitmap = NULL;
	struct stat_dma_irq_info *dma_irq_info = NULL;
	int dma_cnt = 0, type_index = 0, irq_happen = 0;
	u32 dma_status = 0, irq_bit = 0;
	struct dma_irq_data *dma_irq = (struct dma_irq_data *)irq_data;

	ISP_DRV_CHECK_POINTER(stats_node);

	dma_irq_bitmap = stats_node->stat_dma_irq_bitmap;
	dma_irq_info = stats_node->dma_irq_info;
	for (dma_cnt = 0; dma_cnt < ISP_STAT_THROUGH_DMA_COUNT; dma_cnt++) {	// normal is 2.
		spin_lock(&dma_irq_info[dma_cnt].flag_lock);
		for (type_index = 0; type_index < ISP_DMA_IRQ_TYPE_NUM; type_index++) {
			irq_bit = dma_irq_bitmap[dma_cnt].irq_bit[type_index];
			if (irq_bit >= ISP_STAT_DMA_IRQ_BIT_MAX) {
				//use dma1 status
				irq_bit = irq_bit - ISP_STAT_DMA_IRQ_BIT_MAX;
				dma_status = dma_irq->status2;
			} else
				dma_status = dma_irq->status1;

			if (dma_status & BIT(irq_bit)) {
				dma_irq_info[dma_cnt].irq_flag[type_index] = true;
				if (!irq_happen)
					irq_happen = true;
			}
		}
		spin_unlock(&dma_irq_info[dma_cnt].flag_lock);
	}

	return irq_happen;
}

//tasklet context
static int isp_stat_dma_err_handler(struct x1isp_stats_node *stats_node, u32 stat_id)
{
	int ret = 0;
	struct isp_stat_buffer_queue *buffer_queue = NULL;

	buffer_queue = &stats_node->stat_bufqueue[stat_id];
	x1isp_stat_get_busybuffer(buffer_queue, 1, stats_node->hw_pipe_id);

	return ret;
}

/*the actiual dma write size of pdc is changing by hardware*/
static int isp_stat_get_pdc_real_ch_size(u32 hw_pipe_id, struct isp_kbuffer_info *kbuf_info)
{
	int ret = 0, i;
	ulong reg_addr = 0;

	ISP_DRV_CHECK_POINTER(kbuf_info);
	if (X1_ISP_PDC_CHANNEL_NUM != kbuf_info->plane_count) {
		isp_log_err("the error plane count(%d) for pdc buffer!", kbuf_info->plane_count);
		ret = -EINVAL;
	}

	for (i = 0; i < kbuf_info->plane_count; i++) {
		reg_addr = REG_ISP_PDC_BASE(hw_pipe_id) + 0x2f8 + i * 4;
		//bytes peer channel(channel_cnt)
		kbuf_info->buf_planes[i].reserved[0] = x1isp_reg_readl(reg_addr);
		//width peer channel, channel_cnt = channel_width * height;
		kbuf_info->buf_planes[i].reserved[1] = x1isp_reg_readl(reg_addr + 0x10);
		isp_log_dbg("pdc channel%d: count=0x%x, width=0x%x!", i, kbuf_info->buf_planes[i].reserved[0],
			kbuf_info->buf_planes[i].reserved[1]);
	}

	return ret;
}

//tasklet context:dma eof.
static int isp_stat_dma_eof_handler(struct x1isp_stats_node *stats_node, u32 stat_id)
{
	int ret = 0, return_idle = 0;
	struct isp_kbuffer_info *kbuf_info = NULL;
	struct isp_stat_buffer_queue *buffer_queue = NULL;
	struct stats_notify_params notify_param;

	buffer_queue = &stats_node->stat_bufqueue[stat_id];
	kbuf_info = x1isp_stat_get_busybuffer(buffer_queue, 0, stats_node->hw_pipe_id);

	if (kbuf_info) {
		if (kbuf_info->buf_status == ISP_BUFFER_STATUS_ERROR) {
			//error status handle
			x1isp_stat_put_idlebuffer(buffer_queue, kbuf_info);
			isp_log_warn("the error buffer(%d) on stat%d return to idle!", kbuf_info->buf_index, stat_id);
		} else {
			if (ISP_STAT_ID_PDC == stat_id) {
				if (stats_node->dma_irq_info[0].dynamic_switch != STAT_DMA_SWITCH_DYNAMIC_ON) {
					//pdc hardware open, but software have no idea.
					return_idle = 1;
				} else {
					isp_stat_get_pdc_real_ch_size(stats_node->hw_pipe_id, kbuf_info);
				}
			}

			if (return_idle)
				x1isp_stat_put_idlebuffer(buffer_queue, kbuf_info);
			else
				x1isp_stat_put_donebuffer(stat_id, &stats_node->stat_done_info[stat_id], kbuf_info);

			if (ISP_STAT_ID_PDC == stat_id && !return_idle) {
				notify_param.stat_id = stat_id;
				notify_param.event_enable = true;
				notify_param.frame_id = kbuf_info->frame_id;
				stats_node->notify_event(stats_node->private_dev, PIPE_EVENT_CAST_VOTE, (void*)&notify_param,
										sizeof(struct stats_notify_params));
			}
		}
	} else
		ret = -EPERM;

	return ret;
}

//tasklet context and when streamon
int isp_stat_dma_sof_handler(struct x1isp_stats_node *stats_node, u32 stat_id,
			     u32 dma_ch_id, u32 frame_id)
{
	int ret = 0, array_index = 0, i = 0;
	struct isp_kbuffer_info *kbuf_info = NULL;
	struct isp_stat_buffer_queue *buffer_queue = NULL;
	struct isp_reg_unit reg_unit[12] = { 0 };
	u32 reg_base_addr = 0, addr_offset = 0;
	u32 reg_high_addr = 0, low_addr = 0, high_addr = 0;

	buffer_queue = &stats_node->stat_bufqueue[stat_id];
	kbuf_info = x1isp_stat_get_idlebuffer(buffer_queue, stats_node->hw_pipe_id);
	if (!kbuf_info) {
		isp_log_dbg("no buffer in stat(%d) idle for pipe%d,frameID:%d!",
			    stat_id, stats_node->hw_pipe_id, frame_id);
		// pBufQue->bNextEofMiss = true;
		return -EPERM;
	}

	kbuf_info->frame_id = frame_id;
	if (dma_ch_id < PIPE0_PDC_DMA_CH_ID) {
		//dma channel y and uv addr.
		reg_base_addr = REG_ISP_DMA_Y_ADDR(dma_ch_id);
		reg_high_addr = REG_ISP_DMA_Y_HIGH_ADDR(dma_ch_id);
	} else if (dma_ch_id <= PIPE1_PDC_DMA_CH_ID) {
		//pdc dma
		reg_base_addr =
		    REG_ISP_PDC_DMA_BASE_ADDR(dma_ch_id - PIPE0_PDC_DMA_CH_ID);
		reg_high_addr =
		    REG_ISP_PDC_DMA_HIGH_BASE_ADDR(dma_ch_id - PIPE0_PDC_DMA_CH_ID);
	} else {
		isp_log_err("unknown this dma ch ID:%d!", dma_ch_id);
		return -EINVAL;
	}

	low_addr = kbuf_info->phy_addr & 0xffffffff;
	high_addr = (kbuf_info->phy_addr >> 32) & 0x3;
	for (i = 0; i < kbuf_info->plane_count; i++) {
		reg_unit[array_index].reg_addr = reg_base_addr + (i * 0x4);
		reg_unit[array_index].reg_value = low_addr + addr_offset;
		reg_unit[array_index].reg_mask = 0xffffffff;
		addr_offset += kbuf_info->buf_planes[i].length;
		array_index++;

		if (high_addr) {
			reg_unit[array_index].reg_addr = reg_high_addr + (i * 0x4);
			reg_unit[array_index].reg_value = high_addr;
			reg_unit[array_index].reg_mask = 0x3;
			array_index++;
		}
	}

	//dma channel ready.
	if (dma_ch_id < PIPE0_PDC_DMA_CH_ID) {
		reg_unit[array_index].reg_addr = REG_ISP_DMA_CHANNEL_MASTER(dma_ch_id);
		reg_unit[array_index].reg_value = BIT(31);
		reg_unit[array_index].reg_mask = BIT(31);
		array_index++;
	} else if (dma_ch_id <= PIPE1_PDC_DMA_CH_ID) {
		reg_unit[array_index].reg_addr = REG_ISP_PDC_DMA_MASTER;
		if (PIPE0_PDC_DMA_CH_ID == dma_ch_id) {
			reg_unit[array_index].reg_value = BIT(30);
			reg_unit[array_index].reg_mask = BIT(30);
		} else {
			reg_unit[array_index].reg_value = BIT(31);
			reg_unit[array_index].reg_mask = BIT(31);
		}
		array_index++;
	}

	ret = x1isp_reg_write_brust(reg_unit, array_index, false, NULL);
	if (ret)
		isp_log_err("isp stat dma sof config registers failed!");

	x1isp_stat_put_busybuffer(buffer_queue, kbuf_info);

	return ret;
}

static int isp_stat_get_dma_ch_id(struct x1isp_stats_node *stats_node, u32 stat_id)
{
	int ch_id = -1, i = 0;
	struct stat_dma_irq_bits *dma_irq_bitmap = NULL;

	if (stats_node) {
		for (i = 0; i < ISP_STAT_THROUGH_DMA_COUNT; i++) {
			dma_irq_bitmap = &stats_node->stat_dma_irq_bitmap[i];
			if (stat_id == dma_irq_bitmap->stat_id)
				ch_id = dma_irq_bitmap->dma_ch_id;
		}
	}

	return ch_id;
}

static int isp_stat_dma_trigger_dynamic_condition(struct x1isp_stats_node *stats_node,
					   u32 irq_info_index)
{
	int ret = 0;
	struct stat_dma_irq_info *dma_irq_info = NULL;
	struct stats_notify_params notify_param;

	dma_irq_info = &stats_node->dma_irq_info[irq_info_index];
	notify_param.stat_id = dma_irq_info->stat_id;
	notify_param.frame_id = 0;
	if (STAT_DMA_SWITCH_DYNAMIC_ON == dma_irq_info->dynamic_switch) {
		if (dma_irq_info->dynamic_trigger != 1) {
			notify_param.event_enable = true;
			stats_node->notify_event(stats_node->private_dev,
						 PIPE_EVENT_TRIGGER_VOTE_SYS,
						 (void *)&notify_param,
						 sizeof(struct stats_notify_params));
			dma_irq_info->dynamic_trigger = 1;
		}
	} else if (STAT_DMA_SWITCH_DYNAMIC_OFF == dma_irq_info->dynamic_switch) {
		if (dma_irq_info->dynamic_trigger != 2) {
			notify_param.event_enable = false;
			stats_node->notify_event(stats_node->private_dev,
						 PIPE_EVENT_TRIGGER_VOTE_SYS,
						 (void *)&notify_param,
						 sizeof(struct stats_notify_params));
			dma_irq_info->dynamic_trigger = 2;
		}
	}

	return ret;
}

//tasklet context
int x1isp_stat_dma_lower_half_irq(struct x1isp_stats_node *stats_node, u32 frame_id)
{
	int dma_cnt = 0, type_index = 0, ret = 0;
	struct stat_dma_irq_info dest_irq_info[ISP_STAT_THROUGH_DMA_COUNT] = { 0 };
	struct stat_dma_irq_info *dma_irq_info = NULL;
	unsigned long flags = 0;

	ISP_DRV_CHECK_POINTER(stats_node);
	dma_irq_info = stats_node->dma_irq_info;
	for (dma_cnt = 0; dma_cnt < ISP_STAT_THROUGH_DMA_COUNT; dma_cnt++) {	// normal is 2.
		spin_lock_irqsave(&dma_irq_info[dma_cnt].flag_lock, flags);

		for(type_index = 0; type_index < ISP_DMA_IRQ_TYPE_NUM; type_index++) {
			dest_irq_info[dma_cnt].irq_flag[type_index] = dma_irq_info[dma_cnt].irq_flag[type_index];
			dest_irq_info[dma_cnt].stat_id = dma_irq_info[dma_cnt].stat_id;
			if (dest_irq_info[dma_cnt].irq_flag[type_index])
				dma_irq_info[dma_cnt].irq_flag[type_index] = false;
		}

		spin_unlock_irqrestore(&dma_irq_info[dma_cnt].flag_lock, flags);
	}

	// handler the irq
	for (dma_cnt = 0; dma_cnt < ISP_STAT_THROUGH_DMA_COUNT; dma_cnt++) {	// normal is 2.
		for (type_index = 0; type_index < ISP_DMA_IRQ_TYPE_NUM; type_index++) {
			if (dest_irq_info[dma_cnt].irq_flag[type_index]) {
				switch (type_index) {
				case ISP_DMA_IRQ_TYPE_ERR:
					isp_stat_dma_err_handler(stats_node, dest_irq_info[dma_cnt].stat_id);
					break;
				case ISP_DMA_IRQ_TYPE_EOF:
					isp_stat_dma_eof_handler(stats_node, dest_irq_info[dma_cnt].stat_id);
					break;
				case ISP_DMA_IRQ_TYPE_SOF:
				{
					int dma_ch = -1;
					dma_ch = isp_stat_get_dma_ch_id(stats_node, dest_irq_info[dma_cnt].stat_id);
					if (dma_ch < 0) {
						isp_log_err("get isp stat(%d) dma channel ID failed!", dest_irq_info[dma_cnt].stat_id);
						ret = -EPERM;
					} else {
						isp_stat_dma_trigger_dynamic_condition(stats_node, dma_cnt);
						isp_stat_dma_sof_handler(stats_node, dest_irq_info[dma_cnt].stat_id, dma_ch, frame_id + 1);
					}
					break;
				}
				default:
					isp_log_err("unsupport dma irq type:%d!", type_index);
					ret = -EINVAL;
					break;
				}
			}
		}
	}

	return ret;
}

/*tasklet context: ae,af,awb result read from mem*/
struct isp_kbuffer_info* x1isp_stat_read_from_mem(struct x1isp_stats_node *stats_node, u32 stat_id, u32 frame_num)
{
	struct isp_kbuffer_info *kbuf_info = NULL;
	u32 *buf_temp = NULL;
	int i = 0, read_cnt = 0;
	ulong reg_addr = 0, reg_base = 0;

	if (!stats_node) {
		isp_log_err("%s: Invalid pointer!", __func__);
		return NULL;
	}

	if (ISP_STAT_ID_AE == stat_id) {
		reg_base = REG_STAT_AEM_RESULT_MEM(stats_node->hw_pipe_id) + 512 * 4;
	} else if (ISP_STAT_ID_AF == stat_id) {
		reg_base = REG_STAT_AFC_RESULT_MEM(stats_node->hw_pipe_id);
	} else if (ISP_STAT_ID_AWB == stat_id) {
		reg_base = REG_STAT_WBM_RESULT_MEM(stats_node->hw_pipe_id);
	} else if (ISP_STAT_ID_LTM == stat_id) {
		reg_base = REG_STAT_LTM_RESULT_MEM(stats_node->hw_pipe_id);
	} else {
		isp_log_err("%s: invalid stat(%d) for pipe%d!", __func__, stat_id, stats_node->hw_pipe_id);
		return NULL;
	}

	kbuf_info = x1isp_stat_get_idlebuffer(&stats_node->stat_bufqueue[stat_id], stats_node->hw_pipe_id);
	if (!kbuf_info) {
		isp_log_info("no buffer in stat(%d) idle queue for pipe%d!", stat_id, stats_node->hw_pipe_id);
		return NULL;
	}

	if (!kbuf_info->kvir_addr) {
		isp_log_err("the kaddr is NULL in stat(%d) for pipe%d!", stat_id, stats_node->hw_pipe_id);
		x1isp_stat_put_idlebuffer(&stats_node->stat_bufqueue[stat_id], kbuf_info);
		return NULL;
	}

	read_cnt = kbuf_info->buf_planes[0].length / 4;
	if (ISP_STAT_ID_AE == stat_id) {
		read_cnt = read_cnt - 512;
		kbuf_info->buf_planes[0].reserved[0] = read_cnt * 4;
	}

	kbuf_info->frame_id = frame_num;
	isp_log_dbg("ready to read the addr(0x%lx),size=%d!", reg_base, read_cnt);
	for (i = 0; i < read_cnt; i++) {
		buf_temp = (u32 *) kbuf_info->kvir_addr + i;
		reg_addr = reg_base + i * 4;
		*buf_temp = x1isp_reg_readl(reg_addr);
	}

	return kbuf_info;
}

//isp tasklet context
void x1isp_stat_mem_lower_half_irq(struct x1isp_stats_node *stats_node, u32 stat_id, u32 frame_num)
{
	unsigned long flags = 0;
	struct isp_kbuffer_info *kbuf_info = NULL;
	u8 start_read = false;
	int info_index = 0;

	if (!stats_node) {
		isp_log_err("%s: Invalid pointer!", __func__);
		return;
	}

	if (stat_id >= ISP_STAT_ID_MAX) {
		isp_log_err("%s: Invalid stat id:%d!", __func__, stat_id);
		return;
	}

	info_index = stats_node->mem_irq_index[stat_id];
	if (stats_node->stat_active[stat_id]) {
		spin_lock_irqsave(&stats_node->mem_irq_info[info_index].mem_flag_lock, flags);
		start_read = stats_node->mem_irq_info[info_index].start_read;
		if (start_read)
			stats_node->mem_irq_info[info_index].start_read = false;
		spin_unlock_irqrestore(&stats_node->mem_irq_info[info_index].mem_flag_lock, flags);
	}

	if (start_read) {
		kbuf_info = x1isp_stat_read_from_mem(stats_node, stat_id, frame_num);
		if (kbuf_info)
			x1isp_stat_put_donebuffer(stat_id, &stats_node->stat_done_info[stat_id], kbuf_info);
	}
}

/* the upper half irq context */
int x1isp_stat_mem_set_irq_flag(struct x1isp_stats_node *stats_node, u32 stat_id, u32 hw_pipe_id)
{
	int info_index = -1;

	ISP_DRV_CHECK_POINTER(stats_node);

	if (hw_pipe_id != stats_node->hw_pipe_id) {
		isp_log_err("%s: Invalid pipe:%d for stats node(%d)!", __func__,
			    hw_pipe_id, stats_node->hw_pipe_id);
		return -EINVAL;
	}

	if (!stats_node->stat_active[stat_id]) {
		isp_log_err("%s: stat%d isn't active on pipe:%d!", __func__, stat_id, hw_pipe_id);
		return -EINVAL;
	}

	info_index = stats_node->mem_irq_index[stat_id];
	if (info_index < 0) {
		isp_log_err("%s: stat%d isn't memregs on pipe:%d!", __func__, stat_id, hw_pipe_id);
		return -EINVAL;
	}

	spin_lock(&stats_node->mem_irq_info[info_index].mem_flag_lock);
	stats_node->mem_irq_info[info_index].start_read = true;
	spin_unlock(&stats_node->mem_irq_info[info_index].mem_flag_lock);

	return 0;
}

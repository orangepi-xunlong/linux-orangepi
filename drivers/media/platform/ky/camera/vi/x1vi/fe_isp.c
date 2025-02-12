// SPDX-License-Identifier: GPL-2.0
/*
 * fe_isp.c - x1isp front end
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include "x1vi.h"
#include "fe_isp.h"
#include "hw-seq/hw_isp.h"
#include "hw-seq/hw_dma.h"
#include "hw-seq/hw_postpipe.h"
#include "hw-seq/hw_reg.h"
#ifdef CONFIG_KY_X1_VI_IOMMU
#include "hw-seq/hw_iommu.h"
#include <linux/dma-mapping.h>
#endif
#include "../vdev.h"
#include "../ky_videobuf2.h"
#include "../vsensor.h"
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/dma-map-ops.h>
#include <linux/cma.h>
#include <media/videobuf2-core.h>
#include <media/x1/x1_videodev2.h>
#include <media/x1/x1_media_bus_format.h>
#include <linux/reset.h>
#include "../../cam_ccic/ccic_drv.h"
//#include <soc/spm/plat.h>
//#include <soc/spm/clk-plat.h>
#ifdef CAM_MODULE_TAG
#undef CAM_MODULE_TAG
#endif
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>

#define USE_TASKLET		(1)
#define USE_WORKQ		(0)
#define CCIC_MAX_CNT		(3)
#define DMA_START_CNT_WITH_DWT	(5)
#define FAKE_CCIC_IRQ		(-1)
#define FAKE_CCIC_ID		(CCIC_MAX_CNT)

#define ISP_FNC_CLK_FREQ_LOW	(307200000)
#define ISP_BUS_CLK_FREQ_LOW	(307200000)
#define ISP_FNC_CLK_FREQ_HIGH	(416000000)
#define ISP_BUS_CLK_FREQ_HIGH	(307200000)

#define MMU_RESERVED_MEM_SIZE	(4 * 1024)

enum {
	ISP_CLK_LOW = 0,
	ISP_CLK_HIGH,
};

#ifdef CONFIG_KY_DEBUG
struct dev_running_info {
	bool b_dev_running;
	bool (*is_dev_running)(struct dev_running_info *p_devinfo);
	struct notifier_block nb;
} vi_running_info;

static bool __maybe_unused check_dev_running_status(struct dev_running_info *p_devinfo)
{
	return p_devinfo->b_dev_running;
}

#define to_devinfo(_nb) container_of(_nb, struct dev_running_info, nb)

static int __maybe_unused dev_clkoffdet_notifier_handler(struct notifier_block *nb,
							 unsigned long msg, void *data)
{
	struct clk_notifier_data *cnd = data;
	struct dev_running_info *p_devinfo = to_devinfo(nb);

	if ((__clk_is_enabled(cnd->clk)) && (msg & PRE_RATE_CHANGE) &&
	    (cnd->new_rate == 0) && (cnd->old_rate != 0)) {
		if (p_devinfo->is_dev_running(p_devinfo))
			return NOTIFY_BAD;
	}

	return NOTIFY_OK;
}
#endif

struct isp_context;

struct frame_id {
	__u64 id;
	struct list_head entry;
};

#define ISP_DMA_WORK_MAX_CNT	(16)
struct isp_dma_context;
struct isp_dma_work_struct {
	struct work_struct dma_work;
	struct tasklet_struct dma_tasklet;
	struct list_head idle_list_entry;
	struct list_head busy_list_entry;
	unsigned int irq_status;
	struct isp_dma_context *dma_ctx;
};

struct isp_dma_context {
	struct list_head dma_work_idle_list;
	struct list_head dma_work_busy_list;
	struct list_head list_entry;
	spinlock_t slock;
	struct spm_camera_vnode *vnode;
	struct isp_context *isp_ctx;
	struct frame_id frame_id;
	int used_for_hdr;
	int trig_dma_reload;
	struct wait_queue_head waitq_head;
	struct wait_queue_head waitq_eof;
	int in_streamoff;
	int in_irq;
	atomic_t busy_cnt;
	int id;
#ifdef CONFIG_KY_X1_VI_IOMMU
	dma_addr_t tt_addr[2][2];
	unsigned int *tt_base[2][2];
	unsigned int tbu_update_cnt[2];
#endif
};

struct ccic {
	atomic_t pwr_cnt;
	struct spm_camera_sensor *sc_sensor;
	struct ccic_ctrl *csi_ctrl;
};

enum {
	MMU_TBU_OK = 0,
	//MMU_TBU_TRIGGER_READY,
	MMU_TBU_RELOAD,
	MMU_TBU_RELOAD_START = 4,
};

struct isp_pipeline_context {
	struct list_head fmt_wdma_list[FORMATTER_NUM];
	struct list_head wdma_list;
	struct list_head fmt_wdma_sync[FORMATTER_NUM][VB2_MAX_FRAME];
	unsigned int fmt_wdma_sync_cnt[FORMATTER_NUM][VB2_MAX_FRAME];
	unsigned int fmt_wdma_start_cnt[FORMATTER_NUM];
	unsigned int fmt_wdma_cnt[FORMATTER_NUM];
	unsigned int mmu_tbu_reload;
	struct camera_capture_slice_info cc_slice_info;
};

#define ISP_PRINT_WORK_MAX_CNT	(64)
struct isp_print_work_struct {
	struct work_struct print_work;
	struct list_head list;
	char msg_string[128];
	struct isp_context *isp_ctx;
};

#define ISP_FATAL_ERR_PIPE0_OVERRUN	(1 << 0)
#define ISP_FATAL_ERR_PIPE1_OVERRUN	(1 << 1)
#define ISP_FATAL_ERR_DMA_OVERLAP	(1 << 2)

struct isp_context {
	struct fe_rawdump *rawdumps[RAWDUMP_NUM];
	struct fe_pipe *pipes[PIPE_NUM];
	struct fe_formatter *formatters[FORMATTER_NUM];
	struct isp_dma_context dma_out_ctx[AOUT_NUM];
	struct isp_dma_context dma_in_ctx[AIN_NUM];
	struct frame_id pipe_frame_id[PIPE_NUM];
	struct platform_device *pdev;
	unsigned long base_addr;
//	struct clk *ahb_clk;
	struct reset_control *ahb_reset;
	struct reset_control *isp_reset;
	struct reset_control *isp_ci_reset;
	struct reset_control *lcd_mclk_reset;

	struct clk *fnc_clk;
	struct clk *bus_clk;
	struct clk *dpu_clk;
	struct spm_camera_block *dma_block;
	struct ccic ccic[CCIC_MAX_CNT];
	struct isp_print_work_struct print_works[ISP_PRINT_WORK_MAX_CNT];
	struct list_head print_work_list;
	atomic_t pwr_cnt;
	unsigned int isp_fatal_error;
	unsigned int dma_overlap_cnt;
	spinlock_t slock;
	struct completion global_reset_done;
#ifdef CONFIG_KY_X1_VI_IOMMU
	struct isp_iommu_device *mmu_dev;
	dma_addr_t trans_tab_dma_addr;
	void *trans_tab_cpu_addr;
	size_t total_trans_tab_sz;
	dma_addr_t rsvd_phy_addr;
	unsigned char *rsvd_vaddr;
#endif
};

struct vi_port_cfg {
	struct wdma_fifo_ctrl w_fifo_ctrl;
	unsigned int usage;
	unsigned int buf_required_min;
};

#define vi_irq_print(fmt, ...)	do {	\
		unsigned long vi_irq_flags = 0;	\
		struct isp_print_work_struct *print_work = NULL;	\
		spin_lock_irqsave(&isp_ctx->slock, vi_irq_flags);	\
		print_work = list_first_entry_or_null(&isp_ctx->print_work_list, struct isp_print_work_struct, list);	\
		if (print_work) {	\
			list_del_init(&print_work->list);	\
			spin_unlock_irqrestore(&isp_ctx->slock, vi_irq_flags);\
			snprintf(print_work->msg_string, 128, fmt, ##__VA_ARGS__);	\
			schedule_work(&print_work->print_work);	\
		} else {	\
			spin_unlock_irqrestore(&isp_ctx->slock, vi_irq_flags);\
			cam_err("d " fmt, ##__VA_ARGS__);	\
		}	\
	} while(0)

static irqreturn_t fe_isp_irq_handler(int irq, void *dev_id);
static irqreturn_t fe_isp_dma_irq_handler(int irq, void *dev_id);
static irqreturn_t fe_isp_process_dma_reload(struct isp_context *isp_ctx,
					     struct spm_camera_pipeline *sc_pipeline);
static void fe_isp_reset_frame_id(struct spm_camera_pipeline *sc_pipeline);
static void fe_isp_flush_pipeline_buffers(struct isp_context *isp_ctx,
					  struct spm_camera_pipeline *sc_pipeline);
static void fe_isp_export_camera_vbuffer(struct spm_camera_vnode *sc_vnode,
					 struct spm_camera_vbuffer *sc_vb);
static void fe_isp_dma_bh_handler(struct isp_dma_work_struct *isp_dma_work);

#define IDI_FMT_FLAG_OFFLINE_INPUT		(1 << 0)
#define IDI_FMT_FLAG_ONLINE_INPUT		(1 << 1)
#define IDI_FMT_FLAG_OUTPUT			(1 << 2)
static struct {
	unsigned int fmt_code;
	int cfa_pattern;
	unsigned int bit_depth;
	unsigned int flags;
} idi_fmts_table[] = {
	{
		.fmt_code = MEDIA_BUS_FMT_SRGB8_KYPACK_1X8,
		.cfa_pattern = CFA_IGNR,
		.bit_depth = 8,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SBGGR8_1X8,
		.cfa_pattern = BGGR,
		.bit_depth = 8,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SGBRG8_1X8,
		.cfa_pattern = GBRG,
		.bit_depth = 8,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SGRBG8_1X8,
		.cfa_pattern = GRBG,
		.bit_depth = 8,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SRGGB8_1X8,
		.cfa_pattern = RGGB,
		.bit_depth = 8,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SRGB10_KYPACK_1X10,
		.cfa_pattern = CFA_IGNR,
		.bit_depth = 10,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.cfa_pattern = BGGR,
		.bit_depth = 10,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.cfa_pattern = GBRG,
		.bit_depth = 10,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.cfa_pattern = GRBG,
		.bit_depth = 10,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SRGGB10_1X10,
		.cfa_pattern = RGGB,
		.bit_depth = 10,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SRGB12_KYPACK_1X12,
		.cfa_pattern = CFA_IGNR,
		.bit_depth = 12,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SBGGR12_1X12,
		.cfa_pattern = BGGR,
		.bit_depth = 12,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SGBRG12_1X12,
		.cfa_pattern = GBRG,
		.bit_depth = 12,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SGRBG12_1X12,
		.cfa_pattern = GRBG,
		.bit_depth = 12,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.cfa_pattern = RGGB,
		.bit_depth = 12,
		.flags = IDI_FMT_FLAG_OFFLINE_INPUT | IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SRGB14_KYPACK_1X14,
		.cfa_pattern = CFA_IGNR,
		.bit_depth = 14,
		.flags = IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SBGGR14_1X14,
		.cfa_pattern = BGGR,
		.bit_depth = 14,
		.flags = IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SGBRG14_1X14,
		.cfa_pattern = GBRG,
		.bit_depth = 14,
		.flags = IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SGRBG14_1X14,
		.cfa_pattern = GRBG,
		.bit_depth = 14,
		.flags = IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_SRGGB14_1X14,
		.cfa_pattern = RGGB,
		.bit_depth = 14,
		.flags = IDI_FMT_FLAG_ONLINE_INPUT | IDI_FMT_FLAG_OUTPUT,
	},
};

static struct {
	unsigned int fmt_code;
	int format;
} formatter_fmts_table[] = {
	{
		.fmt_code = MEDIA_BUS_FMT_RGB565_1X16,
		.format = RGB565,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_RGB888_1X24,
		.format = RGB888,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YUYV8_1_5X8,
		.format = NV12,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YVYU8_1_5X8,
		.format = NV21,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YUYV10_1X20,
		.format = Y210,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YUYV10_2X10,
		.format = P210,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YUYV10_1_5X10,
		.format = P010,
	},
};

static struct {
	unsigned int fmt_code;
	int layer_idx;
}dwt_fmts_table[] = {
	{
		.fmt_code = MEDIA_BUS_FMT_YUYV10_1_5X10_D1,
		.layer_idx = 1,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YUYV10_1_5X10_D2,
		.layer_idx = 2,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YUYV10_1_5X10_D3,
		.layer_idx = 3,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YUYV10_1_5X10_D4,
		.layer_idx = 4,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YVYU10_1_5X10_D1,
		.layer_idx = 1,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YVYU10_1_5X10_D2,
		.layer_idx = 2,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YVYU10_1_5X10_D3,
		.layer_idx = 3,
	},
	{
		.fmt_code = MEDIA_BUS_FMT_YVYU10_1_5X10_D4,
		.layer_idx = 4,
	},
};

static struct {
	unsigned int width;
	unsigned int height;
	unsigned int hblank;
	unsigned int vblank;
}tpg_timming_table[] = {
	{
		.width = 2560,
		.height = 1440,
		.hblank = 3218,
		.vblank = 455830,
	},
	{
		.width = 3840,
		.height = 2160,
		.hblank = 3864,
		.vblank = 32495,
	},
	{
		.width = 4608,
		.height = 3456,
		.hblank = 207,
		.vblank = 32495,
	},
};

static void fe_isp_set_clk(struct isp_context *isp_ctx, int clk_mode)
{
	unsigned long clk_val = 0;

	if (clk_mode == ISP_CLK_LOW) {
		clk_val = clk_round_rate(isp_ctx->fnc_clk, ISP_FNC_CLK_FREQ_LOW);
		clk_set_rate(isp_ctx->fnc_clk, clk_val);
		clk_val = clk_round_rate(isp_ctx->bus_clk, ISP_BUS_CLK_FREQ_LOW);
		clk_set_rate(isp_ctx->bus_clk, clk_val);
	} else {
		clk_val = clk_round_rate(isp_ctx->fnc_clk, ISP_FNC_CLK_FREQ_HIGH);
		clk_set_rate(isp_ctx->fnc_clk, clk_val);
		clk_val = clk_round_rate(isp_ctx->bus_clk, ISP_BUS_CLK_FREQ_HIGH);
		clk_set_rate(isp_ctx->bus_clk, clk_val);
	}
}

#ifdef CONFIG_KY_X1_VI_IOMMU

static uint32_t fe_isp_fill_trans_tab_by_sg(uint32_t *tt_base, struct sg_table *sgt,
					    uint32_t offset, uint32_t length)
{
	struct scatterlist *sg = NULL;
	size_t temp_size = 0, temp_offset = 0, temp_length = 0;
	dma_addr_t start_addr = 0, end_addr = 0, dmad = 0;
	int i = 0;
	uint32_t tt_size = 0;

	sg = sgt->sgl;
	for (i = 0; i < sgt->nents; ++i, sg = sg_next(sg)) {
		cam_dbg("sg%d: addr 0x%llx, size 0x%x", i, sg_phys(sg), sg_dma_len(sg));
		temp_size += sg_dma_len(sg);
		if (temp_size <= offset)
			continue;

		if (offset > temp_size - sg_dma_len(sg))
			temp_offset = offset - temp_size + sg_dma_len(sg);
		else
			temp_offset = 0;
		start_addr = ((phys_cpu2cam(sg_phys(sg)) + temp_offset) >> 12) << 12;

		temp_length = temp_size - offset;
		if (temp_length >= length)
			temp_offset = sg_dma_len(sg) - temp_length + length;
		else
			temp_offset = sg_dma_len(sg);
		end_addr = ((phys_cpu2cam(sg_phys(sg)) + temp_offset + 0xfff) >> 12) << 12;

		for (dmad = start_addr; dmad < end_addr; dmad += 0x1000)
			tt_base[tt_size++] = (dmad >> 12) & 0x3fffff;

		if (temp_length >= length)
			break;
	}

	return tt_size;
}

static dma_addr_t spm_vb2_buf_paddr(struct vb2_buffer *vb, unsigned int plane_no)
{
	unsigned int offset = 0, length = 0, tt_size = 0, tid = 0;
	int index = 0;
	uint32_t *tt_base = NULL;
	dma_addr_t tt_addr = 0;
	dma_addr_t paddr = 0;
	struct spm_camera_vbuffer *sc_vb = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct isp_context *isp_ctx = NULL;
	struct scatterlist *sg = NULL;
	struct sg_table *sgt = NULL;
	struct platform_device *pdev = x1vi_get_platform_device();
	struct x1vi_platform_data *drvdata = NULL;

	drvdata = platform_get_drvdata(pdev);
	BUG_ON(!drvdata);
	isp_ctx = drvdata->isp_ctx;
	BUG_ON(!isp_ctx);
	sc_vb = vb2_buffer_to_spm_camera_vbuffer(vb);
	if (sc_vb->flags & SC_BUF_FLAG_RSVD_Z1) {
		return isp_ctx->rsvd_phy_addr;
	}
	sc_vnode = sc_vb->sc_vnode;
	BUG_ON(!sc_vnode);
	sgt = (struct sg_table *)vb2_plane_cookie(vb, plane_no);
	offset = sc_vnode->planes_offset[vb->index][plane_no];
	length = vb->planes[plane_no].length;
	if (sc_vb->flags & SC_BUF_FLAG_CONTINOUS) {
		sg = sgt->sgl;
		paddr = sg_phys(sg) + offset;
	} else {
		if (KY_VNODE_DIR_OUT == sc_vnode->direction) {
			index = (isp_ctx->dma_out_ctx[sc_vnode->idx].tbu_update_cnt[plane_no])++ & 0x1;
			tt_base = isp_ctx->dma_out_ctx[sc_vnode->idx].tt_base[index][plane_no];
			tt_addr = isp_ctx->dma_out_ctx[sc_vnode->idx].tt_addr[index][plane_no];
		} else {
			tt_base = isp_ctx->dma_in_ctx[sc_vnode->idx].tt_base[0][plane_no];
			tt_addr = isp_ctx->dma_in_ctx[sc_vnode->idx].tt_addr[0][plane_no];
		}
		tid = MMU_TID(sc_vnode->direction, sc_vnode->idx, plane_no);
		tt_size = fe_isp_fill_trans_tab_by_sg(tt_base, sgt, offset, length);
		isp_mmu_call(isp_ctx->mmu_dev, config_channel, tid, tt_addr, tt_size);
		isp_mmu_call(isp_ctx->mmu_dev, enable_channel, tid);
		paddr = (dma_addr_t)isp_ctx->mmu_dev->ops->get_sva(isp_ctx->mmu_dev, tid, offset);
	}

	return paddr;
}
#endif

static int __maybe_unused fe_isp_lookup_tpg_timming_table(unsigned int width,
							  unsigned int height,
							  unsigned int *hblank,
							  unsigned int *vblank)
{
	int i = 0;

	if (!hblank || !vblank)
		return -1;
	for (i = 0; i < ARRAY_SIZE(tpg_timming_table); i++) {
		if (tpg_timming_table[i].width == width
		    && tpg_timming_table[i].height == height) {
			*hblank = tpg_timming_table[i].hblank;
			*vblank = tpg_timming_table[i].vblank;
			return 0;
		}
	}

	return -1;
}

static int list_add_no_repeat(struct list_head *new, struct list_head *head)
{
	struct list_head *pos = NULL;

	list_for_each(pos, head) {
		if (pos == new)
			return 1;
	}
	list_add(new, head);
	return 0;
}

static int fe_isp_lookup_dwt_fmts_table(struct v4l2_subdev_format *sd_format,
					int layer_idx)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(dwt_fmts_table); i++) {
		if (sd_format->format.code == dwt_fmts_table[i].fmt_code
		    && layer_idx == dwt_fmts_table[i].layer_idx)
			return 1;
	}

	return 0;
}

static int fe_isp_lookup_formatter_fmts_table(struct v4l2_subdev_format *sd_format,
					      int *format, int dwt_mode)
{
	int i = 0;

	if (dwt_mode && sd_format->format.code != MEDIA_BUS_FMT_YUYV8_1_5X8
	    && sd_format->format.code != MEDIA_BUS_FMT_YVYU8_1_5X8)
		return 0;
	for (i = 0; i < ARRAY_SIZE(formatter_fmts_table); i++) {
		if (sd_format->format.code == formatter_fmts_table[i].fmt_code) {
			*format = formatter_fmts_table[i].format;
			return 1;
		}
	}

	return 0;
}

static void fe_isp_update_ain_dma_addr(struct spm_camera_vnode *sc_vnode,
				       struct spm_camera_vbuffer *sc_vbuf,
				       unsigned int offset)
{
	hw_dma_update_rdma_address(SC_BLOCK(sc_vnode),
				   sc_vnode->idx,
				   (uint64_t)spm_vb2_buf_paddr(&(sc_vbuf->vb2_v4l2_buf.vb2_buf), 0) + offset);
}

static void fe_isp_update_aout_dma_addr(struct spm_camera_vnode *sc_vnode,
					struct spm_camera_vbuffer *sc_vbuf,
					unsigned int offset)
{
	dma_addr_t p0 = 0, p1 = 0;
	struct vb2_buffer *vb2_buf = &(sc_vbuf->vb2_v4l2_buf.vb2_buf);
	struct media_pad *remote_pad = media_entity_remote_pad(&sc_vnode->pad);
	struct v4l2_subdev *remote_sd = NULL;
	struct x1vi_platform_data *drvdata = NULL;
	struct isp_context *isp_ctx = NULL;

	if (!remote_pad) {
		cam_err("%s(%s) remote_pad was null", __func__, sc_vnode->name);
		return;
	}
	remote_sd = media_entity_to_v4l2_subdev(remote_pad->entity);
	if (!remote_sd) {
		cam_err("%s(%s) remote_sd was null", __func__, sc_vnode->name);
		return;
	}
	drvdata = dev_get_drvdata(remote_sd->dev);
	if (!drvdata) {
		cam_err("%s(%s) drvdata was null", __func__, sc_vnode->name);
		return;
	}
	isp_ctx = drvdata->isp_ctx;
	p0 = spm_vb2_buf_paddr(vb2_buf, 0) + offset;
	if (vb2_buf->num_planes > 1) {
		p1 = spm_vb2_buf_paddr(vb2_buf, 1) + offset;
	}
	hw_dma_update_wdma_address(SC_BLOCK(sc_vnode), sc_vnode->idx, p0, p1);
}

static int fe_isp_vnode_notifier_handler(struct notifier_block *nb,
					 unsigned long action, void *data)
{
	struct spm_camera_subdev *sc_subdev =
	    container_of(nb, struct spm_camera_subdev, vnode_nb);
	struct media_pipeline *pipe = media_entity_pipeline(&sc_subdev->pcsd.sd.entity);
	struct spm_camera_vnode *sc_vnode = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct isp_context *isp_ctx = NULL;
	struct csi *csi = NULL;
	struct fe_dwt *dwt = NULL;
	struct fe_formatter *formatter = NULL;
	struct fe_offline_channel *offline_channel = NULL;
	struct fe_rawdump *rawdump = NULL;
	//struct media_pad *remote_pad = NULL;
	int ret = 0, i = 0, j = 0;
	struct spm_camera_vbuffer *sc_vb = NULL, *pos = NULL, *n = NULL;
	unsigned long flags = 0;
	unsigned int offset = 0;
	//unsigned char *buf_vaddr = NULL;
	//static const char uv_padding[] = {0x00, 0x02, 0x08, 0x20, 0x80};

	if (!pipe)
		return NOTIFY_DONE;
	dwt = v4l2_subdev_to_dwt(&sc_subdev->pcsd.sd);
	csi = v4l2_subdev_to_csi(&sc_subdev->pcsd.sd);
	formatter = v4l2_subdev_to_formatter(&sc_subdev->pcsd.sd);
	offline_channel = v4l2_subdev_to_offline_channel(&sc_subdev->pcsd.sd);
	rawdump = v4l2_subdev_to_rawdump(&sc_subdev->pcsd.sd);
	isp_ctx = spm_subdev_get_drvdata(sc_subdev);
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	pipe_ctx = (struct isp_pipeline_context *)sc_pipeline->usr_data;
	if (action == KY_VNODE_NOTIFY_BUF_QUEUED) {
		sc_vnode = (struct spm_camera_vnode *)data;
		if (!sc_vnode)
			return NOTIFY_DONE;
		if (!(sc_pipeline->is_online_mode) && is_vnode_streaming(sc_vnode)) {
			if (!sc_pipeline->is_slice_mode) {
				spin_lock_irqsave(&sc_vnode->slock, flags);
				if (sc_vnode->direction == KY_VNODE_DIR_IN
					&& __spm_vdev_busy_list_empty(sc_vnode)) {
					ret = __spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
					if (ret) {
						spin_unlock_irqrestore(&sc_vnode->slock, flags);
						return NOTIFY_DONE;
					}
					fe_isp_update_ain_dma_addr(sc_vnode, sc_vb, 0);
					__spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
					hw_dma_rdma_trigger(SC_BLOCK(sc_vnode), sc_vnode->idx);
				}
				spin_unlock_irqrestore(&sc_vnode->slock, flags);
			} else {
				BUG_ON(!pipe_ctx);
				if (offline_channel) {
					offset = pipe_ctx->cc_slice_info.raw_read_offset;
					cam_dbg("%s(%s) raw_read_offset=%d", __func__, sc_vnode->name, offset);
				} else if (formatter) {
					offset = pipe_ctx->cc_slice_info.yuv_out_offset;
					cam_dbg("%s(%s) yuv_out_offset=%d", __func__, sc_vnode->name, offset);
				} else if (dwt) {
					offset = pipe_ctx->cc_slice_info.dwt_offset[dwt->layer_idx - 1];
					cam_dbg("%s(%s) dwt[%d]_offset=%d", __func__, sc_vnode->name, dwt->layer_idx, offset);
				} else {
					BUG_ON(1);
				}
				spin_lock_irqsave(&sc_vnode->slock, flags);
				ret = __spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
				if (ret) {
					spin_unlock_irqrestore(&sc_vnode->slock, flags);
					return NOTIFY_STOP;
				}
				__spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
				spin_unlock_irqrestore(&sc_vnode->slock, flags);
				if ((sc_pipeline->slice_id + 1) == pipe_ctx->cc_slice_info.total_slice_cnt)
					sc_vb->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_SLICES_DONE;
				if (offline_channel) {
					fe_isp_update_ain_dma_addr(sc_vnode, sc_vb, offset);
					hw_dma_rdma_trigger(SC_BLOCK(sc_vnode), sc_vnode->idx);
				} else {
					fe_isp_update_aout_dma_addr(sc_vnode, sc_vb, offset);
					hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 1);
				}
			}
		}
		if (rawdump && is_vnode_streaming(sc_vnode)) {
			spin_lock_irqsave(&sc_vnode->slock, flags);
			if (__spm_vdev_busy_list_empty(sc_vnode)) {
				sc_vb = NULL;
				__spm_vdev_pick_idle_vbuffer(sc_vnode, &sc_vb);
				if (sc_vb && sc_vb->flags & SC_BUF_FLAG_FORCE_SHADOW) {
					__spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
					__spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
					fe_isp_update_aout_dma_addr(sc_vnode, sc_vb, 0);
					hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 1);
				}
			}
			spin_unlock_irqrestore(&sc_vnode->slock, flags);
		}
	} else if(action == KY_VNODE_NOTIFY_BUF_PREPARE) {
		//sc_vb = (struct spm_camera_vbuffer*)data;
		//if (!sc_vb)
		//	return NOTIFY_DONE;
		//if (dwt && !sc_vb->reset_flag) {
		//	remote_pad = media_entity_remote_pad(&(dwt->pads[PAD_OUT]));
		//	if (!remote_pad)
		//		return NOTIFY_DONE;
		//	sc_vnode = media_entity_to_sc_vnode(remote_pad->entity);
		//	if (!sc_vnode)
		//		return NOTIFY_DONE;
		//	sc_vb->reset_flag = 1;
		//	buf_vaddr = (unsigned char*)vb2_plane_vaddr(&sc_vb->vb2_v4l2_buf.vb2_buf, 0);
		//	BUG_ON(!buf_vaddr);
		//	memset(buf_vaddr, 0, sc_vnode->cur_fmt.fmt.pix_mp.plane_fmt[0].sizeimage);
		//	buf_vaddr = vb2_plane_vaddr(&sc_vb->vb2_v4l2_buf.vb2_buf, 1);
		//	for (i = 0; (i + 5) <= sc_vnode->cur_fmt.fmt.pix_mp.plane_fmt[1].sizeimage; i += 5) {
		//		memcpy(buf_vaddr + i, uv_padding, 5);
		//	}
		//}
	} else if (action == KY_VNODE_NOTIFY_STREAM_OFF) {
		sc_vnode = (struct spm_camera_vnode *)data;
		if (!csi && pipe_ctx) {
			spin_lock_irqsave(&sc_pipeline->slock, flags);
			for (i = 0; i < FORMATTER_NUM; i++) {
				for (j = 0; j < VB2_MAX_FRAME; j++) {
					list_for_each_entry_safe(pos, n, &pipe_ctx->fmt_wdma_sync[i][j], list_entry) {
						if (sc_vnode->idx == pos->sc_vnode->idx) {
							spm_vdev_export_camera_vbuffer(pos, 1);
							list_del_init(&(pos->list_entry));
						}
					}
				}
			}
			spin_unlock_irqrestore(&sc_pipeline->slock, flags);
		}
	}
	return NOTIFY_DONE;
}

static int fe_isp_init_dma_context(struct isp_dma_context *dma_ctx,
				   struct isp_context *isp_ctx,
				   unsigned int dma_worker_cnt,
				   void (*dma_work_handler)(struct work_struct *),
				   void (*dma_tasklet_handler)(unsigned long),
				   struct device *dev)
{
	int i = 0;
	struct isp_dma_work_struct *isp_dma_work = NULL;

	INIT_LIST_HEAD(&dma_ctx->dma_work_idle_list);
	INIT_LIST_HEAD(&dma_ctx->dma_work_busy_list);
	INIT_LIST_HEAD(&dma_ctx->list_entry);
	INIT_LIST_HEAD(&dma_ctx->frame_id.entry);
	spin_lock_init(&dma_ctx->slock);
	init_waitqueue_head(&dma_ctx->waitq_head);
	init_waitqueue_head(&dma_ctx->waitq_eof);
	dma_ctx->in_streamoff = 0;
	dma_ctx->in_irq = 0;
	atomic_set(&dma_ctx->busy_cnt, 0);
	for (i = 0; i < dma_worker_cnt; i++) {
		isp_dma_work = devm_kzalloc(dev, sizeof(*isp_dma_work), GFP_KERNEL);
		if (!isp_dma_work) {
			cam_err("%s not enough mem", __func__);
			return -ENOMEM;
		}
		INIT_WORK(&isp_dma_work->dma_work, dma_work_handler);
		tasklet_init(&isp_dma_work->dma_tasklet, dma_tasklet_handler,
			     (unsigned long)isp_dma_work);
		INIT_LIST_HEAD(&isp_dma_work->idle_list_entry);
		INIT_LIST_HEAD(&isp_dma_work->busy_list_entry);
		isp_dma_work->dma_ctx = dma_ctx;
		list_add(&isp_dma_work->idle_list_entry, &dma_ctx->dma_work_idle_list);
	}
	dma_ctx->isp_ctx = isp_ctx;

	return 0;
}

static int fe_isp_get_dma_work(struct isp_dma_context *dma_ctx,
			       struct isp_dma_work_struct **isp_dma_work)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&dma_ctx->slock, flags);
	*isp_dma_work =
	    list_first_entry_or_null(&dma_ctx->dma_work_idle_list,
				     struct isp_dma_work_struct, idle_list_entry);
	if (NULL == *isp_dma_work) {
		spin_unlock_irqrestore(&dma_ctx->slock, flags);
		return -1;
	}
	list_del_init(&((*isp_dma_work)->idle_list_entry));
	list_add(&((*isp_dma_work)->busy_list_entry), &dma_ctx->dma_work_busy_list);
	spin_unlock_irqrestore(&dma_ctx->slock, flags);

	return 0;
}

static int fe_isp_put_dma_work(struct isp_dma_context *dma_ctx,
			       struct isp_dma_work_struct *isp_dma_work)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&dma_ctx->slock, flags);
	list_del_init(&isp_dma_work->busy_list_entry);
	list_add(&isp_dma_work->idle_list_entry, &dma_ctx->dma_work_idle_list);
	spin_unlock_irqrestore(&dma_ctx->slock, flags);

	return 0;
}

static int fe_rawdump_subdev_pad_get_fmt(struct v4l2_subdev *sd,
					 struct v4l2_subdev_state *state,
					 struct v4l2_subdev_format *format)
{
	struct fe_rawdump *rawdump = v4l2_subdev_to_rawdump(sd);

	if (format->pad >= RAWDUMP_PAD_NUM) {
		cam_dbg("%s(%s) invalid pad%d.", __func__, rawdump->sc_subdev.name,
			format->pad);
		return -EINVAL;
	}

	format->format = rawdump->pad_fmts[format->pad].format;

	return 0;
}

static int fe_isp_lookup_raw_fmts_table(struct v4l2_subdev_format *format,
					unsigned int flags,
					int *cfa_pattern, unsigned int *bit_depth)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(idi_fmts_table); i++) {
		if (format->format.code == idi_fmts_table[i].fmt_code
		    && (flags & idi_fmts_table[i].flags)) {
			if (cfa_pattern)
				*cfa_pattern = idi_fmts_table[i].cfa_pattern;
			if (bit_depth)
				*bit_depth = idi_fmts_table[i].bit_depth;
			return 1;
		}
	}

	return 0;
}

static int fe_rawdump_subdev_pad_set_fmt(struct v4l2_subdev *sd,
					 struct v4l2_subdev_state *state,
					 struct v4l2_subdev_format *format)
{
	struct fe_rawdump *rawdump = v4l2_subdev_to_rawdump(sd);
	struct spm_camera_subdev *sc_subdev = &rawdump->sc_subdev;
	struct v4l2_format v4l2_fmt;
	struct v4l2_subdev_format *pad_in_fmt = NULL;
	unsigned int bit_depth = 0;
	struct isp_context *isp_ctx = v4l2_get_subdevdata(sd);
	struct spm_camera_vnode *sc_vnode = NULL;
	struct media_pad *remote_pad = NULL;
	struct media_entity *me = &sd->entity;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *mpipe = media_entity_pipeline(me);
	struct fe_pipe *pipe = NULL;
	int is_mix_hdr = 0;

	if (format->which != V4L2_SUBDEV_FORMAT_ACTIVE) {
		cam_err("%s(%s) didn't support format which(%d)", __func__,
			sc_subdev->name, format->which);
		return -EINVAL;
	}
	if (format->pad >= RAWDUMP_PAD_NUM) {
		cam_err("%s(%s) invalid pad%d.", __func__, sc_subdev->name,
			format->pad);
		return -EINVAL;
	}

	switch (format->pad) {
	case PAD_IN:
		if (!mpipe) {
			cam_err("%s(%s) pipe is null", __func__, sc_subdev->name);
			return -1;
		}
		sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
		//if (format->format.width > sc_pipeline->max_width[0]
		//	|| format->format.height > sc_pipeline->max_height[0]
		//	|| format->format.width < sc_pipeline->min_width[0]
		//	|| format->format.height < sc_pipeline->min_height[0]) {
		//	cam_err("%s(%s) %ux%u exceeded max %ux%u min %ux%u", __func__, sc_subdev->name,
		//			format->format.width, format->format.height,
		//			sc_pipeline->max_width[0], sc_pipeline->max_height[0],
		//			sc_pipeline->min_width[0], sc_pipeline->min_height[0]);
		//	return -1;
		//}
		if (!fe_isp_lookup_raw_fmts_table(format, IDI_FMT_FLAG_ONLINE_INPUT, NULL, &bit_depth)) {
			cam_err("%s(%s) pad%d didn't support format(%dx%d code:0x%08x).", __func__, sc_subdev->name, format->pad,
					format->format.width, format->format.height, format->format.code);
			return -1;
		}
		hw_isp_top_set_rawdump_fmt(SC_BLOCK(isp_ctx->pipes[0]),
					   rawdump->idx,
					   format->format.width,
					   format->format.height, bit_depth);
		rawdump->pad_fmts[PAD_IN].format = format->format;
		rawdump->pad_fmts[PAD_OUT].format = format->format;
		cam_dbg("%s(%s) pad%d set format(%dx%d code:0x%08x bit_depth:%u).", __func__, sc_subdev->name, format->pad,
				format->format.width, format->format.height, format->format.code, bit_depth);
		return 0;
	case PAD_OUT:
		pad_in_fmt = &rawdump->pad_fmts[PAD_IN];
		remote_pad = media_entity_remote_pad(&rawdump->pads[format->pad]);
		if (!remote_pad) {
			cam_err("%s(%s) PAD_OUT had no link.", __func__, sc_subdev->name);
			return -1;
		}
		sc_vnode = media_entity_to_sc_vnode(remote_pad->entity);
		BUG_ON(!sc_vnode);
		if (!fe_isp_lookup_raw_fmts_table(format, IDI_FMT_FLAG_OUTPUT, NULL, &bit_depth)) {
			cam_err("%s(%s) pad%d didn't support format(%dx%d code:0x%08x).", __func__, sc_subdev->name, format->pad,
					format->format.width, format->format.height, format->format.code);
			return -1;
		}
		if (format->format.code != pad_in_fmt->format.code
			|| format->format.width != pad_in_fmt->format.width
			|| format->format.height != pad_in_fmt->format.height) {
			cam_err("%s(%s) PAD_OUT format(%dx%d code:0x%08x) is not the same with PAD_IN format(%dx%d code:0x%08x).",
					__func__, sc_subdev->name,
					format->format.width, format->format.height, format->format.code,
					pad_in_fmt->format.width, pad_in_fmt->format.height, pad_in_fmt->format.code);
			return -1;
		}
		remote_pad = media_entity_remote_pad(&rawdump->pads[PAD_IN]);
		if (remote_pad) {
			pipe = media_entity_to_pipe(remote_pad->entity);
			if (pipe)
				is_mix_hdr = 1;
		}
		spm_vdev_fill_v4l2_format(format, &v4l2_fmt);
		//fill HW sequence
		//hw_isp_top_set_rawdump_fmt(&(isp_ctx->pipes[0]->sc_subdev.sc_block),
		//						rawdump->idx,
		//						format->format.width,
		//						format->format.height,
		//						bit_depth);
		hw_dma_set_wdma_pitch(SC_BLOCK(sc_vnode),
				      sc_vnode->idx,
				      v4l2_fmt.fmt.pix_mp.num_planes,
				      v4l2_fmt.fmt.pix_mp.plane_fmt[0].bytesperline,
				      v4l2_fmt.fmt.pix_mp.plane_fmt[1].bytesperline);
		if (is_mix_hdr)
			hw_dma_set_rdma_pitch(SC_BLOCK(sc_vnode), 0, v4l2_fmt.fmt.pix_mp.plane_fmt[0].bytesperline);
		return 0;
	default:
		cam_dbg("%s(%s) didn't support set fmt for pad%d.", __func__, sc_subdev->name, format->pad);
		return -1;
	}

	return 0;
}

static int fe_rawdump_subdev_video_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct fe_rawdump *rawdump = v4l2_subdev_to_rawdump(sd);
	struct spm_camera_subdev *sc_subdev = &rawdump->sc_subdev;
	struct media_pad *remote_pad = NULL;
	struct csi *csi = NULL;
	struct fe_pipe *pipe = NULL;
	struct isp_context *isp_ctx = v4l2_get_subdevdata(sd);
	struct media_entity *me_pipe0 = (struct media_entity*)isp_ctx->pipes[0];
	struct media_entity *me_pipe1 = (struct media_entity*)isp_ctx->pipes[1];
	struct spm_camera_pipeline *sc_pipeline = NULL, *sc_pipeline0 = NULL, *sc_pipeline1 = NULL;
	struct media_pipeline *mpipe = media_entity_pipeline(&sd->entity);
	unsigned int irq_bitmap = 0, cap_to_preview = 0;
	unsigned int vi_flags = 0, clk_high = 0;
	int ret = 0, source = 0, rawdump_only = 0, sensor_id = 0;

	BUG_ON(!me_pipe0);
	BUG_ON(!me_pipe1);
	if (!mpipe) {
		cam_err("%s(%s) pipe was null", __func__, sc_subdev->name);
		return -1;
	}

	sc_pipeline0 = media_pipeline_to_sc_pipeline(me_pipe0);
	sc_pipeline1 = media_pipeline_to_sc_pipeline(me_pipe1);
	vi_flags = (rawdump->pad_fmts[PAD_IN].format.field >> KY_VI_SWITCH_FLAGS_SHIFT) & KY_VI_PRI_DATA_MASK;
	cap_to_preview = vi_flags & KY_VI_FLAG_BACK_TO_PREVIEW;
	clk_high = vi_flags & KY_VI_FLAG_CLK_HIGH;
	sensor_id = rawdump->pad_fmts[PAD_IN].format.field & KY_VI_PRI_DATA_MASK;
	sensor_id = (sensor_id & KY_VI_SENSOR_ID_MASK) >> KY_VI_SENSOR_ID_SHIFT;
	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	remote_pad = media_entity_remote_pad(&rawdump->pads[PAD_IN]);
	if (!remote_pad) {
		cam_err("%s(%s) PAD_IN had no link", __func__, sc_subdev->name);
		return -1;
	}
	csi = media_entity_to_csi(remote_pad->entity);
	pipe = media_entity_to_pipe(remote_pad->entity);

	if (pipe && !csi) {
		remote_pad = media_entity_remote_pad(&(pipe->pads[PIPE_PAD_IN]));
		if (!remote_pad) {
			cam_err("%s(%s) PAD_IN->pipe had no active input link", __func__, sc_subdev->name);
			return -1;
		}
		csi = media_entity_to_csi(remote_pad->entity);
	}
	if (!csi) {
		cam_err("%s(%s) PAD_IN had no link to csi or pipe->csi.", __func__, sc_subdev->name);
		return -1;
	}
	ret = blocking_notifier_call_chain(&sc_pipeline->blocking_notify_chain,
					   PIPELINE_ACTION_PIPE_ACK, NULL);
	if (NOTIFY_STOP == ret) {
		rawdump_only = 0;
		rawdump->rawdump_only = 0;
	} else {
		rawdump_only = 1;
		rawdump->rawdump_only = 1;
	}
	if (rawdump->idx == 0)
		irq_bitmap = POSTERR_IRQ_RDP0_SDW_CLOSE_DONE;
	else
		irq_bitmap = POSTERR_IRQ_RDP1_SDW_CLOSE_DONE;
	hw_isp_top_set_rdp_cfg_rdy(SC_BLOCK(isp_ctx->pipes[0]), rawdump->idx, 0);
	if (enable) {
		if (clk_high)
			fe_isp_set_clk(isp_ctx, ISP_CLK_HIGH);
		else
			fe_isp_set_clk(isp_ctx, ISP_CLK_LOW);
		atomic_set(&rawdump->close_done, 0);
		fe_isp_reset_frame_id(sc_pipeline);
		isp_ctx->dma_overlap_cnt = 0;
		if (sensor_id == 0)
			source = SENSOR0_CH0;
		else
			source = SENSOR1_CH0;
		hw_isp_top_clr_posterr_irq_status(SC_BLOCK(isp_ctx->pipes[0]), irq_bitmap);
		hw_isp_top_set_posterr_irq_enable(SC_BLOCK(isp_ctx->pipes[0]), irq_bitmap, 0);
		hw_isp_top_set_rawdump_source(SC_BLOCK(isp_ctx->pipes[0]), rawdump->idx, source);
		hw_isp_top_enable_rawdump(SC_BLOCK(isp_ctx->pipes[rawdump->idx]), 1, rawdump_only);
		if (isp_ctx->dma_block)
			hw_dma_reset(isp_ctx->dma_block);
		if (sc_pipeline0 && sc_pipeline1) {
			if ((sc_pipeline0->is_online_mode && !sc_pipeline1->is_online_mode)
				|| (!sc_pipeline0->is_online_mode && sc_pipeline1->is_online_mode)) {
				hw_isp_top_set_speed_ctrl(SC_BLOCK(isp_ctx->pipes[0]), 1);
			} else {
				hw_isp_top_set_speed_ctrl(SC_BLOCK(isp_ctx->pipes[0]), 0);
			}
		} else {
			hw_isp_top_set_speed_ctrl(SC_BLOCK(isp_ctx->pipes[0]), 0);
		}
		if (!cap_to_preview)
			hw_isp_top_shadow_latch(SC_BLOCK(isp_ctx->pipes[rawdump->idx]));
	} else {
		hw_isp_top_set_rawdump_source(SC_BLOCK(isp_ctx->pipes[0]), rawdump->idx, INVALID_CH);
		hw_isp_top_enable_rawdump(SC_BLOCK(isp_ctx->pipes[rawdump->idx]), 0, rawdump_only);
		hw_isp_top_set_posterr_irq_enable(SC_BLOCK(isp_ctx->pipes[0]), 0, irq_bitmap);
		if (rawdump_only) {
			if (!atomic_read(&rawdump->close_done))
				cam_warn("%s(%s) stream off not signaled", __func__, sc_subdev->name);
			else
				cam_dbg("%s(%s) stream off ok", __func__, sc_subdev->name);
		}
	}
	hw_isp_top_set_rdp_cfg_rdy(SC_BLOCK(isp_ctx->pipes[0]), rawdump->idx, 1);
	return 0;
}

static int csi_subdev_core_s_power(struct v4l2_subdev *sd, int on);

static int csi_subdev_video_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct csi *csi = v4l2_subdev_to_csi(sd);
	struct spm_camera_subdev *sc_subdev = &csi->sc_subdev;
	struct isp_context *isp_ctx = v4l2_get_subdevdata(sd);
	struct ccic_ctrl *csi_ctrl = NULL;
	int ret = 0, csi2vc = 0, csi2idi = 0, mipi_lane_num = 0, sensor_id = 0;

	sensor_id = csi->pad_fmts[CSI_PAD_IN].format.field & KY_VI_PRI_DATA_MASK;
	sensor_id = (sensor_id & KY_VI_SENSOR_ID_MASK) >> KY_VI_SENSOR_ID_SHIFT;
	csi_ctrl = isp_ctx->ccic[sensor_id].csi_ctrl;
	BUG_ON(!csi_ctrl);
	if (sensor_id == 0)
		csi2idi = CCIC_CSI2IDI0;
	else
		csi2idi = CCIC_CSI2IDI1;
	if (csi->channel_type == CSI_MAIN)
		csi2vc = CCIC_CSI2VC_MAIN;
	else
		csi2vc = CCIC_CSI2VC_VCDT;
	if (enable) {
#ifndef CONFIG_KY_XILINX_ZYNQMP
		csi_subdev_core_s_power(sd, 1);
#endif
		mipi_lane_num = csi->pad_fmts[CSI_PAD_IN].format.field & KY_VI_PRI_DATA_MASK;
		mipi_lane_num &= KY_VI_MIPI_LANE_MASK;
		cam_dbg("%s(%s) mipi lane num:%d", __func__, sc_subdev->name, mipi_lane_num);
		ret = csi_ctrl->ops->config_csi2idi_mux(csi_ctrl, csi2vc, csi2idi, 1);
		if (ret) {
			cam_err("%s(%s) config mux(enable) failed ret=%d", __func__, sc_subdev->name, ret);
			return ret;
		}
		ret = csi_ctrl->ops->config_csi2_mbus(csi_ctrl, CCIC_CSI2VC_NM, 0, 0, 0, 0, mipi_lane_num);
		if (ret) {
			cam_err("%s(%s) config mbus(enable) lane=%d failed ret=%d", __func__, sc_subdev->name, 4, ret);
			return ret;
		}
		csi_ctrl->ops->irq_mask(csi_ctrl, 1);
	} else {
		csi_ctrl->ops->irq_mask(csi_ctrl, 0);
		csi_ctrl->ops->config_csi2_mbus(csi_ctrl, CCIC_CSI2VC_NM, 0, 0, 0, 0, 0);
		csi_ctrl->ops->config_csi2idi_mux(csi_ctrl, csi2vc, csi2idi, 0);
		csi_subdev_core_s_power(sd, 0);
	}

	return 0;
}

static struct isp_pipeline_context *fe_pipeline_create_ctx(struct device *dev)
{
	int i = 0, j = 0;
	struct isp_pipeline_context *pipe_ctx = devm_kzalloc(dev, sizeof(*pipe_ctx), GFP_KERNEL);

	if (!pipe_ctx)
		return NULL;
	for (i = 0; i < FORMATTER_NUM; i++)
		INIT_LIST_HEAD(&pipe_ctx->fmt_wdma_list[i]);
	for (i = 0; i < FORMATTER_NUM; i++) {
		for (j = 0; j < VB2_MAX_FRAME; j++)
			INIT_LIST_HEAD(&pipe_ctx->fmt_wdma_sync[i][j]);
	}
	INIT_LIST_HEAD(&pipe_ctx->wdma_list);

	return pipe_ctx;
}

static int fe_offline_channel_subdev_pad_set_fmt(struct v4l2_subdev *sd,
						 struct v4l2_subdev_state *state,
						 struct v4l2_subdev_format *format)
{
	struct fe_offline_channel *offline_channel = v4l2_subdev_to_offline_channel(sd);
	struct spm_camera_subdev *sc_subdev = &offline_channel->sc_subdev;
	struct media_pad *remote_pad_in = NULL, *remote_pad_p0out = NULL, *remote_pad_p1out = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct v4l2_format v4l2_fmt;

	if (format->which != V4L2_SUBDEV_FORMAT_ACTIVE) {
		cam_err("%s(%s) didn't support format which(%d)", __func__,
			sc_subdev->name, format->which);
		return -EINVAL;
	}
	if (format->pad >= OFFLINE_CH_PAD_NUM) {
		cam_err("%s(%s) invalid pad%d.", __func__, sc_subdev->name, format->pad);
		return -EINVAL;
	}
	remote_pad_in = media_entity_remote_pad(&(offline_channel->pads[OFFLINE_CH_PAD_IN]));
	remote_pad_p0out = media_entity_remote_pad(&(offline_channel->pads[OFFLINE_CH_PAD_P0OUT]));
	remote_pad_p1out = media_entity_remote_pad(&(offline_channel->pads[OFFLINE_CH_PAD_P1OUT]));
	if (!remote_pad_in || (!remote_pad_p0out && !remote_pad_p1out)) {
		cam_err("%s didn't have valid link.", sc_subdev->name);
		return -1;
	}
	sc_vnode = media_entity_to_sc_vnode(remote_pad_in->entity);
	if (!sc_vnode) {
		cam_err("%s(%s) OFFLINE_CH_PAD_IN should link to ain", __func__, sc_subdev->name);
		return -1;
	}
	switch (format->pad) {
	case OFFLINE_CH_PAD_IN:
		if (!fe_isp_lookup_raw_fmts_table(format, IDI_FMT_FLAG_OFFLINE_INPUT, NULL, NULL)) {
			cam_err("%s(%s) pad%d didn't support format(%dx%d code:0x%08x).",
				__func__, sc_subdev->name, format->pad, format->format.width,
				format->format.height, format->format.code);
			return -1;
		}
		spm_vdev_fill_v4l2_format(format, &v4l2_fmt);
		hw_dma_set_rdma_pitch(SC_BLOCK(sc_vnode),
				      sc_vnode->idx,
				      v4l2_fmt.fmt.pix_mp.plane_fmt[0].bytesperline);
		offline_channel->pad_fmts[OFFLINE_CH_PAD_IN].format = format->format;
		offline_channel->pad_fmts[OFFLINE_CH_PAD_P0OUT].format = format->format;
		offline_channel->pad_fmts[OFFLINE_CH_PAD_P1OUT].format = format->format;
		return 0;
	default:
		cam_dbg("%s(%s) didn't support set fmt for pad%d.", __func__, sc_subdev->name, format->pad);
		return -1;
	}

	return 0;
}

static int fe_offline_channel_subdev_pad_get_fmt(struct v4l2_subdev *sd,
						 struct v4l2_subdev_state *state,
						 struct v4l2_subdev_format *format)
{
	struct fe_offline_channel *offline_channel = v4l2_subdev_to_offline_channel(sd);

	if (format->pad >= OFFLINE_CH_PAD_NUM) {
		cam_dbg("%s didn't have pad%d.", offline_channel->sc_subdev.name,
			format->pad);
		return -EINVAL;
	}

	format->format = offline_channel->pad_fmts[format->pad].format;

	return 0;
}

static int __fe_offline_channel_pad_s_stream(struct fe_offline_channel *offline_channel,
					     unsigned int pad, int enable)
{
	struct spm_camera_vnode *sc_vnode = NULL;
	struct spm_camera_subdev *sc_subdev = &offline_channel->sc_subdev;
	//struct spm_camera_vbuffer *sc_vbuf = NULL;
	struct isp_context *isp_ctx = spm_subdev_get_drvdata(sc_subdev);
	int ret = 0;
	unsigned long flags = 0;

	switch (pad) {
	case OFFLINE_CH_PAD_IN:
		sc_vnode = spm_vdev_remote_vnode(&offline_channel->pads[pad]);
		if (!sc_vnode) {
			cam_err("%s(%s) ain was not found", __func__, sc_subdev->name);
			return -1;
		}
		if (enable) {	// stream on
			//ret = spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vbuf);
			//if (ret) {
			//	cam_err("%s(%s) enable:1 dq buf from %s failed.", __func__, sc_subdev->name, sc_vnode->name);
			//	return ret;
			//}
			//hw_isp_top_enable_hw_gap(SC_BLOCK(isp_ctx->pipes[0]), 0);
			//fe_isp_update_ain_dma_addr(sc_vnode, sc_vbuf, 0);
			//spm_vdev_q_busy_vbuffer(sc_vnode, sc_vbuf);
			isp_ctx->dma_in_ctx[sc_vnode->idx].vnode = sc_vnode;
			hw_isp_top_set_idi_rd_burst_len(SC_BLOCK(isp_ctx->pipes[0]), offline_channel->idx, 22, 8);
			hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
					      DMA_IRQ_SRC_RDMA_CH0 + sc_vnode->idx,
					      DMA_IRQ_START | DMA_IRQ_DONE | DMA_IRQ_ERR,
					      0);
			//hw_dma_rdma_trigger(SC_BLOCK(sc_vnode), sc_vnode->idx);
		} else {// stream off
			spin_lock_irqsave(&(isp_ctx->dma_in_ctx[sc_vnode->idx].waitq_head.lock), flags);
			wait_event_interruptible_locked_irq(isp_ctx->dma_in_ctx[sc_vnode->idx].waitq_head,
							    !isp_ctx->dma_in_ctx[sc_vnode->idx].in_irq);
			isp_ctx->dma_in_ctx[sc_vnode->idx].in_streamoff = 1;
			spin_unlock_irqrestore(&(isp_ctx->dma_in_ctx[sc_vnode->idx].waitq_head.lock), flags);
			hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
					      DMA_IRQ_SRC_RDMA_CH0 + sc_vnode->idx,
					      0,
					      DMA_IRQ_ALL);
			//hw_isp_top_enable_hw_gap(SC_BLOCK(isp_ctx->pipes[0]), 1);
			isp_ctx->dma_in_ctx[sc_vnode->idx].vnode = NULL;
			spin_lock_irqsave(&(isp_ctx->dma_in_ctx[sc_vnode->idx].waitq_head.lock), flags);
			isp_ctx->dma_in_ctx[sc_vnode->idx].in_streamoff = 0;
			spin_unlock_irqrestore(&(isp_ctx->dma_in_ctx[sc_vnode->idx].waitq_head.lock), flags);
		}
		break;
	default:
		cam_err("%s(%s) pad s_stream not supported on pad%d", __func__, sc_subdev->name, pad);
		return -1;
	}
	ret = spm_subdev_pad_s_stream(sc_subdev, pad, enable);
	if (ret)
		cam_err("%s(%s) s_stream on pad%u failed", __func__, sc_subdev->name, pad);
	return 0;
}

static int fe_offline_channel_subdev_pad_s_stream(struct v4l2_subdev *sd,
						  struct media_link *link,
						  struct v4l2_subdev_format *source_fmt,
						  struct v4l2_subdev_format *sink_fmt)
{
	struct fe_offline_channel *offline_channel = v4l2_subdev_to_offline_channel(sd);
	unsigned int pad = source_fmt->pad;
	int enable = source_fmt->which;

	return __fe_offline_channel_pad_s_stream(offline_channel, pad, enable);
}

static int fe_formatter_subdev_pad_set_fmt(struct v4l2_subdev *sd,
					   struct v4l2_subdev_state *state,
					   struct v4l2_subdev_format *format)
{
	struct fe_formatter *formatter = v4l2_subdev_to_formatter(sd);
	struct spm_camera_subdev *sc_subdev = &formatter->sc_subdev;
	struct media_pad *remote_pad_in = NULL, *remote_pad_aout = NULL;
	struct media_pad *remote_pad_d1out = NULL, *remote_pad_d2out = NULL;
	struct media_pad *remote_pad_d3out = NULL, *remote_pad_d4out = NULL;
	struct media_pad *remote_pad_out = NULL;
	int formatter_fmt = 0;
	struct v4l2_subdev_format *pad_fmt_in = NULL;
	struct v4l2_format v4l2_fmt;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(&sd->entity);
	int valid_link = 0, dwt_mode = 0;

	if (format->which != V4L2_SUBDEV_FORMAT_ACTIVE) {
		cam_err("%s(%s) didn't support format which(%d)", __func__,
			sc_subdev->name, format->which);
		return -EINVAL;
	}
	if (format->pad >= FORMATTER_PAD_NUM) {
		cam_err("%s(%s) invalid pad%d.", __func__, sc_subdev->name, format->pad);
		return -EINVAL;
	}
	if (!pipe) {
		cam_err("%s(%s) pipe was null", __func__, sc_subdev->name);
		return -1;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	mutex_lock(&sc_pipeline->mlock);
	if (!sc_pipeline->usr_data) {
		pipe_ctx = fe_pipeline_create_ctx(formatter->sc_subdev.pcsd.sd.dev);
		if (!pipe_ctx) {
			mutex_unlock(&sc_pipeline->mlock);
			cam_err("%s(%s) create pipe_ctx failed", __func__, sc_subdev->name);
			return -1;
		}
		sc_pipeline->usr_data = pipe_ctx;
	} else {
		pipe_ctx = (struct isp_pipeline_context *)sc_pipeline->usr_data;
	}
	mutex_unlock(&sc_pipeline->mlock);
	remote_pad_in = media_entity_remote_pad(&(formatter->pads[FMT_PAD_IN]));
	remote_pad_aout = media_entity_remote_pad(&(formatter->pads[FMT_PAD_AOUT]));
	remote_pad_d1out = media_entity_remote_pad(&(formatter->pads[FMT_PAD_D1OUT]));
	remote_pad_d2out = media_entity_remote_pad(&(formatter->pads[FMT_PAD_D2OUT]));
	remote_pad_d3out = media_entity_remote_pad(&(formatter->pads[FMT_PAD_D3OUT]));
	remote_pad_d4out = media_entity_remote_pad(&(formatter->pads[FMT_PAD_D4OUT]));
	if (remote_pad_in) {
		if (remote_pad_aout && !remote_pad_d1out && !remote_pad_d2out
			&& !remote_pad_d3out && !remote_pad_d4out) {
			valid_link = 1;
			dwt_mode = 0;
		} else if (remote_pad_aout && remote_pad_d1out && remote_pad_d2out
			&& remote_pad_d3out && remote_pad_d4out) {
			valid_link = 1;
			dwt_mode = 1;
		}
	}

	if (!valid_link) {
		cam_err("%s didn't have valid link.", sc_subdev->name);
		return -1;
	}

	switch (format->pad) {
	case FMT_PAD_IN:
		formatter->pad_fmts[FMT_PAD_IN].format = format->format;
		if (!sc_subdev->is_resetting) {
			formatter->pad_fmts[FMT_PAD_AOUT].format = format->format;
			formatter->pad_fmts[FMT_PAD_D1OUT].format = format->format;
			formatter->pad_fmts[FMT_PAD_D2OUT].format = format->format;
			formatter->pad_fmts[FMT_PAD_D3OUT].format = format->format;
			formatter->pad_fmts[FMT_PAD_D4OUT].format = format->format;
		}
		return 0;
		break;
	case FMT_PAD_AOUT:
		fallthrough;
	case FMT_PAD_D1OUT:
	case FMT_PAD_D2OUT:
	case FMT_PAD_D3OUT:
	case FMT_PAD_D4OUT:
		if (!fe_isp_lookup_formatter_fmts_table(format, &formatter_fmt, /*dwt_mode*/0)) {
			if (!sc_subdev->is_resetting)
				cam_err("%s(%s) mbus format code(0x%08x) not supported in dwt_mode(0)",
					__func__, sc_subdev->name, format->format.code);
			else
				cam_dbg("%s(%s) mbus format code(0x%08x) not supported in dwt_mode(0)",
					__func__, sc_subdev->name, format->format.code);
			return -1;
		}
		if (dwt_mode) {
			if (NV12 == formatter_fmt || NV21 == formatter_fmt)
				pipe_ctx->fmt_wdma_start_cnt[formatter->idx] = DMA_START_CNT_WITH_DWT;
			else
				pipe_ctx->fmt_wdma_start_cnt[formatter->idx] = 1;
		}
		pad_fmt_in = &(formatter->pad_fmts[FMT_PAD_IN]);
		if (format->format.width > pad_fmt_in->format.width
			|| format->format.height > pad_fmt_in->format.height) {
			cam_err("%s(%s) FMT_PAD_AOUT(%ux%u) didn't match FMT_PAD_IN(%ux%u)",
				__func__, sc_subdev->name, format->format.width, format->format.height,
				pad_fmt_in->format.width, pad_fmt_in->format.height);
			return -1;
		}
		formatter->pad_fmts[format->pad].format = format->format;
		hw_postpipe_set_formatter_format(SC_BLOCK(formatter), formatter->idx, formatter_fmt);
		remote_pad_out = media_entity_remote_pad(&(formatter->pads[format->pad]));
		sc_vnode = media_entity_to_sc_vnode(remote_pad_out->entity);
		BUG_ON((format->pad == FMT_PAD_AOUT) && !sc_vnode);
		if (sc_vnode) {
			spm_vdev_fill_v4l2_format(format, &v4l2_fmt);
			hw_dma_set_wdma_pitch(SC_BLOCK(sc_vnode),
					sc_vnode->idx,
					v4l2_fmt.fmt.pix_mp.num_planes,
					v4l2_fmt.fmt.pix_mp.plane_fmt[0].bytesperline,
					v4l2_fmt.fmt.pix_mp.plane_fmt[1].bytesperline);
		}
		return 0;
		break;
	default:
		cam_dbg("%s(%s) didn't support set fmt for pad%d.", __func__, sc_subdev->name, format->pad);
		return -1;
		break;
	}

	return 0;
}

static int fe_formatter_subdev_pad_get_fmt(struct v4l2_subdev *sd,
					   struct v4l2_subdev_state *state,
					   struct v4l2_subdev_format *format)
{
	struct fe_formatter *formatter = v4l2_subdev_to_formatter(sd);

	if (format->pad >= FORMATTER_PAD_NUM) {
		cam_dbg("%s(%s) invalid pad%d.", __func__, formatter->sc_subdev.name, format->pad);
		return -EINVAL;
	}

	format->format = formatter->pad_fmts[format->pad].format;

	return 0;
}

static int __fe_formatter_pad_s_stream(struct fe_formatter *formatter, unsigned int pad, int enable)
{
	int source = 0, ret = 0, need_initial_load = 1;
	struct media_pad *remote_pad_aout = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct spm_camera_subdev *sc_subdev = &formatter->sc_subdev;
	struct device *dev = sc_subdev->pcsd.sd.dev;
	struct media_pipeline *pipe = media_entity_pipeline(&sc_subdev->pcsd.sd.entity);
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct isp_context *isp_ctx = spm_subdev_get_drvdata(sc_subdev);
	struct vi_port_cfg *port_cfg = NULL;
	unsigned int wdma_fifo_offset = 0, wdma_fifo_depth = 0, wdma_weight = 0, wdma_fifo_div_mode = 8;
	unsigned long flags = 0;
	struct spm_camera_vbuffer *sc_vb = NULL;

	if (!pipe) {
		cam_err("%s(%s) pipe was null", __func__, sc_subdev->name);
		return -1;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	switch (pad) {
	case FMT_PAD_AOUT:
		remote_pad_aout = media_entity_remote_pad(&(formatter->pads[pad]));
		if (!remote_pad_aout) {
			cam_err("%s(%s) FMT_PAD_AOUT had no active link.", __func__, sc_subdev->name);
			return -1;
		}
		sc_vnode = media_entity_to_sc_vnode(remote_pad_aout->entity);
		BUG_ON(!sc_vnode);
		if (enable) {
			atomic_set(&isp_ctx->dma_out_ctx[sc_vnode->idx].busy_cnt, 0);
			mutex_lock(&sc_pipeline->mlock);
			if (!sc_pipeline->usr_data) {
				pipe_ctx = fe_pipeline_create_ctx(dev);
				if (!pipe_ctx) {
					mutex_unlock(&sc_pipeline->mlock);
					cam_err("%s(%s) create pipeline context failed", __func__, sc_subdev->name);
					return -ENOMEM;
				}
				sc_pipeline->usr_data = pipe_ctx;
			} else {
				pipe_ctx = (struct isp_pipeline_context*)sc_pipeline->usr_data;
			}
			mutex_unlock(&sc_pipeline->mlock);
			if (pipe_ctx->fmt_wdma_start_cnt[formatter->idx] == 1)
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 1;
			else
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 0;
			source = FORMATTER0 + formatter->idx;
			port_cfg = sc_vnode_get_usrdata(sc_vnode);
			if (port_cfg) {
				wdma_fifo_offset = port_cfg->w_fifo_ctrl.offset;
				wdma_fifo_depth = port_cfg->w_fifo_ctrl.depth;
				wdma_weight = port_cfg->w_fifo_ctrl.weight;
				wdma_fifo_div_mode = port_cfg->w_fifo_ctrl.div_mode;
			}
			hw_dma_set_wdma_source(SC_BLOCK(sc_vnode), sc_vnode->idx, source, wdma_fifo_offset,
					       wdma_fifo_depth, wdma_weight, wdma_fifo_div_mode);
			if (isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload) {
				hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
									DMA_IRQ_SRC_WDMA_CH0 + sc_vnode->idx,
									DMA_IRQ_START | DMA_IRQ_DONE | DMA_IRQ_ERR,
									0);
			} else {
				hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
									DMA_IRQ_SRC_WDMA_CH0 + sc_vnode->idx,
									DMA_IRQ_DONE | DMA_IRQ_ERR,
									DMA_IRQ_START);
			}
			isp_ctx->dma_out_ctx[sc_vnode->idx].vnode = sc_vnode;
			spin_lock_irqsave(&sc_pipeline->slock, flags);
			ret = list_add_no_repeat(&isp_ctx->dma_out_ctx[sc_vnode->idx].list_entry, &pipe_ctx->fmt_wdma_list[formatter->idx]);
			if (0 == ret)
				pipe_ctx->fmt_wdma_cnt[formatter->idx]++;
			spin_unlock_irqrestore(&sc_pipeline->slock, flags);
			if (need_initial_load) {
				ret = spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
				if (ret) {
					if (sc_vnode->sc_vb)
						sc_vb = sc_vnode->sc_vb;
					else
						cam_info("%s(%s) no initial buffer available", __func__, sc_subdev->name);
				} else {
					spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
				}
				if (sc_vb) {
					fe_isp_update_aout_dma_addr(sc_vnode, sc_vb, 0);
					hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 1);
				}
			}
			if (pipe_ctx->fmt_wdma_start_cnt[formatter->idx] == 1)
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 1;
			else
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 0;
		} else {// stream off
			if (sc_pipeline->is_online_mode) {
				ret = wait_event_interruptible_timeout(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_eof,
								       (atomic_read(&isp_ctx->dma_out_ctx[sc_vnode->idx].busy_cnt) <= 0),
								       msecs_to_jiffies(60));
				if (0 == ret)
					cam_warn("%s(%s) stream off wait eof timeout", __func__, sc_subdev->name);
				else if (ret < 0)
					cam_warn("%s(%s) stream off wait eof error ret=%d", __func__, sc_subdev->name, ret);
			}
			spin_lock_irqsave(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
			wait_event_interruptible_locked_irq(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head,
							    !isp_ctx->dma_out_ctx[sc_vnode->idx].in_irq);
			isp_ctx->dma_out_ctx[sc_vnode->idx].in_streamoff = 1;
			spin_unlock_irqrestore(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
			hw_dma_set_irq_enable(SC_BLOCK(sc_vnode), DMA_IRQ_SRC_WDMA_CH0 + sc_vnode->idx, 0, DMA_IRQ_ALL);
			pipe_ctx = (struct isp_pipeline_context*)sc_pipeline->usr_data;
			if (pipe_ctx) {
				spin_lock_irqsave(&sc_pipeline->slock, flags);
				list_del_init(&isp_ctx->dma_out_ctx[sc_vnode->idx].list_entry);
				pipe_ctx->fmt_wdma_cnt[formatter->idx]--;
				spin_unlock_irqrestore(&sc_pipeline->slock, flags);
			}
			isp_ctx->dma_out_ctx[sc_vnode->idx].vnode = NULL;
			spin_lock_irqsave(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
			isp_ctx->dma_out_ctx[sc_vnode->idx].in_streamoff = 0;
			spin_unlock_irqrestore(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
		}
		break;
	default:
		return -1;
	}
	ret = spm_subdev_pad_s_stream(sc_subdev, pad, enable);
	if (ret) {
		cam_err("%s(%s) s_stream on pad%u failed", __func__, sc_subdev->name, pad);
		return ret;
	}
	return 0;
}

static int fe_formatter_subdev_pad_s_stream(struct v4l2_subdev *sd,
					    struct media_link *link,
					    struct v4l2_subdev_format *source_fmt,
					    struct v4l2_subdev_format *sink_fmt)
{
	struct fe_formatter *formatter = v4l2_subdev_to_formatter(sd);
	unsigned int pad = source_fmt->pad;
	int enable = source_fmt->which;

	return __fe_formatter_pad_s_stream(formatter, pad, enable);
}

static int fe_formatter_subdev_video_s_stream(struct v4l2_subdev *sd, int enable)
{
	//int source = 0, source_twin = 0;
	//struct media_pad *remote_pad_in = NULL;
	//struct fe_formatter *formatter = v4l2_subdev_to_formatter(sd), *formatter_twin = NULL;
	//struct spm_camera_subdev *sc_subdev = &formatter->sc_subdev;
	//struct fe_pipe *pipe = NULL;
	//struct fe_hdr_combine *hdr_combine = NULL;
	//struct isp_context *isp_ctx = v4l2_get_subdevdata(sd);
	//struct media_entity *me = NULL;

	//if (formatter->idx == 0)
	//	formatter_twin = isp_ctx->formatters[1];
	//else
	//	formatter_twin = isp_ctx->formatters[0];
	//me = &formatter_twin->sc_subdev.pcsd.sd.entity;
	//remote_pad_in = media_entity_remote_pad(&(formatter->pads[FMT_PAD_IN]));
	//if (!remote_pad_in) {
	//	cam_err("%s(%s) FMT_PAD_IN had no active link", __func__, sc_subdev->name);
	//	return -1;
	//}
	//pipe = media_entity_to_pipe(remote_pad_in->entity);
	//if (pipe) {
	//	if (pipe->idx == 0) {
	//		source = SCL_SRC_SEL_PIPE0;
	//		source_twin = SCL_SRC_SEL_PIPE1;
	//	}
	//	else {
	//		source = SCL_SRC_SEL_PIPE1;
	//		source_twin = SCL_SRC_SEL_PIPE0;
	//	}
	//} else {
	//	hdr_combine = media_entity_to_hdr_combine(remote_pad_in->entity);
	//	if (!hdr_combine) {
	//		cam_err("%s(%s) FMT_PAD_IN should link to pipe or hdr_combine", __func__, sc_subdev->name);
	//		return -1;
	//	}
	//	source = SCL_SRC_SEL_PIPE0;
	//	source_twin = SCL_SRC_SEL_PIPE1;
	//}
	//hw_postpipe_set_scaler_source(SC_BLOCK(formatter), formatter->idx, source);
	//if (!me->pipe) {
	//	hw_postpipe_set_scaler_source(SC_BLOCK(formatter_twin), formatter_twin->idx, source_twin);
	//}
	return 0;
}

static int fe_dwt_subdev_pad_set_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *state,
				     struct v4l2_subdev_format *format)
{
	struct fe_dwt *dwt = v4l2_subdev_to_dwt(sd);
	struct spm_camera_subdev *sc_subdev = &dwt->sc_subdev;
	struct media_pad *remote_pad_in = NULL, *remote_pad_out = NULL;
	struct v4l2_subdev_format sd_format;
	struct v4l2_subdev *remote_sd_in = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct v4l2_format v4l2_fmt;
	int i = 1, ret = 0;

	if (format->which != V4L2_SUBDEV_FORMAT_ACTIVE) {
		cam_err("%s(%s) didn't support format which(%d)", __func__,
			sc_subdev->name, format->which);
		return -EINVAL;
	}
	if (format->pad >= DWT_PAD_NUM) {
		cam_err("%s(%s) invalid pad%d.", __func__, sc_subdev->name, format->pad);
		return -EINVAL;
	}
	remote_pad_in = media_entity_remote_pad(&(dwt->pads[PAD_IN]));
	remote_pad_out = media_entity_remote_pad(&(dwt->pads[PAD_OUT]));
	if (!remote_pad_in || !remote_pad_out) {
		cam_err("%s didn't have valid link.", sc_subdev->name);
		return -1;
	}

	switch (format->pad) {
	case PAD_IN:
		dwt->pad_fmts[PAD_IN].format = format->format;
		if (!sc_subdev->is_resetting) {
			dwt->pad_fmts[PAD_OUT].format = format->format;
			for (i = 1; i <= dwt->layer_idx; i++) {
				dwt->pad_fmts[PAD_OUT].format.width =
				    (dwt->pad_fmts[PAD_OUT].format.width + 1) >> 1;
				dwt->pad_fmts[PAD_OUT].format.height =
				    (dwt->pad_fmts[PAD_OUT].format.height + 1) >> 1;
			}
		}
		return 0;
	case PAD_OUT:
		sc_vnode = media_entity_to_sc_vnode(remote_pad_out->entity);
		BUG_ON(!sc_vnode);
		if (!fe_isp_lookup_dwt_fmts_table(format, dwt->layer_idx)) {
			if (!sc_subdev->is_resetting)
				cam_err("%s(%s) mbus format code(0x%08x) not supported",
					__func__, sc_subdev->name, format->format.code);
			else
				cam_dbg("%s(%s) mbus format code(0x%08x) not supported",
					__func__, sc_subdev->name, format->format.code);
			return -1;
		}
		//if (format->format.width != dwt->pad_fmts[PAD_OUT].format.width
		//	|| format->format.height != dwt->pad_fmts[PAD_OUT].format.height) {
		//	cam_err("%s(%s) PAD_OUT(%ux%u) didn't match PAD_IN(%ux%u)",
		//			__func__, sc_subdev->name, format->format.width, format->format.height,
		//			dwt->pad_fmts[PAD_OUT].format.width, dwt->pad_fmts[PAD_OUT].format.height);
		//	return -1;
		//}
		//dwt->pad_fmts[PAD_OUT].format.code = format->format.code;
		dwt->pad_fmts[PAD_OUT].format = format->format;
		sd_format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
		sd_format.pad = remote_pad_in->index;
		if (format->format.code >= MEDIA_BUS_FMT_YUYV10_1_5X10_D1
		    && format->format.code <= MEDIA_BUS_FMT_YUYV10_1_5X10_D4)
			sd_format.format.code = MEDIA_BUS_FMT_YUYV8_1_5X8;
		else
			sd_format.format.code = MEDIA_BUS_FMT_YVYU8_1_5X8;
		sd_format.format.width = dwt->pad_fmts[PAD_OUT].format.width;
		sd_format.format.height = dwt->pad_fmts[PAD_OUT].format.height;
		sd_format.stream = 0;
		remote_sd_in = media_entity_to_v4l2_subdev(remote_pad_in->entity);
		ret = v4l2_subdev_call(remote_sd_in, pad, set_fmt, NULL, &sd_format);
		if (ret) {
			cam_err("%s(%s) remote_pad_in set format(%ux%u code=0x%08x) failed",
				__func__, sc_subdev->name, sd_format.format.width, sd_format.format.height,
				sd_format.format.code);
			return ret;
		}
		dwt->pad_fmts[PAD_IN].format = dwt->pad_fmts[PAD_OUT].format;
		spm_vdev_fill_v4l2_format(format, &v4l2_fmt);
		hw_dma_set_wdma_pitch(SC_BLOCK(sc_vnode),
				      sc_vnode->idx,
				      v4l2_fmt.fmt.pix_mp.num_planes,
				      v4l2_fmt.fmt.pix_mp.plane_fmt[0].bytesperline,
				      v4l2_fmt.fmt.pix_mp.plane_fmt[1].bytesperline);
		return 0;
	default:
		cam_dbg("%s(%s) didn't support set fmt for pad%d.", __func__, sc_subdev->name, format->pad);
		return -1;
	}

	return 0;
}

static int fe_dwt_subdev_pad_get_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *state,
				     struct v4l2_subdev_format *format)
{
	struct fe_dwt *dwt = v4l2_subdev_to_dwt(sd);

	if (format->pad >= DWT_PAD_NUM) {
		cam_dbg("%s didn't have pad%d.", dwt->sc_subdev.name, format->pad);
		return -EINVAL;
	}

	format->format = dwt->pad_fmts[format->pad].format;

	return 0;
}

static int __fe_dwt_pad_s_stream(struct fe_dwt *dwt, unsigned int pad, int enable)
{
	int source = 0, mux_sel = 0, ret = 0, need_initial_load = 1;
	struct media_pad *remote_pad = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct spm_camera_subdev *sc_subdev = &dwt->sc_subdev;
	struct device *dev = sc_subdev->pcsd.sd.dev;
	struct media_pipeline *pipe = media_entity_pipeline(&sc_subdev->pcsd.sd.entity);
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct fe_formatter *formatter = NULL;
	struct isp_context *isp_ctx = spm_subdev_get_drvdata(sc_subdev);
	struct vi_port_cfg *port_cfg = NULL;
	struct spm_camera_vbuffer *sc_vb = NULL;
	unsigned int wdma_fifo_offset = 0, wdma_fifo_depth = 0, wdma_weight = 0, wdma_fifo_div_mode = 8;
	unsigned long flags = 0;

	if (!pipe) {
		cam_err("%s(%s) pipe was null", __func__, sc_subdev->name);
		return -1;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	switch (pad) {
	case PAD_OUT:
		remote_pad = media_entity_remote_pad(&(dwt->pads[pad]));
		if (!remote_pad) {
			cam_err("%s(%s) PAD_OUT had no active link.", __func__, sc_subdev->name);
			return -1;
		}
		sc_vnode = media_entity_to_sc_vnode(remote_pad->entity);
		BUG_ON(!sc_vnode);
		remote_pad = media_entity_remote_pad(&(dwt->pads[PAD_IN]));
		if (!remote_pad) {
			cam_err("%s(%s) PAD_IN had no active link", __func__, sc_subdev->name);
			return -1;
		}
		formatter = media_entity_to_formatter(remote_pad->entity);
		BUG_ON(!formatter);
		if (enable) {
			atomic_set(&isp_ctx->dma_out_ctx[sc_vnode->idx].busy_cnt, 0);
			if (dwt->layer_idx == 1)
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 1;
			else
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 0;
			if (sc_subdev->is_resetting || atomic_inc_return(&formatter->dwt_refcnt) == 1)
				hw_postpipe_enable_dwt(SC_BLOCK(dwt), dwt->idx, DWT_SRC_SEL_FORMATTER0 + formatter->idx, 1);
			if (dwt->idx == 0) {
				mux_sel = MUX_SEL_DWT0_LAYER1 + dwt->layer_idx - 1;
				hw_postpipe_dma_mux_enable(SC_BLOCK(dwt), mux_sel);
				source = DWT0_LAYER1 + dwt->layer_idx - 1;
			} else {
				source = DWT1_LAYER1 + dwt->layer_idx - 1;
			}
			port_cfg = sc_vnode_get_usrdata(sc_vnode);
			if (port_cfg) {
				wdma_fifo_offset = port_cfg->w_fifo_ctrl.offset;
				wdma_fifo_depth = port_cfg->w_fifo_ctrl.depth;
				wdma_weight = port_cfg->w_fifo_ctrl.weight;
				wdma_fifo_div_mode = port_cfg->w_fifo_ctrl.div_mode;
			}
			hw_dma_set_wdma_source(SC_BLOCK(sc_vnode), sc_vnode->idx, source, wdma_fifo_offset,
					       wdma_fifo_depth, wdma_weight, wdma_fifo_div_mode);
			if (isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload) {
				hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
						      DMA_IRQ_SRC_WDMA_CH0 + sc_vnode->idx,
						      DMA_IRQ_START | DMA_IRQ_DONE | DMA_IRQ_ERR,
						      0);
			} else {
				hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
						      DMA_IRQ_SRC_WDMA_CH0 + sc_vnode->idx,
						      DMA_IRQ_DONE | DMA_IRQ_ERR,
						      DMA_IRQ_START);
			}
			isp_ctx->dma_out_ctx[sc_vnode->idx].vnode = sc_vnode;
			mutex_lock(&sc_pipeline->mlock);
			if (!sc_pipeline->usr_data) {
				pipe_ctx = fe_pipeline_create_ctx(dev);
				if (!pipe_ctx) {
					mutex_unlock(&sc_pipeline->mlock);
					cam_err("%s(%s) create pipeline context failed", __func__, sc_subdev->name);
					return -ENOMEM;
				}
				sc_pipeline->usr_data = pipe_ctx;
			} else {
				pipe_ctx = (struct isp_pipeline_context*)sc_pipeline->usr_data;
			}
			mutex_unlock(&sc_pipeline->mlock);
			spin_lock_irqsave(&sc_pipeline->slock, flags);
			ret = list_add_no_repeat(&isp_ctx->dma_out_ctx[sc_vnode->idx].list_entry, &pipe_ctx->fmt_wdma_list[formatter->idx]);
			if (0 == ret)
				pipe_ctx->fmt_wdma_cnt[formatter->idx]++;
			spin_unlock_irqrestore(&sc_pipeline->slock, flags);
			if (need_initial_load) {
				ret = spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
				if (ret) {
					if (sc_vnode->sc_vb)
						sc_vb = sc_vnode->sc_vb;
					else
						cam_info("%s(%s) no initial buffer available", __func__, sc_subdev->name);
				} else {
					spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
				}
				if (sc_vb) {
					fe_isp_update_aout_dma_addr(sc_vnode, sc_vb, 0);
					hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 1);
				}
			}
			if (dwt->layer_idx == 1)
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 1;
			else
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 0;
		} else {	// stream off
			if (sc_pipeline->is_online_mode) {
				ret = wait_event_interruptible_timeout(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_eof,
								       (atomic_read(&isp_ctx->dma_out_ctx[sc_vnode->idx].busy_cnt) <= 0),
								       msecs_to_jiffies(60));
				if (0 == ret)
					cam_warn("%s(%s) stream off wait eof timeout", __func__, sc_subdev->name);
				else if (ret < 0)
					cam_warn("%s(%s) stream off wait eof error ret=%d", __func__, sc_subdev->name, ret);
			}
			spin_lock_irqsave(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
			wait_event_interruptible_locked_irq(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head,
							    !isp_ctx->dma_out_ctx[sc_vnode->idx].in_irq);
			isp_ctx->dma_out_ctx[sc_vnode->idx].in_streamoff = 1;
			spin_unlock_irqrestore(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
			hw_dma_set_irq_enable(SC_BLOCK(sc_vnode), DMA_IRQ_SRC_WDMA_CH0 + sc_vnode->idx, 0, DMA_IRQ_ALL);
			pipe_ctx = (struct isp_pipeline_context*)sc_pipeline->usr_data;
			if (pipe_ctx) {
				spin_lock_irqsave(&sc_pipeline->slock, flags);
				list_del_init(&isp_ctx->dma_out_ctx[sc_vnode->idx].list_entry);
				pipe_ctx->fmt_wdma_cnt[formatter->idx]--;
				spin_unlock_irqrestore(&sc_pipeline->slock, flags);
			}
			isp_ctx->dma_out_ctx[sc_vnode->idx].vnode = NULL;
			if (atomic_dec_and_test(&formatter->dwt_refcnt))
				hw_postpipe_enable_dwt(SC_BLOCK(dwt), dwt->idx, DWT_SRC_SEL_FORMATTER0 + formatter->idx, 0);
			spin_lock_irqsave(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
			isp_ctx->dma_out_ctx[sc_vnode->idx].in_streamoff = 0;
			spin_unlock_irqrestore(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
		}
		break;
	default:
		return -1;
	}

	ret = spm_subdev_pad_s_stream(sc_subdev, pad, enable);
	if (ret) {
		cam_err("%s(%s) s_stream on pad%u failed", __func__, sc_subdev->name, pad);
		return ret;
	}
	return 0;
}

static int fe_dwt_subdev_pad_s_stream(struct v4l2_subdev *sd, struct media_link *link,
				      struct v4l2_subdev_format *source_fmt,
				      struct v4l2_subdev_format *sink_fmt)
{
	struct fe_dwt *dwt = v4l2_subdev_to_dwt(sd);
	unsigned int pad = source_fmt->pad;
	int enable = source_fmt->which;

	return __fe_dwt_pad_s_stream(dwt, pad, enable);
}

static int fe_pipe_subdev_pad_set_fmt(struct v4l2_subdev *sd,
				      struct v4l2_subdev_state *state,
				      struct v4l2_subdev_format *format)
{
	struct fe_pipe *pipe = v4l2_subdev_to_pipe(sd);
	struct isp_context *isp_ctx = v4l2_get_subdevdata(sd);
	struct spm_camera_subdev *sc_subdev = &pipe->sc_subdev;
	struct media_pad *remote_pad = NULL;
	int cfa_pattern = 0, hdr_mode = HDR_NONE, offline_channel_idx = 0;
	unsigned int bit_depth = 0, flags = 0;
	struct fe_offline_channel *offline_channel = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct fe_rawdump *rawdump = NULL;
	struct fe_hdr_combine *hdr_combine = NULL;
	struct csi *csi = NULL;
	struct media_pipeline *mpipe = media_entity_pipeline(&sd->entity);
	int i = 0;

	if (format->which != V4L2_SUBDEV_FORMAT_ACTIVE) {
		cam_err("%s(%s) didn't support format which(%d)", __func__,
			sc_subdev->name, format->which);
		return -EINVAL;
	}
	if (format->pad >= PIPE_PAD_NUM) {
		cam_err("%s(%s) invalid pad%d.", __func__, sc_subdev->name, format->pad);
		return -EINVAL;
	}

	if (!mpipe) {
		cam_err("%s(%s) pipe is null", __func__, sc_subdev->name);
		return -1;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);

	switch (format->pad) {
	case PIPE_PAD_IN:
		//if (format->format.width > sc_pipeline->max_width[0]
		//	|| format->format.height > sc_pipeline->max_height[0]
		//	|| format->format.width < sc_pipeline->min_width[0]
		//	|| format->format.height < sc_pipeline->min_height[0]) {
		//	cam_err("%s(%s) %ux%u exceeded max %ux%u min %ux%u", __func__, sc_subdev->name,
		//			format->format.width, format->format.height,
		//			sc_pipeline->max_width[0], sc_pipeline->max_height[0],
		//			sc_pipeline->min_width[0], sc_pipeline->min_height[0]);
		//	return -EINVAL;
		//}
		remote_pad = media_entity_remote_pad(&(pipe->pads[format->pad]));
		if (!remote_pad) {
			cam_err("%s(%s) PIPE_PAD_IN had no active link", __func__, sc_subdev->name);
			return -1;
		}
		csi = media_entity_to_csi(remote_pad->entity);
		if (csi) {
			flags = IDI_FMT_FLAG_ONLINE_INPUT;
		} else {
			flags = IDI_FMT_FLAG_OFFLINE_INPUT;
			offline_channel = media_entity_to_offline_channel(remote_pad->entity);
			if (!offline_channel) {
				cam_err("%s(%s) PIPE_PAD_IN should link to sensor or offline channel", __func__, sc_subdev->name);
				return -1;
			}
			offline_channel_idx = offline_channel->idx;
		}
		if (!fe_isp_lookup_raw_fmts_table(format, flags, &cfa_pattern, &bit_depth)) {
			cam_err("%s(%s) pad%d didn't support format(%dx%d code:0x%08x).", __func__, sc_subdev->name, format->pad,
					format->format.width, format->format.height, format->format.code);
					return -1;
		}
		remote_pad = media_entity_remote_pad(&(pipe->pads[PIPE_PAD_HDROUT]));
		if (remote_pad)
			hdr_combine = media_entity_to_hdr_combine(remote_pad->entity);
		remote_pad = media_entity_remote_pad(&(isp_ctx->pipes[0]->pads[PIPE_PAD_RAWDUMP0OUT]));
		if (remote_pad)
			rawdump = media_entity_to_rawdump(remote_pad->entity);
		if (hdr_combine) {
			if (offline_channel) {
				hdr_mode = HDR_OFFLINE;
			} else {
				if (rawdump)
					hdr_mode = HDR_MIX;
				else
					hdr_mode = HDR_ONLINE;
			}
		} else {
			hdr_mode = HDR_NONE;
		}

		if (offline_channel || (pipe->idx == 0 && hdr_mode == HDR_MIX)) {	// offline
			hw_isp_top_set_idi_offline_input_fmt(SC_BLOCK(pipe),
							     offline_channel_idx,
							     format->format.width,
							     format->format.height,
							     cfa_pattern, bit_depth);
		} else {	// online
			hw_isp_top_set_idi_online_input_fmt(SC_BLOCK(pipe),
							    format->format.width,
							    format->format.height,
							    cfa_pattern);
		}
		for (i = 0; i < PIPE_PAD_NUM; i++)
			pipe->pad_fmts[i].format = format->format;
		break;
	default:
		cam_dbg("%s(%s) didn't support pad%d.", __func__, sc_subdev->name, format->pad);
		return -EINVAL;
	}
	return 0;
}

static int fe_pipe_subdev_pad_get_fmt(struct v4l2_subdev *sd,
				      struct v4l2_subdev_state *state,
				      struct v4l2_subdev_format *format)
{
	struct fe_pipe *pipe = v4l2_subdev_to_pipe(sd);

	if (format->pad >= PIPE_PAD_NUM) {
		cam_dbg("%s(%s) invalid pad%d.", __func__, pipe->sc_subdev.name, format->pad);
		return -EINVAL;
	}

	format->format = pipe->pad_fmts[format->pad].format;

	return 0;
}

static int fe_pipe_subdev_video_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct fe_pipe *pipe = v4l2_subdev_to_pipe(sd);
	struct isp_context *isp_ctx = v4l2_get_subdevdata(sd);
	struct spm_camera_subdev *sc_subdev = &pipe->sc_subdev;
	unsigned int pipe0_width = 0, pipe1_width = 0, idi0_fifo_depth = 0, idi1_fifo_depth = 0;
	unsigned int idi_line_depth = 0, idi_pix_depth = 0, mix_hdr_line = 0, idi_insert_dummy_line = 8;
	struct media_entity *me_pipe0 = (struct media_entity*)isp_ctx->pipes[0];
	struct media_entity *me_pipe1 = (struct media_entity*)isp_ctx->pipes[1];
	struct media_pipeline *mpipe0 = media_entity_pipeline(me_pipe0);
	struct media_pipeline *mpipe1 = media_entity_pipeline(me_pipe1);
	struct media_pad *remote_pad = NULL;
	int idi_source = 0, hdr_mode = HDR_NONE, enable_read_outstanding = 0, sensor_id = 0;
	struct spm_camera_sensor *sc_sensor = NULL;
	struct fe_offline_channel *offline_channel = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL, *sc_pipeline0 = NULL, *sc_pipeline1 = NULL;
	struct fe_rawdump *rawdump = NULL;
	struct fe_hdr_combine *hdr_combine = NULL;
	struct csi *csi = NULL;
	unsigned int irq_bitmap = 0, cap_to_preview = 0;
	unsigned int vi_flags = 0, clk_high = 0, force_sw_gap = 0;
	struct media_pipeline *mpipe = media_entity_pipeline(&sd->entity);
	long l_ret = 0;

	BUG_ON(!me_pipe0);
	BUG_ON(!me_pipe1);
	vi_flags = (pipe->pad_fmts[PIPE_PAD_IN].format.field >> KY_VI_SWITCH_FLAGS_SHIFT) & KY_VI_PRI_DATA_MASK;
	cap_to_preview = vi_flags & KY_VI_FLAG_BACK_TO_PREVIEW;
	//clk_high = vi_flags & KY_VI_FLAG_CLK_HIGH;
	//force_sw_gap = vi_flags & KY_VI_FLAG_FORCE_SW_GAP;
	sensor_id = pipe->pad_fmts[PIPE_PAD_IN].format.field & KY_VI_PRI_DATA_MASK;
	sensor_id = (sensor_id & KY_VI_SENSOR_ID_MASK) >> KY_VI_SENSOR_ID_SHIFT;
	if (mpipe0 && mpipe1) {
		sc_pipeline0 = media_pipeline_to_sc_pipeline(mpipe0);
		sc_pipeline1 = media_pipeline_to_sc_pipeline(mpipe1);
		if (sc_pipeline0->is_online_mode && sc_pipeline1->is_online_mode) {
			clk_high = 1;
			force_sw_gap = 1;
		}
	}
	remote_pad = media_entity_remote_pad(&(pipe->pads[PIPE_PAD_HDROUT]));
	if (remote_pad)
		hdr_combine = media_entity_to_hdr_combine(remote_pad->entity);
	remote_pad = media_entity_remote_pad(&(isp_ctx->pipes[0]->pads[PIPE_PAD_RAWDUMP0OUT]));
	if (remote_pad)
		rawdump = media_entity_to_rawdump(remote_pad->entity);
	remote_pad = media_entity_remote_pad(&(pipe->pads[PIPE_PAD_IN]));
	if (!remote_pad) {
		cam_err("%s(%s) PIPE_PAD_IN had no active link", __func__, sc_subdev->name);
		return -1;
	}
	csi = media_entity_to_csi(remote_pad->entity);
	if (csi) {
		remote_pad = media_entity_remote_pad(&(csi->pads[CSI_PAD_IN]));
		BUG_ON(!remote_pad);
		sc_sensor = media_entity_to_sc_sensor(remote_pad->entity);
	}
	if (!sc_sensor) {
		offline_channel = media_entity_to_offline_channel(remote_pad->entity);
	}
	if (pipe->idx == 0)
		irq_bitmap = POSTERR_IRQ_PIP0_SDW_CLOSE_DONE;
	else
		irq_bitmap = POSTERR_IRQ_PIP1_SDW_CLOSE_DONE;
	if (!mpipe) {
		cam_err("%s entity pipe is null", __func__);
		return -1;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	BUG_ON(!sc_pipeline);
	if (enable) {
		if (clk_high)
			fe_isp_set_clk(isp_ctx, ISP_CLK_HIGH);
		else
			fe_isp_set_clk(isp_ctx, ISP_CLK_LOW);
		isp_ctx->isp_fatal_error = 0;
		isp_ctx->dma_overlap_cnt = 0;
		if (isp_ctx->dma_block)
			hw_dma_reset(isp_ctx->dma_block);
		atomic_set(&sc_pipeline->slice_info_update, 0);
		fe_isp_reset_frame_id(sc_pipeline);
		if (mpipe0) {
			pipe0_width = CAM_ALIGN(isp_ctx->pipes[0]->pad_fmts[PIPE_PAD_IN].format.width, 8);
			if (pipe0_width == 0)
				pipe0_width = 1920;
		}
		if (mpipe1) {
			pipe1_width = CAM_ALIGN(isp_ctx->pipes[1]->pad_fmts[PIPE_PAD_IN].format.width, 8);
			if (pipe1_width == 0)
				pipe1_width = 1920;
		}
		if (hdr_combine) {
			if (offline_channel) {
				hdr_mode = HDR_OFFLINE;
			} else if (sc_sensor) {
				if (rawdump)
					hdr_mode = HDR_MIX;
				else
					hdr_mode = HDR_ONLINE;
			} else {
				cam_err("%s(%s) PIPE_PAD_IN should link to offline_channel or sensor", __func__, sc_subdev->name);
				return -1;
			}
		} else {
			hdr_mode = HDR_NONE;
		}
		if (hdr_mode == HDR_MIX) {
			enable_read_outstanding = 1;
			idi_insert_dummy_line = 9;
		} else {
			idi_insert_dummy_line = 8;
		}
		BUG_ON(0 == (pipe0_width + pipe1_width));
		if (hdr_mode == HDR_MIX) {
			idi1_fifo_depth = pipe1_width >> 1;
			idi0_fifo_depth = 4750 - idi1_fifo_depth;
			BUG_ON(0 == isp_ctx->pipes[0]->pad_fmts[PIPE_PAD_IN].format.width);
		} else {
			if (pipe0_width > 0 && pipe1_width > 0)
				idi0_fifo_depth = 4750 >> 1;
			else
				idi0_fifo_depth = (4750 * pipe0_width) / (pipe0_width + pipe1_width);
			idi1_fifo_depth = 4750 - idi0_fifo_depth;
		}
		if (!enable_read_outstanding) {
			if (pipe0_width == 0)
				pipe0_width = 1920;
			if (pipe1_width == 0)
				pipe1_width = 1920;
			idi_line_depth = (idi0_fifo_depth << 2) / pipe0_width;
			idi_pix_depth = (idi0_fifo_depth << 2) - idi_line_depth * pipe0_width;
			idi_line_depth = (idi1_fifo_depth << 2) / pipe1_width;
			idi_pix_depth = (idi1_fifo_depth << 2) - idi_line_depth * pipe1_width;
			hw_isp_top_enable_rd_outstanding(SC_BLOCK(isp_ctx->pipes[0]), 0);
		} else { // enable read outstanding
			BUG_ON(0 == pipe0_width);
			BUG_ON(0 == pipe1_width);
			BUG_ON(pipe0_width != pipe1_width);
			idi_line_depth = (idi0_fifo_depth << 2) / pipe0_width;
			idi_line_depth >>= 1;
			idi_line_depth <<= 1;
			idi0_fifo_depth = (idi_line_depth * pipe0_width) >> 2;
			idi1_fifo_depth = 4750 - idi0_fifo_depth;
			if (0 == pipe->idx) {
				idi_pix_depth = (idi0_fifo_depth << 2) - idi_line_depth * pipe0_width;
			} else {
				idi_line_depth = (idi1_fifo_depth << 2) / pipe1_width;
				idi_pix_depth = (idi1_fifo_depth << 2) - idi_line_depth * pipe1_width;
			}
			hw_isp_top_enable_rd_outstanding(SC_BLOCK(isp_ctx->pipes[0]), 1);
		}
		if (csi) {
			if (hdr_mode == HDR_MIX) {
				if (pipe->idx == 0) {
					idi_source = OFFLINE_CH0;
				} else {
					if (sensor_id == 0)
						idi_source = SENSOR1_CH1;
					else
						idi_source = SENSOR0_CH1;
				}
			} else {
				if (sensor_id == 0)
					idi_source = SENSOR0_CH0;
				else
					idi_source = SENSOR1_CH0;
			}

		} else {
			if (!offline_channel) {
				cam_err("%s(%s) PIPE_PAD_IN should link to offline_channel or sensor", __func__, sc_subdev->name);
				return -1;
			}
			if (offline_channel->idx == 0)
				idi_source = OFFLINE_CH0;
			else
				idi_source = OFFLINE_CH1;
		}
		hw_isp_top_set_idi_input_source(SC_BLOCK(pipe), idi_source);
		hw_isp_top_set_idi_dummyline(SC_BLOCK(isp_ctx->pipes[0]), idi_insert_dummy_line);
		if (csi) {
			hw_isp_top_enable_vsync_pass_through(SC_BLOCK(isp_ctx->pipes[0]), pipe->idx, 1);
			hw_isp_top_set_vsync2href_dly_cnt(SC_BLOCK(isp_ctx->pipes[0]), pipe->idx, 0xc8);
		} else {
			hw_isp_top_enable_vsync_pass_through(SC_BLOCK(isp_ctx->pipes[0]), pipe->idx, 0);
			hw_isp_top_set_vsync2href_dly_cnt(SC_BLOCK(isp_ctx->pipes[0]), pipe->idx, 0x3e8);
		}
		hw_isp_top_enable_hdr(SC_BLOCK(isp_ctx->pipes[0]), hdr_mode);
		if (hdr_mode == HDR_MIX) {
			mix_hdr_line = CAM_ALIGN((idi0_fifo_depth << 3) / isp_ctx->pipes[0]->pad_fmts[PIPE_PAD_IN].format.width, 2);
			hw_isp_top_set_mix_hdr_line(SC_BLOCK(isp_ctx->pipes[0]), mix_hdr_line);
			hw_isp_top_set_ddr_wr_line(SC_BLOCK(isp_ctx->pipes[0]), 2);
		}
		if (csi)
			hw_isp_top_set_irq_enable(SC_BLOCK(pipe), ISP_IRQ_SDE_SOF, 0);
		hw_isp_top_clr_err0_irq_status(SC_BLOCK(isp_ctx->pipes[0]), 0xffffffff);
		hw_isp_top_set_err0_irq_enable(SC_BLOCK(isp_ctx->pipes[0]), 0xffffffff, 0);
		hw_isp_top_clr_err2_irq_status(SC_BLOCK(isp_ctx->pipes[0]), 0xffffffff);
		hw_isp_top_set_err2_irq_enable(SC_BLOCK(isp_ctx->pipes[0]), 0xffffffff, 0);
		hw_isp_top_clr_posterr_irq_status(SC_BLOCK(isp_ctx->pipes[0]), irq_bitmap);
		hw_isp_top_set_posterr_irq_enable(SC_BLOCK(isp_ctx->pipes[0]), irq_bitmap, 0);
		hw_isp_top_set_gap_value(SC_BLOCK(isp_ctx->pipes[0]), 200, 200, 200);
		if (csi && !force_sw_gap) {
			hw_isp_top_enable_hw_gap(SC_BLOCK(isp_ctx->pipes[0]), pipe->idx, 1);
		} else {
			hw_isp_top_enable_hw_gap(SC_BLOCK(isp_ctx->pipes[0]), pipe->idx, 0);
		}
		if (sc_pipeline0 && sc_pipeline1) {
			if ((sc_pipeline0->is_online_mode && !sc_pipeline1->is_online_mode)
				|| (!sc_pipeline0->is_online_mode && sc_pipeline1->is_online_mode)) {
				hw_isp_top_set_speed_ctrl(SC_BLOCK(isp_ctx->pipes[0]), 1);
			} else {
				hw_isp_top_set_speed_ctrl(SC_BLOCK(isp_ctx->pipes[0]), 0);
			}
		} else {
				hw_isp_top_set_speed_ctrl(SC_BLOCK(isp_ctx->pipes[0]), 0);
		}
		if (!cap_to_preview)
			hw_isp_top_shadow_latch(SC_BLOCK(pipe));
	} else {
		complete_all(&sc_pipeline->slice_done);
		if (csi) {
			reinit_completion(&(pipe->close_done));
		}
		hw_isp_top_set_idi_input_source(SC_BLOCK(pipe), INVALID_CH);
		hw_isp_top_set_cfg_rdy(SC_BLOCK(pipe), 1);
		if (csi) {
			l_ret = wait_for_completion_interruptible_timeout(&(pipe->close_done), msecs_to_jiffies(500));
			if (l_ret == 0)
				cam_warn("%s(%s) wait stream off timeout", __func__, sc_subdev->name);
			else if (l_ret < 0)
				cam_warn("%s(%s) wait stream off interrputed by user app", __func__, sc_subdev->name);
			else
				cam_dbg("%s(%s) wait stream off ok", __func__, sc_subdev->name);
		}
		hw_isp_top_enable_vsync_pass_through(SC_BLOCK(isp_ctx->pipes[0]), pipe->idx, 0);
		hw_isp_top_set_vsync2href_dly_cnt(SC_BLOCK(isp_ctx->pipes[0]), pipe->idx, 0);
		hw_isp_top_set_posterr_irq_enable(SC_BLOCK(isp_ctx->pipes[0]), 0, irq_bitmap);
	}
	return 0;
}

static int fe_isp_pipeline_notifier_handler(struct notifier_block *nb,
					    unsigned long action, void *data)
{
	struct fe_x *x = container_of(nb, struct fe_x, pipeline_notify_block);
	struct fe_pipe *pipe = NULL;
	struct csi *csi = NULL;
	struct fe_offline_channel *offline_channel = NULL;
	struct media_pad *remote_pad = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct list_head *pos = NULL, *n = NULL;
	unsigned long flags = 0;
	int i = 0, j = 0, ret = 0;
	unsigned int idi_fifo_depth = 0, idi_line_depth = 0, idi_pix_depth = 0;
	struct v4l2_subdev_format format = { 0 };
	unsigned int bit_depth = 0;

	csi = v4l2_subdev_to_csi(&(x->sc_subdev.pcsd.sd));
	pipe = v4l2_subdev_to_pipe(&(x->sc_subdev.pcsd.sd));
	switch (action) {
	case PIPELINE_ACTION_PIPE_ACK:
		//pipe = v4l2_subdev_to_pipe(&(x->sc_subdev.pcsd.sd));
		if (pipe)
			return NOTIFY_OK | NOTIFY_STOP_MASK;
		return NOTIFY_DONE;
		break;
	case PIPELINE_ACTION_CLEAN_USR_DATA:
		sc_pipeline = (struct spm_camera_pipeline *)data;
		pipe_ctx = (struct isp_pipeline_context *)sc_pipeline->usr_data;
		if (pipe_ctx) {
			spin_lock_irqsave(&sc_pipeline->slock, flags);
			for (i = 0; i < FORMATTER_NUM; i++) {
				list_for_each_safe(pos, n, &pipe_ctx->fmt_wdma_list[i]) {
					list_del_init(pos);
				}
			}
			for (i = 0; i < FORMATTER_NUM; i++) {
				for (j = 0; j < VB2_MAX_FRAME; j++) {
					INIT_LIST_HEAD(&pipe_ctx->fmt_wdma_sync[i][j]);
					pipe_ctx->fmt_wdma_sync_cnt[i][j] = 0;
				}
			}
			list_for_each_safe(pos, n, &(pipe_ctx->wdma_list)) {
				list_del_init(pos);
			}
			pipe_ctx->mmu_tbu_reload = MMU_TBU_OK;
			spin_unlock_irqrestore(&sc_pipeline->slock, flags);
		}
		return NOTIFY_OK | NOTIFY_STOP_MASK;
		break;
	case PIPELINE_ACTION_SENSOR_STREAM_ON:
		if (csi) {
			ret = csi_subdev_video_s_stream(&(csi->sc_subdev.pcsd.sd), 1);
			if (ret)
				return NOTIFY_BAD;
			return NOTIFY_OK | NOTIFY_STOP_MASK;
		}
		break;
	case PIPELINE_ACTION_SENSOR_STREAM_OFF:
		if (csi) {
			ret = csi_subdev_video_s_stream(&(csi->sc_subdev.pcsd.sd), 0);
			if (ret)
				return NOTIFY_BAD;
			return NOTIFY_OK | NOTIFY_STOP_MASK;
		}
		break;
	case PIPELINE_ACTION_SLICE_READY:
		if (pipe) {
			sc_pipeline = (struct spm_camera_pipeline *)data;
			BUG_ON(!sc_pipeline);
			pipe_ctx = (struct isp_pipeline_context *)sc_pipeline->usr_data;
			BUG_ON(!pipe_ctx);
			idi_fifo_depth = hw_isp_top_get_idi_fifo_depth(SC_BLOCK(pipe));
			idi_line_depth = (idi_fifo_depth << 2) / pipe_ctx->cc_slice_info.slice_width;
			idi_pix_depth = (idi_fifo_depth << 2) - idi_line_depth * pipe_ctx->cc_slice_info.slice_width;
			//hw_isp_top_set_idi_linebuf(SC_BLOCK(pipe), idi_fifo_depth, idi_line_depth, idi_pix_depth);
			format.format = pipe->pad_fmts[PIPE_PAD_IN].format;
			ret = fe_isp_lookup_raw_fmts_table(&format, IDI_FMT_FLAG_OFFLINE_INPUT, NULL, &bit_depth);
			BUG_ON(!ret);
			remote_pad = media_entity_remote_pad(&pipe->pads[PIPE_PAD_IN]);
			BUG_ON(!remote_pad);
			offline_channel = media_entity_to_offline_channel(remote_pad->entity);
			BUG_ON(!offline_channel);
			hw_isp_top_set_idi_offline_input_fmt(SC_BLOCK(pipe),
							     offline_channel->idx,
							     pipe_ctx->cc_slice_info.slice_width,
							     format.format.height,
							     0, bit_depth);
			//cam_not("slice width=%d idi_fifo_depth=%u", pipe_ctx->cc_slice_info.slice_width, idi_fifo_depth);
		}
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_DONE;
}

static int __fe_rawdump_pad_s_stream(struct fe_rawdump *rawdump, unsigned int pad, int enable)
{
	int source = 0, ret = 0, need_initial_load = 0, rawdump_only = 0;
	struct media_pad *remote_pad_out = NULL, *remote_pad_in = NULL;
	//struct spm_camera_sensor *sc_sensor = NULL;
	struct fe_pipe *pipe = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct spm_camera_subdev *sc_subdev = &rawdump->sc_subdev;
	struct device *dev = sc_subdev->pcsd.sd.dev;
	struct media_pipeline *mpipe = media_entity_pipeline(&sc_subdev->pcsd.sd.entity);
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct isp_context *isp_ctx = spm_subdev_get_drvdata(sc_subdev);
	struct spm_camera_vbuffer *sc_vb = NULL;
	struct vi_port_cfg *port_cfg = NULL;
	unsigned int wdma_fifo_offset = 0, wdma_fifo_depth = 0, wdma_weight = 0, wdma_fifo_div_mode = 8;
	unsigned long flags = 0;

	if (!mpipe) {
		cam_err("%s(%s) pipe was null", __func__, sc_subdev->name);
		return -1;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	switch (pad) {
	case PAD_OUT:
		remote_pad_out = media_entity_remote_pad(&(rawdump->pads[pad]));
		if (!remote_pad_out) {
			cam_err("%s(%s) PAD_OUT had no active link.", __func__, sc_subdev->name);
			return -1;
		}
		sc_vnode = media_entity_to_sc_vnode(remote_pad_out->entity);
		if (!sc_vnode) {
			cam_err("%s(%s) PAD_OUT had no link to vnode.", __func__, sc_subdev->name);
			return -1;
		}
		remote_pad_in = media_entity_remote_pad(&(rawdump->pads[PAD_IN]));
		if (!remote_pad_in) {
			cam_err("%s(%s) PAD_IN had no active link.", __func__, sc_subdev->name);
			return -1;
		}
		//sc_sensor = media_entity_to_sc_sensor(remote_pad_in->entity);
		pipe = media_entity_to_pipe(remote_pad_in->entity);
		//if (pipe && !sc_sensor) {
		//	remote_pad_in = media_entity_remote_pad(&(pipe->pads[PIPE_PAD_IN]));
		//	if (!remote_pad_in) {
		//		cam_err("%s(%s) PAD_IN->pipe had no active input link", __func__, sc_subdev->name);
		//		return -1;
		//	}
		//	sc_sensor = media_entity_to_sc_sensor(remote_pad_in->entity);
		//}
		//if (!sc_sensor) {
		//	cam_err("%s(%s) PAD_IN had no link to sensor or pipe->sensor.", __func__, sc_subdev->name);
		//	return -1;
		//}
		if (enable) {
			atomic_set(&isp_ctx->dma_out_ctx[sc_vnode->idx].busy_cnt, 0);
			if (rawdump->idx == 0)
				source = RAWDUMP0;
			else
				source = RAWDUMP1;
			port_cfg = sc_vnode_get_usrdata(sc_vnode);
			if (port_cfg) {
				wdma_fifo_offset = port_cfg->w_fifo_ctrl.offset;
				wdma_fifo_depth = port_cfg->w_fifo_ctrl.depth;
				wdma_weight = port_cfg->w_fifo_ctrl.weight;
				wdma_fifo_div_mode = port_cfg->w_fifo_ctrl.div_mode;
				ret = blocking_notifier_call_chain(&sc_pipeline->blocking_notify_chain,
								   PIPELINE_ACTION_PIPE_ACK, NULL);
				if (NOTIFY_STOP == ret) {
					port_cfg->buf_required_min = 0;
					rawdump_only = 0;
				} else {
					port_cfg->buf_required_min = 0;
					need_initial_load = 1;
					rawdump_only = 1;
				}
			}
			if (rawdump_only)
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 1;
			else
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 0;
			hw_dma_set_wdma_source(SC_BLOCK(sc_vnode),
								sc_vnode->idx,
								source,
								wdma_fifo_offset,
								wdma_fifo_depth,
								wdma_weight,
								wdma_fifo_div_mode);
			if (pipe || need_initial_load) { //mix hdr or need_initial_load
				spin_lock_irqsave(&sc_vnode->slock, flags);
				ret = __spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
				if (ret) {
					if (pipe) {
						ret = __spm_vdev_pick_busy_vbuffer(sc_vnode, &sc_vb);
						if (ret) {
							spin_unlock_irqrestore(&sc_vnode->slock, flags);
							cam_err("%s(%s) no buffer available for mix hdr rawdump", __func__, sc_subdev->name);
							return ret;
						}
					} else {
						//if (rawdump_only) {
						//	cam_err("%s(%s) no initial buffer available for rawdump only", __func__, sc_subdev->name);
						//	return ret;
						//} else {
						//	if (sc_vnode->sc_vb) {
						//		sc_vb = sc_vnode->sc_vb;
						//	} else {
								cam_info("%s(%s) no initial buffer available", __func__, sc_subdev->name);
						//	}
						//}
					}
				} else {
					__spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
				}
				if (sc_vb) {
					fe_isp_update_aout_dma_addr(sc_vnode, sc_vb, 0);
					hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 1);
				}
				spin_unlock_irqrestore(&sc_vnode->slock, flags);
			}
			hw_dma_enable_rawdump(SC_BLOCK(sc_vnode), rawdump->idx, 1);
			if (isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload && !pipe) {
				hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
						      DMA_IRQ_SRC_WDMA_CH0 + sc_vnode->idx,
						      DMA_IRQ_START | DMA_IRQ_DONE | DMA_IRQ_ERR,
						      0);
			} else {
				hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
						      DMA_IRQ_SRC_WDMA_CH0 + sc_vnode->idx,
						      DMA_IRQ_DONE | DMA_IRQ_ERR,
						      DMA_IRQ_START);
			}
			isp_ctx->dma_out_ctx[sc_vnode->idx].vnode = sc_vnode;
			if (pipe) { //mix hdr
				BUG_ON(!sc_vb);
				sc_vb->flags |= SC_BUF_FLAG_SPECIAL_USE;
				hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
						      DMA_IRQ_SRC_RDMA_CH0,
						      DMA_IRQ_START | DMA_IRQ_DONE | DMA_IRQ_ERR,
						      0);
				hw_dma_update_rdma_address(SC_BLOCK(sc_vnode), 0, (uint64_t)spm_vb2_buf_paddr(&(sc_vb->vb2_v4l2_buf.vb2_buf), 0));
				hw_isp_top_set_idi_rd_burst_len(SC_BLOCK(isp_ctx->pipes[0]), 0, 22, 32);
			} else {
				mutex_lock(&sc_pipeline->mlock);
				if (!sc_pipeline->usr_data) {
					pipe_ctx = fe_pipeline_create_ctx(dev);
					if (!pipe_ctx) {
						mutex_unlock(&sc_pipeline->mlock);
						cam_err("%s(%s) create pipeline context failed", __func__, sc_subdev->name);
						return -ENOMEM;
					}
					sc_pipeline->usr_data = pipe_ctx;
				} else {
					pipe_ctx = (struct isp_pipeline_context*)sc_pipeline->usr_data;
				}
				mutex_unlock(&sc_pipeline->mlock);
				spin_lock_irqsave(&sc_pipeline->slock, flags);
				list_add_no_repeat(&isp_ctx->dma_out_ctx[sc_vnode->idx].list_entry, &pipe_ctx->wdma_list);
				spin_unlock_irqrestore(&sc_pipeline->slock, flags);
			}
		} else {// stream off
			ret = wait_event_interruptible_timeout(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_eof,
							       (atomic_read(&isp_ctx->dma_out_ctx[sc_vnode->idx].busy_cnt) <= 0),
							       msecs_to_jiffies(60));
			if (0 == ret)
				cam_warn("%s(%s) stream off wait eof timeout", __func__, sc_subdev->name);
			else if (ret < 0)
				cam_warn("%s(%s) stream off wait eof error ret=%d", __func__, sc_subdev->name, ret);

			spin_lock_irqsave(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
			wait_event_interruptible_locked_irq(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head,
							    !isp_ctx->dma_out_ctx[sc_vnode->idx].in_irq);
			isp_ctx->dma_out_ctx[sc_vnode->idx].in_streamoff = 1;
			spin_unlock_irqrestore(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
			hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
					      DMA_IRQ_SRC_WDMA_CH0 + sc_vnode->idx,
					      0,
					      DMA_IRQ_ALL);
			hw_dma_enable_rawdump(SC_BLOCK(sc_vnode), rawdump->idx, 0);
			if (!pipe) {
				pipe_ctx = (struct isp_pipeline_context*)sc_pipeline->usr_data;
				if (pipe_ctx) {
					spin_lock_irqsave(&sc_pipeline->slock, flags);
					list_del_init(&isp_ctx->dma_out_ctx[sc_vnode->idx].list_entry);
					spin_unlock_irqrestore(&sc_pipeline->slock, flags);
				}
			} else { //mix hdr
				hw_dma_set_irq_enable(SC_BLOCK(sc_vnode),
						      DMA_IRQ_SRC_RDMA_CH0,
						      0,
						      DMA_IRQ_ALL);
			}
			isp_ctx->dma_out_ctx[sc_vnode->idx].vnode = NULL;
			spin_lock_irqsave(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
			isp_ctx->dma_out_ctx[sc_vnode->idx].in_streamoff = 0;
			spin_unlock_irqrestore(&(isp_ctx->dma_out_ctx[sc_vnode->idx].waitq_head.lock), flags);
		}
		break;
	default:
		return -1;
	}

	ret = spm_subdev_pad_s_stream(sc_subdev, pad, enable);
	if (ret) {
		cam_err("%s(%s) s_stream on pad%u failed", __func__, sc_subdev->name, pad);
		return ret;
	}
	return 0;
}

static int fe_rawdump_subdev_pad_s_stream(struct v4l2_subdev *sd,
					  struct media_link *link,
					  struct v4l2_subdev_format *source_fmt,
					  struct v4l2_subdev_format *sink_fmt)
{
	struct fe_rawdump *rawdump = v4l2_subdev_to_rawdump(sd);
	unsigned int pad = source_fmt->pad;
	int enable = source_fmt->which;

	return __fe_rawdump_pad_s_stream(rawdump, pad, enable);
}

static int fe_hdr_combine_subdev_pad_set_fmt(struct v4l2_subdev *sd,
					     struct v4l2_subdev_state *state,
					     struct v4l2_subdev_format *format)
{
	struct fe_hdr_combine *hdr_combine = v4l2_subdev_to_hdr_combine(sd);
	struct spm_camera_subdev *sc_subdev = NULL;
	int i = 0;

	if (!hdr_combine) {
		cam_err("%sinvalid sd(%s)", __func__, sd->name);
		return -EINVAL;
	}
	sc_subdev = &hdr_combine->sc_subdev;

	if (format->which != V4L2_SUBDEV_FORMAT_ACTIVE) {
		cam_err("%s(%s) didn't support format which(%d)", __func__,
			sc_subdev->name, format->which);
		return -EINVAL;
	}
	if (format->pad >= HDR_COMBINE_PAD_NUM) {
		cam_err("%s(%s) invalid pad%d.", __func__, sc_subdev->name, format->pad);
		return -EINVAL;
	}

	switch (format->pad) {
	case HDR_PAD_P0IN:
	case HDR_PAD_P1IN:
		for (i = 0; i < HDR_COMBINE_PAD_NUM; i++)
			hdr_combine->pad_fmts[i].format = format->format;

		break;
	default:
		cam_dbg("%s(%s) didn't support pad%d.", __func__, sc_subdev->name, format->pad);
		return -EINVAL;
	}

	return 0;
}

static int fe_hdr_combine_subdev_pad_get_fmt(struct v4l2_subdev *sd,
					     struct v4l2_subdev_state *state,
					     struct v4l2_subdev_format *format)
{
	struct fe_hdr_combine *hdr_combine = v4l2_subdev_to_hdr_combine(sd);

	if (!hdr_combine) {
		cam_err("%s invalid sd(%s)", __func__, sd->name);
		return -EINVAL;
	}

	if (format->pad >= HDR_COMBINE_PAD_NUM) {
		cam_dbg("%s(%s) invalid pad%d.", __func__, hdr_combine->sc_subdev.name,
			format->pad);
		return -EINVAL;
	}

	format->format = hdr_combine->pad_fmts[format->pad].format;

	return 0;
}

static int csi_subdev_pad_set_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_format *format)
{
	struct csi *csi = v4l2_subdev_to_csi(sd);
	struct spm_camera_subdev *sc_subdev = &csi->sc_subdev;
	int i = 0;

	if (format->which != V4L2_SUBDEV_FORMAT_ACTIVE) {
		cam_err("%s(%s) didn't support format which(%d)", __func__,
			sc_subdev->name, format->which);
		return -EINVAL;
	}

	switch (format->pad) {
	case CSI_PAD_IN:
		for (i = 0; i < CSI_PAD_NUM; i++)
			csi->pad_fmts[i].format = format->format;
		break;
	default:
		cam_dbg("%s(%s) didn't support pad%d.", __func__, sc_subdev->name, format->pad);
		return -EINVAL;
	}
	return 0;
}

static int csi_subdev_pad_get_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_format *format)
{
	struct csi *csi = v4l2_subdev_to_csi(sd);

	if (format->pad >= CSI_PAD_NUM) {
		cam_dbg("%s didn't have pad%d.", csi->sc_subdev.name, format->pad);
		return -EINVAL;
	}

	format->format = csi->pad_fmts[format->pad].format;

	return 0;
}

static int csi_subdev_pad_s_stream(struct v4l2_subdev *sd, struct media_link *link,
				   struct v4l2_subdev_format *source_fmt,
				   struct v4l2_subdev_format *sink_fmt)
{
	return 0;
}

static struct v4l2_subdev_pad_ops rawdump_subdev_pad_ops = {
	.set_fmt = fe_rawdump_subdev_pad_set_fmt,
	.get_fmt = fe_rawdump_subdev_pad_get_fmt,
	.link_validate = fe_rawdump_subdev_pad_s_stream,
};

static struct v4l2_subdev_pad_ops offline_channel_subdev_pad_ops = {
	.set_fmt = fe_offline_channel_subdev_pad_set_fmt,
	.get_fmt = fe_offline_channel_subdev_pad_get_fmt,
	.link_validate = fe_offline_channel_subdev_pad_s_stream,
};

static struct v4l2_subdev_pad_ops formatter_subdev_pad_ops = {
	.set_fmt = fe_formatter_subdev_pad_set_fmt,
	.get_fmt = fe_formatter_subdev_pad_get_fmt,
	.link_validate = fe_formatter_subdev_pad_s_stream,
};

static struct v4l2_subdev_pad_ops dwt_subdev_pad_ops = {
	.set_fmt = fe_dwt_subdev_pad_set_fmt,
	.get_fmt = fe_dwt_subdev_pad_get_fmt,
	.link_validate = fe_dwt_subdev_pad_s_stream,
};

static struct v4l2_subdev_pad_ops pipe_subdev_pad_ops = {
	.set_fmt = fe_pipe_subdev_pad_set_fmt,
	.get_fmt = fe_pipe_subdev_pad_get_fmt,
};

static struct v4l2_subdev_pad_ops hdr_combine_subdev_pad_ops = {
	.set_fmt = fe_hdr_combine_subdev_pad_set_fmt,
	.get_fmt = fe_hdr_combine_subdev_pad_get_fmt,
};

static struct v4l2_subdev_pad_ops csi_subdev_pad_ops = {
	.set_fmt = csi_subdev_pad_set_fmt,
	.get_fmt = csi_subdev_pad_get_fmt,
	.link_validate = csi_subdev_pad_s_stream,
};

static long fe_isp_global_reset(struct isp_context *isp_ctx)
{
	reinit_completion(&isp_ctx->global_reset_done);
	hw_isp_top_global_reset(SC_BLOCK(isp_ctx->pipes[0]));
	return wait_for_completion_interruptible_timeout(&isp_ctx->global_reset_done, msecs_to_jiffies(500));
}
extern void dpu_mclk_exclusive_put(void);
extern bool dpu_mclk_exclusive_get(void);

static int __fe_isp_s_power(struct v4l2_subdev *sd, int on)
{
	struct spm_camera_subdev *sc_subdev = v4l2_subdev_to_sc_subdev(sd);
	struct isp_context *isp_ctx = v4l2_get_subdevdata(sd);
	int v = 0;
	int ret = 0;
	long l_ret = 0;
	unsigned int idi0_fifo_depth = 0, idi1_fifo_depth = 0;

	//HW sequence
	if (on) {
		if (atomic_inc_return(&isp_ctx->pwr_cnt) == 1) {
			cam_not("vi s_power 1");
#ifdef CONFIG_ARCH_KY
			ret = pm_runtime_get_sync(&isp_ctx->pdev->dev);
			if (ret < 0) {
				cam_err("rpm get failed");
				return -1;
			}
			pm_stay_awake(&isp_ctx->pdev->dev);

//			fe_isp_set_clk(isp_ctx, ISP_CLK_LOW);

			reset_control_deassert(isp_ctx->ahb_reset);
//			clk_prepare_enable(isp_ctx->ahb_clk);

			clk_prepare_enable(isp_ctx->fnc_clk);
			reset_control_deassert(isp_ctx->isp_reset);

			clk_prepare_enable(isp_ctx->bus_clk);
			reset_control_deassert(isp_ctx->isp_ci_reset);

			clk_prepare_enable(isp_ctx->dpu_clk);
			reset_control_deassert(isp_ctx->lcd_mclk_reset);
			fe_isp_set_clk(isp_ctx, ISP_CLK_LOW);

#endif
#ifdef CONFIG_KY_DEBUG
			vi_running_info.b_dev_running = true;
#endif
			hw_isp_top_enable_debug_clk(SC_BLOCK(isp_ctx->pipes[0]), 1);
			hw_isp_top_set_irq_enable(SC_BLOCK(isp_ctx->pipes[0]), ISP_IRQ_G_RST_DONE, 0);
			hw_isp_top_set_irq_enable(SC_BLOCK(isp_ctx->pipes[0]),
							ISP_IRQ_PIPE_SOF | ISP_IRQ_STATS_ERR | ISP_IRQ_IDI_SHADOW_DONE,
							0);
			hw_isp_top_set_irq_enable(SC_BLOCK(isp_ctx->pipes[1]),
							ISP_IRQ_PIPE_SOF | ISP_IRQ_STATS_ERR | ISP_IRQ_IDI_SHADOW_DONE,
							0);
			idi0_fifo_depth = 4750 >> 1;
			idi1_fifo_depth = 4750 - idi0_fifo_depth;
			hw_isp_top_set_idi_linebuf(SC_BLOCK(isp_ctx->pipes[0]), idi0_fifo_depth, 0, 0);
			hw_isp_top_set_idi_linebuf(SC_BLOCK(isp_ctx->pipes[1]), idi1_fifo_depth, 0, 0);
			hw_dma_reset(isp_ctx->dma_block);
#if IS_ENABLED(CONFIG_DRM_KY)
			while (1) {
				if (dpu_mclk_exclusive_get()) {
					clk_set_rate(isp_ctx->dpu_clk, 409600000);
					if (ret < 0 && ret != -EBUSY) {
						cam_err("%s lock dpu clk failed ret=%d", __func__, ret);
						return ret;
					} else if (ret == 0) {
						break;
					}
				} else {
					continue;
				}
			}
#endif
#ifdef CONFIG_KY_X1_VI_IOMMU
			isp_ctx->mmu_dev->ops->set_timeout_default_addr(isp_ctx->mmu_dev, (uint64_t)isp_ctx->rsvd_phy_addr);
#endif
		}
	} else {
		v = atomic_dec_return(&isp_ctx->pwr_cnt);
		if (v == 0) {
#if IS_ENABLED(CONFIG_DRM_KY)
			dpu_mclk_exclusive_put();
#endif
			l_ret = fe_isp_global_reset(isp_ctx);
			if (l_ret == 0)
				cam_err("%s global reset timeout", __func__);
			else if (l_ret < 0)
				cam_err("%s global reset is interrupted by user app", __func__);
			else
				cam_dbg("%s global reset done", __func__);

#ifdef CONFIG_KY_DEBUG
			vi_running_info.b_dev_running = false;
#endif
			cam_not("vi s_power 0");
#ifdef CONFIG_ARCH_KY
//			clk_disable_unprepare(isp_ctx->ahb_clk);
			reset_control_assert(isp_ctx->ahb_reset);

			reset_control_deassert(isp_ctx->isp_reset);
			clk_disable_unprepare(isp_ctx->fnc_clk);

			reset_control_deassert(isp_ctx->isp_ci_reset);
			clk_disable_unprepare(isp_ctx->bus_clk);

			reset_control_deassert(isp_ctx->lcd_mclk_reset);
			clk_disable_unprepare(isp_ctx->dpu_clk);

			pm_relax(&isp_ctx->pdev->dev);
			pm_runtime_put_sync(&isp_ctx->pdev->dev);
#endif
		} else if (v < 0) {
			atomic_inc(&isp_ctx->pwr_cnt);
			cam_err("%s(%s) invalid power off", __func__, sc_subdev->name);
		}
	}

	return 0;
}

int fe_isp_s_power(void *isp_context, int on)
{
	struct isp_context *isp_ctx = isp_context;

	if (isp_ctx->pipes[0] == NULL)
		return -10;

	return __fe_isp_s_power(&(isp_ctx->pipes[0]->sc_subdev.pcsd.sd), on);
}

static int csi_subdev_core_s_power(struct v4l2_subdev *sd, int on)
{
	struct spm_camera_subdev *sc_subdev = v4l2_subdev_to_sc_subdev(sd);
	struct isp_context *isp_ctx = v4l2_get_subdevdata(sd);
	struct csi *csi = v4l2_subdev_to_csi(sd);
	struct ccic_ctrl *csi_ctrl = NULL;
	int v = 0, sensor_id = 0;

	sensor_id = csi->pad_fmts[CSI_PAD_IN].format.field & KY_VI_PRI_DATA_MASK;
	sensor_id = (sensor_id & KY_VI_SENSOR_ID_MASK) >> KY_VI_SENSOR_ID_SHIFT;
	csi_ctrl = isp_ctx->ccic[sensor_id].csi_ctrl;
	BUG_ON(!csi_ctrl);
	if (on) {
		if (atomic_inc_return(&(isp_ctx->ccic[sensor_id].pwr_cnt)) == 1) {
			cam_not("csi%d s_power 1", sensor_id);
			csi_ctrl->ops->clk_enable(csi_ctrl, 1);
		}
	} else {
		v = atomic_dec_return(&(isp_ctx->ccic[sensor_id].pwr_cnt));
		if (v == 0) {
			cam_not("csi s_power 0");
			csi_ctrl->ops->clk_enable(csi_ctrl, 0);
		} else if (v < 0) {
			atomic_inc(&(isp_ctx->ccic[sensor_id].pwr_cnt));
			cam_err("%s(%s) invalid power off", __func__, sc_subdev->name);
		}
	}
	return 0;
}

static int csi_subdev_core_reset(struct v4l2_subdev *sd, u32 val)
{
	if (RESET_STAGE1 == val) {
		//do csi reset
	}
	return 0;
}

static struct v4l2_subdev_core_ops isp_subdev_core_ops = {
	.ioctl = spm_subdev_ioctl,
	.s_power = __fe_isp_s_power,
	.reset = spm_subdev_reset,
//#ifdef CONFIG_COMPAT
#if 0
	.compat_ioctl32 = spm_subdev_compat_ioctl32,
#endif
};

static struct v4l2_subdev_core_ops csi_subdev_core_ops = {
	.ioctl = spm_subdev_ioctl,
	//.s_power = csi_subdev_core_s_power,
	.reset = csi_subdev_core_reset,
//#ifdef CONFIG_COMPAT
#if 0
	.compat_ioctl32 = spm_subdev_compat_ioctl32,
#endif

};

static struct v4l2_subdev_video_ops formatter_subdev_video_ops = {
	.s_stream = fe_formatter_subdev_video_s_stream,
};

static struct v4l2_subdev_video_ops pipe_subdev_video_ops = {
	.s_stream = fe_pipe_subdev_video_s_stream,
};

static struct v4l2_subdev_video_ops rawdump_subdev_video_ops = {
	.s_stream = fe_rawdump_subdev_video_s_stream,
};

//static struct v4l2_subdev_video_ops csi_subdev_video_ops = {
//	.s_stream = csi_subdev_video_s_stream,
//};

static struct v4l2_subdev_ops fe_rawdump_subdev_ops = {
	.core = &isp_subdev_core_ops,
	.pad = &rawdump_subdev_pad_ops,
	.video = &rawdump_subdev_video_ops,
};

static struct v4l2_subdev_ops fe_offline_channel_subdev_ops = {
	.core = &isp_subdev_core_ops,
	.pad = &offline_channel_subdev_pad_ops,
};

static struct v4l2_subdev_ops fe_formatter_subdev_ops = {
	.core = &isp_subdev_core_ops,
	.pad = &formatter_subdev_pad_ops,
	.video = &formatter_subdev_video_ops,
};

static struct v4l2_subdev_ops fe_dwt_subdev_ops = {
	.core = &isp_subdev_core_ops,
	.pad = &dwt_subdev_pad_ops,
};

static struct v4l2_subdev_ops fe_pipe_subdev_ops = {
	.core = &isp_subdev_core_ops,
	.pad = &pipe_subdev_pad_ops,
	.video = &pipe_subdev_video_ops,
};

static struct v4l2_subdev_ops fe_hdr_combine_subdev_ops = {
	.core = &isp_subdev_core_ops,
	.pad = &hdr_combine_subdev_pad_ops,
};

static struct v4l2_subdev_ops csi_subdev_ops = {
	.core = &csi_subdev_core_ops,
	.pad = &csi_subdev_pad_ops,
	//.video = &csi_subdev_video_ops,
};

static void fe_pipe_subdev_release(struct spm_camera_subdev *sc_subdev)
{
	struct isp_context *isp_ctx = NULL;
	struct fe_pipe *pipe = container_of(sc_subdev, struct fe_pipe, sc_subdev);

	isp_ctx = spm_subdev_get_drvdata(sc_subdev);
	isp_ctx->pipes[pipe->idx] = NULL;
}

static void fe_rawdump_subdev_release(struct spm_camera_subdev *sc_subdev)
{
	struct isp_context *isp_ctx = NULL;
	struct fe_rawdump *rawdump = container_of(sc_subdev, struct fe_rawdump, sc_subdev);
	isp_ctx = spm_subdev_get_drvdata(sc_subdev);
	isp_ctx->rawdumps[rawdump->idx] = NULL;
}

static void fe_formatter_subdev_release(struct spm_camera_subdev *sc_subdev)
{
	struct isp_context *isp_ctx = NULL;
	struct fe_formatter *formatter = container_of(sc_subdev, struct fe_formatter, sc_subdev);

	isp_ctx = spm_subdev_get_drvdata(sc_subdev);
	isp_ctx->formatters[formatter->idx] = NULL;
}

static int fe_isp_media_link_setup(struct media_entity *entity,
				   const struct media_pad *local,
				   const struct media_pad *remote, u32 flags)
{
	struct spm_camera_subdev *sc_subdev = media_entity_to_sc_subdev(entity);
	struct csi *csi = media_entity_to_csi(entity);
	struct spm_camera_vnode *sc_vnode = media_entity_to_sc_vnode(remote->entity);
	int ret = 0, irq = 0;
	struct isp_context *isp_ctx = NULL;
	struct platform_device *pdev = NULL;
	struct device *dev = NULL;

	BUG_ON(!sc_subdev);
	isp_ctx = spm_subdev_get_drvdata(sc_subdev);
	pdev = isp_ctx->pdev;
	dev = &pdev->dev;
	if (sc_vnode) {
		if (flags & MEDIA_LNK_FL_ENABLED) {
			if (!csi) {
				spm_camera_block_set_base_addr(SC_BLOCK(sc_vnode), isp_ctx->base_addr + KY_ISP_DMA_OFFSET);
				cam_dbg("install baseaddr(0x%08lx) for vnode(%s)", isp_ctx->base_addr + KY_ISP_DMA_OFFSET, sc_vnode->name);
				if (!isp_ctx->dma_block) {
					isp_ctx->dma_block = SC_BLOCK(sc_vnode);
					irq = platform_get_irq_byname(pdev, "feisp-dma-irq");
					if (irq < 0) {
						cam_err("get irq resource for feisp-dma failed ret=%d", irq);
						return -1;
					}
					ret = devm_request_irq(dev, irq, fe_isp_dma_irq_handler, IRQF_SHARED, "feisp-dma", isp_ctx);
					if (ret) {
						cam_err("request irq for dma failed ret=%d", ret);
						return ret;
					}
				}
			}
			ret = spm_vdev_register_vnode_notify(sc_vnode, &(sc_subdev->vnode_nb));
			if (ret) {
				cam_err("%s(%s) register notifier to vnode(%s) failed", __func__, sc_subdev->name, sc_vnode->name);
				return ret;
			}
			cam_dbg("%s(%s) register notifier to vnode(%s)", __func__, sc_subdev->name, sc_vnode->name);
		} else {
			ret = spm_vdev_unregister_vnode_notify(sc_vnode, &(sc_subdev->vnode_nb));
			if (ret) {
				cam_err("%s(%s) unregister notifier to vnode(%s) failed", __func__, sc_subdev->name, sc_vnode->name);
				return ret;
			}
			cam_dbg("%s(%s) unregister notifier to vnode(%s)", __func__, sc_subdev->name, sc_vnode->name);
			if (sc_vnode->direction == KY_VNODE_DIR_OUT) {
				isp_ctx->dma_out_ctx[sc_vnode->idx].used_for_hdr = 0;
				isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 0;
			}
		}
	}
	return 0;
}

static int fe_rawdump_link_validate(struct media_link *link)
{
	int ret = 0;
	struct media_entity *me = NULL;
	struct fe_rawdump *rawdump = NULL;
	struct media_pad *remote_pad_in = NULL, *remote_pad_out = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *mpipe = NULL;
	struct device *dev = NULL;
	struct x1vi_platform_data *drvdata = NULL;
	struct csi *csi = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct fe_pipe *pipe = NULL;
	struct isp_context *isp_ctx = NULL;
	unsigned long flags = 0;

	me = link->sink->entity;
	rawdump = media_entity_to_rawdump(me);
	if (!rawdump)
		return -1;
	isp_ctx = spm_subdev_get_drvdata(&rawdump->sc_subdev);
	remote_pad_in = media_entity_remote_pad(&(rawdump->pads[PAD_IN]));
	remote_pad_out = media_entity_remote_pad(&(rawdump->pads[PAD_OUT]));
	dev = rawdump->sc_subdev.pcsd.sd.dev;
	drvdata = dev_get_drvdata(dev);

	if (!remote_pad_in)
		return -2;
	if (!remote_pad_out)
		return -3;
	csi = media_entity_to_csi(remote_pad_in->entity);
	pipe = media_entity_to_pipe(remote_pad_in->entity);
	sc_vnode = media_entity_to_sc_vnode(remote_pad_out->entity);
	if (!csi) {
		if ((rawdump->idx == 0 && !pipe) || rawdump->idx == 1)
			return -4;
		if (!pipe)
			return -7;
		remote_pad_in = media_entity_remote_pad(&(pipe->pads[PIPE_PAD_IN]));
		if (!remote_pad_in)
			return -8;
		csi = media_entity_to_csi(remote_pad_in->entity);
		if (!csi)
			return -9;
	}
	if (!sc_vnode)
		return -5;
	mpipe = media_entity_pipeline(me);
	if (!mpipe)
		return -6;
	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	sc_pipeline->max_width[0] = FE_ISP_MAX_WIDTH;
	sc_pipeline->max_height[0] = FE_ISP_MAX_HEIGHT;
	sc_pipeline->min_width[0] = FE_ISP_MIN_WIDTH;
	sc_pipeline->min_height[0] = FE_ISP_MIN_HEIGHT;
	ret = blocking_notifier_chain_register(&sc_pipeline->blocking_notify_chain, &rawdump->pipeline_notify_block);
	if (ret)
		return ret;
	if (drvdata->isp_firm)
		sc_pipeline->ispfirm_ops = drvdata->isp_firm->ispfirm_ops;
	else
		sc_pipeline->ispfirm_ops = NULL;
	sc_pipeline->sensor_ops = drvdata->sensor_ops;
	spin_lock_irqsave(&sc_pipeline->slock, flags);
	list_add(&isp_ctx->dma_out_ctx[sc_vnode->idx].frame_id.entry, &sc_pipeline->frame_id_list);
	spin_unlock_irqrestore(&sc_pipeline->slock, flags);
	if (pipe)
		isp_ctx->dma_out_ctx[sc_vnode->idx].used_for_hdr = 1;
	return 0;
}

static struct media_entity_operations rawdump_media_entity_ops = {
	.link_setup = fe_isp_media_link_setup,
	.link_validate = fe_rawdump_link_validate,
};

static int fe_offline_channel_link_validate(struct media_link *link)
{
	struct media_entity *me = link->sink->entity;
	struct fe_offline_channel *offline_channel = media_entity_to_offline_channel(me);
	struct media_pad *remote_pad_in = NULL, *remote_pad_p0out = NULL, *remote_pad_p1out = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct fe_pipe *pipe = NULL;

	BUG_ON(!offline_channel);
	remote_pad_in = media_entity_remote_pad(&(offline_channel->pads[OFFLINE_CH_PAD_IN]));
	remote_pad_p0out = media_entity_remote_pad(&(offline_channel->pads[OFFLINE_CH_PAD_P0OUT]));
	remote_pad_p1out = media_entity_remote_pad(&(offline_channel->pads[OFFLINE_CH_PAD_P1OUT]));

	if (!remote_pad_in)
		return -1;
	if (!remote_pad_p0out && !remote_pad_p1out)
		return -2;

	sc_vnode = media_entity_to_sc_vnode(remote_pad_in->entity);
	if (!sc_vnode)
		return -3;
	if (remote_pad_p0out) {
		pipe = media_entity_to_pipe(remote_pad_p0out->entity);
		if (!pipe)
			return -4;
	}
	if (remote_pad_p1out) {
		pipe = media_entity_to_pipe(remote_pad_p1out->entity);
		if (!pipe)
			return -5;
	}
	return 0;
}

static struct media_entity_operations offline_channel_media_entity_ops = {
	.link_setup = fe_isp_media_link_setup,
	.link_validate = fe_offline_channel_link_validate,
};

static int fe_formatter_link_validate(struct media_link *link)
{
	struct media_entity *me = link->sink->entity;
	struct fe_formatter *formatter = NULL;
	struct media_pad *remote_pad_in = NULL, *remote_pad_aout = NULL;
	struct media_pad *remote_pad_d1out = NULL, *remote_pad_d2out = NULL;
	struct media_pad *remote_pad_d3out = NULL, *remote_pad_d4out = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct v4l2_subdev *sd = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *mpipe = NULL;
	struct isp_context *isp_ctx = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	//struct fe_pipe *pipe = NULL;
	//struct spm_camera_sensor *sc_sensor = NULL;
	int valid_link = 0;
	unsigned int dma_start_cnt = 0;
	unsigned long flags = 0;

	formatter = media_entity_to_formatter(me);
	if (!formatter)
		return -100;
	isp_ctx = spm_subdev_get_drvdata(&formatter->sc_subdev);

	remote_pad_in = media_entity_remote_pad(&(formatter->pads[FMT_PAD_IN]));
	remote_pad_aout = media_entity_remote_pad(&(formatter->pads[FMT_PAD_AOUT]));
	remote_pad_d1out = media_entity_remote_pad(&(formatter->pads[FMT_PAD_D1OUT]));
	remote_pad_d2out = media_entity_remote_pad(&(formatter->pads[FMT_PAD_D2OUT]));
	remote_pad_d3out = media_entity_remote_pad(&(formatter->pads[FMT_PAD_D3OUT]));
	remote_pad_d4out = media_entity_remote_pad(&(formatter->pads[FMT_PAD_D4OUT]));

	if (remote_pad_in) {
		if (remote_pad_aout && !remote_pad_d1out
			&& !remote_pad_d2out && !remote_pad_d3out
			&& !remote_pad_d4out) {
			valid_link = 1;
			dma_start_cnt = 1;
		} else if (remote_pad_aout && remote_pad_d1out
			&& remote_pad_d2out && remote_pad_d3out
			&& remote_pad_d4out) {
			valid_link = 2;
			dma_start_cnt = DMA_START_CNT_WITH_DWT;
		}
	}
	if (!valid_link)
		return -1;
	sd = media_entity_to_v4l2_subdev(remote_pad_in->entity);
	if (SD_GRP(sd->grp_id) != FE_ISP)
		return -2;
	if (SD_SUB(sd->grp_id) != PIPE && SD_SUB(sd->grp_id) != HDR_COMBINE)
		return -3;

	sc_vnode = media_entity_to_sc_vnode(remote_pad_aout->entity);
	if (!sc_vnode)
		return -4;
	if (2 == valid_link) {
		if (!is_subdev(remote_pad_d1out->entity))
			return -5;
		sd = media_entity_to_v4l2_subdev(remote_pad_d1out->entity);
		if (SD_GRP(sd->grp_id) != FE_ISP)
			return -6;
		if (SD_SUB(sd->grp_id) != DWT0 && SD_SUB(sd->grp_id) != DWT1)
			return -7;
		if (!is_subdev(remote_pad_d2out->entity))
			return -8;
		sd = media_entity_to_v4l2_subdev(remote_pad_d2out->entity);
		if (SD_GRP(sd->grp_id) != FE_ISP)
			return -9;
		if (SD_SUB(sd->grp_id) != DWT0 && SD_SUB(sd->grp_id) != DWT1)
			return -10;
		if (!is_subdev(remote_pad_d3out->entity))
			return -11;
		sd = media_entity_to_v4l2_subdev(remote_pad_d3out->entity);
		if (SD_GRP(sd->grp_id) != FE_ISP)
			return -12;
		if (SD_SUB(sd->grp_id) != DWT0 && SD_SUB(sd->grp_id) != DWT1)
			return -13;
		if (!is_subdev(remote_pad_d4out->entity))
			return -14;
		sd = media_entity_to_v4l2_subdev(remote_pad_d4out->entity);
		if (SD_GRP(sd->grp_id) != FE_ISP)
			return -15;
		if (SD_SUB(sd->grp_id) != DWT0 && SD_SUB(sd->grp_id) != DWT1)
			return -16;
	}
	//pipe = media_entity_to_pipe(remote_pad_in->entity);
	//if (pipe) {
	//	remote_pad_in = media_entity_remote_pad(&pipe->pads[PIPE_PAD_IN]);
	//	if (remote_pad_in) {
	//		sc_sensor = media_entity_to_sc_sensor(remote_pad_in->entity);
	//		if (sc_sensor && sc_sensor->idx == 3 && valid_link == 1) //tpg
	//			isp_ctx->dma_out_ctx[sc_vnode->idx].trig_dma_reload = 1;
	//	}
	//}
	mpipe = media_entity_pipeline(me);
	if (!mpipe)
		return -17;
	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	mutex_lock(&sc_pipeline->mlock);
	if (!sc_pipeline->usr_data) {
		pipe_ctx = fe_pipeline_create_ctx(formatter->sc_subdev.pcsd.sd.dev);
		if (!pipe_ctx) {
			mutex_unlock(&sc_pipeline->mlock);
			return -18;
		}
		sc_pipeline->usr_data = pipe_ctx;
	} else {
		pipe_ctx = (struct isp_pipeline_context *)sc_pipeline->usr_data;
	}
	mutex_unlock(&sc_pipeline->mlock);
	spin_lock_irqsave(&sc_pipeline->slock, flags);
	list_add(&isp_ctx->dma_out_ctx[sc_vnode->idx].frame_id.entry, &sc_pipeline->frame_id_list);
	pipe_ctx->fmt_wdma_start_cnt[formatter->idx] = dma_start_cnt;
	spin_unlock_irqrestore(&sc_pipeline->slock, flags);
	return 0;
}

static struct media_entity_operations formatter_media_entity_ops = {
	.link_setup = fe_isp_media_link_setup,
	.link_validate = fe_formatter_link_validate,
};

static int fe_dwt_link_validate(struct media_link *link)
{
	struct media_entity *me = link->sink->entity;
	struct fe_dwt *dwt = media_entity_to_dwt(me);
	struct media_pad *remote_pad_in = NULL, *remote_pad_out = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct v4l2_subdev *sd = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *mpipe = NULL;
	struct isp_context *isp_ctx = NULL;
	unsigned long flags = 0;

	if (!dwt) {
		return -999;
	}
	isp_ctx = spm_subdev_get_drvdata(&dwt->sc_subdev);
	remote_pad_in = media_entity_remote_pad(&(dwt->pads[PAD_IN]));
	remote_pad_out = media_entity_remote_pad(&(dwt->pads[PAD_OUT]));

	if (!remote_pad_in)
		return -1;
	if (!remote_pad_out)
		return -2;

	sc_vnode = media_entity_to_sc_vnode(remote_pad_out->entity);
	if (!sc_vnode)
		return -3;
	if (!is_subdev(remote_pad_in->entity))
		return -4;
	sd = media_entity_to_v4l2_subdev(remote_pad_in->entity);
	if (SD_GRP(sd->grp_id) != FE_ISP)
		return -5;
	if (SD_SUB(sd->grp_id) != FORMATTER)
		return -6;
	mpipe = media_entity_pipeline(me);
	if (!mpipe)
		return -7;
	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	spin_lock_irqsave(&sc_pipeline->slock, flags);
	list_add(&isp_ctx->dma_out_ctx[sc_vnode->idx].frame_id.entry, &sc_pipeline->frame_id_list);
	spin_unlock_irqrestore(&sc_pipeline->slock, flags);

	return 0;
}

static struct media_entity_operations dwt_media_entity_ops = {
	.link_setup = fe_isp_media_link_setup,
	.link_validate = fe_dwt_link_validate,
};

static int fe_pipe_link_validate(struct media_link *link)
{
	struct media_entity *me = NULL;
	struct fe_pipe *pipe = NULL;
	struct media_pad *remote_pad_in = NULL, *remote_pad_out = NULL;
	int out_link_num = 0, i = 0, ret = 0;
	struct csi *csi = NULL;
	struct fe_offline_channel *offline_channel = NULL;
	struct fe_hdr_combine *hdr_combine = NULL;
	struct fe_formatter *formatter = NULL;
	struct fe_rawdump *rawdump = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *mpipe = NULL;
	struct device *dev = NULL;
	struct x1vi_platform_data *drvdata = NULL;
	struct isp_context *isp_ctx = NULL;
	unsigned long flags = 0;

	me = link->sink->entity;
	pipe = media_entity_to_pipe(me);
	if (!pipe)
		return -100;
	isp_ctx = spm_subdev_get_drvdata(&pipe->sc_subdev);
	dev = pipe->sc_subdev.pcsd.sd.dev;
	drvdata = dev_get_drvdata(dev);
	remote_pad_in = media_entity_remote_pad(&(pipe->pads[PIPE_PAD_IN]));
	if (!remote_pad_in)
		return -1;
	csi = media_entity_to_csi(remote_pad_in->entity);
	offline_channel = media_entity_to_offline_channel(remote_pad_in->entity);
	if (!csi && !offline_channel)
		return -2;
	for (i = 1; i < PIPE_PAD_NUM; i++) {
		remote_pad_out = media_entity_remote_pad(&(pipe->pads[i]));
		if (remote_pad_out) {
			if (i == PIPE_PAD_HDROUT) {
				hdr_combine = media_entity_to_hdr_combine(remote_pad_out->entity);
				if (!hdr_combine)
					return -3;
			} else if (i == PIPE_PAD_RAWDUMP0OUT) {
				rawdump = media_entity_to_rawdump(remote_pad_out->entity);
				if (!rawdump)
					return -4;
				if (rawdump->idx != 0)
					return -5;
			} else {
				formatter = media_entity_to_formatter(remote_pad_out->entity);
				if (!formatter)
					return -6;
			}
			out_link_num++;
		}
	}
	if (out_link_num <= 0)
		return -7;
	if (pipe->idx == 0) {
		if (!hdr_combine && rawdump)
			return -8;
	} else if (hdr_combine && out_link_num > 1) {
		return -6;
	}
	mpipe = media_entity_pipeline(me);
	if (!mpipe) {
		return -7;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	sc_pipeline->max_width[0] = FE_ISP_MAX_WIDTH;
	sc_pipeline->max_height[0] = FE_ISP_MAX_HEIGHT;
	sc_pipeline->min_width[0] = FE_ISP_MIN_WIDTH;
	sc_pipeline->min_height[0] = FE_ISP_MIN_HEIGHT;
	ret = blocking_notifier_chain_register(&sc_pipeline->blocking_notify_chain, &pipe->pipeline_notify_block);
	if (ret)
		return ret;
	cam_dbg("%s register pipe(%d) notify block to pipeline blocking notify chain", __func__, pipe->idx);
	if (drvdata->isp_firm)
		sc_pipeline->ispfirm_ops = drvdata->isp_firm->ispfirm_ops;
	else
		sc_pipeline->ispfirm_ops = NULL;
	sc_pipeline->sensor_ops = drvdata->sensor_ops;
	if (hdr_combine) {
		if (pipe->idx == 0)
			sc_pipeline->id = MAKE_SC_PIPELINE_ID(PIPELINE_TYPE_HDR, pipe->idx);
	} else {
		sc_pipeline->id = MAKE_SC_PIPELINE_ID(PIPELINE_TYPE_SINGLE, pipe->idx);
	}
	spin_lock_irqsave(&sc_pipeline->slock, flags);
	list_add(&(isp_ctx->pipe_frame_id[pipe->idx].entry), &(sc_pipeline->frame_id_list));
	spin_unlock_irqrestore(&sc_pipeline->slock, flags);
	return 0;
}

static struct media_entity_operations pipe_media_entity_ops = {
	.link_validate = fe_pipe_link_validate,
};

static int fe_hdr_combine_link_validate(struct media_link *link)
{
	struct media_entity *me = link->sink->entity;
	struct fe_hdr_combine *hdr_combine = NULL;
	struct media_pad *remote_pad = NULL;
	int in_link_num = 0, out_link_num = 0, i = 0;
	struct fe_pipe *pipe = NULL;
	struct fe_formatter *formatter = NULL;

	hdr_combine = media_entity_to_hdr_combine(me);
	if (!hdr_combine) {
		return -1;
	}
	for (i = 0; i < HDR_COMBINE_PAD_NUM; i++) {
		remote_pad = media_entity_remote_pad(&(hdr_combine->pads[i]));
		if (remote_pad) {
			if (i <= HDR_PAD_P1IN) {
				pipe = media_entity_to_pipe(remote_pad->entity);
				if (pipe)
					in_link_num++;
			} else {
				formatter = media_entity_to_formatter(remote_pad->entity);
				if (formatter)
					out_link_num++;
			}
		}
	}
	if (in_link_num < 2)
		return -2;
	if (out_link_num < 1)
		return -3;

	return 0;
}

static struct media_entity_operations hdr_combine_media_entity_ops = {
	.link_validate = fe_hdr_combine_link_validate,
};

static int csi_link_validate(struct media_link *link)
{
	struct media_entity *me = link->sink->entity;
	struct csi *csi = media_entity_to_csi(me);
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *mpipe = NULL;
	int ret = 0;

	BUG_ON(!csi);
	mpipe = media_entity_pipeline(me);
	BUG_ON(!mpipe);
	sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	BUG_ON(!sc_pipeline);
	ret = blocking_notifier_chain_register(&sc_pipeline->blocking_notify_chain, &csi->pipeline_notify_block);
	if (ret)
		return -9;
	return 0;
}

static struct media_entity_operations csi_media_entity_ops = {
	.link_setup = fe_isp_media_link_setup,
	.link_validate = csi_link_validate,
};

static long fe_isp_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct v4l2_vi_port_cfg *v4l2_port_cfg = NULL;
	struct vi_port_cfg *port_cfg = NULL;
	struct v4l2_vi_input_interface *input_intf = NULL;
	struct device *dev = sd->dev;
	struct isp_context *isp_ctx = v4l2_get_subdevdata(sd);
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct v4l2_vi_dbg_reg *dbg_reg = NULL;
	struct entity_usrdata usrdata;
	struct media_entity *me = NULL;
	struct spm_camera_sensor *sc_sensor = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(&sd->entity);
	unsigned int reg_val = 0, offset = 0, isp_fatal_error = 0;
	unsigned int *pipe_status = NULL;
	int ret = 0, p0_overrun = 0, p1_overrun = 0;
	long l_ret = 0;

	switch (cmd) {
	case VIDIOC_G_PIPE_STATUS:
		pipe_status = (unsigned int *)arg;
		*pipe_status = 0;
		if (!pipe) {
			cam_warn("%s(VIDIOC_G_PIPE_STATUS): pipe was null", __func__);
			return 0;
		}
		sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
		BUG_ON(!sc_pipeline);
		isp_fatal_error = isp_ctx->isp_fatal_error;
		if (isp_fatal_error & ISP_FATAL_ERR_PIPE0_OVERRUN)
			p0_overrun = 1;
		if (isp_fatal_error & ISP_FATAL_ERR_PIPE1_OVERRUN)
			p1_overrun = 1;
		if (isp_fatal_error & ISP_FATAL_ERR_DMA_OVERLAP) {
			cam_err("fatal error: dma overlap");
			*pipe_status = 1;
		}
		if (PIPELINE_TYPE_SINGLE == PIPELINE_TYPE(sc_pipeline->id)) {
			if ((PIPELINE_ID(sc_pipeline->id) == 0 && p0_overrun)
				|| (PIPELINE_ID(sc_pipeline->id) == 1 && p1_overrun))
				*pipe_status = 1;
		} else if (p0_overrun || p1_overrun) {
			*pipe_status = 1;
		}
		return 0;
		break;
	case VIDIOC_S_PORT_CFG:
		BUG_ON(!pipe);
		sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
		BUG_ON(!sc_pipeline);
		v4l2_port_cfg = (struct v4l2_vi_port_cfg *)arg;
		usrdata.entity_id = v4l2_port_cfg->port_entity_id;
		usrdata.usr_data = NULL;
		ret = blocking_notifier_call_chain(&sc_pipeline->blocking_notify_chain,
						   PIPELINE_ACTION_GET_ENTITY_USRDATA,
						   &usrdata);
		if (ret != NOTIFY_STOP) {
			cam_err("%s: get entity(%d) usrdata fail.", __func__, usrdata.entity_id);
			return -1;
		}
		if (!usrdata.usr_data) {
			port_cfg = devm_kzalloc(dev, sizeof(*port_cfg), GFP_KERNEL);
			if (!port_cfg) {
				cam_err("%s: no mem.", __func__);
				return -ENOMEM;
			}
			usrdata.usr_data = port_cfg;
			blocking_notifier_call_chain(&sc_pipeline->blocking_notify_chain,
							PIPELINE_ACTION_SET_ENTITY_USRDATA,
							&usrdata);
		} else {
			port_cfg = (struct vi_port_cfg *)usrdata.usr_data;
		}
		port_cfg->w_fifo_ctrl.offset = v4l2_port_cfg->offset;
		port_cfg->w_fifo_ctrl.depth = v4l2_port_cfg->depth;
		port_cfg->w_fifo_ctrl.weight = v4l2_port_cfg->weight;
		port_cfg->w_fifo_ctrl.div_mode = v4l2_port_cfg->div_mode;
		port_cfg->usage = v4l2_port_cfg->usage;
		return 0;
		break;
	case VIDIOC_CFG_INPUT_INTF:
		BUG_ON(!pipe);
		sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
		BUG_ON(!sc_pipeline);
		input_intf = (struct v4l2_vi_input_interface *)arg;
		me = spm_mlink_find_sensor(&(sd->entity));
		sc_pipeline->is_slice_mode = 0;
		if (input_intf->type == VI_INPUT_INTERFACE_MIPI) {
			if (!me) {
				cam_err("config vi input interface(mipi), but no sensor entity founded");
				return -1;
			}
			sc_sensor = (struct spm_camera_sensor *)me;
			if (input_intf->ccic_idx >= CCIC_MAX_CNT) {
				cam_err("invalid ccic idx(%u)", input_intf->ccic_idx);
				return -EINVAL;
			}
			isp_ctx->ccic[input_intf->ccic_idx].sc_sensor = sc_sensor;
		} else {
			if (me) {
				cam_err("config vi input interface(offline), but sensor entity was founded");
				return -1;
			}
			if (input_intf->type == VI_INPUT_INTERFACE_OFFLINE_SLICE) {
				sc_pipeline->is_slice_mode = 1;
			}
		}
		return 0;
		break;
	case VIDIOC_S_BANDWIDTH:
		return ret;
		break;
	case VIDIOC_DBG_REG_READ:
		dbg_reg = (struct v4l2_vi_dbg_reg *)arg;
		//offset = dbg_reg->addr - 0xa3430000;
		offset = dbg_reg->addr;
		dbg_reg->value = read32(isp_ctx->base_addr + offset);
		return 0;
		break;
	case VIDIOC_DBG_REG_WRITE:
		dbg_reg = (struct v4l2_vi_dbg_reg *)arg;
		//offset = dbg_reg->addr - 0xa3430000;
		offset = dbg_reg->addr;
		if (dbg_reg->mask != 0xffffffff) {
			reg_val = read32(isp_ctx->base_addr + offset);
			reg_val &= ~(dbg_reg->mask);
			dbg_reg->value &= dbg_reg->mask;
			reg_val |= dbg_reg->value;
		} else
			reg_val = dbg_reg->value;
		write32(isp_ctx->base_addr + offset, reg_val);
		return 0;
		break;
	case VIDIOC_GLOBAL_RESET:
		l_ret = fe_isp_global_reset(isp_ctx);
		if (l_ret == 0) {
			cam_err("%s global reset timeout", __func__);
			return -ETIME;
		} else if (l_ret < 0) {
			cam_err("%s global reset is interrupted by user app", __func__);
			return -1;
		} else {
			cam_dbg("%s global reset done", __func__);
		}
		return 0;
		break;
	case VIDIOC_FLUSH_BUFFERS:
		BUG_ON(!pipe);
		sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
		BUG_ON(!sc_pipeline);
		if (!sc_pipeline->is_slice_mode) {
			fe_isp_flush_pipeline_buffers(isp_ctx, sc_pipeline);
		}
		return 0;
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return -ENOIOCTLCMD;
}

static void fe_isp_subdev_notify(struct spm_camera_subdev *sc_subdev,
				 unsigned int notification, void *arg)
{
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_entity *me = &sc_subdev->pcsd.sd.entity;
	struct x1vi_platform_data *drvdata = NULL;
	struct media_pipeline *mpipe = media_entity_pipeline(me);

	if (mpipe) {
		sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
	}
	drvdata = dev_get_drvdata(sc_subdev->pcsd.sd.dev);
	switch (notification) {
	case PLAT_SD_NOTIFY_REGISTER_ISPFIRM:
		x1vi_register_isp_firmware((struct isp_firm *)arg);
		if (sc_pipeline && drvdata && drvdata->isp_firm)
			sc_pipeline->ispfirm_ops = drvdata->isp_firm->ispfirm_ops;
		break;
	case PLAT_SD_NOTIFY_REGISTER_SENSOR_OPS:
		x1vi_register_sensor_ops((struct spm_camera_sensor_ops *)arg);
		if (sc_pipeline)
			sc_pipeline->sensor_ops = (struct spm_camera_sensor_ops *)arg;
		break;
	default:
		break;
	}
}

struct fe_pipe *fe_pipe_create(unsigned int grp_id, void *isp_ctx)
{
	char name[KY_VI_ENTITY_NAME_LEN];
	struct fe_pipe *pipe = NULL;
	struct device *dev = NULL;
	struct platform_device *pdev = NULL;
	int ret = 0;
	struct isp_context *isp_context = (struct isp_context *)isp_ctx;
	int i = 0, irq = 0;

	if (!isp_ctx) {
		pr_err("%s invalid arguments.", __func__);
		return NULL;
	}
	pdev = isp_context->pdev;
	dev = &isp_context->pdev->dev;
	if (SD_GRP(grp_id) != FE_ISP || SD_SUB(grp_id) != PIPE) {
		cam_err("%s invalid grp_id(%u).", __func__, grp_id);
		return NULL;
	}
	if (SD_IDX(grp_id) >= PIPE_NUM) {
		cam_err("%s id(%d) is greater than %d.", __func__, SD_IDX(grp_id), PIPE_NUM - 1);
		return NULL;
	}
	if (isp_context->pipes[SD_IDX(grp_id)]) {
		cam_err("%s pipe%u had been already created before.",
			__func__, SD_IDX(grp_id));
		return NULL;
	}
	pipe = devm_kzalloc(dev, sizeof(*pipe), GFP_KERNEL);
	if (!pipe) {
		cam_err("%s not enough mem.", __func__);
		return NULL;
	}

	pipe->idx = SD_IDX(grp_id);
	pipe->pads[0].flags = MEDIA_PAD_FL_SINK;
	for (i = 1; i < PIPE_PAD_NUM; i++) {
		pipe->pads[i].flags = MEDIA_PAD_FL_SOURCE;
	}
	pipe->sc_subdev.pcsd.sd.entity.ops = &pipe_media_entity_ops;

	snprintf(name, KY_VI_ENTITY_NAME_LEN, "pipe%d", pipe->idx);
	ret = spm_subdev_init(grp_id, name, 0, &fe_pipe_subdev_ops,
			      PIPE_PAD_NUM, pipe->pads, isp_ctx, &pipe->sc_subdev);
	if (ret) {
		cam_err("%s spm_subdev_init fail ret=%d.", __func__, ret);
		goto pipe_sc_subdev_init_fail;
	}
	spm_camera_block_set_base_addr(&pipe->sc_subdev.sc_block, isp_context->base_addr + KY_ISP_TOP0_OFFSET + pipe->idx * KY_PIPE_OFFSET);
	pipe->sc_subdev.ioctl = fe_isp_subdev_ioctl;
	pipe->sc_subdev.release = fe_pipe_subdev_release;
	pipe->sc_subdev.notify = fe_isp_subdev_notify;
	pipe->sc_subdev.pcsd.sd.dev = dev;
	pipe->pipeline_notify_block.notifier_call = fe_isp_pipeline_notifier_handler;
	pipe->pipeline_notify_block.priority = SC_PIPE_NOTIFY_PRIO_NORMAL;
	init_completion(&(pipe->close_done));
	init_completion(&(pipe->sde_sof));
	isp_context->pipes[pipe->idx] = pipe;
	if (pipe->idx == (PIPE_NUM - 1)) {
		irq = platform_get_irq_byname(pdev, "feisp-irq");
		if (irq < 0) {
			cam_err("get irq resource for feisp failed ret=%d", irq);
			goto pipe_sc_subdev_init_fail;
		}
		ret = devm_request_irq(dev, irq, fe_isp_irq_handler, IRQF_SHARED, "feisp", isp_ctx);
		if (ret) {
			cam_err("request irq for isp failed ret=%d", ret);
			goto pipe_sc_subdev_init_fail;
		}
	}
	return pipe;
pipe_sc_subdev_init_fail:
	devm_kfree(dev, pipe);
	return NULL;
}

struct fe_rawdump *fe_rawdump_create(unsigned int grp_id, void *isp_ctx)
{
	char name[KY_VI_ENTITY_NAME_LEN];
	struct fe_rawdump *rawdump = NULL;
	struct device *dev = NULL;
	int ret = 0;
	struct isp_context *isp_context = (struct isp_context *)isp_ctx;

	if (!isp_ctx) {
		pr_err("%s invalid arguments.", __func__);
		return NULL;
	}
	dev = &isp_context->pdev->dev;
	if (SD_GRP(grp_id) != FE_ISP || SD_SUB(grp_id) != RAWDUMP) {
		cam_err("%s invalid grp_id(%u).", __func__, grp_id);
		return NULL;
	}
	if (SD_IDX(grp_id) >= RAWDUMP_NUM) {
		cam_err("%s id(%d) is greater than %d.", __func__, SD_IDX(grp_id), RAWDUMP_NUM - 1);
		return NULL;
	}
	if (isp_context->rawdumps[SD_IDX(grp_id)]) {
		cam_err("%s rawdump with the same id(%u) had been already created before.",
				__func__, SD_IDX(grp_id));
		return NULL;
	}

	rawdump = devm_kzalloc(dev, sizeof(*rawdump), GFP_KERNEL);
	if (!rawdump) {
		cam_err("%s not enough mem.", __func__);
		return NULL;
	}

	rawdump->idx = SD_IDX(grp_id);
	rawdump->pads[0].flags = MEDIA_PAD_FL_SINK;
	rawdump->pads[1].flags = MEDIA_PAD_FL_SOURCE;
	rawdump->sc_subdev.pcsd.sd.entity.ops = &rawdump_media_entity_ops;

	snprintf(name, KY_VI_ENTITY_NAME_LEN, "rawdump%d", rawdump->idx);
	ret = spm_subdev_init(grp_id, name, 0, &fe_rawdump_subdev_ops,
			      RAWDUMP_PAD_NUM, rawdump->pads, isp_ctx,
			      &rawdump->sc_subdev);
	if (ret) {
		cam_err("%s spm_subdev_init fail ret=%d.", __func__, ret);
		goto rawdump_sc_subdev_init_fail;
	}
	spm_camera_block_set_base_addr(&rawdump->sc_subdev.sc_block,
				       isp_context->base_addr +
				       KY_ISP_TOP0_OFFSET);
	rawdump->sc_subdev.ioctl = fe_isp_subdev_ioctl;
	rawdump->sc_subdev.release = fe_rawdump_subdev_release;
	rawdump->sc_subdev.vnode_nb.notifier_call = fe_isp_vnode_notifier_handler;
	rawdump->sc_subdev.pcsd.sd.dev = dev;
	rawdump->pipeline_notify_block.notifier_call = fe_isp_pipeline_notifier_handler;
	rawdump->pipeline_notify_block.priority = SC_PIPE_NOTIFY_PRIO_NORMAL;
	atomic_set(&rawdump->close_done, 0);
	isp_context->rawdumps[rawdump->idx] = rawdump;
	return rawdump;
rawdump_sc_subdev_init_fail:
	devm_kfree(dev, rawdump);
	return NULL;
}

struct fe_offline_channel *fe_offline_channel_create(unsigned int grp_id, void *isp_ctx)
{
	char name[KY_VI_ENTITY_NAME_LEN];
	struct fe_offline_channel *offline_channel = NULL;
	struct device *dev = NULL;
	struct isp_context *isp_context = isp_ctx;
	int ret = 0;

	if (!isp_ctx) {
		pr_err("%s invalid arguments.", __func__);
		return NULL;
	}
	dev = &isp_context->pdev->dev;
	if (SD_GRP(grp_id) != FE_ISP || SD_SUB(grp_id) != OFFLINE_CHANNEL) {
		cam_err("%s invalid grp_id(%u).", __func__, grp_id);
		return NULL;
	}
	if (SD_IDX(grp_id) >= OFFLINE_CH_NUM) {
		cam_err("%s id(%d) is greater than %d.", __func__, SD_IDX(grp_id),
			OFFLINE_CH_NUM - 1);
		return NULL;
	}

	offline_channel = devm_kzalloc(dev, sizeof(*offline_channel), GFP_KERNEL);
	if (!offline_channel) {
		cam_err("%s not enough mem.", __func__);
		return NULL;
	}

	offline_channel->idx = SD_IDX(grp_id);
	offline_channel->pads[0].flags = MEDIA_PAD_FL_SINK;
	offline_channel->pads[1].flags = MEDIA_PAD_FL_SOURCE;
	offline_channel->pads[2].flags = MEDIA_PAD_FL_SOURCE;
	offline_channel->sc_subdev.pcsd.sd.entity.ops = &offline_channel_media_entity_ops;
	snprintf(name, KY_VI_ENTITY_NAME_LEN, "offline_channel%d", offline_channel->idx);
	ret = spm_subdev_init(grp_id, name, 0, &fe_offline_channel_subdev_ops,
			      OFFLINE_CH_PAD_NUM, offline_channel->pads, isp_ctx, &offline_channel->sc_subdev);
	if (ret) {
		cam_err("%s spm_subdev_init fail ret=%d.", __func__, ret);
		goto offline_channel_sc_subdev_init_fail;
	}
	spm_camera_block_set_base_addr(SC_BLOCK(offline_channel), isp_context->base_addr + KY_ISP_TOP0_OFFSET);
	offline_channel->sc_subdev.ioctl = fe_isp_subdev_ioctl;
	offline_channel->sc_subdev.vnode_nb.notifier_call = fe_isp_vnode_notifier_handler;
	offline_channel->sc_subdev.pcsd.sd.dev = dev;
	return offline_channel;
offline_channel_sc_subdev_init_fail:
	devm_kfree(dev, offline_channel);
	return NULL;
}

struct fe_formatter *fe_formatter_create(unsigned int grp_id, void *isp_ctx)
{
	char name[KY_VI_ENTITY_NAME_LEN];
	struct fe_formatter *formatter = NULL;
	struct device *dev = NULL;
	struct isp_context *isp_context = isp_ctx;
	int ret = 0;

	if (!isp_ctx) {
		cam_err("%s invalid arguments.", __func__);
		return NULL;
	}
	dev = &isp_context->pdev->dev;
	if (SD_GRP(grp_id) != FE_ISP || SD_SUB(grp_id) != FORMATTER) {
		cam_err("%s invalid grp_id(%u).", __func__, grp_id);
		return NULL;
	}
	if (SD_IDX(grp_id) >= FORMATTER_NUM) {
		cam_err("%s id(%d) is greater than %d.", __func__, SD_IDX(grp_id),
			FORMATTER_NUM - 1);
		return NULL;
	}

	formatter = devm_kzalloc(dev, sizeof(*formatter), GFP_KERNEL);
	if (!formatter) {
		cam_err("%s not enough mem.", __func__);
		return NULL;
	}

	formatter->idx = SD_IDX(grp_id);
	atomic_set(&formatter->dwt_refcnt, 0);
	formatter->pads[0].flags = MEDIA_PAD_FL_SINK;
	formatter->pads[1].flags = MEDIA_PAD_FL_SOURCE;
	formatter->pads[2].flags = MEDIA_PAD_FL_SOURCE;
	formatter->pads[3].flags = MEDIA_PAD_FL_SOURCE;
	formatter->pads[4].flags = MEDIA_PAD_FL_SOURCE;
	formatter->pads[5].flags = MEDIA_PAD_FL_SOURCE;
	snprintf(name, KY_VI_ENTITY_NAME_LEN, "formatter%d", formatter->idx);
	ret =
	    spm_subdev_init(grp_id, name, 0, &fe_formatter_subdev_ops,
			    FORMATTER_PAD_NUM, formatter->pads, isp_ctx,
			    &formatter->sc_subdev);
	if (ret) {
		cam_err("%s spm_subdev_init fail ret=%d.", __func__, ret);
		goto formatter_sc_subdev_init_fail;
	}
	formatter->sc_subdev.pcsd.sd.entity.ops = &formatter_media_entity_ops;
	spm_camera_block_set_base_addr(SC_BLOCK(formatter), isp_context->base_addr + KY_POSTPIPE_OFFSET);
	formatter->sc_subdev.ioctl = fe_isp_subdev_ioctl;
	formatter->sc_subdev.release = fe_formatter_subdev_release;
	formatter->sc_subdev.vnode_nb.notifier_call = fe_isp_vnode_notifier_handler;
	formatter->sc_subdev.pcsd.sd.dev = dev;
	isp_context->formatters[formatter->idx] = formatter;
	return formatter;
formatter_sc_subdev_init_fail:
	devm_kfree(dev, formatter);
	return NULL;
}

struct fe_dwt *fe_dwt_create(unsigned int grp_id, void *isp_ctx)
{
	char name[KY_VI_ENTITY_NAME_LEN];
	struct fe_dwt *dwt = NULL;
	struct device *dev = NULL;
	struct isp_context *isp_context = isp_ctx;
	int ret = 0;

	if (!isp_ctx) {
		cam_err("%s invalid arguments.", __func__);
		return NULL;
	}
	dev = &isp_context->pdev->dev;
	if (SD_GRP(grp_id) != FE_ISP || !(SD_SUB(grp_id) == DWT0 || SD_SUB(grp_id) == DWT1)) {
		cam_err("%s invalid grp_id(%u).", __func__, grp_id);
		return NULL;
	}
	if (SD_IDX(grp_id) > DWT_LAYER_NUM) {
		cam_err("%s layer id(%d) is greater than %d.", __func__, SD_IDX(grp_id), DWT_LAYER_NUM);
		return NULL;
	}

	dwt = devm_kzalloc(dev, sizeof(*dwt), GFP_KERNEL);
	if (!dwt) {
		cam_err("%s not enough mem.", __func__);
		return NULL;
	}

	dwt->idx = SD_SUB(grp_id) - DWT0;
	dwt->layer_idx = SD_IDX(grp_id);
	dwt->pads[0].flags = MEDIA_PAD_FL_SINK;
	dwt->pads[1].flags = MEDIA_PAD_FL_SOURCE;
	snprintf(name, KY_VI_ENTITY_NAME_LEN, "dwt%d_layer%d", dwt->idx, dwt->layer_idx);
	ret = spm_subdev_init(grp_id, name, 0, &fe_dwt_subdev_ops, DWT_PAD_NUM, dwt->pads, isp_ctx, &dwt->sc_subdev);
	if (ret) {
		cam_err("%s spm_subdev_init fail ret=%d.", __func__, ret);
		goto dwt_sc_subdev_init_fail;
	}
	dwt->sc_subdev.pcsd.sd.entity.ops = &dwt_media_entity_ops;
	spm_camera_block_set_base_addr(SC_BLOCK(dwt), isp_context->base_addr + KY_POSTPIPE_OFFSET);
	dwt->sc_subdev.ioctl = fe_isp_subdev_ioctl;
	dwt->sc_subdev.vnode_nb.notifier_call = fe_isp_vnode_notifier_handler;
	dwt->sc_subdev.pcsd.sd.dev = dev;
	return dwt;
dwt_sc_subdev_init_fail:
	devm_kfree(dev, dwt);
	return NULL;
}

struct fe_hdr_combine *fe_hdr_combine_create(unsigned int grp_id, void *isp_ctx)
{
	char name[KY_VI_ENTITY_NAME_LEN];
	struct fe_hdr_combine *hdr_combine = NULL;
	struct device *dev = NULL;
	struct isp_context *isp_context = isp_ctx;
	int ret = 0, i = 0;

	if (!isp_ctx) {
		cam_err("%s invalid arguments.", __func__);
		return NULL;
	}
	dev = &isp_context->pdev->dev;
	if (SD_GRP(grp_id) != FE_ISP || SD_SUB(grp_id) != HDR_COMBINE) {
		cam_err("%s invalid grp_id(%u).", __func__, grp_id);
		return NULL;
	}
	if (SD_IDX(grp_id) >= HDR_COMBINE_NUM) {
		cam_err("%s id(%d) is greater than %d.", __func__, SD_IDX(grp_id), HDR_COMBINE_NUM - 1);
		return NULL;
	}

	hdr_combine = devm_kzalloc(dev, sizeof(*hdr_combine), GFP_KERNEL);
	if (!hdr_combine) {
		cam_err("%s not enough mem.", __func__);
		return NULL;
	}

	for (i = HDR_PAD_P0IN; i <= HDR_PAD_P1IN; i++) {
		hdr_combine->pads[i].flags = MEDIA_PAD_FL_SINK;
	}
	for (i = HDR_PAD_F0OUT; i < HDR_COMBINE_PAD_NUM; i++) {
		hdr_combine->pads[i].flags = MEDIA_PAD_FL_SOURCE;
	}
	strlcpy(name, "hdr_combine", KY_VI_ENTITY_NAME_LEN);
	ret = spm_subdev_init(grp_id, name, 0, &fe_hdr_combine_subdev_ops, HDR_COMBINE_PAD_NUM, hdr_combine->pads, isp_ctx, &hdr_combine->sc_subdev);
	if (ret) {
		cam_err("%s spm_subdev_init fail ret=%d.", __func__, ret);
		goto hdr_combine_sc_subdev_init_fail;
	}
	hdr_combine->sc_subdev.pcsd.sd.entity.ops = &hdr_combine_media_entity_ops;
	spm_camera_block_set_base_addr(SC_BLOCK(hdr_combine), isp_context->base_addr + KY_ISP_TOP0_OFFSET);
	hdr_combine->sc_subdev.ioctl = fe_isp_subdev_ioctl;
	hdr_combine->sc_subdev.pcsd.sd.dev = dev;
	return hdr_combine;
hdr_combine_sc_subdev_init_fail:
	devm_kfree(dev, hdr_combine);
	return NULL;
}

struct csi *csi_create(unsigned int grp_id, void *isp_ctx)
{
	char name[KY_VI_ENTITY_NAME_LEN];
	struct csi *csi = NULL;
	struct device *dev = NULL;
	struct isp_context *isp_context = isp_ctx;
	int i = 0, ret = 0;

	if (!isp_ctx) {
		cam_err("%s invalid arguments.", __func__);
		return NULL;
	}
	dev = &isp_context->pdev->dev;
	if (SD_GRP(grp_id) != MIPI || (SD_SUB(grp_id) != CSI_MAIN && SD_SUB(grp_id) != CSI_VCDT)) {
		cam_err("%s invalid grp_id(%u).", __func__, grp_id);
		return NULL;
	}
	if (SD_IDX(grp_id) >= CSI_NUM) {
		cam_err("%s id(%d) is greater than %d.", __func__, SD_IDX(grp_id), CSI_NUM - 1);
		return NULL;
	}

	csi = devm_kzalloc(dev, sizeof(*csi), GFP_KERNEL);
	if (!csi) {
		cam_err("%s not enough mem.", __func__);
		return NULL;
	}

	csi->pads[0].flags = MEDIA_PAD_FL_SINK;
	for (i = CSI_PAD_RAWDUMP0; i < CSI_PAD_NUM; i++) {
		csi->pads[i].flags = MEDIA_PAD_FL_SOURCE;
	}
	csi->idx = SD_IDX(grp_id);
	if (SD_SUB(grp_id) == CSI_MAIN) {
		csi->channel_type = CSI_MAIN;
		snprintf(name, KY_VI_ENTITY_NAME_LEN, "csi%d_main", csi->idx);
	} else {
		csi->channel_type = CSI_VCDT;
		snprintf(name, KY_VI_ENTITY_NAME_LEN, "csi%d_vcdt", csi->idx);
	}
	csi->sc_subdev.pcsd.sd.entity.ops = &csi_media_entity_ops;
	ret = spm_subdev_init(grp_id, name, 0, &csi_subdev_ops,
			      CSI_PAD_NUM, csi->pads, isp_ctx, &csi->sc_subdev);
	if (ret) {
		cam_err("%s spm_subdev_init fail ret=%d.", __func__, ret);
		goto csi_sc_subdev_init_fail;
	}
	csi->sc_subdev.ioctl = fe_isp_subdev_ioctl;
	csi->sc_subdev.vnode_nb.notifier_call = fe_isp_vnode_notifier_handler;
	csi->sc_subdev.pcsd.sd.dev = dev;
	csi->pipeline_notify_block.notifier_call = fe_isp_pipeline_notifier_handler;
	csi->pipeline_notify_block.priority = SC_PIPE_NOTIFY_PRIO_NORMAL;
	return csi;
csi_sc_subdev_init_fail:
	devm_kfree(dev, csi);
	return NULL;
}

static struct frame_id *fe_isp_frame_id(struct list_head *frame_id_list)
{
	struct frame_id *pos = NULL, *max_frame_id = NULL;
	max_frame_id = list_first_entry_or_null(frame_id_list, struct frame_id, entry);
	if (max_frame_id == NULL)
		return NULL;
	list_for_each_entry(pos, frame_id_list, entry) {
		if (pos->id >= max_frame_id->id)
			max_frame_id = pos;
	}

	return max_frame_id;
}

static void fe_isp_reset_frame_id(struct spm_camera_pipeline *sc_pipeline)
{
	struct frame_id *pos = NULL;
	unsigned long flags = 0;

	if (!sc_pipeline)
		return;
	spin_lock_irqsave(&sc_pipeline->slock, flags);
	list_for_each_entry(pos, &sc_pipeline->frame_id_list, entry) {
		pos->id = 0;
	}
	spin_unlock_irqrestore(&sc_pipeline->slock, flags);
}

#define PIPE_ERR_SHIFT	(21)
#define PIPE_ERR(a)	((a) << PIPE_ERR_SHIFT)
#define DMA_OVERLAP_CNT_MAX	(5)
static irqreturn_t fe_isp_dma_irq_handler(int irq, void *dev_id)
{
	unsigned int status1 = 0, status2 = 0, irq_status = 0, isp_fatal_error = 0, tmp = 0;
	unsigned int tmp_status1 = 0, tmp_status2 = 0;
#ifdef CONFIG_KY_X1_VI_IOMMU
	unsigned int mmu_irq_status = 0;
	struct media_pipeline *pipe = NULL;
#endif
	int irq_src = 0, is_mix_hdr = 0, i = 0, p0_overrun = 0, p1_overrun = 0;
	unsigned int *hw_err_code = NULL;
	struct isp_context *isp_ctx = (struct isp_context *)dev_id;
	struct isp_dma_context *dma_ctx = NULL, *dma_ctx_pos = NULL;
	struct media_entity *me = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL, *sc_pipelines[2] = {NULL, NULL};
	struct media_pipeline *mpipe = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct fe_rawdump *rawdump = NULL;
	struct media_pad *remote_pad = NULL;
	struct frame_id *frame_id = NULL;
	uint32_t frame_idx = 0;
	struct spm_camera_vbuffer *pos = NULL;
	struct isp_dma_work_struct *isp_dma_work = NULL;
	struct dma_irq_data irq_data = { 0 };
	struct x1vi_platform_data *drvdata = platform_get_drvdata(isp_ctx->pdev);
	struct spm_camera_ispfirm_ops *ispfirm_ops = NULL;
	unsigned long tasklet_state[ISP_DMA_WORK_MAX_CNT];
	int ret = 0;
	static unsigned long print_jiffies = 0;

	if (!isp_ctx->dma_block)
		return IRQ_HANDLED;

#ifdef CONFIG_KY_X1_VI_IOMMU
	mmu_irq_status = isp_ctx->mmu_dev->ops->irq_status(isp_ctx->mmu_dev);
	if (mmu_irq_status & MMU_RD_TIMEOUT) {
		cam_err("isp iommu RD_Timeout_error_IRQ");
		isp_ctx->mmu_dev->ops->dump_channel_regs(isp_ctx->mmu_dev, 3);
	}
	if (mmu_irq_status & MMU_WR_TIMEOUT) {
		for (i = 0; i < PIPE_NUM; i++) {
			pipe = media_entity_pipeline(&isp_ctx->pipes[i]->sc_subdev.pcsd.sd.entity);
			if (pipe) {
				sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
				spin_lock(&sc_pipeline->slock);
				if (sc_pipeline->usr_data) {
					pipe_ctx = (struct isp_pipeline_context*)sc_pipeline->usr_data;
					pipe_ctx->mmu_tbu_reload = MMU_TBU_RELOAD_START;
				}
				spin_unlock(&sc_pipeline->slock);
			}
		}
		cam_err("isp iommu WR_Timeout_error_IRQ");
		isp_ctx->mmu_dev->ops->dump_channel_regs(isp_ctx->mmu_dev, 4);
	}
	for (i = 0; i < 16; i++) {
		if (mmu_irq_status & (0x1 << i))
			cam_err("isp iommu tbu%d/%d dma err", 2 * i, 2 * i + 1);
	}
#endif
	isp_fatal_error = isp_ctx->isp_fatal_error;
	if (isp_fatal_error & ISP_FATAL_ERR_PIPE0_OVERRUN)
		p0_overrun = 1;
	if (isp_fatal_error & ISP_FATAL_ERR_PIPE1_OVERRUN)
		p1_overrun = 1;

	if (drvdata && drvdata->isp_firm)
		ispfirm_ops = drvdata->isp_firm->ispfirm_ops;
	status1 = hw_dma_get_irq_status1(isp_ctx->dma_block);
	if (status1)
		hw_dma_clr_irq_status1(isp_ctx->dma_block, status1);
	status2 = hw_dma_get_irq_status2(isp_ctx->dma_block);
	if (status2)
		hw_dma_clr_irq_status2(isp_ctx->dma_block, status2);

	if (status2 & DMA_IRQ_OVERRUN) {
		vi_irq_print("dma overrun occured!");
	}
	if (status2 & DMA_IRQ_OVERLAP) {
		vi_irq_print("dma overlap occured!");
		if (isp_ctx->dma_overlap_cnt++ >= DMA_OVERLAP_CNT_MAX) {
			isp_ctx->dma_overlap_cnt = 0;
			isp_ctx->isp_fatal_error |= ISP_FATAL_ERR_DMA_OVERLAP;
		}
	} else {
		isp_ctx->dma_overlap_cnt = 0;
		isp_ctx->isp_fatal_error &= ~ISP_FATAL_ERR_DMA_OVERLAP;
	}
	//if (status1 || status2)
	//	cam_dbg("dma irq status1=0x%08x status2=0x%08x", status1, status2);
	for (irq_src = DMA_IRQ_SRC_WDMA_CH0; irq_src <= DMA_IRQ_SRC_WDMA_CH13; irq_src++) {
		dma_ctx = &(isp_ctx->dma_out_ctx[irq_src - DMA_IRQ_SRC_WDMA_CH0]);
		spin_lock(&dma_ctx->waitq_head.lock);
		dma_ctx->in_irq = 1;
		if (dma_ctx->in_streamoff) {
			dma_ctx->in_irq = 0;
			wake_up_locked(&dma_ctx->waitq_head);
			spin_unlock(&dma_ctx->waitq_head.lock);
			continue;
		}
		spin_unlock(&dma_ctx->waitq_head.lock);
		if (dma_ctx->vnode) {
			irq_status = hw_dma_irq_analyze(irq_src, status1, status2);
			me = &dma_ctx->vnode->vnode.entity;
			mpipe = media_entity_pipeline(me);
			if (mpipe)
				sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
			else
				sc_pipeline = NULL;
			if (sc_pipeline) {
				spin_lock(&sc_pipeline->slock);
				pipe_ctx = (struct isp_pipeline_context*)sc_pipeline->usr_data;
				if (pipe_ctx && (irq_status & DMA_IRQ_START) && dma_ctx->trig_dma_reload) {
					for (i = 0; i < FORMATTER_NUM; i++) {
						list_for_each_entry(dma_ctx_pos, &pipe_ctx->fmt_wdma_list[i], list_entry) {
							__hw_dma_set_irq_enable(DMA_IRQ_SRC_WDMA_CH0 + dma_ctx_pos->id,
										DMA_IRQ_START, 0, &tmp_status1, &tmp_status2);
						}
					}
					list_for_each_entry(dma_ctx_pos, &pipe_ctx->wdma_list, list_entry) {
						__hw_dma_set_irq_enable(DMA_IRQ_SRC_WDMA_CH0 + dma_ctx_pos->id,
								        DMA_IRQ_START, 0, &tmp_status1, &tmp_status2);
					}
					status1 |= tmp_status1;
					status2 |= tmp_status2;
					if (PIPELINE_ID(sc_pipeline->id) == 0)
						sc_pipelines[0] = sc_pipeline;
					else
						sc_pipelines[1] = sc_pipeline;
				}
				spin_unlock(&sc_pipeline->slock);
			}
		}
		spin_lock(&dma_ctx->waitq_head.lock);
		dma_ctx->in_irq = 0;
		wake_up_locked(&dma_ctx->waitq_head);
		spin_unlock(&dma_ctx->waitq_head.lock);
	}
	for (irq_src = DMA_IRQ_SRC_WDMA_CH0; irq_src <= DMA_IRQ_SRC_WDMA_CH13; irq_src++) {
		dma_ctx = &(isp_ctx->dma_out_ctx[irq_src - DMA_IRQ_SRC_WDMA_CH0]);
		spin_lock(&dma_ctx->waitq_head.lock);
		dma_ctx->in_irq = 1;
		if (dma_ctx->in_streamoff) {
			dma_ctx->in_irq = 0;
			wake_up_locked(&dma_ctx->waitq_head);
			spin_unlock(&dma_ctx->waitq_head.lock);
			continue;
		}
		spin_unlock(&dma_ctx->waitq_head.lock);
		if (dma_ctx->vnode) {
			if (dma_ctx->used_for_hdr)
				is_mix_hdr = 1;
			irq_status = hw_dma_irq_analyze(irq_src, status1, status2);
			me = &dma_ctx->vnode->vnode.entity;
			mpipe = media_entity_pipeline(me);
			if (mpipe)
				sc_pipeline = media_pipeline_to_sc_pipeline(mpipe);
			else
				sc_pipeline = NULL;
			if (sc_pipeline) {
				if (PIPELINE_TYPE_SINGLE == PIPELINE_TYPE(sc_pipeline->id)) {
					if ((PIPELINE_ID(sc_pipeline->id) == 0 && p0_overrun)
						|| (PIPELINE_ID(sc_pipeline->id) == 1 && p1_overrun))
						irq_status |= PIPE_ERR(1);
				} else if (p0_overrun || p1_overrun) {
					irq_status |= PIPE_ERR(1);
				}
			}
			remote_pad = media_entity_remote_pad(&dma_ctx->vnode->pad);
			BUG_ON(!remote_pad);
			rawdump = media_entity_to_rawdump(remote_pad->entity);
			if (irq_status) {
				if (((irq_status & DMA_IRQ_START) && sc_pipeline && sc_pipeline->is_online_mode)
					|| ((irq_status & DMA_IRQ_DONE) && sc_pipeline && !sc_pipeline->is_online_mode)) {
					spin_lock(&sc_pipeline->slock);
					dma_ctx->frame_id.id++;
					frame_id = fe_isp_frame_id(&sc_pipeline->frame_id_list);
					if (frame_id) {
						if (dma_ctx->frame_id.id < frame_id->id)
							dma_ctx->frame_id.id = frame_id->id;
					} else {
						cam_warn("%s:frame id list is null", __func__);
					}
					frame_idx = dma_ctx->frame_id.id - 1;
					spin_unlock(&sc_pipeline->slock);
					dma_ctx->vnode->total_frm++;
				}
				if (!dma_ctx->used_for_hdr) {
					tmp = irq_status & (~PIPE_ERR(1));
					spin_lock(&(dma_ctx->vnode->slock));
					list_for_each_entry(pos, &(dma_ctx->vnode->busy_list), list_entry) {
						if (!tmp)
							break;
						if (tmp & DMA_IRQ_ERR) {
							if (!(pos->flags & SC_BUF_FLAG_SOF_TOUCH)) {
								vi_irq_print("%s dma err(0x%08x) without sof, drop it", dma_ctx->vnode->name, tmp);
								tmp &= ~DMA_IRQ_ERR;
							} else if (!(pos->flags & SC_BUF_FLAG_HW_ERR)) {
								pos->flags |= SC_BUF_FLAG_HW_ERR;
								hw_err_code = (unsigned int*)(&pos->reserved[0]);
								*hw_err_code = irq_status;
								tmp &= ~DMA_IRQ_ERR;
								pos->timestamp_eof = ktime_get_boottime_ns();
							}
							atomic_dec(&dma_ctx->busy_cnt);
							wake_up_interruptible_all(&dma_ctx->waitq_eof);
						}
						if (tmp & DMA_IRQ_DONE) {
							if (!(pos->flags & SC_BUF_FLAG_SOF_TOUCH) && sc_pipeline && sc_pipeline->is_online_mode) {
								cam_dbg("%s dma done without sof, drop it", dma_ctx->vnode->name);
								tmp &= ~DMA_IRQ_DONE;
							} else if (!(pos->flags & SC_BUF_FLAG_DONE_TOUCH)) {
								if (sc_pipeline && !sc_pipeline->is_online_mode) {
									pos->vb2_v4l2_buf.sequence = frame_idx;
									pos->vb2_v4l2_buf.vb2_buf.timestamp = ktime_get_boottime_ns();
								}
								pos->timestamp_eof = ktime_get_boottime_ns();
								pos->flags |= SC_BUF_FLAG_DONE_TOUCH;
								tmp &= ~DMA_IRQ_DONE;
								if (irq_status & PIPE_ERR(1)) {
									hw_err_code = (unsigned int*)(&pos->reserved[0]);
									*hw_err_code = irq_status;
									pos->flags |= SC_BUF_FLAG_HW_ERR;
								}
							}
							atomic_dec(&dma_ctx->busy_cnt);
							wake_up_interruptible_all(&dma_ctx->waitq_eof);
						}
						if (tmp & DMA_IRQ_START) {
							if (pos->flags & SC_BUF_FLAG_SOF_TOUCH) {
								if (!(pos->flags & (SC_BUF_FLAG_DONE_TOUCH | SC_BUF_FLAG_HW_ERR | SC_BUF_FLAG_SW_ERR))
									&& !(pos->flags & SC_BUF_FLAG_FORCE_SHADOW)) {
									cam_warn("%s next sof arrived without dma done or err", dma_ctx->vnode->name);
									pos->flags |= SC_BUF_FLAG_SW_ERR;
									atomic_dec(&dma_ctx->busy_cnt);
								}
							} else {
								pos->flags |= (SC_BUF_FLAG_SOF_TOUCH | SC_BUF_FLAG_TIMESTAMPED);
								tmp &= ~DMA_IRQ_START;
								if (sc_pipeline && sc_pipeline->is_online_mode) {
									pos->vb2_v4l2_buf.sequence = frame_idx;
									pos->vb2_v4l2_buf.vb2_buf.timestamp = ktime_get_boottime_ns();
								}
								atomic_inc(&dma_ctx->busy_cnt);
								if (rawdump && rawdump->rawdump_only) {
									if (__spm_vdev_idle_list_empty(dma_ctx->vnode)) {
										hw_isp_top_set_rdp_cfg_rdy(SC_BLOCK(isp_ctx->pipes[0]), rawdump->idx, 0);
										hw_isp_top_set_rawdump_source(SC_BLOCK(isp_ctx->pipes[0]), rawdump->idx, INVALID_CH);
										hw_isp_top_set_rdp_cfg_rdy(SC_BLOCK(isp_ctx->pipes[0]), rawdump->idx, 1);
										pos->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_CLOSE_DOWN;
									}
								}
							}
						}
					}
					spin_unlock(&(dma_ctx->vnode->slock));

					ret = fe_isp_get_dma_work(dma_ctx, &isp_dma_work);
					if (ret) {
						if (printk_timed_ratelimit(&print_jiffies, 5000)) {
							cam_warn("%s dma work idle list was null", dma_ctx->vnode->name);
							i = 0;
							spin_lock(&dma_ctx->slock);
							list_for_each_entry(isp_dma_work, &dma_ctx->dma_work_busy_list, busy_list_entry) {
								tasklet_state[i++] = isp_dma_work->dma_tasklet.state;
							}
							spin_unlock(&dma_ctx->slock);
							while (--i >= 0) {
								cam_warn("%s tasklet(%d) state=%lu", dma_ctx->vnode->name, i, tasklet_state[i]);
							}
						}
					} else {
						isp_dma_work->irq_status = irq_status;
#if USE_TASKLET
						//if (irq_status & DMA_IRQ_DONE && sc_pipeline && !sc_pipeline->is_online_mode) {
						//	cam_dbg("ao[%d] t", dma_ctx->id);
						//}
						tasklet_schedule(&(isp_dma_work->dma_tasklet));
#elif USE_WORKQ
						if(!schedule_work(&(isp_dma_work->dma_work)))
							cam_err("%s schedule work failed", dma_ctx->vnode->name);
#else
						fe_isp_dma_bh_handler(isp_dma_work);
#endif
					}

					if (irq_status & DMA_IRQ_DONE) {
						//if (sc_pipeline && !sc_pipeline->is_online_mode) {
						//	cam_dbg("ao[%d] d", dma_ctx->id);
						//}
						if (dma_ctx->id == 0)
							hw_isp_top_clr_irq_status(SC_BLOCK(isp_ctx->pipes[0]), 0x5bff0000);
						if (dma_ctx->id == 1)
							hw_isp_top_clr_irq_status(SC_BLOCK(isp_ctx->pipes[1]), 0x5bff0000);
					}
				} else if (irq_status & DMA_IRQ_START) {
					hw_dma_set_wdma_ready(SC_BLOCK(dma_ctx->vnode), dma_ctx->vnode->idx, 1);
				}
			}
		}
		spin_lock(&dma_ctx->waitq_head.lock);
		dma_ctx->in_irq = 0;
		wake_up_locked(&dma_ctx->waitq_head);
		spin_unlock(&dma_ctx->waitq_head.lock);
	}
	for (irq_src = DMA_IRQ_SRC_RDMA_CH0; irq_src <= DMA_IRQ_SRC_RDMA_CH1; irq_src++) {
		dma_ctx = &(isp_ctx->dma_in_ctx[irq_src - DMA_IRQ_SRC_RDMA_CH0]);
		spin_lock(&dma_ctx->waitq_head.lock);
		dma_ctx->in_irq = 1;
		if (dma_ctx->in_streamoff) {
			dma_ctx->in_irq = 0;
			wake_up_locked(&dma_ctx->waitq_head);
			spin_unlock(&dma_ctx->waitq_head.lock);
			continue;
		}
		spin_unlock(&dma_ctx->waitq_head.lock);
		if (dma_ctx->vnode || (is_mix_hdr && irq_src == DMA_IRQ_SRC_RDMA_CH0)) {
			irq_status = hw_dma_irq_analyze(irq_src, status1, status2);
			if (is_mix_hdr && (irq_status & DMA_IRQ_ERR)) {
				cam_err("ain0 DMA_IRQ_ERR");
			}
			if (irq_status && dma_ctx->vnode) {
				tmp = irq_status;
				spin_lock(&(dma_ctx->vnode->slock));
				list_for_each_entry(pos, &dma_ctx->vnode->busy_list, list_entry) {
					if (!tmp)
						break;
					if ((tmp & DMA_IRQ_ERR) && !(pos->flags & SC_BUF_FLAG_HW_ERR)) {
						pos->flags |= SC_BUF_FLAG_HW_ERR;
						tmp &= ~DMA_IRQ_ERR;
					}
					if ((tmp & DMA_IRQ_DONE) && !(pos->flags & SC_BUF_FLAG_DONE_TOUCH)) {
						pos->flags |= SC_BUF_FLAG_DONE_TOUCH;
						tmp &= ~DMA_IRQ_DONE;
					}
					if (tmp & DMA_IRQ_START) {
						if (pos->flags & SC_BUF_FLAG_SOF_TOUCH) {
							if (!(pos->flags & (SC_BUF_FLAG_DONE_TOUCH | SC_BUF_FLAG_HW_ERR | SC_BUF_FLAG_SW_ERR))) {
								cam_warn("%s next sof arrived without dma done or err", dma_ctx->vnode->name);
								pos->flags |= SC_BUF_FLAG_SW_ERR;
							}
						} else {
							pos->flags |= SC_BUF_FLAG_SOF_TOUCH;
						}
						tmp &= ~DMA_IRQ_START;
					}
				}
				spin_unlock(&(dma_ctx->vnode->slock));
				ret = fe_isp_get_dma_work(dma_ctx, &isp_dma_work);
				if (ret) {
					cam_warn("%s dma work idle list was null", dma_ctx->vnode->name);
				} else {
					isp_dma_work->irq_status = irq_status;
#if USE_TASKLET
					tasklet_schedule(&(isp_dma_work->dma_tasklet));
#elif USE_WORKQ
					if (!schedule_work(&(isp_dma_work->dma_work)))
						cam_err("%s schedule work failed", dma_ctx->vnode->name);
#else
					fe_isp_dma_bh_handler(isp_dma_work);
#endif
				}
				//if (irq_status & DMA_IRQ_DONE) {
				//	cam_dbg("ai[%d] d", dma_ctx->id);
				//}
			}
		}
		spin_lock(&dma_ctx->waitq_head.lock);
		dma_ctx->in_irq = 0;
		wake_up_locked(&dma_ctx->waitq_head);
		spin_unlock(&dma_ctx->waitq_head.lock);
	}

	for (i = 0; i < 2; i++) {
		if (sc_pipelines[i]) {
			fe_isp_process_dma_reload(isp_ctx, sc_pipelines[i]);
		}
	}
	// dma9, dma10,dma14 and dma15 irqs forward to isp firmware
	if ((status1 & 0xf8000000) || (status2 & 0xfc01)) {
		irq_data.status1 = status1;
		irq_data.status2 = status2;
		sc_ispfirm_call(ispfirm_ops, irq_callback, DMA_IRQ, &irq_data, sizeof(irq_data));
	}

	return IRQ_HANDLED;
}

static irqreturn_t fe_isp_process_dma_reload(struct isp_context *isp_ctx, struct spm_camera_pipeline *sc_pipeline)
{
	int i = 0, buf_ready = 0, ret = 0, flag = 0, dma_busy = 0;
	unsigned int buf_index = 0;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct isp_dma_context *dma_ctx = NULL;
	struct spm_camera_vbuffer *sc_vb = NULL, *pos = NULL;

	pipe_ctx = (struct isp_pipeline_context *)sc_pipeline->usr_data;
	if (!pipe_ctx) {
		cam_err("%s pipe_ctx was null", __func__);
		return IRQ_HANDLED;
	}
	spin_lock(&sc_pipeline->slock);
	if (pipe_ctx->mmu_tbu_reload > 0) {
		pipe_ctx->mmu_tbu_reload--;
	}
	if (sc_pipeline->state < PIPELINE_ST_STARTED) {
		spin_unlock(&sc_pipeline->slock);
		return IRQ_HANDLED;
	}
	for (i = 0; i < FORMATTER_NUM && pipe_ctx->mmu_tbu_reload <= MMU_TBU_RELOAD; i++) {
		buf_ready = 1;
		if (pipe_ctx->fmt_wdma_cnt[i] < pipe_ctx->fmt_wdma_start_cnt[i])
			buf_ready = 0;
		if (buf_ready) {
			list_for_each_entry(dma_ctx, &pipe_ctx->fmt_wdma_list[i], list_entry) {
				sc_vnode = dma_ctx->vnode;
				flag = 0;
				if (sc_vnode) {
					spin_lock(&sc_vnode->slock);
					list_for_each_entry(pos, &sc_vnode->queued_list, list_entry) {
						if (!(pos->flags & SC_BUF_FLAG_CCIC_TOUCH)) {
							flag = 1;
							break;
						}
					}
					if (!flag) {
						buf_ready = 0;
						spin_unlock(&sc_vnode->slock);
						break;
					}
					spin_unlock(&sc_vnode->slock);
				}
			}
		}
		if (buf_ready) {
			flag = 0;
			list_for_each_entry(dma_ctx, &pipe_ctx->fmt_wdma_list[i], list_entry) {
				sc_vnode = dma_ctx->vnode;
				if (sc_vnode) {
					//hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 0);
					spin_lock(&sc_vnode->slock);
					//if (pipe_ctx->mmu_tbu_reload == MMU_TBU_OK) {
						list_for_each_entry(pos, &sc_vnode->queued_list, list_entry) {
							if (!(pos->flags & SC_BUF_FLAG_CCIC_TOUCH)) {
								pos->flags |= SC_BUF_FLAG_CCIC_TOUCH;
								break;
							}
						}
						ret = __spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
					//} else { // MMU_TBU_RELOAD
					//	ret = __spm_vdev_pick_idle_vbuffer(sc_vnode, &sc_vb);
					//}
					if (0 == ret) {
						if (flag == 0)
							buf_index = sc_vb->vb2_v4l2_buf.vb2_buf.index;
						else if (buf_index != sc_vb->vb2_v4l2_buf.vb2_buf.index) {
							vi_irq_print("%s(%s) buf index miss match (%u vs %u)", __func__, sc_vnode->name, buf_index, sc_vb->vb2_v4l2_buf.vb2_buf.index);
						}
						fe_isp_update_aout_dma_addr(sc_vnode, sc_vb, 0);
						//if (pipe_ctx->mmu_tbu_reload == MMU_TBU_OK) {
							hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 1);
							__spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
						//}
					} else {
						vi_irq_print("%s(%s) failed to dq idle buf", __func__, sc_vnode->name);
					}
					spin_unlock(&sc_vnode->slock);
					flag++;
				}
			}
		} else {
			list_for_each_entry(dma_ctx, &pipe_ctx->fmt_wdma_list[i], list_entry) {
				sc_vnode = dma_ctx->vnode;
				if (sc_vnode) {
					//hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 0);
					spin_lock(&sc_vnode->slock);
					if (sc_vnode->sc_vb) {
						sc_vb = sc_vnode->sc_vb;
						fe_isp_update_aout_dma_addr(sc_vnode, sc_vb, 0);
						hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 1);
					}
					spin_unlock(&sc_vnode->slock);
				}
			}
		}
	}
	if (pipe_ctx->mmu_tbu_reload <= MMU_TBU_RELOAD) {
		list_for_each_entry(dma_ctx, &pipe_ctx->wdma_list, list_entry) {
			sc_vnode = dma_ctx->vnode;
			if (sc_vnode) {
				//hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 0);
				spin_lock(&sc_vnode->slock);
				//list_for_each_entry(pos, &sc_vnode->queued_list, list_entry) {
				//	if (!(pos->flags & SC_BUF_FLAG_CCIC_TOUCH)) {
				//		pos->flags |= SC_BUF_FLAG_CCIC_TOUCH;
				//		break;
				//	}
				//}
				if ((sc_vnode->idx == 12 || sc_vnode->idx == 13) && sc_vnode->sc_vb) {// rawdump
					if (__spm_vdev_busy_list_empty(sc_vnode)) {
						dma_busy = 0;
					} else {
						dma_busy = 1;
					}
					if (dma_busy) {
						ret = __spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
						if (0 == ret) {
							__spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
						}
					} else {
						ret = __spm_vdev_pick_idle_vbuffer(sc_vnode, &sc_vb);
						if (0 == ret) {
							if (sc_vb->flags & SC_BUF_FLAG_GEN_EOF) {
								__spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
								__spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
							} else {
								sc_vb->flags |= SC_BUF_FLAG_GEN_EOF;
								sc_vb = sc_vnode->sc_vb;
							}
						}
					}
				} else {
					ret = __spm_vdev_dq_idle_vbuffer(sc_vnode, &sc_vb);
					if (0 == ret) {
						__spm_vdev_q_busy_vbuffer(sc_vnode, sc_vb);
					} else {
						if (sc_vnode->sc_vb) {
							sc_vb = sc_vnode->sc_vb;
						}
					}
				}
				if (sc_vb) {
					fe_isp_update_aout_dma_addr(sc_vnode, sc_vb, 0);
					hw_dma_set_wdma_ready(SC_BLOCK(sc_vnode), sc_vnode->idx, 1);
				}
				spin_unlock(&sc_vnode->slock);
			}
		}
	}
	spin_unlock(&sc_pipeline->slock);
	return IRQ_HANDLED;
}

static void fe_isp_flush_pipeline_buffers(struct isp_context *isp_ctx, struct spm_camera_pipeline *sc_pipeline)
{
	int i = 0;
	unsigned long flags = 0;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct isp_dma_context *dma_ctx = NULL;
	struct spm_camera_vbuffer *pos = NULL, *n = NULL;

	pipe_ctx = (struct isp_pipeline_context *)sc_pipeline->usr_data;
	if (!pipe_ctx) {
		cam_err("%s pipe_ctx was null", __func__);
		return;
	}
	spin_lock_irqsave(&sc_pipeline->slock, flags);
	for (i = 0; i < FORMATTER_NUM; i++) {
		list_for_each_entry(dma_ctx, &pipe_ctx->fmt_wdma_list[i], list_entry) {
			sc_vnode = dma_ctx->vnode;
			if (sc_vnode) {
				spin_lock(&sc_vnode->slock);
				list_for_each_entry_safe(pos, n, &sc_vnode->queued_list, list_entry) {
					list_del_init(&(pos->list_entry));
					pos->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_IGNOR;
					fe_isp_export_camera_vbuffer(sc_vnode, pos);
				}
				spin_unlock(&sc_vnode->slock);
			}
		}
	}
	list_for_each_entry(dma_ctx, &pipe_ctx->wdma_list, list_entry) {
		sc_vnode = dma_ctx->vnode;
		if (sc_vnode) {
			spin_lock(&sc_vnode->slock);
			list_for_each_entry_safe(pos, n, &sc_vnode->queued_list, list_entry) {
				list_del_init(&(pos->list_entry));
				pos->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_IGNOR;
				fe_isp_export_camera_vbuffer(sc_vnode, pos);
			}
			spin_unlock(&sc_vnode->slock);
		}
	}
	spin_unlock_irqrestore(&sc_pipeline->slock, flags);
}

static irqreturn_t fe_isp_irq_handler(int irq, void *dev_id)
{
	unsigned int pipe0_irq_status = 0, pipe1_irq_status = 0, err0_irq_status = 0, err2_irq_status = 0, err1_irq_status = 0;
	unsigned int posterr_status = 0, pipe0_irq_raw_status = 0, pipe1_irq_raw_status = 0;
	unsigned int err0_detect_mask = 0;
	struct isp_context *isp_ctx = (struct isp_context *)dev_id;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_entity *p0_me = &(isp_ctx->pipes[0]->sc_subdev.pcsd.sd.entity);
	struct media_entity *p1_me = &(isp_ctx->pipes[1]->sc_subdev.pcsd.sd.entity);
	struct media_pipeline *mpipe0 = media_entity_pipeline(p0_me);
	struct media_pipeline *mpipe1 = media_entity_pipeline(p1_me);
	struct frame_id *frame_id = NULL;
	struct isp_irq_data irq_data = { 0 };
	struct spm_camera_ispfirm_ops *ispfirm_ops = NULL;
	struct x1vi_platform_data *drvdata = platform_get_drvdata(isp_ctx->pdev);
	__u64 p0_frame_id = 0, p1_frame_id = 0;
	static DEFINE_RATELIMIT_STATE(rs, HZ / 10, 5);

	if (drvdata && drvdata->isp_firm)
		ispfirm_ops = drvdata->isp_firm->ispfirm_ops;

	pipe0_irq_status = hw_isp_top_get_irq_status(SC_BLOCK(isp_ctx->pipes[0]));
	if (pipe0_irq_status) {
		hw_isp_top_clr_irq_status(SC_BLOCK(isp_ctx->pipes[0]), pipe0_irq_status);
		if ((pipe0_irq_status & ISP_IRQ_PIPE_SOF) && mpipe0) {
			sc_pipeline = media_pipeline_to_sc_pipeline(mpipe0);
			spin_lock(&sc_pipeline->slock);
			isp_ctx->pipe_frame_id[0].id++;
			frame_id = fe_isp_frame_id(&sc_pipeline->frame_id_list);
			if (frame_id) {
				if (isp_ctx->pipe_frame_id[0].id < frame_id->id)
					isp_ctx->pipe_frame_id[0].id = frame_id->id;
				p0_frame_id = frame_id->id;
			} else {
				cam_warn("p0 irq handler:frame id list is null");
			}
			irq_data.pipe0_frame_id = isp_ctx->pipe_frame_id[0].id - 1;
			spin_unlock(&sc_pipeline->slock);
		}
		if (pipe0_irq_status & ISP_IRQ_G_RST_DONE)
			complete_all(&isp_ctx->global_reset_done);
		if (pipe0_irq_status & ISP_IRQ_SDE_SOF)
			complete_all(&isp_ctx->pipes[0]->sde_sof);
	}
	pipe1_irq_status = hw_isp_top_get_irq_status(SC_BLOCK(isp_ctx->pipes[1]));
	if (pipe1_irq_status) {
		hw_isp_top_clr_irq_status(SC_BLOCK(isp_ctx->pipes[1]), pipe1_irq_status);
		if ((pipe1_irq_status & ISP_IRQ_PIPE_SOF) && mpipe1) {
			sc_pipeline = media_pipeline_to_sc_pipeline(mpipe1);
			spin_lock(&sc_pipeline->slock);
			isp_ctx->pipe_frame_id[1].id++;
			frame_id = fe_isp_frame_id(&sc_pipeline->frame_id_list);
			if (frame_id) {
				if (isp_ctx->pipe_frame_id[1].id < frame_id->id)
					isp_ctx->pipe_frame_id[1].id = frame_id->id;
				p1_frame_id = frame_id->id;
			} else {
				cam_warn("p1 irq handler:frame id list is null");
			}
			irq_data.pipe1_frame_id = isp_ctx->pipe_frame_id[1].id - 1;
			spin_unlock(&sc_pipeline->slock);
		}
		if (pipe1_irq_status & ISP_IRQ_SDE_SOF)
			complete_all(&isp_ctx->pipes[1]->sde_sof);
	}
	posterr_status = hw_isp_top_get_posterr_irq_status(SC_BLOCK(isp_ctx->pipes[0]));
	if (posterr_status) {
		hw_isp_top_clr_posterr_irq_status(SC_BLOCK(isp_ctx->pipes[0]), posterr_status);
		if (posterr_status & POSTERR_IRQ_PIP0_SDW_CLOSE_DONE) {
			//cam_dbg("POSTERR_IRQ_PIP0_SDW_CLOSE_DONE");
			complete_all(&(isp_ctx->pipes[0]->close_done));
		}
		if (posterr_status & POSTERR_IRQ_PIP1_SDW_CLOSE_DONE) {
			//cam_dbg("POSTERR_IRQ_PIP1_SDW_CLOSE_DONE");
			complete_all(&(isp_ctx->pipes[1]->close_done));
		}
		if (posterr_status & POSTERR_IRQ_RDP0_SDW_CLOSE_DONE) {
			//cam_dbg("POSTERR_IRQ_RDP0_SDW_CLOSE_DONE");
			atomic_set(&(isp_ctx->rawdumps[0]->close_done), 1);
		}
		if (posterr_status & POSTERR_IRQ_RDP1_SDW_CLOSE_DONE) {
			//cam_dbg("POSTERR_IRQ_RDP1_SDW_CLOSE_DONE");
			atomic_set(&(isp_ctx->rawdumps[1]->close_done), 1);
		}
	}
	if (p0_frame_id == 0 && mpipe0) {
		sc_pipeline = media_pipeline_to_sc_pipeline(mpipe0);
		spin_lock(&sc_pipeline->slock);
		frame_id = fe_isp_frame_id(&sc_pipeline->frame_id_list);
		if (frame_id)
			p0_frame_id = frame_id->id;
		spin_unlock(&sc_pipeline->slock);
	}
	if (p1_frame_id == 0 && mpipe1) {
		sc_pipeline = media_pipeline_to_sc_pipeline(mpipe1);
		spin_lock(&sc_pipeline->slock);
		frame_id = fe_isp_frame_id(&sc_pipeline->frame_id_list);
		if (frame_id)
			p1_frame_id = frame_id->id;
		spin_unlock(&sc_pipeline->slock);
	}
	err0_irq_status = hw_isp_top_get_err0_irq_status(SC_BLOCK(isp_ctx->pipes[0]));
	if (err0_irq_status)
		hw_isp_top_clr_err0_irq_status(SC_BLOCK(isp_ctx->pipes[0]), err0_irq_status);
	err2_irq_status = hw_isp_top_get_err2_irq_status(SC_BLOCK(isp_ctx->pipes[0]));
	if (err2_irq_status)
		hw_isp_top_clr_err2_irq_status(SC_BLOCK(isp_ctx->pipes[0]), err2_irq_status);
	irq_data.pipe0_irq_status = pipe0_irq_status;
	irq_data.pipe1_irq_status = pipe1_irq_status;
	sc_ispfirm_call(ispfirm_ops, irq_callback, ISP_IRQ, &irq_data, sizeof(irq_data));
	//cam_dbg("pipe0 irq status=0x%08x, pipe1 irq status=0x%08x", pipe0_irq_status, pipe1_irq_status);
	if (mpipe0)
		sc_pipeline = media_pipeline_to_sc_pipeline(mpipe0);
	else if (mpipe1)
		sc_pipeline = media_pipeline_to_sc_pipeline(mpipe1);
	else
		sc_pipeline = NULL;
	if (!sc_pipeline || PIPELINE_TYPE(sc_pipeline->id) == PIPELINE_TYPE_SINGLE)
		err0_detect_mask = 0xffff001f;
	else
		err0_detect_mask = 0xffff001c;
	if (err2_irq_status & ERR2_PIPE0_OVERRUN)
		isp_ctx->isp_fatal_error |= ISP_FATAL_ERR_PIPE0_OVERRUN;
	if (err2_irq_status & ERR2_PIPE1_OVERRUN)
		isp_ctx->isp_fatal_error |= ISP_FATAL_ERR_PIPE1_OVERRUN;

	err1_irq_status = hw_isp_top_get_err1_irq_status(SC_BLOCK(isp_ctx->pipes[0]));
	if (err1_irq_status)
		hw_isp_top_clr_err1_irq_status(SC_BLOCK(isp_ctx->pipes[0]), err1_irq_status);

	if ((err0_irq_status & err0_detect_mask) || (err2_irq_status & 0xf80001ff) || err1_irq_status) {
		pipe0_irq_raw_status = hw_isp_top_get_irq_raw_status(SC_BLOCK(isp_ctx->pipes[0]));
		pipe1_irq_raw_status = hw_isp_top_get_irq_raw_status(SC_BLOCK(isp_ctx->pipes[1]));
		if (__ratelimit(&rs)) {
			cam_err("err0_irq_status=0x%08x err1_irq_status=0x%08x err2_irq_status=0x%08x p0_frame_id=%llu p1_frame_id=%llu",
				err0_irq_status, err1_irq_status, err2_irq_status, p0_frame_id, p1_frame_id);
			cam_err("p0_irq_raw_status=0x%08x p1_irq_raw_status=0x%08x", pipe0_irq_raw_status, pipe1_irq_raw_status);
			hw_isp_top_pipe0_debug_dump(SC_BLOCK(isp_ctx->pipes[0]));
			hw_isp_top_pipe1_debug_dump(SC_BLOCK(isp_ctx->pipes[1]));
		}
	}
	return IRQ_HANDLED;
}

static void fe_isp_print_work_handler(struct work_struct *work)
{
	struct isp_print_work_struct *print_work = container_of(work, struct isp_print_work_struct, print_work);
	struct isp_context *isp_ctx = print_work->isp_ctx;
	unsigned long flags = 0;

	cam_err("%s", print_work->msg_string);
	spin_lock_irqsave(&isp_ctx->slock, flags);
	list_add(&print_work->list, &isp_ctx->print_work_list);
	spin_unlock_irqrestore(&isp_ctx->slock, flags);
}

static void fe_isp_dma_work_handler(struct work_struct *work)
{
}

static void fe_isp_export_camera_vbuffer(struct spm_camera_vnode *sc_vnode,
					 struct spm_camera_vbuffer *sc_vb)
{
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	struct media_pad *remote_pad = NULL;
	struct fe_formatter *formatter = NULL;
	struct fe_dwt *dwt = NULL;
	struct spm_camera_vbuffer *pos = NULL, *n = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(&sc_vnode->vnode.entity);
	unsigned int buf_index = sc_vb->vb2_v4l2_buf.vb2_buf.index;

	if (!pipe)
		goto export_buffer;
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	pipe_ctx = (struct isp_pipeline_context *)sc_pipeline->usr_data;
	if (!pipe_ctx)
		goto export_buffer;
	remote_pad = media_entity_remote_pad(&sc_vnode->pad);
	if (!remote_pad)
		goto export_buffer;
	formatter = media_entity_to_formatter(remote_pad->entity);
	dwt = media_entity_to_dwt(remote_pad->entity);
	if (!formatter) {
		if (!dwt)
			goto export_buffer;
		remote_pad = media_entity_remote_pad(&dwt->pads[PAD_IN]);
		if (!remote_pad)
			goto export_buffer;
		formatter = media_entity_to_formatter(remote_pad->entity);
		if (!formatter)
			goto export_buffer;
	}
	if (pipe_ctx->fmt_wdma_start_cnt[formatter->idx] <= 1) {
		goto export_buffer;
	}
	if (++(pipe_ctx->fmt_wdma_sync_cnt[formatter->idx][buf_index]) >= 5) {
		list_for_each_entry_safe(pos, n, &pipe_ctx->fmt_wdma_sync[formatter->idx][buf_index], list_entry) {
			list_del_init(&(pos->list_entry));
			if (pos->flags & (SC_BUF_FLAG_HW_ERR | SC_BUF_FLAG_SW_ERR))
				spm_vdev_export_camera_vbuffer(pos, 1);
			else
				spm_vdev_export_camera_vbuffer(pos, 0);
		}
		pipe_ctx->fmt_wdma_sync_cnt[formatter->idx][buf_index] = 0;
	} else {
		list_add(&sc_vb->list_entry, &pipe_ctx->fmt_wdma_sync[formatter->idx][buf_index]);
		return;
	}

export_buffer:
	if (sc_vb->flags & (SC_BUF_FLAG_HW_ERR | SC_BUF_FLAG_SW_ERR))
		spm_vdev_export_camera_vbuffer(sc_vb, 1);
	else
		spm_vdev_export_camera_vbuffer(sc_vb, 0);
}

static void fe_isp_dma_bh_handler(struct isp_dma_work_struct *isp_dma_work)
{
	struct isp_dma_context *dma_ctx = isp_dma_work->dma_ctx;
	struct spm_camera_vnode *sc_vnode = dma_ctx->vnode;
	//struct isp_context *isp_ctx = dma_ctx->isp_ctx;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *pipe = NULL;
	struct spm_camera_vbuffer *n = NULL, *pos = NULL;
	unsigned int *hw_err_code = NULL;
	unsigned int irq_status = isp_dma_work->irq_status;
	LIST_HEAD(export_list);
	unsigned long flags = 0;

	if (!sc_vnode) {
		cam_dbg("a[%d] debug 0", dma_ctx->id);
		goto dma_tasklet_finish;
	}

	pipe = media_entity_pipeline(&sc_vnode->vnode.entity);

	if (!sc_vnode || !pipe) {
		cam_dbg("a[%d] debug 1", dma_ctx->id);
		goto dma_tasklet_finish;
	}

	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);

	if (irq_status & DMA_IRQ_DONE && sc_pipeline && !sc_pipeline->is_online_mode)
		cam_dbg("a[%d] direction=%d debug 2", dma_ctx->id, sc_vnode->direction);
	spin_lock(&sc_vnode->waitq_head.lock);
	sc_vnode->in_tasklet = 1;
	if (sc_vnode->in_streamoff || !is_vnode_streaming(sc_vnode)) {
		wake_up_locked(&sc_vnode->waitq_head);
		spin_unlock(&sc_vnode->waitq_head.lock);
		if (irq_status & DMA_IRQ_DONE && sc_pipeline && !sc_pipeline->is_online_mode)
			cam_dbg("a[%d] debug 3", dma_ctx->id);
		goto dma_tasklet_finish;
	}
	wake_up_locked(&sc_vnode->waitq_head);
	spin_unlock(&sc_vnode->waitq_head.lock);

	if (irq_status & DMA_IRQ_START) {
		if (dma_ctx->used_for_hdr) {
			cam_dbg("a[%d] debug 4", dma_ctx->id);
			goto dma_tasklet_finish;
		}
	}

	spin_lock_irqsave(&sc_vnode->slock, flags);
	list_for_each_entry_safe(pos, n, &sc_vnode->busy_list, list_entry) {
		if (pos->flags & (SC_BUF_FLAG_HW_ERR | SC_BUF_FLAG_SW_ERR | SC_BUF_FLAG_DONE_TOUCH)) {
			list_del_init(&(pos->list_entry));
			atomic_dec(&sc_vnode->busy_buf_cnt);
			list_add_tail(&(pos->list_entry), &export_list);
		}
	}
	spin_unlock_irqrestore(&sc_vnode->slock, flags);
	list_for_each_entry_safe(pos, n, &export_list, list_entry) {
		if (!(pos->flags & SC_BUF_FLAG_SOF_TOUCH) && sc_pipeline && sc_pipeline->is_online_mode) {
			cam_warn("%s export buf index=%u frameid=%u without sof touch", sc_vnode->name, pos->vb2_v4l2_buf.vb2_buf.index, pos->vb2_v4l2_buf.sequence);
		}
		if (pos->flags & SC_BUF_FLAG_HW_ERR) {
			hw_err_code = (unsigned int *)(&(pos->reserved[0]));
			pos->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_ERROR_HW;
			if (*hw_err_code & PIPE_ERR(1)) {
				pos->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_IDI_OVERRUN;
			}
			//vi_irq_print("%s export buf index=%u frameid=%u with hw error(0x%08x)", sc_vnode->name, pos->vb2_v4l2_buf.vb2_buf.index, pos->vb2_v4l2_buf.sequence, *hw_err_code);
			spin_lock_irqsave(&sc_pipeline->slock, flags);
			fe_isp_export_camera_vbuffer(sc_vnode, pos);
			spin_unlock_irqrestore(&sc_pipeline->slock, flags);
			sc_vnode->hw_err_frm++;
		} else if (pos->flags & SC_BUF_FLAG_SW_ERR) {
			pos->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_ERROR_SW;
			cam_warn("%s export buf index=%u frameid=%u with sw error", sc_vnode->name, pos->vb2_v4l2_buf.vb2_buf.index, pos->vb2_v4l2_buf.sequence);
			spin_lock_irqsave(&sc_pipeline->slock, flags);
			fe_isp_export_camera_vbuffer(sc_vnode, pos);
			spin_unlock_irqrestore(&sc_pipeline->slock, flags);
			sc_vnode->sw_err_frm++;
		} else if (pos->flags & SC_BUF_FLAG_DONE_TOUCH) {
			if (sc_pipeline && !sc_pipeline->is_online_mode) {
				cam_dbg("a[%d] direction=%d export buf index=%u frameid=%u",
					dma_ctx->id, sc_vnode->direction, pos->vb2_v4l2_buf.vb2_buf.index, pos->vb2_v4l2_buf.sequence);
			}
			spin_lock_irqsave(&sc_pipeline->slock, flags);
			fe_isp_export_camera_vbuffer(sc_vnode, pos);
			spin_unlock_irqrestore(&sc_pipeline->slock, flags);
			sc_vnode->ok_frm++;
		}
	}

dma_tasklet_finish:
	if (sc_vnode) {
		spin_lock(&sc_vnode->waitq_head.lock);
		sc_vnode->in_tasklet = 0;
		wake_up_locked(&sc_vnode->waitq_head);
		spin_unlock(&sc_vnode->waitq_head.lock);
	}
	fe_isp_put_dma_work(dma_ctx, isp_dma_work);

}

static void fe_isp_dma_tasklet_handler(unsigned long param)
{
	struct isp_dma_work_struct *isp_dma_work = (struct isp_dma_work_struct *)param;
	fe_isp_dma_bh_handler(isp_dma_work);
}

void *fe_isp_create_ctx(struct platform_device *pdev)
{
	struct device *dev = NULL;
	struct isp_context *isp_ctx = NULL;
	int i = 0;
	int ret = 0;
	struct resource *pdev_resc = NULL;
	void __iomem *io_base = NULL;
#ifdef CONFIG_KY_X1_VI_IOMMU
	size_t tabs_size = 0, tab_offset = 0;
	int j = 0;
#endif
	//unsigned long rsvd_mem_size = 0, align = 0;
	//struct page **pages = NULL;

	if (!pdev) {
		pr_err("%s invalid arguments.", __func__);
		return NULL;
	}
	dev = &pdev->dev;
	isp_ctx = devm_kzalloc(dev, sizeof(*isp_ctx), GFP_KERNEL);
	if (!isp_ctx) {
		cam_err("%s not enough mem.", __func__);
		return NULL;
	}
	isp_ctx->pdev = pdev;
	pdev_resc = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vi");
	if (!pdev_resc) {
		cam_err("get reg resource for vi failed");
		return NULL;
	}
	io_base = devm_ioremap_resource(dev, pdev_resc);
	if (IS_ERR(io_base)) {
		cam_err("ioremap for isp-frontend failed");
		return NULL;
	}

	/* get clock(s) */
#ifdef CONFIG_ARCH_KY
/*	
	isp_ctx->ahb_clk = devm_clk_get(&pdev->dev, "isp_ahb");
	if (IS_ERR(isp_ctx->ahb_clk)) {
		ret = PTR_ERR(isp_ctx->ahb_clk);
		cam_err("failed to get ahb clock: %d\n", ret);
		return NULL;
	}
*/
	isp_ctx->ahb_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_ahb_reset");
	if (IS_ERR_OR_NULL(isp_ctx->ahb_reset)) {
		ret = PTR_ERR(isp_ctx->ahb_reset);
		dev_err(&pdev->dev, "not found core isp_ahb_reset, %d\n", ret);
		return NULL;
	}
	isp_ctx->isp_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_reset");
	if (IS_ERR_OR_NULL(isp_ctx->isp_reset)) {
		ret = PTR_ERR(isp_ctx->isp_reset);
		dev_err(&pdev->dev, "not found core isp_reset, %d\n", ret);
		return NULL;
	}

	isp_ctx->isp_ci_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_ci_reset");
	if (IS_ERR_OR_NULL(isp_ctx->isp_ci_reset)) {
		ret = PTR_ERR(isp_ctx->isp_ci_reset);
		dev_err(&pdev->dev, "not found core isp_ci_reset, %d\n", ret);
		return NULL;
	}

	isp_ctx->lcd_mclk_reset = devm_reset_control_get_optional_shared(&pdev->dev, "lcd_mclk_reset");
	if (IS_ERR_OR_NULL(isp_ctx->lcd_mclk_reset)) {
		ret = PTR_ERR(isp_ctx->lcd_mclk_reset);
		dev_err(&pdev->dev, "not found core lcd_mclk_reset, %d\n", ret);
		return NULL;
	}

	isp_ctx->fnc_clk = devm_clk_get(&pdev->dev, "isp_func");
	if (IS_ERR(isp_ctx->fnc_clk)) {
		ret = PTR_ERR(isp_ctx->fnc_clk);
		cam_err("failed to get function clock: %d\n", ret);
		return NULL;
	}
#ifdef CONFIG_KY_DEBUG
	vi_running_info.is_dev_running = check_dev_running_status;
	vi_running_info.nb.notifier_call = dev_clkoffdet_notifier_handler;
	clk_notifier_register(isp_ctx->fnc_clk, &vi_running_info.nb);
#endif
	isp_ctx->bus_clk = devm_clk_get(&pdev->dev, "isp_axi");
	if (IS_ERR(isp_ctx->bus_clk)) {
		ret = PTR_ERR(isp_ctx->bus_clk);
		cam_err("failed to get bus clock: %d\n", ret);
		return NULL;
	}

	isp_ctx->dpu_clk = devm_clk_get(&pdev->dev, "dpu_mclk");
	if (IS_ERR(isp_ctx->dpu_clk)) {
		ret = PTR_ERR(isp_ctx->dpu_clk);
		cam_err("failed to get dpu clock: %d\n", ret);
		return NULL;
	}
#endif

	isp_ctx->base_addr = (unsigned long)io_base;
	for (i = 0; i < PIPE_NUM; i++) {
		INIT_LIST_HEAD(&isp_ctx->pipe_frame_id[i].entry);
	}

#ifdef CONFIG_KY_X1_VI_IOMMU
	isp_ctx->rsvd_vaddr = devm_kmalloc(&pdev->dev, MMU_RESERVED_MEM_SIZE, GFP_KERNEL);
	if (!isp_ctx->rsvd_vaddr) {
		cam_err("failed to alloc mem for mmu reserved");
		return NULL;
	}
	isp_ctx->rsvd_phy_addr = phys_cpu2cam(virt_to_phys(isp_ctx->rsvd_vaddr));
	cam_dbg("rsvd_phy_addr=0x%llx size=%d", (uint64_t)isp_ctx->rsvd_phy_addr, MMU_RESERVED_MEM_SIZE);
	memset(isp_ctx->rsvd_vaddr, 0xff, MMU_RESERVED_MEM_SIZE);

	isp_ctx->mmu_dev = isp_iommu_create(dev, isp_ctx->base_addr);
	if (!isp_ctx->mmu_dev) {
		cam_err("failed to create iommu device");
		return NULL;
	}
	tabs_size = 2 * IOMMU_TRANS_TAB_MAX_NUM * sizeof(uint32_t) * ISP_IOMMU_TBU_NUM;
	isp_ctx->trans_tab_cpu_addr = dmam_alloc_coherent(dev,
							  tabs_size,
							  &isp_ctx->trans_tab_dma_addr,
							  GFP_KERNEL);
	if (!isp_ctx->trans_tab_cpu_addr) {
		cam_err("%s alloc page tables failed", __func__);
		return NULL;
	}
	isp_ctx->total_trans_tab_sz = tabs_size;
	tab_offset = 0;
#endif

	for (i = 0; i < AOUT_NUM; i++) {
		isp_ctx->dma_out_ctx[i].id = i;
		ret = fe_isp_init_dma_context(&isp_ctx->dma_out_ctx[i],
					      isp_ctx,
					      ISP_DMA_WORK_MAX_CNT,
					      fe_isp_dma_work_handler,
					      fe_isp_dma_tasklet_handler,
					      dev);
		if (ret)
			return NULL;
#ifdef CONFIG_KY_X1_VI_IOMMU
		if (i == 9 || i == 10)
			continue;
		for (j = 0; j < 2; j++) {
			isp_ctx->dma_out_ctx[i].tt_addr[j][0] = isp_ctx->trans_tab_dma_addr + tab_offset;
			isp_ctx->dma_out_ctx[i].tt_base[j][0] = isp_ctx->trans_tab_cpu_addr + tab_offset;
			tab_offset += IOMMU_TRANS_TAB_MAX_NUM * sizeof(uint32_t);
			if (i != 12 && i != 13) {
				isp_ctx->dma_out_ctx[i].tt_addr[j][1] = isp_ctx->trans_tab_dma_addr + tab_offset;
				isp_ctx->dma_out_ctx[i].tt_base[j][1] = isp_ctx->trans_tab_cpu_addr + tab_offset;
				tab_offset += IOMMU_TRANS_TAB_MAX_NUM * sizeof(uint32_t);
			}
		}
#endif
	}
	for (i = 0; i < AIN_NUM; i++) {
		isp_ctx->dma_in_ctx[i].id = i;
		ret = fe_isp_init_dma_context(&isp_ctx->dma_in_ctx[i],
					      isp_ctx,
					      ISP_DMA_WORK_MAX_CNT,
					      fe_isp_dma_work_handler,
					      fe_isp_dma_tasklet_handler,
					      dev);
		if (ret)
			return NULL;
#ifdef CONFIG_KY_X1_VI_IOMMU
		for (j = 0; j < 2; j++) {
			isp_ctx->dma_in_ctx[i].tt_addr[j][0] = isp_ctx->trans_tab_dma_addr + tab_offset;
			isp_ctx->dma_in_ctx[i].tt_base[j][0] = isp_ctx->trans_tab_cpu_addr + tab_offset;
			tab_offset += IOMMU_TRANS_TAB_MAX_NUM * sizeof(uint32_t);
		}
#endif
	}

	for (i = 0; i < CCIC_MAX_CNT; i++) {
		ret = ccic_ctrl_get(&(isp_ctx->ccic[i].csi_ctrl), i, NULL);
		if (ret) {
			cam_err("get csi%d ctrl failed ret=%d", i, ret);
			return NULL;
		}
	}

	INIT_LIST_HEAD(&isp_ctx->print_work_list);
	for (i = 0; i < ISP_PRINT_WORK_MAX_CNT; i++) {
		isp_ctx->print_works[i].isp_ctx = isp_ctx;
		INIT_WORK(&(isp_ctx->print_works[i].print_work), fe_isp_print_work_handler);
		list_add(&(isp_ctx->print_works[i].list), &isp_ctx->print_work_list);
	}
	spin_lock_init(&isp_ctx->slock);
	atomic_set(&isp_ctx->pwr_cnt, 0);
	init_completion(&isp_ctx->global_reset_done);

	return isp_ctx;
}

void fe_isp_release_ctx(void *isp_context)
{
#ifdef CONFIG_KY_X1_VI_IOMMU
	struct isp_context *isp_ctx = (struct isp_context *)isp_context;
	isp_iommu_unregister(isp_ctx->mmu_dev);
#endif
}

static int notify_caputre_until_done(int slice_index,
				     struct camera_capture_slice_info *slice_info,
				     int timeout)
{
	struct platform_device *pdev = x1vi_get_platform_device();
	struct x1vi_platform_data *drvdata = NULL;
	struct isp_context *isp_ctx = NULL;
	struct media_pipeline *pipe = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct isp_pipeline_context *pipe_ctx = NULL;
	long l_ret = 0;

	/*
	cam_not("slice(%d/%d) slice_width:%d raw_read_offset:%d yuv_out_offset:%d dwt[1]_offset:%d dwt[2]_offset:%d dwt[3]_offset:%d dwt[4]_offset:%d notify",
		slice_index, slice_info->total_slice_cnt,
		slice_info->slice_width, slice_info->raw_read_offset,
		slice_info->yuv_out_offset,
		slice_info->dwt_offset[0], slice_info->dwt_offset[1],
		slice_info->dwt_offset[2], slice_info->dwt_offset[3]);
	*/
	if (!pdev) {
		cam_err("%s pdev is null", __func__);
		return -1;
	}
	if (slice_info->exception_exit) {
		cam_err("%s isp exception exit", __func__);
		return 0;
	}
	drvdata = platform_get_drvdata(pdev);
	BUG_ON(!drvdata);
	isp_ctx = drvdata->isp_ctx;
	BUG_ON(!isp_ctx);
	if (slice_info->hw_pipe_id < 0 || slice_info->hw_pipe_id > 1) {
		cam_err("%s hw_pipe_id %d is invalid", __func__, slice_info->hw_pipe_id);
		return -1;
	}
	pipe = media_entity_pipeline(&isp_ctx->pipes[slice_info->hw_pipe_id]->sc_subdev.pcsd.sd.entity);
	BUG_ON(!pipe);
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	BUG_ON(!sc_pipeline);
	mutex_lock(&sc_pipeline->mlock);
	if (!sc_pipeline->usr_data) {
		pipe_ctx = fe_pipeline_create_ctx(&pdev->dev);
		if (!pipe_ctx) {
			mutex_unlock(&sc_pipeline->mlock);
			cam_err("%s create pipe_ctx failed", __func__);
			return -1;
		}
		sc_pipeline->usr_data = pipe_ctx;
	} else {
		pipe_ctx = (struct isp_pipeline_context *)sc_pipeline->usr_data;
	}
	pipe_ctx->cc_slice_info = *slice_info;
	sc_pipeline->slice_id = slice_index;
	sc_pipeline->total_slice_cnt = slice_info->total_slice_cnt;
	sc_pipeline->slice_result = 0;
	reinit_completion(&sc_pipeline->slice_done);
	mutex_unlock(&sc_pipeline->mlock);
	atomic_set(&sc_pipeline->slice_info_update, 1);
	wake_up_interruptible_all(&sc_pipeline->slice_waitq);
	if (sc_pipeline->state <= PIPELINE_ST_STOPPING) {
		return 0;
	}
	l_ret = wait_for_completion_interruptible_timeout(&sc_pipeline->slice_done, msecs_to_jiffies(timeout));
	if (sc_pipeline->state <= PIPELINE_ST_STOPPING) {
		return 0;
	}
	if (l_ret == 0) {
		cam_err("%s wait for slice(%d/%d) done timeout(%d)", __func__, slice_index, slice_info->total_slice_cnt, timeout);
		hw_dma_dump_regs(isp_ctx->dma_block);
		return -1;
	} else if (l_ret < 0) {
		cam_err("%s wait for slice doen interrupted by user app(%lu)", __func__, l_ret);
		return -1;
	}
	return sc_pipeline->slice_result;
}

struct spm_camera_vi_ops vi_ops = {
	.notify_caputre_until_done = notify_caputre_until_done,
};

#include <vi.h>
#include <base_ctx.h>
#include <linux/comm_errno.h>
#include <proc/vi_dbg_proc.h>
#include <proc/vi_proc.h>
#include <proc/vi_isp_proc.h>
#include <vi_ext.h>
#include <base_cb.h>
#include <base_ctx.h>
#include <cif_cb.h>
#include <vpss_cb.h>
#include <vi_cb.h>
#include <ldc_cb.h>
#include <vb.h>
#include <vip/vi_perf_chk.h>
#include <vi_raw_dump.h>
#include "vi_sys.h"
#include "vbq.h"
#include "base_common.h"
#include "ion.h"
#include "cmdq.h"

/*******************************************************
 *  MACRO defines
 ******************************************************/
#define DMA_SETUP_2(raw_num, id)					\
	do {								\
		bufaddr = _mempool_get_addr();				\
		ispblk_dma_setaddr(ictx, raw_num, id, bufaddr);			\
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, id);	\
		_mempool_pop(bufsize);					\
	} while (0)

#define DMA_SETUP(raw_num, id)						\
	do {								\
		bufaddr = _mempool_get_addr();				\
		bufsize = ispblk_dma_config(ictx, raw_num, id, bufaddr);\
		_mempool_pop(bufsize);					\
	} while (0)

#define VI_PROFILE
#define VI_MAX_LIST_NUM		(0x80)
#define VI_CMDQ_BUF_SIZE	(0x20000)
#define VI_DDR_RETRAIN_REG	(0x281000f4)
/* In practical application, it is necessary to drop the frame of AE convergence process.
 * But in vi-vpss online & vpss-vc sbm scenario, there is no way to drop the frame.
 * Use cover with black to avoid this problem.
 */
#ifndef PORTING_TEST
#define COVER_WITH_BLACK
#endif
/*******************************************************
 *  Global variables
 ******************************************************/
u32 vi_log_lv = VI_ERR | VI_WARN | VI_NOTICE | VI_INFO | VI_DBG;
module_param(vi_log_lv, int, 0644);

#ifdef PORTING_TEST //test only
static bool usr_trigger;
int stop_stream_en;
static int count;
module_param(stop_stream_en, int, 0644);
#endif

struct sop_vi_ctx *g_vi_ctx;
struct overflow_info *g_overflow_info;
struct _vi_gdc_cb_param {
	mmf_chn_s chn;
	enum gdc_usage usage;
};

struct gdc_mesh g_vi_mesh[VI_MAX_CHN_NUM];
extern struct vb_jobs_t vi_jobs[VI_MAX_CHN_NUM];

/*******************************************************
 *  Internal APIs
 ******************************************************/

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
static void legacy_timer_emu_func(struct timer_list *t)
{
	struct legacy_timer_emu *lt = from_timer(lt, t, t);

	lt->function(lt->data);
}
#endif //(KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)

/**
 * _mempool_reset - reset the byteused and assigned buffer for each dma
 *
 */
static void _vi_mempool_reset(void)
{
	u8 i = 0;

	isp_mempool.byteused = 0;

	memset(isp_bufpool, 0x0, (sizeof(struct _membuf) * ISP_PRERAW_MAX));

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		spin_lock_init(&isp_bufpool[i].pre_fe_sts_lock);
		spin_lock_init(&isp_bufpool[i].pre_be_sts_lock);
		spin_lock_init(&isp_bufpool[i].post_sts_lock);
	}
}

/**
 * _mempool_get_addr - get mempool's latest address.
 *
 * @return: the latest address of the mempool.
 */
static u64 _mempool_get_addr(void)
{
	return isp_mempool.base + isp_mempool.byteused;
}

/**
 * _mempool_pop - acquire a buffer-space from mempool.
 *
 * @param size: the space acquired.
 * @return: negative if no enough space; o/w, the address of the buffer needed.
 */
static int64_t _mempool_pop(u32 size)
{
	int64_t addr;

	size = VI_ALIGN(size);

	if ((isp_mempool.byteused + size) > isp_mempool.size) {
		vi_pr(VI_ERR, "reserved_memory(0x%x) is not enough. byteused(0x%x) alloc_size(0x%x)\n",
				isp_mempool.size, isp_mempool.byteused, size);
		return -EINVAL;
	}

	addr = isp_mempool.base + isp_mempool.byteused;
	isp_mempool.byteused += size;

	return addr;
}

static bool ddr_need_retrain(struct sop_vi_dev *vdev)
{
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	if(!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_fe && !vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be) {
		if (ioread32(vdev->ddr_retrain_reg) & BIT(8)) {
			return true;
		}
	}

	return false;
}

static void trig_8051_if_pre_idle(struct sop_vi_dev *vdev)
{
	enum sop_isp_fe_chn_num fe_chn;
	enum sop_isp_raw raw_num;
	enum isp_splt_num_e splt_num;
	u32 ddr_retrain = 0;

	for (splt_num = ISP_SPLT0; splt_num < ISP_SPLT_MAX; splt_num++) {
		if ((atomic_read(&vdev->splt_state[splt_num][ISP_SPLT_CHN0]) != ISP_STATE_IDLE) ||
			(atomic_read(&vdev->splt_state[splt_num][ISP_SPLT_CHN1]) != ISP_STATE_IDLE)) {
			vi_pr(VI_WARN, "need retrain but raw_%d split busy\n", splt_num);
			return;
		}
	}
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;

		for (fe_chn = ISP_FE_CH0; fe_chn < ISP_FE_CHN_MAX; fe_chn++) {
			if (atomic_read(&vdev->pre_fe_state[raw_num][fe_chn]) != ISP_STATE_IDLE) {
				vi_pr(VI_WARN, "need retrain but raw_%d FE busy\n", raw_num);
				return;
			}
		}
	}

	/*if fe and splt idle trig 8051 retrain*/
	ddr_retrain = ioread32(vdev->ddr_retrain_reg);
	iowrite32(ddr_retrain | BIT(9), vdev->ddr_retrain_reg);

	vi_pr(VI_WARN, "call ddr retrain now 0x%x\n", ioread32(vdev->ddr_retrain_reg));
}

/**
 * try to trigger preraw and linespliter after postraw done or drop frame done
 */
static void isp_trig_whole_preraw(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	s8 ret = ISP_SUCCESS;

	ret = _pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ret |= _pre_hw_enque(vdev, raw_num, ISP_FE_CH1);
	}

	if (ctx->isp_pipe_cfg[raw_num].is_tile || line_spliter_en) {
		ret |= _pre_hw_enque(vdev, raw_num + 1, ISP_FE_CH0);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			ret |= _pre_hw_enque(vdev, raw_num + 1, ISP_FE_CH1);

		if (ddr_need_retrain(vdev) || ret == ISP_SUCCESS) {
			_splt_hw_enque(vdev, raw_num);
		}
	}
}

/**
 * block num 16 [0~15]
 *
 * line_spliter, preraw_fe:
 * left input [0~7], right input [8~15]
 * left output [0~7], right output [8~15]
 *
 * preraw_be, postraw:
 * left input [0~8], right input [7~15]
 * left output [0~7], right output [8~15]
 */
static void _isp_tile_calc_size(struct isp_ctx *ictx)
{
	struct _isp_cfg *cfg_raw0 = &ictx->isp_pipe_cfg[ISP_PRERAW0];
	struct _isp_cfg *cfg_raw1 = &ictx->isp_pipe_cfg[ISP_PRERAW1];
	u32 guard = (ictx->img_width % 16 == 0)
			? (ictx->img_width / 16)
			: (ictx->img_width / 16) + 1;
	u32 left_width = guard * (16 / 2 + 1);

	ictx->tile_cfg.l_in.start  = 0;
	ictx->tile_cfg.l_in.end    = cfg_raw0->crop.x + left_width - 1;
	ictx->tile_cfg.l_out.start = 0;
	ictx->tile_cfg.l_out.end   = left_width - guard - 1;
	ictx->tile_cfg.r_in.start  = cfg_raw0->crop.x + left_width - guard - guard;
	ictx->tile_cfg.r_in.end    = cfg_raw0->max_width - 1;
	ictx->tile_cfg.r_out.start = left_width - guard;
	ictx->tile_cfg.r_out.end   = ictx->img_width - 1;

	vi_pr(VI_INFO, "tile cfg: img_width(%d) left_width(%d) guard(%d)\n",
	      ictx->img_width, left_width, guard);
	vi_pr(VI_INFO, "tile cfg: Left in(%d %d) out(%d %d)\n",
	      ictx->tile_cfg.l_in.start, ictx->tile_cfg.l_in.end,
	      ictx->tile_cfg.l_out.start, ictx->tile_cfg.l_out.end);
	vi_pr(VI_INFO, "tile cfg: Right in(%d %d) out(%d %d)\n",
	      ictx->tile_cfg.r_in.start, ictx->tile_cfg.r_in.end,
	      ictx->tile_cfg.r_out.start, ictx->tile_cfg.r_out.end);

	memcpy(cfg_raw1, cfg_raw0, sizeof(struct _isp_cfg));
	cfg_raw0->csibdg_width = cfg_raw0->crop.x + ictx->tile_cfg.l_out.end + 1;
	cfg_raw1->csibdg_width = cfg_raw0->max_width - cfg_raw0->csibdg_width;

	cfg_raw0->crop.x       = cfg_raw0->crop.x;
	cfg_raw0->crop.y       = cfg_raw0->crop.y;
	cfg_raw0->crop.w       = cfg_raw0->csibdg_width - cfg_raw0->crop.x;
	cfg_raw0->crop.h       = cfg_raw0->crop.h;
	cfg_raw0->post_img_w   = cfg_raw0->crop.w;
	cfg_raw0->post_img_h   = cfg_raw0->crop.h;

	cfg_raw1->crop.x       = 0;
	cfg_raw1->crop.y       = cfg_raw1->crop.y;
	cfg_raw1->crop.w       = cfg_raw1->crop.w - ictx->tile_cfg.l_out.end - 1;
	cfg_raw1->crop.h       = cfg_raw1->crop.h;
	cfg_raw1->post_img_w   = cfg_raw1->crop.w;
	cfg_raw1->post_img_h   = cfg_raw1->crop.h;

	if (ictx->is_hdr_on) {
		memcpy(&cfg_raw0->crop_se, &cfg_raw0->crop, sizeof(struct vi_rect));
		memcpy(&cfg_raw1->crop_se, &cfg_raw1->crop, sizeof(struct vi_rect));
	}

	cfg_raw0->is_work_on_r_tile = false;
	cfg_raw1->is_work_on_r_tile = true;

	ictx->img_width  = cfg_raw0->crop.w;
	ictx->img_height = cfg_raw0->crop.h;

	vi_pr(VI_INFO, "csibdg max, w:h=%d:%d\n", cfg_raw0->max_width, cfg_raw0->max_height);
	vi_pr(VI_INFO, "csibdg_%d, w:h=%d:%d, crop_x:y:w:h=%d:%d:%d:%d\n",
	      ISP_PRERAW0, cfg_raw0->csibdg_width, cfg_raw0->csibdg_height,
	      cfg_raw0->crop.x, cfg_raw0->crop.y, cfg_raw0->crop.w, cfg_raw0->crop.h);
	vi_pr(VI_INFO, "csibdg_%d, w:h=%d:%d, crop_x:y:w:h=%d:%d:%d:%d\n",
	      ISP_PRERAW1, cfg_raw1->csibdg_width, cfg_raw1->csibdg_height,
	      cfg_raw1->crop.x, cfg_raw1->crop.y, cfg_raw1->crop.w, cfg_raw1->crop.h);
}

static void vi_gdc_callback(void *param, vb_blk blk)
{
	struct _vi_gdc_cb_param *_param = param;

	if (!param)
		return;

	vi_pr(VI_DBG, "vi_chn(%d) usage(%d)\n", _param->chn.chn_id, _param->usage);
	mutex_unlock(&g_vi_mesh[_param->chn.chn_id].lock);
	if (blk != VB_INVALID_HANDLE)
		vb_done_handler(_param->chn, CHN_TYPE_OUT, &vi_jobs[_param->chn.chn_id], blk);
	vfree(param);
}

static s32 _mesh_gdc_do_op_cb(enum gdc_usage usage, const void *usage_param,
			      struct vb_s *vb_in, pixel_format_e pixformat, u64 mesh_addr,
			      u8 sync_io, void *cb_param, u32 cb_param_size,
			      mod_id_e mod_id, rotation_e rotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.usage_param = usage_param;
	cfg.vb_in = vb_in;
	cfg.pix_format = pixformat;
	cfg.mesh_addr = mesh_addr;
	cfg.sync_io = sync_io;
	cfg.cb_param = cb_param;
	cfg.cb_param_size = cb_param_size;
	cfg.rotation = rotation;

	exe_cb.callee = E_MODULE_LDC;
	exe_cb.caller = E_MODULE_VI;
	exe_cb.cmd_id = LDC_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;
	return base_exe_module_cb(&exe_cb);
}

static void _vi_release_ext_buf(u64 phy_addr)
{
	vb_blk blk = 0;

	blk = vb_phys_addr2handle(phy_addr);
	if (blk != VB_INVALID_HANDLE) {
		vb_release_block(blk);
	}
}

void _isp_snr_cfg_enq(struct sop_isp_snr_update *snr_node, const enum sop_isp_raw raw_num)
{
	unsigned long flags;
	struct _isp_snr_i2c_node *n, *q;

	if (snr_node == NULL)
		return;

	spin_lock_irqsave(&snr_node_lock[raw_num], flags);

	if (snr_node->snr_cfg_node.snsr.need_update) {
		n = kmalloc(sizeof(*n), GFP_ATOMIC);
		if (n == NULL) {
			vi_pr(VI_ERR, "SNR cfg node alloc size(%zu) fail\n", sizeof(*n));
			spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
			return;
		}
		memcpy(&n->n, &snr_node->snr_cfg_node.snsr, sizeof(struct snsr_regs_s));

		while (!list_empty(&isp_snr_i2c_queue[raw_num].list)
			&& (isp_snr_i2c_queue[raw_num].num_rdy >= (VI_MAX_LIST_NUM - 1))) {
			q = list_first_entry(&isp_snr_i2c_queue[raw_num].list, struct _isp_snr_i2c_node, list);
			list_del_init(&q->list);
			--isp_snr_i2c_queue[raw_num].num_rdy;
			kfree(q);
		}
		list_add_tail(&n->list, &isp_snr_i2c_queue[raw_num].list);
		++isp_snr_i2c_queue[raw_num].num_rdy;
	}

	spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
}

void sop_isp_rdy_buf_queue(struct sop_vi_dev *vdev, struct sop_isp_buf *b)
{
	unsigned long flags;
	u32 raw_num = (b->buf.index / VI_MAX_CHN_NUM) % VI_MAX_CHN_NUM;
	u32 chn_num = b->buf.index % VI_MAX_CHN_NUM;

	vi_pr(VI_DBG, "ISP buf_queue raw_num=%d chn_num=%d\n", raw_num, chn_num);

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	list_add_tail(&b->list, &vdev->qbuf_list[raw_num][chn_num]);
	++vdev->qbuf_num[raw_num][chn_num];
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);

	if (!atomic_read(&vdev->isp_streamoff) && vdev->qbuf_num[raw_num][chn_num] == 1) {
		if (vdev->ctx.isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_BYPASS)
			_isp_yuv_bypass_trigger(vdev, raw_num, ISP_FE_CH0);
		else
			_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
	}
}

struct sop_isp_buf *sop_isp_rdy_buf_next(struct sop_vi_dev *vdev, const u8 raw_num, const u8 chn_num)
{
	unsigned long flags;
	struct sop_isp_buf *b = NULL;

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	if (!list_empty(&vdev->qbuf_list[raw_num][chn_num]))
		b = list_first_entry(&vdev->qbuf_list[raw_num][chn_num], struct sop_isp_buf, list);
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);

	return b;
}

int sop_isp_rdy_buf_empty(struct sop_vi_dev *vdev, const u8 raw_num, const u8 chn_num)
{
	unsigned long flags;
	int empty = 0;

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	empty = (vdev->qbuf_num[raw_num][chn_num] == 0);
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);

	return empty;
}

void sop_isp_rdy_buf_pop(struct sop_vi_dev *vdev, const u8 raw_num, const u8 chn_num)
{
	unsigned long flags;

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	vdev->qbuf_num[raw_num][chn_num]--;
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);
}

void sop_isp_rdy_buf_remove(struct sop_vi_dev *vdev, const u8 raw_num, const u8 chn_num)
{
	unsigned long flags;
	struct sop_isp_buf *b = NULL;

	spin_lock_irqsave(&vdev->qbuf_lock, flags);
	if (!list_empty(&vdev->qbuf_list[raw_num][chn_num])) {
		b = list_first_entry(&vdev->qbuf_list[raw_num][chn_num], struct sop_isp_buf, list);
		list_del_init(&b->list);
		kfree(b);
	}
	spin_unlock_irqrestore(&vdev->qbuf_lock, flags);
}

void _vi_yuv_dma_setup(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	struct _membuf *pool = &isp_bufpool[raw_num];
	struct isp_buffer *b;
	struct isp_ctx *ictx = ctx;
	u64 bufaddr = 0;
	u32 bufsize = 0;
	u32 base = 0, dma = 0;
	u32 rgbmap_le = 0;
	u8 i = 0, j = 0;
	u8 total_chn = ctx->isp_pipe_cfg[raw_num].mux_mode + 1;

	if (ctx->isp_pipe_cfg[raw_num].is_bt_demux)
		base = csibdg_lite_dma_find_hwid(raw_num, ISP_FE_CH0);
	else
		base = csibdg_dma_find_hwid(raw_num, ISP_FE_CH0);

	for (i = 0; i < total_chn; i++) {
		dma = base + i;

		for (j = 0; j < OFFLINE_YUV_BUF_NUM; j++) {
			b = vmalloc(sizeof(*b));
			if (b == NULL) {
				vi_pr(VI_ERR, "yuv_buf isp_buf_%d vmalloc size(%zu) fail\n", j, sizeof(*b));
				return;
			}
			memset(b, 0, sizeof(*b));
			b->raw_num = raw_num;
			b->chn_num = i;
			bufsize = ispblk_dma_yuv_bypass_config(ctx, dma, 0, raw_num);
			pool->yuv_yuyv[i][j] = b->addr = _mempool_pop(bufsize);

			if (j == 0)
				ispblk_dma_setaddr(ctx, raw_num, dma, b->addr);

			isp_buf_queue(&pre_fe_out_q[raw_num][i], b);
		}
	}

	if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP &&
	    !ctx->isp_pipe_cfg[raw_num].is_bt_demux) {
		if (ctx->is_3dnr_on) {
			// rgbmap_le
			rgbmap_le = rgbmap_dma_find_hwid(raw_num, ISP_RAW_PATH_LE);
			DMA_SETUP_2(raw_num, rgbmap_le);
			isp_bufpool[raw_num].rgbmap_le[0] = bufaddr;
			ispblk_rgbmap_dma_config(ctx, raw_num, rgbmap_le);

			//Slice buffer mode use ring buffer
			if (!(_is_fe_be_online(ctx) && ctx->is_rgbmap_sbm_on)) {
				if (!_is_all_online(ctx)) {
					for (i = 1; i < RGBMAP_BUF_IDX; i++)
						isp_bufpool[raw_num].rgbmap_le[i] = _mempool_pop(bufsize);
				}
			}
		}
	}
}

static void _isp_preraw_fe_dma_dump(struct isp_ctx *ictx, enum sop_isp_raw raw_num)
{
	u8 i = 0;
	u8 pre_fe_buf_num = OFFLINE_RAW_BUF_NUM;
	char str[64] = "PRERAW_FE";

	//for fe ai isp, need more buffer
	if (ictx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_FE)
		pre_fe_buf_num += BNR_AI_ISP_BUF_NUM;

	vi_pr(VI_INFO, "***************%s_%d************************\n", str, raw_num);
	for (i = 0; i < pre_fe_buf_num; i++)
		vi_pr(VI_INFO, "pre_fe_le(0x%llx)\n", isp_bufpool[raw_num].pre_fe_le[i]);

	for (i = 0; i < RGBMAP_BUF_IDX; i++)
		vi_pr(VI_INFO, "rgbmap_le(0x%llx)\n", isp_bufpool[raw_num].rgbmap_le[i]);

	if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
		for (i = 0; i < pre_fe_buf_num; i++)
			vi_pr(VI_INFO, "pre_fe_se(0x%llx)\n", isp_bufpool[raw_num].pre_fe_se[i]);

		for (i = 0; i < RGBMAP_BUF_IDX; i++)
			vi_pr(VI_INFO, "rgbmap_se(0x%llx)\n", isp_bufpool[raw_num].rgbmap_se[i]);
	}

	if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
		ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
		for (i = 0; i < ISP_FE_CHN_MAX; i++) {
			vi_pr(VI_INFO, "yuyv_yuv(0x%llx), yuyv_yuv(0x%llx)\n",
				isp_bufpool[raw_num].yuv_yuyv[i][0], isp_bufpool[raw_num].yuv_yuyv[i][1]);
		}
	}
	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_preraw_be_dma_dump(struct isp_ctx *ictx)
{
	u8 i = 0;
	char str[64] = "PRERAW_BE";
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	u8 first_raw_num = vi_get_first_raw_num(ictx);

	vi_pr(VI_INFO, "***************%s************************\n", str);
	vi_pr(VI_INFO, "be_rdma_le(0x%llx)\n", isp_bufpool[first_raw_num].pre_fe_le[0]);
	for (i = 0; i < OFFLINE_PRE_BE_BUF_NUM; i++)
		vi_pr(VI_INFO, "be_wdma_le(0x%llx)\n", isp_bufpool[first_raw_num].pre_be_le[i]);

	if (ictx->is_hdr_on) {
		vi_pr(VI_INFO, "be_rdma_se(0x%llx)\n", isp_bufpool[first_raw_num].pre_fe_se[0]);
		for (i = 0; i < OFFLINE_PRE_BE_BUF_NUM; i++)
			vi_pr(VI_INFO, "be_wdma_se(0x%llx)\n", isp_bufpool[first_raw_num].pre_be_se[i]);
	}

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;

		vi_pr(VI_INFO, "***********************raw_num(%d)**********************\n", raw_num);
		vi_pr(VI_INFO, "af(0x%llx, 0x%llx)\n",
				isp_bufpool[raw_num].sts_mem[0].af.phy_addr,
				isp_bufpool[raw_num].sts_mem[1].af.phy_addr);

		for (i = 0; i < OFFLINE_PRE_BE_BUF_NUM; i++) {
			if (ictx->isp_pipe_cfg[raw_num].is_rgbir_sensor) {
				vi_pr(VI_INFO, "ir_le(0x%llx)\n", isp_bufpool[raw_num].ir_le[i]);
				if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
					vi_pr(VI_INFO, "ir_se(0x%llx)\n", isp_bufpool[raw_num].ir_se[i]);
				}
			}
		}
	}
	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_rawtop_dma_dump(struct isp_ctx *ictx)
{
	char str[64] = "RAW_TOP";
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	vi_pr(VI_INFO, "***************%s************************\n", str);
	vi_pr(VI_INFO, "rawtop_rdma_le(0x%llx)\n", isp_bufpool[raw_num].pre_be_le[0]);

	if (ictx->is_hdr_on)
		vi_pr(VI_INFO, "rawtop_rdma_se(0x%llx)\n", isp_bufpool[raw_num].pre_be_se[0]);

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;

		vi_pr(VI_INFO, "***********************raw_num(%d)**********************\n", raw_num);
		vi_pr(VI_INFO, "lsc(0x%llx)\n", isp_bufpool[raw_num].lsc);

		vi_pr(VI_INFO, "ae_le(0x%llx, 0x%llx)\n",
				isp_bufpool[raw_num].sts_mem[0].ae_le.phy_addr,
				isp_bufpool[raw_num].sts_mem[1].ae_le.phy_addr);
		vi_pr(VI_INFO, "gms(0x%llx, 0x%llx)\n",
				isp_bufpool[raw_num].sts_mem[0].gms.phy_addr,
				isp_bufpool[raw_num].sts_mem[1].gms.phy_addr);
		vi_pr(VI_INFO, "awb(0x%llx, 0x%llx)\n",
				isp_bufpool[raw_num].sts_mem[0].awb.phy_addr,
				isp_bufpool[raw_num].sts_mem[1].awb.phy_addr);
		vi_pr(VI_INFO, "lmap_le(0x%llx)\n", isp_bufpool[raw_num].lmap_le);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			vi_pr(VI_INFO, "ae_se(0x%llx, 0x%llx)\n",
					isp_bufpool[raw_num].sts_mem[0].ae_se.phy_addr,
					isp_bufpool[raw_num].sts_mem[1].ae_se.phy_addr);
			vi_pr(VI_INFO, "lmap_se(0x%llx)\n", isp_bufpool[raw_num].lmap_se);
		}
	}

	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_rgbtop_dma_dump(struct isp_ctx *ictx)
{
	char str[64] = "RGB_TOP";
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	vi_pr(VI_INFO, "***************%s************************\n", str);
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;

		vi_pr(VI_INFO, "***********************raw_num(%d)**********************\n", raw_num);
		vi_pr(VI_INFO, "hist_edge_v(0x%llx, 0x%llx)\n",
				isp_bufpool[raw_num].sts_mem[0].hist_edge_v.phy_addr,
				isp_bufpool[raw_num].sts_mem[1].hist_edge_v.phy_addr);
		vi_pr(VI_INFO, "manr(0x%llx, 0x%llx), manr_rtile(0x%llx, 0x%llx)\n",
				isp_bufpool[raw_num].manr[0],
				isp_bufpool[raw_num].manr[1],
				isp_bufpool[raw_num].manr_rtile[0],
				isp_bufpool[raw_num].manr_rtile[1]);
		vi_pr(VI_INFO, "tdnr(0x%llx, 0x%llx, 0x%llx), tdnr_rtile(0x%llx, 0x%llx, 0x%llx)\n",
				isp_bufpool[raw_num].tdnr[0],
				isp_bufpool[raw_num].tdnr[1],
				isp_bufpool[raw_num].tdnr[2],
				isp_bufpool[raw_num].tdnr_rtile[0],
				isp_bufpool[raw_num].tdnr_rtile[1],
				isp_bufpool[raw_num].tdnr_rtile[2]);
	}

	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_yuvtop_dma_dump(struct isp_ctx *ictx)
{
	char str[64] = "YUV_TOP";
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	u64 dci_bufaddr = 0;
	u64 ldci_bufaddr = 0;

	vi_pr(VI_INFO, "***************%s************************\n", str);
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;

		vi_pr(VI_INFO, "***********************raw_num(%d)**********************\n", raw_num);
		vi_pr(VI_INFO, "dci(0x%llx, 0x%llx)\n",
				isp_bufpool[raw_num].sts_mem[0].dci.phy_addr,
				isp_bufpool[raw_num].sts_mem[1].dci.phy_addr);
		vi_pr(VI_INFO, "ldci(0x%llx)\n", isp_bufpool[raw_num].ldci);

		// show wasted buf size for 256B-aligned ldci bufaddr
		dci_bufaddr = isp_bufpool[raw_num].sts_mem[1].dci.phy_addr;
		ldci_bufaddr = isp_bufpool[raw_num].ldci;
		vi_pr(VI_INFO, "ldci wasted_bufsize_for_alignment(%d)\n",
			(u32)(ldci_bufaddr - (dci_bufaddr + 0x200)));
	}
	vi_pr(VI_INFO, "*************************************************\n");
	vi_pr(VI_INFO, "VI total reserved memory(0x%x)\n", isp_mempool.byteused);
	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_manr_dma_setup(struct isp_ctx *ictx, enum sop_isp_raw raw_num) {
	u64 bufaddr = 0;
	u32 bufsize = 0;

	if (!ictx->is_3dnr_on)
		return;

	DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_MMAP_IIR_R);
	isp_bufpool[raw_num].manr[0] = bufaddr;
	ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_IIR_W, isp_bufpool[raw_num].manr[0]);

	isp_bufpool[raw_num].sts_mem[0].mmap.phy_addr =
		isp_bufpool[raw_num].sts_mem[1].mmap.phy_addr = bufaddr;
	isp_bufpool[raw_num].sts_mem[0].mmap.size =
		isp_bufpool[raw_num].sts_mem[1].mmap.size = bufsize;

	if (ictx->isp_pipe_cfg[raw_num].is_tile) { //iir right tile
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1, ISP_BLK_ID_DMA_CTL_MMAP_IIR_R);
		isp_bufpool[raw_num].manr_rtile[0] = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[0].mmap.size =
			isp_bufpool[raw_num].sts_mem[1].mmap.size += bufsize;
	}

	if (ictx->isp_pipe_cfg[raw_num].is_tnr_ai_isp) {
		DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_MMAP_AI_ISP);
		isp_bufpool[raw_num].manr[1] = bufaddr;
	}

	if (_is_all_online(ictx)) {
		ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R,
							isp_bufpool[raw_num].rgbmap_le[0]);
	} else {
		ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R,
							isp_bufpool[raw_num].rgbmap_le[0]);
		if (_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)
			ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R,
						isp_bufpool[raw_num].rgbmap_le[0]);
		else
			ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R,
						isp_bufpool[raw_num].rgbmap_le[1]);
	}

	if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R,
					isp_bufpool[raw_num].rgbmap_se[0]);
		if (_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)
			ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R,
						isp_bufpool[raw_num].rgbmap_se[0]);
		else
			ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R,
						isp_bufpool[raw_num].rgbmap_se[1]);
	}

	// tnr motion
	DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_MO);
	isp_bufpool[raw_num].tdnr[0] = bufaddr;
	ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_MO, bufaddr);

	if (ictx->isp_pipe_cfg[raw_num].is_tile) { //tnr motion right tile
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1, ISP_BLK_ID_DMA_CTL_TNR_ST_MO);
		isp_bufpool[raw_num].tdnr_rtile[0] = bufaddr;
	}

	// tnr y/uv
	if (ictx->is_fbc_on) {
		u64 bufaddr_tmp = 0;

		bufaddr_tmp = _mempool_get_addr();
		//ring buffer constraint. reg_base is 4k byte-align
		bufaddr = VI_4K_ALIGN(bufaddr_tmp);
		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_Y, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_Y);
		_mempool_pop(bufsize + (u32)(bufaddr - bufaddr_tmp));

		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_Y, bufaddr);
		isp_bufpool[raw_num].tdnr[1] = bufaddr;

		bufaddr_tmp = _mempool_get_addr();
		//ring buffer constraint. reg_base is 4k byte-align
		bufaddr = VI_4K_ALIGN(bufaddr_tmp);
		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_C, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_C);
		_mempool_pop(bufsize + (u32)(bufaddr - bufaddr_tmp));

		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_C, bufaddr);
		isp_bufpool[raw_num].tdnr[2] = bufaddr;
	} else {
		DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_Y);
		ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_Y, bufaddr);
		isp_bufpool[raw_num].tdnr[1] = bufaddr;

		DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_C);
		ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_C, bufaddr);
		isp_bufpool[raw_num].tdnr[2] = bufaddr;

		if (ictx->isp_pipe_cfg[raw_num].is_tile) {
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1,
					ISP_BLK_ID_DMA_CTL_TNR_LD_Y);
			isp_bufpool[raw_num].tdnr_rtile[1] = _mempool_pop(bufsize);

			bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1,
					ISP_BLK_ID_DMA_CTL_TNR_ST_C);
			isp_bufpool[raw_num].tdnr_rtile[2] = _mempool_pop(bufsize);
		}

		if (ictx->isp_pipe_cfg[raw_num].is_tnr_ai_isp) {
			DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_Y);
			isp_bufpool[raw_num].tnr_ai_isp[0] = bufaddr;

			DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_U);
			isp_bufpool[raw_num].tnr_ai_isp[1] = bufaddr;

			DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_V);
			isp_bufpool[raw_num].tnr_ai_isp[2] = bufaddr;
		}
	}
}

void _isp_splt_dma_setup(struct isp_ctx *ictx, enum sop_isp_raw raw_num)
{
	u64 dma_addr = 0;
	u64 bufaddr = 0;
	u32 bufsize = 0;
	u32 splt_fe0_le = ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_LE;
	u32 splt_fe0_se = ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_SE;
	u32 splt_fe1_le = ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_LE;
	u32 splt_fe1_se = ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_SE;
	u8  i = 0;
	struct isp_buffer *b;
	enum sop_isp_fe_chn_num chn_num = ISP_FE_CH0;

	if (raw_num != ISP_PRERAW0)
		return;

	if (ictx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT) {
		chn_num = ISP_FE_CH0;
		for (i = 0; i < OFFLINE_SPLT_BUF_NUM; i++) {
			DMA_SETUP_2(raw_num, splt_fe0_le);
			b = vmalloc(sizeof(*b));
			if (b == NULL) {
				vi_pr(VI_ERR, "splt_fe%d_le isp_buf_%d vmalloc size(%zu) fail\n",
						raw_num, i, sizeof(*b));
				return;
			}
			memset(b, 0, sizeof(*b));
			b->addr = bufaddr;
			b->raw_num = raw_num;
			b->chn_num = chn_num;
			b->ir_idx = i;
			isp_bufpool[raw_num].splt_le[i] = b->addr;
			isp_buf_queue(&splt_out_q[raw_num][chn_num], b);
		}
		ispblk_dma_config(ictx, raw_num, splt_fe0_le, isp_bufpool[raw_num].splt_le[0]);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			chn_num = ISP_FE_CH1;
			for (i = 0; i < OFFLINE_SPLT_BUF_NUM; i++) {
				DMA_SETUP_2(raw_num, splt_fe0_se);
				b = vmalloc(sizeof(*b));
				if (b == NULL) {
					vi_pr(VI_ERR, "splt_fe%d_se isp_buf_%d vmalloc size(%zu) fail\n",
							raw_num, i, sizeof(*b));
					return;
				}
				memset(b, 0, sizeof(*b));
				b->addr = bufaddr;
				b->raw_num = raw_num;
				b->chn_num = chn_num;
				b->ir_idx = i;
				isp_bufpool[raw_num].splt_se[i] = b->addr;
				isp_buf_queue(&splt_out_q[raw_num][chn_num], b);
			}
			ispblk_dma_config(ictx, raw_num, splt_fe0_se, isp_bufpool[raw_num].splt_se[0]);
		}

		if (ictx->isp_pipe_cfg[raw_num].is_tile) {
			dma_addr = isp_bufpool[raw_num].splt_le[0];
			dma_addr += (ictx->isp_pipe_cfg[raw_num].csibdg_width * 3) / 2;
			ispblk_dma_config(ictx, raw_num + 1, splt_fe1_le, dma_addr);

			if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
				dma_addr = isp_bufpool[raw_num].splt_se[0];
				dma_addr += (ictx->isp_pipe_cfg[raw_num].csibdg_width * 3) / 2;
				ispblk_dma_config(ictx, raw_num + 1, splt_fe1_se, dma_addr);
			}
		}
	}
}

void _isp_pre_fe_dma_setup(struct isp_ctx *ictx, enum sop_isp_raw raw_num)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;
	u8  i = 0;
	u8  pre_fe_buf_num = OFFLINE_RAW_BUF_NUM;
	struct isp_buffer *b;

	u32 raw_le, raw_se;
	u32 rgbmap_le, rgbmap_se;

	if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //YUV sensor
		if (ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) //yuv fe->dram->post
			_vi_yuv_dma_setup(ictx, raw_num);

		goto EXIT;
	}

	raw_le = csibdg_dma_find_hwid(raw_num, ISP_FE_CH0);
	raw_se = csibdg_dma_find_hwid(raw_num, ISP_FE_CH1);
	rgbmap_le = rgbmap_dma_find_hwid(raw_num, ISP_RAW_PATH_LE);
	rgbmap_se = rgbmap_dma_find_hwid(raw_num, ISP_RAW_PATH_SE);

	if (_is_be_post_online(ictx) && !ictx->isp_pipe_cfg[raw_num].is_raw_replay_be) { //fe->dram->be->post
		//for ai isp, need more buffer
		if (ictx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_FE)
			pre_fe_buf_num += BNR_AI_ISP_BUF_NUM;

		for (i = 0; i < pre_fe_buf_num; i++) {
			DMA_SETUP_2(raw_num, raw_le);
			b = vmalloc(sizeof(*b));
			if (b == NULL) {
				vi_pr(VI_ERR, "raw_le isp_buf_%d vmalloc size(%zu) fail\n", i, sizeof(*b));
				return;
			}
			memset(b, 0, sizeof(*b));
			b->addr = bufaddr;
			b->raw_num = raw_num;
			b->chn_num = ISP_FE_CH0;
			b->ir_idx = i;
			b->is_ext = INTERNAL_BUFFER;
			isp_bufpool[raw_num].pre_fe_le[i] = b->addr;
			isp_buf_queue(&pre_fe_out_q[raw_num][ISP_FE_CH0], b);
		}
		ispblk_dma_config(ictx, raw_num, raw_le, isp_bufpool[raw_num].pre_fe_le[0]);
		if (ictx->isp_pipe_cfg[raw_num].is_tile) {
			raw_le = csibdg_dma_find_hwid(raw_num + 1, ISP_FE_CH0);
			bufaddr = isp_bufpool[raw_num].pre_fe_le[0] +
					3 * UPPER(ictx->isp_pipe_cfg[raw_num].crop.w, 1);
			ispblk_dma_config(ictx, raw_num + 1, raw_le, bufaddr);
		}

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			for (i = 0; i < pre_fe_buf_num; i++) {
				DMA_SETUP_2(raw_num, raw_se);
				b = vmalloc(sizeof(*b));
				if (b == NULL) {
					vi_pr(VI_ERR, "raw_se isp_buf_%d vmalloc size(%zu) fail\n", i, sizeof(*b));
					return;
				}
				memset(b, 0, sizeof(*b));
				b->addr = bufaddr;
				b->raw_num = raw_num;
				b->chn_num = ISP_FE_CH1;
				b->ir_idx = i;
				b->is_ext = INTERNAL_BUFFER;
				isp_bufpool[raw_num].pre_fe_se[i] = b->addr;
				isp_buf_queue(&pre_fe_out_q[raw_num][ISP_FE_CH1], b);
			}
			ispblk_dma_config(ictx, raw_num, raw_se, isp_bufpool[raw_num].pre_fe_se[0]);
			if (ictx->isp_pipe_cfg[raw_num].is_tile) {
				raw_se = csibdg_dma_find_hwid(raw_num + 1, ISP_FE_CH1);
				bufaddr = isp_bufpool[raw_num].pre_fe_se[0] +
						3 * UPPER(ictx->isp_pipe_cfg[raw_num].crop_se.w, 1);
				ispblk_dma_config(ictx, raw_num + 1, raw_se, bufaddr);
			}
		}
	}

	if (ictx->is_3dnr_on) {
		// rgbmap_le
		DMA_SETUP_2(raw_num, rgbmap_le);
		isp_bufpool[raw_num].rgbmap_le[0] = bufaddr;
		ispblk_rgbmap_dma_config(ictx, raw_num, rgbmap_le);
		if (ictx->isp_pipe_cfg[raw_num].is_tile) {
			u32 grid_size = (1 << g_w_bit[raw_num]);
			u32 w = ictx->isp_pipe_cfg[raw_num].crop.w;

			rgbmap_le = rgbmap_dma_find_hwid(raw_num + 1, ISP_RAW_PATH_LE);
			bufaddr += ((w + grid_size - 1) / grid_size) * 6;
			ispblk_dma_setaddr(ictx, raw_num, rgbmap_le, bufaddr);
			ispblk_rgbmap_dma_config(ictx, raw_num + 1, rgbmap_le);
		}
		//Slice buffer mode use ring buffer
		if (!(_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)) {
			if (!_is_all_online(ictx)) {
				for (i = 1; i < RGBMAP_BUF_IDX; i++)
					isp_bufpool[raw_num].rgbmap_le[i] = _mempool_pop(bufsize);
			}
		}

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			// rgbmap se
			DMA_SETUP_2(raw_num, rgbmap_se);
			isp_bufpool[raw_num].rgbmap_se[0] = bufaddr;
			ispblk_rgbmap_dma_config(ictx, raw_num, rgbmap_se);
			if (ictx->isp_pipe_cfg[raw_num].is_tile) {
				u32 grid_size = (1 << g_w_bit[raw_num]);
				u32 w = ictx->isp_pipe_cfg[raw_num].crop.w;

				rgbmap_se = rgbmap_dma_find_hwid(raw_num + 1, ISP_RAW_PATH_SE);
				bufaddr += ((w + grid_size - 1) / grid_size) * 6;
				ispblk_dma_setaddr(ictx, raw_num, rgbmap_se, bufaddr);
				ispblk_rgbmap_dma_config(ictx, raw_num + 1, rgbmap_se);
			}

			//Slice buffer mode use ring buffer
			if (!(_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)) {
				for (i = 1; i < RGBMAP_BUF_IDX; i++)
					isp_bufpool[raw_num].rgbmap_se[i] = _mempool_pop(bufsize);
			}
		}
	}

EXIT:
	_isp_preraw_fe_dma_dump(ictx, raw_num);
}

void _isp_pre_be_dma_setup(struct isp_ctx *ictx)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;
	u8  buf_num = 0;
	struct isp_buffer *b;

	enum sop_isp_raw raw_num = ISP_PRERAW0;
	u8 first_raw_num = vi_get_first_raw_num(ictx);
	u8 cfg_dma = false;

	//RGB path
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (!ictx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			cfg_dma = true;
			break;
		}
	}

	if (cfg_dma == false)
		goto EXIT;

	if (ictx->is_offline_be && !ictx->isp_pipe_cfg[first_raw_num].is_raw_replay_be) {
		//apply pre_fe_le buffer
		ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE,
					isp_bufpool[first_raw_num].pre_fe_le[0]);

		if (ictx->is_hdr_on) {
			//apply pre_fe_se buffer
			ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE,
						isp_bufpool[first_raw_num].pre_fe_se[0]);
		}
	}

	if (_is_fe_be_online(ictx)) { //fe->be->dram->post
		if (ictx->is_slice_buf_on) {
			DMA_SETUP_2(first_raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE);
			isp_bufpool[first_raw_num].pre_be_le[0] = bufaddr;

			if (ictx->isp_pipe_cfg[first_raw_num].is_hdr_on) {
				DMA_SETUP_2(first_raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE);
				isp_bufpool[first_raw_num].pre_be_se[0] = bufaddr;
			}
		} else {
			for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
				DMA_SETUP_2(first_raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE);
				b = vmalloc(sizeof(*b));
				if (b == NULL) {
					vi_pr(VI_ERR, "be_wdma_le isp_buf_%d vmalloc size(%zu) fail\n",
						buf_num, sizeof(*b));
					return;
				}
				memset(b, 0, sizeof(*b));
				b->addr = bufaddr;
				b->raw_num = first_raw_num;
				b->ir_idx = buf_num;
				b->is_ext = INTERNAL_BUFFER;
				isp_bufpool[first_raw_num].pre_be_le[buf_num] = b->addr;
				isp_buf_queue(&pre_be_out_q[ISP_RAW_PATH_LE], b);
			}

			ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE,
						isp_bufpool[first_raw_num].pre_be_le[0]);

			if (ictx->isp_pipe_cfg[first_raw_num].is_hdr_on) {
				for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
					DMA_SETUP_2(first_raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE);
					b = vmalloc(sizeof(*b));
					if (b == NULL) {
						vi_pr(VI_ERR, "be_wdma_se isp_buf_%d vmalloc size(%zu) fail\n",
								buf_num, sizeof(*b));
						return;
					}
					memset(b, 0, sizeof(*b));
					b->addr = bufaddr;
					b->raw_num = first_raw_num;
					b->ir_idx = buf_num;
					b->is_ext = INTERNAL_BUFFER;
					isp_bufpool[first_raw_num].pre_be_se[buf_num] = b->addr;
					isp_buf_queue(&pre_be_out_q[ISP_RAW_PATH_SE], b);
				}

				ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE,
							isp_bufpool[first_raw_num].pre_be_se[0]);
			}
		}
	}

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		//Be out dma
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_ISP)
			continue;

		// af
		DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_AF_W);
		isp_bufpool[raw_num].sts_mem[0].af.phy_addr = bufaddr;
		isp_bufpool[raw_num].sts_mem[0].af.size = bufsize;
		isp_bufpool[raw_num].sts_mem[1].af.phy_addr = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[1].af.size = bufsize;
		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_AF_W, isp_bufpool[raw_num].sts_mem[0].af.phy_addr);

		// rgbir
		if (ictx->isp_pipe_cfg[raw_num].is_rgbir_sensor) {
			for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
				DMA_SETUP(raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_LE);
				isp_bufpool[raw_num].ir_le[buf_num] = bufaddr;
			}
			ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_LE, isp_bufpool[raw_num].ir_le[0]);

			if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
				for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
					DMA_SETUP(raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_SE);
					isp_bufpool[raw_num].ir_se[buf_num] = bufaddr;
				}
				ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_SE, isp_bufpool[raw_num].ir_se[0]);
			}
		}
	}
EXIT:
	_isp_preraw_be_dma_dump(ictx);
}

void _isp_rawtop_dma_setup(struct isp_ctx *ictx)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;

	enum sop_isp_raw raw_num = ISP_PRERAW0;

	u8 cfg_dma = false;

	//RGB path
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (!ictx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			cfg_dma = true;
			break;
		}
	}

	if (cfg_dma == false) //YUV sensor only
		goto EXIT;

	if (_is_fe_be_online(ictx)) { //fe->be->dram->post
		u8 first_raw_num = vi_get_first_raw_num(ictx);

		ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_RAW_RDMA0,
					isp_bufpool[first_raw_num].pre_be_le[0]);
		if (ictx->is_hdr_on) {
			ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_RAW_RDMA1,
						isp_bufpool[first_raw_num].pre_be_se[0]);
		}
	}

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_ISP) //YUV sensor
			continue;

		// lsc
		DMA_SETUP(raw_num, ISP_BLK_ID_DMA_CTL_LSC_LE);
		isp_bufpool[raw_num].lsc = bufaddr;

		// left tile and right tile use the same lsc addr
		if (ictx->isp_pipe_cfg[raw_num].is_tile) {
			isp_bufpool[raw_num + 1].lsc = bufaddr;
		}

		// gms
		bufaddr = _mempool_get_addr();
		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_GMS, bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_GMS);
		_mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[0].gms.phy_addr = bufaddr;
		isp_bufpool[raw_num].sts_mem[0].gms.size = bufsize;
		isp_bufpool[raw_num].sts_mem[1].gms.phy_addr = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[1].gms.size = bufsize;

		// lmap_le
		DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_LMAP_LE);
		isp_bufpool[raw_num].lmap_le = bufaddr;
		if (ictx->isp_pipe_cfg[raw_num].is_tile) {
			bufaddr = _mempool_get_addr();
			_mempool_pop(bufsize);
			isp_bufpool[raw_num + 1].lmap_le = bufaddr;
		}

		// ae le
		DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_AE_HIST_LE);
		isp_bufpool[raw_num].sts_mem[0].ae_le.phy_addr = bufaddr;
		isp_bufpool[raw_num].sts_mem[0].ae_le.size = bufsize;
		isp_bufpool[raw_num].sts_mem[1].ae_le.phy_addr = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[1].ae_le.size = bufsize;
		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_AE_HIST_LE,
				isp_bufpool[raw_num].sts_mem[0].ae_le.phy_addr);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			// lsc_se
			ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LSC_SE, isp_bufpool[raw_num].lsc);

			// lmap_se
			DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_LMAP_SE);
			isp_bufpool[raw_num].lmap_se = bufaddr;
			if (ictx->isp_pipe_cfg[raw_num].is_tile) {
				bufaddr = _mempool_get_addr();
				_mempool_pop(bufsize);
				isp_bufpool[raw_num + 1].lmap_se = bufaddr;
			}

			// ae se
			DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_AE_HIST_SE);
			isp_bufpool[raw_num].sts_mem[0].ae_se.phy_addr = bufaddr;
			isp_bufpool[raw_num].sts_mem[0].ae_se.size = bufsize;
			isp_bufpool[raw_num].sts_mem[1].ae_se.phy_addr = _mempool_pop(bufsize);
			isp_bufpool[raw_num].sts_mem[1].ae_se.size = bufsize;
			ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_AE_HIST_SE,
				isp_bufpool[raw_num].sts_mem[0].ae_se.phy_addr);
		}
	}

EXIT:
	_isp_rawtop_dma_dump(ictx);
}

void _isp_rgbtop_dma_setup(struct isp_ctx *ictx)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;

	enum sop_isp_raw raw_num = ISP_PRERAW0;

	u8 cfg_dma = false;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;

		cfg_dma = true;
		break;
	}

	if (cfg_dma == false) //No pipe need to setup
		goto EXIT;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
		    ictx->isp_pipe_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_ISP)
			continue;

		if (!ictx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			// hist_edge_v
			DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_HIST_EDGE_V);
			isp_bufpool[raw_num].sts_mem[0].hist_edge_v.phy_addr = bufaddr;
			isp_bufpool[raw_num].sts_mem[0].hist_edge_v.size = bufsize;
			isp_bufpool[raw_num].sts_mem[1].hist_edge_v.phy_addr = _mempool_pop(bufsize);
			isp_bufpool[raw_num].sts_mem[1].hist_edge_v.size = bufsize;

			// ltm dma
			ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LTM_LE, isp_bufpool[raw_num].lmap_le);

			if (ictx->isp_pipe_cfg[raw_num].is_hdr_on)
				ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LTM_SE, isp_bufpool[raw_num].lmap_se);
			else
				ispblk_dma_setaddr(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LTM_SE, isp_bufpool[raw_num].lmap_le);
		}

		// manr
		if (ictx->is_3dnr_on) {
			_isp_manr_dma_setup(ictx, raw_num);
		}
	}
EXIT:
	_isp_rgbtop_dma_dump(ictx);
}

void _isp_yuvtop_dma_setup(struct isp_ctx *ictx)
{
	u64 bufaddr = 0;
	u64 tmp_bufaddr = 0;
	u32 bufsize = 0;

	enum sop_isp_raw raw_num = ISP_PRERAW0;

	u8 cfg_dma = false;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (!ictx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			cfg_dma = true;
			break;
		}
	}

	if (cfg_dma == false)
		goto EXIT;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_ISP)
			continue;

		// dci
		DMA_SETUP_2(raw_num, ISP_BLK_ID_DMA_CTL_DCI);
		isp_bufpool[raw_num].sts_mem[0].dci.phy_addr = bufaddr;
		isp_bufpool[raw_num].sts_mem[0].dci.size = bufsize;
		isp_bufpool[raw_num].sts_mem[1].dci.phy_addr = _mempool_pop(bufsize);
		isp_bufpool[raw_num].sts_mem[1].dci.size = bufsize;
		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_DCI,
				isp_bufpool[raw_num].sts_mem[0].dci.phy_addr);

		// ldci
		//DMA_SETUP(ISP_BLK_ID_DMA_CTL_LDCI_W);
		//addr 256B alignment workaround
		tmp_bufaddr = _mempool_get_addr();
		bufaddr = VI_256_ALIGN(tmp_bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LDCI_W);
		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LDCI_W, bufaddr);
		_mempool_pop(bufsize + (u32)(bufaddr - tmp_bufaddr));

		isp_bufpool[raw_num].ldci = bufaddr;
		ispblk_dma_config(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LDCI_R, isp_bufpool[raw_num].ldci);
		if (ictx->isp_pipe_cfg[raw_num].is_tile) {
			tmp_bufaddr = _mempool_get_addr();
			bufaddr = VI_256_ALIGN(tmp_bufaddr);
			_mempool_pop(bufsize + (u32)(bufaddr - tmp_bufaddr));
			isp_bufpool[raw_num + 1].ldci = bufaddr;
		}
	}

	if (cfg_dma) {
		if (ictx->isp_pipe_cfg[ISP_PRERAW0].is_offline_scaler ||
			(ictx->is_multi_sensor && ictx->isp_pipe_cfg[ISP_PRERAW1].is_offline_scaler)) {
			//SW workaround. Need to set y/uv dma_disable = 1 before csibdg enable
			if (_is_be_post_online(ictx) && !ictx->isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_be) {
				ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, 1, 1);
				ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, 1, 1);
			} else {
				ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, 1, 0);
				ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, 1, 0);
			}
		} else {
			ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, 0, 0);
			ispblk_dma_enable(ictx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, 0, 0);
		}
	}

EXIT:
	_isp_yuvtop_dma_dump(ictx);
}

void _isp_cmdq_dma_setup(struct isp_ctx *ictx)
{
	u64 bufaddr = 0;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		bufaddr = _mempool_get_addr();
		ictx->isp_pipe_cfg[raw_num].cmdq_buf.buf_size = VI_CMDQ_BUF_SIZE;
		ictx->isp_pipe_cfg[raw_num].cmdq_buf.phy_addr = bufaddr;
		ictx->isp_pipe_cfg[raw_num].cmdq_buf.vir_addr = phys_to_virt(bufaddr);
		_mempool_pop(VI_CMDQ_BUF_SIZE);
	}
}

void _vi_dma_setup(struct isp_ctx *ictx)
{
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		_isp_splt_dma_setup(ictx, raw_num);
		_isp_pre_fe_dma_setup(ictx, raw_num);
	}

	_isp_pre_be_dma_setup(ictx);
	_isp_rawtop_dma_setup(ictx);
	_isp_rgbtop_dma_setup(ictx);
	_isp_yuvtop_dma_setup(ictx);
	_isp_cmdq_dma_setup(ictx);
}

void _vi_dma_set_sw_mode(struct isp_ctx *ctx)
{
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_LE, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_SE, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_LE, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_SE, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_LE, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_SE, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_LE, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_SE, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE, ctx->is_tile ? true : false);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE1_RGBMAP_LE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE1_RGBMAP_SE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE2_RGBMAP_LE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE2_RGBMAP_SE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE3_RGBMAP_LE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE3_RGBMAP_SE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE4_RGBMAP_LE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE4_RGBMAP_SE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE5_RGBMAP_LE);
	ispblk_rgbmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_FE5_RGBMAP_SE);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LMAP_LE, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LMAP_SE, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG0, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG1, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG2, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG3, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI1_BDG0, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI1_BDG1, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI1_BDG2, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI1_BDG3, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI2_BDG0, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI2_BDG1, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI3_BDG0, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI3_BDG1, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI4_BDG0, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI4_BDG1, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI5_BDG0, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_CSI5_BDG1, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT0_LITE0, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT0_LITE1, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT0_LITE2, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT0_LITE3, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT1_LITE0, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT1_LITE1, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT1_LITE2, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_BT1_LITE3, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_RGBIR_LE, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_RGBIR_SE, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_AF_W, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LSC_LE, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LSC_SE, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_GMS, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_AE_HIST_LE, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_AE_HIST_SE, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA0, ctx->is_slice_buf_on ? false : true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA1, ctx->is_slice_buf_on ? false : true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_HIST_EDGE_V, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, ctx->is_tile ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, ctx->is_tile ? true : false);
	ispblk_mmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R);
	ispblk_mmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R);
	ispblk_mmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R);
	ispblk_mmap_dma_mode(ctx, ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_MMAP_IIR_R, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_MMAP_IIR_W, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_MMAP_AI_ISP, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_MO, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_LD_MO, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_Y, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_C, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_LD_Y, ctx->is_fbc_on ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_LD_C, ctx->is_fbc_on ? true : false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_Y, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_U, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_V, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_DCI, true);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LTM_LE, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LTM_SE, false);
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LDCI_W, true); //ldci_iir_w
	ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_LDCI_R, true); //ldci_iir_r
}

void _vi_yuv_get_dma_size(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	u32 bufsize = 0;
	u32 base = 0, dma = 0;
	u32 rgbmap_le = 0;
	u8 i = 0, j = 0;
	u8 chn_str = ctx->raw_chnstr_num[raw_num];
	u8 total_chn = chn_str + ctx->isp_pipe_cfg[raw_num].mux_mode + 1;

	if (ctx->isp_pipe_cfg[raw_num].is_bt_demux)
		base = csibdg_lite_dma_find_hwid(raw_num, ISP_FE_CH0);
	else
		base = csibdg_dma_find_hwid(raw_num, ISP_FE_CH0);

	for (i = chn_str; i < total_chn; i++) {
		dma = base + i - chn_str;

		for (j = 0; j < OFFLINE_YUV_BUF_NUM; j++) {
			bufsize = ispblk_dma_yuv_bypass_config(ctx, dma, 0, raw_num);
			_mempool_pop(bufsize);
		}
	}

	if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP &&
	    !ctx->isp_pipe_cfg[raw_num].is_bt_demux) {
		if (ctx->is_3dnr_on) {
			// rgbmap le
			rgbmap_le = rgbmap_dma_find_hwid(raw_num, ISP_RAW_PATH_LE);
			bufsize = ispblk_dma_buf_get_size(ctx, raw_num, rgbmap_le);
			_mempool_pop(bufsize);

			//Slice buffer mode use ring buffer
			if (!(_is_fe_be_online(ctx) && ctx->is_rgbmap_sbm_on)) {
				if (!_is_all_online(ctx)) {
					for (i = 1; i < RGBMAP_BUF_IDX; i++)
						_mempool_pop(bufsize);
				}
			}
		}
	}
}

void _vi_splt_get_dma_size(struct isp_ctx *ictx, enum sop_isp_raw raw_num)
{
	u32 bufsize = 0;
	u32 splt_le = 0, splt_se = 0;
	u8  i = 0;

	if (raw_num != ISP_PRERAW0)
		return;

	if (ictx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT) {
		splt_le = ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_LE;
		splt_se = ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_SE;

		for (i = 0; i < OFFLINE_SPLT_BUF_NUM; i++) {
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num, splt_le);
			_mempool_pop(bufsize);
		}

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			for (i = 0; i < OFFLINE_SPLT_BUF_NUM; i++) {
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num, splt_se);
				_mempool_pop(bufsize);
			}
		}
	}
}

void _vi_pre_fe_get_dma_size(struct isp_ctx *ictx, enum sop_isp_raw raw_num)
{
	u32 bufsize = 0;
	u8  i = 0;
	u8  pre_fe_buf_num = OFFLINE_RAW_BUF_NUM;
	u32 raw_le, raw_se;
	u32 rgbmap_le, rgbmap_se;

	raw_le = csibdg_dma_find_hwid(raw_num, ISP_FE_CH0);
	raw_se = csibdg_dma_find_hwid(raw_num, ISP_FE_CH1);
	rgbmap_le = rgbmap_dma_find_hwid(raw_num, ISP_RAW_PATH_LE);
	rgbmap_se = rgbmap_dma_find_hwid(raw_num, ISP_RAW_PATH_SE);

	if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //YUV sensor
		if (ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) //Online mode to scaler
			_vi_yuv_get_dma_size(ictx, raw_num);

		goto EXIT;
	}

	if (_is_be_post_online(ictx)) { //fe->dram->be->post
		//for fe ai isp, need more buffer
		if (ictx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_FE)
			pre_fe_buf_num += BNR_AI_ISP_BUF_NUM;

		for (i = 0; i < pre_fe_buf_num; i++) {
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num, raw_le);
			_mempool_pop(bufsize);
		}

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			for (i = 0; i < pre_fe_buf_num; i++) {
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num, raw_se);
				_mempool_pop(bufsize);
			}
		}
	}

	// rgbmap le
	bufsize = ispblk_dma_buf_get_size(ictx, raw_num, rgbmap_le);
	_mempool_pop(bufsize);

	//Slice buffer mode use ring buffer
	if (!(_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)) {
		if (!_is_all_online(ictx)) {
			for (i = 1; i < RGBMAP_BUF_IDX; i++)
				_mempool_pop(bufsize);
		}
	}

	if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
		// rgbmap se
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, rgbmap_se);
		_mempool_pop(bufsize);

		//Slice buffer mode use ring buffer
		if (!(_is_fe_be_online(ictx) && ictx->is_rgbmap_sbm_on)) {
			for (i = 1; i < RGBMAP_BUF_IDX; i++)
				_mempool_pop(bufsize);
		}
	}
EXIT:
	return;
}

void _vi_pre_be_get_dma_size(struct isp_ctx *ictx)
{
	u32 bufsize = 0;
	u8  buf_num = 0;

	enum sop_isp_raw raw_num = ISP_PRERAW0;

	u8 cfg_dma = false;

	//RGB path
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (!ictx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			cfg_dma = true;
			break;
		}
	}

	if (cfg_dma == false)
		goto EXIT;

	if (_is_fe_be_online(ictx)) { //fe->be->dram->post
		raw_num = vi_get_first_raw_num(ictx);

		if (ictx->is_slice_buf_on) {
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num,
					ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE);
			_mempool_pop(bufsize);

			if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num,
						ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE);
				_mempool_pop(bufsize);
			}
		} else {
			for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num,
						ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE);
				_mempool_pop(bufsize);
			}

			if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
				for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
					bufsize = ispblk_dma_buf_get_size(ictx, raw_num,
							ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE);
					_mempool_pop(bufsize);
				}
			}
		}
	}

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		//Be out dma
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_ISP)
			continue;

		// af
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_AF_W);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// rgbir
		if (ictx->isp_pipe_cfg[raw_num].is_rgbir_sensor) {
			for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_LE);
				_mempool_pop(bufsize);
			}

			if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
				for (buf_num = 0; buf_num < OFFLINE_PRE_BE_BUF_NUM; buf_num++) {
					bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_SE);
					_mempool_pop(bufsize);
				}
			}
		}
	}
EXIT:
	return;
}

void _vi_rawtop_get_dma_size(struct isp_ctx *ictx)
{
	u32 bufsize = 0;

	enum sop_isp_raw raw_num = ISP_PRERAW0;

	u8 cfg_dma = false;

	//RGB path
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (!ictx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			cfg_dma = true;
			break;
		}
	}

	if (cfg_dma == false) //YUV sensor only
		goto EXIT;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_ISP) //YUV sensor
			continue;

		// lsc
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LSC_LE);
		_mempool_pop(bufsize);

		// gms
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_GMS);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		// lmap_le
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LMAP_LE);
		_mempool_pop(bufsize);
		if (ictx->isp_pipe_cfg[raw_num].is_tile) {
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1, ISP_BLK_ID_DMA_CTL_LMAP_LE);
			_mempool_pop(bufsize);
		}

		// ae le
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_AE_HIST_LE);
		_mempool_pop(bufsize);
		_mempool_pop(bufsize);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			// lmap_se
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LMAP_SE);
			_mempool_pop(bufsize);
			if (ictx->isp_pipe_cfg[raw_num].is_tile) {
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1, ISP_BLK_ID_DMA_CTL_LMAP_SE);
				_mempool_pop(bufsize);
			}

			// ae se
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_AE_HIST_SE);
			_mempool_pop(bufsize);
			_mempool_pop(bufsize);
		}
	}
EXIT:
	return;
}

void _vi_rgbtop_get_dma_size(struct isp_ctx *ictx)
{
	u32 bufsize = 0;

	enum sop_isp_raw raw_num = ISP_PRERAW0;

	u8 cfg_dma = false;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;

		cfg_dma = true;
		break;
	}

	if (cfg_dma == false) //No pipe need to setup
		goto EXIT;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_ISP) //YUV sensor
			continue;

		if (!ictx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			// hist_edge_v
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_HIST_EDGE_V);
			_mempool_pop(bufsize);
			_mempool_pop(bufsize);
		}

		// manr
		if (ictx->is_3dnr_on) {
			u64 bufaddr = 0;
			u64 tmp_bufaddr = 0;

			// MANR M + H
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_IIR_R);
			_mempool_pop(bufsize);
			if (ictx->isp_pipe_cfg[raw_num].is_tile) {
				// MANR M + H right tile
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1, ISP_BLK_ID_DMA_CTL_MMAP_IIR_R);
				_mempool_pop(bufsize);
			}

			if (ictx->isp_pipe_cfg[raw_num].is_tnr_ai_isp) {
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_AI_ISP);
				_mempool_pop(bufsize);
			}

			// TNR motion
			bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_MO);
			_mempool_pop(bufsize);

			if (ictx->isp_pipe_cfg[raw_num].is_tile) {
				// TNR motion right tile
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1, ISP_BLK_ID_DMA_CTL_TNR_ST_MO);
				_mempool_pop(bufsize);
			}

			if (ictx->is_fbc_on) {
				// TNR Y
				tmp_bufaddr = _mempool_get_addr();
				//ring buffer constraint. reg_base is 4k byte-align
				bufaddr = VI_4K_ALIGN(tmp_bufaddr);
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_Y);
				_mempool_pop(bufsize + (u32)(bufaddr - tmp_bufaddr));

				// TNR UV
				tmp_bufaddr = _mempool_get_addr();
				//ring buffer constraint. reg_base is 4k byte-align
				bufaddr = VI_4K_ALIGN(tmp_bufaddr);
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_C);
				_mempool_pop(bufsize + (u32)(bufaddr - tmp_bufaddr));
			} else {
				// TNR Y
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_Y);
				_mempool_pop(bufsize);

				// TNR UV
				bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_C);
				_mempool_pop(bufsize);
				if (ictx->isp_pipe_cfg[raw_num].is_tile) {
					// TNR Y right tile
					bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1,
							ISP_BLK_ID_DMA_CTL_TNR_LD_Y);
					_mempool_pop(bufsize);

					// TNR UV right tile
					bufsize = ispblk_dma_buf_get_size(ictx, raw_num + 1,
							ISP_BLK_ID_DMA_CTL_TNR_LD_C);
					_mempool_pop(bufsize);
				}

				if (ictx->isp_pipe_cfg[raw_num].is_tnr_ai_isp) {
					bufsize = ispblk_dma_buf_get_size(ictx, raw_num,
							ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_Y);
					_mempool_pop(bufsize);

					bufsize = ispblk_dma_buf_get_size(ictx, raw_num,
							ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_U);
					_mempool_pop(bufsize);

					bufsize = ispblk_dma_buf_get_size(ictx, raw_num,
							ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_V);
					_mempool_pop(bufsize);
				}
			}
		}
	}
EXIT:
	return;
}

void _vi_yuvtop_get_dma_size(struct isp_ctx *ictx)
{
	u32 bufsize = 0;
	u64 bufaddr = 0;
	u64 tmp_bufaddr = 0;

	enum sop_isp_raw raw_num = ISP_PRERAW0;

	u8 cfg_dma = false;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (!ictx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			cfg_dma = true;
			break;
		}
	}

	if (cfg_dma == false) //No pipe need get_dma_size
		goto EXIT;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
			ictx->isp_pipe_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_ISP) //YUV sensor
			continue;

		// dci
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_DCI);
		_mempool_pop(bufsize);
		tmp_bufaddr = _mempool_pop(bufsize);

		// ldci
		bufaddr = VI_256_ALIGN(tmp_bufaddr);
		bufsize = ispblk_dma_buf_get_size(ictx, raw_num, ISP_BLK_ID_DMA_CTL_LDCI_W);
		_mempool_pop(bufsize + (u32)(bufaddr - tmp_bufaddr));

		vi_pr(VI_INFO, "ldci bufsize: total(%d), used(%d), wasted_for_alignment(%d)\n",
			bufsize + (u32)(bufaddr - tmp_bufaddr),
			bufsize,
			(u32)(bufaddr - tmp_bufaddr));

		if (ictx->isp_pipe_cfg[raw_num].is_tile) { //ldci right tile
			tmp_bufaddr = _mempool_get_addr();
			bufaddr = VI_256_ALIGN(tmp_bufaddr);
			_mempool_pop(bufsize + (u32)(bufaddr - tmp_bufaddr));

			vi_pr(VI_INFO, "ldci right tile bufsize: total(%d), used(%d), wasted_for_alignment(%d)\n",
				bufsize + (u32)(bufaddr - tmp_bufaddr),
				bufsize,
				(u32)(bufaddr - tmp_bufaddr));
		}
	}
EXIT:
	return;
}

void _vi_cmdq_get_dma_size(struct isp_ctx *ictx)
{
	u32 bufsize = 0;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		bufsize += VI_CMDQ_BUF_SIZE;
	}

	_mempool_pop(bufsize);
}

void _vi_get_dma_buf_size(struct isp_ctx *ictx)
{
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ictx, raw_num))
			continue;
		_vi_splt_get_dma_size(ictx, raw_num);
		_vi_pre_fe_get_dma_size(ictx, raw_num);
	}

	_vi_pre_be_get_dma_size(ictx);
	_vi_rawtop_get_dma_size(ictx);
	_vi_rgbtop_get_dma_size(ictx);
	_vi_yuvtop_get_dma_size(ictx);
	_vi_cmdq_get_dma_size(ictx);
}

static void _vi_preraw_be_init(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u32 bps[40] = {((0 << 12) | 0), ((0 << 12) | 20), ((0 << 12) | 40), ((0 << 12) | 60)};
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	u8 cfg_be = false;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
			cfg_be = true;
			break;
		}
	}

	if (cfg_be) { //RGB sensor
		// preraw_vi_sel
		ispblk_preraw_vi_sel_config(ctx);
		// preraw_be_top
		ispblk_preraw_be_config(ctx, raw_num);
		// preraw_be wdma ctrl
		ispblk_pre_wdma_ctrl_config(ctx, raw_num);

		//ispblk_blc_set_offset(ctx, ISP_BLC_ID_BE_LE, 511, 511, 511, 511);
		ispblk_blc_set_gain(ctx, ISP_BLC_ID_BE_LE, 0x40f, 0x419, 0x419, 0x405);
		//ispblk_blc_set_2ndoffset(ctx, ISP_BLC_ID_BE_LE, 511, 511, 511, 511);
		ispblk_blc_enable(ctx, ISP_BLC_ID_BE_LE, false, false);

		if (ctx->is_hdr_on) {
			ispblk_blc_set_offset(ctx, ISP_BLC_ID_BE_SE, 511, 511, 511, 511);
			ispblk_blc_set_gain(ctx, ISP_BLC_ID_BE_SE, 0x800, 0x800, 0x800, 0x800);
			ispblk_blc_set_2ndoffset(ctx, ISP_BLC_ID_BE_SE, 511, 511, 511, 511);
			ispblk_blc_enable(ctx, ISP_BLC_ID_BE_SE, false, false);
		}

		if (ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor) {
			ispblk_rgbir_config(ctx, ISP_RAW_PATH_LE, true);
			ispblk_rgbir_config(ctx, ISP_RAW_PATH_SE, ctx->is_hdr_on);
		} else {
			ispblk_rgbir_config(ctx, ISP_RAW_PATH_LE, false);
			ispblk_rgbir_config(ctx, ISP_RAW_PATH_SE, false);
		}

		ispblk_dpc_set_static(ctx, ISP_RAW_PATH_LE, 0, bps, 4);
		ispblk_dpc_config(ctx, ISP_RAW_PATH_LE, false, 0);
		ispblk_dpc_config(ctx, ISP_RAW_PATH_SE, ctx->is_hdr_on, 0);

		ispblk_af_config(ctx, true);

		if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)
			ispblk_slice_buf_config(&vdev->ctx, raw_num, true);
		else
			ispblk_slice_buf_config(&vdev->ctx, raw_num, false);
	}
}

static void _isp_rawtop_init(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;
	u8 first_raw_num = vi_get_first_raw_num(ictx);

	// raw_top
	ispblk_rawtop_config(ictx, first_raw_num);
	// raw_rdma ctrl
	ispblk_raw_rdma_ctrl_config(ictx, first_raw_num, ISP_BLK_ID_RAW_RDMA0,
					ictx->is_offline_postraw);
	ispblk_raw_rdma_ctrl_config(ictx, first_raw_num, ISP_BLK_ID_RAW_RDMA1,
					ictx->is_offline_postraw && ictx->is_hdr_on);

	ispblk_bnr_config(ictx, ISP_BLK_ID_BNR0, ISP_BNR_OUT_B_DELAY, false, 0, 0);

	ispblk_lsc_config(ictx, ISP_BLK_ID_LSC0, false);

	ispblk_cfa_config(ictx, ISP_BLK_ID_CFA0);
	ispblk_rgbcac_config(ictx, ISP_BLK_ID_RGBCAC0, true, 0);
	ispblk_lcac_config(ictx, ISP_BLK_ID_LCAC0, false, 0);
	ispblk_gms_config(ictx, true);

	ispblk_wbg_config(ictx, ISP_WBG_ID_RAW_TOP_LE, 0x400, 0x400, 0x400);
	ispblk_wbg_enable(ictx, ISP_WBG_ID_RAW_TOP_LE, false, false);

	ispblk_lmap_config(ictx, ISP_BLK_ID_LMAP0, true);

	ispblk_aehist_config(ictx, ISP_BLK_ID_AE_HIST0, true);

	if (ictx->is_hdr_on) {
		ispblk_bnr_config(ictx, ISP_BLK_ID_BNR1, ISP_BNR_OUT_B_DELAY, false, 0, 0);
		ispblk_lsc_config(ictx, ISP_BLK_ID_LSC1, false);
		ispblk_cfa_config(ictx, ISP_BLK_ID_CFA1);
		ispblk_rgbcac_config(ictx, ISP_BLK_ID_RGBCAC1, true, 0);
		ispblk_lcac_config(ictx, ISP_BLK_ID_LCAC1, false, 0);
		ispblk_wbg_config(ictx, ISP_WBG_ID_RAW_TOP_SE, 0x400, 0x400, 0x400);
		ispblk_wbg_enable(ictx, ISP_WBG_ID_RAW_TOP_SE, false, false);
		ispblk_lmap_config(ictx, ISP_BLK_ID_LMAP1, true);
		ispblk_aehist_config(ictx, ISP_BLK_ID_AE_HIST1, true);
	} else {
		ispblk_bnr_config(ictx, ISP_BLK_ID_BNR1, ISP_BNR_OUT_B_DELAY, false, 0, 0);
		ispblk_lsc_config(ictx, ISP_BLK_ID_LSC1, false);
		ispblk_rgbcac_config(ictx, ISP_BLK_ID_RGBCAC1, false, 0);
		ispblk_lcac_config(ictx, ISP_BLK_ID_LCAC1, false, 0);
		ispblk_lmap_config(ictx, ISP_BLK_ID_LMAP1, false);
		ispblk_aehist_config(ictx, ISP_BLK_ID_AE_HIST1, false);
	}
}

static void _isp_rgbtop_init(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;
	u8 first_raw_num = vi_get_first_raw_num(ictx);

	ispblk_rgbtop_config(ictx, first_raw_num);

	ispblk_hist_v_config(ictx, true, 0);

	//ispblk_awb_config(ictx, ISP_BLK_ID_AWB2, true, ISP_AWB_LE);

	ispblk_ccm_config(ictx, ISP_BLK_ID_CCM0, false, &ccm_hw_cfg);
	ispblk_dhz_config(ictx, false);

	ispblk_ygamma_config(ictx, false, ictx->gamma_tbl_idx, ygamma_data, 0, 0);
	ispblk_ygamma_enable(ictx, false);

	ispblk_gamma_config(ictx, false, ictx->gamma_tbl_idx, gamma_data, 0);
	ispblk_gamma_enable(ictx, false);

	//ispblk_clut_config(ictx, false, c_lut_r_lut, c_lut_g_lut, c_lut_b_lut);
	ispblk_rgbdither_config(ictx, false, false, false, false);
	ispblk_csc_config(ictx);

	ispblk_manr_config(ictx, ictx->is_3dnr_on);

	if (ictx->is_hdr_on) {
		ispblk_ccm_config(ictx, ISP_BLK_ID_CCM1, false, &ccm_hw_cfg);
		ispblk_fusion_config(ictx, true, true, ISP_FS_OUT_FS);
		ispblk_ltm_b_lut(ictx, 0, ltm_b_lut);
		ispblk_ltm_d_lut(ictx, 0, ltm_d_lut);
		ispblk_ltm_g_lut(ictx, 0, ltm_g_lut);
		ispblk_ltm_config(ictx, true, true, true, true);
	} else {
		ispblk_fusion_config(ictx, !_is_all_online(ictx), true, ISP_FS_OUT_LONG);
		ispblk_ltm_config(ictx, false, false, false, false);
	}
}

static void _isp_yuvtop_init(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;
	u8 first_raw_num = vi_get_first_raw_num(ictx);

	ispblk_yuvtop_config(ictx, first_raw_num);

	ispblk_yuvdither_config(ictx, 0, false, true, true, true);
	ispblk_yuvdither_config(ictx, 1, false, true, true, true);

	ispblk_tnr_config(ictx, ictx->is_3dnr_on, 0);
	if (ictx->is_3dnr_on && ictx->is_fbc_on) {
		ispblk_fbce_config(ictx, true);
		ispblk_fbcd_config(ictx, true);
		ispblk_fbc_ring_buf_config(ictx, true);
	} else {
		ispblk_fbce_config(ictx, false);
		ispblk_fbcd_config(ictx, false);
		ispblk_fbc_ring_buf_config(ictx, false);
	}

	ispblk_ynr_config(ictx, ISP_YNR_OUT_Y_DELAY, 128);
	ispblk_cnr_config(ictx, false, false, 255, 0);
	ispblk_pre_ee_config(ictx, false);
	ispblk_ee_config(ictx, false);
#ifdef COVER_WITH_BLACK
	memset(ycur_data, 0, sizeof(ycur_data));
	ispblk_ycur_config(ictx, false, 0, ycur_data);
	ispblk_ycur_enable(ictx, true, 0);
#else
	ispblk_ycur_config(ictx, false, 0, ycur_data);
	ispblk_ycur_enable(ictx, false, 0);
#endif
	ispblk_dci_config(ictx, true, ictx->gamma_tbl_idx, dci_map_lut_50, 0);
	ispblk_ldci_config(ictx, false, 0);

	ispblk_ca_config(ictx, false, 1);
	ispblk_ca_lite_config(ictx, false);

	ispblk_crop_enable(ictx, ISP_BLK_ID_YUV_CROP_Y, false);
	ispblk_crop_enable(ictx, ISP_BLK_ID_YUV_CROP_C, false);
}

static u32 _is_drop_next_frame(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u32 start_drop_num = ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num;
	u32 end_drop_num = start_drop_num + ctx->isp_pipe_cfg[raw_num].drop_frm_cnt;
	u32 frm_num = 0;
	u8 dev_num = 0;

	if (ctx->isp_pipe_cfg[raw_num].is_drop_next_frame) {
		//for tuning_dis, shoudn't trigger preraw;
		if ((ctx->is_multi_sensor) && (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor)) {
			dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
			if ((tuning_dis[0] > 0) && ((tuning_dis[0] - 1) != dev_num)) {
				vi_pr(VI_DBG, "input buf is not equal to current tuning number\n");
				return 1;
			}
		}

		//if sof_num in [start_sof, end_sof), shoudn't trigger preraw;
		frm_num = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];

		if ((start_drop_num != 0) && (frm_num >= start_drop_num) && (frm_num < end_drop_num))
			return 1;

		if (ddr_need_retrain(vdev))
			return 1;
	}

	return 0;
}

static void _set_drop_frm_info(
	const struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	struct isp_i2c_data *i2c_data)
{
	struct isp_ctx *ctx = (struct isp_ctx *)(&vdev->ctx);

	ctx->isp_pipe_cfg[raw_num].drop_frm_cnt = i2c_data->drop_frame_cnt;

	ctx->isp_pipe_cfg[raw_num].is_drop_next_frame = true;
	ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];

	vi_pr(VI_DBG, "raw_%d, drop_ref_frm_num=%d, drop frame=%d\n", raw_num,
				ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num,
				i2c_data->drop_frame_cnt);
}

static void _clear_drop_frm_info(
	const struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = (struct isp_ctx *)(&vdev->ctx);

	ctx->isp_pipe_cfg[raw_num].drop_frm_cnt = 0;
	ctx->isp_pipe_cfg[raw_num].drop_ref_frm_num = 0;
	ctx->isp_pipe_cfg[raw_num].is_drop_next_frame = false;
}

static void _snr_i2c_update(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	struct _isp_snr_i2c_node **_i2c_n,
	const u16 _i2c_num,
	int is_vblank_update)
{
	struct _isp_snr_i2c_node *node;
	struct _isp_snr_i2c_node *next_node;
	struct isp_i2c_data *i2c_data;
	struct isp_i2c_data *next_i2c_data;
	unsigned long flags;
	u16 i = 0, j = 0;
	// 0: Delete Node, 1: Postpone Node, 2: Do nothing
	u16 del_node = 0;
	u32 dev_mask = 0;
	u32 cmd = burst_i2c_en ? SNS_I2C_BURST_QUEUE : SNS_I2C_WRITE;
	u8 no_update = 1;
	u32 fe_frm_num = 0;

	if (vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be || vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en)
		return;

	fe_frm_num = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];

	for (j = 0; j < _i2c_num; j++) {
		node = _i2c_n[j];
		no_update = 1;

		vi_pr(VI_DBG, "raw_num=%d, i2c_num=%d, j=%d, magic_num=%d, fe_frm_num=%d, v_blank_update=%d\n",
				raw_num, _i2c_num, j, node->n.magic_num, fe_frm_num, is_vblank_update);

		//magic num set by ISP team. fire i2c when magic num same as last fe frm num.
		if (((node->n.magic_num == fe_frm_num ||
			 (node->n.magic_num < fe_frm_num && (j + 1) >= _i2c_num)) && (!is_vblank_update)) ||
			 ((node->n.magic_num_vblank  == fe_frm_num ||
			 (node->n.magic_num_vblank  < fe_frm_num && (j + 1) >= _i2c_num)) && (is_vblank_update))) {

			if ((node->n.magic_num != fe_frm_num && !is_vblank_update) ||
				(node->n.magic_num_vblank != fe_frm_num && is_vblank_update)) {
				vi_pr(VI_WARN, "exception handle, send delayed i2c data.\n");
			}

			for (i = 0; i < node->n.regs_num; i++) {
				i2c_data = &node->n.i2c_data[i];

				vi_pr(VI_DBG, "i2cdata[%d]:i2c_addr=0x%x write:0x%x needvblank:%d needupdate:%d\n", i,
				i2c_data->reg_addr, i2c_data->data, i2c_data->vblank_update, i2c_data->update);

				if (i2c_data->update && (i2c_data->dly_frm_num == 0)) {
					if ((i2c_data->vblank_update && is_vblank_update)
					|| (!i2c_data->vblank_update &&!is_vblank_update)) {
						vi_sys_cmm_cb_i2c(cmd, (void *)i2c_data);
						i2c_data->update = 0;
						if (burst_i2c_en)
							dev_mask |= BIT(i2c_data->i2c_dev);
						if (i2c_data->drop_frame)
							_set_drop_frm_info(vdev, raw_num, i2c_data);
					} else {
						no_update = 0;
					}
				} else if (i2c_data->update && !(i2c_data->dly_frm_num == 0)) {
					vi_pr(VI_DBG, "addr=0x%x, dly_frm=%d\n",
							i2c_data->reg_addr, i2c_data->dly_frm_num);
					i2c_data->dly_frm_num--;
					del_node = 1;
				}
			}

		} else if ((node->n.magic_num < fe_frm_num && !is_vblank_update) ||
					(node->n.magic_num_vblank < fe_frm_num && is_vblank_update)) {

			if ((j + 1) < _i2c_num) {

				next_node = _i2c_n[j + 1];

				for (i = 0; i < next_node->n.regs_num; i++) {
					next_i2c_data = &next_node->n.i2c_data[i];
					i2c_data = &node->n.i2c_data[i];

					if (i2c_data->update && next_i2c_data->update == 0) {
						next_i2c_data->update = i2c_data->update;
						vi_pr(VI_WARN, "exception handle, i2c node merge, addr: 0x%x\n",
							i2c_data->reg_addr);
					}
				}

				del_node = 0;
			} else {
				// impossible case
			}
		} else {
			del_node = 2;
		}

		if (del_node == 0 && no_update) {
			vi_pr(VI_DBG, "i2c node %d del node and free\n", j);
			spin_lock_irqsave(&snr_node_lock[raw_num], flags);
			list_del_init(&node->list);
			--isp_snr_i2c_queue[raw_num].num_rdy;
			kfree(node);
			spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);
		} else if (del_node == 1) {
			if (is_vblank_update)
				node->n.magic_num_vblank++;
			else
				node->n.magic_num++;
			vi_pr(VI_DBG, "postpone i2c node\n");
		}
	}

	while (dev_mask) {
		u32 tmp = ffs(dev_mask) - 1;

		vi_sys_cmm_cb_i2c(SNS_I2C_BURST_FIRE, (void *)&tmp);
		dev_mask &= ~BIT(tmp);
	}
}

static void _isp_snr_cfg_deq_and_fire(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	int needvblank)
{
	struct list_head *pos, *temp;
	struct _isp_snr_i2c_node *i2c_n[VI_MAX_LIST_NUM], *i2c_n_temp[0];
	unsigned long flags;
	u16 i2c_num = 0;
	int i;

	spin_lock_irqsave(&snr_node_lock[raw_num], flags);
	if (needvblank == 0) {
		list_for_each_safe(pos, temp, &isp_snr_i2c_queue[raw_num].list) {
			i2c_n[i2c_num] = list_entry(pos, struct _isp_snr_i2c_node, list);
			i2c_num++;
		}
	} else {
		list_for_each_safe(pos, temp, &isp_snr_i2c_queue[raw_num].list) {
			i2c_n_temp[0] = list_entry(pos, struct _isp_snr_i2c_node, list);
			for (i = 0; i < i2c_n_temp[0]->n.regs_num; i++) {
				if (i2c_n_temp[0]->n.i2c_data[i].vblank_update &&
				    i2c_n_temp[0]->n.i2c_data[i].update) {
					i2c_n[i2c_num] = i2c_n_temp[0];
					i2c_num++;
					break;
				}
			}
		}
	}

	spin_unlock_irqrestore(&snr_node_lock[raw_num], flags);

	_snr_i2c_update(vdev, raw_num, i2c_n, i2c_num, needvblank);
}

static inline void _vi_clear_mmap_fbc_ring_base(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	//Clear mmap previous ring base to start addr after first frame done.
	if (ctx->is_3dnr_on && (ctx->isp_pipe_cfg[raw_num].first_frm_cnt == 1)) {
		manr_clear_prv_ring_base(ctx, raw_num);

		if (ctx->is_fbc_on) {
			ispblk_fbc_chg_to_sw_mode(&vdev->ctx, raw_num);
			ispblk_fbc_clear_fbcd_ring_base(&vdev->ctx, raw_num);
		}
	}
}

static inline void _vi_wake_up_preraw_th(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	unsigned long flags;
	struct _isp_raw_num_n *n;

	n = kzalloc(sizeof(*n), GFP_ATOMIC);
	if (n == NULL) {
		vi_pr(VI_ERR, "pre_raw_num_q kmalloc size(%zu) fail\n", sizeof(*n));
		return;
	}
	n->raw_num = raw_num;

	spin_lock_irqsave(&raw_num_lock, flags);
	list_add_tail(&n->list, &pre_raw_num_q.list);
	spin_unlock_irqrestore(&raw_num_lock, flags);

	vdev->vi_th[E_VI_TH_PRERAW].flag = 1;
	wake_up(&vdev->vi_th[E_VI_TH_PRERAW].wq);
}

int user_pic_trig(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	s8 s8Ret = ISP_SUCCESS;

	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe) {
		if (atomic_read(&vdev->pre_fe_state[raw_num][ISP_FE_CH0]) != ISP_STATE_IDLE)
			return 1;

		if (atomic_read(&vdev->pre_fe_state[raw_num][ISP_FE_CH1]) != ISP_STATE_IDLE &&
			ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			return 1;

		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			//reconfig rdma for tile dma
			u32 splt_fe0_le = ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_LE;
			u32 splt_fe0_se = ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_SE;
			u32 splt_fe1_le = ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_LE;
			u32 splt_fe1_se = ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_SE;
			u64 dma_addr = 0;

			if (atomic_read(&vdev->postraw_state) != ISP_STATE_IDLE)
				return 1;

			if (atomic_read(&vdev->pre_fe_state[raw_num + 1][ISP_FE_CH0]) != ISP_STATE_IDLE)
				return 1;

			if (atomic_read(&vdev->pre_fe_state[raw_num + 1][ISP_FE_CH1]) != ISP_STATE_IDLE &&
				ctx->isp_pipe_cfg[raw_num].is_hdr_on)
				return 1;

			dma_addr = ispblk_dma_getaddr(ctx, splt_fe0_le);
			ispblk_dma_config(ctx, raw_num, splt_fe0_le, dma_addr);
			if (ctx->is_dpcm_on)
				dma_addr += (ctx->isp_pipe_cfg[raw_num].csibdg_width * 3) / 4;
			else
				dma_addr += (ctx->isp_pipe_cfg[raw_num].csibdg_width * 3) / 2;
			ispblk_dma_config(ctx, raw_num + 1, splt_fe1_le, dma_addr);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				dma_addr = ispblk_dma_getaddr(ctx, splt_fe0_se);
				ispblk_dma_config(ctx, raw_num, splt_fe0_se, dma_addr);
				if (ctx->is_dpcm_on)
					dma_addr += (ctx->isp_pipe_cfg[raw_num].csibdg_width * 3) / 4;
				else
					dma_addr += (ctx->isp_pipe_cfg[raw_num].csibdg_width * 3) / 2;
				ispblk_dma_config(ctx, raw_num + 1, splt_fe1_se, dma_addr);
			}
		}

		if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT) {
			_vi_wake_up_tpu_th(vdev, raw_num, AI_ISP_TYPE_BNR);
		} else {
			tasklet_hi_schedule(&vdev->job_work);

			s8Ret = _pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
				s8Ret |= _pre_hw_enque(vdev, raw_num, ISP_FE_CH1);

			if (ctx->isp_pipe_cfg[raw_num].is_tile) {
				s8Ret |= _pre_hw_enque(vdev, raw_num + 1, ISP_FE_CH0);
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
					s8Ret |= _pre_hw_enque(vdev, raw_num + 1, ISP_FE_CH1);
			}
			if (s8Ret == ISP_SUCCESS)
				_splt_hw_enque(vdev, raw_num);
		}
	} else if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		if (atomic_read(&vdev->pre_be_state[ISP_BE_CH0]) != ISP_STATE_IDLE)
			return 1;

		// maybe no used now, a2 used be offline mode for raw replay
		if (_is_fe_be_online(ctx)) {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				b = isp_buf_next(&pre_be_out_q[ISP_RAW_PATH_SE]);
				if (!b) {
					vi_pr(VI_DBG, "pre_be chn_num_%d outbuf is empty\n", ISP_FE_CH1);
					return 1;
				}

				ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE, b->addr);
			}

			if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) { // raw_dump flow
				_isp_fe_be_raw_dump_cfg(vdev, raw_num, 0);
				atomic_set(&vdev->isp_raw_dump_en[raw_num], 3);
			}
		}

		_vi_clear_mmap_fbc_ring_base(vdev, raw_num);

		vi_tuning_gamma_ips_update(ctx, raw_num);
		vi_tuning_clut_update(ctx, raw_num);
		vi_tuning_dci_update(ctx, raw_num);
		vi_tuning_drc_update(ctx, raw_num);

		_post_rgbmap_update(ctx, raw_num, vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);

		_pre_hw_enque(vdev, raw_num, (enum sop_isp_fe_chn_num)ISP_BE_CH0);

		_vi_wake_up_preraw_th(vdev, raw_num);

		//if (!_is_all_online(ctx)) //Not on the fly mode
		//	tasklet_hi_schedule(&vdev->job_work);
	}
	return 0;
}

static void _usr_pic_timer_handler(unsigned long data)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)usr_pic_timer.data;
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	if (!(ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe || ctx->isp_pipe_cfg[raw_num].is_raw_replay_be))
		goto EXIT;

	if (!ctx->is_ctrl_inited)
		goto EXIT;

	if (atomic_read(&vdev->isp_streamoff) == 1)
		goto EXIT;

#ifdef PORTING_TEST
	if (!usr_trigger)
		goto EXIT;
	usr_trigger = stop_stream_en ? false : true;
#endif

	if (user_pic_trig(vdev)) {
		goto EXIT;
	}

EXIT:
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	mod_timer(&usr_pic_timer.t, jiffies + vdev->usr_pic_delay);
#else
	mod_timer(&usr_pic_timer, jiffies + vdev->usr_pic_delay);
#endif
}

void usr_pic_time_remove(void)
{
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	if (timer_pending(&usr_pic_timer.t)) {
		del_timer_sync(&usr_pic_timer.t);
		timer_setup(&usr_pic_timer.t, legacy_timer_emu_func, 0);
#else
	if (timer_pending(&usr_pic_timer)) {
		del_timer_sync(&usr_pic_timer);
		init_timer(&usr_pic_timer);
#endif
	}
}

int usr_pic_timer_init(struct sop_vi_dev *vdev)
{
#ifdef PORTING_TEST
	usr_trigger = true;
#endif
	usr_pic_time_remove();
	usr_pic_timer.function = _usr_pic_timer_handler;
	usr_pic_timer.data = (uintptr_t)vdev;
#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	usr_pic_timer.t.expires = jiffies + vdev->usr_pic_delay;
	add_timer(&usr_pic_timer.t);
#else
	usr_pic_timer.expires = jiffies + vdev->usr_pic_delay;
	add_timer(&usr_pic_timer);
#endif

	return 0;
}

void vi_event_queue(struct sop_vi_dev *vdev, const u32 type, const u32 frm_num)
{
	unsigned long flags;
	struct vi_event_k *ev_k;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 ts;
#endif

	if (type >= VI_EVENT_MAX) {
		vi_pr(VI_ERR, "event queue type(%d) error\n", type);
		return;
	}

	ev_k = kzalloc(sizeof(*ev_k), GFP_ATOMIC);
	if (ev_k == NULL) {
		vi_pr(VI_ERR, "event queue kzalloc size(%zu) fail\n", sizeof(*ev_k));
		return;
	}

	spin_lock_irqsave(&event_lock, flags);
	ev_k->ev.type = type;
	ev_k->ev.frame_sequence = frm_num;

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	ts = ktime_to_timespec64(ktime_get());
	ev_k->ev.timestamp.tv_sec = ts.tv_sec;
	ev_k->ev.timestamp.tv_nsec = ts.tv_nsec;
#else
	ev_k->ev.timestamp = ktime_to_timeval(ktime_get());
#endif
	list_add_tail(&ev_k->list, &event_q.list);
	spin_unlock_irqrestore(&event_lock, flags);

	wake_up(&vdev->isp_event_wait_q);
}

void sop_isp_dqbuf_list(struct sop_vi_dev *vdev, const u32 frm_num,
			const u8 raw_id, const u8 chn_id, struct timespec64 ts)
{
	unsigned long flags;
	struct _isp_dqbuf_n *n;

	n = kzalloc(sizeof(struct _isp_dqbuf_n), GFP_ATOMIC);
	if (n == NULL) {
		vi_pr(VI_ERR, "DQbuf kmalloc size(%zu) fail\n", sizeof(struct _isp_dqbuf_n));
		return;
	}
	n->raw_id	= raw_id;
	n->chn_id	= chn_id;
	n->frm_num	= frm_num;
	n->timestamp	= ts;

	spin_lock_irqsave(&dq_lock, flags);
	list_add_tail(&n->list, &dqbuf_q.list);
	spin_unlock_irqrestore(&dq_lock, flags);
}

int vi_dqbuf(struct _vi_buffer *b)
{
	unsigned long flags;
	struct _isp_dqbuf_n *n = NULL;
	int ret = -1;

	spin_lock_irqsave(&dq_lock, flags);
	if (!list_empty(&dqbuf_q.list)) {
		n = list_first_entry(&dqbuf_q.list, struct _isp_dqbuf_n, list);
		b->raw_id		= n->raw_id;
		b->chn_id		= n->chn_id;
		b->sequence		= n->frm_num;
		b->timestamp	= n->timestamp;
		list_del_init(&n->list);
		kfree(n);
		ret = 0;
	}
	spin_unlock_irqrestore(&dq_lock, flags);

	return ret;
}

static int _vi_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_VI;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

static void vi_init(void)
{
	int i, j;

	memset(g_vi_ctx, 0, sizeof(*g_vi_ctx));

	for (i = 0; i < VI_MAX_CHN_NUM; ++i) {
		g_vi_ctx->rotation[i] = ROTATION_0;
		g_vi_ctx->ldc_attr[i].enable = 0;
		mutex_init(&g_vi_mesh[i].lock);
	}

	for (i = 0; i < VI_MAX_CHN_NUM; ++i)
		for (j = 0; j < VI_MAX_EXTCHN_BIND_PER_CHN; ++j)
			g_vi_ctx->chn_bind[i][j] = VI_INVALID_CHN;
}

static void _isp_yuv_bypass_buf_enq(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_isp_buf *b = NULL;
	enum isp_blk_id_t dmaid;
	u64 tmp_addr = 0, i = 0;

	sop_isp_rdy_buf_pop(vdev, raw_num, hw_chn_num);
	b = sop_isp_rdy_buf_next(vdev, raw_num, hw_chn_num);
	if (b == NULL)
		return;

	vi_pr(VI_DBG, "update yuv bypass outbuf: 0x%llx raw_%d chn_num(%d)\n",
			b->buf.planes[0].addr, raw_num, hw_chn_num);

	if (ctx->isp_pipe_cfg[raw_num].is_bt_demux)
		dmaid = csibdg_lite_dma_find_hwid(raw_num, hw_chn_num);
	else
		dmaid = csibdg_dma_find_hwid(raw_num, hw_chn_num);

	if (ctx->isp_pipe_cfg[raw_num].is_422_to_420) {
		for (i = 0; i < 2; i++) {
			tmp_addr = b->buf.planes[i].addr;
			if (vdev->pre_fe_frm_num[raw_num][hw_chn_num] == 0)
				ispblk_dma_yuv_bypass_config(ctx, dmaid + i, tmp_addr, raw_num);
			else
				ispblk_dma_setaddr(ctx, raw_num, dmaid + i, tmp_addr);
		}
	} else {
		tmp_addr = b->buf.planes[0].addr;

		if (vdev->pre_fe_frm_num[raw_num][hw_chn_num] == 0)
			ispblk_dma_yuv_bypass_config(ctx, dmaid, tmp_addr, raw_num);
		else
			ispblk_dma_setaddr(ctx, raw_num, dmaid, tmp_addr);
	}
}

static void _isp_yuv_bypass_trigger(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (atomic_read(&vdev->isp_err_handle_flag) == 1)
		return;

	if (atomic_read(&vdev->isp_streamoff) == 0) {
		if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][hw_chn_num],
					ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
					ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "fe_%d chn_num_%d is running\n", raw_num, hw_chn_num);
			return;
		}

		_isp_yuv_bypass_buf_enq(vdev, raw_num, hw_chn_num);
		isp_pre_trig(ctx, raw_num, hw_chn_num);
	}
}

void _vi_postraw_ctrl_setup(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num;
	u8 cfg_post = false;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
			ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			cfg_post = true;
			break;
		}
	}

	if (cfg_post) {
		_isp_rawtop_init(vdev);
		_isp_rgbtop_init(vdev);
		_isp_yuvtop_init(vdev);
	}

	ispblk_isptop_config(ctx);
}

void _vi_pre_fe_ctrl_setup(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	struct isp_ctx *ictx = &vdev->ctx;
	u32 blc_le_id, blc_se_id, rgbmap_le_id, rgbmap_se_id, wbg_le_id, wbg_se_id;
	struct cif_yuv_swap_s swap = {0};

	if (ictx->isp_pipe_cfg[raw_num].is_yuv_sensor) {//YUV sensor
		if (ictx->isp_pipe_cfg[raw_num].is_422_to_420) {//uyvy to yuyv to 420
			swap.devno = raw_num;
			swap.yc_swap = 1;
			swap.uv_swap = 1;
			_vi_call_cb(E_MODULE_CIF, MIPI_SET_YUV_SWAP, &swap);
		}

		if (ictx->isp_pipe_cfg[raw_num].is_bt_demux)
			ispblk_csibdg_lite_config(ictx, raw_num);
		else {
			ispblk_csibdg_yuv_bypass_config(ictx, raw_num);
			if (ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				switch (raw_num) {
				case ISP_PRERAW0:
					rgbmap_le_id = ISP_RGBMAP_ID_FE0_LE;
					rgbmap_se_id = ISP_RGBMAP_ID_FE0_SE;
					break;
				case ISP_PRERAW1:
					rgbmap_le_id = ISP_RGBMAP_ID_FE1_LE;
					rgbmap_se_id = ISP_RGBMAP_ID_FE1_SE;
					break;
				case ISP_PRERAW2:
					rgbmap_le_id = ISP_RGBMAP_ID_FE2_LE;
					rgbmap_se_id = ISP_RGBMAP_ID_FE2_SE;
					break;
				case ISP_PRERAW3:
					rgbmap_le_id = ISP_RGBMAP_ID_FE3_LE;
					rgbmap_se_id = ISP_RGBMAP_ID_FE3_SE;
					break;
				case ISP_PRERAW4:
					rgbmap_le_id = ISP_RGBMAP_ID_FE4_LE;
					rgbmap_se_id = ISP_RGBMAP_ID_FE4_SE;
					break;
				case ISP_PRERAW5:
					rgbmap_le_id = ISP_RGBMAP_ID_FE5_LE;
					rgbmap_se_id = ISP_RGBMAP_ID_FE5_SE;
					break;
				default:
					break;
				}
				ispblk_rgbmap_config(ictx, rgbmap_le_id, ictx->is_3dnr_on);
				ispblk_rgbmap_config(ictx, rgbmap_se_id, false);

				ispblk_preraw_fe_config(ictx, raw_num);
			}
		}

		if (ictx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) { //vi vpss offline mode
			u8 chn_str = 0;

			for (; chn_str < ictx->isp_pipe_cfg[raw_num].mux_mode + 1; chn_str++)
				_isp_yuv_bypass_buf_enq(vdev, raw_num, chn_str);
		}
	} else { //RGB sensor
		switch (raw_num) {
		case ISP_PRERAW0:
			blc_le_id    = ISP_BLC_ID_FE0_LE;
			blc_se_id    = ISP_BLC_ID_FE0_SE;
			wbg_le_id    = ISP_WBG_ID_FE0_LE;
			wbg_se_id    = ISP_WBG_ID_FE0_SE;
			rgbmap_le_id = ISP_RGBMAP_ID_FE0_LE;
			rgbmap_se_id = ISP_RGBMAP_ID_FE0_SE;
			break;
		case ISP_PRERAW1:
			blc_le_id    = ISP_BLC_ID_FE1_LE;
			blc_se_id    = ISP_BLC_ID_FE1_SE;
			wbg_le_id    = ISP_WBG_ID_FE1_LE;
			wbg_se_id    = ISP_WBG_ID_FE1_SE;
			rgbmap_le_id = ISP_RGBMAP_ID_FE1_LE;
			rgbmap_se_id = ISP_RGBMAP_ID_FE1_SE;
			break;
		case ISP_PRERAW2:
			blc_le_id    = ISP_BLC_ID_FE2_LE;
			blc_se_id    = ISP_BLC_ID_FE2_SE;
			wbg_le_id    = ISP_WBG_ID_FE2_LE;
			wbg_se_id    = ISP_WBG_ID_FE2_SE;
			rgbmap_le_id = ISP_RGBMAP_ID_FE2_LE;
			rgbmap_se_id = ISP_RGBMAP_ID_FE2_SE;
			break;
		case ISP_PRERAW3:
			blc_le_id    = ISP_BLC_ID_FE3_LE;
			blc_se_id    = ISP_BLC_ID_FE3_SE;
			wbg_le_id    = ISP_WBG_ID_FE3_LE;
			wbg_se_id    = ISP_WBG_ID_FE3_SE;
			rgbmap_le_id = ISP_RGBMAP_ID_FE3_LE;
			rgbmap_se_id = ISP_RGBMAP_ID_FE3_SE;
			break;
		case ISP_PRERAW4:
			blc_le_id    = ISP_BLC_ID_FE4_LE;
			blc_se_id    = ISP_BLC_ID_FE4_SE;
			wbg_le_id    = ISP_WBG_ID_FE4_LE;
			wbg_se_id    = ISP_WBG_ID_FE4_SE;
			rgbmap_le_id = ISP_RGBMAP_ID_FE4_LE;
			rgbmap_se_id = ISP_RGBMAP_ID_FE4_SE;
			break;
		case ISP_PRERAW5:
			blc_le_id    = ISP_BLC_ID_FE5_LE;
			blc_se_id    = ISP_BLC_ID_FE5_SE;
			wbg_le_id    = ISP_WBG_ID_FE5_LE;
			wbg_se_id    = ISP_WBG_ID_FE5_SE;
			rgbmap_le_id = ISP_RGBMAP_ID_FE5_LE;
			rgbmap_se_id = ISP_RGBMAP_ID_FE5_SE;
			break;
		default:
			break;
		}

		ispblk_preraw_fe_config(ictx, raw_num);
		ispblk_csibdg_config(ictx, raw_num);
		ispblk_csibdg_crop_update(ictx, raw_num, true);

		ispblk_blc_set_gain(ictx, blc_le_id, 0x40f, 0x419, 0x419, 0x405);
		ispblk_blc_enable(ictx, blc_le_id, false, false);

		ispblk_wbg_config(ictx, wbg_le_id, 0x400, 0x400, 0x400);
		ispblk_wbg_enable(ictx, wbg_le_id, false, false);

		ispblk_rgbmap_config(ictx, rgbmap_le_id, ictx->is_3dnr_on);

		if (ictx->isp_pipe_cfg[raw_num].is_hdr_on) {
			ispblk_blc_set_gain(ictx, blc_se_id, 0x40f, 0x419, 0x419, 0x405);
			ispblk_blc_enable(ictx, blc_se_id, false, false);

			ispblk_rgbmap_config(ictx, rgbmap_se_id, ictx->is_3dnr_on);

			ispblk_wbg_config(ictx, wbg_se_id, 0x400, 0x400, 0x400);
			ispblk_wbg_enable(ictx, wbg_se_id, false, false);
		} else {
			ispblk_rgbmap_config(ictx, rgbmap_se_id, false);
		}
	}
}

void _vi_splt_ctrl_setup(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (raw_num != ISP_PRERAW0)
		return;

	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe ||
	    ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT ||
	    ctx->isp_pipe_cfg[raw_num].is_tile ||
	    line_spliter_en) {
		ispblk_splt_config(ctx, raw_num, true);
	} else {
		ispblk_splt_config(ctx, raw_num, false);
	}

	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe) {
		ispblk_splt_wdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE0_WDMA, false);
		ispblk_splt_wdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE1_WDMA, false);

		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE0_RDMA_LE, true);
		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE0_RDMA_SE,
						ctx->isp_pipe_cfg[raw_num].is_hdr_on || ctx->isp_pipe_cfg[raw_num].is_yuv_sensor);

		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE1_RDMA_LE,
						ctx->isp_pipe_cfg[raw_num].is_tile);
		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE1_RDMA_SE,
						ctx->isp_pipe_cfg[raw_num].is_tile &&
						(ctx->isp_pipe_cfg[raw_num].is_hdr_on || ctx->isp_pipe_cfg[raw_num].is_yuv_sensor));
	} else if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT) {
		ispblk_splt_wdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE0_WDMA, true);
		ispblk_splt_wdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE1_WDMA,
					     ctx->isp_pipe_cfg[raw_num].is_tile);

		// rdma have no data at first frame
		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE0_RDMA_LE, false);
		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE0_RDMA_SE, false);

		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE1_RDMA_LE, false);
		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE1_RDMA_SE, false);
	} else {
		ispblk_splt_wdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE0_WDMA, false);
		ispblk_splt_wdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE1_WDMA, false);

		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE0_RDMA_LE, false);
		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE0_RDMA_SE, false);

		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE1_RDMA_LE, false);
		ispblk_splt_rdma_ctrl_config(ctx, ISP_BLK_ID_SPLT_FE1_RDMA_SE, false);
	}
}

void _vi_ctrl_init(enum sop_isp_raw raw_num, struct sop_vi_dev *vdev)
{
	struct isp_ctx *ictx = &vdev->ctx;
	bool is_ctrl_inited = true;
	u8 dev_num = vi_get_dev_num_by_raw(ictx, raw_num);
	u8 first_raw_num = vi_get_first_raw_num(ictx);

	if (ictx->is_ctrl_inited)
		return;

	if (vdev->snr_info[dev_num].snr_fmt.img_size[0].active_w != 0) { //MW config snr_info flow
		ictx->isp_pipe_cfg[raw_num].csibdg_width  = vdev->snr_info[dev_num].snr_fmt.img_size[0].width;
		ictx->isp_pipe_cfg[raw_num].csibdg_height = vdev->snr_info[dev_num].snr_fmt.img_size[0].height;
		ictx->isp_pipe_cfg[raw_num].max_width     = vdev->snr_info[dev_num].snr_fmt.img_size[0].max_width;
		ictx->isp_pipe_cfg[raw_num].max_height    = vdev->snr_info[dev_num].snr_fmt.img_size[0].max_height;

		ictx->isp_pipe_cfg[raw_num].crop.w = vdev->snr_info[dev_num].snr_fmt.img_size[0].active_w;
		ictx->isp_pipe_cfg[raw_num].crop.h = vdev->snr_info[dev_num].snr_fmt.img_size[0].active_h;
		ictx->isp_pipe_cfg[raw_num].crop.x = vdev->snr_info[dev_num].snr_fmt.img_size[0].start_x;
		ictx->isp_pipe_cfg[raw_num].crop.y = vdev->snr_info[dev_num].snr_fmt.img_size[0].start_y;

		if (vdev->snr_info[dev_num].snr_fmt.frm_num > 1) { //HDR
			ictx->isp_pipe_cfg[raw_num].crop_se.w = vdev->snr_info[dev_num].snr_fmt.img_size[1].active_w;
			ictx->isp_pipe_cfg[raw_num].crop_se.h = vdev->snr_info[dev_num].snr_fmt.img_size[1].active_h;
			ictx->isp_pipe_cfg[raw_num].crop_se.x = vdev->snr_info[dev_num].snr_fmt.img_size[1].start_x;
			ictx->isp_pipe_cfg[raw_num].crop_se.y = vdev->snr_info[dev_num].snr_fmt.img_size[1].start_y;

			ictx->isp_pipe_cfg[raw_num].is_hdr_on = true;
			vdev->ctx.is_hdr_on = true;
		}

		ictx->rgb_color_mode[raw_num] = vdev->snr_info[dev_num].color_mode;
	}

	if (ictx->isp_pipe_cfg[raw_num].is_patgen_en ||
	    ictx->isp_pipe_cfg[raw_num].is_raw_replay_fe ||
	    ictx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		ictx->isp_pipe_cfg[raw_num].crop.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop.y = vdev->usr_crop.top;

		ictx->isp_pipe_cfg[raw_num].crop_se.w = vdev->usr_crop.width;
		ictx->isp_pipe_cfg[raw_num].crop_se.h = vdev->usr_crop.height;
		ictx->isp_pipe_cfg[raw_num].crop_se.x = vdev->usr_crop.left;
		ictx->isp_pipe_cfg[raw_num].crop_se.y = vdev->usr_crop.top;

		ictx->isp_pipe_cfg[raw_num].csibdg_width  = vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].csibdg_height = vdev->usr_fmt.height;
		ictx->isp_pipe_cfg[raw_num].max_width     = vdev->usr_fmt.width;
		ictx->isp_pipe_cfg[raw_num].max_height    = vdev->usr_fmt.height;

		ictx->rgb_color_mode[raw_num] = vdev->usr_fmt.code;

#if defined(__CV180X__)
/**
 * the hardware limit is clk_mac <= clk_be * 2
 * cv180x's clk_mac is 594M, but clk_be just 198M(ND)/250M(OD)
 * clk_mac need to do frequency division.
 * ratio = (div_val + 1) / 32
 * target = source * ratio
 * div_val = target / source * 32 - 1
 * ex: target = 200, source = 594, div_val = 200 / 594 * 32 - 1 = 10
 */
		if (ictx->isp_pipe_cfg[raw_num].is_patgen_en) {
			vip_sys_reg_write_mask(VIP_SYS_REG_NORM_DIV_VAL_CSI_MAC0,
						VIP_SYS_REG_NORM_DIV_VAL_CSI_MAC0_MASK,
						10 << VIP_SYS_REG_NORM_DIV_VAL_CSI_MAC0_OFFSET);
			vip_sys_reg_write_mask(VIP_SYS_REG_NORM_DIV_EN_CSI_MAC0,
						VIP_SYS_REG_NORM_DIV_EN_CSI_MAC0_MASK,
						1 << VIP_SYS_REG_NORM_DIV_EN_CSI_MAC0_OFFSET);
			vip_sys_reg_write_mask(VIP_SYS_REG_UPDATE_SEL_CSI_MAC0,
						VIP_SYS_REG_UPDATE_SEL_CSI_MAC0_MASK,
						1 << VIP_SYS_REG_UPDATE_SEL_CSI_MAC0_OFFSET);
		}
#endif
	}

	ictx->isp_pipe_cfg[raw_num].post_img_w = ictx->isp_pipe_cfg[raw_num].crop.w;
	ictx->isp_pipe_cfg[raw_num].post_img_h = ictx->isp_pipe_cfg[raw_num].crop.h;

	/* use csibdg crop */
	ictx->crop_x = 0;
	ictx->crop_y = 0;
	ictx->crop_se_x = 0;
	ictx->crop_se_y = 0;

	if (!ictx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
		//Postraw out size
		ictx->img_width = ictx->isp_pipe_cfg[first_raw_num].crop.w;
		ictx->img_height = ictx->isp_pipe_cfg[first_raw_num].crop.h;
	}

	if ((ictx->rgb_color_mode[raw_num] >= ISP_BAYER_TYPE_GRGBI) &&
	    (ictx->rgb_color_mode[raw_num] <= ISP_BAYER_TYPE_IGBGR)) {
		ictx->isp_pipe_cfg[raw_num].is_rgbir_sensor = true;
	}

	vi_pr(VI_INFO, "raw_num(%d) color_mode(%d) csibdg_w_h(%d:%d) max_w_h(%d:%d) crop_x_y_w_h(%d:%d:%d:%d)\n",
		raw_num, ictx->rgb_color_mode[raw_num],
		ictx->isp_pipe_cfg[raw_num].csibdg_width, ictx->isp_pipe_cfg[raw_num].csibdg_height,
		ictx->isp_pipe_cfg[raw_num].max_width, ictx->isp_pipe_cfg[raw_num].max_height,
		ictx->isp_pipe_cfg[raw_num].crop.x, ictx->isp_pipe_cfg[raw_num].crop.y,
		ictx->isp_pipe_cfg[raw_num].crop.w, ictx->isp_pipe_cfg[raw_num].crop.h);

	if (raw_num == first_raw_num) {
		if (_is_fe_be_online(ictx) && ictx->is_slice_buf_on)
			vi_calculate_slice_buf_setting(ictx, raw_num);

		isp_init(ictx);
	}

	ictx->isp_pipe_cfg[raw_num].is_ctrl_inited = true;
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ictx->isp_pipe_enable[raw_num])
			continue;

		if (!ictx->isp_pipe_cfg[raw_num].is_ctrl_inited)
			is_ctrl_inited = false;
	}

	if (is_ctrl_inited)
		ictx->is_ctrl_inited = true;
}

void _vi_scene_ctrl(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	u8 dev_num = 0;
	u8 rgb_snsr_num = 0;

	if (ctx->is_ctrl_inited) {
		return;
	}

	if (g_vi_ctx->total_dev_num >= 2) { //Multi sensor scenario
		ctx->is_multi_sensor = true;

		for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
			if (!ctx->isp_pipe_enable[raw_num])
				continue;
			if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor)
				rgb_snsr_num++;
		}

		if (rgb_snsr_num == 1) { //rgb+yuv
			ctx->is_offline_be = true;
			ctx->is_offline_postraw = false;
			ctx->is_slice_buf_on = false;
			/*
			// if 422to420, chn_attr must be NV21
			if (ctx->isp_pipe_cfg[ISP_PRERAW1].mux_mode == VI_WORK_MODE_1MULTIPLEX &&
				g_vi_ctx->chn_attr[ctx->rawb_chnstr_num].pixel_format == PIXEL_FORMAT_NV21)
				ctx->isp_pipe_cfg[ISP_PRERAW1].is_422_to_420  = true;
			*/
			RGBMAP_BUF_IDX = 3;
		} else {
			ctx->is_offline_be = true;
			ctx->is_offline_postraw = false;
			ctx->is_slice_buf_on = false;
			ctx->is_fbc_on = false;
			/*
			if (ctx->isp_pipe_cfg[ISP_PRERAW2].is_yuv_sensor &&
				ctx->isp_pipe_cfg[ISP_PRERAW2].mux_mode == VI_WORK_MODE_1MULTIPLEX &&
				g_vi_ctx->chn_attr[ctx->rawb_chnstr_num].pixel_format == PIXEL_FORMAT_NV21)
				ctx->isp_pipe_cfg[ISP_PRERAW2].is_422_to_420  = true;
			*/
			//Only single sensor with non-tile can use two rgbmap buf, two sensors need 3 rgbmap
			RGBMAP_BUF_IDX = 3;
		}
	} else { //Single sensor
		raw_num = vi_get_first_raw_num(ctx);
		dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
		ctx->is_multi_sensor = false;

		//for 585 wdr need use 3 rgbmap, fe ch0 after sof
		RGBMAP_BUF_IDX = 3;

#ifndef PORTING_TEST
		if (ctx->is_offline_be || ctx->is_offline_postraw) {
			ctx->is_offline_be = false;
			ctx->is_offline_postraw = true;
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { // rgb sensor
			if ((vdev->snr_info[dev_num].snr_fmt.img_size[0].active_w > 4608) || tile_en) {
				ctx->is_tile = true;
				ctx->is_fbc_on = false;
				ctx->isp_pipe_cfg[raw_num].is_tile = true;
				ctx->isp_pipe_cfg[raw_num + 1].is_tile = true;
			} else if (vdev->snr_info[dev_num].snr_fmt.img_size[0].active_w <= 4608 &&
				   vdev->snr_info[dev_num].snr_fmt.img_size[0].active_h <= 2160) {
				ctx->is_slice_buf_on = true;
			}

			if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe ||
			    ctx->isp_pipe_cfg[raw_num].is_tile) { // splt->dram->fe0~1->dram->be->post
				ctx->is_offline_be = true;
				ctx->is_offline_postraw = false;
				ctx->is_slice_buf_on = false;
				RGBMAP_BUF_IDX = 3;
			} else if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) { // dram->be->post
				ctx->is_offline_be = true;
				ctx->is_offline_postraw = false;
				ctx->is_slice_buf_on = false;
				ctx->rgbmap_prebuf_idx = 0;
				RGBMAP_BUF_IDX = 3;
			} else { // fe->be->dram/slice->post
				//Only single sensor with non-tile can use two rgbmap buf
				RGBMAP_BUF_IDX = 2;
			}
		} else { // yuv sensor
			ctx->isp_pipe_cfg[raw_num].is_offline_scaler = true;
			ctx->isp_pipe_cfg[raw_num].is_422_to_420 = false;

			if (ctx->isp_pipe_cfg[raw_num].mux_mode > 0 &&
			    ctx->isp_pipe_cfg[raw_num].inf_mode >= VI_MODE_BT656 &&
			    ctx->isp_pipe_cfg[raw_num].inf_mode <= VI_MODE_BT1120_INTERLEAVED) {
				ctx->isp_pipe_cfg[raw_num].is_bt_demux = true;
				if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
					vi_pr(VI_WARN, "bt_demux sensor switch scene_mode to YUV_SCENE_ONLINE\n");
					ctx->isp_pipe_cfg[raw_num].yuv_scene_mode = ISP_YUV_SCENE_ONLINE;
				}
			}

			ctx->is_offline_be = true;
			ctx->is_offline_postraw = false;
			ctx->is_slice_buf_on = false;
		}
#else
		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { // rgb sensor
			if ((ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
			     vdev->usr_fmt.width > 4608) ||
			    (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
			     vdev->snr_info[dev_num].snr_fmt.img_size[0].active_w > 4608) ||
			    (tile_en)) {
				ctx->is_tile = true;
				ctx->is_fbc_on = false;
				ctx->is_offline_be = true;
				ctx->is_offline_postraw = false;
				ctx->isp_pipe_cfg[raw_num].is_tile = true;
				ctx->isp_pipe_cfg[raw_num + 1].is_tile = true;
			}
		}
#endif
		if (!ctx->is_3dnr_on)
			ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp = false;

		if (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp)
			ctx->is_fbc_on = false; //send uncompressed data to AI

		vi_pr(VI_INFO, "is_3dnr_on[%d], is_fbc_on[%d], is_tile[%d]\n",
				ctx->is_3dnr_on, ctx->is_fbc_on, ctx->is_tile);
	}

	if (line_spliter_en) {
		if (ctx->is_multi_sensor) {
			vi_pr(VI_WARN, "force line spliter not support multi sensor now!\n");
			line_spliter_en = 0;
		} else if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
			vi_pr(VI_WARN, "force line spliter not support yuv sensor now!\n");
			line_spliter_en = 0;
		} else if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe ||
			   ctx->isp_pipe_cfg[raw_num].is_raw_replay_be ||
			   ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap ||
			   ctx->isp_pipe_cfg[raw_num].is_tile) {
			vi_pr(VI_WARN, "raw_raplay/ai_isp_raw/tile mode is already on line spliter mode!\n");
			line_spliter_en = 0;
		}

		// force line spiliter mode always go through splt->fe0->dram->be->post
		if (line_spliter_en) {
			ctx->is_offline_be = true;
			ctx->is_offline_postraw = false;
			ctx->is_slice_buf_on = false;
			RGBMAP_BUF_IDX = 2;
		}
	}

	if (!sbm_en)
		ctx->is_slice_buf_on = false;

	if (rgbmap_sbm_en && ctx->is_slice_buf_on)
		ctx->is_rgbmap_sbm_on = true;

	for (dev_num = 0; dev_num < VI_MAX_DEV_NUM; dev_num++) {
		if (!g_vi_ctx->is_dev_enable[dev_num])
			continue;

		raw_num = vi_get_raw_num_by_dev(ctx, dev_num);

		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;

		if (_is_right_tile(ctx, raw_num))
			continue;

		ctx->raw_chnstr_num[raw_num] = ctx->total_chn_num;

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
			ctx->isp_pipe_cfg[raw_num].pixel_fmt =
					g_vi_ctx->chn_attr[ctx->raw_chnstr_num[raw_num]].pixel_format;
			ctx->total_chn_num++;
			vi_pr(VI_INFO, "raw_num=%d, rgb, chnstr_num=%d, chn_num=%d\n",
					raw_num, ctx->raw_chnstr_num[raw_num], 1);
		} else {
			ctx->total_chn_num += ctx->isp_pipe_cfg[raw_num].mux_mode + 1;

			vi_pr(VI_INFO, "raw_num=%d, yuv, chnstr_num=%d, chn_num=%d\n",
					raw_num, ctx->raw_chnstr_num[raw_num],
					ctx->isp_pipe_cfg[raw_num].mux_mode + 1);
		}
	}

	vi_pr(VI_INFO, "Total_chn_num=%d\n", ctx->total_chn_num);
}

#if 0
static void _vi_suspend(struct sop_vi_dev *vdev)
{
	struct sop_vi_ctx *vi_proc_ctx = NULL;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	vi_proc_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	if (vi_proc_ctx->vi_stt == VI_SUSPEND) {
		for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++)
			isp_streaming(&vdev->ctx, false, raw_num);

		_vi_sw_init(vdev);
#ifndef FPGA_PORTING
		_vi_clk_ctrl(vdev, false);
#endif
	}
}

static int _vi_resume(struct sop_vi_dev *vdev)
{
	struct sop_vi_ctx *vi_proc_ctx = NULL;

	vi_proc_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	if (vi_proc_ctx->vi_stt == VI_SUSPEND) {
		//_vi_mempool_reset();
		//sop_isp_sw_init(vdev);

		vi_proc_ctx->vi_stt = VI_RUNNING;
	}

	return 0;
}
#endif

void _vi_bw_cal_set(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_proc_ctx = NULL;
	void __iomem *bw_limiter;
	u64 bwladdr[ISP_BW_LIMIT_MAX] = {0x0A074020, 0x0A072020, 0x0A078020}; // rdma, wdma0, wdma1
	u32 data_size[ISP_BW_LIMIT_MAX] = {0}, BW[ISP_PRERAW_MAX][ISP_BW_LIMIT_MAX] = {0};
	u32 total_bw = 0, bwlwin = 0, bwltxn = 0, margin = 125, fps = 25;
	u32 def_bwltxn = 4, def_fps = 25;
	u32 width, height;
	u8 i, yuv_chn_num;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	vi_proc_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		fps = vi_proc_ctx->dev_attr[raw_num].snr_fps ?
			vi_proc_ctx->dev_attr[raw_num].snr_fps :
			def_fps;
		width = ctx->isp_pipe_cfg[raw_num].crop.w;
		height = ctx->isp_pipe_cfg[raw_num].crop.h;

		if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {//RGB sensor
			if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				data_size[ISP_BW_LIMIT_RDMA] = (411 * width * height) / 128 + 50052;
				data_size[ISP_BW_LIMIT_WDMA0] = (396 * width * height) / 128 + 8160;
				data_size[ISP_BW_LIMIT_WDMA1] = (391 * width * height) / 128 + 21536;
			} else {
				data_size[ISP_BW_LIMIT_RDMA] = (630 * width * height) / 128 + 50052;
				data_size[ISP_BW_LIMIT_WDMA0] = (792 * width * height) / 128 + 8160;
				data_size[ISP_BW_LIMIT_WDMA1] = (394 * width * height) / 128 + 38496;
			}
		} else { //YUV sensor
			yuv_chn_num = ctx->isp_pipe_cfg[raw_num].mux_mode + 1;
			data_size[ISP_BW_LIMIT_RDMA] = (192 * yuv_chn_num * width * height) / 128;
			data_size[ISP_BW_LIMIT_WDMA0] = (192 * yuv_chn_num * width * height) / 128;
			data_size[ISP_BW_LIMIT_WDMA1] = 0;
		}

		for (i = 0; i < ISP_BW_LIMIT_MAX; ++i) {
			BW[raw_num][i] = (fps * data_size[i]) / 1000000 + 1;
		}
	}

	// TODO
	// just restrain RDMA now, WDMA0/WDMA1 wait for Brian
	// for (i = 0; i < BW_LIMIT_MAX; ++i) {
	for (i = 0; i < 1; ++i) {
		total_bw = 0;

		for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
			total_bw += BW[raw_num][i];
		}

		for (bwltxn = def_bwltxn; bwltxn > 1; --bwltxn) {
			bwlwin = bwltxn * 256000 / ((((total_bw * 33) / 10) * margin) / 100);
			if (bwlwin <= 1024)
				break;
		}

		bw_limiter = ioremap(bwladdr[i], 0x4);
		iowrite32(((bwltxn << 10) | bwlwin), bw_limiter);
		vi_pr(VI_INFO, "isp %s bw_limiter=0x%x, BW=%d, bwltxn=%d, bwlwin=%d\n",
				(i == 0) ? "rdma" : ((i == 1) ? "wdma0" : "wdma1"),
				ioread32(bw_limiter), total_bw, bwltxn, bwlwin);
		iounmap(bw_limiter);
	}
}

static void _vi_splt_set_all_state(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw hw_raw_num,
	const enum sop_isp_state splt_state)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = hw_raw_num;

	vi_pr(VI_DBG, "splt_%d, %s\n", raw_num,
			(splt_state == ISP_STATE_IDLE) ? "ISP_STATE_IDLE" :
			(splt_state == ISP_STATE_RUNNING) ? "ISP_STATE_RUNNING" : "ISP_STATE_PREPARE");

	if (_is_right_tile(ctx, raw_num))
		raw_num = ISP_PRERAW0;

	atomic_set(&vdev->splt_state[raw_num][ISP_FE_CH0], splt_state);
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
		atomic_set(&vdev->splt_state[raw_num][ISP_FE_CH1], splt_state);

	if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		atomic_set(&vdev->splt_state[raw_num + 1][ISP_FE_CH0], splt_state);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			atomic_set(&vdev->splt_state[raw_num + 1][ISP_FE_CH1], splt_state);
	}
}

int vi_start_streaming(struct sop_vi_dev *vdev)
{
	struct cif_attr_s cif_attr;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	int rc = 0;

	vi_pr(VI_DBG, "+\n");

#if 0
	if (_vi_resume(vdev) != 0) {
		vi_pr(VI_ERR, "vi resume failed\n");
		return -1;
	}
#endif
	_vi_mempool_reset();
	vi_tuning_buf_setup(&vdev->ctx);
	vi_tuning_buf_clear();

	//SW workaround to disable csibdg enable first due to csibdg enable is on as default.
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++)
		isp_streaming(&vdev->ctx, false, raw_num);

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (vdev->ctx.isp_pipe_cfg[raw_num].raw_ai_isp_ap) {
			vi_pr(VI_INFO, "raw_%d ai isp access point:%d\n",
			      raw_num, vdev->ctx.isp_pipe_cfg[raw_num].raw_ai_isp_ap);
			++vdev->tpu_thread_num;
			if (vdev->tpu_thread_num == 1) {
				vdev->tpu_thd_bind[raw_num] = E_VI_TH_RUN_TPU1;
			} else if (vdev->tpu_thread_num == 2) {
				vdev->tpu_thd_bind[raw_num] = E_VI_TH_RUN_TPU2;
			} else {
				vi_pr(VI_ERR, "only support 2 tpu thread\n");
				break;
			}

			if (vi_create_thread(vdev, vdev->tpu_thd_bind[raw_num])) {
				vi_pr(VI_ERR, "Failed to create tpu thread\n");
			}
		}
	}

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;

		if (_is_right_tile(&vdev->ctx, raw_num))
			continue;

		/* cif lvds reset */
		_vi_call_cb(E_MODULE_CIF, CIF_CB_RESET_LVDS, &raw_num);

		/* Get stagger vsync info from cif */
		cif_attr.devno = raw_num;
#if defined(__CV186X__)
		if (raw_num == ISP_PRERAW1)
			cif_attr.devno = ISP_PRERAW3;
		else if (raw_num == ISP_PRERAW3)
			cif_attr.devno = ISP_PRERAW1;
#endif
		if (_vi_call_cb(E_MODULE_CIF, CIF_CB_GET_CIF_ATTR, &cif_attr) == 0)
			vdev->ctx.isp_pipe_cfg[raw_num].is_stagger_vsync = cif_attr.stagger_vsync;
	}

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;

		// _vi_ctrl_init(raw_num, vdev);
		if (raw_num == ISP_PRERAW0) {
			_vi_splt_ctrl_setup(vdev, raw_num);
			if (vdev->ctx.isp_pipe_cfg[raw_num].is_tile) {
				vdev->ctx.rgb_color_mode[raw_num + 1] =
						vdev->ctx.rgb_color_mode[raw_num];
				vdev->ctx.isp_pipe_cfg[raw_num + 1].is_stagger_vsync =
						vdev->ctx.isp_pipe_cfg[raw_num].is_stagger_vsync;
			}
		}
		if (!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be)
			_vi_pre_fe_ctrl_setup(vdev, raw_num);
		if (!vdev->ctx.is_multi_sensor) { //only single sensor maybe break
			if (_is_all_online(&vdev->ctx) ||
				(_is_fe_be_online(&vdev->ctx) && vdev->ctx.is_slice_buf_on)) {
				vi_pr(VI_INFO, "on-the-fly mode or slice_buffer is on\n");
				break;
			}
		}

		if (!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_fe &&
		    !vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be &&
		     vdev->ctx.isp_pipe_cfg[raw_num].raw_ai_isp_ap != RAW_AI_ISP_SPLT) {
			if (!vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB sensor
				isp_pre_trig(&vdev->ctx, raw_num, ISP_FE_CH0);
				if (vdev->ctx.isp_pipe_cfg[raw_num].is_hdr_on)
					isp_pre_trig(&vdev->ctx, raw_num, ISP_FE_CH1);
			} else { //YUV sensor
				u8 chn_str = 0;
				u8 total_chn = vdev->ctx.isp_pipe_cfg[raw_num].mux_mode + 1;

				for (; chn_str < total_chn; chn_str++)
					isp_pre_trig(&vdev->ctx, raw_num, chn_str);
			}
		}

		if (vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_sensor) {
			if (vdev->ctx.isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				if (vdev->ctx.isp_pipe_cfg[raw_num].is_offline_scaler) {
					vi_pr(VI_INFO, "raw_num [%d] YUV with ISP & offline scaler\n", raw_num);
				} else {
					vi_pr(VI_INFO, "raw_num [%d] YUV with ISP & online scaler\n", raw_num);
				}
			} else {
				vi_pr(VI_INFO, "raw_num [%d] YUV bypass ISP\n", raw_num);
			}
		}
	}

	_vi_preraw_be_init(vdev);
	_vi_postraw_ctrl_setup(vdev);
	_vi_dma_setup(&vdev->ctx);
	_vi_dma_set_sw_mode(&vdev->ctx);

	vi_pr(VI_INFO, "ISP scene path, be_off=%d, post_off=%d, slice_buff_on=%d, 3dnr_on=%d, fbc_on=%d\n",
			vdev->ctx.is_offline_be, vdev->ctx.is_offline_postraw, vdev->ctx.is_slice_buf_on,
			vdev->ctx.is_3dnr_on, vdev->ctx.is_fbc_on);

	raw_num = vi_get_first_raw_num(&vdev->ctx);
	if (_is_all_online(&vdev->ctx)) {
		if (vdev->ctx.isp_pipe_cfg[raw_num].is_offline_scaler) { //offline mode
			if (!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be)
				isp_pre_trig(&vdev->ctx, raw_num, ISP_FE_CH0);

			_postraw_outbuf_enq(vdev, raw_num, ISP_FE_CH0); //all online must be ch0
		} else { //online mode
			struct sc_cfg_cb post_para = {0};
			u8 dev_num = vi_get_dev_num_by_raw(&vdev->ctx, raw_num);

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = dev_num;
			post_para.is_tile = false;
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_INFO, "snr_num_%d, sc_%d is not ready. try later\n", raw_num, dev_num);
			} else {
				atomic_set(&vdev->ol_sc_frm_done, 0);

				if (!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be)
					isp_pre_trig(&vdev->ctx, raw_num, ISP_FE_CH0);
			}
		}
	} else if (_is_fe_be_online(&vdev->ctx) && vdev->ctx.is_slice_buf_on) {
		if (vdev->ctx.isp_pipe_cfg[raw_num].is_offline_scaler) { //offline mode
			_postraw_outbuf_enq(vdev, raw_num, ISP_FE_CH0); //slice must be ch0

			isp_post_trig(&vdev->ctx, raw_num);
			vi_record_post_trigger(vdev, raw_num);

			if (!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be) {
				if (!vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB sensor
					isp_pre_trig(&vdev->ctx, raw_num, ISP_FE_CH0);
					if (vdev->ctx.isp_pipe_cfg[raw_num].is_hdr_on)
						isp_pre_trig(&vdev->ctx, raw_num, ISP_FE_CH1);
				}
			}

			atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_RUNNING);
		} else { //online mode
			struct sc_cfg_cb post_para = {0};
			u8 dev_num = vi_get_dev_num_by_raw(&vdev->ctx, raw_num);

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = dev_num;
			post_para.is_tile = false;
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_INFO, "snr_num_%d, sc_%d is not ready. try later\n", raw_num, dev_num);
			} else {
				atomic_set(&vdev->ol_sc_frm_done, 0);

				isp_post_trig(&vdev->ctx, raw_num);
				vi_record_post_trigger(vdev, raw_num);

				if (!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be) {
					isp_pre_trig(&vdev->ctx, raw_num, ISP_FE_CH0);
					if (vdev->ctx.isp_pipe_cfg[raw_num].is_hdr_on)
						isp_pre_trig(&vdev->ctx, raw_num, ISP_FE_CH1);
				}

				atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_RUNNING);
			}
		}
	} else if (_is_be_post_online(&vdev->ctx)) {
		if (!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be &&
		    !vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_fe) { //not rawreplay
			if (vdev->ctx.isp_pipe_cfg[raw_num].is_tile ||
			    vdev->ctx.isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT ||
			    line_spliter_en) {
				isp_splt_trig(&vdev->ctx, raw_num);
				if (vdev->ctx.isp_pipe_cfg[raw_num].raw_ai_isp_ap)
					_vi_splt_set_all_state(vdev, raw_num, ISP_STATE_RUNNING);
			}
		}
	}

#ifndef PORTING_TEST
	_vi_bw_cal_set(vdev);
#endif

#ifdef PORTING_TEST
	vi_ip_test_cases_init(&vdev->ctx);
#endif

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;

		isp_streaming(&vdev->ctx, true, raw_num);
	}

	atomic_set(&vdev->isp_streamoff, 0);

	isp_raw_dump_init();

	return rc;
}

/* abort streaming and wait for last buffer */
int vi_stop_streaming(struct sop_vi_dev *vdev)
{
	struct sop_isp_buf *sop_vb, *tmp;
	struct _isp_dqbuf_n *n = NULL;
	struct vi_event_k   *ev_k = NULL;
	unsigned long flags;
	struct isp_buffer *isp_b;
	struct _isp_snr_i2c_node *i2c_n;
	struct _isp_raw_num_n    *raw_n;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	u8 i = 0, j = 0, count = 10;
	u8 rc = 0;
	struct sop_vi_ctx *vi_proc_ctx = NULL;

	vi_proc_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

	vi_pr(VI_INFO, "+\n");

	atomic_set(&vdev->isp_streamoff, 1);

	// disable load-from-dram at streamoff
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++)
		vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be = false;

	usr_pic_time_remove();

	// wait to make sure hw stopped.
	while (--count > 0) {
		if (atomic_read(&vdev->postraw_state) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH0]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH1]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH2]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH3]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH0]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH1]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH2]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH3]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW2][ISP_FE_CH0]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW2][ISP_FE_CH1]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW3][ISP_FE_CH0]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW3][ISP_FE_CH1]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW4][ISP_FE_CH0]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW4][ISP_FE_CH1]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW5][ISP_FE_CH0]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW5][ISP_FE_CH1]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH0]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH1]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH2]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH3]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH0]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH1]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH2]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH3]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_be_state[ISP_BE_CH0]) == ISP_STATE_IDLE &&
			atomic_read(&vdev->pre_be_state[ISP_BE_CH1]) == ISP_STATE_IDLE)
			break;
		vi_pr(VI_WARN, "wait count(%d)\n", count);
#ifdef FPGA_PORTING
		msleep(200);
#else
		msleep(20);
#endif
	}

	if (count == 0) {
		vi_pr(VI_ERR, "isp status fe_0(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH2]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status fe_1(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH2]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status fe_2(ch0:%d, ch1:%d) fe_3(ch0:%d, ch1:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW2][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW2][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW3][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW3][ISP_FE_CH1]));
		vi_pr(VI_ERR, "isp status fe_4(ch0:%d, ch1:%d) fe_5(ch0:%d, ch1:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW4][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW4][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW5][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW5][ISP_FE_CH1]));
		vi_pr(VI_ERR, "isp status fe_lite0(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH2]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status fe_lite1(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH2]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status be(ch0:%d, ch1:%d) postraw(%d)\n",
				atomic_read(&vdev->pre_be_state[ISP_BE_CH0]),
				atomic_read(&vdev->pre_be_state[ISP_BE_CH1]),
				atomic_read(&vdev->postraw_state));
	}

#if 0
	for (i = 0; i < 2; i++) {
		/*
		 * Release all the buffers enqueued to driver
		 * when streamoff is issued
		 */
		spin_lock_irqsave(&vdev->rdy_lock, flags);
		list_for_each_entry_safe(sop_vb, tmp, &(vdev->rdy_queue[i]), list) {
			vfree(sop_vb);
		}
		vdev->num_rdy[i] = 0;
		INIT_LIST_HEAD(&vdev->rdy_queue[i]);
		spin_unlock_irqrestore(&vdev->rdy_lock, flags);
	}
#endif
	for (i = 0; i < VI_MAX_CHN_NUM; i++) {
			/*
			* Release all the buffers enqueued to driver
			* when streamoff is issued
			*/
		spin_lock_irqsave(&vdev->qbuf_lock, flags);
		for (j = 0; j < VI_MAX_CHN_NUM; j++) {
			list_for_each_entry_safe(sop_vb, tmp, &(vdev->qbuf_list[i][j]), list) {
				kfree(sop_vb);
			}
			vdev->qbuf_num[i][j] = 0;
			INIT_LIST_HEAD(&vdev->qbuf_list[i][j]);
		}
		spin_unlock_irqrestore(&vdev->qbuf_lock, flags);
	}

	spin_lock_irqsave(&dq_lock, flags);
	while (!list_empty(&dqbuf_q.list)) {
		n = list_first_entry(&dqbuf_q.list, struct _isp_dqbuf_n, list);
		list_del_init(&n->list);
		kfree(n);
	}
	spin_unlock_irqrestore(&dq_lock, flags);

	spin_lock_irqsave(&event_lock, flags);
	while (!list_empty(&event_q.list)) {
		ev_k = list_first_entry(&event_q.list, struct vi_event_k, list);
		list_del_init(&ev_k->list);
		kfree(ev_k);
	}
	spin_unlock_irqrestore(&event_lock, flags);

	for (i = 0; i < ISP_SPLT_MAX; i++) {
		for (j = 0; j < ISP_SPLT_CHN_MAX; j++) {
			while ((isp_b = isp_buf_remove(&splt_out_q[i][j])) != NULL)
				vfree(isp_b);
			while ((isp_b = isp_buf_remove(&pre_fe_in_q[i][j])) != NULL)
				vfree(isp_b);
		}
	}

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		for (j = 0; j < ISP_FE_CHN_MAX; j++) {
			while ((isp_b = isp_buf_remove(&pre_fe_out_q[i][j])) != NULL)
				vfree(isp_b);
			while ((isp_b = isp_buf_remove(&bnr_ai_isp_q[i][j])) != NULL) {
				if (unlikely(isp_b->is_ext == EXTERNAL_BUFFER)) {
					_vi_release_ext_buf(isp_b->addr);
					kfree(isp_b);
				} else {
					vfree(isp_b);
				}
			}
		}

		while ((isp_b = isp_buf_remove(&raw_dump_b_dq[i])) != NULL)
			vfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_b_se_dq[i])) != NULL)
			vfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_b_q[i])) != NULL)
			vfree(isp_b);
		while ((isp_b = isp_buf_remove(&raw_dump_b_se_q[i])) != NULL)
			vfree(isp_b);

		spin_lock_irqsave(&snr_node_lock[i], flags);
		while (!list_empty(&isp_snr_i2c_queue[i].list)) {
			i2c_n = list_first_entry(&isp_snr_i2c_queue[i].list, struct _isp_snr_i2c_node, list);
			list_del_init(&i2c_n->list);
			kfree(i2c_n);
		}
		isp_snr_i2c_queue[i].num_rdy = 0;
		spin_unlock_irqrestore(&snr_node_lock[i], flags);

		while ((isp_b = isp_buf_remove(&pre_be_in_se_q[i])) != NULL)
			vfree(isp_b);
	}

	while ((isp_b = isp_buf_remove(&pre_be_in_q)) != NULL) {
		if (unlikely(isp_b->is_ext == EXTERNAL_BUFFER)) {
			_vi_release_ext_buf(isp_b->addr);
			kfree(isp_b);
		} else {
			vfree(isp_b);
		}
	}

	for (i = 0; i < ISP_RAW_PATH_MAX; i++) {
		while ((isp_b = isp_buf_remove(&pre_be_out_q[i])) != NULL)
			vfree(isp_b);
		while ((isp_b = isp_buf_remove(&postraw_in_q[i])) != NULL)
			vfree(isp_b);
	}

	spin_lock_irqsave(&raw_num_lock, flags);
	while (!list_empty(&pre_raw_num_q.list)) {
		raw_n = list_first_entry(&pre_raw_num_q.list, struct _isp_raw_num_n, list);
		list_del_init(&raw_n->list);
		kfree(raw_n);
	}
	spin_unlock_irqrestore(&raw_num_lock, flags);

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		kfree(isp_bufpool[i].fswdr_rpt);
		isp_bufpool[i].fswdr_rpt = 0;
	}

	isp_raw_dump_deinit();

	// reset at stop for next run.
	isp_reset(&vdev->ctx);
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++)
		isp_streaming(&vdev->ctx, false, raw_num);

#ifdef PORTING_TEST
	vi_ip_test_cases_uninit(&vdev->ctx);
#endif
//	_vi_suspend(vdev);

	vi_tuning_buf_release();

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (vdev->tpu_thd_bind[raw_num]) {
			vi_destory_thread(vdev, vdev->tpu_thd_bind[raw_num]);
			vdev->tpu_thread_num--;
		}
	}

	vi_pr(VI_INFO, "-\n");

	return rc;
}

static int check_pre_fe_state(struct sop_vi_dev *vdev,
			      const enum sop_isp_raw raw_num,
			      enum sop_isp_fe_chn_num chn_num)
{
	int wait_count = 10;
	int wait_ms_per_times = 2;

	while (wait_count--) {
		if (atomic_read(&vdev->pre_fe_state[raw_num][chn_num]) != ISP_STATE_RUNNING)
			break;
		if (atomic_read(&vdev->isp_streamoff)) {
			atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_STATE_IDLE);
			return 0;
		}
		msleep(wait_ms_per_times);

		vi_pr(VI_DBG, "wait fe_state cnt(%d)\n", wait_count);
	}

	if (wait_count < 0) {
		vi_pr(VI_INFO, "wait fe_state idle timeout! need skip pre_hw_enque\n");
		atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_STATE_IDLE);
		return 0;
	}

	return 1;
}

static u64 bnr_ai_isp_launch_tpu(struct sop_vi_dev *vdev,
				 const enum sop_isp_raw raw_num,
				 enum sop_isp_fe_chn_num chn_num,
				 u64 input_addr)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	u64 output_addr = input_addr;
	u8 dev_num = vi_get_dev_num_by_raw(&vdev->ctx, raw_num);
	bool is_lauch_sucess = false;
	int timeout = 200;
	int ret = 0;

	if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT)
		b = isp_buf_last(&splt_out_q[raw_num][chn_num]);
	else
		b = isp_buf_last(&pre_fe_out_q[raw_num][chn_num]);

	if (!b) {
		vi_pr(VI_ERR, "ai isp outbuf %d_%d is NULL!\n", raw_num, chn_num);
		goto exit;
	}

	output_addr = b->addr;

	if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe)
		b->addr = input_addr;

	++ctx->isp_pipe_cfg[raw_num].raw_ai_isp_frm_cnt;

	ai_isp_cfg_info[raw_num].ai_bnr_addr_pool[0] = input_addr;
	ai_isp_cfg_info[raw_num].ai_bnr_addr_pool[1] = output_addr;

	atomic_set(&vdev->ai_isp_int_flag[raw_num], 1);

	wake_up(&vdev->ai_isp_wait_q[raw_num]);

	ret = wait_event_timeout(
		vdev->ai_isp_wait_q[raw_num],
		atomic_read(&vdev->ai_isp_int_flag[raw_num]) == 2,
		msecs_to_jiffies(timeout));

	if (!ret) {
		vi_pr(VI_WARN, "dev_%d wait process timeout(%d ms) at frame(%d)\n",
			dev_num, timeout, ctx->isp_pipe_cfg[raw_num].raw_ai_isp_frm_cnt);
		goto exit;
	}

	is_lauch_sucess = true;

exit:
	if (!is_lauch_sucess) {
		b->addr = output_addr;
		output_addr = input_addr;
	}

	return output_addr;
}

static void ai_isp_handle_process(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	enum ai_isp_type_e ai_isp_type = AI_ISP_TYPE_BUTT;
	struct isp_queue *tpu_w_q = NULL;
	struct isp_queue *tpu_r_q = NULL;
	enum sop_isp_fe_chn_num chn_num = ISP_FE_CH0;
	enum sop_isp_fe_chn_num chn_max = chn_num;
	u64 output_buf = 0;

	ai_isp_type = atomic_read(&vdev->ai_isp_type);
	switch (ai_isp_type) {
	case AI_ISP_TYPE_BNR:
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			chn_max = ISP_FE_CH1;
		else
			chn_max = ISP_FE_CH0;

		reinit_completion(&vdev->tpu_done[raw_num]);
		while (chn_num <= chn_max) {
			if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT) {
				if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe) {//raw replay
					u32 splt_fe0 = chn_num ?
							ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_SE :
							ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_LE;
					u64 in_addr = 0;
					u64 out_addr = 0;

					in_addr = vdev->usr_pic_phy_addr[ISP_FE_CH0];

					if (atomic_read(&vdev->bnr_run_tpu[raw_num])) {
						out_addr = bnr_ai_isp_launch_tpu(vdev, raw_num,
										 chn_num, in_addr);
						ispblk_dma_config(ctx, raw_num, splt_fe0, out_addr);
					}

					if (atomic_read(&vdev->isp_streamoff))
						break;

					_pre_hw_enque(vdev, raw_num, chn_num);
					_splt_hw_enque(vdev, raw_num);
				} else {//splt ai isp
					tpu_w_q = &splt_out_q[raw_num][chn_num];
					tpu_r_q = &pre_fe_in_q[raw_num][chn_num];
					b = isp_buf_remove(tpu_w_q);
					if (!b) {
						vi_pr(VI_ERR, "tpu_w_q_%d_%d is NULL!\n",
						      raw_num, chn_num);
						break;
					}
					_splt_hw_enque(vdev, raw_num);

					if (atomic_read(&vdev->bnr_run_tpu[raw_num]))
						b->addr = bnr_ai_isp_launch_tpu(vdev, raw_num,
										chn_num, b->addr);

					if (check_pre_fe_state(vdev, raw_num, chn_num)) {
						isp_buf_queue(tpu_r_q, b);
						_pre_hw_enque(vdev, raw_num, chn_num);
					} else {
						isp_buf_queue(tpu_w_q, b);
					}
				}
			} else if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_FE) {
				tpu_w_q = &bnr_ai_isp_q[raw_num][chn_num];
				tpu_r_q = (chn_num == ISP_FE_CH0) ? &pre_be_in_q :
					   &pre_be_in_se_q[raw_num];
				b = isp_buf_remove(tpu_w_q);
				if (!b) {
					vi_pr(VI_ERR, "tpu_w_q_%d_%d is NULL!\n", raw_num, chn_num);
					break;
				}

				// to avoid i2c_data send delay
				--vdev->pre_fe_frm_num[raw_num][chn_num];
				mutex_lock(&vdev->ai_isp_lock);
				_pre_hw_enque(vdev, raw_num, chn_num);
				mutex_unlock(&vdev->ai_isp_lock);

				if (atomic_read(&vdev->bnr_run_tpu[raw_num])) {
					output_buf = bnr_ai_isp_launch_tpu(vdev, raw_num,
									chn_num, b->addr);
					//for dump_raw
					if (b->is_ext == EXTERNAL_BUFFER && b->addr != output_buf) {
						_vi_release_ext_buf(b->addr);
					}
					b->addr = output_buf;
				}
				++vdev->pre_fe_frm_num[raw_num][chn_num];
				isp_buf_queue(tpu_r_q, b);
			}

			chn_num++;
		}
		complete(&vdev->tpu_done[raw_num]);

		tasklet_hi_schedule(&vdev->job_work);

		break;
	default:
		vi_pr(VI_WARN, "unknown ai_isp_type (%d)!\n", ai_isp_type);
		break;
	}
}

static int ai_isp_resolve_cfg(struct sop_vi_dev *vdev, ai_isp_cfg_t cfg)
{
	struct isp_ctx *ctx = &vdev->ctx;
	int timeout = 200;  //ms
	int ret;
	u8 raw_num = vi_get_raw_num_by_dev(ctx, cfg.vi_pipe);

	switch (cfg.ai_isp_type) {
	case AI_ISP_TYPE_BNR:
	{
		switch (cfg.ai_isp_cfg_type) {
		case AI_ISP_PIPE_LOAD:
		{
			int access_point = 0;

			if (cfg.param_size == sizeof(access_point))
				memcpy(&access_point, (void *)cfg.param_addr, cfg.param_size);

			if (access_point == RAW_AI_ISP_SPLT && raw_num != ISP_PRERAW0) {
				vi_pr(VI_WARN, "splt_ai_isp only support raw0!\n");
				break;
			}

			ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap = access_point;

			break;
		}
		case AI_ISP_PIPE_UNLOAD:
		{
			//nothing to do
			break;
		}
		case AI_ISP_CFG_INIT:
		{
			if (!ctx->is_offline_be || ctx->is_offline_postraw) {
				vi_pr(VI_WARN, "bnr ai isp need (be offline)&(post online)\n");
				return -EPERM;
			}
			if (!ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap) {
				vi_pr(VI_WARN, "need to enable raw_ai_isp_ap before streaming!\n");
				return -EPERM;
			}

			break;
		}
		case AI_ISP_CFG_DEINIT:
		{
			ret = wait_for_completion_timeout(&vdev->tpu_done[raw_num],
					msecs_to_jiffies(timeout));
			if (ret == 0) {
				vi_pr(VI_WARN, "wait raw_%d tpu_done timeout(%d)!\n",
						raw_num, timeout);
			}

			break;
		}
		case AI_ISP_CFG_ENABLE:
		{
			ctx->isp_pipe_cfg[raw_num].raw_ai_isp_frm_cnt = 0;
			atomic_set(&vdev->bnr_run_tpu[raw_num], 1);
			break;
		}
		case AI_ISP_CFG_DISABLE:
		{
			atomic_set(&vdev->bnr_run_tpu[raw_num], 0);

			atomic_set(&vdev->ai_isp_int_flag[raw_num], 2);
			wake_up(&vdev->ai_isp_wait_q[raw_num]);

			ret = wait_for_completion_timeout(&vdev->tpu_done[raw_num],
					msecs_to_jiffies(timeout));
			if (ret == 0) {
				vi_pr(VI_WARN, "wait raw_%d tpu_done timeout(%d)!\n",
						raw_num, timeout);
			}

			break;
		}
		default:
			break;
		}
		break;
	}

	default:
		break;
	}

	return 0;
}

static inline void _vi_wake_up_tpu_th(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum ai_isp_type_e type)
{
	enum E_VI_TH tpu_th_id = vdev->tpu_thd_bind[raw_num];

	if (tpu_th_id < E_VI_TH_RUN_TPU1 || tpu_th_id > E_VI_TH_RUN_TPU2) {
		vi_pr(VI_ERR, "invalid thread_id(%d) for raw_%d\n",
			tpu_th_id, raw_num);
		return;
	}

	atomic_set(&vdev->ai_isp_type, type);
	vdev->vi_th[tpu_th_id].flag = raw_num + 1;
	wake_up(&vdev->vi_th[tpu_th_id].wq);
}

static int _vi_run_tpu_thread1(void *arg)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum E_VI_TH th_id = E_VI_TH_RUN_TPU1;

	while (1) {
		wait_event(vdev->vi_th[th_id].wq, vdev->vi_th[th_id].flag != 0
			|| kthread_should_stop());

		if (vdev->vi_th[th_id].flag != 0) {
			raw_num = vdev->vi_th[th_id].flag - 1;
			vdev->vi_th[th_id].flag = 0;
		}

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		ai_isp_handle_process(vdev, raw_num);
	}

	return 0;
}

static int _vi_run_tpu_thread2(void *arg)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum E_VI_TH th_id = E_VI_TH_RUN_TPU2;

	while (1) {
		wait_event(vdev->vi_th[th_id].wq, vdev->vi_th[th_id].flag != 0
			|| kthread_should_stop());

		if (vdev->vi_th[th_id].flag != 0) {
			raw_num = vdev->vi_th[th_id].flag - 1;
			vdev->vi_th[th_id].flag = 0;
		}

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		ai_isp_handle_process(vdev, raw_num);
	}

	return 0;
}

static int _pre_be_outbuf_enque(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB sensor
		struct isp_queue *be_out_q = &pre_be_out_q[hw_chn_num];
		enum isp_blk_id_t pre_be_dma = (hw_chn_num == ISP_BE_CH0) ?
						ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_LE :
						ISP_BLK_ID_DMA_CTL_PRE_RAW_BE_SE;
		struct isp_buffer *b = NULL;

		b = isp_buf_next(be_out_q);
		if (!b) {
			vi_pr(VI_DBG, "pre_be chn_num_%d outbuf is empty\n", hw_chn_num);
			return 0;
		}

		ispblk_dma_setaddr(ctx, raw_num, pre_be_dma, b->addr);

		if (ctx->isp_pipe_cfg[b->raw_num].is_rgbir_sensor) {
			isp_bufpool[b->raw_num].pre_be_ir_busy_idx = b->ir_idx;

			if (hw_chn_num == ISP_BE_CH0)
				ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_LE,
							isp_bufpool[raw_num].ir_le[b->ir_idx]);
			else
				ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_SE,
							isp_bufpool[raw_num].ir_se[b->ir_idx]);
		}
	} else if (ctx->is_multi_sensor &&
		   ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB+YUV sensor
		enum isp_blk_id_t pre_fe_dma;
		struct isp_queue *fe_out_q = &pre_fe_out_q[raw_num][hw_chn_num];
		struct isp_buffer *b = NULL;

		b = isp_buf_next(fe_out_q);
		if (!b) {
			vi_pr(VI_DBG, "pre_fe_%d buf_chn_num_%d outbuf is empty\n", raw_num, hw_chn_num);
			return 0;
		}

		if (ctx->isp_pipe_cfg[raw_num].is_bt_demux)
			pre_fe_dma = csibdg_lite_dma_find_hwid(raw_num, hw_chn_num);
		else
			pre_fe_dma = csibdg_dma_find_hwid(raw_num, hw_chn_num);

		ispblk_dma_setaddr(ctx, raw_num, pre_fe_dma, b->addr);
	}

	return 1;
}

static int _pre_fe_inbuf_enque(struct sop_vi_dev *vdev,
			       const enum sop_isp_raw raw_num,
			       const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_queue *splt_r_q = NULL;
	struct isp_buffer *b = NULL;
	u64 splt_rdma_addr = 0;
	u32 splt_fe0_r = (chn_num == ISP_FE_CH0) ?
				ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_LE :
				ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_SE;
	u32 splt_fe1_r = (chn_num == ISP_FE_CH0) ?
				ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_LE :
				ISP_BLK_ID_DMA_CTL_SPLT_FE1_RDMA_SE;

	if (_is_right_tile(ctx, raw_num)) {
		splt_rdma_addr = ispblk_dma_getaddr(ctx, splt_fe0_r);
		splt_r_q = &pre_fe_in_q[ISP_PRERAW0][chn_num];
		b = isp_buf_next(splt_r_q);
		if (!b) {
			vi_pr(VI_INFO, "pre_fe_%d chn_num_%d inbuf is empty\n", raw_num, chn_num);
			return 0;
		}

		if (splt_rdma_addr != b->addr) {
			vi_pr(VI_INFO, "pre_fe0/fe1 addr mismatch buffer=0x%llx, b->addr=0x%llx\n",
			      splt_rdma_addr, b->addr);
			return 0;
		}

		ispblk_dma_setaddr(ctx, raw_num, splt_fe1_r, b->addr + (ctx->tile_cfg.r_in.start * 3) / 2);
	} else {
		splt_r_q = &pre_fe_in_q[raw_num][chn_num];
		b = isp_buf_next(splt_r_q);
		if (!b) {
			vi_pr(VI_INFO, "pre_fe_%d chn_num_%d inbuf is empty\n", raw_num, chn_num);
			return 0;
		}

		ispblk_dma_setaddr(ctx, raw_num, splt_fe0_r, b->addr);
	}

	return 1;
}

static int _pre_fe_outbuf_enque(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB sensor
		struct isp_queue *fe_out_q = &pre_fe_out_q[raw_num][chn_num];
		enum isp_blk_id_t pre_fe_dma;
		struct isp_buffer *b = NULL;
		bool trigger = false;
		u32 dmaid_le, dmaid_se;

		dmaid_le = csibdg_dma_find_hwid(raw_num, ISP_FE_CH0);
		dmaid_se = csibdg_dma_find_hwid(raw_num, ISP_FE_CH1);

		if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) {//raw_dump flow
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				trigger = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] ==
						vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1];
			} else {
				trigger = true;
			}

			if (!trigger)
				return 1;

			if (_is_right_tile(ctx, raw_num)) {
				u64 buffaddr = ispblk_dma_getaddr(ctx, csibdg_dma_find_hwid(ISP_PRERAW0, chn_num));
				fe_out_q = &raw_dump_b_q[ISP_PRERAW0];

				b = isp_buf_next(fe_out_q);
				if (b == NULL) {
					vi_pr(VI_ERR, "Pre_fe_%d LE raw_dump outbuf is empty\n", raw_num);
					return 0;
				}

				if (buffaddr != b->addr) {
					vi_pr(VI_DBG, "pre_fe0/fe1 addr mismatch buffer=0x%llx, b->addr=0x%llx\n",
						buffaddr, b->addr);
					return 0;
				}

				pre_fe_dma = (chn_num == ISP_FE_CH0) ? dmaid_le : dmaid_se;

				ispblk_dma_setaddr(ctx, raw_num, pre_fe_dma,
					buffaddr + 3 * UPPER(ctx->isp_pipe_cfg[ISP_PRERAW0].csibdg_width, 1));
			} else {
				fe_out_q = &raw_dump_b_q[raw_num];

				vi_pr(VI_DBG, "pre_fe raw_dump cfg start\n");

				b = isp_buf_next(fe_out_q);
				if (b == NULL) {
					vi_pr(VI_ERR, "Pre_fe_%d LE raw_dump outbuf is empty\n", raw_num);
					return 0;
				}

				ispblk_dma_setaddr(ctx, raw_num, dmaid_le, b->addr);

				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					struct isp_buffer *b_se = NULL;
					struct isp_queue *fe_out_q_se = &raw_dump_b_se_q[raw_num];

					b_se = isp_buf_next(fe_out_q_se);
					if (b_se == NULL) {
						vi_pr(VI_ERR, "Pre_fe_%d SE raw_dump outbuf is empty\n", raw_num);
						return 0;
					}

					ispblk_dma_setaddr(ctx, raw_num, dmaid_se, b_se->addr);
				}
			}

			atomic_set(&vdev->isp_raw_dump_en[raw_num], 2);
		} else {
			//TODO maybe we can combine code, reduce code;
			if (_is_right_tile(ctx, raw_num)) { //raw_num = ISP_PRERAW1
				//TODO we suggest that fe0 done at first
				u64 buffaddr = ispblk_dma_getaddr(ctx, csibdg_dma_find_hwid(ISP_PRERAW0, chn_num));

				fe_out_q = &pre_fe_out_q[ISP_PRERAW0][chn_num];

				b = isp_buf_next(fe_out_q);
				if (!b) {
					vi_pr(VI_DBG, "pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, chn_num);
					return 0;
				}

				if (buffaddr != b->addr) {
					vi_pr(VI_DBG, "pre_fe0/fe1 addr mismatch buffer=0x%llx, b->addr=0x%llx\n",
						buffaddr, b->addr);
					return 0;
				}

				pre_fe_dma = (chn_num == ISP_FE_CH0) ? dmaid_le : dmaid_se;

				ispblk_dma_setaddr(ctx, raw_num, pre_fe_dma,
					buffaddr + 3 * UPPER(ctx->isp_pipe_cfg[ISP_PRERAW0].csibdg_width, 1));
			} else {
				b = isp_buf_next(fe_out_q);
				if (!b) {
					vi_pr(VI_INFO, "pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, chn_num);
					return 0;
				}

				pre_fe_dma = (chn_num == ISP_FE_CH0) ? dmaid_le : dmaid_se;
				ispblk_dma_setaddr(ctx, raw_num, pre_fe_dma, b->addr);
			}
		}
	} else if (ctx->is_multi_sensor &&
		   ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB+YUV sensor
		enum isp_blk_id_t dmaid;
		struct isp_queue *fe_out_q = &pre_fe_out_q[raw_num][chn_num];
		struct isp_buffer *b = NULL;

		b = isp_buf_next(fe_out_q);
		if (!b) {
			vi_pr(VI_DBG, "pre_fe_%d buf_chn_num_%d outbuf is empty\n", raw_num, chn_num);
			return 0;
		}

		if (ctx->isp_pipe_cfg[raw_num].is_bt_demux)
			dmaid = csibdg_lite_dma_find_hwid(raw_num, chn_num);
		else
			dmaid = csibdg_dma_find_hwid(raw_num, chn_num);

		ispblk_dma_setaddr(ctx, raw_num, dmaid, b->addr);
	} else if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
		struct isp_queue *fe_out_q = &pre_fe_out_q[raw_num][chn_num];
		struct isp_buffer *b = NULL;
		u32 pre_fe_dma;

		pre_fe_dma = csibdg_dma_find_hwid(raw_num, chn_num);

		b = isp_buf_next(fe_out_q);
		if (!b) {
			vi_pr(VI_INFO, "pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, chn_num);
			return 0;
		}

		ispblk_dma_setaddr(ctx, raw_num, pre_fe_dma, b->addr);
	}

	return 1;
}

static int _postraw_inbuf_enq_check(
	struct sop_vi_dev *vdev,
	enum sop_isp_raw *raw_num,
	enum sop_isp_fe_chn_num *chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_queue *in_q = NULL, *in_se_q = NULL;
	struct isp_buffer *b = NULL, *b_se = NULL;
	int ret = 0;

	// LE
	if (_is_fe_be_online(ctx)) { //fe->be->dram->post
		in_q = &postraw_in_q[ISP_RAW_PATH_LE];
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		in_q = &pre_be_in_q;
	}

	b = isp_buf_next(in_q);
	if (b == NULL) {
		if (_is_fe_be_online(ctx)) //fe->be->dram->post
			vi_pr(VI_DBG, "Postraw input buf is empty\n");
		else if (_is_be_post_online(ctx)) //fe->dram->be->post
			vi_pr(VI_DBG, "Pre_be input buf is empty\n");
		ret = 1;
		return ret;
	}

	*raw_num = b->raw_num;
	*chn_num = b->chn_num;

	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.x = b->crop_le.x;
	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.y = b->crop_le.y;
	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.w = vdev->ctx.img_width =
							ctx->isp_pipe_cfg[b->raw_num].post_img_w;
	vdev->ctx.isp_pipe_cfg[b->raw_num].crop.h = vdev->ctx.img_height =
							ctx->isp_pipe_cfg[b->raw_num].post_img_h;

	//YUV sensor, offline return error, online than config be read dma.
	if (ctx->isp_pipe_cfg[b->raw_num].is_yuv_sensor) {
		if (ctx->isp_pipe_cfg[b->raw_num].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) {
			ret = 1;
		} else if (ctx->isp_pipe_cfg[b->raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			vi_pr(VI_DBG, "YUV fe->dram->yuv post input buf_addr = 0x%llx\n", b->addr);
			ispblk_dma_yuv_bypass_config(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA0, b->addr, b->raw_num);
		}
		vdev->ctx.isp_pipe_cfg[b->raw_num].rgbmap_i.w_bit = b->rgbmap_i.w_bit;
		vdev->ctx.isp_pipe_cfg[b->raw_num].rgbmap_i.h_bit = b->rgbmap_i.h_bit;

		vdev->ctx.isp_pipe_cfg[b->raw_num].lmap_i.w_bit = b->lmap_i.w_bit;
		vdev->ctx.isp_pipe_cfg[b->raw_num].lmap_i.h_bit = b->lmap_i.h_bit;
		return ret;
	}

	// SE
	if (_is_fe_be_online(ctx)) { //fe->be->dram->post
		in_se_q = &postraw_in_q[ISP_RAW_PATH_SE];
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		in_se_q = &pre_be_in_se_q[b->raw_num];
	}

	if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on) {
		b_se = isp_buf_next(in_se_q);
		if (b_se == NULL) {
			if (_is_fe_be_online(ctx)) //fe->be->dram->post
				vi_pr(VI_DBG, "Postraw se input buf is empty\n");
			else if (_is_be_post_online(ctx)) //fe->dram->be->post
				vi_pr(VI_DBG, "Pre_be se input buf is empty\n");
			ret = 1;
			return ret;
		}
	}

	vdev->ctx.isp_pipe_cfg[b->raw_num].rgbmap_i.w_bit = b->rgbmap_i.w_bit;
	vdev->ctx.isp_pipe_cfg[b->raw_num].rgbmap_i.h_bit = b->rgbmap_i.h_bit;

	vdev->ctx.isp_pipe_cfg[b->raw_num].lmap_i.w_bit = b->lmap_i.w_bit;
	vdev->ctx.isp_pipe_cfg[b->raw_num].lmap_i.h_bit = b->lmap_i.h_bit;
	vdev->ctx.isp_pipe_cfg[b->raw_num].ts = b->timestamp;

	if (_is_fe_be_online(ctx)) { //fe->be->dram->post
		ispblk_dma_setaddr(ctx, b->raw_num, ISP_BLK_ID_DMA_CTL_RAW_RDMA0, b->addr);
		if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on) {
			vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.x = b_se->crop_se.x;
			vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.y = b_se->crop_se.y;
			vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.w = vdev->ctx.img_width;
			vdev->ctx.isp_pipe_cfg[b->raw_num].crop_se.h = vdev->ctx.img_height;

			ispblk_dma_setaddr(ctx, b->raw_num, ISP_BLK_ID_DMA_CTL_RAW_RDMA1, b_se->addr);
		}
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		u64 addr_le, addr_se;

		addr_le = b->addr;
		if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on)
			addr_se = b_se->addr;

		if (ctx->isp_pipe_cfg[b->raw_num].is_tile) {
			ctx->img_width = ctx->tile_cfg.l_in.end - ctx->isp_pipe_cfg[b->raw_num].crop.x + 1;
			if (++vdev->postraw_proc_num == 2) {
				ctx->is_work_on_r_tile = true;
				ctx->img_width = ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_in.start + 1;
				addr_le += (ctx->tile_cfg.r_in.start - ctx->isp_pipe_cfg[b->raw_num].crop.x) * 3 / 2;
				if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on)
					addr_se += (ctx->tile_cfg.r_in.start
							- ctx->isp_pipe_cfg[b->raw_num].crop.x) * 3 / 2;
			}
		}

		vi_pr(VI_DBG, "is_right_tile %d, update addr_le 0x%llx\n", ctx->is_work_on_r_tile, addr_le);
		ispblk_dma_config(ctx, b->raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE, addr_le);
		if (ctx->isp_pipe_cfg[b->raw_num].is_hdr_on)
			ispblk_dma_config(ctx, b->raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE, addr_se);
	}

	if (ctx->isp_pipe_cfg[b->raw_num].is_tnr_ai_isp) {
		// TODO, This is for test.
		// Normally, it is up to the TPU playback.
		if (ctx->isp_pipe_cfg[b->raw_num].first_frm_cnt == 1)
			ctx->isp_pipe_cfg[b->raw_num].is_tnr_ai_isp_rdy = true;
	}

	return ret;
}

static void _postraw_outbuf_enque(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct vi_buffer *vb2_buf;
	struct sop_isp_buf *b = NULL;
	struct isp_ctx *ctx = &vdev->ctx;
	u64 dma_addr = 0;
	u64 dma_offset = 0;
	u32 dma_id = 0;
	u8 i = 0;

	//Get the buffer for postraw output buffer
	b = sop_isp_rdy_buf_next(vdev, raw_num, chn_num);
	if (b == NULL) {
		vi_pr(VI_DBG, "postraw_%d chn_%d next buffer is empty\n", raw_num, chn_num);
		return;
	}

	vb2_buf = &b->buf;

	vi_pr(VI_DBG, "update isp-buf: 0x%llx-0x%llx\n",
		vb2_buf->planes[0].addr, vb2_buf->planes[1].addr);

	for (i = 0; i < 2; i++) {
		dma_addr = (u64)vb2_buf->planes[i].addr;
		dma_id = (i == 0) ? ISP_BLK_ID_DMA_CTL_YUV_CROP_Y : ISP_BLK_ID_DMA_CTL_YUV_CROP_C;

		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			if (ctx->is_work_on_r_tile) {
				dma_offset = ctx->tile_cfg.r_out.start;
				if (ctx->isp_pipe_cfg[raw_num].is_postout_crop) {
					//only start_x less than half of the image crop width, then update offset
					dma_offset = (ctx->isp_pipe_cfg[raw_num].postout_crop.x < dma_offset) ?
						dma_offset - ctx->isp_pipe_cfg[raw_num].postout_crop.x : 0;
				}
				dma_addr += dma_offset;
			}

			ispblk_dma_config(ctx, raw_num, dma_id, dma_addr);
			vi_pr(VI_DBG, "tile mode dma_addr 0x%llx\n", dma_addr);
		} else {
			ispblk_dma_setaddr(ctx, raw_num, dma_id, dma_addr);
		}
	}
}

static u8 _postraw_outbuf_empty(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u8 hw_chn_num)
{
	u8 ret = 0;

	if (sop_isp_rdy_buf_empty(vdev, raw_num, hw_chn_num)) {
		vi_pr(VI_DBG, "postraw chn_%d output buffer is empty\n", raw_num);
		ret = 1;
	}

	return ret;
}

void _postraw_outbuf_enq(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	if (!(vdev->ctx.isp_pipe_cfg[raw_num].is_tile && vdev->ctx.is_work_on_r_tile))
		sop_isp_rdy_buf_pop(vdev, raw_num, chn_num);
	_postraw_outbuf_enque(vdev, raw_num, chn_num);
}

/*
 * for postraw offline only.
 *  trig preraw if there is output buffer in preraw output.
 */
s8 _pre_hw_enque(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum isp_blk_id_t splt_rdma_id;

	if ((atomic_read(&vdev->is_suspend) == 1) && (atomic_read(&vdev->is_suspend_pre_trig_done) == 1)) {
		vi_pr(VI_DBG, "already pre_hw_enque\n");
		return -ISP_ERROR;
	}

	//ISP frame error handling
	if (atomic_read(&vdev->isp_err_handle_flag) == 1) {
		vi_pr(VI_DBG, "wait err_handling done\n");
		return -ISP_ERROR;
	}

	if (ddr_need_retrain(vdev)) {
		ctx->isp_pipe_cfg[raw_num].is_drop_next_frame = true;
		trig_8051_if_pre_idle(vdev);
		return -ISP_DROP_FRM;
	}

#ifdef PORTING_TEST //test only
	if (!ctx->isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_fe &&
	    !ctx->isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_be &&
	    stop_stream_en) {
		vi_pr(VI_WARN, "stop_stream_en\n");
		return -ISP_STOP;
	}
#endif

	if (atomic_read(&vdev->isp_streamoff) == 0) {
		if (_is_drop_next_frame(vdev, raw_num, chn_num)) {
			vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d drop_frame_num %d\n",
					raw_num, chn_num, vdev->drop_frame_number[raw_num]);
			return -ISP_DROP_FRM;
		}

		if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { //fe->be->dram->post
			if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB sensor
				if (atomic_cmpxchg(&vdev->pre_be_state[chn_num],
							ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
							ISP_STATE_RUNNING) {
					vi_pr(VI_DBG, "Pre_be chn_num_%d is running\n", chn_num);
					return -ISP_RUNNING;
				}
			} else { //YUV sensor
				if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][chn_num],
							ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
							ISP_STATE_RUNNING) {
					vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d is running\n", raw_num, chn_num);
					return -ISP_RUNNING;
				}
			}

			// only if fe->be->dram
			if (_pre_be_outbuf_enque(vdev, raw_num, chn_num)) {
				if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) //raw_dump flow
					_isp_fe_be_raw_dump_cfg(vdev, raw_num, chn_num);
				isp_pre_trig(ctx, raw_num, chn_num);
			} else {
				if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) //RGB sensor
					atomic_set(&vdev->pre_be_state[chn_num], ISP_STATE_IDLE);
				else  //YUV sensor
					atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_STATE_IDLE);
				return -ISP_NO_BUFFER;
			}
		} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
			if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
				if (atomic_read(&vdev->isp_streamon) == 0) {
					vi_pr(VI_DBG, "VI not ready\n");
					return -ISP_STOP;
				}

				if (atomic_cmpxchg(&vdev->postraw_state,
							ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
							ISP_STATE_RUNNING) {
					vi_pr(VI_DBG, "Postraw is running\n");
					return -ISP_RUNNING;
				}

				if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler onffline mode
					if (_postraw_outbuf_empty(vdev, raw_num, chn_num)) {
						atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
						return -ISP_NO_BUFFER;
					}

					_postraw_outbuf_enq(vdev, raw_num, chn_num);
				} else { //Scaler online mode
					struct sc_cfg_cb post_para = {0};
					u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);

					/* VI Online VPSS sc cb trigger */
					post_para.snr_num = dev_num;
					post_para.is_tile = false;
					vi_fill_mlv_info(NULL, dev_num, &post_para.m_lv_i, false);
					if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
						vi_pr(VI_DBG, "snr_num_%d, sc_%d is running\n", raw_num, dev_num);
						atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
						atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
						return -ISP_RUNNING;
					}

					atomic_set(&vdev->ol_sc_frm_done, 0);
				}

				if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) //raw_dump flow
					_isp_fe_be_raw_dump_cfg(vdev, raw_num, chn_num);

				isp_pre_trig(ctx, raw_num, chn_num);
			} else if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT &&
				   !ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe) {
				if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][chn_num],
						   ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
						   ISP_STATE_RUNNING) {
					vi_pr(VI_INFO, "Pre_fe_%d chn_num_%d is running\n",
					      raw_num, chn_num);
					return -ISP_RUNNING;
				}

				if (_is_right_tile(ctx, raw_num)) {
					splt_rdma_id = (chn_num == ISP_FE_CH0) ?
							ISP_BLK_ID_SPLT_FE1_RDMA_LE :
							ISP_BLK_ID_SPLT_FE1_RDMA_SE;
				} else {
					splt_rdma_id = (chn_num == ISP_FE_CH0) ?
							ISP_BLK_ID_SPLT_FE0_RDMA_LE :
							ISP_BLK_ID_SPLT_FE0_RDMA_SE;
				}

				if (_pre_fe_inbuf_enque(vdev, raw_num, chn_num) &&
				    _pre_fe_outbuf_enque(vdev, raw_num, chn_num)) {
					ispblk_splt_rdma_ctrl_config(ctx, splt_rdma_id, true);
					isp_pre_trig(ctx, raw_num, chn_num);
				} else {
					// ispblk_splt_rdma_ctrl_config(ctx, splt_rdma_id, false);
					atomic_set(&vdev->pre_fe_state[raw_num][chn_num],
						   ISP_STATE_IDLE);
					return -ISP_NO_BUFFER;
				}
			} else {
				if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][chn_num],
							ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
							ISP_STATE_RUNNING) {
					vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d is running\n", raw_num, chn_num);
					return -ISP_RUNNING;
				}

				// only if fe->dram
				if (_pre_fe_outbuf_enque(vdev, raw_num, chn_num)) {
					isp_pre_trig(ctx, raw_num, chn_num);
				} else {
					atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_STATE_IDLE);
					return -ISP_NO_BUFFER;
				}
			}
		} else if (_is_all_online(ctx)) {
			if (atomic_cmpxchg(&vdev->postraw_state,
						ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
						ISP_STATE_RUNNING) {
				vi_pr(VI_DBG, "Postraw is running\n");
				return -ISP_RUNNING;
			}

			if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler onffline mode
				if (_postraw_outbuf_empty(vdev, raw_num, chn_num)) {
					atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
					return -ISP_NO_BUFFER;
				}

				_postraw_outbuf_enq(vdev, raw_num, chn_num);
			}

			if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) //raw_dump flow
				_isp_fe_be_raw_dump_cfg(vdev, raw_num, chn_num);

			isp_pre_trig(ctx, raw_num, chn_num);
		} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) { //fe->be->dram->post
			if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB sensor
				if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][chn_num],
							ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
							ISP_STATE_RUNNING) {
					vi_pr(VI_DBG, "Pre_fe chn_num_%d is running\n", chn_num);
					return -ISP_RUNNING;
				}
			} else { //YUV sensor
				if (atomic_cmpxchg(&vdev->pre_fe_state[raw_num][chn_num],
							ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
							ISP_STATE_RUNNING) {
					vi_pr(VI_DBG, "Pre_fe_%d chn_num_%d is running\n", raw_num, chn_num);
					return -ISP_RUNNING;
				}
			}

			if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 1) //raw_dump flow
				_isp_fe_be_raw_dump_cfg(vdev, raw_num, chn_num);
			isp_pre_trig(ctx, raw_num, chn_num);
		}
	}
	return ISP_SUCCESS;
}

static inline void _swap_post_sts_buf(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	struct _membuf *pool;
	unsigned long flags;
	u8 idx;
	u64 gms_phyaddr, ae_le_phyaddr, ae_se_phyaddr, dci_phyaddr;
	pool = &isp_bufpool[raw_num];

	//right tile don't swap idx
	if (!(ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile)) {
		spin_lock_irqsave(&pool->post_sts_lock, flags);
		if (pool->post_sts_in_use == 1) {
			spin_unlock_irqrestore(&pool->post_sts_lock, flags);
			vi_pr(VI_DBG, "post sts is in use\n");
			return;
		}
		pool->post_sts_busy_idx ^= 1;
		spin_unlock_irqrestore(&pool->post_sts_lock, flags);
	}

	if (_is_be_post_online(ctx))
		idx = pool->post_sts_busy_idx ^ 1;
	else
		idx = pool->post_sts_busy_idx;

	gms_phyaddr = pool->sts_mem[idx].gms.phy_addr;
	ae_le_phyaddr = pool->sts_mem[idx].ae_le.phy_addr;
	ae_se_phyaddr = pool->sts_mem[idx].ae_se.phy_addr;
	dci_phyaddr = pool->sts_mem[idx].dci.phy_addr;

	if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile) {
		gms_phyaddr += VI_ALIGN(GMS_SEC_SIZE);
		ae_le_phyaddr += VI_ALIGN(AE_DMA_SIZE);
		ae_se_phyaddr += VI_ALIGN(AE_DMA_SIZE);
		dci_phyaddr += VI_ALIGN(DCI_DMA_SIZE);
	}

	vi_pr(VI_DBG, "raw_%d, idx_%d ae_le_addr(%llx), ae_se_addr(%llx), gms_addr(0x%llx), dci_addr(0x%llx)\n",
		raw_num, idx, ae_le_phyaddr, ae_se_phyaddr, gms_phyaddr, dci_phyaddr);

	//gms dma
	ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_GMS, gms_phyaddr);

	//ae le dma
	ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_AE_HIST_LE, ae_le_phyaddr);
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		//ae se dma
		ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_AE_HIST_SE, ae_se_phyaddr);
	}

	//dci dma is fixed size
	ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_DCI, dci_phyaddr);
	//hist edge v dma is fixed size
	ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_HIST_EDGE_V, pool->sts_mem[idx].hist_edge_v.phy_addr);
}

static inline void _post_rgbmap_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, const u32 frm_num)
{
	u64 cur_le_r, cur_se_r, pre_le_r, pre_se_r;
	u16 w_bit = ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit;
	u8 cur_idx = (frm_num - ctx->rgbmap_prebuf_idx) % RGBMAP_BUF_IDX;
	u8 pre_idx = (frm_num - 1 + RGBMAP_BUF_IDX - ctx->rgbmap_prebuf_idx) % RGBMAP_BUF_IDX;

	cur_le_r = isp_bufpool[raw_num].rgbmap_le[cur_idx];
	if (frm_num <= ctx->rgbmap_prebuf_idx)
		pre_le_r = isp_bufpool[raw_num].rgbmap_le[0];
	else
		pre_le_r = isp_bufpool[raw_num].rgbmap_le[pre_idx];

	if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile) {
		cur_le_r += UPPER(ctx->tile_cfg.r_in.start - ctx->isp_pipe_cfg[raw_num].crop.x, w_bit) * 6;
		pre_le_r += UPPER(ctx->tile_cfg.r_in.start - ctx->isp_pipe_cfg[raw_num].crop.x, w_bit) * 6;
		ispblk_dma_config(ctx, raw_num + 1, ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R, cur_le_r);
		ispblk_dma_config(ctx, raw_num + 1, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R, pre_le_r);
	} else {
		ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_CUR_LE_R, cur_le_r);
		ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_PRE_LE_R, pre_le_r);
	}

	vi_pr(VI_DBG, "is_tile(%d), is_right(%d), cur_le_r(0x%llx), pre_le_r(0x%llx)\n",
		ctx->isp_pipe_cfg[raw_num].is_tile, ctx->is_work_on_r_tile, cur_le_r, pre_le_r);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		cur_se_r = isp_bufpool[raw_num].rgbmap_se[cur_idx];
		if (frm_num <= ctx->rgbmap_prebuf_idx)
			pre_se_r = isp_bufpool[raw_num].rgbmap_se[0];
		else
			pre_se_r = isp_bufpool[raw_num].rgbmap_se[pre_idx];

		if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile) {
			cur_se_r += UPPER(ctx->tile_cfg.r_in.start - ctx->isp_pipe_cfg[raw_num].crop.x, w_bit) * 6;
			pre_se_r += UPPER(ctx->tile_cfg.r_in.start - ctx->isp_pipe_cfg[raw_num].crop.x, w_bit) * 6;
			ispblk_dma_config(ctx, raw_num + 1, ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R, cur_se_r);
			ispblk_dma_config(ctx, raw_num + 1, ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R, pre_se_r);
		} else {
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_CUR_SE_R, cur_se_r);
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_PRE_SE_R, pre_se_r);
		}
	}
}

static inline void _post_lmap_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	u64 lmap_le = isp_bufpool[raw_num].lmap_le;
	u64 lmap_se = isp_bufpool[raw_num].lmap_se;

	if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile) {
		lmap_le = isp_bufpool[raw_num + 1].lmap_le;
		lmap_se = isp_bufpool[raw_num + 1].lmap_se;
	}

	ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LMAP_LE, lmap_le);
	ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LTM_LE, lmap_le);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LTM_SE, lmap_se);
		ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LMAP_SE, lmap_se);
	} else {
		ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LTM_SE, lmap_le);
		ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LMAP_SE, lmap_le);
	}
}

static inline void _post_mlsc_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	u64 lsc = isp_bufpool[raw_num].lsc;

	ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LSC_LE, lsc);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
		ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LSC_SE, lsc);
}

static inline void _post_ldci_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	u64 ldci_wdma = isp_bufpool[raw_num].ldci;
	u64 ldci_rdma = isp_bufpool[raw_num].ldci;

	if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile) {
		ldci_wdma = isp_bufpool[raw_num + 1].ldci;
		ldci_rdma = isp_bufpool[raw_num + 1].ldci;
	}

	ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LDCI_W, ldci_wdma);
	ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LDCI_R, ldci_rdma);
}

static inline void _post_3dnr_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	u64 manr_addr = isp_bufpool[raw_num].manr[0];
	u64 ai_isp_y_addr, ai_isp_u_addr, ai_isp_v_addr, ai_isp_manr_addr;
	u64 r_mo_addr, r_y_addr, r_uv_addr;
	u64 w_mo_addr, w_y_addr, w_uv_addr;
	u32 size;
	void *pSrc = NULL, *pDst = NULL;

	r_mo_addr = w_mo_addr = isp_bufpool[raw_num].tdnr[0];
	r_y_addr  = w_y_addr  = isp_bufpool[raw_num].tdnr[1];
	r_uv_addr = w_uv_addr = isp_bufpool[raw_num].tdnr[2];

	if (ctx->is_3dnr_on) {
		if (ctx->is_fbc_on) {
			//3dnr y
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_Y, r_y_addr);
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_Y, w_y_addr);

			//3dnr uv
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_C, r_uv_addr);
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_C, w_uv_addr);
		} else {
			if (ctx->isp_pipe_cfg[raw_num].is_tile) {
				if (ctx->is_work_on_r_tile) { //Right tile
					w_y_addr  = r_y_addr  = isp_bufpool[raw_num].tdnr_rtile[1];
					w_uv_addr = r_uv_addr = isp_bufpool[raw_num].tdnr_rtile[2];
				}
			}

			//3dnr y
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_Y, r_y_addr);
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_Y, w_y_addr);

			//3dnr uv
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_C, r_uv_addr);
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_C, w_uv_addr);
		}

		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			if (ctx->is_work_on_r_tile) {
				r_mo_addr = w_mo_addr =  isp_bufpool[raw_num].tdnr_rtile[0];
				manr_addr = isp_bufpool[raw_num].manr_rtile[0];
			}
		}
		//3dnr mo
		ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_ST_MO, r_mo_addr);
		ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_TNR_LD_MO, w_mo_addr);

		//manr
		ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_IIR_R, manr_addr);
		ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_IIR_W, manr_addr);

		//ai_isp
		if (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp &&
		    ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp_rdy) {

			ai_isp_y_addr    = isp_bufpool[raw_num].tnr_ai_isp[0];
			ai_isp_u_addr    = isp_bufpool[raw_num].tnr_ai_isp[1];
			ai_isp_v_addr    = isp_bufpool[raw_num].tnr_ai_isp[2];
			ai_isp_manr_addr = isp_bufpool[raw_num].manr[1];

			pSrc = phys_to_virt(r_y_addr);
			pDst = phys_to_virt(ai_isp_y_addr);
			size = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_Y);
			memcpy(pDst, pSrc, size);

			pSrc = phys_to_virt(r_uv_addr);
			pDst = phys_to_virt(ai_isp_u_addr);
			size = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_U);
			memcpy(pDst, pSrc, size);

			pSrc += size;
			pDst = phys_to_virt(ai_isp_v_addr);
			size = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_V);
			memcpy(pDst, pSrc, size);

			pSrc = phys_to_virt(manr_addr);
			pDst = phys_to_virt(ai_isp_manr_addr);
			size = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_AI_ISP);
			memcpy(pDst, pSrc, size);

			ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_Y, ai_isp_y_addr);
			ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_U, ai_isp_u_addr);
			ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_AI_ISP_RDMA_V, ai_isp_v_addr);
			ispblk_dma_setaddr(ctx, raw_num, ISP_BLK_ID_DMA_CTL_MMAP_AI_ISP, ai_isp_manr_addr);
		}
	}
}

static inline void _post_raw_crop_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
		if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA0, true, false);
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_RAW_RDMA1, true, false);
		}
	}
}

static inline void _post_yuv_crop_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
		if (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp &&
		    !ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp_rdy) {
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, true, true);
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, true, true);
		} else {
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, true, false);
			ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, true, false);
		}
		if (ctx->isp_pipe_cfg[raw_num].is_postout_crop) {
			struct vi_rect crop = ctx->isp_pipe_cfg[raw_num].postout_crop;

			ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_Y, ctx->isp_pipe_cfg[raw_num].crop);
			crop.x >>= 1;
			crop.y >>= 1;
			crop.w >>= 1;
			crop.h >>= 1;
			ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_C, crop);
		} else {
			if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				struct vi_rect crop;
				crop.x = 0;
				crop.y = 0;
				crop.w = ctx->img_width;
				crop.h = ctx->img_height;
				ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_Y, crop);
				crop.x >>= 1;
				crop.y >>= 1;
				crop.w >>= 1;
				crop.h >>= 1;
				ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_C, crop);
			}
			ispblk_crop_enable(ctx, ISP_BLK_ID_YUV_CROP_Y, false);
			ispblk_crop_enable(ctx, ISP_BLK_ID_YUV_CROP_C, false);
		}

		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			struct vi_rect crop = {0};

			if (!ctx->is_work_on_r_tile) { //left tile
				crop.w = ctx->tile_cfg.l_out.end - ctx->tile_cfg.l_out.start + 1;
				crop.h = ctx->img_height;

				if (ctx->isp_pipe_cfg[raw_num].is_postout_crop) {
					if (ctx->isp_pipe_cfg[raw_num].postout_crop.x >= ctx->tile_cfg.r_out.start) {
						//it's means left tile dma disable
						ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, true, true);
						ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, true, true);
					} else {
						crop.x = ctx->isp_pipe_cfg[raw_num].postout_crop.x;
						crop.y = ctx->isp_pipe_cfg[raw_num].postout_crop.y;
						crop.w = (crop.x + ctx->isp_pipe_cfg[raw_num].postout_crop.w
								> ctx->tile_cfg.r_out.start)
								? ctx->tile_cfg.r_out.start - crop.x
								: ctx->isp_pipe_cfg[raw_num].postout_crop.w;
						crop.h = ctx->isp_pipe_cfg[raw_num].postout_crop.h;
					}
				}

				vi_pr(VI_DBG, "left tile y crop x=%d y=%d w=%d h=%d",
					crop.x, crop.y, crop.w, crop.h);
				ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_Y, crop);
				crop.x >>= 1;
				crop.w >>= 1;
				if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {//offline2sc
					crop.y >>= 1;
					crop.h >>= 1;
				}
				ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_C, crop);

				vi_pr(VI_DBG, "left tile uv crop x=%d y=%d w=%d h=%d",
					crop.x, crop.y, crop.w, crop.h);
			} else { //right tile
				crop.h = ctx->img_height;
				if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
					crop.x = 0;
					crop.w = ctx->img_width;
				} else { //offline2sc
					crop.x = ctx->tile_cfg.r_out.start - ctx->tile_cfg.r_in.start;
					crop.w = ctx->tile_cfg.r_out.end - ctx->tile_cfg.r_out.start + 1;
				}

				if (ctx->isp_pipe_cfg[raw_num].is_postout_crop) {
					if (ctx->isp_pipe_cfg[raw_num].postout_crop.x +
						ctx->isp_pipe_cfg[raw_num].postout_crop.w <= ctx->tile_cfg.r_out.start) {
						//it's means right tile dma disable
						ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, true, true);
						ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, true, true);
					} else {
						crop.x = (ctx->isp_pipe_cfg[raw_num].postout_crop.x < ctx->tile_cfg.r_out.start)
							? crop.x
							: (crop.x + ctx->isp_pipe_cfg[raw_num].postout_crop.x
							- ctx->tile_cfg.r_out.start);
						crop.y = ctx->isp_pipe_cfg[raw_num].postout_crop.y;
						crop.w = (ctx->isp_pipe_cfg[raw_num].postout_crop.x < ctx->tile_cfg.r_out.start)
							? (ctx->isp_pipe_cfg[raw_num].postout_crop.x
							+ ctx->isp_pipe_cfg[raw_num].postout_crop.w
							- ctx->tile_cfg.r_out.start)
							: ctx->isp_pipe_cfg[raw_num].postout_crop.w;
						crop.h = ctx->isp_pipe_cfg[raw_num].postout_crop.h;
					}
				}

				vi_pr(VI_DBG, "right tile y crop x=%d y=%d w=%d h=%d",
					crop.x, crop.y, crop.w, crop.h);
				ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_Y, crop);
				crop.x >>= 1;
				crop.w >>= 1;
				if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {//offline2sc
					crop.y >>= 1;
					crop.h >>= 1;
				}
				ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_C, crop);

				vi_pr(VI_DBG, "right tile uv crop x=%d y=%d w=%d h=%d",
					crop.x, crop.y, crop.w, crop.h);
			}
		}
	} else {
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_Y, false, false);
		ispblk_dma_enable(ctx, ISP_BLK_ID_DMA_CTL_YUV_CROP_C, false, false);
	}
}

static inline void _post_dma_update(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB Sensor
		//Update rgbmap dma addr
		_post_rgbmap_update(ctx, raw_num, vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]);

		//update lmap dma
		_post_lmap_update(ctx, raw_num);

		//update mlsc dma
		_post_mlsc_update(ctx, raw_num);

		//update ldci dma
		_post_ldci_update(ctx, raw_num);

		//update 3dnr dma
		_post_3dnr_update(ctx, raw_num);

		//update yuv_crop dma
		_post_yuv_crop_update(ctx, raw_num);
	} else {
		//YUV Sensor
		if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			if (ctx->is_3dnr_on) {
				//Update rgbmap dma addr
				_post_rgbmap_update(ctx, raw_num, vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]);
				//update 3dnr dma
				_post_3dnr_update(ctx, raw_num);
			}
			//update yuv_crop dma
			_post_yuv_crop_update(ctx, raw_num);
			//update raw_crop dma
			_post_raw_crop_update(ctx, raw_num);
		}
	}
}

static u32 _is_fisrt_frm_after_drop(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u32 first_frm_num_after_drop = ctx->isp_pipe_cfg[raw_num].isp_reset_frm;
	u32 frm_num = 0;

	frm_num = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];

	if ((first_frm_num_after_drop != 0) && (frm_num == first_frm_num_after_drop)) {
		vi_pr(VI_DBG, "reset isp frm_num[%d]\n", frm_num);
		ctx->isp_pipe_cfg[raw_num].isp_reset_frm = 0;
		return 1;
	} else
		return 0;
}

static inline void _post_ctrl_update(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw dst_raw = (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile)
					? raw_num + 1 : raw_num;

	ispblk_post_cfg_update(ctx, raw_num);

	// ispblk_fusion_hdr_cfg(ctx, raw_num);

	_vi_clear_mmap_fbc_ring_base(vdev, raw_num);

	if (ctx->is_3dnr_on) {
		ispblk_tnr_post_chg(ctx, raw_num);

		//To set apply the prev frm or not for manr/3dnr
		if (vdev->preraw_first_frm[dst_raw] ||
		    _is_fisrt_frm_after_drop(vdev, dst_raw)) {
			vdev->preraw_first_frm[dst_raw] = false;
			isp_first_frm_reset(ctx, 1);
		} else {
			isp_first_frm_reset(ctx, 0);
		}
	}
}

static u8 _pre_be_sts_in_use_chk(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num,
				 const enum sop_isp_be_chn_num chn_num)
{
	unsigned long flags;
	static u8 be_in_use;

	if (chn_num == ISP_BE_CH0) {
		spin_lock_irqsave(&isp_bufpool[raw_num].pre_be_sts_lock, flags);
		if (isp_bufpool[raw_num].pre_be_sts_in_use == 1) {
			be_in_use = 1;
		} else {
			be_in_use = 0;
			if (!(vdev->ctx.isp_pipe_cfg[raw_num].is_tile && vdev->ctx.is_work_on_r_tile))
				isp_bufpool[raw_num].pre_be_sts_busy_idx ^= 1;
		}
		spin_unlock_irqrestore(&isp_bufpool[raw_num].pre_be_sts_lock, flags);
	}

	return be_in_use;
}

static inline void _swap_pre_be_sts_buf(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_be_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct _membuf *pool;
	u8 idx;
	u64 af_phyaddr;

	if (_pre_be_sts_in_use_chk(vdev, raw_num, chn_num) == 0) {
		pool = &isp_bufpool[raw_num];

		if (_is_be_post_online(ctx))
			idx = isp_bufpool[raw_num].pre_be_sts_busy_idx ^ 1;
		else
			idx = isp_bufpool[raw_num].pre_be_sts_busy_idx;

		if (chn_num == ISP_BE_CH0) {
			//af dma
			af_phyaddr = pool->sts_mem[idx].af.phy_addr;
			if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile)
				af_phyaddr += VI_ALIGN(AF_DMA_SIZE);

			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_AF_W, af_phyaddr);
		}
	} else {
		vi_pr(VI_DBG, "be sts is in use\n");
	}
}

static inline void _pre_be_dma_update(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u64 ir_le, ir_se;
	u8 idx = 0;

	if (ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor && _is_be_post_online(ctx)) {
		idx = isp_bufpool[raw_num].pre_be_ir_busy_idx;
		ir_le = isp_bufpool[raw_num].ir_le[idx];
		ir_se = isp_bufpool[raw_num].ir_se[idx];

		if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile) {
			ir_le += 3 * UPPER((ctx->tile_cfg.r_in.start
						- ctx->isp_pipe_cfg[raw_num].crop.x + 1) >> 1, 1);
			ispblk_dma_config(ctx, raw_num + 1, ISP_BLK_ID_DMA_CTL_RGBIR_LE, ir_le);
		} else {
			ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_LE, ir_le);
		}

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			if (ctx->isp_pipe_cfg[raw_num].is_tile && ctx->is_work_on_r_tile) {
				ir_se += 3 * UPPER((ctx->tile_cfg.r_in.start
							- ctx->isp_pipe_cfg[raw_num].crop.x + 1) >> 1, 1);
				ispblk_dma_config(ctx, raw_num + 1, ISP_BLK_ID_DMA_CTL_RGBIR_SE, ir_se);
			} else {
				ispblk_dma_config(ctx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_SE, ir_se);
			}
		}
	}
}

static inline void _pre_be_ctrl_update(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	ispblk_pre_be_cfg_update(ctx, raw_num);
}

static inline int _isp_clk_dynamic_en(struct sop_vi_dev *vdev, bool en)
{
#if 0//ToDo
	if (clk_dynamic_en && vdev->isp_clk[5]) {
		struct isp_ctx *ctx = &vdev->ctx;

		if (en && !__clk_is_enabled(vdev->isp_clk[5])) {
			if (clk_enable(vdev->isp_clk[5])) {
				vi_pr(VI_ERR, "[ERR] ISP_CLK(%s) enable fail\n", CLK_ISP_NAME[5]);
				if (_is_fe_be_online(ctx))
					atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				else if (_is_be_post_online(ctx)) {
					atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
					atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				}

				return -1;
			}

			vi_pr(VI_DBG, "enable clk(%s)\n", CLK_ISP_NAME[5]);
		} else if (!en && __clk_is_enabled(vdev->isp_clk[5])) {
			clk_disable(vdev->isp_clk[5]);

			vi_pr(VI_DBG, "disable clk(%s)\n", CLK_ISP_NAME[5]);
		}
	} else { //check isp_top_clk is enabled
		struct isp_ctx *ctx = &vdev->ctx;

		if (!__clk_is_enabled(vdev->isp_clk[5])) {
			if (clk_enable(vdev->isp_clk[5])) {
				vip_pr(ERR, "[ERR] ISP_CLK(%s) enable fail\n", CLK_ISP_NAME[5]);
				if (_is_fe_be_online(ctx))
					atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				else if (_is_be_post_online(ctx)) {
					atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
					atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				}
			}
		}
	}
#endif
	return 0;
}

/*
 * update cfg by raw_num when post_num==0, maybe will update by raw_num and chn_num.
 */
static void _postraw_update_cfg_from_user(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw = ISP_PRERAW0;
	int chn_num = 0;

	//only update [raw_num][chn0]
	for (; raw < raw_num; raw++) {
		if (!vdev->ctx.isp_pipe_enable[raw])
			continue;
		if (!ctx->isp_pipe_cfg[raw].is_yuv_sensor) {
			chn_num++;
		} else {
			chn_num += ctx->isp_pipe_cfg[raw].mux_mode + 1;
		}
	}

	//can't update crop when postraw work on r_tile
	if (ctx->isp_pipe_cfg[raw_num].is_tile && vdev->postraw_proc_num == 2)
		return;

	if (!g_vi_ctx->chn_crop[chn_num].enable) {
		ctx->isp_pipe_cfg[raw_num].is_postout_crop = false;
		return;
	}

	ctx->isp_pipe_cfg[raw_num].postout_crop.x = g_vi_ctx->chn_crop[chn_num].crop_rect.x;
	ctx->isp_pipe_cfg[raw_num].postout_crop.y = g_vi_ctx->chn_crop[chn_num].crop_rect.y;
	ctx->isp_pipe_cfg[raw_num].postout_crop.w = g_vi_ctx->chn_crop[chn_num].crop_rect.width;
	ctx->isp_pipe_cfg[raw_num].postout_crop.h = g_vi_ctx->chn_crop[chn_num].crop_rect.height;
	ctx->isp_pipe_cfg[raw_num].is_postout_crop = true;
}

/*
 * - postraw offline -
 *  trig postraw if there is in/out buffer for postraw
 * - postraw online -
 *  trig preraw if there is output buffer for postraw
 */
static void _post_hw_enque(
	struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum sop_isp_fe_chn_num chn_num = ISP_FE_CH0;

	if ((atomic_read(&vdev->is_suspend) == 1) && (atomic_read(&vdev->is_suspend_post_trig_done) == 1)) {
		vi_pr(VI_DBG, "already post_hw_enque\n");
		return;
	}

	if (atomic_read(&vdev->isp_streamoff) == 1 && !ctx->is_slice_buf_on) {
		vi_pr(VI_DBG, "stop streaming\n");
		return;
	}

	if (atomic_read(&vdev->isp_err_handle_flag) == 1) {
		vi_pr(VI_DBG, "wait err_handing done\n");
		return;
	}

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { //fe->be->dram->post
		if (atomic_cmpxchg(&vdev->postraw_state,
					ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
					ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "Postraw is running\n");
			return;
		}

		if (_postraw_inbuf_enq_check(vdev, &raw_num, &chn_num)) {
			atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
			return;
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler online mode
			struct sc_cfg_cb post_para = {0};
			u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = dev_num;
			post_para.is_tile = false;
			post_para.ts = ctx->isp_pipe_cfg[raw_num].ts;
			vi_fill_mlv_info(NULL, dev_num, &post_para.m_lv_i, false);
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_DBG, "snr_num_%d, sc_%d is running\n", raw_num, dev_num);
				atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				return;
			}

			atomic_set(&vdev->ol_sc_frm_done, 0);
		} else { //Scaler offline mode
			if (_postraw_outbuf_empty(vdev, raw_num, chn_num)) {
				atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				return;
			}

			_postraw_outbuf_enq(vdev, raw_num, chn_num);
		}

		if (_isp_clk_dynamic_en(vdev, true) < 0)
			return;

		ispblk_post_yuv_cfg_update(ctx, raw_num);

		if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //YUV sensor
			if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				postraw_tuning_update(&vdev->ctx, raw_num);

				//Update postraw dma size/addr
				_post_dma_update(vdev, raw_num);
			}
		} else { //RGB sensor
			postraw_tuning_update(&vdev->ctx, raw_num);

			//Update postraw size/ctrl flow
			_post_ctrl_update(vdev, raw_num);
			//Update postraw dma size/addr
			_post_dma_update(vdev, raw_num);
			//Update postraw stt gms/ae/hist_edge_v dma size/addr
			_swap_post_sts_buf(ctx, raw_num);
		}

		vdev->offline_raw_num = raw_num;

		ctx->cam_id = raw_num;

		isp_post_trig(ctx, raw_num);

		vi_record_post_trigger(vdev, raw_num);
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		if (atomic_cmpxchg(&vdev->pre_be_state[ISP_BE_CH0],
					ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
					ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "Pre_be ch_num_%d is running\n", ISP_BE_CH0);
			return;
		}

		if (atomic_cmpxchg(&vdev->postraw_state,
					ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
					ISP_STATE_RUNNING) {
			atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
			vi_pr(VI_DBG, "Postraw is running\n");
			return;
		}

		if (_postraw_inbuf_enq_check(vdev, &raw_num, &chn_num)) {
			atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
			atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
			return;
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler online mode
			struct sc_cfg_cb post_para = {0};
			u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = dev_num;
			post_para.is_tile = ctx->isp_pipe_cfg[raw_num].is_tile;
			post_para.ts = ctx->isp_pipe_cfg[raw_num].ts;
			if (post_para.is_tile || (tile_en && !ctx->isp_pipe_cfg[raw_num].is_yuv_sensor)) {
				post_para.is_tile		= true;
				post_para.is_left_tile		= !ctx->is_work_on_r_tile;
				post_para.l_in.start		= ctx->tile_cfg.l_in.start;
				post_para.l_in.end		= ctx->tile_cfg.l_in.end;
				post_para.l_out.start		= ctx->tile_cfg.l_out.start;
				post_para.l_out.end		= ctx->tile_cfg.l_out.end;
				post_para.r_in.start		= ctx->tile_cfg.r_in.start;
				post_para.r_in.end		= ctx->tile_cfg.r_in.end;
				post_para.r_out.start		= ctx->tile_cfg.r_out.start;
				post_para.r_out.end		= ctx->tile_cfg.r_out.end;
			}
			vi_fill_mlv_info(NULL, dev_num, &post_para.m_lv_i, false);
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_DBG, "snr_num_%d, sc_%d is running\n", raw_num, dev_num);
				atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
				atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				return;
			}

			atomic_set(&vdev->ol_sc_frm_done, 0);
		} else { //Scaler offline mode
			if (_postraw_outbuf_empty(vdev, raw_num, chn_num)) {
				atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
				atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				return;
			}

			//only update crop, offline2sc
			_postraw_update_cfg_from_user(vdev, raw_num);
			_postraw_outbuf_enq(vdev, raw_num, chn_num);
		}

		if (_isp_clk_dynamic_en(vdev, true) < 0)
			return;

		ispblk_post_yuv_cfg_update(ctx, raw_num);

		if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //YUV sensor
			if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				postraw_tuning_update(&vdev->ctx, raw_num);

				//Update postraw dma size/addr
				_post_dma_update(vdev, raw_num);
			}
		} else { //RGB sensor
			pre_be_tuning_update(&vdev->ctx, raw_num);

			//Update pre be size/ctrl flow
			_pre_be_ctrl_update(vdev, raw_num);
			//Update pre be dma size/addr
			_pre_be_dma_update(vdev, raw_num);
			//Update pre be sts size/addr
			_swap_pre_be_sts_buf(vdev, raw_num, ISP_BE_CH0);

			postraw_tuning_update(&vdev->ctx, raw_num);

			//Update postraw size/ctrl flow
			_post_ctrl_update(vdev, raw_num);
			//Update postraw dma size/addr
			_post_dma_update(vdev, raw_num);
			//Update postraw sts awb/dci/hist_edge_v dma size/addr
			_swap_post_sts_buf(ctx, raw_num);
		}

		vdev->offline_raw_num = raw_num;

		ctx->cam_id = raw_num;

		isp_post_trig(ctx, raw_num);
	} else if (_is_all_online(ctx)) { //on-the-fly

		if (atomic_read(&vdev->postraw_state) == ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "Postraw is running\n");
			return;
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler online mode
			struct sc_cfg_cb post_para = {0};
			u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);

			/* VI Online VPSS sc cb trigger */
			post_para.snr_num = dev_num;
			post_para.is_tile = false;
			vi_fill_mlv_info(NULL, dev_num, &post_para.m_lv_i, false);
			if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
				vi_pr(VI_DBG, "snr_num_%d, sc_%d is running\n", raw_num, dev_num);
				atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
				return;
			}

			atomic_set(&vdev->ol_sc_frm_done, 0);
		}

		_vi_clear_mmap_fbc_ring_base(vdev, raw_num);

		vi_tuning_gamma_ips_update(ctx, raw_num);
		vi_tuning_clut_update(ctx, raw_num);
		vi_tuning_dci_update(ctx, raw_num);
		vi_tuning_drc_update(ctx, raw_num);

		if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
		    !ctx->isp_pipe_cfg[raw_num].is_raw_replay_be)
			_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
	} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) {
		//Things have to be done in post done
		if (atomic_cmpxchg(&ctx->is_post_done, 1, 0) == 1) { //Change is_post_done flag to 0
			if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler offline mode
				if (!_postraw_outbuf_empty(vdev, raw_num, ISP_FE_CH0)) //slice must be ch0
					_postraw_outbuf_enq(vdev, raw_num, ISP_FE_CH0); //slice must be ch0
			}

			_vi_clear_mmap_fbc_ring_base(vdev, raw_num);

			vi_tuning_gamma_ips_update(ctx, raw_num);
			vi_tuning_clut_update(ctx, raw_num);
			vi_tuning_dci_update(ctx, raw_num);
			vi_tuning_drc_update(ctx, raw_num);

			if (!ctx->is_rgbmap_sbm_on) {
				//Update rgbmap dma addr
				_post_rgbmap_update(ctx, raw_num, vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]);
			}
		} else { //Things have to be done in be done for fps issue
			if (atomic_read(&vdev->isp_streamoff) == 0) {
				if (atomic_cmpxchg(&vdev->pre_be_state[ISP_BE_CH0],
							ISP_STATE_IDLE, ISP_STATE_RUNNING) ==
							ISP_STATE_RUNNING) {
					vi_pr(VI_DBG, "BE is running\n");
					return;
				}

				if (!ctx->isp_pipe_cfg[raw_num].is_offline_scaler) { //Scaler online mode
					struct sc_cfg_cb post_para = {0};
					u8 dev_num = vi_get_dev_num_by_raw(ctx, raw_num);

					/* VI Online VPSS sc cb trigger */
					post_para.snr_num = dev_num;
					post_para.is_tile = false;
					vi_fill_mlv_info(NULL, dev_num, &post_para.m_lv_i, false);
					if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_VI_ONLINE_TRIGGER, &post_para) != 0) {
						vi_pr(VI_DBG, "snr_num_%d, sc_%d is not ready\n", raw_num, dev_num);
						atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
						return;
					}

					atomic_set(&vdev->ol_sc_frm_done, 0);
				}
			}

			vdev->offline_raw_num = raw_num;
			ctx->cam_id = raw_num;

			atomic_set(&vdev->postraw_state, ISP_STATE_RUNNING);

			isp_post_trig(ctx, raw_num);
			vi_record_post_trigger(vdev, raw_num);

			if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
			    !ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
				_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
					_pre_hw_enque(vdev, raw_num, ISP_FE_CH1);
			}
		}
	}
}

static void _splt_hw_enque(struct sop_vi_dev *vdev, const enum sop_isp_raw hw_raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	struct isp_queue *splt_w_q = NULL;
	u32 splt_fe0_w;
	u32 splt_fe1_w;
	enum sop_isp_fe_chn_num chn_num, chn_max;
	enum sop_isp_raw raw_num = hw_raw_num;
	enum isp_blk_id_t blk_id = (raw_num == ISP_PRERAW0)
					? ISP_BLK_ID_SPLT_FE0_WDMA
					: ISP_BLK_ID_SPLT_FE1_WDMA;

	if (atomic_read(&vdev->isp_streamoff) == 1) {
		vi_pr(VI_DBG, "stop streaming\n");
		return;
	}

	if (atomic_read(&vdev->isp_err_handle_flag) == 1) {
		vi_pr(VI_DBG, "wait err_handing done\n");
		return;
	}

	/*disable wdma whnen ai isp ddr retrain */
	if (ddr_need_retrain(vdev)) {
		ctx->isp_pipe_cfg[raw_num].is_drop_next_frame = true;
		if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT)
			ispblk_splt_wdma_ctrl_config(ctx, blk_id, false);
		trig_8051_if_pre_idle(vdev);
		vi_pr(VI_DBG, "ddr need retrain\n");
	}

	if (_is_right_tile(ctx, raw_num))
		raw_num = ISP_PRERAW0;

	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe) {
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			if (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] !=
			    vdev->pre_fe_frm_num[raw_num + 1][ISP_FE_CH0])
				return;

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on &&
			    (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] !=
			     vdev->pre_fe_frm_num[raw_num + 1][ISP_FE_CH1]))
				return;
		} else {
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on &&
			    (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] !=
			     vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]))
				return;
		}

		isp_splt_trig(ctx, raw_num);
	} else if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap) {
		if (atomic_read(&vdev->splt_state[raw_num][ISP_BE_CH0]) == ISP_STATE_RUNNING) {
			vi_pr(VI_DBG, "splt_%d chn_%d is running\n", raw_num, ISP_BE_CH0);
			return;
		}
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			if (atomic_read(&vdev->splt_state[raw_num][ISP_BE_CH1]) == ISP_STATE_RUNNING) {
				vi_pr(VI_DBG, "splt_%d chn_%d is running\n", raw_num, ISP_BE_CH1);
				return;
			}
		}

		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			if (atomic_read(&vdev->splt_state[raw_num + 1][ISP_BE_CH0]) == ISP_STATE_RUNNING) {
				vi_pr(VI_DBG, "splt_%d chn_%d is running\n", raw_num + 1, ISP_BE_CH0);
				return;
			}
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				if (atomic_read(&vdev->splt_state[raw_num + 1][ISP_BE_CH1]) == ISP_STATE_RUNNING) {
					vi_pr(VI_DBG, "splt_%d chn_%d is running\n", raw_num + 1, ISP_BE_CH1);
					return;
				}
			}
		}

		_vi_splt_set_all_state(vdev, raw_num, ISP_STATE_RUNNING);

		chn_max = (ctx->isp_pipe_cfg[raw_num].is_hdr_on) ? ISP_FE_CH1 : ISP_FE_CH0;

		for (chn_num = ISP_FE_CH0; chn_num <= chn_max; chn_num++) {
			splt_w_q = &splt_out_q[raw_num][chn_num];
			splt_fe0_w = (chn_num == ISP_FE_CH0) ?
					ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_LE : ISP_BLK_ID_DMA_CTL_SPLT_FE0_WDMA_SE;
			splt_fe1_w = (chn_num == ISP_FE_CH0) ?
					ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_LE : ISP_BLK_ID_DMA_CTL_SPLT_FE1_WDMA_SE;

			// set wdma address
			b = isp_buf_next(splt_w_q);
			if (!b) {
				vi_pr(VI_ERR, "splt_out_%d chn_num_%d is empty, use previous wdma addr.\n",
						raw_num, ISP_FE_CH0);
				ctx->isp_pipe_cfg[raw_num].is_fake_splt_wdma = true;
			} else {
				ispblk_dma_setaddr(ctx, raw_num, splt_fe0_w, b->addr);
				ctx->isp_pipe_cfg[raw_num].is_fake_splt_wdma = false;
			}
		}

		isp_splt_trig(ctx, raw_num);
	} else if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		if (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] !=
		    vdev->pre_fe_frm_num[raw_num + 1][ISP_FE_CH0])
			return;

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on &&
		    (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] !=
		     vdev->pre_fe_frm_num[raw_num + 1][ISP_FE_CH1]))
			return;

		vi_pr(VI_DBG, "in tile mode, trig spliter raw_num = %d/%d\n", raw_num, raw_num + 1);
		isp_splt_trig(ctx, raw_num);
	} else if (line_spliter_en) {
		isp_splt_trig(ctx, raw_num);
	}
}

static void _pre_fe_rgbmap_update(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u8 rgbmap_idx = (vdev->pre_fe_frm_num[raw_num][chn_num]) % RGBMAP_BUF_IDX;
	u32 dma_id = 0;
	u64 buffaddr = 0;

	if (chn_num == ISP_FE_CH0) {
		dma_id = rgbmap_dma_find_hwid(raw_num, ISP_RAW_PATH_LE);
		buffaddr = isp_bufpool[raw_num].rgbmap_le[rgbmap_idx];

		ispblk_dma_setaddr(ctx, raw_num, dma_id, isp_bufpool[raw_num].rgbmap_le[rgbmap_idx]);
		vi_pr(VI_DBG, "dmaid=%d buffaddr=0x%llx\n", dma_id, buffaddr);

		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			u32 grid_size = (1 << g_w_bit[raw_num]);
			u32 w = ctx->isp_pipe_cfg[raw_num].crop.w;

			dma_id = rgbmap_dma_find_hwid(raw_num + 1, ISP_RAW_PATH_LE);
			buffaddr += ((w + grid_size - 1) / grid_size) * 6;
			ispblk_dma_setaddr(ctx, raw_num, dma_id, buffaddr);
			vi_pr(VI_DBG, "dmaid=%d buffaddr=0x%llx\n", dma_id, buffaddr);
		}
	} else if (chn_num == ISP_FE_CH1) {
		dma_id = rgbmap_dma_find_hwid(raw_num, ISP_RAW_PATH_SE);
		buffaddr = isp_bufpool[raw_num].rgbmap_se[rgbmap_idx];

		ispblk_dma_setaddr(ctx, raw_num, dma_id, isp_bufpool[raw_num].rgbmap_se[rgbmap_idx]);
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			u32 grid_size = (1 << g_w_bit[raw_num]);
			u32 w = ctx->isp_pipe_cfg[raw_num].crop.w;

			dma_id = rgbmap_dma_find_hwid(raw_num + 1, ISP_RAW_PATH_SE);
			buffaddr += ((w + grid_size - 1) / grid_size) * 6;
			ispblk_dma_setaddr(ctx, raw_num, dma_id, buffaddr);
		}
	}
}

void vi_suspend(struct sop_vi_dev *vdev)
{
	union reg_isp_top_int_event2_en ev2_en;
	union reg_isp_top_int_event2_en_fe345 ev2_en_fe345;
	uintptr_t isptopb = vdev->ctx.phys_regs[ISP_BLK_ID_ISPTOP];
	u8 count = 20;
	atomic_set(&vdev->is_suspend, 1);
	ev2_en.raw = 0;
	ev2_en.bits.frame_start_enable_fe0	= 0;
	ev2_en.bits.frame_start_enable_fe1	= 0;
	ev2_en.bits.frame_start_enable_fe2	= 0;
	ev2_en_fe345.bits.frame_start_enable_fe3	= 0;
	ev2_en_fe345.bits.frame_start_enable_fe4	= 0;
	ev2_en_fe345.bits.frame_start_enable_fe5	= 0;
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_en, ev2_en.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_en_fe345, ev2_en_fe345.raw);
	if (atomic_read(&vdev->isp_streamon)) {
		while(count--) {
			if (atomic_read(&vdev->postraw_state) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH1]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH2]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH3]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH1]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH2]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH3]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW2][ISP_FE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW2][ISP_FE_CH1]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW3][ISP_FE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW3][ISP_FE_CH1]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW4][ISP_FE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW4][ISP_FE_CH1]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW5][ISP_FE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW5][ISP_FE_CH1]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH1]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH2]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH3]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH1]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH2]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH3]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_be_state[ISP_BE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_be_state[ISP_BE_CH1]) == ISP_STATE_IDLE) {
					break;
				}
			msleep(5);
		}
		_vi_clk_ctrl(vdev, false);
	}
}

void vi_resume(struct sop_vi_dev *vdev)
{

	union reg_isp_top_int_event2_en ev2_en;
	union reg_isp_top_int_event2_en_fe345 ev2_en_fe345;
	uintptr_t isptopb = vdev->ctx.phys_regs[ISP_BLK_ID_ISPTOP];
	atomic_set(&vdev->is_suspend, 0);
	atomic_set(&vdev->is_suspend_pre_trig_done, 0);
	atomic_set(&vdev->is_suspend_post_trig_done, 0);
	if (vdev && atomic_read(&vdev->isp_streamon)) {
		_vi_clk_ctrl(vdev, true);
	}
	ev2_en.raw = 0;
	ev2_en.bits.frame_start_enable_fe0	= 0xF;
	ev2_en.bits.frame_start_enable_fe1	= 0xF;
	ev2_en.bits.frame_start_enable_fe2	= 0x3;
	ev2_en_fe345.bits.frame_start_enable_fe3	= 0x3;
	ev2_en_fe345.bits.frame_start_enable_fe4	= 0x3;
	ev2_en_fe345.bits.frame_start_enable_fe5	= 0x3;
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_en, ev2_en.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_en_fe345, ev2_en_fe345.raw);

}

void vi_destory_thread(struct sop_vi_dev *vdev, enum E_VI_TH th_id)
{
	if (th_id < 0 || th_id >= E_VI_TH_MAX) {
		pr_err("No such thread_id(%d)\n", th_id);
		return;
	}

	if (vdev->vi_th[th_id].w_thread != NULL) {
		int ret;

		ret = kthread_stop(vdev->vi_th[th_id].w_thread);
		if (!ret) {
			while (atomic_read(&vdev->vi_th[th_id].thread_exit) == 0) {
				pr_info("wait for %s exit\n", vdev->vi_th[th_id].th_name);
				usleep_range(5 * 1000, 10 * 1000);
			}
		}
		vdev->vi_th[th_id].w_thread = NULL;
	}
}

int vi_create_thread(struct sop_vi_dev *vdev, enum E_VI_TH th_id)
{
	struct sched_param param;
	int rc = 0;

	if (th_id < 0 || th_id >= E_VI_TH_MAX) {
		pr_err("No such thread_id(%d)\n", th_id);
		return -1;
	}

	param.sched_priority = MAX_USER_RT_PRIO - 10;

	if (vdev->vi_th[th_id].w_thread == NULL) {
		switch (th_id) {
		case E_VI_TH_PRERAW:
			strncat(vdev->vi_th[th_id].th_name, "task_isp_pre", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_preraw_thread;
			break;
		case E_VI_TH_VBLANK_HANDLER:
			strncat(vdev->vi_th[th_id].th_name, "task_isp_blank", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_vblank_handler_thread;
			break;
		case E_VI_TH_ERR_HANDLER:
			strncat(vdev->vi_th[th_id].th_name, "task_isp_err", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_err_handler_thread;
			break;
		case E_VI_TH_EVENT_HANDLER:
			strncat(vdev->vi_th[th_id].th_name, "vi_event_handler", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_event_handler_thread;
			break;
		case E_VI_TH_RUN_TPU1:
			strncat(vdev->vi_th[th_id].th_name, "ai_isp_tpu1", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_run_tpu_thread1;
			break;
		case E_VI_TH_RUN_TPU2:
			strncat(vdev->vi_th[th_id].th_name, "ai_isp_tpu2", sizeof(vdev->vi_th[th_id].th_name));
			vdev->vi_th[th_id].th_handler = _vi_run_tpu_thread2;
			break;
		default:
			pr_err("No such thread(%d)\n", th_id);
			return -1;
		}
		vdev->vi_th[th_id].w_thread = kthread_create(vdev->vi_th[th_id].th_handler,
								(void *)vdev,
								vdev->vi_th[th_id].th_name);
		if (IS_ERR(vdev->vi_th[th_id].w_thread)) {
			pr_err("Unable to start %s.\n", vdev->vi_th[th_id].th_name);
			return -1;
		}

		sched_setscheduler(vdev->vi_th[th_id].w_thread, SCHED_FIFO, &param);

		vdev->vi_th[th_id].flag = 0;
		atomic_set(&vdev->vi_th[th_id].thread_exit, 0);
		init_waitqueue_head(&vdev->vi_th[th_id].wq);
		wake_up_process(vdev->vi_th[th_id].w_thread);
	}

	return rc;
}

static void _vi_sw_init(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_proc_ctx = NULL;
	u8 i = 0, j = 0;

	vi_proc_ctx = (struct sop_vi_ctx *)(vdev->shared_mem);

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
	timer_setup(&usr_pic_timer.t, legacy_timer_emu_func, 0);
#else
	init_timer(&usr_pic_timer);
#endif

	ctx->is_tile            = false;
	ctx->is_offline_be      = true;
	ctx->is_offline_postraw = false;
	ctx->is_3dnr_on         = true;
	ctx->is_dpcm_on         = false;
	ctx->is_hdr_on          = false;
	ctx->is_multi_sensor    = false;
	ctx->is_sublvds_path    = false;
	ctx->is_fbc_on          = true;
	ctx->is_ctrl_inited     = false;
	ctx->is_slice_buf_on    = false;
	ctx->is_rgbmap_sbm_on   = false;
	ctx->is_3dnr_old2new    = false;
	ctx->rgbmap_prebuf_idx  = 1;
	ctx->cam_id             = 0;
	ctx->total_chn_num      = 0;
	ctx->gamma_tbl_idx      = 0;
	vdev->postraw_proc_num  = 0;
	vdev->tpu_thread_num    = 0;
	vdev->usr_pic_delay     = 0;
	vdev->isp_source        = ISP_SOURCE_DEV;

	if (vi_proc_ctx->vi_stt != VI_SUSPEND)
		memset(vdev->snr_info, 0, sizeof(struct sop_isp_snr_info) * ISP_PRERAW_MAX);

	for (i = 0; i < ISP_RAW_PATH_MAX; i++) {
		vdev->usr_pic_phy_addr[i] = 0;
	}

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		vdev->preraw_first_frm[i]     = true;
		vdev->postraw_frame_number[i] = 0;
		vdev->drop_frame_number[i]    = 0;
		vdev->dump_frame_number[i]    = 0;
		vdev->isp_int_flag[i]         = false;
		ctx->mmap_grid_size[i]        = 3;
		ctx->isp_pipe_enable[i]       = false;
		ctx->raw_chnstr_num[i]        = 0;

		ctx->isp_bind_info[i].is_bind = false;
		ctx->isp_bind_info[i].bind_dev_num = ISP_PRERAW_MAX;

		ctx->isp_pipe_cfg[i].raw_ai_isp_ap = 0;
		ctx->isp_pipe_cfg[i].tnr_mode = (ctx->is_3dnr_on) ?
						ISP_TNR_TYPE_OLD_MODE :
						ISP_TNR_TYPE_BYPASS_MODE;

		for (j = 0; j < ISP_FE_CHN_MAX; j++) {
			vdev->pre_fe_sof_cnt[i][j] = 0;
			vdev->pre_fe_frm_num[i][j] = 0;

			atomic_set(&vdev->pre_fe_state[i][j], ISP_STATE_IDLE);
		}

		for (j = 0; j < ISP_BE_CHN_MAX; j++) {
			vdev->pre_be_frm_num[i][j] = 0;
		}

		spin_lock_init(&snr_node_lock[i]);

		atomic_set(&vdev->isp_raw_dump_en[i], 0);
		atomic_set(&vdev->isp_smooth_raw_dump_en[i], 0);
		atomic_set(&vdev->isp_err_times[i], 0);
	}

	for (i = 0; i < ISP_SPLT_MAX; i++) {
		for (j = 0; j < ISP_SPLT_CHN_MAX; j++) {
			vdev->splt_wdma_frm_num[i][j] = 0;
			vdev->splt_rdma_frm_num[i][j] = 0;

			INIT_LIST_HEAD(&splt_out_q[i][j].rdy_queue);
			INIT_LIST_HEAD(&pre_fe_in_q[i][j].rdy_queue);
			splt_out_q[i][j].num_rdy        = 0;
			splt_out_q[i][j].raw_num        = i;
			pre_fe_in_q[i][j].num_rdy       = 0;
			pre_fe_in_q[i][j].raw_num       = i;

			atomic_set(&vdev->splt_state[i][j], ISP_STATE_IDLE);
		}
	}

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		for (j = 0; j < ISP_FE_CHN_MAX; j++) {
			INIT_LIST_HEAD(&pre_fe_out_q[i][j].rdy_queue);
			pre_fe_out_q[i][j].num_rdy      = 0;
			pre_fe_out_q[i][j].raw_num      = i;
		}

		INIT_LIST_HEAD(&raw_dump_b_dq[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_b_se_dq[i].rdy_queue);
		raw_dump_b_dq[i].num_rdy     = 0;
		raw_dump_b_dq[i].raw_num     = i;
		raw_dump_b_se_dq[i].num_rdy  = 0;
		raw_dump_b_se_dq[i].raw_num  = i;

		INIT_LIST_HEAD(&raw_dump_b_q[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_b_se_q[i].rdy_queue);
		raw_dump_b_q[i].num_rdy      = 0;
		raw_dump_b_q[i].raw_num      = i;
		raw_dump_b_se_q[i].num_rdy   = 0;
		raw_dump_b_se_q[i].raw_num   = i;

		INIT_LIST_HEAD(&isp_snr_i2c_queue[i].list);
		isp_snr_i2c_queue[i].num_rdy = 0;

		INIT_LIST_HEAD(&pre_be_in_se_q[i].rdy_queue);
		pre_be_in_se_q[i].num_rdy    = 0;

		// for ai isp init
		for (j = 0; j < ISP_FE_CHN_MAX; j++) {
			INIT_LIST_HEAD(&bnr_ai_isp_q[i][j].rdy_queue);
			bnr_ai_isp_q[i][j].num_rdy = 0;
			bnr_ai_isp_q[i][j].raw_num = i;
		}
		atomic_set(&vdev->bnr_run_tpu[i], 0);
		atomic_set(&vdev->ai_isp_int_flag[i], 0);
		init_completion(&vdev->tpu_done[i]);
		init_waitqueue_head(&vdev->ai_isp_wait_q[i]);
	}

	INIT_LIST_HEAD(&pre_raw_num_q.list);
	INIT_LIST_HEAD(&dqbuf_q.list);
	INIT_LIST_HEAD(&event_q.list);

	INIT_LIST_HEAD(&pre_be_in_q.rdy_queue);
	pre_be_in_q.num_rdy     = 0;

	for (i = 0; i < ISP_RAW_PATH_MAX; i++) {
		INIT_LIST_HEAD(&pre_be_out_q[i].rdy_queue);
		INIT_LIST_HEAD(&postraw_in_q[i].rdy_queue);
		pre_be_out_q[i].num_rdy     = 0;
		postraw_in_q[i].num_rdy     = 0;
	}

	atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
	atomic_set(&vdev->pre_be_state[ISP_BE_CH1], ISP_STATE_IDLE);
	atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
	atomic_set(&vdev->isp_streamoff, 1);
	atomic_set(&vdev->isp_err_handle_flag, 0);
	atomic_set(&vdev->ol_sc_frm_done, 1);
	atomic_set(&vdev->isp_dbg_flag, 0);
	atomic_set(&vdev->ctx.is_post_done, 0);
	atomic_set(&vdev->is_suspend, 0);
	atomic_set(&vdev->is_suspend_pre_trig_done, 0);
	atomic_set(&vdev->is_suspend_post_trig_done, 0);

	atomic_set(&vdev->ai_isp_type, AI_ISP_TYPE_BUTT);
	mutex_init(&vdev->ai_isp_lock);

	spin_lock_init(&buf_lock);
	spin_lock_init(&raw_num_lock);
	spin_lock_init(&dq_lock);
	spin_lock_init(&event_lock);
	spin_lock_init(&vdev->qbuf_lock);

	init_waitqueue_head(&vdev->isp_dq_wait_q);
	init_waitqueue_head(&vdev->isp_event_wait_q);
	init_waitqueue_head(&vdev->isp_dbg_wait_q);

	vi_tuning_sw_init();
}

static void _vi_init_param(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u8 i = 0, j = 0;

	atomic_set(&dev_open_cnt, 0);

	memset(ctx, 0, sizeof(*ctx));

	ctx->phys_regs = isp_get_phys_reg_bases();
	ctx->cam_id    = 0;

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		ctx->rgb_color_mode[i]                  = ISP_BAYER_TYPE_GB;
		ctx->isp_pipe_cfg[i].is_patgen_en       = false;
		ctx->isp_pipe_cfg[i].is_raw_replay_be   = false;
		ctx->isp_pipe_cfg[i].is_yuv_sensor      = false;
		ctx->isp_pipe_cfg[i].is_drop_next_frame = false;
		ctx->isp_pipe_cfg[i].isp_reset_frm      = 0;
		ctx->isp_pipe_cfg[i].is_422_to_420      = false;
		ctx->isp_pipe_cfg[i].max_height         = 0;
		ctx->isp_pipe_cfg[i].max_width          = 0;
		ctx->isp_pipe_cfg[i].csibdg_width       = 0;
		ctx->isp_pipe_cfg[i].csibdg_height      = 0;
		ctx->isp_pipe_cfg[i].yuv_scene_mode     = ISP_YUV_SCENE_BYPASS;

		INIT_LIST_HEAD(&raw_dump_b_dq[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_b_se_dq[i].rdy_queue);
		raw_dump_b_dq[i].num_rdy         = 0;
		raw_dump_b_dq[i].raw_num         = i;
		raw_dump_b_se_dq[i].num_rdy      = 0;
		raw_dump_b_se_dq[i].raw_num      = i;

		INIT_LIST_HEAD(&raw_dump_b_q[i].rdy_queue);
		INIT_LIST_HEAD(&raw_dump_b_se_q[i].rdy_queue);
		raw_dump_b_q[i].num_rdy          = 0;
		raw_dump_b_q[i].raw_num          = i;
		raw_dump_b_se_q[i].num_rdy       = 0;
		raw_dump_b_se_q[i].raw_num       = i;

		INIT_LIST_HEAD(&isp_snr_i2c_queue[i].list);
		isp_snr_i2c_queue[i].num_rdy     = 0;

		INIT_LIST_HEAD(&pre_be_in_se_q[i].rdy_queue);
		pre_be_in_se_q[i].num_rdy        = 0;
	}

	INIT_LIST_HEAD(&pre_raw_num_q.list);

	memset(&vdev->usr_crop, 0, sizeof(vdev->usr_crop));

	for (i = 0; i < VI_MAX_CHN_NUM; i++) {
		for (j = 0; j < VI_MAX_CHN_NUM; j++) {
			INIT_LIST_HEAD(&vdev->qbuf_list[i][j]);
			vdev->qbuf_num[i][j] = 0;
		}
	}

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		vdev->isp_int_flag[i] = false;
		init_waitqueue_head(&vdev->isp_int_wait_q[i]);
	}

	//ToDo sync_task_ext
	for (i = 0; i < ISP_PRERAW_MAX; i++)
		sync_task_init(i);

	tasklet_init(&vdev->job_work, isp_post_tasklet, (unsigned long)vdev);

	atomic_set(&vdev->isp_streamon, 0);
}

static int _vi_mempool_setup(void)
{
	int ret = 0;

	_vi_mempool_reset();

	return ret;
}

int vi_mac_clk_ctrl(struct sop_vi_dev *vdev, u8 mac_num, u8 enable)
{
	int rc = 0;

	if (mac_num >= ARRAY_SIZE(vdev->clk_mac))
		return rc;

	if (vdev->clk_mac[mac_num]) {
		if (enable) {
			if (clk_prepare_enable(vdev->clk_mac[mac_num])) {
				vi_pr(VI_ERR, "Failed to prepare and enable clk_mac(%d)\n", mac_num);
				rc = -EAGAIN;
				goto EXIT;
			}
		} else {
			if (__clk_is_enabled(vdev->clk_mac[mac_num]))
				clk_disable_unprepare(vdev->clk_mac[mac_num]);
			else
				clk_unprepare(vdev->clk_mac[mac_num]);
		}
	} else {
		vi_pr(VI_ERR, "clk_mac(%d) is null\n", mac_num);
		rc = -EAGAIN;
		goto EXIT;
	}
EXIT:
	return rc;
}


#ifndef FPGA_PORTING
static int _vi_clk_ctrl(struct sop_vi_dev *vdev, u8 enable)
{
	u8 i = 0;
	int rc = 0;

	for (i = 0; i < ARRAY_SIZE(vdev->clk_sys); ++i) {
		if (vdev->clk_sys[i]) {
			if (enable) {
				if (clk_prepare_enable(vdev->clk_sys[i])) {
					vi_pr(VI_ERR, "Failed to prepare and enable clk_sys(%d)\n", i);
					rc = -EAGAIN;
					goto EXIT;
				}
			} else {
				if (__clk_is_enabled(vdev->clk_sys[i]))
					clk_disable_unprepare(vdev->clk_sys[i]);
				else
					clk_unprepare(vdev->clk_sys[i]);
			}
		} else {
			vi_pr(VI_ERR, "clk_sys(%d) is null\n", i);
			rc = -EAGAIN;
			goto EXIT;
		}
	}

	for (i = 0; i < ARRAY_SIZE(vdev->clk_isp); ++i) {
		if (vdev->clk_isp[i]) {
			if (enable) {
				if (clk_prepare_enable(vdev->clk_isp[i])) {
					vi_pr(VI_ERR, "Failed to enable clk_isp(%d)\n", i);
					rc = -EAGAIN;
					goto EXIT;
				}
			} else {
				if (__clk_is_enabled(vdev->clk_isp[i]))
					clk_disable_unprepare(vdev->clk_isp[i]);
				else
					clk_unprepare(vdev->clk_isp[i]);
			}
		} else {
			vi_pr(VI_ERR, "clk_isp(%d) is null\n", i);
			rc = -EAGAIN;
			goto EXIT;
		}
	}

	//Set axi_isp_top_clk_en 1
	vi_sys_reg_write_mask(VI_SYS_REG_CLK_AXI_ISP_TOP_EN,
				VI_SYS_REG_CLK_AXI_ISP_TOP_EN_MASK,
				0x1 << VI_SYS_REG_CLK_AXI_ISP_TOP_EN_OFFSET);
EXIT:
	return rc;
}
#endif

void _vi_sdk_release(struct sop_vi_dev *vdev)
{
	u8 i = 0;
	struct isp_ctx *ctx = &vdev->ctx;

	vi_disable_chn(0);

	for (i = 0; i < VI_MAX_CHN_NUM; i++) {
		memset(&g_vi_ctx->chn_attr[i], 0, sizeof(vi_chn_attr_s));
		memset(&g_vi_ctx->chn_status[i], 0, sizeof(vi_chn_status_s));
		g_vi_ctx->blk_size[i] = 0;
		g_vi_ctx->bypass_frm[i] = 0;
	}

	for (i = 0; i < VI_MAX_PIPE_NUM; i++)
		g_vi_ctx->is_pipe_created[i] = false;

	for (i = 0; i < VI_MAX_DEV_NUM; i++)
		g_vi_ctx->is_dev_enable[i] = false;

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		memset(&ctx->isp_pipe_cfg[i], 0, sizeof(struct _isp_cfg));
		ctx->isp_pipe_cfg[i].is_offline_scaler = true;
	}
}

static int _vi_create_proc(struct sop_vi_dev *vdev)
{
	int ret = 0;

	/* vi proc setup */
	vdev->shared_mem = kzalloc(VI_SHARE_MEM_SIZE, GFP_ATOMIC);
	if (!vdev->shared_mem)
		return -ENOMEM;

	if (vi_proc_init(vdev, vdev->shared_mem) < 0) {
		pr_err("vi proc init failed\n");
		return -EAGAIN;
	}

	if (vi_dbg_proc_init(vdev) < 0) {
		pr_err("vi_dbg proc init failed\n");
		return -EAGAIN;
	}

	if (isp_proc_init(vdev) < 0) {
		pr_err("isp proc init failed\n");
		return -EAGAIN;
	}

	return ret;
}

static void _vi_destroy_proc(struct sop_vi_dev *vdev)
{
	vi_proc_remove();
	vi_dbg_proc_remove();
	kfree(vdev->shared_mem);
	vdev->shared_mem = NULL;

	isp_proc_remove();
}

static int _vi_get_dma_size(struct sop_vi_dev *vdev, u32 *size)
{
	struct isp_ctx *ctx = &vdev->ctx;
	u32 tmp_size = 0;
	u8 raw_num = 0;
	u8 dev_num = 0;

	tmp_size = isp_mempool.size;
	isp_mempool.base = 0xabde2000; //tmp addr only for check alignment
	isp_mempool.size = 0x40000000; //1024M
	isp_mempool.byteused = 0;

	_vi_scene_ctrl(vdev);

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		ctx->isp_pipe_cfg[raw_num].is_patgen_en = csi_patgen_en[raw_num];
		dev_num = vi_get_dev_num_by_raw(ctx, raw_num);

		if (ctx->isp_pipe_cfg[raw_num].is_patgen_en) {
			vdev->usr_fmt.width = vdev->snr_info[dev_num].snr_fmt.img_size[0].active_w;
			vdev->usr_fmt.height = vdev->snr_info[dev_num].snr_fmt.img_size[0].active_h;
			vdev->usr_crop.width = vdev->snr_info[dev_num].snr_fmt.img_size[0].active_w;
			vdev->usr_crop.height = vdev->snr_info[dev_num].snr_fmt.img_size[0].active_h;

			vdev->usr_fmt.code = ISP_BAYER_TYPE_BG;
			vdev->usr_crop.left = 0;
			vdev->usr_crop.top = 0;

			vi_pr(VI_WARN, "patgen enable, w_h(%d:%d), color mode(%d)\n",
					vdev->usr_fmt.width, vdev->usr_fmt.height, vdev->usr_fmt.code);
		}

		_vi_ctrl_init(raw_num, vdev);
	}

	if (ctx->isp_pipe_cfg[ISP_PRERAW0].is_tile) {
		ctx->isp_pipe_enable[ISP_PRERAW1] = true;
		ctx->isp_bind_info[ISP_PRERAW1].is_bind = true;
		ctx->isp_bind_info[ISP_PRERAW1].bind_dev_num = ctx->isp_bind_info[ISP_PRERAW0].bind_dev_num;
		_isp_tile_calc_size(ctx);
	}

	_vi_get_dma_buf_size(ctx);

	*size = isp_mempool.byteused;

	isp_mempool.base	= 0;
	isp_mempool.size	= tmp_size;
	isp_mempool.byteused	= 0;

	return 0;
}
/*******************************************************
 *  File operations for core
 ******************************************************/
static long _vi_s_ctrl(struct sop_vi_dev *vdev, struct vi_ext_control *p)
{
	u32 id = p->id;
	long rc = -EINVAL;
	struct isp_ctx *ctx = &vdev->ctx;

	switch (id) {
	case VI_IOCTL_SDK_CTRL:
	{
		rc = vi_sdk_ctrl(vdev, p);
		break;
	}

	case VI_IOCTL_HDR:
	{
		u8 first_raw_num = vi_get_first_raw_num(ctx);
#if defined(__CV180X__)
		if (p->value == true) {
			vi_pr(VI_ERR, "only support linear mode.\n");
			break;
		}
#endif
		ctx->is_hdr_on = p->value;
		ctx->isp_pipe_cfg[first_raw_num].is_hdr_on = p->value;
		vi_pr(VI_INFO, "HDR_ON(%d) for test\n", ctx->is_hdr_on);
		rc = 0;
		break;
	}

	case VI_IOCTL_3DNR:
	{
		ctx->is_3dnr_on = p->value;
		vi_pr(VI_INFO, "is_3dnr_on=%d\n", ctx->is_3dnr_on);
		rc = 0;
		break;
	}

	case VI_IOCTL_NEW_3DNR:
	{
		u8 first_raw_num = vi_get_first_raw_num(ctx);
		ctx->isp_pipe_cfg[first_raw_num].tnr_mode =
			p->value ? ISP_TNR_TYPE_NEW_MODE : ISP_TNR_TYPE_OLD_MODE;
		vi_pr(VI_INFO, "is_new_3dnr_on=%d\n",
			(ctx->isp_pipe_cfg[first_raw_num].tnr_mode == ISP_TNR_TYPE_NEW_MODE) ? 1 : 0);
		rc = 0;
		break;
	}

	case VI_IOCTL_TILE:
	{
		ctx->is_tile = p->value;
		ctx->isp_pipe_cfg[ISP_PRERAW0].is_tile = p->value;
		vi_pr(VI_INFO, "TILE_ON(%d)\n", ctx->is_tile);
		rc = 0;
		break;
	}

	case VI_IOCTL_COMPRESS_EN:
	{
		ctx->is_dpcm_on = p->value;
		vi_pr(VI_INFO, "ISP_COMPRESS_ON(%d)\n", ctx->is_dpcm_on);
		rc = 0;
		break;
	}

	case VI_IOCTL_AI_ISP_CFG:
	{
		ai_isp_cfg_t cfg;

		if (copy_from_user(&cfg, p->ptr, sizeof(ai_isp_cfg_t)))
			break;

		vi_pr(VI_INFO, "[ai_isp_cfg] vi_pipe(%d) algo_type=%d, cfg_type=%d\n",
		      cfg.vi_pipe, cfg.ai_isp_type, cfg.ai_isp_cfg_type);

		rc = ai_isp_resolve_cfg(vdev, cfg);
		break;
	}

	case VI_IOCTL_AI_ISP_SET_BUF:
	{
		ai_isp_api_t usr_api;
		enum ai_isp_type_e ai_isp_type;
		u8 raw_num;

		if (copy_from_user(&usr_api, p->ptr, sizeof(ai_isp_api_t)))
			break;

		ai_isp_type = usr_api.ai_isp_type;
		raw_num = vi_get_raw_num_by_dev(ctx, usr_api.vi_pipe);
		vi_pr(VI_DBG, "[ai_isp_set_buf] raw_num(%d) algo_type=%d\n", raw_num, ai_isp_type);

		switch (ai_isp_type) {
		case AI_ISP_TYPE_BNR:
			break;
		default:
			vi_pr(VI_WARN, "unknown ai isp type(%d)\n", ai_isp_type);
			break;
		}

		rc = 0;
		break;
	}

	case VI_IOCTL_STS_PUT:
	{
		u8 dev_num = 0;
		u8 raw_num = 0;
		unsigned long flags;

		dev_num = p->value;

		if (dev_num >= ISP_PRERAW_MAX)
			break;

		raw_num = vi_get_raw_num_by_dev(ctx, dev_num);

		if (raw_num >= ISP_PRERAW_MAX) {
			rc = 0;
			break;
		}

		spin_lock_irqsave(&isp_bufpool[raw_num].pre_be_sts_lock, flags);
		isp_bufpool[raw_num].pre_be_sts_in_use = 0;
		spin_unlock_irqrestore(&isp_bufpool[raw_num].pre_be_sts_lock, flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_POST_STS_PUT:
	{
		u8 dev_num = 0;
		u8 raw_num = 0;
		unsigned long flags;

		dev_num = p->value;

		if (dev_num >= ISP_PRERAW_MAX)
			break;

		raw_num = vi_get_raw_num_by_dev(ctx, dev_num);

		if (raw_num >= ISP_PRERAW_MAX) {
			rc = 0;
			break;
		}

		spin_lock_irqsave(&isp_bufpool[raw_num].post_sts_lock, flags);
		isp_bufpool[raw_num].post_sts_in_use = 0;
		spin_unlock_irqrestore(&isp_bufpool[raw_num].post_sts_lock, flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_USR_PIC_CFG:
	{
		struct sop_isp_usr_pic_cfg cfg;
		u8 first_raw_num = vi_get_first_raw_num(ctx);

		if (copy_from_user(&cfg, p->ptr, sizeof(struct sop_isp_usr_pic_cfg)))
			break;

		if ((cfg.crop.width < 32) || (cfg.crop.width > 4096) ||
		    (cfg.crop.left > cfg.crop.width) || (cfg.crop.top > cfg.crop.height)) {
			vi_pr(VI_ERR, "USR_PIC_CFG:(Invalid Param) w(%d) h(%d) x(%d) y(%d)",
				cfg.crop.width, cfg.crop.height, cfg.crop.left, cfg.crop.top);
		} else {
			vdev->usr_fmt = cfg.fmt;
			vdev->usr_crop = cfg.crop;

			vdev->ctx.isp_pipe_cfg[first_raw_num].csibdg_width  = vdev->usr_fmt.width;
			vdev->ctx.isp_pipe_cfg[first_raw_num].csibdg_height = vdev->usr_fmt.height;
			vdev->ctx.isp_pipe_cfg[first_raw_num].max_width     = vdev->usr_fmt.width;
			vdev->ctx.isp_pipe_cfg[first_raw_num].max_height    = vdev->usr_fmt.height;

			rc = 0;
		}

		break;
	}

	case VI_IOCTL_USR_PIC_ONOFF:
	{
		u8 first_raw_num = vi_get_first_raw_num(ctx);
		vdev->isp_source = p->value;

		ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_fe = (vdev->isp_source == ISP_SOURCE_FE);
		ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be = (vdev->isp_source == ISP_SOURCE_BE);

		vi_pr(VI_INFO, "vdev->isp_source=%d\n", vdev->isp_source);
		vi_pr(VI_INFO, "ISP_PRERAW%d, is_raw_replay_fe=%d, is_raw_replay_be=%d\n",
			first_raw_num,
			ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_fe,
			ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be);

		rc = 0;
		break;
	}

	case VI_IOCTL_PUT_PIPE_DUMP:
	{
		u32 raw_num = 0;

		raw_num = vi_get_raw_num_by_dev(ctx, p->value);

		if (raw_num >= ISP_PRERAW_MAX) {
			rc = 0;
			break;
		}

		if (isp_byr[raw_num]) {
			vfree(isp_byr[raw_num]);
			isp_byr[raw_num] = NULL;
		}

		if (isp_byr_se[raw_num]) {
			vfree(isp_byr_se[raw_num]);
			isp_byr_se[raw_num] = NULL;
		}

		rc = 0;
		break;
	}

	case VI_IOCTL_USR_PIC_PUT:
	{
		u8 first_raw_num = vi_get_first_raw_num(ctx);
		if (ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_fe ||
		    ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be) {
#if 1
			u64 phy_addr = p->value64;
			ispblk_dma_setaddr(ctx, first_raw_num, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE, phy_addr);
			vdev->usr_pic_phy_addr[ISP_RAW_PATH_LE] = phy_addr;
			vi_pr(VI_INFO, "\nvdev->usr_pic_phy_addr(0x%llx)\n", vdev->usr_pic_phy_addr[ISP_RAW_PATH_LE]);
			rc = 0;

			if (vdev->usr_pic_delay)
				usr_pic_timer_init(vdev);
#else //for vip_FPGA test
			u64 bufaddr = 0;
			u32 bufsize = 0;

			bufaddr = _mempool_get_addr();
			bufsize = ispblk_dma_config(ctx, ISP_PRERAW0, ISP_BLK_ID_RDMA0, bufaddr);
			_mempool_pop(bufsize);

			vi_pr(VI_WARN, "\nRDMA0 base_addr=0x%x\n", bufaddr);

			vdev->usr_pic_phy_addr = bufaddr;
			rc = 0;
#endif
		}
		break;
	}

	case VI_IOCTL_USR_PIC_TIMING:
	{
		if (p->value > 30)
			vdev->usr_pic_delay = msecs_to_jiffies(33);
		else if (p->value > 0)
			vdev->usr_pic_delay = msecs_to_jiffies(1000 / p->value);
		else
			vdev->usr_pic_delay = 0;

		if (!vdev->usr_pic_delay)
			usr_pic_time_remove();

		rc = 0;
		break;
	}

	case VI_IOCTL_ONLINE:
	{
		ctx->is_offline_postraw = !p->value;
		vi_pr(VI_INFO, "is_offline_postraw=%d\n", ctx->is_offline_postraw);
		rc = 0;
		break;
	}

	case VI_IOCTL_BE_ONLINE:
	{
		ctx->is_offline_be = !p->value;
		vi_pr(VI_INFO, "is_offline_be=%d\n", ctx->is_offline_be);
		rc = 0;
		break;
	}

	case VI_IOCTL_SET_SNR_CFG_NODE:
	{
		struct sop_isp_snr_update *snr_update;
		u8 first_raw_num = vi_get_first_raw_num(ctx);
		u8 raw_num;

		if (vdev->ctx.isp_pipe_cfg[first_raw_num].is_raw_replay_be)
			break;

		snr_update = vmalloc(sizeof(struct sop_isp_snr_update));
		if (copy_from_user(snr_update, p->ptr, sizeof(struct sop_isp_snr_update)) != 0) {
			vi_pr(VI_ERR, "SNR_CFG_NODE copy from user fail.\n");
			vfree(snr_update);
			break;
		}

		raw_num = vi_get_raw_num_by_dev(ctx, snr_update->raw_num);

		if (raw_num >= ISP_PRERAW_MAX) {
			vfree(snr_update);
			break;
		}

		if (vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_fe ||
			vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be ||
			vdev->ctx.isp_pipe_cfg[raw_num].is_patgen_en) {
			rc = 0;
			vfree(snr_update);
			break;
		}

		vi_pr(VI_DBG, "dev_num=%d, raw_num=%d, magic_num=%d, regs_num=%d, i2c_update=%d, isp_update=%d\n",
			snr_update->raw_num,
			raw_num,
			snr_update->snr_cfg_node.snsr.magic_num,
			snr_update->snr_cfg_node.snsr.regs_num,
			snr_update->snr_cfg_node.snsr.need_update,
			snr_update->snr_cfg_node.isp.need_update);

		_isp_snr_cfg_enq(snr_update, raw_num);

		vfree(snr_update);

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_SNR_INFO:
	{
		struct sop_isp_snr_info snr_info;
		u8 dev_num;

		if (copy_from_user(&snr_info, p->ptr, sizeof(struct sop_isp_snr_info)) != 0)
			break;
#if defined(__CV180X__)
		if (snr_info.raw_num >= ISP_PRERAW1) {
			vi_pr(VI_ERR, "only support single sensor.\n");
			break;
		}
#endif
		dev_num = snr_info.raw_num;
		memcpy(&vdev->snr_info[dev_num], &snr_info, sizeof(struct sop_isp_snr_info));
		vi_pr(VI_WARN, "dev_num=%d, color_mode=%d, frm_num=%d, snr_w:h=%d:%d, active_w:h=%d:%d\n",
			snr_info.raw_num,
			vdev->snr_info[dev_num].color_mode,
			vdev->snr_info[dev_num].snr_fmt.frm_num,
			vdev->snr_info[dev_num].snr_fmt.img_size[0].width,
			vdev->snr_info[dev_num].snr_fmt.img_size[0].height,
			vdev->snr_info[dev_num].snr_fmt.img_size[0].active_w,
			vdev->snr_info[dev_num].snr_fmt.img_size[0].active_h);

		rc = 0;
		break;
	}

	case VI_IOCTL_MMAP_GRID_SIZE:
	{
		struct sop_isp_mmap_grid_size m_gd_sz;
		u8 raw_num;

		if (copy_from_user(&m_gd_sz, p->ptr, sizeof(struct sop_isp_mmap_grid_size)) != 0)
			break;

		raw_num = vi_get_raw_num_by_dev(ctx, m_gd_sz.raw_num);
		m_gd_sz.grid_size = ctx->mmap_grid_size[raw_num];

		if (copy_to_user(p->ptr, &m_gd_sz, sizeof(struct sop_isp_mmap_grid_size)) != 0)
			break;

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_PROC_CONTENT:
	{
		struct isp_proc_cfg proc_cfg;
		int rval = 0;

		rval = copy_from_user(&proc_cfg, p->ptr, sizeof(struct isp_proc_cfg));
		if ((rval != 0) || (proc_cfg.buffer_size == 0))
			break;
		isp_proc_set_proc_content(proc_cfg.buffer, proc_cfg.buffer_size);

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_SC_ONLINE:
	{
		struct sop_isp_sc_online sc_online;
		u8 raw_num;

		if (copy_from_user(&sc_online, p->ptr, sizeof(struct sop_isp_sc_online)) != 0)
			break;

		raw_num = vi_get_raw_num_by_dev(ctx, sc_online.raw_num);
		//Currently both sensor are needed to be online or offline at same time.
		ctx->isp_pipe_offline_sc[sc_online.raw_num] = !sc_online.is_sc_online;
		ctx->isp_pipe_cfg[raw_num].is_offline_scaler = !sc_online.is_sc_online;
		vi_pr(VI_WARN, "dev_num_%d, raw_num_%d set is_offline_scaler:%d\n",
				  sc_online.raw_num, raw_num, !sc_online.is_sc_online);
		rc = 0;
		break;
	}

	case VI_IOCTL_AWB_STS_PUT:
	{
		rc = 0;
		break;
	}

	case VI_IOCTL_ENQ_BUF:
	{
		struct vi_buffer    buf;
		struct sop_isp_buf *qbuf;
		u8 pre_trig = false, post_trig = false;
		u8 raw_num = 0, chn_num = 0;

		if (copy_from_user(&buf, p->ptr, sizeof(buf))) {
			vi_pr(VI_ERR, "VI_IOCTL_ENQ_BUF, copy_from_user failed.\n");
			rc = -ENOMEM;
			break;
		}

		qbuf = kzalloc(sizeof(struct sop_isp_buf), GFP_ATOMIC);
		if (qbuf == NULL) {
			vi_pr(VI_ERR, "QBUF kzalloc size(%zu) fail\n", sizeof(struct sop_isp_buf));
			rc = -ENOMEM;
			break;
		}

		raw_num = vi_get_raw_num_by_dev(ctx, buf.index);
		buf.index = raw_num * VI_MAX_CHN_NUM + chn_num;
		vdev->chn_id = vdev->ctx.raw_chnstr_num[raw_num] + chn_num;
		memcpy(&qbuf->buf, &buf, sizeof(buf));

		if (_is_all_online(ctx) &&
			sop_isp_rdy_buf_empty(vdev, raw_num, chn_num) &&
			vdev->pre_fe_frm_num[raw_num][chn_num] > 0) {
			pre_trig = true;
		} else if (_is_fe_be_online(ctx)) { //fe->be->dram->post
			if (sop_isp_rdy_buf_empty(vdev, raw_num, chn_num) &&
				vdev->postraw_frame_number[raw_num] > 0) {
				vi_pr(VI_DBG, "chn_%d buf empty, trigger post\n", vdev->chn_id);
				post_trig = true;
			}
		}

		sop_isp_rdy_buf_queue(vdev, qbuf);

		if (pre_trig || post_trig)
			tasklet_hi_schedule(&vdev->job_work);

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_DMA_BUF_INFO:
	{
		struct sop_vi_dma_buf_info info;
		int rval = 0;

		rval = copy_from_user(&info, p->ptr, sizeof(struct sop_vi_dma_buf_info));
		if ((rval != 0) || (info.size == 0) || (info.paddr == 0))
			break;

		isp_mempool.base = info.paddr;
		isp_mempool.size = info.size;

		vi_pr(VI_INFO, "ISP dma buf paddr(0x%llx) size=0x%x\n",
				isp_mempool.base, isp_mempool.size);

		rc = 0;
		break;
	}

	case VI_IOCTL_START_STREAMING:
	{
		if (vi_start_streaming(vdev)) {
			vi_pr(VI_ERR, "Failed to vi start streaming\n");
			break;
		}

		atomic_set(&vdev->isp_streamon, 1);

		rc = 0;
		break;
	}

	case VI_IOCTL_STOP_STREAMING:
	{
		if (vi_stop_streaming(vdev)) {
			vi_pr(VI_ERR, "Failed to vi stop streaming\n");
			break;
		}

		atomic_set(&vdev->isp_streamon, 0);

		rc = 0;
		break;
	}

	case VI_IOCTL_SET_SLICE_BUF_EN:
	{
		ctx->is_slice_buf_on = p->value;
		vi_pr(VI_INFO, "ISP_SLICE_BUF_ON(%d)\n", ctx->is_slice_buf_on);
		rc = 0;
		break;
	}

	case VI_IOCTL_PUT_AI_ISP_RAW:
	{
		u8 raw_num;
		u8 dev_num = p->sdk_cfg.pipe;

		if (dev_num >= ISP_PRERAW_MAX)
			break;

		raw_num = vi_get_raw_num_by_dev(ctx, dev_num);

		atomic_set(&vdev->ai_isp_int_flag[raw_num], 2);
		wake_up(&vdev->ai_isp_wait_q[raw_num]);

		rc = 0;
		break;
	}

	default:
		break;
	}

	return rc;
}

static long _vi_g_ctrl(struct sop_vi_dev *vdev, struct vi_ext_control *p)
{
	u32 id = p->id;
	long rc = -EINVAL;
	struct isp_ctx *ctx = &vdev->ctx;

	switch (id) {
	case VI_IOCTL_STS_GET:
	{
		u8 dev_num;
		u8 raw_num;
		unsigned long flags;

		dev_num = p->value;

		if (dev_num >= ISP_PRERAW_MAX)
			break;

		raw_num = vi_get_raw_num_by_dev(ctx, dev_num);

		if (raw_num >= ISP_PRERAW_MAX) {
			rc = 0;
			break;
		}

		spin_lock_irqsave(&isp_bufpool[raw_num].pre_be_sts_lock, flags);
		isp_bufpool[raw_num].pre_be_sts_in_use = 1;
		p->value = isp_bufpool[raw_num].pre_be_sts_busy_idx ^ 1;
		spin_unlock_irqrestore(&isp_bufpool[raw_num].pre_be_sts_lock, flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_POST_STS_GET:
	{
		u8 dev_num;
		u8 raw_num;
		unsigned long flags;

		dev_num = p->value;

		if (dev_num >= ISP_PRERAW_MAX)
			break;

		raw_num = vi_get_raw_num_by_dev(ctx, dev_num);

		if (raw_num >= ISP_PRERAW_MAX) {
			rc = 0;
			break;
		}

		spin_lock_irqsave(&isp_bufpool[raw_num].post_sts_lock, flags);
		isp_bufpool[raw_num].post_sts_in_use = 1;
		p->value = isp_bufpool[raw_num].post_sts_busy_idx ^ 1;
		spin_unlock_irqrestore(&isp_bufpool[raw_num].post_sts_lock, flags);

		rc = 0;
		break;
	}

	case VI_IOCTL_STS_MEM:
	{
		struct sop_isp_sts_mem sts_mem[2];
		int rval = 0;
		u8 dev_num = 0;
		u8 raw_num = 0;

		if (copy_from_user(&sts_mem, p->ptr, sizeof(struct sop_isp_sts_mem) * 2) != 0)
			break;

		dev_num = sts_mem[0].raw_num;
		if (dev_num >= ISP_PRERAW_MAX) {
			vi_pr(VI_ERR, "sts_mem wrong dev_num(%d)\n", dev_num);
			break;
		}
		raw_num = vi_get_raw_num_by_dev(ctx, dev_num);
#if 0//PORTING_TEST //test only
		isp_bufpool[raw_num].sts_mem[0].ae_le.phy_addr = 0x11223344;
		isp_bufpool[raw_num].sts_mem[0].ae_le.size = 44800;
		isp_bufpool[raw_num].sts_mem[0].af.phy_addr = 0xaabbccdd;
		isp_bufpool[raw_num].sts_mem[0].af.size = 16320;
		isp_bufpool[raw_num].sts_mem[0].awb.phy_addr = 0x12345678;
		isp_bufpool[raw_num].sts_mem[0].awb.size = 71808;
#endif
		rval = copy_to_user(p->ptr,
					isp_bufpool[raw_num].sts_mem,
					sizeof(struct sop_isp_sts_mem) * 2);

		if (rval)
			vi_pr(VI_ERR, "fail copying %d bytes of ISP_STS_MEM info\n", rval);
		else
			rc = 0;
		break;
	}

	case VI_IOCTL_GET_LSC_PHY_BUF:
	{
		struct sop_vip_memblock *isp_mem;
		u8 raw_num;

		isp_mem = vmalloc(sizeof(struct sop_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}
		raw_num = vi_get_raw_num_by_dev(ctx, isp_mem->raw_num);

		if (raw_num >= ISP_PRERAW_MAX) {
			vfree(isp_mem);
			rc = 0;
			break;
		}

		isp_mem->phy_addr = isp_bufpool[raw_num].lsc;
		isp_mem->size = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_LSC_LE);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_PIPE_DUMP:
	{
		struct sop_vip_isp_raw_blk dump[2];
		u8 dev_num;

		if (copy_from_user(&dump[0], p->ptr, sizeof(struct sop_vip_isp_raw_blk) * 2) != 0)
			break;

#if 0//PORTING_TEST //test only
		dump[0].raw_dump.phy_addr = 0x11223344;
		if (copy_to_user(p->ptr, &dump[0], sizeof(struct sop_vip_isp_raw_blk) * 2) != 0)
			break;
		rc = 0;
#else
		dev_num = dump[0].raw_dump.raw_num;
		dump[0].raw_dump.raw_num = vi_get_raw_num_by_dev(ctx, dev_num);
		rc = isp_raw_dump(vdev, &dump[0]);
		if (copy_to_user(p->ptr, &dump[0], sizeof(struct sop_vip_isp_raw_blk) * 2) != 0)
			break;
#endif
		break;
	}

	case VI_IOCTL_GET_SC_ONLINE:
	{
		struct sop_isp_sc_online sc_online;
		u8 raw_num;

		if (copy_from_user(&sc_online, p->ptr, sizeof(struct sop_isp_sc_online)) != 0) {
			vi_pr(VI_ERR, "Failed to copy sop_isp_sc_online\n");
			break;
		}

		raw_num = vi_get_raw_num_by_dev(ctx, sc_online.raw_num);
		sc_online.is_sc_online = !ctx->isp_pipe_cfg[raw_num].is_offline_scaler;

		if (copy_to_user(p->ptr, &sc_online, sizeof(struct sop_isp_sc_online)) != 0) {
			vi_pr(VI_ERR, "Failed to copy sop_isp_sc_online\n");
			break;
		}

		rc = 0;
		break;
	}

	case VI_IOCTL_AWB_STS_GET:
	{
		rc = 0;
		break;
	}

	case VI_IOCTL_GET_FSWDR_PHY_BUF:
	{
		struct sop_vip_memblock *isp_mem;
		u8 raw_num;

		isp_mem = vmalloc(sizeof(struct sop_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}
		raw_num = vi_get_raw_num_by_dev(ctx, isp_mem->raw_num);


		if (raw_num >= ISP_PRERAW_MAX) {
			vfree(isp_mem);
			rc = 0;
			break;
		}

		isp_mem->size = sizeof(struct sop_vip_isp_fswdr_report);
		if (isp_bufpool[raw_num].fswdr_rpt == NULL) {
			isp_bufpool[raw_num].fswdr_rpt = kmalloc(
				isp_mem->size, GFP_DMA | GFP_KERNEL);
			if (isp_bufpool[raw_num].fswdr_rpt == NULL) {
				vi_pr(VI_ERR, "dev_%d isp_bufpool[%d].fswdr_rpt alloc size(%d) fail\n",
					isp_mem->raw_num, raw_num, isp_mem->size);
				vfree(isp_mem);
				break;
			}
		}
		isp_mem->vir_addr = isp_bufpool[raw_num].fswdr_rpt;
		isp_mem->phy_addr = virt_to_phys(isp_bufpool[raw_num].fswdr_rpt);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_SCENE_INFO:
	{
		enum isp_scene_info info = FE_ON_BE_OFF_POST_ON_SC;
		u8 raw_num = vi_get_first_raw_num(ctx);

		if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
			if (_is_fe_be_online(ctx))
				info = FE_ON_BE_OFF_POST_OFF_SC;
			else if (_is_be_post_online(ctx))
				info = FE_OFF_BE_ON_POST_OFF_SC;
			else if (_is_all_online(ctx))
				info = FE_ON_BE_ON_POST_OFF_SC;
		} else {
			if (_is_fe_be_online(ctx))
				info = FE_ON_BE_OFF_POST_ON_SC;
			else if (_is_be_post_online(ctx))
				info = FE_OFF_BE_ON_POST_ON_SC;
			else if (_is_all_online(ctx))
				info = FE_ON_BE_ON_POST_ON_SC;
		}

		p->value = info;

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_BUF_SIZE:
	{
		u32 size = 0;

		_vi_get_dma_size(vdev, &size);
		p->value = size;
		rc = 0;
		break;
	}

	case VI_IOCTL_GET_DEV_NUM:
	{
		p->value = g_vi_ctx->total_dev_num;
		rc = 0;
		break;
	}

	case VI_IOCTL_GET_TUN_ADDR:
	{
		void *tun_addr = NULL;
		u32 size;

		vi_tuning_buf_setup(&vdev->ctx);

		tun_addr = vi_get_tuning_buf_addr(&size);

		if (copy_to_user(p->ptr, tun_addr, size) != 0) {
			vi_pr(VI_ERR, "Failed to copy tun_addr\n");
			break;
		}

		rc = 0;
		break;
	}

	case VI_IOCTL_DQEVENT:
	{
		struct vi_event ev_u = {.type = VI_EVENT_MAX};
		struct vi_event_k *ev_k;
		unsigned long flags;

		spin_lock_irqsave(&event_lock, flags);
#if 0//PORTING_TEST //test only
		struct vi_event_k *ev_test;
		static u32 frm_num, type;

		ev_test = kzalloc(sizeof(*ev_test), GFP_ATOMIC);

		ev_test->ev.dev_id = 0;
		ev_test->ev.type = type++ % (VI_EVENT_MAX - 1);
		ev_test->ev.frame_sequence = frm_num++;
		ev_test->ev.timestamp = ktime_to_timeval(ktime_get());
		list_add_tail(&ev_test->list, &event_q.list);
#endif
		if (!list_empty(&event_q.list)) {
			ev_k = list_first_entry(&event_q.list, struct vi_event_k, list);
			ev_u.dev_id		= ev_k->ev.dev_id;
			ev_u.type		= ev_k->ev.type;
			ev_u.frame_sequence	= ev_k->ev.frame_sequence;
			ev_u.timestamp		= ev_k->ev.timestamp;
			list_del_init(&ev_k->list);
			kfree(ev_k);
		}
		spin_unlock_irqrestore(&event_lock, flags);

		if (copy_to_user(p->ptr, &ev_u, sizeof(struct vi_event))) {
			vi_pr(VI_ERR, "Failed to dqevent\n");
			break;
		}

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_CLUT_TBL_IDX:
	{
		u8 raw_num = 0;
		u8 dev_num = p->sdk_cfg.pipe;
		u8 tun_idx = p->value;

		if (dev_num >= ISP_PRERAW_MAX)
			break;

		raw_num = vi_get_raw_num_by_dev(ctx, dev_num);

		p->value = vi_tuning_get_clut_tbl_idx(raw_num, tun_idx);

		if (p->value < 0)
			break;
		rc = 0;
		break;
	}

	case VI_IOCTL_GET_IP_INFO:
	{
		if (copy_to_user(p->ptr, &ip_info_list, sizeof(struct ip_info) * IP_INFO_ID_MAX) != 0) {
			vi_pr(VI_ERR, "Failed to copy ip_info_list\n");
			break;
		}

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_RGBMAP_LE_PHY_BUF:
	{
		struct sop_vip_memblock *isp_mem;
		u8 raw_num;

		isp_mem = vmalloc(sizeof(struct sop_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		raw_num = vi_get_raw_num_by_dev(ctx, isp_mem->raw_num);
		isp_mem->phy_addr = isp_bufpool[raw_num].rgbmap_le[0];
		isp_mem->size = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_LE);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_RGBMAP_SE_PHY_BUF:
	{
		struct sop_vip_memblock *isp_mem;
		u8 raw_num;

		isp_mem = vmalloc(sizeof(struct sop_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		raw_num = vi_get_raw_num_by_dev(ctx, isp_mem->raw_num);
		isp_mem->phy_addr = isp_bufpool[raw_num].rgbmap_se[0];
		isp_mem->size = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_FE0_RGBMAP_SE);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_IR_LE_PHY_BUF:
	{
		struct sop_vip_memblock *isp_mem;
		u8 idx;
		u8 raw_num;

		isp_mem = vmalloc(sizeof(struct sop_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		raw_num = vi_get_raw_num_by_dev(ctx, isp_mem->raw_num);
		idx = isp_bufpool[raw_num].pre_be_ir_busy_idx ^ 1;
		isp_mem->phy_addr = isp_bufpool[raw_num].ir_le[idx];
		isp_mem->size = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_LE);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_IR_SE_PHY_BUF:
	{
		struct sop_vip_memblock *isp_mem;
		u8 idx;
		u8 raw_num;

		isp_mem = vmalloc(sizeof(struct sop_vip_memblock));
		if (copy_from_user(isp_mem, p->ptr, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		raw_num = vi_get_raw_num_by_dev(ctx, isp_mem->raw_num);
		idx = isp_bufpool[raw_num].pre_be_ir_busy_idx ^ 1;
		isp_mem->phy_addr = isp_bufpool[raw_num].ir_se[idx];
		isp_mem->size = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_RGBIR_SE);

		if (copy_to_user(p->ptr, isp_mem, sizeof(struct sop_vip_memblock)) != 0) {
			vfree(isp_mem);
			break;
		}

		vfree(isp_mem);

		rc = 0;
		break;
	}

	case VI_IOCTL_GET_AI_ISP_RAW:
	{
		int timeout = 200;
		int ret;
		u8 raw_num;
		u8 dev_num = p->sdk_cfg.pipe;

		if (dev_num >= ISP_PRERAW_MAX)
			break;

		raw_num = vi_get_raw_num_by_dev(ctx, dev_num);

		ret = wait_event_timeout(
			vdev->ai_isp_wait_q[raw_num],
			atomic_read(&vdev->ai_isp_int_flag[raw_num]) == 1,
			msecs_to_jiffies(timeout));

		if (!ret) {
			vi_pr(VI_WARN, "dev_%d wait ai_isp_raw timeout(%d ms)\n",
				dev_num, timeout);
			break;
		}

		if (copy_to_user(p->ptr, ai_isp_cfg_info[raw_num].ai_bnr_addr_pool,
				sizeof(ai_isp_cfg_info[raw_num].ai_bnr_addr_pool)) != 0) {
			break;
		}

		rc = 0;
		break;
	}

	default:
		break;
	}

	return rc;
}

int vi_get_ion_buf(struct sop_vi_dev *vdev)
{
	int ret = 0;
	u32 size = 0;
	u64 pAddr = 0;
	void *ion_v = NULL;

	ret = _vi_get_dma_size(vdev, &size);

	if (ret != 0) {
		vi_pr(VI_ERR, "vi_get_dma_size ioctl failed\n");
		return ERR_VI_NOMEM;
	}

	if (size == 0) {
		//return success and size = 0, it's means yuv sensor;
		vi_pr(VI_DBG, "yuv sensor not need dma size\n");
		return 0;
	}

	ret = base_ion_alloc(&pAddr, &ion_v, "VI_DMA_BUF", size, true);
	if (ret != 0) {
		vi_pr(VI_ERR, "VI ion alloc size(%d) failed.\n", size);
		return ERR_VI_NOMEM;
	}

	isp_mempool.base = pAddr;
	isp_mempool.size = size;

	vi_pr(VI_INFO, "ISP dma buf paddr(0x%llx) size=0x%x\n",
				isp_mempool.base, isp_mempool.size);

	return 0;
}

int vi_free_ion_buf(struct sop_vi_dev *dev)
{
	int ret = 0;

	if (isp_mempool.base) {
		ret = base_ion_free(isp_mempool.base);
		if (ret != 0) {
			vi_pr(VI_ERR, "free ion phy fail.\n");
			return ERR_SYS_ILLEGAL_PARAM;
		}
	}

	isp_mempool.base = 0;
	isp_mempool.size = 0;

	return ret;
}

long vi_ioctl(struct file *file, u_int cmd, u_long arg)
{
	struct sop_vi_dev *vdev = file->private_data;
	long ret = 0;
	struct vi_ext_control p;

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct vi_ext_control)))
		return -EINVAL;

	switch (cmd) {
	case VI_IOC_S_CTRL:
		ret = _vi_s_ctrl(vdev, &p);
		break;
	case VI_IOC_G_CTRL:
		ret = _vi_g_ctrl(vdev, &p);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	if (copy_to_user((void __user *)arg, &p, sizeof(struct vi_ext_control)))
		return -EINVAL;

	return ret;
}

int vi_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct sop_vi_dev *vdev;

	vdev = container_of(inode->i_cdev, struct sop_vi_dev, cdev);
	file->private_data = vdev;

	if (!atomic_read(&dev_open_cnt)) {
#ifndef FPGA_PORTING
		_vi_clk_ctrl(vdev, true);
#endif
		vi_init();

		_vi_sw_init(vdev);

		vi_pr(VI_INFO, "-\n");
	}

	atomic_inc(&dev_open_cnt);

	return ret;
}

int vi_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	atomic_dec(&dev_open_cnt);

	if (!atomic_read(&dev_open_cnt)) {
		struct sop_vi_dev *vdev;

		vdev = container_of(inode->i_cdev, struct sop_vi_dev, cdev);

		_vi_sdk_release(vdev);

#ifndef FPGA_PORTING
		_vi_clk_ctrl(vdev, false);
#endif

		vi_pr(VI_INFO, "-\n");
	}

	return ret;
}

int vi_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct sop_vi_dev *vdev;
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos;

	vdev = file->private_data;
	pos = vdev->shared_mem;

	if ((vm_size + offset) > VI_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		vi_pr(VI_DBG, "vi proc mmap vir(%p) phys(%#llx)\n",
		      pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

unsigned int vi_poll(struct file *file, struct poll_table_struct *wait)
{
	struct sop_vi_dev *vdev = file->private_data;
	unsigned long req_events = poll_requested_events(wait);
	unsigned int res = 0;
	unsigned long flags;

	if (req_events & POLLPRI) {
		/*
		 * If event buf is not empty, then notify MW to DQ event.
		 * Otherwise poll_wait.
		 */
		spin_lock_irqsave(&event_lock, flags);
		if (!list_empty(&event_q.list))
			res = POLLPRI;
		else
			poll_wait(file, &vdev->isp_event_wait_q, wait);
		spin_unlock_irqrestore(&event_lock, flags);
	}

	if (req_events & POLLIN) {
		if (atomic_read(&vdev->isp_dbg_flag)) {
			res = POLLIN | POLLRDNORM;
			atomic_set(&vdev->isp_dbg_flag, 0);
		} else {
			poll_wait(file, &vdev->isp_dbg_wait_q, wait);
		}
	}

	return res;
}

int vi_cb(void *dev, enum enum_modules_id caller, u32 cmd, void *arg)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)dev;
	struct isp_ctx *ctx = &vdev->ctx;
	int rc = -1;

	switch (cmd) {
	case VI_CB_QBUF_TRIGGER:
		vi_pr(VI_INFO, "isp_ol_sc_trig_post\n");

		tasklet_hi_schedule(&vdev->job_work);

		rc = 0;
		break;
	case VI_CB_SC_FRM_DONE:
		vi_pr(VI_DBG, "sc frm done cb\n");

		atomic_set(&vdev->ol_sc_frm_done, 1);
		tasklet_hi_schedule(&vdev->job_work);

		rc = 0;
		break;
	case VI_CB_SET_VIVPSSMODE:
	{
		vi_vpss_mode_s vi_vpss_mode;
		u8   dev_num = 0;
		u8 vi_online = 0;

		memcpy(&vi_vpss_mode, arg, sizeof(vi_vpss_mode_s));

		vi_online = (vi_vpss_mode.mode[0] == VI_ONLINE_VPSS_ONLINE) ||
			    (vi_vpss_mode.mode[0] == VI_ONLINE_VPSS_OFFLINE);

		if (vi_online) {
			ctx->is_offline_postraw = ctx->is_offline_be = !vi_online;

			vi_pr(VI_DBG, "Caller_Mod(%d) set vi_online:%d, is_offline_postraw=%d, is_offline_be=%d\n",
					caller, vi_online, ctx->is_offline_postraw, ctx->is_offline_be);
		}

		for (dev_num = 0; dev_num < VI_MAX_DEV_NUM; dev_num++) {
			u8 is_vpss_online = (vi_vpss_mode.mode[dev_num] == VI_ONLINE_VPSS_ONLINE) ||
						  (vi_vpss_mode.mode[dev_num] == VI_OFFLINE_VPSS_ONLINE);

			ctx->isp_pipe_offline_sc[dev_num] = !is_vpss_online;
			vi_pr(VI_DBG, "raw_num_%d set is_offline_scaler:%d\n",
					dev_num, !is_vpss_online);
		}

		rc = 0;
		break;
	}
	case VI_CB_GDC_OP_DONE:
	{
		struct ldc_op_done_cfg *cfg =
			(struct ldc_op_done_cfg *)arg;

		vi_gdc_callback(cfg->param, cfg->blk);
		rc = 0;
		break;
	}
	default:
		break;
	}

	return rc;
}

/********************************************************************************
 *  VI event handler related
 *******************************************************************************/
void vi_destory_dbg_thread(struct sop_vi_dev *vdev)
{
	atomic_set(&vdev->isp_dbg_flag, 1);
	wake_up(&vdev->isp_dbg_wait_q);
}

static void _vi_timeout_chk(struct sop_vi_dev *vdev)
{
	if (++g_vi_ctx->timeout_cnt >= 2) {
		atomic_set(&vdev->isp_dbg_flag, 1);
		wake_up(&vdev->isp_dbg_wait_q);
		g_vi_ctx->timeout_cnt = 0;
	}
}
#ifdef VI_PROFILE
static void _vi_update_chn_real_frame_rate(vi_chn_status_s *vi_chn_status)
{
	u64 duration, cur_timeus;
	struct timespec64 cur_time;

	cur_time = ktime_to_timespec64(ktime_get());
	cur_timeus = cur_time.tv_sec * 1000000L + cur_time.tv_nsec / 1000L;
	duration = cur_timeus - vi_chn_status->prev_time;

	if (duration >= 1000000) {
		vi_chn_status->frame_rate = vi_chn_status->frame_num;
		vi_chn_status->frame_num = 0;
		vi_chn_status->prev_time = cur_timeus;
	}

	vi_pr(VI_DBG, "FrameRate=%d\n", vi_chn_status->frame_rate);
}
#endif
static int _vi_event_handler_thread(void *arg)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
#ifdef FPGA_PORTING
	u32 timeout = 100000;//ms
#else
	u32 timeout = 500;//ms
#endif
	int ret = 0, flag = 0, ret2 = 0;
	enum E_VI_TH th_id = E_VI_TH_EVENT_HANDLER;
	mmf_chn_s chn = {.mod_id = ID_VI, .dev_id = 0, .chn_id = 0};
	mmf_chn_s isp_chn = {.mod_id = ID_VI, .dev_id = 0, .chn_id = 0};
#ifdef VI_PROFILE
	struct timespec64 time[2];
	u32 sum = 0, duration, duration_max = 0, duration_min = 1000 * 1000;
	u8 count = 0;
#endif

	while (1) {
#ifdef VI_PROFILE
		_vi_update_chn_real_frame_rate(&g_vi_ctx->chn_status[chn.chn_id]);
		time[0] = ktime_to_timespec64(ktime_get());
#endif
		ret = wait_event_timeout(vdev->vi_th[th_id].wq,
					vdev->vi_th[th_id].flag != 0 || kthread_should_stop(),
					msecs_to_jiffies(timeout) - 1);

		if (vdev->vi_th[th_id].flag != 0) {
			flag = vdev->vi_th[th_id].flag - 1;
			vdev->vi_th[th_id].flag = 0;
		}

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		if (!ret) {
			if (!vdev->usr_pic_delay) {
				continue;
			}
			vi_pr(VI_ERR, "vi_event_handler timeout(%d)ms\n", timeout);
			_vi_timeout_chk(vdev);
			continue;
		} else {
			struct _vi_buffer b;
			vb_blk blk = 0;
			struct gdc_mesh *pmesh = NULL;
			struct vb_s *vb = NULL;

			//DQbuf from list.
			if (vi_dqbuf(&b) == -1) {
				vi_pr(VI_WARN, "illegal wakeup raw_num[%d]\n", flag);
				continue;
			}

			chn.dev_id = 0;
			chn.chn_id = b.chn_id;
			ret2 = vb_dqbuf(chn, &vi_jobs[chn.chn_id], &blk);
			if (ret2 != 0) {
				if (blk == VB_INVALID_HANDLE)
					vi_pr(VI_ERR, "chn(%d) can't get vb-blk.\n", chn.chn_id);
				continue;
			}

			vb = (struct vb_s *)blk;

			vb->buf.dev_num = 0;
			vb->buf.frm_num = b.sequence;
			vb->buf.pts = (u64)b.timestamp.tv_sec * 1000000 + b.timestamp.tv_nsec / 1000; //microsec

			g_vi_ctx->chn_status[chn.chn_id].int_cnt++;
			g_vi_ctx->chn_status[chn.chn_id].frame_num++;
			g_vi_ctx->chn_status[chn.chn_id].recv_pic = b.sequence;

			vi_pr(VI_DBG, "dqbuf out_chn_id=%d, frm_num=%d\n", b.chn_id, b.sequence);

			if (g_vi_ctx->bypass_frm[chn.chn_id] >= b.sequence) {
				//Release buffer if bypass_frm is not zero
				vb_release_block(blk);
				goto QBUF;
			}
			if (!g_vi_ctx->pipe_attr[chn.chn_id].yuv_bypass_path) {
				vi_fill_mlv_info(vb, vi_get_dev_num_by_raw(&vdev->ctx, b.raw_id), NULL, true);
				vi_fill_dis_info(vb, vi_get_dev_num_by_raw(&vdev->ctx, b.raw_id));
			}

			// TODO: extchn only support works on original frame without GDC effect.
			//_vi_handle_extchn(chn.chn_id, chn, blk, &bFisheyeOn);
			//if (bFisheyeOn)
				//goto VB_DONE;

			pmesh = &g_vi_mesh[chn.chn_id];

			if (mutex_trylock(&pmesh->lock)) {
				if (g_vi_ctx->rotation[chn.chn_id] != ROTATION_0) {
					struct _vi_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_ROTATION};

					if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
						, NULL
						, vb, g_vi_ctx->chn_attr[chn.chn_id].pixel_format
						, pmesh->paddr
						, 0, &cb_param
						, sizeof(cb_param), ID_VI
						, g_vi_ctx->rotation[chn.chn_id]) != 0) {
						mutex_unlock(&pmesh->lock);
						vi_pr(VI_ERR, "gdc rotation failed.\n");
					}
					goto QBUF;
				}
				mutex_unlock(&pmesh->lock);
			} else {
				vi_pr(VI_WARN, "chn(%d) drop frame due to gdc op blocked.\n", chn.chn_id);
				// release blk if gdc not done yet
				vb_release_block(blk);
				goto QBUF;
			}
// VB_DONE:
			/* out buffer chn = buf_chn, dev = 0 */
			chn.dev_id = 0;
			vb_done_handler(chn, CHN_TYPE_OUT, &vi_jobs[chn.chn_id], blk);
QBUF:
			// get another vb for next frame
			isp_chn.chn_id = chn.chn_id - vdev->ctx.raw_chnstr_num[b.raw_id];
			isp_chn.dev_id = b.raw_id;
			/* isp buffer chn = hw_chn, dev = raw_num */
			if (vi_sdk_qbuf(isp_chn, NULL) != 0) {
				vb_pool poolid = VB_INVALID_POOLID;

				if (g_vi_ctx->chn_attr[chn.chn_id].bind_vb_pool == VB_INVALID_POOLID)
					poolid = find_vb_pool(g_vi_ctx->blk_size[chn.chn_id]);
				else
					poolid = g_vi_ctx->chn_attr[chn.chn_id].bind_vb_pool;

				if (poolid != VB_INVALID_POOLID)
					vb_acquire_block(vi_sdk_qbuf, chn, poolid, NULL);
			}
		}
#ifdef VI_PROFILE
		time[1] = ktime_to_timespec64(ktime_get());
		duration = get_diff_in_us(time[0], time[1]);
		duration_max = MAX(duration, duration_max);
		duration_min = MIN(duration, duration_min);
		sum += duration;
		if (++count == 100) {
			vi_pr(VI_DBG, "VI duration(ms): average(%d), max(%d) min(%d)\n"
				, sum / count / 1000, duration_max / 1000, duration_min / 1000);
			count = 0;
			sum = duration_max = 0;
			duration_min = 1000 * 1000;
		}
#endif
	}

	return 0;
}

/*******************************************************
 *  Irq handlers
 ******************************************************/

static void _vi_record_debug_info(struct isp_ctx *ctx)
{
	u8 i = 0;
	uintptr_t isptop = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	uintptr_t rdma28 = ctx->phys_regs[ISP_BLK_ID_DMA_CTL_RAW_RDMA0];
	struct vi_info *vi_info = NULL;

	if (g_overflow_info)
		return;

	g_overflow_info = kzalloc(sizeof(*g_overflow_info), GFP_ATOMIC);
	if (!g_overflow_info) {
		vi_pr(VI_ERR, "g_overflow_info kzalloc size(%zu) fail\n", sizeof(struct overflow_info));
		return;
	}

	vi_info = &g_overflow_info->vi_info;

#if 0
	if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on &&
	    !ctx->isp_pipe_cfg[ISP_PRERAW0].is_offline_scaler) { //VPSS online
		g_overflow_info->vpss_info.dev_num = ISP_PRERAW0;
		if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_OVERFLOW_CHECK, g_overflow_info) != 0)
			vi_pr(VI_ERR, "VPSS_CB_OVERFLOW_CHECK is failed\n");
	}
#endif

	//isp_top
	vi_info->isp_top.blk_idle = ISP_RD_REG(isptop, reg_isp_top_t, blk_idle);
	for (i = 0; i <= 6; i++) {
		//Debug
		ISP_WR_BITS(isptop, reg_isp_top_t, dummy, dbus_sel, i);
		vi_info->isp_top.dbus_sel[i].r_0 = ISP_RD_REG(isptop, reg_isp_top_t, dbus0); //0x0A070040
		vi_info->isp_top.dbus_sel[i].r_4 = ISP_RD_REG(isptop, reg_isp_top_t, dbus1); //0x0A070044
		vi_info->isp_top.dbus_sel[i].r_8 = ISP_RD_REG(isptop, reg_isp_top_t, dbus2); //0x0A070048
		vi_info->isp_top.dbus_sel[i].r_c = ISP_RD_REG(isptop, reg_isp_top_t, dbus3); //0x0A07004C
	}

	//pre_raw_fe
	vi_info->preraw_fe.preraw_info = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_info);
	vi_info->preraw_fe.fe_idle_info = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, fe_idle_info);

	//pre_raw_be
	vi_info->preraw_be.preraw_be_info = ISP_RD_REG(preraw_be, reg_pre_raw_be_t, be_info);
	vi_info->preraw_be.be_dma_idle_info = ISP_RD_REG(preraw_be, reg_pre_raw_be_t, be_dma_idle_info);
	vi_info->preraw_be.ip_idle_info = ISP_RD_REG(preraw_be, reg_pre_raw_be_t, be_ip_idle_info);
	vi_info->preraw_be.stvalid_status = ISP_RD_REG(preraw_be, reg_pre_raw_be_t, tvalid_status);
	vi_info->preraw_be.stready_status = ISP_RD_REG(preraw_be, reg_pre_raw_be_t, tready_status);

	//rawtop
	vi_info->rawtop.stvalid_status = ISP_RD_REG(rawtop, reg_raw_top_t, stvalid_status);
	vi_info->rawtop.stready_status = ISP_RD_REG(rawtop, reg_raw_top_t, stready_status);
	vi_info->rawtop.dma_idle = ISP_RD_REG(rawtop, reg_raw_top_t, dma_idle);

#if 0
	ISP_WR_BITS(isptop, reg_raw_top_t, DEBUG_SELECT, RAW_TOP_DEBUG_SELECT, 0);
	vi_pr(VI_INFO, "RAW_TOP, debug_select(h2c)=0x0, debug(h28)=0x%x\n",
		ISP_RD_REG(rawtop, reg_raw_top_t, DEBUG));

	ISP_WR_BITS(isptop, reg_raw_top_t, DEBUG_SELECT, RAW_TOP_DEBUG_SELECT, 4);
	vi_pr(VI_INFO, "RAW_TOP, debug_select(h2c)=0x4, debug(h28)=0x%x\n",
		ISP_RD_REG(rawtop, reg_raw_top_t, DEBUG));
#endif

	//rgbtop
	vi_info->rgbtop.ip_stvalid_status = ISP_RD_REG(rgbtop, reg_isp_rgb_top_t, dbg_ip_s_vld);
	vi_info->rgbtop.ip_stready_status = ISP_RD_REG(rgbtop, reg_isp_rgb_top_t, dbg_ip_s_rdy);
	vi_info->rgbtop.dmi_stvalid_status = ISP_RD_REG(rgbtop, reg_isp_rgb_top_t, dbg_dmi_vld);
	vi_info->rgbtop.dmi_stready_status = ISP_RD_REG(rgbtop, reg_isp_rgb_top_t, dbg_dmi_rdy);
	vi_info->rgbtop.xcnt_rpt = ISP_RD_BITS(rgbtop, reg_isp_rgb_top_t, patgen4, xcnt_rpt);
	vi_info->rgbtop.ycnt_rpt = ISP_RD_BITS(rgbtop, reg_isp_rgb_top_t, patgen4, ycnt_rpt);

	//yuvtop
	vi_info->yuvtop.debug_state = ISP_RD_REG(yuvtop, reg_yuv_top_t, yuv_debug_state);
	vi_info->yuvtop.stvalid_status = ISP_RD_REG(yuvtop, reg_yuv_top_t, stvalid_status);
	vi_info->yuvtop.stready_status = ISP_RD_REG(yuvtop, reg_yuv_top_t, stready_status);
	vi_info->yuvtop.xcnt_rpt = ISP_RD_BITS(yuvtop, reg_yuv_top_t, patgen4, xcnt_rpt);
	vi_info->yuvtop.ycnt_rpt = ISP_RD_BITS(yuvtop, reg_yuv_top_t, patgen4, ycnt_rpt);

	//rdma28
	ISP_WR_BITS(rdma28, reg_isp_dma_ctl_t, sys_control, dbg_sel, 0x1);
	vi_info->rdma28[0].dbg_sel = 0x1;
	vi_info->rdma28[0].status = ISP_RD_REG(rdma28, reg_isp_dma_ctl_t, dma_status);
	ISP_WR_BITS(rdma28, reg_isp_dma_ctl_t, sys_control, dbg_sel, 0x2);
	vi_info->rdma28[1].dbg_sel = 0x2;
	vi_info->rdma28[1].status = ISP_RD_REG(rdma28, reg_isp_dma_ctl_t, dma_status);
	vi_info->enable = true;
}

static void _vi_show_debug_info(void)
{
	struct vi_info *vi_info = NULL;
	u8 i = 0;

	if (!g_overflow_info)
		return;

	vi_info = &g_overflow_info->vi_info;

	if (vi_info->enable) {
		vi_info->enable = false;
		pr_info("ISP_TOP, blk_idle(h38)=0x%x\n", vi_info->isp_top.blk_idle);
		for (i = 0; i <= 6; i++) {
			pr_info("dbus_sel=%d, r_0=0x%x, r_4=0x%x, r_8=0x%x, r_c=0x%x\n",
				i,
				vi_info->isp_top.dbus_sel[i].r_0,
				vi_info->isp_top.dbus_sel[i].r_4,
				vi_info->isp_top.dbus_sel[i].r_8,
				vi_info->isp_top.dbus_sel[i].r_c);
		}
		pr_info("PRE_RAW_FE0, preraw_info(h34)=0x%x, fe_idle_info(h50)=0x%x\n",
			vi_info->preraw_fe.preraw_info,
			vi_info->preraw_fe.fe_idle_info);
		pr_info("PRE_RAW_BE, preraw_be_info(h14)=0x%x, be_dma_idle_info(h18)=0x%x, ip_idle_info(h1c)=0x%x\n",
			vi_info->preraw_be.preraw_be_info,
			vi_info->preraw_be.be_dma_idle_info,
			vi_info->preraw_be.ip_idle_info);
		pr_info("PRE_RAW_BE, stvalid_status(h28)=0x%x, stready_status(h2c)=0x%x\n",
			vi_info->preraw_be.stvalid_status,
			vi_info->preraw_be.stready_status);
		pr_info("RAW_TOP, stvalid_status(h40)=0x%x, stready_status(h44)=0x%x, dma_idle(h60)=0x%x\n",
			vi_info->rawtop.stvalid_status,
			vi_info->rawtop.stready_status,
			vi_info->rawtop.dma_idle);
		pr_info("RGB_TOP, ip_stvalid_status(h50)=0x%x, ip_stready_status(h54)=0x%x\n",
			vi_info->rgbtop.ip_stvalid_status,
			vi_info->rgbtop.ip_stready_status);
		pr_info("RGB_TOP, dmi_stvalid_status(h58)=0x%x, dmi_stready_status(h5c)=0x%x\n",
			vi_info->rgbtop.dmi_stvalid_status,
			vi_info->rgbtop.dmi_stready_status);
		pr_info("RGB_TOP xcnt_rpt=0x%x, ycnt_rpt=0x%x\n",
			vi_info->rgbtop.xcnt_rpt,
			vi_info->rgbtop.ycnt_rpt);
		pr_info("YUV_TOP debug_state(h18)=0x%x, stvalid_status(h6c)=0x%x, stready_status(h70)=0x%x\n",
			vi_info->yuvtop.debug_state,
			vi_info->yuvtop.stvalid_status,
			vi_info->yuvtop.stready_status);
		pr_info("YUV_TOP xcnt_rpt=0x%x, ycnt_rpt=0x%x\n",
			vi_info->yuvtop.xcnt_rpt,
			vi_info->yuvtop.ycnt_rpt);
		pr_info("rdma28, dbg_sel(h000)=0x%x, status(h014)=0x%x\n",
			vi_info->rdma28[0].dbg_sel,
			vi_info->rdma28[0].status);
		pr_info("rdma28, dbg_sel(h000)=0x%x, status(h014)=0x%x\n",
			vi_info->rdma28[1].dbg_sel,
			vi_info->rdma28[1].status);
	}

	kfree(g_overflow_info);
	g_overflow_info = NULL;
}

static void _vi_err_retrig_pre_fe(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num;
	enum sop_isp_fe_chn_num fe_max, fe_chn;
	unsigned long flags;
	s8 ret = ISP_SUCCESS;

	if (!_is_be_post_online(ctx))
		return;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;
		if (atomic_read(&vdev->isp_err_times[raw_num]) > 30) {
			vi_pr(VI_ERR, "raw_%d too much errors happened\n", raw_num);
			continue;
		}

		vi_pr(VI_WARN, "fe_%d trig retry %d times\n", raw_num, atomic_read(&vdev->isp_err_times[raw_num]));

		//yuv sensor offline2sc
		if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor && ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
			fe_max = (enum sop_isp_fe_chn_num)ctx->isp_pipe_cfg[raw_num].mux_mode;
			for (fe_chn = ISP_FE_CH0; fe_chn <= fe_max; fe_chn++) {
				if (atomic_read(&vdev->isp_err_times[raw_num])) {
					spin_lock_irqsave(&vdev->qbuf_lock, flags);
					vdev->qbuf_num[raw_num][fe_chn]++;
					spin_unlock_irqrestore(&vdev->qbuf_lock, flags);
				}

				if (sop_isp_rdy_buf_empty(vdev, raw_num, fe_chn)) {
					vi_pr(VI_INFO, "fe_%d chn_%d yuv bypass outbuf is empty\n", raw_num, fe_chn);
				} else {
					_isp_yuv_bypass_trigger(vdev, raw_num, fe_chn);
				}
			}
		} else { //rgb senosr or yuv sensor online2sc
			fe_max = ctx->isp_pipe_cfg[raw_num].is_hdr_on
				     ? ISP_FE_CH1
				     : ctx->isp_pipe_cfg[raw_num].mux_mode;
			for (fe_chn = ISP_FE_CH0; fe_chn <= fe_max; fe_chn++) {
				ret |= _pre_hw_enque(vdev, raw_num, fe_chn);
			}
		}
	}

	if (ctx->is_tile && (ret == ISP_SUCCESS)) {
		_splt_hw_enque(vdev, ISP_PRERAW0);
}
}

void _vi_err_handler(struct sop_vi_dev *vdev, const enum sop_isp_raw err_raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num;
	enum sop_isp_fe_chn_num fe_chn;
	int count = 10;
	bool fe_idle, be_idle, post_idle;

	//step 1 : set frm vld = 0
	isp_frm_err_handler(ctx, err_raw_num, 1);

	//step 2 : wait to make sure post and the other fe is done.
	while (--count > 0) {
		if (_is_be_post_online(ctx)) {
			fe_idle = be_idle = post_idle = true;

			for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
				if (!vdev->ctx.isp_pipe_enable[raw_num])
					continue;
				if (atomic_read(&vdev->isp_err_times[raw_num]) && count < 5)
					continue;
				for (fe_chn = ISP_FE_CH0; fe_chn < ISP_FE_CHN_MAX; fe_chn++) {
					if (atomic_read(&vdev->pre_fe_state[raw_num][fe_chn]) != ISP_STATE_IDLE) {
						fe_idle = false;
						break;
					}
				}
			}

			if (!(atomic_read(&vdev->pre_be_state[ISP_BE_CH0]) == ISP_STATE_IDLE &&
				atomic_read(&vdev->pre_be_state[ISP_BE_CH1]) == ISP_STATE_IDLE))
				be_idle = false;

			if (!(atomic_read(&vdev->postraw_state) == ISP_STATE_IDLE))
				post_idle = false;

			if (fe_idle && be_idle && post_idle)
				break;

			vi_pr(VI_WARN, "wait fe/be/post idle count(%d) for be_post_online\n", count);
		} else if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) {
			if (atomic_read(&vdev->postraw_state) == ISP_STATE_IDLE)
				break;

			vi_pr(VI_WARN, "wait post idle(%d) count(%d) for fe_be online single\n",
					atomic_read(&vdev->postraw_state), count);
		} else {
			break;
		}

		usleep_range(5 * 1000, 10 * 1000);
	}

	//If fe/be/post not done;
	if (count == 0) {
		vi_pr(VI_ERR, "isp status fe_0(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH2]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW0][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status fe_1(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH2]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW1][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status fe_2(ch0:%d, ch1:%d) fe_3(ch0:%d, ch1:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW2][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW2][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW3][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW3][ISP_FE_CH1]));
		vi_pr(VI_ERR, "isp status fe_4(ch0:%d, ch1:%d) fe_5(ch0:%d, ch1:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW4][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW4][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW5][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW5][ISP_FE_CH1]));
		vi_pr(VI_ERR, "isp status fe_lite0(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH2]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE0][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status fe_lite1(ch0:%d, ch1:%d, ch2:%d, ch3:%d)\n",
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH0]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH1]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH2]),
				atomic_read(&vdev->pre_fe_state[ISP_PRERAW_LITE1][ISP_FE_CH3]));
		vi_pr(VI_ERR, "isp status be(ch0:%d, ch1:%d) postraw(%d)\n",
				atomic_read(&vdev->pre_be_state[ISP_BE_CH0]),
				atomic_read(&vdev->pre_be_state[ISP_BE_CH1]),
				atomic_read(&vdev->postraw_state));
	}

	//step 3 : set csibdg sw abort and wait abort done
	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;

		if (isp_frm_err_handler(ctx, raw_num, 3) < 0)
			return;
	}

	//step 4 : isp sw reset and vip reset pull up
	isp_frm_err_handler(ctx, err_raw_num, 4);

	//send err cb to vpss if vpss online
	if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on &&
		!ctx->isp_pipe_cfg[err_raw_num].is_offline_scaler) { //VPSS online
		struct sc_err_handle_cb err_cb = {0};

		/* VPSS Online error handle */
		err_cb.snr_num = err_raw_num;
		if (_vi_call_cb(E_MODULE_VPSS, VPSS_CB_ONLINE_ERR_HANDLE, &err_cb) != 0) {
			vi_pr(VI_ERR, "VPSS_CB_ONLINE_ERR_HANDLE is failed\n");
		}
	}

	//step 5 : isp sw reset and vip reset pull down
	isp_frm_err_handler(ctx, err_raw_num, 5);

	//step 6 : wait ISP idle
	if (isp_frm_err_handler(ctx, err_raw_num, 6) < 0)
		return;

	//step 7 : reset sw state to idle
	if (_is_be_post_online(ctx)) {
		for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
			atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH0], ISP_STATE_IDLE);
			atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH1], ISP_STATE_IDLE);
			atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH2], ISP_STATE_IDLE);
			atomic_set(&vdev->pre_fe_state[raw_num][ISP_FE_CH3], ISP_STATE_IDLE);
		}
	} else if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) {
		atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
		atomic_set(&vdev->pre_be_state[ISP_BE_CH1], ISP_STATE_IDLE);
	} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) { //slice buffer on
		atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH0], ISP_STATE_IDLE);
		atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH1], ISP_STATE_IDLE);
		atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH2], ISP_STATE_IDLE);
		atomic_set(&vdev->pre_fe_state[err_raw_num][ISP_FE_CH3], ISP_STATE_IDLE);
		atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);
		atomic_set(&vdev->pre_be_state[ISP_BE_CH1], ISP_STATE_IDLE);
	}

	//step 8 : set fbcd dma to hw mode if fbc is on
	if (ctx->is_fbc_on) {
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_Y, false);
		ispblk_dma_set_sw_mode(ctx, ISP_BLK_ID_DMA_CTL_TNR_ST_C, false);
	}

	//step 9 : reset first frame count
	vdev->ctx.isp_pipe_cfg[err_raw_num].first_frm_cnt = 0;

	//step 10 : show overflow info
	_vi_show_debug_info();

	//Let postraw trigger go
	atomic_set(&vdev->isp_err_handle_flag, 0);

	//retrig pre_fe
	_vi_err_retrig_pre_fe(vdev);
}

static int _vi_err_handler_thread(void *arg)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	enum sop_isp_raw err_raw_num;
	enum E_VI_TH th_id = E_VI_TH_ERR_HANDLER;

	while (1) {
		wait_event(vdev->vi_th[th_id].wq, vdev->vi_th[th_id].flag != 0 || kthread_should_stop());

		if (vdev->vi_th[th_id].flag != 0) {
			err_raw_num = vdev->vi_th[th_id].flag - 1;
			vdev->vi_th[th_id].flag = 0;
		}

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		_vi_err_handler(vdev, err_raw_num);
	}

	return 0;
}

static inline void vi_err_wake_up_th(struct sop_vi_dev *vdev, enum sop_isp_raw err_raw)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_fe_chn_num fe_chn = ISP_FE_CH0;
	enum sop_isp_fe_chn_num fe_max = ctx->isp_pipe_cfg[err_raw].is_hdr_on
					? ISP_FE_CH1
					: ctx->isp_pipe_cfg[err_raw].mux_mode;

	//record err_num;
	atomic_add(1, &vdev->isp_err_times[err_raw]);

	for (fe_chn = ISP_FE_CH0; fe_chn <= fe_max; fe_chn++) {
		vi_pr(VI_WARN, "dbg_0=0x%x, dbg_1=0x%x, dbg_2=0x%x, dbg_3=0x%x\n",
		      ctx->isp_pipe_cfg[err_raw].dg_info.bdg_chn_debug[fe_chn].dbg_0,
		      ctx->isp_pipe_cfg[err_raw].dg_info.bdg_chn_debug[fe_chn].dbg_1,
		      ctx->isp_pipe_cfg[err_raw].dg_info.bdg_chn_debug[fe_chn].dbg_2,
		      ctx->isp_pipe_cfg[err_raw].dg_info.bdg_chn_debug[fe_chn].dbg_3);
	}

	//Stop pre/postraw trigger go
	if (atomic_read(&vdev->isp_err_handle_flag) == 1) {
		vi_pr(VI_WARN, "err_handler running\n");
		return;
	}

	atomic_set(&vdev->isp_err_handle_flag, 1);

	vdev->vi_th[E_VI_TH_ERR_HANDLER].flag = err_raw + 1;

	wake_up(&vdev->vi_th[E_VI_TH_ERR_HANDLER].wq);
}

u32 isp_err_chk(
	struct sop_vi_dev *vdev,
	struct isp_ctx *ctx,
	union reg_isp_csi_bdg_interrupt_status_0 *cbdg_0_sts,
	union reg_isp_csi_bdg_interrupt_status_1 *cbdg_1_sts)
{
	u32 ret = 0;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum sop_isp_fe_chn_num fe_chn = ISP_FE_CH0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;

		if (cbdg_1_sts[raw_num].bits.fifo_overflow_int) {
			vi_pr(VI_ERR, "CSIBDG_%d fifo overflow\n", raw_num);
			_vi_record_debug_info(ctx);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_fifo_of_cnt++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (cbdg_1_sts[raw_num].bits.frame_resolution_over_max_int) {
			vi_pr(VI_ERR, "CSIBDG_%d frm size over max\n", raw_num);
			ret = -1;
		}

		if (cbdg_1_sts[raw_num].bits.dma_error_int) {
			u32 wdma_0_err = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_err_sts;
			u32 wdma_1_err = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_err_sts;
			u32 wdma_2_err = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_2_err_sts;
			u32 wdma_3_err = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_3_err_sts;
			u32 rdma_0_err = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_0_err_sts;
			u32 rdma_1_err = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_1_err_sts;
			u32 wdma_0_idle = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_idle;
			u32 wdma_1_idle = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_idle;
			u32 wdma_2_idle = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_2_idle;
			u32 wdma_3_idle = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_3_idle;
			u32 rdma_0_idle = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_0_idle;
			u32 rdma_1_idle = ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_1_idle;

			if ((wdma_0_err & 0x10) || (wdma_1_err & 0x10) ||
			(wdma_2_err & 0x10) || (wdma_3_err & 0x10) ||
			(rdma_0_err & 0x10) || (rdma_1_err & 0x10)) {
				vi_pr(VI_ERR, "DMA axi error\n");
				ret = -1;
			} else if ((wdma_0_err & 0x20) || (wdma_1_err & 0x20) ||
				(wdma_2_err & 0x20) || (wdma_3_err & 0x20) ||
				(rdma_0_err & 0x20) || (rdma_1_err & 0x20)) {
				vi_pr(VI_ERR, "DMA axi mismatch\n");
				ret = -1;
			} else if ((wdma_0_err & 0x40) || (wdma_1_err & 0x40) ||
				(wdma_2_err & 0x40) || (wdma_3_err & 0x40)) {
				vi_pr(VI_WARN, "WDMA buffer full\n");
			}

			vi_pr(VI_WARN, "Err status wdma[0(0x%x) 1(0x%x) 2(0x%x) 3(0x%x)] rdma[0(0x%x) 1(0x%x)]\n",
					wdma_0_err, wdma_1_err, wdma_2_err, wdma_3_err, rdma_0_err, rdma_1_err);
			vi_pr(VI_WARN, "Idle status wdma[0(0x%x) 1(0x%x) 2(0x%x) 3(0x%x)] rdma[0(0x%x) 1(0x%x)]\n",
					wdma_0_idle, wdma_1_idle, wdma_2_idle, wdma_3_idle, rdma_0_idle, rdma_1_idle);
		}

		fe_chn = ISP_FE_CH0;

		if (cbdg_0_sts[raw_num].bits.ch0_frame_width_gt_int) {
			vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width greater than setting(%d)\n",
					raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (cbdg_0_sts[raw_num].bits.ch0_frame_width_ls_int) {
			vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width less than setting(%d)\n",
					raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (cbdg_0_sts[raw_num].bits.ch0_frame_height_gt_int) {
			vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height greater than setting(%d)\n",
					raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (cbdg_0_sts[raw_num].bits.ch0_frame_height_ls_int) {
			vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height less than setting(%d)\n",
					raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[fe_chn]++;
			vi_err_wake_up_th(vdev, raw_num);
			ret = -1;
		}

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on ||
		    ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_1MULTIPLEX) {
			fe_chn = ISP_FE_CH1;

			if (cbdg_0_sts[raw_num].bits.ch1_frame_width_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch1_frame_width_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch1_frame_height_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch1_frame_height_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}
		}

		if (ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_2MULTIPLEX) {
			fe_chn = ISP_FE_CH2;

			if (cbdg_0_sts[raw_num].bits.ch2_frame_width_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch2_frame_width_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch2_frame_height_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch2_frame_height_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}
		}

		if (ctx->isp_pipe_cfg[raw_num].mux_mode > VI_WORK_MODE_3MULTIPLEX) {
			fe_chn = ISP_FE_CH3;

			if (cbdg_0_sts[raw_num].bits.ch3_frame_width_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch3_frame_width_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm width less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_width);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch3_frame_height_gt_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height greater than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}

			if (cbdg_0_sts[raw_num].bits.ch3_frame_height_ls_int) {
				vi_pr(VI_ERR, "CSIBDG_%d CH%d frm height less than setting(%d)\n",
						raw_num, fe_chn, ctx->isp_pipe_cfg[raw_num].csibdg_height);
				ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[fe_chn]++;
				vi_err_wake_up_th(vdev, raw_num);
				ret = -1;
			}
		}
	}

	return ret;
}

void isp_post_tasklet(unsigned long data)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)data;

	_post_hw_enque(vdev);
}

static int _vi_preraw_thread(void *arg)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	struct isp_ctx *ctx = &vdev->ctx;

	struct list_head *pos, *temp;
	struct _isp_raw_num_n  *n[VI_MAX_LIST_NUM];
	unsigned long flags;
	u32 enq_num = 0, i = 0;
	u8 dev_num = 0;
	enum E_VI_TH th_id = E_VI_TH_PRERAW;

	while (1) {
		wait_event(vdev->vi_th[th_id].wq, vdev->vi_th[th_id].flag != 0 || kthread_should_stop());
		vdev->vi_th[th_id].flag = 0;

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		spin_lock_irqsave(&raw_num_lock, flags);
		list_for_each_safe(pos, temp, &pre_raw_num_q.list) {
			n[enq_num] = list_entry(pos, struct _isp_raw_num_n, list);
			enq_num++;
		}
		spin_unlock_irqrestore(&raw_num_lock, flags);

		for (i = 0; i < enq_num; i++) {
			raw_num = n[i]->raw_num;

			spin_lock_irqsave(&raw_num_lock, flags);
			list_del_init(&n[i]->list);
			kfree(n[i]);
			spin_unlock_irqrestore(&raw_num_lock, flags);

			if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
				pre_be_tuning_update(&vdev->ctx, raw_num);
				//Update pre be sts size/addr
				_swap_pre_be_sts_buf(vdev, raw_num, ISP_BE_CH0);

				postraw_tuning_update(&vdev->ctx, raw_num);
				//Update postraw sts awb/dci/hist_edge_v dma size/addr
				_swap_post_sts_buf(ctx, raw_num);
			} else {
				if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe)
					_isp_snr_cfg_deq_and_fire(vdev, raw_num, 0);

				pre_fe_tuning_update(&vdev->ctx, raw_num);

				//fe->be->dram->post or on the fly
				if (_is_fe_be_online(ctx) || _is_all_online(ctx)) {
					pre_be_tuning_update(&vdev->ctx, raw_num);

					//on the fly or slice buffer mode on
					if (_is_all_online(ctx) || ctx->is_slice_buf_on)
						postraw_tuning_update(&vdev->ctx, raw_num);
				}
			}

			if ((ctx->is_multi_sensor) && (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor)) {
				dev_num = vi_get_dev_num_by_raw(ctx, raw_num);
				if ((tuning_dis[0] > 0) && ((tuning_dis[0] - 1) != dev_num)) {
					vi_pr(VI_DBG, "dev_%d start drop\n", dev_num);
					ctx->isp_pipe_cfg[raw_num].is_drop_next_frame = true;
				}
			}

			if (ctx->isp_pipe_cfg[raw_num].is_drop_next_frame) {
				//if !is_drop_next_frame, set is_drop_next_frame flags false;
				if (_is_drop_next_frame(vdev, raw_num, ISP_FE_CH0))
					++vdev->drop_frame_number[raw_num];
				else {
					vi_pr(VI_DBG, "raw_%d stop drop\n", raw_num);
					/*
					ctx->isp_pipe_cfg[raw_num].isp_reset_frm =
						vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] + 1;
					*/
					_clear_drop_frm_info(vdev, raw_num);
				}

				//vi onthefly and vpss online will trigger preraw in post_hw_enque
				if (_is_all_online(ctx) && !ctx->isp_pipe_cfg[raw_num].is_offline_scaler)
					continue;

				//TODO raw_num maybe not RAW_)
				if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
				    !ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
					isp_trig_whole_preraw(vdev, raw_num);
				}

				if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_SPLT) {
					enum isp_blk_id_t blk_id = (raw_num == ISP_PRERAW0)
									? ISP_BLK_ID_SPLT_FE0_WDMA
									: ISP_BLK_ID_SPLT_FE1_WDMA;

					ispblk_splt_wdma_ctrl_config(ctx, blk_id, true);
					_splt_hw_enque(vdev, raw_num);
				}
			}
		}

		enq_num = 0;
	}

	return 0;
}

static int _vi_vblank_handler_thread(void *arg)
{
	struct sop_vi_dev *vdev = (struct sop_vi_dev *)arg;
	enum sop_isp_raw raw_num = ISP_PRERAW0;

	enum E_VI_TH th_id = E_VI_TH_VBLANK_HANDLER;

	while (1) {
		wait_event(vdev->vi_th[th_id].wq, vdev->vi_th[th_id].flag != 0 || kthread_should_stop());
		raw_num = vdev->vi_th[th_id].flag - 1;
		vdev->vi_th[th_id].flag = 0;

		if (kthread_should_stop()) {
			pr_info("%s exit\n", vdev->vi_th[th_id].th_name);
			atomic_set(&vdev->vi_th[th_id].thread_exit, 1);
			do_exit(1);
		}

		_isp_snr_cfg_deq_and_fire(vdev, raw_num, 1);
	}

	return 0;
}

static inline void _vi_wake_up_vblank_th(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	vdev->vi_th[E_VI_TH_VBLANK_HANDLER].flag = raw_num + 1;
	wake_up(&vdev->vi_th[E_VI_TH_VBLANK_HANDLER].wq);
}

static void _isp_yuv_online_handler(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u8 hw_chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;

	// No changed in onthefly mode or slice buffer on
	if (ctx->is_3dnr_on) {
		if (!_is_all_online(ctx) && !(_is_fe_be_online(ctx) && ctx->is_rgbmap_sbm_on)) {
			ispblk_tnr_rgbmap_chg(ctx, raw_num, hw_chn_num);
			_pre_fe_rgbmap_update(vdev, raw_num, hw_chn_num);
		}
	}

	if (_is_fe_be_online(ctx) || _is_all_online(ctx)) { //fe->be->dram->post or on the fly mode
		atomic_set(&vdev->pre_fe_state[raw_num][hw_chn_num], ISP_STATE_IDLE);

		if (_is_all_online(ctx)) {
			struct isp_grid_s_info m_info;

			m_info = ispblk_rgbmap_info(ctx, raw_num);
			ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit = m_info.w_bit;
			ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit = m_info.h_bit;

			m_info = ispblk_lmap_info(ctx, raw_num);
			ctx->isp_pipe_cfg[raw_num].lmap_i.w_bit = m_info.w_bit;
			ctx->isp_pipe_cfg[raw_num].lmap_i.h_bit = m_info.h_bit;
		}
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		struct isp_buffer *b = NULL;
		struct isp_grid_s_info m_info;
		struct isp_queue *fe_out_q, *be_in_q;

		fe_out_q = &pre_fe_out_q[raw_num][hw_chn_num];
		be_in_q = &pre_be_in_q;

		b = isp_buf_remove(fe_out_q);
		if (b == NULL) {
			vi_pr(VI_DBG, "YUV_ISP Pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, hw_chn_num);
			return;
		}

		b->crop_le.x = 0;
		b->crop_le.y = 0;
		b->crop_le.w = ctx->isp_pipe_cfg[raw_num].post_img_w;
		b->crop_le.h = ctx->isp_pipe_cfg[raw_num].post_img_h;
		b->raw_num = raw_num;
		b->chn_num = 0;

		m_info = ispblk_rgbmap_info(ctx, raw_num);
		b->rgbmap_i.w_bit = m_info.w_bit;
		b->rgbmap_i.h_bit = m_info.h_bit;

		m_info = ispblk_lmap_info(ctx, raw_num);
		b->lmap_i.w_bit = m_info.w_bit;
		b->lmap_i.h_bit = m_info.h_bit;

		isp_buf_queue(be_in_q, b);

		atomic_set(&vdev->pre_fe_state[raw_num][hw_chn_num], ISP_STATE_IDLE);

		vi_pr(VI_DBG, "YUV fe->dram->yuv raw_num=%d frame number=%d out buf_addr=0x%llx\n",
			  raw_num, vdev->pre_fe_frm_num[raw_num][hw_chn_num], b->addr);

		tasklet_hi_schedule(&vdev->job_work);
		_vi_wake_up_vblank_th(vdev, raw_num);

		vi_event_queue(vdev, VI_EVENT_PRE0_EOF + vi_get_dev_num_by_raw(ctx, raw_num),
				vdev->pre_fe_frm_num[raw_num][hw_chn_num]);

		if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
			!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be &&
			ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap != RAW_AI_ISP_SPLT) {
			_pre_hw_enque(vdev, raw_num, hw_chn_num);
		}
	}
}

static void _isp_yuv_bypass_handler(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num, const u8 hw_chn_num)
{
	u8 buf_chn = vdev->ctx.raw_chnstr_num[raw_num] + hw_chn_num;

	atomic_set(&vdev->pre_fe_state[raw_num][hw_chn_num], ISP_STATE_IDLE);

	sop_isp_rdy_buf_remove(vdev, raw_num, hw_chn_num);

	sop_isp_dqbuf_list(vdev, vdev->pre_fe_frm_num[raw_num][hw_chn_num],
			   raw_num, buf_chn, ktime_to_timespec64(ktime_get()));

	vdev->vi_th[E_VI_TH_EVENT_HANDLER].flag = raw_num + 1;

	wake_up(&vdev->vi_th[E_VI_TH_EVENT_HANDLER].wq);

	if (sop_isp_rdy_buf_empty(vdev, raw_num, hw_chn_num))
		vi_pr(VI_INFO, "fe_%d chn_num_%d yuv bypass outbuf is empty\n", raw_num, hw_chn_num);
	else
		_isp_yuv_bypass_trigger(vdev, raw_num, hw_chn_num);
}

static void _isp_sof_handler(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct _isp_dqbuf_n *n = NULL;
	unsigned long flags;

	if (atomic_read(&vdev->is_suspend) == 1) {
		return;
	}

	if (atomic_read(&vdev->isp_streamoff) == 1)
		return;

	if (_is_right_tile(ctx, raw_num))
		return;

	if (!(_is_fe_be_online(ctx) && ctx->is_slice_buf_on) || ctx->isp_pipe_cfg[raw_num].is_drop_next_frame)
		_vi_wake_up_preraw_th(vdev, raw_num);

	if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 2) //raw_dump flow
		atomic_set(&vdev->isp_raw_dump_en[raw_num], 3);

	tasklet_hi_schedule(&vdev->job_work);

	if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
		spin_lock_irqsave(&dq_lock, flags);
		if (!list_empty(&dqbuf_q.list)) {
			n = list_first_entry(&dqbuf_q.list, struct _isp_dqbuf_n, list);
			vdev->vi_th[E_VI_TH_EVENT_HANDLER].flag = n->chn_id + 1;
			wake_up(&vdev->vi_th[E_VI_TH_EVENT_HANDLER].wq);
		}
		spin_unlock_irqrestore(&dq_lock, flags);
	}

	isp_sync_task_process(raw_num);

	vi_event_queue(vdev, VI_EVENT_PRE0_SOF + vi_get_dev_num_by_raw(&vdev->ctx, raw_num),
						vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);
}

static inline void _isp_splt_wdma_done_handler(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw hw_raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = hw_raw_num;
	bool trigger = false;

	/*only splt_ai_isp need splt_wdma*/
	if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap != RAW_AI_ISP_SPLT)
		return;

	vi_pr(VI_DBG, "splt_wdma_%d frm_done chn_num=%d frm_num=%d\n",
			raw_num, chn_num, vdev->splt_wdma_frm_num[raw_num][chn_num]);

	atomic_set(&vdev->splt_state[raw_num][chn_num], ISP_STATE_IDLE);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		trigger = vdev->splt_wdma_frm_num[raw_num][ISP_FE_CH0] ==
				vdev->splt_wdma_frm_num[raw_num][ISP_FE_CH1];
	} else {
		trigger = true;
	}

	if (trigger) {
		if (ctx->isp_pipe_cfg[raw_num].is_fake_splt_wdma) {
			// fake_splt_wdma will not run tpu
			_pre_hw_enque(vdev, raw_num, ISP_FE_CH0);
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
				_pre_hw_enque(vdev, raw_num, ISP_FE_CH1);
		} else {
			if (ddr_need_retrain(vdev)) {
				_splt_hw_enque(vdev, raw_num);
			} else {
				// go to splt_ai_isp process
				_vi_wake_up_tpu_th(vdev, raw_num, AI_ISP_TYPE_BNR);
				_vi_wake_up_vblank_th(vdev, raw_num);
			}
		}
	}
}

static inline void _isp_splt_rdma_done_handler(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw hw_raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_queue *splt_w_q = NULL;
	struct isp_queue *splt_r_q = NULL;
	struct isp_buffer *b = NULL;
	enum isp_blk_id_t splt_rdma_id = 0;
	enum sop_isp_raw raw_num = hw_raw_num;
	bool trigger = false;

	vi_pr(VI_DBG, "splt_rdma_%d frm_done chn_num=%d frm_num=%d\n",
			raw_num, chn_num, vdev->splt_rdma_frm_num[raw_num][chn_num]);

	if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		raw_num = ISP_PRERAW0;
		trigger = (vdev->splt_rdma_frm_num[raw_num][chn_num] ==
				vdev->splt_rdma_frm_num[raw_num + 1][chn_num]);
	} else
		trigger = true;

	if (trigger) {
		splt_w_q = &splt_out_q[raw_num][chn_num];
		splt_r_q = &pre_fe_in_q[raw_num][chn_num];

		b = isp_buf_remove(splt_r_q);
		if (b == NULL) {
			vi_pr(VI_DBG, "splt_rdma_%d chn_num_%d outbuf is empty\n", raw_num, chn_num);
			return;
		}
		isp_buf_queue(splt_w_q, b);

		splt_rdma_id = (chn_num == ISP_FE_CH0) ?
				ISP_BLK_ID_SPLT_FE0_RDMA_LE : ISP_BLK_ID_SPLT_FE0_RDMA_SE;

		//ispblk_splt_rdma_ctrl_config(ctx, splt_rdma_id, false);
		//if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		//	splt_rdma_id = (chn_num == ISP_FE_CH0) ?
		//			ISP_BLK_ID_SPLT_FE1_RDMA_LE : ISP_BLK_ID_SPLT_FE1_RDMA_SE;
		//	ispblk_splt_rdma_ctrl_config(ctx, splt_rdma_id, false);
		//}
	}
}

static inline void _isp_pre_fe_done_handler(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw hw_raw_num,
	const enum sop_isp_fe_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = hw_raw_num;
	bool trigger = false;
	s8 ret = ISP_SUCCESS;

	//reset error times when fe_done
	if (unlikely(atomic_read(&vdev->isp_err_times[raw_num]))) {
		atomic_set(&vdev->isp_err_times[raw_num], 0);
	}

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
		if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) {
			vi_pr(VI_DBG, "pre_fe_%d yuv bypass done chn_num=%d frm_num=%d\n",
					raw_num, chn_num, vdev->pre_fe_frm_num[raw_num][chn_num]);
			_isp_yuv_bypass_handler(vdev, raw_num, chn_num);
		} else if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ONLINE) {
			vi_pr(VI_DBG, "pre_fe_%d yuv online done chn_num=%d frm_num=%d\n",
					raw_num, chn_num, vdev->pre_fe_frm_num[raw_num][chn_num]);
			_isp_yuv_online_handler(vdev, raw_num, chn_num);
		} else if (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			vi_pr(VI_DBG, "pre_fe_%d yuv isp done chn_num=%d frm_num=%d\n",
					raw_num, chn_num, vdev->pre_fe_frm_num[raw_num][chn_num]);
			_isp_yuv_online_handler(vdev, raw_num, chn_num);
		} else {
			vi_pr(VI_ERR, "pre_fe_%d isp_domain error, yuv_scene_mode=%d\n",
					raw_num, ctx->isp_pipe_cfg[raw_num].yuv_scene_mode);
		}
		return;
	}

	vi_pr(VI_DBG, "pre_fe_%d frm_done chn_num=%d frm_num=%d\n",
			raw_num, chn_num, vdev->pre_fe_frm_num[raw_num][chn_num]);

	if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		//it's means fe0/fe1 chn frame done
		trigger = (vdev->pre_fe_frm_num[ISP_PRERAW0][chn_num] ==
					vdev->pre_fe_frm_num[ISP_PRERAW1][chn_num]);

		if (!trigger) {
			atomic_set(&vdev->pre_fe_state[hw_raw_num][chn_num], ISP_STATE_IDLE);
			vi_pr(VI_DBG, "tile mode wait fe0/fe1 frame done\n");
			return;
		}

		//for tile mode, it's only can opreate fe0
		raw_num = ISP_PRERAW0;
	}

	// No changed in onthefly mode or slice buffer on
	if (!_is_all_online(ctx) && !(_is_fe_be_online(ctx) && ctx->is_rgbmap_sbm_on)) {
		ispblk_tnr_rgbmap_chg(ctx, raw_num, chn_num);
		_pre_fe_rgbmap_update(vdev, raw_num, chn_num);
	}

	if (_is_fe_be_online(ctx) || _is_all_online(ctx)) { //fe->be->dram->post or on the fly mode
		if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 3) { //raw_dump flow
			struct isp_buffer *b = NULL;
			struct isp_queue *fe_out_q = (chn_num == ISP_FE_CH0) ?
							&raw_dump_b_q[raw_num] : &raw_dump_b_se_q[raw_num];
			struct isp_queue *raw_d_q = (chn_num == ISP_FE_CH0) ?
							&raw_dump_b_dq[raw_num] : &raw_dump_b_se_dq[raw_num];
			u32 x, y, w, h, dmaid;

			if (ctx->isp_pipe_cfg[raw_num].rawdump_crop.w &&
				ctx->isp_pipe_cfg[raw_num].rawdump_crop.h) {
				x = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].rawdump_crop.x :
					ctx->isp_pipe_cfg[raw_num].rawdump_crop_se.x;
				y = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].rawdump_crop.y :
					ctx->isp_pipe_cfg[raw_num].rawdump_crop_se.y;
				w = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].rawdump_crop.w :
					ctx->isp_pipe_cfg[raw_num].rawdump_crop_se.w;
				h = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].rawdump_crop.h :
					ctx->isp_pipe_cfg[raw_num].rawdump_crop_se.h;
			} else {
				x = 0;
				y = 0;
				w = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].crop.w :
					ctx->isp_pipe_cfg[raw_num].crop_se.w;
				h = (chn_num == ISP_FE_CH0) ?
					ctx->isp_pipe_cfg[raw_num].crop.h :
					ctx->isp_pipe_cfg[raw_num].crop_se.h;
			}

			dmaid = csibdg_dma_find_hwid(raw_num, chn_num);

			if (chn_num == ISP_FE_CH0)
				++vdev->dump_frame_number[raw_num];

			ispblk_csidbg_dma_wr_en(ctx, raw_num, chn_num, 0);

			b = isp_buf_remove(fe_out_q);
			if (b == NULL) {
				vi_pr(VI_ERR, "Pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, chn_num);
				return;
			}

			b->crop_le.x = b->crop_se.x = x;
			b->crop_le.y = b->crop_se.y = y;
			b->crop_le.w = b->crop_se.w = w;
			b->crop_le.h = b->crop_se.h = h;
			b->byr_size = ispblk_dma_get_size(ctx, dmaid, w, h);
			b->frm_num = vdev->pre_fe_frm_num[raw_num][chn_num];

			isp_buf_queue(raw_d_q, b);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				trigger = (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] ==
						vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]);
			} else
				trigger = true;

			if (trigger)
				_isp_raw_dump_chk(vdev, raw_num, b->frm_num);
		}

		atomic_set(&vdev->pre_fe_state[raw_num][chn_num], ISP_STATE_IDLE);

		if (_is_all_online(ctx)) {
			struct isp_grid_s_info m_info;

			m_info = ispblk_rgbmap_info(ctx, raw_num);
			ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit = m_info.w_bit;
			ctx->isp_pipe_cfg[raw_num].rgbmap_i.h_bit = m_info.h_bit;

			m_info = ispblk_lmap_info(ctx, raw_num);
			ctx->isp_pipe_cfg[raw_num].lmap_i.w_bit = m_info.w_bit;
			ctx->isp_pipe_cfg[raw_num].lmap_i.h_bit = m_info.h_bit;
		}
	} else if (_is_be_post_online(ctx)) { //fe->dram->be->post
		struct isp_buffer *b = NULL, *b_dup = NULL;
		struct isp_grid_s_info m_info;
		struct isp_queue *fe_out_q, *be_in_q, *raw_d_q;
		bool fe_ai_isp = ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap == RAW_AI_ISP_FE ? 1 : 0;
		vb_blk blk = 0;
		struct vb_s *vb = NULL;

		fe_out_q = &pre_fe_out_q[raw_num][chn_num];
		if (fe_ai_isp)
			be_in_q = &bnr_ai_isp_q[raw_num][chn_num];
		else
			be_in_q = (chn_num == ISP_FE_CH0) ? &pre_be_in_q : &pre_be_in_se_q[raw_num];

		raw_d_q = (chn_num == ISP_FE_CH0) ?
				&raw_dump_b_dq[raw_num] : &raw_dump_b_se_dq[raw_num];

		if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 3) //raw dump enable
			fe_out_q = (chn_num == ISP_FE_CH0) ? &raw_dump_b_q[raw_num] : &raw_dump_b_se_q[raw_num];

		b = isp_buf_remove(fe_out_q);
		if (b == NULL) {
			vi_pr(VI_ERR, "Pre_fe_%d chn_num_%d outbuf is empty\n", raw_num, chn_num);
			return;
		}

		m_info = ispblk_rgbmap_info(ctx, raw_num);
		b->rgbmap_i.w_bit = m_info.w_bit;
		b->rgbmap_i.h_bit = m_info.h_bit;

		m_info = ispblk_lmap_info(ctx, raw_num);
		b->lmap_i.w_bit = m_info.w_bit;
		b->lmap_i.h_bit = m_info.h_bit;

		b->chn_num = chn_num;
		b->timestamp = ktime_to_timespec64(ktime_get());

		if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 3) { //raw dump enable
			u32 w = ctx->isp_pipe_cfg[raw_num].crop.w;
			u32 h = ctx->isp_pipe_cfg[raw_num].crop.h;
			u32 dmaid = csibdg_dma_find_hwid(raw_num, chn_num);

			if (chn_num == ISP_FE_CH0)
				++vdev->dump_frame_number[raw_num];

			if (ctx->isp_pipe_cfg[raw_num].is_tile)
				w += ctx->isp_pipe_cfg[raw_num + 1].crop.w;

			b->crop_le.x = b->crop_se.x = 0;
			b->crop_le.y = b->crop_se.y = 0;
			b->crop_le.w = b->crop_se.w = w;
			b->crop_le.h = b->crop_se.h = h;
			b->byr_size = ispblk_dma_get_size(ctx, dmaid, w, h);
			b->frm_num = vdev->pre_fe_frm_num[raw_num][chn_num];

			b_dup = kzalloc(sizeof(struct isp_buffer), GFP_ATOMIC);

			if (b_dup) {
				*b_dup = *b;

				b_dup->is_ext = EXTERNAL_BUFFER;
				blk = vb_phys_addr2handle(b_dup->addr);
				vb = (struct vb_s *)blk;
				atomic_fetch_add(1, &vb->usr_cnt);
				atomic_long_set(&((struct vb_s *)blk)->mod_ids, BIT(ID_ISP));

				isp_buf_queue(be_in_q, b_dup);
			} else {
				vi_pr(VI_WARN, "fail to malloc isp_buffer\n");
			}

			isp_buf_queue(raw_d_q, b);
		} else {
			isp_buf_queue(be_in_q, b);
		}

		atomic_set(&vdev->pre_fe_state[hw_raw_num][chn_num], ISP_STATE_IDLE);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			trigger = (vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0] ==
					vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]);
		} else
			trigger = true;

		if (trigger) {
			vi_pr(VI_DBG, "fe->dram->be->post trigger raw_num=%d\n", raw_num);

			if (atomic_read(&vdev->isp_raw_dump_en[raw_num]) == 3) { //raw dump flow
				_isp_raw_dump_chk(vdev, raw_num, b->frm_num);
				if (fe_ai_isp)
					_pre_hw_enque(vdev, raw_num, chn_num);
			} else {
				if (fe_ai_isp)
					_vi_wake_up_tpu_th(vdev, raw_num, AI_ISP_TYPE_BNR);
				else
					tasklet_hi_schedule(&vdev->job_work);

				if (ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap != RAW_AI_ISP_SPLT)
					_vi_wake_up_vblank_th(vdev, raw_num);
			}
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
			!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be &&
			!ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap) {
			ret = _pre_hw_enque(vdev, raw_num, chn_num);
			if (ctx->isp_pipe_cfg[raw_num].is_tile || line_spliter_en) {
				ret |= _pre_hw_enque(vdev, raw_num + 1, chn_num);

				if (ddr_need_retrain(vdev) || ret == ISP_SUCCESS)
					_splt_hw_enque(vdev, raw_num);
			}
		}
	}
}

static inline void _isp_pre_be_done_handler(struct sop_vi_dev *vdev,
					    const enum sop_isp_be_chn_num chn_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = vi_get_first_raw_num(ctx);
	bool trigger = false;

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { // fe->be->dram->post
		struct isp_buffer *b = NULL;
		struct isp_grid_s_info m_info;
		struct isp_queue *be_out_q = &pre_be_out_q[chn_num];
		struct isp_queue *post_in_q = &postraw_in_q[chn_num];

		++vdev->pre_be_frm_num[raw_num][chn_num];

		vi_pr(VI_DBG, "pre_be_%d frm_done chn_num=%d frm_num=%d\n",
				raw_num, chn_num, vdev->pre_be_frm_num[raw_num][chn_num]);

		b = isp_buf_remove(be_out_q);
		if (b == NULL) {
			vi_pr(VI_ERR, "Pre_be chn_num_%d outbuf is empty\n", chn_num);
			return;
		}

		m_info = ispblk_rgbmap_info(ctx, raw_num);
		b->rgbmap_i.w_bit = m_info.w_bit;
		b->rgbmap_i.h_bit = m_info.h_bit;

		m_info = ispblk_lmap_info(ctx, raw_num);
		b->lmap_i.w_bit = m_info.w_bit;
		b->lmap_i.h_bit = m_info.h_bit;
		b->timestamp = ktime_to_timespec64(ktime_get());

		isp_buf_queue(post_in_q, b);

		//Pre_be done for tuning to get stt.
		_swap_pre_be_sts_buf(vdev, raw_num, chn_num);

		atomic_set(&vdev->pre_be_state[chn_num], ISP_STATE_IDLE);

		if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
			!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be)
			_pre_hw_enque(vdev, raw_num, (enum sop_isp_fe_chn_num)chn_num);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			trigger = (vdev->pre_be_frm_num[raw_num][ISP_BE_CH0] ==
					vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);
		} else
			trigger = true;

		if (trigger) {
			vi_event_queue(vdev, VI_EVENT_PRE0_EOF + vi_get_dev_num_by_raw(ctx, raw_num),
					vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);

			tasklet_hi_schedule(&vdev->job_work);
			_vi_wake_up_vblank_th(vdev, raw_num);
		}
	} else if (_is_be_post_online(ctx)) { // fe->dram->be->post
		struct isp_buffer *b = NULL;
		struct isp_queue *be_in_q = (chn_num == ISP_BE_CH0) ?
						&pre_be_in_q : &pre_be_in_se_q[ctx->cam_id];
		struct isp_queue *pre_out_q = NULL;

		if (ctx->isp_pipe_cfg[ctx->cam_id].is_tile) {
			if (vdev->postraw_proc_num % 2 != 0) {
				vi_pr(VI_DBG, "pre_be_%d chn_num=%d left tile frm_done\n",
					ctx->cam_id, chn_num);
				atomic_set(&vdev->pre_be_state[chn_num], ISP_STATE_IDLE);
				return;
			}
		}

		if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
			b = isp_buf_remove(be_in_q);
			if (b == NULL) {
				vi_pr(VI_ERR, "Pre_be chn_num_%d input buf is empty\n", chn_num);
				return;
			}
			if (b->raw_num >= ISP_PRERAW_MAX) {
				vi_pr(VI_ERR, "buf raw_num_%d is wrong\n", b->raw_num);
				return;
			}
			raw_num = b->raw_num;
		}

		++vdev->pre_be_frm_num[raw_num][chn_num];

		vi_pr(VI_DBG, "pre_be_%d frm_done chn_num=%d frm_num=%d\n",
				raw_num, chn_num, vdev->pre_be_frm_num[raw_num][chn_num]);

		if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
			if (unlikely(b->is_ext == EXTERNAL_BUFFER)) {
				isp_raw_dump_vb_queue(b);
			} else {
				pre_out_q = &pre_fe_out_q[raw_num][chn_num];
				isp_buf_queue(pre_out_q, b);
			}
		}

		atomic_set(&vdev->pre_be_state[chn_num], ISP_STATE_IDLE);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			trigger = (vdev->pre_be_frm_num[raw_num][ISP_BE_CH0] ==
					vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);
		} else
			trigger = true;

		if (trigger)
			vi_event_queue(vdev, VI_EVENT_PRE0_EOF + vi_get_dev_num_by_raw(ctx, raw_num),
					vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);
	} else if (_is_all_online(ctx)) { // fly-mode
		++vdev->pre_be_frm_num[raw_num][chn_num];

		vi_pr(VI_DBG, "pre_be_%d frm_done chn_num=%d frm_num=%d\n",
				raw_num, chn_num, vdev->pre_be_frm_num[raw_num][chn_num]);

		//Pre_be done for tuning to get stt.
		_swap_pre_be_sts_buf(vdev, raw_num, chn_num);

		vi_event_queue(vdev, VI_EVENT_PRE0_EOF + vi_get_dev_num_by_raw(ctx, raw_num),
				vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);
	} else if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on) { // fe->be->dram->post
		++vdev->pre_be_frm_num[raw_num][chn_num];

		vi_pr(VI_DBG, "pre_be_%d frm_done chn_num=%d frm_num=%d\n",
				raw_num, chn_num, vdev->pre_be_frm_num[raw_num][chn_num]);

		//Pre_be done for tuning to get stt.
		_swap_pre_be_sts_buf(vdev, raw_num, chn_num);

		atomic_set(&vdev->pre_be_state[chn_num], ISP_STATE_IDLE);

		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
			trigger = (vdev->pre_be_frm_num[raw_num][ISP_BE_CH0] ==
					vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);
		} else
			trigger = true;

		if (trigger) {
			tasklet_hi_schedule(&vdev->job_work);
			_vi_wake_up_vblank_th(vdev, raw_num);

			vi_event_queue(vdev, VI_EVENT_PRE0_EOF + vi_get_dev_num_by_raw(ctx, raw_num),
					vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);
		}
	}
}

static void _isp_postraw_shaw_done_handler(struct sop_vi_dev *vdev)
{
	if (_is_fe_be_online(&vdev->ctx) && vdev->ctx.is_slice_buf_on) {
		vi_pr(VI_INFO, "postraw shaw done\n");
		_vi_wake_up_preraw_th(vdev, vi_get_first_raw_num(&vdev->ctx));
	}
}

static void _isp_postraw_done_handler(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	enum sop_isp_fe_chn_num chn_num = ISP_FE_CH0;

	if (vdev->postraw_proc_num % 2 == 0) {
		vdev->postraw_proc_num = 0;
		ctx->is_work_on_r_tile = false;
	} else {
		//TODO for tile, maybe enable fbc
		atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
		if (_is_be_post_online(ctx) && ctx->isp_pipe_cfg[raw_num].is_yuv_sensor)
			atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);

		vi_pr(VI_DBG, "Postraw_%d left tile frm_done\n", raw_num);

		if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler)
			tasklet_hi_schedule(&vdev->job_work);
		return;
	}

	if (_isp_clk_dynamic_en(vdev, false) < 0)
		return;

	if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { //fe->be->dram->post
		struct isp_buffer *b;

		b = isp_buf_remove(&postraw_in_q[ISP_RAW_PATH_LE]);
		if (b == NULL) {
			vi_pr(VI_ERR, "post_in_q is empty\n");
			return;
		}
		if (b->raw_num >= ISP_PRERAW_MAX) {
			vi_pr(VI_ERR, "buf raw_num_%d is wrong\n", b->raw_num);
			return;
		}
		raw_num = b->raw_num;
		chn_num = b->chn_num;

		if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
			isp_buf_queue(&pre_fe_out_q[raw_num][chn_num], b);
		} else {
			isp_buf_queue(&pre_be_out_q[ISP_RAW_PATH_LE], b);

			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				b = isp_buf_remove(&postraw_in_q[ISP_RAW_PATH_SE]);
				if (b == NULL) {
					vi_pr(VI_ERR, "post_in_se_q is empty\n");
					return;
				}
				isp_buf_queue(&pre_be_out_q[ISP_RAW_PATH_SE], b);
			}
		}
	} else if (_is_be_post_online(ctx)) { // fe->dram->be->post
		raw_num = ctx->cam_id;

		if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
			struct isp_buffer *b = NULL;
			struct isp_queue *be_in_q = &pre_be_in_q;
			struct isp_queue *pre_out_q = NULL;

			b = isp_buf_remove(be_in_q);
			if (b == NULL) {
				vi_pr(VI_ERR, "pre_be_in_q is empty\n");
				return;
			}
			if (b->chn_num >= ISP_FE_CHN_MAX) {
				vi_pr(VI_ERR, "buf chn_num_%d is wrong\n", b->chn_num);
				return;
			}
			chn_num = b->chn_num;

			pre_out_q = &pre_fe_out_q[raw_num][chn_num];
			isp_buf_queue(pre_out_q, b);
		}
	} else if (_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)) {
		raw_num = vi_get_first_raw_num(ctx);

		//Update postraw stt gms/ae/hist_edge_v dma size/addr
		_swap_post_sts_buf(ctx, raw_num);

		//Change post done flag to be true
		if (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)
			atomic_set(&vdev->ctx.is_post_done, 1);
	}

	atomic_set(&vdev->postraw_state, ISP_STATE_IDLE);
	if (_is_be_post_online(ctx) && ctx->isp_pipe_cfg[raw_num].is_yuv_sensor)
		atomic_set(&vdev->pre_be_state[ISP_BE_CH0], ISP_STATE_IDLE);

	++ctx->isp_pipe_cfg[raw_num].first_frm_cnt;
	++vdev->postraw_frame_number[raw_num];

	vi_pr(VI_DBG, "Postraw_%d frm_done frm_num=%d\n", raw_num, vdev->postraw_frame_number[raw_num]);

	if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor ||
		ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) { //ISP team no need yuv post done
		if (isp_bufpool[raw_num].fswdr_rpt)
			ispblk_fswdr_update_rpt(ctx, isp_bufpool[raw_num].fswdr_rpt);

		ctx->mmap_grid_size[raw_num] = ctx->isp_pipe_cfg[raw_num].rgbmap_i.w_bit;

		vi_event_queue(vdev, VI_EVENT_POST0_EOF + vi_get_dev_num_by_raw(ctx, raw_num),
				vdev->postraw_frame_number[raw_num]);
	}

	if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
		sop_isp_rdy_buf_remove(vdev, raw_num, chn_num);
		sop_isp_dqbuf_list(vdev, vdev->postraw_frame_number[raw_num],
				   raw_num, ctx->raw_chnstr_num[raw_num] + chn_num,
				   ctx->isp_pipe_cfg[raw_num].ts);

		vdev->vi_th[E_VI_TH_EVENT_HANDLER].flag = raw_num + 1;
		wake_up(&vdev->vi_th[E_VI_TH_EVENT_HANDLER].wq);
	}

	if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
	    !ctx->isp_pipe_cfg[raw_num].is_raw_replay_be &&
	     ctx->isp_pipe_cfg[raw_num].raw_ai_isp_ap != RAW_AI_ISP_SPLT) {
		tasklet_hi_schedule(&vdev->job_work);

		//TODO need refactor
		if (!_is_all_online(ctx) && !ctx->is_slice_buf_on) {
			isp_trig_whole_preraw(vdev, raw_num);
		}
	}
}

static void _isp_cmdq_done_chk(struct sop_vi_dev *vdev, const u32 cmdq_intr)
{
	uintptr_t cmdq = vdev->ctx.phys_regs[ISP_BLK_ID_CMDQ];

	if (cmdq_intr & BIT(0)) {
		u8 cmdq_intr_stat = cmdq_intr_status(cmdq);

		vi_pr(VI_DBG, "cmdq_intr 0x%08x\n", cmdq_intr_stat);
		cmdq_intr_clr(cmdq, cmdq_intr_stat);
	}
}

static void _isp_csibdg_lite_frame_start_chk(struct sop_vi_dev *vdev,
					     const enum sop_isp_raw raw_num,
					     const u32 frame_start)
{
	if (frame_start & BIT(1)) {
		vi_record_sof_perf(vdev, raw_num, ISP_FE_CH0);

		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];

		vi_pr(VI_INFO, "csibgd_lite_%d sof chn_num=%d frm_num=%d\n",
			raw_num - ISP_PRERAW_LITE0, ISP_FE_CH0, vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);
	}

	if (frame_start & BIT(9))
		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH1];

	if (frame_start & BIT(17))
		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH2];

	if (frame_start & BIT(25))
		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH3];
}

static void _isp_csibdg_lite_frame_done_chk(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u32 frame_start)
{
	if (frame_start & BIT(3)) {
		vi_record_fe_perf(vdev, raw_num, ISP_FE_CH0);

		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH0);
	}

	if (frame_start & BIT(11)) {
		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH1);
	}

	if (frame_start & BIT(19)) {
		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH2];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH2);
	}

	if (frame_start & BIT(27)) {
		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH3];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH3);
	}
}

static void _isp_splt_frame_done_chk(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u8 frame_done)
{
	// wdma done
	if (frame_done & BIT(0)) {
		++vdev->splt_wdma_frm_num[raw_num][ISP_FE_CH0];
		_isp_splt_wdma_done_handler(vdev, raw_num, ISP_FE_CH0);
	}

	if (frame_done & BIT(1)) {
		++vdev->splt_wdma_frm_num[raw_num][ISP_FE_CH1];
		_isp_splt_wdma_done_handler(vdev, raw_num, ISP_FE_CH1);
	}

	// rdma done
	if (frame_done & BIT(2)) {
		++vdev->splt_rdma_frm_num[raw_num][ISP_FE_CH0];
		_isp_splt_rdma_done_handler(vdev, raw_num, ISP_FE_CH0);
	}

	if (frame_done & BIT(3)) {
		++vdev->splt_rdma_frm_num[raw_num][ISP_FE_CH1];
		_isp_splt_rdma_done_handler(vdev, raw_num, ISP_FE_CH1);
	}

}

static void _isp_pre_fe_frame_start_chk(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u8 frame_start)
{
	if (frame_start & BIT(0)) {
		vi_record_sof_perf(vdev, raw_num, ISP_FE_CH0);

		if (!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be)
			++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];

		vi_pr(VI_DBG, "pre_fe_%d sof chn_num=%d frm_num=%d\n",
			raw_num, ISP_FE_CH0, vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);

		if (vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_sensor) { //YUV sensor
			if (vdev->ctx.isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
				tasklet_hi_schedule(&vdev->job_work);
				vi_event_queue(vdev, VI_EVENT_PRE0_SOF + vi_get_dev_num_by_raw(&vdev->ctx, raw_num),
						vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);
			}
		} else { //RGB sensor
			if (!vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be) {
				_isp_sof_handler(vdev, raw_num);
			}
		}
	}

	if (frame_start & BIT(1))
		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH1];

	if (frame_start & BIT(2))
		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH2];

	if (frame_start & BIT(3))
		++vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH3];
}

static void _isp_pre_fe_frame_done_chk(
	struct sop_vi_dev *vdev,
	const enum sop_isp_raw raw_num,
	const u8 frame_done)
{
	if (frame_done & BIT(0)) {
		vi_record_fe_perf(vdev, raw_num, ISP_FE_CH0);

		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH0);
	}

	if (frame_done & BIT(1)) {
		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH1);
	}

	if (frame_done & BIT(2)) {
		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH2];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH2);
	}

	if (frame_done & BIT(3)) {
		++vdev->pre_fe_frm_num[raw_num][ISP_FE_CH3];
		_isp_pre_fe_done_handler(vdev, raw_num, ISP_FE_CH3);
	}
}

static void _isp_pre_be_frame_done_chk(
	struct sop_vi_dev *vdev,
	const u8 frame_done)
{
	if (frame_done & BIT(0)) {
		vi_record_be_perf(vdev, vi_get_first_raw_num(&vdev->ctx), ISP_BE_CH0);

		_isp_pre_be_done_handler(vdev, ISP_BE_CH0);
	}

	if (frame_done & BIT(1))
		_isp_pre_be_done_handler(vdev, ISP_BE_CH1);
}

static void _isp_postraw_shadow_done_chk(
	struct sop_vi_dev *vdev,
	const u8 shadow_done)
{
	if (shadow_done)
		_isp_postraw_shaw_done_handler(vdev);
}

static void _isp_postraw_frame_done_chk(
	struct sop_vi_dev *vdev,
	const u8 frame_done)
{
	if (frame_done) {
		vi_record_post_end(vdev, vi_get_first_raw_num(&vdev->ctx));

		_isp_postraw_done_handler(vdev);
	}
}

void vi_irq_handler(struct sop_vi_dev *vdev)
{
	struct isp_ctx *ctx = &vdev->ctx;
	union reg_isp_csi_bdg_interrupt_status_0 cbdg_0_sts[ISP_PRERAW_MAX] = { 0 };
	union reg_isp_csi_bdg_interrupt_status_1 cbdg_1_sts[ISP_PRERAW_MAX] = { 0 };
	union reg_isp_top_int_event0 top_sts_0;
	union reg_isp_top_int_event1 top_sts_1;
	union reg_isp_top_int_event2 top_sts_2;
	union reg_isp_top_int_event0_fe345 top_sts_0_fe345;
	union reg_isp_top_int_event1_fe345 top_sts_1_fe345;
	union reg_isp_top_int_event2_fe345 top_sts_2_fe345;
	u8 i = 0, raw_num = ISP_PRERAW0;

	isp_intr_status(ctx, &top_sts_0, &top_sts_1, &top_sts_2,
			&top_sts_0_fe345, &top_sts_1_fe345, &top_sts_2_fe345);

	if (!atomic_read(&vdev->isp_streamon))
		return;

	vi_perf_record_dump();

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;

		isp_csi_intr_status(ctx, raw_num, &cbdg_0_sts[raw_num], &cbdg_1_sts[raw_num]);

		ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_0 = cbdg_0_sts[raw_num].raw;
		ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_1 = cbdg_1_sts[raw_num].raw;

		if (!ctx->isp_pipe_cfg[raw_num].is_bt_demux)
			ctx->isp_pipe_cfg[raw_num].dg_info.fe_sts = ispblk_fe_dbg_info(ctx, raw_num);

		if (raw_num == vi_get_first_raw_num(ctx)) {
			ctx->isp_pipe_cfg[raw_num].dg_info.be_sts = ispblk_be_dbg_info(ctx);
			ctx->isp_pipe_cfg[raw_num].dg_info.post_sts = ispblk_post_dbg_info(ctx);
			ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts = ispblk_dma_dbg_info(ctx);
		}

		for (i = 0; i < ISP_FE_CHN_MAX; i++)
			ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[i] = ispblk_csibdg_chn_dbg(ctx, raw_num, i);
	}

	isp_err_chk(vdev, ctx, cbdg_0_sts, cbdg_1_sts);

	//if (top_sts_0.bits.INT_DMA_ERR)
	//	vi_pr(VI_ERR, "DMA error\n");

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;

		if (ctx->isp_pipe_cfg[raw_num].is_bt_demux) {
			_isp_csibdg_lite_frame_start_chk(vdev, raw_num, cbdg_0_sts[raw_num].raw);
			_isp_csibdg_lite_frame_done_chk(vdev, raw_num, cbdg_0_sts[raw_num].raw);
		}
	}

	_isp_cmdq_done_chk(vdev, top_sts_2.bits.cmdq_int);
	_isp_splt_frame_done_chk(vdev, ISP_PRERAW0, top_sts_0_fe345.bits.line_spliter_dma_done_fe0);
	_isp_splt_frame_done_chk(vdev, ISP_PRERAW1, top_sts_0_fe345.bits.line_spliter_dma_done_fe1);

	_isp_pre_fe_frame_start_chk(vdev, ISP_PRERAW0, top_sts_2.bits.frame_start_fe0);
	_isp_pre_fe_frame_start_chk(vdev, ISP_PRERAW1, top_sts_2.bits.frame_start_fe1);
	_isp_pre_fe_frame_start_chk(vdev, ISP_PRERAW2, top_sts_2.bits.frame_start_fe2);
	_isp_pre_fe_frame_start_chk(vdev, ISP_PRERAW3, top_sts_2_fe345.bits.frame_start_fe3);
	_isp_pre_fe_frame_start_chk(vdev, ISP_PRERAW4, top_sts_2_fe345.bits.frame_start_fe4);
	_isp_pre_fe_frame_start_chk(vdev, ISP_PRERAW5, top_sts_2_fe345.bits.frame_start_fe5);

	_isp_pre_fe_frame_done_chk(vdev, ISP_PRERAW0, top_sts_0.bits.frame_done_fe0);
	_isp_pre_fe_frame_done_chk(vdev, ISP_PRERAW1, top_sts_0.bits.frame_done_fe1);
	_isp_pre_fe_frame_done_chk(vdev, ISP_PRERAW2, top_sts_0.bits.frame_done_fe2);
	_isp_pre_fe_frame_done_chk(vdev, ISP_PRERAW3, top_sts_0_fe345.bits.frame_done_fe3);
	_isp_pre_fe_frame_done_chk(vdev, ISP_PRERAW4, top_sts_0_fe345.bits.frame_done_fe4);
	_isp_pre_fe_frame_done_chk(vdev, ISP_PRERAW5, top_sts_0_fe345.bits.frame_done_fe5);

	_isp_pre_be_frame_done_chk(vdev, top_sts_0.bits.frame_done_be);
	_isp_postraw_shadow_done_chk(vdev, top_sts_0.bits.shaw_done_post);
	_isp_postraw_frame_done_chk(vdev, top_sts_0.bits.frame_done_post);
}

/*******************************************************
 *  Common interface for core
 ******************************************************/
int vi_create_instance(struct platform_device *pdev)
{
	int ret = 0;
	struct sop_vi_dev *vdev;

	vdev = dev_get_drvdata(&pdev->dev);
	if (!vdev) {
		vi_pr(VI_ERR, "invalid data\n");
		return -EINVAL;
	}

	vi_set_base_addr(vdev->reg_base);

	ret = _vi_mempool_setup();
	if (ret) {
		vi_pr(VI_ERR, "Failed to setup isp memory\n");
		goto err;
	}

	ret = _vi_create_proc(vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create proc\n");
		goto err;
	}

	_vi_init_param(vdev);

	ret = vi_create_thread(vdev, E_VI_TH_PRERAW);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create preraw thread\n");
		goto err;
	}

	ret = vi_create_thread(vdev, E_VI_TH_VBLANK_HANDLER);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create vblank_update thread\n");
		goto err;
	}

	ret = vi_create_thread(vdev, E_VI_TH_ERR_HANDLER);
	if (ret) {
		vi_pr(VI_ERR, "Failed to create err_handler thread\n");
		goto err;
	}

	vdev->ddr_retrain_reg = ioremap(VI_DDR_RETRAIN_REG, 0x4);
	if (!vdev->ddr_retrain_reg) {
		vi_pr(VI_ERR, "Failed to ioremap ddr_retrain_reg\n");
		goto err;
	}

	g_vi_ctx = (struct sop_vi_ctx *)vdev->shared_mem;

err:
	return ret;
}

int vi_destroy_instance(struct platform_device *pdev)
{
	int ret = 0, i = 0;
	struct sop_vi_dev *vdev;

	vdev = dev_get_drvdata(&pdev->dev);
	if (!vdev) {
		vi_pr(VI_ERR, "invalid data\n");
		return -EINVAL;
	}

	_vi_destroy_proc(vdev);

	for (i = 0; i < E_VI_TH_MAX; i++)
		vi_destory_thread(vdev, i);

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		sync_task_exit(i);
		kfree(isp_bufpool[i].fswdr_rpt);
		isp_bufpool[i].fswdr_rpt = 0;
	}

	tasklet_kill(&vdev->job_work);

	iounmap(vdev->ddr_retrain_reg);

	return ret;
}

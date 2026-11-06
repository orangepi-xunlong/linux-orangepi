#ifndef __VI_H__
#define __VI_H__

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
#if (KERNEL_VERSION(4, 11, 0) <= LINUX_VERSION_CODE)
#include <uapi/linux/sched/types.h>
#endif

#include <vi_tun_cfg.h>
#include <vi_isp.h>
#include <vi_ctx.h>
#include <vi_common.h>
#include <vip/vi_drv.h>
#include <snsr_i2c.h>
#include <vi_defines.h>
#include <vi_sdk_layer.h>
#include <reg_vi_sys.h>

#include <vi_raw_dump.h>

#define BNR_AI_ISP_BUF_NUM		2
#define OFFLINE_SPLT_BUF_NUM	4
#define OFFLINE_RAW_BUF_NUM		2
#define OFFLINE_PRE_BE_BUF_NUM	2
#define OFFLINE_YUV_BUF_NUM		2
#define MAX_RGBMAP_BUF_NUM		3

// fixed cmd from bmtpu driver
#define BMDEV_SEND_API	      _IOW('p', 0x20, unsigned long)
#define BMDEV_THREAD_SYNC_API _IOW('p', 0x21, unsigned long)

enum sop_isp_state {
	ISP_STATE_IDLE,
	ISP_STATE_RUNNING,
	ISP_STATE_PREPARE,
};

enum sop_isp_bw_limit {
	ISP_BW_LIMIT_RDMA,
	ISP_BW_LIMIT_WDMA0,
	ISP_BW_LIMIT_WDMA1,
	ISP_BW_LIMIT_MAX,
};

enum sop_vi_err {
	ISP_SUCCESS,
	ISP_NO_BUFFER,
	ISP_ERROR,
	ISP_STOP,
	ISP_DROP_FRM,
	ISP_RUNNING,
};

/* struct mempool
 * @base: the address of the mempool
 * @size: the size of the mempool
 * @byteused: the number of bytes used
 */
struct _mempool {
	u64 base;
	u32 size;
	u32 byteused;
} isp_mempool;

struct _membuf {
	u64 splt_le[OFFLINE_SPLT_BUF_NUM];
	u64 splt_se[OFFLINE_SPLT_BUF_NUM];
	u64 pre_fe_le[OFFLINE_RAW_BUF_NUM];
	u64 pre_fe_se[OFFLINE_RAW_BUF_NUM];
	u64 pre_be_le[OFFLINE_PRE_BE_BUF_NUM];
	u64 pre_be_se[OFFLINE_PRE_BE_BUF_NUM];
	u64 yuv_yuyv[ISP_FE_CHN_MAX][OFFLINE_YUV_BUF_NUM];//yuv sensor is yuyv format
	u64 manr[2]; //0 for current iir, 1 for ai_isp iir
	u64 manr_rtile[2]; //tile
	u64 rgbmap_le[MAX_RGBMAP_BUF_NUM];
	u64 rgbmap_se[MAX_RGBMAP_BUF_NUM];
	u64 lmap_le;
	u64 lmap_se;
	u64 lsc;
	u64 tdnr[3]; //0 for motion, 1 for y, 2 for uv
	u64 tdnr_rtile[3]; //tile
	u64 tnr_ai_isp[3]; //0 for y, 1 for u, 2 for v
	u64 tnr_ai_isp_rtile[3]; //tile
	u64 ir_le[OFFLINE_PRE_BE_BUF_NUM];
	u64 ir_se[OFFLINE_PRE_BE_BUF_NUM];
	u64 ldci;
	struct sop_vip_isp_fswdr_report *fswdr_rpt;

	struct sop_isp_sts_mem sts_mem[2];
	u8 pre_fe_sts_busy_idx;
	u8 pre_be_sts_busy_idx;
	u8 pre_be_ir_busy_idx;
	u8 post_sts_busy_idx;

	spinlock_t pre_fe_sts_lock;
	u8 pre_fe_sts_in_use;
	spinlock_t pre_be_sts_lock;
	u8 pre_be_sts_in_use;
	spinlock_t post_sts_lock;
	u8 post_sts_in_use;
} isp_bufpool[ISP_PRERAW_MAX] = {0};

/*
 * splt->dram->fe: splt_out_q <-> pre_fe_in_q
 * fe->dram->be->post: pre_fe_out_q <-> pre_be_in_q
 * fe->be->dram->post: pre_be_out_q <-> postraw_in_q
 */
struct isp_queue splt_out_q[ISP_SPLT_MAX][ISP_SPLT_CHN_MAX],
		pre_fe_in_q[ISP_SPLT_MAX][ISP_SPLT_CHN_MAX],
		pre_fe_out_q[ISP_PRERAW_MAX][ISP_FE_CHN_MAX],
		bnr_ai_isp_q[ISP_PRERAW_MAX][ISP_FE_CHN_MAX],
		pre_be_in_q, pre_be_in_se_q[ISP_PRERAW_MAX],
		pre_be_out_q[ISP_RAW_PATH_MAX],
		postraw_in_q[ISP_RAW_PATH_MAX];

struct _isp_snr_i2c_node {
	struct snsr_regs_s n;
	struct list_head list;
};

struct _isp_snr_cfg_queue {
	struct list_head list;
	u32 num_rdy;
} isp_snr_i2c_queue[ISP_PRERAW_MAX];

struct _ai_isp_cfg_info {
	__u64 ai_bnr_addr_pool[2];
} ai_isp_cfg_info[ISP_PRERAW_MAX];

struct _isp_raw_num_n {
	enum sop_isp_raw raw_num;
	struct list_head list;
};

struct _isp_sof_raw_num_q {
	struct list_head	list;
} pre_raw_num_q;

struct _isp_dqbuf_n {
	u8		raw_id; // vi raw_num
	u8		chn_id; // vi_out buf_chn
	u32		frm_num;
	struct timespec64 timestamp;
	struct list_head list;
};

struct _isp_dqbuf_q {
	struct list_head	list;
} dqbuf_q;

struct vi_event_k {
	struct vi_event		ev;
	struct list_head	list;
};

struct _isp_event_q {
	struct list_head	list;
} event_q;

struct _vi_buffer {
	__u32			raw_id; // raw_id
	__u32			chn_id; // out_buf_chn
	__u32			sequence;
	struct timespec64	timestamp;
	__u32			reserved;
};


static u8 RGBMAP_BUF_IDX	= 2;

static spinlock_t raw_num_lock;
static spinlock_t dq_lock;
static spinlock_t snr_node_lock[ISP_PRERAW_MAX];

static spinlock_t event_lock;

#if (KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE)
typedef struct legacy_timer_emu {
	struct timer_list t;
	void (*function)(unsigned long);
	unsigned long data;
} _timer;
#else
typedef struct timer_list _timer;
#endif

_timer			usr_pic_timer;
static atomic_t		dev_open_cnt;

struct ip_info ip_info_list[IP_INFO_ID_MAX] = {
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE0, sizeof(struct reg_pre_raw_fe_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG0, sizeof(struct reg_isp_csi_bdg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI0_BDG0, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI0_BDG1, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI0_BDG2, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI0_BDG3, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE0_BLC0, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE0_BLC1, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE0_LE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG0, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE0_RGBMAP_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE0_SE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG1, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE0_RGBMAP_SE, sizeof(struct reg_isp_dma_ctl_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE1, sizeof(struct reg_pre_raw_fe_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG1, sizeof(struct reg_isp_csi_bdg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI1_BDG0, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI1_BDG1, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI1_BDG2, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI1_BDG3, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE1_BLC0, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE1_BLC1, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE1_LE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG2, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE1_RGBMAP_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE1_SE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG3, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE1_RGBMAP_SE, sizeof(struct reg_isp_dma_ctl_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE2, sizeof(struct reg_pre_raw_fe_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG2, sizeof(struct reg_isp_csi_bdg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI2_BDG0, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI2_BDG1, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE2_BLC0, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE2_BLC1, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE2_LE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG4, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE2_RGBMAP_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE2_SE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG5, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE2_RGBMAP_SE, sizeof(struct reg_isp_dma_ctl_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE3, sizeof(struct reg_pre_raw_fe_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG3, sizeof(struct reg_isp_csi_bdg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI3_BDG0, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI3_BDG1, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE3_BLC0, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE3_BLC1, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE3_LE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG6, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE3_RGBMAP_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE3_SE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG7, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE3_RGBMAP_SE, sizeof(struct reg_isp_dma_ctl_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE4, sizeof(struct reg_pre_raw_fe_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG4, sizeof(struct reg_isp_csi_bdg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI4_BDG0, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI4_BDG1, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE4_BLC0, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE4_BLC1, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE4_LE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG8, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE4_RGBMAP_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE4_SE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG9, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE4_RGBMAP_SE, sizeof(struct reg_isp_dma_ctl_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE5, sizeof(struct reg_pre_raw_fe_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG5, sizeof(struct reg_isp_csi_bdg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI5_BDG0, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_CSI5_BDG1, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE5_BLC0, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_FE5_BLC1, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE5_LE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG10, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE5_RGBMAP_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_FE5_SE, sizeof(struct reg_isp_rgbmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBMAP_WBG11, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_FE5_RGBMAP_SE, sizeof(struct reg_isp_dma_ctl_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_BE, sizeof(struct reg_pre_raw_be_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BE_CROP_LE, sizeof(struct reg_crop_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BE_CROP_SE, sizeof(struct reg_crop_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_BE_BLC0, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_BE_BLC1, sizeof(struct reg_isp_blc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AF, sizeof(struct reg_isp_af_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_AF_W, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DPC0, sizeof(struct reg_isp_dpc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_PRE_RAW_BE_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_PRE_RAW_BE_SE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_WDMA, sizeof(struct reg_pre_wdma_ctrl_t)},
	// {ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PCHK0, sizeof(struct )},
	// {ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PCHK1, sizeof(struct )},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBIR0, sizeof(struct reg_isp_rgbir_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_RGBIR_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DPC1, sizeof(struct reg_isp_dpc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBIR1, sizeof(struct reg_isp_rgbir_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_RGBIR_SE, sizeof(struct reg_isp_dma_ctl_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WDMA_CORE0, sizeof(struct reg_wdma_core_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WDMA_CORE1, sizeof(struct reg_wdma_core_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WDMA_CORE2, sizeof(struct reg_wdma_core_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_WDMA_CORE3, sizeof(struct reg_wdma_core_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_SPLT_FE0_WDMA_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_SPLT_FE0_WDMA_SE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_SPLT_FE0_WDMA, sizeof(struct reg_pre_wdma_ctrl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_SPLT_FE0_RDMA_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_SPLT_FE0_RDMA_LE, sizeof(struct reg_raw_rdma_ctrl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_SPLT_FE0_RDMA_SE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_SPLT_FE0_RDMA_SE, sizeof(struct reg_raw_rdma_ctrl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_SPLT_FE1_WDMA_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_SPLT_FE1_WDMA_SE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_SPLT_FE1_WDMA, sizeof(struct reg_pre_wdma_ctrl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_SPLT_FE1_RDMA_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_SPLT_FE1_RDMA_LE, sizeof(struct reg_raw_rdma_ctrl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_SPLT_FE1_RDMA_SE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_SPLT_FE1_RDMA_SE, sizeof(struct reg_raw_rdma_ctrl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_SPLT, sizeof(struct reg_isp_line_spliter_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RAWTOP, sizeof(struct reg_raw_top_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CFA0, sizeof(struct reg_isp_cfa_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LSC0, sizeof(struct reg_isp_lsc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_LSC_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_GMS, sizeof(struct reg_isp_gms_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_GMS, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AE_HIST0, sizeof(struct reg_isp_ae_hist_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_AE_HIST_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_AE_HIST1, sizeof(struct reg_isp_ae_hist_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_AE_HIST_SE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_RAW_RDMA0, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RAW_RDMA0, sizeof(struct reg_raw_rdma_ctrl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_RAW_RDMA1, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RAW_RDMA1, sizeof(struct reg_raw_rdma_ctrl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CFA1, sizeof(struct reg_isp_cfa_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LSC1, sizeof(struct reg_isp_lsc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_LSC_SE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LMAP1, sizeof(struct reg_isp_lmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_LMAP_SE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BNR0, sizeof(struct reg_isp_bnr_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_BNR1, sizeof(struct reg_isp_bnr_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RAW_CROP_LE, sizeof(struct reg_crop_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RAW_CROP_SE, sizeof(struct reg_crop_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LMAP0, sizeof(struct reg_isp_lmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_LMAP_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RAW_WBG0, sizeof(struct reg_isp_wbg_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RAW_WBG1, sizeof(struct reg_isp_wbg_t)},
	// {ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PCHK2, sizeof(struct )},
	// {ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PCHK3, sizeof(struct )},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LCAC0, sizeof(struct reg_isp_lcac_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBCAC0, sizeof(struct reg_isp_rgbcac_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LCAC1, sizeof(struct reg_isp_lcac_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBCAC1, sizeof(struct reg_isp_rgbcac_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBTOP, sizeof(struct reg_isp_rgb_top_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CCM0, sizeof(struct reg_isp_ccm_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CCM1, sizeof(struct reg_isp_ccm_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGBGAMMA, sizeof(struct reg_isp_gamma_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YGAMMA, sizeof(struct reg_ygamma_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_MMAP, sizeof(struct reg_isp_mmap_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_MMAP_PRE_LE_R, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_MMAP_PRE_SE_R, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_MMAP_CUR_LE_R, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_MMAP_CUR_SE_R, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_MMAP_IIR_R, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_MMAP_IIR_W, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_MMAP_AI_ISP, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CLUT, sizeof(struct reg_isp_clut_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DEHAZE, sizeof(struct reg_isp_dehaze_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSC, sizeof(struct reg_isp_csc_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RGB_DITHER, sizeof(struct reg_isp_rgb_dither_t)},
	// {ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PCHK4, sizeof(struct )},
	// {ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PCHK5, sizeof(struct )},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_HIST_EDGE_V, sizeof(struct reg_isp_hist_edge_v_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_HIST_EDGE_V, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_FUSION, sizeof(struct reg_fusion_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LTM, sizeof(struct reg_ltm_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_LTM_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_LTM_SE, sizeof(struct reg_isp_dma_ctl_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YUVTOP, sizeof(struct reg_yuv_top_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_TNR, sizeof(struct reg_isp_444_422_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_TNR_ST_MO, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_TNR_LD_MO, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_TNR_ST_Y, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_TNR_ST_C, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_FBCE, sizeof(struct reg_fbce_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_TNR_LD_Y, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_TNR_LD_C, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_FBCD, sizeof(struct reg_fbcd_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YUV_DITHER, sizeof(struct reg_isp_yuv_dither_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CA, sizeof(struct reg_ca_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CA_LITE, sizeof(struct reg_ca_lite_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YNR, sizeof(struct reg_isp_ynr_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CNR, sizeof(struct reg_isp_cnr_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_EE_POST, sizeof(struct reg_isp_ee_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YCURVE, sizeof(struct reg_isp_ycurv_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DCI, sizeof(struct reg_isp_dci_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_DCI, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DCI_GAMMA, sizeof(struct reg_isp_gamma_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YUV_CROP_Y, sizeof(struct reg_crop_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_YUV_CROP_Y, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_YUV_CROP_C, sizeof(struct reg_crop_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_YUV_CROP_C, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_LDCI, sizeof(struct reg_isp_ldci_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_LDCI_W, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_LDCI_R, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_EE_PRE, sizeof(struct reg_isp_preyee_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_AI_ISP_RDMA_Y, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_AI_ISP_RDMA_U, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_AI_ISP_RDMA_V, sizeof(struct reg_isp_dma_ctl_t)},

	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_ISPTOP, sizeof(struct reg_isp_top_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RDMA_CORE0, sizeof(struct reg_rdma_core_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_RDMA_CORE1, sizeof(struct reg_rdma_core_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG0_LITE, sizeof(struct reg_isp_csi_bdg_lite_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_BT0_LITE0, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_BT0_LITE1, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_BT0_LITE2, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_BT0_LITE3, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CSIBDG1_LITE, sizeof(struct reg_isp_csi_bdg_lite_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_BT1_LITE0, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_BT1_LITE1, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_BT1_LITE2, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_BT1_LITE3, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_PRE_RAW_VI_SEL, sizeof(struct reg_pre_raw_vi_sel_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_PRE_RAW_VI_SEL_LE, sizeof(struct reg_isp_dma_ctl_t)},
	{ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_DMA_CTL_PRE_RAW_VI_SEL_SE, sizeof(struct reg_isp_dma_ctl_t)},
	// {ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CMDQ, sizeof(struct )},
};
/*************************************************************************
 *      Init once tuning parameter
 *************************************************************************/

u16 dci_map_lut[] = {
2,    3,    3,    5,    6,    7,    8,   10,   11,   13,   15,   16,   18,   20,   22,   25,
27,   29,   32,   34,   37,   39,   41,   44,   46,   50,   52,   55,   59,   62,   65,   69,
72,   75,   79,   84,   89,   93,   97,  100,  105,  109,  114,  119,  124,  128,  135,  140,
143,  149,  155,  159,  165,  170,  175,  181,  187,  194,  199,  205,  210,  217,  222,  229,
234,  239,  245,  250,  257,  263,  270,  276,  283,  289,  297,  304,  310,  318,  324,  330,
337,  344,  350,  356,  361,  368,  374,  381,  387,  394,  401,  408,  417,  425,  435,  442,
451,  459,  469,  477,  486,  496,  504,  515,  525,  535,  546,  557,  568,  579,  590,  601,
612,  623,  634,  645,  656,  668,  679,  690,  701,  712,  723,  734,  745,  757,  768,  779,
790,  801,  812,  823,  834, 845,  857,  868,  879,  890,  901,  911,  922,  929,  936,  942,
948,  953,  959,  963,  966,  969,  971,  973,  975,  977,  979,  980,  981,  982,  982,  983,
984,  984,  985,  985,  986,  986,  987,  987,  988,  988,  989,  989,  990,  990,  991,  991,
992,  992,  993,  993,  994,  994,  995,  995,  996,  996,  997,  997,  998,  998,  999,  999,
999, 1000, 1000, 1001, 1001, 1001, 1002, 1002, 1002, 1003, 1003, 1004, 1004, 1004, 1005, 1005,
1005, 1006, 1006, 1007, 1007, 1007, 1008, 1008, 1008, 1009, 1009, 1010, 1010, 1010, 1011, 1011,
1011, 1012, 1012, 1013, 1013, 1013, 1014, 1014, 1014, 1015, 1015, 1016, 1016, 1016, 1017, 1017,
1017, 1018, 1018, 1019, 1019, 1019, 1020, 1020, 1020, 1021, 1021, 1022, 1022, 1022, 1023, 1023
};

u16 dci_map_lut_50[] = {
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50
};

u16 lscr_lut[] = {
0x300,   0x310,  0x320,  0x330,  0x340,  0x350,  0x360,  0x370,
0x400,  0x410, 0x420, 0x430, 0x440, 0x450, 0x460, 0x470,
0x500, 0x510, 0x520, 0x530, 0x540, 4095, 4095, 4095,
4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
};

/* for imx327 tuning */
struct isp_ccm_cfg ccm_hw_cfg = {
	.coef = {
			{1024, 0, 0},
			{0, 0, 0},
			{0, 0, 0},
		},
};

u16 gamma_data[] = {
0, 120, 220, 310, 390, 470, 540, 610, 670, 730, 786, 842, 894, 944, 994, 1050, 1096, 1138, 1178,
1218, 1254, 1280, 1314, 1346, 1378, 1408, 1438, 1467, 1493, 1519, 1543, 1568, 1592, 1615, 1638,
1661, 1683, 1705, 1726, 1748, 1769, 1789, 1810, 1830, 1849, 1869, 1888, 1907, 1926, 1945, 1963,
1981, 1999, 2017, 2034, 2052, 2069, 2086, 2102, 2119, 2136, 2152, 2168, 2184, 2200, 2216, 2231,
2247, 2262, 2277, 2292, 2307, 2322, 2337, 2351, 2366, 2380, 2394, 2408, 2422, 2436, 2450, 2464,
2477, 2491, 2504, 2518, 2531, 2544, 2557, 2570, 2583, 2596, 2609, 2621, 2634, 2646, 2659, 2671,
2683, 2696, 2708, 2720, 2732, 2744, 2756, 2767, 2779, 2791, 2802, 2814, 2825, 2837, 2848, 2859,
2871, 2882, 2893, 2904, 2915, 2926, 2937, 2948, 2959, 2969, 2980, 2991, 3001, 3012, 3023, 3033,
3043, 3054, 3064, 3074, 3085, 3095, 3105, 3115, 3125, 3135, 3145, 3155, 3165, 3175, 3185, 3194,
3204, 3214, 3224, 3233, 3243, 3252, 3262, 3271, 3281, 3290, 3300, 3309, 3318, 3327, 3337, 3346,
3355, 3364, 3373, 3382, 3391, 3400, 3409, 3418, 3427, 3436, 3445, 3454, 3463, 3471, 3480, 3489,
3498, 3506, 3515, 3523, 3532, 3540, 3549, 3557, 3566, 3574, 3583, 3591, 3600, 3608, 3616, 3624,
3633, 3641, 3649, 3657, 3665, 3674, 3682, 3690, 3698, 3706, 3714, 3722, 3730, 3738, 3746, 3754,
3762, 3769, 3777, 3785, 3793, 3801, 3808, 3816, 3824, 3832, 3839, 3847, 3855, 3862, 3870, 3877,
3885, 3892, 3900, 3907, 3915, 3922, 3930, 3937, 3945, 3952, 3959, 3967, 3974, 3981, 3989, 3996,
4003, 4010, 4018, 4025, 4032, 4039, 4046, 4054, 4061, 4068, 4075, 4082, 4089, 4095
};

u16 ygamma_data[] = {
0,        20,   40,     59,    79,    99,   119,   139,   159,   178,   198,   218,   240,   263,   286,   312,
338,     365,  394,    424,   456,   489,   523,   558,   595,   633,   673,   714,   756,   800,   845,   892,
940,     990,  1041,  1094,  1148,  1204,  1262,  1320,  1381,  1443,  1507,  1572,  1639,  1708,  1778,  1850,
1923,   1998,  2075,  2154,  2234,  2316,  2400,  2485,  2572,  2661,  2752,  2845,  2939,  3035,  3133,  3233,
3334,   3438,  3543,  3650,  3759,  3870,  3983,  4097,  4214,  4332,  4453,  4575,  4699,  4825,  4953,  5083,
5216,   5350,  5486,  5624,  5764,  5906,  6050,  6196,  6344,  6494,  6646,  6801,  6957,  7115,  7276,  7439,
7603,   7770,  7939,  8110,  8283,  8459,  8636,  8816,  8998,  9182,  9368,  9556,  9747,  9939, 10134, 10331,
10531, 10732, 10936, 11142, 11350, 11561, 11774, 11989, 12206, 12426, 12648, 12872, 13098, 13327, 13558, 13792,
14028, 14266, 14506, 14749, 14994, 15242, 15492, 15744, 15999, 16256, 16515, 16777, 17041, 17308, 17577, 17848,
18122, 18399, 18677, 18959, 19242, 19528, 19817, 20108, 20402, 20698, 20996, 21297, 21601, 21907, 22216, 22527,
22840, 23157, 23475, 23796, 24120, 24447, 24776, 25107, 25441, 25778, 26117, 26459, 26803, 27150, 27500, 27852,
28207, 28564, 28924, 29287, 29652, 30020, 30391, 30764, 31140, 31519, 31900, 32284, 32671, 33060, 33452, 33847,
34245, 34645, 35048, 35453, 35862, 36273, 36686, 37103, 37522, 37944, 38369, 38797, 39227, 39660, 40096, 40535,
40976, 41420, 41867, 42317, 42770, 43225, 43683, 44144, 44608, 45075, 45545, 46017, 46492, 46970, 47451, 47935,
48422, 48911, 49404, 49899, 50397, 50898, 51402, 51909, 52419, 52932, 53447, 53966, 54487, 55012, 55539, 56069,
56603, 57139, 57678, 58220, 58765, 59313, 59864, 60418, 60974, 61534, 62097, 62663, 63232, 63804, 64379, 64956,
65535
};

u16 ycur_data[] = {
255,  252,  248,  244,  240,  236,  232,  228,  224,  220,  216,  212,  208,  204,  200,  196,
192,  188,  184,  180,  176,  172,  168,  164,  160,  156,  152,  148,  144,  140,  136,  132,
128,  124,  120,  116,  112,  108,  104,  100,  96,   92,   88,   84,   80,   76,   72,   68,
64,   60,   56,   52,   48,   44,   40,   36,   32,   28,   24,   20,   16,   12,   8,    4,
0,
};

u16 ltm_d_lut[] = {
0, 256, 512, 768,  1024,  1280,  1536,  1792,  2048,  2304,  2560,  2816,  3072,  3328,  3584,  3840,  4096,
4352,  4608,  4864,  5120,  5376,  5632,  5888,  6144,  6400,  6656,  6912,  7168,  7424,  7680,  7936,  8192,
8448,  8704,  8960,  9216,  9472,  9728,  9984, 10240, 10496, 10752, 11008, 11264, 11520, 11776, 12032, 12288,
12544, 12800, 13056, 13312, 13568, 13824, 14080, 14336, 14592, 14848, 15104, 15360, 15616, 15872, 16128, 16384,
16640, 16896, 17152, 17408, 17664, 17920, 18176, 18432, 18688, 18944, 19200, 19456, 19712, 19968, 20224, 20480,
20736, 20992, 21248, 21504, 21760, 22016, 22272, 22528, 22784, 23040, 23296, 23552, 23808, 24064, 24320, 24576,
24832, 25088, 25344, 25600, 25856, 26112, 26368, 26624, 26880, 27136, 27392, 27648, 27904, 28160, 28416, 28672,
28928, 29184, 29440, 29696, 29952, 30208, 30464, 30720, 30976, 31232, 31488, 31744, 32000, 32256, 32512, 32768,
33024, 33280, 33536, 33792, 34048, 34304, 34560, 34816, 35072, 35328, 35584, 35840, 36096, 36352, 36608, 36864,
37120, 37376, 37632, 37888, 38144, 38400, 38656, 38912, 39168, 39424, 39680, 39936, 40192, 40448, 40704, 40960,
41216, 41472, 41728, 41984, 42240, 42496, 42752, 43008, 43264, 43520, 43776, 44032, 44288, 44544, 44800, 45056,
45312, 45568, 45824, 46080, 46336, 46592, 46848, 47104, 47360, 47616, 47872, 48128, 48384, 48640, 48896, 49152,
49408, 49664, 49920, 50176, 50432, 50688, 50944, 51200, 51456, 51712, 51968, 52224, 52480, 52736, 52992, 53248,
53504, 53760, 54016, 54272, 54528, 54784, 55040, 55296, 55552, 55808, 56064, 56320, 56576, 56832, 57088, 57344,
57600, 57856, 58112, 58368, 58624, 58880, 59136, 59392, 59648, 59904, 60160, 60416, 60672, 60928, 61184, 61440,
61696, 61952, 62208, 62464, 62720, 62976, 63232, 63488, 63744, 64000, 64256, 64512, 64768, 65024, 65280, 65535
};

u16 ltm_b_lut[] = {
0, 4096, 8192, 12288, 16384, 20480, 24576, 28672, 32768, 36864, 40960, 45056, 49152, 53248, 57344, 61440, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535
};

u16 ltm_g_lut[] = {
0, 256, 512, 768, 1024, 1280, 1536,  1792,  2048,  2304,  2560,  2816,  3072,  3328,  3584,  3840,  4096,
4352,  4608,  4864,  5120,  5376,  5632,  5888,  6144,  6400,  6656,  6912,  7168,  7424,  7680,  7936,  8192,
8448,  8704,  8960,  9216,  9472,  9728,  9984, 10240, 10496, 10752, 11008, 11264, 11520, 11776, 12032, 12288,
12544, 12800, 13056, 13312, 13568, 13824, 14080, 14336, 14592, 14848, 15104, 15360, 15616, 15872, 16128, 16384,
16640, 16896, 17152, 17408, 17664, 17920, 18176, 18432, 18688, 18944, 19200, 19456, 19712, 19968, 20224, 20480,
20736, 20992, 21248, 21504, 21760, 22016, 22272, 22528, 22784, 23040, 23296, 23552, 23808, 24064, 24320, 24576,
24832, 25088, 25344, 25600, 25856, 26112, 26368, 26624, 26880, 27136, 27392, 27648, 27904, 28160, 28416, 28672,
28928, 29184, 29440, 29696, 29952, 30208, 30464, 30720, 30976, 31232, 31488, 31744, 32000, 32256, 32512, 32768,
33024, 33280, 33536, 33792, 34048, 34304, 34560, 34816, 35072, 35328, 35584, 35840, 36096, 36352, 36608, 36864,
37120, 37376, 37632, 37888, 38144, 38400, 38656, 38912, 39168, 39424, 39680, 39936, 40192, 40448, 40704, 40960,
41216, 41472, 41728, 41984, 42240, 42496, 42752, 43008, 43264, 43520, 43776, 44032, 44288, 44544, 44800, 45056,
45312, 45568, 45824, 46080, 46336, 46592, 46848, 47104, 47360, 47616, 47872, 48128, 48384, 48640, 48896, 49152,
49408, 49664, 49920, 50176, 50432, 50688, 50944, 51200, 51456, 51712, 51968, 52224, 52480, 52736, 52992, 53248,
53504, 53760, 54016, 54272, 54528, 54784, 55040, 55296, 55552, 55808, 56064, 56320, 56576, 56832, 57088, 57344,
57600, 57856, 58112, 58368, 58624, 58880, 59136, 59392, 59648, 59904, 60160, 60416, 60672, 60928, 61184, 61440,
61696, 61952, 62208, 62464, 62720, 62976, 63232, 63488, 63744, 64000, 64256, 64512, 64768, 65024, 65280, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535
};

#ifdef PORTING_TEST
u16 c_lut_r_lut[] = {
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
1023, 959, 895, 831, 767, 703, 639, 575, 511, 447, 383, 319, 255, 191, 127, 63, 0,
};

u16 c_lut_g_lut[] = {
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

u16 c_lut_b_lut[] = {
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959, 959,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895, 895,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831, 831,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767, 767,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703, 703,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639, 639,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575, 575,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447, 447,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383, 383,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319, 319,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191, 191,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
#endif
/*********************************************************************************************/
static int count;

/* control internal patgen
 *
 * 1: enable
 * 0: disable
 */
static int csi_patgen_en[ISP_PRERAW_MAX] = {0, 0};
module_param_array(csi_patgen_en, int, &count, 0644);

int burst_i2c_en;
module_param(burst_i2c_en, int, 0644);

/* runtime tuning control
 * ctrl:
 *	0: all ch stop update.
 *	1: stop after apply ch1 setting
 *	2: stop after apply ch2 setting
 */
int tuning_dis[4] = {0, 0, 0, 0}; //ctrl, fe, be, post
module_param_array(tuning_dis, int, &count, 0664);

/* Runtime to enable/disable isp_top_clk
 * Ctrl:
 *	0: Disable isp_top_clk dynamic contorl
 *	1: Enable isp_top_clk dynamic control
 */
int clk_dynamic_en;
module_param(clk_dynamic_en, int, 0644);

/* Runtime to enable rgbmap slice buffer mode
 * Ctrl:
 *	0: rgbmap use frame buffer mode, support grid bit 3/4/5.
 *	1: rgbmap use slice buffer mode, support grid bit 3/4.
 */
int rgbmap_sbm_en;
module_param(rgbmap_sbm_en, int, 0644);

/* Force control slice buffer mode
 * Ctrl:
 * 1: enable
 * 0: disable
 */
int sbm_en = 1;
module_param(sbm_en, int, 0644);

/* Force control tile mode for debug
 * Ctrl:
 * 1: enable
 * 0: disable
 */
int tile_en;
module_param(tile_en, int, 0644);

/* Force control line spliter for debug
 * Ctrl:
 * 1: enable
 * 0: disable
 */
int line_spliter_en;
module_param(line_spliter_en, int, 0644);

/* Choose the ip test cases for FPGA verification
 * Ctrl:
 *	See the vi_ip_test_case.c for detail description of each test cases
 */
#ifdef PORTING_TEST
int vi_ip_test_case;
module_param(vi_ip_test_case, int, 0644);
#endif

s8 _pre_hw_enque(struct sop_vi_dev *vdev,
		 const enum sop_isp_raw raw_num,
		 const enum sop_isp_fe_chn_num chn_num);
static void _isp_yuv_bypass_trigger(struct sop_vi_dev *vdev,
				    const enum sop_isp_raw raw_num,
				    const u8 hw_chn_num);
void _isp_fe_be_raw_dump_cfg(struct sop_vi_dev *vdev,
			     const enum sop_isp_raw raw_num,
			     const u8 chn_num);
void isp_post_tasklet(unsigned long data);
static void _vi_sw_init(struct sop_vi_dev *vdev);
#ifndef FPGA_PORTING
static int _vi_clk_ctrl(struct sop_vi_dev *vdev, u8 enable);
#endif
static inline void _post_rgbmap_update(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, const u32 frm_num);
void _postraw_outbuf_enq(struct sop_vi_dev *vdev,
					const enum sop_isp_raw raw_num,
					const enum sop_isp_fe_chn_num chn_num);
void isp_fill_rgbmap(struct isp_ctx *ctx, enum sop_isp_raw raw_num);

static int _vi_preraw_thread(void *arg);
static int _vi_vblank_handler_thread(void *arg);
static int _vi_err_handler_thread(void *arg);
static int _vi_event_handler_thread(void *arg);
static void _splt_hw_enque(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num);
static inline void _vi_wake_up_tpu_th(struct sop_vi_dev *vdev,
				      const enum sop_isp_raw raw_num,
				      const enum ai_isp_type_e type);
#ifdef __cplusplus
}
#endif

#endif /* __VI_H__ */

#ifndef __VI_DEFINES_H__
#define __VI_DEFINES_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/defines.h>
#include <linux/vi_uapi.h>
#include <vi_tun_cfg.h>
#include <vi_isp.h>
#include <vip/vi_drv.h>

enum E_VI_TH {
	E_VI_TH_PRERAW,
	E_VI_TH_ERR_HANDLER,
	E_VI_TH_EVENT_HANDLER,
	E_VI_TH_VBLANK_HANDLER,
	E_VI_TH_RUN_TPU1,
	E_VI_TH_RUN_TPU2,
	E_VI_TH_MAX
};

struct vi_thread_attr {
	char th_name[32];
	struct task_struct *w_thread;
	atomic_t           thread_exit;
	wait_queue_head_t  wq;
	u32                flag;
	int (*th_handler)(void *arg);
};

/**
 * struct sop_vi - VI IP abstraction
 */
struct sop_vi_dev {
	struct device			*dev;
	struct class			*vi_class;
	struct cdev			cdev;
	dev_t				cdev_id;
	void __iomem			*reg_base;
	void __iomem			*ddr_retrain_reg;
	int				irq_num;
	struct clk			*clk_sys[6];
	struct clk			*clk_isp[3];
	struct clk			*clk_mac[6];
	void				*shared_mem;
	struct isp_ctx			ctx;
	struct sop_isp_mbus_framefmt	usr_fmt;
	struct sop_isp_rect		usr_crop;
	struct list_head		rdy_queue[ISP_PRERAW_MAX];
	spinlock_t			rdy_lock;
	u8				num_rdy[ISP_PRERAW_MAX];
	u8				chn_id;
	u64				usr_pic_phy_addr[ISP_RAW_PATH_MAX];
	unsigned long			usr_pic_delay;
	enum sop_isp_source		isp_source;
	struct sop_isp_snr_info		snr_info[ISP_PRERAW_MAX];
	atomic_t			isp_raw_dump_en[ISP_PRERAW_MAX];
	atomic_t			isp_smooth_raw_dump_en[ISP_PRERAW_MAX];
	atomic_t			isp_err_times[ISP_PRERAW_MAX];
	u32				isp_int_flag[ISP_PRERAW_MAX];
	wait_queue_head_t		isp_int_wait_q[ISP_PRERAW_MAX];
	wait_queue_head_t		isp_dq_wait_q;
	wait_queue_head_t		isp_event_wait_q;
	wait_queue_head_t		isp_dbg_wait_q;
	atomic_t			isp_dbg_flag;
	atomic_t			isp_err_handle_flag;
	enum sop_isp_raw		offline_raw_num;
	struct tasklet_struct		job_work;
	struct list_head		qbuf_list[VI_MAX_CHN_NUM][VI_MAX_CHN_NUM];
	spinlock_t			qbuf_lock;
	u8				qbuf_num[VI_MAX_CHN_NUM][VI_MAX_CHN_NUM];
	u32				splt_wdma_frm_num[ISP_SPLT_MAX][ISP_SPLT_CHN_MAX];
	u32				splt_rdma_frm_num[ISP_SPLT_MAX][ISP_SPLT_CHN_MAX];
	u32				pre_fe_sof_cnt[ISP_PRERAW_MAX][ISP_FE_CHN_MAX];
	u32				pre_fe_frm_num[ISP_PRERAW_MAX][ISP_FE_CHN_MAX];
	u32				pre_be_frm_num[ISP_PRERAW_MAX][ISP_BE_CHN_MAX];
	bool				preraw_first_frm[ISP_PRERAW_MAX];
	u32				postraw_frame_number[ISP_PRERAW_MAX];
	u32				drop_frame_number[ISP_PRERAW_MAX];
	u32				dump_frame_number[ISP_PRERAW_MAX];
	u8				postraw_proc_num;
	u8				tpu_thread_num;
	u8				api_buf_swap[ISP_PRERAW_MAX];
	u8				tpu_thd_bind[ISP_PRERAW_MAX];
	atomic_t			splt_state[ISP_SPLT_MAX][ISP_SPLT_CHN_MAX];
	atomic_t			pre_fe_state[ISP_PRERAW_MAX][ISP_FE_CHN_MAX];
	atomic_t			pre_be_state[ISP_BE_CHN_MAX];
	atomic_t			postraw_state;
	atomic_t			isp_streamoff;
	atomic_t			isp_streamon;
	atomic_t			ol_sc_frm_done;
	atomic_t			ai_isp_type;
	atomic_t			bnr_run_tpu[ISP_PRERAW_MAX];
	atomic_t			ai_isp_int_flag[ISP_PRERAW_MAX];
	struct mutex			ai_isp_lock;
	struct completion		tpu_done[ISP_PRERAW_MAX];
	wait_queue_head_t		ai_isp_wait_q[ISP_PRERAW_MAX];
	struct vi_thread_attr		vi_th[E_VI_TH_MAX];
	atomic_t			is_suspend;
	atomic_t			is_suspend_pre_trig_done;
	atomic_t			is_suspend_post_trig_done;
};

#ifdef __cplusplus
}
#endif

#endif /* __VI_DEFINES_H__ */

#ifndef _LDC_COMMON_H_
#define _LDC_COMMON_H_

#include "linux/ldc_uapi.h"
#include "linux/comm_video.h"
#include "linux/comm_vb.h"
#include "base_ctx.h"

#define VIP_ALIGNMENT 0x40
#define LDC_ALIGNMENT (VIP_ALIGNMENT)
#define DWA_ALIGNMENT 0x20

#define LDC_USE_WORKQUEUE 1
#define LDC_USE_THREADED_IRQ 0

#define LDC_YUV_BLACK 0x8000
#define BGCOLOR_GREEN  (YUV_8BIT(0, 128, 128))
#define LDC_RGB_BLACK 0x0

#undef BIT
#undef BITMASK
#define BIT(nr)               (1U << (nr))
#define BITMASK(msb, lsb)     ((2U << (msb))-(1U << (lsb)))
#undef MIN
#define MIN(a, b) (((a) < (b))?(a):(b))
#undef MAX
#define MAX(a, b) (((a) > (b))?(a):(b))
#undef UPPER
#define UPPER(x, y) (((x) + ((1 << (y)) - 1)) >> (y))   // for alignment
#undef VIP_ALIGN
#undef VIP_64_ALIGN
#define VIP_64_ALIGN(x) (((x) + 0x3F) & ~0x3F)   // for 64byte alignment
#define VIP_ALIGN(x) (((x) + 0xF) & ~0xF)   // for 16byte alignment

#define LDC_DEV_MAX_CNT (4)
#define VIP_MAX_PLANES (3)
#define END_JOB_MAX_LEN (16)

#define LDC_IP_NUM                 (2)
#define LDC_PROC_JOB_INFO_NUM      (16)
#define LDC_MAX_JOB_NUM            (64)
#define LDC_JOB_MAX_TSK_NUM        (16)

#ifndef FPGA_PORTING
#define LDC_IDLE_WAIT_TIMEOUT_MS    1000
#define LDC_SYNC_IO_WAIT_TIMEOUT_MS 1000*2
#define LDC_EOF_WAIT_TIMEOUT_MS     200
#else
#define LDC_IDLE_WAIT_TIMEOUT_MS    1000*5
#define LDC_SYNC_IO_WAIT_TIMEOUT_MS 1000*10
#define LDC_EOF_WAIT_TIMEOUT_MS     1000*1
#endif

#define R_IDX 0
#define G_IDX 1
#define B_IDX 2

/***************** ldc callback param *************************/
typedef void (*ldc_cb)(void *, vb_blk);
typedef void (*ldc_tsk_cb)(void *data, int top_id);// tsk callback

enum ldc_usage {
	LDC_USAGE_ROTATION,
	LDC_USAGE_FISHEYE,
	LDC_USAGE_LDC,
	LDC_USAGE_MAX
};

enum ldc_task_type {
	LDC_TASK_TYPE_ROT = 0,
	LDC_TASK_TYPE_FISHEYE,
	LDC_TASK_TYPE_AFFINE,
	LDC_TASK_TYPE_LDC,
	LDC_TASK_TYPE_WARP,
	LDC_TASK_TYPE_MAX,
};

enum ldc_task_state {
	LDC_TASK_STATE_WAIT = 0,
	LDC_TASK_STATE_RUNNING,
	LDC_TASK_STATE_DONE,
	LDC_TASK_STATE_CANCLE,
	LDC_TASK_STATE_FAIL,
	LDC_TASK_STATE_MAX,
};

struct ldc_task {
	struct list_head node;// add to ldc_job task_list
	struct gdc_task_attr attr;
	unsigned long long mesh_id_addr;
	unsigned int bgcolor;
	unsigned int bdcolor;
	enum ldc_task_type type;
	rotation_e rotation;
	atomic_t state;//ldc_task_state
	int tsk_id;
	//char coreid;
};

struct ldc_data {
	unsigned int bytesperline[VIP_MAX_PLANES];
	unsigned int sizeimage[VIP_MAX_PLANES];
	unsigned short w;
	unsigned short h;
};

enum ldc_job_state {
	LDC_JOB_CREAT = 0,
	LDC_JOB_WAIT,
	LDC_JOB_WORKING,
	LDC_JOB_END,
	LDC_JOB_CANCLE,
	LDC_JOB_FAIL,
	LDC_JOB_INVALID,
};

enum ldc_job_devs_type {
	DEVS_LDC = 0,
	DEVS_DWA,
	DEVS_MAX,
};

struct ldc_job {
	spinlock_t lock;
	struct ldc_data cap_data, out_data;
	atomic_t job_state; //ldc_job_state
	struct list_head node;//add to ldc_vdev job_list
	struct list_head task_list;
	atomic_t task_num;
	gdc_identity_attr_s identity;
	bool use_cmdq;
	char coreid;
	wait_queue_head_t job_done_wq;
	bool job_done_evt;
	enum ldc_job_devs_type devs_type; // job dev type
	int proc_idx;
};

struct ldc_vb_doneq {
	struct semaphore sem;
	struct list_head doneq;
	spinlock_t lock;
};

struct ldc_vb_done {
	struct list_head node;//add to struct ldc_vb_doneq doneq
	video_frame_info_s img_out;
	struct ldc_job job;
};

struct mesh_ldc_cfg {
	enum ldc_usage usage;
	const void *usage_param;
	struct vb_s *vb_in;
	pixel_format_e pix_format;
	unsigned long long mesh_addr;
	unsigned char sync_io;
	void *cb_param;
	unsigned int cb_param_size;
	rotation_e rotation;
};

/********************** proc status for ldc ***********************/
struct ldc_proc_tsk_status {
	unsigned int success;   //total doing success num
	unsigned int fail;      //total doing fail num
	unsigned int cancel;    //total doing cancel num
	unsigned int begin_num;   //total commit num
	unsigned int procing_num; //total hw start ProcingNum num
	unsigned int wait_num;    //waiting num
};

struct ldc_proc_tsk_info {
	unsigned int in_size;
	unsigned int out_size;
	enum ldc_task_type type;
	enum ldc_task_state state;
	unsigned char top_id;
	unsigned int hw_time; // HW cost time
	unsigned long long hw_start_time; // us
};

struct ldc_proc_job_status {
	unsigned int success;    //total doing success num
	unsigned int fail;       //total doing fail num
	unsigned int cancel;     //total doing cancel num
	unsigned int begin_num;   //total commit num
	unsigned int procing_num; //total hw start ProcingNum num
	unsigned int wait_num;    //job_list waiting num
};

struct ldc_proc_job_info {
	long long handle;
	gdc_identity_attr_s identity;
	unsigned int task_num; // number of tasks per job
	struct ldc_proc_tsk_info tsk_info[LDC_JOB_MAX_TSK_NUM];
	enum ldc_job_state state; // job state
	unsigned int cost_time; // From job submitted to job done
	unsigned int hw_time; // HW cost time
	unsigned int busy_time; // From job submitted to job commit to driver
	unsigned long long submit_time; // us
	int core_id;
};

struct ldc_proc_operation_status {
	unsigned int add_task_suc;
	unsigned int add_task_fail;
	unsigned int end_job_suc;
	unsigned int end_job_fail;
	unsigned int cb_cnt;
};

struct ldc_proc_core_status {
	unsigned long long hw_start_time; // us
	unsigned int core_all_hw_time;
	unsigned int duty_ratio;
};

struct ldc_proc_ctx {
	struct ldc_proc_job_info job_info[LDC_PROC_JOB_INFO_NUM];
	unsigned short job_idx; // latest job submitted
	struct ldc_proc_job_status job_status;
	struct ldc_proc_tsk_status tsk_status;
	struct ldc_proc_operation_status fisheye_status;
	spinlock_t lock;
	struct ldc_proc_core_status gdc_core_status[LDC_DEV_MAX_CNT];
};

#endif /* _LDC_COMMON_H_ */

#ifndef __BASE_CTX_H__
#define __BASE_CTX_H__

#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/version.h>
#include <linux/common.h>
#include <linux/base_uapi.h>
#include <queue.h>

#define GDC_SHARE_MEM_SIZE          (0x8000)

#define NUM_OF_PLANES               3

#define CHN_MATCH(x, y) (((x)->mod_id == (y)->mod_id) && ((x)->dev_id == (y)->dev_id)             \
		&& ((x)->chn_id == (y)->chn_id))

#define FIFO_HEAD(name, type)						\
	struct name {							\
		struct type *fifo;					\
		int front, tail, capacity;				\
	}

#define FIFO_INIT(head, _capacity) do {						\
		if (_capacity > 0)						\
		(head)->fifo = vmalloc(sizeof(*(head)->fifo) * _capacity);	\
		(head)->front = (head)->tail = -1;				\
		(head)->capacity = _capacity;					\
	} while (0)

#define FIFO_EXIT(head) do {						\
		(head)->front = (head)->tail = -1;			\
		(head)->capacity = 0; 					\
		if ((head)->fifo) 					\
			vfree((head)->fifo);				\
		(head)->fifo = NULL;					\
	} while (0)

#define FIFO_EMPTY(head)    ((head)->front == -1)

#define FIFO_FULL(head)     (((head)->front == ((head)->tail + 1))	\
			|| (((head)->front == 0) && ((head)->tail == ((head)->capacity - 1))))

#define FIFO_CAPACITY(head) ((head)->capacity)

#define FIFO_SIZE(head)     (FIFO_EMPTY(head) ?\
		0 : ((((head)->tail + (head)->capacity - (head)->front) % (head)->capacity) + 1))

#define FIFO_PUSH(head, elm) do {						\
		if (FIFO_EMPTY(head))						\
			(head)->front = (head)->tail = 0;			\
		else								\
			(head)->tail = ((head)->tail == (head)->capacity - 1)	\
					? 0 : (head)->tail + 1;			\
		(head)->fifo[(head)->tail] = elm;				\
	} while (0)

#define FIFO_POP(head, pelm) do {						\
		*(pelm) = (head)->fifo[(head)->front];				\
		if ((head)->front == (head)->tail)				\
			(head)->front = (head)->tail = -1;			\
		else								\
			(head)->front = ((head)->front == (head)->capacity - 1)	\
					? 0 : (head)->front + 1;		\
	} while (0)

#define FIFO_FOREACH(var, head, idx)					\
	for (idx = (head)->front, var = (head)->fifo[idx];		\
		idx < (head)->front + FIFO_SIZE(head);			\
		idx = idx + 1, var = (head)->fifo[idx % (head)->capacity])

#define FIFO_GET_FRONT(head, pelm) (*(pelm) = (head)->fifo[(head)->front])

#define FIFO_GET_TAIL(head, pelm) (*(pelm) = (head)->fifo[(head)->tail])

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST((head));				\
		(var) && ((tvar) = TAILQ_NEXT((var), field), 1);	\
		(var) = (tvar))
#endif

#define CHECK_IOCTL_CMD(cmd, type) \
	if (_IOC_SIZE(cmd) != sizeof(type)) { \
		pr_err("data size error!\n"); \
		return -EINVAL; \
	}

#define MO_TBL_SIZE 256

struct vip_point {
	__u16 x;
	__u16 y;
};

struct vip_line {
	__u16 start;
	__u16 end;
};

struct vip_range {
	__u16 start_x;
	__u16 start_y;
	__u16 end_x;
	__u16 end_y;
};

struct vip_frmsize {
	__u32 width;
	__u32 height;
};

struct mlv_i_s {
	u8 mlv_i_level;
	u8 mlv_i_table[MO_TBL_SIZE];
};

struct mod_ctx_s {
	mod_id_e modid;
	u8 ctx_num;
	void *ctx_info;
};

struct vi_info {
	u8 enable;
	struct {
		u32 blk_idle;
		struct {
	       u32 r_0;
	       u32 r_4;
	       u32 r_8;
	       u32 r_c;
		} dbus_sel[7];
	} isp_top;
	struct {
		u32 preraw_info;
		u32 fe_idle_info;
	} preraw_fe;
	struct {
		u32 preraw_be_info;
		u32 be_dma_idle_info;
		u32 ip_idle_info;
		u32 stvalid_status;
		u32 stready_status;
	} preraw_be;
	struct {
		u32 stvalid_status;
		u32 stready_status;
		u32 dma_idle;
	} rawtop;
	struct {
		u32 ip_stvalid_status;
		u32 ip_stready_status;
		u32 dmi_stvalid_status;
		u32 dmi_stready_status;
		u32 xcnt_rpt;
		u32 ycnt_rpt;
	} rgbtop;
	struct {
		u32 debug_state;
		u32 stvalid_status;
		u32 stready_status;
		u32 xcnt_rpt;
		u32 ycnt_rpt;
	} yuvtop;
	struct {
		u32 dbg_sel;
		u32 status;
	} rdma28[2];
};

enum chn_type_e {
	CHN_TYPE_IN = 0,
	CHN_TYPE_OUT,
	CHN_TYPE_MAX
};

// start point is included.
// end point is excluded.
struct crop_size {
	uint16_t  start_x;
	uint16_t  start_y;
	uint16_t  end_x;
	uint16_t  end_y;
};

struct gdc_mesh {
	u64 paddr;
	void *vaddr;
	u32 meshSize;
	struct mutex lock;
};

enum gdc_job_state {
	GDC_JOB_SUCCESS = 0,
	GDC_JOB_FAIL,
	GDC_JOB_WORKING,
};

struct gdc_job_info {
	int64_t handle;
	mod_id_e mod_id; // the module submitted gdc job
	uint32_t task_num; // number of tasks
	enum gdc_job_state state; // job state
	uint32_t in_size;
	uint32_t out_size;
	uint32_t cost_time; // from job submitted to job done
	uint32_t hw_time; // hw cost time
	uint32_t busy_time; // from job submitted to job commit to driver
	uint64_t submit_time; // us
};

struct gdc_job_status {
	uint32_t success;
	uint32_t fail;
	uint32_t cancel;
	uint32_t begin_num;
	uint32_t busy_num;
	uint32_t procing_num;
};

struct gdc_task_status {
	uint32_t success;
	uint32_t fail;
	uint32_t cancel;
	uint32_t busy_num;
};

struct gdc_operation_status {
	uint32_t add_task_suc;
	uint32_t add_task_fail;
	uint32_t end_suc;
	uint32_t end_fail;
	uint32_t cb_cnt;
};

struct gdc_proc_ctx {
	struct gdc_job_info job_info[GDC_PROC_JOB_INFO_NUM];
	uint16_t job_info_idx; // latest job submitted
	struct gdc_job_status job_status;
	struct gdc_task_status task_status;
	struct gdc_operation_status fisheye_status;
};

struct csc_cfg {
	__u16 coef[3][3];
	__u8 sub[3];
	__u8 add[3];
};

enum vip_input_type {
	VIP_INPUT_ISP = 0,
	VIP_INPUT_LDC,
	VIP_INPUT_SHARE,
	VIP_INPUT_MEM,
	VIP_INPUT_ISP_POST,
	VIP_INPUT_MAX,
};

struct rgn_canvas_ctx {
	u64 phy_addr;
	u8 *virt_addr;
	u32 len;
};

struct rgn_canvas_q {
	struct rgn_canvas_ctx **fifo;
	int  front, tail, capacity;
};

enum vip_pattern {
	VIP_PAT_OFF = 0,
	VIP_PAT_SNOW,
	VIP_PAT_AUTO,
	VIP_PAT_RED,
	VIP_PAT_GREEN,
	VIP_PAT_BLUE,
	VIP_PAT_COLORBAR,
	VIP_PAT_GRAY_GRAD_H,
	VIP_PAT_GRAY_GRAD_V,
	VIP_PAT_BLACK,
	VIP_PAT_MAX,
};

struct rgn_ex_cfg {
	struct rgn_param rgn_ex_param[16];
	struct rgn_odec odec;
	__u8 num_of_rgn_ex;
	__u8 hscale_x2;
	__u8 vscale_x2;
	__u8 colorkey_en;
	__u32 colorkey;
};

struct vpss_rgnex_cfg {
	struct rgn_ex_cfg cfg;
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[3];
};

struct vip_plane {
	__u32 length;
	__u64 addr;
};

struct overflow_info {
	struct vi_info vi_info;
};

#endif  /* __CVI_BASE_CTX_H__ */

/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */

#ifndef X1ISP_DEV_DRV_API_H
#define X1ISP_DEV_DRV_API_H

#include <linux/types.h>

#define IOC_X1_ISP_TYPE       'D'
//#define X1_ISP_DEV_NAME         "x1isp"
//#define X1_ISP_PIPE_DEV_NAME    "x1isp-pipe"
#define X1_ISP_DEV_NAME         "mars11isp"
#define X1_ISP_PIPE_DEV_NAME    "mars11isp-pipe"

#define X1_ISP_MAX_PLANE_NUM 4
#define X1_ISP_MAX_BUFFER_NUM 4

#define X1_ISP_PDC_CHANNEL_NUM 4

#define X1ISP_SLICE_MAX_NUM 6
#define X1ISP_SLICE_REG_MAX_NUM 30

/*----------------------- struct define ----------------------- */
enum isp_pipe_work_type {
	ISP_PIPE_WORK_TYPE_INIT,
	ISP_PIPE_WORK_TYPE_PREVIEW,
	ISP_PIPE_WORK_TYPE_CAPTURE,
	ISP_PIPE_WORK_TYPE_MAX,
};

enum isp_hw_pipeline_id {
	ISP_HW_PIPELINE_ID_0,
	ISP_HW_PIPELINE_ID_1,
	ISP_HW_PIPELINE_ID_MAX,
};

enum isp_stat_id {
	ISP_STAT_ID_AE,
	ISP_STAT_ID_AWB,
	ISP_STAT_ID_LTM,
	ISP_STAT_ID_AF,
	ISP_STAT_ID_PDC,
	ISP_STAT_ID_EIS, //5
	ISP_STAT_ID_MAX,
};

enum isp_pipe_task_type {
	ISP_PIPE_TASK_TYPE_SOF, //firmware calc task at sof
	ISP_PIPE_TASK_TYPE_EOF, //firmware calc task at eof
	ISP_PIPE_TASK_TYPE_AF, //firmware calc AF stat task
	ISP_PIPE_TASK_TYPE_MAX,
};

/**
 * struct isp_buffer_plane - description of buffer plane used in x1isp.
 *
 * @m: the id of the buffer, phy_addr(physical address) or fd(ion buffer).
 * @pitch: the pitch of this plane
 * @offset: the plane's offset of this buffer, usually is zero; .
 * @length: the length of this plane.
 * @reserved: reserve for private use, such as pdc can use for channel_cnt(0) and channel_width(1).
 */
struct isp_buffer_plane {
	union {
		__u64 phy_addr;
		__s32 fd;
	} m;
	__u32 pitch;
	__u32 offset;
	__u32 length;
	__u32 reserved[2];
};

/**
 * struct isp_ubuf_uint - the description of buffer used in user space.
 *
 * @plane_count: the count of planes in this buffer.
 * @buf_index: the index of this buffer in the queue.
 * @buf_planes: planes in this buffer.
 */
struct isp_ubuf_uint {
	__u32 plane_count;
	__s32 buf_index; /* the index of this buffer in the queue. */
	struct isp_buffer_plane buf_planes[X1_ISP_MAX_PLANE_NUM];
};

/**
 * struct isp_buffer_enqueue_info - the info of buffer enqueued by user space.
 *
 * @ubuf_uint: the buffer info enqueued by user space.
 */
struct isp_buffer_enqueue_info {
	struct isp_ubuf_uint ubuf_uint[ISP_STAT_ID_MAX];
};

/**
 * struct isp_buffer_request_info - the info of buffer requested by user space.
 *
 * @stat_buf_count: count of buffer used in the stat's queue.
 */
struct isp_buffer_request_info {
	__u32 stat_buf_count[ISP_STAT_ID_MAX];
};

struct isp_reg_unit {
	__u32 reg_addr;
	__u32 reg_value;
	__u32 reg_mask;
};

struct isp_regs_info {
	union {
		__u64 phy_addr;
		__s32 fd;
	} mem;
	__u32 size;
	void *data; /* contains some isp_reg_unit */
	__u32 mem_index;
};

struct isp_slice_regs {
	__u32 reg_count;
	void *data; /* contains some isp_reg_unit */
};

struct isp_capture_slice_pack {
	__s32 slice_width;
	__s32 raw_read_offset;
	__s32 yuv_out_offset;
	__s32 dwt_offset[4];
	struct isp_slice_regs slice_reg;
};

struct isp_capture_package {
	__u32 slice_count;
	struct isp_capture_slice_pack capture_slice_packs[X1ISP_SLICE_MAX_NUM];
};

enum isp_work_status {
	ISP_WORK_STATUS_INIT,
	ISP_WORK_STATUS_START,
	ISP_WORK_STATUS_STOP,
	ISP_WORK_STATUS_DONE, //for once work, licke capture
	ISP_WORK_STATUS_RESTART,//isp hardware error happen, need to reset
};

enum isp_job_action {
	ISP_JOB_ACTION_START,
	ISP_JOB_ACTION_SWITCH,
	ISP_JOB_ACTION_STOP,
	ISP_JOB_ACTION_RESTART,
};

/**
 * struct isp_job_describer - the job info of isp determined by user space.
 *
 * @work_type: the work type of the hw pipeline, which is defined by &enum isp_pipe_work_type
 * @action: the action of job, which is defined by &enum isp_job_action.
 */
struct isp_job_describer {
	__u32 work_type;
	__u32 action;
};

/**
 * struct isp_drv_deployment - the setting of isp driver setted by user space.
 *
 * @reg_mem: the ID of the buffer(fd or phy addr).
 * @fd_buffer: the buffer use fd to transfer if true.
 * @reg_mem_size: the size of buffer filled with registers.
 * @reg_mem_index: the index of reg memory.
 */
struct isp_drv_deployment {
	union {
		__u64 phy_addr;
		__s32 fd;
	} reg_mem;
	__u32 fd_buffer;
	__u32 reg_mem_size;
	__u32 work_type;
	__u32 reg_mem_index;
};

/**
 * struct isp_user_task_info - the info of task run user space.
 *
 * @task_type: the type fo this user task, whose value defined by &enum isp_pipe_task_type
 * @frame_number: the current frame number of the hw pipeline
 * @work_status: the current work status of the hw pipeline, defined by &isp_work_status
 */
struct isp_user_task_info {
	__u32 task_type;
	__u32 frame_number;
	__u32 work_status;
	__u8 result_valid;
	union {
		struct {
			struct isp_ubuf_uint ltm_result;
			struct isp_ubuf_uint awb_result;
			struct isp_ubuf_uint eis_result;
			struct isp_ubuf_uint ae_result;
			// __u32 rgbir_avg[2]; /* AVG0 and AVG1 */
		} sof_task;
		struct {
			struct isp_ubuf_uint ae_result;
		} eof_task;
		struct {
			struct isp_ubuf_uint af_result;
			struct isp_ubuf_uint pdc_result;
		} af_task;
	} stats_result;
};

struct isp_endframe_work_info {
	__u32 process_ae_by_sof;
	__u32 get_frameinfo_by_eof;
};

/*----------------------- ioctl command ----------------------- */
typedef enum ISP_IOC_NR {
	ISP_IOC_NR_DEPLOY_DRV = 1,
	ISP_IOC_NR_UNDEPLOY_DRV,
	ISP_IOC_NR_SET_REG,
	ISP_IOC_NR_GET_REG,
	ISP_IOC_NR_ENABLE_PDC,
	ISP_IOC_NR_SET_JOB,
	ISP_IOC_NR_GET_INTERRUPT,
	ISP_IOC_NR_REQUEST_BUFFER,
	ISP_IOC_NR_ENQUEUE_BUFFER,
	ISP_IOC_NR_FLUSH_BUFFER,
	ISP_IOC_NR_TRIGGER_CAPTURE,
	ISP_IOC_NR_SET_SINGLE_REG,
	ISP_IOC_NR_SET_END_FRAME_WORK,
} ISP_IOC_NR_E;

#define ISP_IOC_DEPLOY_DRV _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_DEPLOY_DRV, struct isp_drv_deployment)
#define ISP_IOC_UNDEPLOY_DRV _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_UNDEPLOY_DRV, __u32)
#define ISP_IOC_SET_REG _IOW(IOC_X1_ISP_TYPE, ISP_IOC_NR_SET_REG, struct isp_regs_info)
#define ISP_IOC_GET_REG _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_GET_REG, struct isp_regs_info)
#define ISP_IOC_SET_PDC _IOW(IOC_X1_ISP_TYPE, ISP_IOC_NR_ENABLE_PDC, __u32)
#define ISP_IOC_SET_JOB _IOW(IOC_X1_ISP_TYPE, ISP_IOC_NR_SET_JOB, struct isp_job_describer)
#define ISP_IOC_GET_INTERRUPT _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_GET_INTERRUPT, struct isp_user_task_info)
#define ISP_IOC_REQUEST_BUFFER _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_REQUEST_BUFFER, struct isp_buffer_request_info)
#define ISP_IOC_ENQUEUE_BUFFER _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_ENQUEUE_BUFFER, struct isp_buffer_enqueue_info)
#define ISP_IOC_FLUSH_BUFFER _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_FLUSH_BUFFER, __u32)
#define ISP_IOC_TRIGGER_CAPTURE _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_TRIGGER_CAPTURE, struct isp_capture_package)
#define ISP_IOC_SET_SINGLE_REG _IOW(IOC_X1_ISP_TYPE, ISP_IOC_NR_SET_SINGLE_REG, struct isp_reg_unit)
#define ISP_IOC_SET_END_FRAME_WORK _IOW(IOC_X1_ISP_TYPE, ISP_IOC_NR_SET_END_FRAME_WORK, struct isp_endframe_work_info)
#endif

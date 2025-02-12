/* SPDX-License-Identifier: GPL-2.0 */
#ifndef x1_ISP_DRV_H
#define x1_ISP_DRV_H

#include "x1_isp_reg.h"

#include <media/x1/x1_isp_drv_uapi.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

#define x1_ISPDEV_DRV_NAME "x1-ispdev-drv"

#define CAM_MODULE_TAG CAM_MDL_ISP
#include "cam_dbg.h"

/* isp log print */
#define isp_log_err cam_err
#define isp_log_info cam_info
#define isp_log_warn cam_warn
#define isp_log_dbg cam_dbg

#define ISP_DEV_COUNT		(1)
#define ISP_PIPE_LINE_COUNT	(ISP_PIPE_DEV_ID_MAX)
#define ISP_STAT_THROUGH_DMA_COUNT	2	//eis and pdc write through dma
#define ISP_STAT_DMA_IRQ_BIT_MAX	32
#define ISP_STAT_THROUGH_MEM_COUNT	4	//ae,awb,af,ltm result read from mem reg.
#define ISP_VOTER_MAX_NUM		2

#define ISP_DRV_CHECK_POINTER(ptr)			\
	do {						\
		if (NULL == ptr) {			\
			isp_log_err("%s:Null Pointer!", __FUNCTION__);	\
			return -EINVAL;			\
		}					\
	} while (0)

#define ISP_DRV_CHECK_PARAMETERS(value, min, max, name)		\
	do {							\
		if (value < min || value > max) {		\
			isp_log_err("%s: invalid parameter(%s):%d!", __FUNCTION__, name, value);	\
			return -EINVAL;				\
		}						\
	} while(0)

#define ISP_DRV_CHECK_MAX_PARAMETERS(value, max, name)		\
	do {							\
		if (value > max) {				\
			isp_log_err("%s: invalid parameter(%s):%d!", __FUNCTION__, name, value);	\
			return -EINVAL;				\
		}						\
	} while(0)

#define ISP_DRV_ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

#define ISP_STAT_ID_IR_AVG  ISP_STAT_ID_MAX

enum isp_slave_stat_id {
	ISP_SLAVE_STAT_ID_AE = ISP_STAT_ID_IR_AVG + 1,
	ISP_SLAVE_STAT_ID_AWB,
	ISP_SLAVE_STAT_ID_EIS,
	ISP_SLAVE_STAT_ID_AF,
	ISP_SLAVE_STAT_ID_MAX,
};

enum isp_pipe_dev_id {
	ISP_PIPE_DEV_ID_0,	// hardware pipeline0
	ISP_PIPE_DEV_ID_1,	// hardware pipeline1
	// ISP_PIPE_DEV_ID_COMBINATION, //HDR,RGBW,RGBIR, use two pipelines once.
	ISP_PIPE_DEV_ID_MAX,
};

enum isp_buffer_status {
	ISP_BUFFER_STATUS_INVALID = 0,
	ISP_BUFFER_STATUS_IDLE = 1,
	ISP_BUFFER_STATUS_BUSY,
	ISP_BUFFER_STATUS_DONE,
	ISP_BUFFER_STATUS_ERROR,
	ISP_BUFFER_STATUS_MAX = ISP_BUFFER_STATUS_ERROR,
};

/**
 * enum isp_dma_irq_type - the types of dma irq.
 *
 * @ISP_DMA_IRQ_TYPE_ERR: deal with error first,because eof also come when error.
 * @ISP_DMA_IRQ_TYPE_EOF: deal with eof sencod,because the sof of next frame may
 *      come before current eof.
 * @ISP_DMA_IRQ_TYPE_SOF: deal with sof last.
 */
enum isp_dma_irq_type {
	ISP_DMA_IRQ_TYPE_ERR,
	ISP_DMA_IRQ_TYPE_EOF,
	ISP_DMA_IRQ_TYPE_SOF,
	ISP_DMA_IRQ_TYPE_NUM,
};

typedef long (*x1isp_ioctl_func)(struct file *file, unsigned int cmd,
				  unsigned long arg);

struct isp_irq_func_params {
	struct x1isp_pipe_dev *pipe_dev;
	u32 frame_num;
	u32 irq_status;
	u32 hw_pipe_id;
};

/**
 * struct x1isp_irq_handler - the function for handling isp irq.
 *
 * @pipe_dev: point to the current pipe devices.
 * @irq_status: status of irq.
 *
 * The return values:
 * 1 : need schedule the bottom of irq context.
 * 0 : needn't schedule the bottom of irq context.
 */
typedef int (*x1isp_irq_handler)(struct isp_irq_func_params *param);

struct isp_cdev_info {
	struct cdev isp_cdev;
	void *p_dev;
};

struct isp_char_device {
	struct isp_cdev_info *cdev_info;
	u32 cdev_major;
	dev_t cdev_num;
	u32 isp_cdev_cnt;
	struct class *isp_class;
	void *isp_dev;
};

struct isp_kmem_info {
	union {
		u64 phy_addr;
		int fd;
	} mem;
	void *kvir_addr;	//virtual addr of kenel
	u32 mem_size;
	u32 config;
	struct dma_buf *dma_buffer;
};

/**
 * struct isp_kbuffer_info - the description of buffer used in kernel.
 *
 * @frame_id: the frame id of this buffer.
 * @buf_index: the index of this buffer in the queue.
 * @plane_count: the count of planes in this buffer.
 * @buf_status: the status of the buffer whose value is defined by &enum isp_buffer_status.
 * @kvir_addr: the virtual addr of the buffer used in kernel.
 * @phy_addr: record the phy addr of this buffer when fd memory.
 * @hook: the list hook.
 * @buf_planes: planes in this buffer.
 * @dma_buffer: the dma buffer according to fd in the plane.
 */
struct isp_kbuffer_info {
	int frame_id;
	int buf_index;
	u32 plane_count;
	u32 buf_status;
	void *kvir_addr;	//kernel vir addr.
	u64 phy_addr;
	struct list_head hook;
	struct isp_buffer_plane buf_planes[X1_ISP_MAX_PLANE_NUM];
	struct dma_buf *dma_buffer;
};

/**
 * struct isp_stat_buffer_queue - buffer queue used in stat node.
 *
 * @fd_memory: use fd to identify the buffer if true.
 * @fill_by_cpu: datas are filled by cpu.
 * @stat_id: the queue belongs to which stats, defined by &enum isp_stat_id.
 * @buf_count: the count of buffers in the queue.
 * @busy_bufcnt: the count of buffers in busy queue.
 * @@idle_bufcnt: the count of buffers in idle queue.
 * @queue_lock: the spinlock.
 * @busy_buflist: busy buffer queue.
 * @idle_buflist: idle buffer queue.
 * @buf_info: all the buffer can used in this queue on kernel space.
 */
struct isp_stat_buffer_queue {
	u8 fd_memory;
	u8 fill_by_cpu;
	u32 stat_id;
	u32 buf_count;
	u32 busy_bufcnt;
	u32 idle_bufcnt;
	spinlock_t queue_lock;
	struct list_head busy_buflist;
	struct list_head idle_buflist;
	struct isp_kbuffer_info buf_info[X1_ISP_MAX_BUFFER_NUM];
	// bool bNextEofMiss; //when sof find no idle buffer, the next eof must miss.
	// bool bLastEofMiss;
};

/**
 * struct isp_stat_done_info - buffer info for isp stat done.
 *
 * @done_cnt: the count of buffers on done list.
 * @done_list: all the stat buffer are ready for user(dequeued).
 * @done_lock: spinlock for protecting the access to done list.
 */
struct isp_stat_done_info {
	u32 done_cnt;
	struct list_head done_list;
	spinlock_t done_lock;
};

/**
 * struct stat_dma_irq_bits - irq bit info of isp stat dma.
 *
 * @stat_id: the dma belongs to which stat, defined by &enum isp_stat_id.
 * @dma_ch_id: the dma channel id.
 * @irq_bit: the irq bit value.
 */
struct stat_dma_irq_bits {
	u32 stat_id;
	u32 dma_ch_id;
	u32 irq_bit[ISP_DMA_IRQ_TYPE_NUM];
};

enum stat_dma_switch_status {
	STAT_DMA_SWITCH_DYNAMIC_ON = 1,
	STAT_DMA_SWITCH_DYNAMIC_OFF,
	STAT_DMA_SWITCH_STATUS_MAX,
};

/**
 * struct stat_dma_irq_info - current irq info of isp stat dma.
 *
 * @stat_id: the dma belongs to which stat, defined by &enum isp_stat_id.
 * @flag_lock: spinlock for irq_flag.
 * @irq_flag: it's true if irq happens, such as sof, eof, error.
 * @dynamic_switch: dma on/off is dynamic by user, 1 means always open,defined by &enum stat_dma_switch_status
 */
struct stat_dma_irq_info {
	u32 stat_id;
	spinlock_t flag_lock;
	u8 irq_flag[ISP_DMA_IRQ_TYPE_NUM];
	u32 dynamic_switch;
	u32 dynamic_trigger;	//1->trigger on; 2->trigger off
};

struct stat_mem_irq_info {
	u32 stat_id;
	u8 start_read;
	spinlock_t mem_flag_lock;
};

enum pipe_event_type {
	PIPE_EVENT_TRIGGER_VOTE_SYS = 1,
	PIPE_EVENT_CAST_VOTE,
	PIPE_EVENT_TYPE_MAX,
};

struct stats_notify_params {
	u32 stat_id;
	u32 event_enable;
	u32 frame_id;
};

/**
 * struct x1isp_stats_node - isp stat node including all stats on one pipeline.
 *
 * @stat_active: the stat is active if true.
 * @hw_pipe_id: this stats node belong to which hardware pipeline.
 * @stat_bufqueue: the buffer queue in this stats node.
 * @stat_done_info: buffer info for isp stat done.
 * @stat_dma_irq_bits: irq bits of stat written through dma.
 * @dma_irq_info: the stat's dma irq info, the results of this stat are written through dma.
 * @mem_irq_index: the index in mem_irq_info array for every stat.
 * @mem_irq_info: the stat's mem irq info, the results of this stat are read through registers.
 */
struct x1isp_stats_node {
	void *private_dev;
	bool stat_active[ISP_STAT_ID_MAX];
	u32 hw_pipe_id;
	struct isp_stat_buffer_queue stat_bufqueue[ISP_STAT_ID_MAX];
	struct isp_stat_done_info stat_done_info[ISP_STAT_ID_MAX];
	struct stat_dma_irq_bits stat_dma_irq_bitmap[ISP_STAT_THROUGH_DMA_COUNT];
	struct stat_dma_irq_info dma_irq_info[ISP_STAT_THROUGH_DMA_COUNT];
	int mem_irq_index[ISP_STAT_ID_MAX];
	struct stat_mem_irq_info mem_irq_info[ISP_STAT_THROUGH_MEM_COUNT];
	int (*notify_event)(void *private_dev, u32 event, void *payload,
			    u32 payload_len);
};

/**
 * struct x1_isp_irq_context - all isp irq work
 *
 * @hw_pipe_id: the master hardware pipeline for this irq ctx.
 * @isp_irq_bitmap: isp irq bit map.
 * @cur_frame_num: the current frame number.
 * @isp_irq_tasklet: the bottom of isp irq handler.
 * @isp_dma_irq_tasklet: the bottom of isp dma irq handler.
 */
struct x1isp_irq_context {
	u32 hw_pipe_id;
	ulong isp_irq_bitmap;
	atomic_t cur_frame_num;
	x1isp_irq_handler isp_irq_handler[ISP_IRQ_BIT_MAX_NUM];
	struct tasklet_struct isp_irq_tasklet;
	struct tasklet_struct isp_dma_irq_tasklet;
};

struct pipe_task_stat_config {
	u32 frame_num;
	ulong mem_stat_bitmap;
};

enum task_voter_type {
	TASK_VOTER_SDE_SOF,
	TASK_VOTER_AE_EOF,
	TASK_VOTER_AF_EOF,
	TASK_VOTER_PDC_EOF,
	TASK_VOTER_TYP_MAX,
};

/**
 * struct task_voting_system - a simple vote system for deal with all irq conditions.
 *
 * @sys_trigger_num: the trigger number(voter number) of this vote system.
 * @cur_ticket_cnt: current vote tickets.
 * @voter_index: the index of voter in the system(voter_validity array).
 * @voter_validity: the flag to recording ticket for voter.
 * @voter_frameID: the frame number for current ticket.
 */
struct task_voting_system {
	u32 sys_trigger_num;
	u32 cur_ticket_cnt;
	int voter_index[TASK_VOTER_TYP_MAX];
	u8 voter_validity[ISP_VOTER_MAX_NUM];
	u32 voter_frameID[ISP_VOTER_MAX_NUM];
	spinlock_t vote_lock;
};

/**
 * struct isp_pipe_task - the pipe's main task oriented to user.
 *
 * @task_type: task's type defined by &enum isp_pipe_task_type
 * @frame_num: the current frame number.
 * @task_trigger: wakeup user for this task if ture;
 * @complete_cnt: task complete count.
 * @complete_frame_num: current complete frame num.
 * @stat_bits_cnt: the total count of bits in stat_bitmap
 * @stat_bitmap: which stats are used in this task.
 * @wait_complete: the completion for user.
 * @task_lock: spinlock for irq ctx and tasklet.
 * @complete_lock: spinlock for user wait complete, used between takslet and thread.
 * @user_stat_cfg: the stat config setted by user for this task.
 * @vote_system: voting system for multi condtions to wakeup user
 * @use_vote_sys: if ture use voting system.
 */
struct isp_pipe_task {
	u32 task_type;
	atomic_t frame_num;
	u32 task_trigger;
	int complete_cnt;
	u32 complete_frame_num;
	u32 stat_bits_cnt;
	ulong stat_bitmap;
	struct completion wait_complete;
	spinlock_t task_lock;
	spinlock_t complete_lock;
	struct pipe_task_stat_config user_stat_cfg;
	struct task_voting_system vote_system;
	u8 use_vote_sys;
};

/**
 * struct x1isp_pipe_dev - abstraction of isp pipeline
 *
 * @dev_num: belongs to which isp device.
 * @open_cnt: the count of open isp pipe device.
 * @pipedev_id: this pipeline's ID whose value is defined by &enum isp_pipe_dev_id. Notice that the isp
 *      hardware has two pipelines, but the combined pipe(like HDR,RGBW) is abstracted by software.
 * @stats_node_cnt: the count of stats node.
 * @work_status: current work status of pipedev, defined by &enum isp_work_status.
 * @work_type: current work type of pipedev, defined by enum isp_pipe_work_type
 * @fd_buffer: use fd to describe buffer if true;
 * @capture_client_num: the number of capture clinets on this pipe dev.
 * @stream_restart: the stream restart and frameid comes for zero.
 * @eof_task_hd_by_sof: eof task process at sof task(ae handle at startframe).
 * @isp_reg_mem: the register buffer memory setted by user space.
 * @frame_info_mem: the frame info buffer memory setted by user space.
 * @pipe_task: info of pipe task.
 * @stats_nodes: ae,awb,af,eis stat work of pipeline, combination pipe may have two nodes, like hdr mode.
 * @hook: list hook.
 * @isp_pipedev_mutex: mutex for isp pipe device.
 * @pipedev_capture_mutex: mutex for isp pipe device do capture.
 * @slice_reg_mem: the memory for a slice regs.
 */
struct x1isp_pipe_dev {
	u32 dev_num;
	u32 open_cnt;
	u32 pipedev_id;
	u32 stats_node_cnt;
	u32 work_status;
	u32 work_type;
	u32 fd_buffer;
	u32 capture_client_num;
	u32 stream_restart;
	u32 eof_task_hd_by_sof;
	u32 frameinfo_get_by_eof;
	struct isp_kmem_info isp_reg_mem[ISP_PIPE_WORK_TYPE_CAPTURE + 1];	//0:preview,1:capture_a, 2: capture_b
	// struct isp_kmem_info frame_info_mem;
	struct isp_pipe_task pipe_tasks[ISP_PIPE_TASK_TYPE_MAX];
	struct x1isp_stats_node *stats_nodes;
	struct x1isp_irq_context isp_irq_ctx;
	struct list_head hook;
	struct mutex isp_pipedev_mutex;
	struct mutex pipedev_capture_mutex;
	void *slice_reg_mem;
};

/**
 * struct x1_isp_dev - abstraction of isp device.
 *
 * @dev_num: this isp device's ID, we may support two isp devices.;
 * @open_cnt: the count of open isp device.
 * @clk_ref: the reference of clock.
 * @pdev: platform device.
 * @isp_reg_source: isp registers from dts file.
 * @ahb_clk: ahb clock of isp
 * @fnc_clk: isp func clk
 * @axi_clk: axi clock
 * @pipe_devs: isp pipeline devices.
 * @reset_irq_complete: global reset irq done complete
 * @restart_complete: restart done complete, which called by vi moudles.
 * @isp_dev_lock: spinlock for isp device.
 */
struct x1isp_dev {
	u8 dev_num;
	u32 open_cnt;
	atomic_t clk_ref;
	struct platform_device *plat_dev;
	struct resource *isp_reg_source;
	ulong __iomem isp_regbase;
	ulong __iomem isp_regend;
//	struct clk *ahb_clk;
	struct reset_control *ahb_reset;
	struct reset_control *isp_reset;
	struct reset_control *isp_ci_reset;
	struct reset_control *lcd_mclk_reset;

	struct clk *fnc_clk;
	struct clk *axi_clk;
	struct clk *dpu_clk;
	struct x1isp_pipe_dev *pipe_devs[ISP_PIPE_DEV_ID_MAX];
	struct completion reset_irq_complete;
	struct completion restart_complete;
	struct spm_camera_vi_ops *vi_funs;
};

int x1isp_dev_clock_set(int enable);
int x1isp_dev_open(void);
int x1isp_dev_release(void);
long x1isp_dev_copy_user(struct file *file, unsigned int cmd, void *arg,
			  x1isp_ioctl_func func);
int x1isp_dev_get_pipedev(u32 hw_pipe_id, struct x1isp_pipe_dev **pp_pipedev);
int x1isp_dev_get_viraddr_from_dma_buf(struct dma_buf *dma_buffer, void **pp_vir_addr);
int x1isp_dev_put_viraddr_to_dma_buf(struct dma_buf *dma_buffer, void *vir_addr);
int x1isp_dev_get_phyaddr_from_dma_buf(int fd, __u64 *phy_addr);
int x1isp_dev_get_vi_ops(struct spm_camera_vi_ops **pp_vi_ops);
#endif

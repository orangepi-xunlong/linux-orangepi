#ifndef __U_VI_EXT_H__
#define __U_VI_EXT_H__

#define ISP_SYNC_TSK_MAX_NUM        4
#define ISP_SYNC_TASK_ID_MAX_LENGTH 64

enum isp_sync_tsk_method {
	ISP_SYNC_TSK_METHOD_HW_IRQ = 0,
	ISP_SYNC_TSK_METHOD_WORKQUE,

	ISP_SYNC_TSK_METHOD_BUTT
};

struct isp_sync_task_node {
	enum isp_sync_tsk_method method;
	__s32 (*isp_sync_tsk_call_back)(__u64 data);
	__u64 data;
	const char *sz_id;
	struct list_head list;
};

struct list_entry {
	__u32 num;
	struct list_head head;
};

struct isp_sync_tsk_ctx {
	int vi_pipe;
	struct work_struct worker;
	struct list_entry hwirq_list;
	struct list_entry workqueue_list;
	struct semaphore sem;
};

int isp_sync_task_process(int vi_pipe);
int isp_sync_task_register(int vi_pipe, struct isp_sync_task_node *new_node);
int isp_sync_task_unregister(int vi_pipe, struct isp_sync_task_node *del_node);
void sync_task_init(int vi_pipe);
void sync_task_exit(int vi_pipe);

#endif // __U_VI_EXT_H__
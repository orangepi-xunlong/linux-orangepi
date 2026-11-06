#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/semaphore.h>

#include <vi_tun_cfg.h>
#include <vi_ext.h>
#include <vi_common.h>

/* isp sync task */
struct isp_sync_tsk_ctx g_isp_sync_tsk_ctx[ISP_PRERAW_MAX];

#define isp_sync_tsk_get_ctx(dev) (&g_isp_sync_tsk_ctx[dev])

static void isp_sync_task_find_and_execute(int vi_pipe, struct list_head *head)
{
	struct list_head *pos = NULL;
	struct list_head *next = NULL;
	struct isp_sync_task_node *sync_tsk_node = NULL;

	if (!list_empty(head)) {
		list_for_each_safe(pos, next, head) {
			sync_tsk_node = list_entry(pos, struct isp_sync_task_node, list);

			if (sync_tsk_node->isp_sync_tsk_call_back) {
				sync_tsk_node->isp_sync_tsk_call_back(sync_tsk_node->data);
			}
		}
	}
}

static void work_queue_handler(struct work_struct *worker)
{
	struct isp_sync_tsk_ctx *sync_tsk = container_of((void *)worker, struct isp_sync_tsk_ctx, worker);

	if (down_interruptible(&sync_tsk->sem)) {
		return;
	}

	isp_sync_task_find_and_execute(sync_tsk->vi_pipe, &sync_tsk->workqueue_list.head);

	up(&sync_tsk->sem);
}

static struct list_head *search_node(struct list_head *head, const char *id)
{
	struct list_head *pos = NULL;
	struct list_head *next = NULL;
	struct isp_sync_task_node *sync_tsk_node = NULL;

	list_for_each_safe(pos, next, head) {
		sync_tsk_node = list_entry(pos, struct isp_sync_task_node, list);
		if (!strncmp(sync_tsk_node->sz_id, id, ISP_SYNC_TASK_ID_MAX_LENGTH)) {
			return pos;
		}
	}

	return NULL;
}

int isp_sync_task_register(int vi_pipe, struct isp_sync_task_node *new_node)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);
	struct list_head *target_list = NULL;
	struct list_head *pos = NULL;
	struct list_entry *list_entry_tmp = NULL;

	if (new_node == NULL) {
		return -1;
	}

	if (new_node->method == ISP_SYNC_TSK_METHOD_HW_IRQ) {
		target_list = &sync_tsk->hwirq_list.head;
	} else {
		target_list = &sync_tsk->workqueue_list.head;
	}

	list_entry_tmp = list_entry(target_list, struct list_entry, head);
	if (list_entry_tmp == NULL) {
		return -1;
	}

	if (list_entry_tmp->num >= ISP_SYNC_TSK_MAX_NUM) {
		return -1;
	}

	pos = search_node(target_list, new_node->sz_id);
	if (pos) {
		return -1;
	}

	if (down_interruptible(&sync_tsk->sem)) {
		return -ERESTARTSYS;
	}

	list_add_tail(&new_node->list, target_list);

	list_entry_tmp->num++;

	up(&sync_tsk->sem);

	return 0;
}
EXPORT_SYMBOL_GPL(isp_sync_task_register);

int isp_sync_task_unregister(int vi_pipe, struct isp_sync_task_node *del_node)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);
	struct list_head *target_list = NULL;
	struct list_entry *list_entry_tmp = NULL;
	struct list_head *pos = NULL;
	int del_success = -1;

	if (del_node == NULL) {
		return -1;
	}

	if (del_node->method == ISP_SYNC_TSK_METHOD_HW_IRQ) {
		target_list = &sync_tsk->hwirq_list.head;
	} else {
		target_list = &sync_tsk->workqueue_list.head;
	}

	list_entry_tmp = list_entry(target_list, struct list_entry, head);
	if (list_entry_tmp == NULL) {
		return -1;
	}

	if (down_interruptible(&sync_tsk->sem)) {
		return -ERESTARTSYS;
	}

	pos = search_node(target_list, del_node->sz_id);

	if (pos) {
		list_del(pos);
		if (list_entry_tmp->num > 0) {
			list_entry_tmp->num = list_entry_tmp->num - 1;
		}

		del_success = 0;
	}

	up(&sync_tsk->sem);

	return del_success;
}
EXPORT_SYMBOL_GPL(isp_sync_task_unregister);

int isp_sync_task_process(int vi_pipe)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);

	if (sync_tsk->hwirq_list.num) {
		isp_sync_task_find_and_execute(vi_pipe, &sync_tsk->hwirq_list.head);
	}

	if (sync_tsk->workqueue_list.num) {
		schedule_work(&sync_tsk->worker);
	}

	return 0;
}

void sync_task_init(int vi_pipe)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);

	INIT_LIST_HEAD(&sync_tsk->hwirq_list.head);
	INIT_LIST_HEAD(&sync_tsk->workqueue_list.head);

	sync_tsk->hwirq_list.num     = 0;
	sync_tsk->workqueue_list.num = 0;
	sema_init(&sync_tsk->sem, 1);

	INIT_WORK(&sync_tsk->worker, work_queue_handler);

	vi_pr(VI_INFO, "vi_pipe %d\n", vi_pipe);
}

void sync_task_exit(int vi_pipe)
{
	struct isp_sync_tsk_ctx *sync_tsk = isp_sync_tsk_get_ctx(vi_pipe);

	cancel_work_sync(&sync_tsk->worker);

	sync_tsk->hwirq_list.num     = 0;
	sync_tsk->workqueue_list.num = 0;

	vi_pr(VI_INFO, "vi_pipe %d\n", vi_pipe);
}

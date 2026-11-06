#include <vi_isp_buf_ctrl.h>

spinlock_t buf_lock;

struct isp_buffer *isp_buf_next(struct isp_queue *q)
{
	struct isp_buffer *b = NULL;
	unsigned long flags;

	spin_lock_irqsave(&buf_lock, flags);
	if (!list_empty(&q->rdy_queue))
		b = list_first_entry(&q->rdy_queue, struct isp_buffer, list);
	spin_unlock_irqrestore(&buf_lock, flags);

	return b;
}

struct isp_buffer *isp_buf_last(struct isp_queue *q)
{
	struct isp_buffer *b = NULL;
	unsigned long flags;

	spin_lock_irqsave(&buf_lock, flags);
	if (!list_empty(&q->rdy_queue))
		b = list_last_entry(&q->rdy_queue, struct isp_buffer, list);
	spin_unlock_irqrestore(&buf_lock, flags);

	return b;
}

void isp_buf_queue(struct isp_queue *q, struct isp_buffer *b)
{
	unsigned long flags;

	if (b == NULL)
		return;

	spin_lock_irqsave(&buf_lock, flags);
	list_add_tail(&b->list, &q->rdy_queue);
	++q->num_rdy;
	spin_unlock_irqrestore(&buf_lock, flags);
}

struct isp_buffer *isp_buf_remove(struct isp_queue *q)
{
	struct isp_buffer *b = NULL;
	unsigned long flags;

	spin_lock_irqsave(&buf_lock, flags);
	if (!list_empty(&q->rdy_queue)) {
		b = list_first_entry(&q->rdy_queue, struct isp_buffer, list);
		list_del_init(&b->list);
		--q->num_rdy;
	}
	spin_unlock_irqrestore(&buf_lock, flags);

	return b;
}

int isp_buf_empty(struct isp_queue *q)
{
	unsigned long flags;
	int empty = 0;

	spin_lock_irqsave(&buf_lock, flags);
	if (list_empty(&q->rdy_queue)) {
		empty = 1;
	}
	spin_unlock_irqrestore(&buf_lock, flags);

	return empty;
}

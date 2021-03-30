#undef TRACE_SYSTEM
#define TRACE_SYSTEM ion

#if !defined(_TRACE_ION_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_ION_H
#include <linux/tracepoint.h>

#define ION_HEAP_MASK	\
	{(unsigned int)ION_HEAP_SYSTEM_MASK,		"system"}, \
	{(unsigned int)ION_HEAP_SYSTEM_CONTIG_MASK,	"system contig"},	\
	{(unsigned int)ION_HEAP_CARVEOUT_MASK,		"carveout"},	\
	{(unsigned int)ION_HEAP_TYPE_DMA_MASK,		"cma"}

#define show_heap_mask(mask)	__print_flags((unsigned int)flags, "|", ION_HEAP_MASK)

TRACE_EVENT(ion_alloc,

	TP_PROTO(size_t len,
	size_t align,
	unsigned int heap_id_mask,
	unsigned int flags,
	unsigned int handle_id,
	void *buf_ptr
	),

	TP_ARGS(len, align, heap_id_mask, flags, handle_id, buf_ptr),

	TP_STRUCT__entry(
		__field(	size_t,		len		)
		__field(	size_t,		align		)
		__field(	unsigned int,	heap_id_mask	)
		__field(	unsigned int,	flags		)
		__field(	unsigned int,	handle_id	)
		__field(	void *,		buf_ptr		)
	),

	TP_fast_assign(
		__entry->len			= len;
		__entry->align			= align;
		__entry->heap_id_mask		= heap_id_mask;
		__entry->flags			= flags;
		__entry->handle_id		= handle_id;
		__entry->buf_ptr		= buf_ptr;
	),

	TP_printk("len %zu align %zu heap:%u flags %x handle id %d buffer %p",
		__entry->len,
		__entry->align,
		//show_heap_mask(__entry->heap_id_mask),
		__entry->heap_id_mask,
		__entry->flags,
		__entry->handle_id,
		__entry->buf_ptr)
);

TRACE_EVENT(ion_free_handle,

	TP_PROTO(unsigned int handle_id,
		int handle_count,
		void *buf_ptr
		),

	TP_ARGS(handle_id, handle_count, buf_ptr),

	TP_STRUCT__entry(
		__field(	unsigned int,	handle_id	)
		__field(	int,		handle_count	)
		__field(	void *,		buf_ptr		)
	),

	TP_fast_assign(
		__entry->handle_id		= handle_id;
		__entry->handle_count		= handle_count;
		__entry->buf_ptr		= buf_ptr;
	),

	TP_printk(" handle id %d free ,buffer:%p handle_count %d",
		__entry->handle_id,
		__entry->buf_ptr,
		__entry->handle_count)
);

TRACE_EVENT(ion_free_buffer,

	TP_PROTO(void *buf_ptr),

	TP_ARGS(buf_ptr),

	TP_STRUCT__entry(
		__field(	void *,		buf_ptr		)
	),

	TP_fast_assign(
		__entry->buf_ptr		= buf_ptr;
	),

	TP_printk("buffer:%p",
		__entry->buf_ptr
	)
);

TRACE_EVENT(ion_map,

	TP_PROTO(unsigned int handle_id, int fd),

	TP_ARGS(handle_id, fd),

	TP_STRUCT__entry(
		__field(	unsigned int,	handle_id	)
		__field( 	int,		fd		)
	),

	TP_fast_assign(
		__entry->handle_id		= handle_id;
		__entry->fd			= fd;
	),

	TP_printk("handle id:%d dma_buf fd:%d",
		__entry->handle_id,
		__entry->fd
	)
);

TRACE_EVENT(ion_import,

	TP_PROTO(unsigned int handle_id, int fd),

	TP_ARGS(handle_id, fd),

	TP_STRUCT__entry(
		__field(	unsigned int,	handle_id	)
		__field( 	int,		fd		)
	),

	TP_fast_assign(
		__entry->handle_id		= handle_id;
		__entry->fd			= fd;
	),

	TP_printk("new handle id:%d dma_buf fd:%d",
		__entry->handle_id,
		__entry->fd
	)
);

TRACE_EVENT(ion_sync,

	TP_PROTO(int fd),

	TP_ARGS(fd),

	TP_STRUCT__entry(
		__field( 	int,	fd	)
	),

	TP_fast_assign(
		__entry->fd			= fd;
	),

	TP_printk("sync dma_buf fd:%d",
		__entry->fd
	)
);

TRACE_EVENT(ion_flush_dma_range,

	TP_PROTO(void * dma_start, void * dma_end),

	TP_ARGS(dma_start, dma_end),

	TP_STRUCT__entry(
		__field(	void *,	dma_start	)
		__field( 	void *,	dma_end		)
	),

	TP_fast_assign(
		__entry->dma_start	= dma_start;
		__entry->dma_end	= dma_end;
	),

	TP_printk("start %p end %p",
		__entry->dma_start,
		__entry->dma_end
	)
);

TRACE_EVENT(ion_client_creat,

	TP_PROTO(struct task_struct *task, pid_t pid),

	TP_ARGS(task, pid),

	TP_STRUCT__entry(
		__field(	struct task_struct *,	task	)
		__field(	pid_t,	pid	)
	),

	TP_fast_assign(
		__entry->task		= task;
		__entry->pid		= pid;
	),

	TP_printk("new client %s pid %d",
		(__entry->task)?(__entry->task->comm):("kernel thread"),
		__entry->pid
	)
);

TRACE_EVENT(ion_client_free,

	TP_PROTO(struct task_struct *task, pid_t pid),

	TP_ARGS(task, pid),

	TP_STRUCT__entry(
		__field(	struct task_struct *,	task	)
		__field(	pid_t,	pid	)
	),

	TP_fast_assign(
		__entry->task		= task;
		__entry->pid		= pid;
	),

	TP_printk("free client %s pid %d",
		__entry->task?__entry->task->comm:"kernel thread",
		__entry->pid
	)
);
#endif

/* This part must be outside protection */
#include <trace/define_trace.h>

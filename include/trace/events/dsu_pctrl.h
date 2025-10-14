#undef TRACE_SYSTEM
#define TRACE_SYSTEM dsu_pctrl

#if !defined(_TRACE_DSU_PCTRL_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_DSU_PCTRL_H

#include <linux/tracepoint.h>

TRACE_EVENT(dsu_pctrl_dev_status,
	TP_PROTO(int dsu_id, unsigned long hits, unsigned long misses,
		 unsigned long freq, unsigned long busy_time,
		 unsigned long total_time),
	TP_ARGS(dsu_id, hits, misses, freq, busy_time, total_time),
	TP_STRUCT__entry(
		__field(int, dsu_id)
		__field(unsigned long, hits)
		__field(unsigned long, misses)
		__field(unsigned long, freq)
		__field(unsigned long, busy_time)
		__field(unsigned long, total_time)
	),
	TP_fast_assign(
		__entry->dsu_id = dsu_id;
		__entry->hits = hits;
		__entry->misses = misses;
		__entry->freq = freq;
		__entry->busy_time = busy_time;
		__entry->total_time = total_time;
	),

	TP_printk("dsu_dev_id=%d hits=%lu misses=%lu cur_freq=%lu busy_time=%lu total_time=%lu",
		  __entry->dsu_id, __entry->hits, __entry->misses,
		  __entry->freq, __entry->busy_time, __entry->total_time)
);
#endif /* _TRACE_DSU_PCTRL_H */

/* This part must be outside protection */
#include <trace/define_trace.h>

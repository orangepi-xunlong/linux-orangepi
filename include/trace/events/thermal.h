#undef TRACE_SYSTEM
#define TRACE_SYSTEM thermal

#if !defined(_TRACE_THERMAL_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_THERMAL_H
#include <linux/tracepoint.h>



/*
 * Tracepoint for cpu_autohotplug.
 */
TRACE_EVENT(handle_non_critical_trips,

	TP_PROTO(int id, int temp),

	TP_ARGS(id, temp),

	TP_STRUCT__entry(
		__field(int,  id)
		__field(int,  temp)
	),

	TP_fast_assign(
		__entry->id  = id;
		__entry->temp = temp;
	),

	TP_printk("tzid=%d temp=%d",
			__entry->id, __entry->temp)
);
TRACE_EVENT(get_tz_trend,

	TP_PROTO(int id, int trend),

	TP_ARGS(id, trend),

	TP_STRUCT__entry(
		__field(int,  id)
		__field(int,  trend)
	),

	TP_fast_assign(
		__entry->id  = id;
		__entry->trend = trend;
	),

	TP_printk("tzid=%d trend=%d",
			__entry->id, __entry->trend)
);
#endif /* _TRACE_THERMAL_H */

/* This part must be outside protection */
#include <trace/define_trace.h>

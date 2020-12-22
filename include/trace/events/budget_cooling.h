#undef TRACE_SYSTEM
#define TRACE_SYSTEM budget_cooling

#if !defined(_TRACE_BUDGET_COOLING_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_BUDGET_COOLING_H

#include <linux/sched.h>
#include <linux/tracepoint.h>
#include <linux/binfmts.h>



/*
 * Tracepoint for cpu budget cooling.
 */
TRACE_EVENT(cpu_budget_throttle,

	TP_PROTO(unsigned int state,unsigned int c0limit,unsigned int c0nr,unsigned int c1limit,unsigned int c1nr, unsigned int gputhrottle),

	TP_ARGS(state,c0limit,c0nr,c1limit,c1nr,gputhrottle),

	TP_STRUCT__entry(
		__field(unsigned int, state)
		__field(unsigned int, c0limit)
		__field(unsigned int, c0nr)
		__field(unsigned int, c1limit)
		__field(unsigned int, c1nr)        
		__field(unsigned int, gputhrottle)
	),

	TP_fast_assign(
		__entry->state  = state;
		__entry->c0limit  = c0limit;
		__entry->c0nr = c0nr;
		__entry->c1limit  = c1limit;
		__entry->c1nr = c1nr;        
		__entry->gputhrottle = gputhrottle;
	),

	TP_printk("state=%u c0limit=%u c0nr=%u c1limit=%u c1nr=%u gputhrottle=%u",
			__entry->state,__entry->c0limit,__entry->c0nr,__entry->c1limit,__entry->c1nr,__entry->gputhrottle)
);

#endif /* _TRACE_BUDGET_COOLING_H */

/* This part must be outside protection */
#include <trace/define_trace.h>

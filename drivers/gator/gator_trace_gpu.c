/**
 * Copyright (C) ARM Limited 2010-2012. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "gator.h"

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/math64.h>

#ifdef MALI_SUPPORT
#include "linux/mali_linux_trace.h"
#endif
#include "gator_trace_gpu.h"

/*
 * Taken from MALI_PROFILING_EVENT_TYPE_* items in Mali DDK.
 */
#define EVENT_TYPE_SINGLE  0
#define EVENT_TYPE_START   1
#define EVENT_TYPE_STOP    2
#define EVENT_TYPE_SUSPEND 3
#define EVENT_TYPE_RESUME  4


/* Note whether tracepoints have been registered */
static int mali_timeline_trace_registered;
static int mali_job_slots_trace_registered;
static int gpu_trace_registered;

#define GPU_START			1
#define GPU_STOP			2

#define GPU_UNIT_VP			1
#define GPU_UNIT_FP			2
#define GPU_UNIT_CL			3

#define MALI_400     (0x0b07)
#define MALI_T6xx    (0x0056)

#if defined(MALI_SUPPORT) && (MALI_SUPPORT != MALI_T6xx)
#include "gator_events_mali_400.h"

/*
 * Taken from MALI_PROFILING_EVENT_CHANNEL_* in Mali DDK.
 */
enum {
	EVENT_CHANNEL_SOFTWARE = 0,
    EVENT_CHANNEL_VP0 = 1,
    EVENT_CHANNEL_FP0 = 5,
    EVENT_CHANNEL_FP1,
    EVENT_CHANNEL_FP2,
    EVENT_CHANNEL_FP3,
    EVENT_CHANNEL_FP4,
    EVENT_CHANNEL_FP5,
    EVENT_CHANNEL_FP6,
    EVENT_CHANNEL_FP7,
    EVENT_CHANNEL_GPU = 21
};

/**
 * These events are applicable when the type MALI_PROFILING_EVENT_TYPE_SINGLE is used from the GPU channel
 */
enum {
	EVENT_REASON_SINGLE_GPU_NONE              = 0,
	EVENT_REASON_SINGLE_GPU_FREQ_VOLT_CHANGE  = 1,
};


GATOR_DEFINE_PROBE(mali_timeline_event, TP_PROTO(unsigned int event_id, unsigned int d0, unsigned int d1, unsigned int d2, unsigned int d3, unsigned int d4))
{
	unsigned int component, state;

	// do as much work as possible before disabling interrupts
	component = (event_id >> 16) & 0xFF; // component is an 8-bit field
	state = (event_id >> 24) & 0xF;      // state is a 4-bit field

	switch (state) {
	case EVENT_TYPE_START:
		if (component == EVENT_CHANNEL_VP0) {
			/* tgid = d0; pid = d1; */
			marshal_sched_gpu(GPU_START, GPU_UNIT_VP, 0, d0, d1);
		} else if (component >= EVENT_CHANNEL_FP0 && component <= EVENT_CHANNEL_FP7) {
			/* tgid = d0; pid = d1; */
			marshal_sched_gpu(GPU_START, GPU_UNIT_FP, component - EVENT_CHANNEL_FP0, d0, d1);
		}
		break;

	case EVENT_TYPE_STOP:
		if (component == EVENT_CHANNEL_VP0) {
			marshal_sched_gpu(GPU_STOP, GPU_UNIT_VP, 0, 0, 0);
		} else if (component >= EVENT_CHANNEL_FP0 && component <= EVENT_CHANNEL_FP7) {
			marshal_sched_gpu(GPU_STOP, GPU_UNIT_FP, component - EVENT_CHANNEL_FP0, 0, 0);
		}
		break;

	case EVENT_TYPE_SINGLE:
		if (component == EVENT_CHANNEL_GPU) {
			unsigned int reason = (event_id & 0xffff);

			if(reason == EVENT_REASON_SINGLE_GPU_FREQ_VOLT_CHANGE) {
				gator_events_mali_log_dvfs_event(d0, d1);
			}
		}
		break;

	default:
		break;
	}
}
#endif

#if defined(MALI_SUPPORT) && (MALI_SUPPORT == MALI_T6xx)
GATOR_DEFINE_PROBE(mali_job_slots_event, TP_PROTO(unsigned int event_id, unsigned int tgid, unsigned int pid))
{
	unsigned int component, state, type, unit = 0;

	component = (event_id >> 16) & 0xFF; // component is an 8-bit field
	state = (event_id >> 24) & 0xF;      // state is a 4-bit field
	type = (state == EVENT_TYPE_START) ? GPU_START : GPU_STOP;

	switch (component)
	{
	case 0:
		unit = GPU_UNIT_FP;
		break;
	case 1:
		unit = GPU_UNIT_VP;
		break;
	case 2:
		unit = GPU_UNIT_CL;
		break;
	}

	if (unit != 0)
	{
		marshal_sched_gpu(type, unit, 0, tgid, (pid != 0 ? pid : tgid));
	}
}
#endif

GATOR_DEFINE_PROBE(gpu_activity_start, TP_PROTO(int gpu_unit, int gpu_core, struct task_struct *p))
{
	marshal_sched_gpu(GPU_START, gpu_unit, gpu_core, (int)p->tgid, (int)p->pid);
}

GATOR_DEFINE_PROBE(gpu_activity_stop, TP_PROTO(int gpu_unit, int gpu_core))
{
	marshal_sched_gpu(GPU_STOP, gpu_unit, gpu_core, 0, 0);
}

int gator_trace_gpu_start(void)
{
	/*
	 * Returns nonzero for installation failed
	 * Absence of gpu trace points is not an error
	 */

	gpu_trace_registered = mali_timeline_trace_registered = mali_job_slots_trace_registered = 0;

#if defined(MALI_SUPPORT) && (MALI_SUPPORT != MALI_T6xx)
    if (!GATOR_REGISTER_TRACE(mali_timeline_event)) {
    	mali_timeline_trace_registered = 1;
    }
#endif

#if defined(MALI_SUPPORT) && (MALI_SUPPORT == MALI_T6xx)
    if (!GATOR_REGISTER_TRACE(mali_job_slots_event)) {
    	mali_job_slots_trace_registered = 1;
    }
#endif

    if (!mali_timeline_trace_registered) {
        if (GATOR_REGISTER_TRACE(gpu_activity_start)) {
        	return 0;
        }
        if (GATOR_REGISTER_TRACE(gpu_activity_stop)) {
        	GATOR_UNREGISTER_TRACE(gpu_activity_start);
        	return 0;
        }
        gpu_trace_registered = 1;
    }

	return 0;
}

void gator_trace_gpu_stop(void)
{
#if defined(MALI_SUPPORT) && (MALI_SUPPORT != MALI_T6xx)
	if (mali_timeline_trace_registered) {
		GATOR_UNREGISTER_TRACE(mali_timeline_event);
	}
#endif

#if defined(MALI_SUPPORT) && (MALI_SUPPORT == MALI_T6xx)
	if (mali_job_slots_trace_registered) {
		GATOR_UNREGISTER_TRACE(mali_job_slots_event);
	}
#endif

	if (gpu_trace_registered) {
		GATOR_UNREGISTER_TRACE(gpu_activity_stop);
		GATOR_UNREGISTER_TRACE(gpu_activity_start);
	}

	gpu_trace_registered = mali_timeline_trace_registered = mali_job_slots_trace_registered = 0;
}

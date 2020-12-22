/*
 * Copyright (C) 2012-2013 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_sync.h
 *
 */

#ifndef _MALI_SYNC_H_
#define _MALI_SYNC_H_

#ifdef CONFIG_SYNC

#include <linux/seq_file.h>
#include <linux/sync.h>

#define MALI_SYNC_TIMED_FENCE_TIMEOUT 4000 /* 4s */

/*
 * Create a stream object.
 * Built on top of timeline object.
 * Exposed as a file descriptor.
 * Life-time controlled via the file descriptor:
 * - dup to add a ref
 * - close to remove a ref
 */
_mali_osk_errcode_t mali_stream_create(const char * name, int * out_fd);

/*
 * Create a fence in a stream object
 */
struct sync_pt *mali_stream_create_point(int tl_fd);
int mali_stream_create_fence(struct sync_pt *pt);
int mali_stream_create_empty_fence(int tl_fd);

/**
 * Commit an empty timed fence
 *
 * This stops the timer of the empty fence and returns wether or not the fence
 * is still suitable for use.
 *
 * Returns -ETIME if fence is already signalled, in which case it can not be
 * used, or 0 when the timer was stopped and the fence is OK to use.
 */
int mali_sync_timed_commit(struct sync_pt *pt);

/*
 * Validate a fd to be a valid fence
 * No reference is taken.
 *
 * This function is only usable to catch unintentional user errors early,
 * it does not stop malicious code changing the fd after this function returns.
 */
_mali_osk_errcode_t mali_fence_validate(int fd);


/* Returns true if the specified timeline is allocated by Mali */
int mali_sync_timeline_is_ours(struct sync_timeline *timeline);

/* Allocates a timeline for Mali
 *
 * One timeline should be allocated per API context.
 */
struct sync_timeline *mali_sync_timeline_alloc(const char *name);

/* Allocates a sync point within the timeline.
 *
 * The timeline must be the one allocated by mali_sync_timeline_alloc
 *
 * Sync points must be triggered in *exactly* the same order as they are allocated.
 */
struct sync_pt *mali_sync_pt_alloc(struct sync_timeline *parent);

/* Allocates a timed sync point within the timeline.
 *
 * The timeline must be the one allocated by mali_sync_timeline_alloc
 *
 * Sync points must be triggered in *exactly* the same order as they are allocated.
 *
 * Timed sync points should be backed by a proper event before reaching the
 * timeout. If timeout is reached the fence will be signalled with an error (-ETIME).
 */
struct sync_pt *mali_sync_timed_pt_alloc(struct sync_timeline *parent);

/* Signals a particular sync point
 *
 * Sync points must be triggered in *exactly* the same order as they are allocated.
 *
 * If they are signalled in the wrong order then a message will be printed in debug
 * builds and otherwise attempts to signal order sync_pts will be ignored.
 */
void mali_sync_signal_pt(struct sync_pt *pt, int error);

#endif /* CONFIG_SYNC */
#endif /* _MALI_SYNC_H_ */

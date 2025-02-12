/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

#ifndef _MVX_SCHEDULER_H_
#define _MVX_SCHEDULER_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/kref.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include "mvx_lsid.h"

/****************************************************************************
 * Types
 ****************************************************************************/

struct mvx_if_ops;
struct mvx_hwreg;

/**
 * struct mvx_sched - Scheduler class.
 * @dev:	Pointer to device.
 * @if_ops:	Pointer to if module operations.
 * @hwreg:	Pointer to hwreg.
 * @mutex:	Mutex protecting the scheduler.
 * @pending:	List if sessions pending scheduling.
 * @nlsid:	Number of LSID.
 * @lsid:	Array of LSID instances.
 */
struct mvx_sched {
	struct device *dev;
	struct mvx_if_ops *if_ops;
	struct mvx_hwreg *hwreg;
	struct mutex mutex;
	struct list_head pending;
	unsigned int nlsid;
	struct mvx_lsid lsid[MVX_LSID_MAX];
	struct work_struct sched_task;
	struct workqueue_struct *sched_queue;
	bool is_suspend;
};

/**
 * struct mvx_sched_session - Client session class.
 * @isession:	Pointer to if session.
 * @head:	List head used to insert session into scheduler pending list.
 * @lsid:	Pointer to LSID the session is mapped to.
 * @pcb:	LSID pcb.
 *
 * This struct is used to keep track of sessions specific information.
 */
struct mvx_sched_session {
	struct mvx_if_session *isession;
	struct list_head pending;
	struct list_head notify;
	struct mvx_lsid *lsid;
	struct mvx_lsid_pcb pcb;
	bool in_pending;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_sched_construct() - Construct the scheduler object.
 * @sched:	Pointer to scheduler object.
 * @dev:	Pointer to device.
 * @if_ops:	Pointer to if ops.
 * @hwreg:	Pointer to hwreg.
 * @parent:	Pointer to parent debugfs directory entry.
 *
 * Return: 0 on success, else errorr code.
 */
int mvx_sched_construct(struct mvx_sched *sched,
			struct device *dev,
			struct mvx_if_ops *if_ops,
			struct mvx_hwreg *hwreg,
			struct dentry *parent);

/**
 * mvx_sched_destruct() - Destruct the scheduler object.
 * @sched:	Pointer to scheduler object.
 */
void mvx_sched_destruct(struct mvx_sched *sched);

/**
 * mvx_sched_session_construct() - Construct the scheduler session object.
 * @if_ops:	If module operations.
 * @session:	Pointer to session object.
 * @isession:	Pointer to if session.
 *
 * Return: 0 on success, else error code.
 */
int mvx_sched_session_construct(struct mvx_sched_session *session,
				struct mvx_if_session *isession);

/**
 * mvx_sched_session_destruct() - Destruct the scheduler session object.
 * @session:	Pointer to session object.
 *
 * The client must make sure the session is terminated before the destructor
 * is called.
 */
void mvx_sched_session_destruct(struct mvx_sched_session *session);

/**
 * mvx_sched_switch_in() - Switch in a session.
 * @sched:	Pointer to scheduler object.
 * @session:	Pointer to session object.
 *
 * Map a session to a LSID and schedule session for execution. If no LSID
 * is available the session is placed in the pending queue.
 *
 * Return: 0 on success, else error code.
 */
int mvx_sched_switch_in(struct mvx_sched *sched,
			struct mvx_sched_session *session);

/**
 * mvx_sched_send_irq() - Send IRQ to session.
 * @sched:	Pointer to scheduler object.
 * @session:	Pointer to session object.
 *
 * Return: 0 on success, else error code.
 */
int mvx_sched_send_irq(struct mvx_sched *sched,
		       struct mvx_sched_session *session);

/**
 * mvx_sched_flush_mmu() - Flush MMU tables.
 * @sched:	Pointer to scheduler object.
 * @session:	Pointer to session object.
 *
 * Return: 0 on success, else error code.
 */
int mvx_sched_flush_mmu(struct mvx_sched *sched,
			struct mvx_sched_session *session);

/**
 * mvx_sched_handle_irq() - Handle interrupt for a LSID.
 * @sched:	Pointer to scheduler object.
 * @lsid:	LSID number.
 */
void mvx_sched_handle_irq(struct mvx_sched *sched,
			  unsigned int lsid);

/**
 * mvx_sched_terminate() - Terminate a session.
 * @sched:	Pointer to scheduler object.
 * @session:	Pointer to session object.
 */
void mvx_sched_terminate(struct mvx_sched *sched,
			 struct mvx_sched_session *session);

/**
 * mvx_sched_print_debug() - Print debug information.
 * @sched:	Pointer to scheduler object.
 * @session:	Pointer to session object.
 */
void mvx_sched_print_debug(struct mvx_sched *sched,
			   struct mvx_sched_session *session);

void mvx_sched_suspend(struct mvx_sched *sched);

void mvx_sched_resume(struct mvx_sched *sched);

void mvx_sched_wait_session_idle(struct mvx_sched *sched, struct mvx_sched_session *session);

#endif /* _MVX_SCHEDULER_H_ */

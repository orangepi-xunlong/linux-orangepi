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

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include "mvx_if.h"
#include "mvx_hwreg.h"
#include "mvx_mmu.h"
#include "mvx_scheduler.h"
#include "mvx_session.h"
#include "mvx_seq.h"
#include "mvx_pm_runtime.h"
#include "mvx_log_group.h"
#include "mvx_dvfs.h"

/****************************************************************************
 * Static functions
 ****************************************************************************/

static struct mvx_lsid *find_free_lsid(struct mvx_sched *sched)
{
	unsigned int i;

	for (i = 0; i < sched->nlsid; i++)
		if (sched->lsid[i].session == NULL)
			return &sched->lsid[i];

	return NULL;
}

static struct mvx_lsid *find_idle_lsid(struct mvx_sched *sched)
{
	unsigned int i;

	for (i = 0; i < sched->nlsid; i++) {
		bool idle;

		idle = mvx_lsid_idle(&sched->lsid[i]);
		if (idle != false)
			return &sched->lsid[i];
	}

	return NULL;
}

static int map_session(struct mvx_sched *sched,
		       struct mvx_sched_session *session,
		       struct mvx_lsid *lsid)
{
	int ret;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
		      "%p Map LSID. lsid=%u, jobqueue=%08x, corelsid=%08x.",
		      mvx_if_session_to_session(session->isession),
		      lsid->lsid,
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

	ret = mvx_lsid_map(lsid, &session->pcb);
	if (ret != 0)
		return ret;

	session->lsid = lsid;
	lsid->session = session;

	return 0;
}

static void unmap_session(struct mvx_sched *sched,
			  struct mvx_sched_session *session)
{
	struct mvx_lsid *lsid = session->lsid;

	if (lsid == NULL)
		return;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
		      "%p Unmap LSID. lsid=%u, jobqueue=%08x, corelsid=%08x.",
		      mvx_if_session_to_session(session->isession),
		      lsid->lsid,
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

	mvx_lsid_unmap(lsid, &session->pcb);
	session->lsid = NULL;
	lsid->session = NULL;
}

static struct list_head *list_find_node(struct list_head *list,
					struct list_head *node)
{
	struct list_head *i;

	list_for_each(i, list) {
		if (i == node)
			return i;
	}

	return NULL;
}

/**
 * pending list is only updated when sched is locked.
 * a session can only be added once
 *
 * notify_list = []
 * lock_sched
 * for pending in pending_list:
 *      if is_mapped(pending):
 *              jobqueue.add(pending)
 *              pending_list.remove(pending)
 *              continue
 *
 *      l = free_lsid
 *      if l is Nul:
 *              l = idle_lsid
 *              if l is Nul:
 *                      break
 *      if is_mapped(l):
 *              s = session[l]
 *              unmap(s)
 *              notify_list.add(s)
 *
 *      map(pending)
 *      jobqueue.add(pending)
 *      pending_list.remove(pending)
 * unlock_sched
 *
 * for s in notify_list:
 *      session_notify(s)
 *      notify_list.remove(s)
 */
static void sched_task(struct work_struct *ws)
{
	struct mvx_sched *sched =
		container_of(ws, struct mvx_sched, sched_task);
	struct mvx_sched_session *pending;
	struct mvx_sched_session *unmapped;
	struct mvx_sched_session *tmp;
	struct mvx_session *m_session;
	LIST_HEAD(notify_list);
	int ret;

	mvx_pm_runtime_get_sync(sched->dev);
	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0) {
		mvx_pm_runtime_put_sync(sched->dev);
		return;
	}

	/*
	 * Try to map sessions from pending queue while possible.
	 */
	list_for_each_entry_safe(pending, tmp, &sched->pending, pending) {
		struct mvx_lsid *lsid;

		/*
		 * This session is already mapped to LSID.
		 * Just make sure it is scheduled.
		 */
		if (pending->lsid != NULL) {
			m_session = mvx_if_session_to_session(pending->isession);
			if (sched->is_suspend == true && m_session != NULL && m_session->switched_in == false) {
                continue;
			}

			ret = mvx_lsid_jobqueue_add(pending->lsid,
						    pending->isession->ncores);
			if (ret != 0) {
				MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
					      "Cannot add pending session to job queue. csession=%p, mvx_session=%p",
					      pending,
					      mvx_if_session_to_session(
						      pending->isession));
				continue;
			}

			pending->in_pending = false;
			list_del(&pending->pending);
			continue;
		} else if (sched->is_suspend == true) {
            continue;
        }

		/* Find LSID to be used for the pending session. */
		lsid = find_free_lsid(sched);
		if (lsid == NULL)
			lsid = find_idle_lsid(sched);

		if (lsid == NULL)
			break;

		/*
		 * This LSID was mapped to some session. We have to notify
		 * the session about an irq in case there are messages in
		 * a message queue.
		 *
		 * Notifications are done after pending list is processed.
		 */
		if (lsid->session != NULL) {
			struct mvx_sched_session *unmapped = lsid->session;

			unmap_session(sched, unmapped);

			/*
			 * If the reference count is 0, then the session is
			 * about to be removed and should be ignored.
			 */
			ret = kref_get_unless_zero(&unmapped->isession->kref);
			if (ret != 0) {
				if (list_find_node(&notify_list,
						   &unmapped->notify))
					/*
					 * Consider a situation when a session
					 * that was unmapped from LSID and added
					 * notify_list was also present in the
					 * pending_list. It is possible that
					 * such a session will be mapped to the
					 * new LSID, executed by the hardware
					 * and switched to idle state while
					 * this function is still looping
					 * through pending list.
					 *
					 * If it happens, then this session
					 * might be unmapped again in order to
					 * make a room for another pending
					 * session. As a result we will try to
					 * add this session to notify_list
					 * again. This will break notify list
					 * and could lead to crashes or hangs.
					 *
					 * However, it is safe just to skip
					 * adding the session to notify_list if
					 * it is already there, because it will
					 * be processed anyway.
					 */
					kref_put(&unmapped->isession->kref,
						 unmapped->isession->release);
				else
					list_add_tail(&unmapped->notify,
						      &notify_list);
			} else {
				MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
					      "Ref is zero. csession=%p",
					      unmapped);
			}
		}

		ret = map_session(sched, pending, lsid);
		if (ret != 0) {
			MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
				      "Cannot map pending session. csession=%p, mvx_session=%p",
				      pending,
				      mvx_if_session_to_session(
					      pending->isession));
			break;
		}

		ret = mvx_lsid_jobqueue_add(lsid, pending->isession->ncores);
		if (ret != 0) {
			MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
				      "Cannot add pending session to job queue. csession=%p, mvx_session=%p",
				      pending,
				      mvx_if_session_to_session(
					      pending->isession));
			continue;
		}

		pending->in_pending = false;
		list_del(&pending->pending);
	}

	/*
	 * It is important that the scheduler mutex is released before the
	 * callbacks to the if-module are invoked. The if-module may issue
	 * requests to the dev-module (for example switch_in()) that would
	 * otherwise deadlock.
	 */
	mutex_unlock(&sched->mutex);

	list_for_each_entry_safe(unmapped, tmp, &notify_list, notify) {
		struct mvx_if_session *iunmapped = unmapped->isession;

		list_del(&unmapped->notify);

		mutex_lock(iunmapped->mutex);
		sched->if_ops->irq(iunmapped);
		ret = kref_put(&iunmapped->kref, iunmapped->release);
		if (ret == 0)
			mutex_unlock(iunmapped->mutex);
	}

	mvx_pm_runtime_put_sync(sched->dev);
}

static void sched_session_print(struct seq_file *s,
				struct mvx_sched_session *session,
				struct mvx_hwreg *hwreg,
				int ind)
{
	struct mvx_lsid *lsid;

	if (session == NULL)
		return;

	mvx_seq_printf(s, "Client session", ind, "%p\n", session->isession);
	mvx_seq_printf(s, "Dev session", ind, "%p\n", session);
	mvx_seq_printf(s, "MVX session", ind, "%p\n",
		       mvx_if_session_to_session(session->isession));

	lsid = session->lsid;
	if (lsid == NULL)
		return;

	mvx_seq_printf(s, "IRQ host", ind, "%d\n",
		       mvx_hwreg_read_lsid(hwreg, lsid->lsid,
					   MVX_HWREG_IRQHOST));
	mvx_seq_printf(s, "IRQ MVE", ind, "%d\n",
		       mvx_hwreg_read_lsid(hwreg, lsid->lsid,
					   MVX_HWREG_LIRQVE));
}

static int sched_show(struct seq_file *s,
		      void *v)
{
	struct mvx_sched *sched = (struct mvx_sched *)s->private;
	struct mvx_hwreg *hwreg = sched->hwreg;
	struct mvx_sched_session *session;
	int i;
	int ret;

	ret = mvx_pm_runtime_get_sync(hwreg->dev);
	if (ret < 0)
		return 0;

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0) {
		mvx_pm_runtime_put_sync(hwreg->dev);
		return ret;
	}

	mvx_seq_printf(s, "Core LSID", 0, "%08x\n",
		       mvx_hwreg_read(hwreg, MVX_HWREG_CORELSID));
	mvx_seq_printf(s, "Job queue", 0, "%08x\n",
		       mvx_hwreg_read(hwreg, MVX_HWREG_JOBQUEUE));
	seq_puts(s, "\n");

	seq_puts(s, "scheduled:\n");
	for (i = 0; i < sched->nlsid; ++i) {
		mvx_seq_printf(s, "LSID", 1, "%d\n", i);
		session = sched->lsid[i].session;
		sched_session_print(s, session, hwreg, 2);
	}

	seq_puts(s, "pending:\n");
	i = 0;
	list_for_each_entry(session, &sched->pending, pending) {
		char tmp[10];

		scnprintf(tmp, sizeof(tmp), "%d", i++);
		mvx_seq_printf(s, tmp, 1, "\n");
		sched_session_print(s, session, hwreg, 2);
	}

	mutex_unlock(&sched->mutex);
	mvx_pm_runtime_put_sync(hwreg->dev);

	return 0;
}

static int sched_open(struct inode *inode,
		      struct file *file)
{
	return single_open(file, sched_show, inode->i_private);
}

static const struct file_operations sched_fops = {
	.open    = sched_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

int sched_debugfs_init(struct mvx_sched *sched,
		       struct dentry *parent)
{
	struct dentry *dentry;

	dentry = debugfs_create_file("sched", 0400, parent, sched,
				     &sched_fops);
	if (IS_ERR_OR_NULL(dentry))
		return -ENOMEM;

	return 0;
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_sched_construct(struct mvx_sched *sched,
			struct device *dev,
			struct mvx_if_ops *if_ops,
			struct mvx_hwreg *hwreg,
			struct dentry *parent)
{
	unsigned int lsid;
	int ret;

	sched->dev = dev;
	sched->hwreg = hwreg;
	sched->if_ops = if_ops;
	sched->is_suspend = false;
	mutex_init(&sched->mutex);
	INIT_LIST_HEAD(&sched->pending);
	INIT_WORK(&sched->sched_task, sched_task);
	sched->sched_queue = create_singlethread_workqueue("mvx_sched");
	if (!sched->sched_queue) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "Cannot create work queue");
		return -EINVAL;
	}

	sched->nlsid = mvx_hwreg_read(hwreg, MVX_HWREG_NLSID);

	for (lsid = 0; lsid < sched->nlsid; lsid++) {
		ret = mvx_lsid_construct(&sched->lsid[lsid], dev, hwreg, lsid);
		if (ret != 0)
			goto destruct_lsid;
	}

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		ret = sched_debugfs_init(sched, parent);
		if (ret != 0)
			goto destruct_lsid;
	}

	mvx_hwreg_write(hwreg, MVX_HWREG_RESET, 1);
	mvx_hwreg_write(hwreg, MVX_HWREG_CLKFORCE, 0);

	return 0;

destruct_lsid:
	while (lsid-- > 0)
		mvx_lsid_destruct(&sched->lsid[lsid]);

	return ret;
}

void mvx_sched_destruct(struct mvx_sched *sched)
{
	destroy_workqueue(sched->sched_queue);

	while (sched->nlsid-- > 0)
		mvx_lsid_destruct(&sched->lsid[sched->nlsid]);
}

int mvx_sched_session_construct(struct mvx_sched_session *session,
				struct mvx_if_session *isession)
{
	uint32_t disallow;
	uint32_t maxcores;

	session->isession = isession;
	INIT_LIST_HEAD(&session->pending);
	INIT_LIST_HEAD(&session->notify);
	session->lsid = NULL;
	session->in_pending = false;

	memset(&session->pcb, 0, sizeof(session->pcb));

	disallow = (0xffffffff << isession->ncores) & MVE_CTRL_DISALLOW_MASK;
	maxcores = isession->ncores & MVE_CTRL_MAXCORES_MASK;
	session->pcb.ctrl = (disallow << MVE_CTRL_DISALLOW_SHIFT) |
			    (maxcores << MVE_CTRL_MAXCORES_SHIFT);

	session->pcb.mmu_ctrl = isession->l0_pte;
	session->pcb.nprot = isession->securevideo == false;

	return 0;
}

void mvx_sched_session_destruct(struct mvx_sched_session *session)
{}

int mvx_sched_switch_in(struct mvx_sched *sched,
			struct mvx_sched_session *session)
{
	int ret;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
		      "%p Switch in session. jobqueue=%08x, coreslid=%08x.",
		      mvx_if_session_to_session(session->isession),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0)
		return ret;

	if (session->in_pending) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_VERBOSE,
			      "Session is already in pending state.");
		goto unlock_mutex;
	}

	session->in_pending = true;
	list_add_tail(&session->pending, &sched->pending);
	queue_work(sched->sched_queue, &sched->sched_task);

unlock_mutex:
	mutex_unlock(&sched->mutex);
	return 0;
}

int mvx_sched_send_irq(struct mvx_sched *sched,
		       struct mvx_sched_session *session)
{
	mutex_lock(&sched->mutex);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_DEBUG,
		      "%p Send irq. lsid=%d, jobqueue=%08x, corelsid=%08x.",
		      mvx_if_session_to_session(session->isession),
		      session->lsid == NULL ? -1 : session->lsid->lsid,
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

	if (session->lsid == NULL)
		session->pcb.irqhost = 1;
	else
		mvx_lsid_send_irq(session->lsid);

	mutex_unlock(&sched->mutex);

	return 0;
}

int mvx_sched_flush_mmu(struct mvx_sched *sched,
			struct mvx_sched_session *session)
{
	mutex_lock(&sched->mutex);

	if (session->lsid != NULL)
		mvx_lsid_flush_mmu(session->lsid);

	mutex_unlock(&sched->mutex);

	return 0;
}

static void print_session(struct mvx_sched *sched,
			  struct mvx_sched_session *session,
			  struct mvx_session *s)
{
	int lsid = -1;
	uint32_t irqve = 0;
	uint32_t irqhost = 0;

	if (session != NULL && session->lsid != NULL) {
		struct mvx_hwreg *hwreg = sched->hwreg;

		lsid = session->lsid->lsid;
		irqve = mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_LIRQVE);
		irqhost = mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_IRQHOST);
	}

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
		      "%p    session=%p, lsid=%d, irqve=%08x, irqhost=%08x",
		      s, mvx_if_session_to_session(session->isession), lsid,
		      irqve, irqhost);
}

void mvx_sched_wait_session_idle(struct mvx_sched *sched,
        struct mvx_sched_session *session)
{
    bool is_idle = 0;
    int wait_count = 20;

    mutex_lock(&sched->mutex);

    if (session->lsid != NULL) {
        is_idle = mvx_lsid_idle(session->lsid);
        if (is_idle == 0) {
            do {
                mutex_unlock(&sched->mutex);
                msleep(50);
                mutex_lock(&sched->mutex);
                if (session->lsid != NULL) {
                    is_idle = mvx_lsid_idle(session->lsid);
                } else {
                    is_idle = 1;
                }
            } while(wait_count-- && is_idle != 1);
        }

        if (is_idle == 0) {
            mvx_lsid_terminate(session->lsid);
        }
    }
    mutex_unlock(&sched->mutex);
}

void mvx_sched_print_debug(struct mvx_sched *sched,
			   struct mvx_sched_session *session)
{
	struct mvx_hwreg *hwreg = sched->hwreg;
	struct mvx_sched_session *pending;
	struct mvx_sched_session *tmp;
	struct mvx_session *s = mvx_if_session_to_session(session->isession);
	unsigned int i;
	int ret;

	mvx_pm_runtime_get_sync(sched->dev);

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0) {
		mvx_pm_runtime_put_sync(sched->dev);
		return;
	}

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING, "%p Current session:", s);
	print_session(sched, session, s);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING, "%p Pending queue:", s);
	list_for_each_entry_safe(pending, tmp, &sched->pending, pending) {
		print_session(sched, pending, s);
	}

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING, "%p Print register:", s);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
		      "%p     jobqueue=%08x, corelsid=%08x, irqve=%08x,HARDWARE_ID=0x%08x,ENABLE=0x%08x,NCORES=0x%08x,NLSID=0x%08x,CLKFORCE=0x%08x,PROTCTRL=0x%08x,RESET=0x%08x,FUSE=0x%08x",
		      s,
		      mvx_hwreg_read(hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(hwreg, MVX_HWREG_CORELSID),
		      mvx_hwreg_read(hwreg, MVX_HWREG_IRQVE),
		      mvx_hwreg_read(hwreg, MVX_HWREG_HARDWARE_ID),
		      mvx_hwreg_read(hwreg, MVX_HWREG_ENABLE),
		      mvx_hwreg_read(hwreg, MVX_HWREG_NCORES),
		      mvx_hwreg_read(hwreg, MVX_HWREG_NLSID),
		      mvx_hwreg_read(hwreg, MVX_HWREG_CLKFORCE),
		      mvx_hwreg_read(hwreg, MVX_HWREG_PROTCTRL),
		      mvx_hwreg_read(hwreg, MVX_HWREG_RESET),
		      mvx_hwreg_read(hwreg, MVX_HWREG_FUSE));

	for (i = 0; i < sched->nlsid; i++) {
		struct mvx_sched_session *ss = sched->lsid[i].session;
		struct mvx_session *ls = NULL;

		if (ss != NULL)
			ls = mvx_if_session_to_session(ss->isession);

		MVX_LOG_PRINT(
			&mvx_log_dev, MVX_LOG_WARNING,
			"%p     lsid=%u, session=%p, irqve=%08x, irqhost=%08x,CTRL=0x%08x,MMU_CTRL=0x%08x,NPROT=0x%08x,ALLOC=0x%08x,FLUSH_ALL=0x%08x,SCHED=0x%08x,TERMINATE=0x%08x,INTSIG=0x%08x,STREAMID=0x%08x",
			s, i, ls,
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_LIRQVE),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_IRQHOST),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_CTRL),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_MMU_CTRL),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_NPROT),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_ALLOC),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_FLUSH_ALL),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_SCHED),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_TERMINATE),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_INTSIG),
			mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_STREAMID));
	}

	mutex_unlock(&sched->mutex);

	mvx_pm_runtime_put_sync(sched->dev);
}

void mvx_sched_handle_irq(struct mvx_sched *sched,
			  unsigned int lsid)
{
	struct mvx_sched_session *session;
	struct mvx_if_session *isession = NULL;
	int ret;

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0)
		return;

	/*
	 * If a session has been terminated/unmapped just before the IRQ bottom
	 * handler has been executed, then the session pointer will be NULL or
	 * may even point at a different session. This is an unharmful
	 * situation.
	 *
	 * If the reference count is 0, then the session is about to be removed
	 * and should be ignored.
	 */
	session = sched->lsid[lsid].session;
	if (session != NULL) {
		ret = kref_get_unless_zero(&session->isession->kref);
		if (ret != 0)
			isession = session->isession;
	}

	/*
	 * It is important that the scheduler mutex is released before the
	 * callbacks to the if-module are invoked. The if-module may issue
	 * requests to the dev-module (for example switch_in()) that would
	 * otherwise deadlock.
	 */
	mutex_unlock(&sched->mutex);

	/* Inform if-session that an IRQ was received. */
	if (isession != NULL) {
		mutex_lock(isession->mutex);
		sched->if_ops->irq(isession);
		ret = kref_put(&isession->kref, isession->release);

		if (ret == 0)
			mutex_unlock(isession->mutex);
	}

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0)
		return;

	if (!list_empty(&sched->pending)) {
		queue_work(sched->sched_queue, &sched->sched_task);
	}
	mutex_unlock(&sched->mutex);
}

void mvx_sched_terminate(struct mvx_sched *sched,
			 struct mvx_sched_session *session)
{
	struct list_head *head;
	struct list_head *tmp;

	mutex_lock(&sched->mutex);

	if (session->lsid != NULL) {
		mvx_lsid_jobqueue_remove(session->lsid);
		mvx_lsid_terminate(session->lsid);
		unmap_session(sched, session);
	}

	list_for_each_safe(head, tmp, &sched->pending) {
		if (head == &session->pending) {
			list_del(head);
			break;
		}
	}

	mutex_unlock(&sched->mutex);
}

static void suspend_session(struct mvx_sched *sched,
        struct mvx_sched_session *session)
{
    struct mvx_lsid *lsid = session->lsid;

    if (lsid == NULL)
        return;

    MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
            "%p suspend_session LSID. lsid=%u, jobqueue=%08x, corelsid=%08x.",
            mvx_if_session_to_session(session->isession),
            lsid->lsid,
            mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
            mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

    mvx_lsid_unmap(lsid, &session->pcb);
}

static int resume_session(struct mvx_sched *sched,
        struct mvx_sched_session *session,
        struct mvx_lsid *lsid)
{
    int ret;

    MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
            "%p resume_session LSID. lsid=%u, jobqueue=%08x, corelsid=%08x.",
            mvx_if_session_to_session(session->isession),
            lsid->lsid,
            mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
            mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

    ret = mvx_lsid_map(lsid, &session->pcb);
    if (ret != 0)
        return ret;

    return 0;
}

static void switch_out_session(struct mvx_sched *sched, unsigned int lsid_id)
{
    int i;
    int ret;
    struct mvx_session *session = NULL;
    struct mvx_if_session *isession = NULL;
    struct mvx_sched_session *sched_session = sched->lsid[lsid_id].session;
    int wait_count = 20;

    if (sched_session != NULL) {
        ret = kref_get_unless_zero(&sched_session->isession->kref);
        if (ret != 0)
            isession = sched_session->isession;
    }

    mutex_unlock(&sched->mutex);

    if (isession != NULL) {
        session = mvx_if_session_to_session(sched_session->isession);
        mutex_lock(isession->mutex);
        if (session != NULL) {
            session->is_suspend = true;
        }
        ret = kref_put(&isession->kref, isession->release);

        if (ret == 0)
            mutex_unlock(isession->mutex);
    }

    mutex_lock(&sched->mutex);
    for (i = 0; i < wait_count; i++) {
        if (sched->lsid[lsid_id].session != NULL && session != NULL && session->switched_in == false) {
            MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                    "%p finish switch_out session LSID. lsid=%u.", session, sched_session->lsid->lsid);
            break;
        }
        mutex_unlock(&sched->mutex);
        msleep(10);

        mutex_lock(&sched->mutex);

        if (sched->lsid[lsid_id].session != NULL && session != NULL) {
            MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
                    "%p wait switch_out session LSID. lsid=%u. loop=%d, switch_in=%d", session, sched_session->lsid->lsid, i, session->switched_in);
        }
    }
}

static int switch_in_session(struct mvx_sched_session *sched_session)
{
    int ret;
    struct mvx_session *session;
    session = mvx_if_session_to_session(sched_session->isession);
    if (session == NULL) {
        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING, "session is null when switch in.");
        return -EINVAL;
    }

    session->is_suspend = false;
    if (session->fw.msg_pending > 0) {
        session->idle_count = 0;

        if (session->switched_in != false)
            return 0;

        MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO, "switch_in_session.");

        ret = session->client_ops->switch_in(session->csession);
        if (ret != 0) {
            MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING, "Failed to switch in session.");
            session->error = ret;
            wake_up(&session->waitq);
            session->event(session, MVX_SESSION_EVENT_ERROR, (void *)session->error);
            return ret;
        }

        session->switched_in = true;
    }
    return 0;
}

void mvx_sched_suspend(struct mvx_sched *sched)
{
    unsigned int i;

    mutex_lock(&sched->mutex);
    sched->is_suspend = true;
    mvx_dvfs_suspend_session();

    for (i = 0; i < sched->nlsid; i++) {
        if (sched->lsid[i].session != NULL) {
            switch_out_session(sched, i);
            if (sched->lsid[i].session != NULL) {
                mvx_lsid_terminate(sched->lsid[i].session->lsid);
                suspend_session(sched, sched->lsid[i].session);
            }
        }
    }

    mutex_unlock(&sched->mutex);
}

void mvx_sched_resume(struct mvx_sched *sched)
{
    unsigned int i;

    mutex_lock(&sched->mutex);

    for (i = 0; i < sched->nlsid; i++) {
        if (sched->lsid[i].session != NULL) {
            resume_session(sched, sched->lsid[i].session, &(sched->lsid[i]));
            switch_in_session(sched->lsid[i].session);
        }
    }
    sched->is_suspend = false;
    mvx_dvfs_resume_session();
    mutex_unlock(&sched->mutex);
}

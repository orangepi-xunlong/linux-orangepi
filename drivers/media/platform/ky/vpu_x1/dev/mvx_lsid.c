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

#include <linux/device.h>
#include <linux/of.h>
#include "mvx_if.h"
#include "mvx_hwreg.h"
#include "mvx_lsid.h"
#include "mvx_log_group.h"
#include "mvx_session.h"

/****************************************************************************
 * Private functions
 ****************************************************************************/

static bool is_alloc(struct mvx_lsid *lsid)
{
	uint32_t alloc;

	alloc = mvx_hwreg_read_lsid(lsid->hwreg, lsid->lsid,
				    MVX_HWREG_ALLOC);

	return alloc != MVE_ALLOC_FREE;
}

static uint32_t get_core_lsid(uint32_t reg,
			      unsigned int core)
{
	return (reg >> (MVE_CORELSID_LSID_BITS * core)) &
	       MVX_CORELSID_LSID_MASK;
}

static uint32_t get_jobqueue_job(uint32_t reg,
				 unsigned int nr)
{
	return (reg >> (MVE_JOBQUEUE_JOB_BITS * nr)) & MVE_JOBQUEUE_JOB_MASK;
}

static uint32_t set_jobqueue_job(uint32_t reg,
				 unsigned int nr,
				 uint32_t job)
{
	reg &= ~(MVE_JOBQUEUE_JOB_MASK << (nr * MVE_JOBQUEUE_JOB_BITS));
	reg |= job << (MVE_JOBQUEUE_JOB_BITS * nr);
	return reg;
}

static uint32_t get_jobqueue_lsid(uint32_t reg,
				  unsigned int nr)
{
	return (reg >> (MVE_JOBQUEUE_JOB_BITS * nr + MVE_JOBQUEUE_LSID_SHIFT)) &
	       MVE_JOBQUEUE_LSID_MASK;
}

static uint32_t set_lsid_ncores(uint32_t reg,
				unsigned int nr,
				unsigned int lsid,
				unsigned int ncores)
{
	reg &= ~(MVE_JOBQUEUE_JOB_MASK << (nr * MVE_JOBQUEUE_JOB_BITS));
	reg |= ((lsid << MVE_JOBQUEUE_LSID_SHIFT) |
		((ncores - 1) << MVE_JOBQUEUE_NCORES_SHIFT)) <<
	       (nr * MVE_JOBQUEUE_JOB_BITS);

	return reg;
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_lsid_construct(struct mvx_lsid *lsid,
		       struct device *dev,
		       struct mvx_hwreg *hwreg,
		       unsigned int id)
{
	lsid->dev = dev;
	lsid->hwreg = hwreg;
	lsid->session = NULL;
	lsid->lsid = id;

	return 0;
}

void mvx_lsid_destruct(struct mvx_lsid *lsid)
{}

int mvx_lsid_map(struct mvx_lsid *lsid,
		 struct mvx_lsid_pcb *pcb)
{
	struct mvx_hwreg *hwreg = lsid->hwreg;
	uint32_t alloc;
	uint32_t busattr[4];
	int ret;

	/* Check that the LSID is not already allocated. */
	if (is_alloc(lsid)) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "Failed to map session to LSID. LSID already allocated. lsid=%u.",
			      lsid->lsid);
		return -EFAULT;
	}

	/* Allocate LSID. */
	alloc = pcb->nprot == 0 ? MVE_ALLOC_PROTECTED : MVE_ALLOC_NON_PROTECTED;
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_ALLOC, alloc);

	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_TERMINATE, 1);
	do {
		ret = mvx_hwreg_read_lsid(hwreg, lsid->lsid,
					  MVX_HWREG_TERMINATE);
	} while (ret != 0);

	/* Configure number of cores to use and which to cores to disable. */
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_CTRL,
			     pcb->ctrl);

	/* Configure MMU L0 entry and flush MMU tables. */
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_MMU_CTRL,
			     pcb->mmu_ctrl);
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_FLUSH_ALL, 0);

	if (of_property_read_u32_array(lsid->dev->of_node, "busattr", busattr,
				       ARRAY_SIZE(busattr))) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_VERBOSE,
			      "busattr in of_node is not available.");

		/* We apply default values in this case. */
		busattr[0] = 0;
		busattr[1] = 0;
		busattr[2] = 0x33;
		busattr[3] = 0x33;
	} else {
		int i;

		for (i = 0; i < ARRAY_SIZE(busattr); i++)
			MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_VERBOSE,
				      "busattr[%d] = 0x%x.", i,
				      busattr[i]);
	}

	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_BUSATTR_0,
			     busattr[0]);
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_BUSATTR_1,
			     busattr[1]);
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_BUSATTR_2,
			     busattr[2]);
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_BUSATTR_3,
			     busattr[3]);

	/* Restore interrupt registers. */
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_LIRQVE, 0);
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_IRQHOST,
			     pcb->irqhost);

	/*
	 * Make sure all register writes have completed before scheduling is
	 * enabled.
	 */
	wmb();

	/* Enable scheduling. */
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_SCHED, 1);

	return 0;
}

void mvx_lsid_unmap(struct mvx_lsid *lsid,
		    struct mvx_lsid_pcb *pcb)
{
	struct mvx_hwreg *hwreg = lsid->hwreg;

	if (!is_alloc(lsid)) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "LSID was not allocated. lsid=%u.",
			      lsid->lsid);
		return;
	}

	/* Disable scheduling. */
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_SCHED, 0);

	/* Store registers in process control block. */
	pcb->ctrl = mvx_hwreg_read_lsid(hwreg, lsid->lsid, MVX_HWREG_CTRL);
	pcb->mmu_ctrl = mvx_hwreg_read_lsid(hwreg, lsid->lsid,
					    MVX_HWREG_MMU_CTRL);
	pcb->irqhost = mvx_hwreg_read_lsid(hwreg, lsid->lsid,
					   MVX_HWREG_IRQHOST);
	pcb->nprot = mvx_hwreg_read_lsid(hwreg, lsid->lsid, MVX_HWREG_NPROT);

	/* Deallocate LSID. */
	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_ALLOC,
			     MVE_ALLOC_FREE);
}

int mvx_lsid_jobqueue_add(struct mvx_lsid *lsid,
			  unsigned int ncores)
{
	struct mvx_hwreg *hwreg = lsid->hwreg;
	uint32_t jobqueue;
	int i;

	/* Disable scheduling. */
	mvx_hwreg_write(hwreg, MVX_HWREG_ENABLE, 0);

	jobqueue = mvx_hwreg_read(hwreg, MVX_HWREG_JOBQUEUE);

	/* Search if the LSID is already in the job queue. */
	for (i = 0; i < MVE_JOBQUEUE_NJOBS; i++)
		if (get_jobqueue_lsid(jobqueue, i) == lsid->lsid)
			goto jobqueue_enable;

	/* Search for a free slot in the job queue. */
	for (i = 0; i < MVE_JOBQUEUE_NJOBS; i++)
		if (get_jobqueue_lsid(jobqueue, i) ==
		    MVE_JOBQUEUE_JOB_INVALID) {
			jobqueue = set_lsid_ncores(jobqueue, i, lsid->lsid,
						   ncores);
			mvx_hwreg_write(hwreg, MVX_HWREG_JOBQUEUE, jobqueue);
			break;
		}

jobqueue_enable:
	/* Reenable scheduling. */
	mvx_hwreg_write(hwreg, MVX_HWREG_ENABLE, 1);

	return i < MVE_JOBQUEUE_NJOBS ? 0 : -EAGAIN;
}

void mvx_lsid_send_irq(struct mvx_lsid *lsid)
{
	struct mvx_hwreg *hwreg = lsid->hwreg;

	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_IRQHOST, 1);
}

void mvx_lsid_flush_mmu(struct mvx_lsid *lsid)
{
	struct mvx_hwreg *hwreg = lsid->hwreg;

	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_FLUSH_ALL, 0);
}

void mvx_lsid_terminate(struct mvx_lsid *lsid)
{
	struct mvx_hwreg *hwreg = lsid->hwreg;
	uint32_t ret;

	mvx_hwreg_write_lsid(hwreg, lsid->lsid, MVX_HWREG_TERMINATE, 1);

	do {
		ret = mvx_hwreg_read_lsid(hwreg, lsid->lsid,
					  MVX_HWREG_TERMINATE);
	} while (ret != 0);
}

void mvx_lsid_jobqueue_remove(struct mvx_lsid *lsid)
{
	struct mvx_hwreg *hwreg = lsid->hwreg;
	uint32_t jobqueue;
	int i;
	int j;
	uint32_t ncores = mvx_hwreg_read(hwreg, MVX_HWREG_NCORES);

	/* Disable scheduling. */
	mvx_hwreg_write(hwreg, MVX_HWREG_ENABLE, 0);

	jobqueue = mvx_hwreg_read(hwreg, MVX_HWREG_JOBQUEUE);

	/* Copy job entries that do not match the LSID to be removed. */
	for (i = 0, j = 0; i < MVE_JOBQUEUE_NJOBS; i++)
		if (get_jobqueue_lsid(jobqueue, i) != lsid->lsid)
			jobqueue = set_jobqueue_job(
				jobqueue, j++, get_jobqueue_job(jobqueue, i));

	/* Blank out remaining job entries. */
	for (; j < MVE_JOBQUEUE_NJOBS; j++)
		jobqueue = set_lsid_ncores(jobqueue, j,
					   MVE_JOBQUEUE_JOB_INVALID, ncores);

	mvx_hwreg_write(hwreg, MVX_HWREG_JOBQUEUE, jobqueue);

	/* Reenable scheduling. */
	mvx_hwreg_write(hwreg, MVX_HWREG_ENABLE, 1);
}

bool mvx_lsid_idle(struct mvx_lsid *lsid)
{
	struct mvx_hwreg *hwreg = lsid->hwreg;
	uint32_t jobqueue;
	uint32_t corelsid;
	uint32_t ncores;
	uint32_t i;

	jobqueue = mvx_hwreg_read(hwreg, MVX_HWREG_JOBQUEUE);
	corelsid = mvx_hwreg_read(hwreg, MVX_HWREG_CORELSID);
	ncores = mvx_hwreg_read(hwreg, MVX_HWREG_NCORES);

	/* Check if LSID is found in job queue. */
	for (i = 0; i < MVE_JOBQUEUE_NJOBS; i++)
		if (get_jobqueue_lsid(jobqueue, i) == lsid->lsid)
			return false;

	/* Check if LSID is found in core lsid. */
	for (i = 0; i < ncores; i++)
		if (get_core_lsid(corelsid, i) == lsid->lsid)
			return false;

	return true;
}

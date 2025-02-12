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

#ifndef _MVX_LSID_H_
#define _MVX_LSID_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/types.h>

/****************************************************************************
 * Defines
 ****************************************************************************/

#define MVX_LSID_MAX    4

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;
struct mvx_hwreg;
struct mvx_sched_session;

/**
 * struct mvx_lsid_pcb - LSID process control block.
 *
 * This structure is used to store the register map when a session is unmapped
 * from a LSID, so it can be restored again when the session is remapped.
 */
struct mvx_lsid_pcb {
	uint32_t ctrl;
	uint32_t mmu_ctrl;
	uint32_t irqhost;
	uint32_t nprot;
};

/**
 * struct mvx_lsid - LSID class.
 */
struct mvx_lsid {
	struct device *dev;
	struct mvx_hwreg *hwreg;
	struct mvx_sched_session *session;
	unsigned int lsid;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_lsid_construct() - Construct the LSID object.
 * @lsid:	Pointer to LSID object.
 * @dev:	Pointer to device.
 * @hwreg:	Pointer to hwreg object.
 * @id:		LSID number.
 *
 * Return: 0 on success, else error code.
 */
int mvx_lsid_construct(struct mvx_lsid *lsid,
		       struct device *dev,
		       struct mvx_hwreg *hwreg,
		       unsigned int id);

/**
 * mvx_lsid_destruct() - Destruct the LSID object.
 * @lsid:	Pointer to LSID object.
 */
void mvx_lsid_destruct(struct mvx_lsid *lsid);

/**
 * mvx_lsid_map() - Map a session to this LSID.
 * @lsid:	Pointer to LSID object.
 * @pcb:	Process control block to be restored.
 *
 * Return: 0 on success, else error code.
 */
int mvx_lsid_map(struct mvx_lsid *lsid,
		 struct mvx_lsid_pcb *pcb);

/**
 * mvx_lsid_unmap() - Unmap session from LSID.
 * @lsid:	Pointer to LSID object.
 * @pcb:	Process control block where the registers are stored.
 *
 * A LSID must not be unmapped if it is present in the job queue or core LSID.
 * It is the responsibility of the scheduler to guarantee that the LSID is idle
 * before it is unmapped.
 */
void mvx_lsid_unmap(struct mvx_lsid *lsid,
		    struct mvx_lsid_pcb *pcb);

/**
 * mvx_lsid_jobqueue_add() - Add LSID to job queue.
 * @lsid:	Pointer to LSID object.
 * @ncores:	Number of cores to request.
 *
 * Return: 0 on success, else error code.
 */
int mvx_lsid_jobqueue_add(struct mvx_lsid *lsid,
			  unsigned int ncores);

/**
 * mvx_lsid_send_irq() - Send IRQ to firmware.
 * @lsid:	Pointer to LSID object.
 */
void mvx_lsid_send_irq(struct mvx_lsid *lsid);

/**
 * mvx_lsid_flush_mmu() - Flush MMU tables.
 * @lsid:	Pointer to LSID object.
 */
void mvx_lsid_flush_mmu(struct mvx_lsid *lsid);

/**
 * mvx_lsid_terminate() - Terminate the LSID.
 * @lsid:	Pointer to LSID object.
 */
void mvx_lsid_terminate(struct mvx_lsid *lsid);

/**
 * mvx_lsid_jobqueue_remove() - Remove LSID from job queue.
 * @lsid:	Pointer to LSID object.
 */
void mvx_lsid_jobqueue_remove(struct mvx_lsid *lsid);

/**
 * mvx_lsid_idle() - Check if LSID is idle.
 * @lsid:	Pointer to LSID object.
 *
 * Return: true if LSID is idle, else false.
 */
bool mvx_lsid_idle(struct mvx_lsid *lsid);

#endif /* _MVX_LSID_H_ */

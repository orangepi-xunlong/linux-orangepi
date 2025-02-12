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

#ifndef _MVX_HW_REG_
#define _MVX_HW_REG_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/ioport.h>
#include "mvx_if.h"
#include "mvx_lsid.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

#define MVX_HWREG_FUSE_DISABLE_AFBC     (1 << 0)
#define MVX_HWREG_FUSE_DISABLE_REAL     (1 << 1)
#define MVX_HWREG_FUSE_DISABLE_VPX      (1 << 2)
#define MVX_HWREG_FUSE_DISABLE_HEVC     (1 << 3)

#define MVE_JOBQUEUE_JOB_BITS           8
#define MVE_JOBQUEUE_JOB_MASK           ((1 << MVE_JOBQUEUE_JOB_BITS) - 1)
#define MVE_JOBQUEUE_JOB_INVALID        0xf
#define MVE_JOBQUEUE_NJOBS              4
#define MVE_JOBQUEUE_LSID_SHIFT         0
#define MVE_JOBQUEUE_LSID_BITS          4
#define MVE_JOBQUEUE_LSID_MASK          ((1 << MVE_JOBQUEUE_LSID_BITS) - 1)
#define MVE_JOBQUEUE_NCORES_SHIFT       4
#define MVE_JOBQUEUE_NCORES_BITS        4

#define MVE_CORELSID_LSID_BITS          4
#define MVX_CORELSID_LSID_MASK          ((1 << MVE_CORELSID_LSID_BITS) - 1)

#define MVE_CTRL_DISALLOW_SHIFT         0
#define MVE_CTRL_DISALLOW_BITS          8
#define MVE_CTRL_DISALLOW_MASK          ((1 << MVE_CTRL_DISALLOW_BITS) - 1)
#define MVE_CTRL_MAXCORES_SHIFT         8
#define MVE_CTRL_MAXCORES_BITS          4
#define MVE_CTRL_MAXCORES_MASK          ((1 << MVE_CTRL_MAXCORES_BITS) - 1)

#define MVE_ALLOC_FREE                  0
#define MVE_ALLOC_NON_PROTECTED         1
#define MVE_ALLOC_PROTECTED             2

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;

/**
 * enum mvx_hwreg_what - Hardware registers that can be read or written.
 */
enum mvx_hwreg_what {
	MVX_HWREG_HARDWARE_ID,
	MVX_HWREG_ENABLE,
	MVX_HWREG_NCORES,
	MVX_HWREG_NLSID,
	MVX_HWREG_CORELSID,
	MVX_HWREG_JOBQUEUE,
	MVX_HWREG_IRQVE,
	MVX_HWREG_CLKFORCE,
	MVX_HWREG_FUSE,
	MVX_HWREG_PROTCTRL,
	MVX_HWREG_RESET,
	MVX_HWREG_CONFIG,
	MVX_HWREG_WHAT_MAX
};

/**
 * enum mvx_hwreg_lsid - Hardware registers per LSID.
 */
enum mvx_hwreg_lsid {
	MVX_HWREG_CTRL,
	MVX_HWREG_MMU_CTRL,
	MVX_HWREG_NPROT,
	MVX_HWREG_ALLOC,
	MVX_HWREG_FLUSH_ALL,
	MVX_HWREG_SCHED,
	MVX_HWREG_TERMINATE,
	MVX_HWREG_LIRQVE,
	MVX_HWREG_IRQHOST,
	MVX_HWREG_INTSIG,
	MVX_HWREG_STREAMID,
	MVX_HWREG_BUSATTR_0,
	MVX_HWREG_BUSATTR_1,
	MVX_HWREG_BUSATTR_2,
	MVX_HWREG_BUSATTR_3,
	MVX_HWREG_LSID_MAX
};

struct mvx_hwreg;

/**
 * struct mvx_lsid_hwreg - Helper struct used for debugfs reading of lsid
 *			   dependent registers.
 */
struct mvx_lsid_hwreg {
	struct mvx_hwreg *hwreg;
	unsigned int lsid;
};

/**
 * struct mvx_hwreg - Context class for the hardware register interface.
 */
struct mvx_hwreg {
	struct device *dev;
	struct resource *res;
	void *registers;
	struct mvx_lsid_hwreg lsid_hwreg[MVX_LSID_MAX];
	struct {
		void (*get_formats)(enum mvx_direction direction,
				    uint64_t *formats);
	} ops;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_hwreg_construct() - Construct the hardware register object.
 * @hwreg:	Pointer to hwreg object.
 * @dev:	Pointer to device struct.
 * @res:	Memory resource.
 * @parent:	Parent debugfs directory entry.
 *
 * Return: 0 on success, else error code.
 */
int mvx_hwreg_construct(struct mvx_hwreg *hwreg,
			struct device *dev,
			struct resource *res,
			struct dentry *parent);

/**
 * mvx_hwreg_destruct() - Destroy the hardware register object.
 * @hwreg:	Pointer to hwreg object.
 */
void mvx_hwreg_destruct(struct mvx_hwreg *hwreg);

/**
 * mvx_hwreg_read() - Read hardware register.
 * @hwreg:	Pointer to hwreg object.
 * @what:	Which register to read.
 *
 * Return: Value of register.
 */
uint32_t mvx_hwreg_read(struct mvx_hwreg *hwreg,
			enum mvx_hwreg_what what);

/**
 * mvx_hwreg_write() - Write hardware register.
 * @hwreg:	Pointer to hwreg object.
 * @what:	Which register to write.
 * @value:	Value to write.
 */
void mvx_hwreg_write(struct mvx_hwreg *hwreg,
		     enum mvx_hwreg_what what,
		     uint32_t value);

/**
 * mvx_hwreg_read_lsid() - Read LSID hardware register.
 * @hwreg:	Pointer to hwreg object.
 * @lsid:	LSID register index.
 * @what:	Which register to read.
 *
 * Return: Value of register.
 */
uint32_t mvx_hwreg_read_lsid(struct mvx_hwreg *hwreg,
			     unsigned int lsid,
			     enum mvx_hwreg_lsid what);

/**
 * mvx_hwreg_write_lsid() - Write LSID hardware register.
 * @hwreg:	Pointer to hwreg object.
 * @lsid:	LSID register index.
 * @what:	Which register to write.
 * @value:	Value to write.
 */
void mvx_hwreg_write_lsid(struct mvx_hwreg *hwreg,
			  unsigned int lsid,
			  enum mvx_hwreg_lsid what,
			  uint32_t value);

/**
 * mvx_hwreg_get_hw_id() - Get hardware id.
 * @hwreg:	Pointer to hwreg object.
 * @revision:	Hardware revision.
 * @patch:	Hardware patch revision.
 *
 * Return: Hardware id.
 */
enum mvx_hw_id mvx_hwreg_get_hw_id(struct mvx_hwreg *hwreg,
				   uint32_t *revision,
				   uint32_t *patch);

#endif /* _MVX_HW_REG_ */

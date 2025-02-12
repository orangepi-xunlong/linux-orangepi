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

#ifndef _MVX_V4L2_FOPS_H_
#define _MVX_V4L2_FOPS_H_

/*
 * Callbacks for struct v4l2_file_operations.
 *
 * Prototypes declared bellow represent callbacks required by v4l2 framework.
 * They are needed to implement certain syscalls.
 */

/**
 * mvx_v4l2_open() - Callback needed to implement the open() syscall.
 */
int mvx_v4l2_open(struct file *file);

/**
 * mvx_v4l2_release() - Callback needed to implement the release() syscall.
 */
int mvx_v4l2_release(struct file *file);

/**
 * mvx_v4l2_poll() - Callback needed to implement the poll() syscall.
 */
unsigned int mvx_v4l2_poll(struct file *file,
			   struct poll_table_struct *wait);

/**
 * mvx_v4l2_mmap() - Callback needed to implement the mmap() syscall.
 */
int mvx_v4l2_mmap(struct file *file,
		  struct vm_area_struct *vma);

#endif /* _MVX_V4L2_FOPS_H_ */

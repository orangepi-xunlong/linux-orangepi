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

#ifndef _MVX_EXT_H_
#define _MVX_EXT_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/mutex.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;
struct mvx_csched;
struct mvx_fw_cache;

struct mvx_ext_if {
	struct mutex lock;
	struct device *dev;
	struct mvx_fw_cache *cache;
	struct mvx_client_ops *client_ops;
	struct video_device vdev;
	struct v4l2_device v4l2_dev;
	struct dentry *dsessions;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_ext_if_construct() - Construct the external interface object.
 * @ext:	Pointer to interface object.
 * @dev:	Pointer to device struct.
 * @cache:	Pointer to firmware cache.
 * @client_ops:	Pointer to client client_ops.
 * @parent:	Parent debugfs directory entry.
 *
 * Return: 0 on success, else error code.
 */
int mvx_ext_if_construct(struct mvx_ext_if *ext,
			 struct device *dev,
			 struct mvx_fw_cache *cache,
			 struct mvx_client_ops *client_ops,
			 struct dentry *parent);

/**
 * mvx_ext_if_destruct() - Destroy external interface instance.
 * @ext:   Pointer to interface object.
 */
void mvx_ext_if_destruct(struct mvx_ext_if *ext);

#endif /* _MVX_EXT_H_ */

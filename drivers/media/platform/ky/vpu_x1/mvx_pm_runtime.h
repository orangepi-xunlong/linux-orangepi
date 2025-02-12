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

#ifndef _MVX_PM_RUNTIME_H_
#define _MVX_PM_RUNTIME_H_

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_pm_runtime_get_sync() - The same function as pm_runtime_get_sync(), but
 *			       with the addon that it prints a log line when
 *			       error happens.
 * @dev:	Pointer to device.
 *
 * Return: 0 on success, 1 if already 'active', else error code.
 */
int mvx_pm_runtime_get_sync(struct device *dev);

/**
 * mvx_pm_runtime_put_sync() - The same function as pm_runtime_put_sync(), but
 *			       with the addon that it prints a log line when
 *			       error happens.
 *			       It will not return error if CONFIG_PM is
 *			       undefined.
 * @dev:	Pointer to device.
 *
 * Return: 0 on success, 1 if already 'suspended', else error code.
 */
int mvx_pm_runtime_put_sync(struct device *dev);

#endif /* _MVX_PM_RUNTIME_H_ */

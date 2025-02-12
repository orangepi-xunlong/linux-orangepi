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

#ifndef _MVX_V4L2_CTRLS_H_
#define _MVX_V4L2_CTRLS_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <media/v4l2-ctrls.h>

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_v4l2_ctrls_init() - Initialize V4L2 control handler.
 * @hnd:	V4L2 control handler.
 *
 * This function initializes V4L2 controls for handler @hnd.
 * Controls set to their default values.
 *
 * Return: 0 on success, error code otherwise.
 */
int mvx_v4l2_ctrls_init(struct v4l2_ctrl_handler *hnd);

/**
 * mvx_v4l2_ctrls_done() - Destroy V4L2 control handler.
 * @hnd:	V4L2 control handler.
 *
 * This function destroys V4L2 control handler.
 */
void mvx_v4l2_ctrls_done(struct v4l2_ctrl_handler *hnd);

#endif /* _MVX_V4L2_CTRLS_H_ */

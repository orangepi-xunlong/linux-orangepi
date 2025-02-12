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

#ifndef _MVX_LOG_GROUP_H_
#define _MVX_LOG_GROUP_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include "mvx_log.h"

/******************************************************************************
 * Prototypes
 ******************************************************************************/

extern struct mvx_log_group mvx_log_if;
extern struct mvx_log_group mvx_log_fwif_if;
extern struct mvx_log_group mvx_log_session_if;
extern struct mvx_log_group mvx_log_dev;

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_log_group_init() - Initialize log module. This function must be called
 *		       before any of the log groups is used.
 * @entry_name:		The name of the directory
 *
 * Return: 0 on success, else error code.
 */
int mvx_log_group_init(const char *entry_name);

/**
 * mvx_log_group_deinit() - Destroy log module.
 */
void mvx_log_group_deinit(void);

#endif /* _MVX_LOG_GROUP_H_ */

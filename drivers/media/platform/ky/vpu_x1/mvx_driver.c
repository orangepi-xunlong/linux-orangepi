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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include "mvx_if.h"
#include "mvx_dev.h"
#include "mvx_log_group.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ARMChina");
MODULE_DESCRIPTION("Tiube VPU Driver.");

static int __init mvx_init(void)
{
	int ret;

	ret = mvx_log_group_init("amvx");
	if (ret != 0) {
		pr_err("Failed to create MVx driver logging.\n");
		return ret;
	}

	ret = mvx_dev_init();
	if (ret != 0) {
		pr_err("Failed to register MVx dev driver.\n");
		mvx_log_group_deinit();
		return ret;
	}

	return 0;
}

static void __exit mvx_exit(void)
{
	mvx_dev_exit();
	mvx_log_group_deinit();
}

module_init(mvx_init);
module_exit(mvx_exit);

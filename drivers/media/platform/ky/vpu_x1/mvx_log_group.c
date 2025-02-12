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

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "mvx_log.h"

/******************************************************************************
 * Private variables
 ******************************************************************************/

static struct mvx_log log;

static struct mvx_log_drain drain_dmesg_if;
static struct mvx_log_drain_ram drain_ram0_if;

#ifdef MVX_LOG_FTRACE_ENABLE
static struct mvx_log_drain drain_ftrace_if;
#endif /* MVX_LOG_FTRACE_ENABLE */

struct mvx_log_group mvx_log_if;
struct mvx_log_group mvx_log_fwif_if;
struct mvx_log_group mvx_log_session_if;
struct mvx_log_group mvx_log_dev;

/******************************************************************************
 * External interface
 ******************************************************************************/

int mvx_log_group_init(const char *entry_name)
{
	int ret;
	struct mvx_log_drain *drain_default = &drain_dmesg_if;
	struct mvx_log_drain *drain_ram = &drain_ram0_if.base;

#ifdef MVX_LOG_FTRACE_ENABLE
	drain_default = &drain_ftrace_if;
#endif /* MVX_LOG_FTRACE_ENABLE */

	/* Construct log object. */
	ret = mvx_log_construct(&log, entry_name);
	if (ret != 0)
		return ret;

	/* Construct drain objects and add them to log. */
	mvx_log_drain_dmesg_construct(&drain_dmesg_if);
	ret = mvx_log_drain_add(&log, "dmesg", &drain_dmesg_if);
	if (ret != 0)
		goto delete_log_entry;

	mvx_log_drain_ram_construct(&drain_ram0_if, 256 * 1024);
	ret = mvx_log_drain_ram_add(&log, "ram0", &drain_ram0_if);
	if (ret != 0)
		goto delete_dmesg_drain;

#ifdef MVX_LOG_FTRACE_ENABLE
	mvx_log_drain_ftrace_construct(&drain_ftrace_if);
	mvx_log_drain_add(&log, "ftrace", &drain_ftrace_if);
	if (ret != 0)
		goto delete_ram_drain;

#endif /* MVX_LOG_FTRACE_ENABLE */

	/* Construct group objects. */
	mvx_log_group_construct(&mvx_log_if, "MVX if", MVX_LOG_WARNING,
				drain_default);
	ret = mvx_log_group_add(&log, "generic", &mvx_log_if);
	if (ret != 0)
		goto delete_ftrace_drain;

	mvx_log_group_construct(&mvx_log_fwif_if, "MVX fwif", MVX_LOG_INFO,
				drain_ram);
	ret = mvx_log_group_add(&log, "firmware_interface",
				&mvx_log_fwif_if);
	if (ret != 0)
		goto delete_generic_group;

	mvx_log_group_construct(&mvx_log_session_if, "MVX session",
				MVX_LOG_WARNING,
				drain_default);
	ret = mvx_log_group_add(&log, "session",
				&mvx_log_session_if);
	if (ret != 0)
		goto delete_fwif_group;

	mvx_log_group_construct(&mvx_log_dev, "MVX dev", MVX_LOG_WARNING,
				drain_default);
	ret = mvx_log_group_add(&log, "dev", &mvx_log_dev);
	if (ret != 0)
		goto delete_session_group;

	return 0;

delete_session_group:
	mvx_log_group_destruct(&mvx_log_session_if);

delete_fwif_group:
	mvx_log_group_destruct(&mvx_log_fwif_if);

delete_generic_group:
	mvx_log_group_destruct(&mvx_log_if);

delete_ftrace_drain:

#ifdef MVX_LOG_FTRACE_ENABLE
	mvx_log_drain_ftrace_destruct(&drain_ftrace_if);

delete_ram_drain:
#endif /* MVX_LOG_FTRACE_ENABLE */

	mvx_log_drain_ram_destruct(&drain_ram0_if);

delete_dmesg_drain:
	mvx_log_drain_dmesg_destruct(&drain_dmesg_if);

delete_log_entry:
	mvx_log_destruct(&log);

	return ret;
}

void mvx_log_group_deinit(void)
{
	/* Destroy objects in reverse order. */
	mvx_log_group_destruct(&mvx_log_dev);
	mvx_log_group_destruct(&mvx_log_session_if);
	mvx_log_group_destruct(&mvx_log_fwif_if);
	mvx_log_group_destruct(&mvx_log_if);

#ifdef MVX_LOG_FTRACE_ENABLE
	mvx_log_drain_ftrace_destruct(&drain_ftrace_if);
#endif /* MVX_LOG_FTRACE_ENABLE */

	mvx_log_drain_ram_destruct(&drain_ram0_if);
	mvx_log_drain_dmesg_destruct(&drain_dmesg_if);

	mvx_log_destruct(&log);
}

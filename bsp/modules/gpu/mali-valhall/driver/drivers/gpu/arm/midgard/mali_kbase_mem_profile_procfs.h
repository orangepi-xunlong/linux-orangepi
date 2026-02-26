/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2012-2016, 2020-2022 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

/**
 * DOC: Header file for mem profiles entries in procfs
 *
 */

#ifndef _KBASE_MEM_PROFILE_PROCFS_H
#define _KBASE_MEM_PROFILE_PROCFS_H

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

int kbase_procfs_init(struct kbase_device *kbdev);
void kbase_procfs_term(struct kbase_device *kbdev);
int kbasep_mem_profile_procfs_create(struct kbase_context *kctx);
int kbasep_mem_profile_procfs_remove(struct kbase_context *kctx);

#endif  /*_KBASE_MEM_PROFILE_PROCFS_H*/


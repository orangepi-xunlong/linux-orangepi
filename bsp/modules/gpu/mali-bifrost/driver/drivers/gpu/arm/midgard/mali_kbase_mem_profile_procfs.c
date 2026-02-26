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

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <mali_kbase.h>
#include "mali_kbase_mem_profile_procfs.h"

#if IS_ENABLED(CONFIG_PROC_FS)

int kbase_procfs_init(struct kbase_device *kbdev)
{
	struct proc_dir_entry *proc_entry;

	proc_entry = proc_mkdir("mali", NULL);
	if (IS_ERR_OR_NULL(proc_entry)) {
		dev_err(kbdev->dev, "Couldn't create mali procfs directory: %s\n", kbdev->devname);
		return -ENOMEM;
	}
	kbdev->mali_procfs_directory = proc_entry;

	return 0;
}

void kbase_procfs_term(struct kbase_device *kbdev)
{
	remove_proc_subtree("mali", NULL);
	kbdev->mali_procfs_directory = NULL;
}

static int kbasep_mem_profile_show(struct seq_file *sfile, void *offset)
{
	struct kbase_context *kctx = sfile->private;

	mutex_lock(&kctx->mem_profile_lock);

	seq_write(sfile, kctx->mem_profile_data, kctx->mem_profile_size);
	seq_printf(sfile, "Total EGL memory: %zu\n", kctx->total_gpu_pages << PAGE_SHIFT);
	seq_putc(sfile, '\n');

	mutex_unlock(&kctx->mem_profile_lock);
	return 0;
}

int kbasep_mem_profile_procfs_create(struct kbase_context *kctx)
{
	struct kbase_device *kbdev = kctx->kbdev;
	struct proc_dir_entry *entry;
	char kctx_name[64];

	snprintf(kctx_name, 64, "%d_%d", kctx->tgid, kctx->id);
	entry = proc_mkdir(kctx_name, kbdev->mali_procfs_directory);

		dev_err(kbdev->dev, "create mali procfs directory: %s\n", kctx_name);
	if (IS_ERR_OR_NULL(entry)) {
		dev_err(kbdev->dev, "Couldn't create mali procfs directory: %s\n", kctx_name);
		kctx->kctx_proc_entry = NULL;
		return -ENOMEM;
	}
	kctx->kctx_proc_entry = entry;

	proc_create_single_data("mem_profile", 444, entry, kbasep_mem_profile_show, kctx);
	return 0;
}

int kbasep_mem_profile_procfs_remove(struct kbase_context *kctx)
{
	proc_remove(kctx->kctx_proc_entry);
	kctx->kctx_proc_entry = NULL;
	return 0;
}

#else
int kbase_procfs_init(struct kbase_device *kbdev)
{
	return 0;
}
void kbase_procfs_term(struct kbase_device *kbdev) {}
int kbasep_mem_profile_procfs_create(struct kbase_context *kctx)
{
	return 0;
}
int kbasep_mem_profile_procfs_remove(struct kbase_context *kctx)
{
	return 0;
}
#endif

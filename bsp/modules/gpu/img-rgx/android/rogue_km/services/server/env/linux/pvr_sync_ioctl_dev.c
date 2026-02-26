/*
 * @File        pvr_sync_ioctl_dev.c
 * @Title       Kernel driver for Android's sync mechanism
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "pvr_drm.h"
#include "pvr_sync_api.h"
#include "pvr_sync_ioctl_common.h"

/* This header must always be included last */
#include "kernel_compatibility.h"

#define	FILE_NAME "pvr_sync_ioctl_dev"

static const struct file_operations pvr_sync_fops;

bool pvr_sync_set_private_data(void *connection_data,
			       struct pvr_sync_file_data *fdata)
{
	if (connection_data) {
		void **pfdata = connection_data;

		*pfdata = fdata;

		return true;
	}

	return false;
}

struct pvr_sync_file_data *
pvr_sync_connection_private_data(void *connection_data)
{
	if (connection_data) {
		void **pfdata = connection_data;

		return *pfdata;
	}

	return NULL;
}

struct pvr_sync_file_data *
pvr_sync_get_private_data(struct file *file)
{
	if (file) {
		struct pvr_sync_file_data *fdata = file->private_data;

		return fdata;
	}

	return NULL;
}

void *pvr_sync_get_api_priv(struct file *file)
{
	return pvr_sync_get_api_priv_common(file);
}

bool pvr_sync_is_timeline(struct file *file)
{
	return file->f_op == &pvr_sync_fops;
}

struct file *pvr_sync_get_file_struct(void *file_handle)
{
	return file_handle;
}

static int pvr_sync_open(struct inode *inode, struct file *file)
{
	void *connection_data = &file->private_data;

	return pvr_sync_open_common(connection_data, file);
}

static int pvr_sync_close(struct inode *inode, struct file *file)
{
	void *connection_data = &file->private_data;

	return pvr_sync_close_common(connection_data);
}

static int pvr_sync_ioctl_rename(struct file *file, void __user *user_data)
{
	struct pvr_sync_rename_ioctl_data data;

	if (copy_from_user(&data, user_data, sizeof(data))) {
		pr_err(FILE_NAME ": %s: Failed copy from user\n", __func__);
		return -EFAULT;
	}

	return pvr_sync_ioctl_common(file,
				     DRM_PVR_SYNC_RENAME_CMD, &data);
}

static int pvr_sync_ioctl_force_sw_only(struct file *file)
{
	return pvr_sync_ioctl_common(file,
				     DRM_PVR_SYNC_FORCE_SW_ONLY_CMD, NULL);
}

static int pvr_sync_ioctl_sw_create_fence(struct file *file,
					  void __user *user_data)
{
	struct pvr_sw_sync_create_fence_data data;
	int err;

	if (copy_from_user(&data, user_data, sizeof(data))) {
		pr_err(FILE_NAME ": %s: Failed copy from user\n", __func__);
		return -EFAULT;
	}

	err = pvr_sync_ioctl_common(file,
				    DRM_PVR_SW_SYNC_CREATE_FENCE_CMD, &data);
	if (!err) {
		if (copy_to_user(user_data, &data, sizeof(data))) {
			pr_err(FILE_NAME ": %s: Failed copy to user\n",
			       __func__);
			err = -EFAULT;
		}
	}

	return err;
}

static int pvr_sync_ioctl_sw_inc(struct file *file, void __user *user_data)
{
	int err;
	struct pvr_sw_timeline_advance_data data = {0};

	err = pvr_sync_ioctl_common(file,
				    DRM_PVR_SW_SYNC_INC_CMD, &data);
	if (!err) {
		if (copy_to_user(user_data, &data, sizeof(data))) {
			pr_err(FILE_NAME ": %s: Failed copy to user\n",
			       __func__);
			return -EFAULT;
		}
	}

	return err;
}

static long
pvr_sync_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *user_data = (void __user *)arg;

	switch (cmd) {
	case DRM_IOCTL_PVR_SYNC_RENAME_CMD:
		return pvr_sync_ioctl_rename(file, user_data);
	case DRM_IOCTL_PVR_SYNC_FORCE_SW_ONLY_CMD:
		return pvr_sync_ioctl_force_sw_only(file);
	case DRM_IOCTL_PVR_SW_SYNC_CREATE_FENCE_CMD:
		return pvr_sync_ioctl_sw_create_fence(file, user_data);
	case DRM_IOCTL_PVR_SW_SYNC_INC_CMD:
		return pvr_sync_ioctl_sw_inc(file, user_data);
	default:
		return -ENOTTY;
	}
}

static const struct file_operations pvr_sync_fops = {
	.owner          = THIS_MODULE,
	.open           = pvr_sync_open,
	.release        = pvr_sync_close,
	.unlocked_ioctl = pvr_sync_ioctl,
	.compat_ioctl   = pvr_sync_ioctl,
};

static struct miscdevice pvr_sync_device = {
	.minor          = MISC_DYNAMIC_MINOR,
	.name           = PVRSYNC_MODNAME,
	.fops           = &pvr_sync_fops,
};

int pvr_sync_ioctl_init(void)
{
	int err;

	err = misc_register(&pvr_sync_device);
	if (err)
		pr_err(FILE_NAME ": %s: Failed to register pvr_sync device (%d)\n",
		       __func__, err);

	return err;
}

void pvr_sync_ioctl_deinit(void)
{
	misc_deregister(&pvr_sync_device);
}

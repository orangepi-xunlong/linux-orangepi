/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <linux/types.h>
#include <linux/ioctl.h>

#include "gralloc_debug.h"
#include "sunxi_gralloc.h"
#include "gralloc_drv.h"

typedef int gralloc_ioctl_t(void *data);

struct gralloc_ioctl_desc {
	unsigned int cmd;
	gralloc_ioctl_t *func;
	const char *name;
};

#define GRALLOC_IOCTL_DEF(ioctl, _func)	\
	[GRALLOC_IOCTL_NR(ioctl)] = {		\
		.cmd = ioctl,			\
		.func = (gralloc_ioctl_t *)_func,	\
		.name = #ioctl			\
	}

/* Ioctl table */
static const struct gralloc_ioctl_desc gralloc_ioctls[] = {
	GRALLOC_IOCTL_DEF(GRALLOC_IOC_ALLOCATE_BUFFER, gralloc_allocate_buffer),
	GRALLOC_IOCTL_DEF(GRALLOC_IOC_IMPORT_BUFFER, gralloc_import_buffer),
	GRALLOC_IOCTL_DEF(GRALLOC_IOC_RELEASE_BUFFER, gralloc_release_buffer),
	GRALLOC_IOCTL_DEF(GRALLOC_IOC_LOCK_BUFFER, gralloc_lock_buffer),
	GRALLOC_IOCTL_DEF(GRALLOC_IOC_UNLOCK_BUFFER, gralloc_unlock_buffer),
};

#define GRALLOC_IOCTL_COUNT	ARRAY_SIZE(gralloc_ioctls)

static long gralloc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	const struct gralloc_ioctl_desc *ioctl = NULL;
	unsigned int nr = GRALLOC_IOCTL_NR(cmd);
	char stack_kdata[128];
	char *kdata = NULL;
	unsigned int in_size, out_size, ksize;

	if (nr >= GRALLOC_IOCTL_COUNT) {
		GRALLOC_ERR("error cmd:%d nr:%d\n", cmd, nr);
		return -EINVAL;
	}

	in_size = out_size = ksize = _IOC_SIZE(cmd);
	if ((cmd & IOC_IN) == 0)
		in_size = 0;
	if ((cmd & IOC_OUT) == 0)
		out_size = 0;

	if (ksize <= sizeof(stack_kdata)) {
		kdata = stack_kdata;
	} else {
		kdata = kmalloc(ksize, GFP_KERNEL);
		if (!kdata) {
			ret = -ENOMEM;
			goto err;
		}
	}

	ioctl = &gralloc_ioctls[nr];
	if (!ioctl->func) {
		GRALLOC_ERR("nr:%d has no ioctl\n", nr);
		ret = -EINVAL;
		goto err;
	}
	if (in_size > 0) {
		if (copy_from_user(kdata, (void __user *)arg, in_size) != 0) {
			GRALLOC_ERR("nr:%d copy_from_user failed\n", nr);
			ret = -EFAULT;
			goto err;
		}
	}
	ret = ioctl->func((void *)kdata);
	if (!ret && out_size > 0)
		if (copy_to_user((void __user *)arg, kdata, out_size) != 0) {
			GRALLOC_ERR("nr:%d copy_to_user failed\n", nr);
			ret = -EFAULT;
		}

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}

static int gralloc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int gralloc_release(struct inode *inode, struct file *file)
{
	return 0;
}

int gralloc_mmap(struct file *file, struct vm_area_struct *vma)
{
	return 0;
}

const struct file_operations gralloc_fops = {
	.owner = THIS_MODULE,
	.open = gralloc_open,
	.release = gralloc_release,
	.unlocked_ioctl = gralloc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gralloc_ioctl,
#endif
};

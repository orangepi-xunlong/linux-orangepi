/*
 * Copyright (c) 2020-2031 Allwinnertech Co., Ltd.
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

#include "sunxi_di.h"
#include "di_client.h"

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

typedef int di_ioctl_t(struct di_client *c, void *data);

struct di_ioctl_desc {
	unsigned int cmd;
	di_ioctl_t *func;
	const char *name;
};

#define DI_IOCTL_DEF(ioctl, _func)	\
	[DI_IOCTL_NR(ioctl)] = {		\
		.cmd = ioctl,			\
		.func = (di_ioctl_t *)_func,	\
		.name = #ioctl			\
	}

/* Ioctl table */
static const struct di_ioctl_desc di_ioctls[] = {
	DI_IOCTL_DEF(DI_IOC_GET_VERSION, di_client_get_version),
	DI_IOCTL_DEF(DI_IOC_SET_TIMEOUT, di_client_set_timeout),
	DI_IOCTL_DEF(DI_IOC_PROCESS_FB, di_client_process_fb),

	DI_IOCTL_DEF(DI_IOC_MEM_REQUEST, di_client_mem_request),
	DI_IOCTL_DEF(DI_IOC_MEM_RELEASE, di_client_mem_release),
};

#define DI_IOCTL_COUNT	ARRAY_SIZE(di_ioctls)

static long di_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	struct di_client *c = file->private_data;
	const struct di_ioctl_desc *ioctl = NULL;
	unsigned int nr = DI_IOCTL_NR(cmd);
	char stack_kdata[128];
	char *kdata = NULL;
	unsigned int in_size, out_size, ksize;

	if ((c == NULL)
		|| (nr >= DI_IOCTL_COUNT)) {
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

	ioctl = &di_ioctls[nr];
	if (!ioctl->func) {
		ret = -EINVAL;
		goto err;
	}
	if (in_size > 0) {
		if (copy_from_user(kdata, (void __user *)arg, in_size) != 0) {
			ret = -EFAULT;
			goto err;
		}
	}
	ret = ioctl->func(c, (void *)kdata);
	if (!ret && out_size > 0)
		if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
			ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}

static int di_open(struct inode *inode, struct file *file)
{
	void *client;
	char debug_name[64];

	snprintf(debug_name, 64, "client_%u",
		task_pid_nr(current->group_leader));
	client = di_client_create(debug_name);
	if (client == NULL)
		return -EINVAL;
	file->private_data = client;
	return 0;
}

static int di_release(struct inode *inode, struct file *file)
{
	di_client_destroy(file->private_data);
	return 0;
}

int di_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long mypfn = vma->vm_pgoff;
	unsigned long vmsize = vma->vm_end - vma->vm_start;

	vma->vm_pgoff = 0;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, mypfn,
			    vmsize, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

const struct file_operations di_fops = {
	.owner = THIS_MODULE,
	.open = di_open,
	.release = di_release,
	.unlocked_ioctl = di_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = di_ioctl,
#endif
	.mmap = di_mmap,
};

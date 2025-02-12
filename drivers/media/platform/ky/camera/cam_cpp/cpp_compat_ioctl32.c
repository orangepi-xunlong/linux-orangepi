// SPDX-License-Identifier: GPL-2.0
/*
 * camera cpp compat ioctl32
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifdef CONFIG_COMPAT
#include "linux/uaccess.h"
#include <linux/compat.h>
#include <media/v4l2-subdev.h>
#include <media/x1/x1_cpp_uapi.h>

#include "cpp_compat_ioctl32.h"

#define assign_in_user(to, from)					\
	({									\
		typeof(*from) __assign_tmp;					\
										\
		get_user(__assign_tmp, from) || put_user(__assign_tmp, to);	\
	})

#define put_user_force(__x, __ptr)					\
	({									\
		put_user((typeof(*__x) __force *)(__x), __ptr);			\
	})

struct cpp_reg_cfg_cmd32 {
	enum cpp_reg_cfg_type reg_type;
	uint32_t reg_len;
	compat_caddr_t reg_data;
};

struct cpp_frame_info32 {
	uint32_t frame_id;
	uint32_t client_id;
	struct cpp_reg_cfg_cmd32 regs[MAX_REG_CMDS];
	struct cpp_buffer_info src_buf_info;
	struct cpp_buffer_info dst_buf_info;
	struct cpp_buffer_info pre_buf_info;
};

static int get_cpp_reg_cfg_cmd32(struct cpp_reg_cfg_cmd __user *p64,
				 struct cpp_reg_cfg_cmd32 __user *p32)
{
	compat_caddr_t tmp;

	if (!access_ok(p32, sizeof(*p32)) ||
	    get_user(tmp, &p32->reg_data) ||
	    put_user_force(compat_ptr(tmp), &p64->reg_data) ||
	    assign_in_user(&p64->reg_type, &p32->reg_type) ||
	    assign_in_user(&p64->reg_len, &p32->reg_len)) {
		pr_err("%s:%-5d: Error\n", __func__, __LINE__);
		return -EFAULT;
	}

	return 0;
}

static int get_cpp_frame_info32(struct cpp_frame_info __user *p64,
				struct cpp_frame_info32 __user *p32)
{
	u32 count = MAX_REG_CMDS;

	if (!access_ok(p32, sizeof(*p32)) ||
	    assign_in_user(&p64->frame_id, &p32->frame_id) ||
	    assign_in_user(&p64->client_id, &p32->client_id) ||
	    copy_in_user(&p64->src_buf_info, &p32->src_buf_info,
			 sizeof(p64->src_buf_info)) ||
	    copy_in_user(&p64->dst_buf_info, &p32->dst_buf_info,
			 sizeof(p64->dst_buf_info)) ||
	    copy_in_user(&p64->pre_buf_info, &p32->pre_buf_info,
			 sizeof(p64->pre_buf_info))) {
		pr_err("%s:%-5d: Error\n", __func__, __LINE__);
		return -EFAULT;
	}

	while (count--)
		if (get_cpp_reg_cfg_cmd32(&p64->regs[count], &p32->regs[count]))
			return -EFAULT;

	return 0;
}

#define VIDIOC_X1_CPP_PROCESS_FRAME32	\
	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct cpp_frame_info32)

/**
 * alloc_userspace() - Allocates a 64-bits userspace pointer compatible
 *	for calling the native 64-bits version of an ioctl.
 *
 * @size:	size of the structure itself to be allocated.
 * @aux_space:	extra size needed to store "extra" data, e.g. space for
 *		other __user data that is pointed to fields inside the
 *		structure.
 * @new_p64:	pointer to a pointer to be filled with the allocated struct.
 *
 * Return:
 *
 * if it can't allocate memory, either -ENOMEM or -EFAULT will be returned.
 * Zero otherwise.
 */
static int alloc_userspace(unsigned int size, u32 aux_space, void __user **new_p64)
{
	*new_p64 = compat_alloc_user_space(size + aux_space);
	if (!*new_p64)
		return -ENOMEM;
	if (clear_user(*new_p64, size))
		return -EFAULT;
	return 0;
}

static long native_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct video_device *vdev = video_devdata(file);
	long ret = -ENODEV;

	if (vdev->fops->unlocked_ioctl)
		if (video_is_registered(vdev))
			ret = vdev->fops->unlocked_ioctl(file, cmd, arg);
	else
		ret = -ENOTTY;

	return ret;
}

long x1_cpp_compat_ioctl32(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *p32 = compat_ptr(arg);
	void __user *new_p64 = NULL;
	int compatible_arg = 1;
	unsigned int ncmd;
	long ret = 0;

	switch (cmd) {
	case VIDIOC_X1_CPP_PROCESS_FRAME32:
		ncmd = VIDIOC_X1_CPP_PROCESS_FRAME;
		break;
	default:
		ncmd = cmd;
		break;
	}

	switch (cmd) {
	case VIDIOC_X1_CPP_PROCESS_FRAME32:
		ret = alloc_userspace(sizeof(struct cpp_frame_info), 0, &new_p64);
		if (!ret)
			ret = get_cpp_frame_info32(new_p64, p32);
		compatible_arg = 0;
		break;
	}
	if (ret)
		return ret;

	if (compatible_arg)
		ret = native_ioctl(file, ncmd, (unsigned long)p32);
	else
		ret = native_ioctl(file, ncmd, (unsigned long)new_p64);

	if (ret)
		pr_err("%s: unknown ioctl '%c', dir=%d, #%d (0x%08x)\n",
		       __func__, _IOC_TYPE(cmd), _IOC_DIR(cmd), _IOC_NR(cmd), cmd);

	return ret;
}
#endif /* CONFIG_COMPAT */

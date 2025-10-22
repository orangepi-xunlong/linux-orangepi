/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "../utility/vin_log.h"
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "vin_h3a.h"

#include "../vin-isp/sunxi_isp.h"
#include "../vin-video/vin_video.h"

static struct ispstat_buffer *__isp_stat_buf_find(struct isp_stat *stat, int look_empty)
{
	struct ispstat_buffer *found = NULL;
	int i;

	for (i = 0; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *curr = &stat->buf[i];

		/*
		 * Don't select the buffer which is being copied to
		 * userspace or used by the module.
		 */
		if (curr == stat->locked_buf || curr == stat->active_buf)
			continue;

		/* Don't select uninitialised buffers if it's not required */
		if (!look_empty && curr->empty)
			continue;

#if !defined ISP_600
		if (curr->empty) {
			found = curr;
			break;
		}

		if (!found ||
		    (s32)curr->frame_number - (s32)found->frame_number < 0)
			found = curr;
#else
		if (curr->empty && (curr->state == ISPSTAT_IDLE)) {
			found = curr;
			break;
		}

		if ((!found || (s32)curr->frame_number - (s32)found->frame_number < 0)
			&& (curr->state == ISPSTAT_READY))
			found = curr;
#endif
	}

	return found;
}

static inline struct ispstat_buffer *isp_stat_buf_find_oldest(struct isp_stat *stat)
{
	return __isp_stat_buf_find(stat, 0);
}

static inline struct ispstat_buffer *isp_stat_buf_find_oldest_or_empty(struct isp_stat *stat)
{
	return __isp_stat_buf_find(stat, 1);
}

/* Get next free buffer to write the statistics to and mark it active. */
static void isp_stat_buf_next(struct isp_stat *stat)
{
	if (unlikely(stat->active_buf)) {
		/* Overwriting unused active buffer */
#if !defined CONFIG_ISP_SERVER_MELIS
		vin_log(VIN_LOG_STAT, "%s: new buffer requested without queuing active one.\n",	stat->sd.name);
#else
		vin_log(VIN_LOG_STAT, "new buffer requested without queuing active one.\n");
#endif
	} else {
		stat->active_buf = isp_stat_buf_find_oldest_or_empty(stat);
	}
}

static void isp_stat_buf_release(struct isp_stat *stat)
{
	unsigned long flags;

	spin_lock_irqsave(&stat->isp->slock, flags);
	stat->locked_buf->state = ISPSTAT_IDLE;
	stat->locked_buf = NULL;
	spin_unlock_irqrestore(&stat->isp->slock, flags);
}
#if !defined CONFIG_ISP_SERVER_MELIS
/* Get buffer to userspace. */
static struct ispstat_buffer *isp_stat_buf_get(struct isp_stat *stat, struct vin_isp_stat_data *data)
{
	int rval = 0;
	unsigned long flags;
	struct ispstat_buffer *buf;

	spin_lock_irqsave(&stat->isp->slock, flags);

	while (1) {
		buf = isp_stat_buf_find_oldest(stat);
		if (!buf) {
			spin_unlock_irqrestore(&stat->isp->slock, flags);
			vin_log(VIN_LOG_STAT, "%s: cannot find a buffer.\n", stat->sd.name);
			return ERR_PTR(-EBUSY);
		}
		break;
	}

	stat->locked_buf = buf;

	spin_unlock_irqrestore(&stat->isp->slock, flags);
	if (NULL != data) {
#if !defined ISP_600
		if (buf->buf_size > data->buf_size) {
			vin_warn("%s: userspace's buffer size is not enough.\n", stat->sd.name);
			isp_stat_buf_release(stat);
			return ERR_PTR(-EINVAL);
		}
		rval = copy_to_user(data->buf, buf->virt_addr, buf->buf_size);
		if (rval) {
			vin_warn("%s: failed copying %d bytes of stat data\n", stat->sd.name, rval);
			buf = ERR_PTR(-EFAULT);
			isp_stat_buf_release(stat);
		}
#else
		if ((buf->buf_size + ISP_SAVE_LOAD_STATISTIC_SIZE) > data->buf_size) {
			vin_warn("%s: userspace's buffer size is not enough.\n", stat->sd.name);
			isp_stat_buf_release(stat);
			return ERR_PTR(-EINVAL);
		}

		rval = copy_to_user(data->buf, buf->virt_addr, buf->buf_size);
		if (rval) {
			vin_warn("%s: failed copying %d bytes of stat data\n", stat->sd.name, rval);
			buf = ERR_PTR(-EFAULT);
			isp_stat_buf_release(stat);
			return buf;
		}

		rval = copy_to_user(data->buf + buf->buf_size, stat->isp->isp_save_load.vir_addr + ISP_SAVE_LOAD_REG_SIZE, ISP_SAVE_LOAD_STATISTIC_SIZE);
		if (rval) {
			vin_warn("%s: failed copying %d bytes of save_stat data\n", stat->sd.name, rval);
			buf = ERR_PTR(-EFAULT);
			isp_stat_buf_release(stat);
			return buf;
		}
#endif
	}
	return buf;
}

static void isp_stat_bufs_free(struct isp_stat *stat)
{
	int i;

	for (i = 1; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];
		mm->size = stat->buf_size;

		if (!buf->virt_addr)
			continue;

		mm->vir_addr = buf->virt_addr;
		mm->dma_addr = buf->dma_addr;
		os_mem_free(&stat->isp->pdev->dev, mm);

		buf->dma_addr = NULL;
		buf->virt_addr = NULL;
		buf->empty = 1;
	}

	vin_log(VIN_LOG_STAT, "%s: all buffers were freed.\n", stat->sd.name);

	stat->buf_size = 0;
	stat->active_buf = NULL;
}

static int isp_stat_bufs_alloc(struct isp_stat *stat, u32 size, u32 count)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&stat->isp->slock, flags);

	BUG_ON(stat->locked_buf != NULL);

	for (i = 1; i < stat->buf_cnt; i++) {
		stat->buf[i].empty = 1;
		stat->buf[i].state = ISPSTAT_IDLE;
	}

	/* Are the old buffers big enough? */
	if ((stat->buf_size >= size) && (stat->buf_cnt == count)) {
		spin_unlock_irqrestore(&stat->isp->slock, flags);
		vin_log(VIN_LOG_STAT, "%s: old stat buffers are enough.\n", stat->sd.name);
		return 0;
	}

	spin_unlock_irqrestore(&stat->isp->slock, flags);

	isp_stat_bufs_free(stat);

	stat->buf_size = size;
	stat->buf_cnt = count;

	for (i = 1; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];
		mm->size = size;
		if (!os_mem_alloc(&stat->isp->pdev->dev, mm)) {
			buf->virt_addr = mm->vir_addr;
			buf->dma_addr = mm->dma_addr;
		}
		if (!buf->virt_addr || !buf->dma_addr) {
			vin_err("%s: Can't acquire memory for DMA buffer %d\n",	stat->sd.name, i);
			isp_stat_bufs_free(stat);
			return -ENOMEM;
		}
		buf->empty = 1;
		buf->state = ISPSTAT_IDLE;
	}

#if VIN_FALSE
	for (i = 0; i < stat->buf_cnt; i++) {
		vin_print("save buffer%d dam_addr is 0x%px/0x%llx\n", i, stat->buf[i].dma_addr, (dma_addr_t)stat->buf[i].dma_addr >> 2);
	}
#endif

	return 0;
}

static void isp_stat_queue_event(struct isp_stat *stat, int err)
{
	struct video_device *vdev = stat->sd.devnode;
	struct v4l2_event event;
	struct vin_isp_stat_event_status *status = (void *)event.u.data;

	memset(&event, 0, sizeof(event));
	if (!err)
		status->frame_number = stat->frame_number;
	else
		status->buf_err = 1;

	event.type = stat->event_type;
	v4l2_event_queue(vdev, &event);
}

int isp_stat_request_statistics(struct isp_stat *stat,
				     struct vin_isp_stat_data *data)
{
	struct ispstat_buffer *buf;

	if (stat->state != ISPSTAT_ENABLED) {
		vin_log(VIN_LOG_STAT, "%s: engine not enabled.\n", stat->sd.name);
		return -EINVAL;
	}
	vin_log(VIN_LOG_STAT, "user wants to request statistics.\n");

	mutex_lock(&stat->ioctl_lock);
	buf = isp_stat_buf_get(stat, data);
	if (IS_ERR(buf)) {
		mutex_unlock(&stat->ioctl_lock);
		return PTR_ERR(buf);
	}

	data->frame_number = buf->frame_number;
	data->buf_size = buf->buf_size;

	buf->empty = 1;
	isp_stat_buf_release(stat);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

int isp_stat_config(struct isp_stat *stat, void *new_conf)
{
	int ret;
	u32 count;
	struct vin_isp_h3a_config *user_cfg = new_conf;

	if (!new_conf) {
		vin_log(VIN_LOG_STAT, "%s: configuration is NULL\n", stat->sd.name);
		return -EINVAL;
	}

	mutex_lock(&stat->ioctl_lock);

	user_cfg->buf_size = ISP_STAT_TOTAL_SIZE;

#if defined ISP_600
	if (stat->sensor_fps <= 30)
		count = 3;
	else if (stat->sensor_fps <= 60)
		count = 4;
	else if (stat->sensor_fps <= 120)
		count = 5;
	else
		count = 6;
#else
	if (stat->sensor_fps <= 30)
		count = 2;
	else if (stat->sensor_fps <= 60)
		count = 3;
	else if (stat->sensor_fps <= 120)
		count = 4;
	else
		count = 5;
#endif
	ret = isp_stat_bufs_alloc(stat, user_cfg->buf_size, count);
	if (ret) {
		mutex_unlock(&stat->ioctl_lock);
		return ret;
	}

	/* Module has a valid configuration. */
	stat->configured = 1;

	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

int isp_stat_enable(struct isp_stat *stat, u8 enable)
{
	unsigned long irqflags;

	vin_log(VIN_LOG_STAT, "%s: user wants to %s module.\n", stat->sd.name, enable ? "enable" : "disable");

	/* Prevent enabling while configuring */
	mutex_lock(&stat->ioctl_lock);

	spin_lock_irqsave(&stat->isp->slock, irqflags);

	if (!stat->configured && enable) {
		spin_unlock_irqrestore(&stat->isp->slock, irqflags);
		mutex_unlock(&stat->ioctl_lock);
		vin_log(VIN_LOG_STAT, "%s: cannot enable module as it's never been successfully configured so far.\n", stat->sd.name);
		return -EINVAL;
	}
	stat->stat_en_flag = enable;
	stat->isp->f1_after_librun = 0;

	if (enable)
		stat->state = ISPSTAT_ENABLED;
	else
		stat->state = ISPSTAT_DISABLED;

	isp_stat_buf_next(stat);

	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

#if defined ISP_600
static int isp_stat_buf_queue(struct isp_stat *stat)
{
	if (!stat->active_buf)
		return STAT_NO_BUF;

	stat->active_buf->buf_size = stat->buf_size;

	stat->active_buf->frame_number = stat->frame_number;
	stat->active_buf = NULL;

	return STAT_BUF_DONE;
}

static int isp_stat_buf_process(struct isp_stat *stat, int buf_state)
{
	int ret = STAT_BUF_DONE;
	dma_addr_t dma_addr;

	if (buf_state == STAT_BUF_DONE && stat->state == ISPSTAT_ENABLED) {
		isp_stat_buf_next(stat);

		if (!stat->active_buf)
			return STAT_NO_BUF;

		dma_addr = (dma_addr_t)(stat->active_buf->dma_addr);
		bsp_isp_set_statistics_addr(stat->isp->id, dma_addr);
		if (bsp_isp_get_irq_status(stat->isp->id, PARA_LOAD_PD)) {
			stat->active_buf = NULL;
			vin_warn("para_load_pd, set active bufffer failed!\n");
		} else
			stat->active_buf->state = ISPSTAT_LOAD_SET;
	}

	return ret;
}

void isp_stat_load_set(struct isp_stat *stat)
{
	int ret = STAT_BUF_DONE;
	int i;

	if (stat->state == ISPSTAT_DISABLED)
		return;

	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET) {
			//vin_warn("lost save_pd to get buffer\n");
			return;
		}
	}

	stat->isp->h3a_stat.frame_number++;

	isp_stat_buf_process(stat, ret);

#if VIN_FALSE
	for (i = 0; i < stat->buf_cnt; i++) {
		vin_print("load buffer%d stat is %d\n", i, stat->buf[i].state);
	}
#endif
}

void isp_stat_isr(struct isp_stat *stat)
{
	int ret = STAT_NO_BUF;
	unsigned long irqflags;
	int i;

	vin_log(VIN_LOG_STAT, "buf state is %d, frame number is %d 0x%x %d\n",
		stat->state, stat->frame_number, stat->buf_size, stat->configured);

	if (stat->state == ISPSTAT_DISABLED)
		return;

	spin_lock_irqsave(&stat->isp->slock, irqflags);
	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET) {
			goto next;
		}
	}
	//vin_warn("lost load_pd to set buffer\n");
	stat->isp->save_get_flag = 1;
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	return;

next:
	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET)
			stat->buf[i].state = ISPSTAT_SAVE_SET;
		else if (stat->buf[i].state == ISPSTAT_SAVE_SET) {
			stat->buf[i].state = ISPSTAT_READY;
			stat->buf[i].empty = 0;
			ret = STAT_BUF_DONE;
		}
		vin_log(VIN_LOG_STAT, "save buffer%d stat is %d\n", i, stat->buf[i].state);
	}
	if (ret != STAT_BUF_DONE)
		stat->isp->save_get_flag = 1;
	else
		stat->isp->save_get_flag = 0;

	isp_stat_buf_queue(stat);
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);

	isp_stat_queue_event(stat, ret != STAT_BUF_DONE);
}
#else /* else not ISP_600*/
static int isp_stat_buf_queue(struct isp_stat *stat)
{
	if (!stat->active_buf)
		return STAT_NO_BUF;

	stat->active_buf->buf_size = stat->buf_size;

	stat->active_buf->frame_number = stat->frame_number;
	stat->active_buf->empty = 0;
	stat->active_buf = NULL;

	return STAT_BUF_DONE;
}

static int isp_stat_buf_process(struct isp_stat *stat, int buf_state)
{
	int ret = STAT_NO_BUF;
	dma_addr_t dma_addr;

	if (buf_state == STAT_BUF_DONE && stat->state == ISPSTAT_ENABLED) {
		ret = isp_stat_buf_queue(stat);
		isp_stat_buf_next(stat);

		if (!stat->active_buf)
			return STAT_NO_BUF;

		dma_addr = (dma_addr_t)(stat->active_buf->dma_addr);
		bsp_isp_set_statistics_addr(stat->isp->id, dma_addr);
	}

	return ret;
}

void isp_stat_isr(struct isp_stat *stat)
{
	int ret = STAT_BUF_DONE;
	unsigned long irqflags;

	vin_log(VIN_LOG_STAT, "buf state is %d, frame number is %d 0x%x %d\n",
		stat->state, stat->frame_number, stat->buf_size, stat->configured);

	spin_lock_irqsave(&stat->isp->slock, irqflags);
	if (stat->state == ISPSTAT_DISABLED) {
		spin_unlock_irqrestore(&stat->isp->slock, irqflags);
		return;
	}
	stat->isp->h3a_stat.frame_number++;

	ret = isp_stat_buf_process(stat, ret);

	spin_unlock_irqrestore(&stat->isp->slock, irqflags);

	isp_stat_queue_event(stat, ret != STAT_BUF_DONE);
}
#endif

static long h3a_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct isp_stat *stat = v4l2_get_subdevdata(sd);
	vin_log(VIN_LOG_STAT, "%s cmd is 0x%x\n", __func__, cmd);
	switch (cmd) {
	case VIDIOC_VIN_ISP_H3A_CFG:
		return isp_stat_config(stat, arg);
	case VIDIOC_VIN_ISP_STAT_REQ:
		return isp_stat_request_statistics(stat, arg);
	case VIDIOC_VIN_ISP_STAT_EN:
		return isp_stat_enable(stat, *(u8 *)arg);
	}

	return -ENOIOCTLCMD;
}

#if IS_ENABLED(CONFIG_COMPAT)
struct vin_isp_stat_data32 {
	compat_caddr_t buf;
	__u32 buf_size;
	__u32 frame_number;
	__u32 config_counter;
};
struct vin_isp_h3a_config32 {
	__u32 buf_size;
	__u32 config_counter;
};

static int get_isp_statistics_buf32(struct vin_isp_stat_data *kp,
			      struct vin_isp_stat_data32 __user *up)
{
	u32 tmp;

	if (!access_ok(up, sizeof(*up)) ||
	    get_user(kp->buf_size, &up->buf_size) ||
	    get_user(kp->frame_number, &up->frame_number) ||
	    get_user(kp->config_counter, &up->config_counter) ||
	    get_user(tmp, &up->buf))
		return -EFAULT;
	kp->buf = compat_ptr(tmp);
	return 0;
}

static int put_isp_statistics_buf32(struct vin_isp_stat_data *kp,
			      struct vin_isp_stat_data32 __user *up)
{
	u32 tmp = (u32) ((unsigned long)kp->buf);

	if (!access_ok(up, sizeof(*up)) ||
	    put_user(kp->buf_size, &up->buf_size) ||
	    put_user(kp->frame_number, &up->frame_number) ||
	    put_user(kp->config_counter, &up->config_counter) ||
	    put_user(tmp, &up->buf))
		return -EFAULT;
	return 0;
}

static int get_isp_statistics_config32(struct vin_isp_h3a_config *kp,
			      struct vin_isp_h3a_config32 __user *up)
{
	if (!access_ok(up, sizeof(*up)) ||
	    get_user(kp->buf_size, &up->buf_size) ||
	    get_user(kp->config_counter, &up->config_counter))
		return -EFAULT;
	return 0;
}

static int put_isp_statistics_config32(struct vin_isp_h3a_config *kp,
			      struct vin_isp_h3a_config32 __user *up)
{
	if (!access_ok(up, sizeof(*up)) ||
	    put_user(kp->buf_size, &up->buf_size) ||
	    put_user(kp->config_counter, &up->config_counter))
		return -EFAULT;
	return 0;
}

static int get_isp_statistics_enable32(unsigned int *kp,
			      unsigned int __user *up)
{
	if (!access_ok(up, sizeof(*up)) ||
	    get_user(*kp, up))
		return -EFAULT;
	return 0;
}

static int put_isp_statistics_enable32(unsigned int *kp,
			      unsigned int __user *up)
{
	if (!access_ok(up, sizeof(*up)) ||
	    put_user(*kp, up))
		return -EFAULT;
	return 0;
}

#define VIDIOC_VIN_ISP_H3A_CFG32 _IOWR('V', BASE_VIDIOC_PRIVATE + 31, struct vin_isp_h3a_config32)
#define VIDIOC_VIN_ISP_STAT_REQ32 _IOWR('V', BASE_VIDIOC_PRIVATE + 32, struct vin_isp_stat_data32)
#define VIDIOC_VIN_ISP_STAT_EN32 _IOWR('V', BASE_VIDIOC_PRIVATE + 33, unsigned int)

static long h3a_compat_ioctl32(struct v4l2_subdev *sd,
		unsigned int cmd, unsigned long arg)
{
	union {
		struct vin_isp_h3a_config isb;
		struct vin_isp_stat_data isd;
		unsigned int isu;
	} karg;
	void __user *up = compat_ptr(arg);
	int compatible_arg = 1;
	long err = 0;

	vin_log(VIN_LOG_STAT, "%s cmd is 0x%x\n", __func__, cmd);

	switch (cmd) {
	case VIDIOC_VIN_ISP_STAT_REQ32:
		cmd = VIDIOC_VIN_ISP_STAT_REQ;
		break;
	case VIDIOC_VIN_ISP_H3A_CFG32:
		cmd = VIDIOC_VIN_ISP_H3A_CFG;
		break;
	case VIDIOC_VIN_ISP_STAT_EN32:
		cmd = VIDIOC_VIN_ISP_STAT_EN;
		break;
	}

	switch (cmd) {
	case VIDIOC_VIN_ISP_STAT_REQ:
		err = get_isp_statistics_buf32(&karg.isd, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_ISP_H3A_CFG:
		err = get_isp_statistics_config32(&karg.isb, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_ISP_STAT_EN:
		err = get_isp_statistics_enable32(&karg.isu, up);
		compatible_arg = 0;
		break;
	}

	if (err)
		return err;

	if (compatible_arg)
		err = h3a_ioctl(sd, cmd, up);
	else {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0) /* after linux 5.15, set_fs/get_fs abandoned*/
		mm_segment_t old_fs = get_fs();
		set_fs(KERNEL_DS);
#endif
		err = h3a_ioctl(sd, cmd, &karg);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
		set_fs(old_fs);
#endif
	}

	switch (cmd) {
	case VIDIOC_VIN_ISP_STAT_REQ:
		err = put_isp_statistics_buf32(&karg.isd, up);
		break;
	case VIDIOC_VIN_ISP_H3A_CFG:
		err = put_isp_statistics_config32(&karg.isb, up);
		break;
	case VIDIOC_VIN_ISP_STAT_EN:
		err = put_isp_statistics_enable32(&karg.isu, up);
		compatible_arg = 0;
		break;
	}

	return err;
}
#endif

int isp_stat_subscribe_event(struct v4l2_subdev *subdev,
				  struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	struct isp_stat *stat = v4l2_get_subdevdata(subdev);

	if (sub->type != stat->event_type)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, STAT_NEVENTS, NULL);
}

static const struct v4l2_subdev_core_ops h3a_subdev_core_ops = {
	.ioctl = h3a_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = h3a_compat_ioctl32,
#endif
	.subscribe_event = isp_stat_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_ops h3a_subdev_ops = {
	.core = &h3a_subdev_core_ops,
};

int vin_isp_h3a_init(struct isp_dev *isp)
{
	struct isp_stat *stat = &isp->h3a_stat;

	vin_log(VIN_LOG_STAT, "%s\n", __func__);

	stat->isp = isp;
	stat->event_type = V4L2_EVENT_VIN_H3A;

	mutex_init(&stat->ioctl_lock);

	v4l2_subdev_init(&stat->sd, &h3a_subdev_ops);
	snprintf(stat->sd.name, V4L2_SUBDEV_NAME_SIZE, "sunxi_h3a.%u", isp->id);
	stat->sd.grp_id = VIN_GRP_ID_STAT;
	stat->sd.flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	v4l2_set_subdevdata(&stat->sd, stat);

	stat->pad.flags = MEDIA_PAD_FL_SINK;
	stat->sd.entity.function = MEDIA_ENT_F_PROC_VIDEO_STATISTICS;

	return media_entity_pads_init(&stat->sd.entity, 1, &stat->pad);
}

void vin_isp_h3a_cleanup(struct isp_dev *isp)
{
	struct isp_stat *stat = &isp->h3a_stat;

	vin_log(VIN_LOG_STAT, "%s\n", __func__);

	media_entity_cleanup(&stat->sd.entity);
	mutex_destroy(&stat->ioctl_lock);
	isp_stat_bufs_free(stat);
}
#else //CONFIG_ISP_SERVER_MELIS

int isp_stat_enable(struct isp_stat *stat, u8 enable) //disable in isp stream_off
{
	unsigned long irqflags;

	vin_log(VIN_LOG_STAT, "melis wants to %s module.\n", enable ? "enable" : "disable");

	mutex_lock(&stat->ioctl_lock);
	spin_lock_irqsave(&stat->isp->slock, irqflags);

	stat->stat_en_flag = enable;
	stat->isp->f1_after_librun = 0;

	if (enable)
		stat->state = ISPSTAT_ENABLED;
	else
		stat->state = ISPSTAT_DISABLED;

	isp_stat_buf_next(stat);

	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

static void isp_stat_bufs_free(struct isp_stat *stat)
{
	int i;

	for (i = 1; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];

		mm->size = stat->buf_size;

		if (!buf->virt_addr)
			continue;

		mm->vir_addr = buf->virt_addr;
		mm->dma_addr = buf->dma_addr;
		os_mem_free(&stat->isp->pdev->dev, mm);

		buf->dma_addr = NULL;
		buf->virt_addr = NULL;
	}

	vin_log(VIN_LOG_STAT, "%s: all buffers were freed.\n", __func__);

	stat->buf_size = 0;
	stat->buf_cnt = 0;
	stat->active_buf = NULL;

	isp_rpbuf_destroy(stat->isp);
}

static int isp_stat_bufs_alloc(struct isp_stat *stat, u32 size, u32 count)
{
	int i;
	struct isp_dev *isp = stat->isp;

	if (count > STAT_MAX_BUFS)
		count = STAT_MAX_BUFS;

	stat->buf_size = size;
	stat->buf_cnt = count;

	for (i = 1; i < stat->buf_cnt; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];

		mm->size = size;
		if (!os_mem_alloc(&stat->isp->pdev->dev, mm)) {
			buf->virt_addr = mm->vir_addr;
			buf->dma_addr = mm->dma_addr;
			buf->state = ISPSTAT_IDLE;
			buf->empty = 1;
		}
		if (!buf->virt_addr || !buf->dma_addr) {
			vin_err("%s: Can't acquire memory for stat buffer %d\n", __func__, i);
			isp_stat_bufs_free(stat);
			return -ENOMEM;
		}
	}

	isp_rpbuf_create(isp);

	return 0;
}

static struct ispstat_buffer *isp_stat_buf_get(struct isp_stat *stat)
{
	unsigned long flags;
	struct ispstat_buffer *buf;

	spin_lock_irqsave(&stat->isp->slock, flags);

	while (1) {
		buf = isp_stat_buf_find_oldest(stat);
		if (!buf) {
			spin_unlock_irqrestore(&stat->isp->slock, flags);
			vin_log(VIN_LOG_STAT, "cannot find a buffer.\n");
			return ERR_PTR(-EBUSY);
		}
		break;
	}

	stat->locked_buf = buf;

	spin_unlock_irqrestore(&stat->isp->slock, flags);
	return buf;
}


int isp_stat_request_statistics(struct isp_stat *stat)
{
	struct ispstat_buffer *buf;
	struct isp_dev *isp = stat->isp;

	if (stat->state != ISPSTAT_ENABLED) {
		vin_log(VIN_LOG_STAT, "engine not enabled.\n");
		return -EINVAL;
	}
	vin_log(VIN_LOG_STAT, "melis wants to request statistics.\n");

	mutex_lock(&stat->ioctl_lock);
	buf = isp_stat_buf_get(stat);
	if (IS_ERR(buf)) {
		mutex_unlock(&stat->ioctl_lock);
		return PTR_ERR(buf);
	}

	isp_save_rpbuf_send(isp, buf->virt_addr);
	buf->state = ISPSTAT_IDLE;

	buf->empty = 1;
	isp_stat_buf_release(stat);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

static int isp_stat_buf_queue(struct isp_stat *stat)
{
	if (!stat->active_buf)
		return STAT_NO_BUF;

	stat->active_buf->buf_size = stat->buf_size;

	stat->active_buf->frame_number = stat->frame_number;
	stat->active_buf = NULL;

	return STAT_BUF_DONE;
}

static int isp_stat_buf_process(struct isp_stat *stat, int buf_state)
{
	dma_addr_t dma_addr;

	if (buf_state == STAT_BUF_DONE && stat->state == ISPSTAT_ENABLED) {
		isp_stat_buf_next(stat);

		if (!stat->active_buf)
			return STAT_NO_BUF;

		dma_addr = (dma_addr_t)(stat->active_buf->dma_addr);
		bsp_isp_set_statistics_addr(stat->isp->id, dma_addr);
		if (bsp_isp_get_irq_status(stat->isp->id, PARA_LOAD_PD)) {
			stat->active_buf = NULL;
			vin_warn("para_load_pd, set active bufffer failed!\n");
		} else
			stat->active_buf->state = ISPSTAT_LOAD_SET;
	}

	return STAT_NO_BUF;
}

void isp_stat_load_set(struct isp_stat *stat)
{
	int ret = STAT_BUF_DONE;
	int i;

	if (stat->state == ISPSTAT_DISABLED)
		return;

	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET) {
			//vin_warn("lost save_pd to get buffer\n");
			return;
		}
	}

	stat->isp->h3a_stat.frame_number++;

	isp_stat_buf_process(stat, ret);

#if VIN_FALSE
	for (i = 0; i < stat->buf_cnt; i++) {
		vin_print("load buffer%d stat is %d\n", i, stat->buf[i].state);
	}
#endif
}

void isp_stat_isr(struct isp_stat *stat)
{
	int ret = STAT_NO_BUF;
	unsigned long irqflags;
	int i;

	vin_log(VIN_LOG_STAT, "frame number is %d 0x%x\n",
		stat->frame_number, stat->buf_size);

	if (stat->state == ISPSTAT_DISABLED)
		return;

	spin_lock_irqsave(&stat->isp->slock, irqflags);
	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET) {
			goto next;
		}
	}
	//vin_warn("lost load_pd to set buffer\n");
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);

next:
	for (i = 0; i < stat->buf_cnt; i++) {
		if (stat->buf[i].state == ISPSTAT_LOAD_SET)
			stat->buf[i].state = ISPSTAT_SAVE_SET;
		else if (stat->buf[i].state == ISPSTAT_SAVE_SET) {
			stat->buf[i].state = ISPSTAT_READY;
			stat->buf[i].empty = 0;
			ret = STAT_BUF_DONE;
		}
		vin_log(VIN_LOG_STAT, "save buffer%d stat is %d\n", i, stat->buf[i].state);
	}
	if (ret != STAT_BUF_DONE)
		stat->isp->save_get_flag = 1;
	else
		stat->isp->save_get_flag = 0;

	isp_stat_buf_queue(stat);
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
}

int isp_reset_config_sensor_info(struct isp_dev *isp, enum rpmsg_cmd cmd)
{
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct sensor_config cfg;
	unsigned int data[20];
	char *sensor_name;
	int ret = 0;
	int i;

	memset(&cfg, 0, sizeof(cfg));

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_busy(&vind->vinc[i]->vid_cap))
			continue;

		vinc = vind->vinc[i];
		if (vinc->isp_sel == isp->id)
			break;
	}
	if (vinc == NULL) {
		vin_err("%s:cannot find a real vinc to get sensor info\n", __func__);
		return -1;
	}

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
				VIDIOC_VIN_SENSOR_CFG_REQ, &cfg);
	if (ret) {
		vin_err("%s:get sensor info fail!!!\n", __func__);
		return ret;
	}

	data[1] = cfg.width;
	data[2] = cfg.height;
	data[3] = cfg.fps_fixed;
	data[4] = cfg.wdr_mode;
	// BT601_FULL_RANGE
	data[5] = 1;

	switch (cfg.mbus_code) {
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		data[6] = 8;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		data[6] = 10;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		data[6] = 12;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
		data[6] = 8;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGBRG10_1X10:
		data[6] = 10;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGBRG12_1X12:
		data[6] = 12;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
		data[6] = 8;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SGRBG10_1X10:
		data[6] = 10;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SGRBG12_1X12:
		data[6] = 12;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		data[6] = 8;
		data[7] = ISP_RGGB;
		break;
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		data[6] = 10;
		data[7] = ISP_RGGB;
		break;
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		data[6] = 12;
		data[7] = ISP_RGGB;
		break;
	default:
		data[6] = 10;
		data[7] = ISP_BGGR;
		break;
	}

	if (cfg.hts && cfg.vts && cfg.pclk) {
		data[8] = cfg.hts;
		data[9] = cfg.vts;
		data[10] = cfg.pclk;
		data[11] = cfg.bin_factor;
		data[12] = cfg.gain_min;
		data[13] = cfg.gain_max;
		data[14] = cfg.hoffset;
		data[15] = cfg.voffset;

	} else {
		data[8] = cfg.width;
		data[9] = cfg.height;
		data[10] = cfg.width * cfg.height * 30;
		data[11] = 1;
		data[12] = 16;
		data[13] = 255;
		data[14] = 0;
		data[15] = 0;
	}

	sensor_name = (char *)&data[16];
	memcpy(sensor_name, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name, 4 * sizeof(unsigned int));

	data[0] = cmd;
	isp_rpmsg_send(isp, data, 20 * sizeof(unsigned int));

	return 0;
}

int isp_config_sensor_info(struct isp_dev *isp)
{
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct sensor_config cfg;
	unsigned int data[20];
	char *sensor_name;
	int ret = 0;
	int i;

	memset(&cfg, 0, sizeof(cfg));

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_busy(&vind->vinc[i]->vid_cap))
			continue;

		vinc = vind->vinc[i];
		if (vinc->isp_sel == isp->id)
			break;
	}
	if (vinc == NULL) {
		vin_err("%s:cannot find a real vinc to get sensor info\n", __func__);
		return -1;
	}

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
				VIDIOC_VIN_SENSOR_CFG_REQ, &cfg);
	if (ret) {
		vin_err("%s:get sensor info fail!!!\n", __func__);
		return ret;
	}

	data[1] = cfg.width;
	data[2] = cfg.height;
	data[3] = cfg.fps_fixed;
	data[4] = cfg.wdr_mode;
	// BT601_FULL_RANGE
	data[5] = 1;

	switch (cfg.mbus_code) {
	case MEDIA_BUS_FMT_SBGGR8_1X8:
		data[6] = 8;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		data[6] = 10;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		data[6] = 12;
		data[7] = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
		data[6] = 8;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGBRG10_1X10:
		data[6] = 10;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGBRG12_1X12:
		data[6] = 12;
		data[7] = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
		data[6] = 8;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SGRBG10_1X10:
		data[6] = 10;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SGRBG12_1X12:
		data[6] = 12;
		data[7] = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		data[6] = 8;
		data[7] = ISP_RGGB;
		break;
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		data[6] = 10;
		data[7] = ISP_RGGB;
		break;
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		data[6] = 12;
		data[7] = ISP_RGGB;
		break;
	default:
		data[6] = 10;
		data[7] = ISP_BGGR;
		break;
	}

	if (cfg.hts && cfg.vts && cfg.pclk) {
		data[8] = cfg.hts;
		data[9] = cfg.vts;
		data[10] = cfg.pclk;
		data[11] = cfg.bin_factor;
		data[12] = cfg.gain_min;
		data[13] = cfg.gain_max;
		data[14] = cfg.hoffset;
		data[15] = cfg.voffset;

	} else {
		data[8] = cfg.width;
		data[9] = cfg.height;
		data[10] = cfg.width * cfg.height * 30;
		data[11] = 1;
		data[12] = 16;
		data[13] = 255;
		data[14] = 0;
		data[15] = 0;
	}

	sensor_name = (char *)&data[16];
	memcpy(sensor_name, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name, 4 * sizeof(unsigned int));

	data[0] = VIN_SET_SENSOR_INFO;
	isp_rpmsg_send(isp, data, 20 * sizeof(unsigned int));

	return 0;
}

void isp_sensor_set_exp_gain(struct isp_dev *isp, void *data)
{
	struct sensor_exp_gain exp_gain;
	static struct sensor_exp_gain exp_gain_save[4];
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	unsigned int *result = data;
	int i;

	exp_gain.exp_val = result[1];
	exp_gain.exp_mid_val = result[2];
	exp_gain.gain_val = result[3];
	exp_gain.gain_mid_val = result[4];
	exp_gain.r_gain = result[5];
	exp_gain.b_gain = result[6];

#if VIN_FALSE
	if (memcmp(&exp_gain_save[isp->id], &exp_gain, sizeof(struct sensor_exp_gain))) {
		memcpy(&exp_gain_save[isp->id], &exp_gain, sizeof(struct sensor_exp_gain));
	} else {
		return;
	}
#else
	if ((exp_gain_save[isp->id].exp_val != exp_gain.exp_val) || (exp_gain_save[isp->id].gain_val != exp_gain.gain_val)) {
		memcpy(&exp_gain_save[isp->id], &exp_gain, sizeof(struct sensor_exp_gain));
	} else {
		return;
	}
#endif

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		vinc = vind->vinc[i];
		if (vinc->isp_sel == isp->id)
			break;
	}
	if (vinc != NULL)
		v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
					VIDIOC_VIN_SENSOR_EXP_GAIN, &exp_gain);
}

void isp_get_sensor_state(struct isp_dev *isp)
{
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct sensor_temp temp;
	struct sensor_flip flip;
	unsigned int rpmsg_data[6];
	int i, ret = 0;
	memset(rpmsg_data, 0, sizeof(rpmsg_data));

	rpmsg_data[0] = VIN_SET_SENSOR_STATE;
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		vinc = vind->vinc[i];
		if (vinc->isp_sel == isp->id)
			break;
	}
	if (vinc != NULL) {
		ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
					VIDIOC_VIN_SENSOR_GET_TEMP, &temp);
		if (ret < 0) {
			rpmsg_data[1] = 0;
			rpmsg_data[2] = 0;
		} else {
			rpmsg_data[1] = 1;
			rpmsg_data[2] = temp.temp;
		}

		ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
					VIDIOC_VIN_SENSOR_GET_FLIP, &flip);
		if (ret < 0) {
			rpmsg_data[3] = 0;
			rpmsg_data[4] = 0;
			rpmsg_data[5] = 0;
		} else {
			rpmsg_data[3] = 1;
			rpmsg_data[4] = flip.hflip;
			rpmsg_data[5] = flip.vflip;
		}
	}
	isp_rpmsg_send(isp, rpmsg_data, 6 * sizeof(unsigned int));
}

void isp_set_ir_cfg(struct isp_dev *isp, void *data)
{
	unsigned int *rpmsg_data = data;

	isp->isp_cfg_attr.ir_status = rpmsg_data[1];
}

void isp_update_isp_attr_cfg(struct isp_dev *isp, void *data)
{
	int *rpmsg_data = data;
	unsigned char *u8_rp_data = NULL;

	isp->isp_cfg_attr.cfg_id = rpmsg_data[1];
	switch (isp->isp_cfg_attr.cfg_id) {
	case ISP_CTRL_DN_STR:
		isp->isp_cfg_attr.denoise_level = rpmsg_data[2];
		break;
	case ISP_CTRL_3DN_STR:
		isp->isp_cfg_attr.tdf_level = rpmsg_data[2];
		break;
	case ISP_CTRL_PLTMWDR_STR:
		isp->isp_cfg_attr.pltmwdr_level = rpmsg_data[2];
		break;
	case ISP_CTRL_EV_IDX:
		isp->isp_cfg_attr.ae_ev_idx = rpmsg_data[2];
		break;
	case ISP_CTRL_MAX_EV_IDX:
		isp->isp_cfg_attr.ae_max_ev_idx = rpmsg_data[2];
		break;
	case ISP_CTRL_AE_LOCK:
		isp->isp_cfg_attr.ae_lock = rpmsg_data[2];
		break;
	case ISP_CTRL_AE_STATS:
		u8_rp_data = (unsigned char *)&rpmsg_data[2];
		memcpy(&isp->isp_cfg_attr.ae_stat_avg[0], &u8_rp_data[0], ISP_AE_ROW*ISP_AE_COL * sizeof(unsigned char));
		break;
	case ISP_CTRL_ISO_LUM_IDX:
		isp->isp_cfg_attr.ae_lum_idx = rpmsg_data[2];
		break;
	case ISP_CTRL_COLOR_TEMP:
		isp->isp_cfg_attr.awb_color_temp = rpmsg_data[2];
		break;
	case ISP_CTRL_AE_EV_LV:
		isp->isp_cfg_attr.ae_ev_lv = rpmsg_data[2];
		break;
	case ISP_CTRL_AE_EV_LV_ADJ:
		isp->isp_cfg_attr.ae_ev_lv_adj = rpmsg_data[2];
		break;
	case ISP_CTRL_IR_STATUS:
		isp->isp_cfg_attr.ir_status = rpmsg_data[2];
		break;
	case ISP_CTRL_IR_AWB_GAIN:
		isp->isp_cfg_attr.awb_ir_gain.awb_rgain_ir = rpmsg_data[2];
		isp->isp_cfg_attr.awb_ir_gain.awb_bgain_ir = rpmsg_data[3];
		break;
	}
	isp->isp_cfg_attr.update_flag = 1;
}

void isp_set_encpp_cfg(struct isp_dev *isp, void *data)
{
	unsigned int i;
	unsigned short *rpmsg_data = data;
	unsigned short *ptr = NULL;

	isp->encpp_en = rpmsg_data[2];
	isp->encpp_static_sharp_cfg.ss_shp_ratio = rpmsg_data[3];
	isp->encpp_static_sharp_cfg.ls_shp_ratio = rpmsg_data[4];
	isp->encpp_static_sharp_cfg.ss_dir_ratio = rpmsg_data[5];
	isp->encpp_static_sharp_cfg.ls_dir_ratio = rpmsg_data[6];
	isp->encpp_static_sharp_cfg.ss_crc_stren = rpmsg_data[7];
	isp->encpp_static_sharp_cfg.ss_crc_min = rpmsg_data[8];
	isp->encpp_static_sharp_cfg.wht_sat_ratio = rpmsg_data[9];
	isp->encpp_static_sharp_cfg.blk_sat_ratio = rpmsg_data[10];
	isp->encpp_static_sharp_cfg.wht_slp_bt = rpmsg_data[11];
	isp->encpp_static_sharp_cfg.blk_slp_bt = rpmsg_data[12];
	ptr = &rpmsg_data[13];
	for (i = 0; i < ISP_REG_TBL_LENGTH; i++, ptr++) {
			isp->encpp_static_sharp_cfg.sharp_ss_value[i] = *ptr;
	}
	ptr = &rpmsg_data[46];
	for (i = 0; i < ISP_REG_TBL_LENGTH; i++, ptr++) {
		isp->encpp_static_sharp_cfg.sharp_ls_value[i] = *ptr;
	}
	ptr = &rpmsg_data[79];
	for (i = 0; i < 46; i++, ptr++) {
		isp->encpp_static_sharp_cfg.sharp_hsv[i] = *ptr;
	}
	isp->encpp_dynamic_sharp_cfg.ss_ns_lw = rpmsg_data[125];
	isp->encpp_dynamic_sharp_cfg.ss_ns_hi = rpmsg_data[126];
	isp->encpp_dynamic_sharp_cfg.ls_ns_lw = rpmsg_data[127];
	isp->encpp_dynamic_sharp_cfg.ls_ns_hi = rpmsg_data[128];
	isp->encpp_dynamic_sharp_cfg.ss_lw_cor = rpmsg_data[129];
	isp->encpp_dynamic_sharp_cfg.ss_hi_cor = rpmsg_data[130];
	isp->encpp_dynamic_sharp_cfg.ls_lw_cor = rpmsg_data[131];
	isp->encpp_dynamic_sharp_cfg.ls_hi_cor = rpmsg_data[132];
	isp->encpp_dynamic_sharp_cfg.ss_blk_stren = rpmsg_data[133];
	isp->encpp_dynamic_sharp_cfg.ss_wht_stren = rpmsg_data[134];
	isp->encpp_dynamic_sharp_cfg.ls_blk_stren = rpmsg_data[135];
	isp->encpp_dynamic_sharp_cfg.ls_wht_stren = rpmsg_data[136];
	isp->encpp_dynamic_sharp_cfg.ss_avg_smth = rpmsg_data[137];
	isp->encpp_dynamic_sharp_cfg.ss_dir_smth = rpmsg_data[138];
	ptr = &rpmsg_data[139];
	for (i = 0; i < 4; i++, ptr++) {
		isp->encpp_dynamic_sharp_cfg.dir_smth[i] = *ptr;
	}
	isp->encpp_dynamic_sharp_cfg.hfr_smth_ratio = rpmsg_data[143];
	isp->encpp_dynamic_sharp_cfg.hfr_hf_wht_stren = rpmsg_data[144];
	isp->encpp_dynamic_sharp_cfg.hfr_hf_blk_stren = rpmsg_data[145];
	isp->encpp_dynamic_sharp_cfg.hfr_mf_wht_stren = rpmsg_data[146];
	isp->encpp_dynamic_sharp_cfg.hfr_mf_blk_stren = rpmsg_data[147];
	isp->encpp_dynamic_sharp_cfg.hfr_hf_cor_ratio = rpmsg_data[148];
	isp->encpp_dynamic_sharp_cfg.hfr_mf_cor_ratio = rpmsg_data[149];
	isp->encpp_dynamic_sharp_cfg.hfr_hf_mix_ratio = rpmsg_data[150];
	isp->encpp_dynamic_sharp_cfg.hfr_mf_mix_ratio = rpmsg_data[151];
	isp->encpp_dynamic_sharp_cfg.hfr_hf_mix_min_ratio = rpmsg_data[152];
	isp->encpp_dynamic_sharp_cfg.hfr_mf_mix_min_ratio = rpmsg_data[153];
	isp->encpp_dynamic_sharp_cfg.hfr_hf_wht_clp = rpmsg_data[154];
	isp->encpp_dynamic_sharp_cfg.hfr_hf_blk_clp = rpmsg_data[155];
	isp->encpp_dynamic_sharp_cfg.hfr_mf_wht_clp = rpmsg_data[156];
	isp->encpp_dynamic_sharp_cfg.hfr_mf_blk_clp = rpmsg_data[157];
	isp->encpp_dynamic_sharp_cfg.wht_clp_para = rpmsg_data[158];
	isp->encpp_dynamic_sharp_cfg.blk_clp_para = rpmsg_data[159];
	isp->encpp_dynamic_sharp_cfg.wht_clp_slp = rpmsg_data[160];
	isp->encpp_dynamic_sharp_cfg.blk_clp_slp = rpmsg_data[161];
	isp->encpp_dynamic_sharp_cfg.max_clp_ratio = rpmsg_data[162];
	ptr = &rpmsg_data[163];
	for (i = 0; i < ISP_REG_TBL_LENGTH; i++, ptr++) {
		isp->encpp_dynamic_sharp_cfg.sharp_edge_lum[i] = *ptr;
	}
	isp->encoder_3dnr_cfg.enable_3d_fliter = rpmsg_data[196];
	isp->encoder_3dnr_cfg.adjust_pix_level_enable = rpmsg_data[197];
	isp->encoder_3dnr_cfg.smooth_filter_enable = rpmsg_data[198];
	isp->encoder_3dnr_cfg.max_pix_diff_th = rpmsg_data[199];
	isp->encoder_3dnr_cfg.max_mad_th = rpmsg_data[200];
	isp->encoder_3dnr_cfg.max_mv_th = rpmsg_data[201];
	isp->encoder_3dnr_cfg.min_coef = rpmsg_data[202];
	isp->encoder_3dnr_cfg.max_coef = rpmsg_data[203];
	isp->encoder_2dnr_cfg.enable_2d_fliter = rpmsg_data[204];
	isp->encoder_2dnr_cfg.filter_strength_uv = rpmsg_data[205];
	isp->encoder_2dnr_cfg.filter_strength_y = rpmsg_data[206];
	isp->encoder_2dnr_cfg.filter_th_uv = rpmsg_data[207];
	isp->encoder_2dnr_cfg.filter_th_y = rpmsg_data[208];
}

int isp_write_nor_flash(loff_t to, loff_t to_offset, size_t len, const u_char *buf)
{
#if	IS_ENABLED(CONFIG_MTD_SPI_NOR)
	struct spi_nor *nor = get_spinor();
	struct mtd_info *mtd = &nor->mtd;
	struct erase_info instr;
	uint32_t block_size = 4096;
	int ret;
	size_t block_remain;
	ssize_t written = 0;
	loff_t to_base;
	int offset;
	char *cache = kmalloc(block_size, GFP_KERNEL);

	/* Operable range limits */
	if ((to != ISP0_THRESHOLD_PARAM_OFFSET * 512) && (to != ISP1_THRESHOLD_PARAM_OFFSET * 512)) {
		vin_err("must write 0x%x/0x%x, not to 0x%x\n", ISP0_THRESHOLD_PARAM_OFFSET * 512, ISP1_THRESHOLD_PARAM_OFFSET * 512, (u32)to);
		return 0;
	}

	/* Maximum size of each write is 4K */
	block_remain = min_t(size_t, block_size, len);

	/* read 4k data otherwise being erase*/
	to_base = round_down(to / 512, 8) * 512; /* to_base is the begin of 4k */
	ret = mtd->_read(mtd, to_base, block_size, &written, cache);

	instr.mtd = mtd;
	instr.addr = to;
	instr.len = block_size;
	instr.callback = NULL;
	ret = mtd->_erase_4k(mtd, &instr);
	if (ret)
		goto write_err;

	/* write data by offset from 4k data*/
	offset = to - to_base;
	memcpy(cache + offset + to_offset, buf, block_remain); /* to_offset is offset from the writing sector of 'to' */
	ret = mtd->_write(mtd, to_base, block_size, &written, cache);
	if (ret < 0)
		goto write_err;

	ret = written;

#if VIN_FALSE
	ret = mtd->_read(mtd, to_base, block_size, &written, cache);
	vin_print("read form nor:idx = %d, again = %d, exp = %d\n", *((unsigned int *)(cache + offset)), *((unsigned int *)(cache + offset) + 1), *((unsigned int *)(cache + offset) + 2));
#endif
	kfree(cache);
	return ret;
write_err:
	kfree(cache);
	vin_err("%s err!!!\n", __func__);
	return ret;
#else
	return 0;
#endif
}

void isp_save_ae(struct isp_dev *isp, void *data)
{
	loff_t to;

	if (isp->id == 0)
		to = ISP0_THRESHOLD_PARAM_OFFSET * 512;
	else
		to = ISP1_THRESHOLD_PARAM_OFFSET * 512;

	//vin_print("isp%d:idx = %d, again = %d, exp = %d\n", isp->id, *((unsigned int *)data), *((unsigned int *)data + 1), *((unsigned int *)data + 2));

	isp_write_nor_flash(to, 0, 5*4, data);
}

int vin_isp_h3a_init(struct isp_dev *isp)
{
	struct isp_stat *stat = &isp->h3a_stat;

	if (isp->is_empty)
		return 0;

	vin_log(VIN_LOG_STAT, "%s\n", __func__);

	stat->isp = isp;
	mutex_init(&stat->ioctl_lock);

	isp_stat_bufs_alloc(stat, ISP_STAT_TOTAL_SIZE, STAT_MAX_BUFS);
	return 0;
}
void vin_isp_h3a_cleanup(struct isp_dev *isp)
{
	struct isp_stat *stat = &isp->h3a_stat;

	if (isp->is_empty)
		return;

	vin_log(VIN_LOG_STAT, "%s\n", __func__);

	stat->stat_en_flag = 0;
	mutex_destroy(&stat->ioctl_lock);

	isp_stat_bufs_free(stat);
}
#endif
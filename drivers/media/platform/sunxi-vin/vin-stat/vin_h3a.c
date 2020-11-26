/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-stat/vin_h3a.c
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

/*
 * vin_3a.c
 *
 */

#include <linux/slab.h>
#include <linux/uaccess.h>

#include "vin_h3a.h"
#include "vin_ispstat.h"

/*
 * h3a_setup_regs - Helper function to update h3a registers.
 */
static void h3a_setup_regs(struct isp_stat *h3a, void *priv)
{
	struct vin_isp_h3a_config *conf = priv;
	struct isp_dev *isp = container_of(h3a, struct isp_dev, h3a_stat);
	dma_addr_t dma_addr;

	if ((!h3a->active_buf) || (h3a->state == ISPSTAT_DISABLED))
		return;

	dma_addr = (dma_addr_t)(h3a->active_buf->dma_addr);

	bsp_isp_set_statistics_addr(isp->id, dma_addr);

	if (!h3a->update)
		return;

	h3a->update = 0;
	h3a->config_counter += h3a->inc_config;
	h3a->inc_config = 0;
	h3a->buf_size = conf->buf_size;
}

static void h3a_enable(struct isp_stat *h3a, int enable)
{

}

static int h3a_busy(struct isp_stat *h3a)
{
	return 0;
}

static u32 h3a_get_buf_size(struct vin_isp_h3a_config *conf)
{
	return ISP_STAT_TOTAL_SIZE;
}

static int h3a_validate_params(struct isp_stat *aewb, void *new_conf)
{
	struct vin_isp_h3a_config *user_cfg = new_conf;
	u32 buf_size;

	buf_size = h3a_get_buf_size(user_cfg);
	if (buf_size > user_cfg->buf_size)
		user_cfg->buf_size = buf_size;
	else if (user_cfg->buf_size > ISP_STAT_TOTAL_SIZE)
		user_cfg->buf_size = ISP_STAT_TOTAL_SIZE;

	return 0;
}

static void h3a_set_params(struct isp_stat *stat, void *new_conf)
{
	/*struct vin_isp_h3a_config *user_cfg = new_conf;*/
	struct vin_isp_h3a_config *cur_cfg = stat->priv;
	int update = 0;
	/*
	 * check the privat field, which will write to the hardware later.
	 */
	if (update || !stat->configured) {
		stat->inc_config++;
		stat->update = 1;
		cur_cfg->buf_size = h3a_get_buf_size(cur_cfg);
	}
}

int isp_ae_stat_req(struct isp_dev *isp, struct isp_stat_buf *ae_buf)
{
	int ret = 0;

	if (isp->stat_buf->ae_buf == NULL)
		return -EINVAL;

	ae_buf->buf_size = ISP_STAT_AE_MEM_SIZE;
	ret = copy_to_user(ae_buf->buf, isp->stat_buf->ae_buf, ae_buf->buf_size);
	return ret;
}

int isp_gamma_req(struct isp_dev *isp, struct isp_stat_buf *gamma_buf)
{
	int ret = 0;

	if (isp->isp_tbl.isp_gamma_tbl_vaddr == NULL)
		return -EINVAL;

	gamma_buf->buf_size = ISP_GAMMA_MEM_SIZE;
	ret = copy_to_user(gamma_buf->buf, isp->isp_tbl.isp_gamma_tbl_vaddr,
			   gamma_buf->buf_size);
	return ret;
}

int isp_hist_stat_req(struct isp_dev *isp, struct isp_stat_buf *hist_buf)
{
	int ret = 0;

	if (isp->stat_buf->hist_buf == NULL)
		return -EINVAL;

	hist_buf->buf_size = ISP_STAT_HIST_MEM_SIZE;
	ret = copy_to_user(hist_buf->buf, isp->stat_buf->hist_buf,
			   hist_buf->buf_size);
	return ret;
}

int isp_af_stat_req(struct isp_dev *isp, struct isp_stat_buf *af_buf)
{
	int ret = 0;

	if (isp->stat_buf->af_buf == NULL)
		return -EINVAL;

	af_buf->buf_size = ISP_STAT_AF_MEM_SIZE;
	ret = copy_to_user(af_buf->buf, isp->stat_buf->af_buf, af_buf->buf_size);
	return ret;
}

static long h3a_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct isp_stat *stat = v4l2_get_subdevdata(sd);

	vin_log(VIN_LOG_STAT, "%s cmd is 0x%x\n", __func__, cmd);
	switch (cmd) {
	case VIDIOC_ISP_AE_STAT_REQ:
		return isp_ae_stat_req(stat->isp, arg);
	case VIDIOC_ISP_AF_STAT_REQ:
		return isp_af_stat_req(stat->isp, arg);
	case VIDIOC_ISP_HIST_STAT_REQ:
		return isp_hist_stat_req(stat->isp, arg);
	case VIDIOC_ISP_GAMMA_REQ:
		return isp_gamma_req(stat->isp, arg);
	case VIDIOC_VIN_ISP_H3A_CFG:
		return vin_isp_stat_config(stat, arg);
	case VIDIOC_VIN_ISP_STAT_REQ:
		return vin_isp_stat_request_statistics(stat, arg);
	case VIDIOC_VIN_ISP_STAT_EN: {
		unsigned int *en = arg;

		return vin_isp_stat_enable(stat, !!*en);
	}
	}

	return -ENOIOCTLCMD;
}

#ifdef CONFIG_COMPAT
struct isp_stat_buf32 {
	compat_caddr_t buf;
	__u32 buf_size;
};
static int get_isp_stat_buf32(struct isp_stat_buf *kp,
			      struct isp_stat_buf32 __user *up)
{
	u32 tmp;

	if (!access_ok(VERIFY_READ, up, sizeof(struct isp_stat_buf32)) ||
	    get_user(kp->buf_size, &up->buf_size) || get_user(tmp, &up->buf))
		return -EFAULT;
	kp->buf = compat_ptr(tmp);
	return 0;
}
static int put_isp_stat_buf32(struct isp_stat_buf *kp,
			      struct isp_stat_buf32 __user *up)
{
	u32 tmp = (u32) ((unsigned long)kp->buf);

	if (!access_ok(VERIFY_WRITE, up, sizeof(struct isp_stat_buf32)) ||
	    put_user(kp->buf_size, &up->buf_size) || put_user(tmp, &up->buf))
		return -EFAULT;
	return 0;
}

struct vin_isp_stat_data32 {
	compat_caddr_t buf;
	__u32 buf_size;
	__u32 frame_number;
	__u32 config_counter;
};

static int get_isp_statistics_buf32(struct vin_isp_stat_data *kp,
			      struct vin_isp_stat_data32 __user *up)
{
	u32 tmp;

	if (!access_ok(VERIFY_READ, up, sizeof(struct vin_isp_stat_data32)) ||
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

	if (!access_ok(VERIFY_READ, up, sizeof(struct vin_isp_stat_data32)) ||
	    put_user(kp->buf_size, &up->buf_size) ||
	    put_user(kp->frame_number, &up->frame_number) ||
	    put_user(kp->config_counter, &up->config_counter) ||
	    put_user(tmp, &up->buf))
		return -EFAULT;
	return 0;
}

#define VIDIOC_ISP_AE_STAT_REQ32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct isp_stat_buf32)
#define VIDIOC_ISP_HIST_STAT_REQ32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct isp_stat_buf32)
#define VIDIOC_ISP_AF_STAT_REQ32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 3, struct isp_stat_buf32)
#define VIDIOC_ISP_GAMMA_REQ32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 5, struct isp_stat_buf32)

#define VIDIOC_VIN_ISP_STAT_REQ32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 32, struct vin_isp_stat_data32)

static long h3a_compat_ioctl32(struct v4l2_subdev *sd,
		unsigned int cmd, unsigned long arg)
{
	union {
		struct isp_stat_buf isb;
		struct vin_isp_stat_data isd;
	} karg;
	void __user *up = compat_ptr(arg);
	int compatible_arg = 1;
	long err = 0;

	vin_log(VIN_LOG_STAT, "%s cmd is 0x%x\n", __func__, cmd);

	switch (cmd) {
	case VIDIOC_ISP_AE_STAT_REQ32:
		cmd = VIDIOC_ISP_AE_STAT_REQ;
		break;
	case VIDIOC_ISP_HIST_STAT_REQ32:
		cmd = VIDIOC_ISP_HIST_STAT_REQ;
		break;
	case VIDIOC_ISP_AF_STAT_REQ32:
		cmd = VIDIOC_ISP_AF_STAT_REQ;
		break;
	case VIDIOC_ISP_GAMMA_REQ32:
		cmd = VIDIOC_ISP_GAMMA_REQ;
		break;
	case VIDIOC_VIN_ISP_STAT_REQ32:
		cmd = VIDIOC_VIN_ISP_STAT_REQ;
		break;
	}

	switch (cmd) {
	case VIDIOC_ISP_AE_STAT_REQ:
	case VIDIOC_ISP_HIST_STAT_REQ:
	case VIDIOC_ISP_AF_STAT_REQ:
	case VIDIOC_ISP_GAMMA_REQ:
		err = get_isp_stat_buf32(&karg.isb, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_ISP_STAT_REQ:
		err = get_isp_statistics_buf32(&karg.isd, up);
		compatible_arg = 0;
		break;
	}

	if (err)
		return err;

	if (compatible_arg)
		err = h3a_ioctl(sd, cmd, up);
	else {
		mm_segment_t old_fs = get_fs();

		set_fs(KERNEL_DS);
		err = h3a_ioctl(sd, cmd, &karg);
		set_fs(old_fs);
	}

	switch (cmd) {
	case VIDIOC_ISP_AE_STAT_REQ:
	case VIDIOC_ISP_HIST_STAT_REQ:
	case VIDIOC_ISP_AF_STAT_REQ:
	case VIDIOC_ISP_GAMMA_REQ:
		err = put_isp_stat_buf32(&karg.isb, up);
		break;
	case VIDIOC_VIN_ISP_STAT_REQ:
		err = put_isp_statistics_buf32(&karg.isd, up);
		break;
	}

	return err;
}
#endif

static const struct ispstat_ops h3a_ops = {
	.validate_params	= h3a_validate_params,
	.set_params		= h3a_set_params,
	.setup_regs		= h3a_setup_regs,
	.enable			= h3a_enable,
	.busy			= h3a_busy,
};

static const struct v4l2_subdev_core_ops h3a_subdev_core_ops = {
	.ioctl = h3a_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = h3a_compat_ioctl32,
#endif
	.subscribe_event = vin_isp_stat_subscribe_event,
	.unsubscribe_event = vin_isp_stat_unsubscribe_event,
};

static const struct v4l2_subdev_video_ops h3a_subdev_video_ops = {
	.s_stream = vin_isp_stat_s_stream,
};

static const struct v4l2_subdev_ops h3a_subdev_ops = {
	.core = &h3a_subdev_core_ops,
	.video = &h3a_subdev_video_ops,
};

int vin_isp_h3a_init(struct isp_dev *isp)
{
	struct isp_stat *stat = &isp->h3a_stat;
	struct vin_isp_h3a_config *cfg;
	struct vin_isp_h3a_config *recover_cfg;
	char name[32];

	vin_log(VIN_LOG_STAT, "%s\n", __func__);

	cfg = kzalloc(sizeof(*cfg), GFP_KERNEL);
	if (!cfg)
		return -ENOMEM;

	stat->ops = &h3a_ops;
	stat->priv = cfg;
	stat->isp = isp;
	stat->event_type = V4L2_EVENT_VIN_H3A;

	recover_cfg = kzalloc(sizeof(*recover_cfg), GFP_KERNEL);
	if (!recover_cfg) {
		vin_err("H3A: cannot allocate memory for recover config.\n");
		return -ENOMEM;
	}

	if (h3a_validate_params(stat, recover_cfg)) {
		vin_err("H3A: recover configuration is invalid.\n");
		return -EINVAL;
	}

	recover_cfg->buf_size = h3a_get_buf_size(recover_cfg);
	stat->recover_priv = recover_cfg;
	snprintf(name, sizeof(name), "h3a.%u", isp->id);

	return vin_isp_stat_init(stat, &name[0], &h3a_subdev_ops);
}

void vin_isp_h3a_cleanup(struct isp_dev *isp)
{
	vin_isp_stat_cleanup(&isp->h3a_stat);
	vin_log(VIN_LOG_STAT, "%s\n", __func__);
}

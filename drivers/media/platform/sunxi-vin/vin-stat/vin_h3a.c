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
	if (h3a->state == ISPSTAT_DISABLED)
		return;
#if 0
	bsp_isp_set_statistics_addr((unsigned int)(h3a->active_buf->dma_addr));
	vin_print("%s active buf addr is 0x%x\n", __func__,
		 (int)h3a->active_buf->dma_addr);
#endif
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

static long h3a_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct isp_stat *stat = v4l2_get_subdevdata(sd);
	vin_log(VIN_LOG_STAT, "%s cmd is %d\n", __func__, cmd);
	switch (cmd) {
	case VIDIOC_VIN_ISP_H3A_CFG:
		return vin_isp_stat_config(stat, arg);
	case VIDIOC_VIN_ISP_STAT_REQ:
		return vin_isp_stat_request_statistics(stat, arg);
	case VIDIOC_VIN_ISP_STAT_EN: {
		unsigned long *en = arg;
		return vin_isp_stat_enable(stat, !!*en);
	}
	}

	return -ENOIOCTLCMD;
}

static const struct ispstat_ops h3a_ops = {
	.validate_params	= h3a_validate_params,
	.set_params		= h3a_set_params,
	.setup_regs		= h3a_setup_regs,
	.enable			= h3a_enable,
	.busy			= h3a_busy,
};

static const struct v4l2_subdev_core_ops h3a_subdev_core_ops = {
	.ioctl = h3a_ioctl,
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

	vin_print("%s", __func__);
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
	vin_print("%s\n", __func__);
}

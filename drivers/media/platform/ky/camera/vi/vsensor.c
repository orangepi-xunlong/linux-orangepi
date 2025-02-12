// SPDX-License-Identifier: GPL-2.0
/*
 * vsensor.c - virtual sensor funcitons
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include "vsensor.h"
#include "mlink.h"
#include <cam_plat.h>
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>

static int spm_sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct spm_camera_sensor *sc_sensor = v4l2_subdev_to_sc_sensor(sd);
	struct spm_camera_subdev *sc_subdev = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(&sd->entity);
	int ret = 0, action = 0;

	cam_dbg("%s s_stream %d.", sd->name, enable);
	BUG_ON(!sc_sensor);
	BUG_ON(!pipe);
	sc_subdev = &sc_sensor->sc_subdev;
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	BUG_ON(!sc_pipeline);
	if (enable)
		action = PIPELINE_ACTION_SENSOR_STREAM_ON;
	else
		action = PIPELINE_ACTION_SENSOR_STREAM_OFF;
	ret = blocking_notifier_call_chain(&sc_pipeline->blocking_notify_chain, action, NULL);
	if (NOTIFY_BAD == ret) {
		cam_err("%s(%s) blocking_notifier_call_chain failed", __func__, sc_subdev->name);
		return -1;
	}
	return 0;
}

static struct v4l2_subdev_video_ops spm_sensor_subdev_video_ops = {
	.s_stream = spm_sensor_s_stream,
};

static int spm_sensor_s_power(struct v4l2_subdev *sd, int on)
{
	cam_dbg("%s s_power %d.", sd->name, on);
	return 0;
}

static int spm_sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	struct spm_camera_sensor *sc_sensor = v4l2_subdev_to_sc_sensor(sd);
	struct spm_camera_sensor_strm_ctrl snr_strm_ctrl;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct spm_camera_ispfirm_ops *ispfirm_ops = NULL;
	struct spm_camera_sensor_ops *sensor_ops = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(&sd->entity);
	struct isp_pipe_reset pipe_reset;
	int ret = 0;

	if (!sc_sensor) {
		cam_err("%s sc_sensor was null", __func__);
		return -EINVAL;
	}
	if (!pipe) {
		cam_err("%s(%s) pipe was null", __func__, sc_sensor->sc_subdev.name);
		return -1;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	ispfirm_ops = sc_pipeline->ispfirm_ops;
	sensor_ops = sc_pipeline->sensor_ops;
	if (sc_sensor->idx == 0)
		snr_strm_ctrl.sensor_idx = 0;
	else
		snr_strm_ctrl.sensor_idx = 1;
	if (RESET_STAGE1 == val) {
		snr_strm_ctrl.cmd = SC_SENSOR_STRM_PAUSE;
		sc_sensor_call(sensor_ops, send_cmd, SC_SENSOR_CMD_STRM_CTRL, &snr_strm_ctrl, sizeof(snr_strm_ctrl));
		pipe_reset.pipe_id = PIPELINE_ID(sc_pipeline->id);
		ret = sc_ispfirm_call(ispfirm_ops, send_cmd, SC_ISPFIRM_CMD_PIPE_RESET_START, &pipe_reset, sizeof(pipe_reset));
		if (ret && ret != -ENODEV && ret != -ENOIOCTLCMD) {
			cam_err("%s(%s) pipe%u global reset failed", __func__, sc_sensor->sc_subdev.name, PIPELINE_ID(sc_pipeline->id));
			return ret;
		}
		cam_info("%s(%s) sensor stream ctrl(%u)", __func__, sc_sensor->sc_subdev.name, snr_strm_ctrl.cmd);
	} else if (RESET_STAGE2 == val) {
		pipe_reset.pipe_id = PIPELINE_ID(sc_pipeline->id);
		sc_ispfirm_call(ispfirm_ops, send_cmd, SC_ISPFIRM_CMD_PIPE_RESET_END, &pipe_reset, sizeof(pipe_reset));
	} else if (RESET_STAGE3 == val) {
		snr_strm_ctrl.cmd = SC_SENSOR_STRM_RESUME;
		sc_sensor_call(sensor_ops, send_cmd, SC_SENSOR_CMD_STRM_CTRL, &snr_strm_ctrl, sizeof(snr_strm_ctrl));
		cam_info("%s(%s) sensor stream ctrl(%u)", __func__, sc_sensor->sc_subdev.name, snr_strm_ctrl.cmd);
	}
	return 0;
}

static struct v4l2_subdev_core_ops spm_sensor_subdev_core_ops = {
	.ioctl = spm_subdev_ioctl,
	.s_power = spm_sensor_s_power,
	.reset = spm_sensor_reset,
//#ifdef CONFIG_COMPAT
#if 0
	.compat_ioctl32 = spm_subdev_compat_ioctl32,
#endif
};

static int spm_sensor_pad_set_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_format *format)
{
	struct spm_camera_sensor *sc_sensor = v4l2_subdev_to_sc_sensor(sd);
	struct v4l2_subdev_format remote_pad_fmt;
	struct media_pad *remote_pad = NULL;
	struct v4l2_subdev *remote_sd = NULL;
	struct media_entity *me = &sd->entity;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(me);
	int ret = 0;

	if (!sc_sensor) {
		cam_err("%s sc_sensor was null", __func__);
		return -EINVAL;
	}
	if (format->which != V4L2_SUBDEV_FORMAT_ACTIVE) {
		cam_err("%s didn't support format which(%d).", sd->name, format->which);
		return -EINVAL;
	}
	if (format->pad >= SENSOR_PAD_NUM) {
		cam_err("%s didn't have pad%d.", sd->name, format->pad);
		return -EINVAL;
	}
	remote_pad = media_entity_remote_pad(&(sc_sensor->pads[format->pad]));
	if (!remote_pad) {
		cam_err("%s didn't have valid link.", sd->name);
		return -1;
	}
	if (me->use_count <= 0 || !pipe) {
		cam_err("%s need a pipeline!", sd->name);
		return -1;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	if (sc_pipeline->state >= PIPELINE_ST_STARTED) {
		cam_err("%s %s if busy.", __func__, sd->name);
		return -EBUSY;
	}
	remote_sd = media_entity_to_v4l2_subdev(remote_pad->entity);
	remote_pad_fmt = *format;
	remote_pad_fmt.pad = remote_pad->index;
	ret = v4l2_subdev_call(remote_sd, pad, set_fmt, NULL, &remote_pad_fmt);
	if (ret) {
		cam_err("%s didn't support format(%dx%d code=0x%08x)",
			sd->name,
			format->format.width,
			format->format.height, format->format.code);
		return ret;
	}
	format->format = remote_pad_fmt.format;
	sc_sensor->pad_fmts[format->pad].format = format->format;
	return 0;
}

static int spm_sensor_pad_get_fmt(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state,
				  struct v4l2_subdev_format *format)
{
	struct spm_camera_sensor *sc_sensor = v4l2_subdev_to_sc_sensor(sd);
	struct media_pad *remote_pad = NULL;

	if (!sc_sensor) {
		cam_err("%s sc_sensor was null", __func__);
		return -EINVAL;
	}
	if (format->pad >= SENSOR_PAD_NUM) {
		cam_err("%s didn't have pad%d.", sd->name, format->pad);
		return -EINVAL;
	}
	remote_pad = media_entity_remote_pad(&sc_sensor->pads[format->pad]);
	if (!remote_pad) {
		cam_err("%s didn't have valid link.", sd->name);
		return -1;
	}
	format->format = sc_sensor->pad_fmts[format->pad].format;

	return 0;
}

static struct v4l2_subdev_pad_ops spm_sensor_subdev_pad_ops = {
	.set_fmt = spm_sensor_pad_set_fmt,
	.get_fmt = spm_sensor_pad_get_fmt,
};

static struct v4l2_subdev_ops spm_sensor_subdev_ops = {
	.video = &spm_sensor_subdev_video_ops,
	.core = &spm_sensor_subdev_core_ops,
	.pad = &spm_sensor_subdev_pad_ops,
};

static void spm_sensor_release(struct spm_camera_subdev *sc_subdev)
{
	cam_dbg("%s(%s) enter", __func__, sc_subdev->name);
}

struct spm_camera_sensor *spm_sensor_create(unsigned int grp_id, struct device *dev)
{
	char name[KY_VI_ENTITY_NAME_LEN];
	struct spm_camera_sensor *sc_sensor = NULL;
	int ret = 0, i = 0;

	if (!dev) {
		cam_err("%s dev is null ", __func__);
		return NULL;
	}
	sc_sensor = devm_kzalloc(dev, sizeof(*sc_sensor), GFP_KERNEL);
	if (!sc_sensor) {
		cam_err("%s not enough mem.", __func__);
		return NULL;
	}

	sc_sensor->idx = SD_IDX(grp_id);
	for (i = 0; i < SENSOR_PAD_NUM; i++) {
		sc_sensor->pads[i].flags = MEDIA_PAD_FL_SOURCE;
	}

	snprintf(name, KY_VI_ENTITY_NAME_LEN, "sensor%d", sc_sensor->idx);
	ret = spm_subdev_init(grp_id, name, 1, &spm_sensor_subdev_ops,
			      SENSOR_PAD_NUM, sc_sensor->pads, NULL,
			      &sc_sensor->sc_subdev);
	if (ret) {
		cam_err("%s spm_subdev_init failed ret=%d.", __func__, ret);
		goto sc_subdev_init_fail;
	}
	sc_sensor->sc_subdev.release = spm_sensor_release;
	sc_sensor->sc_subdev.pcsd.sd.dev = dev;
	return sc_sensor;
sc_subdev_init_fail:
	devm_kfree(dev, sc_sensor);
	return NULL;
}

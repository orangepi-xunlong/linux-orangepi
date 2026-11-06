#include "isp_dev.h"

static inline struct sop_isp_device *sd_to_isp_dev(struct v4l2_subdev *sd)
{
	return container_of(sd->v4l2_dev, struct sop_isp_device, v4l2_dev);
}

/* Get cif subdev by enabled media link */
static struct v4l2_subdev *get_remote_sd(struct v4l2_subdev *sd)
{
	struct media_pad *local, *remote;
	struct media_entity *remote_me;
	struct v4l2_subdev *remote_sd = NULL;
	unsigned int pad_id = 0;

	local = &sd->entity.pads[pad_id];
	if (!local) {
		vi_pr(VI_ERR, "%s pad[%d] do not exist!\n", sd->entity.name, pad_id);
		return NULL;
	}
	remote = media_entity_remote_pad(local);
	if (!remote) {
		return NULL;
	}

	remote_me = remote->entity;
	remote_sd = media_entity_to_v4l2_subdev(remote_me);

	return remote_sd;
}

static int get_sensor_index(struct v4l2_subdev *sd)
{
	int index, ret;
	char name[2];
	memcpy(name, sd->name + 3, 2);

	if (name[1] < '0' || name[1] > '9')
		name[1] = 0;

	vi_pr(VI_INFO, "index = %s", name);

	ret = kstrtouint(name, 10, &index); //cmaX

	return index;
}

/* Get sensor subdev by enabled media link */
static struct v4l2_subdev *get_remote_sensor(struct v4l2_subdev *sd, int pad_id)
{
	struct media_pad *local, *remote;
	struct media_entity *remote_me;
	struct v4l2_subdev *remote_cif, *remote_sensor = NULL;
	int num_backlinks;

	remote_cif = get_remote_sd(sd);

	num_backlinks = remote_cif->entity.num_backlinks;

	if (pad_id >= num_backlinks) {
		vi_pr(VI_INFO, "pad(%d) beyond cif backlinks_num(%d)\n", pad_id, num_backlinks);
		return NULL;
	}

	local = &remote_cif->entity.pads[pad_id];
	if (!local) {
		vi_pr(VI_INFO, "%s pad[%d] do not exist!\n",
			remote_cif->entity.name, pad_id);
		return NULL;
	}
	remote = media_entity_remote_pad(local);
	if (!remote) {
		return NULL;
	}

	remote_me = remote->entity;
	remote_sensor = media_entity_to_v4l2_subdev(remote_me);

	return remote_sensor;
}

/***************************** isp sub-devs *******************************/
static int sop_isp_sd_enum_mbus_code(struct v4l2_subdev *sd,
					struct v4l2_subdev_pad_config *cfg,
					struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static int sop_isp_sd_get_fmt(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_format *fmt)
{
	struct v4l2_subdev *remote_sd = get_remote_sd(sd);
	int ret;

	if (!remote_sd) {
		vi_pr(VI_WARN, "no remote subdev!\n");
		return -1;
	}

	ret = v4l2_subdev_call(remote_sd, pad, get_fmt, NULL, fmt);

	return ret;
}

static int sop_isp_sd_enum_frame_size(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
{
	struct v4l2_subdev *remote_sd =  get_remote_sensor(sd, 0);
	int ret;

	ret = v4l2_subdev_call(remote_sd, pad, enum_frame_size, NULL, fse);

	return ret;
}

static int sop_isp_sd_set_fmt(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_format *fmt)
{
	vi_pr(VI_INFO, "+\n");
	return 0;
}

static int sop_isp_sd_get_selection(struct v4l2_subdev *sd,
				      struct v4l2_subdev_pad_config *cfg,
				      struct v4l2_subdev_selection *sel)
{

	return 0;
}

static int sop_isp_sd_set_selection(struct v4l2_subdev *sd,
				      struct v4l2_subdev_pad_config *cfg,
				      struct v4l2_subdev_selection *sel)
{

	return 0;
}

static int sop_isp_sd_s_stream(struct v4l2_subdev *sd, int on)
{
	struct v4l2_subdev *remote_sd = get_remote_sd(sd);

	vi_pr(VI_INFO, "on(%d)\n", on);

	if (!remote_sd) {
		vi_pr(VI_WARN, "no remote cif subdev!\n");
		return -1;
	}
	v4l2_subdev_call(remote_sd, video, s_stream, on);

	return 0;
}

static int sop_isp_sd_s_power(struct v4l2_subdev *sd, int on)
{
	struct sop_isp_device *isp_dev = sd_to_isp_dev(sd);
	struct v4l2_subdev *remote_cif = get_remote_sd(sd);
	struct v4l2_subdev *remote_sensor;
	int ret = 0;
	int index = 0;
	int i;

	vi_pr(VI_INFO, "on(%d)\n", on);
	if (!remote_cif) {
		vi_pr(VI_WARN, "no remote cif subdev!\n");
		return -1;
	}

	v4l2_subdev_call(remote_cif, core, s_power, on);

	if (on) {
		for (i = 0; i < MAX_SENSOR_NUM; i++) {
			remote_sensor = get_remote_sensor(sd, i);
			if(!remote_sensor)
				continue;

			index = get_sensor_index(remote_sensor);
			if (index >= MAX_SENSOR_NUM || index < 0) {
				vi_pr(VI_WARN, "invalid(%d),check sensor subdev name!\n", index);
				continue;
			}

			isp_dev->sensors[index].sd = remote_sensor;
		}

		isp_dev->cif_sdev = remote_cif;

	} else {
		for (i = 0; i < MAX_SENSOR_NUM; i++)
			isp_dev->sensors[index].sd = NULL;
	}

	return ret;
}

static int sop_isp_subdev_link_setup(struct media_entity *entity,
				    const struct media_pad *local,
				    const struct media_pad *remote,
				    u32 flags)
{
	return 0;
}

static int sop_isp_subdev_link_validate(struct media_link *link)
{
	return v4l2_subdev_link_validate(link);
}

#ifdef CONFIG_MEDIA_CONTROLLER
static int sop_isp_subdev_fmt_link_validate(struct v4l2_subdev *sd,
			     struct media_link *link,
			     struct v4l2_subdev_format *source_fmt,
			     struct v4l2_subdev_format *sink_fmt)
{
	if (source_fmt->format.code != sink_fmt->format.code)
		return -EINVAL;

	/* Crop is available */
	if (source_fmt->format.width < sink_fmt->format.width ||
		source_fmt->format.height < sink_fmt->format.height)
		return -EINVAL;

	return 0;
}
#endif

void
sop_isp_queue_event_sof(struct sop_isp_subdev *isp)
{
	struct v4l2_event event = {
		.type = V4L2_EVENT_FRAME_SYNC,
		.u.frame_sync.frame_sequence =
			atomic_inc_return(&isp->frm_sync_seq) - 1,
	};

	v4l2_event_queue(isp->sd.devnode, &event);
}

static int sop_isp_sd_subs_evt(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	if (sub->type != V4L2_EVENT_FRAME_SYNC)
		return -EINVAL;

	/* Line number. For now only zero accepted. */
	if (sub->id != 0)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, ISP_V4L2_EVENT_ELEMS, NULL);
}

static long sop_isp_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	long ret = 0;

	return ret;
}

#ifdef CONFIG_COMPAT
static long sop_isp_compat_ioctl32(struct v4l2_subdev *sd, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	return ret;
}
#endif

static const struct v4l2_subdev_pad_ops sop_isp_sd_pad_ops = {
	.enum_mbus_code = sop_isp_sd_enum_mbus_code,
	.get_selection = sop_isp_sd_get_selection,
	.set_selection = sop_isp_sd_set_selection,
	.enum_frame_size = sop_isp_sd_enum_frame_size,
	.get_fmt = sop_isp_sd_get_fmt,
	.set_fmt = sop_isp_sd_set_fmt,
#ifdef CONFIG_MEDIA_CONTROLLER
	.link_validate = sop_isp_subdev_fmt_link_validate,
#endif
};

static const struct media_entity_operations sop_isp_sd_media_ops = {
	.link_setup = sop_isp_subdev_link_setup,
	.link_validate = sop_isp_subdev_link_validate,
};

static const struct v4l2_subdev_video_ops sop_isp_sd_video_ops = {
	.s_stream = sop_isp_sd_s_stream,
};

static const struct v4l2_subdev_core_ops sop_isp_core_ops = {
	.subscribe_event = sop_isp_sd_subs_evt,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
	.s_power = sop_isp_sd_s_power,
	.ioctl = sop_isp_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = sop_isp_compat_ioctl32,
#endif
};

static struct v4l2_subdev_ops sop_isp_sd_ops = {
	.core = &sop_isp_core_ops,
	.video = &sop_isp_sd_video_ops,
	.pad = &sop_isp_sd_pad_ops,
};

int isp_subdev_register(struct sop_isp_device *isp_dev)
{
	struct sop_isp_subdev *isp_sdev = &isp_dev->isp_sdev;
	struct v4l2_device *v4l2_dev =  &isp_dev->v4l2_dev;
	struct v4l2_subdev *sd = &isp_sdev->sd;
	int ret;

	v4l2_subdev_init(sd, &sop_isp_sd_ops);
	sd->owner = THIS_MODULE;
	sd->grp_id = GRP_ID_ISP;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	sd->entity.ops = &sop_isp_sd_media_ops;
	sd->entity.function = MEDIA_INTF_T_V4L_SUBDEV;
	snprintf(sd->name, sizeof(sd->name), ISP_SUBDEV_NAME);

	isp_sdev->pads[ISP_PAD_SINK].flags =
		MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;
	isp_sdev->pads[ISP_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&sd->entity, 1, isp_sdev->pads);
	if (ret < 0) {
		v4l2_err(sd, "Failed to init isp pads\n");
		goto err_cleanup_media_entity;
	}

	v4l2_set_subdevdata(sd, isp_dev);

	ret = v4l2_device_register_subdev(v4l2_dev, sd);
	if (ret < 0) {
		v4l2_err(sd, "Failed to register isp subdev\n");
		goto err_cleanup_media_entity;
	}

	v4l2_info(sd, "register success\n");

	return 0;
err_cleanup_media_entity:
	media_entity_cleanup(&sd->entity);

	return ret;
}

void isp_subdev_unregister(struct sop_isp_device *isp_dev)
{
	struct v4l2_subdev *sd = &isp_dev->isp_sdev.sd;

	v4l2_device_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
}

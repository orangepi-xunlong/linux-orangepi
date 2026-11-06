#include "cif.h"
#include <linux/sns_v4l2_uapi.h>

static int sns_number;
struct combo_dev_attr_s *g_attr[MAX_CHN_NUM];

static struct v4l2_subdev *get_remote_sensor(struct v4l2_subdev *sd, int pad_index)
{
	struct media_pad *local, *remote;
	struct media_entity *sensor_me;

	if (pad_index >= CIF_PAD_NUM) {
		CIF_PR(CIF_WARNING, "illegal pad index(%d)\n", pad_index);
		return NULL;
	}

	local = &sd->entity.pads[pad_index];
	if (!local) {
		return NULL;
	}
	remote = media_entity_remote_pad(local);
	if (!remote) {
		CIF_PR(CIF_WARNING, "No link between CIF and sensor\n");
		return NULL;
	}

	sensor_me = media_entity_remote_pad(local)->entity;
	if (media_entity_to_v4l2_subdev(sensor_me)) {
		return media_entity_to_v4l2_subdev(sensor_me);
	} else {
		return NULL;
	}
	return NULL;
}

static inline struct cvi_cif_dev *sd_to_dev(struct v4l2_subdev *sdev)
{
	return container_of(sdev, struct cvi_cif_dev, sd);
}

static struct cif_sensor_info *sd_to_sensor(struct cvi_cif_dev *dev,
					struct v4l2_subdev *sd)
{
	int i;

	for (i = 0; i < dev->num_sensors; ++i)
		if (dev->sensors[i].sd == sd)
			return &dev->sensors[i];

	return NULL;
}

static int cif_link_setup(struct media_entity *entity,
				const struct media_pad *local,
				const struct media_pad *remote,
				u32 flags)
{
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(entity);
	struct cvi_cif_dev *dev = sd_to_dev(sd);
	struct v4l2_subdev *remote_sd;
	int ret = 0;

	remote_sd = media_entity_to_v4l2_subdev(remote->entity);

	spin_lock(&dev->lock);

	if (local->flags & MEDIA_PAD_FL_SOURCE) {
		if (flags & MEDIA_LNK_FL_ENABLED) {
			if (dev->sink_linked[local->index - 1]) {
				ret = -EBUSY;
				goto out;
			}
			dev->sink_linked[local->index - 1] = true;
		} else {
			dev->sink_linked[local->index - 1] = false;
		}
	} else {
		if (flags & MEDIA_LNK_FL_ENABLED) {
			if (dev->src_sd) {
				ret = -EBUSY;
				goto out;
			}
			dev->src_sd = remote_sd;
		} else {
			dev->src_sd = NULL;
		}
	}

out:
	spin_unlock(&dev->lock);
	return ret;
}

#ifndef FPGA_PORTING
static int cif_get_sensor_clk_input_info(struct v4l2_subdev *sd,
	struct combo_dev_attr_s *attr, int devno)
{
	//struct cvi_cif_dev *dev = sd_to_dev(sd);
	struct v4l2_subdev *sensor_sd = get_remote_sensor(sd, CIF_PAD_SNS0 + devno);
	struct v4l2_ctrl *link_freq;
	struct v4l2_querymenu qm = { .id = V4L2_CID_LINK_FREQ, };
	int ret, link_num, i;
	int lane_start = SNS_CFG_TYPE_DATA_LANE0;
	int swap_start = SNS_CFG_TYPE_PN_SWAP0;

	if (!sensor_sd) {
		CIF_PR(CIF_ERROR, "sensor subdev is NULL!\n");
		return -1;
	}

	CIF_PR(CIF_WARNING, "get [%s] menu info\n", sensor_sd->name);

	link_freq = v4l2_ctrl_find(sensor_sd->ctrl_handler, V4L2_CID_LINK_FREQ);
	if (!link_freq) {
		CIF_PR(CIF_WARNING, "No pixel rate control in subdev\n");
		return -EPIPE;
	}

	qm.index = v4l2_ctrl_g_ctrl(link_freq);
	ret = v4l2_querymenu(sensor_sd->ctrl_handler, &qm);
	if (ret < 0) {
		CIF_PR(CIF_ERROR, "Failed to get sns clk menu\n");
		return ret;
	}
	link_num = qm.value;

	for (i = 1; i <= link_num; i++) {
		qm.index = i;
		ret = v4l2_querymenu(sensor_sd->ctrl_handler, &qm);
		if (ret < 0) {
			CIF_PR(CIF_ERROR, "Failed to get menu\n");
			return ret;
		}

		if (qm.index == SNS_CFG_TYPE_MCLK_FREQ) {
			attr->mclk.freq = qm.value;
			CIF_PR(CIF_DEBUG, "mclk_freq [%s]\n", _to_string_mclk(qm.value));
		} else if (qm.index == SNS_CFG_TYPE_MCLK_NUM) {
			attr->mclk.cam = qm.value;
			CIF_PR(CIF_DEBUG, "mclk_cam [%lld]\n", qm.value);
		} else if (qm.index == SNS_CFG_TYPE_MAC_FREQ) {
			attr->mac_clk = qm.value;
			CIF_PR(CIF_DEBUG, "mac_freq [%s]\n", _to_string_mac_clk(qm.value));
		} else if (qm.index == SNS_CFG_TYPE_INPUT_MODE) {
			if (qm.value <= INPUT_MODE_BUTT && qm.value >= INPUT_MODE_MIPI) {
				attr->input_mode = qm.value;
				CIF_PR(CIF_DEBUG, "input_mode [%s]\n", _to_string_input_mode(qm.value));
			} else {
				CIF_PR(CIF_ERROR, "have not set right input mode\n");
			}
		} else if (qm.index == SNS_CFG_TYPE_WDR_MODE) {
			if (qm.value <= MIPI_WDR_MODE_BUTT && qm.value >= MIPI_WDR_MODE_NONE) {
				attr->mipi_attr.wdr_mode = qm.value;
				CIF_PR(CIF_DEBUG, "wdr_mode [%s]\n", _to_string_mipi_wdr_mode(qm.value));
			} else {
				CIF_PR(CIF_ERROR, "have not set right hdr mode\n");
			}
		} else if (qm.index == SNS_CFG_TYPE_DATE_BIT) {
			if (qm.value <= RAW_DATA_BUTT && qm.value >= RAW_DATA_8BIT) {
				attr->mipi_attr.raw_data_type = qm.value;
				CIF_PR(CIF_DEBUG, "data_type [%s]\n", _to_string_raw_data_type(qm.value));
			} else {
				CIF_PR(CIF_ERROR, "have not set right data type\n");
			}
		} else if (qm.index == SNS_CFG_TYPE_MIPI_DEV) {
			if (qm.value < MAX_LINK_NUM && qm.value >= 0) {
				attr->devno = qm.value;
				CIF_PR(CIF_DEBUG, "devno [%lld]\n", qm.value);
			} else {
				CIF_PR(CIF_ERROR, "have not set right devno\n");
			}
		} else if (qm.index == SNS_CFG_TYPE_DPHY_EN) {
			attr->mipi_attr.dphy.enable = qm.value;
			CIF_PR(CIF_DEBUG, "dphy.enable [%lld]\n", qm.value);
		}  else if (qm.index == SNS_CFG_TYPE_DPHY_SETTLE) {
			attr->mipi_attr.dphy.hs_settle = qm.value;
			CIF_PR(CIF_DEBUG, "dphy.hs_settle [%lld]\n", qm.value);
		}  else if (qm.index == SNS_CFG_TYPE_PHY_MODE) {
			attr->cif_mode = qm.value;
			CIF_PR(CIF_DEBUG, "cif_mode [%lld]\n", qm.value);
		} else if (qm.index >= lane_start && qm.index < lane_start + 5) {
			attr->mipi_attr.lane_id[qm.index - lane_start] = qm.value;
			CIF_PR(CIF_DEBUG, "lane_id [%d] = [%lld]\n", qm.index - lane_start, qm.value);
		} else if (qm.index >= swap_start && qm.index < swap_start + 5) {
			if (qm.value == 1 || qm.value == 0) {
				attr->mipi_attr.pn_swap[qm.index - swap_start] = qm.value;
				CIF_PR(CIF_DEBUG, "lane_swap [%d] = [%lld]\n", qm.index - swap_start, qm.value);
			} else {
				attr->mipi_attr.pn_swap[qm.index - swap_start] = -1;
				CIF_PR(CIF_DEBUG, "lane swap [%d] = [-1]\n", qm.index - swap_start);
			}
		}
	}

	return 0;
}
#endif

static int cvicif_cif_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
						struct v4l2_event_subscription *sub)
{
#if 0
	if (sub->type == V4L2_EVENT_FRAME_SYNC || sub->type == V4L2_EVENT_RESET_DEV) {
		return v4l2_event_subscribe(fh, sub, CIF_EVENT_ELEMS, NULL);
	} else {
		return -EINVAL;
	}
#endif
	return 0;
}

struct combo_dev_attr_s ex_attr[MAX_CHN_NUM] = {
	[0 ... MAX_CHN_NUM - 1] = {
		.input_mode = INPUT_MODE_MIPI,
		.mac_clk = RX_MAC_CLK_400M,
		.mipi_attr = {
			.raw_data_type = RAW_DATA_12BIT,
			.lane_id = {0, 1, 4, 2, 3, -1, -1, -1, -1},
			.pn_swap = {1, 1, 1, 1, 1, 0, 0, 0, 0},
			.wdr_mode = MIPI_WDR_MODE_NONE,
			.dphy = {
				.enable = 1,
				.hs_settle = 10,
			}
		},
		.mclk = {
			.cam = 1,
			.freq = CAMPLL_FREQ_25M,
		},
		.devno = 0,
	},
};

static int cvicif_cif_s_power(struct v4l2_subdev *sd, int on)
{
	struct cvi_cif_dev *dev = sd_to_dev(sd);

	sns_number = dev->num_sensors;

	CIF_PR(CIF_DEBUG, "cif power(%d), sensor num:%d\n", on, sns_number);

	return 0;
}

static long cvicif_cif_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	//struct cvi_cif_dev *dev = sd_to_dev(sd);
	long ret = 0;

	switch (cmd) {
	default:
		ret = 0;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long cvicif_cif_compat_ioctl32(struct v4l2_subdev *sd, unsigned int cmd, unsigned long arg)
{
	//u32 csi_idx = 0;
	long ret;

	switch (cmd) {
	default:
		ret = 0;
		break;
	}

	return ret;
}
#endif

static int cif_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct cvi_cif_dev *dev = sd_to_dev(sd);
	struct v4l2_subdev *sensor = NULL;
	int ret = 0, i = 0;

	if (sd) {
		CIF_PR(CIF_DEBUG, "cif set stream(%d)\n", enable);

		for (i = 0; i < sns_number; i++) {
			if (enable) {
				g_attr[i] = &ex_attr[i];
				if (cif_get_sensor_clk_input_info(sd, &ex_attr[i], i)) {
					CIF_PR(CIF_ERROR, "cif get sns[%d] clk & input info fail!\n", i);
				}

				if (g_attr[i] != NULL) {
					ret = cif_start_stream(dev, g_attr[i]);
					if (ret) {
						break;
					}
				} else {
					return -EINVAL;
				}
			} else {
				cif_reset_mipi(dev, i);
			}
		}

		for (i = 0; i < sns_number; i++) {
			sensor = get_remote_sensor(sd, CIF_PAD_SNS0 + i);
			if (!sensor) {
				CIF_PR(CIF_ERROR, "sensor subdev is NULL!\n");
				return -1;
			}

			ret = v4l2_subdev_call(sensor, video, s_stream, enable);
			if (ret) {
				CIF_PR(CIF_ERROR, "failed to set sns[%d] on(%d),ret:%d\n", i, enable, ret);
				return ret;
			}
		}
	}

	return ret;
}

static int cif_get_set_fmt(struct v4l2_subdev *sd,
			    struct v4l2_subdev_pad_config *cfg,
			    struct v4l2_subdev_format *fmt)
{
	int ret;
	//struct cvi_cif_dev *dev = sd_to_dev(sd);
	struct v4l2_subdev *sensor = get_remote_sensor(sd, CIF_PAD_SNS0);

	/*
	 * Do not allow format changes and just relay whatever
	 * set currently in the sensor.
	 */
	ret = v4l2_subdev_call(sensor, pad, get_fmt, NULL, fmt);

	return ret;
}

static int cif_enum_frame_size(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
{
	struct v4l2_subdev *sensor = get_remote_sensor(sd, CIF_PAD_SNS0);

	if (!sensor) {
		CIF_PR(CIF_ERROR, "sensor subdev is NULL!\n");
		return -1;
	}

	v4l2_subdev_call(sensor, pad, enum_frame_size, NULL, fse);

	return 0;
}

static int cif_get_selection(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_selection *sel)
{
	return 0;
}

static int cif_set_selection(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_selection *sel)
{
	return 0;
}

static int cif_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad_id,
			      struct v4l2_mbus_config *mbus)
{
	//struct cvi_cif_dev *dev = sd_to_dev(sd);
	struct v4l2_subdev *sensor_sd = get_remote_sensor(sd, pad_id + CIF_PAD_SNS0);
	int ret;

	ret = v4l2_subdev_call(sensor_sd, pad, get_mbus_config, 0, mbus);
	if (ret) {
		return ret;
	}
	if (mbus->flags == 1) {
		if (mbus->type == V4L2_MBUS_CSI2_DPHY) {
			g_attr[pad_id + CIF_PAD_SNS0]->input_mode = INPUT_MODE_MIPI;
		}
	}

	return ret;
}

static int get_sensor_index(struct v4l2_subdev *sd)
{
	int index, ret;
	char name[2];
	memcpy(name, sd->name + 3, 2);

	if (name[1] < '0' || name[1] > '9')
		name[1] = 0;

	CIF_PR(CIF_DEBUG, "index = %s", name);

	ret = kstrtouint(name, 10, &index); //cmaX

	return index;
}

static int cif_attach_ispdev(struct cvi_cif_dev *dev)
{
	struct device_node *np;
	struct platform_device *pdev;
	struct v4l2_device *isp_v4l2;

	np = of_parse_phandle(dev->dev->of_node, "ispv4l2_region", 0);
	if (!np || !of_device_is_available(np)) {
		dev_err(dev->dev, "failed to get isp dev node\n");
		return -ENODEV;
	}

	pdev = of_find_device_by_node(np);
	of_node_put(np);
	if (!pdev) {
		dev_err(dev->dev, "failed to get isp dev from node\n");
		return -ENODEV;
	}

	isp_v4l2 = platform_get_drvdata(pdev);
	if (!isp_v4l2) {
		dev_err(dev->dev, "failed attach isp v4l2 dev\n");
		return -EINVAL;
	}

 	dev->isp_sd = list_first_entry(&isp_v4l2->subdevs,
									struct v4l2_subdev, list);

	printk("attach %s done\n", dev->isp_sd->name);

	return 0;
}

static int cif_notifier_bound(struct v4l2_async_notifier *notifier,
		    struct v4l2_subdev *sd,
		    struct v4l2_async_subdev *asd)
{
	struct cvi_cif_dev *dev = container_of(notifier, struct cvi_cif_dev, notifier);
	struct cif_sensor_info *sensor;
	struct media_link *link;
	unsigned int pad, ret;
	int sensor_index = get_sensor_index(sd);

	CIF_PR(CIF_DEBUG, "cif bound sensor[%s],index:%d\n", sd->name, sensor_index);

	if (sensor_index >= ARRAY_SIZE(dev->sensors) || sensor_index < 0) {
		CIF_PR(CIF_ERROR, "the num of sd is beyond:%d\n", sensor_index);
		return -EBUSY;
	}

	dev->num_sensors++;

	sensor = &dev->sensors[sensor_index];
	sensor->sd = sd;
	if (&sensor->sd->entity == NULL) {
		CIF_PR(CIF_ERROR, "failed to find entity for %s\n", sd->name);
	}

	for (pad = 0; pad < sd->entity.num_pads; pad++)
		if (sensor->sd->entity.pads[pad].flags & MEDIA_PAD_FL_SOURCE)
			break;

	if (pad == sensor->sd->entity.num_pads) {
		CIF_PR(CIF_ERROR, "failed to find src pad for %s\n", sd->name);

		return -ENXIO;
	}

	// link sensor -> cif
	ret = media_create_pad_link(&sensor->sd->entity, pad,
				    &dev->sd.entity, CIF_PAD_SNS0 + sensor_index,
				    MEDIA_LNK_FL_ENABLED);
	if (ret) {
		CIF_PR(CIF_ERROR, "failed to create link pad [%d] for %s : %d\n", pad, sd->name, ret);
		return ret;
	}

	// link cif -> isp
	cif_attach_ispdev(dev);
	ret = media_create_pad_link(&dev->sd.entity, CIF_PAD_ISP,
				    &dev->isp_sd->entity, 0,
				    MEDIA_LNK_FL_ENABLED);
	if (ret) {
		CIF_PR(CIF_ERROR, "failed to create link for %s\n",
							dev->isp_sd->entity.name);
		return ret;
	}

	list_for_each_entry(link, &dev->sd.entity.links, list) {
		media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
	}

	ret = v4l2_device_register_subdev_nodes(&dev->v4l2_dev);

	return 0;
}

static void cif_notifier_unbind(struct v4l2_async_notifier *notifier,
				 struct v4l2_subdev *sd,
				 struct v4l2_async_subdev *asd)
{
	struct cvi_cif_dev *dev = container_of(notifier,
						  struct cvi_cif_dev,
						  notifier);
	struct cif_sensor_info *sensor = sd_to_sensor(dev, sd);

	if (sensor)
		sensor->sd = NULL;
}

static int subdev_notifier_complete(struct v4l2_async_notifier *notifier)
{
	CIF_PR(CIF_DEBUG, "== subdev_notifier_complete ==\n");
	return 0;
}
static const struct v4l2_async_notifier_operations cif_async_ops = {
	.bound = cif_notifier_bound,
	.unbind = cif_notifier_unbind,
	.complete = subdev_notifier_complete,
};

struct cvi_cif_async_subdev {
	struct v4l2_async_subdev asd;
	struct v4l2_mbus_config mbus;
};

static int cif_parse_endpoint(struct device *dev,
			       struct v4l2_fwnode_endpoint *vep,
			       struct v4l2_async_subdev *asd)
{
	struct cvi_cif_async_subdev *cvi_asd =
			container_of(asd, struct cvi_cif_async_subdev, asd);
	struct v4l2_fwnode_bus_parallel *bus = &vep->bus.parallel;

	// CIF_PR(CIF_ERROR, "parse %s, bus type:%d lane:%d\n", asd->match.device_name,
	// 	vep->bus_type, vep->bus.mipi_csi2.num_data_lanes);

	if (vep->bus_type != V4L2_MBUS_BT656 &&
	    vep->bus_type != V4L2_MBUS_CSI2_DPHY &&
	    vep->bus_type != V4L2_MBUS_CSI2_CPHY &&
	    vep->bus_type != V4L2_MBUS_CCP2)
		return 0;


	cvi_asd->mbus.flags = bus->flags;
	cvi_asd->mbus.type = vep->bus_type;

	return 0;
}

static int cif_notifier(struct cvi_cif_dev *dev)
{
	struct v4l2_async_notifier *ntf = &dev->notifier;
	int ret;

	v4l2_async_notifier_init(ntf);

	ret = v4l2_async_notifier_parse_fwnode_endpoints(
		dev->dev, ntf, sizeof(struct cvi_cif_async_subdev), cif_parse_endpoint);
	if (ret) {
		CIF_PR(CIF_ERROR, "parse fwnode endpoints falied : %d\n", ret);
		return ret;
	}

	ntf->ops = &cif_async_ops;

	ret = v4l2_async_notifier_register(&dev->v4l2_dev, ntf);
	if (ret) {
		CIF_PR(CIF_ERROR, "async notifier register falied : %d\n", ret);
		return ret;
	}

	ret = v4l2_async_register_subdev(&dev->sd);

	CIF_PR(CIF_ERROR, "cif register notifier done:%d\n", ret);

	return ret;
}

static const struct media_device_ops cif_media_ops = {
	.link_notify = v4l2_pipeline_link_notify,
};

static const struct media_entity_operations cif_entity_ops = {
	.link_setup = cif_link_setup,
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_core_ops cif_core_ops = {
	.subscribe_event = cvicif_cif_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
	.s_power = cvicif_cif_s_power,
	.ioctl = cvicif_cif_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = cvicif_cif_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops cif_video_ops = {
	.s_stream = cif_s_stream,
};

static const struct v4l2_subdev_pad_ops cif_pad_ops = {
	.get_fmt = cif_get_set_fmt,
	.set_fmt = cif_get_set_fmt,
	.enum_frame_size = cif_enum_frame_size,
	.get_selection = cif_get_selection,
	.set_selection = cif_set_selection,
	.get_mbus_config = cif_g_mbus_config,
};

static const struct v4l2_subdev_ops cif_subdev_ops = {
	.core = &cif_core_ops,
	.video = &cif_video_ops,
	.pad = &cif_pad_ops,
};

int cif_init_subdev(struct platform_device *pdev, struct cvi_cif_dev *dev)
{
	int rc, i;
	struct v4l2_device *v4l2_dev;

	/*init v4l2 dev*/
	v4l2_dev = &dev->v4l2_dev;
	rc = strscpy(v4l2_dev->name, "cif_v4l2", sizeof(dev->sd.name));
	if (rc < 0) {
		dev_err(&pdev->dev, "failed to copy v4l2_dev name :%d\n", rc);
	}

	rc = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to register v4l2 device:%d\n", rc);
		return rc;
	} else {
		dev_info(&pdev->dev, "v4l2 device registered successfully\n");
	}

	snprintf(dev->media_dev.model,
		sizeof(dev->media_dev.model),"%s", "cif_media");
	strscpy(dev->media_dev.driver_name, "cif_subdev",
		sizeof(dev->media_dev.driver_name));

	dev->media_dev.dev = &pdev->dev;
	dev->media_dev.ops = &cif_media_ops;
	v4l2_dev->mdev = &dev->media_dev;
	media_device_init(&dev->media_dev);
	rc = media_device_register(&dev->media_dev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to register media device:%d\n", rc);
	}

	/*init subdev*/
	v4l2_subdev_init(&dev->sd, &cif_subdev_ops);

	/*init pads*/
	for (i = 0; i < CIF_PAD_NUM; i++) {
		if (i == CIF_PAD_ISP) {
			dev->pads[i].flags = MEDIA_PAD_FL_SOURCE;
		} else {
			dev->pads[i].flags = MEDIA_PAD_FL_SINK;
		}
	}
	dev->pads[CIF_PAD_SNS0].flags =
		MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;
	dev->pads[CIF_PAD_ISP].flags =
		MEDIA_PAD_FL_SOURCE | MEDIA_PAD_FL_MUST_CONNECT;
	dev->sd.entity.ops = &cif_entity_ops;
	dev->sd.entity.function = MEDIA_INTF_T_V4L_SUBDEV;

	rc = media_entity_pads_init(&dev->sd.entity, CIF_PAD_NUM, dev->pads);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to init isp pads :%d\n", rc);
	}

	/*set subdev parameter, must be set after init subdev*/
	//dev->sd.dev = &pdev->dev;
	dev->sd.owner = THIS_MODULE;
	dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	rc = strscpy(dev->sd.name, "cif_v4l2", sizeof(dev->sd.name));
	if (rc < 0) {
		dev_err(&pdev->dev, "failed to copy subdev name :%d\n", rc);
	}

	v4l2_set_subdevdata(&dev->sd, dev);

	rc = v4l2_device_register_subdev(&dev->v4l2_dev, &dev->sd);
	if (rc < 0) {
		dev_err(&pdev->dev, "failed to register subdev : %d\n", rc);
		return rc;
	} else {
		dev_info(&pdev->dev, "subdev registered successfully\n");
	}

	rc = cif_notifier(dev);
	if (rc < 0) {
		dev_err(&pdev->dev, "failed to register notifier : %d\n", rc);
		return rc;
	} else {
		dev_info(&pdev->dev, " register notifier successfully\n");
	}

	for (i = 0; i < MAX_LINK_NUM; i++) {
		struct cif_ctx *ctx = &dev->link[i].cif_ctx;

		ctx->mac_phys_regs = cif_get_mac_phys_reg_bases(i);
		ctx->wrap_phys_regs = cif_get_wrap_phys_reg_bases(i);
	}

	return rc;
}
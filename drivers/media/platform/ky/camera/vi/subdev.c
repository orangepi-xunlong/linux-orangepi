// SPDX-License-Identifier: GPL-2.0
/*
 * subdev.c - subdev functions
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include "subdev.h"
#include "mlink.h"
#include "vdev.h"
#include <media/x1/x1_plat_cam.h>
#include <linux/compat.h>
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>

static int spm_subdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct spm_camera_subdev *sc_subdev = v4l2_subdev_to_sc_subdev(sd);
	cam_dbg("%s(%s) enter.", __func__, sc_subdev->name);
	return 0;
}

static int spm_subdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct media_entity *me = &sd->entity;
	struct media_device *mdev = me->graph_obj.mdev;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct spm_camera_subdev *sc_subdev = v4l2_subdev_to_sc_subdev(sd);
	struct media_pipeline *pipe = media_entity_pipeline(me);

	cam_dbg("%s(%s) enter.", __func__, sc_subdev->name);
	mutex_lock(&mdev->graph_mutex);
	if (pipe) {
		sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
		if (sc_pipeline->state >= PIPELINE_ST_STARTED) {
			__spm_mlink_stop_pipeline(me);
		}
		while (sc_pipeline->state >= PIPELINE_ST_GET) {
			__spm_mlink_put_pipeline(me, 1);
		}
	}
	mutex_unlock(&mdev->graph_mutex);
	return 0;
}

static void spm_subdev_notify(struct v4l2_subdev *sd, unsigned int notification, void *arg)
{
	struct spm_camera_subdev *sc_subdev = v4l2_subdev_to_sc_subdev(sd);

	if (sc_subdev && sc_subdev->notify)
		sc_subdev->notify(sc_subdev, notification, arg);
}

struct v4l2_subdev_internal_ops spm_subdev_internal_ops = {
	.open = spm_subdev_open,
	.close = spm_subdev_close,
};

static const struct spm_v4l2_subdev_ops spm_subdev_ops = {
	.notify = spm_subdev_notify,
};

static void spm_subdev_block_release(struct spm_camera_block *sc_block)
{
	struct spm_camera_subdev *sc_subdev =
	    container_of(sc_block, struct spm_camera_subdev, sc_block);

	if (sc_subdev->release)
		sc_subdev->release(sc_subdev);
	spm_mlink_pipeline_release(&sc_subdev->sc_pipeline);
	plat_cam_unregister_subdev(&sc_subdev->pcsd);
}

static struct spm_camera_block_ops spm_subdev_block_ops = {
	.release = spm_subdev_block_release,
};

int spm_subdev_init(unsigned int grp_id,
		    const char *name,
		    int is_sensor,
		    const struct v4l2_subdev_ops *ops,
		    unsigned int pads_cnt,
		    struct media_pad *pads,
		    void *drvdata, struct spm_camera_subdev *sc_subdev)
{
	int ret = 0;
	struct plat_cam_subdev *pcsd = NULL;

	if (!name || !sc_subdev) {
		cam_err("%s invalid arguments.", __func__);
		return -EINVAL;
	}
	cam_dbg("%s(%s) enter.", __func__, name);
	pcsd = &sc_subdev->pcsd;
	spm_mlink_pipeline_init(&sc_subdev->sc_pipeline);
	spm_camera_block_init(&sc_subdev->sc_block, &spm_subdev_block_ops);
	pcsd->ops = ops;
	pcsd->internal_ops = &spm_subdev_internal_ops;
	pcsd->spm_ops = &spm_subdev_ops;
	pcsd->name = (char *)name;
	pcsd->sd_flags = V4L2_SUBDEV_FL_HAS_DEVNODE;
	if (is_sensor)
		pcsd->ent_function = MEDIA_ENT_F_CAM_SENSOR;
	else
		pcsd->ent_function = MEDIA_ENT_F_X1_VI;
	pcsd->pads_cnt = pads_cnt;
	pcsd->pads = pads;
	pcsd->token = drvdata;
	ret = plat_cam_register_subdev(pcsd);
	if (ret) {
		cam_err("%s register plat cam(%s) failed ret=%d ", __func__, name, ret);
		return ret;
	}
	strlcpy(sc_subdev->name, name, V4L2_SUBDEV_NAME_SIZE);
	sc_subdev->pcsd.sd.grp_id = grp_id;
	return 0;
}

long spm_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	long ret = 0;
	struct spm_camera_subdev *sc_subdev = v4l2_subdev_to_sc_subdev(sd);
	struct v4l2_vi_entity_info *entity_info = NULL;
	struct v4l2_subdev_format *format = NULL;

	if (sc_subdev->ioctl) {
		ret = sc_subdev->ioctl(sd, cmd, arg);
		if (ret != -ENOIOCTLCMD)
			return ret;
	}

	switch (cmd) {
	case VIDIOC_GET_PIPELINE:
		return spm_mlink_get_pipeline(&sc_subdev->sc_pipeline.media_pipe, &sd->entity);
	case VIDIOC_PUT_PIPELINE:
		return spm_mlink_put_pipeline(&sd->entity, *((int *)arg));
	case VIDIOC_APPLY_PIPELINE:
		return spm_mlink_apply_pipeline(&sd->entity);
	case VIDIOC_START_PIPELINE:
		return spm_mlink_start_pipeline(&sd->entity);
	case VIDIOC_STOP_PIPELINE:
		return spm_mlink_stop_pipeline(&sd->entity);
	case VIDIOC_RESET_PIPELINE:
		return spm_mlink_reset_pipeline(&sd->entity, *((int *)arg));
	case VIDIOC_G_ENTITY_INFO:
		entity_info = (struct v4l2_vi_entity_info *)arg;
		entity_info->id = media_entity_id(&sd->entity);
		strlcpy(entity_info->name, sc_subdev->name, KY_VI_ENTITY_NAME_LEN);
		return 0;
	case VIDIOC_QUERYCAP: {
		struct v4l2_capability *cap = (struct v4l2_capability*)arg;
		strlcpy(cap->driver, "kyisp", 16);
		cap->capabilities = V4L2_CAP_DEVICE_CAPS | V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
		cap->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	}
		return 0;
	case VIDIOC_SUBDEV_S_FMT:
		format = (struct v4l2_subdev_format *)arg;
		return v4l2_subdev_call(sd, pad, set_fmt, NULL, format);
		break;
	default:
		cam_warn("%s(%s) unknown ioctl(%d)", __func__, sc_subdev->name, cmd);
		return -ENOIOCTLCMD;
	}
	return 0;
}

//#ifdef CONFIG_COMPAT
#if 0

static int alloc_userspace(unsigned int size, u32 aux_space, void __user **new_p64)
{
	*new_p64 = compat_alloc_user_space(size + aux_space);
	if (!*new_p64)
		return -ENOMEM;
	if (clear_user(*new_p64, size))
		return -EFAULT;
	return 0;
}

static int video_get_user(void __user *arg, void *parg, unsigned int cmd,
			  bool *always_copy)
{
	unsigned int n = _IOC_SIZE(cmd);

	if (!(_IOC_DIR(cmd) & _IOC_WRITE)) {
		/* read-only ioctl */
		memset(parg, 0, n);
		return 0;
	}

	switch (cmd) {
	default:

		if (copy_from_user(parg, (void __user *)arg, n))
			return -EFAULT;

		/* zero out anything we don't copy from userspace */
		if (n < _IOC_SIZE(cmd))
			memset((u8 *) parg + n, 0, _IOC_SIZE(cmd) - n);
		break;
	}

	return 0;
}

static int video_put_user(void __user *arg, void *parg, unsigned int cmd)
{
	if (!(_IOC_DIR(cmd) & _IOC_READ))
		return 0;

	switch (cmd) {
	default:
		/*  Copy results into user buffer  */
		if (copy_to_user(arg, parg, _IOC_SIZE(cmd)))
			return -EFAULT;
		break;
	}

	return 0;
}

static long spm_subdev_video_usercopy(struct v4l2_subdev *sd, unsigned int cmd, unsigned long arg,
	       long (*func)(struct v4l2_subdev *sd, unsigned int cmd, void *arg))
{
	char sbuf[128];
	void *mbuf = NULL, *array_buf = NULL;
	void *parg = (void *)arg;
	long err = -EINVAL;
	bool always_copy = false;
	const size_t ioc_size = _IOC_SIZE(cmd);

	/*  Copy arguments into temp kernel buffer  */
	if (_IOC_DIR(cmd) != _IOC_NONE) {
		if (ioc_size <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
			/* too big to allocate from stack */
			mbuf = kvmalloc(ioc_size, GFP_KERNEL);
			if (NULL == mbuf)
				return -ENOMEM;
			parg = mbuf;
		}

		err = video_get_user((void __user *)arg, parg, cmd, &always_copy);
		if (err)
			goto out;
	}

	/* Handles IOCTL */
	err = func(sd, cmd, parg);
	if (err == -ENOTTY || err == -ENOIOCTLCMD) {
		err = -ENOTTY;
		goto out;
	}

	/*
	 * Some ioctls can return an error, but still have valid
	 * results that must be returned.
	 */
	if (err < 0 && !always_copy)
		goto out;

	if (video_put_user((void __user *)arg, parg, cmd))
		err = -EFAULT;
out:
	kvfree(array_buf);
	kvfree(mbuf);
	return err;
}
long spm_subdev_compat_ioctl32(struct v4l2_subdev *sd, unsigned int cmd, unsigned long arg)
{
	void __user *p32 = compat_ptr(arg);
	void __user *new_p64 = NULL;
	//void __user *aux_buf;
	//u32 aux_space;
	long err = 0;
	const size_t ioc_size = _IOC_SIZE(cmd);
	//size_t ioc_size64 = 0;

	//if (_IOC_TYPE(cmd) == 'V') {
	//	switch (_IOC_NR(cmd)) {
	//		//int r
	//		case _IOC_NR(VIDIOC_G_SLICE_MODE):
	//		case _IOC_NR(VIDIOC_CPU_Z1):
	//		//int w
	//		case _IOC_NR(VIDIOC_PUT_PIPELINE):
	//		case _IOC_NR(VIDIOC_RESET_PIPELINE):
	//		ioc_size64 = sizeof(int);
	//		break;
	//		//unsigned int
	//		case _IOC_NR(VIDIOC_G_PIPE_STATUS):
	//		ioc_size64 = sizeof(int);
	//		break;
	//		case _IOC_NR(VIDIOC_S_PORT_CFG):
	//		ioc_size64 = sizeof(struct v4l2_vi_port_cfg);
	//		break;
	//		case _IOC_NR(VIDIOC_DBG_REG_WRITE):
	//		case _IOC_NR(VIDIOC_DBG_REG_READ):
	//		ioc_size64 = sizeof(struct v4l2_vi_dbg_reg);
	//		break;
	//		case _IOC_NR(VIDIOC_CFG_INPUT_INTF):
	//		ioc_size64 = sizeof(struct v4l2_vi_input_interface);
	//		break;
	//		case _IOC_NR(VIDIOC_SET_SELECTION):
	//		ioc_size64 = sizeof(struct v4l2_vi_selection);
	//		break;
	//		case _IOC_NR(VIDIOC_QUERY_SLICE_READY):
	//		ioc_size64 = sizeof(struct v4l2_vi_slice_info);
	//		break;
	//		case _IOC_NR(VIDIOC_S_BANDWIDTH):
	//		ioc_size64 = sizeof(struct v4l2_vi_bandwidth_info);
	//		break;
	//		case _IOC_NR(VIDIOC_G_ENTITY_INFO):
	//		ioc_size64 = sizeof(struct v4l2_vi_entity_info);
	//		break;
	//	}
	//	cam_dbg("%s cmd_nr=%d ioc_size32=%u ioc_size64=%u",__func__,  _IOC_NR(cmd), ioc_size, ioc_size64);
	//}
	if (_IOC_DIR(cmd) != _IOC_NONE) {
		err = alloc_userspace(ioc_size, 0, &new_p64);
		if (err) {
			cam_err("%s alloc userspace failed err=%l cmd=%d ioc_size=%u",
				__func__, err, _IOC_NR(cmd), ioc_size);
			return err;
		}
		if ((_IOC_DIR(cmd) & _IOC_WRITE)) {
			err = copy_in_user(new_p64, p32, ioc_size);
			if (err) {
				cam_err
				    ("%s copy in user 1 failed err=%l cmd=%d ioc_size=%u",
				     __func__, err, _IOC_NR(cmd), ioc_size);
				return err;
			}
		}
	}

	err = spm_subdev_video_usercopy(sd, cmd, (unsigned long)new_p64, spm_subdev_ioctl);
	if (err) {
		return err;
	}

	if ((_IOC_DIR(cmd) & _IOC_READ)) {
		err = copy_in_user(p32, new_p64, ioc_size);
		if (err) {
			cam_err("%s copy in user 2 failed err=%l cmd=%d ioc_size=%u",
				__func__, err, _IOC_NR(cmd), ioc_size);
			return err;
		}
	}
	//switch (cmd) {
	//	//int r
	//	case VIDIOC_G_SLICE_MODE:
	//	case VIDIOC_CPU_Z1:
	//	//int w 
	//	case VIDIOC_PUT_PIPELINE:
	//	case VIDIOC_RESET_PIPELINE:
	//	err = alloc_userspace(sizeof(int), 0, &new_p64);
	//	if (!err && assign_in_user((int __user *)new_p64,
	//				   (compat_int_t __user *)p32))
	//		err = -EFAULT;
	//	break;
	//	//unsigned int
	//	case VIDIOC_G_PIPE_STATUS:
	//	err = alloc_userspace(sizeof(unsigned int), 0, &new_p64);
	//	if (!err && assign_in_user((unsigned int __user *)new_p64,
	//				   (compat_uint_t __user *)p32))
	//		err = -EFAULT;
	//	break;
	//	case VIDIOC_S_PORT_CFG:
	//	err = alloc_userspace(sizeof(struct v4l2_vi_port_cfg), 0, &new_p64);
	//	if (!err) {
	//		err = -EFAULT;
	//		break;
	//	}
	//	break;
	//	case VIDIOC_DBG_REG_WRITE:
	//	case VIDIOC_DBG_REG_READ:
	//	break;
	//	case VIDIOC_CFG_INPUT_INTF:
	//	break;
	//	case VIDIOC_SET_SELECTION:
	//	break;
	//	case VIDIOC_QUERY_SLICE_READY:
	//	break;
	//	case VIDIOC_S_BANDWIDTH:
	//	break;
	//	case VIDIOC_G_ENTITY_INFO:
	//	break;
		
	//}	
	//if (err)
	//	return err;
	return 0;
}
#endif

static struct v4l2_subdev_format pad_fmts[SUBDEV_MAX_PADS];
int spm_subdev_reset(struct v4l2_subdev *sd, u32 val)
{
	int ret = 0;
	unsigned int i = 0;
	struct spm_camera_subdev *sc_subdev = v4l2_subdev_to_sc_subdev(sd);
	struct v4l2_subdev_format pad_fmt;
	struct media_pad *pad = NULL, *remote_pad = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;
	struct spm_camera_vbuffer *sc_vb = NULL;

	sc_subdev->is_resetting = 1;
	if (RESET_STAGE1 == val) {
		for (i = 0; i < sd->entity.num_pads; i++) {
			pad_fmts[i].pad = i;
			pad_fmts[i].which = V4L2_SUBDEV_FORMAT_ACTIVE;
			v4l2_subdev_call(sd, pad, get_fmt, NULL, &pad_fmts[i]);
		}
		for (i = 0; i < sd->entity.num_pads; i++) {
			v4l2_subdev_call(sd, pad, set_fmt, NULL, &pad_fmts[i]);
		}
	} else if (RESET_STAGE2 == val) {
		for (i = 0; i < sd->entity.num_pads; i++) {
			pad = sd->entity.pads + i;
			remote_pad = media_entity_remote_pad(pad);
			if (remote_pad) {
				sc_vnode = media_entity_to_sc_vnode(remote_pad->entity);
				if (sc_vnode) {
					while (0 == ret) {
						ret = spm_vdev_pick_busy_vbuffer(sc_vnode, &sc_vb);
						if (0 == ret && !(sc_vb->flags & SC_BUF_FLAG_SPECIAL_USE)) {
							ret = spm_vdev_dq_busy_vbuffer(sc_vnode, &sc_vb);
							if (0 == ret) {
								sc_vb->vb2_v4l2_buf.flags |= V4L2_BUF_FLAG_IGNOR;
								spm_vdev_export_camera_vbuffer(sc_vb, 0);
							}
						} else {
							ret = -1;
						}
					}
				}
			}
		}
		ret = v4l2_subdev_call(sd, video, s_stream, 1);
		if (ret && ret != -ENOIOCTLCMD) {
			cam_err("%s(%s) video stream on failed", __func__, sc_subdev->name);
			sc_subdev->is_resetting = 0;
			return ret;
		}
		for (i = 0; i < sd->entity.num_pads; i++) {
			if (sc_subdev->pads_stream_enable & (1 << i)) {
				pad_fmt.pad = i;
				pad_fmt.which = 1;
				ret = v4l2_subdev_call(sd, pad, link_validate, NULL, &pad_fmt, &pad_fmt);
				if (ret && ret != -ENOIOCTLCMD) {
					cam_err("%s(%s) stream on pad%u failed", __func__, sc_subdev->name, pad_fmt.pad);
					sc_subdev->is_resetting = 0;
					return ret;
				}
			}
		}
	}

	sc_subdev->is_resetting = 0;
	return 0;
}

int spm_subdev_pad_s_stream(struct spm_camera_subdev *sc_subdev, unsigned int pad, int enable)
{
	if (pad >= SUBDEV_MAX_PADS)
		return -EINVAL;

	if (enable)
		sc_subdev->pads_stream_enable |= (1 << pad);
	else
		sc_subdev->pads_stream_enable &= ~(1 << pad);

	return 0;
}

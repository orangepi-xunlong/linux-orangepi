// SPDX-License-Identifier: GPL-2.0
/*
 * mlink.h - media link functions
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _KY_MDEV_H_
#define _KY_MDEV_H_
#include <media/v4l2-device.h>
#include <media/media-device.h>
#include <media/v4l2-ctrls.h>
#include <cam_plat.h>

enum {
	PIPELINE_ST_IDLE = 0,
	PIPELINE_ST_GET,
	PIPELINE_ST_STOPPED = PIPELINE_ST_GET,
	PIPELINE_ST_STOPPING,
	PIPELINE_ST_STARTED,
};

#define sc_ispfirm_call(ispfirm_ops, f, args...)	\
	({						\
		int __result;				\
		if (!(ispfirm_ops))			\
			__result = -ENODEV;		\
		else if (!(ispfirm_ops)->f)		\
			__result = -ENOIOCTLCMD;	\
		else						\
			__result = (ispfirm_ops)->f(args);	\
		__result;					\
	})

#define sc_sensor_call(sensor_ops, f, args...)		\
	({						\
		int __result;				\
		if (!(sensor_ops))			\
			__result = -ENODEV;		\
		else if (!(sensor_ops)->f)		\
			__result = -ENOIOCTLCMD;	\
		else						\
			__result = (sensor_ops)->f(args);	\
		__result;					\
	})

#define MAX_PIPE_DOMAIN		(1)
#define PIPELINE_TYPE_SHIFT	(8)
#define PIPELINE_ID_MASK	((1 << PIPELINE_TYPE_SHIFT) - 1)
#define MAKE_SC_PIPELINE_ID(type, id)	(((type) << PIPELINE_TYPE_SHIFT) | (id))
#define PIPELINE_ID(sc_id)	((sc_id) & PIPELINE_ID_MASK)
#define PIPELINE_TYPE(sc_id)	((sc_id) >> PIPELINE_TYPE_SHIFT)
enum {
	PIPELINE_TYPE_SINGLE = 0,
	PIPELINE_TYPE_HDR,
};
struct spm_camera_pipeline {
	struct media_pipeline media_pipe;
	struct media_graph graph;
	int id;
	int state;
	int is_online_mode;
	int is_slice_mode;
	int slice_id;
	int total_slice_cnt;
	atomic_t slice_info_update;
	struct wait_queue_head slice_waitq;
	struct completion slice_done;
	int slice_result;
	struct list_head frame_id_list;
	spinlock_t slock;
	struct mutex mlock;
	unsigned int max_width[MAX_PIPE_DOMAIN];
	unsigned int max_height[MAX_PIPE_DOMAIN];
	unsigned int min_width[MAX_PIPE_DOMAIN];
	unsigned int min_height[MAX_PIPE_DOMAIN];
	struct blocking_notifier_head blocking_notify_chain;
	struct spm_camera_ispfirm_ops *ispfirm_ops;
	struct spm_camera_sensor_ops *sensor_ops;
	void *usr_data;
};

enum {
	SC_PIPE_NOTIFY_PRIO_NORMAL = 0,
	SC_PIPE_NOTIFY_PRIO_EMGER,
};

enum {
	PIPELINE_ACTION_SET_ENTITY_USRDATA = 0,
	PIPELINE_ACTION_GET_ENTITY_USRDATA,
	PIPELINE_ACTION_WAIT_EOF,
	PIPELINE_ACTION_CLEAN_USR_DATA,
	PIPELINE_ACTION_SENSOR_STREAM_ON,
	PIPELINE_ACTION_SENSOR_STREAM_OFF,
	PIPELINE_ACTION_SLICE_READY,
	PIPELINE_ACTION_CUSTOM_BASE = 1000,
};

struct entity_usrdata {
	unsigned int entity_id;
	void *usr_data;
};

#define media_pipeline_to_sc_pipeline(pipe)	((struct spm_camera_pipeline*)(pipe))

#define VNODE_PAD_IN	(0)
#define VNODE_PAD_OUT	(0)
#define PAD_IN	(0)
#define PAD_OUT	(1)

enum {
	RESET_STAGE1 = 0,
	RESET_STAGE2,
	RESET_STAGE3,
};

#define KY_MEDIA_CREATE_LINK(source, source_pad, sink, sink_pad)	\
	media_create_pad_link((struct media_entity*)(source), (source_pad), (struct media_entity*)(sink), (sink_pad), 0)

static inline int is_subdev(struct media_entity *me)
{
	return (me->obj_type == MEDIA_ENTITY_TYPE_V4L2_SUBDEV);
}

static inline int is_sensor(struct media_entity *me)
{
	return (me->obj_type == MEDIA_ENTITY_TYPE_V4L2_SUBDEV
		&& me->function == MEDIA_ENT_F_CAM_SENSOR);
}

static inline int is_link_source(struct media_entity *me, struct media_link *link)
{
	return (me == link->source->entity);
}

static inline int is_link_sink(struct media_entity *me, struct media_link *link)
{
	return (me == link->sink->entity);
}

static inline int is_link_enabled(struct media_link *link)
{
	return (link->flags & MEDIA_LNK_FL_ENABLED);
}

static inline int is_source_leaf(struct media_entity *me)
{
	struct media_link *link = NULL;

	list_for_each_entry(link, &me->links, list) {
		if (is_link_enabled(link) && is_link_sink(me, link))
			return 0;
	}

	return 1;
}

int spm_mlink_get_pipeline(struct media_pipeline *pipeline, struct media_entity *me);
int __spm_mlink_put_pipeline(struct media_entity *me, int auto_disable_link);
int spm_mlink_put_pipeline(struct media_entity *me, int auto_disable_link);
int spm_mlink_apply_pipeline(struct media_entity *me);
int spm_mlink_start_pipeline(struct media_entity *me);
int __spm_mlink_stop_pipeline(struct media_entity *me);
int spm_mlink_stop_pipeline(struct media_entity *me);
int spm_mlink_reset_pipeline(struct media_entity *me, int reset_stage);
int spm_mlink_pipeline_init(struct spm_camera_pipeline *sc_pipeline);
void spm_mlink_pipeline_release(struct spm_camera_pipeline *sc_pipeline);
const char *media_entity_name(struct media_entity *me);
struct media_entity *spm_mlink_find_sensor(struct media_entity *me);
struct media_pad *media_entity_remote_pad(const struct media_pad *pad);
#endif

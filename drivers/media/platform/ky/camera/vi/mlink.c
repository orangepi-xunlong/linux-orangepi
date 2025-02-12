// SPDX-License-Identifier: GPL-2.0
/*
 * mlink.c - media link functions
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include <media/media-entity.h>
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>
#include "mlink.h"
#include "subdev.h"
#include "vdev.h"

const char *media_entity_name(struct media_entity *me)
{
	struct spm_camera_subdev *sc_subdev = NULL;
	struct spm_camera_vnode *sc_vnode = NULL;

	if (is_subdev(me)) {
		sc_subdev = media_entity_to_sc_subdev(me);
		return sc_subdev->name;
	} else {
		sc_vnode = media_entity_to_sc_vnode(me);
		return sc_vnode->name;
	}

	return NULL;
}

static void spm_mlink_reset_pipeline_context(struct spm_camera_pipeline *sc_pipeline)
{
	blocking_notifier_call_chain(&(sc_pipeline->blocking_notify_chain),
				     PIPELINE_ACTION_CLEAN_USR_DATA, sc_pipeline);
	sc_pipeline->state = PIPELINE_ST_IDLE;
	sc_pipeline->is_online_mode = 0;
	sc_pipeline->is_slice_mode = 0;
	sc_pipeline->slice_id = 0;
	sc_pipeline->slice_result = 0;
	atomic_set(&sc_pipeline->slice_info_update, 0);
	INIT_LIST_HEAD(&sc_pipeline->frame_id_list);
	down_write(&sc_pipeline->blocking_notify_chain.rwsem);
	rcu_assign_pointer(sc_pipeline->blocking_notify_chain.head, NULL);
	up_write(&sc_pipeline->blocking_notify_chain.rwsem);
}

#define MEDIA_ENTITY_MAX_PADS	512
#define MAX_SENSORS		(16)
int spm_mlink_get_pipeline(struct media_pipeline *pipeline, struct media_entity *me)
{
	int ret = 0;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(me);
	struct media_pipeline *pipe_me = NULL;
	struct media_graph *graph = NULL;
	struct media_entity *error_entity = me;
	struct media_device *mdev = me->graph_obj.mdev;
	struct v4l2_subdev *sd = NULL;
	struct media_entity *sensors[MAX_SENSORS] = { NULL };
	int num_sensors = 0;
	int power_loop = 0;
	struct media_link *link = NULL;

	mutex_lock(&mdev->graph_mutex);
	if (NULL == pipe)
		pipe = pipeline;
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	if (sc_pipeline->state > PIPELINE_ST_GET) {
		cam_err("%s pipeline state(%d) error.", __func__, sc_pipeline->state);
		mutex_unlock(&mdev->graph_mutex);
		return -EPIPE;
	}
	graph = &sc_pipeline->graph;

	if (!pipe->start_count++) {
		ret = media_graph_walk_init(graph, mdev);
		if (ret)
			goto error_graph_walk_start;
	}
	media_graph_walk_start(graph, me);
	while ((me = media_graph_walk_next(graph))) {
		DECLARE_BITMAP(active, MEDIA_ENTITY_MAX_PADS);
		DECLARE_BITMAP(has_no_links, MEDIA_ENTITY_MAX_PADS);

		me->use_count++;
		pipe_me = media_entity_pipeline(me);
		if (WARN_ON(pipe_me && pipe_me != pipe)) {
			ret = -EBUSY;
			goto error;
		}
		me->pads[0].pipe = pipe;

		/* Already streaming --- no need to check. */
		if (me->use_count > 1)
			continue;

		if (is_subdev(me)) {
			if (!is_sensor(me)) {
				sd = media_entity_to_v4l2_subdev(me);
				ret = v4l2_subdev_call(sd, core, s_power, 1);
				if (ret && ret != -ENOIOCTLCMD) {
					cam_err("(%s) power 1 fail.", media_entity_name(me));
					goto error;
				}
			} else {
				if (num_sensors >= MAX_SENSORS) {
					cam_err("too many sensors entity.");
					BUG_ON(1);
				}
				sensors[num_sensors++] = me;
			}
		}

		if (!me->ops || !me->ops->link_validate)
			continue;

		bitmap_zero(active, me->num_pads);
		bitmap_fill(has_no_links, me->num_pads);

		list_for_each_entry(link, &me->links, list) {
			struct media_pad *pad = link->sink->entity == me ? link->sink : link->source;

			/* Mark that a pad is connected by a link. */
			bitmap_clear(has_no_links, pad->index, 1);

			/*
			 * Pads that either do not need to connect or
			 * are connected through an enabled link are
			 * fine.
			 */
			if (!(pad->flags & MEDIA_PAD_FL_MUST_CONNECT) ||
			    link->flags & MEDIA_LNK_FL_ENABLED)
				bitmap_set(active, pad->index, 1);

			/*
			 * Link validation will only take place for
			 * sink ends of the link that are enabled.
			 */
			if (link->sink != pad || !(link->flags & MEDIA_LNK_FL_ENABLED))
				continue;

			ret = me->ops->link_validate(link);
			if (ret < 0 && ret != -ENOIOCTLCMD) {
				cam_err("link validation failed for \"%s\":%u -> \"%s\":%u, error %d",
					media_entity_name(link->source->entity),
					link->source->index, media_entity_name(me),
					link->sink->index, ret);
				goto error;
			}
		}

		/* Either no links or validated links are fine. */
		bitmap_or(active, active, has_no_links, me->num_pads);

		if (!bitmap_full(active, me->num_pads)) {
			ret = -ENOLINK;
			cam_err("\"%s\":%u must be connected by an enabled link",
				media_entity_name(me),
				(unsigned)find_first_zero_bit(active, me->num_pads));
			goto error;
		}
	}

	for (power_loop = 0; power_loop < num_sensors; power_loop++) {
		sd = media_entity_to_v4l2_subdev(sensors[power_loop]);
		ret = v4l2_subdev_call(sd, core, s_power, 1);
		if (ret && ret != -ENOIOCTLCMD) {
			cam_err("(%s) power 1 failed.", media_entity_name(sensors[power_loop]));
			goto error;
		}
	}

	if (num_sensors > 0)
		sc_pipeline->is_online_mode = 1;
	else
		sc_pipeline->is_online_mode = 0;
	sc_pipeline->state = PIPELINE_ST_GET;
	mutex_unlock(&mdev->graph_mutex);

	return 0;
error:
	media_graph_walk_start(graph, error_entity);
	while (power_loop > 0) {
		sd = media_entity_to_v4l2_subdev(sensors[--power_loop]);
		v4l2_subdev_call(sd, core, s_power, 0);
	}
	while ((error_entity = media_graph_walk_next(graph))) {
		if (!WARN_ON_ONCE(error_entity->use_count <= 0)) {
			error_entity->use_count--;
			if (error_entity->use_count == 0) {
				if (is_subdev(error_entity)) {
					if (!is_sensor(error_entity)) {
						sd = media_entity_to_v4l2_subdev(error_entity);
						v4l2_subdev_call(sd, core, s_power, 0);
					}
				}
				error_entity->pads[0].pipe = NULL;
			}
		}

		/*
		 * We haven't increased use_count further than this
		 * so we quit here.
		 */
		if (error_entity == me)
			break;
	}
error_graph_walk_start:
	if (!--pipe->start_count) {
		spm_mlink_reset_pipeline_context(sc_pipeline);
		media_graph_walk_cleanup(graph);
	}
	mutex_unlock(&mdev->graph_mutex);

	return ret;
}

static int __spm_media_entity_setup_link_notify(struct media_link *link, u32 flags)
{
	int ret;

	/* Notify both entities. */
	ret = media_entity_call(link->source->entity, link_setup, link->source, link->sink, flags);
	if (ret < 0 && ret != -ENOIOCTLCMD)
		return ret;

	ret = media_entity_call(link->sink->entity, link_setup, link->sink, link->source, flags);
	if (ret < 0 && ret != -ENOIOCTLCMD) {
		media_entity_call(link->source->entity, link_setup,
				  link->source, link->sink, link->flags);
		return ret;
	}

	link->flags = flags;
	link->reverse->flags = link->flags;

	return 0;
}

static int __spm_media_entity_setup_link(struct media_link *link, u32 flags)
{
	const u32 mask = MEDIA_LNK_FL_ENABLED;
	struct media_device *mdev;
	struct media_entity *source, *sink;
	int ret = -EBUSY;

	if (link == NULL)
		return -EINVAL;

	/* The non-modifiable link flags must not be modified. */
	if ((link->flags & ~mask) != (flags & ~mask))
		return -EINVAL;

	if (link->flags & MEDIA_LNK_FL_IMMUTABLE)
		return link->flags == flags ? 0 : -EINVAL;

	if (link->flags == flags)
		return 0;

	source = link->source->entity;
	sink = link->sink->entity;
/*
	if (!(link->flags & MEDIA_LNK_FL_DYNAMIC) &&
		(source->stream_count || sink->stream_count))
		return -EBUSY;
*/
	mdev = source->graph_obj.mdev;

	if (mdev->ops && mdev->ops->link_notify) {
		ret = mdev->ops->link_notify(link, flags, MEDIA_DEV_NOTIFY_PRE_LINK_CH);
		if (ret < 0)
			return ret;
	}

	ret = __spm_media_entity_setup_link_notify(link, flags);

	if (mdev->ops && mdev->ops->link_notify)
		mdev->ops->link_notify(link, flags, MEDIA_DEV_NOTIFY_POST_LINK_CH);

	return ret;
}

int __spm_mlink_put_pipeline(struct media_entity *me, int auto_disable_link)
{
	struct media_graph *graph = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct v4l2_subdev *sd = NULL;
	struct media_link *link = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(me);

	cam_dbg("%s enter.", __func__);
	if (pipe) {
		sc_pipeline = container_of(pipe, struct spm_camera_pipeline, media_pipe);
		if (sc_pipeline->state > PIPELINE_ST_STOPPED) {
			cam_err("%s pipeline state(%d) error.", __func__, sc_pipeline->state);
			return -EPIPE;
		} else if (sc_pipeline->state <= PIPELINE_ST_IDLE) {
			return 0;
		}
		graph = &sc_pipeline->graph;
	} else {
		return 0;
	}

	media_graph_walk_start(graph, me);
	while ((me = media_graph_walk_next(graph))) {
		if (!WARN_ON_ONCE(me->use_count <= 0)) {
			me->use_count--;
			if (me->use_count == 0) {
				if (is_subdev(me)) {
					sd = media_entity_to_v4l2_subdev(me);
					v4l2_subdev_call(sd, core, s_power, 0);
				}
				if (auto_disable_link) {
					list_for_each_entry(link, &me->links, list) {
						if (link->flags & MEDIA_LNK_FL_ENABLED)
							__spm_media_entity_setup_link(link, link->flags & (~MEDIA_LNK_FL_ENABLED));
					}
				}
				me->pads[0].pipe = NULL;
			}
		}
	}
	if (!--pipe->start_count) {
		spm_mlink_reset_pipeline_context(sc_pipeline);
		media_graph_walk_cleanup(graph);
	}
	return 0;
}

int spm_mlink_put_pipeline(struct media_entity *me, int auto_disable_link)
{
	int ret = 0;
	struct media_device *mdev = me->graph_obj.mdev;

	mutex_lock(&mdev->graph_mutex);
	ret = __spm_mlink_put_pipeline(me, auto_disable_link);
	mutex_unlock(&mdev->graph_mutex);

	return ret;
}

static int spm_mlink_apply_format_forward(struct media_entity *me)
{
	struct media_link *link = NULL;
	struct media_entity *remote_me = NULL;
	struct v4l2_subdev *remote_sd = NULL, *sd = NULL;
	int source_pad_id = 0, sink_pad_id = 0;
	struct v4l2_subdev_format fmt;
	int ret = 0;

	if (!is_subdev(me))
		return 0;
	list_for_each_entry(link, &me->links, list) {
		if (!is_link_enabled(link))
			continue;
		if (is_link_sink(me, link))
			continue;
		source_pad_id = link->source->index;
		sink_pad_id = link->sink->index;
		sd = media_entity_to_v4l2_subdev(me);
		remote_me = link->sink->entity;
		if (!is_subdev(remote_me))
			continue;
		remote_sd = media_entity_to_v4l2_subdev(remote_me);
		fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
		fmt.pad = source_pad_id;
		fmt.stream = 0;

		ret = v4l2_subdev_call(sd, pad, get_fmt, NULL, &fmt);
		if (ret && ret != -ENOIOCTLCMD) {
			cam_err("%s get pad(%d) fmt from %s failed. (%d)", __func__,
				source_pad_id, media_entity_name(me), ret);

			return ret;
		}

		fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
		fmt.pad = sink_pad_id;
		fmt.stream = 0;
		cam_dbg("set format(%dx%d mbus_code=0x%08x) to %s pad%d.",
			fmt.format.width, fmt.format.height, fmt.format.code,
			media_entity_name(remote_me), fmt.pad);
		ret = v4l2_subdev_call(remote_sd, pad, set_fmt, NULL, &fmt);
		if (ret && ret != -ENOIOCTLCMD) {
			cam_err("%s set pad(%d) fmt(%dx%d code:0x%08x) to %s failed.",
				__func__, sink_pad_id, fmt.format.width,
				fmt.format.height, fmt.format.code,
				media_entity_name(remote_me));
			return ret;
		}
	}

	return 0;
}

struct media_ent_list_entry {
	struct list_head list_entry;
	struct media_entity *entity;
};

int spm_mlink_apply_pipeline(struct media_entity *me)
{
	int ret = 0, i = 0, idx_max = 0;
	struct media_entity *start_points[MAX_SENSORS];
	int num_points = 0;
	struct media_graph *graph = NULL;
	struct media_device *mdev = me->graph_obj.mdev;
	static struct media_ent_list_entry *entities = NULL;
	LIST_HEAD(entities_list);
	static unsigned long *visited = NULL;
	struct media_entity *next = NULL;
	struct media_ent_list_entry *media_ent = NULL;
	struct media_pad *remote_pad = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(me);
	struct media_link *link = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(me);

	mutex_lock(&mdev->graph_mutex);
	if (NULL == pipe) {
		cam_err("(%s) pipeline is null.", media_entity_name(me));
		ret = -EPIPE;
		goto apply_error;
	} else {
		sc_pipeline = container_of(pipe, struct spm_camera_pipeline, media_pipe);
		if (sc_pipeline->state == PIPELINE_ST_IDLE ||
			sc_pipeline->state >= PIPELINE_ST_STARTED) {
			cam_err("%s pipeline state(%d) error.", __func__, sc_pipeline->state);
			ret = -EPIPE;
			goto apply_error;
		}
	}
	graph = &sc_pipeline->graph;
	if (!sd->dev) {
		cam_err("%s(%s) dev is null", __func__, media_entity_name(me));
		ret = -ENODEV;
		goto apply_error;
	}
	if (!entities) {
		cam_dbg("entities idx max %d", graph->ent_enum.idx_max);
		entities = devm_kcalloc(sd->dev, graph->ent_enum.idx_max, sizeof(*entities), GFP_KERNEL);
		if (!entities) {
			cam_err("%s not enough mem for entities(%d max)", __func__, graph->ent_enum.idx_max);
			ret = -ENOMEM;
			goto apply_error;
		}
	}
	if (!visited) {
		idx_max = ALIGN(graph->ent_enum.idx_max, BITS_PER_LONG);
		visited = devm_kcalloc(sd->dev, idx_max / BITS_PER_LONG, sizeof(long), GFP_KERNEL);
		if (!visited) {
			cam_err("%s not enough mem for visited map", __func__);
			ret = -ENOMEM;
			goto apply_error;
		}
	}

	media_graph_walk_start(graph, me);

	while ((me = media_graph_walk_next(graph))) {
		if (me->internal_idx >= graph->ent_enum.idx_max) {
			cam_err("%s entity id(%d) exceeded max %d.", __func__,
				me->internal_idx, graph->ent_enum.idx_max - 1);
			ret = -EPIPE;
			goto apply_error;
		}
		entities[me->internal_idx].entity = me;
		INIT_LIST_HEAD(&(entities[me->internal_idx].list_entry));
		if (is_source_leaf(me)) {
			if (num_points >= MAX_SENSORS) {
				cam_err("too many start points entity.");
				ret = -EPIPE;
				goto apply_error;
			}
			//if source leaf is a vnode
			if (!is_subdev(me)) {
				BUG_ON(0 == me->num_pads);
				remote_pad = media_entity_remote_pad(&me->pads[0]);
				if (NULL == remote_pad) {
					cam_err("source leaf(%s) has no active links.", media_entity_name(me));
					goto apply_error;
				}
				me = remote_pad->entity;
			}
			start_points[num_points++] = me;
		}
	}
	if (num_points == 0) {
		cam_err("not found start points entity in pipeline.");
		goto apply_error;
	}
	for (i = 0; i < num_points; i++) {
		INIT_LIST_HEAD(&entities_list);
		bitmap_zero(visited, graph->ent_enum.idx_max);
		me = start_points[i];
		__set_bit(me->internal_idx, visited);
		ret = spm_mlink_apply_format_forward(me);
		if (ret)
			goto apply_error;
		list_add_tail(&entities[me->internal_idx].list_entry, &entities_list);
		while (!list_empty(&entities_list)) {
			media_ent = list_first_entry(&entities_list, struct media_ent_list_entry, list_entry);
			me = media_ent->entity;
			list_del_init(&media_ent->list_entry);
			list_for_each_entry(link, &me->links, list) {
				if (!is_link_enabled(link))
					continue;
				if (is_link_sink(me, link))
					continue;
				next = link->sink->entity;
				if (__test_and_set_bit(next->internal_idx, visited))
					continue;
				ret = spm_mlink_apply_format_forward(next);
				if (ret)
					goto apply_error;
				list_add_tail(&entities[next->internal_idx].list_entry, &entities_list);
			}
		}
	}
	mutex_unlock(&mdev->graph_mutex);
	return 0;
apply_error:
	mutex_unlock(&mdev->graph_mutex);
	return ret;
}

static int spm_mlink_stream_entity(struct media_entity *me, int stream_on)
{
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(me);

	return v4l2_subdev_call(sd, video, s_stream, stream_on);
}

static int spm_mlink_reset_entity(struct media_entity *me, int reset_stage)
{
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(me);

	return v4l2_subdev_call(sd, core, reset, reset_stage);
}

int spm_mlink_start_pipeline(struct media_entity *me)
{
	int ret = 0, i = 0;
	struct media_graph *graph = NULL;
	struct media_device *mdev = me->graph_obj.mdev;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(me);
	struct media_entity *sensors[MAX_SENSORS], *entity_error = me;
	int num_sensors = 0;

	cam_dbg("%s enter.", __func__);
	mutex_lock(&mdev->graph_mutex);

	if (NULL == pipe) {
		cam_err("(%s) pipeline is null.", media_entity_name(me));
		mutex_unlock(&mdev->graph_mutex);
		return -EPIPE;
	}
	sc_pipeline = container_of(pipe, struct spm_camera_pipeline, media_pipe);
	if (sc_pipeline->state >= PIPELINE_ST_STARTED) {
		mutex_unlock(&mdev->graph_mutex);
		return 0;
	} else if (sc_pipeline->state <= PIPELINE_ST_IDLE) {
		cam_err("%s pipeline state(%d) error.", __func__, sc_pipeline->state);
		mutex_unlock(&mdev->graph_mutex);
		return -EPIPE;
	}

	graph = &sc_pipeline->graph;
	media_graph_walk_start(graph, me);

	while ((me = media_graph_walk_next(graph))) {
		if (is_subdev(me)) {
			if (!is_sensor(me)) {
				ret = spm_mlink_stream_entity(me, 1);
				if (ret && ret != -ENOIOCTLCMD) {
					cam_err("%s start pipe(%s) failed.",
						__func__, media_entity_name(me));
					goto start_fail;
				}
			} else {
				if (num_sensors >= MAX_SENSORS) {
					cam_err("%s too many sensors.", __func__);
					BUG_ON(1);
				}
				sensors[num_sensors++] = me;
			}
		}
	}

	for (i = 0; i < num_sensors; i++) {
		ret = spm_mlink_stream_entity(sensors[i], 1);
		if (ret && ret != -ENOIOCTLCMD) {
			cam_err("%s start pipe(%s) failed.",
				__func__, media_entity_name(sensors[i]));
			goto start_fail;
		}
	}

	sc_pipeline->state = PIPELINE_ST_STARTED;
	mutex_unlock(&mdev->graph_mutex);
	return 0;
start_fail:
	media_graph_walk_start(graph, entity_error);

	while ((entity_error = media_graph_walk_next(graph))) {
		if (is_subdev(entity_error) && !is_sensor(entity_error)) {
			spm_mlink_stream_entity(entity_error, 0);
		}
		if (entity_error == me)
			break;
	}
	while (i > 0) {
		spm_mlink_stream_entity(sensors[--i], 0);
	}
	mutex_unlock(&mdev->graph_mutex);
	return ret;
}

int __spm_mlink_stop_pipeline(struct media_entity *me)
{
	struct media_graph *graph = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_entity *me_bak = me;
	struct media_pipeline *pipe = media_entity_pipeline(me);

	cam_dbg("%s enter.", __func__);

	if (NULL == pipe) {
		cam_err("(%s) pipeline is null.", media_entity_name(me));
		return -EPIPE;
	}
	sc_pipeline = container_of(pipe, struct spm_camera_pipeline, media_pipe);
	if (sc_pipeline->state <= PIPELINE_ST_STOPPED) {
		return 0;
	}

	sc_pipeline->state = PIPELINE_ST_STOPPING;
	graph = &sc_pipeline->graph;
	media_graph_walk_start(graph, me);

	while ((me = media_graph_walk_next(graph))) {
		if (is_subdev(me) && !is_sensor(me))
			spm_mlink_stream_entity(me, 0);
	}

	media_graph_walk_start(graph, me_bak);

	while ((me = media_graph_walk_next(graph))) {
		if (is_sensor(me)) {
			spm_mlink_stream_entity(me, 0);
		}
	}

	sc_pipeline->state = PIPELINE_ST_STOPPED;
	return 0;
}

int spm_mlink_stop_pipeline(struct media_entity *me)
{
	int ret = 0;
	struct media_device *mdev = me->graph_obj.mdev;

	mutex_lock(&mdev->graph_mutex);
	ret = __spm_mlink_stop_pipeline(me);
	mutex_unlock(&mdev->graph_mutex);

	return ret;
}

static int __spm_mlink_reset_pipeline(struct media_entity *me, int reset_stage)
{
	struct media_graph *graph = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;
	struct media_pipeline *pipe = media_entity_pipeline(me);
	struct media_entity *me_bak = me, *sensors[MAX_SENSORS];
	int ret = 0, num_sensors = 0;
	int reset_stages[VI_PIPE_RESET_STAGE_CNT] = {RESET_STAGE1, RESET_STAGE2, RESET_STAGE3};

	cam_dbg("%s enter", __func__);

	if (NULL == pipe) {
		cam_err("%s(%s) pipe was null", __func__, media_entity_name(me));
		return -EPIPE;
	}
	if (reset_stage < VI_PIPE_RESET_STAGE1 || reset_stage >= VI_PIPE_RESET_STAGE_CNT) {
		cam_err("%s(%s) invalid reset_stage(%d)", __func__, media_entity_name(me), reset_stage);
		return -EINVAL;
	}
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	if (sc_pipeline->state <= PIPELINE_ST_STOPPED) {
		return 0;
	}

	graph = &sc_pipeline->graph;
	media_graph_walk_start(graph, me);

	if (RESET_STAGE1 == reset_stages[reset_stage]) {
		while ((me = media_graph_walk_next(graph))) {
			if (is_sensor(me)) {
				ret = spm_mlink_reset_entity(me, RESET_STAGE1);
				if (ret) {
					cam_err("%s(%s) reset stage1 failed", __func__, media_entity_name(me));
					return -1;
				}
				if (num_sensors >= MAX_SENSORS) {
					cam_err("%s too many sensors", __func__);
					BUG_ON(1);
				}
				sensors[num_sensors++] = me;
			}
		}
		me = me_bak;
		media_graph_walk_start(graph, me);
		while ((me = media_graph_walk_next(graph))) {
			if (is_subdev(me) && !is_sensor(me)) {
				ret = spm_mlink_reset_entity(me, RESET_STAGE1);
				if (ret) {
					cam_err("%s(%s) reset stage1 failed", __func__, media_entity_name(me));
					return -1;
				}
			}
		}
	} else {
		while ((me = media_graph_walk_next(graph))) {
			if (is_sensor(me)) {
				if (num_sensors >= MAX_SENSORS) {
					cam_err("%s too many sensors", __func__);
					BUG_ON(1);
				}
				sensors[num_sensors++] = me;
			}
		}
		me = me_bak;
		media_graph_walk_start(graph, me);
		while ((me = media_graph_walk_next(graph))) {
			if (is_subdev(me) && !is_sensor(me)) {
				ret = spm_mlink_reset_entity(me, reset_stages[reset_stage]);
				if (ret) {
					cam_err("%s(%s) reset stage%d failed", __func__, media_entity_name(me), reset_stages[reset_stage] + 1);
					return -1;
				}
			}
		}
		while (num_sensors > 0) {
			ret = spm_mlink_reset_entity(sensors[--num_sensors], reset_stages[reset_stage]);
			if (ret) {
				cam_err("%s(%s) reset stage%d failed", __func__, media_entity_name(sensors[num_sensors]), reset_stages[reset_stage] + 1);
				return -1;
			}
		}
	}

	return 0;
}

int spm_mlink_reset_pipeline(struct media_entity *me, int reset_stage)
{
	int ret = 0;
	struct media_device *mdev = me->graph_obj.mdev;

	mutex_lock(&mdev->graph_mutex);
	ret = __spm_mlink_reset_pipeline(me, reset_stage);
	mutex_unlock(&mdev->graph_mutex);

	return ret;
}

int spm_mlink_pipeline_init(struct spm_camera_pipeline *sc_pipeline)
{
	sc_pipeline->state = PIPELINE_ST_IDLE;
	INIT_LIST_HEAD(&sc_pipeline->frame_id_list);
	spin_lock_init(&sc_pipeline->slock);
	mutex_init(&sc_pipeline->mlock);
	BLOCKING_INIT_NOTIFIER_HEAD(&sc_pipeline->blocking_notify_chain);
	init_waitqueue_head(&sc_pipeline->slice_waitq);
	atomic_set(&sc_pipeline->slice_info_update, 0);
	init_completion(&sc_pipeline->slice_done);
	return 0;
}

void spm_mlink_pipeline_release(struct spm_camera_pipeline *sc_pipeline)
{
	mutex_destroy(&sc_pipeline->mlock);
}

struct media_entity *spm_mlink_find_sensor(struct media_entity *me)
{
	struct media_pipeline *pipe = NULL;
	struct media_graph *graph = NULL;
	struct media_device *mdev = NULL;
	struct spm_camera_pipeline *sc_pipeline = NULL;

	if (!me)
		return NULL;
	pipe = media_entity_pipeline(me);
	if (!pipe)
		return NULL;
	mdev = me->graph_obj.mdev;
	sc_pipeline = media_pipeline_to_sc_pipeline(pipe);
	graph = &sc_pipeline->graph;
	mutex_lock(&mdev->graph_mutex);
	if (!pipe->start_count) {
		mutex_unlock(&mdev->graph_mutex);
		return NULL;
	}
	media_graph_walk_start(graph, me);
	while ((me = media_graph_walk_next(graph))) {
		if (is_sensor(me)) {
			mutex_unlock(&mdev->graph_mutex);
			return me;
		}
	}

	mutex_unlock(&mdev->graph_mutex);
	return NULL;
}

struct media_pad *media_entity_remote_pad(const struct media_pad *pad)
{
	struct media_link *link;

	list_for_each_entry(link, &pad->entity->links, list) {
		if (!(link->flags & MEDIA_LNK_FL_ENABLED))
			continue;

		if (link->source == pad)
			return link->sink;

		if (link->sink == pad)
			return link->source;
	}

	return NULL;
}

/*
 * vin_ispstat.c
 *
 */

#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <media/v4l2-event.h>

#include "../vin-video/vin_video.h"
#include "../vin-isp/sunxi_isp.h"
#include "vin_ispstat.h"

static void isp_stat_buf_clear(struct isp_stat *stat)
{
	int i;

	for (i = 0; i < STAT_MAX_BUFS; i++)
		stat->buf[i].empty = 1;
}

static struct ispstat_buffer *
__isp_stat_buf_find(struct isp_stat *stat, int look_empty)
{
	struct ispstat_buffer *found = NULL;
	int i;

	for (i = 0; i < STAT_MAX_BUFS; i++) {
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

		if (curr->empty) {
			found = curr;
			break;
		}

		if (!found ||
		    (s32)curr->frame_number - (s32)found->frame_number < 0)
			found = curr;
	}

	return found;
}

static inline struct ispstat_buffer *
isp_stat_buf_find_oldest(struct isp_stat *stat)
{
	return __isp_stat_buf_find(stat, 0);
}

static inline struct ispstat_buffer *
isp_stat_buf_find_oldest_or_empty(struct isp_stat *stat)
{
	return __isp_stat_buf_find(stat, 1);
}

static int isp_stat_buf_queue(struct isp_stat *stat)
{
	if (!stat->active_buf)
		return STAT_NO_BUF;

	ktime_get_ts(&stat->active_buf->ts);

	stat->active_buf->buf_size = stat->buf_size;

	stat->active_buf->config_counter = stat->config_counter;
	stat->active_buf->frame_number = stat->frame_number;
	stat->active_buf->empty = 0;
	stat->active_buf = NULL;

	return STAT_BUF_DONE;
}

/* Get next free buffer to write the statistics to and mark it active. */
static void isp_stat_buf_next(struct isp_stat *stat)
{
	if (unlikely(stat->active_buf)) {
		/* Overwriting unused active buffer */
		vin_log(VIN_LOG_STAT, "%s: new buffer requested without "
					"queuing active one.\n",
					stat->sd.name);
	} else {
		stat->active_buf = isp_stat_buf_find_oldest_or_empty(stat);
	}
}

static void isp_stat_buf_release(struct isp_stat *stat)
{
	unsigned long flags;

	spin_lock_irqsave(&stat->isp->slock, flags);
	stat->locked_buf = NULL;
	spin_unlock_irqrestore(&stat->isp->slock, flags);
}

/* Get buffer to userspace. */
static struct ispstat_buffer *isp_stat_buf_get(struct isp_stat *stat,
					       struct vin_isp_stat_data *data)
{
	int rval = 0, i = 0, *tmp_data;
	unsigned long flags;
	struct ispstat_buffer *buf;

	spin_lock_irqsave(&stat->isp->slock, flags);

	while (1) {
		buf = isp_stat_buf_find_oldest(stat);
		if (!buf) {
			spin_unlock_irqrestore(&stat->isp->slock, flags);
			vin_log(VIN_LOG_STAT, "%s: cannot find a buffer.\n",
				stat->sd.name);
			return ERR_PTR(-EBUSY);
		}
		break;
	}

	stat->locked_buf = buf;

	spin_unlock_irqrestore(&stat->isp->slock, flags);
	if (NULL != data) {
		if (buf->buf_size > data->buf_size) {
			vin_warn("%s: userspace's buffer size is not enough.\n",
					stat->sd.name);
			isp_stat_buf_release(stat);
			return ERR_PTR(-EINVAL);
		}
		tmp_data = (int *)buf->virt_addr;
		for (i = 0; i < buf->buf_size/(sizeof(int)); i++) {
			tmp_data[i] = 65535;
		}
		rval = copy_to_user(data->buf,
				    buf->virt_addr,
				    buf->buf_size);

		if (rval) {
			vin_print("%s: failed copying %d bytes of stat data\n",
					stat->sd.name, rval);
			buf = ERR_PTR(-EFAULT);
			isp_stat_buf_release(stat);
		}
	}
	return buf;
}

static void isp_stat_bufs_free(struct isp_stat *stat)
{
	int i;

	for (i = 0; i < STAT_MAX_BUFS; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];
		mm->size = stat->buf_alloc_size;

		if (!buf->virt_addr)
			continue;

		mm->vir_addr = buf->virt_addr;
		mm->phy_addr = buf->phy_addr;
		mm->dma_addr = buf->dma_addr;
		os_mem_free(mm);

		buf->dma_addr = NULL;
		buf->virt_addr = NULL;
		buf->phy_addr = NULL;
		buf->empty = 1;
	}

	vin_log(VIN_LOG_STAT, "%s: all buffers were freed.\n", stat->sd.name);

	stat->buf_alloc_size = 0;
	stat->active_buf = NULL;
}

static int isp_stat_bufs_alloc_dma(struct isp_stat *stat, unsigned int size)
{
	int i;

	stat->buf_alloc_size = size;

	for (i = 0; i < STAT_MAX_BUFS; i++) {
		struct ispstat_buffer *buf = &stat->buf[i];
		struct vin_mm *mm = &stat->ion_man[i];
		mm->size = stat->buf_alloc_size;
		if (!os_mem_alloc(mm)) {
			buf->virt_addr = mm->vir_addr;
			buf->phy_addr = mm->phy_addr;
			buf->dma_addr = mm->dma_addr;
		}
		if (!buf->virt_addr || !buf->dma_addr) {
			vin_print("%s: Can't acquire memory for DMA buffer %d\n",
				stat->sd.name, i);
			isp_stat_bufs_free(stat);
			return -ENOMEM;
		}
		buf->empty = 1;

		vin_log(VIN_LOG_STAT, "%s: buffer[%d] allocated."
			"dma_addr=0x%08lx virt_addr=0x%08lx\n",
			stat->sd.name, i, (unsigned long)buf->dma_addr,
			(unsigned long)buf->virt_addr);
	}
	return 0;
}

static int isp_stat_bufs_alloc(struct isp_stat *stat, u32 size)
{
	unsigned long flags;

	spin_lock_irqsave(&stat->isp->slock, flags);

	BUG_ON(stat->locked_buf != NULL);

	/* Are the old buffers big enough? */
	if (stat->buf_alloc_size >= size) {
		spin_unlock_irqrestore(&stat->isp->slock, flags);
		return 0;
	}

	if (stat->state != ISPSTAT_DISABLED || stat->buf_processing) {
		vin_print("%s: trying to allocate memory when busy\n",
			 stat->sd.name);
		spin_unlock_irqrestore(&stat->isp->slock, flags);
		return -EBUSY;
	}

	spin_unlock_irqrestore(&stat->isp->slock, flags);

	isp_stat_bufs_free(stat);

	return isp_stat_bufs_alloc_dma(stat, size);
}

static void isp_stat_queue_event(struct isp_stat *stat, int err)
{
	struct video_device *vdev = stat->sd.devnode;
	struct v4l2_event event;
	struct vin_isp_stat_event_status *status = (void *)event.u.data;

	memset(&event, 0, sizeof(event));
	if (!err) {
		status->frame_number = stat->frame_number;
		status->config_counter = stat->config_counter;
	} else {
		status->buf_err = 1;
	}

	event.type = stat->event_type;
	v4l2_event_queue(vdev, &event);
}

int vin_isp_stat_request_statistics(struct isp_stat *stat,
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

	data->ts.tv_sec = buf->ts.tv_sec;
	data->ts.tv_usec = buf->ts.tv_nsec / NSEC_PER_USEC;
	data->config_counter = buf->config_counter;
	data->frame_number = buf->frame_number;
	data->buf_size = buf->buf_size;

	buf->empty = 1;
	isp_stat_buf_release(stat);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

int vin_isp_stat_config(struct isp_stat *stat, void *new_conf)
{
	int ret;
	unsigned long irqflags;
	struct ispstat_generic_config *user_cfg = new_conf;
	u32 buf_size = user_cfg->buf_size;

	if (!new_conf) {
		vin_log(VIN_LOG_STAT, "%s: configuration is NULL\n", stat->sd.name);
		return -EINVAL;
	}

	mutex_lock(&stat->ioctl_lock);

	vin_print("%s: configuring module with buffer size=0x%08lx\n",
		stat->sd.name, (unsigned long)buf_size);

	ret = stat->ops->validate_params(stat, new_conf);
	if (ret) {
		mutex_unlock(&stat->ioctl_lock);
		vin_log(VIN_LOG_STAT, "%s: configuration values are invalid.\n",
			stat->sd.name);
		return ret;
	}

	if (buf_size != user_cfg->buf_size)
		vin_log(VIN_LOG_STAT, "%s: driver has corrected buffer size "
			"request to 0x%08lx\n", stat->sd.name,
			(unsigned long)user_cfg->buf_size);

	ret = isp_stat_bufs_alloc(stat, buf_size);
	if (ret) {
		mutex_unlock(&stat->ioctl_lock);
		return ret;
	}

	spin_lock_irqsave(&stat->isp->slock, irqflags);
	stat->ops->set_params(stat, new_conf);
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);

	/*
	 * Returning the right future config_counter for this setup, so
	 * userspace can *know* when it has been applied.
	 */
	user_cfg->config_counter = stat->config_counter + stat->inc_config;

	/* Module has a valid configuration. */
	stat->configured = 1;
	vin_log(VIN_LOG_STAT, "%s: module has been successfully configured.\n",
		stat->sd.name);

	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

static int isp_stat_buf_process(struct isp_stat *stat, int buf_state)
{
	int ret = STAT_NO_BUF;

	if (!atomic_add_unless(&stat->buf_err, -1, 0) &&
	    buf_state == STAT_BUF_DONE && stat->state == ISPSTAT_ENABLED) {
		ret = isp_stat_buf_queue(stat);
		isp_stat_buf_next(stat);
	}

	return ret;
}

int vin_isp_stat_pcr_busy(struct isp_stat *stat)
{
	return stat->ops->busy(stat);
}

int vin_isp_stat_busy(struct isp_stat *stat)
{
	return vin_isp_stat_pcr_busy(stat) | stat->buf_processing |
		(stat->state != ISPSTAT_DISABLED);
}

static void isp_stat_pcr_enable(struct isp_stat *stat, u8 pcr_enable)
{
	if ((stat->state != ISPSTAT_ENABLING &&
	     stat->state != ISPSTAT_ENABLED) && pcr_enable)
		/* Userspace has disabled the module. Aborting. */
		return;

	stat->ops->enable(stat, pcr_enable);
	if (stat->state == ISPSTAT_DISABLING && !pcr_enable)
		stat->state = ISPSTAT_DISABLED;
	else if (stat->state == ISPSTAT_ENABLING && pcr_enable)
		stat->state = ISPSTAT_ENABLED;
}

void vin_isp_stat_suspend(struct isp_stat *stat)
{
	unsigned long flags;

	spin_lock_irqsave(&stat->isp->slock, flags);

	if (stat->state != ISPSTAT_DISABLED)
		stat->ops->enable(stat, 0);
	if (stat->state == ISPSTAT_ENABLED)
		stat->state = ISPSTAT_SUSPENDED;

	spin_unlock_irqrestore(&stat->isp->slock, flags);
}

void vin_isp_stat_resume(struct isp_stat *stat)
{
	/* Module will be re-enabled with its pipeline */
	if (stat->state == ISPSTAT_SUSPENDED)
		stat->state = ISPSTAT_ENABLING;
}

static void isp_stat_try_enable(struct isp_stat *stat)
{
	unsigned long irqflags;

	if (stat->priv == NULL)
		/* driver wasn't initialised */
		return;

	spin_lock_irqsave(&stat->isp->slock, irqflags);
	if (stat->state == ISPSTAT_ENABLING && !stat->buf_processing &&
	    stat->buf_alloc_size) {
		/*
		 * Userspace's requested to enable the engine but it wasn't yet.
		 * Let's do that now.
		 */
		stat->update = 1;
		isp_stat_buf_next(stat);
		stat->ops->setup_regs(stat, stat->priv);

		isp_stat_pcr_enable(stat, 1);
		spin_unlock_irqrestore(&stat->isp->slock, irqflags);
		vin_log(VIN_LOG_STAT, "%s: module is enabled.\n",
			stat->sd.name);
	} else {
		spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	}
}

void vin_isp_stat_isr_frame_sync(struct isp_stat *stat)
{
	isp_stat_try_enable(stat);
}

void vin_isp_stat_overflow(struct isp_stat *stat)
{
	unsigned long irqflags;

	spin_lock_irqsave(&stat->isp->slock, irqflags);

	atomic_set(&stat->buf_err, 2);

	if (stat->recover_priv)
		stat->ovl_recover = 1;
	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
}

int vin_isp_stat_enable(struct isp_stat *stat, u8 enable)
{
	unsigned long irqflags;

	vin_print("%s: user wants to %s module.\n",
		stat->sd.name, enable ? "enable" : "disable");

	/* Prevent enabling while configuring */
	mutex_lock(&stat->ioctl_lock);

	spin_lock_irqsave(&stat->isp->slock, irqflags);

	if (!stat->configured && enable) {
		spin_unlock_irqrestore(&stat->isp->slock, irqflags);
		mutex_unlock(&stat->ioctl_lock);
		vin_log(VIN_LOG_STAT, "%s: cannot enable module as it's "
			"never been successfully configured so far.\n",
			stat->sd.name);
		return -EINVAL;
	}

	if (enable) {
		if (stat->state == ISPSTAT_DISABLING)
			/* Previous disabling request wasn't done yet */
			stat->state = ISPSTAT_ENABLED;
		else if (stat->state == ISPSTAT_DISABLED)
			/* Module is now being enabled */
			stat->state = ISPSTAT_ENABLING;
	} else {
		if (stat->state == ISPSTAT_ENABLING) {
			/* Previous enabling request wasn't done yet */
			stat->state = ISPSTAT_DISABLED;
		} else if (stat->state == ISPSTAT_ENABLED) {
			/* Module is now being disabled */
			stat->state = ISPSTAT_DISABLING;
			isp_stat_buf_clear(stat);
		}
	}

	spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	mutex_unlock(&stat->ioctl_lock);

	return 0;
}

int vin_isp_stat_s_stream(struct v4l2_subdev *subdev, int enable)
{
	struct isp_stat *stat = v4l2_get_subdevdata(subdev);
	vin_print("%s", __func__);

	if (enable) {
		isp_stat_try_enable(stat);
	} else {
		unsigned long flags;
		vin_isp_stat_enable(stat, 0);
		spin_lock_irqsave(&stat->isp->slock, flags);
		stat->ops->enable(stat, 0);
		spin_unlock_irqrestore(&stat->isp->slock, flags);

		vin_log(VIN_LOG_STAT, "%s: module is being disabled\n",
			stat->sd.name);
	}

	return 0;
}

static void __stat_isr(struct isp_stat *stat)
{
	int ret = STAT_BUF_DONE;
	int buf_processing;
	unsigned long irqflags;
	struct vin_pipeline *pipe;
	vin_log(VIN_LOG_STAT, "%s buf state is %d, frame number is %d "
		"0x%x %d %d %d %d %d\n", __func__,
		stat->state, stat->frame_number,
		stat->buf_size, stat->config_counter,
		stat->configured, stat->update,
		stat->buf_processing, stat->ovl_recover);

	spin_lock_irqsave(&stat->isp->slock, irqflags);
	if (stat->state == ISPSTAT_DISABLED) {
		spin_unlock_irqrestore(&stat->isp->slock, irqflags);
		return;
	}
	buf_processing = stat->buf_processing;
	stat->buf_processing = 1;
	stat->ops->enable(stat, 0);

	spin_unlock_irqrestore(&stat->isp->slock, irqflags);

	if (!vin_isp_stat_pcr_busy(stat)) {
		if (stat->ops->buf_process)
			/* Module still need to copy data to buffer. */
			ret = stat->ops->buf_process(stat);
		spin_lock_irqsave(&stat->isp->slock, irqflags);

		pipe = to_vin_pipeline(&stat->sd.entity);
		if (pipe == NULL) {
			vin_err("stat subdev is not in any pipeline!\n");
			return;
		}
		stat->frame_number = atomic_read(&pipe->frame_number);

		ret = isp_stat_buf_process(stat, ret);

		if (likely(!stat->ovl_recover)) {
			stat->ops->setup_regs(stat, stat->priv);
		} else {
			stat->update = 1;
			stat->ops->setup_regs(stat, stat->recover_priv);
			stat->ovl_recover = 0;
			/*
			 * Set 'update' in case of the module needs to use
			 * regular configuration after next buffer.
			 */
			stat->update = 1;
		}

		isp_stat_pcr_enable(stat, 1);
		spin_unlock_irqrestore(&stat->isp->slock, irqflags);
	} else {
		if (stat->ops->buf_process)
			atomic_set(&stat->buf_err, 1);

		ret = STAT_NO_BUF;
		vin_log(VIN_LOG_STAT, "%s: cannot process buffer, "
			"device is busy.\n", stat->sd.name);
	}

	stat->buf_processing = 0;
	isp_stat_queue_event(stat, ret != STAT_BUF_DONE);
}

void vin_isp_stat_isr(struct isp_stat *stat)
{
	__stat_isr(stat);
}

int vin_isp_stat_subscribe_event(struct v4l2_subdev *subdev,
				  struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	struct isp_stat *stat = v4l2_get_subdevdata(subdev);

	if (sub->type != stat->event_type)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, STAT_NEVENTS, NULL);
}

int vin_isp_stat_unsubscribe_event(struct v4l2_subdev *subdev,
				    struct v4l2_fh *fh,
				    struct v4l2_event_subscription *sub)
{
	return v4l2_event_unsubscribe(fh, sub);
}

static int isp_stat_init_entities(struct isp_stat *stat, const char *name,
				  const struct v4l2_subdev_ops *sd_ops)
{
	struct v4l2_subdev *subdev = &stat->sd;
	struct media_entity *me = &subdev->entity;

	v4l2_subdev_init(subdev, sd_ops);
	snprintf(subdev->name, V4L2_SUBDEV_NAME_SIZE, "sunxi_%s", name);
	subdev->grp_id = VIN_GRP_ID_STAT;
	subdev->flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	v4l2_set_subdevdata(subdev, stat);

	stat->pad.flags = MEDIA_PAD_FL_SINK;
	me->ops = NULL;

	return media_entity_init(me, 1, &stat->pad, 0);
}

int vin_isp_stat_init(struct isp_stat *stat, const char *name,
		       const struct v4l2_subdev_ops *sd_ops)
{
	int ret;
	vin_print("%s\n", __func__);
	stat->buf = kcalloc(STAT_MAX_BUFS, sizeof(*stat->buf), GFP_KERNEL);
	if (!stat->buf)
		return -ENOMEM;

	isp_stat_buf_clear(stat);
	mutex_init(&stat->ioctl_lock);
	atomic_set(&stat->buf_err, 0);

	ret = isp_stat_init_entities(stat, name, sd_ops);
	if (ret < 0) {
		mutex_destroy(&stat->ioctl_lock);
		kfree(stat->buf);
	}

	return ret;
}

void vin_isp_stat_cleanup(struct isp_stat *stat)
{
	media_entity_cleanup(&stat->sd.entity);
	mutex_destroy(&stat->ioctl_lock);
	isp_stat_bufs_free(stat);
	kfree(stat->buf);
	vin_print("%s", __func__);
}

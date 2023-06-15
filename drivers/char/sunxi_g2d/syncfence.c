/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2018 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*
 * This driver is modified from sw_sync, because we cannot
 * access sw_sync through debugfs after Android-11.
 */

#include <linux/dma-fence.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sync_file.h>
#include <linux/miscdevice.h>

/*
 * struct syncfence_create_data
 * @value:	the seqno to initialise the fence with
 * @name:	the name of the new sync point
 * @fence:	return the fd of the new sync_file with the created fence
 */
struct syncfence_create_data {
	__u32   value;
	char    name[32];
	__s32   fence;
};

#define SYNCFENCE_IOC_MAGIC	'Z'
#define SYNCFENCE_IOC_CREATE_FENCE \
	_IOWR(SYNCFENCE_IOC_MAGIC, 0, struct syncfence_create_data)
#define SYNCFENCE_IOC_INC _IOW(SYNCFENCE_IOC_MAGIC, 1, __u32)

struct fence_timeline {
	struct kref ref;
	char name[32];

	u64 context;
	int value;
	struct list_head pt_list;
	spinlock_t lock;

	struct list_head timeline_list;
};

struct syncfence {
	struct dma_fence base;
	struct list_head link;
};

static LIST_HEAD(timeline_list_head);
static DEFINE_SPINLOCK(timeline_list_lock);

static const struct dma_fence_ops timeline_fence_ops;

static inline struct syncfence *dma_fence_to_syncfence(struct dma_fence *fence)
{
	if (fence->ops != &timeline_fence_ops)
		return NULL;
	return container_of(fence, struct syncfence, base);
}

static inline struct fence_timeline *dma_fence_parent(struct dma_fence *fence)
{
	return container_of(fence->lock, struct fence_timeline, lock);
}

static struct fence_timeline *fence_timeline_create(const char *name)
{
	unsigned long flags;
	struct fence_timeline *timeline;

	timeline = kzalloc(sizeof(*timeline), GFP_KERNEL);
	if (!timeline)
		return NULL;

	kref_init(&timeline->ref);
	timeline->context = dma_fence_context_alloc(1);
	strlcpy(timeline->name, name, sizeof(timeline->name));
	INIT_LIST_HEAD(&timeline->pt_list);

	/* add the new timeline into timeline_list_head */
	spin_lock_irqsave(&timeline_list_lock, flags);
	list_add_tail(&timeline->timeline_list, &timeline_list_head);
	spin_unlock_irqrestore(&timeline_list_lock, flags);

	return timeline;
}

static void fence_timeline_free(struct kref *ref)
{
	unsigned long flags;
	struct fence_timeline *timeline =
		container_of(ref, struct fence_timeline, ref);

	/* remove it from timeline_list_head */
	spin_lock_irqsave(&timeline_list_lock, flags);
	list_del(&timeline->timeline_list);
	spin_unlock_irqrestore(&timeline_list_lock, flags);

	kfree(timeline);
}

static void fence_timeline_get(struct fence_timeline *timeline)
{
	kref_get(&timeline->ref);
}

static void fence_timeline_put(struct fence_timeline *timeline)
{
	kref_put(&timeline->ref, fence_timeline_free);
}

static const char *timeline_fence_get_driver_name(struct dma_fence *fence)
{
	return "syncfence";
}

static const char *timeline_fence_get_timeline_name(struct dma_fence *fence)
{
	struct fence_timeline *parent = dma_fence_parent(fence);
	return parent->name;
}

static void timeline_fence_release(struct dma_fence *fence)
{
	unsigned long flags;
	struct syncfence *sf = dma_fence_to_syncfence(fence);
	struct fence_timeline *parent = dma_fence_parent(fence);

	spin_lock_irqsave(fence->lock, flags);
	if (!list_empty(&sf->link))
		list_del(&sf->link);
	spin_unlock_irqrestore(fence->lock, flags);

	fence_timeline_put(parent);
	dma_fence_free(fence);
}

static bool timeline_fence_signaled(struct dma_fence *fence)
{
	struct fence_timeline *parent = dma_fence_parent(fence);
	return !__dma_fence_is_later(fence->seqno, parent->value, fence->ops);
}

static bool timeline_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static void timeline_fence_value_str(struct dma_fence *fence,
		char *str, int size)
{
	snprintf(str, size, "%lld", fence->seqno);
}

static void timeline_fence_timeline_value_str(struct dma_fence *fence,
		char *str, int size)
{
	struct fence_timeline *parent = dma_fence_parent(fence);
	snprintf(str, size, "%d", parent->value);
}

static const struct dma_fence_ops timeline_fence_ops = {
	.get_driver_name = timeline_fence_get_driver_name,
	.get_timeline_name = timeline_fence_get_timeline_name,
	.enable_signaling = timeline_fence_enable_signaling,
	.signaled = timeline_fence_signaled,
	.release = timeline_fence_release,
	.fence_value_str = timeline_fence_value_str,
	.timeline_value_str = timeline_fence_timeline_value_str,
};

static void fence_timeline_signal(struct fence_timeline *timeline, unsigned int inc)
{
	struct syncfence *pt, *next;

	spin_lock_irq(&timeline->lock);

	timeline->value += inc;

	list_for_each_entry_safe(pt, next, &timeline->pt_list, link) {
		if (!timeline_fence_signaled(&pt->base))
			continue;

		list_del_init(&pt->link);
		dma_fence_signal_locked(&pt->base);
	}

	spin_unlock_irq(&timeline->lock);
}

static struct syncfence *syncfence_create(struct fence_timeline *timeline,
		unsigned int value)
{
	struct syncfence *fence;

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence)
		return NULL;

	fence_timeline_get(timeline);
	dma_fence_init(&fence->base, &timeline_fence_ops, &timeline->lock,
			timeline->context, value);
	INIT_LIST_HEAD(&fence->link);

	spin_lock_irq(&timeline->lock);
	if (!dma_fence_is_signaled_locked(&fence->base)) {
		list_add_tail(&fence->link, &timeline->pt_list);
	}

	spin_unlock_irq(&timeline->lock);

	return fence;
}

static int syncfence_open(struct inode *inode, struct file *file)
{
	struct fence_timeline *timeline;
	char name[64];

	sprintf(name, "syncfence-%d", task_pid_nr(current));
	timeline = fence_timeline_create(name);
	if (!timeline)
		return -ENOMEM;

	file->private_data = timeline;
	return 0;
}

static int syncfence_release(struct inode *inode, struct file *file)
{
	struct fence_timeline *timeline = file->private_data;
	struct syncfence *fence, *next;

	spin_lock_irq(&timeline->lock);

	list_for_each_entry_safe(fence, next, &timeline->pt_list, link) {
		dma_fence_set_error(&fence->base, -ENOENT);
		dma_fence_signal_locked(&fence->base);
	}

	spin_unlock_irq(&timeline->lock);

	fence_timeline_put(timeline);
	return 0;
}

static long syncfence_ioctl_create_fence(struct fence_timeline *timeline,
		unsigned long arg)
{
	int fd = get_unused_fd_flags(O_CLOEXEC);
	int err;
	struct syncfence *fence;
	struct sync_file *sync_file;
	struct syncfence_create_data data;

	if (fd < 0)
		return fd;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
		err = -EFAULT;
		goto err;
	}

	fence = syncfence_create(timeline, data.value);
	if (!fence) {
		err = -ENOMEM;
		goto err;
	}

	sync_file = sync_file_create(&fence->base);
	dma_fence_put(&fence->base);
	if (!sync_file) {
		err = -ENOMEM;
		goto err;
	}

	data.fence = fd;
	if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
		fput(sync_file->file);
		err = -EFAULT;
		goto err;
	}

	fd_install(fd, sync_file->file);

	return 0;

err:
	put_unused_fd(fd);
	return err;
}

static long syncfence_ioctl_inc(struct fence_timeline *timeline, unsigned long arg)
{
	u32 value;

	if (copy_from_user(&value, (void __user *)arg, sizeof(value)))
		return -EFAULT;

	while (value > INT_MAX)  {
		fence_timeline_signal(timeline, INT_MAX);
		value -= INT_MAX;
	}

	fence_timeline_signal(timeline, value);

	return 0;
}

static long syncfence_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	struct fence_timeline *timeline = file->private_data;

	switch (cmd) {
	case SYNCFENCE_IOC_CREATE_FENCE:
		return syncfence_ioctl_create_fence(timeline, arg);

	case SYNCFENCE_IOC_INC:
		return syncfence_ioctl_inc(timeline, arg);

	default:
		return -ENOTTY;
	}
}

static ssize_t syncfence_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned long flags;
	ssize_t printed_count = 0;
	struct fence_timeline *timeline, *next_timeline;
	struct syncfence *pt, *next;

	spin_lock_irqsave(&timeline_list_lock, flags);

	list_for_each_entry_safe(timeline, next_timeline, &timeline_list_head, timeline_list) {
		printed_count += snprintf(buf+printed_count, PAGE_SIZE-printed_count,
				"\n+ timeline: %s [%d]:\n", timeline->name, timeline->value);

		spin_lock_irq(&timeline->lock);
		list_for_each_entry_safe(pt, next, &timeline->pt_list, link) {
			bool signaled = timeline_fence_signaled(&pt->base);

			printed_count += snprintf(buf+printed_count, PAGE_SIZE-printed_count,
					"\t%lld [%d]\n", pt->base.seqno, signaled);
		}
		spin_unlock_irq(&timeline->lock);
	}

	spin_unlock_irqrestore(&timeline_list_lock, flags);
	return printed_count;
}

static DEVICE_ATTR(debug, 0444, syncfence_debug_show, NULL);

static struct attribute *syncfence_attrs[] = {
	&dev_attr_debug.attr,
	NULL
};

static const struct attribute_group syncfence_attr_group = {
	.attrs = syncfence_attrs
};

static const struct attribute_group *syncfence_attr_groups[] = {
	&syncfence_attr_group,
	NULL
};

static struct file_operations syncfence_ops = {
	.owner          = THIS_MODULE,
	.open           = syncfence_open,
	.release        = syncfence_release,
	.unlocked_ioctl = syncfence_ioctl,
	.compat_ioctl   = syncfence_ioctl,
};

struct miscdevice syncfence_device = {
	.minor  = MISC_DYNAMIC_MINOR,
	.name   = "syncfence",
	.fops   = &syncfence_ops,
	.groups = syncfence_attr_groups,
};

int syncfence_init(void)
{
	int result = 0;
	result = misc_register(&syncfence_device);
	if (result)
		pr_err("Error %d adding syncfence", result);
	return 0;
}

void syncfence_exit(void)
{
	misc_deregister(&syncfence_device);
}


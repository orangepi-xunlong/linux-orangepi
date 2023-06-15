/*
 * Copyright (c) 2007-2018 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>

#include "gralloc_debug.h"
#include "sunxi_gralloc.h"
#include "gralloc_fops.h"
#include "gralloc_drv.h"

#define GRALLOC_MODULE_NAME "gralloc"
#define TAG "[GRALLOC]"

#define GRALLOC_VERSION_MAJOR 1
#define GRALLOC_VERSION_MINOR 0
#define GRALLOC_VERSION_PATCHLEVEL 0

struct gralloc_driver_data {
	dev_t devt;
	struct cdev *pcdev;
	struct class *pclass;
	struct device *pdev;

	//list of sunxi_gralloc_buffer
	struct mutex mlock;
	struct list_head buffer_head;
	int buffer_cnt;
};

enum buffer_flag {
	BUFFER_NONE = 0,
	BUFFER_REALLOCATE_ERROR = 1 << 0,
};

#define IMPORT_TASK_MAX 50
#define RELEASE_TASK_MAX 51
#define LOCK_PROCESS_MAX 20
struct sunxi_gralloc_buffer {
	char name[200];
	unsigned long long unique_id;
	unsigned int allocate_pid;

	unsigned int width;
	unsigned int height;
	unsigned int format;

	//0:No any import 1:import first time ...
	int import_count;
	unsigned int import_pid[IMPORT_TASK_MAX];

	int release_count;
	unsigned int release_pid[RELEASE_TASK_MAX];

	int lock_count;
	unsigned int lock_process_count;
	unsigned int lock_pid[LOCK_PROCESS_MAX];
	unsigned int unlock_process_count;
	unsigned int unlock_pid[LOCK_PROCESS_MAX];

	//reference to enum buffer_flag
	unsigned int flag;
	struct list_head node;
	int refcount;
};

static struct gralloc_driver_data *gralloc_drvdata;

int gralloc_allocate_buffer(void *data)
{
	struct gralloc_buffer *buf = (struct gralloc_buffer *)data;
	struct sunxi_gralloc_buffer *sunxi_buf = NULL;

	GRALLOC_INFO("%s\n", __func__);

	mutex_lock(&gralloc_drvdata->mlock);
	list_for_each_entry(sunxi_buf, &gralloc_drvdata->buffer_head, node) {
		if (sunxi_buf->unique_id == buf->unique_id) {
			sunxi_buf->flag |= BUFFER_REALLOCATE_ERROR;
			GRALLOC_ERR("re allocate the same buffer:%s id:%lld\n",
				sunxi_buf->name, sunxi_buf->unique_id);
			mutex_unlock(&gralloc_drvdata->mlock);
			return -1;
		}
	}
	mutex_unlock(&gralloc_drvdata->mlock);

	sunxi_buf = kzalloc(sizeof(struct sunxi_gralloc_buffer), GFP_KERNEL);
	strcpy(sunxi_buf->name, buf->name);
	sunxi_buf->unique_id = buf->unique_id;
	sunxi_buf->width = buf->width;
	sunxi_buf->height = buf->height;
	sunxi_buf->format = buf->format;
	sunxi_buf->refcount = 1;
	sunxi_buf->allocate_pid = current->pid;

	mutex_lock(&gralloc_drvdata->mlock);
	list_add(&sunxi_buf->node, &gralloc_drvdata->buffer_head);
	gralloc_drvdata->buffer_cnt++;
	mutex_unlock(&gralloc_drvdata->mlock);

	return 0;
}

int gralloc_import_buffer(void *data)
{
	struct gralloc_buffer *buf = (struct gralloc_buffer *)data;
	struct sunxi_gralloc_buffer *sunxi_buf = NULL;

	GRALLOC_INFO("%s\n", __func__);

	mutex_lock(&gralloc_drvdata->mlock);
	list_for_each_entry(sunxi_buf, &gralloc_drvdata->buffer_head, node) {
		if (sunxi_buf->unique_id == buf->unique_id) {
			sunxi_buf->refcount++;

			if ((sunxi_buf->import_count + 1) < IMPORT_TASK_MAX) {
				sunxi_buf->import_pid[sunxi_buf->import_count] = current->pid;
				sunxi_buf->import_count++;
			}
		}
	}
	mutex_unlock(&gralloc_drvdata->mlock);

	return 0;
}

int gralloc_release_buffer(void *data)
{
	struct gralloc_buffer *buf = (struct gralloc_buffer *)data;
	struct sunxi_gralloc_buffer *sunxi_buf = NULL;
	unsigned char find = 0;

	GRALLOC_INFO("%s \n", __func__);

	mutex_lock(&gralloc_drvdata->mlock);

	list_for_each_entry(sunxi_buf, &gralloc_drvdata->buffer_head, node) {
		if (sunxi_buf->unique_id == buf->unique_id) {
			find = 1;
			sunxi_buf->refcount--;

			if ((sunxi_buf->release_count + 1) < RELEASE_TASK_MAX) {
				sunxi_buf->release_pid[sunxi_buf->release_count] = current->pid;
				sunxi_buf->release_count++;
			}

			break;
		}
	}

	if (!find) {
		mutex_unlock(&gralloc_drvdata->mlock);
		return 0;
	}

	if (find && sunxi_buf
		&& (sunxi_buf->refcount <= 0)
		&& (sunxi_buf->lock_count <= 0)) {
		list_del(&sunxi_buf->node);
		gralloc_drvdata->buffer_cnt--;
		kfree(sunxi_buf);
	}
	mutex_unlock(&gralloc_drvdata->mlock);

	return 0;
}

int gralloc_lock_buffer(void *data)
{
	struct gralloc_buffer *buf = (struct gralloc_buffer *)data;
	struct sunxi_gralloc_buffer *sunxi_buf = NULL;
	int i;

	GRALLOC_INFO("%s\n", __func__);

	mutex_lock(&gralloc_drvdata->mlock);
	list_for_each_entry(sunxi_buf, &gralloc_drvdata->buffer_head, node) {
		if (sunxi_buf->unique_id == buf->unique_id) {
			sunxi_buf->lock_count++;

			for (i = 0; i < sunxi_buf->lock_process_count; i++) {
				if (sunxi_buf->lock_pid[i] == current->pid)
					break;
			}

			if ((sunxi_buf->lock_process_count + 1) < LOCK_PROCESS_MAX) {
				sunxi_buf->lock_pid[sunxi_buf->lock_process_count] = current->pid;
				sunxi_buf->lock_process_count++;
			}
		}
	}
	mutex_unlock(&gralloc_drvdata->mlock);

	return 0;
}

int gralloc_unlock_buffer(void *data)
{
	struct gralloc_buffer *buf = (struct gralloc_buffer *)data;
	struct sunxi_gralloc_buffer *sunxi_buf = NULL;
	int i;

	GRALLOC_INFO("%s\n", __func__);

	mutex_lock(&gralloc_drvdata->mlock);
	list_for_each_entry(sunxi_buf, &gralloc_drvdata->buffer_head, node) {
		if (sunxi_buf->unique_id == buf->unique_id) {
			sunxi_buf->lock_count--;

			for (i = 0; i < sunxi_buf->unlock_process_count; i++) {
				if (sunxi_buf->unlock_pid[i] == current->pid)
					break;
			}

			if ((sunxi_buf->unlock_process_count + 1) < LOCK_PROCESS_MAX) {
				sunxi_buf->unlock_pid[sunxi_buf->unlock_process_count] = current->pid;
				sunxi_buf->unlock_process_count++;
			}
		}
	}
	mutex_unlock(&gralloc_drvdata->mlock);

	return 0;
}

int gralloc_dump(char *buf)
{
	ssize_t n = 0;
	unsigned int i;

	struct sunxi_gralloc_buffer *sunxi_buf = NULL;

	n += sprintf(buf + n, "Total buffer count:%d\n\n", gralloc_drvdata->buffer_cnt);
	mutex_lock(&gralloc_drvdata->mlock);
	list_for_each_entry(sunxi_buf, &gralloc_drvdata->buffer_head, node) {
		n += sprintf(buf + n,
				"name:%s allocate_pid:%d unique_id:%llu %dx%d format:%d refcnt:%d flag:%x\n",
				sunxi_buf->name, sunxi_buf->allocate_pid, sunxi_buf->unique_id,
				sunxi_buf->width, sunxi_buf->height,
				sunxi_buf->format,
				sunxi_buf->refcount, sunxi_buf->flag);
	
		if (sunxi_buf->import_count) {
			n += sprintf(buf + n, "import number:%d ", sunxi_buf->import_count);
			n += sprintf(buf + n, "import task:\n");
			for (i = 0; i < sunxi_buf->import_count; i++) {
				n += sprintf(buf + n, "pid:%d\n", sunxi_buf->import_pid[i]);
			}
		}

		if (sunxi_buf->release_count) {
			n += sprintf(buf + n, "release number:%d ", sunxi_buf->release_count);
			n += sprintf(buf + n, "release task:\n");
			for (i = 0; i < sunxi_buf->release_count; i++) {
				n += sprintf(buf + n, "pid:%d\n", sunxi_buf->release_pid[i]);
			}
		}

		if (sunxi_buf->lock_count) {
			n += sprintf(buf + n, "lock number:%d ", sunxi_buf->lock_count);
			n += sprintf(buf + n, "lock task:\n");
			for (i = 0; i < sunxi_buf->lock_process_count; i++) {
				n += sprintf(buf + n, "pid:%d\n", sunxi_buf->lock_pid[i]);
			}

			if (sunxi_buf->unlock_process_count) {
				n += sprintf(buf + n, "unlock task:\n");
				for (i = 0; i < sunxi_buf->unlock_process_count; i++) {
					n += sprintf(buf + n, "pid:%d\n", sunxi_buf->unlock_pid[i]);
				}
			}
		}

		if (sunxi_buf->import_count
			|| sunxi_buf->release_count
			|| sunxi_buf->lock_count)
			n += sprintf(buf + n, "\n");
	}

	mutex_unlock(&gralloc_drvdata->mlock);

	return n;

}

static ssize_t
gralloc_device_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "\nNow the debug level is:%d\n", debug_mask);

	return n;
}

static ssize_t
gralloc_device_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	char *end;

	debug_mask = (int)simple_strtoull(buf, &end, 0);

	return count;
}

static DEVICE_ATTR(debug, S_IWUSR | S_IRUGO,
	gralloc_device_debug_show, gralloc_device_debug_store);


static ssize_t
gralloc_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return gralloc_dump(buf);
}

static ssize_t
gralloc_dump_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(dump, S_IWUSR | S_IRUGO,
	gralloc_dump_show, gralloc_dump_store);

static ssize_t
gralloc_kernel_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int i;

	struct sunxi_gralloc_buffer *sunxi_buf = NULL;

	printk("Total buffer count:%d\n\n", gralloc_drvdata->buffer_cnt);
	mutex_lock(&gralloc_drvdata->mlock);
	list_for_each_entry(sunxi_buf, &gralloc_drvdata->buffer_head, node) {
		printk("name:%s allocate_pid:%d unique_id:%llu %dx%d format:%d refcnt:%d flag:%x\n",
				sunxi_buf->name, sunxi_buf->allocate_pid,
				sunxi_buf->unique_id,
				sunxi_buf->width, sunxi_buf->height,
				sunxi_buf->format,
				sunxi_buf->refcount, sunxi_buf->flag);
	
		if (sunxi_buf->import_count) {
			printk("import number:%d ", sunxi_buf->import_count);
			printk("import task:\n");
			for (i = 0; i < sunxi_buf->import_count; i++) {
				printk("pid:%d\n", sunxi_buf->import_pid[i]);
			}
		}

		if (sunxi_buf->release_count) {
			printk("release number:%d ", sunxi_buf->release_count);
			printk("release task:\n");
			for (i = 0; i < sunxi_buf->release_count; i++) {
				printk("pid:%d\n", sunxi_buf->release_pid[i]);
			}
		}

		if (sunxi_buf->lock_count) {
			printk("lock number:%d ", sunxi_buf->lock_count);
			printk("lock task:\n");
			for (i = 0; i < sunxi_buf->lock_process_count; i++) {
				printk("pid:%d\n", sunxi_buf->lock_pid[i]);
			}

			if (sunxi_buf->unlock_process_count) {
				printk("unlock task:\n");
				for (i = 0; i < sunxi_buf->unlock_process_count; i++) {
					printk("pid:%d\n", sunxi_buf->unlock_pid[i]);
				}
			}
		}

		if (sunxi_buf->import_count
			|| sunxi_buf->release_count
			|| sunxi_buf->lock_count)
			printk("\n");
	}

	mutex_unlock(&gralloc_drvdata->mlock);

	return 0;
}

static ssize_t
gralloc_kernel_dump_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(kernel_dump, S_IWUSR | S_IRUGO,
	gralloc_kernel_dump_show, gralloc_kernel_dump_store);

static struct attribute *gralloc_device_attrs[] = {
	&dev_attr_debug.attr,
	&dev_attr_dump.attr,
	&dev_attr_kernel_dump.attr,
	NULL
};

static struct attribute_group gralloc_device_attr_group = {
	.attrs	= gralloc_device_attrs,
};

static const struct attribute_group *gralloc_device_attr_groups[] = {
	&gralloc_device_attr_group,
	NULL
};

/* parse and load resources of di device */
static int gralloc_parse_dt(struct platform_device *pdev,
	struct gralloc_driver_data *drvdata)
{
	return 0;
}

static int gralloc_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct gralloc_driver_data *drvdata = NULL;

	GRALLOC_INFO("%s start!\n", __func__);

	drvdata = kzalloc(sizeof(*drvdata), GFP_KERNEL);
	if (drvdata == NULL) {
		GRALLOC_ERR(TAG"kzalloc for drvdata failed\n");
		return -ENOMEM;
	}

	ret = gralloc_parse_dt(pdev, drvdata);
	if (ret)
		goto probe_done;


	mutex_init(&drvdata->mlock);
	INIT_LIST_HEAD(&drvdata->buffer_head);

	alloc_chrdev_region(&drvdata->devt, 0, 1, GRALLOC_MODULE_NAME);
	drvdata->pcdev = cdev_alloc();
	cdev_init(drvdata->pcdev, &gralloc_fops);
	drvdata->pcdev->owner = THIS_MODULE;
	ret = cdev_add(drvdata->pcdev, drvdata->devt, 1);
	if (ret) {
		GRALLOC_ERR(TAG"cdev add major(%d).\n", MAJOR(drvdata->devt));
		goto probe_done;
	}
	drvdata->pclass = class_create(THIS_MODULE, GRALLOC_MODULE_NAME);
	if (IS_ERR(drvdata->pclass)) {
		GRALLOC_ERR(TAG"create class error\n");
		ret = PTR_ERR(drvdata->pclass);
		goto probe_done;
	}

	drvdata->pdev = device_create_with_groups(
			drvdata->pclass,  NULL, drvdata->devt,
			NULL, gralloc_device_attr_groups,
			GRALLOC_MODULE_NAME);
	if (IS_ERR(drvdata->pdev)) {
		GRALLOC_ERR(TAG"device_create error\n");
		ret = PTR_ERR(drvdata->pdev);
		goto probe_done;
	}

	gralloc_drvdata = drvdata;
	platform_set_drvdata(pdev, (void *)drvdata);

	GRALLOC_INFO("gralloc probe finished!\n");
	return 0;

probe_done:
	if (ret) {
		kfree(drvdata);
		dev_err(&pdev->dev, "probe failed, errno %d!\n", ret);
	}

	return ret;

}

static int gralloc_remove(struct platform_device *pdev)
{
	kfree(gralloc_drvdata);
	return 0;
}

static int gralloc_suspend(struct device *dev)
{
	return 0;
}

static int gralloc_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops gralloc_pm_ops = {
	.suspend = gralloc_suspend,
	.resume = gralloc_resume,
};

static const struct of_device_id gralloc_dt_match[] = {
	{.compatible = "allwinner,sunxi-gralloc"},
	{},
};

static struct platform_driver gralloc_driver = {
	.probe = gralloc_probe,
	.remove = gralloc_remove,
	.driver = {
		.name = GRALLOC_MODULE_NAME,
		.owner = THIS_MODULE,
		.pm = &gralloc_pm_ops,
		.of_match_table = gralloc_dt_match,
	},
};

static int __init sunxi_gralloc_init(void)
{
	pr_info("Gralloc Module initialized.\n");
	return platform_driver_register(&gralloc_driver);
}

static void __exit sunxi_gralloc_exit(void)
{
	platform_driver_unregister(&gralloc_driver);
}
module_init(sunxi_gralloc_init);
module_exit(sunxi_gralloc_exit);

int debug_mask = DEBUG_LEVEL_ERR;
module_param_named(debug_mask, debug_mask, int, 0644);

MODULE_DEVICE_TABLE(of, gralloc_dt_match);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhengwanyu@allwinnertech.com");
MODULE_DESCRIPTION("Sunxi Gralloc");

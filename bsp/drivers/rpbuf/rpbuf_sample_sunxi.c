// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * (C) Copyright 2020-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Junyan Lin <junyanlin@allwinnertech.com>
 *
 * A sample driver to use RPBuf framework on Allwinner platform.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>

#include <linux/rpbuf.h>

#define SUNXI_RPBUF_SAMPLE_VERSION "1.1.1"

struct sunxi_rpbuf_sample_instance {
	struct rpbuf_controller *controller;
	char name[RPBUF_NAME_SIZE];
	int len;
	struct rpbuf_buffer *buffer;
	int write_offset;
};

static int sunxi_rpbuf_sample_alloc_payload_memory(struct rpbuf_buffer *buffer, void *priv)
{
	struct device *dev = priv;
	int ret;
	void *va;
	dma_addr_t pa;

	va = dma_alloc_coherent(dev, rpbuf_buffer_len(buffer), &pa, GFP_KERNEL);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "dma_alloc_coherent for len %d failed\n",
			rpbuf_buffer_len(buffer));
		ret = -ENOMEM;
		goto err_out;
	}
	dev_info(dev, "allocate payload memory: va 0x%pK, pa %pad, len %d\n",
		va, &pa, rpbuf_buffer_len(buffer));
	rpbuf_buffer_set_va(buffer, va);
	rpbuf_buffer_set_pa(buffer, (phys_addr_t)pa);

	return 0;
err_out:
	return ret;
}

static void sunxi_rpbuf_sample_free_payload_memory(struct rpbuf_buffer *buffer, void *priv)
{

	struct device *dev = priv;
	void *va;
	dma_addr_t pa;

	va = rpbuf_buffer_va(buffer);
	pa = (dma_addr_t)(rpbuf_buffer_pa(buffer));

	dev_info(dev, "free payload memory: va %pK, pa %pad, len %d\n",
		va, &pa, rpbuf_buffer_len(buffer));
	dma_free_coherent(dev, rpbuf_buffer_len(buffer), va, pa);
}

/*
 * Optional:
 *   We can define our own operations to overwrite the default ones.
 */
static const struct rpbuf_buffer_ops sunxi_rpbuf_sample_ops = {
	.alloc_payload_memory = sunxi_rpbuf_sample_alloc_payload_memory,
	.free_payload_memory = sunxi_rpbuf_sample_free_payload_memory,
};

static void sunxi_rpbuf_sample_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printk("buffer \"%s\" is available\n", rpbuf_buffer_name(buffer));
}

static int sunxi_rpbuf_sample_rx_cb(struct rpbuf_buffer *buffer,
				    void *data, int data_len, void *priv)
{
	int i;

	printk("buffer \"%s\" received data (offset: %ld, len: %d):\n",
	       rpbuf_buffer_name(buffer), data - rpbuf_buffer_va(buffer), data_len);
	for (i = 0; i < data_len; ++i) {
		printk(" 0x%x,", *((u8 *)(data) + i));
	}
	printk("\n");

	return 0;
}

static int sunxi_rpbuf_sample_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	printk("buffer \"%s\": remote buffer destroyed\n", rpbuf_buffer_name(buffer));

	return 0;
}

static const struct rpbuf_buffer_cbs sunxi_rpbuf_sample_cbs = {
	.available_cb = sunxi_rpbuf_sample_available_cb,
	.rx_cb = sunxi_rpbuf_sample_rx_cb,
	.destroyed_cb = sunxi_rpbuf_sample_destroyed_cb,
};

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", inst->name);
}

static ssize_t name_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);
	size_t name_len;
	int i;

	for (i = 0; i < count; i++) {
		if (buf[i] == '\n' || buf[i] == '\0') {
			break;
		}
	}
	if (i == 0 || i >= RPBUF_NAME_SIZE) {
		dev_err(dev, "invalid name \"%s\", its length should be in "
			"range [1, %d)\n", buf, RPBUF_NAME_SIZE);
		return -EINVAL;
	}
	name_len = i;

	strncpy(inst->name, buf, name_len);
	inst->name[name_len] = '\0';

	return count;
}
static DEVICE_ATTR_RW(name);

static ssize_t len_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", inst->len);
}

static ssize_t len_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);
	int ret, value;

	ret = sscanf(buf, "%d\n", &value);
	if (ret <= 0) {
		dev_err(dev, "invalid value\n");
		return -EINVAL;
	}
	inst->len = value;
	return count;
}
static DEVICE_ATTR_RW(len);

static ssize_t state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);
	struct rpbuf_buffer *buffer = inst->buffer;

	if (buffer) {
		return sprintf(buf, "created\n");
	} else {
		return sprintf(buf, "not created\n");
	}
}

static ssize_t state_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);
	struct rpbuf_controller *controller = inst->controller;
	struct rpbuf_buffer *buffer = inst->buffer;
	int ret;

	if (sysfs_streq(buf, "create")) {
		if (buffer) {
			dev_err(dev, "buffer has been created\n");
			return -EBUSY;
		}

		buffer = rpbuf_alloc_buffer(controller, inst->name, inst->len,
					    &sunxi_rpbuf_sample_ops,
					    &sunxi_rpbuf_sample_cbs,
					    (void *)dev);
		if (!buffer) {
			dev_err(dev, "rpbuf_alloc_buffer failed\n");
			return -ENOENT;
		}
		inst->buffer = buffer;

	} else if (sysfs_streq(buf, "destroy")) {
		if (!buffer) {
			dev_err(dev, "buffer not created\n");
			return -ENOENT;
		}

		ret = rpbuf_free_buffer(buffer);
		if (ret < 0) {
			dev_err(dev, "rpbuf_free_buffer failed\n");
			return ret;
		}
		inst->buffer = NULL;

	} else {
		dev_err(dev, "Unexpected option: %s\n", buf);
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR_RW(state);

static ssize_t data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);
	struct rpbuf_buffer *buffer = inst->buffer;
	const char *buf_name;
	void *buf_va;
	int buf_len;

	if (!buffer) {
		dev_err(dev, "buffer \"%s\" not created\n", inst->name);
		return -ENOENT;
	}
	buf_name = rpbuf_buffer_name(buffer);
	buf_va = rpbuf_buffer_va(buffer);
	buf_len = rpbuf_buffer_len(buffer);

	if (!rpbuf_buffer_is_available(buffer)) {
		dev_err(dev, "buffer \"%s\" not available\n", buf_name);
		return -EACCES;
	}

	memcpy(buf, buf_va, buf_len);
	return buf_len;
}

static ssize_t data_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);
	struct rpbuf_buffer *buffer = inst->buffer;
	const char *buf_name;
	void *buf_va;
	int buf_len;
	int data_len = count - 1; /* exclude trailing LF */
	int offset = inst->write_offset;
	int ret;

	if (!buffer) {
		dev_err(dev, "buffer \"%s\" not created\n", inst->name);
		return -ENOENT;
	}
	buf_name = rpbuf_buffer_name(buffer);
	buf_va = rpbuf_buffer_va(buffer);
	buf_len = rpbuf_buffer_len(buffer);

	if (buf_len < data_len) {
		dev_err(dev, "buffer \"%s\": data size (%d) out of buffer length (%d)\n",
			buf_name, data_len, buf_len);
		return -EINVAL;
	}

	if (!rpbuf_buffer_is_available(buffer)) {
		dev_err(dev, "buffer \"%s\" not available\n", buf_name);
		return -EACCES;
	}

	memcpy(buf_va + offset, buf, data_len);

	ret = rpbuf_transmit_buffer(buffer, offset, data_len);
	if (ret < 0) {
		dev_err(dev, "buffer \"%s\": rpbuf_transmit_buffer failed\n", buf_name);
		return ret;
	}

	return count;
}
static DEVICE_ATTR_RW(data);

static ssize_t write_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", inst->write_offset);
}

static ssize_t write_offset_store(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);
	int ret, value;

	ret = sscanf(buf, "%d\n", &value);
	if (ret <= 0) {
		dev_err(dev, "invalid value\n");
		return -EINVAL;
	}
	inst->write_offset = value;
	return count;
}
static DEVICE_ATTR_RW(write_offset);

static struct attribute *sunxi_rpbuf_sample_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_len.attr,
	&dev_attr_state.attr,
	&dev_attr_data.attr,
	&dev_attr_write_offset.attr,
	NULL,
};

static struct attribute_group sunxi_rpbuf_sample_attr_group = {
	.name	= "control",
	.attrs	= sunxi_rpbuf_sample_attrs,
};

static int sunxi_rpbuf_sample_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct sunxi_rpbuf_sample_instance *inst;
	struct rpbuf_controller *controller;
	struct device_node *tmp_np;

	inst = devm_kzalloc(dev, sizeof(struct sunxi_rpbuf_sample_instance), GFP_KERNEL);
	if (!inst) {
		ret = -ENOMEM;
		goto err_out;
	}

	/*
	 * Optional:
	 *   We can define our own "memory-region" in DTS to use specific reserved
	 *   memory in our own alloc_payload_memory/free_payload_memory operations.
	 */
	tmp_np = of_parse_phandle(np, "memory-region", 0);
	if (tmp_np) {
		ret = of_reserved_mem_device_init(dev);
		if (ret < 0) {
			dev_err(dev, "failed to get reserved memory (ret: %d)\n", ret);
			goto err_out;
		}
	}

	controller = rpbuf_get_controller_by_of_node(np, 0);
	if (!controller) {
		dev_err(dev, "cannot get rpbuf controller\n");
		ret = -ENOENT;
		goto err_free_inst;
	}
	inst->controller = controller;

	/*
	 * default value
	 */
	strncpy(inst->name, "sample", RPBUF_NAME_SIZE);
	inst->len = 32;
	inst->write_offset = 0;

	dev_set_drvdata(dev, inst);

	ret = sysfs_create_group(&pdev->dev.kobj, &sunxi_rpbuf_sample_attr_group);
	if (ret < 0) {
		dev_err(dev, "failed to create attr group\n");
		goto err_free_inst;
	}

	return 0;

err_free_inst:
	devm_kfree(dev, inst);
err_out:
	return ret;
}

static int sunxi_rpbuf_sample_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_rpbuf_sample_instance *inst = dev_get_drvdata(dev);

	sysfs_remove_group(&pdev->dev.kobj, &sunxi_rpbuf_sample_attr_group);

	devm_kfree(dev, inst);

	return 0;
}

static const struct of_device_id sunxi_rpbuf_sample_ids[] = {
	{ .compatible = "allwinner,rpbuf-sample" },
	{}
};

static struct platform_driver sunxi_rpbuf_sample_driver = {
	.probe	= sunxi_rpbuf_sample_probe,
	.remove	= sunxi_rpbuf_sample_remove,
	.driver	= {
		.owner = THIS_MODULE,
		.name = "sunxi-rpbuf-sample",
		.of_match_table = sunxi_rpbuf_sample_ids,
	},
};
module_platform_driver(sunxi_rpbuf_sample_driver);

MODULE_DESCRIPTION("Allwinner RPBuf sample driver");
MODULE_AUTHOR("Junyan Lin <junyanlin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPBUF_SAMPLE_VERSION);

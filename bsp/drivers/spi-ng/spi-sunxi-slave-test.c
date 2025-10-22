// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2022 Allwinner.
 *
 * SUNXI SPI Slave test Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * 2022.09.30 JingyanLiang <jingyanliang@allwinnertech.com>
 * Create driver file
 *
 * assuming /dev/spidev1.0 corresponds to the SPI master on the remote
 * system and /sys/class/spi_slave/spi2/spi2.0/ corresponds to the SPI
 * Slave on the current system
 *
 * Usage for master command :
 *   # ./spislave-test -D /dev/spidev1.0 -R -s 10000000 -a 0 -S 2048
 *
 * Dump the slave data in memroy :
 *   # hexdump /sys/class/spi_slave/spi2/spi2.0/sunxi-slave-test
 *
 * Abort the slave transfer in user space manually :
 *  # echo > /sys/class/spi_slave/spi2/spi2.0/abort
 */

#include <sunxi-log.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/spi/spi.h>

#include "spi-sunxi-debug.h"

#define SALVE_CACHE_MAX (16 * PAGE_SIZE)

#define PKT_HEAD_LEN 6
#define OP_MASK		0
#define BITS_MASK	1
#define ADDR_MASK_0	2
#define ADDR_MASK_1	3
#define LEN_MASK_0	4
#define LEN_MASK_1	5

#define SUNXI_OP_WR		0x01
#define SUNXI_OP_RD		0x02
#define SUNXI_OP_WR_RD	0x03
#define SUNXI_OP_HEAD	0xff /* Only used in kernel space */

enum sunxi_spi_slave_status {
	SUNXI_SPI_SLAVE_IDLE = 0,
	SUNXI_SPI_SLAVE_RUNNING,
	SUNXI_SPI_SLAVE_RETRY,
	SUNXI_SPI_SLAVE_STOP,
};

struct sunxi_spi_slave_head {
	u8 op_code;
	u8 bits;
	u16 addr;
	u16 len;
};

struct sunxi_spi_slave_frame {
	u8 data[PKT_HEAD_LEN];
	struct sunxi_spi_slave_head pkt_head;
	u8 *tx_buf;
	u8 *rx_buf;
};

struct sunxi_spi_slave_cache {
	spinlock_t buffer_lock; /* cache buffer access lock */
	u8 *buffer;
	u32 size;
};

struct sunxi_spi_slave_test {
	struct spi_device *spi;
	struct completion finished;
	struct spi_transfer xfer;
	struct spi_message msg;
	struct sunxi_spi_slave_frame frame;
	struct sunxi_spi_slave_cache cache;
	struct bin_attribute bin;
	enum sunxi_spi_slave_status status;
};

static int sunxi_spi_slave_test_submit(struct sunxi_spi_slave_test *slave);

static bool sunxi_spi_slave_has_ptk_head(struct sunxi_spi_slave_head *head)
{
	if (head->op_code || head->bits || head->addr || head->len)
		return true;

	return false;
}

static void sunxi_spi_slave_head_data_parse(unsigned char *data, struct sunxi_spi_slave_head *head)
{
	head->op_code = data[OP_MASK];
	head->bits = data[BITS_MASK];
	head->addr = (data[ADDR_MASK_0] << 8) | data[ADDR_MASK_1];
	head->len = (data[LEN_MASK_0] << 8) | data[LEN_MASK_1];
}

static void sunxi_spi_slave_head_data_clear(unsigned char *data, int len)
{
	memset(data, 0, len);
}

static int sunxi_spi_slave_set_cache_data(struct sunxi_spi_slave_test *slave,
									struct sunxi_spi_slave_head *head, u8 *buf)
{
	struct device *dev = &slave->spi->dev;
	struct sunxi_spi_slave_cache *cache = &slave->cache;
	int real_size = head->len;

	if (cache->size < head->addr) {
		sunxi_err(dev, "Set data addr over range\n");
		return 0;
	}

	if (cache->size < head->addr + head->len) {
		real_size = cache->size - head->addr;
		sunxi_err(dev, "Write size %d over range, some of data will be lost, real size to write is %d\n",
				head->len, real_size);
	}

	spin_lock(&cache->buffer_lock);
	memcpy(cache->buffer + head->addr, buf, real_size);
	spin_unlock(&cache->buffer_lock);

	return 0;
}

static int sunxi_spi_slave_get_cache_data(struct sunxi_spi_slave_test *slave,
										struct sunxi_spi_slave_head *head, u8 *buf)
{
	struct device *dev = &slave->spi->dev;
	struct sunxi_spi_slave_cache *cache = &slave->cache;
	int real_size = head->len;

	if (cache->size < head->addr) {
		sunxi_err(dev, "Get data addr over range\n");
		return 0;
	}

	if (cache->size < head->addr + head->len) {
		real_size = cache->size - head->addr;
		sunxi_err(dev, "Read size %d over range, some of data will be lost, real size to read is %d\n",
			head->len, real_size);
	}

	spin_lock(&cache->buffer_lock);
	memcpy(buf, cache->buffer + head->addr, real_size);
	spin_unlock(&cache->buffer_lock);

	return 0;
}

static void sunxi_spi_slave_test_complete(void *arg)
{
	struct sunxi_spi_slave_test *slave = (struct sunxi_spi_slave_test *)arg;
	struct device *dev = &slave->spi->dev;
	struct sunxi_spi_slave_head *pkt_head = &slave->frame.pkt_head;
	int ret;

	ret = slave->msg.status;
	if (ret) {
		switch (slave->status) {
		case SUNXI_SPI_SLAVE_RETRY:
			sunxi_info(dev, "slave transfer retry\n");
			sunxi_spi_slave_head_data_clear(slave->frame.data, sizeof(slave->frame.data));
			goto retry;
			break;
		case SUNXI_SPI_SLAVE_STOP:
			sunxi_info(dev, "slave transfer stop\n");
			goto terminate;
			break;
		case SUNXI_SPI_SLAVE_IDLE:
		case SUNXI_SPI_SLAVE_RUNNING:
		default:
			break;
		}
	}

	switch (pkt_head->op_code) {
	case SUNXI_OP_HEAD:
		sunxi_debug(dev, "sunxi slave get package head\n");
		break;
	case SUNXI_OP_WR:
		sunxi_debug(dev, "slave recv data from master done\n");
		sunxi_spi_slave_set_cache_data(slave, pkt_head, slave->xfer.rx_buf);
		devm_kfree(dev, slave->xfer.rx_buf);
		slave->xfer.rx_buf = NULL;
		slave->frame.rx_buf = NULL;
		break;
	case SUNXI_OP_RD:
		sunxi_debug(dev, "slave send data to master done\n");
		devm_kfree(dev, slave->xfer.tx_buf);
		slave->xfer.tx_buf = NULL;
		slave->frame.tx_buf = NULL;
		break;
	case SUNXI_OP_WR_RD:
		sunxi_debug(dev, "slave recv/send data from/to master done\n");
		sunxi_spi_slave_set_cache_data(slave, pkt_head, slave->xfer.rx_buf);
		devm_kfree(dev, slave->xfer.rx_buf);
		slave->xfer.rx_buf = NULL;
		slave->frame.rx_buf = NULL;
		devm_kfree(dev, slave->xfer.tx_buf);
		slave->xfer.tx_buf = NULL;
		slave->frame.tx_buf = NULL;
		break;
	default:
		sunxi_debug(dev, "sunxi slave get op_code filed\n");
		sunxi_spi_slave_head_data_clear(slave->frame.data, sizeof(slave->frame.data));
		break;
	}

retry:
	spi_transfer_del(&slave->xfer);
	memset(&slave->xfer, 0, sizeof(slave->xfer));

	ret = sunxi_spi_slave_test_submit(slave);
	if (ret)
		goto terminate;

	return ;

terminate:
	slave->status = SUNXI_SPI_SLAVE_IDLE;
	sunxi_info(dev, "slave transfer terminate\n");
	complete(&slave->finished);
}

static int sunxi_spi_slave_test_submit(struct sunxi_spi_slave_test *slave)
{
	struct sunxi_spi_slave_head *pkt_head = &slave->frame.pkt_head;
	struct device *dev = &slave->spi->dev;
	int ret = 0;

	slave->status = SUNXI_SPI_SLAVE_RUNNING;

	sunxi_spi_slave_head_data_parse(slave->frame.data, pkt_head);

	if (!sunxi_spi_slave_has_ptk_head(pkt_head)) {
		sunxi_debug(dev, "No Package head, wait revice from master\n");
		pkt_head->op_code = SUNXI_OP_HEAD;
		slave->xfer.rx_buf = slave->frame.data;
		slave->xfer.len = sizeof(slave->frame.data);
	} else {
		sunxi_spi_slave_head_data_clear(slave->frame.data, sizeof(slave->frame.data));
		sunxi_debug(dev, "op=0x%x bits=%d addr=0x%x len=0x%x\n", pkt_head->op_code, pkt_head->bits, pkt_head->addr, pkt_head->len);

		switch (pkt_head->op_code) {
		case SUNXI_OP_WR:
			slave->frame.rx_buf = devm_kzalloc(&slave->spi->dev, pkt_head->len, GFP_KERNEL);
			slave->xfer.rx_buf = slave->frame.rx_buf;
			slave->xfer.rx_nbits = pkt_head->bits;
			slave->xfer.tx_buf = NULL;
			slave->xfer.len = pkt_head->len;
			sunxi_debug(dev, "slave recv data from master prepare\n");
			break;
		case SUNXI_OP_RD:
			slave->frame.tx_buf = devm_kzalloc(&slave->spi->dev, pkt_head->len, GFP_KERNEL);
			slave->xfer.tx_buf = slave->frame.tx_buf;
			slave->xfer.tx_nbits = pkt_head->bits;
			slave->xfer.rx_buf = NULL;
			slave->xfer.len = pkt_head->len;
			sunxi_spi_slave_get_cache_data(slave, pkt_head, (u8 *)slave->xfer.tx_buf);
			sunxi_debug(dev, "slave send data to master prepare\n");
			break;
		case SUNXI_OP_WR_RD:
			if (pkt_head->bits != SPI_NBITS_SINGLE) {
				sunxi_err(dev, "full duplex only support single(1-bit) mode\n");
				ret = -EINVAL;
			} else {
				slave->frame.rx_buf = devm_kzalloc(&slave->spi->dev, pkt_head->len, GFP_KERNEL);
				slave->xfer.rx_buf = slave->frame.rx_buf;
				slave->xfer.rx_nbits = pkt_head->bits;
				slave->frame.tx_buf = devm_kzalloc(&slave->spi->dev, pkt_head->len, GFP_KERNEL);
				slave->xfer.tx_buf = slave->frame.tx_buf;
				slave->xfer.tx_nbits = pkt_head->bits;
				slave->xfer.len = pkt_head->len;
				sunxi_spi_slave_get_cache_data(slave, pkt_head, (u8 *)slave->xfer.tx_buf);
				sunxi_debug(dev, "slave recv/send data from/to master prepare\n");
			}
			break;
		default:
			sunxi_err(dev, "unknown op code %d, wait revice from master\n", pkt_head->op_code);
			ret = -EINVAL;
			break;
		}
	}

	if (ret < 0) {
		sunxi_spi_slave_head_data_clear(slave->frame.data, sizeof(slave->frame.data));
		pkt_head->op_code = SUNXI_OP_HEAD;
		slave->xfer.rx_buf = slave->frame.data;
		slave->xfer.len = sizeof(slave->frame.data);
	}

	spi_message_init_with_transfers(&slave->msg, &slave->xfer, 1);
	slave->msg.complete = sunxi_spi_slave_test_complete;
	slave->msg.context = slave;

	ret = spi_async(slave->spi, &slave->msg);
	if (ret)
		sunxi_err(dev, "spi_async failed %d\n", ret);

	return ret;
}

static ssize_t sunxi_spi_slave_bin_read(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	struct sunxi_spi_slave_test *slave = dev_get_drvdata(kobj_to_dev(kobj));

	spin_lock(&slave->cache.buffer_lock);
	memcpy(buf, &slave->cache.buffer[off], count);
	spin_unlock(&slave->cache.buffer_lock);

	return count;
}

static ssize_t sunxi_spi_slave_bin_write(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	struct sunxi_spi_slave_test *slave = dev_get_drvdata(kobj_to_dev(kobj));

	spin_lock(&slave->cache.buffer_lock);
	memcpy(&slave->cache.buffer[off], buf, count);
	spin_unlock(&slave->cache.buffer_lock);

	return count;
}

int sunxi_spi_init_slave_data(struct sunxi_spi_slave_test *slave, u8 pattern)
{
	memset(slave->cache.buffer, pattern, slave->cache.size);
	return 0;
}

static ssize_t sunxi_slave_abort_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct sunxi_spi_slave_test *slave = spi_get_drvdata(spi);

	sunxi_info(dev, "slave transfer abort\n");

	slave->status = SUNXI_SPI_SLAVE_RETRY;
	spi_slave_abort(spi);

	return count;
}

static ssize_t sunxi_slave_submit_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct sunxi_spi_slave_test *slave = spi_get_drvdata(spi);

	if (slave->status != SUNXI_SPI_SLAVE_IDLE) {
		sunxi_warn(dev, "slave transfer isn't idle\n");
		return count;
	}

	sunxi_info(dev, "slave transfer submit\n");

	sunxi_spi_slave_test_submit(slave);

	return count;
}

static struct device_attribute sunxi_spi_slave_debug_attr[] = {
	__ATTR(abort, 0200, NULL, sunxi_slave_abort_store),
	__ATTR(submit, 0200, NULL, sunxi_slave_submit_store),
};

static void sunxi_spi_slave_create_sysfs(struct sunxi_spi_slave_test *slave)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_spi_slave_debug_attr); i++)
		device_create_file(&slave->spi->dev, &sunxi_spi_slave_debug_attr[i]);
}

static void sunxi_spi_slave_remove_sysfs(struct sunxi_spi_slave_test *slave)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_spi_slave_debug_attr); i++)
		device_remove_file(&slave->spi->dev, &sunxi_spi_slave_debug_attr[i]);
}

static int sunxi_spi_slave_test_probe(struct spi_device *spi)
{
	struct sunxi_spi_slave_test *slave;
	struct device *dev = &spi->dev;
	int ret;

	slave = devm_kzalloc(dev, sizeof(*slave), GFP_KERNEL);
	if (!slave)
		return -ENOMEM;

	slave->spi = spi;
	init_completion(&slave->finished);
	spin_lock_init(&slave->cache.buffer_lock);

	ret = of_property_read_u32(spi->dev.of_node, "slave-cachesize", &slave->cache.size);
	if (ret) {
		slave->cache.size = SALVE_CACHE_MAX;
		sunxi_warn(dev, "failed to get slave-cachesize, use default value %d\n", slave->cache.size);
	} else if (slave->cache.size > SALVE_CACHE_MAX) {
		slave->cache.size = SALVE_CACHE_MAX;
		sunxi_err(dev, "slave-cachesize too big, use default value %d\n", slave->cache.size);
	}

	slave->cache.buffer = devm_kzalloc(dev, slave->cache.size, GFP_KERNEL);
	if (!slave->cache.buffer)
		return -ENOMEM;

	sunxi_info(dev, "slave cachesize %d\n", slave->cache.size);

	sunxi_spi_init_slave_data(slave, 0xff);

	spi_set_drvdata(spi, slave);

	sysfs_bin_attr_init(&slave->bin);
	slave->bin.attr.name = "sunxi-slave-test";
	slave->bin.attr.mode = S_IRUSR | S_IWUSR;
	slave->bin.read = sunxi_spi_slave_bin_read;
	slave->bin.write = sunxi_spi_slave_bin_write;
	slave->bin.size = slave->cache.size;
	ret = sysfs_create_bin_file(&spi->dev.kobj, &slave->bin);
	if (ret)
		return ret;

	sunxi_spi_slave_create_sysfs(slave);

	return sunxi_spi_slave_test_submit(slave);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
static void sunxi_spi_slave_test_remove(struct spi_device *spi)
#else
static int sunxi_spi_slave_test_remove(struct spi_device *spi)
#endif
{
	struct sunxi_spi_slave_test *slave = spi_get_drvdata(spi);

	slave->status = SUNXI_SPI_SLAVE_STOP;
	spi_slave_abort(spi);
	wait_for_completion(&slave->finished);
	sysfs_remove_bin_file(&spi->dev.kobj, &slave->bin);
	sunxi_spi_slave_remove_sysfs(slave);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
#else
	return 0;
#endif
}

static const struct spi_device_id sunxi_spislave_ids[] = {
	{ .name = "spislave" },
	{},
};
MODULE_DEVICE_TABLE(spi, sunxi_spislave_ids);

static const struct of_device_id sunxi_spislave_dt_ids[] = {
	{ .compatible = "sunxi,spislave" },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_spislave_dt_ids);

static struct spi_driver sunxi_spi_slave_test_driver = {
	.driver = {
		.name	= "sunxi-spi-slave",
		.owner	= THIS_MODULE,
		.of_match_table = sunxi_spislave_dt_ids,
	},
	.probe  = sunxi_spi_slave_test_probe,
	.remove = sunxi_spi_slave_test_remove,
	.id_table   = sunxi_spislave_ids,
};
module_spi_driver(sunxi_spi_slave_test_driver);

MODULE_AUTHOR("jingyanliang <jingyanliang@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI SPI slave test driver");
MODULE_VERSION("1.2.0");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:sunxi-slave-test");

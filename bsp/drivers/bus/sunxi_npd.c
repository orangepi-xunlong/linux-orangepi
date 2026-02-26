/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI NPD driver
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
 * Author: chendecheng <chendecheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
/* support file operation */
#include <linux/fs.h>

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>

#include "sunxi-nsi.h"
#include "sunxi_npd.h"

static struct class *npd_class;
static uint32_t PMU_STEP_VALUE;
static uint32_t PMU_REG_STEP_VALUE;
static unsigned long npd_num_value = 1;
dma_addr_t dma_handle;
/* support power manager API */
#if IS_ENABLED(CONFIG_PM)
static int sunxi_npd_suspend(struct device *dev)
{
	dev_info(dev, "npd suspend okay\n");

	return 0;
}

static int sunxi_npd_resume(struct device *dev)
{
	dev_info(dev, "npd resume okay\n");

	return 0;
}

static const struct dev_pm_ops sunxi_npd_pm_ops = {
	.suspend = sunxi_npd_suspend,
	.resume = sunxi_npd_resume,
};

#define SUNXI_MBUS_PM_OPS (&sunxi_npd_pm_ops)
#else
#define SUNXI_MBUS_PM_OPS NULL
#endif

static void sunxi_pmu_enable(void)
{
	uint32_t val;

	val = readl(sunxi_npd.base_addr + PMU_EN_REG);
	val = 0xff;
	writel(val, sunxi_npd.base_addr + PMU_EN_REG);

}

static void sunxi_pmu_addr_set(void)
{
	unsigned int i;

/* support cpu directly access */
#ifdef CPU_DIRECT_ACC
	i = 0;
	writel(CPU_PMU_ADDR, sunxi_npd.base_addr + PMU_BASEADDR_REG(i));
#endif

	for (i = 0; i < PMU_MAX; i++) {
		if (pmu_pick[i] < 0)
			continue;

		writel(NSI_BASE_ADDR(pmu_pick[i]), sunxi_npd.base_addr + PMU_BASEADDR_REG(i));
	}

}

static void sunxi_com_enable(void)
{
	/* close now, if want to use common register function, change here */
	return ;
}

static void sunxi_com_addr_set(void)
{
	/* if want to use common register function, add here, the following code is an example. */
	return ;
}

static void sunxi_sample_period_set(uint32_t cycle)
{
	uint32_t val;

	/* `echo > period` to change period */

	/* default sampling frequent is 1/16s,
	 * 16 times in one sampling period,
	 * so one sampling period is 1s.
	 */
	if (cycle > 0) {
		val = cycle;
	} else {
		val = 0x8f0d18;
		sunxi_npd.period = (1000 * 1000) / 16;
		sunxi_npd.cycle = val;
	}

	writel(val, sunxi_npd.base_addr + NPD_PERIOD_REG);

}

static void sunxi_sample_num_set(void)
{
	uint32_t val;
	val = readl(sunxi_npd.base_addr + NPD_NUM_REG);
	/* hardware will auto mult 16 */
	val = npd_num_value;
	writel(val, sunxi_npd.base_addr + NPD_NUM_REG);
}

static void sunxi_ddr_addr_set(void)
{
	uint32_t val;

	/* val = virt_to_phys(sunxi_npd.ddr_addr); */
	val = (uint32_t)dma_handle;
	writel((val >> 2), sunxi_npd.base_addr + DDR_BASE_REG);

}

static void sunxi_npd_enable(void)
{
	uint32_t val;

	val = 0x1;
	writel(val, sunxi_npd.base_addr + NPD_EN_REG);

}

/*
 * note: PMU_STEP_VALUE and PMU_REG_STEP_VALUE flow NDP spec
 */
static void sunxi_npd_read_pmu_step(void)
{
	PMU_STEP_VALUE = readl(sunxi_npd.base_addr + PMU_STEP_REG);
	PMU_REG_STEP_VALUE = readl(sunxi_npd.base_addr + PMUREG_STEP_REG);

	/* PMU_STEP_VALUE is word, mult 4 trans to byte */
	PMU_STEP_VALUE = PMU_STEP_VALUE * 4;
}

static void sunxi_npd_init(void)
{
	/* pmu assert */
	sunxi_pmu_enable();
	/* configure pmu base address */
	sunxi_pmu_addr_set();
	/* common register assert */
	sunxi_com_enable();
	/* configure common register start address */
	sunxi_com_addr_set();
	/* set npd sampling period */
	sunxi_sample_period_set(sunxi_npd.cycle);
	/* set npd sampling number of each period */
	sunxi_sample_num_set();
	/* set memory start address */
	sunxi_ddr_addr_set();
	/* npd assert */
	sunxi_npd_enable();
	/* read pmu reg */
	sunxi_npd_read_pmu_step();
}

static ssize_t npd_bandwidth_simple_show(struct device *dev,
					 struct device_attribute *da, char *buf)
{
	unsigned long bread, bwrite, bandrw[PMU_MAX], total = 0;
	unsigned int i, j;
	unsigned int len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	uint32_t ret;
	static unsigned char npd_read_cnt = 1;

	ret = readl(sunxi_npd.base_addr + NPD_EN_REG);
	if (ret != 0) {
		printk("npd sample not finished. %u \n", npd_read_cnt);
	}

	spin_lock_irqsave(&sunxi_npd.bwlock, flags);

	/* PMU_REG_STEP_VALUE: 0x40 ,PMU_STEP_VALUE: 0x180 */
	for (j = 0; j < 16; j++) {
		for (i = 0; i < PMU_MAX; i++) {
			bread = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 2) + (4 * j) + (i * PMU_STEP_VALUE));
			bwrite = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 3) + (4 * j) + (i * PMU_STEP_VALUE));
			bandrw[i] = bread + bwrite;
			total += bandrw[i];
		}

		for (i = 0; i < PMU_MAX; i++) {
			len += sprintf(bwbuf, "%lu  ", bandrw[i]);
			strcat(buf, bwbuf);
		}

		len += sprintf(bwbuf, "%lu  ", total);
		strcat(buf, bwbuf);
		total = 0;
	}

	spin_unlock_irqrestore(&sunxi_npd.bwlock, flags);
	npd_read_cnt = npd_read_cnt + 1;

	ret = readl(sunxi_npd.base_addr + NPD_EN_REG);
	if (ret == 0)
		sunxi_npd_init();

	return len;
}

static ssize_t npd_read_bandwidth_show(struct device *dev,
				       struct device_attribute *da, char *buf)
{
	unsigned int bread, bandr[PMU_MAX];
	unsigned int i, total = 0;
	unsigned int len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	uint32_t ret;

	ret = readl(sunxi_npd.base_addr + NPD_EN_REG);
	if (ret == 0)
		sunxi_npd_init();

	spin_lock_irqsave(&sunxi_npd.bwlock, flags);

	for (i = 0; i < PMU_MAX; i++) {
		bread = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 2) + (i * PMU_STEP_VALUE));
		bandr[i] = bread;
		total += bandr[i];
	}

	for (i = 0; i < PMU_MAX; i++) {
		len += sprintf(bwbuf, "%u  ", bandr[i] / 1024);
		strcat(buf, bwbuf);
	}

	len += sprintf(bwbuf, "%u\n", total / 1024);
	strcat(buf, bwbuf);

	spin_unlock_irqrestore(&sunxi_npd.bwlock, flags);

	return len;
}

static ssize_t npd_write_bandwidth_show(struct device *dev,
					 struct device_attribute *da, char *buf)
{
	unsigned int bwrite, bandw[PMU_MAX];
	unsigned int i, total = 0;
	unsigned int len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	uint32_t ret;

	ret = readl(sunxi_npd.base_addr + NPD_EN_REG);
	if (ret == 0)
		sunxi_npd_init();

	spin_lock_irqsave(&sunxi_npd.bwlock, flags);

	for (i = 0; i < PMU_MAX; i++) {
		bwrite = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 3) + (i * PMU_STEP_VALUE));
		bandw[i] = bwrite;
		total += bandw[i];
	}

	for (i = 0; i < PMU_MAX; i++) {
		len += sprintf(bwbuf, "%u  ", bandw[i] / 1024);
		strcat(buf, bwbuf);
	}

	len += sprintf(bwbuf, "%u\n", total / 1024);
	strcat(buf, bwbuf);

	spin_unlock_irqrestore(&sunxi_npd.bwlock, flags);

	return len;
}

/* show write delay */
static ssize_t npd_write_delay_show(struct device *dev,
				    struct device_attribute *da, char *buf)
{
	unsigned int dwrite, drecord[PMU_MAX];
	unsigned int i, total = 0;
	unsigned int len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	uint32_t ret;

	ret = readl(sunxi_npd.base_addr + NPD_EN_REG);
	if (ret == 0)
		sunxi_npd_init();

	spin_lock_irqsave(&sunxi_npd.bwlock, flags);

	for (i = 0; i < PMU_MAX; i++) {
		dwrite = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 5) + (i * PMU_STEP_VALUE));
		drecord[i] = dwrite;
		total += drecord[i];
	}

	for (i = 0; i < PMU_MAX; i++) {
		len += sprintf(bwbuf, "%u  ", drecord[i]);
		strcat(buf, bwbuf);
	}

	len += sprintf(bwbuf, "%u\n", total);
	strcat(buf, bwbuf);

	spin_unlock_irqrestore(&sunxi_npd.bwlock, flags);

	return len;
}

/* show read delay */
static ssize_t npd_read_delay_show(struct device *dev,
				   struct device_attribute *da, char *buf)
{
	unsigned int dread, drecord[PMU_MAX];
	unsigned int i, total = 0;
	unsigned int len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	uint32_t ret;

	ret = readl(sunxi_npd.base_addr + NPD_EN_REG);
	if (ret == 0)
		sunxi_npd_init();

	spin_lock_irqsave(&sunxi_npd.bwlock, flags);

	for (i = 0; i < PMU_MAX; i++) {
		dread = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 4) + (i * PMU_STEP_VALUE));
		drecord[i] = dread;
		total += drecord[i];
	}

	for (i = 0; i < PMU_MAX; i++) {
		len += sprintf(bwbuf, "%u  ", drecord[i]);
		strcat(buf, bwbuf);
	}

	len += sprintf(bwbuf, "%u\n", total);
	strcat(buf, bwbuf);

	spin_unlock_irqrestore(&sunxi_npd.bwlock, flags);

	return len;
}

/* show number of write instruction */
static ssize_t npd_write_inst_num_show(struct device *dev,
				       struct device_attribute *da, char *buf)
{
	unsigned int wlatency, nwrite, nrecord[PMU_MAX];
	unsigned int i, total = 0;
	unsigned int len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	uint32_t ret;

	ret = readl(sunxi_npd.base_addr + NPD_EN_REG);
	if (ret == 0)
		sunxi_npd_init();

	spin_lock_irqsave(&sunxi_npd.bwlock, flags);

	for (i = 0; i < PMU_MAX; i++) {
		wlatency = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 5) + (i * PMU_STEP_VALUE));
		nwrite = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 1) + (i * PMU_STEP_VALUE));
		nrecord[i] = wlatency / nwrite;
		total += nrecord[i];
	}

	for (i = 0; i < PMU_MAX; i++) {
		len += sprintf(bwbuf, "%u  ", nrecord[i]);
		strcat(buf, bwbuf);
	}

	len += sprintf(bwbuf, "%u\n", total);
	strcat(buf, bwbuf);

	spin_unlock_irqrestore(&sunxi_npd.bwlock, flags);

	return len;
}

/* show number of read instruction */
static ssize_t npd_read_inst_num_show(struct device *dev,
				      struct device_attribute *da, char *buf)
{
	unsigned int rlatency, nread, nrecord[PMU_MAX];
	unsigned int i, total = 0;
	unsigned int len = 0;
	unsigned long flags = 0;
	char bwbuf[16];
	uint32_t ret;

	ret = readl(sunxi_npd.base_addr + NPD_EN_REG);
	if (ret == 0)
		sunxi_npd_init();

	sunxi_npd_init();

	spin_lock_irqsave(&sunxi_npd.bwlock, flags);

	for (i = 0; i < PMU_MAX; i++) {
		rlatency = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 4) + (i * PMU_STEP_VALUE));
		nread = readl(sunxi_npd.ddr_addr + (PMU_REG_STEP_VALUE * 0) + (i * PMU_STEP_VALUE));
		nrecord[i] = rlatency / nread;
		total += nrecord[i];
	}

	for (i = 0; i < PMU_MAX; i++) {
		len += sprintf(bwbuf, "%u  ", nrecord[i]);
		strcat(buf, bwbuf);
	}

	len += sprintf(bwbuf, "%u\n", total);
	strcat(buf, bwbuf);

	spin_unlock_irqrestore(&sunxi_npd.bwlock, flags);

	return len;
}

/* support period switch realtime */
static ssize_t npd_period_show(struct device *dev,
			       struct device_attribute *da, char *buf)
{
	/* int len; */
	/* char databuf[32]; */

	/* len = sprintf(databuf, "show period: %d, cycle: 0x%x \n", sunxi_npd.period, readl(sunxi_npd.base_addr + NPD_PERIOD_REG)); */

	/* strcat(buf, databuf); */

	printk("show period: %d, cycle: 0x%x \n", sunxi_npd.period, readl(sunxi_npd.base_addr + NPD_PERIOD_REG));

	/* return len; */

	return 0;
}

ssize_t npd_period_store(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret = 0;
	unsigned long period;
	unsigned int ahb_rate;
	unsigned long cycle;

	ahb_rate = sunxi_npd.ahb_rate;

	ret = kstrtoul(buf, 10, &period);

	if (ret < 0) {
		printk("kstrtoul error.\n");
		return count;
	}

	cycle = period * (ahb_rate / (1000 * 1000));

	sunxi_npd.period = period;
	sunxi_npd.cycle = cycle;

	printk("period: %lu, cycle: %lu", period, cycle);

	sunxi_sample_period_set(cycle);
	return count;
}

static ssize_t npd_ddr_addr_show(struct device *dev,
				 struct device_attribute *da, char *buf)
{
	int len;
	char databuf[32];

	len = sprintf(databuf, "%lx\n", (unsigned long)sunxi_npd.ddr_addr);

	strcat(buf, databuf);
	return len;
}

static ssize_t npd_pmu_pick_show(struct device *dev,
				 struct device_attribute *da, char *buf)
{
	int len = 0;
	int i;
	char databuf[32];

	for (i = 0; i < PMU_MAX; i++)
		len += sprintf(databuf, "%d ", pmu_pick[i]);

	strcat(buf, databuf);

	return len;
}

/* support write to file for offline analyse */
/*
static ssize_t npd_to_terminal_show(struct device *dev,
			struct device_attribute *da, char *buf)
{
	struct file *fp = NULL;
	mm_segment_t fs;
	loff_t pos;
	ssize_t rec;
	int test;

	unsigned char send[3072];
	unsigned char tmp = 0;
	int i;

	char path[128];

	sprintf(path, "/data/local/tmp/npd_origin");

	for (i = 0; i < 32; i++) {
		printk("send[%d]: 0x%x", i, send[i]);
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(path, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR(fp)) {
		printk("open file error.");
		goto err;
	}

	if (fp->f_op == NULL) {
		printk("ERROR: f_op is null!");
		goto err;
	}

	for (test = 0; test < 1000; test++) {

		for (i = 0; i < 3072; i++) {
			tmp = ((readl(sunxi_npd.ddr_addr + i) >> i) % 3) & 0xFF;
			send[i] = tmp;
		}

		pos = fp->f_pos;
		rec = vfs_write(fp, send, sizeof(send), &pos);
		fp->f_pos = pos;

		msleep(1);
	}

err:
	vfs_fsync(fp, 0);
	filp_close(fp, NULL);
	set_fs(fs);

	printk("finish %d time write to file.", test);

	return 0;
}

*/

static ssize_t npd_num_write(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret = 0;
	unsigned long npd_num_temp;

	ret = kstrtoul(buf, 10, &npd_num_temp);

	if (ret < 0) {
		printk("kstrtoul error.\n");
		return count;
	}
	npd_num_value = npd_num_temp;
	sunxi_sample_num_set();

	/* here init npd sample */
	sunxi_npd_init();
	printk("input npd_num_value %lu\n", npd_num_value);
	return count;
}

static ssize_t npd_num_read(struct device *dev,
			       struct device_attribute *da, char *buf)
{
	int len;
	char databuf[32];

	len = sprintf(databuf, "%u\n", readl(sunxi_npd.base_addr + NPD_NUM_REG));

	strcat(buf, databuf);

	return len;
}

static struct device_attribute dev_attr_npd_bandwidth =
	__ATTR(npd_bandwidth, 0444, npd_bandwidth_simple_show, NULL);

static struct device_attribute dev_attr_npd_period =
	__ATTR(period, 0644, npd_period_show, npd_period_store);

static struct device_attribute dev_attr_npd_ddr_addr =
	__ATTR(ddr_addr, 0444, npd_ddr_addr_show, NULL);

static struct device_attribute dev_attr_npd_pmu_pick =
	__ATTR(pmu, 0444, npd_pmu_pick_show, NULL);

static struct device_attribute dev_attr_npd_write_delay =
	__ATTR(wlatency, 0444, npd_write_delay_show, NULL);

static struct device_attribute dev_attr_npd_read_delay =
	__ATTR(rlatency, 0444, npd_read_delay_show, NULL);

static struct device_attribute dev_attr_npd_write_instnum =
	__ATTR(winstnum, 0444, npd_write_inst_num_show, NULL);

static struct device_attribute dev_attr_npd_read_instnum =
	__ATTR(rinstnum, 0444, npd_read_inst_num_show, NULL);

static struct device_attribute dev_attr_npd_read_bandwidth =
	__ATTR(rbandwidth, 0444, npd_read_bandwidth_show, NULL);

static struct device_attribute dev_attr_npd_write_bandwidth =
	__ATTR(wbandwidth, 0444, npd_write_bandwidth_show, NULL);

static struct device_attribute dev_attr_npd_num_write =
	__ATTR(npd_num, 0644, npd_num_read, npd_num_write);

/* static struct device_attribute dev_attr_npd_to_ternminal = */
	/* __ATTR(copy, 0444, npd_to_terminal_show, NULL); */


static struct attribute *npd_attributes[] = {
	&dev_attr_npd_period.attr,
	&dev_attr_npd_bandwidth.attr,
	&dev_attr_npd_ddr_addr.attr,
	&dev_attr_npd_pmu_pick.attr,
	&dev_attr_npd_write_delay.attr,
	&dev_attr_npd_read_delay.attr,
	&dev_attr_npd_read_instnum.attr,
	&dev_attr_npd_write_instnum.attr,
	&dev_attr_npd_read_bandwidth.attr,
	&dev_attr_npd_write_bandwidth.attr,
	&dev_attr_npd_num_write.attr,
	/* &dev_attr_npd_to_ternminal.attr, */

	NULL,
};

static struct attribute_group npd_group = {
	.attrs = npd_attributes,
};

static const struct attribute_group *npd_groups[] = {
	&npd_group,
	NULL,
};

static int npd_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	uint8_t *ddr_addr;
	unsigned int ahb_rate;
	struct clk *ahb_clk;

	sunxi_npd.dev_npd = dev;
	sunxi_npd.dev_npd = device_create_with_groups(npd_class,
						      dev, MKDEV(NPD_MAJOR, 0), NULL,
						      npd_groups, "hwmon%d", 0);

	if (IS_ERR(sunxi_npd.dev_npd)) {
		ret = PTR_ERR(sunxi_npd.dev_npd);
		goto out_err;
	}

	sunxi_npd.base_addr = ioremap(NPD_BASE_ADDR, 4);

	/* ddr_addr = kmalloc(MEM_SIZE, GFP_KERNEL); */
	ddr_addr = dma_alloc_coherent(dev, MEM_SIZE, &dma_handle, GFP_KERNEL);
	/* memset(ddr_addr, 0, MEM_SIZE); */
	sunxi_npd.ddr_addr = ddr_addr;

	ahb_clk = devm_clk_get(&pdev->dev, "ahb");
	if (IS_ERR_OR_NULL(ahb_clk)) {
		printk("[npd]Unable to get ahb clock!");
		goto out_err;
	}

	ret = of_property_read_u32(np, "clock-frequency", &ahb_rate);
	if (ret) {
		printk("[npd]Get ahb-clock-frequency property failed");
		goto out_err;
	}

	sunxi_npd.ahb_rate = clk_round_rate(ahb_clk, ahb_rate);

	if (clk_set_rate(ahb_clk, sunxi_npd.ahb_rate)) {
		printk("[npd]clk_set_rate failed");
		goto out_err;
	}

	if (clk_prepare_enable(ahb_clk)) {
		printk("[npd]ahb_clk enable failed!");
		goto out_err;
	}

	sunxi_npd.ahb_clk = ahb_clk;

	spin_lock_init(&sunxi_npd.bwlock);

	return 1;

out_err:
	dev_err(&pdev->dev, "probed failed\n");
	return ret;
}

static int npd_remove(struct platform_device *pdev)
{
	if (sunxi_npd.dev_npd) {
		device_remove_groups(sunxi_npd.dev_npd, npd_groups);
		device_destroy(npd_class, MKDEV(NPD_MAJOR, 0));
		sunxi_npd.dev_npd = NULL;
	}

	kfree(sunxi_npd.ddr_addr);

	return 0;
}

static const struct of_device_id sunxi_npd_matches[] = {
	{.compatible = "allwinner,sun55i-npd", },
	{},
};

static struct platform_driver npd_driver = {
	.driver = {
		.name   = DRIVER_NAME,
		.pm     = SUNXI_MBUS_PM_OPS,
		.of_match_table = sunxi_npd_matches,
	},
	.probe = npd_probe,
	.remove = npd_remove,
};

static const struct file_operations npd_fops = {
	.owner = THIS_MODULE,
};

static int npd_init(void)
{
	int ret;

	ret = register_chrdev_region(MKDEV(NPD_MAJOR, 0), NPD_MINORS, "npd");
	if (ret)
		goto out_err;

	cdev_init(&sunxi_npd.cdev, &npd_fops);
	sunxi_npd.cdev.owner = THIS_MODULE;
	ret = cdev_add(&sunxi_npd.cdev, MKDEV(NPD_MAJOR, 0), 1);
	if (ret) {
		pr_err("add cdev fail\n");
		goto out_err;
	}

	npd_class = class_create(THIS_MODULE, "npd-pmu");
	if (IS_ERR(npd_class)) {
		ret = PTR_ERR(npd_class);
		goto out_err;
	}

	ret = platform_driver_register(&npd_driver);
	if (ret) {
		pr_err("register sunxi npd platform driver failed\n");
		goto drv_err;
	}

	return ret;

drv_err:
	platform_driver_unregister(&npd_driver);
out_err:
	unregister_chrdev_region(MKDEV(NPD_MAJOR, 0), NPD_MINORS);
	return ret;
}

static void npd_exit(void)
{
	platform_driver_unregister(&npd_driver);
	class_destroy(npd_class);
	cdev_del(&sunxi_npd.cdev);
	unregister_chrdev_region(MKDEV(NPD_MAJOR, 0), NPD_MINORS);
}

module_init(npd_init);
module_exit(npd_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SUNXI NPD support");
MODULE_AUTHOR("chendecheng");
MODULE_VERSION("1.0.0");

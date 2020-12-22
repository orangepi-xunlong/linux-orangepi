/*
 * drivers/char/cma_test/sunxi_cma_test.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi cma test dirver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/random.h>

#define DEFAULT_TEST_SECS 6
#define CMA_NUM  10
typedef struct {
	dma_addr_t phys;
	void *virt;
	u32 size; /* in mbytes */
}cma_buf_t;
static cma_buf_t cma_buf[CMA_NUM];
static u32 test_secs = DEFAULT_TEST_SECS;

ssize_t test_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	printk("%s: buf %s\n", __func__, buf);
	return size;
}

ssize_t time_show(struct class *class, struct class_attribute *attr, char *buf)
{
	int cnt;

	cnt = sprintf(buf, "%s: test senconds %d\n", __func__, test_secs);
	return cnt;
}

ssize_t time_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	u32 seconds;

	if(strict_strtoul(buf, 10, (long unsigned int *)&seconds)) {
		pr_err("%s: invalid string %s\n", __func__, buf);
		return -EINVAL;
	}
	test_secs = seconds;
	printk("%s: get test seconds %d\n", __func__, seconds);
	return size;
}

ssize_t test_show(struct class *class, struct class_attribute *attr, char *buf)
{
	struct sysinfo start, end;
	int i;

	pr_info("%s: enter\n", __func__);

	do_sysinfo(&start);
loop:
	memset(cma_buf, 0, sizeof(cma_buf));
	for(i = 0; i < CMA_NUM; i++) {
		cma_buf[i].size = get_random_int();
		cma_buf[i].size %= (CONFIG_CMA_SIZE_MBYTES/CMA_NUM) + 10; /* give some more to meet fail */
		if(!cma_buf[i].size)
			cma_buf[i].size = CONFIG_CMA_SIZE_MBYTES/CMA_NUM;
		cma_buf[i].virt = dma_alloc_coherent(NULL, cma_buf[i].size*SZ_1M, &cma_buf[i].phys, GFP_KERNEL);
		if(cma_buf[i].virt)
			pr_info("%s: alloc %dMbytes success, 0x%08x-0x%08x\n", __func__,
				cma_buf[i].size, (u32)cma_buf[i].virt, cma_buf[i].phys);
		else
			pr_info("%s: alloc %dM bytes failed\n", __func__, cma_buf[i].size);
	}
	for(i = 0; i < CMA_NUM; i++) {
		if(cma_buf[i].virt) {
			pr_info("%s: free %d Mbytes(0x%08x-0x%08x)\n", __func__,
				cma_buf[i].size, (u32)cma_buf[i].virt, cma_buf[i].phys);
			dma_free_coherent(NULL, cma_buf[i].size*SZ_1M, cma_buf[i].virt, cma_buf[i].phys);
		}
	}
	do_sysinfo(&end);
	if(end.uptime - start.uptime < test_secs)
		goto loop;

	pr_info("%s: end\n", __func__);
	return 0;
}

ssize_t help_show(struct class *class, struct class_attribute *attr, char *buf)
{
	int cnt;

	cnt = sprintf(buf, "cma total reserve:  %dM\n", CONFIG_CMA_SIZE_MBYTES);
	cnt += sprintf(buf+cnt, "usage:\n");
	cnt += sprintf(buf+cnt, "    set test seconds: echo 5 > time(0 means test forever, but recommend set <=6, incase log buf overflow)\n");
	cnt += sprintf(buf+cnt, "    start test:       cat test\n");
	return cnt;
}

static struct class_attribute cma_class_attrs[] = {
	__ATTR(test, 0664, test_show, test_store), /* not 222, for CTS, other group cannot have write permission */
	__ATTR(time, 0664, time_show, time_store),
	__ATTR(help, 0444, help_show, NULL),
	__ATTR_NULL,
};

static struct class cma_test_class = {
	.name		= "sunxi_cma_test",
	.owner		= THIS_MODULE,
	.class_attrs	= cma_class_attrs,
};

static int __init sunxi_cmatest_init(void)
{
	int status;

	printk("%s enter\n", __func__);

	status = class_register(&cma_test_class);
	if(status < 0)
		printk("%s err, status %d\n", __func__, status);
	else
		printk("%s success\n", __func__);
	return 0;
}

static void  __exit sunxi_cmatest_exit(void)
{
	pr_info("%s enter\n", __func__);
}

module_init(sunxi_cmatest_init);
module_exit(sunxi_cmatest_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liugang");
MODULE_DESCRIPTION("sunxi cma test driver");


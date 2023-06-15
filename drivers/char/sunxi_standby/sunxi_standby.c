/*
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/device.h>
#include <soc/allwinner/sunxi_sip.h>

static u32 time_to_wakeup_ms;
static u32 debug_dram_crc_en;
static u32 debug_dram_crc_srcaddr = 0x40000000;
static u32 debug_dram_crc_len = (1024 * 1024);

static ssize_t time_to_wakeup_ms_show(struct class *class, struct class_attribute *attr,
		char *buf)
{
	ssize_t size = 0;

	size = sprintf(buf, "%u\n", time_to_wakeup_ms);

	return size;
}

static ssize_t time_to_wakeup_ms_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	u32 value = 0;
	int ret;

	ret = kstrtoint(buf, 10, &value);
	if (ret) {
		pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
		return -EINVAL;
	}

	time_to_wakeup_ms = value;

	invoke_scp_fn_smc(SET_WAKEUP_SRC,
			SET_WAKEUP_TIME_MS(time_to_wakeup_ms), 0, 0);

	pr_info("time_to_wakeup_ms change to %d\n", time_to_wakeup_ms);

	return count;
}
static CLASS_ATTR_RW(time_to_wakeup_ms);

static ssize_t dram_crc_paras_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	ssize_t size = 0;

	size = sprintf(buf, "enable:0x%x srcaddr:0x%x lenght:0x%x\n", debug_dram_crc_en,
			debug_dram_crc_srcaddr, debug_dram_crc_len);

	return size;
}

static ssize_t dram_crc_paras_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	u32 dram_crc_en      = 0;
	u32 dram_crc_srcaddr = 0;
	u32 dram_crc_len     = 0;

	sscanf(buf, "%x %x %x\n", &dram_crc_en, &dram_crc_srcaddr, &dram_crc_len);

	if ((dram_crc_en != 0) && (dram_crc_en != 1)) {
		pr_err("invalid debug dram crc paras [%x] [%x] [%x] to set\n",
			dram_crc_en, dram_crc_srcaddr, dram_crc_len);

		return count;
	}

	debug_dram_crc_en = dram_crc_en;
	debug_dram_crc_srcaddr = dram_crc_srcaddr;
	debug_dram_crc_len = dram_crc_len;
	invoke_scp_fn_smc(SET_DEBUG_DRAM_CRC_PARAS,
			debug_dram_crc_en,
			debug_dram_crc_srcaddr,
			debug_dram_crc_len);
	pr_info("dram_crc_en=0x%x, dram_crc_srcaddr=0x%x, dram_crc_len=0x%x\n",
		debug_dram_crc_en, debug_dram_crc_srcaddr, debug_dram_crc_len);

	return count;
}
static CLASS_ATTR_RW(dram_crc_paras);

static struct attribute *sunxi_standby_class_attrs[] = {
	&class_attr_time_to_wakeup_ms.attr,
	&class_attr_dram_crc_paras.attr,
	NULL,
};
ATTRIBUTE_GROUPS(sunxi_standby_class);

struct class sunxi_standby_class = {
	.name = "sunxi_standby",
	.class_groups = sunxi_standby_class_groups,
};

static int __init sunxi_standby_debug_init(void)
{
	int ret;

	ret = class_register(&sunxi_standby_class);
	if (ret < 0)
		pr_err("%s,%d err, ret:%d\n", __func__, __LINE__, ret);

	return ret;
}

static void __exit sunxi_standby_debug_exit(void)
{
	class_unregister(&sunxi_standby_class);
}

module_init(sunxi_standby_debug_init);
module_exit(sunxi_standby_debug_exit);

MODULE_DESCRIPTION("SUNXI STANDBY DEBUG");
MODULE_LICENSE("GPL");

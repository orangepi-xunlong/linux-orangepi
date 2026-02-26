/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include "aw_hdmi_define.h"
#include "aw_hdmi_drv.h"
#include "aw_hdmi_dev.h"

aw_device_t aw_dev_hdmi;

extern struct aw_hdmi_driver *g_hdmi_drv;

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
extern char *gHdcp_esm_fw_vir_addr;
extern u32 gHdcp_esm_fw_size;
#endif

static u8 reg_region;

/* use for HDMI address range out of range detection */
#define AW_HDMI_REGISTER_RANGE 0X100000 

/**
 * @short of_device_id structure
 */
static const struct of_device_id dw_hdmi_tx[] = {
	{ .compatible =	"allwinner,sunxi-hdmi" },
	{ }
};
MODULE_DEVICE_TABLE(of, dw_hdmi_tx);

/**
 * @short Platform driver structure
 */
static struct platform_driver __refdata dw_hdmi_pdrv = {
	.remove = aw_hdmi_drv_remove,
	.probe  = aw_hdmi_drv_probe,
	.driver = {
		.name = "allwinner,sunxi-hdmi",
		.owner = THIS_MODULE,
		.of_match_table = dw_hdmi_tx,
	},
};

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
#if IS_ENABLED(CONFIG_PM_SLEEP)
static SIMPLE_DEV_PM_OPS(aw_hdmi_cec_pm_ops,
		aw_hdmi_cec_suspend, aw_hdmi_cec_resume);
#endif

static struct platform_driver aw_hdmi_cec_driver = {
	.probe	= aw_hdmi_cec_probe,
	.remove	= aw_hdmi_cec_remove,
	.driver = {
		.name = "aw-hdmi-cec",
#if IS_ENABLED(CONFIG_PM_SLEEP)
		.pm   = &aw_hdmi_cec_pm_ops,
#endif
	},
};

#endif

static int __parse_dump_str(const char *buf, size_t size,
				unsigned long *start, unsigned long *end)
{
	char *ptr = NULL;
	char *ptr2 = (char *)buf;
	int ret = 0, times = 0;

	/* Support single address mode, some time it haven't ',' */
next:

	/* Default dump only one register(*start =*end).
	If ptr is not NULL, we will cover the default value of end. */
	if (times == 1)
		*start = *end;

	if (!ptr2 || (ptr2 - buf) >= size)
		goto out;

	ptr = ptr2;
	ptr2 = strnchr(ptr, size - (ptr - buf), ',');
	if (ptr2) {
		*ptr2 = '\0';
		ptr2++;
	}

	ptr = strim(ptr);
	if (!strlen(ptr))
		goto next;

	ret = kstrtoul(ptr, 16, end);
	if (!ret) {
		times++;
		goto next;
	} else {
		hdmi_wrn("String syntax errors: %s\n", ptr);
	}

out:
	return ret;
}

static ssize_t hdmi_hpd_mask_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%x\n", aw_hdmi_drv_get_hpd_mask());
}

static ssize_t hdmi_hpd_mask_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long val;

	if (count < 1)
		return -EINVAL;

	err = kstrtoul(buf, 16, &val);
	if (err) {
		hdmi_err("%s: parse buf error: %s\n", __func__, buf);
		return err;
	}

	aw_hdmi_drv_set_hpd_mask(val);
	hdmi_inf("set hpd mask: 0x%x\n", (u32)val);

	return count;
}

static DEVICE_ATTR(hpd_mask, 0664,
		hdmi_hpd_mask_show, hdmi_hpd_mask_store);

static ssize_t hdmi_edid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 *edid = NULL;
	u8 *edid_ext = NULL;

	/* get edid base block0 */
	edid = (u8 *)g_hdmi_drv->hdmi_core->mode.edid;
	if (edid == NULL) {
		hdmi_err("%s: edid base point is null!\n", __func__);
		return 0;
	}
	memcpy(buf, edid, 0x80);

	/* get edid extension block */
	edid_ext = (u8 *)g_hdmi_drv->hdmi_core->mode.edid_ext;
	if (edid_ext != NULL) {
		memcpy(buf + 0x80, edid_ext,
			0x80 * ((struct edid *)edid)->extensions);
	}

	if (edid && (!edid_ext))
		return 0x80;
	if (edid && edid_ext)
		return 0x80 * (1 + ((struct edid *)edid)->extensions);
	else
		return 0;
}

static ssize_t hdmi_edid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	g_hdmi_drv->hdmi_core->edid_ops.set_test_data(buf, count);
	return count;
}

static DEVICE_ATTR(edid, 0664,
		hdmi_edid_show, hdmi_edid_store);

static ssize_t hdmi_edid_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	bool mode = g_hdmi_drv->hdmi_core->edid_ops.get_test_mode();
	return sprintf(buf, "edid test mode: %d.\n", mode);
}

static ssize_t hdmi_edid_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	bool state = false;
	if (strncmp(buf, "0", 1) == 0)
		state = false;
	else
		state = true;

	g_hdmi_drv->hdmi_core->edid_ops.set_test_mode(state);

	return count;
}

static DEVICE_ATTR(edid_test, 0664,
		hdmi_edid_test_show, hdmi_edid_test_store);

static ssize_t hdmi_test_reg_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i = 0;
	u32 value = 0;

	enum hdmi2_reg_type_e {
		HDMI2_NONE_REG = 0,
		HDMI2_BASIC_REG,
		HDMI2_PHY_REG,
	};
	struct hdmi2_reg_t {
		const char *name;
		int start;
		int end;
		enum hdmi2_reg_type_e type;
	};
	struct hdmi2_reg_t hdmi2_regs[] = {
		{"Help",                           0x0000, 0x0000, HDMI2_NONE_REG},
		{"Identification Register",        0x0000, 0x0008, HDMI2_BASIC_REG},
		{"Interrupt Register",             0x0100, 0x0200, HDMI2_BASIC_REG},
		{"Video Sampler Register",         0x0200, 0x0208, HDMI2_BASIC_REG},
		{"Video Packetiser Register",      0x0800, 0x0808, HDMI2_BASIC_REG},
		{"Frame Composer Register",        0x1000, 0x121d, HDMI2_BASIC_REG},
		{"PHY Configuration Register",     0x3000, 0x3039, HDMI2_BASIC_REG},
		{"Audio Sampler Register",         0x3100, 0x3105, HDMI2_BASIC_REG},
		{"Audio Packetiser Register",      0x3200, 0x3208, HDMI2_BASIC_REG},
		{"Audio Sample SPDIF Register",    0x3300, 0x3305, HDMI2_BASIC_REG},
		{"Audio Sample GP Register",       0x3500, 0x3507, HDMI2_BASIC_REG},
		{"Audio DMA Register",             0x3600, 0x3628, HDMI2_BASIC_REG},
		{"Main Controller Register",       0x4001, 0x400b, HDMI2_BASIC_REG},
		{"Color Space Conventer Register", 0x4100, 0x411e, HDMI2_BASIC_REG},
		{"HDCP Register",                  0x5000, 0x5020, HDMI2_BASIC_REG},
		{"HDCP2.2 Register",               0x7900, 0x791a, HDMI2_BASIC_REG},
		{"CEC Register",                   0x7d00, 0x7d32, HDMI2_BASIC_REG},
		{"EDID Register",                  0x7e00, 0x7e32, HDMI2_BASIC_REG},
#if defined(SUNXI_HDMI20_PHY_AW)
		{"Aw Phy Register",                0x0040, 0x00a5, HDMI2_PHY_REG},
#elif defined(SUNXI_HDMI20_PHY_INNO)
		{"Inno Phy Register",              0x0000, 0x00d4, HDMI2_PHY_REG},
#elif defined(SUNXI_HDMI20_PHY_SNPS)
		{"Snps Phy Register",              0x0000, 0x0028, HDMI2_PHY_REG},
#endif
	};

	if ((!reg_region) || (reg_region > ((sizeof(hdmi2_regs) / sizeof(struct hdmi2_reg_t)) - 1))) {
		/* print help information */
		n += sprintf(buf + n, "echo [region_number] > /sys/class/hdmi/hdmi/attr/reg_dump\n");
		n += sprintf(buf + n, "cat /sys/class/hdmi/hdmi/attr/reg_dump\n");

		n += sprintf(buf + n, "\nregion_number:\n");
		for (i = 1; i < (sizeof(hdmi2_regs) / sizeof(struct hdmi2_reg_t)); i++) {
			n += sprintf(buf + n, "%d: %s\n", i, hdmi2_regs[i].name);
		}

		reg_region = 0;
		return n;
	}

	/* start to dump register */
	n += sprintf(buf + n, "\n%d: %s\n", reg_region, hdmi2_regs[reg_region].name);

	for (i = hdmi2_regs[reg_region].start; i < hdmi2_regs[reg_region].end; i++) {
		if ((i % 16) == 0 || i == hdmi2_regs[reg_region].start) {
			if ((i + 0x0f) < hdmi2_regs[reg_region].end) {
				n += sprintf(buf + n, "\n0x%04x-0x%04x:",
						i, i + 0x0f);
			} else {
				n += sprintf(buf + n, "\n0x%04x-0x%04x:",
						i, hdmi2_regs[reg_region].end-1);
			}
		}

		if (hdmi2_regs[reg_region].type == HDMI2_BASIC_REG) {
			n += sprintf(buf + n, " 0x%02x", aw_hdmi_drv_read(i * 4));
		} else if (hdmi2_regs[reg_region].type == HDMI2_PHY_REG) {
			if (g_hdmi_drv->hdmi_core->phy_ops.phy_read((u16)i, &value))
				break;

#if defined(SUNXI_HDMI20_PHY_SNPS)
			n += sprintf(buf + n, " 0x%04x", value);
#else
			n += sprintf(buf + n, " 0x%02x", value);
#endif
		}
	}
	n += sprintf(buf + n, "\n");

	reg_region = 0;
	return n;
}

ssize_t hdmi_test_reg_dump_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	char *end;

	reg_region = (u8)simple_strtoull(buf, &end, 0);

	return count;
}

static DEVICE_ATTR(reg_dump, 0664,
		hdmi_test_reg_dump_show, hdmi_test_reg_dump_store);

static ssize_t hdmi_test_reg_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n",
		"echo [0x(address offset), 0x(count)] > read");
}

ssize_t hdmi_test_reg_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long start_reg = 0;
	unsigned long read_count = 0;
	u32 i;
	u8 *separator;
	u32 data = 0;

	separator = strchr(buf, ',');
	if (separator != NULL) {
		if (__parse_dump_str(buf, count, &start_reg, &read_count))
			hdmi_err("%s: parse buf error: %s\n", __func__, buf);

		hdmi_inf("start_reg=0x%lx  read_count=%ld\n", start_reg, read_count);
		for (i = 0; i < read_count; i++) {
			hdmi_inf("hdmi_addr_offset: 0x%lx = 0x%x\n", start_reg,
					aw_hdmi_drv_read(start_reg * 4));

			start_reg++;
		}
	} else {
		separator = strchr(buf, ' ');
		if (separator != NULL) {
			start_reg = simple_strtoul(buf, NULL, 0);
			read_count = simple_strtoul(separator + 1, NULL, 0);
			for (i = 0; i < read_count; i += 4) {
				data = (u8)aw_hdmi_drv_read((start_reg + i) * 4);
				data |= ((u8)aw_hdmi_drv_read((start_reg + i + 1) * 4)) << 8;
				data |= ((u8)aw_hdmi_drv_read((start_reg + i + 2) * 4)) << 16;
				data |= ((u8)aw_hdmi_drv_read((start_reg + i + 3) * 4)) << 24;
				if ((i % 16) == 0)
					printk(KERN_CONT "\n0x%08lx: 0x%08x",
							(start_reg + i), data);
				else
					printk(KERN_CONT " 0x%08x", data);
			}
		} else {
			start_reg = simple_strtoul(buf, NULL, 0);
			hdmi_inf("hdmi_addr_offset: 0x%lx = 0x%x\n", start_reg,
					aw_hdmi_drv_read(start_reg * 4));
		}
	}
	hdmi_inf("\n");

	return count;
}

static DEVICE_ATTR(read, 0664,
		hdmi_test_reg_read_show, hdmi_test_reg_read_store);

static ssize_t hdmi_test_reg_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "echo [0x(address offset), 0x(value)] > write");
}

ssize_t hdmi_test_reg_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long reg_addr = 0;
	unsigned long value = 0;
	u8 *separator1 = NULL;
	u8 *separator2 = NULL;

	separator1 = strchr(buf, ',');
	separator2 = strchr(buf, ' ');
	if (separator1 != NULL) {
		if (__parse_dump_str(buf, count, &reg_addr, &value))
			hdmi_err("%s: parse buf error: %s\n", __func__, buf);

		hdmi_inf("reg_addr=0x%lx  write_value=0x%lx\n", reg_addr, value);

		if (reg_addr * 4 < 0 || (reg_addr * 4 + 0x4 > AW_HDMI_REGISTER_RANGE)) {
			hdmi_err("%s: register address is out of range!\n", __func__);
			return -EINVAL;
		}

		aw_hdmi_drv_write((reg_addr * 4), value);

		mdelay(1);
		hdmi_inf("after write,reg(%lx)=%x\n", reg_addr,
				aw_hdmi_drv_read(reg_addr * 4));
	} else if (separator2 != NULL) {
		reg_addr = simple_strtoul(buf, NULL, 0);
		value = simple_strtoul(separator2 + 1, NULL, 0);
		hdmi_inf("reg_addr=0x%lx  write_value=0x%lx\n", reg_addr, value);
		aw_hdmi_drv_write((reg_addr * 4), value);

		if (reg_addr * 4 < 0 || (reg_addr * 4 + 0x4 > AW_HDMI_REGISTER_RANGE)) {
			hdmi_err("%s: register address is out of range!\n", __func__);
			return -EINVAL;
		}

		mdelay(1);
		hdmi_inf("after write,red(%lx)=%x\n", reg_addr,
				aw_hdmi_drv_read(reg_addr * 4));
	} else {
		hdmi_err("%s: error input: %s\n", __func__, buf);
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR(write, 0664,
		hdmi_test_reg_write_show, hdmi_test_reg_write_store);

static ssize_t phy_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	hdmi_inf("OPMODE_PLLCFG-0x06\n");
	hdmi_inf("CKSYMTXCTRL-0x09\n");
	hdmi_inf("PLLCURRCTRL-0x10\n");
	hdmi_inf("VLEVCTRL-0x0E\n");
	hdmi_inf("PLLGMPCTRL-0x15\n");
	hdmi_inf("TXTERM-0x19\n");

	return sprintf(buf, "%s\n", "echo [0x(address offset), 0x(value)] > phy_write");
}

static ssize_t phy_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u16 reg_addr = 0;
	u16 value = 0;
	char *end;
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;

	reg_addr = (u16)simple_strtoull(buf, &end, 0);

	if ((*end != ' ') && (*end != ',')) {
		hdmi_err("error separator:%c\n", *end);
		return count;
	}

	value = (u16)simple_strtoull(end + 1, &end, 0);

	hdmi_inf("reg_addr=0x%x  write_value=0x%x\n", reg_addr, value);

	core->phy_ops.phy_write((u16)reg_addr, (u32)value);
	return count;
}

static DEVICE_ATTR(phy_write, 0664,
		phy_write_show, phy_write_store);

static ssize_t phy_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	hdmi_inf("OPMODE_PLLCFG-0x06\n");
	hdmi_inf("CKSYMTXCTRL-0x09\n");
	hdmi_inf("PLLCURRCTRL-0x10\n");
	hdmi_inf("VLEVCTRL-0x0E\n");
	hdmi_inf("PLLGMPCTRL-0x15\n");
	hdmi_inf("TXTERM-0x19\n");

	return sprintf(buf, "%s\n", "echo [0x(address offset), 0x(count)] > phy_read");
}

ssize_t phy_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long start_reg = 0;
	u32 value = 0;
	unsigned long read_count = 0;
	u32 i;
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;

	if (__parse_dump_str(buf, count, (unsigned long *)&start_reg, &read_count))
		hdmi_err("%s: parse buf error: %s\n", __func__, buf);

	hdmi_inf("start_reg=0x%lx  read_count=%ld\n", start_reg, read_count);

	for (i = 0; i < read_count; i++) {
		core->phy_ops.phy_read((u16)start_reg, &value);
		hdmi_inf("hdmi_addr_offset: 0x%lx = 0x%x\n", start_reg, (u32)value);
		start_reg++;
	}
	hdmi_inf("\n");

	return count;
}

static DEVICE_ATTR(phy_read, 0664,
		phy_read_show, phy_read_store);

extern u16 i2c_min_ss_scl_low_time;
extern u16 i2c_min_ss_scl_high_time;
static ssize_t hdmi_set_ddc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "low:%d high:%d\n", i2c_min_ss_scl_low_time,
		i2c_min_ss_scl_high_time);
	n += sprintf(buf + n, "%s\n",
		"echo [low_time, high_time] > set_ddc");

	return n;
}

ssize_t hdmi_set_ddc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	char *end;

	i2c_min_ss_scl_low_time = (u16)simple_strtoull(buf, &end, 0);

	if ((*end != ' ') && (*end != ',')) {
		hdmi_err("%s: error separator:%c\n", __func__, *end);
		return count;
	}

	i2c_min_ss_scl_high_time = (u16)simple_strtoull(end + 1, &end, 0);

	hdmi_inf("low:%d  high:%d\n", i2c_min_ss_scl_low_time,
			i2c_min_ss_scl_high_time);
	return count;
}

static DEVICE_ATTR(set_ddc, 0664,
		hdmi_set_ddc_show, hdmi_set_ddc_store);

#ifndef SUPPORT_ONLY_HDMI14
static ssize_t scdc_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "echo [0x(address offset), 0x(count)] > scdc_read");
}

ssize_t scdc_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long start_reg = 0;
	u8 value = 0;
	unsigned long read_count = 0;
	u32 i;
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;

	if (__parse_dump_str(buf, count, &start_reg, &read_count))
		hdmi_err("%s: parse buf error: %s\n", __func__, buf);

	hdmi_inf("start_reg=0x%x  read_count=%ld\n", (u32)start_reg, read_count);
	for (i = 0; i < read_count; i++) {
		core->dev_ops.scdc_read((u8)start_reg, 1, &value);
		hdmi_inf("hdmi_addr_offset: 0x%x = 0x%x\n", (u32)start_reg, value);
		start_reg++;
	}
	hdmi_inf("\n");

	return count;
}

static DEVICE_ATTR(scdc_read, 0664,
		scdc_read_show, scdc_read_store);

static ssize_t scdc_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "echo [0x(address offset), 0x(value)] > scdc_write");
}

static ssize_t scdc_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long reg_addr = 0;
	unsigned long value = 0;
	struct aw_hdmi_core_s *core = NULL;

	core = g_hdmi_drv->hdmi_core;

	if (__parse_dump_str(buf, count, &reg_addr, &value))
		hdmi_err("%s: parse buf error: %s\n", __func__, buf);

	hdmi_inf("reg_addr=0x%x  write_value=0x%x\n", (u8)reg_addr, (u8)value);
	core->dev_ops.scdc_write((u8)reg_addr, 0x1, (u8 *)&value);
	return count;
}

static DEVICE_ATTR(scdc_write, 0664,
		scdc_write_show, scdc_write_store);
#endif

static ssize_t hdmi_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "Current debug=%d\n\n", gHdmi_log_level);

	n += sprintf(buf + n, "hdmi log debug level:\n");
	n += sprintf(buf + n, "debug = 1, print video log\n");
	n += sprintf(buf + n, "debug = 2, print edid log\n");
	n += sprintf(buf + n, "debug = 3, print audio log\n");
	n += sprintf(buf + n, "debug = 4, print video+edid+audio log\n");
	n += sprintf(buf + n, "debug = 5, print cec log\n");
	n += sprintf(buf + n, "debug = 6, print hdcp log\n");
	n += sprintf(buf + n, "debug = 7, print all of the logs above\n");
	n += sprintf(buf + n, "debug = 8, print all of the logs above and trace log\n");

	return n;
}

static ssize_t hdmi_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strncmp(buf, "9", 1) == 0)
		gHdmi_log_level = 9;
	else if (strncmp(buf, "8", 1) == 0)
		gHdmi_log_level = 8;
	else if (strncmp(buf, "7", 1) == 0)
		gHdmi_log_level = 7;
	else if (strncmp(buf, "6", 1) == 0)
		gHdmi_log_level = 6;
	else if (strncmp(buf, "5", 1) == 0)
			gHdmi_log_level = 5;
	else if (strncmp(buf, "4", 1) == 0)
		gHdmi_log_level = 4;
	else if (strncmp(buf, "3", 1) == 0)
		gHdmi_log_level = 3;
	else if (strncmp(buf, "2", 1) == 0)
		gHdmi_log_level = 2;
	else if (strncmp(buf, "1", 1) == 0)
		gHdmi_log_level = 1;
	else if (strncmp(buf, "0", 1) == 0)
		gHdmi_log_level = 0;
	else
		hdmi_err("%s: invalid input: %s\n", __func__, buf);

	hdmi_inf("set hdmi debug level: %d\n", gHdmi_log_level);
	return count;
}

static DEVICE_ATTR(debug, 0664,
		hdmi_debug_show, hdmi_debug_store);

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
static ssize_t hdmi_hdcp_type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char hdcp_type = (char)aw_hdmi_drv_get_hdcp_type(g_hdmi_drv->hdmi_core);

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_hdcp);
	memcpy(buf, &hdcp_type, 1);
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_hdcp);

	return 1;
}

static ssize_t hdmi_hdcp_type_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(hdcp_type, 0664,
		hdmi_hdcp_type_show, hdmi_hdcp_type_store);

static ssize_t hdmi_hdcp_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *hdcp_enable_state[] = {"enable", "force_14", "force_22"};

	hdmi_inf("[Guide]\n");
	hdmi_inf("1-14: force enable hdcp14 function.\n");
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	hdmi_inf("1-22: force enable hdcp22 function.\n");
#endif
	hdmi_inf("1: enable hdcp function.\n");
	hdmi_inf("0: disable hdcp function.\n");
	if (!g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_on)
		hdmi_inf("current state: disable\n");
	else
		hdmi_inf("current state: %s\n", hdcp_enable_state[
				g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_debug]);

	return sprintf(buf, "%d\n",
		g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_on);
}

static ssize_t hdmi_hdcp_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_hdcp);
	if (strncmp(buf, "1-14", 4) == 0) {
		hdmi_inf("force enable hdcp14!\n");
		g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_debug = AW_HDCP_ENABLE_FORCE_14;
		aw_hdmi_drv_set_hdcp_state(AW_HDCP_ING);
		aw_hdmi_drv_hdcp_enable(g_hdmi_drv->hdmi_core, 1);
	} else if (strncmp(buf, "1-22", 4) == 0) {
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
		hdmi_inf("force enable hdcp22!\n");
		g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_debug = AW_HDCP_ENABLE_FORCE_22;
		aw_hdmi_drv_set_hdcp_state(AW_HDCP_ING);
		aw_hdmi_drv_hdcp_enable(g_hdmi_drv->hdmi_core, 1);
#else
		hdmi_err("%s: not enable CONFIG_AW_HDMI2_HDCP22_SUNXI!!!\n", __func__);
#endif
	} else if (strncmp(buf, "1", 1) == 0) {
		g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_debug = AW_HDCP_ENABLE_NORMAL;
		aw_hdmi_drv_set_hdcp_state(AW_HDCP_ING);
		aw_hdmi_drv_hdcp_enable(g_hdmi_drv->hdmi_core, 1);
	} else {
		g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_debug = AW_HDCP_ENABLE_NORMAL;
		aw_hdmi_drv_hdcp_enable(g_hdmi_drv->hdmi_core, 0);
		aw_hdmi_drv_set_hdcp_state(AW_HDCP_DISABLE);
	}
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_hdcp);

	return count;
}

static DEVICE_ATTR(hdcp_enable, 0664,
		hdmi_hdcp_enable_show, hdmi_hdcp_enable_store);

static ssize_t hdcp_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 count = sizeof(u8);
	u8 statue = aw_hdmi_drv_get_hdcp_state();

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_hdcp);
	memcpy(buf, &statue, count);
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_hdcp);

	return count;
}

static ssize_t hdcp_status_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	return count;
}

static DEVICE_ATTR(hdcp_status, 0664,
		hdcp_status_show, hdcp_status_store);

static ssize_t hdcp_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	ret += g_hdmi_drv->hdmi_core->hdcp_ops.hdcp_config_dump(buf + ret);

	return ret;
}

static DEVICE_ATTR(hdcp_dump, 0664,
		hdcp_dump_show, NULL);


#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
static char *esm_addr;
static ssize_t esm_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "addr=%p, size=0x%04x\n",
		gHdcp_esm_fw_vir_addr, gHdcp_esm_fw_size);
	esm_addr = gHdcp_esm_fw_vir_addr;

	return n;
}

static ssize_t esm_dump_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (esm_addr == 0 || gHdcp_esm_fw_size == 0) {
		hdmi_err("%s: esm address or fize is zero!!!\n", __func__);
		return -1;
	}

	memcpy(esm_addr, buf, count);
	esm_addr = esm_addr + count;
	hdmi_inf("esm_addr=%p, count=0x%04x\n", esm_addr, (unsigned int)count);
	return count;
}

static DEVICE_ATTR(esm_dump, 0664,
		esm_dump_show, esm_dump_store);

static ssize_t hpi_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "echo [0x(address offset), 0x(value)] > hpi_write");
}

static ssize_t hpi_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u32 reg_addr = 0;
	u32 value = 0;
	struct aw_hdmi_core_s *core = NULL;

	core = g_hdmi_drv->hdmi_core;

	if (__parse_dump_str(buf, count, (unsigned long *)&reg_addr, (unsigned long *)&value))
		hdmi_err("%s: parse buf error: %s\n", __func__, buf);

	hdmi_inf("reg_addr=0x%x  write_value=0x%x\n", reg_addr, value);

	if (reg_addr % 4 || reg_addr < 0 || reg_addr + 0x4 > AW_HDMI_REGISTER_RANGE) {
		hdmi_err("%s: register address is out of range\n", __func__);
		return -EINVAL;
	}

	*((u32 *)(core->mode.pHdcp.esm_hpi_base + reg_addr)) = (u32)value;
	return count;
}

static DEVICE_ATTR(hpi_write, 0664,
		hpi_write_show, hpi_write_store);

static ssize_t hpi_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "echo [0x(address offset)] > hpi_read");
}

ssize_t hpi_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u32 start_reg = 0;
	u32 value = 0;
	char *end;
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;

	start_reg = (u32)simple_strtoull(buf, &end, 0);

	if ((*end != ' ') && (*end != ',')) {
		hdmi_err("%s: error separator:%c\n", __func__, *end);
		return count;
	}

	hdmi_inf("start_reg = 0x%x\n", (u32)start_reg);
	value = *((u32 *)(core->mode.pHdcp.esm_hpi_base + start_reg));
	hdmi_inf("hdmi_addr_offset: 0x%x = 0x%x\n", (u32)start_reg, value);

	return count;
}

static DEVICE_ATTR(hpi_read, 0664,
		hpi_read_show, hpi_read_store);
#endif
#endif

#if IS_ENABLED(CONFIG_HDMI2_FREQ_SPREAD_SPECTRUM)
extern u32 freq_ss_amp;
static ssize_t freq_spread_spectrum_amp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "frequency spread spectrum amp:\n");
	n += sprintf(buf + n, "2/4/6/../30\n\n");

	n += sprintf(buf + n, "echo [amp] > /sys/clas/hdmi/hdmi/attr/ss_amp\n\n");

	n += sprintf(buf + n, "NOTE: if echo 0 > /sys/clas/hdmi/hdmi/attr/ss_amp,\n");
	n += sprintf(buf + n, "disable sprum spectrum\n\n");

	n += sprintf(buf + n, "The current amp is:%d\n", ss_amp);

	return n;
}

static ssize_t freq_spread_spectrum_amp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	char *end;
	struct disp_video_timings *video_info;

	freq_ss_amp = (u32)simple_strtoull(buf, &end, 0);

	_aw_disp_hdmi_get_video_timming_info(&video_info);

	_aw_freq_spread_sprectrum_enabled(video_info->pixel_clk);

	return count;
}

static DEVICE_ATTR(ss_amp, 0664,
		freq_spread_spectrum_amp_show, freq_spread_spectrum_amp_store);

extern bool freq_ss_old;
static ssize_t freq_spread_spectrum_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "Use the old frequency spread spectrum:\n");
	n += sprintf(buf + n, "echo old > /sys/clas/hdmi/hdmi/attr/ss_version\n\n");

	n += sprintf(buf + n, "Use the new frequency spread spectrum:\n");
	n += sprintf(buf + n, "echo new > /sys/clas/hdmi/hdmi/attr/ss_version\n\n");

	n += sprintf(buf + n, "The current version of frequency spread spectrum  is:%s\n",
		freq_ss_old ? "old" : "new");

	return n;
}

static ssize_t freq_spread_spectrum_version_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct disp_video_timings *video_info;

	if (strncmp(buf, "old", 1) == 0) {
		hdmi_inf("set old version freq_spread_spectrum\n");
		freq_ss_old = 1;
	} else if (strncmp(buf, "new", 1) == 0) {
		hdmi_inf("set new version freq_spread_spectrum\n");
		freq_ss_old = 0;
	} else {
		hdmi_err("%s: invalid input: %s\n", __func__, buf);
		return count;
	}

	_aw_disp_hdmi_get_video_timming_info(&video_info);

	_aw_freq_spread_sprectrum_enabled(video_info->pixel_clk);

	return count;
}

static DEVICE_ATTR(ss_version, 0664,
		freq_spread_spectrum_version_show, freq_spread_spectrum_version_store);
#endif

static const struct cea_vic hdmi_4kx2k_vics[] = {
	__DEF_VIC(HDMI_UNKNOWN),
	__DEF_VIC(HDMI_3840x2160P30_16x9),
	__DEF_VIC(HDMI_3840x2160P25_16x9),
	__DEF_VIC(HDMI_3840x2160P24_16x9),
	__DEF_VIC(HDMI_4096x2160P24_256x135),
};

static char *hdmi_audio_code_name[] = {
	"LPCM",
	"AC-3",
	"MPEG1",
	"MP3",
	"MPEG2",
	"AAC",
	"DTS",
	"ATRAC",
	"OneBitAudio",
	"DolbyDigital+",
	"DTS-HD",
	"MAT",
	"DST",
	"WMAPro",
};

static char *hdmi_video_colorimetry_name[] = {
	"XVYCC601",
	"XVYCC709",
	"SYCC601",
	"ADOBE_YCC601",
	"ADOBE_RGB",
	"BT2020_CYCC",
	"BT2020_YCC",
	"BT2020_RGB",
};

static char *hdmi_video_hdr_metadata_name[] = {
	"SDR_LUMINANCE_RANGE",
	"HDR_LUMINANCE_RANGE",
	"SMPTE_ST_2084",
	"FUTURE_EOTF",
};

static const char *debug_get_video_name(int hdmi_mode)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hdmi_cea_vics); i++) {
		if (hdmi_cea_vics[i].vic == hdmi_mode)
			return hdmi_cea_vics[i].name;
	}

	return NULL;
}

static ssize_t _aw_hdmi_sink_edid_parse(char *buf, ssize_t n)
{
	int i = 0;
	sink_edid_t *sink_cap = g_hdmi_drv->hdmi_core->mode.sink_cap;

	if (!sink_cap) {
		n += sprintf(buf + n, "%s\n", "Do not read edid from sink");
		return n;
	}

	if (!g_hdmi_drv->hdmi_core->mode.edid_done) {
		hdmi_wrn("can not read edid and return!\n");
		return n;
	}

	/* Basic Data Block */
	n += sprintf(buf + n, "\n[Baisc]");
	n += sprintf(buf + n, "\n - Monitor Name: %s", sink_cap->edid_mMonitorName);
	n += sprintf(buf + n, "\n - HDMI Version: %s",
			sink_cap->edid_m20Sink ? "support 2.0 VSDB" : "support 1.x VSDB");
	n += sprintf(buf + n, "\n - Tmds Rate:");
	if (sink_cap->edid_mHdmiForumvsdb.mValid) {
		n += sprintf(buf + n, " %dMHz (max)",
				sink_cap->edid_mHdmiForumvsdb.mMaxTmdsCharRate * 5);
	} else {
		n += sprintf(buf + n, " Unkonw");
	}
	n += sprintf(buf + n, "\n");


	/* Video Data Block */
	n += sprintf(buf + n, "\n[Video]");
	n += sprintf(buf + n, "\n - Resolution:");
	for (i = 0; i < sink_cap->edid_mSvdIndex; i++) {
		n += sprintf(buf + n, "\n\t%3d",
				(int)sink_cap->edid_mSvd[i].mCode);

		n += sprintf(buf + n, "\t%-25s",
			debug_get_video_name(
			(int)sink_cap->edid_mSvd[i].mCode));

		if (sink_cap->edid_mSvd[i].mNative)
			n += sprintf(buf + n, "\t[native]");

		/* only support yuv420 */
		if (sink_cap->edid_mSvd[i].mLimitedToYcc420) {
			n += sprintf(buf + n, "\t[only_yuv420]");
		}

		/* also support yuv420 */
		if (sink_cap->edid_mSvd[i].mYcc420) {
			n += sprintf(buf + n, "\t[yuv420]");
		}
	}

	/* HDMI1.4b VSDB 4kx2k vic */
	for (i = 0; i < sink_cap->edid_mHdmivsdb.mHdmiVicCount; i++) {
		if (sink_cap->edid_mHdmivsdb.mHdmiVic[i] <= 0x4) {
			n += sprintf(buf + n, "\n\t%3d",
					hdmi_4kx2k_vics[
					sink_cap->edid_mHdmivsdb.mHdmiVic[i]].vic);

			n += sprintf(buf + n, "\t%-25s",
			hdmi_4kx2k_vics[
			sink_cap->edid_mHdmivsdb.mHdmiVic[i]].name);
			n += sprintf(buf + n, "\t[4kx2k]");
		}
	}

	/* video pixel format and color depth */
	n += sprintf(buf + n, "\n - Color Format and Depth:");
	n += sprintf(buf + n, "\n\tRGB444: 24bit");
	if (sink_cap->edid_mHdmivsdb.mDeepColor30)
		n += sprintf(buf + n, " 30bit");
	if (sink_cap->edid_mHdmivsdb.mDeepColor36)
		n += sprintf(buf + n, " 36bit");
	if (sink_cap->edid_mHdmivsdb.mDeepColor48)
		n += sprintf(buf + n, " 48bit");

	n += sprintf(buf + n, "\n\tYUV444:");
	if (sink_cap->edid_mYcc444Support) {
		n += sprintf(buf + n, " 24bit");
		if (sink_cap->edid_mHdmivsdb.mDeepColor30)
			n += sprintf(buf + n, " 30bit");
		if (sink_cap->edid_mHdmivsdb.mDeepColor36)
			n += sprintf(buf + n, " 36bit");
		if (sink_cap->edid_mHdmivsdb.mDeepColor48)
			n += sprintf(buf + n, " 48bit");
	}

	n += sprintf(buf + n, "\n\tYUV422:");
	if (sink_cap->edid_mYcc422Support) {
		n += sprintf(buf + n, " 24bit");
		if (sink_cap->edid_mHdmivsdb.mDeepColor30)
			n += sprintf(buf + n, " 30bit");
		if (sink_cap->edid_mHdmivsdb.mDeepColor36)
			n += sprintf(buf + n, " 36bit");
		if (sink_cap->edid_mHdmivsdb.mDeepColor48)
			n += sprintf(buf + n, " 48bit");
	}

	n += sprintf(buf + n, "\n\tYUV420:");
	if (sink_cap->edid_mYcc420Support) {
		n += sprintf(buf + n, " 24bit");
		if (sink_cap->edid_mHdmiForumvsdb.mDC_30bit_420)
			n += sprintf(buf + n, " 30bit");
		if (sink_cap->edid_mHdmiForumvsdb.mDC_36bit_420)
			n += sprintf(buf + n, " 36bit");
		if (sink_cap->edid_mHdmiForumvsdb.mDC_48bit_420)
			n += sprintf(buf + n, " 48bit");
	}

	n += sprintf(buf + n, "\n - Colorimetry:");
	for (i = 0; i < 8; i++) {
		if (sink_cap->edid_mColorimetryDataBlock.mByte3 & BIT(i)) {
			n += sprintf(buf + n, "\n\t%s",
					hdmi_video_colorimetry_name[i]);
		}
	}

	n += sprintf(buf + n, "\n - HDR EOTF:");
	for (i = 0; i < 4; i++) {
		if (sink_cap->edid_hdr_static_metadata_data_block.et_n & BIT(i)) {
			n += sprintf(buf + n, "\n\t%s",
					hdmi_video_hdr_metadata_name[i]);
		}
	}

	n += sprintf(buf + n, "\n - HDR Metadata Descriptor: %x",
			sink_cap->edid_hdr_static_metadata_data_block.sm_n);

	n += sprintf(buf + n, "\n - HDR Max Luminance: %x",
			sink_cap->edid_hdr_static_metadata_data_block.dc_max_lum_data);

	n += sprintf(buf + n, "\n - HDR Max Frame Average Luminance: %x",
			sink_cap->edid_hdr_static_metadata_data_block.dc_max_fa_lum_data);

	n += sprintf(buf + n, "\n - HDR Min Luminance: %x",
			sink_cap->edid_hdr_static_metadata_data_block.dc_min_lum_data);

	/* 3D format */
	n += sprintf(buf + n, "\n - 3D Mode:");
	if (sink_cap->edid_mHdmivsdb.m3dPresent) {
		for (i = 0; i < 16; i++) {
			if (sink_cap->edid_mHdmivsdb.mVideo3dStruct[i][0] == 1
			&& i < sink_cap->edid_mSvdIndex) {
				n += sprintf(buf + n, "\n\t%s_FP",
				debug_get_video_name(
				(int)sink_cap->edid_mSvd[i].mCode));
			}
			if (sink_cap->edid_mHdmivsdb.mVideo3dStruct[i][6] == 1
			&& i < sink_cap->edid_mSvdIndex) {
				n += sprintf(buf + n, " \n\t%s_SBS",
				debug_get_video_name(
				(int)sink_cap->edid_mSvd[i].mCode));
			}
			if (sink_cap->edid_mHdmivsdb.mVideo3dStruct[i][8] == 1
			&& i < sink_cap->edid_mSvdIndex) {
				n += sprintf(buf + n, "\n\t%s_TAB",
				debug_get_video_name(
				(int)sink_cap->edid_mSvd[i].mCode));
			}
		}
	} else {
		n += sprintf(buf + n, " Not Support");
	}
	n += sprintf(buf + n, "\n");

	/* audio */
	n += sprintf(buf + n, "\n[Audio]");
	n += sprintf(buf + n, "\n - Basic Audio:");
	if (sink_cap->edid_mBasicAudioSupport) {
		for (i = 0; i < sink_cap->edid_mSadIndex; i++) {
			n += sprintf(buf + n, "\n\t%s",
			hdmi_audio_code_name[sink_cap->edid_mSad[i].mFormat-1]);
		}
	} else {
		n += sprintf(buf + n, " Not Support");
	}
	n += sprintf(buf + n, "\n");

	/* cec */
	n += sprintf(buf + n, "\n[CEC]");
	n += sprintf(buf + n, "\n - Physical Addr: %d.%d.%d.%d",
			(sink_cap->edid_mHdmivsdb.mPhysicalAddress & 0xF000) >> 12,
			(sink_cap->edid_mHdmivsdb.mPhysicalAddress & 0x0F00) >> 8,
			(sink_cap->edid_mHdmivsdb.mPhysicalAddress & 0x00F0) >> 4,
			sink_cap->edid_mHdmivsdb.mPhysicalAddress & 0x000F);
	n += sprintf(buf + n, "\n");

	/* hdcp */
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	n += sprintf(buf + n, "\n[HDCP]");
	n += sprintf(buf + n, "\n - HDCP2.2 Support: %s",
			g_hdmi_drv->hdmi_core->hdcp_ops.hdcp22_get_support() == 1 ?
			"YES" : "No");
#endif

	n += sprintf(buf + n, "\n");

	return n;
}

#ifndef SUPPORT_ONLY_HDMI14
static ssize_t _aw_hdmi_sink_scdc_parse(char *buf, ssize_t n)
{
	u8 scdc_val = 0;
	u16 value = 0;

	sink_edid_t *sink_cap = g_hdmi_drv->hdmi_core->mode.sink_cap;
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;

	if (!sink_cap) {
		n += sprintf(buf + n, "Edid parse failed, maybe can't read edid!\n");
		return n;
	}

	if (!core) {
		n += sprintf(buf + n, "%s core is null!!!\n", __func__);
		return n;
	}

	/* scdc info */
	n += sprintf(buf + n, "\n[SCDC V2.0]\n");
	if (!sink_cap->edid_mHdmiForumvsdb.mSCDC_Present) {
		n += sprintf(buf + n, "Sink unsupport scdc function.\n");
		return n;
	}

	core->dev_ops.scdc_read(0x20, 0x1, (u8 *)&scdc_val);
	n += sprintf(buf + n, " - Scrambling_Enable: %s\n",
		(scdc_val & 0x1) ? "enable" : "disable");
	n += sprintf(buf + n, " - TMDS_Bit_Clock_Ratio: %s\n",
		(scdc_val & 0x2) ? "1:40" : "1:10");

	scdc_val = 0;
	core->dev_ops.scdc_read(0x21, 0x1, (u8 *)&scdc_val);
	n += sprintf(buf + n, " - Scrambling_Status: %s\n",
		(scdc_val & 0x1) ? "enable" : "disable");

	scdc_val = 0;
	core->dev_ops.scdc_read(0x40, 0x1, (u8 *)&scdc_val);
	n += sprintf(buf + n, " - Clock_Detected: %s\n",
		(scdc_val & 0x1) ? "valid" : "invalid");
	n += sprintf(buf + n, " - Ch0_Locked: %s\n",
		(scdc_val & 0x2) ? "lock" : "unlock");
	n += sprintf(buf + n, " - Ch1_Locked: %s\n",
		(scdc_val & 0x4) ? "lock" : "unlock");
	n += sprintf(buf + n, " - Ch2_Locked: %s\n",
		(scdc_val & 0x8) ? "lock" : "unlock");

	scdc_val = 0;
	core->dev_ops.scdc_read(0x51, 0x1, (u8 *)&scdc_val);
	if (scdc_val & 0x80) {
		value = scdc_val & 0x7F;
		scdc_val = 0;
		core->dev_ops.scdc_read(0x50, 0x1, (u8 *)&scdc_val);
		value = (value << 7) + scdc_val;
		n += sprintf(buf + n, " - CH0 char error: %d\n", value);
	}

	scdc_val = 0;
	core->dev_ops.scdc_read(0x53, 0x1, (u8 *)&scdc_val);
	if (scdc_val & 0x80) {
		value = scdc_val & 0x7F;
		scdc_val = 0;
		core->dev_ops.scdc_read(0x52, 0x1, (u8 *)&scdc_val);
		value = (value << 7) + scdc_val;
		n += sprintf(buf + n, " - CH1 char error: %d\n", value);
	}

	scdc_val = 0;
	core->dev_ops.scdc_read(0x55, 0x1, (u8 *)&scdc_val);
	if (scdc_val & 0x80) {
		value = scdc_val & 0x7F;
		scdc_val = 0;
		core->dev_ops.scdc_read(0x54, 0x1, (u8 *)&scdc_val);
		value = (value << 7) + scdc_val;
		n += sprintf(buf + n, " - CH2 char error: %d\n", value);
	}

	scdc_val = 0;
	core->dev_ops.scdc_read(0x56, 0x1, (u8 *)&scdc_val);
	n += sprintf(buf + n, " - char error checksum: %d\n", scdc_val);

	return n;
}
#endif /* SUPPORT_ONLY_HDMI14 */

static ssize_t hdmi_sink_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += _aw_hdmi_sink_edid_parse(buf, n);

#ifndef SUPPORT_ONLY_HDMI14
	n += _aw_hdmi_sink_scdc_parse(buf, n);
#endif

	return n;

}

static DEVICE_ATTR(hdmi_sink, 0664,
		hdmi_sink_show, NULL);

static char *pixel_format_name[] = {
	"RGB",
	"YUV422",
	"YUV444",
	"YUV420",
};

static char *colorimetry_name[] = {
	"NULL",
	"ITU601",
	"ITU709",
	"XV_YCC601",
	"XV_YCC709",
	"S_YCC601",
	"ADOBE_YCC601",
	"ADOBE_RGB",
	"BT2020_Yc_Cbc_Crc",
	"BT2020_Y_CB_CR",
};

static ssize_t hdmi_rxsense_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 rxsense = g_hdmi_drv->hdmi_core->phy_ops.phy_get_rxsense();
	return sprintf(buf, "%u\n", rxsense);
}

static DEVICE_ATTR(rxsense, 0664,
		hdmi_rxsense_show, NULL);

static ssize_t hdmi_source_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	u32 temp = 0;
	int i;

	/* driver info */
	n += sprintf(buf + n, "[Version]\n");
	n += sprintf(buf + n, " - hardware: 2.0\n");
	n += sprintf(buf + n, " - software: 2.0.0\n");

	/* dts */
	n += sprintf(buf + n, "\n[DTS]\n");
	n += sprintf(buf + n, " - hdmi_cts: %d\n", g_hdmi_drv->aw_dts.hdmi_cts);
	n += sprintf(buf + n, " - cec_support: %d\n", g_hdmi_drv->aw_dts.cec_support);
	n += sprintf(buf + n, " - cec_super_standby: %d\n", g_hdmi_drv->aw_dts.cec_super_standby);
	n += sprintf(buf + n, " - support_hdcp: %d\n", g_hdmi_drv->aw_dts.support_hdcp);
	n += sprintf(buf + n, " - support_hdcp22: %d\n", g_hdmi_drv->aw_dts.support_hdcp22);
	n += sprintf(buf + n, " - ddc_ctrl_en: %d\n", g_hdmi_drv->ddc_ctrl_en);

	/* power */
	if (g_hdmi_drv->aw_power.power_count) {
		n += sprintf(buf + n, "\n[Power]\n");
		for (i = 0; i < g_hdmi_drv->aw_power.power_count; i++) {
			n += sprintf(buf + n, " - %s: %s\n",
					g_hdmi_drv->aw_power.power_name[i],
					regulator_is_enabled(g_hdmi_drv->aw_power.power_regu[i]) ?
					"enable" : "disable");
		}
	} else {
		n += sprintf(buf + n, "Warn: hdmi power count not set. check actual power status!\n");
	}

	/* software state */
	n += sprintf(buf + n, "\n[Software State]\n");
	n += sprintf(buf + n, " - boot hdmi: %d\n", aw_hdmi_drv_get_boot_enable());
	n += sprintf(buf + n, " - video on: %d\n", aw_hdmi_drv_get_video_enable());
	n += sprintf(buf + n, " - sw hpd: %d\n", _aw_hdmi_drv_get_hpd());
	n += sprintf(buf + n, " - hpd mask: 0x%x\n", aw_hdmi_drv_get_hpd_mask());
	n += sprintf(buf + n, " - hdmi enable: %d\n", aw_hdmi_drv_get_enable_mask());
	n += sprintf(buf + n, " - clock enable: %d\n", aw_hdmi_drv_get_clk_enable());
	n += sprintf(buf + n, " - suspend: %d\n", aw_hdmi_drv_get_suspend_mask());

	/* phy info */
	n += sprintf(buf + n, "\n[PHY]\n");

	n += sprintf(buf + n, " - hpd: %d\n",
		g_hdmi_drv->hdmi_core->phy_ops.get_hpd());

	n += sprintf(buf + n, " - rxsense: %d\n",
		g_hdmi_drv->hdmi_core->phy_ops.phy_get_rxsense());

	temp = g_hdmi_drv->hdmi_core->phy_ops.phy_get_pll_lock();
	n += sprintf(buf + n, " - state: %s\n", temp == 1 ? "lock" : "unlock");

	temp = g_hdmi_drv->hdmi_core->phy_ops.phy_get_power();
	n += sprintf(buf + n, " - power: %s\n", temp == 1 ? "on" : "off");

	/* video info */
	n += sprintf(buf + n, "\n[Video]\n");

	temp = g_hdmi_drv->hdmi_core->video_ops.get_vic_code();
	n += sprintf(buf + n, " - video format: %s\n", debug_get_video_name((int)temp));

	temp = g_hdmi_drv->hdmi_core->video_ops.get_color_space();
	n += sprintf(buf + n, " - color format: %s\n", pixel_format_name[temp]);

	n += sprintf(buf + n, " - color depth: %d\n",
		g_hdmi_drv->hdmi_core->video_ops.get_color_depth());

	temp = g_hdmi_drv->hdmi_core->video_ops.get_tmds_mode();
	n += sprintf(buf + n, " - tmds mode: %s\n", temp == 1 ? "hdmi" : "dvi");

#ifndef SUPPORT_ONLY_HDMI14
	temp = g_hdmi_drv->hdmi_core->video_ops.get_scramble();
	n += sprintf(buf + n, " - scramble: %s\n", temp == 1 ? "on": "off");
#endif

	temp = g_hdmi_drv->hdmi_core->video_ops.get_avmute();
	n += sprintf(buf + n, " - avmute: %s\n", temp == 1 ? "on" : "off");

	temp = g_hdmi_drv->hdmi_core->video_ops.get_pixel_repetion();
	n += sprintf(buf + n, " - pixel repetion: %s\n", temp == 1 ? "on" : "off");

	temp = g_hdmi_drv->hdmi_core->video_ops.get_color_metry();
	n += sprintf(buf + n, " - colorimetry: %s\n", colorimetry_name[temp]);

	/* auido info */
	n += sprintf(buf + n, "\n[Audio]\n");

	n += sprintf(buf + n, " - layout: %d\n",
		g_hdmi_drv->hdmi_core->audio_ops.get_layout());

	n += sprintf(buf + n, " - channel count: %d\n",
		g_hdmi_drv->hdmi_core->audio_ops.get_channel_count());

	n += sprintf(buf + n, " - sample freq: %d\n",
		g_hdmi_drv->hdmi_core->audio_ops.get_sample_freq());

	n += sprintf(buf + n, " - sample size: %d\n",
		g_hdmi_drv->hdmi_core->audio_ops.get_sample_size());

	n += sprintf(buf + n, " - acr n: %d\n",
		g_hdmi_drv->hdmi_core->audio_ops.get_acr_n());

	n += sprintf(buf + n, "\n");
	return n;
}

static DEVICE_ATTR(hdmi_source, 0664,
		hdmi_source_show, NULL);

static ssize_t hdmi_avmute_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n%s\n",
		"echo [value] > avmute",
		"-----value =1:avmute on; =0:avmute off");
}

static ssize_t hdmi_avmute_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strncmp(buf, "1", 1) == 0)
		g_hdmi_drv->hdmi_core->video_ops.set_avmute(1);
	else
		g_hdmi_drv->hdmi_core->video_ops.set_avmute(0);

	return count;
}

static DEVICE_ATTR(avmute, 0664,
		hdmi_avmute_show, hdmi_avmute_store);

static ssize_t phy_power_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n%s\n",
		 "echo [value] > phy_power",
		"-----value =1:phy power on; =0:phy power off");
}

static ssize_t phy_power_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strncmp(buf, "1", 1) == 0)
		g_hdmi_drv->hdmi_core->phy_ops.phy_set_power(1);
	else
		g_hdmi_drv->hdmi_core->phy_ops.phy_set_power(0);

	return count;
}

static DEVICE_ATTR(phy_power, 0664,
		phy_power_show, phy_power_store);

static ssize_t dvi_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "%s\n%s\n",
			 "echo [value] > dvi_mode",
			"-----value =0:HDMI mode; =1:DVI mode");
	return n;
}

static ssize_t dvi_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strncmp(buf, "1", 1) == 0)
		g_hdmi_drv->hdmi_core->video_ops.set_tmds_mode(1);
	else
		g_hdmi_drv->hdmi_core->video_ops.set_tmds_mode(0);

	return count;
}

static DEVICE_ATTR(dvi_mode, 0664,
		dvi_mode_show, dvi_mode_store);

#if IS_ENABLED(CONFIG_AW_HDMI2_LOG_BUFFER)
static ssize_t hdmi_log_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int i;
	char *addr = aw_hdmi_log_get_address();
	unsigned int start = aw_hdmi_log_get_start_index();
	unsigned int max_size = aw_hdmi_log_get_max_size();
	unsigned int used_size = aw_hdmi_log_get_used_size();

	printk("%s start.\n", __func__);
	if (used_size < max_size) {
		for (i = 0; i < used_size; i++)
			printk(KERN_CONT "%c", addr[i]);
	} else {
		for (i = start; i < max_size; i++)
			printk(KERN_CONT "%c", addr[i]);
		for (i = 0; i < start; i++)
			printk(KERN_CONT "%c", addr[i]);
	}
	printk("%s finish!\n", __func__);
	aw_hdmi_log_put_address();

	n += sprintf(buf + n, "hdmi_log enable: %d\n", aw_hdmi_log_get_enable());
	return n;
}

static ssize_t hdmi_log_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	if (strncmp(buf, "1", 1) == 0)
		aw_hdmi_log_set_enable(true);
	else if (strncmp(buf, "0", 1) == 0)
		aw_hdmi_log_set_enable(false);
	else
		hdmi_err("%s: echo 0/1 > hdmi_log\n", __func__);

	return count;
}

static DEVICE_ATTR(hdmi_log, 0664,
		hdmi_log_show, hdmi_log_store);
#endif

static struct attribute *hdmi_attributes[] = {
	&dev_attr_reg_dump.attr,
	&dev_attr_read.attr,
	&dev_attr_write.attr,
	&dev_attr_phy_write.attr,
	&dev_attr_phy_read.attr,
	&dev_attr_set_ddc.attr,
#ifndef SUPPORT_ONLY_HDMI14
	&dev_attr_scdc_read.attr,
	&dev_attr_scdc_write.attr,
#endif
	&dev_attr_debug.attr,
	&dev_attr_hpd_mask.attr,
	&dev_attr_edid.attr,
	&dev_attr_edid_test.attr,

	&dev_attr_hdmi_sink.attr,
	&dev_attr_hdmi_source.attr,
	&dev_attr_rxsense.attr,
	&dev_attr_avmute.attr,
	&dev_attr_phy_power.attr,
	&dev_attr_dvi_mode.attr,

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	&dev_attr_hdcp_status.attr,
	&dev_attr_hdcp_type.attr,
	&dev_attr_hdcp_enable.attr,
	&dev_attr_hdcp_dump.attr,
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	&dev_attr_esm_dump.attr,
	&dev_attr_hpi_read.attr,
	&dev_attr_hpi_write.attr,
#endif
#endif

#if IS_ENABLED(CONFIG_HDMI2_FREQ_SPREAD_SPECTRUM)
	&dev_attr_ss_amp.attr,
	&dev_attr_ss_version.attr,
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_LOG_BUFFER)
	&dev_attr_hdmi_log.attr,
#endif
	NULL
};

static struct attribute_group hdmi_attribute_group = {
	.name = "attr",
	.attrs = hdmi_attributes
};

static int _aw_hdmi_dev_open(struct inode *inode, struct file *filp)
{
	struct file_ops *fops;

	fops = kmalloc(sizeof(struct file_ops), GFP_KERNEL | __GFP_ZERO);
	if (!fops) {
		hdmi_err("%s: memory allocated for hdmi fops failed!\n", __func__);
		return -EINVAL;
	}

	filp->private_data = fops;

	return 0;
}

static int _aw_hdmi_dev_release(struct inode *inode, struct file *filp)
{
	struct file_ops *fops = (struct file_ops *)filp->private_data;

	kfree(fops);
	return 0;
}

static ssize_t _aw_hdmi_dev_read(struct file *file, char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static ssize_t _aw_hdmi_dev_write(struct file *file, const char __user *buf,
						size_t count,
						loff_t *ppos)
{
	return -EINVAL;
}

static int _aw_hdmi_dev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}

static long _aw_hdmi_dev_ioctl_inner(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct file_ops *fops = (struct file_ops *)filp->private_data;
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI) || IS_ENABLED(CONFIG_AW_HDMI2_LOG_BUFFER)
	unsigned long *p_arg = (unsigned long *)arg;
#endif
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	struct hdmi_hdcp_info hdcp_info;
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	/* for hdcp keys */
	unsigned int key_size;
#endif
#endif

	if (!fops) {
		hdmi_err("%s: param fops is null!!!\n", __func__);
		return -EINVAL;
	}

	switch (cmd) {
	case AW_IOCTL_HDMI_NULL:
		fops->ioctl_cmd = AW_IOCTL_HDMI_NULL;
		break;

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	case AW_IOCTL_HDMI_HDCP22_LOAD_FW:
		fops->ioctl_cmd = AW_IOCTL_HDMI_HDCP22_LOAD_FW;
		if (p_arg[1] > gHdcp_esm_fw_size) {
			hdmi_err("%s: hdcp22 firmware is too big! arg_size:%lu esm_size:%d\n",
				__func__, p_arg[1], gHdcp_esm_fw_size);
			return -EINVAL;
		}

		key_size = p_arg[1];
		memset(gHdcp_esm_fw_vir_addr, 0, gHdcp_esm_fw_size);
		if (copy_from_user((void *)gHdcp_esm_fw_vir_addr,
					(void __user *)p_arg[0], key_size)) {
			hdmi_err("%s: copy from user fail when hdcp load firmware!!!\n", __func__);
			return -EINVAL;
		}
		hdmi_inf("ioctl hdcp22 load firmware has commpleted!\n");
		break;
#endif

	case AW_IOCTL_HDMI_HDCP_ENABLE:
		aw_hdmi_drv_set_hdcp_state(AW_HDCP_ING);
		aw_hdmi_drv_hdcp_enable(g_hdmi_drv->hdmi_core, 1);
		break;

	case AW_IOCTL_HDMI_HDCP_DISABLE:
		aw_hdmi_drv_hdcp_enable(g_hdmi_drv->hdmi_core, 0);
		aw_hdmi_drv_set_hdcp_state(AW_HDCP_DISABLE);
		break;

	case AW_IOCTL_HDMI_HDCP_INFO:
		hdcp_info.hdcp_type = aw_hdmi_drv_get_hdcp_type(g_hdmi_drv->hdmi_core);
		hdcp_info.hdcp_status = (unsigned int)aw_hdmi_drv_get_hdcp_state();

		if (copy_to_user((void __user *)p_arg[0], (void *)&hdcp_info,
								sizeof(struct hdmi_hdcp_info))) {
			hdmi_err("%s: copy to user fail when get hdcp info!!!\n", __func__);
			return -EINVAL;
		}

		break;
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_LOG_BUFFER)
	case AW_IOCTL_HDMI_GET_LOG_SIZE:
		{
			unsigned int size = aw_hdmi_log_get_max_size();

			if (copy_to_user((void __user *)p_arg[0], (void *)&size, sizeof(unsigned int))) {
				hdmi_err("%s: copy to user fail when get hdmi log size!\n", __func__);
				return -EINVAL;
			}
			break;
		}

	case AW_IOCTL_HDMI_GET_LOG:
		{
			char *addr = aw_hdmi_log_get_address();
			unsigned int start = aw_hdmi_log_get_start_index();
			unsigned int max_size = aw_hdmi_log_get_max_size();
			unsigned int used_size = aw_hdmi_log_get_used_size();

			if (used_size < max_size) {
				if (copy_to_user((void __user *)p_arg[0], (void *)addr, used_size)) {
					aw_hdmi_log_put_address();
					hdmi_err("%s: copy to user fail when get hdmi log!\n", __func__);
					return -EINVAL;
				}
			} else {
				if (copy_to_user((void __user *)p_arg[0], (void *)(addr + start),
							max_size - start)) {
					aw_hdmi_log_put_address();
					hdmi_err("%s: copy to user fail when get hdmi log!\n", __func__);
					return -EINVAL;
				}
				if (copy_to_user((void __user *)(p_arg[0] + (max_size - start)),
							(void *)addr, start)) {
					aw_hdmi_log_put_address();
					hdmi_err("%s: copy to user fail when get hdmi log!\n", __func__);
					return -EINVAL;
				}
			}
			aw_hdmi_log_put_address();
			break;
		}
#endif

	default:
		hdmi_err("%s: cmd %d invalid\n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}

static long _aw_hdmi_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long arg64[3] = {0};

	if (copy_from_user((void *)arg64, (void __user *)arg,
						3 * sizeof(unsigned long))) {
		hdmi_err("%s: copy from user fail when hdmi ioctl!!!\n", __func__);
		return -EFAULT;
	}

	return _aw_hdmi_dev_ioctl_inner(filp, cmd, (unsigned long)arg64);
}

#if IS_ENABLED(CONFIG_COMPAT)
static long _aw_hdmi_dev_compat_ioctl(struct file *filp, unsigned int cmd,
						unsigned long arg)
{
	compat_uptr_t arg32[3] = {0};
	unsigned long arg64[3] = {0};

	if (!arg)
		return _aw_hdmi_dev_ioctl_inner(filp, cmd, 0);

	if (copy_from_user((void *)arg32, (void __user *)arg,
						3 * sizeof(compat_uptr_t))) {
		hdmi_err("%s: copy from user fail when hdmi compat ioctl!!!\n", __func__);
		return -EFAULT;
	}

	arg64[0] = (unsigned long)arg32[0];
	arg64[1] = (unsigned long)arg32[1];
	arg64[2] = (unsigned long)arg32[2];
	return _aw_hdmi_dev_ioctl_inner(filp, cmd, (unsigned long)arg64);
}
#endif

static const struct file_operations aw_hdmi_fops = {
	.owner		    = THIS_MODULE,
	.open		    = _aw_hdmi_dev_open,
	.release	    = _aw_hdmi_dev_release,
	.write		    = _aw_hdmi_dev_write,
	.read		    = _aw_hdmi_dev_read,
	.mmap		    = _aw_hdmi_dev_mmap,
	.unlocked_ioctl	= _aw_hdmi_dev_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl	= _aw_hdmi_dev_compat_ioctl,
#endif
};

static int __init aw_hdmi_module_init(void)
{
	int ret = 0;

	/* Create and add a character device */
	alloc_chrdev_region(&aw_dev_hdmi.hdmi_devid, 0, 1, "hdmi");/* corely for device number */
	aw_dev_hdmi.hdmi_cdev = cdev_alloc();
	if (!aw_dev_hdmi.hdmi_cdev) {
		hdmi_err("%s: device register cdev_alloc failed!!!\n", __func__);
		return -1;
	}

	cdev_init(aw_dev_hdmi.hdmi_cdev, &aw_hdmi_fops);
	aw_dev_hdmi.hdmi_cdev->owner = THIS_MODULE;

	/* Create a path: /proc/device/hdmi */
	if (cdev_add(aw_dev_hdmi.hdmi_cdev, aw_dev_hdmi.hdmi_devid, 1)) {
		hdmi_err("%s: device register cdev_add failed!!!\n", __func__);
		return -1;
	}

	/* Create a path: sys/class/hdmi */
	aw_dev_hdmi.hdmi_class = class_create(THIS_MODULE, "hdmi");
	if (IS_ERR(aw_dev_hdmi.hdmi_class)) {
		hdmi_err("%s: device register class_create failed!!!\n", __func__);
		return -1;
	}

	/* Create a path "sys/class/hdmi/hdmi" */
	aw_dev_hdmi.hdmi_device = device_create(aw_dev_hdmi.hdmi_class, NULL, aw_dev_hdmi.hdmi_devid, NULL, "hdmi");
	if (!aw_dev_hdmi.hdmi_device) {
		hdmi_err("%s: device register device_create failed!!!\n", __func__);
		return -1;
	}

	/* Create a path: sys/class/hdmi/hdmi/attr */
	ret = sysfs_create_group(&aw_dev_hdmi.hdmi_device->kobj, &hdmi_attribute_group);
	if (ret) {
		hdmi_err("%s: device register sysfs_create_group failed!!!\n", __func__);
		return -1;
	}

	ret = platform_driver_register(&dw_hdmi_pdrv);
	if (ret != 0) {
		hdmi_err("%s: device register platform_driver_register failed!!!\n", __func__);
		return -1;
	}

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	ret = platform_driver_register(&aw_hdmi_cec_driver);
	if (ret != 0) {
		hdmi_err("%s: cec platform_driver_register failed!!!\n", __func__);
		return -1;
	}
#endif

	hdmi_inf("hdmi module init end.\n");

	return ret;
}

static void __exit aw_hdmi_module_exit(void)
{
	aw_hdmi_drv_remove(g_hdmi_drv->pdev);

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	platform_driver_unregister(&aw_hdmi_cec_driver);
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	dma_free_coherent(g_hdmi_drv->parent_dev,
				HDCP22_DATA_SIZE,
				&g_hdmi_drv->hdmi_core->mode.pHdcp.esm_data_phy_addr,
				GFP_KERNEL | __GFP_ZERO);
	dma_free_coherent(g_hdmi_drv->parent_dev,
					HDCP22_FIRMWARE_SIZE,
					&g_hdmi_drv->hdmi_core->mode.pHdcp.esm_firm_phy_addr,
					GFP_KERNEL | __GFP_ZERO);
#endif
	aw_hdmi_core_exit(g_hdmi_drv->hdmi_core);

	kfree(g_hdmi_drv);

	platform_driver_unregister(&dw_hdmi_pdrv);

	sysfs_remove_group(&aw_dev_hdmi.hdmi_device->kobj, &hdmi_attribute_group);

	device_destroy(aw_dev_hdmi.hdmi_class, aw_dev_hdmi.hdmi_devid);

	class_destroy(aw_dev_hdmi.hdmi_class);

	cdev_del(aw_dev_hdmi.hdmi_cdev);

#if IS_ENABLED(CONFIG_AW_HDMI2_LOG_BUFFER)
	aw_hdmi_log_exit();
#endif
}

late_initcall(aw_hdmi_module_init);
module_exit(aw_hdmi_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_AUTHOR("liangxianhui");
MODULE_DESCRIPTION("aw hdmi tx module driver");
MODULE_VERSION("2.0");

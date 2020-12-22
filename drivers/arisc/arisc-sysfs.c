/*
 * ARISC subsystem, sysfs interface
 *
 * Copyright (C) 20015 AllWinnertech
 * Author: Xiafeng <xiafeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "arisc_i.h"

#if defined CONFIG_ARCH_SUN8IW1P1
static unsigned int arisc_debug_baudrate = 57600;
#else
static unsigned int arisc_debug_baudrate = 115200;
#endif
unsigned int arisc_debug_level = 2;

unsigned int arisc_debug_dram_crc_en = 0;
static unsigned int arisc_debug_dram_crc_srcaddr = 0x40000000;
static unsigned int arisc_debug_dram_crc_len = (1024 * 1024);
static unsigned int arisc_debug_dram_crc_error = 0;
static unsigned int arisc_debug_dram_crc_total_count = 0;
static unsigned int arisc_debug_dram_crc_error_count = 0;

static unsigned char arisc_version[40] = "arisc defualt version";

static unsigned int arisc_pll = 0;

static u8 regaddr = 0;
#if defined CONFIG_ARCH_SUN8IW1P1
static struct arisc_p2wi_block_cfg block_cfg;
static u8 data = 0;
#else
static struct arisc_rsb_block_cfg block_cfg;
static u32 data = 0;
#endif

/* for save power check configuration */
struct standby_info_para arisc_powchk_back;

static ssize_t
arisc_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t size = 0;

	size = sprintf(buf, "%s\n", arisc_version);

	return size;
}

static ssize_t
arisc_debug_mask_show(struct device *dev, struct device_attribute *attr,
                      char *buf)
{
	ssize_t size = 0;

	size = sprintf(buf, "%u\n", arisc_debug_level);

	return size;
}

static ssize_t
arisc_debug_mask_store(struct device *dev, struct device_attribute *attr,
                       const char *buf, size_t size)
{
	u32 value = 0;

	sscanf(buf, "%u", &value);
	if ((value < 0) || (value > 3)) {
		ARISC_WRN("invalid debug mask [%d] to set\n", value);
		return size;
	}

	arisc_debug_level = value;
	arisc_set_debug_level(arisc_debug_level);
	ARISC_LOG("debug_mask change to %d\n", arisc_debug_level);

	return size;
}

static ssize_t
arisc_debug_baudrate_show(struct device *dev, struct device_attribute *attr,
                          char *buf)
{
	ssize_t size = 0;

	size = sprintf(buf, "%u\n", arisc_debug_baudrate);

	return size;
}

static ssize_t
arisc_debug_baudrate_store(struct device *dev, struct device_attribute *attr,
                           const char *buf, size_t size)
{
	u32 value = 0;

	sscanf(buf, "%u", &value);
	if ((value != 115200) && (value != 57600) && (value != 9600)) {
		ARISC_WRN("invalid uart baudrate:%d to set\n", value);
		return size;
	}

	arisc_debug_baudrate = value;
	arisc_set_uart_baudrate(arisc_debug_baudrate);
	ARISC_LOG("debug_baudrate change to %d\n", arisc_debug_baudrate);

	return size;
}

static ssize_t
arisc_dram_crc_paras_show(struct device *dev, struct device_attribute *attr,
                          char *buf)
{
	ssize_t size = 0;

	size = sprintf(buf, "enable:0x%x srcaddr:0x%x lenght:0x%x\n",
	               arisc_debug_dram_crc_en, arisc_debug_dram_crc_srcaddr,
	               arisc_debug_dram_crc_len);

	return size;
}

static ssize_t
arisc_dram_crc_paras_store(struct device *dev, struct device_attribute *attr,
                           const char *buf, size_t size)
{
	u32 crc_en = 0;
	u32 crc_srcaddr = 0;
	u32 crc_len = 0;

	sscanf(buf, "%x %x %x\n", &crc_en, &crc_srcaddr, &crc_len);

	if ((crc_en != 0) && (crc_en != 1)) {
		ARISC_WRN("invalid debug dram crc paras:%x %x %x to set\n",
		          crc_en, crc_srcaddr, crc_len);

		return size;
	}

	arisc_debug_dram_crc_en = crc_en;
	arisc_debug_dram_crc_srcaddr = crc_srcaddr;
	arisc_debug_dram_crc_len = crc_len;
	arisc_set_dram_crc_paras(arisc_debug_dram_crc_en,
	                         arisc_debug_dram_crc_srcaddr,
	                         arisc_debug_dram_crc_len);
	ARISC_LOG("dram crc enable:0x%x, src:0x%x, len:0x%x\n",
	          arisc_debug_dram_crc_en, arisc_debug_dram_crc_srcaddr,
	          arisc_debug_dram_crc_len);

	return size;
}

static ssize_t
arisc_dram_crc_result_show(struct device *dev, struct device_attribute *attr,
                           char *buf)
{
	arisc_query_dram_crc_result(&arisc_debug_dram_crc_error,
	                            &arisc_debug_dram_crc_total_count,
	                            &arisc_debug_dram_crc_error_count);
	return sprintf(buf, "dram info:\n" \
	                    "  enable %u\n" \
	                    "  error %u\n" \
	                    "  total count %u\n" \
	                    "  error count %u\n" \
	                    "  src:%p\n" \
	                    "  len:0x%x\n", \
	               arisc_debug_dram_crc_en,
	               arisc_debug_dram_crc_error,
	               arisc_debug_dram_crc_total_count,
	               arisc_debug_dram_crc_error_count,
	               (void *)arisc_debug_dram_crc_srcaddr,
	               arisc_debug_dram_crc_len);
}

static ssize_t
arisc_dram_crc_result_store(struct device *dev, struct device_attribute *attr,
                            const char *buf, size_t size)
{
	u32 error = 0;
	u32 total_count = 0;
	u32 error_count = 0;

	sscanf(buf, "%u %u %u", &error, &total_count, &error_count);
	if ((error != 0) && (error != 1)) {
		ARISC_WRN("invalid dram crc result [%d] [%d] [%d] to set\n",
		          error, total_count, error_count);
		return size;
	}

	arisc_debug_dram_crc_error = error;
	arisc_debug_dram_crc_total_count = total_count;
	arisc_debug_dram_crc_error_count = error_count;
	arisc_set_dram_crc_result(arisc_debug_dram_crc_error,
	                          arisc_debug_dram_crc_total_count,
	                          arisc_debug_dram_crc_error_count);
	ARISC_LOG("crc result change to error:%u total count:%u error count:%u\n",
	          arisc_debug_dram_crc_error, arisc_debug_dram_crc_total_count,
	          arisc_debug_dram_crc_error_count);

	return size;
}

static ssize_t
arisc_power_enable_show(struct device *dev, struct device_attribute *attr,
                        char *buf)
{
	return sprintf(buf, "power enable 0x%x\n", \
	               arisc_powchk_back.power_state.enable);
}

static ssize_t
arisc_power_enable_store(struct device *dev, struct device_attribute *attr,
                         const char *buf, size_t size)
{
	unsigned long value;

	if (kstrtoul(buf, 0, &value) < 0) {
		ARISC_ERR("illegal value, only one para support\n");
		return -EINVAL;
	}

	if (value & ~(CPUS_ENABLE_POWER_EXP | CPUS_WAKEUP_POWER_STA | CPUS_WAKEUP_POWER_CSM)) {
		ARISC_ERR("invalid format, 'enable' should:\n"\
		          "  bit31:enable power check function during standby\n"\
		          "  bit1: enable wakeup when power state exception\n"\
		          "  bit0: enable wakeup when power consume exception\n");
		return size;
	}

	arisc_powchk_back.power_state.enable = value;
	arisc_set_standby_power_cfg(&arisc_powchk_back);
	ARISC_LOG("standby_power_set enable:0x%x\n", arisc_powchk_back.power_state.enable);

	return size;
}

static ssize_t
arisc_power_state_show(struct device *dev, struct device_attribute *attr,
                       char *buf)
{
	return sprintf(buf, "power regs 0x%x\n", \
	               arisc_powchk_back.power_state.power_reg);
}

static ssize_t
arisc_power_state_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t size)
{
	unsigned long value;

	if (kstrtoul(buf, 0, &value) < 0) {
		ARISC_ERR("illegal value, only one para support");
		return -EINVAL;
	}
	arisc_powchk_back.power_state.power_reg = value;

	arisc_set_standby_power_cfg(&arisc_powchk_back);
	ARISC_LOG("standby_power_set power_state 0x%x\n",
	          arisc_powchk_back.power_state.power_reg);

	return size;
}

static ssize_t
arisc_power_consum_show(struct device *dev, struct device_attribute *attr,
                        char *buf)
{
	return sprintf(buf, "power consume %dmw\n", \
	               arisc_powchk_back.power_state.system_power);
}

static ssize_t
arisc_power_consum_store(struct device *dev, struct device_attribute *attr,
                         const char *buf, size_t size)
{
	unsigned long value;

	if (kstrtoul(buf, 0, &value) < 0) {
		if (1 != sscanf(buf, "%lu", &value)) {
			ARISC_ERR("illegal value, only one para support");
			return -EINVAL;
		}
	}
	arisc_powchk_back.power_state.system_power = value;

	arisc_set_standby_power_cfg(&arisc_powchk_back);
	ARISC_LOG("standby_power_set power_consum %dmw\n",
	          arisc_powchk_back.power_state.system_power);

	return size;
}

#if (defined CONFIG_ARCH_SUN9IW1P1)
#define SST_POWER_MASK 0xffffffff
static const unsigned char pmu_powername0[][8] = {
	"dc5ldo", "dcdc1", "dcdc2",  "dcdc3",  "dcdc4", "dcdc5", "aldo1", "aldo2",
	"eldo1",  "eldo2", "eldo3",  "dldo1",  "dldo2", "aldo3", "swout", "dc1sw",
};
static const unsigned char pmu_powername1[][8] = {
	"dcdca",  "dcdcb", "dcdcc",  "dcdcd",  "dcdce", "aldo1", "aldo2", "aldo3",
	"bldo1",  "bldo2", "bldo3",  "bldo4",  "cldo1", "cldo2", "cldo3", "swout",
};
#elif (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || \
      (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW7P1) || \
      (defined CONFIG_ARCH_SUN8IW9P1)
#define SST_POWER_MASK 0x0007ffff
static const unsigned char pmu_powername0[][8] = {
	"dc5ldo", "dcdc1",  "dcdc2",  "dcdc3", "dcdc4", "dcdc5", "aldo1", "aldo2",
	"eldo1",  "eldo2",  "eldo3",  "dldo1", "dldo2", "dldo3", "dldo4", "dc1sw",
	"aldo3",  "io0ldo", "io1ldo",
};
#elif (defined CONFIG_ARCH_SUN8IW6P1)
#define SST_POWER_MASK 0x00ffffff
static const unsigned char pmu_powername0[][8] = {
	"dcdc1",  "dcdc2",  "dcdc3",  "dcdc4", "dcdc5", "dcdc6", "dcdc7", "resev0",
	"eldo1",  "eldo2",  "eldo3",  "dldo1", "dldo2", "dldo3", "dldo4", "dc1sw",
	"resev1", "resev2", "fldo1",  "fldo2", "fldo3", "aldo1", "aldo2", "aldo3",
	"io0ldo", "io1ldo",
};
#endif

static ssize_t
arisc_power_trueinfo_show(struct device *dev, struct device_attribute *attr,
                          char *buf)
{
#define PRINT_ON_AXP "these power on during standby:\n  axp main:"
#define PRINT_OFF_AXP "\nthese power off during standby:\n  axp main:"

	unsigned int i, count, power_reg;
	standby_info_para_t sst_info;
	unsigned char pmu_buf[320];
	unsigned char *pbuf;

	arisc_query_standby_power(&sst_info);
	power_reg = sst_info.power_state.power_reg;
	power_reg &= SST_POWER_MASK;

	/* print the pmu power on state */
	strcpy(pmu_buf, PRINT_ON_AXP);
	pbuf = pmu_buf + sizeof(PRINT_ON_AXP) - 1;
	count = 0;
	for (i = 0; i < ARRY_SIZE(pmu_powername0); i++) {
		if (power_reg & (1 << i)) {
			strncpy(pbuf, pmu_powername0[i], 8);
			pbuf += strlen(pmu_powername0[i]);
			*pbuf++ = ',';
			count++;
		}
	}
	if (count) {
		strcpy(--pbuf, ""); /* rollback the last ',' */
	} else {
		strcpy(pbuf, "(null)");
		pbuf += sizeof("(null)") - 1;
	}

#if (defined CONFIG_ARCH_SUN9IW1P1)
	strcpy(pbuf, "\n  axp slave:");
	pbuf += sizeof("\n  axp slave:") - 1;
	count = 0;
	for (i = 0; i < ARRY_SIZE(pmu_powername1); i++) {
		if (power_reg & (1 << (16 + i))) {
			strncpy(pbuf, pmu_powername1[i], 8);
			pbuf += strlen(pmu_powername1[i]);
			*pbuf++ = ',';
			count++;
		}
	}
	if (count) {
		strcpy(--pbuf, "\n"); /* rollback the last ',' */
	} else {
		strcpy(pbuf, "(null)");
		pbuf += sizeof("(null)") - 1;
	}
#endif

	/* print the pmu power off state */
	power_reg ^= SST_POWER_MASK;

	strcpy(pbuf, PRINT_OFF_AXP);
	pbuf += sizeof(PRINT_OFF_AXP) - 1;
	count = 0;
	for (i = 0; i < ARRY_SIZE(pmu_powername0); i++) {
		if (power_reg & (1 << i)) {
			strncpy(pbuf, pmu_powername0[i], 8);
			pbuf += strlen(pmu_powername0[i]);
			*pbuf++ = ',';
			count++;
		}
	}
	if (count) {
		strcpy(--pbuf, ""); /* rollback the last ',' */
	} else {
		strcpy(pbuf, "(null)");
		pbuf += sizeof("(null)") - 1;
	}
#if (defined CONFIG_ARCH_SUN9IW1P1)
	strcpy(pbuf, "\n  axp slave:");
	pbuf += sizeof("\n  axp slave:") - 1;
	count = 0;
	for (i = 0; i < ARRY_SIZE(pmu_powername1); i++) {
		if (power_reg & (1 << (16 + i))) {
			strncpy(pbuf, pmu_powername1[i], 8);
			pbuf += strlen(pmu_powername1[i]);
			*pbuf++ = ',';
			count++;
		}
	}
	if (count) {
		strcpy(--pbuf, ""); /* rollback the last ',' */
	} else {
		strcpy(pbuf, "(null)");
		pbuf += sizeof("(null)") - 1;
	}
#endif

	return sprintf(buf, "power info:\n" \
	                    "  enable 0x%x\n" \
	                    "  regs 0x%x\n" \
	                    "  power consume %dmw\n" \
	                    "%s\n", \
	               sst_info.power_state.enable, \
	               sst_info.power_state.power_reg, \
	               sst_info.power_state.system_power, \
	               pmu_buf);
}

static ssize_t
arisc_freq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t size = 0;

	struct clk *pll = NULL;

#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1)
	if (arisc_pll == 1)
		pll = clk_get(NULL, "pll1");
#elif defined CONFIG_ARCH_SUN9IW1P1
	if (arisc_pll == 1)
		pll = clk_get(NULL, "pll1");
	else if (arisc_pll == 2)
		pll = clk_get(NULL, "pll2");
#elif (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW7P1)
	if (arisc_pll == 1)
		pll = clk_get(NULL, "pll_cpu");
#elif (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN8IW9P1)
	if (arisc_pll == 1)
		pll = clk_get(NULL, "pll_cpu0");
	else if (arisc_pll == 2)
		pll = clk_get(NULL, "pll_cpu1");
#endif

	if (!pll || IS_ERR(pll)) {
		ARISC_ERR("try to get pll%u failed!\n", arisc_pll);
		return size;
	}

	size = sprintf(buf, "%u\n", (unsigned int)clk_get_rate(pll));

	return size;
}

static ssize_t
arisc_freq_store(struct device *dev, struct device_attribute *attr,
                 const char *buf, size_t size)
{
	u32 freq = 0;
	u32 pll  = 0;
	u32 mode = 0;
	u32 ret = 0;

	sscanf(buf, "%u %u", &pll, &freq);

#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || \
    (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW7P1)
	if ((pll != 1) || (freq < 0) || (freq > 3000000)) {
		ARISC_WRN("invalid pll [%u] or freq [%u] to set, only support "
		          "pll1, freq [0, 3000000]KHz\n", pll, freq);
		ARISC_WRN("echo like: echo pll freq > freq\n");
		return size;
	}
#elif (defined CONFIG_ARCH_SUN9IW1P1) || (defined CONFIG_ARCH_SUN8IW6P1) || \
      (defined CONFIG_ARCH_SUN8IW9P1)
	if (((pll != 1) && (pll != 2)) || (freq < 0) || (freq > 3000000)) {
		ARISC_WRN("invalid pll [%u] or freq [%u] to set, only support "
		          "pll1 and pll2, freq [0, 3000000]KHz\n", pll, freq);
		ARISC_WRN("echo like: echo pll freq > freq\n");
		return size;
	}
#endif

	arisc_pll = pll;
	ret = arisc_dvfs_set_cpufreq(freq, pll, mode, NULL, NULL);
	ARISC_LOG("pll%u freq set to %u\n", pll, freq);
	if (ret)
		ARISC_ERR(" fail\n");
	else
		ARISC_LOG(" success\n");

	return size;
}

static ssize_t
arisc_rsb_read_block_data_show(struct device *dev, struct device_attribute *attr,
                               char *buf)
{
	ssize_t size = 0;
	u32 ret = 0;

#if defined CONFIG_ARCH_SUN8IW1P1
	if ((block_cfg.addr == NULL) || (block_cfg.data == NULL) || \
	    (*block_cfg.addr < 0) || (*block_cfg.addr > 0xff)) {
		ARISC_LOG("invalid p2wi paras, regaddr:0x%x\n",
		          block_cfg.addr ? *block_cfg.addr : 0);
		ARISC_LOG("echo like: echo regaddr > p2wi_read_block_data\n");
		return size;
	}

	ret = arisc_p2wi_read_block_data(&block_cfg);
	ARISC_LOG("p2wi read data:0x%x from regaddr:0x%x\n",
	          *block_cfg.data, *block_cfg.addr);
	if (ret)
		ARISC_ERR(" fail\n");
	else
		ARISC_LOG(" success\n");
#else
	if ((block_cfg.regaddr == NULL) || (block_cfg.data == NULL) || (block_cfg.devaddr > 0xff) ||
	    ((block_cfg.datatype !=  RSB_DATA_TYPE_BYTE) && (block_cfg.datatype !=  RSB_DATA_TYPE_HWORD) &&
	    (block_cfg.datatype !=  RSB_DATA_TYPE_WORD))) {
		ARISC_WRN("invalid paras, devaddr:0x%x, regaddr:0x%x, datatype:0x%x\n",
		          block_cfg.devaddr, block_cfg.regaddr ? *block_cfg.regaddr : 0, block_cfg.datatype);
		ARISC_WRN("echo like: echo devaddr regaddr datatype > rsb_read_block_data\n");
		return size;
	}

	ret = arisc_rsb_read_block_data(&block_cfg);
	ARISC_LOG("rsb read data:0x%x from devaddr:0x%x regaddr:0x%x\n",
	          *block_cfg.data, block_cfg.devaddr, *block_cfg.regaddr);
	if (ret)
		ARISC_ERR(" fail\n");
	else
		ARISC_LOG(" success\n");
#endif
	size = sprintf(buf, "%x\n", data);

	return size;
}

static ssize_t
arisc_rsb_read_block_data_store(struct device *dev, struct device_attribute *attr,
                                const char *buf, size_t size)
{
#if defined CONFIG_ARCH_SUN8IW1P1
	sscanf(buf, "%x", (u32 *)&regaddr);
	if ((regaddr < 0) || (regaddr > 0xff)) {
		ARISC_WRN("invalid paras, regaddr:0x%x\n", regaddr);
		ARISC_LOG("echo like: echo regaddr > p2wi_read_block_data\n");
		return size;
	}

	block_cfg.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	block_cfg.len = 1;
	block_cfg.addr = &regaddr;
	block_cfg.data = &data;

	ARISC_LOG("p2wi read regaddr:0x%x\n", *block_cfg.addr);
#else
	u32 devaddr = 0;
	u32 datatype = 0;

	sscanf(buf, "%x %x %x", &devaddr, (u32 *)&regaddr, &datatype);
	if ((devaddr > 0xff) || ((datatype !=  RSB_DATA_TYPE_BYTE) && (datatype !=  RSB_DATA_TYPE_HWORD) &&
	    (datatype !=  RSB_DATA_TYPE_WORD))) {
		ARISC_WRN("invalid paras to set, devaddr:0x%x, regaddr:0x%x, "
		          "datatype:0x%x\n", devaddr, regaddr, datatype);
		ARISC_WRN("echo like: echo devaddr regaddr datatype > rsb_read_block_data\n");
		return size;
	}

	block_cfg.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	block_cfg.datatype = datatype;
	block_cfg.len = 1;
	block_cfg.devaddr = devaddr;
	block_cfg.regaddr = &regaddr;
	block_cfg.data = &data;

	ARISC_LOG("rsb read data from devaddr:0x%x regaddr:0x%x\n", devaddr, regaddr);
#endif

	return size;
}

static ssize_t
arisc_rsb_write_block_data_show(struct device *dev, struct device_attribute *attr,
                                char *buf)
{
	ssize_t size = 0;
	u32 ret = 0;

#if defined CONFIG_ARCH_SUN8IW1P1
	if ((block_cfg.addr == NULL) || (block_cfg.data == NULL) || \
	    (*block_cfg.addr < 0) || (*block_cfg.addr > 0xff)) {
		ARISC_WRN("invalid paras, regaddr:0x%x, data:0x%x\n",
		          block_cfg.addr ? *block_cfg.addr : 0,
		          block_cfg.data ? *block_cfg.data : 0);
		ARISC_LOG("echo like: echo regaddr data > p2wi_write_block_data\n");
		return size;
	}

	block_cfg.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	block_cfg.len = 1;
	block_cfg.addr = &regaddr;
	block_cfg.data = &data;
	ret = arisc_p2wi_read_block_data(&block_cfg);
	ARISC_LOG("p2wi read data:0x%x from regaddr:0x%x\n",
	          *block_cfg.data, *block_cfg.addr);
	if (ret)
		ARISC_WRN(" fail\n");
	else
		ARISC_LOG(" success\n");
#else
	if ((block_cfg.regaddr == NULL) || (block_cfg.data == NULL) || (block_cfg.devaddr > 0xff) ||
	    ((block_cfg.datatype !=  RSB_DATA_TYPE_BYTE) && (block_cfg.datatype !=  RSB_DATA_TYPE_HWORD) &&
	    (block_cfg.datatype !=  RSB_DATA_TYPE_WORD))) {
		ARISC_WRN("invalid paras, devaddr:0x%x, regaddr:0x%x, "
		          "datatype:0x%x\n", block_cfg.devaddr,
		          block_cfg.regaddr ? *block_cfg.regaddr : 0, block_cfg.datatype);
		ARISC_WRN("echo like: echo devaddr regaddr data datatype > rsb_write_block_data\n");
		return size;
	}

	ret = arisc_rsb_read_block_data(&block_cfg);
	ARISC_LOG("rsb read data:0x%x from devaddr:0x%x regaddr:0x%x\n",
	          *block_cfg.data, block_cfg.devaddr, *block_cfg.regaddr);
	if (ret)
		ARISC_ERR(" fail\n");
	else
		ARISC_LOG(" success\n");
#endif
	size = sprintf(buf, "%x\n", data);

	return size;
}

static ssize_t
arisc_rsb_write_block_data_store(struct device *dev, struct device_attribute *attr,
                                 const char *buf, size_t size)
{
	u32 ret = 0;

#if defined CONFIG_ARCH_SUN8IW1P1
	sscanf(buf, "%x %x", (u32 *)&regaddr, (u32 *)&data);
	if ((regaddr < 0) || (regaddr > 0xff)) {
		ARISC_WRN("invalid paras, regaddr:0x%x, data:0x%x\n", regaddr, data);
		ARISC_WRN("echo like: echo regaddr data > p2wi_write_block_data\n");
		return size;
	}

	block_cfg.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	block_cfg.len = 1;
	block_cfg.addr = &regaddr;
	block_cfg.data = &data;
	ret = arisc_p2wi_write_block_data(&block_cfg);
	ARISC_LOG("p2wi write data:0x%x to regaddr:0x%x\n",
	          *block_cfg.data, *block_cfg.addr);
	if (ret)
		ARISC_WRN(" fail\n");
	else
		ARISC_LOG(" success\n");
#else
	u32 devaddr = 0;
	u32 datatype = 0;

	sscanf(buf, "%x %x %x %x", &devaddr, (u32 *)&regaddr, (u32 *)&data, &datatype);
	if ((devaddr > 0xff) ||
	    ((datatype !=  RSB_DATA_TYPE_BYTE) && (datatype != RSB_DATA_TYPE_HWORD) &&
	    (datatype !=  RSB_DATA_TYPE_WORD))) {
		ARISC_WRN("invalid paras, devaddr:0x%x, regaddr:0x%x, data:0x%x, "
		          "datatype:0x%x\n", devaddr, regaddr, data, datatype);
		ARISC_WRN("echo like: echo devaddr regaddr data datatype > rsb_write_block_data\n");
		return size;
	}

	block_cfg.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	block_cfg.datatype = datatype;
	block_cfg.len = 1;
	block_cfg.devaddr = devaddr;
	block_cfg.regaddr = &regaddr;
	block_cfg.data = &data;
	ret = arisc_rsb_write_block_data(&block_cfg);
	ARISC_LOG("rsb write data:0x%x to devaddr:0x%x regaddr:0x%x\n",
	          *block_cfg.data, block_cfg.devaddr, *block_cfg.regaddr);
	if (ret)
		ARISC_ERR(" fail\n");
	else
		ARISC_LOG(" success\n");
#endif

	return size;
}

static struct device_attribute sunxi_arisc_attrs[] = {
	__ATTR(version,                 S_IRUGO,            arisc_version_show,                 NULL),
	__ATTR(debug_mask,              S_IRUGO | S_IWUSR,  arisc_debug_mask_show,              arisc_debug_mask_store),
	__ATTR(debug_baudrate,          S_IRUGO | S_IWUSR,  arisc_debug_baudrate_show,          arisc_debug_baudrate_store),
	__ATTR(dram_crc_paras,          S_IRUGO | S_IWUSR,  arisc_dram_crc_paras_show,          arisc_dram_crc_paras_store),
	__ATTR(dram_crc_result,         S_IRUGO | S_IWUSR,  arisc_dram_crc_result_show,         arisc_dram_crc_result_store),
	__ATTR(sst_power_enable_mask,   S_IRUGO | S_IWUSR,  arisc_power_enable_show,            arisc_power_enable_store),
	__ATTR(sst_power_state_mask,    S_IRUGO | S_IWUSR,  arisc_power_state_show,             arisc_power_state_store),
	__ATTR(sst_power_consume_mask,  S_IRUGO | S_IWUSR,  arisc_power_consum_show,            arisc_power_consum_store),
	__ATTR(sst_power_real_info,     S_IRUGO,            arisc_power_trueinfo_show,          NULL),
	__ATTR(freq,                    S_IRUGO | S_IWUSR,  arisc_freq_show,                    arisc_freq_store),
	__ATTR(rsb_read_block_data,     S_IRUGO | S_IWUSR,  arisc_rsb_read_block_data_show,     arisc_rsb_read_block_data_store),
	__ATTR(rsb_write_block_data,    S_IRUGO | S_IWUSR,  arisc_rsb_write_block_data_show,    arisc_rsb_write_block_data_store),
};

void *arisc_version_store(const void *src, size_t count)
{
	memcpy((void *)arisc_version, src, count);

	return (void *)arisc_version;
}

void __init sunxi_arisc_sysfs(struct platform_device *pdev)
{
	unsigned int i;

	memset((void*)&block_cfg, 0, sizeof(block_cfg));
	for (i = 0; i < ARRAY_SIZE(sunxi_arisc_attrs); i++)
		device_create_file(&pdev->dev, &sunxi_arisc_attrs[i]);
}

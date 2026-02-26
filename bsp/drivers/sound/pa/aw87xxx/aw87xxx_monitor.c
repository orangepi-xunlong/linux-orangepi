// SPDX-License-Identifier: GPL-2.0
/* aw87xxx_monitor.c
 *
 * Copyright (c) 2021 AWINIC Technology CO., LTD
 *
 * Author: Barry <zhaozhongbo@awinic.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/power_supply.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/gameport.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include "aw87xxx.h"
#include "aw87xxx_log.h"
#include "aw87xxx_monitor.h"
#include "aw87xxx_dsp.h"
#include "aw87xxx_bin_parse.h"
#include "aw87xxx_device.h"

#define AW_MONITOT_BIN_PARSE_VERSION	"V0.1.0"

#define AW87XXX_MONITOR_NAME "awinic_monitor.bin"

#define AW_GET_32_DATA(w, x, y, z) \
	((uint32_t)((((uint8_t)w) << 24) | (((uint8_t)x) << 16) | \
	(((uint8_t)y) << 8) | ((uint8_t)z)))

/****************************************************************************
 *
 * aw87xxx monitor bin check
 *
 ****************************************************************************/

static int aw_monitor_get_ctrl_info(struct device *dev, uint32_t *switch_ctrl, uint32_t *count,
			uint32_t *time)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	switch (monitor->version) {
	case DATA_VERSION_V1:
		*time = monitor->monitor_hdr.monitor_time;
		*count = monitor->monitor_hdr.monitor_count;
		*switch_ctrl = monitor->monitor_hdr.monitor_switch;
		break;
	case AW_MONITOR_HDR_VER_0_1_2:
		*time = monitor->monitor_cfg.monitor_time;
		*count = monitor->monitor_cfg.monitor_count;
		*switch_ctrl = monitor->monitor_cfg.monitor_switch;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int aw_monitor_check_header_v_1_0_0(struct device *dev,
				const char *data, uint32_t data_len)
{
	int i = 0;
	struct aw_bin_header *header = (struct aw_bin_header *)data;

	if (header->bin_data_type != DATA_TYPE_MONITOR_ANALOG) {
		AW_DEV_LOGE(dev, "monitor data_type check error!");
		return -EINVAL;
	}

	if (header->bin_data_size != AW_MONITOR_HDR_DATA_SIZE) {
		AW_DEV_LOGE(dev, "monitor data_size error!");
		return -EINVAL;
	}

	if (header->data_byte_len != AW_MONITOR_HDR_DATA_BYTE_LEN) {
		AW_DEV_LOGE(dev, "monitor data_byte_len error!");
		return -EINVAL;
	}

	for (i = 0; i < AW_MONITOR_DATA_VER_MAX; i++) {
		if (header->bin_data_ver == i) {
			AW_LOGD("monitor bin_data_ver[0x%x]", i);
			break;
		}
	}
	if (i == AW_MONITOR_DATA_VER_MAX)
		return -EINVAL;

	return 0;
}

static int aw_monitor_check_data_v1_size(struct device *dev,
				const char *data, int32_t data_len)
{
	int32_t bin_header_len  = sizeof(struct aw_bin_header);
	int32_t monitor_header_len = sizeof(struct aw_monitor_header);
	int32_t monitor_data_len = sizeof(struct vmax_step_config);
	int32_t len = 0;
	struct aw_monitor_header *monitor_header = NULL;

	AW_DEV_LOGD(dev, "enter");

	if (data_len < bin_header_len + monitor_header_len) {
		AW_DEV_LOGE(dev, "bin len is less than aw_bin_header and monitoor_header,check failed");
		return -EINVAL;
	}

	monitor_header = (struct aw_monitor_header *)(data + bin_header_len);
	len = data_len - bin_header_len - monitor_header_len;
	if (len < monitor_header->step_count * monitor_data_len) {
		AW_DEV_LOGE(dev, "bin data len is not enough,check failed");
		return -EINVAL;
	}

	AW_DEV_LOGD(dev, "succeed");

	return 0;
}

static int aw_monitor_check_data_size(struct device *dev,
			const char *data, int32_t data_len)
{
	int ret = -1;
	struct aw_bin_header *header = (struct aw_bin_header *)data;

	switch (header->bin_data_ver) {
	case AW_MONITOR_DATA_VER:
		ret = aw_monitor_check_data_v1_size(dev, data, data_len);
		if (ret < 0)
			return ret;
		break;
	default:
		AW_DEV_LOGE(dev, "bin data_ver[0x%x] non support",
			header->bin_data_ver);
		return -EINVAL;
	}

	return 0;
}


static int aw_monitor_check_bin_header(struct device *dev,
				const char *data, int32_t data_len)
{
	int ret = -1;
	struct aw_bin_header *header = NULL;

	if (data_len < sizeof(struct aw_bin_header)) {
		AW_DEV_LOGE(dev, "bin len is less than aw_bin_header,check failed");
		return -EINVAL;
	}
	header = (struct aw_bin_header *)data;

	switch (header->header_ver) {
	case HEADER_VERSION_1_0_0:
		ret = aw_monitor_check_header_v_1_0_0(dev, data, data_len);
		if (ret < 0) {
			AW_DEV_LOGE(dev, "monitor bin haeder info check error!");
			return ret;
		}
		break;
	default:
		AW_DEV_LOGE(dev, "bin version[0x%x] non support",
			header->header_ver);
		return -EINVAL;
	}

	return 0;
}

static int aw_monitor_bin_check_sum(struct device *dev,
			const char *data, int32_t data_len)
{
	int i, data_sum = 0;
	uint32_t *check_sum = (uint32_t *)data;

	for (i = 4; i < data_len; i++)
		data_sum += data[i];

	if (*check_sum != data_sum) {
		AW_DEV_LOGE(dev, "check_sum[%d] is not equal to data_sum[%d]",
				*check_sum, data_sum);
		return -ENOMEM;
	}

	AW_DEV_LOGD(dev, "succeed");

	return 0;
}

static int aw_monitor_bin_check(struct device *dev,
				const char *monitor_data, uint32_t data_len)
{
	int ret = -1;

	if (monitor_data == NULL || data_len == 0) {
		AW_DEV_LOGE(dev, "none data to parse");
		return -EINVAL;
	}

	ret = aw_monitor_bin_check_sum(dev, monitor_data, data_len);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "bin data check sum failed");
		return ret;
	}

	ret = aw_monitor_check_bin_header(dev, monitor_data, data_len);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "bin data len check failed");
		return ret;
	}

	ret = aw_monitor_check_data_size(dev, monitor_data, data_len);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "bin header info check failed");
		return ret;
	}

	return 0;
}

/*****************************************************************************
 *
 * aw87xxx monitor header bin parse
 *
 *****************************************************************************/
static void aw_monitor_write_to_table_v1(struct device *dev,
			struct vmax_step_config *vmax_step,
			const char *vmax_data, uint32_t step_count)
{
	int i = 0;
	int index = 0;
	int vmax_step_size = (int)sizeof(struct vmax_step_config);

	for (i = 0; i < step_count; i++) {
		index = vmax_step_size * i;
		vmax_step[i].vbat_min =
			AW_GET_32_DATA(vmax_data[index + 3],
					vmax_data[index + 2],
					vmax_data[index + 1],
					vmax_data[index + 0]);
		vmax_step[i].vbat_max =
			AW_GET_32_DATA(vmax_data[index + 7],
					vmax_data[index + 6],
					vmax_data[index + 5],
					vmax_data[index + 4]);
		vmax_step[i].vmax_vol =
			AW_GET_32_DATA(vmax_data[index + 11],
					vmax_data[index + 10],
					vmax_data[index + 9],
					vmax_data[index + 8]);
	}

	for (i = 0; i < step_count; i++)
		AW_DEV_LOGD(dev, "vbat_min:%d, vbat_max%d, vmax_vol:0x%x",
			vmax_step[i].vbat_min,
			vmax_step[i].vbat_max,
			vmax_step[i].vmax_vol);
}

static int aw_monitor_parse_vol_data_v1(struct device *dev,
			struct aw_monitor *monitor, const char *monitor_data)
{
	uint32_t step_count = 0;
	const char *vmax_data = NULL;
	struct vmax_step_config *vmax_step = NULL;

	AW_DEV_LOGD(dev, "enter");

	step_count = monitor->monitor_hdr.step_count;
	if (step_count) {
		vmax_step = devm_kzalloc(dev, sizeof(struct vmax_step_config) * step_count,
					GFP_KERNEL);
		if (vmax_step == NULL) {
			AW_DEV_LOGE(dev, "vmax_cfg vmalloc failed");
			return -ENOMEM;
		}
		memset(vmax_step, 0,
			sizeof(struct vmax_step_config) * step_count);
	}

	vmax_data = monitor_data + sizeof(struct aw_bin_header) +
		sizeof(struct aw_monitor_header);
	aw_monitor_write_to_table_v1(dev, vmax_step, vmax_data, step_count);
	monitor->vmax_cfg = vmax_step;

	AW_DEV_LOGD(dev, "vmax_data parse succeed");

	return 0;
}

static int aw_monitor_parse_data_v1(struct device *dev,
			struct aw_monitor *monitor, const char *monitor_data)
{
	int ret = -1;
	int header_len = 0;
	struct aw_monitor_header *monitor_hdr = &monitor->monitor_hdr;

	header_len = sizeof(struct aw_bin_header);
	memcpy(monitor_hdr, monitor_data + header_len,
		sizeof(struct aw_monitor_header));

	AW_DEV_LOGD(dev, "monitor_switch:%d, monitor_time:%d (ms), monitor_count:%d, step_count:%d",
		monitor_hdr->monitor_switch, monitor_hdr->monitor_time,
		monitor_hdr->monitor_count, monitor_hdr->step_count);

	ret = aw_monitor_parse_vol_data_v1(dev, monitor, monitor_data);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "vmax_data parse failed");
		return ret;
	}

	monitor->bin_status = AW_MONITOR_CFG_OK;

	return 0;
}


static int aw_monitor_parse_v_0_0_1(struct device *dev,
			struct aw_monitor *monitor, const char *monitor_data)
{
	int ret = -1;
	struct aw_bin_header *header = (struct aw_bin_header *)monitor_data;

	switch (header->bin_data_ver) {
	case AW_MONITOR_DATA_VER:
		ret = aw_monitor_parse_data_v1(dev, monitor, monitor_data);
		if (ret < 0)
			return ret;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void aw_monitor_cfg_v_0_0_1_free(struct aw_monitor *monitor)
{
	struct aw87xxx *aw87xxx =
		container_of(monitor, struct aw87xxx, monitor);

	monitor->bin_status = AW_MONITOR_CFG_WAIT;
	memset(&monitor->monitor_hdr, 0,
		sizeof(struct aw_monitor_header));
	if (monitor->vmax_cfg) {
		devm_kfree(aw87xxx->dev, monitor->vmax_cfg);
		monitor->vmax_cfg = NULL;
	}
}

static void aw_monitor_write_data_to_table(struct device *dev,
		struct aw_table_info *table_info, const char *offset_ptr)
{
	int i = 0;
	int table_size = AW_TABLE_SIZE;

	for (i = 0; i < (int)(table_info->table_num * table_size); i += table_size) {
		table_info->aw_table[i / AW_TABLE_SIZE].min_val =
			AW_GET_16_DATA(offset_ptr[1 + i], offset_ptr[i]);
		table_info->aw_table[i / AW_TABLE_SIZE].max_val =
			AW_GET_16_DATA(offset_ptr[3 + i], offset_ptr[2 + i]);
		table_info->aw_table[i / AW_TABLE_SIZE].ipeak =
			AW_GET_16_DATA(offset_ptr[5 + i], offset_ptr[4 + i]);
		table_info->aw_table[i / AW_TABLE_SIZE].gain =
			AW_GET_16_DATA(offset_ptr[7 + i], offset_ptr[6 + i]);
		table_info->aw_table[i / AW_TABLE_SIZE].vmax =
			AW_GET_32_DATA(offset_ptr[11 + i], offset_ptr[10 + i],
				offset_ptr[9 + i], offset_ptr[8 + i]);
	}

	for (i = 0; i < table_info->table_num; i++)
		AW_DEV_LOGD(dev,
			"min_val:%d, max_val:%d, ipeak:0x%x, gain:0x%x, vmax:0x%x",
			table_info->aw_table[i].min_val,
			table_info->aw_table[i].max_val,
			table_info->aw_table[i].ipeak,
			table_info->aw_table[i].gain,
			table_info->aw_table[i].vmax);

}

static int aw_monitor_parse_vol_data_v_0_1_2(struct device *dev, const uint8_t *data)
{
	struct aw_monitor_hdr *monitor_hdr =
			(struct aw_monitor_hdr *)data;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor_cfg *monitor_cfg = &aw87xxx->monitor.monitor_cfg;
	struct aw_table_info *vol_info = &monitor_cfg->vol_info;

	AW_DEV_LOGD(dev, "===parse vol start ===");
	if (vol_info->aw_table != NULL) {
		devm_kfree(dev, vol_info->aw_table);
		vol_info->aw_table = NULL;
	}

	vol_info->aw_table = devm_kzalloc(dev,
					(monitor_hdr->vol_num * AW_TABLE_SIZE),
					GFP_KERNEL);
	if (vol_info->aw_table == NULL)
		return -ENOMEM;

	vol_info->table_num = monitor_hdr->vol_num;
	aw_monitor_write_data_to_table(dev, vol_info,
		&data[monitor_hdr->vol_offset]);
	AW_DEV_LOGD(dev, "===parse vol end ===");
	return 0;
}

static int aw_monitor_parse_temp_data_v_0_1_2(struct device *dev, const uint8_t *data)
{
	struct aw_monitor_hdr *monitor_hdr =
			(struct aw_monitor_hdr *)data;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor_cfg *monitor_cfg = &aw87xxx->monitor.monitor_cfg;
	struct aw_table_info *temp_info = &monitor_cfg->temp_info;

	AW_DEV_LOGD(dev, "===parse temp start ===");

	if (temp_info->aw_table != NULL) {
		devm_kfree(dev, temp_info->aw_table);
		temp_info->aw_table = NULL;
	}

	temp_info->aw_table = devm_kzalloc(dev,
					(monitor_hdr->temp_num * AW_TABLE_SIZE),
					GFP_KERNEL);
	if (temp_info->aw_table == NULL)
		return -ENOMEM;

	temp_info->table_num = monitor_hdr->temp_num;
	aw_monitor_write_data_to_table(dev, temp_info,
		&data[monitor_hdr->temp_offset]);
	AW_DEV_LOGD(dev, "===parse temp end ===");
	return 0;
}

static void aw_monitor_parse_hdr_v_0_1_2(struct device *dev, const uint8_t *data)
{
	struct aw_monitor_hdr *monitor_hdr =
			(struct aw_monitor_hdr *)data;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor_cfg *monitor_cfg = &aw87xxx->monitor.monitor_cfg;

	monitor_cfg->monitor_switch =
		(monitor_hdr->enable_flag >> MONITOR_EN_BIT) & MONITOR_EN_MASK;
	monitor_cfg->monitor_time = monitor_hdr->monitor_time;
	monitor_cfg->monitor_count = monitor_hdr->monitor_count;
	monitor_cfg->ipeak_switch =
		(monitor_hdr->enable_flag >> MONITOR_IPEAK_EN_BIT) & MONITOR_EN_MASK;
	monitor_cfg->logic_switch =
		(monitor_hdr->enable_flag >> MONITOR_LOGIC_BIT) & MONITOR_EN_MASK;
	monitor_cfg->gain_switch =
		(monitor_hdr->enable_flag >> MONITOR_GAIN_EN_BIT) & MONITOR_EN_MASK;
	monitor_cfg->vmax_switch =
		(monitor_hdr->enable_flag >> MONITOR_VMAX_EN_BIT) & MONITOR_EN_MASK;
	monitor_cfg->temp_switch =
		(monitor_hdr->enable_flag >> MONITOR_TEMP_EN_BIT) & MONITOR_EN_MASK;
	monitor_cfg->temp_aplha = monitor_hdr->temp_aplha;
	monitor_cfg->vol_switch =
		(monitor_hdr->enable_flag >> MONITOR_VOL_EN_BIT) & MONITOR_EN_MASK;
	monitor_cfg->vol_aplha = monitor_hdr->vol_aplha;
	monitor_cfg->temp_source =
		(monitor_hdr->enable_flag >> MONITOR_TEMPERATURE_SOURCE_BIT) & MONITOR_EN_MASK;
	monitor_cfg->vol_source =
		(monitor_hdr->enable_flag >> MONITOR_VOLTAGE_SOURCE_BIT) & MONITOR_EN_MASK;
	monitor_cfg->vol_mode =
		(monitor_hdr->enable_flag >> MONITOR_VOLTAGE_MODE_BIT) & MONITOR_EN_MASK;

	AW_DEV_LOGD(dev, "chip name:%s", monitor_hdr->chip_type);
	AW_DEV_LOGD(dev, "ui ver:0x%x", monitor_hdr->ui_ver);

	AW_DEV_LOGD(dev,
		"voltage mode:%d, voltage source:%d , temperature source:%d",
		monitor_cfg->vol_mode, monitor_cfg->vol_source,
		monitor_cfg->temp_source);

	AW_DEV_LOGD(dev,
		"monitor_switch:%d, monitor_time:%d (ms), monitor_count:%d",
		monitor_cfg->monitor_switch, monitor_cfg->monitor_time,
		monitor_cfg->monitor_count);

	AW_DEV_LOGD(dev,
		"logic_switch:%d, ipeak_switch:%d, gain_switch:%d, vmax_switch:%d",
		monitor_cfg->logic_switch, monitor_cfg->ipeak_switch,
		monitor_cfg->gain_switch, monitor_cfg->vmax_switch);

	AW_DEV_LOGD(dev,
		"temp_switch:%d, temp_aplha:%d, vol_switch:%d, vol_aplha:%d",
		monitor_cfg->temp_switch, monitor_cfg->temp_aplha,
		monitor_cfg->vol_switch, monitor_cfg->vol_aplha);
}

static int aw_monitor_check_fw_v_0_1_2(struct device *dev,
					const uint8_t *data, uint32_t data_len)
{
	struct aw_monitor_hdr *monitor_hdr =
				(struct aw_monitor_hdr *)data;
	uint32_t temp_size = 0;
	uint32_t vol_size = 0;

	if (data_len < sizeof(struct aw_monitor_hdr)) {
		AW_DEV_LOGE(dev,
			"params size[%d] < struct aw_monitor_hdr size[%d]!",
			data_len, (int)sizeof(struct aw_monitor_hdr));
		return -ENOMEM;
	}

	if (monitor_hdr->temp_offset > data_len) {
		AW_DEV_LOGE(dev, "temp_offset[%d] overflow file size[%d]!",
			monitor_hdr->temp_offset, data_len);
		return -ENOMEM;
	}

	if (monitor_hdr->vol_offset > data_len) {
		AW_DEV_LOGE(dev, "vol_offset[%d] overflow file size[%d]!",
			monitor_hdr->vol_offset, data_len);
		return -ENOMEM;
	}

	temp_size = (uint32_t)(monitor_hdr->temp_num * monitor_hdr->single_temp_size);
	if (temp_size > data_len) {
		AW_DEV_LOGE(dev, "temp_size:[%d] overflow file size[%d]!",
			temp_size, data_len);
		return -ENOMEM;
	}

	vol_size = (uint32_t)(monitor_hdr->vol_num * monitor_hdr->single_vol_size);
	if (vol_size > data_len) {
		AW_DEV_LOGE(dev, "vol_size:[%d] overflow file size[%d]!",
			vol_size, data_len);
		return -ENOMEM;
	}

	return 0;
}

static int aw_monitor_parse_data_v_0_1_2(struct device *dev,
				const uint8_t *data, uint32_t data_len)
{
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	ret = aw_monitor_check_fw_v_0_1_2(dev, data, data_len);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "check monitor failed");
		return ret;
	}

	aw_monitor_parse_hdr_v_0_1_2(dev, data);

	ret = aw_monitor_parse_temp_data_v_0_1_2(dev, data);
	if (ret < 0)
		return ret;

	ret = aw_monitor_parse_vol_data_v_0_1_2(dev, data);
	if (ret < 0)
		return ret;

	monitor->bin_status = AW_MONITOR_CFG_OK;

	return 0;
}

static int aw_monitor_param_check_sum(struct device *dev,
					const uint8_t *data, uint32_t data_len)
{
	int i = 0, check_sum = 0;
	struct aw_monitor_hdr *monitor_hdr =
		(struct aw_monitor_hdr *)data;

	if (data_len < sizeof(struct aw_monitor_hdr)) {
		AW_DEV_LOGE(dev,
			"data size smaller than hdr , please check monitor bin");
		return -ENOMEM;
	}

	for (i = 4 ; i < data_len; i++)
		check_sum += (uint8_t)data[i];

	if (monitor_hdr->check_sum != check_sum) {
		AW_DEV_LOGE(dev,
			"check_sum[%d] is not equal to actual check_sum[%d]",
				monitor_hdr->check_sum, check_sum);
		return -ENOMEM;
	}

	return 0;
}

static int aw87xxx_monitor_bin_version_parse(struct device *dev, const char *monitor_data)
{
	struct aw_bin_header *bin_header = NULL;
	struct aw_monitor_hdr *monitor_hdr = NULL;

	bin_header = (struct aw_bin_header *)monitor_data;
	switch (bin_header->bin_data_ver) {
	case DATA_VERSION_V1:
		return DATA_VERSION_V1;
	default:
		AW_DEV_LOGE(dev, "DATA_VERSION_V1 version Mismatched");
	}

	monitor_hdr = (struct aw_monitor_hdr *)monitor_data;
	switch (monitor_hdr->monitor_ver) {
	case AW_MONITOR_HDR_VER_0_1_2:
		return AW_MONITOR_HDR_VER_0_1_2;
	default:
		AW_DEV_LOGE(dev, "HDR_VER_0_1_2 version Mismatched");
	}

	AW_DEV_LOGE(dev, "unsupported monitor_hdr");

	return -EINVAL;
}

int aw87xxx_monitor_bin_parse(struct device *dev,
				const char *monitor_data, uint32_t data_len)
{
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = NULL;
	struct aw_container *monitor_container = NULL;

	if (aw87xxx == NULL) {
		AW_DEV_LOGE(dev, "get struct aw87xxx failed");
		return -EINVAL;
	}

	monitor = &aw87xxx->monitor;
	monitor->bin_status = AW_MONITOR_CFG_WAIT;

	ret = aw87xxx_monitor_bin_version_parse(dev, monitor_data);
	switch (ret) {
	case DATA_VERSION_V1:
		AW_DEV_LOGD(dev, "DATA_VERSION_V1 enter");
	ret = aw_monitor_bin_check(dev, monitor_data, data_len);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "monitor bin check failed");
		return ret;
	}

		ret = aw_monitor_parse_v_0_0_1(dev, monitor,
				monitor_data);
		if (ret < 0) {
			aw_monitor_cfg_v_0_0_1_free(monitor);
			return ret;
		}
		monitor->version = DATA_VERSION_V1;
		break;
	case AW_MONITOR_HDR_VER_0_1_2:
		AW_DEV_LOGD(dev, "AW_MONITOR_HDR_VER_0_1_2 enter");

		monitor_container = devm_kzalloc(dev,
				data_len + sizeof(uint32_t), GFP_KERNEL);
		if (monitor_container == NULL)
			return -ENOMEM;

		if (monitor->monitor_container) {
			devm_kfree(dev, monitor->monitor_container);
			monitor->monitor_container = NULL;
		}

		monitor->monitor_container = monitor_container;
		monitor_container->len = data_len;
		memcpy(monitor_container->data, monitor_data, data_len);

		if (monitor_container == NULL)
			AW_DEV_LOGE(dev, "monitor_container not null");
		else
			AW_DEV_LOGE(dev, "len %d ", monitor_container->len);

		ret = aw_monitor_param_check_sum(dev, monitor_data, data_len);
		if (ret < 0) {
			AW_DEV_LOGE(dev, "monitor bin check failed");
			return ret;
		}

		ret = aw_monitor_parse_data_v_0_1_2(dev, monitor_data, data_len);
		if (ret < 0)
			return ret;
		monitor->version = AW_MONITOR_HDR_VER_0_1_2;
		break;
	default:
		AW_DEV_LOGE(dev, "Unrecognized this bin data version[0x%x]",
			ret);
	}

	return 0;
}

/***************************************************************************
 *
 * aw87xxx monitor get adjustment vmax of power
 *
 ***************************************************************************/
static int aw_monitor_get_battery_status(struct device *dev,
				int type, int *value)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	int ret = -1;
	union power_supply_propval prop = { 0 };

	if (!aw87xxx->psy || IS_ERR(aw87xxx->psy)) {
		AW_DEV_LOGE(dev, "get power supply failed");
		return -EINVAL;
	}

	ret = power_supply_get_property(aw87xxx->psy, type, &prop);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "get type:%d failed", type);
		return -EINVAL;
	}

	*value = prop.intval;
	AW_DEV_LOGD(dev, "get type:%d value %d",
		type, *value);

	return 0;
}

static int aw_monitor_get_chip_temperature(struct device *dev, int *value)
{
	return 0;
}

static int aw_monitor_get_chip_voltage(struct device *dev, int *value)
{
	return 0;
}

static int aw_monitor_get_sys_temperature(struct device *dev, int *value)
{
	int ret = -1;
	int temperature = 0;

	ret = aw_monitor_get_battery_status(dev, POWER_SUPPLY_PROP_TEMP, &temperature);
	if (ret < 0)
		return ret;

	*value = temperature / 10;

	AW_DEV_LOGD(dev, "sys temperature: %d", *value);

	return ret;
}

static int aw_monitor_get_sys_voltage(struct device *dev, int *value)
{
	int ret = 0;
	int voltage = 0;

	ret = aw_monitor_get_battery_status(dev, POWER_SUPPLY_PROP_VOLTAGE_NOW, &voltage);
	if (ret < 0)
		return ret;

	*value = voltage / 1000;

	AW_DEV_LOGD(dev, "sys voltage: %d", *value);

	return ret;
}

static int aw_monitor_get_sys_capacity(struct device *dev, int *value)
{
	int ret = 0;

	ret = aw_monitor_get_battery_status(dev, POWER_SUPPLY_PROP_CAPACITY, value);
	if (ret < 0)
		return ret;

	AW_DEV_LOGD(dev, "sys capacity: %d", *value);

	return ret;
}

static int aw_monitor_get_chip_runtime_info(struct device *dev, uint8_t info_type, int *value)
{
	switch (info_type) {
	case AW_VOLTAGE_INFO:
		aw_monitor_get_chip_voltage(dev, value);
		break;
	case AW_TEMPERATURE_INFO:
		aw_monitor_get_chip_temperature(dev, value);
		break;
	default:
		AW_DEV_LOGE(dev, "unsupported type: %d", info_type);
		return -EINVAL;
	}

	return 0;
}

static int aw_monitor_get_sys_runtime_info(struct device *dev, uint8_t info_type, int *value)
{
	AW_DEV_LOGD(dev, "info_type: %s", (info_type == AW_TEMPERATURE_INFO) ? "temperature info" :
				((info_type == AW_VOLTAGE_INFO) ? "voltage info" : "capacity info"));

	switch (info_type) {
	case AW_VOLTAGE_INFO:
		aw_monitor_get_sys_voltage(dev, value);
		break;
	case AW_TEMPERATURE_INFO:
		aw_monitor_get_sys_temperature(dev, value);
		break;
	case AW_CAPACITY_INFO:
		aw_monitor_get_sys_capacity(dev, value);
		break;
	default:
		AW_DEV_LOGE(dev, "unsupported type: %d", info_type);
		return -EINVAL;
	}

	return 0;
}

static int aw_monitor_get_runtime_info(struct device *dev, uint8_t src_type, uint8_t info_type, int *value)
{
	AW_DEV_LOGD(dev, "source type: %s", src_type ? "platform info" : "chip info");

	switch (src_type) {
	case AW_CHIP_INFO:
		aw_monitor_get_chip_runtime_info(dev, info_type, value);
		break;
	case AW_PLATFORM_INFO:
		aw_monitor_get_sys_runtime_info(dev, info_type, value);
		break;
	default:
		AW_DEV_LOGE(dev, "unsupported type: %d", info_type);
		return -EINVAL;
	}

	return 0;
}

static int aw_search_vmax_from_table(struct device *dev,
				struct aw_monitor *monitor,
				const int vbat_vol, int *vmax_vol)
{
	int i = 0;
	int vmax_set = 0;
	uint32_t vmax_flag = 0;
	struct aw_monitor_header *monitor_hdr = &monitor->monitor_hdr;
	struct vmax_step_config *vmax_cfg = monitor->vmax_cfg;

	if (monitor->bin_status == AW_MONITOR_CFG_WAIT) {
		AW_DEV_LOGE(dev, "vmax_cfg not loaded or parse failed");
		return -ENODATA;
	}

	for (i = 0; i < monitor_hdr->step_count; i++) {
		if (vbat_vol == AW_VBAT_MAX) {
			vmax_set = AW_VMAX_MAX;
			vmax_flag = 1;
			AW_DEV_LOGD(dev, "vbat=%d, setting vmax=0x%x",
				vbat_vol, vmax_set);
			break;
		}

		if (vbat_vol >= vmax_cfg[i].vbat_min &&
			vbat_vol < vmax_cfg[i].vbat_max) {
			vmax_set = vmax_cfg[i].vmax_vol;
			vmax_flag = 1;
			AW_DEV_LOGD(dev, "read setting vmax=0x%x, step[%d]: vbat_min=%d,vbat_max=%d",
				vmax_set, i,
				vmax_cfg[i].vbat_min,
				vmax_cfg[i].vbat_max);
			break;
		}
	}

	if (!vmax_flag) {
		AW_DEV_LOGE(dev, "vmax_cfg not found");
		return -ENODATA;
	}

	*vmax_vol = vmax_set;
	return 0;
}


/***************************************************************************
 *
 *monitor_esd_func
 *
 ***************************************************************************/
static int aw_chip_status_recover(struct aw87xxx *aw87xxx)
{
	int ret = -1;
	struct aw_monitor *monitor = &aw87xxx->monitor;
	char *profile = aw87xxx->current_profile;

	AW_DEV_LOGD(aw87xxx->dev, "enter");

	ret = aw87xxx_update_profile_esd(aw87xxx, profile);
	if (ret < 0) {
		AW_DEV_LOGE(aw87xxx->dev, "load profile[%s] failed ",
			profile);
		return ret;
	}

	AW_DEV_LOGD(aw87xxx->dev, "current prof[%s], dev_index[%d] ",
			profile, aw87xxx->dev_index);

	monitor->pre_vmax = AW_VMAX_INIT_VAL;
	monitor->first_entry = AW_FIRST_ENTRY;
	monitor->timer_cnt = 0;
	monitor->vbat_sum = 0;

	return 0;
}

static int aw_monitor_chip_esd_check_work(struct aw87xxx *aw87xxx)
{
	int ret = 0;
	int i = 0;

	for (i = 0; i < REG_STATUS_CHECK_MAX; i++) {
		AW_DEV_LOGD(aw87xxx->dev, "reg_status_check[%d]", i);

		ret = aw87xxx_dev_esd_reg_status_check(&aw87xxx->aw_dev);
		if (ret < 0) {
			if (ret == -EINVAL)
				aw_chip_status_recover(aw87xxx);
		} else {
			AW_DEV_LOGD(aw87xxx->dev, "chip status check succeed");
			break;
		}
		msleep(AW_ESD_CHECK_DELAY);
	}

	return ret;
}


/***************************************************************************
 *
 * aw87xxx monitor work with dsp
 *
 ***************************************************************************/
static int aw_monitor_update_vmax_to_dsp(struct device *dev,
				struct aw_monitor *monitor, int vmax_set)
{
	int ret = -1;
	uint32_t enable = 0;

	if (monitor->pre_vmax != vmax_set) {
		ret = aw87xxx_dsp_get_rx_module_enable(&enable);
		if (!enable || ret < 0) {
			AW_DEV_LOGE(dev, "get rx failed or rx disable, ret=%d, enable=%d",
				ret, enable);
			return -EPERM;
		}

		ret = aw87xxx_dsp_set_vmax(vmax_set, monitor->dev_index);
		if (ret) {
			AW_DEV_LOGE(dev, "set dsp msg fail, ret=%d", ret);
			return ret;
		}

		AW_DEV_LOGD(dev, "set dsp vmax=0x%x sucess", vmax_set);
		monitor->pre_vmax = vmax_set;
	} else {
		AW_DEV_LOGD(dev, "vmax=0x%x no change", vmax_set);
	}

	return 0;
}

static void aw_monitor_with_dsp_vmax_work(struct device *dev,
					struct aw_monitor *monitor)
{
	int ret = -1;
	int vmax_set = 0;
	int vbat_capacity = 0;
	int ave_capacity = 0;
	struct aw_monitor_header *monitor_hdr = &monitor->monitor_hdr;

	AW_DEV_LOGD(dev, "enter with dsp monitor");

	ret = aw_monitor_get_sys_capacity(dev, &vbat_capacity);
	if (ret < 0)
		return;

	if (monitor->timer_cnt < monitor_hdr->monitor_count) {
		monitor->timer_cnt++;
		monitor->vbat_sum += vbat_capacity;
			AW_DEV_LOGD(dev, "timer_cnt = %d",
			monitor->timer_cnt);
	}
	if ((monitor->timer_cnt >= monitor_hdr->monitor_count) ||
	    (monitor->first_entry == AW_FIRST_ENTRY)) {
		if (monitor->first_entry == AW_FIRST_ENTRY)
			monitor->first_entry = AW_NOT_FIRST_ENTRY;
		ave_capacity = monitor->vbat_sum / monitor->timer_cnt;

		if (monitor->custom_capacity)
			ave_capacity = monitor->custom_capacity;

		AW_DEV_LOGD(dev, "get average capacity = %d", ave_capacity);

		ret = aw_search_vmax_from_table(dev, monitor,
				ave_capacity, &vmax_set);
		if (ret < 0)
			AW_DEV_LOGE(dev, "not find vmax_vol");
		else
			aw_monitor_update_vmax_to_dsp(dev, monitor, vmax_set);

		monitor->timer_cnt = 0;
		monitor->vbat_sum = 0;
	}
}

static int aw_monitor_get_temp_and_vol(struct device *dev)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	unsigned int voltage = 0;
	int current_temp = 0;
	int ret = -1;

#ifdef AW_DEBUG
	if (monitor->test_vol == 0) {
		ret = aw_monitor_get_runtime_info(dev, monitor->monitor_cfg.temp_source,
			AW_VOLTAGE_INFO, &voltage);
		if (ret < 0)
			return ret;
	} else {
		voltage = monitor->test_vol;
	}

	if (monitor->test_temp == 0) {
		ret = aw_monitor_get_runtime_info(dev, monitor->monitor_cfg.temp_source,
				AW_TEMPERATURE_INFO, &current_temp);
		if (ret < 0)
			return ret;
	} else {
		current_temp = monitor->test_temp;
	}
#else

	ret = aw_monitor_get_runtime_info(dev, monitor->monitor_cfg.vol_source,
			monitor->monitor_cfg.vol_mode, &voltage);
	if (ret < 0)
		return ret;

	ret = aw_monitor_get_runtime_info(dev, monitor->monitor_cfg.temp_source,
			AW_TEMPERATURE_INFO, &current_temp);
	if (ret < 0)
		return ret;

#endif

	monitor->vol_trace.sum_val += voltage;
	monitor->temp_trace.sum_val += current_temp;
	monitor->samp_count++;

	return 0;
}

static int aw_monitor_first_get_data_form_table(struct device *dev,
				struct aw_table_info table_info,
			struct aw_monitor_trace *data_trace)
{
	int i = 0;

	if (table_info.aw_table == NULL) {
		AW_DEV_LOGE(dev, "table_info.aw_table is null");
		return -EINVAL;
	}

	for (i = 0; i < table_info.table_num; i++) {
		if (data_trace->sum_val >= table_info.aw_table[i].min_val) {
			memcpy(&data_trace->aw_table, &table_info.aw_table[i],
				sizeof(struct aw_table));
			break;
		}
	}
	return 0;
}

static int aw_monitor_trace_data_from_table(struct device *dev,
			struct aw_table_info table_info,
			struct aw_monitor_trace *data_trace)
{
	int i = 0;

	if (table_info.aw_table == NULL) {
		AW_DEV_LOGE(dev, "table_info.aw_table is null");
		return -EINVAL;
	}

	for (i = 0; i < table_info.table_num; i++) {
		if (data_trace->sum_val >= table_info.aw_table[i].min_val &&
			data_trace->sum_val <= table_info.aw_table[i].max_val) {
			memcpy(&data_trace->aw_table, &table_info.aw_table[i],
				sizeof(struct aw_table));
			break;
		}
	}

	return 0;
}

static int aw_monitor_get_data_from_table(struct device *dev,
					struct aw_table_info table_info,
					struct aw_monitor_trace *data_trace,
					uint32_t aplha)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	if (monitor->first_entry == AW_FIRST_ENTRY) {
		return aw_monitor_first_get_data_form_table(dev,
						table_info, data_trace);
	} else {
		if (monitor->samp_count == 0) {
			AW_DEV_LOGE(dev, "monitor->samp_count:%d unsupported", monitor->samp_count);
			return -EINVAL;
		}

		data_trace->sum_val = data_trace->sum_val / monitor->samp_count;
		data_trace->sum_val = ((int32_t)aplha * data_trace->sum_val +
			(1000 - (int32_t)aplha) * data_trace->pre_val) / 1000;
		return aw_monitor_trace_data_from_table(dev,
						table_info, data_trace);
	}

	return 0;
}

static int aw_monitor_get_data(struct device *dev)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	struct aw_monitor_cfg *monitor_cfg = &monitor->monitor_cfg;
	struct aw_monitor_trace *vol_trace = &monitor->vol_trace;
	struct aw_monitor_trace *temp_trace = &monitor->temp_trace;
	int ret = -1;

	if (monitor_cfg->vol_switch) {
		ret = aw_monitor_get_data_from_table(dev,
			monitor_cfg->vol_info, vol_trace,
			monitor_cfg->vol_aplha);
		if (ret < 0)
			return ret;
	} else {
		vol_trace->aw_table.ipeak = IPEAK_NONE;
		vol_trace->aw_table.gain = GAIN_NONE;
		vol_trace->aw_table.vmax = VMAX_NONE;
	}

	if (monitor_cfg->temp_switch) {
		ret = aw_monitor_get_data_from_table(dev,
			monitor_cfg->temp_info, temp_trace,
			monitor_cfg->temp_aplha);
		if (ret < 0)
			return ret;
	} else {
		temp_trace->aw_table.ipeak = IPEAK_NONE;
		temp_trace->aw_table.gain = GAIN_NONE;
		temp_trace->aw_table.vmax = VMAX_NONE;
	}

	AW_DEV_LOGD(dev,
			"filter_vol:%d, vol: ipeak = 0x%x, gain = 0x%x, vmax = 0x%x",
			monitor->vol_trace.sum_val, vol_trace->aw_table.ipeak,
			vol_trace->aw_table.gain, vol_trace->aw_table.vmax);

	AW_DEV_LOGD(dev,
			"filter_temp:%d, temp: ipeak = 0x%x, gain = 0x%x, vmax = 0x%x",
			monitor->temp_trace.sum_val, temp_trace->aw_table.ipeak,
			temp_trace->aw_table.gain, temp_trace->aw_table.vmax);
	return 0;
}

static void aw_monitor_get_cfg(struct device *dev,
					struct aw_table *set_table)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	struct aw_table *temp_data = &monitor->temp_trace.aw_table;
	struct aw_table *vol_data = &monitor->vol_trace.aw_table;

	if (temp_data->ipeak == IPEAK_NONE && vol_data->ipeak == IPEAK_NONE) {
		memcpy(set_table, temp_data, sizeof(struct aw_table));
	} else if (temp_data->ipeak == IPEAK_NONE) {
		memcpy(set_table, vol_data, sizeof(struct aw_table));
	} else if (vol_data->ipeak == IPEAK_NONE) {
		memcpy(set_table, temp_data, sizeof(struct aw_table));
	} else {
		if (monitor->monitor_cfg.logic_switch == AW_MON_LOGIC_OR) {
			set_table->ipeak = (temp_data->ipeak < vol_data->ipeak ?
					temp_data->ipeak : vol_data->ipeak);
			set_table->gain = (temp_data->gain < vol_data->gain ?
					vol_data->gain : temp_data->gain);
			set_table->vmax = (temp_data->vmax < vol_data->vmax ?
					vol_data->vmax : temp_data->vmax);
		} else {
			set_table->ipeak = (temp_data->ipeak < vol_data->ipeak ?
				vol_data->ipeak : temp_data->ipeak);
			set_table->gain = (temp_data->gain < vol_data->gain ?
					temp_data->gain : vol_data->gain);
			set_table->vmax = (temp_data->vmax < vol_data->vmax ?
					temp_data->vmax : vol_data->vmax);
		}
	}
}

static void aw_monitor_set_ipeak(struct device *dev,
				uint16_t ipeak)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	struct aw_monitor_cfg *monitor_cfg = &monitor->monitor_cfg;
	uint8_t reg_val = 0;
	uint8_t read_reg_val = 0;
	int ret = -1;
	struct aw_ipeak_desc *desc = &aw87xxx->aw_dev.ipeak_desc;

	if (desc->reg == AW_REG_NONE)
		return;

	if (ipeak == IPEAK_NONE || (!monitor_cfg->ipeak_switch))
		return;

	ret = aw87xxx_dev_i2c_read_byte(&aw87xxx->aw_dev, desc->reg, &reg_val);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "read ipeak failed");
		return;
	}

	read_reg_val = reg_val;
	read_reg_val &= (~desc->mask);

	if (read_reg_val == ipeak) {
		AW_DEV_LOGD(dev, "ipeak = 0x%x, no change",
					read_reg_val);
		return;
	}
	reg_val &= desc->mask;
	read_reg_val = ipeak;
	reg_val |= read_reg_val;

	ret = aw87xxx_dev_i2c_write_byte(&aw87xxx->aw_dev, desc->reg, reg_val);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "write ipeak failed");
		return;
	}
	AW_DEV_LOGD(dev, "set reg val = 0x%x, ipeak = 0x%x",
				reg_val, ipeak);

}

static void aw_monitor_set_gain(struct device *dev, uint16_t gain)
{
}

static void aw_monitor_set_vmax(struct device *dev,
						uint32_t vmax)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	struct aw_monitor_cfg *monitor_cfg = &monitor->monitor_cfg;

	if (vmax == VMAX_NONE || (!monitor_cfg->vmax_switch))
		return;

	aw_monitor_update_vmax_to_dsp(dev, monitor, vmax);
}

static void aw_monitor_with_dsp_work(struct device *dev)
{
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	struct aw_table set_table;

	ret = aw_monitor_get_temp_and_vol(dev);
	if (ret < 0)
		return;

	if (monitor->samp_count < monitor->monitor_cfg.monitor_count &&
		(monitor->first_entry == AW_NOT_FIRST_ENTRY))
		return;

	ret = aw_monitor_get_data(dev);
	if (ret < 0)
		return;

	aw_monitor_get_cfg(dev, &set_table);

	AW_DEV_LOGD(dev,
		"set_ipeak = 0x%x, set_gain = 0x%x, set_vmax = 0x%x",
		set_table.ipeak, set_table.gain, set_table.vmax);

	aw_monitor_set_ipeak(dev, set_table.ipeak);

	aw_monitor_set_gain(dev, set_table.gain);

	aw_monitor_set_vmax(dev, set_table.vmax);

	monitor->samp_count = 0;
	monitor->temp_trace.pre_val = monitor->temp_trace.sum_val;
	monitor->temp_trace.sum_val = 0;

	monitor->vol_trace.pre_val = monitor->vol_trace.sum_val;
	monitor->vol_trace.sum_val = 0;

	if (monitor->first_entry == AW_FIRST_ENTRY)
		monitor->first_entry = AW_NOT_FIRST_ENTRY;

}

static void aw_monitor_work_func(struct work_struct *work)
{
	int ret = 0;
	struct aw87xxx *aw87xxx = container_of(work,
				struct aw87xxx, monitor.with_dsp_work.work);
	struct device *dev = aw87xxx->dev;
	struct aw_monitor *monitor = &aw87xxx->monitor;
	uint32_t monitor_switch = 0;
	uint32_t monitor_time = 0;
	uint32_t monitor_count = 0;

	AW_DEV_LOGD(dev, "enter");

	pm_stay_awake(dev);

	if (!aw87xxx->is_suspend) {
		if (monitor->esd_enable &&
				monitor->esd_err_cnt < AW_MONOTOR_ESD_ERR_CNT_MAX) {
			ret = aw_monitor_chip_esd_check_work(aw87xxx);
			if (ret < 0) {
				aw87xxx->monitor.esd_err_cnt++;
				AW_DEV_LOGD(dev, "esd check failed");
			}
		}

		aw_monitor_get_ctrl_info(dev, &monitor_switch,
			&monitor_count, &monitor_time);

		if (monitor_switch && !(aw87xxx->aw_dev.is_rec_mode) &&
			monitor->open_dsp_en && monitor->bin_status == AW_MONITOR_CFG_OK) {
			AW_DEV_LOGD(dev, "monitor version 0x%x", monitor->version);
			switch (monitor->version) {
			case DATA_VERSION_V1:
				aw_monitor_with_dsp_vmax_work(dev, monitor);
				break;
			case AW_MONITOR_HDR_VER_0_1_2:
				aw_monitor_with_dsp_work(dev);
				break;
			default:
				AW_DEV_LOGE(dev, "INVALID version:%d", monitor->version);
				return;
			}
		}
	}

	pm_relax(dev);

	if (monitor->esd_enable || (monitor_switch &&
		!(aw87xxx->aw_dev.is_rec_mode) && monitor->open_dsp_en &&
		monitor->bin_status == AW_MONITOR_CFG_OK)) {
		schedule_delayed_work(&monitor->with_dsp_work,
			msecs_to_jiffies(monitor_time));
	}
}

void aw87xxx_monitor_stop(struct aw_monitor *monitor)
{
	struct aw87xxx *aw87xxx =
		container_of(monitor, struct aw87xxx, monitor);

	AW_DEV_LOGD(aw87xxx->dev, "enter");
	cancel_delayed_work_sync(&monitor->with_dsp_work);
}

void aw87xxx_monitor_start(struct aw_monitor *monitor)
{
	struct aw87xxx *aw87xxx =
		container_of(monitor, struct aw87xxx, monitor);
	uint32_t monitor_switch = 0;
	uint32_t monitor_time = 0;
	uint32_t monitor_count = 0;
	int ret = 0;

	ret = aw87xxx_dev_check_reg_is_rec_mode(&aw87xxx->aw_dev);
	if (ret < 0) {
		AW_DEV_LOGE(aw87xxx->dev, "get reg current mode failed");
		return;
	}

	aw_monitor_get_ctrl_info(aw87xxx->dev, &monitor_switch,
		&monitor_count, &monitor_time);

	AW_DEV_LOGD(aw87xxx->dev,
		"esd_enable:%d, monitor_open_dsp_en:%d, is_rec_mode:%d",
		monitor->esd_enable, monitor->open_dsp_en, aw87xxx->aw_dev.is_rec_mode);
	AW_DEV_LOGD(aw87xxx->dev,
		"monitor_switch:%d, monitor_count:%d, monitor_time:%d",
		monitor_switch, monitor_count, monitor_time);

	if (monitor->esd_enable || (monitor_switch &&
			!(aw87xxx->aw_dev.is_rec_mode) && monitor->open_dsp_en
			&& monitor->bin_status == AW_MONITOR_CFG_OK)) {

		AW_DEV_LOGD(aw87xxx->dev, "enter");
		monitor->pre_vmax = AW_VMAX_INIT_VAL;
		monitor->first_entry = AW_FIRST_ENTRY;
		monitor->timer_cnt = 0;
		monitor->vbat_sum = 0;
		monitor->esd_err_cnt = 0;

		schedule_delayed_work(&monitor->with_dsp_work,
				msecs_to_jiffies(monitor_time));
	}
}
/***************************************************************************
 *
 * aw87xxx no dsp monitor func
 *
 ***************************************************************************/
static int aw87xxx_hal_get_vmax(struct device *dev, int32_t *vmax)
{
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	struct aw_table set_table;

	monitor->first_entry = AW_FIRST_ENTRY;
	monitor->samp_count = 0;
	monitor->temp_trace.sum_val = 0;
	monitor->temp_trace.pre_val = 0;
	monitor->vol_trace.sum_val = 0;
	monitor->vol_trace.pre_val = 0;

	ret = aw_monitor_get_temp_and_vol(dev);
	if (ret < 0)
		return ret;

	ret = aw_monitor_get_data(dev);
	if (ret < 0)
		return ret;

	aw_monitor_get_cfg(dev, &set_table);

	*vmax = set_table.vmax;

	return 0;
}

int aw87xxx_monitor_no_dsp_get_vmax(struct aw_monitor *monitor, int32_t *vmax)
{
	int vbat_capacity = 0;
	int ret = -1;
	int vmax_vol = 0;
	struct aw87xxx *aw87xxx =
		container_of(monitor, struct aw87xxx, monitor);
	struct device *dev = aw87xxx->dev;

	if (monitor->version == AW_MONITOR_HDR_VER_0_1_2)
		return aw87xxx_hal_get_vmax(dev, vmax);

	ret = aw_monitor_get_sys_capacity(dev, &vbat_capacity);
	if (ret < 0)
		return ret;

	if (monitor->custom_capacity)
		vbat_capacity = monitor->custom_capacity;
	AW_DEV_LOGD(dev, "get_battery_capacity is[%d]", vbat_capacity);

	ret = aw_search_vmax_from_table(dev, monitor,
				vbat_capacity, &vmax_vol);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "not find vmax_vol");
		return ret;
	}

	*vmax = vmax_vol;
	return 0;
}


/***************************************************************************
 *
 * aw87xxx monitor sysfs nodes
 *
 ***************************************************************************/
#ifdef AW_DEBUG
static ssize_t vol_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	uint32_t vol = 0;

	if (count == 0)
		return 0;

	ret = kstrtouint(buf, 0, &vol);
	if (ret < 0)
		return ret;

	AW_DEV_LOGD(aw87xxx->dev, "vol set =%d", vol);
	monitor->test_vol = vol;

	return count;
}

static ssize_t vol_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len,
		"vol: %d\n",
		monitor->test_vol);
	return len;
}

static ssize_t temp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	int32_t temp = 0;

	if (count == 0)
		return 0;

	ret = kstrtoint(buf, 0, &temp);
	if (ret < 0)
		return ret;

	AW_DEV_LOGD(aw87xxx->dev, "temp set =%d", temp);

	monitor->test_temp = temp;

	return count;
}

static ssize_t temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	ssize_t len = 0;

	len += snprintf(buf+len, PAGE_SIZE-len,
		"temp: %d\n",
		monitor->test_temp);

	return len;
}

static DEVICE_ATTR_RW(vol);
static DEVICE_ATTR_RW(temp);
#endif


static ssize_t esd_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	if (monitor->esd_enable) {
		AW_DEV_LOGD(aw87xxx->dev, "esd-enable=true");
		len += snprintf(buf + len, PAGE_SIZE - len,
			"esd-enable=true\n");
	} else {
		AW_DEV_LOGD(aw87xxx->dev, "esd-enable=false");
		len += snprintf(buf + len, PAGE_SIZE - len,
			"esd-enable=false\n");
	}

	return len;
}

static ssize_t esd_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	char esd_enable[AW_ESD_ENABLE_STRLEN] = {0};
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	if (strlen(buf) > AW_ESD_ENABLE_STRLEN) {
		AW_DEV_LOGE(aw87xxx->dev, "input esd_enable_str_len is out of max[%d]",
				AW_ESD_ENABLE_STRLEN);
		return -EINVAL;
	}

	if (sscanf(buf, "%15s", esd_enable) == 1) {
		AW_DEV_LOGD(aw87xxx->dev, "input esd-enable=[%s]", esd_enable);
		if (!strcmp(esd_enable, "true"))
			monitor->esd_enable = AW_ESD_ENABLE;
		else
			monitor->esd_enable = AW_ESD_DISABLE;
		AW_DEV_LOGD(dev, "set esd-enable=[%s]",
				monitor->esd_enable ? "true" : "false");
	} else {
		AW_DEV_LOGE(aw87xxx->dev, "input esd-enable error");
		return -EINVAL;
	}

	return len;
}

static ssize_t vbat_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	int ret = -1;
	int vbat_capacity = 0;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	if (monitor->custom_capacity == 0) {
		ret = aw_monitor_get_sys_capacity(dev, &vbat_capacity);
		if (ret < 0) {
			AW_DEV_LOGE(aw87xxx->dev, "get battery_capacity failed");
			return ret;
		}
		len += snprintf(buf + len, PAGE_SIZE - len,
			"vbat capacity=%d\n", vbat_capacity);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len,
				"vbat capacity=%d\n",
				monitor->custom_capacity);
	}

	return len;
}

static ssize_t vbat_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	int ret = -1;
	int capacity = 0;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	ret = kstrtouint(buf, 0, &capacity);
	if (ret < 0)
		return ret;
	AW_DEV_LOGD(aw87xxx->dev, "set capacity = %d", capacity);
	if (capacity >= AW_VBAT_CAPACITY_MIN &&
			capacity <= AW_VBAT_CAPACITY_MAX){
		monitor->custom_capacity = capacity;
	} else {
		AW_DEV_LOGE(aw87xxx->dev, "vbat_set=invalid,please input value [%d-%d]",
			AW_VBAT_CAPACITY_MIN, AW_VBAT_CAPACITY_MAX);
		return -EINVAL;
	}

	return len;
}

static ssize_t vmax_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	int ret = -1;
	int vbat_capacity = 0;
	int vmax_get = 0;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	if (monitor->open_dsp_en) {
		ret = aw87xxx_dsp_get_vmax(&vmax_get, aw87xxx->dev_index);
		if (ret < 0) {
			AW_DEV_LOGE(aw87xxx->dev,
				"get dsp vmax fail, ret=%d", ret);
			return ret;
		}
		len += snprintf(buf + len, PAGE_SIZE - len,
				"get_vmax=%d\n", vmax_get);
	} else {
		ret = aw_monitor_get_sys_capacity(dev, &vbat_capacity);
		if (ret < 0)
			return ret;
		AW_DEV_LOGD(aw87xxx->dev, "get_battery_capacity is [%d]",
			vbat_capacity);

		if (monitor->custom_capacity) {
			vbat_capacity = monitor->custom_capacity;
			AW_DEV_LOGD(aw87xxx->dev, "get custom_capacity is [%d]",
				vbat_capacity);
		}

		ret = aw_search_vmax_from_table(aw87xxx->dev, monitor,
					vbat_capacity, &vmax_get);
		if (ret < 0) {
			AW_DEV_LOGE(aw87xxx->dev, "not find vmax_vol");
			len += snprintf(buf + len, PAGE_SIZE - len,
				"not_find_vmax_vol\n");
			return len;
		}
		len += snprintf(buf + len, PAGE_SIZE - len,
			"0x%x\n", vmax_get);
		AW_DEV_LOGD(aw87xxx->dev, "0x%x", vmax_get);
	}

	return len;
}

static ssize_t vmax_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t vmax_set = 0;
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	ret = kstrtouint(buf, 0, &vmax_set);
	if (ret < 0)
		return ret;

	AW_DEV_LOGD(aw87xxx->dev, "vmax_set=0x%x", vmax_set);

	if (monitor->open_dsp_en) {
		ret = aw87xxx_dsp_set_vmax(vmax_set, aw87xxx->dev_index);
		if (ret < 0) {
			AW_DEV_LOGE(aw87xxx->dev, "send dsp_msg error, ret = %d",
				ret);
			return ret;
		}
		usleep_range(2000, 2010);
	} else {
		AW_DEV_LOGE(aw87xxx->dev, "no_dsp system,vmax_set invalid");
		return -EINVAL;
	}

	return count;
}

static ssize_t monitor_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	uint32_t monitor_count = 0;
	uint32_t monitor_time = 0;
	uint32_t monitor_switch = 0;

	aw_monitor_get_ctrl_info(dev, &monitor_switch,
		&monitor_count, &monitor_time);

	len += snprintf(buf + len, PAGE_SIZE - len,
			"aw87xxx monitor switch: %u\n",
			monitor_switch);
	return len;
}


int aw87xxx_dev_monitor_switch_set(struct aw_monitor *monitor, uint32_t enable)
{
	struct aw87xxx *aw87xxx =
			container_of(monitor, struct aw87xxx, monitor);
	struct aw_monitor_header *monitor_hdr = &monitor->monitor_hdr;
	struct aw_monitor_cfg *monitor_cfg = &monitor->monitor_cfg;

	AW_DEV_LOGD(aw87xxx->dev, "monitor switch set =%d", enable);

	if (monitor->bin_status == AW_MONITOR_CFG_WAIT) {
		AW_DEV_LOGE(aw87xxx->dev, "bin parse faile or not loaded,set invalid");
		return -EINVAL;
	}

	if (enable) {
		monitor_hdr->monitor_switch = 1;
		monitor_cfg->monitor_switch = 1;
		if (monitor->open_dsp_en) {
			monitor->pre_vmax = AW_VMAX_INIT_VAL;
			monitor->first_entry = AW_FIRST_ENTRY;
			monitor->timer_cnt = 0;
			monitor->vbat_sum = 0;
		}
	} else {
		monitor_hdr->monitor_switch = 0;
		monitor_cfg->monitor_switch = 0;
	}

	return 0;
}

static ssize_t monitor_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	uint32_t enable = 0;
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;

	ret = kstrtouint(buf, 0, &enable);
	if (ret < 0)
		return ret;

	ret = aw87xxx_dev_monitor_switch_set(monitor, enable);
	if (ret)
		return ret;

	return count;
}

static ssize_t monitor_time_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	uint32_t monitor_count = 0;
	uint32_t monitor_time = 0;
	uint32_t monitor_switch = 0;

	aw_monitor_get_ctrl_info(dev, &monitor_switch,
		&monitor_count, &monitor_time);

	len += snprintf(buf + len, PAGE_SIZE - len,
			"aw_monitor_timer = %u(ms)\n",
			monitor_time);
	return len;
}

static ssize_t monitor_time_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int timer_val = 0;
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	struct aw_monitor_header *monitor_hdr = &monitor->monitor_hdr;
	struct aw_monitor_cfg *monitor_cfg = &monitor->monitor_cfg;

	ret = kstrtouint(buf, 0, &timer_val);
	if (ret < 0)
		return ret;

	AW_DEV_LOGD(aw87xxx->dev, "input monitor timer=%d(ms)", timer_val);

	if (monitor->bin_status == AW_MONITOR_CFG_WAIT) {
		AW_DEV_LOGE(aw87xxx->dev, "bin parse faile or not loaded,set invalid");
		return -EINVAL;
	}


	monitor_hdr->monitor_time = timer_val;
	monitor_cfg->monitor_time = timer_val;

	return count;
}

static ssize_t monitor_count_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	uint32_t monitor_count = 0;
	uint32_t monitor_time = 0;
	uint32_t monitor_switch = 0;

	aw_monitor_get_ctrl_info(dev, &monitor_switch,
		&monitor_count, &monitor_time);

	len += snprintf(buf + len, PAGE_SIZE - len,
			"aw_monitor_count = %u\n",
			monitor_count);
	return len;
}

static ssize_t monitor_count_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int monitor_count = 0;
	int ret = -1;
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	struct aw_monitor_header *monitor_hdr = &monitor->monitor_hdr;
	struct aw_monitor_cfg *monitor_cfg = &monitor->monitor_cfg;

	ret = kstrtouint(buf, 0, &monitor_count);
	if (ret < 0)
		return ret;
	AW_DEV_LOGD(aw87xxx->dev, "input monitor count=%d", monitor_count);

	if (monitor->bin_status == AW_MONITOR_CFG_WAIT) {
		AW_DEV_LOGE(aw87xxx->dev, "bin parse faile or not loaded,set invalid");
		return -EINVAL;
	}

	monitor_hdr->monitor_count = monitor_count;
	monitor_cfg->monitor_count = monitor_count;

	return count;
}


static ssize_t rx_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	ssize_t len = 0;
	int ret = -1;
	uint32_t enable = 0;

	if (monitor->open_dsp_en) {
		ret = aw87xxx_dsp_get_rx_module_enable(&enable);
		if (ret) {
			AW_DEV_LOGE(aw87xxx->dev, "dsp_msg error, ret=%d", ret);
			return ret;
		}
		len += snprintf(buf + len, PAGE_SIZE - len,
			"aw87xxx rx: %u\n", enable);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len,
				"command is invalid\n");
	}

	return len;
}

static ssize_t rx_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	int ret = -1;
	uint32_t enable;

	ret = kstrtouint(buf, 0, &enable);
	if (ret < 0)
		return ret;

	if (monitor->open_dsp_en) {
		AW_DEV_LOGD(aw87xxx->dev, "set rx enable=%d", enable);

		ret = aw87xxx_dsp_set_rx_module_enable(enable);
		if (ret < 0) {
			AW_DEV_LOGE(aw87xxx->dev, "dsp_msg error, ret=%d",
				ret);
			return ret;
		}
	} else {
		AW_DEV_LOGE(aw87xxx->dev, "command is invalid");
		return -EINVAL;
	}

	return count;
}

static ssize_t monitor_params_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *monitor = &aw87xxx->monitor;
	struct aw_container *monitor_container = monitor->monitor_container;
	ssize_t len = 0;
	int i = 0;

	if ((monitor_container == NULL) || (monitor->bin_status == AW_MONITOR_CFG_WAIT)) {
		len += snprintf((char *)(buf + len), PAGE_SIZE - len,
				"0\n");
		return len;
	}

	len += snprintf((char *)(buf + len), PAGE_SIZE - len, "1 ");
	for (i = 0; i < monitor_container->len; i++) {
		len += snprintf((char *)(buf + len), PAGE_SIZE - len,
		"%02x,", monitor_container->data[i]);
	}

	len += snprintf((char *)(buf + len), PAGE_SIZE - len, "\n");

	return len;
}

static int aw_monitor_real_time_update_monitor(struct device *dev)
{
	const struct firmware *cont = NULL;
	int ret = 0;

	ret = request_firmware(&cont, AW87XXX_MONITOR_NAME, dev);
	if (ret < 0) {
		AW_DEV_LOGE(dev, "failed to read %s", AW87XXX_MONITOR_NAME);
		release_firmware(cont);
		return ret;
	}

	ret = aw87xxx_monitor_bin_parse(dev, cont->data, (uint32_t)cont->size);
	if (ret < 0)
		AW_DEV_LOGE(dev, "parse monitor firmware failed!");

	release_firmware(cont);

	return ret;
}

void aw87xxx_monitor_cfg_free(struct aw_monitor *monitor)
{
	struct aw_monitor_cfg *monitor_cfg = &monitor->monitor_cfg;
	struct aw87xxx *aw87xxx =
		container_of(monitor, struct aw87xxx, monitor);

	aw_monitor_cfg_v_0_0_1_free(monitor);

	if (monitor_cfg->temp_info.aw_table != NULL) {
		devm_kfree(aw87xxx->dev, monitor_cfg->temp_info.aw_table);
		monitor_cfg->temp_info.aw_table = NULL;
	}

	if (monitor_cfg->vol_info.aw_table != NULL) {
		devm_kfree(aw87xxx->dev, monitor_cfg->vol_info.aw_table);
		monitor_cfg->vol_info.aw_table = NULL;
	}

	memset(monitor_cfg, 0, sizeof(struct aw_monitor_cfg));
}

static ssize_t monitor_update_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *aw_monitor = &aw87xxx->monitor;
	ssize_t len = 0;

	if (aw_monitor->version == AW_MONITOR_DATA_VER) {
		len += snprintf((char *)(buf + len), PAGE_SIZE - len,
				"0\n");
		AW_DEV_LOGE(dev, "unsupported monitor version");
		return len;
	}

	if (aw_monitor->bin_status == AW_MONITOR_CFG_WAIT) {
		len += snprintf((char *)(buf + len), PAGE_SIZE - len,
				"0\n");
	} else if (aw_monitor->bin_status == AW_MONITOR_CFG_OK) {
		len += snprintf((char *)(buf + len), PAGE_SIZE - len,
		"1\n");
	} else {
		AW_DEV_LOGE(dev, "unsupported bin_status");
	}

	return len;
}

static ssize_t monitor_update_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct aw87xxx *aw87xxx = dev_get_drvdata(dev);
	struct aw_monitor *aw_monitor = &aw87xxx->monitor;
	uint32_t update = 0;
	int ret = -1;

	ret = kstrtouint(buf, 0, &update);
	if (ret < 0)
		return ret;

	AW_DEV_LOGD(dev, "monitor update = %d", update);

	if (update) {
		aw87xxx_monitor_stop(aw_monitor);
		aw87xxx_monitor_cfg_free(aw_monitor);
		ret = aw_monitor_real_time_update_monitor(dev);
		if (ret < 0)
			return ret;
		aw87xxx_monitor_start(aw_monitor);
	}

	return count;
}

static DEVICE_ATTR_RW(esd_enable);
static DEVICE_ATTR_RW(vbat);
static DEVICE_ATTR_RW(vmax);

static DEVICE_ATTR_RW(monitor);
static DEVICE_ATTR_RW(monitor_time);
static DEVICE_ATTR_RW(monitor_count);
static DEVICE_ATTR_RW(rx);
static DEVICE_ATTR_RW(monitor_update);
static DEVICE_ATTR_RO(monitor_params);

static struct attribute *aw_monitor_vol_adjust[] = {
	&dev_attr_esd_enable.attr,
	&dev_attr_vbat.attr,
	&dev_attr_vmax.attr,
#ifdef AW_DEBUG
	&dev_attr_vol.attr,
	&dev_attr_temp.attr,
#endif
	NULL
};

static struct attribute_group aw_monitor_vol_adjust_group = {
	.attrs = aw_monitor_vol_adjust,
};

static struct attribute *aw_monitor_control[] = {
	&dev_attr_monitor.attr,
	&dev_attr_monitor_time.attr,
	&dev_attr_monitor_count.attr,
	&dev_attr_rx.attr,
	&dev_attr_monitor_update.attr,
	&dev_attr_monitor_params.attr,
	NULL
};

static struct attribute_group aw_monitor_control_group = {
	.attrs = aw_monitor_control,
};

/***************************************************************************
 *
 * aw87xxx monitor init
 *
 ***************************************************************************/
static void aw_monitor_dtsi_parse(struct device *dev,
				struct aw_monitor *monitor,
				struct device_node *dev_node)
{
	int ret = -1;
	const char *esd_enable;

	ret = of_property_read_string(dev_node, "esd-enable", &esd_enable);
	if (ret < 0) {
		AW_DEV_LOGD(dev, "esd_enable parse failed, user default[disable]");
		monitor->esd_enable = AW_ESD_DISABLE;
	} else {
		if (!strcmp(esd_enable, "false"))
			monitor->esd_enable = AW_ESD_DISABLE;
		else
			monitor->esd_enable = AW_ESD_ENABLE;

		AW_DEV_LOGD(dev, "parse esd-enable=[%s]",
				monitor->esd_enable ? "true" : "false");
	}
}

void aw87xxx_monitor_init(struct device *dev, struct aw_monitor *monitor,
				struct device_node *dev_node)
{
	int ret = -1;
	struct aw87xxx *aw87xxx =
		container_of(monitor, struct aw87xxx, monitor);

	monitor->dev_index = aw87xxx->dev_index;
	monitor->monitor_hdr.monitor_time = AW_DEFAULT_MONITOR_TIME;

	aw_monitor_dtsi_parse(dev, monitor, dev_node);

	/* get platform open dsp type */
	monitor->open_dsp_en = aw87xxx_dsp_isEnable();

	ret = sysfs_create_group(&dev->kobj, &aw_monitor_vol_adjust_group);
	if (ret < 0)
		AW_DEV_LOGE(dev, "failed to create monitor vol_adjust sysfs nodes");

	INIT_DELAYED_WORK(&monitor->with_dsp_work, aw_monitor_work_func);

	if (monitor->open_dsp_en) {
		ret = sysfs_create_group(&dev->kobj, &aw_monitor_control_group);
		if (ret < 0)
			AW_DEV_LOGE(dev, "failed to create monitor dsp control sysfs nodes");
	}

	if (!ret)
		AW_DEV_LOGD(dev, "monitor init succeed");
}

void aw87xxx_monitor_exit(struct aw_monitor *monitor)
{
	struct aw87xxx *aw87xxx =
		container_of(monitor, struct aw87xxx, monitor);
	/*rm attr node*/
	sysfs_remove_group(&aw87xxx->dev->kobj,
			&aw_monitor_vol_adjust_group);

	aw87xxx_monitor_stop(monitor);

	if (monitor->open_dsp_en) {
		sysfs_remove_group(&aw87xxx->dev->kobj,
				&aw_monitor_control_group);
	}
}


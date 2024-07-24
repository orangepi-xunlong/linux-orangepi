/* SPDX-License-Identifier: GPL-2.0
 * aw87xxx_monitor.h
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

#ifndef __AW87XXX_MONITOR_H__
#define __AW87XXX_MONITOR_H__

/*#define AW_DEBUG*/

#define AW_WAIT_DSP_OPEN_TIME			(3000)
#define AW_VBAT_CAPACITY_MIN			(0)
#define AW_VBAT_CAPACITY_MAX			(100)
#define AW_VMAX_INIT_VAL			(0xFFFFFFFF)
#define AW_VBAT_MAX				(100)
#define AW_VMAX_MAX				(0)
#define AW_DEFAULT_MONITOR_TIME			(3000)
#define AW_WAIT_TIME				(3000)
#define REG_STATUS_CHECK_MAX			(5)
#define AW_ESD_CHECK_DELAY			(1)
#define AW_MONOTOR_ESD_ERR_CNT_MAX		(3)


#define AW_MONITOR_TIME_MIN			(0)
#define AW_MONITOR_TIME_MAX			(50000)


#define AW_ESD_ENABLE				(true)
#define AW_ESD_DISABLE				(false)
#define AW_ESD_ENABLE_STRLEN			(16)
#define MONITOR_EN_MASK  0x01

#define AW_TABLE_SIZE	sizeof(struct aw_table)

#define AW_GET_32_DATA(w, x, y, z) \
		((uint32_t)((((uint8_t)w) << 24) | (((uint8_t)x) << 16) | (((uint8_t)y) << 8) | ((uint8_t)z)))
#define AW_GET_16_DATA(x, y) \
		((uint16_t)((((uint8_t)x) << 8) | (uint8_t)y))

#define IPEAK_NONE	(0xFF)
#define GAIN_NONE	(0xFF)
#define VMAX_NONE	(0xFFFFFFFF)

enum {
	AW_MON_LOGIC_OR = 0,
	AW_MON_LOGIC_AND = 1,
};

enum {
	MONITOR_EN_BIT = 0,
	MONITOR_LOGIC_BIT = 1,
	MONITOR_IPEAK_EN_BIT = 2,
	MONITOR_GAIN_EN_BIT = 3,
	MONITOR_VMAX_EN_BIT = 4,
	MONITOR_TEMP_EN_BIT = 5,
	MONITOR_VOL_EN_BIT = 6,
	MONITOR_TEMPERATURE_SOURCE_BIT = 7,
	MONITOR_VOLTAGE_SOURCE_BIT = 8,
	MONITOR_VOLTAGE_MODE_BIT = 9,
};

enum {
	AW_SYS_VOLTAGE_NOW = 0,
	AW_SYS_CAPACITY = 1,
};

enum {
	AW_VOLTAGE_INFO = 0,
	AW_CAPACITY_INFO = 1,
	AW_TEMPERATURE_INFO = 2,
};

enum {
	AW_CHIP_INFO = 0,
	AW_PLATFORM_INFO = 1,
};

enum aw_monitor_init {
	AW_MONITOR_CFG_WAIT = 0,
	AW_MONITOR_CFG_OK = 1,
};

enum aw_monitor_hdr_info {
	AW_MONITOR_HDR_DATA_SIZE = 0x00000004,
	AW_MONITOR_HDR_DATA_BYTE_LEN = 0x00000004,
};

enum aw_monitor_data_ver {
	AW_MONITOR_DATA_VER = 0x00000001,
	AW_MONITOR_DATA_VER_MAX,
};

enum aw_monitor_hdr_ver {
	AW_MONITOR_HDR_VER_0_1_0 = 0x00010000,
	AW_MONITOR_HDR_VER_0_1_1 = 0x00010100,
	AW_MONITOR_HDR_VER_0_1_2 = 0x00010200,
};

enum aw_monitor_first_enter {
	AW_FIRST_ENTRY = 0,
	AW_NOT_FIRST_ENTRY = 1,
};

struct aw_container {
	int len;
	uint8_t data[];
};

struct aw_table {
	int16_t min_val;
	int16_t max_val;
	uint16_t ipeak;
	uint16_t gain;
	uint32_t vmax;
};

struct aw_bin_header {
	uint32_t check_sum;
	uint32_t header_ver;
	uint32_t bin_data_type;
	uint32_t bin_data_ver;
	uint32_t bin_data_size;
	uint32_t ui_ver;
	char product[8];
	uint32_t addr_byte_len;
	uint32_t data_byte_len;
	uint32_t device_addr;
	uint32_t reserve[4];
};

struct aw_monitor_header {
	uint32_t monitor_switch;
	uint32_t monitor_time;
	uint32_t monitor_count;
	uint32_t step_count;
	uint32_t reserve[4];
};


/* v0.1.2 */
struct aw_monitor_hdr {
	uint32_t check_sum;
	uint32_t monitor_ver;
	char chip_type[16];
	uint32_t ui_ver;
	uint32_t monitor_time;
	uint32_t monitor_count;
	uint32_t enable_flag;
		/* [bit 31:7]*/
		/* [bit 9: voltage mode]*/
		/* [bit 8: voltage source]*/
		/* [bit 7: temperature source]*/
		/* [bit 6: vol en]*/
		/* [bit 5: temp en]*/
		/* [bit 4: vmax en]*/
		/* [bit 3: gain en]*/
		/* [bit 2: ipeak en]*/
		/* [bit 1: & or | flag]*/
		/* [bit 0: monitor en]*/
	uint32_t temp_aplha;
	uint32_t temp_num;
	uint32_t single_temp_size;
	uint32_t temp_offset;
	uint32_t vol_aplha;
	uint32_t vol_num;
	uint32_t single_vol_size;
	uint32_t vol_offset;
	uint32_t reserver[3];
};

struct vmax_step_config {
	uint32_t vbat_min;
	uint32_t vbat_max;
	int vmax_vol;
};

struct aw_table_info {
	uint8_t table_num;
	struct aw_table *aw_table;
};

struct aw_monitor_cfg {
	uint8_t monitor_status;
	uint32_t monitor_switch;
	uint32_t monitor_time;
	uint32_t monitor_count;
	uint32_t logic_switch;
	uint32_t temp_switch;
	uint32_t temp_aplha;
	uint32_t vol_switch;
	uint32_t vol_aplha;
	uint32_t ipeak_switch;
	uint32_t gain_switch;
	uint32_t vmax_switch;
	uint32_t temp_source;
	uint32_t vol_source;
	uint32_t vol_mode;
	struct aw_table_info temp_info;
	struct aw_table_info vol_info;
};

struct aw_monitor_trace {
	int32_t pre_val;
	int32_t sum_val;
	struct aw_table aw_table;
};

struct aw_monitor {
	bool open_dsp_en;
	bool esd_enable;
	int32_t dev_index;
	uint8_t first_entry;
	uint8_t timer_cnt;
	uint32_t vbat_sum;
	int32_t custom_capacity;
	uint32_t pre_vmax;
	uint32_t esd_err_cnt;
	uint8_t samp_count;
	uint32_t version;

#ifdef AW_DEBUG
	uint16_t test_vol;
	int16_t test_temp;
#endif

	int bin_status;
	struct aw_monitor_header monitor_hdr;
	struct vmax_step_config *vmax_cfg;
	struct aw_monitor_cfg monitor_cfg;

	struct aw_monitor_trace temp_trace;
	struct aw_monitor_trace vol_trace;
	struct aw_container *monitor_container;

	struct delayed_work with_dsp_work;
};

void aw87xxx_monitor_cfg_free(struct aw_monitor *monitor);
int aw87xxx_monitor_bin_parse(struct device *dev,
			const char *monitor_data, uint32_t data_len);
void aw87xxx_monitor_stop(struct aw_monitor *monitor);
void aw87xxx_monitor_start(struct aw_monitor *monitor);
int aw87xxx_monitor_no_dsp_get_vmax(struct aw_monitor *monitor,
					int32_t *vmax);
void aw87xxx_monitor_init(struct device *dev, struct aw_monitor *monitor,
				struct device_node *dev_node);
void aw87xxx_monitor_exit(struct aw_monitor *monitor);
int aw87xxx_dev_monitor_switch_set(struct aw_monitor *monitor, uint32_t enable);

#endif

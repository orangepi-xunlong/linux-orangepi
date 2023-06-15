/*
 * configs for sensor list.
 *
 * Copyright (c) 2020 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Li Huiyu <lihuiyu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sensor_list.h"

#ifdef CONFIG_SENSOR_LIST_MODULE

struct sensor_list rear_sl = {
	.used = REAR_USED,
	.csi_sel = REAR_CSI_SEL,
	.device_sel = REAR_DEVICE_SEL,
	.sensor_bus_sel = REAR_SENSOR_TWI_ID,
	.inst = {
		{
			.cam_name = REAR_SENSOR_NAME0,
			.cam_addr = REAR_SENSOR_TWI_ADDR0,
			.cam_type = REAR_SENSOR_TYPE0,
			.vflip = REAR_SENSOR_VFLIP0,
			.hflip = REAR_SENSOR_HFLIP0,
			.act_used = REAR_ACT_USED0,
			.act_name = REAR_ACT_NAME0,
			.act_addr = REAR_ACT_TWI_ADDR0,
		},

		{
			.cam_name = REAR_SENSOR_NAME1,
			.cam_addr = REAR_SENSOR_TWI_ADDR1,
			.cam_type = REAR_SENSOR_TYPE1,
			.vflip = REAR_SENSOR_VFLIP1,
			.hflip = REAR_SENSOR_HFLIP1,
			.act_used = REAR_ACT_USED1,
			.act_name = REAR_ACT_NAME1,
			.act_addr = REAR_ACT_TWI_ADDR1,
		},

		{
			.cam_name = REAR_SENSOR_NAME2,
			.cam_addr = REAR_SENSOR_TWI_ADDR2,
			.cam_type = REAR_SENSOR_TYPE2,
			.vflip = REAR_SENSOR_VFLIP2,
			.hflip = REAR_SENSOR_HFLIP2,
			.act_used = REAR_ACT_USED2,
			.act_name = REAR_ACT_NAME2,
			.act_addr = REAR_ACT_TWI_ADDR2,
		},
	},
};

struct sensor_list front_sl = {
	.used = FRONT_USED,
	.csi_sel = FRONT_CSI_SEL,
	.device_sel = FRONT_DEVICE_SEL,
	.sensor_bus_sel = FRONT_SENSOR_TWI_ID,
	.inst = {
		{
			.cam_name = FRONT_SENSOR_NAME0,
			.cam_addr = FRONT_SENSOR_TWI_ADDR0,
			.cam_type = FRONT_SENSOR_TYPE0,
			.vflip = FRONT_SENSOR_VFLIP0,
			.hflip = FRONT_SENSOR_HFLIP0,
			.act_used = FRONT_ACT_USED0,
			.act_name = FRONT_ACT_NAME0,
			.act_addr = FRONT_ACT_TWI_ADDR0,
		},

		{
			.cam_name = FRONT_SENSOR_NAME1,
			.cam_addr = FRONT_SENSOR_TWI_ADDR1,
			.cam_type = FRONT_SENSOR_TYPE1,
			.vflip = FRONT_SENSOR_VFLIP1,
			.hflip = FRONT_SENSOR_HFLIP1,
			.act_used = FRONT_ACT_USED1,
			.act_name = FRONT_ACT_NAME1,
			.act_addr = FRONT_ACT_TWI_ADDR1,
		},

		{
			.cam_name = FRONT_SENSOR_NAME2,
			.cam_addr = FRONT_SENSOR_TWI_ADDR2,
			.cam_type = FRONT_SENSOR_TYPE2,
			.vflip = FRONT_SENSOR_VFLIP2,
			.hflip = FRONT_SENSOR_HFLIP2,
			.act_used = FRONT_ACT_USED2,
			.act_name = FRONT_ACT_NAME2,
			.act_addr = FRONT_ACT_TWI_ADDR2,
		},
	},
};

static void set_used(struct sensor_list *sensors,
		struct sensor_list *source)
{
	sensors->used = source->used;
	vin_log(VIN_LOG_CONFIG, "sensors->used = %d!\n", sensors->used);
}
static void set_csi_sel(struct sensor_list *sensors,
		struct sensor_list *source)
{
	sensors->csi_sel = source->csi_sel;
	vin_log(VIN_LOG_CONFIG, "sensors->csi_sel = %d!\n",
			sensors->csi_sel);

}
static void set_device_sel(struct sensor_list *sensors,
		struct sensor_list *source)
{
	sensors->device_sel = source->device_sel;
	vin_log(VIN_LOG_CONFIG, "sensors->device_sel = %d!\n",
			sensors->device_sel);

}
static void set_sensor_twi_id(struct sensor_list *sensors,
		struct sensor_list *source)
{
	sensors->sensor_bus_sel = source->sensor_bus_sel;
	vin_log(VIN_LOG_CONFIG, "sensors->sensor_bus_sel = %d!\n",
			sensors->sensor_bus_sel);
}

static void __set_sensor_inst(struct sensor_list *sensors,
		struct sensor_list *source, unsigned int cnt)
{
	struct sensor_instance *inst;
	struct sensor_instance *inst_source;

	if (cnt >= MAX_DETECT_NUM)
		return;
	inst = &sensors->inst[cnt];
	inst_source = &source->inst[cnt];

	if (strcmp(inst_source->cam_name, ""))
		strcpy(inst->cam_name, inst_source->cam_name);
	if (strcmp(inst_source->act_name, ""))
		strcpy(inst->act_name, inst_source->act_name);
	if (inst_source->cam_addr != 0)
		inst->cam_addr = inst_source->cam_addr;
	if (inst_source->act_addr != 0)
		inst->act_addr = inst_source->act_addr;

	inst->cam_type = inst_source->cam_type;
	inst->act_used = inst_source->act_used;
	inst->vflip = inst_source->vflip;
	inst->hflip = inst_source->hflip;

	vin_log(VIN_LOG_CONFIG, "sensors->cam_name = %s!\n", inst->cam_name);
	vin_log(VIN_LOG_CONFIG, "sensors->cam_addr = 0x%x!\n", inst->cam_addr);
	vin_log(VIN_LOG_CONFIG, "sensors->act_name = %s!\n", inst->act_name);
	vin_log(VIN_LOG_CONFIG, "sensors->act_addr = 0x%x!\n", inst->act_addr);
	vin_log(VIN_LOG_CONFIG, "sensors->hflip = %d!\n", inst->hflip);
	vin_log(VIN_LOG_CONFIG, "sensors->vflip = %d!\n", inst->vflip);
	vin_log(VIN_LOG_CONFIG, "sensors->cam_type = %d!\n", inst->cam_type);
	vin_log(VIN_LOG_CONFIG, "sensors->act_used = %d!\n", inst->act_used);

}

static void set_sensor_inst(struct sensor_list *sensors,
		struct sensor_list *source, unsigned int det_num)
{
	int i;

	if (det_num > MAX_DETECT_NUM)
		det_num  = MAX_DETECT_NUM;

	for (i = 0; i < det_num; i++) {
		__set_sensor_inst(sensors, source, i);
	}
}

static struct fetch_sl fetch_list[] = {
	{1, set_used},
	{1, set_csi_sel},
	{1, set_device_sel},
	{1, set_sensor_twi_id},
};

#define FETCH_SIZE (ARRAY_SIZE(fetch_list))

int sensor_list_get_parms(struct sensor_list *sensors, char *pos)
{
	struct sensor_list *source;
	int i;

	if (strcmp(pos, "rear") && strcmp(pos, "REAR") && strcmp(pos, "FRONT")
			&& strcmp(pos, "front")) {
		vin_err("Camera position config ERR! POS = %s\n", pos);
		return -EINVAL;
	}
	if (strcmp(pos, "rear") == 0 || strcmp(pos, "REAR") == 0) {
		source = &rear_sl;
		sensors->detect_num = REAR_DETECT_NUM;
	} else {
		source = &front_sl;
		sensors->detect_num = FRONT_DETECT_NUM;
	}

	for (i = 0; i < FETCH_SIZE; i++) {
		if (fetch_list[i].flag)
			fetch_list[i].fun(sensors, source);
	}
	set_sensor_inst(sensors, source, sensors->detect_num);

	return 0;
}
#else

int sensor_list_get_parms(struct sensor_list *sensors, char *pos)
{
	return 0;
}
#endif


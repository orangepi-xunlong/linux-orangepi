/*
 ******************************************************************************
 *
 * vin_core.h
 *
 * Hawkview ISP - vin_core.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		Author         Date		    Description
 *
 *   3.0		Yang Feng   	2015/12/02	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef _VIN_CORE_H_
#define _VIN_CORE_H_

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/pm_runtime.h>

#include <media/sunxi_camera_v2.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>

#include "../platform/platform_cfg.h"
#include "../modules/flash/flash.h"

#include "../modules/actuator/actuator.h"
#include "../vin-csi/bsp_csi.h"
#include "../vin-mipi/bsp_mipi_csi.h"
#include "../vin-isp/bsp_isp.h"
#include "../vin-isp/bsp_isp_algo.h"
#include "../utility/vin_supply.h"
#include "vin_video.h"

#define MAX_FRAME_MEM   (150*1024*1024)
#define MIN_WIDTH       (32)
#define MIN_HEIGHT      (32)
#define MAX_WIDTH       (4800)
#define MAX_HEIGHT      (4800)
#define DUMP_CSI		(1 << 0)
#define DUMP_ISP		(1 << 1)

struct vin_coor {
	unsigned int x1;
	unsigned int y1;
	unsigned int x2;
	unsigned int y2;
};

enum vin_sub_device_regulator {
	ENUM_IOVDD,
	ENUM_AVDD,
	ENUM_DVDD,
	ENUM_AFVDD,
	ENUM_FLVDD,
	ENUM_MAX_REGU,
};

struct vin_power {
	struct regulator *pmic;
	int power_vol;
	char power_str[32];
};
struct sensor_instance {
	char cam_name[I2C_NAME_SIZE];
	int cam_addr;
	int cam_type;
	int is_isp_used;
	int is_bayer_raw;
	int vflip;
	int hflip;
	int act_addr;
	char act_name[I2C_NAME_SIZE];
	char isp_cfg_name[I2C_NAME_SIZE];
};

struct sensor_list {
	int use_sensor_list;
	int used;
	int csi_sel;
	int device_sel;
	int twi_id;
	int power_set;
	int detect_num;
	char sensor_pos[32];
	int valid_idx;
	struct vin_power power[ENUM_MAX_REGU];
	struct gpio_config gpio[MAX_GPIO_NUM];
	struct sensor_instance inst[MAX_DETECT_NUM];
};

enum module_type {
	VIN_MODULE_TYPE_CCI,
	VIN_MODULE_TYPE_I2C,
	VIN_MODULE_TYPE_SPI,
	VIN_MODULE_TYPE_GPIO,
	VIN_MODULE_TYPE_MAX,
};

struct vin_act_info {
	struct v4l2_subdev *sd;
	enum module_type type;
	int id;
};

struct vin_flash_info {
	struct v4l2_subdev *sd;
	enum module_type type;
	int id;
};

struct vin_sensor_info {
	struct v4l2_subdev *sd;
	enum module_type type;
	int id;
};

struct vin_module_info {
	struct vin_act_info act[MAX_DETECT_NUM];
	struct vin_flash_info flash;
	struct vin_sensor_info sensor[MAX_DETECT_NUM];
	int id;
};

struct modules_config {
	struct vin_module_info modules;
	struct sensor_list sensors;
	int bus_sel;
	int flash_used;
	int act_used;
};

struct vin_pipeline_cfg {
	int mipi_ind;
	int csi_ind;
	int isp_ind;
	int scaler_ind;
};

struct vin_core {
	struct platform_device *pdev;
	int id;
	/* various device info */
	struct vin_vid_cap vid_cap;
	/* about system resource */
	struct regulator *vin_system_power[3];
	int vin_sensor_power_cnt;
	/* about vfe channel */
	unsigned int cur_ch;
	/* about some global info */
	enum v4l2_mbus_type mbus_type;
	unsigned int csi_sel;
	unsigned int mipi_sel;
	unsigned int scaler_sel;
	struct modules_config modu_cfg;
	unsigned int platform_id;
	struct vin_pipeline_cfg pipe_cfg;

	struct v4l2_device *v4l2_dev;
	const struct vin_pipeline_ops *pipeline_ops;
	int support_raw;
	struct v4l2_subdev *csi_sd;
	int irq;
};

static inline struct sensor_instance *get_valid_sensor(struct vin_core *vinc)
{
	int valid_idx = vinc->modu_cfg.sensors.valid_idx;
	return &vinc->modu_cfg.sensors.inst[valid_idx];
}

int sunxi_vin_core_register_driver(void);
void sunxi_vin_core_unregister_driver(void);
struct vin_core *sunxi_vin_core_get_dev(int index);
struct vin_fmt *vin_get_format(unsigned int index);
void vin_get_fmt_mplane(struct vin_frame *frame, struct v4l2_format *f);
struct vin_fmt *vin_find_format(const u32 *pixelformat, const u32 *mbus_code,
				  unsigned int mask, int index, bool have_code);
int vin_core_check_sensor_list(struct vin_core *vinc, int i);

#endif /*_VIN_CORE_H_*/


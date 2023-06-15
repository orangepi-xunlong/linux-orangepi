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

#ifndef __SENSOR_LIST_H__
#define __SENSOR_LIST_H__

#include "../../platform/platform_cfg.h"
#include "../../vin-video/vin_core.h"
#include "../../vin.h"

/************************bus config******************************
 *REAR_USED               0: not used, 	1: used;
 *REAR_CSI_SEL            0: mipi, 	    1:  parallel ;
 *REAR_DEVICE_SEL         0: dev0, 	    1: dev1;
 *REAR_SENSOR_TWI_ID      twi id, for example: 0 : i2c0; 1 : i2c1
 *
 ************************detect sensor configs*******************
 *REAR_DETECT_NUM:			The number of sensors need be detected in this bus.
 *REAR_SENSOR_NAME0        The sensor name in sensor driver.
 *REAR_SENSOR_TWI_ADDR0    The i2c address of this sensor.
 *REAR_SENSOR_TYPE0        The sensor type, 0: YUV, 1: RAW;
 *REAR_SENSOR_STBY_MODE0	 Not used;
 *REAR_SENSOR_HFLIP0       Horizontal flip;
 *REAR_SENSOR_VFLIP0       Vertical  flip;
 *REAR_ACT_NAME0           The VCM name in vcm driver, only RAW sensor need be configured;
 *REAR_ACT_TWI_ADDR0       The i2c address of this VCM;
 *
 *****************************************************************/


/**************************************REAR SESNOR COFNIG***************************************/

#define REAR_USED                       1
#define REAR_CSI_SEL                    0
#define REAR_DEVICE_SEL                 0
#define REAR_SENSOR_TWI_ID              2

/*********DETECT SESNOR OCNFIG*********/

#define REAR_DETECT_NUM                 3


#define REAR_SENSOR_NAME0               "gc5025_mipi"
#define REAR_SENSOR_TWI_ADDR0           0x6e
#define REAR_SENSOR_TYPE0               1
#define REAR_SENSOR_STBY_MODE0          0
#define REAR_SENSOR_HFLIP0              0
#define REAR_SENSOR_VFLIP0              0
#define REAR_ACT_USED0              	1
#define REAR_ACT_NAME0                  "dw9714_act"
#define REAR_ACT_TWI_ADDR0              0x18

#define REAR_SENSOR_NAME1               "sp5409_mipi"
#define REAR_SENSOR_TWI_ADDR1           0x78
#define REAR_SENSOR_TYPE1               1
#define REAR_SENSOR_STBY_MODE1          0
#define REAR_SENSOR_HFLIP1              0
#define REAR_SENSOR_VFLIP1              0
#define REAR_ACT_USED1	             	0
#define REAR_ACT_NAME1                  "dw9714_act"
#define REAR_ACT_TWI_ADDR1              0x18

#define REAR_SENSOR_NAME2               "gc2385_mipi_2"
#define REAR_SENSOR_TWI_ADDR2           0x6c
#define REAR_SENSOR_TYPE2               1
#define REAR_SENSOR_STBY_MODE2          0
#define REAR_SENSOR_HFLIP2              0
#define REAR_SENSOR_VFLIP2              0
#define REAR_ACT_USED2	             	0
#define REAR_ACT_NAME2                  "dw9714_act"
#define REAR_ACT_TWI_ADDR2              0x18


/**************************************FRONT SESNOR COFNIG***************************************/
#define FRONT_USED                      1
#define FRONT_CSI_SEL                   0
#define FRONT_DEVICE_SEL                0
#define FRONT_SENSOR_TWI_ID             2

/*********DETECT SESNOR OCNFIG*********/

#define FRONT_DETECT_NUM                3

#define FRONT_SENSOR_NAME0              "gc2385_mipi"
#define FRONT_SENSOR_TWI_ADDR0          0x6a
#define FRONT_SENSOR_TYPE0              1
#define FRONT_SENSOR_STBY_MODE0         0
#define FRONT_SENSOR_HFLIP0             0
#define FRONT_SENSOR_VFLIP0             0
#define FRONT_ACT_USED0             	0
#define FRONT_ACT_NAME0                 ""
#define FRONT_ACT_TWI_ADDR0             0

#define FRONT_SENSOR_NAME1              "gc030a_mipi"
#define FRONT_SENSOR_TWI_ADDR1          0x42
#define FRONT_SENSOR_TYPE1              1
#define FRONT_SENSOR_STBY_MODE1         0
#define FRONT_SENSOR_HFLIP1             0
#define FRONT_SENSOR_VFLIP1             0
#define FRONT_ACT_USED1              	0
#define FRONT_ACT_NAME1                 ""
#define FRONT_ACT_TWI_ADDR1             0

#define FRONT_SENSOR_NAME2              "c2590_mipi"
#define FRONT_SENSOR_TWI_ADDR2          0x6c
#define FRONT_SENSOR_TYPE2              1
#define FRONT_SENSOR_STBY_MODE2         0
#define FRONT_SENSOR_HFLIP2             0
#define FRONT_SENSOR_VFLIP2             0
#define FRONT_ACT_USED2             	0
#define FRONT_ACT_NAME2                 ""
#define FRONT_ACT_TWI_ADDR2             0

struct fetch_sl{
	int flag;
	void (*fun)(struct sensor_list *, struct sensor_list *);
};

int sensor_list_get_parms(struct sensor_list *sensors, char *pos);

#endif

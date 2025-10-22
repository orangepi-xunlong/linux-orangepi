#ifndef _RK628_TWI_HELPER_H
#define _RK628_TWI_HELPER_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "../camera.h"
#include "../sensor_helper.h"

// 1: big endian
// 0: little endian
#define RK628_REG_ENDIAN_BIG 0

int read_rk628(struct v4l2_subdev *sd, unsigned int addr, unsigned int *value);
int write_rk628(struct v4l2_subdev *sd, unsigned int addr, unsigned int value);

#endif

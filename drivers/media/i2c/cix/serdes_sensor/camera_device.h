/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * camera device driver header
 *
 * Copyright (c) 2018-2022 NVIDIA Corporation.  All rights reserved.
 */

#ifndef __UAPI_CAMERA_DEVICE_H_
#define __UAPI_CAMERA_DEVICE_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define __CAMERA_DEVICE_ALIGN __aligned(8)

/* Sensor, focuser, iris etc., */
#define MAX_DEVICES_PER_CHANNEL 4

/*
 * Increasing below values must validate
 * copy_from or copy_to works properly
 */
#define MAX_COMMANDS 256
#define MAX_BLOB_SIZE 2048

struct i2c_bus {
	__u32 reg_base;
	__u32 clk_rate;
	__u32 flags;
	__u8 reserved[4];
};

struct i2c_mux {
	bool is_mux_valid;
	__u8 mux_channel;
	__u16 mux_addr;
	__u8 reserved[4];
};

struct i2c_dev {
	__u16 addr;
	__u8 pad[2];
	__u32 flags;
};

struct spi_bus {
	__u32 reg_base;
	__u32 clk_rate;
	__u32 flags;
	__u8 reserved[4];
};

struct spi_dev {
	__u8 port;
	__u16 addr;
	__u8 pad;
	__u32 flags;
	__u8 pad1[4];
};

struct i2c_sensor_cfg {
	__u32 num_devs;
	struct i2c_bus bus;
	struct i2c_mux mux;
	struct i2c_dev sd[MAX_DEVICES_PER_CHANNEL];
};

struct spi_sensor_cfg {
	__u32 num_devs;
	struct spi_bus bus;
	struct spi_dev sd[MAX_DEVICES_PER_CHANNEL];
};

struct sensor_cfg {
	__u8 type; /* SPI or I2C */
	__u8 pad[3]; /* for alignment */
	union {
		struct i2c_sensor_cfg i2c_sensor;
		struct spi_sensor_cfg spi_sensor;
	} u;
} __CAMERA_DEVICE_ALIGN;

struct sensor_cmd {
	__u32 opcode;
	__u32 addr;
};

struct sensor_blob {
	__u32 num_cmds;
	__u32 buf_size;
	struct sensor_cmd cmds[MAX_COMMANDS];
	__u8 buf[MAX_BLOB_SIZE];
} __CAMERA_DEVICE_ALIGN;

struct sensor_blob_cfg {
	__u32 nlines;
	struct sensor_blob *blob;
} __CAMERA_DEVICE_ALIGN;

#define CAMERA_DEVICE_NONE		0
#define	CAMERA_DEVICE_I2C_SENSOR	(0x1 << 1)
#define	CAMERA_DEVICE_SPI_SENSOR	(0x1 << 2)
/* Future extensions - if necessary */
#define	CAMERA_DEVICE_VI		(0x1 << 8)
#define	CAMERA_DEVICE_CSI		(0x1 << 9)
#define CAMERA_DEVICE_ISP		(0x1 << 16)

struct camdev_chan_cfg {
	__u32 type;
	struct sensor_cfg scfg;
} __CAMERA_DEVICE_ALIGN;

/* common functionality */
#define CAMERA_DEVICE_REGISTER _IOW('C', 1, struct camdev_chan_cfg)
#define CAMERA_DEVICE_UNREGISTER _IOW('C', 2, __u32)
/* sensor functionality */
#define SENSOR_BLOB_EXECUTE _IOW('C', 10, struct sensor_blob_cfg)

#endif

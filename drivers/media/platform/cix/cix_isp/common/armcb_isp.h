/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ARMCB_ISP_H__
#define __ARMCB_ISP_H__

#include <linux/types.h>
#include <linux/videodev2.h>

/// Define data type
enum drv_data_type {
	DRV_DATA_TYPE_INVALID = 0,
	DRV_DATA_TYPE_BYTE = 1,
	DRV_DATA_TYPE_WORD = 2,
	DRV_DATA_TYPE_DWORD = 3,
	DRV_DATA_TYPE_WORD_REVERSE = 4,
	DRV_DATA_TYPE_MAX
};

/// Define address type
enum drv_addr_type {
	DRV_ADDR_TYPE_INVALID = 0,
	DRV_ADDR_TYPE_BYTE = 1,
	DRV_ADDR_TYPE_WORD = 2,
	DRV_ADDR_TYPE_WORD_REVERSE = 3,
	DRV_ADDR_TYPE_MAX
};

/// Define direction
enum drv_direction {
	DRV_DIRECTION_WRITE = 0, /// write only
	DRV_DIRECTION_READ = 1, /// read only
	DRV_DIRECTION_READ_POLL = 2, ///  read until getting wanted value
	DRV_DIRECTION_WRITE_POLL = 3, /// write and read for taking effect @TODO
	DRV_DIRECTION_MAX,
};

/// Define type of hardware connect
enum hw_type {
	DRV_HW_I2C = 0,
	DRV_HW_SPI,
	DRV_HW_ISP,
	DRV_HW_MAX,
};

/// Define type of hardware bus connect
enum hw_bus_type {
	HW_BUS_I2C = 0,
	HW_BUS_SPI,
	HW_BUS_GPIO,
	HW_BUS_PMIC,
	HW_BUS_AHB,
	HW_BUS_AHB_POWER,
	HW_BUS_DMA_REG,
	HW_BUS_XDMA_REG,
	HW_BUS_DMA_SRAM,
	HW_BUS_XDMA_SRAM,
	HW_BUS_MAX,
};

/// Define type of device
enum dev_type {
	DRV_DEV_SENSOR = 0,
	DRV_DEV_ACTUOAOR,
	DRV_DEV_ISP,
	DRV_DEV_CSI,
	DRV_DEV_MAX,
};

/// Define type of hardware connect
enum imgs_power_name {
	IMGS_POWER_NULL = 0,
	IMGS_POWER_AVDD,
	IMGS_POWER_VIO,
	IMGS_POWER_DVDD,
	IMGS_POWER_ENABLE,
	IMGS_POWER_REST,
	IMGS_POWER_PWDN,
	IMGS_POWER_MCLK,
	IMGS_POWER_MAX,
};

/// Define power type
enum drv_power_type {
	DRV_POWER_INVALID = 0,
	DRV_POWER_REG = 1,
	DRV_POWER_GPIO = 2,
	DRV_POWER_PMIC = 3,
	DRV_POWER_MCLK = 4,
	DRV_POWER_MAX
};

typedef enum {
	ISP_OUTPUT_PORT_VIN = 0,
	ISP_OUTPUT_PORT_3A,
	ISP_OUTPUT_PORT_VOUT0,
	ISP_OUTPUT_PORT_VOUT1,
	ISP_OUTPUT_PORT_VOUT2,
	ISP_OUTPUT_PORT_VOUT3,
	ISP_OUTPUT_PORT_VOUT4,
	ISP_OUTPUT_PORT_VOUT5,
	ISP_OUTPUT_PORT_VOUT6,
	ISP_OUTPUT_PORT_VOUT7,
	ISP_OUTPUT_PORT_VOUT8,
	ISP_OUTPUT_PORT_VOUT9,
	ISP_OUTPUT_PORT_MAX
} isp_output_port_t;

/// Define register address and value
struct reg_cfg {
	unsigned int addr; /// register address
	unsigned int val; /// register configure value
	unsigned int sof_delay; /// Delay(us) of register operation;
};

/// Define sensor single register information
struct drv_sensor_reg_info {
	unsigned int reg_addr; /// Address of register
	unsigned int reg_data; /// Data of register
	unsigned int delay_us; /// Delay(us) of register operation;
};

/// Define register array information
struct drv_regs_array_info {
	enum drv_data_type reg_data_type; /// register data type
	enum drv_addr_type reg_addr_type; /// register address type
	enum drv_direction direction; /// Read or write
	unsigned int setting_size; /// Size of register array
	struct drv_sensor_reg_info *settings; /// Register array
};

/// register configuration structure
struct isp_reg_list {
	unsigned short num; /// size of isp register cfg buf
	struct reg_cfg *cfg; /// address and value pair
	struct isp_reg_list *next; /// point to next register hw update block
};

/// Define registers array configure
struct isp_hw_regs_cfg {
	unsigned int channel;
	unsigned int slave_addr;
	struct drv_regs_array_info *p_regs_info;
};

/// Define register list configure
struct isp_hw_list_cfg {
	unsigned int frame_index;
	struct isp_reg_list *p_update_list;
};

/// Define hardware request
struct isp_hw_req {
	enum hw_type hw_type; /// Hardware type
	enum dev_type dev_type; /// Device type
	void *argv; /// argv of hardware request
};

/// I2C setting
struct cmd_i2c_setting {
	unsigned int channel;
	unsigned int slave_addr;
	enum drv_direction direct;
	enum drv_addr_type reg_addr_type;
	enum drv_data_type reg_data_type;
	unsigned int reg_addr;
	unsigned int *ptr_user;
	unsigned int val;
	unsigned int delay_us;
};

/// SPI setting
struct cmd_spi_setting {
	unsigned int channel;
	enum drv_direction direct;
	enum drv_addr_type reg_addr_type;
	enum drv_data_type reg_data_type;
	unsigned int reg_addr;
	unsigned int *ptr_user;
	unsigned int val;
	unsigned int delay_us;
};

/// AHB setting
struct cmd_ahb_setting {
	enum drv_direction direct;
	unsigned int reg_addr;
	unsigned int *ptr_user;
	unsigned int val;
	unsigned int delay_us;
};

/// AHB Power setting
struct cmd_ahb_power_setting {
	unsigned int reg_addr; /// reg address
	unsigned int bit_mask; /// bit mask of register data
	unsigned char bit_val; /// bit value of register data
	unsigned int delay_us; /// delay by us
	enum drv_power_type type;
};

/// DMA AHB buffer
struct cmd_dma_reg_buf {
	unsigned int val;
	unsigned int reg_addr_offset;
};

/// Define MCFB trigger register index
enum mcfb_trig {
	MCFB_TRIG_DDR_START = 0,
	MCFB_TRIG_DDR_LEN = 1,
	MCFB_TRIG_EN = 2,
	MCFB_TRIG_MAX,
};

/// DMA AHB setting
struct cmd_dma_reg_setting {
	struct cmd_ahb_setting mcfb[MCFB_TRIG_MAX];
	struct cmd_dma_reg_buf dma_buf[1];
};

/// DMA SRAM setting
struct cmd_dma_sram_setting {
	struct cmd_ahb_setting mcfb[MCFB_TRIG_MAX];
	unsigned int dma_buf[1];
};

/// cmd apply condition
enum cmd_condition {
	CMD_COND_IMMEDIATELY = 0,
	CMD_COND_SOF,
	CMD_COND_SOL,
	CMD_COND_SOL_LOOP,
	CMD_COND_MAX,
};

/// cmd type
enum cmd_type {
	CMD_TYPE_POWERUP = 0,
	CMD_TYPE_PROBE,
	CMD_TYPE_INIT,
	CMD_TYPE_STREAMON,
	CMD_TYPE_POST_STREAMON,
	CMD_TYPE_UPDATE,
	CMD_TYPE_STREAMOFF,
	CMD_TYPE_POWERDOWN,
	CMD_TYPE_MAX,
};

/* type of stream */
typedef enum {
	V4L2_STREAM_TYPE_PREVIEW = 0,
	V4L2_STREAM_TYPE_VIDEO,
	V4L2_STREAM_TYPE_CAPTURE,
	V4L2_STREAM_TYPE_STATIS,
	V4L2_STREAM_TYPE_META,
	V4L2_STREAM_TYPE_MAX
} armcb_v4l2_stream_type_t;

/// cmd shared info
struct cmd_static_info {
	unsigned int camId;
	enum dev_type dev;
	enum hw_bus_type bus;
	enum cmd_condition trigger_cond;
	unsigned int max_cmd_cnt;
	unsigned int max_buf_num;
	unsigned int buf_size;
	char sName[32];
};

/// cmd buf status
enum cmd_status {
	CMD_STATUS_IDEL = 0,
	CMD_STATUS_ACQUIRED,
	CMD_STATUS_SUBMIT,
	CMD_STATUS_APPLYING,
	CMD_STATUS_APPLIED,
	CMD_STATUS_EFFECTIVE,
	CMD_STATUS_OVERDUE,
	CMD_STATUS_FLUSH,
	CMD_STATUS_MAX,
};

/// cmd setting
union cmd_setting {
	struct cmd_i2c_setting i2c[1];
	struct cmd_spi_setting spi[1];
	struct cmd_ahb_setting ahb[1];
	struct cmd_ahb_power_setting ahb_power[1];
	struct cmd_dma_reg_setting dma_reg;
	struct cmd_dma_sram_setting dma_sram;
};

/// cmd private data
struct cmd_priv {
	void **pp_cb;
	void *p_stream_buf;
};

/// Continue command buffer
struct cmd_buf {
	struct cmd_static_info static_info;
	enum cmd_status status;
	struct cmd_priv priv;
	enum cmd_type cmd_type;
	unsigned int order;
	unsigned int apply_frame_id;
	unsigned int effect_frame_id;
	unsigned int cmd_cnt;
	union cmd_setting settings;
};

struct mem_block {
	unsigned char name[10];
	unsigned char is_use_buffer;
	int id;
	unsigned int offset;
	unsigned int len;
	unsigned long phy_addr;
	unsigned long usr_addr;
	void *kernel_addr;
};

struct armcb_isp_stat_isr_info {
	unsigned int stat_type;
	unsigned int sensor_id;
	unsigned int flags;
	unsigned long timestamp;
};

struct armcb_isp_stat_event_status {
	unsigned int frame_number;
	unsigned short config_counter;
	unsigned char buf_err;
};

struct armcb_dma_req {
	unsigned int size;
	unsigned int rmt_addr;
	void *buf;
};

struct armcb_xdma_req {
	unsigned int size;
	unsigned int direction;
	unsigned int rmt_addr;
	void *local_addr;
};

struct perf_bus_params {
	unsigned int reg_addr;
	unsigned int reg_val;
	unsigned int test_times;
	unsigned int rmt_addr;
	unsigned int local_addr;
	unsigned int bytes;
	unsigned int write_cost;
	unsigned int read_cost;
};

struct hw_mem_map_out_params {
	///@TODO
	unsigned long phyAddr;
	int kBufhandle; /// kernel buffer queue index
	unsigned long kvAddr; /// kernel virtual address
	int reserved;
};

struct hw_mem_map_cmd {
	int fd;
	struct hw_mem_map_out_params out;
};

/* custom v4l2 formats */
#define ISP_V4L2_PIX_FMT_RAW10 v4l2_fourcc('R', 'A', 'W', '0') /* RAW10 */
#define ISP_V4L2_PIX_FMT_RAW12 v4l2_fourcc('R', 'A', 'W', '2') /* RAW12 */
#define ISP_V4L2_PIX_FMT_RAW16 v4l2_fourcc('R', 'A', 'W', '6') /* RAW16 */
#define ISP_V4L2_PIX_FMT_META v4l2_fourcc('M', 'E', 'T', 'A') /* META */
#define ISP_V4L2_PIX_FMT_STATIS v4l2_fourcc('S', 'T', 'A', 'T') /* STAT */
#ifndef V4L2_PIX_FMT_X016
#define V4L2_PIX_FMT_X016 \
	v4l2_fourcc('X', '0', '1', '6') /* 32  XY/UV 4:2:0 16-bit */
#endif

struct hw_mem_release_cmd {
	int kBufhandle;
	int reserved;
};

/*
 * Private IOCTLs
 *
 * VIDIOC_ARMCB_ISP_STAT_EN: Enable/disable a statistics module
 * VIDIOC_ARMCB_ISP_STAT_BUF_REQ: Require statistics buffer
 * VIDIOC_ARMCB_ISP_STAT_BUF_QUERY: Query statistics buffer info
 * VIDIOC_ARMCB_ISP_STAT_BUF_DQ: Enqueue the statistics buffer
 * VIDIOC_ARMCB_ISP_STAT_BUF_EQ: Dequeue the statistics buffer
 * VIDIOC_ARMCB_ISP_STAT_STREAM_ON: Start statistics
 * VIDIOC_ARMCB_ISP_STAT_STREAM_OFF: Stop statistics
 */
#define ARMCB_IOCTL_CMD_PRIVATE_START \
	(BASE_VIDIOC_PRIVATE) /* 192-255 are private */

#define ARMCB_VIDIOC_S_REG_LIST (ARMCB_IOCTL_CMD_PRIVATE_START + 0)
#define ARMCB_VIDIOC_G_REG_LIST (ARMCB_IOCTL_CMD_PRIVATE_START + 1)
#define ARMCB_VIDIOC_APPLY_CMD (ARMCB_IOCTL_CMD_PRIVATE_START + 2)

#define VIDIOC_ARMCB_ISP_STAT_EN \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 2, unsigned long)
#define VIDIOC_ARMCB_ISP_STAT_BUF_REQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 3, struct v4l2_requestbuffers)
#define VIDIOC_ARMCB_ISP_STAT_BUF_QUERY \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 4, struct v4l2_buffer)
#define VIDIOC_ARMCB_ISP_STAT_BUF_DQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 5, struct v4l2_buffer)
#define VIDIOC_ARMCB_ISP_STAT_BUF_EQ \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 6, struct v4l2_buffer)
#define VIDIOC_ARMCB_ISP_STAT_STREAM_ON _IOWR('V', BASE_VIDIOC_PRIVATE + 7, int)
#define VIDIOC_ARMCB_ISP_STAT_STREAM_OFF \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 8, int)

#define CMEM_IOCTL_MAGIC 'm'
#define CMEM_ALLOCATE _IOW(CMEM_IOCTL_MAGIC, 1, struct mem_block)
#define CMEM_FREE _IOW(CMEM_IOCTL_MAGIC, 2, struct mem_block)
#define ARMCB_VIDIOC_ISP_XDMA _IOWR(CMEM_IOCTL_MAGIC, 3, struct armcb_xdma_req)
#define CAM_HW_BUFFER_MAP _IOWR(CMEM_IOCTL_MAGIC, 4, struct hw_mem_map_cmd)
#define CAM_HW_BUFFER_RELEASE \
	_IOWR(CMEM_IOCTL_MAGIC, 5, struct hw_mem_release_cmd)
#define CMEM_CMA_IMPORT _IOWR(CMEM_IOCTL_MAGIC, 6, struct mem_block)

#define ARMCB_VIDIOC_ISP_DMA_COPY 194
#define ARMCB_VIDIOC_SYS_BUS_TEST 196
#define ARMCB_VIDIOC_ISP_XDMA_TEST 198

#define VIDIOC_S_PLATFORM_HW_INIT 0x8000
#define VIDIOC_S_PLATFORM_HW_UNINIT 0x8001

#define VIN_BUFFER_LONG_ADDR 0x80000000
#define VIN_BUFFER_MID_ADDR 0xA0000000
#define VIN_BUFFER_SHORT_ADDR 0xC0000000
#define VIN_BUFFER_VSHORT_ADDR 0xE0000000

///#define QEMU_ON_VEXPRESS
///#define ENABLE_RUNTIME_UPDATE_STATS_ADDR

#endif

/*
 * lt7911uxc.h - Lontium DP-CSI bridge driver
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */
#ifndef __LT7911UXC_H__
#define __LT7911UXC_H__

#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/module.h>

#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>

#include <media/v4l2-event.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-fwnode.h>

/* ----------------------- Structure ----------------------- */

/* LT7911UXC GPIO ID */
enum lt7911uxc_gpio_id {
	RSET,
	POWERH,
	POWERL,
	GPIO_NUM,
};

/* LT7911UXC PAD ID */
enum lt7911uxc_pad_id {
	LT7911UXC_SD_PAD_SOURCE_CSI,
	LT7911UXC_SD_PAD_MAX_NUM,
};

/* LT7911UXC GPIO Structure */
struct lt7911uxc_gpio {
	int gpio;
	int level;
};

/* LT7911UXC Platform Data */
struct lt7911uxc_platform_data {
	const struct firmware *fwbin;
	struct lt7911uxc_gpio gpio[GPIO_NUM];
};

struct lt7911uxc_intf_state {
	bool video_in;
	bool audio_in;
};

struct lt7911uxc_external_state {
	bool enable_i2c;
	bool enable_mipi;
};

struct lt7911uxc_video_state {
	int fps;
	u16 width;
	u16 height;
	int pixelfmt;
};

struct lt7911uxc_audio_state {
	int fps;
	u16 audiofs;
};

struct lt7911uxc_mipi_state {
	int portnum;
	u32 clkrate;
	u32 datarate;
};

struct lt7911uxc_fw_state {
	int chipid;
	int chipver;
	char *chipname;
};

/* LT7911UXC Device Status */
struct lt7911uxc_state {
	struct lt7911uxc_intf_state intf;
	struct lt7911uxc_external_state external;
	struct lt7911uxc_video_state video;
	struct lt7911uxc_audio_state audio;
	struct lt7911uxc_mipi_state mipi;
	struct lt7911uxc_fw_state fw;
};

struct lt7911uxc_mode {
	u32 width;
	u32 height;
	struct v4l2_fract max_fps;
	u32 hts_def;
	u32 vts_def;
	u32 exp_def;
	u32 mipi_freq_idx;
};

static const struct lt7911uxc_mode supported_modes_dphy[] = {
#ifdef SKY1_SOC
	{
		.width = 3840,
		.height = 2160,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 4400,
		.vts_def = 2250,
		.mipi_freq_idx = 0,
	}, {
		.width = 1920,
		.height = 1080,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 2200,
		.vts_def = 1125,
		.mipi_freq_idx = 0,
	}, {
		.width = 1280,
		.height = 720,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 1650,
		.vts_def = 750,
		.mipi_freq_idx = 0,
	}, {
		.width = 720,
		.height = 576,
		.max_fps = {
			.numerator = 10000,
			.denominator = 500000,
		},
		.hts_def = 864,
		.vts_def = 625,
		.mipi_freq_idx = 1,
	}, {
		.width = 720,
		.height = 480,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 858,
		.vts_def = 525,
		.mipi_freq_idx = 1,
	},
#else
	{
		.width = 640,
		.height = 480,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 864,
		.vts_def = 625,
		.mipi_freq_idx = 1,
	},
#endif
};
/* LT7911UXC Device Structure */
struct lt7911uxc {
	struct mutex lock;
	struct media_pad pad[LT7911UXC_SD_PAD_MAX_NUM];
	struct v4l2_fwnode_endpoint ep;
	const struct lt7911uxc_mode *cur_mode;
	const struct lt7911uxc_mode *support_modes;
	u32 cfg_num;
	struct v4l2_subdev sd;
	struct i2c_client *i2c_client;

	struct lt7911uxc_state *priv;
	struct lt7911uxc_platform_data *pdata;
};

/* LT7911UXC Register Table */
typedef struct _lt7911uxc_reg {
	u8 slave;
	u8 address;
	u8 value;
	u8 rw;
} lt7911uxc_reg;

/* ----------------------- Register Table ----------------------- */

/* Enabel I2C */
lt7911uxc_reg lt7911uxc_EnableI2CTable[] =
{
	{0x86, 0xff, 0xe0, 0x00},
	{0x86, 0xee, 0x01, 0x00},
	{0x00, 0x00, 0x00, 0x00},
};

/* Disable I2C */
lt7911uxc_reg lt7911uxc_DisableI2CTable[] =
{
	{0x86, 0xff, 0xe0, 0x00},
	{0x86, 0xee, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00},
};

/* Enabel MIPI */
lt7911uxc_reg lt7911uxc_EnableMIPITable[] =
{
	{0x86, 0xff, 0xe0, 0x00},
	{0x86, 0xb0, 0x01, 0x00},
	{0x00, 0x00, 0x00, 0x00},
};

/* Disable MIPI */
lt7911uxc_reg lt7911uxc_DisableMIPITable[] =
{
	{0x86, 0xff, 0xe0, 0x00},
	{0x86, 0xb0, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00},
};

/* Chip ID */
lt7911uxc_reg lt7911uxc_CheckIDTable[] =
{
	{0x86, 0xff, 0xe1, 0x00},
	{0x86, 0x00, 0x02, 0xFF},
	{0x00, 0x00, 0x00, 0x00},
};

/* Configure Parameters */
lt7911uxc_reg lt7911uxc_ConfigureParaTable[] =
{
	{0x86, 0xff, 0xe0, 0x00},
	{0x86, 0xee, 0x01, 0x00},
	{0x86, 0x5e, 0xc1, 0x00},
	{0x86, 0x58, 0x00, 0x00},
	{0x86, 0x59, 0x50, 0x00},
	{0x86, 0x5a, 0x10, 0x00},
	{0x86, 0x5a, 0x00, 0x00},
	{0x86, 0x58, 0x21, 0x00},
	{0x00, 0x00, 0x00, 0x00},
};

/* WREN */
lt7911uxc_reg lt7911uxc_WrenTable[] =
{
	{0x86, 0xff, 0xe1, 0x00},
	{0x86, 0x03, 0x2e, 0x00},
	{0x86, 0x03, 0xee, 0x00},
	{0x86, 0xff, 0xe0, 0x00},
	{0x86, 0x5a, 0x04, 0x00},
	{0x86, 0x5a, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00},
};

/* I2C to FIFO */
lt7911uxc_reg lt7911uxc_I2cData2FifoTable[] =
{
	{0x86, 0x5e, 0xdf, 0x00},
	{0x86, 0x5a, 0x20, 0x00},
	{0x86, 0x5a, 0x00, 0x00},
	{0x86, 0x58, 0x21, 0x00},
	{0x00, 0x00, 0x00, 0x00},
};

/* FIFO to I2C */
lt7911uxc_reg lt7911uxc_Flash2I2cTable[] =
{
	{0x86, 0x58, 0x21, 0x00},
	{0x86, 0x5f, 0x20, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* WRDI */
lt7911uxc_reg lt7911uxc_WrdiTable[] =
{
	{0x86, 0x5a, 0x08, 0x00},
	{0x86, 0x5a, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00},
};

/* Log */
lt7911uxc_reg lt7911uxc_LogTable[] =
{
	{0x56, 0xff, 0xA0, 0x00},
	{0x00, 0x00, 0x00, 0x00},
};

/* IRQ */
lt7911uxc_reg lt7911uxc_IRQTable[] =
{
	{0x86, 0x84, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* Pixel Clock */
lt7911uxc_reg lt7911uxc_PixClkTable[] =
{
	{0x86, 0x85, 0x01, 0xff},
	{0x86, 0x86, 0x01, 0xff},
	{0x86, 0x87, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* H Total */
lt7911uxc_reg lt7911uxc_HTotalTable[] =
{
	{0x86, 0x88, 0x01, 0xff},
	{0x86, 0x89, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* V Total */
lt7911uxc_reg lt7911uxc_VTotalTable[] =
{
	{0x86, 0x8A, 0x01, 0xff},
	{0x86, 0x8B, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* H Active */
lt7911uxc_reg lt7911uxc_HActiveTable[] =
{
	{0x86, 0x8C, 0x01, 0xff},
	{0x86, 0x8D, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* V Active */
lt7911uxc_reg lt7911uxc_VActiveTable[] =
{
	{0x86, 0x8E, 0x01, 0xff},
	{0x86, 0x8F, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* Audio FS */
lt7911uxc_reg lt7911uxc_AudioFSTable[] =
{
	{0x86, 0x90, 0x01, 0xff},
	{0x86, 0x91, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* Byte Clock */
lt7911uxc_reg lt7911uxc_ByteClkTable[] =
{
	{0x86, 0x92, 0x01, 0xff},
	{0x86, 0x93, 0x01, 0xff},
	{0x86, 0x94, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* Port Number */
lt7911uxc_reg lt7911uxc_PortNumTable[] =
{
	{0x86, 0xA0, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* MIPI Format */
lt7911uxc_reg lt7911uxc_MIPIFmtTable[] =
{
	{0x86, 0xA1, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* Chip ID */
lt7911uxc_reg lt7911uxc_ChipIDTable[] =
{
	{0x86, 0x00, 0x01, 0xff},
	{0x86, 0x01, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* Chip Name */
lt7911uxc_reg lt7911uxc_ChipNameTable[] =
{
	{0x86, 0x80, 0x01, 0xff},
	{0x86, 0x81, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};

/* Chip Version */
lt7911uxc_reg lt7911uxc_ChipVersionTable[] =
{
	{0x86, 0x82, 0x01, 0xff},
	{0x86, 0x83, 0x01, 0xff},
	{0x00, 0x00, 0x00, 0x00},
};
#endif  /* __LT7911UXC_H__ */
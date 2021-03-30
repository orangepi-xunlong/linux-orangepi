/*
 * A V4L2 driver for GalaxyCore gc2155 cameras.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>
#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for GalaxyCore gc2155 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_FALLING
#define V4L2_IDENT_SENSOR 0x2155

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 8
/*
 * The gc2155 sits on i2c with ID 0x78
 */
#define I2C_ADDR   0x78
#define SENSOR_NAME "gc2155"

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;	/* coming later */

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}

/*
 * The default register settings
 *
 */

static struct regval_list sensor_default_regs[] = {

	{0xfe, 0xf0},
	{0xfe, 0xf0},
	{0xfe, 0xf0},
	{0xfc, 0x06},
	{0xf6, 0x00},
	{0xf7, 0x1d},
	{0xf8, 0x84},
	{0xfa, 0x00},
	{0xf9, 0xfe},
	{0xf2, 0x00},
	{0xfe, 0x00},
	{0x03, 0x04},
	{0x04, 0xe2},
	{0x09, 0x00},
	{0x0a, 0x00},
	{0x0b, 0x00},
	{0x0c, 0x00},
	{0x0d, 0x04},
	{0x0e, 0xc0},
	{0x0f, 0x06},
	{0x10, 0x50},
	{0x12, 0x2e},
	{0x17, 0x14},
	{0x18, 0x02},
	{0x19, 0x0e},
	{0x1a, 0x01},
	{0x1b, 0x4b},
	{0x1c, 0x07},
	{0x1d, 0x10},
	{0x1e, 0x98},
	{0x1f, 0x78},
	{0x20, 0x05},
	{0x21, 0x40},
	{0x22, 0xf0},
	{0x24, 0x16},
	{0x25, 0x01},
	{0x26, 0x10},
	{0x2d, 0x40},
	{0x30, 0x01},
	{0x31, 0x90},
	{0x33, 0x04},
	{0x34, 0x01},
	{0xfe, 0x00},
	{0x80, 0xff},
	{0x81, 0x2c},
	{0x82, 0xfa},
	{0x83, 0x00},
	{0x84, 0x02},
	{0x85, 0x08},
	{0x86, 0x07},
	{0x89, 0x03},
	{0x8a, 0x00},
	{0x8b, 0x00},
	{0xb0, 0x55},
	{0xc3, 0x11},
	{0xc4, 0x20},
	{0xc5, 0x30},
	{0xc6, 0x38},
	{0xc7, 0x40},
	{0xec, 0x02},
	{0xed, 0x04},
	{0xee, 0x60},
	{0xef, 0x90},
	{0xb6, 0x01},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x04},
	{0x96, 0xb0},
	{0x97, 0x06},
	{0x98, 0x40},
	{0xfe, 0x00},
	{0x18, 0x02},
	{0x40, 0x42},
	{0x41, 0x00},
	{0x43, 0x54},
	{0x5e, 0x00},
	{0x5f, 0x00},
	{0x60, 0x00},
	{0x61, 0x00},
	{0x62, 0x00},
	{0x63, 0x00},
	{0x64, 0x00},
	{0x65, 0x00},
	{0x66, 0x20},
	{0x67, 0x20},
	{0x68, 0x20},
	{0x69, 0x20},
	{0x6a, 0x08},
	{0x6b, 0x08},
	{0x6c, 0x08},
	{0x6d, 0x08},
	{0x6e, 0x08},
	{0x6f, 0x08},
	{0x70, 0x08},
	{0x71, 0x08},
	{0x72, 0xf0},
	{0x7e, 0x3c},
	{0x7f, 0x00},
	{0xfe, 0x00},
	{0xfe, 0x01},
	{0x01, 0x08},
	{0x02, 0xc0},
	{0x03, 0x04},
	{0x04, 0x90},
	{0x05, 0x30},
	{0x06, 0x98},
	{0x07, 0x28},
	{0x08, 0x6c},
	{0x09, 0x00},
	{0x0a, 0xc2},
	{0x0b, 0x11},
	{0x0c, 0x10},
	{0x13, 0x2d},
	{0x17, 0x00},
	{0x1c, 0x11},
	{0x1e, 0x61},
	{0x1f, 0x30},
	{0x20, 0x40},
	{0x22, 0x80},
	{0x23, 0x20},

	{0x12, 0x35},
	{0x15, 0x50},
	{0x10, 0x31},
	{0x3e, 0x28},
	{0x3f, 0xe0},
	{0x40, 0xe0},
	{0x41, 0x08},

	{0xfe, 0x02},
	{0x0f, 0x05},
	{0xfe, 0x02},
	{0x90, 0x6c},
	{0x91, 0x03},
	{0x92, 0xc4},
	{0x97, 0x64},
	{0x98, 0x88},
	{0x9d, 0x08},
	{0xa2, 0x11},
	{0xfe, 0x00},
	{0xfe, 0x02},
	{0x80, 0xc1},
	{0x81, 0x08},
	{0x82, 0x10},
	{0x83, 0x30},
	{0x84, 0x0a},
	{0x86, 0x80},
	{0x87, 0x30},
	{0x88, 0x25},
	{0x89, 0x80},
	{0x8a, 0x60},
	{0x8b, 0x30},
	{0xfe, 0x01},
	{0x21, 0x14},
	{0xfe, 0x02},
	{0x3c, 0x06},
	{0x3d, 0x40},
	{0x48, 0x30},
	{0x49, 0x06},
	{0x4b, 0x08},
	{0x4c, 0x20},
	{0xa3, 0x50},
	{0xa4, 0x30},
	{0xa5, 0x40},
	{0xa6, 0x80},
	{0xab, 0x40},
	{0xae, 0x0c},
	{0xb3, 0x42},
	{0xb4, 0x24},
	{0xb6, 0x50},
	{0xb7, 0x01},
	{0xb9, 0x28},
	{0xfe, 0x00},
	{0xfe, 0x02},
	{0x10, 0x0d},
	{0x11, 0x12},
	{0x12, 0x17},
	{0x13, 0x1c},
	{0x14, 0x27},
	{0x15, 0x34},
	{0x16, 0x44},
	{0x17, 0x55},
	{0x18, 0x6e},
	{0x19, 0x81},
	{0x1a, 0x91},
	{0x1b, 0x9c},
	{0x1c, 0xaa},
	{0x1d, 0xbb},
	{0x1e, 0xca},
	{0x1f, 0xd5},
	{0x20, 0xe0},
	{0x21, 0xe7},
	{0x22, 0xed},
	{0x23, 0xf6},
	{0x24, 0xfb},
	{0x25, 0xff},
	{0xfe, 0x02},
	{0x26, 0x0d},
	{0x27, 0x12},
	{0x28, 0x17},
	{0x29, 0x1c},
	{0x2a, 0x27},
	{0x2b, 0x34},
	{0x2c, 0x44},
	{0x2d, 0x55},
	{0x2e, 0x6e},
	{0x2f, 0x81},
	{0x30, 0x91},
	{0x31, 0x9c},
	{0x32, 0xaa},
	{0x33, 0xbb},
	{0x34, 0xca},
	{0x35, 0xd5},
	{0x36, 0xe0},
	{0x37, 0xe7},
	{0x38, 0xed},
	{0x39, 0xf6},
	{0x3a, 0xfb},
	{0x3b, 0xff},
	{0xfe, 0x02},
	{0xd1, 0x20},
	{0xd2, 0x20},
	{0xd3, 0x38},
	{0xdd, 0x14},
	{0xde, 0x88},
	{0xed, 0x80},
	{0xfe, 0x01},
	{0xc2, 0x1f},
	{0xc3, 0x13},
	{0xc4, 0x0e},
	{0xc8, 0x16},
	{0xc9, 0x0f},
	{0xca, 0x0c},
	{0xbc, 0x52},
	{0xbd, 0x2c},
	{0xbe, 0x27},
	{0xb6, 0x47},
	{0xb7, 0x32},
	{0xb8, 0x30},
	{0xc5, 0x00},
	{0xc6, 0x00},
	{0xc7, 0x00},
	{0xcb, 0x00},
	{0xcc, 0x00},
	{0xcd, 0x00},
	{0xbf, 0x0e},
	{0xc0, 0x00},
	{0xc1, 0x00},
	{0xb9, 0x08},
	{0xba, 0x00},
	{0xbb, 0x00},
	{0xaa, 0x0a},
	{0xab, 0x0c},
	{0xac, 0x0d},
	{0xad, 0x02},
	{0xae, 0x06},
	{0xaf, 0x05},
	{0xb0, 0x00},
	{0xb1, 0x05},
	{0xb2, 0x02},
	{0xb3, 0x04},
	{0xb4, 0x04},
	{0xb5, 0x05},
	{0xd0, 0x00},
	{0xd1, 0x00},
	{0xd2, 0x00},
	{0xd6, 0x02},
	{0xd7, 0x00},
	{0xd8, 0x00},
	{0xd9, 0x00},
	{0xda, 0x00},
	{0xdb, 0x00},
	{0xd3, 0x00},
	{0xd4, 0x00},
	{0xd5, 0x00},
	{0xa4, 0x04},
	{0xa5, 0x00},
	{0xa6, 0x77},
	{0xa7, 0x77},
	{0xa8, 0x77},
	{0xa9, 0x77},
	{0xa1, 0x80},
	{0xa2, 0x80},

	{0xfe, 0x01},
	{0xdc, 0x35},
	{0xdd, 0x28},
	{0xdf, 0x0d},
	{0xe0, 0x70},
	{0xe1, 0x78},
	{0xe2, 0x70},
	{0xe3, 0x78},
	{0xe6, 0x90},
	{0xe7, 0x70},
	{0xe8, 0x90},
	{0xe9, 0x70},
	{0xfe, 0x00},
	{0xfe, 0x01},
	{0x4f, 0x00},
	{0x4f, 0x00},
	{0x4b, 0x01},
	{0x4f, 0x00},
	{0x4c, 0x01},
	{0x4d, 0x71},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x91},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x50},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x70},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x90},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xb0},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xd0},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x4f},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x6f},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x8f},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xaf},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xcf},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x6e},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8e},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xae},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xce},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x4d},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x6d},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8d},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xad},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xcd},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x4c},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x6c},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8c},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xac},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xcc},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xec},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x4b},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x6b},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8b},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xab},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8a},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xaa},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xca},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xa9},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xc9},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xcb},
	{0x4e, 0x05},
	{0x4c, 0x01},
	{0x4d, 0xeb},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x0b},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x2b},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x4b},
	{0x4e, 0x05},
	{0x4c, 0x01},
	{0x4d, 0xea},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x0a},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x2a},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x6a},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x29},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x49},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x69},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x89},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0xa9},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0xc9},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x48},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x68},
	{0x4e, 0x06},
	{0x4c, 0x03},
	{0x4d, 0x09},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xa8},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xc8},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xe8},
	{0x4e, 0x07},
	{0x4c, 0x03},
	{0x4d, 0x08},
	{0x4e, 0x07},
	{0x4c, 0x03},
	{0x4d, 0x28},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0x87},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xa7},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xc7},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xe7},
	{0x4e, 0x07},
	{0x4c, 0x03},
	{0x4d, 0x07},
	{0x4e, 0x07},
	{0x4f, 0x01},
	{0xfe, 0x01},

	{0x50, 0x80},
	{0x51, 0xa8},
	{0x52, 0x57},
	{0x53, 0x38},
	{0x54, 0xc7},
	{0x56, 0x0e},
	{0x58, 0x08},
	{0x5b, 0x00},
	{0x5c, 0x74},
	{0x5d, 0x8b},
	{0x61, 0xd3},
	{0x62, 0x90},
	{0x63, 0xaa},
	{0x65, 0x04},
	{0x67, 0xb2},
	{0x68, 0xac},
	{0x69, 0x00},
	{0x6a, 0xb2},
	{0x6b, 0xac},
	{0x6c, 0xdc},
	{0x6d, 0xb0},
	{0x6e, 0x30},
	{0x6f, 0x40},
	{0x70, 0x05},
	{0x71, 0x80},
	{0x72, 0x80},
	{0x73, 0x30},
	{0x74, 0x01},
	{0x75, 0x01},
	{0x7f, 0x08},
	{0x76, 0x70},
	{0x77, 0x48},
	{0x78, 0xa0},
	{0xfe, 0x00},

	{0xfe, 0x02},
	{0xc0, 0x01},
	{0xc1, 0x4a},
	{0xc2, 0xf3},
	{0xc3, 0xfc},
	{0xc4, 0xe4},
	{0xc5, 0x48},
	{0xc6, 0xec},
	{0xc7, 0x45},
	{0xc8, 0xf8},
	{0xc9, 0x02},
	{0xca, 0xfe},
	{0xcb, 0x42},
	{0xcc, 0x00},
	{0xcd, 0x45},
	{0xce, 0xf0},
	{0xcf, 0x00},
	{0xe3, 0xf0},
	{0xe4, 0x45},
	{0xe5, 0xe8},
	{0xfe, 0x01},
	{0x9f, 0x42},
	{0xfe, 0x00},

	{0xfe, 0x00},
	{0x05, 0x02},
	{0x06, 0x2d},
	{0x07, 0x00},
	{0x08, 0xa0},
	{0xfe, 0x01},
	{0x25, 0x00},
	{0x26, 0xd4},
	{0x27, 0x04},
	{0x28, 0xf8},
	{0x29, 0x07},
	{0x2a, 0x74},
	{0x2b, 0x08},
	{0x2c, 0x48},
	{0x2d, 0x0f},
	{0x2e, 0xbc},
	{0xfe, 0x00},

	{0xfe, 0x00},
	{0xfa, 0x00},
	{0xfd, 0x01},
	{0xfe, 0x00},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x02},
	{0x96, 0x58},
	{0x97, 0x03},
	{0x98, 0x20},
	{0x99, 0x11},
	{0x9a, 0x06},
	{0xfe, 0x00},
	{0xec, 0x01},
	{0xed, 0x02},
	{0xee, 0x30},
	{0xef, 0x48},
	{0xfe, 0x01},
	{0x74, 0x00},
	{0xfe, 0x01},
	{0x01, 0x04},
	{0x02, 0x60},
	{0x03, 0x02},
	{0x04, 0x48},
	{0x05, 0x18},
	{0x06, 0x4c},
	{0x07, 0x14},
	{0x08, 0x36},
	{0x0a, 0xc0},
	{0x21, 0x14},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xc3, 0x11},
	{0xc4, 0x20},
	{0xc5, 0x30},
	{0xfe, 0x00},
	{0xfe, 0x00},
	{0xf2, 0x0f},
};

/* 1600X1200 UXGA capture */
static struct regval_list sensor_uxga_regs[] = {
	{0xfe, 0x00},
	{0xb6, 0x00},
	{0xfa, 0x11},
	{0xfd, 0x00},
	{0xfe, 0x00},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x04},
	{0x96, 0xb0},
	{0x97, 0x06},
	{0x98, 0x40},
	{0x99, 0x11},
	{0x9a, 0x06},

	{0x9b, 0x00},
	{0x9c, 0x00},
	{0x9d, 0x00},
	{0x9e, 0x00},
	{0x9f, 0x00},
	{0xa0, 0x00},
	{0xa1, 0x00},
	{0xa2, 0x00},

	{0xfe, 0x00},
	{0xec, 0x02},
	{0xed, 0x04},
	{0xee, 0x60},
	{0xef, 0x90},
	{0xfe, 0x01},
	{0x74, 0x01},
	{0xfe, 0x01},
	{0x01, 0x08},
	{0x02, 0xc0},
	{0x03, 0x04},
	{0x04, 0x90},
	{0x05, 0x30},
	{0x06, 0x98},
	{0x07, 0x28},
	{0x08, 0x6c},
	{0x0a, 0xc2},
	{0x21, 0x15},
	{0xfe, 0x00},

	{0xfe, 0x00},
	{0xc3, 0x00},
	{0xc4, 0x90},
	{0xc5, 0x98},
	{0xfe, 0x00},
};

/* 800X600 SVGA,30fps*/
static struct regval_list sensor_svga_regs[] = {
	{0xfe, 0x00},
	{0xb6, 0x01},
	{0xfa, 0x00},
	{0xfd, 0x01},

	{0x09, 0x00},
	{0x0a, 0x00},
	{0x0b, 0x00},
	{0x0c, 0x00},
	{0x0d, 0x04},
	{0x0e, 0xc0},
	{0x0f, 0x06},
	{0x10, 0x50},

	{0xfe, 0x00},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x02},
	{0x96, 0x58},
	{0x97, 0x03},
	{0x98, 0x20},
	{0x99, 0x11},
	{0x9a, 0x06},
	{0x9b, 0x00},
	{0x9c, 0x00},
	{0x9d, 0x00},
	{0x9e, 0x00},
	{0x9f, 0x00},
	{0xa0, 0x00},
	{0xa1, 0x00},
	{0xa2, 0x00},


	{0xfe, 0x00},
	{0xec, 0x01},
	{0xed, 0x02},
	{0xee, 0x30},
	{0xef, 0x48},
	{0xfe, 0x01},
	{0x74, 0x00},

	{0xfe, 0x01},
	{0x01, 0x04},
	{0x02, 0x60},
	{0x03, 0x02},
	{0x04, 0x48},
	{0x05, 0x18},
	{0x06, 0x4c},
	{0x07, 0x14},
	{0x08, 0x36},
	{0x0a, 0xc0},
	{0x21, 0x14},
	{0xfe, 0x00},

	{0xfe, 0x00},
	{0xc3, 0x11},
	{0xc4, 0x20},
	{0xc5, 0x30},
	{0xfe, 0x00},


	{0xfe, 0x00},
	{0x05, 0x02},
	{0x06, 0x2d},
	{0x07, 0x00},
	{0x08, 0xa0},
	{0xfe, 0x01},
	{0x25, 0x00},
	{0x26, 0xd4},
	{0x27, 0x04},
	{0x28, 0xf8},
	{0x29, 0x07},
	{0x2a, 0x74},
	{0x2b, 0x08},
	{0x2c, 0x48},
	{0x2d, 0x0f},
	{0x2e, 0xbc},
	{0xfe, 0x00},
};

static struct regval_list gc2155_hd720_regs[] = {
#if defined(GC2155_SUBSAMPLE)	/*subsample for 720P*/
	{0xfe, 0x00},
	{0xb6, 0x01},
	{0xfd, 0x00},
	{0xfe, 0x00},
	{0x99, 0x55},
	{0x9a, 0x06},
	{0x9b, 0x00},
	{0x9c, 0x00},
	{0x9d, 0x01},
	{0x9e, 0x23},
	{0x9f, 0x00},
	{0xa0, 0x00},
	{0xa1, 0x01},
	{0xa2, 0x23},

	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x78},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x02},
	{0x96, 0xd0},
	{0x97, 0x05},
	{0x98, 0x00},


	{0xfe, 0x00},
	{0xec, 0x02},
	{0xed, 0x04},
	{0xee, 0x60},
	{0xef, 0x90},
	{0xfe, 0x01},
	{0x74, 0x01},

	{0xfe, 0x01},
	{0x01, 0x08},
	{0x02, 0xc0},
	{0x03, 0x04},
	{0x04, 0x90},
	{0x05, 0x30},
	{0x06, 0x98},
	{0x07, 0x28},
	{0x08, 0x6c},
	{0x0a, 0xc2},
	{0x21, 0x15},
	{0xfe, 0x00},

	{0xfe, 0x00},
	{0x03, 0x03},
	{0x04, 0xe8},
	{0x05, 0x01},
	{0x06, 0x56},
	{0x07, 0x00},
	{0x08, 0x32},
	{0xfe, 0x01},
	{0x25, 0x00},
	{0x26, 0xfa},
	{0x27, 0x04},
	{0x28, 0xe2},
	{0x29, 0x04},
	{0x2a, 0xe2},
	{0x2b, 0x04},
	{0x2c, 0xe2},
	{0x2d, 0x04},
	{0x2e, 0xe2},
	{0x3c, 0x00},

	{0xfe, 0x00},
#else
	{0xfe, 0x00},
	{0xb6, 0x01},
	{0xfd, 0x00},

	{0xfe, 0x00},
	{0x09, 0x00},
	{0x0a, 0xf0},		/*row start  (1280-720)/2*/
	{0x0b, 0x00},
	{0x0c, 0xa0},		/*col start (1600-1280)/2*/
	{0x0d, 0x02},
	{0x0e, 0xe0},
	{0x0f, 0x05},
	{0x10, 0x10},

	{0x99, 0x11},
	{0x9a, 0x06},
	{0x9b, 0x00},
	{0x9c, 0x00},
	{0x9d, 0x00},
	{0x9e, 0x00},
	{0x9f, 0x00},
	{0xa0, 0x00},
	{0xa1, 0x00},
	{0xa2, 0x00},

	{0x90, 0x01},
	{0x95, 0x02},
	{0x96, 0xd0},
	{0x97, 0x05},
	{0x98, 0x00},

	{0xfe, 0x00},
	{0xec, 0x04},
	{0xed, 0x02},
	{0xee, 0x50},
	{0xef, 0x5a},
	{0xfe, 0x01},
	{0x74, 0x01},
	{0xfe, 0x01},
	{0x01, 0x04},
	{0x02, 0x50},
	{0x03, 0x02},
	{0x04, 0x58},
	{0x05, 0x20},
	{0x06, 0x30},
	{0x07, 0x20},
	{0x08, 0x40},
	{0x09, 0x80},
	{0x0a, 0xc2},
	{0x21, 0x15},
	{0xfe, 0x00},

	/******720P high Frame Rate 16fps~32fps,start set***********************/
	{0xfe, 0x00},
	{0x03, 0x02},
	{0x04, 0x34},
	{0x05, 0x01},
	{0x06, 0x6e},
	{0x07, 0x00},
	{0x08, 0x80},
	{0xfe, 0x01},
	{0x25, 0x01},
	{0x26, 0x1a},

	{0x27, 0x03},
	{0x28, 0x4e},

	{0x29, 0x04},
	{0x2a, 0x68},

	{0x2b, 0x05},
	{0x2c, 0x82},

	{0x2d, 0x06},
	{0x2e, 0x9c},
	{0x3c, 0x00},
	{0xfe, 0x00},
	/******720P high Frame Rate 16fps~32fps,end set*************************/
#endif
};

/*
 * The white balance settings
 * Here only tune the R G B channel gain.
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
static struct regval_list sensor_wb_manual[] = {
};

static struct regval_list sensor_wb_auto_regs[] = {
	{0xfe, 0x00},
	{0x82, 0xfe},
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	{0xfe, 0x00},
	{0x82, 0xfc},
	{0xb3, 0x50},
	{0xb4, 0x40},
	{0xb5, 0xa8},
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	{0xfe, 0x00},
	{0x82, 0xfc},
	{0xb3, 0x72},
	{0xb4, 0x40},
	{0xb5, 0x5B},
};

static struct regval_list sensor_wb_tungsten_regs[] = {
	{0xfe, 0x00},
	{0x82, 0xfc},
	{0xb3, 0xa0},
	{0xb4, 0x45},
	{0xb5, 0x40},
};

static struct regval_list sensor_wb_horizon[] = {

};
static struct regval_list sensor_wb_daylight_regs[] = {
	{0xfe, 0x00},
	{0x82, 0xfc},
	{0xb3, 0x70},
	{0xb4, 0x40},
	{0xb5, 0x50},
};

static struct regval_list sensor_wb_flash[] = {

};

static struct regval_list sensor_wb_cloud_regs[] = {
	{0xfe, 0x00},
	{0x82, 0xfc},
	{0xb3, 0x58},
	{0xb4, 0x40},
	{0xb5, 0x50},
};

static struct regval_list sensor_wb_shade[] = {
};

static struct cfg_array sensor_wb[] = {
	{
	 .regs = sensor_wb_manual,
	 .size = ARRAY_SIZE(sensor_wb_manual),
	 },
	{
	 .regs = sensor_wb_auto_regs,
	 .size = ARRAY_SIZE(sensor_wb_auto_regs),
	 },
	{
	 .regs = sensor_wb_incandescence_regs,
	 .size = ARRAY_SIZE(sensor_wb_incandescence_regs),
	 },
	{
	 .regs = sensor_wb_fluorescent_regs,
	 .size = ARRAY_SIZE(sensor_wb_fluorescent_regs),
	 },
	{
	 .regs = sensor_wb_tungsten_regs,
	 .size = ARRAY_SIZE(sensor_wb_tungsten_regs),
	 },
	{
	 .regs = sensor_wb_horizon,
	 .size = ARRAY_SIZE(sensor_wb_horizon),
	 },
	{
	 .regs = sensor_wb_daylight_regs,
	 .size = ARRAY_SIZE(sensor_wb_daylight_regs),
	 },
	{
	 .regs = sensor_wb_flash,
	 .size = ARRAY_SIZE(sensor_wb_flash),
	 },
	{
	 .regs = sensor_wb_cloud_regs,
	 .size = ARRAY_SIZE(sensor_wb_cloud_regs),
	 },
	{
	 .regs = sensor_wb_shade,
	 .size = ARRAY_SIZE(sensor_wb_shade),
	 },
};

/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
	{0xfe, 0x00},
	{0x83, 0xe0},
};

static struct regval_list sensor_colorfx_bw_regs[] = {

};

static struct regval_list sensor_colorfx_sepia_regs[] = {
	{0xfe, 0x00},
	{0x83, 0x82},
};

static struct regval_list sensor_colorfx_negative_regs[] = {
	{0xfe, 0x00},
	{0x83, 0x01},
};

static struct regval_list sensor_colorfx_emboss_regs[] = {
	{0xfe, 0x00},
	{0x83, 0x12},
};

static struct regval_list sensor_colorfx_sketch_regs[] = {

};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
	{0xfe, 0x00},
	{0x83, 0x62},
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
	{0xfe, 0x00},
	{0x83, 0x52},
};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
};

static struct regval_list sensor_colorfx_aqua_regs[] = {
};

static struct regval_list sensor_colorfx_art_freeze_regs[] = {
};

static struct regval_list sensor_colorfx_silhouette_regs[] = {
};

static struct regval_list sensor_colorfx_solarization_regs[] = {
};

static struct regval_list sensor_colorfx_antique_regs[] = {
};

static struct regval_list sensor_colorfx_set_cbcr_regs[] = {
};

static struct cfg_array sensor_colorfx[] = {
	{
	 .regs = sensor_colorfx_none_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_none_regs),
	 },
	{
	 .regs = sensor_colorfx_bw_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_bw_regs),
	 },
	{
	 .regs = sensor_colorfx_sepia_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_sepia_regs),
	 },
	{
	 .regs = sensor_colorfx_negative_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_negative_regs),
	 },
	{
	 .regs = sensor_colorfx_emboss_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_emboss_regs),
	 },
	{
	 .regs = sensor_colorfx_sketch_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_sketch_regs),
	 },
	{
	 .regs = sensor_colorfx_sky_blue_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_sky_blue_regs),
	 },
	{
	 .regs = sensor_colorfx_grass_green_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_grass_green_regs),
	 },
	{
	 .regs = sensor_colorfx_skin_whiten_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_skin_whiten_regs),
	 },
	{
	 .regs = sensor_colorfx_vivid_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_vivid_regs),
	 },
	{
	 .regs = sensor_colorfx_aqua_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_aqua_regs),
	 },
	{
	 .regs = sensor_colorfx_art_freeze_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_art_freeze_regs),
	 },
	{
	 .regs = sensor_colorfx_silhouette_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_silhouette_regs),
	 },
	{
	 .regs = sensor_colorfx_solarization_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_solarization_regs),
	 },
	{
	 .regs = sensor_colorfx_antique_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_antique_regs),
	 },
	{
	 .regs = sensor_colorfx_set_cbcr_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_set_cbcr_regs),
	 },
};

/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {
};

static struct regval_list sensor_brightness_neg3_regs[] = {
};

static struct regval_list sensor_brightness_neg2_regs[] = {
};

static struct regval_list sensor_brightness_neg1_regs[] = {
};

static struct regval_list sensor_brightness_zero_regs[] = {
};

static struct regval_list sensor_brightness_pos1_regs[] = {
};

static struct regval_list sensor_brightness_pos2_regs[] = {
};

static struct regval_list sensor_brightness_pos3_regs[] = {
};

static struct regval_list sensor_brightness_pos4_regs[] = {
};

static struct cfg_array sensor_brightness[] = {
	{
	 .regs = sensor_brightness_neg4_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg4_regs),
	 },
	{
	 .regs = sensor_brightness_neg3_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg3_regs),
	 },
	{
	 .regs = sensor_brightness_neg2_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg2_regs),
	 },
	{
	 .regs = sensor_brightness_neg1_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg1_regs),
	 },
	{
	 .regs = sensor_brightness_zero_regs,
	 .size = ARRAY_SIZE(sensor_brightness_zero_regs),
	 },
	{
	 .regs = sensor_brightness_pos1_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos1_regs),
	 },
	{
	 .regs = sensor_brightness_pos2_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos2_regs),
	 },
	{
	 .regs = sensor_brightness_pos3_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos3_regs),
	 },
	{
	 .regs = sensor_brightness_pos4_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos4_regs),
	 },
};

/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {
};

static struct regval_list sensor_contrast_neg3_regs[] = {
};

static struct regval_list sensor_contrast_neg2_regs[] = {
};

static struct regval_list sensor_contrast_neg1_regs[] = {
};

static struct regval_list sensor_contrast_zero_regs[] = {
};

static struct regval_list sensor_contrast_pos1_regs[] = {
};

static struct regval_list sensor_contrast_pos2_regs[] = {
};

static struct regval_list sensor_contrast_pos3_regs[] = {
};

static struct regval_list sensor_contrast_pos4_regs[] = {
};

static struct cfg_array sensor_contrast[] = {
	{
	 .regs = sensor_contrast_neg4_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg4_regs),
	 },
	{
	 .regs = sensor_contrast_neg3_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg3_regs),
	 },
	{
	 .regs = sensor_contrast_neg2_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg2_regs),
	 },
	{
	 .regs = sensor_contrast_neg1_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg1_regs),
	 },
	{
	 .regs = sensor_contrast_zero_regs,
	 .size = ARRAY_SIZE(sensor_contrast_zero_regs),
	 },
	{
	 .regs = sensor_contrast_pos1_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos1_regs),
	 },
	{
	 .regs = sensor_contrast_pos2_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos2_regs),
	 },
	{
	 .regs = sensor_contrast_pos3_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos3_regs),
	 },
	{
	 .regs = sensor_contrast_pos4_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos4_regs),
	 },
};

/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {
};

static struct regval_list sensor_saturation_neg3_regs[] = {
};

static struct regval_list sensor_saturation_neg2_regs[] = {
};

static struct regval_list sensor_saturation_neg1_regs[] = {
};

static struct regval_list sensor_saturation_zero_regs[] = {
};

static struct regval_list sensor_saturation_pos1_regs[] = {
};

static struct regval_list sensor_saturation_pos2_regs[] = {
};

static struct regval_list sensor_saturation_pos3_regs[] = {
};

static struct regval_list sensor_saturation_pos4_regs[] = {
};

static struct cfg_array sensor_saturation[] = {
	{
	 .regs = sensor_saturation_neg4_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg4_regs),
	 },
	{
	 .regs = sensor_saturation_neg3_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg3_regs),
	 },
	{
	 .regs = sensor_saturation_neg2_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg2_regs),
	 },
	{
	 .regs = sensor_saturation_neg1_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg1_regs),
	 },
	{
	 .regs = sensor_saturation_zero_regs,
	 .size = ARRAY_SIZE(sensor_saturation_zero_regs),
	 },
	{
	 .regs = sensor_saturation_pos1_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos1_regs),
	 },
	{
	 .regs = sensor_saturation_pos2_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos2_regs),
	 },
	{
	 .regs = sensor_saturation_pos3_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos3_regs),
	 },
	{
	 .regs = sensor_saturation_pos4_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos4_regs),
	 },
};

/*
 * The exposure target setttings
 */
static struct regval_list sensor_ev_neg4_regs[] = {
	{0xfe, 0x01},
	{0x13, 0x10},
	{0xfe, 0x00},
};

static struct regval_list sensor_ev_neg3_regs[] = {
	{0xfe, 0x01},
	{0x13, 0x15},
	{0xfe, 0x00},
};

static struct regval_list sensor_ev_neg2_regs[] = {
	{0xfe, 0x01},
	{0x13, 0x20},
	{0xfe, 0x00},
};

static struct regval_list sensor_ev_neg1_regs[] = {
	{0xfe, 0x01},
	{0x13, 0x25},
	{0xfe, 0x00},
};

static struct regval_list sensor_ev_zero_regs[] = {
	{0xfe, 0x01},
	{0x13, 0x30},
	{0xfe, 0x00},
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{0xfe, 0x01},
	{0x13, 0x35},
	{0xfe, 0x00},
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{0xfe, 0x01},
	{0x13, 0x40},
	{0xfe, 0x00},
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{0xfe, 0x01},
	{0x13, 0x45},
	{0xfe, 0x00},
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{0xfe, 0x01},
	{0x13, 0x50},
	{0xfe, 0x00},
};

static struct cfg_array sensor_ev[] = {
	{
	 .regs = sensor_ev_neg4_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg4_regs),
	 },
	{
	 .regs = sensor_ev_neg3_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg3_regs),
	 },
	{
	 .regs = sensor_ev_neg2_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg2_regs),
	 },
	{
	 .regs = sensor_ev_neg1_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg1_regs),
	 },
	{
	 .regs = sensor_ev_zero_regs,
	 .size = ARRAY_SIZE(sensor_ev_zero_regs),
	 },
	{
	 .regs = sensor_ev_pos1_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos1_regs),
	 },
	{
	 .regs = sensor_ev_pos2_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos2_regs),
	 },
	{
	 .regs = sensor_ev_pos3_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos3_regs),
	 },
	{
	 .regs = sensor_ev_pos4_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos4_regs),
	 },
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{0x84, 0x02},
};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{0x84, 0x03},
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{0x84, 0x01},
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{0x84, 0x00},
};

static struct regval_list sensor_fmt_raw[] = {
	{0x84, 0x18},
};

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	data_type val;
	ret = sensor_write(sd, 0xfe, 0x00);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_g_hflip!\n");
		return ret;
	}

	ret = sensor_read(sd, 0x17, &val);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_g_hflip!\n");
		return ret;
	}

	val &= (1 << 0);
	val = val >> 0;

	*value = val;

	info->hflip = *value;
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	data_type val;

	ret = sensor_write(sd, 0xfe, 0);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_s_hflip!\n");
		return ret;
	}
	ret = sensor_read(sd, 0x17, &val);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_s_hflip!\n");
		return ret;
	}

	switch (value) {
	case 0:
		val &= 0xfe;
		break;
	case 1:
		val |= 0x01;
		break;
	default:
		return -EINVAL;
	}
	ret = sensor_write(sd, 0x17, val);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_s_hflip!\n");
		return ret;
	}

	usleep_range(20000, 22000);

	info->hflip = value;
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	data_type val;

	ret = sensor_write(sd, 0xfe, 0x00);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_g_vflip!\n");
		return ret;
	}

	ret = sensor_read(sd, 0x17, &val);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_g_vflip!\n");
		return ret;
	}

	val &= (1 << 1);
	val = val >> 1;

	*value = val;

	info->vflip = *value;
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	data_type val;

	ret = sensor_write(sd, 0xfe, 0x00);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}

	ret = sensor_read(sd, 0x17, &val);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_s_vflip!\n");
		return ret;
	}

	switch (value) {
	case 0:
		val &= 0xfd;
		break;
	case 1:
		val |= 0x02;
		break;
	default:
		return -EINVAL;
	}
	ret = sensor_write(sd, 0x17, val);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}

	usleep_range(20000, 22000);

	info->vflip = value;
	return 0;
}

static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	data_type val;

	ret = sensor_write(sd, 0xfe, 0x00);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_g_autoexp!\n");
		return ret;
	}

	ret = sensor_read(sd, 0xb6, &val);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_g_autoexp!\n");
		return ret;
	}

	val &= 0x01;
	if (val == 0x01) {
		*value = V4L2_EXPOSURE_AUTO;
	} else {
		*value = V4L2_EXPOSURE_MANUAL;
	}

	info->autoexp = *value;
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
			    enum v4l2_exposure_auto_type value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	data_type val;

	ret = sensor_write(sd, 0xfe, 0x00);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_s_autoexp!\n");
		return ret;
	}

	ret = sensor_read(sd, 0xb6, &val);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_s_autoexp!\n");
		return ret;
	}

	switch (value) {
	case V4L2_EXPOSURE_AUTO:
		val |= 0x01;
		break;
	case V4L2_EXPOSURE_MANUAL:
		val &= 0xfe;
		break;
	case V4L2_EXPOSURE_SHUTTER_PRIORITY:
		return -EINVAL;
	case V4L2_EXPOSURE_APERTURE_PRIORITY:
		return -EINVAL;
	default:
		return -EINVAL;
	}
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_s_autoexp!\n");
		return ret;
	}
	usleep_range(10000, 12000);
	info->autoexp = value;
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	data_type val;

	ret = sensor_write(sd, 0xfe, 0x00);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_g_autowb!\n");
		return ret;
	}

	ret = sensor_read(sd, 0x82, &val);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_g_autowb!\n");
		return ret;
	}

	val &= (1 << 1);
	val = val >> 1;
	*value = val;
	info->autowb = *value;
	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	data_type val;

	ret =
	    sensor_write_array(sd, sensor_wb_auto_regs,
			       ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		sensor_err("sensor_write_array err at sensor_s_autowb!\n");
		return ret;
	}

	ret = sensor_read(sd, 0x82, &val);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_s_autowb!\n");
		return ret;
	}

	switch (value) {
	case 0:
		val &= 0xfd;
		break;
	case 1:
		val |= 0x02;
		break;
	default:
		break;
	}
	ret = sensor_write(sd, 0x82, val);
	if (ret < 0) {
		sensor_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}

	usleep_range(10000, 12000);

	info->autowb = value;
	return 0;
}

static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_hue(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

/* ****************end of ********************** */

static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->brightness;
	return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->brightness == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	sensor_write_array(sd, sensor_brightness[value + 4].regs,
			sensor_brightness[value + 4].size);

	info->brightness = value;
	return 0;
}

static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->contrast;
	return 0;
}

static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->contrast == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	sensor_write_array(sd, sensor_contrast[value + 4].regs,
			sensor_contrast[value + 4].size);

	info->contrast = value;
	return 0;
}

static int sensor_g_saturation(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->saturation;
	return 0;
}

static int sensor_s_saturation(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->saturation == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	sensor_write_array(sd, sensor_saturation[value + 4].regs,
			sensor_saturation[value + 4].size);

	info->saturation = value;
	return 0;
}

static int sensor_g_exp_bias(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp_bias;
	return 0;
}

static int sensor_s_exp_bias(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->exp_bias == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	sensor_write_array(sd, sensor_ev[value + 4].regs,
			sensor_ev[value + 4].size);

	info->exp_bias = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_auto_n_preset_white_balance *wb_type =
	    (enum v4l2_auto_n_preset_white_balance *)value;

	*wb_type = info->wb;

	return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd,
		       enum v4l2_auto_n_preset_white_balance value)
{
	struct sensor_info *info = to_state(sd);

	if (info->capture_mode == V4L2_MODE_IMAGE)
		return 0;

	sensor_write_array(sd, sensor_wb[value].regs, sensor_wb[value].size);

	if (value == V4L2_WHITE_BALANCE_AUTO)
		info->autowb = 1;
	else
		info->autowb = 0;

	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx *)value;

	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd, enum v4l2_colorfx value)
{
	struct sensor_info *info = to_state(sd);

	if (info->clrfx == value)
		return 0;

	sensor_write_array(sd, sensor_colorfx[value].regs,
			sensor_colorfx[value].size);

	info->clrfx = value;
	return 0;
}

static int sensor_g_flash_mode(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_flash_led_mode *flash_mode =
	    (enum v4l2_flash_led_mode *)value;

	*flash_mode = info->flash_mode;
	return 0;
}

static int sensor_s_flash_mode(struct v4l2_subdev *sd,
			       enum v4l2_flash_led_mode value)
{
	struct sensor_info *info = to_state(sd);

	info->flash_mode = value;
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	cci_lock(sd);
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON\n");
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(5000, 12000);
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF\n");
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(5000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(5000, 12000);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON\n");
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_set_pmu_channel(sd, DVDD, ON);
		usleep_range(10000, 12000);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, AFVDD, ON);
		usleep_range(10000, 12000);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF\n");
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		usleep_range(10000, 12000);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_set_pmu_channel(sd, DVDD, OFF);
		usleep_range(5000, 12000);
		vin_set_mclk(sd, OFF);
		usleep_range(5000, 12000);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		break;
	default:
		return -EINVAL;
	}
	cci_unlock(sd);
	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret;
	unsigned int SENSOR_ID = 0;
	data_type val;
	ret = sensor_read(sd, 0xf0, &val);
	SENSOR_ID |= (val << 8);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_detect!\n");
		return ret;
	}

	ret = sensor_read(sd, 0xf1, &val);
	SENSOR_ID |= (val);
	sensor_print("V4L2_IDENT_SENSOR = %x\n", SENSOR_ID);
	if (ret < 0) {
		sensor_err("sensor_read err at sensor_detect!\n");
		return ret;
	}

	if (SENSOR_ID != V4L2_IDENT_SENSOR)
		return -ENODEV;

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	sensor_dbg("sensor_init\n");
	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}
	ret =
	    sensor_write_array(sd, sensor_default_regs,
			       ARRAY_SIZE(sensor_default_regs));
	msleep(350);
	return 0;
}

static int sensor_g_exif(struct v4l2_subdev *sd,
			 struct sensor_exif_attribute *exif)
{
	int ret = 0;
	unsigned int temp = 0, shutter = 0, gain = 0;
	data_type val;

	sensor_write(sd, 0xfe, 0x00);

	/*read shutter */
	sensor_read(sd, 0x03, &val);
	temp |= (val << 8);
	sensor_read(sd, 0x04, &val);
	temp |= (val & 0xff);
	shutter = temp;

	sensor_read(sd, 0xb1, &val);
	gain = val;
	exif->fnumber = 280;
	exif->focal_length = 425;
	exif->brightness = 125;
	exif->flash_fire = 0;
	exif->iso_speed = 50 * gain / 16;

	exif->exposure_time_num = 1;
	exif->exposure_time_den = 16000 / shutter;
	return ret;
}

static void sensor_get_lum(struct v4l2_subdev *sd, unsigned int *lum)
{
	data_type temp = 0;

	sensor_write(sd, 0xfe, 0x01);
	sensor_read(sd, 0x14, &temp);
	*lum = temp;
	sensor_dbg("check luminance=0x%x\n", *lum);
}

static void sensor_g_flash_flag(struct v4l2_subdev *sd,
				unsigned int *flash_flag)
{
	unsigned int current_lum = 0;

	sensor_get_lum(sd, &current_lum);

	if (current_lum < 0x1c)
		*flash_flag = 1;
	else
		*flash_flag = 0;
}

static void sensor_cfg_req(struct v4l2_subdev *sd,
						struct sensor_config *cfg)
{
	struct sensor_info *info = to_state(sd);
	if (info == NULL) {
		sensor_err("sensor is not initialized.\n");
		return;
	}
	if (info->current_wins == NULL) {
		sensor_err("sensor format is not initialized.\n");
		return;
	}

	cfg->width = info->current_wins->width;
	cfg->height = info->current_wins->height;
	cfg->hoffset = info->current_wins->hoffset;
	cfg->voffset = info->current_wins->voffset;
	cfg->hts = info->current_wins->hts;
	cfg->vts = info->current_wins->vts;
	cfg->pclk = info->current_wins->pclk;
	cfg->bin_factor = info->current_wins->bin_factor;
	cfg->intg_min = info->current_wins->intg_min;
	cfg->intg_max = info->current_wins->intg_max;
	cfg->gain_min = info->current_wins->gain_min;
	cfg->gain_max = info->current_wins->gain_max;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	switch (cmd) {
	case GET_SENSOR_EXIF:
		sensor_g_exif(sd, (struct sensor_exif_attribute *)arg);
		break;
	case GET_FLASH_FLAG:
		sensor_g_flash_flag(sd, (unsigned int *)arg);
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct {
	__u8 *desc;
	enum v4l2_mbus_pixelcode mbus_code;
	struct regval_list *regs;
	int regs_size;
	int bpp;		/* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc = "YUYV 4:2:2",
		.mbus_code = V4L2_MBUS_FMT_YUYV8_2X8,
		.regs = sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp = 2,
	}, {
		.desc = "YVYU 4:2:2",
		.mbus_code = V4L2_MBUS_FMT_YVYU8_2X8,
		.regs = sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp = 2,
	}, {
		.desc = "UYVY 4:2:2",
		.mbus_code = V4L2_MBUS_FMT_UYVY8_2X8,
		.regs = sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp = 2,
	}, {
		.desc = "VYUY 4:2:2",
		.mbus_code = V4L2_MBUS_FMT_VYUY8_2X8,
		.regs = sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp = 2,
	}, {
		.desc = "Raw RGB Bayer",
		.mbus_code = V4L2_MBUS_FMT_SBGGR8_1X8,
		.regs = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp = 1
	},
};

#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
	/* UXGA */
	{
	 .width = UXGA_WIDTH,
	 .height = UXGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = sensor_uxga_regs,
	 .regs_size = ARRAY_SIZE(sensor_uxga_regs),
	 .set_size = NULL,
	 },
	/* 720p */
	{
	 .width = HD720_WIDTH,
	 .height = HD720_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = gc2155_hd720_regs,
	 .regs_size = ARRAY_SIZE(gc2155_hd720_regs),
	 .set_size = NULL,
	 },
	/* SVGA */
	{
	 .width = SVGA_WIDTH,
	 .height = SVGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = sensor_svga_regs,
	 .regs_size = ARRAY_SIZE(sensor_svga_regs),
	 .set_size = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_reg_init(struct sensor_info *info)
{

	int ret = 0;
	unsigned int temp = 0, shutter = 0;
	data_type val;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	if ((wsize->width == 1600) && (wsize->height == 1200)) {/*capture mode > 640*480*/

		sensor_write(sd, 0xfe, 0x00);
		sensor_write(sd, 0xb6, 0x00);
		/*read shutter */
		sensor_read(sd, 0x03, &val);
		temp |= (val << 8);
		sensor_read(sd, 0x04, &val);
		temp |= (val & 0xff);
		shutter = temp;
	}

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs) {
		ret = sensor_write_array(sd, wsize->regs, wsize->regs_size);
		if (ret < 0)
			return ret;
	}

	if (wsize->set_size) {
		ret = wsize->set_size(sd);
		if (ret < 0)
			return ret;
	}

	if ((wsize->width == 1600) && (wsize->height == 1200)) {
		sensor_write(sd, 0xfe, 0x00);
		shutter = shutter * 2 / 5;
		if (shutter < 1)
			shutter = 1;
		sensor_write(sd, 0x03, ((shutter >> 8) & 0xff));
		sensor_write(sd, 0x04, (shutter & 0xff));
		msleep(400);
	}

	sensor_s_hflip(sd, info->hflip);
	sensor_s_vflip(sd, info->vflip);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		  info->current_wins->width,
		  info->current_wins->height, info->fmt->mbus_code);

	if (!enable)
		return 0;
	return sensor_reg_init(info);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL;

	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;

	if (info->width > SVGA_WIDTH && info->height > SVGA_HEIGHT) {
		cp->timeperframe.denominator = SENSOR_FRAME_RATE / 2;
	} else {
		cp->timeperframe.denominator = SENSOR_FRAME_RATE;
	}

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	return 0;
}

/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************begin of ***************************** */
static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	/* see sensor_s_parm and sensor_g_parm for the meaning of value */

	switch (qc->id) {
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 1);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_COLORFX:
		return v4l2_ctrl_query_fill(qc, 0, 15, 1, 0);
	case V4L2_CID_FLASH_LED_MODE:
		return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_g_contrast(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_g_saturation(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return sensor_g_hue(sd, &ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_g_hflip(sd, &ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_g_exp_bias(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_g_autoexp(sd, &ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_g_wb(sd, &ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_g_autowb(sd, &ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_g_colorfx(sd, &ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_g_flash_mode(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;

	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0) {
		return ret;
	}

	if (qc.type == V4L2_CTRL_TYPE_MENU ||
	    qc.type == V4L2_CTRL_TYPE_INTEGER ||
	    qc.type == V4L2_CTRL_TYPE_BOOLEAN) {
		if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
			return -ERANGE;
		}
	}
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_s_brightness(sd, ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_s_contrast(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_s_saturation(sd, ctrl->value);
	case V4L2_CID_HUE:
		return sensor_s_hue(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_s_exp_bias(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_s_autoexp(sd,
				(enum v4l2_exposure_auto_type)ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_s_wb(sd,
			(enum v4l2_auto_n_preset_white_balance)ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_colorfx(sd, (enum v4l2_colorfx)ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_s_flash_mode(sd,
				(enum v4l2_flash_led_mode)ctrl->value);
	}
	return -EINVAL;
}

static int sensor_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}


/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_chip_ident = sensor_g_chip_ident,
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
	.g_mbus_config = sensor_g_mbus_config,
};

static void sensor_fill_mbus_fmt(struct v4l2_mbus_framefmt *mf,
				 const struct sensor_win_size *ws, u32 code)
{
	mf->width = ws->width;
	mf->height = ws->height;
	mf->code = code;
	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field = V4L2_FIELD_NONE;
}


SENSOR_ENUM_MBUS_CODE;
SENSOR_ENUM_FRAME_SIZE;
SENSOR_FIND_MBUS_CODE;
SENSOR_FIND_FRAME_SIZE;
SENSOR_TRY_FORMAT;
SENSOR_GET_FMT;
SENSOR_SET_FMT;

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_8,
	.data_width = CCI_BITS_8,
};

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	mutex_init(&info->lock);

	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);

	if (client) {
		client->addr = 0x78 >> 1;
	}
	info->fmt = &sensor_formats[0];
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp = 0;
	info->autoexp = 0;
	info->autowb = 1;
	info->wb = 0;
	info->clrfx = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);
static struct i2c_driver sensor_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_NAME,
		   },
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static __init int init_sensor(void)
{
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);

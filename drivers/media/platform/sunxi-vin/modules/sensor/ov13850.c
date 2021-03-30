/*
 * A V4L2 driver for ov13850 cameras.
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

MODULE_AUTHOR("Chomoly");
MODULE_DESCRIPTION("A low-level driver for ov13850 camera sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x13850
int ov13850_sensor_vts;

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The ov13850 sits on i2c with ID 0x6c or 0x20
 */

#define SENSOR_NAME "ov13850"

static struct v4l2_subdev *glb_sd;


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
};


static struct regval_list sensor_13mega_regs[] = {

	{0x0103, 0x01},

	{0x0300, 0x01},
	{0x0301, 0x00},
	{0x0302, 0x28},
	{0x0303, 0x00},
	{0x030a, 0x00},

	{0x300f, 0x11},
	{0x3010, 0x01},
	{0x3011, 0x76},
	{0x3012, 0x41},
	{0x3013, 0x12},
	{0x3014, 0x11},
	{0x301f, 0x03},
	{0x3106, 0x00},
	{0x3210, 0x47},
	{0x3500, 0x00},
	{0x3501, 0x67},
	{0x3502, 0x80},
	{0x3506, 0x00},
	{0x3507, 0x02},
	{0x3508, 0x00},
	{0x3509, 0x10},
	{0x350a, 0x00},
	{0x350b, 0x10},
	{0x350e, 0x00},
	{0x350f, 0x10},
	{0x3600, 0x40},
	{0x3601, 0xfc},
	{0x3602, 0x02},
	{0x3603, 0x48},
	{0x3604, 0xa5},
	{0x3605, 0x9f},
	{0x3607, 0x00},
	{0x360a, 0x40},
	{0x360b, 0x91},
	{0x360c, 0x49},
	{0x360f, 0x8a},
	{0x3611, 0x10},

	{0x3612, 0x13},

	{0x3613, 0x22},
	{0x3615, 0x08},
	{0x3641, 0x02},
	{0x3660, 0x82},
	{0x3668, 0x54},
	{0x3669, 0x40},
	{0x3667, 0xa0},
	{0x3702, 0x40},
	{0x3703, 0x44},
	{0x3704, 0x2c},
	{0x3705, 0x24},
	{0x3706, 0x50},
	{0x3707, 0x44},
	{0x3708, 0x3c},
	{0x3709, 0x1f},
	{0x370a, 0x26},
	{0x370b, 0x3c},
	{0x3720, 0x66},
	{0x3722, 0x84},
	{0x3728, 0x40},
	{0x372a, 0x00},
	{0x372f, 0x90},
	{0x3710, 0x28},
	{0x3716, 0x03},
	{0x3718, 0x10},
	{0x3719, 0x08},
	{0x371c, 0xfc},
	{0x3760, 0x13},
	{0x3761, 0x34},
	{0x3767, 0x24},
	{0x3768, 0x06},
	{0x3769, 0x45},
	{0x376c, 0x23},
	{0x3d84, 0x00},
	{0x3d85, 0x17},
	{0x3d8c, 0x73},
	{0x3d8d, 0xbf},
	{0x3800, 0x00},
	{0x3801, 0x08},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x10},
	{0x3805, 0x97},
	{0x3806, 0x0c},
	{0x3807, 0x4b},
	{0x3808, 0x08},
	{0x3809, 0x40},

	{0x380a, 0x06},
	{0x380b, 0x20},
	{0x380c, 0x25},
	{0x380d, 0x80},
	{0x380e, 0x06},
	{0x380f, 0x80},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x02},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x1c},
	{0x3836, 0x08},
	{0x3837, 0x02},
	{0x4000, 0xf1},
	{0x4001, 0x00},
	{0x400b, 0x0c},
	{0x4011, 0x00},
	{0x401a, 0x00},
	{0x401b, 0x00},
	{0x401c, 0x00},
	{0x401d, 0x00},
	{0x4020, 0x00},
	{0x4021, 0xe4},
	{0x4022, 0x07},
	{0x4023, 0x5f},
	{0x4024, 0x08},
	{0x4025, 0x44},
	{0x4026, 0x08},
	{0x4027, 0x47},
	{0x4028, 0x00},
	{0x4029, 0x02},
	{0x402a, 0x04},
	{0x402b, 0x08},
	{0x402c, 0x02},
	{0x402d, 0x02},
	{0x402e, 0x0c},
	{0x402f, 0x08},
	{0x403d, 0x2c},
	{0x403f, 0x7f},
	{0x4500, 0x82},
	{0x4501, 0x38},
	{0x4601, 0x04},
	{0x4602, 0x22},

	{0x4603, 0x01},
	{0x4837, 0x19},

	{0x4d00, 0x04},
	{0x4d01, 0x42},
	{0x4d02, 0xd1},
	{0x4d03, 0x90},
	{0x4d04, 0x66},
	{0x4d05, 0x65},
	{0x5000, 0x0e},
	{0x5001, 0x03},
	{0x5002, 0x07},
	{0x5013, 0x40},
	{0x501c, 0x00},
	{0x501d, 0x10},

	{0x5242, 0x00},
	{0x5243, 0xb8},
	{0x5244, 0x00},
	{0x5245, 0xf9},
	{0x5246, 0x00},
	{0x5247, 0xf6},
	{0x5248, 0x00},
	{0x5249, 0xa6},
	{0x5300, 0xfc},
	{0x5301, 0xdf},
	{0x5302, 0x3f},
	{0x5303, 0x08},
	{0x5304, 0x0c},
	{0x5305, 0x10},
	{0x5306, 0x20},
	{0x5307, 0x40},
	{0x5308, 0x08},
	{0x5309, 0x08},
	{0x530a, 0x02},
	{0x530b, 0x01},
	{0x530c, 0x01},
	{0x530d, 0x0c},
	{0x530e, 0x02},
	{0x530f, 0x01},
	{0x5310, 0x01},
	{0x5400, 0x00},
	{0x5401, 0x61},
	{0x5402, 0x00},
	{0x5403, 0x00},
	{0x5404, 0x00},
	{0x5405, 0x40},
	{0x540c, 0x05},
	{0x5b00, 0x00},
	{0x5b01, 0x00},

	{0x5b02, 0x01},
	{0x5b03, 0xff},
	{0x5b04, 0x02},
	{0x5b05, 0x6c},
	{0x5b09, 0x02},
	{0x5e00, 0x00},
	{0x5e10, 0x1c},

	/*above  for init */
	{0x0300, 0x01},
	{0x0302, 0x38},
	{0xffff, 50},

	{0x3501, 0xcf},
	{0x370a, 0x24},
	{0x372a, 0x04},
	{0x372f, 0xa0},
	{0x3801, 0x14},
	{0x3803, 0x0c},
	{0x3805, 0x8b},
	{0x3807, 0x43},
	{0x3808, 0x10},
	{0x3809, 0x70},
	{0x380a, 0x0c},
	{0x380b, 0x30},
	{0x380c, 0x25},
	{0x380d, 0x80},
	{0x380e, 0x0d},
	{0x380f, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x04},
	{0x3836, 0x04},
	{0x3837, 0x01},
	{0x4020, 0x02},
	{0x4021, 0x3c},
	{0x4022, 0x0e},
	{0x4023, 0x37},
	{0x4024, 0x0f},
	{0x4025, 0x1c},
	{0x4026, 0x0f},
	{0x4027, 0x1f},
	{0x4603, 0x01},
	{0x4837, 0x19},
	{0x4826, 0x12},
	{0x5401, 0x71},
	{0x5405, 0x80},
	{0x0100, 0x01},
};


static struct regval_list sensor_4k_videos[] = {
	{0x0103, 0x01},

	{0x030a, 0x00},

	{0x300f, 0x11},
	{0x3010, 0x01},
	{0x3011, 0x76},
	{0x3012, 0x41},
	{0x3013, 0x12},
	{0x3014, 0x11},
	{0x301f, 0x03},
	{0x3106, 0x00},
	{0x3210, 0x47},
	{0x3500, 0x00},
	{0x3501, 0x67},
	{0x3502, 0x80},
	{0x3506, 0x00},
	{0x3507, 0x02},
	{0x3508, 0x00},
	{0x3509, 0x10},
	{0x350a, 0x00},
	{0x350b, 0x10},
	{0x350e, 0x00},
	{0x350f, 0x10},
	{0x3600, 0x40},
	{0x3601, 0xfc},
	{0x3602, 0x02},
	{0x3603, 0x48},
	{0x3604, 0xa5},
	{0x3605, 0x9f},
	{0x3607, 0x00},
	{0x360a, 0x40},
	{0x360b, 0x91},
	{0x360c, 0x49},
	{0x360f, 0x8a},
	{0x3611, 0x10},

	{0x3613, 0x11},
	{0xffff, 50},
	{0x3615, 0x08},
	{0x3641, 0x02},
	{0x3660, 0x82},
	{0x3668, 0x54},
	{0x3669, 0x40},
	{0x3667, 0xa0},
	{0x3702, 0x40},
	{0x3703, 0x44},
	{0x3704, 0x2c},
	{0x3705, 0x24},
	{0x3706, 0x50},
	{0x3707, 0x44},
	{0x3708, 0x3c},
	{0x3709, 0x1f},
	{0x370a, 0x26},
	{0x370b, 0x3c},
	{0x3720, 0x66},
	{0x3722, 0x84},
	{0x3728, 0x40},
	{0x372a, 0x00},
	{0x372f, 0x90},
	{0x3710, 0x28},
	{0x3716, 0x03},
	{0x3718, 0x10},
	{0x3719, 0x08},
	{0x371c, 0xfc},
	{0x3760, 0x13},
	{0x3761, 0x34},
	{0x3767, 0x24},
	{0x3768, 0x06},
	{0x3769, 0x45},
	{0x376c, 0x23},
	{0x3d84, 0x00},
	{0x3d85, 0x17},

	{0x3d8c, 0x73},
	{0x3d8d, 0xbf},
	{0x3800, 0x00},
	{0x3801, 0x08},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x10},
	{0x3805, 0x97},
	{0x3806, 0x0c},
	{0x3807, 0x4b},
	{0x3808, 0x08},
	{0x3809, 0x40},

	{0x380a, 0x06},
	{0x380b, 0x20},
	{0x380c, 0x25},
	{0x380d, 0x80},
	{0x380e, 0x06},
	{0x380f, 0x80},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x02},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x1c},
	{0x3836, 0x08},
	{0x3837, 0x02},
	{0x4000, 0xf1},
	{0x4001, 0x00},
	{0x400b, 0x0c},
	{0x4011, 0x00},
	{0x401a, 0x00},
	{0x401b, 0x00},
	{0x401c, 0x00},
	{0x401d, 0x00},
	{0x4020, 0x00},
	{0x4021, 0xe4},
	{0x4022, 0x07},
	{0x4023, 0x5f},
	{0x4024, 0x08},
	{0x4025, 0x44},
	{0x4026, 0x08},
	{0x4027, 0x47},
	{0x4028, 0x00},
	{0x4029, 0x02},
	{0x402a, 0x04},
	{0x402b, 0x08},
	{0x402c, 0x02},
	{0x402d, 0x02},
	{0x402e, 0x0c},
	{0x402f, 0x08},
	{0x403d, 0x2c},
	{0x403f, 0x7f},
	{0x4500, 0x82},
	{0x4501, 0x38},
	{0x4601, 0x04},
	{0x4602, 0x22},

	{0x4603, 0x01},
	{0x4837, 0x19},
	{0x4800, 0x04},
	{0x4802, 0x42},
	{0x481a, 0x00},
	{0x481b, 0x1c},
	{0x4826, 0x12},

	{0x4d00, 0x04},
	{0x4d01, 0x42},
	{0x4d02, 0xd1},
	{0x4d03, 0x90},
	{0x4d04, 0x66},
	{0x4d05, 0x65},
	{0x5000, 0x0e},
	{0x5001, 0x03},
	{0x5002, 0x07},
	{0x5013, 0x40},
	{0x501c, 0x00},
	{0x501d, 0x10},
	{0x5242, 0x00},
	{0x5243, 0xb8},
	{0x5244, 0x00},
	{0x5245, 0xf9},
	{0x5246, 0x00},
	{0x5247, 0xf6},
	{0x5248, 0x00},
	{0x5249, 0xa6},
	{0x5300, 0xfc},
	{0x5301, 0xdf},
	{0x5302, 0x3f},
	{0x5303, 0x08},
	{0x5304, 0x0c},
	{0x5305, 0x10},
	{0x5306, 0x20},
	{0x5307, 0x40},
	{0x5308, 0x08},
	{0x5309, 0x08},
	{0x530a, 0x02},
	{0x530b, 0x01},
	{0x530c, 0x01},
	{0x530d, 0x0c},
	{0x530e, 0x02},
	{0x530f, 0x01},
	{0x5310, 0x01},
	{0x5400, 0x00},
	{0x5401, 0x61},
	{0x5402, 0x00},
	{0x5403, 0x00},
	{0x5404, 0x00},
	{0x5405, 0x40},
	{0x540c, 0x05},
	{0x5b00, 0x00},
	{0x5b01, 0x00},

	{0x5b02, 0x01},
	{0x5b03, 0xff},
	{0x5b04, 0x02},
	{0x5b05, 0x6c},
	{0x5b09, 0x02},
	{0x5e00, 0x00},
	{0x5e10, 0x1c},

	{0x3813, 0x04},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x04},
	{0x3836, 0x04},
	{0x3837, 0x01},

	{0x4837, 0x0a},
	{0x4826, 0x12},
	{0x5401, 0x71},
	{0x5405, 0x80},

	{0x3612, 0x07},

	{0x0300, 0x00},
	{0x0301, 0x00},
	{0x0302, 0x20},

	{0x0303, 0x00},
	{0x4837, 0x0d},

	{0x370a, 0x24},
	{0x372a, 0x04},
	{0x372f, 0xa0},
	{0x3800, 0x01},
	{0x3801, 0x4c},
	{0x3802, 0x02},
	{0x3803, 0x8C},
	{0x3804, 0x10},
	{0x3805, 0x53},
	{0x3806, 0x0B},
	{0x3807, 0x03},
	{0x3808, 0x0F},
	{0x3809, 0x0},
	{0x380A, 0x08},
	{0x380B, 0x70},
	{0x380C, 0x1a},
	{0x380D, 0x90},
	{0x380E, 0x0b},
	{0x380F, 0xb0},

	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3836, 0x04},
	{0x3837, 0x01},
	{0x4020, 0x0},
	{0x4021, 0xE6},
	{0x4022, 0xE},
	{0x4023, 0x1E},
	{0x4024, 0xF},
	{0x4025, 0x0},
	{0x4026, 0xF},
	{0x4027, 0x6},

	{0x0100, 0x01},

};

static struct regval_list sensor_1080p_regs[] = {

/*
* using quarter size to scale down
*/
	{0x0103, 0x01},

	{0x0300, 0x01},
	{0x0301, 0x00},
	{0x0302, 0x28},
	{0x0303, 0x00},
	{0x030a, 0x00},

	{0x300f, 0x11},
	{0x3010, 0x01},
	{0x3011, 0x76},
	{0x3012, 0x41},
	{0x3013, 0x12},
	{0x3014, 0x11},
	{0x301f, 0x03},
	{0x3106, 0x00},
	{0x3210, 0x47},
	{0x3500, 0x00},
	{0x3501, 0x67},
	{0x3502, 0x80},
	{0x3506, 0x00},
	{0x3507, 0x02},
	{0x3508, 0x00},
	{0x3509, 0x10},
	{0x350a, 0x00},
	{0x350b, 0x10},
	{0x350e, 0x00},
	{0x350f, 0x10},
	{0x3600, 0x40},
	{0x3601, 0xfc},
	{0x3602, 0x02},
	{0x3603, 0x48},
	{0x3604, 0xa5},
	{0x3605, 0x9f},
	{0x3607, 0x00},
	{0x360a, 0x40},
	{0x360b, 0x91},
	{0x360c, 0x49},
	{0x360f, 0x8a},
	{0x3611, 0x10},

	{0x3612, 0x13},

	{0x3613, 0x22},

	{0x3615, 0x08},
	{0x3641, 0x02},
	{0x3660, 0x82},
	{0x3668, 0x54},
	{0x3669, 0x40},
	{0x3667, 0xa0},
	{0x3702, 0x40},
	{0x3703, 0x44},
	{0x3704, 0x2c},
	{0x3705, 0x24},
	{0x3706, 0x50},
	{0x3707, 0x44},
	{0x3708, 0x3c},
	{0x3709, 0x1f},
	{0x370a, 0x26},
	{0x370b, 0x3c},
	{0x3720, 0x66},
	{0x3722, 0x84},
	{0x3728, 0x40},
	{0x372a, 0x00},
	{0x372f, 0x90},
	{0x3710, 0x28},
	{0x3716, 0x03},
	{0x3718, 0x10},
	{0x3719, 0x08},
	{0x371c, 0xfc},
	{0x3760, 0x13},
	{0x3761, 0x34},
	{0x3767, 0x24},
	{0x3768, 0x06},
	{0x3769, 0x45},
	{0x376c, 0x23},
	{0x3d84, 0x00},
	{0x3d85, 0x17},
	{0x3d8c, 0x73},
	{0x3d8d, 0xbf},
	{0x3800, 0x00},
	{0x3801, 0x08},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x10},
	{0x3805, 0x97},
	{0x3806, 0x0c},
	{0x3807, 0x4b},
	{0x3808, 0x08},
	{0x3809, 0x40},

	{0x380a, 0x06},
	{0x380b, 0x20},
	{0x380c, 0x25},
	{0x380d, 0x80},
	{0x380e, 0x06},
	{0x380f, 0x80},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x02},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x05},
	{0x3834, 0x00},
	{0x3835, 0x1c},
	{0x3836, 0x08},
	{0x3837, 0x02},
	{0x4000, 0xf1},
	{0x4001, 0x00},
	{0x400b, 0x0c},
	{0x4011, 0x00},
	{0x401a, 0x00},
	{0x401b, 0x00},
	{0x401c, 0x00},
	{0x401d, 0x00},
	{0x4020, 0x00},
	{0x4021, 0xe4},
	{0x4022, 0x07},
	{0x4023, 0x5f},
	{0x4024, 0x08},
	{0x4025, 0x44},
	{0x4026, 0x08},
	{0x4027, 0x47},
	{0x4028, 0x00},
	{0x4029, 0x02},
	{0x402a, 0x04},
	{0x402b, 0x08},
	{0x402c, 0x02},
	{0x402d, 0x02},
	{0x402e, 0x0c},
	{0x402f, 0x08},
	{0x403d, 0x2c},
	{0x403f, 0x7f},
	{0x4500, 0x82},
	{0x4501, 0x38},
	{0x4601, 0x04},
	{0x4602, 0x22},

	{0x4603, 0x01},
	{0x4837, 0x19},

	{0x4d00, 0x04},
	{0x4d01, 0x42},
	{0x4d02, 0xd1},
	{0x4d03, 0x90},
	{0x4d04, 0x66},
	{0x4d05, 0x65},
	{0x5000, 0x0e},
	{0x5001, 0x03},
	{0x5002, 0x07},
	{0x5013, 0x40},
	{0x501c, 0x00},
	{0x501d, 0x10},

	{0x5056, 0x08},
	{0x5058, 0x08},
	{0x505a, 0x08},
	{0x5242, 0x00},
	{0x5243, 0xb8},
	{0x5244, 0x00},
	{0x5245, 0xf9},
	{0x5246, 0x00},
	{0x5247, 0xf6},
	{0x5248, 0x00},
	{0x5249, 0xa6},
	{0x5300, 0xfc},
	{0x5301, 0xdf},
	{0x5302, 0x3f},
	{0x5303, 0x08},
	{0x5304, 0x0c},
	{0x5305, 0x10},
	{0x5306, 0x20},
	{0x5307, 0x40},
	{0x5308, 0x08},
	{0x5309, 0x08},
	{0x530a, 0x02},
	{0x530b, 0x01},
	{0x530c, 0x01},
	{0x530d, 0x0c},
	{0x530e, 0x02},
	{0x530f, 0x01},
	{0x5310, 0x01},
	{0x5400, 0x00},
	{0x5401, 0x61},
	{0x5402, 0x00},
	{0x5403, 0x00},
	{0x5404, 0x00},
	{0x5405, 0x40},
	{0x540c, 0x05},
	{0x5b00, 0x00},
	{0x5b01, 0x00},

	{0x5b02, 0x01},
	{0x5b03, 0xff},
	{0x5b04, 0x02},
	{0x5b05, 0x6c},
	{0x5b09, 0x02},
	{0x5e00, 0x00},
	{0x5e10, 0x1c},

	{0x0300, 0x01},
	{0x0302, 0x28},
	{0xffff, 50},
	{0x3501, 0x67},
	{0x370a, 0x26},
	{0x372a, 0x00},
	{0x372f, 0x90},
	{0x3801, 0x08},
	{0x3803, 0x04},
	{0x3805, 0x97},
	{0x3807, 0x4b},
	{0x3808, 0x08},
	{0x3809, 0x40},
	{0x380a, 0x06},
	{0x380b, 0x20},
	{0x380c, 0x25},
	{0x380d, 0x80},
	{0x380e, 0x0a},
	{0x380f, 0x80},
	{0x3813, 0x02},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x02},
	{0x3821, 0x05},
	{0x3836, 0x08},
	{0x3837, 0x02},
	{0x4020, 0x00},
	{0x4021, 0xe4},
	{0x4022, 0x07},
	{0x4023, 0x5f},
	{0x4024, 0x08},
	{0x4025, 0x44},
	{0x4026, 0x08},
	{0x4027, 0x47},
	{0x4603, 0x01},
	{0x4837, 0x19},
	{0x4802, 0x42},
	{0x481a, 0x00},
	{0x481b, 0x1c},
	{0x4826, 0x12},

	{0x5401, 0x61},
	{0x5405, 0x40},
	{0x0100, 0x01},

};

/*
* Here we'll try to encapsulate the changes for just the output
* video format.
*
*/
static struct regval_list sensor_fmt_raw[] = {

};

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, frame_length, shutter;
	unsigned char explow = 0, expmid = 0, exphigh = 0;
	unsigned short gainlow = 0, gainhigh = 0;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;



	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	if (gain_val < 1 * 16)
		gain_val = 16;

	gainlow = (unsigned char)(gain_val & 0xff);
	gainhigh = (unsigned char)((gain_val >> 8) & 0x3);

	exp_val >>= 4;
	exphigh = (unsigned char)((0xffff & exp_val) >> 12);
	expmid = (unsigned char)((0xfff & exp_val) >> 4);
	explow = (unsigned char)((0x0f & exp_val) << 4);

	shutter = exp_val;
	if (shutter > ov13850_sensor_vts - 6)
		frame_length = shutter + 6;
	else
		frame_length = ov13850_sensor_vts;

	sensor_write(sd, 0x3208, 0x00);

	sensor_write(sd, 0x380f, (frame_length & 0xff));
	sensor_write(sd, 0x380e, (frame_length >> 8));

	sensor_write(sd, 0x3502, explow);
	sensor_write(sd, 0x3501, expmid);
	sensor_write(sd, 0x3500, exphigh);

	sensor_write(sd, 0x350b, gainlow);
	sensor_write(sd, 0x350a, gainhigh);

	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xa0);

	info->gain = gain_val;
	info->exp = exp_val;

	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	exp_val >>= 4;
	exphigh = (unsigned char)((0xffff & exp_val) >> 12);
	expmid = (unsigned char)((0xfff & exp_val) >> 4);
	explow = (unsigned char)((0x0f & exp_val) << 4);

	sensor_write(sd, 0x3208, 0x00);

	sensor_write(sd, 0x3502, explow);
	sensor_write(sd, 0x3501, expmid);
	sensor_write(sd, 0x3500, exphigh);



	info->exp = exp_val;
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	unsigned char gainlow = 0;
	unsigned char gainhigh = 0;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;
	sensor_dbg("sensor_set_gain = %d\n", gain_val);

	gainlow = (unsigned char)(gain_val & 0xff);
	gainhigh = (unsigned char)((gain_val >> 8) & 0x3);

	sensor_write(sd, 0x350b, gainlow);
	sensor_write(sd, 0x350a, gainhigh);
	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xa0);


	info->gain = gain_val;

	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	ret = sensor_read(sd, 0x0100, &rdval);
	if (ret != 0)
		return ret;

	if (on_off == CSI_GPIO_LOW) {
		ret = sensor_write(sd, 0x0100, rdval & 0xfe);
	} else {
		ret = sensor_write(sd, 0x0100, rdval | 0x01);
	}
	return ret;
}

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		cci_unlock(sd);
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		ret = sensor_s_sw_stby(sd, CSI_GPIO_HIGH);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AFVDD, ON);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}
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
	data_type rdval;

	sensor_read(sd, 0x300a, &rdval);
	if (rdval != 0xD8)
		return -ENODEV;

	sensor_read(sd, 0x300b, &rdval);
	if (rdval != 0x50)
		return -ENODEV;

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;


	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 15;	/* 30fps */

	info->preview_first_flag = 1;

	return 0;
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
	struct sensor_info *info = to_state(sd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg,
			       info->current_wins,
			       sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case ISP_SET_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
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
		.desc = "Raw RGB Bayer",
		.mbus_code = V4L2_MBUS_FMT_SBGGR10_1X10,
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
#if 1
	/* Fullsize: 4208*3120 */
	{
	 .width = 4208,
	 .height = 3120,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 9600 / 16,
	 .vts = 3328 - 6,
	 .pclk = (637 * 1000 * 1000 + 800 * 1000) / 16,
	 .mipi_bps = 850 * 1000 * 1000,
	 .fps_fixed = 2,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (3328 - 6) << 4,
	 .gain_min = 20,
	 .gain_max = (16 << 4) - 1,
	 .regs = sensor_13mega_regs,
	 .regs_size = ARRAY_SIZE(sensor_13mega_regs),
	 .set_size = NULL,
	 },
#endif
#if 1
	/*4k video */
	{
	 .width = 3840,
	 .height = 2160,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 6800 / 16,
	 .vts = 2992,
	 .pclk = 468 * 1000 * 1000 / 16,
	 .mipi_bps = 850 * 1000 * 1000,
	 .fps_fixed = 2,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = 2992 << 4,
	 .gain_min = 20,
	 .gain_max = (8 << 4) - 1,
	 .regs = sensor_4k_videos,
	 .regs_size = ARRAY_SIZE(sensor_4k_videos),
	 .set_size = NULL,
	 },

	/* 2112*1568 */
	{
	 .width = 1920,
	 .height = 1080,
	 .hoffset = (2112 - 1920) / 2,
	 .voffset = (1568 - 1080) / 2,
	 .hts = 9600 / 16,
	 .vts = 1664,
	 .pclk = 639 * 1000 * 1000 / 16,
	 .mipi_bps = 640 * 1000 * 1000,
	 .fps_fixed = 1,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = 1664 << 4,
	 .gain_min = 20,
	 .gain_max = (16 << 4) - 1,
	 .regs = sensor_1080p_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p_regs),
	 .set_size = NULL,
	 },
#endif

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_reg_init(struct sensor_info *info)
{

	int ret = 0;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = sensor_write_array(sd, sensor_default_regs,
			       ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	ov13850_sensor_vts = wsize->vts;

	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
		      wsize->height);

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
	cfg->type = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

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
	cp->capturemode = info->capture_mode;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_s_parm\n");

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (info->tpf.numerator == 0)
		return -EINVAL;

	info->capture_mode = cp->capturemode;

	return 0;
}

static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */

	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1 * 16, 64 * 16 - 1, 1, 1 * 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 65535 * 16, 1, 0);
	case V4L2_CID_FRAME_RATE:
		return v4l2_ctrl_query_fill(qc, 15, 120, 1, 120);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->value);
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

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		sensor_err("max gain qurery is %d,min gain qurey is %d\n",
			    qc.maximum, qc.minimum);
		return -ERANGE;
	}

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->value);
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
	.addr_width = CCI_BITS_16,
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
	sd = &info->sd;
	glb_sd = sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);

	info->fmt = &sensor_formats[0];
	info->af_first_flag = 1;

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

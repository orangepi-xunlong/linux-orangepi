/*
 * A V4L2 driver for ov2640 cameras.
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


MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for ov5640 sensors");
MODULE_LICENSE("GPL");

#define AF_WIN_NEW_COORD
//for internel driver debug
#define DEV_DBG_EN      0
#if(DEV_DBG_EN == 1)
#define vfe_dev_dbg(x,arg...) printk("[OV2640]"x,##arg)
#else
#define vfe_dev_dbg(x,arg...)
#endif
#define vfe_dev_err(x,arg...) printk("[OV2640]"x,##arg)
#define vfe_dev_print(x,arg...) printk("[OV2640]"x,##arg)

#define LOG_ERR_RET(x)  { \
                          int ret;  \
                          ret = x; \
                          if(ret < 0) {\
                            vfe_dev_err("error at %s\n",__func__);  \
                            return ret; \
                          } \
                        }

//define module timing
#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x2640

//define the voltage level of control signal
#define CSI_STBY_ON     1
#define CSI_STBY_OFF    0
#define CSI_RST_ON      0
#define CSI_RST_OFF     1
#define CSI_PWR_ON      1
#define CSI_PWR_OFF     0

#define regval_list reg_list_a8_d8

#define REG_TERM 0xff
#define VAL_TERM 0xff
#define REG_DLY  0xffff

//#define AUTO_FPS
#define SHARPNESS 0x10


/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The ov2640 sits on i2c with ID 0x60
 */
#define I2C_ADDR 0x60

#define SENSOR_NAME "ov2640"

//static struct delayed_work sensor_s_ae_ratio_work;
static struct v4l2_subdev *glb_sd;

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */

struct cfg_array { /* coming later */
	struct regval_list * regs;
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
#if 0
{0xff,0x00},
{0x2c,0xff},
{0x2e,0xdf},
{0xff,0x01},
{0x3c,0x32},
{0x11,0x00},
{0x09,0x03},
{0x04,0xf8},
{0x13,0xf5},
{0x14,0x48},
{0x2c,0x0c},
{0x33,0x78},
{0x3a,0x33},
{0x3b,0xfB},
{0x3e,0x00},
{0x43,0x11},
{0x16,0x10},
{0x39,0x02},
{0x35,0x88},
{0x22,0x09},
{0x37,0x40},
{0x23,0x00},
{0x34,0xa0},
{0x36,0x1a},
{0x06,0x02},
{0x07,0xc0},
{0x0d,0xb7},
{0x0e,0x01},
{0x4c,0x00},
{0x4a,0x81},
{0x21,0x99},
{0x24,0x40},
{0x25,0x38},
{0x26,0x82},
{0x5c,0x00},
{0x63,0x00},
{0x46,0x00},
{0x0c,0x3c},
{0x61,0x70},
{0x62,0x80},
{0x7c,0x05},
{0x20,0x80},
{0x28,0x30},
{0x6c,0x00},
{0x6d,0x80},
{0x6e,0x00},
{0x70,0x02},
{0x71,0x94},
{0x73,0xc1},
{0x3d,0x38},
{0x5a,0x9b},
{0x4f,0x7c},
{0x50,0x67},
{0xff,0x00},
{0xe5,0x7f},
{0xf9,0xc0},
{0x41,0x24},
{0xe0,0x14},
{0x76,0xff},
{0x33,0xa0},
{0x42,0x20},
{0x43,0x18},
{0x4c,0x00},
{0x87,0xd0},
{0x88,0x3f},
{0xd7,0x03},
{0xd9,0x10},
{0xd3,0x82},
{0xc8,0x08},
{0xc9,0x80},
{0x7c,0x00},
{0x7d,0x00},
{0x7c,0x03},
{0x7d,0x48},
{0x7d,0x48},
{0x7c,0x08},
{0x7d,0x20},
{0x7d,0x10},
{0x7d,0x0e},
{0x90,0x00},
{0x91,0x0e},
{0x91,0x1a},
{0x91,0x31},
{0x91,0x5a},
{0x91,0x69},
{0x91,0x75},
{0x91,0x7e},
{0x91,0x88},
{0x91,0x8f},
{0x91,0x96},
{0x91,0xa3},
{0x91,0xaf},
{0x91,0xc4},
{0x91,0xd7},
{0x91,0xe8},
{0x91,0x20},
{0x92,0x00},
{0x93,0x06},
{0x93,0xe3},
{0x93,0x05},
{0x93,0x05},
{0x93,0x00},
{0x93,0x04},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},
{0x96,0x00},
{0x97,0x08},
{0x97,0x19},
{0x97,0x02},
{0x97,0x0c},
{0x97,0x24},
{0x97,0x30},
{0x97,0x28},
{0x97,0x26},
{0x97,0x02},
{0x97,0x98},
{0x97,0x80},
{0x97,0x00},
{0x97,0x00},
{0xc3,0xed},
{0xa4,0x00},
{0xa8,0x00},
{0xc5,0x11},
{0xc6,0x51},
{0xbf,0x80},
{0xc7,0x10},
{0xb6,0x66},
{0xb8,0xA5},
{0xb7,0x64},
{0xb9,0x7C},
{0xb3,0xaf},
{0xb4,0x97},
{0xb5,0xFF},
{0xb0,0xC5},
{0xb1,0x94},
{0xb2,0x0f},
{0xc4,0x5c},
{0xe0,0x04},
{0xc0,0xc8},
{0xc1,0x96},
{0x86,0x3d},
{0x50,0x89},
{0x51,0x90},
{0x52,0x2c},
{0x53,0x00},
{0x54,0x00},
{0x55,0x88},
{0x57,0x00},
{0x5a,0xa0},
{0x5b,0x78},
{0x5c,0x00},
{0xe0,0x00},
{0xc3,0xed},
{0x7f,0x00},
{0xda,0x00},
{0xe5,0x1f},
{0xe1,0x67},
{0xe0,0x00},
{0xdd,0x7f},
{0x05,0x00},
#else
{0xff,0x01},
{0x12,0x80},
{0xff,0x00},
{0x2c,0xff},
{0x2e,0xdf},
{0xff,0x01},

{0x03,0x0f},// 0x8f peak	//4f	//Bit[7:6]: Dummy frame control
{0x0f,0x4b},


{0x3c,0x32},
{0x11,0x00},
{0x09,0x01},//03},	//Bit[1:0]: Output drive select
{0x04,0xa8},//b7,b6 directs
{0x13,0xe5},
{0x14,0x28}, //0x48 peak
{0x2c,0x0c},
{0x33,0x78},
{0x3a,0x33},
{0x3b,0xfB},
{0x3e,0x00},
{0x43,0x11},
{0x16,0x10},
{0x39,0x02},
{0x35,0x88},
{0x22,0x09},
{0x37,0x40},
{0x23,0x00},
{0x34,0xa0},
{0x36,0x1a},
{0x06,0x02},
{0x07,0xc0},
{0x0d,0xb7},
{0x0e,0x01},
{0x4c,0x00},
{0x4a,0x81},
{0x21,0x99},
//aec
//{{0x24},{0x58}},
//{{0x25},{0x50}},
//{{0x26},{0x92}},

//{{0x24, 0x70}},
//{{0x25, 0x60}},
//{{0x26, 0xa4}},
{0x24,0x48},
{0x25,0x38},//40
{0x26,0x82},//82

{0x5c,0x00},
{0x63,0x00},
{0x46,0x3f},
{0x0c,0x3c},
{0x61,0x70},
{0x62,0x80},
{0x7c,0x05},
{0x20,0x80},
{0x28,0x30},
{0x6c,0x00},
{0x6d,0x80},
{0x6e,0x00},
{0x70,0x02},
{0x71,0x94},
{0x73,0xc1},
{0x3d,0x34},
{0x5a,0x57},
{0x4f,0xbb},
{0x50,0x9c},
{0xff,0x00},
{0xe5,0x7f},
{0xf9,0xc0},
{0x41,0x24},
{0xe0,0x14},
{0x76,0xff},
{0x33,0xa0},
{0x42,0x20},
{0x43,0x18},
{0x4c,0x00},
{0x87,0xd0},
{0x88,0x3f},
{0xd7,0x03},
{0xd9,0x10},
{0xd3,0x82},
{0xc8,0x08},
{0xc9,0x80},
//
//{{0xff},{0x00}}, //added by peak on 20120409
{0x7c,0x00},
{0x7d,0x02},//0x00 peak//0x07,��������?����e��????t��???��??2??
{0x7c,0x03},
{0x7d,0x28},//0x48//0x40 ?a???�̨�??-o��D?��?,3y��????����e��????t��?��?D�䨢?
{0x7d,0x28},//0x48 peak//0x40 ?a???�̨�??-o��D?��?,3y��????����e��????t��?��?D�䨢?

// removed by peak on 20120409

{0x7c,0x08},
{0x7d,0x20},
{0x7d,0x10},//0x10
{0x7d,0x0e},//0x0e

//contrast added by peak on 20120409
{0x7c,0x00},
{0x7d,0x04},//0x48//0x40
{0x7c,0x07},//0x48 peak//0x40
{0x7d,0x20},
{0x7d,0x1c},
{0x7d,0x2a},//0x10
{0x7d,0x06},//0x0e

{0xff,0x00},//register page
{0x7c,0x00},
{0x7d,0x04},
{0x7c,0x09},
{0x7d,0x00},//brightness value
{0x7d,0x00},//sign

{0xff,0x00},
{0x7c,0x00},
{0x7d,0x00},
{0x7c,0x05},
{0x7d,0x80},
{0x7d,0x80},

{0x90,0x00},
{0x91,0x0e},
{0x91,0x1a},//e3
{0x91,0x31},
{0x91,0x5a},
{0x91,0x69},
{0x91,0x75},
{0x91,0x7e},
{0x91,0x88},
{0x91,0x8f},
{0x91,0x96},
{0x91,0xa3},
{0x91,0xaf},
{0x91,0xc4},
{0x91,0xd7},
{0x91,0xe8},
{0x91,0x20},

{0x90,0x00},
{0x91,0x04},
{0x91,0x0c},
{0x91,0x20},
{0x91,0x4c},
{0x91,0x60},
{0x91,0x74},
{0x91,0x82},
{0x91,0x8e},
{0x91,0x97},
{0x91,0x9f},
{0x91,0xaa},
{0x91,0xb4},
{0x91,0xc8},
{0x91,0xd9},
{0x91,0xe8},
{0x91,0x20},



{0x92,0x00},
{0x93,0x06},
{0x93,0xc8},//e3
{0x93,0x05},
{0x93,0x05},
{0x93,0x00},
{0x93,0x04},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},
{0x93,0x00},

{0x96,0x00},
{0x97,0x08},
{0x97,0x19},
{0x97,0x02},
{0x97,0x0c},
{0x97,0x24},
{0x97,0x30},
{0x97,0x28},
{0x97,0x26},
{0x97,0x02},
{0x97,0x98},
{0x97,0x80},
{0x97,0x00},
{0x97,0x00},
{0xc3,0xef},//ed
{0xa4,0x00},
{0xa8,0x00},

{0xbf,0x00},
{0xba,0xdc},
{0xbb,0x08},
{0xb6,0x20},
{0xb8,0x30},
{0xb7,0x20},
{0xb9,0x30},
{0xb3,0xb4},
{0xb4,0xca},
{0xb5,0x34},
{0xb0,0x46},
{0xb1,0x46},
{0xb2,0x06},
{0xc7,0x00},
{0xc6,0x51},
{0xc5,0x11},
{0xc4,0x9c},
///
{0xc0,0xc8},
{0xc1,0x96},
{0x86,0x3d},
{0x50,0x92},
{0x51,0x90},
{0x52,0x2c},
{0x53,0x00},
{0x54,0x00},
{0x55,0x88},
{0x57,0x00},
{0x5a,0x50},
{0x5b,0x3c},
{0x5c,0x00},
{0xc3,0xed},
{0x7f,0x00},
{0xda,0x01},
{0xe5,0x1f},
{0xe1,0x67},
{0xe0,0x00},
{0xdd,0x7f},
{0x05,0x00},


#if 1
{0xff,0x01},
{0x5d,0x55},//0x00
//{{0x5e, 0x7d}},//0x3c
//{{0x5f, 0x7d}},//0x28
//{{0x60, 0x55}},//0x55
{0x5e,0x55},//0x3c
{0x5f,0x55},//0x28
{0x60,0x55},//0x55

{0xff,0x00},
{0xc3,0xef},
{0xa6,0x00},
{0xa7,0x0f},
{0xa7,0x4e},
{0xa7,0x7a},
{0xa7,0x33},
{0xa7,0x00},
{0xa7,0x23},
{0xa7,0x27},
{0xa7,0x3a},
{0xa7,0x70},
{0xa7,0x33},
{0xa7,0x00},//L
{0xa7,0x23},
{0xa7,0x20},
{0xa7,0x0c},
{0xa7,0x66},
{0xa7,0x33},
{0xa7,0x00},
{0xa7,0x23},
{0xc3,0xef},

{0xa6,0x00},
{0xa7,0x20},
{0xa7,0x00},
{0xa7,0x20},//red	//18
{0xa7,0x31},
{0xa7,0x10},
{0xa7,0x28},
{0xa7,0x00},
{0xa7,0x20},
{0xa7,0x1a},//green
{0xa7,0x31},
{0xa7,0x10},
{0xa7,0x28},
{0xa7,0x00},
{0xa7,0x20},
{0xa7,0x19},//blue
{0xa7,0x31},
{0xa7,0x10},
{0xa7,0x28},

#endif


#if 1
{0xff,0x00},
{0x92,0x00},
{0x93,0x06}, //0x06 peak
{0x93,0xe3},//e
{0x93,0x05},
{0x93,0x03},
{0x93,0x00},
{0x93,0x04},
#endif

//{{0x03, 0x0f}},

{0xe0, 0x04},
{0xc0, 0xc8},
{0xc1, 0x96},
{0x86, 0x3d},
{0x50, 0x89},
{0x51, 0x90},
{0x52, 0x2c},
{0x53, 0x00},
{0x54, 0x00},
{0x55, 0x88},
{0x57, 0x00},
{0x5a, 0xa0},
{0x5b, 0x78},
{0x5c, 0x00},
{0xd3, 0x82},
{0xe0, 0x00},
#endif
};

static struct regval_list sensor_uxga_regs[] = {    //1600*1200
#if 0
{0xff,0x01},
{0x12,0x00},
{0x11,0x00},
{0x3d,0x38},
{0x17,0x11},
{0x18,0x75},
{0x19,0x01},
{0x1a,0x97},
{0x32,0x36},
{0x37,0x40},
{0x4f,0x7c},
{0x50,0x67},
{0x5a,0x9b},
{0x6d,0x80},
{0x3d,0x38},
{0x39,0x02},
{0x35,0x88},
{0x22,0x0a},
{0x37,0x40},
{0x23,0x00},
{0x34,0xa0},
{0x36,0x1a},
{0x06,0x02},
{0x07,0xc0},
{0x0d,0xb7},
{0x0e,0x01},
{0x4c,0x00},
{0xff,0x00},
{0xe0,0x04},
{0xc0,0xc8},
{0xc1,0x96},
{0x86,0x3d},
{0x50,0x89},
{0x51,0x90},
{0x52,0x2c},
{0x53,0x00},
{0x54,0x00},
{0x55,0x88},
{0x57,0x00},
{0x5a,0xa0},
{0x5b,0x78},
{0x5c,0x00},
{0xd3,0x82},
{0xe0,0x00},
#else
{0xff,0x00},
{0xe0,0x04},
{0xc0,0xc8},
{0xc1,0x96},
{0x86,0x3d},
{0x50,0x00},
{0x51,0x90},
{0x52,0x2c},
{0x53,0x00},
{0x54,0x00},
{0x55,0x88},
{0x57,0x00},
{0x5a,0x90},
{0x5b,0x2c},
{0x5c,0x05},
{0xd3,0x82},
{0xe0,0x00},
#endif
};


static struct regval_list sensor_sxga_regs[] = { //1280*960

};

static struct regval_list sensor_xga_regs[] = { //XGA: 1024*768
{0xff,0x00},
{0xe0,0x04},
{0xc0,0xc8},
{0xc1,0x96},
{0x86,0x3d},
{0x50,0x00},
{0x51,0x90},
{0x52,0x2c},
{0x53,0x00},
{0x54,0x00},
{0x55,0x88},
{0x57,0x00},
{0x5a,0x90},
{0x5b,0x2c},
{0x5c,0x05},
{0xd3,0x82},
{0xe0,0x00},

};


static struct regval_list sensor_720p_regs[] = { //1280*720
#if 0
#if 0
{0x0f,0x14},
{0x10,0x0a},
{0x11,0x01},

{0x29,0x06},//;dummy pixels //24.75M 0x29,0x06//24M 0x29,0x06,//
{0x2a,0x40},                //24.75M 0x2a,0x72//24M 0x2a,0x40,//
{0x2b,0x02},//;dummy lines  //24.75M 0x2b,0x02//24M 0x2b,0x02,//
{0x2c,0xee},                //24.75M 0x2c,0xee//24M 0x2c,0xee,//
//for 25fps 0x2ee*1.2=0x384

{0x1c,0x25},//vsync width
{0x1d,0x02},//0x04,//0x02,
{0x1e,0x00},
{0x1f,0xe1},

{0xde,0x7c},
{0x3d,0x08},
{0xff,0xff},//delay 255ms
{0xff,0xff},//delay 255ms
#else
{0xfe,0x00},
{0x05,0x01},
{0x06,0x9e},
{0x07,0x01},
{0x08,0x6d},
{0x0a,0xf0}, //row start
{0x0c,0xa0}, //col start
{0x0d,0x02},
{0x0e,0xd8},
{0x0f,0x05}, //Window setting
{0x10,0x18},

{0xfe,0x01},
{0x27,0x00},
{0x28,0xd9},
{0x29,0x04},
{0x2a,0x3d},//18fps
{0x2b,0x06},
{0x2c,0xc8},//12.5fps
{0x2d,0x0a},
{0x2e,0x2c},//8fps
{0x3e,0x40},//0x40 0x00

//measure window
{0xfe,0x00},
{0xec,0x04},
{0xed,0x04},
{0xee,0x50},
{0xef,0x58},




{0x90,0x01},  //crop enable
{0x95,0x02},
{0x96,0xd0},
{0x97,0x05},
{0x98,0x00},


{0xfe,0x03},
{0x42,0x80},
{0x43,0x06}, //output buf width
{0x41,0x00}, // delay
{0x40,0x00}, //fifo half full trig
{0x17,0x01}, //widv
{0xfe,0x00},


{0x99,0x11},
{0xc8,0x00},

{0xfa,0x11},
{0xff,0xff},//delay 255ms
#endif
#endif
#if 0
{0xff,0x01},
{0x12,0x00},
{0x11,0x00},
{0x3d,0x38},
{0x17,0x11},
{0x18,0x75},
{0x19,0x01},
{0x1a,0x97},
{0x32,0x36},
{0x37,0x40},
{0x4f,0x7c},
{0x50,0x67},
{0x5a,0x9b},
{0x6d,0x80},
{0x3d,0x38},
{0x39,0x02},
{0x35,0x88},
{0x22,0x0a},
{0x37,0x40},
{0x23,0x00},
{0x34,0xa0},
{0x36,0x1a},
{0x06,0x02},
{0x07,0xc0},
{0x0d,0xb7},
{0x0e,0x01},
{0x4c,0x00},
#endif

{0xff,0x00},
{0xe0,0x04},
{0xc0,0xc8},
{0xc1,0x96},
{0x86,0x3d},
{0x50,0x00},
{0x51,0x90},
{0x52,0x2c},
{0x53,0x00},
{0x54,0x00},
{0x55,0x88},
{0x57,0x00},
{0x5a,0x40},
{0x5b,0xb4},
{0x5c,0x01},
{0xd3,0x82},
{0xe0,0x00},

};

static struct regval_list sensor_svga_regs[] = { //SVGA: 800*600
#if 0
{0xff,0x01},
{0x12,0x00},
{0x11,0x00},
{0x3d,0x38},
{0x17,0x11},
{0x18,0x75},
{0x19,0x01},
{0x1a,0x97},
{0x32,0x36},
{0x37,0x40},
{0x4f,0x7c},
{0x50,0x67},
{0x5a,0x9b},
{0x6d,0x80},
{0x3d,0x38},
{0x39,0x02},
{0x35,0x88},
{0x22,0x0a},
{0x37,0x40},
{0x23,0x00},
{0x34,0xa0},
{0x36,0x1a},
{0x06,0x02},
{0x07,0xc0},
{0x0d,0xb7},
{0x0e,0x01},
{0x4c,0x00},
{0xff,0x00},
{0xe0,0x04},
{0xc0,0xc8},
{0xc1,0x96},
{0x86,0x35},
{0x50,0x89},
{0x51,0x90},
{0x52,0x2c},
{0x53,0x00},
{0x54,0x00},
{0x55,0x88},
{0x57,0x00},
{0x5a,0xc8},
{0x5b,0x96},
{0x5c,0x00},
{0xd3,0x82},
{0xe0,0x00},
#else
#if 0
{0xff,0x01},
{0x12,0x00},
{0x11,0x00},
{0x3d,0x38},
{0x17,0x11},
{0x18,0x75},
{0x19,0x01},
{0x1a,0x97},
{0x32,0x36},
{0x37,0x40},
{0x4f,0x7c},
{0x50,0x67},
{0x5a,0x9b},
{0x6d,0x80},
{0x3d,0x38},
{0x39,0x02},
{0x35,0x88},
{0x22,0x0a},
{0x37,0x40},
{0x23,0x00},
{0x34,0xa0},
{0x36,0x1a},
{0x06,0x02},
{0x07,0xc0},
{0x0d,0xb7},
{0x0e,0x01},
{0x4c,0x00},
#endif
{0xff,0x00},
{0xe0,0x04},
{0xc0,0xc8},
{0xc1,0x96},
{0x86,0x35},
{0x50,0x89},
{0x51,0x90},
{0x52,0x2c},
{0x53,0x00},
{0x54,0x00},
{0x55,0x88},
{0x57,0x00},
{0x5a,0xc8},
{0x5b,0x96},
{0x5c,0x00},
{0xd3,0x82},
{0xe0,0x00},

#endif
};

static struct regval_list sensor_vga_regs[] = { //VGA:  640*480
#if 0
{0xff,0x00},
{0xe0,0x04},
{0xc0,0xc8},
{0xc1,0x96},
{0x86,0x3d},
{0x50,0x89},
{0x51,0x90},
{0x52,0x2c},
{0x53,0x00},
{0x54,0x00},
{0x55,0x88},
{0x57,0x00},
{0x5a,0xa0},
{0x5b,0x78},
{0x5c,0x00},
{0xd3,0x82},
{0xe0,0x00},
#else
{0xe0, 0x04},
{0xc0, 0xc8},
{0xc1, 0x96},
{0x86, 0x3d},
{0x50, 0x89},
{0x51, 0x90},
{0x52, 0x2c},
{0x53, 0x00},
{0x54, 0x00},
{0x55, 0x88},
{0x57, 0x00},
{0x5a, 0xa0},
{0x5b, 0x78},
{0x5c, 0x00},
{0xd3, 0x82},
{0xe0, 0x00},
#endif
};

//misc
static struct regval_list sensor_oe_disable_regs[] = {
	{0xc3,0x00},
	{0xc4,0x00},
};

static struct regval_list sensor_oe_enable_regs[] = {
};

static struct regval_list sensor_sw_stby_on_regs[] = {
};

static struct regval_list sensor_sw_stby_off_regs[] = {
};


/*
 * The white balance settings
 * Here only tune the R G B channel gain.
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
 /*
static struct regval_list sensor_wb_manual[] = {
};
*/
static struct regval_list sensor_wb_auto_regs[] = {
    {0xff, 0x00},  //AWB auto, bit[1]:0,auto
    {0xc7, 0x00},

};

static struct regval_list sensor_wb_incandescence_regs[] = {
   {0xff, 0x00},
    {0xc7, 0x40},
    {0xcc, 0x42},
    {0xcd, 0x3f},
    {0xce, 0x71},

};

static struct regval_list sensor_wb_fluorescent_regs[] = {
   {0xff, 0x00},
    {0xc7, 0x40},
    {0xcc, 0x52},
    {0xcd, 0x41},
    {0xce, 0x66},
};

static struct regval_list sensor_wb_tungsten_regs[] = {
    {0xff, 0x00},
    {0xc7, 0x40},
    {0xcc, 0x52},
    {0xcd, 0x41},
    {0xce, 0x66},

};
/*
static struct regval_list sensor_wb_horizon[] = {
//null
};
*/
static struct regval_list sensor_wb_daylight_regs[] = {
	{0xff, 0x00},
    {0xc7, 0x40},
    {0xcc, 0x5e},
    {0xcd, 0x41},
    {0xce, 0x54},

};
/*
static struct regval_list sensor_wb_flash[] = {
//null
};
*/
static struct regval_list sensor_wb_cloud_regs[] = {
	{0xff, 0x00},
    {0xc7, 0x40},
    {0xcc, 0x65},
    {0xcd, 0x41},
    {0xce, 0x4f},

};

/*static struct regval_list sensor_wb_shade[] = {
//null
};

static struct cfg_array sensor_wb[] = {
  {
  	.regs = sensor_wb_manual,             //V4L2_WHITE_BALANCE_MANUAL
    .size = ARRAY_SIZE(sensor_wb_manual),
  },
  {
  	.regs = sensor_wb_auto_regs,          //V4L2_WHITE_BALANCE_AUTO
    .size = ARRAY_SIZE(sensor_wb_auto_regs),
  },
  {
  	.regs = sensor_wb_incandescence_regs, //V4L2_WHITE_BALANCE_INCANDESCENT
    .size = ARRAY_SIZE(sensor_wb_incandescence_regs),
  },
  {
  	.regs = sensor_wb_fluorescent_regs,   //V4L2_WHITE_BALANCE_FLUORESCENT
    .size = ARRAY_SIZE(sensor_wb_fluorescent_regs),
  },
  {
  	.regs = sensor_wb_tungsten_regs,      //V4L2_WHITE_BALANCE_FLUORESCENT_H
    .size = ARRAY_SIZE(sensor_wb_tungsten_regs),
  },
  {
  	.regs = sensor_wb_horizon,            //V4L2_WHITE_BALANCE_HORIZON
    .size = ARRAY_SIZE(sensor_wb_horizon),
  },
  {
  	.regs = sensor_wb_daylight_regs,      //V4L2_WHITE_BALANCE_DAYLIGHT
    .size = ARRAY_SIZE(sensor_wb_daylight_regs),
  },
  {
  	.regs = sensor_wb_flash,              //V4L2_WHITE_BALANCE_FLASH
    .size = ARRAY_SIZE(sensor_wb_flash),
  },
  {
  	.regs = sensor_wb_cloud_regs,         //V4L2_WHITE_BALANCE_CLOUDY
    .size = ARRAY_SIZE(sensor_wb_cloud_regs),
  },
  {
  	.regs = sensor_wb_shade,              //V4L2_WHITE_BALANCE_SHADE
    .size = ARRAY_SIZE(sensor_wb_shade),
  },
//  {
//  	.regs = NULL,
//    .size = 0,
//  },
};
 */

/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
    {0x7c,0x00},
	{0x7d,0x06},

};

static struct regval_list sensor_colorfx_bw_regs[] = {
	{0x7c,0x00},
	{0x7d,0x20},
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
	{0x7c,0x00},
	{0x7d,0x18},
	{0x7c,0x05},
	{0x7d,0x40},
	{0x7d,0xA0},
};

static struct regval_list sensor_colorfx_negative_regs[] = {
    {0x7c,0x00},
	{0x7d,0x40},

};

static struct regval_list sensor_colorfx_emboss_regs[] = {
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
    {0x7c,0x00},
	{0x7d,0x18},
	{0x7c,0x05},
	{0x7d,0xa0},
	{0x7d,0x40},

};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
	{0x7c,0x00},
	{0x7d,0x18},
	{0x7c,0x05},
	{0x7d,0x40},
	{0x7d,0x40},

};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
//NULL
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
//NULL
};
/*
static struct regval_list sensor_colorfx_aqua_regs[] = {
//null
};

static struct regval_list sensor_colorfx_art_freeze_regs[] = {
//null
};

static struct regval_list sensor_colorfx_silhouette_regs[] = {
//null
};

static struct regval_list sensor_colorfx_solarization_regs[] = {
//null
};

static struct regval_list sensor_colorfx_antique_regs[] = {
//null
};

static struct regval_list sensor_colorfx_set_cbcr_regs[] = {
//null
};*/
/*
static struct cfg_array sensor_colorfx[] = {
  {
  	.regs = sensor_colorfx_none_regs,         //V4L2_COLORFX_NONE = 0,
    .size = ARRAY_SIZE(sensor_colorfx_none_regs),
  },
  {
  	.regs = sensor_colorfx_bw_regs,           //V4L2_COLORFX_BW   = 1,
    .size = ARRAY_SIZE(sensor_colorfx_bw_regs),
  },
  {
  	.regs = sensor_colorfx_sepia_regs,        //V4L2_COLORFX_SEPIA  = 2,
    .size = ARRAY_SIZE(sensor_colorfx_sepia_regs),
  },
  {
  	.regs = sensor_colorfx_negative_regs,     //V4L2_COLORFX_NEGATIVE = 3,
    .size = ARRAY_SIZE(sensor_colorfx_negative_regs),
  },
  {
  	.regs = sensor_colorfx_emboss_regs,       //V4L2_COLORFX_EMBOSS = 4,
    .size = ARRAY_SIZE(sensor_colorfx_emboss_regs),
  },
  {
  	.regs = sensor_colorfx_sketch_regs,       //V4L2_COLORFX_SKETCH = 5,
    .size = ARRAY_SIZE(sensor_colorfx_sketch_regs),
  },
  {
  	.regs = sensor_colorfx_sky_blue_regs,     //V4L2_COLORFX_SKY_BLUE = 6,
    .size = ARRAY_SIZE(sensor_colorfx_sky_blue_regs),
  },
  {
  	.regs = sensor_colorfx_grass_green_regs,  //V4L2_COLORFX_GRASS_GREEN = 7,
    .size = ARRAY_SIZE(sensor_colorfx_grass_green_regs),
  },
  {
  	.regs = sensor_colorfx_skin_whiten_regs,  //V4L2_COLORFX_SKIN_WHITEN = 8,
    .size = ARRAY_SIZE(sensor_colorfx_skin_whiten_regs),
  },
  {
  	.regs = sensor_colorfx_vivid_regs,        //V4L2_COLORFX_VIVID = 9,
    .size = ARRAY_SIZE(sensor_colorfx_vivid_regs),
  },
  {
  	.regs = sensor_colorfx_aqua_regs,         //V4L2_COLORFX_AQUA = 10,
    .size = ARRAY_SIZE(sensor_colorfx_aqua_regs),
  },
  {
  	.regs = sensor_colorfx_art_freeze_regs,   //V4L2_COLORFX_ART_FREEZE = 11,
    .size = ARRAY_SIZE(sensor_colorfx_art_freeze_regs),
  },
  {
  	.regs = sensor_colorfx_silhouette_regs,   //V4L2_COLORFX_SILHOUETTE = 12,
    .size = ARRAY_SIZE(sensor_colorfx_silhouette_regs),
  },
  {
  	.regs = sensor_colorfx_solarization_regs, //V4L2_COLORFX_SOLARIZATION = 13,
    .size = ARRAY_SIZE(sensor_colorfx_solarization_regs),
  },
  {
  	.regs = sensor_colorfx_antique_regs,      //V4L2_COLORFX_ANTIQUE = 14,
    .size = ARRAY_SIZE(sensor_colorfx_antique_regs),
  },
  {
  	.regs = sensor_colorfx_set_cbcr_regs,     //V4L2_COLORFX_SET_CBCR = 15,
    .size = ARRAY_SIZE(sensor_colorfx_set_cbcr_regs),
  },
};
*/
/*
static struct regval_list sensor_sharpness_auto_regs[] = {
};
static struct regval_list sensor_denoise_auto_regs[] = {
};
*/
/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_zero_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_brightness_pos4_regs[] = {
//NULL
};
/*
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
*/
/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_zero_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_contrast_pos4_regs[] = {
//NULL
};
/*
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
*/
/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg3_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg2_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_neg1_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_zero_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos1_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos2_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos3_regs[] = {
//NULL
};

static struct regval_list sensor_saturation_pos4_regs[] = {
//NULL
};
/*
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
*/
/*
 * The exposure target setttings
 */
static struct regval_list sensor_ev_neg4_regs[] = {
    {0xff,0x00},//register page
	{0x7c,0x00},
	{0x7d,0x04},
	{0x7c,0x09},
	{0x7d,0x28},//brightness value
    {0x7d,0x0e},//sign
};

static struct regval_list sensor_ev_neg3_regs[] = {
    {0xff,0x00},//register page
	{0x7c,0x00},
	{0x7d,0x04},
	{0x7c,0x09},
	{0x7d,0x20},//brightness value
    {0x7d,0x0e},//sign
};

static struct regval_list sensor_ev_neg2_regs[] = {
    {0xff,0x00},//register page
	{0x7c,0x00},
	{0x7d,0x04},
	{0x7c,0x09},
	{0x7d,0x18},//brightness value
    {0x7d,0x0e},//sign
};

static struct regval_list sensor_ev_neg1_regs[] = {
    {0xff,0x00},//register page
	{0x7c,0x00},
	{0x7d,0x04},
	{0x7c,0x09},
	{0x7d,0x10},//brightness value
    {0x7d,0x0e},//sign
};

static struct regval_list sensor_ev_zero_regs[] = {
    {0xff,0x00},//register page
	{0x7c,0x00},
	{0x7d,0x04},
	{0x7c,0x09},
	{0x7d,0x00},//brightness value
    {0x7d,0x00},//sign
};

static struct regval_list sensor_ev_pos1_regs[] = {
    {0xff,0x00},//register page
	{0x7c,0x00},
	{0x7d,0x04},
	{0x7c,0x09},
	{0x7d,0x00},//brightness value
    {0x7d,0x00},//sign
};

static struct regval_list sensor_ev_pos2_regs[] = {
    {0xff,0x00},//register page
	{0x7c,0x00},
	{0x7d,0x04},
	{0x7c,0x09},
	{0x7d,0x10},//brightness value
    {0x7d,0x00},//sign
};

static struct regval_list sensor_ev_pos3_regs[] = {
    {0xff,0x00},//register page
	{0x7c,0x00},
	{0x7d,0x04},
	{0x7c,0x09},
	{0x7d,0x20},//brightness value
    {0x7d,0x00},//sign
};

static struct regval_list sensor_ev_pos4_regs[] = {
    {0xff,0x00},//register page
	{0x7c,0x00},
	{0x7d,0x04},
	{0x7c,0x09},
	{0x7d,0x30},//brightness value
    {0x7d,0x00},//sign
};
/*
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
*/
/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{0xff,0x00},
	{0xda,0x01},
};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {
{0xff,0x00},
{0xda,0x01},
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
{0xff,0x00},
{0xda,0x01},
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
{0xff,0x00},
{0xda,0x01},
};

//static struct regval_list sensor_fmt_raw[] = {
//
//};

/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
static int sensor_read(struct v4l2_subdev *sd, unsigned short reg,
    unsigned char *value)
{
	int ret=0;
	int cnt=0;

  ret = cci_read_a8_d8(sd,reg,value);
  while(ret!=0&&cnt<2)
  {
  	ret = cci_read_a8_d8(sd,reg,value);
  	cnt++;
  }
  if(cnt>0)
  	vfe_dev_dbg("sensor read retry=%d\n",cnt);

  return ret;
}

static int sensor_write(struct v4l2_subdev *sd, unsigned short reg,
    unsigned char value)
{
	int ret=0;
	int cnt=0;
  ret = cci_write_a8_d8(sd,reg,value);
  while(ret!=0&&cnt<2)
  {
  	ret = cci_write_a8_d8(sd,reg,value);
  	cnt++;
  }
  if(cnt>0)
  	vfe_dev_dbg("sensor write retry=%d\n",cnt);

  return ret;
}

/*
 * Write a list of register settings;
 */
static int sensor_write_array(struct v4l2_subdev *sd, struct regval_list *regs, int array_size)
{
	int i=0;

  if(!regs)
  	return -EINVAL;

  while(i<array_size)
  {
    if(regs->addr == REG_DLY) {
      msleep(regs->data);
    }
    else {
    	//printk("write 0x%x=0x%x\n", regs->addr, regs->data);
      LOG_ERR_RET(sensor_write(sd, regs->addr, regs->data))
    }
    i++;
    regs++;
  }
  return 0;
}


/* stuff about exposure when capturing image and video*/
//static int sensor_s_denoise_value(struct v4l2_subdev *sd, unsigned char value);
unsigned char ogain,oexposurelow,oexposuremid,oexposurehigh;
unsigned int preview_exp_line,preview_fps;
unsigned long preview_pclk;

/*
static int sensor_get_pclk(struct v4l2_subdev *sd)
{
  unsigned long pclk;
  unsigned char pre_div,mul,sys_div,pll_rdiv,bit_div,sclk_rdiv;

  sensor_read(sd, 0x3037, &pre_div);
  pre_div = pre_div & 0x0f;

  if(pre_div == 0)
    pre_div = 1;

  sensor_read(sd, 0x3036, &mul);
  if(mul < 128)
    mul = mul;
  else
    mul = mul/2*2;

  sensor_read(sd, 0x3035, &sys_div);
  sys_div = (sys_div & 0xf0) >> 4;

  sensor_read(sd, 0x3037, &pll_rdiv);
  pll_rdiv = (pll_rdiv & 0x10) >> 4;
  pll_rdiv = pll_rdiv + 1;

  sensor_read(sd, 0x3034, &bit_div);
  bit_div = (bit_div & 0x0f);

  sensor_read(sd, 0x3108, &sclk_rdiv);
  sclk_rdiv = (sclk_rdiv & 0x03);
  sclk_rdiv = sclk_rdiv << sclk_rdiv;

  vfe_dev_dbg("pre_div = %d,mul = %d,sys_div = %d,pll_rdiv = %d,sclk_rdiv = %d\n",\
          pre_div,mul,sys_div,pll_rdiv,sclk_rdiv);

  if((pre_div&&sys_div&&pll_rdiv&&sclk_rdiv) == 0)
    return -EFAULT;

  if(bit_div == 8)
    pclk = MCLK / pre_div * mul / sys_div / pll_rdiv / 2 / sclk_rdiv;
  else if(bit_div == 10)
    pclk = MCLK / pre_div * mul / sys_div / pll_rdiv * 2 / 5 / sclk_rdiv;
  else
    pclk = MCLK / pre_div * mul / sys_div / pll_rdiv / 1 / sclk_rdiv;

  vfe_dev_dbg("pclk = %ld\n",pclk);

  preview_pclk = pclk;
  return 0;
}
*/
#if 0
static int sensor_get_fps(struct v4l2_subdev *sd)
{
  unsigned char vts_low,vts_high,hts_low,hts_high,vts_extra_high,vts_extra_low;
  unsigned long vts,hts,vts_extra;

  sensor_read(sd, 0x380c, &hts_high);
  sensor_read(sd, 0x380d, &hts_low);
  sensor_read(sd, 0x380e, &vts_high);
  sensor_read(sd, 0x380f, &vts_low);
  sensor_read(sd, 0x350c, &vts_extra_high);
  sensor_read(sd, 0x350d, &vts_extra_low);

  hts = hts_high * 256 + hts_low;
  vts = vts_high * 256 + vts_low;
  vts_extra = vts_extra_high * 256 + vts_extra_low;

  if((hts&&(vts+vts_extra)) == 0)
    return -EFAULT;

  if(sensor_get_pclk(sd))
    vfe_dev_err("get pclk error!\n");

  preview_fps = preview_pclk / ((vts_extra+vts) * hts);
  vfe_dev_dbg("preview fps = %d\n",preview_fps);

  return 0;
}

//static void sensor_s_ae_ratio(struct work_struct *work)
//{
//  vfe_dev_dbg("sensor_s_ae_ratio\n");
//  sensor_write(glb_sd, 0x3a05, 0x30);//normal aec ratio
//}


static int sensor_g_3a_lock(struct v4l2_subdev *sd)
{
  //int ret=0;
  struct sensor_info *info = to_state(sd);
  return ( (info->auto_focus==0)?V4L2_LOCK_FOCUS:~V4L2_LOCK_FOCUS |
           (info->autowb==0)?V4L2_LOCK_WHITE_BALANCE:~V4L2_LOCK_WHITE_BALANCE |
           (~V4L2_LOCK_EXPOSURE)
         );
}
#endif



#if 0
static int sensor_s_sharpness_auto(struct v4l2_subdev *sd)
{
	unsigned char rdval;
	sensor_read(sd,0x5308,&rdval);
	sensor_write(sd,0x5308,rdval&0xbf); //bit6 is sharpness manual enable
	return sensor_write_array(sd, sensor_sharpness_auto_regs ,ARRAY_SIZE(sensor_sharpness_auto_regs));
}

static int sensor_s_sharpness_value(struct v4l2_subdev *sd, unsigned char value)
{
	unsigned char rdval;
	sensor_read(sd,0x5308,&rdval);
	sensor_write(sd,0x5308,rdval|0x40); //bit6 is sharpness manual enable
  return sensor_write(sd,0x5302,value);
}
#endif

#if 0
static int sensor_s_denoise_auto(struct v4l2_subdev *sd)
{
	unsigned char rdval;
	sensor_read(sd,0x5308,&rdval);
	sensor_write(sd,0x5308,rdval&0xef); //bit4 is denoise manual enable
	return sensor_write_array(sd, sensor_denoise_auto_regs ,ARRAY_SIZE(sensor_denoise_auto_regs));
}


static int sensor_s_denoise_value(struct v4l2_subdev *sd, unsigned char value)
{
	unsigned char rdval;
	sensor_read(sd,0x5308,&rdval);
	sensor_write(sd,0x5308,rdval|0x10); //bit4 is denoise manual enable
	return sensor_write(sd,0x5306,value);
}
#endif



/* *********************************************begin of ******************************************** */

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	//struct regval_list regs;
	unsigned char val;

//	 regs.reg_num[0] = 0xFF;
//	 regs.value[0] = 0x01;
	 ret = sensor_write(sd, 0xFF, 0x01);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}
//	regs.reg_num[0] = 0x04;
	ret = sensor_read(sd, 0x04, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_g_hflip!\n");
		return ret;
	}

	val &= (1<<7);
	val = val>>5;		//0x12 bit5 is mirror

	*value = val;

	info->hflip = *value;
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{

	int ret;
	struct sensor_info *info = to_state(sd);
	//struct regval_list regs;
	unsigned char val;
        vfe_dev_dbg("%s,---------------------%d\n",__func__,value);

	//regs.reg_num[0] = 0xFF;
	// regs.value[0] = 0x01;
	 ret = sensor_write(sd, 0xFF, 0x01);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}

	//regs.reg_num[0] = 0x04;

	ret = sensor_read(sd, 0x04, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_s_hflip!\n");
		return ret;
	}
       val&= ~0x80;
	switch (value) {
		case 0:
		  val |= 0x00;
			break;
		case 1:
			val |= 0x80;
			break;
		default:
			return -EINVAL;
	}

	ret = sensor_write(sd, 0x04, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_hflip!\n");
		return ret;
	}

	msleep(50);

	info->hflip = value;

	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{

	int ret;
	struct sensor_info *info = to_state(sd);
	//struct regval_list regs;
	unsigned char val;

	 //regs.reg_num[0] = 0xFF;
	 //regs.value[0] = 0x01;
	 ret = sensor_write(sd, 0xFF, 0x01);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}
	//regs.reg_num[0] = 0x04;
	ret = sensor_read(sd, 0x04, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_g_vflip!\n");
		return ret;
	}

	val &= (1<<4);
	val = val>>4;		//0x12 bit4 is upsidedown

	*value = val;

	info->vflip = *value;
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{

	int ret;
	struct sensor_info *info = to_state(sd);
	//struct regval_list regs;
	unsigned char val;


        vfe_dev_dbg("%s,------------------%d\n",__func__,value);

	 //regs.reg_num[0] = 0xFF;
	 //regs.value[0] = 0x01;
	 ret = sensor_write(sd, 0xFF, 0x01);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}
	//regs.reg_num[0] = 0x04;

	ret = sensor_read(sd, 0x04, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_s_vflip!\n");
		return ret;
	}
	val&= ~0x50;
	switch (value) {
		case 0:
		  val |= 0x00;

			break;
		case 1:
			val |= 0x50;

			break;
		default:
			return -EINVAL;
	}

	ret = sensor_write(sd, 0x04, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_vflip!\n");
		return ret;
	}

	msleep(50);

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
	//struct regval_list regs;
	unsigned char val;

	//regs.reg_num[0] = 0x13;
	ret = sensor_read(sd, 0x13, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_g_autoexp!\n");
		return ret;
	}

	val &= 0x01;
	if (val == 0x01) {
		*value = V4L2_EXPOSURE_AUTO;
	}
	else
	{
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
	//struct regval_list regs;
	unsigned char val;

	//regs.reg_num[0] = 0x13;
	ret = sensor_read(sd, 0x13, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_s_autoexp!\n");
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

	ret = sensor_write(sd, 0x13, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_autoexp!\n");
		return ret;
	}

	msleep(50);

	info->autoexp = value;
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	//struct regval_list regs;
	unsigned char val;

	//regs.reg_num[0] = 0x13;
	ret = sensor_read(sd, 0x13, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_g_autowb!\n");
		return ret;
	}

	val &= (1<<2);
	val = val>>2;		//0x13 bit2 is awb enable

	*value = val;
	info->autowb = *value;

	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	//struct regval_list regs;
	unsigned char val;

	ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_autowb!\n");
		return ret;
	}

	//regs.reg_num[0] = 0xc7;
	ret = sensor_read(sd, 0xc7, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_s_autowb!\n");
		return ret;
	}

	switch(value) {
	case 0:
		val |= 0x40;
		break;
	case 1:
		val &=~0x40;
		break;
	default:
		break;
	}
	ret = sensor_write(sd, 0xc7, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}

	msleep(50);

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

/* *********************************************end of ******************************************** */

static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
  struct sensor_info *info = to_state(sd);

  *value = info->brightness;
  return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_brightness_neg4_regs, ARRAY_SIZE(sensor_brightness_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_brightness_neg3_regs, ARRAY_SIZE(sensor_brightness_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_brightness_neg2_regs, ARRAY_SIZE(sensor_brightness_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_brightness_neg1_regs, ARRAY_SIZE(sensor_brightness_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_brightness_zero_regs, ARRAY_SIZE(sensor_brightness_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_brightness_pos1_regs, ARRAY_SIZE(sensor_brightness_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_brightness_pos2_regs, ARRAY_SIZE(sensor_brightness_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_brightness_pos3_regs, ARRAY_SIZE(sensor_brightness_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_brightness_pos4_regs, ARRAY_SIZE(sensor_brightness_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_brightness!\n");
		return ret;
	}

	msleep(50);

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
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_contrast_neg4_regs, ARRAY_SIZE(sensor_contrast_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_contrast_neg3_regs, ARRAY_SIZE(sensor_contrast_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_contrast_neg2_regs, ARRAY_SIZE(sensor_contrast_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_contrast_neg1_regs, ARRAY_SIZE(sensor_contrast_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_contrast_zero_regs, ARRAY_SIZE(sensor_contrast_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_contrast_pos1_regs, ARRAY_SIZE(sensor_contrast_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_contrast_pos2_regs, ARRAY_SIZE(sensor_contrast_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_contrast_pos3_regs, ARRAY_SIZE(sensor_contrast_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_contrast_pos4_regs, ARRAY_SIZE(sensor_contrast_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_contrast!\n");
		return ret;
	}

	msleep(50);

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
	int ret;
	struct sensor_info *info = to_state(sd);

	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_saturation_neg4_regs, ARRAY_SIZE(sensor_saturation_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_saturation_neg3_regs, ARRAY_SIZE(sensor_saturation_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_saturation_neg2_regs, ARRAY_SIZE(sensor_saturation_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_saturation_neg1_regs, ARRAY_SIZE(sensor_saturation_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_saturation_zero_regs, ARRAY_SIZE(sensor_saturation_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_saturation_pos1_regs, ARRAY_SIZE(sensor_saturation_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_saturation_pos2_regs, ARRAY_SIZE(sensor_saturation_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_saturation_pos3_regs, ARRAY_SIZE(sensor_saturation_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_saturation_pos4_regs, ARRAY_SIZE(sensor_saturation_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_saturation!\n");
		return ret;
	}

	msleep(50);

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
	int ret;
	struct sensor_info *info = to_state(sd);
    vfe_dev_dbg("%s,%d:value = %d\n",__func__,__LINE__,value);
	switch (value) {
		case -4:
		  ret = sensor_write_array(sd, sensor_ev_neg4_regs, ARRAY_SIZE(sensor_ev_neg4_regs));
			break;
		case -3:
			ret = sensor_write_array(sd, sensor_ev_neg3_regs, ARRAY_SIZE(sensor_ev_neg3_regs));
			break;
		case -2:
			ret = sensor_write_array(sd, sensor_ev_neg2_regs, ARRAY_SIZE(sensor_ev_neg2_regs));
			break;
		case -1:
			ret = sensor_write_array(sd, sensor_ev_neg1_regs, ARRAY_SIZE(sensor_ev_neg1_regs));
			break;
		case 0:
			ret = sensor_write_array(sd, sensor_ev_zero_regs, ARRAY_SIZE(sensor_ev_zero_regs));
			break;
		case 1:
			ret = sensor_write_array(sd, sensor_ev_pos1_regs, ARRAY_SIZE(sensor_ev_pos1_regs));
			break;
		case 2:
			ret = sensor_write_array(sd, sensor_ev_pos2_regs, ARRAY_SIZE(sensor_ev_pos2_regs));
			break;
		case 3:
			ret = sensor_write_array(sd, sensor_ev_pos3_regs, ARRAY_SIZE(sensor_ev_pos3_regs));
			break;
		case 4:
			ret = sensor_write_array(sd, sensor_ev_pos4_regs, ARRAY_SIZE(sensor_ev_pos4_regs));
			break;
		default:
			return -EINVAL;
	}

	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_exp!\n");
		return ret;
	}

	msleep(50);

	info->exp = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
  struct sensor_info *info = to_state(sd);
  enum v4l2_auto_n_preset_white_balance *wb_type = (enum v4l2_auto_n_preset_white_balance*)value;

  *wb_type = info->wb;

  return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd,
    enum v4l2_auto_n_preset_white_balance value)
{
	int ret;
	struct sensor_info *info = to_state(sd);

        vfe_dev_dbg("func=%s,value=%d\n",__func__,value);
	if (value == V4L2_WHITE_BALANCE_AUTO) {

		 ret = sensor_s_autowb(sd, 1);
		  return ret;
	 }
     else {

		ret = sensor_s_autowb(sd, 0);
		if(ret < 0) {
			vfe_dev_err("sensor_s_autowb error, return %x!\n",ret);
			return ret;
		}

		switch (value) {
			case V4L2_WHITE_BALANCE_CLOUDY:
			  ret = sensor_write_array(sd, sensor_wb_cloud_regs, ARRAY_SIZE(sensor_wb_cloud_regs));
				break;
			case V4L2_WHITE_BALANCE_DAYLIGHT:
				ret = sensor_write_array(sd, sensor_wb_daylight_regs, ARRAY_SIZE(sensor_wb_daylight_regs));
				break;
			case V4L2_WHITE_BALANCE_INCANDESCENT:
				ret = sensor_write_array(sd, sensor_wb_incandescence_regs, ARRAY_SIZE(sensor_wb_incandescence_regs));
				break;
			case V4L2_WHITE_BALANCE_FLUORESCENT:
				ret = sensor_write_array(sd, sensor_wb_fluorescent_regs, ARRAY_SIZE(sensor_wb_fluorescent_regs));
				break;
			case V4L2_WHITE_BALANCE_FLUORESCENT_H:
				ret = sensor_write_array(sd, sensor_wb_tungsten_regs, ARRAY_SIZE(sensor_wb_tungsten_regs));
				break;
			default:
				return -EINVAL;
		}
	}

	if (ret < 0) {
		vfe_dev_err("sensor_s_wb error, return %x!\n",ret);
		return ret;
	}

	msleep(50);

	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd,
    __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx*)value;

  *clrfx_type = info->clrfx;
  return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd,
    enum v4l2_colorfx value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	switch (value) {
	case V4L2_COLORFX_NONE:
	  ret = sensor_write_array(sd, sensor_colorfx_none_regs, ARRAY_SIZE(sensor_colorfx_none_regs));
		break;
	case V4L2_COLORFX_BW:
		ret = sensor_write_array(sd, sensor_colorfx_bw_regs, ARRAY_SIZE(sensor_colorfx_bw_regs));
		break;
	case V4L2_COLORFX_SEPIA:
		ret = sensor_write_array(sd, sensor_colorfx_sepia_regs, ARRAY_SIZE(sensor_colorfx_sepia_regs));
		break;
	case V4L2_COLORFX_NEGATIVE:
		ret = sensor_write_array(sd, sensor_colorfx_negative_regs, ARRAY_SIZE(sensor_colorfx_negative_regs));
		break;
	case V4L2_COLORFX_EMBOSS:
		ret = sensor_write_array(sd, sensor_colorfx_emboss_regs, ARRAY_SIZE(sensor_colorfx_emboss_regs));
		break;
	case V4L2_COLORFX_SKETCH:
		ret = sensor_write_array(sd, sensor_colorfx_sketch_regs, ARRAY_SIZE(sensor_colorfx_sketch_regs));
		break;
	case V4L2_COLORFX_SKY_BLUE:
		ret = sensor_write_array(sd, sensor_colorfx_sky_blue_regs, ARRAY_SIZE(sensor_colorfx_sky_blue_regs));
		break;
	case V4L2_COLORFX_GRASS_GREEN:
		ret = sensor_write_array(sd, sensor_colorfx_grass_green_regs, ARRAY_SIZE(sensor_colorfx_grass_green_regs));
		break;
	case V4L2_COLORFX_SKIN_WHITEN:
		ret = sensor_write_array(sd, sensor_colorfx_skin_whiten_regs, ARRAY_SIZE(sensor_colorfx_skin_whiten_regs));
		break;
	case V4L2_COLORFX_VIVID:
		ret = sensor_write_array(sd, sensor_colorfx_vivid_regs, ARRAY_SIZE(sensor_colorfx_vivid_regs));
		break;
	default:
		return -EINVAL;
	}

	if (ret < 0) {
		vfe_dev_err("sensor_s_colorfx error, return %x!\n",ret);
		return ret;
	}

	msleep(50);

	info->clrfx = value;
	return 0;
}

static int sensor_g_flash_mode(struct v4l2_subdev *sd,
    __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  enum v4l2_flash_led_mode *flash_mode = (enum v4l2_flash_led_mode*)value;

  *flash_mode = info->flash_mode;
  return 0;
}

static int sensor_s_flash_mode(struct v4l2_subdev *sd,
    enum v4l2_flash_led_mode value)
{
	struct sensor_info *info = to_state(sd);
/*	struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	int flash_on,flash_off;

	flash_on = (dev->flash_pol!=0)?1:0;
	flash_off = (flash_on==1)?0:1;

	switch (value) {
	case V4L2_FLASH_MODE_OFF:
		csi_gpio_write(sd,&dev->flash_io,flash_off);
		break;
	case V4L2_FLASH_MODE_AUTO:
		return -EINVAL;
		break;
	case V4L2_FLASH_MODE_ON:
		csi_gpio_write(sd,&dev->flash_io,flash_on);
		break;
	case V4L2_FLASH_MODE_TORCH:
		return -EINVAL;
		break;
	case V4L2_FLASH_MODE_RED_EYE:
		return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	*/
	info->flash_mode = value;
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
  int ret;

  //insure that clk_disable() and clk_enable() are called in pair
  //when calling CSI_SUBDEV_STBY_ON/OFF and CSI_SUBDEV_PWR_ON/OFF
  ret = 0;
  switch(on)
  {
    case CSI_SUBDEV_STBY_ON:
      vfe_dev_dbg("CSI_SUBDEV_STBY_ON!\n");
      //software standby
      ret = sensor_write_array(sd, sensor_sw_stby_on_regs ,ARRAY_SIZE(sensor_sw_stby_on_regs));
      if(ret < 0)
        vfe_dev_err("soft stby falied!\n");
      msleep(10);
      //disable io oe
      vfe_dev_print("disalbe oe!\n");
      ret = sensor_write_array(sd, sensor_oe_disable_regs, ARRAY_SIZE(sensor_oe_disable_regs));
      if(ret < 0)
        vfe_dev_err("disalbe oe falied!\n");
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      cci_lock(sd);
      //standby on io
      vfe_gpio_write(sd,PWDN,CSI_STBY_ON);
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      cci_unlock(sd);
      //inactive mclk after stadby in
      vfe_set_mclk(sd,OFF);
      break;
    case CSI_SUBDEV_STBY_OFF:
      vfe_dev_dbg("CSI_SUBDEV_STBY_OFF!\n");
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      cci_lock(sd);
      //active mclk before stadby out
      vfe_set_mclk_freq(sd,MCLK);
      vfe_set_mclk(sd,ON);
      msleep(10);
      //standby off io
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
      msleep(10);
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      cci_unlock(sd);
      vfe_dev_print("enable oe!\n");
      ret = sensor_write_array(sd, sensor_oe_enable_regs,  ARRAY_SIZE(sensor_oe_enable_regs));
      if(ret < 0)
        vfe_dev_err("enable oe falied!\n");
      //software standby
      ret = sensor_write_array(sd, sensor_sw_stby_off_regs ,ARRAY_SIZE(sensor_sw_stby_off_regs));
      if(ret < 0)
        vfe_dev_err("soft stby off falied!\n");
      msleep(10);
      break;
    case CSI_SUBDEV_PWR_ON:
      vfe_dev_dbg("CSI_SUBDEV_PWR_ON!\n");
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      cci_lock(sd);
      //power on reset
      vfe_gpio_set_status(sd,PWDN,1);//set the gpio to output
      vfe_gpio_set_status(sd,RESET,1);//set the gpio to output
      //power down io
      vfe_gpio_write(sd,PWDN,CSI_STBY_ON);
      //reset on io
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
      msleep(1);
      //active mclk before power on
      vfe_set_mclk_freq(sd,MCLK);
      vfe_set_mclk(sd,ON);
      msleep(10);
      //power supply
      vfe_gpio_write(sd,POWER_EN,CSI_PWR_ON);
      vfe_set_pmu_channel(sd,IOVDD,ON);
      vfe_set_pmu_channel(sd,AVDD,ON);
      vfe_set_pmu_channel(sd,DVDD,ON);
      vfe_set_pmu_channel(sd,AFVDD,ON);
      //standby off io
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
      msleep(10);
      //reset after power on
      vfe_gpio_write(sd,RESET,CSI_RST_OFF);
      msleep(30);
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      cci_unlock(sd);
      break;
    case CSI_SUBDEV_PWR_OFF:
      vfe_dev_dbg("CSI_SUBDEV_PWR_OFF!\n");
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      cci_lock(sd);
      //inactive mclk before power off
      vfe_set_mclk(sd,OFF);
      //power supply off
      vfe_gpio_write(sd,POWER_EN,CSI_PWR_OFF);
      vfe_set_pmu_channel(sd,AFVDD,OFF);
      vfe_set_pmu_channel(sd,DVDD,OFF);
      vfe_set_pmu_channel(sd,AVDD,OFF);
      vfe_set_pmu_channel(sd,IOVDD,OFF);
      //standby and reset io
      msleep(10);
      vfe_gpio_write(sd,POWER_EN,CSI_STBY_OFF);
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
      //set the io to hi-z
      vfe_gpio_set_status(sd,RESET,0);//set the gpio to input
      vfe_gpio_set_status(sd,PWDN,0);//set the gpio to input
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      cci_unlock(sd);
      break;
    default:
      return -EINVAL;
  }

  return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
  switch(val)
  {
    case 0:
      vfe_gpio_write(sd,RESET,CSI_RST_OFF);
      msleep(10);
      break;
    case 1:
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
      msleep(10);
      break;
    default:
      return -EINVAL;
  }

  return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
  unsigned char rdval;
  LOG_ERR_RET(sensor_write(sd, 0xff, 0x01))
  LOG_ERR_RET(sensor_read(sd, 0x0a, &rdval))

  if(rdval != 0x26)
    return -ENODEV;

  LOG_ERR_RET(sensor_read(sd, 0x0b, &rdval))
  printk("%s,%d:rdval = 0x%x\n",__func__,__LINE__,rdval);
  if(rdval != 0x42)
    return -ENODEV;

  return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
  int ret;
  struct sensor_info *info = to_state(sd);

  vfe_dev_dbg("sensor_init 0x%x\n",val);

  /*Make sure it is a target sensor*/
  ret = sensor_detect(sd);
  if (ret) {
    vfe_dev_err("chip found is not an target chip.\n");
    return ret;
  }

  vfe_get_standby_mode(sd,&info->stby_mode);

  if((info->stby_mode == HW_STBY || info->stby_mode == SW_STBY) \
      && info->init_first_flag == 0) {
    vfe_dev_print("stby_mode and init_first_flag = 0\n");
    return 0;
  }

  info->focus_status = 0;
  info->low_speed = 0;
  info->width = 0;
  info->height = 0;
  info->brightness = 0;
  info->contrast = 0;
  info->saturation = 0;
  info->hue = 0;
  info->hflip = 0;
  info->vflip = 0;
  info->gain = 0;
  info->autogain = 1;
  info->exp_bias = 0;
  info->autoexp = 1;
  info->autowb = 1;
  info->wb = V4L2_WHITE_BALANCE_AUTO;
  info->clrfx = V4L2_COLORFX_NONE;
  info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;

//  info->af_ctrl = V4L2_AF_RELEASE;
  info->tpf.numerator = 1;
  info->tpf.denominator = 30;    /* 30fps */

  ret = sensor_write_array(sd, sensor_default_regs, ARRAY_SIZE(sensor_default_regs));
  if(ret < 0) {
    vfe_dev_err("write sensor_default_regs error\n");
    return ret;
  }

  //sensor_s_band_filter(sd, V4L2_CID_POWER_LINE_FREQUENCY_50HZ);

  if(info->stby_mode == 0)
    info->init_first_flag = 0;

  info->preview_first_flag = 1;
//  INIT_DELAYED_WORK(&sensor_s_ae_ratio_work, sensor_s_ae_ratio);

  return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
  int ret=0;
  return ret;
}


/*
 * Store information about the video data format.
 */
static struct sensor_format_struct {
  __u8 *desc;
  //__u32 pixelformat;
  enum v4l2_mbus_pixelcode mbus_code;
  struct regval_list *regs;
  int regs_size;
  int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
  {
    .desc   = "YUYV 4:2:2",
    .mbus_code  = V4L2_MBUS_FMT_YUYV8_2X8,
    .regs     = sensor_fmt_yuv422_yuyv,
    .regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
    .bpp    = 2,
  },
  {
    .desc   = "YVYU 4:2:2",
    .mbus_code  = V4L2_MBUS_FMT_YVYU8_2X8,
    .regs     = sensor_fmt_yuv422_yvyu,
    .regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
    .bpp    = 2,
  },
  {
    .desc   = "UYVY 4:2:2",
    .mbus_code  = V4L2_MBUS_FMT_UYVY8_2X8,
    .regs     = sensor_fmt_yuv422_uyvy,
    .regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
    .bpp    = 2,
  },
  {
    .desc   = "VYUY 4:2:2",
    .mbus_code  = V4L2_MBUS_FMT_VYUY8_2X8,
    .regs     = sensor_fmt_yuv422_vyuy,
    .regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
    .bpp    = 2,
  },
//  {
//    .desc   = "Raw RGB Bayer",
//    .mbus_code  = V4L2_MBUS_FMT_SBGGR8_1X8,
//    .regs     = sensor_fmt_raw,
//    .regs_size = ARRAY_SIZE(sensor_fmt_raw),
//    .bpp    = 1
//  },
};
#define N_FMTS ARRAY_SIZE(sensor_formats)



/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */


static struct sensor_win_size sensor_win_sizes[] = {
  /* qsxga: 2592*1936 */
/*  {
    .width      = QSXGA_WIDTH,
    .height     = QSXGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_qsxga_regs,
    .regs_size  = ARRAY_SIZE(sensor_qsxga_regs),
    .set_size   = NULL,
  },*/
  /* qxga: 2048*1536 */
/*  {
    .width      = QXGA_WIDTH,
    .height     = QXGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_qxga_regs,
    .regs_size  = ARRAY_SIZE(sensor_qxga_regs),
    .set_size   = NULL,
  },*/
  /* 1080P */
/*  {
    .width      = HD1080_WIDTH,
    .height     = HD1080_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_1080p_regs,
    .regs_size  = ARRAY_SIZE(sensor_1080p_regs),
    .set_size   = NULL,
  },*/
  /* UXGA */
  {
    .width      = UXGA_WIDTH,
    .height     = UXGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_uxga_regs,
    .regs_size  = ARRAY_SIZE(sensor_uxga_regs),
    .set_size   = NULL,
  },
  /* SXGA */
  {
    .width      = SXGA_WIDTH,
    .height     = SXGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_sxga_regs,
    .regs_size  = ARRAY_SIZE(sensor_sxga_regs),
    .set_size   = NULL,
  },
  /* 720p */
  {
    .width      = HD720_WIDTH,
    .height     = HD720_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_720p_regs,
    .regs_size  = ARRAY_SIZE(sensor_720p_regs),
    .set_size   = NULL,
  },
  /* XGA */
  {
    .width      = XGA_WIDTH,
    .height     = XGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_xga_regs,
    .regs_size  = ARRAY_SIZE(sensor_xga_regs),
    .set_size   = NULL,
  },
  /* SVGA */
  {
    .width      = SVGA_WIDTH,
    .height     = SVGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_svga_regs,
    .regs_size  = ARRAY_SIZE(sensor_svga_regs),
    .set_size   = NULL,
  },
  /* VGA */
  {
    .width      = VGA_WIDTH,
    .height     = VGA_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_vga_regs,
    .regs_size  = ARRAY_SIZE(sensor_vga_regs),
    .set_size   = NULL,
  },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
                 enum v4l2_mbus_pixelcode *code)
{
  if (index >= N_FMTS)
    return -EINVAL;

  *code = sensor_formats[index].mbus_code;
  return 0;
}


static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
    struct v4l2_mbus_framefmt *fmt,
    struct sensor_format_struct **ret_fmt,
    struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;
//	struct v4l2_pix_format *pix = &fmt->fmt.pix;//linux-3.0
	vfe_dev_dbg("sensor_try_fmt_internal\n");
       vfe_dev_dbg("---------------------fmt->code = %x\n", fmt->code);
	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)//linux-3.0
			break;

	if (index >= N_FMTS) {
		/* default to first format */
		index = 0;
		fmt->code = sensor_formats[0].mbus_code;//linux-3.0
	}

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	 * Fields: the sensor devices claim to be progressive.
	 */
	fmt->field = V4L2_FIELD_NONE;//linux-3.0


	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)//linux-3.0
			break;

	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;//linux-3.0
	fmt->height = wsize->height;//linux-3.0
	printk("wsize->width = %d,wsize->height = %d\n",wsize->width,wsize->height);
	//pix->bytesperline = pix->width*sensor_formats[index].bpp;//linux-3.0
	//pix->sizeimage = pix->height*pix->bytesperline;//linux-3.0

	return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd,
             struct v4l2_mbus_framefmt *fmt)
{
  return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
           struct v4l2_mbus_config *cfg)
{
  cfg->type = V4L2_MBUS_PARALLEL;
  cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL ;

  return 0;
}

/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd,
             struct v4l2_mbus_framefmt *fmt)
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);
	vfe_dev_dbg("-------------------------------------------sensor_s_fmt\n");
	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;

       vfe_dev_dbg("sensor_s_fmt_mbus_code=%x\n",sensor_fmt->mbus_code);
	sensor_write_array(sd, sensor_fmt->regs , sensor_fmt->regs_size);

	ret = 0;
	vfe_dev_dbg("wsize->width=%d,wsize->height=%d\n" ,wsize->width,wsize->height);
	if (wsize->regs)
	{
		ret = sensor_write_array(sd, wsize->regs , wsize->regs_size);
		if (ret < 0)
			return ret;
	}

	if (wsize->set_size)
	{
		ret = wsize->set_size(sd);
		if (ret < 0)
			return ret;
	}

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	//struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = SENSOR_FRAME_RATE;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
    #if 0
  struct v4l2_captureparm *cp = &parms->parm.capture;
  struct v4l2_fract *tpf = &cp->timeperframe;
  struct sensor_info *info = to_state(sd);
  unsigned char div;

  vfe_dev_dbg("sensor_s_parm\n");

  if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE){
  	vfe_dev_dbg("parms->type!=V4L2_BUF_TYPE_VIDEO_CAPTURE\n");
    return -EINVAL;
  }

  if (info->tpf.numerator == 0){
  	vfe_dev_dbg("info->tpf.numerator == 0\n");
    return -EINVAL;
  }

  info->capture_mode = cp->capturemode;

  if (info->capture_mode == V4L2_MODE_IMAGE) {
    vfe_dev_dbg("capture mode is not video mode,can not set frame rate!\n");
    return 0;
  }

  if (tpf->numerator == 0 || tpf->denominator == 0) {
    tpf->numerator = 1;
    tpf->denominator = SENSOR_FRAME_RATE;/* Reset to full rate */
    vfe_dev_err("sensor frame rate reset to full rate!\n");
  }

  div = SENSOR_FRAME_RATE/(tpf->denominator/tpf->numerator);
  if(div > 15 || div == 0)
  {
  	vfe_dev_print("SENSOR_FRAME_RATE=%d\n",SENSOR_FRAME_RATE);
  	vfe_dev_print("tpf->denominator=%d\n",tpf->denominator);
  	vfe_dev_print("tpf->numerator=%d\n",tpf->numerator);
    return -EINVAL;
  }

  vfe_dev_dbg("set frame rate %d\n",tpf->denominator/tpf->numerator);

  info->tpf.denominator = SENSOR_FRAME_RATE;
  info->tpf.numerator = div;

	if(info->tpf.denominator/info->tpf.numerator < 30)
		info->low_speed = 1;
  return 0;
    #endif
    return 0;
}


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
static int sensor_queryctrl(struct v4l2_subdev *sd,
    struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	/* see sensor_s_parm and sensor_g_parm for the meaning of value */
//	vfe_dev_dbg("%s,%d:qc->id = %d\n",__func__,__LINE__,qc->id);
	switch (qc->id) {
//	case V4L2_CID_BRIGHTNESS:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_CONTRAST:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_SATURATION:
//		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
//	case V4L2_CID_HUE:
//		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
//	case V4L2_CID_GAIN:
//		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
//	case V4L2_CID_AUTOGAIN:
//		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_EXPOSURE:
    case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_DO_WHITE_BALANCE:
    case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 0);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_COLORFX:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 0);
	case V4L2_CID_FLASH_LED_MODE:
	  return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
  //vfe_dev_dbg("sensor_g_ctrl ctrl->id=0x%8x\n", ctrl->id);
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
	case V4L2_CID_DO_WHITE_BALANCE:
    case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_g_wb(sd, &ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_g_autowb(sd, &ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_g_colorfx(sd,	&ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_g_flash_mode(sd, &ctrl->value);
  }
  return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
  struct v4l2_queryctrl qc;
  int ret;

  //vfe_dev_dbg("sensor_s_ctrl ctrl->id=0x%8x\n", ctrl->id);
  qc.id = ctrl->id;
  ret = sensor_queryctrl(sd, &qc);
  if (ret < 0) {
    return ret;
  }

	if (qc.type == V4L2_CTRL_TYPE_MENU ||
		qc.type == V4L2_CTRL_TYPE_INTEGER ||
		qc.type == V4L2_CTRL_TYPE_BOOLEAN)
	{
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
				(enum v4l2_exposure_auto_type) ctrl->value);
	case V4L2_CID_DO_WHITE_BALANCE:
    case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_s_wb(sd,
				(enum v4l2_auto_n_preset_white_balance) ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_colorfx(sd,
				(enum v4l2_colorfx) ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
	  return sensor_s_flash_mode(sd,
	      (enum v4l2_flash_led_mode) ctrl->value);
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
  .enum_mbus_fmt = sensor_enum_fmt,
  .try_mbus_fmt = sensor_try_fmt,
  .s_mbus_fmt = sensor_s_fmt,
  .s_parm = sensor_s_parm,
  .g_parm = sensor_g_parm,
  .g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
  .core = &sensor_core_ops,
  .video = &sensor_video_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
};

static int sensor_probe(struct i2c_client *client,
      const struct i2c_device_id *id)
{
  struct v4l2_subdev *sd;
  struct sensor_info *info;
//  int ret;

  info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
  if (info == NULL)
    return -ENOMEM;
  sd = &info->sd;
  glb_sd = sd;
  cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
//  client->addr=0x30;
  info->fmt = &sensor_formats[0];
  info->af_first_flag = 1;
  info->init_first_flag = 1;
  info->auto_focus = 0;

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
  { SENSOR_NAME, 0 },
  { }
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


/*
 * A V4L2 driver for IMX214 cameras.
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


MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for IMX214 sensors");
MODULE_LICENSE("GPL");

//for internel driver debug
#define DEV_DBG_EN      1 
#if(DEV_DBG_EN == 1)    
#define vfe_dev_dbg(x,arg...) printk("[IMX214]"x,##arg)
#else
#define vfe_dev_dbg(x,arg...) 
#endif
#define vfe_dev_err(x,arg...) printk("[IMX214]"x,##arg)
#define vfe_dev_print(x,arg...) printk("[IMX214]"x,##arg)

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
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x0214


//define the voltage level of control signal
#define CSI_STBY_ON     0
#define CSI_STBY_OFF    1
#define CSI_RST_ON      0
#define CSI_RST_OFF     1
#define CSI_PWR_ON      1
#define CSI_PWR_OFF     0
#define CSI_AF_PWR_ON   1
#define CSI_AF_PWR_OFF  0
#define regval_list reg_list_a16_d8

#define REG_TERM 0xfffe
#define VAL_TERM 0xfe
#define REG_DLY  0xffff

#define DGAIN_R  0x100
#define DGAIN_G  0x100
#define DGAIN_B  0x100

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30


/*
 * The IMX214 sits on i2c with ID 0x6c
 */
#define I2C_ADDR 0x20
#define SENSOR_NAME "imx214"
int imx214_sensor_vts;


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
{0x0101,0x03},
{0x0105,0x01},
{0x0106,0x01},
{0x4550,0x02},
{0x4601,0x04},
{0x4642,0x01},
{0x6227,0x11},
{0x6276,0x00},
{0x900E,0x06},
{0xA802,0x90},
{0xA803,0x11},
{0xA804,0x62},
{0xA805,0x77},
{0xA806,0xAE},
{0xA807,0x34},
{0xA808,0xAE},
{0xA809,0x35},
{0xA80A,0x62},
{0xA80B,0x83},
{0xAE33,0x00},
{0x4174,0x00},
{0x4175,0x11},
{0x4612,0x29},
{0x461B,0x2C},
{0x461F,0x06},
{0x4635,0x07},
{0x4637,0x30},
{0x463F,0x18},
{0x4641,0x0D},
{0x465B,0x2C},
{0x465F,0x2B},
{0x4663,0x2B},
{0x4667,0x24},
{0x466F,0x24},
{0x470E,0x09},
{0x4909,0xAB},
{0x490B,0x95},
{0x4915,0x5D},
{0x4A5F,0xFF},
{0x4A61,0xFF},
{0x4A73,0x62},
{0x4A85,0x00},
{0x4A87,0xFF},
{0x583C,0x04},
{0x620E,0x04},
{0x6EB2,0x01},
{0x6EB3,0x00},
{0x9300,0x02},
{0x080b,119 },
{0x080d,55  },
{0x080f,103 },
{0x0811,55  },
{0x0813,55  },
{0x0815,55  },
{0x0817,223 },
{0x0819,47  },
{0x4601,0x04},
{0x4642,0x01},
{0x461B,0x2C},
{0x465B,0x2C},
{0x465F,0x2B},
{0x4663,0x2B},
{0x4667,0x24},
{0x466F,0x24},

};

//for capture                                                                         
static struct regval_list sensor_13mega_regs[] = {
	{0x0114, 0x03},
	{0x0220, 0x00},
	{0x0221, 0x11},
	{0x0222, 0x01},
	{0x0340, 0x0C},
	{0x0341, 0x7A},
	{0x0342, 0x13},
	{0x0343, 0x90},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x10},
	{0x0349, 0x6F},
	{0x034A, 0x0C},
	{0x034B, 0x2F},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0900, 0x00},
	{0x0901, 0x00},
	{0x0902, 0x00},
	{0x3000, 0x35},
	{0x3054, 0x01},
	{0x305C, 0x11},
	{0x0112, 0x0A},
	{0x0113, 0x0A},
	{0x034C, 0x10},
	{0x034D, 0x70},
	{0x034E, 0x0C},
	{0x034F, 0x30},
	{0x0401, 0x00},
	{0x0404, 0x00},
	{0x0405, 0x10},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x10},
	{0x040D, 0x70},
	{0x040E, 0x0C},
	{0x040F, 0x30},
	{0x0301, 0x05},
	{0x0303, 0x02},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x64},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x0310, 0x00},
	{0x0820, 0x0C},
	{0x0821, 0x80},
	{0x0822, 0x00},
	{0x0823, 0x00},
	{0x3A03, 0x08},
	{0x3A04, 0xD0},
	{0x3A05, 0x02},
	{0x0B06, 0x01},
	{0x30A2, 0x00},
	{0x30B4, 0x00},
	{0x3A02, 0xFF},
	{0x3011, 0x00},
	{0x3013, 0x01},
	{0x0202, 0x0C},
	{0x0203, 0x70},
	{0x0224, 0x01},
	{0x0225, 0xF4},
	{0x0204, 0x00},
	{0x0205, 0x00},
	{0x020E, 0x01},
	{0x020F, 0x00},
	{0x0210, 0x01},
	{0x0211, 0x00},
	{0x0212, 0x01},
	{0x0213, 0x00},
	{0x0214, 0x01},
	{0x0215, 0x00},
	{0x0216, 0x00},
	{0x0217, 0x00},
	{0x4170, 0x00},
	{0x4171, 0x10},
	{0x4176, 0x00},
	{0x4177, 0x3C},
	{0xAE20, 0x04},
	{0xAE21, 0x5C},
	{0x0138, 0x01},
	{0x0100, 0x01},
};

//for video
static struct regval_list sensor_4k_videos[]=
{
#if 1
	{0x0114,	0x03},
	{0x0220,	0x00},
	{0x0221,	0x11},
	{0x0222,	0x01},
	{0x0340,	0x09},
	{0x0341,	0x3C},
	{0x0342,	0x13},
	{0x0343,	0x90},
	{0x0344,	0x00},
	{0x0345,	0xB8},
	{0x0346,	0x01},
	{0x0347,	0xE0},
	{0x0348,	0x0F},
	{0x0349,	0xB7},
	{0x034A,	0x0A},
	{0x034B,	0x4F},
	{0x0381,	0x01},
	{0x0383,	0x01},
	{0x0385,	0x01},
	{0x0387,	0x01},
	{0x0900,	0x00},
	{0x0901,	0x00},
	{0x0902,	0x00},
	{0x3000,	0x35},
	{0x3054,	0x01},
	{0x305C,	0x11},
	{0x0112,	0x0A},
	{0x0113,	0x0A},
	{0x034C,	0x0F},
	{0x034D,	0x00},
	{0x034E,	0x08},
	{0x034F,	0x70},
	{0x0401,	0x00},
	{0x0404,	0x00},
	{0x0405,	0x10},
	{0x0408,	0x00},
	{0x0409,	0x00},
	{0x040A,	0x00},
	{0x040B,	0x00},
	{0x040C,	0x0F},
	{0x040D,	0x00},
	{0x040E,	0x08},
	{0x040F,	0x70},
	{0x0301,	0x05},
	{0x0303,	0x02},
	{0x0305,	0x01},
	{0x0306,	0x00},
	{0x0307,	0x25},
	{0x0309,	0x0A},
	{0x030B,	0x01},
	{0x0310,	0x00},
	{0x0820,	0x0D},
	{0x0821,	0xE0},
	{0x0822,	0x00},
	{0x0823,	0x00},
	{0x3A03,	0x09},
	{0x3A04,	0xA0},
	{0x3A05,	0x04},
	{0x0B06,	0x01},
	{0x30A2,	0x00},
	{0x30B4,	0x00},
	{0x3A02,	0xFF},
	{0x3011,	0x00},
	{0x3013,	0x01},
	{0x0202,	0x09},
	{0x0203,	0x32},
	{0x0224,	0x01},
	{0x0225,	0xF4},
	{0x0204,	0x00},
	{0x0205,	0x00},
	{0x020E,	0x01},
	{0x020F,	0x00},
	{0x0210,	0x01},
	{0x0211,	0x00},
	{0x0212,	0x01},
	{0x0213,	0x00},
	{0x0214,	0x01},
	{0x0215,	0x00},
	{0x0216,	0x00},
	{0x0217,	0x00},
	{0x4170,	0x00},
	{0x4171,	0x10},
	{0x4176,	0x00},
	{0x4177,	0x3C},
	{0xAE20,	0x04},
	{0xAE21,	0x5C},

#else
	{0x0114,0x03},
	{0x0220,0x00},
	{0x0221,0x11},
	{0x0222,0x01},
	{0x0340,0x08},
	{0x0341,0x98},
	{0x0342,0x13},
	{0x0343,0x90},
	{0x0344,0x00},
	{0x0345,0x38},
	{0x0346,0x01},
	{0x0347,0xE0},
	{0x0348,0x10},
	{0x0349,0x37},
	{0x034A,0x0A},
	{0x034B,0x4F},
	{0x0381,0x01},
	{0x0383,0x01},
	{0x0385,0x01},
	{0x0387,0x01},
	{0x0900,0x00},
	{0x0901,0x00},
	{0x0902,0x00},
	{0x3000,0x35},
	{0x3054,0x01},
	{0x305C,0x11},
	{0x0112,0x0A},
	{0x0113,0x0A},
	{0x034C,0x10},
	{0x034D,0x00},
	{0x034E,0x08},
	{0x034F,0x70},
	{0x0401,0x00},
	{0x0404,0x00},
	{0x0405,0x10},
	{0x0408,0x00},
	{0x0409,0x00},
	{0x040A,0x00},
	{0x040B,0x00},
	{0x040C,0x10},
	{0x040D,0x00},
	{0x040E,0x08},
	{0x040F,0x70},
	{0x0301,0x05},
	{0x0303,0x02},
	{0x0305,0x03},
	{0x0306,0x00},
	{0x0307,0x64},
	{0x0309,0x0A},
	{0x030B,0x01},
	{0x0310,0x00},
	{0x0820,0x0C},
	{0x0821,0x80},
	{0x0822,0x00},
	{0x0823,0x00},
	{0x3A03,0x08},
	{0x3A04,0xE0},
	{0x3A05,0x02},
	{0x0B06,0x01},
	{0x30A2,0x00},
	{0x30B4,0x00},
	{0x3A02,0xFF},
	{0x3011,0x00},
	{0x3013,0x01},
	{0x0202,0x08},
	{0x0203,0x8E},
	{0x0224,0x01},
	{0x0225,0xF4},
	{0x0204,0x00},
	{0x0205,0x00},
	{0x020E,0x01},
	{0x020F,0x00},
	{0x0210,0x01},
	{0x0211,0x00},
	{0x0212,0x01},
	{0x0213,0x00},
	{0x0214,0x01},
	{0x0215,0x00},
	{0x0216,0x00},
	{0x0217,0x00},
	{0x4170,0x00},
	{0x4171,0x10},
	{0x4176,0x00},
	{0x4177,0x3C},
	{0xAE20,0x04},
	{0xAE21,0x5C},
	{0x0138, 0x01},
	{0x0100, 0x01},
	#endif
};

static struct regval_list sensor_1080p_regs[] = { 
	{0x0114,0x03},
	{0x0220,0x00},
	{0x0221,0x11},
	{0x0222,0x01},
	{0x0340,0x08},
	{0x0341,0x50},
	{0x0342,0x13},
	{0x0343,0x90},
	{0x0344,0x00},
	{0x0345,0xB8},
	{0x0346,0x01},
	{0x0347,0xE0},
	{0x0348,0x0F},
	{0x0349,0xB7},
	{0x034A,0x0A},
	{0x034B,0x4F},
	{0x0381,0x01},
	{0x0383,0x01},
	{0x0385,0x01},
	{0x0387,0x01},
	{0x0900,0x01},
	{0x0901,0x22},
	{0x0902,0x02},
	{0x3000,0x35},
	{0x3054,0x01},
	{0x305C,0x11},
	{0x0112,0x0A},
	{0x0113,0x0A},
	{0x034C,0x07},
	{0x034D,0x80},
	{0x034E,0x04},
	{0x034F,0x38},
	{0x0401,0x00},
	{0x0404,0x00},
	{0x0405,0x10},
	{0x0408,0x00},
	{0x0409,0x00},
	{0x040A,0x00},
	{0x040B,0x00},
	{0x040C,0x07},
	{0x040D,0x80},
	{0x040E,0x04},
	{0x040F,0x38},
	{0x0301,0x05},
	{0x0303,0x02},
	{0x0305,0x03},
	{0x0306,0x00},
	{0x0307,0x64},
	{0x0309,0x0A},
	{0x030B,0x01},
	{0x0310,0x00},
	{0x0820,0x0C},
	{0x0821,0x80},
	{0x0822,0x00},
	{0x0823,0x00},
	{0x3A03,0x06},
	{0x3A04,0x28},
	{0x3A05,0x04},
	{0x0B06,0x01},
	{0x30A2,0x00},
	{0x30B4,0x00},
	{0x3A02,0xFF},
	{0x3011,0x00},
	{0x3013,0x01},
	{0x0202,0x08},
	{0x0203,0x46},
	{0x0224,0x01},
	{0x0225,0xF4},
	{0x0204,0x00},
	{0x0205,0x00},
	{0x020E,0x01},
	{0x020F,0x00},
	{0x0210,0x01},
	{0x0211,0x00},
	{0x0212,0x01},
	{0x0213,0x00},
	{0x0214,0x01},
	{0x0215,0x00},
	{0x0216,0x00},
	{0x0217,0x00},
	{0x4170,0x00},
	{0x4171,0x10},
	{0x4176,0x00},
	{0x4177,0x3C},
	{0xAE20,0x04},
	{0xAE21,0x5C},
	{0x0138, 0x01},
	{0x0100, 0x01},
};

 

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 * 
 */

static struct regval_list sensor_fmt_raw[] = {

  //{REG_TERM,VAL_TERM},
};

/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
static int sensor_read(struct v4l2_subdev *sd, unsigned short reg,
    unsigned char *value) //!!!!be careful of the para type!!!
{
	int ret=0;
	int cnt=0;
	
  ret = cci_read_a16_d8(sd,reg,value);
  while(ret!=0&&cnt<2)
  {
  	ret = cci_read_a16_d8(sd,reg,value);
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
  ret = cci_write_a16_d8(sd,reg,value);
  while(ret!=0&&cnt<2)
  {
  	ret = cci_write_a16_d8(sd,reg,value);
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
      LOG_ERR_RET(sensor_write(sd, regs->addr, regs->data))
    }
    i++;
    regs++;
  }
  return 0;
}

/* 
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
/*
static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;
    
  LOG_ERR_RET(sensor_read(sd, 0x3821, &rdval))
  
  rdval &= (1<<1);
  rdval >>= 1;
    
  *value = rdval;

  info->hflip = *value;
  return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  if(info->hflip == value)
    return 0;
    
  LOG_ERR_RET(sensor_read(sd, 0x3821, &rdval))
  
  switch (value) {
    case 0:
      rdval &= 0xf9;
      break;
    case 1:
      rdval |= 0x06;
      break;
    default:
      return -EINVAL;
  }
  
  LOG_ERR_RET(sensor_write(sd, 0x3821, rdval))
  
  mdelay(10);
  info->hflip = value;
  return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  LOG_ERR_RET(sensor_read(sd, 0x3820, &rdval))
  
  rdval &= (1<<1);  
  *value = rdval;
  rdval >>= 1;
  
  info->vflip = *value;
  return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  if(info->vflip == value)
    return 0;
  
  LOG_ERR_RET(sensor_read(sd, 0x3820, &rdval))

  switch (value) {
    case 0:
      rdval &= 0xf9;
      break;
    case 1:
      rdval |= 0x06;
      break;
    default:
      return -EINVAL;
  }

  LOG_ERR_RET(sensor_write(sd, 0x3820, rdval))
  
  mdelay(10);
  info->vflip = value;
  return 0;
}
*/

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->exp;
	vfe_dev_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow,exphigh;
	struct sensor_info *info = to_state(sd);

	if(exp_val>0xffffff)
		exp_val=0xfffff0;
	if(exp_val<16)
		exp_val=16;
	
	exp_val=(exp_val+8)>>4;//rounding to 1
	
	exphigh = (unsigned char) ( (0xff00&exp_val)>>8);
	explow	= (unsigned char) ( (0x00ff&exp_val) );

	sensor_write(sd, 0x0203, explow);//coarse integration time
	sensor_write(sd, 0x0202, exphigh);	
	
	info->exp = exp_val;
	return 0;
}


static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->gain;
	vfe_dev_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
//	return 0;
	struct sensor_info *info = to_state(sd);
	unsigned char gainlow = 0;
	unsigned char gainhigh = 0;
	int gainana = 512 - 8192/gain_val;
	
	gainlow=(unsigned char)(gainana&0xff);
	gainhigh=(unsigned char)((gainana>>8)&0xff);
	
	sensor_write(sd, 0x0205, gainlow);
	sensor_write(sd, 0x0204, gainhigh);
	
//	printk("imx214 set_gain = %d %d\n", gain_val, gainana);
	info->gain = gain_val;
	
	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd, struct sensor_exp_gain *exp_gain)
{
//return 0;
	int exp_val, gain_val,shutter,frame_length;  
  struct sensor_info *info = to_state(sd);
  
  exp_val = exp_gain->exp_val;
  gain_val = exp_gain->gain_val;
  
  if(gain_val<1*16)
	  gain_val=16;
  if(gain_val>64*16-1)
	  gain_val=64*16-1;
  
  if(exp_val>0xfffff)
	  exp_val=0xfffff;
  
  shutter = exp_val/16;
  if(shutter  > imx214_sensor_vts)
		frame_length = shutter;
  else
		frame_length = imx214_sensor_vts;

//  sensor_write(sd, 0x0341, (frame_length & 0xff));
//  sensor_write(sd, 0x0340, (frame_length >> 8));
  
  sensor_write(sd,0x0104,0x01);
  sensor_s_exp(sd,exp_val);
  sensor_s_gain(sd,gain_val);
  sensor_write(sd,0x0104,0x00);

  if (gain_val > 64)
  {
	  sensor_write(sd,0x30a2, 0x03);	//enable LNR CNR
	  sensor_write(sd,0x9706, (gain_val-64)/12);	//LNR	0x00~0x10
	  sensor_write(sd,0x9e25, (gain_val-64));	//CNR	0x00~0x8c
  }
  else
  {
	  sensor_write(sd,0x30a2,0x00);
  }
  
//  printk("norm exp_val = %d,gain_val = %d,frame_length = %d,imx214_sensor_vts = %d 30a2 0x%x\n",exp_val,gain_val,frame_length,imx214_sensor_vts,rdval);

  info->exp = exp_val;
  info->gain = gain_val;
  return 0;
}


#if 0
static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	unsigned char rdval;
	
	ret=sensor_read(sd, 0x0100, &rdval);
	if(ret!=0)
		return ret;
	
	if(on_off==CSI_STBY_ON)//sw stby on
	{
		ret=sensor_write(sd, 0x0100, rdval&0xfe);
	}
	else//sw stby off
	{
		ret=sensor_write(sd, 0x0100, rdval|0x01);
	}
	return ret;
}
#endif


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
//      //disable io oe
//      vfe_dev_print("disalbe oe!\n");
//      ret = sensor_write_array(sd, sensor_oe_disable_regs, ARRAY_SIZE(sensor_oe_disable_regs));
//      if(ret < 0)
//        vfe_dev_err("disalbe oe falied!\n");           
      //software standby on
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      cci_lock(sd);    

//      //reset on io
     vfe_gpio_write(sd,RESET,CSI_RST_ON);
//      mdelay(10);
      //standby on io
     // vfe_gpio_write(sd,PWDN,CSI_STBY_ON);
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
      msleep(20);
      //standby off io
     // vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
    
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      cci_unlock(sd);        
      //software standby

	  vfe_gpio_write(sd,RESET,CSI_RST_OFF);
  //    mdelay(10);
     // vfe_dev_print("enable oe!\n");
    //  ret = sensor_write_array(sd, sensor_oe_enable_regs,  ARRAY_SIZE(sensor_oe_enable_regs));
    //  if(ret < 0)
    //    vfe_dev_err("enable oe falied!\n");
      break;
    case CSI_SUBDEV_PWR_ON:
      vfe_dev_dbg("CSI_SUBDEV_PWR_ON!\n");
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      cci_lock(sd);    

      //power on reset
      //vfe_gpio_set_status(sd,PWDN,1);//set the gpio to output
      vfe_gpio_set_status(sd,RESET,1);//set the gpio to output
      //power down io
     // vfe_gpio_write(sd,PWDN,CSI_STBY_ON);
      //reset on io
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
      usleep_range(1000,1200);
      //active mclk before power on
      vfe_set_mclk_freq(sd,MCLK);
      vfe_set_mclk(sd,ON);
      usleep_range(10000,12000);
      //power supply
     vfe_gpio_write(sd,POWER_EN,CSI_PWR_ON);
      vfe_set_pmu_channel(sd,IOVDD,ON);
      vfe_set_pmu_channel(sd,AVDD,ON);
      vfe_set_pmu_channel(sd,DVDD,ON);
      vfe_set_pmu_channel(sd,AFVDD,ON);
      //standby off io
    //  vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
      usleep_range(10000,12000);
      //reset after power on
      vfe_gpio_write(sd,RESET,CSI_RST_OFF);
      usleep_range(30000,31000);
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
      usleep_range(10000,12000);
      vfe_gpio_write(sd,POWER_EN,CSI_STBY_OFF);
     // vfe_gpio_write(sd,RESET,CSI_RST_ON);
      //set the io to hi-z
      vfe_gpio_set_status(sd,RESET,0);//set the gpio to input
     // vfe_gpio_set_status(sd,PWDN,0);//set the gpio to input
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
      mdelay(10);
      break;
    case 1:
      vfe_gpio_write(sd,RESET,CSI_RST_ON);
      mdelay(10);
      break;
    default:
      return -EINVAL;
  }
    
  return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
  unsigned char rdval;
  
  LOG_ERR_RET(sensor_read(sd, 0x0000, &rdval))
//  if((rdval&0x0f) != 0x02)
//    return -ENODEV;
 
  LOG_ERR_RET(sensor_read(sd, 0x0001, &rdval))
//  if(rdval != 0x19)
//    return -ENODEV;
  printk("find the sony IMX214 ***********\n");
  return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
  int ret;
  struct sensor_info *info = to_state(sd);
  
  vfe_dev_dbg("sensor_init\n");
  
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
  info->width = 4208;
  info->height = 3120;
  info->hflip = 0;
  info->vflip = 0;
  info->gain = 0;

  info->tpf.numerator = 1;            
  info->tpf.denominator = 30;    /* 30fps */    
  
  ret = sensor_write_array(sd, sensor_default_regs, ARRAY_SIZE(sensor_default_regs));  
  if(ret < 0) {
    vfe_dev_err("write sensor_default_regs error\n");
    return ret;
  }
  
  if(info->stby_mode == 0)
    info->init_first_flag = 0;
  
  info->preview_first_flag = 1;
  
  return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
  int ret=0;
  struct sensor_info *info = to_state(sd);

  switch(cmd) {
    case GET_CURRENT_WIN_CFG:
      if(info->current_wins != NULL)
      {
        memcpy( arg,
                info->current_wins,
                sizeof(struct sensor_win_size) );
        ret=0;
      }
      else
      {
        vfe_dev_err("empty wins!\n");
        ret=-1;
      }
      break;
    case SET_FPS:
      ret=0;
      break;
    case ISP_SET_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
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
  //__u32 pixelformat;
  enum v4l2_mbus_pixelcode mbus_code;
  struct regval_list *regs;
  int regs_size;
  int bpp;   /* Bytes per pixel */
}sensor_formats[] = {
	{
		.desc				= "Raw RGB Bayer",
	    .mbus_code	= V4L2_MBUS_FMT_SBGGR10_10X1,
		.regs 			= sensor_fmt_raw,
		.regs_size 	= ARRAY_SIZE(sensor_fmt_raw),
		.bpp				= 1
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
     .width      = 4208,
     .height     = 3120,
     .hoffset    = 0,
     .voffset    = 0,
     .hts        = 5008,
     .vts        = 3194,
     .pclk       = 320*1000*1000,
     .mipi_bps	 = 800*1000*1000,
     .fps_fixed  = 2,
     .bin_factor = 1,
     .intg_min   = 16,
     .intg_max   = (3194-10)<<4,
     .gain_min   = 16,
     .gain_max   = (16<<4),
     .regs       = sensor_13mega_regs,
     .regs_size  = ARRAY_SIZE(sensor_13mega_regs),
     .set_size   = NULL,
   },
#endif
#if 1
	/*4k video*/
	   {
	  .width	  = 3840,
	  .height	  = 2160,
	  .hoffset	  = 0,//(4096-3840)>>1,
	  .voffset	  = 0,
	  .hts		  = 5008,
	  .vts		  = 2362,//2200,
	  .pclk 	  = 355*1000*1000,//320
	  .mipi_bps   = 888*1000*1000,//800
	  .fps_fixed  = 2,
	  .bin_factor = 1,
	  .intg_min   = 16,
	  .intg_max   = (2362-10)<<4,
	  .gain_min   = 16,
	  .gain_max   = (16<<4),
	  .regs 	  = sensor_4k_videos,
	  .regs_size  = ARRAY_SIZE(sensor_4k_videos),
	  .set_size   = NULL,
	},

   /* 1080p */
   {
     .width	     = 1920,
     .height 	 = 1080,
     .hoffset	 = 0,
     .voffset	 = 0,
     .hts        = 5008,
     .vts        = 2128,
	 .pclk		 = 320*1000*1000,
     .mipi_bps   = 800*1000*1000,
     .fps_fixed  = 1,
     .bin_factor = 1,
     .intg_min   = 16,
     .intg_max   = (2128-10)<<4,
     .gain_min   = 16,
     .gain_max   = (16<<4),
     .regs		 = sensor_1080p_regs,
     .regs_size  = ARRAY_SIZE(sensor_1080p_regs),
     .set_size	 = NULL,
   },
#endif

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

static int sensor_enum_size(struct v4l2_subdev *sd,
                            struct v4l2_frmsizeenum *fsize)
{
  if(fsize->index > N_WIN_SIZES-1)
    return -EINVAL;
  
  fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
  fsize->discrete.width = sensor_win_sizes[fsize->index].width;
  fsize->discrete.height = sensor_win_sizes[fsize->index].height;
  
  return 0;
}


static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
    struct v4l2_mbus_framefmt *fmt,
    struct sensor_format_struct **ret_fmt,
    struct sensor_win_size **ret_wsize)
{
  int index;
  struct sensor_win_size *wsize;
  struct sensor_info *info = to_state(sd);

  for (index = 0; index < N_FMTS; index++)
    if (sensor_formats[index].mbus_code == fmt->code)
      break;

  if (index >= N_FMTS) 
    return -EINVAL;
  
  if (ret_fmt != NULL)
    *ret_fmt = sensor_formats + index;
    
  /*
   * Fields: the sensor devices claim to be progressive.
   */
  
  fmt->field = V4L2_FIELD_NONE;
  
  /*
   * Round requested image size down to the nearest
   * we support, but not below the smallest.
   */
  for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
       wsize++)
    if (fmt->width >= wsize->width && fmt->height >= wsize->height)
      break;
    
  if (wsize >= sensor_win_sizes + N_WIN_SIZES)
    wsize--;   /* Take the smallest one */
  if (ret_wsize != NULL)
    *ret_wsize = wsize;
  /*
   * Note the size we'll actually handle.
   */
  fmt->width = wsize->width;
  fmt->height = wsize->height;
  info->current_wins = wsize;
  //pix->bytesperline = pix->width*sensor_formats[index].bpp;
  //pix->sizeimage = pix->height*pix->bytesperline;

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
  cfg->type = V4L2_MBUS_CSI2;
  cfg->flags = 0|V4L2_MBUS_CSI2_4_LANE|V4L2_MBUS_CSI2_CHANNEL_0;
  
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
  
  vfe_dev_dbg("sensor_s_fmt\n");
  
  //sensor_write_array(sd, sensor_oe_disable_regs, ARRAY_SIZE(sensor_oe_disable_regs));
  
  ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
  if (ret)
    return ret;

  if(info->capture_mode == V4L2_MODE_VIDEO)
  {
    //video
  }
  else if(info->capture_mode == V4L2_MODE_IMAGE)
  {
    //image 
    
  }

  sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

  ret = 0;
  if (wsize->regs)
    LOG_ERR_RET(sensor_write_array(sd, wsize->regs, wsize->regs_size))
  
  if (wsize->set_size)
    LOG_ERR_RET(wsize->set_size(sd))

  info->fmt = sensor_fmt;
  info->width = wsize->width;
  info->height = wsize->height;
   imx214_sensor_vts = wsize->vts;

  vfe_dev_print("s_fmt = %x, width = %d, height = %d\n",sensor_fmt->mbus_code,wsize->width,wsize->height);

  if(info->capture_mode == V4L2_MODE_VIDEO)
  {
    //video
   
  } else {
    //capture image

  }
	
	//sensor_write_array(sd, sensor_oe_enable_regs, ARRAY_SIZE(sensor_oe_enable_regs));
	vfe_dev_print("s_fmt end\n");
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
  //struct v4l2_fract *tpf = &cp->timeperframe;
  struct sensor_info *info = to_state(sd);
  //unsigned char div;
  
  vfe_dev_dbg("sensor_s_parm\n");
  
  if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    return -EINVAL;
  
  if (info->tpf.numerator == 0)
    return -EINVAL;
    
  info->capture_mode = cp->capturemode;
  
  return 0;
}


static int sensor_queryctrl(struct v4l2_subdev *sd,
    struct v4l2_queryctrl *qc)
{
  /* Fill in min, max, step and default value for these controls. */
  /* see include/linux/videodev2.h for details */
  
  switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1*16, 64*16-1, 1, 1*16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 65535*16, 1, 0);
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
  .enum_mbus_fmt = sensor_enum_fmt,
  .enum_framesizes = sensor_enum_size,
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

  info->fmt = &sensor_formats[0];
  info->af_first_flag = 1;
  info->init_first_flag = 1;

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


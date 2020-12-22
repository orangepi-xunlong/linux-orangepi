/*
 * A V4L2 driver for h22_mipi Raw cameras.
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
MODULE_DESCRIPTION("A low-level driver for h22_mipi Raw sensors");
MODULE_LICENSE("GPL");

//for internel driver debug
#define DEV_DBG_EN      1 
#if(DEV_DBG_EN == 1)    
#define vfe_dev_dbg(x,arg...) printk("[h22_mipi Raw]"x,##arg)
#else
#define vfe_dev_dbg(x,arg...) 
#endif
#define vfe_dev_err(x,arg...) printk("[h22_mipi Raw]"x,##arg)
#define vfe_dev_print(x,arg...) printk("[h22_mipi Raw]"x,##arg)

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
#define V4L2_IDENT_SENSOR  0xa022


//define the voltage level of control signal
#define CSI_STBY_ON     1
#define CSI_STBY_OFF    0
#define CSI_RST_ON      0
#define CSI_RST_OFF     1
#define CSI_PWR_ON      1
#define CSI_PWR_OFF     0
#define CSI_AF_PWR_ON   1
#define CSI_AF_PWR_OFF  0
#define regval_list reg_list_a8_d8
#define REG_DLY  0xff

//define the registers
#define EXP_HIGH		0xff
#define EXP_MID			0x02
#define EXP_LOW			0x01
#define GAIN_HIGH		0xff
#define GAIN_LOW		0x00
//#define FRACTION_EXP
#define ID_REG_HIGH		0x0a
#define ID_REG_LOW		0x0b
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)

//#define GROUP_WRITE

/*
 *Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30


/*
 * The h22_mipi i2c address
 */
#define I2C_ADDR 0x60
#define  SENSOR_NAME "h22_mipi"

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

static struct regval_list sensor_default_regs[] = 
{
	{0x12, 0x40},
	{0x1F, 0x00},
	{0x0E, 0x1D},
	{0x0F, 0x09},
	{0x10, 0x20},
	{0x11, 0x80},
	{0x20, 0xDC},
	{0x21, 0x05},
	{0x22, 0x56},
	{0x23, 0x03},
	{0x24, 0x00},
	{0x25, 0xD0},
	{0x26, 0x25},
	{0x27, 0xBB},
	{0x28, 0x0D},
	{0x29, 0x00},
	{0x2A, 0xAC},
	{0x2B, 0x10},
	{0x2C, 0x01},
	{0x2D, 0x0A},
	{0x2E, 0xC2},
	{0x2F, 0x20},
	{0x31, 0x08},
	{0x33, 0x14},
	{0x1D, 0x00},
	{0x1E, 0x00},
	{0x6C, 0x10},
	{0x73, 0x33},
	{0x70, 0x69},
	{0x76, 0x40},
	{0x77, 0x06},
	{0x67, 0x30},
	{0x6D, 0x09},
	{0x13, 0xC7},
	{0x14, 0x80},
	{0x16, 0xC0},
	{0x17, 0x40},
	{0x18, 0x00},
	{0x19, 0x01},
	{0x37, 0x35},
	{0x38, 0x98},
	{0x4a, 0x03},
	{0x49, 0x10},
	{0x69, 0x32},
	{0x0D, 0x58},
	{0x12, 0x00},

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
static int sensor_read(struct v4l2_subdev *sd, unsigned char reg,
    unsigned char *value) //!!!!be careful of the para type!!!
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

static int sensor_write(struct v4l2_subdev *sd, unsigned char reg,
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
static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->exp;
	vfe_dev_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow,expmid,exphigh;
	struct sensor_info *info = to_state(sd);

	if(exp_val>0xfffff)
		exp_val=0xfffff;
	
	#ifdef FRACTION_EXP
		exphigh   = (unsigned char) ( (0x0f0000&exp_val)>>16);
		expmid	  = (unsigned char) ( (0x00ff00&exp_val)>>8);
		explow	  = (unsigned char) ( (0x0000ff&exp_val)   );
		sensor_write(sd, EXP_HIGH, exphigh);
	#else
	    exphigh = 0;
	    expmid  = (unsigned char) ( (0x0ff000&exp_val)>>12);
	    explow  = (unsigned char) ( (0x000ff0&exp_val)>>4);
	#endif

#ifdef GROUP_WRITE
	sensor_write(sd, 0xc0, EXP_MID);
	sensor_write(sd, 0xc1, expmid);
	sensor_write(sd, 0xc2, EXP_LOW);
	sensor_write(sd, 0xc3, explow);
#else
	sensor_write(sd, EXP_MID, expmid);
	sensor_write(sd, EXP_LOW, explow);
#endif

//	printk("h22_mipi sensor_set_exp = 0x%x  0x%x, Done!\n", expmid, explow);
	
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
	struct sensor_info *info = to_state(sd);
	unsigned char gainlow=0;
	unsigned char gainhigh=0;
	unsigned char gainlow_l4b=0;
	unsigned int tmp_gain_val=0;

	tmp_gain_val=gain_val;
	
	//determine ?gain_val>31
	if(tmp_gain_val>31)
	{
		gainlow |= 0x10;
		tmp_gain_val = tmp_gain_val>>1;
	}
	//determine ?gain_val>2*31
	if(tmp_gain_val>31)
	{
		gainlow |= 0x20;
		tmp_gain_val = tmp_gain_val>>1;
	}
	//determine ?gain_val>4*31
	if(tmp_gain_val>31)
	{
		gainlow |= 0x40;
		tmp_gain_val = tmp_gain_val>>1;
	}
	//determine ?gain_val>8*31
	if(tmp_gain_val>31)
	{
		gainlow |= 0x80;
		tmp_gain_val = tmp_gain_val>>1;
	}
	//determine ?gain_val>16*31
	if(tmp_gain_val>31)
	{
		gainhigh = 0x01;
		tmp_gain_val = tmp_gain_val>>1;
	}
	
	if(tmp_gain_val>=16)
		gainlow_l4b=((tmp_gain_val-16)&0x0f);
	
	gainlow  = gainlow | gainlow_l4b;

#ifdef GROUP_WRITE
	sensor_write(sd, 0xc4, 0x00);
	sensor_write(sd, 0xc5, gainlow);
#else
	sensor_write(sd, GAIN_LOW, gainlow);
#endif
	
	info->gain = gain_val;
	
	return 0;
}


static int h22_sensor_vts;
static int sensor_s_exp_gain(struct v4l2_subdev *sd, struct sensor_exp_gain *exp_gain)
{
  int exp_val, gain_val;  
  struct sensor_info *info = to_state(sd);
  unsigned char rdval;
  
  exp_val = exp_gain->exp_val;
  gain_val = exp_gain->gain_val;
  
//  printk("h22 sensor exp & gain is %d\t%d\n", exp_val, gain_val);
  
  if(gain_val<1*16)
	  gain_val=16;
  if(gain_val>64*16-1)
	  gain_val=64*16-1;
  
  if(exp_val>0xfffff)
	  exp_val=0xfffff;
  
  LOG_ERR_RET(sensor_read(sd, 0x12, &rdval))
  rdval &= 0xF7;
  sensor_write(sd, 0x12, rdval);

  sensor_s_exp(sd,exp_val);
  sensor_s_gain(sd,gain_val);
  
  LOG_ERR_RET(sensor_read(sd, 0x12, &rdval))
  rdval |= 0x08;
  sensor_write(sd, 0x12, rdval);

  info->exp = exp_val;
  info->gain = gain_val;
  return 0;
}


/*
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
*/

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
      //disable io oe
     // vfe_dev_print("disalbe oe!\n");
      //ret = sensor_write_array(sd, sensor_oe_disable_regs, ARRAY_SIZE(sensor_oe_disable_regs));
     // if(ret < 0)
     //   vfe_dev_err("disalbe oe falied!\n");
      //software standby on
      //ret = sensor_s_sw_stby(sd, CSI_STBY_ON);
      if(ret < 0)
        vfe_dev_err("soft stby falied!\n");
      usleep_range(10000,12000);
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      cci_lock(sd);
//      //reset on io
//      vfe_gpio_write(sd,RESET,CSI_RST_ON);
//      usleep_range(10000,12000);
      //standby on io
     vfe_gpio_write(sd,PWDN,CSI_STBY_ON);
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      cci_unlock(sd);
      //inactive mclk after stadby in
//      vfe_set_mclk(sd,OFF);
      break;
    case CSI_SUBDEV_STBY_OFF:
      vfe_dev_dbg("CSI_SUBDEV_STBY_OFF!\n");
      //make sure that no device can access i2c bus during sensor initial or power down
      //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
      cci_lock(sd);
      //active mclk before stadby out
      vfe_set_mclk_freq(sd,MCLK);
      vfe_set_mclk(sd,ON);
      usleep_range(10000,12000);
      //standby off io
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);

      usleep_range(10000,12000);
      //remember to unlock i2c adapter, so the device can access the i2c bus again
      cci_unlock(sd);
      //software standby
     // ret = sensor_s_sw_stby(sd, CSI_STBY_OFF);
     // if(ret < 0)
      //  vfe_dev_err("soft stby off falied!\n");
      usleep_range(10000,12000);
    //  vfe_dev_print("enable oe!\n");
     // ret = sensor_write_array(sd, sensor_oe_enable_regs,  ARRAY_SIZE(sensor_oe_enable_regs));
     // if(ret < 0)
     //   vfe_dev_err("enable oe falied!\n");
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
      usleep_range(1000,1200);

      //power supply
      vfe_gpio_write(sd,POWER_EN,CSI_PWR_ON);
      vfe_set_pmu_channel(sd,DVDD,ON);
      vfe_set_pmu_channel(sd,IOVDD,ON);
      vfe_set_pmu_channel(sd,AVDD,ON);
         //active mclk before power on
      vfe_set_mclk_freq(sd,MCLK);
      vfe_set_mclk(sd,ON);
      usleep_range(10000,12000);
      //vfe_set_pmu_channel(sd,AFVDD,ON);
      //standby off io
      vfe_gpio_write(sd,PWDN,CSI_STBY_OFF);
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
      //vfe_set_pmu_channel(sd,AFVDD,OFF);
      vfe_set_pmu_channel(sd,AVDD,OFF);
      vfe_set_pmu_channel(sd,IOVDD,OFF);
      vfe_set_pmu_channel(sd,DVDD,OFF);
      //standby and reset io
      usleep_range(10000,12000);
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
            usleep_range(10000,12000);
            break;
        case 1:
            vfe_gpio_write(sd,RESET,CSI_RST_ON);
            usleep_range(10000,12000);
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
  unsigned char rdval;
  
  LOG_ERR_RET(sensor_read(sd, ID_REG_HIGH, &rdval))
  vfe_dev_dbg("h22_mipi ID_VAL_HIGH = %2x, Done!\n", rdval);
  if(rdval != ID_VAL_HIGH)
    return -ENODEV;
 

  LOG_ERR_RET(sensor_read(sd, ID_REG_LOW, &rdval))
  vfe_dev_dbg("h22_mipi ID_VAL_LOW = %2x, Done!\n", rdval);
  if(rdval != ID_VAL_LOW)
    return -ENODEV;

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
    info->width = HD720_WIDTH;
    info->height = HD720_HEIGHT;
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
		.desc		= "Raw RGB Bayer",
		.mbus_code	= V4L2_MBUS_FMT_SBGGR10_10X1,
		.regs 		= sensor_fmt_raw,
		.regs_size  = ARRAY_SIZE(sensor_fmt_raw),
		.bpp		= 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
    /* qsxga: 2304*1296 */  
    {
        .width      = HD720_WIDTH,
        .height     = HD720_HEIGHT,
        .hoffset    = 0,
        .voffset    = 0,
        .hts        = 1500,
        .vts        = 854,
        .pclk       = 38400000,
        .mipi_bps	= 384*1000*1000,
        .fps_fixed  = 1,
        .bin_factor = 1,
        .intg_min   = 1<<4,
        .intg_max   = (854-4)<<4,//
        .gain_min   = 1<<4,
        .gain_max   = (16<<4)-1,
        .regs       = sensor_default_regs,
        .regs_size  = ARRAY_SIZE(sensor_default_regs),
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
  cfg->flags = 0|V4L2_MBUS_CSI2_1_LANE|V4L2_MBUS_CSI2_CHANNEL_0;
  
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
    //  sensor_write_array(sd, sensor_oe_disable_regs, ARRAY_SIZE(sensor_oe_disable_regs));
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
    LOG_ERR_RET(sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size))
    ret = 0;
    if (wsize->regs)
        LOG_ERR_RET(sensor_write_array(sd, wsize->regs, wsize->regs_size))
    if (wsize->set_size)
        LOG_ERR_RET(wsize->set_size(sd))

    info->fmt = sensor_fmt;
    info->width = wsize->width;
    info->height = wsize->height;
    h22_sensor_vts = wsize->vts;
    // show_regs_array(sd,sensor_1080p_regs);

    vfe_dev_print("s_fmt set width = %d, height = %d\n",wsize->width,wsize->height);
    if(info->capture_mode == V4L2_MODE_VIDEO)
    {
        //video
    } else {
        //capture image
    }
    //sensor_write_array(sd, sensor_oe_enable_regs, ARRAY_SIZE(sensor_oe_enable_regs));
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
		return v4l2_ctrl_query_fill(qc, 1*16, 32*16, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 65535*16, 1, 0);
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
    struct v4l2_subdev * sd;

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

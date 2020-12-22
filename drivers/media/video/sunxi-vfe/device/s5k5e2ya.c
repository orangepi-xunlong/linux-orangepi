/*
 * A V4L2 driver for s5k5e2_mipi cameras.
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


MODULE_AUTHOR("zw");
MODULE_DESCRIPTION("A low-level driver for s5k5e2ya sensors");
MODULE_LICENSE("GPL");

//for internel driver debug
#define DEV_DBG_EN      0
#if(DEV_DBG_EN == 1)    
#define vfe_dev_dbg(x,arg...) printk("[s5k5e2ya]"x,##arg)
#else
#define vfe_dev_dbg(x,arg...) 
#endif
#define vfe_dev_err(x,arg...) printk("[s5k5e2ya]"x,##arg)
#define vfe_dev_print(x,arg...) printk("[s5k5e2ya]"x,##arg)

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
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_FALLING
#define V4L2_IDENT_SENSOR 0x5e20

//define the voltage level of control signal
#define CSI_STBY_ON     0
#define CSI_STBY_OFF    1
#define CSI_RST_ON      0
#define CSI_RST_OFF     1
#define CSI_PWR_ON      1
#define CSI_PWR_OFF     0
#define CSI_AF_PWR_ON   1
#define CSI_AF_PWR_OFF  0

//modified for each device for i2c access format
#define regval_list 		reg_list_a16_d8

#define REG_TERM 0xfffe
#define VAL_TERM 0xfe
#define REG_DLY  0xffff

/*
 * Our nominal (default) frame rate.
 */
#ifdef FPGA
#define SENSOR_FRAME_RATE 15
#else
#define SENSOR_FRAME_RATE 30
#endif

/*
 * The s5k5e2_mipi sits on i2c with ID 0x20
 */
#define I2C_ADDR (0x20)
#define SENSOR_NAME "s5k5e2ya"
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
    // +++++++++++++++++++++++++++//
    // Reset for operation 
    {0x0100,0x00}, //  Stream off   
    // +++++++++++++++++++++++++++//  
    //global setting                  
    {0x3000,0x04}, //  ct_ld_start                                    
    {0x3002,0x03}, //  ct_sl_start                                    
    {0x3003,0x04}, //  ct_sl_margin                                   
    {0x3004,0x02}, //  ct_rx_start                                    
    {0x3005,0x00}, //  ct_rx_margin (MSB)                             
    {0x3006,0x10}, //  ct_rx_margin (LSB)                             
    {0x3007,0x03}, //  ct_tx_start                                    
    {0x3008,0x55}, //  ct_tx_width                                    
    {0x3039,0x00}, //  cintc1_margin (10 --> 00)                      
    {0x303A,0x00}, //  cintc2_margin (10 --> 00)                      
    {0x303B,0x00}, //  offs_sh                                        
    {0x3009,0x05}, //  ct_srx_margin                                  
    {0x300A,0x55}, //  ct_stx_width                                   
    {0x300B,0x38}, //  ct_dstx_width                                  
    {0x300C,0x10}, //  ct_stx2dstx                                    
    {0x3012,0x05}, //  ct_cds_start                                   
    {0x3013,0x00}, //  ct_s1s_start                                   
    {0x3014,0x22}, //  ct_s1s_end                                     
    {0x300E,0x79}, //  ct_s3_width                                    
    {0x3010,0x68}, //  ct_s4_width                                    
    {0x3019,0x03}, //  ct_s4d_start                                   
    {0x301A,0x00}, //  ct_pbr_start                                   
    {0x301B,0x06}, //  ct_pbr_width                                   
    {0x301C,0x00}, //  ct_pbs_start                                   
    {0x301D,0x22}, //  ct_pbs_width                                   
    {0x301E,0x00}, //  ct_pbr_ob_start                                
    {0x301F,0x10}, //  ct_pbr_ob_width                                
    {0x3020,0x00}, //  ct_pbs_ob_start                                
    {0x3021,0x00}, //  ct_pbs_ob_width                                
    {0x3022,0x0A}, //  ct_cds_lim_start                               
    {0x3023,0x1E}, //  ct_crs_start                                   
    {0x3024,0x00}, //  ct_lp_hblk_cds_start (MSB)                     
    {0x3025,0x00}, //  ct_lp_hblk_cds_start (LSB)                     
    {0x3026,0x00}, //  ct_lp_hblk_cds_end (MSB)                       
    {0x3027,0x00}, //  ct_lp_hblk_cds_end (LSB)                       
    {0x3028,0x1A}, //  ct_rmp_off_start                               
    {0x3015,0x00}, //  ct_rmp_rst_start (MSB)                         
    {0x3016,0x84}, //  ct_rmp_rst_start (LSB)                         
    {0x3017,0x00}, //  ct_rmp_sig_start (MSB)                         
    {0x3018,0xA0}, //  ct_rmp_sig_start (LSB)                         
    {0x302B,0x10}, //  ct_cnt_margin                                  
    {0x302C,0x0A}, //  ct_rmp_per                                     
    {0x302D,0x06}, //  ct_cnt_ms_margin1                              
    {0x302E,0x05}, //  ct_cnt_ms_margin2                              
    {0x302F,0x0E}, //  rst_mx                                         
    {0x3030,0x2F}, //  sig_mx                                         
    {0x3031,0x08}, //  ct_latch_start                                 
    {0x3032,0x05}, //  ct_latch_width                                 
    {0x3033,0x09}, //  ct_hold_start                                  
    {0x3034,0x05}, //  ct_hold_width                                  
    {0x3035,0x00}, //  ct_lp_hblk_dbs_start (MSB)                     
    {0x3036,0x00}, //  ct_lp_hblk_dbs_start (LSB)                     
    {0x3037,0x00}, //  ct_lp_hblk_dbs_end (MSB)                       
    {0x3038,0x00}, //  ct_lp_hblk_dbs_end (LSB)                       
    {0x3088,0x06}, //  ct_lat_lsb_offset_start1                       
    {0x308A,0x08}, //  ct_lat_lsb_offset_end1                         
    {0x308C,0x05}, //  ct_lat_lsb_offset_start2                       
    {0x308E,0x07}, //  ct_lat_lsb_offset_end2                         
    {0x3090,0x06}, //  ct_conv_en_offset_start1                       
    {0x3092,0x08}, //  ct_conv_en_offset_end1                         
    {0x3094,0x05}, //  ct_conv_en_offset_start2                       
    {0x3096,0x21}, //  ct_conv_en_offset_end2                         
    {0x3099,0x0E}, // cds_option ([3]:crs switch disable, s3,s4 streng
    {0x3070,0x10}, // comp1_bias (default:77)                         
    {0x3085,0x11}, // comp1_bias (gain1~4)                            
    {0x3086,0x01}, // comp1_bias (gain4~8) modified 813               
                    //                                                 
    {0x3064,0x00}, // Multiple sampling(gainx8,x16)                   
    {0x3062,0x08}, // off_rst                                         
    {0x3061,0x11}, // dbr_tune_rd (default :08)                       
    {0x307B,0x20}, // dbr_tune_rgsl (default :08)                     
    {0x3068,0x00}, // RMP BP bias sampling                            
    {0x3074,0x00}, // Pixel bias sampling [2]:Default L               
    {0x307D,0x00}, // VREF sampling [1]                               
    {0x3045,0x01}, // ct_opt_l1_start                                 
    {0x3046,0x05}, // ct_opt_l1_width                                 
    {0x3047,0x78}, //                                                 
    {0x307F,0xB1}, //RDV_OPTION[5:4], RG default high                 
    {0x3098,0x01}, //CDS_OPTION[16] SPLA-II enable                    
    {0x305C,0xF6}, //lob_extension[6]                                 
    {0x306B,0x10}, //                                                 
    {0x3063,0x27}, // ADC_SAT 490mV --> 610mV                         
    {0x3400,0x01}, // GAS bypass                                      
    {0x3235,0x49}, // L/F-ADLC on                                     
    {0x3233,0x00}, // D-pedestal L/F ADLC off (1FC0h)                 
    {0x3234,0x00}, //                                                 
    {0x3300,0x0C}, //BPC bypass                                       
    //12/15 Added                                      
    {0x3203,0x45}, //ADC_OFFSET_EVEN                                  
    {0x3205,0x4D}, //ADC_OFFSET_ODD                                   
    {0x320B,0x40}, //ADC_DEFAULT                                      
    {0x320C,0x06}, //ADC_MAX                                          
    {0x320D,0xC0}, //   
                                                       
    //"Digital Bininig(Default)" 
    {0x0900,0x00},
    {0x0901,0x20},
    {0x0381,0x01},
    {0x0383,0x01},
    {0x0385,0x01},
    {0x0387,0x01},   
    
    //Analog gain
    {0x0204,0x00},
    {0x0205,0x80},
    //Integration time                   
    {0x0202,0x02},
    {0x0203,0x00},
    {0x0200,0x04},
    {0x0201,0x98},  
    //LSC size setting(Optional)          
//    {0x340B,0x00},
//    {0x340C,0x00},
//    {0x340D,0x00},
//    {0x340E,0x00},
//    {0x3401,0x50},
//    {0x3402,0x3C},
//    {0x3403,0x03},
//    {0x3404,0x33},
//    {0x3405,0x04},
//    {0x3406,0x44},
//    {0x3458,0x03},
//    {0x3459,0x33},
//    {0x345A,0x04},
//    {0x345B,0x44},
//    {0x3400,0x00},
		{0x0100,0x01},
};

static struct regval_list sensor_qsxga_regs[] = { //2576*1936 pclk=89.5MHz//2560x1920

    // ***********************************************************************        
    // 5E2 MIPI 2lane Serial IN, Serial Out Mode Set File                             
    // EXTCLK : 24MHz                                                                 
    // VT picxel clk :179.2Mhz                                                       
    // OP system clk :179.2Mhz                                                       
    // MIPI CLK :     896   Mbps/lane                                                     
    // Data Format : Bayer10                                                          
    // Target Image Size : 2560 x 1920  30fps                                              
    // History                                                                                       
    // ***********************************************************************           
          
    //$MV1[MCLK:24,Width:2560,Height:1920,Format:MIPI_RAW10,mipi_lane:2,mipi_hssettle:19,pvi_pclk_inverse:0]    
    
    {0x0100,0x00}, //stream off      
    {0x0301,0x01},
    {REG_DLY,0x22},    
     // +++++++++++++++++++++++++++//
    {0x0305,0x06}, //PLLP (def:5)                                      
    {0x0306,0x00}, //                                                  
    {0x0307,0xE0}, //PLLM (def:CCh 204d --> B3h 179d)                  
    {0x3C1F,0x00}, //PLLS                                          
    {0x0820,0x03}, // requested link bit rate mbps : (def:3D3h 979d -->
    {0x0821,0x80},                                                  
    {0x3C1C,0x58},  //dbr_div           
    {REG_DLY,0x10},                         
    {0x0114,0x01},  //MIPI Lane  
    // Size Setting  
    {0x0340,0x07}, // //frame_length_lines :2025      
    {0x0341,0xE9}, //                                 
    {0x0342,0x0B}, // //line_length_pck    :2950      
    {0x0343,0x86}, //                                 
    {0x0344,0x00}, // //x_addr_start             
    {0x0345,0x08}, //                            
    {0x0346,0x00}, // //y_addr_start             
    {0x0347,0x08}, //                            
    {0x0348,0x0A}, // //x_addr_end               
    {0x0349,0x07}, //                            
    {0x034A,0x07}, // //y_addr_end               
    {0x034B,0x87}, //                                 
    {0x034C,0x0A}, // //x_output size      :2560      
    {0x034D,0x00}, //                                 
    {0x034E,0x07}, // //y_output size      :1920      
    {0x034F,0x80}, //
    //"Digital Bininig(Default)" 
    {0x0900,0x00},
    {0x0901,0x20},
    {0x0381,0x01},
    {0x0383,0x01},
    {0x0385,0x01},
    {0x0387,0x01},   
    {0x0100,0x01},
};

static struct regval_list sensor_sxga_regs[] = { //1280*960
    // ***********************************************************************        
    // 5E2 MIPI 2lane Serial IN, Serial Out Mode Set File                             
    // EXTCLK : 24MHz                                                                 
    // VT picxel clk :179.2Mhz                                                       
    // OP system clk :179.2Mhz                                                       
    // MIPI CLK :     896   Mbps/lane                                                     
    // Data Format : Bayer10                                                          
    // Target Image Size : 1280 x 960 60fps                                              
    // History                                                                                       
    // ***********************************************************************     
               
    //$MV1[MCLK:24,Width:1280,Height:960,Format:MIPI_RAW10,mipi_lane:2,mipi_hssettle:19,pvi_pclk_inverse:0] 
    
    {0x0100,0x00}, //stream off           
    // Size Setting  
    {0x0340,0x07}, // //frame_length_lines :2025      
    {0x0341,0xE9}, //                                 
    {0x0342,0x0B}, // //line_length_pck    :2950      
    {0x0343,0x86}, //                                 
    {0x0344,0x00}, // //x_addr_start             
    {0x0345,0x08}, //                            
    {0x0346,0x00}, // //y_addr_start             
    {0x0347,0x08}, //                            
    {0x0348,0x0A}, // //x_addr_end               
    {0x0349,0x07}, //                            
    {0x034A,0x07}, // //y_addr_end               
    {0x034B,0x87}, //                                 
    {0x034C,0x05}, // //x_output size      :1280      
    {0x034D,0x00}, //                                 
    {0x034E,0x03}, // //y_output size      :960      
    {0x034F,0xC0}, //
    
    //"Digital Bininig(Default)" 
    {0x0900,0x01},
    {0x0901,0x22},
    {0x0381,0x01},
    {0x0383,0x01},
    {0x0385,0x01},
    {0x0387,0x03},
    
    {0x0100,0x01},
};

//static struct regval_list sensor_1080p_regs[] = { //1920*1080

//};

//static struct regval_list sensor_720p_regs[] = { //1280*720

//};


static struct regval_list sensor_fmt_raw[] = {
//
};

static int sensor_read(struct v4l2_subdev *sd, unsigned short reg,
    unsigned char *value)
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
    	return 0;
    while(i<array_size)
    {
        if(regs->addr == REG_DLY) {
            msleep(regs->data);
        } else {  
            LOG_ERR_RET(sensor_write(sd, regs->addr, regs->data))
        }
        i++;
        regs++;
    }
    return 0;
}
static int reg_val_show(struct v4l2_subdev *sd,unsigned short reg)
{
	unsigned char tmp;
	sensor_read(sd,reg,&tmp);
	printk("0x%x value is 0x%x\n",reg,tmp);
	return 0;
}

/* 
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	
	*value = info->exp;
	vfe_dev_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}
static int s5k5e2_sensor_vts ;

static int sensor_s_exp_gain(struct v4l2_subdev *sd, struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val,shutter,frame_length;
	unsigned char explow,exphigh;
	unsigned char gainlow=0;
	unsigned char gainhigh=0;
	struct sensor_info *info = to_state(sd);
	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	exp_val=exp_val>>4;//rounding to 1
	shutter = exp_val;
	gain_val=gain_val*2;//shift to 1/32 step

	exphigh = (unsigned char) ( (0xff00&exp_val)>>8);
	explow  = (unsigned char) ( (0x00ff&exp_val) );

	gainlow=(unsigned char)(gain_val&0xff);
	gainhigh=(unsigned char)((gain_val>>8)&0xff);
	if(shutter > s5k5e2_sensor_vts-4)
		frame_length = shutter+4;
	else
		frame_length = s5k5e2_sensor_vts;
		
    vfe_dev_dbg("frame_length = %d,%d,%d\n",frame_length,shutter,s5k5e2_sensor_vts);
    sensor_write(sd,0x0104,0x01);
	sensor_write(sd, 0x0341,( (frame_length) & 0xff));
	sensor_write(sd, 0x0340,((frame_length) >> 8));
	sensor_write(sd, 0x0203, explow);
	sensor_write(sd, 0x0202, exphigh);
	sensor_write(sd, 0x0205, gainlow);
	sensor_write(sd, 0x0204, gainhigh);
	
    sensor_write(sd,0x0104,0x00);

	//vfe_dev_dbg("s5k5e2 sensor_set_gain = %d, Done!\n", gain_val);

	info->gain = gain_val;
	info->exp = exp_val;
	return 0;
}
static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow,exphigh,tmp1,tmp2;unsigned short tmp = 0;
	struct sensor_info *info = to_state(sd);

	//vfe_dev_dbg("sensor_set_exposure = %d\n", exp_val);
	if(exp_val>0xffffff)
		exp_val=0xfffff0;
	if(exp_val<16)
		exp_val=16;	
	exp_val=(exp_val+8)>>4;//rounding to 1
	
	//printk("sensor_set_exposure real= %d\n", exp_val);
  
    exphigh = (unsigned char) ( (0xff00&exp_val)>>8);
    explow  = (unsigned char) ( (0x00ff&exp_val) );
	
    sensor_write(sd,0x0104,0x01);
	sensor_write(sd, 0x0203, explow);//coarse integration time
	sensor_write(sd, 0x0202, exphigh);	
	
    sensor_write(sd,0x0104,0x00);
	sensor_read(sd,0x0203,&tmp1);
	sensor_read(sd,0x0202,&tmp2);
	tmp = ((tmp2<<8)|tmp1);
//	printk("readout shutter =%d\n",tmp);
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
	
	gain_val=gain_val*2;
	gainlow=(unsigned char)(gain_val&0xff);
	gainhigh=(unsigned char)((gain_val>>8)&0xff);
	
    sensor_write(sd,0x0104,0x01);
	sensor_write(sd, 0x0205, gainlow);
	sensor_write(sd, 0x0204, gainhigh);	
    sensor_write(sd,0x0104,0x00);
	
//	printk("s5k5e2ya set_gain = %d\n", gain_val);
	info->gain = gain_val;
	
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	unsigned char rdval;
	
	ret=sensor_read(sd, 0x0100, &rdval);
//	printk("sensor read 0x0100 value is 0x%x",rdval);
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
            //software standby on
            ret = sensor_s_sw_stby(sd, 1);
            if(ret < 0)
                vfe_dev_err("soft stby falied!\n");
            usleep_range(10000,12000);
            //make sure that no device can access i2c bus during sensor initial or power down
            //when using i2c_lock_adpater function, the following codes must not access i2c bus before calling i2c_unlock_adapter
            cci_lock(sd);
            //standby on io
            vfe_gpio_write(sd,PWDN,CSI_GPIO_LOW);
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
            usleep_range(10000,12000);
            //standby off io
            vfe_gpio_write(sd,PWDN,CSI_GPIO_HIGH);
            usleep_range(10000,12000);
            //remember to unlock i2c adapter, so the device can access the i2c bus again
            cci_unlock(sd);        
            //software standby
            ret = sensor_s_sw_stby(sd, 0);
            if(ret < 0)
                vfe_dev_err("soft stby off falied!\n");
            usleep_range(10000,12000);
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
            vfe_gpio_write(sd,PWDN,CSI_GPIO_LOW);
            //reset on io
            vfe_gpio_write(sd,RESET,CSI_GPIO_LOW);
            usleep_range(1000,1200);
            //active mclk before power on
            vfe_set_mclk_freq(sd,MCLK);
            vfe_set_mclk(sd,ON);
            usleep_range(10000,12000);
            //power supply
            vfe_set_pmu_channel(sd,IOVDD,ON);
            vfe_set_pmu_channel(sd,AVDD,ON);
            vfe_set_pmu_channel(sd,DVDD,ON);
            vfe_set_pmu_channel(sd,AFVDD,ON);
            //standby off io
            vfe_gpio_write(sd,PWDN,CSI_GPIO_HIGH);
            usleep_range(10000,12000);
            //reset after power on
            vfe_gpio_write(sd,RESET,CSI_GPIO_HIGH);
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
            vfe_gpio_write(sd,RESET,CSI_GPIO_LOW);
			vfe_gpio_write(sd,PWDN,CSI_GPIO_LOW);
            //power supply off
            vfe_set_pmu_channel(sd,DVDD,OFF);
            vfe_set_pmu_channel(sd,AVDD,OFF);
            vfe_set_pmu_channel(sd,IOVDD,OFF);  
			vfe_set_pmu_channel(sd,AFVDD,OFF);
            //standby and reset io
            //set the io to hi-z
            //vfe_gpio_set_status(sd,RESET,0);//set the gpio to input
            //vfe_gpio_set_status(sd,PWDN,0);//set the gpio to input
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
    switch(val) {
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

    sensor_read(sd, 0x0000, &rdval);
    printk("0x0000=0x%x\n",rdval);
    if(rdval != 0x5e)
        return -ENODEV;
    sensor_read(sd, 0x0001, &rdval);
    printk("0x0001=0x%x\n",rdval);
    if(rdval != 0x20)
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
    info->width = QSXGA_WIDTH;
    info->height = QSXGA_HEIGHT;
    info->hflip = 0;
    info->vflip = 0;
    info->gain = 0;

    info->tpf.numerator = 1;            
    info->tpf.denominator = 15;    /* 30fps */    
//    reg_val_show(sd,0x0002);
//    reg_val_show(sd,0x0003);
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
		.mbus_code	= V4L2_MBUS_FMT_SGRBG10_10X1,
		.regs 		= sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp		= 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)  

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
    /* qsxga: 2576*1936 */
    {
        .width      = 2560,//QSXGA_WIDTH,
        .height     = 1920,//QSXGA_HEIGHT,
        .hoffset    = 0,
        .voffset    = 0,
        .hts        = 2950,//must over 2738, limited by sensor
        .vts        = 2025,
        .pclk 	  = 179200000,
        .mipi_bps 	= (896*1000*1000)/1,
        .fps_fixed  = 1,
        .bin_factor = 1,
        .intg_min   = 3<<4,
        .intg_max   = (2025-8)<<4,
        .gain_min   = 1<<4,
        .gain_max   = 16<<4,
        .regs       = sensor_qsxga_regs,
        .regs_size  = ARRAY_SIZE(sensor_qsxga_regs),
        .set_size   = NULL,
    },

    /* 1080P */
    {
        .width		= HD1080_WIDTH,
        .height 	= HD1080_HEIGHT,
        .hoffset	= 0,
        .voffset	= (1920-1440)/2,
        .hts        = 2950,
        .vts        = 2025,
        .pclk 	    = 179200000,
        .mipi_bps 	= (896*1000*1000)/1,
        .fps_fixed  = 1,
        .bin_factor = 1,
        .intg_min   = 3<<4,
        .intg_max   = (2025-8)<<4,
        .gain_min   = 1<<4,
        .gain_max   = 16<<4,
        .width_input	  = 2560,
		.height_input 	  = 1440,
        .regs       = sensor_qsxga_regs,
        .regs_size  = ARRAY_SIZE(sensor_qsxga_regs),
        .set_size	= NULL,
    },

    /* SXGA */
    {
        .width		= SXGA_WIDTH,
        .height 	= SXGA_HEIGHT,
        .hoffset	= 0,
        .voffset	= 0,
        .hts        = 2950,//must > 2738, limited by sensor
        .vts        = 2025,
        .pclk       = 179200000,
        .mipi_bps	= 896*1000*1000,
        .fps_fixed  = 1,
        .bin_factor = 1,
        .intg_min   = 3<<4,
        .intg_max   = (2025-8)<<4,
        .gain_min   = 1<<4,
        .gain_max   = 16<<4,
        .regs		= sensor_sxga_regs,
        .regs_size	= ARRAY_SIZE(sensor_sxga_regs),
        .set_size	= NULL,
    },

    /* 720p */
    {
        .width      = HD720_WIDTH,
        .height     = HD720_HEIGHT,
        .hoffset    = 0,
        .voffset    = (1920-1440)/2,
        .hts        = 2950,
        .vts        = 2025,
        .pclk       = 179200000,
        .mipi_bps   = (896*1000*1000)/1,
        .fps_fixed  = 1,
        .bin_factor = 1,
        .intg_min   = 3<<4,
        .intg_max   = (2025-8)<<4,
        .gain_min   = 1<<4,
        .gain_max   = 16<<4,
        .width_input    = 2560,
        .height_input   = 1440,
        .regs       = sensor_qsxga_regs,
        .regs_size  = ARRAY_SIZE(sensor_qsxga_regs),
        .set_size   = NULL,
    },
    
    /* VGA */
    {
        .width      = VGA_WIDTH,
        .height     = VGA_HEIGHT,
        .hoffset    = 0,
        .voffset    = 0,
        .hts        = 2950,//must > 2738, limited by sensor
        .vts        = 2025,
        .pclk       = 179200000,
        .mipi_bps   = 896*1000*1000,
        .fps_fixed  = 1,
        .bin_factor = 1,
        .intg_min   = 3<<4,
        .intg_max   = (2025-8)<<4,
        .gain_min   = 1<<4,
        .gain_max   = 16<<4,
        .width_input    = SXGA_WIDTH,
        .height_input   = SXGA_HEIGHT,     
        .regs       = sensor_sxga_regs,
        .regs_size  = ARRAY_SIZE(sensor_sxga_regs),
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
    cfg->flags = 0|V4L2_MBUS_CSI2_2_LANE|V4L2_MBUS_CSI2_CHANNEL_0;

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

    LOG_ERR_RET(sensor_write_array(sd, sensor_default_regs, ARRAY_SIZE(sensor_default_regs)))
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
    s5k5e2_sensor_vts = wsize->vts;
    vfe_dev_print("s_fmt set width = %d, height = %d\n",wsize->width,wsize->height);
//    reg_val_show(sd,0x0005);
//    reg_val_show(sd,0x0111);
    if(info->capture_mode == V4L2_MODE_VIDEO)
    {
        //video
    } else {
        //capture image
    }
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
    	return v4l2_ctrl_query_fill(qc, 1*32, 16*32, 1, 32);
    case V4L2_CID_EXPOSURE:
    	return v4l2_ctrl_query_fill(qc, 3*16, 65535*16, 1, 3*16);
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


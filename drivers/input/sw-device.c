/*
 *  sw_device.c - Linux kernel modules for  Detection i2c device.
 */
 
#include "sw-device.h"

static int ctp_mask = 0x0;
static u32 debug_mask = 0x00;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(ctp_mask, ctp_mask, int , S_IRUGO | S_IWUSR | S_IWGRP);

/*gsensor info*/
static struct sw_device_info gsensors[] = {
        {    "bma250", {0x18, 0x19, 0x08, 0x38}, 0x00, {0x02,0x03,0xf9}, 0},
#ifdef CONFIG_ARCH_SUN8IW8

		{     "da380", {0x27, 0x26            }, 0x01, {0x13          }, 0},
#endif
        {   "mma8452", {0x1c, 0x1d            }, 0x0d, {0x2A          }, 0},
        {   "mma7660", {0x4c                  }, 0x00, {0x00          }, 0},
        {   "mma865x", {0x1d                  }, 0x0d, {0x4A,0x5A     }, 0},
        {    "afa750", {0x3d                  }, 0x37, {0x3d,0x3c     }, 0},
        {"lis3de_acc", {0x28, 0x29            }, 0x0f, {0x33          }, 0},
        {"lis3dh_acc", {0x18, 0x19            }, 0x0f, {0x33          }, 0},
        {     "kxtik", {0x0f                  }, 0x0f, {0x05,0x08     }, 0},
        {   "dmard10", {0x18                  }, 0x00, {0x00          }, 1},
        {   "dmard06", {0x1c                  }, 0x0f, {0x06          }, 0},
        {   "mxc622x", {0x15                  }, 0x00, {0x00          }, 0},    
        {  "fxos8700", {0x1c, 0x1d, 0x1e, 0x1f}, 0x0d, {0xc7          }, 0},
        {   "lsm303d", {0x1e, 0x1d            }, 0x0f, {0x49          }, 0},    
};

#ifndef CONFIG_ARCH_SUN8IW8
/*ctp info*/
static struct sw_device_info ctps[] = {
        { "ft5x_ts", {      0x38},   0xa3, {0x55,0x08,0x02,0x06,0xa3}, 0},
        {   "gt82x", {      0x5d},  0xf7d, {0x13,0x27,0x28          }, 0},
        { "gslX680", {      0x40},   0x00, {0x00                    }, 0},
        {"gt9xx_ts", {0x14, 0x5d}, 0x8140, {0x39                    }, 0},
        {   "gt811", {      0x5d},  0x715, {0x11                    }, 0},
        {"aw5306_ts", {     0x38},   0x01, {0xA8                    }, 0},
        { "zet622x", {      0x76},   0x00, {0x00                    }, 0},
};

static struct sw_device_info lsensors[] = {
        {"ltr_501als", {0x23   },    0x87, {0x05                    }, 0},
};

static struct sw_device_info gyr_sensors[] = {
        {"l3gd20_gyr", {0x6a, 0x6b}, 0x0F, {0x00D4                  }, 0},
		{"bmg160_gyr", {0x68      }, 0x00, {0x00                    }, 0},
};

static struct sw_device_info compass_sensors[] = {
        {"lsm303d", {0x1e, 0x1d            }, 0x0f, {0x49          }, 0}, 
};


/*lsensors para_name info*/
static struct para_name ls_name = {
        "ls_para",
        "ls_used",
        "ls_list_para",
        "ls_det_used",
        "ls_twi_id",
        LSENSOR_DEVICE_KEY_NAME,
};

/*compass sensors para_name info*/
static struct para_name compass_name = {
        "compass_para",
        "compass_used",
        "compass_list_para",
        "compass_det_used",
        "compass_twi_id",
        COMPASS_SENSOR_DEVICE_KEY_NAME,
};

/*gyr para_name info*/
static struct para_name gyr_name = {
        "gy_para",
        "gy_used",
        "gy_list_para",
        "gy_det_used",
        "gy_twi_id",
        GYR_SENSOR_DEVICE_KEY_NAME,
};

/*ctp para_name info*/
static struct para_name c_name = {
        "ctp_para",
        "ctp_used",
        "ctp_list_para",
        "ctp_det_used",
        "ctp_twi_id",
        CTP_DEVICE_KEY_NAME,
};

#endif

/*gsensor para_name info*/
static struct para_name g_name = {
        "gsensor_para",
        "gsensor_used",
        "gsensor_list_para",
        "gsensor_det_used",
        "gsensor_twi_id",
        GSENSOR_DEVICE_KEY_NAME,
};


	
static struct sw_device_name d_name = {"", "", "", "", 0, 0, 0, 0};
static void sw_devices_events(struct work_struct *work);
static struct workqueue_struct *sw_wq;
static DECLARE_WORK(sw_work, sw_devices_events);

static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret=-1;
	
	msg.flags = !I2C_M_RD;
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;		
	
	ret=i2c_transfer(client->adapter, &msg,1);
	return ret;
}

static int i2c_read_bytes_addr16(struct i2c_client *client, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msgs[2];
	int ret=-1;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 2;		//data address
	msgs[0].buf   = buf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len-2;
	msgs[1].buf   = buf+2;
	
	ret=i2c_transfer(client->adapter, msgs, 2);
	return ret;
}

static bool i2c_test(struct i2c_client * client)
{
        int ret,retry;
        uint8_t test_data[1] = { 0 };	//only write a data address.
        
        for(retry=0; retry < 5; retry++)
        {
                ret = i2c_write_bytes(client, test_data, 1);	//Test i2c.
        	if (ret == 1)
        	        break;
        	msleep(10);
        }
        
        return ret==1 ? true : false;
} 

static int i2c_check_addr(char check_addr[], unsigned short address)
{
        int ret = 0;
        
        while((check_addr[ret]) && (ret < NAME_LENGTH)) {
                if(check_addr[ret] == address){
                        dprintk(DEBUG_INIT, "address:0x%x\n", check_addr[ret]);
                        return 1;
                }
                 ret++;              
        }
        
        return 0;
}

#ifndef CONFIG_ARCH_SUN8IW8
/**
 * ctp_wakeup - function
 *
 */
static int ctp_wakeup(struct gpio_config gpio, int ms)
{
	int status = 0;

	if(0 != gpio_request(gpio.gpio, NULL)) {
		printk(KERN_ERR "wakeup gpio_request is failed\n");
		return -1;
	}

	if (0 != gpio_direction_output(gpio.gpio, gpio.data)) {
		printk(KERN_ERR "wakeup gpio set err!");
		return -1;
	}

	printk("%s: !!!!gpio data %d\n", __func__, gpio.data);

	if(!(gpio.data))
		status = 1;

	if(status == 0){               
		if(ms == 0) { 
		        __gpio_set_value(gpio.gpio, 0);
		}else {
		        __gpio_set_value(gpio.gpio, 0);
		        msleep(ms);
		        __gpio_set_value(gpio.gpio, 1);
		}
	}
	if(status == 1){
		if(ms == 0) {
		        __gpio_set_value(gpio.gpio, 1);
		}else {
		        __gpio_set_value(gpio.gpio, 1); 
		        msleep(ms);
		        __gpio_set_value(gpio.gpio, 0); 
		}      
	}

	msleep(5);

	gpio_free(gpio.gpio);

	return 0;
}
#endif

static script_item_u get_para_value(char* keyname, char* subname)
{
        script_item_u	val;
        script_item_value_type_e type = 0;

        
        if((!keyname) || (!subname)) {
               printk("keyname:%s  subname:%s \n", keyname, subname);
               goto script_get_item_err;
        }
                
        memset(&val, 0, sizeof(val));
        type = script_get_item(keyname, subname, &val);
         
        if((SCIRPT_ITEM_VALUE_TYPE_INT != type) && (SCIRPT_ITEM_VALUE_TYPE_STR != type) &&
          (SCIRPT_ITEM_VALUE_TYPE_PIO != type)) {
	        goto script_get_item_err;
	}
	
        return val;
        
script_get_item_err:
        printk("keyname:%s  subname:%s ,get error!\n", keyname, subname);
        val.val = -1;
	return val;
}

static void get_pio_para(char* name[], struct gpio_config value[], int num)
{
        script_item_u	val;
        int ret = 0;
        
        if(num < 1) {
                printk("%s: num:%d ,error!\n", __func__, num);
                return;
        }
        
        while(ret < num)
        {     
                val = get_para_value(name[0],name[ret+1]); 
                if(val.val == -1) {                        
                        val.val = 0;
                        val.gpio.gpio = -1;
                }
                value[ret] = val.gpio;
	        ret++;
        }
}

/*
*********************************************************************************************************
*                                   i2c_get_chip_id_value_addr16
*
*Description: By reading chip_id register for 16 bit address, get chip_id value
*
*Arguments  :address     Register address
*Return     : result;
*             Chip_id value
*********************************************************************************************************
*/
static int i2c_get_chip_id_value_addr16(unsigned short address, struct i2c_client* temp_client)
{
        int value = -1;
        uint8_t buf[5] = {0};
        int retry = 2;
        
        if(address & 0xff00) {
                buf[0] = address >> 8;
                buf[1] = address & 0x00ff;
        }
        
        while(retry--) {
                value = i2c_read_bytes_addr16(temp_client, buf, 3);
                if(value != 2){
                        msleep(1);
                        printk("%s:read chip id error!", __func__);
                }else{
                        break;
                }
        }
        
        value = buf[2] & 0xffff;
        dprintk(DEBUG_INIT, "buf[0]:0x%x, buf[1]:0x%x, buf[2]:0x%x, value:0x%x\n",
                buf[0], buf[1], buf[2], value);
        
        return value;     
}
/*
*********************************************************************************************************
*                                   i2c_get_chip_id_value_addr8
*
*Description: By reading chip_id register for 8 bit address, get chip_id value
*
*Arguments  :address     Register address
*Return     : result;
*             Chip_id value
*********************************************************************************************************
*/
static int i2c_get_chip_id_value_addr8(unsigned short address, struct i2c_client* temp_client)
{
        int value = -1;
        int retry = 2;
        
        while(retry--) {
                value = i2c_smbus_read_byte_data(temp_client, address);
                if(value < 0){
                        msleep(1);
                }else { 
                        break;
                }
        }
        value = value & 0xffff;

        return value;
}


static int is_alpha(char chr)
{
        int ret = -1;
        
        ret = ((chr >= 'a') && (chr <= 'z') ) ? 0 : 1;
                
        return ret;
}

static int  get_device_name(char *buf, char * name)
{
        int s1 = 0, i = 0;
        int ret = -1;
        char ch = '\"';
        char tmp_name[64];
        char * str1;
        char * str2;
        
        memset(&tmp_name, 0, sizeof(tmp_name));
        if(!strlen (buf)){
                printk("%s:the buf is null!\n", __func__);
                return 0; 
        }
        
        str1 = strchr(buf, ch);
        str2 = strrchr(buf, ch);
        if((str1 == NULL) || (str2 == NULL)) {
                printk("the str1 or str2 is null!\n");
                return 1;
        }
                   
        ret = str1 - buf + 1;  
        s1 =  str2 - buf; 
        dprintk(DEBUG_INIT,"----ret : %d,s1 : %d---\n ", ret, s1);
        
        while(ret != s1)
        {
                tmp_name[i++] = buf[ret++];         
         
        }
        tmp_name[i] = '\0';
        strcpy(name, tmp_name);
        
        dprintk(DEBUG_INIT, "name:%s\n", name);
        return 1;
}  

static int get_device_name_info(struct sw_device *sw)
{
         int row_number = 0;
         
         
         row_number = sw->total_raw;
         
	 while(row_number--) {
	        dprintk(DEBUG_INIT, "config_info[%d].str_info:%s\n", row_number, 
	                sw->write_info[row_number].str_info);
	    
	        if(is_alpha(sw->write_info[row_number].str_info[0])) {
	                continue;
	        } else if(!strncmp(sw->write_info[row_number].str_info, sw->name->write_key_name, 
	                strlen(sw->name->write_key_name))){
	                        
	                dprintk(DEBUG_INIT, "info:%s, key_name:%s\n", sw->write_info[row_number].str_info,
	                         sw->name->write_key_name);
	                         
	                if(sw->write_info[0].str_id == sw->write_info[1].str_id)          
	                        sw->write_id = row_number;
	                else
	                        sw->write_id = sw->write_info[row_number].str_id;
	               
	                if(get_device_name(sw->write_info[row_number].str_info, sw->device_name)) {
	                        dprintk(DEBUG_INIT, "device_name:%s,write_id:%d\n", sw->device_name,
	                                 sw->write_id);
	                        return 0; 
	                }	                
	        }
	 }
	 return -1;
}
static int get_device_same_addr(struct sw_device *sw, int now_number)
{
        int ret = -1;
        int i = 0;
        int number = sw->support_number;
        int scan_number = 0;
               
        while(scan_number < number) {
                scan_number++;
                i = 0; 
                now_number = (now_number == number) ? 0 : now_number; 
                        
                while((sw->info[now_number].i2c_address[i++]) && (i < ADDRESS_NUMBER)) {
                        dprintk(DEBUG_INIT, "addr:0x%x, response_addr:0x%x\n", 
                                sw->info[now_number].i2c_address[i -1], sw->response_addr);	                
                        if( sw->info[now_number].i2c_address[i - 1] == sw->response_addr) {
	                        dprintk(DEBUG_INIT, "return number: %d \n", now_number);
                                return now_number;
	                }      
	                  
                } 
                
                now_number++;                  
        }
        dprintk(DEBUG_INIT, "-----not find !-----\n");
        return ret;
}

static int get_device_name_number(struct sw_device *sw)
{
        int ret = -1;
        int number = sw->support_number;
        
        if(strlen(sw->device_name)) {
                while((number)--) {
                        if (!strncmp(sw->device_name, sw->info[number].name,
                             strlen(sw->info[number].name))) {
                                dprintk(DEBUG_INIT, "number: %d \n", number);
                                return number;
                        }
                }
        }
        dprintk(DEBUG_INIT, "-----the name is null !-----\n");
        return ret;
}

static int get_device_para_value(char* keyname, char* subname)
{
        script_item_u	val;
        
        memset(&val, 0, sizeof(val));
        if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item(keyname, subname, &val)) {
	        printk("%s:get subname:%s error! val.val:%d \n", __func__, subname, val.val);
	        goto script_get_item_err;
	}
        return val.val;
        
script_get_item_err:
        dprintk(DEBUG_INIT, "keyname:%s  subname:%s ,get error!\n", keyname, subname);
	return -1;
}


static void get_detect_list(struct sw_device *sw)
{
        int i = 0;
        int val = -1;
        int number = sw->support_number;
        
        while((number)--) {
                i = 0;
                val = get_device_para_value(sw->name->detect_keyname,  sw->info[number].name);
                
                if(val == -1) {
	                printk("%s: script_get_item err.support_number = %d. \n", __func__, number);
	                continue;
	        }
	        
                while((sw->info[number].i2c_address[i++]) && (i < ADDRESS_NUMBER)) {
	                if(val == 0) {
	                        sw->info[number].i2c_address[i-1] = 0x00;      
	                }    
                }
        } 
 }


static int sw_sysconfig_get_para(struct sw_device *sw) 
{
        int ret = -1;
        int device_used = 0;
		
	dprintk(DEBUG_INIT, "========%s===================\n", __func__);
	 
        device_used = get_device_para_value(sw->name->used_keyname, sw->name->used_subname);
        
	if(1 == device_used ){               
        	sw->twi_id = get_device_para_value(sw->name->used_keyname, sw->name->twi_id_name);
        	dprintk(DEBUG_INIT, "%s: device_twi_id is %d. \n", __func__, sw->twi_id);
        
	        sw->detect_used = get_device_para_value(sw->name->detect_keyname, sw->name->detect_subname);
	        if(sw->detect_used >= 0) {
                        get_detect_list(sw);
                        ret = 0;
                } else {
                        printk("get detect_used fail!\n ");
                        ret = -1;
                }
	
	}else{
	        dprintk(DEBUG_INIT, "%s: device_unused. \n",  __func__);
		ret = -1;
	}

	return ret;
}
static int sw_analytic_write_info(char * src_string, struct sw_device *sw)
{
        int ret = -1;
        int i = 0, j = 0, k = 0;
        sw->total_raw = 0;
        
        dprintk(DEBUG_INIT, "%s:strlen(src_string):%d\n", src_string, strlen(src_string));
        if(strlen(src_string) < 16 ) {
                sw->total_raw = DEFAULT_TOTAL_ROW;
                dprintk(DEBUG_INIT, "%s: the src string is null !\n", __func__);
                ret = 0;
                return ret;
        }  
               
        while(src_string[i++]) {  
                sw->write_info[k].str_info[j++] = src_string[i-1];
                
                if(src_string[i-1] == '\n') {
                        (sw->total_raw)++; 
                        sw->write_info[k].str_info[j] = '\0';
                        sw->write_info[k].str_id = k;         
                        k += 1;
                        j = 0;
                    
                }   
        } 
        
        if(src_string[strlen(src_string)] != '\n') {
                (sw->total_raw)++; 
                sw->write_info[k].str_id = k;
        } 
                      
        dprintk(DEBUG_INIT, "%s:total_raw:%d\n", __func__, sw->total_raw);
        ret = 1;
        
        return ret;

}
static int sw_get_write_info(char * tmp, struct sw_device *sw)
{
        mm_segment_t old_fs;
        int ret;
        
        sw->filp = filp_open(FILE_DIR,O_RDWR | O_CREAT, 0666);
        if(!sw->filp || IS_ERR(sw->filp)) {
                printk("%s:open error ....IS(filp):%ld\n", __func__, IS_ERR(sw->filp));
                return -1;
        } 
        
        old_fs = get_fs();
        set_fs(get_ds());
        sw->filp->f_op->llseek(sw->filp, 0, 0);
        ret = sw->filp->f_op->read(sw->filp, tmp, FILE_LENGTH, &sw->filp->f_pos);
        
        if(ret <= 0) {
                printk("%s:read erro or read null !\n", __func__);
        }
        
        set_fs(old_fs);
        filp_close(sw->filp, NULL);
        
        return ret;

}

static int sw_set_write_info(struct sw_device *sw)
{
        mm_segment_t old_fs;
        int ret = 0, i =0;
        
        sw->filp = filp_open(FILE_DIR, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if(!sw->filp || IS_ERR(sw->filp)) {
                printk("%s:open error ....IS(filp):%ld\n", __func__,IS_ERR(sw->filp));
                return -1;
        } 
        
        old_fs = get_fs();
        set_fs(get_ds());
        
        sw->filp->f_op->llseek(sw->filp, 0, 0);
        
        while(i < sw->total_raw ) {
                dprintk(DEBUG_INIT, "write info:%s\n", sw->write_info[i].str_info);
                ret = sw->filp->f_op->write(sw->filp, sw->write_info[i].str_info,
                        strlen(sw->write_info[i].str_info), &sw->filp->f_pos);
                i++;
        }
        
        set_fs(old_fs);
        filp_close(sw->filp, NULL);
        return ret;
}

static bool sw_match_chip_id(struct sw_device_info *info, int number, int value)
{
        int i = 0;
        while(info[number].id_value[i])
        {
                if(info[number].id_value[i] == value) {
                        dprintk(DEBUG_INIT, "Chip id value equal!\n");
                        return true;
                }
                i++;
        }
        dprintk(DEBUG_INIT, "Chip id value does not match!--value:%d--\n", value);
        return false;
}

static int sw_chip_id_gain(struct sw_device *sw, int now_number)
{
        unsigned short reg_addr;
        int match_value = 0;
        int id_value = -1;
        
        reg_addr = sw->info[now_number].chip_id_reg;        
        if(reg_addr & 0xff00)
                id_value = i2c_get_chip_id_value_addr16(reg_addr, sw->temp_client);
        else
                id_value = i2c_get_chip_id_value_addr8(reg_addr, sw->temp_client);
       
        dprintk(DEBUG_INIT, "-----%s:chip_id_reg:0x%x, chip_id_value = 0x%x-----\n",
                __func__, reg_addr, id_value);
                
        match_value = sw_match_chip_id(sw->info, now_number, id_value);
        
        return match_value;
}

static int sw_chip_id_detect(struct sw_device *sw, int now_number)
{
        int result = -1;
        int same_number = 0;
        
        while (!(sw->info[now_number].id_value[0])) {                
                if(sw->info[now_number].same_flag) {
                        same_number = get_device_same_addr(sw, now_number + 1);
                        while((same_number != now_number) && (same_number != -1) &&
                              (sw->info[same_number].id_value[0])) {
                                result = sw_chip_id_gain(sw, same_number); 
                                if(result) {
                                        sw->current_number = same_number;
                                        return result; 
                                } 
                               same_number = get_device_same_addr(sw, same_number + 1);               
                        }  
                }              
                result = 1;
                dprintk(DEBUG_INIT, "-----%s:chip_id_reg value:0x%x",
                        __func__, sw->info[now_number].id_value[0]);
                return result;
        }        
        result = sw_chip_id_gain(sw, now_number);         
        return result;
}

static int sw_device_response_test(struct sw_device *sw, int now_number)
{       
        struct i2c_adapter *adap;
        int ret = 0;
        int addr_scan = 0;
       	
        adap = i2c_get_adapter(sw->twi_id);
        sw->temp_client->adapter = adap;
         
        if (!i2c_check_functionality(sw->temp_client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;
          
        if(sw->twi_id == sw->temp_client->adapter->nr){
                while((sw->info[now_number].i2c_address[addr_scan++]) &&  
                      (addr_scan < (ADDRESS_NUMBER+1))) {
                        
                        sw->temp_client->addr = sw->info[now_number].i2c_address[addr_scan - 1];
                        dprintk(DEBUG_INIT, "%s: name = %s, addr = 0x%x\n", __func__,
                                sw->info[now_number].name, sw->temp_client->addr); 
                                                        
                        if(i2c_check_addr(sw->check_addr, sw->temp_client->addr)) {
                                ret = 0;
                                continue;
                        }
   
                        ret = i2c_test(sw->temp_client);
                        if(!ret) {
                                dprintk(DEBUG_INIT, "sw->check_addr[%d]:0x%x\n", strlen(sw->check_addr),
                                         sw->temp_client->addr);
                                sw->check_addr[strlen(sw->check_addr)] = sw->temp_client->addr;                 
                                ret = 0;
                        	continue;
                        } else {   
                                sw->response_addr = sw->temp_client->addr;      	    
                                dprintk(DEBUG_INIT, "I2C connection sucess!\n");
                                break;
                        }  
                }   
        }
        
        return ret;
}
static void sw_update_write_info(struct sw_device *sw)
{
        
        if((sw->device_name == NULL) && (sw->write_id < 0)) {
                dprintk(DEBUG_INIT, "%s:key_name is null or the id is error !\n", __func__);
                return ;
        }
                       
        memset(&sw->write_info[sw->write_id].str_info, 0, sizeof(sw->write_info[sw->write_id].str_info));
        sprintf(sw->write_info[sw->write_id].str_info, "%s=\"%s\"\n", sw->name->write_key_name, sw->device_name); 
        
}
static void sw_i2c_test(struct sw_device *sw)
{        
        int number = sw->support_number;
        int now_number = -1;
        int scan_number = 0;
        int is_find = 0;
        int ret = -1;
        
        
        now_number = get_device_name_number(sw);
        if(now_number < 0)
                now_number = 0;
                
        dprintk(DEBUG_INIT, "number:%d now_number:%d,scan_number:%d\n", number,
                now_number, scan_number);
                
        while(scan_number < number) {
                dprintk(DEBUG_INIT, "scan_number:%d, now_number:%d\n", 
                        scan_number, now_number);
                scan_number++;
                now_number = (now_number == number) ? 0 : now_number;
               
                ret = sw_device_response_test(sw, now_number);
                if(!ret) {
                        now_number++; 
        	        continue;
        	}   
        	sw->current_number = now_number;
                if(sw_chip_id_detect(sw, now_number)) {                        
                        is_find = 1;
                        break;
                }
                        
                now_number++; 
                is_find = 0;
         
        }
        
        if(is_find == 1) {
                dprintk(DEBUG_INIT, "from copy name:%s, strlen(name):%d\n", 
                        sw->info[sw->current_number].name, strlen(sw->device_name));
                        
                if((strncmp(sw->info[sw->current_number].name, sw->device_name, 
                   strlen(sw->device_name))) || (!strlen(sw->device_name))) {
                        sw->write_flag = 1;
                        memset(&sw->device_name, 0, sizeof(sw->device_name));
                        strcpy(sw->device_name, sw->info[sw->current_number].name);
                        sw_update_write_info(sw);
                        
                }
                
                dprintk(DEBUG_INIT, "%s: write_key_name:%s\n", __func__, sw->name->write_key_name);
                if(!strncmp(sw->name->write_key_name, "ctp", 3)) {               
                        strcpy(d_name.c_name, sw->device_name);
                        d_name.c_addr = sw->response_addr;
                } else if(!strncmp(sw->name->write_key_name, "gsensor", 7)) {
                        strcpy(d_name.g_name, sw->device_name);
                        d_name.g_addr = sw->response_addr;
                }
        }
}

static int sw_device_detect_start(struct sw_device *sw)
{        
        char tmp[FILE_LENGTH];
       
        /*step1: Get sysconfig.fex profile information*/
        memset(&tmp, 0, sizeof(tmp));
        if(sw_sysconfig_get_para(sw) < 0) {
                printk("get sysconfig para erro!\n");
                return -1;
        }
        
        if(sw->detect_used) {
                /*step 2:Read the device.info file information ,get device name!*/
                if(ctp_mask != 1) {                       
                        if(sw_get_write_info(tmp, sw) <= 0) {
                                sw_set_write_info(sw);
                                printk("get write info erro!\n");
                        } else {
                                if(!sw_analytic_write_info(tmp, sw)) {
                                        printk("analytic_write_info erro!\n");
                	                sw_set_write_info(sw);
                                }                       
                        }
                        get_device_name_info(sw);
                }
                
                /*step 3: The i2c address detection equipment, find the device used at present.*/                
                sw_i2c_test(sw);
                
                /*step 4:Update the device.info file information*/
                if(ctp_mask != 1) {                        
                        dprintk(DEBUG_INIT, "write_flag:%d\n", sw->write_flag);
                        if(sw->write_flag) {
                                sw_set_write_info(sw);       
                        }
                }
        }    
        
        return 0;           
}

static int sw_register_device_detect(struct sw_device_info info[], struct para_name *name,
                                     int number)
{
        struct sw_device *sw;        
        struct i2c_client *client;
        
        dprintk(DEBUG_INIT, "[sw_device]:%s begin!\n", __func__);

        sw = kzalloc(sizeof(*sw), GFP_KERNEL);
	if (!sw){
	        printk("allocate data fail!\n");
		return -ENOMEM;
	}
	
	        
	client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!client){
		printk("allocate client fail!\n");
		kfree(sw);
                return -ENOMEM;        
	}
		
	sw->temp_client = client;      
	sw->info = info;
	sw->name = name; 
	sw->support_number = number;  
	sw->total_raw = DEFAULT_TOTAL_ROW;  
	
	strcpy(sw->write_info[0].str_info, NOTE_INFO1);
        strcpy(sw->write_info[1].str_info, NOTE_INFO2);
        sprintf(sw->write_info[2].str_info, "%s=\"\"\n", GSENSOR_DEVICE_KEY_NAME);
        sprintf(sw->write_info[3].str_info, "%s=\"\"\n", CTP_DEVICE_KEY_NAME);
        sprintf(sw->write_info[4].str_info, "%s=\"\"\n", LSENSOR_DEVICE_KEY_NAME);
        sprintf(sw->write_info[5].str_info, "%s=\"\"\n", GYR_SENSOR_DEVICE_KEY_NAME);
		sprintf(sw->write_info[6].str_info, "%s=\"\"\n", COMPASS_SENSOR_DEVICE_KEY_NAME);
        
        sw_device_detect_start(sw);
        
        kfree(sw);
        kfree(client);
        
        dprintk(DEBUG_INIT, "[sw_device]:%s end!\n", __func__);
	
	return 0; 
}


static void sw_devices_events(struct work_struct *work)
{
        int ret = -1;
        int device_number = 0;
        struct gpio_config value;
        char *name[] = {"ctp_para", "ctp_wakeup"};
        
        get_pio_para(name, &value, 1);
        
        dprintk(DEBUG_INIT, "[sw_device]:%s begin!\n", __func__);
        
        if(ctp_mask != 1) {
                device_number = (sizeof(gsensors)) / (sizeof(gsensors[0]));
                ret = sw_register_device_detect(gsensors, &g_name, device_number);
                if(ret < 0)
                        printk("gsensor detect fail!\n");
#ifndef CONFIG_ARCH_SUN8IW8
                device_number = (sizeof(lsensors)) / (sizeof(lsensors[0]));
                ret = sw_register_device_detect(lsensors, &ls_name, device_number);
                if(ret < 0)
                        printk("lsensor detect fail!\n");

                device_number = (sizeof(gyr_sensors)) / (sizeof(gyr_sensors[0]));
                ret = sw_register_device_detect(gyr_sensors, &gyr_name, device_number);
                if(ret < 0)
                        printk("gyr detect fail!\n");

				device_number = (sizeof(compass_sensors)) / (sizeof(compass_sensors[0]));
                ret = sw_register_device_detect(compass_sensors, &compass_name, device_number);
                if(ret < 0)
                        printk("compass detect fail!\n");
#endif				
        }
#ifndef CONFIG_ARCH_SUN8IW8
        
        device_number = (sizeof(ctps)) / (sizeof(ctps[0]));
        ctp_wakeup(value, 20);
        msleep(50);
        ret = sw_register_device_detect(ctps, &c_name, device_number);        
        if(ret < 0)
                printk("ctp detect fail!\n"); 
#endif		
        dprintk(DEBUG_INIT, "[sw_device]:%s end!\n", __func__);       
}	

static ssize_t sw_device_gsensor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{   
        ssize_t cnt = 0;
        dprintk(DEBUG_INIT, "[sw_device]:%s: device name:%s\n", __func__, d_name.g_name);
        
        cnt += sprintf(buf + cnt,"device name:%s\n" ,d_name.g_name); 
        cnt += sprintf(buf + cnt,"device addr:0x%x\n" ,d_name.g_addr);
        return cnt;

}

static ssize_t sw_device_gsensor_store(struct device *dev,struct device_attribute *attr,
		const char *buf, size_t size)
{
        return 0;
}

static ssize_t sw_device_ctp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{   
         ssize_t cnt = 0;
         dprintk(DEBUG_INIT, "[sw_device]:%s: write_key_name:%s\n", __func__, d_name.c_name);
         if(ctp_mask != 1) {
                cnt += sprintf(buf + cnt,"device name:%s\n" ,d_name.c_name); 
                cnt += sprintf(buf + cnt,"device addr:0x%x\n" ,d_name.c_addr);
        } else 
                return sprintf(buf, d_name.c_name);
                
        return cnt;

}

static ssize_t sw_device_ctp_store(struct device *dev,struct device_attribute *attr,
		const char *buf, size_t size)
{
        return 0;
}

static DEVICE_ATTR(gsensor, S_IRUGO|S_IWUSR|S_IWGRP, sw_device_gsensor_show, sw_device_gsensor_store);
static DEVICE_ATTR(ctp, S_IRUGO|S_IWUSR|S_IWGRP, sw_device_ctp_show, sw_device_ctp_store);

static struct attribute *sw_device_attributes[] = {
        &dev_attr_gsensor.attr,
        &dev_attr_ctp.attr,
        NULL
};

static struct attribute_group dev_attr_group = {
	.attrs = sw_device_attributes,
};

static const struct attribute_group *dev_attr_groups[] = {
	&dev_attr_group,
	NULL,
};

static void sw_device_release(struct device *dev)
{
    
}

static struct device sw_device_detect = {
        .init_name = "sw_device",
        .release = sw_device_release,
};	
		
static int __init sw_device_init(void)
{        	
	int err = 0;
	dprintk(DEBUG_INIT, "[sw_device]:%s begin!\n", __func__);
	sw_wq = create_singlethread_workqueue("sw_wq");
	if (sw_wq == NULL) {
		printk("create sw_wq fail!\n");
		return 0;
	}

	queue_work(sw_wq, &sw_work);
	
	sw_device_detect.groups = dev_attr_groups;
	err = device_register(&sw_device_detect);
	if (err) {
		printk("%s register camera detect driver as misc device error\n", __FUNCTION__);
	}
	dprintk(DEBUG_INIT, "[sw_device]:%s begin!\n", __func__);
	return 0;
}

static void __exit sw_device_exit(void)
{
        printk(" sw device driver exit!\n"); 
        device_unregister(&sw_device_detect);   
    
}
/*************************************************************************************/

MODULE_AUTHOR("Ida yin");
MODULE_DESCRIPTION("Detection i2c device driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

module_init(sw_device_init);
module_exit(sw_device_exit);

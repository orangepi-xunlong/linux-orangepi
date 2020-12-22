/* Vibrator driver for sun8i platform
 * ported from msm pmic vibrator driver
 *  by tom  <richard.liu@allwinner.com>
 *
 * Copyright (C) 2011 ReuuiMlla Technology.
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/dma-mapping.h>  

#include "vib_sys_inf.h"

struct timer_list 	g_timer;
static ktime_t 		g_ktime;
static struct hrtimer 	g_hrtimer;
static struct mutex vib_mutex;
struct completion vib_completion;

static int g_vib_sel = 0;
static int g_direction = 0;
static int g_times = 0;

unsigned int vib_m0_used = 0 ;
unsigned int vib_m1_used = 0 ;
//script_item_u	*vib_m0_used;
//script_item_u	*vib_m1_used;


unsigned char table1[]={0x03,0x06,0x0c,0x09}; /*正转表*/
unsigned char table2[]={0x03,0x09,0x0c,0x06}; /*反转表*/

static script_gpio_set_t vibe_gpio[8];
static unsigned int vibe_gpio_handler[8];
static  char *name_mo[] ={"mo_1","mo_2","mo_3","mo_4"};
static  char *name_m1[] ={"m1_1","m1_2","m1_3","m1_4"};

static char *limit_name1[] ={"vibrator_M0","vibrator_M0","vibrator_M1","vibrator_M1"};
static char *limit_name2[] ={"pan1","pan2","til1","til2"};
static script_gpio_set_t limit_gpio[4];
static unsigned int limit_handler[4];/*对应的4个限位开关 "pan1","pan2","til1","til2"  */


static ssize_t my_attr_show(struct kobject *kobj, struct attribute *attr, char *buf) 
{ 
	struct my_attribute *my_attr; 
    ssize_t ret = -EIO; 
    my_attr = container_of(attr, struct my_attribute, attr); 
    if (my_attr->show) 
    ret = my_attr->show(container_of(kobj, struct my_kobj, kobj), my_attr, buf); 

	return ret; 
} 

static ssize_t my_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count) 
{ 
	struct my_attribute *my_attr; 
    ssize_t ret = -EIO; 
    my_attr = container_of(attr, struct my_attribute, attr); 
    if (my_attr->store) 
		ret = my_attr->store(container_of(kobj, struct my_kobj, kobj), my_attr, buf, count); 
	return ret; 
} 

const struct sysfs_ops my_sysfs_ops = { 
	.show = my_attr_show, 
	.store = my_attr_store, 
}; 

void obj_release(struct kobject *kobj) 
{ 
	struct my_kobj *obj = container_of(kobj, struct my_kobj, kobj); 
    printk(KERN_INFO "obj_release %s\n", kobject_name(&obj->kobj)); 
    kfree(obj); 
} 

static struct kobj_type my_ktype = { 
	.release = obj_release, 
	.sysfs_ops = &my_sysfs_ops, 
}; 

ssize_t m1_show(struct my_kobj *obj, struct my_attribute *attr, char *buffer) 
{ 
	return sprintf(buffer, "%d\n", obj->val); 
} 

ssize_t m1_store(struct my_kobj *obj, struct my_attribute *attr, const char *buffer, size_t size) 
{
	char * p = buffer;
	vib_direction direction = VIB_ROUND_FORWARD;

	if('b'==*buffer || 'B' ==*buffer)
	{
		direction = VIB_ROUND_BACK;	
		p += 1;
	}

	if('a'==*buffer || 'A' ==*buffer)
	{
		if( (vib_m1_used == 1) )
		vib_round_always(VIB_M1_SELECT,direction);
	}
	sscanf(p, "%d", &obj->val); 
	obj->val -= obj->val % 4;
	VIB_DEBUG("m1_store %d  %d  %d\n",VIB_M1_SELECT,direction,obj->val);
	if( (vib_m1_used == 1) )
	vib_round_times(VIB_M1_SELECT ,direction,obj->val);
	return size; 
} 

ssize_t m0_show(struct my_kobj *obj, struct my_attribute *attr, char *buffer) 
{ 
	return sprintf(buffer, "%d\n", obj->val); 
} 

ssize_t m0_store(struct my_kobj *obj, struct my_attribute *attr, const char *buffer, size_t size) 
{ 
	char * p = buffer;
	vib_direction direction = VIB_ROUND_FORWARD;

	if('b'==*buffer || 'B' ==*buffer)
	{
		direction = VIB_ROUND_BACK;	
		p += 1;
	}
	if('a'==*buffer || 'A' ==*buffer)
	{
		printk("@vib_round_always \n");
		if( (vib_m0_used == 1) )
		vib_round_always(VIB_M0_SELECT,direction);
	}
	sscanf(p, "%d", &obj->val); 
	obj->val -= obj->val % 4;
	VIB_DEBUG("m0_store %d  %d  %d\n",VIB_M0_SELECT,direction,obj->val);
	if( (vib_m0_used == 1) )
	vib_round_times(VIB_M0_SELECT,direction,obj->val);
	return size; 

} 

struct my_attribute name_attribute = __ATTR(vib_m1, 0666, m1_show, m1_store); 
struct my_attribute val_attribute = __ATTR(vib_m0, 0666, m0_show, m0_store); 

struct attribute *my_attrs[] = { 
	&name_attribute.attr, 
	&val_attribute.attr, 
	NULL, 
}; 

struct attribute_group my_group = { 
	.name = "mygroup", 
	.attrs = my_attrs, 
}; 

/*返回0，表示已经转到头了*/
int is_vib_unblock(vib_select vib_sel,vib_direction direction)
{
    int re = 1;
	if(VIB_M0_SELECT == vib_sel && direction == VIB_ROUND_FORWARD) {
		re = gpio_read_one_pin_value(limit_handler[0],NULL);
	}
	else if (VIB_M0_SELECT == vib_sel && direction == VIB_ROUND_BACK) {
		re = gpio_read_one_pin_value(limit_handler[1],NULL);
	}
	else if (VIB_M1_SELECT == vib_sel && direction == VIB_ROUND_FORWARD) {
		re = gpio_read_one_pin_value(limit_handler[3],NULL);
	}
	else if (VIB_M1_SELECT == vib_sel && direction == VIB_ROUND_BACK) {
		re = gpio_read_one_pin_value(limit_handler[2],NULL);
	}
	return re;
}

void vib_round_always(vib_select vib_sel,vib_direction direction)
{

}
void vib_senddata(vib_select vib_sel,unsigned char value){
    int i;
	int index = 0;
   
	if(VIB_M1_SELECT == vib_sel)
		index = 4;
	else if(VIB_M0_SELECT == vib_sel)
		index = 0;
	else 
		return;
	
	switch (value) {
		case 0x00:
			for(i=0;i<4;i++)
			gpio_write_one_pin_value(vibe_gpio_handler[i + index], 1, NULL);
			
			break;
		case 0x03:
			gpio_write_one_pin_value(vibe_gpio_handler[0 + index], 0, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[1 + index], 0, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[2 + index], 1, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[3 + index], 1, NULL);
			break;
		case 0x06:
			gpio_write_one_pin_value(vibe_gpio_handler[0 + index], 1, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[1 + index], 0, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[2 + index], 0, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[3 + index], 1, NULL);
			break;
		case 0x09:
			gpio_write_one_pin_value(vibe_gpio_handler[0 + index], 0, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[1 + index], 1, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[2 + index], 1, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[3 + index], 0, NULL);
			break;
		case 0x0c:
			gpio_write_one_pin_value(vibe_gpio_handler[0 + index], 1, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[1 + index], 1, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[2 + index], 0, NULL);
			gpio_write_one_pin_value(vibe_gpio_handler[3 + index], 0, NULL);
			break;
		default:
			break;
			
		}

}

static int motor_cnt = 0;
int motor_action(vib_select vib_sel,vib_direction direction)
{
	int ret = 0;
	
	if(is_vib_unblock(vib_sel,direction) == 0) {
	  	VIB_DEBUG("vibrator %d , %d block\n",vib_sel,direction);
		ret = 1;
		motor_cnt = 0 ;
		return ret;
	 }
	   
    if(VIB_ROUND_FORWARD == direction) {
		vib_senddata(vib_sel,(unsigned char)table1[motor_cnt]);
	}
	else {
		vib_senddata(vib_sel,(unsigned char)table2[motor_cnt]);
	}
    motor_cnt++;
	if( motor_cnt >= 4) {
		motor_cnt = 0;
	}
	return 0 ;
}

static enum hrtimer_restart hrtimer_handle(struct hrtimer *cur_timer)
{
	int ret = -1;
	int is_block = 0; 
	static int cnt = 0;
	int interv_us = 2000;
	int i = 0;
	
	if( cnt++ < g_times) {
	
		is_block = motor_action(g_vib_sel,g_direction );
		if(is_block != 0) {
			g_times = 0;
		}
		g_hrtimer.function = &hrtimer_handle;
		g_ktime = ktime_set(0, interv_us * 1000);
		ret = hrtimer_start(&g_hrtimer, g_ktime, HRTIMER_MODE_REL);
	} else {
		complete(&vib_completion);
		cnt = 0;
		motor_cnt = 0;
		VIB_DEBUG("@set all pin low\n");
        for(i = 0;i < 8; i++) {

			gpio_write_one_pin_value(vibe_gpio_handler[i], 0, NULL);
			mdelay(10);
#if 0			
			if(g_vib_sel == 0 && (i < 4)) {
				gpio_write_one_pin_value(vibe_gpio_handler[i], 0, NULL);
				mdelay(10);
			}
			else if( (g_vib_sel == 1) && ( i >= 4) ) {
				gpio_write_one_pin_value(vibe_gpio_handler[i], 0, NULL);
				mdelay(10);
			}
#endif			
		}
	}
    return HRTIMER_NORESTART;
}

unsigned int  hrtimer_function(unsigned int  interv_us)
{
	int ret = -1;

    g_ktime = ktime_set(0, interv_us * 1000);
	g_hrtimer.function = &hrtimer_handle;
    ret = hrtimer_start(&g_hrtimer, g_ktime, HRTIMER_MODE_REL);
//	VIB_DEBUG(" %s: hrtimer_start return %d\n", __func__, ret);
    return 0;
}

void vib_round_times(vib_select vib_sel,vib_direction direction,int times)
{
	// times :2600 one round
	int i = 0;
    vib_senddata(vib_sel,(unsigned char)0x00);
	mdelay(4);
	g_vib_sel =  vib_sel;
	g_direction = direction ;
	g_times = times;
	if(motor_action(vib_sel,direction) == 0) {
		hrtimer_function(2000);		
		wait_for_completion(&vib_completion);
	}		
	else {
			for(i = 0; i < 4;i++) {	
				if(VIB_M0_SELECT == vib_sel) {
					gpio_write_one_pin_value(vibe_gpio_handler[i], 0, NULL);	
					msleep(10);	
				}
			    else if(VIB_M1_SELECT == vib_sel) {
						gpio_write_one_pin_value(vibe_gpio_handler[i+4], 0, NULL);	
					    msleep(10);
					}
				}
		}
						
}


struct my_kobj *obj; 
static int __init sun8i_vibrator_init(void)
{

	int err = -1;
    int i = 0;
	int retval; 
    pr_info("enter sun8i_vibrator_init\n");
	if(SCIRPT_ITEM_VALUE_TYPE_INT != (err = script_get_item("vibrator_M0", "vib_used", &vib_m0_used))) {
		pr_err("%s: script_parser_fetch fail.ret = %d. \n", __func__, err);
	} 
	if(SCIRPT_ITEM_VALUE_TYPE_INT != (err = script_get_item("vibrator_M1", "vib_used", &vib_m1_used))){
	    pr_err("%s: script_parser_fetch fail.ret = %d. \n", __func__, err);
	}

	if(1 != vib_m0_used) {
		pr_err("%s: mo_unused. \n",  __func__);		
		return err;
	}
	if(1 != vib_m1_used)
	{
		pr_err("%s: m1_unused. \n",  __func__);
		return err;
	}
		
	
	mutex_init(&vib_mutex);
	hrtimer_init(&g_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	init_completion(&vib_completion);
	g_hrtimer.function = &hrtimer_handle;
		
	for (i = 0; i<4;i++){
		err = vib_sys_script_get_item("vibrator_M0", name_mo[i],
						(int *)&vibe_gpio[i],sizeof(script_gpio_set_t)/sizeof(__u32));
        vibe_gpio_handler[i] = vib_sys_gpio_request(&vibe_gpio[i], 1);
		if(!vibe_gpio_handler[i])
			gpio_write_one_pin_value(vibe_gpio_handler[i], 0, NULL);
        err = vib_sys_script_get_item("vibrator_M1", name_m1[i],
						(int *)&vibe_gpio[i+4],sizeof(script_gpio_set_t)/sizeof(__u32));
        VIB_DEBUG("vib_sys_script_get_item  vibrator_M1:   %s ",name_m1[i]);
       
		vibe_gpio_handler[i+4] = vib_sys_gpio_request(&vibe_gpio[i+4], 1);;
		if(!vibe_gpio_handler[i+4])
			gpio_write_one_pin_value(vibe_gpio_handler[i+4], 0, NULL);
			
		err = vib_sys_script_get_item(limit_name1[i], limit_name2[i],
						(int *)&limit_gpio[i],sizeof(script_gpio_set_t)/sizeof(__u32));
       VIB_DEBUG("vib_sys_script_get_item vibrator_M0:   %s \n",limit_name2[i]);
       limit_handler[i] = vib_sys_gpio_request(&limit_gpio[i], 1);

		}


	obj = kmalloc(sizeof(struct my_kobj), GFP_KERNEL); 
	if (!obj) { 
		err = -ENOMEM;
		goto exit;
	} 
	
	obj->val = 1; 
	memset(&obj->kobj, 0, sizeof(struct kobject)); 
	kobject_init_and_add(&obj->kobj, &my_ktype, NULL, "vibrator"); 
	retval = sysfs_create_files(&obj->kobj, (const struct attribute **)my_attrs); 
	if (retval) { 
		kobject_put(&obj->kobj); 
		return retval; 
	}

	
	return 0; 
exit:
	pr_info("vib script_parser_fetch_fail\n");

	return err;
}

static void __exit sun8i_vibrator_exit(void)
{
	int i;
	for(i=0;i<8;i++)
		vib_sys_gpio_release(vibe_gpio_handler[i], 0);
}
module_init(sun8i_vibrator_init);
module_exit(sun8i_vibrator_exit);

MODULE_DESCRIPTION("timed output vibrator device for sun8i");
MODULE_LICENSE("GPL");


/*
 * Battery charger driver for X-POWERS AXP28X
 *
 * Copyright (C) 2014 X-POWERS Ltd.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifdef CONFIG_AW_AXP81X
#include "axp81x/axp81x-sply.h"
#include "axp81x/axp81x-common.h"
#elif defined (CONFIG_AW_AXP19)
#include "axp19/axp19-sply.h"
#include "axp19/axp19-common.h"
#endif

int axp_debug = 0x0;
int vbus_curr_limit_debug = 1;
static int reg_debug = 0x0;

static ssize_t axpdebug_store(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	int var;
	var = simple_strtoul(buf, NULL, 16);
	printk("%s: var=%d\n", __func__, var);
	if(var)
		axp_debug = var;
	else
		axp_debug = 0;
	return count;
}

static ssize_t axpdebug_show(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "bat-debug value is %d\n", axp_debug);
}

static ssize_t axp_regdebug_store(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	int var;
	var = simple_strtoul(buf, NULL, 16);
	if(var)
		reg_debug = var;
	else
		reg_debug = 0;
	return count;
}

static ssize_t axp_regdebug_show(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "reg-debug value is 0x%x\n", reg_debug);
}

void axp_reg_debug(int reg, int len, uint8_t *val)
{
	int i = 0;
	if (reg_debug != 0) {
		for (i=0; i<len; i++) {
			if (reg+i == reg_debug)
				printk(KERN_ERR "###***axp_reg 0x%x write value 0x%x\n", reg_debug, *(val+i));
		}
	}
	return;
}

#ifdef CONFIG_AW_AXP19
static struct class_attribute axppower_class_attrs[] = {
	__ATTR(axpdebug,S_IRUGO|S_IWUSR,axpdebug_show,axpdebug_store),
	__ATTR(regdebug,S_IRUGO|S_IWUSR,axp_regdebug_show,axp_regdebug_store),
	__ATTR_NULL
};

#else
static int long_key_power_off = 1;

static ssize_t vbuslimit_store(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	if(buf[0] == '1')
		vbus_curr_limit_debug = 1;
	else
		vbus_curr_limit_debug = 0;
	return count;
}

static ssize_t vbuslimit_show(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "vbus curr limit value is %d\n", vbus_curr_limit_debug);
}
/*for long key power off control added by zhwj at 20130502*/
static ssize_t longkeypoweroff_store(struct class *class,
			struct class_attribute *attr,	const char *buf, size_t count)
{
	uint8_t data = 0;
	if(buf[0] == '1'){
		long_key_power_off = 1;
	}
	else{
		long_key_power_off = 0;
	}
	/*for long key power off control added by zhwj at 20130502*/
	data |= (long_key_power_off << 3);
	axp_update(axp_charger->master, AXP_POK_SET, data, 0x08);
	return count;
}

static ssize_t longkeypoweroff_show(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	return sprintf(buf, "long key power off value is %d\n", long_key_power_off);
}

static ssize_t out_factory_mode_show(struct class *class,
    struct class_attribute *attr, char *buf)
{
	uint8_t addr = AXP_BUFFERC;
	uint8_t data;
	axp_read(axp_charger->master, addr , &data);
	return sprintf(buf, "0x%x\n",data);
}

static ssize_t out_factory_mode_store(struct class *class,
        struct class_attribute *attr, const char *buf, size_t count)
{
	uint8_t addr = AXP_BUFFERC;
	uint8_t data;
	int var;
	var = simple_strtoul(buf, NULL, 10);
	if(var){
	  data = 0x0d;
	  axp_write(axp_charger->master, addr , data);
	}
	else{
	  data = 0x00;
	  axp_write(axp_charger->master, addr , data);
	}
	return count;
}

static struct class_attribute axppower_class_attrs[] = {
	__ATTR(vbuslimit,S_IRUGO|S_IWUSR,vbuslimit_show,vbuslimit_store),
	__ATTR(axpdebug,S_IRUGO|S_IWUSR,axpdebug_show,axpdebug_store),
	__ATTR(regdebug,S_IRUGO|S_IWUSR,axp_regdebug_show,axp_regdebug_store),
	__ATTR(longkeypoweroff,S_IRUGO|S_IWUSR,longkeypoweroff_show,longkeypoweroff_store),
	__ATTR(out_factory_mode,S_IRUGO|S_IWUSR,out_factory_mode_show,out_factory_mode_store),
	__ATTR_NULL
};
#endif
struct class axppower_class = {
	.name = "axppower",
	.class_attrs = axppower_class_attrs,
};

static ssize_t chgen_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 uint8_t val;

	 axp_read(charger->master, AXP_CHARGE_CONTROL1, &val);
	 spin_lock(&charger->charger_lock);
	 charger->chgen  = val >> 7;
	 spin_unlock(&charger->charger_lock);
	 return sprintf(buf, "%d\n",charger->chgen);
}

static ssize_t chgen_store(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 int var;

	 var = simple_strtoul(buf, NULL, 10);
	 if(var){
		spin_lock(&charger->charger_lock);
		charger->chgen = 1;
		spin_unlock(&charger->charger_lock);
		axp_set_bits(charger->master,AXP_CHARGE_CONTROL1,0x80);
	 }else{
	 	spin_lock(&charger->charger_lock);
		charger->chgen = 0;
		spin_unlock(&charger->charger_lock);
		axp_clr_bits(charger->master,AXP_CHARGE_CONTROL1,0x80);
	 }
	 return count;
}

static ssize_t chgmicrovol_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 uint8_t val;

	 axp_read(charger->master, AXP_CHARGE_CONTROL1, &val);
	 spin_lock(&charger->charger_lock);
	 switch ((val >> 5) & 0x03){
		 case 0: charger->chgvol = CHARGE_VOLTAGE_LEVEL0;break;
		 case 1: charger->chgvol = CHARGE_VOLTAGE_LEVEL1;break;
		 case 2: charger->chgvol = CHARGE_VOLTAGE_LEVEL2;break;
		 case 3: charger->chgvol = CHARGE_VOLTAGE_LEVEL3;break;
	 }
	 spin_unlock(&charger->charger_lock);
	 return sprintf(buf, "%d\n",charger->chgvol);
}

static ssize_t chgmicrovol_store(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 int var = 0;
	 uint8_t tmp = 0, val = 0;
	 
	 var = simple_strtoul(buf, NULL, 10);
	 switch(var){
		 case CHARGE_VOLTAGE_LEVEL0:tmp = 0;break;
		 case CHARGE_VOLTAGE_LEVEL1:tmp = 1;break;
		 case CHARGE_VOLTAGE_LEVEL2:tmp = 2;break;
		 case CHARGE_VOLTAGE_LEVEL3:tmp = 3;break;
		 default:  tmp = 4;break;
	 }
	 if(tmp < 4){
		spin_lock(&charger->charger_lock);
		charger->chgvol = var;
		spin_unlock(&charger->charger_lock);
		val |= tmp << 5;
		axp_update(charger->master, AXP_CHARGE_CONTROL1, val, 0x60);
	 }
	 return count;
}

static ssize_t chgintmicrocur_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 uint8_t val;

	 axp_read(charger->master, AXP_CHARGE_CONTROL1, &val);
	 spin_lock(&charger->charger_lock);
	 charger->chgcur = (val & 0x0F) * CHARGE_CURRENT_STEP +CHARGE_CURRENT_MIN;
	 spin_unlock(&charger->charger_lock);
	 return sprintf(buf, "%d\n",charger->chgcur);
}

static ssize_t chgintmicrocur_store(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 int var = 0;
	 uint8_t val = 0,tmp = 0;

	 var = simple_strtoul(buf, NULL, 10);
	 if(var >= CHARGE_CURRENT_MIN && var <= CHARGE_CURRENT_MAX){
		 tmp = (var -CHARGE_CURRENT_STEP)/(CHARGE_CURRENT_STEP);
		 spin_lock(&charger->charger_lock);
		 charger->chgcur = tmp *CHARGE_CURRENT_STEP + CHARGE_CURRENT_MIN;
		 spin_unlock(&charger->charger_lock);
		 val |= tmp;
		 axp_update(charger->master, AXP_CHARGE_CONTROL1, val, 0x0F);
	 }
	 return count;
}

static ssize_t chgendcur_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 uint8_t val;

	 axp_read(charger->master, AXP_CHARGE_CONTROL1, &val);
	 spin_lock(&charger->charger_lock);
	 charger->chgend = ((val >> 4)& 0x01)? CHARGE_END_LEVEL1 : CHARGE_END_LEVEL0;
	 spin_unlock(&charger->charger_lock);
	 return sprintf(buf, "%d\n",charger->chgend);
}

static ssize_t chgendcur_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var;

	var = simple_strtoul(buf, NULL, 10);
	if(var == CHARGE_END_LEVEL0 ){
		 axp_clr_bits(charger->master ,AXP_CHARGE_CONTROL1,0x10);
	}else if (var == CHARGE_END_LEVEL1){
		 axp_set_bits(charger->master ,AXP_CHARGE_CONTROL1,0x10);
	}
	spin_lock(&charger->charger_lock);
	charger->chgend = var;
	spin_unlock(&charger->charger_lock);
	return count;
}

static ssize_t chgpretimemin_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;

	axp_read(charger->master,AXP_CHARGE_CONTROL2, &val);
	spin_lock(&charger->charger_lock);
	charger->chgpretime = (val >> 6) * CHARGE_PRETIME_STEP + CHARGE_PRETIME_MIN;
	spin_unlock(&charger->charger_lock);
	return sprintf(buf, "%d\n",charger->chgpretime);
}

static ssize_t chgpretimemin_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var  = 0;
	uint8_t tmp = 0,val = 0;

	var = simple_strtoul(buf, NULL, 10);
	if(var >= CHARGE_PRETIME_MIN && var <= CHARGE_PRETIME_MAX){
		tmp = (var - CHARGE_PRETIME_MIN)/CHARGE_PRETIME_STEP;
		spin_lock(&charger->charger_lock);
		charger->chgpretime = tmp * CHARGE_PRETIME_STEP + CHARGE_PRETIME_MIN;
		spin_unlock(&charger->charger_lock);
		val |= (tmp << 6);
		axp_update(charger->master, AXP_CHARGE_CONTROL2, val, 0xC0);
	}
	return count;
}

static ssize_t chgcsttimemin_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;

	axp_read(charger->master,AXP_CHARGE_CONTROL2, &val);
	spin_lock(&charger->charger_lock);
	charger->chgcsttime = (val & 0x03) *120 + 360;
	spin_unlock(&charger->charger_lock);
	return sprintf(buf, "%d\n",charger->chgcsttime);
}

static ssize_t chgcsttimemin_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	int var = 0;
	uint8_t tmp = 0,val = 0;

	var = simple_strtoul(buf, NULL, 10);
	if(var >= CHARGE_FASTTIME_MIN && var <= CHARGE_FASTTIME_MAX){
		tmp = (var - CHARGE_FASTTIME_MIN)/CHARGE_FASTTIME_STEP;
		spin_lock(&charger->charger_lock);
		charger->chgcsttime = tmp * CHARGE_FASTTIME_STEP + CHARGE_FASTTIME_MIN;
		spin_unlock(&charger->charger_lock);
		val |= tmp;
		axp_update(charger->master, AXP_CHARGE_CONTROL2, val, 0x03);
	}
	return count;
}

static ssize_t adcfreq_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	struct axp_charger *charger = dev_get_drvdata(dev);
	uint8_t val;

	axp_read(charger->master, AXP_ADC_CONTROL4, &val);
	spin_lock(&charger->charger_lock);
	switch ((val >> 4) & 0x03){
		case 0: charger->sample_time = 100;break;
		case 1: charger->sample_time = 200;break;
		case 2: charger->sample_time = 400;break;
		case 3: charger->sample_time = 800;break;
		default:break;
	}
	spin_unlock(&charger->charger_lock);
	return sprintf(buf, "%d\n",charger->sample_time);
}

static ssize_t adcfreq_store(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 int var = 0;
	 uint8_t val = 0;

	 var = simple_strtoul(buf, NULL, 10);
	 spin_lock(&charger->charger_lock);
	 switch ((var>>4) && 0x03){
		 case 0: val &= ~(3 << 4);charger->sample_time = 100;break;
		 case 1: val &= ~(3 << 4);val |= 1 << 4;charger->sample_time = 200;break;
		 case 2: val &= ~(3 << 4);val |= 2 << 4;charger->sample_time = 400;break;
		 case 3: val |= 3 << 4;charger->sample_time = 800;break;
		 default: break;
	 }
	 spin_unlock(&charger->charger_lock);
	 axp_update(charger->master, AXP_ADC_CONTROL4, val, 0x30);
	 return count;
}


static ssize_t vholden_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 uint8_t val;

	 axp_read(charger->master,AXP_CHARGE_VBUS, &val);
	 val = (val>>6) & 0x01;
	 return sprintf(buf, "%d\n",val);
}

static ssize_t vholden_store(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 int var;

	 var = simple_strtoul(buf, NULL, 10);
	 if(var)
	 axp_set_bits(charger->master, AXP_CHARGE_VBUS, 0x40);
	 else
	 axp_clr_bits(charger->master, AXP_CHARGE_VBUS, 0x40);
	 return count;
}

static ssize_t vhold_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 uint8_t val;
	 int vhold;

	 axp_read(charger->master,AXP_CHARGE_VBUS, &val);
	 vhold = ((val >> 3) & 0x07) * 100000 + 4000000;
	 return sprintf(buf, "%d\n",vhold);
}

static ssize_t vhold_store(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 int var = 0;
	 uint8_t val = 0,tmp = 0;

	 var = simple_strtoul(buf, NULL, 10);
	 if(var >= 4000000 && var <=4700000){
		 tmp = (var - 4000000)/100000;
		 val |= tmp << 3;
		 axp_update(charger->master, AXP_CHARGE_VBUS, val, 0x38);
	 }
	 return count;
}

static ssize_t iholden_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	 return sprintf(buf, "%d\n",1);
}

static ssize_t iholden_store(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	 return count;
}

static ssize_t ihold_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
	 struct axp_charger *charger = dev_get_drvdata(dev);
	 uint8_t val,tmp;
	 int ihold;

	 axp_read(charger->master,AXP_CHARGE_CONTROL3, &val);
	 tmp = (val>>4) & 0x0f;
	 switch(tmp){
		 case 0: ihold = 100000;break;
		 case 1: ihold = 500000;break;
		 case 2: ihold = 900000;break;
		 case 3: ihold = 1500000;break;
		 case 4: ihold = 2000000;break;
		 case 5: ihold = 2500000;break;
		 case 6: ihold = 3000000;break;
		 case 7: ihold = 3500000;break;
		 case 8: ihold = 4000000;break;
		 default: ihold = 0;break;
	 }
	 return sprintf(buf, "%d\n",ihold);
}

static ssize_t ihold_store(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	 return count;
}

static struct device_attribute axp_charger_attrs[] = {
	 AXP_CHG_ATTR(chgen),
	 AXP_CHG_ATTR(chgmicrovol),
	 AXP_CHG_ATTR(chgintmicrocur),
	 AXP_CHG_ATTR(chgendcur),
	 AXP_CHG_ATTR(chgpretimemin),
	 AXP_CHG_ATTR(chgcsttimemin),
	 AXP_CHG_ATTR(adcfreq),
	 AXP_CHG_ATTR(vholden),
	 AXP_CHG_ATTR(vhold),
	 AXP_CHG_ATTR(iholden),
	 AXP_CHG_ATTR(ihold),
};

int axp_charger_create_attrs(struct power_supply *psy)
{
	int j,ret;

	for (j = 0; j < ARRAY_SIZE(axp_charger_attrs); j++) {
		ret = device_create_file(psy->dev,&axp_charger_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}
	goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(psy->dev, &axp_charger_attrs[j]);
succeed:
  return ret;
}


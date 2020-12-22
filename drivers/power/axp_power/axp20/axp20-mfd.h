#include "../axp-rw.h"
#include "../axp-cfg.h"

static uint8_t axp_reg_addr = 0;

static void axp20_mfd_irq_work(struct work_struct *work)
{
	struct axp_dev *chip =
		container_of(work, struct axp_dev, irq_work);
	uint64_t irqs = 0;
	while (1) {
		if (chip->ops->read_irqs(chip, &irqs)){
			break;
		}
		
		irqs &= chip->irqs_enabled;
		if (irqs == 0){
			break;
		}
		
		if(irqs > 0xffffffff){
			blocking_notifier_call_chain(
					&chip->notifier_list, (irqs >>32), (void *)1);
		}
		else{
			blocking_notifier_call_chain(
					&chip->notifier_list, irqs, 0);
		}
	}
	enable_irq(chip->client->irq);
}

int ADC_Freq_Get_mfd(struct axp_dev *chip)
{
	uint8_t  temp=0;
	int rValue = 25;

	__axp_read(NULL, chip->client, POWER20_ADC_SPEED,&temp, false);
	temp &= 0xc0;
	switch(temp >> 6)
	{
		case 0:
			rValue = 25;
			break;
		case 1:
			rValue = 50;
			break;
		case 2:
			rValue = 100;
			break;
		case 3:
			rValue = 200;
			break;
		default:
			break;
	}
	return rValue;
}

void Buffer_Cou_Set_mfd(struct axp_dev *chip,uint16_t Cou_Counter)
{
	uint8_t	temp[3]	= {0,POWER20_DATA_BUFFER4,0};
	Cou_Counter	|= 0x8000;
	temp[0]	= ((Cou_Counter	& 0xff00) >> 8);
	temp[2]	= (Cou_Counter & 0x00ff);
	__axp_writes(NULL, chip->client,POWER20_DATA_BUFFER3,3,temp, false);
}

void Set_Rest_Cap_mfd(struct axp_dev *chip, int rest_cap)
{
	uint8_t	val;
	if(rest_cap	>= 0)
		val	= rest_cap & 0x7F;
	else
		val	= ABS(rest_cap)	| 0x80;
	__axp_write(NULL, chip->client, POWER20_DATA_BUFFER5, val, false);
}


int Get_Bat_Coulomb_Count_mfd(struct axp_dev *chip)
{
	uint8_t  temp[8];
	int64_t  rValue1,rValue2,rValue;
	int Cur_CoulombCounter_tmp,m;

	__axp_reads(NULL, chip->client, POWER20_BAT_CHGCOULOMB3,8,temp, false);
	rValue1 = ((temp[0] << 24) + (temp[1] << 16) + (temp[2] << 8) + temp[3]);
	rValue2 = ((temp[4] << 24) + (temp[5] << 16) + (temp[6] << 8) + temp[7]);
	rValue = (ABS(rValue1 - rValue2)) * 4369;
	m = ADC_Freq_Get_mfd(chip) * 480;
	do_div(rValue,m);
	if(rValue1 >= rValue2)
		Cur_CoulombCounter_tmp = (int)rValue;
	else
		Cur_CoulombCounter_tmp = (int)(0 - rValue);
	return Cur_CoulombCounter_tmp;				//unit mAh
}

void Cou_Count_Clear_mfd(struct	axp_dev *chip)
{
	uint8_t	temp = 0xff;
	__axp_read(NULL, chip->client, POWER20_COULOMB_CTL, &temp, false);
	temp |=	0x20;
	temp &=	0xbf;
	__axp_write(NULL, chip->client, POWER20_COULOMB_CTL, temp, false);
	temp |=	0x80;
	temp &=	0xbf;
	__axp_write(NULL, chip->client, POWER20_COULOMB_CTL, temp, false);
}

void axp20_correct_restcap(struct axp_dev *chip)
{
	int	Cou_Correction_Flag;
	uint8_t val[2]={0,0};
	uint8_t v[2];
	int bat_val;
	int Cur_CoulombCounter;
	int saved_cap,bat_cap;
	__axp_read(NULL, chip->client, POWER20_INTSTS2, val, false);
	if(val[0] &= 0x04){
		__axp_reads(NULL, chip->client, 0xbc, 2,v, false);
		bat_val = ((int)((v[0] << 4) | (v[1] & 0x0F))) * 1100 / 1000;
		if(bat_val > 4080){
			__axp_read(NULL, chip->client, POWER20_DATA_BUFFER1, val, false);
			Cou_Correction_Flag	= (val[0]	>> 5) &	0x1;
			if(Cou_Correction_Flag){
				printk("[AXP20-MFD] ----------charger finish need to be corrected-----------\n");
				printk("[AXP20-MFD] ----------correct the coulunb counter-----------\n");
				__axp_read(NULL, chip->client, POWER20_DATA_BUFFER6, val, false);
				Cur_CoulombCounter = Get_Bat_Coulomb_Count_mfd(chip);
				bat_cap	= ABS(Cur_CoulombCounter) / (100 - val[0]) * 100;
				Buffer_Cou_Set_mfd(chip,bat_cap);
				Cou_Correction_Flag	= 0;
				axp_clr_bits(chip->dev,POWER20_DATA_BUFFER1,0x20);
				saved_cap =	100;
				Set_Rest_Cap_mfd(chip,saved_cap);
				Cou_Count_Clear_mfd(chip);
			}
			else{
				printk("[AXP20-MFD] ----------charger finish need to be corrected-----------\n");
				saved_cap =	100;
				Set_Rest_Cap_mfd(chip,saved_cap);
				Cou_Count_Clear_mfd(chip);
			}
		}
	}
}

static int __devinit axp20_init_chip(struct axp_dev *chip)
{
	uint8_t chip_id;
	uint8_t v[19] = {0xd8,POWER20_INTEN2, 0xff,POWER20_INTEN3,0x03,
												POWER20_INTEN4, 0x01,POWER20_INTEN5, 0x00,
												POWER20_INTSTS1,0xff,POWER20_INTSTS2, 0xff,
												POWER20_INTSTS3,0xff,POWER20_INTSTS4, 0xff,
												POWER20_INTSTS5,0xff};
	int err;
	/*read chip id*/
	err =  __axp_read(NULL, chip->client, POWER20_IC_TYPE, &chip_id, false);
	if (err) {
	    printk("[AXP20-MFD] try to read chip id failed!\n");
		return err;
	}
	/*enable irqs and clear*/
	axp20_correct_restcap(chip);
	INIT_WORK(&chip->irq_work, axp20_mfd_irq_work);
	err = __axp_reads(NULL, chip->client, POWER20_INTSTS1, 5, v, false);
	if (err) {
	    printk("[AXP20-MFD] try to clear irqs failed!\n");
		return err;
	}
	err = __axp_writes(NULL, chip->client, POWER20_INTEN1, 19, v, false);
	if (err) {
	    printk("[AXP20-MFD] try to enable irqs failed!\n");
		return err;
	}

	dev_info(chip->dev, "AXP (CHIP ID: 0x%02x) detected\n", chip_id);
	chip->type = AXP20;

	/* mask and clear all IRQs */
	chip->irqs_enabled = 0xffffffff | (uint64_t)0xff << 32;
	chip->ops->disable_irqs(chip, chip->irqs_enabled);
    #ifdef CONFIG_ARCH_SUN7I
    writel(0x00,NMI_CTL_REG);
    writel(0x01,NMI_IRG_PENDING_REG);
    writel(0x00,NMI_INT_ENABLE_REG);
    #endif
	return 0;
}

static int axp20_disable_irqs(struct axp_dev *chip, uint64_t irqs)
{
	uint8_t v[9];
	int ret;

	chip->irqs_enabled &= ~irqs;

	v[0] = ((chip->irqs_enabled) & 0xff);
	v[1] = POWER20_INTEN2;
	v[2] = ((chip->irqs_enabled) >> 8) & 0xff;
	v[3] = POWER20_INTEN3;
	v[4] = ((chip->irqs_enabled) >> 16) & 0xff;
	v[5] = POWER20_INTEN4;
	v[6] = ((chip->irqs_enabled) >> 24) & 0xff;
	v[7] = POWER20_INTEN5;
	v[8] = ((chip->irqs_enabled) >> 32) & 0xff;
	ret =  __axp_writes(NULL, chip->client, POWER20_INTEN1, 9, v, false);
    #ifdef CONFIG_ARCH_SUN7I
    writel(0x0,NMI_INT_ENABLE_REG);
    #endif
	return ret;

}

static int axp20_enable_irqs(struct axp_dev *chip, uint64_t irqs)
{
	uint8_t v[9];
	int ret;

	chip->irqs_enabled |=  irqs;

	v[0] = ((chip->irqs_enabled) & 0xff);
	v[1] = POWER20_INTEN2;
	v[2] = ((chip->irqs_enabled) >> 8) & 0xff;
	v[3] = POWER20_INTEN3;
	v[4] = ((chip->irqs_enabled) >> 16) & 0xff;
	v[5] = POWER20_INTEN4;
	v[6] = ((chip->irqs_enabled) >> 24) & 0xff;
	v[7] = POWER20_INTEN5;
	v[8] = ((chip->irqs_enabled) >> 32) & 0xff;
	ret =  __axp_writes(NULL, chip->client, POWER20_INTEN1, 9, v, false);
    #ifdef CONFIG_ARCH_SUN7I
    writel(0x1,NMI_INT_ENABLE_REG);
    #endif

	return ret;
}

static int axp20_read_irqs(struct axp_dev *chip, uint64_t *irqs)
{
	uint8_t v[5] = {0, 0, 0, 0, 0};
	int ret;
	ret =  __axp_reads(NULL, chip->client, POWER20_INTSTS1, 5, v,false);
    #ifdef CONFIG_ARCH_SUN7I
    writel(0x01,NMI_IRG_PENDING_REG);
    #endif
	if (ret < 0)
		return ret;

	*irqs =(((uint64_t) v[4]) << 32) |(((uint64_t) v[3]) << 24) | (((uint64_t) v[2])<< 16) | (((uint64_t)v[1]) << 8) | ((uint64_t) v[0]);
    return 0;
}


static ssize_t axp20_offvol_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    uint8_t val = 0;
	axp_read(dev,POWER20_VOFF_SET,&val);
	return sprintf(buf,"%d\n",(val & 0x07) * 100 + 2600);
}

static ssize_t axp20_offvol_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 10);
	if (tmp < 2600)
		tmp = 2600;
	if (tmp > 3300)
		tmp = 3300;

	axp_read(dev,POWER20_VOFF_SET,&val);
	val &= 0xf8;
	val |= ((tmp - 2600) / 100);
	axp_write(dev,POWER20_VOFF_SET,val);
	return count;
}

static ssize_t axp20_noedelay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    uint8_t val;
	axp_read(dev,POWER20_OFF_CTL,&val);
	if( (val & 0x03) == 0)
		return sprintf(buf,"%d\n",128);
	else
		return sprintf(buf,"%d\n",(val & 0x03) * 1000);
}

static ssize_t axp20_noedelay_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 10);
	if (tmp < 1000)
		tmp = 128;
	if (tmp > 3000)
		tmp = 3000;
	axp_read(dev,POWER19_OFF_CTL,&val);
	val &= 0xfc;
	val |= ((tmp) / 1000);
	axp_write(dev,POWER20_OFF_CTL,val);
	return count;
}

static ssize_t axp20_pekopen_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    uint8_t val;
	int tmp = 0;
	axp_read(dev,POWER20_PEK_SET,&val);
	switch(val >> 6){
		case 0: tmp = 128;break;
		case 1: tmp = 3000;break;
		case 2: tmp = 1000;break;
		case 3: tmp = 2000;break;
		default:
			tmp = 0;break;
	}
	return sprintf(buf,"%d\n",tmp);
}

static ssize_t axp20_pekopen_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 10);
	axp_read(dev,POWER20_PEK_SET,&val);
	if (tmp < 1000)
		val &= 0x3f;
	else if(tmp < 2000){
		val &= 0x3f;
		val |= 0x80;
	}
	else if(tmp < 3000){
		val &= 0x3f;
		val |= 0xc0;
	}
	else {
		val &= 0x3f;
		val |= 0x40;
	}
	axp_write(dev,POWER20_PEK_SET,val);
	return count;
}

static ssize_t axp20_peklong_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    uint8_t val = 0;
	axp_read(dev,POWER20_PEK_SET,&val);
	return sprintf(buf,"%d\n",((val >> 4) & 0x03) * 500 + 1000);
}

static ssize_t axp20_peklong_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 10);
	if(tmp < 1000)
		tmp = 1000;
	if(tmp > 2500)
		tmp = 2500;
	axp_read(dev,POWER20_PEK_SET,&val);
	val &= 0xcf;
	val |= (((tmp - 1000) / 500) << 4);
	axp_write(dev,POWER20_PEK_SET,val);
	return count;
}

static ssize_t axp20_peken_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    uint8_t val;
	axp_read(dev,POWER20_PEK_SET,&val);
	return sprintf(buf,"%d\n",((val >> 3) & 0x01));
}

static ssize_t axp20_peken_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 10);
	if(tmp)
		tmp = 1;
	axp_read(dev,POWER20_PEK_SET,&val);
	val &= 0xf7;
	val |= (tmp << 3);
	axp_write(dev,POWER20_PEK_SET,val);
	return count;
}

static ssize_t axp20_pekdelay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    uint8_t val;
	axp_read(dev,POWER20_PEK_SET,&val);

	return sprintf(buf,"%d\n",((val >> 2) & 0x01)? 64:8);
}

static ssize_t axp20_pekdelay_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 10);
	if(tmp <= 8)
		tmp = 0;
	else
		tmp = 1;
	axp_read(dev,POWER20_PEK_SET,&val);
	val &= 0xfb;
	val |= tmp << 2;
	axp_write(dev,POWER20_PEK_SET,val);
	return count;
}

static ssize_t axp20_pekclose_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    uint8_t val;
	axp_read(dev,POWER20_PEK_SET,&val);
	return sprintf(buf,"%d\n",((val & 0x03) * 2000) + 4000);
}

static ssize_t axp20_pekclose_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 10);
	if(tmp < 4000)
		tmp = 4000;
	if(tmp > 10000)
		tmp =10000;
	tmp = (tmp - 4000) / 2000 ;
	axp_read(dev,POWER20_PEK_SET,&val);
	val &= 0xfc;
	val |= tmp ;
	axp_write(dev,POWER20_PEK_SET,val);
	return count;
}

static ssize_t axp20_ovtemclsen_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    uint8_t val;
	axp_read(dev,POWER20_HOTOVER_CTL,&val);
	return sprintf(buf,"%d\n",((val >> 2) & 0x01));
}

static ssize_t axp20_ovtemclsen_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 10);
	if(tmp)
		tmp = 1;
	axp_read(dev,POWER20_HOTOVER_CTL,&val);
	val &= 0xfb;
	val |= tmp << 2 ;
	axp_write(dev,POWER20_HOTOVER_CTL,val);
	return count;
}

static ssize_t axp20_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    uint8_t val;
	axp_read(dev,axp_reg_addr,&val);
	return sprintf(buf,"REG[%x]=%x\n",axp_reg_addr,val);
}

static ssize_t axp20_reg_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val;
	tmp = simple_strtoul(buf, NULL, 16);
	if( tmp < 256 )
		axp_reg_addr = tmp;
	else {
		val = tmp & 0x00FF;
		axp_reg_addr= (tmp >> 8) & 0x00FF;
		axp_write(dev,axp_reg_addr, val);
	}
	return count;
}

static ssize_t axp20_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
  uint8_t val[2];
	axp_reads(dev,axp_reg_addr,2,val);
	return sprintf(buf,"REG[0x%x]=0x%x,REG[0x%x]=0x%x\n",axp_reg_addr,val[0],axp_reg_addr+1,val[1]);
}

static ssize_t axp20_regs_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;
	uint8_t val[3];
	tmp = simple_strtoul(buf, NULL, 16);
	if( tmp < 256 )
		axp_reg_addr = tmp;
	else {
		axp_reg_addr= (tmp >> 16) & 0xFF;
		val[0] = (tmp >> 8) & 0xFF;
		val[1] = axp_reg_addr + 1;
		val[2] = tmp & 0xFF;
		axp_writes(dev,axp_reg_addr,3,val);
	}
	return count;
}

static struct device_attribute axp20_mfd_attrs[] = {
	AXP_MFD_ATTR(axp20_offvol),
	AXP_MFD_ATTR(axp20_noedelay),
	AXP_MFD_ATTR(axp20_pekopen),
	AXP_MFD_ATTR(axp20_peklong),
	AXP_MFD_ATTR(axp20_peken),
	AXP_MFD_ATTR(axp20_pekdelay),
	AXP_MFD_ATTR(axp20_pekclose),
	AXP_MFD_ATTR(axp20_ovtemclsen),
	AXP_MFD_ATTR(axp20_reg),
	AXP_MFD_ATTR(axp20_regs),
};


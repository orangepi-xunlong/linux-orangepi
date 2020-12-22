#include "axp-rw.h"

static uint8_t axp_reg_addr = 0;

static void axp15_mfd_irq_work(struct work_struct *work)
{
#ifdef CONFIG_AXP15_IRQ
    struct axp_dev *chip = container_of(work, struct axp_dev, irq_work);
    uint64_t irqs = 0;

    while (1) {
        if (chip->ops->read_irqs(chip, &irqs)) {
            printk("[AXP15-MFD] read irq failed!\n");
            break;
        }

        irqs &=chip->irqs_enabled;
        if (irqs == 0) break;

        blocking_notifier_call_chain(&chip->notifier_list, (uint32_t)irqs, (void *)0);
    }

#ifdef  CONFIG_AXP_TWI_USED
    enable_irq(chip->client->irq);
#else
    arisc_enable_nmi_irq();
#endif

#else
    return;
#endif /* CONFIG_AXP15_IRQ */
}

static int axp15_disable_irqs(struct axp_dev *chip, uint64_t irqs)
{
	uint8_t v[3];
	int ret;
	unsigned char devaddr = RSB_RTSADDR_AXP806;

	chip->irqs_enabled &= ~irqs;

	v[0] = ((chip->irqs_enabled) & 0xff);
	v[1] = AXP15_INTEN2;
	v[2] = ((chip->irqs_enabled) >> 8) & 0xff;

	ret =  __axp_writes(&devaddr, chip->client, AXP15_INTEN1, 3, v, false);

	return ret;
}

static int axp15_enable_irqs(struct axp_dev *chip, uint64_t irqs)
{
	uint8_t v[3];
	int ret;
	unsigned char devaddr = RSB_RTSADDR_AXP806;

	chip->irqs_enabled |=  irqs;

	v[0] = ((chip->irqs_enabled) & 0xff);
	v[1] = AXP15_INTEN2;
	v[2] = ((chip->irqs_enabled) >> 8) & 0xff;

	ret =  __axp_writes(&devaddr, chip->client, AXP15_INTEN1, 3, v, false);

	return ret;
}

static int axp15_read_irqs(struct axp_dev *chip, uint64_t *irqs)
{
	uint8_t v[2] = {0, 0};
	int ret;
	unsigned char devaddr = RSB_RTSADDR_AXP806;
	ret =  __axp_reads(&devaddr, chip->client, AXP15_INTSTS1, 2, v, false);
	if (ret < 0)
		return ret;

	*irqs =((((uint64_t)v[1]) << 8) | ((uint64_t) v[0]));
	return 0;
}

static int  axp15_init_chip(struct axp_dev *chip)
{
	uint8_t chip_id;
	int err;
	unsigned char devaddr = RSB_RTSADDR_AXP806;

	INIT_WORK(&chip->irq_work, axp15_mfd_irq_work);

	/*read chip id*/	//???which int should enable must check with SD4
	err =  __axp_read(&devaddr, chip->client, AXP15_IC_TYPE, &chip_id, false);
	if (err) {
		printk("[AXP15-MFD] try to read chip id failed!\n");
		return err;
	}

	if(((chip_id & 0xc0) == 0x40) && ((chip_id & 0x0f) == 0x00))
		chip->type = AXP15;
	else
		chip->type = 0;

    /* clear all IRQs */
    __axp_write(&devaddr, chip->client, AXP15_INTSTS1, 0xff, false);
    __axp_write(&devaddr, chip->client, AXP15_INTSTS2, 0xff, false);

	/* mask and clear all IRQs */
	chip->irqs_enabled = 0xffff;
	chip->ops->disable_irqs(chip, chip->irqs_enabled);

	return 0;
}

static ssize_t axp15_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	uint8_t val;

	axp_read(dev,axp_reg_addr,&val);
	return sprintf(buf,"REG[%x]=%x\n",axp_reg_addr,val);
}

static ssize_t axp15_reg_store(struct device *dev,
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

static ssize_t axp15_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	uint8_t val[20];
	int count = 0, i = 0;

	axp_reads(dev,axp_reg_addr,20,val);
	for (i=0;i<20;i++) {
		count += sprintf(buf+count,"REG[0x%x]=0x%x\n",axp_reg_addr+i,val[i]);
	}
	return count;
}

static ssize_t axp15_regs_store(struct device *dev,
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

static struct device_attribute axp15_mfd_attrs[] = {
	AXP_MFD_ATTR(axp15_reg),
	AXP_MFD_ATTR(axp15_regs),
};


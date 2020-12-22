#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/spinlock.h>
#include <mach/irqs.h>
#include <linux/module.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <linux/mfd/axp-mfd.h>
#include "axp-cfg.h"
#include "axp-rw.h"

int axp_register_notifier(struct device *dev, struct notifier_block *nb,
				uint64_t irqs)
{
	struct axp_dev *chip = dev_get_drvdata(dev);

	if(NULL != nb) {
		blocking_notifier_chain_register(&chip->notifier_list, nb);
	}
	chip->ops->enable_irqs(chip, irqs);
#ifdef CONFIG_SUNXI_ARISC
	arisc_enable_nmi_irq();
#else
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(axp_register_notifier);

int axp_unregister_notifier(struct device *dev, struct notifier_block *nb,
				uint64_t irqs)
{
	struct axp_dev *chip = dev_get_drvdata(dev);
#ifdef CONFIG_SUNXI_ARISC
	arisc_disable_nmi_irq();
#else
#endif
	chip->ops->disable_irqs(chip, irqs);
	if(NULL != nb) {
	    return blocking_notifier_chain_unregister(&chip->notifier_list, nb);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(axp_unregister_notifier);

int axp_write(struct device *dev, int reg, uint8_t val)
{
	struct axp_dev *chip;
	int ret = 0;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	chip = dev_get_drvdata(dev);

	switch (chip->type) {
	case AXP22:
	case AXP81X:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	mutex_lock(&chip->lock);
	ret = __axp_write(&devaddr, to_i2c_client(dev), reg, val, false);
	mutex_unlock(&chip->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_write);

int axp_writes(struct device *dev, int reg, int len, uint8_t *val)
{
	struct axp_dev *chip;
	int ret = 0;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	chip = dev_get_drvdata(dev);

	switch (chip->type) {
	case AXP22:
	case AXP81X:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	mutex_lock(&chip->lock);
	ret = __axp_writes(&devaddr, to_i2c_client(dev), reg, len, val, false);
	mutex_unlock(&chip->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(axp_writes);

int axp_read(struct device *dev, int reg, uint8_t *val)
{
	struct axp_dev *chip;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	chip = dev_get_drvdata(dev);

	switch (chip->type) {
	case AXP22:
	case AXP81X:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	return __axp_read(&devaddr, to_i2c_client(dev), reg, val, false);
}
EXPORT_SYMBOL_GPL(axp_read);

int axp_reads(struct device *dev, int reg, int len, uint8_t *val)
{
	struct axp_dev *chip;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	chip = dev_get_drvdata(dev);

	switch (chip->type) {
	case AXP22:
	case AXP81X:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	return __axp_reads(&devaddr, to_i2c_client(dev), reg, len, val, false);
}
EXPORT_SYMBOL_GPL(axp_reads);

int axp_set_bits(struct device *dev, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;
	struct axp_dev *chip;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	chip = dev_get_drvdata(dev);

	switch (chip->type) {
	case AXP22:
	case AXP81X:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	mutex_lock(&chip->lock);
	ret = __axp_read(&devaddr, chip->client, reg, &reg_val, false);
	if (ret)
		goto out;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = __axp_write(&devaddr, chip->client, reg, reg_val, false);
	}
out:
	mutex_unlock(&chip->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_set_bits);

int axp_clr_bits(struct device *dev, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;
	struct axp_dev *chip;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	chip = dev_get_drvdata(dev);

	switch (chip->type) {
	case AXP22:
	case AXP81X:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	mutex_lock(&chip->lock);

	ret = __axp_read(&devaddr, chip->client, reg, &reg_val, false);
	if (ret)
		goto out;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = __axp_write(&devaddr, chip->client, reg, reg_val, false);
	}
out:
	mutex_unlock(&chip->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_clr_bits);

int axp_update(struct device *dev, int reg, uint8_t val, uint8_t mask)
{
	struct axp_dev *chip = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	switch (chip->type) {
	case AXP22:
	case AXP81X:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	mutex_lock(&chip->lock);

	ret = __axp_read(&devaddr, chip->client, reg, &reg_val, false);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = __axp_write(&devaddr, chip->client, reg, reg_val, false);
	}
out:
	mutex_unlock(&chip->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_update);


int axp_set_bits_sync(struct device *dev, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;
	unsigned long irqflags;
	struct axp_dev *chip;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	chip = dev_get_drvdata(dev);

	switch (chip->type) {
	case AXP22:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	spin_lock_irqsave(&chip->spinlock, irqflags);
	ret = __axp_read(&devaddr, chip->client, reg, &reg_val, true);
	if (ret)
		goto out;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = __axp_write(&devaddr, chip->client, reg, reg_val, true);
	}
out:
	spin_unlock_irqrestore(&chip->spinlock, irqflags);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_set_bits_sync);

int axp_clr_bits_sync(struct device *dev, int reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;
	unsigned long irqflags;
	struct axp_dev *chip;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	chip = dev_get_drvdata(dev);

	switch (chip->type) {
	case AXP22:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	spin_lock_irqsave(&chip->spinlock, irqflags);
	ret = __axp_read(&devaddr, chip->client, reg, &reg_val, true);
	if (ret)
		goto out;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = __axp_write(&devaddr, chip->client, reg, reg_val, true);
	}
out:
	spin_unlock_irqrestore(&chip->spinlock, irqflags);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_clr_bits_sync);

int axp_update_sync(struct device *dev, int reg, uint8_t val, uint8_t mask)
{
	struct axp_dev *chip = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;
	unsigned long irqflags;
	unsigned char devaddr = RSB_RTSADDR_AXP809;

	switch (chip->type) {
	case AXP22:
	case AXP81X:
		devaddr = RSB_RTSADDR_AXP809;
		break;
	case AXP15:
		devaddr = RSB_RTSADDR_AXP806;
		break;
	default:
		break;
	}

	spin_lock_irqsave(&chip->spinlock, irqflags);
	ret = __axp_read(&devaddr, chip->client, reg, &reg_val, true);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = __axp_write(&devaddr, chip->client, reg, reg_val, true);
	}
out:
	spin_unlock_irqrestore(&chip->spinlock, irqflags);
	return ret;
}
EXPORT_SYMBOL_GPL(axp_update_sync);


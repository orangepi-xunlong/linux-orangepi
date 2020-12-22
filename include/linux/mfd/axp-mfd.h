/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef _AXP_MFD_CORE_H
#define _AXP_MFD_CORE_H
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/module.h>

#include <linux/mfd/axp-mfd-81x.h>
#include <linux/mfd/axp-mfd-22.h>
#include <linux/mfd/axp-mfd-19.h>
#include <linux/mfd/axp-mfd-15.h>
#include <linux/mfd/axp-mfd-20.h>

#define AXP_MFD_ATTR(_name)				\
{							\
	.attr = { .name = #_name,.mode = 0644 },	\
	.show =  _name##_show,				\
	.store = _name##_store,				\
}

/* AXP battery charger data */
struct power_supply_info;

struct axp_supply_init_data {
	/* battery parameters */
	struct power_supply_info *battery_info;

	/* current and voltage to use for battery charging */
	unsigned int chgcur;
	unsigned int chgvol;
	unsigned int chgend;
	/*charger control*/
	bool chgen;
	bool limit_on;
	/*charger time */
	int chgpretime;
	int chgcsttime;

	/*adc sample time */
	unsigned int sample_time;

	/* platform callbacks for battery low and critical IRQs */
	void (*battery_low)(void);
	void (*battery_critical)(void);
};

struct axp_funcdev_info {
	int		id;
	const char	*name;
	void	*platform_data;
};

struct axp_platform_data {
	int num_regl_devs;
	int num_sply_devs;
	int num_gpio_devs;
	int gpio_base;
	struct axp_funcdev_info *regl_devs;
	struct axp_funcdev_info *sply_devs;
	struct axp_funcdev_info *gpio_devs;

};

struct axp_dev {
	struct axp_platform_data *pdata;
	struct axp_mfd_chip_ops	*ops;
	struct device		*dev;

	int attrs_number;
	struct device_attribute *attrs;

	struct i2c_client *client;

	int			type;
	uint64_t		irqs_enabled;

	spinlock_t		spinlock;
	struct mutex		lock;
	struct work_struct	irq_work;

	struct blocking_notifier_head notifier_list;
	/* lists we belong to */
	struct list_head list; /* list of all axps */

};

struct axp_mfd_chip_ops {
	int	(*init_chip)(struct axp_dev *);
	int	(*enable_irqs)(struct axp_dev *, uint64_t irqs);
	int	(*disable_irqs)(struct axp_dev *, uint64_t irqs);
	int	(*read_irqs)(struct axp_dev *, uint64_t *irqs);
};

extern int axp_register_notifier(struct device *dev,
		struct notifier_block *nb, uint64_t irqs);
extern int axp_unregister_notifier(struct device *dev,
		struct notifier_block *nb, uint64_t irqs);


typedef enum AW_CHARGE_TYPE
{
	CHARGE_AC,
	CHARGE_USB_20,
	CHARGE_USB_30,
	CHARGE_MAX
} aw_charge_type;

/* NOTE: the functions below are not intended for use outside
 * of the AXP sub-device drivers
 */
extern int axp_write(struct device *dev, int reg, uint8_t val);
extern int axp_writes(struct device *dev, int reg, int len, uint8_t *val);
extern int axp_read(struct device *dev, int reg, uint8_t *val);
extern int axp_reads(struct device *dev, int reg, int len, uint8_t *val);
extern int axp_update(struct device *dev, int reg, uint8_t val, uint8_t mask);
extern int axp_set_bits(struct device *dev, int reg, uint8_t bit_mask);
extern int axp_clr_bits(struct device *dev, int reg, uint8_t bit_mask);
extern int axp_update_sync(struct device *dev, int reg, uint8_t val, uint8_t mask);
extern int axp_set_bits_sync(struct device *dev, int reg, uint8_t bit_mask);
extern int axp_clr_bits_sync(struct device *dev, int reg, uint8_t bit_mask);
extern struct axp_dev *axp_dev_lookup(int type);
int axp_register_mfd(struct axp_dev *dev);
void axp_unregister_mfd(struct axp_dev *dev);
extern void axp_reg_debug(int reg, int len, uint8_t *val);

/* NOTE: the functions below are used for outside
 * of the AXP drivers
 */
extern int axp_usbcur(aw_charge_type type);
extern int axp_usbvol(aw_charge_type type);
extern int axp_usb_det(void);
extern int axp_powerkey_get(void);
extern void axp_powerkey_set(int value);
extern unsigned long axp_read_power_sply(void);
extern int axp_read_bat_cap(void);
extern int axp_read_ac_chg(void);
#endif

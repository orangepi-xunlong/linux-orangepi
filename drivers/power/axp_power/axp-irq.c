/*
 * Battery charger driver for AW-POWERS
 *
 * Copyright (C) 2014 ALLWINNERTECH.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/input.h>
#include <linux/wakelock.h>
#include "axp-cfg.h"
#ifdef CONFIG_SUNXI_ARISC
#include <linux/arisc/arisc.h>
#endif
#ifdef CONFIG_AW_AXP81X
#include "axp81x/axp81x-sply.h"
#include "axp81x/axp81x-common.h"
static const struct axp_config_info *axp_config = &axp81x_config;
static const uint64_t AXP_NOTIFIER_ON = AXP81X_NOTIFIER_ON;
static int Total_Cap = 0;
static int Bat_Cap_Buffer[AXP_VOL_MAX];
#elif defined CONFIG_AW_AXP19
#include "axp19/axp19-sply.h"
#include "axp19/axp19-common.h"
const struct axp_config_info *axp_config = &axp19_config;
static const uint64_t AXP_NOTIFIER_ON = AXP19_NOTIFIER_ON;
#endif
#ifdef CONFIG_HAS_WAKELOCK
static struct wake_lock axp_wakeup_lock;
#endif
static  struct input_dev * powerkeydev;
static int axp_power_key = 0;

static DEFINE_SPINLOCK(axp_powerkey_lock);

void axp_powerkey_set(int value)
{
	spin_lock(&axp_powerkey_lock);
	axp_power_key = value;
	spin_unlock(&axp_powerkey_lock);
}
EXPORT_SYMBOL_GPL(axp_powerkey_set);

int axp_powerkey_get(void)
{
	int value;

	spin_lock(&axp_powerkey_lock);
	value = axp_power_key;
	spin_unlock(&axp_powerkey_lock);

	return value;
}
EXPORT_SYMBOL_GPL(axp_powerkey_get);


static void axp_change(struct axp_charger *charger)
{
	DBG_PSY_MSG(DEBUG_INT, "battery state change\n");
	axp_charger_update_state(charger);
	axp_charger_update(charger, axp_config);
	power_supply_changed(&charger->batt);
}

static void axp_presslong(struct axp_charger *charger)
{
	DBG_PSY_MSG(DEBUG_INT, "press long\n");
	input_report_key(powerkeydev, KEY_POWER, 1);
	input_sync(powerkeydev);
	ssleep(2);
	DBG_PSY_MSG(DEBUG_INT, "press long up\n");
	input_report_key(powerkeydev, KEY_POWER, 0);
	input_sync(powerkeydev);
}

static void axp_pressshort(struct axp_charger *charger)
{
	DBG_PSY_MSG(DEBUG_INT, "press short\n");
	input_report_key(powerkeydev, KEY_POWER, 1);
	input_sync(powerkeydev);
	msleep(100);
	input_report_key(powerkeydev, KEY_POWER, 0);
	input_sync(powerkeydev);
}

#ifdef CONFIG_AW_AXP81X
static void axp_keyup(struct axp_charger *charger)
{
	DBG_PSY_MSG(DEBUG_INT, "power key up\n");
	input_report_key(powerkeydev, KEY_POWER, 0);
	input_sync(powerkeydev);
}

static void axp_keydown(struct axp_charger *charger)
{
	DBG_PSY_MSG(DEBUG_INT, "power key down\n");
	input_report_key(powerkeydev, KEY_POWER, 1);
	input_sync(powerkeydev);
}

static void axp_capchange(struct axp_charger *charger)
{
	uint8_t val;
	int k;

	DBG_PSY_MSG(DEBUG_INT, "battery change\n");
	ssleep(2);
	axp_charger_update_state(charger);
	axp_charger_update(charger, axp_config);
	axp_read(charger->master, AXP_CAP,&val);
	charger->rest_vol = (int) (val & 0x7F);
	if((charger->bat_det == 0) || (charger->rest_vol == 127)){
		charger->rest_vol = 100;
	}

	DBG_PSY_MSG(DEBUG_INT, "rest_vol = %d\n",charger->rest_vol);
	memset(Bat_Cap_Buffer, 0, sizeof(Bat_Cap_Buffer));
	for(k = 0;k < AXP81X_VOL_MAX; k++){
		Bat_Cap_Buffer[k] = charger->rest_vol;
	}
	Total_Cap = charger->rest_vol * AXP_VOL_MAX;
	power_supply_changed(&charger->batt);
}

static int axp_battery_event(struct notifier_block *nb, unsigned long event,
	 void *data)
{
	struct axp_charger *charger =
	container_of(nb, struct axp_charger, nb);
	uint8_t w[11];
	int value;

	DBG_PSY_MSG(DEBUG_INT, "axp_battery_event enter...\n");
	if((bool)data==0){
		DBG_PSY_MSG(DEBUG_INT, "low 32bit status...\n");
		if(event & (AXP_IRQ_BATIN|AXP_IRQ_BATRE))
			axp_capchange(charger);
		if(event & (AXP_IRQ_BATINWORK|AXP_IRQ_BATOVWORK|AXP_IRQ_QBATINCHG|AXP_IRQ_BATINCHG
			|AXP_IRQ_QBATOVCHG|AXP_IRQ_BATOVCHG))
			axp_change(charger);
		if(event & (AXP_IRQ_ACOV|AXP_IRQ_USBOV|AXP_IRQ_CHAOV|AXP_IRQ_CHAST))
			axp_change(charger);
		if(event & (AXP_IRQ_ACIN|AXP_IRQ_USBIN)) {
#ifdef CONFIG_HAS_WAKELOCK
			wake_lock_timeout(&axp_wakeup_lock, msecs_to_jiffies(2000));
#endif
			axp_usbac_checkst(charger);
			axp_change(charger);
			axp_usbac_in(charger);
		}
		if(event & (AXP_IRQ_ACRE|AXP_IRQ_USBRE)) {
#ifdef CONFIG_HAS_WAKELOCK
			wake_lock_timeout(&axp_wakeup_lock, msecs_to_jiffies(2000));
#endif
			axp_usbac_checkst(charger);
			axp_change(charger);
			axp_usbac_out(charger);
		}
		if(event & AXP_IRQ_PEK_LONGTIME)
			axp_presslong(charger);
		if(event & AXP_IRQ_PEK_SHORTTIME)
			axp_pressshort(charger);
		w[0] = (uint8_t) ((event) & 0xFF);
		w[1] = AXP_INTSTS2;
		w[2] = (uint8_t) ((event >> 8) & 0xFF);
		w[3] = AXP_INTSTS3;
		w[4] = (uint8_t) ((event >> 16) & 0xFF);
		w[5] = AXP_INTSTS4;
		w[6] = (uint8_t) ((event >> 24) & 0xFF);
		w[7] = AXP_INTSTS5;
		w[8] = 0;
		w[9] = AXP_INTSTS6;
		w[10] = 0;
	} else {
		value = axp_powerkey_get();
		if (0 != value) {
			axp_powerkey_set(0);
		} else {
			if((event) & (AXP_IRQ_PEK_NEDGE>>32))
				axp_keydown(charger);
			if((event) & (AXP_IRQ_PEK_PEDGE>>32))
				axp_keyup(charger);
		}

		DBG_PSY_MSG(DEBUG_INT, "high 32bit status...\n");
		w[0] = 0;
		w[1] = AXP_INTSTS2;
		w[2] = 0;
		w[3] = AXP_INTSTS3;
		w[4] = 0;
		w[5] = AXP_INTSTS4;
		w[6] = 0;
		w[7] = AXP_INTSTS5;
		w[8] = (uint8_t) ((event) & 0xFF);
		w[9] = AXP_INTSTS6;
		w[10] = (uint8_t) ((event>> 8) & 0xFF);
	}
	DBG_PSY_MSG(DEBUG_INT, "event = 0x%x\n",(int) event);
	axp_writes(charger->master,AXP_INTSTS1,11,w);
	return 0;
}

int axp_disable_irq(struct axp_charger *charger)
{
	struct axp_dev *chip = dev_get_drvdata(charger->master);
	uint8_t irq_w[11];
	s32 ret = 0;

#ifdef CONFIG_HAS_WAKELOCK
	if (wake_lock_active(&axp_wakeup_lock)) {
		printk(KERN_ERR "AXP:axp_wakeup_lock wakeup system\n");
		return -EPERM;
	}
#endif

	/*clear all irqs events*/
	irq_w[0] = 0xff;
	irq_w[1] = AXP_INTSTS2;
	irq_w[2] = 0xff;
	irq_w[3] = AXP_INTSTS3;
	irq_w[4] = 0xff;
	irq_w[5] = AXP_INTSTS4;
	irq_w[6] = 0xff;
	irq_w[7] = AXP_INTSTS5;
	irq_w[8] = 0xff;
	irq_w[9] = AXP_INTSTS6;
	irq_w[10] = 0xff;
	axp_writes(charger->master,  AXP_INTSTS1,  11,  irq_w);

#ifdef CONFIG_SUNXI_ARISC
	arisc_disable_nmi_irq();
#else
#endif

	/* close all irqs*/
	ret = chip->ops->disable_irqs(chip, AXP_NOTIFIER_ON);
	if(0 != ret)
		printk("%s: axp irq disable failed.\n", __func__);

	return 0;
}

#elif defined CONFIG_AW_AXP19
static int axp_battery_event(struct notifier_block *nb, unsigned long event,
	 void *data)
{
	struct axp_charger *charger =
		container_of(nb, struct axp_charger, nb);
	uint8_t w[7];

	DBG_PSY_MSG(DEBUG_INT, "axp_battery_event enter...\n");

	if(event & (AXP_IRQ_BATIN|AXP_IRQ_ACIN|AXP_IRQ_USBIN)) {
		axp_set_bits(charger->master,AXP_CHARGE_CONTROL1,0x80);
#ifdef CONFIG_HAS_WAKELOCK
		wake_lock_timeout(&axp_wakeup_lock, msecs_to_jiffies(2000));
#endif
		axp_change(charger);
	}
	if(event & (AXP_IRQ_BATRE|AXP_IRQ_ACRE|AXP_IRQ_USBRE)) {
		axp_clr_bits(charger->master,AXP_CHARGE_CONTROL1,0x80);
#ifdef CONFIG_HAS_WAKELOCK
		wake_lock_timeout(&axp_wakeup_lock, msecs_to_jiffies(2000));
#endif
		axp_change(charger);
	}
	if(event & (AXP_IRQ_ACOV|AXP_IRQ_USBOV
		|AXP_IRQ_TEMOV|AXP_IRQ_TEMLO)) {
		axp_clr_bits(charger->master,AXP_CHARGE_CONTROL1,0x80);
		axp_change(charger);
	}
	if(event & AXP_IRQ_PEKLO)
		axp_presslong(charger);
	if(event & AXP_IRQ_PEKSH)
		axp_pressshort(charger);

	w[0] = (uint8_t) ((event) & 0xFF);
	w[1] = AXP_INTSTS2;
	w[2] = (uint8_t) ((event >> 8) & 0xFF);
	w[3] = AXP_INTSTS3;
	w[4] = (uint8_t) ((event >> 16) & 0xFF);
	w[5] = AXP_INTSTS4;
	w[6] = (uint8_t) ((event >> 24) & 0xFF);
	DBG_PSY_MSG(DEBUG_INT, "event = 0x%x\n",(int) event);
	axp_writes(charger->master,AXP_INTSTS1,7,w);

	return 0;
}

int axp_disable_irq(struct axp_charger *charger)
{
	uint8_t irq_w[7];

#ifdef CONFIG_HAS_WAKELOCK
	if (wake_lock_active(&axp_wakeup_lock)) {
		printk(KERN_ERR "AXP:axp_wakeup_lock wakeup system\n");
		return -EPERM;
	}
#endif

	/*clear all irqs events*/
	irq_w[0] = 0xff;
	irq_w[1] = AXP_INTSTS2;
	irq_w[2] = 0xff;
	irq_w[3] = AXP_INTSTS3;
	irq_w[4] = 0xff;
	irq_w[5] = AXP_INTSTS4;
	irq_w[6] = 0xff;

	axp_writes(charger->master,  AXP_INTSTS1,  7,  irq_w);
	/* close all irqs*/
	axp_unregister_notifier(charger->master, &charger->nb, AXP_NOTIFIER_ON);

	return 0;
}
#endif

int axp_enable_irq(struct axp_charger *charger)
{
	struct axp_dev *chip = dev_get_drvdata(charger->master);
	s32 ret = 0;

	ret = chip->ops->enable_irqs(chip, AXP_NOTIFIER_ON);
	if(0 != ret)
		printk("%s: axp irq enable failed.\n", __func__);

#ifdef CONFIG_SUNXI_ARISC
	arisc_enable_nmi_irq();
#else
#endif

	return ret;
}

int axp_irq_init(struct axp_charger *charger, struct platform_device *pdev)
{
	int ret = 0;

#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&axp_wakeup_lock, WAKE_LOCK_SUSPEND, "axp_wakeup_lock");
#endif

	powerkeydev = input_allocate_device();
	if (!powerkeydev) {
		kfree(powerkeydev);
		return -ENODEV;
	}
	powerkeydev->name = pdev->name;
	powerkeydev->phys = "m1kbd/input2";
	powerkeydev->id.bustype = BUS_HOST;
	powerkeydev->id.vendor = 0x0001;
	powerkeydev->id.product = 0x0001;
	powerkeydev->id.version = 0x0100;
	powerkeydev->open = NULL;
	powerkeydev->close = NULL;
	powerkeydev->dev.parent = &pdev->dev;
	set_bit(EV_KEY, powerkeydev->evbit);
	set_bit(EV_REL, powerkeydev->evbit);
	set_bit(KEY_POWER, powerkeydev->keybit);
	ret = input_register_device(powerkeydev);
	if(ret)
		printk("Unable to Register the power key\n");

	charger->nb.notifier_call = axp_battery_event;
	ret = axp_register_notifier(charger->master, &charger->nb, AXP_NOTIFIER_ON);
	if (ret)
		goto err_notifier;

	return ret;

err_notifier:
	axp_unregister_notifier(charger->master, &charger->nb, AXP_NOTIFIER_ON);
	return ret;
}

void axp_irq_exit(struct axp_charger *charger)
{
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_destroy(&axp_wakeup_lock);
#endif
	axp_unregister_notifier(charger->master, &charger->nb, AXP_NOTIFIER_ON);
	input_unregister_device(powerkeydev);
	kfree(powerkeydev);
	return;
}


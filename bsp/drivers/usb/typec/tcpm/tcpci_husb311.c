// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2023 Allwinner Technology Co., Ltd.
 *
 * Hynetek Husb311 Type-C Chip Driver
 */

#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm_wakeirq.h>
#include <linux/version.h>
#include <linux/ktime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/usb/tcpm.h>
#include <linux/platform_device.h>  // 提供 platform_get_irq()
#include <linux/usb/role.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 130)
#include <linux/usb/tcpci.h>
#else
#include "tcpci.h"
#endif

#include <linux/power_supply.h>

#define HUSB311_VENDOR_ID			0x2E99
#define HUSB311_PRODUCT_ID			0x0311
#define HUSB311_REG_BMC_CTRL			0x90
#define HUSB311_REG_VCONN_CLIMITEN		0x95
#define HUSB311_REG_IDLE_CTRL			0x9B
#define HUSB311_REG_I2CRST_CTRL			0x9E
#define HUSB311_REG_SWRESET			0xA0
#define HUSB311_REG_TTCPC_FILTER		0xA1
#define HUSB311_REG_DRP_TOGGLE_CYCLE		0xA2
#define HUSB311_REG_DRP_DUTY_CTRL		0xA3

#define TCPC_CMD_RESETTRANSMITBUFFER		0xDD
#define TCPC_ROLE_CTRL_SET(drp, rp, cc1, cc2) \
	((drp) << 6 | (rp) << 4 | (cc2) << 2 | (cc1))
#define HUSB311_REG_ENEXTMSG			BIT(4)
#define HUSB311_REG_IDLE_SET(ck300, ship_dis, auto_idle, tout) \
	((ck300 << 7) | (ship_dis << 5) | (auto_idle << 3) | (tout & 0x07) | HUSB311_REG_ENEXTMSG)
#define HUSB311_TCPM_DEBOUNCE_MS		500 /* ms */

struct tcpci {
	struct device *dev;
	struct tcpm_port *port;
	struct regmap *regmap;
	bool controls_vbus;
	struct tcpc_dev tcpc;
	struct tcpci_data *data;
};

struct husb311_chip {
	struct tcpci_data data;
	struct tcpci *tcpci;
	struct device *dev;
	struct regulator *vbus;
	struct power_supply *usb_psy;
	struct usb_role_switch *role_sw;
	unsigned long debounce_jiffies;
	struct delayed_work wq_detcable;

	bool vbus_on;
	bool vbus_debounce_quirk;
	bool port_reset_quirk;
	bool battery_exist;
	u32 vbus_tryon_debounce;
	u32 vbus_check_debounce;
	int gpio_int_n;
	int gpio_int_n_irq;
	struct gpio_desc *irq_gpio_desc;

	/* *
	 * FIXME:
	 * Some PD Adapter exists in leakage path of electricity,
	 * such as UGREEN's CD275.
	 */
	ktime_t try_on; /* Vbus Try On Time */
	ktime_t closed; /* Vbus Off Time */
	bool can_skip;
	bool need_check;
};

static const char * const typec_cc_status_name[] = {
	[TYPEC_CC_OPEN]		= "Open",
	[TYPEC_CC_RA]		= "Ra",
	[TYPEC_CC_RD]		= "Rd",
	[TYPEC_CC_RP_DEF]	= "Rp-def",
	[TYPEC_CC_RP_1_5]	= "Rp-1.5",
	[TYPEC_CC_RP_3_0]	= "Rp-3.0",
};

static const char * const usb_role_name[] = {
	[USB_ROLE_NONE]		= "NONE",
	[USB_ROLE_HOST]		= "HOST",
	[USB_ROLE_DEVICE]	= "DEVICE",
};

#define tcpci_cc_is_sink(cc) \
	((cc) == TYPEC_CC_RP_DEF || (cc) == TYPEC_CC_RP_1_5 || \
	 (cc) == TYPEC_CC_RP_3_0)

/* As long as cc is pulled up, we can consider it as sink. */
#define tcpci_port_is_sink(cc1, cc2) \
	(tcpci_cc_is_sink(cc1) || tcpci_cc_is_sink(cc2))

#define tcpci_cc_is_source(cc) ((cc) == TYPEC_CC_RD)

#define tcpci_port_is_source(cc1, cc2) \
	((tcpci_cc_is_source(cc1) && !tcpci_cc_is_source(cc2)) || \
	 (tcpci_cc_is_source(cc2) && !tcpci_cc_is_source(cc1)))

static int husb311_read8(struct husb311_chip *chip, unsigned int reg, u8 *val)
{
	return regmap_raw_read(chip->data.regmap, reg, val, sizeof(u8));
}

static int husb311_write8(struct husb311_chip *chip, unsigned int reg, u8 val)
{
	return regmap_raw_write(chip->data.regmap, reg, &val, sizeof(u8));
}

static int husb311_write16(struct husb311_chip *chip, unsigned int reg, u16 val)
{
	return regmap_raw_write(chip->data.regmap, reg, &val, sizeof(u16));
}

static int tcpci_read16(struct tcpci *tcpci, unsigned int reg, u16 *val)
{
	return regmap_raw_read(tcpci->regmap, reg, val, sizeof(u16));
}

static int tcpci_write16(struct tcpci *tcpci, unsigned int reg, u16 val)
{
	return regmap_raw_write(tcpci->regmap, reg, &val, sizeof(u16));
}

static const struct regmap_config husb311_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xFF, /* 0x80 .. 0xFF are vendor defined */
};

static struct husb311_chip *tdata_to_husb311(struct tcpci_data *tdata)
{
	return container_of(tdata, struct husb311_chip, data);
}

static int husb311_sw_reset(struct husb311_chip *chip)
{
	/* soft reset */
	return husb311_write8(chip, HUSB311_REG_SWRESET, 0x01);
}

static int husb311_init(struct tcpci *tcpci, struct tcpci_data *tdata)
{
	int ret;
	struct husb311_chip *chip = tdata_to_husb311(tdata);

	/* I2C reset : (val + 1) * 12.5ms */
	ret = husb311_write8(chip, HUSB311_REG_I2CRST_CTRL, 0x8F);
	/* UFP Both RD setting : DRP = 0, RpVal = 0 (Default), Rd, Rd */
	ret |= husb311_write8(chip, TCPC_ROLE_CTRL, TCPC_ROLE_CTRL_SET(0, 0, TCPC_ROLE_CTRL_CC_RD, TCPC_ROLE_CTRL_CC_RD));
	/* tTCPCfilter : (26.7 * val) us */
	ret |= husb311_write8(chip, HUSB311_REG_TTCPC_FILTER, 0x0A);
	/* tDRP : (51.2 + 6.4 * val) ms */
	ret |= husb311_write8(chip, HUSB311_REG_DRP_TOGGLE_CYCLE, 0x04);
	/* dcSRC.DRP : 33% */
	ret |= husb311_write16(chip, HUSB311_REG_DRP_DUTY_CTRL, 330);
	/* Vconn OC */
	ret |= husb311_write8(chip, HUSB311_REG_VCONN_CLIMITEN, 0x1);
	/* CK_300K from 320K, SHIPPING off, AUTOIDLE enable, TIMEOUT = 6.4ms */
	ret |= husb311_write8(chip, HUSB311_REG_IDLE_CTRL, HUSB311_REG_IDLE_SET(0, 1, 1, 0));

	if (ret < 0)
		dev_err(chip->dev, "fail to init registers(%d)\n", ret);

	return ret;
}

static int husb311_set_vconn(struct tcpci *tcpci, struct tcpci_data *tdata, bool enable)
{
	int ret;
	struct husb311_chip *chip = tdata_to_husb311(tdata);

	ret = husb311_write8(chip, HUSB311_REG_IDLE_CTRL,
			     HUSB311_REG_IDLE_SET(0, 1, enable ? 0 : 1, 0));

	if (ret < 0)
		dev_err(chip->dev, "fail to set vconn(%d)\n", ret);

	return ret;
}

static void mod_try_vbus_check(struct husb311_chip *chip, bool on)
{
	s64 elapsed_ms = 0;

	if (!chip->vbus_debounce_quirk) {
		dev_dbg(chip->dev, "vbus debounce disabled, not need to check.");
		return;
	}

	if (on) {
		chip->try_on = ktime_get();
		elapsed_ms = ktime_to_ms(ktime_sub(chip->try_on, chip->closed));
		/* Once elapsed time beyond try on debounce time, we can consider it disconnect. */
		if (chip->need_check && elapsed_ms < chip->vbus_tryon_debounce)
			chip->can_skip = true;
		else
			chip->can_skip = false;
		dev_dbg(chip->dev, "try set vbus %s elapsed %lld ms, skip: %s",
			on ? "On" : "Off", elapsed_ms, chip->can_skip ? "true" : "false");
	} else {
		chip->closed = ktime_get();
		elapsed_ms = ktime_to_ms(ktime_sub(chip->closed, chip->try_on));
		/* Once elapsed time beyond check debounce time, we can ignore it. */
		if (elapsed_ms < chip->vbus_check_debounce)
			chip->need_check = true;
		else
			chip->need_check = false;
		dev_dbg(chip->dev, "try set vbus %s elapsed %lld ms, check: %s",
			on ? "On" : "Off", elapsed_ms, chip->need_check ? "true" : "false");
	}
}

static int husb311_set_vbus(struct tcpci *tcpci, struct tcpci_data *tdata,
			    bool on, bool charge)
{
	struct husb311_chip *chip = tdata_to_husb311(tdata);
	int ret = 0;
	union power_supply_propval temp;

	mod_try_vbus_check(chip, on);
	if (chip->vbus_on == on) {
		dev_info(chip->dev, "vbus is already %s", on ? "On" : "Off");
		goto done;
	}
	if (chip->vbus_debounce_quirk && on && chip->can_skip) {
		dev_warn(chip->dev, "Refuse Set Vbus On");
		goto done;
	}

	if (chip->usb_psy) {
		temp.intval = on ? 1 : 0;
		power_supply_set_property(chip->usb_psy, POWER_SUPPLY_PROP_ONLINE, &temp);
	}

	dev_info(chip->dev, "set vbus %s", on ? "On" : "Off");

	if (on)
		ret = regulator_enable(chip->vbus);
	else
		ret = regulator_disable(chip->vbus);
	if (ret < 0) {
		dev_err(chip->dev, "cannot %s vbus regulator, ret=%d",
			on ? "enable" : "disable", ret);
		goto done;
	}

	chip->vbus_on = on;

done:
	return ret;
}

static inline struct tcpci *tcpc_to_tcpci(struct tcpc_dev *tcpc)
{
	return container_of(tcpc, struct tcpci, tcpc);
}

static int husb311_set_cc(struct tcpc_dev *tcpc, enum typec_cc_status cc)
{
	struct tcpci *tcpci = tcpc_to_tcpci(tcpc);
	struct husb311_chip *chip = tdata_to_husb311(tcpci->data);
	bool vconn_pres;
	enum typec_cc_polarity polarity = TYPEC_POLARITY_CC1;
	unsigned int reg;
	int ret;

	dev_dbg(chip->dev, "Setting CC Status %s", typec_cc_status_name[cc]);

	ret = regmap_read(tcpci->regmap, TCPC_POWER_STATUS, &reg);
	if (ret < 0)
		return ret;

	vconn_pres = !!(reg & TCPC_POWER_STATUS_VCONN_PRES);
	if (vconn_pres) {
		ret = regmap_read(tcpci->regmap, TCPC_TCPC_CTRL, &reg);
		if (ret < 0)
			return ret;

		if (reg & TCPC_TCPC_CTRL_ORIENTATION)
			polarity = TYPEC_POLARITY_CC2;
	}

	switch (cc) {
	case TYPEC_CC_RA:
		reg = (TCPC_ROLE_CTRL_CC_RA << TCPC_ROLE_CTRL_CC1_SHIFT) |
			(TCPC_ROLE_CTRL_CC_RA << TCPC_ROLE_CTRL_CC2_SHIFT);
		break;
	case TYPEC_CC_RD:
		reg = (TCPC_ROLE_CTRL_CC_RD << TCPC_ROLE_CTRL_CC1_SHIFT) |
			(TCPC_ROLE_CTRL_CC_RD << TCPC_ROLE_CTRL_CC2_SHIFT);
		break;
	case TYPEC_CC_RP_DEF:
		reg = (TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC1_SHIFT) |
			(TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC2_SHIFT) |
			(TCPC_ROLE_CTRL_RP_VAL_DEF <<
			 TCPC_ROLE_CTRL_RP_VAL_SHIFT);
		break;
	case TYPEC_CC_RP_1_5:
		reg = (TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC1_SHIFT) |
			(TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC2_SHIFT) |
			(TCPC_ROLE_CTRL_RP_VAL_1_5 <<
			 TCPC_ROLE_CTRL_RP_VAL_SHIFT);
		break;
	case TYPEC_CC_RP_3_0:
		reg = (TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC1_SHIFT) |
			(TCPC_ROLE_CTRL_CC_RP << TCPC_ROLE_CTRL_CC2_SHIFT) |
			(TCPC_ROLE_CTRL_RP_VAL_3_0 <<
			 TCPC_ROLE_CTRL_RP_VAL_SHIFT);
		break;
	case TYPEC_CC_OPEN:
		if (!chip->battery_exist)
			break;
		fallthrough;
	default:
		reg = (TCPC_ROLE_CTRL_CC_OPEN << TCPC_ROLE_CTRL_CC1_SHIFT) |
			(TCPC_ROLE_CTRL_CC_OPEN << TCPC_ROLE_CTRL_CC2_SHIFT);
		break;
	}

	if (vconn_pres) {
		if (polarity == TYPEC_POLARITY_CC2) {
			reg &= ~(TCPC_ROLE_CTRL_CC1_MASK << TCPC_ROLE_CTRL_CC1_SHIFT);
			reg |= (TCPC_ROLE_CTRL_CC_OPEN << TCPC_ROLE_CTRL_CC1_SHIFT);
		} else {
			reg &= ~(TCPC_ROLE_CTRL_CC2_MASK << TCPC_ROLE_CTRL_CC2_SHIFT);
			reg |= (TCPC_ROLE_CTRL_CC_OPEN << TCPC_ROLE_CTRL_CC2_SHIFT);
		}
	}

	ret = regmap_write(tcpci->regmap, TCPC_ROLE_CTRL, reg);
	if (ret < 0)
		return ret;

	return 0;
}

static int husb311_get_current_limit(struct tcpc_dev *tcpc)
{
	union power_supply_propval temp;
	struct tcpci *tcpci = tcpc_to_tcpci(tcpc);
	struct husb311_chip *chip = tdata_to_husb311(tcpci->data);
	u32 limit;

	power_supply_get_property(chip->usb_psy, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &temp);
	if (temp.intval)
		limit = temp.intval;
	else
		limit = 0;
	dev_info(chip->dev, "get current limit %u mA", limit);

	return limit;
}

static int husb311_set_current_limit(struct tcpc_dev *tcpc, u32 max_ma, u32 mv)
{
	union power_supply_propval temp;
	struct tcpci *tcpci = tcpc_to_tcpci(tcpc);
	struct husb311_chip *chip = tdata_to_husb311(tcpci->data);
	int ret = 0;

	dev_info(chip->dev, "Setting voltage/current limit %u mV %u mA", mv, max_ma);

	temp.intval = mv;
	power_supply_set_property(chip->usb_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &temp);

	temp.intval = max_ma;
	power_supply_set_property(chip->usb_psy, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &temp);

	return ret;
}

static void husb311_init_tcpci_data(struct husb311_chip *chip)
{
	if (chip->vbus)
		chip->data.set_vbus = husb311_set_vbus;
	chip->data.init = husb311_init;
	chip->data.set_vconn = husb311_set_vconn;
}

static void husb311_init_tcpci_data_late(struct husb311_chip *chip)
{
	union power_supply_propval temp;
	if (!chip->usb_psy)
		return;

	power_supply_get_property(chip->usb_psy,
				  POWER_SUPPLY_PROP_CALIBRATE, &temp);
	chip->battery_exist = temp.intval ? true : false;
	dev_info(chip->dev, "battery exist: %s", chip->battery_exist ? "yes" : "no");

	chip->tcpci->tcpc.set_cc = husb311_set_cc;
	chip->tcpci->tcpc.set_current_limit = husb311_set_current_limit;
	chip->tcpci->tcpc.get_current_limit = husb311_get_current_limit;
	dev_dbg(chip->dev, "Vbus Debounce: %s, Time: %d-%d (ms)", chip->vbus_debounce_quirk ? "enabled" : "disabled",
		chip->vbus_tryon_debounce, chip->vbus_check_debounce);
}

irqreturn_t tcpci_irq_override(struct tcpci *tcpci)
{
	u16 status;
	int ret;
	unsigned int raw;

	tcpci_read16(tcpci, TCPC_ALERT, &status);

	/*
	 * Clear alert status for everything except RX_STATUS, which shouldn't
	 * be cleared until we have successfully retrieved message.
	 */
	if (status & ~TCPC_ALERT_RX_STATUS)
		tcpci_write16(tcpci, TCPC_ALERT,
			      status & ~TCPC_ALERT_RX_STATUS);

	if (status & TCPC_ALERT_CC_STATUS)
		tcpm_cc_change(tcpci->port);

	if (status & TCPC_ALERT_POWER_STATUS) {
		regmap_read(tcpci->regmap, TCPC_POWER_STATUS_MASK, &raw);
		/*
		 * If power status mask has been reset, then the TCPC
		 * has reset.
		 */
		if (raw == 0xff)
			tcpm_tcpc_reset(tcpci->port);
		else
			tcpm_vbus_change(tcpci->port);
	}

	if (status & TCPC_ALERT_RX_STATUS) {
		struct pd_message msg;
		unsigned int cnt, payload_cnt;
		u16 header;

		regmap_read(tcpci->regmap, TCPC_RX_BYTE_CNT, &cnt);
		/*
		 * 'cnt' corresponds to READABLE_BYTE_COUNT in section 4.4.14
		 * of the TCPCI spec [Rev 2.0 Ver 1.0 October 2017] and is
		 * defined in table 4-36 as one greater than the number of
		 * bytes received. And that number includes the header. So:
		 */
		if (cnt > 3)
			payload_cnt = cnt - (1 + sizeof(msg.header));
		else
			payload_cnt = 0;

		tcpci_read16(tcpci, TCPC_RX_HDR, &header);
		msg.header = cpu_to_le16(header);

		if (WARN_ON(payload_cnt > sizeof(msg.payload)))
			payload_cnt = sizeof(msg.payload);

		if (payload_cnt > 0)
			regmap_raw_read(tcpci->regmap, TCPC_RX_DATA,
					&msg.payload, payload_cnt);

		/* Read complete, clear RX status alert bit */
		tcpci_write16(tcpci, TCPC_ALERT, TCPC_ALERT_RX_STATUS);

		tcpm_pd_receive(tcpci->port, &msg);
	}

	if (tcpci->data->vbus_vsafe0v && (status & TCPC_ALERT_EXTENDED_STATUS)) {
		ret = regmap_read(tcpci->regmap, TCPC_EXTENDED_STATUS, &raw);
		if (!ret && (raw & TCPC_EXTENDED_STATUS_VSAFE0V))
			tcpm_vbus_change(tcpci->port);
	}

	if (status & TCPC_ALERT_RX_HARD_RST)
		tcpm_pd_hard_reset(tcpci->port);

	if (status & TCPC_ALERT_TX_SUCCESS)
		tcpm_pd_transmit_complete(tcpci->port, TCPC_TX_SUCCESS);
	else if (status & TCPC_ALERT_TX_DISCARDED)
		tcpm_pd_transmit_complete(tcpci->port, TCPC_TX_DISCARDED);
	else if (status & TCPC_ALERT_TX_FAILED)
		tcpm_pd_transmit_complete(tcpci->port, TCPC_TX_FAILED);

	if (status & TCPC_ALERT_TX_DISCARDED || status & TCPC_ALERT_TX_FAILED)
		regmap_write(tcpci->regmap, TCPC_COMMAND, TCPC_CMD_RESETTRANSMITBUFFER);

	return IRQ_HANDLED;
}

static irqreturn_t husb311_irq(int irq, void *dev_id)
{
	struct husb311_chip *chip = dev_id;

	/**
	 * NOTE:
	 * We provide tcpci_irq_override instead of tcpci_irq for vendor hooks.
	 */

	queue_delayed_work(system_power_efficient_wq, &chip->wq_detcable,
			   chip->debounce_jiffies);

	return tcpci_irq_override(chip->tcpci);
}

static int husb311_init_gpio_optional(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;
	int ret = 0;
	int gpio_int_n;

	ret = of_get_named_gpio(np, "power_gpios", 0);
	if (ret < 0) {
		dev_dbg(dev, "no power_gpios info, ret = %d\n", ret);
		return ret;
	}
	gpio_int_n = ret;

	ret = devm_gpio_request(dev, gpio_int_n, "power_gpios");
	if (ret < 0) {
		dev_err(dev, "failed to request GPIO%d (ret = %d)\n", gpio_int_n, ret);
		return ret;
	}
	/* power on avoid i2c bus is stuck low. */
	gpio_direction_output(gpio_int_n, 1);

	gpio_free(gpio_int_n);

	return 0;
}
static int husb311_init_gpio(struct husb311_chip *chip)
{
	struct device *dev = chip->dev;
	struct gpio_desc *desc;
	int irq;

	irq = platform_get_irq(to_platform_device(dev), 0);
	if (irq < 0) {
		dev_err(dev, "failed to get irq: %d\n", irq);
		return irq;
	}

	chip->gpio_int_n_irq = irq;

        return 0;
}


//static int husb311_init_gpio(struct husb311_chip *chip)
//{
//	struct device *dev = chip->dev;
//	struct device_node *np = dev->of_node;
//	int ret = 0;
//
//	ret = of_get_named_gpio(np, "husb311,intr_gpio", 0);
//	if (ret < 0) {
//		dev_err(dev, "no intr_gpio info, ret = %d\n", ret);
//		return ret;
//	}
//	chip->gpio_int_n = ret;
//
//	ret = devm_gpio_request(chip->dev, chip->gpio_int_n, "husb311,intr_gpio");
//	if (ret < 0) {
//		dev_err(dev, "failed to request GPIO%d (ret = %d)\n", chip->gpio_int_n, ret);
//		return ret;
//	}
//
//	ret = gpio_direction_input(chip->gpio_int_n);
//	if (ret < 0) {
//		dev_err(dev, "failed to set GPIO%d as input pin(ret = %d)\n", chip->gpio_int_n, ret);
//		return ret;
//	}
//
//	ret = gpio_to_irq(chip->gpio_int_n);
//	if (ret < 0) {
//		dev_err(dev,
//			"cannot request IRQ for GPIO Int_N, ret=%d", ret);
//		return ret;
//	}
//	chip->gpio_int_n_irq = ret;
//
//	return 0;
//}

static int husb311_check_revision(struct i2c_client *i2c)
{
	int ret;

	husb311_init_gpio_optional(i2c);

	ret = i2c_smbus_read_word_data(i2c, TCPC_VENDOR_ID);
	if (ret < 0) {
		dev_err(&i2c->dev, "fail to read Vendor id(%d)\n", ret);
		return ret;
	}

	if (ret != HUSB311_VENDOR_ID) {
		dev_err(&i2c->dev, "vid is not correct, 0x%04x\n", ret);
		return -ENODEV;
	}

	ret = i2c_smbus_read_word_data(i2c, TCPC_PRODUCT_ID);
	if (ret < 0) {
		dev_err(&i2c->dev, "fail to read Product id(%d)\n", ret);
		return ret;
	}

	if (ret != HUSB311_PRODUCT_ID) {
		dev_err(&i2c->dev, "pid is not correct, 0x%04x\n", ret);
		return -ENODEV;
	}

	return 0;
}

static void husb311_detect_cable(struct work_struct *work)
{
	struct husb311_chip *chip = container_of(to_delayed_work(work), struct husb311_chip,
						 wq_detcable);
	enum typec_cc_status cc1, cc2;
	union power_supply_propval temp;

	chip->tcpci->tcpc.get_cc(&chip->tcpci->tcpc, &cc1, &cc2);

	//dev_info(chip->dev, "CC1: %d - %s, CC2: %d - %s\n",
	//	 cc1, typec_cc_status_name[cc1], cc2, typec_cc_status_name[cc2]);

	/**
	 * FIXME:
	 * 1. Support double Rp to Vbus cable as sink and device.
	 * 2. Update symbol list for 'usb_role_switch_get_role' function.
	 */
	if (!tcpci_port_is_sink(cc1, cc2) && !tcpci_port_is_source(cc1, cc2)) {

		dev_dbg(chip->dev, "Setting Role [%s]\n", usb_role_name[USB_ROLE_NONE]);
		if (chip->usb_psy) {
			temp.intval = 0;
			power_supply_set_property(chip->usb_psy, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &temp);
		}
		usb_role_switch_set_role(chip->role_sw, USB_ROLE_NONE);

	} else if (tcpci_cc_is_sink(cc1) && tcpci_cc_is_sink(cc2)) {

		dev_dbg(chip->dev, "Setting Role [%s]\n", usb_role_name[USB_ROLE_DEVICE]);
		if (chip->usb_psy) {
			temp.intval = 500;
			power_supply_set_property(chip->usb_psy, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &temp);
		}
		usb_role_switch_set_role(chip->role_sw, USB_ROLE_DEVICE);
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
static int husb311_probe(struct i2c_client *client,
			 const struct i2c_device_id *i2c_id)
#else
static int husb311_probe(struct i2c_client *client)
#endif
{
	int ret;
	struct husb311_chip *chip;

	if (i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_I2C_BLOCK | I2C_FUNC_SMBUS_BYTE_DATA))
		pr_info("I2C functionality : OK...\n");
	else
		pr_info("I2C functionality check : failuare...\n");
	ret = husb311_check_revision(client);
	if (ret < 0) {
		dev_err(&client->dev, "check vid/pid fail(%d)\n", ret);
		return ret;
	}

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->data.regmap = devm_regmap_init_i2c(client,
						 &husb311_regmap_config);
	if (IS_ERR(chip->data.regmap)) {
		dev_err(&client->dev, "husb311 regmap init fail\n");
		return PTR_ERR(chip->data.regmap);
	}

	chip->dev = &client->dev;
	i2c_set_clientdata(client, chip);

	if (of_find_property(chip->dev->of_node, "det_usb_supply", NULL)) {
		chip->usb_psy = devm_power_supply_get_by_phandle(chip->dev, "det_usb_supply");
	} else {
		pr_err("husb311 failed to find usb power\n");
		chip->usb_psy =  NULL;
	}

	chip->vbus = devm_regulator_get_optional(chip->dev, "vbus");
	if (IS_ERR(chip->vbus)) {
		ret = PTR_ERR(chip->vbus);
		chip->vbus = NULL;
		if (ret != -ENODEV)
			return ret;
	}
	/* Here are some compatible issues for Type-C Port Controller. */
	chip->vbus_debounce_quirk = device_property_read_bool(chip->dev, "aw,vbus-debounce-quirk");
	device_property_read_u32(chip->dev, "aw,vbus-tryon-debounce", &chip->vbus_tryon_debounce);
	device_property_read_u32(chip->dev, "aw,vbus-check-debounce", &chip->vbus_check_debounce);
	chip->port_reset_quirk = device_property_read_bool(chip->dev, "aw,port-reset-quirk");

	chip->debounce_jiffies = msecs_to_jiffies(HUSB311_TCPM_DEBOUNCE_MS);
	INIT_DELAYED_WORK(&chip->wq_detcable, husb311_detect_cable);

	ret = husb311_sw_reset(chip);
	if (ret < 0) {
		dev_err(chip->dev, "fail to soft reset, ret = %d\n", ret);
		return ret;
	}

	if (client->irq) {
		chip->gpio_int_n_irq = client->irq;
	} else {
		ret = husb311_init_gpio(chip);
		if (ret < 0)
			return ret;
		client->irq = chip->gpio_int_n_irq;
	}

	husb311_init_tcpci_data(chip);

	chip->tcpci = tcpci_register_port(chip->dev, &chip->data);
	if (IS_ERR(chip->tcpci)) {
		dev_err(chip->dev, "fail to register tcpci port, %ld\n", PTR_ERR(chip->tcpci));
		return PTR_ERR(chip->tcpci);
	}

	husb311_init_tcpci_data_late(chip);

	chip->role_sw = usb_role_switch_get(chip->dev);
	if (IS_ERR(chip->role_sw)) {
		ret = PTR_ERR(chip->role_sw);
		dev_err(chip->dev, "fail to get usb role switch, %ld\n", PTR_ERR(chip->role_sw));
		return ret;
	}

	ret = devm_request_threaded_irq(chip->dev, client->irq, NULL,
					husb311_irq,
					IRQF_ONESHOT | IRQF_TRIGGER_LOW,
					client->name, chip);
	if (ret < 0) {
		dev_err(chip->dev, "fail to request irq %d, ret = %d\n", client->irq, ret);
		usb_role_switch_put(chip->role_sw);
		tcpci_unregister_port(chip->tcpci);
		return ret;
	}

	//if (!device_property_read_bool(chip->dev, "wakeup-source")) {
	//	dev_info(chip->dev, "wakeup source is disabled!\n");
	//} else {
	//	if (chip->gpio_int_n_irq > 0) {
	//		ret = dev_pm_set_wake_irq(chip->dev, client->irq);
	//		if (ret < 0) {
	//			dev_err(chip->dev, "failed to set wake IRQ %d: %d\n", client->irq, ret);
	//			return ret;
	//		}
	//	}
	//}

	dev_info(chip->dev, "Vendor ID:0x%04x, Product ID:0x%04x, probe success\n", HUSB311_VENDOR_ID, HUSB311_PRODUCT_ID);

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
static int husb311_remove(struct i2c_client *client)
#else
static void husb311_remove(struct i2c_client *client)
#endif
{
	struct husb311_chip *chip = i2c_get_clientdata(client);

	if (device_may_wakeup(chip->dev)) {
		dev_pm_clear_wake_irq(chip->dev);
		device_init_wakeup(chip->dev, false);
	}
	cancel_delayed_work_sync(&chip->wq_detcable);
	usb_role_switch_put(chip->role_sw);
	tcpci_unregister_port(chip->tcpci);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
	return 0;
#endif
}

/*
 * NOTE: The system is about to shutdown and poweroff, that is to say,
 * we'd better hard reset the port and notify attached device.
 */
static void husb311_shutdown(struct i2c_client *client)
{
	int ret;
	struct husb311_chip *chip = i2c_get_clientdata(client);

	disable_irq(chip->gpio_int_n_irq);
	if (!chip->battery_exist) {
		dev_dbg(chip->dev, "no battery, hardreset forbidden");
		return;
	}
	/* Transmit Hard Reset: nRetryCount is 3 in PD2.0 spec where 2 in PD3.0 spec */
	ret = husb311_write8(chip, TCPC_TRANSMIT,
			     (0x2 << TCPC_TRANSMIT_RETRY_SHIFT) | (TCPC_TX_HARD_RESET << TCPC_TRANSMIT_TYPE_SHIFT));
	if (ret < 0)
		dev_err(chip->dev, "cannot send hardreset, ret=%d", ret);
}

static int husb311_pm_suspend(struct device *dev)
{
	struct husb311_chip *chip = dev->driver_data;
	int ret = 0;
	u8 pwr;

	cancel_delayed_work_sync(&chip->wq_detcable);
	/*
	 * When system suspend, disable irq to prevent interrupt trigger
	 * during I2C bus suspend
	 */
	if (!device_may_wakeup(chip->dev))
		disable_irq(chip->gpio_int_n_irq);
	/*
	 * Disable 12M oscillator to save power consumption, and it will be
	 * enabled automatically when INT occur after system resume.
	 */
	ret = husb311_read8(chip, HUSB311_REG_BMC_CTRL, &pwr);
	if (ret < 0)
		return ret;

	pwr &= ~BIT(0);
	ret = husb311_write8(chip, HUSB311_REG_BMC_CTRL, pwr);
	if (ret < 0)
		return ret;

	return 0;
}

static int husb311_pm_resume(struct device *dev)
{
	struct husb311_chip *chip = dev->driver_data;
	int ret = 0;
	u8 pwr;

	/* Enable irq after I2C bus already resume */
	if (!device_may_wakeup(chip->dev))
		enable_irq(chip->gpio_int_n_irq);

	queue_delayed_work(system_power_efficient_wq,
			   &chip->wq_detcable, chip->debounce_jiffies);

	if (!chip->port_reset_quirk) {
		dev_info(chip->dev, "disable reset this port\n");
		return 0;
	}

	/*
	 * When the power of husb311 is lost or i2c read failed in PM S/R
	 * process, we must reset the tcpm port first to ensure the devices
	 * can attach again.
	 */
	ret = husb311_read8(chip, HUSB311_REG_BMC_CTRL, &pwr);
	if (pwr & BIT(0) || ret < 0) {
		ret = husb311_sw_reset(chip);
		if (ret < 0) {
			dev_err(chip->dev, "fail to soft reset, ret = %d\n", ret);
			return ret;
		}

		tcpm_tcpc_reset(tcpci_get_tcpm_port(chip->tcpci));
		dev_info(chip->dev, "reset the port success(%d)\n", pwr);
	}

	return 0;
}

static const struct i2c_device_id husb311_id[] = {
	{ "husb311", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, husb311_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id husb311_of_match[] = {
	{ .compatible = "hynetek,husb311" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, husb311_of_match);
#endif

static const struct dev_pm_ops husb311_pm_ops = {
	.suspend = husb311_pm_suspend,
	.resume = husb311_pm_resume,
};

static struct i2c_driver husb311_i2c_driver = {
	.driver = {
		.name = "husb311",
		.pm = &husb311_pm_ops,
		.of_match_table = of_match_ptr(husb311_of_match),
	},
	.probe = husb311_probe,
	.remove = husb311_remove,
	.shutdown = husb311_shutdown,
	.id_table = husb311_id,
};
module_i2c_driver(husb311_i2c_driver);

MODULE_ALIAS("platform:husb311-i2c-driver");
MODULE_DESCRIPTION("Husb311 USB Type-C Port Controller Interface Driver");
MODULE_AUTHOR("kanghoupeng<kanghoupeng@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.19");

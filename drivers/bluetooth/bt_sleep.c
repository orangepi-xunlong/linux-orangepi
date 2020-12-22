/*

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.


   Copyright (C) 2006-2007 - Motorola
   Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.

   Date         Author           Comment
   -----------  --------------   --------------------------------
   2006-Apr-28	Motorola	 The kernel module for running the Bluetooth(R)
				 Sleep-Mode Protocol from the Host side
   2006-Sep-08  Motorola         Added workqueue for handling sleep work.
   2007-Jan-24  Motorola         Added mbm_handle_ioi() call to ISR.
   2013-Jun-13	Magic            Modified for EVB
   2013-Mar-27	Magic            Modified for support BLE
   2013-Jul-24  Huzhen           Modified for aw
   2015-Apr-13  Blue             Modified for aw
*/

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>

#include <linux/irq.h>
#include <linux/param.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <mach/sys_config.h>
#include <mach/irqs.h>
#include <mach/gpio.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <linux/serial_core.h>
#include "hci_uart.h"

#define BT_SLEEP_DBG
#ifdef  BT_SLEEP_DBG
#define BT_LPM_DBG(fmt, arg...) printk(KERN_ERR "[BT_LPM] %s: " fmt "\n" , __func__ , ## arg)
#else
#define BT_LPM_DBG(fmt, arg...)
#endif
/*
 * Defines
 */

#define VERSION		"1.2"
#define PROC_DIR	"bluetooth/sleep"

#define BT_BLUEDROID_SUPPORT 1
static int bluesleep_start(void);
static void bluesleep_stop(void);

struct bluesleep_info {
	unsigned host_wake;
	unsigned ext_wake;
	unsigned host_wake_irq;
	struct wake_lock wake_lock;
	struct uart_port *uport;
};

/* work function */
static void bluesleep_sleep_work(struct work_struct *work);

static int no_usb = 0;

/* work queue */
DECLARE_DELAYED_WORK(sleep_workqueue, bluesleep_sleep_work);

/* Macros for handling sleep work */
#define bluesleep_rx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_rx_idle()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_idle()     schedule_delayed_work(&sleep_workqueue, 0)

/* 10 second timeout */
#define TX_TIMER_INTERVAL	10

/* state variable names and bit positions */
#define BT_PROTO	0x01
#define BT_TXDATA	0x02
#define BT_ASLEEP	0x04

#if BT_BLUEDROID_SUPPORT
static bool has_lpm_enabled = false;
#else
/* global pointer to a single hci device. */
static struct hci_dev *bluesleep_hdev;
#endif

#if BT_BLUEDROID_SUPPORT
static struct platform_device *bluesleep_uart_dev = NULL;
#endif
static struct bluesleep_info *bsi;

/* module usage */
static atomic_t open_count = ATOMIC_INIT(1);

/*
 * Local function prototypes
 */

#if !BT_BLUEDROID_SUPPORT
static int bluesleep_hci_event(struct notifier_block *this,
			    unsigned long event, void *data);
#endif

/*
 * Global variables
 */

static int host_wake_invert = 0;

/** Global state flags */
static unsigned long flags;

/** Tasklet to respond to change in hostwake line */
static struct tasklet_struct hostwake_task;

/** Transmission timer */
static struct timer_list tx_timer;

/** Lock for state transitions */
static spinlock_t rw_lock;

#if !BT_BLUEDROID_SUPPORT
/** Notifier block for HCI events */
struct notifier_block hci_event_nblock = {
	.notifier_call = bluesleep_hci_event,
};
#endif

struct proc_dir_entry *bluetooth_dir, *sleep_dir;

static int uart_id = 2, baud_rate = 1500000;

static int vendor_id = 0;
/*
 * Local functions
 */

/*
 * return 0: not handle read or write proc info operation.
 * return 1: handle read or write proc info operation.
*/
static inline int read_write_proc_info(void)
{
	if ((no_usb == 1) || (vendor_id == 2))
		return 0;
	return 1;
}

/*
 * bt go to sleep will call this function tell uart stop data interactive
 */
static void hsuart_power(int on)
{
	if (on) {
		bsi->uport->ops->set_mctrl(bsi->uport, TIOCM_RTS);
	} else {
		bsi->uport->ops->set_mctrl(bsi->uport, 0);
	}
}

/**
 * @return 1 if the Host can go to sleep, 0 otherwise.
 */
static inline int bluesleep_can_sleep(void)
{
	/* check if HOST_WAKE_BT_GPIO and BT_WAKE_HOST_GPIO are both deasserted */
	if (!host_wake_invert) {
		return !gpio_get_value(bsi->ext_wake) &&
			!gpio_get_value(bsi->host_wake) &&
			(bsi->uport != NULL);
	} else {
		return !gpio_get_value(bsi->ext_wake) &&
			gpio_get_value(bsi->host_wake) &&
			(bsi->uport != NULL);
	}
}

/*
 * after bt wakeup should clean BT_ASLEEP flag and start time.
 */
void bluesleep_sleep_wakeup(void)
{
	if (test_bit(BT_ASLEEP, &flags)) {
		BT_LPM_DBG("waking up...");
		/* Start the timer */
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
		gpio_set_value(bsi->ext_wake, 1);
		clear_bit(BT_ASLEEP, &flags);
		/*Activating UART */
		hsuart_power(1);
	}
}

/**
 * @brief@  main sleep work handling function which update the flags
 * and activate and deactivate UART ,check FIFO.
 */
static void bluesleep_sleep_work(struct work_struct *work)
{
	if (bluesleep_can_sleep()) {
		/* already asleep, this is an error case */
		if (test_bit(BT_ASLEEP, &flags)) {
			BT_LPM_DBG("already asleep");
			return;
		}
		if (bsi->uport->ops->tx_empty(bsi->uport)) {
			BT_LPM_DBG("going to sleep...");
			set_bit(BT_ASLEEP, &flags);
			/*Deactivating UART */
			hsuart_power(0);
			wake_lock_timeout(&bsi->wake_lock, HZ / 2);
		} else {
		  	mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
			return;
		}
	} else if(!gpio_get_value(bsi->ext_wake) && !test_bit(BT_ASLEEP, &flags) ) {
		gpio_set_value(bsi->ext_wake, 1);
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
	}else {
		bluesleep_sleep_wakeup();
	}
}

/**
 * A tasklet function that runs in tasklet context and reads the value
 * of the HOST_WAKE GPIO pin and further defer the work.
 * @param data Not used.
 */
static void bluesleep_hostwake_task(unsigned long data)
{
	//BT_LPM_DBG("hostwake line change");
	spin_lock(&rw_lock);

    if (!host_wake_invert) {
        if (gpio_get_value(bsi->host_wake))
            bluesleep_rx_busy();
        else
            bluesleep_rx_idle();
    } else {
        if (!gpio_get_value(bsi->host_wake))
            bluesleep_rx_busy();
        else
            bluesleep_rx_idle();
    }

    spin_unlock(&rw_lock);
}

/**
 * Handles proper timer action when outgoing data is delivered to the
 * HCI line discipline. Sets BT_TXDATA.
 */
static void bluesleep_outgoing_data(void)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	/* log data passing by */
	set_bit(BT_TXDATA, &flags);
	/* if the tx side is sleeping... */
	if (!gpio_get_value(bsi->ext_wake)) {

		BT_LPM_DBG("tx was sleeping");
		bluesleep_sleep_wakeup();
	}

	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

#if BT_BLUEDROID_SUPPORT
static struct uart_port *bluesleep_get_uart_port(void)
{
	struct uart_port *uport = NULL;
	if (bluesleep_uart_dev){
		uport = platform_get_drvdata(bluesleep_uart_dev);
		if(uport)
			BT_LPM_DBG("%s get uart_port from blusleep_uart_dev: %s, port irq: %d",
					__FUNCTION__, bluesleep_uart_dev->name, uport->irq);
    }

	return uport;
}

static int bluesleep_read_proc_lpm(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	if (!read_write_proc_info())
		return 0;

	*eof = 1;
	return sprintf(page, "unsupported to read\n");
}

static int bluesleep_write_proc_lpm(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char b;

	if (!read_write_proc_info())
		return 0;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	if (b == '0') {
		/* HCI_DEV_UNREG */
		bluesleep_stop();
		has_lpm_enabled = false;
		bsi->uport = NULL;
	} else {
		/* HCI_DEV_REG */
		if (!has_lpm_enabled) {
			has_lpm_enabled = true;
			if (bluesleep_uart_dev)
				bsi->uport = bluesleep_get_uart_port();

			/* if bluetooth started, start bluesleep*/
			bluesleep_start();
		}
	}

	return count;
}

static int bluesleep_read_proc_btwrite(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	if (!read_write_proc_info())
		return 0;

	*eof = 1;
	return sprintf(page, "unsupported to read\n");
}

static int bluesleep_write_proc_btwrite(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char b;

	if (!read_write_proc_info())
		return 0;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	/* HCI_DEV_WRITE */
	if (b != '0') {
		bluesleep_outgoing_data();
	}

	return count;
}
#else
/**
 * Handles HCI device events.
 * @param this Not used.
 * @param event The event that occurred.
 * @param data The HCI device associated with the event.
 * @return <code>NOTIFY_DONE</code>.
 */
static int bluesleep_hci_event(struct notifier_block *this,
				unsigned long event, void *data)
{
	struct hci_dev *hdev = (struct hci_dev *) data;
	struct hci_uart *hu;
	struct uart_state *state;

	if (!hdev)
		return NOTIFY_DONE;

	switch (event) {
	case HCI_DEV_REG:
		if (!bluesleep_hdev) {
			bluesleep_hdev = hdev;
			hu  = (struct hci_uart *) hdev->driver_data;
			state = (struct uart_state *) hu->tty->driver_data;
			bsi->uport = state->uart_port;
		}
		break;
	case HCI_DEV_UNREG:
		bluesleep_hdev = NULL;
		bsi->uport = NULL;
		break;
	case HCI_DEV_WRITE:
		bluesleep_outgoing_data();
		break;
	}

	return NOTIFY_DONE;
}
#endif

/**
 * Handles transmission timer expiration.
 * @param data Not used.
 */
static void bluesleep_tx_timer_expire(unsigned long data)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	//BT_LPM_DBG("Tx timer expired");

	/* were we silent during the last timeout */
	if (!test_bit(BT_TXDATA, &flags)) {
		//BT_LPM_DBG("Tx has been idle");
		gpio_set_value(bsi->ext_wake, 0);
		bluesleep_tx_idle();
	} else {
		//BT_LPM_DBG("Tx data during last period");
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));
	}

	/* clear the incoming data flag */
	clear_bit(BT_TXDATA, &flags);

	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

/**
 * Schedules a tasklet to run when receiving an interrupt on the
 * <code>HOST_WAKE</code> GPIO pin.
 * @param irq Not used.
 * @param dev_id Not used.
 */
static irqreturn_t bluesleep_hostwake_isr(int irq, void *dev_id)
{
	/* schedule a tasklet to handle the change in the host wake line */
	tasklet_schedule(&hostwake_task);
	wake_lock(&bsi->wake_lock);
	return IRQ_HANDLED;
}

/**
 * Starts the Sleep-Mode Protocol on the Host.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int bluesleep_start(void)
{
	int retval;
	unsigned long irq_flags;
	script_item_value_type_e type;
	script_item_u val;

	if (!read_write_proc_info())
		return 0;

	spin_lock_irqsave(&rw_lock, irq_flags);

	if (test_bit(BT_PROTO, &flags)) {
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return 0;
	}

	spin_unlock_irqrestore(&rw_lock, irq_flags);

	if (!atomic_dec_and_test(&open_count)) {
		atomic_inc(&open_count);
		return -EBUSY;
	}

	/* start the timer */

	mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));

	/* assert BT_WAKE */
	gpio_set_value(bsi->ext_wake, 1);
	type = script_get_item("bt_para", "bt_host_wake_invert", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		BT_INFO("has no bt_host_wake_invert\n");
	} else {
		host_wake_invert = val.val;
	}

	if (vendor_id == 1)
		host_wake_invert = !host_wake_invert;
	if(!host_wake_invert) {
		retval = request_irq(bsi->host_wake_irq, bluesleep_hostwake_isr, IRQF_DISABLED | IRQF_TRIGGER_RISING,
			"bluetooth hostwake", NULL);
	} else {
		retval = request_irq(bsi->host_wake_irq, bluesleep_hostwake_isr, IRQF_DISABLED | IRQF_TRIGGER_FALLING,
			"bluetooth hostwake", NULL);
	}
	if (retval < 0) {
		BT_ERR("Couldn't acquire bt_host_wake IRQ or enable it");
		goto fail;
	}

	set_bit(BT_PROTO, &flags);
	wake_lock(&bsi->wake_lock);

	return 0;
fail:
	del_timer(&tx_timer);
	atomic_inc(&open_count);

	return retval;
}

/**
 * Stops the Sleep-Mode Protocol on the Host.
 */
static void bluesleep_stop(void)
{
	unsigned long irq_flags;

	if (!read_write_proc_info())
		return;

	spin_lock_irqsave(&rw_lock, irq_flags);

	if (!test_bit(BT_PROTO, &flags)) {
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return;
	}

	/* assert BT_WAKE */
	gpio_set_value(bsi->ext_wake, 1);
	del_timer(&tx_timer);
	clear_bit(BT_PROTO, &flags);

	if (test_bit(BT_ASLEEP, &flags)) {
		clear_bit(BT_ASLEEP, &flags);
		hsuart_power(1);
	}

	atomic_inc(&open_count);

	spin_unlock_irqrestore(&rw_lock, irq_flags);
	free_irq(bsi->host_wake_irq, NULL);
	wake_lock_timeout(&bsi->wake_lock, HZ / 2);
}
/**
 * Read the <code>BT_WAKE</code> GPIO pin value via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the
 * pin is high, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_btwake(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	if (!read_write_proc_info())
		return 0;

	*eof = 1;
	return sprintf(page, "btwake:%u\n", gpio_get_value(bsi->ext_wake));
}

/**
 * Write the <code>BT_WAKE</code> GPIO pin value via the proc interface.
 * @param file Not used.
 * @param buffer The buffer to read from.
 * @param count The number of bytes to be written.
 * @param data Not used.
 * @return On success, the number of bytes written. On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluepower_write_proc_btwake(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char *buf;

	if (!read_write_proc_info())
		return 0;

	if (count < 1)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EFAULT;
	}

	if (buf[0] == '0') {
		gpio_set_value(bsi->ext_wake, 0);
	} else if (buf[0] == '1') {
		gpio_set_value(bsi->ext_wake, 1);
	} else {
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);
	return count;
}

/**
 * Read the <code>BT_HOST_WAKE</code> GPIO pin value via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the pin
 * is high, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_hostwake(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	if (!read_write_proc_info())
		return 0;

	*eof = 1;
	return sprintf(page, "hostwake: %u \n", gpio_get_value(bsi->host_wake));
}

static int bluepower_read_proc_uart_id(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	if (!read_write_proc_info())
		return 0;

	return sprintf(page, "%d\n", uart_id);
}

static int bluepower_read_proc_baud_rate(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	if (!read_write_proc_info())
		return 0;

	return sprintf(page, "%d\n", baud_rate);
}

static int bluepower_read_proc_vendor_id(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	return sprintf(page, "%d\n", vendor_id);
}

static int bluepower_write_proc_vendor_id(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char b;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	vendor_id = b;

	return count;
}

/**
 * Read the low-power status of the Host via the proc interface.
 * When this function returns, <code>page</code> contains a 1 if the Host
 * is asleep, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_asleep(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	unsigned int asleep;

	if (!read_write_proc_info())
		return 0;

	asleep = test_bit(BT_ASLEEP, &flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "asleep: %u\n", asleep);
}

/**
 * Read the low-power protocol being used by the Host via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the Host
 * is using the Sleep Mode Protocol, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_proto(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	unsigned int proto;

	if (!read_write_proc_info())
		return 0;

	proto = test_bit(BT_PROTO, &flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "proto: %u\n", proto);
}

/**
 * Modify the low-power protocol used by the Host via the proc interface.
 * @param file Not used.
 * @param buffer The buffer to read from.
 * @param count The number of bytes to be written.
 * @param data Not used.
 * @return On success, the number of bytes written. On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluesleep_write_proc_proto(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char proto;

	if (!read_write_proc_info())
		return 0;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&proto, buffer, 1))
		return -EFAULT;

	if (proto == '0')
		bluesleep_stop();
	else
		bluesleep_start();

	/* claim that we wrote everything */
	return count;
}

extern struct platform_device *sw_uart_get_pdev(int uart_id);
static int __init bluesleep_probe(struct platform_device *pdev)
{
	int ret;
	script_item_u val;
	script_item_value_type_e type;

    type = script_get_item("bt_para", "bt_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		BT_ERR("failed to fetch bt configuration!\n");
		return -1;
	}
	if (!val.val) {
		BT_ERR("init no bt used in configuration\n");
        no_usb = 1;
		return 0;
	}

	bsi = kzalloc(sizeof(struct bluesleep_info), GFP_KERNEL);
	if (!bsi)
		return -ENOMEM;

	//get bt_wake & bt_host_wake from sys_config.fex
	type = script_get_item("bt_para", "bt_wake", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		BT_ERR("get bt_wake gpio failed\n");
	else
		bsi->ext_wake = val.gpio.gpio;	
	
	type = script_get_item("bt_para", "bt_host_wake", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		BT_ERR("get bt_host_wake gpio failed\n");
	else
		bsi->host_wake = val.gpio.gpio;		

#if BT_BLUEDROID_SUPPORT
    type = script_get_item("bt_para", "bt_uart_id", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        BT_ERR("failed to fetch bt uart configuration.");
        return -1;
    }
    if(val.val != 0)
	{
		bluesleep_uart_dev = sw_uart_get_pdev(val.val);
		uart_id = val.val;
	}
#endif

	type = script_get_item("bt_para", "bt_uart_baud", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		BT_ERR("failed to fetch bt uart configuration.");
		return -1;
	}
	if(val.val != 0)
		baud_rate = val.val;

	wake_lock_init(&bsi->wake_lock, WAKE_LOCK_SUSPEND, "bluesleep");

	//1.set bt_wake as output and the level is 1, assert bt wake
	ret = gpio_request_one(bsi->ext_wake, GPIOF_DIR_OUT|GPIOF_INIT_HIGH, NULL);
	if (ret != 0) {
		BT_ERR("couldn't set bt_wake as output function\n");
		goto free_bsi;
	} 
	
	//2.get bt_host_wake gpio irq
	bsi->host_wake_irq = gpio_to_irq(bsi->host_wake);
	if (IS_ERR_VALUE(bsi->host_wake_irq)) {
		BT_ERR("map gpio [%d] to virq failed, errno = %d\n",bsi->host_wake, bsi->host_wake_irq);
		ret = -ENODEV;
		goto free_bt_ext_wake;
	}
	
	return 0;

free_bt_ext_wake:
	gpio_free(bsi->ext_wake);
free_bsi:
	kfree(bsi);
	return ret;
}

static int bluesleep_remove(struct platform_device *pdev)
{
	if (no_usb == 1)
		return 0;

	/* assert bt wake */
	gpio_set_value(bsi->ext_wake, 1);
	if (test_bit(BT_PROTO, &flags)) {
		free_irq(bsi->host_wake_irq, NULL);
		del_timer(&tx_timer);
		if (test_bit(BT_ASLEEP, &flags))
			hsuart_power(1);
	}

	gpio_free(bsi->host_wake);
	gpio_free(bsi->ext_wake);
	wake_lock_destroy(&bsi->wake_lock);
	kfree(bsi);
	return 0;
}

static struct platform_driver bluesleep_driver = {
	.remove = bluesleep_remove,
	.driver = {
		.name = "bluesleep",
		.owner = THIS_MODULE,
	},
};

static struct platform_device bluesleep_device[] = {
	[0] = {
		.name           	= "bluesleep",
		.id             	= 0,
		.num_resources		= 0,
		.resource       	= NULL,
	}
};

/**
 * Initializes the module.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int __init bluesleep_init(void)
{
	int retval;
	struct proc_dir_entry *ent;

  retval = platform_device_register(&bluesleep_device[0]);
	if (retval) {
		BT_ERR("%s platform_device_register error.", __FUNCTION__);
		return retval;
	}
  
	BT_INFO("MSM Sleep Mode Driver Ver %s", VERSION);

	retval = platform_driver_probe(&bluesleep_driver, bluesleep_probe);
	if (retval)
		return retval;

#if !BT_BLUEDROID_SUPPORT
	bluesleep_hdev = NULL;
#endif

	bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (bluetooth_dir == NULL) {
		BT_ERR("Unable to create /proc/bluetooth directory");
		return -ENOMEM;
	}

	if (create_proc_read_entry("uart_id", 0, bluetooth_dir,
				bluepower_read_proc_uart_id, NULL) == NULL) {
		BT_ERR("Unable to create /proc/%s/uart_id entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	if (create_proc_read_entry("baud_rate", 0, bluetooth_dir,
				bluepower_read_proc_baud_rate, NULL) == NULL) {
		BT_ERR("Unable to create /proc/%s/baud_rate entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	ent = create_proc_entry("vendor_id", 0666, bluetooth_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/vendor_id entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluepower_read_proc_vendor_id;
	ent->write_proc = bluepower_write_proc_vendor_id;

	sleep_dir = proc_mkdir("sleep", bluetooth_dir);
	if (sleep_dir == NULL) {
		BT_ERR("Unable to create /proc/%s directory", PROC_DIR);
		return -ENOMEM;
	}

	/* Creating read/write "btwake" entry */
	ent = create_proc_entry("btwake", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/btwake entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluepower_read_proc_btwake;
	ent->write_proc = bluepower_write_proc_btwake;

	/* read only proc entries */
	if (create_proc_read_entry("hostwake", 0, sleep_dir,
				bluepower_read_proc_hostwake, NULL) == NULL) {
		BT_ERR("Unable to create /proc/%s/hostwake entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	/* read/write proc entries */
	ent = create_proc_entry("proto", 0666, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/proto entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluesleep_read_proc_proto;
	ent->write_proc = bluesleep_write_proc_proto;

	/* read only proc entries */
	if (create_proc_read_entry("asleep", 0,
			sleep_dir, bluesleep_read_proc_asleep, NULL) == NULL) {
		BT_ERR("Unable to create /proc/%s/asleep entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

#if BT_BLUEDROID_SUPPORT
	/* read/write proc entries */
	ent = create_proc_entry("lpm", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/lpm entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluesleep_read_proc_lpm;
	ent->write_proc = bluesleep_write_proc_lpm;

	/* read/write proc entries */
	ent = create_proc_entry("btwrite", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/btwrite entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluesleep_read_proc_btwrite;
	ent->write_proc = bluesleep_write_proc_btwrite;
#endif

	flags = 0; /* clear all status bits */

	/* Initialize spinlock. */
	spin_lock_init(&rw_lock);

	/* Initialize timer */
	init_timer(&tx_timer);
	tx_timer.function = bluesleep_tx_timer_expire;
	tx_timer.data = 0;

	/* initialize host wake tasklet */
	tasklet_init(&hostwake_task, bluesleep_hostwake_task, 0);

#if !BT_BLUEDROID_SUPPORT
	hci_register_notifier(&hci_event_nblock);
#endif

	return 0;

fail:
#if BT_BLUEDROID_SUPPORT
	remove_proc_entry("btwrite", sleep_dir);
	remove_proc_entry("lpm", sleep_dir);
#endif
	remove_proc_entry("asleep", sleep_dir);
	remove_proc_entry("proto", sleep_dir);
	remove_proc_entry("hostwake", sleep_dir);
	remove_proc_entry("btwake", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
	return retval;
}

/**
 * Cleans up the module.
 */
static void __exit bluesleep_exit(void)
{
#if !BT_BLUEDROID_SUPPORT
	hci_unregister_notifier(&hci_event_nblock);
#endif
	platform_driver_unregister(&bluesleep_driver);

#if BT_BLUEDROID_SUPPORT
	remove_proc_entry("btwrite", sleep_dir);
	remove_proc_entry("lpm", sleep_dir);
#endif
	remove_proc_entry("asleep", sleep_dir);
	remove_proc_entry("proto", sleep_dir);
	remove_proc_entry("hostwake", sleep_dir);
	remove_proc_entry("btwake", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
}

module_init(bluesleep_init);
module_exit(bluesleep_exit);

MODULE_DESCRIPTION("Bluetooth Sleep Mode Driver ver %s " VERSION);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif

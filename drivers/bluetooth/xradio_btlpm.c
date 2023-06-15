/*
** Bluetooth sleep and wakeup host interface for XRADIO Drivers
**
** Copyright (c) 2013, XRadio
** Author: XRadio
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License version 2 as
** published by the Free Software Foundation.
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
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/pm_wakeirq.h>
#include <linux/serial_core.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>


#define BT_SLEEP_DEBUG

#ifdef BT_SLEEP_DEBUG
#define BT_SLEEP_DBG(fmt, arg...) printk(KERN_DEBUG "[XR_BT_LPM] %s: " fmt "\n", __func__, ## arg)
#else
#define BT_SLEEP_DBG(fmt, arg...)
#endif

#define BT_SLEEP_INF(fmt, arg...) printk(KERN_INFO "[XR_BT_LPM] %s: " fmt "\n", __func__, ## arg)
#define VERSION					"1.0.10"
#define PROC_SLEEP_DIR			"bluetooth/sleep"
#define PROC_POWER_DIR			"bluetooth/power"
#define BT_BLUEDROID_SUPPORT 	1

#define AW1732_BT				1
#define XRADIO_ETF_RFKILL		1

struct xr_btsleep_info {
	unsigned int wakeup_enable;
	unsigned int host_wake;
	unsigned int ext_wake;
	unsigned int host_wake_irq;
	struct wakeup_source *ws;
	struct uart_port *uport;
	unsigned int host_wake_polarity:1;
	unsigned int bt_wake_polarity:1;
	struct platform_device *pdev;
};

/* work function */
static void bluesleep_sleep_work(struct work_struct *work);


/* work queue */
DECLARE_DELAYED_WORK(sleep_workqueue, bluesleep_sleep_work);


/* Macros for handling sleep work */
#define bluesleep_rx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_rx_idle()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_idle()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_wake_up()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_standby()     schedule_delayed_work(&sleep_workqueue, 0)



/* 10 second timeout */
#define TX_TIMER_INTERVAL	2	//10s

/* state variable names and bit positions */
#define BT_PROTO	0x01
#define BT_TXDATA	0x02
#define BT_ASLEEP	0x04


/* variable use indicate lpm modle */
volatile bool has_lpm_enabled;

/* struct use save platform_device from uart */
static struct platform_device *bluesleep_uart_dev;
static struct xr_btsleep_info *bsi;

/* Global state flags */
static unsigned long flags;

#ifdef AW1732_BT
/* module usage */
static atomic_t open_count = ATOMIC_INIT(1);
/* Tasklet to respond to change in hostwake line */
static struct tasklet_struct hostwake_task;
#endif

/* Transmission timer */
static struct timer_list tx_timer;

/* Lock for state transitions */
static spinlock_t rw_lock;
static spinlock_t irq_lock;


struct proc_dir_entry *bluetooth_dir, *sleep_dir;

#ifdef XRADIO_ETF_RFKILL
struct proc_dir_entry *power_dir;
volatile char power_state_save;
#endif

/*
 * bt go to sleep will call this function tell uart stop data interactive
 */
static void hsuart_power(int on)
{
	if (bsi->uport == NULL) {
		return;
	}
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
	int flag1, flag2;
	flag1 = !test_bit(BT_TXDATA, &flags);
	flag2 = (bsi->uport != NULL);
	BT_SLEEP_DBG(" !BT_TXDATA is %d, bsi->uport is %d", flag1, flag2);
	return (flag1 && flag2);
}

/*
 * after bt wakeup should clean BT_ASLEEP flag and start time.
 */
int bluesleep_sleep_wakeup(void)
{
	int poll_count = 0;
	BT_SLEEP_DBG("test_bit(BT_ASLEEP, &flags) %d", test_bit(BT_ASLEEP, &flags));
	if (gpio_get_value(bsi->ext_wake) != bsi->bt_wake_polarity ||
		gpio_get_value(bsi->host_wake) != bsi->host_wake_polarity ||
		test_bit(BT_ASLEEP, &flags) == 1) {
		BT_SLEEP_DBG("waking up...");
		/*Activating UART */
		hsuart_power(1);
		gpio_set_value(bsi->ext_wake, bsi->bt_wake_polarity);
		//add poll bsi->hostwake
		while (poll_count++ < 6) {
			mdelay(25);
			if (gpio_get_value(bsi->host_wake) == 1) {
				BT_SLEEP_DBG("bt device active");
				/*wake up, maybe no data or cmd need send to fw*/
				/*so didn't change TXDATA in here*/
				clear_bit(BT_ASLEEP, &flags);
				return 0;
			}
		}
		/*
		*add once more wakeup operation when first operation failed,
		*because if host wakeup device during device is wakeuped by yourself,
		*device maybe couldn't receive wakeup GPIO rise-up signal
		*/
		poll_count = 0;
		while (poll_count++ < 6) {
			mdelay(25);
			if (gpio_get_value(bsi->host_wake) == 1) {
				BT_SLEEP_DBG("bt device active");
				/*wake up, maybe no data or cmd need send to fw*/
				/*so didn't change TXDATA in here*/
				clear_bit(BT_ASLEEP, &flags);
				return 0;
			}
		}
		return 1;
	} else {
		BT_SLEEP_DBG("bluesleep_sleep_wakeup del_timer");
		del_timer(&tx_timer);
		return 0;
	}
	return 1;
}

/**
 * @brief@  main sleep work handling function which update the flags
 * and activate and deactivate UART ,check FIFO.
 */
static void bluesleep_sleep_work(struct work_struct *work)
{
	if (!has_lpm_enabled) {
		BT_SLEEP_DBG("lpm disabled already");
		return;
	}

	if (bluesleep_can_sleep()) {
		/* already asleep, this is an error case */
		if (test_bit(BT_ASLEEP, &flags)) {
			BT_SLEEP_DBG("already asleep");
			return;
		}
		if (bsi->uport->ops->tx_empty(bsi->uport)) {
			BT_SLEEP_DBG("going to sleep...");
			gpio_set_value(bsi->ext_wake, !bsi->bt_wake_polarity);
			set_bit(BT_ASLEEP, &flags);
			clear_bit(BT_TXDATA, &flags);
			/*Deactivating UART */
			hsuart_power(0);
			__pm_relax(bsi->ws);
		}
	} else {
		bluesleep_sleep_wakeup();
	}
}

#ifdef AW1732_BT
static void bluesleep_hostwake_task(unsigned long data)
{
	BT_SLEEP_DBG("hostwake line change");
	if (!has_lpm_enabled) {
		return;
	}
	spin_lock(&rw_lock);
	//wakeup signal from fw
	if (test_bit(BT_ASLEEP, &flags) && !test_bit(BT_TXDATA, &flags)) {
		clear_bit(BT_ASLEEP, &flags);
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
		bluesleep_sleep_wakeup();
	}
	spin_unlock(&rw_lock);
}
#endif


static void bluesleep_outgoing_data(void)
{
	unsigned long irq_flags;
	BT_SLEEP_DBG();
	spin_lock_irqsave(&rw_lock, irq_flags);
	/* log data passing by */
	set_bit(BT_TXDATA, &flags);
	/* if the tx side is sleeping... */
	if (gpio_get_value(bsi->ext_wake) != bsi->bt_wake_polarity ||
			gpio_get_value(bsi->host_wake) != bsi->host_wake_polarity) {
		BT_SLEEP_DBG("tx was sleeping");
		if (bluesleep_sleep_wakeup() == 1) {
			BT_SLEEP_DBG("can't wakeup bt device!");
		}
	}
	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

static struct uart_port *bluesleep_get_uart_port(void)
{
	struct uart_port *uport = NULL;
	BT_INFO("%s enter.", __func__);
	if (bluesleep_uart_dev) {
		uport = platform_get_drvdata(bluesleep_uart_dev);
		BT_INFO("%s get uart_port from blusleep_uart_dev: %s",
					__func__, bluesleep_uart_dev->name);
	}
	return uport;
}

#ifdef AW1732_BT
static void bluesleep_tx_timer_expire(struct timer_list *t)
{
	unsigned long irq_flags;
	spin_lock_irqsave(&rw_lock, irq_flags);
	BT_SLEEP_DBG("Tx timer expired");
	/* were we silent during the last timeout */
	if (!test_bit(BT_ASLEEP, &flags) && !test_bit(BT_TXDATA, &flags)) {
		BT_SLEEP_DBG("Tx has been idle");
		bluesleep_tx_idle();
	} else {
		BT_SLEEP_DBG("Tx data during last period");
	}

	del_timer(&tx_timer);
	/* clear the incoming data flag */
	spin_unlock_irqrestore(&rw_lock, irq_flags);
}


static irqreturn_t bluesleep_hostwake_isr(int irq, void *dev_id)
{
	/* schedule a tasklet to handle the change in the host wake line */
	tasklet_schedule(&hostwake_task);
	__pm_stay_awake(bsi->ws);
	return IRQ_HANDLED;
}


static int bluesleep_start(void)
{
	int retval;
	unsigned long irq_flags;

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
	/*wake up bt device ,initial irq service*/
	gpio_set_value(bsi->ext_wake, bsi->bt_wake_polarity);
	retval = request_irq(bsi->host_wake_irq, bluesleep_hostwake_isr,
				bsi->host_wake_polarity ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING,
				"bluetooth hostwake", &bsi->pdev->dev);

	if (retval  < 0) {
		BT_ERR("Couldn't acquire BT_HOST_WAKE IRQ");
		goto fail;
	}

	set_bit(BT_PROTO, &flags);
	__pm_stay_awake(bsi->ws);

	return 0;


fail:
	atomic_inc(&open_count);
	return retval;
}

static void bluesleep_stop(void)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	if (!test_bit(BT_PROTO, &flags)) {
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return;
	}

	clear_bit(BT_PROTO, &flags);
	del_timer(&tx_timer);

	if (test_bit(BT_ASLEEP, &flags)) {
		clear_bit(BT_ASLEEP, &flags);
		hsuart_power(1);
	}

	if (test_bit(BT_TXDATA, &flags)) {
		clear_bit(BT_TXDATA, &flags);
	}

	atomic_inc(&open_count);
	spin_unlock_irqrestore(&rw_lock, irq_flags);
	free_irq(bsi->host_wake_irq, &bsi->pdev->dev);
	__pm_wakeup_event(bsi->ws, jiffies_to_msecs(HZ * 5));
}
#endif


#ifdef XRADIO_ETF_RFKILL
extern void sunxi_bluetooth_set_power(bool on_off);

static int bluetooth_pwr_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "power state: %d\n", power_state_save);
	return 0;
}

static int bluetooth_pwr_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bluetooth_pwr_proc_show, NULL);
}

static ssize_t bluetooth_write_proc_pwr(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	char b;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	if (b == '0') {
		BT_SLEEP_INF("bt power off");
		//sunxi_bluetooth_set_power(false);
		power_state_save = 0;
	} else {
		BT_SLEEP_INF("bt power on");
		//sunxi_bluetooth_set_power(true);
		power_state_save = 1;
	}

	return count;
}
#endif

static int bluedroid_lpm_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "lpm enable: %d\n", has_lpm_enabled);
	return 0;
}

static int bluedroid_lpm_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bluedroid_lpm_proc_show, NULL);
}

static ssize_t bluedroid_write_proc_lpm(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)

{
	char b;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

#ifdef AW1732_BT
	if (b == '0') {
		has_lpm_enabled = false;
		BT_SLEEP_DBG("disable lpm mode");
		bluesleep_stop();
		bsi->uport = NULL;
	} else {
		BT_SLEEP_DBG("enable lpm mode");
		if (!has_lpm_enabled) {
			has_lpm_enabled = true;
			if (bluesleep_uart_dev)
				bsi->uport = bluesleep_get_uart_port();
			bluesleep_start();
		}
	}
#else 	//work around for aw1722 wakeup bt before brom download
	if (b == '0') {			//disable lpm mode
		BT_SLEEP_DBG("disable lpm mode");
		has_lpm_enabled = false;
	} else {				//enable lpm mode
		BT_SLEEP_DBG("enable lpm mode");
		has_lpm_enabled = true;
		if (bluesleep_uart_dev)
			bsi->uport = bluesleep_get_uart_port();
	}
#endif

	return count;
}

static int bluedroid_btwrite_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "it's not support\n");
	return 0;
}

static int bluedroid_btwrite_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bluedroid_btwrite_proc_show, NULL);
}

static ssize_t bluedroid_write_proc_btwrite(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	char b;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	BT_SLEEP_DBG("bluedroid_write_proc_btwrite %c", b);
	if (b == '1') {
		mdelay(10);
		set_bit(BT_TXDATA, &flags);
		bluesleep_outgoing_data();
		//start the timer when host receive event but reply no data or cmd send to fw
		del_timer(&tx_timer);
	} else if (b == '0') {
		clear_bit(BT_TXDATA, &flags);
		bluesleep_tx_idle();
	}
	return count;
}

static int bluedroid_btwake_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "bt wake state:%d\n", gpio_get_value(bsi->ext_wake));
	return 0;
}

static int bluedroid_btwake_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bluedroid_btwake_proc_show, NULL);
}

static ssize_t bluedroid_write_proc_btwake(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	char *buf;	if (count < 1)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EFAULT;
	}
	BT_SLEEP_DBG("bluedroid_write_proc_btwake %c", buf[0]);
	if (buf[0] == '0') {
		BT_SLEEP_DBG("bt sleeping");
		gpio_set_value(bsi->ext_wake, !bsi->bt_wake_polarity);
	} else if (buf[0] == '1') {
		BT_SLEEP_DBG("wakeup bt device");
		gpio_set_value(bsi->ext_wake, bsi->bt_wake_polarity);
	} else {
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);
	return count;
}

#ifdef XRADIO_ETF_RFKILL
static const struct file_operations pwr_fops = {
	.owner		= THIS_MODULE,
	.open		= bluetooth_pwr_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= bluetooth_write_proc_pwr,
};
#endif

static const struct file_operations lpm_fops = {
	.owner		= THIS_MODULE,
	.open		= bluedroid_lpm_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= bluedroid_write_proc_lpm,
};
static const struct file_operations btwrite_fops = {
	.owner		= THIS_MODULE,
	.open		= bluedroid_btwrite_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= bluedroid_write_proc_btwrite,
};
static const struct file_operations btwake_fops = {
	.owner		= THIS_MODULE,
	.open		= bluedroid_btwake_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= bluedroid_write_proc_btwake,
};

static int assert_level = -1;
module_param(assert_level, int, S_IRUGO);
MODULE_PARM_DESC(assert_level, "BT_LPM hostwake/btwake assert level");

static struct platform_device *sw_uart_get_pdev(int id)
{
	struct device_node *np;
	char match[20];
	sprintf(match, "uart%d", id);
	np = of_find_node_by_type(NULL, match);
	return of_find_device_by_node(np);
}

static int bluesleep_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	enum of_gpio_flags config;
	int ret;
	u32 val;

	bsi = devm_kzalloc(&pdev->dev, sizeof(struct xr_btsleep_info), GFP_KERNEL);
	if (!bsi)
		return -ENOMEM;

	//get bt_wake & bt_host_wake from sys_config.fex
	bsi->ext_wake = of_get_named_gpio_flags(np, "bt_wake", 0, &config);
	if (!gpio_is_valid(bsi->ext_wake)) {
		BT_ERR("get gpio bt_wake failed");
		return -EINVAL;
	}

	bsi->bt_wake_polarity = (config == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	if (assert_level != -1) {
		bsi->bt_wake_polarity = (assert_level & 0x01) > 0;
		BT_SLEEP_DBG("override bt_wake polarity to %d", bsi->bt_wake_polarity);
	}

	BT_SLEEP_DBG("bt_wake polarity: %d", bsi->bt_wake_polarity);

	ret = devm_gpio_request(dev, bsi->ext_wake, "bt_wake");
	if (ret < 0) {
		BT_ERR("can't request bt_wake gpio %d",
			bsi->ext_wake);
		goto exit;
	}
	ret = gpio_direction_output(bsi->ext_wake, bsi->bt_wake_polarity);
	if (ret < 0) {
		BT_ERR("can't request output direction bt_wake gpio %d",
			bsi->ext_wake);
		goto exit;
	}

	bsi->host_wake = of_get_named_gpio_flags(np, "bt_hostwake", 0, &config);
	if (!gpio_is_valid(bsi->host_wake)) {
		BT_ERR("get gpio bt_hostwake failed");
		return -EINVAL;
	}

	bsi->host_wake_polarity = (config == OF_GPIO_ACTIVE_LOW) ? 0 : 1;
	if (assert_level != -1) {
		bsi->host_wake_polarity = (assert_level & 0x02) > 0;
		BT_SLEEP_DBG("override host_wake polarity to %d", bsi->host_wake_polarity);
	}

	BT_SLEEP_DBG("host_wake polarity: %d", bsi->host_wake_polarity);

	ret = devm_gpio_request(dev, bsi->host_wake, "bt_hostwake");
	if (ret < 0) {
		BT_ERR("can't request bt_hostwake gpio %d",
			bsi->host_wake);
		goto exit;
	}
	ret = gpio_direction_input(bsi->host_wake);
	if (ret < 0) {
		BT_ERR("can't request input direction bt_wake gpio %d",
			bsi->host_wake);
		goto exit;
	}
	if (!of_property_read_bool(np, "wakeup-source")) {
		BT_SLEEP_DBG("wakeup source is disabled!\n");
	} else {
		ret = device_init_wakeup(dev, true);
		if (ret < 0) {
			BT_ERR("device init wakeup failed!\n");
			return ret;
		}
		ret = dev_pm_set_wake_irq(dev, gpio_to_irq(bsi->host_wake));
		if (ret < 0) {
			BT_ERR("can't enable wakeup src for bt_hostwake %d\n",
				bsi->host_wake);
			return ret;
		}
		bsi->wakeup_enable = 1;
	}
	if (!of_property_read_u32(np, "uart_index", &val)) {
		BT_SLEEP_DBG("uart_index(%u)", val);
		bluesleep_uart_dev = sw_uart_get_pdev(val);
	}

	bsi->ws = wakeup_source_register(dev, "bluesleep");

	//1.set bt_wake as output and the level is 0
	gpio_set_value(bsi->ext_wake, 0);

	//2.get bt_host_wake gpio irq
#ifdef AW1732_BT
	bsi->host_wake_irq = gpio_to_irq(bsi->host_wake);
	if (bsi->host_wake_irq < 0) {
		BT_ERR("map gpio [%d] to virq failed, errno = %d\n", bsi->host_wake, bsi->host_wake_irq);
		ret = -ENODEV;
		goto free_bt_ext_wake;
	}
#endif
	bsi->pdev = pdev;

	return 0;

#ifdef AW1732_BT
free_bt_ext_wake:
	gpio_free(bsi->ext_wake);
	gpio_free(bsi->host_wake);
#endif

exit:
	return ret;
}

static int bluesleep_remove(struct platform_device *pdev)
{
	BT_SLEEP_DBG();
	/* assert bt wake */
	gpio_set_value(bsi->ext_wake, bsi->bt_wake_polarity);
	if (test_bit(BT_PROTO, &flags)) {
#ifdef AW1732_BT
		free_irq(bsi->host_wake_irq, &bsi->pdev->dev);
#endif
		del_timer(&tx_timer);
		if (test_bit(BT_ASLEEP, &flags))
			hsuart_power(1);
	}
	gpio_free(bsi->host_wake);
	gpio_free(bsi->ext_wake);
	wakeup_source_unregister(bsi->ws);
	if (bsi->wakeup_enable) {
		BT_SLEEP_DBG("Deinit wakeup source");
		device_init_wakeup(&pdev->dev, false);
		dev_pm_clear_wake_irq(&pdev->dev);
	}

	return 0;
}

static int bluesleep_resume(struct platform_device *pdev)
{
	BT_SLEEP_DBG();
	if (!has_lpm_enabled) {
		return 0;
	}
#if 0   //fix bug for always wakeup
	if (test_bit(BT_ASLEEP, &flags)) {
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));
	}
	bluesleep_sleep_wakeup();
#endif
	return 0;
}

static int bluesleep_suspend(struct platform_device *pdev, pm_message_t state)
{
	BT_SLEEP_DBG();
	if (!has_lpm_enabled) {
		return 0;
	}
	/*TXing DATA,if suspend will cause a error*/
	if (test_bit(BT_TXDATA, &flags)) {
		return -EBUSY;
	}
	del_timer(&tx_timer);
	bluesleep_standby();
	return 0;
}


static const struct of_device_id sunxi_btlpm_ids[] = {
	{ .compatible = "allwinner,sunxi-btlpm" },
	{ /* Sentinel */ }
};

static struct platform_driver bluesleep_driver = {
	.probe = bluesleep_probe,
	.remove = bluesleep_remove,
	.suspend = bluesleep_suspend,
	.resume = bluesleep_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = "sunxi-btlpm",
		.of_match_table	= sunxi_btlpm_ids,
	},
};

static int __init btsleep_init(void)
{
	int retval;
	struct proc_dir_entry *ent;

	BT_INFO("XRadio Bluetooth LPM Mode Driver Ver %s", VERSION);

	retval = platform_driver_probe(&bluesleep_driver, bluesleep_probe);
	if (retval)
		goto fail;

	bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (bluetooth_dir == NULL) {
		BT_ERR("Unable to create /proc/bluetooth directory");
		goto unreg_drv;
	}
	sleep_dir = proc_mkdir("sleep", bluetooth_dir);
	if (sleep_dir == NULL) {
		BT_ERR("Unable to create /proc/%s directory", PROC_SLEEP_DIR);
		goto rm_bt_dir;
	}

#ifdef XRADIO_ETF_RFKILL
	power_dir = proc_mkdir("power", bluetooth_dir);
	if (power_dir == NULL) {
		BT_ERR("Unable to create /proc/%s directory", PROC_POWER_DIR);
		goto rm_bt_dir;
	}
	/* read/write proc entries */
	ent = proc_create("state", 0220, power_dir, &pwr_fops);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/state entry", PROC_POWER_DIR);
		retval = -ENOMEM;
		goto rm_power_dir;
	}
#endif

	/* read/write proc entries */
	ent = proc_create("lpm", 0220, sleep_dir, &lpm_fops);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/lpm entry", PROC_SLEEP_DIR);
		retval = -ENOMEM;
		goto rm_sleep_dir;
	}
	ent = proc_create("btwrite", 0220, sleep_dir, &btwrite_fops);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/btwrite entry", PROC_SLEEP_DIR);
		retval = -ENOMEM;
		goto rm_btwrite_dir;
	}
	ent = proc_create("btwake", 0220, sleep_dir, &btwake_fops);
	if (ent == NULL) {
		BT_ERR("Unable to create /proc/%s/btwrite entry", PROC_SLEEP_DIR);
		retval = -ENOMEM;
		goto rm_btwake_dir;
	}

#ifdef AW1732_BT
	flags = 0;
	/* Initialize spinlock. */
	spin_lock_init(&rw_lock);
	spin_lock_init(&irq_lock);
	/* Initialize timer */
	timer_setup(&tx_timer, bluesleep_tx_timer_expire, 0);
	/* initialize host wake tasklet */
	tasklet_init(&hostwake_task, bluesleep_hostwake_task, 0);
#endif

	return 0;

rm_btwake_dir:
	remove_proc_entry("btwake", sleep_dir);
rm_btwrite_dir:
	remove_proc_entry("btwrite", sleep_dir);
rm_sleep_dir:
	remove_proc_entry("sleep", bluetooth_dir);
#ifdef XRADIO_ETF_RFKILL
rm_power_dir:
	remove_proc_entry("state", power_dir);
	remove_proc_entry("power", bluetooth_dir);
#endif
rm_bt_dir:
	remove_proc_entry("bluetooth", 0);
unreg_drv:
	platform_driver_unregister(&bluesleep_driver);
fail:
	return retval;
}

static void __exit btsleep_exit(void)
{
	platform_driver_unregister(&bluesleep_driver);

	remove_proc_entry("btwake", sleep_dir);
	remove_proc_entry("btwrite", sleep_dir);
	remove_proc_entry("lpm", sleep_dir);
	remove_proc_entry("sleep", bluetooth_dir);
#ifdef XRADIO_ETF_RFKILL
	remove_proc_entry("state", power_dir);
	remove_proc_entry("power", bluetooth_dir);
#endif
	remove_proc_entry("bluetooth", 0);
}

module_init(btsleep_init);
module_exit(btsleep_exit);
MODULE_AUTHOR("XRadioTech");
MODULE_DESCRIPTION("Bluetooth Sleep Mode Driver ver. %s" VERSION);
MODULE_LICENSE("GPL");

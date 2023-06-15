/**
 * aicwf_sdmmc.c
 *
 * SDIO function declarations
 *
 * Copyright (C) AICSemi 2018-2020
 */
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/semaphore.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include "aicsdio_txrxif.h"
#include "aicsdio.h"
#include "aic_bsp_driver.h"
#include <linux/version.h>

#ifdef CONFIG_PLATFORM_ALLWINNER
extern void sunxi_mmc_rescan_card(unsigned ids);
extern void sunxi_wlan_set_power(int on);
extern int sunxi_wlan_get_bus_index(void);
static int aicbsp_bus_index = -1;
static int aicbsp_platform_power_on(void);
static void aicbsp_platform_power_off(void);
#endif

static struct aic_sdio_dev *aicbsp_sdiodev;
static struct semaphore *aicbsp_notify_semaphore;
static const struct sdio_device_id aicbsp_sdmmc_ids[];

static int aicbsp_dummy_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
	if (func && (func->num != 2))
		return 0;

	if (aicbsp_notify_semaphore)
		up(aicbsp_notify_semaphore);
	return 0;
}

static void aicbsp_dummy_remove(struct sdio_func *func)
{
}

static struct sdio_driver aicbsp_dummy_sdmmc_driver = {
	.probe		= aicbsp_dummy_probe,
	.remove		= aicbsp_dummy_remove,
	.name		= "aicbsp_dummy_sdmmc",
	.id_table	= aicbsp_sdmmc_ids,
};

static int aicbsp_reg_sdio_notify(void *semaphore)
{
	aicbsp_notify_semaphore = semaphore;
	return sdio_register_driver(&aicbsp_dummy_sdmmc_driver);
}

static void aicbsp_unreg_sdio_notify(void)
{
	mdelay(15);
	sdio_unregister_driver(&aicbsp_dummy_sdmmc_driver);
}

static const char *aicbsp_subsys_name(int subsys)
{
	switch (subsys) {
	case AIC_BLUETOOTH:
		return "AIC_BLUETOOTH";
	case AIC_WIFI:
		return "AIC_WIFI";
	default:
		return "unknown subsys";
	}
}

int aicbsp_set_subsys(int subsys, int state)
{
	static int pre_power_map;
	int cur_power_map;
	int pre_power_state;
	int cur_power_state;

	mutex_lock(&aicbsp_power_lock);
	cur_power_map = pre_power_map;
	if (state)
		cur_power_map |= (1 << subsys);
	else
		cur_power_map &= ~(1 << subsys);

	pre_power_state = pre_power_map > 0;
	cur_power_state = cur_power_map > 0;

	sdio_dbg("%s, subsys: %s, state to: %d\n", __func__, aicbsp_subsys_name(subsys), state);

	if (cur_power_state != pre_power_state) {
		sdio_dbg("%s, power state change to %d dure to %s\n", __func__, cur_power_state, aicbsp_subsys_name(subsys));
		if (cur_power_state) {
			if (aicbsp_platform_power_on() < 0)
				goto err0;
			if (aicbsp_sdio_init())
				goto err1;
			if (!aicbsp_sdiodev)
				goto err2;
			if (aicbsp_driver_fw_init(aicbsp_sdiodev))
				goto err3;
			aicbsp_sdio_release(aicbsp_sdiodev);
		} else {
			aicbsp_sdio_exit();
			aicbsp_platform_power_off();
		}
	} else {
		sdio_dbg("%s, power state no need to change, current: %d\n", __func__, cur_power_state);
	}
	pre_power_map = cur_power_map;
	mutex_unlock(&aicbsp_power_lock);

	return cur_power_state;

err3:
	aicbsp_sdio_release(aicbsp_sdiodev);

err2:
	aicbsp_sdio_exit();

err1:
	aicbsp_platform_power_off();

err0:
	sdio_dbg("%s, fail to set %s power state to %d\n", __func__, aicbsp_subsys_name(subsys), state);
	mutex_unlock(&aicbsp_power_lock);
	return -1;
}
EXPORT_SYMBOL_GPL(aicbsp_set_subsys);

void *aicbsp_get_drvdata(void *args)
{
	(void)args;
	if (aicbsp_sdiodev)
		return aicbsp_sdiodev->bus_if;
	return NULL;
}

static int aicbsp_sdio_probe(struct sdio_func *func,
	const struct sdio_device_id *id)
{
	struct mmc_host *host;
	struct aic_sdio_dev *sdiodev;
	struct aicwf_bus *bus_if;
	int err = -ENODEV;

	sdio_dbg("%s:%d\n", __func__, func->num);
	if (func->num != 2) {
		return err;
	}

	func = func->card->sdio_func[1 - 1]; //replace 2 with 1
	host = func->card->host;
	sdio_dbg("%s after replace:%d\n", __func__, func->num);

	bus_if = kzalloc(sizeof(struct aicwf_bus), GFP_KERNEL);
	if (!bus_if) {
		sdio_err("alloc bus fail\n");
		return -ENOMEM;
	}

	sdiodev = kzalloc(sizeof(struct aic_sdio_dev), GFP_KERNEL);
	if (!sdiodev) {
		sdio_err("alloc sdiodev fail\n");
		kfree(bus_if);
		return -ENOMEM;
	}

	sdiodev->func = func;
	sdiodev->bus_if = bus_if;
	bus_if->bus_priv.sdio = sdiodev;
	dev_set_drvdata(&func->dev, bus_if);
	sdiodev->dev = &func->dev;
	err = aicwf_sdio_func_init(sdiodev);
	if (err < 0) {
		sdio_err("sdio func init fail\n");
		goto fail;
	}

	if (aicwf_sdio_bus_init(sdiodev) == NULL) {
		sdio_err("sdio bus init err\r\n");
		err = -1;
		goto fail;
	}
	host->caps |= MMC_CAP_NONREMOVABLE;
	aicbsp_platform_init(sdiodev);
	aicbsp_sdiodev = sdiodev;

	return 0;
fail:
	aicwf_sdio_func_deinit(sdiodev);
	dev_set_drvdata(&func->dev, NULL);
	kfree(sdiodev);
	kfree(bus_if);
	return err;
}

static void aicbsp_sdio_remove(struct sdio_func *func)
{
	struct mmc_host *host;
	struct aicwf_bus *bus_if = NULL;
	struct aic_sdio_dev *sdiodev = NULL;

	sdio_dbg("%s\n", __func__);
	bus_if = aicbsp_get_drvdata(&func->dev);
	if (!bus_if) {
		sdio_dbg("%s: allready unregister\n", __func__);
		goto done;
	}

	sdiodev = bus_if->bus_priv.sdio;
	if (!sdiodev) {
		goto done;
	}

	func = sdiodev->func;
	host = func->card->host;
	host->caps &= ~MMC_CAP_NONREMOVABLE;

	aicwf_sdio_release(sdiodev);
	aicwf_sdio_func_deinit(sdiodev);

	dev_set_drvdata(&sdiodev->func->dev, NULL);
	kfree(sdiodev);
	kfree(bus_if);

done:
	aicbsp_sdiodev = NULL;
	sdio_dbg("%s done\n", __func__);
}

static int aicbsp_sdio_suspend(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	int err;
	mmc_pm_flag_t sdio_flags;
	sdio_dbg("%s, func->num = %d\n", __func__, func->num);
	if (func->num != 2)
		return 0;

	sdio_flags = sdio_get_host_pm_caps(func);
	if (!(sdio_flags & MMC_PM_KEEP_POWER)) {
		sdio_dbg("%s: can't keep power while host is suspended\n", __func__);
		return  -EINVAL;
	}

	/* keep power while host suspended */
	err = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
	if (err) {
		sdio_dbg("%s: error while trying to keep power\n", __func__);
		return err;
	}

	return 0;
}

static int aicbsp_sdio_resume(struct device *dev)
{
	sdio_dbg("%s\n", __func__);
	return 0;
}

static const struct sdio_device_id aicbsp_sdmmc_ids[] = {
	{SDIO_DEVICE_CLASS(SDIO_CLASS_WLAN)},
	{ },
};

MODULE_DEVICE_TABLE(sdio, aicbsp_sdmmc_ids);

static const struct dev_pm_ops aicbsp_sdio_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(aicbsp_sdio_suspend, aicbsp_sdio_resume)
};

static struct sdio_driver aicbsp_sdio_driver = {
	.probe = aicbsp_sdio_probe,
	.remove = aicbsp_sdio_remove,
	.name = AICBSP_SDIO_NAME,
	.id_table = aicbsp_sdmmc_ids,
	.drv = {
		.pm = &aicbsp_sdio_pm_ops,
	},
};

#ifdef CONFIG_PLATFORM_ALLWINNER
static int aicbsp_platform_power_on(void)
{
	int ret = 0;
	struct semaphore aic_chipup_sem;
	sdio_dbg("%s\n", __func__);
	if (aicbsp_bus_index < 0)
		 aicbsp_bus_index = sunxi_wlan_get_bus_index();
	if (aicbsp_bus_index < 0)
		return aicbsp_bus_index;

	sema_init(&aic_chipup_sem, 0);
	ret = aicbsp_reg_sdio_notify(&aic_chipup_sem);
	if (ret) {
		sdio_dbg("%s aicbsp_reg_sdio_notify fail(%d)\n", __func__, ret);
			return ret;
	}
	sunxi_wlan_set_power(1);
	mdelay(50);
	sunxi_mmc_rescan_card(aicbsp_bus_index);

	if (down_timeout(&aic_chipup_sem, msecs_to_jiffies(2000)) == 0) {
		aicbsp_unreg_sdio_notify();
		return 0;
	}

	aicbsp_unreg_sdio_notify();
	sunxi_wlan_set_power(0);
	return -1;
}

static void aicbsp_platform_power_off(void)
{
	if (aicbsp_bus_index < 0)
		 aicbsp_bus_index = sunxi_wlan_get_bus_index();
	if (aicbsp_bus_index < 0) {
		sdio_dbg("no aicbsp_bus_index\n");
		return;
	}
	sunxi_wlan_set_power(0);
	mdelay(100);
	sunxi_mmc_rescan_card(aicbsp_bus_index);

	sdio_dbg("%s\n", __func__);
}
#endif

int aicbsp_sdio_init(void)
{
	if (sdio_register_driver(&aicbsp_sdio_driver))
		return -1;

	return 0;
}

void aicbsp_sdio_exit(void)
{
	sdio_unregister_driver(&aicbsp_sdio_driver);
}

void aicbsp_sdio_release(struct aic_sdio_dev *sdiodev)
{
	sdiodev->bus_if->state = BUS_DOWN_ST;
	sdio_claim_host(sdiodev->func);
	sdio_release_irq(sdiodev->func);
	sdio_release_host(sdiodev->func);
}

int aicwf_sdio_readb(struct aic_sdio_dev *sdiodev, uint regaddr, u8 *val)
{
	int ret;
	sdio_claim_host(sdiodev->func);
	*val = sdio_readb(sdiodev->func, regaddr, &ret);
	sdio_release_host(sdiodev->func);
	return ret;
}

int aicwf_sdio_writeb(struct aic_sdio_dev *sdiodev, uint regaddr, u8 val)
{
	int ret;
	sdio_claim_host(sdiodev->func);
	sdio_writeb(sdiodev->func, val, regaddr, &ret);
	sdio_release_host(sdiodev->func);
	return ret;
}

int aicwf_sdio_flow_ctrl(struct aic_sdio_dev *sdiodev)
{
	int ret = -1;
	u8 fc_reg = 0;
	u32 count = 0;

	while (true) {
		ret = aicwf_sdio_readb(sdiodev, SDIOWIFI_FLOW_CTRL_REG, &fc_reg);
		if (ret) {
			return -1;
		}

		if ((fc_reg & SDIOWIFI_FLOWCTRL_MASK_REG) != 0) {
			ret = fc_reg & SDIOWIFI_FLOWCTRL_MASK_REG;
			return ret;
		} else {
			if (count >= FLOW_CTRL_RETRY_COUNT) {
				ret = -fc_reg;
				break;
			}
			count++;
			if (count < 30)
				udelay(200);
			else if (count < 40)
				mdelay(1);
			else
				mdelay(10);
		}
	}

	return ret;
}

int aicwf_sdio_send_pkt(struct aic_sdio_dev *sdiodev, u8 *buf, uint count)
{
	int ret = 0;

	sdio_claim_host(sdiodev->func);
	ret = sdio_writesb(sdiodev->func, 7, buf, count);
	sdio_release_host(sdiodev->func);

	return ret;
}

int aicwf_sdio_recv_pkt(struct aic_sdio_dev *sdiodev, struct sk_buff *skbbuf,
	u32 size)
{
	int ret;

	if ((!skbbuf) || (!size)) {
		return -EINVAL;;
	}

	sdio_claim_host(sdiodev->func);
	ret = sdio_readsb(sdiodev->func, skbbuf->data, 8, size);
	sdio_release_host(sdiodev->func);

	if (ret < 0) {
		return ret;
	}
	skbbuf->len = size;

	return ret;
}

int aicwf_sdio_wakeup(struct aic_sdio_dev *sdiodev)
{
	int ret = 0;
		int read_retry;
		int write_retry = 20;

	if (sdiodev->state == SDIO_SLEEP_ST) {
		//if (sdiodev->rwnx_hw->vif_started) {
			down(&sdiodev->pwrctl_wakeup_sema);
			while (write_retry) {
				ret = aicwf_sdio_writeb(sdiodev, SDIOWIFI_WAKEUP_REG, 1);
				if (ret) {
					txrx_err("sdio wakeup fail\n");
					ret = -1;
				} else {
					read_retry = 10;
					while (read_retry) {
						u8 val;
						ret = aicwf_sdio_readb(sdiodev, SDIOWIFI_SLEEP_REG, &val);
						if ((ret == 0) && (val & 0x10)) {
							break;
						}
						read_retry--;
						udelay(200);
					}
					if (read_retry != 0)
						break;
				}
				sdio_dbg("write retry:  %d \n", write_retry);
				write_retry--;
				udelay(100);
			}
			up(&sdiodev->pwrctl_wakeup_sema);
	   // }
	}

	sdiodev->state = SDIO_ACTIVE_ST;
	aicwf_sdio_pwrctl_timer(sdiodev, sdiodev->active_duration);

	return ret;
}

extern u8 dhcped;
int aicwf_sdio_sleep_allow(struct aic_sdio_dev *sdiodev)
{
	int ret = 0;
	struct aicwf_bus *bus_if = sdiodev->bus_if;

	if (bus_if->state == BUS_DOWN_ST) {
		ret = aicwf_sdio_writeb(sdiodev, SDIOWIFI_SLEEP_REG, 0x10);
		if (ret) {
			sdio_err("Write sleep fail!\n");
	}
		aicwf_sdio_pwrctl_timer(sdiodev, 0);
		return ret;
	}

	if (sdiodev->state == SDIO_ACTIVE_ST) {
		{
			sdio_dbg("s\n");
			ret = aicwf_sdio_writeb(sdiodev, SDIOWIFI_SLEEP_REG, 0x10);
			if (ret)
				sdio_err("Write sleep fail!\n");
		}
		sdiodev->state = SDIO_SLEEP_ST;
		aicwf_sdio_pwrctl_timer(sdiodev, 0);
	} else {
		aicwf_sdio_pwrctl_timer(sdiodev, sdiodev->active_duration);
	}

	return ret;
}

int aicwf_sdio_pwr_stctl(struct aic_sdio_dev *sdiodev, uint target)
{
	int ret = 0;

	if (sdiodev->bus_if->state == BUS_DOWN_ST) {
		return -1;
	}

	if (sdiodev->state == target) {
		if (target == SDIO_ACTIVE_ST) {
			aicwf_sdio_pwrctl_timer(sdiodev, sdiodev->active_duration);
		}
		return ret;
	}

	switch (target) {
	case SDIO_ACTIVE_ST:
		aicwf_sdio_wakeup(sdiodev);
		break;
	case SDIO_SLEEP_ST:
		aicwf_sdio_sleep_allow(sdiodev);
		break;
	}

	return ret;
}

int aicwf_sdio_txpkt(struct aic_sdio_dev *sdiodev, struct sk_buff *pkt)
{
	int ret = 0;
	u8 *frame;
	u32 len = 0;
	struct aicwf_bus *bus_if = dev_get_drvdata(sdiodev->dev);

	if (bus_if->state == BUS_DOWN_ST) {
		sdio_dbg("tx bus is down!\n");
		return -EINVAL;
	}

	frame = (u8 *) (pkt->data);
	len = pkt->len;
	len = (len + SDIOWIFI_FUNC_BLOCKSIZE - 1) / SDIOWIFI_FUNC_BLOCKSIZE * SDIOWIFI_FUNC_BLOCKSIZE;
	ret = aicwf_sdio_send_pkt(sdiodev, pkt->data, len);
	if (ret)
		sdio_err("aicwf_sdio_send_pkt fail%d\n", ret);

	return ret;
}

static int aicwf_sdio_intr_get_len_bytemode(struct aic_sdio_dev *sdiodev, u8 *byte_len)
{
	int ret = 0;

	if (!byte_len)
		return -EBADE;

	if (sdiodev->bus_if->state == BUS_DOWN_ST) {
		*byte_len = 0;
	} else {
		ret = aicwf_sdio_readb(sdiodev, SDIOWIFI_BYTEMODE_LEN_REG, byte_len);
		sdiodev->rx_priv->data_len = (*byte_len)*4;
	}

	return ret;
}

static void aicwf_sdio_bus_stop(struct device *dev)
{
	struct aicwf_bus *bus_if = aicbsp_get_drvdata(dev);
	struct aic_sdio_dev *sdiodev = bus_if->bus_priv.sdio;
	int ret = 0;

	aicwf_sdio_pwrctl_timer(sdiodev, 0);
	sdio_dbg("%s\n", __func__);
	if (sdiodev->pwrctl_tsk) {
		complete(&sdiodev->pwrctrl_trgg);
		kthread_stop(sdiodev->pwrctl_tsk);
		sdiodev->pwrctl_tsk = NULL;
	}

	bus_if->state = BUS_DOWN_ST;
	if (sdiodev->tx_priv) {
		ret = down_interruptible(&sdiodev->tx_priv->txctl_sema);
		if (ret)
			sdio_err("down txctl_sema fail\n");
	}

	aicwf_sdio_pwr_stctl(sdiodev, SDIO_SLEEP_ST);
	if (sdiodev->tx_priv) {
		if (!ret)
			up(&sdiodev->tx_priv->txctl_sema);
		aicwf_frame_queue_flush(&sdiodev->tx_priv->txq);
	}
}

struct sk_buff *aicwf_sdio_readframes(struct aic_sdio_dev *sdiodev)
{
	int ret = 0;
	u32 size = 0;
	struct sk_buff *skb = NULL;
	struct aicwf_bus *bus_if = dev_get_drvdata(sdiodev->dev);

	if (bus_if->state == BUS_DOWN_ST) {
		sdio_dbg("bus down\n");
		return NULL;
	}

	size = sdiodev->rx_priv->data_len;
	skb =  __dev_alloc_skb(size, GFP_KERNEL);
	if (!skb) {
		return NULL;
	}

	ret = aicwf_sdio_recv_pkt(sdiodev, skb, size);
	if (ret) {
		dev_kfree_skb(skb);
		skb = NULL;
	}

	return skb;
}

static int aicwf_sdio_tx_msg(struct aic_sdio_dev *sdiodev)
{
	int err = 0;
	u16 len;
	u8 *payload = sdiodev->tx_priv->cmd_buf;
	u16 payload_len = sdiodev->tx_priv->cmd_len;
	u8 adjust_str[4] = {0, 0, 0, 0};
	int adjust_len = 0;
	int buffer_cnt = 0;
	u8 retry = 0;

	len = payload_len;
	if ((len % TX_ALIGNMENT) != 0) {
		adjust_len = roundup(len, TX_ALIGNMENT);
		memcpy(payload+payload_len, adjust_str, (adjust_len - len));
		payload_len += (adjust_len - len);
	}
	len = payload_len;

	//link tail is necessary
	if ((len % SDIOWIFI_FUNC_BLOCKSIZE) != 0) {
		memset(payload+payload_len, 0, TAIL_LEN);
		payload_len += TAIL_LEN;
		len = (payload_len/SDIOWIFI_FUNC_BLOCKSIZE + 1) * SDIOWIFI_FUNC_BLOCKSIZE;
	} else
		len = payload_len;

	buffer_cnt = aicwf_sdio_flow_ctrl(sdiodev);
	while ((buffer_cnt <= 0 || len > (buffer_cnt * BUFFER_SIZE)) && retry < 5) {
		retry++;
		buffer_cnt = aicwf_sdio_flow_ctrl(sdiodev);
	}

	down(&sdiodev->tx_priv->cmd_txsema);
	if (buffer_cnt > 0 && len < (buffer_cnt * BUFFER_SIZE)) {
		err = aicwf_sdio_send_pkt(sdiodev, payload, len);
		if (err) {
			sdio_err("aicwf_sdio_send_pkt fail%d\n", err);
		}
	} else {
		up(&sdiodev->tx_priv->cmd_txsema);
		sdio_err("tx msg fc retry fail\n");
		return -1;
	}

	sdiodev->tx_priv->cmd_txstate = false;
	if (!err)
		sdiodev->tx_priv->cmd_tx_succ = true;
	else
		sdiodev->tx_priv->cmd_tx_succ = false;

	up(&sdiodev->tx_priv->cmd_txsema);

	return err;
}

static void aicwf_sdio_tx_process(struct aic_sdio_dev *sdiodev)
{
	int err = 0;

	if (sdiodev->bus_if->state == BUS_DOWN_ST) {
		sdio_err("Bus is down\n");
		return;
	}

	aicwf_sdio_pwr_stctl(sdiodev, SDIO_ACTIVE_ST);

	//config
	sdio_info("send cmd\n");
	if (sdiodev->tx_priv->cmd_txstate) {
		if (down_interruptible(&sdiodev->tx_priv->txctl_sema)) {
			txrx_err("txctl down bus->txctl_sema fail\n");
			return;
		}
		if (sdiodev->state != SDIO_ACTIVE_ST) {
			txrx_err("state err\n");
			up(&sdiodev->tx_priv->txctl_sema);
			txrx_err("txctl up bus->txctl_sema fail\n");
			return;
		}

		err = aicwf_sdio_tx_msg(sdiodev);
		up(&sdiodev->tx_priv->txctl_sema);
		if (waitqueue_active(&sdiodev->tx_priv->cmd_txdone_wait))
			wake_up(&sdiodev->tx_priv->cmd_txdone_wait);
	}

	//data
	sdio_info("send data\n");
	if (down_interruptible(&sdiodev->tx_priv->txctl_sema)) {
		txrx_err("txdata down bus->txctl_sema\n");
		return;
	}

	if (sdiodev->state != SDIO_ACTIVE_ST) {
		txrx_err("sdio state err\n");
		up(&sdiodev->tx_priv->txctl_sema);
		return;
	}

	sdiodev->tx_priv->fw_avail_bufcnt = aicwf_sdio_flow_ctrl(sdiodev);
	while (!aicwf_is_framequeue_empty(&sdiodev->tx_priv->txq)) {
		aicwf_sdio_send(sdiodev->tx_priv);
		if (sdiodev->tx_priv->cmd_txstate)
			break;
	}

	up(&sdiodev->tx_priv->txctl_sema);
}

static int aicwf_sdio_bus_txdata(struct device *dev, struct sk_buff *pkt)
{
	uint prio;
	int ret = -EBADE;
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_sdio_dev *sdiodev = bus_if->bus_priv.sdio;

	prio = (pkt->priority & 0x7);
	spin_lock_bh(&sdiodev->tx_priv->txqlock);
	if (!aicwf_frame_enq(sdiodev->dev, &sdiodev->tx_priv->txq, pkt, prio)) {
		spin_unlock_bh(&sdiodev->tx_priv->txqlock);
		return -ENOSR;
	} else {
		ret = 0;
	}

	if (bus_if->state != BUS_UP_ST) {
		sdio_err("bus_if stopped\n");
		spin_unlock_bh(&sdiodev->tx_priv->txqlock);
		return -1;
	}

	atomic_inc(&sdiodev->tx_priv->tx_pktcnt);
	spin_unlock_bh(&sdiodev->tx_priv->txqlock);
	complete(&bus_if->bustx_trgg);

	return ret;
}

static int aicwf_sdio_bus_txmsg(struct device *dev, u8 *msg, uint msglen)
{
	int ret = -1;
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_sdio_dev *sdiodev = bus_if->bus_priv.sdio;

	down(&sdiodev->tx_priv->cmd_txsema);
	sdiodev->tx_priv->cmd_txstate = true;
	sdiodev->tx_priv->cmd_tx_succ = false;
	sdiodev->tx_priv->cmd_buf = msg;
	sdiodev->tx_priv->cmd_len = msglen;
	up(&sdiodev->tx_priv->cmd_txsema);

	if (bus_if->state != BUS_UP_ST) {
		sdio_err("bus has stop\n");
		return -1;
	}

	complete(&bus_if->bustx_trgg);

	if (sdiodev->tx_priv->cmd_txstate) {
		int timeout = msecs_to_jiffies(CMD_TX_TIMEOUT);
		ret = wait_event_interruptible_timeout(sdiodev->tx_priv->cmd_txdone_wait, \
											!(sdiodev->tx_priv->cmd_txstate), timeout);
	}

	if (!sdiodev->tx_priv->cmd_txstate && sdiodev->tx_priv->cmd_tx_succ) {
		ret = 0;
	} else {
		sdio_err("send faild:%d, %d,%x\n", sdiodev->tx_priv->cmd_txstate, sdiodev->tx_priv->cmd_tx_succ, ret);
		ret = -EIO;
	}

	return ret;
}

int aicwf_sdio_send(struct aicwf_tx_priv *tx_priv)
{
	struct sk_buff *pkt;
	struct aic_sdio_dev *sdiodev = tx_priv->sdiodev;
	u16 aggr_len = 0;
	int retry_times = 0;
	int max_retry_times = 5;

	aggr_len = (tx_priv->tail - tx_priv->head);
	if (((atomic_read(&tx_priv->aggr_count) == 0) && (aggr_len != 0))
		|| ((atomic_read(&tx_priv->aggr_count) != 0) && (aggr_len == 0))) {
		if (aggr_len > 0)
			aicwf_sdio_aggrbuf_reset(tx_priv);
		goto done;
	}

	if (tx_priv->fw_avail_bufcnt <= 0) { //flow control failed
		tx_priv->fw_avail_bufcnt = aicwf_sdio_flow_ctrl(sdiodev);
		while (tx_priv->fw_avail_bufcnt <= 0 && retry_times < max_retry_times) {
			retry_times++;
			tx_priv->fw_avail_bufcnt = aicwf_sdio_flow_ctrl(sdiodev);
		}
		if (tx_priv->fw_avail_bufcnt <= 0) {
			sdio_err("fc retry %d fail\n", tx_priv->fw_avail_bufcnt);
			goto done;
		}
	}

	if (atomic_read(&tx_priv->aggr_count) == tx_priv->fw_avail_bufcnt) {
		if (atomic_read(&tx_priv->aggr_count) > 0) {
			tx_priv->fw_avail_bufcnt -= atomic_read(&tx_priv->aggr_count);
			aicwf_sdio_aggr_send(tx_priv); //send and check the next pkt;
		}
	} else {
		spin_lock_bh(&sdiodev->tx_priv->txqlock);
		pkt = aicwf_frame_dequeue(&sdiodev->tx_priv->txq);
		if (pkt == NULL) {
			sdio_err("txq no pkt\n");
			spin_unlock_bh(&sdiodev->tx_priv->txqlock);
			goto done;
		}
		atomic_dec(&sdiodev->tx_priv->tx_pktcnt);
		spin_unlock_bh(&sdiodev->tx_priv->txqlock);

		if (tx_priv == NULL || tx_priv->tail == NULL || pkt == NULL)
			txrx_err("null error\n");
		if (aicwf_sdio_aggr(tx_priv, pkt)) {
			aicwf_sdio_aggrbuf_reset(tx_priv);
			sdio_err("add aggr pkts failed!\n");
			goto done;
		}

		//when aggr finish or there is cmd to send, just send this aggr pkt to fw
		if ((int)atomic_read(&sdiodev->tx_priv->tx_pktcnt) == 0 || sdiodev->tx_priv->cmd_txstate) { //no more pkt send it!
			tx_priv->fw_avail_bufcnt -= atomic_read(&tx_priv->aggr_count);
			aicwf_sdio_aggr_send(tx_priv);
		} else
			goto done;
	}

done:
	return 0;
}

int aicwf_sdio_aggr(struct aicwf_tx_priv *tx_priv, struct sk_buff *pkt)
{
	//struct rwnx_txhdr *txhdr = (struct rwnx_txhdr *)pkt->data;
	u8 *start_ptr = tx_priv->tail;
	u8 sdio_header[4];
	u8 adjust_str[4] = {0, 0, 0, 0};
	u16 curr_len = 0;
	int allign_len = 0;

	//sdio_header[0] =((pkt->len - sizeof(struct rwnx_txhdr) + sizeof(struct txdesc_api)) & 0xff);
	//sdio_header[1] =(((pkt->len - sizeof(struct rwnx_txhdr) + sizeof(struct txdesc_api)) >> 8)&0x0f);
	sdio_header[2] = 0x01; //data
	sdio_header[3] = 0; //reserved

	memcpy(tx_priv->tail, (u8 *)&sdio_header, sizeof(sdio_header));
	tx_priv->tail += sizeof(sdio_header);
	//payload
	//memcpy(tx_priv->tail, (u8 *)(long)&txhdr->sw_hdr->desc, sizeof(struct txdesc_api));
	//tx_priv->tail += sizeof(struct txdesc_api); //hostdesc
	//memcpy(tx_priv->tail, (u8 *)((u8 *)txhdr + txhdr->sw_hdr->headroom), pkt->len-txhdr->sw_hdr->headroom);
	//tx_priv->tail += (pkt->len - txhdr->sw_hdr->headroom);

	//word alignment
	curr_len = tx_priv->tail - tx_priv->head;
	if (curr_len & (TX_ALIGNMENT - 1)) {
		allign_len = roundup(curr_len, TX_ALIGNMENT)-curr_len;
		memcpy(tx_priv->tail, adjust_str, allign_len);
		tx_priv->tail += allign_len;
	}

	start_ptr[0] = ((tx_priv->tail - start_ptr - 4) & 0xff);
	start_ptr[1] = (((tx_priv->tail - start_ptr - 4)>>8) & 0x0f);
	tx_priv->aggr_buf->dev = pkt->dev;

	#if 0
	if (!txhdr->sw_hdr->need_cfm) {
		kmem_cache_free(txhdr->sw_hdr->rwnx_vif->rwnx_hw->sw_txhdr_cache, txhdr->sw_hdr);
		skb_pull(pkt, txhdr->sw_hdr->headroom);
		consume_skb(pkt);
	}
	#endif

	consume_skb(pkt);
	atomic_inc(&tx_priv->aggr_count);
	return 0;
}

void aicwf_sdio_aggr_send(struct aicwf_tx_priv *tx_priv)
{
	struct sk_buff *tx_buf = tx_priv->aggr_buf;
	int ret = 0;
	int curr_len = 0;

	//link tail is necessary
	curr_len = tx_priv->tail - tx_priv->head;
	if ((curr_len % TXPKT_BLOCKSIZE) != 0) {
		memset(tx_priv->tail, 0, TAIL_LEN);
		tx_priv->tail += TAIL_LEN;
	}

	tx_buf->len = tx_priv->tail - tx_priv->head;
	ret = aicwf_sdio_txpkt(tx_priv->sdiodev, tx_buf);
	if (ret < 0) {
		sdio_err("fail to send aggr pkt!\n");
	}

	aicwf_sdio_aggrbuf_reset(tx_priv);
}

void aicwf_sdio_aggrbuf_reset(struct aicwf_tx_priv *tx_priv)
{
	struct sk_buff *aggr_buf = tx_priv->aggr_buf;

	tx_priv->tail = tx_priv->head;
	aggr_buf->len = 0;
	atomic_set(&tx_priv->aggr_count, 0);
}

static int aicwf_sdio_bus_start(struct device *dev)
{
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_sdio_dev *sdiodev = bus_if->bus_priv.sdio;
	int ret = 0;

#if 1
	sdio_claim_host(sdiodev->func);
	sdio_claim_irq(sdiodev->func, aicwf_sdio_hal_irqhandler);
#else
	//since we have func2 we don't register irq handler
	sdio_claim_irq(sdiodev->func, NULL);
	sdiodev->func->irq_handler = (sdio_irq_handler_t *)aicwf_sdio_hal_irqhandler;
#endif
	//enable sdio interrupt
	ret = aicwf_sdio_writeb(sdiodev, SDIOWIFI_INTR_CONFIG_REG, 0x07);
	sdio_release_host(sdiodev->func);

	if (ret != 0)
		sdio_err("intr register failed:%d\n", ret);

	bus_if->state = BUS_UP_ST;

	return ret;
}

int aicwf_sdio_bustx_thread(void *data)
{
	struct aicwf_bus *bus = (struct aicwf_bus *) data;
	struct aic_sdio_dev *sdiodev = bus->bus_priv.sdio;

	while (1) {
		if (kthread_should_stop()) {
			sdio_err("sdio bustx thread stop\n");
			break;
		}
		if (!wait_for_completion_interruptible(&bus->bustx_trgg)) {
			if ((int)(atomic_read(&sdiodev->tx_priv->tx_pktcnt) > 0) || (sdiodev->tx_priv->cmd_txstate == true))
				aicwf_sdio_tx_process(sdiodev);
		}
	}

	return 0;
}

int aicwf_sdio_busrx_thread(void *data)
{
	struct aicwf_rx_priv *rx_priv = (struct aicwf_rx_priv *)data;
	struct aicwf_bus *bus_if = rx_priv->sdiodev->bus_if;

	while (1) {
		if (kthread_should_stop()) {
			sdio_err("sdio busrx thread stop\n");
			break;
		}
		if (!wait_for_completion_interruptible(&bus_if->busrx_trgg)) {
			aicwf_process_rxframes(rx_priv);
		}
	}

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
static void aicwf_sdio_bus_pwrctl(struct timer_list *t)
{
	struct aic_sdio_dev *sdiodev = from_timer(sdiodev, t, timer);
#else
static void aicwf_sdio_bus_pwrctl(ulong data)
{
	struct aic_sdio_dev *sdiodev = (struct aic_sdio_dev *) data;
#endif
	if (sdiodev->bus_if->state == BUS_DOWN_ST) {
		sdio_err("bus down\n");
		return;
	}

	if (sdiodev->pwrctl_tsk) {
		complete(&sdiodev->pwrctrl_trgg);
	}
}


static void aicwf_sdio_enq_rxpkt(struct aic_sdio_dev *sdiodev, struct sk_buff *pkt)
{
	struct aicwf_rx_priv *rx_priv = sdiodev->rx_priv;
	unsigned long flags = 0;

	spin_lock_irqsave(&rx_priv->rxqlock, flags);
	if (!aicwf_rxframe_enqueue(sdiodev->dev, &rx_priv->rxq, pkt)) {
		spin_unlock_irqrestore(&rx_priv->rxqlock, flags);
		aicwf_dev_skb_free(pkt);
		return;
	}
	spin_unlock_irqrestore(&rx_priv->rxqlock, flags);

	atomic_inc(&rx_priv->rx_cnt);
}

#define SDIO_OTHER_INTERRUPT (0x1ul << 7)

void aicwf_sdio_hal_irqhandler(struct sdio_func *func)
{
	struct aicwf_bus *bus_if = dev_get_drvdata(&func->dev);
	struct aic_sdio_dev *sdiodev = bus_if->bus_priv.sdio;
	u8 intstatus = 0;
	u8 byte_len = 0;
	struct sk_buff *pkt = NULL;
	int ret;

	if (!bus_if || bus_if->state == BUS_DOWN_ST) {
		sdio_err("bus err\n");
		return;
	}

	ret = aicwf_sdio_readb(sdiodev, SDIOWIFI_BLOCK_CNT_REG, &intstatus);
	while (ret || (intstatus & SDIO_OTHER_INTERRUPT)) {
		sdio_err("ret=%d, intstatus=%x\r\n", ret, intstatus);
		ret = aicwf_sdio_readb(sdiodev, SDIOWIFI_BLOCK_CNT_REG, &intstatus);
	}
	sdiodev->rx_priv->data_len = intstatus * SDIOWIFI_FUNC_BLOCKSIZE;

	if (intstatus > 0) {
		if (intstatus < 64) {
			pkt = aicwf_sdio_readframes(sdiodev);
		} else {
			aicwf_sdio_intr_get_len_bytemode(sdiodev, &byte_len);//byte_len must<= 128
			sdio_info("byte mode len=%d\r\n", byte_len);
			pkt = aicwf_sdio_readframes(sdiodev);
		}
	} else {
		sdio_err("Interrupt but no data\n");
	}

	if (pkt)
		aicwf_sdio_enq_rxpkt(sdiodev, pkt);

	complete(&bus_if->busrx_trgg);
}

void aicwf_sdio_pwrctl_timer(struct aic_sdio_dev *sdiodev, uint duration)
{
	uint timeout;

	if (sdiodev->bus_if->state == BUS_DOWN_ST && duration)
		return;

	spin_lock_bh(&sdiodev->pwrctl_lock);
	if (!duration) {
		if (timer_pending(&sdiodev->timer))
			del_timer_sync(&sdiodev->timer);
	} else {
		sdiodev->active_duration = duration;
		timeout = msecs_to_jiffies(sdiodev->active_duration);
		mod_timer(&sdiodev->timer, jiffies + timeout);
	}
	spin_unlock_bh(&sdiodev->pwrctl_lock);
}

static struct aicwf_bus_ops aicwf_sdio_bus_ops = {
	.stop = aicwf_sdio_bus_stop,
	.start = aicwf_sdio_bus_start,
	.txdata = aicwf_sdio_bus_txdata,
	.txmsg = aicwf_sdio_bus_txmsg,
};

void aicwf_sdio_release(struct aic_sdio_dev *sdiodev)
{
	struct aicwf_bus *bus_if;
	int ret = 0;

	sdio_dbg("%s\n", __func__);

	bus_if = sdiodev->bus_if;
	bus_if->state = BUS_DOWN_ST;

	sdio_claim_host(sdiodev->func);
	//disable sdio interrupt
	ret = aicwf_sdio_writeb(sdiodev, SDIOWIFI_INTR_CONFIG_REG, 0x0);
	if (ret < 0) {
		sdio_err("reg:%d write failed!\n", SDIOWIFI_INTR_CONFIG_REG);
	}
	sdio_release_irq(sdiodev->func);
	sdio_release_host(sdiodev->func);

	if (sdiodev->dev)
		aicwf_bus_deinit(sdiodev->dev);

	if (sdiodev->tx_priv)
		aicwf_tx_deinit(sdiodev->tx_priv);

	if (sdiodev->rx_priv)
		aicwf_rx_deinit(sdiodev->rx_priv);

	if (sdiodev->cmd_mgr.state == RWNX_CMD_MGR_STATE_INITED)
		rwnx_cmd_mgr_deinit(&sdiodev->cmd_mgr);
}

int aicwf_sdio_func_init(struct aic_sdio_dev *sdiodev)
{
	struct mmc_host *host;
	u8 block_bit0 = 0x1;
	u8 byte_mode_disable = 0x1;//1: no byte mode
	int ret = 0;
	struct aicbsp_feature_t feature;

	aicbsp_get_feature(&feature);
	host = sdiodev->func->card->host;

	sdio_claim_host(sdiodev->func);
	sdiodev->func->card->quirks |= MMC_QUIRK_LENIENT_FN0;
	sdio_f0_writeb(sdiodev->func, feature.sdio_phase, 0x13, &ret);
	if (ret < 0) {
		sdio_err("write func0 fail %d\n", ret);
		return ret;
	}

	ret = sdio_set_block_size(sdiodev->func, SDIOWIFI_FUNC_BLOCKSIZE);
	if (ret < 0) {
		sdio_err("set blocksize fail %d\n", ret);
		sdio_release_host(sdiodev->func);
		return ret;
	}
	ret = sdio_enable_func(sdiodev->func);
	if (ret < 0) {
		sdio_release_host(sdiodev->func);
		sdio_err("enable func fail %d.\n", ret);
	}

	if (feature.sdio_clock > 0) {
		host->ios.clock = feature.sdio_clock;
		host->ops->set_ios(host, &host->ios);
		sdio_dbg("Set SDIO Clock %d MHz\n", host->ios.clock/1000000);
	}
	sdio_release_host(sdiodev->func);

	ret = aicwf_sdio_writeb(sdiodev, SDIOWIFI_REGISTER_BLOCK, block_bit0);
	if (ret < 0) {
		sdio_err("reg:%d write failed!\n", SDIOWIFI_REGISTER_BLOCK);
		return ret;
	}

	//1: no byte mode
	ret = aicwf_sdio_writeb(sdiodev, SDIOWIFI_BYTEMODE_ENABLE_REG, byte_mode_disable);
	if (ret < 0) {
		sdio_err("reg:%d write failed!\n", SDIOWIFI_BYTEMODE_ENABLE_REG);
		return ret;
	}

	return ret;
}


void aicwf_sdio_func_deinit(struct aic_sdio_dev *sdiodev)
{
	sdio_claim_host(sdiodev->func);
	sdio_disable_func(sdiodev->func);
	sdio_release_host(sdiodev->func);
}

void *aicwf_sdio_bus_init(struct aic_sdio_dev *sdiodev)
{
	int ret;
	struct aicwf_bus *bus_if;
	struct aicwf_rx_priv *rx_priv;
	struct aicwf_tx_priv *tx_priv;

	spin_lock_init(&sdiodev->pwrctl_lock);
	sema_init(&sdiodev->pwrctl_wakeup_sema, 1);

	bus_if = sdiodev->bus_if;
	bus_if->dev = sdiodev->dev;
	bus_if->ops = &aicwf_sdio_bus_ops;
	bus_if->state = BUS_DOWN_ST;
	sdiodev->state = SDIO_SLEEP_ST;
	sdiodev->active_duration = SDIOWIFI_PWR_CTRL_INTERVAL;

	rx_priv = aicwf_rx_init(sdiodev);
	if (!rx_priv) {
		sdio_err("rx init fail\n");
		goto fail;
	}
	sdiodev->rx_priv = rx_priv;

	tx_priv = aicwf_tx_init(sdiodev);
	if (!tx_priv) {
		sdio_err("tx init fail\n");
		goto fail;
	}
	sdiodev->tx_priv = tx_priv;
	aicwf_frame_queue_init(&tx_priv->txq, 8, TXQLEN);
	spin_lock_init(&tx_priv->txqlock);
	sema_init(&tx_priv->txctl_sema, 1);
	sema_init(&tx_priv->cmd_txsema, 1);
	init_waitqueue_head(&tx_priv->cmd_txdone_wait);
	atomic_set(&tx_priv->tx_pktcnt, 0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	timer_setup(&sdiodev->timer, aicwf_sdio_bus_pwrctl, 0);
#else
	init_timer(&sdiodev->timer);
	sdiodev->timer.data = (ulong) sdiodev;
	sdiodev->timer.function = aicwf_sdio_bus_pwrctl;
#endif
	init_completion(&sdiodev->pwrctrl_trgg);
	ret = aicwf_bus_init(0, sdiodev->dev);
	if (ret < 0) {
		sdio_err("bus init fail\n");
		goto fail;
	}

	ret  = aicwf_bus_start(bus_if);
	if (ret != 0) {
		sdio_err("bus start fail\n");
		goto fail;
	}

	return sdiodev;

fail:
	aicwf_sdio_release(sdiodev);
	return NULL;
}


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
#include <linux/pm_wakeirq.h>
#include "aicwf_txrxif.h"
#include "aicwf_sdio.h"
#include "sdio_host.h"
#include "rwnx_defs.h"
#include "rwnx_platform.h"
#ifdef CONFIG_PREALLOC_RX_SKB
#include "aicwf_rx_prealloc.h"
#endif
#ifdef CONFIG_INGENIC_T20
#include "mach/jzmmc.h"
#endif /* CONFIG_INGENIC_T20 */
#include "aic_bsp_export.h"
#include "rwnx_wakelock.h"
extern uint8_t scanning;

static int aicwf_chipid;
struct aicbsp_feature_t aicwf_feature;

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

int aicwf_sdio_func2_readb(struct aic_sdio_dev *sdiodev, uint regaddr, u8 *val)
{
	int ret;
	sdio_claim_host(sdiodev->func2);
	*val = sdio_readb(sdiodev->func2, regaddr, &ret);
	sdio_release_host(sdiodev->func2);
	return ret;
}

int aicwf_sdio_func2_writeb(struct aic_sdio_dev *sdiodev, uint regaddr, u8 val)
{
	int ret;
	sdio_claim_host(sdiodev->func2);
	sdio_writeb(sdiodev->func2, val, regaddr, &ret);
	sdio_release_host(sdiodev->func2);
	return ret;
}

int tx_aggr_counter = 64;
int aicwf_sdio_flow_ctrl_msg(struct aic_sdio_dev *sdiodev)
{
	int ret = -1;
	u8 fc_reg = 0;
	u32 count = 0;

	while (true) {
		ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.flow_ctrl_reg, &fc_reg);
		if (ret) {
			return -1;
		}

		if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D || sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
			sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
			fc_reg = fc_reg & SDIOWIFI_FLOWCTRL_MASK_REG;
		}

		if (fc_reg != 0) {
			ret = fc_reg;
			if (ret > tx_aggr_counter) {
				ret = tx_aggr_counter;
			}
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
				msleep(2);
			else
				msleep(10);
		}
	}

	return ret;
}

int aicwf_sdio_flow_ctrl(struct aic_sdio_dev *sdiodev)
{
	int ret = -1;
	u8 fc_reg = 0;
	u32 count = 0;

	while (true) {
		ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.flow_ctrl_reg, &fc_reg);
		if (ret) {
			return -1;
		}

		if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D || sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
			sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
			fc_reg = fc_reg & SDIOWIFI_FLOWCTRL_MASK_REG;
		}

		if (fc_reg > DATA_FLOW_CTRL_THRESH) {
			ret = fc_reg;
			if (ret > tx_aggr_counter) {
				ret = tx_aggr_counter;
			}
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
				msleep(2);
			else
				msleep(10);
		}
	}

	return ret;
}

int aicwf_sdio_send_pkt(struct aic_sdio_dev *sdiodev, u8 *buf, uint count)
{
	int ret = 0;

	sdio_claim_host(sdiodev->func);
	ret = sdio_writesb(sdiodev->func, sdiodev->sdio_reg.wr_fifo_addr, buf, count);
	sdio_release_host(sdiodev->func);

	return ret;
}

#ifdef CONFIG_PREALLOC_RX_SKB
int aicwf_sdio_recv_pkt(struct aic_sdio_dev *sdiodev, struct rx_buff *rxbuff,
	u32 size)
{
	int ret;

	if ((!rxbuff->data) || (!size)) {
		return -EINVAL;;
	}

	sdio_claim_host(sdiodev->func);
	ret = sdio_readsb(sdiodev->func, rxbuff->data, sdiodev->sdio_reg.rd_fifo_addr, size);
	sdio_release_host(sdiodev->func);

	if (ret < 0) {
		return ret;
	}
	rxbuff->len = size;

	return ret;
}
#else
int aicwf_sdio_recv_pkt(struct aic_sdio_dev *sdiodev, struct sk_buff *skbbuf,
	u32 size)
{
	int ret;

	if ((!skbbuf) || (!size)) {
		return -EINVAL;;
	}

	sdio_claim_host(sdiodev->func);
	ret = sdio_readsb(sdiodev->func, skbbuf->data, sdiodev->sdio_reg.rd_fifo_addr, size);
	sdio_release_host(sdiodev->func);

	if (ret < 0) {
		return ret;
	}
	skbbuf->len = size;

	return ret;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
extern int sunxi_wlan_get_oob_irq(int *, int *);
#else
extern int sunxi_wlan_get_oob_irq(void);
extern int sunxi_wlan_get_oob_irq_flags(void);
#endif

static irqreturn_t rwnx_hostwake_irq_handler(int irq, void *para)
{
	static int wake_cnt;
	wake_cnt++;
	rwnx_wakeup_lock_timeout(g_rwnx_plat->sdiodev->rwnx_hw->ws_rx, 1000);

#ifdef CONFIG_OOB
	complete(&g_rwnx_plat->sdiodev->bus_if->busrx_trgg);
#endif

	return IRQ_HANDLED;
}

static int rwnx_register_hostwake_irq(struct device *dev)
{
	int ret = -1;
	int irq_flags;
	int wakeup_enable;
	u32 hostwake_irq_num;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	hostwake_irq_num = sunxi_wlan_get_oob_irq(&irq_flags, &wakeup_enable);
#else
	hostwake_irq_num = sunxi_wlan_get_oob_irq();
	irq_flags = sunxi_wlan_get_oob_irq_flags();
	wakeup_enable = 1;
#endif

	if (wakeup_enable) {
		ret = device_init_wakeup(dev, true);
		if (ret < 0) {
			pr_err("%s(%d): device init wakeup failed!\n", __func__, __LINE__);
			return ret;
		}

		ret = dev_pm_set_wake_irq(dev, hostwake_irq_num);
		if (ret < 0) {
			pr_err("%s(%d): can't enable wakeup src!\n", __func__, __LINE__);
			goto fail1;
		}

		irq_flags = IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND;
#if IS_ENABLED(CONFIG_AIC_IRQ_ACTIVE_HIGH)
		irq_flags = IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND;
#elif IS_ENABLED(CONFIG_AIC_IRQ_ACTIVE_RISING)
		irq_flags = IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND;
#elif IS_ENABLED(CONFIG_AIC_IRQ_ACTIVE_LOW)
		irq_flags = IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND;
#elif IS_ENABLED(CONFIG_AIC_IRQ_ACTIVE_FALLING)
		irq_flags = IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND;
#endif
		ret = request_irq(hostwake_irq_num,
				rwnx_hostwake_irq_handler, irq_flags,
				"rwnx_hostwake_irq", NULL);

		if (ret < 0) {
			pr_err("%s(%d): request_irq fail! ret = %d\n", __func__, __LINE__, ret);
			goto fail2;
		}
	}
	g_rwnx_plat->sdiodev->rwnx_hw->wakeup_enable = wakeup_enable;
	g_rwnx_plat->sdiodev->rwnx_hw->hostwake_irq_num = hostwake_irq_num;

	sdio_info("%s(%d)\n", __func__, __LINE__);
	return ret;

fail2:
	dev_pm_clear_wake_irq(dev);
fail1:
	device_init_wakeup(dev, false);
	return ret;
}

static int rwnx_unregister_hostwake_irq(struct device *dev)
{
	int wakeup_enable = g_rwnx_plat->sdiodev->rwnx_hw->wakeup_enable;
	u32 hostwake_irq_num = g_rwnx_plat->sdiodev->rwnx_hw->hostwake_irq_num;
	if (wakeup_enable) {
		free_irq(hostwake_irq_num, NULL);
		device_init_wakeup(dev, false);
		dev_pm_clear_wake_irq(dev);
	}
	printk("%s(%d)\n", __func__, __LINE__);
	return 0;
}

static int aicwf_sdio_probe(struct sdio_func *func,
	const struct sdio_device_id *id)
{
	struct mmc_host *host;
	struct aic_sdio_dev *sdiodev;
	struct aicwf_bus *bus_if;
	int err = -ENODEV;

	sdio_dbg("%s:%d\n", __func__, func->num);
	host = func->card->host;
	if (func->num != 1) {
		return err;
	}

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
	sdiodev->oob_enable = false;
	atomic_set(&sdiodev->is_bus_suspend, 0);
	bus_if->bus_priv.sdio = sdiodev;
	dev_set_drvdata(&func->dev, bus_if);
	sdiodev->dev = &func->dev;
	host->ops->enable_sdio_irq(host, false);

#ifdef AICWF_BSP_CTRL
	aicbsp_get_feature(&aicwf_feature);
#endif
	aicwf_chipid = aicwf_feature.chipinfo->chipid;

#ifdef CONFIG_OOB
	if (aicwf_chipid == PRODUCT_ID_AIC8800D) {
		sdio_err("%s ERROR!!! 8800D not support OOB \r\n", __func__);
		goto fail;
	}
#endif //CONFIG_OOB

	if (aicwf_chipid == PRODUCT_ID_AIC8800D || aicwf_chipid == PRODUCT_ID_AIC8800DC ||
		aicwf_chipid == PRODUCT_ID_AIC8800DW) {
		sdiodev->func2 = func->card->sdio_func[1];
	}

	if (aicwf_chipid != PRODUCT_ID_AIC8800D80) {
		err = aicwf_sdio_func_init(sdiodev);
	} else {
		err = aicwf_sdiov3_func_init(sdiodev);
	}
	if (err < 0) {
		sdio_err("sdio func init fail\n");
		goto fail;
	}

	if (aicwf_sdio_bus_init(sdiodev) == NULL) {
		sdio_err("sdio bus init fail\n");
		err = -1;
		goto fail;
	}

	host->caps |= MMC_CAP_NONREMOVABLE;
	err = aicwf_rwnx_sdio_platform_init(sdiodev);
	if (err != 0)
		goto init_hostif_fail;

	host->ops->enable_sdio_irq(host, true);

	err = rwnx_register_hostwake_irq(sdiodev->dev);
	if (err != 0)
		goto init_hostif_fail;

	aicwf_hostif_ready();

#ifdef CONFIG_OOB
	sdio_info("%s SDIOWIFI_INTR_CONFIG_REG Disable\n", __func__);
	sdio_claim_host(sdiodev->func);
	// disable sdio interrupt
	err = aicwf_sdio_writeb(sdiodev, SDIOWIFI_INTR_CONFIG_REG, 0x0);
	if (err < 0) {
		sdio_err("reg:%d write failed!\n", SDIOWIFI_INTR_CONFIG_REG);
	}
	sdio_release_irq(sdiodev->func);
	sdio_release_host(sdiodev->func);
	sdiodev->oob_enable = true;
#endif
	device_disable_async_suspend(sdiodev->dev);
	return 0;

init_hostif_fail:
	aicwf_sdio_release(sdiodev);

fail:
	aicwf_sdio_func_deinit(sdiodev);
	dev_set_drvdata(&func->dev, NULL);
	kfree(sdiodev);
	kfree(bus_if);
	aicwf_hostif_fail();
	return err;
}

static void aicwf_sdio_remove(struct sdio_func *func)
{
	struct mmc_host *host;
	struct aicwf_bus *bus_if = NULL;
	struct aic_sdio_dev *sdiodev = NULL;

	sdio_dbg("%s\n", __func__);
	host = func->card->host;
	host->caps &= ~MMC_CAP_NONREMOVABLE;
	bus_if = dev_get_drvdata(&func->dev);
	if (!bus_if) {
		return;
	}

	sdiodev = bus_if->bus_priv.sdio;
	if (!sdiodev) {
		return;
	}
	sdiodev->bus_if->state = BUS_DOWN_ST;
	aicwf_sdio_release(sdiodev);
	aicwf_sdio_func_deinit(sdiodev);
	dev_set_drvdata(&sdiodev->func->dev, NULL);
	kfree(sdiodev);
	kfree(bus_if);
	sdio_dbg("%s done\n", __func__);
}
#if IS_ENABLED(CONFIG_PM)
static int aicwf_sdio_suspend(struct device *dev)
{
	int ret = 0;
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_sdio_dev *sdiodev = bus_if->bus_priv.sdio;
	mmc_pm_flag_t sdio_flags;
	struct rwnx_vif *rwnx_vif, *tmp;

	sdio_dbg("%s Enter\n", __func__);
	list_for_each_entry_safe(rwnx_vif, tmp, &sdiodev->rwnx_hw->vifs, list) {
		if (rwnx_vif->ndev)
			netif_device_detach(rwnx_vif->ndev);
	}

	sdio_flags = sdio_get_host_pm_caps(sdiodev->func);
	if (!(sdio_flags & MMC_PM_KEEP_POWER)) {
		sdio_dbg("%s sdio get host pm caps: no MMC_PM_KEEP_POWER\n", __func__);
		return -EINVAL;
	}
	ret = sdio_set_host_pm_flags(sdiodev->func, MMC_PM_KEEP_POWER);
	if (ret) {
		sdio_dbg("%s sdio set host pm flags of MMC_PM_KEEP_POWER, err: %d\n", __func__, ret);
		return ret;
	}

	if (aicwf_wakeup_lock_status(sdiodev->rwnx_hw)) {
		sdio_dbg("%s ws active dont suspend\n", __func__);
		return -EBUSY;
	}

	while (sdiodev->state == SDIO_ACTIVE_ST) {
		if (down_interruptible(&sdiodev->tx_priv->txctl_sema))
			continue;
#if defined(CONFIG_SDIO_PWRCTRL)
		aicwf_sdio_pwr_stctl(sdiodev, SDIO_SLEEP_ST);
#endif
		up(&sdiodev->tx_priv->txctl_sema);
		break;
	}
	atomic_set(&sdiodev->is_bus_suspend, 1);
	sdio_dbg("%s Exit\n", __func__);
	return 0;
}

static int aicwf_sdio_resume(struct device *dev)
{
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_sdio_dev *sdiodev = bus_if->bus_priv.sdio;
	struct rwnx_vif *rwnx_vif, *tmp;

	sdio_dbg("%s Enter\n", __func__);
	list_for_each_entry_safe(rwnx_vif, tmp, &sdiodev->rwnx_hw->vifs, list) {
		if (rwnx_vif->ndev)
			netif_device_attach(rwnx_vif->ndev);
	}
#if defined(CONFIG_SDIO_PWRCTRL)
	aicwf_sdio_pwr_stctl(sdiodev, SDIO_ACTIVE_ST);
#endif
	atomic_set(&sdiodev->is_bus_suspend, 0);
	sdio_dbg("%s Exit\n", __func__);
	return 0;
}

static const struct dev_pm_ops aicwf_sdio_pm_ops = {
	.suspend = aicwf_sdio_suspend,
	.resume = aicwf_sdio_resume,
};
#endif  /* CONFIG_PM */

static const struct sdio_device_id aicwf_sdmmc_ids[] = {
	{SDIO_DEVICE(0x5449, 0x0145)}, // 8800d in nomal mode
	{SDIO_DEVICE(0xc8a1, 0xc08d)}, // 8800dc in nomal mode
	{SDIO_DEVICE(0xc8a1, 0x0082)}, // 8800d80 in nomal mode
	{ },
};

MODULE_DEVICE_TABLE(sdio, aicwf_sdmmc_ids);

static struct sdio_driver aicwf_sdio_driver = {
	.probe = aicwf_sdio_probe,
	.remove = aicwf_sdio_remove,
	.name = AICWF_SDIO_NAME,
	.id_table = aicwf_sdmmc_ids,
#if IS_ENABLED(CONFIG_PM)
	.drv = {
		.pm = &aicwf_sdio_pm_ops,
	},
#endif  /* CONFIG_PM */
};

#ifdef CONFIG_NANOPI_M4
extern int mmc_rescan_try_freq(struct mmc_host *host, unsigned freq);
extern unsigned  aic_max_freqs;
extern struct mmc_host *aic_host_drv;
extern int __mmc_claim_host(struct mmc_host *host, atomic_t *abort);
extern void mmc_release_host(struct mmc_host *host);
#endif

void aicwf_sdio_register(void)
{
#ifdef CONFIG_PLATFORM_NANOPI
	extern_wifi_set_enable(0);
	mdelay(200);
	extern_wifi_set_enable(1);
	mdelay(200);
	sdio_reinit();
#endif /*CONFIG_PLATFORM_NANOPI*/

#ifdef CONFIG_INGENIC_T20
	jzmmc_manual_detect(1, 1);
#endif /* CONFIG_INGENIC_T20 */

#ifdef CONFIG_NANOPI_M4
	if (aic_host_drv->card == NULL) {
		__mmc_claim_host(aic_host_drv, NULL);
		printk("aic: >>>mmc_rescan_try_freq\n");
		mmc_rescan_try_freq(aic_host_drv, aic_max_freqs);
		mmc_release_host(aic_host_drv);
	}
#endif
	if (sdio_register_driver(&aicwf_sdio_driver)) {

	} else {
		//may add mmc_rescan here
	}
}

void aicwf_sdio_exit(void)
{
	if (g_rwnx_plat && g_rwnx_plat->enabled) {
#if defined(CONFIG_SDIO_PWRCTRL)
		if (timer_pending(&g_rwnx_plat->sdiodev->timer)) {
			del_timer_sync(&g_rwnx_plat->sdiodev->timer);
		}
#endif
		rwnx_unregister_hostwake_irq(g_rwnx_plat->sdiodev->dev);
		rwnx_platform_deinit(g_rwnx_plat->sdiodev->rwnx_hw);
	}

	udelay(500);
	sdio_unregister_driver(&aicwf_sdio_driver);

#ifdef CONFIG_PLATFORM_NANOPI
	extern_wifi_set_enable(0);
#endif /*CONFIG_PLATFORM_NANOPI*/
	kfree(g_rwnx_plat);
}

#if defined(CONFIG_SDIO_PWRCTRL)
int aicwf_sdio_wakeup(struct aic_sdio_dev *sdiodev)
{
	int ret = 0;
	int read_retry;
	int write_retry = 1;

	int wakeup_reg_val;

	if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D ||
		sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
		sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
		wakeup_reg_val = 1;
	} else if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
		wakeup_reg_val = 0x11;
	}

	if (sdiodev->state == SDIO_SLEEP_ST) {
		sdio_info("w\n");
		while (write_retry) {
			ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.wakeup_reg, wakeup_reg_val);
			if (ret) {
				txrx_err("sdio wakeup fail\n");
				ret = -1;
			} else {
				read_retry = 50;
				while (read_retry) {
					u8 val;
					ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.wakeup_reg, &val);
					if (ret < 0)
						txrx_err("sdio wakeup read fail\n");
					else if ((val & 0x1) == 0) {
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

		sdiodev->state = SDIO_ACTIVE_ST;
		aicwf_sdio_pwrctl_timer(sdiodev, sdiodev->active_duration);
	}
	return ret;
}

int aicwf_sdio_sleep_allow(struct aic_sdio_dev *sdiodev)
{
	int ret = 0;
	struct aicwf_bus *bus_if = sdiodev->bus_if;
	struct rwnx_hw *rwnx_hw = sdiodev->rwnx_hw;
	u8 read_retry;
	u8 val;

	if (bus_if->state == BUS_DOWN_ST) {
		ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.sleep_reg, 0x10);
		if (ret) {
			sdio_err("Write sleep fail!\n");
		}
		aicwf_sdio_pwrctl_timer(sdiodev, 0);
		return ret;
	}

	sdio_info("sleep: %d, %d\n", sdiodev->state, scanning);
	if (sdiodev->state == SDIO_ACTIVE_ST  && !scanning && !rwnx_hw->is_p2p_alive \
				&& !rwnx_hw->is_p2p_connected && (int)(atomic_read(&sdiodev->tx_priv->tx_pktcnt) <= 0) && (sdiodev->tx_priv->cmd_txstate == false) && \
				(int)(atomic_read(&sdiodev->rx_priv->rx_cnt) == 0)) {
		sdio_info("s\n");
		if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
			if (aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.wakeup_reg, 0x02) < 0) {
				sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.wakeup_reg);
			}
		} else if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D ||
			sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
			sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
			if (aicwf_sdio_func2_writeb(sdiodev, sdiodev->sdio_reg.wakeup_reg, 0x2) < 0) {
				sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.wakeup_reg);
			}
			read_retry = 100;
			while (read_retry) {
				val = 0;
				if (aicwf_sdio_func2_readb(sdiodev, sdiodev->sdio_reg.wakeup_reg, &val) < 0) {
					sdio_err("reg %d read fail\n", sdiodev->sdio_reg.wakeup_reg);
				} else if ((val & 0x2) == 0) {
					break;
				} else {
					sdio_info("val:%d\n", val);
				}
				read_retry--;
				if (read_retry < 90)
					sdio_info("warning: read cnt %d\n", read_retry);
				udelay(500);
			}
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

	down(&sdiodev->pwrctl_wakeup_sema);

	if (sdiodev->state == target) {
		if (target == SDIO_ACTIVE_ST) {
			aicwf_sdio_pwrctl_timer(sdiodev, sdiodev->active_duration);
		}
		up(&sdiodev->pwrctl_wakeup_sema);
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

	up(&sdiodev->pwrctl_wakeup_sema);
	return ret;
}
#endif

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
		ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.bytemode_len_reg, byte_len);
		sdiodev->rx_priv->data_len = (*byte_len)*4;
	}

	return ret;
}

static void aicwf_sdio_bus_stop(struct device *dev)
{
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_sdio_dev *sdiodev = bus_if->bus_priv.sdio;
	int ret = 0;

#if defined(CONFIG_SDIO_PWRCTRL)
	aicwf_sdio_pwrctl_timer(sdiodev, 0);
#endif
	sdio_dbg("%s\n", __func__);

	bus_if->state = BUS_DOWN_ST;
	if (sdiodev->tx_priv) {
		ret = down_interruptible(&sdiodev->tx_priv->txctl_sema);
		if (ret)
			sdio_err("down txctl_sema fail\n");
	}

#if defined(CONFIG_SDIO_PWRCTRL)
	aicwf_sdio_pwr_stctl(sdiodev, SDIO_SLEEP_ST);
#endif

	if (sdiodev->tx_priv) {
		if (!ret)
			up(&sdiodev->tx_priv->txctl_sema);
		aicwf_frame_queue_flush(&sdiodev->tx_priv->txq);
	}
	sdio_dbg("exit %s\n", __func__);
}

#ifdef CONFIG_PREALLOC_RX_SKB
struct rx_buff *aicwf_sdio_readframes(struct aic_sdio_dev *sdiodev)
{
	int ret = 0;
	u32 size = 0;
	struct aicwf_bus *bus_if = dev_get_drvdata(sdiodev->dev);
	struct rx_buff* rxbuff;

	if (bus_if->state == BUS_DOWN_ST) {
		sdio_dbg("bus down\n");
		return NULL;
	}

	size = sdiodev->rx_priv->data_len;
	rxbuff =  aicwf_prealloc_rxbuff_alloc(&sdiodev->rx_priv->rxbuff_lock);
	if (rxbuff == NULL) {
		printk("failed to alloc rxbuff\n");
		return NULL;
	}
	rxbuff->len = 0;
	rxbuff->start = rxbuff->data;
	rxbuff->read = rxbuff->start;
	rxbuff->end = rxbuff->data + size;

	ret = aicwf_sdio_recv_pkt(sdiodev, rxbuff, size);
	if (ret) {
		printk("%s %d, sdio recv pkt fail\n", __func__, __LINE__);
		aicwf_prealloc_rxbuff_free(rxbuff, &sdiodev->rx_priv->rxbuff_lock);
		return NULL;
	}

	return rxbuff;
}
#else
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
#endif

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

	if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D || sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
		buffer_cnt = aicwf_sdio_flow_ctrl_msg(sdiodev);
		while ((buffer_cnt <= 0 || (buffer_cnt > 0 && len > (buffer_cnt * BUFFER_SIZE))) && retry < 10) {
			retry++;
			buffer_cnt = aicwf_sdio_flow_ctrl_msg(sdiodev);
			printk("buffer_cnt = %d\n", buffer_cnt);
		}
	}
	down(&sdiodev->tx_priv->cmd_txsema);

	if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D || sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
		if (buffer_cnt > 0 && len < (buffer_cnt * BUFFER_SIZE)) {
			err = aicwf_sdio_send_pkt(sdiodev, payload, len);
			if (err) {
				sdio_err("aicwf_sdio_send_pkt fail%d\n", err);
			}
		} else {
			sdio_err("tx msg fc retry fail:%d, %d\n", buffer_cnt, len);
			up(&sdiodev->tx_priv->cmd_txsema);
			return -1;
		}
	} else if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DC) {
		err = aicwf_sdio_send_pkt(sdiodev, payload, len);
		if (err) {
			sdio_err("aicwf_sdio_send_pkt fail%d\n", err);
		}
	} else {
		sdio_err("tx msg fc retry fail:%d, %d\n", buffer_cnt, len);
		up(&sdiodev->tx_priv->cmd_txsema);
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

#if defined(CONFIG_SDIO_PWRCTRL)
	aicwf_sdio_pwr_stctl(sdiodev, SDIO_ACTIVE_ST);
#endif

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
		if (sdiodev->tx_priv->fw_avail_bufcnt <= DATA_FLOW_CTRL_THRESH) {
			if (sdiodev->tx_priv->cmd_txstate)
				break;
			sdiodev->tx_priv->fw_avail_bufcnt = aicwf_sdio_flow_ctrl(sdiodev);
		} else {
			if (sdiodev->tx_priv->cmd_txstate) {
				aicwf_sdio_send(sdiodev->tx_priv, 1);
				break;
			} else {
				aicwf_sdio_send(sdiodev->tx_priv, 0);
			}
		}
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
		struct rwnx_txhdr *txhdr = (struct rwnx_txhdr *)pkt->data;
		int headroom = txhdr->sw_hdr->headroom;
		kmem_cache_free(txhdr->sw_hdr->rwnx_vif->rwnx_hw->sw_txhdr_cache, txhdr->sw_hdr);
		skb_pull(pkt, headroom);
		consume_skb(pkt);
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
	if (atomic_read(&sdiodev->tx_priv->tx_pktcnt) == 1)
		complete(&bus_if->bustx_trgg);

	return ret;
}

static int aicwf_sdio_bus_txmsg(struct device *dev, u8 *msg, uint msglen)
{
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
	return 0;
}

int aicwf_sdio_send(struct aicwf_tx_priv *tx_priv, u8 txnow)
{
	struct sk_buff *pkt;
	struct aic_sdio_dev *sdiodev = tx_priv->sdiodev;
	u32 aggr_len = 0;

	aggr_len = (tx_priv->tail - tx_priv->head);
	if (((atomic_read(&tx_priv->aggr_count) == 0) && (aggr_len != 0))
		|| ((atomic_read(&tx_priv->aggr_count) != 0) && (aggr_len == 0))) {
		if (aggr_len > 0)
			aicwf_sdio_aggrbuf_reset(tx_priv);
		return 0;
	}

	if (atomic_read(&tx_priv->aggr_count) == (tx_priv->fw_avail_bufcnt - DATA_FLOW_CTRL_THRESH)) {
		if (atomic_read(&tx_priv->aggr_count) > 0) {
			tx_priv->fw_avail_bufcnt -= atomic_read(&tx_priv->aggr_count);
			aicwf_sdio_aggr_send(tx_priv); //send and check the next pkt;
		}
		return 0;
	} else {
		spin_lock_bh(&sdiodev->tx_priv->txqlock);
		pkt = aicwf_frame_dequeue(&sdiodev->tx_priv->txq);
		if (pkt == NULL) {
			sdio_err("txq no pkt\n");
			spin_unlock_bh(&sdiodev->tx_priv->txqlock);
			return 0;
		}
		//atomic_dec(&sdiodev->tx_priv->tx_pktcnt);
		spin_unlock_bh(&sdiodev->tx_priv->txqlock);

		if (tx_priv == NULL || tx_priv->tail == NULL || pkt == NULL)
			txrx_err("null error\n");
		if (aicwf_sdio_aggr(tx_priv, pkt)) {
			aicwf_sdio_aggrbuf_reset(tx_priv);
			sdio_err("add aggr pkts failed!\n");
			atomic_dec(&sdiodev->tx_priv->tx_pktcnt);
			return 0;
		}

		//when aggr finish or there is cmd to send, just send this aggr pkt to fw
		if ((int)atomic_read(&sdiodev->tx_priv->tx_pktcnt) == 1 || txnow || (atomic_read(&tx_priv->aggr_count) == (tx_priv->fw_avail_bufcnt - DATA_FLOW_CTRL_THRESH))) {
			tx_priv->fw_avail_bufcnt -= atomic_read(&tx_priv->aggr_count);
			aicwf_sdio_aggr_send(tx_priv);
			atomic_dec(&sdiodev->tx_priv->tx_pktcnt);
			return 0;
		} else {
			atomic_dec(&sdiodev->tx_priv->tx_pktcnt);
			return 0;
		}
	}
}

static void aicwf_count_tx_tp(struct aicwf_tx_priv *tx_priv, int len)
{
#ifdef AICWF_SDIO_SUPPORT
	struct device *rwnx_dev = tx_priv->sdiodev->dev;
#endif
#ifdef AICWF_USB_SUPPORT
	struct device *rwnx_dev = tx_priv->usbdev->dev;
#endif
	long long timeus = 0;
	char *envp[] = {
		"SYSTEM=WIFI",
		"EVENT=BOOSTREQ",
		"SUBEVENT=TX",
		"TIMEOUT_SEC=5",
		NULL};

	tx_priv->tx_data_len += len;

	tx_priv->txtimeend = ktime_get();
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
	timeus = div_u64(tx_priv->txtimeend.tv64 - tx_priv->txtimebegin.tv64, NSEC_PER_USEC);
#else
	timeus = ktime_to_us(tx_priv->txtimeend - tx_priv->txtimebegin);
#endif

	if (timeus >= USEC_PER_SEC) {
		// calc & send uevent
		if (div_u64(tx_priv->tx_data_len, timeus) >= 6)
			kobject_uevent_env(&rwnx_dev->kobj, KOBJ_CHANGE, envp);

		tx_priv->tx_data_len = 0;
		tx_priv->txtimebegin = tx_priv->txtimeend;
	}
}

int aicwf_sdio_aggr(struct aicwf_tx_priv *tx_priv, struct sk_buff *pkt)
{
	struct rwnx_txhdr *txhdr = (struct rwnx_txhdr *)pkt->data;
	u8 *start_ptr = tx_priv->tail;
	u8 sdio_header[4];
	u8 adjust_str[4] = {0, 0, 0, 0};
	u32 curr_len = 0;
	int allign_len = 0;
	int headroom;

	aicwf_count_tx_tp(tx_priv, pkt->len);
	sdio_header[0] = ((pkt->len - txhdr->sw_hdr->headroom + sizeof(struct txdesc_api)) & 0xff);
	sdio_header[1] = (((pkt->len - txhdr->sw_hdr->headroom + sizeof(struct txdesc_api)) >> 8)&0x0f);
	sdio_header[2] = 0x01; //data
	if (tx_priv->sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D ||
		tx_priv->sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
		tx_priv->sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DW)
		sdio_header[3] = 0; //reserved
	else if (tx_priv->sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D80)
		sdio_header[3] = crc8_ponl_107(&sdio_header[0], 3); // crc8

	memcpy(tx_priv->tail, (u8 *)&sdio_header, sizeof(sdio_header));
	tx_priv->tail += sizeof(sdio_header);
	//payload
	memcpy(tx_priv->tail, (u8 *)(long)&txhdr->sw_hdr->desc, sizeof(struct txdesc_api));
	tx_priv->tail += sizeof(struct txdesc_api); //hostdesc
	memcpy(tx_priv->tail, (u8 *)((u8 *)txhdr + txhdr->sw_hdr->headroom), pkt->len-txhdr->sw_hdr->headroom);
	tx_priv->tail += (pkt->len - txhdr->sw_hdr->headroom);

	//word alignment
	curr_len = tx_priv->tail - tx_priv->head;
	if (curr_len & (TX_ALIGNMENT - 1)) {
		allign_len = roundup(curr_len, TX_ALIGNMENT)-curr_len;
		memcpy(tx_priv->tail, adjust_str, allign_len);
		tx_priv->tail += allign_len;
	}

	if (tx_priv->sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D || tx_priv->sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
		tx_priv->sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
		start_ptr[0] = ((tx_priv->tail - start_ptr - 4) & 0xff);
		start_ptr[1] = (((tx_priv->tail - start_ptr - 4)>>8) & 0x0f);
	}
	tx_priv->aggr_buf->dev = pkt->dev;

	if (!txhdr->sw_hdr->need_cfm) {
		headroom = txhdr->sw_hdr->headroom;
		if (unlikely(txhdr->sw_hdr->rwnx_vif == NULL ||
					txhdr->sw_hdr->rwnx_vif->rwnx_hw == NULL)) {
			printk("Null pointer in %s! rwnx_vif addr is %p", __func__, txhdr->sw_hdr->rwnx_vif);
		} else {
			kmem_cache_free(txhdr->sw_hdr->rwnx_vif->rwnx_hw->sw_txhdr_cache, txhdr->sw_hdr);
			skb_pull(pkt, headroom);
			consume_skb(pkt);
		}
	}

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

	sdio_claim_host(sdiodev->func);
	sdio_claim_irq(sdiodev->func, aicwf_sdio_hal_irqhandler);
	if (aicwf_chipid == PRODUCT_ID_AIC8800D80) {
		sdio_f0_writeb(sdiodev->func, 0x07, 0x04, &ret);
		if (ret) {
			sdio_err("set func0 int en fail %d\n", ret);
		}
	}
	sdio_release_host(sdiodev->func);

	//enable sdio interrupt
	ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.intr_config_reg, 0x07);
	if (ret != 0)
		sdio_err("intr register failed:%d\n", ret);

	bus_if->state = BUS_UP_ST;

	return ret;
}

static inline void aic_thread_wait_stop(void)
{
#if 1// PLATFORM_LINUX
	#if 0
	while (!kthread_should_stop()){
		printk("%s waiting for thread_stop notify \r\n", __func__);
		msleep(100);
	}
	#else
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		printk("%s waiting for thread_stop notify \r\n", __func__);
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	#endif
#endif
}

#ifdef CONFIG_OOB
static int rx_thread_wait_to = 1000;
module_param_named(rx_thread_wait_to, rx_thread_wait_to, int, 0644);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include "uapi/linux/sched/types.h"
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
#include "linux/sched/types.h"
#else
#include "linux/sched/rt.h"
#endif

int bustx_thread_prio = 1;
module_param(bustx_thread_prio, int, 0);
int busrx_thread_prio = 1;
module_param(busrx_thread_prio, int, 0);

int sdio_bustx_thread(void *data)
{
	struct aicwf_bus *bus = (struct aicwf_bus *) data;
	struct aic_sdio_dev *sdiodev = bus->bus_priv.sdio;
	if (bustx_thread_prio > 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
		sched_set_fifo_low(current);
#else
		struct sched_param param;
		param.sched_priority = (bustx_thread_prio < MAX_RT_PRIO) ? bustx_thread_prio : (MAX_RT_PRIO - 1);
		sched_setscheduler(current, SCHED_FIFO, &param);
#endif
	}

	while (1) {
		#if 0
		if (kthread_should_stop()) {
			sdio_err("sdio bustx thread stop\n");
			break;
		}
		#endif
		if (!wait_for_completion_interruptible(&bus->bustx_trgg)) {
			if (sdiodev->bus_if->state == BUS_DOWN_ST)
				break;

			rwnx_wakeup_lock(sdiodev->rwnx_hw->ws_tx);
			while ((int)(atomic_read(&sdiodev->tx_priv->tx_pktcnt) > 0) || (sdiodev->tx_priv->cmd_txstate == true)) {
				aicwf_sdio_tx_process(sdiodev);
				if (sdiodev->bus_if->state == BUS_DOWN_ST)
					break;
			}
			rwnx_wakeup_unlock(sdiodev->rwnx_hw->ws_tx);
		}
	}

	aic_thread_wait_stop();
	printk("%s Exit\r\n", __func__);

	return 0;
}

int sdio_busrx_thread(void *data)
{
	struct aicwf_rx_priv *rx_priv = (struct aicwf_rx_priv *)data;
	struct aicwf_bus *bus_if = rx_priv->sdiodev->bus_if;
	struct aic_sdio_dev *sdiodev = rx_priv->sdiodev;
	if (busrx_thread_prio > 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
		sched_set_fifo_low(current);
#else
		struct sched_param param;
		param.sched_priority = (busrx_thread_prio < MAX_RT_PRIO) ? busrx_thread_prio : (MAX_RT_PRIO - 1);
		sched_setscheduler(current, SCHED_FIFO, &param);
#endif
	}

	while (1) {
		#if 0
		if (kthread_should_stop()) {
			sdio_err("sdio busrx thread stop\n");
			break;
		}
		#endif
#ifndef CONFIG_OOB
		if (!wait_for_completion_interruptible(&bus_if->busrx_trgg)) {
#else
			if (!wait_for_completion_timeout(&bus_if->busrx_trgg, msecs_to_jiffies(rx_thread_wait_to))) {
				sdio_dbg("%s wait for completion timout \r\n", __func__);
			}
#endif
			if (bus_if->state == BUS_DOWN_ST)
				break;
#ifdef CONFIG_OOB
#ifdef CONFIG_SDIO_PWRCTRL
			while (atomic_read(&bus_if->bus_priv.sdio->is_bus_suspend) == 1) {
				sdio_dbg("%s waiting for sdio bus resume \r\n", __func__);
				msleep(100);
			}
			aicwf_sdio_pwr_stctl(bus_if->bus_priv.sdio, SDIO_ACTIVE_ST);
#endif // CONFIG_SDIO_PWRCTRL
			aicwf_sdio_hal_irqhandler(bus_if->bus_priv.sdio->func);
#endif // CONFIG_OOB
			rwnx_wakeup_lock(sdiodev->rwnx_hw->ws_rx);
			aicwf_process_rxframes(rx_priv);
			rwnx_wakeup_unlock(sdiodev->rwnx_hw->ws_rx);
#ifndef CONFIG_OOB
		}
#endif
	}

	aic_thread_wait_stop();
	printk("%s Exit\r\n", __func__);

	return 0;
}

#if defined(CONFIG_SDIO_PWRCTRL)
static int aicwf_sdio_pwrctl_thread(void *data)
{
	struct aic_sdio_dev *sdiodev = (struct aic_sdio_dev *) data;

	while (1) {
		if (kthread_should_stop()) {
			sdio_err("sdio pwrctl thread stop\n");
			break;
		}
		if (!wait_for_completion_interruptible(&sdiodev->pwrctrl_trgg)) {
			if (sdiodev->bus_if->state == BUS_DOWN_ST)
				continue;

			rwnx_wakeup_lock(sdiodev->rwnx_hw->ws_pwrctrl);
			if ((int)(atomic_read(&sdiodev->tx_priv->tx_pktcnt) <= 0) && (sdiodev->tx_priv->cmd_txstate == false) && \
					(int)(atomic_read(&sdiodev->rx_priv->rx_cnt) == 0))
				aicwf_sdio_pwr_stctl(sdiodev, SDIO_SLEEP_ST);
			else
				aicwf_sdio_pwrctl_timer(sdiodev, sdiodev->active_duration);
			rwnx_wakeup_unlock(sdiodev->rwnx_hw->ws_pwrctrl);
		}
	}

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
static void aicwf_sdio_bus_pwrctl(ulong data)
#else
static void aicwf_sdio_bus_pwrctl(struct timer_list *t)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
	struct aic_sdio_dev *sdiodev = (struct aic_sdio_dev *) data;
#else
	struct aic_sdio_dev *sdiodev = from_timer(sdiodev, t, timer);
#endif

	if (sdiodev->bus_if->state == BUS_DOWN_ST) {
		sdio_err("bus down\n");
		return;
	}

	if (sdiodev->pwrctl_tsk) {
		complete(&sdiodev->pwrctrl_trgg);
	}
}
#endif

#ifdef CONFIG_PREALLOC_RX_SKB
static void aicwf_sdio_enq_rxpkt(struct aic_sdio_dev *sdiodev, struct rx_buff *pkt)
#else
static void aicwf_sdio_enq_rxpkt(struct aic_sdio_dev *sdiodev, struct sk_buff *pkt)
#endif
{
	struct aicwf_rx_priv *rx_priv = sdiodev->rx_priv;
	unsigned long flags = 0;

	spin_lock_irqsave(&rx_priv->rxqlock, flags);
#ifdef CONFIG_PREALLOC_RX_SKB
	if (!aicwf_rxbuff_enqueue(sdiodev->dev, &rx_priv->rxq, pkt)) {
		spin_unlock_irqrestore(&rx_priv->rxqlock, flags);
		printk("%s %d, enqueue rxq fail\n", __func__, __LINE__);
		aicwf_prealloc_rxbuff_free(pkt, &rx_priv->rxbuff_lock);
		return;
	}
#else
	if (!aicwf_rxframe_enqueue(sdiodev->dev, &rx_priv->rxq, pkt)) {
		spin_unlock_irqrestore(&rx_priv->rxqlock, flags);
		aicwf_dev_skb_free(pkt);
		return;
	}
#endif
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
#ifdef CONFIG_PREALLOC_RX_SKB
	struct rx_buff *pkt = NULL;
#else
	struct sk_buff *pkt = NULL;
#endif
	int ret;
	int retry = 10;

	if ((sdiodev->rwnx_hw) == NULL) {
		sdio_err("waiting for rwnx_hw->irq_enable is true\r\n");
		return;
	}

	rwnx_wakeup_lock(sdiodev->rwnx_hw->ws_irqrx);

	if (!bus_if || bus_if->state == BUS_DOWN_ST) {
		sdio_err("bus err\n");
		rwnx_wakeup_unlock(sdiodev->rwnx_hw->ws_irqrx);
		return;
	}

#ifdef CONFIG_PREALLOC_RX_SKB
	if (list_empty(&aic_rx_buff_list.rxbuff_list)) {
		printk("%s %d, rxbuff list is empty\n", __func__, __LINE__);
		rwnx_wakeup_unlock(sdiodev->rwnx_hw->ws_irqrx);
		return;
	}
#endif

	if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D || sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DC ||
		sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800DW) {
		ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.block_cnt_reg, &intstatus);
		while (ret || (intstatus & SDIO_OTHER_INTERRUPT)) {
			sdio_err("ret=%d, intstatus=%x\r\n", ret, intstatus);
			ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.block_cnt_reg, &intstatus);
			if (retry-- <= 0)
				break;
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
		#ifndef CONFIG_PLATFORM_ALLWINNER
			sdio_err("Interrupt but no data\n");
		#endif
		}

		if (pkt)
			aicwf_sdio_enq_rxpkt(sdiodev, pkt);

		if (atomic_read(&sdiodev->rx_priv->rx_cnt) == 1 &&
			sdiodev->oob_enable == false){
			complete(&bus_if->busrx_trgg);
		}
	} else if (sdiodev->rwnx_hw->chipid == PRODUCT_ID_AIC8800D80) {
		do {
			ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.misc_int_status_reg, &intstatus);
			if (ret) {
				if (retry-- <= 0) {
					rwnx_wakeup_unlock(sdiodev->rwnx_hw->ws_irqrx);
					return;
				}
			} else
				break;
			sdio_err("ret=%d, intstatus=%x\r\n", ret, intstatus);
		} while (1);
		if (intstatus & SDIO_OTHER_INTERRUPT) {
			u8 int_pending;
			ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.sleep_reg, &int_pending);
			if (ret < 0) {
				sdio_err("reg:%d read failed!\n", sdiodev->sdio_reg.sleep_reg);
			}
			int_pending &= ~0x01; // dev to host soft irq
			ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.sleep_reg, int_pending);
			if (ret < 0) {
				sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.sleep_reg);
			}
		}

		if (intstatus > 0) {
			uint8_t intmaskf2 = intstatus | (0x1UL << 3);
			if (intmaskf2 > 120U) { // func2
				if (intmaskf2 == 127U) { // byte mode
					//aicwf_sdio_intr_get_len_bytemode(sdiodev, &byte_len, 1);//byte_len must<= 128
					aicwf_sdio_intr_get_len_bytemode(sdiodev, &byte_len);//byte_len must<= 128
					sdio_info("byte mode len=%d\r\n", byte_len);
					//pkt = aicwf_sdio_readframes(sdiodev, 1);
					pkt = aicwf_sdio_readframes(sdiodev);
				} else { // block mode
					sdiodev->rx_priv->data_len = (intstatus & 0x7U) * SDIOWIFI_FUNC_BLOCKSIZE;
					//pkt = aicwf_sdio_readframes(sdiodev, 1);
					pkt = aicwf_sdio_readframes(sdiodev);
				}
			} else { // func1
				if (intstatus == 120U) { // byte mode
					//aicwf_sdio_intr_get_len_bytemode(sdiodev, &byte_len, 0);//byte_len must<= 128
					aicwf_sdio_intr_get_len_bytemode(sdiodev, &byte_len);//byte_len must<= 128
					sdio_info("byte mode len=%d\r\n", byte_len);
					//pkt = aicwf_sdio_readframes(sdiodev, 0);
					pkt = aicwf_sdio_readframes(sdiodev);
				} else { // block mode
					sdiodev->rx_priv->data_len = (intstatus & 0x7FU) * SDIOWIFI_FUNC_BLOCKSIZE;
					//pkt = aicwf_sdio_readframes(sdiodev, 0);
					pkt = aicwf_sdio_readframes(sdiodev);
				}
			}
		} else {
		#ifndef CONFIG_PLATFORM_ALLWINNER
			sdio_err("Interrupt but no data\n");
		#endif
		}

		if (pkt)
			aicwf_sdio_enq_rxpkt(sdiodev, pkt);

		if (atomic_read(&sdiodev->rx_priv->rx_cnt) == 1 &&
			sdiodev->oob_enable == false){
			complete(&bus_if->busrx_trgg);
		}
	}
	rwnx_wakeup_unlock(sdiodev->rwnx_hw->ws_irqrx);
}

#if defined(CONFIG_SDIO_PWRCTRL)
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
#endif

static struct aicwf_bus_ops aicwf_sdio_bus_ops = {
	.stop = aicwf_sdio_bus_stop,
	.start = aicwf_sdio_bus_start,
	.txdata = aicwf_sdio_bus_txdata,
	.txmsg = aicwf_sdio_bus_txmsg,
};

void aicwf_sdio_release(struct aic_sdio_dev *sdiodev)
{
	struct aicwf_bus *bus_if;
#ifndef CONFIG_OOB
	int ret;
#endif
	sdio_dbg("%s\n", __func__);

	bus_if = dev_get_drvdata(sdiodev->dev);
	bus_if->state = BUS_DOWN_ST;

#ifndef CONFIG_OOB
	sdio_claim_host(sdiodev->func);
	//disable sdio interrupt
	ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.intr_config_reg, 0x0);
	if (ret < 0) {
		sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.intr_config_reg);
	}
	sdio_release_irq(sdiodev->func);
	sdio_release_host(sdiodev->func);
#endif

	if (sdiodev->dev)
		aicwf_bus_deinit(sdiodev->dev);

	if (sdiodev->tx_priv)
		aicwf_tx_deinit(sdiodev->tx_priv);

	if (sdiodev->rx_priv)
		aicwf_rx_deinit(sdiodev->rx_priv);

#if defined(CONFIG_SDIO_PWRCTRL)
	if (sdiodev->pwrctl_tsk) {
		complete_all(&sdiodev->pwrctrl_trgg);
		kthread_stop(sdiodev->pwrctl_tsk);
		sdiodev->pwrctl_tsk = NULL;
	}

	sdio_dbg("%s:pwrctl stopped\n", __func__);
#endif

	if (sdiodev->cmd_mgr.state == RWNX_CMD_MGR_STATE_INITED)
		rwnx_cmd_mgr_deinit(&sdiodev->cmd_mgr);
	sdio_dbg("exit %s\n", __func__);
}

void aicwf_sdio_reg_init(struct aic_sdio_dev *sdiodev)
{
	sdio_dbg("%s\n", __func__);

	if (aicwf_chipid == PRODUCT_ID_AIC8800D || aicwf_chipid == PRODUCT_ID_AIC8800DC ||
		aicwf_chipid == PRODUCT_ID_AIC8800DW) {
		sdiodev->sdio_reg.bytemode_len_reg    = SDIOWIFI_BYTEMODE_LEN_REG;
		sdiodev->sdio_reg.intr_config_reg     = SDIOWIFI_INTR_CONFIG_REG;
		sdiodev->sdio_reg.sleep_reg           = SDIOWIFI_SLEEP_REG;
		sdiodev->sdio_reg.wakeup_reg          = SDIOWIFI_WAKEUP_REG;
		sdiodev->sdio_reg.flow_ctrl_reg       = SDIOWIFI_FLOW_CTRL_REG;
		sdiodev->sdio_reg.register_block      = SDIOWIFI_REGISTER_BLOCK;
		sdiodev->sdio_reg.bytemode_enable_reg = SDIOWIFI_BYTEMODE_ENABLE_REG;
		sdiodev->sdio_reg.block_cnt_reg       = SDIOWIFI_BLOCK_CNT_REG;
		sdiodev->sdio_reg.rd_fifo_addr        = SDIOWIFI_RD_FIFO_ADDR;
		sdiodev->sdio_reg.wr_fifo_addr        = SDIOWIFI_WR_FIFO_ADDR;
	} else if (aicwf_chipid == PRODUCT_ID_AIC8800D80) {
		sdiodev->sdio_reg.bytemode_len_reg    = SDIOWIFI_BYTEMODE_LEN_REG_V3;
		sdiodev->sdio_reg.intr_config_reg     = SDIOWIFI_INTR_ENABLE_REG_V3;
		sdiodev->sdio_reg.sleep_reg           = SDIOWIFI_INTR_PENDING_REG_V3;
		sdiodev->sdio_reg.wakeup_reg          = SDIOWIFI_INTR_TO_DEVICE_REG_V3;
		sdiodev->sdio_reg.flow_ctrl_reg       = SDIOWIFI_FLOW_CTRL_Q1_REG_V3;
		sdiodev->sdio_reg.bytemode_enable_reg = SDIOWIFI_BYTEMODE_ENABLE_REG_V3;
		sdiodev->sdio_reg.misc_int_status_reg = SDIOWIFI_MISC_INT_STATUS_REG_V3;
		sdiodev->sdio_reg.rd_fifo_addr        = SDIOWIFI_RD_FIFO_ADDR_V3;
		sdiodev->sdio_reg.wr_fifo_addr        = SDIOWIFI_WR_FIFO_ADDR_V3;
	}
}

int aicwf_sdio_func_init(struct aic_sdio_dev *sdiodev)
{
	struct mmc_host *host;
	u8 block_bit0 = 0x1;
	u8 byte_mode_disable = 0x1;//1: no byte mode
	int ret = 0;
	struct aicbsp_feature_t feature;
	//u8 val = 0;

	feature = aicwf_feature;
	aicwf_sdio_reg_init(sdiodev);

	host = sdiodev->func->card->host;

	sdio_claim_host(sdiodev->func);
	sdiodev->func->card->quirks |= MMC_QUIRK_LENIENT_FN0;
	sdio_f0_writeb(sdiodev->func, feature.sdio_phase, 0x13, &ret);
	if (ret < 0) {
		sdio_err("write func0 fail %d\n", ret);
		sdio_release_host(sdiodev->func);
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
		return ret;
	}

	if (feature.sdio_clock > 0) {
		host->ios.clock = feature.sdio_clock;
		host->ops->set_ios(host, &host->ios);
		sdio_dbg("Set SDIO Clock %d MHz\n", host->ios.clock/1000000);
	}
	sdio_release_host(sdiodev->func);

	if (aicwf_chipid == PRODUCT_ID_AIC8800D || aicwf_chipid == PRODUCT_ID_AIC8800DC ||
			aicwf_chipid == PRODUCT_ID_AIC8800DW) {
		sdio_claim_host(sdiodev->func2);

		//set sdio blocksize
		ret = sdio_set_block_size(sdiodev->func2, SDIOWIFI_FUNC_BLOCKSIZE);
		if (ret < 0) {
			sdio_err("set func2 blocksize fail %d\n", ret);
			sdio_release_host(sdiodev->func2);
			return ret;
		}

		//set sdio enable func
		ret = sdio_enable_func(sdiodev->func2);
		if (ret < 0) {
			sdio_release_host(sdiodev->func2);
			sdio_err("enable func2 fail %d.\n", ret);
			return ret;
		}

		sdio_release_host(sdiodev->func2);

		ret = aicwf_sdio_func2_writeb(sdiodev, sdiodev->sdio_reg.register_block, block_bit0);
		if (ret < 0) {
			sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.register_block);
			return ret;
		}

		//1: no byte mode
		ret = aicwf_sdio_func2_writeb(sdiodev, sdiodev->sdio_reg.bytemode_enable_reg, byte_mode_disable);
		if (ret < 0) {
			sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.bytemode_enable_reg);
			return ret;
		}
	}

	if (aicwf_chipid != PRODUCT_ID_AIC8800D80) {
		ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.register_block, block_bit0);
		if (ret < 0) {
			sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.register_block);
			return ret;
		}
	}

	//1: no byte mode
	ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.bytemode_enable_reg, byte_mode_disable);
	if (ret < 0) {
		sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.bytemode_enable_reg);
		return ret;
	}

#if 0
	mdelay(5);
	ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.wakeup_reg, 1);
	if (ret < 0) {
		sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.wakeup_reg);
		return ret;
	}

	mdelay(5);
	ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.sleep_reg, &val);
	if (ret < 0) {
		sdio_err("reg:%d read failed!\n", sdiodev->sdio_reg.sleep_reg);
		return ret;
	}

	if (!(val & 0x10))
		sdio_dbg("wakeup fail\n");
	else
		sdio_info("sdio ready\n");
#else
	mdelay(10);
#endif
	return ret;
}

int aicwf_sdiov3_func_init(struct aic_sdio_dev *sdiodev)
{
	struct mmc_host *host;
	u8 byte_mode_disable = 0x1;//1: no byte mode
	int ret = 0;
	struct aicbsp_feature_t feature;
	u8 val1 = 0;

	feature = aicwf_feature;
	aicwf_sdio_reg_init(sdiodev);

	host = sdiodev->func->card->host;

	sdio_claim_host(sdiodev->func);
	sdiodev->func->card->quirks |= MMC_QUIRK_LENIENT_FN0;

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
		return ret;
	}

	if ((feature.sdio_clock > 0) && (host->ios.timing != MMC_TIMING_UHS_DDR50)) {
		host->ios.clock = feature.sdio_clock;
		host->ops->set_ios(host, &host->ios);
		sdio_info("Set SDIO Clock %d MHz\n", host->ios.clock/1000000);
	}

	sdio_release_host(sdiodev->func);

	//1: no byte mode
	ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.bytemode_enable_reg, byte_mode_disable);
	if (ret < 0) {
		sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.bytemode_enable_reg);
		return ret;
	}

	ret = aicwf_sdio_writeb(sdiodev, sdiodev->sdio_reg.wakeup_reg, 0x11);
	if (ret < 0) {
		sdio_err("reg:%d write failed!\n", sdiodev->sdio_reg.wakeup_reg);
		return ret;
	}

	mdelay(5);
	ret = aicwf_sdio_readb(sdiodev, sdiodev->sdio_reg.sleep_reg, &val1);
	if (ret < 0) {
		sdio_err("reg:%d read failed!\n", sdiodev->sdio_reg.sleep_reg);
		return ret;
	}

	if (!(val1 & 0x10)) {
		sdio_err("wakeup fail\n");
	} else {
		sdio_info("sdio ready\n");
	}
	return ret;
}

void aicwf_sdio_func_deinit(struct aic_sdio_dev *sdiodev)
{
	sdio_claim_host(sdiodev->func);
	sdio_disable_func(sdiodev->func);
	sdio_release_host(sdiodev->func);

	if (aicwf_chipid == PRODUCT_ID_AIC8800D || aicwf_chipid == PRODUCT_ID_AIC8800DC ||
			aicwf_chipid == PRODUCT_ID_AIC8800DW) {
		sdio_claim_host(sdiodev->func2);
		sdio_disable_func(sdiodev->func2);
		sdio_release_host(sdiodev->func2);
	}
}

void *aicwf_sdio_bus_init(struct aic_sdio_dev *sdiodev)
{
	int ret;
	struct aicwf_bus *bus_if;
	struct aicwf_rx_priv *rx_priv;
	struct aicwf_tx_priv *tx_priv;

#if defined(CONFIG_SDIO_PWRCTRL)
	spin_lock_init(&sdiodev->pwrctl_lock);
	sema_init(&sdiodev->pwrctl_wakeup_sema, 1);
#endif

	bus_if = sdiodev->bus_if;
	bus_if->dev = sdiodev->dev;
	bus_if->ops = &aicwf_sdio_bus_ops;
	bus_if->state = BUS_DOWN_ST;
#if defined(CONFIG_SDIO_PWRCTRL)
	sdiodev->state = SDIO_SLEEP_ST;
	sdiodev->active_duration = SDIOWIFI_PWR_CTRL_INTERVAL;
#else
	sdiodev->state = SDIO_ACTIVE_ST;
#endif

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

#if defined(CONFIG_SDIO_PWRCTRL)
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
	init_timer(&sdiodev->timer);
	sdiodev->timer.data = (ulong) sdiodev;
	sdiodev->timer.function = aicwf_sdio_bus_pwrctl;
#else
	timer_setup(&sdiodev->timer, aicwf_sdio_bus_pwrctl, 0);
#endif
	init_completion(&sdiodev->pwrctrl_trgg);
#ifdef AICWF_SDIO_SUPPORT
	sdiodev->pwrctl_tsk = kthread_run(aicwf_sdio_pwrctl_thread, sdiodev, "aicwf_pwrctl");
#endif
	if (IS_ERR(sdiodev->pwrctl_tsk)) {
		sdiodev->pwrctl_tsk = NULL;
	}
#endif

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

uint8_t crc8_ponl_107(uint8_t *p_buffer, uint16_t cal_size)
{
	uint8_t i;
	uint8_t crc = 0;
	if (cal_size == 0) {
		return crc;
	}
	while (cal_size--) {
		for (i = 0x80; i > 0; i /= 2) {
			if (crc & 0x80)  {
				crc *= 2;
				crc ^= 0x07; //polynomial X8 + X2 + X + 1,(0x107)
			} else {
				crc *= 2;
			}
			if ((*p_buffer) & i) {
				crc ^= 0x07;
			}
		}
		p_buffer++;
	}

	return crc;
}

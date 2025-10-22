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
#include "aic_bsp_txrxif.h"
#include "aicsdio.h"
#include "aic_bsp_driver.h"
#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#ifdef CONFIG_PLATFORM_ALLWINNER
extern void sunxi_mmc_rescan_card(unsigned ids);
extern void sunxi_wlan_set_power(int on);
extern int  sunxi_wlan_get_bus_index(void);
static int  aicbsp_bus_index = -1;
#endif
static int  aicbsp_platform_power_on(void);
static void aicbsp_platform_power_off(void);
static void aicbsp_sdio_exit(void);
static int  aicbsp_sdio_init(void);
static void aicwf_sdio_hal_irqhandler(struct sdio_func *func);
static void aicwf_sdio_hal_irqhandler_func2(struct sdio_func *func);
#if defined(CONFIG_SDIO_PWRCTRL)
static void aicwf_sdio_pwrctl_timer(struct priv_dev *aicdev, uint duration);
#endif
static void aicwf_sdio_reg_init(struct priv_dev *aicdev);
static int  aicwf_sdio_func_init(struct priv_dev *aicdev);
static int  aicwf_sdiov3_func_init(struct priv_dev *aicdev);
static void aicwf_sdio_func_deinit(struct priv_dev *aicdev);
static void *aicwf_sdio_bus_init(struct priv_dev *aicdev);
static void aicwf_sdio_release(struct priv_dev *aicdev);
static void aicbsp_sdio_release(struct priv_dev *aicdev);
static int  aicwf_sdio_aggr(struct aicwf_tx_priv *tx_priv, struct sk_buff *pkt);
static int  aicwf_sdio_send(struct aicwf_tx_priv *tx_priv);
static void aicwf_sdio_aggr_send(struct aicwf_tx_priv *tx_priv);
static void aicwf_sdio_aggrbuf_reset(struct aicwf_tx_priv *tx_priv);

static struct priv_dev *sdiodev;
static struct semaphore *aicbsp_notify_semaphore;
static const struct sdio_device_id aicbsp_sdmmc_ids[];

#ifdef CONFIG_SDIO_F1_FLAG
extern bool sdio_f1_flag;
#endif

int aicbsp_device_init(void)
{
	return 0;
}

void aicbsp_device_exit(void)
{
	aicbsp_set_subsys(AIC_BLUETOOTH, AIC_PWR_OFF);
	aicbsp_set_subsys(AIC_WIFI, AIC_PWR_OFF);
}

static struct device_match_entry aicdev_match_table[] = {
	{0x544a, 0x0146, PRODUCT_ID_AIC8800D,   "aic8800d",   0, 0, 0, 0, 0},                 // 8800d in bootloader mode
	{0xc8a1, 0xc18d, PRODUCT_ID_AIC8800DC,  "aic8800dc",  0, 0, 0, AICBT_PCM_SUPPORT, 0}, // 8800dc in bootloader mode
//	{0x5449, 0x0145, PRODUCT_ID_AIC8800DW,  "aic8800dw",  0, 0, 0, AICBT_PCM_SUPPORT, 0},
	{0xc8a1, 0x0182, PRODUCT_ID_AIC8800D80, "aic8800d80", 0, 0, 0, AICBT_PCM_SUPPORT, 0}, // 8800d80 in bootloader mode
};

static struct device_match_entry *aic_matched_ic;

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

	bsp_dbg("%s, subsys: %s, state to: %d\n", __func__, aicbsp_subsys_name(subsys), state);

	if (cur_power_state != pre_power_state) {
		bsp_dbg("%s, power state change to %d dure to %s\n", __func__, cur_power_state, aicbsp_subsys_name(subsys));
		if (cur_power_state) {
			if (aicbsp_platform_power_on() < 0)
				goto err0;
			if (aicbsp_sdio_init())
				goto err1;
			if (!sdiodev)
				goto err2;
			if (aicbsp_driver_fw_init(sdiodev))
				goto err3;
			aicbsp_sdio_release(sdiodev);
		} else {
			aicbsp_sdio_exit();
			aicbsp_platform_power_off();
		}
	} else {
		bsp_dbg("%s, power state no need to change, current: %d\n", __func__, cur_power_state);
	}
	pre_power_map = cur_power_map;
	mutex_unlock(&aicbsp_power_lock);

	return cur_power_state;

err3:
	aicbsp_sdio_release(sdiodev);

err2:
	aicbsp_sdio_exit();

err1:
	aicbsp_platform_power_off();

err0:
	bsp_dbg("%s, fail to set %s power state to %d\n", __func__, aicbsp_subsys_name(subsys), state);
	mutex_unlock(&aicbsp_power_lock);
	return -1;
}
EXPORT_SYMBOL_GPL(aicbsp_set_subsys);

void *aicbsp_get_drvdata(void *args)
{
	(void)args;
	if (sdiodev)
		return sdiodev->bus_if;
	return dev_get_drvdata((const struct device *)args);
}

static int aicbsp_sdio_probe(struct sdio_func *func,
	const struct sdio_device_id *id)
{
	struct mmc_host *host;
	struct priv_dev *aicdev;
	struct aicwf_bus *bus_if;
	int err = -ENODEV;
	int i = 0;

	bsp_dbg("%s:%d\n", __func__, func->num);
	for (i = 0; i < sizeof(aicdev_match_table) / sizeof(aicdev_match_table[0]); i++) {
		if (func->vendor == aicdev_match_table[i].vid && func->device == aicdev_match_table[i].pid) {
			aic_matched_ic = &aicdev_match_table[i];
		}
	}

	if (aic_matched_ic == NULL)
		return err;

	bsp_dbg("%s:%d, matched chip: %s\n", __func__, func->num, aic_matched_ic ? aic_matched_ic->name : "none");
	if (func->num != 2)
			return err;

	func = func->card->sdio_func[1 - 1]; //replace 2 with 1
	host = func->card->host;
	bsp_dbg("%s after replace:%d\n", __func__, func->num);

	bus_if = kzalloc(sizeof(struct aicwf_bus), GFP_KERNEL);
	if (!bus_if) {
		bsp_err("alloc bus fail\n");
		return -ENOMEM;
	}

	aicdev = kzalloc(sizeof(struct priv_dev), GFP_KERNEL);
	if (!aicdev) {
		bsp_err("alloc aicdev fail\n");
		kfree(bus_if);
		return -ENOMEM;
	}

	aicbsp_info.chipinfo = aic_matched_ic;
	aicdev->func[0] = func;
	aicdev->bus_if = bus_if;
	bus_if->bus_priv.dev = aicdev;

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC) {
		aicdev->func[1] = func->card->sdio_func[1];
		dev_set_drvdata(&aicdev->func[1]->dev, bus_if);
	}

	dev_set_drvdata(&func->dev, bus_if);
	aicdev->dev = &func->dev;

	bsp_dbg("%s host max clock frequecy: %d\n", __func__, host->f_max);
	if (aicbsp_info.chipinfo->chipid != PRODUCT_ID_AIC8800D80) {
		if (aicbsp_info.sdio_clock < 0)
			aicbsp_info.sdio_clock = min(host->f_max, AIC_SDIO_V2_CLOCK);
		if (aicbsp_info.sdio_phase < 0)
			aicbsp_info.sdio_phase = AIC_SDIO_V2_PHASE;
		err = aicwf_sdio_func_init(aicdev);
	} else {
		if (aicbsp_info.sdio_clock < 0)
			aicbsp_info.sdio_clock = min(host->f_max, AIC_SDIO_V3_CLOCK);
		if (aicbsp_info.sdio_phase < 0)
			aicbsp_info.sdio_phase = AIC_SDIO_V3_PHASE;
		err = aicwf_sdiov3_func_init(aicdev);
	}

	aicbsp_info.btpcm = (aicbsp_info.chipinfo->feature & AICBT_PCM_SUPPORT) ? 1 : 0;
	if (err < 0) {
		bsp_err("sdio func init fail\n");
		goto fail;
	}

	if (aicwf_sdio_bus_init(aicdev) == NULL) {
		bsp_err("sdio bus init err\r\n");
		err = -1;
		goto fail;
	}
	host->caps |= MMC_CAP_NONREMOVABLE;
	aicbsp_platform_init(aicdev);
	sdiodev = aicdev;

	return 0;
fail:
	aicwf_sdio_func_deinit(aicdev);
	dev_set_drvdata(&func->dev, NULL);
	kfree(aicdev);
	kfree(bus_if);
	aic_matched_ic = NULL;
	return err;
}

static void aicbsp_sdio_remove(struct sdio_func *func)
{
	struct mmc_host *host;
	struct aicwf_bus *bus_if = NULL;
	struct priv_dev *aicdev = NULL;

	bsp_dbg("%s\n", __func__);
	bus_if = aicbsp_get_drvdata(&func->dev);
	if (!bus_if) {
		bsp_dbg("%s: allready unregister\n", __func__);
		goto done;
	}

	aicdev = bus_if->bus_priv.dev;
	if (!aicdev) {
		goto done;
	}

	func = aicdev->func[0];
	host = func->card->host;
	host->caps &= ~MMC_CAP_NONREMOVABLE;

	aicwf_sdio_release(aicdev);
	aicwf_sdio_func_deinit(aicdev);

	dev_set_drvdata(&aicdev->func[0]->dev, NULL);
	kfree(aicdev);
	kfree(bus_if);

done:
	sdiodev = NULL;
	aic_matched_ic = NULL;
	bsp_dbg("%s done\n", __func__);
}

#if IS_ENABLED(CONFIG_PM)
static int aicbsp_sdio_suspend(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	int err;
	mmc_pm_flag_t sdio_flags;
	bsp_dbg("%s, func->num = %d\n", __func__, func->num);
	if (func->num != 2)
		return 0;

	sdio_flags = sdio_get_host_pm_caps(func);
	if (!(sdio_flags & MMC_PM_KEEP_POWER)) {
		bsp_dbg("%s: can't keep power while host is suspended\n", __func__);
		return  -EINVAL;
	}

	/* keep power while host suspended */
	err = sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
	if (err) {
		bsp_dbg("%s: error while trying to keep power\n", __func__);
		return err;
	}

	return 0;
}

static int aicbsp_sdio_resume(struct device *dev)
{
	bsp_dbg("%s\n", __func__);
	return 0;
}

static const struct dev_pm_ops aicbsp_sdio_pm_ops = {
	.suspend = aicbsp_sdio_suspend,
	.resume = aicbsp_sdio_resume,
};
#endif  /* CONFIG_PM */

static const struct sdio_device_id aicbsp_sdmmc_ids[] = {
	{SDIO_DEVICE(0x544a, 0x0146)}, // aic8800d in bootloader mode
	{SDIO_DEVICE(0xc8a1, 0xc18d)}, // aic8800dc in bootloader mode
	{SDIO_DEVICE(0xc8a1, 0x0182)}, // aic8800d80 in bootloader mode
	{ },
};

MODULE_DEVICE_TABLE(sdio, aicbsp_sdmmc_ids);

static struct sdio_driver aicbsp_sdio_driver = {
	.probe = aicbsp_sdio_probe,
	.remove = aicbsp_sdio_remove,
	.name = AICBSP_SDIO_NAME,
	.id_table = aicbsp_sdmmc_ids,
#if IS_ENABLED(CONFIG_PM)
	.drv = {
		.pm = &aicbsp_sdio_pm_ops,
	},
#endif  /* CONFIG_PM */
};

static int aicbsp_platform_power_on(void)
{
#ifdef CONFIG_PLATFORM_ALLWINNER
	char   type_name[20];
	struct device_node *np_node;
	struct mmc_host *host;
	void  *drv_data;
	struct platform_device *pdev;

	int ret = 0;
	struct semaphore aic_chipup_sem;
	bsp_dbg("%s\n", __func__);
	if (aicbsp_bus_index < 0)
		 aicbsp_bus_index = sunxi_wlan_get_bus_index();
	if (aicbsp_bus_index < 0)
		return aicbsp_bus_index;

	sprintf(type_name, "sdc%d", aicbsp_bus_index);
	np_node = of_find_node_by_type(NULL, type_name);
	if (np_node == NULL) {
		bsp_err("%s get sdhci-name failed\n", __func__);
		return -1;
	}

	pdev = of_find_device_by_node(np_node);
	if (pdev == NULL) {
		bsp_err("%s sdio dev get platform device failed!!!\n", __func__);
		return -1;
	}

	drv_data = platform_get_drvdata(pdev);
	if (drv_data == NULL) {
		bsp_err("sdio dev get drv data failed!!!\n");
		return -1;
	}

	host = drv_data;
	if (*(host->private) != (unsigned long)host)
		host = container_of(drv_data, struct mmc_host, private);

	bsp_dbg("%s disable MMC_CAP_UHS_DDR50 for host %p\n", __func__, host);
	host->caps &= ~MMC_CAP_UHS_DDR50;

	sema_init(&aic_chipup_sem, 0);
	ret = aicbsp_reg_sdio_notify(&aic_chipup_sem);
	if (ret) {
		bsp_dbg("%s aicbsp_reg_sdio_notify fail(%d)\n", __func__, ret);
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
#else
	return 0;
#endif
}

static void aicbsp_platform_power_off(void)
{
#ifdef CONFIG_PLATFORM_ALLWINNER
	if (aicbsp_bus_index < 0)
		 aicbsp_bus_index = sunxi_wlan_get_bus_index();
	if (aicbsp_bus_index < 0) {
		bsp_dbg("no aicbsp_bus_index\n");
		return;
	}
	sunxi_wlan_set_power(0);
	mdelay(100);
	sunxi_mmc_rescan_card(aicbsp_bus_index);
#endif
	bsp_dbg("%s\n", __func__);
}

static int aicbsp_sdio_init(void)
{
	if (sdio_register_driver(&aicbsp_sdio_driver))
		return -1;

	return 0;
}

static void aicbsp_sdio_exit(void)
{
	sdio_unregister_driver(&aicbsp_sdio_driver);
}

static void aicbsp_sdio_release(struct priv_dev *aicdev)
{
	int funcnum = 1, i = 0;

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC)
		funcnum = 2;

	aicdev->bus_if->state = BUS_DOWN_ST;
	for (i = 0; i < funcnum; i++) {
		sdio_claim_host(aicdev->func[i]);
		sdio_release_irq(aicdev->func[i]);
		sdio_release_host(aicdev->func[i]);
	}
}

static int aicwf_sdio_readb(struct sdio_func *func, uint regaddr, u8 *val)
{
	int ret;
	sdio_claim_host(func);
	*val = sdio_readb(func, regaddr, &ret);
	sdio_release_host(func);
	return ret;
}

int aicwf_sdio_writeb(struct sdio_func *func, uint regaddr, u8 val)
{
	int ret;
	sdio_claim_host(func);
	sdio_writeb(func, val, regaddr, &ret);
	sdio_release_host(func);
	return ret;
}

static int aicwf_sdio_flow_ctrl(struct priv_dev *aicdev)
{
	int ret = -1;
	u8 fc_reg = 0;
	u32 count = 0;

	while (true) {
		ret = aicwf_sdio_readb(aicdev->func[0], aicdev->sdio_reg.flow_ctrl_reg, &fc_reg);
		if (ret) {
			return -1;
		}

		if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D || aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC ||
			aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DW) {
			fc_reg = fc_reg & SDIOWIFI_FLOWCTRL_MASK_REG;
		}

		if (fc_reg != 0) {
			ret = fc_reg;
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

static int aicwf_sdio_send_pkt(struct priv_dev *aicdev, u8 *buf, uint count)
{
	int ret = 0;

	sdio_claim_host(aicdev->func[0]);
	ret = sdio_writesb(aicdev->func[0], aicdev->sdio_reg.wr_fifo_addr, buf, count);
	sdio_release_host(aicdev->func[0]);

	return ret;
}

static int aicwf_sdio_send_msg(struct priv_dev *aicdev, u8 *buf, uint count)
{
	int ret = 0;

	sdio_claim_host(aicdev->func[1]);
	ret = sdio_writesb(aicdev->func[1], aicdev->sdio_reg.wr_fifo_addr, buf, count);
	sdio_release_host(aicdev->func[1]);

	return ret;
}

static int aicwf_sdio_recv_pkt(struct priv_dev *aicdev, struct sk_buff *skbbuf,
	u32 size)
{
	int ret;

	if ((!skbbuf) || (!size)) {
		return -EINVAL;;
	}

	sdio_claim_host(aicdev->func[0]);
	ret = sdio_readsb(aicdev->func[0], skbbuf->data, aicdev->sdio_reg.rd_fifo_addr, size);
	sdio_release_host(aicdev->func[0]);

	if (ret < 0) {
		return ret;
	}
	skbbuf->len = size;

	return ret;
}

static int aicwf_sdio_recv_msg(struct priv_dev *aicdev, struct sk_buff *skbbuf,
	u32 size)
{
	int ret;

	if ((!skbbuf) || (!size)) {
		return -EINVAL;;
	}

	sdio_claim_host(aicdev->func[1]);
	ret = sdio_readsb(aicdev->func[1], skbbuf->data, aicdev->sdio_reg.rd_fifo_addr, size);
	sdio_release_host(aicdev->func[1]);

	if (ret < 0) {
		return ret;
	}
	skbbuf->len = size;

	return ret;
}

#if defined(CONFIG_SDIO_PWRCTRL)
static int aicwf_sdio_wakeup(struct priv_dev *aicdev)
{
	int ret = 0;
	int read_retry;
	int write_retry = 20;
	int wakeup_reg_val;

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D ||
		aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC ||
		aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DW) {
		wakeup_reg_val = 1;
	} else if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D80) {
		wakeup_reg_val = 0x11;
	}

	if (aicdev->state == SDIO_SLEEP_ST) {
		down(&aicdev->pwrctl_wakeup_sema);
		while (write_retry) {
			ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.wakeup_reg, wakeup_reg_val);
			if (ret) {
				bsp_err("sdio wakeup fail\n");
				ret = -1;
			} else {
				read_retry = 10;
				while (read_retry) {
					u8 val;
					ret = aicwf_sdio_readb(aicdev->func[0], aicdev->sdio_reg.sleep_reg, &val);
					if ((ret == 0) && (val & 0x10)) {
						break;
					}
					read_retry--;
					udelay(200);
				}
				if (read_retry != 0)
					break;
			}
			bsp_dbg("write retry:  %d \n", write_retry);
			write_retry--;
			udelay(100);
		}
		up(&aicdev->pwrctl_wakeup_sema);
	}

	aicdev->state = SDIO_ACTIVE_ST;
	aicwf_sdio_pwrctl_timer(aicdev, aicdev->active_duration);

	return ret;
}

static int aicwf_sdio_sleep_allow(struct priv_dev *aicdev)
{
	int ret = 0;
	struct aicwf_bus *bus_if = aicdev->bus_if;

	if (bus_if->state == BUS_DOWN_ST) {
		ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.sleep_reg, 0x10);
		if (ret) {
			bsp_err("Write sleep fail!\n");
	}
		aicwf_sdio_pwrctl_timer(aicdev, 0);
		return ret;
	}

	if (aicdev->state == SDIO_ACTIVE_ST) {
		{
			bsp_dbg("s\n");
			ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.sleep_reg, 0x10);
			if (ret)
				bsp_err("Write sleep fail!\n");
		}
		aicdev->state = SDIO_SLEEP_ST;
		aicwf_sdio_pwrctl_timer(aicdev, 0);
	} else {
		aicwf_sdio_pwrctl_timer(aicdev, aicdev->active_duration);
	}

	return ret;
}

int aicwf_sdio_pwr_stctl(struct priv_dev *aicdev, uint target)
{
	int ret = 0;

	if (aicdev->bus_if->state == BUS_DOWN_ST) {
		return -1;
	}

	if (aicdev->state == target) {
		if (target == SDIO_ACTIVE_ST) {
			aicwf_sdio_pwrctl_timer(aicdev, aicdev->active_duration);
		}
		return ret;
	}

	switch (target) {
	case SDIO_ACTIVE_ST:
		aicwf_sdio_wakeup(aicdev);
		break;
	case SDIO_SLEEP_ST:
		aicwf_sdio_sleep_allow(aicdev);
		break;
	}

	return ret;
}
#endif

static int aicwf_sdio_txpkt(struct priv_dev *aicdev, struct sk_buff *pkt)
{
	int ret = 0;
	u8 *frame;
	u32 len = 0;
	struct aicwf_bus *bus_if = dev_get_drvdata(aicdev->dev);

	if (bus_if->state == BUS_DOWN_ST) {
		bsp_dbg("tx bus is down!\n");
		return -EINVAL;
	}

	frame = (u8 *) (pkt->data);
	len = pkt->len;
	len = (len + SDIOWIFI_FUNC_BLOCKSIZE - 1) / SDIOWIFI_FUNC_BLOCKSIZE * SDIOWIFI_FUNC_BLOCKSIZE;
	ret = aicwf_sdio_send_pkt(aicdev, pkt->data, len);
	if (ret)
		bsp_err("aicwf_sdio_send_pkt fail%d\n", ret);

	return ret;
}

static int aicwf_sdio_intr_get_len_bytemode(struct priv_dev *aicdev, u8 *byte_len)
{
	int ret = 0;

	if (!byte_len)
		return -EBADE;

	if (aicdev->bus_if->state == BUS_DOWN_ST) {
		*byte_len = 0;
	} else {
		ret = aicwf_sdio_readb(aicdev->func[0], aicdev->sdio_reg.bytemode_len_reg, byte_len);
		aicdev->rx_priv->data_len = (*byte_len)*4;
	}

	return ret;
}

static void aicwf_sdio_bus_stop(struct device *dev)
{
	struct aicwf_bus *bus_if = aicbsp_get_drvdata(dev);
	struct priv_dev *aicdev = bus_if->bus_priv.dev;
	int ret = 0;

#if defined(CONFIG_SDIO_PWRCTRL)
	aicwf_sdio_pwrctl_timer(aicdev, 0);
	bsp_dbg("%s\n", __func__);
	if (aicdev->pwrctl_tsk) {
		complete(&aicdev->pwrctrl_trgg);
		kthread_stop(aicdev->pwrctl_tsk);
		aicdev->pwrctl_tsk = NULL;
	}
#endif
	bus_if->state = BUS_DOWN_ST;
	if (aicdev->tx_priv) {
		ret = down_interruptible(&aicdev->tx_priv->txctl_sema);
		if (ret)
		   bsp_err("down txctl_sema fail\n");
	}

#if defined(CONFIG_SDIO_PWRCTRL)
	aicwf_sdio_pwr_stctl(aicdev, SDIO_SLEEP_ST);
#endif
	if (aicdev->tx_priv) {
		if (!ret)
			up(&aicdev->tx_priv->txctl_sema);
		aicwf_frame_queue_flush(&aicdev->tx_priv->txq);
	}
}

struct sk_buff *aicwf_sdio_readframes(struct priv_dev *aicdev)
{
	int ret = 0;
	u32 size = 0;
	struct sk_buff *skb = NULL;
	struct aicwf_bus *bus_if = dev_get_drvdata(aicdev->dev);

	if (bus_if->state == BUS_DOWN_ST) {
		bsp_dbg("bus down\n");
		return NULL;
	}

	size = aicdev->rx_priv->data_len;
	skb =  __dev_alloc_skb(size, GFP_KERNEL);
	if (!skb) {
		return NULL;
	}

	ret = aicwf_sdio_recv_pkt(aicdev, skb, size);
	if (ret) {
		dev_kfree_skb(skb);
		skb = NULL;
	}

	return skb;
}

static struct sk_buff *aicwf_sdio_readmessages(struct priv_dev *aicdev)
{
	int ret = 0;
	u32 size = 0;
	struct sk_buff *skb = NULL;
	struct aicwf_bus *bus_if = dev_get_drvdata(aicdev->dev);

	if (bus_if->state == BUS_DOWN_ST) {
		bsp_dbg("bus down\n");
		return NULL;
	}

	size = aicdev->rx_priv->data_len;
	skb =  __dev_alloc_skb(size, GFP_KERNEL);
	if (!skb) {
		return NULL;
	}

	ret = aicwf_sdio_recv_msg(aicdev, skb, size);
	if (ret) {
		dev_kfree_skb(skb);
		skb = NULL;
	}

	return skb;
}

static int aicwf_sdio_tx_msg(struct priv_dev *aicdev)
{
	int err = 0;
	u16 len;
	u8 *payload = aicdev->tx_priv->cmd_buf;
	u16 payload_len = aicdev->tx_priv->cmd_len;
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

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D || aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D80) {
		buffer_cnt = aicwf_sdio_flow_ctrl(aicdev);
		while ((buffer_cnt <= 0 || len > (buffer_cnt * BUFFER_SIZE)) && retry < 5) {
			retry++;
			buffer_cnt = aicwf_sdio_flow_ctrl(aicdev);
		}
	}
	down(&aicdev->tx_priv->cmd_txsema);

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D || aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D80) {
		if (buffer_cnt > 0 && len < (buffer_cnt * BUFFER_SIZE)) {
			err = aicwf_sdio_send_pkt(aicdev, payload, len);
			if (err) {
				bsp_err("aicwf_sdio_send_pkt fail%d\n", err);
			}
		} else {
			up(&aicdev->tx_priv->cmd_txsema);
			bsp_err("tx msg fc retry fail\n");
			return -1;
		}
	} else if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC) {
		err = aicwf_sdio_send_msg(aicdev, payload, len);
		if (err) {
			bsp_err("aicwf_sdio_send_pkt fail%d\n", err);
		}
	}

	aicdev->tx_priv->cmd_txstate = false;
	if (!err)
		aicdev->tx_priv->cmd_tx_succ = true;
	else
		aicdev->tx_priv->cmd_tx_succ = false;

	up(&aicdev->tx_priv->cmd_txsema);

	return err;
}

static void aicwf_sdio_tx_process(struct priv_dev *aicdev)
{
	int err = 0;

	if (aicdev->bus_if->state == BUS_DOWN_ST) {
		bsp_err("Bus is down\n");
		return;
	}
#if defined(CONFIG_SDIO_PWRCTRL)
	aicwf_sdio_pwr_stctl(aicdev, SDIO_ACTIVE_ST);
#endif

	//config
	bsp_info("send cmd\n");
	if (aicdev->tx_priv->cmd_txstate) {
		if (down_interruptible(&aicdev->tx_priv->txctl_sema)) {
			bsp_err("txctl down bus->txctl_sema fail\n");
			return;
		}
		if (aicdev->state != SDIO_ACTIVE_ST) {
			bsp_err("state err\n");
			up(&aicdev->tx_priv->txctl_sema);
			bsp_err("txctl up bus->txctl_sema fail\n");
			return;
		}

		err = aicwf_sdio_tx_msg(aicdev);
		up(&aicdev->tx_priv->txctl_sema);
		if (waitqueue_active(&aicdev->tx_priv->cmd_txdone_wait))
			wake_up(&aicdev->tx_priv->cmd_txdone_wait);
	}

	//data
	bsp_info("send data\n");
	if (down_interruptible(&aicdev->tx_priv->txctl_sema)) {
		bsp_err("txdata down bus->txctl_sema\n");
		return;
	}

	if (aicdev->state != SDIO_ACTIVE_ST) {
		bsp_err("sdio state err\n");
		up(&aicdev->tx_priv->txctl_sema);
		return;
	}

	if (!aicwf_is_framequeue_empty(&aicdev->tx_priv->txq))
		aicdev->tx_priv->fw_avail_bufcnt = aicwf_sdio_flow_ctrl(aicdev);

	while (!aicwf_is_framequeue_empty(&aicdev->tx_priv->txq)) {
		aicwf_sdio_send(aicdev->tx_priv);
		if (aicdev->tx_priv->cmd_txstate)
			break;
	}

	up(&aicdev->tx_priv->txctl_sema);
}

static int aicwf_sdio_bus_txdata(struct device *dev, struct sk_buff *pkt)
{
	uint prio;
	int ret = -EBADE;
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct priv_dev *aicdev = bus_if->bus_priv.dev;

	prio = (pkt->priority & 0x7);
	spin_lock_bh(&aicdev->tx_priv->txqlock);
	if (!aicwf_frame_enq(aicdev->dev, &aicdev->tx_priv->txq, pkt, prio)) {
		spin_unlock_bh(&aicdev->tx_priv->txqlock);
		return -ENOSR;
	} else {
		ret = 0;
	}

	if (bus_if->state != BUS_UP_ST) {
		bsp_err("bus_if stopped\n");
		spin_unlock_bh(&aicdev->tx_priv->txqlock);
		return -1;
	}

	atomic_inc(&aicdev->tx_priv->tx_pktcnt);
	spin_unlock_bh(&aicdev->tx_priv->txqlock);
	complete(&bus_if->bustx_trgg);

	return ret;
}

static int aicwf_sdio_bus_txmsg(struct device *dev, u8 *msg, uint msglen)
{
	int ret = -1;
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct priv_dev *aicdev = bus_if->bus_priv.dev;

	down(&aicdev->tx_priv->cmd_txsema);
	aicdev->tx_priv->cmd_txstate = true;
	aicdev->tx_priv->cmd_tx_succ = false;
	aicdev->tx_priv->cmd_buf = msg;
	aicdev->tx_priv->cmd_len = msglen;
	up(&aicdev->tx_priv->cmd_txsema);

	if (bus_if->state != BUS_UP_ST) {
		bsp_err("bus has stop\n");
		return -1;
	}

	complete(&bus_if->bustx_trgg);

	if (aicdev->tx_priv->cmd_txstate) {
		int timeout = msecs_to_jiffies(CMD_TX_TIMEOUT);
		ret = wait_event_timeout(aicdev->tx_priv->cmd_txdone_wait, \
											!(aicdev->tx_priv->cmd_txstate), timeout);
	}

	if (!aicdev->tx_priv->cmd_txstate && aicdev->tx_priv->cmd_tx_succ) {
		ret = 0;
	} else {
		bsp_err("send faild:%d, %d,%x\n", aicdev->tx_priv->cmd_txstate, aicdev->tx_priv->cmd_tx_succ, ret);
		ret = -EIO;
	}

	return ret;
}

static int aicwf_sdio_send(struct aicwf_tx_priv *tx_priv)
{
	struct sk_buff *pkt;
	struct priv_dev *aicdev = tx_priv->aicdev;
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
		tx_priv->fw_avail_bufcnt = aicwf_sdio_flow_ctrl(aicdev);
		while (tx_priv->fw_avail_bufcnt <= 0 && retry_times < max_retry_times) {
			retry_times++;
			tx_priv->fw_avail_bufcnt = aicwf_sdio_flow_ctrl(aicdev);
		}
		if (tx_priv->fw_avail_bufcnt <= 0) {
			bsp_err("fc retry %d fail\n", tx_priv->fw_avail_bufcnt);
			goto done;
		}
	}

	if (atomic_read(&tx_priv->aggr_count) == tx_priv->fw_avail_bufcnt) {
		if (atomic_read(&tx_priv->aggr_count) > 0) {
			tx_priv->fw_avail_bufcnt -= atomic_read(&tx_priv->aggr_count);
			aicwf_sdio_aggr_send(tx_priv); //send and check the next pkt;
		}
	} else {
		spin_lock_bh(&aicdev->tx_priv->txqlock);
		pkt = aicwf_frame_dequeue(&aicdev->tx_priv->txq);
		if (pkt == NULL) {
			bsp_err("txq no pkt\n");
			spin_unlock_bh(&aicdev->tx_priv->txqlock);
			goto done;
		}
		atomic_dec(&aicdev->tx_priv->tx_pktcnt);
		spin_unlock_bh(&aicdev->tx_priv->txqlock);

		if (tx_priv == NULL || tx_priv->tail == NULL || pkt == NULL)
			bsp_err("null error\n");
		if (aicwf_sdio_aggr(tx_priv, pkt)) {
			aicwf_sdio_aggrbuf_reset(tx_priv);
			bsp_err("add aggr pkts failed!\n");
			goto done;
		}

		//when aggr finish or there is cmd to send, just send this aggr pkt to fw
		if ((int)atomic_read(&aicdev->tx_priv->tx_pktcnt) == 0 || aicdev->tx_priv->cmd_txstate) { //no more pkt send it!
			tx_priv->fw_avail_bufcnt -= atomic_read(&tx_priv->aggr_count);
			aicwf_sdio_aggr_send(tx_priv);
		} else
			goto done;
	}

done:
	return 0;
}

static int aicwf_sdio_aggr(struct aicwf_tx_priv *tx_priv, struct sk_buff *pkt)
{
	u8 *start_ptr = tx_priv->tail;
	u8 sdio_header[4];
	u8 adjust_str[4] = {0, 0, 0, 0};
	u16 curr_len = 0;
	int allign_len = 0;

	sdio_header[2] = 0x01; // data
	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D || aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC ||
		aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DW)
		sdio_header[3] = 0; //reserved
	else if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D80)
		sdio_header[3] = crc8_ponl_107(&sdio_header[0], 3); // crc8

	memcpy(tx_priv->tail, (u8 *)&sdio_header, sizeof(sdio_header));
	tx_priv->tail += sizeof(sdio_header);

	// word alignment
	curr_len = tx_priv->tail - tx_priv->head;
	if (curr_len & (TX_ALIGNMENT - 1)) {
		allign_len = roundup(curr_len, TX_ALIGNMENT)-curr_len;
		memcpy(tx_priv->tail, adjust_str, allign_len);
		tx_priv->tail += allign_len;
	}

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D || aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC ||
		aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D) {
		start_ptr[0] = ((tx_priv->tail - start_ptr - 4) & 0xff);
		start_ptr[1] = (((tx_priv->tail - start_ptr - 4)>>8) & 0x0f);
	}
	tx_priv->aggr_buf->dev = pkt->dev;

	consume_skb(pkt);
	atomic_inc(&tx_priv->aggr_count);
	return 0;
}

static void aicwf_sdio_aggr_send(struct aicwf_tx_priv *tx_priv)
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
	ret = aicwf_sdio_txpkt(tx_priv->aicdev, tx_buf);
	if (ret < 0) {
		bsp_err("fail to send aggr pkt!\n");
	}

	aicwf_sdio_aggrbuf_reset(tx_priv);
}

static void aicwf_sdio_aggrbuf_reset(struct aicwf_tx_priv *tx_priv)
{
	struct sk_buff *aggr_buf = tx_priv->aggr_buf;

	tx_priv->tail = tx_priv->head;
	aggr_buf->len = 0;
	atomic_set(&tx_priv->aggr_count, 0);
}

static int aicwf_sdio_bus_start(struct device *dev)
{
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct priv_dev *aicdev = bus_if->bus_priv.dev;
	int ret = 0;

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D) {
		sdio_claim_host(aicdev->func[0]);
		sdio_claim_irq(aicdev->func[0], aicwf_sdio_hal_irqhandler);
		//enable sdio interrupt
		ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.intr_config_reg, 0x07);
		if (ret != 0)
			bsp_err("intr register failed:%d\n", ret);
		sdio_release_host(aicdev->func[0]);
	} else if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC) {
		sdio_claim_host(aicdev->func[0]);

		//since we have func2 we don't register irq handler
		sdio_claim_irq(aicdev->func[0], NULL);
		sdio_claim_irq(aicdev->func[1], NULL);

		aicdev->func[0]->irq_handler = (sdio_irq_handler_t *)aicwf_sdio_hal_irqhandler;
		aicdev->func[1]->irq_handler = (sdio_irq_handler_t *)aicwf_sdio_hal_irqhandler_func2;
		sdio_release_host(aicdev->func[0]);

		//enable sdio interrupt
		ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.intr_config_reg, 0x07);

		if (ret != 0)
			bsp_err("intr register failed:%d\n", ret);

		//enable sdio interrupt
		ret = aicwf_sdio_writeb(aicdev->func[1], aicdev->sdio_reg.intr_config_reg, 0x07);

		if (ret != 0)
			bsp_err("func2 intr register failed:%d\n", ret);
	} else if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D80) {
		sdio_claim_host(aicdev->func[0]);
		sdio_claim_irq(aicdev->func[0], aicwf_sdio_hal_irqhandler);

		sdio_f0_writeb(aicdev->func[0], 0x07, 0x04, &ret);
		if (ret) {
			bsp_err("set func0 int en fail %d\n", ret);
		}
		sdio_release_host(aicdev->func[0]);
		//enable sdio interrupt
		ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.intr_config_reg, 0x07);
		if (ret != 0)
			bsp_err("intr register failed:%d\n", ret);
	}

	bus_if->state = BUS_UP_ST;

	return ret;
}

int aicwf_bustx_thread(void *data)
{
	struct aicwf_bus *bus = (struct aicwf_bus *) data;
	struct priv_dev *aicdev = bus->bus_priv.dev;

	while (1) {
		if (kthread_should_stop()) {
			bsp_err("sdio bustx thread stop\n");
			break;
		}
		if (!wait_for_completion_interruptible(&bus->bustx_trgg)) {
			if ((int)(atomic_read(&aicdev->tx_priv->tx_pktcnt) > 0) || (aicdev->tx_priv->cmd_txstate == true))
				aicwf_sdio_tx_process(aicdev);
		}
	}

	return 0;
}

int aicwf_busrx_thread(void *data)
{
	struct aicwf_rx_priv *rx_priv = (struct aicwf_rx_priv *)data;
	struct aicwf_bus *bus_if = rx_priv->aicdev->bus_if;

	while (1) {
		if (kthread_should_stop()) {
			bsp_err("sdio busrx thread stop\n");
			break;
		}
		if (!wait_for_completion_interruptible(&bus_if->busrx_trgg)) {
			aicwf_process_rxframes(rx_priv);
		}
	}

	return 0;
}

#if defined(CONFIG_SDIO_PWRCTRL)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
static void aicwf_sdio_bus_pwrctl(struct timer_list *t)
{
	struct priv_dev *aicdev = from_timer(aicdev, t, timer);
#else
static void aicwf_sdio_bus_pwrctl(ulong data)
{
	struct priv_dev *aicdev = (struct priv_dev *) data;
#endif
	if (aicdev->bus_if->state == BUS_DOWN_ST) {
		bsp_err("bus down\n");
		return;
	}

	if (aicdev->pwrctl_tsk) {
		complete(&aicdev->pwrctrl_trgg);
	}
}
#endif

static void aicwf_sdio_enq_rxpkt(struct priv_dev *aicdev, struct sk_buff *pkt)
{
	struct aicwf_rx_priv *rx_priv = aicdev->rx_priv;
	unsigned long flags = 0;

	spin_lock_irqsave(&rx_priv->rxqlock, flags);
	if (!aicwf_rxframe_enqueue(aicdev->dev, &rx_priv->rxq, pkt)) {
		spin_unlock_irqrestore(&rx_priv->rxqlock, flags);
		aicwf_dev_skb_free(pkt);
		return;
	}
	spin_unlock_irqrestore(&rx_priv->rxqlock, flags);

	atomic_inc(&rx_priv->rx_cnt);
}

static void aicwf_sdio_hal_irqhandler(struct sdio_func *func)
{
#define SDIO_OTHER_INTERRUPT (0x1ul << 7)
	struct aicwf_bus *bus_if = dev_get_drvdata(&func->dev);
	struct priv_dev *aicdev = bus_if->bus_priv.dev;
	u8 intstatus = 0;
	u8 byte_len = 0;
	struct sk_buff *pkt = NULL;
	int ret;
	int retry = 10;

	if (!bus_if || bus_if->state == BUS_DOWN_ST) {
		bsp_err("bus err\n");
		return;
	}
	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D || aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC ||
		aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DW) {
		ret = aicwf_sdio_readb(aicdev->func[0], aicdev->sdio_reg.block_cnt_reg, &intstatus);
		while (ret || (intstatus & SDIO_OTHER_INTERRUPT)) {
			bsp_err("ret=%d, intstatus=%x\r\n", ret, intstatus);
			ret = aicwf_sdio_readb(aicdev->func[0], aicdev->sdio_reg.block_cnt_reg, &intstatus);
			if (retry-- <= 0)
				break;
		}
		aicdev->rx_priv->data_len = intstatus * SDIOWIFI_FUNC_BLOCKSIZE;

		if (intstatus > 0) {
			if (intstatus < 64) {
				pkt = aicwf_sdio_readframes(aicdev);
			} else {
				aicwf_sdio_intr_get_len_bytemode(aicdev, &byte_len);//byte_len must<= 128
				bsp_info("byte mode len=%d\r\n", byte_len);
				pkt = aicwf_sdio_readframes(aicdev);
			}
		} else {
			bsp_err("Interrupt but no data\n");
		}

		if (pkt)
			aicwf_sdio_enq_rxpkt(aicdev, pkt);
	} else if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D80) {
		do {
			ret = aicwf_sdio_readb(aicdev->func[0], aicdev->sdio_reg.misc_int_status_reg, &intstatus);
			if (!ret) {
				break;
			}
			bsp_err("ret=%d, intstatus=%x\r\n", ret, intstatus);
			#ifdef CONFIG_SDIO_F1_FLAG
			sdio_f1_flag = true;
			#endif
			if (retry-- <= 0)
				break;
		} while (1);
		if (intstatus & SDIO_OTHER_INTERRUPT) {
			u8 int_pending;
			ret = aicwf_sdio_readb(aicdev->func[0], aicdev->sdio_reg.sleep_reg, &int_pending);
			if (ret < 0) {
				bsp_err("reg:%d read failed!\n", aicdev->sdio_reg.sleep_reg);
			}
			int_pending &= ~0x01; // dev to host soft irq
			ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.sleep_reg, int_pending);
			if (ret < 0) {
				bsp_err("reg:%d write failed!\n", aicdev->sdio_reg.sleep_reg);
			}
		}

		if (intstatus > 0) {
			uint8_t intmaskf2 = intstatus | (0x1UL << 3);
			if (intmaskf2 > 120U) { // func2
				if (intmaskf2 == 127U) { // byte mode
					//aicwf_sdio_intr_get_len_bytemode(aicdev, &byte_len, 1);//byte_len must<= 128
					aicwf_sdio_intr_get_len_bytemode(aicdev, &byte_len);//byte_len must<= 128
					bsp_info("byte mode len=%d\r\n", byte_len);
					//pkt = aicwf_sdio_readframes(aicdev, 1);
					pkt = aicwf_sdio_readframes(aicdev);
				} else { // block mode
					aicdev->rx_priv->data_len = (intstatus & 0x7U) * SDIOWIFI_FUNC_BLOCKSIZE;
					//pkt = aicwf_sdio_readframes(aicdev, 1);
					pkt = aicwf_sdio_readframes(aicdev);
				}
			} else { // func1
				if (intstatus == 120U) { // byte mode
					//aicwf_sdio_intr_get_len_bytemode(aicdev, &byte_len, 0);//byte_len must<= 128
					aicwf_sdio_intr_get_len_bytemode(aicdev, &byte_len);//byte_len must<= 128
					bsp_info("byte mode len=%d\r\n", byte_len);
					//pkt = aicwf_sdio_readframes(aicdev, 0);
					pkt = aicwf_sdio_readframes(aicdev);
				} else { // block mode
					aicdev->rx_priv->data_len = (intstatus & 0x7FU) * SDIOWIFI_FUNC_BLOCKSIZE;
					//pkt = aicwf_sdio_readframes(aicdev, 0);
					pkt = aicwf_sdio_readframes(aicdev);
				}
			}
	} else {
		#ifndef CONFIG_PLATFORM_ALLWINNER
			bsp_err("Interrupt but no data\n");
		#endif
		}

		if (pkt)
			aicwf_sdio_enq_rxpkt(aicdev, pkt);
	}

	complete(&bus_if->busrx_trgg);
}

static void aicwf_sdio_hal_irqhandler_func2(struct sdio_func *func)
{
	struct aicwf_bus *bus_if = sdiodev->bus_if;
	struct priv_dev *aicdev = sdiodev;
	u8 intstatus = 0;
	u8 byte_len = 0;
#ifdef CONFIG_PREALLOC_RX_SKB
	struct rx_buff *pkt = NULL;
#else
	struct sk_buff *pkt = NULL;
#endif
	int ret;

	if (!bus_if || bus_if->state == BUS_DOWN_ST) {
		if (!bus_if)
			bsp_err("bus if none\n");
		else
			bsp_err("bus state down\n");
		bsp_err("bus err\n");
		return;
	}

#ifdef CONFIG_PREALLOC_RX_SKB
	if (list_empty(&aic_rx_buff_list.rxbuff_list)) {
		printk("%s %d, rxbuff list is empty\n", __func__, __LINE__);
		return;
	}
#endif

	ret = aicwf_sdio_readb(aicdev->func[1], aicdev->sdio_reg.block_cnt_reg, &intstatus);

	while (ret || (intstatus & SDIO_OTHER_INTERRUPT)) {
		bsp_err("ret=%d, intstatus=%x\r\n", ret, intstatus);
		ret = aicwf_sdio_readb(aicdev->func[0], aicdev->sdio_reg.block_cnt_reg, &intstatus);
	}
	aicdev->rx_priv->data_len = intstatus * SDIOWIFI_FUNC_BLOCKSIZE;
	if (intstatus > 0) {
		if (intstatus < 64) {
			pkt = aicwf_sdio_readmessages(aicdev);
		} else {
			bsp_info("byte mode len=%d\r\n", byte_len);

			aicwf_sdio_intr_get_len_bytemode(aicdev, &byte_len);//byte_len must<= 128
			pkt = aicwf_sdio_readmessages(aicdev);
		}
	} else {
	#ifndef CONFIG_PLATFORM_ALLWINNER
		bsp_err("Interrupt but no data\n");
	#endif
	}

	if (pkt)
		aicwf_sdio_enq_rxpkt(aicdev, pkt);

	complete(&bus_if->busrx_trgg);
}

#if defined(CONFIG_SDIO_PWRCTRL)
static void aicwf_sdio_pwrctl_timer(struct priv_dev *aicdev, uint duration)
{
	uint timeout;

	if (aicdev->bus_if->state == BUS_DOWN_ST && duration)
		return;

	spin_lock_bh(&aicdev->pwrctl_lock);
	if (!duration) {
		if (timer_pending(&aicdev->timer))
			del_timer_sync(&aicdev->timer);
	} else {
		aicdev->active_duration = duration;
		timeout = msecs_to_jiffies(aicdev->active_duration);
		mod_timer(&aicdev->timer, jiffies + timeout);
	}
	spin_unlock_bh(&aicdev->pwrctl_lock);
}
#endif

static struct aicwf_bus_ops aicwf_sdio_bus_ops = {
	.stop = aicwf_sdio_bus_stop,
	.start = aicwf_sdio_bus_start,
	.txdata = aicwf_sdio_bus_txdata,
	.txmsg = aicwf_sdio_bus_txmsg,
};

static void aicwf_sdio_release(struct priv_dev *aicdev)
{
	struct aicwf_bus *bus_if;
	int ret = 0;
	int funcnum = 1, i = 0;

	bsp_dbg("%s\n", __func__);

	bus_if = aicdev->bus_if;
	bus_if->state = BUS_DOWN_ST;

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC)
		funcnum = 2;

	for (i = 0; i < funcnum; i++) {
		sdio_claim_host(aicdev->func[i]);
		//disable sdio interrupt
		ret = aicwf_sdio_writeb(aicdev->func[i], aicdev->sdio_reg.intr_config_reg, 0x0);
		if (ret < 0) {
			bsp_err("reg:%d write failed!\n", aicdev->sdio_reg.intr_config_reg);
		}
		sdio_release_irq(aicdev->func[i]);
		sdio_release_host(aicdev->func[i]);
	}

	if (aicdev->dev)
		aicwf_bus_deinit(aicdev->dev);

	if (aicdev->tx_priv)
		aicwf_tx_deinit(aicdev->tx_priv);

	if (aicdev->rx_priv)
		aicwf_rx_deinit(aicdev->rx_priv);

	if (aicdev->cmd_mgr.state == RWNX_CMD_MGR_STATE_INITED)
		rwnx_cmd_mgr_deinit(&aicdev->cmd_mgr);
}

static void aicwf_sdio_reg_init(struct priv_dev *aicdev)
{
	bsp_dbg("%s\n", __func__);

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D || aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC ||
		aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DW) {
		aicdev->sdio_reg.bytemode_len_reg    = SDIOWIFI_BYTEMODE_LEN_REG;
		aicdev->sdio_reg.intr_config_reg     = SDIOWIFI_INTR_CONFIG_REG;
		aicdev->sdio_reg.sleep_reg           = SDIOWIFI_SLEEP_REG;
		aicdev->sdio_reg.wakeup_reg          = SDIOWIFI_WAKEUP_REG;
		aicdev->sdio_reg.flow_ctrl_reg       = SDIOWIFI_FLOW_CTRL_REG;
		aicdev->sdio_reg.register_block      = SDIOWIFI_REGISTER_BLOCK;
		aicdev->sdio_reg.bytemode_enable_reg = SDIOWIFI_BYTEMODE_ENABLE_REG;
		aicdev->sdio_reg.block_cnt_reg       = SDIOWIFI_BLOCK_CNT_REG;
		aicdev->sdio_reg.rd_fifo_addr        = SDIOWIFI_RD_FIFO_ADDR;
		aicdev->sdio_reg.wr_fifo_addr        = SDIOWIFI_WR_FIFO_ADDR;
	} else if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D80) {
		aicdev->sdio_reg.bytemode_len_reg    = SDIOWIFI_BYTEMODE_LEN_REG_V3;
		aicdev->sdio_reg.intr_config_reg     = SDIOWIFI_INTR_ENABLE_REG_V3;
		aicdev->sdio_reg.sleep_reg           = SDIOWIFI_INTR_PENDING_REG_V3;
		aicdev->sdio_reg.wakeup_reg          = SDIOWIFI_INTR_TO_DEVICE_REG_V3;
		aicdev->sdio_reg.flow_ctrl_reg       = SDIOWIFI_FLOW_CTRL_Q1_REG_V3;
		aicdev->sdio_reg.bytemode_enable_reg = SDIOWIFI_BYTEMODE_ENABLE_REG_V3;
		aicdev->sdio_reg.misc_int_status_reg = SDIOWIFI_MISC_INT_STATUS_REG_V3;
		aicdev->sdio_reg.rd_fifo_addr        = SDIOWIFI_RD_FIFO_ADDR_V3;
		aicdev->sdio_reg.wr_fifo_addr        = SDIOWIFI_WR_FIFO_ADDR_V3;
	}
}

static int aicwf_sdio_func_init(struct priv_dev *aicdev)
{
	struct mmc_host *host;
	u8 block_bit0 = 0x1;
	u8 byte_mode_disable = 0x1;//1: no byte mode
	int ret = 0;
	struct aicbsp_feature_t feature;

	aicbsp_get_feature(&feature);
	aicwf_sdio_reg_init(aicdev);
	host = aicdev->func[0]->card->host;

	sdio_claim_host(aicdev->func[0]);
	aicdev->func[0]->card->quirks |= MMC_QUIRK_LENIENT_FN0;
	sdio_f0_writeb(aicdev->func[0], feature.sdio_phase, 0x13, &ret);
	if (ret < 0) {
		bsp_err("write func0 fail %d\n", ret);
		return ret;
	}

	ret = sdio_set_block_size(aicdev->func[0], SDIOWIFI_FUNC_BLOCKSIZE);
	if (ret < 0) {
		bsp_err("set blocksize fail %d\n", ret);
		sdio_release_host(aicdev->func[0]);
		return ret;
	}
	ret = sdio_enable_func(aicdev->func[0]);
	if (ret < 0) {
		bsp_err("enable func fail %d.\n", ret);
		sdio_release_host(aicdev->func[0]);
		return ret;
	}

	if (feature.sdio_clock > 0) {
		host->ios.clock = feature.sdio_clock;
		host->ops->set_ios(host, &host->ios);
		bsp_dbg("Set SDIO Clock %d MHz\n", host->ios.clock/1000000);
	}
	sdio_release_host(aicdev->func[0]);

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC) {
		sdio_claim_host(aicdev->func[1]);

		//set sdio blocksize
		ret = sdio_set_block_size(aicdev->func[1], SDIOWIFI_FUNC_BLOCKSIZE);
		if (ret < 0) {
			printk("set func2 blocksize fail %d\n", ret);
			sdio_release_host(aicdev->func[1]);
			return ret;
		}

		//set sdio enable func
		ret = sdio_enable_func(aicdev->func[1]);
		if (ret < 0) {
			printk("enable func2 fail %d.\n", ret);
		}

		sdio_release_host(aicdev->func[1]);

		ret = aicwf_sdio_writeb(aicdev->func[1], aicdev->sdio_reg.register_block, block_bit0);
		if (ret < 0) {
			printk("reg:%d write failed!\n", aicdev->sdio_reg.register_block);
			return ret;
		}

		//1: no byte mode
		ret = aicwf_sdio_writeb(aicdev->func[1], aicdev->sdio_reg.bytemode_enable_reg, byte_mode_disable);
		if (ret < 0) {
			printk("reg:%d write failed!\n", aicdev->sdio_reg.bytemode_enable_reg);
			return ret;
		}
	}

	ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.register_block, block_bit0);
	if (ret < 0) {
		bsp_err("reg:%d write failed!\n", aicdev->sdio_reg.register_block);
		return ret;
	}

	//1: no byte mode
	ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.bytemode_enable_reg, byte_mode_disable);
	if (ret < 0) {
		bsp_err("reg:%d write failed!\n", aicdev->sdio_reg.bytemode_enable_reg);
		return ret;
	}

	return ret;
}

static int aicwf_sdiov3_func_init(struct priv_dev *aicdev)
{
	struct mmc_host *host;
	u8 byte_mode_disable = 0x1;//1: no byte mode
	int ret = 0;
	struct aicbsp_feature_t feature;

	aicbsp_get_feature(&feature);
	aicwf_sdio_reg_init(aicdev);

	host = aicdev->func[0]->card->host;

	sdio_claim_host(aicdev->func[0]);
	aicdev->func[0]->card->quirks |= MMC_QUIRK_LENIENT_FN0;
	ret = sdio_set_block_size(aicdev->func[0], SDIOWIFI_FUNC_BLOCKSIZE);
	if (ret < 0) {
		bsp_err("set blocksize fail %d\n", ret);
		sdio_release_host(aicdev->func[0]);
		return ret;
	}
	ret = sdio_enable_func(aicdev->func[0]);
	if (ret < 0) {
		bsp_err("enable func fail %d.\n", ret);
		sdio_release_host(aicdev->func[0]);
		return ret;
	}
	sdio_f0_writeb(aicdev->func[0], 0x7F, 0xF2, &ret);
	if (ret) {
	   bsp_err("set fn0 0xF2 fail %d\n", ret);
		sdio_release_host(aicdev->func[0]);
		return ret;
	}
	#ifdef CONFIG_SDIO_F1_FLAG
	if (sdio_f1_flag) {
		bsp_err("func_init sdio_f1\n");
	#endif
		sdio_f0_writeb(aicdev->func[0], 0x80, 0xF1, &ret);
		if (ret) {
			bsp_err("set iopad delay1 fail %d\n", ret);
			sdio_release_host(aicdev->func[0]);
			return ret;
		}
	#ifdef CONFIG_SDIO_F1_FLAG
	}
	#endif
	msleep(1);
#if 1 // SDIO CLOCK SETTING
	if ((feature.sdio_clock > 0) && (host->ios.timing != MMC_TIMING_UHS_DDR50)) {
		host->ios.clock = feature.sdio_clock;
		host->ops->set_ios(host, &host->ios);
		bsp_dbg("Set SDIO Clock %d MHz\n", host->ios.clock/1000000);
	}
#endif
	sdio_release_host(aicdev->func[0]);

	//1: no byte mode
	ret = aicwf_sdio_writeb(aicdev->func[0], aicdev->sdio_reg.bytemode_enable_reg, byte_mode_disable);
	if (ret < 0) {
		bsp_err("reg:%d write failed!\n", aicdev->sdio_reg.bytemode_enable_reg);
		return ret;
	}

	return ret;
}

static void aicwf_sdio_func_deinit(struct priv_dev *aicdev)
{
	sdio_claim_host(aicdev->func[0]);
	sdio_disable_func(aicdev->func[0]);
	sdio_release_host(aicdev->func[0]);
}

static void *aicwf_sdio_bus_init(struct priv_dev *aicdev)
{
	int ret;
	struct aicwf_bus *bus_if;
	struct aicwf_rx_priv *rx_priv;
	struct aicwf_tx_priv *tx_priv;

#if defined(CONFIG_SDIO_PWRCTRL)
	spin_lock_init(&aicdev->pwrctl_lock);
	sema_init(&aicdev->pwrctl_wakeup_sema, 1);
#endif

	bus_if = aicdev->bus_if;
	bus_if->dev = aicdev->dev;
	bus_if->ops = &aicwf_sdio_bus_ops;
	bus_if->state = BUS_DOWN_ST;
#if defined(CONFIG_SDIO_PWRCTRL)
	aicdev->state = SDIO_SLEEP_ST;
	aicdev->active_duration = SDIOWIFI_PWR_CTRL_INTERVAL;
#else
	aicdev->state = SDIO_ACTIVE_ST;
#endif

	rx_priv = aicwf_rx_init(aicdev);
	if (!rx_priv) {
		bsp_err("rx init fail\n");
		goto fail;
	}
	aicdev->rx_priv = rx_priv;

	tx_priv = aicwf_tx_init(aicdev);
	if (!tx_priv) {
		bsp_err("tx init fail\n");
		goto fail;
	}
	aicdev->tx_priv = tx_priv;
	aicwf_frame_queue_init(&tx_priv->txq, 8, TXQLEN);
	spin_lock_init(&tx_priv->txqlock);
	sema_init(&tx_priv->txctl_sema, 1);
	sema_init(&tx_priv->cmd_txsema, 1);
	init_waitqueue_head(&tx_priv->cmd_txdone_wait);
	atomic_set(&tx_priv->tx_pktcnt, 0);

#if defined(CONFIG_SDIO_PWRCTRL)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	timer_setup(&aicdev->timer, aicwf_sdio_bus_pwrctl, 0);
#else
	init_timer(&aicdev->timer);
	aicdev->timer.data = (ulong) aicdev;
	aicdev->timer.function = aicwf_sdio_bus_pwrctl;
#endif
	init_completion(&aicdev->pwrctrl_trgg);
#endif
	ret = aicwf_bus_init(0, aicdev->dev);
	if (ret < 0) {
		bsp_err("bus init fail\n");
		goto fail;
	}

	ret  = aicwf_bus_start(bus_if);
	if (ret != 0) {
		bsp_err("bus start fail\n");
		goto fail;
	}

	return aicdev;

fail:
	aicwf_sdio_release(aicdev);
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


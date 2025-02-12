/******************************************************************************
 *
 * Copyright(c) 2007 - 2021 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#define _HCI_INTF_C_

#include <drv_types.h>
#include <platform_ops.h>
#include <linux/pm_wakeirq.h>
extern struct rtw_intf_ops sdio_ops;

#ifndef CONFIG_SDIO_HCI
#error "CONFIG_SDIO_HCI shall be on!\n"
#endif

#ifndef dev_to_sdio_func
#define dev_to_sdio_func(d)     container_of(d, struct sdio_func, dev)
#endif

static const struct sdio_device_id sdio_ids[] = {

#ifdef CONFIG_RTL8852A
	{SDIO_DEVICE(0x024c, 0x8852), .class = SDIO_CLASS_WLAN, .driver_data = RTL8852A},
	{SDIO_DEVICE(0x024c, 0xa852), .class = SDIO_CLASS_WLAN, .driver_data = RTL8852A},
#endif

#ifdef CONFIG_RTL8852B
	{SDIO_DEVICE(0x024c, 0xb852), .class = SDIO_CLASS_WLAN, .driver_data = RTL8852B},
#endif
#ifdef CONFIG_RTL8852BP
	{SDIO_DEVICE(0x024c, 0xA85C), .class = SDIO_CLASS_WLAN, .driver_data = RTL8852BP},
#endif
#ifdef CONFIG_RTL8851B
	{SDIO_DEVICE(0x024c, 0xB851), .class = SDIO_CLASS_WLAN, .driver_data = RTL8851B},
#endif

#if defined(RTW_ENABLE_WIFI_CONTROL_FUNC) /* temporarily add this to accept all sdio wlan id */
	{ SDIO_DEVICE_CLASS(SDIO_CLASS_WLAN) },
#endif
	{ /* end: all zeroes */				},
};

MODULE_DEVICE_TABLE(sdio, sdio_ids);

static int rtw_dev_probe(struct sdio_func *func, const struct sdio_device_id *id);
static void rtw_dev_remove(struct sdio_func *func);
#ifdef CONFIG_SDIO_HOOK_DEV_SHUTDOWN
static void rtw_dev_shutdown(struct device *dev);
#endif
static int rtw_sdio_resume(struct device *dev);
static int rtw_sdio_suspend(struct device *dev);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29))
static const struct dev_pm_ops rtw_sdio_pm_ops = {
	.suspend = rtw_sdio_suspend,
	.resume	= rtw_sdio_resume,
};
#endif

struct sdio_drv_priv {
	struct sdio_driver rtw_sdio_drv;
	int drv_registered;
};

static struct sdio_drv_priv sdio_drvpriv = {
	.rtw_sdio_drv.probe = rtw_dev_probe,
	.rtw_sdio_drv.remove = rtw_dev_remove,
	.rtw_sdio_drv.name = (char *)DRV_NAME,
	.rtw_sdio_drv.id_table = sdio_ids,
	.rtw_sdio_drv.drv = {
#ifdef CONFIG_SDIO_HOOK_DEV_SHUTDOWN
		.shutdown = rtw_dev_shutdown,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29))
		.pm = &rtw_sdio_pm_ops,
#endif
	}
};


static void sd_sync_int_hdl(struct sdio_func *func)
{
	struct dvobj_priv *dvobj;

	dvobj = sdio_get_drvdata(func);

	if (RTW_CANNOT_RUN(dvobj))
		return;

	rtw_sdio_set_irq_thd(dvobj, current);
	/*sd_int_hdl(dvobj_get_primary_adapter(psdpriv));*/
	rtw_phl_interrupt_handler(dvobj->phl);
	rtw_sdio_set_irq_thd(dvobj, NULL);
}

int rtw_sdio_alloc_irq(struct dvobj_priv *dvobj)
{
	struct sdio_data *psdio_data;
	struct sdio_func *func;
	int err;

	psdio_data = dvobj_to_sdio(dvobj);
	func = psdio_data->func;

	sdio_claim_host(func);

	err = sdio_claim_irq(func, &sd_sync_int_hdl);
	if (err) {
		dvobj->drv_dbg.dbg_sdio_alloc_irq_error_cnt++;
		RTW_PRINT("%s: sdio_claim_irq FAIL(%d)!\n", __func__, err);
	} else {
		dvobj->drv_dbg.dbg_sdio_alloc_irq_cnt++;
		psdio_data->irq_alloc = 1;
	}

	sdio_release_host(func);

	return err ? _FAIL : _SUCCESS;
}

void rtw_sdio_free_irq(struct dvobj_priv *dvobj)
{
	struct sdio_data *psdio_data;
	struct sdio_func *func;
	int err;

	psdio_data = dvobj_to_sdio(dvobj);
	if (psdio_data && psdio_data->irq_alloc) {
		func = psdio_data->func;

		if (func) {
			sdio_claim_host(func);
			err = sdio_release_irq(func);
			if (err) {
				dvobj->drv_dbg.dbg_sdio_free_irq_error_cnt++;
				RTW_ERR("%s: sdio_release_irq FAIL(%d)!\n", __func__, err);
			} else
				dvobj->drv_dbg.dbg_sdio_free_irq_cnt++;
			sdio_release_host(func);
		}
		psdio_data->irq_alloc = 0;
	}
}

#ifdef CONFIG_GPIO_WAKEUP
extern unsigned int oob_irq;
extern unsigned int oob_gpio;
static irqreturn_t gpio_hostwakeup_irq_thread(int irq, void *data)
{
	_adapter *padapter = (_adapter *)data;
	RTW_PRINT("gpio_hostwakeup_irq_thread\n");
	/* Disable interrupt before calling handler */
	/* disable_irq_nosync(oob_irq); */
#ifdef CONFIG_PLATFORM_ARM_SUN6I
	return 0;
#else
	return IRQ_HANDLED;
#endif
}

static u8 gpio_hostwakeup_alloc_irq(_adapter *padapter)
{
	int err;
	u32 status = 0;
	struct device *dev = dvobj_to_dev(padapter->dvobj);

	if (oob_irq == 0) {
		RTW_INFO("oob_irq ZERO!\n");
		return _FAIL;
	}

	RTW_INFO("%s : oob_irq = %d\n", __func__, oob_irq);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32))
	status = IRQF_NO_SUSPEND;
#endif

	if (HIGH_ACTIVE_DEV2HST)
		status |= IRQF_TRIGGER_RISING;
	else
		status |= IRQF_TRIGGER_FALLING;

	err = request_threaded_irq(oob_irq, gpio_hostwakeup_irq_thread, NULL,
		status, "rtw_wifi_gpio_wakeup", padapter);

	if (err < 0) {
		RTW_INFO("Oops: can't allocate gpio irq %d err:%d\n", oob_irq, err);
		return _FALSE;
	} else
		RTW_INFO("allocate gpio irq %d ok\n", oob_irq);

	device_init_wakeup(dev, true);
	dev_pm_set_wake_irq(dev, oob_irq);
#ifndef CONFIG_PLATFORM_ARM_SUN8I
	enable_irq_wake(oob_irq);
#endif
	return _SUCCESS;
}

static void gpio_hostwakeup_free_irq(_adapter *padapter)
{
	struct device *dev = dvobj_to_dev(padapter->dvobj);

	if (oob_irq == 0)
		return;

	dev_pm_clear_wake_irq(dev);
	device_init_wakeup(dev, false);
#ifndef CONFIG_PLATFORM_ARM_SUN8I
	disable_irq_wake(oob_irq);
#endif
	free_irq(oob_irq, padapter);
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
/*
 * mmc_blksz_for_byte_mode() & mmc_card_broken_byte_mode_512() has been moved
 * to drivers/mmc/core/card.h from include/linux/mmc/card.h since kernel v4.11.
 */
static inline int mmc_blksz_for_byte_mode(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_BLKSZ_FOR_BYTE_MODE;
}

static inline int mmc_card_broken_byte_mode_512(const struct mmc_card *c)
{
	return c->quirks & MMC_QUIRK_BROKEN_BYTE_MODE_512;
}
#endif /* kernel >= v4.11 */

/*
 * Calculate the maximum byte mode transfer size
 */
static inline unsigned int sdio_max_byte_size(struct sdio_func *func)
{
	unsigned mval =	func->card->host->max_blk_size;

	if (mmc_blksz_for_byte_mode(func->card))
		mval = min(mval, func->cur_blksize);
	else
		mval = min(mval, func->max_blksize);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
	if (mmc_card_broken_byte_mode_512(func->card))
		return min(mval, 511u);
#endif /* kernel v3.3 */

	return min(mval, 512u); /* maximum size for byte mode */
}

void dump_sdio_card_info(void *sel, struct dvobj_priv *dvobj)
{
	struct sdio_data *psdio_data = dvobj_to_sdio(dvobj);
	struct mmc_card *card = psdio_data->card;
	int i;

	RTW_PRINT_SEL(sel, "== SDIO Card Info ==\n");
	RTW_PRINT_SEL(sel, "  card: %p\n", card);
	RTW_PRINT_SEL(sel, "  clock: %d Hz\n", psdio_data->clock);

	RTW_PRINT_SEL(sel, "  timing spec: ");
	switch (psdio_data->timing) {
	case MMC_TIMING_LEGACY:
		_RTW_PRINT_SEL(sel, "legacy");
		break;
	case MMC_TIMING_MMC_HS:
		_RTW_PRINT_SEL(sel, "mmc high-speed");
		break;
	case MMC_TIMING_SD_HS:
		_RTW_PRINT_SEL(sel, "sd high-speed");
		break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	case MMC_TIMING_UHS_SDR12:
		_RTW_PRINT_SEL(sel, "sd uhs SDR12");
		break;
	case MMC_TIMING_UHS_SDR25:
		_RTW_PRINT_SEL(sel, "sd uhs SDR25");
		break;
	#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0) */

	case MMC_TIMING_UHS_SDR50:
		_RTW_PRINT_SEL(sel, "sd uhs SDR50");
		break;

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
	case MMC_TIMING_MMC_DDR52:
		_RTW_PRINT_SEL(sel, "mmc DDR52");
		break;
	#endif

	case MMC_TIMING_UHS_SDR104:
		_RTW_PRINT_SEL(sel, "sd uhs SDR104");
		break;
	case MMC_TIMING_UHS_DDR50:
		_RTW_PRINT_SEL(sel, "sd uhs DDR50");
		break;

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
	case MMC_TIMING_MMC_HS200:
		_RTW_PRINT_SEL(sel, "mmc HS200");
		break;
	#endif

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
	case MMC_TIMING_MMC_HS400:
		_RTW_PRINT_SEL(sel, "mmc HS400");
		break;
	#endif
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0) */
	default:
		_RTW_PRINT_SEL(sel, "unknown(%d)", psdio_data->timing);
		break;
	}
	_RTW_PRINT_SEL(sel, "\n");

	RTW_PRINT_SEL(sel, "  sd3_bus_mode: %s\n", (psdio_data->sd3_bus_mode) ? "TRUE" : "FALSE");

	rtw_warn_on(card->sdio_funcs != rtw_sdio_get_num_of_func(dvobj));
	RTW_PRINT_SEL(sel, "  func num: %u\n", card->sdio_funcs);
	for (i = 0; card->sdio_func[i]; i++) {
		RTW_PRINT_SEL(sel, "  func%u: %p%s\n"
			, card->sdio_func[i]->num, card->sdio_func[i]
			, psdio_data->func == card->sdio_func[i] ? " (*)" : "");
	}

	RTW_PRINT_SEL(sel, "  max_byte_size: %u\n", psdio_data->max_byte_size);

	RTW_PRINT_SEL(sel, "================\n");
}

#define SDIO_CARD_INFO_DUMP(dvobj)	dump_sdio_card_info(RTW_DBGDUMP, dvobj)

#ifdef DBG_SDIO
#if (DBG_SDIO >= 2)
void rtw_sdio_dbg_reg_free(struct dvobj_priv *d)
{
	struct sdio_data *sdio;
	u8 *buf;
	u32 size;


	sdio = dvobj_to_sdio(d);

	buf = sdio->dbg_msg;
	size = sdio->dbg_msg_size;
	if (buf){
		sdio->dbg_msg = NULL;
		sdio->dbg_msg_size = 0;
		rtw_mfree(buf, size);
	}

	buf = sdio->reg_mac;
	if (buf) {
		sdio->reg_mac = NULL;
		rtw_mfree(buf, 0x800);
	}

	buf = sdio->reg_mac_ext;
	if (buf) {
		sdio->reg_mac_ext = NULL;
		rtw_mfree(buf, 0x800);
	}

	buf = sdio->reg_local;
	if (buf) {
		sdio->reg_local = NULL;
		rtw_mfree(buf, 0x100);
	}

	buf = sdio->reg_cia;
	if (buf) {
		sdio->reg_cia = NULL;
		rtw_mfree(buf, 0x200);
	}
}

void rtw_sdio_dbg_reg_alloc(struct dvobj_priv *d)
{
	struct sdio_data *sdio;
	u8 *buf;


	sdio = dvobj_to_sdio(d);

	buf = _rtw_zmalloc(0x800);
	if (buf)
		sdio->reg_mac = buf;

	buf = _rtw_zmalloc(0x800);
	if (buf)
		sdio->reg_mac_ext = buf;

	buf = _rtw_zmalloc(0x100);
	if (buf)
		sdio->reg_local = buf;

	buf = _rtw_zmalloc(0x200);
	if (buf)
		sdio->reg_cia = buf;
}
#endif /* DBG_SDIO >= 2 */

static void sdio_dbg_init(struct dvobj_priv *d)
{
	struct sdio_data *sdio;


	sdio = dvobj_to_sdio(d);

	sdio->cmd52_err_cnt = 0;
	sdio->cmd53_err_cnt = 0;

#if (DBG_SDIO >= 1)
	sdio->reg_dump_mark = 0;
#endif /* DBG_SDIO >= 1 */

#if (DBG_SDIO >= 3)
	sdio->dbg_enable = 0;
	sdio->err_stop = 0;
	sdio->err_test = 0;
	sdio->err_test_triggered = 0;
#endif /* DBG_SDIO >= 3 */
}

static void sdio_dbg_deinit(struct dvobj_priv *d)
{
#if (DBG_SDIO >= 2)
	rtw_sdio_dbg_reg_free(d);
#endif /* DBG_SDIO >= 2 */
}
#endif /* DBG_SDIO */

u32 rtw_sdio_init(struct dvobj_priv *dvobj)
{
	struct sdio_data *psdio_data;
	struct sdio_func *func;
	int err;


	psdio_data = dvobj_to_sdio(dvobj);
	func = psdio_data->func;

	/* 3 1. init SDIO bus */
	sdio_claim_host(func);

	err = sdio_enable_func(func);
	if (err) {
		dvobj->drv_dbg.dbg_sdio_init_error_cnt++;
		RTW_PRINT("%s: sdio_enable_func FAIL(%d)!\n", __func__, err);
		goto release;
	}

	err = sdio_set_block_size(func, 512);
	if (err) {
		dvobj->drv_dbg.dbg_sdio_init_error_cnt++;
		RTW_PRINT("%s: sdio_set_block_size FAIL(%d)!\n", __func__, err);
		goto release;
	}
	psdio_data->block_transfer_len = 512;
	psdio_data->tx_block_mode = 1;
	psdio_data->rx_block_mode = 1;

	psdio_data->card = func->card;
	psdio_data->timing = func->card->host->ios.timing;
	psdio_data->clock = func->card->host->ios.clock;
	psdio_data->func_number = func->card->sdio_funcs;

	psdio_data->sd3_bus_mode = _FALSE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
	if (psdio_data->timing <= MMC_TIMING_UHS_DDR50
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
		&& psdio_data->timing >= MMC_TIMING_UHS_SDR12
		#else
		&& psdio_data->timing >= MMC_TIMING_UHS_SDR50
		#endif
	)
		psdio_data->sd3_bus_mode = _TRUE;
#endif

	psdio_data->max_byte_size = sdio_max_byte_size(func);

#ifdef DBG_SDIO
	sdio_dbg_init(dvobj);
#endif /* DBG_SDIO */

	SDIO_CARD_INFO_DUMP(dvobj);


release:
	sdio_release_host(func);

	if (err)
		return _FAIL;
	return _SUCCESS;
}

void rtw_sdio_deinit(struct dvobj_priv *dvobj)
{
	struct sdio_func *func;
	int err;

	func = dvobj_to_sdio(dvobj)->func;

	if (func) {
		sdio_claim_host(func);
		err = sdio_disable_func(func);
		if (err) {
			dvobj->drv_dbg.dbg_sdio_deinit_error_cnt++;
			RTW_ERR("%s: sdio_disable_func(%d)\n", __func__, err);
		}

		sdio_release_host(func);
	}

#ifdef DBG_SDIO
	sdio_dbg_deinit(dvobj);
#endif /* DBG_SDIO */
}

u8 rtw_sdio_get_num_of_func(struct dvobj_priv *dvobj)
{
	return dvobj_to_sdio(dvobj)->func_number;
}

static struct dvobj_priv *sdio_dvobj_init(struct sdio_func *func,
					const struct sdio_device_id *pdid)
{
	int status = _FAIL;
	struct dvobj_priv *dvobj = NULL;
	struct sdio_data *psdio;


	dvobj = devobj_init();
	if (dvobj == NULL)
		goto exit;

	sdio_set_drvdata(func, dvobj);

	psdio = dvobj_to_sdio(dvobj);
	psdio->func = func;

	psdio->tmpbuf_sz = 32;
	psdio->tmpbuf = rtw_malloc(psdio->tmpbuf_sz);
	if (!psdio->tmpbuf) {
		psdio->tmpbuf_sz = 0;
		goto free_dvobj;
	}

	if (rtw_sdio_init(dvobj) != _SUCCESS) {
		goto free_dvobj;
	}

	dvobj->interface_type = RTW_HCI_SDIO;
	dvobj->ic_id = pdid->driver_data;
	dvobj->intf_ops = &sdio_ops;

	rtw_reset_continual_io_error(dvobj);
	status = _SUCCESS;

free_dvobj:
	if (status != _SUCCESS && dvobj) {
		sdio_set_drvdata(func, NULL);
		devobj_deinit(dvobj);
		dvobj = NULL;
	}
exit:
	return dvobj;
}

static void sdio_dvobj_deinit(struct sdio_func *func)
{
	struct dvobj_priv *dvobj = sdio_get_drvdata(func);
	struct sdio_data *sdio;


	sdio_set_drvdata(func, NULL);
	if (dvobj) {
		rtw_sdio_deinit(dvobj);
		rtw_sdio_free_irq(dvobj);

		sdio = dvobj_to_sdio(dvobj);
		if (sdio->tmpbuf_sz) {
			rtw_mfree(sdio->tmpbuf, sdio->tmpbuf_sz);
			sdio->tmpbuf_sz = 0;
			sdio->tmpbuf = NULL;
		}

		devobj_deinit(dvobj);
	}

	return;
}

#ifdef RTW_SUPPORT_PLATFORM_SHUTDOWN
_adapter * g_test_adapter = NULL;
#endif /* RTW_SUPPORT_PLATFORM_SHUTDOWN */

_adapter *rtw_sdio_primary_adapter_init(struct dvobj_priv *dvobj)
{
	int status = _FAIL;
	_adapter *padapter = NULL;
	u8 hw_mac_addr[ETH_ALEN] = {0};

	padapter = (_adapter *)rtw_zvmalloc(sizeof(*padapter));
	if (padapter == NULL)
		goto exit;

	/*registry_priv*/
	if (rtw_load_registry(padapter) != _SUCCESS)
		goto free_adapter;

#ifdef RTW_SUPPORT_PLATFORM_SHUTDOWN
	g_test_adapter = padapter;
#endif /* RTW_SUPPORT_PLATFORM_SHUTDOWN */
	padapter->dvobj = dvobj;

	dvobj->padapters[dvobj->iface_nums++] = padapter;
	padapter->iface_id = IFACE_ID0;

	/* set adapter_type/iface type for primary padapter */
	padapter->isprimary = _TRUE;
	padapter->adapter_type = PRIMARY_ADAPTER;

	/* 3 7. init driver common data */
	if (rtw_init_drv_sw(padapter) == _FAIL) {
		goto free_adapter;
	}

	/* get mac addr */
	rtw_hw_get_mac_addr(dvobj, hw_mac_addr);

	/* set mac addr */
	rtw_macaddr_cfg(adapter_mac_addr(padapter), hw_mac_addr);

	RTW_INFO("bDriverStopped:%s, bSurpriseRemoved:%s, netif_up:%d, hw_init_completed:%d\n"
		, dev_is_drv_stopped(dvobj) ? "True" : "False"
		, dev_is_surprise_removed(dvobj) ? "True" : "False"
		, padapter->netif_up
		, rtw_hw_get_init_completed(dvobj)
	);

	status = _SUCCESS;

free_adapter:
	if (status != _SUCCESS && padapter) {
		rtw_vmfree((u8 *)padapter, sizeof(*padapter));
		padapter = NULL;
	}
exit:
	return padapter;
}

static void rtw_sdio_primary_adapter_deinit(_adapter *padapter)
{

#ifdef CONFIG_GPIO_WAKEUP
#ifdef CONFIG_PLATFORM_ARM_SUN6I
	sw_gpio_eint_set_enable(gpio_eint_wlan, 0);
	sw_gpio_irq_free(eint_wlan_handle);
#else
	gpio_hostwakeup_free_irq(padapter);
#endif
#endif
	rtw_free_drv_sw(padapter);

	/* TODO: use rtw_os_ndevs_deinit instead at the first stage of driver's dev deinit function */
	rtw_os_ndev_free(padapter);

	rtw_vmfree((u8 *)padapter, sizeof(_adapter));
#ifdef RTW_SUPPORT_PLATFORM_SHUTDOWN
	g_test_adapter = NULL;
#endif /* RTW_SUPPORT_PLATFORM_SHUTDOWN */
}

/*
 * drv_init() - a device potentially for us
 *
 * notes: drv_init() is called when the bus driver has located a card for us to support.
 *        We accept the new device by returning 0.
 */
static int rtw_dev_probe(
	struct sdio_func *func,
	const struct sdio_device_id *id)
{
	_adapter *padapter = NULL;
	struct dvobj_priv *dvobj;

	RTW_INFO("+%s\n", __func__);

	dvobj = sdio_dvobj_init(func, id);
	if (dvobj == NULL) {
		RTW_ERR("dvobj == NULL\n");
		goto exit;
	}

	if (devobj_trx_resource_init(dvobj) == _FAIL)
		goto free_dvobj;

	/*init hw - register and get chip-info and hw capability*/
	if (rtw_hw_init(dvobj) == _FAIL) {
		RTW_ERR("rtw_hw_init Failed!\n");
		goto free_trx_reso;
	}

	padapter = rtw_sdio_primary_adapter_init(dvobj);
	if (padapter == NULL) {
		RTW_INFO("rtw_init_primary_adapter Failed!\n");
		goto free_hw;
	}

#ifdef CONFIG_CONCURRENT_MODE
	if (rtw_drv_add_vir_ifaces(dvobj) == _FAIL)
		goto free_if_vir;
#endif

	if (rtw_adapter_link_init(dvobj) != _SUCCESS)
		goto free_adapter_link;

	/*init data of dvobj from registary and ic spec*/
	if (devobj_data_init(dvobj) == _FAIL) {
		RTW_ERR("devobj_data_init Failed!\n");
		/*free self beacuse of the function donnot clean memory when fail*/
		goto free_dvobj_data;
	}

	/* dev_alloc_name && register_netdev */
	if (rtw_os_ndevs_init(dvobj) != _SUCCESS)
		goto free_dvobj_data;

	/* Update link_mlme_priv's ht/vht/he priv from padapter->mlmepriv */
	rtw_init_link_capab(dvobj);

#ifdef CONFIG_HOSTAPD_MLME
	hostapd_mode_init(padapter);
#endif

	if (rtw_sdio_alloc_irq(dvobj) != _SUCCESS)
		goto os_ndevs_deinit;

#ifdef CONFIG_GPIO_WAKEUP
	#ifdef CONFIG_PLATFORM_ARM_SUN6I
	eint_wlan_handle = sw_gpio_irq_request(gpio_eint_wlan, TRIG_EDGE_NEGATIVE, (peint_handle)gpio_hostwakeup_irq_thread, NULL);
	if (!eint_wlan_handle) {
		RTW_INFO("%s: request irq failed\n", __func__);
		goto os_ndevs_deinit;
	}
	#else
	gpio_hostwakeup_alloc_irq(padapter);
	#endif
#endif/*CONFIG_GPIO_WAKEUP*/

#ifdef CONFIG_GLOBAL_UI_PID
	if (ui_pid[1] != 0) {
		RTW_INFO("ui_pid[1]:%d\n", ui_pid[1]);
		rtw_signal_process(ui_pid[1], SIGUSR2);
	}
#endif
	RTW_INFO("-%s success\n", __func__);

	return 0; /*_SUCCESS*/


os_ndevs_deinit:
	rtw_os_ndevs_deinit(dvobj);

free_dvobj_data:
	devobj_data_deinit(dvobj);

free_adapter_link:
	rtw_adapter_link_deinit(dvobj);

#ifdef CONFIG_CONCURRENT_MODE
free_if_vir:
	rtw_drv_stop_vir_ifaces(dvobj);
	rtw_drv_free_vir_ifaces(dvobj);
#endif
	if (padapter)
		rtw_sdio_primary_adapter_deinit(padapter);
free_hw:
	rtw_hw_deinit(dvobj);

free_trx_reso:
	devobj_trx_resource_deinit(dvobj);

free_dvobj:
	sdio_dvobj_deinit(func);
exit:
	return -ENODEV;
}

static void rtw_dev_remove(struct sdio_func *func)
{
	struct dvobj_priv *dvobj = sdio_get_drvdata(func);
	struct pwrctrl_priv *pwrctl = dvobj_to_pwrctl(dvobj);
	_adapter *padapter = dvobj_get_primary_adapter(dvobj);

	RTW_INFO("+%s\n", __func__);

	dvobj->processing_dev_remove = _TRUE;

	/* TODO: use rtw_os_ndevs_deinit instead at the first stage of driver's dev deinit function */
	rtw_os_ndevs_unregister(dvobj);

	if (!dev_is_surprise_removed(dvobj)) {
		int err;

		/* test surprise remove */
		sdio_claim_host(func);
		sdio_readb(func, 0, &err);
		sdio_release_host(func);
		if (err == -ENOMEDIUM) {
			dev_set_surprise_removed(dvobj);
			RTW_INFO("%s: device had been removed!\n", __func__);
		}
	}

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_ANDROID_POWER)
	rtw_unregister_early_suspend(pwrctl);
#endif
	dev_set_drv_stopped(adapter_to_dvobj(padapter));	/*for stop thread*/
#if 0 /*#ifdef CONFIG_CORE_CMD_THREAD*/
	rtw_stop_cmd_thread(padapter);
#endif

#ifdef CONFIG_CONCURRENT_MODE
	rtw_drv_stop_vir_ifaces(dvobj);
#endif
	rtw_drv_stop_prim_iface(padapter);

	rtw_hw_stop(dvobj);
	dev_set_surprise_removed(dvobj);

	rtw_adapter_link_deinit(dvobj);

	rtw_sdio_primary_adapter_deinit(padapter);

#ifdef CONFIG_CONCURRENT_MODE
	rtw_drv_free_vir_ifaces(dvobj);
#endif
	rtw_hw_deinit(dvobj);
	devobj_data_deinit(dvobj);
	devobj_trx_resource_deinit(dvobj);
	sdio_dvobj_deinit(func);

	RTW_INFO("-%s done\n", __func__);
}

#ifdef CONFIG_SDIO_HOOK_DEV_SHUTDOWN
static void rtw_dev_shutdown(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);

	if (func == NULL)
		return;

	RTW_INFO("==> %s !\n", __func__);

	rtw_dev_remove(func);

	RTW_INFO("<== %s !\n", __func__);
}
#endif

static int rtw_sdio_suspend(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	struct dvobj_priv *psdpriv;
	struct pwrctrl_priv *pwrpriv = NULL;
	_adapter *padapter = NULL;
	struct debug_priv *pdbgpriv = NULL;
	int ret = 0;
#ifdef CONFIG_RTW_SDIO_PM_KEEP_POWER
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34))
	mmc_pm_flag_t pm_flag = 0;
#endif
#endif

	if (dev == NULL)
		goto exit;

	psdpriv = sdio_get_drvdata(func);
	if (psdpriv == NULL)
		return ret;

	pwrpriv = dvobj_to_pwrctl(psdpriv);
	padapter = dvobj_get_primary_adapter(psdpriv);
	pdbgpriv = &psdpriv->drv_dbg;
	if (dev_is_drv_stopped(adapter_to_dvobj(padapter))) {
		RTW_INFO("%s bDriverStopped == _TRUE\n", __func__);
		rtw_sdio_deinit(adapter_to_dvobj(padapter));
		#if !(CONFIG_RTW_SDIO_KEEP_IRQ)
		rtw_sdio_free_irq(adapter_to_dvobj(padapter));
		#endif
		return ret;
	}

	if (pwrpriv->bInSuspend == _TRUE) {
		RTW_INFO("%s bInSuspend = %d\n", __func__, pwrpriv->bInSuspend);
		pdbgpriv->dbg_suspend_error_cnt++;
		goto exit;
	}

	ret = rtw_suspend_common(padapter);

#ifdef CONFIG_RTW_SDIO_PM_KEEP_POWER
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34))
	/* Android 4.0 don't support WIFI close power */
	/* or power down or clock will close after wifi resume, */
	/* this is sprd's bug in Android 4.0, but sprd don't */
	/* want to fix it. */
	/* we have test power under 8723as, power consumption is ok */
	pm_flag = sdio_get_host_pm_caps(func);
	RTW_INFO("cmd: %s: suspend: PM flag = 0x%x\n", sdio_func_id(func), pm_flag);
	if (!(pm_flag & MMC_PM_KEEP_POWER)) {
		RTW_INFO("%s: cannot remain alive while host is suspended\n", sdio_func_id(func));
		if (pdbgpriv)
			pdbgpriv->dbg_suspend_error_cnt++;
		return -ENOSYS;
	} else {
		RTW_INFO("cmd: suspend with MMC_PM_KEEP_POWER\n");
		sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
	}
#endif
#endif
exit:
	return ret;
}
static int rtw_resume_process(_adapter *padapter)
{
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	struct dvobj_priv *psdpriv = padapter->dvobj;
	struct debug_priv *pdbgpriv = &psdpriv->drv_dbg;

	if (pwrpriv->bInSuspend == _FALSE) {
		pdbgpriv->dbg_resume_error_cnt++;
		RTW_INFO("%s bInSuspend = %d\n", __FUNCTION__, pwrpriv->bInSuspend);
		if (dev_is_drv_stopped(adapter_to_dvobj(padapter))) {
			/* interface init */
			if (rtw_sdio_init(psdpriv) != _SUCCESS) {
				RTW_WARN("%s rtw_sdio_init fail\n", __FUNCTION__);
				return -1;
			}
			#if !(CONFIG_RTW_SDIO_KEEP_IRQ)
			if (rtw_sdio_alloc_irq(psdpriv) != _SUCCESS) {
				RTW_WARN("%s rtw_sdio_alloc_irq fail\n", __FUNCTION__);
				return -1;
			}
			#endif/*CONFIG_RTW_SDIO_KEEP_IRQ*/
			return 0;
		}
		return -1;
	}

	return rtw_resume_common(padapter);
}

static int rtw_sdio_resume(struct device *dev)
{
	struct sdio_func *func = dev_to_sdio_func(dev);
	struct dvobj_priv *psdpriv = sdio_get_drvdata(func);
	struct pwrctrl_priv *pwrpriv = dvobj_to_pwrctl(psdpriv);
	_adapter *padapter = dvobj_get_primary_adapter(psdpriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	int ret = 0;
	struct debug_priv *pdbgpriv = &psdpriv->drv_dbg;

	RTW_INFO("==> %s (%s:%d)\n", __FUNCTION__, current->comm, current->pid);

	pdbgpriv->dbg_resume_cnt++;

	if (pwrpriv->wowlan_mode || pwrpriv->wowlan_ap_mode) {
		rtw_resume_lock_suspend();
		ret = rtw_resume_process(padapter);
		rtw_resume_unlock_suspend();
	} else {
#ifdef CONFIG_RESUME_IN_WORKQUEUE
		rtw_resume_in_workqueue(pwrpriv);
#else
		if (rtw_is_earlysuspend_registered(pwrpriv)) {
			/* jeff: bypass resume here, do in late_resume */
			rtw_set_do_late_resume(pwrpriv, _TRUE);
		} else {
			rtw_resume_lock_suspend();
			ret = rtw_resume_process(padapter);
			rtw_resume_unlock_suspend();
		}
#endif
	}
	pmlmeext->last_scan_time = rtw_get_current_time();
	RTW_INFO("<========  %s return %d\n", __FUNCTION__, ret);
	return ret;

}

static int __init rtw_drv_entry(void)
{
	int ret = 0;

	RTW_PRINT("module init start\n");
	dump_drv_version(RTW_DBGDUMP);
#ifdef BTCOEXVERSION
	RTW_PRINT(DRV_NAME" BT-Coex version = %s\n", BTCOEXVERSION);
#endif /* BTCOEXVERSION */

#if (defined(CONFIG_RTKM) && defined(CONFIG_RTKM_BUILT_IN))
	ret = rtkm_prealloc_init();
	if (ret) {
		RTW_INFO("%s: pre-allocate memory failed!!(%d)\n", __FUNCTION__,
			 ret);
		goto exit;
	}
#endif /* CONFIG_RTKM */

	rtw_android_wifictrl_func_add();
#ifdef CONFIG_GPIO_WAKEUP
	platform_wifi_get_oob_irq(&oob_irq);
#endif

	ret = platform_wifi_power_on();
	if (ret) {
		RTW_INFO("%s: power on failed!!(%d)\n", __FUNCTION__, ret);
		ret = -1;
		goto exit;
	}

	sdio_drvpriv.drv_registered = _TRUE;
	rtw_suspend_lock_init();
	rtw_drv_proc_init();
	rtw_nlrtw_init();
	rtw_ndev_notifier_register();
	rtw_inetaddr_notifier_register();

	ret = sdio_register_driver(&sdio_drvpriv.rtw_sdio_drv);
	if (ret != 0) {
		sdio_drvpriv.drv_registered = _FALSE;
		rtw_suspend_lock_uninit();
		rtw_drv_proc_deinit();
		rtw_nlrtw_deinit();
		rtw_ndev_notifier_unregister();
		rtw_inetaddr_notifier_unregister();
		RTW_INFO("%s: register driver failed!!(%d)\n", __FUNCTION__, ret);
		goto poweroff;
	}

	goto exit;

poweroff:
	platform_wifi_power_off();

exit:
	RTW_PRINT("module init ret=%d\n", ret);
	return ret;
}

static void __exit rtw_drv_halt(void)
{
	RTW_PRINT("module exit start\n");

	sdio_drvpriv.drv_registered = _FALSE;

	sdio_unregister_driver(&sdio_drvpriv.rtw_sdio_drv);

	rtw_android_wifictrl_func_del();

	platform_wifi_power_off();

	rtw_suspend_lock_uninit();
	rtw_drv_proc_deinit();
	rtw_nlrtw_deinit();
	rtw_ndev_notifier_unregister();
	rtw_inetaddr_notifier_unregister();

	RTW_PRINT("module exit success\n");

	rtw_mstat_dump(RTW_DBGDUMP);

#if (defined(CONFIG_RTKM) && defined(CONFIG_RTKM_BUILT_IN))
	rtkm_prealloc_destroy();
#elif (defined(CONFIG_RTKM) && defined(CONFIG_RTKM_STANDALONE))
	rtkm_dump_mstatus(RTW_DBGDUMP);
#endif /* CONFIG_RTKM */
}

module_init(rtw_drv_entry);
module_exit(rtw_drv_halt);

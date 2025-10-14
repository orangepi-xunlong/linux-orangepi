// SPDX-License-Identifier: GPL-2.0

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/watchdog.h>
#include <linux/acpi.h>
#ifdef CONFIG_CIX_DST
#include <mntn_public_interface.h>
#include <linux/soc/cix/util.h>
#endif

#define DRIVER_NAME "sky1-wdt"

/* Control frame registers */
#define SKY1_WDT_WCS_RW         0x000   /* Watchdog control and status register */
#define SKY1_WDT_WCS_ENABLE     BIT(0)  /* Watchdog Enable */
#define SKY1_WDT_WCS_WS0        BIT(1)  /* Watchdog Signal0 */
#define SKY1_WDT_WCS_WS1        BIT(2)  /* Watchdog Signal1 */

#define SKY1_WDT_WOR_LOW_RW     0x008   /* Watchdog offset register */
#define SKY1_WDT_WOR_HIGH_RW    0x00C   /* Watchdog offset register */
#define SKY1_WDT_WCV_LOW_RW     0x010   /* Watchdog compare value */
#define SKY1_WDT_WCV_HIGH_RW    0x014   /* Watchdog compare value */

/* Refresh frame registers */
#define SKY1_WDT_WRR_RW         0x000  /* Watchdog refresh register */
#define SKY1_WDT_WRR_REFRESH    BIT(0)  /* Explicit watchdog refresh ocurrs */
#define SKY1_WDT_W_IID_RO       0xFCC   /* W_IIDR is a 32-bit read-only register */

#define SKY1_WDT_MAX_TIME       0x44B82
#define SKY1_WDT_DEFAULT_TIME   60      /* in seconds */
#define WDG_FEED_MOMENT_ADJUST  1

#define MSECS_TO_JIFFIES	1000
#define WDOG_SEC_TO_COUNT	1000000000 /* sky1 global watchdog frequency 1G */

struct sky1_wdt_device {
	struct watchdog_device wdd;
	struct regmap *regmap_ctrl;
	struct regmap *regmap_fresh;
	unsigned int kick_time;
	struct delayed_work sky1_wdt_delayed_work;
	struct workqueue_struct *sky1_wdt_wq;
};

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
			__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static unsigned int timeout;
module_param(timeout, uint, 0);
MODULE_PARM_DESC(timeout, "Watchdog timeout in seconds (default="
			__MODULE_STRING(SKY1_WDT_DEFAULT_TIME) ")");

static const struct watchdog_info sky1_wdt_info = {
	.options  = WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE | WDIOF_KEEPALIVEPING,
	.identity = "sky1, watchdog",
};

static int sky1_wdt_restart(struct watchdog_device *wdd,
			  unsigned long action, void *data)
{
	struct sky1_wdt_device *wdt = watchdog_get_drvdata(wdd);
	u32 wcs_val;

	regmap_read(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, &wcs_val);
	if (!wcs_val & SKY1_WDT_WCS_ENABLE) {
		wcs_val |= SKY1_WDT_WCS_ENABLE;
		regmap_write(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, wcs_val);
	}

	/*
	 * CompareValue = SystemCounter[63:0] + ZeroExtend(WatchdogOffsetValue)
	 * TimeoutRefresh = SystemCounter > CompareValue.
	 */
	regmap_write(wdt->regmap_ctrl, SKY1_WDT_WOR_LOW_RW, 0);
	regmap_write(wdt->regmap_ctrl, SKY1_WDT_WOR_HIGH_RW, 0);

	/* wait for reset to assert... */
	mdelay(500);

	return 0;
}

static void __sky1_wdt_set_timeout(struct watchdog_device *wdd, unsigned int timeout)
{
	u32 wor_low_val, wor_high_val;
	u64 wor_val;
	struct sky1_wdt_device *wdt = watchdog_get_drvdata(wdd);

	regmap_read(wdt->regmap_ctrl, SKY1_WDT_WOR_LOW_RW, &wor_low_val);
	regmap_read(wdt->regmap_ctrl, SKY1_WDT_WOR_HIGH_RW, &wor_high_val);
	wor_val = wor_high_val;
	wor_val = wor_val << 32 | wor_low_val;

	wor_val +=(u64)(timeout) * WDOG_SEC_TO_COUNT;
	wor_low_val = (u32)wor_val;
	wor_high_val = wor_val >> 32;
	regmap_write(wdt->regmap_ctrl, SKY1_WDT_WOR_LOW_RW, wor_low_val);
	regmap_write(wdt->regmap_ctrl, SKY1_WDT_WOR_HIGH_RW, wor_high_val);
}

static inline void sky1_wdt_setup(struct watchdog_device *wdd)
{
	struct sky1_wdt_device *wdt = watchdog_get_drvdata(wdd);
	u32 wcs_val;

	/* Keep Watchdog Disabled */
	regmap_read(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, &wcs_val);
	wcs_val &= ~(SKY1_WDT_WCS_ENABLE|SKY1_WDT_WCS_WS0|SKY1_WDT_WCS_WS1);
	regmap_write(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, wcs_val);

	/* Strip the old watchdog Time-Out value */
	regmap_write(wdt->regmap_ctrl, SKY1_WDT_WOR_LOW_RW, 0);
	regmap_write(wdt->regmap_ctrl, SKY1_WDT_WOR_HIGH_RW, 0);

	/* Set the watchdog's Time-Out value */
	__sky1_wdt_set_timeout(wdd, wdd->timeout);

	/* enable the watchdog */
	regmap_read(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, &wcs_val);
	wcs_val |= SKY1_WDT_WCS_ENABLE;
	regmap_write(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, wcs_val);
}

static inline bool sky1_wdt_is_running(struct sky1_wdt_device *wdt)
{
	u32 val;

	regmap_read(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, &val);

	return val & SKY1_WDT_WCS_ENABLE;
}

static int sky1_wdt_ping(struct watchdog_device *wdd)
{
	struct sky1_wdt_device *wdt = watchdog_get_drvdata(wdd);

	dev_dbg(wdd->parent, "%s\n", __func__);

	/*
	 * An explicit watchdog refresh occurs :
	 * 1. Watchdog Refresh Register is written.
	 * 2. Watchdog Offset Register is written.
	 * 3. Watchdog Control and Status register is written.
	 */
	regmap_write(wdt->regmap_fresh, SKY1_WDT_WRR_RW, SKY1_WDT_WRR_REFRESH);

	return 0;
}

static int sky1_wdt_set_timeout(struct watchdog_device *wdd,
				  unsigned int timeout)
{
	unsigned int actual;

	dev_dbg(wdd->parent, "%s timeout: %d sec\n", __func__, timeout);

	actual = min(timeout, (unsigned int)(SKY1_WDT_MAX_TIME));
	__sky1_wdt_set_timeout(wdd, actual);
	wdd->timeout = actual;
	return 0;
}

/* disable watchdog timers reset */
static int wdt_disable(struct watchdog_device *wdd)
{
	struct sky1_wdt_device *wdt = watchdog_get_drvdata(wdd);
	u32 wcs_val;

	/* Keep Watchdog Disabled */
	regmap_read(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, &wcs_val);
	wcs_val &= ~SKY1_WDT_WCS_ENABLE;
	regmap_write(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, wcs_val);

	return 0;
}

static void sky1_wdt_mond(struct work_struct *work)
{
	struct sky1_wdt_device *wdt = NULL;

	wdt = container_of(work, struct sky1_wdt_device, sky1_wdt_delayed_work.work);
	sky1_wdt_ping(&wdt->wdd);

	if (cpu_online(0))
		queue_delayed_work_on(0, wdt->sky1_wdt_wq,
					&wdt->sky1_wdt_delayed_work,
					msecs_to_jiffies(wdt->kick_time * MSECS_TO_JIFFIES));
	else
		queue_delayed_work(wdt->sky1_wdt_wq,
					&wdt->sky1_wdt_delayed_work,
					msecs_to_jiffies(wdt->kick_time * MSECS_TO_JIFFIES));
}

static int sky1_wdt_start(struct watchdog_device *wdd)
{
	struct sky1_wdt_device *wdt = watchdog_get_drvdata(wdd);

	dev_dbg(wdd->parent, "%s\n", __func__);
	if (sky1_wdt_is_running(wdt))
		return sky1_wdt_set_timeout(wdd, wdd->timeout);

	sky1_wdt_setup(wdd);
	set_bit(WDOG_HW_RUNNING, &wdd->status);

	return sky1_wdt_ping(wdd);
}

static const struct watchdog_ops sky1_wdt_ops = {
	.owner = THIS_MODULE,
	.start = sky1_wdt_start,
	.ping  = sky1_wdt_ping,
	.set_timeout = sky1_wdt_set_timeout,
	.restart = sky1_wdt_restart,
};

static const struct regmap_config sky1_wdt_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
};

static int sky1_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sky1_wdt_device *wdt;
	struct watchdog_device *wdd;
	void __iomem *base_ctrl, *base_fresh;
	u32 val;
	int ret;

#ifdef CONFIG_CIX_DST
	if (check_himntn(HIMNTN_AP_WDT) == 0) {
		dev_err(dev, "ap watchdog is closed in nv!!!\n");
		return -1;
	}
#endif
	wdt = devm_kzalloc(dev, sizeof(*wdt), GFP_KERNEL);
	if (!wdt)
		return -ENOMEM;

	/* This is the watchdog control base.*/
	base_ctrl = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base_ctrl))
		return PTR_ERR(base_ctrl);

	wdt->regmap_ctrl = devm_regmap_init_mmio_clk(dev, NULL,
			base_ctrl, &sky1_wdt_regmap_config);
	if (IS_ERR(wdt->regmap_ctrl)) {
		dev_err(dev, "regmap ctrl init failed\n");
		return PTR_ERR(wdt->regmap_ctrl);
	}

	/* This is the watchdog refresh base.*/
	base_fresh = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(base_fresh))
		return PTR_ERR(base_fresh);

	wdt->regmap_fresh = devm_regmap_init_mmio_clk(dev, NULL,
			base_fresh, &sky1_wdt_regmap_config);
	if (IS_ERR(wdt->regmap_fresh)) {
		dev_err(dev, "regmap fresh init failed\n");
		return PTR_ERR(wdt->regmap_fresh);
	}

	/* Initialize struct watchdog_device. */
	wdd = &wdt->wdd;
	wdd->parent = dev;
	wdd->info = &sky1_wdt_info;
	wdd->ops = &sky1_wdt_ops;
	wdd->min_timeout = 1;
	wdd->timeout = SKY1_WDT_DEFAULT_TIME;
	wdd->max_hw_heartbeat_ms = SKY1_WDT_MAX_TIME;
	regmap_read(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, &val);
	wdd->bootstatus = val & SKY1_WDT_WCS_WS0 ? WDIOF_CARDRESET : 0;

	platform_set_drvdata(pdev, wdd);
	watchdog_set_drvdata(wdd, wdt);
	watchdog_set_nowayout(wdd, nowayout);
	watchdog_set_restart_priority(wdd, 128);
	watchdog_init_timeout(wdd, timeout, dev);

	if ((wdd->timeout >> 1) < WDG_FEED_MOMENT_ADJUST)
		wdt->kick_time = (wdd->timeout >> 1) - 1;
	else
		wdt->kick_time = ((wdd->timeout >> 1) - 1) / WDG_FEED_MOMENT_ADJUST; /* minus 1 from the total */

	INIT_DELAYED_WORK(&wdt->sky1_wdt_delayed_work, sky1_wdt_mond);
	wdt->sky1_wdt_wq = alloc_workqueue("wdt_wq", WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (wdt->sky1_wdt_wq == NULL) {
		dev_err(dev, "alloc workqueue failed\n");
		ret = -ENOMEM;
		goto probe_fail;
	}

	ret = devm_watchdog_register_device(dev, wdd);
	if (ret) {
		dev_err(dev, "watchdog_register_device() failed: %d\n", ret);
		goto workqueue_destroy;
	}

	if (cpu_online(0))
		queue_delayed_work_on(0, wdt->sky1_wdt_wq, &wdt->sky1_wdt_delayed_work, 0);
	else
		queue_delayed_work(wdt->sky1_wdt_wq, &wdt->sky1_wdt_delayed_work, 0);

	ret = sky1_wdt_start(wdd);
	if (ret)
		return ret;
	regmap_read(wdt->regmap_ctrl, SKY1_WDT_WCS_RW, &val);

	dev_info(dev, "registeration successful\n");

	return 0;

workqueue_destroy:
	if (wdt->sky1_wdt_wq != NULL) {
		destroy_workqueue(wdt->sky1_wdt_wq);
		wdt->sky1_wdt_wq = NULL;
	}
probe_fail:
	dev_err(dev, "Probe Failled!!!\n");
	return ret;
}

static void sky1_wdt_shutdown(struct platform_device *pdev)
{
	struct watchdog_device *wdd = platform_get_drvdata(pdev);
	struct sky1_wdt_device *wdt = watchdog_get_drvdata(wdd);

	if (sky1_wdt_is_running(wdt)) {
		/*
		 * We are running, configure max timeout before reboot
		 * will take place.
		 */
		__sky1_wdt_set_timeout(wdd, SKY1_WDT_MAX_TIME);
		sky1_wdt_ping(wdd);
		dev_crit(&pdev->dev, "Device shutdown: Expect reboot!\n");
	}
}

/* Disable watchdog if it is active */
static int sky1_wdt_suspend(struct device *dev)
{
	struct watchdog_device *wdd = dev_get_drvdata(dev);
	struct sky1_wdt_device *wdt = watchdog_get_drvdata(wdd);

	/* The watchdog IP block is running */
	if (sky1_wdt_is_running(wdt)) {
		/*
		 * Don't update wdd->timeout, we'll restore the current value
		 * during resume.
		 */
		__sky1_wdt_set_timeout(wdd, SKY1_WDT_MAX_TIME);
		sky1_wdt_ping(wdd);
		cancel_delayed_work(&wdt->sky1_wdt_delayed_work);
		wdt_disable(wdd);
	}

	dev_dbg(dev,"%s-\n", __func__);

	return 0;
}

static int sky1_wdt_resume(struct device *dev)
{
	struct watchdog_device *wdd = dev_get_drvdata(dev);
	struct sky1_wdt_device *wdt = watchdog_get_drvdata(wdd);

	if (watchdog_active(wdd) && !sky1_wdt_is_running(wdt)) {
		/*
		 * If the watchdog is still active and resumes
		 * from deep sleep state, need to restart the
		 * watchdog again.
		 */

		sky1_wdt_setup(wdd);

		if (cpu_online(0))
			queue_delayed_work_on(0, wdt->sky1_wdt_wq, &wdt->sky1_wdt_delayed_work, 0);
		else
			queue_delayed_work(wdt->sky1_wdt_wq, &wdt->sky1_wdt_delayed_work, 0);

	}
	dev_dbg(dev,"%s-\n", __func__);
	return 0;
}

static SIMPLE_DEV_PM_OPS(sky1_wdt_pm_ops, sky1_wdt_suspend,
		sky1_wdt_resume);

static const struct of_device_id sky1_wdt_device_of_match[] = {
	{ .compatible = "cix,sky1-wdt", .data = NULL },
	{ /* end node */ }
};
MODULE_DEVICE_TABLE(of, sky1_wdt_device_of_match);

static const struct acpi_device_id sky1_wdt_acpi_match[] = {
        { "CIXH1005", 0 },
        { }
};
MODULE_DEVICE_TABLE(acpi, sky1_wdt_acpi_match);

static struct platform_driver sky1_wdt_driver = {
	.shutdown       = sky1_wdt_shutdown,
	.probe          = sky1_wdt_probe,
	.driver         = {
		.name   = DRIVER_NAME,
		.pm	= &sky1_wdt_pm_ops,
		.of_match_table = of_match_ptr(sky1_wdt_device_of_match),
		.acpi_match_table = ACPI_PTR(sky1_wdt_acpi_match),
	},
};
module_platform_driver(sky1_wdt_driver);

MODULE_AUTHOR("Jerry Zhu <Jerry.Zhu@cixtech.com>");
MODULE_DESCRIPTION("Watchdog driver for SKY1 and later");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRIVER_NAME);

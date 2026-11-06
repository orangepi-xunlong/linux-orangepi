/*
 * An RTC driver for the CVITEK RTC.
 */
#include <linux/kernel.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/of.h>
#include <linux/version.h>
#include <asm/div64.h>
#include <linux/io.h>

/* CVITEK RTC registers */
#define CVI_RTC_ANA_CALIB				0x0
#define CVI_RTC_SEC_PULSE_GEN			0x4
#define CVI_RTC_ALARM_TIME				0x8
#define CVI_RTC_ALARM_ENABLE			0xC
#define CVI_RTC_SET_SEC_CNTR_VALUE		0x10
#define CVI_RTC_SET_SEC_CNTR_TRIG		0x14
#define CVI_RTC_SEC_CNTR_VALUE			0x18
#define CVI_RTC_APB_RDATA_SEL			0x3C
#define CVI_RTC_POR_DB_MAGIC_KEY		0x68
#define CVI_RTC_EN_PWR_WAKEUP			0xBC
#define CVI_RTC_PWR_DET_SEL				0x140

/* CVITEK RTC MACRO registers */
#define RTC_MACRO_DA_CLEAR_ALL			0x480
#define RTC_MACRO_DA_SOC_READY			0x48C
#define RTC_MACRO_RO_T					0x4A8
#define RTC_MACRO_RG_SET_T				0x498

/* CVITEK RTC CTRL registers */
#define CVI_RTC_FC_COARSE_EN			0x40
#define CVI_RTC_FC_COARSE_CAL			0x44
#define CVI_RTC_FC_FINE_EN				0x48
#define CVI_RTC_FC_FINE_CAL				0x50

#define RTC_SEC_MAX_VAL		0xFFFFFFFF

#define CVI_RTC_HANDLE_IRQ
#define CV_RTC_FINE_CALIB /* use rtc 32k calibration flow */

struct cvi_rtc_info {
	struct platform_device	*pdev;
	struct rtc_device	*rtc_dev;
	void __iomem		*rtc_base; /* NULL if not initialized. */
	void __iomem		*rtc_ctrl_base; /* NULL if not initialized. */
	struct clk		*clk;
	int			cvi_rtc_irq; /* alarm and periodic irq */
	spinlock_t		cvi_rtc_lock;
	struct delayed_work cvi_rtc_work;
};

static int cvi_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct cvi_rtc_info *info = dev_get_drvdata(dev);
	unsigned long sec;
	unsigned long sec_ro_t;
	unsigned long sl_irq_flags;

	spin_lock_irqsave(&info->cvi_rtc_lock, sl_irq_flags);

	sec = readl(info->rtc_base + CVI_RTC_SEC_CNTR_VALUE);
	sec_ro_t = readl(info->rtc_base + RTC_MACRO_RO_T);

	if (sec_ro_t > 0x30000000) {
		sec = sec_ro_t;
		// Writeback to SEC CVI_RTC_SEC_CNTR_VALUE
		writel(sec, info->rtc_base + CVI_RTC_SET_SEC_CNTR_VALUE);
		writel(1, info->rtc_base + CVI_RTC_SET_SEC_CNTR_TRIG);
	} else if (sec < 0x30000000) {
		dev_err(NULL, "RTC invalid time\n");
	}

	spin_unlock_irqrestore(&info->cvi_rtc_lock, sl_irq_flags);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	rtc_time64_to_tm(sec, tm);
#else
	rtc_time_to_tm(sec, tm);
#endif

	dev_vdbg(dev, "%s %lu\n", __func__, sec);

	dev_notice(dev, "time read as %lu. %d/%d/%d %d:%02u:%02u\n",
		sec,
		tm->tm_mon + 1,
		tm->tm_mday,
		tm->tm_year + 1900,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec
	);

	return 0;
}

static int cvi_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct cvi_rtc_info *info = dev_get_drvdata(dev);
	unsigned long sec;
	int ret;
	unsigned long sl_irq_flags;

	/* convert tm to seconds. */
	ret = rtc_valid_tm(tm);
	if (ret)
		return ret;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	sec = rtc_tm_to_time64(tm);
#else
	rtc_tm_to_time(tm, &sec);
#endif
	dev_vdbg(dev, "%s %lu\n", __func__, sec);

	dev_notice(dev, "time set to %lu. %d/%d/%d %d:%02u:%02u\n",
		sec,
		tm->tm_mon+1,
		tm->tm_mday,
		tm->tm_year+1900,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec
	);

	spin_lock_irqsave(&info->cvi_rtc_lock, sl_irq_flags);

	writel(sec, info->rtc_base + CVI_RTC_SET_SEC_CNTR_VALUE);
	writel(1, info->rtc_base + CVI_RTC_SET_SEC_CNTR_TRIG);

	writel(sec, info->rtc_base + RTC_MACRO_RG_SET_T);

	writel(1, info->rtc_base + RTC_MACRO_DA_CLEAR_ALL);
	writel(1, info->rtc_base + RTC_MACRO_DA_SOC_READY);

	writel(0, info->rtc_base + RTC_MACRO_DA_CLEAR_ALL);
	writel(0, info->rtc_base + RTC_MACRO_RG_SET_T);
	writel(0, info->rtc_base + RTC_MACRO_DA_SOC_READY);

	spin_unlock_irqrestore(&info->cvi_rtc_lock, sl_irq_flags);

	return ret;
}

static int cvi_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct cvi_rtc_info *info = dev_get_drvdata(dev);

	dev_notice(dev, "CVITEK current time %d\n", readl(info->rtc_base + CVI_RTC_SEC_CNTR_VALUE));
	dev_notice(dev, "CVITEK current alarm %d\n", readl(info->rtc_base + CVI_RTC_ALARM_TIME));

	alarm->enabled = readl(info->rtc_base + CVI_RTC_ALARM_ENABLE) & 0x1;

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	rtc_time64_to_tm(readl(info->rtc_base + CVI_RTC_ALARM_TIME), &alarm->time);
#else
	rtc_time_to_tm(readl(info->rtc_base + CVI_RTC_ALARM_TIME), &alarm->time);
#endif
	return 0;
}

static int cvi_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct cvi_rtc_info *info = dev_get_drvdata(dev);

	writel(enabled ? 0x1 : 0x0, info->rtc_base + CVI_RTC_ALARM_ENABLE);
	return 0;
}

static int cvi_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct cvi_rtc_info *info = dev_get_drvdata(dev);
	unsigned long alarm_time;
	uint32_t wakeup_src_mask;

	alarm_time = rtc_tm_to_time64(&alarm->time);
	dev_notice(dev, "CVITEK set alarm %lu\n", alarm_time);
	dev_notice(dev, "CVITEK current time %d\n", readl(info->rtc_base + CVI_RTC_SEC_CNTR_VALUE));

	if (alarm_time > RTC_SEC_MAX_VAL)
		return -EINVAL;

	writel(0x0, info->rtc_base + CVI_RTC_ALARM_ENABLE);
	dev_notice(dev, "CVITEK CVI_RTC_ALARM_ENABLE %x\n", readl(info->rtc_base + CVI_RTC_ALARM_ENABLE));
	udelay(200);

	writel((u32)alarm_time, info->rtc_base + CVI_RTC_ALARM_TIME);
	writel(0x1, info->rtc_base + CVI_RTC_APB_RDATA_SEL);
	writel(0x1, info->rtc_base + CVI_RTC_ALARM_ENABLE);
	wakeup_src_mask = readl(info->rtc_base + CVI_RTC_EN_PWR_WAKEUP) | 0x30;
	writel(wakeup_src_mask, info->rtc_base + CVI_RTC_EN_PWR_WAKEUP);
	readl(info->rtc_base + CVI_RTC_SEC_CNTR_VALUE);
	dev_notice(dev, "CVITEK CVI_RTC_ALARM_ENABLE %x\n", readl(info->rtc_base + CVI_RTC_ALARM_ENABLE));

	return 0;
}

static int cvi_rtc_proc(struct device *dev, struct seq_file *seq)
{
	if (!dev || !dev->driver)
		return 0;

	seq_printf(seq, "name\t\t: %s\n", dev_name(dev));

	return 0;
}

#if defined(CVI_RTC_HANDLE_IRQ)
static irqreturn_t cvi_rtc_irq_handler(int irq, void *data)
{
	struct cvi_rtc_info *info = dev_get_drvdata((struct device *)data);

	writel(0x0, info->rtc_base + CVI_RTC_ALARM_ENABLE);//Clear interrupt bit
	schedule_delayed_work(&info->cvi_rtc_work, 0);
	return IRQ_HANDLED;
}

static void cvi_rtc_irq_work(struct work_struct *work)
{
	struct cvi_rtc_info *info = container_of(work, struct cvi_rtc_info, cvi_rtc_work.work);
	struct rtc_device *rtc = info->rtc_dev;
	struct rtc_wkalrm alrm;

	/* Update the alarm_IRQ state in /proc */
	rtc_read_alarm(rtc, &alrm);
	alrm.enabled = 0;
	rtc_set_alarm(rtc, &alrm);
}
#endif

static const struct rtc_class_ops cvi_rtc_ops = {
	.read_time	= cvi_rtc_read_time,
	.set_time	= cvi_rtc_set_time,
	.read_alarm	= cvi_rtc_read_alarm,
	.set_alarm	= cvi_rtc_set_alarm,
	.proc		= cvi_rtc_proc,
	.alarm_irq_enable = cvi_rtc_alarm_irq_enable,
};

static const struct of_device_id cvi_rtc_dt_match[] = {
	{ .compatible = "cvitek,rtc", },
	{}
};
MODULE_DEVICE_TABLE(of, cvi_rtc_dt_match);

static void rtc_enable_sec_counter(struct cvi_rtc_info *info)
{
	uint32_t value = 0;

	value = readl(info->rtc_base + CVI_RTC_SEC_PULSE_GEN) & ~(1 << 31);
	writel(value, info->rtc_base + CVI_RTC_SEC_PULSE_GEN);

	value = readl(info->rtc_base + CVI_RTC_ANA_CALIB) & ~(1 << 31);
	writel(value, info->rtc_base + CVI_RTC_ANA_CALIB);

	readl(info->rtc_base + CVI_RTC_SEC_CNTR_VALUE);
	writel(0x0, info->rtc_base + CVI_RTC_ALARM_ENABLE);
}

#if defined(CV_RTC_FINE_CALIB)
static void rtc_32k_coarse_value_calib(struct cvi_rtc_info *info)
{
	uint32_t analog_calib_value = 0;
	uint32_t fc_coarse_time1 = 0;
	uint32_t fc_coarse_time2 = 0;
	uint32_t fc_coarse_value = 0;
	uint32_t offset = 128;
	uint32_t value = 0;

	writel(0x10100, info->rtc_base + CVI_RTC_ANA_CALIB);
	udelay(200);

	// Select 32K OSC tuning value source from rtc_sys
	value = readl(info->rtc_base + CVI_RTC_SEC_PULSE_GEN) & ~(1 << 31);
	writel(value, info->rtc_base + CVI_RTC_SEC_PULSE_GEN);

	analog_calib_value = readl(info->rtc_base + CVI_RTC_ANA_CALIB);
	// dev_notice(NULL, "RTC_ANA_CALIB: 0x%x\n", analog_calib_value);

	writel(1, info->rtc_ctrl_base + CVI_RTC_FC_COARSE_EN);

	while (1) {
		fc_coarse_time1 = readl(info->rtc_ctrl_base + CVI_RTC_FC_COARSE_CAL);
		fc_coarse_time1 >>= 16;
		// dev_notice(NULL, "fc_coarse_time1 = 0x%x\n", fc_coarse_time1);
		// dev_notice(NULL, "fc_coarse_time2 = 0x%x\n", fc_coarse_time2);

		while (fc_coarse_time2 <= fc_coarse_time1) {
			fc_coarse_time2 = readl(info->rtc_ctrl_base + CVI_RTC_FC_COARSE_CAL);
			fc_coarse_time2 >>= 16;
			// dev_notice(NULL, "fc_coarse_time2 = 0x%x\n", fc_coarse_time2);
		}

		udelay(400);
		fc_coarse_value = readl(info->rtc_ctrl_base + CVI_RTC_FC_COARSE_CAL);
		fc_coarse_value &= 0xFFFF;
		// dev_notice(NULL, "fc_coarse_value = 0x%x\n", fc_coarse_value);

		if (fc_coarse_value > 770) {
			analog_calib_value += offset;
			offset >>= 1;
			writel(analog_calib_value, info->rtc_base + CVI_RTC_ANA_CALIB);
		} else if (fc_coarse_value < 755) {
			analog_calib_value -= offset;
			offset >>= 1;
			writel(analog_calib_value, info->rtc_base + CVI_RTC_ANA_CALIB);
		} else {
			writel(0, info->rtc_ctrl_base + CVI_RTC_FC_COARSE_EN);
			// dev_notice(NULL, "RTC coarse calib done\n");
			break;
		}
		if (offset == 0) {
			dev_err(NULL, "RTC calib failed\n");
			break;
		}
		// dev_notice(NULL, "RTC_ANA_CALIB: 0x%x\n", analog_calib_value);
	}
}

static void rtc_32k_fine_value_calib(struct cvi_rtc_info *info)
{
	uint32_t fc_fine_time1 = 0;
	uint32_t fc_fine_time2 = 0;
	uint32_t fc_fine_value = 0;
	uint64_t freq = 256000000000;
	uint32_t sec_cnt;
	uint32_t frac_ext = 10000;

	writel(1, info->rtc_ctrl_base + CVI_RTC_FC_FINE_EN);

	fc_fine_time1 = readl(info->rtc_ctrl_base + CVI_RTC_FC_FINE_CAL);
	fc_fine_time1 >>= 24;
	// dev_notice(NULL, "fc_fine_time1 = 0x%x\n", fc_fine_time1);

	while (fc_fine_time2 <= fc_fine_time1) {
		fc_fine_time2 = readl(info->rtc_ctrl_base + CVI_RTC_FC_FINE_CAL);
		fc_fine_time2 >>= 24;
		// dev_notice(NULL, "fc_fine_time2 = 0x%x\n", fc_fine_time2);
	}

	fc_fine_value = readl(info->rtc_ctrl_base + CVI_RTC_FC_FINE_CAL);
	fc_fine_value &= 0xFFFFFF;
	// dev_notice(NULL, "fc_fine_value = 0x%x\n", fc_fine_value);

	// Frequency = 256 / (RTC_FC_FINE_VALUE x 40ns)
	// freq = 256000000000 / 40;
	// freq = (freq * frac_ext) / fc_fine_value;
	// dev_notice(NULL, "freq = %u\n", freq);

	// sec_cnt = ((freq / frac_ext) << 8) + (((freq % frac_ext) * 256) / frac_ext & 0xFF);
	// dev_notice(NULL, "sec_cnt = 0x%x\n", sec_cnt);

	/* Fix a data size overflow error. From jinyu.zhao */
	do_div(freq, 40);
	freq = freq * frac_ext;
	do_div(freq, fc_fine_value);
	sec_cnt =  ((do_div(freq, frac_ext) * 256) / frac_ext & 0xFF) + (freq << 8);

	writel(sec_cnt, info->rtc_base + CVI_RTC_SEC_PULSE_GEN);
	writel(0, info->rtc_ctrl_base + CVI_RTC_FC_FINE_EN);
}
#endif

static int __init cvi_rtc_probe(struct platform_device *pdev)
{
	struct cvi_rtc_info *info;
	struct resource *res;
	int ret;

	info = devm_kzalloc(&pdev->dev, sizeof(struct cvi_rtc_info),
		GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->rtc_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(info->rtc_base))
		return PTR_ERR(info->rtc_base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	info->rtc_ctrl_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(info->rtc_ctrl_base))
		return PTR_ERR(info->rtc_ctrl_base);

	info->cvi_rtc_irq = platform_get_irq(pdev, 0);
	if (info->cvi_rtc_irq <= 0)
		return -EBUSY;

	info->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(info->clk))
		return PTR_ERR(info->clk);

	ret = clk_prepare_enable(info->clk);
	if (ret < 0)
		return ret;

	/* set context info. */
	info->pdev = pdev;
	spin_lock_init(&info->cvi_rtc_lock);

	platform_set_drvdata(pdev, info);

	device_init_wakeup(&pdev->dev, 1);

	info->rtc_dev = devm_rtc_device_register(&pdev->dev,
				dev_name(&pdev->dev), &cvi_rtc_ops,
				THIS_MODULE);
	if (IS_ERR(info->rtc_dev)) {
		ret = PTR_ERR(info->rtc_dev);
		dev_err(&pdev->dev, "Unable to register device (err=%d).\n",
			ret);
		goto disable_clk;
	}

#if defined(CVI_RTC_HANDLE_IRQ)
	ret = devm_request_irq(&pdev->dev, info->cvi_rtc_irq,
			cvi_rtc_irq_handler, IRQF_TRIGGER_HIGH,
			dev_name(&pdev->dev), &pdev->dev);
	if (ret) {
		dev_err(&pdev->dev,
			"Unable to request interrupt for device (err=%d).\n",
			ret);
		goto disable_clk;
	}
	INIT_DELAYED_WORK(&info->cvi_rtc_work, cvi_rtc_irq_work);
#endif

#if defined(CV_RTC_FINE_CALIB)
	if ((readl(info->rtc_ctrl_base + 0x8) & 0x400) == 0x0) {
		/* Enable calibration only when use internal osc */
		rtc_32k_coarse_value_calib(info);
		rtc_32k_fine_value_calib(info);
		dev_notice(&pdev->dev, "rtc 32k calibration has been completed\n");
	} else {
		dev_notice(&pdev->dev, "Disable calibration because using external xtal\n");
	}

#endif

	rtc_enable_sec_counter(info);

	dev_notice(&pdev->dev, "CVITEK real time clock\n");

	return 0;

disable_clk:
	clk_disable_unprepare(info->clk);
	return ret;
}

static int cvi_rtc_remove(struct platform_device *pdev)
{
	struct cvi_rtc_info *info = platform_get_drvdata(pdev);

	clk_disable_unprepare(info->clk);
#if defined(CVI_RTC_HANDLE_IRQ)
	cancel_delayed_work(&info->cvi_rtc_work);
#endif

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int cvi_rtc_suspend(struct device *dev)
{
	return 0;
}

static int cvi_rtc_resume(struct device *dev)
{
	struct cvi_rtc_info *info = dev_get_drvdata(dev);

	writel(0x0, info->rtc_base + CVI_RTC_ALARM_ENABLE);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(cvi_rtc_pm_ops, cvi_rtc_suspend, cvi_rtc_resume);

static void cvi_rtc_shutdown(struct platform_device *pdev)
{
	dev_vdbg(&pdev->dev, "disabling interrupts.\n");
	cvi_rtc_alarm_irq_enable(&pdev->dev, 0);
}

MODULE_ALIAS("platform:cvi_rtc");
static struct platform_driver cvi_rtc_driver = {
	.remove		= cvi_rtc_remove,
	.shutdown	= cvi_rtc_shutdown,
	.driver		= {
		.name	= "cvi_rtc",
		.of_match_table = cvi_rtc_dt_match,
		.pm	= &cvi_rtc_pm_ops,
	},
};

module_platform_driver_probe(cvi_rtc_driver, cvi_rtc_probe);

MODULE_AUTHOR("Mark Hsieh <mark.hsieh@cvitek.com>");
MODULE_DESCRIPTION("driver for CVITEK RTC");
MODULE_LICENSE("GPL");

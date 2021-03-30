/*
 * An RTC driver for Allwinner A10/A20/A64
 *
 *  Copyright (c) 2013, Carlo Caione <carlo.caione@gmail.com>
 *
 *  2015-11-25  Support sun8iw10 platform, and fixup get/set years
 *		errors by Sugar<shuge@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/rtc.h>
#include <linux/types.h>
#include <linux/bootup_extend.h>

#include "rtc-sunxi.h"

static struct sunxi_rtc_data_year data_year_param[] = {
	[0] = {
		.min		= 2010,
		.max		= 2073,
		.mask		= 0x3f,
		.yshift		= 16,
		.leap_shift	= 22,
	},
	[1] = {
		.min		= 1970,
		.max		= 2225,
		.mask		= 0xff,
		.yshift		= 16,
		.leap_shift	= 24,
	},
	[2] = {
		.min		= 2010,
		.max		= 2137,
		.mask		= 0x7f,
		.yshift		= 15,
		.leap_shift	= 22,
	},
	[3] = {
		.min		= 1970,
		.max		= 2097,
		.mask		= 0x7f,
		.yshift		= 16,
		.leap_shift	= 23,
	},
};

#ifdef CONFIG_RTC_SHUTDOWN_ALARM
static int alarm_in_booting = 0;
module_param_named(alarm_in_booting, alarm_in_booting, int, S_IRUGO | S_IWUSR);

static void sunxi_rtc_alarm_in_boot(struct sunxi_rtc_dev *rtc)
{
	unsigned int cnt, cur, en, int_ctrl, int_stat;

	/*
	 * when alarm irq occur at boot0~rtc_driver.probe() process in shutdown
	 * charger mode, /charger in userspace must know this irq through sysfs
	 * node 'alarm_in_booting' to reboot and startup system.
	 * */
	cnt = readl(rtc->base + SUNXI_ALRM_COUNTER);
	cur = readl(rtc->base + SUNXI_ALRM_CURRENT);
	en = readl(rtc->base + SUNXI_ALRM_EN);
	int_ctrl = readl(rtc->base + SUNXI_ALRM_IRQ_EN);
	int_stat = readl(rtc->base + SUNXI_ALRM_IRQ_STA);
	if (int_stat && int_ctrl && en && (cnt <= cur))
		alarm_in_booting = 1;
}
#endif

#ifdef SUNXI_RTC_DYNAMIC_YEAR_RANGE
void sunxi_rtc_set_dynamic_year_range(struct sunxi_rtc_data_year *data)
{
	s8 year_str[16] = "";
	u32 min_year = 0;
	u32 max_year = 0;

	strncpy(year_str, __DATE__, 16);
	kstrtou32(&year_str[strlen(year_str) - 4], 10, &min_year);
	max_year = min_year - data->min + data->max;
	data->min = min_year;
	data->max = max_year;
	pr_debug("Set dynamic range: [%d, %d]\n", min_year, max_year);
}
#endif

static int sunxi_rtc_wait(struct sunxi_rtc_dev *chip, int offset,
			  unsigned int mask, unsigned int ms_timeout)
{
	const unsigned long timeout = jiffies + msecs_to_jiffies(ms_timeout);
	u32 reg;

	do {
		reg = readl(chip->base + offset);
		reg &= mask;

		if (reg == mask)
			return 0;

	} while (time_before(jiffies, timeout));

	return -ETIMEDOUT;
}

static int sunxi_rtc_verify_ymd(struct sunxi_rtc_dev *rtc)
{
	u32 date = readl(rtc->base + SUNXI_RTC_YMD);

	if ((SUNXI_DATE_GET_DAY_VALUE(date) != 0)
		&& (SUNXI_DATE_GET_MON_VALUE(date) != 0))
		return 0;

	/* If rtc YY-MM-DD registr is error, set it to the initial date */
	date = SUNXI_DATE_SET_DAY_VALUE(1) |
		SUNXI_DATE_SET_MON_VALUE(1)  |
		SUNXI_DATE_SET_YEAR_VALUE(0, rtc->data_year);

	if (is_leap_year(rtc->data_year->min))
		date |= SUNXI_LEAP_SET_VALUE(1, rtc->data_year->leap_shift);

	writel(0, rtc->base + SUNXI_RTC_YMD);
	writel(date, rtc->base + SUNXI_RTC_YMD);

	if (sunxi_rtc_wait(rtc, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_YMD_ACC, 50)) {
		pr_err("Failed to set vaild hardware RTC date\n");
		return -EIO;
	}

	pr_info("Set vaild hardware RTC date %04d-%02d-%02d\n",
			rtc->data_year->min
			+ SUNXI_DATE_GET_YEAR_VALUE(date, rtc->data_year),
			SUNXI_DATE_GET_MON_VALUE(date),
			SUNXI_DATE_GET_DAY_VALUE(date));

	return 0;
}

static irqreturn_t sunxi_rtc_alarmirq(int irq, void *id)
{
	struct sunxi_rtc_dev *chip = (struct sunxi_rtc_dev *) id;
	u32 val;

	val = readl(chip->base + SUNXI_ALRM_IRQ_STA);

	if (val & SUNXI_ALRM_IRQ_STA_CNT_IRQ_PEND) {
		val |= SUNXI_ALRM_IRQ_STA_CNT_IRQ_PEND;
		writel(val, chip->base + SUNXI_ALRM_IRQ_STA);

		rtc_update_irq(chip->rtc, 1, RTC_AF | RTC_IRQF);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static void sunxi_rtc_setaie(int to, struct sunxi_rtc_dev *chip)
{
	u32 alrm_val = 0;
	u32 alrm_irq_val = 0;
	u32 alrm_config = 0;

	if (to) {
		alrm_val = readl(chip->base + SUNXI_ALRM_EN);
		alrm_val |= SUNXI_ALRM_EN_CNT_EN;

		alrm_irq_val = readl(chip->base + SUNXI_ALRM_IRQ_EN);
		alrm_irq_val |= SUNXI_ALRM_IRQ_EN_CNT_IRQ_EN;

		alrm_config = SUNXI_ALRM_WAKEUP_OUTPUT_EN;
	} else {
		writel(SUNXI_ALRM_IRQ_STA_CNT_IRQ_PEND,
				chip->base + SUNXI_ALRM_IRQ_STA);
	}

	writel(alrm_val, chip->base + SUNXI_ALRM_EN);
	writel(alrm_irq_val, chip->base + SUNXI_ALRM_IRQ_EN);
	writel(alrm_config, chip->base + SUNXI_ALARM_CONFIG);
}

#if defined(CONFIG_SUNXI_BOOTUP_EXTEND)
int sunxi_rtc_set_bootup_extend_mode(int mode)
{
	struct device_node *np = NULL;
	struct sunxi_rtc_dev *chip = NULL;
	struct platform_device *pdev;

	np = of_find_compatible_node(NULL, NULL, "allwinner,sun50i-rtc");
	if (!np) {
		pr_err("ERROR! can't get rtc node\n");
		return -1;
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		pr_err("of_find_device_by_node %s failed\n", __func__);
		return -1;
	}

	chip = platform_get_drvdata(pdev);

	if (mode == SUNXI_BOOTUP_EXTEND_MODE_POWEROFF)
		writel(SUNXI_BOOTUP_EXTEND_RTC_POWEROFF,
			chip->base + SUNXI_GP_DATA_REG2);
	else if (mode == SUNXI_BOOTUP_EXTEND_MODE_RESTART)
		writel(SUNXI_BOOTUP_EXTEND_RTC_RESTART,
			chip->base + SUNXI_GP_DATA_REG2);
	else
		pr_err("%s unkonwn mode=%d\n", __func__, mode);
	return 0;
}
EXPORT_SYMBOL(sunxi_rtc_set_bootup_extend_mode);
#endif

static int sunxi_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm);

static int sunxi_rtc_getalarm(struct device *dev, struct rtc_wkalrm *wkalrm)
{
	struct sunxi_rtc_dev *chip = dev_get_drvdata(dev);
	struct rtc_time *alrm_tm = &wkalrm->time;
	u32 alrm_en;
#ifdef SUNXI_ALARM1_USED
	u32 alrm;
	u32 date;
#else
	u32 alarm_cur = 0, alarm_cnt = 0;
	unsigned long alarm_seconds = 0;
#endif
	int ret;

#ifdef SUNXI_ALARM1_USED
	alrm = readl(chip->base + SUNXI_ALRM_DHMS);
	date = readl(chip->base + SUNXI_RTC_YMD);

	alrm_tm->tm_sec = SUNXI_ALRM_GET_SEC_VALUE(alrm);
	alrm_tm->tm_min = SUNXI_ALRM_GET_MIN_VALUE(alrm);
	alrm_tm->tm_hour = SUNXI_ALRM_GET_HOUR_VALUE(alrm);

	alrm_tm->tm_mday = SUNXI_DATE_GET_DAY_VALUE(date);
	alrm_tm->tm_mon = SUNXI_DATE_GET_MON_VALUE(date);
	alrm_tm->tm_year = SUNXI_DATE_GET_YEAR_VALUE(date, chip->data_year);

	alrm_tm->tm_mon -= 1;

	/*
	 * switch from (data_year->min)-relative offset to
	 * a (1900)-relative one
	 */
	alrm_tm->tm_year += SUNXI_YEAR_OFF(chip->data_year);
#else
	alarm_cnt = readl(chip->base + SUNXI_ALRM_COUNTER);
	alarm_cur = readl(chip->base + SUNXI_ALRM_CURRENT);

	dev_dbg(dev, "alarm_cnt: %d, alarm_cur: %d\n", alarm_cnt, alarm_cur);
	if (alarm_cur > alarm_cnt) {
		/* alarm is disabled. */
		wkalrm->enabled = 0;
		alrm_tm->tm_mon = -1;
		alrm_tm->tm_mday = -1;
		alrm_tm->tm_year = -1;
		alrm_tm->tm_hour = -1;
		alrm_tm->tm_min = -1;
		alrm_tm->tm_sec = -1;
		return 0;
	}

	ret = sunxi_rtc_gettime(dev, alrm_tm);
	if (ret)
		return -EINVAL;

	rtc_tm_to_time(alrm_tm, &alarm_seconds);
	alarm_cnt = (alarm_cnt - alarm_cur);
	alarm_cur = 0;
	alarm_seconds += alarm_cnt;

	rtc_time_to_tm(alarm_seconds, alrm_tm);
	dev_dbg(dev, "alarm_seconds: %ld\n", alarm_seconds);
#endif

	alrm_en = readl(chip->base + SUNXI_ALRM_IRQ_EN);
	if (alrm_en & SUNXI_ALRM_EN_CNT_EN)
		wkalrm->enabled = 1;

	return 0;
}

static int sunxi_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	struct sunxi_rtc_dev *chip = dev_get_drvdata(dev);
	u32 date, time;

	/*
	 * read again in case it changes
	 */
	do {
		date = readl(chip->base + SUNXI_RTC_YMD);
		time = readl(chip->base + SUNXI_RTC_HMS);
	} while ((date != readl(chip->base + SUNXI_RTC_YMD)) ||
		 (time != readl(chip->base + SUNXI_RTC_HMS)));

	rtc_tm->tm_sec  = SUNXI_TIME_GET_SEC_VALUE(time);
	rtc_tm->tm_min  = SUNXI_TIME_GET_MIN_VALUE(time);
	rtc_tm->tm_hour = SUNXI_TIME_GET_HOUR_VALUE(time);

	rtc_tm->tm_mday = SUNXI_DATE_GET_DAY_VALUE(date);
	rtc_tm->tm_mon  = SUNXI_DATE_GET_MON_VALUE(date);
	rtc_tm->tm_year = SUNXI_DATE_GET_YEAR_VALUE(date, chip->data_year);

	rtc_tm->tm_mon  -= 1;

	/*
	 * switch from (data_year->min)-relative offset to
	 * a (1900)-relative one
	 */
	rtc_tm->tm_year += SUNXI_YEAR_OFF(chip->data_year);

	dev_dbg(dev, "Read hardware RTC time %04d-%02d-%02d %02d:%02d:%02d\n",
			rtc_tm->tm_year + 1900, rtc_tm->tm_mon + 1, rtc_tm->tm_mday,
			rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	return rtc_valid_tm(rtc_tm);
}

static int sunxi_rtc_setalarm(struct device *dev, struct rtc_wkalrm *wkalrm)
{
	struct sunxi_rtc_dev *chip = dev_get_drvdata(dev);
	struct rtc_time *alrm_tm = &wkalrm->time;
	struct rtc_time tm_now;
	u32 alrm = 0;
	unsigned long time_now = 0;
	unsigned long time_set = 0;
	unsigned long time_gap = 0;
	unsigned long time_gap_day = 0;
#ifdef SUNXI_ALARM1_USED
	unsigned long time_gap_hour = 0;
	unsigned long time_gap_min = 0;
#endif
	int ret = 0;

	ret = sunxi_rtc_gettime(dev, &tm_now);
	if (ret < 0) {
		dev_err(dev, "Error in getting time\n");
		return -EINVAL;
	}

	rtc_tm_to_time(alrm_tm, &time_set);
	rtc_tm_to_time(&tm_now, &time_now);
	if (time_set <= time_now) {
		dev_err(dev, "Date to set in the past\n");
		return -EINVAL;
	}

	time_gap = time_set - time_now;
	time_gap_day = time_gap / SEC_IN_DAY;
#ifdef SUNXI_ALARM1_USED
	time_gap -= time_gap_day * SEC_IN_DAY;
	time_gap_hour = time_gap / SEC_IN_HOUR;
	time_gap -= time_gap_hour * SEC_IN_HOUR;
	time_gap_min = time_gap / SEC_IN_MIN;
	time_gap -= time_gap_min * SEC_IN_MIN;
#endif

	if (time_gap_day > 255) {
		dev_err(dev, "Day must be in the range 0 - 255\n");
		return -EINVAL;
	}

	sunxi_rtc_setaie(0, chip);
#ifdef SUNXI_ALARM1_USED
	writel(0, chip->base + SUNXI_ALRM_DHMS);
	usleep_range(100, 300);

	alrm = SUNXI_ALRM_SET_SEC_VALUE(time_gap) |
		SUNXI_ALRM_SET_MIN_VALUE(time_gap_min) |
		SUNXI_ALRM_SET_HOUR_VALUE(time_gap_hour) |
		SUNXI_ALRM_SET_DAY_VALUE(time_gap_day);

	writel(alrm, chip->base + SUNXI_ALRM_DHMS);
#else
	writel(0, chip->base + SUNXI_ALRM_COUNTER);
	alrm = time_gap;

	dev_dbg(dev, "set alarm seconds:%d enable:%d\n", alrm, wkalrm->enabled);
	writel(alrm, chip->base + SUNXI_ALRM_COUNTER);
#endif

	writel(0, chip->base + SUNXI_ALRM_IRQ_EN);
	writel(SUNXI_ALRM_IRQ_EN_CNT_IRQ_EN, chip->base + SUNXI_ALRM_IRQ_EN);

	sunxi_rtc_setaie(wkalrm->enabled, chip);

	return 0;
}

static int sunxi_rtc_settime(struct device *dev, struct rtc_time *rtc_tm)
{
	struct sunxi_rtc_dev *chip = dev_get_drvdata(dev);
	u32 date = 0;
	u32 time = 0;
	int year;

	/*
	 * the input rtc_tm->tm_year is the offset relative to 1900. We use
	 * the SUNXI_YEAR_OFF macro to rebase it with respect to the min year
	 * allowed by the hardware
	 */

	year = rtc_tm->tm_year + 1900;
	if (rtc_valid_tm(rtc_tm) || year < chip->data_year->min \
			|| year > chip->data_year->max) {
		dev_err(dev, "rtc only supports year in range %d - %d\n",
				chip->data_year->min, chip->data_year->max);
		return -EINVAL;
	}

	rtc_tm->tm_year -= SUNXI_YEAR_OFF(chip->data_year);
	rtc_tm->tm_mon += 1;

	dev_dbg(dev, "Will set hardware RTC time %04d-%02d-%02d %02d:%02d:%02d\n",
			rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,
			rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	date = SUNXI_DATE_SET_DAY_VALUE(rtc_tm->tm_mday) |
		SUNXI_DATE_SET_MON_VALUE(rtc_tm->tm_mon)  |
		SUNXI_DATE_SET_YEAR_VALUE(rtc_tm->tm_year, chip->data_year);

	if (is_leap_year(year))
		date |= SUNXI_LEAP_SET_VALUE(1, chip->data_year->leap_shift);

	time = SUNXI_TIME_SET_SEC_VALUE(rtc_tm->tm_sec)  |
		SUNXI_TIME_SET_MIN_VALUE(rtc_tm->tm_min)  |
		SUNXI_TIME_SET_HOUR_VALUE(rtc_tm->tm_hour);

	writel(0, chip->base + SUNXI_RTC_HMS);
	writel(0, chip->base + SUNXI_RTC_YMD);

	writel(time, chip->base + SUNXI_RTC_HMS);

	/*
	 * After writing the RTC HH-MM-SS register, the
	 * SUNXI_LOSC_CTRL_RTC_HMS_ACC bit is set and it will not
	 * be cleared until the real writing operation is finished
	 */

	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_HMS_ACC, 50)) {
		dev_err(dev, "Failed to set rtc time.\n");
		return -1;
	}

	writel(date, chip->base + SUNXI_RTC_YMD);

	/*
	 * After writing the RTC YY-MM-DD register, the
	 * SUNXI_LOSC_CTRL_RTC_YMD_ACC bit is set and it will not
	 * be cleared until the real writing operation is finished
	 */

	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_YMD_ACC, 50)) {
		dev_err(dev, "Failed to set rtc time.\n");
		return -1;
	}

	return 0;
}

static int sunxi_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct sunxi_rtc_dev *chip = dev_get_drvdata(dev);

	if (!enabled)
		sunxi_rtc_setaie(enabled, chip);

	return 0;
}

static const struct rtc_class_ops sunxi_rtc_ops = {
	.read_time		= sunxi_rtc_gettime,
	.set_time		= sunxi_rtc_settime,
	.read_alarm		= sunxi_rtc_getalarm,
	.set_alarm		= sunxi_rtc_setalarm,
	.alarm_irq_enable	= sunxi_rtc_alarm_irq_enable
};

static const struct of_device_id sunxi_rtc_dt_ids[] = {
	{ .compatible = "allwinner,sun4i-a10-rtc", .data = &data_year_param[0] },
	{ .compatible = "allwinner,sun7i-a20-rtc", .data = &data_year_param[1] },
	{ .compatible = "allwinner,sun8iw10-rtc", .data = &data_year_param[2] },
	{ .compatible = "allwinner,sun8iw11p1-rtc", .data = &data_year_param[3] },
	{ .compatible = "allwinner,sun50i-rtc", .data = &data_year_param[0] },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sunxi_rtc_dt_ids);

static ssize_t sunxi_rtc_min_year_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct sunxi_rtc_dev *rtc_dev = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d \n", rtc_dev->data_year->min);
}
static struct device_attribute sunxi_rtc_min_year_attr =
	__ATTR(min_year, S_IRUGO, sunxi_rtc_min_year_show, NULL);

static ssize_t sunxi_rtc_max_year_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct sunxi_rtc_dev *rtc_dev = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%d \n", rtc_dev->data_year->max);
}
static struct device_attribute sunxi_rtc_max_year_attr =
	__ATTR(max_year, S_IRUGO, sunxi_rtc_max_year_show, NULL);

static int sunxi_rtc_probe(struct platform_device *pdev)
{
	struct sunxi_rtc_dev *chip;
	struct resource *res;
	const struct of_device_id *of_id;
	int ret;
	unsigned int tmp_data;

	of_id = of_match_device(sunxi_rtc_dt_ids, &pdev->dev);
	if (!of_id) {
		dev_err(&pdev->dev, "Unable to setup RTC data\n");
		return -ENODEV;
	}

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	platform_set_drvdata(pdev, chip);
	chip->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->base = devm_ioremap_resource(&pdev->dev, res);

	if (IS_ERR(chip->base))
		return PTR_ERR(chip->base);

	/* Enable the clock/module so that we can access the registers */
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	chip->data_year = (struct sunxi_rtc_data_year *) of_id->data;
#ifdef SUNXI_RTC_DYNAMIC_YEAR_RANGE
	sunxi_rtc_set_dynamic_year_range(chip->data_year);
#endif

	/* verify hardware rtc YY-MM-DD register */
	ret = sunxi_rtc_verify_ymd(chip);
	if (ret) {
		dev_err(&pdev->dev, "failed to set vaild rtc date\n");
		goto fail;
	}

#ifdef CONFIG_RTC_SHUTDOWN_ALARM
	sunxi_rtc_alarm_in_boot(chip);
#else

	/*
	 * to support RTC shutdown alarm, we should not clear alarm for android
	 * will restart in charge mode.
	 * alarm will be cleared by android in normal start mode.
	 * */
	/* clear the alarm count value */
#ifdef SUNXI_ALARM1_USED
	writel(0, chip->base + SUNXI_ALRM_DHMS);
#else
	writel(0, chip->base + SUNXI_ALRM_COUNTER);
#endif

	/* disable alarm, not generate irq pending */
	writel(0, chip->base + SUNXI_ALRM_EN);

	/* disable alarm week/cnt irq, unset to cpu */
	writel(0, chip->base + SUNXI_ALRM_IRQ_EN);

	/* clear alarm week/cnt irq pending */
	writel(SUNXI_ALRM_IRQ_STA_CNT_IRQ_PEND, chip->base +
			SUNXI_ALRM_IRQ_STA);
#endif
	/* clear alarm wakeup output */
	writel(SUNXI_ALRM_WAKEUP_OUTPUT_EN, chip->base +
	       SUNXI_ALARM_CONFIG);
	/*
	 * select RTC clock source, use default auto switch mode
	 * and set Crystal oscillator strength
	 */
	tmp_data = REG_CLK32K_AUTO_SWT_EN | EXT_LOSC_GSM;
	writel(tmp_data, chip->base + SUNXI_LOSC_CTRL);
	device_init_wakeup(&pdev->dev, 1);

	chip->rtc = devm_rtc_device_register(&pdev->dev, "sunxi-rtc",
	                                &sunxi_rtc_ops, THIS_MODULE);
	if (IS_ERR(chip->rtc)) {
		dev_err(&pdev->dev, "unable to register device\n");
		goto fail;
	}

	chip->irq = platform_get_irq(pdev, 0);
	if (chip->irq < 0) {
		dev_err(&pdev->dev, "No IRQ resource\n");
		goto fail;
	}

	ret = devm_request_irq(&pdev->dev, chip->irq, sunxi_rtc_alarmirq,
	                       0, dev_name(&pdev->dev), chip);
	if (ret) {
		dev_err(&pdev->dev, "Could not request IRQ\n");
		goto fail;
	}
	dev_info(&pdev->dev, "RTC enabled\n");

	device_create_file(&pdev->dev, &sunxi_rtc_min_year_attr);
	device_create_file(&pdev->dev, &sunxi_rtc_max_year_attr);

	return 0;

fail:
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return -EIO;
}

static int sunxi_rtc_remove(struct platform_device *pdev)
{
	struct sunxi_rtc_dev *chip = platform_get_drvdata(pdev);

	device_remove_file(&pdev->dev, &sunxi_rtc_min_year_attr);
	device_remove_file(&pdev->dev, &sunxi_rtc_max_year_attr);

	devm_rtc_device_unregister(chip->dev, chip->rtc);

	/* Disable the clock/module */
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static struct platform_driver sunxi_rtc_driver = {
	.probe		= sunxi_rtc_probe,
	.remove		= sunxi_rtc_remove,
	.driver		= {
		.name		= "sunxi-rtc",
		.owner		= THIS_MODULE,
		.of_match_table = sunxi_rtc_dt_ids,
	},
};

module_platform_driver(sunxi_rtc_driver);

MODULE_DESCRIPTION("sunxi RTC driver");
MODULE_AUTHOR("Carlo Caione <carlo.caione@gmail.com>");
MODULE_LICENSE("Dual BSD/GPL");

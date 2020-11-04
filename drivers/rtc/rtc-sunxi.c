/*
 * An RTC driver for Sunxi Platform of Allwinner SoC
 *
 * Copyright (c) 2013, Carlo Caione <carlo.caione@gmail.com>
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
#include <linux/reboot.h>
#include <linux/pm_wakeirq.h>
#include <linux/hwspinlock.h>
#include "rtc-sunxi.h"

static void __iomem *global_pgregbase;

atomic_t rtc_sync_flag = ATOMIC_INIT(0);

static struct sunxi_rtc_data_year data_year_param =
#if (defined CONFIG_ARCH_SUN50IW1)
	{
		.min		= 2010,
		.max		= 2073,
		.mask		= 0x3f,
		.yshift		= 16,
		.leap_shift	= 22,
	};
#else
	{
		.min		= 1970,
		.max		= 2097,
		.mask		= 0x7f,
		.yshift		= 16,
		.leap_shift	= 23,
	};
#endif

#ifdef CONFIG_RTC_SHUTDOWN_ALARM
static int alarm_in_booting;
module_param_named(alarm_in_booting, alarm_in_booting, int, S_IRUGO | S_IWUSR);

static void sunxi_rtc_alarm_in_boot(struct sunxi_rtc_dev *rtc)
{
	unsigned int cnt, cur, en, int_ctrl, int_stat;

	/*
	 * when alarm irq occur at boot0~rtc_driver.probe() process in shutdown
	 * charger mode, /charger in userspace must know this irq through sysfs
	 * node 'alarm_in_booting' to reboot and startup system.
	 */
	cnt = readl(rtc->base + SUNXI_ALRM_COUNTER);
	cur = readl(rtc->base + SUNXI_ALRM_CURRENT);
	en = readl(rtc->base + SUNXI_ALRM_EN);
	int_ctrl = readl(rtc->base + SUNXI_ALRM_IRQ_EN);
	int_stat = readl(rtc->base + SUNXI_ALRM_IRQ_STA);
	if (int_stat && int_ctrl && en && (cnt <= cur))
		alarm_in_booting = 1;
}
#endif

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

	if (to) {
		alrm_val = readl(chip->base + SUNXI_ALRM_EN);
		alrm_val |= SUNXI_ALRM_EN_CNT_EN;

		alrm_irq_val = readl(chip->base + SUNXI_ALRM_IRQ_EN);
		alrm_irq_val |= SUNXI_ALRM_IRQ_EN_CNT_IRQ_EN;
	} else {
		writel(SUNXI_ALRM_IRQ_STA_CNT_IRQ_PEND,
				chip->base + SUNXI_ALRM_IRQ_STA);
	}

	writel(alrm_val, chip->base + SUNXI_ALRM_EN);
	writel(alrm_irq_val, chip->base + SUNXI_ALRM_IRQ_EN);
}

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
	int ret;
#endif

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

#ifdef SUNXI_SIMPLIFIED_TIMER
static short month_days[2][13] = {
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};


static int sunxi_rtc_day_to_ymd(struct rtc_time *rtc_tm, u32 min_year,
								u32 udate)
{
	static u32 last_date;
	static int last_year, last_mon, last_mday;
	int year = 0, leap, i;
	int date = (int)udate;

	if (date == last_date) {
		rtc_tm->tm_mday = last_mday;
		rtc_tm->tm_mon = last_mon;
		rtc_tm->tm_year = last_year;
		return 0;
	}

	year = min_year;
	while (1) {
		if (is_leap_year(year)) {
			if (date > 366) {
				year++;
				date -= 366;
			} else
				break;
		} else {
			if (date > 365) {
				year++;
				date -= 365;
			} else
				break;
		}
	}
	rtc_tm->tm_year = year - 1900;
	last_year = rtc_tm->tm_year;

	leap = is_leap_year(rtc_tm->tm_year);
	for (i = 1; date > month_days[leap][i]; i++)
		date -= month_days[leap][i];
	rtc_tm->tm_mon = i;
	last_mon = rtc_tm->tm_mon;
	rtc_tm->tm_mday = date;
	last_mday = rtc_tm->tm_mday;

	return 0;
}
#endif

static int sunxi_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	struct sunxi_rtc_dev *chip = dev_get_drvdata(dev);
	u32 date, time;

	/*
	 * we must wait 500ms for rtc reg sync when power up first,
	 * include reset and standby up
	 */
	if (atomic_read(&rtc_sync_flag))
		msleep(500);
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

#ifndef SUNXI_SIMPLIFIED_TIMER
	rtc_tm->tm_mday = SUNXI_DATE_GET_DAY_VALUE(date);
	rtc_tm->tm_mon  = SUNXI_DATE_GET_MON_VALUE(date);
	rtc_tm->tm_year = SUNXI_DATE_GET_YEAR_VALUE(date, chip->data_year);

	/*
	 * switch from (data_year->min)-relative offset to
	 * a (1900)-relative one
	 */
	rtc_tm->tm_year += SUNXI_YEAR_OFF(chip->data_year);
#else
	sunxi_rtc_day_to_ymd(rtc_tm, chip->data_year->min, date);
#endif
	rtc_tm->tm_mon  -= 1;

	dev_dbg(dev, "Read hardware RTC time %04d-%02d-%02d %02d:%02d:%02d\n",
		rtc_tm->tm_year + 1900, rtc_tm->tm_mon + 1, rtc_tm->tm_mday,
		rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	return rtc_valid_tm(rtc_tm);
}

static int sunxi_rtc_wait(struct sunxi_rtc_dev *chip, int offset,
			  unsigned int mask, unsigned int ms_timeout)
{
	const unsigned long timeout = jiffies + msecs_to_jiffies(ms_timeout);
	u32 reg;

	do {
		reg = readl(chip->base + offset);
		reg &= mask;

		if (reg != mask)
			return 0;

	} while (time_before(jiffies, timeout));

	return -ETIMEDOUT;
}

static int sunxi_rtc_setalarm(struct device *dev, struct rtc_wkalrm *wkalrm)
{
	struct sunxi_rtc_dev *chip = dev_get_drvdata(dev);
	struct rtc_time *alrm_tm = &wkalrm->time;
	struct rtc_time tm_now;
	u32 alrm;
	time64_t diff;
	unsigned long time_gap;
	unsigned long time_gap_day;
#if defined(SUNXI_ALARM1_USED)
	unsigned long time_gap_hour = 0;
	unsigned long time_gap_min = 0;
#endif
	int ret;
	ret = sunxi_rtc_gettime(dev, &tm_now);
	if (ret < 0) {
		dev_err(dev, "Error in getting time\n");
		return -EINVAL;
	}

	diff = rtc_tm_sub(alrm_tm, &tm_now);
	if (diff <= 0) {
		dev_err(dev, "Date to set in the past\n");
		return -EINVAL;
	}

	if (diff > 255 * SEC_IN_DAY) {
		dev_err(dev, "Day must be in the range 0 - 255\n");
		return -EINVAL;
	}

	time_gap = diff;
	time_gap_day = alrm_tm->tm_mday - tm_now.tm_mday;
#ifdef SUNXI_SIMPLIFIED_TIMER
	sunxi_rtc_setaie(0, chip);

#ifdef CONFIG_ARCH_SUN8IW16
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_ALARM_ACC, 50)) {
		dev_err(dev, "Failed to set rtc alarm1.\n");
		return -1;
	}
#endif
	writel(0, chip->base + SUNXI_ALRM_DAY);

#ifdef CONFIG_ARCH_SUN8IW16
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_ALARM_ACC, 50)) {
		dev_err(dev, "Failed to set rtc alarm1.\n");
		return -1;
	}
	msleep(2);
#endif

	writel(0, chip->base + SUNXI_ALRM_HMS);

#ifdef CONFIG_ARCH_SUN8IW16
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_ALARM_ACC, 50)) {
		dev_err(dev, "Failed to set rtc alarm1.\n");
		return -1;
	}
	msleep(2);
#endif
	writel(time_gap_day + readl(chip->base + SUNXI_RTC_YMD),
					chip->base + SUNXI_ALRM_DAY);

	alrm = SUNXI_ALRM_SET_SEC_VALUE(alrm_tm->tm_sec) |
		SUNXI_ALRM_SET_MIN_VALUE(alrm_tm->tm_min) |
		SUNXI_ALRM_SET_HOUR_VALUE(alrm_tm->tm_hour);

#ifdef CONFIG_ARCH_SUN8IW16
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_ALARM_ACC, 50)) {
		dev_err(dev, "Failed to set rtc alarm1.\n");
		return -1;
	}
	msleep(2);
#endif
	writel(alrm, chip->base + SUNXI_ALRM_HMS);

#ifdef CONFIG_ARCH_SUN8IW16
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_ALARM_ACC, 50)) {
		dev_err(dev, "Failed to set rtc alarm1.\n");
		return -1;
	}
	msleep(2);
#endif

#else
#ifdef SUNXI_ALARM1_USED
	time_gap -= time_gap_day * SEC_IN_DAY;
	time_gap_hour = time_gap / SEC_IN_HOUR;
	time_gap -= time_gap_hour * SEC_IN_HOUR;
	time_gap_min = time_gap / SEC_IN_MIN;
	time_gap -= time_gap_min * SEC_IN_MIN;
#endif /* end of SUNXI_ALARM1_USED */

	sunxi_rtc_setaie(0, chip);
#ifdef SUNXI_ALARM1_USED
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_ALARM_ACC, 50)) {
		dev_err(dev, "Failed to set rtc alarm1.\n");
		return -1;
	}
	msleep(2);

	writel(0, chip->base + SUNXI_ALRM_DHMS);
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_ALARM_ACC, 50)) {
		dev_err(dev, "Failed to set rtc alarm1.\n");
		return -1;
	}
	msleep(2);

	alrm = SUNXI_ALRM_SET_SEC_VALUE(time_gap) |
		SUNXI_ALRM_SET_MIN_VALUE(time_gap_min) |
		SUNXI_ALRM_SET_HOUR_VALUE(time_gap_hour) |
		SUNXI_ALRM_SET_DAY_VALUE(time_gap_day);
	writel(alrm, chip->base + SUNXI_ALRM_DHMS);
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_ALARM_ACC, 50)) {
		dev_err(dev, "Failed to set rtc alarm1.\n");
		return -1;
	}
	msleep(2);
#else
	writel(0, chip->base + SUNXI_ALRM_COUNTER);
	alrm = time_gap;

	dev_dbg(dev, "set alarm seconds:%d enable:%d\n", alrm, wkalrm->enabled);
	writel(alrm, chip->base + SUNXI_ALRM_COUNTER);
#endif /* end of SUNXI_ALARM1_USED */

#endif /* end of SUNXI_SIMPLIFIED_TIMER */

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
#ifdef SUNXI_SIMPLIFIED_TIMER
	int i, leap;
#endif
	/*
	 * the input rtc_tm->tm_year is the offset relative to 1900. We use
	 * the SUNXI_YEAR_OFF macro to rebase it with respect to the min year
	 * allowed by the hardware
	 */

	year = rtc_tm->tm_year + 1900;
	if (rtc_valid_tm(rtc_tm) || year < chip->data_year->min
			|| year > chip->data_year->max) {
		dev_err(dev, "rtc only supports year in range %d - %d\n",
				chip->data_year->min, chip->data_year->max);
		return -EINVAL;
	}

#ifndef SUNXI_SIMPLIFIED_TIMER
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
#else
	date = rtc_tm->tm_mday;
	rtc_tm->tm_mon += 1;
	leap = is_leap_year(year);
	for (i = 1; i < rtc_tm->tm_mon; i++)
		date += month_days[leap][i];

	for (i = year - 1; i >= chip->data_year->min; i--) {
		if (is_leap_year(i))
			date += 366;
		else
			date += 365;
	}
#endif

	time = SUNXI_TIME_SET_SEC_VALUE(rtc_tm->tm_sec)  |
		SUNXI_TIME_SET_MIN_VALUE(rtc_tm->tm_min)  |
		SUNXI_TIME_SET_HOUR_VALUE(rtc_tm->tm_hour);


	/*
	 * before we write the RTC HH-MM-SS register,we
	 * should check the SUNXI_LOSC_CTRL_RTC_HMS_ACC bit
	 */
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_HMS_ACC, 50)) {
		dev_err(dev, "Failed to set rtc time.\n");
		return -1;
	}

	/*
	 * we must wait at least 2ms to make sure the bit clear really.
	 */
	msleep(2);

	writel(0, chip->base + SUNXI_RTC_HMS);
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

	msleep(2);
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
	msleep(2);

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
	msleep(2);

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
	msleep(2);

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
	{.compatible = "allwinner,sunxi-rtc", .data = &data_year_param},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sunxi_rtc_dt_ids);

static ssize_t sunxi_rtc_min_year_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct sunxi_rtc_dev *rtc_dev = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%u \n", rtc_dev->data_year->min);
}
static struct device_attribute sunxi_rtc_min_year_attr =
	__ATTR(min_year, S_IRUGO, sunxi_rtc_min_year_show, NULL);

static ssize_t sunxi_rtc_max_year_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct sunxi_rtc_dev *rtc_dev = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "%u \n", rtc_dev->data_year->max);
}
static struct device_attribute sunxi_rtc_max_year_attr =
	__ATTR(max_year, S_IRUGO, sunxi_rtc_max_year_show, NULL);

#ifdef CONFIG_SUNXI_BOOTUP_EXTEND
static int sunxi_reboot_callback(struct notifier_block *this,
		unsigned long code, void *data)
{
	unsigned int rtc_flag = 0;

	if (data == NULL)
		return NOTIFY_DONE;

	pr_info("sunxi rtc reboot, arg %s\n", (char *)data);

	if (!strncmp(data, "debug", sizeof("debug"))) {
		rtc_flag = SUNXI_DEBUG_MODE_FLAG;
	} else if (!strncmp(data, "efex", sizeof("efex"))) {
		rtc_flag = SUNXI_EFEX_CMD_FLAG;
	} else if (!strncmp(data, "boot-resignature",
						sizeof("boot-resignature"))) {
		rtc_flag = SUNXI_BOOT_RESIGNATURE_FLAG;
	} else if (!strncmp(data, "recovery", sizeof("recovery"))
		|| !strncmp(data, "boot-recovery", sizeof("boot-recovery"))) {
		rtc_flag = SUNXI_BOOT_RECOVERY_FLAG;
	} else if (!strncmp(data, "sysrecovery", sizeof("sysrecovery"))) {
		rtc_flag = SUNXI_SYS_RECOVERY_FLAG;
	} else if (!strncmp(data, "bootloader", sizeof("bootloader"))) {
		rtc_flag = SUNXI_FASTBOOT_FLAG;
	} else if (!strncmp(data, "usb-recovery", sizeof("usb-recovery"))) {
		rtc_flag = SUNXI_USB_RECOVERY_FLAG;
	} else if (!strncmp(data, "uboot", sizeof("uboot"))) {
		rtc_flag = SUNXI_UBOOT_FLAG;
	} else {
		pr_warn("unkown reboot arg %s", (char *)data);
		return NOTIFY_DONE;
	}

	/*write the data to reg*/
	writel(rtc_flag, global_pgregbase);
	return NOTIFY_DONE;
}


static struct notifier_block sunxi_reboot_notifier = {
	.notifier_call = sunxi_reboot_callback,
};
#endif

#ifdef CONFIG_SUNXI_FAKE_POWEROFF
static struct hwspinlock *intc_mgr_hwlock;
static int bootup_extend_enabel;
static void __iomem *rtc_base_addr;

#define GENERAL_RTC_REG_MAX   (0x03)
#define GENERAL_RTC_REG_MIN   (0x01)

#define INTC_HWSPINLOCK_TIMEOUT      (4000)

static int __rtc_reg_write(u32 addr, u32 data)
{
	unsigned long hwflags;

	if ((addr < GENERAL_RTC_REG_MIN) || (addr > GENERAL_RTC_REG_MAX)) {
		printk(KERN_ERR "%s: rtc address error, address:0x%x\n", __func__, addr);
		return -1;
	}

	if (data > 0xff) {
		printk(KERN_ERR "%s: rtc data error, data:0x%x\n", __func__, data);
		return -1;
	}

	if (!hwspin_lock_timeout_irqsave(intc_mgr_hwlock, INTC_HWSPINLOCK_TIMEOUT, &hwflags)) {

		if (rtc_base_addr != NULL) {
			writel(data, (rtc_base_addr + 0x100 + 0x4 * addr));
		} else {
			printk(KERN_ERR "%s: rtc rtc_base_addr error, data:0x%x\n", __func__, data);
		}

		hwspin_unlock_irqrestore(intc_mgr_hwlock, &hwflags);
		printk(KERN_DEBUG "%s: write rtc reg success, rtc reg 0x%x:0x%x\n", __func__, addr, data);
		return 0;
	}

	printk(KERN_DEBUG "%s: get hwspinlock unsuccess\n", __func__);
	return -1;
}

void sunxi_bootup_extend_fix(unsigned int *cmd)
{
	if (bootup_extend_enabel == 1) {
		if (*cmd == LINUX_REBOOT_CMD_POWER_OFF) {
			__rtc_reg_write(2, 0x2);
			*cmd = LINUX_REBOOT_CMD_RESTART;
			printk(KERN_INFO "will enter boot_start_os\n");
		} else if (*cmd == LINUX_REBOOT_CMD_RESTART ||
			*cmd == LINUX_REBOOT_CMD_RESTART2) {
			printk(KERN_INFO "not enter boot_start_os\n");
			__rtc_reg_write(2, 0xf);
		}
	}
}
EXPORT_SYMBOL(sunxi_bootup_extend_fix);

#endif

static int sunxi_rtc_probe(struct platform_device *pdev)
{
	struct sunxi_rtc_dev *chip;
	struct resource *res;
	const struct of_device_id *of_id;
	int ret;
	unsigned int tmp_data;

#ifdef CONFIG_SUNXI_BOOTUP_EXTEND
	u32 gpr_offset = 0;
	u32 gpr_len = 0;
	u32 gpr_num = 0;
#endif
	global_pgregbase = NULL;
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

#ifdef CONFIG_RTC_SHUTDOWN_ALARM
	sunxi_rtc_alarm_in_boot(chip);
#else
	/*
	 * to support RTC shutdown alarm, we should not clear alarm for android
	 * will restart in charge mode.
	 * alarm will be cleared by android in normal start mode.
	 */
	/* clear the alarm count value */
#ifdef SUNXI_ALARM1_USED
	writel(0, chip->base + SUNXI_ALRM_DHMS);
	if (sunxi_rtc_wait(chip, SUNXI_LOSC_CTRL,
				SUNXI_LOSC_CTRL_RTC_ALARM_ACC, 50)) {
		dev_err(dev, "Failed to set rtc alarm1.\n");
		return -1;
	}
	udelay(100);
#else
	writel(0, chip->base + SUNXI_ALRM_COUNTER);
#endif

#ifdef SUNXI_RTC_COMP_CTRL
	tmp_data = readl(chip->base + SUNXI_RTC_COMP_CTRL);
	tmp_data |= SUNXI_COMP_ENABLE;
	tmp_data &= ~(SUNXI_ADC_VDD_ON_DISABLE);
	writel(tmp_data, chip->base + SUNXI_RTC_COMP_CTRL);
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

	if (!of_property_read_bool(pdev->dev.of_node, "auto_switch")) {

		/*
		 * Step1: select RTC clock source
		 */
		tmp_data = readl(chip->base + SUNXI_LOSC_CTRL);
		tmp_data &= (~REG_CLK32K_AUTO_SWT_EN);

		/* Disable auto switch function */
		tmp_data |= REG_CLK32K_AUTO_SWT_BYPASS;
		writel(tmp_data, chip->base + SUNXI_LOSC_CTRL);

		tmp_data = readl(chip->base + SUNXI_LOSC_CTRL);
		tmp_data |= (RTC_SOURCE_EXTERNAL | REG_LOSCCTRL_MAGIC);
		writel(tmp_data, chip->base + SUNXI_LOSC_CTRL);

		/* We need to set GSM after change clock source */
		udelay(10);
		tmp_data = readl(chip->base + SUNXI_LOSC_CTRL);
		tmp_data |= (EXT_LOSC_GSM | REG_LOSCCTRL_MAGIC);
		writel(tmp_data, chip->base + SUNXI_LOSC_CTRL);
	} else {
		/* enable auto switch function manual
		 * because of in some case,we boot with auto switch function disable,
		 * and want to reboot to enable the auto switch function,
		 * but the rtc default value does not change unless vcc-rtc disable
		 * so we should not depend on the default value of reg.
		 */
		tmp_data = readl(chip->base + SUNXI_LOSC_CTRL);
		tmp_data &= (~REG_CLK32K_AUTO_SWT_BYPASS);
		tmp_data |= REG_CLK32K_AUTO_SWT_EN;
		tmp_data |= (RTC_SOURCE_EXTERNAL | REG_LOSCCTRL_MAGIC);
		writel(tmp_data, chip->base + SUNXI_LOSC_CTRL);
#ifdef SUNXI_RTC_CALI_REG
		/* enable cali for 32k */
		tmp_data = readl(chip->base + SUNXI_RTC_CALI_REG);
		tmp_data |= (REG_CLK32K_CALI_FUNC_EN | REG_CLK32K_CALI_EN);
		writel(tmp_data, chip->base + SUNXI_RTC_CALI_REG);

#ifdef SUNXI_RTC_XO_CTRL
		/*enable dcxo */
		tmp_data = readl(chip->base + SUNXI_RTC_XO_CTRL);
		tmp_data |= REG_DC_XO_EN;
		writel(tmp_data, chip->base + SUNXI_RTC_XO_CTRL);
#endif

#ifdef SUNXI_RTC_VDD_REG
		tmp_data = readl(chip->base + SUNXI_RTC_VDD_REG);
		tmp_data |= REG_V_SEL;
		writel(tmp_data, chip->base + SUNXI_RTC_VDD_REG);
#endif

#endif
	}

	chip->irq = platform_get_irq(pdev, 0);
	if (chip->irq < 0) {
		dev_err(&pdev->dev, "No IRQ resource\n");
		goto fail;
	}

	if (of_property_read_bool(pdev->dev.of_node, "wakeup-source")) {
		device_init_wakeup(&pdev->dev, true);
		dev_pm_set_wake_irq(&pdev->dev, chip->irq);
	}


	chip->rtc = devm_rtc_device_register(&pdev->dev, "sunxi-rtc",
			&sunxi_rtc_ops, THIS_MODULE);
	if (IS_ERR(chip->rtc)) {
		dev_err(&pdev->dev, "unable to register device\n");
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

#ifdef CONFIG_SUNXI_FAKE_POWEROFF

	rtc_base_addr = chip->base;
	intc_mgr_hwlock = hwspin_lock_request_specific(SUNXI_INTC_HWSPINLOCK);
	if (!intc_mgr_hwlock) {
		printk(KERN_ERR "%s,%d request hwspinlock faild!\n", __func__, __LINE__);
		return 0;
	}

	bootup_extend_enabel = 1;
	printk(KERN_INFO "%s: bootup extend state %d\n", __func__, bootup_extend_enabel);
#endif

#ifdef CONFIG_SUNXI_BOOTUP_EXTEND
		ret = of_property_read_u32(pdev->dev.of_node,
						"gpr_offset", &gpr_offset);
		if (ret) {
			dev_err(&pdev->dev, "Could not get Gpr offset\n");
			goto fail_3;
		}

		ret = of_property_read_u32(pdev->dev.of_node,
						"gpr_len", &gpr_len);
		if (ret) {
			dev_err(&pdev->dev, "Could not get Gpr len\n");
			goto fail_3;
		}

		ret = of_property_read_u32(pdev->dev.of_node,
						"gpr_cur_pos", &gpr_num);
		if (ret) {
			dev_err(&pdev->dev, "Could not get Gpr reboot cur pos");
			goto fail_3;
		} else {

			if (gpr_num >= gpr_len) {
				dev_err(&pdev->dev,
					"gpr_cur_pos is out of range!\n");
				goto fail_3;
			}
			/*
			  * This notification function is for monitoring reboot command
			  * when the system has been started, the reboot parameter is
			  * stored in the RTC General Purpose register.
			  *
			  * gpr_offset:  General Purpose register's offset
			  * gpr_len: The number of General Purpose registers
			  * gpr_cur_pos: which to store the parameter in
			  * General Purpose register
			 */
			ret = register_reboot_notifier(&sunxi_reboot_notifier);
			if (ret) {
				dev_err(&pdev->dev,
					"register reboot notifier error %d\n", ret);
				goto fail_3;
			}
			global_pgregbase = chip->base + gpr_offset + 0x4 * gpr_num;
		}

		return 0;

fail_3:
		device_remove_file(&pdev->dev, &sunxi_rtc_min_year_attr);
		device_remove_file(&pdev->dev, &sunxi_rtc_max_year_attr);
		devm_free_irq(&pdev->dev, chip->irq, chip);
#else
		return 0;
#endif

fail:
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	return -EIO;
}

static int sunxi_rtc_remove(struct platform_device *pdev)
{
	struct sunxi_rtc_dev *chip = platform_get_drvdata(pdev);
#ifdef CONFIG_SUNXI_BOOTUP_EXTEND
	unregister_reboot_notifier(&sunxi_reboot_notifier);
#endif
	device_remove_file(&pdev->dev, &sunxi_rtc_min_year_attr);
	device_remove_file(&pdev->dev, &sunxi_rtc_max_year_attr);

	devm_rtc_device_unregister(chip->dev, chip->rtc);

	/* Disable the clock/module */
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static void sunxi_rtc_shutdown(struct platform_device *pdev)
{
#ifdef SUNXI_RTC_COMP_CTRL
	u32 tmp_val;

	struct sunxi_rtc_dev *chip = platform_get_drvdata(pdev);

	tmp_val = readl(chip->base + SUNXI_RTC_COMP_CTRL);
	tmp_val &= ~(SUNXI_COMP_ENABLE);
	tmp_val |= SUNXI_ADC_VDD_ON_DISABLE;
	writel(tmp_val, chip->base + SUNXI_RTC_COMP_CTRL);
#endif
}

#ifdef CONFIG_PM_SLEEP
/* timer use to wait rec reg sync when wakeup */
struct hrtimer rtc_sync_timer;

static enum hrtimer_restart rtc_hrtimer_trig_handler(struct hrtimer *timer)
{
	atomic_dec(&rtc_sync_flag);
	return HRTIMER_NORESTART;
}

static int sunxi_rtc_suspend(struct device *dev)
{
	atomic_inc(&rtc_sync_flag);
	return 0;
}

static int sunxi_rtc_resume(struct device *dev)
{

	hrtimer_init(&rtc_sync_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	rtc_sync_timer.function = rtc_hrtimer_trig_handler;

	hrtimer_start(&rtc_sync_timer, ms_to_ktime(500), HRTIMER_MODE_REL);
	return 0;
}

const struct dev_pm_ops sunxi_rtc_pm_ops = {
	.suspend = sunxi_rtc_suspend,
	.resume = sunxi_rtc_resume,
};
#else
const struct dev_pm_ops sunxi_rtc_pm_ops = {
	.suspend = NULL,
	.resume = NULL,
};
#endif

static struct platform_driver sunxi_rtc_driver = {
	.probe		= sunxi_rtc_probe,
	.remove		= sunxi_rtc_remove,
	.shutdown   = sunxi_rtc_shutdown,
	.driver		= {
		.name		= "sunxi-rtc",
		.pm		= &sunxi_rtc_pm_ops,
		.of_match_table = sunxi_rtc_dt_ids,
	},
};

module_platform_driver(sunxi_rtc_driver);

MODULE_DESCRIPTION("sunxi RTC driver");
MODULE_AUTHOR("Carlo Caione <carlo.caione@gmail.com>");
MODULE_LICENSE("GPL");

/*
 * drivers\rtc\rtc-sunxi.c
 * (C) Copyright 2010-2016
 * ruinerwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#if defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN9IW1P1)
#include "rtc-sunxi-external.c"
#else
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <mach/platform.h>


#ifdef	SUNXI_RTC_VBASE
#define SUNXI_VA_TIMERC_IO_BASE		SUNXI_RTC_VBASE
#else
#define SUNXI_VA_TIMERC_IO_BASE		0xf1f00000
#endif

#define SUNXI_LOSC_CTRL_REG		(0x00)

#define SUNXI_RTC_DATE_REG		(0x10)
#define SUNXI_RTC_TIME_REG		(0x14)

#define SUNXI_RTC_ALARM_COUNTER_REG	(0x20)
#define SUNXI_RTC_ALARM_CURRENT_REG	(0x24)
#define SUNXI_ALARM_EN_REG		(0x28)
#define SUNXI_ALARM_INT_CTRL_REG	(0x2c)
#define SUNXI_ALARM_INT_STATUS_REG	(0x30)
#define SUNXI_ALARM_CONFIG		(0x50)
#define SUNXI_LOSC_OUT_GATING		(0x60)
#define SUNXI_GPDATA_REG(x)		(0x100 + ((x) << 2)) /* x: 0 ~ 7 */

#define SUNXI_INTOSC_CLK_PRESCAL_REG	(0x08)
#define INTOSC_PERSCAL					(15)

/*rtc count interrupt control*/
#define RTC_ALARM_COUNT_INT_EN		0x00000001

#define RTC_ENABLE_CNT_IRQ		0x00000001

/*Crystal Control*/
#define REG_LOSCCTRL_MAGIC		0x16aa0000
#define REG_CLK32K_AUTO_SWT_EN		0x00004000
#define RTC_SOURCE_EXTERNAL		0x00000001
#define RTC_HHMMSS_ACCESS		0x00000100
#define RTC_YYMMDD_ACCESS		0x00000080
#define EXT_LOSC_GSM			0x00000008

/*Date Value*/
#define DATE_GET_DAY_VALUE(x)		((x) & 0x0000001f)
#define DATE_GET_MON_VALUE(x)		(((x) & 0x00000f00) >> 8 )
#define DATE_GET_YEAR_VALUE(x)		(((x) & 0x003f0000) >> 16)

#define DATE_SET_DAY_VALUE(x)		DATE_GET_DAY_VALUE(x)
#define DATE_SET_MON_VALUE(x)		(((x) & 0x0000000f) << 8 )
#define DATE_SET_YEAR_VALUE(x)		(((x) & 0x0000003f) << 16)
#define LEAP_SET_VALUE(x)		(((x) & 0x00000001) << 22)

/*Time Value*/
#define TIME_GET_SEC_VALUE(x)		((x) & 0x0000003f)
#define TIME_GET_MIN_VALUE(x)		(((x) & 0x00003f00) >> 8 )
#define TIME_GET_HOUR_VALUE(x)		(((x) & 0x001f0000) >> 16)

#define TIME_SET_SEC_VALUE(x)		TIME_GET_SEC_VALUE(x)
#define TIME_SET_MIN_VALUE(x)		(((x) & 0x0000003f) << 8 )
#define TIME_SET_HOUR_VALUE(x)		(((x) & 0x0000001f) << 16)

#define SUNXI_ALARM

static void __iomem *sunxi_rtc_base;
static int sunxi_rtc_alarmno	= NO_IRQ;
static int losc_err_flag	= 0;

#define sunxi_rtc_read(addr)		readl(sunxi_rtc_base + (addr))
#define sunxi_rtc_write(val, addr)	writel(val, sunxi_rtc_base + (addr))

#ifdef CONFIG_ARCH_SUN8IW8
#define OSC_32K	32000
#define OSC_ORG	32768
/*
 * This RTC count flow on sun8iw8 platform.
 * 32.768KHz---   --- [32768 count] ---
 *              \/                     \___[seconds reg]
 *              /\                     /
 * 23KHz    ---   --- [32000 count] ---
 *
 * We can calculate how much seconds missing, call sunxi_rtc_fixup.
 */
void sunxi_rtc_fixup(unsigned long *org_time, unsigned long *time)
{
	unsigned long delta;
	unsigned long t1, t2;
	bool out;

	if (!time)
		return;

	if (!org_time) {
		t1 = (unsigned long long)sunxi_rtc_read(SUNXI_GPDATA_REG(6)) << 32;
		t1 |= (unsigned long)sunxi_rtc_read(SUNXI_GPDATA_REG(7));
	} else
		t1 = *org_time;

	t2 = *time;

	out = sunxi_rtc_read(SUNXI_LOSC_CTRL_REG) & RTC_SOURCE_EXTERNAL;

	delta = (t2 > t1) ? (t2 - t1) : (t1 - t2);
	delta = (delta * (OSC_ORG - OSC_32K)) / OSC_ORG;

	if (out)
		*time -= delta;
	else
		*time += delta;
}


void sunxi_rtc_save(unsigned long time, bool force)
{
	unsigned long long tsec = (unsigned long long)time;
	unsigned int save = (0x80000000 & sunxi_rtc_read(SUNXI_GPDATA_REG(6)));

	if (force || !save) {
		sunxi_rtc_write((unsigned int)tsec, SUNXI_GPDATA_REG(7));
		sunxi_rtc_write(((unsigned int)(tsec >> 32) | 0x80000000), SUNXI_GPDATA_REG(6));
	}
}

#else

#define sunxi_rtc_fixup(a, b) {}
#define sunxi_rtc_save(a, b) {}

#endif

#ifdef SUNXI_ALARM
/* IRQ Handlers, irq no. is shared with timer2 */
static irqreturn_t sunxi_rtc_alarmirq(int irq, void *rtc)
{
	u32 val;

	/*judge the irq is beyond to the alarm0*/
	val = sunxi_rtc_read(SUNXI_ALARM_INT_STATUS_REG);
	sunxi_rtc_write(val, SUNXI_ALARM_INT_STATUS_REG);
	if (val & RTC_ENABLE_CNT_IRQ) {
		rtc_update_irq(rtc, 1, RTC_AF | RTC_IRQF);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

/* Update control registers,asynchronous interrupt enable*/
static void sunxi_rtc_setaie(int to)
{
	u32 val;
	switch(to){
	case 1:
		sunxi_rtc_write(0x01, SUNXI_ALARM_EN_REG);
		sunxi_rtc_write(0x01, SUNXI_ALARM_CONFIG);
		break;
	case 0:
	default:
		sunxi_rtc_write(0x00, SUNXI_ALARM_EN_REG);
		/*clear the alarm irq*/
		sunxi_rtc_write(0x00, SUNXI_ALARM_INT_CTRL_REG);
		sunxi_rtc_write(0x00, SUNXI_ALARM_CONFIG);

		val = sunxi_rtc_read(SUNXI_ALARM_INT_STATUS_REG);
		if (val & 1) {
			pr_err("%s,%d, pending reg is val:0x%x", __func__, __LINE__, val);
			/*Clear pending count alarm*/
			sunxi_rtc_write(0x01, SUNXI_ALARM_INT_STATUS_REG);
		}
		break;
	}
}
#endif

/* Time read/write */
static int sunxi_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int date = 0;
	unsigned long time = 0;

	/*only for alarm losc err occur.*/
	if(losc_err_flag) {
		rtc_tm->tm_sec	= 0;
		rtc_tm->tm_min	= 0;
		rtc_tm->tm_hour = 0;

		rtc_tm->tm_mday = 0;
		rtc_tm->tm_mon	= 0;
		rtc_tm->tm_year = 0;
		return -1;
	}

	/*first to get the date, then time, because the sec turn to 0 will effect the date;*/
	do {
		date = sunxi_rtc_read(SUNXI_RTC_DATE_REG);
		time = (unsigned long)sunxi_rtc_read(SUNXI_RTC_TIME_REG);
	} while(date != sunxi_rtc_read(SUNXI_RTC_DATE_REG)
			|| time != (unsigned long)sunxi_rtc_read(SUNXI_RTC_TIME_REG));


	rtc_tm->tm_sec	= TIME_GET_SEC_VALUE(time);
	rtc_tm->tm_min	= TIME_GET_MIN_VALUE(time);
	rtc_tm->tm_hour = TIME_GET_HOUR_VALUE(time);

	rtc_tm->tm_mday = DATE_GET_DAY_VALUE(date);
	rtc_tm->tm_mon	= DATE_GET_MON_VALUE(date);
	rtc_tm->tm_year = DATE_GET_YEAR_VALUE(date);

	rtc_tm->tm_year += 70;
	rtc_tm->tm_mon	-= 1;

	/*
	 * If it first time read and have not save it
	 * we should save it to backup register
	 */
	sunxi_rtc_save(time, false);
	rtc_tm_to_time(rtc_tm, &time);
	sunxi_rtc_fixup(NULL, &time);
	rtc_time_to_tm(time, rtc_tm);

	dev_dbg(dev, "Read hardware RTC time %04d-%02d-%02d %02d:%02d:%02d\n",
			rtc_tm->tm_year + 1900, rtc_tm->tm_mon + 1, rtc_tm->tm_mday,
			rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	return 0;
}

static int sunxi_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	unsigned int date = 0;
	unsigned long time = 0;
	unsigned int crystal_data = 0;
	unsigned int timeout = 0;
	int year = 0;

	/*int tm_year; years from 1900
	 *int tm_mon; months since january 0-11
	 *the input para tm->tm_year is the offset related 1900;
	 */
	year = tm->tm_year + 1900;
	if( rtc_valid_tm(tm) || year > 2033) {
		dev_err(dev, "RTC time is invalid,tm->tm_year:%d\n", tm->tm_year);
		if ((tm->tm_year + 1900) > 2033) {
			printk(KERN_WARNING "The process is \"%s\" (pid %i)\n", current->comm, current->pid);
			tm->tm_year = 132;
		}
	}

	crystal_data = sunxi_rtc_read(SUNXI_LOSC_CTRL_REG);

	/*Any bit of [9:7] is set, The time and date
	 * register can`t be written, we re-try the entried read
	 */
	{
		/*check at most 3 times.*/
		int times = 3;
		while((crystal_data & 0x380) && (times--)){
			dev_dbg(dev, "[RTC]canot change rtc now!\n");
			msleep(500);
			crystal_data = sunxi_rtc_read(SUNXI_LOSC_CTRL_REG);
		}
	}

	/*sunxi ONLY SUPPORTS 63 YEARS,hardware base time:1900.
	 *1970 as 0
	 */
	dev_dbg(dev, "Will set time: %04d-%02d-%02d %02d:%02d:%02d\n",
			tm->tm_year, tm->tm_mon, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* We need to save origin time as seconds */
	rtc_tm_to_time(tm, &time);
	sunxi_rtc_save(time, true);

	tm->tm_year -= 70;
	tm->tm_mon  += 1;

	/*Set Leap Year bit*/
	if(is_leap_year(year))
		date |= LEAP_SET_VALUE(1);

	date |= (DATE_SET_DAY_VALUE(tm->tm_mday)
			| DATE_SET_MON_VALUE(tm->tm_mon)
			| DATE_SET_YEAR_VALUE(tm->tm_year));

	time = (unsigned long)(TIME_SET_SEC_VALUE(tm->tm_sec)
			| TIME_SET_MIN_VALUE(tm->tm_min)
			| TIME_SET_HOUR_VALUE(tm->tm_hour));

	sunxi_rtc_write((unsigned int)time, SUNXI_RTC_TIME_REG);
	timeout = 0xffff;

	while((sunxi_rtc_read(SUNXI_LOSC_CTRL_REG)&(RTC_HHMMSS_ACCESS))&&(--timeout))
		if (timeout == 0) {
			dev_err(dev, "fail to set rtc time.\n");
			return -1;
		}


	sunxi_rtc_write(date, SUNXI_RTC_DATE_REG);
	timeout = 0xffff;
	while((sunxi_rtc_read(SUNXI_LOSC_CTRL_REG)&(RTC_YYMMDD_ACCESS))&&(--timeout))
		if (timeout == 0) {
			dev_err(dev, "fail to set rtc date.\n");
			return -1;
		}

	return 0;
}

#ifdef SUNXI_ALARM
static int sunxi_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	unsigned long alarm_cur = 0, alarm_cnt = 0;
	unsigned long alarm_seconds = 0;
	int ret;

	alarm_cnt = sunxi_rtc_read(SUNXI_RTC_ALARM_COUNTER_REG);
	alarm_cur = sunxi_rtc_read(SUNXI_RTC_ALARM_CURRENT_REG);

	dev_dbg(dev, "alarm_cnt: %lu, alarm_cur: %lu\n", alarm_cnt, alarm_cur);
	if (alarm_cur > alarm_cnt) {
		/* alarm is disabled. */
		alarm->enabled = 0;
		alarm->time.tm_mon = -1;
		alarm->time.tm_mday = -1;
		alarm->time.tm_year = -1;
		alarm->time.tm_hour = -1;
		alarm->time.tm_min = -1;
		alarm->time.tm_sec = -1;
		return 0;
	}

	ret = sunxi_rtc_gettime(dev, &alarm->time);
	if (ret)
		return -EINVAL;

	rtc_tm_to_time(&alarm->time, &alarm_seconds);
	alarm_cnt = (alarm_cnt - alarm_cur);
	alarm_cur = 0;
	sunxi_rtc_fixup(&alarm_cur, &alarm_cnt);
	alarm_seconds += alarm_cnt;

	rtc_time_to_tm(alarm_seconds, &alarm->time);
	dev_dbg(dev, "alarm_seconds: %lu\n", alarm_seconds);

	if(RTC_ALARM_COUNT_INT_EN
		& sunxi_rtc_read(SUNXI_ALARM_INT_CTRL_REG))
		alarm->enabled = 1;

	return 0;
}

static int sunxi_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alm)
{
	struct rtc_time *tm = &alm->time;
	int ret = 0;
	struct rtc_time tm_now;
	unsigned long time_now = 0;
	unsigned long time_set = 0;

	/*
	 * Sorry, sunxi hardware max time is 2033 year.
	 */
	if( rtc_valid_tm(tm) || (tm->tm_year + 1900) > 2033) {
		dev_err(dev, "Alarm time is invalid, tm->tm_year:%d\n", tm->tm_year);
		if ((tm->tm_year + 1900) > 2033) {
			printk(KERN_WARNING "The process is \"%s\" (pid %i)\n", current->comm, current->pid);
			tm->tm_year = 132;
		}
	}

	ret = sunxi_rtc_gettime(dev, &tm_now);
	if (ret)
		return -EINVAL;

	rtc_tm_to_time(tm, &time_set);
	rtc_tm_to_time(&tm_now, &time_now);

	dev_dbg(dev, "Now RTC time %04d-%02d-%02d %02d:%02d:%02d\n",
			tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
			tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
	dev_dbg(dev, "Set alarm time %04d-%02d-%02d %02d:%02d:%02d\n",
			tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);

	if(time_set <= time_now){
		dev_err(dev, "Ignore alarm set, it has passed\n");
		return -EINVAL;
	}

	/*
	 * Here, time_set & time_now are normal seconds,
	 * but we need to calculate fake time_now to get missing seconds.
	 */
	sunxi_rtc_fixup(&time_set, &time_now);
	/* Get gap time, max is 0xFFFFFFFF */
	time_set = (time_set - time_now) & 0xFFFFFFFF;

	/* Clear the alm counter enable bit */
	sunxi_rtc_setaie(0);

	/* Clear the alm count & irq value */
	sunxi_rtc_write(0x00, SUNXI_RTC_ALARM_COUNTER_REG);
	sunxi_rtc_write(0x00, SUNXI_ALARM_INT_CTRL_REG);

	/* Rewrite the alm count & irq value */
	sunxi_rtc_write(time_set, SUNXI_RTC_ALARM_COUNTER_REG);
	sunxi_rtc_write(RTC_ENABLE_CNT_IRQ, SUNXI_ALARM_INT_CTRL_REG);

	/*decided whether we should start the counter to down count*/
	sunxi_rtc_setaie(alm->enabled);

	return 0;
}

static int sunxi_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	dev_dbg(dev, "%s: will %s RTC IRQ\n", __func__,
			enabled ? "enable" : "disabled");
	if (!enabled)
		sunxi_rtc_setaie(enabled);

	return 0;
}
#endif

static const struct rtc_class_ops sunxi_rtcops = {
	.read_time	= sunxi_rtc_gettime,
	.set_time	= sunxi_rtc_settime,
#ifdef SUNXI_ALARM
	.read_alarm	= sunxi_rtc_getalarm,
	.set_alarm	= sunxi_rtc_setalarm,
	.alarm_irq_enable = sunxi_rtc_alarm_irq_enable,
#endif
};

static int __exit sunxi_rtc_remove(struct platform_device *pdev)
{
	struct rtc_device *rtc = platform_get_drvdata(pdev);

#ifdef SUNXI_ALARM
	free_irq(sunxi_rtc_alarmno, rtc);
#endif

	rtc_device_unregister(rtc);
	platform_set_drvdata(pdev, NULL);

#ifdef SUNXI_ALARM
	sunxi_rtc_setaie(0);
#endif

	return 0;
}

static int __init sunxi_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	int ret;
	unsigned int tmp_data;

	sunxi_rtc_base = (void __iomem *)(SUNXI_VA_TIMERC_IO_BASE);

	sunxi_rtc_alarmno = platform_get_irq(pdev, 0);
	if (sunxi_rtc_alarmno <= 0) {
		pr_debug("%s: no update irq?\n", pdev->name);
		return -ENOENT;
	}

	/*
	 * Step1: select RTC clock source
	 */
	tmp_data = sunxi_rtc_read(SUNXI_LOSC_CTRL_REG);
	tmp_data &= (~REG_CLK32K_AUTO_SWT_EN);
	tmp_data |= (RTC_SOURCE_EXTERNAL | REG_LOSCCTRL_MAGIC);
	tmp_data |= (EXT_LOSC_GSM);
#ifdef CONFIG_ARCH_SUN8IW8
	/* Fixup auto switch BUG, when powerdown */
	tmp_data |= (1 <<15);
#endif
	sunxi_rtc_write(tmp_data, SUNXI_LOSC_CTRL_REG);

	dev_dbg(&(pdev->dev),"sunxi_rtc_probe tmp_data = %d\n", tmp_data);

	/*
	 * Step2: check set result
	 */
	tmp_data = sunxi_rtc_read(SUNXI_LOSC_CTRL_REG);
	if(!(tmp_data & RTC_SOURCE_EXTERNAL)){
		printk(KERN_ERR "[RTC] WARNING: Rtc time will be wrong!!\n");
		printk(KERN_ERR "[RTC] WARNING: use *internal OSC* as clock source\n");
		tmp_data = sunxi_rtc_read(SUNXI_LOSC_CTRL_REG);
		tmp_data &= (~RTC_SOURCE_EXTERNAL);
		tmp_data |= (REG_CLK32K_AUTO_SWT_EN | REG_LOSCCTRL_MAGIC);
		sunxi_rtc_write(tmp_data, SUNXI_LOSC_CTRL_REG);
		sunxi_rtc_write(INTOSC_PERSCAL, SUNXI_INTOSC_CLK_PRESCAL_REG);
	}

	device_init_wakeup(&pdev->dev, 1);
	/*
	 * Register RTC and exit
	 */
	rtc = rtc_device_register(pdev->name, &pdev->dev, &sunxi_rtcops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		ret = PTR_ERR(rtc);
		goto err_out;
	}

#ifdef SUNXI_ALARM
	ret = request_irq(sunxi_rtc_alarmno, sunxi_rtc_alarmirq,
				IRQF_DISABLED,	"sunxi-rtc alarm", rtc);
	if (ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", sunxi_rtc_alarmno, ret);
		rtc_device_unregister(rtc);
		return ret;
	}
#endif

	platform_set_drvdata(pdev, rtc);

	/* Clean the alarm count value */
	sunxi_rtc_write(0x00, SUNXI_RTC_ALARM_COUNTER_REG);
	/* Clean the alarm current value */
	sunxi_rtc_write(0x00, SUNXI_RTC_ALARM_CURRENT_REG);
	/* Disable the alarm0 when init */
	sunxi_rtc_write(0x00, SUNXI_ALARM_EN_REG);
	/* Disable the alarm0 irq */
	sunxi_rtc_write(0x00, SUNXI_ALARM_INT_CTRL_REG);
	/* Clear pending count alarm */
	sunxi_rtc_write(0x01, SUNXI_ALARM_INT_STATUS_REG);
	sunxi_rtc_write(0x00, SUNXI_ALARM_CONFIG);

	return 0;

err_out:
	return ret;
}

/*share the irq no. with timer2*/
static struct resource sunxi_rtc_res[] = {
	[0] = {
		.start	= SUNXI_IRQ_RALARM0,
		.end	= SUNXI_IRQ_RALARM0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device sunxi_device_rtc = {
	.name		= "sunxi-rtc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(sunxi_rtc_res),
	.resource	= sunxi_rtc_res,
};

static struct platform_driver sunxi_rtc_driver = {
	.probe		= sunxi_rtc_probe,
	.remove		= __exit_p(sunxi_rtc_remove),
	.driver		= {
		.name	= "sunxi-rtc",
		.owner	= THIS_MODULE,
	},
};

static int __init sunxi_rtc_init(void)
{
	platform_device_register(&sunxi_device_rtc);
	return platform_driver_register(&sunxi_rtc_driver);
}

static void __exit sunxi_rtc_exit(void)
{
	platform_driver_unregister(&sunxi_rtc_driver);
}

module_init(sunxi_rtc_init);
module_exit(sunxi_rtc_exit);

MODULE_DESCRIPTION("Sunxi RTC driver");
MODULE_AUTHOR("Huangxin <huangxin@allwinnertech.com>");
MODULE_AUTHOR("Sguar <shuge@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-rtc");
MODULE_VERSION("1.0");
#endif


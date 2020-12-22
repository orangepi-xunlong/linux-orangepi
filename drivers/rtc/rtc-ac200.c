/*
 * drivers/rtc/rtc-ac200.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: huangxin <huangxin@allwinnertech.com>
 *
 * rtc driver for sunxi external rtc chip
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/bcd.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/arisc/arisc.h>
#include <linux/mfd/acx00-mfd.h>
#include <mach/sys_config.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>

#define DRV_VERSION "0.4.3"
static int ac200_used 	= 0;
static int ac100_used 	= 0;

/* reg off defination */
#define LOSC_CTRL_REG0				0xa000
#define LOSC_CTRL_REG1				0xa002
#define LOSC_AUTO_SWT_STA_REG		0xa004
#define INTOSC_CLK_PRESCAL_REG		0xa008
#define RTC_YY_MM_DD_REG0			0xa010
#define RTC_YY_MM_DD_REG1			0xa012
#define RTC_HH_MM_SS_REG0			0xa014
#define RTC_HH_MM_SS_REG1			0xa016
#define ALARM0_COUNTER_REG0			0xa020
#define ALARM0_COUNTER_REG1			0xa022
#define ALARM0_CUR_VLU_REG0			0xa024
#define ALARM0_CUR_VLU_REG1			0xa026
#define ALARM0_ENABLE_REG			0xa028
#define ALARM0_IRQ_EN				0xa02c
#define ALARM0_IRQ_STA_REG			0xa030
#define ALARM_CONFIG_REG			0xa050
#define LOSC_OUT_GATING_REG			0xa060
#define GP_DATA_REG(x)				(0xa100 + ((x) << 1)) /* x: 0 ~ 15 */
#define RTC_DEB_REG					0xa170
#define GPL_HOLD_OUTPUT_REG			0xa180
#define VDD_RTC_REG					0xa190
#define IC_CHARA_REG0				0xa1f0
#define IC_CHARA_REG1				0xa1f2

/*LOSC_CTRL_REG0:0xa000*/
#define LOSC_AUTO_SWT_EN		14
#define EXT_LOSC_GSM			2
#define LOSC_SRC_SEL			0

/*LOSC_CTRL_REG1:0xa002*/
#define KEY_FIELD				0

/*LOSC_AUTO_SWT_STA_REG:0xa004*/
#define LOSC_AUTO_SWT_PEND		1

/*RTC_YY_MM_DD_REG0:0xa010*/
#define MONTH					8
#define DAY						0

/*RTC_YY_MM_DD_REG1:0xa012*/
#define LEAP					6
#define YEAR					0

/*RTC_HH_MM_SS_REG0:0xa014*/
#define MINUTE					8
#define SECOND					0

/*RTC_HH_MM_SS_REG1:0xa016*/
#define WK_NO					13
#define HOUR					0

/*ALARM0_COUNTER_REG0:0xa020*/
#define ALARM0_COUNTER_LOW		0

/*ALARM0_COUNTER_REG1:0xa022*/
#define ALARM0_COUNTER_HIGH		0

/*ALARM0_CUR_VLU_REG0:0xa024*/
#define ALARM0_CUR_ULU_LOW		0

/*ALARM0_CUR_VLU_REG1:0xa026*/
#define ALARM0_CUR_VLU_HIGH		0

/*ALARM0_ENABLE_REG:0xa028*/
#define ALM_0_EN				0

/*ALARM0_IRQ_EN:0xa02c*/
//#define ALARM0_IRQ_EN			0

/*ALARM0_IRQ_STA_REG:0xa030*/
#define ALARM0_IRQ_PEND			0

/*ALARM_CONFIG_REG:0xa050*/
#define ALARM_WAKEUP			0

/*LOSC_OUT_GATING_REG:0xa060*/
#define LOSC_OUT_GATING			0

#define RTC_NAME		"rtc0_ac200"

#define ENABLE_ALARM

#ifdef ENABLE_ALARM
#define ALARM_IRQ_GPIO		GPIOE(0)	/* may need change */
#define ALARM_IRQ_NAME		"PE0_EINT"
static int rtc_alarmno	= NO_IRQ;
#endif

#define MAX_YEAR_REG_VAL	63	/* year reg max value, it's 63 from ac200 spec */
#define YEAR_SUPPORT_MIN	1970	/* the mimimum year support to set, 1970 is from rtc_valid_tm(rtc-lib.c), donot change */
#define YEAR_SUPPORT_MAX	(YEAR_SUPPORT_MIN + MAX_YEAR_REG_VAL) /* the max year we support to set */

#define IS_LEAP_YEAR(year)	(((year)%400)==0 || (((year)%100)!=0 && ((year)%4)==0))

#ifdef CONFIG_ARCH_SUN8IW6P1
static script_item_u item_eint;
#endif

struct sunxi_rtc {
	int virq; /*alarm irq*/
	struct rtc_device *rtc;
#ifdef ENABLE_ALARM
	struct work_struct work;
#endif
	struct mutex mutex;
	struct acx00 *acx00;
};

/* reg_val: raw value to be wrote to the reg, bcd convertion is done outside */
static inline void rtc_write_reg(struct acx00 *acx00, int reg, int value)
{
	acx00_reg_write(acx00, reg, value);
}

/* return raw value from the reg, then it may do bcd convertion if needed */
static inline int rtc_read_reg(struct acx00 *acx00, int reg)
{
	return acx00_reg_read(acx00, reg);
}

static int rtc_hms_read_start(struct acx00 *acx00)
{
	int reg_val;

	reg_val = rtc_read_reg(acx00, LOSC_CTRL_REG0)|(0x1<<13);
	rtc_write_reg(acx00, LOSC_CTRL_REG0, reg_val);

	return 0;
}

static int rtc_hms_read_end(struct acx00 *acx00)
{
	int reg_val;

	reg_val = rtc_read_reg(acx00, LOSC_CTRL_REG0)&~(0x1<<13);
	rtc_write_reg(acx00, LOSC_CTRL_REG0, reg_val);

	return 0;
}

static int rtc_hms_write_end(struct acx00 *acx00)
{
	int reg_val;

	reg_val = rtc_read_reg(acx00, LOSC_CTRL_REG0)|(0x1<<12);
	rtc_write_reg(acx00, LOSC_CTRL_REG0, reg_val);

	return 0;
}

static int rtc_ymd_read_start(struct acx00 *acx00)
{
	int reg_val;

	reg_val = rtc_read_reg(acx00, LOSC_CTRL_REG0)|(0x1<<5);
	rtc_write_reg(acx00, LOSC_CTRL_REG0, reg_val);

	return 0;
}

static int rtc_ymd_read_end(struct acx00 *acx00)
{
	int reg_val;

	reg_val = rtc_read_reg(acx00, LOSC_CTRL_REG0)&~(0x1<<5);
	rtc_write_reg(acx00, LOSC_CTRL_REG0, reg_val);

	return 0;
}

static int rtc_ymd_write_end(struct acx00 *acx00)
{
	int reg_val;

	reg_val = rtc_read_reg(acx00, LOSC_CTRL_REG0)|(0x1<<4);
	rtc_write_reg(acx00, LOSC_CTRL_REG0, reg_val);

	return 0;
}

/*write y m d*/
static int write_rtc_ymd_reg(struct acx00 *acx00, int reg, int value)
{
	int reg_val = 0;

	rtc_write_reg(acx00, reg, value);
	rtc_ymd_write_end(acx00);

	return reg_val;
}

/*read y m d*/
static int read_rtc_ymd_reg(struct acx00 *acx00, int reg)
{
	int reg_val;

	rtc_ymd_read_start(acx00);
	reg_val = rtc_read_reg(acx00, reg);
	rtc_ymd_read_end(acx00);

	return reg_val;
}

/*write h m s*/
static int write_rtc_hms_reg(struct acx00 *acx00, int reg, int value)
{
	int reg_val = 0;

	rtc_write_reg(acx00, reg, value);
	rtc_hms_write_end(acx00);

	return reg_val;
}

/*read h m s*/
static int read_rtc_hms_reg(struct acx00 *acx00, int reg)
{
	int reg_val;

	rtc_hms_read_start(acx00);
	reg_val = rtc_read_reg(acx00, reg);
	rtc_hms_read_end(acx00);

	return reg_val;
}

/**
 * get the actual year from year reg
 * @actual_year: year to set, eg: 2014.
 *
 * return the value to be set to year reg.
 */
static int actual_year_to_reg_year(int actual_year)
{
	if (actual_year < YEAR_SUPPORT_MIN || actual_year > YEAR_SUPPORT_MAX) {
		pr_err("%s(%d) err: we only support (%d ~ %d), but input %d\n", __func__, __LINE__,
			YEAR_SUPPORT_MIN, YEAR_SUPPORT_MAX, actual_year);
			actual_year = YEAR_SUPPORT_MAX -1;
	}
	return actual_year - YEAR_SUPPORT_MIN;
}

/**
 * get the year reg value from actual year
 * @reg_val: value from year reg.
 *
 * return the actual year, eg 2014.
 */
static int reg_year_to_actual_year(int reg_val)
{
	if (reg_val > MAX_YEAR_REG_VAL)
		reg_val = MAX_YEAR_REG_VAL;
	return reg_val + YEAR_SUPPORT_MIN;
}

static bool time_valid(struct rtc_time *tm)
{
	int actual_year, actual_month, leap_year = 0;
	int err = 0;

	actual_year = tm->tm_year + 1900;	/* tm_year is from 1900 in linux */
	actual_month = tm->tm_mon + 1;		/* month is 1..12 in RTC reg but 0..11 in linux */
	leap_year = IS_LEAP_YEAR(actual_year);

	if (rtc_valid_tm(tm) < 0) {
		pr_err("%s(%d): time(%d-%d-%d %d:%d:%d) invalid.\n", __func__, __LINE__, actual_year,
			actual_month, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		return false;
	}

	/* int tm_year; years from 1900
	 * int tm_mon; months since january 0-11
	 * the input para tm->tm_year is the offset related 1900 */
	if(actual_year > YEAR_SUPPORT_MAX || actual_year < YEAR_SUPPORT_MIN) {
		pr_err("%s(%d) err: rtc only supports (%d~%d) years, but to set is %d\n", __func__, __LINE__,
			YEAR_SUPPORT_MIN, YEAR_SUPPORT_MAX, actual_year);
			actual_year = YEAR_SUPPORT_MAX -1;
	}

	switch(actual_month) {
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		if(tm->tm_mday > 31)
			err = __LINE__;
		if(tm->tm_hour > 24 || tm->tm_min > 59 || tm->tm_sec > 59)
			err = __LINE__;
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		if(tm->tm_mday > 30)
			err = __LINE__;
		if(tm->tm_hour > 24 || tm->tm_min > 59 || tm->tm_sec > 59)
			err = __LINE__;
		break;
	case 2:
		if(leap_year) {
			if(tm->tm_mday > 29)
				err = __LINE__;
			if(tm->tm_hour > 24 || tm->tm_min > 59 || tm->tm_sec > 59)
				err = __LINE__;
		} else {
			if(tm->tm_mday > 28)
				err = __LINE__;
			if(tm->tm_hour > 24 || tm->tm_min > 59 || tm->tm_sec > 59)
				err = __LINE__;
		}
		break;
	default:
		err = __LINE__;
		break;
	}

	if (err) {
		pr_err("%s err, line %d!\n", __func__, err);
		return false;
	}
	return true;
}

static int rtc_settime(struct sunxi_rtc *rtc_dev, struct rtc_time *tm)
{
	int actual_year, actual_month, leap_year = 0, reg_val;
	unsigned int timeout = 0;
	actual_year = tm->tm_year + 1900;	/* tm_year is from 1900 in linux */
	actual_month = tm->tm_mon + 1;		/* month is 1..12 in RTC reg but 0..11 in linux */
	leap_year = IS_LEAP_YEAR(actual_year);

	pr_debug("%s(%d): time to set %d-%d-%d %d:%d:%d\n", __func__, __LINE__, actual_year,
		actual_month, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* prevent the application seting the error time */
	if (!time_valid(tm)) {
#if 1
		pr_err("%s(%d) err: date %d-%d-%d %d:%d:%d invalid!\n", __func__, __LINE__,
			actual_year, actual_month, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		return -EINVAL;
#else
		pr_info("%s(%d) err: date %d-%d-%d %d:%d:%d invalid, so reset to 1970-1-1 00:00:00\n", __func__, __LINE__,
			actual_year, actual_month, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		tm->tm_sec  = 0;
		tm->tm_min  = 0;
		tm->tm_hour = 0;
		tm->tm_mday = 0;
		tm->tm_mon  = 0;  /* 0 is month 1(January) */
		tm->tm_year = 70; /* tm_year=0 is 1900, so tm_year=70 is 1970 */

		actual_year = tm->tm_year + 1900;	/* refresh actual_year for actual_year_to_reg_year below */
		actual_month = tm->tm_mon + 1;		/* refresh actual_month for below */
		leap_year = IS_LEAP_YEAR(actual_year);	/* refresh leap_year flag for below */
#endif
	}
	/* set year */
	reg_val = actual_year_to_reg_year(actual_year);
	if (leap_year)
		reg_val |= (0x1<<LEAP);
	write_rtc_ymd_reg(rtc_dev->acx00, RTC_YY_MM_DD_REG1, reg_val);
	write_rtc_ymd_reg(rtc_dev->acx00, RTC_YY_MM_DD_REG0, ((actual_month<<MONTH)|tm->tm_mday));
	/* set time */
	write_rtc_hms_reg(rtc_dev->acx00, RTC_HH_MM_SS_REG1,((tm->tm_wday<<WK_NO)|tm->tm_hour));
	write_rtc_hms_reg(rtc_dev->acx00, RTC_HH_MM_SS_REG0,((tm->tm_min<<MINUTE)|tm->tm_sec));

	msleep(30); /* delay 30ms to wait write stable */
	timeout = 0xffff;
	while((rtc_read_reg(rtc_dev->acx00, LOSC_CTRL_REG0)&((0x3<<9)))&&(--timeout))
	if (timeout == 0) {
		pr_err("fail to set rtc time:%s,l:%d.\n", __func__, __LINE__);
		return -1;
	}
	timeout = 0xffff;
	while((rtc_read_reg(rtc_dev->acx00, LOSC_CTRL_REG0)&(0x3<<7))&&(--timeout))
	if (timeout == 0) {
		pr_err("fail to set rtc time:%s,l:%d.\n", __func__, __LINE__);
		return -1;
	}
	pr_info("%s(%d): set time %d-%d-%d %d:%d:%d success!\n", __func__, __LINE__, actual_year,
		actual_month, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	return 0;
}

static int rtc_gettime(struct sunxi_rtc *rtc_dev, struct rtc_time *tm)
{
	struct rtc_time time_tmp;
	int reg_val;

	/* get sec */
	reg_val = read_rtc_hms_reg(rtc_dev->acx00, RTC_HH_MM_SS_REG0);
	reg_val &= 0x3f;
	time_tmp.tm_sec = reg_val;

	/* get min */
	reg_val = read_rtc_hms_reg(rtc_dev->acx00, RTC_HH_MM_SS_REG0);
	reg_val = reg_val>>MINUTE;
	time_tmp.tm_min = reg_val;

	/* get hour */
	reg_val = read_rtc_hms_reg(rtc_dev->acx00, RTC_HH_MM_SS_REG1);
	reg_val &= 0x1f;
	time_tmp.tm_hour = reg_val;

	/* get week day */
	reg_val = read_rtc_hms_reg(rtc_dev->acx00, RTC_HH_MM_SS_REG1);
	reg_val = reg_val>>WK_NO;
	time_tmp.tm_wday = reg_val;

	/* get day */
	reg_val = read_rtc_ymd_reg(rtc_dev->acx00, RTC_YY_MM_DD_REG0);
	reg_val&=0x1f;
	time_tmp.tm_mday = reg_val;

	/* get month */
	reg_val = read_rtc_ymd_reg(rtc_dev->acx00, RTC_YY_MM_DD_REG0);
	reg_val = reg_val>>MONTH;
	time_tmp.tm_mon = reg_val;
	time_tmp.tm_mon -= 1; /* month is 1..12 in RTC reg but 0..11 in linux */

	/* get year */
	reg_val = read_rtc_ymd_reg(rtc_dev->acx00, RTC_YY_MM_DD_REG1);
	reg_val&=0x3f;
	time_tmp.tm_year = reg_val;
	time_tmp.tm_year = reg_year_to_actual_year(time_tmp.tm_year) - 1900; /* in linux, tm_year=0 is 1900 */

	pr_info("%s(%d): read time %d-%d-%d %d:%d:%d\n", __func__, __LINE__, time_tmp.tm_year + 1900,
		time_tmp.tm_mon + 1, time_tmp.tm_mday, time_tmp.tm_hour, time_tmp.tm_min, time_tmp.tm_sec);

	/* check if time read from rtc reg is OK */
	if (!time_valid(&time_tmp)) {
		pr_err("%s(%d) err: retrieved date/time is not valid.\n", __func__, __LINE__);
		return -EIO;
	}

	*tm = time_tmp;
	return 0;
}

static int sunxi_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtc *rtc_dev = platform_get_drvdata(pdev);
	int ret = 0;

	ret = rtc_gettime(rtc_dev, tm);

	return ret;
}

static int sunxi_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtc *rtc_dev = platform_get_drvdata(pdev);
	int ret = 0;

	mutex_lock(&rtc_dev->mutex);
	ret = rtc_settime(rtc_dev, tm);
	mutex_unlock(&rtc_dev->mutex);
	return ret;
}

#ifdef ENABLE_ALARM
void alarm_enable(struct sunxi_rtc *rtc_dev)
{
	int reg_val = 0;

	/* enable alarm irq */
	reg_val = rtc_read_reg(rtc_dev->acx00, SYS_IRQ_ENABLE);
	reg_val |= (0x1<<RTC_IRQ_ENABLE)|(0x1<<INTB_OUTPUT_ENABLE);
	rtc_write_reg(rtc_dev->acx00, SYS_IRQ_ENABLE, reg_val);

	rtc_write_reg(rtc_dev->acx00, ALARM0_ENABLE_REG, 0x1);
	rtc_write_reg(rtc_dev->acx00, ALARM0_IRQ_EN, 0x1);
}

static void alarm_disable(struct sunxi_rtc *rtc_dev)
{
	int reg_val = 0;

	/* disable alarm irq */
	reg_val = rtc_read_reg(rtc_dev->acx00, SYS_IRQ_ENABLE);
	reg_val &= ~(0x1<<RTC_IRQ_ENABLE);
	reg_val &= ~(0x1<<INTB_OUTPUT_ENABLE);
	rtc_write_reg(rtc_dev->acx00, SYS_IRQ_ENABLE, reg_val);

	rtc_write_reg(rtc_dev->acx00, ALARM0_ENABLE_REG, 0x0);
	rtc_write_reg(rtc_dev->acx00, ALARM0_IRQ_EN, 0x0);

	/* clear alarm irq pending */
	reg_val = rtc_read_reg(rtc_dev->acx00, SYS_IRQ_STATUS);
	if (reg_val & (0x1<<RTC_IRQ_STATUS)) {
		pr_err("%s(%d) maybe err: alarm irq pending is set, clear it! reg is 0x%x\n", __func__, __LINE__, reg_val);
		reg_val &= ~(0x1<<RTC_IRQ_STATUS);
		rtc_write_reg(rtc_dev->acx00, SYS_IRQ_STATUS, reg_val);
	}
	reg_val = rtc_read_reg(rtc_dev->acx00, ALARM0_IRQ_STA_REG);
	if (reg_val & 0x1) {
		pr_err("%s(%d) maybe err: alarm irq pending is set, clear it! reg is 0x%x\n", __func__, __LINE__, reg_val);
		rtc_write_reg(rtc_dev->acx00, ALARM0_IRQ_STA_REG, 0x0);
	}

	rtc_write_reg(rtc_dev->acx00, ALARM0_COUNTER_REG0, 0x0);
	rtc_write_reg(rtc_dev->acx00, ALARM0_COUNTER_REG1, 0x0);
}

static int sunxi_alarm_enable(struct sunxi_rtc *rtc_dev)
{
	mutex_lock(&rtc_dev->mutex);

	alarm_enable(rtc_dev);

	mutex_unlock(&rtc_dev->mutex);
	return 0;
}

static int sunxi_alarm_disable(struct sunxi_rtc *rtc_dev)
{
	mutex_lock(&rtc_dev->mutex);

	alarm_disable(rtc_dev);

	mutex_unlock(&rtc_dev->mutex);
	return 0;
}

#ifdef CONFIG_ARCH_SUN8IW6P1
static irqreturn_t sunxi_irq_handle(int irq, void *dev_id)
{
	struct sunxi_rtc *rtc_dev = dev_id;

	(void)schedule_work(&rtc_dev->work);
	return 0;
}
#endif

static void alarm_irq_work(struct work_struct *work)
{
	struct sunxi_rtc *rtc_dev = container_of(work, struct sunxi_rtc, work);
	int reg_val = 0;

	mutex_lock(&rtc_dev->mutex);

	/* clear alarm irq pending */
	reg_val = rtc_read_reg(rtc_dev->acx00, ALARM0_IRQ_STA_REG);
	if (reg_val < 0) {
		mutex_unlock(&rtc_dev->mutex);
		pr_err("%s(%d) err: read alarm int status reg failed!\n", __func__, __LINE__);
		return;
	}
	if (reg_val & 1) {
		rtc_write_reg(rtc_dev->acx00, ALARM0_IRQ_STA_REG, 1);
		mutex_unlock(&rtc_dev->mutex);

		reg_val = rtc_read_reg(rtc_dev->acx00, SYS_IRQ_STATUS);
		if (reg_val & (0x1<<RTC_IRQ_STATUS)) {
			reg_val &= ~(0x1<<RTC_IRQ_STATUS);
			rtc_write_reg(rtc_dev->acx00, SYS_IRQ_STATUS, reg_val);
		}

		rtc_update_irq(rtc_dev->rtc, 1, RTC_AF | RTC_IRQF);
		return;
	}
	mutex_unlock(&rtc_dev->mutex);
}

static int sunxi_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtc *rtc_dev = platform_get_drvdata(pdev);
	unsigned long alarm_cur = 0, alarm_cnt = 0;
	unsigned long alarm_seconds = 0;
	int reg_val = 0;
	int ret = 0;

	mutex_lock(&rtc_dev->mutex);
	alarm_cur = rtc_read_reg(rtc_dev->acx00, ALARM0_CUR_VLU_HIGH);
	alarm_cur = (alarm_cur<<15)|(rtc_read_reg(rtc_dev->acx00, ALARM0_CUR_ULU_LOW));

	alarm_cnt = rtc_read_reg(rtc_dev->acx00, ALARM0_COUNTER_HIGH);
	alarm_cnt = (alarm_cnt<<15)|rtc_read_reg(rtc_dev->acx00, ALARM0_COUNTER_HIGH);

	dev_err(dev, "alarm_cnt: %lu, alarm_cur: %lu\n", alarm_cnt, alarm_cur);
	if (alarm_cur > alarm_cnt) {
		/* alarm is disabled. */
		alarm->enabled = 0;
		alarm->time.tm_mon = -1;
		alarm->time.tm_mday = -1;
		alarm->time.tm_year = -1;
		alarm->time.tm_hour = -1;
		alarm->time.tm_min = -1;
		alarm->time.tm_sec = -1;
		goto end;
	}

	ret = sunxi_rtc_read_time(dev, &alarm->time);
	if (ret) {
		ret = -EINVAL;
		goto end;
	}
	rtc_tm_to_time(&alarm->time, &alarm_seconds);
	alarm_cnt = (alarm_cnt - alarm_cur);
	alarm_cur = 0;
	alarm_seconds += alarm_cnt;

	rtc_time_to_tm(alarm_seconds, &alarm->time);
	dev_dbg(dev, "alarm_seconds: %lu\n", alarm_seconds);

	reg_val = rtc_read_reg(rtc_dev->acx00, ALARM0_ENABLE_REG);
	if (reg_val & 0x1)
		alarm->enabled = 1;
end:
	mutex_unlock(&rtc_dev->mutex);
	return ret;
}

static int sunxi_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtc *rtc_dev = platform_get_drvdata(pdev);
	struct rtc_time *tm_set = &alrm->time, tm_now;
	unsigned long time_set = 0, time_now = 0;
	int ret = 0;

	ret = sunxi_rtc_read_time(dev, &tm_now);

	pr_info("%s(%d): time to set (%d-%d-%d %d:%d:%d), get cur time (%d-%d-%d %d:%d:%d)\n", __func__, __LINE__,
		tm_set->tm_year + 1900,	tm_set->tm_mon + 1, tm_set->tm_mday, tm_set->tm_hour, tm_set->tm_min, tm_set->tm_sec,
		tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);

	/* prevent the application seting the error time */
	if (!time_valid(tm_set)) {
		pr_err("%s(%d) err: time to set is not valid!\n", __func__, __LINE__);
		return -EINVAL;
	}

	/* if alarm date is before now, omit it */
	rtc_tm_to_time(tm_set, &time_set);
	rtc_tm_to_time(&tm_now, &time_now);
	if (!ret && time_set <= time_now) {
		pr_err("%s(%d) err: the day has been passed!\n", __func__, __LINE__);
		return -EINVAL;
	}

	mutex_lock(&rtc_dev->mutex);
	/* disable alarm */
	alarm_disable(rtc_dev);
	/* Get gap time, max is 0xFFFFFFFF */
	time_set = (time_set - time_now) & 0xFFFFFFFF;

	/* Rewrite the alm count */
	rtc_write_reg(rtc_dev->acx00, ALARM0_COUNTER_REG0, (time_set&0xFFFF));
	rtc_write_reg(rtc_dev->acx00, ALARM0_COUNTER_REG1, (time_set>>15)&0xFFFF);

	pr_debug("%s(%d): alarm->enabled=%d\n", __func__, __LINE__, alrm->enabled);
	/* enable alarm if flag set */
	if (alrm->enabled) {
		alarm_enable(rtc_dev);
	}

	mutex_unlock(&rtc_dev->mutex);
	return 0;
}

static int sunxi_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtc *rtc_dev = platform_get_drvdata(pdev);
	int ret = 0;

	pr_debug("%s(%d): enabled=%d\n", __func__, __LINE__, enabled);
	if (!enabled) {
		ret = sunxi_alarm_disable(rtc_dev);
		if (ret < 0)
			pr_err("%s(%d) err, disable alarm failed!\n", __func__, __LINE__);
	} else {
		ret = sunxi_alarm_enable(rtc_dev);
	}

	return ret;
}
#endif

static const struct rtc_class_ops sunxi_rtc_ops = {
	.read_time      = sunxi_rtc_read_time,
	.set_time       = sunxi_rtc_set_time,
#ifdef ENABLE_ALARM
	.read_alarm     = sunxi_rtc_getalarm,
	.set_alarm      = sunxi_rtc_setalarm,
	.alarm_irq_enable = sunxi_rtc_alarm_irq_enable,
#endif
};

static int __init sunxi_rtc_probe(struct platform_device *pdev)
{
	struct sunxi_rtc *rtc_dev;
	int err = 0;
#ifdef CONFIG_ARCH_SUN8IW6P1
	int ret = 0;
	int reg_val = 0;
#ifdef ENABLE_ALARM
	int req_status = 0;
	script_item_value_type_e  type;
#endif
#endif

	rtc_dev = kzalloc(sizeof(struct sunxi_rtc), GFP_KERNEL);
	if (!rtc_dev)
		return -ENOMEM;

	/* must before rtc_device_register, because it will call rtc_device_register -> __rtc_read_alarm ->
	 *  rtc_read_time-> __rtc_read_time -> sunxi_rtc_read_time, witch may use platform_get_drvdata. */
	platform_set_drvdata(pdev, rtc_dev);
	rtc_dev->acx00 = dev_get_drvdata(pdev->dev.parent);

#ifdef ENABLE_ALARM
	INIT_WORK(&rtc_dev->work, alarm_irq_work);
	device_init_wakeup(&pdev->dev, true); /* enable alarm wakeup */
#endif
	mutex_init(&rtc_dev->mutex);

#ifdef CONFIG_ARCH_SUN8IW6P1
	/*
	 * Step1: select RTC clock source
	 */
	reg_val = rtc_read_reg(rtc_dev->acx00, LOSC_CTRL_REG1);
	reg_val |= (0x16ab<<KEY_FIELD);
	rtc_write_reg(rtc_dev->acx00, LOSC_CTRL_REG1, reg_val);

	reg_val = rtc_read_reg(rtc_dev->acx00, LOSC_CTRL_REG0);
	reg_val |= (0x1<<LOSC_AUTO_SWT_EN);
	reg_val |= ((0x1<<LOSC_SRC_SEL) | (0x2<<EXT_LOSC_GSM));
	reg_val |= (EXT_LOSC_GSM);
	rtc_write_reg(rtc_dev->acx00, LOSC_CTRL_REG0, reg_val);

	/*
	 * Step2: check set result
	 */
	reg_val = rtc_read_reg(rtc_dev->acx00, LOSC_CTRL_REG0);
	if(!(reg_val & (0x1 << LOSC_SRC_SEL))){
		printk(KERN_ERR "[RTC] WARNING: Rtc time will be wrong!!LOSC_CTRL_REG0:%x:\n", rtc_read_reg(rtc_dev->acx00, LOSC_CTRL_REG0));
	}
#endif

	rtc_dev->rtc = rtc_device_register(RTC_NAME, &pdev->dev, &sunxi_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc_dev->rtc)) {
		err = PTR_ERR(rtc_dev->rtc);
		kfree(rtc_dev);
		return err;
	}

#if defined ENABLE_ALARM && defined CONFIG_ARCH_SUN8IW6P1
	sunxi_alarm_disable(rtc_dev);

	type = script_get_item("rtc0", "rtc_int_ctrl", &item_eint);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("[AC100] script_get_item return type err\n");
	}
	rtc_dev->virq = gpio_to_irq(item_eint.gpio.gpio);
	if (IS_ERR_VALUE(rtc_dev->virq)) {
		pr_warn("[rtc_dev] map gpio to virq failed, errno = %d\n",rtc_dev->virq);
		return -EINVAL;
	}
	rtc_alarmno = rtc_dev->virq;
	pr_debug("[rtc_dev] gpio [%d] map to virq [%d] ok\n", item_eint.gpio.gpio, rtc_dev->virq);
	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(&rtc_dev->rtc->dev, rtc_dev->virq, sunxi_irq_handle, IRQF_TRIGGER_FALLING, "ALARM_EINT", rtc_dev);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("[AC100] request virq %d failed, errno = %d\n", rtc_dev->virq, ret);
	        return -EINVAL;
	}
	/*
	* item_eint.gpio.gpio = GPIO*(*);
	* select HOSC 24Mhz(PIO Interrupt Clock Select)
	*/
	req_status = gpio_request(item_eint.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_warn("[AC100]request gpio[%d] failed!\n", item_eint.gpio.gpio);
		return -EINVAL;
	}
	gpio_set_debounce(item_eint.gpio.gpio, 1);
#endif

	return 0;
}

static int __exit sunxi_rtc_remove(struct platform_device *pdev)
{
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);
#ifdef ENABLE_ALARM
	devm_free_irq(&rtc_dev->dev, rtc_alarmno, NULL);
#endif

	rtc_device_unregister(rtc_dev);
	kfree(rtc_dev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver sunxi_rtc_driver = {
	.probe		= sunxi_rtc_probe,
	.remove		= __exit_p(sunxi_rtc_remove),
	.driver		= {
		.name	= RTC_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init sunxi_rtc_init(void)
{
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("acx0", "ac200_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[acx0] ac200_used type err!\n");
	}
	ac200_used = val.val;
	type = script_get_item("acx0", "ac100_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[acx0] ac100_used type err!\n");
	}
	ac100_used = val.val;

	if (ac200_used&&(ac100_used==0)) {
		return platform_driver_register(&sunxi_rtc_driver);
	}
	return 0;
}

static void __exit sunxi_rtc_exit(void)
{
	platform_driver_unregister(&sunxi_rtc_driver);
}

MODULE_AUTHOR("liugang");
MODULE_DESCRIPTION("Allwinner RTC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

module_init(sunxi_rtc_init);
module_exit(sunxi_rtc_exit);

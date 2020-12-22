/*
 * drivers/rtc/rtc-sunxi-external.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
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
#include <linux/mfd/ac100-mfd.h>
#include <mach/sys_config.h>

#define DRV_VERSION "0.4.3"

/* reg off defination */
#define CLK32KOUT_ACTRL_OFF		0xc0
#define CK32K_OUT_CTRL1_OFF		0xc1
#define CK32K_OUT_CTRL2_OFF		0xc2
#define CK32K_OUT_CTRL3_OFF		0xc3
#define TWI_PAD_CTRL_OFF		0xc4
#define RTC_RST_REG_OFF			0xc6
#define RTC_CTRL_REG_OFF		0xc7
#define RTC_SEC_REG_OFF			0xc8
#define RTC_MIN_REG_OFF			0xc9
#define RTC_HOU_REG_OFF			0xca
#define RTC_WEE_REG_OFF			0xcb
#define RTC_DAY_REG_OFF			0xcc
#define RTC_MON_REG_OFF			0xcd
#define RTC_YEA_REG_OFF			0xce
#define RTC_UPD_TRIG_OFF		0xcf
#define ALM_INT_ENA_OFF			0xd0
#define ALM_INT_STA_REG_OFF		0xd1
#define ALM_SEC_REG_OFF			0xd8
#define ALM_MIN_REG_OFF			0xd9
#define ALM_HOU_REG_OFF			0xda
#define ALM_WEEK_REG_OFF		0xdb
#define ALM_DAY_REG_OFF			0xdc
#define ALM_MON_REG_OFF			0xdd
#define ALM_YEA_REG_OFF			0xde
#define ALM_UPD_TRIG_OFF		0xdf
#define RTC_GP_REG_N_OFF		0xe0

/* special macro defination */
#define LEAP_YEAR_FLAG		BIT(15)
#define RTC_WRITE_RTIG_FLAG	BIT(15)
#define RTC_12H_24H_MODE	BIT(0)

#define RTC_NAME		"rtc0"

#define ENABLE_ALARM

#ifdef ENABLE_ALARM
#define ALARM_IRQ_GPIO		GPIOE(0)	/* may need change */
#define ALARM_IRQ_NAME		"PE0_EINT"
#endif

#define MAX_YEAR_REG_VAL	99	/* year reg max value, it's 99 from 1670 spec */
#define YEAR_SUPPORT_MIN	1970	/* the mimimum year support to set, 1970 is from rtc_valid_tm(rtc-lib.c), donot change */
#define YEAR_SUPPORT_MAX	(YEAR_SUPPORT_MIN + MAX_YEAR_REG_VAL) /* the max year we support to set */

#define IS_LEAP_YEAR(year)	(((year)%400)==0 || (((year)%100)!=0 && ((year)%4)==0))

struct sunxi_rtc {
	struct rtc_device *rtc;
#ifdef ENABLE_ALARM
	struct work_struct work;
#endif
	struct mutex mutex;
	struct ac100 *ac100;
};

/* reg_val: raw value to be wrote to the reg, bcd convertion is done outside */
static inline void rtc_write_reg(struct ac100 *ac100, int reg, int value)
{
#if 0
	arisc_rsb_block_cfg_t rsb_data;
	unsigned char addr;
	unsigned int data;
	addr = (unsigned char)reg;

	data = value;
	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_HWORD;
	rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	rsb_data.devaddr = RSB_RTSADDR_AC100;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data;

	/* read axp registers */
	if (arisc_rsb_write_block_data(&rsb_data))
		pr_err("%s(%d) err: write reg-0x%x failed", __func__, __LINE__, reg);
#else
	ac100_reg_write(ac100, reg, value);
#endif
}

/* return raw value from the reg, then it may do bcd convertion if needed */
static inline int rtc_read_reg(struct ac100 *ac100, int reg)
{
#if 0
	arisc_rsb_block_cfg_t rsb_data;
	unsigned char addr;
	unsigned int data;

	addr = (unsigned char)reg;
	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_HWORD;
	rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	rsb_data.devaddr = RSB_RTSADDR_AC100;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data;

	/* read axp registers */
	if (arisc_rsb_read_block_data(&rsb_data)) {
		pr_err("%s(%d) err: read reg-0x%x failed", __func__, __LINE__, reg);
		return -EPERM;
	}
	return data;
#else
	return ac100_reg_read(ac100, reg);
#endif
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

	actual_year = tm->tm_year + 1900;	/* tm_year is from 1900 in linux */
	actual_month = tm->tm_mon + 1;		/* month is 1..12 in RTC reg but 0..11 in linux */
	leap_year = IS_LEAP_YEAR(actual_year);

	pr_info("%s(%d): time to set %d-%d-%d %d:%d:%d\n", __func__, __LINE__, actual_year,
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

	/* set time */
	rtc_write_reg(rtc_dev->ac100, RTC_SEC_REG_OFF, bin2bcd(tm->tm_sec) & 0x7f);
	rtc_write_reg(rtc_dev->ac100, RTC_MIN_REG_OFF, bin2bcd(tm->tm_min) & 0x7f);
	rtc_write_reg(rtc_dev->ac100, RTC_HOU_REG_OFF, bin2bcd(tm->tm_hour) & 0x3f);
	rtc_write_reg(rtc_dev->ac100, RTC_WEE_REG_OFF, bin2bcd(tm->tm_wday) & 0x7);
	rtc_write_reg(rtc_dev->ac100, RTC_DAY_REG_OFF, bin2bcd(tm->tm_mday) & 0x3f);
	rtc_write_reg(rtc_dev->ac100, RTC_MON_REG_OFF, bin2bcd(actual_month) & 0x1f);

	/* set year */
	reg_val = actual_year_to_reg_year(actual_year);
	reg_val = bin2bcd(reg_val) & 0xff;
	if (leap_year)
		reg_val |= LEAP_YEAR_FLAG;
	rtc_write_reg(rtc_dev->ac100, RTC_YEA_REG_OFF, reg_val);

	/* set write trig bit */
	rtc_write_reg(rtc_dev->ac100, RTC_UPD_TRIG_OFF, RTC_WRITE_RTIG_FLAG);
	mdelay(30); /* delay 30ms to wait write stable */

	pr_info("%s(%d): set time %d-%d-%d %d:%d:%d success!\n", __func__, __LINE__, actual_year,
		actual_month, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return 0;
}

static int rtc_gettime(struct sunxi_rtc *rtc_dev, struct rtc_time *tm)
{
	struct rtc_time time_tmp;
	int tmp;

	/* get sec */
	tmp = rtc_read_reg(rtc_dev->ac100, RTC_SEC_REG_OFF);
	time_tmp.tm_sec = bcd2bin(tmp & 0x7f);

	/* get min */
	tmp = rtc_read_reg(rtc_dev->ac100, RTC_MIN_REG_OFF);
	time_tmp.tm_min = bcd2bin(tmp & 0x7f);

	/* get hour */
	tmp = rtc_read_reg(rtc_dev->ac100, RTC_HOU_REG_OFF);
	time_tmp.tm_hour = bcd2bin(tmp & 0x3f);

	/* get week day */
	tmp = rtc_read_reg(rtc_dev->ac100, RTC_WEE_REG_OFF);
	time_tmp.tm_wday = bcd2bin(tmp & 0x7);

	/* get day */
	tmp = rtc_read_reg(rtc_dev->ac100, RTC_DAY_REG_OFF);
	time_tmp.tm_mday = bcd2bin(tmp & 0x3f);

	/* get month */
	tmp = rtc_read_reg(rtc_dev->ac100, RTC_MON_REG_OFF);
	time_tmp.tm_mon = bcd2bin(tmp & 0x1f);
	time_tmp.tm_mon -= 1; /* month is 1..12 in RTC reg but 0..11 in linux */

	/* get year */
	tmp = rtc_read_reg(rtc_dev->ac100, RTC_YEA_REG_OFF);
	tmp = bcd2bin(tmp & 0xff);
	time_tmp.tm_year = reg_year_to_actual_year(tmp) - 1900; /* in linux, tm_year=0 is 1900 */

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

	mutex_lock(&rtc_dev->mutex);
	ret = rtc_gettime(rtc_dev, tm);
	mutex_unlock(&rtc_dev->mutex);
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
#define ALM_YEA_ENA_FLAG	BIT(15)
#define ALM_MON_ENA_FLAG	BIT(15)
#define ALM_DAY_ENA_FLAG	BIT(15)
#define ALM_WEE_ENA_FLAG	BIT(15)
#define ALM_HOU_ENA_FLAG	BIT(15)
#define ALM_MIN_ENA_FLAG	BIT(15)
#define ALM_SEC_ENA_FLAG	BIT(15)
#define ALM_WRITE_RTIG_FLAG	BIT(15)
static int alarm_settime(struct sunxi_rtc *rtc_dev, struct rtc_time *tm)
{
	int actual_year, actual_month, reg_val;

	actual_year = tm->tm_year + 1900;	/* tm_year is from 1900 in linux */
	actual_month = tm->tm_mon + 1;		/* month is 1..12 in RTC reg but 0..11 in linux */

	pr_info("%s(%d): time to set %d-%d-%d %d:%d:%d\n", __func__, __LINE__, actual_year,
		actual_month, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* set sec */
	reg_val = bin2bcd(tm->tm_sec) & 0x7f;
	reg_val |= ALM_SEC_ENA_FLAG;
	rtc_write_reg(rtc_dev->ac100, ALM_SEC_REG_OFF, reg_val);

	/* set minute */
	reg_val = bin2bcd(tm->tm_min) & 0x7f;
	reg_val |= ALM_MIN_ENA_FLAG;
	rtc_write_reg(rtc_dev->ac100, ALM_MIN_REG_OFF, reg_val);

	/* set hour */
	reg_val = bin2bcd(tm->tm_hour) & 0x3f;
	reg_val |= ALM_HOU_ENA_FLAG;
	rtc_write_reg(rtc_dev->ac100, ALM_HOU_REG_OFF, reg_val);

	/* set week */
	reg_val = bin2bcd(tm->tm_wday) & 0x7;
#if 0 /* donot both enable day & week alarm */
	reg_val |= ALM_WEE_ENA_FLAG;
#endif
	rtc_write_reg(rtc_dev->ac100, ALM_WEEK_REG_OFF, reg_val);

	/* set day */
	reg_val = bin2bcd(tm->tm_mday) & 0x3f;
	reg_val |= ALM_DAY_ENA_FLAG;
	rtc_write_reg(rtc_dev->ac100, ALM_DAY_REG_OFF, reg_val);

	/* set month */
	reg_val = bin2bcd(actual_month) & 0x1f;
	reg_val |= ALM_MON_ENA_FLAG;
	rtc_write_reg(rtc_dev->ac100, ALM_MON_REG_OFF, reg_val);

	/* set year */
	reg_val = actual_year_to_reg_year(actual_year);
	reg_val = bin2bcd(reg_val) & 0xff;
	reg_val |= ALM_YEA_ENA_FLAG;
	rtc_write_reg(rtc_dev->ac100, ALM_YEA_REG_OFF, reg_val);

	/* set write trig bit */
	rtc_write_reg(rtc_dev->ac100, ALM_UPD_TRIG_OFF, ALM_WRITE_RTIG_FLAG);
	mdelay(30); /* delay 30ms to wait stable */

	pr_info("%s(%d): set alarm time %d-%d-%d %d:%d:%d success!\n", __func__, __LINE__, actual_year,
		actual_month, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	return 0;
}

static int alarm_gettime(struct sunxi_rtc *rtc_dev, struct rtc_time *tm)
{
	int reg_val;

	/* get sec */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_SEC_REG_OFF);
	tm->tm_sec = bcd2bin(reg_val & 0x7f);

	/* get minute */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_MIN_REG_OFF);
	tm->tm_min = bcd2bin(reg_val & 0x7f);

	/* get hour */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_HOU_REG_OFF);
	tm->tm_hour = bcd2bin(reg_val & 0x3f);

	/* get week */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_WEEK_REG_OFF);
	tm->tm_wday = bcd2bin(reg_val & 0x7);

	/* get day */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_DAY_REG_OFF);
	tm->tm_mday = bcd2bin(reg_val & 0x3f);

	/* get month */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_MON_REG_OFF);
	reg_val = bcd2bin(reg_val & 0x1f);
	tm->tm_mon = reg_val - 1; /* month is 1..12 in RTC reg but 0..11 in linux */

	/* get year */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_YEA_REG_OFF);
	reg_val = bcd2bin(reg_val & 0xff);
	tm->tm_year = reg_year_to_actual_year(reg_val) - 1900; /* tm_year is from 1900 in linux */

	pr_info("%s(%d): get alarm time %d-%d-%d %d:%d:%d success!\n", __func__, __LINE__, tm->tm_year + 1900,
		tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* check if time read from rtc reg is OK */
	if (!time_valid(tm)) {
		pr_err("%s(%d) err: get time is not valid.\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

static void alarm_enable(struct sunxi_rtc *rtc_dev)
{
#ifdef CONFIG_ARCH_SUN8IW6P1
	int reg_val = 0;
#endif
	/* enable alarm irq */
	rtc_write_reg(rtc_dev->ac100, ALM_INT_ENA_OFF, 1);

#ifdef CONFIG_ARCH_SUN8IW6P1
	/*
	 * shutdown alarm need alarm interrupt enable info when system startup, but
	 * the info be cleared in u-boot boot process for a problem. so store the
	 * info to RTC_GP_REG_N_OFF reg.
	 * */
	reg_val = rtc_read_reg(rtc_dev->ac100, RTC_GP_REG_N_OFF);
	reg_val |= 0x1;
	rtc_write_reg(rtc_dev->ac100, RTC_GP_REG_N_OFF, reg_val);
#endif
}

static void alarm_disable(struct sunxi_rtc *rtc_dev)
{
	int reg_val = 0;
	/* disable alarm irq */
	rtc_write_reg(rtc_dev->ac100, ALM_INT_ENA_OFF, 0);

	/* clear alarm irq pending */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_INT_STA_REG_OFF);
	if (reg_val & 1) {
		pr_err("%s(%d) maybe err: alarm irq pending is set, clear it! reg is 0x%x\n",
			__func__, __LINE__, reg_val);
		rtc_write_reg(rtc_dev->ac100, ALM_INT_STA_REG_OFF, 1);
	}

#ifdef CONFIG_ARCH_SUN8IW6P1
	/*
	 * shutdown alarm need alarm interrupt enable info when system startup, but
	 * the info be cleared in u-boot boot process for a problem. so store the
	 * info to RTC_GP_REG_N_OFF reg.
	 * */
	reg_val = rtc_read_reg(rtc_dev->ac100, RTC_GP_REG_N_OFF);
	reg_val &= ~0x1;
	rtc_write_reg(rtc_dev->ac100, RTC_GP_REG_N_OFF, reg_val);
#endif
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

static int sunxi_irq_handle(void *dev_id)
{
	struct sunxi_rtc *rtc_dev = dev_id;

	(void)schedule_work(&rtc_dev->work);
	return 0;
}

static void alarm_irq_work(struct work_struct *work)
{
	struct sunxi_rtc *rtc_dev = container_of(work, struct sunxi_rtc, work);
	int reg_val;

	mutex_lock(&rtc_dev->mutex);

	/* clear alarm irq pending */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_INT_STA_REG_OFF);
	if (reg_val < 0) {
		mutex_unlock(&rtc_dev->mutex);
		pr_err("%s(%d) err: read alarm int status reg failed!\n", __func__, __LINE__);
		goto end;
	}

	if (reg_val & 1) {
		rtc_write_reg(rtc_dev->ac100, ALM_INT_STA_REG_OFF, 1);
		mutex_unlock(&rtc_dev->mutex);

		pr_err("%s(%d): alarm interrupt!\n", __func__, __LINE__);
		rtc_update_irq(rtc_dev->rtc, 1, RTC_AF | RTC_IRQF);
		goto end;
	}

	mutex_unlock(&rtc_dev->mutex);
end:
	arisc_enable_nmi_irq();
}

static int sunxi_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtc *rtc_dev = platform_get_drvdata(pdev);
	int ret = 0;
	int reg_val = 0;

	mutex_lock(&rtc_dev->mutex);
	/* get alarm time */
	if (alarm_gettime(rtc_dev, &alrm->time)) {
		pr_err("%s(%d) err: get alarm time failed!\n", __func__, __LINE__);
		ret = -EINVAL;
		goto end;
	}

	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_INT_ENA_OFF);
	if (reg_val & 0x1)
		alrm->enabled = 1;

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

	/* set alarm time */
	if (alarm_settime(rtc_dev, &alrm->time)) {
		mutex_unlock(&rtc_dev->mutex);
		pr_err("%s(%d) err: set alarm time failed!\n", __func__, __LINE__);
		return -EINVAL;
	}

	pr_info("%s(%d): alarm->enabled=%d\n", __func__, __LINE__, alrm->enabled);
	/* enable alarm if flag set */
	if (alrm->enabled) {
		arisc_enable_nmi_irq(); /* temperally, but no better way */
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

	pr_info("%s(%d): enabled=%d\n", __func__, __LINE__, enabled);
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

extern struct i2c_device_id rtc_device_id[];
extern unsigned short rtc_address_list[];
extern struct i2c_driver rtc_i2c_driver;

static const struct rtc_class_ops sunxi_rtc_ops = {
	.read_time      = sunxi_rtc_read_time,
	.set_time       = sunxi_rtc_set_time,
#ifdef ENABLE_ALARM
	.read_alarm     = sunxi_rtc_getalarm,
	.set_alarm      = sunxi_rtc_setalarm,
	.alarm_irq_enable = sunxi_rtc_alarm_irq_enable,
#endif
};

void sunxi_rtc_hw_init(struct sunxi_rtc *rtc_dev)
{
	int reg_val;

	/* reset rtc reg??? */

	/* set 24h mode */
	rtc_write_reg(rtc_dev->ac100, RTC_CTRL_REG_OFF, RTC_12H_24H_MODE);

	/* disable alarm irq remove to after rtc_device_register() */
	// rtc_write_reg(rtc_dev->ac100, ALM_INT_ENA_OFF, 0);

	/* clear alarm irq pending */
	reg_val = rtc_read_reg(rtc_dev->ac100, ALM_INT_STA_REG_OFF);
	if (!IS_ERR_VALUE(reg_val) && (reg_val & 1))
		rtc_write_reg(rtc_dev->ac100, ALM_INT_STA_REG_OFF, 1);
}

static int __init sunxi_rtc_probe(struct platform_device *pdev)
{
	struct sunxi_rtc *rtc_dev;
	int err = 0;
#ifdef CONFIG_ARCH_SUN8IW6P1
	int reg_val = 0;
#endif
	pr_debug("%s,line:%d\n", __func__, __LINE__);
	rtc_dev = kzalloc(sizeof(struct sunxi_rtc), GFP_KERNEL);
	if (!rtc_dev)
		return -ENOMEM;

	/* must before rtc_device_register, because it will call rtc_device_register -> __rtc_read_alarm ->
	 *  rtc_read_time-> __rtc_read_time -> sunxi_rtc_read_time, witch may use platform_get_drvdata. */
	platform_set_drvdata(pdev, rtc_dev);
	rtc_dev->ac100 = dev_get_drvdata(pdev->dev.parent);

#ifdef ENABLE_ALARM
	INIT_WORK(&rtc_dev->work, alarm_irq_work);
	device_init_wakeup(&pdev->dev, true); /* enable alarm wakeup */
#endif
	mutex_init(&rtc_dev->mutex);

	sunxi_rtc_hw_init(rtc_dev);

#ifdef CONFIG_ARCH_SUN8IW6P1
	/*
	 * restore alarm interrupt enable which is cleared by u-boot,
	 * rtc_device_register() need alarm interrupt enable bit to initialize alarm
	 * */
	reg_val = rtc_read_reg(rtc_dev->ac100, RTC_GP_REG_N_OFF);
	if (reg_val & 0x1) {
		rtc_write_reg(rtc_dev->ac100, ALM_INT_ENA_OFF, 1);
	}
#endif

	rtc_dev->rtc = rtc_device_register(RTC_NAME, &pdev->dev, &sunxi_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc_dev->rtc)) {
		err = PTR_ERR(rtc_dev->rtc);
		kfree(rtc_dev);
		return err;
	}

#ifdef ENABLE_ALARM
	sunxi_alarm_disable(rtc_dev);

	pr_info("%s(%d)!\n", __func__, __LINE__);
	err = arisc_nmi_cb_register(NMI_INT_TYPE_RTC, sunxi_irq_handle, (void *)rtc_dev);
	if (IS_ERR_VALUE(err)) {
		pr_err("%s(%d): request irq failed\n", __func__, __LINE__);
		rtc_device_unregister(rtc_dev->rtc);
		kfree(rtc_dev);
		return err;
	}
#endif

	return 0;
}

static int __exit sunxi_rtc_remove(struct platform_device *pdev)
{
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);

#ifdef ENABLE_ALARM
	arisc_nmi_cb_unregister(NMI_INT_TYPE_RTC, sunxi_irq_handle);
	//sunxi_rtc_setaie(0);
#endif

	rtc_device_unregister(rtc_dev);
	kfree(rtc_dev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static void sunxi_rtc_shutdown(struct platform_device *pdev)
{
	#ifdef CONFIG_ARCH_SUN8IW6
	struct sunxi_rtc *rtc_dev = platform_get_drvdata(pdev);
	int reg_val;
	reg_val = rtc_read_reg(rtc_dev->ac100, CK32K_OUT_CTRL1_OFF);
	reg_val &= ~(0x1<<0);
	rtc_write_reg(rtc_dev->ac100, CK32K_OUT_CTRL1_OFF, reg_val);

	reg_val = rtc_read_reg(rtc_dev->ac100, CK32K_OUT_CTRL2_OFF);
	reg_val &= ~(0x1<<0);
	rtc_write_reg(rtc_dev->ac100, CK32K_OUT_CTRL2_OFF, reg_val);

	reg_val = rtc_read_reg(rtc_dev->ac100, CK32K_OUT_CTRL3_OFF);
	reg_val &= ~(0x1<<0);
	rtc_write_reg(rtc_dev->ac100, CK32K_OUT_CTRL3_OFF, reg_val);
	#endif
}


static struct platform_driver sunxi_rtc_driver = {
	.probe		= sunxi_rtc_probe,
	.shutdown	= sunxi_rtc_shutdown,
	.remove		= __exit_p(sunxi_rtc_remove),
	.driver		= {
		.name	= RTC_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init sunxi_rtc_init(void)
{
	pr_info("%s(%d): sunxi rtc device register!\n", __func__, __LINE__);
	return platform_driver_register(&sunxi_rtc_driver);
}

static void __exit sunxi_rtc_exit(void)
{
	pr_info("%s(%d): sunxi rtc device unregister!\n", __func__, __LINE__);
	platform_driver_unregister(&sunxi_rtc_driver);
}

MODULE_AUTHOR("liugang");
MODULE_DESCRIPTION("Allwinner RTC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

module_init(sunxi_rtc_init);
module_exit(sunxi_rtc_exit);

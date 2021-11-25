/*
 * RTC subsystem, initialize system time on startup
 *
 * Copyright (C) 2005 Tower Technologies
 * Author: Alessandro Zummo <a.zummo@towertech.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/rtc.h>

#include <generated/compile.h>

/* IMPORTANT: the RTC only stores whole seconds. It is arbitrary
 * whether it stores the most close value or the value with partial
 * seconds truncated. However, it is important that we use it to store
 * the truncated value. This is because otherwise it is necessary,
 * in an rtc sync function, to read both xtime.tv_sec and
 * xtime.tv_nsec. On some processors (i.e. ARM), an atomic read
 * of >32bits is not possible. So storing the most close value would
 * slow down the sync API. So here we have the truncated value and
 * the best guess is to add 0.5s.
 */

#define MONTHS_PER_YEAR 12
static char *month[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};
static int uts_version_to_tm(struct rtc_time *tm, struct rtc_device *rtc)
{
	char mon[32], *s;
	int i, get_month = 0, ret = 0;

	s = UTS_VERSION;
	while (*s != 'S')
		s = s + 1;
	ret = sscanf(s, "SMP PREEMPT Thu %03s %02d %02d:%02d:%02d CST %04d",
			mon, &tm->tm_mday, &tm->tm_hour, &tm->tm_min,
			&tm->tm_sec, &tm->tm_year);
	if (ret != 6)
		return -EINVAL;

	for (i = 0; i < MONTHS_PER_YEAR; i++) {
		if (!strncmp(mon, month[i], 3)) {
			tm->tm_mon = i;
			get_month = 1;
			break;
		}
	}
	if (get_month != 1)
		return -EINVAL;

	dev_info(rtc->dev.parent,
		"utc version to rtc_tm:  "
		"%04d-%02d-%02d %02d:%02d:%02d\n",
		tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	tm->tm_year -= 1900;
	return 0;
}

static int __init rtc_hctosys(void)
{
	int err = -ENODEV;
	struct rtc_time tm;
	struct timespec64 tv64 = {
		.tv_nsec = NSEC_PER_SEC >> 1,
	};
	struct rtc_device *rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);

	if (rtc == NULL) {
		pr_info("unable to open rtc device (%s)\n",
			CONFIG_RTC_HCTOSYS_DEVICE);
		goto err_open;
	}

	err = rtc_read_time(rtc, &tm);
	if (err) {
		dev_err(rtc->dev.parent,
			"hctosys: unable to read the hardware clock\n");
		goto err_read;

	}

	tv64.tv_sec = rtc_tm_to_time64(&tm);

	if (tv64.tv_sec < 0) { //rtc get the wrong value
		if (!uts_version_to_tm(&tm, rtc)) {
			err = rtc_set_time(rtc, &tm);
			if (err) {
				dev_err(rtc->dev.parent,
					"hctosys: unable to set the hardware clock\n");
				goto err_set;
			}
			err = rtc_read_time(rtc, &tm);
			if (err) {
				dev_err(rtc->dev.parent,
				"hctosys: unable to read the hardware clock\n");
				goto err_read;
			}
			tv64.tv_sec = rtc_tm_to_time64(&tm);
		}
	}

#if BITS_PER_LONG == 32
	if (tv64.tv_sec > INT_MAX)
		goto err_read;
#endif

	err = do_settimeofday64(&tv64);

	dev_info(rtc->dev.parent,
		"setting system clock to "
		"%d-%02d-%02d %02d:%02d:%02d UTC (%lld)\n",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		(long long) tv64.tv_sec);

err_set:
err_read:
	rtc_class_close(rtc);

err_open:
	rtc_hctosys_ret = err;

	return err;
}

late_initcall(rtc_hctosys);

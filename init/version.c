// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/init/version.c
 *
 *  Copyright (C) 1992  Theodore Ts'o
 *
 *  May be freely distributed as part of Linux.
 */

#include <generated/compile.h>
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/uts.h>
#include <linux/utsname.h>
#include <generated/utsrelease.h>
#include <linux/proc_ns.h>

static int __init early_hostname(char *arg)
{
	size_t bufsize = sizeof(init_uts_ns.name.nodename);
	size_t maxlen  = bufsize - 1;
	size_t arglen;

	arglen = strlcpy(init_uts_ns.name.nodename, arg, bufsize);
	if (arglen > maxlen) {
		pr_warn("hostname parameter exceeds %zd characters and will be truncated",
			maxlen);
	}
	return 0;
}
early_param("hostname", early_hostname);

const char linux_proc_banner[] =
	"%s version %s"
	" (" LINUX_COMPILE_BY "@" LINUX_COMPILE_HOST ")"
	" (" LINUX_COMPILER ") %s\n";

BUILD_SALT;
BUILD_LTO_INFO;

/*
 * init_uts_ns and linux_banner contain the build version and timestamp,
 * which are really fixed at the very last step of build process.
 * They are compiled with __weak first, and without __weak later.
 */

struct uts_namespace init_uts_ns __weak;
const char linux_banner[] __weak;

#include "version-timestamp.c"

EXPORT_SYMBOL_GPL(init_uts_ns);

/* UTV_VERSION fomat: "#1 SMP PREEMPT Wed Jun 15 20:21:27 CST 2016"
*  just get time from the pos Month of UTS_VERSION first,
*  get time from string like: "Jun 15 20:21:27 CST 2016"
* function     : get_kernel_build_time
* description  : get build date and build time of kernel
* input        : blddt : buffer for get build date
*                dtlen : length of blddt buffer
*                bldtm : buffer for get build time
*                tmlen : length of bldtm buffer
* output       : blddt : kernel build date string
*                bldtm : kernel build time string
* ret value  : 0   successfull
*              <0  failed to get date&time
*****************************************************************************/
/*hh:mm:ss*/
#define TIMELEN 8
/*"XXX xx 2XXX"*/
#define DATELEN 11
#define MONTHCOUNT 12
#define DATETIMELEN 23
#define YEARLEN 4
#define MONLEN 3
#define MAXDAYLEN 2
enum {
	MONSTR=1,
	DAYSTR=2,
	TIMESTR=3,
	STYLESTR=4,
};

int get_kernel_build_time(char* blddt, int dtlen, char* bldtm, int tmlen)
{
	char *timepos = NULL;
	int i = 0;
	int dstlen = 0;
	int ret = -EINVAL;
	const char *str_mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	if ((NULL == blddt || NULL == bldtm) ||
		(DATELEN > dtlen || TIMELEN > tmlen))
	{
		pr_err("input invalid params!!\n");
		return ret;
	}

	/* find start position of UTS_VERSION */
	timepos = strrchr(linux_banner, ')');
	if (NULL == timepos)
	{
		pr_err("please check whether linux_banner changed or not!!!");
		return ret;
	}

	for (i = 0; i < MONTHCOUNT; i++)
	{
		char *tmpstr = NULL;

		tmpstr = strstr(timepos, str_mon[i]);
		if (tmpstr && strlen(tmpstr) >= DATETIMELEN)
		{
			char *tmpdt = NULL;
			char *delm = " ";
			int subindex = 0;
			int getinval = 0;

			tmpdt = strstr(tmpstr, delm);
			while (tmpdt)
			{
				int getstrlen = tmpdt - tmpstr;

				subindex++;
				switch (subindex) {
				/* get month string */
				case MONSTR:
					if (getstrlen > (MONLEN + 1))
					{
						getinval = -EINVAL;
						pr_err("please check whether UTS_VERSION changed or not, can not get month value!!\n");
						break;
					}
					memset(blddt, 0, dtlen);
					/* cpy ' ' at the same time */
					memcpy(blddt, tmpstr, getstrlen + 1);
					dstlen = getstrlen + 1;
					break;
				/* get days string */
				case DAYSTR:
					if (getstrlen > (MAXDAYLEN + 1))
					{
						getinval = -EINVAL;
						pr_err("please check whether UTS_VERSION changed or not, can not get date value!!\n");
						break;
					}
					//cpy ' ' at the same time
					memcpy(blddt + dstlen, tmpstr, getstrlen + 1);
					dstlen += (getstrlen + 1);
					break;
				/* get time string */
				case TIMESTR:
					if (getstrlen > TIMELEN)
					{
						getinval = -EINVAL;
						pr_err("please check whether UTS_VERSION changed or not, can not get time value!!\n");
						break;
					}
					memset(bldtm, 0, tmlen);
					memcpy(bldtm, tmpstr, getstrlen);
					break;
				/* skip string like "CST" "UTS" "GMT" */
				case STYLESTR:
					break;
				default:
					break;
				}

				do {
					tmpdt++;
				}while(*tmpdt == ' ');

				tmpstr = tmpdt;
				if (tmpstr == NULL)
					break;
				tmpdt = strstr(tmpstr, delm);
				if (subindex >= STYLESTR)
				{
					if (strlen(tmpstr) >= YEARLEN)
					{
						memcpy(blddt + dstlen, tmpstr, YEARLEN);
						ret = 0;
					}
					break;
				}

				if (getinval != 0)
					break;
			}
			break;
		}
	}

	if (ret)
	{
		pr_err("please check whether UTS_VERSION changed or not!!\n");
	}

	return ret;
}
EXPORT_SYMBOL_GPL(get_kernel_build_time);

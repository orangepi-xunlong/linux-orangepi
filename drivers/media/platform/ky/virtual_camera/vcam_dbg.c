// SPDX-License-Identifier: GPL-2.0
/*
 * vcam_dbg.c - vcamera debug utility
 *
 * Copyright(C) 2023 KY Micro Limited
 */
#define DEBUG			/* for pr_debug() */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include "vcam_dbg.h"

static uint debug_mdl = 0x0; /* disable all modules at default */
//static uint debug_mdl = 0x1FF; /* enable all modules for debug */

void vcam_printk(int module_tag, const char *vcam_level, const char *kern_level,
		const char *func, int line, const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, format);

	vaf.fmt = format;
	vaf.va = &args;

	printk("%s" "%s: %s %d: %pV\n", kern_level, vcam_level, func, line, &vaf);
	va_end(args);
}

EXPORT_SYMBOL(vcam_printk);

void vcam_printk_ratelimited(int module_tag, const char *vcam_level,
			    const char *kern_level, const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, format);

	vaf.fmt = format;
	vaf.va = &args;

	printk_ratelimited("%s" "%s: %pV\n", kern_level, vcam_level, &vaf);
	va_end(args);
}

EXPORT_SYMBOL(vcam_printk_ratelimited);

void vcam_debug(int module_tag, const char *vcam_level, const char *func, int line, const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	if (!debug_mdl)
		return;

	va_start(args, format);

	vaf.fmt = format;
	vaf.va = &args;

	pr_debug("%s: %s %d: %pV\n", vcam_level, func, line, &vaf);
	va_end(args);
}

EXPORT_SYMBOL(vcam_debug);

// MODULE_PARM_DESC(debug_mdl, "Enable debug output, where each bit enables a module.\n"
// 				 "\t\tBit 0 (0x01)  will enable VI messages\n"
// 				 "\t\tBit 1 (0x02)  will enable ISP messages\n"
// 				 "\t\tBit 2 (0x04)  will enable CPP messages\n"
// 				 "\t\tBit 3 (0x08)  will enable VBE messages\n"
// 				 "\t\tBit 4 (0x10)  will enable SENSOR messages\n"
// 				 "\t\tBit 5 (0x20)  will enable IRCUT messages\n"
// 				 "\t\tBit 8 (0x100) will enable COMMON messages");
module_param(debug_mdl, uint, 0644);


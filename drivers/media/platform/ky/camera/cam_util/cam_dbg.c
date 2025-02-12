// SPDX-License-Identifier: GPL-2.0
/*
 * cam_dbg.c - camera debug utility
 *
 * Copyright(C) 2023 KY Micro Limited
 */
#define DEBUG			/* for pr_debug() */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include "cam_dbg.h"

static uint debug_mdl = 0x0; /* disable all modules at default */
//static uint debug_mdl = 0x1FF; /* enable all modules for debug */

static const char *cam_mdl_str[] = {
	[CAM_MDL_VI] = "vi",
	[CAM_MDL_ISP] = "isp",
	[CAM_MDL_CPP] = "cpp",
	[CAM_MDL_VBE] = "vbe",
	[CAM_MDL_SNR] = "snr",
	[CAM_MDL_IRCUT] = "ircut",
	[CAM_MDL_COMMON] = "",
};

void cam_printk(int module_tag, const char *cam_level, const char *kern_level,
		const char *func, int line, const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, format);

	vaf.fmt = format;
	vaf.va = &args;

	printk("%s" "%s:%s (%s %d): %pV\n", kern_level, cam_level, cam_mdl_str[module_tag], func, line, &vaf);
	va_end(args);
}

EXPORT_SYMBOL(cam_printk);

void cam_printk_ratelimited(int module_tag, const char *cam_level,
			    const char *kern_level, const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, format);

	vaf.fmt = format;
	vaf.va = &args;

	printk_ratelimited("%s" "%s: %s: %pV\n", kern_level, cam_level,
			   cam_mdl_str[module_tag], &vaf);
	va_end(args);
}

EXPORT_SYMBOL(cam_printk_ratelimited);

void cam_debug(int module_tag, const char *cam_level, const char *func, int line, const char *format, ...)
{
	struct va_format vaf;
	va_list args;

	if (!(debug_mdl & (1 << module_tag)))
		return;

	va_start(args, format);

	vaf.fmt = format;
	vaf.va = &args;

	pr_debug("%s:%s (%s %d): %pV\n", cam_level, cam_mdl_str[module_tag], func, line, &vaf);
	va_end(args);
}

EXPORT_SYMBOL(cam_debug);

MODULE_PARM_DESC(debug_mdl, "Enable debug output, where each bit enables a module.\n"
				 "\t\tBit 0 (0x01)  will enable VI messages\n"
				 "\t\tBit 1 (0x02)  will enable ISP messages\n"
				 "\t\tBit 2 (0x04)  will enable CPP messages\n"
				 "\t\tBit 3 (0x08)  will enable VBE messages\n"
				 "\t\tBit 4 (0x10)  will enable SENSOR messages\n"
				 "\t\tBit 5 (0x20)  will enable IRCUT messages\n"
				 "\t\tBit 8 (0x100) will enable COMMON messages");
module_param(debug_mdl, uint, 0644);

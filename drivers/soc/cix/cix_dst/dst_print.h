/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DST_PRINTER_H__
#define __DST_PRINTER_H__

#include <linux/types.h>
#include <linux/printk.h>

#define DST_ALERT(fmt, args...) \
	pr_alert("<dst::%s:%d> " fmt, __FUNCTION__, __LINE__, ##args)

#define DST_ERR(fmt, args...) \
	pr_err("<dst::%s:%d error> " fmt, __FUNCTION__, __LINE__, ##args)

#define DST_PN(fmt, args...) \
	pr_info("<dst::%s:%d> " fmt, __FUNCTION__, __LINE__, ##args)

#define DST_DBG(fmt, args...) \
	pr_debug("<dst::%s:%d> " fmt, __FUNCTION__, __LINE__, ##args)

#define DST_PR_START() pr_debug("<dst::%s:%d> enter\n", __FUNCTION__, __LINE__)

#define DST_PR_END() pr_debug("<dst::%s:%d> exit\n", __FUNCTION__, __LINE__)

#endif /* End #define __DST_PRINTER_H__ */

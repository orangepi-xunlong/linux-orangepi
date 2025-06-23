#ifndef __DST_PRINTER_H__
#define __DST_PRINTER_H__

#include <linux/types.h>
#include <linux/printk.h>

#define DST_PRINT_PN(args...)    pr_info("<dst>"args)
#define DST_PRINT_ERR(args...)   pr_err("<dst error>"args)
#define DST_PRINT_DBG(args...)   pr_debug("<dst>"args)
#define DST_PRINT_START(args...) \
	pr_debug(">>>>>enter dst %s: %.4d\n", __func__, __LINE__)
#define DST_PRINT_END(args...)   \
	pr_debug("<<<<<exit  dst %s: %.4d\n", __func__, __LINE__)

#endif /* End #define __DST_PRINTER_H__ */

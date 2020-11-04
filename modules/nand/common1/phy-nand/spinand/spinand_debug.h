/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SPINAND_DEBUG_H__
#define __SPINAND_DEBUG_H__
#include <linux/printk.h>
#include <linux/kernel.h>


#define SPINAND_INFO(...) printk(KERN_INFO "[NI] "__VA_ARGS__)
#define SPINAND_DBG(...) printk(KERN_DEBUG "[ND] "__VA_ARGS__)
#define SPINAND_ERR(...) printk(KERN_ERR "[NE] "__VA_ARGS__)

#define spinand_print(fmt, arg...)                        \
	do {                                                \
		printk("[spinand] "fmt "\n", ##arg); \
	} while (0)

#endif /*SPINAND_DEBUG_H*/

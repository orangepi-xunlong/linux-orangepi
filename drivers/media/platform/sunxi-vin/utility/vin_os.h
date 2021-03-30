
/*
 ******************************************************************************
 *
 * vin_os.h
 *
 * Hawkview ISP - vin_os.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/12/02	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef __VIN__OS__H__
#define __VIN__OS__H__

#include <linux/device.h>
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/interrupt.h>
#include "../platform/platform_cfg.h"

#ifdef SUNXI_MEM
#include <linux/ion.h>		/*for all "ion api"*/
#include <linux/ion_sunxi.h>	/*for import "sunxi_ion_client_create"*/
#include <linux/dma-mapping.h>	/*just include "PAGE_SIZE" macro*/
#else
#include <linux/dma-mapping.h>
#endif
#define IS_FLAG(x, y) (((x)&(y)) == y)

#define VIN_LOG_MD				(1 << 0)	/*0x0 */
#define VIN_LOG_FLASH				(1 << 1)	/*0x2 */
#define VIN_LOG_CCI				(1 << 2)	/*0x4 */
#define VIN_LOG_CSI				(1 << 3)	/*0x8 */
#define VIN_LOG_MIPI				(1 << 4)	/*0x10*/
#define VIN_LOG_ISP				(1 << 5)	/*0x20*/
#define VIN_LOG_STAT				(1 << 6)	/*0x40*/
#define VIN_LOG_SCALER				(1 << 7)	/*0x80*/
#define VIN_LOG_POWER				(1 << 8)	/*0x100*/
#define VIN_LOG_CONFIG				(1 << 9)	/*0x200*/
#define VIN_LOG_VIDEO				(1 << 10)	/*0x400*/
#define VIN_LOG_FMT				(1 << 11)	/*0x800*/

extern unsigned int vin_log_mask;

#define vin_log(flag, arg...) do { \
	if (flag & vin_log_mask) { \
		switch (flag) { \
		case VIN_LOG_MD: \
			printk(KERN_DEBUG "[VIN_LOG_MD]" arg); \
			break; \
		case VIN_LOG_FLASH: \
			printk(KERN_DEBUG "[VIN_LOG_FLASH]" arg); \
			break; \
		case VIN_LOG_CCI: \
			printk(KERN_DEBUG "[VIN_LOG_CCI]" arg); \
			break; \
		case VIN_LOG_CSI: \
			printk(KERN_DEBUG "[VIN_LOG_CSI]" arg); \
			break; \
		case VIN_LOG_MIPI: \
			printk(KERN_DEBUG "[VIN_LOG_MIPI]" arg); \
			break; \
		case VIN_LOG_ISP: \
			printk(KERN_DEBUG "[VIN_LOG_ISP]" arg); \
			break; \
		case VIN_LOG_STAT: \
			printk(KERN_DEBUG "[VIN_LOG_STAT]" arg); \
			break; \
		case VIN_LOG_SCALER: \
			printk(KERN_DEBUG "[VIN_LOG_SCALER]" arg); \
			break; \
		case VIN_LOG_POWER: \
			printk(KERN_DEBUG "[VIN_LOG_POWER]" arg); \
			break; \
		case VIN_LOG_CONFIG: \
			printk(KERN_DEBUG "[VIN_LOG_CONFIG]" arg); \
			break; \
		case VIN_LOG_VIDEO: \
			printk(KERN_DEBUG "[VIN_LOG_VIDEO]" arg); \
			break; \
		case VIN_LOG_FMT: \
			printk(KERN_DEBUG "[VIN_LOG_FMT]" arg); \
			break; \
		default: \
			printk(KERN_DEBUG "[VIN_LOG]" arg); \
			break; \
		} \
	} \
} while (0)

#define vin_err(x, arg...) printk(KERN_ERR"[VIN_ERR]"x, ##arg)
#define vin_warn(x, arg...) printk(KERN_WARNING"[VIN_WARN]"x, ##arg)
#define vin_print(x, arg...) printk(KERN_NOTICE"[VIN]"x, ##arg)

struct vin_mm {
	size_t size;
	void *phy_addr;
	void *vir_addr;
	void *dma_addr;
	struct ion_client *client;
	struct ion_handle *handle;
};

extern int os_gpio_request(struct gpio_config *gpio_list,
			   __u32 group_count_max);
extern int os_gpio_set(struct gpio_config *gpio_list, __u32 group_count_max);
extern int os_gpio_release(u32 p_handler, __s32 if_release_to_default_status);
extern int os_gpio_write(u32 p_handler, __u32 value_to_gpio,
			 const char *gpio_name, int force_value_flag);
extern int os_gpio_set_status(u32 p_handler, __u32 if_set_to_output_status,
			      const char *gpio_name);
extern int os_mem_alloc(struct vin_mm *mem_man);
extern void os_mem_free(struct vin_mm *mem_man);

#endif	/*__VIN__OS__H__*/

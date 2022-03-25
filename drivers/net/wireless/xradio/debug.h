/*
 * DebugFS code for XRadio drivers
 *
 * Copyright (c) 2013, XRadio
 * Author: XRadio
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef XRADIO_DEBUG_H_INCLUDED
#define XRADIO_DEBUG_H_INCLUDED

/* debug classes */
#define XRADIO_DBG_ALWY   0x01		/* general - always */
#define XRADIO_DBG_ERROR  0x02		/* error */
#define XRADIO_DBG_WARN   0x04		/* warning */
#define XRADIO_DBG_NIY    0x08		/* info (parameters etc.) */
#define XRADIO_DBG_MSG    0x10		/* verbose debug */
#define XRADIO_DBG_OPS    0x20		/* trace IEEE802.11 ops calls */
#define XRADIO_DBG_DEV    0x40		/* current dev */

/* debug levels, default 0x07 */
#define XRADIO_DBG_LEV_AP	0x47	/* ap */
#define XRADIO_DBG_LEV_PM	0x47	/* pm */
#define XRADIO_DBG_LEV_SCAN	0x47	/* scan */
#define XRADIO_DBG_LEV_STA	0x47	/* sta, keys */
#define XRADIO_DBG_LEV_TXRX	0x47	/* tx, rx, ht, p2p, queue */
#define XRADIO_DBG_LEV_WSM	0x47	/* wsm */
#define XRADIO_DBG_LEV_XR	0x47	/* xradio, fwio, sdio, bh */

#define ap_printk(level, ...)	\
	do {			\
		if ((level) & XRADIO_DBG_LEV_AP & XRADIO_DBG_ERROR)		\
			printk(KERN_ERR "xradio AP-ERR: " __VA_ARGS__);		\
		else if ((level) & XRADIO_DBG_LEV_AP & XRADIO_DBG_WARN)		\
			printk(KERN_WARNING "xradio AP-WRN: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_AP & XRADIO_DBG_NIY)		\
			printk(KERN_DEBUG "xradio AP-NIY: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_AP & XRADIO_DBG_MSG)		\
			printk(KERN_DEBUG "xradio AP-MSG: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_AP & XRADIO_DBG_OPS)		\
			printk(KERN_DEBUG "xradio AP-OPS: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_AP & XRADIO_DBG_DEV)		\
			printk(KERN_NOTICE "xradio AP-DEV: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_AP & XRADIO_DBG_ALWY)		\
			printk(KERN_INFO "xradio AP: " __VA_ARGS__); 		\
	} while (0)

#define pm_printk(level, ...)	\
	do {			\
		if ((level) & XRADIO_DBG_LEV_PM & XRADIO_DBG_ERROR)		\
			printk(KERN_ERR "xradio PM-ERR: " __VA_ARGS__);		\
		else if ((level) & XRADIO_DBG_LEV_PM & XRADIO_DBG_WARN)		\
			printk(KERN_WARNING "xradio PM-WRN: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_PM & XRADIO_DBG_NIY)		\
			printk(KERN_DEBUG "xradio PM-NIY: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_PM & XRADIO_DBG_MSG)		\
			printk(KERN_DEBUG "xradio PM-MSG: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_PM & XRADIO_DBG_OPS)		\
			printk(KERN_DEBUG "xradio PM-OPS: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_PM & XRADIO_DBG_DEV)		\
			printk(KERN_NOTICE "xradio PM-DEV: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_PM & XRADIO_DBG_ALWY)		\
			printk(KERN_INFO "xradio PM: " __VA_ARGS__); 		\
	} while (0)

#define scan_printk(level, ...)	\
	do {			\
		if ((level) & XRADIO_DBG_LEV_SCAN & XRADIO_DBG_ERROR)		\
			printk(KERN_ERR "xradio SCAN-ERR: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_SCAN & XRADIO_DBG_WARN)	\
			printk(KERN_WARNING "xradio SCAN-WRN: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_SCAN & XRADIO_DBG_NIY)	\
			printk(KERN_DEBUG "xradio SCAN-NIY: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_SCAN & XRADIO_DBG_MSG)	\
			printk(KERN_DEBUG "xradio SCAN-MSG: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_SCAN & XRADIO_DBG_OPS)	\
			printk(KERN_DEBUG "xradio SCAN-OPS: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_SCAN & XRADIO_DBG_DEV)	\
			printk(KERN_NOTICE "xradio SCAN-DEV: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_SCAN & XRADIO_DBG_ALWY)	\
			printk(KERN_INFO "xradio SCAN: " __VA_ARGS__); 		\
	} while (0)

#define sta_printk(level, ...)	\
	do {			\
		if ((level) & XRADIO_DBG_LEV_STA & XRADIO_DBG_ERROR)		\
			printk(KERN_ERR "xradio STA-ERR: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_STA & XRADIO_DBG_WARN)	\
			printk(KERN_WARNING "xradio STA-WRN: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_STA & XRADIO_DBG_NIY)		\
			printk(KERN_DEBUG "xradio STA-NIY: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_STA & XRADIO_DBG_MSG)		\
			printk(KERN_DEBUG "xradio STA-MSG: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_STA & XRADIO_DBG_OPS)		\
			printk(KERN_DEBUG "xradio STA-OPS: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_STA & XRADIO_DBG_DEV)		\
			printk(KERN_NOTICE "xradio STA-DEV: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_STA & XRADIO_DBG_ALWY)	\
			printk(KERN_INFO "xradio STA: " __VA_ARGS__); 		\
	} while (0)

#define txrx_printk(level, ...)	\
	do {			\
		if ((level) & XRADIO_DBG_LEV_TXRX & XRADIO_DBG_ERROR)		\
			printk(KERN_ERR "xradio TXRX-ERR: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_TXRX & XRADIO_DBG_WARN)	\
			printk(KERN_WARNING "xradio TXRX-WRN: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_TXRX & XRADIO_DBG_NIY)	\
			printk(KERN_DEBUG "xradio TXRX-NIY: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_TXRX & XRADIO_DBG_MSG)	\
			printk(KERN_DEBUG "xradio TXRX-MSG: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_TXRX & XRADIO_DBG_OPS)	\
			printk(KERN_DEBUG "xradio TXRX-OPS: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_TXRX & XRADIO_DBG_DEV)	\
			printk(KERN_NOTICE "xradio TXRX-DEV: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_TXRX & XRADIO_DBG_ALWY)	\
			printk(KERN_INFO "xradio TXRX: " __VA_ARGS__); 		\
	} while (0)

#define wsm_printk(level, ...)	\
	do {			\
		if ((level) & XRADIO_DBG_LEV_WSM & XRADIO_DBG_ERROR)		\
			printk(KERN_ERR "xradio WSM-ERR: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_WSM & XRADIO_DBG_WARN)	\
			printk(KERN_WARNING "xradio WSM-WRN: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_WSM & XRADIO_DBG_NIY)		\
			printk(KERN_DEBUG "xradio WSM-NIY: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_WSM & XRADIO_DBG_MSG)		\
			printk(KERN_DEBUG "xradio WSM-MSG: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_WSM & XRADIO_DBG_OPS)		\
			printk(KERN_DEBUG "xradio WSM-OPS: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_WSM & XRADIO_DBG_DEV)		\
			printk(KERN_NOTICE "xradio WSM-DEV: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_WSM & XRADIO_DBG_ALWY)	\
			printk(KERN_INFO "xradio WSM: " __VA_ARGS__); 		\
	} while (0)

#define xr_printk(level, ...)	\
	do {			\
		if ((level) & XRADIO_DBG_LEV_XR & XRADIO_DBG_ERROR)		\
			printk(KERN_ERR "xradio ERR: " __VA_ARGS__);		\
		else if ((level) & XRADIO_DBG_LEV_XR & XRADIO_DBG_WARN)		\
			printk(KERN_WARNING "xradio WRN: " __VA_ARGS__);	\
		else if ((level) & XRADIO_DBG_LEV_XR & XRADIO_DBG_NIY)		\
			printk(KERN_DEBUG "xradio NIY: " __VA_ARGS__);		\
		else if ((level) & XRADIO_DBG_LEV_XR & XRADIO_DBG_MSG)		\
			printk(KERN_DEBUG "xradio MSG: " __VA_ARGS__);		\
		else if ((level) & XRADIO_DBG_LEV_XR & XRADIO_DBG_OPS)		\
			printk(KERN_DEBUG "xradio OPS: " __VA_ARGS__);		\
		else if ((level) & XRADIO_DBG_LEV_XR & XRADIO_DBG_DEV)		\
			printk(KERN_NOTICE "xradio DEV: " __VA_ARGS__);		\
		else if ((level) & XRADIO_DBG_LEV_XR & XRADIO_DBG_ALWY)		\
			printk(KERN_INFO "xradio: " __VA_ARGS__); 		\
	} while (0)

#endif /* XRADIO_DEBUG_H_INCLUDED */

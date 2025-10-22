// SPDX-License-Identifier: GPL-2.0
/*
 * u_uac1.h - Utility definitions for UAC1 function
 *
 * Copyright (C) 2016 Ruslan Bilovol <ruslan.bilovol@gmail.com>
 */

#ifndef __U_UAC1_H
#define __U_UAC1_H

#include <linux/usb/composite.h>

#define UAC1_OUT_EP_MAX_PACKET_SIZE	200
#define UAC1_DEF_CCHMASK	0x3
#define UAC1_DEF_CSRATE		48000
#define UAC1_DEF_CSSIZE		2
#define UAC1_DEF_PCHMASK	0x3
#define UAC1_DEF_PSRATE		48000
#define UAC1_DEF_PSSIZE		2
#define UAC1_DEF_REQ_NUM	2


struct f_uac1_opts {
	struct usb_function_instance	func_inst;
	unsigned int			c_chmask;
	unsigned int			c_srate;
	unsigned int			c_ssize;
	unsigned int			p_chmask;
	unsigned int			p_srate;
	unsigned int			p_ssize;
	unsigned int			req_number;
	unsigned			bound:1;

	struct mutex			lock;
	int				refcnt;
};

#endif /* __U_UAC1_H */

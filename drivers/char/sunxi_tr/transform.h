//*********************************************************************************************************************
//  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
//
//  File name   :	de_tr.h
//
//  Description :	display engine 2.0 rotation processing base functions implement
//
//  History     :	2014/04/08  iptang  v0.1  Initial version
//
//*********************************************************************************************************************

#ifndef __DE_TR_H__
#define __DE_TR_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include "asm-generic/int-ll64.h"
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <mach/irqs.h>
#include <mach/platform.h>
#include <linux/sync.h>
#include <linux/sw_sync.h>
#include <asm/div64.h>
#include <linux/debugfs.h>
#include <linux/sunxi_tr.h>

int de_tr_set_base(unsigned int reg_base);
int de_tr_irq_enable(void);
int de_tr_irq_query(void);
int de_tr_init(void);
int de_tr_exit(void);
int de_tr_set_cfg(tr_info *info);
int de_tr_reset(void);
int de_tr_exception(void);

int sunxi_tr_request(void);
int sunxi_tr_release(int hdl);
int sunxi_tr_commit(int hdl, tr_info *info);
int sunxi_tr_query(int hdl);
int sunxi_tr_set_timeout(int hdl, unsigned long timeout /* ms */);

#endif

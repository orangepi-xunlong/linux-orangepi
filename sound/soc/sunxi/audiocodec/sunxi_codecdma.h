/*
 * sound\soc\sunxi\audiocodec\sunxi-codecdma.h
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef SUNXI_PCMDMA_H_
#define SUNXI_PCMDMA_H_

#undef AUDIOCODEC_DBG
#if (0)
    #define AUDIOCODEC_DBG(format,args...)  printk("[SWITCH] "format,##args)    
#else
    #define AUDIOCODEC_DBG(...)    
#endif

struct sunxi_dma_params {
	char *name;		
	dma_addr_t dma_addr;
};

#endif

/*
 * sound\soc\sunxi\i2s0\sunxi-i2s0dma.h
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

#ifndef SUNXI_I2S0DMA_H_
#define SUNXI_I2S0DMA_H_

#undef I2S0_DBG
#if (0)
    #define I2S0_DBG(format,args...)  printk("[I2S0] "format,##args)    
#else
    #define I2S0_DBG(...)    
#endif

struct sunxi_dma_params {
	char *name;		
	dma_addr_t dma_addr;
};

#endif

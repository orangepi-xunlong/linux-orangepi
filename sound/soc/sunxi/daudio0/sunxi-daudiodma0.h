/*
 * sound\soc\sunxi\daudio\sunxi-daudiodma.h
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

#ifndef SUNXI_DAUDIODMA_H_
#define SUNXI_DAUDIODMA_H_

#undef DAUDIO_DBG
#if (0)
    #define DAUDIO_DBG(format,args...)  printk("[DAUDIO] "format,##args)    
#else
    #define DAUDIO_DBG(...)    
#endif
#ifndef SUNXI_DMA_PARAM
#define SUNXI_DMA_PARAM
struct sunxi_dma_params {
	char *name;		
	dma_addr_t dma_addr;
};
#endif

#endif

/*
 * AHCI SATA platform driver
 *
 * Copyright 2004-2005  Red Hat, Inc.
 *   Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2010  MontaVista Software, LLC.
 *   Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef _SATA_SUNXI_H_
#define _SATA_SUNXI_H_

#include <mach/platform.h>
#include <mach/irqs.h>

#define	ahci_readb(base,offset)  		(*((volatile unsigned char*)((base)+(offset))))
#define	ahci_readw(base,offset)  		(*((volatile unsigned short*)((base)+(offset))))
#define	ahci_readl(base,offset)  		(*((volatile unsigned int*)((base)+(offset))))
#define ahci_writeb(base,offset,val)	(*((volatile unsigned char*)((base)+(offset))) = (val))
#define ahci_writew(base,offset,val)	(*((volatile unsigned short*)((base)+(offset))) = (val))
#define ahci_writel(base,offset,val)	(*((volatile unsigned int*)((base)+(offset))) = (val))

#define AHCI_REG_BISTAFR		0x00A0
#define AHCI_REG_BISTCR			0x00A4
#define AHCI_REG_BISTFCTR		0x00A8
#define AHCI_REG_BISTSR			0x00AC
#define AHCI_REG_BISTDECR		0x00B0
#define AHCI_REG_DIAGNR			0x00B4
#define AHCI_REG_DIAGNR1		0x00B8
#define AHCI_REG_OOBR			0x00BC
#define AHCI_REG_PHYCS0R		0x00C0
#define AHCI_REG_PHYCS1R		0x00C4
#define AHCI_REG_PHYCS2R		0x00C8
#define AHCI_REG_TIMER1MS		0x00E0
#define AHCI_REG_GPARAM1R		0x00E8
#define AHCI_REG_GPARAM2R		0x00EC
#define AHCI_REG_PPARAMR		0x00F0
#define AHCI_REG_TESTR			0x00F4
#define AHCI_REG_VERSIONR		0x00F8
#define AHCI_REG_IDR			0x00FC
#define AHCI_REG_RWCR			0x00FC

#define AHCI_REG_P0DMACR		0x0170
#define AHCI_REG_P0PHYCR		0x0178
#define AHCI_REG_P0PHYSR		0x017C

#define SUNXI_AHCI_ACCESS_LOCK(base,x)	(*((volatile unsigned int *)((base)+AHCI_REG_RWCR)) = (x))

#define SUNXI_AHCI_DEV_NAME		"sata"

#endif /* end of _SATA_SUNXI_H_ */


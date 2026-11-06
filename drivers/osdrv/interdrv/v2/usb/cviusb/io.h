#ifndef __DRIVERS_USB_CVI_IO_H
#define __DRIVERS_USB_CVI_IO_H

#include <linux/io.h>

static inline u32 cviusb_readl(uint32_t __iomem *reg)
{
	u32 value = 0;

	value = readl(reg);
	return value;
}

static inline void cviusb_writel(uint32_t __iomem *reg, u32 value)
{
	writel(value, reg);
}


#endif /* __DRIVERS_USB_CVI_IO_H */

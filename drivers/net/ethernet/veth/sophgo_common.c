#include <linux/types.h>
#include <linux/io.h>
#include "sophgo_common.h"

void sg_write32(void __iomem *base, u32 offset, u32 value)
{
	iowrite32(value, (void __iomem *)(((unsigned long)base) + offset));
}

u32 sg_read32(void __iomem *base, u32 offset)
{
	return ioread32((void __iomem *)(((unsigned long)base) + offset));
}

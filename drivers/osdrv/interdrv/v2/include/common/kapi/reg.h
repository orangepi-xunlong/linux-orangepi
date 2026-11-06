#ifndef _REG_H_
#define _REG_H_

#include <linux/io.h>

#define _reg_read(addr) readl((void __iomem *)addr)
#define _reg_write(addr, data) writel(data, (void __iomem *)addr)
void _reg_write_mask(uintptr_t addr, u32 mask, u32 data);

#endif //_REG_H_

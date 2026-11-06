#ifndef ACCESS_H_
#define ACCESS_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

#define ADDR_JUMP 4

#define _reg_read(addr) readl((void __iomem *)addr)
#define _reg_write(addr, data) writel(data, (void __iomem *)addr)

typedef unsigned long int uintptr_t;

struct device_access {
	char name[30];

	int (*initialize) (void);
	int (*disable) (void);

	void (*write) (u32 addr, u32 data);
	u32  (*read) (u32 addr);
};


u8 dev_read(u32 addr);
u8 dev_read_mask(u32 addr, u32 mask);
void dev_write(u32 addr, u32 data);
void dev_write_mask(u32 addr, u32 mask, u32 data);
void hdmi_set_reg_base(void* base);

#endif /* ACCESS_H_ */

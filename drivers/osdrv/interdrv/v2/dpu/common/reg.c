#ifdef ENV_CVITEST
#include <common.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "system_common.h"
#include "timer.h"
#elif defined(ENV_EMU)
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "emu/command.h"
#endif  // ENV_CVITEST

#include "reg.h"
#include <linux/spinlock.h>

#ifdef ENV_CVITEST
void _reg_write_mask(unsigned long long addr, unsigned int mask, unsigned int data)
{
	unsigned int value;

	value = _reg_read(addr) & ~mask;
	value |= (data & mask);
	_reg_write(addr, value);
}
#elif defined(ENV_EMU)
static unsigned int regs_dwa[0x100];
static unsigned int regs_cmdq[0x30];
static unsigned int regs_vip[0x10];
static unsigned int regs_scl[0x10000];

unsigned int _reg_read(unsigned long long addr)
{
	if (addr > REG_VIP_SYS_BASE)
		return regs_vip[(addr - REG_VIP_SYS_BASE) >> 2];
	if (addr > REG_DWA_CMDQ_BASE)
		return regs_cmdq[(addr - REG_DWA_CMDQ_BASE) >> 2];
	if (addr > REG_DWA_BASE)
		return regs_dwa[(addr - REG_DWA_BASE) >> 2];
	if (addr > REG_SCL_TOP_BASE)
		return regs_scl[(addr - REG_SCL_TOP_BASE) >> 2];

	return 0;
}

void _reg_write(unsigned long long addr, unsigned int data)
{
	if (addr > REG_VIP_SYS_BASE)
		regs_vip[(addr - REG_VIP_SYS_BASE) >> 2] = data;
	else if (addr > REG_DWA_CMDQ_BASE)
		regs_cmdq[(addr - REG_DWA_CMDQ_BASE) >> 2] = data;
	else if (addr > REG_DWA_BASE)
		regs_dwa[(addr - REG_DWA_BASE) >> 2] = data;
	else if (addr > REG_SCL_TOP_BASE)
		regs_scl[(addr - REG_SCL_TOP_BASE) >> 2] = data;
}

void _reg_write_mask(unsigned long long addr, unsigned int mask, unsigned int data)
{
	unsigned int value;

	value = _reg_read(addr) & ~mask;
	value |= (data & mask);
	_reg_write(addr, value);
}
#else
static DEFINE_RAW_SPINLOCK(__io_lock);

void reg_write_mask(unsigned long long addr, unsigned int mask, unsigned int data)
{
	unsigned long flags;
	unsigned int value;

	raw_spin_lock_irqsave(&__io_lock, flags);
	value = readl_relaxed((void __iomem *)addr) & ~mask;
	value |= (data & mask);
	writel(value, (void __iomem *)addr);
	raw_spin_unlock_irqrestore(&__io_lock, flags);
}

unsigned int reg_read_mask(unsigned long long addr, unsigned int mask,unsigned int shift)
{
	unsigned long flags;
	unsigned int value;
	raw_spin_lock_irqsave(&__io_lock, flags);
	value= ((readl_relaxed((void __iomem *)addr) & mask) >> shift);
	raw_spin_unlock_irqrestore(&__io_lock, flags);
	return value;
}

void write_reg(unsigned long long addr,unsigned int data)
{
	unsigned long flags;
	raw_spin_lock_irqsave(&__io_lock, flags);
	writel(data, (void __iomem *)addr);
	raw_spin_unlock_irqrestore(&__io_lock, flags);
}

unsigned int read_reg(unsigned long long  addr)
{
	unsigned long flags;
	unsigned int value;
	raw_spin_lock_irqsave(&__io_lock, flags);
    value =readl_relaxed((void __iomem *)addr);
	raw_spin_unlock_irqrestore(&__io_lock, flags);
	return value;
};
#endif

#ifndef _REG_H_
#define _REG_H_

#ifdef ENV_CVITEST
#define _reg_read(addr) readl((unsigned long long )addr)
#define _reg_write(addr, data) writel((unsigned long long )addr, data)
void _reg_write_mask(unsigned long long addr, unsigned int mask, unsigned int data);

#elif defined(ENV_EMU)
unsigned int _reg_read(unsigned long long addr);
void _reg_write(unsigned long long addr, unsigned int data);
void _reg_write_mask(unsigned long long addr, unsigned int mask, unsigned int data);

#else
#include <linux/io.h>
#include <linux/comm_dpu.h>
extern int dump_reg;

void reg_write_mask(unsigned long long addr, unsigned int mask, unsigned int data);
unsigned int reg_read_mask(unsigned long long addr, unsigned int mask,unsigned int shift);
unsigned int read_reg(unsigned long long addr);
void write_reg(unsigned long long addr,unsigned int data);
#endif

#endif //_REG_H_
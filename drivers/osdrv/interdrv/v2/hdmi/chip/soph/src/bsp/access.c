#include "access.h"
#include "core/hdmitx_dev.h"
#include "util/util.h"

static uintptr_t hdmi_reg_base;

void hdmi_set_reg_base(void* base)
{
	hdmi_reg_base = (uintptr_t)base;
}

u8 dev_read(u32 addr)
{
	u8 value;
	value = _reg_read(hdmi_reg_base + addr);
	return value;
}

u8 dev_read_mask(u32 addr, u32 mask)
{
	u32 shift = first_bit_set(mask);
	return ((dev_read(addr) & mask) >> shift);
}

void dev_write(u32 addr, u32 data)
{
	_reg_write(hdmi_reg_base + addr, data);
}

void dev_write_mask(u32 addr, u32 mask, u32 data)
{
	u32 temp = 0;
	u32 shift = first_bit_set(mask);

	temp = dev_read(addr);
	temp &= ~(mask);
	temp |= (mask & data << shift);
	dev_write(addr, temp);
}

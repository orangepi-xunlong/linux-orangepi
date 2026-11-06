#include <linux/types.h>
#include <linux/delay.h>

#include "vo_sys.h"
#include "reg.h"

/* vo_sys */
static uintptr_t vo_sys_reg_base;
static struct spinlock vo_sys_lock;

void vo_sys_set_base_addr(void *base)
{
	vo_sys_reg_base = (uintptr_t)base;
	spin_lock_init(&vo_sys_lock);
}
EXPORT_SYMBOL_GPL(vo_sys_set_base_addr);

// clk_lp
union vo_sys_clk_lp vo_sys_get_clk_lp(void)
{
	union vo_sys_clk_lp clk;

	clk.raw = _reg_read(vo_sys_reg_base + VO_SYS_VO_CLK_LP);
	return clk;
}
EXPORT_SYMBOL_GPL(vo_sys_get_clk_lp);

void vo_sys_set_clk_lp(union vo_sys_clk_lp clk)
{
	_reg_write(vo_sys_reg_base + VO_SYS_VO_CLK_LP, clk.raw);
}
EXPORT_SYMBOL_GPL(vo_sys_set_clk_lp);

// clk_ctrl0
union vo_sys_clk_ctrl0 vo_sys_get_clk_ctrl0(void)
{
	union vo_sys_clk_ctrl0 cfg;

	cfg.raw = _reg_read(vo_sys_reg_base + VO_SYS_VO_CLK_CTRL0);
	return cfg;
}
EXPORT_SYMBOL_GPL(vo_sys_get_clk_ctrl0);

void vo_sys_set_clk_ctrl0(union vo_sys_clk_ctrl0 cfg)
{
	_reg_write(vo_sys_reg_base + VO_SYS_VO_CLK_CTRL0, cfg.raw);
}
EXPORT_SYMBOL_GPL(vo_sys_set_clk_ctrl0);

// clk_ctrl1
union vo_sys_clk_ctrl1 vo_sys_get_clk_ctrl1(void)
{
	union vo_sys_clk_ctrl1 cfg;

	cfg.raw = _reg_read(vo_sys_reg_base + VO_SYS_VO_CLK_CTRL1);
	return cfg;
}
EXPORT_SYMBOL_GPL(vo_sys_get_clk_ctrl1);

void vo_sys_set_clk_ctrl1(union vo_sys_clk_ctrl1 cfg)
{
	_reg_write(vo_sys_reg_base + VO_SYS_VO_CLK_CTRL1, cfg.raw);
}
EXPORT_SYMBOL_GPL(vo_sys_set_clk_ctrl1);

// reset
union vo_sys_reset vo_sys_get_reset(void)
{
	union vo_sys_reset reset;

	reset.raw = _reg_read(vo_sys_reg_base + VO_SYS_VO_IP_RESETS);
	return reset;
}
EXPORT_SYMBOL_GPL(vo_sys_get_reset);

void vo_sys_set_reset(union vo_sys_reset reset)
{
	_reg_write(vo_sys_reg_base + VO_SYS_VO_IP_RESETS, reset.raw);
}
EXPORT_SYMBOL_GPL(vo_sys_set_reset);

/**
 * vo_sys_toggle_reset - enable/disable reset specified in mask. Lock protected.
 *
 * @param mask: resets want to be toggled.
 */
void vo_sys_toggle_reset(union vo_sys_reset mask)
{
	union vo_sys_reset value;
	unsigned long flags;

	spin_lock_irqsave(&vo_sys_lock, flags);
	value = vo_sys_get_reset();
	value.raw |= mask.raw;
	vo_sys_set_reset(value);

	udelay(20);
	value.raw &= ~mask.raw;
	vo_sys_set_reset(value);
	spin_unlock_irqrestore(&vo_sys_lock, flags);
}
EXPORT_SYMBOL_GPL(vo_sys_toggle_reset);

//reset apb
union vo_sys_reset_apb vo_sys_get_reset_apb(void)
{
	union vo_sys_reset_apb reset;

	reset.raw = _reg_read(vo_sys_reg_base + VO_SYS_VO_APB_RESETS);
	return reset;
}
EXPORT_SYMBOL_GPL(vo_sys_get_reset_apb);

void vo_sys_set_reset_apb(union vo_sys_reset_apb reset)
{
	_reg_write(vo_sys_reg_base + VO_SYS_VO_APB_RESETS, reset.raw);
}
EXPORT_SYMBOL_GPL(vo_sys_set_reset_apb);

void vo_sys_toggle_reset_apb(union vo_sys_reset_apb mask)
{
	union vo_sys_reset_apb value;
	unsigned long flags;

	spin_lock_irqsave(&vo_sys_lock, flags);
	value = vo_sys_get_reset_apb();
	value.raw |= mask.raw;
	vo_sys_set_reset_apb(value);

	udelay(20);
	value.raw &= ~mask.raw;
	vo_sys_set_reset_apb(value);
	spin_unlock_irqrestore(&vo_sys_lock, flags);
}
EXPORT_SYMBOL_GPL(vo_sys_toggle_reset_apb);

// interrupt
union vo_sys_intr vo_sys_get_intr_status(void)
{
	union vo_sys_intr intr;

	intr.raw = _reg_read(vo_sys_reg_base + VO_SYS_VO_INT);
	return intr;
}
EXPORT_SYMBOL_GPL(vo_sys_get_intr_status);

void vo_set_intr_mask(union vo_sys_intr mask)
{
	union vo_sys_intr value;
	unsigned long flags;

	spin_lock_irqsave(&vo_sys_lock, flags);
	value = vo_sys_get_intr_status();
	value.raw |= mask.raw;
	_reg_write(vo_sys_reg_base + VO_SYS_VO_INT, value.raw);
	spin_unlock_irqrestore(&vo_sys_lock, flags);
}
EXPORT_SYMBOL_GPL(vo_set_intr_mask);

// r/w register
unsigned int vo_sys_reg_read(uintptr_t addr)
{
	return _reg_read(vo_sys_reg_base + addr);
}
EXPORT_SYMBOL_GPL(vo_sys_reg_read);

void vo_sys_reg_write_mask(uintptr_t addr, u32 mask, u32 data)
{
	_reg_write_mask(vo_sys_reg_base + addr, mask, data);
}
EXPORT_SYMBOL_GPL(vo_sys_reg_write_mask);

/**
 * vo_sys_set_offline - control vo_sys axi channel attribute, realtime/offline.
 *
 * @param bus: axi bus to control.
 * @param offline: true: offline; false: realtime.
 */
void vo_sys_set_offline(enum vo_sys_axi_bus bus, bool offline)
{
	u32 mask = BIT(bus);
	u32 value = (offline) ? ~mask : mask;

	_reg_write_mask(vo_sys_reg_base + VO_SYS_VO_AXI_SW, mask, value);
}
EXPORT_SYMBOL_GPL(vo_sys_set_offline);

void extern_axi_to_36bit(u32 high_bit)
{
	u32 axi_ext_ow = 0;
	u32 axi_ext1 = 0;

	//extern to 36 bit
	axi_ext_ow = _reg_read(vo_sys_reg_base + VO_SYS_VO_AXI_ADDR_EXT_OW);
	_reg_write(vo_sys_reg_base + VO_SYS_VO_AXI_ADDR_EXT_OW, axi_ext_ow | 0x30000000);
	axi_ext1 = _reg_read(vo_sys_reg_base + VO_SYS_VO_AXI_ADDR_EXT1);
	_reg_write(vo_sys_reg_base + VO_SYS_VO_AXI_ADDR_EXT1, axi_ext1 | ((high_bit << 16) | (high_bit << 20)));
}
EXPORT_SYMBOL_GPL(extern_axi_to_36bit);

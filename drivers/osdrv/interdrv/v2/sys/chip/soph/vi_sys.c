#include <linux/types.h>
#include <linux/delay.h>

#include "vi_sys.h"
#include "reg.h"

/* sensor cmm extern function. */
enum vi_sys_cmm {
	VI_CMM_I2C = 0,
	VI_CMM_SSP,
	VI_CMM_BUTT,
};

struct vi_sys_cmm_ops {
	long (*cb)(void *hdlr, unsigned int cmd, void *arg);
};

struct vi_sys_cmm_dev {
	enum vi_sys_cmm		cmm_type;
	void				*hdlr;
	struct vi_sys_cmm_ops		ops;
};


static struct vi_sys_cmm_dev cmm_ssp;
static struct vi_sys_cmm_dev cmm_i2c;

static uintptr_t vi_sys_reg_base;
static struct spinlock vi_sys_lock;

int vi_sys_register_cmm_cb(unsigned long cmm, void *hdlr, void *cb)
{
	struct vi_sys_cmm_dev *cmm_dev;

	if ((cmm >= VI_CMM_BUTT) || !hdlr || !cb)
		return -1;

	cmm_dev = (cmm == VI_CMM_I2C) ? &cmm_i2c : &cmm_ssp;

	cmm_dev->cmm_type = cmm;
	cmm_dev->hdlr = hdlr;
	cmm_dev->ops.cb = cb;

	return 0;
}
EXPORT_SYMBOL_GPL(vi_sys_register_cmm_cb);

int vi_sys_cmm_cb_i2c(unsigned int cmd, void *arg)
{
	struct vi_sys_cmm_dev *cmm_dev = &cmm_i2c;

	if (cmm_dev->cmm_type != VI_CMM_I2C)
		return -1;

	return (cmm_dev->ops.cb) ? cmm_dev->ops.cb(cmm_dev->hdlr, cmd, arg) : (-1);
}
EXPORT_SYMBOL_GPL(vi_sys_cmm_cb_i2c);

int vi_sys_cmm_cb_ssp(unsigned int cmd, void *arg)
{
	struct vi_sys_cmm_dev *cmm_dev = &cmm_ssp;

	if (cmm_dev->cmm_type != VI_CMM_SSP)
		return -1;

	return (cmm_dev->ops.cb) ? cmm_dev->ops.cb(cmm_dev->hdlr, cmd, arg) : (-1);
}
EXPORT_SYMBOL_GPL(vi_sys_cmm_cb_ssp);


void vi_sys_set_base_addr(void *base)
{
	vi_sys_reg_base = (uintptr_t)base;
	spin_lock_init(&vi_sys_lock);
}
EXPORT_SYMBOL_GPL(vi_sys_set_base_addr);

// clk_lp0
union vi_sys_clk_lp0 vi_sys_get_clk_lp0(void)
{
	union vi_sys_clk_lp0 clk;

	clk.raw = _reg_read(vi_sys_reg_base + VI_SYS_VI_CLK_LP0);
	return clk;
}
EXPORT_SYMBOL_GPL(vi_sys_get_clk_lp0);

void vi_sys_set_clk_lp0(union vi_sys_clk_lp0 clk)
{
	_reg_write(vi_sys_reg_base + VI_SYS_VI_CLK_LP0, clk.raw);
}
EXPORT_SYMBOL_GPL(vi_sys_set_clk_lp0);

// clk_lp1
union vi_sys_clk_lp1 vi_sys_get_clk_lp1(void)
{
	union vi_sys_clk_lp1 clk;

	clk.raw = _reg_read(vi_sys_reg_base + VI_SYS_VI_CLK_LP1);
	return clk;
}
EXPORT_SYMBOL_GPL(vi_sys_get_clk_lp1);

void vi_sys_set_clk_lp1(union vi_sys_clk_lp1 clk)
{
	_reg_write(vi_sys_reg_base + VI_SYS_VI_CLK_LP1, clk.raw);
}
EXPORT_SYMBOL_GPL(vi_sys_set_clk_lp1);

// clk_lp2
union vi_sys_clk_lp2 vi_sys_get_clk_lp2(void)
{
	union vi_sys_clk_lp2 clk;

	clk.raw = _reg_read(vi_sys_reg_base + VI_SYS_VI_CLK_LP2);
	return clk;
}
EXPORT_SYMBOL_GPL(vi_sys_get_clk_lp2);

void vi_sys_set_clk_lp2(union vi_sys_clk_lp2 clk)
{
	_reg_write(vi_sys_reg_base + VI_SYS_VI_CLK_LP2, clk.raw);
}
EXPORT_SYMBOL_GPL(vi_sys_set_clk_lp2);

// clk_ctrl0
union vi_sys_clk_ctrl0 vi_sys_get_clk_ctrl0(void)
{
	union vi_sys_clk_ctrl0 cfg;

	cfg.raw = _reg_read(vi_sys_reg_base + VI_SYS_VI_CLK_CTRL0);
	return cfg;
}
EXPORT_SYMBOL_GPL(vi_sys_get_clk_ctrl0);

void vi_sys_set_clk_ctrl0(union vi_sys_clk_ctrl0 cfg)
{
	_reg_write(vi_sys_reg_base + VI_SYS_VI_CLK_CTRL0, cfg.raw);
}
EXPORT_SYMBOL_GPL(vi_sys_set_clk_ctrl0);

// clk_ctrl1
union vi_sys_clk_ctrl1 vi_sys_get_clk_ctrl1(void)
{
	union vi_sys_clk_ctrl1 cfg;

	cfg.raw = _reg_read(vi_sys_reg_base + VI_SYS_VI_CLK_CTRL1);
	return cfg;
}
EXPORT_SYMBOL_GPL(vi_sys_get_clk_ctrl1);

void vi_sys_set_clk_ctrl1(union vi_sys_clk_ctrl1 cfg)
{
	_reg_write(vi_sys_reg_base + VI_SYS_VI_CLK_CTRL1, cfg.raw);
}
EXPORT_SYMBOL_GPL(vi_sys_set_clk_ctrl1);

// clk_ctrl2
union vi_sys_clk_ctrl2 vi_sys_get_clk_ctrl2(void)
{
	union vi_sys_clk_ctrl2 cfg;

	cfg.raw = _reg_read(vi_sys_reg_base + VI_SYS_VI_CLK_CTRL2);
	return cfg;
}
EXPORT_SYMBOL_GPL(vi_sys_get_clk_ctrl2);

void vi_sys_set_clk_ctrl2(union vi_sys_clk_ctrl2 cfg)
{
	_reg_write(vi_sys_reg_base + VI_SYS_VI_CLK_CTRL2, cfg.raw);
}
EXPORT_SYMBOL_GPL(vi_sys_set_clk_ctrl2);

// reset
union vi_sys_reset vi_sys_get_reset(void)
{
	union vi_sys_reset reset;

	reset.raw = _reg_read(vi_sys_reg_base + VI_SYS_VI_RESETS);
	return reset;
}
EXPORT_SYMBOL_GPL(vi_sys_get_reset);

void vi_sys_set_reset(union vi_sys_reset reset)
{
	_reg_write(vi_sys_reg_base + VI_SYS_VI_RESETS, reset.raw);
}
EXPORT_SYMBOL_GPL(vi_sys_set_reset);

/**
 * vi_sys_toggle_reset - enable/disable reset specified in mask. Lock protected.
 *
 * @param mask: resets want to be toggled.
 */
void vi_sys_toggle_reset(union vi_sys_reset mask)
{
	union vi_sys_reset value;
	unsigned long flags;

	spin_lock_irqsave(&vi_sys_lock, flags);
	value = vi_sys_get_reset();
	value.raw |= mask.raw;
	vi_sys_set_reset(value);

	udelay(20);
	value.raw &= ~mask.raw;
	vi_sys_set_reset(value);
	spin_unlock_irqrestore(&vi_sys_lock, flags);
}
EXPORT_SYMBOL_GPL(vi_sys_toggle_reset);

//reset apb
union vi_sys_reset_apb vi_sys_get_reset_apb(void)
{
	union vi_sys_reset_apb reset;

	reset.raw = _reg_read(vi_sys_reg_base + VI_SYS_VI_RESETS_APB);
	return reset;
}
EXPORT_SYMBOL_GPL(vi_sys_get_reset_apb);

void vi_sys_set_reset_apb(union vi_sys_reset_apb reset)
{
	_reg_write(vi_sys_reg_base + VI_SYS_VI_RESETS_APB, reset.raw);
}
EXPORT_SYMBOL_GPL(vi_sys_set_reset_apb);

void vi_sys_toggle_reset_apb(union vi_sys_reset_apb mask)
{
	union vi_sys_reset_apb value;
	unsigned long flags;

	spin_lock_irqsave(&vi_sys_lock, flags);
	value = vi_sys_get_reset_apb();
	value.raw |= mask.raw;
	vi_sys_set_reset_apb(value);

	udelay(20);
	value.raw &= ~mask.raw;
	vi_sys_set_reset_apb(value);
	spin_unlock_irqrestore(&vi_sys_lock, flags);
}
EXPORT_SYMBOL_GPL(vi_sys_toggle_reset_apb);

// interrupt
union vi_sys_intr vi_sys_get_intr_status(void)
{
	union vi_sys_intr intr;

	intr.raw = _reg_read(vi_sys_reg_base + VI_SYS_VI_INT);
	return intr;
}
EXPORT_SYMBOL_GPL(vi_sys_get_intr_status);

void vi_set_intr_mask(union vi_sys_intr mask)
{
	union vi_sys_intr value;
	unsigned long flags;

	spin_lock_irqsave(&vi_sys_lock, flags);
	value = vi_sys_get_intr_status();
	value.raw |= mask.raw;
	_reg_write(vi_sys_reg_base + VI_SYS_VI_INT, value.raw);
	spin_unlock_irqrestore(&vi_sys_lock, flags);
}
EXPORT_SYMBOL_GPL(vi_set_intr_mask);

// r/w register
unsigned int vi_sys_reg_read(uintptr_t addr)
{
	return _reg_read(vi_sys_reg_base + addr);
}
EXPORT_SYMBOL_GPL(vi_sys_reg_read);

void vi_sys_reg_write_mask(uintptr_t addr, u32 mask, u32 data)
{
	_reg_write_mask(vi_sys_reg_base + addr, mask, data);
}
EXPORT_SYMBOL_GPL(vi_sys_reg_write_mask);

/**
 * vi_sys_set_offline - control vi_sys axi channel attribute, realtime/offline.
 *
 * @param bus: axi bus to control.
 * @param offline: true: offline; false: realtime.
 */
void vi_sys_set_offline(enum vi_sys_axi_bus bus, bool offline)
{
	u32 mask = BIT(bus);
	u32 value = (offline) ? mask : ~mask;

	_reg_write_mask(vi_sys_reg_base + VI_SYS_VI_AXI_SW, mask, value);
}
EXPORT_SYMBOL_GPL(vi_sys_set_offline);




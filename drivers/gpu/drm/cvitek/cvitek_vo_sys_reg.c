#include <drm/drm_print.h>
#include "cvitek_vo_sys_reg.h"

void __iomem  *vo_sys_f_base;
void __iomem  *vo_sys_b_base;
void __iomem  *top_pll_base;

static DEFINE_RAW_SPINLOCK(__io_lock);
u32 _reg_read(void __iomem *addr)
{
	unsigned long flags;
	u32 value;

	raw_spin_lock_irqsave(&__io_lock, flags);
	value = readl_relaxed(addr);
	raw_spin_unlock_irqrestore(&__io_lock, flags);
	return value;
}

void _reg_write(void __iomem *addr, u32 value)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&__io_lock, flags);
	writel_relaxed(value, addr);
	raw_spin_unlock_irqrestore(&__io_lock, flags);
}

void _reg_write_mask(void __iomem *addr, u32 mask, u32 data)
{
	unsigned long flags;
	u32 value;

	raw_spin_lock_irqsave(&__io_lock, flags);
	value = readl_relaxed(addr) & ~mask;
	value |= (data & mask);
	writel(value, addr);
	raw_spin_unlock_irqrestore(&__io_lock, flags);
}

void extend_axi_to_36bit(u32 high_bit, enum drm_intf intf)
{
	switch (intf) {
	case DRM_INTF_DISP0:
		_reg_write_mask(REG_VO_SYS_AXI_ADDR_EXT_OW, 0xff000, 0xff000);
		_reg_write(REG_VO_SYS_AXI_ADDR_EXT2, (high_bit) | (high_bit << 4) | (high_bit << 8) | (high_bit << 12) |
									     (high_bit << 16) | (high_bit << 20) | (high_bit << 24) | (high_bit << 28));
		break;
	case DRM_INTF_DISP1:
		_reg_write_mask(REG_VO_SYS_AXI_ADDR_EXT_OW, 0xff00000, 0xff00000);
		_reg_write(REG_VO_SYS_AXI_ADDR_EXT3, (high_bit) | (high_bit << 4) | (high_bit << 8) | (high_bit << 12) |
									     (high_bit << 16) | (high_bit << 20) | (high_bit << 24) | (high_bit << 28));
		break;
	default:
		DRM_ERROR("drm intf %d is not support\n", intf);
		break;
	}
}

void reset_disp(void)
{
	/*ip and apb reset*/
	_reg_write_mask(REG_VO_SYS_IP_RESET, 0xC0, 0xC0);
	_reg_write_mask(REG_VO_SYS_APB_RESET, 0xC0, 0xC0);
	udelay(10);
	_reg_write_mask(REG_VO_SYS_IP_RESET, 0xC0, 0x00);
	_reg_write_mask(REG_VO_SYS_APB_RESET, 0xC0, 0x00);
}

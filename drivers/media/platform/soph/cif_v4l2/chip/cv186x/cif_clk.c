#include "cif.h"
//#include "drv/cif_drv.h"

#define MAC0_CLK_CTRL1_OFFSET		4U
#define MAC1_CLK_CTRL1_OFFSET		8U
#define MAC2_CLK_CTRL1_OFFSET		30U
#define MAC_CLK_CTRL1_MASK		0x3U

extern int mclk0;
extern int mclk1;
extern int mclk2;
extern int mclk3;
extern int mclk4;
extern int mclk5;
extern int mclk6;
extern int lane_phase[LANE_SKEW_NUM];
extern int bypass_mac_clk;
extern unsigned int max_mac_clk;

/**********************************************/
/**************set CSI MAC CLK*****************/
/**********************************************/
/*
 * 1. Precondition: the mac_max = 400M by default, can be adjust by ioctl (app).
 * 2. If mac_max is below 400M, the vip_sys_2 is 400M (parent: mipimpll).
 * 3. If mac_max = 500M, the vip_sys_2 is 500M. (parent: fpll, 1835 OD case)
 * 4. If mac_max = 600M, the vip_sys_2 is 600M. (parent: mipimpll 1838 case)
 * 5. Return error in the others.
 */
static inline unsigned int _cif_mac_enum_to_value(enum rx_mac_clk_e mac_clk)
{
	unsigned int val;

	switch (mac_clk) {
	case RX_MAC_CLK_150M:
		val = 150;
		break;
	case RX_MAC_CLK_200M:
		val = 200;
		break;
	case RX_MAC_CLK_300M:
		val = 300;
		break;
	case RX_MAC_CLK_500M:
		val = 500;
		break;
	case RX_MAC_CLK_600M:
		val = 600;
		break;
	case RX_MAC_CLK_900M:
		val = 900;
		break;
	case RX_MAC_CLK_400M:
	default:
		val = 400;
		break;
	}
	return val;
}

// static DEFINE_RAW_SPINLOCK(__io_lock);
void cif_ops_reg_write_mask(uintptr_t addr, u32 mask, u32 data)
{
	// unsigned long flags;
	u32 value;
	void __iomem *register_addr;

	// raw_spin_lock_irqsave(&__io_lock, flags);
	register_addr = ioremap(addr, sizeof(u32));
	value = readl_relaxed(register_addr) & ~mask;
	value |= (data & mask);
	writel(value, register_addr);
	iounmap(register_addr);
	// raw_spin_unlock_irqrestore(&__io_lock, flags);
}

int _cif_set_mac_clk(struct cvi_cif_dev *cdev, uint32_t devno,
		enum rx_mac_clk_e mac_clk)
{
	struct cvi_link *link = &cdev->link[devno];
	struct cif_ctx *ctx;
	u32 addr, data, mask, tmp;
	u32 clk_val = _cif_mac_enum_to_value(mac_clk);

	link->mac_clk = mac_clk;
	ctx = &cdev->link[devno].cif_ctx;

#ifdef FPGA_PORTING
	// FPGA don't set mac spped.
	return 0;
#endif
	/* select the source to vip_sys2 */
	switch (ctx->mac_num) {
	case CIF_MAC_1:
		addr = MAC_CLK_CLK_CSI_MAC1_SRC_SEL;
		mask = MAC_CLK_CLK_CSI_MAC1_SRC_SEL_MASK;
		data = 0x2 << MAC_CLK_CLK_CSI_MAC1_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_2:
		addr = MAC_CLK_CLK_CSI_MAC2_SRC_SEL;
		mask = MAC_CLK_CLK_CSI_MAC2_SRC_SEL_MASK;
		data = 0x2 << MAC_CLK_CLK_CSI_MAC2_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_3:
		addr = MAC_CLK_CLK_CSI_MAC3_SRC_SEL;
		mask = MAC_CLK_CLK_CSI_MAC3_SRC_SEL_MASK;
		data = 0x2 << MAC_CLK_CLK_CSI_MAC3_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_4:
		addr = MAC_CLK_CLK_CSI_MAC4_SRC_SEL;
		mask = MAC_CLK_CLK_CSI_MAC4_SRC_SEL_MASK;
		data = 0x2 << MAC_CLK_CLK_CSI_MAC4_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_5:
		addr = MAC_CLK_CLK_CSI_MAC5_SRC_SEL;
		mask = MAC_CLK_CLK_CSI_MAC5_SRC_SEL_MASK;
		data = 0x2 << MAC_CLK_CLK_CSI_MAC5_SRC_SEL_OFFSET;
		break;
	case CIF_MAC_0:
	default:
		addr = MAC_CLK_CLK_CSI_MAC0_SRC_SEL;
		mask = MAC_CLK_CLK_CSI_MAC0_SRC_SEL_MASK;
		data = 0x2 << MAC_CLK_CLK_CSI_MAC0_SRC_SEL_OFFSET;
		break;
	}
	cif_ops_reg_write_mask(addr + VIP_SYS_ADDRESS, mask, data);

	if (bypass_mac_clk)
		return 0;

	// if (clk_val > cdev->max_mac_clk) {
	// 	dev_err(link->dev, "cannot reach %dM since the max clk is %dM\n", clk_val, cdev->max_mac_clk);
	// 	return -EINVAL;
	// }

	/*A2 no need to set paernt clk, mipipll0 == mipipll1 = 1800M*/
	//if (cdev->max_mac_clk == 900) {
	//	dev_dbg(link->dev, "use div1 fpll as vip_sys2 source\n");
	//	clk_set_parent(cdev->vip_sys2.clk_o, cdev->clk_fpll.clk_o);
	//} else {
	//	dev_dbg(link->dev, "use div0 clk_disppll as vip_sys2 source\n");
	//	clk_set_parent(cdev->vip_sys2.clk_o, cdev->clk_disppll.clk_o);
	//}

	{
	switch (ctx->mac_num) {
	case CIF_MAC_0:
		/* target = source * (1 + ratio) / 32, ratio <= 0x1F */
		/* ratio = target * 32 / source -1*/
		tmp = clk_val * 32 / 900;
		/* roundup */
		if ((clk_val * 32) % 900)
			tmp++;
		tmp--;
		if (clk_val >= 900) {
			tmp = 31;
		}
		break;
	case CIF_MAC_1:
	case CIF_MAC_2:
	case CIF_MAC_3:
	case CIF_MAC_4:
	case CIF_MAC_5:
		/* target = source * (1 + ratio) / 32, ratio <= 0x1F */
		/* ratio = target * 32 / source -1*/
		tmp = clk_val * 32 / 600;
		/* roundup */
		if ((clk_val * 32) % 600)
			tmp++;
		tmp--;
		if (clk_val >= 600) {
			tmp = 31;
		}
		break;
	default:
		break;
	}
	switch (ctx->mac_num) {
	case CIF_MAC_1:
		MAC_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC1, tmp);
		MAC_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC1, 1);
		MAC_UPDATE_CLK_RATIO(SEL_CSI_MAC1);
		break;
	case CIF_MAC_2:
		MAC_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC2, tmp);
		MAC_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC2, 1);
		MAC_UPDATE_CLK_RATIO(SEL_CSI_MAC2);
		break;
	case CIF_MAC_3:
		MAC_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC3, tmp);
		MAC_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC3, 1);
		MAC_UPDATE_CLK_RATIO(SEL_CSI_MAC3);
		break;
	case CIF_MAC_4:
		MAC_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC4, tmp);
		MAC_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC4, 1);
		MAC_UPDATE_CLK_RATIO(SEL_CSI_MAC4);
		break;
	case CIF_MAC_5:
		MAC_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC5, tmp);
		MAC_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC5, 1);
		MAC_UPDATE_CLK_RATIO(SEL_CSI_MAC5);
		break;
	case CIF_MAC_0:
	default:
		MAC_NORM_CLK_RATIO_CONFIG(VAL_CSI_MAC0, tmp);
		MAC_NORM_CLK_RATIO_CONFIG(EN_CSI_MAC0, 1);
		MAC_UPDATE_CLK_RATIO(SEL_CSI_MAC0);
		break;
	}

	//clk_set_rate(clk_get_parent(cdev->vip_sys2.clk_o), cdev->max_mac_clk * 1000000UL);
	dev_dbg(link->dev, "ratio %d, set rate %dM\n", tmp, (tmp + 1) * cdev->max_mac_clk / 32);
	udelay(5);
	}

	return 0;
}
/**********************************************/
/****************set sns mclk******************/
/**********************************************/
#if defined(CONFIG_COMMON_CLK_CVITEK)
struct cam_pll_s {
	unsigned int	div_val_sel;
	unsigned int	src_sel;
	unsigned int	div_val;
};

const struct cam_pll_s cam_pll_setting[CAMPLL_FREQ_NUM] = {
	[CAMPLL_FREQ_37P125M] = {
		.div_val_sel = 1,
		.src_sel = 3,
		.div_val = 16,
	},
	[CAMPLL_FREQ_25M] = {
		.div_val_sel = 1,
		.src_sel = 0,
		.div_val = 72,
	},
	[CAMPLL_FREQ_27M] = {
		.div_val_sel = 1,
		.src_sel = 3,
		.div_val = 22,
	},
	[CAMPLL_FREQ_24M] = {
		.div_val_sel = 1,
		.src_sel = 0,
		.div_val = 75,
	},
	[CAMPLL_FREQ_26M] = {
		.div_val_sel = 1,
		.src_sel = 2,
		.div_val = 25,
	},
};

#else

struct cam_pll_s {
	uint32_t	sync_set;
	uint8_t		div_sel;
	uint8_t		post_div_sel;
	uint8_t		ictrl;
	uint8_t		sel_mode;
	uint8_t		pre_div_sel;
	uint8_t		clk_div;
};

const struct cam_pll_s cam_pll_setting[CAMPLL_FREQ_NUM] = {
	[CAMPLL_FREQ_37P125M] = {
		.sync_set = 406720388UL,
		.div_sel = 12,
		.post_div_sel = 1,
		.ictrl = 0,
		.sel_mode = 1,
		.pre_div_sel = 1,
		.clk_div = 32,
	},
	[CAMPLL_FREQ_25M] = {
		.sync_set = 393705325UL,
		.div_sel = 11,
		.post_div_sel = 1,
		.ictrl = 0,
		.sel_mode = 1,
		.pre_div_sel = 1,
		.clk_div = 45,
	},
	[CAMPLL_FREQ_27M] = {
		.sync_set = 406720388UL,
		.div_sel = 12,
		.post_div_sel = 1,
		.ictrl = 0,
		.sel_mode = 1,
		.pre_div_sel = 1,
		.clk_div = 44,
	},
};
#endif

#include <linux/io.h>
#ifndef FPGA_PORTING
static void __iomem *mclk_enable;
static void __iomem *mclk_update;
static void __iomem *mclk_src;
#endif
#define CIF_MCLK_GEN_BASE 0x28100000
#define CIF_MCLK_GROUP0_PLL 0x2400
#define CIF_MCLK_GROUP1_PLL 0x2500
#define CIF_MCLK_GROUP2_PLL 0x2700
#define CIF_MCLK_GROUP5_PLL 0x2600
#define CIF_MCLK_GROUP6_PLL 0x2900
#define CIF_MCLK0_GEN_OFFSET 0xf0
#define CIF_MCLK1_GEN_OFFSET 0xf4
#define CIF_MCLK2_GEN_OFFSET 0xf8
#define CIF_MCLK0_SRC_OFFSET 0x94
#define CIF_MCLK1_SRC_OFFSET 0xd4
#define CIF_MCLK2_SRC_OFFSET 0xb4
#define CIF_MCLK_UPDATE_OFFSET 0x60
void write_value_range(u32 *data, unsigned int start_bit, unsigned int end_bit, unsigned int value)
{
	unsigned int num_bits;
	unsigned int max_value;
	unsigned int mask;

	if (start_bit > end_bit) {
		printk("[ERROR]: start_bit must be less than or equal to end_bit.\n");
		return;
	}

	num_bits = end_bit - start_bit + 1;
	max_value = (1U << num_bits) - 1;

	if (value > max_value) {
		printk("[ERROR]: value exceeds the range defined by start_bit and end_bit.\n");
		return;
	}

	mask = max_value << start_bit;
	*data = (*data & ~mask) | ((value << start_bit) & mask);
}

int _cif_enable_snsr_clk(struct cvi_cif_dev *cdev, uint32_t devno, uint8_t on)
{
#ifndef FPGA_PORTING
u32 clk_div, clk_src, clk_update;

	if (mclk0 > CAMPLL_FREQ_NONE && mclk0 < CAMPLL_FREQ_NUM) {
		if (on) {
			if (!cdev->clk_cam0.is_on) {
				printk("start set mclk0:%d\n", mclk0);
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK0_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK0_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk0 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk0].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk0].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk0].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam0.is_on = 1;
			}
		} else {
			if (cdev->clk_cam0.is_on) {
				cdev->clk_cam0.is_on = 0;
			}
		}
	}

	if (mclk1 > CAMPLL_FREQ_NONE && mclk1 < CAMPLL_FREQ_NUM) {
		printk("start set mclk1 \n");
		if (on) {
			if (!cdev->clk_cam1.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK1_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK1_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk1 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk1].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk1].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk1].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam1.is_on = 1;
			}
		} else {
			if (cdev->clk_cam1.is_on) {
				cdev->clk_cam1.is_on = 0;
			}
		}
	}

	if (mclk2 > CAMPLL_FREQ_NONE && mclk2 < CAMPLL_FREQ_NUM) {
		printk("start set mclk2 \n");
		if (on) {
			if (!cdev->clk_cam2.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK2_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK2_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP0_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk2 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk2].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk2].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk2].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam2.is_on = 1;
			}
		} else {
			if (cdev->clk_cam2.is_on) {
				cdev->clk_cam2.is_on = 0;
			}
		}
	}

	if (mclk3 > CAMPLL_FREQ_NONE && mclk3 < CAMPLL_FREQ_NUM) {
		printk("start set mclk3 \n");
		if (on) {
			if (!cdev->clk_cam3.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK0_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK0_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk3 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk3].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk3].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk3].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam3.is_on = 1;
			}
		} else {
			if (cdev->clk_cam3.is_on) {
				cdev->clk_cam3.is_on = 0;
			}
		}
	}

	if (mclk4 > CAMPLL_FREQ_NONE && mclk4 < CAMPLL_FREQ_NUM) {
		printk("start set mclk4 \n");
		if (on) {
			if (!cdev->clk_cam4.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK1_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK1_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk4 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk4].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk4].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk4].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam4.is_on = 1;
			}
		} else {
			if (cdev->clk_cam4.is_on) {
				cdev->clk_cam4.is_on = 0;
			}
		}
	}

	if (mclk5 > CAMPLL_FREQ_NONE && mclk5 < CAMPLL_FREQ_NUM) {
		printk("start set mclk5 \n");
		if (on) {
			if (!cdev->clk_cam5.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK2_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK2_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP1_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk5 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk5].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk5].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk5].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam5.is_on = 1;
			}
		} else {
			if (cdev->clk_cam5.is_on) {
				cdev->clk_cam5.is_on = 0;
			}
		}
	}

	if (mclk6 > CAMPLL_FREQ_NONE && mclk6 < CAMPLL_FREQ_NUM) {
		printk("start set mclk6 \n");
		if (on) {
			if (!cdev->clk_cam6.is_on) {
				mclk_enable = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP2_PLL + CIF_MCLK0_GEN_OFFSET,
					sizeof(u32));
				mclk_src = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP2_PLL + CIF_MCLK0_SRC_OFFSET,
					sizeof(u32));
				mclk_update = ioremap(CIF_MCLK_GEN_BASE + CIF_MCLK_GROUP2_PLL + CIF_MCLK_UPDATE_OFFSET,
					sizeof(u32));
				if (!mclk_enable || !mclk_src || !mclk_update) {
					printk("Failed to map mclk6 register\n");
					return -ENOMEM;
				}
				clk_div = readl(mclk_enable);
				clk_src = readl(mclk_src);
				clk_update = readl(mclk_update);
				write_value_range(&clk_div, 16, 23, cam_pll_setting[mclk6].div_val);
				write_value_range(&clk_src, 2, 3, cam_pll_setting[mclk6].src_sel);
				write_value_range(&clk_update, 0, 0, cam_pll_setting[mclk6].div_val_sel);
				writel(clk_div, mclk_enable);
				writel(clk_src, mclk_src);
				writel(clk_update, mclk_update);
				iounmap(mclk_enable);
				iounmap(mclk_src);
				iounmap(mclk_update);
				cdev->clk_cam6.is_on = 1;
			}
		} else {
			if (cdev->clk_cam6.is_on) {
				cdev->clk_cam6.is_on = 0;
			}
		}
	}
#endif
	return 0;
}
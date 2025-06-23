/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LINUX_CDNSP_SKY1_H
#define __LINUX_CDNSP_SKY1_H
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/gpio/consumer.h>

#define USB_MODE_STRAP_S5_DOMAIN 0x424

#define MODE_STRAP_OTG		0
#define MODE_STRAP_HOST		0x1
#define MODE_STRAP_DEVICE	0x2

#define U3_TYPEC_DRD_ID		0
#define U3_TYPEC_HOST0_ID	1
#define U3_TYPEC_HOST1_ID	2
#define U3_TYPEC_HOST2_ID	3
#define U3_TYPEA_CTRL0_ID	4
#define U3_TYPEA_CTRL1_ID	5
#define U2_HOST0_ID		6
#define U2_HOST1_ID		7
#define U2_HOST2_ID		8
#define U2_HOST3_ID		9
#define SKY1_USB_S5_NUM		10

#define U3_TYPEC_DRD_MODE_STRAP_BIT		12
#define U3_TYPEC_HOST0_MODE_STRAP_BIT		14
#define U3_TYPEC_HOST1_MODE_STRAP_BIT		16
#define U3_TYPEC_HOST2_MODE_STRAP_BIT		18
#define U3_TYPEA_CTRL0_MODE_STRAP_BIT		8
#define U3_TYPEA_CTRL1_MODE_STRAP_BIT		10
#define U2_HOST0_MODE_STRAP_BIT			0
#define U2_HOST1_MODE_STRAP_BIT			2
#define U2_HOST2_MODE_STRAP_BIT			4
#define U2_HOST3_MODE_STRAP_BIT			6

#define AXI_HALT				BIT(31)
#define AXI_BMAX_VALUE_DEFAULT			0x7

#define D_XEC_CFG_3XPORT_MODE		0x2040
#define D_XEC_AXI_CAP			0x2174
#define D_XEC_AXI_CTRL0			0x217C
#define D_XEC_PRE_REG_250NS		0x21E8
#define D_XEC_PRE_REG_1US		0x21EC
#define D_XEC_PRE_REG_10US		0x21F0
#define D_XEC_PRE_REG_100US		0x21F4
#define D_XEC_PRE_REG_125US		0x21F8
#define D_XEC_PRE_REG_1MS		0x21FC
#define D_XEC_PRE_REG_10MS		0x2200
#define D_XEC_PRE_REG_100MS		0x2204
#define D_XEC_LPM_PRE_REG_250NS		0x2208
#define D_XEC_LPM_PRE_REG_1US		0x220C
#define D_XEC_LPM_PRE_REG_10US		0x2210
#define D_XEC_LPM_PRE_REG_100US		0x2214
#define D_XEC_LPM_PRE_REG_125US		0x2218
#define D_XEC_LPM_PRE_REG_1MS		0x221C
#define D_XEC_LPM_PRE_REG_10MS		0x2220
#define D_XEC_LPM_PRE_REG_100MS		0x2224

#define XEC_CFG_3XPORT_MODE		0x2040
#define XEC_PRE_REG_250NS		0x21E8
#define XEC_PRE_REG_1US			0x21EC
#define XEC_PRE_REG_10US		0x21F0
#define XEC_PRE_REG_100US		0x21F4
#define XEC_PRE_REG_125US		0x21F8
#define XEC_PRE_REG_1MS			0x21FC
#define XEC_PRE_REG_10MS		0x2200
#define XEC_PRE_REG_100MS		0x2204
#define XEC_LPM_PRE_REG_250NS		0x2208
#define XEC_LPM_PRE_REG_1US		0x220C
#define XEC_LPM_PRE_REG_10US		0x2210
#define XEC_LPM_PRE_REG_100US		0x2214
#define XEC_LPM_PRE_REG_125US		0x2218
#define XEC_LPM_PRE_REG_1MS		0x221C
#define XEC_LPM_PRE_REG_10MS		0x2220
#define XEC_LPM_PRE_REG_100MS		0x2224
#define XEC_USBSSP_CHICKEN_BITS_3	0x2230

#define D_XEC_CFG_3XPORT_MODE_VALUE		0xa0031e07
#define XEC_CFG_3XPORT_MODE_VALUE		0xa0031e07
#define CFG_3XPORT_MODE_DIS_SSP		(~(1 << 31))

#define APB_TIMEOUT_VALUE_50MS_FREQ_200M	0x2710
#define APB_TIMEOUT_MASK			(~((1 << 22) - 1))

#define CIX_USB_CLK_NUM				(4)
#define CIX_USB_CLK_OFF_NUM			(2)
#define CIX_USB_AXI_WR_CACHE_VALUE		0X33
#define CIX_USB_CLK_32K				32000
#define CIX_USB_CLK_4M				4000000
#define CIX_USB_CLK_8M				8000000
#define CIX_USB_CLK_24M				24000000

#define AXI_CLOCK_ENABLE		BIT(0)
#define AXI_CLOCK_VALID			BIT(1)
#define AXI_CLOCK_REQ			BIT(2)

struct cdnsp_sky1 {
	struct device *dev;
	void __iomem *axi_base;
	void __iomem *ctst_base;
	void __iomem *dr_base;
	void __iomem *xhci_base;
	void __iomem *device_base;
	struct platform_device *cdnsp_pdev;
	struct reset_control *reset;
	struct reset_control *preset;
	struct clk *cix_usb_clks[CIX_USB_CLK_NUM];
	int id;
	struct regmap	*usb_syscon;
	int lpm_clk_freq;
	int sof_clk_freq;
	bool u3_disable;
	bool ssp_disable;
	int axi_bmax_value;
	struct gpio_desc * oc_gpio;
};

#endif /* __LINUX_CDNSP_SKY1_H */
